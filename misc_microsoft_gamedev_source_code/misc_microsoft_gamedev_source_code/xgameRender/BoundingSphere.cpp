//============================================================================
//
//  BoundingSphere.cpp
//
//  Copyright (c) 1997-2001, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xgameRender.h"
#include "render.h"

// xsystem
#include "mathutil.h"
#include "color.h"

// local1
#include "debugprimitives.h"
#include "boundingbox.h"


//============================================================================
//  MACROS
//============================================================================
#define CLIP_PLANE_ORIGIN(plane, normal)                           \
if (clipHint & (plane))                                            \
{                                                                  \
   radiusTimesN.assignProduct(mRadius, (normal));                  \
                                                                   \
   /* If the point furthest "into" of the view volume (wrt this */ \
   /* plane) is out of the volume, then the whole thing is out. */ \
   newPoint.assignDifference(transformedCenter, radiusTimesN);     \
   if ((newPoint.dot((normal))) > 0.0f)                               \
      return BClipHints::cOffscreen;                                \
                                                                   \
   /* If the point furthest "out" of the view volume (wrt this  */ \
   /* plane) is out of the volume, then the whole thing is      */ \
   /* clipped against the plane.                                */ \
   newPoint.assignSum(transformedCenter, radiusTimesN);            \
   if ((newPoint.dot((normal))) > 0.0f)                               \
      clipResult |= (plane);                                       \
}

#define CLIP_PLANE_ORIGIN2(normal)                                 \
   radiusTimesN.assignProduct(mRadius, (normal));                  \
   \
   /* If the point furthest "into" of the view volume (wrt this */ \
   /* plane) is out of the volume, then the whole thing is out. */ \
   newPoint.assignDifference(transformedCenter, radiusTimesN);     \
   if ((newPoint.dot((normal))) > 0.0f)                               \
      return false;


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BBoundingSphere::BBoundingSphere() :
   mRadius(0.0f)
{
   mCenter.zero();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BBoundingSphere::BBoundingSphere(const BVector& center, float radius) :
   mCenter(center),
   mRadius(radius)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BBoundingSphere::~BBoundingSphere()
{
}

//============================================================================
//  INTERFACE
//============================================================================
//----------------------------------------------------------------------------
//  rayIntersectsSphere()
//  Returns true if the ray defined by the point "origin" and vector
//  "direction" intersects this bounding sphere.  Returns false otherwise.
//  Also returns false if direction is an empty vector.
//----------------------------------------------------------------------------
bool BBoundingSphere::rayIntersectsSphere(const BVector& origin, const BVector& direction) const
{
   return(::rayIntersectsSphere(origin, direction, mCenter, mRadius));
}

//----------------------------------------------------------------------------
//  rayIntersectsSphere()
//  Identical to function above except that the "scale" is applied to the
//  sphere's radius.
//----------------------------------------------------------------------------
bool BBoundingSphere::rayIntersectsSphere(const BVector& origin, const BVector& direction, float scale) const
{
   //-- Normalize the direction.
   BVector n = direction;
   if (!n.safeNormalize())
      return false;

   //-- From Graphics Gems I
   BVector originToCenter;
   originToCenter.assignDifference(mCenter, origin);

   float v    = originToCenter.dot(n);
   float disc = (mRadius * mRadius * scale * scale) - ((originToCenter.dot(originToCenter)) - (v * v));
   return (disc >= 0);
}

//----------------------------------------------------------------------------
//  spheresIntersect()
//  Returns true if the given sphere intersects this sphere.
//----------------------------------------------------------------------------
bool BBoundingSphere::spheresIntersect(const BVector& center, float radius) const
{
   return(::spheresIntersect(center, radius, mCenter, mRadius));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BBoundingSphere::translate(const BVector& translation)
{
   //-- Ya gotta love spheres.
   mCenter += translation;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BBoundingSphere::debugList(void) const
{
   blog("Center = (%0.1f, %0.1f, %0.1f), radius = %0.1f", mCenter.x, mCenter.y, mCenter.z, mRadius);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BBoundingSphere::draw(DWORD color) const
{
#ifndef BUILD_FINAL
   BMatrix mtx;   
   mtx.makeIdentity();
   mtx.setTranslation(mCenter);
   //-- Draw the sphere.
   gpDebugPrimitives->addDebugSphere(mtx, mRadius, color);
#endif   
}



