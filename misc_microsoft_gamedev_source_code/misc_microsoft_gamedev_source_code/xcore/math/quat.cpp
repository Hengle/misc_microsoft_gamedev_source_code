//-----------------------------------------------------------------------------
// File: quat.cpp
// Copyright (c) 2005-2006, Ensemble Studios
// rg [1/6/05] - Ported from xgrlib, SSE code removed, needs to be retested!
//-----------------------------------------------------------------------------
#include "xcore.h"

#include "quat.h"

const BQuat BQuat::I(0.0f, 0.0f, 0.0f, 1.0f);
const BQuat BQuat::Z(0.0f, 0.0f, 0.0f, 0.0f);

//-----------------------------------------------------------------------------
// BQuat::operator=
//-----------------------------------------------------------------------------
BQuat& BQuat::operator= (const BMatrix44& m)
{
   *this = makeFromMatrix(m);
   return *this;
}

//-----------------------------------------------------------------------------
// BQuat::makeFromMatrix
//-----------------------------------------------------------------------------
BQuat BQuat::makeFromMatrix(const BMatrix44& m)
{
   BQuat ret;

   if (!m.hasNoReflection3x3())
      return BQuat(0.0f, 0.0f, 0.0f, 0.0f);

   // See "Quaternion Calculus and Fast Animation", Ken Shoemake 1987 SIGGRAPH course notes.

   double trace = m[0][0] + m[1][1] + m[2][2];

   if (trace > 0.0f) 
   {
      double s = sqrt(trace + 1.0f);
      
      ret.mVec[3] = static_cast<float>(s * .5f);
      s = 0.5f / s;
      ret.mVec[0] = static_cast<float>((m[2][1] - m[1][2]) * s);
      ret.mVec[1] = static_cast<float>((m[0][2] - m[2][0]) * s);
      ret.mVec[2] = static_cast<float>((m[1][0] - m[0][1]) * s);
   }
   else 
   {
      int i = 0;
      if (m[1][1] > m[0][0])
         i = 1;
      if (m[2][2] > m[i][i])
         i = 2;
      
      const int j = Math::NextWrap(i, 3);  
      const int k = Math::NextWrap(j, 3);

      double s = sqrt(1.0f + m[i][i] - m[j][j] - m[k][k]);

      ret.mVec[i] = static_cast<float>(s * 0.5f);
      BDEBUG_ASSERT(s > 0.0f);
      s = 0.5f / s;
      ret.mVec[3] = static_cast<float>((m[k][j] - m[j][k]) * s);
      ret.mVec[j] = static_cast<float>((m[j][i] + m[i][j]) * s);
      ret.mVec[k] = static_cast<float>((m[k][i] + m[i][k]) * s);
   }
#ifdef DEBUG
   BDEBUG_ASSERT(ret.isUnit());
   BMatrix44 temp;
   ret.toMatrix(temp);
   BDEBUG_ASSERT(BMatrix44::equalTol3x3(m, temp, Math::fSmallEpsilon));
#endif

   return ret;
}

//-----------------------------------------------------------------------------
// BQuat::slerp
//-----------------------------------------------------------------------------
BQuat BQuat::slerp(const BQuat& a, const BQuat& b, float t)
{
   float d = a.mVec * b.mVec;
   
   BVec4 bvec(b.mVec);

   if (d < 0.0f)
   {
      d = -d;
      bvec = -bvec;
   }

   if (d > (1.0f - fQuatEpsilon))
      return lerp(a, bvec, t);      

   float omega = acos(d);
   float l = sin(omega * (1.0f - t));
   float r = sin(omega * t);

   return (a.mVec * l + bvec * r).normalize();
}

//-----------------------------------------------------------------------------
// BQuat::slerpExtraSpins
//-----------------------------------------------------------------------------
BQuat BQuat::slerpExtraSpins(const BQuat& a, const BQuat& b, float t, int extraSpins)
{
   float d = a.mVec * b.mVec;
   
   BVec4 bvec(b.mVec);

   if (d < 0.0f)
   {
      d = -d;
      bvec = -bvec;
   }

   if (d > (1.0f - fQuatEpsilon))
      return lerp(a, bvec, t);

   float omega = acos(d);
   float phase = Math::fPi * extraSpins * t;
   float l = sin(omega * (1.0f - t) - phase);
   float r = sin(omega * t + phase);

   return (a.mVec * l + bvec * r).normalize();
}

