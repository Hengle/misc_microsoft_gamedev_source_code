//============================================================================
// quat.h
//
// XBox Quaternion class
//
// Copyright (c) 2002, Ensemble Studios
//============================================================================

#pragma once

// Includes
#include "math\vector.h"


//============================================================================
// BQuat
//============================================================================
class BQuat : public D3DXQUATERNION
{
   public:

      //-- Constructors
      BQuat() : D3DXQUATERNION()                                             {};
      BQuat(float x, float y, float z, float w) : D3DXQUATERNION(x, y, z, w) {};
      BQuat(const D3DXQUATERNION& q) : D3DXQUATERNION(q.x, q.y, q.z, q.w)      {};
      
      //-- Destructors
     ~BQuat() {};

      //-- Accessors
      inline float length        () const;
      inline float lengthSquared () const;

      inline void  getRotation   (BQuat& rotation) const;
      inline void  getRotation   (BVector& v, float& radians) const;
      inline void  getRotationDeg(BVector& v, float& degrees) const;
      inline void  getRotationMatrix (BPhysicsMatrix &m) const;
      inline void  getOrientationVectors(BVector *pvDir, BVector *pvUp, BVector *pvRight) const;
                
      //-- Mutators
      inline void  identity      ();
      inline void  invert        ();
      inline void  naturalLog    ();
      inline void  multiply      (const BQuat& q);
      inline void  normalize     ();
      inline void  slerp         (const BQuat& q1, const BQuat& q2, float alpha);
      inline void  squad         (const BQuat& q1, const BQuat& q2, const BQuat& q3, const BQuat& q4, float alpha);
      inline void  exponential   ();
      inline void  conjugate     (); 
      inline float dot           (const BQuat& q);

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
// inline BQuat::set(v0,v1)
//
// intiialization that accepts two vectors and constucts the rotation from v0 to v1
// This tecnique was adapted from Game Programming Gems p.217
//----------------------------------------------------------------------------
void BQuat::set(const BVector &inV0, const BVector &inV1)
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
// inline float BQuat::length() const
//
// returns the length (magnitude) of the quat
//----------------------------------------------------------------------------
inline float BQuat::length() const
{
   return D3DXQuaternionLength(this);
}

//----------------------------------------------------------------------------
// inline float BQuat::lengthSquared() const
// 
// returns the length squared (magnitude squared) of the quat
//----------------------------------------------------------------------------
inline float BQuat::lengthSquared() const
{
   return D3DXQuaternionLengthSq(this);
}

//----------------------------------------------------------------------------
// inline void BQuat::getRotation(BQuat& rotation) const
//
// copies the quat into the passed in quat
//----------------------------------------------------------------------------
inline void BQuat::getRotation(BQuat& rotation) const
{
   rotation.x = x;
   rotation.y = y;
   rotation.z = z;
   rotation.w = w;
}

//----------------------------------------------------------------------------
// inline void BQuat::getRotation(BVector& v, float& radians) const
//
// Returns the axis angle (radians) represantion of the quat
//----------------------------------------------------------------------------
inline void BQuat::getRotation(BVector& v, float& radians) const
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
// inline void BQuat::getRotationDeg(BVector& v, float& degrees) const
//
// Returns the axis angle (degrees) represantion of the quat
//----------------------------------------------------------------------------
inline void BQuat::getRotationDeg(BVector& v, float& degrees) const
{
   float radians = 0.0f;   
   getRotation(v, radians);
   degrees = RADTODEG(radians);
}

//----------------------------------------------------------------------------
// inline void BQuat::getRotationMatrix(BPhysicsMatrix& m) const
//
// Builds a rotation matrix from a quaternion
//----------------------------------------------------------------------------
inline void BQuat::getRotationMatrix(BPhysicsMatrix& m) const
{
   D3DXMatrixRotationQuaternion( &m, this);
}

//----------------------------------------------------------------------------
// inline void BQuat::getOrientationVectors(BVector *pvDir, BVector *pvUp, BVector *pvRight) const
//
// Converts the Quaternion to orientation vectors.
//----------------------------------------------------------------------------
inline void BQuat::getOrientationVectors(BVector *pvDir, BVector *pvUp, BVector *pvRight) const
{
   BPhysicsMatrix m;
   D3DXMatrixRotationQuaternion(&m, this);

   *pvRight = m.getRight();
   *pvUp = m.getUp();
   *pvDir = m.getForward();
}

//----------------------------------------------------------------------------
// inline void BQuat::identity()
//
// stores the quat identity in the quat
//----------------------------------------------------------------------------
inline void BQuat::identity()
{
   D3DXQuaternionIdentity(this);
}

//----------------------------------------------------------------------------
// inline void BQuat::invert()
// 
// inverts the quat
//----------------------------------------------------------------------------
inline void BQuat::invert()
{
   D3DXQuaternionInverse(this, this);
}

//----------------------------------------------------------------------------
// inline void BQuat::naturalLog()
//
// computes and stores the natural log of the quat
//----------------------------------------------------------------------------
inline void BQuat::naturalLog()
{
   D3DXQuaternionLn(this, this);
}

//----------------------------------------------------------------------------
// inline void BQuat::multiply(const BQuat& q)
//
// performs/stores the non commutative multiplication of the passed in quat
// with itself where Out = q * this
//----------------------------------------------------------------------------
inline void BQuat::multiply(const BQuat& q)
{
   D3DXQuaternionMultiply(this, this, &q);
}

//----------------------------------------------------------------------------
// inline void BQuat::normalize()
//
// normalizes the quaternion to become a unit quaternion
//----------------------------------------------------------------------------
inline void BQuat::normalize()
{   
   D3DXQuaternionNormalize(this, this);   
}

//----------------------------------------------------------------------------
// inline void BQuat::slerp(const BQuat& q1, const BQuat& q2, float alpha)
// 
// computes and stores the spherical interpolation between two quaternion 
// with the given alpha value
//----------------------------------------------------------------------------
inline void BQuat::slerp(const BQuat& q1, const BQuat& q2, float alpha)
{
   if (alpha < 0.0f) alpha = 0.0f;
   if (alpha > 1.0f) alpha = 1.0f;

   D3DXQuaternionSlerp(this, &q1, &q2, alpha);
}

//----------------------------------------------------------------------------
// inline void BQuat::squad(const BQuat& q1, const BQuat& q2, const BQuat& q3, const BQuat& q4, float alpha)
//
// computes and stores the quadratic spherical interpolation of 4 points in
// the quaternion
//----------------------------------------------------------------------------
inline void BQuat::squad(const BQuat& q1, const BQuat& q2, const BQuat& q3, const BQuat& q4, float alpha)
{
   if (alpha < 0.0f) alpha = 0.0f;
   if (alpha > 1.0f) alpha = 1.0f;

   D3DXQuaternionSquad(this, &q1, &q2, &q3, &q4, alpha);
}

//----------------------------------------------------------------------------
// inline void BQuat::exponential()
//
// calculate the exponential of the quat and store it.
// Note: this only works on pure quaternions (w == 0)
//----------------------------------------------------------------------------
inline void BQuat::exponential()
{
   D3DXQuaternionExp(this, this);
}

//----------------------------------------------------------------------------
// inline void BQuat::conjugate()
//
// stores the conjugate of the current quaternion. (x,y,z,w) => (-x,-y,-z,w)
//----------------------------------------------------------------------------
inline void BQuat::conjugate()
{
   D3DXQuaternionConjugate(this, this);
}

//----------------------------------------------------------------------------
// inline void BQuat::dot()
//
// performs the dot product on the two quaternions and returns the value
//----------------------------------------------------------------------------
inline float BQuat::dot(const BQuat& q)
{
   return( D3DXQuaternionDot(this, &q) );
}

//----------------------------------------------------------------------------
// inline void BQuat::setRotation(float yaw, float pitch, float roll)
//
// sets the quaternion up to have a rotation with the passed in yaw, pitch, roll
// where : yaw   == rotation around y axis
//         pitch == rotation around x axis
//         roll  == rotation around z axis
//----------------------------------------------------------------------------
inline void BQuat::setRotation(float yaw, float pitch, float roll)
{
   D3DXQuaternionRotationYawPitchRoll(this, yaw, pitch, roll);
}

//----------------------------------------------------------------------------
// inline void BQuat::setRotation(const D3DXMATRIX& m)
//
// stores the rotation of the matrix in the current quat.
//----------------------------------------------------------------------------
inline void BQuat::setRotation(const D3DXMATRIX& m)
{
   D3DXQuaternionRotationMatrix(this, &m);
}

//----------------------------------------------------------------------------
// inline void BQuat::setRotation(const BVector& v, float radians)
//
// creates a quaternion that stores the rotation around an arbitrary axis with
// the given angle of rotation in degrees
//----------------------------------------------------------------------------
inline void BQuat::setRotation(const BVector& v, float radians)
{
   D3DXQuaternionRotationAxis(this, (D3DXVECTOR3*)&v, radians);
}

//----------------------------------------------------------------------------
//inline void BQuat::setRotation(float x, float y, float z, radians)
//
// creates a quaternion that stores the rotation around an arbitrary axis with
// the given angle of rotation in radians 
// NOTE: this creates a temp BVector
//----------------------------------------------------------------------------
inline void BQuat::setRotation(float x, float y, float z, float radians)
{
   BVector v(x, y, z);
   D3DXQuaternionRotationAxis(this, (D3DXVECTOR3*)&v, radians);
}

//----------------------------------------------------------------------------
// inline void BQuat::setRotationDeg(const BVector& v, float angle)
// 
// creates a quaternion that stores the rotation around an arbitrary axis with
// the given angle of rotation in degrees
//----------------------------------------------------------------------------
inline void BQuat::setRotationDeg(const BVector& v, float degrees)
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
inline void  BQuat::setRotationDeg(float x, float y, float z, float degrees)
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
inline void  BQuat::set(float x, float y, float z, float w)
{
   this->x = x;
   this->y = y;
   this->z = z;
   this->w = w;
}