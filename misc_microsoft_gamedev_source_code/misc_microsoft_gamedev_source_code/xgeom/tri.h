// tri.h
#pragma once

#include "math/plane.h"
#include "math/vectorInterval.h"

#include "indexedTri.h"

class BVec2Vert : public BVec2
{
public:
   typedef BVec2 PositionType;
   
   BVec2Vert() : BVec2() { }
   BVec2Vert(const BVec2& v) : BVec2(v) { }
   explicit BVec2Vert(float s) : BVec2(s) { }
   BVec2Vert(float x, float y) : BVec2(x, y) { }      
   BVec2Vert(const BVec2Vert& o) { *this = o; }
   BVec2Vert& operator= (const BVec2Vert& o) { BVec2::operator=(o.pos()); return *this; }
      
   const PositionType& pos(void) const  { return static_cast<const PositionType&>(*this); }
         PositionType& pos(void)        { return static_cast<PositionType&>(*this); }
};

class BVec3Vert : public BVec3
{
public:
   typedef BVec3 PositionType;
   
   BVec3Vert() : BVec3() { }
   BVec3Vert(const BVec3& v) : BVec3(v) { }
   explicit BVec3Vert(float s) : BVec3(s) { }
   BVec3Vert(const BVec3Vert& o) { *this = o; }
   BVec3Vert(float x, float y, float z) : BVec3(x, y, z) { }      
   BVec3Vert& operator= (const BVec3Vert& o) { BVec3::operator=(o.pos()); return *this; }
   
   const PositionType& pos(void) const  { return static_cast<const PositionType&>(*this); }
         PositionType& pos(void)        { return static_cast<PositionType&>(*this); }
};

// Vert can be BVec2, BVec3, or Univert, anything else won't make sense in many methods.
// Univert is stretching it!
template <typename Vert>
class BTri
{
public:
   typedef typename Vert VertexType;
   typedef typename Vert::PositionType PositionType;
      
   BTri()
   {
   }
               
   BTri(const VertexType& a, const VertexType& b, const VertexType& c)
   {
      mVerts[0] = a;
      mVerts[1] = b;
      mVerts[2] = c;
   }
   
   void clear(void)
   {
      mVerts[0].clear();
      mVerts[1].clear();
      mVerts[2].clear();
   }
   
   BTri& operator= (const BTri& rhs)
   {
      Utils::Copy(rhs.mVerts, rhs.mVerts + NumTriVerts, mVerts);
      return *this;
   }
   
   const VertexType& operator[] (int i) const   { return mVerts[debugRangeCheck(i, NumTriVerts)]; }
         VertexType& operator[] (int i)         { return mVerts[debugRangeCheck(i, NumTriVerts)]; }
   
   const PositionType& vert(int i) const  { return (*this)[i]; }
         PositionType& vert(int i)        { return (*this)[i]; }            
         
   const PositionType& pos(int i) const  { return (*this)[i].pos(); }
         PositionType& pos(int i)        { return (*this)[i].pos(); }
   
   BTri& flip(void)
   {
      std::swap(vert(0), vert(2));
      return *this;
   }
   
   BTri& shift(void) 
   {
      const VertexType temp(vert(0));
      vert(0) = vert(1);
      vert(1) = vert(2);
      vert(2) = temp;
      return *this;
   }
   
   BTri rotated(int i) const
   {
      return BTri( vert((i+0)%3), vert((i+1)%3), vert((i+2)%3) );
   }
   
   BVec3 normal(bool tryNormalize = true) const
   {
      BCOMPILETIMEASSERT((PositionType::numElements == 2) || (PositionType::numElements == 3));
      
      BVec3 n((pos(2) - pos(0)) % (pos(0) - pos(1)));
      if (tryNormalize)
         n.tryNormalize();
      return n;
   }
   
   float area(void) const
   {
      return normal(false).len() * .5f;
   }
   
   Plane plane(void) const
   {
      BVec3 n(normal());
      
      float minD = Math::fNearlyInfinite;
      // take minimum, due to float precision (may want other methods?)
      for (int i = 0; i < NumTriVerts; i++)
         minD = Math::Min(minD, BVec3(pos(i)) * n);
      return Plane(n, minD);
   }
   
   VecInterval<PositionType> bounds(void) const
   {
      VecInterval<PositionType> bounds(VecInterval<PositionType>::eInitExpand);
      
      for (int i = 0; i < NumTriVerts; i++)
         bounds.expand(pos(i));
      
      return bounds;
   }
   
   // returns triangle with positions transformed by matrix
   template<typename M>
   BTri transformed(const M& matrix) const
   {
      BTri tri(*this);
      tri.pos(0) = pos(0) * matrix;
      tri.pos(1) = pos(1) * matrix;
      tri.pos(2) = pos(2) * matrix;
      return ret;
   }
   
   BTri& scale(const PositionType& s) 
   {
      pos(0) = PositionType::multiply(pos(0), s);
      pos(1) = PositionType::multiply(pos(1), s);
      pos(2) = PositionType::multiply(pos(2), s);
      return *this;
   }
   
   // returns scaled triangle
   BTri scaled(const PositionType& s) const
   {
      BTri tri(*this);
      tri.pos(0) = PositionType::multiply(pos(0), s);
      tri.pos(1) = PositionType::multiply(pos(1), s);
      tri.pos(2) = PositionType::multiply(pos(2), s);
      return tri;
   }
   
