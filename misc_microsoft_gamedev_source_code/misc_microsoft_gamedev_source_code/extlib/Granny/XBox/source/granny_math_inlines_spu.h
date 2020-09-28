#if !defined(GRANNY_MATH_INLINES_SPU_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_math_inlines_spu.h $
// $DateTime: 2007/09/06 10:05:47 $
// $Change: 15908 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_INTRINSICS_H)
#include "granny_intrinsics.h"
#endif

#if !PROCESSOR_CELL_SPU
#error "Only valid for SPU"
#endif

#include <vectormath/cpp/vectormath_aos.h>
#include <spu_intrinsics_gcc.h>

BEGIN_GRANNY_NAMESPACE;

//
// Scalar operations
//
inline real32 Square(real32 const Value)
{
    return(Value*Value);
}

inline real32 SquareRoot(real32 const Value)
{
    return((real32)IntrinsicSquareRoot(Value));
}

inline real32 ACos(real32 const Value)
{
    return((real32)IntrinsicACos(Value));
}

#if !GRANNY_NO_SINCOS
inline real32 Sin(real32 const Value)
{
    return((real32)IntrinsicSin(Value));
}

inline real32 Cos(real32 const Value)
{
    return((real32)IntrinsicCos(Value));
}
#endif

inline real32 Tan(real32 const Value)
{
    return((real32)(IntrinsicSin(Value)/IntrinsicCos(Value)));
}

inline real32 ATan2(real32 const SinAngle, real32 const CosAngle)
{
    return((real32)IntrinsicATan2(SinAngle, CosAngle));
}

inline real32 Pow(real32 const X, real32 const Y)
{
    return((real32)IntrinsicPow(X, Y));
}

inline void Swap(real32 &A, real32 &B)
{
    real32 const Temp = A;
    A = B;
    B = Temp;
}

inline real32
Modulus(real32 const Dividend, real32 const Divisor)
{
    return(IntrinsicModulus(Dividend, Divisor));
}

inline real32
Maximum(real32 const A, real32 const B)
{
    return(A > B ? A : B);
}

inline real32
Minimum(real32 const A, real32 const B)
{
    return(A < B ? A : B);
}

inline real32
IsInRange(real32 const Min, real32 const Value, real32 const Max)
{
    return((Value >= Min) && (Value <= Max));
}

inline real64x
AbsoluteValue64(real64x const A)
{
    return(fabs(A));
}

inline real32
AbsoluteValue(real32 const A)
{
    return((real32)fabs(A));
}

inline int32
Ceiling(real32 const A)
{
    return(CeilingReal32ToInt32(A));
}

inline int32
Floor(real32 const A)
{
    return(FloorReal32ToInt32(A));
}

inline real32
Clamp(real32 Min, real32 Value, real32 Max)
{
    if(Value < Min)
    {
        Value = Min;
    }
    else if(Value > Max)
    {
        Value = Max;
    }

    return(Value);
}

inline real32
Sign(real32 Value)
{
    return((Value < 0.0f) ? -1.0f : 1.0f);
}

inline bool
AreEqual(int32x Dimension, real32 const *A, real32 const *B, real32 Epsilon)
{
    // TODO: is this a good test?  Or should I be just doing single-value
    // based epsilons instead of sums?

    real32 Difference = 0.0f;
    while(Dimension--)
    {
        Difference += AbsoluteValue(*A++ - *B++);
    }

    return(Difference < Epsilon);
}

//
// Vector/scalar operations
//
inline void VectorScale3(real32 *Dest, real32 const Scalar)
{
    Dest[0] *= Scalar;
    Dest[1] *= Scalar;
    Dest[2] *= Scalar;
}

inline void VectorScale3(real32 *Dest, real32 const Scalar,
                         real32 const *Source)
{
    Dest[0] = Source[0] * Scalar;
    Dest[1] = Source[1] * Scalar;
    Dest[2] = Source[2] * Scalar;
}

inline void VectorScale4(real32 *Dest, real32 const Scalar)
{
    Dest[0] *= Scalar;
    Dest[1] *= Scalar;
    Dest[2] *= Scalar;
    Dest[3] *= Scalar;
}

inline void VectorScale4(real32 *Dest, real32 const Scalar,
                         real32 const *Source)
{
    Dest[0] = Source[0] * Scalar;
    Dest[1] = Source[1] * Scalar;
    Dest[2] = Source[2] * Scalar;
    Dest[3] = Source[3] * Scalar;
}

inline void VectorScaleN(real32 *Dest, real32 const Scalar, int32x const Dimension)
{
    {for(int32x Dim = 0; Dim < Dimension; ++Dim)
    {
        Dest[Dim] *= Scalar;
    }}
}

inline void VectorScale3(real32 *Dest, real32 const Scalar,
                         real32 const *Source, int32x const Dimension)
{
    {for(int32x Dim = 0; Dim < Dimension; ++Dim)
    {
        Dest[Dim] = Source[Dim] * Scalar;
    }}
}

//
// Vector operations
//
inline void VectorZero3(real32 *Dest)
{
    Dest[0] = Dest[1] = Dest[2] = 0.0f;
}

inline void VectorZero4(real32 *Dest)
{
    Dest[0] = Dest[1] = Dest[2] = Dest[3] = 0.0f;
}


