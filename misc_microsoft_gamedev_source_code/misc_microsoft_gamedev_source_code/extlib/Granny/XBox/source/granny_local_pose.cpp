// ========================================================================
// $File: //jeffr/granny/rt/granny_local_pose.cpp $
// $DateTime: 2007/08/24 14:47:10 $
// $Change: 15826 $
// $Revision: #33 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_LOCAL_POSE_H)
#include "granny_local_pose.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_WORLD_POSE_H)
#include "granny_world_pose.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_TRACK_MASK_H)
#include "granny_track_mask.h"
#endif

// For FindAllowedErrorNumbers
#if !defined(GRANNY_CONTROLLED_ANIMATION_H)
#include "granny_controlled_animation.h"
#endif

#if !defined(GRANNY_PREFETCH_H)
#include "granny_prefetch.h"
#endif


#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif


#undef SubsystemCode
#define SubsystemCode LocalPoseLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

// don't break this!  It's important for the PS3
CompileAssert(IS_ALIGNED_16(SizeOf(local_pose_transform)));
#define LocalPoseAlignment 16

data_type_definition LocalPoseTransformType[] =
{
    { Real32Member, "Weight" },
    { Int32Member, "Count" },
    { TransformMember, "Transform" },
    { Int32Member, "TraversalID" },

    { EndMember }
};

data_type_definition LocalPoseType[] =
{
    { ReferenceToArrayMember, "Transforms", LocalPoseTransformType },
    { Real32Member, "FillThreshold" },
    { Int32Member, "TraversalID" },

    { EndMember }
};



END_GRANNY_NAMESPACE;

local_pose *GRANNY
NewLocalPose(int32x BoneCount)
{
    local_pose *Pose =
        NewLocalPoseInPlace(BoneCount,
                            AllocateSizeAligned(LocalPoseAlignment,
                                                GetResultingLocalPoseSize(BoneCount)));
    return Pose;
}

void GRANNY
FreeLocalPose(local_pose *LocalPose)
{
    Deallocate(LocalPose);
}

static void
AggrLocalPose(aggr_allocator &Allocator, int32x BoneCount, local_pose *&Pose)
{
    SetAggrAlignment(Allocator, LocalPoseAlignment);
    AggrAllocPtr(Allocator, Pose);
    // TODO: Should I remove the flags field from the transform structure
    // so I can go ahead and make the local pose array always line up on 32-byte
    // boundaries?
    AggrAllocOffsetArrayPtr(Allocator, Pose, BoneCount, BoneCount, Transforms);
}

int32x GRANNY
GetResultingLocalPoseSize(int32x BoneCount)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    local_pose *Pose;
    AggrLocalPose(Allocator, BoneCount, Pose);

    int32x ResultingSize;
    CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
    return ResultingSize;
}

local_pose* GRANNY
NewLocalPoseInPlace(int32x BoneCount, void *Memory)
{
    CheckCondition(IS_ALIGNED_16(Memory), return NULL);
    local_pose *Pose = 0;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    AggrLocalPose(Allocator, BoneCount, Pose);
    if(EndAggrPlacement(Allocator, Memory))
    {
        Assert(IS_ALIGNED_16(Pose));
        Assert(IS_ALIGNED_16(Pose->Transforms));
        // TODO: This is a somewhat arbitrary value.  Should I try to
        // determine a more rigorous value here?
        Pose->FillThreshold = DefaultLocalPoseFillThreshold;

        Pose->TraversalID = 0;
        ZeroArray(BoneCount, Pose->Transforms);
    }

    return(Pose);
}

int32x GRANNY
GetLocalPoseBoneCount(local_pose const &LocalPose)
{
    return(LocalPose.BoneCount);
}

transform *GRANNY
GetLocalPoseTransform(local_pose const &LocalPose, int32x BoneIndex)
{
    CheckCountedInt32(BoneIndex, LocalPose.BoneCount, return(0));

    return(&LocalPose.Transforms[BoneIndex].Transform);
}

void* GRANNY
GetLocalPoseOpaqueTransformArray(local_pose const& LocalPose)
{
    return (void*)LocalPose.Transforms;
}


