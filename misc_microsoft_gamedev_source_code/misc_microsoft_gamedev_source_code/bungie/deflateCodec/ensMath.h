//============================================================================
// math.h
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#pragma once

namespace ens
{
   namespace Math
   {
      template<class T> inline T Max(T i, T j)
      {
         return (i > j) ? i : j;
      }
      
      template<class T> inline T Min(T i, T j)
      {
         return (i < j) ? i : j;
      }

      template<class T> inline T Min3(T a, T b, T c)
      {
         return Min(Min(a, b), c);
      }
      
      inline bool IsPow2(uint num)
      {
         return (num > 0) && ((num & (uint)-(int)num) == num);
      }

      inline int FloatToIntTrunc(float f)
      {
         return static_cast<int>(f);
      }
      
      template<class T> inline T Clamp(T i, T l, T h)
      {
         if (i < l)
            return l;
         else if (i > h)
            return h;

         return i;
      }
   } // namespace Math
} // namespace ens