inline real32 VectorLengthSquared3(real32 const *Source)
{
    return(Source[0]*Source[0] +
           Source[1]*Source[1] +
           Source[2]*Source[2]);
}

inline real32 VectorLengthSquared4(real32 const *Source)
{
    return(Source[0]*Source[0] +
           Source[1]*Source[1] +
           Source[2]*Source[2] +
           Source[3]*Source[3]);
}

inline real32 VectorLengthSquaredN(real32 const *Source, int32x const Dimension)
{
    real32 Sum = 0;
    {for(int32x Dim = 0; Dim < Dimension; ++Dim)
    {
        Sum += Source[Dim] * Source[Dim];
    }}

    return Sum;
}

inline real32 VectorLength3(real32 const *Source)
{
    return(SquareRoot(VectorLengthSquared3(Source)));
}

inline real32 VectorLength4(real32 const *Source)
{
    return(SquareRoot(VectorLengthSquared4(Source)));
}

inline real32 VectorLengthN(real32 const *Source, int32x const Dimension)
{
    return(SquareRoot(VectorLengthSquaredN(Source, Dimension)));
}


void VectorIdentityQuaternion(real32 *Dest)
{
    Dest[0] = Dest[1] = Dest[2] = 0.0f;
    Dest[3] = 1.0f;
}

inline void VectorSet2(real32 *Dest, real32 const X, real32 const Y)
{
    Dest[0] = X;
    Dest[1] = Y;
}

inline void VectorSet3(real32 *Dest,
                       real32 const X, real32 const Y, real32 const Z)
{
    Dest[0] = X;
    Dest[1] = Y;
    Dest[2] = Z;
}

inline void VectorSet4(real32 *Dest,
                       real32 const X, real32 const Y,
                       real32 const Z, real32 const W)
{
    Dest[0] = X;
    Dest[1] = Y;
    Dest[2] = Z;
    Dest[3] = W;
}

inline void VectorNegate3(real32 *Dest)
{
    Dest[0] = -Dest[0];
    Dest[1] = -Dest[1];
    Dest[2] = -Dest[2];
}

inline void VectorNegate3(real32 *Dest, real32 const *Source)
{
    Dest[0] = -Source[0];
    Dest[1] = -Source[1];
    Dest[2] = -Source[2];
}

inline void VectorNegate4(real32 *Dest)
{
    Dest[0] = -Dest[0];
    Dest[1] = -Dest[1];
    Dest[2] = -Dest[2];
    Dest[3] = -Dest[3];
}

inline void VectorNegate4(real32 *Dest, real32 const *Source)
{
    Dest[0] = -Source[0];
    Dest[1] = -Source[1];
    Dest[2] = -Source[2];
    Dest[3] = -Source[3];
}

inline real32 NormalizeOrZero3(real32 *Dest)
{
    real32 Length = VectorLength3(Dest);
    if(Length > 0.00001f)
    {
        VectorScale3(Dest, 1.0f / Length);
    }
    else
    {
        VectorZero3(Dest);
        Length = 0.0f;
    }

    return(Length);
}

inline real32 NormalizeOrZero4(real32 *Dest)
{
    real32 Length = VectorLength4(Dest);
    if(Length > 0.00001f)
    {
        VectorScale4(Dest, 1.0f / Length);
    }
    else
    {
        VectorZero4(Dest);
        Length = 0.0f;
    }

    return(Length);
}

inline void NormalizeCloseToOne3(real32 *Dest)
{
    real32 const Sum = (Dest[0] * Dest[0] +
                        Dest[1] * Dest[1] +
                        Dest[2] * Dest[2]);
    real32 const ApproximateOneOverRoot = (3.0f - Sum) * 0.5f;
    Dest[0] *= ApproximateOneOverRoot;
    Dest[1] *= ApproximateOneOverRoot;
    Dest[2] *= ApproximateOneOverRoot;
}

inline void NormalizeCloseToOne4(real32 *Dest)
{
    real32 const Sum = (Dest[0] * Dest[0] +
                        Dest[1] * Dest[1] +
                        Dest[2] * Dest[2] +
                        Dest[3] * Dest[3]);
    real32 const ApproximateOneOverRoot = (3.0f - Sum) * 0.5f;
    Dest[0] *= ApproximateOneOverRoot;
    Dest[1] *= ApproximateOneOverRoot;
    Dest[2] *= ApproximateOneOverRoot;
    Dest[3] *= ApproximateOneOverRoot;
}

inline real32 NormalizeN(real32 *Dest, int32x const Dimension)
{
    real32 Length = VectorLengthN(Dest, Dimension);
    VectorScaleN(Dest, 1.0f / Length, Dimension);

    return(Length);
}

inline void Conjugate4(real32 *Dest)
{
    Dest[0] = -Dest[0];
    Dest[1] = -Dest[1];
    Dest[2] = -Dest[2];

    // Subtlety: we don't need to touch the last element, since it remains
    // the same during conjugation (only the vector component of the quaternion
    // is negated)
}

inline void Conjugate4(real32 *Dest, real32 const *Source)
{
    Dest[0] = -Source[0];
    Dest[1] = -Source[1];
    Dest[2] = -Source[2];
    Dest[3] = Source[3];
}

