// ========================================================================
// $File: //jeffr/granny/rt/granny_world_pose.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #22 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_WORLD_POSE_H)
#include "granny_world_pose.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_LOCAL_POSE_H)
#include "granny_local_pose.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_PREFETCH_H)
#include "granny_prefetch.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode WorldPoseLogMessage

USING_GRANNY_NAMESPACE;

world_pose *GRANNY
NewWorldPose(int32 BoneCount)
{
    world_pose *Pose =
        NewWorldPoseInPlace(
            BoneCount, IncludeComposites,
            AllocateSizeAligned(MatrixBufferAlignment,
                                GetResultingWorldPoseSize(BoneCount, IncludeComposites)));

    return(Pose);
}

world_pose *GRANNY
NewWorldPoseNoComposite(int32 BoneCount)
{
    world_pose *Pose =
        NewWorldPoseInPlace(
            BoneCount, ExcludeComposites,
            AllocateSizeAligned(MatrixBufferAlignment,
                                GetResultingWorldPoseSize(BoneCount, ExcludeComposites)));

    return(Pose);
}

void GRANNY
FreeWorldPose(world_pose *WorldPose)
{
    Deallocate(WorldPose);
}

int32x GRANNY
GetWorldPoseBoneCount(world_pose const &WorldPose)
{
    return(WorldPose.BoneCount);
}

static void
AggrWorldPose(aggr_allocator &Allocator,
              int32x BoneCount,
              composite_flag CompositeFlag,
              world_pose *&Pose)
{
    AggrAllocPtr(Allocator, Pose);

    SetAggrAlignment(Allocator, MatrixBufferAlignment);
    AggrAllocOffsetArrayPtr(Allocator, Pose, BoneCount, BoneCount,
                            WorldTransformBuffer);
    if (CompositeFlag == IncludeComposites)
    {
        AggrAllocOffsetArrayPtr(Allocator, Pose, BoneCount, BoneCount,
                                CompositeTransformBuffer);
    }
}

int32x GRANNY
GetResultingWorldPoseSize(int32x BoneCount, composite_flag CompositeFlag)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    world_pose *Pose;
    AggrWorldPose(Allocator, BoneCount, CompositeFlag, Pose);

    int32x ResultingSize;
    CheckConvertToInt32(ResultingSize, EndAggrSize(Allocator), return 0);
    return ResultingSize;
}

world_pose *GRANNY
NewWorldPoseInPlace(int32x BoneCount, composite_flag CompositeFlag, void *Memory)
{
    world_pose *Pose = 0;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    AggrWorldPose(Allocator, BoneCount, CompositeFlag, Pose);
    if(EndAggrPlacement(Allocator, Memory))
    {
        // Make sure the pointer is nulled if we don't have
        // composites...
        if (CompositeFlag == ExcludeComposites)
        {
            Pose->CompositeTransformBuffer = 0;
        }

        // All is good.
        if(((uintaddrx)Pose->WorldTransformBuffer % MatrixBufferAlignment) != 0)
        {
            Log0(WarningLogMessage, WorldPoseLogMessage,
                 "world_pose WorldTransformBuffer is unaligned");
        }

        if(((uintaddrx)Pose->CompositeTransformBuffer % MatrixBufferAlignment) != 0)
        {
            Log0(WarningLogMessage, WorldPoseLogMessage,
                 "world_pose CompositeTransformBuffer is unaligned");
        }
    }

    return(Pose);
}

real32 *GRANNY
GetWorldPose4x4(world_pose const &WorldPose, int32x BoneIndex)
{
    real32 *Result = 0;

    if(BoneIndex < WorldPose.BoneCount)
    {
        Result = (real32 *)WorldPose.WorldTransformBuffer[BoneIndex];
    }

    return(Result);
}

