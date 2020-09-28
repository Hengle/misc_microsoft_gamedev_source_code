#if !defined(GRANNY_BASIS_CONVERSION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_basis_conversion.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #13 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(BasisConversionGroup);

struct file_info;
struct mesh;
struct skeleton;
struct model;
struct animation;

EXPTYPE enum transform_file_flags
{
    RenormalizeNormals = 0x1,
    ReorderTriangleIndices = 0x2,
};

EXPAPI GS_READ bool ComputeBasisConversion(file_info const &FileInfo,
                                           real32 DesiredUnitsPerMeter,
                                           real32 const *DesiredOrigin3,
                                           real32 const *DesiredRight3,
                                           real32 const *DesiredUp3,
                                           real32 const *DesiredBack3,
                                           real32 *ResultAffine3,
                                           real32 *ResultLinear3x3,
                                           real32 *ResultInverseLinear3x3);

EXPAPI GS_PARAM void TransformMesh(mesh &Mesh,
                                   real32 const *Affine3,
                                   real32 const *Linear3x3,
                                   real32 const *InverseLinear3x3,
                                   real32 AffineTolerance,
                                   real32 LinearTolerance,
                                   uint32x Flags);
EXPAPI GS_PARAM void TransformSkeleton(skeleton &Skeleton,
                                       real32 const *Affine3,
                                       real32 const *Linear3x3,
                                       real32 const *InverseLinear3x3,
                                       real32 AffineTolerance,
                                       real32 LinearTolerance,
                                       uint32x Flags);
EXPAPI GS_PARAM void TransformModel(model &Model,
                                    real32 const *Affine3,
                                    real32 const *Linear3x3,
                                    real32 const *InverseLinear3x3,
                                    real32 AffineTolerance,
                                    real32 LinearTolerance,
                                    uint32x Flags);
EXPAPI GS_PARAM void TransformAnimation(animation &Animation,
                                        real32 const *Affine3,
                                        real32 const *Linear3x3,
                                        real32 const *InverseLinear3x3,
                                        real32 AffineTolerance,
                                        real32 LinearTolerance,
                                        uint32x Flags);
EXPAPI GS_PARAM void TransformFile(file_info &FileInfo,
                                   real32 const *Affine3,
                                   real32 const *Linear3x3,
                                   real32 const *InverseLinear3x3,
                                   real32 AffineTolerance,
                                   real32 LinearTolerance,
                                   uint32x Flags);

EXPAPI GS_PARAM void ResortAllAnimationTrackGroups(animation &Animation);
EXPAPI GS_PARAM void ResortAllFileTrackGroups(file_info &FileInfo);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BASIS_CONVERSION_H
#endif
