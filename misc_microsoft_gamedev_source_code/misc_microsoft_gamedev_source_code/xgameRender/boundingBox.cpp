//==============================================================================
// boundingbox.cpp
//
// Copyright (c) 1997, 1998, 1999 Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xgameRender.h"

#include "render.h"

#include "boundingbox.h"
#include "mathutil.h"
#include "pointin.h"

#include "plane.h"
#include "point.h"

//xcore
#include "math\fastFloat.h"

//==============================================================================
// Defines
//#define DEBUG_SCREENERRORINTERSECTION

// rg [1/18/06] - This class is not thread safe.

// Scratch space for transformed corners
BVector BBoundingBox::mTransformedCorners[8];

// Face arrays
//static const long cQuads[6][4] = { {0, 4, 5, 1}, {1, 5, 6, 2}, {2, 6, 7, 3}, {3, 7, 4, 0}, {0, 1, 2, 3}, {7, 6, 5, 4} };
static const DWORD cFaces[12][3] = { {4, 5, 6}, {4, 6, 7}, {0, 1, 2}, {0, 2, 3},
                        {4, 0, 3}, {4, 3, 7}, {5, 1, 2}, {5, 2, 6},
                        {4, 0, 1}, {4, 1, 5}, {7, 3, 2}, {7, 2, 6} };

/*
               1-----2
   y   z      /|    /|
   | /      0-----3  |
   |/       |  |  |  |  
   +---x    |  5--|--6
            | /   | /
            4-----7  
*/

//=============================================================================
// raySegmentIntersectsBox
//=============================================================================
bool raySegmentIntersectsBox(const BVector corners[8], const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float &intersectDistanceSqr)
{
   static const float cErrorEpsilon = 0.001f;
   long faceNum = 0;

   bool originInside=true;

   // For each direction
   BVector edge1;
   BVector edge2;
   BVector normal;
   float ooDen=0.0f;
   for(long i=0; i<3; i++)
   {
      // ray(t) = origin + direction*t and the equation of the triangles plane is
      // normal|p + d = 0  (where p is any point on the plane).
      // Thus at the intersection of the plane and the ray, t = -(d+N|origin)/(N|direction).

      // Get normal.
      long sEdgeVector1[3]={7, 0, 5};
      edge1.assignDifference(corners[sEdgeVector1[i]], corners[4]);
      long sEdgeVector2[3]={5, 7, 0};
      edge2.assignDifference(corners[sEdgeVector2[i]], corners[4]);
      normal.assignCrossProduct(edge1, edge2);

      // Correct for really short normals.
      float len = normal.x*normal.x + normal.y*normal.y + normal.z*normal.z;
      if(len < 0.01f)
         normal *= 100.0f;

      // Get denominator.
      float den = normal.dot(vector);

      // If the denominator is 0, then the ray is parallel to the plane so
      // there is no intersection.
      bool denOk;
      if(_fabs(den) > cFloatCompareEpsilon)
      {
         denOk=true;
         ooDen=1.0f/den;
      }
      else
         denOk=false;

      // Check the first plane.
      // Calculate d parameter of polygon's plane equation.
      float d = -(corners[4].dot(normal));
      // Get the numerator.
      float num = d + (normal.dot(origin));

      if(denOk)
      {
         // Calculate t.
         float t = -num*ooDen;
         // If t is less than 0, than the intersection is behind the origin of the ray, so 
         // there is no intersection.
         // TRB 5/8/08:  Changed check so points on one of the faces would be reported as an intersection
         if((t < -cFloatCompareEpsilon) || (segment && t>(1.0f+cFloatCompareEpsilon)))
            faceNum+=2;
         else
         {
            // Get the actual point of intersection of the ray and the plane.
            BVector vt(vector.x*t, vector.y*t, vector.z*t);
            float vtLengthSquared = vt.lengthSquared();
            if(!distanceSqr || vtLengthSquared < *distanceSqr)
            {
               BVector iPoint(origin.x+vt.x, origin.y+vt.y, origin.z+vt.z);
               // See if this point is inside either triangle.
               bool result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result)
               {
                  intersectDistanceSqr = vtLengthSquared;
                  return(true);
               }
               faceNum++;
               result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result)
               {
                  intersectDistanceSqr = vtLengthSquared;
                  return(true);
               }
               faceNum++;
            }
            else
               faceNum+=2;
         }
      }
      else
         faceNum+=2;

      // See if segment origin is inside or outside the box.
      // ajl 1/25/02 - need to do this for rays too (not just segments)
      //if(segment && num>0)
      if(num>0)
         originInside=false;

      // Check second plane.
      // Calculate d parameter of polygon's plane equation.
      long sDirectionVector[3] = {0,5,7};
      d = -(corners[sDirectionVector[i]].dot(normal));
      // Get the numerator.
      num = d + (normal.dot(origin));

      if(denOk)
      {
         // Calculate t.
         float t = -num*ooDen;
         // If t is less than 0, than the intersection is behind the origin of the ray, so 
         // there is no intersection.
         // TRB 5/8/08:  Changed check so points on one of the faces would be reported as an intersection
         if((t < -cFloatCompareEpsilon) || (segment && t>(1.0f+cFloatCompareEpsilon)))
            faceNum+=2;
         else
         {
            // Get the actual point of intersection of the ray and the plane.
            BVector vt(vector.x*t, vector.y*t, vector.z*t);
            float vtLengthSquared = vt.lengthSquared();
            if(!distanceSqr || vtLengthSquared < *distanceSqr)
            {
               BVector iPoint(origin.x+vt.x, origin.y+vt.y, origin.z+vt.z);
               // See if this point is inside either triangle.
               bool result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result)
               {
                  intersectDistanceSqr = vtLengthSquared;
                  return(true);
               }
               faceNum++;
               result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result)
               {
                  intersectDistanceSqr = vtLengthSquared;
                  return(true);
               }
               faceNum++;
            }
            else
               faceNum+=2;
         }
      }
      else
         faceNum+=2;

      // See if segment origin is inside or outside the box.
      // ajl 1/25/02 - need to do this for rays too (not just segments)
      //if(segment && num<0)
      if(num<0)
         originInside=false;
   }

   
   // If this was segment check and either endpoint was inside the box, we hit the box.
   // ajl 1/25/02 - need to do this for rays too (not just segments)
   //if(segment && originInside)
   if(originInside)
   {
      intersectDistanceSqr = 0.0f;
      return(true);
   }

   // If we got here, no intersection was found.
   return(false);
}