real32 *GRANNY
GetWorldPoseComposite4x4(world_pose const &WorldPose, int32x BoneIndex)
{
    // Shouldn't call this if this is a "without composites" world pose
    // don't assert, we use this to test in various places
    if (WorldPose.CompositeTransformBuffer == NULL)
        return NULL;

    real32 *Result = 0;
    if(BoneIndex < WorldPose.BoneCount)
    {
        Result = (real32 *)WorldPose.CompositeTransformBuffer[BoneIndex];
    }

    return(Result);
}

matrix_4x4 *GRANNY
GetWorldPose4x4Array(world_pose const &WorldPose)
{
    return(WorldPose.WorldTransformBuffer);
}

matrix_4x4 *GRANNY
GetWorldPoseComposite4x4Array(world_pose const &WorldPose)
{
    // Shouldn't call this if this is a "without composites" world pose
    // don't assert, we use this to test in various places
    if (WorldPose.CompositeTransformBuffer == NULL)
        return NULL;

    return(WorldPose.CompositeTransformBuffer);
}


void GRANNY
BuildWorldPose(skeleton const &Skeleton,
               int32x FirstBone, int32x BoneCount,
               local_pose const &LocalPose,
               real32 const *Offset4x4,
               world_pose &Result)
{
    COUNT_BLOCK("BuildWorldPose");

    BuildWorldPoseLOD(Skeleton,
                      FirstBone, BoneCount,
                      FirstBone, BoneCount,  // entire range is valid in this case
                      LocalPose,
                      Offset4x4,
                      Result);
}

static void
BWP_Dispatch(transform* Transform,
             real32 const *ParentWorld,
             real32 const *InverseWorld4x4,
             real32 *Composite,
             real32 *World)
{
#if 1
    if(Transform->Flags & HasScaleShear)
    {
        BuildFullWorldPoseComposite(*Transform, ParentWorld,
                                    InverseWorld4x4,
                                    Composite,
                                    World);
    }
    else if(Transform->Flags == HasPosition)
    {
        BuildPositionWorldPoseComposite(Transform->Position, ParentWorld,
                                        InverseWorld4x4,
                                        Composite,
                                        World);
    }
    else if(Transform->Flags == 0)
    {
        BuildIdentityWorldPoseComposite(ParentWorld,
                                        InverseWorld4x4,
                                        Composite,
                                        World);
    }
    else
    {
        BuildPositionOrientationWorldPoseComposite(
            Transform->Position, Transform->Orientation, ParentWorld,
            InverseWorld4x4,
            Composite,
            World);
    }
#else
#if !DEBUG
#error This path should never be compiled in for releases
#endif
    // You can always force things through this path for testing
    BuildFullWorldPoseComposite(*Transform, ParentWorld,
                                InverseWorld4x4,
                                Composite,
                                World);
#endif
}

static void
BWPNC_Dispatch(transform* Transform,
               real32 const *ParentWorld,
               real32 *World)
{
#if 1
    if(Transform->Flags & HasScaleShear)
    {
        BuildFullWorldPoseOnly(*Transform, ParentWorld, World);
    }
    else if(Transform->Flags == HasPosition)
    {
        BuildPositionWorldPoseOnly(Transform->Position, ParentWorld, World);
    }
    else if(Transform->Flags == 0)
    {
        BuildIdentityWorldPoseOnly(ParentWorld, World);
    }
    else
    {
        BuildPositionOrientationWorldPoseOnly(
            Transform->Position, Transform->Orientation,
            ParentWorld, World);
    }
#else
#if !DEBUG
#error This path should never be compiled in for releases
#endif
    // You can always force things through this path for testing
    BuildFullWorldPoseOnly(Transform, ParentWorld, World);
#endif
}


