//==============================================================================
// Copyright (c) 1997-2005 Ensemble Studios
//
// Vector class
//==============================================================================

#include "xcore.h"
#include "math\vector.h"
#include "math\matrix.h"

//==============================================================================
static BSimpleShortVector stemp;

#if defined(XBOX)
BSimpleVector::BSimpleVector(BIntrinsicVector v)
{
   set(v.x, v.y, v.z);
}

BSimpleVector &BSimpleVector::operator=(BIntrinsicVector v)
{
   set(v.x, v.y, v.z);
   return *this;
}
#endif

//==============================================================================
// BSimpleVector::BSimpleVector
//==============================================================================
BSimpleVector::BSimpleVector(const BSimpleShortVector &v)
{
   x = v.mx.asFloat();
   y = v.my.asFloat();
   z = v.mz.asFloat();
}

//==============================================================================
// BSimpleVector::distanceToLine
//
// Returns the distance from this point (BSimpleVector treated as a point) to the
// line through p in the direction of dir.
//==============================================================================
float BSimpleVector::distanceToLine(const BSimpleVector &p, const BSimpleVector &dir) const
{
   return((float)sqrt(distanceToLineSqr(p, dir)));
}


//==============================================================================
// BSimpleVector::distanceToLineSqr
//
// Returns the distance from this point (BSimpleVector treated as a point) to the
// line through p in the direction of dir.
//==============================================================================
float BSimpleVector::distanceToLineSqr(const BSimpleVector &p, const BSimpleVector &dir) const
{
   // jce 11/29/99 -- does this really work???

   float a = dir.x;
   float b = dir.y; 
   float c = dir.z;
   float dx = x-p.x;
   float dy = y-p.y;
   float dz = z-p.z;

   float term1 = c*dy-b*dz;
   float term2 = a*dz-c*dx;
   float term3 = b*dx-a*dy;

   return((term1*term1 + term2*term2 + term3*term3)/(a*a+b*b+c*c));
}


//==============================================================================
// BSimpleVector::distanceToPlane
//
// Returns the distance from this point to the plane defined by the given
// point and normal.
//==============================================================================
float BSimpleVector::distanceToPlane(const BSimpleVector& pointOnPlane, const BSimpleVector& planeNormal) const
{
   return(fabs(signedDistanceToPlane(pointOnPlane, planeNormal)));
}


//==============================================================================
// BSimpleVector::signedDistanceToPlane
//
// Returns the signed distance from this point to the plane defined by the given
// point and normal.
//==============================================================================
float BSimpleVector::signedDistanceToPlane(const BSimpleVector& pointOnPlane, const BSimpleVector& planeNormal) const
{
   //Formula: (A*x + B*y + C*z + D)/sqrt(A^2 + B^2 + C^2)
   //Where A, B, and C are the values of the plane normal components and (x, y, z) is
   //the point in question.

   //Calc the D component of the plane equation: Ax+By+Cz+D=0
   float D=-(planeNormal.x*pointOnPlane.x + planeNormal.y*pointOnPlane.y + planeNormal.z*pointOnPlane.z);
   //Make sure the denominator is valid.
   float den=(float)sqrt(planeNormal.x*planeNormal.x + planeNormal.y*planeNormal.y + planeNormal.z*planeNormal.z);
   if (fabs(den) < cFloatCompareEpsilon)
      return(cMaximumFloat);
   //Calc the numerator and return the overall result.
   float num=planeNormal.x*x + planeNormal.y*y + planeNormal.z*z + D;
   return(num/den);
}


//==============================================================================
// BSimpleVector::distanceToLineSegment
//==============================================================================
float BSimpleVector::distanceToLineSegment(const BSimpleVector &p1, const BSimpleVector &p2) const
{
   float d;

   BSimpleVector p1p2=p2-p1;

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   BSimpleVector p1p=*this-p1;
   float dP=p1p2.dot(p1p);
   if(dP < 0.0f)
   {
      // Get distance from p1.
      d = distance(p1);
      return(d);
   }

   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   BSimpleVector p2p=*this-p2;
   p1p2=-p1p2;
   dP=p1p2.dot(p2p);
   if(dP < 0.0f)
   {
      // Get distance from p2.
      d = distance(p2);
      return(d);
   }

   // Get distance to line.
   d = distanceToLine(p1, p1p2);
   return(d);
}




