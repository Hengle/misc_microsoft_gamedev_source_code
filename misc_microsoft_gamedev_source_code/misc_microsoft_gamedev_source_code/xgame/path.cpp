//==============================================================================
// Copyright (c) 1997 Ensemble Studios
//
// Path Stuff
//==============================================================================

//==============================================================================
//Includes
#include "common.h"
#include "path.h"
#include "entity.h"
#include "debugprimitives.h"
//#include "debugprimrender.h"
#include "game.h"
#include "world.h"
#include "terrainsimrep.h"
//#include "terrainbase.h"
//#include "syncmacros.h"
#include "mathutil.h"

//#define DEBUGNEXTPATROLWAYPOINT

GFIMPLEMENTVERSION(BPath,1);
enum
{
   cSaveMarkerPath1=10000,
};

//==============================================================================
// BPath::getPathResultName - returns the string that corresponds to the result value
//==============================================================================
const char *BPath::getPathResultName(int nPathResult)
{
   static const char *PathResultNames[10] =
   {
      "cNone",
      "cError",
      "cFailed",
      "cInRangeAtStart",
      "Partial",
      "Full",
      "cOutsideHullAreaFailed",
      "cOutsideHullAreaPartial",
      "cReachedHull",
      "FailedOutOfTime"
   };
   if (nPathResult < -1 || nPathResult > 8)
      return "Unknown";

   return PathResultNames[nPathResult + 1];
}

//==============================================================================
// BPath::BPath
//==============================================================================
BPath::BPath(long numberFlags/* = cNumberPathFlags*/) :
   //mWaypoints doesn't need any ctor args.
   mPathLength(0.0f),
   mCreationTime((DWORD)0)
   //mFlags doesn't need any ctor args.
{
   if(numberFlags<cNumberPathFlags)
   {
      BFAIL("Trying to decrease path flags.");
      numberFlags = cNumberPathFlags;
   }
   mFlags = cLinear | cGoingForward;
}

//==============================================================================
// BPath::~BPath
//==============================================================================
BPath::~BPath(void)
{
}

//==============================================================================
// BPath::operator=
//==============================================================================
const BPath& BPath::operator=(const BPath& path)
{
   if (this != &path)
   {
      mWaypoints = path.mWaypoints;
      mFlags = path.mFlags;
      mPathLength = path.mPathLength;
      mCreationTime = path.mCreationTime;
   }
   return (*this);
}

//==============================================================================
// BPath::getWaypoint
//==============================================================================
const BVector &BPath::getWaypoint(long wpIndex) const
{
   if ((wpIndex >= 0) && (wpIndex < mWaypoints.getNumber()))
      return(mWaypoints[wpIndex]);
   return(cOriginVector);
}

//==============================================================================
// BPath::getWaypointIndex
//==============================================================================
long BPath::getWaypointIndex(const BVector& wp) const
{
   for (long i=0; i < mWaypoints.getNumber(); i++)
      if (mWaypoints[i] == wp)
         return(i);
   return(-1);
}

//==============================================================================
// BPath::addWaypointAtStart
//==============================================================================
bool BPath::addWaypointAtStart(const BVector& wp)
{
   //Add one to the size.
   long number=mWaypoints.getNumber();
   if (mWaypoints.setNumber(number+1) == false)
      return(false);

   //Shuffle all of the waypoints down one slot and stuff this new one
   //into that first slot.
   for (long i=number; i >= 1; i--)
      mWaypoints[i]=mWaypoints[i-1];
   mWaypoints[0]=wp;

   return(true);
}

//==============================================================================
// BPath::addWaypointBefore
//==============================================================================
bool BPath::addWaypointBefore(long wpIndex, const BVector& wp)
{
   //This doesn't work unless wpIndex is at least less than the number of waypoints and not less than 0.
   long number=mWaypoints.getNumber();
   if ((wpIndex >= mWaypoints.getNumber()) || (wpIndex < 0))
      return(false);

   //Add one to the size.
   if (mWaypoints.setNumber(number+1) == false)
      return(false);

   //Shuffle all of the waypoints after and including the index down one slot and stuff this new one
   //into the index slot.
   for (long i=number; i > wpIndex; i--)
      mWaypoints[i]=mWaypoints[i-1];
   mWaypoints[wpIndex]=wp;

   return(true);
}

