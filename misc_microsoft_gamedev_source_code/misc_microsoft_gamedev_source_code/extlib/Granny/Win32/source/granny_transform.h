#if !defined(GRANNY_TRANSFORM_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_transform.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TransformGroup);

EXPTYPE enum transform_flags
{
    HasPosition = 0x1,
    HasOrientation = 0x2,
    HasScaleShear = 0x4
};

EXPTYPE struct transform
{
    uint32 Flags;
    triple Position;
    quad Orientation;
    triple ScaleShear[3];
};

EXPAPI GS_SAFE void SetTransform(transform &Result,
                                 real32 const *Position3,
                                 real32 const *Orientation4,
                                 real32 const *ScaleShear3x3);
EXPAPI GS_SAFE void SetTransformWithIdentityCheck(transform &Result,
                                                  real32 const *Position3,
                                                  real32 const *Orientation4,
                                                  real32 const *ScaleShear3x3);
EXPAPI GS_SAFE void MakeIdentity(transform &Result);
EXPAPI GS_SAFE void ZeroTransform(transform &Result);

EXPAPI GS_SAFE real32 GetTransformDeterminant(transform const &Transform);

// These will transform vectors
EXPAPI GS_SAFE void TransformVectorInPlace(real32 *Result, transform const &Transform);
EXPAPI GS_SAFE void TransformVectorInPlaceTransposed(real32 *Result, transform const &Transform);
EXPAPI GS_SAFE void TransformVector(real32 *Dest, transform const &Transform,
                                    real32 const *Source);
EXPAPI GS_SAFE void TransformPointInPlace(real32 *Result, transform const &Transform);
EXPAPI GS_SAFE void TransformPoint(real32 *Dest, transform const &Transform,
                                   real32 const *Source);

// These will perform straight transform concatenation
EXPAPI GS_SAFE void PreMultiplyBy(transform &Transform, transform const &PreMult);
EXPAPI GS_SAFE void PostMultiplyBy(transform &Transform, transform const &PostMult);
EXPAPI GS_SAFE void Multiply(transform &Result, transform const &A, transform const &B);
EXPAPI GS_SAFE void LinearBlendTransform(transform &Result, transform const &A,
                                         real32 t, transform const &B);

EXPAPI GS_SAFE void BuildInverse(transform &Result, transform const &Source);

EXPAPI GS_SAFE void SimilarityTransform(transform &Result,
                                        real32 const *Affine3,
                                        real32 const *Linear3x3,
                                        real32 const *InverseLinear3x3);

// These will invert one or both of their operands prior to concatenation
// (or, more specifically, they will carry out the operation in a manner
// such that the result is equivalent)
void PreMultiplyByInverse(transform &Transform, transform const &PreMult);
void PostMultiplyByInverse(transform &Transform, transform const &PostMult);
void MultiplyInverseA(transform &Result, transform const &A, transform const &B);
void MultiplyInverseB(transform &Result, transform const &A, transform const &B);

// These will generate the upper 3x3 portion of a standard transform matrix
// matrix out the transform struct.
EXPAPI GS_SAFE void BuildCompositeTransform(transform const &Transform,
                                            int32 Stride,
                                            real32 *Composite3x3);
EXPAPI GS_SAFE void BuildCompositeTransform4x4(transform const &Transform,
                                               real32 *Composite4x4);
EXPAPI GS_SAFE void BuildCompositeTransform4x3(transform const &Transform,
                                               real32 *Composite4x3);

/*
void BuildTransposedCompositeTransform(transform const &Transform,
                                              int32 Stride,
                                              real32 *Composite3x3);
void BuildTransposedCompositeTransform4(transform const &Transform,
                                               int32x Stride,
                                               real32 *Composite3x4);

void BuildTransposedCompositeTransformIT(transform const &Transform,
                                                int32 Stride,
                                                real32 *Composite3x3,
                                                real32 *ITComposite3x3);

// These will perform various inter-transform blending operations
void TransformMultiplyAccumulate(
    transform &Result, real32 C, transform const &Transform);
*/

transform const GlobalIdentityTransform =
{
    0,
    {0, 0, 0},
    {0, 0, 0, 1},
    {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}},
};

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TRANSFORM_H
#endif