inline void ConstructQuaternion4(real32 *Quaternion,
                                 real32 const *Axis,
                                 real32 const Angle)
{
    real32 const HalfSin = Sin(Angle * 0.5f);
    real32 const HalfCos = Cos(Angle * 0.5f);
    Quaternion[0] = Axis[0] * HalfSin;
    Quaternion[1] = Axis[1] * HalfSin;
    Quaternion[2] = Axis[2] * HalfSin;
    Quaternion[3] = HalfCos;
}

inline void ElementMinimum3(real32 *Result, real32 *Compare)
{
    Result[0] = Minimum(Result[0], Compare[0]);
    Result[1] = Minimum(Result[1], Compare[1]);
    Result[2] = Minimum(Result[2], Compare[2]);
}

inline void ElementMaximum3(real32 *Result, real32 *Compare)
{
    Result[0] = Maximum(Result[0], Compare[0]);
    Result[1] = Maximum(Result[1], Compare[1]);
    Result[2] = Maximum(Result[2], Compare[2]);
}

//
// Vector/vector operations
//
inline void VectorEquals3(real32 *Dest, real32 const *Source)
{
    Dest[0] = Source[0];
    Dest[1] = Source[1];
    Dest[2] = Source[2];
}

inline void VectorEquals4(real32 *Dest, real32 const *Source)
{
    Dest[0] = Source[0];
    Dest[1] = Source[1];
    Dest[2] = Source[2];
    Dest[3] = Source[3];
}

inline void VectorAdd3(real32 *Dest, real32 const *Addend)
{
    Dest[0] += Addend[0];
    Dest[1] += Addend[1];
    Dest[2] += Addend[2];
}

inline void VectorAdd4(real32 *Dest, real32 const *Addend)
{
    Dest[0] += Addend[0];
    Dest[1] += Addend[1];
    Dest[2] += Addend[2];
    Dest[3] += Addend[3];
}

inline void VectorAdd3(real32 *Dest, real32 const *Addend0,
                       real32 const *Addend1)
{
    Dest[0] = Addend0[0] + Addend1[0];
    Dest[1] = Addend0[1] + Addend1[1];
    Dest[2] = Addend0[2] + Addend1[2];
}

inline void VectorAdd4(real32 *Dest, real32 const *Addend0,
                       real32 const *Addend1)
{
    Dest[0] = Addend0[0] + Addend1[0];
    Dest[1] = Addend0[1] + Addend1[1];
    Dest[2] = Addend0[2] + Addend1[2];
    Dest[3] = Addend0[3] + Addend1[3];
}

inline void VectorSubtract3(real32 *Dest, real32 const *Addend)
{
    Dest[0] -= Addend[0];
    Dest[1] -= Addend[1];
    Dest[2] -= Addend[2];
}

inline void VectorSubtract3(real32 *Dest, real32 const *Vector0,
                            real32 const *Vector1)
{
    Dest[0] = Vector0[0] - Vector1[0];
    Dest[1] = Vector0[1] - Vector1[1];
    Dest[2] = Vector0[2] - Vector1[2];
}

inline void VectorSubtract4(real32 *Dest, real32 const *Addend)
{
    Dest[0] -= Addend[0];
    Dest[1] -= Addend[1];
    Dest[2] -= Addend[2];
    Dest[3] -= Addend[3];
}

inline void VectorSubtract4(real32 *Dest, real32 const *Vector0,
                            real32 const *Vector1)
{
    Dest[0] = Vector0[0] - Vector1[0];
    Dest[1] = Vector0[1] - Vector1[1];
    Dest[2] = Vector0[2] - Vector1[2];
    Dest[3] = Vector0[3] - Vector1[3];
}

inline real32 InnerProduct3(real32 const *V0, real32 const *V1)
{
    return(V0[0] * V1[0] +
           V0[1] * V1[1] +
           V0[2] * V1[2]);
}

inline real32 InnerProduct4(real32 const *V0, real32 const *V1)
{
    return(V0[0] * V1[0] +
           V0[1] * V1[1] +
           V0[2] * V1[2] +
           V0[3] * V1[3]);
}

inline real32 InnerProductN(int32x Dimension,
                            real32 const *V0, real32 const *V1)
{
    real32 Result = 0.0f;

    while(Dimension--)
    {
        Result += *V0++ * *V1++;
    }

    return(Result);
}

inline void VectorCrossProduct3(real32 *Dest, real32 const *Operand1,
                                real32 const *Operand2)
{
    real32 const X = Operand1[1]*Operand2[2] - Operand1[2]*Operand2[1];
    real32 const Y = Operand1[2]*Operand2[0] - Operand1[0]*Operand2[2];
    real32 const Z = Operand1[0]*Operand2[1] - Operand1[1]*Operand2[0];

    Dest[0] = X;
    Dest[1] = Y;
    Dest[2] = Z;
}

inline void TriangleCrossProduct3(real32 *Dest,
                                  real32 const *V0,
                                  real32 const *V1,
                                  real32 const *V2)
{
    triple E01, E02;
    VectorSubtract3(E01, V1, V0);
    VectorSubtract3(E02, V2, V0);

    VectorCrossProduct3(Dest, E01, E02);
}

inline void TriangleCrossProduct3(real32 *Dest,
                                  real32 *E01,
                                  real32 *E02,
                                  real32 const *V0,
                                  real32 const *V1,
                                  real32 const *V2)
{
    VectorSubtract3(E01, V1, V0);
    VectorSubtract3(E02, V2, V0);

    VectorCrossProduct3(Dest, E01, E02);
}