//==============================================================================
// BPath::addWaypointAfter
//==============================================================================
bool BPath::addWaypointAfter(long wpIndex, const BVector& wp)
{
   //This doesn't work unless wpIndex is at least less than the number of waypoints and not less than 0.
   long number=mWaypoints.getNumber();
   if ((wpIndex >= mWaypoints.getNumber()) || (wpIndex < 0))
      return(false);

   //Add one to the size.
   if (mWaypoints.setNumber(number+1) == false)
      return(false);

   //Shuffle all of the waypoints after and including the index down one slot and stuff this new one
   //into the index slot.
   for (long i=number; i > (wpIndex+1); i--)
      mWaypoints[i]=mWaypoints[i-1];
   mWaypoints[wpIndex+1]=wp;

   return(true);
}

//==============================================================================
// BPath::addWaypointAtEnd
//==============================================================================
bool BPath::addWaypointAtEnd(const BVector& wp)
{
   //Add one to the size.
   long number=mWaypoints.getNumber();
   if (mWaypoints.setNumber(number+1) == false)
      return(false);
   mWaypoints[number]=wp;

   return(true);
}

//==============================================================================
// BPath::addWaypointsAtEnd
//==============================================================================
bool BPath::addWaypointsAtEnd(const BDynamicVectorArray& wpArray)
{
   // Add the waypoints
   long oldNumber = mWaypoints.getNumber();
   if (mWaypoints.setNumber(oldNumber + wpArray.getNumber()) == false)
      return(false);

   long wpIndex;
   for (wpIndex = 0; wpIndex < wpArray.getNumber(); wpIndex++)
   {
      mWaypoints[oldNumber + wpIndex] = wpArray[wpIndex];
   }

   return(true);
}

//==============================================================================
// BPath::splitSegment
//==============================================================================
bool BPath::splitSegment(long segmentNumber)
{
   //The segments are number using the first waypoint, so we have to make sure this
   //number is valid not counting our last waypoint.
   if ((segmentNumber < 0) || (segmentNumber >= mWaypoints.getNumber()-1))
      return(false);

   BVector diff=mWaypoints[segmentNumber+1]-mWaypoints[segmentNumber];
   float diffLength=diff.length();
   diff.normalize();
   diff*=diffLength/2.0f;
   BVector newWaypoint=mWaypoints[segmentNumber]+diff;
   return(addWaypointAfter(segmentNumber, newWaypoint));
}

//==============================================================================
// BPath::setWaypoint
//==============================================================================
bool BPath::setWaypoint(long wpIndex, const BVector& wp)
{
   if ((wpIndex < 0) || (wpIndex >= mWaypoints.getNumber()))
      return(false);
   mWaypoints[wpIndex]=wp;

   return(true);
}

//==============================================================================
// BPath::setWaypoints
//==============================================================================
bool BPath::setWaypoints(const BVector* wps, long numWps)
{
   //Bomb check.  We have to have at least two endpoints (a start and a goal).
   if ((wps == NULL) || (numWps < 2))
      return(false);

   //Make sure we have enough room.
   if (mWaypoints.setNumber(numWps) == false)
      return(false);

   //If we're here, then we have enough room to copy over the waypoints.
   for (long i=0; i < numWps; i++)
      mWaypoints[i]=wps[i];
   return(true);
}

//==============================================================================
// BPath::removeWaypoint
//==============================================================================
bool BPath::removeWaypoint(long wpIndex)
{
   //Bomb check.
   if ((wpIndex < 0) || (wpIndex >= mWaypoints.getNumber()))
      return(false);

   //If we're here, shuffle all of the waypoints after this index up one.
   for (long i=wpIndex; i < mWaypoints.getNumber()-1; i++)
      mWaypoints[i]=mWaypoints[i+1];
   mWaypoints.setNumber(mWaypoints.getNumber()-1);
   return(true);
}

//==============================================================================
// BPath::removeWaypoint
//==============================================================================
bool BPath::removeWaypoint(const BVector& wp)
{
   return(removeWaypoint(getWaypointIndex(wp)));
}

