//-----------------------------------------------------------------------------
// File: quat.h
// Copyright (c) 2005-2006, Ensemble Studios
// rg [1/6/05] - Ported from xgrlib, SSE code removed, needs to be retested!
//-----------------------------------------------------------------------------
#pragma once

#include "math\generalVector.h"
#include "math\generalMatrix.h"

const float fQuatEpsilon = .00125f;

//-----------------------------------------------------------------------------
// struct BQuat
//-----------------------------------------------------------------------------
struct BQuat
{
   BVec4 mVec;

   BQuat();

   // directly sets vector/scalar components
   BQuat(const BVec4& v);

   // directly sets vector and scalar components
   BQuat(const BVec4& v, float w);

   BQuat(float x, float y, float z, float w);

   BQuat(const BMatrix44& m);
   BQuat& operator= (const BMatrix44& m);

   BQuat& operator= (const BVec4& v);
      
   BQuat& operator= (const BQuat& b);
   
   const float operator[] (int element) const;
         float& operator[] (int element);
   
   BQuat& setIdentity(void);

   // returns 0 quat on error
   BQuat& setFromMatrix(const BMatrix44& m);

   BQuat& setFromAxisAngle(const BVec4& axis, float angle);
   
   // XYZ rotation ordering
   BQuat& setFromEuler(const BVec4& euler);
                                       
   BVec4 imaginary(void) const;
   
   float scalar(void) const;
   
   // broadcasts w
   BVec4 scalarBroadcast(void) const;
   
   // rotation axis
   BVec4 axis(void) const;
   
   // rotation angle
   float angle(void) const;
   
   // squared length
   float norm(void) const;
   
   // length
   float len(void) const;
   
   // in-place normalization
   BQuat normalize(void);
   
   // returns 0 or 1/sqrt(norm())
   float tryNormalize(void);
   
   // returns normalized vector
   BQuat normalized(void) const;
   
   // dot product 4
   float dot(const BQuat& b) const;
   
   BQuat inverse(void) const;
   BQuat unitInverse(void) const;
   BQuat conjugate(void) const;

   // concatenates a and b
   friend BQuat operator* (const BQuat& a, const BQuat& b);

   // component ops
   friend BQuat operator* (const BQuat& a, float s);
   friend BQuat operator* (float s, const BQuat& b);
   friend BQuat operator/ (const BQuat& a, float s);
   friend BQuat operator+ (const BQuat& a, const BQuat& b);
   friend BQuat operator- (const BQuat& a, const BQuat& b);
               
   BQuat operator- () const;
   BQuat operator+ () const;
   
   BQuat& operator += (const BQuat& b);
   BQuat& operator -= (const BQuat& b);

   bool operator== (const BQuat& b) const;
   bool operator!= (const BQuat& b) const;
   
   // converts to homogeneous 3x3 matrix (element 3,3=1)
   BMatrix44& toMatrix(BMatrix44& m) const;
               
   bool isPure(void) const;
   
   bool isUnit(void) const;
   
   BVec4 rotateVec(const BVec4& v) const;
   BVec4 rotateVecTransposed(const BVec4& v) const;

   static BQuat makeIdentity(void);

   // returns 0 quat on error
   static BQuat makeFromMatrix(const BMatrix44& m);

   // returns rotation that rotates v0 into v1
   static BQuat makeRotationArc(const BVec4& v0, const BVec4& v1);

   static BQuat makeRotation(const BQuat& from, const BQuat& to);
   
   static BQuat makeRandom(float x1 = Math::fRand(0.0f, 1.0f), float x2 = Math::fRand(0.0f, 1.0f), float x3 = Math::fRand(0.0f, 1.0f));

   // XYZ rotation ordering
   static BQuat makeFromEuler(const BVec4& euler);
      
   static BQuat unitLog(const BQuat& q);
   static BQuat unitExp(const BQuat& q);
   // interpolates from identity 
   static BQuat unitPow(const BQuat& q, float p);
   // same as pow(.5)
   static BQuat squareRoot(const BQuat& q);

   // fastest on P4
   static BQuat multiplySIMD(const BQuat& a, const BQuat& b);
   
   static BQuat multiplyScalar(const BQuat& a, const BQuat& b);
   
   // concatenates a and b
   static BQuat multiply(const BQuat& a, const BQuat& b);

   static BQuat lerp(const BQuat& a, const BQuat& b, float t);
   static BQuat lerpNoNormalize(const BQuat& a, const BQuat& b, float t);
   
   static BQuat slerp(const BQuat& a, const BQuat& b, float t);
   
   static BQuat slerpExtraSpins(const BQuat& a, const BQuat& b, float t, int extraSpins);

   // used by squad()
   static BQuat slerpNoInvert(const BQuat& a, const BQuat& b, float t);
         
   // 3x-4x faster than slerp(), if you can tolerate the loss of accuracy
   static BQuat slerpFast(const BQuat& a, const BQuat& b, float t);

   static BQuat slerpLogForm(const BQuat& a, const BQuat& b, float t);

   static BQuat squad(const BQuat& q1, const BQuat& q2, const BQuat& a, const BQuat& b, float t);
   static void squadSetup(BQuat& pa, BQuat& pb, const BQuat& p0, const BQuat& p1, const BQuat& p2);
         
   static bool equalTol(const BQuat& a, const BQuat& b, float tol = Math::fSmallEpsilon);

   static BQuat catmullRom(const BQuat& q00, const BQuat& q01, const BQuat& q02, const BQuat& q03, float t);
   static BQuat bezier(const BQuat& q00, const BQuat& q01, const BQuat& q02, const BQuat& q03, float t);
   static BQuat bSpline(const BQuat& q00, const BQuat& q01, const BQuat& q02, const BQuat& q03, float t);

   static BMatrix44 createMatrix(const BQuat& a);
                           
   static const BQuat I;
   static const BQuat Z;
};

#include "quat.inl"
