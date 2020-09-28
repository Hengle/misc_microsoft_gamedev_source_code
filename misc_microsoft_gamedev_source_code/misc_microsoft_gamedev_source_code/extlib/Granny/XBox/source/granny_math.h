#if !defined(GRANNY_MATH_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_math.h $
// $DateTime: 2007/08/27 11:08:32 $
// $Change: 15836 $
// $Revision: #27 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_INTRINSICS_H)
#include "granny_intrinsics.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MathGroup);

// Constants
ALIGN16(real32) const Pi32 = 3.14159265358979323846f;

ALIGN16(real32) const GlobalZeroVector[4] = {0, 0, 0, 0};
ALIGN16(real32) const GlobalXAxis[4] = {1, 0, 0, 0};
ALIGN16(real32) const GlobalYAxis[4] = {0, 1, 0, 0};
ALIGN16(real32) const GlobalZAxis[4] = {0, 0, 1, 0};
ALIGN16(real32) const GlobalWAxis[4] = {0, 0, 0, 1};
ALIGN16(real32) const GlobalNegativeXAxis[4] = {-1, 0, 0, 0};
ALIGN16(real32) const GlobalNegativeYAxis[4] = {0, -1, 0, 0};
ALIGN16(real32) const GlobalNegativeZAxis[4] = {0, 0, -1, 0};
ALIGN16(real32) const GlobalNegativeWAxis[4] = {0, 0, 0, -1};
ALIGN16(real32) const GlobalIdentity3x3[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
ALIGN16(real32) const GlobalIdentity4x4[4][4] = {{1, 0, 0, 0},
                                                 {0, 1, 0, 0},
                                                 {0, 0, 1, 0},
                                                 {0, 0, 0, 1}};
ALIGN16(real32) const GlobalIdentity6[6] = {1, 0, 0, 1, 0, 1};
ALIGN16(real32) const GlobalZero16[16] = {0};

// Scalar operations
inline real32 Square(real32 const Value);
inline real32 SquareRoot(real32 const Value);
inline real32 ACos(real32 const Value);
#if !GRANNY_NO_SINCOS
// MAX sometimes defines these, so you can turn Granny's ones off.
inline real32 Sin(real32 const Value);
inline real32 Cos(real32 const Value);
#endif
inline real32 Tan(real32 const Value);
inline real32 ATan2(real32 const SinAngle, real32 const CosAngle);
inline void Swap(real32 &A, real32 &B);
inline real32 Modulus(real32 const Dividend, real32 const Divisor);
inline real32 Maximum(real32 const A, real32 const B);
inline real32 Minimum(real32 const A, real32 const B);
inline real32 IsInRange(real32 const Min, real32 const Value, real32 const Max);
inline real64x AbsoluteValue64(real64x const A);
inline real32 AbsoluteValue(real32 const A);
inline int32 Ceiling(real32 const A);
inline int32 Floor(real32 const A);
inline real32 Clamp(real32 Min, real32 Value, real32 Max);
inline real32 Sign(real32 Value);

inline bool AreEqual(int32x Dimension, real32 const *A, real32 const *B,
                     real32 Epsilon);

// A couple of important fast operations for various platforms, mostly Xenon and Cell.
// Only value for inputs strictly > 0
inline real32 FastSquareRootGreater0(real32 Value);
inline real32 FastInvSquareRootGreater0(real32 Value);
inline real32 FastDivGreater0(real32 Numerator, real32 Denominator);

// Vector operations
inline void VectorZero3(real32 *Dest);
inline void VectorZero4(real32 *Dest);
inline void VectorIdentityQuaternion(real32 *Dest);
inline void VectorSet2(real32 *Dest, real32 const X, real32 const Y);
inline void VectorSet3(real32 *Dest,
                       real32 const X, real32 const Y, real32 const Z);
inline void VectorSet4(real32 *Dest,
                       real32 const X, real32 const Y,
                       real32 const Z, real32 const W);
inline void VectorNegate3(real32 *Dest);
inline void VectorNegate3(real32 *Dest, real32 const *Source);
inline void VectorNegate4(real32 *Dest);
inline void VectorNegate4(real32 *Dest, real32 const *Source);
inline void Normalize3(real32 *Dest);
inline void Normalize4(real32 *Dest);
inline real32 NormalizeOrZero3(real32 *Dest);
inline real32 NormalizeOrZero4(real32 *Dest);
inline void NormalizeCloseToOne3(real32 *Dest);
inline void NormalizeCloseToOne4(real32 *Dest);
inline void NormalizeWithTest4(real32 *Dest);
inline real32 NormalizeN(real32 *Dest);
inline void Conjugate4(real32 *Dest);
inline void Conjugate4(real32 *Dest, real32 const *Source);
inline void ConstructQuaternion4(real32 *Quaternion,
                                 real32 const *Axis,
                                 real32 const Angle);

inline void ElementMinimum3(real32 *Result, real32 *Compare);
inline void ElementMaximum3(real32 *Result, real32 *Compare);

EXPAPI GS_PARAM void EnsureQuaternionContinuity(int32x QuaternionCount,
                                                real32 *Quaternions);

// Vector/scalar operations
inline void VectorScale3(real32 *Dest, real32 const Scalar);
inline void VectorScale3(real32 *Dest, real32 const Scalar,
                         real32 const *Source);
inline void VectorScale4(real32 *Dest, real32 const Scalar);
inline void VectorScale4(real32 *Dest, real32 const Scalar,
                         real32 const *Source);

inline void VectorScaleN(real32 *Dest, real32 const Scalar, int32x const Dimension);
inline void VectorScaleN(real32 *Dest, real32 const Scalar,
                         real32 const *Source, int32x const Dimension);

// Vector/vector operations
inline void VectorEquals3(real32 *Dest, real32 const *Source);
inline void VectorEquals4(real32 *Dest, real32 const *Source);
inline void VectorAdd3(real32 *Dest, real32 const *Addend);
inline void VectorAdd4(real32 *Dest, real32 const *Addend);
inline void VectorAdd3(real32 *Dest, real32 const *Addend0,
                       real32 const *Addend1);
inline void VectorAdd4(real32 *Dest, real32 const *Addend0,
                       real32 const *Addend1);
inline void VectorSubtract3(real32 *Dest, real32 const *Addend);
inline void VectorSubtract3(real32 *Dest, real32 const *Vector0,
                            real32 const *Vector1);
inline void VectorSubtract4(real32 *Dest, real32 const *Addend);
inline void VectorSubtract4(real32 *Dest, real32 const *Vector0,
                            real32 const *Vector1);
inline real32 InnerProduct3(real32 const *V0, real32 const *V1);
inline real32 InnerProduct4(real32 const *V0, real32 const *V1);
inline real32 InnerProductN(int32x Dimension,
                            real32 const *V0, real32 const *V1);
inline void VectorCrossProduct3(real32 *Dest, real32 const *Operand1,
                                real32 const *Operand2);
inline void TriangleCrossProduct3(real32 *Dest,
                                  real32 const *V0,
                                  real32 const *V1,
                                  real32 const *V2);
inline void TriangleCrossProduct3(real32 *Dest,
                                  real32 *E01,
                                  real32 *E02,
                                  real32 const *V0,
                                  real32 const *V1,
                                  real32 const *V2);
inline void VectorHadamardProduct3(real32 *Dest, real32 const *Source);
inline void VectorHadamardAccumulate3(real32 *Dest, real32 const *V0, real32 const *V1);

void QuaternionRotationDifference(real32 *Dest,
                                  real32 const *FromOrientation,
                                  real32 const *ToOrientation);
void QuaternionInterpolate4(real32 *Dest,
                            real32 Operand1Coefficient,
                            real32 const *Operand1,
                            real32 Operand2Coefficient,
                            real32 const *Operand2);
void QuaternionMultiply4(real32 *Dest, real32 const *Operand1,
                         real32 const *Operand2);
void NormalQuaternionTransform3(real32 *Vector3, real32 const *Quaternion4);
void QuaternionDifferenceToAngularVelocity(real32 *Result3,
                                           real32 const *From4, real32 const *To4);

// Matrix operations
inline void MatrixZero3x3(real32 *Dest);
inline void MatrixZero4x4(real32 *Dest);
inline void MatrixRows3x3(triple *Dest,
                          float const* Row0,
                          float const* Row1,
                          float const* Row2);
inline void MatrixColumns3x3(triple *Dest,
                             float const* Column0,
                             float const* Column1,
                             float const* Column2);
inline void TransformMatrix4x4(quad *Dest, real32 const *Column0,
                               real32 const *Column1, real32 const *Column2,
                               real32 const *Row3);

inline void AxisRotationColumns3x3(triple *Result,
                                   triple const Axis,
                                   real32 const AngleInRadians);
inline void XRotationColumns3x3(triple *Result, real32 const AngleInRadians);
inline void YRotationColumns3x3(triple *Result, real32 const AngleInRadians);
inline void ZRotationColumns3x3(triple *Result, real32 const AngleInRadians);

void DextralBases(real32 const *XAxis, real32 *YAxis, real32 *ZAxis);
void SinistralBases(real32 const *XAxis, real32 *YAxis, real32 *ZAxis);

inline void NegateMatrix3x3(real32 (*Dest)[3]);
inline void MatrixIdentity3x3(real32 (*Dest)[3]);
inline void MatrixIdentity4x4(real32 *Dest);

inline real32 MatrixDeterminant3x3(real32 const *Source);
inline real32 MatrixDeterminant4x3(real32 const *Source);
inline real32 MatrixTrace3x3(real32 const *Matrix);

inline void MatrixTranspose3x3(triple *Dest);

void MatrixInvert3x3(real32 *Dest3x3, real32 const *Source3x3);
void MatrixInvert4x3(real32 *Dest4x4, real32 const *Source4x4);
EXPAPI GS_SAFE bool PolarDecompose(real32 const *Source3x3, real32 Tolerance,
                                   real32 *Q3x3, real32 *S3x3);

void NormalizeMatrix3x3(real32 *Dest, real32 const *Source);

// Matrix/matrix operations
inline void MatrixEquals3x3(real32 *Dest, real32 const *Source);
inline void MatrixEquals3x3Transpose(real32 *Dest, real32 const *Source);
inline void MatrixEquals4x4(real32 *Dest, real32 const *Source);
inline void Matrix3x3Equals4x4(real32 *Dest3x3, real32 const *Source4x4);
inline void Matrix3x3Equals4x4Transpose(real32 *Dest3x3, real32 const *Source4x4);

inline void MatrixAdd3x3(real32 *Dest, real32 const *Source);
inline void ScaleMatrixAdd3x3(real32 *Dest, real32 C, real32 const *Source);
inline void ScaleMatrixAdd4x3(real32 *Dest4x4, real32 C, real32 const *Source4x4);
inline void ScaleMatrixPlusScaleMatrix3x3(real32 *Dest,
                                          real32 C0, real32 const *M0,
                                          real32 C1, real32 const *M1);
inline void MatrixScale3x3(real32 *Dest, real32 Scalar);
inline void MatrixScale3x3(real32 *Dest, real32 Scalar, real32 const *Source);

void MatrixMultiply3x3(real32 *IntoMatrix3x3,
                       real32 const *Matrix3x3,
                       real32 const *ByMatrix3x3);
void TransposeMatrixMultiply3x3(real32 *IntoMatrix3x3,
                                real32 const *TransposedMatrix3x3,
                                real32 const *ByMatrix3x3);

EXPAPI GS_PARAM void ColumnMatrixMultiply4x3(real32 *IntoMatrix4x4,
                                             real32 const *Matrix4x4,
                                             real32 const *ByMatrix4x4);
EXPAPI GS_PARAM void ColumnMatrixMultiply4x3Transpose(real32 *IntoMatrix4x3,
                                                      real32 const *Matrix4x4,
                                                      real32 const *ByMatrix4x4);
EXPAPI GS_PARAM void ColumnMatrixMultiply4x4(real32 *IntoMatrix4x4,
                                             real32 const *Matrix4x4,
                                             real32 const *ByMatrix4x4);

// Vector/matrix operations
inline void VectorEqualsColumn3(real32 *Dest, real32 const *Matrix,
                                int32x ColumnIndex);
inline void VectorEqualsRow3(real32 *Dest, real32 const *Matrix,
                             int32x RowIndex);

inline void VectorTransform3(real32 *Dest, real32 const *Transform);
inline void TransposeVectorTransform3(real32 *Dest, real32 const *Transform);
inline void VectorTransform3(real32 *Dest,
                             real32 const *Transform,
                             real32 const *Source);
inline void TransposeVectorTransform3(real32 *Dest,
                                      real32 const *Transform,
                                      real32 const *Source);

// This uses a 3-vector, but multiplies by a 4x4, and uses D3 as the 4th element
inline void VectorTransform4x3(real32 *Dest, real32 D3, real32 const *Transform4x4);
inline void VectorTransform4x3(real32 *Dest, real32 const *Source,
                               real32 S3, real32 const *Transform4x4);
inline void TransposeVectorTransform4x3(real32 *Dest, real32 D3, real32 const *Transform4x4);
inline void TransposeVectorTransform4x3(real32 *Dest, real32 const *Source,
                                        real32 S3, real32 const *Transform4x4);

inline void VectorTransform4(real32 *Dest, real32 const *Transform4x4);
inline void TransposeVectorTransform4x4(real32 *Dest, real32 const *Transform4x4);

EXPAPI GS_PARAM void MatrixEqualsQuaternion3x3(real32 *Dest, real32 const *Quaternion);
EXPAPI GS_PARAM void QuaternionEqualsMatrix3x3(real32 *Quaternion, real32 const *Matrix);

// Composite operations
inline real32 VectorLengthSquared3(real32 const *Source);
inline real32 VectorLengthSquared4(real32 const *Source);
inline real32 VectorLengthSquaredN(real32 const *Source);
inline real32 VectorLength3(real32 const *Source);
inline real32 VectorLength4(real32 const *Source);
inline real32 VectorLengthN(real32 const *Source, int32x const Dimension);
inline real32 SquaredDistanceBetween3(real32 const *Vector0,
                                      real32 const *Vector1);
inline real32 SquaredDistanceBetweenN(int32x ElementCount,
                                      real32 const *Vector0,
                                      real32 const *Vector1);
inline void ScaleVectorAdd3(real32 *Dest, real32 const C,
                            real32 const *Addend);
inline void ScaleVectorAdd4(real32 *Dest, real32 const C,
                            real32 const *Addend);
inline void ScaleVectorAdd3(real32 *Dest, real32 const *Addend,
                            real32 const C, real32 const *ScaledAddend);
inline void DestScaleVectorAdd3(real32 *Dest, real32 const C,
                                real32 const *Addend);
inline void ScaleVectorPlusScaleVector3(real32 *Dest,
                                        real32 const C0,
                                        real32 const *V0,
                                        real32 const C1,
                                        real32 const *V1);
inline void ScaleVectorPlusScaleVector4(real32 *Dest,
                                        real32 const C0,
                                        real32 const *V0,
                                        real32 const C1,
                                        real32 const *V1);

EXPAPI GS_PARAM void InPlaceSimilarityTransformPosition(real32 const *Affine3,
                                                        real32 const *Linear3x3,
                                                        real32 *Position3);
EXPAPI GS_PARAM void InPlaceSimilarityTransformOrientation(real32 const *Linear3x3,
                                                           real32 const *InverseLinear3x3,
                                                           real32 *Orientation4);
EXPAPI GS_PARAM void InPlaceSimilarityTransformScaleShear(real32 const *Linear3x3,
                                                          real32 const *InverseLinear3x3,
                                                          real32 *ScaleShear3x3);
EXPAPI GS_PARAM void InPlaceSimilarityTransform(real32 const *Affine3,
                                                real32 const *Linear3x3,
                                                real32 const *InverseLinear3x3,
                                                real32 *Position3,
                                                real32 *Orientation4,
                                                real32 *ScaleShear3x3);
EXPAPI GS_PARAM void InPlaceSimilarityTransform4x3(real32 const *Affine3,
                                                   real32 const *Linear3x3,
                                                   real32 const *InverseLinear3x3,
                                                   real32 *Result4x4);

void DistanceToBoxSquared(real32 *MinCorner, real32 *MaxCorner,
                          real32 *Point,
                          real32 &MinDistanceSquared,
                          real32 &MaxDistanceSquared);

END_GRANNY_NAMESPACE;

// Note: All of the inline math functions are defined in granny_math_inlines.h,
// so if you prefer to compile them out-of-line, you can simply move
// this include to granny_math.cpp instead.
#if PLATFORM_XENON || (PLATFORM_PS3 && PROCESSOR_CELL)
   #if !defined(GRANNY_MATH_INLINES_PPC_H)
   #include "granny_math_inlines_ppc.h"
   #endif
#elif PLATFORM_PS3 && PROCESSOR_CELL_SPU
   #if !defined(GRANNY_MATH_INLINES_SPU_H)
   #include "granny_math_inlines_spu.h"
   #endif
#else
   #if !defined(GRANNY_MATH_INLINES_H)
   #include "granny_math_inlines.h"
   #endif
#endif

#include "header_postfix.h"
#define GRANNY_MATH_H
#endif
