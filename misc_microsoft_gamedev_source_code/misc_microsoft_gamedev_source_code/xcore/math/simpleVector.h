//==============================================================================
// Copyright (c) 1997-2005 Ensemble Studios
//
// Vector class
//==============================================================================

#ifndef _SIMPLE_VECTOR_H_
#define _SIMPLE_VECTOR_H_

#if defined(XBOX)
   class BIntrinsicVector;
#endif
class BSimpleShortVector;

//==============================================================================
// BSimpleVector Class
//
// WARNING: This class can never have ANY virtual functions.  Don't add any
// member variables to it either.
//==============================================================================

class BSimpleVector
{
public:
   BSimpleVector(void)
   {
      // don't EVER initialize anything here
   }

#if defined(XBOX)
   explicit BSimpleVector(BIntrinsicVector v);
   BSimpleVector &operator=(BIntrinsicVector v);
#endif

   explicit BSimpleVector(float nx, float ny, float nz) :
   x(nx),
      y(ny),
      z(nz)
   {
   }

   explicit BSimpleVector(float v) :
   x(v),
      y(v),
      z(v)
   {
   }

   BSimpleVector(const BSimpleShortVector &);

   float                   length() const {return (float)sqrt(x*x + y*y+ z*z);}
   float                   lengthSquared(void) const {return(x*x + y*y + z*z);}
   inline void             normalize() {float len=length(); x/=len; y/=len; z/=len;}
   bool                    safeNormalize() {float len=length(); if(len>cFloatCompareEpsilon) {x/=len; y/=len; z/=len; return(true);} else return(false);}

   void                    zero( void ) { x=0.0f; y=0.0f; z=0.0f; }
   void                    makePositive( void ) { if (x < 0.0f) x=-x; if (y < 0.0f) y=-y; if (z < 0.0f) z=-z; }
   float                   distanceToLine( const BSimpleVector& p, const BSimpleVector& dir ) const;
   float                   distanceToLineSqr( const BSimpleVector& p, const BSimpleVector& dir ) const;
   float                   distanceToPlane( const BSimpleVector& pointOnPlane, const BSimpleVector& planeNormal ) const;
   float                   signedDistanceToPlane( const BSimpleVector& pointOnPlane, const BSimpleVector& planeNormal ) const;
   float                   distanceToLineSegment(const BSimpleVector &p1, const BSimpleVector &p2) const;
   float                   distanceToLineSegmentSqr(const BSimpleVector &p1, const BSimpleVector &p2) const;
   float                   distanceToLineSegment2( const BSimpleVector& p1, const BSimpleVector& p2, bool& onSegment ) const;

   float                   xzDistanceToLine( const BSimpleVector& p, const BSimpleVector& dir, BSimpleVector *pvClosest = NULL ) const;
   float                   xzDistanceToLineSqr( const BSimpleVector& p, const BSimpleVector& dir, BSimpleVector *pvClosest = NULL ) const;
   float                   xzDistanceToLineSegment(const BSimpleVector &p1, const BSimpleVector &p2, BSimpleVector *pvClosest = NULL) const;
   float                   xzDistanceToLineSegmentSqr(const BSimpleVector &p1, const BSimpleVector &p2, BSimpleVector *pvClosest = NULL) const;

   float                   distance(const BSimpleVector &point) const;
   float                   distanceSqr(const BSimpleVector &point) const;
   float                   xzDistance(const BSimpleVector &point) const;
   float                   xzDistanceSqr(const BSimpleVector &point) const;
   long                    xzEqualTo(const BSimpleVector &v) const;
   float                   xyDistance(const BSimpleVector &point) const;
   float                   xyDistanceSqr(const BSimpleVector &point) const;


   float                   angleBetweenVector( const BSimpleVector& v ) const;
   void                    rotateXY( float theta );
   void                    rotateRelativeXY( const BSimpleVector &rotate, float theta );
   void                    rotateXZ( float theta );
   void                    rotateRelativeXZ( const BSimpleVector &rotate, float theta );
   void                    rotateAroundPoint(const BSimpleVector &c, float xRads, float yRads, float zRads);
   float                   getAngleAroundY(void) const;

   void                    projectOntoPlane(const BSimpleVector &planeNormal, const BSimpleVector &planePoint, BSimpleVector &projectedPoint);
   void                    projectOntoPlane(const BSimpleVector &planeNormal, const BSimpleVector &planePoint, 
      const BSimpleVector &projectionDir, BSimpleVector &projectedPoint);
   void                    projectOntoPlaneAsVector(const BSimpleVector &planeNormal, BSimpleVector &projectedVector);

   void                    randomPerpendicular(BSimpleVector &result);

