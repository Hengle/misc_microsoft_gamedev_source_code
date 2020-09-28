//============================================================================
// quat.h
//
// XBox Quaternion class
//
// Copyright (c) 2002, Ensemble Studios
//============================================================================

#ifndef _PHYSICS_QUAT_H_
#define _PHYSICS_QUAT_H_

#include "physics.h"

//============================================================================
// BPhysicsQuat
//============================================================================
class BPhysicsQuat : public D3DXQUATERNION
{
   public:

      //-- Constructors
      BPhysicsQuat() : D3DXQUATERNION()                                             {};
      BPhysicsQuat(float x, float y, float z, float w) : D3DXQUATERNION(x, y, z, w) {};
      BPhysicsQuat(const D3DXQUATERNION& q) : D3DXQUATERNION(q.x, q.y, q.z, q.w)      {};
      
      //-- Destructors
     ~BPhysicsQuat() {};

      //-- Accessors
      inline float length        () const;
      inline float lengthSquared () const;

      inline void  getRotation   (BPhysicsQuat& rotation) const;
      inline void  getRotation   (BVector& v, float& radians) const;
      inline void  getRotationDeg(BVector& v, float& degrees) const;
      inline void  getRotationMatrix (BPhysicsMatrix &m) const;
      inline void  getOrientationVectors(BVector *pvDir, BVector *pvUp, BVector *pvRight) const;
                
      //-- Mutators
      inline void  identity      ();
      inline void  invert        ();
      inline void  naturalLog    ();
      inline void  multiply      (const BPhysicsQuat& q);
      inline void  normalize     ();
      inline void  slerp         (const BPhysicsQuat& q1, const BPhysicsQuat& q2, float alpha);
      inline void  squad         (const BPhysicsQuat& q1, const BPhysicsQuat& q2, const BPhysicsQuat& q3, const BPhysicsQuat& q4, float alpha);
      inline void  exponential   ();
      inline void  conjugate     (); 
      inline float dot           (const BPhysicsQuat& q);

      inline void  setRotation   (float yaw, float pitch, float roll);
      inline void  setRotation   (const D3DXMATRIX& m);
      inline void  setRotation   (const BVector& v, float radians);
      inline void  setRotation   (float x, float y, float z, float radians);
      inline void  setRotationDeg(const BVector& v, float degrees);
      inline void  setRotationDeg(float x, float y, float z, float degrees);

      inline void  set           (float x, float y, float z, float w);
      inline void  set(const BVector &v0, const BVector &v1);


      //-- Operators defined in base class
};

//----------------------------------------------------------------------------
// inline BPhysicsQuat::set(v0,v1)
//
// intiialization that accepts two vectors and constucts the rotation from v0 to v1
// This tecnique was adapted from Game Programming Gems p.217
//----------------------------------------------------------------------------
void BPhysicsQuat::set(const BVector &inV0, const BVector &inV1)
{
   BVector v0 = inV0;
   v0.normalize();
   BVector v1 = inV1;
   v1.normalize();
   BVector c;
   c.assignCrossProduct(v0,v1);
   float d = v0.dot(v1);
   float s = (float) sqrt((1+d) * 2);
   x = c.x / s;
   y = c.y / s;
   z = c.z / s;
   w = s / 2.0f;
}

//----------------------------------------------------------------------------
// inline float BPhysicsQuat::length() const
//
// returns the length (magnitude) of the quat
//----------------------------------------------------------------------------
inline float BPhysicsQuat::length() const
{
   return D3DXQuaternionLength(this);
}