//==============================================================================
// BPath::offsetWaypoints
//==============================================================================
void BPath::offsetWaypoints(const BVector& o, long startIndex)
{
   if (startIndex < 0)
      startIndex=0;
   for (long i=startIndex; i < mWaypoints.getNumber(); i++)
      mWaypoints[i]+=o;
}

//==============================================================================
// BPath::calculatePathLength
//==============================================================================
float BPath::calculatePathLength(bool ignoreY)
{
   //Add up the current length of the entire path.
   setFlag(cLengthValid, true);
   mPathLength=0.0f;
   long lNum = mWaypoints.getNumber();
   if (lNum < 1)
   {
      return 0.0f;
   }

   for (long i=0; i < lNum - 1; i++)
   {
      BVector diff=mWaypoints[i+1]-mWaypoints[i];
      if (ignoreY == true)
         diff.y=0.0f;
      mPathLength+=diff.length();
   }
   return(mPathLength);
}

//==============================================================================
//==============================================================================
float BPath::calculatePathLengthConst(bool ignoreY) const
{
   //Add up the current length of the entire path.
   float foo=0.0f;
   long lNum = mWaypoints.getNumber();
   if (lNum < 1)
      return 0.0f;

   for (long i=0; i < lNum - 1; i++)
   {
      BVector diff=mWaypoints[i+1]-mWaypoints[i];
      if (ignoreY == true)
         diff.y=0.0f;
      foo+=diff.length();
   }
   return(foo);
}

//==============================================================================
// BPath::calculateRemainingDistance
//==============================================================================
float BPath::calculateRemainingDistance(const BVector& point, long currentWaypoint, bool ignoreY) const
{
   if ((currentWaypoint < 0) || (currentWaypoint >= mWaypoints.getNumber()))
      return(0.0f);

   //Calc the distance between the point and the current waypoint.
   BVector diff=mWaypoints[currentWaypoint]-point;
   if (ignoreY == true)
      diff.y=0.0f;
   float rVal=diff.length();

   //Loop through the rest of the waypoints to calc their distance.
   for (long i=currentWaypoint+1; i < mWaypoints.getNumber(); i++)
   {
      diff=mWaypoints[i]-mWaypoints[i-1];
      if (ignoreY == true)
         diff.y=0.0f;
      rVal+=diff.length();
   }

   return(rVal);
}

//==============================================================================
// BPath::calculateDistanceFromEnd
//==============================================================================
float BPath::calculateDistanceFromEnd(const BVector& point, bool ignoreY) const
{
   return(calculateRemainingDistance(point, mWaypoints.getNumber()-1, ignoreY));
}

//==============================================================================
// BPath::calculatePointAlongPath
//==============================================================================
bool BPath::calculatePointAlongPath(float distance, long startWaypoint, BVector& newPoint) const
{
   newPoint = cInvalidVector;

   if( (startWaypoint<0) || (startWaypoint >= mWaypoints.getNumber()))
      return(false);


   BVector diff=cInvalidVector;
   float currentDistance=0.0f;
   float length = 0.0f;
   for(long i=startWaypoint+1; i<mWaypoints.getNumber(); i++)
   {
      diff=mWaypoints[i]-mWaypoints[i-1];
      diff.y=0.0f;

      length = diff.length();
      if(length < cFloatCompareEpsilon)
         continue;

      currentDistance += length;
      if(currentDistance > distance)
      {
         diff*=((currentDistance-distance)/length);
         newPoint=mWaypoints[i]-diff;
         return(true);
      }
   }

   return(false);
}

