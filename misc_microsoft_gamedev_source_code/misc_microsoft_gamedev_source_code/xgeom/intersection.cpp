//-----------------------------------------------------------------------------
// File: intersection.cpp
//-----------------------------------------------------------------------------
#include "xgeom.h"
#include "intersection.h"

#include "utils\utils.h"

#include "aabbTri.h"
                             

namespace Intersection
{
   //-----------------------------------------------------------------------------
   // AABBvsAABB
   // returns cOUTSIDE (not touching), cBINSIDEA, cAINSIDEB, or cPARTIAL (touching but no containment)
   //-----------------------------------------------------------------------------
   eResult AABBvsAABB(const AABB& a, const AABB& b)
   {
      if (!a.overlaps(b))
         return cOUTSIDE;

      if (a.contains(b))
         return cBINSIDEA;

      if (b.contains(a))
         return cAINSIDEB;

      return cPARTIAL;
   }
   
   //-----------------------------------------------------------------------------
   // AABBvsTri3
   //-----------------------------------------------------------------------------
   bool AABBvsTri3(const AABB& box, const BTri3& tri)
   {
      TriBoxReal boxcenter[3];
      TriBoxReal boxhalfsize[3];
      
      boxcenter[0] = (box[0][0] + box[1][0]) * .5f;
      boxcenter[1] = (box[0][1] + box[1][1]) * .5f;
      boxcenter[2] = (box[0][2] + box[1][2]) * .5f;

      boxhalfsize[0] = box[1][0] - boxcenter[0];
      boxhalfsize[1] = box[1][1] - boxcenter[1];
      boxhalfsize[2] = box[1][2] - boxcenter[2];
      
      TriBoxReal verts[3][3];
      verts[0][0] = tri[0][0];
      verts[0][1] = tri[0][1];
      verts[0][2] = tri[0][2];
      verts[1][0] = tri[1][0];
      verts[1][1] = tri[1][1];
      verts[1][2] = tri[1][2];
      verts[2][0] = tri[2][0];
      verts[2][1] = tri[2][1];
      verts[2][2] = tri[2][2];
      
      return 0 != TriBoxOverlap(
         boxcenter, 
         boxhalfsize,
         verts);            
   }
   
   //-----------------------------------------------------------------------------
   // AABBvsTri3
   //-----------------------------------------------------------------------------
   bool AABBvsTri3(const BVec3& center, const BVec3& extents, const BTri3& tri)
   {
      TriBoxReal boxcenter[3];
      TriBoxReal boxhalfsize[3];
      
      boxcenter[0] = center[0];
      boxcenter[1] = center[1];
      boxcenter[2] = center[2];
      
      boxhalfsize[0] = extents[0];
      boxhalfsize[1] = extents[1];
      boxhalfsize[2] = extents[2];
      
      TriBoxReal verts[3][3];
      verts[0][0] = tri[0][0];
      verts[0][1] = tri[0][1];
      verts[0][2] = tri[0][2];
      verts[1][0] = tri[1][0];
      verts[1][1] = tri[1][1];
      verts[1][2] = tri[1][2];
      verts[2][0] = tri[2][0];
      verts[2][1] = tri[2][1];
      verts[2][2] = tri[2][2];
      
      return 0 != TriBoxOverlap(
         boxcenter,
         boxhalfsize,
         verts);            
   }  
         
