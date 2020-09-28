//==============================================================================
// triangle.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _TRIANGLE_H_
#define _TRIANGLE_H_

#include "plane.h"

//==============================================================================
// BTriangle
//==============================================================================
class BTriangle
{
   public:
      /*
      const BVector&            getCenter() const { return mCenter; }
      const BPlane&              getPlane() const { return mPlane; }
      const BVector&            getNormal() const { return mPlane.mNormal; }
      float                      getRadius() const { return mRadius; }

      void                       getCenter(BVector& center) const { center=mCenter; }
      void                       getPlane(BPlane& plane) const { plane=mPlane; }
      void                       getNormal(BVector& normal) const { normal=mPlane.mNormal; }
      */

      BVector                   getCenter() const;
      BPlane                     getPlane() const;
      BVector                   getNormal() const;
      float                      getRadius() const;

      void                       getCenter(BVector& center) const;
      void                       getPlane(BPlane& plane) const;
      void                       getNormal(BVector& normal) const;

      /*
      BVector                   mCenter;
      float                      mRadius;
      BPlane                     mPlane;
      */
      BVector                   mPoints[3];

      // Xemu [10/21/2003] -- added for surface lookup from world triangles.  If we need a purer math triangle, this will
      //   have to be split out separately, like a BSimTriangle or something
      long                       mSurface;

};



//===============================================================================
// Triangle List
//===============================================================================
typedef BPointerList<BTriangle> BTrianglePointerList;
typedef BCopyList<BTriangle> BTriangleCopyList;

#endif