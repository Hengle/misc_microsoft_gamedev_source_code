//============================================================================
//
// File: plane2.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "generalVector.h"

struct Plane2
{
   BVec2 n;
   float d;
   
   Plane2()
   {
   }
   
   Plane2(float x, float y, float dist) : n(x, y), d(dist)
   {
   }
   
   Plane2(const BVec2& a, const BVec2& b)
   {
      setFromSegment(a, b);
   }
   
   Plane2(const BVec2& norm, float dist) : n(norm), d(dist)
   {
   }
         
   bool isValid(void) const
   {
      return 0.0f != n.norm();
   }
   
   Plane2& setInvalid(void) 
   {
      n.setZero();
      d = 0;
      return *this;
   }
   
   void clear(void)
   {
      setInvalid();
   }
   
   // true on failure
   // Positive halfspace will be to the right of the line.
   bool setFromSegment(const BVec2& a, const BVec2& b, float eps = Math::fTinyEpsilon)
   {
      n = a - b;//b - a;
      if (n.len() < eps)
      {
         n.setZero();
         d = 0.0f;
         return true;
      }

      n = n.normalize().perp();
      d = n * b;
      
      return false;
   }
   
   Plane2& setFromNormalOrigin(const BVec2& norm, const BVec2& org)
   {
      n = norm;
      d = norm * org;
      return *this;
   }

#if 0   
   Plane2& setFromRay(const Ray2& ray)
   {
      n = ray.dir().perp();
      d = n * ray.origin();
      return *this;
   }
#endif   
   
   Plane2& normalize(void)
   {
      float len = n.tryNormalize();
      if (0.0f == len)
      {
         setInvalid();
         return *this;
      }
      d /= len;
      return *this;
   }
   
   BVec2 origin(void) const
   {
      return n * d;
   }
   
   const BVec2& normal(void) const   { return n; }
         BVec2& normal(void)         { return n; }

   const float& distance(void) const { return d; }
         float& distance(void)       { return d; }
         
   // Returns line's direction (perp to normal).
   BVec2 dir(void) const
   {
      return n.perp();
   }            
         
   bool operator== (const Plane2& b) const
   {
      return (n == b.n) && (d == b.d);
   }

   bool operator!= (const Plane2& b) const
   {
      return !(*this == b);
   }

   float distanceToPoint(const BVec2& p) const
   {
      return p * n - d;
   }

   Plane2 flipped(void) const
   {
      return Plane2(-n, -d);
   }
   
   BVec3 equation(void) const
   {
      return BVec3(n[0], n[1], -d);
   }
   
   const Plane2& set(float x, float y, float dist)
   {
      n[0] = x;
      n[1] = y;
      d = dist;
      return *this;
   }

   const Plane2& setFromEquation(const BVec3& v) 
   {
      n = v;
      d = -v[3];
      return *this;
   }

   // x0 axis change with respect to axis x1
   float gradient(int x0, int x1, float eps = Math::fTinyEpsilon) const
   {
      const float dA = -n[x1];
      const float dB = n[x0];
      return (fabs(dB) <= eps) ? 0.0f : dA / dB;
   }
   
   BVec2 project(const BVec2& p) const
   {
      return p - n * distanceToPoint(p);
   }
               
   void clipPoly(BDynamicArray<BVec2>& result, const BDynamicArray<BVec2>& verts) const
   {
      result.resize(0);

      if (verts.empty())
         return;

      float prevDist = distanceToPoint(verts[0]);
      const int numVerts = static_cast<int>(verts.size());

      for (int prev = 0; prev < numVerts; prev++)
      {
         int cur = Math::NextWrap(prev, numVerts);
         float curDist = distanceToPoint(verts[cur]);

         if (prevDist >= 0.0f)
            result.pushBack(verts[prev]);

         if (((prevDist < 0.0f) && (curDist > 0.0f)) ||
            ((prevDist > 0.0f) && (curDist < 0.0f)))
         {
            result.pushBack(BVec2::lerp(verts[prev], verts[cur], prevDist / (prevDist - curDist)));
         }

         prevDist = curDist;
      }
   }
   
   // Intersects two planes.
   // true on failure
   static bool intersect(BVec2& p, const Plane2& u, const Plane2& w, float eps = .00000000125f)
   {
      const float a = u.n[0];
      const float b = u.n[1];
      const float c = -u.d;
      
      const float d = w.n[0];
      const float e = w.n[1];
      const float f = -w.d;
                        
      const double r = d * b - e * a;
      if (fabs(r) < eps)
      {
         p.setZero();
         return true;
      }
         
      p[0] = static_cast<float>((e * c - f * b) / r);
      p[1] = static_cast<float>((f * a - d * c) / r);
      
      return false;
   }
   
};