//=============================================================================
// raySegmentIntersectsBoxFirst
//
// MS 10/7/2008: I needed a function to return the first hit, not just any hit.
//=============================================================================
bool raySegmentIntersectsBoxFirst(const BVector corners[8], const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float &intersectDistanceSqr)
{
   static const float cErrorEpsilon = 0.001f;
   long faceNum = 0;

   bool originInside=true;

   // For each direction
   BVector edge1;
   BVector edge2;
   BVector normal;
   float ooDen=0.0f;
   float closestDistSqr = cMaximumFloat;
   bool foundHit = false;
   for(long i=0; i<3; i++)
   {
      // ray(t) = origin + direction*t and the equation of the triangles plane is
      // normal|p + d = 0  (where p is any point on the plane).
      // Thus at the intersection of the plane and the ray, t = -(d+N|origin)/(N|direction).

      // Get normal.
      long sEdgeVector1[3]={7, 0, 5};
      edge1.assignDifference(corners[sEdgeVector1[i]], corners[4]);
      long sEdgeVector2[3]={5, 7, 0};
      edge2.assignDifference(corners[sEdgeVector2[i]], corners[4]);
      normal.assignCrossProduct(edge1, edge2);

      // Correct for really short normals.
      float len = normal.x*normal.x + normal.y*normal.y + normal.z*normal.z;
      if(len < 0.01f)
         normal *= 100.0f;

      // Get denominator.
      float den = normal.dot(vector);

      // If the denominator is 0, then the ray is parallel to the plane so
      // there is no intersection.
      bool denOk;
      if(_fabs(den) > cFloatCompareEpsilon)
      {
         denOk=true;
         ooDen=1.0f/den;
      }
      else
         denOk=false;

      // Check the first plane.
      // Calculate d parameter of polygon's plane equation.
      float d = -(corners[4].dot(normal));
      // Get the numerator.
      float num = d + (normal.dot(origin));

      if(denOk)
      {
         // Calculate t.
         float t = -num*ooDen;
         // If t is less than 0, than the intersection is behind the origin of the ray, so 
         // there is no intersection.
         // TRB 5/8/08:  Changed check so points on one of the faces would be reported as an intersection
         if((t < -cFloatCompareEpsilon) || (segment && t>(1.0f+cFloatCompareEpsilon)))
            faceNum+=2;
         else
         {
            // Get the actual point of intersection of the ray and the plane.
            BVector vt(vector.x*t, vector.y*t, vector.z*t);
            float vtLengthSquared = vt.lengthSquared();
            if(!distanceSqr || vtLengthSquared < *distanceSqr)
            {
               BVector iPoint(origin.x+vt.x, origin.y+vt.y, origin.z+vt.z);
               // See if this point is inside either triangle.
               bool result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result && vtLengthSquared < closestDistSqr)
               {
                  closestDistSqr = vtLengthSquared;
                  foundHit = true;
                  //return(true);
               }
               faceNum++;
               result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result && vtLengthSquared < closestDistSqr)
               {
                  closestDistSqr = vtLengthSquared;
                  foundHit = true;
                  //return(true);
               }
               faceNum++;
            }
            else
               faceNum+=2;
         }
      }
      else
         faceNum+=2;

      // See if segment origin is inside or outside the box.
      // ajl 1/25/02 - need to do this for rays too (not just segments)
      //if(segment && num>0)
      if(num>0)
         originInside=false;

      // Check second plane.
      // Calculate d parameter of polygon's plane equation.
      long sDirectionVector[3] = {0,5,7};
      d = -(corners[sDirectionVector[i]].dot(normal));
      // Get the numerator.
      num = d + (normal.dot(origin));

      if(denOk)
      {
         // Calculate t.
         float t = -num*ooDen;
         // If t is less than 0, than the intersection is behind the origin of the ray, so 
         // there is no intersection.
         // TRB 5/8/08:  Changed check so points on one of the faces would be reported as an intersection
         if((t < -cFloatCompareEpsilon) || (segment && t>(1.0f+cFloatCompareEpsilon)))
            faceNum+=2;
         else
         {
            // Get the actual point of intersection of the ray and the plane.
            BVector vt(vector.x*t, vector.y*t, vector.z*t);
            float vtLengthSquared = vt.lengthSquared();
            if(!distanceSqr || vtLengthSquared < *distanceSqr)
            {
               BVector iPoint(origin.x+vt.x, origin.y+vt.y, origin.z+vt.z);
               // See if this point is inside either triangle.
               bool result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result && vtLengthSquared < closestDistSqr)
               {
                  closestDistSqr = vtLengthSquared;
                  foundHit = true;
                  //return(true);
               }
               faceNum++;
               result = pointInTriangle(iPoint, corners, cFaces[faceNum], normal, cErrorEpsilon);
               if(result && vtLengthSquared < closestDistSqr)
               {
                  closestDistSqr = vtLengthSquared;
                  foundHit = true;
                  //return(true);
               }
               faceNum++;
            }
            else
               faceNum+=2;
         }
      }
      else
         faceNum+=2;

      // See if segment origin is inside or outside the box.
      // ajl 1/25/02 - need to do this for rays too (not just segments)
      //if(segment && num<0)
      if(num<0)
         originInside=false;
   }


   // If this was segment check and either endpoint was inside the box, we hit the box.
   // ajl 1/25/02 - need to do this for rays too (not just segments)
   //if(segment && originInside)
   if(originInside)
   {
      intersectDistanceSqr = 0.0f;
      return(true);
   }

   // MS 10/7/2008: if we found a hit anywhere above, return closest squared dist
   if(foundHit)
   {
      intersectDistanceSqr = closestDistSqr;
      return true;
   }

   // If we got here, no intersection was found.
   return(false);
}

