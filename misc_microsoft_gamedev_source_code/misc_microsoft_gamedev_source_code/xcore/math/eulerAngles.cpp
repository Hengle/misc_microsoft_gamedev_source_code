//-----------------------------------------------------------------------------
// File: euler_angles.cpp - Convert Euler angles to/from matrix or quat
// Originally by Ken Shoemake, 1993 
// Converted to C++ by Rich Geldreich, July 2003
// FIXME: CAN I USE THIS CODE?
// The original code appeared in Graphics Gems, which I believe is public domain.
//-----------------------------------------------------------------------------
#include "xcore.h"
#include "eulerAngles.h"

/* Construct quaternion from Euler angles (in radians). */
BQuat& BEulerAngles::toQuat(BQuat& qu) const
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
BMatrix& BEulerAngles::toMatrix(BMatrix& M) const
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
      M.setElement(i, i, cj);   M.setElement(i, j, sj*si);    M.setElement(i, k, sj*ci);
      M.setElement(j, i, sj*sh);  M.setElement(j, j, -cj*ss+cc); M.setElement(j, k, -cj*cs-sc);
      M.setElement(k, i, -sj*ch); M.setElement(k, j, cj*sc+cs); M.setElement(k, k, cj*cc-ss);
   }
   else
   {
      M.setElement(i, i, cj*ch); M.setElement(i, j, sj*sc-cs); M.setElement(i, k, sj*cc+ss);
      M.setElement(j, i, cj*sh); M.setElement(j, j, sj*ss+cc); M.setElement(j, k, sj*cs-sc);
      M.setElement(k, i, -sj);   M.setElement(k, j, cj*si);    M.setElement(k, k, cj*ci);
   }

   M.setElement(W, X, 0.0f); 
   M.setElement(W, Y, 0.0f); 
   M.setElement(W, Z, 0.0f); 
   M.setElement(X, W, 0.0f); 
   M.setElement(Y, W, 0.0f); 
   M.setElement(Z, W, 0.0f);
   M.setElement(W, W, 1.0f);
   return M;
}

/* Convert matrix to Euler angles (in radians). */
BEulerAngles& BEulerAngles::setFromMatrix(const BMatrix& M, int newOrder)
{
   BEulerAngles& ea = *this;

   int i,j,k,h,n,s,f;
   ea.order = (newOrder != -1) ? newOrder : order;
   EulGetOrd(ea.order,i,j,k,h,n,s,f);
   if (s==EulRepYes)
   {
   float sy = sqrt(M.getElement(i, j)*M.getElement(i, j) + M.getElement(i, k)*M.getElement(i, k));
   if (sy > 16*FLT_EPSILON)
   {
      ea.x = atan2(M.getElement(i, j), M.getElement(i, k));
      ea.y = atan2(sy, M.getElement(i, i));
      ea.z = atan2(M.getElement(j, i), -M.getElement(k, i));
   }
   else
   {
      ea.x = atan2(-M.getElement(j, k), M.getElement(j, j));
      ea.y = atan2(sy, M.getElement(i, i));
      ea.z = 0;
   }
   }
   else
   {
   float cy = sqrt(M.getElement(i, i)*M.getElement(i, i) + M.getElement(j, i)*M.getElement(j, i));
   if (cy > 16*FLT_EPSILON)
   {
      ea.x = atan2(M.getElement(k, j), M.getElement(k, k));
      ea.y = atan2(-M.getElement(k, i), cy);
      ea.z = atan2(M.getElement(j, i), M.getElement(i, i));
   }
   else
   {
      ea.x = atan2(-M.getElement(j, k), M.getElement(j, j));
      ea.y = atan2(-M.getElement(k, i), cy);
      ea.z = 0;
   }
   }
   if (n==EulParOdd) {ea.x = -ea.x; ea.y = -ea.y; ea.z = -ea.z;}
   if (f==EulFrmR) {float t = ea.x; ea.x = ea.z; ea.z = t;}

   return *this;
}

/* Convert quaternion to Euler angles (in radians). */
BEulerAngles& BEulerAngles::setFromQuat(const BQuat& q, int newOrder)
{
   BMatrix M;

   float Nq = q[X]*q[X]+q[Y]*q[Y]+q[Z]*q[Z]+q[W]*q[W];
   float s = (Nq > 0.0f) ? (2.0f / Nq) : 0.0f;
   float xs = q[X]*s,    ys = q[Y]*s,  zs = q[Z]*s;
   float wx = q[W]*xs,   wy = q[W]*ys,   wz = q[W]*zs;
   float xx = q[X]*xs,   xy = q[X]*ys,   xz = q[X]*zs;
   float yy = q[Y]*ys,   yz = q[Y]*zs,   zz = q[Z]*zs;

   M.setElement(X, X, 1.0f - (yy + zz)); M.setElement(X, Y, xy - wz); M.setElement(X, Z, xz + wy);
   M.setElement(Y, X, xy + wz); M.setElement(Y, Y, 1.0f - (xx + zz)); M.setElement(Y, Z, yz - wx);
   M.setElement(Z, X, xz - wy); M.setElement(Z, Y, yz + wx); M.setElement(Z, Z, 1.0f - (xx + yy));
   M.setElement(W, X, 0.0f); 
   M.setElement(W, Y, 0.0f); 
   M.setElement(W, Z, 0.0f); 
   M.setElement(X, W, 0.0f); 
   M.setElement(Y, W, 0.0f); 
   M.setElement(Z, W, 0.0f); 
   M.setElement(W, W, 1.0f);
   return setFromMatrix(M, newOrder);
}

BQuat BEulerAngles::createQuat(const BEulerAngles& eu)
{
   BQuat ret;
   return eu.toQuat(ret);
}

BMatrix BEulerAngles::createMatrix(const BEulerAngles& eu)
{
   BMatrix ret;
   return eu.toMatrix(ret);
}

