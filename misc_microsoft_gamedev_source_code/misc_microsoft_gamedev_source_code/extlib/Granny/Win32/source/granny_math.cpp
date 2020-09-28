// ========================================================================
// $File: //jeffr/granny/rt/granny_math.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #13 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void GRANNY
MatrixInvert3x3(real32 *DestInit, real32 const *SourceInit)
{
    triple *Dest = (triple *)DestInit;
    triple const *Source = (triple const *)SourceInit;

    Assert(Source != Dest);

    // TODO: This inverse is neither fast nor numerically stable.
    //       It should be replaced with some real code.

    real32 const Det = MatrixDeterminant3x3((real32 const *)Source);

    // TODO: Use some sort of norm here!
    if(Det != 0)
    {
        real32 const InverseDet = 1.0f / Det;

        Dest[0][0] =  (Source[1][1]*Source[2][2] -
                       Source[1][2]*Source[2][1]) * InverseDet;
        Dest[1][0] = -(Source[1][0]*Source[2][2] -
                       Source[1][2]*Source[2][0]) * InverseDet;
        Dest[2][0] =  (Source[1][0]*Source[2][1] -
                       Source[1][1]*Source[2][0]) * InverseDet;

        Dest[0][1] = -(Source[0][1]*Source[2][2] -
                       Source[0][2]*Source[2][1]) * InverseDet;
        Dest[1][1] =  (Source[0][0]*Source[2][2] -
                       Source[0][2]*Source[2][0]) * InverseDet;
        Dest[2][1] = -(Source[0][0]*Source[2][1] -
                       Source[0][1]*Source[2][0]) * InverseDet;

        Dest[0][2] =  (Source[0][1]*Source[1][2] -
                       Source[0][2]*Source[1][1]) * InverseDet;
        Dest[1][2] = -(Source[0][0]*Source[1][2] -
                       Source[0][2]*Source[1][0]) * InverseDet;
        Dest[2][2] =  (Source[0][0]*Source[1][1] -
                       Source[0][1]*Source[1][0]) * InverseDet;
    }
}

void GRANNY
MatrixInvert4x3(real32 *DestInit, real32 const *SourceInit)
{
    quad *Dest = (quad *)DestInit;
    quad const *Source = (quad const *)SourceInit;

    Assert(Source != Dest);

    // TODO: This inverse is neither fast nor numerically stable.
    //       It should be replaced with some real code.

    real32 const Det = MatrixDeterminant4x3((real32 const *)Source);

    // TODO: Use some sort of norm here!
    if(Det != 0)
    {
        real32 const InverseDet = 1.0f / Det;

        Dest[0][0] =  (Source[1][1]*Source[2][2] -
                       Source[2][1]*Source[1][2]) * InverseDet;
        Dest[0][1] = -(Source[0][1]*Source[2][2] -
                       Source[2][1]*Source[0][2]) * InverseDet;
        Dest[0][2] =  (Source[0][1]*Source[1][2] -
                       Source[1][1]*Source[0][2]) * InverseDet;

        Dest[1][0] = -(Source[1][0]*Source[2][2] -
                       Source[2][0]*Source[1][2]) * InverseDet;
        Dest[1][1] =  (Source[0][0]*Source[2][2] -
                       Source[2][0]*Source[0][2]) * InverseDet;
        Dest[1][2] = -(Source[0][0]*Source[1][2] -
                       Source[1][0]*Source[0][2]) * InverseDet;

        Dest[2][0] =  (Source[1][0]*Source[2][1] -
                       Source[2][0]*Source[1][1]) * InverseDet;
        Dest[2][1] = -(Source[0][0]*Source[2][1] -
                       Source[2][0]*Source[0][1]) * InverseDet;
        Dest[2][2] =  (Source[0][0]*Source[1][1] -
                       Source[1][0]*Source[0][1]) * InverseDet;
    }

    Dest[0][3] = Dest[1][3] = Dest[2][3] = 0.0f;

    Dest[3][0] = -(Dest[0][0]*Source[3][0] +
                   Dest[1][0]*Source[3][1] +
                   Dest[2][0]*Source[3][2]);
    Dest[3][1] = -(Dest[0][1]*Source[3][0] +
                   Dest[1][1]*Source[3][1] +
                   Dest[2][1]*Source[3][2]);
    Dest[3][2] = -(Dest[0][2]*Source[3][0] +
                   Dest[1][2]*Source[3][1] +
                   Dest[2][2]*Source[3][2]);
    Dest[3][3] = 1.0f;
}