   BTri& translate(const PositionType& trans)
   {
      pos(0) = pos(0) + trans;
      pos(1) = pos(1) + trans;
      pos(2) = pos(2) + trans;
      return *this;
   }
         
   // returns translated triangle
   BTri translated(const PositionType& trans) const
   {
      BTri tri(*this);
      tri.pos(0) = pos(0) + trans;
      tri.pos(1) = pos(1) + trans;
      tri.pos(2) = pos(2) + trans;
      return tri;
   }
   
   BTri<BVec2Vert> compSelect2(int c0, int c1) const
   {
      return BTri<BVec2Vert>(pos(0).compSelect2(c0, c1), pos(1).compSelect2(c0, c1), pos(2).compSelect2(c0, c1));
   }
   
   BTri<BVec3Vert> compSelect3(int c0, int c1, int c2) const
   {
      return BTri<BVec3Vert>(pos(0).compSelect3(c0, c1, c2), pos(1).compSelect3(c0, c1, c2), pos(2).compSelect3(c0, c1, c2));
   }
   
   // due to FP precision it's possible the returned point may lie slightly outside the tri
   PositionType barycenter(void) const
   {
      PositionType center;
      
      // / 3.0f instead of * (1.0f/3.0f) should be slightly more precise?
      for (int i = 0; i < PositionType::numElements; i++)
      {
         // maximizes precision in debug builds and avoids automatic VC71 SSE optimization
         const double a = pos(0)[i];
         const double b = pos(1)[i];
         const double c = pos(2)[i];
         center[i] = (float)((a + b + c) / 3.0f);
      }
               
      // workaround for FP precision issues
      return bounds().clamp(center);
   }
   
   PositionType centroid(void) const { return barycenter(); }
   
   PositionType toBarycentric(const PositionType& p) const
   {
      int uAxis, vAxis;
      if (PositionType::numElements == 2)
      {
         uAxis = 0;
         vAxis = 1;
      }
      else
         normal().projectionAxes(uAxis, vAxis);
      
      const float px0 = pos(0)[uAxis];
      const float py0 = pos(0)[vAxis];
      const float px1 = pos(1)[uAxis];
      const float py1 = pos(1)[vAxis];
      const float px2 = pos(2)[uAxis];
      const float py2 = pos(2)[vAxis];
      const float lx = p[uAxis];
      const float ly = p[vAxis];

      // From a message board on flipcode.com. There are faster/simpler ways! (Graphics Gems 1)
      const float ooA = 1.0f / (px1 * py2 - px1 * py0 - px0 * py2 - py1 * px2 + py1 * px0 + py0 * px2);
      const float s =  (px0 * ly - px0 * py2 - ly * px2 + lx * py2 + py0 * px2 - py0 * lx) * ooA;
      const float t = -(px1 * py0 + px0 * ly - py1 * px0 - px1 * ly - py0 * lx + py1 * lx) * ooA;
      
      return BVec2(s, t);
   }
   
   PositionType fromBarycentric(const BVec2& barycentric) const
   {
      return pos(0) + (pos(1) - pos(0)) * barycentric[0] + (pos(2) - pos(0)) * barycentric[1];
   }
   
   // u and v must be within [0,1]
   PositionType randomPoint(float u, float v) const
   {
      debugRangeCheckIncl(u, 1.0f);
      debugRangeCheckIncl(v, 1.0f);
      const float sqrtV = sqrt(v);
      
      const float s = 1.0f - sqrtV;
      const float t = u * sqrtV;
      BASSERT((s + t) >= 0.0f);
      BASSERT((s + t) <= 1.000125f);
      return fromBarycentric(BVec2(s, t));
   }
   
   float edgeLen(int i) const
   {
      const PositionType& a = pos(i);
      const PositionType& b = pos(Math::NextWrap(i, 3));
      return a.dist(b);
   }

protected:
   VertexType mVerts[NumTriVerts];
};

typedef BTri<BVec2Vert> BTri2;
typedef BTri<BVec3Vert> BTri3;
typedef BDynamicArray<BTri2> BTri2Vec;
typedef BDynamicArray<BTri3> BTri3Vec;

template <typename Vert>
class BIndexTri : public BTri<Vert>
{
public:
   typedef typename Vert VertexType;
   typedef typename Vert::PositionType PositionType;
      
   BIndexTri() : BTri<Vert>()
   {
   }
   
   explicit BIndexTri(int index) : mIndex(index), BTri<Vert>()
   {
   }
   
   BIndexTri(int index, const VertexType& a, const VertexType& b, const VertexType& c) : BTri<Vert>(a, b, c) : mIndex(index)
   {
   }
   
   int  index(void) const  { return mIndex; }
   int& index(void)        { return mIndex; }
   
   void clear(void)
   {
      BTri<Vert>::clear();
      mIndex = 0;
   }
      
protected:
   int mIndex;
};

typedef BIndexTri<BVec2Vert> BIndexTri2;
typedef BIndexTri<BVec3Vert> BIndexTri3;
typedef BDynamicArray<BIndexTri2> BIndexTri2Vec;
typedef BDynamicArray<BIndexTri3> BIndexTri3Vec;