//=============================================================================
// BBoundingBox::BBoundingBox(void)
//=============================================================================
BBoundingBox::BBoundingBox(void)
{
} // BBoundingBox::BBoundingBox


//=============================================================================
// BBoundingBox::~BBoundingBox(void)
//=============================================================================
BBoundingBox::~BBoundingBox(void)
{
} // BBoundingBox::~BBoundingBox


//=============================================================================
// BBoundingBox::initialize(const BVector &minCorner, const BVector &maxCorner)
//
// Initializes the bounding box with the given corners.  minCorner should contain
// the minimum x, y, and z coordinates of the bounded area, and maxCorner should
// contain the maximum x, y, and z coordinates of the bounded area.
//=============================================================================
void BBoundingBox::initialize(const BVector &minCorner, const BVector &maxCorner)
{
   // Adjust corners to prevent degenerate boxes.
   BVector adjustedMin = minCorner;
   BVector adjustedMax = maxCorner;
   fixCorners(adjustedMin, adjustedMax);

#ifdef XBOX
   //CLM [04.31.08] removed LHS
   BVector delta;
   float* pmAxis0 = (float*)&mAxes[0];
   float* pmAxis1 = (float*)&mAxes[1];
   float* pmAxis2 = (float*)&mAxes[2];
   float* pmCenter = (float*)&mCenter;
   float* pDelta = (float*)&delta;
   const float* padjMin = (float*)&adjustedMin;
   const float* padjMax = (float*)&adjustedMax;

   

   // Compute center point.
   pmCenter[0] = (padjMin[0] + padjMax[0]) * 0.5f;
   pmCenter[1] = (padjMin[1] + padjMax[1]) * 0.5f;
   pmCenter[2] = (padjMin[2] + padjMax[2]) * 0.5f;
   pmCenter[3] = 0;
   
   //compute delta
      // Compute extents.
   mExtents[0] = pDelta[0] = pmCenter[0] - padjMin[0];
   mExtents[1] = pDelta[1] = pmCenter[1] - padjMin[1];
   mExtents[2] = pDelta[2] = pmCenter[2] - padjMin[2];


   // Axis aligned, so just use the x, y, z axes.
   pmAxis0[0] = 1;
   pmAxis0[1] = 0;
   pmAxis0[2] = 0;   
   pmAxis0[3] = 0;   
   pmAxis1[0] = 0;
   pmAxis1[1] = 1;
   pmAxis1[2] = 0;
   pmAxis1[3] = 0;   
   pmAxis2[0] = 0;
   pmAxis2[1] = 0;
   pmAxis2[2] = 1;
   pmAxis2[3] = 0;   

   // Radius of sphere enclosing the box.
   const XMVECTOR doubl = XMVector3Length(delta);
   XMStoreScalar(&mSphereRadius,doubl);

#else
   //origional

   // Compute center point.
   mCenter.set((adjustedMin.x+adjustedMax.x)*0.5f, (adjustedMin.y+adjustedMax.y)*0.5f, (adjustedMin.z+adjustedMax.z)*0.5f);

   // Compute extents.
   BVector delta(mCenter-adjustedMin);
   mExtents[0] = delta.x;
   mExtents[1] = delta.y;
   mExtents[2] = delta.z;

   // Axis aligned, so just use the x, y, z axes
   mAxes[0] = cXAxisVector;
   mAxes[1] = cYAxisVector;
   mAxes[2] = cZAxisVector;
   
   // Radius of sphere enclosing the box.
   mSphereRadius = delta.length();
   
#endif

} // BBoundingBox::initialize