   // Faster (but less convenient notationally) math functions.
   void                    assignDifference(const BSimpleVector &v1, const BSimpleVector &v2) {x=v1.x-v2.x; y=v1.y-v2.y; z=v1.z-v2.z;}
   void                    assignSum(const BSimpleVector &v1, const BSimpleVector &v2) {x=v1.x+v2.x; y=v1.y+v2.y; z=v1.z+v2.z;}
   void                    assignProduct(const float a, const BSimpleVector &v1) {x=a*v1.x; y=a*v1.y; z=a*v1.z;}
   void                    assignCrossProduct(const BSimpleVector &v1, const BSimpleVector &v2) 
   {
      x = v1.y*v2.z - v1.z*v2.y;
      y = v1.z*v2.x - v1.x*v2.z;
      z = v1.x*v2.y - v1.y*v2.x;
   }

   void                    inverse() {x=-x; y=-y; z=-z;}

   inline BSimpleVector          cross( const BSimpleVector &v ) const
   {
      return BSimpleVector( y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x );
   }

   inline const float      dot( const BSimpleVector &v ) const
   {
      return(x*v.x + y*v.y + z*v.z); 
   }

   // finds where the current point, projected in direction dir, will intersect the plane y = yplane
   // having the more general version of this might also be cool
   bool                    projectYPlane(const BSimpleVector &dir, BSimpleVector &point, float yplane) const;

   // multiplication and division by a scalar
   inline const BSimpleVector     operator*(const float a) const;

   inline friend const BSimpleVector          operator*( const float a, const BSimpleVector &v);

   inline void             scale(float a)
   {
      x*=a;
      y*=a;
      z*=a;
   }

   inline BSimpleVector          &operator*=(const float a)
   {
      x*=a;
      y*=a;
      z*=a;
      return *this;
   }

   inline const BSimpleVector                 operator/(const float a) const;

   BSimpleVector                 &operator/=(const float a)
   {
      x/=a;
      y/=a;
      z/=a;
      return *this;
   }

   inline const BSimpleVector           operator-( void ) const;

   // addition and subtraction with a vector
   inline const BSimpleVector           operator+(const BSimpleVector &v) const;

   BSimpleVector                 &operator+=(const BSimpleVector &v)
   {
      x += v.x;
      y += v.y;
      z += v.z;
      return *this;
   }

   inline const BSimpleVector          operator-(const BSimpleVector &v) const;

   BSimpleVector                 &operator-=(const BSimpleVector &v)
   {
      x -= v.x;
      y -= v.y;
      z -= v.z;
      return *this;
   }

   BSimpleVector                 &operator=(const BSimpleShortVector &sv);

   // equality/inequality
   long                    operator==(const BSimpleVector &v) const
   {
      return(x==v.x && y==v.y && z==v.z);
   }

   long                    operator!=(const BSimpleVector &v) const
   {
      return(x!=v.x || y!=v.y || z!=v.z);
   }

   bool                    almostEqual(const BSimpleVector &v, float tolerance = cFloatCompareEpsilon) const;
   bool                    almostEqualXZ(const BSimpleVector &v, float tolerance = cFloatCompareEpsilon) const;


   // round all dimensions to nearest integral value
   void                    round()
   {
      x = (float)floor(x + 0.5f);
      y = (float)floor(y + 0.5f);
      z = (float)floor(z + 0.5f);
   }

   void                    set(float nx, float ny, float nz) {x=nx; y=ny; z=nz;}
   void                    clamp(const BSimpleVector &minVec, const BSimpleVector &maxVec);

   void                    scale(const BSimpleVector &v);

   // Lerp between 2 positions according to t (between 0 and 1)
   void                    lerpPosition(const float t, const BSimpleVector &p0, const BSimpleVector &p1)
   {
      float temp = max(0.0f, min(1.0f, t));
      x = p1.x * temp + p0.x * (1.0f - temp);
      y = p1.y * temp + p0.y * (1.0f - temp);
      z = p1.z * temp + p0.z * (1.0f - temp);
   }

   float                   x, y, z;

}; // BSimpleVector

//==============================================================================
class BSimpleVector2
{
public:
   BSimpleVector2(void)
   {
      // don't EVER initialize anything here
   }

   explicit BSimpleVector2(float nx, float ny) :
   x(nx),
      y(ny)
   {
   }

   // addition and subtraction with a vector
   inline const BSimpleVector2   operator+(const BSimpleVector2 &v) const;

   // multiplication and division by a scalar
   inline const BSimpleVector2   operator*(const float a) const;

   float x, y;
};

//==============================================================================
class BSimpleVector4
{
public:

   BSimpleVector4(void)
   {
      // don't EVER initialize anything here
   }

   explicit BSimpleVector4(float nx, float ny, float nz, float nw) :
   x(nx),
      y(ny),
      z(nz),
      w(nw)
   {
   }

   // addition and subtraction with a vector
   inline const BSimpleVector4   operator+(const BSimpleVector4 &v) const;

   // multiplication and division by a scalar
   inline const BSimpleVector4   operator*(const float a) const;

