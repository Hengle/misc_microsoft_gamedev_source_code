// File: volumeIntersection.h
#pragma once

#include "math\generateCombinations.h"

inline void compareAbsAndSwap(double& a, double& b)
{
   if (fabs(a) > fabs(b))
      std::swap(a, b);
}

template<typename Vec3Array, uint cNumExpectedPlaneCombinations>
inline void calcVolumeIntersection(Vec3Array& points, const Plane* pPlanes, uint numPlanes)
{
   BStaticArray<WORD, cNumExpectedPlaneCombinations> combinations;
   uint numCombinations = GenerateCombinations(combinations, numPlanes, 3);

   points.resize(0);

   // Compute the intersection of every 3 planes, discarding the points which are not inside or touching the polyhedron.
   // Dark voodoo here.
   for (uint i = 0; i < numCombinations; i++)
   {
      const uint a = combinations[i*3+0];
      const uint b = combinations[i*3+1];
      const uint c = combinations[i*3+2];
      const Plane& p1 = pPlanes[a];
      const Plane& p2 = pPlanes[b];
      const Plane& p3 = pPlanes[c];

      double det;
      det  =  p1.n[0]*(p2.n[1]*p3.n[2]-p2.n[2]*p3.n[1]);
      det -=  p2.n[0]*(p1.n[1]*p3.n[2]-p1.n[2]*p3.n[1]);
      det +=  p3.n[0]*(p1.n[1]*p2.n[2]-p1.n[2]*p2.n[1]);

      if (Math::EqualTol<double>(det, 0.0f, Math::fMinuteEpsilon))
         continue;

      const BVec3D p1n(p1.n);
      const BVec3D p2n(p2.n);
      const BVec3D p3n(p3.n);
                  
      const BVec3D point( 
         ( ((double)p1.d * (p2n % p3n)) +
           ((double)p2.d * (p3n % p1n)) +
           ((double)p3.d * (p1n % p2n)) ) / (double)det );
      
      uint j;
      for (j = 0; j < numPlanes; j++)
      {
         if ((j == a) || (j == b) || (j == c))
            continue;
            
         double v[4];
         v[0] = pPlanes[j].n[0] * point[0];
         v[1] = pPlanes[j].n[1] * point[1];
         v[2] = pPlanes[j].n[2] * point[2];
         v[3] = -pPlanes[j].d;
                  
         compareAbsAndSwap(v[0], v[2]);
         compareAbsAndSwap(v[1], v[3]);
         compareAbsAndSwap(v[0], v[1]);
         compareAbsAndSwap(v[2], v[3]);
         compareAbsAndSwap(v[1], v[2]);
         
         //BDEBUG_ASSERT(fabs(v[0]) <= fabs(v[1]));
         //BDEBUG_ASSERT(fabs(v[1]) <= fabs(v[2]));
         //BDEBUG_ASSERT(fabs(v[2]) <= fabs(v[3]));
                           
         double sum = v[0];
         for (uint q = 1; q < 4; q++)
            sum += v[q];
         
         if (sum < -Math::fSmallEpsilon)
            break;
      }
      if (j < numPlanes)
         continue;

      points.pushBack(point);
   }
}
