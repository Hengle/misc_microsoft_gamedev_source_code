//==============================================================================
// Copyright (c) 1997-2005 Ensemble Studios
//
// Vector class
//==============================================================================

#ifndef _INTRINSIC_VECTOR_H_
#define _INTRINSIC_VECTOR_H_

//==============================================================================
//
//==============================================================================
#define BVECTOR_TEST_ALIGNMENT_FLAG   0

#if BVECTOR_TEST_ALIGNMENT_FLAG
   #define BVECTOR_TEST_ALIGNMENT_16(address)  BASSERTM(~(unsigned int(address) & 0x000F), "BIntrinsicVector not 16 byte aligned!")
#else
   #define BVECTOR_TEST_ALIGNMENT_16(address)
#endif

class BSimpleVector;
class BIntrinsicShortVector;

//==============================================================================
// BIntrinsicVector Class
//
// WARNING: This class can never have ANY virtual functions.  Don't add any
// member variables to it either.
//==============================================================================
__declspec(intrin_type) _DECLSPEC_ALIGN_16_  class BIntrinsicVector : public XMVECTOR
{
   public:

      // Constructors
      XMFINLINE BIntrinsicVector(void);
      XMFINLINE BIntrinsicVector(const XMVECTOR v);
      XMFINLINE BIntrinsicVector(const float v);
      XMFINLINE BIntrinsicVector(const float nx, const float ny, const float nz);
      XMFINLINE BIntrinsicVector(const BIntrinsicShortVector &sv);
      XMFINLINE BIntrinsicVector(const BSimpleVector &v);
      XMFINLINE BIntrinsicVector(const XMDHEN3 *v);

      // Assignment
      XMFINLINE BIntrinsicVector operator=(const XMVECTOR v);
      XMFINLINE BIntrinsicVector operator=(const BIntrinsicShortVector &sv);
      XMFINLINE BIntrinsicVector operator=(const BSimpleVector &v);
      XMFINLINE void set(const float nx, const float ny, const float nz);
      XMFINLINE void set(const BIntrinsicVector v);
      XMFINLINE void zero(void);

      // Packing/Unpacking
         // These are designed to pack/unpack euler angles in radians
      XMFINLINE void pack(XMDHEN3 *output) const;
      XMFINLINE void unpack(const XMDHEN3 *input);

      // Comparison
      XMFINLINE long operator==(const BIntrinsicVector v) const;
      XMFINLINE long operator!=(const BIntrinsicVector v) const;

      // Scaling with floats
      XMFINLINE friend BIntrinsicVector operator*(const float a, const BIntrinsicVector v);
      XMFINLINE friend BIntrinsicVector operator/(const float a, const BIntrinsicVector v);
      XMFINLINE BIntrinsicVector operator*(const float a) const;
      XMFINLINE BIntrinsicVector operator/(const float a) const;
      XMFINLINE BIntrinsicVector operator*=(const float a);
      XMFINLINE BIntrinsicVector operator/=(const float a);
      XMFINLINE void scale(const float a);
      XMFINLINE void assignProduct(const float a, const BIntrinsicVector v1);

      // Negation
      XMFINLINE BIntrinsicVector operator-(void) const;
      XMFINLINE void inverse(void);

      // Basic vector arithmetic
      XMFINLINE BIntrinsicVector operator+(const BIntrinsicVector v) const;
      XMFINLINE BIntrinsicVector operator+=(const BIntrinsicVector v);
      XMFINLINE BIntrinsicVector operator-(const BIntrinsicVector v) const;
      XMFINLINE BIntrinsicVector operator-=(const BIntrinsicVector v);
      XMFINLINE void assignSum(const BIntrinsicVector v1, const BIntrinsicVector v2);
      XMFINLINE void assignDifference(const BIntrinsicVector v1, const BIntrinsicVector v2);
      XMFINLINE void scale(const BIntrinsicVector v);

      // Cross product
      XMFINLINE BIntrinsicVector cross(const BIntrinsicVector v) const;
      XMFINLINE void assignCrossProduct(const BIntrinsicVector v1, const BIntrinsicVector v2);

      // Dot product
      XMFINLINE float dot(const BIntrinsicVector v) const;

      // Length and normalize
      XMFINLINE float length(void) const;
      XMFINLINE float lengthEstimate(void) const;
      XMFINLINE float lengthSquared(void) const;
      XMFINLINE void normalize(void);
      XMFINLINE void normalizeEstimate(void);
      XMFINLINE bool safeNormalize(void);
      XMFINLINE bool safeNormalizeEstimate(void);

      // Other useful functions
      XMFINLINE void makePositive(void);
      XMFINLINE void round(void);
      XMFINLINE void clamp(const BIntrinsicVector minVec, const BIntrinsicVector maxVec);
      XMFINLINE void lerpPosition(const float t, const BIntrinsicVector p0, const BIntrinsicVector p1);
      XMFINLINE float distance(const BIntrinsicVector point) const;
      XMFINLINE float distanceEstimate(const BIntrinsicVector point) const;
      XMFINLINE float distanceSqr(const BIntrinsicVector point) const;
      XMFINLINE float xzDistance(const BIntrinsicVector point) const;
      XMFINLINE float xzDistanceSqr(const BIntrinsicVector point) const;
      XMFINLINE float xyDistance(const BIntrinsicVector point) const;
      XMFINLINE float xyDistanceSqr(const BIntrinsicVector point) const;
      float distanceToLine(const BIntrinsicVector p, const BIntrinsicVector dir) const;
      float distanceToLineSqr(const BIntrinsicVector p, const BIntrinsicVector dir) const;
      float distanceToPlane(const BIntrinsicVector pointOnPlane, const BIntrinsicVector planeNormal) const;
      float signedDistanceToPlane(const BIntrinsicVector pointOnPlane, const BIntrinsicVector planeNormal) const;
      float distanceToLineSegment(const BIntrinsicVector p1, const BIntrinsicVector p2) const;
      float distanceToLineSegmentSqr(const BIntrinsicVector p1, const BIntrinsicVector p2) const;
      float distanceToLineSegment2(const BIntrinsicVector p1, const BIntrinsicVector p2, bool &onSegment) const;
      float xzDistanceToLine(const BIntrinsicVector p, const BIntrinsicVector dir, BIntrinsicVector *pvClosest = NULL) const;
      float xzDistanceToLineSqr(const BIntrinsicVector p, const BIntrinsicVector dir, BIntrinsicVector *pvClosest = NULL) const;
      float xzDistanceToLineSegment(const BIntrinsicVector p1, const BIntrinsicVector p2, BIntrinsicVector *pvClosest = NULL) const;
      float xzDistanceToLineSegmentSqr(const BIntrinsicVector p1, const BIntrinsicVector p2, BIntrinsicVector *pvClosest = NULL) const;
      long xzEqualTo(const BIntrinsicVector v) const;
      float angleBetweenVector(const BIntrinsicVector v) const;
      void rotateXY(float theta);
      void rotateRelativeXY(const BIntrinsicVector rotate, float theta);
      void rotateXZ(float theta);
      void rotateRelativeXZ(const BIntrinsicVector rotate, float theta);
      void rotateYZ(float theta);
      void rotateRelativeYZ(const BIntrinsicVector rotate, float theta);
      void rotateAroundPoint(const BIntrinsicVector c, float xRads, float yRads, float zRads);
      float getAngleAroundY(void) const;
      float getAngleAroundX(void) const;
      float getAngleAroundZ(void) const;
      void projectOntoPlane(const BIntrinsicVector planeNormal, const BIntrinsicVector planePoint, BIntrinsicVector &projectedPoint);
      void projectOntoPlane(const BIntrinsicVector planeNormal, const BIntrinsicVector planePoint, const BIntrinsicVector projectionDir, BIntrinsicVector &projectedPoint);
      void projectOntoPlaneAsVector(const BIntrinsicVector planeNormal, BIntrinsicVector &projectedVector);
      void randomPerpendicular(BIntrinsicVector &result);
      bool projectYPlane(const BIntrinsicVector dir, BIntrinsicVector &point, float yplane) const;
      bool almostEqual(const BIntrinsicVector v, float tolerance = cFloatCompareEpsilon) const;
      bool almostEqualXZ(const BIntrinsicVector v, float tolerance = cFloatCompareEpsilon) const;
}; // BIntrinsicVector


