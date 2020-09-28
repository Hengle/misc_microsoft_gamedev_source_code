//============================================================================
//
// File: hyperRayLine.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "generalVector.h"

template<class T>
struct HyperRay
{
   T o, d;

   HyperRay(void) 
   { 
   }
   
   HyperRay(const T& origin, const T& dir) : o(origin), d(dir) 
   { 
   }

   T evaluate(float t) const
   {
      return o + d * t;
   }

   const T& origin(void) const { return o; }
         T& origin(void)       { return o; }

   const T& dir(void) const { return d; }
         T& dir(void)       { return d; }
};

typedef HyperRay<BVec2> Ray2;
typedef HyperRay<BVec3> Ray3;

template<class T>
struct HyperLineSegment
{
   T s, e;
   
   HyperLineSegment() 
   { 
   }

   HyperLineSegment(const T& start, const T& end) : s(start), e(end) 
   { 
   }

   const T& start(void) const { return s; }
         T& start(void)       { return s; }
   
   const T& end(void) const   { return e; }
         T& end(void)         { return e; }
         
   const T& operator[](int i) const { return (&s)[debugRangeCheck(i, 2)]; }
         T& operator[](int i)       { return (&s)[debugRangeCheck(i, 2)]; }

   T vector(void) const
   {
      return end - start;
   }

   T unitDir(void) const
   {
      return vector.normalize();
   }

   float len(void) const
   {
      return vector.len();
   }

   T evaluate(float t) const
   {  
      return T::lerp(s, e, t);
   }
   
   T evaluateInverted(float t) const
   {  
      return T::lerp(e, s, t);
   }

   HyperRay<T> ray(void) const
   {
      return HyperRay<T>(s, unitDir());
   }

   const HyperLineSegment& invert(void) 
   {
      std::swap(s, e);
      return *this;
   }
      
   HyperLineSegment inverted(void) const
   {
      return HyperLineSegment(e, s);
   }

   static bool equalTol(const HyperLineSegment& a, const HyperLineSegment& b, float tol = Math::fSmallEpsilon)
   {
      return T::equalTol(a.s, b.s, tol) && T::equalTol(a.e, b.e, tol);
   }
};

typedef HyperLineSegment<BVec2> Line2;
typedef HyperLineSegment<BVec3> Line3;
