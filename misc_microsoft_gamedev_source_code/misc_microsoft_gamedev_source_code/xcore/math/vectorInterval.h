//============================================================================
//
// File: VectorInterval.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "generalVector.h"
#include "generalMatrix.h"

// warning C4063: case '2' is not a valid value for switch of enum 'BVecN<size>::<unnamed-tag>'
#pragma warning(disable:4063)

template<class T>
struct VecInterval
{
   T extent[2];

   VecInterval()
   {
   }

   VecInterval(const T& v)
   {
      extent[0] = v;
      extent[1] = v;
   }

   VecInterval(const T& low, const T& high)
   {
      extent[0] = low;
      extent[1] = high;
   }

   VecInterval(float low, float high)
   {
      extent[0] = T(low);
      extent[1] = T(high);
   }

   VecInterval(float v)
   {
      extent[0] = T(v);
      extent[1] = T(v);
   }

   enum EInitExpand { eInitExpand };
   VecInterval(EInitExpand e)
   {
      e;
      initExpand();
   }

   VecInterval(eClear e)
   {
      e;
      clear();
   }

   void clear(void)
   {
      extent[0] = extent[1] = 0;
   }
   
   void set(const T& low, const T& high)
   {
      extent[0] = low;
      extent[1] = high;
   }

   const T& operator[] (int i) const      { return extent[debugRangeCheck(i, 2)]; }
         T& operator[] (int i)            { return extent[debugRangeCheck(i, 2)]; }

   const T& low(void) const         { return extent[0]; }
         T& low(void)               { return extent[0]; }

   const T& high(void) const        { return extent[1]; }
         T& high(void)              { return extent[1]; }

   bool contains(const T& p) const
   {
      return (extent[0].allLessEqual(p)) && (p.allLessEqual(extent[1]));
   }

   bool contains(const VecInterval& v) const
   {
      for (int i = 0; i < T::numElements; i++)
         if ((extent[0][i] > v.extent[0][i]) ||
            (extent[1][i] < v.extent[1][i]))
            return false;
      return true;
   }

   VecInterval& normalize(void) const
   {
      for (int i = 0; i < T::numElements; i++)
         if (extent[0][i] > extent[1][i])
            std::swap(extent[0][i], extent[1][i]);
      return *this;
   }

   VecInterval& initExpand(void) 
   {
      extent[0] = T(+Math::fNearlyInfinite);
      extent[1] = T(-Math::fNearlyInfinite);
      return *this;
   }

   VecInterval& expand(const T& p) 
   {
      for (int i = 0; i < T::numElements; i++)
      {
         if (p[i] < extent[0][i])
            extent[0][i] = p[i];
         if (p[i] > extent[1][i])
            extent[1][i] = p[i];
      }
      return *this;
   }

   VecInterval& expand(const VecInterval& p) 
   {
      for (int i = 0; i < T::numElements; i++)
      {
         if (p[0][i] < extent[0][i])
            extent[0][i] = p[0][i];
         if (p[1][i] > extent[1][i])
            extent[1][i] = p[1][i];
      }
      return *this;
   }

   VecInterval& translate(const T& t)
   {
      extent[0] += t;
      extent[1] += t;
      return *this;
   }

   const VecInterval& bounds(void) const
   {
      return *this;
   }

   T diagonal(void) const
   {
      return extent[1] - extent[0];
   }

   T center(void) const
   {
      return T::lerp(extent[0], extent[1], .5f);
   }

   T centroid(void) const
   {
      return center();
   }

   bool overlapsAxis(const VecInterval& b, int e) const
   {
      return (extent[0][e] <= b.extent[1][e]) && (extent[1][e] >= b.extent[0][e]);
   }

   bool overlaps(const VecInterval& b) const
   {
      for (int i = 0; i < T::numElements; i++)
         if (!overlapsAxis(b, i))
            return false;
      return true;
   }

   float dimension(int e = 0) const
   {
      return extent[1][e] - extent[0][e];
   }

   int minorDimension(void) const
   {
      int d = 0;
      float v = dimension(0);
      for (int i = 1; i < T::numElements; i++)
      {
         if (dimension(i) < v)
         {
            v = dimension(i);
            d = i;
         }
      }
      return d;
   }

   int majorDimension(void) const
   {
      int d = 0;
      float v = dimension(0);
      for (int i = 1; i < T::numElements; i++)
      {
         if (dimension(i) > v)
         {
            v = dimension(i);
            d = i;
         }
      }
      return d;
   }

   float volume(void) const
   {
      float ret = dimension(0);
      for (int i = 1; i < T::numElements; i++)
         ret *= dimension(i);
      return ret;
   }

   float surfaceArea(void) const
   {
      float area = 0.0f;

      BCOMPILETIMEASSERT((T::numElements == 3) || (T::numElements == 4));

      switch (T::numElements)
      {
      case 2:
         area = dimension(0) * 2.0f + dimension(1) * 2.0f;
         break;
      case 3:
         area = dimension(0) * dimension(1) * 2.0f +
            dimension(1) * dimension(2) * 2.0f +
            dimension(2) * dimension(0) * 2.0f;               
         break;
      }

      return area;
   }

   float minPossibleDist2(const T& v) const
   {
      float ret = 0.0f;
      for (int i = 0; i < T::numElements; i++)
      {
         if (v[i] < extent[0][i])
            ret += sqr(extent[0][i] - v[i]);
         else if (v[i] > extent[1][i])
            ret += sqr(v[i] - extent[1][i]);
      }
      return ret;
   }

