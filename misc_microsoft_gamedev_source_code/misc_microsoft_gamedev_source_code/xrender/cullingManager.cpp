//============================================================================
//
//  cullingManager.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
// rg [2/23/06] - This class currently only handles perspective/parallel 
// projections. I plan on expanding it to handle nested frustums.//
//============================================================================
#include "xrender.h"
#include "cullingManager.h"

#define VRLIMI_CONST(x, y, z, w) (((x)<<3)|((y)<<2)|((z)<<1)|(w))

BCullingManager::BCullingManager() 
{
}

BCullingManager::BCullingManager(const BCullingManager& other)
{
   if (this != &other)
      Utils::FastMemCpy(this, &other, sizeof(BCullingManager));
}

BCullingManager& BCullingManager::operator= (const BCullingManager& rhs)
{
   if (this != &rhs)
      Utils::FastMemCpy(this, &rhs, sizeof(BCullingManager));
      
   return *this;      
}

void BCullingManager::clear(void)
{
   Utils::ClearObj(mBasePlanes);
   Utils::ClearObj(mExclusionPlanes);
   Utils::ClearObj(mInclusionPlanes);
   
   mUseExclusionPlanes = false;
   mNumInclusionPlanes = 0;
}

void BCullingManager::update(const BFrustum& frustum)
{
   for (uint i = 0; i < 6; i++)
      mBasePlanes[i] = XMLoadFloat4((XMFLOAT4*)frustum.plane(i).equation().getPtr());
}

bool BCullingManager::checkSphereForRejection6(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius) 
{
   center = __vrlimi(center, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
   XMVECTOR negRadius = XMVectorNegate(XMVectorReplicate(radius));

   XMVECTOR dist0 = XMVector4Dot(center, pPlanes[0]);
   XMVECTOR dist1 = XMVector4Dot(center, pPlanes[1]);
   XMVECTOR dist2 = XMVector4Dot(center, pPlanes[2]);
   XMVECTOR dist3 = XMVector4Dot(center, pPlanes[3]);
   XMVECTOR dist4 = XMVector4Dot(center, pPlanes[4]);
   XMVECTOR dist5 = XMVector4Dot(center, pPlanes[5]);

   XMVECTOR minDist = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(dist0, dist1), dist2), dist3), dist4), dist5);

   XMVECTOR comp = XMVectorLess(minDist, negRadius);

   return (*(uint*)&comp.x) > 0;
}   

bool BCullingManager::checkSphereForRejection(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR center, float radius) 
{
   center = __vrlimi(center, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
   XMVECTOR negRadius = XMVectorNegate(XMVectorReplicate(radius));
   
   XMVECTOR minDist = XMVectorSplatOne();
   
   for (uint i = 0; i < numPlanes; i++)
   {
      XMVECTOR dist = XMVector4Dot(center, pPlanes[i]);

      minDist = XMVectorMin(minDist, dist);
   }      

   XMVECTOR comp = XMVectorLess(minDist, negRadius);

   return (*(uint*)&comp.x) > 0;
}   

bool BCullingManager::checkSphereForInclusion6(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius) 
{
   center = __vrlimi(center, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
   XMVECTOR xradius = XMVectorReplicate(radius);

   XMVECTOR dist0 = XMVector4Dot(center, pPlanes[0]);
   XMVECTOR dist1 = XMVector4Dot(center, pPlanes[1]);
   XMVECTOR dist2 = XMVector4Dot(center, pPlanes[2]);
   XMVECTOR dist3 = XMVector4Dot(center, pPlanes[3]);
   XMVECTOR dist4 = XMVector4Dot(center, pPlanes[4]);
   XMVECTOR dist5 = XMVector4Dot(center, pPlanes[5]);

   XMVECTOR minDist = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(dist0, dist1), dist2), dist3), dist4), dist5);

   XMVECTOR comp = XMVectorGreater(minDist, xradius);

   return (*(uint*)&comp.x) > 0;
}   

bool BCullingManager::isSphereVisible(XMVECTOR center, float radius) const
{
   if (checkSphereForRejection6(mBasePlanes, center, radius))
      return false;
      
   if (mNumInclusionPlanes)
   {
      if (checkSphereForRejection(mInclusionPlanes, mNumInclusionPlanes, center, radius))
         return false;
   }
   
   if (mUseExclusionPlanes)
   {
      if (checkSphereForInclusion6(mExclusionPlanes, center, radius))
         return false;
   }
     
   return true;
}

