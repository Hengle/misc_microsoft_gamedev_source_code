//=============================================================================
// plane.cpp
//
// Copyright (c) 2000 Ensemble Studios
//=============================================================================


#include "xsystem.h"
#include "plane.h"
#include "math\vector.h"

//=============================================================================
// BPlane Constructor
//=============================================================================
BPlane::BPlane() :
   mNormal(cOriginVector),
   mDistance(0.0f)
{
}

//=============================================================================
// BPlane Constructor
//
// Construct a plane from three points on the plane.
//=============================================================================
BPlane::BPlane(const BVector &point1, const BVector &point2, const BVector &point3)
{
   assign(point1, point2, point3);
}

//=============================================================================
// BPlane Constructor
//
// Construct a plane from a normal and a point on the plane.
//=============================================================================
BPlane::BPlane(const BVector &normal, const BVector &point)
{
   // Save the normal
   mNormal = normal;

   // Calculate the distance
   mDistance = -(mNormal.dot(point));
}

//=============================================================================
// BPlane Copy Constructor
//=============================================================================
BPlane::BPlane(const BPlane& sourcePlane) 
{
   *this = sourcePlane;
}

//=============================================================================
// BPlane Normal + dist Constructor
//=============================================================================
BPlane::BPlane(const float a, const float b, const float c, const float d) :
   mNormal(a, b, c),
   mDistance(d)
{
   normalize();		
}

//=============================================================================
// BPlane Destructor
//=============================================================================
BPlane::~BPlane()
{
   // Nothing
}

//=============================================================================
// BPlane copy operator
//=============================================================================
BPlane& BPlane::operator =(const BPlane& sourcePlane)
{
   mNormal = sourcePlane.mNormal;
   mDistance = sourcePlane.mDistance;

   return(*this);
}

//=============================================================================
// BPlane equlatiy operator
//=============================================================================
bool BPlane::operator ==(const BPlane& sourcePlane) const
{
   if (mNormal == sourcePlane.mNormal && mDistance == sourcePlane.mDistance)
      return true;
   else
      return false;
}

//=============================================================================
// BPlane::assign
//=============================================================================
void BPlane::assign(const BVector &point1, const BVector &point2, const BVector &point3)
{
   // Calculate the plane normal
   mNormal.assignCrossProduct( (point2 - point1), (point3 - point1) );

   // Make sure this is a valid polygon... if the normal is 0,0,0 then it's not.
   // This is an error in the level file.
   if (mNormal == cOriginVector)
   {
      mDistance=0.0f;
      return;
   }

   // Make it a unit vector.
   mNormal.normalize();

   // Calculate the distance
   mDistance = -(mNormal.dot(point1));
}

//=============================================================================
// BPlane::assign
//=============================================================================
void BPlane::assign(const BVector *vertices, const WORD indices[3])
{
   BVector point1(vertices[indices[0]]);
   BVector point2(vertices[indices[1]]);
   BVector point3(vertices[indices[2]]);

   // Calculate the plane normal
   mNormal.assignCrossProduct( (point2 - point1), (point3 - point1) );

   // Make sure this is a valid polygon... if the normal is 0,0,0 then it's not.
   // This is an error in the level file.
   if (mNormal == cOriginVector)
   {
      mDistance=0.0f;
      return;
   }

   // Make it a unit vector.
   mNormal.normalize();

   // Calculate the distance
   mDistance = -(mNormal.dot(point1));
}

//=============================================================================
// BPlane::assign
//=============================================================================
void BPlane::assign(const BVector &normal, const BVector &point)
{
   // Save the normal
   mNormal = normal;

   // Calculate the distance
   mDistance = -(mNormal.dot(point));
}

//=============================================================================
// BPlane::assign
//=============================================================================
void BPlane::assign(const float a, const float b, const float c, const float d)
{
   mNormal.x = a;
   mNormal.y = b;
   mNormal.z = c;

   mDistance = d;

   normalize();
}