void GRANNY
BuildRestLocalPose(skeleton const &Skeleton,
                   int32x FirstBone, int32x BoneCount,
                   local_pose &LocalPose)
{
    CheckCountedInt32(FirstBone, LocalPose.BoneCount, return);
    CheckBoundedInt32(0, FirstBone + BoneCount, LocalPose.BoneCount, return);

    local_pose_transform *Local = LocalPose.Transforms + FirstBone;
    {for(int32x LocalPoseBoneIndex = FirstBone;
         LocalPoseBoneIndex < (FirstBone + BoneCount);
         ++LocalPoseBoneIndex)
    {
        // No sparse version for now...
        int32x SkeletonBoneIndex = LocalPoseBoneIndex;
//         if ( SparseBoneArray != NULL )
//         {
//             // This is a mapping from the LocalPose array to the skeleton's bones.
//             SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex];
//         }

        // Just set the transform to the reference transform.
        Local->Weight = 1.0f;
        Local->Count  = 1;
        Local->Transform = Skeleton.Bones[SkeletonBoneIndex].LocalTransform;

        ++Local;
    }}
}


void GRANNY
BeginLocalPoseAccumulation(local_pose &LocalPose,
                           int32x FirstBone, int32x BoneCount, int32x const *SparseBoneArray)
{
    ++LocalPose.TraversalID;
}

void GRANNY
AccumulateLocalTransform(local_pose &LocalPose,
                         int32x LocalPoseBoneIndex, int32x SkeletonBoneIndex,
                         real32 Weight,
                         skeleton const &ReferenceSkeleton,
                         quaternion_mode Mode,
                         transform const &Transform)
{
    local_pose_transform &Local = LocalPose.Transforms[LocalPoseBoneIndex];

    real32 QuaternionWeight;
    switch(Mode)
    {
        default:
        case BlendQuaternionDirectly:
        {
            QuaternionWeight = Weight;
        } break;

        case BlendQuaternionInverted:
        {
            QuaternionWeight = -Weight;
        } break;

        case BlendQuaternionNeighborhooded:
        {
            transform const &ReferenceTransform =
                ReferenceSkeleton.Bones[SkeletonBoneIndex].LocalTransform;
            real32 IP = InnerProduct4(ReferenceTransform.Orientation,
                                      Transform.Orientation);
            QuaternionWeight = (IP >= 0) ? Weight : -Weight;
        } break;

        case BlendQuaternionAccumNeighborhooded:
        {
            real32 IP;
            if (Local.TraversalID != LocalPose.TraversalID)
            {
                // first time through
                transform const &ReferenceTransform =
                    ReferenceSkeleton.Bones[SkeletonBoneIndex].LocalTransform;
                IP = InnerProduct4(ReferenceTransform.Orientation,
                                   Transform.Orientation);
            }
            else
            {
                // neighborhood to the blend in progress...
                IP = InnerProduct4(Local.Transform.Orientation,
                                   Transform.Orientation);
            }
            QuaternionWeight = (IP >= 0) ? Weight : -Weight;
        } break;
    }

    if(Local.TraversalID == LocalPose.TraversalID)
    {
        // We've already started accumulating, so there's data in there.
        Local.Transform.Flags |= Transform.Flags;

        ScaleVectorAdd3(Local.Transform.Position, Weight, Transform.Position);

        ScaleVectorAdd4(Local.Transform.Orientation,
                        QuaternionWeight, Transform.Orientation);

        ScaleMatrixAdd3x3((real32 *)Local.Transform.ScaleShear, Weight,
                          (real32 *)Transform.ScaleShear);

        Local.Weight += Weight;
        ++Local.Count;
    }
    else
    {
        // First time this transform has been accumulated to this time around, so the existing data is ignored.
        if(Weight == 1.0f)
        {
            Local.Transform = Transform;
        }
        else
        {
            Local.Transform.Flags = Transform.Flags;
            VectorScale3(Local.Transform.Position, Weight, Transform.Position);
            MatrixScale3x3((real32 *)Local.Transform.ScaleShear, Weight,
                           (real32 *)Transform.ScaleShear);
        }
        VectorScale4(Local.Transform.Orientation,
                     QuaternionWeight, Transform.Orientation);

        Local.Weight = Weight;
        Local.Count = 1;
        Local.TraversalID = LocalPose.TraversalID;
    }
}