//==============================================================================
// BPath::calculatePointAlongPath
//==============================================================================
bool BPath::calculatePointAlongPath(float distance, const BVector& startPos, long nextWaypoint, BVector& newPoint, float& realDistance, long& newNextWaypoint) const
{
   newPoint = cInvalidVector;
   realDistance = 0.0f;
   newNextWaypoint = nextWaypoint;

   if ((nextWaypoint < 0) || (nextWaypoint >= mWaypoints.getNumber()))
      return false;

   BVector prevWaypointPos = startPos;             // Previous waypoint for first loop is the start pos
   BVector diff = cInvalidVector;
   float currentDistance = 0.0f;
   float length = 0.0f;
   for (long i = nextWaypoint; i < mWaypoints.getNumber(); i++)
   {
      diff = mWaypoints[i] - prevWaypointPos;
      diff.y = 0.0f;

      // Advance previous waypoint for next loop
      prevWaypointPos = mWaypoints[i];

      length = diff.length();
      if (length < cFloatCompareEpsilon)
         continue;

      currentDistance += length;
      if (currentDistance > distance)
      {
         diff *= (1 / length);
         diff *= (currentDistance - distance);
         newPoint = mWaypoints[i] - diff;
         realDistance = distance;
         newNextWaypoint = i;
         return true;
      }
   }

   // Traveled beyond end of path so return last waypoint
   if (mWaypoints.getNumber() > 0)
      newPoint = mWaypoints[mWaypoints.getNumber() - 1];
   realDistance = currentDistance;
   newNextWaypoint = mWaypoints.getNumber();

   return true;
}

//==============================================================================
//==============================================================================
bool BPath::calculatePointAlongPath(float distance, const BVector& startPos, long nextWaypoint,
   BDynamicVectorArray& newPoints, float& realDistance) const
{
   newPoints.setNumber(0);
   realDistance = 0.0f;
   if ((nextWaypoint < 0) || (nextWaypoint >= mWaypoints.getNumber()))
      return false;

   BVector tempPoint;
   BVector prevWaypointPos = startPos;             // Previous waypoint for first loop is the start pos
   BVector diff = cInvalidVector;
   float currentDistance = 0.0f;
   float length = 0.0f;
   for (long i = nextWaypoint; i < mWaypoints.getNumber(); i++)
   {
      diff = mWaypoints[i] - prevWaypointPos;
      diff.y = 0.0f;

      // Advance previous waypoint for next loop
      prevWaypointPos = mWaypoints[i];

      length = diff.length();
      currentDistance += length;
      if (currentDistance > distance)
      {
         diff *= (1 / length);
         diff *= (currentDistance - distance);
         tempPoint = mWaypoints[i] - diff;
         realDistance = distance;
         newPoints.add(tempPoint);
         return true;
      }
      else
         newPoints.add(mWaypoints[i]);
   }

   realDistance = currentDistance;

   return true;
}

//==============================================================================
// BPath::calculatePointAlongPathFromEnd
//==============================================================================
bool BPath::calculatePointAlongPathFromEnd(float distance, BVector& newPoint) const
{
   newPoint = cInvalidVector;

   BVector diff=cInvalidVector;
   float currentDistance=0.0f;
   float length = 0.0f;
   for(long i=mWaypoints.getNumber()-1; i>0; i--)
   {
      diff=mWaypoints[i]-mWaypoints[i-1];
      diff.y=0.0f;

      length = diff.length();
      if(length < cFloatCompareEpsilon)
         continue;

      currentDistance += length;
      if(currentDistance > distance)
      {
         diff*=((currentDistance-distance)/length);
         newPoint=mWaypoints[i-1]+diff;
         return(true);
      }
   }

   return(false);
}

//=============================================================================
// BPath::findBestStartWaypointIndex
//=============================================================================
long BPath::findBestStartWaypointIndex(const BVector& location) const
{
   //--JER
   //-- This will find the waypoint that is closest to the location.
   long numWayPts = this->mWaypoints.getNumber();
   float distSqr = 0.0f;
   float bestDistSqr = cMaximumFloat;
   long bestIndex = 0;
   for(long i=0; i<numWayPts; i++)
   {
      distSqr = location.xzDistanceSqr(mWaypoints[i]);
      if(distSqr < bestDistSqr)
      {
         bestDistSqr = distSqr;
         bestIndex = i;
      }
   }

   //-- if we are at the end, just return that.
   if(bestIndex == (numWayPts-1))
      return(bestIndex);

   //-- if the (location to bestIndex+1) is shorter than (bestIndex to bestIndex+1), than bestIndex+1 is where we want to start.
   float distSqrBestToNext = mWaypoints[bestIndex].xzDistanceSqr(mWaypoints[bestIndex+1]);
   float distSqrLocToNext = location.xzDistanceSqr(mWaypoints[bestIndex+1]);
   if(distSqrLocToNext < distSqrBestToNext)
      bestIndex++;
   
   return(bestIndex);
}


