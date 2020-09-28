//==============================================================================
// Copyright (c) 1997-2005 Ensemble Studios
//
// Vector class
//==============================================================================

#include "xcore.h"
#include "vector.h"
#include "vector.h"
#include "matrix.h"

//==============================================================================
#define USE_FANCY_UNTESTED_CODE 0 // SLB: I didn't have time to finish this or test it properly.

//==============================================================================
// BIntrinsicVector::distanceToLine
//
// Returns the distance from this point (BIntrinsicVector treated as a point) to the
// line through p in the direction of dir.
//==============================================================================
float BIntrinsicVector::distanceToLine(const BIntrinsicVector p, const BIntrinsicVector dir) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p);
   BVECTOR_TEST_ALIGNMENT_16(&dir);

#if USE_FANCY_UNTESTED_CODE
   XMVECTOR term = XMVector3Cross(XMVectorSubtract(*this, p), dir);
   XMVECTOR result = XMVectorMultiply(XMVectorSqrtEst(XMVector3Dot(term, term)), XMVectorReciprocalSqrtEst(XMVector3Dot(dir, dir)));
   return result.x;
#else
   return((float)sqrt(distanceToLineSqr(p, dir)));
#endif
}

//==============================================================================
// BIntrinsicVector::distanceToLineSqr
//
// Returns the distance from this point (BIntrinsicVector treated as a point) to the
// line through p in the direction of dir.
//==============================================================================
float BIntrinsicVector::distanceToLineSqr(const BIntrinsicVector p, const BIntrinsicVector dir) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p);
   BVECTOR_TEST_ALIGNMENT_16(&dir);

#if USE_FANCY_UNTESTED_CODE
   XMVECTOR term = XMVector3Cross(XMVectorSubtract(*this, p), dir);
   XMVECTOR result = XMVectorMultiply(XMVector3Dot(term, term), XMVectorReciprocalEst(XMVector3Dot(dir, dir)));
   return result.x;
#else
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
#endif
}


//==============================================================================
// BIntrinsicVector::distanceToPlane
//
// Returns the distance from this point to the plane defined by the given
// point and normal.
//==============================================================================
float BIntrinsicVector::distanceToPlane(const BIntrinsicVector pointOnPlane, const BIntrinsicVector planeNormal) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&pointOnPlane);
   BVECTOR_TEST_ALIGNMENT_16(&planeNormal);

#if USE_FANCY_UNTESTED_CODE
   XMVECTOR result = XMVectorAbs(XMVectorMultiply(XMVectorAdd(XMVector3Dot(planeNormal, *this), XMVector3Dot(planeNormal, pointOnPlane)), XMVectorReciprocalSqrtEst(XMVector3Dot(planeNormal, planeNormal))));
   return result.x;
#else
   return(fabs(signedDistanceToPlane(pointOnPlane, planeNormal)));
#endif
}


//==============================================================================
// BIntrinsicVector::signedDistanceToPlane
//
// Returns the signed distance from this point to the plane defined by the given
// point and normal.
//==============================================================================
float BIntrinsicVector::signedDistanceToPlane(const BIntrinsicVector pointOnPlane, const BIntrinsicVector planeNormal) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&pointOnPlane);
   BVECTOR_TEST_ALIGNMENT_16(&planeNormal);

#if USE_FANCY_UNTESTED_CODE
   XMVECTOR result = XMVectorMultiply(XMVectorAdd(XMVector3Dot(planeNormal, *this), XMVector3Dot(planeNormal, pointOnPlane)), XMVectorReciprocalSqrtEst(XMVector3Dot(planeNormal, planeNormal)));
   return result.x;
#else
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
#endif
}