void GRANNY
EndLocalPoseAccumulation(local_pose &LocalPose,
                         int32x FirstBone, int32x BoneCount,
                         skeleton const &ReferenceSkeleton, int32x const *SparseBoneArray)
{
    EndLocalPoseAccumulationLOD(LocalPose, FirstBone, BoneCount,
                                ReferenceSkeleton, 0.0f,
                                SparseBoneArray);
}

void GRANNY
EndLocalPoseAccumulationLOD(local_pose &LocalPose,
                            int32x FirstBone, int32x BoneCount,
                            skeleton const &ReferenceSkeleton,
                            real32 AllowedError,
                            int32x const *SparseBoneArray)
{
    CheckCountedInt32(FirstBone, LocalPose.BoneCount, return);
    CheckBoundedInt32(0, FirstBone + BoneCount, LocalPose.BoneCount, return);

    real32 AllowedErrorEnd;
    real32 AllowedErrorScaler;
    if (ReferenceSkeleton.LODType == NoSkeletonLOD)
    {
        AllowedError       = 0.0f;
        AllowedErrorEnd    = 0.0f;
        AllowedErrorScaler = 0.0f;
    }
    else
    {
        FindAllowedErrorNumbers(AllowedError, &AllowedErrorEnd, &AllowedErrorScaler);
    }

    local_pose_transform *Local = LocalPose.Transforms + FirstBone;
    {for(int32x LocalPoseBoneIndex = FirstBone;
         LocalPoseBoneIndex < (FirstBone + BoneCount);
         ++LocalPoseBoneIndex)
    {
        if (LocalPoseBoneIndex < (FirstBone + BoneCount) - 1)
        {
            int32x SkeletonBoneIndex = LocalPoseBoneIndex+1;
            if ( SparseBoneArray != NULL )
            {
                // This is a mapping from the LocalPose array to the skeleton's bones.
                SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex+1];
            }
            PrefetchAddress(&ReferenceSkeleton.Bones[SkeletonBoneIndex], sizeof(bone));
        }

        int32x SkeletonBoneIndex = LocalPoseBoneIndex;
        if ( SparseBoneArray != NULL )
        {
            // This is a mapping from the LocalPose array to the skeleton's bones.
            SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex];
        }

        if(Local->TraversalID == LocalPose.TraversalID)
        {
            if(Local->Weight < LocalPose.FillThreshold)
            {
                AccumulateLocalTransform(LocalPose, LocalPoseBoneIndex, SkeletonBoneIndex,
                                         LocalPose.FillThreshold - Local->Weight,
                                         ReferenceSkeleton,
                                         BlendQuaternionAccumNeighborhooded,
                                         ReferenceSkeleton.Bones[SkeletonBoneIndex].LocalTransform);
            }

            // Determine what blending regime the bone is in
            // w.r.t. the allowed error
            const real32 BoneError = ReferenceSkeleton.Bones[SkeletonBoneIndex].LODError;
            real32 BlendFactor = ( AllowedError - BoneError ) * AllowedErrorScaler;
            if (BlendFactor <= 0.0f)
            {
                // No change, this bone is not lerped
            }
            else if (BlendFactor < 0.99f)  // cap to prevent ludicrous weights
            {
                // Since the localpose already has weight, we need to compute a new weight
                //  such that the accumulate we're going to do here does the correct lerp.
                //  In this case, the correct factor is (Weight / (1 - BlendFactor))
                BlendFactor *= (Local->Weight / (1 - BlendFactor));
                AccumulateLocalTransform(LocalPose, LocalPoseBoneIndex, SkeletonBoneIndex,
                                         BlendFactor,
                                         ReferenceSkeleton,
                                         BlendQuaternionNeighborhooded,
                                         ReferenceSkeleton.Bones[SkeletonBoneIndex].LocalTransform);
            }
            else
            {
                // Just set the transform to the reference transform.
                Local->Weight = 1.0f;
                Local->Count  = 1;
                Local->Transform = ReferenceSkeleton.Bones[SkeletonBoneIndex].LocalTransform;
            }

#if DEBUG
            quad OriginalOrientation;
            VectorEquals4(OriginalOrientation,
                          Local->Transform.Orientation);
            real32 OriginalLength = VectorLength4(OriginalOrientation);
            OriginalLength = OriginalLength;        // Stops warnings.
#endif

            if((Local->Count != 1) || (Local->Weight != 1.0f))
            {
                real32 const InverseTotalWeight = 1.0f / Local->Weight;
                VectorScale3(Local->Transform.Position,
                             InverseTotalWeight);
                MatrixScale3x3((real32 *)Local->Transform.ScaleShear,
                               InverseTotalWeight);
            }
            NormalizeWithTest4(Local->Transform.Orientation);

#if DEBUG
            real32 OrientationLength =
                VectorLength4(Local->Transform.Orientation);
            Assert(AbsoluteValue(OrientationLength - 1.0f) < 0.001f);
#endif
        }
        else
        {
            Local->Transform = ReferenceSkeleton.Bones[SkeletonBoneIndex].LocalTransform;
        }

        ++Local;
    }}
}

