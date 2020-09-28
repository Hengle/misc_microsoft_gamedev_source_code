//==============================================================================
// Copyright (c) 1997 Ensemble Studios
//
// Point in XZ projection of polygon test function
//==============================================================================

#include "xsystem.h"
#include "pointin.h"


//==============================================================================
// pointInXZProjection
//==============================================================================
bool pointInXZProjection(const BVector *vertices, const long numVertices, const BVector &v)
{
   bool result=false;
   for (long i = 0, j = numVertices-1; i < numVertices; j = i++) 
   {
      float xi = vertices[i].x;
      float zi = vertices[i].z;
      float xj = vertices[j].x;
      float zj = vertices[j].z;
      if ((((zi<=v.z) && (v.z<zj)) ||
         ((zj<=v.z) && (v.z<zi))) &&
         (v.x < (xj - xi) * (v.z - zi) / (zj - zi) + xi))
            result = !result;      
   }
   return result;
}


//==============================================================================
// pointInXZProjection
//==============================================================================
bool pointInXZProjection(const BVector *vertices, const long *indices, const long numVertices, const BVector &v)
{
   bool result=false;
   for (long i = 0, j = numVertices-1; i < numVertices; j = i++) 
   {
      float xi = vertices[indices[i]].x;
      float zi = vertices[indices[i]].z;
      float xj = vertices[indices[j]].x;
      float zj = vertices[indices[j]].z;
      if ((((zi<=v.z) && (v.z<zj)) ||
         ((zj<=v.z) && (v.z<zi))) &&
         (v.x < (xj - xi) * (v.z - zi) / (zj - zi) + xi))
            result = !result;      
   }
   return result;
}


//==============================================================================
// pointInTriangleXY
//
// Determines if the given point p is in the triangle defined by vert0-2.
// Uses the projection of the point and triangle on the XY plane.
//==============================================================================
bool pointInTriangleXY(const BVector *p, const BVector *vert0, const BVector *vert1, const BVector *vert2)
{
   float u0 = p->x - vert0->x;
   float v0 = p->y - vert0->y;

   float u1 = vert1->x - vert0->x;
   float v1 = vert1->y - vert0->y;

   float u2 = vert2->x - vert0->x;
   float v2 = vert2->y - vert0->y;

   float alpha, beta;
   if(fabs(u1) < cFloatCompareEpsilon)
   {
      beta = u0/u2;
      alpha = (v0-beta*v2)/v1;
   }
   else
   {
      beta = (v0*u1 - u0*v1)/(v2*u1 - u2*v1);
      alpha = (u0 - beta*u2)/u1;
   }

   if(alpha >= 0 && beta>=0 && (alpha+beta) <= 1)
      return true;
   else
      return false;
}


//==============================================================================
// pointInTriangleYZ
//
// Determines if the given point p is in the triangle defined by vert0-2.
// Uses the projection of the point and triangle on the YZ plane.
//==============================================================================
bool pointInTriangleYZ(const BVector *p, const BVector *vert0, const BVector *vert1, const BVector *vert2)
{
   float u0 = p->y - vert0->y;
   float v0 = p->z - vert0->z;

   float u1 = vert1->y - vert0->y;
   float v1 = vert1->z - vert0->z;

   float u2 = vert2->y - vert0->y;
   float v2 = vert2->z - vert0->z;

   float alpha, beta;
   if(fabs(u1) < cFloatCompareEpsilon)
   {
      beta = u0/u2;
      alpha = (v0-beta*v2)/v1;
   }
   else
   {
      beta = (v0*u1 - u0*v1)/(v2*u1 - u2*v1);
      alpha = (u0 - beta*u2)/u1;
   }

   if(alpha >= 0 && beta>=0 && (alpha+beta) <= 1)
      return true;
   else
      return false;
}



//==============================================================================
// pointInTriangleXZ
//
// Determines if the given point p is in the triangle defined by vert0-2.
// Uses the projection of the point and triangle on the XZ plane.
//==============================================================================
bool pointInTriangleXZ(const BVector *p, const BVector *vert0, const BVector *vert1, const BVector *vert2)
{
   float u0 = p->x - vert0->x;
   float v0 = p->z - vert0->z;

   float u1 = vert1->x - vert0->x;
   float v1 = vert1->z - vert0->z;

   float u2 = vert2->x - vert0->x;
   float v2 = vert2->z - vert0->z;

   float alpha, beta;
   if(fabs(u1) < cFloatCompareEpsilon)
   {
      beta = u0/u2;
      alpha = (v0-beta*v2)/v1;
   }
   else
   {
      beta = (v0*u1 - u0*v1)/(v2*u1 - u2*v1);
      alpha = (u0 - beta*u2)/u1;
   }

   if(alpha >= 0 && beta>=0 && (alpha+beta) <= 1)
      return true;
   else
      return false;
}


//==============================================================================
// pointInTriangleXZ
//==============================================================================
bool pointInTriangleXZ(const BVector& p, const BVector& vect0, const BVector& vect1, const BVector& vect2)
{
   float u0 = p.x - vect0.x;
   float v0 = p.z - vect0.z;

   float u1 = vect1.x - vect0.x;
   float v1 = vect1.z - vect0.z;

   float u2 = vect2.x - vect0.x;
   float v2 = vect2.z - vect0.z;

   float alpha, beta;
   if(fabs(u1) < cFloatCompareEpsilon)
   {
      beta = u0/u2;
      alpha = (v0-beta*v2)/v1;
   }
   else
   {
      beta = (v0*u1 - u0*v1)/(v2*u1 - u2*v1);
      alpha = (u0 - beta*u2)/u1;
   }

   if(alpha >= 0 && beta>=0 && (alpha+beta) <= 1)
      return true;
   else
      return false;
}


//==============================================================================
// pointInXYProjection
//==============================================================================
bool pointInXYProjection(const BVector *vertices, const long numVertices, const BVector &v)
{
   bool result=false;
   for (long i = 0, j = numVertices-1; i < numVertices; j = i++) 
   {
      float xi = vertices[i].x;
      float yi = vertices[i].y;
      float xj = vertices[j].x;
      float yj = vertices[j].y;
      if ((((yi<=v.y) && (v.y<yj)) ||
         ((yj<=v.y) && (v.y<yi))) &&
         (v.x < (xj - xi) * (v.y - yi) / (yj - yi) + xi))
            result = !result;      
   }
   return result;
}


//==============================================================================
// pointInYZProjection
//==============================================================================
bool pointInYZProjection(const BVector *vertices, const long numVertices, const BVector &v)
{
   bool result=false;
   for (long i = 0, j = numVertices-1; i < numVertices; j = i++) 
   {
      float yi = vertices[i].y;
      float zi = vertices[i].z;
      float yj = vertices[j].y;
      float zj = vertices[j].z;
      if ((((zi<=v.z) && (v.z<zj)) ||
         ((zj<=v.z) && (v.z<zi))) &&
         (v.y < (yj - yi) * (v.z - zi) / (zj - zi) + yi))
            result = !result;      
   }
   return result;
}