//==============================================================================
// BSimpleVector::distanceToLineSegmentSqr
//==============================================================================
float BSimpleVector::distanceToLineSegmentSqr(const BSimpleVector &p1, const BSimpleVector &p2) const
{
   float d;

   BSimpleVector p1p2=p2-p1;

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   BSimpleVector p1p=*this-p1;
   float dP=p1p2.dot(p1p);
   if(dP < 0.0f)
   {
      // Get distance from p1.
      d = distanceSqr(p1);
      return(d);
   }

   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   BSimpleVector p2p=*this-p2;
   p1p2=-p1p2;
   dP=p1p2.dot(p2p);
   if(dP < 0.0f)
   {
      // Get distance from p2.
      d = distanceSqr(p2);
      return(d);
   }

   // Get distance to line.
   d = distanceToLineSqr(p1, p1p2);
   return(d);
}


//==============================================================================
// BSimpleVector::xzDistanceToLine
//==============================================================================
float BSimpleVector::xzDistanceToLine( const BSimpleVector& p, const BSimpleVector& dir, BSimpleVector *pvClosest ) const
{
   return(float(sqrt(xzDistanceToLineSqr(p, dir, pvClosest))));
}


//==============================================================================
// BSimpleVector::xzDistanceToLineSqr
//==============================================================================
float BSimpleVector::xzDistanceToLineSqr( const BSimpleVector& p, const BSimpleVector& dir, BSimpleVector *pvClosest ) const
{
   // init return value if it exists.
   if (pvClosest)
   {   
      pvClosest->set(0.0f, 0.0f, 0.0f);
   }

   // Get vector length.
   float lenSqr = dir.x*dir.x + dir.z*dir.z;

   // If 0, just return distance to the point.
   if(lenSqr < cFloatCompareEpsilon)
      return(xzDistanceSqr(p));

   // Get parameter.
   float t = ((x-p.x)*dir.x + (z-p.z)*dir.z)/lenSqr;

   float px = p.x+t*dir.x;
   float pz = p.z+t*dir.z;
   if (pvClosest)
   {
      pvClosest->x = px;
      pvClosest->z = pz;
   }

   float dx = x-px;
   float dz = z-pz;

   return(dx*dx + dz*dz);
}


//==============================================================================
// BSimpleVector::xzDistanceToLineSegment
//==============================================================================
float BSimpleVector::xzDistanceToLineSegment(const BSimpleVector &p1, const BSimpleVector &p2, BSimpleVector *pvClosest) const
{
   return((float)sqrt(xzDistanceToLineSegmentSqr(p1, p2, pvClosest)));
}


//==============================================================================
// BSimpleVector::xzDistanceToLineSegmentSqr
// Added ability to get the point on the line that provided the closest distance.
//==============================================================================
float BSimpleVector::xzDistanceToLineSegmentSqr(const BSimpleVector &p1, const BSimpleVector &p2, BSimpleVector *pvClosest) const
{
   float d;

   BSimpleVector p1p2;
   p1p2.assignDifference(p2, p1);

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   float p1px = x-p1.x;
   float p1pz = z-p1.z;
   float dP=p1p2.x*p1px + p1p2.z*p1pz;
   if(dP < 0.0f)
   {
      // Get distance from p1.
      d = xzDistanceSqr(p1);
      if (pvClosest)
      {
         pvClosest->x = p1.x;
         pvClosest->z = p1.z;
      }
      return(d);
   }

   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   float pp2x = p2.x-x;
   float pp2z = p2.z-z;
   dP=p1p2.x*pp2x + p1p2.z*pp2z;
   if(dP < 0.0f)
   {
      // Get distance from p2.
      d = xzDistanceSqr(p2);
      if (pvClosest)
      {
         pvClosest->x = p2.x;
         pvClosest->z = p2.z;
      }
      return(d);
   }

   return(xzDistanceToLineSqr(p1, p1p2, pvClosest));
}


//==============================================================================
// BSimpleVector::distanceToLineSegment2
//
// Returns the distance from this point to the line through p1 and p2.  If
// this point's intersection with the line falls within the endpoints of the
// line segment, onSegment is set to TRUE.
//==============================================================================
float BSimpleVector::distanceToLineSegment2(const BSimpleVector& p1, const BSimpleVector& p2, bool& onSegment) const
{
   //This is a terrible way to do this.  We find the distance from this point
   //to the line.  Then we do a double dot product to see if the point is within
   //the segment's boundaries.

   BSimpleVector p1p2=p2-p1;
   float d=distanceToLine(p1, p1p2);

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   BSimpleVector p1p=*this-p1;
   float dP=p1p2.dot(p1p);
   if (dP < 0.0f)
   {
      onSegment=FALSE;
      return(d);
   }
   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   BSimpleVector p2p=*this-p2;
   p1p2=-p1p2;
   dP=p1p2.dot(p2p);
   if (dP < 0.0f)
   {
      onSegment=FALSE;
      return(d);
   }

   onSegment=TRUE;
   return(d);
}