real32 GRANNY
GetLocalPoseFillThreshold(local_pose const &LocalPose)
{
    return(LocalPose.FillThreshold);
}

void GRANNY
SetLocalPoseFillThreshold(local_pose &LocalPose, real32 FillThreshold)
{
    LocalPose.FillThreshold = FillThreshold;
}

void GRANNY
GetWorldMatrixFromLocalPose(skeleton const &Skeleton,
                            int32x BoneIndex,
                            local_pose const &LocalPose,
                            real32 const *Offset4x4,
                            real32 *Result4x4,
                            int32x const *SparseBoneArray,
                            int32x const *SparseBoneArrayReverse)
{
    matrix_4x4 Temp, Temp2;

    MatrixIdentity4x4(Result4x4);
    CheckCountedInt32(BoneIndex, GetLocalPoseBoneCount(LocalPose), return);

    while(BoneIndex != NoParentBone)
    {
        transform *Local;
        if ( SparseBoneArrayReverse != NULL )
        {
            // The local pose is sparsely mapped - do the indirection.
            Local = GetLocalPoseTransform(LocalPose, SparseBoneArrayReverse[BoneIndex]);
        }
        else
        {
            // Direct mapping.
            Local = GetLocalPoseTransform(LocalPose, BoneIndex);
        }

        BuildCompositeTransform4x4(*Local, (real32 *)Temp);
        ColumnMatrixMultiply4x3((real32 *)*Temp2, (real32 const *)Result4x4,
                                (real32 const *)Temp);
        MatrixEquals4x4(Result4x4, (real32 const *)Temp2);
        BoneIndex = Skeleton.Bones[BoneIndex].ParentIndex;
    }

    if(Offset4x4)
    {
        ColumnMatrixMultiply4x3((real32 *)*Temp2, (real32 const *)Result4x4,
                                (real32 const *)Offset4x4);
        MatrixEquals4x4(Result4x4, (real32 const *)Temp2);
    }
}

void GRANNY
GetAttachmentOffset(skeleton const &Skeleton,
                    int32x BoneIndex,
                    local_pose const &LocalPose,
                    real32 const *Offset4x4,
                    real32 *Result4x4,
                    int32x const *SparseBoneArray,
                    int32x const *SparseBoneArrayReverse)
{
    matrix_4x4 Temp;
    GetWorldMatrixFromLocalPose(Skeleton, BoneIndex,
                LocalPose, Offset4x4, (real32 *)Temp, SparseBoneArray, SparseBoneArrayReverse);
    MatrixInvert4x3(Result4x4, (real32 const *)Temp);
}

