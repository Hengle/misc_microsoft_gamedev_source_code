// ========================================================================
// $File: //jeffr/granny/rt/granny_transform.cpp $
// $DateTime: 2007/09/04 17:04:39 $
// $Change: 15891 $
// $Revision: #13 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void GRANNY
SetTransform(transform &Result,
             real32 const *Position3,
             real32 const *Orientation4,
             real32 const *ScaleShear3x3)
{
    Result.Flags = HasPosition | HasOrientation | HasScaleShear;
    VectorEquals3(Result.Position, Position3);
    VectorEquals4(Result.Orientation, Orientation4);
    MatrixEquals3x3((real32 *)Result.ScaleShear, ScaleShear3x3);
}

void GRANNY
SetTransformWithIdentityCheck(transform &Result,
                              real32 const *Position3,
                              real32 const *Orientation4,
                              real32 const *ScaleShear3x3)
{
    // TODO: Check and set flags
    SetTransform(Result, Position3, Orientation4, ScaleShear3x3);
    Result.Flags =
        (AreEqual(3, Position3, GlobalZeroVector, PositionIdentityThreshold) ? 0 : HasPosition) |
        (AreEqual(4, Orientation4, GlobalWAxis, OrientationIdentityThreshold) ? 0 : HasOrientation) |
        (AreEqual(9, ScaleShear3x3, (real32 const *)GlobalIdentity3x3, ScaleShearIdentityThreshold) ? 0 : HasScaleShear);
}

void GRANNY
MakeIdentity(transform &Result)
{
    Result.Flags = 0;
    VectorZero3(Result.Position);
    VectorEquals4(Result.Orientation, GlobalWAxis);
    MatrixIdentity3x3(Result.ScaleShear);
}

void GRANNY
ZeroTransform(transform &Result)
{
    Result.Flags = 0;
    VectorZero3(Result.Position);
    VectorZero4(Result.Orientation);
    MatrixZero3x3((real32 *)Result.ScaleShear);
}

real32 GRANNY
GetTransformDeterminant(transform const &Transform)
{
    return(MatrixDeterminant3x3((real32 const *)Transform.ScaleShear));
}

void GRANNY
TransformVectorInPlace(real32 *Result, transform const &Transform)
{
    VectorTransform3(Result, (real32 *)Transform.ScaleShear);
    NormalQuaternionTransform3(Result, Transform.Orientation);
}

void GRANNY
TransformVectorInPlaceTransposed(real32 *Result, transform const &Transform)
{
    quad InverseOrientation = {-Transform.Orientation[0],
                               -Transform.Orientation[1],
                               -Transform.Orientation[2],
                               Transform.Orientation[3]};
    NormalQuaternionTransform3(Result, InverseOrientation);
    TransposeVectorTransform3(Result, (real32 const *)Transform.ScaleShear);
}

void GRANNY
TransformVector(real32 *Dest, transform const &Transform,
                real32 const *Source)
{
    VectorTransform3(Dest, (real32 const *)Transform.ScaleShear, Source);
    NormalQuaternionTransform3(Dest, Transform.Orientation);
}

void GRANNY
TransformPointInPlace(real32 *Result, transform const &Transform)
{
    VectorTransform3(Result, (real32 *)Transform.ScaleShear);
    NormalQuaternionTransform3(Result, Transform.Orientation);
    VectorAdd3(Result, Transform.Position);
}

void GRANNY
TransformPoint(real32 *Dest, transform const &Transform,
               real32 const *Source)
{
    VectorTransform3(Dest, (real32 const *)Transform.ScaleShear, Source);
    NormalQuaternionTransform3(Dest, Transform.Orientation);
    VectorAdd3(Dest, Transform.Position);
}

void GRANNY
PreMultiplyByInverse(transform &Transform, transform const &PreMult)
{
    // TODO: Optimize this once Multiply() works optimally
    transform Temp, Temp2;
    BuildInverse(Temp, PreMult);
    Multiply(Temp2, Temp, Transform);
    Transform = Temp2;
}

