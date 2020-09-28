// ========================================================================
// $File: //jeffr/granny/rt/granny_ik.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_IK_H)
#include "granny_ik.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOCAL_POSE_H)
#include "granny_local_pose.h"
#endif

#if !defined(GRANNY_WORLD_POSE_H)
#include "granny_world_pose.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_MODEL_INSTANCE_H)
#include "granny_model_instance.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif


#undef SubsystemCode
#define SubsystemCode IKLogMessage

USING_GRANNY_NAMESPACE;

void GRANNY
IKUpdate(int32x LinkCountInit, int32x EEBoneIndex,
         real32 const *DesiredPosition3,
         int32x IterationCount,
         skeleton const &Skeleton,
         real32 const *ModelRootTransform,
         local_pose &LocalPose,
         world_pose &WorldPose)
{
    COUNT_BLOCK("IKUpdate");

    Assert ( LinkCountInit <= MaximumIKLinkCount );
    int32x Links[MaximumIKLinkCount];
    int32x LinkCount = 0;
    int32x BoneIndex = EEBoneIndex;
    {for(int32x LinkIndex = 0;
         LinkIndex <= LinkCountInit;
         ++LinkIndex)
    {
        Links[LinkIndex] = BoneIndex;
        BoneIndex = Skeleton.Bones[BoneIndex].ParentIndex;
        ++LinkCount;
        if(BoneIndex == NoParentBone)
        {
            break;
        }
    }}

    {for(int32x CCDIteration = 0;
         CCDIteration < IterationCount;
         ++CCDIteration)
    {
        {for(int32x LinkIndex = 1;
             LinkIndex < LinkCount;
             ++LinkIndex)
        {
            real32 CurrentPosition[3];
            VectorEquals3(CurrentPosition,
                          &GetWorldPose4x4(WorldPose, Links[0])[12]);

            int32x BoneIndex = Links[LinkIndex];
            transform *Relative = GetLocalPoseTransform(LocalPose, BoneIndex);
            real32 const *Absolute = GetWorldPose4x4(WorldPose, BoneIndex);

            triple VectorToDesired;
            VectorSubtract3(VectorToDesired, DesiredPosition3, &Absolute[12]);
            TransposeVectorTransform4x3(VectorToDesired, 0.0f, Absolute);

            triple VectorToActual;
            VectorSubtract3(VectorToActual, CurrentPosition, &Absolute[12]);
            TransposeVectorTransform4x3(VectorToActual, 0.0f, Absolute);

            VectorAdd3(VectorToActual, VectorToDesired);
            VectorScale3(VectorToActual, 0.5f);

            quad LocalRotationIncrement;
            VectorCrossProduct3(LocalRotationIncrement,
                                VectorToActual, VectorToDesired);

            LocalRotationIncrement[3] = InnerProduct3(
                VectorToActual, VectorToDesired);

            Normalize4(LocalRotationIncrement);

            Relative->Flags |= HasOrientation;
            QuaternionMultiply4(Relative->Orientation,
                                Relative->Orientation,
                                LocalRotationIncrement);

            UpdateWorldPoseChildren(Skeleton,
                                    BoneIndex,
                                    LocalPose,
                                    ModelRootTransform,
                                    WorldPose);
        }}
    }}
}



