#if !defined(GRANNY_INTERSECTION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_intersection.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(CollisionGroup);

EXPAPI GS_SAFE int32x RayIntersectsPlaneAt(real32 const *PlaneNormal, real32 PlaneD,
                                           real32 const *RayOrigin,
                                           real32 const *RayNormal,
                                           real32 &T);

EXPAPI GS_SAFE bool RayIntersectsSphere(real32 const *Center, real32 Radius,
                                        real32 const *RayOrigin,
                                        real32 const *RayNormal);
EXPAPI GS_SAFE int32x RayIntersectsSphereAt(real32 const *Center, real32 Radius,
                                            real32 const *RayOrigin,
                                            real32 const *RayNormal,
                                            real32 &InT, real32 &OutT);

EXPTYPE_EPHEMERAL struct box_intersection
{
    real32 MinT;
    triple MinNormal;

    real32 MaxT;
    triple MaxNormal;
};
EXPAPI GS_SAFE int32x RayIntersectsBox(real32 const *Transform4x4,
                                       real32 const *Min3, real32 const *Max3,
                                       real32 const *RayOrigin,
                                       real32 const *RayNormal);
EXPAPI GS_SAFE int32x RayIntersectsBoxAt(real32 const *Transform4x4,
                                         real32 const *Min3, real32 const *Max3,
                                         real32 const *RayOrigin,
                                         real32 const *RayNormal,
                                         box_intersection &Intersection);

EXPTYPE_EPHEMERAL struct triangle_intersection
{
    real32 T;
    real32 TriangleU;
    real32 TriangleV;

    // This is P1 - P0
    triple EdgeU;

    // This is P2 - P0
    triple EdgeV;

    // This is the triangle's orientation relative to the ray
    bool Backfacing;
};
EXPAPI GS_SAFE int32x RayIntersectsTriangleAt(real32 const *P0,
                                              real32 const *P1,
                                              real32 const *P2,
                                              real32 const *RayOrigin,
                                              real32 const *RayNormal,
                                              triangle_intersection &Intersection);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_INTERSECTION_H
#endif