void GRANNY
ModulationCompositeLocalPoseSparse(local_pose &BasePose,
                                   real32 WeightNone, real32 WeightAll,
                                   track_mask *Mask,
                                   local_pose const &CompositePose,
                                   int32x const *SparseBoneArray)
{
    int32x const BoneCount = GetLocalPoseBoneCount(BasePose);
    CheckEqualInt32(GetLocalPoseBoneCount(CompositePose), BoneCount, return);
    if ( Mask == NULL )
    {
        // There's no track mask, so we're always going to be using WeightAll.
        WeightNone = WeightAll;
    }

    local_pose_transform *Base = BasePose.Transforms;
    local_pose_transform *Composite = CompositePose.Transforms;

    real32 WeightBase = WeightNone;
    real32 WeightScale = WeightAll - WeightNone;

    if ( AbsoluteValue ( WeightScale ) < BlendEffectivelyZero )
    {
        // Both weights are the same, so don't bother sampling the track mask.
        Mask = NULL;

        if ( WeightBase < BlendEffectivelyZero )
        {
            // Not only are both the same, they're both zero. Which means this blend is a waste of time.
            return;
        }
        if ( WeightBase > BlendEffectivelyOne )
        {
            // Both are one, so the result is just a copy of the CompositePose.
            CopyLocalPose ( CompositePose, BasePose );
            return;
        }
    }

    {for(int32x BoneIndex = 0; BoneIndex < BoneCount; BoneIndex++)
    {
        // Without a mask, all weights are at WeightAll (i.e. as if the mask was all 1.0f)
        real32 C = WeightAll;
        if (Mask != NULL)
        {
            if (SparseBoneArray != NULL)
            {
                // We're doing a sparse blend, but the track mask is
                // not sparse, so we need to do an indirection.
                C = WeightBase + (GetTrackMaskBoneWeight(*Mask, SparseBoneArray[BoneIndex]) * WeightScale);
            }
            else
            {
                C = WeightBase + (GetTrackMaskBoneWeight(*Mask, BoneIndex) * WeightScale);
            }
        }
        real32 const InvC = 1.0f - C;

        Base->Transform.Flags |= Composite->Transform.Flags;

        if(Base->Transform.Flags & HasPosition)
        {
            ScaleVectorPlusScaleVector3(Base->Transform.Position,
                                        InvC, Base->Transform.Position,
                                        C, Composite->Transform.Position);
        }

        if(Base->Transform.Flags & HasOrientation)
        {
            real32 IP = InnerProduct4(Base->Transform.Orientation,
                                      Composite->Transform.Orientation);
            ScaleVectorPlusScaleVector4(
                Base->Transform.Orientation,
                InvC, Base->Transform.Orientation,
                (IP >= 0) ? C : -C, Composite->Transform.Orientation);
            NormalizeOrZero4(Base->Transform.Orientation);
        }

        if(Base->Transform.Flags & HasScaleShear)
        {
            ScaleMatrixPlusScaleMatrix3x3(
                (real32 *)Base->Transform.ScaleShear,
                InvC, (real32 *)Base->Transform.ScaleShear,
                C, (real32 *)Composite->Transform.ScaleShear);
        }

        ++Base;
        ++Composite;
    }}
}


void GRANNY
LocalPoseFromWorldPoseNoScale(skeleton const* Skeleton,
                              local_pose *Result,
                              world_pose const &WorldPose,
                              real32 const *Offset4x4,
                              int32x FirstBone,
                              int32x BoneCount)
{
    CheckPointerNotNull(Result, return);
    CheckPointerNotNull(Skeleton, return);
    CheckCondition(FirstBone >= 0, return);
    CheckCondition(BoneCount >= 0, return);
    CheckCondition(Skeleton->BoneCount >= FirstBone + BoneCount, return);
    CheckCondition(GetLocalPoseBoneCount(*Result)  >= FirstBone + BoneCount, return);
    CheckCondition(GetWorldPoseBoneCount(WorldPose) >= FirstBone + BoneCount, return);

    if (Offset4x4 == NULL)
    {
        Offset4x4 = (real32 const*)GlobalIdentity4x4;
    }

    matrix_4x4 const *WorldPoseMatrices = (matrix_4x4 const *)GetWorldPose4x4Array(WorldPose);
    {for(int32x BoneIdx = FirstBone + BoneCount - 1; BoneIdx >= 0; --BoneIdx)
    {
        bone const &Bone = Skeleton->Bones[BoneIdx];
        real32 const* ParentMatrix;
        if (Bone.ParentIndex != NoParentBone)
        {
            ParentMatrix = (real32 const*)WorldPoseMatrices[Bone.ParentIndex];
        }
        else
        {
            ParentMatrix = Offset4x4;
        }
        Assert(ParentMatrix);

        matrix_4x4 InverseParent;
        MatrixInvert4x3((real32*)&InverseParent, ParentMatrix);

        matrix_4x4 Local;
        ColumnMatrixMultiply4x3((real32*)&Local,
                                (real32 const*)&WorldPoseMatrices[BoneIdx],
                                (real32 const*)&InverseParent);

        // With no scale, the upper 3x3 is the rotation matrix
        triple Rotation[3];
        MatrixColumns3x3(Rotation, Local[0], Local[1], Local[2]);

        transform *BoneTransform = GetLocalPoseTransform(*Result, BoneIdx);
        VectorEquals3(BoneTransform->Position, Local[3]);
        QuaternionEqualsMatrix3x3(BoneTransform->Orientation, (real32 *)Rotation);
        MatrixEquals3x3((real32*)BoneTransform->ScaleShear, (real32 const*)GlobalIdentity3x3);
        BoneTransform->Flags = HasPosition | HasOrientation;

        Normalize4(BoneTransform->Orientation);  // Just to be sure...
    }}
}

