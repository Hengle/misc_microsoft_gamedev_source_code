//============================================================================
//
//  mathlib.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "TerrainPCH.h"

#include "mathlib.h"

//-----------------------------------------------------------------------------

bool raySegmentIntersectionTriangle(const D3DXVECTOR3 vertex[3], const D3DXVECTOR3 &origin, const D3DXVECTOR3 &direction, 
                                    const bool segment, D3DXVECTOR3 &iPoint)
{
   //   if (segment)
   
      // Moller/Trumbore method -- supposedly quick without requiring the normal.
      // Also computes the barycentric coordinates of the intersection point if we wanted to give 
      // those back for some reason.

      // Get vectors along the two triangle edges sharing vertex 0.
      D3DXVECTOR3 edge1;
      edge1 = vertex[1]-vertex[0];
      D3DXVECTOR3 edge2;
      edge2 = vertex[2]-vertex[0];

      // Calc determinant.
      D3DXVECTOR3 pvec;
      D3DXVec3Cross(&pvec,&direction, &edge2);
      float det=D3DXVec3Dot(&edge1,&pvec);

      // If determinant is near, the ray is in the plane of the triangle.  In that case there is no intersection.
      if((det > -cFloatCompareEpsilon) && (det < cFloatCompareEpsilon))
         return(false);

      // Get reciprocal.
      float recipDet=1.0f/det;

      // Calc dist from vertex 0 to origin
      D3DXVECTOR3 tvec;
      tvec =origin- vertex[0];

      // Get u param of barycentric coords.
      float u=D3DXVec3Dot(&tvec,&pvec)*recipDet;

      // See if it's inside triangle.
      if(u<-cFloatCompareEpsilon || u>1.0f+cFloatCompareEpsilon)
         return(false);

      // Calc vector for v portion.
      D3DXVECTOR3 qvec;
      D3DXVec3Cross(&qvec,&tvec,&edge1);

      // Calc v.
      float v=D3DXVec3Dot(&direction,&qvec)*recipDet;

      // See if it's inside triangle.
      if(v<-cFloatCompareEpsilon || u+v>1.0f+cFloatCompareEpsilon)
         return(false);

      // Compute t.
      float t=D3DXVec3Dot(&edge2,&qvec)*recipDet;

      // See if we're off the ray/segment.
      if(t<-cFloatCompareEpsilon || (segment && t>1.0f+cFloatCompareEpsilon))
         return(false);

      // Get intersection point.
      iPoint=origin + (t*direction);

      return(true);
}  

//-----------------------------------------------------------------------------
bool ray3AABB(D3DXVECTOR3& coord,
                 float& t,
                 const D3DXVECTOR3& rayOrig,
                 const D3DXVECTOR3& rayDir,
                 const D3DXVECTOR3& boxmin,
                 const D3DXVECTOR3& boxmax)
{
   enum 
   { 
      NumDim = 3, 
      Right = 0, 
      Left = 1, 
      Middle = 2 
   };

   bool inside = true;
   char quadrant[NumDim];
   float candidatePlane[NumDim];

   for (int i = 0; i < NumDim; i++)
   {
      if (rayOrig[i] < boxmin[i])
      {
         quadrant[i] = Left;
         candidatePlane[i] = boxmin[i];
         inside = false;
      }
      else if (rayOrig[i] > boxmax[i])
      {
         quadrant[i] = Right;
         candidatePlane[i] = boxmax[i];
         inside = false;
      }
      else
      {
         quadrant[i] = Middle;
      }
   }

   if (inside)
   {
      coord = rayOrig;
      t = 0.0f;
      return true;
   }

   float maxT[NumDim];
   for (int i = 0; i < NumDim; i++)
   {
      if ((quadrant[i] != Middle) && (rayDir[i] != 0.0f))
         maxT[i] = (candidatePlane[i] - rayOrig[i]) / rayDir[i];
      else
         maxT[i] = -1.0f;
   }         

   int whichPlane = 0;
   for (int i = 1; i < NumDim; i++)
      if (maxT[whichPlane] < maxT[i])
         whichPlane = i;

   if (maxT[whichPlane] < 0.0f)
      return false;               

   for (int i = 0; i < NumDim; i++)
   {
      if (i != whichPlane)
      {
         coord[i] = rayOrig[i] + maxT[whichPlane] * rayDir[i];

         if ( (coord[i] < boxmin[i]) || (coord[i] > boxmax[i]) )
         {
            return false;
         }
      }
      else
      {
         coord[i] = candidatePlane[i];
      }
   }               

   t = maxT[whichPlane];
   return true;
}

//-----------------------------------------------------------------------------
bool spheresIntersect(const D3DXVECTOR3 &center1,const float radius1, const D3DXVECTOR3 &center2,const float radius2)
{
   //-- Get distance between the centers.
   float dx          = center1.x - center2.x;
   float dy          = center1.y - center2.y;
   float dz          = center1.z - center2.z;
   float distanceSqr = (dx * dx) + (dy * dy) + (dz * dz);

   //-- Check the distance.
   float combinedRadius = radius1 + radius2;
   return (distanceSqr <= (combinedRadius * combinedRadius));
}
//-----------------------------------------------------------------------------
bool aabbsIntersect(const D3DXVECTOR3 &aMin,const D3DXVECTOR3 &aMax,const D3DXVECTOR3 &bMin,const D3DXVECTOR3 &bMax)
{
   if ((bMax.x < aMin.x) || (bMax.y < aMin.y) || (bMax.z < aMin.z) ||
       (bMin.x > aMax.x) || (bMin.y > aMax.y) || (bMin.z > aMax.z))
      return false;
      
   return true;
}
//-----------------------------------------------------------------------------
bool pointBoxIntersect(const D3DXVECTOR3 &tA,const D3DXVECTOR3 &tB,const D3DXVECTOR3 &tC,const D3DXVECTOR3 &tD,
                       const D3DXVECTOR3 &bA,const D3DXVECTOR3 &bB,const D3DXVECTOR3 &bC,const D3DXVECTOR3 &bD,
                       const D3DXVECTOR3 &pt)
{
   D3DXPLANE m_planes[6];

   D3DXPlaneFromPoints(&m_planes[0],&tA,&tC,&tB);
   D3DXPlaneFromPoints(&m_planes[1],&tA,&tB,&bA);
   D3DXPlaneFromPoints(&m_planes[2],&tA,&bA,&tC);
   D3DXPlaneFromPoints(&m_planes[3],&tB,&tD,&bB);
   D3DXPlaneFromPoints(&m_planes[4],&tD,&tC,&bD);
   D3DXPlaneFromPoints(&m_planes[5],&bA,&bB,&bC);

   for(int i=0; i < 6; i++)   
   {
      D3DXPlaneNormalize(&m_planes[i], &m_planes[i]);
      float fDistance = m_planes[i].a*pt.x + m_planes[i].b*pt.y + m_planes[i].c*pt.z + m_planes[i].d;

      // if this distance is < 0, we're on the backside
      if(fDistance < 0)
         return(false);
   }
   

   return true;
}