void GRANNY
NormalizeMatrix3x3(real32 *Dest, real32 const *Source)
{
    if(Dest != Source)
    {
        MatrixEquals3x3(Dest, Source);
    }

    NormalizeOrZero3(&Dest[0]);
    NormalizeOrZero3(&Dest[3]);
    NormalizeOrZero3(&Dest[6]);
}

void GRANNY
MatrixMultiply3x3(real32 *IntoMatrix,
                  real32 const *Matrix,
                  real32 const *ByMatrix)
{
    Assert(Matrix != IntoMatrix);
    Assert(ByMatrix != IntoMatrix);

#if 0
    // 27 multiplies, 18 adds
    {for(int32x RowIndex = 0;
         RowIndex < 3;
         ++RowIndex)
    {
        IntoMatrix[0] = (Matrix[0]*ByMatrix[0 + 0] +
                         Matrix[1]*ByMatrix[3 + 0] +
                         Matrix[2]*ByMatrix[6 + 0]);
        IntoMatrix[1] = (Matrix[0]*ByMatrix[0 + 1] +
                         Matrix[1]*ByMatrix[3 + 1] +
                         Matrix[2]*ByMatrix[6 + 1]);
        IntoMatrix[2] = (Matrix[0]*ByMatrix[0 + 2] +
                         Matrix[1]*ByMatrix[3 + 2] +
                         Matrix[2]*ByMatrix[6 + 2]);

        Matrix += 3;
        IntoMatrix += 3;
    }}
#else
    triple *Result = (triple *)IntoMatrix;
    triple const *A = (triple const *)Matrix;
    triple const *B = (triple const *)ByMatrix;

    Result[0][0] = A[0][0]*B[0][0] + A[0][1]*B[1][0] + A[0][2]*B[2][0];
    Result[0][1] = A[0][0]*B[0][1] + A[0][1]*B[1][1] + A[0][2]*B[2][1];
    Result[0][2] = A[0][0]*B[0][2] + A[0][1]*B[1][2] + A[0][2]*B[2][2];

    Result[1][0] = A[1][0]*B[0][0] + A[1][1]*B[1][0] + A[1][2]*B[2][0];
    Result[1][1] = A[1][0]*B[0][1] + A[1][1]*B[1][1] + A[1][2]*B[2][1];
    Result[1][2] = A[1][0]*B[0][2] + A[1][1]*B[1][2] + A[1][2]*B[2][2];

    Result[2][0] = A[2][0]*B[0][0] + A[2][1]*B[1][0] + A[2][2]*B[2][0];
    Result[2][1] = A[2][0]*B[0][1] + A[2][1]*B[1][1] + A[2][2]*B[2][1];
    Result[2][2] = A[2][0]*B[0][2] + A[2][1]*B[1][2] + A[2][2]*B[2][2];
#endif
}

void GRANNY
TransposeMatrixMultiply3x3(real32 *IntoMatrix,
                           real32 const *TransposedMatrix,
                           real32 const *ByMatrix)
{
    Assert(TransposedMatrix != IntoMatrix);
    Assert(ByMatrix != IntoMatrix);

#if 0
    real32 const *Matrix = TransposedMatrix;

    {for(int32x RowIndex = 0;
         RowIndex < 3;
         ++RowIndex)
    {
        IntoMatrix[0] = (Matrix[0]*ByMatrix[0 + 0] +
                         Matrix[3]*ByMatrix[3 + 0] +
                         Matrix[6]*ByMatrix[6 + 0]);
        IntoMatrix[1] = (Matrix[0]*ByMatrix[0 + 1] +
                         Matrix[3]*ByMatrix[3 + 1] +
                         Matrix[6]*ByMatrix[6 + 1]);
        IntoMatrix[2] = (Matrix[0]*ByMatrix[0 + 2] +
                         Matrix[3]*ByMatrix[3 + 2] +
                         Matrix[6]*ByMatrix[6 + 2]);

        Matrix += 1;
        IntoMatrix += 3;
    }}
#else
    triple *Result = (triple *)IntoMatrix;
    triple const *A = (triple const *)TransposedMatrix;
    triple const *B = (triple const *)ByMatrix;

    Result[0][0] = A[0][0]*B[0][0] + A[1][0]*B[1][0] + A[2][0]*B[2][0];
    Result[0][1] = A[0][0]*B[0][1] + A[1][0]*B[1][1] + A[2][0]*B[2][1];
    Result[0][2] = A[0][0]*B[0][2] + A[1][0]*B[1][2] + A[2][0]*B[2][2];

    Result[1][0] = A[0][1]*B[0][0] + A[1][1]*B[1][0] + A[2][1]*B[2][0];
    Result[1][1] = A[0][1]*B[0][1] + A[1][1]*B[1][1] + A[2][1]*B[2][1];
    Result[1][2] = A[0][1]*B[0][2] + A[1][1]*B[1][2] + A[2][1]*B[2][2];

    Result[2][0] = A[0][2]*B[0][0] + A[1][2]*B[1][0] + A[2][2]*B[2][0];
    Result[2][1] = A[0][2]*B[0][1] + A[1][2]*B[1][1] + A[2][2]*B[2][1];
    Result[2][2] = A[0][2]*B[0][2] + A[1][2]*B[1][2] + A[2][2]*B[2][2];
#endif
}