   //-----------------------------------------------------------------------------
   // ray3AABB
   // cINSIDE, cFAILURE, or cSUCCESS
   // coord and t not set on cFAILURE
   //-----------------------------------------------------------------------------
   eResult ray3AABB(
      BVec3& coord,
      float& t,
      const Ray3& ray,
      const AABB& box)
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
         if (ray.origin()[i] < box[0][i])
         {
            quadrant[i] = Left;
            candidatePlane[i] = box[0][i];
            inside = false;
         }
         else if (ray.origin()[i] > box[1][i])
         {
            quadrant[i] = Right;
            candidatePlane[i] = box[1][i];
            inside = false;
         }
         else
         {
            quadrant[i] = Middle;
         }
      }
      
      if (inside)
      {
         coord = ray.origin();
         t = 0.0f;
         return cINSIDE;
      }
      
      float maxT[NumDim];
      for (int i = 0; i < NumDim; i++)
      {
         if ((quadrant[i] != Middle) && (ray.dir()[i] != 0.0f))
            maxT[i] = (candidatePlane[i] - ray.origin()[i]) / ray.dir()[i];
         else
            maxT[i] = -1.0f;
      }         
      
      int whichPlane = 0;
      for (int i = 1; i < NumDim; i++)
         if (maxT[whichPlane] < maxT[i])
            whichPlane = i;
            
      if (maxT[whichPlane] < 0.0f)
         return cFAILURE;               
            
      for (int i = 0; i < NumDim; i++)
      {
         if (i != whichPlane)
         {
            coord[i] = ray.origin()[i] + maxT[whichPlane] * ray.dir()[i];
            
            if ( (coord[i] < box[0][i]) || (coord[i] > box[1][i]) )
            {
               return cFAILURE;
            }
         }
         else
         {
            coord[i] = candidatePlane[i];
         }
         
         BDEBUG_ASSERT(coord[i] >= box[0][i] && coord[i] <= box[1][i]); 
      }               
      
      t = maxT[whichPlane];
      return cSUCCESS;
   }
   
   //-----------------------------------------------------------------------------
   // ray3Plane
   // returns cPARALLEL, cFAILURE, or cSUCCESS
   //-----------------------------------------------------------------------------
   eResult ray3Plane(
      float& t,
      const Plane& plane,
      const Ray3& ray,
      bool backside,
      float maxDist,
      float parallelEps)
   {
      const float dirAlong = plane.n * ray.dir();

      if (fabs(dirAlong) <= parallelEps)
         return cPARALLEL;
               
      const float posAlong = plane.n * ray.origin();

      t = (plane.d - posAlong);
      
      if ((!backside) && (t > 0.0f))
         return cFAILURE;
      
      t = t / dirAlong;
      if (fabs(t) > maxDist)
         return cFAILURE;
               
      return cSUCCESS;
   }
   
   //-----------------------------------------------------------------------------
   // ray3AxialPlane
   // returns cPARALLEL, cFAILURE, or cSUCCESS
   //-----------------------------------------------------------------------------
   eResult ray3AxialPlane(
      float& t,
      int axis,
      float dist,
      const Ray3& ray,
      bool backside,
      float maxDist,
      float parallelEps)
   {
      debugRangeCheck(axis, 3);
      
      const float dirAlong = ray.dir()[axis];

      if (fabs(dirAlong) <= parallelEps)
         return cPARALLEL;
      
      const float posAlong = ray.origin()[axis];

      float tt = (dist - posAlong);
      
      if ((!backside) && (tt > 0.0f))
         return cFAILURE;
      
      tt = tt / dirAlong;
      if (fabs(tt) > maxDist)
         return cFAILURE;
               
      t = tt;
      return cSUCCESS;
   }
   
   //-----------------------------------------------------------------------------
   // ray3Tri
   // cSUCCESS, cFAILURE, or cPARALLEL
   //-----------------------------------------------------------------------------
   eResult ray3Tri(float& t, float& u, float& v, const Ray3& ray, const BTri3& tri, bool backFaceCulling, float epsilon)
   {
      BVec3 edge1, edge2, tvec, pvec, qvec;
      float det, invDet;
      
      edge1 = tri[1] - tri[0];
      edge2 = tri[2] - tri[0];
      
      pvec = ray.dir() % edge2;
      
      det = edge1 * pvec;
      
      if (backFaceCulling)
      {
         if (det < epsilon)
         {
            if (det > -epsilon)
               return cPARALLEL;
            return cFAILURE;
         }
         
         tvec = ray.origin() - tri[0];
         
         u = tvec * pvec;
         if ((u < 0.0f) || (u > det))
            return cFAILURE;
            
         qvec = tvec % edge1;
         
         v = ray.dir() * qvec;
         
         if ((v < 0.0f) || (u + v > det))
            return cFAILURE;
            
         t = edge2 * qvec;
         invDet = 1.0f / det;
         
         t *= invDet;
         u *= invDet;
         v *= invDet;         
      }
      else
      {
         if ((det > -epsilon) && (det < epsilon))
            return cPARALLEL;
            
         invDet = 1.0f / det;
         
         tvec = ray.origin() - tri[0];
         
         u = (tvec * pvec) * invDet;
         if ((u < 0.0f) || (u > 1.0f))      
            return cFAILURE;
            
         qvec = tvec % edge1;
         v = (ray.dir() * qvec) * invDet;
         if ((v < 0.0f) || (u + v > 1.0f))
            return cFAILURE;
            
         t = (edge2 * qvec) * invDet;
      }
      
      return cSUCCESS;      
   }      
   
   //-----------------------------------------------------------------------------
   // point2InPoly
   // Uses crossing test: Real Time Rendering, 1st Edition, page 309
   // Doesn't properly handle boundary cases 
   // cSUCCESS, cFAILURE
   //-----------------------------------------------------------------------------
   eResult point2InPolyRTR(const BVec2& t, const BVec2* pV, int n)
   {
      bool inside = false;

      BVec2 e0(pV[n - 1]);
      BVec2 e1(pV[0]);

      bool y0 = (e0[1] >= t[1]);
      for (int i = 1; i <= n; i++)
      {
         bool y1 = (e1[1] >= t[1]);

         if (y0 != y1)
         {
            if (y1 == ((e1[1] - t[1]) * (e0[0] - e1[0]) >= (e1[0] - t[0]) * (e0[1] - e1[1])))
               inside = !inside;
         }
    
         y0 = y1;

         e0 = e1;
         // This reads past the end of the array on the last loop!
         e1 = pV[i];
      }

      return inside ? cSUCCESS : cFAILURE;
   }
   
   //-----------------------------------------------------------------------------
   // point2InPoly
   // Uses crossing test. Logic is same as: 
   // http://www.ecse.rpi.edu/Homepages/wrf/research/geom/pnpoly.html
   // except division-free.
   // Properly handles cases but is slightly slower
   // cSUCCESS, cFAILURE
   //-----------------------------------------------------------------------------
   eResult point2InPoly(const BVec2& t, const BVec2* pV, int n)
   {
      bool inside = false;
      
      BVec2 e0(pV[n - 1]);
      BVec2 e1(pV[0]);
      
      // double to prevent SSE optimizations in VC71, which cause the boundary cases to fail
      typedef double XT;
      XT x = t[0];
      XT y = t[1];
      
      bool y0 = (e0[1] > t[1]);
      bool z0 = (e0[1] <= t[1]);
      for (int i = 1; i <= n; i++)
      {
         bool y1 = (e1[1] > t[1]);
         bool z1 = (e1[1] <= t[1]);

         // moved into local vars to avoid fp32 temporaries being created during the calculations,
         // which screws up boundary conditions
         XT a = e1[0];
         XT b = e0[0];
         XT c = e0[1];
         XT d = e1[1];
         
         if (z0 && y1)
         {
            if ((d - y) * (b - a) > (a - x) * (c - d))
               inside = !inside;
         }
         else if (z1 && y0)
         {
            if ((d - y) * (b - a) < (a - x) * (c - d))
               inside = !inside;
         }
                 
         y0 = y1;
         z0 = z1;
         
         e0 = e1;
         // This reads past the end of the array on the last loop!
         e1 = pV[i];
      }
      
      return inside ? cSUCCESS : cFAILURE;
   }
   
   //-----------------------------------------------------------------------------
   // point2InTriBarycentric
   //-----------------------------------------------------------------------------
   bool point2InTriBary(float& alpha, float& beta, const BVec2& t, const BVec2* pV)
   {
      const float u0 = t[0] - pV[0][0];
      const float v0 = t[1] - pV[0][1];
      
      const float u1 = pV[1][0] - pV[0][0];
      const float v1 = pV[1][1] - pV[0][1];
      const float u2 = pV[2][0] - pV[0][0];
      const float v2 = pV[2][1] - pV[0][1];
      
      if (u1 == 0.0f)
      {
         beta = u0 / u2;
         if ((beta >= 0.0f) && (beta <= 1.0f))
         {
            alpha = (v0 - beta * v2) / v1;
            return ((alpha >= 0.0f) && ((alpha + beta) <= 1.0f));
         }         
      }
      else
      {
         beta = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
         if ((beta >= 0.0f) && (beta <= 1.0f))
         {
            alpha = (u0 - beta * u2) / u1;
            return ((alpha >= 0.0f) && ((alpha + beta) <= 1.0f));
         }
      }
      
      return false;
      
      //gamma = 1 - (alpha+beta);
      //i=2,ray.normal[0] = gamma * N[0][0] + alpha * N[i-1][0] + beta * N[i][0];
   }
              
   //-----------------------------------------------------------------------------
   // sphereSphere
   // true if touching
   //-----------------------------------------------------------------------------
   bool sphereSphere(const Sphere& a, const Sphere& b)
   {
      return a.touches(b);
   }
   
   //-----------------------------------------------------------------------------
   // AABBBehindPlane
   //-----------------------------------------------------------------------------
   bool AABBBehindPlane(
      const AABB& box,
      const Plane& plane)
   {
      // --- choose the corner that will maximize the dot product
      const BVec3 p(
         (plane.n[0] > 0.0f) ? box[1][0] : box[0][0],
         (plane.n[1] > 0.0f) ? box[1][1] : box[0][1],
         (plane.n[2] > 0.0f) ? box[1][2] : box[0][2]);

      const float d = plane.distanceToPoint(p);

      return (d < 0.0f);
   }
   
   //-----------------------------------------------------------------------------
   // AABBInFrontPlane
   //-----------------------------------------------------------------------------
   bool AABBInFrontPlane(
      const AABB& box,
      const Plane& plane)
   {
      // --- choose the corner that will minimize the dot product
      const BVec3 p(
         (plane.n[0] > 0.0f) ? box[0][0] : box[1][0],
         (plane.n[1] > 0.0f) ? box[0][1] : box[1][1],
         (plane.n[2] > 0.0f) ? box[0][2] : box[1][2]);

      const float d = plane.distanceToPoint(p);

      return (d > 0.0f);
   }
   
   //-----------------------------------------------------------------------------
   // boxFrustumSlow
   // cINSIDE, cPARTIAL, or cOUTSIDE
   //-----------------------------------------------------------------------------
   eResult boxFrustumSlow(
      const AABB& box,
      const BFrustum& frustum)
   {
      bool partial = false;

      for (int planeIter = 0; planeIter < BFrustum::cPlaneMax; planeIter++)
      {
         const Plane& plane = frustum.plane(planeIter);

         int countOutside = 0;

         for (int boxCornerIter = 0; boxCornerIter < 8; boxCornerIter++)
         {
            const BVec3& corner = box.corner(boxCornerIter);

            const float d = plane.distanceToPoint(corner);

            if (d < 0.0f)
               countOutside++;
         }

         const int countInside = 8 - countOutside;

         if (!countInside)
         {
            // all corners outside, trivial reject
            return cOUTSIDE;   
         }
         else if (countInside != 8)
         {
            // points inside & outside, partial (could be a false partial!)
            partial = true;
         }
      }

      return partial ? cPARTIAL : cINSIDE;
   }
   
   //-----------------------------------------------------------------------------
   // boxFrustum
   //-----------------------------------------------------------------------------
   eResult boxFrustum(
      const AABB& box,
      const BFrustum& frustum)
   {
      bool partial = false;

      for (int planeIter = 0; planeIter < BFrustum::cPlaneMax; planeIter++)
      {
         const Plane& plane = frustum.plane(planeIter);

         if (AABBBehindPlane(box, plane))
         {
            //BDEBUG_ASSERT(cOUTSIDE == boxFrustumSlow(box, frustum));
            return cOUTSIDE;
         }
         else if (!AABBInFrontPlane(box, plane))
            partial = true;
      }
               
      const eResult result = partial ? cPARTIAL : cINSIDE;
      
      //BDEBUG_ASSERT(result == boxFrustumSlow(box, frustum));
      
      return result;
   }
   
   //-----------------------------------------------------------------------------
   // pointFrustum
   // returns cOUTSIDE or cINSIDE
   //-----------------------------------------------------------------------------
   eResult pointFrustum(
      const BVec3& point,
      const BFrustum& frustum)
   {
      for (int i = 0; i < BFrustum::cPlaneMax; i++)
      {
         const float d = frustum.plane(i).distanceToPoint(point);

         if (d < 0.0f)
            return cOUTSIDE;
      }

      return cINSIDE;
   }
   
   //-----------------------------------------------------------------------------
   // sphereFrustum
   // returns cOUTSIDE, cINSIDE, or cPARTIAL
   //-----------------------------------------------------------------------------
   eResult sphereFrustum(
      const Sphere& sphere,
      const BFrustum& frustum)
   {
      eResult result = cINSIDE;

      for (int i = 0; i < BFrustum::cPlaneMax; i++)
      {
         const float d = frustum.plane(i).distanceToPoint(sphere.origin());

         if (d < -sphere.radius())
            return cOUTSIDE;

         if (d <= sphere.radius())
            result = cPARTIAL;
      }

      return result;
   }
   
   //-----------------------------------------------------------------------------
   // raySphere
   // Returns number of intersection points.
   //-----------------------------------------------------------------------------
   int raySphere(BVec3 pPoint[2], const Ray3& ray, const Sphere& sphere)
   {
      const BVec3 diff(ray.origin() - sphere.origin());
      const float a = ray.dir().squaredLen();
      const float b = diff * ray.dir();
      const float c = diff.squaredLen() - Math::Sqr(sphere.radius());

      float m[2];
      const float k = Math::Sqr(b) - a * c;

      if (k >= 0.0f)
      {
         if (k > 0.0f)
         {
            const float r = sqrt(k);

            const float ooA = 1.0f / a;
            m[0] = (-b - r) * ooA;
            m[1] = (-b + r) * ooA;

            if (m[0] >= 0.0f)
            {
               pPoint[0] = ray.origin() + m[0] * ray.dir();
               pPoint[1] = ray.origin() + m[1] * ray.dir();
               return 2;
            }
            else if (m[1] >= 0.0f)
            {
               pPoint[0] = ray.origin() + m[1] * ray.dir();
               return 1;
            }
         }
         else
         {
            if ((m[0] = -b / a) >= 0.0f)
            {
               pPoint[0] = ray.origin() + m[0] * ray.dir();
               return 1;
            }
         }
      }

      return 0;         
   }
   
   //-----------------------------------------------------------------------------
   // conePoint
   // true if point inside cone
   //-----------------------------------------------------------------------------
   bool conePoint(const Cone& c, const BVec3& p) 
   {
      const float a = c.axis() * (p - c.apex());

      if (a < 0.0f) 
         return false;

      return ((p - c.apex()).len2() * c.cosAng() * c.cosAng()) <= (a * a);
   }
   
   //-----------------------------------------------------------------------------
   // coneSphere
   // true if sphere touches or inside cone
   //-----------------------------------------------------------------------------
   bool coneSphere(const Cone& c, const Sphere& s)
   {
      const BVec3 diff(s.origin() - c.apex());
      const float frsqr = s.radius() * s.radius();
      const float flsqr = diff.len2();
      if (flsqr <= frsqr)
         return true;

      const float fdot = diff * c.axis();
      const float fdotsqr = fdot * fdot;
      const float fcossqr = c.cosAng() * c.cosAng();
      if ((fdotsqr >= flsqr * fcossqr) && (fdot > 0.0f))
         return true;

      const float fulen = sqrt(fabs(flsqr - fdotsqr));
      const float ftest = c.cosAng() * fdot + c.sinAng() * fulen;
      const float fdiscr = ftest * ftest - flsqr + frsqr;
      return fdiscr >= 0.0f && ftest >= 0.0f;
   }
   
   // See "Fast Ray-Box Intersection", Graphics Gems 2
   eResult line3AABB(const Line3& line, const AABB& box) 
   {
      bool anyStartOutside = false;
      bool anyEndOutside = false;

      for (int i = 0; i < 3; i++)
      {
         int j = i + 1; if (j == 3) j = 0;
         int k = j + 1; if (k == 3) k = 0;

         for (int s = 0; s < 2; s++)
         {
            const float normalDir = float(s * 2 - 1);
            const float sDist = normalDir * (line.start()[i] - box[s][i]);
            const float eDist = normalDir * (line.end()[i] - box[s][i]);
            const bool sOutside = sDist > 0.0f;
            const bool eOutside = eDist > 0.0f;
            
            // apply separating axis theorem
            if ((sOutside) && (eOutside))
               return cFAILURE;

            anyStartOutside = anyStartOutside || sOutside;
            anyEndOutside = anyEndOutside || eOutside;

            if (sOutside != eOutside)
            {
               const float fract = sDist / (sDist - eDist);
               const float x = Math::Lerp(line.start()[j], line.end()[j], fract);
               if ((x >= box.low()[j]) && (x <= box.high()[j]))
               {
                  const float y = Math::Lerp(line.start()[k], line.end()[k], fract);
                  if ((y >= box.low()[k]) && (y <= box.high()[k]))
                     return cSUCCESS;
               }
            }
         }
      }

      if ((anyStartOutside) && (anyEndOutside))
         return cFAILURE;

      return cSUCCESS;
   }

   EDistFromLineSegmentResult distFromLineSegment(
      const Line2& l,
      const BVec2& p,
      float& distFrom,
      float& distAlong,
      float eps)
   {
      const BVec2& a = l.start();
      const BVec2& b = l.end();
               
      distFrom = (p[0] - a[0]) * (b[1] - a[1]) - (p[1] - a[1]) * (b[0] - a[0]);
               
      float len2 = a.dist2(b);
      if (len2 < eps)
         return eInvalidLine;

      const bool neg = (distFrom < 0.0f);         
      
      distFrom = (distFrom * distFrom) / len2;
                        
      distAlong = p.dist2(a) - distFrom;
      
      distFrom = neg ? -distFrom : distFrom;

      float t = (p[0] - a[0]) * (b[0] - a[0]) +
                (p[1] - a[1]) * (b[1] - a[1]);

      if (t <= 0.0f)
      {
         //distFrom = a.dist2(p);
         return ePBeforeA;
      }

      t = (b[0] - p[0]) * (b[0] - a[0]) +
          (b[1] - p[1]) * (b[1] - a[1]);

      if (t < 0.0f)
      {
         //distFrom = b.dist2(p);
         return ePAfterB;
      }
      else
      {
         
         return ePWithinAB;
      }
   }
         
   eResult line2Plane2(Line2& result, const Line2& line, const Plane2& plane2)
   {
      float aDist = plane2.distanceToPoint(line[0]);
      float bDist = plane2.distanceToPoint(line[1]);
      
      result = line;
      
      if ((aDist < 0.0f) && (bDist < 0.0f))
         return cOUTSIDE;
      
      if ((aDist >= 0.0f) && (bDist >= 0.0f))
         return cINSIDE;
                           
      if (aDist < 0.0f)
         result[0] = line.evaluate(aDist / (aDist - bDist ));
      else
         result[1] = line.evaluateInverted(bDist / (bDist - aDist));
      
      return cPARTIAL;
   }
   
   // Reference: Graphics Gems 2, "Intersection of Three Planes", page 305
   // Returns true on failure.
   bool threePlanes(BVec3& dest, const Plane& p1, const Plane& p2, const Plane& p3, float eps)
   {
      double det;
      det  =  p1.n[0]*(p2.n[1]*p3.n[2]-p2.n[2]*p3.n[1]);
      det -=  p2.n[0]*(p1.n[1]*p3.n[2]-p1.n[2]*p3.n[1]);
      det +=  p3.n[0]*(p1.n[1]*p2.n[2]-p1.n[2]*p2.n[1]);

      if (Math::EqualTol<double>(det, 0.0f, eps))
         return true;

      dest = ( (p1.d * (p2.n % p3.n)) +
               (p2.d * (p3.n % p1.n)) +
               (p3.d * (p1.n % p2.n)) ) / (float)det;

      return false;
   }
   
   bool twoPlanes(Ray3& result, const Plane& p1, const Plane& p2, float eps)
   {
      result.d = p1.normal() % p2.normal();
      if (result.d.len2() < eps)
         return true;
      
      result.d.normalize();
               
      const int largestAxis = result.d.majorAxis();
      const int xAxis = Math::NextWrap(largestAxis, 3);
      const int yAxis = Math::NextWrap(xAxis, 3);
               
      const float a1 = p1.normal()[xAxis];
      const float b1 = p1.normal()[yAxis];
      const float d1 = -p1.distance();
      
      const float a2 = p2.normal()[xAxis];
      const float b2 = p2.normal()[yAxis];
      const float d2 = -p2.distance();
      
      const double d = a1 * b2 - a2 * b1;         
      const double denomEps = Math::fMinuteEpsilon;
      if (fabs(d) < denomEps)
         return true;
                     
      result.o[largestAxis] = 0.0f;
      result.o[xAxis] = float((b1 * d2 - b2 * d1) / d);
      result.o[yAxis] = float((a2 * d1 - a1 * d2) / d);
         
      return false;
   }

} // namespace intersection