void GRANNY
LocalPoseFromWorldPose(skeleton const* Skeleton,
                       local_pose *Result,
                       world_pose const &WorldPose,
                       real32 const *Offset4x4,
                       int32x FirstBone,
                       int32x BoneCount)
{
    CheckPointerNotNull(Result, return);
    CheckPointerNotNull(Skeleton, return);
    CheckCondition(FirstBone >= 0, return);
    CheckCondition(BoneCount >= 0, return);
    CheckCondition(Skeleton->BoneCount >= FirstBone + BoneCount, return);
    CheckCondition(GetLocalPoseBoneCount(*Result)  >= FirstBone + BoneCount, return);
    CheckCondition(GetWorldPoseBoneCount(WorldPose) >= FirstBone + BoneCount, return);

    if (Offset4x4 == NULL)
    {
        Offset4x4 = (real32 const*)GlobalIdentity4x4;
    }

    matrix_4x4 const *WorldPoseMatrices = (matrix_4x4 const *)GetWorldPose4x4Array(WorldPose);
    {for(int32x BoneIdx = FirstBone + BoneCount - 1; BoneIdx >= 0; --BoneIdx)
    {
        bone const &Bone = Skeleton->Bones[BoneIdx];
        real32 const* ParentMatrix;
        if (Bone.ParentIndex != NoParentBone)
        {
            ParentMatrix = (real32 const*)WorldPoseMatrices[Bone.ParentIndex];
        }
        else
        {
            ParentMatrix = Offset4x4;
        }
        Assert(ParentMatrix);

        matrix_4x4 InverseParent;
        MatrixInvert4x3((real32*)&InverseParent, ParentMatrix);

        matrix_4x4 Local;
        ColumnMatrixMultiply4x3((real32*)&Local,
                                (real32 const*)&WorldPoseMatrices[BoneIdx],
                                (real32 const*)&InverseParent);

        triple Linear[3];
        MatrixColumns3x3(Linear, Local[0], Local[1], Local[2]);

        triple Q[3];
        triple Scale[3];
        if(!PolarDecompose((real32 *)Linear, 0.00001f,
                           (real32 *)Q, (real32 *)Scale))
        {
            Log0(WarningLogMessage, LocalPoseLogMessage,
                 "Unable to accurately decompose MAX transform Q");
        }

        triple Position;
        quad   Orientation;
        VectorEquals3(Position, Local[3]);
        QuaternionEqualsMatrix3x3(Orientation, (real32 *)Q);
        Normalize4(Orientation);

        transform *BoneTransform = GetLocalPoseTransform(*Result, BoneIdx);
        SetTransformWithIdentityCheck(*BoneTransform,
                                      Position, Orientation, &Scale[0][0]);
    }}
}

void GRANNY
CopyLocalPose ( local_pose const &Src, local_pose &Dst )
{
    int32x BoneCount = Src.BoneCount;
    //CheckEqualInt32(Dst.BoneCount, BoneCount, return);

    Copy32 ( ( sizeof(Src.Transforms[0]) / 4 ) * BoneCount, Src.Transforms, Dst.Transforms );
}