//==============================================================================
// BIntrinsicVector::distanceToLineSegment
//==============================================================================
float BIntrinsicVector::distanceToLineSegment(const BIntrinsicVector p1, const BIntrinsicVector p2) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p1);
   BVECTOR_TEST_ALIGNMENT_16(&p2);

   float d;

   BIntrinsicVector p1p2=p2-p1;

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   BIntrinsicVector p1p=*this-p1;
   float dP=p1p2.dot(p1p);
   if(dP < 0.0f)
   {
      // Get distance from p1.
      d = distance(p1);
      return(d);
   }

   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   BIntrinsicVector p2p=*this-p2;
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
// BIntrinsicVector::distanceToLineSegmentSqr
//==============================================================================
float BIntrinsicVector::distanceToLineSegmentSqr(const BIntrinsicVector p1, const BIntrinsicVector p2) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p1);
   BVECTOR_TEST_ALIGNMENT_16(&p2);

   float d;

   BIntrinsicVector p1p2=p2-p1;

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   BIntrinsicVector p1p=*this-p1;
   float dP=p1p2.dot(p1p);
   if(dP < 0.0f)
   {
      // Get distance from p1.
      d = distanceSqr(p1);
      return(d);
   }

   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   BIntrinsicVector p2p=*this-p2;
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
// BIntrinsicVector::xzDistanceToLine
//==============================================================================
float BIntrinsicVector::xzDistanceToLine( const BIntrinsicVector p, const BIntrinsicVector dir, BIntrinsicVector *pvClosest ) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p);
   BVECTOR_TEST_ALIGNMENT_16(&dir);
   BVECTOR_TEST_ALIGNMENT_16(&pvClosest);

   return(float(sqrt(xzDistanceToLineSqr(p, dir, pvClosest))));
}


//==============================================================================
// BIntrinsicVector::xzDistanceToLineSqr
//==============================================================================
float BIntrinsicVector::xzDistanceToLineSqr( const BIntrinsicVector p, const BIntrinsicVector dir, BIntrinsicVector *pvClosest ) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p);
   BVECTOR_TEST_ALIGNMENT_16(&dir);
   BVECTOR_TEST_ALIGNMENT_16(&pvClosest);

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
// BIntrinsicVector::xzDistanceToLineSegment
//==============================================================================
float BIntrinsicVector::xzDistanceToLineSegment(const BIntrinsicVector p1, const BIntrinsicVector p2, BIntrinsicVector *pvClosest) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p1);
   BVECTOR_TEST_ALIGNMENT_16(&p2);
   BVECTOR_TEST_ALIGNMENT_16(&pvClosest);

   return((float)sqrt(xzDistanceToLineSegmentSqr(p1, p2, pvClosest)));
}


