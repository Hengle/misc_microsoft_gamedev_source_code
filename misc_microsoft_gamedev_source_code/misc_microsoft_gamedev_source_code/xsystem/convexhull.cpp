//==============================================================================
// convexhull.cpp
//
// Copyright (c) 1999-2000, Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "convexhull.h"
#include "mathutil.h"
#include "pointin.h"
#include "segintersect.h"
#include "bitarray.h"
#include "stream\stream.h"

//#define DEBUG_ADDPOINTS

const float cSpecialIntersectionEpsilon=10.0f*cFloatCompareEpsilon;
const long cInitialScratchSize = 48;

//=============================================================================
// Static member variables.
BDynamicSimVectorArray  BConvexHull::msScratchPoints;
BBitArray BConvexHull::mUsedIndices;
BDynamicSimLongArray BConvexHull::mUpperIndices;
BDynamicSimLongArray BConvexHull::mLowerIndices;

GFIMPLEMENTVERSION(BConvexHull, 1);

//=============================================================================
// comparePoints
//=============================================================================
int comparePoints(const void *v1, const void *v2)
{
   if(((BVector *)v1)->x < ((BVector *)v2)->x)
      return(-1);

   if(((BVector *)v1)->x == ((BVector *)v2)->x)
   {
      if(((BVector *)v1)->z < ((BVector *)v2)->z)
         return(-1);
      return(1);
   }
   
   return(1);
}


//=============================================================================
// comparePoints2
//=============================================================================
int comparePoints2(const void *v1, const void *v2)
{
   if(((BVector *)v1)->x < ((BVector *)v2)->x)
      return(1);

   if(((BVector *)v1)->x == ((BVector *)v2)->x)
   {
      if(((BVector *)v1)->z < ((BVector *)v2)->z)
         return(1);
      return(-1);
   }
   
   return(-1);
}


//=============================================================================
//  shellsort
//=============================================================================
void shellsort(BVector *a, long num)
{
	long h = 1;
   // Find the largest h value possible 
   while ((h * 3 + 1) < num) 
   {
	    h = 3 * h + 1;
	}

   // while h remains larger than 0 
   while( h > 0 ) 
   {
      // For each set of elements (there are h sets)
      for (int i = h - 1; i < num; i++) 
      {
         // pick the last element in the set
         BVector v = a[i];
         long j = i;

         // compare the element at B to the one before it in the set
         // if they are out of order continue this loop, moving
         // elements "back" to make room for B to be inserted.
         for( j = i; (j >= h) && (comparePoints((void*)(a+(j-h)), (void*)&v) > 0); j -= h) 
         //for( j = i; (j >= h) && (a[j-h].x>v.x || (a[j-h].x==v.x && a[j-h].z>v.z)); j -= h) 
         {
            a[j] = a[j-h];
         }
         a[j] = v;
      }
      h = h / 3;
   }
}


//=============================================================================
// shellsort
//=============================================================================
void shellsort2(BVector *a, long num)
{ 
	long h = 1;
   // Find the largest h value possible 
   while ((h * 3 + 1) < num) 
   {
	    h = 3 * h + 1;
	}

   // while h remains larger than 0 
   while( h > 0 ) 
   {
      // For each set of elements (there are h sets)
      for (int i = h - 1; i < num; i++) 
      {
         // pick the last element in the set
         BVector v = a[i];
         long j = i;

         // compare the element at B to the one before it in the set
         // if they are out of order continue this loop, moving
         // elements "back" to make room for B to be inserted.
         for( j = i; (j >= h) && (comparePoints2((void*)(a+(j-h)), (void*)&v) > 0); j -= h) 
         {
            a[j] = a[j-h];
         }
         a[j] = v;
      }
      h = h / 3;
   }
}


//=============================================================================
// insertionSort
//=============================================================================
void insertionSort(BVector *a, long num)
{
   BVector temp;
   for (long i=num-2; i>=0; i-- )
   {
      temp = a[i];
      long j;
      for(j=i+1; j<num && (comparePoints((void*)&temp, (void*)(&a[j]))>0); j++)
         a[j-1] = a[j];
      a[j-1] = temp;
   }
}


//=============================================================================
// scanHull
//=============================================================================
long scanHull(BVector *points, long n)
{
   long h=1;
   for(long i=1; i<n; i++)
   {
      // While top two points on stack and points[i] don't make a right turn,
      // pop the top stack element off.
      while(h>=2 && !rightTurn(points[h-2], points[h-1], points[i]))
         h--;

      // Put points[i] on the stack.
      bswap(points[i], points[h]);
      h++;
   }
   return(h);
}


//=============================================================================
// BConvexHull::sortPoints
//=============================================================================
void BConvexHull::sortPoints(void)
{
   // Sort.
   shellsort(mPoints.getPtr(), mPoints.getNumber());
}


//=============================================================================
// BConvexHull::scanHullUpper
//=============================================================================
void BConvexHull::scanHullUpper(void)
{
   // Start with with just the first point on stack.
   mUpperIndices.setNumber(0);
   mUsedIndices.setNumber(mPoints.getNumber());
   mUsedIndices.clear();
   mUpperIndices.add(0);
   mUsedIndices.setBit(0);

   // Scan through each original point.
   for(long i=1; i<mPoints.getNumber(); i++)
   {
      // While top two points on stack and points[i] don't make a right turn,
      // pop the top stack element off.
      while(mUpperIndices.getNumber()>=2 && !rightTurn(mPoints[mUpperIndices[mUpperIndices.getNumber()-2]], mPoints[mUpperIndices[mUpperIndices.getNumber()-1]], mPoints[i]))
      {
         // Mark index as unused.
         long index=mUpperIndices.getNumber()-1;
         mUsedIndices.clearBit(mUpperIndices[index]);
         // Pop it off the stack.
         mUpperIndices.setNumber(index);
      }

      // Push point on the stack.
      mUpperIndices.add(i);
      mUsedIndices.setBit(i);
   }
}


//=============================================================================
// BConvexHull::scanHullLower
//=============================================================================
void BConvexHull::scanHullLower(void)
{
   // Start with with just the first point on stack.
   mLowerIndices.setNumber(0);
   mLowerIndices.add(0);

   // Reset last point.
   mUsedIndices.clearBit(mUsedIndices.getNumber()-1);

   // Scan through each original point.
   for(long i=1; i<mPoints.getNumber(); i++)
   {
      // Skip already used points,
      if(mUsedIndices.isBitSet(i))
         continue;

      // While top two points on stack and points[i] don't make a right turn,
      // pop the top stack element off.
      while(mLowerIndices.getNumber()>=2 && !leftTurn(mPoints[mLowerIndices[mLowerIndices.getNumber()-2]], mPoints[mLowerIndices[mLowerIndices.getNumber()-1]], mPoints[i]))
      {
         // Mark index as unused.
         long index=mLowerIndices.getNumber()-1;
         mUsedIndices.clearBit(mLowerIndices[index]);
         // Pop it off the stack.
         mLowerIndices.setNumber(index);
      }

      // Push point on the stack.
      mLowerIndices.add(i);
      mUsedIndices.setBit(i);
   }
}


//=============================================================================
// BConvexHull::BConvexHull
//=============================================================================
BConvexHull::BConvexHull(void)
{
   clear();
}


//=============================================================================
// BConvexHull::BConvexHull
//=============================================================================
BConvexHull::BConvexHull(const BConvexHull &h)
{
   clear();

   operator=(h);
}


//=============================================================================
// BConvexHull::~BConvexHull
//=============================================================================
BConvexHull::~BConvexHull(void)
{
}