//-----------------------------------------------------------------------------
// BQuat::slerpNoInvert
//-----------------------------------------------------------------------------
BQuat BQuat::slerpNoInvert(const BQuat& a, const BQuat& b, float t)
{
   float d = a.mVec * b.mVec;
   
   if (d > (1.0f - fQuatEpsilon))
      return lerp(a, b, t);

   // should probably check for d close to -1 here

   BVec4 bvec(b.mVec);

   float omega = acos(d);
   float l = sin(omega * (1.0f - t));
   float r = sin(omega * t);

   return (a.mVec * l + bvec * r).normalize();
}

//-----------------------------------------------------------------------------
// BQuat::squad
//-----------------------------------------------------------------------------
BQuat BQuat::squad(const BQuat& p0, const BQuat& pa, const BQuat& pb, const BQuat& p1, float t)
{
   return slerpNoInvert(
      slerpNoInvert(p0, p1, t), 
      slerpNoInvert(pa, pb, t), 
      2.0f * t * (1.0f - t));
}

//-----------------------------------------------------------------------------
// BQuat::squadSetup
//-----------------------------------------------------------------------------
void BQuat::squadSetup(BQuat& pa, BQuat& pb, const BQuat& p0, const BQuat& p1, const BQuat& p2)
{
   BQuat temp(0.25f * (unitLog(p0.unitInverse() * p1) - unitLog(p1.unitInverse() * p2)));
   pa = p1 * unitExp(temp);
   pb = p1 * unitExp(-temp);
}

//-----------------------------------------------------------------------------
// BQuat::slerpFast
// 3x-4x faster than slerp(), if you can tolerate less accuracy
//-----------------------------------------------------------------------------
BQuat BQuat::slerpFast(const BQuat& a, const BQuat& b, float t)
{
   float d = a.mVec * b.mVec;
   if (d > (1.0f - fQuatEpsilon))
      return lerp(a, b, t);

   BVec4 bvec(b.mVec);

   if (d < 0.0f)
   {
      d = -d;
      bvec = -bvec;
   }

   float omega = Math::fFastACos(d);
   float l = Math::fFastSin(omega * (1.0f - t));
   float r = Math::fFastSin(omega * t);

   return (a.mVec * l + bvec * r).normalize();
}

//-----------------------------------------------------------------------------
// BQuat::makeRotationArc
//-----------------------------------------------------------------------------
BQuat BQuat::makeRotationArc(const BVec4& v0, const BVec4& v1)
{
   BDEBUG_ASSERT(v0.isVector());
   BDEBUG_ASSERT(v1.isVector());
   BQuat q;
   const BVec4 c(v0 % v1);
   const float d = v0 * v1;
   const float s = (float)sqrt((1 + d) * 2);
   // could multiply by 1/s instead
   q[0] = c[0] / s;
   q[1] = c[1] / s;
   q[2] = c[2] / s;
   q[3] = s * .5f;
   return q;
}

//-----------------------------------------------------------------------------
// BQuat::unitLog
//-----------------------------------------------------------------------------
BQuat BQuat::unitLog(const BQuat& q) 
{
   BDEBUG_ASSERT(q.isUnit());
   float scale = sqrt(Math::Sqr(q.mVec[0]) + Math::Sqr(q.mVec[1]) + Math::Sqr(q.mVec[2]));
   float t = atan2(scale, q.mVec[3]);
   if (scale > 0.0f) 
      scale = t / scale;
   else 
      scale = 1.0f;
   return BQuat(scale * q.mVec[0], scale * q.mVec[1], scale * q.mVec[2], 0.0f);
}

//-----------------------------------------------------------------------------
// BQuat::unitExp
//-----------------------------------------------------------------------------
BQuat BQuat::unitExp(const BQuat& q) 
{
   BDEBUG_ASSERT(q.isPure());

   float t = sqrt(Math::Sqr(q.mVec[0]) + Math::Sqr(q.mVec[1]) + Math::Sqr(q.mVec[2]));
   float scale;
   if (t > Math::fSmallEpsilon) 
      scale = sin(t) / t;
   else
      scale = 1.0f;
   
   BQuat ret(scale * q.mVec[0], scale * q.mVec[1], scale * q.mVec[2], cos(t));

#ifdef DEBUG
//      BQuat temp(unitLog(ret));
//      BDEBUG_ASSERT(BQuat::equalTol(temp, q, Math::fLargeEpsilon));
#endif
   
   return ret;
}