//==============================================================================
// BSimpleVector::distance
//==============================================================================
float BSimpleVector::distance(const BSimpleVector &point) const
{
   float dx = x-point.x;
   float dy = y-point.y;
   float dz = z-point.z;

   return((float)sqrt(dx*dx + dy*dy + dz*dz));
}


//==============================================================================
// BSimpleVector::distanceSqr
//==============================================================================
float BSimpleVector::distanceSqr(const BSimpleVector &point) const
{
   float dx = x-point.x;
   float dy = y-point.y;
   float dz = z-point.z;

   return(dx*dx + dy*dy + dz*dz);
}


//==============================================================================
// BSimpleVector::xzDistance
//==============================================================================
float BSimpleVector::xzDistance(const BSimpleVector &point) const
{
   float dx = x-point.x;
   float dz = z-point.z;

   return((float)sqrt(dx*dx + dz*dz));
}


//==============================================================================
// BSimpleVector::xzDistanceSqr
//==============================================================================
float BSimpleVector::xzDistanceSqr(const BSimpleVector &point) const
{
   float dx = x-point.x;
   float dz = z-point.z;

   return(dx*dx + dz*dz);
}


//==============================================================================
// BSimpleVector::xyDistance
//==============================================================================
float BSimpleVector::xyDistance(const BSimpleVector &point) const
{
   float dx = x-point.x;
   float dy = y-point.y;

   return((float)sqrt(dx*dx + dy*dy));
}


//==============================================================================
// BSimpleVector::xyDistanceSqr
//==============================================================================
float BSimpleVector::xyDistanceSqr(const BSimpleVector &point) const
{
   float dx = x-point.x;
   float dy = y-point.y;

   return(dx*dx + dy*dy);
}


//==============================================================================
// BSimpleVector::angleBetweenVector
//
// Returns the radian angle of the smallest angle between this vector and the
// vector v.
//==============================================================================
float BSimpleVector::angleBetweenVector(const BSimpleVector& v) const
{
   float cosAngle=x*v.x + y*v.y + z*v.z;
   if (fabs(cosAngle) < cFloatCompareEpsilon)
      return(cPiOver2);
   else if (cosAngle >= 1.0f)
      return(0.0f);
   else if (cosAngle <= -1.0f)
      return(cPi);
   return((float)acos(cosAngle));
}


//==============================================================================
// BSimpleVector::rotateXY
//
// Rotates a vector by alpha radians along the XY plane.
//==============================================================================
void BSimpleVector::rotateXY( float theta )
{
   /*
   // maybe faster to do this with a rotation matrix and then strip out the y change?
   float alpha; // current angle
   alpha = circleTan(x,y);

   float beta; // our post-rotation angle;
   beta = theta + alpha;

   // make sure we are in the range 0 to 2pi
   // this assumes we are no more than 2pi out of range
   if (beta < 0)
   beta = beta + TWO_PI;
   else if (beta > TWO_PI)
   beta = beta - TWO_PI;

   float mag = (float)sqrt(x*x + y*y);
   // given our new rotation angle, just project out
   x = lookupCos(beta) * mag;
   y = lookupSin(beta) * -1 * mag;
   */

   BMatrix rotmat;
   float sinval, cosval;
   // these are faster, but cause "jumps" in the sin/cos lookup
   // if we get better precision or deal with "rounding" the value
   // better then we may want to go back to it.
   //sinval = lookupSin(theta);
   //cosval = lookupCos(theta);
   sinval = float(sin(theta));
   cosval = float(cos(theta));
   //trace("rotateXY: theta %f, sinval = %f, cosval = %f",theta,sinval,cosval);
   rotmat.makeRotateZSinCos(sinval,cosval);

   BVector temp;
   BVector vec;
   vec = *this;
   rotmat.transformVector(vec,temp);
   *this = temp;
}