void GRANNY
BuildWorldPoseLOD(skeleton const &Skeleton,
                  int32x FirstBone, int32x BoneCount,
                  int32x FirstValidLocalBone, int32x ValidLocalBoneCount,
                  local_pose const &LocalPose,
                  real32 const *Offset4x4,
                  world_pose &Result)
{
    COUNT_BLOCK("BuildWorldPoseLOD");

    int32x OnePastLastBone = FirstBone + BoneCount;
    int32x OnePastLastValidLocalBone = FirstValidLocalBone + ValidLocalBoneCount;

    CheckBoundedInt32(0, OnePastLastValidLocalBone, GetLocalPoseBoneCount(LocalPose),
                      return);
    CheckBoundedInt32(0, OnePastLastBone, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, OnePastLastBone, Result.BoneCount, return);

    // If we don't have composite matrices, don't bother to build
    // them...
    if (Result.CompositeTransformBuffer == 0)
    {
        BuildWorldPoseNoCompositeLOD( Skeleton, FirstBone, BoneCount,
                                      FirstValidLocalBone, ValidLocalBoneCount,
                                      LocalPose, Offset4x4, Result );
        return;
    }

#if 0
#if !DEBUG
#error This path should never be compiled in for releases
#endif
    // You can always force things through this path for testing
    BuildWorldPoseNoComposite ( Skeleton, FirstBone, BoneCount, LocalPose, Offset4x4, Result );
    BuildCompositeFromWorldPose ( Skeleton, FirstBone, BoneCount, Result );
    return;
#endif

    if(!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }

    ALIGN16(real32) OffsetBuffer[16];
    if(!IS_ALIGNED_16(Offset4x4))
    {
        MatrixEquals4x4(OffsetBuffer, Offset4x4);
        Offset4x4 = OffsetBuffer;
    }

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);
    Assert(IS_ALIGNED_16(WorldBuffer));
    local_pose_transform *Local = LocalPose.Transforms;

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = Result.CompositeTransformBuffer;  // never null, that's handled above
    Assert(Composite);

    Bone      += FirstBone;
    World     += FirstBone;
    Composite += FirstBone;
    Local     += FirstBone;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;
    CompileAssert(sizeof(*World) == sizeof(*Composite));

    Assert(IS_ALIGNED_16(WorldBuffer));
    Assert(IS_ALIGNED_16(Composite));
    Assert(IS_ALIGNED_16(Offset4x4));
    while(BoneCount)
    {
        // Prefetch
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));
            PrefetchAddress(Local + 1, sizeof(*Local));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
                PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x ParentIndex = Bone->ParentIndex;
        real32 const *ParentWorld = (ParentIndex == NoParentBone) ?
            Offset4x4 :
            (real32 *)WorldBuffer[ParentIndex];

        transform* Transform;
        if (ValidLocalBoneCount > 0)
            Transform = &Local->Transform;
        else
            Transform = &Bone->LocalTransform;

        BWP_Dispatch(Transform,
                     ParentWorld,
                     (real32 *)Bone->InverseWorld4x4,
                     (real32 *)*Composite,
                     (real32 *)*World);

        ++Bone;
        ++World;
        ++Composite;
        ++Local;
        --BoneCount;
        --ValidLocalBoneCount;
    }
}


