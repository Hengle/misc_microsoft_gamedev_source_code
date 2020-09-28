//============================================================================
//
//  volumeCuller.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "math\frustum.h"

//============================================================================
// class BVolumeCuller
// Must be bitwise-copyable and self sufficient.
//============================================================================
class BVolumeCuller
{
public:
   BVolumeCuller();
   
   BVolumeCuller(const BVolumeCuller& other);
   
   BVolumeCuller& operator= (const BVolumeCuller& rhs);
  
   void clear(void);
         
   // true if the sphere should be rendered.
   bool isSphereVisible(XMVECTOR center, float radius) const;
   bool isSphereVisible(XMVECTOR center, XMVECTOR radius) const;
         
   // true if the AABB should be rendered.
   bool isAABBVisible(const D3DXVECTOR3& min, const D3DXVECTOR3& max) const;
   
   // c = center
   // d = half diagonol
   bool isAABBVisible(XMVECTOR c, XMVECTOR d) const;
   
   bool isAABBVisibleBounds(XMVECTOR min, XMVECTOR max) const;
   
   void setBasePlanes(const BFrustum& frustum, XMMATRIX projMatrix, bool computeBounds = true);
   void setBasePlanes(XMMATRIX projMatrix, bool computeBounds = true);
   void setBasePlanes(const BFrustum& frustum, bool computeBounds = true);
   void setBasePlanes(XMVECTOR* pPlanes, bool computeBounds = true);
   void setBasePlanes(XMVECTOR min, XMVECTOR max);
   
   void setBaseBounds(XMVECTOR min, XMVECTOR max) { mBaseMin = min; mBaseMax = max; }
   
   void enableBasePlanes(void) { mUseBasePlanes = true; }
   void disableBasePlanes(void) { mUseBasePlanes = false; }
   bool getUsingBasePlanes(void) const { return mUseBasePlanes; }
   uint getNumBasePlanes(void) const { return cMaxBasePlanes; }
   
   void enableExclusionPlanes(const BFrustum& frustum);
   void disableExclusionPlanes(void) { mUseExclusionPlanes = false; }
   bool getUsingExclusionPlanes(void) const { return mUseExclusionPlanes; }
   uint getNumExclusionPlanes(void) const { return cMaxExclusionPlanes; }
   
   void enableInclusionPlanes(const BFrustum& frustum);
   void enableInclusionPlanes(const XMVECTOR* pPlanes, uint numPlanes);
   void disableInclusionPlanes(void) { mNumInclusionPlanes = 0; }
   bool getUsingInclusionPlanes(void) const { return 0 != mNumInclusionPlanes; }
   uint getNumInclusionPlanes(void) const { return mNumInclusionPlanes; }
   
   const XMVECTOR* getBasePlanes(void) const { return mBasePlanes; }
   XMVECTOR getBaseMin(void) const { return mBaseMin; }
   XMVECTOR getBaseMax(void) const { return mBaseMax; }
   
   const XMVECTOR* getExclusionPlanes(void) const { return mExclusionPlanes; }
   const XMVECTOR* getInclusionPlanes(void) const { return mInclusionPlanes; }
      
   uint getSerializeSize(void) const;
   void serialize(uchar* RESTRICT pDst) const;
   uint deserialize(const uchar* RESTRICT pSrc);
   
   enum { cMaxBasePlanes = 6 };
   enum { cMaxExclusionPlanes = 6 };
   enum { cMaxInclusionPlanes = 24 };
                  
private:
   // The basic set of planes to cull against (typically the camera frustum). 
   // Objects completely outside the basic region are culled.
   XMVECTOR mBasePlanes[cMaxBasePlanes];

   // Objects completely inside the exclusion region are culled.
   XMVECTOR mExclusionPlanes[cMaxExclusionPlanes];
   
   // Objects completely outside the inclusion region are culled.
   XMVECTOR mInclusionPlanes[cMaxInclusionPlanes];
   
   XMVECTOR mBaseMin;
   XMVECTOR mBaseMax;
      
   uchar mNumInclusionPlanes;
   
   bool mUseBasePlanes : 1;
   bool mUseExclusionPlanes : 1;
   
   void computeBaseBounds(void);
};

