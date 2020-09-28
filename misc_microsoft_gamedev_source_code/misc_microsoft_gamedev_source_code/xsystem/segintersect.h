//=============================================================================
// Copyright (c) 1997-2001 Ensemble Studios
//
// Line segment intersection function
//=============================================================================

#ifndef _SEGINTERSECTION_H_
#define _SEGINTERSECTION_H_

// Return value defines.
enum
{
   cNoIntersection,
   cIntersection,
   cCoincident,
   cRayHits,      // used only by pather function
   cLineHits      // used only by pather function
};

long segIntersect(float x11, float y11, float x12, float y12, float x21, float y21, float x22, float y22,
                  float &r, float &s, float errorEpsilon=cFloatCompareEpsilon);

long segmentIntersect(const BVector& p11, const BVector& p12, const BVector& p21, const BVector& p22, BVector& intersection, float errorEpsilon=0.001f);
long segmentIntersectLine(const BVector &p11, const BVector &p12, const BVector &point, const BVector &direction, BVector &intersection);
long segmentIntersectRay(const BVector &p11, const BVector &p12, const BVector &point, const BVector &direction, BVector &intersection);


long linesIntersect(float x11, float y11, float x12, float y12, float x21, float y21, float x22, float y22,
                    float &ix, float &iy);




enum
{
   cSegNoHit,
   cSegHit,
   cSegHitAtStart,
   cSegEscapesEdge,
   cSegCutsThrough
};      
long patherSegmentIntersect(const BVector &start, const BVector &goal, const BVector &p1, const BVector &p2, BVector &intersection, bool &atP2);
long patherSegmentIntersectPoly(const BVector &start, const BVector &goal, const BVector *points, long numPoints, BVector &iPoint, float &distSqr, long &segIndex);


#endif