// ========================================================================
// $File: //jeffr/granny/rt/granny_skeleton_builder.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_SKELETON_BUILDER_H)
#include "granny_skeleton_builder.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_MEMORY_ARENA_H)
#include "granny_memory_arena.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode SkeletonLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct skeleton_builder_bone
{
    int32x Parent;
    transform LocalTransform;

    matrix_4x4 InverseWorld4x4;

    real32 LODError;
};

struct skeleton_builder
{
    int32x MaxBoneCount;
    int32x BoneCount;
    skeleton_builder_bone *Bones;
    skeleton_lod_type LODType;

    int32x* BoneWritten;
};

END_GRANNY_NAMESPACE;

static void
EstimateSkeletonLOD(skeleton_builder& Builder)
{
    // Alloc our temp space
    memory_arena* LODArena = NewMemoryArena();
    real32* ComputedLOD = (real32*)MemoryArenaPush(*LODArena, Builder.BoneCount * sizeof(real32));
    {for(int32x BoneIndex = 0; BoneIndex < Builder.BoneCount; ++BoneIndex)
    {
        ComputedLOD[BoneIndex] = -1.0f;
    }}

    // Our braindead estimate of this parameter is as follows.  Find
    //  the furthest child bone, and set our error to twice that
    //  distance.  If we wind up with no children, then set our
    //  parameter to half that of our parent.
    // Note that we really need to look at the bounding information to
    //  do this properly, but at this stage, we don't have that info.
    //  Note that if the user has set a lod manually, then we always
    //  use that.  This allows the creation of proper post-processing
    //  tools that take the bounding/vertex data into account.  (See the
    //  preprocessor for an example.)
    // Note also we enforce the parent.error >= child.error condition.

    // These loops probably do unecessary work, but we can't count on
    //  the topo sort property yet.

    // initial computation
    {for(int32x BoneIndex = 0; BoneIndex < Builder.BoneCount; ++BoneIndex)
    {
        triple CurrOffset;
        Copy32(3, Builder.Bones[BoneIndex].LocalTransform.Position, CurrOffset);

        // Walk up the skeleton, and set the parent errors...
        int Walk = Builder.Bones[BoneIndex].Parent;
        while (Walk != -1)
        {
            Assert(Walk >= 0 && Walk < Builder.BoneCount);

            // First, see if this offset causes us to exceed the
            //  currently computed LOD parameter
            const real32 EstError = VectorLength3(CurrOffset);
            if (EstError > ComputedLOD[Walk])
            {
                ComputedLOD[Walk] = EstError;
            }

            // Transform CurrOffset and step to parent
            TransformVectorInPlace(CurrOffset, Builder.Bones[Walk].LocalTransform);
            Walk = Builder.Bones[Walk].Parent;
        }
    }}

    // Copy to the bones, but only if no explicitly set parameter is
    //  in place...
    {for(int32x BoneIndex = 0; BoneIndex < Builder.BoneCount; ++BoneIndex)
    {
        if (Builder.Bones[BoneIndex].LODError < 0.0f)
        {
            Builder.Bones[BoneIndex].LODError = ComputedLOD[BoneIndex];
        }
        else
        {
            // If the user sets a parameter, we assume that this is measured
            // lod.
            Builder.LODType = MeasuredLOD;
        }
    }}

    // Ensure heap property, and catch the leaf bones.
    {for(int32x BoneIndex = 0; BoneIndex < Builder.BoneCount; ++BoneIndex)
    {
        if (Builder.Bones[BoneIndex].LODError < 0.0f)
        {
            if (Builder.Bones[BoneIndex].Parent == -1)
            {
                // Singleton bone, no parent, no children.  There's nothing
                //  good about this situation, just set the LOD to 0.  This
                //  means that as soon as lod engages, this bone will be
                //  shut off if the export estimates are used.
                Builder.Bones[BoneIndex].LODError = 0.0f;
            }
            else
            {
                int ParentIndex = Builder.Bones[BoneIndex].Parent;
                Assert(ParentIndex >= 0 && ParentIndex < Builder.BoneCount);

                // Half the parent
                Builder.Bones[BoneIndex].LODError = Builder.Bones[ParentIndex].LODError / 2;
            }
        }

        // And traverse up for the heap-property
        int Walk = BoneIndex;
        while (Builder.Bones[Walk].Parent != -1)
        {
            int ParentIndex = Builder.Bones[Walk].Parent;
            Assert(ParentIndex >= 0 && ParentIndex < Builder.BoneCount);


            if (Builder.Bones[Walk].LODError > Builder.Bones[ParentIndex].LODError)
                Builder.Bones[ParentIndex].LODError = Builder.Bones[Walk].LODError;

            Walk = ParentIndex;
        }
    }}

    // Clear our temp mem
    FreeMemoryArena(LODArena);
}