void GRANNY
ColumnMatrixMultiply4x3(real32 *IntoMatrix4x4,
                        real32 const *Matrix4x4,
                        real32 const *ByMatrix4x4)
{
    // 6*4 + 3 = 27 adds
    // 9*4 = 36 multiplies
    // 4*4 = 16 stores
    Assert(!StructuresOverlap(*IntoMatrix4x4, *Matrix4x4));
    Assert(!StructuresOverlap(*IntoMatrix4x4, *ByMatrix4x4));

    quad* NOALIAS Result = (quad *)IntoMatrix4x4;
    quad const *A = (quad const *)Matrix4x4;
    quad const *B = (quad const *)ByMatrix4x4;

    // Matrix-multiply the linear component (upper 3x3)
    Result[0][0] = A[0][0]*B[0][0] + A[0][1]*B[1][0] + A[0][2]*B[2][0];
    Result[0][1] = A[0][0]*B[0][1] + A[0][1]*B[1][1] + A[0][2]*B[2][1];
    Result[0][2] = A[0][0]*B[0][2] + A[0][1]*B[1][2] + A[0][2]*B[2][2];

    Result[1][0] = A[1][0]*B[0][0] + A[1][1]*B[1][0] + A[1][2]*B[2][0];
    Result[1][1] = A[1][0]*B[0][1] + A[1][1]*B[1][1] + A[1][2]*B[2][1];
    Result[1][2] = A[1][0]*B[0][2] + A[1][1]*B[1][2] + A[1][2]*B[2][2];

    Result[2][0] = A[2][0]*B[0][0] + A[2][1]*B[1][0] + A[2][2]*B[2][0];
    Result[2][1] = A[2][0]*B[0][1] + A[2][1]*B[1][1] + A[2][2]*B[2][1];
    Result[2][2] = A[2][0]*B[0][2] + A[2][1]*B[1][2] + A[2][2]*B[2][2];

    // Special-case multiply the affine component (position)
    Result[3][0] = (A[3][0]*B[0][0] + A[3][1]*B[1][0] + A[3][2]*B[2][0] +
                    B[3][0]);
    Result[3][1] = (A[3][0]*B[0][1] + A[3][1]*B[1][1] + A[3][2]*B[2][1] +
                    B[3][1]);
    Result[3][2] = (A[3][0]*B[0][2] + A[3][1]*B[1][2] + A[3][2]*B[2][2] +
                    B[3][2]);

    // Ignore the full 4x4-ness of the matrix, since we only care about
    // the 4x3 subsection.
    Result[0][3] = Result[1][3] = Result[2][3] = 0.0f;
    Result[3][3] = 1.0f;
}

