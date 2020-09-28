// ========================================================================
// $File: //jeffr/granny/rt/granny_intersection.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_INTERSECTION_H)
#include "granny_intersection.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

int32x GRANNY
RayIntersectsPlaneAt(real32 const *PlaneNormal, real32 PlaneD,
                     real32 const *RayOrigin,
                     real32 const *RayNormal,
                     real32 &T)
{
    real32 const Denominator = InnerProduct3(PlaneNormal, RayNormal);
    if(Denominator != 0.0f)
    {
        T = -((PlaneD + InnerProduct3(PlaneNormal, RayOrigin)) /
              Denominator);

        return((T >= 0.0f) ? 1 : -1);
    }

    return(0);
}

inline void
SphereValues(real32 const *Center, real32 Radius,
             real32 const *RayOrigin, real32 const *RayNormal,
             real32 &cTd, real32 &RootTerm)
{
    triple RelativeCenter;
    VectorSubtract3(RelativeCenter, Center, RayOrigin);

    real32 const cTc = InnerProduct3(RelativeCenter, RelativeCenter);
    real32 const r2 = Square(Radius);

    cTd = InnerProduct3(RelativeCenter, RayNormal);
    RootTerm = Square(cTd) - cTc + r2;
}

bool GRANNY
RayIntersectsSphere(real32 const *Center, real32 Radius,
                    real32 const *RayOrigin, real32 const *RayNormal)
{
    real32 cTd, RootTerm;
    SphereValues(Center, Radius, RayOrigin, RayNormal, cTd, RootTerm);
    return(RootTerm >= 0.0f);
}

int32x GRANNY
RayIntersectsSphereAt(real32 const *Center, real32 Radius,
                      real32 const *RayOrigin, real32 const *RayNormal,
                      real32 &MinT, real32 &MaxT)
{
    real32 cTd, RootTerm;
    SphereValues(Center, Radius, RayOrigin, RayNormal, cTd, RootTerm);

    if(RootTerm >= 0.0f)
    {
        RootTerm = SquareRoot(RootTerm);
        MinT = cTd - RootTerm;
        MaxT = cTd + RootTerm;

        return(((MinT >= 0.0f) || (MaxT >= 0.0f)) ? 1 : -1);
    }

    return(0);
}

static void
ClipToPlane(real32 const *Normal, real32 const D,
            real32 const *RayOrigin, real32 const *RayNormal,
            real32 &MinT, real32 &MaxT)
{
    real32 const Denominator = InnerProduct3(Normal, RayNormal);
    if(Denominator != 0.0f)
    {
        real32 T = -((D + InnerProduct3(Normal, RayOrigin)) / Denominator);
        if(Denominator < 0.0f)
        {
            if(MinT < T)
            {
                MinT = T;
            }
        }
        else
        {
            if(MaxT > T)
            {
                MaxT = T;
            }
        }
    }
    else
    {
        // Since the ray is parallel to the plane, test to see if
        //  the origin is contained in the negative halfspace.  If
        //  not, we know that no intersection is possible
        if(InnerProduct3(Normal, RayOrigin) + D > 0)
        {
            MinT = Real32Maximum;
            MaxT = -Real32Maximum;
        }
    }
}

static void
ClipToPlane(real32 const *Normal, real32 const D,
            real32 const *RayOrigin, real32 const *RayNormal,
            real32 *MinNormal, real32 *MaxNormal,
            real32 &MinT, real32 &MaxT)
{
    real32 const Denominator = InnerProduct3(Normal, RayNormal);
    if(Denominator != 0.0f)
    {
        real32 T = -((D + InnerProduct3(Normal, RayOrigin)) / Denominator);
        if(Denominator < 0.0f)
        {
            if(MinT < T)
            {
                VectorEquals3(MinNormal, Normal);
                MinT = T;
            }
        }
        else
        {
            if(MaxT > T)
            {
                VectorEquals3(MaxNormal, Normal);
                MaxT = T;
            }
        }
    }
    else
    {
        // Since the ray is parallel to the plane, test to see if
        //  the origin is contained in the negative halfspace.  If
        //  not, we know that no intersection is possible
        if(InnerProduct3(Normal, RayOrigin) + D > 0)
        {
            MinT = Real32Maximum;
            MaxT = -Real32Maximum;
        }
    }
}