//==============================================================================
// BSimpleVector::rotateRelativeXY
//
// like RotateXY but rotates around a fixed other point
//==============================================================================
void BSimpleVector::rotateRelativeXY( const BSimpleVector &rotate, float theta )
{
   // figure out our offset from the rotation point.
   x -= rotate.x;
   y -= rotate.y;
   z -= rotate.z;

   // rotate that
   rotateXY(theta);

   // now apply it to our main source
   x += rotate.x;
   y += rotate.y;
   z += rotate.z;
}

//==============================================================================
// BSimpleVector::rotateXZ
//
// Rotates a vector by alpha radians along the XZ plane.
//==============================================================================
void BSimpleVector::rotateXZ( float theta )
{
   float sinval = float(sin(theta));
   float cosval = float(cos(theta));
   BMatrix rotmat;
   rotmat.makeRotateYSinCos(sinval,cosval);

   BVector temp;
   BVector vec;
   vec = *this;
   rotmat.transformVector(vec,temp);
   *this = temp;
}

//==============================================================================
// BSimpleVector::rotateRelativeXZ
//
// like RotateXZ but rotates around a fixed other point
//==============================================================================
void BSimpleVector::rotateRelativeXZ( const BSimpleVector &rotate, float theta )
{
   // figure out our offset from the rotation point.
   x -= rotate.x;
   y -= rotate.y;
   z -= rotate.z;

   // rotate that
   rotateXZ(theta);

   // now apply it to our main source
   x += rotate.x;
   y += rotate.y;
   z += rotate.z;
}

//==============================================================================
// BSimpleVector::projectOntoPlane
//==============================================================================
void BSimpleVector::projectOntoPlane(const BSimpleVector &planeNormal, const BSimpleVector &planePoint, BSimpleVector &projectedPoint)
{
   // Calculate d parameter of plane equation.
   float d = -(planePoint.dot(planeNormal));

   // Get the denominator.  
   float den = planeNormal.dot(planeNormal);

   // Get the numerator.
   float num = d + (planeNormal.dot(*this));

   // Calculate t.
   float t = -num/den;

   // Get the actual point.
   projectedPoint = *this + planeNormal*t;
}


//==============================================================================
// BSimpleVector::projectOntoPlane
//==============================================================================
void BSimpleVector::projectOntoPlane(const BSimpleVector &planeNormal, const BSimpleVector &planePoint, 
                               const BSimpleVector &projectionDir, BSimpleVector &projectedPoint)
{
   // Calculate d parameter of polygon's plane equation.
   float d = -(planePoint.dot(planeNormal));

   // Get the denominator.  
   float den = planeNormal.dot(projectionDir);

   // If the denominator is 0, then the line is parallel to the plane so
   // there is no intersection.
   if(fabs(den) < cFloatCompareEpsilon)
   {
      projectedPoint.set(0.0f, 0.0f, 0.0f);
      return;
   }

   // Get the numerator.
   float num = d + (planeNormal.dot(*this));

   // Calculate t.
   float t = -num/den;

   // Now get the point.
   projectedPoint=*this+projectionDir*t;
}


//==============================================================================
// BSimpleVector::projectYPlane
//==============================================================================
bool BSimpleVector::projectYPlane(const BSimpleVector &dir, BSimpleVector &iPoint, float yplane) const
{
   float t;
   if (dir.y == 0)
      t = 999999999.0f; // maybe we should just error out?
   else
      t = (yplane - y) / dir.y;

   iPoint.x = x + (t * dir.x);
   iPoint.y = yplane;
   //BASSERT(fabs(iPoint.y - (y + (t * dir.y))) < cFloatCompareEpsilon);
   iPoint.z = z + (t * dir.z);

   return(t >= 0);
}


//==============================================================================
// BSimpleVector::projectOntoPlaneAsVector
// jce [3/12/2004] -- Assumes normalized plane normal. 
//==============================================================================
void BSimpleVector::projectOntoPlaneAsVector(const BSimpleVector &planeNormal, BSimpleVector &projectedVector)
{
   BSimpleVector temp = this->cross(planeNormal);
   projectedVector = planeNormal.cross(temp);
}