void GRANNY
ColumnMatrixMultiply4x3Transpose(real32 *IntoMatrix4x4,
                                 real32 const *Matrix4x4,
                                 real32 const *ByMatrix4x4)
{
    // 6*4 + 3 = 27 adds
    // 9*4 = 36 multiplies
    // 4*3 = 12 stores
    Assert(!StructuresOverlap(*IntoMatrix4x4, *Matrix4x4));
    Assert(!StructuresOverlap(*IntoMatrix4x4, *ByMatrix4x4));

    quad* NOALIAS Result = (quad *)IntoMatrix4x4;
    quad const *A = (quad const *)Matrix4x4;
    quad const *B = (quad const *)ByMatrix4x4;

    // Matrix-multiply the linear component (upper 3x3)
    Result[0][0] = A[0][0]*B[0][0] + A[0][1]*B[1][0] + A[0][2]*B[2][0];
    Result[0][1] = A[1][0]*B[0][0] + A[1][1]*B[1][0] + A[1][2]*B[2][0];
    Result[0][2] = A[2][0]*B[0][0] + A[2][1]*B[1][0] + A[2][2]*B[2][0];
    // Special-case multiply the affine component (position)
    Result[0][3] = (A[3][0]*B[0][0] + A[3][1]*B[1][0] + A[3][2]*B[2][0] +
                    B[3][0]);

    Result[1][0] = A[0][0]*B[0][1] + A[0][1]*B[1][1] + A[0][2]*B[2][1];
    Result[1][1] = A[1][0]*B[0][1] + A[1][1]*B[1][1] + A[1][2]*B[2][1];
    Result[1][2] = A[2][0]*B[0][1] + A[2][1]*B[1][1] + A[2][2]*B[2][1];
    Result[1][3] = (A[3][0]*B[0][1] + A[3][1]*B[1][1] + A[3][2]*B[2][1] +
                    B[3][1]);

    Result[2][0] = A[0][0]*B[0][2] + A[0][1]*B[1][2] + A[0][2]*B[2][2];
    Result[2][1] = A[1][0]*B[0][2] + A[1][1]*B[1][2] + A[1][2]*B[2][2];
    Result[2][2] = A[2][0]*B[0][2] + A[2][1]*B[1][2] + A[2][2]*B[2][2];
    Result[2][3] = (A[3][0]*B[0][2] + A[3][1]*B[1][2] + A[3][2]*B[2][2] +
                    B[3][2]);
}

void GRANNY
ColumnMatrixMultiply4x4(real32 *IntoMatrix4x4,
                        real32 const *Matrix4x4,
                        real32 const *ByMatrix4x4)
{
    Assert(!StructuresOverlap(*IntoMatrix4x4, *Matrix4x4));
    Assert(!StructuresOverlap(*IntoMatrix4x4, *ByMatrix4x4));

    quad* NOALIAS Result = (quad *)IntoMatrix4x4;
    quad const *A = (quad const *)Matrix4x4;
    quad const *B = (quad const *)ByMatrix4x4;

    Result[0][0] = A[0][0]*B[0][0] + A[0][1]*B[1][0] + A[0][2]*B[2][0] + A[0][3]*B[3][0];
    Result[0][1] = A[0][0]*B[0][1] + A[0][1]*B[1][1] + A[0][2]*B[2][1] + A[0][3]*B[3][1];
    Result[0][2] = A[0][0]*B[0][2] + A[0][1]*B[1][2] + A[0][2]*B[2][2] + A[0][3]*B[3][2];
    Result[0][3] = A[0][0]*B[0][3] + A[0][1]*B[1][3] + A[0][2]*B[2][3] + A[0][3]*B[3][3];

    Result[1][0] = A[1][0]*B[0][0] + A[1][1]*B[1][0] + A[1][2]*B[2][0] + A[1][3]*B[3][0];
    Result[1][1] = A[1][0]*B[0][1] + A[1][1]*B[1][1] + A[1][2]*B[2][1] + A[1][3]*B[3][1];
    Result[1][2] = A[1][0]*B[0][2] + A[1][1]*B[1][2] + A[1][2]*B[2][2] + A[1][3]*B[3][2];
    Result[1][3] = A[1][0]*B[0][3] + A[1][1]*B[1][3] + A[1][2]*B[2][3] + A[1][3]*B[3][3];

    Result[2][0] = A[2][0]*B[0][0] + A[2][1]*B[1][0] + A[2][2]*B[2][0] + A[2][3]*B[3][0];
    Result[2][1] = A[2][0]*B[0][1] + A[2][1]*B[1][1] + A[2][2]*B[2][1] + A[2][3]*B[3][1];
    Result[2][2] = A[2][0]*B[0][2] + A[2][1]*B[1][2] + A[2][2]*B[2][2] + A[2][3]*B[3][2];
    Result[2][3] = A[2][0]*B[0][3] + A[2][1]*B[1][3] + A[2][2]*B[2][3] + A[2][3]*B[3][3];

    Result[3][0] = A[3][0]*B[0][0] + A[3][1]*B[1][0] + A[3][2]*B[2][0] + A[3][3]*B[3][0];
    Result[3][1] = A[3][0]*B[0][1] + A[3][1]*B[1][1] + A[3][2]*B[2][1] + A[3][3]*B[3][1];
    Result[3][2] = A[3][0]*B[0][2] + A[3][1]*B[1][2] + A[3][2]*B[2][2] + A[3][3]*B[3][2];
    Result[3][3] = A[3][0]*B[0][3] + A[3][1]*B[1][3] + A[3][2]*B[2][3] + A[3][3]*B[3][3];
}