//==============================================================================
__declspec(intrin_type) _DECLSPEC_ALIGN_16_ class BIntrinsicVector2 : public XMVECTOR
{
   public:

      // Constructors
      XMFINLINE BIntrinsicVector2(void);
      XMFINLINE BIntrinsicVector2(const XMVECTOR v);
      XMFINLINE BIntrinsicVector2(const float nx, const float ny);

      // Functions
      XMFINLINE BIntrinsicVector2 operator+(const BIntrinsicVector2 v) const;
      XMFINLINE BIntrinsicVector2 operator*(const float a) const;
}; // BIntrinsicVector2


//==============================================================================
__declspec(intrin_type) _DECLSPEC_ALIGN_16_ class BIntrinsicVector4 : public XMVECTOR
{
   public:

      // Constructors
      XMFINLINE BIntrinsicVector4(void);
      XMFINLINE BIntrinsicVector4(const XMVECTOR v);
      XMFINLINE BIntrinsicVector4(const float nx, const float ny, const float nz, const float nw);

      // Functions
      XMFINLINE BIntrinsicVector4 operator+(const BIntrinsicVector4 v) const;
      XMFINLINE BIntrinsicVector4 operator*(const float a) const;
      XMFINLINE void lerp(const float t, const BIntrinsicVector4 p0, const BIntrinsicVector4 p1);
}; // BIntrinsicVector4


//==============================================================================
class BIntrinsicShortVector
{
   public:

      // Constructors
      XMFINLINE BIntrinsicShortVector(void);
      XMFINLINE BIntrinsicShortVector(const BIntrinsicVector v);
      XMFINLINE BIntrinsicShortVector(const float nx, const float ny, const float nz);

      // Assignments
      XMFINLINE BIntrinsicShortVector &operator=(const BIntrinsicVector v);
      XMFINLINE void set(const float nx, const float ny, const float nz);
      XMFINLINE void zero(void);

      BShortFloat mx, my, mz;
}; // BIntrinsicShortVector

#include "intrinsicvector.inl"

#endif // _INTRINSIC_VECTOR_H_

//==============================================================================
// eof: intrinsicvector.h
//==============================================================================