inline void VectorHadamardProduct3(real32 *Dest, real32 const *Source)
{
    Dest[0] *= Source[0];
    Dest[1] *= Source[1];
    Dest[2] *= Source[2];
}

inline void VectorHadamardAccumulate3(real32 *Dest, real32 const *V0, real32 const *V1)
{
    Dest[0] += V0[0] * V1[0];
    Dest[1] += V0[1] * V1[1];
    Dest[2] += V0[2] * V1[2];
}

//
// Matrix operations
//
inline void
MatrixZero3x3(real32 *Dest)
{
    Dest[0] = 0.0f;
    Dest[1] = 0.0f;
    Dest[2] = 0.0f;
    Dest[3] = 0.0f;
    Dest[4] = 0.0f;
    Dest[5] = 0.0f;
    Dest[6] = 0.0f;
    Dest[7] = 0.0f;
    Dest[8] = 0.0f;
}

inline void
MatrixZero4x4(real32 *Dest)
{
    Dest[0] = 0.0f;
    Dest[1] = 0.0f;
    Dest[2] = 0.0f;
    Dest[3] = 0.0f;
    Dest[4] = 0.0f;
    Dest[5] = 0.0f;
    Dest[6] = 0.0f;
    Dest[7] = 0.0f;
    Dest[8] = 0.0f;
    Dest[9] = 0.0f;
    Dest[10] = 0.0f;
    Dest[11] = 0.0f;
    Dest[12] = 0.0f;
    Dest[13] = 0.0f;
    Dest[14] = 0.0f;
    Dest[15] = 0.0f;
}

inline void
MatrixRows3x3(triple      *Dest,
              float const *Row0,
              float const *Row1,
              float const *Row2)
{
    Dest[0][0] = Row0[0];
    Dest[0][1] = Row0[1];
    Dest[0][2] = Row0[2];
    Dest[1][0] = Row1[0];
    Dest[1][1] = Row1[1];
    Dest[1][2] = Row1[2];
    Dest[2][0] = Row2[0];
    Dest[2][1] = Row2[1];
    Dest[2][2] = Row2[2];
}

inline void
MatrixColumns3x3(triple *Dest,
                 float const* Column0,
                 float const* Column1,
                 float const* Column2)
{
    Dest[0][0] = Column0[0];
    Dest[1][0] = Column0[1];
    Dest[2][0] = Column0[2];
    Dest[0][1] = Column1[0];
    Dest[1][1] = Column1[1];
    Dest[2][1] = Column1[2];
    Dest[0][2] = Column2[0];
    Dest[1][2] = Column2[1];
    Dest[2][2] = Column2[2];
}

inline void
TransformMatrix4x4(quad *Dest, real32 const *Column0,
                   real32 const *Column1, real32 const *Column2,
                   real32 const *Row3)
{
    Dest[0][0] = Column0[0];
    Dest[1][0] = Column0[1];
    Dest[2][0] = Column0[2];
    Dest[3][0] = Row3[0];
    Dest[0][1] = Column1[0];
    Dest[1][1] = Column1[1];
    Dest[2][1] = Column1[2];
    Dest[3][1] = Row3[1];
    Dest[0][2] = Column2[0];
    Dest[1][2] = Column2[1];
    Dest[2][2] = Column2[2];
    Dest[3][2] = Row3[2];
    Dest[0][3] = 0;
    Dest[1][3] = 0;
    Dest[2][3] = 0;
    Dest[3][3] = 1;
}

inline void
AxisRotationColumns3x3(triple *Result, triple const Axis,
                       real32 const AngleInRadians)
{
    real32 const AngleSin = Sin(AngleInRadians);
    real32 const AngleCos = Cos(AngleInRadians);

    real32 const Vx = Axis[0];
    real32 const Vx2 = Vx*Vx;

    real32 const Vy = Axis[1];
    real32 const Vy2 = Vy*Vy;

    real32 const Vz = Axis[2];
    real32 const Vz2 = Vz*Vz;

    Result[0][0] = Vx2 + AngleCos * (1 - Vx2);
    Result[0][1] = Vx * Vy * (1 - AngleCos) - Vz * AngleSin;
    Result[0][2] = Vz * Vx * (1 - AngleCos) + Vy * AngleSin;

    Result[1][0] = Vx * Vy * (1 - AngleCos) + Vz * AngleSin;
    Result[1][1] = Vy2 + AngleCos * (1 - Vy2);
    Result[1][2] = Vy * Vz * (1 - AngleCos) - Vx * AngleSin;

    Result[2][0] = Vz * Vx * (1 - AngleCos) - Vy * AngleSin;
    Result[2][1] = Vy * Vz * (1 - AngleCos) + Vx * AngleSin;
    Result[2][2] = Vz2 + AngleCos * (1 - Vz2);
}

inline void
XRotationColumns3x3(triple *Result, real32 const AngleInRadians)
{
    real32 const AngleSin = Sin(AngleInRadians);
    real32 const AngleCos = Cos(AngleInRadians);

    Result[0][0] = 1.0f;
    Result[0][1] = 0.0f;
    Result[0][2] = 0.0f;
    Result[1][0] = 0.0f;
    Result[1][1] = AngleCos;
    Result[1][2] = -AngleSin;
    Result[2][0] = 0.0f;
    Result[2][1] = AngleSin;
    Result[2][2] = AngleCos;
}

