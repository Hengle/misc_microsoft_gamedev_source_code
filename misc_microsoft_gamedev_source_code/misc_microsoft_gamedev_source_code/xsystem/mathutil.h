//==============================================================================
// mathutil.h
// 
// Copyright (c) 1999-2001 Ensemble Studios
//
// BANG Math Stuff
//==============================================================================

#ifndef _BMATH_H_
#define _BMATH_H_

//==============================================================================
class BPlane;

//==============================================================================
const float INV_RAND_MAX = (1.0f/(float)RAND_MAX);

//==============================================================================
inline bool isequal(float f1, float f2)
{
   return fabs(f1 - f2) < 0.0001;

} // isequal

//==============================================================================
inline float DotProduct(const float x1, const float y1a, const float z1,
                 const float x2, const float y2, const float z2)
{
   return x1 * x2 + y1a * y2 + z1 * z2;

} // DotProduct

// Confuse C++ into treating the bytes of a float as a long
#define INT_VAL(x) (*(long *) &(x))

// Confuse C++ into treating the bytes of a long as a float
#define FLOAT_VAL(x) (*(float *) &(x))

// Get sign bit of float.
#define SIGN_BIT(x) ((INT_VAL(x)&(1<<31)))

// Compare sign bits of floats.
#define SAME_SIGNS(a,b) (SIGN_BIT(a)==SIGN_BIT(b))

// Check PRECISE equality of floats.
#define IDENTICAL_FLOAT(a,b) (INT_VAL(a) == INT_VAL(b))

// Tests equality w/ epsilon, as in return(fabs(a-b)<cFloatCompareEpsilon)
// jce [1/19/2005] -- Note that if you make the epsilon value a parameter to this, it will generate longer assembly code,
// even though it is set to inline.  If you need a alternate epsilon, cut-n-paste is certainly one way...
__inline bool floatEqual(float a, float b)
{
   // Difference.
   float temp = a-b;
   
   // Clear sign bit to get absolute value of difference.
   INT_VAL(temp) &= 0x7FFFFFFF;
   
   // Compare with epsilon.
   return(INT_VAL(temp)<=INT_VAL(cFloatCompareEpsilon));
}


__inline bool floatEqual(float a, float b, float tolerance)
{
   // Difference.
   float temp = a-b;
   
   // Clear sign bit to get absolute value of difference.
   INT_VAL(temp) &= 0x7FFFFFFF;
   
   // Compare with epsilon.
   return(INT_VAL(temp)<=INT_VAL(tolerance));
}



// This function only reliably rounds within the update (because we reset the fp control
// word at the beginning of the update).  In render, etc. the result will depend on the current
// state of the FP rounding mode, which is stomped on by D3D.

//lint -e530
//lint -e715
inline long fastRound(float val)
{
#ifdef XBOX
   // rg [6/15/05] - FIXME - Might need to emulate exactly what happened on x86 depending on rounding mode.
   return static_cast<long>(val);
#else
   // non asm version: return((long)val);

   long temp;
   _asm
   {
      fld val; 
      fistp temp;
   }
   return(temp);
#endif
}
//lint +e530
//lint +e715

//==============================================================================
void createSinTable(void);
float lookupSin(float fangle);

void createCosTable(void);
float lookupCos(float fangle);

float circleTan(float x, float z);
//float randFloat( float max );

//void seedrandom(long s);
float getrandfraction(void);
long getirand(void);

//==============================================================================
long                          logBase2(long num);
long                          logBase2WithError(WORD val);
long                          logBase2WithError(DWORD val);
long                          twoTo(long num);

void                          VectorFromPoints(const float *px1, const float *py1, const float *pz1, 
                                 const float *px2, const float *py2, const float *pz2,
                                 float *px, float *py, float *pz);
void                          CrossProduct(const float *px1, const float *py1, const float *pz1, 
                                 const float *px2, const float *py2, const float *pz2,
                                 float *px, float *py, float *pz);
inline float                  VectorLength(const float *px, const float *py, const float *pz);
void                          normalize(float *px, float *py, float *pz);

bool                          orderXZCW(const BVector *vertices, const long num, long *sorted);

float                         distanceToLineSqr(float x, float y, float px, float py, float dx, float dy);
float                         distanceToLine(float x, float y, float px, float py, float dx, float dy);
float                         distanceToSegmentSqr(float x, float y, float sx1, float sy1, float sx2, float sy2);
float                         distanceToSegment(float x, float y, float sx1, float sy1, float sx2, float sy2);
float                         distanceBetweenSegmentsSqr(float x11, float y11, float x12, float y12,
                                 float x21, float y21, float x22, float y22);
float                         distanceBetweenSegments(float x11, float y11, float x12, float y12,
                                 float x21, float y21, float x22, float y22);
float                         closestPointOnSegment(float x, float y, float sx1, float sy1, float sx2, float sy2, float &res_x, float &res_y);

bool                          raySegmentIntersectionTriangle(const BVector *vertex, const DWORD vertexIndex[3], const BVector &normal, 
                                 const BVector &origin, const BVector &direction, const bool segment, 
                                 BVector &iPoint, const float errorEpsilon);

