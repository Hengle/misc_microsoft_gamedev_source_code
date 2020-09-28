// ========================================================================
// $File: //jeffr/granny/rt/granny_mesh_binding.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #12 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MESH_BINDING_H)
#include "granny_mesh_binding.h"
#endif

#if !defined(GRANNY_MESH_H)
#include "granny_mesh.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_WORLD_POSE_H)
#include "granny_world_pose.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode MeshBindingLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct mesh_binding
{
    mesh const *Mesh;

    int32x BoneCount;

    skeleton const *FromSkeleton;
    int32x *FromBoneIndices;

    skeleton const *ToSkeleton;
    int32x *ToBoneIndices;
};

END_GRANNY_NAMESPACE;

mesh_binding *GRANNY
NewMeshBinding(mesh const &Mesh,
               skeleton const &FromSkeleton,
               skeleton const &ToSkeleton)
{
    mesh_binding *Binding = NewMeshBindingInPlace(
        Mesh, FromSkeleton, ToSkeleton,
        AllocateSize(
            GetResultingMeshBindingSize(Mesh, FromSkeleton, ToSkeleton)));

    return(Binding);
}

void GRANNY
FreeMeshBinding(mesh_binding *Binding)
{
    Deallocate(Binding);
}

static void
AggrMeshBinding(aggr_allocator &Allocator,
                mesh const &Mesh,
                skeleton const &FromSkeleton,
                skeleton const &ToSkeleton,
                mesh_binding *&Binding)
{
    bool Transferred = (&FromSkeleton != &ToSkeleton);
    int32x const BoneCount = Mesh.BoneBindingCount;

    AggrAllocPtr(Allocator, Binding);
    AggrAllocOffsetArrayPtr(Allocator, Binding, BoneCount, BoneCount, FromBoneIndices);
    if(Transferred)
    {
        AggrAllocOffsetArrayPtr(Allocator, Binding, BoneCount, BoneCount, ToBoneIndices);
    }
}

int32x GRANNY
GetResultingMeshBindingSize(mesh const &Mesh,
                            skeleton const &FromSkeleton,
                            skeleton const &ToSkeleton)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    mesh_binding *Binding;
    AggrMeshBinding(Allocator, Mesh, FromSkeleton, ToSkeleton, Binding);

    int32x ResultingSize;
    CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
    return ResultingSize;
}

mesh_binding *GRANNY
NewMeshBindingInPlace(mesh const &Mesh,
                      skeleton const &FromSkeleton,
                      skeleton const &ToSkeleton,
                      void *Memory)
{
    mesh_binding *Binding = 0;

    bool Transferred = (&FromSkeleton != &ToSkeleton);
    int32x const BoneCount = Mesh.BoneBindingCount;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    AggrMeshBinding(Allocator, Mesh, FromSkeleton, ToSkeleton, Binding);
    if(EndAggrPlacement(Allocator, Memory))
    {
        Binding->BoneCount = BoneCount;
        Binding->Mesh = &Mesh;
        Binding->FromSkeleton = &FromSkeleton;
        Binding->ToSkeleton = &ToSkeleton;

        if(!Transferred)
        {
            Binding->ToBoneIndices = Binding->FromBoneIndices;
        }

        {for(int32x BoneIndex = 0;
             BoneIndex < BoneCount;
             ++BoneIndex)
        {
            int32x &ToIndex = Binding->ToBoneIndices[BoneIndex];
            int32x &FromIndex = Binding->FromBoneIndices[BoneIndex];

            char const *MeshBoneName = Mesh.BoneBindings[BoneIndex].BoneName;
            if(!FindBoneByName(&ToSkeleton, MeshBoneName, ToIndex))
            {
                Log1(ErrorLogMessage, MeshBindingLogMessage,
                     "NewMeshBinding: Unable to find bone: '%s' in the ToSkeleton",
                     MeshBoneName);
                ToIndex = 0;
            }

            if(Transferred)
            {
                if(!FindBoneByName(&FromSkeleton, MeshBoneName, FromIndex))
                {
                    Log1(ErrorLogMessage, MeshBindingLogMessage,
                         "NewMeshBinding: Unable to find bone: '%s' in the FromSkeleton",
                         MeshBoneName);
                    FromIndex = 0;
                }
            }
        }}
    }

    return(Binding);
}

