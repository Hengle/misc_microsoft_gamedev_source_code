//============================================================================
//
// File: plane.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "generalVector.h"
#include "generalMatrix.h"

template<typename scalarType> struct ParametricPlaneT;

template<typename scalarType = float>
struct PlaneT
{
   typedef scalarType ScalarType;
   typedef BVecN<3, scalarType> VecType;
   
   VecType n;
   // POSITIVE distance along normal to plane's origin (i.e. this is NOT a plane equation).
   scalarType d;       

   PlaneT()
   {
   }

   PlaneT(scalarType x, scalarType y, scalarType z, scalarType dist) : n(x, y, z), d(dist)
   {
   }

   PlaneT(const VecType& a, const VecType& b, const VecType& c, scalarType eps = Math::fTinyEpsilon)
   {
      setFromTriangle(a, b, c, eps);
   }

   PlaneT(const VecType& norm, scalarType dist) : n(norm), d(dist)
   {
   }

   PlaneT(const VecType& norm, const VecType& origin) : n(norm), d(norm * origin)
   {
   }

   PlaneT(const ParametricPlaneT<scalarType>& pp);

   PlaneT(const BVecN<4, scalarType>& equation) : n(equation), d(-equation[3])
   {
   }

   PlaneT(const BVecN<4, scalarType>& norm, scalarType dist) : n(norm), d(dist)
   {
   }
         
   bool isValid(void) const
   {
      return 0.0f != n.norm();
   }

   PlaneT& setInvalid(void) 
   {
      n.setZero();
      d = 0;
      return *this;
   }
   
   void clear(void)
   {
      setInvalid();
   }

   // false on failure
   bool setFromTriangle(const VecType& a, const VecType& b, const VecType& c, scalarType eps = Math::fTinyEpsilon)
   {
      n = (c - b) % (a - b);
      
      if (n.len() <= eps)
      {
         setInvalid();
         return false;
      }

      if (0.0f == n.tryNormalize())
      {
         setInvalid();
         return false;
      }
      
      d = n * b;
      return true;
   }

   PlaneT& setFromNormalOrigin(const VecType& norm, const VecType& origin)
   {
      n = norm;
      d = norm * origin;
      return *this;
   }

   PlaneT& setFromParametricPlane(const ParametricPlaneT<scalarType>& pp);

   bool normalize(void)
   {
      scalarType len = n.tryNormalize();
      if (0.0f == len)
      {
         setInvalid();
         return false;
      }
      d /= len;
      return true;
   }

   VecType origin(void) const
   {
      return n * d;
   }

   const VecType& normal(void) const   { return n; }
         VecType& normal(void)         { return n; }

   const scalarType& distance(void) const { return d; }
         scalarType& distance(void)       { return d; }

   bool operator== (const PlaneT& b) const
   {
      return (n == b.n) && (d == b.d);
   }

   bool operator!= (const PlaneT& b) const
   {
      return !(*this == b);
   }

   scalarType distanceToPoint(const VecType& p) const
   {
      return p * n - d;
   }

   PlaneT flipped(void) const
   {
      return PlaneT(-n, -d);
   }
   
   PlaneT& flip(void) 
   {
      n = -n;
      d = -d;
      return *this;
   }

   BVecN<4, scalarType> equation(void) const
   {
      return BVecN<4, scalarType>(n[0], n[1], n[2], -d);
   }
   
   const PlaneT& set(scalarType x, scalarType y, scalarType z, scalarType dist)
   {
      n[0] = x;
      n[1] = y;
      n[2] = z;
      d = dist;
      return *this;
   }
   
   const PlaneT& setFromEquation(const BVecN<4, scalarType>& v) 
   {
      n = v;
      d = -v[3];
      return *this;
   }
   
   const PlaneT& setFromEquation(scalarType x, scalarType y, scalarType z, scalarType w)
   {
      n.set(x, y, z);
      d = -w;
      return *this;
   }

   // x0 axis change with respect to axis x1
   bool gradient(scalarType& result, int x0, int x1, scalarType eps = Math::fTinyEpsilon) const
   {
      const scalarType dA = -n[x1];
      const scalarType dB = n[x0];
      if (fabs(dB) <= eps)
         return false;
      result = dA / dB;
      return true;
   }
   
   // Slow! This could be optimized if assumptions could be made about the matrix.
   friend PlaneT operator* (const PlaneT& p, const BMatrixNxN<4,4,scalarType>& m)
   {
      PlaneT ret;
      //BMatrixNxN<4,4,scalarType> y(m.inverse());
      //y.transpose();
      //ret.n = BMatrixNxN<4,4,scalarType>::transformVector(BVecN<4, scalarType>(p.n), y).normalize();
      ret.n = BMatrixNxN<4,4,scalarType>::transformVectorTransposed(m.inverse(), BVecN<4, scalarType>(p.n)).normalize3();
      ret.d = BMatrixNxN<4,4,scalarType>::transformPoint(BVecN<4, scalarType>(p.origin()), m).dot3(BVecN<4, scalarType>(ret.n));
      return ret;
   }

   static PlaneT transformOrthonormal(const PlaneT& p, const BMatrixNxN<4,4,scalarType>& m)
   {
      PlaneT ret;
      ret.n = BMatrixNxN<4,4,scalarType>::transformVector(BVecN<4, scalarType>(p.n), m);
      ret.d = p.d + VecType(m.getTranslate()) * ret.n;
      return ret;
   }

