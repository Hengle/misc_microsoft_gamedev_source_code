//============================================================================
//
//  volumeCuller.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//  rg [2/23/06] - Initial implementation.
//============================================================================
#include "xrender.h"
#include "volumeCuller.h"
#include "math\VMXIntersection.h"
#include "volumeIntersection.h"

//============================================================================
// BVolumeCuller::BVolumeCuller
//============================================================================
BVolumeCuller::BVolumeCuller()
{
   clear();
}

//============================================================================
// BVolumeCuller::BVolumeCuller
//============================================================================
BVolumeCuller::BVolumeCuller(const BVolumeCuller& other)
{
   if (this != &other)
      Utils::FastMemCpy(this, &other, sizeof(BVolumeCuller));
}

//============================================================================
// BVolumeCuller::operator=
//============================================================================
BVolumeCuller& BVolumeCuller::operator= (const BVolumeCuller& rhs)
{
   if (this != &rhs)
      Utils::FastMemCpy(this, &rhs, sizeof(BVolumeCuller));
      
   return *this;      
}

//============================================================================
// BVolumeCuller::clear
//============================================================================
void BVolumeCuller::clear(void)
{
   Utils::ClearObj(mBasePlanes);
   Utils::ClearObj(mExclusionPlanes);
   Utils::ClearObj(mInclusionPlanes);
   Utils::ClearObj(mBaseMin);
   Utils::ClearObj(mBaseMax);
   
   mUseExclusionPlanes = false;
   mNumInclusionPlanes = 0;
   mUseBasePlanes = true;
}

//============================================================================
// BVolumeCuller::setBasePlanes
//============================================================================
void BVolumeCuller::setBasePlanes(const BFrustum& frustum, XMMATRIX projMatrix, bool computeBounds)
{
   setBasePlanes(frustum, false);

   if (computeBounds)
   {
      XMVECTOR verts[8];
      BVMXIntersection::computeFrustumVertices(verts, projMatrix);
      BVMXIntersection::computeBoundsOfPoints(verts, 8, &mBaseMin, &mBaseMax);
      //trace("setBasePlanes: %u %f %f %f, %f %f %f", 8, mBaseMin.x, mBaseMin.y, mBaseMin.z, mBaseMax.x, mBaseMax.y, mBaseMax.z);
   }      
}

//============================================================================
// BVolumeCuller::setBasePlanes
//============================================================================
void BVolumeCuller::setBasePlanes(XMMATRIX projMatrix, bool computeBounds)
{
   BFrustum frustum;
   frustum.set(projMatrix);
   
   setBasePlanes(frustum, projMatrix, computeBounds);
}

//============================================================================
// BVolumeCuller::setBasePlanes
//============================================================================
void BVolumeCuller::setBasePlanes(XMVECTOR min, XMVECTOR max)
{
   // rg [6/27/07] - Not tested yet
   mBasePlanes[0] = XMVectorSet(-1.0f, 0.0f, 0.0f, max.x);
   mBasePlanes[1] = XMVectorSet( 1.0f, 0.0f, 0.0f, -min.x);
   
   mBasePlanes[2] = XMVectorSet( 0.0f, -1.0f, 0.0f, max.y);
   mBasePlanes[3] = XMVectorSet( 0.0f,  1.0f, 0.0f, -min.y);
   
   mBasePlanes[4] = XMVectorSet( 0.0f,  0.0f, -1.0f, max.z);
   mBasePlanes[5] = XMVectorSet( 0.0f,  0.0f,  1.0f, -min.z);
   
   mBaseMin = min;
   mBaseMax = max;
}

//============================================================================
// BVolumeCuller::computeBaseBounds
//============================================================================
void BVolumeCuller::computeBaseBounds(void)
{
   Plane planes[cMaxBasePlanes];
   for (uint i = 0; i < cMaxBasePlanes; i++)
      planes[i].setFromEquation(*reinterpret_cast<const BVecN<4, float>*>(&mBasePlanes[i]));
   
   typedef BStaticArray<BVec3, 16> BVec3Array;
   BVec3Array frustumVerts;
   calcVolumeIntersection<BVec3Array, 256>(frustumVerts, planes, cMaxBasePlanes);
         
   if (frustumVerts.isEmpty())
   {
      Utils::ClearObj(mBaseMin);
      Utils::ClearObj(mBaseMax);
      //trace("computeBasePlanes: %u %f %f %f, %f %f %f", frustumVerts.getSize(), mBaseMin.x, mBaseMin.y, mBaseMin.z, mBaseMax.x, mBaseMax.y, mBaseMax.z);
      return;  
   }
   
   BVec3 minP(frustumVerts[0]);
   BVec3 maxP(frustumVerts[0]);
   for (uint i = 1; i < frustumVerts.size(); i++)
   {
      minP = BVec3::elementMin(minP, frustumVerts[i]);
      maxP = BVec3::elementMax(maxP, frustumVerts[i]);
   }
   
   mBaseMin = XMVectorSet(minP[0], minP[1], minP[2], 1.0f);
   mBaseMax = XMVectorSet(maxP[0], maxP[1], maxP[2], 1.0f);
   //trace("computeBasePlanes: %u %f %f %f, %f %f %f", frustumVerts.getSize(), mBaseMin.x, mBaseMin.y, mBaseMin.z, mBaseMax.x, mBaseMax.y, mBaseMax.z);
}