void GRANNY
BuildWorldPoseSparse(skeleton const &Skeleton,
                    int32x FirstBone, int32x BoneCount,
                    int32x const *SparseBoneArray,
                    int32x const *SparseBoneArrayReverse,
                    local_pose const &LocalPose,
                    real32 const *Offset4x4,
                    world_pose &Result)
{
    COUNT_BLOCK("BuildWorldPoseSparse");

    int32x OnePastLastBone = FirstBone + BoneCount;

    CheckBoundedInt32(0, OnePastLastBone, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, OnePastLastBone, Result.BoneCount, return);

    // If we don't have composite matrices, don't bother to build
    // them...
    if (Result.CompositeTransformBuffer == 0)
    {
        BuildWorldPoseNoCompositeSparse ( Skeleton, FirstBone, BoneCount, SparseBoneArray,
                                          SparseBoneArrayReverse, LocalPose, Offset4x4, Result );
        return;
    }

#if 0
#if !DEBUG
#error This path should never be compiled in for releases
#endif
    // You can always force things through this path for testing
    BuildWorldPoseNoCompositeSparse ( Skeleton, FirstBone, BoneCount, SparseBoneArray, SparseBoneArrayReverse, LocalPose, Offset4x4, Result );
    BuildCompositeFromWorldPose ( Skeleton, FirstBone, BoneCount, Result );
    return;
#endif

    if(!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }

    ALIGN16(real32) OffsetBuffer[16];
    if(!IS_ALIGNED_16(Offset4x4))
    {
        MatrixEquals4x4(OffsetBuffer, Offset4x4);
        Offset4x4 = OffsetBuffer;
    }
    Assert(IS_ALIGNED_16(Offset4x4));

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);
    local_pose_transform *FirstLocal = LocalPose.Transforms;

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = Result.CompositeTransformBuffer;  // never null, that's handled above
    Assert(Composite);

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;
    CompileAssert(sizeof(*World) == sizeof(*Composite));

    Bone += FirstBone;
    World += FirstBone;
    Composite += FirstBone;
    int32x const *SparseBoneArrayReverseCurrent = SparseBoneArrayReverse + FirstBone;

    while(BoneCount)
    {
        // Prefetch
        if (BoneCount > 0)
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));
            PrefetchAddress(&FirstLocal[SparseBoneArrayReverseCurrent[1]].Transform,
                                  sizeof(transform));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
                PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x ParentIndex = Bone->ParentIndex;
        real32 const *ParentWorld = (ParentIndex == NoParentBone) ?
            Offset4x4 :
            (real32 *)WorldBuffer[ParentIndex];
        int32x LocalPoseBoneNum = SparseBoneArrayReverseCurrent[0];
        // If there is no data for this bone in the local pose, use the skeleton's default pose.
        transform *BoneTransform = &(Bone->LocalTransform);
        if ( LocalPoseBoneNum != NoSparseBone )
        {
            // But we have data. Excellent.
            Assert ( LocalPoseBoneNum >= 0 );
            Assert ( LocalPoseBoneNum < LocalPose.BoneCount );
            BoneTransform = &(FirstLocal[LocalPoseBoneNum].Transform);
        }

        BWP_Dispatch(BoneTransform,
                     ParentWorld,
                     (real32 *)Bone->InverseWorld4x4,
                     (real32 *)*Composite,
                     (real32 *)*World);

        ++Bone;
        ++World;
        ++Composite;
        ++SparseBoneArrayReverseCurrent;
        --BoneCount;
    }
}


void GRANNY
BuildRestWorldPose(skeleton const &Skeleton,
                   int32x FirstBone, int32x BoneCount,
                   real32 const *Offset4x4,
                   world_pose &Result)
{
    COUNT_BLOCK("BuildRestWorldPose");

    int32x OnePastLastBone = FirstBone + BoneCount;
    CheckBoundedInt32(0, OnePastLastBone, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, OnePastLastBone, Result.BoneCount, return);

    ALIGN16(real32) OffsetBuffer[16];
    if(!IS_ALIGNED_16(Offset4x4))
    {
        MatrixEquals4x4(OffsetBuffer, Offset4x4);
        Offset4x4 = OffsetBuffer;
    }
    if(!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = Result.CompositeTransformBuffer;  // may be null

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;
    CompileAssert(sizeof(*World) == sizeof(*Composite));

    Bone += FirstBone;
    World += FirstBone;

    // Handle the null composite array case...
    if (Composite)
        Composite += FirstBone;

    while(BoneCount)
    {
        // Prefetch
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
                if (Composite)
                    PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x ParentIndex = Bone->ParentIndex;
        real32 const *ParentWorld = (ParentIndex == NoParentBone) ?
            Offset4x4 :
            (real32 *)WorldBuffer[ParentIndex];
        transform &Transform = Bone->LocalTransform;

        if (Composite)
        {
            BWP_Dispatch(&Transform,
                         ParentWorld,
                         (real32 *)Bone->InverseWorld4x4,
                         (real32 *)*Composite,
                         (real32 *)*World);
            ++Composite;
        }
        else
        {
            BWPNC_Dispatch(&Transform,
                           ParentWorld,
                           (real32 *)*World);
        }

        ++Bone;
        ++World;
        --BoneCount;
    }
}


void GRANNY
BuildWorldPoseNoComposite(skeleton const &Skeleton,
                          int32x FirstBone, int32x BoneCount,
                          local_pose const &LocalPose,
                          real32 const *Offset4x4,
                          world_pose &Result)
{
    COUNT_BLOCK("BuildWorldPoseNoComposite");

    int32x OnePastLastBone = FirstBone + BoneCount;

    CheckBoundedInt32(0, OnePastLastBone, GetLocalPoseBoneCount(LocalPose),
                      return);
    CheckBoundedInt32(0, OnePastLastBone, Result.BoneCount, return);

    if(!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }

    ALIGN16(real32) OffsetBuffer[16];
    if(!IS_ALIGNED_16(Offset4x4))
    {
        MatrixEquals4x4(OffsetBuffer, Offset4x4);
        Offset4x4 = OffsetBuffer;
    }

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);
    local_pose_transform *Local = LocalPose.Transforms;

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;

    Bone += FirstBone;
    World += FirstBone;
    Local += FirstBone;

    while(BoneCount)
    {
        // Prefetch
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));
            PrefetchAddress(Local + 1, sizeof(*Local));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x ParentIndex = Bone->ParentIndex;
        real32 const *ParentWorld = (ParentIndex == NoParentBone) ?
            Offset4x4 :
            (real32 *)WorldBuffer[ParentIndex];
        transform &Transform = Local->Transform;

        BWPNC_Dispatch(&Transform, ParentWorld, (real32*)*World);

        ++Bone;
        ++World;
        ++Local;
        --BoneCount;
    }
}