int32x GRANNY
RayIntersectsBox(real32 const *Transform4x4,
                 real32 const *Min3, real32 const *Max3,
                 real32 const *RayOrigin,
                 real32 const *RayNormal)
{
    real32 MinT = -Real32Maximum;
    real32 MaxT = Real32Maximum;

    triple LocalRayOrigin;
    VectorSubtract3(LocalRayOrigin, RayOrigin, &Transform4x4[12]);

    triple Normal;

    {for(int32x Axis = 0;
         Axis < 3;
         ++Axis)
    {
        VectorEquals3(Normal, &Transform4x4[4*Axis]);
        ClipToPlane(Normal, -Max3[Axis], LocalRayOrigin, RayNormal, MinT, MaxT);
        VectorNegate3(Normal);
        ClipToPlane(Normal, Min3[Axis], LocalRayOrigin, RayNormal, MinT, MaxT);
    }}

    if(MinT <= MaxT)
    {
        return(((MinT >= 0.0f) || (MaxT >= 0.0f)) ? 1 : 0);
    }

    return(0);
}

int32x GRANNY
RayIntersectsBoxAt(real32 const *Transform4x4,
                   real32 const *Min3, real32 const *Max3,
                   real32 const *RayOrigin,
                   real32 const *RayNormal,
                   box_intersection &Intersection)
{
    real32 &MinT = Intersection.MinT = -Real32Maximum;
    real32 *MinNormal = Intersection.MinNormal;

    real32 &MaxT = Intersection.MaxT = Real32Maximum;
    real32 *MaxNormal = Intersection.MaxNormal;

    triple LocalRayOrigin;
    VectorSubtract3(LocalRayOrigin, RayOrigin, &Transform4x4[12]);

    triple Normal;

    {for(int32x Axis = 0;
         Axis < 3;
         ++Axis)
    {
        VectorEquals3(Normal, &Transform4x4[4*Axis]);
        real32 const Scale2 = VectorLengthSquared3(Normal);
        ClipToPlane(Normal, -Max3[Axis] * Scale2, LocalRayOrigin, RayNormal,
                    MinNormal, MaxNormal, MinT, MaxT);
        VectorNegate3(Normal);
        ClipToPlane(Normal, Min3[Axis] * Scale2, LocalRayOrigin, RayNormal,
                    MinNormal, MaxNormal, MinT, MaxT);
    }}

    if(MinT <= MaxT)
    {
        return(((MinT >= 0.0f) || (MaxT >= 0.0f)) ? 1 : 0);
    }

    return(0);
}

int32x GRANNY
RayIntersectsTriangleAt(real32 const *P0, real32 const *P1, real32 const *P2,
                        real32 const *RayOrigin,
                        real32 const *RayNormal,
                        triangle_intersection &Intersection)
{
    real32 *Edge1 = Intersection.EdgeU;
    real32 *Edge2 = Intersection.EdgeV;

    VectorSubtract3(Edge1, P1, P0);
    VectorSubtract3(Edge2, P2, P0);

    // Find the determinant of the matrix formed by the direction
    // and the two edges (by means of the triple product)
    // then the ray is in the same plane as the triangle
    triple P;
    VectorCrossProduct3(P, RayNormal, Edge2);
    real32 const Determinant = InnerProduct3(P, Edge1);

    // Since the determinant measures the area suggested by the matrix,
    // if it is near zero then the ray is in the plane of the triangle
    if(Determinant != 0.0f)
    {
        real32 const InverseDeterminant = 1.0f / Determinant;
        triple T;
        VectorSubtract3(T, RayOrigin, P0);
        Intersection.TriangleU =
            InnerProduct3(T, P) * InverseDeterminant;
        if((Intersection.TriangleU > 0.0f) && (Intersection.TriangleU < 1.0f))
        {
            triple Q;
            VectorCrossProduct3(Q, T, Edge1);

            Intersection.TriangleV =
                InnerProduct3(RayNormal, Q) * InverseDeterminant;
            if((Intersection.TriangleV > 0.0f) &&
               ((Intersection.TriangleU + Intersection.TriangleV) < 1.0f))
            {
                Intersection.T =
                    InnerProduct3(Q, Edge2) * InverseDeterminant;
                Intersection.Backfacing = (Determinant < 0.0f);

                return((Intersection.T >= 0.0f) ? 1 : -1);
            }
        }
    }
    else
    {
        // TODO: Shouldn't this do an intersection with the boundary
        // of the two edges?
    }

    return(0);
}