//============================================================================
// BVolumeCuller::setBasePlanes
//============================================================================
void BVolumeCuller::setBasePlanes(const BFrustum& frustum, bool computeBounds)
{
   for (uint i = 0; i < 6; i++)
      mBasePlanes[i] = XMLoadFloat4((const XMFLOAT4*)frustum.plane(i).equation().getPtr());
   
   if (computeBounds)
      computeBaseBounds();
}

//============================================================================
// BVolumeCuller::setBasePlanes
//============================================================================
void BVolumeCuller::setBasePlanes(XMVECTOR* pPlanes, bool computeBounds)
{
   BDEBUG_ASSERT(pPlanes);
   Utils::FastMemCpy(mBasePlanes, pPlanes, sizeof(XMVECTOR) * cMaxBasePlanes);
   
   if (computeBounds)
      computeBaseBounds();
}

//============================================================================
// BVolumeCuller::isSphereVisible
//============================================================================
bool BVolumeCuller::isSphereVisible(XMVECTOR center, float radius) const
{
   if (mUseBasePlanes)
   {
      if (BVMXIntersection::checkSphereForRejectionFrustum(mBasePlanes, center, radius))
         return false;
   }         
      
   if (mNumInclusionPlanes)
   {
      if (BVMXIntersection::checkSphereForRejectionVolume(mInclusionPlanes, mNumInclusionPlanes, center, radius))
         return false;
   }
   
   if (mUseExclusionPlanes)
   {
      if (BVMXIntersection::checkSphereForInclusionFrustum(mExclusionPlanes, center, radius))
         return false;
   }
     
   return true;
}

//============================================================================
// BVolumeCuller::isSphereVisible
//============================================================================
bool BVolumeCuller::isSphereVisible(XMVECTOR center, XMVECTOR radius) const
{
   if (mUseBasePlanes)
   {
      if (BVMXIntersection::checkSphereForRejectionFrustum(mBasePlanes, center, radius))
         return false;
   }         
      
   if (mNumInclusionPlanes)
   {
      if (BVMXIntersection::checkSphereForRejectionVolume(mInclusionPlanes, mNumInclusionPlanes, center, radius))
         return false;
   }
   
   if (mUseExclusionPlanes)
   {
      if (BVMXIntersection::checkSphereForInclusionFrustum(mExclusionPlanes, center, radius))
         return false;
   }
     
   return true;
}

//============================================================================
// BVolumeCuller::isAABBVisible
//============================================================================
bool BVolumeCuller::isAABBVisible(const D3DXVECTOR3& min, const D3DXVECTOR3& max) const
{
   XMVECTOR xmin = XMLoadFloat3((const XMFLOAT3*)&min);
   XMVECTOR xmax = XMLoadFloat3((const XMFLOAT3*)&max);

   XMVECTOR c = XMVectorMultiply(XMVectorAdd(xmax, xmin), gXMOneHalf);
   XMVECTOR d = XMVectorSubtract(xmax, c);
   
   return isAABBVisible(c, d);
}

//============================================================================
// BVolumeCuller::isAABBVisibleBounds
//============================================================================
bool BVolumeCuller::isAABBVisibleBounds(XMVECTOR min, XMVECTOR max) const
{
   XMVECTOR c = XMVectorMultiply(XMVectorAdd(max, min), gXMOneHalf);
   XMVECTOR d = XMVectorSubtract(max, c);  
   return isAABBVisible(c, d);
}