void GRANNY
BuildWorldPoseNoCompositeSparse(skeleton const &Skeleton,
                                int32x FirstBone, int32x BoneCount,
                                int32x const *SparseBoneArray,
                                int32x const *SparseBoneArrayReverse,
                                local_pose const &LocalPose,
                                real32 const *Offset4x4,
                                world_pose &Result)
{
    COUNT_BLOCK("BuildWorldPoseNoCompositeSparse");

    int32x OnePastLastBone = FirstBone + BoneCount;

    CheckBoundedInt32(0, OnePastLastBone, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, OnePastLastBone, Result.BoneCount, return);

    if(!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }

    ALIGN16(real32) OffsetBuffer[16];
    if(!IS_ALIGNED_16(Offset4x4))
    {
        MatrixEquals4x4(OffsetBuffer, Offset4x4);
        Offset4x4 = OffsetBuffer;
    }

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);
    local_pose_transform *FirstLocal = LocalPose.Transforms;

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;

    Bone += FirstBone;
    World += FirstBone;
    int32x const *SparseBoneArrayReverseCurrent = SparseBoneArrayReverse + FirstBone;

    while(BoneCount)
    {
        // Prefetch
        if (BoneCount > 0)
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));
            PrefetchAddress(&FirstLocal[SparseBoneArrayReverseCurrent[1]].Transform,
                                  sizeof(transform));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x ParentIndex = Bone->ParentIndex;
        real32 const *ParentWorld = (ParentIndex == NoParentBone) ?
            Offset4x4 :
            (real32 *)WorldBuffer[ParentIndex];
        int32x LocalPoseBoneNum = SparseBoneArrayReverseCurrent[0];
        // If there is no data for this bone in the local pose, use the skeleton's default pose.
        transform *BoneTransform = &(Bone->LocalTransform);
        if ( LocalPoseBoneNum != NoSparseBone )
        {
            // But we have data. Excellent.
            Assert ( LocalPoseBoneNum >= 0 );
            Assert ( LocalPoseBoneNum < LocalPose.BoneCount );
            BoneTransform = &(FirstLocal[LocalPoseBoneNum].Transform);
        }
        transform &Transform = *BoneTransform;

        BWPNC_Dispatch(&Transform, ParentWorld, (real32*)*World);

        ++Bone;
        ++SparseBoneArrayReverseCurrent;
        ++World;
        --BoneCount;
    }
}