void GRANNY
QuaternionRotationDifference(real32 *Dest,
                             real32 const *FromOrientation,
                             real32 const *ToOrientation)
{
    Conjugate4(Dest, FromOrientation);
    QuaternionMultiply4(Dest, Dest, ToOrientation);
}

void GRANNY
QuaternionInterpolate4(real32 *Dest,
                       real32 Operand1Coefficient,
                       real32 const *Operand1,
                       real32 Operand2Coefficient,
                       real32 const *Operand2)
{
    // Find the cosine of the angle between the quaternions by
    // taking the inner product
    real32 CosineAngle = InnerProduct4(Operand1, Operand2);
    real32 Sign = 1.0f;
    if(CosineAngle < 0.0f)
    {
        CosineAngle = -CosineAngle;
        Sign = -Sign;
    }

    // TODO: Pick this constant
    real32 const SphericalLinearInterpolationThreshold = 0.0001f;
    if((1.0f - CosineAngle) > SphericalLinearInterpolationThreshold)
    {
        // TODO: Change this to use ATan2.

        // Fit for spherical interpolation
        real32 const Angle = ACos(CosineAngle);
        real32 const OneOverSinAngle = 1.0f / Sin(Angle);

        Operand1Coefficient =
            Sin(Operand1Coefficient * Angle) * Sign * OneOverSinAngle;
        Operand2Coefficient =
            Sin(Operand2Coefficient * Angle) * OneOverSinAngle;
    }
    else
    {
        Operand1Coefficient *= Sign;
    }

    ScaleVectorPlusScaleVector4(Dest,
                                Operand1Coefficient, Operand1,
                                Operand2Coefficient, Operand2);
}

void GRANNY
MatrixEqualsQuaternion3x3(real32 *Dest, real32 const *Operand)
{
    // 3 multiplies
    // Squares (for the diagonal)
    real32 const XSquared = Operand[0] * Operand[0];
    real32 const YSquared = Operand[1] * Operand[1];
    real32 const ZSquared = Operand[2] * Operand[2];

    // 6 multiplies
    // Combinations (for the non-diagonal)
    real32 const XY = Operand[0] * Operand[1];
    real32 const XZ = Operand[0] * Operand[2];
    real32 const YZ = Operand[1] * Operand[2];
    real32 const WX = Operand[3] * Operand[0];
    real32 const WY = Operand[3] * Operand[1];
    real32 const WZ = Operand[3] * Operand[2];

    // 9 multiplies, 12 adds
    // Top row
    Dest[0] = 1 - 2*(YSquared + ZSquared);
    Dest[1] = 2*(XY - WZ);
    Dest[2] = 2*(XZ + WY);

    // Middle row
    Dest[3] = 2*(XY + WZ);
    Dest[4] = 1 - 2*(XSquared + ZSquared);
    Dest[5] = 2*(YZ - WX);

    // Bottom row
    Dest[6] = 2*(XZ - WY);
    Dest[7] = 2*(YZ + WX);
    Dest[8] = 1 - 2*(XSquared + YSquared);

    // Total: 18 multiplies, 12 adds
}

static void
QuaternionDecomposeFill3x3(real32 *Quaternion,
                           triple const *Matrix,
                           int32x const X)
{
    int32x const Y = (X + 1) % 3;
    int32x const Z = (X + 2) % 3;

    real32 const S =
        SquareRoot((Matrix[X][X] - (Matrix[Y][Y] + Matrix[Z][Z])) + 1.0f);
    Quaternion[X] = S * 0.5f;
    real32 const InverseS = 0.5f / S;
    Quaternion[Y] = (Matrix[X][Y] + Matrix[Y][X]) * InverseS;
    Quaternion[Z] = (Matrix[Z][X] + Matrix[X][Z]) * InverseS;
    Quaternion[3] = (Matrix[Z][Y] - Matrix[Y][Z]) * InverseS;
}

