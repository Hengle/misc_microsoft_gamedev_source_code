// rg [8/13/05] - Class derived from this source code:
// http://cm.bell-labs.com/who/clarkson/2dch.c

/*
* Ken Clarkson wrote this.  Copyright (c) 1996 by AT&T..
* Permission to use, copy, modify, and distribute this software for any
* purpose without fee is hereby granted, provided that this entire notice
* is included in all copies of any software which is or includes a copy
* or modification of this software and in all copies of the supporting
* documentation for such software.
* THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
* WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR AT&T MAKE ANY
* REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
* OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
*/

/*
* two-dimensional convex hull
* read points from stdin,
*      one point per line, as two numbers separated by whitespace
* on stdout, points on convex hull in order around hull, given
*      by their numbers in input order
* the results should be "robust", and not return a wildly wrong hull,
*	despite using floating point
* works in O(n log n); I think a bit faster than Graham scan;
* 	somewhat like Procedure 8.2 in Edelsbrunner's "Algorithms in Combinatorial
*	Geometry", and very close to:
*	    A.M. Andrew, "Another Efficient Algorithm for Convex Hulls in Two Dimensions",
*		Info. Proc. Letters 9, 216-219 (1979)
*	(See also http://geometryalgorithms.com/Archive/algorithm_0110/algorithm_0110.htm)
*/

#pragma once

#include "math/generalVector.h"

class BConvexHull2
{
public:
   BConvexHull2(uint maxPoints) :
      mpPoints(NULL),
      mNumPoints(0),
      mpVerts(maxPoints + 1),
      mNumVerts(0)
      
   {
   }
               
   int calculate(const BVec2* pPoints, uint n)
   {
      mpPoints = pPoints;
      BDEBUG_ASSERT(n < mpVerts.size());
               
      for (uint i = 0; i < n; i++)
         mpVerts[i] = &pPoints[i];
      
      // make lower hull 
      const int u = makeChain(&mpVerts[0], n, cmpl);		
      if (!n) 
      {
         mNumVerts = 0;
         return 0;
      }
         
      mpVerts[n] = mpVerts[0];
      
      // make upper hull
      mNumVerts = u + makeChain(&mpVerts[u], n - u + 1, cmph);	
      return mNumVerts;
   }
   
   uint numVerts(void) const { return mNumVerts; }
   uint vertex(uint i) const { return mpVerts[i] - mpPoints; }
         
private:
   const BVec2* mpPoints;
   uint mNumPoints;
   BDynamicArray<const BVec2*> mpVerts;
   uint mNumVerts;
   
   static int ccw(const BVec2** P, int i, int j, int k) 
   {
      // true if points i, j, k counterclockwise 
      return (P[i]->element[0] - P[j]->element[0]) * (P[k]->element[1] - P[j]->element[1]) - 
             (P[i]->element[1] - P[j]->element[1]) * (P[k]->element[0] - P[j]->element[0]) <= 0.0f;	   
   }

   static int cmpm(int c, const BVec2** A, const BVec2** B)
   {
      const float v = (*A)->element[c] - (*B)->element[c];
      if (v > 0.0f) 
         return 1;
      if (v < 0.0f) 
         return -1;
      return 0;
   }

   static int cmpl(const void *a, const void *b) 
   {
      const int compResult = cmpm(0, (const BVec2**)a, (const BVec2**)b);
      if (compResult)
         return compResult;
      return cmpm(1, (const BVec2**)b, (const BVec2**)(a));
   }

   static int cmph(const void *a, const void *b) 
   {
      return cmpl(b, a);
   }

   static int makeChain(const BVec2** V, int n, int (*cmp)(const void*, const void*)) 
   {
      int s = 1;
      
      qsort(V, n, sizeof(const BVec2**), cmp);
      
      for (int i = 2; i < n; i++) 
      {
         int j;
         for (j = s; j >= 1 && ccw(V, i, j, j - 1); j--)
            ;  
         
         s = j + 1;
         
         std::swap(V[s], V[i]);
      }
      
      return s;
   }
}; // class BConvexHull2