void GRANNY
BuildWorldPoseNoCompositeLOD(skeleton const &Skeleton,
                             int32x FirstBone, int32x BoneCount,
                             int32x FirstValidLocalBone, int32x ValidLocalBoneCount,
                             local_pose const &LocalPose,
                             real32 const *Offset4x4,
                             world_pose &Result)
{
    COUNT_BLOCK("BuildWorldPoseNoCompositeLOD");

    int32x OnePastLastBone = FirstBone + BoneCount;
    int32x OnePastLastValidLocalBone = FirstValidLocalBone + ValidLocalBoneCount;

    CheckBoundedInt32(0, OnePastLastValidLocalBone, GetLocalPoseBoneCount(LocalPose),
                      return);
    CheckBoundedInt32(0, OnePastLastBone, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, OnePastLastBone, Result.BoneCount, return);

#if 0
#if !DEBUG
#error This path should never be compiled in for releases
#endif
    // You can always force things through this path for testing
    BuildWorldPoseNoComposite ( Skeleton, FirstBone, BoneCount, LocalPose, Offset4x4, Result );
    return;
#endif

    if(!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }

    ALIGN16(real32) OffsetBuffer[16];
    if(!IS_ALIGNED_16(Offset4x4))
    {
        MatrixEquals4x4(OffsetBuffer, Offset4x4);
        Offset4x4 = OffsetBuffer;
    }

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);
    Assert(IS_ALIGNED_16(WorldBuffer));
    local_pose_transform *Local = LocalPose.Transforms;

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;

    Bone      += FirstBone;
    World     += FirstBone;
    Local     += FirstBone;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;

    Assert(IS_ALIGNED_16(WorldBuffer));
    Assert(IS_ALIGNED_16(Offset4x4));
    while(BoneCount)
    {
        // Prefetch
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));
            PrefetchAddress(Local + 1, sizeof(*Local));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x ParentIndex = Bone->ParentIndex;
        real32 const *ParentWorld = (ParentIndex == NoParentBone) ?
            Offset4x4 :
            (real32 *)WorldBuffer[ParentIndex];

        transform* Transform;
        if (ValidLocalBoneCount > 0)
            Transform = &Local->Transform;
        else
            Transform = &Bone->LocalTransform;

        BWPNC_Dispatch(Transform,
                       ParentWorld,
                       (real32 *)*World);

        ++Bone;
        ++World;
        ++Local;
        --BoneCount;
        --ValidLocalBoneCount;
    }
}


void GRANNY
BuildWorldPoseComposites(skeleton const &Skeleton,
                         int32x FirstBone, int32x BoneCount,
                         world_pose &Result)
{
    COUNT_BLOCK("BuildWorldPoseComposites");
    CheckPointerNotNull(Result.CompositeTransformBuffer, return);

    int32x OnePastLastBone = FirstBone + BoneCount;

    CheckBoundedInt32(0, OnePastLastBone, Result.BoneCount, return);

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = GetWorldPoseComposite4x4Array(Result);  // never null, checked above
    Assert(Composite);

    Bone += FirstBone;
    World += FirstBone;
    Composite += FirstBone;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;

    while(BoneCount)
    {
        // Prefetch
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
                PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }


        BuildSingleCompositeFromWorldPose((real32*)Bone->InverseWorld4x4,
                                          (real32 *)*World,
                                          (real32 *)*Composite);
        ++Bone;
        ++World;
        ++Composite;
        --BoneCount;
    }
}


void GRANNY
BuildCompositeBuffer(skeleton const &Skeleton,
                     int32x FirstBone, int32x BoneCount,
                     world_pose const &Pose,
                     matrix_4x4* CompositeBuffer)
{
    COUNT_BLOCK("BuildCompositeBuffer");
    CheckPointerNotNull(CompositeBuffer, return);
    CheckBoundedInt32(0, FirstBone, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, FirstBone + BoneCount, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, FirstBone, Pose.BoneCount, return);
    CheckBoundedInt32(0, FirstBone + BoneCount, Pose.BoneCount, return);

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Pose);

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = CompositeBuffer;

    Bone += FirstBone;
    World += FirstBone;
    Composite += FirstBone;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;

    while(BoneCount)
    {
        // Prefetch
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
                PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        BuildSingleCompositeFromWorldPose((real32*)Bone->InverseWorld4x4,
                                          (real32 *)*World,
                                          (real32 *)*Composite);
        ++Bone;
        ++World;
        ++Composite;
        --BoneCount;
    }
}