bool                          raySegmentIntersectionTriangle(const BVector *vertex, const WORD vertexIndex[3], 
                                                             const BVector &normal, const BVector &origin, 
                                                             const BVector &direction, const bool segment, 
                                                             BVector &iPoint, const float errorEpsilon);
bool                          raySegmentIntersectionTriangle(const BVector vertex[3], const BVector &normal, 
                                 const BVector &origin, const BVector &direction, const bool segment, 
                                 BVector &iPoint, const float errorEpsilon);

bool                          lineIntersectsPlane(const BPlane &plane, const BVector &origin, const BVector &direction, BVector &iPoint);
bool                          lineIntersectsPlane(const BVector &pointOnPlane, const BVector &normal, const BVector &origin, 
                                 const BVector &direction, BVector &iPoint);

bool                          raySegmentIntersectionTriangle(const BVector vertex[3], const BVector &origin, const BVector &direction, 
                                 const bool segment, BVector &iPoint, const float errorEpsilon);
bool                          raySegmentIntersectionTriangle(const BVector vertex[3], const WORD index[3], const BVector &origin, const BVector &direction, 
                                 const bool segment, BVector &iPoint, const float errorEpsilon);

bool                          raySegmentIntersectionPlane(const BVector vertex[3], const WORD index[3], const BVector &origin, const BVector &direction, 
                                                          const bool segment, BVector &iPoint, const float errorEpsilon);

float                         intersectSphere(const BVector &rayOrigin, const BVector &rayVector, const BVector &sphereOrigin, float radius);
bool                          spheresIntersect(const BVector &center1, float radius1, const BVector &center2, float radius2);
bool                          rayIntersectsSphere(const BVector& origin, const BVector& direction, const BVector &center, float radius);

BVector                       closestPointOnTriangle(const BVector vert1, const BVector vert2, const BVector vert3, const BVector point);
BVector                       closestPointOnTriangle(const BVector *vertex, const WORD vertexIndex[3], const BVector &point);

BVector                       closestPointOnLine(const BVector &a, const BVector &b, const BVector &point);
   
// Helper functions that you probably shouldn't use unless you really know what you are doing.
bool                          raySegmentIntersectionPlane(const BVector &pointOnPlane, const BVector &normal, const BVector &origin, 
                                 const BVector &direction, bool segment, BVector &iPoint, const float errorEpsilon);
bool                          pointInTriangle(const BVector &point, const BVector *vertex, const DWORD vertexIndex[3], const BVector &normal, 
                                 const float errorEpsilon);

bool                          pointInTriangle(const BVector &point, const BVector *vertex, const WORD vertexIndex[3], 
                                              const BVector &normal, const float errorEpsilon);

bool                          pointInTriangle(const BVector point, const BVector v1, const BVector v2, const BVector v3, 
                                              const BVector normal, const float errorEpsilon);

long                          factorial( long n );
DWORD                         vectorToRGBA(const BVector &vector);

//Simple "series" expander.
float                         expandSeries( float base, float factor, long number );


bool                          checkSide(const BVector *points, long count, float dx, float dz, float vx, float vz, float errorEpsilon); // used fore overlaps check
bool                          hullsOverlapXZ(const BVector *hull1, long hull1Count, const BVector *hull2, long hull2Count, float errorEpsilon);

long                          powerOfTwoGreaterOrEqual(long num);

float                         calculatePolyArea( BVector* points, long numberPoints );
void                          findXZConvexHull( BVector *points, long numberPoints, BDynamicSimVectorArray& result );
bool                          computeHullMajorAxisEdge(BDynamicVectorArray &hull, BVector &point1, BVector &point2);
bool                          pointBetweenLines(const float x, const float z, const BVector &p1, const BVector &p2, 
                                 const BVector &normal);
bool                          pointOnPlusSideOfLine(const float x, const float z, const BVector &p1, const BVector &normal);
float                         makeAngleBetweenZeroAndTwoPi(float angle);

float                         xyAngle(float x, float y);

enum
{
   cIntersectCirclesNone,
   cIntersectCirclesNormal,
   cIntersectCirclesContained
};
long                          intersectCircles(float center0x, float center0y, float radius0, float center1x, float center1y, float radius1,
                                 float &ix0, float &iy0, float &ix1, float &iy1);

bool                          intersect2Planes(const BPlane& Pn1, const BPlane& Pn2, BVector& lineP1, BVector& lineP2);

//==============================================================================
// These do left/right turn checks in the XZ plane
//==============================================================================
bool                          leftTurn(const BVector &v1, const BVector &v2, const BVector &v3);
bool                          rightTurn(const BVector &v1, const BVector &v2, const BVector &v3);

//==============================================================================
// quadratic equation solver
//==============================================================================
long                          quadratic(float a, float b, float c, float& x1, float& x2);

#include "mathutil.inl"

//==============================================================================
#endif // _BMATH_H_

//==============================================================================
// eof: mathutil.h
//==============================================================================













