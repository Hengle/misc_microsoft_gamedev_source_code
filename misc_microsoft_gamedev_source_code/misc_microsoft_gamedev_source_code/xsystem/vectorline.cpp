//==============================================================================
// vectorline.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "vectorline.h"

//==============================================================================
// Defines

//==============================================================================
// Static variables


//==============================================================================
// BVectorLine::BVectorLine
//==============================================================================
BVectorLine::BVectorLine() :
   //mPoints doesn't need any ctor args.
   mLength(-1.0f),
   mXZOnly(true),
   mJoinEnds(true),
   mCurrentPoint(-1.0f),
   mCurrentDir(-1.0f),
   mCurrentLength(-1.0f),
   mInternalLength(-1.0f)
{
}

//==============================================================================
// BVectorLine::~BVectorLine
//==============================================================================
BVectorLine::~BVectorLine(void)
{
}

//==============================================================================
// BVectorLine::getPoint
//==============================================================================
const BVector &BVectorLine::getPoint(long index) const
{
   if ((index >= 0) && (index < mPoints.getNumber()))
      return(mPoints[index]);
   return(cOriginVector);
}

//==============================================================================
// BVectorLine::getPointIndex
//==============================================================================
long BVectorLine::getPointIndex(const BVector &p) const
{
   for (long i=0; i < mPoints.getNumber(); i++)
      if (mPoints[i] == p)
         return(i);
   return(-1);
}

//==============================================================================
// BVectorLine::addPointAtStart
//==============================================================================
bool BVectorLine::addPointAtStart(const BVector &p)
{
   return(addPointBefore(0, p));
}

//==============================================================================
// BVectorLine::addPointBefore
//==============================================================================
bool BVectorLine::addPointBefore(long index, const BVector &p)
{
   //This doesn't work unless index is at least less than the number of Points and not less than 0.
   long number=mPoints.getNumber();
   if ((index >= mPoints.getNumber()) || (index < 0))
      return(false);

   //Add one to the size.
   if (mPoints.setNumber(number+1) == false)
      return(false);

   //Shuffle all of the Points after and including the index down one slot and stuff this new one
   //into the index slot.
   for (long i=number; i > index; i--)
      mPoints[i]=mPoints[i-1];
   mPoints[index]=p;

   //Recalculate the length.
   calculateLength();
   return(true);
}

//==============================================================================
// BVectorLine::addPointAfter
//==============================================================================
bool BVectorLine::addPointAfter(long index, const BVector &p)
{
   //This doesn't work unless index is at least less than the number of Points and not less than 0.
   long number=mPoints.getNumber();
   if ((index >= mPoints.getNumber()) || (index < 0))
      return(false);

   //Add one to the size.
   if (mPoints.setNumber(number+1) == false)
      return(false);

   //Shuffle all of the Points after and including the index down one slot and stuff this new one
   //into the index slot.
   for (long i=number; i > (index+1); i--)
      mPoints[i]=mPoints[i-1];
   mPoints[index+1]=p;

   //Recalculate the length.
   calculateLength();
   return(true);
}

//==============================================================================
// BVectorLine::addPointAtEnd
//==============================================================================
bool BVectorLine::addPointAtEnd(const BVector &p)
{
   //Add one to the size.
   long number=mPoints.getNumber();
   if (mPoints.setNumber(number+1) == false)
      return(false);
   mPoints[number]=p;

   //Recalculate the length.
   calculateLength();
   return(true);
}

//==============================================================================
// BVectorLine::addPointAtEnd
//==============================================================================
bool BVectorLine::addPointAtEnd(float x, float y, float z)
{
   BVector foo(x, y, z);
   return(addPointAtEnd(foo));
}

//==============================================================================
// BVectorLine::splitSegment
//==============================================================================
bool BVectorLine::splitSegment(long segmentNumber)
{
   //The segments are numbered using the first Point, so we have to make sure this
   //number is valid not counting our last Point.
   if ((segmentNumber < 0) || (segmentNumber >= mPoints.getNumber()-1))
      return(false);

   BVector diff=mPoints[segmentNumber+1]-mPoints[segmentNumber];
   float diffLength=diff.length();
   diff.normalize();
   diff*=diffLength/2.0f;
   BVector newPoint=mPoints[segmentNumber]+diff;
   return(addPointAfter(segmentNumber, newPoint));
}