//==============================================================================
// BIntrinsicVector::xzDistanceToLineSegmentSqr
// Added ability to get the point on the line that provided the closest distance.
//==============================================================================
float BIntrinsicVector::xzDistanceToLineSegmentSqr(const BIntrinsicVector p1, const BIntrinsicVector p2, BIntrinsicVector *pvClosest) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p1);
   BVECTOR_TEST_ALIGNMENT_16(&p2);
   BVECTOR_TEST_ALIGNMENT_16(&pvClosest);

   float d;

   BIntrinsicVector p1p2;
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
// BIntrinsicVector::distanceToLineSegment2
//
// Returns the distance from this point to the line through p1 and p2.  If
// this point's intersection with the line falls within the endpoints of the
// line segment, onSegment is set to TRUE.
//==============================================================================
float BIntrinsicVector::distanceToLineSegment2(const BIntrinsicVector p1, const BIntrinsicVector p2, bool& onSegment) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p1);
   BVECTOR_TEST_ALIGNMENT_16(&p2);
   BVECTOR_TEST_ALIGNMENT_16(&onSegment);

   //This is a terrible way to do this.  We find the distance from this point
   //to the line.  Then we do a double dot product to see if the point is within
   //the segment's boundaries.

   BIntrinsicVector p1p2=p2-p1;
   float d=distanceToLine(p1, p1p2);

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   BIntrinsicVector p1p=*this-p1;
   float dP=p1p2.dot(p1p);
   if (dP < 0.0f)
   {
      onSegment=FALSE;
      return(d);
   }
   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   BIntrinsicVector p2p=*this-p2;
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
// BIntrinsicVector::angleBetweenVector
//
// Returns the radian angle of the smallest angle between this vector and the
// vector v.
//==============================================================================
float BIntrinsicVector::angleBetweenVector(const BIntrinsicVector v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   //XMVECTOR result = XMVector3AngleBetweenVectors(*this, v);
   //return result.x;

   // The code below is copied (and slightly modified) from XMVector3AngleBetweenVectors
   // in xmvector.inl.  In the _VMX128_INTRINSICS_ #else section below, the CosAngle
   // is clamped to -1.0..1.0 because of some rare calculation errors that would result
   // in values outside this range for equal or opposite vectors.  The acos function
   // doesn't like values outside -1.0..1.0, it will return qnan in those cases.

   XMVECTOR V1 = *this;
   XMVECTOR V2 = v;

#if defined(_NO_INTRINSICS_)

    XMVECTOR L1;
    XMVECTOR L2;
    XMVECTOR Dot;
    XMVECTOR CosAngle;
    XMVECTOR Result;

    L1 = XMVector3ReciprocalLength(V1);
    L2 = XMVector3ReciprocalLength(V2);

    Dot = XMVector3Dot(V1, V2);

    L1 = XMVectorMultiply(L1, L2);

    CosAngle = XMVectorMultiply(Dot, L1);

    Result = XMVectorACos(CosAngle);

    return Result.x;

#elif defined(_VMX32_INTRINSICS_)

    XMVECTOR L1;
    XMVECTOR L2;
    XMVECTOR Dot;
    XMVECTOR CosAngle;
    XMVECTOR L1A, L1B, L2A, L2B, DotA, DotB;
    XMVECTOR Rsq1, Rsq2, Rcp1, Rcp2, RT1, RT2;
    XMVECTOR Result;

    L1B = __vsel(*(XMVECTOR*)g_XMSelect1110, V1, *(XMVECTOR*)g_XMSelect1110);
    L2B = __vsel(*(XMVECTOR*)g_XMSelect1110, V2, *(XMVECTOR*)g_XMSelect1110);

    L1A = __vmaddfp(L1B, L1B, *(XMVECTOR*)g_XMNegativeZero);
    L2A = __vmaddfp(L2B, L2B, *(XMVECTOR*)g_XMNegativeZero);
    DotA = __vmaddfp(L1B, L2B, *(XMVECTOR*)g_XMNegativeZero);

    L1B = __vsldoi(L1A, L1A, 8);
    L2B = __vsldoi(L2A, L2A, 8);
    DotB = __vsldoi(DotA, DotA, 8);

    L1A = __vaddfp(L1A, L1B);
    L2A = __vaddfp(L2A, L2B);
    DotA = __vaddfp(DotA, DotB);

    L1B = __vsldoi(L1A, L1A, 4);
    L2B = __vsldoi(L2A, L2A, 4);
    DotB = __vsldoi(DotA, DotA, 4);

    L1A = __vaddfp(L1A, L1B);
    L2A = __vaddfp(L2A, L2B);
    Dot = __vaddfp(DotA, DotB);

    Rsq1 = __vrsqrtefp(L1A);
    Rsq2 = __vrsqrtefp(L2A);
    RT1 = __vmaddfp(L1A, g_XMOneHalf, *(XMVECTOR*)g_XMNegativeZero);
    RT2 = __vmaddfp(L2A, g_XMOneHalf, *(XMVECTOR*)g_XMNegativeZero);
    Rcp1 = __vmaddfp(Rsq1, Rsq1, *(XMVECTOR*)g_XMNegativeZero);
    Rcp2 = __vmaddfp(Rsq2, Rsq2, *(XMVECTOR*)g_XMNegativeZero);
    RT1 = __vnmsubfp(RT1, Rcp1, g_XMOneHalf);
    RT2 = __vnmsubfp(RT2, Rcp2, g_XMOneHalf);
    L1 = __vmaddfp(Rsq1, RT1, Rsq1);
    L2 = __vmaddfp(Rsq2, RT2, Rsq2);

    L1 = __vmaddfp(L1, L2, *(XMVECTOR*)g_XMNegativeZero);

    CosAngle = __vmaddfp(Dot, L1, *(XMVECTOR*)g_XMNegativeZero);

    Result = XMVectorACos(CosAngle);

    return Result.x;

#else // _VMX128_INTRINSICS_

    XMVECTOR D1, D2;
    XMVECTOR Rsq1, Rsq2;
    XMVECTOR Rcp1, Rcp2;
    XMVECTOR RT1, RT2;
    XMVECTOR H1, H2;
    XMVECTOR L;
    XMVECTOR Dot;
    XMVECTOR One, NegativeOne;
    XMVECTOR CosAngle, MaxedCosAngle, ClampedCosAngle;
    XMVECTOR Result;

    D1 = __vmsum3fp(V1, V1);
    D2 = __vmsum3fp(V2, V2);
    H2 = __vspltisw(1);
    H1 = __vcfsx(H2, 1);
    H2 = __vcfsx(H2, 1);
    Rsq1 = __vrsqrtefp(D1);
    Rsq2 = __vrsqrtefp(D2);
    RT1 = __vmulfp(D1, H1);
    RT2 = __vmulfp(D2, H2);
    Rcp1 = __vmulfp(Rsq1, Rsq1);
    Rcp2 = __vmulfp(Rsq2, Rsq2);
    H1 = __vnmsubfp(RT1, Rcp1, H1);
    H2 = __vnmsubfp(RT2, Rcp2, H2);
    Rsq1 = __vmaddfp(Rsq1, H1, Rsq1);
    Rsq2 = __vmaddfp(Rsq2, H2, Rsq2);
    Dot = __vmsum3fp(V1, V2);
    L = __vmulfp(Rsq1, Rsq2);
    CosAngle = __vmulfp(Dot, L);

    One = XMVectorSplatOne();
    NegativeOne = __vspltisw(-1);
    NegativeOne = __vcfsx(NegativeOne, 0);
    MaxedCosAngle = __vmaxfp(CosAngle, NegativeOne);
    ClampedCosAngle = __vminfp(MaxedCosAngle, One);

    Result = XMVectorACos(ClampedCosAngle);

    return Result.x;

#endif // _VMX128_INTRINSICS_
}