// This is more agressive than necessary and checks to make sure that every bone
// has all its parents, and that it's ordered correctly.
static void
SparseBoneArrayCheck(int32x const *SourceArray, int32x const *SourceArrayReverse, int32x SourceBoneCount, skeleton const &Skeleton)
{
#if DEBUG
    {for ( int32x CurBone = 0; CurBone < SourceBoneCount; CurBone++ )
    {
        Assert ( SourceArray[CurBone] >= 0 );
        Assert ( SourceArray[CurBone] < Skeleton.BoneCount );
        Assert ( SourceArrayReverse[SourceArray[CurBone]] == CurBone );
        // Should be ordered.
        if ( CurBone > 0 )
        {
            Assert ( SourceArray[CurBone-1] > SourceArray[CurBone] );
        }
        // Make sure the parent exists.
        int32x ParentBone = Skeleton.Bones[SourceArray[CurBone]].ParentIndex;
        if ( ParentBone != NoParentBone )
        {
            Assert ( ParentBone < SourceArray[CurBone] );
            bool Found = false;
            {for ( int32x TempBone = CurBone + 1; TempBone < SourceBoneCount; TempBone++ )
            {
                if ( ParentBone == SourceArray[TempBone] )
                {
                    Found = true;
                    break;
                }
            }}
            Assert ( Found );
        }
    }}
#endif
}


int32x GRANNY
SparseBoneArrayCreateSingleBone(skeleton const &Skeleton, int32x FirstBoneRequired, int32x *ResultSparseBoneArray, int32x *ResultSparseBoneArrayReverse)
{
    // Sparse bone lists are always sorted largest to smallest.
    SetInt32x ( Skeleton.BoneCount, NoSparseBone, ResultSparseBoneArrayReverse );
    int32x ResultBoneCount = 0;
    int32x BoneIndex = FirstBoneRequired;
    int32x *SparseBoneArray = ResultSparseBoneArray;

    while(BoneIndex != NoParentBone)
    {
        *SparseBoneArray = BoneIndex;
        ResultSparseBoneArrayReverse[BoneIndex] = ResultBoneCount;
        ++SparseBoneArray;
        ++ResultBoneCount;

        BoneIndex = Skeleton.Bones[BoneIndex].ParentIndex;
    }
    SparseBoneArrayCheck ( ResultSparseBoneArray, ResultSparseBoneArrayReverse, ResultBoneCount, Skeleton );
    return ResultBoneCount;
}

int32x GRANNY
SparseBoneArrayAddBone(skeleton const &Skeleton, int32x BoneToAdd, int32x InitialBoneCount, int32x *SparseBoneArray, int32x *SparseBoneArrayReverse)
{
    SparseBoneArrayCheck ( SparseBoneArray, SparseBoneArrayReverse, InitialBoneCount, Skeleton );

    // Step through the existing list looking for the current bone.
    // If the bone already exists in the list, stop, since presumably all the parents
    // of that bone have already been added as well.
    // Otherwise, insert the bone at the correct place, find its parent bone, and continue.
    int32x CurrentListIndex = 0;
    int32x ResultBoneCount = InitialBoneCount;
    while ( BoneToAdd != NoParentBone )
    {
        // Is this bone in the list already?
        if ( SparseBoneArrayReverse[BoneToAdd] != NoSparseBone )
        {
            Assert ( SparseBoneArrayReverse[BoneToAdd] >= 0 );
            Assert ( SparseBoneArrayReverse[BoneToAdd] < Skeleton.BoneCount );
            Assert ( SparseBoneArray[SparseBoneArrayReverse[BoneToAdd]] == BoneToAdd );
            // Yes, it's already in the list. Excellent, we can stop then.
            SparseBoneArrayCheck ( SparseBoneArray, SparseBoneArrayReverse, ResultBoneCount, Skeleton );
            return ResultBoneCount;
        }

        // It's not in the list already, so we need to add it.
        // Find where in the list needs to be added.
        while ( CurrentListIndex < ResultBoneCount )
        {
            int32x CurBone = SparseBoneArray[CurrentListIndex];
            // If it is in here, we should already have found it above!
            Assert ( CurBone != BoneToAdd );
            if ( CurBone < BoneToAdd )
            {
                // The bone we're adding has a higher index than the one in the list,
                // so it's not in the list (or we would have seen it already),
                // and it needs to be added here.
                break;
            }
            ++CurrentListIndex;
        }

        // Either we hit the end of the list, or we hit a bone in the list
        // smaller than the one we need to add.
        // Either way, the bone needs adding at this position in the list.
        // Shuffle everything up one and insert it.
        Assert ( ResultBoneCount < Skeleton.BoneCount );
        {for ( int32x TempListIndex = ResultBoneCount; TempListIndex > CurrentListIndex; --TempListIndex )
        {
            int32x MovingBone = SparseBoneArray[TempListIndex-1];
            Assert ( SparseBoneArrayReverse[MovingBone] == (TempListIndex-1) );
            SparseBoneArray[TempListIndex] = MovingBone;
            SparseBoneArrayReverse[MovingBone] = TempListIndex;
        }}
        ++ResultBoneCount;
        // And insert the parent in the right place.
        SparseBoneArray[CurrentListIndex] = BoneToAdd;
        SparseBoneArrayReverse[BoneToAdd] = CurrentListIndex;
        ++CurrentListIndex;

        // Now find or add the parent.
        BoneToAdd = Skeleton.Bones[BoneToAdd].ParentIndex;
    }
    SparseBoneArrayCheck ( SparseBoneArray, SparseBoneArrayReverse, ResultBoneCount, Skeleton );
    return ResultBoneCount;
}