//=============================================================================
// BConvexHull::operator=
//=============================================================================
BConvexHull &BConvexHull::operator=(const BConvexHull &h)
{
   if (this == &h)
   {
      BASSERT(0);
      return *this;
   }
   //initialize(h.mPoints, h.mPoints.getNumber(), true);

   // Copy points.
   long num = h.mPoints.getNumber();
   mPoints.setNumber(num);
   if(num > 0)
      memcpy((BVector*)mPoints.getPtr(), (const BVector *)h.mPoints.getPtr(), num*sizeof(BVector));

   mBoundingMin = h.mBoundingMin;
   mBoundingMax = h.mBoundingMax;
   mCenter = h.mCenter;

   return(*this);
}


//=============================================================================
// BConvexHull::initialize
//
// Initializes the hull with the given points (clearing any exisitng info).
// If alreadyConvex is true, the points are considered to already define a
// convex hull -- it is bad to lie about this.
//=============================================================================
bool BConvexHull::initialize(const BVector *points, const long num, const bool alreadyConvex)
{
   // Check params for validity.
   if(!points || num<=0)
   {
      BASSERT(0);
      return(false);
   }

   // Clear out any old points.
   clear();

   // If the points are already convex, just copy them over.
   if(alreadyConvex)
   {
      mPoints.setNumber(num);
      for (long i = 0; i < num; i++)
      {
         // Actually do the add.
         mPoints[i] = points[i];

         // Check bounding box.
         if(mPoints[i].x < mBoundingMin.x)
            mBoundingMin.x = mPoints[i].x;
         if(mPoints[i].z < mBoundingMin.z)
            mBoundingMin.z = mPoints[i].z;
         if(mPoints[i].x > mBoundingMax.x)
            mBoundingMax.x = mPoints[i].x;
         if(mPoints[i].z > mBoundingMax.z)
            mBoundingMax.z = mPoints[i].z;
      }

      // Copy the points over.
      // Can you say thrash memory? 
      //for(long i=0; i<num; i++)
      //   addPoint(points[i]);
   
      // Compute the center.
      computeCenter();

      return(true);
   }
   
   // Add the points.
   bool result = addPoints(points, num);   
   return(result);
}