inline void
YRotationColumns3x3(triple *Result, real32 const AngleInRadians)
{
    real32 const AngleSin = Sin(AngleInRadians);
    real32 const AngleCos = Cos(AngleInRadians);

    Result[0][0] = AngleCos;
    Result[0][1] = 0.0f;
    Result[0][2] = AngleSin;
    Result[1][0] = 0.0f;
    Result[1][1] = 1.0f;
    Result[1][2] = 0.0f;
    Result[2][0] = -AngleSin;
    Result[2][1] = 0.0f;
    Result[2][2] = AngleCos;
}

inline void
ZRotationColumns3x3(triple *Result, real32 const AngleInRadians)
{
    real32 const AngleSin = Sin(AngleInRadians);
    real32 const AngleCos = Cos(AngleInRadians);

    Result[0][0] = AngleCos;
    Result[0][1] = -AngleSin;
    Result[0][2] = 0.0f;
    Result[1][0] = AngleSin;
    Result[1][1] = AngleCos;
    Result[1][2] = 0.0f;
    Result[2][0] = 0.0f;
    Result[2][1] = 0.0f;
    Result[2][2] = 1.0f;
}

inline void NegateMatrix3x3(real32 (*Dest)[3])
{
    Dest[0][0] = -Dest[0][0];
    Dest[0][1] = -Dest[0][1];
    Dest[0][2] = -Dest[0][2];
    Dest[1][0] = -Dest[1][0];
    Dest[1][1] = -Dest[1][1];
    Dest[1][2] = -Dest[1][2];
    Dest[2][0] = -Dest[2][0];
    Dest[2][1] = -Dest[2][1];
    Dest[2][2] = -Dest[2][2];
}

inline void MatrixIdentity3x3(real32 (*Dest)[3])
{
    Dest[0][0] = 1;
    Dest[0][1] = 0;
    Dest[0][2] = 0;

    Dest[1][0] = 0;
    Dest[1][1] = 1;
    Dest[1][2] = 0;

    Dest[2][0] = 0;
    Dest[2][1] = 0;
    Dest[2][2] = 1;
}

inline void
MatrixIdentity4x4(real32 *Dest)
{
    Dest[0] = Dest[5] = Dest[10] = Dest[15] = 1.0f;
    Dest[1] = Dest[2] = Dest[3] =
        Dest[4] = Dest[6] = Dest[7] =
        Dest[8] = Dest[9] = Dest[11] =
        Dest[12] = Dest[13] = Dest[14] = 0.0f;
}

inline real32 MatrixDeterminant3x3(real32 const *SourceInit)
{
    triple const *Source = (triple const *)SourceInit;
    return(Source[0][0] * (Source[1][1] * Source[2][2] -
                           Source[1][2] * Source[2][1]) -
           Source[0][1] * (Source[1][0] * Source[2][2] -
                           Source[1][2] * Source[2][0]) +
           Source[0][2] * (Source[1][0] * Source[2][1] -
                           Source[1][1] * Source[2][0]));
}

inline real32 MatrixDeterminant4x3(real32 const *SourceInit)
{
    quad const *Source = (quad const *)SourceInit;
    return(Source[0][0] * (Source[1][1] * Source[2][2] -
                           Source[2][1] * Source[1][2]) -
           Source[1][0] * (Source[0][1] * Source[2][2] -
                           Source[2][1] * Source[0][2]) +
           Source[2][0] * (Source[0][1] * Source[1][2] -
                           Source[1][1] * Source[0][2]));
}

inline real32
MatrixTrace3x3(real32 const *SourceInit)
{
    triple const *Source = (triple const *)SourceInit;
    return(Source[0][0] + Source[1][1] + Source[2][2]);
}

inline void MatrixTranspose3x3(triple *Dest)
{
    Swap(Dest[0][1], Dest[1][0]);
    Swap(Dest[0][2], Dest[2][0]);
    Swap(Dest[1][2], Dest[2][1]);
}

//
// Matrix/matrix operations
//
inline void MatrixEquals3x3(real32 *Dest, real32 const *Source)
{
    Dest[0] = Source[0];
    Dest[1] = Source[1];
    Dest[2] = Source[2];
    Dest[3] = Source[3];
    Dest[4] = Source[4];
    Dest[5] = Source[5];
    Dest[6] = Source[6];
    Dest[7] = Source[7];
    Dest[8] = Source[8];
}

inline void MatrixEquals3x3Transpose(real32 *Dest, real32 const *Source)
{
    Dest[0] = Source[0];
    Dest[1] = Source[3];
    Dest[2] = Source[6];
    Dest[3] = Source[1];
    Dest[4] = Source[4];
    Dest[5] = Source[7];
    Dest[6] = Source[2];
    Dest[7] = Source[5];
    Dest[8] = Source[8];
}

inline void MatrixEquals4x4(real32 *Dest, real32 const *Source)
{
    Dest[0] = Source[0];
    Dest[1] = Source[1];
    Dest[2] = Source[2];
    Dest[3] = Source[3];
    Dest[4] = Source[4];
    Dest[5] = Source[5];
    Dest[6] = Source[6];
    Dest[7] = Source[7];
    Dest[8] = Source[8];
    Dest[9] = Source[9];
    Dest[10] = Source[10];
    Dest[11] = Source[11];
    Dest[12] = Source[12];
    Dest[13] = Source[13];
    Dest[14] = Source[14];
    Dest[15] = Source[15];
}