void GRANNY
BuildCompositeBufferTransposed(skeleton const &Skeleton,
                               int32x FirstBone, int32x BoneCount,
                               world_pose const &Pose,
                               matrix_3x4* CompositeBuffer)
{
    COUNT_BLOCK("BuildCompositeBuffer");
    CheckPointerNotNull(CompositeBuffer, return);
    CheckBoundedInt32(0, FirstBone, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, FirstBone + BoneCount, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, FirstBone, Pose.BoneCount, return);
    CheckBoundedInt32(0, FirstBone + BoneCount, Pose.BoneCount, return);

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Pose);

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_3x4 *Composite = CompositeBuffer;

    Bone += FirstBone;
    World += FirstBone;
    Composite += FirstBone;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneCount % Prefetch4x4Count;

    while(BoneCount)
    {
        // Prefetch
        {
            PrefetchAddress(Bone + 1, sizeof(*Bone));

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((BoneCount % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
                PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        BuildSingleCompositeFromWorldPoseTranspose((real32*)Bone->InverseWorld4x4,
                                                   (real32 *)*World,
                                                   (real32 *)*Composite);
        ++Bone;
        ++World;
        ++Composite;
        --BoneCount;
    }
}

void GRANNY
BuildIndexedCompositeBuffer(skeleton const &Skeleton,
                            world_pose const &Pose,
                            int32x const* Indices,
                            int32x IndexCount,
                            matrix_4x4* CompositeBuffer)
{
    COUNT_BLOCK("BuildIndexedCompositeBuffer");
    CheckPointerNotNull(CompositeBuffer, return);
    CheckPointerNotNull(Indices, return);
    CheckBoundedInt32(0, IndexCount, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, IndexCount, Pose.BoneCount, return);

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Pose);

    bone *Bones = Skeleton.Bones;
    matrix_4x4 *Composite = CompositeBuffer;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*Composite);

    {for(int32x CurrIdx = 0; CurrIdx < IndexCount; ++CurrIdx)
    {
        // Prefetch
        if (CurrIdx < IndexCount-1)
        {
            PrefetchAddress(Bones + Indices[CurrIdx+1], sizeof(*Bones));
            // todo: missing a world prefetch

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((CurrIdx % Prefetch4x4Count) == 0)
            {
                PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x Index = Indices[CurrIdx];
        Assert(Index >= 0 && Index < Skeleton.BoneCount);
        Assert(Index < Pose.BoneCount);

        bone* ThisBone = Bones + Index;
        matrix_4x4* ThisWorld = WorldBuffer + Index;

        BuildSingleCompositeFromWorldPose((real32*)ThisBone->InverseWorld4x4,
                                          (real32 *)*ThisWorld,
                                          (real32 *)*Composite);
        ++Composite;
    }}
}

void GRANNY
BuildIndexedCompositeBufferTransposed(skeleton const &Skeleton,
                                      world_pose const &Pose,
                                      int32x const* Indices,
                                      int32x IndexCount,
                                      matrix_3x4* CompositeBuffer)
{
    COUNT_BLOCK("BuildIndexedCompositeBuffer");
    CheckPointerNotNull(CompositeBuffer, return);
    CheckPointerNotNull(Indices, return);
    CheckBoundedInt32(0, IndexCount, Skeleton.BoneCount, return);
    CheckBoundedInt32(0, IndexCount, Pose.BoneCount, return);

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Pose);

    bone *Bones = Skeleton.Bones;
    matrix_3x4 *Composite = CompositeBuffer;

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*Composite);

    {for(int32x CurrIdx = 0; CurrIdx < IndexCount; ++CurrIdx)
    {
        // Prefetch
        if (CurrIdx < IndexCount-1)
        {
            PrefetchAddress(Bones + Indices[CurrIdx+1], sizeof(*Bones));
            // todo: missing a world prefetch

            // We only have to prefetch World and Composite every few
            // times, since they're quite small.  We do a little dance
            // here to make sure that the alignment of the structures
            // along a cache line boundary won't cause problems.
            if ((CurrIdx % Prefetch4x4Count) == 0)
            {
                PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }
        }

        int32x Index = Indices[CurrIdx];
        Assert(Index >= 0 && Index < Skeleton.BoneCount);
        Assert(Index < Pose.BoneCount);

        bone* ThisBone = Bones + Index;
        matrix_4x4* ThisWorld = WorldBuffer + Index;

        BuildSingleCompositeFromWorldPoseTranspose((real32*)ThisBone->InverseWorld4x4,
                                                   (real32 *)*ThisWorld,
                                                   (real32 *)*Composite);
        ++Composite;
    }}
}


