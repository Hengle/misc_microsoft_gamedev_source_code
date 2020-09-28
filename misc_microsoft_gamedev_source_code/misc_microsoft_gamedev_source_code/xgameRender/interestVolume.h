//==============================================================================
// File: interestVolume.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#include "math\generalVector.h"
#include "math\plane.h"
#include "containers\staticArray.h"
#include "volumeCuller.h"
#include "matrixTracker.h"

class BInterestVolume
{
public:
   BInterestVolume();
   ~BInterestVolume();
   
   void clear(void);
   void update(const BMatrixTracker& matrixTracker, const BVec3& lightDir, float maxLocalLightRadius, float worldMinY, float worldMaxY);
   
   uint getNumInterestVolumePlanes(void) const { return mNumInterestVolumePlanes; }
   const XMVECTOR* getDirLightInterestVolume(void) const { return mDirLightInterestVolume; }
   const XMVECTOR* getAllLightInterestVolume(void) const { return mAllLightInterestVolume; }
   
private:
   enum { cMaxInterestVolumePlanes = BVolumeCuller::cMaxInclusionPlanes };
   XMVECTOR mDirLightInterestVolume[cMaxInterestVolumePlanes];
   XMVECTOR mAllLightInterestVolume[cMaxInterestVolumePlanes];
   uint mNumInterestVolumePlanes;
   
   typedef BStaticArray<BVec3, 16> BVec3Array;
   typedef BStaticArray<Plane, 32> BPlaneArray;
   
   static void computeConvexHull(BPlaneArray& planes, const BVec3Array& points);
};