//=============================================================================
// BConvexHull::addPoints
// The bValidPoints parm means you have already done the check inside work
// for this set of points, and you don't want convexhull to do it again.
// As with the initialize function, it would be bad to lie about this.
//=============================================================================
bool BConvexHull::addPoints(const BVector *points, const long num, const bool bValidPoints)
{
#if 0
   // Check params for validity.
   if(!points || num<=0)
   {
      BASSERT(0);
      return(false);
   }
   
   bValidPoints;

   // Add new points to the list.
   long origNum=mPoints.getNumber();
   mPoints.setNumber(origNum+num);
   memcpy(((BVector *)mPoints.getData())+origNum, points, num*sizeof(mPoints[0]));

   // Sort.
   sortPoints();

   // Scan upper hull.
   scanHullUpper();

   // Lower.
   scanHullLower();

   // Copy used points on upper hull.
   msScratchPoints.setNumber(mUpperIndices.getNumber()+mLowerIndices.getNumber()-2);
   long currIndex=0;
   for(long i=0; i<mUpperIndices.getNumber(); i++)
   {
      msScratchPoints[currIndex]=mPoints[mUpperIndices[i]];
      currIndex++;
   }

   // Now extra points from lower hull.
   for(i=mLowerIndices.getNumber()-2; i>=1; i--)
   {
      msScratchPoints[currIndex]=mPoints[mLowerIndices[i]];
      currIndex++;
   }

   // Copy back,
   mPoints.setNumber(msScratchPoints.getNumber());
   memcpy((BVector*)mPoints.getData(), (BVector*)msScratchPoints.getData(), msScratchPoints.getNumber()*sizeof(mPoints[0]));


   computeBoundingBox();
   computeCenter();
#endif
//#if 0
   // Check params for validity.
   if(!points || num<=0)
   {
      BASSERT(0);
      return(false);
   }
   
   bValidPoints;

   // Add new points to the list.
   long origNum=mPoints.getNumber();
   mPoints.setNumber(origNum+num);
   memcpy(((BVector *)mPoints.getPtr())+origNum, points, num*sizeof(mPoints[0]));

   // Sort.
   //if(mPoints.getNumber()>10)
     // qsort((BVector *)mPoints, mPoints.getNumber(), sizeof(mPoints[0]), comparePoints);
   //else
      shellsort(mPoints.getPtr(), mPoints.getNumber());
   //insertionSort(mPoints, mPoints.getNumber());

   // Build upper hull.
   long f=scanHull(mPoints.getPtr(), mPoints.getNumber());

   for(long i=0; i<f-1; i++)
      bswap(mPoints[i], mPoints[i+1]);

   // Sort for lower hull.
   //if(mPoints.getNumber()>10)
     // qsort(&(mPoints[f-2]), mPoints.getNumber()-f+2, sizeof(mPoints[0]), comparePoints2);
   //else
      shellsort2(mPoints.getPtr()+(f-2), mPoints.getNumber()-f+2);

   // Build lower hull.
   long g=scanHull(&mPoints[f-2], mPoints.getNumber()-f+2);

   // Update point count.
   mPoints.setNumber(f+g-2);

   computeBoundingBox();
   computeCenter();
//#endif
#if 0

   #ifdef DEBUG_ADDPOINTS
   blog("BConvexHull::addPoints");
   #endif

   // Check params for validity.
   if(!points || num<=0)
   {
      BASSERT(0);
      return(false);
   }

   // Make an array to hold all input points plus all existing points in
   // the hull.  Uses temporary scratch variable msScratchPoints -- so not reentrant.
   // First, clear the scratch space.
   msScratchPoints.setNumber(0);

   // Add new points, dropping any that are inside the hull already.
   for(long i=0; i<num; i++)
   {
      // See if the point is already one of the hull vertices.
      long hullVertexIndex = hullVertex(points[i]);
      if(hullVertexIndex >= 0)
         continue;      

      // See if the point is inside the current hull.
      bool result = false;
      if (!bValidPoints)
         result = inside(points[i]);
      if(!result)
      {
         // If it is not inside, add it to the list to consider.
         long currentNum = msScratchPoints.getNumber();
         msScratchPoints.setNumber(currentNum+1);
         msScratchPoints[currentNum] = points[i];
      }
   }

   // If all the points were on/in the hull, we don't need to do anything -- so
   // just bail out now.
   if(msScratchPoints.getNumber() == 0)
      return(true);

   // Now copy in all existing hull points.
   long numExisting = mPoints.getNumber();
   long currentNum=msScratchPoints.getNumber();
   long numPoints = numExisting+currentNum;
   msScratchPoints.setNumber(numPoints);
   memcpy(&(msScratchPoints[currentNum]), (BVector*)mPoints, numExisting*sizeof(BVector));


   #ifdef DEBUG_ADDPOINTS
   blog("  starting points (new + existing):");
   for(i=0; i<msScratchPoints.getNumber(); i++)
      blog("    (%0.3f, %0.3f, %0.3f)", msScratchPoints[i].x, msScratchPoints[i].y, msScratchPoints[i].z);
   #endif

   // If the Array of Points hasn't been preallocated, then addPoint is 
   // going to do a memory allocation for every point it adds.  This is SLOW.
   // Preallocate the mPoints array to be at least large enough to hold
   // all the points, and then "set" it's size back to zero so that 
   // the other array operations work correctly.  Worse case scenario, 
   // the actual size of the mPoints array might be slightly larger than
   // the actual number of points it needs, but this is better than doing
   // an allocation per add.  DLM 6/30/01
   mPoints.setNumber(numPoints);

   // Clear out the points array so it can hold the results.
   clear();


   //Find the point with the smallest Z value and add that as our first point.
   //If two points have the smallest Z, we'll take the one with the smallest X.
   long startIndex=0;
   float startZ=msScratchPoints[0].z;
   for (i=1; i < numPoints; i++)
   {
      if ((msScratchPoints[i].z < startZ) ||
         ((msScratchPoints[i].z == startZ) && (msScratchPoints[i].x < msScratchPoints[startIndex].x)))
      {
         startZ=msScratchPoints[i].z;
         startIndex=i;
      }
   }

   // Add starting point to result list.
   bool result = addPoint(msScratchPoints[startIndex]);
   if(!result)
   {
      BASSERT(0);
      return(false);
   }

   #ifdef DEBUG_ADDPOINTS
   blog("  Choosing initial point of (%0.3f, %0.3f, %0.3f)", msScratchPoints[startIndex].x, msScratchPoints[startIndex].y, msScratchPoints[startIndex].z);
   #endif

   // Use that Static array.
   
   mIndices.setNumber(numPoints);
   for(i=0; i<numPoints; i++)
      mIndices[i] = i;
   // Remove startindex from list.
   mIndices[startIndex] = mIndices[numPoints-1];
   mIndices.setNumber(numPoints-1);

   //We set our current circle tan value to 0 (since we're starting with the
   //smallest Z (and X if Z ties) valued point).
   float currentCT=0.0f;
   long currentIndex=startIndex;

   //Do the wrap.  Run through the msScratchPoints (until we get back to the start point)
   //and add in the msScratchPoints with the smallest theta from the current point.
   do 
   {
      #ifdef DEBUG_ADDPOINTS
      blog("  start pass:");
      #endif

      long bestIndex=-1;
      float bestCT=0.0f;
      float bestDistance=0.0f;
      long bestIndicesIndex=-1;
      for (long i=0; i < mIndices.getNumber(); i++)
      {
         long checkIndex = mIndices[i];

         //Calc the circle tan between this point and the current point.
         float dX=msScratchPoints[checkIndex].x-msScratchPoints[currentIndex].x;
         float dZ=msScratchPoints[checkIndex].z-msScratchPoints[currentIndex].z;
         float cT=circleTan(dX, dZ);

         #ifdef DEBUG_ADDPOINTS
         blog("    Checking (%0.3f, %0.3f, %0.3f)  cT=%f", msScratchPoints[checkIndex].x, msScratchPoints[checkIndex].y, 
            msScratchPoints[checkIndex].z, cT);
         #endif

         //If the circleTan is less than our currentCT, then it's not a point we want (since
         //it would wrap past the circle if it were on the hull).  We also don't want the
         //point if its cT is over 2PI (though that should never happen) for the same reason.
         if ((cT < currentCT) || (cT > cTwoPi))
         {
            #ifdef DEBUG_ADDPOINTS
            if(cT<currentCT)
               blog("      Skipping, cT<currentCT");
            else
               blog("      Skipping, cT>2PI");
            #endif
   
            continue;
         }
         //Calc the distance between the two msScratchPoints (because we really want the farthest
         //point if any msScratchPoints are colinear).
         float distance=(float)sqrt(dX*dX+dZ*dZ);

         //If this is the first pass through, take the point.  Otherwise, take this point
         //if its cT is less than our bestCT or the circle tans are colinear and this point
         //is farther from the last hull point than the best one.
         if ((bestIndex == -1) ||
            (cT < bestCT) ||
            ((_fabs(cT-bestCT) < cFloatCompareEpsilon) && (distance > bestDistance)))
         {
            bestIndex=checkIndex;
            bestIndicesIndex=i;
            bestCT=cT;
            bestDistance=distance;
         }
      }

      //Calc the circle tan between the current point and the start point.  If
      //that is less than the best new point we could find, then we're done because
      //we've looped back to where the start point should be the next point on the
      //convex hull.
      float dX=msScratchPoints[startIndex].x-msScratchPoints[currentIndex].x;
      float dZ=msScratchPoints[startIndex].z-msScratchPoints[currentIndex].z;
      float startCT=circleTan(dX, dZ);
      float startDistance=(float)sqrt(dX*dX+dZ*dZ);
      if ((startCT < bestCT) ||
         ((_fabs(startCT-bestCT) < cFloatCompareEpsilon) && (startDistance > bestDistance)))
         break;

      //If we don't have a best point, we're done.
      if (bestIndex == -1)
         break;

      // We've got a point we want to add.  First check for duplicates of this
      // point already in the list.
      long hullVertexIndex = hullVertex(msScratchPoints[bestIndex]);
      // If we didn't find a match, add the point to the list.
      if(hullVertexIndex<0)
      {
         bool result = addPoint(msScratchPoints[bestIndex]);
         if(!result)
         {
            BASSERT(0);
            return(false);
         }

         #ifdef DEBUG_ADDPOINTS
         blog("  Adding (%0.3f, %0.3f, %0.3f)", msScratchPoints[bestIndex].x, msScratchPoints[bestIndex].y, msScratchPoints[bestIndex].z);
         #endif
      }

      // Remove from index list.
      long num = mIndices.getNumber();
      mIndices[bestIndicesIndex] = mIndices[num-1];
      mIndices.setNumber(num-1);

      currentCT=bestCT;
      currentIndex=bestIndex;

   #pragma warning(disable: 4127)
   } while(1);
   #pragma warning(default: 4127)

   computeCenter();   
#endif


   return(true);
}


//=============================================================================
// BConvexHull::removePoint
//=============================================================================
bool BConvexHull::removePoint(const long index)
{
   // Check for valid index.
   long numPoints = mPoints.getNumber();
   if(index<0 || index>=numPoints)
   {
      BASSERT(0);
      return(false);
   }

   // If we are removing the last remaining point, handle this as a special case.
   if(numPoints == 1)
   {
      clear();
      return(true);
   }

   // Size scratch space.
   bool result = msScratchPoints.setNumber(numPoints-1);
   if(!result)
      return(false);

   // Copy the points we want to keep into scratch space.
   long currIndex=0;
   for(long i=0; i<numPoints; i++, currIndex++)
      msScratchPoints[currIndex] = mPoints[i];

   // Reinitialize the hull.
   initialize(msScratchPoints.getPtr(), numPoints-1, true);

   return(true);
}


//=============================================================================
// BConvexHull::inside
//=============================================================================
bool BConvexHull::inside(const BVector &point) const
{
   // If we have no points, bail out now.
   if(mPoints.getNumber() == 0)
      return(false);

   // First check bounding box.
   if(point.x < mBoundingMin.x)
      return(false);
   if(point.z < mBoundingMin.z)
      return(false);
   if(point.x > mBoundingMax.x)
      return(false);
   if(point.z > mBoundingMax.z)
      return(false);

   // Ok were inside the bounding box.  Now check the actual hull.
   bool result = pointInXZProjection(mPoints.getPtr(), mPoints.getNumber(), point);
   return(result);
}