//=============================================================================
// BBoundingBox::
//
// Initializes the bounding box with the given corners.  minCorner should contain
// the minimum x, y, and z coordinates of the bounded area, and maxCorner should
// contain the maximum x, y, and z coordinates of the bounded area.  The box is then
// transformed by the given matrix.
//=============================================================================
void BBoundingBox::initializeTransformed(const BVector &minCorner, const BVector &maxCorner, const BMatrix &matrix)
{
   // Initialize as AABB.
   initialize(minCorner, maxCorner);
   
   // Transform.
   transform(matrix);
} // BBoundingBox::initialize


//=============================================================================
// BBoundingBox::transform
//=============================================================================
void BBoundingBox::transform(const BMatrix &matrix)
{

#ifdef XBOX
   //CLM [05.02.08] Performance fixes (LHS, FCMP, etc)
   mCenter = XMVector3Transform(mCenter, matrix);

   const XMVECTOR v0 = XMVector3TransformNormal(mAxes[0],matrix);
   const XMVECTOR v1 = XMVector3TransformNormal(mAxes[1],matrix);
   const XMVECTOR v2 = XMVector3TransformNormal(mAxes[2],matrix);

   const XMVECTOR e0 = XMVectorReplicate(mExtents[0]);
   const XMVECTOR e1 = XMVectorReplicate(mExtents[1]);
   const XMVECTOR e2 = XMVectorReplicate(mExtents[2]);

   const XMVECTOR v0Len = XMVector3Length(v0);
   const XMVECTOR v1Len = XMVector3Length(v1);
   const XMVECTOR v2Len = XMVector3Length(v2);

   const XMVECTOR v0RcpLen = XMVectorReciprocal(v0Len);
   const XMVECTOR v1RcpLen = XMVectorReciprocal(v1Len);
   const XMVECTOR v2RcpLen = XMVectorReciprocal(v2Len);

   const XMVECTOR e0scl = XMVectorMultiply(e0, v0Len);
   const XMVECTOR e1scl = XMVectorMultiply(e1, v1Len);
   const XMVECTOR e2scl = XMVectorMultiply(e2, v2Len);

   const XMVECTOR v0Norm = XMVectorMultiply(v0,v0RcpLen);
   const XMVECTOR v1Norm = XMVectorMultiply(v1,v1RcpLen);
   const XMVECTOR v2Norm = XMVectorMultiply(v2,v2RcpLen);

   XMStoreScalar(&mExtents[0],e0scl);
   XMStoreScalar(&mExtents[1],e1scl);
   XMStoreScalar(&mExtents[2],e2scl);
  
   XMStoreFloat3((XMFLOAT3*)&mAxes[0],v0Norm);
   XMStoreFloat3((XMFLOAT3*)&mAxes[1],v1Norm);
   XMStoreFloat3((XMFLOAT3*)&mAxes[2],v2Norm);
   
#else
   // Scratch space.
   BVector temp[3];


   // Transform center.
   matrix.transformVectorAsPoint(mCenter, temp[0]);
   mCenter = temp[0];

   // Axes.
   matrix.transformVectorList(mAxes, temp, 3);

   //original
   for(long i=0; i<3; i++)
   {
      // Get new length of transformed axis.
      float len = temp[i].length();
      float ooLen = (len == 0.0f ? 0.0f : 1.0f/len);
      
      // Renormalize
      mAxes[i].set(temp[i].x*ooLen, temp[i].y*ooLen, temp[i].z*ooLen);
      
      // Adjust extents by the length ratio (multiply by new len/old len, which is 1)
      mExtents[i] *= len;
   }
#endif
}


//=============================================================================
// BBoundingBox::translate(const BVector &t)
//
// Translates the bounding box by the given amount
//=============================================================================
void BBoundingBox::translate(const BVector &t)
{
   mCenter += t;
} // BBoundingBox::translate


//=============================================================================
// BBoundingBox::spheresIntersect
//=============================================================================
bool BBoundingBox::spheresIntersect(const BVector &p, const float radius) const
{
   return(::spheresIntersect(p, radius, mCenter, mSphereRadius));
}

//=============================================================================
// BBoundingBox::rayIntersectsSphere
//
// Returns true if the given ray defined by the point "origin" and vector "vector"
// intersects this bounding box's bounding sphere -- false otherwise.
//=============================================================================
bool BBoundingBox::rayIntersectsSphere(const BVector &origin, const BVector &vector) const
{
   return(::rayIntersectsSphere(origin, vector, mCenter, mSphereRadius));
} // BBoundingBox::rayIntersectsSphere



//=============================================================================
// BBoundingBox::rayIntersectsSphere
//
// Returns true if the given ray defined by the point "origin" and vector "vector"
// intersects this bounding box's bounding sphere -- false otherwise.
//=============================================================================
bool BBoundingBox::rayIntersectsSphere(const BVector &origin, const BVector &vector, float scale) const
{
   return(::rayIntersectsSphere(origin, vector, mCenter, mSphereRadius*scale));
} // BBoundingBox::rayIntersectsSphere