//-----------------------------------------------------------------------------
// BQuat::unitPow
// interpolates from identity 
//-----------------------------------------------------------------------------
BQuat BQuat::unitPow(const BQuat& q, float p) 
{
   return unitExp(p * unitLog(q));
}

//-----------------------------------------------------------------------------
// BQuat::slerpLogForm
//-----------------------------------------------------------------------------
BQuat BQuat::slerpLogForm(const BQuat& a, const BQuat& b, float t)
{
   return a * unitPow(a.unitInverse() * b, t);
}

//-----------------------------------------------------------------------------
// BQuat::squareRoot
//-----------------------------------------------------------------------------
// same as pow(.5)
BQuat BQuat::squareRoot(const BQuat& q) 
{
   BDEBUG_ASSERT(q.isUnit());

   const float k = Math::ClampLow(0.5f * (q.scalar() + 1.0f), 0.0f);
   const float s = sqrt(k);   
   const float d = 1.0f / (2.0f * s);
   
   BQuat ret(q.mVec[0] * d, q.mVec[1] * d, q.mVec[2] * d, s);

#ifdef DEBUG
//      BDEBUG_ASSERT(ret.isUnit());
//      BQuat temp(ret * ret);
//      BDEBUG_ASSERT(BQuat::equalTol(temp, q, Math::fLargeEpsilon));
#endif

   return ret;
}

//-----------------------------------------------------------------------------
// BQuat::catmullRom
//-----------------------------------------------------------------------------
BQuat BQuat::catmullRom(const BQuat& q00, const BQuat& q01, const BQuat& q02, const BQuat& q03, float t)
{
   BQuat q10(slerp(q00, q01, t + 1.0f));   BQuat q11(slerp(q01, q02, t)); BQuat q12(slerp(q02, q03, t - 1.0f));
   BQuat q20(slerp(q10, q11, (t + 1.0f) * .5f)); BQuat q21(slerp(q11, q12, t * .5f));
   return slerp(q20, q21, t);
}

//-----------------------------------------------------------------------------
// BQuat::bezier
//-----------------------------------------------------------------------------
BQuat BQuat::bezier(const BQuat& q00, const BQuat& q01, const BQuat& q02, const BQuat& q03, float t)
{
   BQuat q10(slerp(q00, q01, t)); BQuat q11(slerp(q01, q02, t)); BQuat q12(slerp(q02, q03, t));
   BQuat q20(slerp(q10, q11, t)); BQuat q21(slerp(q11, q12, t));
   return slerp(q20, q21, t);
}

//-----------------------------------------------------------------------------
// BQuat::bSpline
//-----------------------------------------------------------------------------
BQuat BQuat::bSpline(const BQuat& q00, const BQuat& q01, const BQuat& q02, const BQuat& q03, float t)
{
   BQuat q10(slerp(q00, q01, (t + 2.0f) / 3.0f)); BQuat q11(slerp(q01, q02, (t + 1.0f) / 3.0f)); BQuat q12(slerp(q02, q03, t / 3.0f));
   BQuat q20(slerp(q10, q11, (t + 1.0f) * .5f)); BQuat q21(slerp(q11, q12, t * .5f));
   return slerp(q20, q21, t);
}

//-----------------------------------------------------------------------------
// BQuat::makeRandom
//-----------------------------------------------------------------------------
BQuat BQuat::makeRandom(float x1, float x2, float x3)
{
   const float z = x1;
   const float o = Math::fTwoPi * x2;
   const float r = sqrt(1.0f - z * z);
   const float w = Math::fPi * x3;
   const float sw = sin(w);
   return BQuat(sw * cos(o) * r, sw * sin(o) * r, sw * z, cos(w));
}

//-----------------------------------------------------------------------------
// BQuat::setFromEuler
//-----------------------------------------------------------------------------
BQuat& BQuat::setFromEuler(const BVec4& euler)
{
   const float a = euler[0];
   const float b = euler[1];
   const float c = euler[2];
   // not a particularly fast method
   const BQuat qX(sin(a * .5f), 0, 0, cos(a * .5f)); // roll
   const BQuat qY(0, sin(b * .5f), 0, cos(b * .5f)); // yaw
   const BQuat qZ(0, 0, sin(c * .5f), cos(c * .5f)); // pitch
   *this = qX * qY * qZ;
   return *this;
}

//-----------------------------------------------------------------------------
// BQuat::makeFromEuler
//-----------------------------------------------------------------------------
BQuat BQuat::makeFromEuler(const BVec4& euler)
{
   BQuat ret;
   return ret.setFromEuler(euler);
}