//=============================================================================
// BConvexHull::insideOrOnEdge
// Slower then inside, but returns true if the point is actually on the
// hull.  ::inside only returns true if the point is inside, or on the top
// or right of the hull.
// This is really slow.  Find an elegant solution that's fast0r.
//=============================================================================
bool BConvexHull::insideOrOnEdge(const BVector &point, bool *pbOnEdge) const
{

   if (pbOnEdge)
      *pbOnEdge = false;

   // If we have no points, bail out now.
   if(mPoints.getNumber() == 0)
      return(false);

   // First check bounding box.
   if(point.x < mBoundingMin.x)
      return(false);
   if(point.z < mBoundingMin.z)
      return(false);
   if(point.x > mBoundingMax.x)
      return(false);
   if(point.z > mBoundingMax.z)
      return(false);

   // Ok were inside the bounding box.  Now check the actual hull.
   bool bRet = pointInXZProjection(mPoints.getPtr(), mPoints.getNumber(), point);
   if (bRet && pbOnEdge == NULL)
      return true;

   long lNumPoints = mPoints.getNumber();
   long lIdx1 = lNumPoints - 1;
   for (long lIdx2 = 0; lIdx2 < lNumPoints; lIdx2++)
   {
      if (point.xzDistanceToLineSegmentSqr(mPoints[lIdx1], mPoints[lIdx2]) < 0.001f)
      {
         if (pbOnEdge)
            *pbOnEdge = true;
         return true;
      }
      lIdx1 = lIdx2;
   }
   return bRet;
}


//=============================================================================
// BConvexHull::overlapsBox
//=============================================================================
bool BConvexHull::overlapsBox(const float minX, const float minZ, const float maxX, const float maxZ) const
{
   // First check if bounding boxes overlap.
   if(minX > mBoundingMax.x)
      return(false);
   if(minZ > mBoundingMax.z)
      return(false);
   if(maxX < mBoundingMin.x)
      return(false);
   if(maxZ < mBoundingMin.z)
      return(false);

   // Now check if the actual hull overlaps the box.
   // First set up points of box.
   BVector points[4] = {BVector(minX, 0.0f, minZ), BVector(minX, 0.0f, maxZ),
      BVector(maxX, 0.0f, maxZ), BVector(maxX, 0.0f, minZ)};

   // Check if hulls points are inside box.
   for(long i=0; i<mPoints.getNumber(); i++)
   {
      if(mPoints[i].x>=minX && mPoints[i].x<=maxX && mPoints[i].z>=minZ && mPoints[i].z<=maxZ)
         return(true);
   }

   // Check if points are inside the hull.
   for(long i=0; i<4; i++)
   {
      // jce 12/6/2000 -- instead of calling the inside function, we just do a check ourselves here.
      // This makes sense because we've already checked the bounding box, etc.
      bool result = pointInXZProjection(mPoints.getPtr(), mPoints.getNumber(), points[i]);
      if(result)
         return(true);
   }

   // Check segments of box against our segments.
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; startPoint=endPoint, endPoint++)
   {
      bool result = segmentIntersects(points[startPoint], points[endPoint], false);
      if(result)
         return(true);
   }

   // If we got here, the two don't overlap.
   return(false);
}




//=============================================================================
// computeVector
//=============================================================================
inline void computeVector(float x, float z, float x2, float z2, float exp, float &ox, float &oz)
{
   ox=x-x2;
   oz=z-z2;
   float expOverLen=exp/float(sqrt(ox*ox+oz*oz));
   ox*=expOverLen;
   oz*=expOverLen;
}

//=============================================================================
// BConvexHull::expandInto
//=============================================================================
bool BConvexHull::expandInto(float expansion, BDynamicSimVectorArray  &expandedPoints) const
{
   // MPB 10/4/2007 - Modified to work with any hull.  Before it really only
   // worked with quad hulls.  Each side is expanded out by "expansion" amount,
   // so at the actual points it will be more than expansion.  The function
   // ::grow attempts to maintain the same distance all the way around but
   // it adds verts to do this.

   // Size the result array.
   long num = mPoints.getNumber();
   expandedPoints.setNumber(num);

   // Bail if no points.
   if(num<1)
      return(false);

   /*
   // jce 5/30/2001 -- this version doesn't work right (except for squares), but it's pretty fast.
   // Compute the scaling factor.
   float dx = mPoints[0].x - mCenter.x;
   float dz = mPoints[0].z - mCenter.z;
   float distance = (float)sqrt(dx*dx+dz*dz);
   // If the center and this point coincide, bail.
   if(_fabs(distance) < cFloatCompareEpsilon)
      return(false);
   float newDistance = distance+expansion;
   float scale = newDistance/distance;

   // Expand the points.
   float sCx = scale*mCenter.x;
   float sCz = scale*mCenter.z;
   for(long i=0; i<num; i++)
   {
      expandedPoints[i].x = scale*mPoints[i].x - sCx + mCenter.x;
      expandedPoints[i].z = scale*mPoints[i].z - sCz + mCenter.z;
   }
   */

   // jce 5/30/2001 -- this version works but is about twice as slow.
   // Compute vectors along segments.
   static BDynamicSimFloatArray sx;
   static BDynamicSimFloatArray sz;
   sx.setNumber(num);
   sz.setNumber(num);
   //float expOverLen;
   //float tx, tz;
   long j=num-1;
   for(long i=0; i<num; i++)
   {
      /*
      tx=mPoints[i].x-mPoints[j].x;
      tz=mPoints[i].z-mPoints[j].z;
      expOverLen=expansion/float(sqrt(tx*tx+tz*tz));
      sx[i]=tx*expOverLen;
      sz[i]=tz*expOverLen;
      */
      // Just get a normalized vector right now, expansion is calculated below
      computeVector(mPoints[i].x, mPoints[i].z, mPoints[j].x, mPoints[j].z, 1.0f, sx[i], sz[i]);

      j=i;
   }

   // Expand.
   j = 0;
   for (long i = num - 1; i >= 0; i--)
   {
      // Calculate the vector that bisects the line segments exiting point i
      BVector v1(-sx[i], 0.0f, -sz[i]);
      BVector v2(sx[j], 0.0f, sz[j]);
      float halfAngleDiff;

      // Check for colinear points
      if (abs(v1.dot(v2) + 1.0f) < cFloatCompareEpsilon)
         halfAngleDiff = cPiOver2;
      else
         // DLM 2/21/08 - Made this Win32 happy.
         #ifdef XBOX
         halfAngleDiff = XMVector3AngleBetweenNormals(v1, v2).x * 0.5f;
         #else
         halfAngleDiff = v1.angleBetweenVector(v2) * 0.5f;
         #endif

      BVector bisectVec = v2;
      bisectVec.rotateXZ(halfAngleDiff);

      // Distance to move along this vector is the hypoteneuse of the triangle
      // with the halfAngleDiff angle opposite the side of length "expansion"
      float dist = expansion / sinf(halfAngleDiff);

      expandedPoints[i].x = mPoints[i].x - (dist * bisectVec.x);
      expandedPoints[i].z = mPoints[i].z - (dist * bisectVec.z);

      j=i;
   }


   // Debugging
   /*
   for(i=0; i<num; i++)
      expandedPoints[i].y = 0.1f;
   addHullDebugLines("expand", cColorBlue, expandedPoints);
   */


   return(true);
}


//=============================================================================
// BConvexHull::expand
//=============================================================================
bool BConvexHull::expand(float expansion)
{
   // Expand the points.
   bool result = expandInto(expansion, mPoints);
   if(!result)
      return(result);

   // Fix bounding box.
   computeBoundingBox();  

   return(true);
}