void GRANNY
PreMultiplyBy(transform &Transform, transform const &PreMult)
{
    // TODO: Optimize this once Multiply() works optimally
    transform Temp;
    Multiply(Temp, PreMult, Transform);
    Transform = Temp;
}

void GRANNY
PostMultiplyBy(transform &Transform, transform const &PostMult)
{
    // TODO: Optimize this once Multiply() works optimally
    transform Temp;
    Multiply(Temp, Transform, PostMult);
    Transform = Temp;
}

void GRANNY
Multiply(transform &Result, transform const &A, transform const &B)
{
    Assert(&Result != &A);
    Assert(&Result != &B);

#if 1
    real32 OrientationA[3][3];
    MatrixEqualsQuaternion3x3((real32 *)OrientationA, A.Orientation);
    real32 OrientationB[3][3];
    MatrixEqualsQuaternion3x3((real32 *)OrientationB, B.Orientation);

    // This does the expanded matrix product of:
    // Transpose(B.Orientation) * A.ScaleShear * B.Orientation * B.ScaleShear;
    real32 Temp[3][3];
    MatrixMultiply3x3((real32 *)Result.ScaleShear, (real32 *)OrientationB,
                      (real32 *)B.ScaleShear);
    MatrixMultiply3x3((real32 *)Temp, (real32 *)A.ScaleShear,
                      (real32 *)Result.ScaleShear);
    TransposeMatrixMultiply3x3((real32 *)Result.ScaleShear, (real32 *)OrientationB,
                               (real32 *)Temp);

    // This does the expanded vector/matrix product of:
    // A.Position + A.Orientation*A.ScaleShear*B.Position
    VectorTransform3(Result.Position, (real32 const *)A.ScaleShear, B.Position);
    VectorTransform3(Result.Position, (real32 *)OrientationA);
    VectorAdd3(Result.Position, A.Position);

    QuaternionMultiply4(Result.Orientation, A.Orientation, B.Orientation);
#else

    polar_decomposition<real32> DecompA(vector_3(A.Position),
                                        vector_4(A.Orientation),
                                        matrix<3, 3, real32>((real32 *)A.ScaleShear));
    polar_decomposition<real32> DecompB(vector_3(B.Position),
                                        vector_4(B.Orientation),
                                        matrix<3, 3, real32>((real32 *)B.ScaleShear));
    polar_decomposition<real32> DecompR = DecompA * DecompB;

    VectorEquals3(Result.Position,
                  DecompR.DirectTranslation().GetElementsPointer());
    VectorEquals4(Result.Orientation,
                  DecompR.DirectOrientation().GetElementsPointer());
    MatrixEquals3x3(Result.ScaleShear,
                    (real32 (*)[3])DecompR.DirectScale().GetElementsPointer());
#endif

    Result.Flags = A.Flags | B.Flags;
}

void GRANNY
LinearBlendTransform(transform &Result, transform const &A, real32 t, transform const &B)
{
    Result.Flags = A.Flags | B.Flags;

    if(Result.Flags & HasPosition)
    {
        ScaleVectorPlusScaleVector3(Result.Position,
                                    (1.0f - t), A.Position,
                                    t, B.Position);
    }
    else
    {
        VectorZero3(Result.Position);
    }

    if(Result.Flags & HasOrientation)
    {
        ScaleVectorPlusScaleVector4(Result.Orientation,
                                    (1.0f - t), A.Orientation,
                                    t, B.Orientation);
        Normalize4(Result.Orientation);
    }
    else
    {
        VectorEquals4(Result.Orientation, GlobalWAxis);
    }

    if(Result.Flags & HasScaleShear)
    {
        ScaleMatrixPlusScaleMatrix3x3((real32 *)Result.ScaleShear,
                                      (1.0f - t), (real32 const *)A.ScaleShear,
                                      t, (real32 const *)B.ScaleShear);
    }
    else
    {
        MatrixIdentity3x3(Result.ScaleShear);
    }
}

