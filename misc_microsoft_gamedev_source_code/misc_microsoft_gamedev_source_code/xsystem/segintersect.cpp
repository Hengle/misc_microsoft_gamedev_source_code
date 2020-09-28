//=============================================================================
// Copyright (c) 1997 Ensemble Studios
//
// Line segment intersection function
//=============================================================================

//Includes.
#include "xsystem.h"
#include "mathutil.h"
#include "segintersect.h"


//=============================================================================
// long segIntersect(float x11, float y11, float x12, float y12, float x21, float y21, 
//            float x22, float y22, float &r, float &s)
//
// Finds the intersection point of the line segments defined by (x11,y11) to (x12, y12) and
// the segment defined by (x21,y21) to (x22, y22).  Returns cNoIntersection if the
// segments do not intersect, cIntersection if the segments do intersect, and 
// cCoincident if the segments are coincident.  If cIntersection is returned, r and s
// hold the parameter values at which the intersection occurs in each of the segments.
//=============================================================================
long segIntersect(float x11, float y11, float x12, float y12, float x21, float y21, float x22, float y22,
                 float &r, float &s, float errorEpsilon)
{
   float denom = (x12-x11)*(y22-y21)-(y12-y11)*(x22-x21);
   float numR = (y11-y21)*(x22-x21)-(x11-x21)*(y22-y21);

   // Check for parallel conditions
   if((INT_VAL(denom)&0x7FFFFFFF)<=INT_VAL(errorEpsilon))
   {
      if((INT_VAL(numR)&0x7FFFFFFF)<=INT_VAL(errorEpsilon))
         return cCoincident;         // coincident
      else
         return cNoIntersection;      // parallel (but not coincident)
   }
   
   /*
   // Check for parallel conditions
   if(_fabs(denom) < errorEpsilon)
   {
      if(_fabs(numR) < errorEpsilon)
         return cCoincident;         // coincident
      else
         return cNoIntersection;      // parallel (but not coincident)
   }
   */

   // Compute param for first segment.
   float ooDenom = 1.0f/denom;
   r = numR * ooDenom;

   // See if it is between 0 and 1.
   // DLM 4/1/08 - In 2001, I introduced modified the segmentIntersects routine to use the errorEpsilon value
   // passed in for it's range checking.  I never did it for the other routines.  Why not?  
   if(r < -errorEpsilon || r > 1.0f+errorEpsilon)
      return(cNoIntersection);
   
   s = ooDenom*((y11-y21)*(x12-x11)-(x11-x21)*(y12-y11));

   // Check to see if r and s are between 0 and 1.
   if(s < -errorEpsilon || s > 1.0f + errorEpsilon)
      return cNoIntersection;

   return cIntersection;
}


//=============================================================================
// linesIntersect
//=============================================================================
long linesIntersect(float x11, float y11, float x12, float y12, float x21, float y21, float x22, float y22,
                    float &ix, float &iy)
{
   float denom = (x12-x11)*(y22-y21)-(y12-y11)*(x22-x21);
   float numR = (y11-y21)*(x22-x21)-(x11-x21)*(y22-y21);

   // Check for parallel conditions
   if(_fabs(denom) < cFloatCompareEpsilon)
   {
      if(_fabs(numR) < cFloatCompareEpsilon)
         return cCoincident;         // coincident
      else
         return cNoIntersection;      // parallel (but not coincident)
   }

   float r = numR /denom;
   ix=x11+(x12-x11)*r;
   iy=y11+(y12-y11)*r;

   return cIntersection;
}


//=============================================================================
// long segmentIntersect(BVector& p11, BVector& p12, BVector& p21, BVector& p22, BVector& intersection)
//
// Finds the intersection point of the line segments defined by the X,Z components
// of the line segments defined by (p11, p12) and (p21, p22).
// Returns cNoIntersection if the segments do not intersect, cIntersection if the
// segments do intersect, and cCoincident if the segments are coincident.  If
// cIntersection is returned, r and s hold the parameter values at which the
// intersection occurs in each of the segments.
//=============================================================================
long segmentIntersect(const BVector& p11, const BVector& p12, const BVector& p21, const BVector& p22, BVector& intersection, float errorEpsilon)
{
   float denom=(p12.x-p11.x)*(p22.z-p21.z)-(p12.z-p11.z)*(p22.x-p21.x);
   float numR =(p11.z-p21.z)*(p22.x-p21.x)-(p11.x-p21.x)*(p22.z-p21.z);

   //Check for parallel conditions
   //if (_fabs(denom) < cFloatCompareEpsilon)
   //if (_fabs(denom) < 0.01f)
   if (_fabs(denom) < errorEpsilon)
   {
      //if (_fabs(numR) < cFloatCompareEpsilon)
      //if (_fabs(numR) < 0.01f)
      if (_fabs(numR) < errorEpsilon)
         //Coincident.
         return(cCoincident);
      else
         //Parallel (but not coincident).
         return(cNoIntersection);
   }
   
   //Check to see if r and s are between 0 and 1.  Numerical errors could be spread
   //out by comparing to 0-cFloatCompareEpsilon and 1+cFloatCompareEpsilon, but this doesn't seem necessary.
   //This check now seems neccessary.  Reduce numerical errors by using cFloatCompareEpsilong in the
   //range checking.  dlm 7/18/01
   float ooDenom=1.0f/denom;
   float r=numR*ooDenom;
//   if(r<0.0f || r>1.0f)
//      return(cNoIntersection);
   if (r < -errorEpsilon || r > 1.0f+errorEpsilon)
      return(cNoIntersection);

   float numS=(p11.z-p21.z)*(p12.x-p11.x)-(p11.x-p21.x)*(p12.z-p11.z);
   float s=numS*ooDenom;
//   if(s<0.0f || s>1.0f)
//      return(cNoIntersection);
   //if (s < -0.01f || s > 1.01f)
   if (s < -errorEpsilon || s > 1.0f+errorEpsilon)
      return(cNoIntersection);

   intersection.x=p11.x+(p12.x-p11.x)*r;
   intersection.y=p11.y+(p12.y-p11.y)*r;
   intersection.z=p11.z+(p12.z-p11.z)*r;
   return(cIntersection);
}