//=============================================================================
// BConvexHull::expandFrom
//=============================================================================
bool BConvexHull::expandFrom(float expansion, const BConvexHull &hull)
{
   // MPB 10/4/2007 - Modified to work with any hull.  Before it really only
   // worked with quad hulls.  Each side is expanded out by "expansion" amount,
   // so at the actual points it will be more than expansion.  The function
   // ::grow attempts to maintain the same distance all the way around but
   // it adds verts to do this.

   // jce 8/16/2001 -- for the sake of speediness, this contains fabulously duplicated
   // code from expand, updateBoundingBox, and operator=.

   // Bail if no points.
   long num=hull.getPointCount();
   if(num<1)
   {
      clear();
      return(false);
   }

   // Set number of points.
   bool ok=mPoints.setNumber(num);
   if(!ok)
   {
      clear();
      return(false);
   }


   // Clear min/maxes for bounding box.
   mBoundingMin.x = cMaximumFloat;
   mBoundingMin.y = 0.0f;
   mBoundingMin.z = cMaximumFloat;

   mBoundingMax.x = -cMaximumFloat;
   mBoundingMax.y = 0.0f;
   mBoundingMax.z = -cMaximumFloat;


   // Compute vectors along segments.
   static BDynamicSimFloatArray sx;
   static BDynamicSimFloatArray sz;
   sx.setNumber(num);
   sz.setNumber(num);
   const BVector *hullPts=hull.mPoints.getPtr();
   long j=num-1;
   for(long i=0; i<num; i++)
   {
      // Just get a normalized vector right now, expansion is calculated below
      computeVector(hullPts[i].x, hullPts[i].z, hullPts[j].x, hullPts[j].z, 1.0f, sx[i], sz[i]);
      j=i;
   }

   j = 0;
   for (long i = num - 1; i >= 0; i--)
   {
      // Calculate the vector that bisects the line segments exiting point i
      BVector v1(-sx[i], 0.0f, -sz[i]);
      BVector v2(sx[j], 0.0f, sz[j]);
      float halfAngleDiff;

      // Check for colinear points
      if (abs(v1.dot(v2) + 1.0f) < cFloatCompareEpsilon)
         halfAngleDiff = cPiOver2;
      else
         #ifdef XBOX
         halfAngleDiff = XMVector3AngleBetweenNormals(v1, v2).x * 0.5f;
         #else
         halfAngleDiff = v1.angleBetweenVector(v2) * 0.5f;
         #endif

      BVector bisectVec = v2;
      bisectVec.rotateXZ(halfAngleDiff);

      // Distance to move along this vector is the hypoteneuse of the triangle
      // with the halfAngleDiff angle opposite the side of length "expansion"
      float dist = expansion / sinf(halfAngleDiff);

      mPoints[i].x=hullPts[i].x - (dist * bisectVec.x);
      mPoints[i].y = 0.0f;
      mPoints[i].z=hullPts[i].z - (dist * bisectVec.z);

      // Min/max bbox calculation.
      if(mPoints[i].x < mBoundingMin.x)
         mBoundingMin.x = mPoints[i].x;
      if(mPoints[i].z < mBoundingMin.z)
         mBoundingMin.z = mPoints[i].z;
      if(mPoints[i].x > mBoundingMax.x)
         mBoundingMax.x = mPoints[i].x;
      if(mPoints[i].z > mBoundingMax.z)
         mBoundingMax.z = mPoints[i].z;

      j = i;
   }

   return(true);
}

//=============================================================================
// BConvexHull::combine
//=============================================================================
bool BConvexHull::combine(const BConvexHull& hull)
{
   if(hull.getPointCount() == 0)
      return(false);

   return( addPoints(hull.getPoints().getPtr(), hull.getPointCount()) );
}


//=============================================================================
// BConvexHull::scale
//=============================================================================
void BConvexHull::scale(float scale)
{
   // Scale the points.
   long num = mPoints.getNumber();
   for(long i=0; i<num; i++)
   {
      mPoints[i] -= mCenter;
      mPoints[i] *= scale;
      mPoints[i] += mCenter;
   }

   // Fix bounding box.
   computeBoundingBox();  
}


//=============================================================================
// BConvexHull::bevel
//=============================================================================
void BConvexHull::bevel(float edgeCutPercent)
{
   //-- A bevel of 0 or less is not desirable.
   if (edgeCutPercent <= 0.0f) 
      return;

   //-- A bevel of more than half the edge length is also not desirable.
   if (edgeCutPercent > 0.5f) 
      edgeCutPercent = 0.5f;
   
   //-- Make sure we have enough verts to bevel.
   long numVerts = mPoints.getNumber();
   if (numVerts < 3)
      return;

   //-- Set up our new vert array.
   BDynamicSimVectorArray  newVerts;

   //-- For every existing vert, create 2 new verts.
   BVector forward, backward;
   for (long vert = 0; vert < numVerts; ++vert)
   {
      //-- Get the 2 edges for this vert.
      forward   = (vert == numVerts - 1) ? mPoints[0] : mPoints[vert + 1];
      backward  = (vert == 0) ? mPoints[numVerts - 1] : mPoints[vert - 1];
      forward  -= mPoints[vert];
      backward -= mPoints[vert];

      //-- Add the new verts.
      forward  *= edgeCutPercent;
      backward *= edgeCutPercent;
      forward  += mPoints[vert];
      backward += mPoints[vert];
      newVerts.add(backward);
      newVerts.add(forward);
   }
   
   mPoints = newVerts;
   
   // Fix bounding box.
   computeBoundingBox();  
}


//=============================================================================
//  BConvexHull::grow
//  This is like the expand() function except that this grows all edges out
//  by the amount specified.  This function is safe to use on any convex hull,
//  but it does create more vertices.  expand() is only predictable for hulls
//  where all the points are equidistant from the center, and it doesn't
//  create more verts.
//
//  NOTE:  this function REQUIRES mPoints to be in clockwise order!
//
//=============================================================================
void BConvexHull::grow(float amount)
{
   //-- We cant grow a negative amount, cuz that would cause complexities.
   if (amount <= 0.0f) 
      return;

   //-- Make sure we have enough verts to grow.
   long numVerts = mPoints.getNumber();
   if (numVerts < 3)
      return;

   //-- Set up our new vert array.
   BDynamicSimVectorArray  newVerts;

   //-- For every existing vert, create 3 new verts.
   BVector edge1, edge2, v1, v2, v3;
   float length;
   for (long vert = 0; vert < numVerts; ++vert)
   {
      //-- Get the edge.
      edge1  = (vert == 0) ? mPoints[numVerts - 1] : mPoints[vert - 1];
      edge2  = (vert == numVerts - 1) ? mPoints[0] : mPoints[vert + 1];
      edge1 -= mPoints[vert];
      edge2 -= mPoints[vert];

      //-- Get a perpendicular segment, 90 degress to the right of the edge1.
      v1.x =  edge1.z;
      v1.z = -edge1.x;
      v1.y =  0;
      length = v1.length();
      if (length < cFloatCompareEpsilon)
         return;
      v1 *= (amount / length);

      //-- Get a perpendicular segment, 90 degress to the left of the edge2.
      v2.x = -edge2.z;
      v2.z =  edge2.x;
      v2.y =  0;
      length = v2.length();
      if (length < cFloatCompareEpsilon)
         return;
      v2 *= (amount / length);

      //-- Make avert that bisects v1 and v2 at the proper distance.
      v3  = v1;
      v3 += v2;
      v3 *= 0.5f;
      length = v3.length();
      if (length < cFloatCompareEpsilon)
         return;
      v3 *= (amount / length);

      //-- Add the new verts.
      v1 += mPoints[vert];
      v2 += mPoints[vert];
      v3 += mPoints[vert];
      newVerts.add(v1);
      newVerts.add(v3);
      newVerts.add(v2);
   }
   
   mPoints = newVerts;
   
   // Fix bounding box.
   computeBoundingBox();  
}


