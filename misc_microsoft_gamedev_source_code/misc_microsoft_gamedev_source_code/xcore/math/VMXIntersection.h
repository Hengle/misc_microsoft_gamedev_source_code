//==============================================================================
// VMXIntersection.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#include "VMXUtils.h"

namespace BVMXIntersection
{
   //-----------------------------------------------------------------------------
   // true if the bounding sphere is completely outside the frustum.
   //-----------------------------------------------------------------------------
   BOOL checkSphereForRejectionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius);
   BOOL checkSphereForRejectionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, XMVECTOR radius);
   
   //-----------------------------------------------------------------------------
   // true if the bounding sphere is completely outside the polyhedron.
   //-----------------------------------------------------------------------------
   BOOL checkSphereForRejectionVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR center, float radius) ;
   BOOL checkSphereForRejectionVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR center, XMVECTOR radius) ;
   
   //-----------------------------------------------------------------------------
   // true if the bounding sphere is completely inside the frustum.
   //-----------------------------------------------------------------------------
   BOOL checkSphereForInclusionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius);
   BOOL checkSphereForInclusionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, XMVECTOR radius);
   
   //-----------------------------------------------------------------------------
   // c - box center
   // d - box half diagonal
   // -1 = completely outside
   // 0 = partial
   // 1 = completely inside
   //-----------------------------------------------------------------------------
   int AABBvsFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d);
   bool AABBTouchesFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d);
   bool AABBTouchesVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR c, XMVECTOR d);
   
   //-----------------------------------------------------------------------------
   // c - box center
   // d - box half diagonal
   // 0 = partial or outside
   // 1 = completely inside
   //-----------------------------------------------------------------------------
   int AABBVsPlane(const XMVECTOR plane, XMVECTOR c, XMVECTOR d);
   
   //-----------------------------------------------------------------------------
   // -1 = completely outside
   // 0 = partial 
   // 1 = completely inside
   //-----------------------------------------------------------------------------
   int AABBInsideFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d);
   
   //-----------------------------------------------------------------------------
   // Calculates the AABB that surrounds the sphere.
   //-----------------------------------------------------------------------------
   void calculateSphereAABB(XMVECTOR centerRadius, XMVECTOR& min, XMVECTOR& max);
   
   //-----------------------------------------------------------------------------
   // Transforms the sphere by the specified rotation/translation matrix.
   //-----------------------------------------------------------------------------
   XMVECTOR transformSphere(XMVECTOR centerRadius, XMMATRIX matrix);
   
   //-----------------------------------------------------------------------------
   // True if sphere touches or is inside the AABB.
   //-----------------------------------------------------------------------------
   BOOL sphereVsAABB(XMVECTOR centerRadius, XMVECTOR min, XMVECTOR max);
   
   //-----------------------------------------------------------------------------
   // Return values: 0 = no intersection, 
   //                1 = intersection, 
   //                2 = sphere is completely inside frustum
   // 
   // pPlanes should be the frustum's 6 planes. 
   // pCorners should be the frustum's 8 corners, as returned by BMatrixTracker.
   //-----------------------------------------------------------------------------
   INT IntersectSphereFrustum( XMVECTOR centerRadius, const XMVECTOR* RESTRICT pPlanes, const XMVECTOR* RESTRICT pCorners);
   
   //-----------------------------------------------------------------------------
   // True if the sphere and cone intersect.
   //-----------------------------------------------------------------------------
   bool coneSphere(XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, XMVECTOR sphereCenterRadius);
   
   //-----------------------------------------------------------------------------
   // Create the 6 vertices of a polyhedron that surrounds a spotlight's cone.
   //-----------------------------------------------------------------------------
   enum { cNumCappedConePoints = 6 };
   void createCappedConePoints(XMVECTOR* RESTRICT points, XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, float coneHeight);
   
   //-----------------------------------------------------------------------------
   // True if the spotlight's capped cone is completely outside the frustum.
   //-----------------------------------------------------------------------------
   bool cappedConeVsFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, float coneHeight);
   
   //-----------------------------------------------------------------------------      
   // Calculates the OBB of a capped cone.
   //-----------------------------------------------------------------------------      
   void calculateCappedConeOBB(
      XMVECTOR& boxMin, XMVECTOR& boxMax, XMMATRIX& boxToWorld, 
      XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, float radius);


   //-----------------------------------------------------------------------------      
   // Calculates the closest theta of a point to on a line segment from a given point
   // if theta >= 0 && theta <= 1.0f then the closest point is on the actual line segment
   // else the closest point is on the line but is not on the line segment [A,B]
   // The axis of the line segment is returned as well.
   //-----------------------------------------------------------------------------      
   void closestPointOnLineSegmentTheta(XMVECTOR point, XMVECTOR segmentA, XMVECTOR segmentB, XMVECTOR& axis, XMVECTOR& theta);

   //-----------------------------------------------------------------------------      
   // Calculates the closest point on a line segment from a given point
   //-----------------------------------------------------------------------------      
   void closestPointOnLineSegment(XMVECTOR point, XMVECTOR segmentA, XMVECTOR segmentB, XMVECTOR& result);

   //-----------------------------------------------------------------------------      
   // Calculates the closest point on a line from a given point.  The closest point 
   // may be outside of the line segment [A,B]
   //-----------------------------------------------------------------------------      
   void closestPointOnLine(XMVECTOR point, XMVECTOR segmentA, XMVECTOR segmentB, XMVECTOR& result);

   //-----------------------------------------------------------------------------      
   // Determine if a 3D point is inside an oriented cylinder
   //-----------------------------------------------------------------------------      
   bool pointInsideCylinder(XMVECTOR point, XMVECTOR cylinderStart, XMVECTOR cylinderEnd, XMVECTOR radiusV);

   //-----------------------------------------------------------------------------      
   // projectSphere() returns an estimate of the projection of a sphere. It's accuracy 
   // is low for very close spheres. centerRadius should be in view space.
   //-----------------------------------------------------------------------------
   enum eProjectSphereResult
   {
      cPSFullscreen,
      cPSPartial
   };
   
   eProjectSphereResult projectSphere(
      XMVECTOR& screenCenter,
      XMVECTOR& screenRadius,
      XMVECTOR centerRadius, 
      XMMATRIX viewToScreen);
      
   bool calculateBoxAreaOrig(
      float& area,
      XMVECTOR boxEye,
      XMVECTOR boxMin,
      XMVECTOR boxMax,
      XMMATRIX boxToWorld,
      XMMATRIX worldToScreen);
      
   bool calculateBoxArea(
      float& area,
      XMVECTOR boxEye,
      XMVECTOR boxMin,
      XMVECTOR boxMax,
      XMMATRIX boxToWorld,
      XMMATRIX worldToScreen);
      
   // Compute a frustum's cornerpoints given D3D-style projection matrix.
   void computeFrustumVertices(XMVECTOR* RESTRICT pVerts, XMMATRIX proj);
   void computeBoundsOfPoints(const XMVECTOR* RESTRICT pPoints, uint numPoints, XMVECTOR* RESTRICT pMin, XMVECTOR* RESTRICT pMax);
         
} // namespace BVMXIntersection