//=============================================================================
// BPlane::normalize
//=============================================================================
void BPlane::normalize()
{
   if (mDistance != 0.0f)
   {
      float s = mNormal.length();
      mDistance = mDistance / s;
   }
   mNormal.normalize();
       

}

//=============================================================================
// BPlane::checkPoint
//=============================================================================
long BPlane::checkPoint(const BVector &point) const
{
   // Put the point through the plane equation.
   float val = (point.dot(mNormal)) + mDistance;

   float compVal = 0.001f; // cFloatCompareEpsilon is too small!

   // If it's positive, then the point is in front of the plane.
   if (val > compVal /*cFloatCompareEpsilon*/)
      return cFrontOfPlane;

   // If it's negative, then the point is behind the plane.
   if (val < -compVal /*-cFloatCompareEpsilon*/)
      return cBehindPlane;

   // Very close to zero... so it's on the plane.
   return cOnPlane;
}

//=============================================================================
// BPlane::checkSphere
//=============================================================================
long BPlane::checkSphere(const BVector &position, float radius) const
{
   // Put the point through the plane equation
	float val = (position.dot(mNormal)) + mDistance;

   float compVal = 0.001f; // cFloatCompareEpsilon is too small!

   // If it's positive, then the sphere is in front of the plane.
	if ((val - radius) > compVal /*cFloatCompareEpsilon*/)
		return cFrontOfPlane;

   // If it's negative, then the point is behind the plane.
	if ((val + radius) < -compVal /*-cFloatCompareEpsilon*/)
		return cBehindPlane;

   // Very close to zero... so it's on the plane.
	return cOnPlane;
}

//=============================================================================
// BPlane::intersect
//=============================================================================
float BPlane::intersect(const BVector &point, const BVector &vector) const
{
   float den = mNormal.dot(vector);

   if (den > -cFloatCompareEpsilon && den < cFloatCompareEpsilon)
      return -1.0f;

   float val = (point.dot(mNormal)) + mDistance;

   return -(val / den);
}

//=============================================================================
// BPlane::closest
//=============================================================================
BVector BPlane::closest(const BVector &point) const
{
   float distance = intersect(point, -mNormal);
   return (point - (mNormal * distance));
}

//=============================================================================
// BPlane::calcX
//=============================================================================
float BPlane::calcX(float y, float z) const
{
   if (mNormal.x == 0.0f)
      return 0.0f;

   return -(mDistance + (y * mNormal.y) + (z * mNormal.z)) / mNormal.x;
}

//=============================================================================
// BPlane::calcY
//=============================================================================
float BPlane::calcY(float x, float z) const
{
   if (mNormal.y == 0.0f)
      return 0.0f;

   return -(mDistance + (x * mNormal.x) + (z * mNormal.z)) / mNormal.y;
}

//=============================================================================
// BPlane::calcZ
//=============================================================================
float BPlane::calcZ(float x, float y) const
{
   if (mNormal.z == 0.0f)
      return 0.0f;

   return -(mDistance + (x * mNormal.x) + (y * mNormal.y)) / mNormal.z;
}

//=============================================================================
// BPlane::getPoint
//=============================================================================
BVector BPlane::getPoint(const BVector2& pt2D) const
{
   BVector o,u,v;
   o = origin();

   if(mNormal.x < mNormal.y)
      if(mNormal.x < mNormal.z)
         v = cXAxisVector;
      else
         v = cZAxisVector;
   else
      if(mNormal.y < mNormal.z)
         v = cYAxisVector;
      else
         v = cZAxisVector;

   BVector temp(mNormal * (v.dot(mNormal)));
   u = v - temp;
   u.normalize();

   v = mNormal.cross(u);

   return (o + u * pt2D.x + v * pt2D.y);
}

//=============================================================================
// BPlane::distanceFromPoint
//=============================================================================
float BPlane::distanceFromPoint(const BVector& pt) const
{
   return pt.dot(mNormal) + mDistance;
}

//=============================================================================
// eof: plane.cpp
//=============================================================================