//==============================================================================
// BIntrinsicVector::rotateXY
//
// Rotates a vector by alpha radians along the XY plane.
//==============================================================================
void BIntrinsicVector::rotateXY( float theta )
{
   BVECTOR_TEST_ALIGNMENT_16(this);

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
   
   BIntrinsicVector temp;
   BIntrinsicVector vec;
   vec = *this;
   rotmat.transformVector(vec,temp);
   *this = temp;
}

//==============================================================================
// BIntrinsicVector::rotateRelativeXY
//
// like RotateXY but rotates around a fixed other point
//==============================================================================
void BIntrinsicVector::rotateRelativeXY( const BIntrinsicVector rotate, float theta )
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&rotate);

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
// BIntrinsicVector::rotateXZ
//
// Rotates a vector by alpha radians along the XZ plane.
//==============================================================================
void BIntrinsicVector::rotateXZ( float theta )
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   float sinval = float(sin(theta));
   float cosval = float(cos(theta));
   BMatrix rotmat;
   rotmat.makeRotateYSinCos(sinval,cosval);
   
   BIntrinsicVector temp;
   BIntrinsicVector vec;
   vec = *this;
   rotmat.transformVector(vec,temp);
   *this = temp;
}

//==============================================================================
// BIntrinsicVector::rotateRelativeXZ
//
// like RotateXZ but rotates around a fixed other point
//==============================================================================
void BIntrinsicVector::rotateRelativeXZ( const BIntrinsicVector rotate, float theta )
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&rotate);

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
// BIntrinsicVector::rotateYZ
//
// Rotates a vector by alpha radians along the YZ plane.
//==============================================================================
void BIntrinsicVector::rotateYZ( float theta )
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   float sinval = float(sin(theta));
   float cosval = float(cos(theta));
   BMatrix rotmat;
   rotmat.makeRotateXSinCos(sinval,cosval);

   BIntrinsicVector temp;
   BIntrinsicVector vec;
   vec = *this;
   rotmat.transformVector(vec,temp);
   *this = temp;
}