void GRANNY
SparseBoneArrayExpand(skeleton const &Skeleton, local_pose const &SourceSparseLocalPose, int32x SparseBoneCount, int32x const *SparseBoneArray, int32x const *SparseBoneArrayReverse, int32x BoneCount, local_pose &DestLocalPose )
{
    CheckBoundedInt32 ( 0, BoneCount, Skeleton.BoneCount, return );
    CheckBoundedInt32 ( 0, BoneCount, DestLocalPose.BoneCount, return );
    CheckBoundedInt32 ( 0, SparseBoneCount, SourceSparseLocalPose.BoneCount, return );

    local_pose_transform const *SrcTransform = SourceSparseLocalPose.Transforms;
    local_pose_transform *DstTransforms = DestLocalPose.Transforms;
    bone const *SkeletonBone = Skeleton.Bones;

    {for ( int32x BoneNum = 0; BoneNum < BoneCount; BoneNum++ )
    {
        int32x SparseBoneNum = SparseBoneArrayReverse[BoneNum];
        transform const *TheTransform;
        if ( SparseBoneNum == NoSparseBone )
        {
            TheTransform = &(SkeletonBone->LocalTransform);
        }
        else
        {
            Assert ( SparseBoneNum >= 0 );
            Assert ( SparseBoneNum < Skeleton.BoneCount );
            TheTransform = &(SrcTransform[SparseBoneNum].Transform);
        }
        Copy32 ( sizeof(TheTransform[0])/4, TheTransform, &(DstTransforms[BoneNum].Transform) );
        ++SkeletonBone;
    }}
}


bool GRANNY
SparseBoneArrayIsValid(int32x SkeletonBoneCount, int32x SparseBoneCount, int32x const *SparseBoneArray, int32x const *SparseBoneArrayReverse)
{
    {for ( int32x CurIndex = 0; CurIndex < SparseBoneCount; CurIndex++ )
    {
        if ( ( SparseBoneArray[CurIndex] < 0 ) || ( SparseBoneArray[CurIndex] >= SkeletonBoneCount ) )
        {
            Log1 ( ErrorLogMessage, LocalPoseLogMessage, "SparseBoneArray[%i] is out of range", CurIndex );
            return false;
        }
        if ( SparseBoneArrayReverse[SparseBoneArray[CurIndex]] != CurIndex )
        {
            Log2 ( ErrorLogMessage, LocalPoseLogMessage, "SparseBoneArrayReverse[%i] should be %i", SparseBoneArray[CurIndex], CurIndex );
            return false;
        }
    }}
    {for ( int32x CurBone = 0; CurBone < SkeletonBoneCount; CurBone++ )
    {
        int32x SparseBoneArrayReverseValue = SparseBoneArrayReverse[CurBone];
        if ( SparseBoneArrayReverseValue != NoSparseBone )
        {
            if ( ( SparseBoneArrayReverseValue < 0 ) || ( SparseBoneArrayReverseValue >= SparseBoneCount ) )
            {
                Log1 ( ErrorLogMessage, LocalPoseLogMessage, "SparseBoneArrayReverse[%i] is out of range", CurBone );
                return false;
            }
            if ( SparseBoneArray[SparseBoneArrayReverseValue] != CurBone )
            {
                Log2 ( ErrorLogMessage, LocalPoseLogMessage, "SparseBoneArray[%i] should be %i", SparseBoneArrayReverseValue, CurBone );
                return false;
            }
        }
    }}
    return true;
}