//=============================================================================
// BConvexHull::segmentIntersects
//=============================================================================
bool BConvexHull::segmentIntersects(const BVector &point1, const BVector &point2, const long ignorePoint,
   BVector &iPoint, long &segmentIndex, float &distanceSqr, long &lNumIntersections, bool checkBBox,
   bool bCheckInside, float ignoreDistSqr) const
{
   // If the segment can't possibly intersect the bounding box, there can't
   // be any intersection.
   lNumIntersections = 0L;
   if(checkBBox && !segmentMightIntersectBox(point1, point2))
      return(false);

   BVector thisIPoint(cOriginVector);
   distanceSqr = cMaximumFloat;
   segmentIndex = -1;

   // Check each segment
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); startPoint=endPoint, endPoint++)
   {
      // Skip segment if it contains the ignore point (if any)
      if((ignorePoint>=0) && ((ignorePoint==endPoint) || (ignorePoint==startPoint)))
         continue;

      // Check this hull segment.
      long result = segmentIntersect(point1, point2, mPoints[startPoint], mPoints[endPoint], thisIPoint);
      if(result==cIntersection)
      {
         // Ignore the intersection with an endpoint.  We've either already got it,
         // or we'll get it eventually.
         /*
         if ((_fabs(mPoints[endPoint].x - thisIPoint.x) < cFloatCompareEpsilon) &&
             (_fabs(mPoints[endPoint].z - thisIPoint.z) < cFloatCompareEpsilon))
             continue;
         */

         // If we've already got one zero distance intersection
         // Check if this is the closest to point1 so far.
         float dx = point1.x-thisIPoint.x;
         float dz = point1.z-thisIPoint.z;
         float thisDistSqr = dx*dx + dz*dz;

         // Ignore intersections closer than ignore dist.
         if(thisDistSqr<ignoreDistSqr)
            continue;

         // If we already have a zero distance intersection.. ignore any others..
         if (thisDistSqr < cFloatCompareEpsilon && distanceSqr < cFloatCompareEpsilon)
            continue;

         ++lNumIntersections;
         if(thisDistSqr < distanceSqr)
         {
            distanceSqr = thisDistSqr;
            iPoint = thisIPoint;
            segmentIndex = startPoint;
         } 
      }
   }

   // If no intersection found, maybe the whole damn thing is inside the hull?
   if (bCheckInside && segmentIndex == -1)
   {
      if(inside(point1))
      {
         segmentIndex=-1;
         distanceSqr=0.0f;
         iPoint = point1;
         return(true);
      }
   }

   return(segmentIndex>=0);
}


//=============================================================================
// BConvexHull::segmentIntersects
//=============================================================================
bool BConvexHull::segmentIntersects(const BVector &point1, const BVector &point2, bool checkBBox) const
{
   // If the segment can't possibly intersect the bounding box, there can't
   // be any intersection.
   if(checkBBox && !segmentMightIntersectBox(point1, point2))
      return(false);

   BVector iPoint(cOriginVector);

   // Check each segment
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      // Check this hull segment.
      long result = segmentIntersect(point1, point2, mPoints[startPoint], mPoints[endPoint], iPoint);
      if(result==cIntersection)
         return(true);

      startPoint=endPoint;
   }

   return(false);
}


//=============================================================================
// BConvexHull::minMaxSegIntersect
//=============================================================================
long BConvexHull::minMaxSegIntersect(const BVector &point1, const BVector &point2, 
   BVector &minPoint, BVector &maxPoint) const
{
   // Check bbox.
   if(!segmentMightIntersectBox(point1, point2))
      return(cMinMaxNone);

   BVector iPoint(cOriginVector);
   long minSeg=-1;
   long maxSeg=-1;
   float minDistSqr=cMaximumFloat;
   float maxDistSqr=-cMaximumFloat;

   // Check each segment
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      // Check this hull segment.
      long result = segmentIntersect(point1, point2, mPoints[startPoint], mPoints[endPoint], iPoint);
      if(result==cIntersection)
      {
         // Get dist from first point.
         float distSqr=iPoint.distanceSqr(point1);

         // Update min.
         if(distSqr<minDistSqr)
         {
            minDistSqr=distSqr;
            minSeg=endPoint;
            minPoint=iPoint;
         }

         // Update max.
         if(distSqr>maxDistSqr)
         {
            maxDistSqr=distSqr;
            maxSeg=endPoint;
            maxPoint=iPoint;
         }
      }
      startPoint=endPoint;
   }

   // Check for no intersection.
   if(minSeg<0 && maxSeg<0)
      return(cMinMaxNone);

   // Check for single intersection.
   if(minSeg==maxSeg)
   {
      // jce 6/8/2001 -- we need to figure out whether this is an "enter" segment or
      // an "exit" segment.  Super cheesy version for now is to just check this at the last instant
      // instead of figuring it out cleverly as we go.
      if(inside(point1))
         return(cMinMaxExit);

      // Otherwise, assume end point was in there.
      return(cMinMaxEnter);
   }

   // Otherwise, we got two intersections
   return(cMinMaxThrough);
}


//=============================================================================
// BConvexHull::rayIntersects
//=============================================================================
bool BConvexHull::rayIntersects(const BVector &point, const BVector &vector, BVector &iPoint, long *segmentIndex, 
   float *distSqr) const
{
   // If this ray can't possibly hit the bounding box of the hull, we know
   // that there is no intersection.
   if(!rayMightIntersectBox(point, vector))
      return(false);

   BVector thisIPoint(cOriginVector);
   float bestDistSqr = cMaximumFloat;
   bool hit=false;

   // Check each segment
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      // Check this hull segment.
      long result = segmentIntersectRay(mPoints[startPoint], mPoints[endPoint], point, vector, thisIPoint);
      if(result==cIntersection)
      {
         // Check if this is the closest to point so far.
         float dx = point.x-thisIPoint.x;
         float dz = point.z-thisIPoint.z;
         float thisDistSqr = dx*dx + dz*dz;
         if(thisDistSqr < bestDistSqr)
         {
            bestDistSqr = thisDistSqr;
            iPoint = thisIPoint;
            if(segmentIndex)
               *segmentIndex = startPoint;
            hit=true;
         } 
      }
      startPoint=endPoint;
   }

   // Record best distance squared if desired.
   if(distSqr)
      *distSqr = bestDistSqr;

   return(hit);
}


//=============================================================================
// BConvexHull::rayIntersectsSpecial
//=============================================================================
long BConvexHull::rayIntersectsSpecial(const BVector &point, const BVector &vector, const BVector *point2, 
   BVector &iPoint, long *segmentIndex, float *distSqr) const
{
   // If this ray can't possibly hit the bounding box of the hull, we know
   // that there is no intersection.
   if(point2)
   {
      if(!segmentMightIntersectBox(point, *point2))
         return(cSpecialNoIntersection);
   }
   else if(!rayMightIntersectBox(point, vector))
      return(cSpecialNoIntersection);
   
   BVector thisIPoint(cOriginVector);
   float bestDistSqr = cMaximumFloat;
   bool hit=false;
   long result;
   bool coincident=false;
   long intersections=0;

   // Check each segment
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); startPoint=endPoint, endPoint++)
   {
      // Check this hull segment.
      result = segmentIntersectRay(mPoints[startPoint], mPoints[endPoint], point, vector, thisIPoint);
      if(result==cIntersection)
      {
         // Check if this is the closest to point so far.
         float dx = point.x-thisIPoint.x;
         float dz = point.z-thisIPoint.z;
         float thisDistSqr = dx*dx + dz*dz;
         
         // See if we have a zero distance intersection, and skip if we do.
         if(thisDistSqr < cSpecialIntersectionEpsilon)
            continue;

         intersections++;

         if(thisDistSqr < bestDistSqr)
         {
            bestDistSqr = thisDistSqr;
            iPoint = thisIPoint;
            if(segmentIndex)
               *segmentIndex = startPoint;
            hit=true;
         } 
      }
      else if(result == cCoincident)
         coincident=true;
   }

   // Record best distance squared if desired.
   if(distSqr)
      *distSqr = bestDistSqr;

   if(intersections==0)
      return(cSpecialNoIntersection);
   else if(intersections==1 && !coincident)
      return(cSpecialInvalidDirection);
   else
      return(cSpecialIntersection);
}


