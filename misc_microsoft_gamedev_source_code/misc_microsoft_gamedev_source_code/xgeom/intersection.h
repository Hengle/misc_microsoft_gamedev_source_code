//-----------------------------------------------------------------------------
// File: intersection.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "math\plane.h"
#include "math\plane2.h"
#include "math\frustum.h"
#include "math\cone.h"
#include "math\hypersphere.h"
#include "math\hyperRayLine.h"
#include "tri.h"

namespace Intersection
{
  // returns OUTSIDE (not touching), BINSIDEA, AINSIDEB, or PARTIAL (touching but no containment)
  eResult AABBvsAABB(const AABB& a, const AABB& b);
  
  // true on intersection
  bool AABBvsTri3(const AABB& box, const BTri3& tri);
  bool AABBvsTri3(const BVec3& center, const BVec3& extents, const BTri3& tri);
  
  // INSIDE, FAILURE, or SUCCESS
  // coord and t not set on FAILURE
  eResult ray3AABB(BVec3& coord, float& t, const Ray3& ray, const AABB& box);
  
  // FAILURE, SUCCESS
  eResult line3AABB(const Line3& line, const AABB& box);
     
  // returns PARALLEL, FAILURE, or SUCCESS
  eResult ray3Plane(
      float& t,
      const Plane& plane,
      const Ray3& ray,
      bool backside = false,
      float maxDist = Math::fNearlyInfinite,
      float parallelEps = Math::fTinyEpsilon);
      
  // returns PARALLEL, FAILURE, or SUCCESS
  eResult ray3AxialPlane(
     float& t,
     int axis,
     float dist,
     const Ray3& ray,
     bool backside = false,
     float maxDist = Math::fNearlyInfinite,
     float parallelEps = Math::fTinyEpsilon);

   // SUCCESS, FAILURE, or PARALLEL         
   eResult ray3Tri(float& t, float& u, float& v, const Ray3& ray, const BTri3& tri, bool backFaceCulling, float epsilon);
   
   // Doesn't properly handle boundary cases.
   // SUCCESS, FAILURE
   eResult point2InPolyRTR(const BVec2& t, const BVec2* pV, int n);
   
   // Properly handles boundary cases but is slightly slower than point2InPolyRTR.
   // SUCCESS, FAILURE
   eResult point2InPoly(const BVec2& t, const BVec2* pV, int n);
     
   bool point2InTriBary(float& alpha, float& beta, const BVec2& t, const BVec2* pV);
                                    
   // true if touching
   bool sphereSphere(const Sphere& a, const Sphere& b);
         
   bool AABBBehindPlane(
      const AABB& box,
      const Plane& plane);

   bool AABBInFrontPlane(
      const AABB& box,
      const Plane& plane);
   
   // INSIDE, PARTIAL, or OUTSIDE
   // For debugging only - tests all 8 box corner points per plane
   eResult boxFrustumSlow(
      const AABB& box,
      const BFrustum& frustum);
                     
   // INSIDE, PARTIAL, or OUTSIDE
   eResult boxFrustum(
      const AABB& box,
      const BFrustum& frustum);
                        
   // returns OUTSIDE or INSIDE
   eResult pointFrustum(
      const BVec3& point,
      const BFrustum& frustum);
         
   // returns OUTSIDE, INSIDE, or PARTIAL
   eResult sphereFrustum(
      const Sphere& sphere,
      const BFrustum& frustum);
         
   // Returns number of intersection points.
   int raySphere(BVec3 pPoint[2], const Ray3& ray, const Sphere& sphere);
   
   // true if point inside cone
   bool conePoint(const Cone& c, const BVec3& p);
   
   // true if sphere touches or inside cone
   bool coneSphere(const Cone& c, const Sphere& s);
   
   enum EDistFromLineSegmentResult
   {
      eInvalidLine = 0,
      ePBeforeA    = 1,
      ePAfterB     = 2,
      ePWithinAB   = 3
   };

   // Returns squared distances.
   // Line segment is [start, end).
   EDistFromLineSegmentResult distFromLineSegment(
      const Line2& l,
      const BVec2& p,
      float& distFrom,
      float& distAlong,
      float eps = Math::fTinyEpsilon);
      
   // Clips line segment to positive halfspace of 2D plane.
   // Returns INSIDE, OUTSIDE, or PARTIAL.
   eResult line2Plane2(Line2& result, const Line2& line, const Plane2& plane2);

   // Returns true on failure.      
   bool threePlanes(BVec3& dest, const Plane& p1, const Plane& p2, const Plane& p3, float eps = Math::fMinuteEpsilon);
   
   // Returns true on failure.
   bool twoPlanes(Ray3& result, const Plane& p1, const Plane& p2, float parallelEps = Math::fMinuteEpsilon);
                              
} // namespace Intersection