void GRANNY
BuildInverse(transform &Result, transform const &Source)
{
    triple ResultPosition;
    quad ResultOrientation;
    matrix_3x3 ResultScaleShear;

#if 1
    // Invert the orientation
    ResultOrientation[0] = -Source.Orientation[0];
    ResultOrientation[1] = -Source.Orientation[1];
    ResultOrientation[2] = -Source.Orientation[2];
    ResultOrientation[3] = Source.Orientation[3];

    triple InverseOrientationMatrix[3];
    MatrixEqualsQuaternion3x3((real32 *)InverseOrientationMatrix,
                              ResultOrientation);

    // Invert the scale/shear (very costly)
    MatrixInvert3x3((real32 *)ResultScaleShear, (real32 *)Source.ScaleShear);
    triple Temp[3];
    MatrixMultiply3x3((real32 *)Temp, (real32 *)ResultScaleShear,
                      (real32 *)InverseOrientationMatrix);
    TransposeMatrixMultiply3x3((real32 *)ResultScaleShear,
                               (real32 *)InverseOrientationMatrix,
                               (real32 *)Temp);

    // Invert the translation
    ResultPosition[0] = -Source.Position[0];
    ResultPosition[1] = -Source.Position[1];
    ResultPosition[2] = -Source.Position[2];
    VectorTransform3(ResultPosition, (real32 *)ResultScaleShear);
    VectorTransform3(ResultPosition, (real32 *)InverseOrientationMatrix);
#else
    polar_decomposition<real32> Decomp(vector_3(Source.Position),
                                       vector_4(Source.Orientation),
                                       matrix<3, 3, real32>((real32 *)Source.ScaleShear));
    Decomp = Inverse(Decomp);
    VectorEquals3(ResultPosition,
                  Decomp.DirectTranslation().GetElementsPointer());
    VectorEquals4(ResultOrientation,
                  Decomp.DirectOrientation().GetElementsPointer());
    MatrixEquals3x3(ResultScaleShear,
                    (real32 (*)[3])Decomp.DirectScale().GetElementsPointer());
#endif

    SetTransformWithIdentityCheck(Result, ResultPosition, ResultOrientation,
                                  (real32 const *)ResultScaleShear);
}

void GRANNY
SimilarityTransform(transform &Result, real32 const *Affine3,
                    real32 const *Linear3x3, real32 const *InverseLinear3x3)
{
    // TODO: Modify flags?
    InPlaceSimilarityTransform(Affine3, Linear3x3, InverseLinear3x3,
                               Result.Position, Result.Orientation,
                               (real32 *)Result.ScaleShear);
}

void GRANNY
BuildCompositeTransform(transform const &Transform,
                        int32 Stride, real32 *Composite3x3)
{
    triple Orientation[3];
    MatrixEqualsQuaternion3x3((real32 *)Orientation, Transform.Orientation);

    triple Upper3x3[3];
    MatrixMultiply3x3((real32 *)Upper3x3,
                      (real32 *)Orientation,
                      (real32 *)Transform.ScaleShear);

    Composite3x3[0] = Upper3x3[0][0];
    Composite3x3[1] = Upper3x3[1][0];
    Composite3x3[2] = Upper3x3[2][0];
    Composite3x3 += Stride;

    Composite3x3[0] = Upper3x3[0][1];
    Composite3x3[1] = Upper3x3[1][1];
    Composite3x3[2] = Upper3x3[2][1];
    Composite3x3 += Stride;

    Composite3x3[0] = Upper3x3[0][2];
    Composite3x3[1] = Upper3x3[1][2];
    Composite3x3[2] = Upper3x3[2][2];
    Composite3x3 += Stride;
}