// c = center
// d = half-diagonol
// -1 = completely outside
// 0 = partial
// 1 = completely inside
int BCullingManager::AABBvsFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d)
{
   XMVECTOR NP0 = XMVector4Dot(d, XMVectorAbs(pPlanes[0]));
   XMVECTOR NP1 = XMVector4Dot(d, XMVectorAbs(pPlanes[1]));
   XMVECTOR NP2 = XMVector4Dot(d, XMVectorAbs(pPlanes[2]));
   XMVECTOR NP3 = XMVector4Dot(d, XMVectorAbs(pPlanes[3]));
   XMVECTOR NP4 = XMVector4Dot(d, XMVectorAbs(pPlanes[4]));
   XMVECTOR NP5 = XMVector4Dot(d, XMVectorAbs(pPlanes[5]));
   
   // MP = center distance from plane (positive = in front/inside)
   XMVECTOR MP0 = XMVector4Dot(c, pPlanes[0]);
   XMVECTOR MP1 = XMVector4Dot(c, pPlanes[1]);
   XMVECTOR MP2 = XMVector4Dot(c, pPlanes[2]);
   XMVECTOR MP3 = XMVector4Dot(c, pPlanes[3]);
   XMVECTOR MP4 = XMVector4Dot(c, pPlanes[4]);
   XMVECTOR MP5 = XMVector4Dot(c, pPlanes[5]);
   
   XMVECTOR a0 = XMVectorAdd(NP0, MP0);     
   XMVECTOR a1 = XMVectorAdd(NP1, MP1);     
   XMVECTOR a2 = XMVectorAdd(NP2, MP2);     
   XMVECTOR a3 = XMVectorAdd(NP3, MP3);     
   XMVECTOR a4 = XMVectorAdd(NP4, MP4);     
   XMVECTOR a5 = XMVectorAdd(NP5, MP5);     
   XMVECTOR minA = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(a0, a1), a2), a3), a4), a5);
   XMVECTOR compA = XMVectorLess(minA, XMVectorZero());
   
   XMVECTOR s0 = XMVectorSubtract(MP0, NP0);     
   XMVECTOR s1 = XMVectorSubtract(MP1, NP1);     
   XMVECTOR s2 = XMVectorSubtract(MP2, NP2);     
   XMVECTOR s3 = XMVectorSubtract(MP3, NP3);     
   XMVECTOR s4 = XMVectorSubtract(MP4, NP4);     
   XMVECTOR s5 = XMVectorSubtract(MP5, NP5);     
   
   XMVECTOR minS = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(s0, s1), s2), s3), s4), s5);
   XMVECTOR compS = XMVectorLess(minS, XMVectorZero());

   // trivial exclusion
   if ((*(uint*)&compA.x) > 0)
      return -1;
      
   // partial
   if ((*(uint*)&compS.x) > 0)
      return 0;
      
   // completely inside
   return 1;
               
#if 0      
   double NP = (float)(d.x*fabs(plane.x) + d.y*fabs(plane.y) + d.z*fabs(plane.z));
   double MP = c.x*plane.x + c.y*plane.y + c.z*plane.z + plane.w;
   // behind clip plane
   if ((MP+NP) < 0.0f) 
   {
      return -1; 
   }
#endif   
}   

int BCullingManager::AABBInsideFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d)
{
   XMVECTOR NP0 = XMVector4Dot(d, XMVectorAbs(pPlanes[0]));
   XMVECTOR NP1 = XMVector4Dot(d, XMVectorAbs(pPlanes[1]));
   XMVECTOR NP2 = XMVector4Dot(d, XMVectorAbs(pPlanes[2]));
   XMVECTOR NP3 = XMVector4Dot(d, XMVectorAbs(pPlanes[3]));
   XMVECTOR NP4 = XMVector4Dot(d, XMVectorAbs(pPlanes[4]));
   XMVECTOR NP5 = XMVector4Dot(d, XMVectorAbs(pPlanes[5]));

   // MP = center distance from plane (positive = in front/inside)
   XMVECTOR MP0 = XMVector4Dot(c, pPlanes[0]);
   XMVECTOR MP1 = XMVector4Dot(c, pPlanes[1]);
   XMVECTOR MP2 = XMVector4Dot(c, pPlanes[2]);
   XMVECTOR MP3 = XMVector4Dot(c, pPlanes[3]);
   XMVECTOR MP4 = XMVector4Dot(c, pPlanes[4]);
   XMVECTOR MP5 = XMVector4Dot(c, pPlanes[5]);

   XMVECTOR s0 = XMVectorSubtract(MP0, NP0);     
   XMVECTOR s1 = XMVectorSubtract(MP1, NP1);     
   XMVECTOR s2 = XMVectorSubtract(MP2, NP2);     
   XMVECTOR s3 = XMVectorSubtract(MP3, NP3);     
   XMVECTOR s4 = XMVectorSubtract(MP4, NP4);     
   XMVECTOR s5 = XMVectorSubtract(MP5, NP5);     

   XMVECTOR minS = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(s0, s1), s2), s3), s4), s5);
   XMVECTOR compS = XMVectorLess(minS, XMVectorZero());

   // partial
   if ((*(uint*)&compS.x) > 0)
      return 0;

   // completely inside
   return 1;

#if 0      
   double NP = (float)(d.x*fabs(plane.x) + d.y*fabs(plane.y) + d.z*fabs(plane.z));
   double MP = c.x*plane.x + c.y*plane.y + c.z*plane.z + plane.w;
   // behind clip plane
   if ((MP+NP) < 0.0f) 
   {
      return -1; 
   }