   float maxPossibleDist2(const T& v) const
   {
      float ret = 0.0f;
      for (int i = 0; i < T::numElements; i++)
      {
         if (v[i] < lo[i])
            ret += sqr(extent[1][i] - v[i]);
         else if (v[i] > extent[1][i])
            ret += sqr(v[i] - extent[0][i]);
         else
            ret += Math::Max(sqr(extent[1][i] - v[i]), sqr(v[i] - extent[0][i]));
      }
      return ret;
   }

   T toNormPos(const T& v) const
   {
      T ret;
      for (int i = 0; i < T::numElements; i++)
         ret[i] = (v[i] - low()[i]) / dimension(i);
      return ret;
   }

   void log(BTextDispatcher& l) const
   {
      extent[0].log(l);
      extent[1].log(l);
   }

   int numCorners(void) const
   {
      return 1 << T::numElements;
   }

   // there are 2^T::numElements possible corner points
   T corner(int p) const
   {
      debugRangeCheck(p, numCorners());

      T ret;
      for (int i = 0; i < T::numElements; i++)
         ret[i] = extent[(p >> i) & 1][i];

      return ret;
   }

   // Returns inward facing planes (pos halfspace=inside).
   // Only works with 3D intervals (AABB's)
   template<class PlaneType>
      PlaneType& plane(PlaneType& dst, int i) const
   {  
      BCOMPILETIMEASSERT(T::numElements == 3);
      debugRangeCheck(i, 0, 6);

      int s = i & 1;
      float fs = 1 - (s * 2);
      int d = i >> 1;

      BVec3 n(0.0f);
      n[d] = fs;

      dst = PlaneType(n, fs * (*this)[s][d]);
      return dst;
   }

   template<class MatrixType>
      VecInterval transform(const MatrixType& m) const
   {
      VecInterval result;
      result.initExpand();
      for (int i = 0; i < numCorners(); i++)
         result.expand(MatrixType::transformPoint(corner(i), m));
      return result;
   }

   // reference: Graphic Gems 2, "Transforming Axis-Aligned Bounding Boxes", page 548
   template<class MatrixType>
      VecInterval transform3(const MatrixType& m) const
   {
      BCOMPILETIMEASSERT(T::numElements == 3);

      VecInterval result(m.getTranslate(), m.getTranslate());

      for (int i = 0; i < 3; i++)
      {
         for (int j = 0; j < 3; j++)
         {
            float a = m[j][i] * (*this)[0][j];
            float b = m[j][i] * (*this)[1][j];

            if (a < b)
            {
               result.extent[0][i] += a;
               result.extent[1][i] += b;
            }
            else
            {
               result.extent[0][i] += b;
               result.extent[1][i] += a;
            }
         }
      }

      return result;
   }

   T clamp(const T& v) const
   {
      T ret;
      for (int i = 0; i < T::numElements; i++)
      {
         ret[i] = v[i];
         if (ret[i] < extent[0][i])
            ret[i] = extent[0][i];
         else if (ret[i] > extent[1][i])
            ret[i] = extent[1][i];
      }
      return ret;
   }

   friend bool operator== (const VecInterval& a, const VecInterval& b)
   {
      return (a[0] == b[0]) && (a[1] == b[1]);
   }

   static VecInterval unionOp(const VecInterval& a, const VecInterval& b)
   {
      VecInterval res(a);
      res.expand(b);
      return res;
   }

   // true if intersection results in non-empty AABB
   static std::pair<VecInterval, bool> intersectionOp(const VecInterval& a, const VecInterval& b)
   {
      VecInterval result;
      result.initExpand();

      for (int i = 0; i < T::numElements; i++)
      {
         const float as = a[0][i];
         const float ae = a[1][i];
         const float bs = b[0][i];
         const float be = b[1][i];

         if ((bs > ae) || (be < as))
            return std::make_pair(VecInterval(eInitExpand), false);

         result[0][i] = Math::Max(as, bs);
         result[1][i] = Math::Min(ae, be);

         BASSERT(result[0][i] <= result[1][i]);
      }

      result.debugCheck();

      return std::make_pair(result, true);
   }

   bool valid(void) const
   {
      for (int i = 0; i < T::numElements; i++)
         if (extent[0][i] > extent[1][i])
            return false;
      return true;
   }

   friend BStream& operator<< (BStream& dst, const VecInterval<T>& src)
   {
      return dst << src.extent[0] << src.extent[1];
   }

   friend BStream& operator>> (BStream& src, VecInterval<T>& dst)
   {
      return src >> dst.extent[0] >> dst.extent[1];
   }

#ifdef DEBUG
   void debugCheck(void) const
   {
      extent[0].debugCheck();
      extent[1].debugCheck();

      for (int i = 0; i < T::numElements; i++)
      {
         BASSERT(extent[1][i] >= extent[0][i]);
      }
   }
#else
   void debugCheck(void) const
   {
   }
#endif
};

typedef VecInterval< BVecN<1> >  BVec1Interval;
typedef BVec1Interval            BInterval;

typedef VecInterval< BVecN<2> >  BVec2Interval;
typedef BVec2Interval            Rect;

typedef VecInterval< BVecN<3> >  BVec3Interval;
typedef BVec3Interval            AABB;
typedef BDynamicArray<AABB>      AABBVec;

typedef VecInterval< BVecN<4> >  BVec4Interval;

#pragma warning(default:4063)