inline void Matrix3x3Equals4x4(real32 *Dest, real32 const *Source)
{
    Dest[0] = Source[0];
    Dest[1] = Source[1];
    Dest[2] = Source[2];

    Dest[3] = Source[4];
    Dest[4] = Source[5];
    Dest[5] = Source[6];

    Dest[6] = Source[8];
    Dest[7] = Source[9];
    Dest[8] = Source[10];
}

inline void Matrix3x3Equals4x4Transpose(real32 *Dest, real32 const *Source)
{
    Dest[0] = Source[0];
    Dest[1] = Source[4];
    Dest[2] = Source[8];

    Dest[3] = Source[1];
    Dest[4] = Source[5];
    Dest[5] = Source[9];

    Dest[6] = Source[2];
    Dest[7] = Source[6];
    Dest[8] = Source[10];
}

inline void MatrixAdd3x3(real32 *Dest, real32 const *Source)
{
    Dest[0] += Source[0];
    Dest[1] += Source[1];
    Dest[2] += Source[2];
    Dest[3] += Source[3];
    Dest[4] += Source[4];
    Dest[5] += Source[5];
    Dest[6] += Source[6];
    Dest[7] += Source[7];
    Dest[8] += Source[8];
}

inline void ScaleMatrixAdd3x3(real32 *Dest, real32 C, real32 const *Source)
{
    Dest[0] += C*Source[0];
    Dest[1] += C*Source[1];
    Dest[2] += C*Source[2];
    Dest[3] += C*Source[3];
    Dest[4] += C*Source[4];
    Dest[5] += C*Source[5];
    Dest[6] += C*Source[6];
    Dest[7] += C*Source[7];
    Dest[8] += C*Source[8];
}

inline void ScaleMatrixAdd4x3(real32 *Dest4x4, real32 C, real32 const *Source4x4)
{
    Dest4x4[ 0] += C*Source4x4[ 0];
    Dest4x4[ 1] += C*Source4x4[ 1];
    Dest4x4[ 2] += C*Source4x4[ 2];

    Dest4x4[ 4] += C*Source4x4[ 4];
    Dest4x4[ 5] += C*Source4x4[ 5];
    Dest4x4[ 6] += C*Source4x4[ 6];

    Dest4x4[ 8] += C*Source4x4[ 8];
    Dest4x4[ 9] += C*Source4x4[ 9];
    Dest4x4[10] += C*Source4x4[10];

    Dest4x4[12] += C*Source4x4[12];
    Dest4x4[13] += C*Source4x4[13];
    Dest4x4[14] += C*Source4x4[14];
}

inline void
ScaleMatrixPlusScaleMatrix3x3(real32 *Dest,
                              real32 C0, real32 const *M0,
                              real32 C1, real32 const *M1)
{
    Dest[0] = C0*M0[0] + C1*M1[0];
    Dest[1] = C0*M0[1] + C1*M1[1];
    Dest[2] = C0*M0[2] + C1*M1[2];
    Dest[3] = C0*M0[3] + C1*M1[3];
    Dest[4] = C0*M0[4] + C1*M1[4];
    Dest[5] = C0*M0[5] + C1*M1[5];
    Dest[6] = C0*M0[6] + C1*M1[6];
    Dest[7] = C0*M0[7] + C1*M1[7];
    Dest[8] = C0*M0[8] + C1*M1[8];
}

inline void MatrixScale3x3(real32 *Dest, real32 Scalar)
{
    Dest[0] *= Scalar;
    Dest[1] *= Scalar;
    Dest[2] *= Scalar;
    Dest[3] *= Scalar;
    Dest[4] *= Scalar;
    Dest[5] *= Scalar;
    Dest[6] *= Scalar;
    Dest[7] *= Scalar;
    Dest[8] *= Scalar;
}

inline void MatrixScale3x3(real32 *Dest, real32 Scalar, real32 const *Source)
{
    Dest[0] = Source[0] * Scalar;
    Dest[1] = Source[1] * Scalar;
    Dest[2] = Source[2] * Scalar;
    Dest[3] = Source[3] * Scalar;
    Dest[4] = Source[4] * Scalar;
    Dest[5] = Source[5] * Scalar;
    Dest[6] = Source[6] * Scalar;
    Dest[7] = Source[7] * Scalar;
    Dest[8] = Source[8] * Scalar;
}

//
// Vector/matrix operations
//
inline void VectorEqualsColumn3(real32 *Dest, real32 const *Matrix,
                                int32x ColumnIndex)
{
    Dest[0] = Matrix[0*3 + ColumnIndex];
    Dest[1] = Matrix[1*3 + ColumnIndex];
    Dest[2] = Matrix[2*3 + ColumnIndex];
}

inline void VectorEqualsRow3(real32 *Dest, real32 const *Matrix,
                             int32x RowIndex)
{
    Dest[0] = Matrix[RowIndex*3 + 0];
    Dest[1] = Matrix[RowIndex*3 + 1];
    Dest[2] = Matrix[RowIndex*3 + 2];
}