#endif   
}   

bool BCullingManager::AABBTouchesFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d)
{
   XMVECTOR NP0 = XMVector4Dot(d, XMVectorAbs(pPlanes[0]));
   XMVECTOR NP1 = XMVector4Dot(d, XMVectorAbs(pPlanes[1]));
   XMVECTOR NP2 = XMVector4Dot(d, XMVectorAbs(pPlanes[2]));
   XMVECTOR NP3 = XMVector4Dot(d, XMVectorAbs(pPlanes[3]));
   XMVECTOR NP4 = XMVector4Dot(d, XMVectorAbs(pPlanes[4]));
   XMVECTOR NP5 = XMVector4Dot(d, XMVectorAbs(pPlanes[5]));

   // MP = center distance from plane (positive = in front/inside)
   XMVECTOR MP0 = XMVector4Dot(c, pPlanes[0]);
   XMVECTOR MP1 = XMVector4Dot(c, pPlanes[1]);
   XMVECTOR MP2 = XMVector4Dot(c, pPlanes[2]);
   XMVECTOR MP3 = XMVector4Dot(c, pPlanes[3]);
   XMVECTOR MP4 = XMVector4Dot(c, pPlanes[4]);
   XMVECTOR MP5 = XMVector4Dot(c, pPlanes[5]);

   XMVECTOR a0 = XMVectorAdd(NP0, MP0);     
   XMVECTOR a1 = XMVectorAdd(NP1, MP1);     
   XMVECTOR a2 = XMVectorAdd(NP2, MP2);     
   XMVECTOR a3 = XMVectorAdd(NP3, MP3);     
   XMVECTOR a4 = XMVectorAdd(NP4, MP4);     
   XMVECTOR a5 = XMVectorAdd(NP5, MP5);     
   XMVECTOR minA = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(a0, a1), a2), a3), a4), a5);
   XMVECTOR compA = XMVectorLess(minA, XMVectorZero());

   // trivial exclusion
   if ((*(uint*)&compA.x) > 0)
      return false;

   return true;
}   

bool BCullingManager::AABBTouchesVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR c, XMVECTOR d)
{
   XMVECTOR minA = XMVectorSplatOne();
   
   for (uint i = 0; i < numPlanes; i++)
   {
      XMVECTOR NP = XMVector4Dot(d, XMVectorAbs(pPlanes[i]));
   
      // MP = center distance from plane (positive = in front/inside)
      XMVECTOR MP = XMVector4Dot(c, pPlanes[i]);
      
      XMVECTOR a = XMVectorAdd(NP, MP);     
      
      minA = XMVectorMin(minA, a);
   }
      
   XMVECTOR compA = XMVectorLess(minA, XMVectorZero());

   // trivial exclusion
   if ((*(uint*)&compA.x) > 0)
      return false;

   return true;
}   

bool BCullingManager::isAABBVisible(const D3DXVECTOR3& min, const D3DXVECTOR3& max) const
{
   XMVECTOR xmin = XMLoadFloat3((const XMFLOAT3*)&min);
   XMVECTOR xmax = XMLoadFloat3((const XMFLOAT3*)&max);

   XMVECTOR c = XMVectorMultiply(XMVectorAdd(xmax, xmin), XMVectorReplicate(.5f));
   XMVECTOR d = XMVectorSubtract(xmax, c);
   
   return isAABBVisible(c, d);
}

bool BCullingManager::isAABBVisible(XMVECTOR c, XMVECTOR d) const      
{
   c = __vrlimi(c, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
   d = __vrlimi(d, XMVectorZero(), VRLIMI_CONST(0, 0, 0, 1), 0);
   
   if (!AABBTouchesFrustum(mBasePlanes, c, d))
      return false;
 
   if (mNumInclusionPlanes)
   {
      if (!AABBTouchesVolume(mInclusionPlanes, mNumInclusionPlanes, c, d))
         return false;
   }

   if (mUseExclusionPlanes)
   {
      if (AABBInsideFrustum(mExclusionPlanes, c, d))
         return false;
   }   
      
   return true;
}

void BCullingManager::enableExclusionPlanes(const BFrustum& frustum)
{
   for (uint i = 0; i < 6; i++)
      mExclusionPlanes[i] = XMLoadFloat4((XMFLOAT4*)frustum.plane(i).equation().getPtr());
      
   mUseExclusionPlanes = true;      
}

void BCullingManager::enableInclusionPlanes(const XMVECTOR* pPlanes, uint numPlanes)
{
   BDEBUG_ASSERT((numPlanes > 0) && (numPlanes <= cMaxInclusionPlanes));
   
   Utils::FastMemCpy(mInclusionPlanes, pPlanes, numPlanes * sizeof(XMVECTOR));
   
   mNumInclusionPlanes = static_cast<uchar>(numPlanes);
}

