//-----------------------------------------------------------------------------
// File: cone.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "math\generalVector.h"

class Cone
{
public:
   Cone()
   {
   }
                     
   // Accepts cone half angle in radians: [0, PI/2)
   Cone(const BVec3& apex, const BVec3& axis, float ang)
   {
      set(apex, axis, ang);
   }
         
   Cone(const BVec3& apex, const BVec3& axis, const BVec3& corner)
   {
      set(apex, axis, corner);
   }
   
   void set(const BVec3& apex, const BVec3& axis, float ang)
   {
      mApex = apex;
      mAxis = axis;
      mCosAng = cos(ang);
      mSinAng = sin(ang);
   }

   void set(const BVec3& apex, const BVec3& axis, const BVec3& corner)
   {
      mApex = apex;
      mAxis = axis;

      mCosAng = (axis * (corner - apex)) / (corner - apex).len();
      mSinAng = sin(acos(mCosAng));
   }

         float& cosAng(void)      { return mCosAng; }
   const float cosAng(void) const { return mCosAng; }

         float& sinAng(void)      { return mSinAng; }
   const float sinAng(void) const { return mSinAng; }

         BVec3& apex(void)        { return mApex; }
   const BVec3& apex(void) const  { return mApex; }

         BVec3& axis(void)        { return mAxis; }
   const BVec3& axis(void) const  { return mAxis; }
   
private:
   float mCosAng;
   float mSinAng;
   BVec3 mApex;
   BVec3 mAxis;
};