//==============================================================================
// BVectorLine::setPoint
//==============================================================================
bool BVectorLine::setPoint(long index, const BVector &p)
{
   if ((index < 0) || (index >= mPoints.getNumber()))
      return(false);
   mPoints[index]=p;

   //Recalculate the length.
   calculateLength();
   return(true);
}

//==============================================================================
// BVectorLine::setPoints
//==============================================================================
bool BVectorLine::setPoints(const BVector *points, long numPoints)
{
   //Bomb check.  We have to have at least two endpoints (a start and a goal).
   if ((points == NULL) || (numPoints < 2))
      return(false);

   //Make sure we have enough room.
   if (mPoints.setNumber(numPoints) == false)
      return(false);

   //If we're here, then we have enough room to copy over the Points.
   for (long i=0; i < numPoints; i++)
      mPoints[i]=points[i];

   //Recalculate the length.
   calculateLength();
   return(true);
}

//==============================================================================
// BVectorLine::removePoint
//==============================================================================
bool BVectorLine::removePoint(long index)
{
   //Bomb check.
   if ((index < 0) || (index >= mPoints.getNumber()))
      return(false);

   //If we're here, shuffle all of the Points after this index up one.
   for (long i=index; i < mPoints.getNumber(); i++)
      mPoints[i]=mPoints[i+1];
   mPoints.setNumber(mPoints.getNumber()-1);

   //Recalculate the length.
   calculateLength();
   return(true);
}

//==============================================================================
// BVectorLine::removePoint
//==============================================================================
bool BVectorLine::removePoint(const BVector &p)
{
   return(removePoint(getPointIndex(p)));
}

//==============================================================================
// BVectorLine::offsetPoints
//==============================================================================
void BVectorLine::offsetPoints(const BVector &o, long startIndex)
{
   if (startIndex < 0)
      startIndex=0;
   for (long i=startIndex; i < mPoints.getNumber(); i++)
      mPoints[i]+=o;

   //Recalculate the length.
   calculateLength();
}


//==============================================================================
// BVectorLine::getSegmentDir
//==============================================================================
void BVectorLine::getSegmentDir(long segmentIndex, BVector& direction, bool normalize) const
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
      direction = cOriginVector;
      return;
   }

   // Get end point (wrapping around if necessary)
   long endPoint = segmentIndex+1;
   if(endPoint >= numPoints)
      endPoint = 0;

   direction = mPoints[endPoint];
   direction -= mPoints[segmentIndex];

   if(normalize)
      direction.normalize();
}

//==============================================================================
// BVectorLine::getDirection
//
// Given to valid segment ids, get the direction from start to end
//==============================================================================
void BVectorLine::getDirection(long segmentStart, long segmentEnd, BVector& direction, bool normalize) const
{
   long delta = segmentEnd - segmentStart;
   if(delta <= 0)
   {
      BASSERT(0);
      direction = cOriginVector;
      return;
   }

   // wrap out of bounds segments indices
   long numPoints = mPoints.getNumber();
   if(segmentStart<0)
   {
      segmentStart += numPoints;
   }

   if(segmentStart>numPoints)
   {
      segmentStart -= numPoints;
   }

   // Bail on bogus segment index.
   if(segmentStart<0 || segmentStart>numPoints)
   {
      BASSERT(0);
      direction = cOriginVector;
      return;
   }

   // Get end point (wrapping around if necessary)
   long endPoint = segmentStart+delta;
   if(endPoint >= numPoints)
      endPoint = 0;

   direction = mPoints[endPoint];
   direction -= mPoints[segmentStart];

   if(normalize)
      direction.normalize();
}
//==============================================================================
// BVectorLine::startFollow
//==============================================================================
bool BVectorLine::startFollow(float length)
{
   if (mPoints.getNumber() <= 0)
      return(false);

   mCurrentPoint=mPoints[0];
   mCurrentDir=cOriginVector;
   mCurrentLength=0.0f;
   mInternalLength=0.0f;
   return(moveForward(length));
}