//=============================================================================
// segmentIntersectLine(BVector &p11, BVector &p12, BVector &point, BVector &direction, BVector &intersection)
//
// Finds the intersection point of the line segment defined by the X,Z components
// (p11, p12) and the line determined by point and direction.
// Returns cNoIntersection if the segments do not intersect, cIntersection if the
// segments do intersect, and cCoincident if the segments are coincident.  If
// cIntersection is returned, intersection holds the point of intersection.
//=============================================================================
const float cErrorEpsilon = 1e-5f;
long segmentIntersectLine(const BVector &p11, const BVector &p12, const BVector &point, const BVector &direction, BVector &intersection)
{
   float denom=(p12.x-p11.x)*direction.z-(p12.z-p11.z)*direction.x;
   float numR =(p11.z-point.z)*direction.x-(p11.x-point.x)*direction.z;

   //Check for parallel conditions
   if (_fabs(denom) < cFloatCompareEpsilon)
   {
      if (_fabs(numR) < cFloatCompareEpsilon)
         //Coincident.
         return(cCoincident);
      else
         //Parallel (but not coincident).
         return(cNoIntersection);
   }
   
   float r=numR/denom;

   //Check to see if r is between 0 and 1
   // DLM 4/1/08 - In 2001, I introduced modified the segmentIntersects routine to use the errorEpsilon value
   // passed in for it's range checking.  I never did it for the other routines.  Why not?  
   if (r >= -cErrorEpsilon && r <= 1.0f + cErrorEpsilon)
   {
      intersection.x=p11.x+(p12.x-p11.x)*r;
      intersection.y=p11.y+(p12.y-p11.y)*r;
      intersection.z=p11.z+(p12.z-p11.z)*r;
      return(cIntersection);
   }

   return cNoIntersection;
}



//=============================================================================
// segmentIntersectRay(BVector &p11, BVector &p12, BVector &point, BVector &direction, BVector &intersection)
//
// Finds the intersection point of the line segment defined by the X,Z components
// (p11, p12) and the ray determined by point and direction.
// Returns cNoIntersection if the no intersection, cIntersection if the
// segments do intersect, and cCoincident if the segments are coincident.  If
// cIntersection is returned, intersection holds the point of intersection.
//=============================================================================
long segmentIntersectRay(const BVector &p11, const BVector &p12, const BVector &point, const BVector &direction, BVector &intersection)
{
   float denom=(p12.x-p11.x)*direction.z-(p12.z-p11.z)*direction.x;
   float numR =(p11.z-point.z)*direction.x-(p11.x-point.x)*direction.z;

   //Check for parallel conditions
   if(_fabs(denom) < cErrorEpsilon)
   {
      if(_fabs(numR) < cErrorEpsilon)
         //Coincident.
         return(cCoincident);
      else
         //Parallel (but not coincident).
         return(cNoIntersection);
   }
   
   //Check to see if the point is on the segment
   float ooDenom=1.0f/denom;
   float r=numR*ooDenom;
   if(r<-cErrorEpsilon || r>1.0f+cErrorEpsilon)
      return(cNoIntersection);

   //Check if the point is on the ray.
   float s=((p11.z-point.z)*(p12.x-p11.x)-(p11.x-point.x)*(p12.z-p11.z))*ooDenom;

   if(s<-cErrorEpsilon)
      return(cNoIntersection);

   // Ok, we found a good intersection.  Compute the actual intersection point.
   intersection.x = point.x + direction.x*s;
   intersection.y = point.y + direction.y*s;
   intersection.z = point.z + direction.z*s;

   return(cIntersection);
}


