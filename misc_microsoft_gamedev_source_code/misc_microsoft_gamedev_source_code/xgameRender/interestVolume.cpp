//==============================================================================
// File: interestVolume.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xgameRender.h"
#include "interestVolume.h"
#include "volumeIntersection.h"

BInterestVolume::BInterestVolume() :
   mNumInterestVolumePlanes(0)
{
}

BInterestVolume::~BInterestVolume()
{
}

void BInterestVolume::clear(void)
{
   Utils::ClearObj(mDirLightInterestVolume);
   Utils::ClearObj(mAllLightInterestVolume);
   mNumInterestVolumePlanes = 0;
}

namespace 
{
   const uchar gQuadVerts[6][4] =
   {
      // CCW
      { 4,0,3,7 }, // 0 left
      { 1,5,6,2 }, // 1 right

      { 0,4,5,1 }, // 2 bottom
      { 2,6,7,3 }, // 3 top

      { 0,1,2,3 }, // 4 near
      { 7,6,5,4 }  // 5 far
   };

   const uchar gEdgeVerts[12][2] =    
   {
      { 4, 0 },
      { 0, 3 },
      { 3, 7 },
      { 7, 4 },
      { 1, 5 },
      { 5, 6 },
      { 6, 2 },
      { 2, 1 },
      { 4, 5 },
      { 1, 0 },
      { 6, 7 },
      { 3, 2 }
   };      

   const uchar gEdgeQuads[12][2] =
   {
      { 0,	2 },
      { 0,	4 },
      { 0,	3 },
      { 0,	5 },
      { 1,	2 },
      { 1,	5 },
      { 1,	3 },
      { 1,	4 },
      { 2,	5 },
      { 2,	4 },
      { 3,	5 },
      { 3,	4 }
   };      
} // anonymous namespace

#if 0
void BInterestVolume::update(const BMatrixTracker& matrixTracker, const BVec3& lightDir, float maxLocalLightRadius)
{
   const BFrustum& frustum = matrixTracker.getWorldFrustum();

   BVec3 topPoint(matrixTracker.getWorldCamPosVec4() - lightDir * 3000.0f);
   
   bool topPointIsInsideFrustum = true;
   for (uint quadIndex = 0; quadIndex < 6; quadIndex++)
   {
      const Plane& plane = frustum.plane(quadIndex);

      if (plane.distanceToPoint(topPoint) < 0.0f)
      {
         topPointIsInsideFrustum = false;
         break;
      }
   }      
      
   XMVECTOR frustumVerticesVecs[8];

   matrixTracker.getFrustumVertices(frustumVerticesVecs);

   const BVec4* frustumVertices = reinterpret_cast<const BVec4*>(frustumVerticesVecs);

   const BVec3 frustumCenter((frustumVertices[0] + frustumVertices[6]) * .5f);

   bool facingFlags[6];
   mNumInterestVolumePlanes = 0;

   for (uint quadIndex = 0; quadIndex < 6; quadIndex++)
   {
      const Plane& plane = frustum.plane(quadIndex);

      BDEBUG_ASSERT(plane.distanceToPoint(frustumCenter) >= 0.0f);

      const bool facing = (plane.normal() * lightDir) < 0.0f;

      facingFlags[quadIndex] = facing;

      if (facing)
      {
         mDirLightInterestVolume[mNumInterestVolumePlanes] = XMLoadFloat4((XMFLOAT4*)plane.equation().getPtr());
         mNumInterestVolumePlanes++;
      }  
      else
      {
         Plane oppPlane;
         oppPlane.setFromNormalOrigin(plane.n, topPoint);

         BDEBUG_ASSERT(oppPlane.distanceToPoint(frustumCenter) >= 0.0f);

         mDirLightInterestVolume[mNumInterestVolumePlanes] = XMLoadFloat4((XMFLOAT4*)oppPlane.equation().getPtr());
         mNumInterestVolumePlanes++;
      }       
   }

   for (uint edgeIndex = 0; edgeIndex < 12; edgeIndex++)
   {
      if (facingFlags[gEdgeQuads[edgeIndex][0]] == facingFlags[gEdgeQuads[edgeIndex][1]])
         continue;

      BVec3 edgeVec(frustumVertices[gEdgeVerts[edgeIndex][0]] - frustumVertices[gEdgeVerts[edgeIndex][1]]);
      edgeVec.tryNormalize();

      BVec3 norm(edgeVec % lightDir);
      Plane plane(norm, frustumVertices[gEdgeVerts[edgeIndex][0]]);

      if (plane.distanceToPoint(frustumCenter) < 0.0f)
         plane = plane.flipped();

      if (mNumInterestVolumePlanes == cMaxInterestVolumePlanes)
         break;

      mDirLightInterestVolume[mNumInterestVolumePlanes] = XMLoadFloat4((XMFLOAT4*)plane.equation().getPtr());            
      mNumInterestVolumePlanes++;
   }

   XMVECTOR v = XMVectorSet(0.0f, 0.0f, 0.0f, maxLocalLightRadius);
   for (uint i = 0; i < mNumInterestVolumePlanes; i++)
   {
      mAllLightInterestVolume[i] = mDirLightInterestVolume[i] + v;
   }
}
#endif