bool GRANNY
IKUpdate2Bone(int32x EEBoneIndex,
              real32 const *DesiredPosition3,
              real32 const *RestrictedMovementPlaneNormal3,
              skeleton const &Skeleton,
              real32 const *ModelRootTransform,
              local_pose &LocalPose,
              world_pose &WorldPose,
              real32 HyperExtensionStart,
              real32 HyperExtensionScale)
{
    bool Success = true;

    // The bone hierarchy goes L1->L2->EE
    // L = "link", EE = end effector.
    int32x L2BoneIndex = Skeleton.Bones[EEBoneIndex].ParentIndex;
    CheckCountedInt32 ( L2BoneIndex, Skeleton.BoneCount, return false );
    int32x L1BoneIndex = Skeleton.Bones[L2BoneIndex].ParentIndex;
    CheckCountedInt32 ( L1BoneIndex, Skeleton.BoneCount, return false );
    // We want to change the local orientations (but not positions) of
    // L1 and L2 so that the world-space position of EE ends up at DesiredPosition3.

    // "WS" = WorldSpace - these are NOT in local space!
    // This doesn't move.
    real32 *L1WS = &(GetWorldPose4x4(WorldPose, L1BoneIndex)[12]);
    // These two will, but I need to know distances first.
    real32 *L2WS = &(GetWorldPose4x4(WorldPose, L2BoneIndex)[12]);
    real32 *EEWS = &(GetWorldPose4x4(WorldPose, EEBoneIndex)[12]);

    // Vector from L1 to desired EE
    triple EEL1WS;
    VectorSubtract3 ( EEL1WS, DesiredPosition3, L1WS );
    real32 EEL1LengthSq = VectorLengthSquared3 ( EEL1WS );

    // "Length" of the two bones to rotate.
    // (note that this needs to be taken from the WS stuff because
    // we need to cope as best we can with scales and shears
    // (though I suspect shears and non-uniform scales will not be dealt with very elegantly).
    real32 L1L2LengthSq = SquaredDistanceBetween3 ( L1WS, L2WS );
    real32 L2EELengthSq = SquaredDistanceBetween3 ( L2WS, EEWS );

    // So what we need to find is the new L2 origin. L1 is fixed, and
    // the new EE is defined by the caller.

    // First find the point P along the line L1->EE that is nearest to the new L2
    // We know that:
    // P = L1 + lambda*(EE-L1)
    // and because angles L2-P-L1 and EE-P-L1 are right angles:
    // |P-L2|^2 + |P-L1|^2 = |L1-L2|^2
    // |P-L2|^2 + |P-EE|^2 = |EE-L2|^2
    // So we can solve for all those and find that:
    // lambda = 0.5 + ( ( |L1-L2|^2 + |EE-L2|^2 ) / ( 2 * |EE-L1|^2 ) )
    real32 Lambda = 0.0f;
#define SomeSmallTolerance 1e-8f
    if ( AbsoluteValue ( EEL1LengthSq ) < SomeSmallTolerance )
    {
        // They're basically coincident - you're doomed. A lambda of zero
        // is as good a value as any.
        Success = false;
    }
    else
    {
        Lambda = ( ( L1L2LengthSq - L2EELengthSq ) / EEL1LengthSq ) * 0.5f + 0.5f;
    }
    // Note that lambda can happily be <0.0f or >1.0f (though not _too_ far - we'll catch this case later)

    // So we know where P is: P = L1 + lambda*(EE-L1)
    triple PWS;
    ScaleVectorAdd3 ( PWS, L1WS, Lambda, EEL1WS );

    // And now we need to know the vector P->L2.
    // We know it's perpendicular to both EEL1 and RestrictedMovementPlaneNormal3,
    // and we know its length is sqrt(|L1-L2|^2 - |P-L1|^2)
    // (and |P-L1| == |EE-L1|*lambda)
    // So we just take the cross product of those two vectors, and normalise.
    // Although there's two possible vectors at right-angles to the two,
    // one the negative of the other, one of the purposes of RestrictedMovementPlaneNormal3
    // is to indicate which direction is the preferred one, so we always take the cross
    // product the same way.
    triple PL2WS;
    VectorCrossProduct3 ( PL2WS, EEL1WS, RestrictedMovementPlaneNormal3 );
    real32 TempLengthSq = VectorLengthSquared3 ( PL2WS );
    real32 DesiredLengthSq = L1L2LengthSq - ( Lambda * Lambda * EEL1LengthSq );

    // As DesiredLengthSq approaches zero, the joint is straightening.
    // This can happen very fast for very little end-effector movement,
    // and leads to "knee pop". This is a fairly good attempt at softening the
    // final motion, and stops the knee or elbow smacking into its stops quite as violently.
    // Good numbers are:
    // HyperExtensionStart = 0.1f;
    // HyperExtensionScale = 0.5f;
    if ( DesiredLengthSq < HyperExtensionStart * L1L2LengthSq )
    {
        // We're almost trying to hyper-extend, so make the transition into locking the
        // joint a lot gentler to avoid "knee pop".
        float Delta = DesiredLengthSq - HyperExtensionStart * L1L2LengthSq;
        Delta *= HyperExtensionScale;
        DesiredLengthSq = Delta + HyperExtensionStart * L1L2LengthSq;
        if ( DesiredLengthSq < 0.0f )
        {
            DesiredLengthSq = 0.0f;
        }
    }

    // Scale PL2WS and add to PWS to get the new L2.
    triple NewL2WS;
    ScaleVectorAdd3 ( NewL2WS, PWS, SquareRoot ( DesiredLengthSq / TempLengthSq ), PL2WS );


    // So, those are all in world-space. Now we need to rotate the
    // bone hierarchy to make those values happen.
    // This code is uncannily similar to the general case above,
    // since it basically does the same thing, except we don't need
    // to iterate now.

    // First, move the L1 bone so that L2's origin is in the correct place.
    {
        int32x BoneIndex = L1BoneIndex;
        const real32 *DesiredPosition = NewL2WS;
        const real32 *CurrentPosition = L2WS;


        transform *Relative = GetLocalPoseTransform(LocalPose, BoneIndex);
        real32 const *Absolute = GetWorldPose4x4(WorldPose, BoneIndex);

        triple VectorToDesired;
        VectorSubtract3(VectorToDesired, DesiredPosition, &Absolute[12]);
        TransposeVectorTransform4x3(VectorToDesired, 0.0f, Absolute);

        triple VectorToActual;
        VectorSubtract3(VectorToActual, CurrentPosition, &Absolute[12]);
        TransposeVectorTransform4x3(VectorToActual, 0.0f, Absolute);

        VectorAdd3(VectorToActual, VectorToDesired);
        VectorScale3(VectorToActual, 0.5f);

        quad LocalRotationIncrement;
        VectorCrossProduct3(LocalRotationIncrement,
                            VectorToActual, VectorToDesired);

        LocalRotationIncrement[3] = InnerProduct3(
            VectorToActual, VectorToDesired);

        Normalize4(LocalRotationIncrement);

        Relative->Flags |= HasOrientation;
        QuaternionMultiply4(Relative->Orientation,
                            Relative->Orientation,
                            LocalRotationIncrement);
    }

    // Re-do the world-space matrices for L1 and L2.
    // We could just call BuildWorldPose, but we just want these two bones.
    int32x L1ParentBoneIndex = Skeleton.Bones[L1BoneIndex].ParentIndex;
    ALIGN16(real32) Offset4x4[16];
    real32 *ParentWorld;
    if ( L1ParentBoneIndex == NoParentBone )
    {
        MatrixEquals4x4(Offset4x4, ModelRootTransform);
        ParentWorld = Offset4x4;
    }
    else
    {
        Assert ( L1ParentBoneIndex >= 0 );
        Assert ( L1ParentBoneIndex < WorldPose.BoneCount );
        ParentWorld = GetWorldPose4x4(WorldPose, L1ParentBoneIndex);
    }

#if 0
    BuildWorldPose(
        Skeleton, 0, Skeleton.BoneCount, LocalPose,
        ModelRootTransform,
        WorldPose);
#endif

    if (GetWorldPoseComposite4x4Array(WorldPose))
    {
        BuildFullWorldPoseComposite ( *GetLocalPoseTransform ( LocalPose, L1BoneIndex ),
                                      ParentWorld,
                                      (real32*)Skeleton.Bones[L1BoneIndex].InverseWorld4x4,
                                      GetWorldPoseComposite4x4 ( WorldPose, L1BoneIndex ),
                                      GetWorldPose4x4 ( WorldPose, L1BoneIndex ) );

        BuildFullWorldPoseComposite ( *GetLocalPoseTransform ( LocalPose, L2BoneIndex ),
                                      GetWorldPose4x4 ( WorldPose, L1BoneIndex ),
                                      (real32*)Skeleton.Bones[L2BoneIndex].InverseWorld4x4,
                                      GetWorldPoseComposite4x4 ( WorldPose, L2BoneIndex ),
                                      GetWorldPose4x4 ( WorldPose, L2BoneIndex ) );

        BuildFullWorldPoseComposite ( *GetLocalPoseTransform ( LocalPose, EEBoneIndex ),
                                      GetWorldPose4x4 ( WorldPose, L2BoneIndex ),
                                      (real32*)Skeleton.Bones[EEBoneIndex].InverseWorld4x4,
                                      GetWorldPoseComposite4x4 ( WorldPose, EEBoneIndex ),
                                      GetWorldPose4x4 ( WorldPose, EEBoneIndex ) );
    }
    else
    {
        BuildFullWorldPoseOnly(*GetLocalPoseTransform ( LocalPose, L1BoneIndex ),
                               ParentWorld,
                               GetWorldPose4x4 ( WorldPose, L1BoneIndex ) );

        BuildFullWorldPoseOnly(*GetLocalPoseTransform ( LocalPose, L2BoneIndex ),
                               GetWorldPose4x4 ( WorldPose, L1BoneIndex ),
                               GetWorldPose4x4 ( WorldPose, L2BoneIndex ) );

        BuildFullWorldPoseOnly(*GetLocalPoseTransform ( LocalPose, EEBoneIndex ),
                               GetWorldPose4x4 ( WorldPose, L2BoneIndex ),
                               GetWorldPose4x4 ( WorldPose, EEBoneIndex ) );
    }

#if 0
#if DEBUG
    if ( CheckStuff )
    {
        real32 OrigWorldL1[16];
        real32 OrigWorldL2[16];
        real32 OrigWorldEE[16];
        MatrixEquals4x4 ( OrigWorldL1, GetWorldPose4x4 ( WorldPose, L1BoneIndex ) );
        MatrixEquals4x4 ( OrigWorldL2, GetWorldPose4x4 ( WorldPose, L2BoneIndex ) );
        MatrixEquals4x4 ( OrigWorldEE, GetWorldPose4x4 ( WorldPose, EEBoneIndex ) );

        real32 OrigCompositeL1[16];
        real32 OrigCompositeL2[16];
        real32 OrigCompositeEE[16];
        MatrixEquals4x4 ( OrigCompositeL1, GetWorldPoseComposite4x4 ( WorldPose, L1BoneIndex ) );
        MatrixEquals4x4 ( OrigCompositeL2, GetWorldPoseComposite4x4 ( WorldPose, L2BoneIndex ) );
        MatrixEquals4x4 ( OrigCompositeEE, GetWorldPoseComposite4x4 ( WorldPose, EEBoneIndex ) );

        BuildWorldPose(
            Skeleton, 0, Skeleton.BoneCount, LocalPose,
            ModelRootTransform,
            WorldPose);

        real32 *NewWorldL1 = GetWorldPose4x4 ( WorldPose, L1BoneIndex );
        real32 *NewWorldL2 = GetWorldPose4x4 ( WorldPose, L2BoneIndex );
        real32 *NewWorldEE = GetWorldPose4x4 ( WorldPose, L2BoneIndex );
        real32 *NewCompositeL1 = GetWorldPoseComposite4x4 ( WorldPose, L1BoneIndex );
        real32 *NewCompositeL2 = GetWorldPoseComposite4x4 ( WorldPose, L2BoneIndex );
        real32 *NewCompositeEE = GetWorldPoseComposite4x4 ( WorldPose, L2BoneIndex );
        for ( int i = 0; i < 16; i++ )
        {
            Assert ( AbsoluteValue ( OrigWorldL1[i] - NewWorldL1[i] ) < 0.001f );
            Assert ( AbsoluteValue ( OrigWorldL2[i] - NewWorldL2[i] ) < 0.001f );
            Assert ( AbsoluteValue ( OrigWorldEE[i] - NewWorldEE[i] ) < 0.001f );
            Assert ( AbsoluteValue ( OrigCompositeL1[i] - NewCompositeL1[i] ) < 0.001f );
            Assert ( AbsoluteValue ( OrigCompositeL2[i] - NewCompositeL2[i] ) < 0.001f );
            Assert ( AbsoluteValue ( OrigCompositeEE[i] - NewCompositeEE[i] ) < 0.001f );
        }
    }
#endif
#endif

    // Now, move the L2 bone so that EE's origin is in the correct place.
    {
        int32x BoneIndex = L2BoneIndex;
        const real32 *DesiredPosition = DesiredPosition3;
        const real32 *CurrentPosition = EEWS;

        transform *Relative = GetLocalPoseTransform(LocalPose, BoneIndex);
        real32 const *Absolute = GetWorldPose4x4(WorldPose, BoneIndex);

        triple VectorToDesired;
        VectorSubtract3(VectorToDesired, DesiredPosition, &Absolute[12]);
        TransposeVectorTransform4x3(VectorToDesired, 0.0f, Absolute);

        triple VectorToActual;
        VectorSubtract3(VectorToActual, CurrentPosition, &Absolute[12]);
        TransposeVectorTransform4x3(VectorToActual, 0.0f, Absolute);

        VectorAdd3(VectorToActual, VectorToDesired);
        VectorScale3(VectorToActual, 0.5f);

        quad LocalRotationIncrement;
        VectorCrossProduct3(LocalRotationIncrement,
                            VectorToActual, VectorToDesired);

        LocalRotationIncrement[3] = InnerProduct3(
            VectorToActual, VectorToDesired);

        Normalize4(LocalRotationIncrement);

        Relative->Flags |= HasOrientation;
        QuaternionMultiply4(Relative->Orientation,
                            Relative->Orientation,
                            LocalRotationIncrement);
    }

    // Update from L1BoneIndex
    UpdateWorldPoseChildren(Skeleton, L1BoneIndex,
                            LocalPose,
                            ModelRootTransform,
                            WorldPose);

    return Success;
}