//==============================================================================
// BVectorLine::moveForward
//==============================================================================
bool BVectorLine::moveForward(float length)
{
   //Do the simple calc to see where the new point will go.  We use internal length
   //here as that just tracks our progress along the line.
   float newLength=mInternalLength+length;

   //If we don't have joined ends, we can go up to the last point.  If that's
   //not far enough, we stop there and return false.
   if (mJoinEnds == false)
   {
      //If we're going to go to the end, we set the point to the last point and
      //set current length to the total length.
      if (newLength >= mLength)
      {
         // mCurrentDir -- direction will stay the same as it was last time
         mCurrentPoint=mPoints[mPoints.getNumber()-1];
         mCurrentLength=mLength;
         mInternalLength=mLength;
         return(false);
      }
      //Else, we set the point to the proper spot and set current length to the
      //new one-way position we calc'ed above.
      if (setCurrentPoint(newLength) == false)
         return(false);
      mCurrentLength=newLength;
   }
   //Else, we loop around
   else
   {
      //Since we loop, we need to make sure that newLength is in the range of our
      //actual length.
      while (newLength > mLength)
         newLength-=mLength;
      //Once it is, we set the current point and ADD the length we moved to the 
      //current length (so that users can track the progress of current length w/o
      //worrying about the wrapping).
      if (setCurrentPoint(newLength) == false)
         return(false);
      mCurrentLength+=length;
   }

   return(true);
}

//==============================================================================
// BVectorLine::cleanUp
//==============================================================================
void BVectorLine::cleanUp(void)
{
   mPoints.setNumber(0);
   mLength=-1.0f;
   mXZOnly=true;
   mCurrentDir.set(-1.0f, -1.0f, -1.0f);
   mCurrentPoint.set(-1.0f, -1.0f, -1.0f);
   mCurrentLength=-1.0f;
   mInternalLength=-1.0f;
}

//==============================================================================
// BVectorLine::setCurrentPoint
//==============================================================================
bool BVectorLine::setCurrentPoint(float totalLength)
{
   if (totalLength > mLength)
      return(false);

   // If only one point, just return it.
   if(mPoints.getNumber()==1)
   {
      mCurrentDir=cOriginVector;
      mCurrentPoint=mPoints[0];
      return(true);
   }


   float l=0.0f;
   long limit=mPoints.getNumber()-1;
   if (mJoinEnds == true)
      limit+=1;
   for (long i=0; i < limit; i++)
   {
      BVector seg;
      if ((mJoinEnds == true) && (i >= limit-1))
         seg=mPoints[0]-mPoints[limit-1];
      else
         seg=mPoints[i+1]-mPoints[i];

      if (mXZOnly == true)
         seg.y=0.0f;
      float segLength=seg.length();
      if (l+segLength < totalLength)
      {
         l+=segLength;
         continue;
      }

      seg.normalize();
      mCurrentDir = seg;
      seg*=totalLength-l;
      mCurrentPoint=mPoints[i];
      mCurrentPoint+=seg;
      mInternalLength=totalLength;
      return(true);
   }

   return(false);
}

//==============================================================================
// BVectorLine::calculateLength
//==============================================================================
float BVectorLine::calculateLength(void)
{
   //Add up the current length of the entire path.
   mLength=0.0f;
   for (long i=0; i < mPoints.getNumber()-1; i++)
   {
      BVector diff=mPoints[i+1]-mPoints[i];
      if (mXZOnly == true)
         diff.y=0.0f;
      mLength+=diff.length();
   }
   if (mJoinEnds == true)
   {
      BVector diff=mPoints[0]-mPoints[mPoints.getNumber()-1];
      if (mXZOnly == true)
         diff.y=0.0f;
      mLength+=diff.length();
   }
   return(mLength);
}


//==============================================================================
// eof: vectorline.cpp
//==============================================================================