inline void VectorTransform3(real32 *Dest, real32 const *Transform)
{
    real32 const D0 = Dest[0];
    real32 const D1 = Dest[1];
    real32 const D2 = Dest[2];

    Dest[0] = D0*Transform[0] + D1*Transform[1] + D2*Transform[2];
    Dest[1] = D0*Transform[3] + D1*Transform[4] + D2*Transform[5];
    Dest[2] = D0*Transform[6] + D1*Transform[7] + D2*Transform[8];
}

inline void TransposeVectorTransform3(real32 *Dest,
                                      real32 const *TransformInit)
{
    triple const *Transform = (triple const *)TransformInit;

    real32 const D0 = Dest[0];
    real32 const D1 = Dest[1];
    real32 const D2 = Dest[2];

    Dest[0] = D0*Transform[0][0] + D1*Transform[1][0] + D2*Transform[2][0];
    Dest[1] = D0*Transform[0][1] + D1*Transform[1][1] + D2*Transform[2][1];
    Dest[2] = D0*Transform[0][2] + D1*Transform[1][2] + D2*Transform[2][2];
}

inline void VectorTransform3(real32 *Dest,
                             real32 const *TransformInit,
                             real32 const *Source)
{
    triple const *Transform = (triple const *)TransformInit;

    Dest[0] = (Source[0] * Transform[0][0] +
               Source[1] * Transform[0][1] +
               Source[2] * Transform[0][2]);
    Dest[1] = (Source[0] * Transform[1][0] +
               Source[1] * Transform[1][1] +
               Source[2] * Transform[1][2]);
    Dest[2] = (Source[0] * Transform[2][0] +
               Source[1] * Transform[2][1] +
               Source[2] * Transform[2][2]);
}

inline void TransposeVectorTransform3(real32 *Dest,
                                      real32 const *TransformInit,
                                      real32 const *Source)
{
    triple const *Transform = (triple const *)TransformInit;

    Dest[0] = (Source[0]*Transform[0][0] +
               Source[1]*Transform[1][0] +
               Source[2]*Transform[2][0]);
    Dest[1] = (Source[0]*Transform[0][1] +
               Source[1]*Transform[1][1] +
               Source[2]*Transform[2][1]);
    Dest[2] = (Source[0]*Transform[0][2] +
               Source[1]*Transform[1][2] +
               Source[2]*Transform[2][2]);
}

inline void VectorTransform4x3(real32 *Dest, real32 D3, real32 const *Transform)
{
    real32 const D0 = Dest[0];
    real32 const D1 = Dest[1];
    real32 const D2 = Dest[2];

    Dest[0] = D0*Transform[0] + D1*Transform[4] + D2*Transform[8] + D3*Transform[12];
    Dest[1] = D0*Transform[1] + D1*Transform[5] + D2*Transform[9] + D3*Transform[13];
    Dest[2] = D0*Transform[2] + D1*Transform[6] + D2*Transform[10] + D3*Transform[14];
}

inline void VectorTransform4x3(real32 *Dest, real32 const *Source,
                               real32 const D3, real32 const *Transform)
{
    real32 const D0 = Source[0];
    real32 const D1 = Source[1];
    real32 const D2 = Source[2];

    Dest[0] = D0*Transform[0] + D1*Transform[4] + D2*Transform[8] + D3*Transform[12];
    Dest[1] = D0*Transform[1] + D1*Transform[5] + D2*Transform[9] + D3*Transform[13];
    Dest[2] = D0*Transform[2] + D1*Transform[6] + D2*Transform[10] + D3*Transform[14];
}

inline void TransposeVectorTransform4x3(real32 *Dest, real32 D3, real32 const *Transform)
{
    real32 const D0 = Dest[0];
    real32 const D1 = Dest[1];
    real32 const D2 = Dest[2];

    Dest[0] = D0*Transform[0] + D1*Transform[1] + D2*Transform[2] + D3*Transform[12];
    Dest[1] = D0*Transform[4] + D1*Transform[5] + D2*Transform[6] + D3*Transform[13];
    Dest[2] = D0*Transform[8] + D1*Transform[9] + D2*Transform[10] + D3*Transform[14];
}

inline void TransposeVectorTransform4x3(real32 *Dest, real32 const *Source,
                                         real32 const D3, real32 const *Transform)
{
    real32 const D0 = Source[0];
    real32 const D1 = Source[1];
    real32 const D2 = Source[2];

    Dest[0] = D0*Transform[0] + D1*Transform[1] + D2*Transform[2] + D3*Transform[12];
    Dest[1] = D0*Transform[4] + D1*Transform[5] + D2*Transform[6] + D3*Transform[13];
    Dest[2] = D0*Transform[8] + D1*Transform[9] + D2*Transform[10] + D3*Transform[14];
}

inline void VectorTransform4(real32 *Dest, real32 const *Transform)
{
    real32 const D0 = Dest[0];
    real32 const D1 = Dest[1];
    real32 const D2 = Dest[2];
    real32 const D3 = Dest[3];

    Dest[0] = D0*Transform[0] + D1*Transform[1] + D2*Transform[2] + D3*Transform[3];
    Dest[1] = D0*Transform[4] + D1*Transform[5] + D2*Transform[6] + D3*Transform[7];
    Dest[2] = D0*Transform[8] + D1*Transform[9] + D2*Transform[10] + D3*Transform[11];
    Dest[3] = D0*Transform[12] + D1*Transform[13] + D2*Transform[14] + D3*Transform[15];
}

