//============================================================================
//
//  Frustum.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "TerrainPCH.h"
#include "Frustum.h"
#include "mathlib.h"

// xcore
#include "math\vector.h"

//xrender
#include "renderThread.h"
#include "renderDraw.h"

#if 0

D3DXPLANE m_planes[6];

void BTerrainFrustum::update()
{
   ASSERT_WORKER_THREAD
      D3DXMATRIX res;
      res=*(D3DXMATRIX*)&gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj);
      
         // left
         m_planes[0].a = res._14 + res._11;
         m_planes[0].b = res._24 + res._21;
         m_planes[0].c = res._34 + res._31;
         m_planes[0].d = res._44 + res._41;

         // right
         m_planes[1].a = res._14 - res._11;
         m_planes[1].b = res._24 - res._21;
         m_planes[1].c = res._34 - res._31;
         m_planes[1].d = res._44 - res._41;

         // top
         m_planes[2].a = res._14 - res._12;
         m_planes[2].b = res._24 - res._22;
         m_planes[2].c = res._34 - res._32;
         m_planes[2].d = res._44 - res._42;

         // bottom
         m_planes[3].a = res._14 + res._12;
         m_planes[3].b = res._24 + res._22;
         m_planes[3].c = res._34 + res._32;
         m_planes[3].d = res._44 + res._42;

         // near
         m_planes[4].a = res._13;
         m_planes[4].b = res._23;
         m_planes[4].c = res._33;
         m_planes[4].d = res._43;

         // far
         m_planes[5].a = res._14 - res._13;
         m_planes[5].b = res._24 - res._23;
         m_planes[5].c = res._34 - res._33;
         m_planes[5].d = res._44 - res._43;

         for(int i=0; i < 6; i++)
         {
            D3DXPlaneNormalize(&m_planes[i], &m_planes[i]);
         }
   
}
//------------------------------------------------
bool  BTerrainFrustum::AABBVisible(const D3DXVECTOR3 min,const D3DXVECTOR3 max)
{
   D3DXVECTOR3 c = (D3DXVECTOR3((float*)(&max))+D3DXVECTOR3((float*)(&min)) ) / 2.f;
   D3DXVECTOR3 d = D3DXVECTOR3((float*)(&max)) - c ;    // half-diagonal

   for(int i=0;i<6;i++)
   {
      D3DXVECTOR3 p;
      p.x=m_planes[i][0];
      p.y=m_planes[i][1];
      p.z=m_planes[i][2];

      float NP = (float)(d.x*fabs(p.x) + d.y*fabs(p.y) + d.z*fabs(p.z));

      float MP = c.x*p.x + c.y*p.y + c.z*p.z + m_planes[i][3];

      if ((MP+NP) < 0.0f) return false; // behind clip plane
   }

   return true;
}
//--------------------------------------------------
bool  BTerrainFrustum::SphereVisible(const D3DXVECTOR3 center, const float rad)
{
   // various distances
   float fDistance;

   // calculate our distances to each of the planes
   for(int i = 0; i < 6; ++i) 
   {
      // find the distance to this plane
      fDistance = m_planes[i].a*center.x + m_planes[i].b*center.y + m_planes[i].c*center.z + m_planes[i].d;

      // if this distance is < -sphere.radius, we are outside
      if(fDistance < -rad)
         return(false);

      // else if the distance is between +- radius, then we intersect
      if((float)fabs(fDistance) < rad)
         return(true);
   }

   // otherwise we are fully in view
   return(true);
}
#endif