// TODO: swap out for insertion sort...
// Yeah, that's right, a bubble sort.  It's not quite as crazy as you
// think, this allows us to avoid the CRT or STL, and the bone array
// is mostly sorted anways, since we enforce the topo sort constraint
// that parents come before children, and a parent's lod must exceed
// that of all of its child nodes.  Note that we do a lot of
// superflous loops here that could be combined, but I want this to be
// super clear about what's going on.
static void
BubbleSortOnLOD(skeleton_builder& Builder,
                bone*             Bones,
                int32x*           RemappingTable)
{
#define BUBBLE_SWAP(type, a, b) do { type Temp = a; a = b; b = Temp; } while (false)

    // Allocate our temp buffers
    memory_arena* SortArena = NewMemoryArena();
    int32x* OldToNewMap = (int32x*)MemoryArenaPush(*SortArena, Builder.BoneCount * sizeof(int32x));
    int32x* NewToOldMap = (int32x*)MemoryArenaPush(*SortArena, Builder.BoneCount * sizeof(int32x));

    {for(int BoneIndex = 0; BoneIndex < Builder.BoneCount; ++BoneIndex)
    {
        OldToNewMap[BoneIndex] = -1;

        // Note that NTO[i] = i to begin
        NewToOldMap[BoneIndex] = BoneIndex;
    }}

    bone* OldBones = (bone*)MemoryArenaPush(*SortArena, Builder.BoneCount * sizeof(bone));
    Copy(sizeof(bone) * Builder.BoneCount, Bones, OldBones);

    {for(int i = Builder.BoneCount - 1; i >= 0; --i)
    {
        {for(int j = 0; j <= i - 1; ++j)
        {
            if (Bones[j].LODError < Bones[j + 1].LODError)
            {
                // Swap the elements that we care about
                BUBBLE_SWAP(int32x, NewToOldMap[j], NewToOldMap[j + 1]);
                BUBBLE_SWAP(bone,   Bones[j], Bones[j+1]);
            }
        }}
    }}

    // Invert the new-to-old map to get our proper ordering...
    {for(int i = 0; i < Builder.BoneCount; ++i)
    {
        OldToNewMap[NewToOldMap[i]] = i;
    }}

    if (RemappingTable)
    {
        // Remap the Remapping table.  Note that we're going to use the
        // NewToOldMap as temp space, since we're done with it.
        {for(int i = 0; i < Builder.BoneCount; ++i)
        {
            RemappingTable[i] = OldToNewMap[RemappingTable[i]];
        }}
    }

    // And clean up the parent indices...
    {for(int BoneIndex = 0; BoneIndex < Builder.BoneCount; ++BoneIndex)
    {
        if (Bones[BoneIndex].ParentIndex != NoParentBone)
        {
            int32x OldParentIndex = Bones[BoneIndex].ParentIndex;
            int32x NewParentIndex = OldToNewMap[OldParentIndex];

            Bones[BoneIndex].ParentIndex = NewParentIndex;
        }
    }}

#ifdef DEBUG
    // Do some checking...
    {for(int BoneIndex = 0; BoneIndex < Builder.BoneCount; ++BoneIndex)
    {
        if (BoneIndex != 0)
        {
            Assert(Bones[BoneIndex-1].LODError >= Bones[BoneIndex].LODError);
        }

        if (Bones[BoneIndex].ParentIndex != NoParentBone)
        {
            Assert(Bones[BoneIndex].ParentIndex >= 0 &&
                   Bones[BoneIndex].ParentIndex < Builder.BoneCount);
            Assert(Bones[BoneIndex].ParentIndex < BoneIndex);
        }
    }}
#endif // DEBUG

    // Clear our temp mem
    FreeMemoryArena(SortArena);

#undef BUBBLE_SWAP
}


skeleton_builder *GRANNY
BeginSkeleton(int32x BoneCount)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    skeleton_builder *Builder;
    AggrAllocPtr(Allocator, Builder);
    AggrAllocOffsetArrayPtr(Allocator, Builder, BoneCount, MaxBoneCount, Bones);
    AggrAllocOffsetArrayPtr(Allocator, Builder, BoneCount, MaxBoneCount, BoneWritten);
    if(EndAggrAlloc(Allocator))
    {
        Builder->BoneCount = 0;
        Builder->LODType = EstimatedLOD;

        {for(int32x BoneIndex = 0;
             BoneIndex < BoneCount;
             ++BoneIndex)
        {
            Builder->BoneWritten[BoneIndex] = -1;
        }}
    }

    return(Builder);
}