   VecType project(const VecType& p) const
   {
      return p - n * distanceToPoint(p);
   }

   // returns value of desiredAxis given the other two
   bool project(scalarType& result, const VecType& p, int desiredAxis, scalarType eps = Math::fTinyEpsilon) const
   {
      const int axis2[] = { 1, 0, 0 };
      const int axis3[] = { 2, 2, 1 };

      const int a2 = axis2[debugRangeCheck(desiredAxis, 3)];
      const int a3 = axis3[desiredAxis];

      if (fabs(n[desiredAxis]) <= eps)
         return false;
         
      result = (d - p[a2] * n[a2] - p[a3] * n[a3]) / n[desiredAxis];
      return true;
   }
   
   void clipPoly(BDynamicArray<VecType>& result, const BDynamicArray<VecType>& verts) const
   {
      result.erase(result.begin(), result.end());

      if (verts.empty())
         return;

      scalarType prevDist = distanceToPoint(verts[0]);
      const int numVerts = static_cast<int>(verts.size());
         
      for (int prev = 0; prev < numVerts; prev++)
      {
         int cur = Math::NextWrap(prev, numVerts);
         scalarType curDist = distanceToPoint(verts[cur]);

         if (prevDist >= 0.0f)
            result.pushBack(verts[prev]);

         if (((prevDist < 0.0f) && (curDist > 0.0f)) ||
               ((prevDist > 0.0f) && (curDist < 0.0f)))
         {
            result.pushBack(VecType::lerp(verts[prev], verts[cur], prevDist / (prevDist - curDist)));
         }
         
         prevDist = curDist;
      }
   }
};
   
template<typename scalarType = float>
struct ParametricPlaneT
{
   typedef scalarType ScalarType;
   typedef BVecN<3, scalarType> VecType;
   
   VecType o;
   VecType u;
   VecType v;

   ParametricPlaneT()
   {
   }

   template<typename otherScalerType>
   ParametricPlaneT(const PlaneT<otherScalerType>& p)
   {
      setFromPlane(p);
   }

   ParametricPlaneT(const VecType& orig, const VecType& uAxis, const VecType& vAxis) :
      o(orig), u(uAxis), v(vAxis)
   {
   }

   ParametricPlaneT(const VecType& orig, const VecType& norm)
   {
      setFromOriginNormal(orig, norm);
   }

   ParametricPlaneT& setFromOriginAxes(const VecType& orig, const VecType& uAxis, const VecType& vAxis)
   {
      o = orig;
      u = uAxis;
      v = vAxis;
      return *this;
   }

   BVecN<2, scalarType> project(const VecType& p) const
   {
      return BVecN<2, scalarType>((p - o) * u, (p - o) * v);
   }

   VecType point(const BVecN<2, scalarType>& p) const
   {
      return o + u * p[0] + v * p[1];
   }

   VecType normal(void) const
   {
      return u % v;
   }

   const VecType& origin(void) const   { return o; }
         VecType& origin(void)         { return o; }

   const VecType& uAxis(void) const { return u; }
         VecType& uAxis(void)       { return u; }

   const VecType& vAxis(void) const { return v; }
         VecType& vAxis(void)       { return v; }

   ParametricPlaneT& normalize(void)
   {
      u.normalize();
      v.normalize();
      return *this;
   }

   template<typename otherScalerType>
   ParametricPlaneT& setFromPlane(const PlaneT<otherScalerType>& p)
   {
      o = p.origin();
      u = VecType::removeCompUnit(VecType::makeAxisVector(p.n.minorAxis()), p.n).normalize();
      v = p.n % u;
      return *this;
   }

   ParametricPlaneT& setFromOriginNormal(const VecType& orig, const VecType& norm)
   {
      o = orig;
      u = VecType::removeCompUnit(VecType::makeAxisVector(norm.minorAxis()), norm).normalize();
      v = norm % u;
      return *this;
   }

   template<typename otherScalerType>
   static ParametricPlaneT makePlanarProjection(const PlaneT<otherScalerType>& p)
   {
      int x0 = 0, x1 = 0;

      switch (p.n.majorAxis())
      {
         case 0: x0 = 1; x1 = 2; if (p.n[0] > 0.0f) { x0 = 2; x1 = 1; } break;
         case 1: x0 = 2; x1 = 0; if (p.n[1] > 0.0f) { x0 = 0; x1 = 2; } break;
         case 2: x0 = 0; x1 = 1; if (p.n[2] > 0.0f) { x0 = 1; x1 = 0; } break;
      }

      return ParametricPlaneT(VecType(0.0f), VecType::makeAxisVector(x0), VecType::makeAxisVector(x1));
   }
};

template<typename scalarType>
inline PlaneT<scalarType>::PlaneT(const ParametricPlaneT<scalarType>& pp) :
   n(pp.normal()),
   d(pp.origin() * pp.normal())
{
}

template<typename scalarType>
inline PlaneT<scalarType>& PlaneT<scalarType>::setFromParametricPlane(const ParametricPlaneT<scalarType>& pp)
{
   n = pp.normal();
   d = pp.origin() * pp.normal();
   return *this;
}

typedef PlaneT<float> Plane;
typedef PlaneT<double> PlaneD;   

typedef ParametricPlaneT<float> ParametricPlane;
typedef ParametricPlaneT<double> ParametricPlaneD;