//==============================================================================
//==============================================================================
BVector BPath::findClosestPoint(const BVector p) const
{
   // Sanity.
   if(mWaypoints.getNumber() < 2)
   {
      BFAIL("Trying to work on path with <2 waypoints");
      return(cOriginVector);
   }
   
   // Nothing so far.
   BVector closestPoint = cOriginVector;
   float closestDistSqr = cMaximumFloat;
   
   // Run through segments.
   BVector waypointA = mWaypoints[0];
   for(long i=1; i<mWaypoints.getNumber(); i++)
   {
      // Get second waypoint.
      BVector waypointB = mWaypoints[i];
      
      // Get closest point on segment.
      float closestX, closestZ;
      closestPointOnSegment(p.x, p.z, waypointA.x, waypointA.z, waypointB.x, waypointB.z, closestX, closestZ);
      
      // Get distance.
      float dx = p.x - closestX;
      float dz = p.z - closestZ;
      float distSqr = dx*dx + dz*dz;
      
      // Is is the best so far?
      if(distSqr < closestDistSqr)
      {
         // Save it.
         closestDistSqr = distSqr;
         closestPoint.x = closestX;
         closestPoint.z = closestZ;
      }
      
      // Last waypoint of this segment is now first waypoint for next one.
      waypointA = waypointB;
   }
   
   // Hand it back to caller.
   return(closestPoint);
}


//==============================================================================
// BPath::getNextPatrolWaypoint
//==============================================================================
long BPath::getNextPatrolWaypoint(long currentWaypoint, bool modifyPath)
{
   if (getFlag(cPatrol) == false)
      return(-1);
   #ifdef DEBUGNEXTPATROLWAYPOINT
   blog("  NPW: cW=%d, modifyPath=%d.", currentWaypoint, modifyPath);
   blog("    Linear=%d, GoingForward=%d.", getFlag(cLinear), getFlag(cGoingForward));
   #endif

   //If we're going linear, we go all the way to one end and then go backwards
   //through the points.
   if (getFlag(cLinear) == true)
   {
      if (getFlag(cGoingForward) == true)
      {
         //If adding one to the current waypoint still leaves us with waypoints
         //in this direction, do that.
         if ((currentWaypoint+1) < mWaypoints.getNumber())
            return(currentWaypoint+1);
         //Else, return the next to last waypoint.
         if (modifyPath == true)
            setFlag(cGoingForward, false);
         return(mWaypoints.getNumber()-2);
      }
      else
      {
         //Same thing, but reversed.
         if (currentWaypoint-1 >= 0)
            return(currentWaypoint-1);
         if (modifyPath == true)
            setFlag(cGoingForward, true);
         return(1);
      }
   }
   //If we're going circular, we go forward through the points and jump from
   //the last point back to the first point.
   else
   {
      if ((currentWaypoint+1) < mWaypoints.getNumber())
         return(currentWaypoint+1);
      return(0);
   }
}

//==============================================================================
// BPath::debugList
//==============================================================================
void BPath::debugList(BEntity* e)
{

#if 0
   if (e == NULL)
      return;
   e->debug("        %d waypoints, length=%f, creationTime=%d, patrol=%d, linear=%d:", mWaypoints.getNumber(), mPathLength, mCreationTime, getFlag(cPatrol), getFlag(cLinear));
   for (long i=0; i < mWaypoints.getNumber(); i++)
      e->debug("          WP[%2d] (%f, %f, %f).", i, mWaypoints[i].x, mWaypoints[i].y, mWaypoints[i].z);
      
#endif
}

//==============================================================================
// BPath::sync
//==============================================================================
void BPath::sync(void)
{

#if 0
   syncEntityMovementData("        num waypoints = ", mWaypoints.getNumber());
   syncEntityMovementData("        mPathLength = ", mPathLength);
   syncEntityMovementData("        mCreationTime = ", mCreationTime);

   for (long i=0; i < mWaypoints.getNumber(); i++)
      syncEntityMovementData("        waypoint = ", mWaypoints[i]);
      
#endif
}