//=============================================================================
// patherSegmentIntersect
//=============================================================================
long patherSegmentIntersect(const BVector &start, const BVector &goal, const BVector &p1, const BVector &p2, BVector &intersection, bool &atP2)
{
   static const float cEpsilon=0.01f;
   static const float cOnePlusEpsilon=1.01f;

   float denom=(p2.x-p1.x)*(goal.z-start.z)-(p2.z-p1.z)*(goal.x-start.x);
   float numR =(p1.z-start.z)*(goal.x-start.x)-(p1.x-start.x)*(goal.z-start.z);

   //Check for parallel conditions
   if (_fabs(denom) < cEpsilon)
   {
      //if (_fabs(numR) < cFloatCompareEpsilon)
      if (_fabs(numR) < cEpsilon)
         //Coincident.
         return(cCoincident);
      else
         //Parallel (but not coincident).
         return(cNoIntersection);
   }

   // The r check determines whether the point is on the p1p2 segment.  If this fails there is no
   // intersection.
   float ooDenom=1.0f/denom;
   float r=numR*ooDenom;
   //if (r < -cEpsilon || r > cOnePlusEpsilon)
   if (r < 0.0f || r > 1.0f)
      return(cNoIntersection);

   // Set "atP2" flag.
   if(r>0.99f && r<1.01f)
      atP2=true;
   else
      atP2=false;

   // The s check tells us about the start to goal segment.  We determine whether the segment hits p1p2, the
   // ray hits p1p2, or the line hits p1p2.  The parallel/coincident cases have already been ruled out above.
   float numS=(p1.z-start.z)*(p2.x-p1.x)-(p1.x-start.x)*(p2.z-p1.z);
   float s=numS*ooDenom;

   long result;
   if(s<-cEpsilon)
      result=cLineHits;
   else if(s>cOnePlusEpsilon)
      result=cRayHits;
   else
      result=cIntersection;

   // In every case we compute the point of intersection.
   intersection.x=p1.x+(p2.x-p1.x)*r;
   intersection.y=p1.y+(p2.y-p1.y)*r;
   intersection.z=p1.z+(p2.z-p1.z)*r;

   return(result);
}


//=============================================================================
// patherSegmentIntersectPoly
//=============================================================================
long patherSegmentIntersectPoly(const BVector &start, const BVector &goal, const BVector *points, long numPoints, BVector &iPoint, float &distSqr, long &segIndex)
{
   // Check each segment
   long startPoint = numPoints-1;
   long rayHits=0;
   long segHits=0;
   distSqr=cMaximumFloat;
   float nextDistSqr=cMaximumFloat;
   segIndex=-1;
   BVector nextIPoint(cOriginVector);
   bool gotNext=false;
   BVector thisIPoint(cOriginVector);
   long nextSegIndex=-1;
   for(long endPoint=0; endPoint<numPoints; startPoint=endPoint, endPoint++)
   {
      // Check this segment.
      bool atEnd=false;
      long result = patherSegmentIntersect(start, goal, points[startPoint], points[endPoint], thisIPoint, atEnd);

      // If the intersection is right at the end point, skip to the next segment.
      if(atEnd && (result==cIntersection || result==cRayHits))
         continue;

      // If it is a segment or ray intersection, we care about it.
      if(result==cIntersection)
      {
         rayHits++;
         segHits++;
      }
      else if(result==cRayHits)
      {
         rayHits++;
      }
      else
         continue;

      // See if this is the closest point so far.
      float thisDistSqr=thisIPoint.xzDistanceSqr(start);
      if(thisDistSqr<distSqr)
      {
         // Push the current one into the next best slot, ignoring really, really close points.
         if(rayHits>1 && distSqr>0.0001f)
         {
            nextIPoint=iPoint;
            nextDistSqr=distSqr;
            nextSegIndex=segIndex;
            gotNext=true;
         }

         // Save intersection off as the closest one yet.
         iPoint=thisIPoint;
         distSqr=thisDistSqr;
         segIndex=startPoint;
      }
   }

   // No hits means, uh, no hits.
   if(segHits<1)
      return(cSegNoHit);

   // If the ray hits the hull an even number of times, the start point is outside the hull.
   if(rayHits%2==0)
   {
      // If the first intersection is really, really close we call this a "hit at start" segment.
      if(distSqr<0.0001f)
         return(cSegHitAtStart);

      // Otherwise it is a good, old-fashioned intersection.
      return(cSegHit);
   }

   // If we got here, it means that the start point was inside the hull.

   // If the intersection point is right on the edge, report that.  Because we know the point is inside,
   // a really close intersection means the segment is leaving the hull.
   if(distSqr<0.0001f)
   {
      // If we have a next closest point, return THAT as the intersection.
      if(gotNext)
      {
         iPoint=nextIPoint;
         distSqr=nextDistSqr;
         segIndex=nextSegIndex;
         return(cSegHit);
      }
      // Otherwise, there was only a single hit, so this segment "escapes" from the edge of the hull.
      else
         return(cSegEscapesEdge);
   }

   // Otherwise, this segment cuts through the hull.
   return(cSegCutsThrough);
}
