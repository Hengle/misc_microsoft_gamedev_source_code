//============================================================================
//
//  cullingManager.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "frustum.h"

// Must be bitwise-copyable and self sufficient.
class BCullingManager
{
public:
   BCullingManager();
   
   BCullingManager(const BCullingManager& other);
   
   BCullingManager& operator= (const BCullingManager& rhs);
  
   void clear(void);
   
   void update(const BFrustum& frustum);
   
   // true if the sphere should be rendered.
   bool isSphereVisible(XMVECTOR center, float radius) const;
         
   // true if the AABB should be rendered.
   bool isAABBVisible(const D3DXVECTOR3& min, const D3DXVECTOR3& max) const;
   
   // c = center
   // d = half diagonol
   bool isAABBVisible(XMVECTOR c, XMVECTOR d) const;
   
   void enableExclusionPlanes(const BFrustum& frustum);
   void disableExclusionPlanes(void) { mUseExclusionPlanes = false; }
   bool getUsingExclusionPlanes(void) const { return mUseExclusionPlanes; }
   
   void enableInclusionPlanes(const XMVECTOR* pPlanes, uint numPlanes);
   void disableInclusionPlanes(void) { mNumInclusionPlanes = 0; }
   uint getNumInclusionPlanes(void) const { return mNumInclusionPlanes; }
   
   const XMVECTOR* getBasePlanes(void) const { return mBasePlanes; }
   const XMVECTOR* getExclusionPlanes(void) const { return mExclusionPlanes; }
   const XMVECTOR* getInclusionPlanes(void) const { return mInclusionPlanes; }
                  
private:
   // The basic set of planes to cull against (typically the camera frustum). 
   // Objects completely outside the basic region are culled.
   enum { cMaxBasePlanes = 6 };
   XMVECTOR mBasePlanes[cMaxBasePlanes];

   // Objects completely inside the exclusion region are culled.
   enum { cMaxExclusionPlanes = 6 };
   XMVECTOR mExclusionPlanes[cMaxExclusionPlanes];
   
   // Objects completely outside the inclusion region are culled.
   enum { cMaxInclusionPlanes = 16 };
   XMVECTOR mInclusionPlanes[cMaxInclusionPlanes];
   
   uchar mNumInclusionPlanes;
   bool mUseExclusionPlanes : 1;
      
   static bool checkSphereForRejection6(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius);
   static bool checkSphereForRejection(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR center, float radius) ;
   static bool checkSphereForInclusion6(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius);
   static int AABBvsFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d);
   static bool AABBTouchesFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d);
   static bool AABBTouchesVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR c, XMVECTOR d);
   static int AABBInsideFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d);
};