//=============================================================================
// BSimpleVector::randomPerpendicular
//=============================================================================
void BSimpleVector::randomPerpendicular(BSimpleVector &result)
{
   // Get some arbitrary vector that is not parallel to this one.
   // jce 4/6/2001 -- is this insane?
   BSimpleVector temp;
   if(fabs(x)>cFloatCompareEpsilon)
   {
      temp.x=1.0f;
      temp.y=1.0f;
      temp.z=-z/x;
   }
   else if(fabs(y)>cFloatCompareEpsilon)
   {
      temp.x=1.0f;
      temp.y=1.0f;
      temp.z=-z/y;
   }
   else if(fabs(z)>cFloatCompareEpsilon)
   {
      temp.x=1.0f;
      temp.y=-y/z;
      temp.z=1.0f;
   }
   else
   {
      result.set(0.0f, 0.0f, 0.0f);
      return;
   }

   // Cross product to get perp vector.
   result=temp.cross(*this);

   BASSERT(temp.length()>cFloatCompareEpsilon);

   // Normalize.
   result.normalize();
}


//=============================================================================
// BSimpleVector::clamp
//=============================================================================
void BSimpleVector::clamp(const BSimpleVector &minVec, const BSimpleVector &maxVec)
{
   if (x < minVec.x)
      x = minVec.x;
   if (y < minVec.y)
      y = minVec.y;
   if (z < minVec.z)
      z = minVec.z;
   if (x > maxVec.x)
      x = maxVec.x;
   if (y > maxVec.y)
      y = maxVec.y;
   if (z > maxVec.z)
      z = maxVec.z;
}

//=============================================================================
// 
//=============================================================================
void BSimpleVector::scale(const BSimpleVector &v)
{
   x *= v.x;
   y *= v.y;
   z *= v.z;
}

//=============================================================================
// BSimpleVector::clamp
//=============================================================================
long BSimpleVector::xzEqualTo(const BSimpleVector &v) const
{ 
   return (fabs(x - v.x) < cFloatCompareEpsilon && fabs(z - v.z) < cFloatCompareEpsilon); 
}

//==============================================================================
// BSimpleVector::operator=
//==============================================================================
BSimpleVector &BSimpleVector::operator=(const BSimpleShortVector &sv)
{
   x = sv.mx.asFloat();
   y = sv.my.asFloat();
   z = sv.mz.asFloat();

   return *this;
}

// ===========================================================================
// BSimpleVector::rotateAroundPoint
// ===========================================================================
void BSimpleVector::rotateAroundPoint(const BSimpleVector &c, float xRads, float yRads, float zRads)
{
   // Xemu [2/5/2004] -- this could be done without a temp, if we wind up doing it anywhere perf sensitive
   BVector temp;
   temp = *this;
   temp -= c;
   BMatrix rotmat;
   rotmat.makeRotateYawPitchRoll(xRads, yRads, zRads);
   rotmat.transformVector(temp, BVector(*this));
   *this += c;
}


// ===========================================================================
// BSimpleVector::almostEqual
// ===========================================================================
bool BSimpleVector::almostEqual(const BSimpleVector &v, float tolerance) const
{
   if(Math::EqualTol(x, v.x, tolerance) &&
      Math::EqualTol(y, v.y, tolerance) &&
      Math::EqualTol(z, v.z, tolerance))
      return true;
   else
      return false;
}


// ===========================================================================
// BSimpleVector::almostEqualXZ
// ===========================================================================
bool BSimpleVector::almostEqualXZ(const BSimpleVector &v, float tolerance) const
{
   if(Math::EqualTol(x, v.x, tolerance) &&
      Math::EqualTol(z, v.z, tolerance))
      return true;
   else
      return false;
}


//==============================================================================
// BSimpleVector::getAngleAroundY
//==============================================================================
float BSimpleVector::getAngleAroundY(void) const
{
   // jce [6/10/2005] -- this function was cut-and-paste around the code in various places
   // so I consolidated it here. 

   BSimpleVector vector;
   vector.x = x;
   vector.y = 0.0f;
   vector.z = z;
   vector.normalize();

   float angle;
   if (vector.x > cFloatCompareEpsilon)
      angle = (float)acos(vector.z);
   else if (vector.x < -cFloatCompareEpsilon)
      angle = cPi + (cPi - (float)acos(vector.z));
   else
   {
      if (vector.z < 0.0f)
         angle = cPi;
      else
         angle = 0.0f;
   }
   return angle;
}





//==============================================================================
// BSimpleShortVector::BSimpleShortVector
//==============================================================================
BSimpleShortVector::BSimpleShortVector(const BSimpleVector &v)
{
   mx = v.x;
   my = v.y;
   mz = v.z;

} // BSimpleShortVector::BSimpleShortVector

