//============================================================================
//
// File: generateCombinations.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "containers\staticArray.h"

/*
** Compute C(n,m) = the number of combinations of n items,
** taken m at a time.
**
** Written by Thad Smith III, Boulder County, CO.
** Released to the Public Domain  10/14/91.
**
** The first version will not overflow if C(n,m) * (n-m+1) < ULONG_MAX.
** The second version will not overflow if C(n,m) < ULONG_MAX, but
** is slightly more complex.
** Function domain: n >= 0,  0 <= m <= n.
**
** Both versions work by reducing the product as it is computed.  It
** relies on the property that the product on n consecutive integers
** must be evenly divisible by n.
**
** The first version can be changed to make cnm and the return value
** double to extend the range of the function.
*/

inline unsigned long ncomb1 (int n, int m)
{
   unsigned long cnm = 1UL;
   int i;

   if (m*2 >n) m = n-m;
   for (i=1 ; i <= m; n--, i++)
      cnm = cnm * n / i;
   return cnm;
}

inline unsigned long ncomb2 (int n, int m)
{
   unsigned long cnm = 1UL;
   int i, f;

   if (m*2 >n) m = n-m;
   for (i=1 ; i <= m; n--, i++)
   {
      if ((f=n) % i == 0)
         f   /= i;
      else  cnm /= i;
      cnm *= f;
   }
   return cnm;
}

// Returns the combinations of n things taken k at a time in lexicographical order.
// Algorithm by Donald Knuth.
// Original C implementation by Glenn C. Rhoads
// http://remus.rutgers.edu/~rhoads/Code/code.html
// default T=BDynamicArray<int>
template<class T>
inline uint GenerateCombinations(T& results, uint n, uint k)
{
   BDEBUG_ASSERT((n >= k) && (n > 0) && (k > 0));

   //results.resize(Math::NumCombinations(n, k) * k);
   results.resize(ncomb2(n, k) * k);
   int numResults = 0;

   int i, j = 1, x;

   BStaticArray<int, 64> c(k + 3);

   for (i=1; i <= (int)k; i++) 
      c[i] = i;
   c[k + 1] = n + 1;
   c[k + 2] = 0;
   j = k;

visit:
   for (i = k; i >= 1; i--) 
      results[numResults * k + i - 1] = (T::valueType)c[i] - 1;
   
   numResults++;
   if (n == k)
      return 1;

   if (j > 0) {x = j + 1; goto incr; }

   if (c[1] + 1 < c[2])
   {
      c[1] += 1;
      goto visit;
   }

   j = 2;

doMore:
   c[j-1] = j-1;
   x = c[j] + 1;
   if (x == c[j + 1]) {j++; goto doMore; }

   if (j > (int)k) 
      return numResults;

incr:
   c[j] = x;
   j--;
   goto visit;
}     