//=============================================================================
// BBoundingBox::raySegmentIntersects
//=============================================================================
bool BBoundingBox::raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float &intersectDistanceSqr) const
{
   // FIXME: calling old thing

   BVector corners[8];
   computeCorners(corners);
   return(raySegmentIntersectsBox(corners, origin, vector, segment, distanceSqr, intersectDistanceSqr));
}


//=============================================================================
// BBoundingBox::raySegmentIntersects
//=============================================================================
bool BBoundingBox::raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float scale, float &intersectDistanceSqr) const
{
   // FIXME: calling old thing

   BVector corners[8];
   computeCorners(corners);

   // First check the intersection with our bounding sphere.  If this fails,
   // we cannot possibly intersect the box.
   bool result = rayIntersectsSphere(origin, vector, scale);
   if(!result)
      return(false);

   BVector negCenter=-mCenter;

   // Transform (scale) corners into temporary scratch space.
   for(long i=0; i<8; i++)
   {
      // Translate so center is at origin.
      mTransformedCorners[i].assignSum(corners[i], negCenter);

      // Scale.
      mTransformedCorners[i]*=scale;

      // Translate back.
      mTransformedCorners[i]-=negCenter;
   }
   
   return(raySegmentIntersectsBox(mTransformedCorners, origin, vector, segment, distanceSqr, intersectDistanceSqr));
}

   
//=============================================================================
// BBoundingBox::raySegmentIntersectsFirst
//=============================================================================
bool BBoundingBox::raySegmentIntersectsFirst(const BVector &origin, const BVector &vector, bool segment, float *distanceSqr, float &intersectDistanceSqr) const
{
   // FIXME: calling old thing

   BVector corners[8];
   computeCorners(corners);
   return(raySegmentIntersectsBoxFirst(corners, origin, vector, segment, distanceSqr, intersectDistanceSqr));
}

//=============================================================================
// BBoundingBox::fastRayIntersect
//=============================================================================
bool BBoundingBox::fastRayIntersect(const BVector &origin, const BVector &vector) const
{
   // First check the intersection with our bounding sphere.  If this fails,
   // we cannot possibly intersect the box.
   bool result = rayIntersectsSphere(origin, vector);
   if(!result)
      return(false);

   BVector diff = origin - mCenter;

   float fAWdU[3];
   for(long i=0; i<3; i++)
   {
      float fWdU = vector.dot(mAxes[i]);
      fAWdU[i] = fabs(fWdU);
      float fDdU = diff.dot(mAxes[i]);
      float fADdU = fabs(fDdU);
      if(fADdU > mExtents[i] && fDdU*fWdU >= 0.0f)
         return(false);
   }

   BVector kWxD = vector.cross(diff);

   float fAWxDdU = fabs(kWxD.dot(mAxes[0]));
   float fRhs = mExtents[1]*fAWdU[2] + mExtents[2]*fAWdU[1];
   if(fAWxDdU > fRhs)
      return(false);

   fAWxDdU = fabs(kWxD.dot(mAxes[1]));
   fRhs = mExtents[0]*fAWdU[2] + mExtents[2]*fAWdU[0];
   if(fAWxDdU > fRhs)
      return(false);

   fAWxDdU = fabs(kWxD.dot(mAxes[2]));
   fRhs = mExtents[0]*fAWdU[1] + mExtents[1]*fAWdU[0];
   if(fAWxDdU > fRhs)
      return(false);
      
   return(true);
}



//=============================================================================
// BBoundingBox::inside(const BVector &p)
//
// Returns true if the point p is inside the bounding box, false otherwise.
// This works if the box is still axis aligned.
//=============================================================================
bool BBoundingBox::insideAABB(const BVector &p) const
{
   // rg [3/19/06] - Changed the signs of + mExtents[x] to - mExtents[x]. 
   // I don't think this function ever worked? It wasn't used in Age3/Phoenix.
   
   if(p.x < mCenter.x - mExtents[0])
      return(false);
   if(p.x > mCenter.x + mExtents[0])
      return(false);

   if(p.y < mCenter.y - mExtents[1])
      return(false);
   if(p.y > mCenter.y + mExtents[1])
      return(false);

   if(p.z < mCenter.z - mExtents[2])
      return(false);
   if(p.z > mCenter.z + mExtents[2])
      return(false);

   return(true);
} // BBoundingBox::inside

//=============================================================================
// BBoundingBox::insideAABBXZ(const BVector &p)
//
// Returns true if the point p is inside the bounding box's X and Z bounds,
// false otherwise.  This works if the box is still axis aligned.
//=============================================================================
bool BBoundingBox::insideAABBXZ(const BVector &p) const
{
   if(p.x < mCenter.x - mExtents[0])
      return(false);
   if(p.x > mCenter.x + mExtents[0])
      return(false);

   if(p.z < mCenter.z - mExtents[2])
      return(false);
   if(p.z > mCenter.z + mExtents[2])
      return(false);

   return(true);
}