void GRANNY
BuildCompositeTransform4x4(transform const &Transform, real32 *Composite4x4)
{
    triple Orientation[3];
    MatrixEqualsQuaternion3x3((real32 *)Orientation, Transform.Orientation);

    triple *Upper3x3;
    triple MultBuffer[3];
    if(Transform.Flags & HasScaleShear)
    {
        MatrixMultiply3x3((real32 *)MultBuffer,
                          (real32 *)Orientation,
                          (real32 *)Transform.ScaleShear);
        Upper3x3 = MultBuffer;
    }
    else
    {
        Upper3x3 = Orientation;
    }

    Composite4x4[0] = Upper3x3[0][0];
    Composite4x4[1] = Upper3x3[1][0];
    Composite4x4[2] = Upper3x3[2][0];
    Composite4x4[3] = 0.0f;
    Composite4x4 += 4;

    Composite4x4[0] = Upper3x3[0][1];
    Composite4x4[1] = Upper3x3[1][1];
    Composite4x4[2] = Upper3x3[2][1];
    Composite4x4[3] = 0.0f;
    Composite4x4 += 4;

    Composite4x4[0] = Upper3x3[0][2];
    Composite4x4[1] = Upper3x3[1][2];
    Composite4x4[2] = Upper3x3[2][2];
    Composite4x4[3] = 0.0f;
    Composite4x4 += 4;

    Composite4x4[0] = Transform.Position[0];
    Composite4x4[1] = Transform.Position[1];
    Composite4x4[2] = Transform.Position[2];
    Composite4x4[3] = 1.0f;
}

void GRANNY
BuildCompositeTransform4x3(transform const &Transform, real32 *Composite4x3)
{
    triple Orientation[3];
    MatrixEqualsQuaternion3x3((real32 *)Orientation, Transform.Orientation);

    triple *Upper3x3;
    triple MultBuffer[3];
    if(Transform.Flags & HasScaleShear)
    {
        MatrixMultiply3x3((real32 *)MultBuffer,
                          (real32 *)Orientation,
                          (real32 *)Transform.ScaleShear);
        Upper3x3 = MultBuffer;
    }
    else
    {
        Upper3x3 = Orientation;
    }

    Composite4x3[0] = Upper3x3[0][0];
    Composite4x3[1] = Upper3x3[1][0];
    Composite4x3[2] = Upper3x3[2][0];
    Composite4x3 += 3;

    Composite4x3[0] = Upper3x3[0][1];
    Composite4x3[1] = Upper3x3[1][1];
    Composite4x3[2] = Upper3x3[2][1];
    Composite4x3 += 3;

    Composite4x3[0] = Upper3x3[0][2];
    Composite4x3[1] = Upper3x3[1][2];
    Composite4x3[2] = Upper3x3[2][2];
    Composite4x3 += 3;

    Composite4x3[0] = Transform.Position[0];
    Composite4x3[1] = Transform.Position[1];
    Composite4x3[2] = Transform.Position[2];
}

#if 0
void GRANNY
BuildTransposedCompositeTransform(transform const &Transform,
                                  int32 Stride, real32 *Composite3x3)
{
    triple Orientation[3];
    MatrixEqualsQuaternion3x3((real32 *)Orientation, Transform.Orientation);

    triple Upper3x3[3];
    MatrixMultiply3x3((real32 *)Upper3x3,
                      (real32 *)Orientation,
                      (real32 *)Transform.ScaleShear);

    Composite3x3[0] = Upper3x3[0][0];
    Composite3x3[1] = Upper3x3[1][0];
    Composite3x3[2] = Upper3x3[2][0];
    Composite3x3 += Stride;

    Composite3x3[0] = Upper3x3[0][1];
    Composite3x3[1] = Upper3x3[1][1];
    Composite3x3[2] = Upper3x3[2][1];
    Composite3x3 += Stride;

    Composite3x3[0] = Upper3x3[0][2];
    Composite3x3[1] = Upper3x3[1][2];
    Composite3x3[2] = Upper3x3[2][2];
    Composite3x3 += Stride;
}

void GRANNY
BuildTransposedCompositeTransform4(transform const &Transform,
                                   int32x Stride,
                                   real32 *Composite3x4)
{
    triple Orientation[3];
    MatrixEqualsQuaternion3x3((real32 *)Orientation, Transform.Orientation);

    triple Upper3x3[3];
    MatrixMultiply3x3((real32 *)Upper3x3,
                      (real32 *)Orientation,
                      (real32 *)Transform.ScaleShear);

    Composite3x4[0] = Upper3x3[0][0];
    Composite3x4[1] = Upper3x3[1][0];
    Composite3x4[2] = Upper3x3[2][0];
    Composite3x4 += Stride;

    Composite3x4[0] = Upper3x3[0][1];
    Composite3x4[1] = Upper3x3[1][1];
    Composite3x4[2] = Upper3x3[2][1];
    Composite3x4 += Stride;

    Composite3x4[0] = Upper3x3[0][2];
    Composite3x4[1] = Upper3x3[1][2];
    Composite3x4[2] = Upper3x3[2][2];
    Composite3x4 += Stride;

    Composite3x4[0] = Transform.Position[0];
    Composite3x4[1] = Transform.Position[1];
    Composite3x4[2] = Transform.Position[2];
}

