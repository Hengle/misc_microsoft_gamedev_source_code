// ========================================================================
// $File: //jeffr/granny/rt/granny_bone_operations.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

// Routines that only generate the world-pose, not the composite.
void GRANNY
BuildIdentityWorldPoseOnly_Generic(
    real32 const *ParentMatrix,
    real32 *ResultWorldMatrix)
{
    MatrixEquals4x4(ResultWorldMatrix, ParentMatrix);
}

void GRANNY
BuildPositionWorldPoseOnly_Generic(
    real32 const *Position,
    real32 const *ParentMatrix,
    real32 *ResultWorldMatrix)
{
    MatrixEquals4x4(ResultWorldMatrix, ParentMatrix);
    ResultWorldMatrix[12] = (Position[0]*ParentMatrix[0] +
                             Position[1]*ParentMatrix[4] +
                             Position[2]*ParentMatrix[8] +
                             ParentMatrix[12]);
    ResultWorldMatrix[13] = (Position[0]*ParentMatrix[1] +
                             Position[1]*ParentMatrix[5] +
                             Position[2]*ParentMatrix[9] +
                             ParentMatrix[13]);
    ResultWorldMatrix[14] = (Position[0]*ParentMatrix[2] +
                             Position[1]*ParentMatrix[6] +
                             Position[2]*ParentMatrix[10] +
                             ParentMatrix[14]);
}

void GRANNY
BuildPositionOrientationWorldPoseOnly_Generic(
    real32 const *Position, real32 const *Orientation,
    real32 const *ParentMatrix,
    real32 * ResultWorldMatrix)
{
    transform Transform;
    Transform.Flags = HasOrientation | HasPosition;
    VectorEquals3(Transform.Position, Position);
    VectorEquals4(Transform.Orientation, Orientation);
    BuildFullWorldPoseOnly_Generic(Transform, ParentMatrix, ResultWorldMatrix);
}

void GRANNY
BuildFullWorldPoseOnly_Generic(
    transform const &Transform,
    real32 const *ParentMatrix, real32 * ResultWorldMatrix)
{
    matrix_4x4 Temp;
    BuildCompositeTransform4x4(Transform, (real32 *)Temp);
    ColumnMatrixMultiply4x3(ResultWorldMatrix, (real32 *)Temp, ParentMatrix);
}



// Generating the composite from the world.
void GRANNY
BuildSingleCompositeFromWorldPose_Generic(
    real32 const *InverseWorld4x4, real32 const *WorldMatrix,
    real32 *ResultComposite )
{
    ColumnMatrixMultiply4x3(ResultComposite, InverseWorld4x4, WorldMatrix);
}

void GRANNY
BuildSingleCompositeFromWorldPoseTranspose_Generic(
    real32 const *InverseWorld4x4, real32 const *WorldMatrix,
    real32 *ResultComposite3x4 )
{
    ColumnMatrixMultiply4x3Transpose(ResultComposite3x4, InverseWorld4x4, WorldMatrix);
}




// Routines that generate both world-pose and composite at the same time.
void GRANNY
BuildIdentityWorldPoseComposite_Generic(
    real32 const *ParentMatrix,
    real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 *ResultWorldMatrix)
{
    BuildIdentityWorldPoseOnly_Generic(ParentMatrix, ResultWorldMatrix);
    BuildSingleCompositeFromWorldPose_Generic(InverseWorld4x4, ResultWorldMatrix, ResultComposite);
}

void GRANNY
BuildPositionWorldPoseComposite_Generic(
    real32 const *Position,
    real32 const *ParentMatrix,
    real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 *ResultWorldMatrix)
{
    BuildPositionWorldPoseOnly_Generic(Position, ParentMatrix, ResultWorldMatrix);
    BuildSingleCompositeFromWorldPose_Generic(InverseWorld4x4, ResultWorldMatrix, ResultComposite);
}

void GRANNY
BuildPositionOrientationWorldPoseComposite_Generic(
    real32 const *Position, real32 const *Orientation,
    real32 const *ParentMatrix, real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 * ResultWorldMatrix)
{
    BuildPositionOrientationWorldPoseOnly_Generic(Position, Orientation, ParentMatrix, ResultWorldMatrix);
    BuildSingleCompositeFromWorldPose_Generic(InverseWorld4x4, ResultWorldMatrix, ResultComposite);
}

void GRANNY
BuildFullWorldPoseComposite_Generic(
    transform const &Transform,
    real32 const *ParentMatrix, real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 * ResultWorldMatrix)
{
    BuildFullWorldPoseOnly_Generic(Transform, ParentMatrix, ResultWorldMatrix);
    BuildSingleCompositeFromWorldPose_Generic(InverseWorld4x4, ResultWorldMatrix, ResultComposite);
}