//==============================================================================
// BBoundingBox::fixCorners
//==============================================================================
void BBoundingBox::fixCorners(const BVector &minCorner, BVector &maxCorner)
{
   // This fixes up the given corners by adjusting them slight in any direction that 
   // has zero size.  This prevents us from having degenerate bounding boxes, making things
   // like intersection easier and more efficient...

   static const float cMinDelta = 2.0f*(float)cFloatCompareEpsilon;

   //CLM[04.30.08] Removed FCMPs.
   const float cMaxX = maxCorner.x;
   const float cMaxY = maxCorner.y;
   const float cMaxZ = maxCorner.z;

   const float maxX = minCorner.x + cMinDelta;
   const float maxY = minCorner.y + cMinDelta;
   const float maxZ = minCorner.z + cMinDelta;

   const float xTest = cMinDelta - _fabs(maxCorner.x - minCorner.x);
   const float yTest = cMinDelta - _fabs(maxCorner.y - minCorner.y);
   const float zTest = cMinDelta - _fabs(maxCorner.z - minCorner.z);

   maxCorner.x = static_cast<float>(Math::fSelect(xTest,maxX,cMaxX));
   maxCorner.y = static_cast<float>(Math::fSelect(yTest,maxY,cMaxY));
   maxCorner.z = static_cast<float>(Math::fSelect(zTest,maxZ,cMaxZ));


   //origional
   // Check x direction.
   /*if((maxCorner.x - minCorner.x) < cMinDelta)
      maxCorner.x = minCorner.x+(float)cMinDelta;

   // Check y direction.
   if((maxCorner.y - minCorner.y) < cMinDelta)
      maxCorner.y = minCorner.y+(float)cMinDelta;

   // Check z direction.
   if((maxCorner.z - minCorner.z) < cMinDelta)
      maxCorner.z = minCorner.z+(float)cMinDelta;*/
      
}

//==============================================================================
// BBoundingBox::draw
//==============================================================================
void BBoundingBox::draw(DWORD color, bool useCurrentWorldMatrix, uint category, float timeout) const
{
#ifndef BUILD_FINAL
   BMatrix matrix;
   
   matrix.r[0] = mAxes[0];
   matrix.r[1] = mAxes[1];
   matrix.r[2] = mAxes[2];
   matrix.setTranslation(mCenter);
   
   if (useCurrentWorldMatrix)
      matrix = XMMatrixMultiply(matrix, gRender.getWorldXMMatrix());
      
   gpDebugPrimitives->addDebugBox(matrix, BVector(mExtents[0], mExtents[1], mExtents[2]), color, category, timeout);
#endif
}

//==============================================================================
// BBoundingBox::lineIntersectsScreenError
//==============================================================================
bool BBoundingBox::lineIntersectsScreenError(const BMatrix &viewMatrix, const BVector &origin, const BVector &vector, float pixelError) const
{
   // Transform origin and vector into view space.
   BVector xformOrigin, xformVector;
   viewMatrix.transformVectorAsPoint(origin, xformOrigin);
   viewMatrix.transformVector(vector, xformVector);

   // Find where the line hits the nearz plane.
   BVector testPoint;
   xformOrigin.projectOntoPlane(cZAxisVector, BVector(0.0f, 0.0f, gRender.getViewParams().getNearZ()), xformVector, testPoint);

   // Project the test point.
   float scaleX=gRender.getViewParams().getScaleX();
   float scaleY=gRender.getViewParams().getScaleY();
   float offsetX=0.5f*gRender.getViewParams().getViewportWidth();
   float offsetY=0.5f*gRender.getViewParams().getViewportHeight();
   BASSERT(testPoint.z != 0.0f);
   float ooz=1.0f/testPoint.z;
   testPoint.x=scaleX*testPoint.x*ooz+offsetX;
   testPoint.y=-scaleY*testPoint.y*ooz+offsetY;

   // Compute corners.
   BVector corners[8];
   computeCorners(corners);
   
   // Transform vertices.
   viewMatrix.transformVectorListAsPoint(corners, mTransformedCorners, 8);

   // Project vertices.
   for(long i=7; i>=0; i--)
   {
      ooz=1.0f/mTransformedCorners[i].z;
      mTransformedCorners[i].x=scaleX*mTransformedCorners[i].x*ooz+offsetX;
      mTransformedCorners[i].y=-scaleY*mTransformedCorners[i].y*ooz+offsetY;
   }

   // Go through each face.
   BVector p[3];
   for(long i=11; i>=0; i--)
   {
      // Copy points of triangle (while getting the center too)
      BVector center=cOriginVector;
      for(long j=0; j<3; j++)
      {
         p[j]=mTransformedCorners[cFaces[i][j]];
         center+=p[j];
      }
      // Average to get center.
      center*=0.33333333333f;
      
      // Offset 'em.
      for(long j=0; j<3; j++)
      {
         // Shift away from center in x direction.
         if(p[j].x>center.x)
            p[j].x+=pixelError;
         else
            p[j].x-=pixelError;
         // Shift away from center in y direction.
         if(p[j].y>center.y)
            p[j].y+=pixelError;
         else
            p[j].y-=pixelError;
      }

      #ifdef DEBUG_SCREENERRORINTERSECTION
      renderer->render()->drawUILine((long)p[0].x, (long)p[0].y, renderer->getNearZ(), (long)p[1].x, (long)p[1].y, renderer->getNearZ(), 0, 0, 255);
      renderer->render()->drawUILine((long)p[0].x, (long)p[0].y, renderer->getNearZ(), (long)p[2].x, (long)p[2].y, renderer->getNearZ(), 0, 0, 255);
      renderer->render()->drawUILine((long)p[2].x, (long)p[2].y, renderer->getNearZ(), (long)p[1].x, (long)p[1].y, renderer->getNearZ(), 0, 0, 255);
      #endif

      // Check if the point is inside the expanded triangle.
      bool result=pointInTriangleXY(&testPoint, p, p+1, p+2);
      if(result)
         return(true);
   }

   return(false);
}