void GRANNY
BuildTransposedCompositeTransformIT(transform const &Transform,
                                    int32 Stride,
                                    real32 *Composite3x3,
                                    real32 *ITComposite3x3)
{
    triple Orientation[3];
    MatrixEqualsQuaternion3x3((real32 *)Orientation, Transform.Orientation);

    triple Upper3x3[3];
    MatrixMultiply3x3((real32 *)Upper3x3,
                      (real32 *)Orientation,
                      (real32 *)Transform.ScaleShear);

    Composite3x3[0] = Upper3x3[0][0];
    Composite3x3[1] = Upper3x3[1][0];
    Composite3x3[2] = Upper3x3[2][0];
    Composite3x3 += Stride;

    Composite3x3[0] = Upper3x3[0][1];
    Composite3x3[1] = Upper3x3[1][1];
    Composite3x3[2] = Upper3x3[2][1];
    Composite3x3 += Stride;

    Composite3x3[0] = Upper3x3[0][2];
    Composite3x3[1] = Upper3x3[1][2];
    Composite3x3[2] = Upper3x3[2][2];
    Composite3x3 += Stride;

    triple ITUpper3x3[3];
    MatrixInvert3x3((real32 *)ITUpper3x3, (real32 *)Upper3x3);
    MatrixTranspose3x3(ITUpper3x3);

    ITComposite3x3[0] = ITUpper3x3[0][0];
    ITComposite3x3[1] = ITUpper3x3[1][0];
    ITComposite3x3[2] = ITUpper3x3[2][0];
    ITComposite3x3 += Stride;

    ITComposite3x3[0] = ITUpper3x3[0][1];
    ITComposite3x3[1] = ITUpper3x3[1][1];
    ITComposite3x3[2] = ITUpper3x3[2][1];
    ITComposite3x3 += Stride;

    ITComposite3x3[0] = ITUpper3x3[0][2];
    ITComposite3x3[1] = ITUpper3x3[1][2];
    ITComposite3x3[2] = ITUpper3x3[2][2];
    ITComposite3x3 += Stride;
}

void GRANNY
TransformMultiplyAccumulate(transform &Result,
                            real32 C1, transform const &Transform)
{
    real32 const C0 = 1.0f - C1;

    ScaleVectorPlusScaleVector3(Result.Position,
                                C0, Result.Position,
                                C1, Transform.Position);

    QuaternionInterpolate4(Result.Orientation,
                           C0, Result.Orientation,
                           C1, Transform.Orientation);
    NormalizeCloseToOne4(Result.Orientation);

    // TODO: Make this a math op.
    real32 *Scale0 = &Result.ScaleShear[0][0];
    real32 const *Scale1 = &Transform.ScaleShear[0][0];
    Scale0[0] = (C0*Scale0[0] + C1*Scale1[0]);
    Scale0[1] = (C0*Scale0[1] + C1*Scale1[1]);
    Scale0[2] = (C0*Scale0[2] + C1*Scale1[2]);
    Scale0[3] = (C0*Scale0[3] + C1*Scale1[3]);
    Scale0[4] = (C0*Scale0[4] + C1*Scale1[4]);
    Scale0[5] = (C0*Scale0[5] + C1*Scale1[5]);
    Scale0[6] = (C0*Scale0[6] + C1*Scale1[6]);
    Scale0[7] = (C0*Scale0[7] + C1*Scale1[7]);
    Scale0[8] = (C0*Scale0[8] + C1*Scale1[8]);
}
#endif