bool GRANNY
MeshBindingIsTransferred(mesh_binding const &Binding)
{
    return(Binding.FromSkeleton != Binding.ToSkeleton);
}

int32x GRANNY
GetMeshBinding4x4ArraySize(mesh_binding const &Binding,
                           int32x BoneCount)
{
    return(BoneCount * SizeOf(matrix_4x4));
}

void GRANNY
BuildMeshBinding4x4Array(mesh_binding const &Binding,
                         world_pose const &WorldPose,
                         int32x FirstBoneIndex,
                         int32x BoneCount,
                         real32 *TransformBuffer)
{
    CheckCountedInt32(FirstBoneIndex, Binding.BoneCount, return);
    CheckCountedInt32(FirstBoneIndex + BoneCount, Binding.BoneCount + 1, return);

    // We need to generate composite transforms
    matrix_4x4 *Composite = (matrix_4x4 *)TransformBuffer;
    int32x LastBoneIndex = FirstBoneIndex + BoneCount;
    {for(int32x BoneIndex = FirstBoneIndex;
         BoneIndex < LastBoneIndex;
         ++BoneIndex)
    {
        int32x const FromBoneIndex = Binding.FromBoneIndices[BoneIndex];
        int32x const ToBoneIndex = Binding.ToBoneIndices[BoneIndex];

#if 1
        ColumnMatrixMultiply4x3(
            (real32 *)*Composite,
            (real32 *)Binding.FromSkeleton->Bones[FromBoneIndex].InverseWorld4x4,
            GetWorldPose4x4(WorldPose, ToBoneIndex));
#else
        // Actually, I think this code is just plain wrong.
        // It should be using ToBoneIndex instead of one of the FromBoneIndex.
        transform TempTransform;
        BuildSkeletonRelativeTransform(SizeOf(bone),
                                       &Binding.FromSkeleton->Bones->LocalTransform,
                                       SizeOf(bone),
                                       &Binding.FromSkeleton->Bones->ParentIndex,
                                       FromBoneIndex,
                                       TempTransform);
        matrix_4x4 Temp4x4;
        BuildCompositeTransform4x4(TempTransform,
                                   (real32 *)Temp4x4);
        ColumnMatrixMultiply4x3(
            (real32 *)*Composite,
            (real32 *)Binding.FromSkeleton->Bones[FromBoneIndex].InverseWorld4x4,
            (real32 *)Temp4x4);
        MatrixIdentity4x4((real32 *)*Composite);
#endif

        ++Composite;
    }}
}

int32x GRANNY
GetMeshBindingBoneCount(mesh_binding const &Binding)
{
    return(Binding.BoneCount);
}

int32x const *GRANNY
GetMeshBindingFromBoneIndices(mesh_binding const &Binding)
{
    return(Binding.FromBoneIndices);
}

int32x const *GRANNY
GetMeshBindingToBoneIndices(mesh_binding const &Binding)
{
    return(Binding.ToBoneIndices);
}

skeleton *GRANNY
GetMeshBindingFromSkeleton(mesh_binding const &Binding)
{
    // todo: can we change this?  Casting away const = bad, and this really shouldn't change...
    return const_cast<skeleton*>(Binding.FromSkeleton);
}

skeleton *GRANNY
GetMeshBindingToSkeleton(mesh_binding const &Binding)
{
    // todo: can we change this?  Casting away const = bad, and this really shouldn't change...
    return const_cast<skeleton*>(Binding.ToSkeleton);
}

mesh *GRANNY
GetMeshBindingSourceMesh(mesh_binding const &Binding)
{
    // todo: can we change this?  Casting away const = bad, and this really shouldn't change...
    return const_cast<mesh*>(Binding.Mesh);
}