void BInterestVolume::computeConvexHull(BPlaneArray& planes, const BVec3Array& points)
{
   BStaticArray<WORD, 256> combinations;
   uint numCombinations = GenerateCombinations(combinations, points.getSize(), 3);
   
   planes.resize(0);
   
   for (uint combinationIndex = 0; combinationIndex < numCombinations; combinationIndex++)
   {
      const uint p1i = combinations[combinationIndex*3+0];
      const uint p2i = combinations[combinationIndex*3+1];
      const uint p3i = combinations[combinationIndex*3+2];
      const BVec3& p1 = points[p1i];
      const BVec3& p2 = points[p2i];
      const BVec3& p3 = points[p3i];
      
      PlaneD p;
      if (!p.setFromTriangle(p1, p2, p3, .00000125f))
         continue;
      
      uint pointIndex;
      double minVal = Math::fNearlyInfinite;
      double maxVal = -Math::fNearlyInfinite;
      for (pointIndex = 0; pointIndex < points.getSize(); pointIndex++)
      {
         if ((pointIndex == p1i) || (pointIndex == p2i) || (pointIndex == p3i))
            continue;
            
         const BVec3& point = points[pointIndex];
         
         double dist = p.distanceToPoint(point);
         minVal = Math::Min(minVal, dist);
         maxVal = Math::Max(maxVal, dist);
      }         
      
      if ((minVal >= -Math::fSmallEpsilon) && (maxVal <= Math::fSmallEpsilon))
         continue;
      
      if ((maxVal <= Math::fSmallEpsilon) || (minVal >= -Math::fSmallEpsilon))
      {
         if (maxVal <= Math::fSmallEpsilon)
            p.flip();
            
         uint planeIndex;
         for (planeIndex = 0; planeIndex < planes.getSize(); planeIndex++)
         {
            const Plane& otherP = planes[planeIndex];
            double a = p.n * BVec3D(otherP.n);
            double b = fabs(p.d - otherP.d);
            if ((a >= .9999f) && (b <= fabs(p.d) * .000125f))
               break;
         }
         if (planeIndex < planes.getSize())
            continue;
            
         Plane fp;
         fp.n = p.n;
         fp.d = static_cast<float>(p.d);
         
         planes.pushBack(fp);
      }         
   }
}

void BInterestVolume::update(const BMatrixTracker& matrixTracker, const BVec3& lightDir, float maxLocalLightRadius, float worldMinY, float worldMaxY)
{
   // rg [7/24/06] - I think it would probably be better to approximate all this crap in 2D.
   
   const BFrustum& worldFrustum = matrixTracker.getWorldFrustum();

   Plane planes[8];
   Utils::FastMemCpy(planes, &worldFrustum.plane(0), 6 * sizeof(Plane));
   planes[6].setFromNormalOrigin(BVec3(0.0f, 1.0f, 0.0f), BVec3(0.0f, worldMinY, 0.0f));
   planes[7].setFromNormalOrigin(BVec3(0.0f, -1.0f, 0.0f), BVec3(0.0f, worldMaxY, 0.0f));
   
   BVec3Array frustumVerts;
   calcVolumeIntersection<BVec3Array, 256>(frustumVerts, planes, 8);
   if (frustumVerts.isEmpty())
   {
      XMVECTOR frustumVerticesVecs[8];

      matrixTracker.getFrustumVertices(frustumVerticesVecs);

      const BVec4* pFrustumVerticesVec4 = reinterpret_cast<const BVec4*>(frustumVerticesVecs);
      
      frustumVerts.resize(8);
      for (uint planeIndex = 0; planeIndex < 8; planeIndex++)
         frustumVerts[planeIndex] = pFrustumVerticesVec4[planeIndex];
   }
   
   const BVec3 topPoint(matrixTracker.getWorldCamPosVec4() - lightDir * 1024.0f);
   
   frustumVerts.pushBack(topPoint);

   BPlaneArray hullPlanes;
   computeConvexHull(hullPlanes, frustumVerts);
   
   if (hullPlanes.isEmpty())  
   {
      hullPlanes.resize(BFrustum::cPlaneMax);
      for (uint planeIndex = 0; planeIndex < BFrustum::cPlaneMax; planeIndex++)
         hullPlanes[planeIndex] = worldFrustum.plane(planeIndex);
   }
 
   mNumInterestVolumePlanes = Math::Min(16U, hullPlanes.getSize());
   
   XMVECTOR v = XMVectorSet(0.0f, 0.0f, 0.0f, maxLocalLightRadius);
   
   for (uint planeIndex = 0; planeIndex < mNumInterestVolumePlanes; planeIndex++)
      mDirLightInterestVolume[planeIndex] = XMLoadFloat4(reinterpret_cast<const XMFLOAT4*>(hullPlanes[planeIndex].equation().getPtr()));

   for (uint planeIndex = 0; planeIndex < mNumInterestVolumePlanes; planeIndex++)
      mAllLightInterestVolume[planeIndex] = mDirLightInterestVolume[planeIndex] + v;
}