//============================================================================
// BVolumeCuller::isAABBVisible
//============================================================================
bool BVolumeCuller::isAABBVisible(XMVECTOR c, XMVECTOR d) const      
{
   //c = __vrlimi(c, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
   //d = __vrlimi(d, XMVectorZero(), VRLIMI_CONST(0, 0, 0, 1), 0);
   
   if (mUseBasePlanes)
   {
      if (!BVMXIntersection::AABBTouchesFrustum(mBasePlanes, c, d))
         return false;
   }
   
   if (mNumInclusionPlanes)
   {
      if (!BVMXIntersection::AABBTouchesVolume(mInclusionPlanes, mNumInclusionPlanes, c, d))
         return false;
   }

   if (mUseExclusionPlanes)
   {
      if (BVMXIntersection::AABBInsideFrustum(mExclusionPlanes, c, d))
         return false;
   }   
      
   return true;
}

//============================================================================
// BVolumeCuller::enableExclusionPlanes
//============================================================================
void BVolumeCuller::enableExclusionPlanes(const BFrustum& frustum)
{
   for (uint i = 0; i < 6; i++)
      mExclusionPlanes[i] = XMLoadFloat4((XMFLOAT4*)frustum.plane(i).equation().getPtr());
      
   mUseExclusionPlanes = true;      
}

//============================================================================
// BVolumeCuller::enableInclusionPlanes
//============================================================================
void BVolumeCuller::enableInclusionPlanes(const BFrustum& frustum)
{
   for (uint i = 0; i < 6; i++)
      mInclusionPlanes[i] = XMLoadFloat4((XMFLOAT4*)frustum.plane(i).equation().getPtr());

   mNumInclusionPlanes = 6;
}


//============================================================================
// BVolumeCuller::enableInclusionPlanes
//============================================================================
void BVolumeCuller::enableInclusionPlanes(const XMVECTOR* pPlanes, uint numPlanes)
{
   BDEBUG_ASSERT((numPlanes > 0) && (numPlanes <= cMaxInclusionPlanes));
   
   Utils::FastMemCpy(mInclusionPlanes, pPlanes, numPlanes * sizeof(XMVECTOR));
   
   mNumInclusionPlanes = static_cast<uchar>(numPlanes);
}

//============================================================================
// BVolumeCuller::getSerializeSize
//============================================================================
uint BVolumeCuller::getSerializeSize(void) const
{
   uint size = sizeof(DWORD) + sizeof(XMVECTOR) * cMaxBasePlanes + sizeof(XMVECTOR) * 2;
   
   if (mUseExclusionPlanes)
      size += sizeof(XMVECTOR) * cMaxExclusionPlanes;
      
   size += sizeof(XMVECTOR) * mNumInclusionPlanes;
   
   return size;
}

namespace
{
   template<class T>
   void writeObj(uchar* RESTRICT & pDst, const T& obj)
   {
      memcpy(pDst, &obj, sizeof(obj));
      pDst += sizeof(obj);
   }

   template<class T>
   void readObj(const uchar* RESTRICT & pSrc, T& obj)
   {
      memcpy(&obj, pSrc, sizeof(obj));
      pSrc += sizeof(obj);
   }
}

//============================================================================
// BVolumeCuller::serialize
//============================================================================
void BVolumeCuller::serialize(uchar* RESTRICT pDst) const
{
   const DWORD flags = mNumInclusionPlanes | (mUseBasePlanes << 8) | (mUseExclusionPlanes << 9);
   writeObj(pDst, flags);
   
   writeObj(pDst, mBasePlanes);
   writeObj(pDst, mBaseMin);
   writeObj(pDst, mBaseMax);
   
   if (mUseExclusionPlanes)
      writeObj(pDst, mExclusionPlanes);
      
   if (mNumInclusionPlanes)      
      memcpy(pDst, mInclusionPlanes, sizeof(XMVECTOR) * mNumInclusionPlanes);
}

//============================================================================
// BVolumeCuller::deserialize
//============================================================================
uint BVolumeCuller::deserialize(const uchar* RESTRICT pSrc)
{
   const uchar* RESTRICT pOrigSrc = pSrc;
   
   DWORD flags;
   readObj(pSrc, flags);
   
   mNumInclusionPlanes = (uchar)flags;
   BDEBUG_ASSERT(mNumInclusionPlanes <= cMaxInclusionPlanes);
   mUseBasePlanes = (flags & 256) != 0;
   mUseExclusionPlanes = (flags & 512) != 0;
   
   readObj(pSrc, mBasePlanes);
   readObj(pSrc, mBaseMin);
   readObj(pSrc, mBaseMax);
   
   if (mUseExclusionPlanes)
      readObj(pSrc, mExclusionPlanes);
   
   if (mNumInclusionPlanes)
   {
      memcpy(mInclusionPlanes, pSrc, sizeof(XMVECTOR) * mNumInclusionPlanes);
      pSrc += sizeof(XMVECTOR) * mNumInclusionPlanes;
   }
      
   return pSrc - pOrigSrc;      
}
