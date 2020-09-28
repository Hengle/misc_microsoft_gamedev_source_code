//============================================================================
//
//  frustum.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "math/plane.h"

//============================================================================
// class BFrustum
// Must be bitwise copyable!
//============================================================================
class BFrustum
{
public:
   enum ePlaneIndex
   {
      cLeftPlane,    // pos X
      cRightPlane,   // neg X
      cBottomPlane,  // pos Y
      cTopPlane,     // neg Y
      cNearPlane,    // pos Z
      cFarPlane,     // neg Z
      cPlaneMax,
      cAllPlanes = (1 << cPlaneMax) - 1
   };

   BFrustum()
   {
   }

   BFrustum(const BMatrix44& proj)
   {
      set(proj);
   }
   
   BFrustum(const BFrustum& rhs)
   {
      *this = rhs;
   }

   BFrustum& operator= (const BFrustum& rhs)
   {
      for (int i = 0; i < cPlaneMax; i++)
         mPlanes[i] = rhs.mPlanes[i];
      return *this;
   }
   
   void clear(void)
   {
      for (int i = 0; i < cPlaneMax; i++)
         mPlanes[i].clear();
   }

#ifdef XBOX   
   BFrustum& set(const XMMATRIX proj)
   {
      return set((const BMatrix44&)proj);
   }
#endif   

   // Handles perspective and orthogonal projections.
   BFrustum& set(const BMatrix44& proj)
   {
      float sign = 1.0f;

      for (int i = 0; i < cPlaneMax; i++, sign *= -1.0f)
      {
         mPlanes[i].setFromEquation(
            (proj.getColumn(i >> 1) * sign + (BVec4((cNearPlane != i) ? proj.getColumn(3) : BVec4(0.0f)))).normalize3()
            );
      }
      return *this;
   }

   BFrustum& transform(const BMatrix44& m)
   {
      for (int i = 0; i < cPlaneMax; i++)
         mPlanes[i] = Plane::transformOrthonormal(mPlanes[i], m);
      return *this;
   }
   
   BFrustum& transform(BFrustum& dest, const BMatrix44& m) const
   {
      for (int i = 0; i < cPlaneMax; i++)
         dest.mPlanes[i] = Plane::transformOrthonormal(mPlanes[i], m);
      return dest;
   }

   // Positive halfspace is inside frustum.
   const Plane& plane(int i) const { return mPlanes[debugRangeCheck(i, cPlaneMax)]; }
   
   const Plane* getPlanes(void) const { return mPlanes; }
  
private:
   Plane mPlanes[cPlaneMax];
};