//=============================================================================
// BConvexHull::rayIntersects
//=============================================================================
bool BConvexHull::rayIntersects(const BVector &point, const BVector &vector) const
{
   // If this ray can't possibly hit the bounding box of the hull, we know
   // that there is no intersection.
   if(!rayMightIntersectBox(point, vector))
      return(false);

   BVector iPoint(cOriginVector);

   // Check each segment
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      // Check this hull segment.
      long result = segmentIntersectRay(mPoints[startPoint], mPoints[endPoint], point, vector, iPoint);
      if(result==cIntersection)
         return(true);
      startPoint=endPoint;
   }

   return(false);
}


//=============================================================================
// BConvexHull::rayCoincidentToEdge
//=============================================================================
bool BConvexHull::rayCoincidentToEdge(const BVector &point, const BVector &vector) const
{
   // If this ray can't possibly hit the bounding box of the hull, we know
   // that there is no intersection.
   if(!rayMightIntersectBox(point, vector))
      return(false);

   BVector iPoint(cOriginVector);

   // Check each segment
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      // Check this hull segment.
      long result = segmentIntersectRay(mPoints[startPoint], mPoints[endPoint], point, vector, iPoint);
      if(result==cCoincident)
         return(true);
      startPoint=endPoint;
   }

   return(false);
}


//=============================================================================
// BConvexHull::insideBox
//=============================================================================
inline bool BConvexHull::insideBox(const BVector &point) const
{
   if(point.x>=mBoundingMin.x && point.x<=mBoundingMax.x && 
      point.z>=mBoundingMin.z && point.z<=mBoundingMax.z)
   {
      return(true);
   }

   return(false);
}


//=============================================================================
// BConvexHull::rayMightIntersectBox
//=============================================================================
inline bool BConvexHull::rayMightIntersectBox(const BVector &point, const BVector &vector) const
{  
   if((vector.x>0.0f) && (point.x > mBoundingMax.x))
      return(false);
   if((vector.x<0.0f) && (point.x < mBoundingMin.x))
      return(false);
   if((vector.z>0.0f) && (point.z > mBoundingMax.z))
      return(false);
   if((vector.z<0.0f) && (point.z < mBoundingMin.z))
      return(false);

   return(true);
}


//=============================================================================
// BConvexHull::segmentMightIntersectBox
//=============================================================================
inline bool BConvexHull::segmentMightIntersectBox(const BVector &point1, const BVector &point2) const
{
   if((point1.x>mBoundingMax.x) && (point2.x>mBoundingMax.x))
      return(false);
   if((point1.x<mBoundingMin.x) && (point2.x<mBoundingMin.x))
      return(false);
   if((point1.z>mBoundingMax.z) && (point2.z>mBoundingMax.z))
      return(false);
   if((point1.z<mBoundingMin.z) && (point2.z<mBoundingMin.z))
      return(false);

   return(true);
}


//=============================================================================
// BConvexHull::getSegmentDir
//=============================================================================
BVector BConvexHull::getSegmentDir(long segmentIndex) const
{
   // wrap out of bounds segments indices
   long numPoints = mPoints.getNumber();
   if(segmentIndex<0)
   {
      segmentIndex += numPoints;
   }

   if(segmentIndex>numPoints)
   {
      segmentIndex -= numPoints;
   }

   // Bail on bogus segment index.
   if(segmentIndex<0 || segmentIndex>numPoints)
   {
      BASSERT(0);
      return(cOriginVector);
   }

   // Get end point (wrapping around if necessary)
   long endPoint = segmentIndex+1;
   if(endPoint >= numPoints)
      endPoint = 0;

   return(mPoints[endPoint]-mPoints[segmentIndex]);
}


//=============================================================================
// BConvexHull::clear
//=============================================================================
void BConvexHull::clear()
{
   mPoints.setNumber(0);

   mBoundingMin.x = cMaximumFloat;
   mBoundingMin.y = 0.0f;
   mBoundingMin.z = cMaximumFloat;

   mBoundingMax.x = -cMaximumFloat;
   mBoundingMax.y = 0.0f;
   mBoundingMax.z = -cMaximumFloat;

   mCenter = cOriginVector;
}


//=============================================================================
// BConvexHull::addPoint
//=============================================================================
bool BConvexHull::addPoint(const BVector &point)
{
   // Make room.
   long num=mPoints.getNumber();
   bool result = mPoints.setNumber(num+1);
   if(!result)
   {
      BASSERT(0);
      return(false);
   }

   // Actually do the add.
   mPoints[num] = point;

   // Check bounding box.
   if(point.x < mBoundingMin.x)
      mBoundingMin.x = point.x;
   if(point.z < mBoundingMin.z)
      mBoundingMin.z = point.z;
   if(point.x > mBoundingMax.x)
      mBoundingMax.x = point.x;
   if(point.z > mBoundingMax.z)
      mBoundingMax.z = point.z;

   return(true);
}


//=============================================================================
// BConvexHull::hullVertex
//=============================================================================
long BConvexHull::hullVertex(const BVector &point, const float epsilon) const 
{
   long num = mPoints.getNumber();
   for(long i=0; i<num; i++)
   {
      // Skip if x or z components do not match.
      if(_fabs(mPoints[i].x-point.x) > epsilon)
         continue;
      if(_fabs(mPoints[i].z-point.z) > epsilon)
         continue;

      // If we got here there was a match.
      return(i);
   }

   // If we got here there was no match found.
   return(-1);
}


//=============================================================================
// BConvexHull::computeCenter
//=============================================================================
void BConvexHull::computeCenter(void)
{
   mCenter = cOriginVector;
   long num = mPoints.getNumber();
   if(num < 1)
      return;

   // Total points
   for(long i=0; i<num; i++)
      mCenter += mPoints[i];

   // Divide by number.
   mCenter /= (float)num;
}


//=============================================================================
// BConvexHull::computeBoundingBox
//=============================================================================
void BConvexHull::computeBoundingBox(void)
{
   // Clear min/maxes.
   mBoundingMin.x = cMaximumFloat;
   mBoundingMin.y = 0.0f;
   mBoundingMin.z = cMaximumFloat;

   mBoundingMax.x = -cMaximumFloat;
   mBoundingMax.y = 0.0f;
   mBoundingMax.z = -cMaximumFloat;

   // Run through points and get new mins/maxes.
   for(long i=0; i<mPoints.getNumber(); i++)
   {
      if(mPoints[i].x < mBoundingMin.x)
         mBoundingMin.x = mPoints[i].x;
      if(mPoints[i].z < mBoundingMin.z)
         mBoundingMin.z = mPoints[i].z;
      if(mPoints[i].x > mBoundingMax.x)
         mBoundingMax.x = mPoints[i].x;
      if(mPoints[i].z > mBoundingMax.z)
         mBoundingMax.z = mPoints[i].z;
   }
}

//=============================================================================
// BConvexHull::calculateArea
//=============================================================================
float BConvexHull::calculateArea(void)
{
   float rVal=calculatePolyArea(mPoints.getPtr(), mPoints.getNumber());
   return(rVal);
}

//=============================================================================
// BConvexHull::distance
//=============================================================================
float BConvexHull::distance(const BVector &point) const
{
   return((float)sqrt(distanceSqr(point)));
}


