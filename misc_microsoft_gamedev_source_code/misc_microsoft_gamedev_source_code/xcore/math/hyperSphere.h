//============================================================================
//
// File: hypperSphere.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "generalVector.h"

template<class T>
struct Hypersphere
{
   T o;
   float r;

   Hypersphere()
   {
   }

   Hypersphere(const T& origin, float radius) : o(origin), r(radius)
   {
   }

   void clear(void)
   {
      o.setZero();
      r = 0;
   }
   
   Hypersphere(eClear e)
   {
      clear();
   }

   const T& origin(void) const   { return o; }
         T& origin(void)         { return o; }

   const float  radius(void) const  { return r; }
         float& radius(void)        { return r; }

   float dist2(const T& p) const
   {
      return (p - o).norm();
   }

   bool contains(const T& p) const
   {
      return dist2(p) < r * r;
   }

   bool containsOrTouches(const T& p) const
   {
      return dist2(p) <= r * r;
   }
   
   bool touches(const Hypersphere& h) const
   {
      return dist2(h.o) <= Math::Sqr(r + h.r);
   }

   Hypersphere& expand(const T& point)
   {
      const float dist2 = (point - o).squaredLen();
      if (dist2 > r * r)
         r = sqrt(dist2);
   }
   
   void log(BTextDispatcher& log) const
   {
      log.printf("Origin: ");
      for (uint i = 0; i < T::numElements; i++)
         log.printf("%f ", o[i]);
      log.printf("Radius: %f\n", r);
   }
   
   friend BStream& operator<< (BStream& dst, const Hypersphere& a)
   {
      return dst << a.o << a.r;
   }
   
   friend BStream& operator>> (BStream& src, Hypersphere& a)
   {
      return src >> a.o >> a.r;
   }
};

typedef Hypersphere<BVec2> Circle;
typedef Hypersphere<BVec3> Sphere;