//==============================================================================
// BSimpleShortVector::BSimpleShortVector
//==============================================================================
BSimpleShortVector::BSimpleShortVector(float x, float y, float z)
{
   mx = x;
   my = y;
   mz = z;

} // BSimpleShortVector::BSimpleShortVector

//==============================================================================
// BSimpleShortVector::operator=
//==============================================================================
BSimpleShortVector &BSimpleShortVector::operator=(const BSimpleVector &v)
{
   mx = v.x;
   my = v.y;
   mz = v.z;

   return *this;

} // BSimpleShortVector::operator=

//==============================================================================
// BSimpleShortVector::operator-
//==============================================================================
BSimpleShortVector BSimpleShortVector::operator-(const BSimpleVector &v) const
{
   stemp.mx = mx.asFloat() - v.x;
   stemp.my = my.asFloat() - v.y;
   stemp.mz = mz.asFloat() - v.z;
   return stemp;
}

//==============================================================================
// BSimpleShortVector::operator*
//==============================================================================
BSimpleShortVector BSimpleShortVector::operator*(float a) const
{
   stemp.mx = mx.asFloat() * a;
   stemp.my = my.asFloat() * a;
   stemp.mz = mz.asFloat() * a;
   return stemp;
}

//==============================================================================
// BSimpleShortVector::operator
//==============================================================================
BSimpleShortVector &BSimpleShortVector::operator*=(const float a)
{
   mx *= a;
   my *= a;
   mz *= a;
   return *this;
}

//==============================================================================
// BSimpleShortVector::length
//==============================================================================
float BSimpleShortVector::length()
{
   return (float)sqrt(mx.asFloat()*mx.asFloat() + my.asFloat()*my.asFloat()+ mz.asFloat()*mz.asFloat());
}

//==============================================================================
// BSimpleShortVector::normalize
//==============================================================================
void BSimpleShortVector::normalize() 
{
   float len=length(); 

   mx /= len; 
   my /= len; 
   mz /= len;
}

//==============================================================================
// BSimpleShortVector::safeNormalize
//==============================================================================
bool BSimpleShortVector::safeNormalize(void) 
{
   float len=length(); 

   if(len>cFloatCompareEpsilon) 
   {
      mx/=len; 
      my/=len; 
      mz/=len; 
      return(true);
   } 

   return(false);
}


//==============================================================================
// BSimpleShortVector::operator-=
//==============================================================================
BSimpleShortVector &BSimpleShortVector::operator-=(const BSimpleVector &v)
{
   mx -= v.x;
   my -= v.y;
   mz -= v.z;

   return *this;
}

//==============================================================================
// BSimpleShortVector::operator+=
//==============================================================================
BSimpleShortVector &BSimpleShortVector::operator+=(const BSimpleVector &v)
{
   mx += v.x;
   my += v.y;
   mz += v.z;

   return *this;
}

//==============================================================================
// BSimpleShortVector::operator
//==============================================================================
BSimpleShortVector BSimpleShortVector::operator*(const BSimpleVector &v) const
{
   stemp.mx = mx.asFloat()*v.z - mx.asFloat()*v.y;
   stemp.my = my.asFloat()*v.x - my.asFloat()*v.z;
   stemp.mz = mz.asFloat()*v.y - mz.asFloat()*v.x;

   return stemp;
}

//==============================================================================
// BSimpleShortVector::operator
//==============================================================================
BSimpleShortVector BSimpleShortVector::operator-(const BSimpleShortVector &v) const
{
   stemp.mx = mx - v.mx;
   stemp.my = my - v.my;
   stemp.mz = mz - v.mz;

   return stemp;
}

//==============================================================================
// BSimpleShortVector::operator
//==============================================================================
BSimpleShortVector BSimpleShortVector::operator+(const BSimpleShortVector &v) const
{
   stemp.mx = mx + v.mx;
   stemp.my = my + v.my;
   stemp.mz = mz + v.mz;

   return stemp;
}

//==============================================================================
// BSimpleShortVector::operator
//==============================================================================
BSimpleShortVector BSimpleShortVector::operator*(const BSimpleShortVector &v) const
{
   stemp.mx = my.asFloat()*v.mz.asFloat() - mz.asFloat()*v.my.asFloat();
   stemp.my = mz.asFloat()*v.mx.asFloat() - mx.asFloat()*v.mz.asFloat();
   stemp.mz = mx.asFloat()*v.my.asFloat() - my.asFloat()*v.mx.asFloat();

   return stemp;
}



//==============================================================================
// eof: simplevector.cpp
//==============================================================================
