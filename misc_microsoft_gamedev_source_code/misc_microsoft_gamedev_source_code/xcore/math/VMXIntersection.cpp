//==============================================================================
// VMXIntersection.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xcore.h"
#include "math\VMXIntersection.h"

namespace BVMXIntersection
{
   static const XMVECTOR g_VectorZero = { 0.0f, 0.0f, 0.0f, 0.0f };
   
   //==============================================================================
   // checkSphereForRejectionFrustum
   //==============================================================================
   BOOL checkSphereForRejectionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius) 
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

      return XMVector4Less(minDist, negRadius);
   } 

   //==============================================================================
   // checkSphereForRejectionFrustum
   //==============================================================================
   BOOL checkSphereForRejectionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, XMVECTOR radius) 
   {
      center = __vrlimi(center, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
      XMVECTOR negRadius = XMVectorNegate(XMVectorSplatX(radius));

      XMVECTOR dist0 = XMVector4Dot(center, pPlanes[0]);
      XMVECTOR dist1 = XMVector4Dot(center, pPlanes[1]);
      XMVECTOR dist2 = XMVector4Dot(center, pPlanes[2]);
      XMVECTOR dist3 = XMVector4Dot(center, pPlanes[3]);
      XMVECTOR dist4 = XMVector4Dot(center, pPlanes[4]);
      XMVECTOR dist5 = XMVector4Dot(center, pPlanes[5]);

      XMVECTOR minDist = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(dist0, dist1), dist2), dist3), dist4), dist5);

      return XMVector4Less(minDist, negRadius);
   } 

   //==============================================================================
   // checkSphereForRejectionVolume
   //==============================================================================
   BOOL checkSphereForRejectionVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR center, float radius) 
   {
      center = __vrlimi(center, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
      XMVECTOR negRadius = XMVectorNegate(XMVectorReplicate(radius));

      // rg [7/24/06] - Umm, why did I make this one?
      XMVECTOR minDist = XMVectorSplatOne();

      for (uint i = 0; i < numPlanes; i++)
      {
         XMVECTOR dist = XMVector4Dot(center, pPlanes[i]);

         minDist = XMVectorMin(minDist, dist);
      }      

      return XMVector4Less(minDist, negRadius);
   }

   //==============================================================================
   // checkSphereForRejectionVolume
   //==============================================================================
   BOOL checkSphereForRejectionVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR center, XMVECTOR radius) 
   {
      center = __vrlimi(center, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
      XMVECTOR negRadius = XMVectorNegate(XMVectorSplatX(radius));

      // rg [7/24/06] - Umm, why did I make this one?
      XMVECTOR minDist = XMVectorSplatOne();

      for (uint i = 0; i < numPlanes; i++)
      {
         XMVECTOR dist = XMVector4Dot(center, pPlanes[i]);

         minDist = XMVectorMin(minDist, dist);
      }      

      return XMVector4Less(minDist, negRadius);
   }

   //==============================================================================
   // checkSphereForInclusionFrustum
   //==============================================================================
   BOOL checkSphereForInclusionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, float radius) 
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

      return XMVector4Greater(minDist, xradius);
   }

   //==============================================================================
   // checkSphereForInclusionFrustum
   //==============================================================================
   BOOL checkSphereForInclusionFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR center, XMVECTOR radius) 
   {
      center = __vrlimi(center, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
      XMVECTOR xradius = XMVectorSplatX(radius);

      XMVECTOR dist0 = XMVector4Dot(center, pPlanes[0]);
      XMVECTOR dist1 = XMVector4Dot(center, pPlanes[1]);
      XMVECTOR dist2 = XMVector4Dot(center, pPlanes[2]);
      XMVECTOR dist3 = XMVector4Dot(center, pPlanes[3]);
      XMVECTOR dist4 = XMVector4Dot(center, pPlanes[4]);
      XMVECTOR dist5 = XMVector4Dot(center, pPlanes[5]);

      XMVECTOR minDist = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(dist0, dist1), dist2), dist3), dist4), dist5);

      return XMVector4Greater(minDist, xradius);
   }

   //==============================================================================
   // AABBvsFrustum
   // c = center
   // d = half-diagnol
   // -1 = completely outside
   // 0 = partial
   // 1 = completely inside
   //==============================================================================
   int AABBvsFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d)
   {
      c = XMVectorInsert(c, XMVectorSplatOne(), 0,  0, 0, 0, 1);
                        
      XMVECTOR NP0 = XMVector3Dot(d, XMVectorAbs(pPlanes[0]));
      XMVECTOR NP1 = XMVector3Dot(d, XMVectorAbs(pPlanes[1]));
      XMVECTOR NP2 = XMVector3Dot(d, XMVectorAbs(pPlanes[2]));
      XMVECTOR NP3 = XMVector3Dot(d, XMVectorAbs(pPlanes[3]));
      XMVECTOR NP4 = XMVector3Dot(d, XMVectorAbs(pPlanes[4]));
      XMVECTOR NP5 = XMVector3Dot(d, XMVectorAbs(pPlanes[5]));

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
      
      XMVECTOR s0 = XMVectorSubtract(MP0, NP0);     
      XMVECTOR s1 = XMVectorSubtract(MP1, NP1);     
      XMVECTOR s2 = XMVectorSubtract(MP2, NP2);     
      XMVECTOR s3 = XMVectorSubtract(MP3, NP3);     
      XMVECTOR s4 = XMVectorSubtract(MP4, NP4);     
      XMVECTOR s5 = XMVectorSubtract(MP5, NP5);     

      XMVECTOR minS = XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(XMVectorMin(s0, s1), s2), s3), s4), s5);
      
      // trivial exclusion
      if (XMVector4Less(minA, XMVectorZero()))
         return -1;
      
      // partial or outside
      if (XMVector4Less(minS, XMVectorZero()))
         return 0;

      // completely inside
      return 1;
   }   

   //==============================================================================
   // AABBInsideFrustum
   //==============================================================================
   int AABBInsideFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d)
   {
      c = XMVectorInsert(c, XMVectorSplatOne(), 0,  0, 0, 0, 1);
            
      XMVECTOR NP0 = XMVector3Dot(d, XMVectorAbs(pPlanes[0]));
      XMVECTOR NP1 = XMVector3Dot(d, XMVectorAbs(pPlanes[1]));
      XMVECTOR NP2 = XMVector3Dot(d, XMVectorAbs(pPlanes[2]));
      XMVECTOR NP3 = XMVector3Dot(d, XMVectorAbs(pPlanes[3]));
      XMVECTOR NP4 = XMVector3Dot(d, XMVectorAbs(pPlanes[4]));
      XMVECTOR NP5 = XMVector3Dot(d, XMVectorAbs(pPlanes[5]));

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
      
      // partial
      if (XMVector4Less(minS, XMVectorZero()))
         return 0;

      // completely inside
      return 1;
   }   
   
   //==============================================================================
   // AABBInsideFrustum
   //==============================================================================
   int AABBVsPlane(const XMVECTOR plane, XMVECTOR c, XMVECTOR d)
   {
      c = XMVectorInsert(c, XMVectorSplatOne(), 0,  0, 0, 0, 1);
      
      XMVECTOR NP0 = XMVector3Dot(d, XMVectorAbs(plane));

      // MP = center distance from plane (positive = in front/inside)
      XMVECTOR MP0 = XMVector4Dot(c, plane);
      
      XMVECTOR s0 = XMVectorSubtract(MP0, NP0);     
      XMVECTOR a0 = XMVectorAdd(MP0, NP0);
      
      // partial or outside
      if (XMVector4Less(s0, XMVectorZero()))
      {
         // completely outside
         if (XMVector4Less(a0, XMVectorZero()))
            return -1;
         
         // partial
         return 0;
      }

      // completely inside
      return 1;
   }

   //==============================================================================
   // AABBTouchesFrustum
   //==============================================================================
   bool AABBTouchesFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR c, XMVECTOR d)
   {
      c = XMVectorInsert(c, XMVectorSplatOne(), 0,  0, 0, 0, 1);
            
      XMVECTOR NP0 = XMVector3Dot(d, XMVectorAbs(pPlanes[0]));
      XMVECTOR NP1 = XMVector3Dot(d, XMVectorAbs(pPlanes[1]));
      XMVECTOR NP2 = XMVector3Dot(d, XMVectorAbs(pPlanes[2]));
      XMVECTOR NP3 = XMVector3Dot(d, XMVectorAbs(pPlanes[3]));
      XMVECTOR NP4 = XMVector3Dot(d, XMVectorAbs(pPlanes[4]));
      XMVECTOR NP5 = XMVector3Dot(d, XMVectorAbs(pPlanes[5]));

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
      
      // trivial exclusion
      if (XMVector4Less(minA, XMVectorZero()))
         return false;

      return true;
   }   

   //==============================================================================
   // AABBTouchesVolume
   //==============================================================================
   bool AABBTouchesVolume(const XMVECTOR* RESTRICT pPlanes, uint numPlanes, XMVECTOR c, XMVECTOR d)
   {
      c = XMVectorInsert(c, XMVectorSplatOne(), 0,  0, 0, 0, 1);
            
      XMVECTOR minA = XMVectorSplatOne();

      for (uint i = 0; i < numPlanes; i++)
      {
         XMVECTOR NP = XMVector3Dot(d, XMVectorAbs(pPlanes[i]));

         // MP = center distance from plane (positive = in front/inside)
         XMVECTOR MP = XMVector4Dot(c, pPlanes[i]);

         XMVECTOR a = XMVectorAdd(NP, MP);     

         minA = XMVectorMin(minA, a);
      }

      if (XMVector4Less(minA, XMVectorZero()))
         return false;

      return true;
   }   
   
   //==============================================================================
   // calculateSphereAABB
   //==============================================================================
   void calculateSphereAABB(XMVECTOR centerRadius, XMVECTOR& min, XMVECTOR& max)
   {
      XMVECTOR radius = XMVectorSplatW(centerRadius);
      
      min = centerRadius - radius;
      max = centerRadius + radius;
   }
   
   //==============================================================================
   // transformSphere
   //==============================================================================
   XMVECTOR transformSphere(XMVECTOR centerRadius, XMMATRIX matrix)
   {
      XMVECTOR p = XMVector3Transform(centerRadius, matrix);
      p = XMVectorInsert(p, centerRadius, 0,  0, 0, 0, 1);   
      return p;
   }
   
   //==============================================================================
   // sphereVsAABB
   //==============================================================================
   BOOL sphereVsAABB(XMVECTOR centerRadius, XMVECTOR BoxMin, XMVECTOR BoxMax)
   {  
      XMVECTOR SphereCenter = centerRadius;
      XMVECTOR SphereRadius = XMVectorSplatW(centerRadius);
      
      XMVECTOR d = XMVectorZero();

      // Compute d for each dimension.
      XMVECTOR LessThanMin = XMVectorLess( SphereCenter, BoxMin );
      XMVECTOR GreaterThanMax = XMVectorGreater( SphereCenter, BoxMax );

      XMVECTOR MinDelta = SphereCenter - BoxMin;
      XMVECTOR MaxDelta = SphereCenter - BoxMax;

      // Choose value for each dimension based on the comparison.
      d = XMVectorSelect( d, MinDelta, LessThanMin );
      d = XMVectorSelect( d, MaxDelta, GreaterThanMax );

      // Use a dot-product to square them and sum them together.
      XMVECTOR d2 = XMVector3Dot( d, d );

      return XMVector4LessOrEqual( d2, XMVectorMultiply( SphereRadius, SphereRadius ) );
   }
   
   //-----------------------------------------------------------------------------
   // Return the point on the line segement (S1, S2) nearest the point P.
   //-----------------------------------------------------------------------------
   static inline XMVECTOR PointOnLineSegmentNearestPoint( XMVECTOR S1, XMVECTOR S2, XMVECTOR P )
   {
      XMVECTOR Dir = S2 - S1;
      XMVECTOR Projection = ( XMVector3Dot( P, Dir ) - XMVector3Dot( S1, Dir ) );
      XMVECTOR LengthSq = XMVector3Dot( Dir, Dir );

      XMVECTOR Point;

      if ( XMVector3Less( Projection, g_VectorZero ) )
      {
         // t < 0
         Point = S1;
      }
      else if ( XMVector3Greater( Projection, LengthSq ) )
      {
         // t > 1
         Point = S2;
      }
      else
      {
         XMVECTOR t = Projection * XMVectorReciprocal( LengthSq );
         Point = S1 + t * Dir;
      }

      return Point;
   }
         
   //-----------------------------------------------------------------------------
   // Exact sphere vs frustum test.  The algorithm first checks the sphere against
   // the planes of the frustum, then if the plane checks were indeterminate finds
   // the nearest feature (plane, line, point) on the frustum to the center of the
   // sphere and compares the distance to the nearest feature to the radius of the 
   // sphere (it is so cool that all the comment lines above are the same length).
   // Return values: 0 = no intersection, 
   //                1 = intersection, 
   //                2 = sphere is completely inside frustum
   //-----------------------------------------------------------------------------
   INT IntersectSphereFrustum( XMVECTOR centerRadius, const XMVECTOR* RESTRICT pPlanes, const XMVECTOR* RESTRICT pCorners)
   {
      // Load the sphere.
      XMVECTOR Center = centerRadius;
      // Set w of the center to one so we can dot4 with the plane.
      Center = XMVectorInsert( centerRadius, XMVectorSplatOne(), 0, 0, 0, 0, 1);

      XMVECTOR Radius = XMVectorSplatW(centerRadius);

      // Check against each plane of the frustum.
      BOOL InsideAll = TRUE;
      BOOL CenterInsideAll = TRUE;

      XMVECTOR Dist[6];

      for ( INT i = 0; i < 6; i++ )
      {
         Dist[i] = XMVector4Dot( Center, pPlanes[i] );

         if ( XMVector4Less( Dist[i], -Radius ) )
         {
            // Outside the plane.
            return 0;
         }
         else if ( XMVector4Less( Dist[i], Radius ) )
         {
            // Intersecting the plane.
            InsideAll = FALSE;

            // Check if the center is outside the plane.
            if ( XMVector4Less( Dist[i], g_VectorZero ) )
               CenterInsideAll = FALSE;
         }
      }

      // If the sphere is inside all planes it is fully inside.
      if ( InsideAll )
         return 2;

      // If the center of the sphere is inside all planes and the sphere intersects 
      // one or more planes then it must intersect.
      if ( CenterInsideAll )
         return 1;

      // The sphere may be outside the frustum or intersecting the frustum.
      // Find the nearest feature (face, edge, or corner) on the frustum 
      // to the sphere.

      // The faces adjacent to each face are:
      static const uchar adjacent_faces[6][4] = { 
         { 2, 3, 4, 5 },    // 0
         { 2, 3, 4, 5 },    // 1
         { 0, 1, 4, 5 },    // 2
         { 0, 1, 4, 5 },    // 3
         { 0, 1, 2, 3 },    // 4
         { 0, 1, 2, 3 } 
      };  // 5

      // Check to see if the nearest feature is one of the planes.
      for ( INT i = 0; i < 6; i++ )
      {
         if ( XMVector4Less( Dist[i], g_VectorZero ) )
         {
            // Find the nearest point on the plane to the center of the sphere.
            XMVECTOR Point = Center - (pPlanes[i] * Dist[i]);

            // Set w of the point to one.
            Point = XMVectorInsert( Point, XMVectorSplatOne(), 0, 0, 0, 0, 1);

            // If the point is inside the face (inside the adjacent planes) then
            // this plane is the nearest feature.
            BOOL InsideFace = TRUE;

            for ( INT j = 0; j < 4; j++ )
            {
               INT plane_index = adjacent_faces[i][j];

               if ( XMVector4Less( XMVector4Dot( Point, pPlanes[plane_index] ), g_VectorZero ) )
               {
                  InsideFace = FALSE;
                  break;
               }
            }

            // Since we have already checked distance from the plane we know
            // that the sphere must intersect if this plane is nearest.
            if ( InsideFace )
               return 1;
         }
      }

      // The Edges are:
      static const uchar edges[12][2] = { 
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

      XMVECTOR RadiusSq = Radius * Radius;

      // Check to see if the nearest feature is one of the edges (or corners).
      for ( INT i = 0; i < 12; i++ )
      {
         INT ei0 = edges[i][0];
         INT ei1 = edges[i][1]; 

         // Find the nearest point on the edge to the center of the sphere.
         // The corners of the frustum are included as the endpoints of the edges.
         XMVECTOR Point = PointOnLineSegmentNearestPoint( pCorners[ei0], pCorners[ei1], Center );

         XMVECTOR Delta = Center - Point;

         XMVECTOR DistSq = XMVector3Dot( Delta, Delta );

         // If the distance to the center of the sphere to the point is less than 
         // the radius of the sphere then it must intersect.
         if ( XMVector4LessOrEqual( DistSq, RadiusSq ) )
            return 1;
      }

      // The sphere must be outside the frustum.
      return 0;
   }

   //==============================================================================
   // coneSphere
   //==============================================================================
   bool coneSphere(XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, XMVECTOR sphereCenterRadius)
   {
      float sinAngle;
      float cosAngle;
      XMScalarSinCosEst(&sinAngle, &cosAngle, coneHalfAngle);
            
      XMVECTOR diff = XMVectorSubtract(sphereCenterRadius, coneApex);
      XMVECTOR flsqr = XMVector3Dot(diff, diff);
      
      //const float frsqr = sphereCenterRadius.w * sphereCenterRadius.w;   
      XMVECTOR sphereRadius = XMVectorSplatW(sphereCenterRadius);
      XMVECTOR frsqr = XMVectorMultiply(sphereRadius, sphereRadius);
      
      //if (flsqr.w <= frsqr)
      //   return true;
      if (XMVector4LessOrEqual(flsqr, frsqr))
         return true;

      XMVECTOR fdot = XMVector3Dot(diff, coneAxis);      
      XMVECTOR fdotsqr = XMVectorMultiply(fdot, fdot);
      XMVECTOR fulen = XMVectorSqrtEst(XMVectorAbs(XMVectorSubtract(flsqr, fdotsqr)));
      
      XMVECTOR vcosangle = XMVectorReplicate(cosAngle);
      XMVECTOR vsinangle = XMVectorReplicate(sinAngle);
               
      //const float fcossqr = cosAngle * cosAngle;
      XMVECTOR fcossqr = XMVectorMultiply(vcosangle, vcosangle);
      
      //const float ftest = cosAngle * fdot.w + sinAngle * fulen.w;
      XMVECTOR ftest = XMVectorAdd(XMVectorMultiply(vcosangle, fdot), XMVectorMultiply(vsinangle, fulen));
      
      //if ((fdotsqr.w >= flsqr.w * fcossqr) && (fdot.w > 0.0f))
      //   return true;
      
      if ((XMVector4GreaterOrEqual(fdotsqr, XMVectorMultiply(flsqr, fcossqr))) && XMVector4Greater(fdot, XMVectorZero()))
         return true;
                  
      //const float fdiscr = ftest * ftest - flsqr.w + frsqr;
      
      XMVECTOR fdiscr = XMVectorAdd(XMVectorSubtract(XMVectorMultiply(ftest, ftest), flsqr), frsqr);
      
      //return fdiscr >= 0.0f && ftest >= 0.0f;
      if (XMVector4GreaterOrEqual(fdiscr, XMVectorZero()) && XMVector4GreaterOrEqual(ftest, XMVectorZero()))
         return true;
      
      return false;
   }
   
   //==============================================================================
   // createCappedConePoints
   //==============================================================================
   void createCappedConePoints(XMVECTOR* RESTRICT pPoints, XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, float radius)
   {
      BDEBUG_ASSERT((coneHalfAngle > 0.0f) && (coneHalfAngle < Math::fDegToRad(179.0f)));
      
      XMVECTOR u = XMVector3Orthogonal(coneAxis);
      u = XMVector3NormalizeEst(u);

      XMVECTOR v = XMVector3Cross(coneAxis, u);

      XMVECTOR vradius = XMVectorReplicate(radius);
      
      float s, c;
      XMScalarSinCosEst(&s, &c, coneHalfAngle);
      
      XMVECTOR vc = XMVectorReplicate(c);
      
      XMVECTOR y = vc * vradius;
      XMVECTOR x = XMVectorReplicate(s) * vradius;

      const XMVECTOR one = XMVectorSplatOne();
      pPoints[0] = XMVectorInsert(coneApex, one, 0, 0, 0, 0, 1);

      XMVECTOR center = coneApex + coneAxis * y;
      pPoints[1] = XMVectorInsert(center - u * x - v * x, one, 0, 0, 0, 0, 1);
      pPoints[2] = XMVectorInsert(center + u * x - v * x, one, 0, 0, 0, 0, 1);
      pPoints[3] = XMVectorInsert(center + u * x + v * x, one, 0, 0, 0, 0, 1);
      pPoints[4] = XMVectorInsert(center - u * x + v * x, one, 0, 0, 0, 0, 1);
      
      //float angleC = Math::fHalfPi - coneHalfAngle;
      //float h = sqrt(x.x*x.x + y.x*y.x) / sin(angleC);
      //h = radius / sin(angleC);
      
      XMVECTOR h = vradius * XMVectorReciprocalEst(vc);
      
      pPoints[5] = XMVectorInsert(coneApex + coneAxis * h, one, 0, 0, 0, 0, 1);
   }      
   
   //==============================================================================
   // cappedConeVsFrustum
   //==============================================================================
   bool cappedConeVsFrustum(const XMVECTOR* RESTRICT pPlanes, XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, float radius)
   {
      XMVECTOR points[cNumCappedConePoints];
            
      createCappedConePoints(points, coneApex, coneAxis, coneHalfAngle, radius);
                  
      for (uint i = 0; i < 6; i++)
      {
         XMVECTOR maxDist = XMVector4Dot(points[0], pPlanes[i]);
         maxDist = XMVectorMax(maxDist, XMVector4Dot(points[1], pPlanes[i]));
         maxDist = XMVectorMax(maxDist, XMVector4Dot(points[2], pPlanes[i]));
         maxDist = XMVectorMax(maxDist, XMVector4Dot(points[3], pPlanes[i]));
         maxDist = XMVectorMax(maxDist, XMVector4Dot(points[4], pPlanes[i]));
         maxDist = XMVectorMax(maxDist, XMVector4Dot(points[5], pPlanes[i]));

         if (XMVector4Less(maxDist, XMVectorZero()))
            return true;
      }

      return false;
   }
   
   //==============================================================================
   // calculateCappedConeOBB
   //==============================================================================
   void calculateCappedConeOBB(
      XMVECTOR& boxMin, XMVECTOR& boxMax, XMMATRIX& boxToWorld, 
      XMVECTOR coneApex, XMVECTOR coneAxis, float coneHalfAngle, float radius)
   {
      XMVECTOR u = XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), coneAxis);
      XMVECTOR altU = XMVector3Orthogonal(coneAxis);
      XMVECTOR lengthSelect = XMVectorGreater(XMVector3LengthSq(u), XMVectorReplicate(.0000125f));
      
      u = XMVectorSelect(altU, u, lengthSelect);
      
      u = XMVector3NormalizeEst(u);
      
      XMVECTOR v = XMVector3Cross(coneAxis, u);
      
      XMVECTOR zero = XMVectorZero();
      XMVECTOR one = XMVectorSplatOne();
      
      boxToWorld.r[0] = XMVectorInsert(u, zero, 0, 0, 0, 0, 1);
      boxToWorld.r[1] = XMVectorInsert(v, zero, 0, 0, 0, 0, 1);
      boxToWorld.r[2] = XMVectorInsert(coneAxis, zero, 0, 0, 0, 0, 1);
      boxToWorld.r[3] = XMVectorInsert(coneApex, one, 0, 0, 0, 0, 1);
                  
      float s = XMScalarSinEst(coneHalfAngle);      
            
      boxMin = XMVectorSet(-radius * s, -radius * s, 0.0f, 0.0f);
      boxMax = XMVectorSet( radius * s,  radius * s,  radius, 0.0f);
   }      

   //==============================================================================
   // projectSphere
   //==============================================================================          
   eProjectSphereResult projectSphere(
      XMVECTOR& screenCenter,
      XMVECTOR& screenRadius,
      XMVECTOR centerRadius, 
      XMMATRIX viewToScreen)
   {
      if (XMVector4Less(XMVectorSplatZ(centerRadius), gXMOneHalf))
         return cPSFullscreen;

      XMVECTOR sc = centerRadius;
      XMVECTOR sr = XMVectorSplatW(centerRadius);

      XMVECTOR sd = XMVectorSqrtEst(XMVector3Dot(sc, sc));

      if (XMVector4LessOrEqual(sd, sr))
         return cPSFullscreen;

      XMVECTOR sa = XMVectorReplicate(Math::fHalfPi) - XMVectorACosEst(sr * XMVectorReciprocalEst(sd));
      XMVECTOR ca = XMVectorSqrtEst(XMVectorAbs(XMVectorSplatOne() - XMVectorMultiply(sa, sa)));
            
      XMVECTOR sl = XMVectorSqrtEst(XMVector2Dot(sc, sc));
      XMVECTOR invSl = XMVectorReciprocalEst(sl);
      XMVECTOR scaledSc = sc * invSl;
      
      XMVECTOR a = sl * ca;
      XMVECTOR b = XMVectorSplatZ(sc) * sa;
      XMVECTOR c = XMVectorSplatZ(sc) * ca;
      XMVECTOR d = sl * sa;
      
      XMVECTOR v1;
      XMVECTOR temp1 = a + b;
      v1 = temp1 * scaledSc;
      XMVECTOR v1z = c - d;
      v1 = XMVectorInsert(v1, v1z, 0, 0, 0, 1, 0);
                  
      XMVECTOR v2;
      XMVECTOR temp2 = a - b;
      v2 = temp2 * scaledSc;
      XMVECTOR v2z = c + d;
      v2 = XMVectorInsert(v2, v2z, 0, 0, 0, 1, 0);
            
      const BOOL fullscreen = XMVector4LessOrEqual(XMVectorMin(XMVectorSplatZ(v1), XMVectorSplatZ(v2)), XMVectorZero());
      
      v1 = XMVector3TransformCoord(v1, viewToScreen);
      v2 = XMVector3TransformCoord(v2, viewToScreen);

      const XMVECTOR point5 = gXMOneHalf;
      screenCenter = (v1 + v2) * point5;
      screenRadius = (XMVector2Length(v1 - v2) * point5); //sqrt( Math::Sqr(v1.x - v2.x) + Math::Sqr(v1.y - v2.y) ) * .5f;

      return fullscreen ? cPSFullscreen : cPSPartial;
   }      
      
   //-----------------------------------------------------------------------------
   // From http://www.acm.org/jgt/papers/SchmalstiegTobler99/bboxarea.html
   //indexlist: this table stores the 64 possible cases of classification of
   //the eyepoint with respect to the 6 defining planes of the bbox (2^6=64)
   //only 26 (3^3-1, where 1 is "inside" cube) of these cases are valid.
   //the first 6 numbers in each row are the indices of the bbox vertices that
   //form the outline of which we want to compute the area (counterclockwise
   //ordering), the 7th entry means the number of vertices in the outline.
   //there are 6 cases with a single face and and a 4-vertex outline, and
   //20 cases with 2 or 3 faces and a 6-vertex outline. a value of 0 indicates
   //an invalid case.
   //-----------------------------------------------------------------------------
   const char indexlist[64][7] =
   {
      {-1,-1,-1,-1,-1,-1,   0}, // 0 inside
      { 0, 4, 7, 3,-1,-1,   4}, // 1 left
      { 1, 2, 6, 5,-1,-1,   4}, // 2 right
      {-1,-1,-1,-1,-1,-1,   0}, // 3 -
      { 0, 1, 5, 4,-1,-1,   4}, // 4 bottom
      { 0, 1, 5, 4, 7, 3,   6}, // 5 bottom, left
      { 0, 1, 2, 6, 5, 4,   6}, // 6 bottom, right
      {-1,-1,-1,-1,-1,-1,   0}, // 7 -
      { 2, 3, 7, 6,-1,-1,   4}, // 8 top
      { 0, 4, 7, 6, 2, 3,   6}, // 9 top, left
      { 1, 2, 3, 7, 6, 5,   6}, //10 top, right
      {-1,-1,-1,-1,-1,-1,   0}, //11 -
      {-1,-1,-1,-1,-1,-1,   0}, //12 -
      {-1,-1,-1,-1,-1,-1,   0}, //13 -
      {-1,-1,-1,-1,-1,-1,   0}, //14 -
      {-1,-1,-1,-1,-1,-1,   0}, //15 -
      { 0, 3, 2, 1,-1,-1,   4}, //16 front
      { 0, 4, 7, 3, 2, 1,   6}, //17 front, left
      { 0, 3, 2, 6, 5, 1,   6}, //18 front, right
      {-1,-1,-1,-1,-1,-1,   0}, //19 -
      { 0, 3, 2, 1, 5, 4,   6}, //20 front, bottom
      { 1, 5, 4, 7, 3, 2,   6}, //21 front, bottom, left
      { 0, 3, 2, 6, 5, 4,   6}, //22 front, bottom, right
      {-1,-1,-1,-1,-1,-1,   0}, //23 -
      { 0, 3, 7, 6, 2, 1,   6}, //24 front, top
      { 0, 4, 7, 6, 2, 1,   6}, //25 front, top, left
      { 0, 3, 7, 6, 5, 1,   6}, //26 front, top, right
      {-1,-1,-1,-1,-1,-1,   0}, //27 -
      {-1,-1,-1,-1,-1,-1,   0}, //28 -
      {-1,-1,-1,-1,-1,-1,   0}, //29 -
      {-1,-1,-1,-1,-1,-1,   0}, //30 -
      {-1,-1,-1,-1,-1,-1,   0}, //31 -
      { 4, 5, 6, 7,-1,-1,   4}, //32 back
      { 0, 4, 5, 6, 7, 3,   6}, //33 back, left
      { 1, 2, 6, 7, 4, 5,   6}, //34 back, right
      {-1,-1,-1,-1,-1,-1,   0}, //35 -
      { 0, 1, 5, 6, 7, 4,   6}, //36 back, bottom
      { 0, 1, 5, 6, 7, 3,   6}, //37 back, bottom, left
      { 0, 1, 2, 6, 7, 4,   6}, //38 back, bottom, right
      {-1,-1,-1,-1,-1,-1,   0}, //39 -
      { 2, 3, 7, 4, 5, 6,   6}, //40 back, top
      { 0, 4, 5, 6, 2, 3,   6}, //41 back, top, left
      { 1, 2, 3, 7, 4, 5,   6}, //42 back, top, right
      {-1,-1,-1,-1,-1,-1,   0}, //43 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //44 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //45 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //46 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //47 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //48 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //49 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //50 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //51 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //52 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //53 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //54 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //55 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //56 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //57 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //58 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //59 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //60 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //61 invalid
      {-1,-1,-1,-1,-1,-1,   0}, //62 invalid
      {-1,-1,-1,-1,-1,-1,   0}  //63 invalid
   };
   
   //----------------------------------------------------------------------------
   // calculateBoxArea: computes the screen-projected 2D area of an oriented 3D
   // bounding box
   //----------------------------------------------------------------------------
   bool calculateBoxAreaOrig(
      float& area,
      XMVECTOR boxEye,
      XMVECTOR boxMin,
      XMVECTOR boxMax,
      XMMATRIX boxToWorld,
      XMMATRIX worldToScreen)
   {
      XMVECTOR minFlags = XMVectorLess(boxEye, boxMin);
      XMVECTOR maxFlags = XMVectorGreater(boxEye, boxMax);

      static const XMVECTORI minMask = { 1, 4, 16, 0 };
      static const XMVECTORI maxMask = { 2, 8, 32, 0 };

      minFlags = XMVectorAndInt(minFlags, (const XMVECTOR&)minMask);
      maxFlags = XMVectorAndInt(maxFlags, (const XMVECTOR&)maxMask);

      XMVECTOR flags = XMVectorOrInt(minFlags, maxFlags);
      
      XMVECTOR vertexBox[8];

      static const XMVECTORI Select100 = {XM_SELECT_1, XM_SELECT_0, XM_SELECT_0, XM_SELECT_0};
      static const XMVECTORI Select110 = {XM_SELECT_1, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0};
      static const XMVECTORI Select010 = {XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0};
      static const XMVECTORI Select001 = {XM_SELECT_0, XM_SELECT_0, XM_SELECT_1, XM_SELECT_0};
      static const XMVECTORI Select101 = {XM_SELECT_1, XM_SELECT_0, XM_SELECT_1, XM_SELECT_0};
      static const XMVECTORI Select011 = {XM_SELECT_0, XM_SELECT_1, XM_SELECT_1, XM_SELECT_0};

      vertexBox[0] = boxMin;
      vertexBox[1] = XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select100);
      vertexBox[2] = XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select110);
      vertexBox[3] = XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select010);
      vertexBox[4] = XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select001);
      vertexBox[5] = XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select101);
      vertexBox[6] = boxMax;
      vertexBox[7] = XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select011);

      int pos = (uint&)flags.x + (uint&)flags.y + (uint&)flags.z;

      int num = indexlist[pos][6]; //look up number of vertices in outline
      if (!num) 
         return false;       //zero indicates invalid case, return -1
      
      XMVECTOR dst[6];
      for(int i = 0; i < num; i++) //transform all outline corners into 2D screen space
      {
         XMVECTOR v = XMVector3Transform(vertexBox[indexlist[pos][i]], boxToWorld);
         dst[i] = XMVector3Transform(v, worldToScreen);
         
         XMVECTOR w = XMVectorSplatW(dst[i]);
         if (XMVector4LessOrEqual(w, XMVectorZero()))
            return false;
         dst[i] = dst[i] * XMVectorReciprocalEst(w);
      }

      float sum = 0.0f; 
      sum = (dst[num-1].x - dst[0].x) * (dst[num-1].y + dst[0].y);
      for (int i=0; i<num-1; i++)
         sum += (dst[i].x - dst[i+1].x) * (dst[i].y + dst[i+1].y);

      area = sum * 0.5f; //return computed value corrected by 0.5
      return true;
   }
   
   //==============================================================================
   // calculateBoxArea
   //==============================================================================
   bool calculateBoxArea(
      float& area,
      XMVECTOR boxEye,
      XMVECTOR boxMin,
      XMVECTOR boxMax,
      XMMATRIX boxToWorld,
      XMMATRIX worldToScreen)
   {
      XMVECTOR minFlags = XMVectorLess(boxEye, boxMin);
      XMVECTOR maxFlags = XMVectorGreater(boxEye, boxMax);

      static const XMVECTORI minMask = { 1, 4, 16, 0 };
      static const XMVECTORI maxMask = { 2, 8, 32, 0 };

      minFlags = XMVectorAndInt(minFlags, (const XMVECTOR&)minMask);
      maxFlags = XMVectorAndInt(maxFlags, (const XMVECTOR&)maxMask);

      XMVECTOR flags = XMVectorOrInt(minFlags, maxFlags);

      static const XMVECTORI Select100 = {XM_SELECT_1, XM_SELECT_0, XM_SELECT_0, XM_SELECT_0};
      static const XMVECTORI Select110 = {XM_SELECT_1, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0};
      static const XMVECTORI Select010 = {XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0};
      static const XMVECTORI Select001 = {XM_SELECT_0, XM_SELECT_0, XM_SELECT_1, XM_SELECT_0};
      static const XMVECTORI Select101 = {XM_SELECT_1, XM_SELECT_0, XM_SELECT_1, XM_SELECT_0};
      static const XMVECTORI Select011 = {XM_SELECT_0, XM_SELECT_1, XM_SELECT_1, XM_SELECT_0};

      XMMATRIX xform = boxToWorld * worldToScreen;
      
      XMVECTOR dst[8];
      dst[0] = XMVector3Transform(boxMin, xform);
      dst[1] = XMVector3Transform(XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select100), xform);
      dst[2] = XMVector3Transform(XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select110), xform);
      dst[3] = XMVector3Transform(XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select010), xform);
      dst[4] = XMVector3Transform(XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select001), xform);
      dst[5] = XMVector3Transform(XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select101), xform);
      dst[6] = XMVector3Transform(boxMax, xform);
      dst[7] = XMVector3Transform(XMVectorSelect(boxMin, boxMax, (const XMVECTOR&)Select011), xform);
                  
      XMVECTOR l = XMVectorMin(dst[0], dst[1]);
      l = XMVectorMin(l, dst[2]);
      l = XMVectorMin(l, dst[3]);
      l = XMVectorMin(l, dst[4]);
      l = XMVectorMin(l, dst[5]);
      l = XMVectorMin(l, dst[6]);
      l = XMVectorMin(l, dst[7]);
      XMVECTOR w = XMVectorSplatW(l);
            
      dst[0] = dst[0] * XMVectorReciprocalEst(XMVectorSplatW(dst[0]));
      dst[1] = dst[1] * XMVectorReciprocalEst(XMVectorSplatW(dst[1]));
      dst[2] = dst[2] * XMVectorReciprocalEst(XMVectorSplatW(dst[2]));
      dst[3] = dst[3] * XMVectorReciprocalEst(XMVectorSplatW(dst[3]));
      dst[4] = dst[4] * XMVectorReciprocalEst(XMVectorSplatW(dst[4]));
      dst[5] = dst[5] * XMVectorReciprocalEst(XMVectorSplatW(dst[5]));
      dst[6] = dst[6] * XMVectorReciprocalEst(XMVectorSplatW(dst[6]));
      dst[7] = dst[7] * XMVectorReciprocalEst(XMVectorSplatW(dst[7]));
      
      if (XMVector4LessOrEqual(w, XMVectorZero()))
         return false;
                           
      int pos = (uint&)flags.x + (uint&)flags.y + (uint&)flags.z;
      const char* pIndices = &indexlist[pos][0];
      
      int num = pIndices[6]; //look up number of vertices in outline
      if (!num) 
         return false;       //zero indicates invalid case, return -1
            
      double sum = (dst[pIndices[0]].x - dst[pIndices[1]].x) * (dst[pIndices[0]].y + dst[pIndices[1]].y);
      for (int i = 1; i < num - 1; i++)
         sum += (dst[pIndices[i]].x - dst[pIndices[i + 1]].x) * (dst[pIndices[i]].y + dst[pIndices[i + 1]].y);
      
      sum += (dst[pIndices[num - 1]].x - dst[pIndices[0]].x) * (dst[pIndices[num - 1]].y + dst[pIndices[0]].y);

      area = static_cast<float>(sum * 0.5f); 
      return true;
   }

   static const XMVECTOR gFrustVerts[] = 
   {
      {-1,-1,0,1},
      {1,-1,0,1},
      {1,1,0,1},
      {-1,1,0,1},

      {-1,-1,1,1},
      {1,-1,1,1},
      {1,1,1,1},
      {-1,1,1,1}
   };

   //==============================================================================
   // computeFrustumVertices
   //==============================================================================
   void computeFrustumVertices(XMVECTOR* RESTRICT pVerts, XMMATRIX proj) 
   {
      BDEBUG_ASSERT(pVerts);
      
      XMVECTOR det;
      XMMATRIX invProj = XMMatrixInverse(&det, proj);

      pVerts[0] = XMVector3TransformCoord(gFrustVerts[0], invProj);
      pVerts[1] = XMVector3TransformCoord(gFrustVerts[1], invProj);
      pVerts[2] = XMVector3TransformCoord(gFrustVerts[2], invProj);
      pVerts[3] = XMVector3TransformCoord(gFrustVerts[3], invProj);
      
      pVerts[4] = XMVector3TransformCoord(gFrustVerts[4], invProj);
      pVerts[5] = XMVector3TransformCoord(gFrustVerts[5], invProj);
      pVerts[6] = XMVector3TransformCoord(gFrustVerts[6], invProj);
      pVerts[7] = XMVector3TransformCoord(gFrustVerts[7], invProj);
   }
   
   //==============================================================================
   // computeBoundsOfPoints
   //==============================================================================
   void computeBoundsOfPoints(const XMVECTOR* RESTRICT pPoints, uint numPoints, XMVECTOR* RESTRICT pMin, XMVECTOR* RESTRICT pMax)
   {
      XMVECTOR minP = pPoints[0];
      XMVECTOR maxP = pPoints[0];
      
      for (uint i = 1; i < numPoints; i++)
      {
         minP = XMVectorMin(minP, pPoints[i]);
         maxP = XMVectorMax(maxP, pPoints[i]);
      }
      
      *pMin = minP;
      *pMax = maxP;
   }

   //==============================================================================   
   // closestPointOnLineSegmentTheta
   //==============================================================================
   void closestPointOnLineSegmentTheta(XMVECTOR point, XMVECTOR segmentA, XMVECTOR segmentB, XMVECTOR& axis, XMVECTOR& theta)
   {
      XMVECTOR ap = XMVectorSubtract(point,segmentA);
      axis = XMVectorSubtract(segmentB, segmentA);
      XMVECTOR dotAxisV = XMVectorReciprocal(XMVector3Dot(axis, axis));
      XMVECTOR dotAPABV = XMVector3Dot(ap,axis);
      theta = XMVectorMultiply(dotAPABV, dotAxisV);      
   }

   //==============================================================================   
   // closestPointOnLineSegment
   //==============================================================================
   void closestPointOnLineSegment(XMVECTOR point, XMVECTOR segmentA, XMVECTOR segmentB, XMVECTOR& result)
   {
      XMVECTOR oneV = XMVectorSet(1,1,1,1);

      XMVECTOR tV;
      XMVECTOR axis;
      closestPointOnLineSegmentTheta(point, segmentA, segmentB, axis, tV);

      // clamp the theta to force the point to be within the line segment
      tV = XMVectorClamp(tV, XMVectorZero(), oneV);       
      result = XMVectorMultiplyAdd(axis, tV, segmentA);               
   }

   //==============================================================================   
   // closestPointOnLineSegment
   //==============================================================================
   void closestPointOnLine(XMVECTOR point, XMVECTOR segmentA, XMVECTOR segmentB, XMVECTOR& result)
   {      
      XMVECTOR tV;
      XMVECTOR axis;
      closestPointOnLineSegmentTheta(point, segmentA, segmentB, axis, tV);
      result = XMVectorMultiplyAdd(axis, tV, segmentA);               
   }

   //==============================================================================   
   // pointInsideCylinder
   //==============================================================================
   bool pointInsideCylinder(XMVECTOR point, XMVECTOR cylinderStart, XMVECTOR cylinderEnd, XMVECTOR radiusV)
   {      
      XMVECTOR oneV = XMVectorSet(1,1,1,1);
      
      XMVECTOR tV;
      XMVECTOR axis;
      closestPointOnLineSegmentTheta(point, cylinderStart, cylinderEnd, axis, tV);

      if (XMVector3Greater(tV, oneV))
         return false;
      else if (XMVector3Less(tV, XMVectorZero()))
         return false;

      // passed the height test
      tV = XMVectorClamp(tV, XMVectorZero(), oneV);

      XMVECTOR pointOnLine = XMVectorMultiplyAdd(axis, tV, cylinderStart);

      XMVECTOR distanceV = XMVector3Length(XMVectorSubtract(point, pointOnLine));
      if (XMVector3Greater(distanceV, radiusV))
         return false;

      return true;
   }
   
} // BVMXIntersection 