void GRANNY
UpdateWorldPoseChildren(skeleton const &Skeleton,
                        int32x ParentBone,
                        local_pose &LocalPose,
                        real32 const *Offset4x4,
                        world_pose &Result)
{
    COUNT_BLOCK("UpdateWorldPoseChildren");

    int32x BoneCount = Skeleton.BoneCount;
    CheckBoundedInt32(0, ParentBone, BoneCount, return);

    // Set the bone's traversal ID to this, then go down
    // the list, updating any bone whose parent matches the ID.
    LocalPose.TraversalID++;
    int32x TraversalID = LocalPose.TraversalID;

    ALIGN16(real32) OffsetBuffer[16];
    if(!Offset4x4)
    {
        Offset4x4 = (real32 const *)GlobalIdentity4x4;
    }

    if(!IS_ALIGNED_16(Offset4x4))
    {
        MatrixEquals4x4(OffsetBuffer, Offset4x4);
        Offset4x4 = OffsetBuffer;
    }

    matrix_4x4 *WorldBuffer = GetWorldPose4x4Array(Result);
    local_pose_transform *FirstLocal = LocalPose.Transforms;
    local_pose_transform *Local = FirstLocal;

    bone *Bone = Skeleton.Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = Result.CompositeTransformBuffer;  // may be null

    Bone += ParentBone;
    World += ParentBone;
    Local += ParentBone;
    BoneCount -= ParentBone;

    // Account for world poses without composite arrays.  Leave the pointer as null
    if (Composite)
        Composite += ParentBone;

    // Slightly odd way to do this loop, because we want to always do
    // ParentBone, but its parent does _not_ have the magic Traversal ID :-)
    while (BoneCount > 0)
    {
        // Redo the current bone.
        int32x ParentIndex = Bone->ParentIndex;
        real32 const *ParentWorld = (ParentIndex == NoParentBone) ?
            Offset4x4 :
            (real32 *)WorldBuffer[ParentIndex];
        transform &Transform = Local->Transform;

        if (Composite)
        {
            BWP_Dispatch(&Transform,
                         ParentWorld,
                         (real32*)Bone->InverseWorld4x4,
                         (real32 *)*Composite,
                         (real32 *)*World);
            ++Composite;
        }
        else
        {
            BWPNC_Dispatch(&Transform,
                           ParentWorld,
                           (real32 *)*World);
        }

        Local->TraversalID = TraversalID;
        ++Bone;
        ++World;
        ++Local;
        --BoneCount;

        // Find the next bone that has a parent with the magic ID.
        while ( BoneCount > 0 )
        {
            int32x ParentIndex = Bone->ParentIndex;
            if ( ParentIndex != NoParentBone )
            {
                // This bone's parent has the magic ID, so we need to re-do it.
                if ( FirstLocal[ParentIndex].TraversalID == TraversalID )
                    break;
            }

            ++Bone;
            ++World;
            ++Local;
            --BoneCount;

            if (Composite)
                ++Composite;
        }
    }
}

