//-----------------------------------------------------------------------------
// File: euler_angles.cpp - Convert Euler angles to/from matrix or quat
// Originally by Ken Shoemake, 1993 
// Converted to C++ by Rich Geldreich, July 2003
// FIXME: CAN I USE THIS CODE?
// The original code appeared in Graphics Gems, which I believe is public domain.
//-----------------------------------------------------------------------------
#include "euler_angles.h"

namespace gr
{
  /* Construct quaternion from Euler angles (in radians). */
  Quat& EulerAngles::toQuat(Quat& qu) const
  {
      float a[3], ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
      int i,j,k,h,n,s,f;
      EulGetOrd(order,i,j,k,h,n,s,f);
			float xx = x;
			float yy = y;
			float zz = z;
      if (f==EulFrmR) {float t = xx; xx = zz; zz = t;}
      if (n==EulParOdd) yy = -yy;
      ti = xx*0.5f; tj = yy*0.5f; th = zz*0.5f;
      ci = cos(ti);  cj = cos(tj);  ch = cos(th);
      si = sin(ti);  sj = sin(tj);  sh = sin(th);
      cc = ci*ch; cs = ci*sh; sc = si*ch; ss = si*sh;
      if (s==EulRepYes)
      {
        a[i] = cj*(cs + sc);  /* Could speed up with */
        a[j] = sj*(cc + ss);  /* trig identities. */
        a[k] = sj*(cs - sc);
        qu[W] = cj*(cc - ss);
      }
      else
      {
        a[i] = cj*sc - sj*cs;
        a[j] = cj*ss + sj*cc;
        a[k] = cj*cs - sj*sc;
        qu[W] = cj*cc + sj*ss;
      }
      if (n==EulParOdd) a[j] = -a[j];
      qu[X] = a[X]; qu[Y] = a[Y]; qu[Z] = a[Z];
      return (qu);
  }

  /* Construct matrix from Euler angles (in radians). */
  Matrix44& EulerAngles::toMatrix(Matrix44& M) const
  {
    float ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
    int i,j,k,h,n,s,f;
    EulGetOrd(order,i,j,k,h,n,s,f);
		float xx = x;
		float yy = y;
		float zz = z;
    if (f==EulFrmR) {float t = xx; xx = zz; zz = t;}
    if (n==EulParOdd) {xx = -xx; yy = -yy; zz = -zz;}
    ti = xx;    tj = yy;  th = zz;
    ci = cos(ti); cj = cos(tj); ch = cos(th);
    si = sin(ti); sj = sin(tj); sh = sin(th);
    cc = ci*ch; cs = ci*sh; sc = si*ch; ss = si*sh;
    if (s==EulRepYes)
    {
      M[i][i] = cj;   M[i][j] =  sj*si;    M[i][k] =  sj*ci;
      M[j][i] = sj*sh;  M[j][j] = -cj*ss+cc; M[j][k] = -cj*cs-sc;
      M[k][i] = -sj*ch; M[k][j] =  cj*sc+cs; M[k][k] =  cj*cc-ss;
    }
    else
    {
      M[i][i] = cj*ch; M[i][j] = sj*sc-cs; M[i][k] = sj*cc+ss;
      M[j][i] = cj*sh; M[j][j] = sj*ss+cc; M[j][k] = sj*cs-sc;
      M[k][i] = -sj;   M[k][j] = cj*si;    M[k][k] = cj*ci;
    }
  
    M[W][X]=M[W][Y]=M[W][Z]=M[X][W]=M[Y][W]=M[Z][W]=0.0f; M[W][W]=1.0f;
    return M;
  }

  /* Convert matrix to Euler angles (in radians). */
  EulerAngles& EulerAngles::setFromMatrix(const Matrix44& M, int newOrder)
  {
    EulerAngles& ea = *this;

    int i,j,k,h,n,s,f;
    ea.order = (newOrder != -1) ? newOrder : order;
    EulGetOrd(ea.order,i,j,k,h,n,s,f);
    if (s==EulRepYes)
    {
      float sy = sqrt(M[i][j]*M[i][j] + M[i][k]*M[i][k]);
      if (sy > 16*FLT_EPSILON)
      {
        ea.x = atan2(M[i][j], M[i][k]);
        ea.y = atan2(sy, M[i][i]);
        ea.z = atan2(M[j][i], -M[k][i]);
      }
      else
      {
        ea.x = atan2(-M[j][k], M[j][j]);
        ea.y = atan2(sy, M[i][i]);
        ea.z = 0;
      }
    }
    else
    {
      float cy = sqrt(M[i][i]*M[i][i] + M[j][i]*M[j][i]);
      if (cy > 16*FLT_EPSILON)
      {
        ea.x = atan2(M[k][j], M[k][k]);
        ea.y = atan2(-M[k][i], cy);
        ea.z = atan2(M[j][i], M[i][i]);
      }
      else
      {
        ea.x = atan2(-M[j][k], M[j][j]);
        ea.y = atan2(-M[k][i], cy);
        ea.z = 0;
      }
    }
    if (n==EulParOdd) {ea.x = -ea.x; ea.y = -ea.y; ea.z = -ea.z;}
    if (f==EulFrmR) {float t = ea.x; ea.x = ea.z; ea.z = t;}

    return *this;
  }

  /* Convert quaternion to Euler angles (in radians). */
  EulerAngles& EulerAngles::setFromQuat(const Quat& q, int newOrder)
  {
    Matrix44 M;

    float Nq = q[X]*q[X]+q[Y]*q[Y]+q[Z]*q[Z]+q[W]*q[W];
    float s = (Nq > 0.0f) ? (2.0f / Nq) : 0.0;
    float xs = q[X]*s,    ys = q[Y]*s,  zs = q[Z]*s;
    float wx = q[W]*xs,   wy = q[W]*ys,   wz = q[W]*zs;
    float xx = q[X]*xs,   xy = q[X]*ys,   xz = q[X]*zs;
    float yy = q[Y]*ys,   yz = q[Y]*zs,   zz = q[Z]*zs;
  
    M[X][X] = 1.0f - (yy + zz); M[X][Y] = xy - wz; M[X][Z] = xz + wy;
    M[Y][X] = xy + wz; M[Y][Y] = 1.0f - (xx + zz); M[Y][Z] = yz - wx;
    M[Z][X] = xz - wy; M[Z][Y] = yz + wx; M[Z][Z] = 1.0f - (xx + yy);
    M[W][X]=M[W][Y]=M[W][Z]=M[X][W]=M[Y][W]=M[Z][W]=0.0f; M[W][W]=1.0f;
    return setFromMatrix(M, newOrder);
  }

	Quat EulerAngles::createQuat(const EulerAngles& eu)
	{
		Quat ret;
		return eu.toQuat(ret);
	}

	Matrix44 EulerAngles::createMatrix(const EulerAngles& eu)
	{
		Matrix44 ret;
		return eu.toMatrix(ret);
	}

} // namespace gr