static void
AggrSkeleton(aggr_allocator &Allocator,
             skeleton_builder const &Builder,
             skeleton *&Skeleton)
{
    AggrAllocPtr(Allocator, Skeleton);
    AggrAllocOffsetArrayPtr(Allocator, Skeleton, Builder.BoneCount,
                            BoneCount, Bones);
}

skeleton *GRANNY
EndSkeleton(skeleton_builder *Builder, int32x *RemappingTable)
{
    skeleton *Skeleton = 0;

    if(Builder)
    {
        int32x TotalSize = GetResultingSkeletonSize(*Builder);
        Skeleton = EndSkeletonInPlace(Builder, AllocateSize(TotalSize),
                                      RemappingTable);
    }

    return(Skeleton);
}

int32x GRANNY
GetResultingSkeletonSize(skeleton_builder const &Builder)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    skeleton *Ignored;
    AggrSkeleton(Allocator, Builder, Ignored);

    int32x ResultingSize;
    CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
    return ResultingSize;
}

static void
WriteBone(skeleton_builder &Builder,
          int32x InBoneIndex,
          int32x &OutBoneIndex,
          bone *BoneData,
          int32x *RemappingTable)
{
    if(Builder.BoneWritten[InBoneIndex] == -2)
    {
        Log1(ErrorLogMessage, SkeletonLogMessage,
             "Circular parenting chain located at bone %d",
             InBoneIndex);
        return;
    }

    if(Builder.BoneWritten[InBoneIndex] == -1)
    {
        Builder.BoneWritten[InBoneIndex] = -2;

        int32x ParentIndex = NoParentBone;
        skeleton_builder_bone const &Bone = Builder.Bones[InBoneIndex];
        if(Bone.Parent != NoParentBone)
        {
            WriteBone(Builder, Bone.Parent, OutBoneIndex,
                      BoneData, RemappingTable);
            Assert(Builder.BoneWritten[Bone.Parent] != -1);
            ParentIndex = Builder.BoneWritten[Bone.Parent];
        }

        MakeEmptyDataTypeObject(BoneType, &BoneData[OutBoneIndex]);
        BoneData[OutBoneIndex].ParentIndex = ParentIndex;
        BoneData[OutBoneIndex].LODError = Bone.LODError;
        BoneData[OutBoneIndex].LocalTransform = Bone.LocalTransform;

        Copy(sizeof(Bone.InverseWorld4x4),
             Bone.InverseWorld4x4,
             BoneData[OutBoneIndex].InverseWorld4x4);

        if(RemappingTable)
        {
            RemappingTable[InBoneIndex] = OutBoneIndex;
        }

        Builder.BoneWritten[InBoneIndex] = OutBoneIndex;
        ++OutBoneIndex;
    }
}

skeleton *GRANNY
EndSkeletonInPlace(skeleton_builder *Builder,
                   void *InMemory,
                   int32x *RemappingTable)
{
    skeleton *Skeleton = 0;

    if(Builder)
    {
        // Estimate the skeleton lod parameters.  Note that we'll use
        // that parameter to sort the skeleton later
        EstimateSkeletonLOD(*Builder);

        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        AggrSkeleton(Allocator, *Builder, Skeleton);
        if(EndAggrPlacement(Allocator, InMemory))
        {
            // Techinically, we could allow the loop below to write
            // these bones out in the course of the recursive
            // WriteBone calls, but this gets all the parents to the
            // top of the tree by default, which is where they belong,
            // for now.  (We'll possibly spoil that ordering by
            // sorting on skeleton lod)
            int32x OutBoneIndex = 0;
            {for(int32x BoneIndex = 0;
                 BoneIndex < Skeleton->BoneCount;
                 ++BoneIndex)
            {
                skeleton_builder_bone &Bone = Builder->Bones[BoneIndex];

                if(Bone.Parent == -1)
                {
                    WriteBone(*Builder, BoneIndex, OutBoneIndex,
                              Skeleton->Bones, RemappingTable);
                }
            }}

            {for(int32x BoneIndex = 0;
                 BoneIndex < Skeleton->BoneCount;
                 ++BoneIndex)
            {
                skeleton_builder_bone &Bone = Builder->Bones[BoneIndex];

                if(Bone.Parent != -1)
                {
                    WriteBone(*Builder, BoneIndex, OutBoneIndex,
                              Skeleton->Bones, RemappingTable);
                }
            }}
            Assert(OutBoneIndex == Skeleton->BoneCount);

            BubbleSortOnLOD(*Builder, Skeleton->Bones, RemappingTable);
            Skeleton->LODType = Builder->LODType;
        }

        Deallocate(Builder);
    }

    return(Skeleton);
}