//==============================================================================
// BBoundingBox::computeCorners
//==============================================================================
void BBoundingBox::computeCorners(BVector corners[8]) const
{
   // Prescale axes by extents.
   BVector scaledAxes[3];
   
   scaledAxes[0] = mExtents[0] * mAxes[0];
   scaledAxes[1] = mExtents[1] * mAxes[1];
   scaledAxes[2] = mExtents[2] * mAxes[2];
   
   // Compute points.
   corners[0] = mCenter - scaledAxes[0] + scaledAxes[1] - scaledAxes[2];
   corners[1] = mCenter - scaledAxes[0] + scaledAxes[1] + scaledAxes[2];
   corners[2] = mCenter + scaledAxes[0] + scaledAxes[1] + scaledAxes[2];
   corners[3] = mCenter + scaledAxes[0] + scaledAxes[1] - scaledAxes[2];
   corners[4] = mCenter - scaledAxes[0] - scaledAxes[1] - scaledAxes[2];
   corners[5] = mCenter - scaledAxes[0] - scaledAxes[1] + scaledAxes[2];
   corners[6] = mCenter + scaledAxes[0] - scaledAxes[1] + scaledAxes[2];
   corners[7] = mCenter + scaledAxes[0] - scaledAxes[1] - scaledAxes[2];
}


//==============================================================================
// BBoundingBox::computeWorldCorners
//==============================================================================
void BBoundingBox::computeWorldCorners(BVector &minCorner, BVector &maxCorner) const
{
   // rg [3/6/06] - Optimized.
   XMVECTOR scaledAxesX;
   XMVECTOR scaledAxesY;
   XMVECTOR scaledAxesZ;

   scaledAxesX = XMVectorAbs(mExtents[0] * mAxes[0]);
   scaledAxesY = XMVectorAbs(mExtents[1] * mAxes[1]);
   scaledAxesZ = XMVectorAbs(mExtents[2] * mAxes[2]);

   minCorner = (XMVECTOR)mCenter - scaledAxesX - scaledAxesY - scaledAxesZ;
   maxCorner = (XMVECTOR)mCenter + scaledAxesX + scaledAxesY + scaledAxesZ;

#if 0   
   // Compute corners.
   BVector corners[8];
   computeCorners(corners);
   
   // Seed with 0th corner.
   XMVECTOR minCorner1=corners[0];
   XMVECTOR maxCorner1=corners[0];

   minCorner1 = XMVectorMin(minCorner1, corners[1]);
   minCorner1 = XMVectorMin(minCorner1, corners[2]);
   minCorner1 = XMVectorMin(minCorner1, corners[3]);
   minCorner1 = XMVectorMin(minCorner1, corners[4]);
   minCorner1 = XMVectorMin(minCorner1, corners[5]);
   minCorner1 = XMVectorMin(minCorner1, corners[6]);
   minCorner1 = XMVectorMin(minCorner1, corners[7]);
   
   maxCorner1 = XMVectorMax(maxCorner1, corners[1]);
   maxCorner1 = XMVectorMax(maxCorner1, corners[2]);
   maxCorner1 = XMVectorMax(maxCorner1, corners[3]);
   maxCorner1 = XMVectorMax(maxCorner1, corners[4]);
   maxCorner1 = XMVectorMax(maxCorner1, corners[5]);
   maxCorner1 = XMVectorMax(maxCorner1, corners[6]);
   maxCorner1 = XMVectorMax(maxCorner1, corners[7]);
   
   BDEBUG_ASSERT(Math::EqualTol(minCorner1.x, minCorner.x));
   BDEBUG_ASSERT(Math::EqualTol(minCorner1.y, minCorner.y));
   BDEBUG_ASSERT(Math::EqualTol(minCorner1.z, minCorner.z));
   
   BDEBUG_ASSERT(Math::EqualTol(maxCorner1.x, maxCorner.x));
   BDEBUG_ASSERT(Math::EqualTol(maxCorner1.y, maxCorner.y));
   BDEBUG_ASSERT(Math::EqualTol(maxCorner1.z, maxCorner.z));
#endif   
}