   void                    lerp(const float t, const BSimpleVector4 &p0, const BSimpleVector4 &p1)
   {
      float temp = max(0.0f, min(1.0f, t));
      x = p1.x * temp + p0.x * (1.0f - temp);
      y = p1.y * temp + p0.y * (1.0f - temp);
      z = p1.z * temp + p0.z * (1.0f - temp);
      w = p1.w * temp + p0.w * (1.0f - temp);
   }

   float x, y, z, w;
};


//==============================================================================
// 
//==============================================================================
class BSimpleShortVector
{
public:
   BSimpleShortVector(void) { } // if you do anything here, I will hunt you down

   BSimpleShortVector(const BSimpleVector &v);

   BSimpleShortVector(float x, float y, float z);

   void zero(void) { mx = 0; my = 0; mz = 0; }

   void set(float x, float y, float z) { mx = x; my = y; mz = z; }

   long operator==(const BSimpleShortVector &v) const
   {
      return (mx.mValue == v.mx.mValue) && (my.mValue == v.my.mValue) && (mz.mValue == v.mz.mValue);
   }

   long operator!=(const BSimpleShortVector &v) const
   {
      return (mx.mValue != v.mx.mValue) || (my.mValue != v.my.mValue) || (mz.mValue != v.mz.mValue);
   }

   BSimpleShortVector &operator=(const BSimpleVector &v);

   BSimpleShortVector operator-(const BSimpleVector &v) const;

   BSimpleShortVector &operator-=(const BSimpleVector &v);

   BSimpleShortVector &operator+=(const BSimpleVector &v);

   BSimpleShortVector operator*(const float a) const;

   BSimpleShortVector operator-(const BSimpleShortVector &v) const;
   BSimpleShortVector operator+(const BSimpleShortVector &v) const;

   // cross product
   BSimpleShortVector operator*(const BSimpleVector &v) const;

   BSimpleShortVector operator*(const BSimpleShortVector &v) const;

   BSimpleShortVector &operator*=(const float a);

   void normalize(void);

   bool safeNormalize(void);

   float length(void);

   BShortFloat mx, my, mz;
};


//==============================================================================
// BSimpleVector::operator*(float a)
//==============================================================================
inline const BSimpleVector BSimpleVector::operator*(const float a) const
{
   return BSimpleVector( x*a, y*a, z*a );
}

//==============================================================================
// operator*=(float a, const BSimpleVector &v)
//
// Friend function to do scalar*vector
//==============================================================================
inline const BSimpleVector operator*( const float a, const BSimpleVector &v)
{
   return BSimpleVector( v.x * a, v.y * a, v.z * a);
}

//==============================================================================
// BSimpleVector::operator/(float a)
//==============================================================================
inline const BSimpleVector BSimpleVector::operator/(const float a) const
{
   return BSimpleVector( x/a, y/a, z/a );
}

//==============================================================================
// BSimpleVector::operator+(BSimpleVector &v)
//==============================================================================
inline const BSimpleVector BSimpleVector::operator+(const BSimpleVector &v) const
{
   return BSimpleVector( x + v.x, y + v.y, z + v.z);
}

//==============================================================================
// BSimpleVector::operator-(const BSimpleVector& v)
//==============================================================================
inline const BSimpleVector BSimpleVector::operator-(const BSimpleVector& v) const
{
   return BSimpleVector( x - v.x, y - v.y, z - v.z);
}

//==============================================================================
// BSimpleVector::operator-(void)
//==============================================================================
inline const BSimpleVector BSimpleVector::operator-(void) const
{
   return BSimpleVector( -x, -y, -z);
}


//==============================================================================
// BSimpleVector2::operator+(const BSimpleVector2 &v)
//==============================================================================
inline const BSimpleVector2 BSimpleVector2::operator+(const BSimpleVector2 &v) const
{
   return BSimpleVector2( x + v.x, y + v.y);
}

//==============================================================================
// BSimpleVector2::operator*(float a)
//==============================================================================
inline const BSimpleVector2 BSimpleVector2::operator*(const float a) const
{
   return BSimpleVector2( x*a, y*a );
}


//==============================================================================
// BSimpleVector4::operator+(const BSimpleVector4 &v)
//==============================================================================
inline const BSimpleVector4 BSimpleVector4::operator+(const BSimpleVector4 &v) const
{
   return BSimpleVector4( x + v.x, y + v.y, z + v.z, w + v.w);
}

//==============================================================================
// BSimpleVector4::operator*(float a)
//==============================================================================
inline const BSimpleVector4 BSimpleVector4::operator*(const float a) const
{
   return BSimpleVector4( x*a, y*a, z*a, w*a );
}

//==============================================================================
// 
//==============================================================================

#endif // _SIMPLE_VECTOR_H_

//==============================================================================
// eof: simplevector.h
//==============================================================================