void GRANNY
QuaternionEqualsMatrix3x3(real32 *Quaternion,
                          real32 const *MatrixInit)
{
    triple const *Matrix = (triple const *)MatrixInit;

    real32 const Trace = MatrixTrace3x3((real32 const *)Matrix);
    if(Trace > 0.0f)
    {
        real32 const S = SquareRoot(Trace + 1.0f);
        Quaternion[3] = S * 0.5f;
        real32 const InverseS = 0.5f / S;
        Quaternion[0] = (Matrix[2][1] - Matrix[1][2]) * InverseS;
        Quaternion[1] = (Matrix[0][2] - Matrix[2][0]) * InverseS;
        Quaternion[2] = (Matrix[1][0] - Matrix[0][1]) * InverseS;
    }
    else
    {
        int32x MaximumIndex = 0;
        if(Matrix[1][1] > Matrix[MaximumIndex][MaximumIndex])
        {
            MaximumIndex = 1;
        }
        if(Matrix[2][2] > Matrix[MaximumIndex][MaximumIndex])
        {
            MaximumIndex = 2;
        }
        QuaternionDecomposeFill3x3(Quaternion, Matrix, MaximumIndex);
    }
}

void GRANNY
QuaternionMultiply4(real32 *Dest, real32 const *Operand1,
                    real32 const *Operand2)
{
    real32 const w1 = Operand1[3];
    real32 const w2 = Operand2[3];

    real32 Cross[3];
    VectorCrossProduct3(Cross, Operand1, Operand2);
    ScaleVectorAdd3(Cross, w1, Operand2);
    ScaleVectorAdd3(Cross, w2, Operand1);

    real32 Dot = InnerProduct3(Operand1, Operand2);

    Dest[0] = Cross[0];
    Dest[1] = Cross[1];
    Dest[2] = Cross[2];
    Dest[3] = w1*w2 - Dot;
}

void GRANNY
NormalQuaternionTransform3(real32 *Vector3, real32 const *Quaternion4)
{
    real32 const aa = Quaternion4[0]*Quaternion4[0];
    real32 const bb = Quaternion4[1]*Quaternion4[1];
    real32 const cc = Quaternion4[2]*Quaternion4[2];

    real32 const ab = Quaternion4[0]*Quaternion4[1];
    real32 const ac = Quaternion4[0]*Quaternion4[2];
    real32 const ad = Quaternion4[0]*Quaternion4[3];
    real32 const bc = Quaternion4[1]*Quaternion4[2];
    real32 const bd = Quaternion4[1]*Quaternion4[3];
    real32 const cd = Quaternion4[2]*Quaternion4[3];

    real32 const x = Vector3[0];
    real32 const y = Vector3[1];
    real32 const z = Vector3[2];

    real32 const x2 = 2.0f * x;
    real32 const y2 = 2.0f * y;
    real32 const z2 = 2.0f * z;

    // TODO: Analyze this for optimality

    Vector3[0] = x*(1.0f - 2.0f*(bb + cc)) + y2*(ab - cd) + z2*(ac + bd);
    Vector3[1] = x2*(ab + cd) + y*(1.0f - 2.0f*(aa + cc)) + z2*(bc - ad);
    Vector3[2] = x2*(ac - bd) + y2*(bc + ad) + z*(1.0f - 2.0f*(aa + bb));
}

void GRANNY
QuaternionDifferenceToAngularVelocity(real32 *Result3,
                                      real32 const *From4, real32 const *To4)
{
    // TODO: This routine could be made more optimal
    quad Diff;
    QuaternionRotationDifference(Diff, From4, To4);
    real32 Sin_2 = VectorLength3(Diff);
    real32 Cos_2 = Diff[3];

    real32 Angle = 2.0f * ATan2(Sin_2, Cos_2);
    if(Sin_2 != 0)
    {
        Angle /= Sin_2;
    }

    VectorScale3(Result3, Angle, Diff);
}

void GRANNY
EnsureQuaternionContinuity(int32x QuaternionCount, real32 *Quaternions)
{
    real32 LastX = 0;
    real32 LastY = 0;
    real32 LastZ = 0;
    real32 LastW = 0;
    while(QuaternionCount--)
    {
        real32 X = Quaternions[0];
        real32 Y = Quaternions[1];
        real32 Z = Quaternions[2];
        real32 W = Quaternions[3];
        if((LastX*X + LastY*Y + LastZ*Z + LastW*W) < 0.0f)
        {
            X = -X;
            Y = -Y;
            Z = -Z;
            W = -W;
        }
        Quaternions[0] = LastX = X;
        Quaternions[1] = LastY = Y;
        Quaternions[2] = LastZ = Z;
        Quaternions[3] = LastW = W;

        Quaternions += 4;
    }
}