//=============================================================================
// BConvexHull::distanceSqr
//=============================================================================
float BConvexHull::distanceSqr(const BVector &point) const
{
   long numPoints=mPoints.getNumber();
   if(numPoints<2)
   {
      BASSERT(0);
      return(cMaximumFloat);
   }

   // If inside the hull, range is 0.
   if(inside(point))
      return(0.0f);

   // Get distance to each segment making up the hull and keep the smallest.
   float bestDistSqr = cMaximumFloat;
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      float thisDistSqr = point.xzDistanceToLineSegmentSqr(mPoints[startPoint], mPoints[endPoint]);
      if(thisDistSqr<bestDistSqr)
         bestDistSqr=thisDistSqr;
      startPoint=endPoint;
   }

   return(bestDistSqr);
}


//=============================================================================
// BConvexHull::distance
//=============================================================================
float BConvexHull::distance(const BConvexHull &hull) const
{
   return((float)sqrt(distanceSqr(hull)));
}


//=============================================================================
// BConvexHull::distanceSqr
//=============================================================================
float BConvexHull::distanceSqr(const BConvexHull &hull) const
{
   // Check for overlap.
   if(overlapsHull(hull))
      return(0.0f);

   // Check segments.
   float bestDistSqr = cMaximumFloat;
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      long hullStartPoint = hull.mPoints.getNumber()-1;
      for(long hullEndPoint=0; hullEndPoint<hull.mPoints.getNumber(); hullEndPoint++)
      {
         float thisDistSqr = distanceBetweenSegmentsSqr(mPoints[startPoint].x, mPoints[startPoint].z, 
            mPoints[endPoint].x, mPoints[endPoint].z, hull.mPoints[hullStartPoint].x, hull.mPoints[hullStartPoint].z, 
            hull.mPoints[hullEndPoint].x, hull.mPoints[hullEndPoint].z);
         if(thisDistSqr<bestDistSqr)
            bestDistSqr=thisDistSqr;
         hullStartPoint=hullEndPoint;
      }
      startPoint=endPoint;
   }

   return(bestDistSqr);   
}


//=============================================================================
// BConvexHull::inRange
//=============================================================================
bool BConvexHull::inRange(const BVector &point, const float range) const
{
   long numPoints=mPoints.getNumber();
   if(numPoints<2)
   {
      BASSERT(0);
      return(false);
   }

   // If inside the hull, we are in range.
   if(inside(point))
      return(true);

   // Go through each segment and see if the distance is small enough.
   float rangeSqr = range*range;
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      float thisDistSqr = point.xzDistanceToLineSegmentSqr(mPoints[startPoint], mPoints[endPoint]);
      if(thisDistSqr<rangeSqr)
         return(true);
      startPoint=endPoint;
   }

   return(false);
}


//=============================================================================
// BConvexHull::inRange
//=============================================================================
bool BConvexHull::inRange(const BConvexHull &hull, const float range) const
{
   // Check for overlap.
   if(overlapsHull(hull))
      return(true);

   // Check segments.
   float rangeSqr = range*range;
   long startPoint = mPoints.getNumber()-1;
   for(long endPoint=0; endPoint<mPoints.getNumber(); endPoint++)
   {
      long hullStartPoint = hull.mPoints.getNumber()-1;
      for(long hullEndPoint=0; hullEndPoint<hull.mPoints.getNumber(); hullEndPoint++)
      {
         float thisDistSqr = distanceBetweenSegmentsSqr(mPoints[startPoint].x, mPoints[startPoint].z, 
            mPoints[endPoint].x, mPoints[endPoint].z, hull.mPoints[hullStartPoint].x, hull.mPoints[hullStartPoint].z, 
            hull.mPoints[hullEndPoint].x, hull.mPoints[hullEndPoint].z);
         if(thisDistSqr<=rangeSqr)
            return(true);
         hullStartPoint=hullEndPoint;
      }
      startPoint=endPoint;
   }

   return(false);
}

//==============================================================================
// BConvexHull::operator ==
//==============================================================================
bool BConvexHull::operator ==(const BConvexHull &hull) const
{
   if (mCenter != hull.mCenter)
      return false;

   for (long l = 0; l < mPoints.getNumber(); l++)
   {
      if (mPoints[l] != hull.mPoints[l])
         return false;
   }
   return true;
}

//=============================================================================
// BVector BConvexHull::findClosestPointOnHull
// Finds the closest point on the nonconvex hull to the point in question.
// the vector returned is that point.  The distancSqr value is set with the
// distance squared between that point and the referenced point.
//=============================================================================
BVector BConvexHull::findClosestPointOnHull(const BVector &vStart, long *plSegmentIndex, float *pfClosestDistSqr)
{
   // Copied straight outta compton, er, nonconvexhull.cpp (Xemu, 11/3/00)

   // Find closest point on the concave hull to this location.
   float fBestDistSqr = cMaximumFloat;
   BVector vClosest(0.0f, 0.0f, 0.0f);

   long lNumPoints = mPoints.getNumber();
   long lStartIdx = lNumPoints - 1;
   long lBestIdx = -1L;

   for (long lEndIdx = 0; lEndIdx < lNumPoints; lEndIdx++)
   {
      BVector vTest(cOriginVector);
      float fDistSqr = vStart.xzDistanceToLineSegmentSqr(mPoints[lStartIdx], mPoints[lEndIdx], &vTest);
      if (fDistSqr < fBestDistSqr)
      {
         fBestDistSqr = fDistSqr;
         vClosest = vTest;
         lBestIdx = lStartIdx;
      }
      lStartIdx = lEndIdx;
   }
   if (plSegmentIndex)
      *plSegmentIndex = lBestIdx;

   if (pfClosestDistSqr)
      *pfClosestDistSqr = fBestDistSqr;

   vClosest.y = 0.0f;
   return vClosest;

}


//=============================================================================
// BConvexHull::patherSegmentIntersect
//=============================================================================
long BConvexHull::patherSegmentIntersect(const BVector &start, const BVector &goal, BVector &iPoint, float &distSqr, long &segIndex)
{
   return(patherSegmentIntersectPoly(start, goal, mPoints.getPtr(), mPoints.getNumber(), iPoint, distSqr, segIndex));
}



//=============================================================================
// BConvexHull::overlapsHull
//=============================================================================
bool BConvexHull::overlapsHull(const BConvexHull &hull, float errorEpsilon) const
{
   // Check if bounding boxes overlap, fail now if they don't.
   if(mBoundingMin.x > hull.mBoundingMax.x)
      return(false);
   if(mBoundingMin.z > hull.mBoundingMax.z)
      return(false);
   if(mBoundingMax.x < hull.mBoundingMin.x)
      return(false);
   if(mBoundingMax.z < hull.mBoundingMin.z)
      return(false);
   
   bool result=hullsOverlapXZ(mPoints.getPtr(), mPoints.getNumber(), hull.mPoints.getPtr(), hull.mPoints.getNumber(), errorEpsilon);
   return(result);
}

//==============================================================================
//==============================================================================
bool BConvexHull::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BVector, mPoints, uint16, 1000);
   GFWRITEVECTOR(pStream, mBoundingMin);
   GFWRITEVECTOR(pStream, mBoundingMax);
   GFWRITEVECTOR(pStream, mCenter);
   return true;
}

//==============================================================================
//==============================================================================
bool BConvexHull::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, BVector, mPoints, uint16, 1000);
   GFREADVECTOR(pStream, mBoundingMin);
   GFREADVECTOR(pStream, mBoundingMax);
   GFREADVECTOR(pStream, mCenter);
   return true;
}