//==============================================================================
// BPath::render
//==============================================================================
bool BPath::render(DWORD lineColor, DWORD pointColor, bool drawOverTerrain)
{

   //Bomb check.
   if (mWaypoints.getNumber() == 0)
      return(false);
   //Run through all of the waypoints to draw connection lines and the waypoints.
   for (long i=0; i < mWaypoints.getNumber()-1; i++)
   {
      if (drawOverTerrain == true)
      {
         gTerrainSimRep.addDebugLineOverTerrain(mWaypoints[i], mWaypoints[i+1], lineColor, lineColor, 0.75f, BDebugPrimitives::cCategoryPathing);
         gTerrainSimRep.addDebugThickCircleOverTerrain(mWaypoints[i], 0.5f, 0.1f, pointColor, 0.75f, BDebugPrimitives::cCategoryPathing);
      
      }
      else
      {
         //BDebugPrimRender::drawDebugLine(mWaypoints[i], mWaypoints[i+1], lineColor, lineColor);
         //BDebugPrimRender::drawDebugPoint(mWaypoints[i], 0.1f, pointColor);
      }
   }
   //Draw the last waypoint.
   if (drawOverTerrain == true)
      gTerrainSimRep.addDebugThickCircleOverTerrain(mWaypoints[mWaypoints.getNumber()-1], 0.5f, 0.1f, pointColor, 0.75f, BDebugPrimitives::cCategoryPathing);
   //else
   //   BDebugPrimRender::drawDebugPoint(mWaypoints[mWaypoints.getNumber()-1], 0.1f, pointColor);*/
  
   return(true);
}

//==============================================================================
//==============================================================================
bool BPath::getFlag( long n ) const
{
   long tempFlags = (long) mFlags;
   tempFlags &= n;
   return (tempFlags != 0);
}

//==============================================================================
//==============================================================================
void BPath::setFlag( long n, bool v )
{
   uchar flag = (uchar) n;
   if (v)
      mFlags |= flag;
   else
      mFlags &= ~flag;
}

//==============================================================================
// BPath::reset
//==============================================================================
void BPath::reset(bool releaseMemory /*=false*/)
{
   // jce [10/7/2008] -- new releaseMemory param will force us to deallocate instead of just changing active count of waypoints.
   if(releaseMemory)
      mWaypoints.clear();
   else
      mWaypoints.setNumber(0);
   
   mPathLength=0.0f;
   mCreationTime=(DWORD)0;
   mFlags = cLinear | cGoingForward;
}

//==============================================================================
//==============================================================================
bool BPath::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTORARRAY(pStream, mWaypoints, uint16, 10000);
   GFWRITEVAR(pStream, uchar, mFlags);
   GFWRITEVAR(pStream, float, mPathLength);
   GFWRITEVAR(pStream, DWORD, mCreationTime);

   GFWRITEMARKER(pStream, cSaveMarkerPath1);
   return true;
}

//==============================================================================
//==============================================================================
bool BPath::load(BStream* pStream, int saveType)
{
   GFREADVECTORARRAY(pStream, mWaypoints, uint16, 10000);

   if (BSaveGame::mGameFileVersion >= 14)
   {
      GFREADVAR(pStream, uchar, mFlags);
   }
   else
   {
      static BBitArray oldFlags;
      oldFlags.setNumber(7);
      oldFlags.clear();
      GFREADBITARRAY(pStream, oldFlags, uint8, 16);

      // Map old flag values to new ones
      setFlag(cPatrol, (oldFlags.isBitSet(0) > 0));
      setFlag(cLinear, (oldFlags.isBitSet(1) > 0));
      setFlag(cGoingForward, (oldFlags.isBitSet(2) > 0));
      setFlag(cIgnoredUnits, (oldFlags.isBitSet(3) > 0));
      setFlag(cIgnoredPassability, (oldFlags.isBitSet(4) > 0));
      setFlag(cLengthValid, (oldFlags.isBitSet(5) > 0));
      setFlag(cJump, (oldFlags.isBitSet(6) > 0));
   }

   GFREADVAR(pStream, float, mPathLength);
   GFREADVAR(pStream, DWORD, mCreationTime);

   GFREADMARKER(pStream, cSaveMarkerPath1);
   return true;
}