inline void TransposeVectorTransform4x4(real32 *Dest, real32 const *Transform)
{
    real32 const D0 = Dest[0];
    real32 const D1 = Dest[1];
    real32 const D2 = Dest[2];
    real32 const D3 = Dest[3];

    Dest[0] = D0*Transform[0] + D1*Transform[4] + D2*Transform[8] + D3*Transform[12];
    Dest[1] = D0*Transform[1] + D1*Transform[5] + D2*Transform[9] + D3*Transform[13];
    Dest[2] = D0*Transform[2] + D1*Transform[6] + D2*Transform[10] + D3*Transform[14];
    Dest[3] = D0*Transform[3] + D1*Transform[7] + D2*Transform[11] + D3*Transform[15];
}

//
// Composite operations
//

inline real32 SquaredDistanceBetween3(real32 const *Vector0, real32 const *Vector1)
{
    triple Difference;
    VectorSubtract3(Difference, Vector0, Vector1);
    return(VectorLengthSquared3(Difference));
}

inline real32 SquaredDistanceBetweenN(int32x ElementCount,
                                      real32 const *Vector0,
                                      real32 const *Vector1)
{
    real32 Result = 0.0f;

    while(ElementCount--)
    {
        Result += Square(*Vector0++ - *Vector1++);
    }

    return(Result);
}

inline void ScaleVectorAdd3(real32 *Dest, real32 const C,
                            real32 const *Addend)
{
    Dest[0] += C * Addend[0];
    Dest[1] += C * Addend[1];
    Dest[2] += C * Addend[2];
}

inline void ScaleVectorAdd4(real32 *Dest, real32 const C,
                            real32 const *Addend)
{
    Dest[0] += C * Addend[0];
    Dest[1] += C * Addend[1];
    Dest[2] += C * Addend[2];
    Dest[3] += C * Addend[3];
}

inline void ScaleVectorAdd3(real32 *Dest, real32 const *Addend,
                            real32 const C, real32 const *ScaledAddend)
{
    Dest[0] = Addend[0] + C*ScaledAddend[0];
    Dest[1] = Addend[1] + C*ScaledAddend[1];
    Dest[2] = Addend[2] + C*ScaledAddend[2];
}

inline void DestScaleVectorAdd3(real32 *Dest, real32 const C,
                                real32 const *Addend)
{
    Dest[0] = C*Dest[0] + Addend[0];
    Dest[1] = C*Dest[1] + Addend[1];
    Dest[2] = C*Dest[2] + Addend[2];
}

inline void ScaleVectorPlusScaleVector3(real32 *Dest,
                                        real32 const C0,
                                        real32 const *V0,
                                        real32 const C1,
                                        real32 const *V1)
{
    Dest[0] = C0*V0[0] + C1*V1[0];
    Dest[1] = C0*V0[1] + C1*V1[1];
    Dest[2] = C0*V0[2] + C1*V1[2];
}

inline void ScaleVectorPlusScaleVector4(real32 *Dest,
                                        real32 const C0,
                                        real32 const *V0,
                                        real32 const C1,
                                        real32 const *V1)
{
    Dest[0] = C0*V0[0] + C1*V1[0];
    Dest[1] = C0*V0[1] + C1*V1[1];
    Dest[2] = C0*V0[2] + C1*V1[2];
    Dest[3] = C0*V0[3] + C1*V1[3];
}


static inline vector float VecInvSquareRootFast(vector float Value)
{
    vector float Half   = spu_splats(0.5f);
    vector float Three  = spu_splats(3.0f);

    vector float Vec = spu_rsqrte(Value);
    Vec = Vec * Half * ( Three - Vec * Vec * Value ); //12
    Vec = Vec * Half * ( Three - Vec * Vec * Value ); //24

    return Vec;
}

inline vector float VecFastSquareRootGreater0(vector float Value)
{
    return Value * VecInvSquareRootFast(Value);
}


static inline vector float InvSquareRootFast(real32 Value)
{
    return VecInvSquareRootFast(spu_splats(Value));
}


inline real32 FastInvSquareRootGreater0(real32 Value)
{
    return InvSquareRootFast(Value)[0];
}

inline real32 FastSquareRootGreater0(real32 Value)
{
    return VecFastSquareRootGreater0(spu_splats(Value))[0];
}

inline real32 FastDivGreater0(real32 Numerator, real32 Den)
{
    // Tested exhaustively: all positive floats, including
    // denorms.  No more than 2 iterations required for 1 ULP
    // precision.
    vector float DenVec = spu_splats(Den);
    vector float One    = spu_splats(1.0f);

    vector float est = spu_re(DenVec);
    est = (One - est * DenVec) * est + est;
    est = (One - est * DenVec) * est + est;

    return est[0] * Numerator;
}

inline void Normalize4(real32* Dest)
{
    vector float v = { Dest[0], Dest[1], Dest[2], Dest[3] };
    Vectormath::Aos::Vector4 dotv(Dest[0], Dest[1], Dest[2], Dest[3]);
    vector float scale = InvSquareRootFast(Vectormath::Aos::dot(dotv, dotv));

    vector float normVec = v * scale;
    Dest[0] = normVec[0];
    Dest[1] = normVec[1];
    Dest[2] = normVec[2];
    Dest[3] = normVec[3];
}

inline void NormalizeWithTest4(real32 *Dest)
{
    Normalize4(Dest);
}


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MATH_INLINES_SPU_H
#endif