//==============================================================================
// BBoundingBox::computeScreenCorners
//==============================================================================
void BBoundingBox::computeScreenCorners(BPoint &minCorner, BPoint &maxCorner) const
{
   // Project the test point.
   float scaleX=gRender.getViewParams().getScaleX();
   float scaleY=gRender.getViewParams().getScaleY();
   float offsetX=0.5f*gRender.getViewParams().getViewportWidth();
   float offsetY=0.5f*gRender.getViewParams().getViewportHeight();
   
   // Compute corners.
   BVector corners[8];
   computeCorners(corners);
   
   // Transform vertices.
   gRender.getViewParams().getViewBMatrix().transformVectorListAsPoint(corners, mTransformedCorners, 8);
   BVector tempMin, tempMax;

   // Project vertices.
   for(long i=0; i<8; i++)
   {
      float ooz=1.0f/mTransformedCorners[i].z;
      mTransformedCorners[i].x=scaleX*mTransformedCorners[i].x*ooz+offsetX;
      mTransformedCorners[i].y=-scaleY*mTransformedCorners[i].y*ooz+offsetY;

      if(i==0)
      {
         tempMin = mTransformedCorners[i];
         tempMax = mTransformedCorners[i];
      }
      else
      {
         if(mTransformedCorners[i].x < tempMin.x)
            tempMin.x = mTransformedCorners[i].x;
         if(mTransformedCorners[i].y < tempMin.y)
            tempMin.y = mTransformedCorners[i].y;
         if(mTransformedCorners[i].z < tempMin.z)
            tempMin.z = mTransformedCorners[i].z;

         if(mTransformedCorners[i].x > tempMax.x)
            tempMax.x = mTransformedCorners[i].x;
         if(mTransformedCorners[i].y > tempMax.y)
            tempMax.y = mTransformedCorners[i].y;
         if(mTransformedCorners[i].z > tempMax.z)
            tempMax.z = mTransformedCorners[i].z;
      }
   }

   minCorner.x = (long)tempMin.x;
   minCorner.y = (long)tempMin.y;
   maxCorner.x = (long)tempMax.x;
   maxCorner.y = (long)tempMax.y;
}

bool BBoundingBox::getSidePlanes(BPlane planes[4], bool normalPointingIn /*= true*/) const
{
   /*
               1-----2
   y   z      /|    /|
   | /      0-----3  |
   |/       |  |  |  |  
   +---x    |  5--|--6
            | /   | /
            4-----7  
   */


   // Compute corners.
   BVector pCorners[8];
   computeCorners(pCorners);
  
   if(normalPointingIn)
   {
      planes[0].assign(pCorners[7], pCorners[0], pCorners[4]);
      planes[1].assign(pCorners[4], pCorners[1], pCorners[5]);
      planes[2].assign(pCorners[5], pCorners[2], pCorners[6]);
      planes[3].assign(pCorners[6], pCorners[3], pCorners[7]);
   }
   else
   {
      planes[0].assign(pCorners[4], pCorners[0], pCorners[7]);
      planes[1].assign(pCorners[5], pCorners[1], pCorners[4]);
      planes[2].assign(pCorners[6], pCorners[2], pCorners[5]);
      planes[3].assign(pCorners[7], pCorners[3], pCorners[6]);
   }
   return true;
}


//==============================================================================
// BBoundingBox::computeMinCornerAABB
//==============================================================================
void BBoundingBox::computeMinCornerAABB(BVector &minCorner) const
{
   // Assumes AABB.
   minCorner.x = mCenter.x - mExtents[0];
   minCorner.y = mCenter.y - mExtents[1];
   minCorner.z = mCenter.z - mExtents[2];
}


//==============================================================================
// BBoundingBox::computeMaxCornerAABB
//==============================================================================
void BBoundingBox::computeMaxCornerAABB(BVector &maxCorner) const
{
   // Assumes AABB.
   maxCorner.x = mCenter.x + mExtents[0];
   maxCorner.y = mCenter.y + mExtents[1];
   maxCorner.z = mCenter.z + mExtents[2];
}


//==============================================================================
// BBoundingBox::isZeroVolume
//==============================================================================
bool BBoundingBox::isZeroVolume() const
{
   if(Math::EqualTol(mExtents[0], 0.0f, cFloatCompareEpsilon) &&
      Math::EqualTol(mExtents[0], 0.0f, cFloatCompareEpsilon) &&
      Math::EqualTol(mExtents[0], 0.0f, cFloatCompareEpsilon))
   {
      return true;
   }
   else
   {
      return false;
   }
}


//==============================================================================
// BBoundingBox::merge
//==============================================================================
void BBoundingBox::merge(const BBoundingBox* pRhs)
{
   BVector min, max;

   BASSERT(pRhs);
   if (!pRhs)
      return;

   min.x = min(mCenter.x - mExtents[0], pRhs->mCenter.x - pRhs->mExtents[0]);
   min.y = min(mCenter.y - mExtents[1], pRhs->mCenter.y - pRhs->mExtents[1]);
   min.z = min(mCenter.z - mExtents[2], pRhs->mCenter.z - pRhs->mExtents[2]);

   max.x = max(mCenter.x + mExtents[0], pRhs->mCenter.x + pRhs->mExtents[0]);
   max.y = max(mCenter.y + mExtents[1], pRhs->mCenter.y + pRhs->mExtents[1]);
   max.z = max(mCenter.z + mExtents[2], pRhs->mCenter.z + pRhs->mExtents[2]);

   initialize(min, max);
}


//==============================================================================
// eof: boundingbox.cpp
//==============================================================================