bool GRANNY
PolarDecompose(real32 const *Source3x3Init, real32 Tolerance,
               real32 *QInit, real32 *SInit)
{
    triple const ZMirror[] = {{1,  0,  0},
                              {0,  1,  0},
                              {0,  0, -1}};

    triple const *Source3x3 = (triple const *)Source3x3Init;
    triple *Q = (triple *)QInit;
    triple *S = (triple *)SInit;

    // TODO: This is a simple polar decomposer that I hacked together,
    // but in reality I could probably do much better if I spent
    // some more time here.

    bool Result = false;

    real32 const Det = MatrixDeterminant3x3((real32 const *)Source3x3);
    if(Det >= 0.0f)
    {
        // The original matrix is not mirrored, so I can use
        // it directly.
        MatrixEquals3x3((real32 *)Q, (real32 *)Source3x3);
    }
    else
    {
        // The original matrix is mirrored, so before decomposing,
        // I "un-mirror" it by multiplying by a Z mirror.  After
        // the decomposition, S is generated from the resultant
        // Q and the original matrix, so it will have the ZMirror
        // back in there which makes everything work out.
        MatrixMultiply3x3((real32 *)Q, (real32 const *)Source3x3,
                          (real32 const *)ZMirror);
    }

    // Compute Q from repeated inversion steps
    int32x const MaxIterations = 1000;
    {for(int32x Iteration = 0;
         Iteration < MaxIterations;
         ++Iteration)
    {
        // We use S as a scratch matrix here, since it has nothing
        // in it until after the loop is complete.
        MatrixInvert3x3((real32 *)S, (real32 *)Q);
        MatrixTranspose3x3(S);
        MatrixAdd3x3((real32 *)S, (real32 *)Q);
        MatrixScale3x3((real32 *)S, 0.5f);

        // S now contains the next step, so we'll diff it with Q,
        // which is still the results from the previous step
        real32 OneNorm = 0.0f;
        OneNorm += AbsoluteValue(S[0][0] - Q[0][0]);
        OneNorm += AbsoluteValue(S[0][1] - Q[0][1]);
        OneNorm += AbsoluteValue(S[0][2] - Q[0][2]);
        OneNorm += AbsoluteValue(S[1][0] - Q[1][0]);
        OneNorm += AbsoluteValue(S[1][1] - Q[1][1]);
        OneNorm += AbsoluteValue(S[1][2] - Q[1][2]);
        OneNorm += AbsoluteValue(S[2][0] - Q[2][0]);
        OneNorm += AbsoluteValue(S[2][1] - Q[2][1]);
        OneNorm += AbsoluteValue(S[2][2] - Q[2][2]);

        // Now that we have the one-norm of the difference, we can
        // actually take the step by writing the result back into Q
        MatrixEquals3x3((real32 *)Q, (real32 *)S);

        // If we made our tolerance, we're done.
        if(OneNorm < Tolerance)
        {
            Result = true;
            break;
        }
    }}

    // S is whatever's left over
    TransposeMatrixMultiply3x3((real32 *)S, (real32 *)Q, (real32 *)Source3x3);

    return(Result);
}

void GRANNY
InPlaceSimilarityTransformPosition(real32 const *Affine3,
                                   real32 const *Linear3x3,
                                   real32 *Position3)
{
    VectorTransform3(Position3, Linear3x3);
    VectorAdd3(Position3, Affine3);
}

void GRANNY
InPlaceSimilarityTransformOrientation(real32 const *Linear3x3,
                                      real32 const *InverseLinear3x3,
                                      real32 *Orientation4)
{
    real32 OriginalLength = VectorLength4(Orientation4);
    VectorScale4(Orientation4, 1.0f / OriginalLength);

    real32 OrientationM[9];
    MatrixEqualsQuaternion3x3(OrientationM, Orientation4);

    real32 Temp[9];
    MatrixMultiply3x3(Temp, Linear3x3, OrientationM);
    MatrixMultiply3x3(OrientationM, Temp, InverseLinear3x3);

    QuaternionEqualsMatrix3x3(Orientation4, OrientationM);

    VectorScale4(Orientation4, OriginalLength);
}

void GRANNY
InPlaceSimilarityTransformScaleShear(real32 const *Linear3x3,
                                     real32 const *InverseLinear3x3,
                                     real32 *ScaleShear3x3)
{
    real32 Temp[9];
    MatrixMultiply3x3(Temp, Linear3x3, ScaleShear3x3);
    MatrixMultiply3x3(ScaleShear3x3, Temp, InverseLinear3x3);
}

void GRANNY
InPlaceSimilarityTransform(real32 const *Affine3,
                           real32 const *Linear3x3,
                           real32 const *InverseLinear3x3,
                           real32 *Position3,
                           real32 *Orientation4,
                           real32 *ScaleShear3x3)
{
    InPlaceSimilarityTransformPosition(
        Affine3, Linear3x3, Position3);
    InPlaceSimilarityTransformOrientation(
        Linear3x3, InverseLinear3x3, Orientation4);
    InPlaceSimilarityTransformScaleShear(
        Linear3x3, InverseLinear3x3, ScaleShear3x3);
}