//==============================================================================
// BIntrinsicVector::rotateRelativeYZ
//
// like RotateYZ but rotates around a fixed other point
//==============================================================================
void BIntrinsicVector::rotateRelativeYZ( const BIntrinsicVector rotate, float theta )
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&rotate);

   // figure out our offset from the rotation point.
   x -= rotate.x;
   y -= rotate.y;
   z -= rotate.z;

   // rotate that
   rotateYZ(theta);

   // now apply it to our main source
   x += rotate.x;
   y += rotate.y;
   z += rotate.z;
}

//==============================================================================
// BIntrinsicVector::projectOntoPlane
//==============================================================================
void BIntrinsicVector::projectOntoPlane(const BIntrinsicVector planeNormal, const BIntrinsicVector planePoint, BIntrinsicVector &projectedPoint)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&planeNormal);
   BVECTOR_TEST_ALIGNMENT_16(&planePoint);
   BVECTOR_TEST_ALIGNMENT_16(&projectedPoint);

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
// BIntrinsicVector::projectOntoPlane
//==============================================================================
void BIntrinsicVector::projectOntoPlane(const BIntrinsicVector planeNormal, const BIntrinsicVector planePoint, 
   const BIntrinsicVector projectionDir, BIntrinsicVector &projectedPoint)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&planeNormal);
   BVECTOR_TEST_ALIGNMENT_16(&planePoint);
   BVECTOR_TEST_ALIGNMENT_16(&projectionDir);
   BVECTOR_TEST_ALIGNMENT_16(&projectedPoint);

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
// BIntrinsicVector::projectYPlane
//==============================================================================
bool BIntrinsicVector::projectYPlane(const BIntrinsicVector dir, BIntrinsicVector &iPoint, float yplane) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&dir);
   BVECTOR_TEST_ALIGNMENT_16(&iPoint);

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
// BIntrinsicVector::projectOntoPlaneAsVector
// jce [3/12/2004] -- Assumes normalized plane normal. 
//==============================================================================
void BIntrinsicVector::projectOntoPlaneAsVector(const BIntrinsicVector planeNormal, BIntrinsicVector &projectedVector)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&planeNormal);
   BVECTOR_TEST_ALIGNMENT_16(&projectedVector);

   BIntrinsicVector temp = this->cross(planeNormal);
   projectedVector = planeNormal.cross(temp);
}


//=============================================================================
// BIntrinsicVector::randomPerpendicular
//=============================================================================
void BIntrinsicVector::randomPerpendicular(BIntrinsicVector &result)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&result);

   // Get some arbitrary vector that is not parallel to this one.
   // jce 4/6/2001 -- is this insane?
   BIntrinsicVector temp;
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
// BIntrinsicVector::clamp
//=============================================================================
long BIntrinsicVector::xzEqualTo(const BIntrinsicVector v) const
{ 
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return (fabs(x - v.x) < cFloatCompareEpsilon && fabs(z - v.z) < cFloatCompareEpsilon); 
}