void GRANNY
AddBoneWithInverse(skeleton_builder &Builder,
                   real32 const *LocalPosition,
                   real32 const *LocalOrientation,
                   real32 const *LocalScaleShear,
                   matrix_4x4 const *InverseWorld4x4)
{
    if(Builder.BoneCount < Builder.MaxBoneCount)
    {
        int32x BoneIndex = Builder.BoneCount++;
        skeleton_builder_bone &Bone = Builder.Bones[BoneIndex];
        Bone.Parent = -1;
        Bone.LODError = -1.0f;
        SetTransformWithIdentityCheck(Bone.LocalTransform,
                                      LocalPosition, LocalOrientation,
                                      LocalScaleShear);

        // Copy the inverse transform.  Note that we can't check the
        //  inverse precision here, since we don't have the forward
        //  transform
        Copy(sizeof(Bone.InverseWorld4x4),
             InverseWorld4x4,
             Bone.InverseWorld4x4);
    }
    else
    {
        Log0(ErrorLogMessage, SkeletonLogMessage,
             "AddBone called after all allotted bones were already used");
    }
}

void GRANNY
AddBone(skeleton_builder &Builder,
        real32 const *LocalPosition,
        real32 const *LocalOrientation,
        real32 const *LocalScaleShear,
        real32 const *WorldPosition,
        real32 const *WorldOrientation,
        real32 const *WorldScaleShear)
{
    real32 World[4][4];
    transform WorldTransform;
    matrix_4x4 InverseWorld4x4;
    SetTransformWithIdentityCheck(WorldTransform,
                                  WorldPosition, WorldOrientation,
                                  WorldScaleShear);
    BuildCompositeTransform4x4(WorldTransform, (real32 *)World);
    MatrixInvert4x3((real32 *)InverseWorld4x4,
                    (real32 const *)World);

    AddBoneWithInverse(Builder,
                       LocalPosition, LocalOrientation, LocalScaleShear,
                       (matrix_4x4 const*)&InverseWorld4x4);

    // Check the inverse precision...
#if DEBUG
        real32 Test1[4][4], Test2[4][4];
        BuildCompositeTransform4x4(WorldTransform,
                                   (real32 *)Test1);
        ColumnMatrixMultiply4x3(
            (real32 *)Test2,
            (real32 const *)Test1,
            (real32 const *)InverseWorld4x4);

        real32 TotalError = 0.0f;
        {for(int32x Row = 0;
             Row < 4;
             ++Row)
        {
            {for(int32x Column = 0;
                 Column < 4;
                 ++Column)
            {
                TotalError += AbsoluteValue(GlobalIdentity4x4[Row][Column] -
                                            Test2[Row][Column]);
            }}
        }}

        if(TotalError > 0.0001f)
        {
            Log0(WarningLogMessage, SkeletonLogMessage,
                 "Matrix inverse is not sufficiently precise");
        }
#endif
}

void GRANNY
SetBoneParent(skeleton_builder &Builder, int32x BoneIndex, int32x ParentIndex)
{
    if(BoneIndex < Builder.MaxBoneCount)
    {
        if(ParentIndex < Builder.MaxBoneCount)
        {
            Builder.Bones[BoneIndex].Parent = ParentIndex;
        }
        else
        {
            Log2(ErrorLogMessage, SkeletonLogMessage,
                 "SetBoneParent parent index (%d) is out of specified bone "
                 "domain (%d)", BoneIndex, Builder.MaxBoneCount);
        }
    }
    else
    {
        Log2(ErrorLogMessage, SkeletonLogMessage,
             "SetBoneParent bone index (%d) is out of specified bone "
             "domain (%d)", BoneIndex, Builder.MaxBoneCount);
    }
}


void GRANNY
SetBoneLODError(skeleton_builder &Builder,
                int32x BoneIndex,
                real32 LODError)
{
    if(BoneIndex < Builder.MaxBoneCount)
    {
        if(BoneIndex < Builder.MaxBoneCount)
        {
            Builder.Bones[BoneIndex].LODError = LODError;
        }
        else
        {
            Log2(ErrorLogMessage, SkeletonLogMessage,
                 "SetBoneParent parent index (%d) is out of specified bone "
                 "domain (%d)", BoneIndex, Builder.MaxBoneCount);
        }
    }
    else
    {
        Log2(ErrorLogMessage, SkeletonLogMessage,
             "SetBoneParent bone index (%d) is out of specified bone "
             "domain (%d)", BoneIndex, Builder.MaxBoneCount);
    }
}