void GRANNY
InPlaceSimilarityTransform4x3(real32 const *Affine3,
                              real32 const *Linear3x3,
                              real32 const *InverseLinear3x3,
                              real32 *ResultInit)
{
    quad *Result = (quad *)ResultInit;
    triple Upper3x3[3];

    Upper3x3[0][0] = Result[0][0];
    Upper3x3[0][1] = Result[1][0];
    Upper3x3[0][2] = Result[2][0];
    Upper3x3[1][0] = Result[0][1];
    Upper3x3[1][1] = Result[1][1];
    Upper3x3[1][2] = Result[2][1];
    Upper3x3[2][0] = Result[0][2];
    Upper3x3[2][1] = Result[1][2];
    Upper3x3[2][2] = Result[2][2];

    real32 Ignored[4] = {0, 0, 0, 1};
    InPlaceSimilarityTransform(Affine3, Linear3x3, InverseLinear3x3,
                               (real32 *)Result[3], Ignored,
                               (real32 *)Upper3x3);

    Result[0][0] = Upper3x3[0][0];
    Result[1][0] = Upper3x3[0][1];
    Result[2][0] = Upper3x3[0][2];
    Result[0][1] = Upper3x3[1][0];
    Result[1][1] = Upper3x3[1][1];
    Result[2][1] = Upper3x3[1][2];
    Result[0][2] = Upper3x3[2][0];
    Result[1][2] = Upper3x3[2][1];
    Result[2][2] = Upper3x3[2][2];
}

void GRANNY
DextralBases(real32 const *XAxis, real32 *YAxis, real32 *ZAxis)
{
    real32 CrossVector[3] = {1.0f, 1.0f, 1.0f};

    real32 MaximumElement = 0.0f;

    int32x MaximumElementIndex = 0;
    {for(int32x ElementIndex = 0;
         ElementIndex < 3;
         ++ElementIndex)
    {
        real32 ElementValue = AbsoluteValue(XAxis[ElementIndex]);
        if(ElementValue > MaximumElement)
        {
            MaximumElement = ElementValue;
            MaximumElementIndex = ElementIndex;
        }
    }}

    CrossVector[MaximumElementIndex] = 0.0f;

    VectorCrossProduct3(YAxis, CrossVector, XAxis);
    Normalize3(YAxis);

    VectorCrossProduct3(ZAxis, XAxis, YAxis);
    Normalize3(ZAxis);
}

void GRANNY
SinistralBases(real32 const *XAxis, real32 *YAxis, real32 *ZAxis)
{
    // TODO: I admit that this function only exists because the
    // word Sinistral is so cool.  I don't think anyone's ever
    // going to actually call it.
    DextralBases(XAxis, YAxis, ZAxis);
    VectorNegate3(YAxis);
}

void GRANNY
DistanceToBoxSquared(real32 *MinCorner, real32 *MaxCorner,
                     real32 *Point,
                     real32 &MinDistanceSquared,
                     real32 &MaxDistanceSquared)
{
    MinDistanceSquared = 0.0f;
    MaxDistanceSquared = 0.0f;
    {for(int32x Element = 0;
         Element < 3;
         ++Element)
    {
        // Calculate the distance to the minimum corner of the prism
        real32 MinCornerDistanceSquared =
            (Point[Element] - MinCorner[Element]);
        MinCornerDistanceSquared *= MinCornerDistanceSquared;

        // Calculate the distance to the maximum corner of the prism
        real32 MaxCornerDistanceSquared =
            (Point[Element] - MaxCorner[Element]);
        MaxCornerDistanceSquared *= MaxCornerDistanceSquared;

        // See how far from the prism we are in this dimension
        if(Point[Element] < MinCorner[Element])
        {
            MinDistanceSquared += MinCornerDistanceSquared;
        }
        else if(Point[Element] > MaxCorner[Element])
        {
            MinDistanceSquared += MaxCornerDistanceSquared;
        }

        // See how far we are from the prism's far corner in this dimension
        if(MinCornerDistanceSquared > MaxCornerDistanceSquared)
        {
            MaxDistanceSquared += MinCornerDistanceSquared;
        }
        else
        {
            MaxDistanceSquared += MaxCornerDistanceSquared;
        }
    }}
}