// ===========================================================================
// BIntrinsicVector::rotateAroundPoint
// ===========================================================================
void BIntrinsicVector::rotateAroundPoint(const BIntrinsicVector c, float xRads, float yRads, float zRads)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&c);

   // Xemu [2/5/2004] -- this could be done without a temp, if we wind up doing it anywhere perf sensitive
   BIntrinsicVector temp;
   temp = *this;
   temp -= c;
   BMatrix rotmat;
   rotmat.makeRotateYawPitchRoll(xRads, yRads, zRads);
   rotmat.transformVector(temp, *this);
   *this += c;
}


// ===========================================================================
// BIntrinsicVector::almostEqual
// ===========================================================================
bool BIntrinsicVector::almostEqual(const BIntrinsicVector v, float tolerance) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   if(Math::EqualTol(x, v.x, tolerance) &&
      Math::EqualTol(y, v.y, tolerance) &&
      Math::EqualTol(z, v.z, tolerance))
      return true;
   else
      return false;
}


// ===========================================================================
// BIntrinsicVector::almostEqualXZ
// ===========================================================================
bool BIntrinsicVector::almostEqualXZ(const BIntrinsicVector v, float tolerance) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   if(Math::EqualTol(x, v.x, tolerance) &&
      Math::EqualTol(z, v.z, tolerance))
      return true;
   else
      return false;
}


//==============================================================================
// BIntrinsicVector::getAngleAroundY
//==============================================================================
float BIntrinsicVector::getAngleAroundY(void) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   // jce [6/10/2005] -- this function was cut-and-paste around the code in various places
   // so I consolidated it here. 

   BIntrinsicVector vector;
   vector.x = x;
   vector.y = 0.0f;
   vector.z = z;
   vector.normalize();
   
   float angle;
   if (vector.x > cFloatCompareEpsilon)
   {
      if (vector.z > 1.0f)
         angle = 0.0f;
      else
         angle = (float)acos(vector.z);
   }
   else if (vector.x < -cFloatCompareEpsilon)
   {
      float ac;
      if (vector.z > 1.0f)
         ac = 0.0f;
      else
         ac = (float)acos(vector.z);
      angle = cPi + (cPi - ac);
   }
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
// BIntrinsicVector::getAngleAroundX
//==============================================================================
float BIntrinsicVector::getAngleAroundX(void) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   // jce [6/10/2005] -- this function was cut-and-paste around the code in various places
   // so I consolidated it here. 

   BIntrinsicVector vector;
   vector.x = 0.0f;
   vector.y = y;
   vector.z = z;
   vector.normalize();

   float angle;
   if (vector.y > cFloatCompareEpsilon)
   {
      if (vector.z > 1.0f)
         angle = 0.0f;
      else
         angle = (float)acos(vector.z);
   }
   else if (vector.y < -cFloatCompareEpsilon)
   {
      float ac;
      if (vector.z > 1.0f)
         ac = 0.0f;
      else
         ac = (float)acos(vector.z);
      angle = cPi + (cPi - ac);
   }
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
// BIntrinsicVector::getAngleAroundZ
//==============================================================================
float BIntrinsicVector::getAngleAroundZ(void) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   // jce [6/10/2005] -- this function was cut-and-paste around the code in various places
   // so I consolidated it here. 

   BIntrinsicVector vector;
   vector.x = x;
   vector.y = y;
   vector.z = 0.0f;
   vector.normalize();

   float angle;
   if (vector.x > cFloatCompareEpsilon)
   {
      if (vector.y > 1.0f)
         angle = 0.0f;
      else
         angle = (float)acos(vector.y);
   }
   else if (vector.x < -cFloatCompareEpsilon)
   {
      float ac;
      if (vector.y > 1.0f)
         ac = 0.0f;
      else
         ac = (float)acos(vector.y);
      angle = cPi + (cPi - ac);
   }
   else
   {
      if (vector.y < 0.0f)
         angle = cPi;
      else
         angle = 0.0f;
   }
   return angle;
}

//==============================================================================
// eof: intrinsicvector.cpp
//==============================================================================