//----------------------------------------------------------------------------
// inline float BPhysicsQuat::lengthSquared() const
// 
// returns the length squared (magnitude squared) of the quat
//----------------------------------------------------------------------------
inline float BPhysicsQuat::lengthSquared() const
{
   return D3DXQuaternionLengthSq(this);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::getRotation(BPhysicsQuat& rotation) const
//
// copies the quat into the passed in quat
//----------------------------------------------------------------------------
inline void BPhysicsQuat::getRotation(BPhysicsQuat& rotation) const
{
   rotation.x = x;
   rotation.y = y;
   rotation.z = z;
   rotation.w = w;
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::getRotation(BVector& v, float& radians) const
//
// Returns the axis angle (radians) represantion of the quat
//----------------------------------------------------------------------------
inline void BPhysicsQuat::getRotation(BVector& v, float& radians) const
{            
   D3DXQuaternionToAxisAngle(this, (D3DXVECTOR3*)&v, &radians);
   v.normalize();

   /*
	//-- Cache variables
	float tx  = x;
	float ty  = y;
	float tz  = z;
	float len = tx*tx + ty*ty + tz*tz;

	if (len > 0.000000001f) 
	{
		v.x = tx * (1.0f / len);
		v.y = ty * (1.0f / len);
		v.z = tz * (1.0f / len);
		radians = 2.0f * (float) acos(w);
	}
	else
	{
		v.x = 0.0f;
		v.y = 0.0f;
		v.z = 1.0f;
		radians = 0.0f;
	}
   
	//-- Normalize
	float mag = (float)sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	v.x /= mag;
	v.y /= mag;
	v.z /= mag;
   */
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::getRotationDeg(BVector& v, float& degrees) const
//
// Returns the axis angle (degrees) represantion of the quat
//----------------------------------------------------------------------------
inline void BPhysicsQuat::getRotationDeg(BVector& v, float& degrees) const
{
   float radians = 0.0f;   
   getRotation(v, radians);
   degrees = RADTODEG(radians);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::getRotationMatrix(BPhysicsMatrix& m) const
//
// Builds a rotation matrix from a quaternion
//----------------------------------------------------------------------------
inline void BPhysicsQuat::getRotationMatrix(BPhysicsMatrix& m) const
{
   D3DXMatrixRotationQuaternion( &m, this);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::getOrientationVectors(BVector *pvDir, BVector *pvUp, BVector *pvRight) const
//
// Converts the Quaternion to orientation vectors.
//----------------------------------------------------------------------------
inline void BPhysicsQuat::getOrientationVectors(BVector *pvDir, BVector *pvUp, BVector *pvRight) const
{
   BPhysicsMatrix m;
   D3DXMatrixRotationQuaternion(&m, this);

   *pvRight = m.getRight();
   *pvUp = m.getUp();
   *pvDir = m.getForward();
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::identity()
//
// stores the quat identity in the quat
//----------------------------------------------------------------------------
inline void BPhysicsQuat::identity()
{
   D3DXQuaternionIdentity(this);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::invert()
// 
// inverts the quat
//----------------------------------------------------------------------------
inline void BPhysicsQuat::invert()
{
   D3DXQuaternionInverse(this, this);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::naturalLog()
//
// computes and stores the natural log of the quat
//----------------------------------------------------------------------------
inline void BPhysicsQuat::naturalLog()
{
   D3DXQuaternionLn(this, this);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::multiply(const BPhysicsQuat& q)
//
// performs/stores the non commutative multiplication of the passed in quat
// with itself where Out = q * this
//----------------------------------------------------------------------------
inline void BPhysicsQuat::multiply(const BPhysicsQuat& q)
{
   D3DXQuaternionMultiply(this, this, &q);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::normalize()
//
// normalizes the quaternion to become a unit quaternion
//----------------------------------------------------------------------------
inline void BPhysicsQuat::normalize()
{   
   D3DXQuaternionNormalize(this, this);   
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::slerp(const BPhysicsQuat& q1, const BPhysicsQuat& q2, float alpha)
// 
// computes and stores the spherical interpolation between two quaternion 
// with the given alpha value
//----------------------------------------------------------------------------
inline void BPhysicsQuat::slerp(const BPhysicsQuat& q1, const BPhysicsQuat& q2, float alpha)
{
   if (alpha < 0.0f) alpha = 0.0f;
   if (alpha > 1.0f) alpha = 1.0f;

   D3DXQuaternionSlerp(this, &q1, &q2, alpha);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::squad(const BPhysicsQuat& q1, const BPhysicsQuat& q2, const BPhysicsQuat& q3, const BPhysicsQuat& q4, float alpha)
//
// computes and stores the quadratic spherical interpolation of 4 points in
// the quaternion
//----------------------------------------------------------------------------
inline void BPhysicsQuat::squad(const BPhysicsQuat& q1, const BPhysicsQuat& q2, const BPhysicsQuat& q3, const BPhysicsQuat& q4, float alpha)
{
   if (alpha < 0.0f) alpha = 0.0f;
   if (alpha > 1.0f) alpha = 1.0f;

   D3DXQuaternionSquad(this, &q1, &q2, &q3, &q4, alpha);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::exponential()
//
// calculate the exponential of the quat and store it.
// Note: this only works on pure quaternions (w == 0)
//----------------------------------------------------------------------------
inline void BPhysicsQuat::exponential()
{
   D3DXQuaternionExp(this, this);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::conjugate()
//
// stores the conjugate of the current quaternion. (x,y,z,w) => (-x,-y,-z,w)
//----------------------------------------------------------------------------
inline void BPhysicsQuat::conjugate()
{
   D3DXQuaternionConjugate(this, this);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::dot()
//
// performs the dot product on the two quaternions and returns the value
//----------------------------------------------------------------------------
inline float BPhysicsQuat::dot(const BPhysicsQuat& q)
{
   return( D3DXQuaternionDot(this, &q) );
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::setRotation(float yaw, float pitch, float roll)
//
// sets the quaternion up to have a rotation with the passed in yaw, pitch, roll
// where : yaw   == rotation around y axis
//         pitch == rotation around x axis
//         roll  == rotation around z axis
//----------------------------------------------------------------------------
inline void BPhysicsQuat::setRotation(float yaw, float pitch, float roll)
{
   D3DXQuaternionRotationYawPitchRoll(this, yaw, pitch, roll);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::setRotation(const D3DXMATRIX& m)
//
// stores the rotation of the matrix in the current quat.
//----------------------------------------------------------------------------
inline void BPhysicsQuat::setRotation(const D3DXMATRIX& m)
{
   D3DXQuaternionRotationMatrix(this, &m);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::setRotation(const BVector& v, float radians)
//
// creates a quaternion that stores the rotation around an arbitrary axis with
// the given angle of rotation in degrees
//----------------------------------------------------------------------------
inline void BPhysicsQuat::setRotation(const BVector& v, float radians)
{
   D3DXQuaternionRotationAxis(this, (D3DXVECTOR3*)&v, radians);
}

//----------------------------------------------------------------------------
//inline void BPhysicsQuat::setRotation(float x, float y, float z, radians)
//
// creates a quaternion that stores the rotation around an arbitrary axis with
// the given angle of rotation in radians 
// NOTE: this creates a temp BVector
//----------------------------------------------------------------------------
inline void BPhysicsQuat::setRotation(float x, float y, float z, float radians)
{
   BVector v(x, y, z);
   D3DXQuaternionRotationAxis(this, (D3DXVECTOR3*)&v, radians);
}

//----------------------------------------------------------------------------
// inline void BPhysicsQuat::setRotationDeg(const BVector& v, float angle)
// 
// creates a quaternion that stores the rotation around an arbitrary axis with
// the given angle of rotation in degrees
//----------------------------------------------------------------------------
inline void BPhysicsQuat::setRotationDeg(const BVector& v, float degrees)
{
   float radians = DEGTORAD(degrees);
   D3DXQuaternionRotationAxis(this, (D3DXVECTOR3*)&v,radians);
}

//----------------------------------------------------------------------------
// inline void  setRotationDeg(float x, float y, float z, float degrees);
// 
// creates a quaternion that stores the rotation around an arbitrary axis with
// the given angle of rotation in degrees
// NOTE: this creates a temp BVector
//----------------------------------------------------------------------------
inline void  BPhysicsQuat::setRotationDeg(float x, float y, float z, float degrees)
{
   BVector v(x, y, z);
   float radians = DEGTORAD(degrees);   
   D3DXQuaternionRotationAxis(this, (D3DXVECTOR3*)&v,radians);  
}

//----------------------------------------------------------------------------
// inline void  set(float x, float y, float z, float w);
// 
// creates a quaternion from the individual components
//----------------------------------------------------------------------------
inline void  BPhysicsQuat::set(float x, float y, float z, float w)
{
   this->x = x;
   this->y = y;
   this->z = z;
   this->w = w;
}


#endif