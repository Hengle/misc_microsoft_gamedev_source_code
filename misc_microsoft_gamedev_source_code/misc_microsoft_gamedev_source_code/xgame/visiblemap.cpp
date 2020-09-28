//=============================================================================
// Copyright (c) 2006 Ensemble Studios
//
// Visible map
//=============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "common.h"
#include "visiblemap.h"
#include "world.h"
#include "TerrainSimRep.h"
#include "nonconvexhull2.h"
#include "convexhull.h"
#include "render.h"
#include "configsgame.h"
#include "tilespanlist.h"
#include "gamedirectories.h"
#include "math\generalVector.h"
#include "usermanager.h"
#include "user.h"
#include "flashminimaprenderer.h"

GFIMPLEMENTVERSION(BVisibleMap, 1);

//==============================================================================
// Definitions
//==============================================================================
#define MIN_RADIUS      1
//#define MAX_RADIUS      BVISIBLEMAP_MAX_RADIUS
#define MAX_FIXED_RADIUS  128
#define MAX_MOVEABLE_RADIUS 32
#define cBlackValue     ((1 << cRefCountNumBits) - 1)
#define cFogValue       0
#define cSentinelValue  (cBlackValue - 1)
#define cClearValue     0x00000000
#define CACHE_LINES     3

const long cMagic = 0x74fcae49;


//==============================================================================
// Globals
//==============================================================================
BVisibleMap gVisibleMap;

//==============================================================================
// Constructors/Destructors
//==============================================================================
BVisibleMap::BVisibleMap() :
   mNumXTiles(-1),
   mNumZTiles(-1),
   mTileScale(-1.0f),
   mRecTileScale(-1.0f),
   mMap(NULL)
{
}

BVisibleMap::~BVisibleMap()
{
}

//==============================================================================
// Initializer/Deinitializer
//==============================================================================
bool BVisibleMap::setup()
{
   TRACEMEM
   
   DWORD startTime = GetTickCount();
   trace("BVisibleMap::setup: begin");
         
   // Build span lists
   long numFixedLists = MAX_FIXED_RADIUS - MIN_RADIUS + 1;
   long numMoveableLists = MAX_MOVEABLE_RADIUS - MIN_RADIUS + 1;

   mEntireRadiusLists      = new BTileSpanList[numFixedLists];
   mAddPartialRadiusLists  = new BTileSpanList*[8];
   mDelPartialRadiusLists  = new BTileSpanList*[8];

   for (long dir = 0; dir < 8; dir++)
   {
      mAddPartialRadiusLists[dir] = NULL;
      mDelPartialRadiusLists[dir] = NULL;
   }

   if(!gConfig.isDefined(cConfigNoVismap))
   {
#ifdef BUILD_DEBUG
      bool loaded=false;
      if(gConfig.isDefined(cConfigUseVismapFile))
         loaded=loadSpanlists(numFixedLists, numMoveableLists);
      if(!loaded)
         generateSpanlists(numFixedLists, numMoveableLists);
      if(gConfig.isDefined(cConfigSaveVismapFile))
         saveSpanlists(numFixedLists, numMoveableLists);
#else
      generateSpanlists(numFixedLists, numMoveableLists);
#endif
   }

   trace("BVisibleMap::setup: finished, %u ms", GetTickCount() - startTime);
   
   TRACEMEM
   
   return true;
}

void BVisibleMap::shutdown(void)
{
   if (mEntireRadiusLists)
   {
      delete []mEntireRadiusLists;
      mEntireRadiusLists = NULL;
   }

   if (mAddPartialRadiusLists)
   {
      for (long i = 0; i < 8; i++)
      {
         if (mAddPartialRadiusLists[i])
            delete []mAddPartialRadiusLists[i];
      }
      delete []mAddPartialRadiusLists;
      mAddPartialRadiusLists = NULL;
   }

   if (mDelPartialRadiusLists)
   {
      for (long i = 0; i < 8; i++)
      {
         if (mDelPartialRadiusLists[i])
            delete []mDelPartialRadiusLists[i];
      }
      delete []mDelPartialRadiusLists;
      mDelPartialRadiusLists = NULL;
   }
}

void BVisibleMap::initMap(long numXTiles, long numZTiles)
{
   DWORD startTime = GetTickCount();
   trace("BVisibleMap::initMap: begin");

   // Cleanup
   deinitMap();

   // Save some variables
   mNumXTiles = numXTiles;
   mNumZTiles = numZTiles;
   mTileScale = gTerrainSimRep.getDataTileScale();
   mRecTileScale = gTerrainSimRep.getReciprocalDataTileScale();

   long numTiles = numXTiles * numZTiles;

   // Allocate map
   mMap = new BVisibilityTile[numTiles];
   BDEBUG_ASSERT(mMap);

   // Clear the map
   BVisibilityTile   clearedTile;
   clearedTile.visibility = 0;
   clearedTile.blocked = 0;
   clearedTile.team.setToOnes();
   for (long i = 0; i < numTiles; i++)
      mMap[i] = clearedTile;

   trace("BVisibleMap::initMap: finished, %u ms", GetTickCount() - startTime);

   gRender.startupStatus("Visible map initMap finished");
}

void BVisibleMap::deinitMap(void)
{
   if (mMap)
   {
      delete []mMap;
      mMap = NULL;
   }
}

void BVisibleMap::generateSpanlists(long numFixedLists, long numMoveableLists)
{
   for (long r = 0; r < numFixedLists; r++)
   {
      mEntireRadiusLists[r].buildFromRadius(r + MIN_RADIUS);
   }

   BTileSpanList tmpList;
   tmpList.reserve(512);

   for (long dir = 0; dir < 8; dir++)
   {
      mAddPartialRadiusLists[dir] = new BTileSpanList[numMoveableLists];
      for (int i = 0; i < numMoveableLists; i++)
         mAddPartialRadiusLists[dir][i].reserve(256);

      mDelPartialRadiusLists[dir] = new BTileSpanList[numMoveableLists];
      for (int i = 0; i < numMoveableLists; i++)
         mDelPartialRadiusLists[dir][i].reserve(256);

      // probably should start at about radius 5
      for (long r = 0; r < numMoveableLists; r++)
         calcSpanDiff(r + MIN_RADIUS, dir, tmpList);
   }

   for (long dir = 0; dir < 8; dir++)
   {
      for (int i = 0; i < numMoveableLists; i++)
      {
         BTileSpanList temp(mAddPartialRadiusLists[dir][i]);
         mAddPartialRadiusLists[dir][i].swap(temp);
      }

      for (int i = 0; i < numMoveableLists; i++)
      {
         BTileSpanList temp(mDelPartialRadiusLists[dir][i]);
         mDelPartialRadiusLists[dir][i].swap(temp);
      }
   } 
}

bool BVisibleMap::loadSpanlists(long numFixedLists, long numMoveableLists)
{
   BFile file;
   if(!file.openReadOnly(cDirData, "vismap.dat", BFILE_OPEN_DISCARD_ON_CLOSE))
      return(false);
   
   DWORD magic = 0;
   file.read(&magic, sizeof(magic));
   if(magic != cMagic)
      return(false);
   
   long checkMaxFixedRadius=0;
   long checkMaxMoveableRadius=0;
   long checkMinRadius=0;
   long checkNumFixedLists=0;
   long checkNumMoveableLists=0;
   file.read(&checkMaxFixedRadius, sizeof(long));
   file.read(&checkMaxMoveableRadius, sizeof(long));
   file.read(&checkMinRadius, sizeof(long));
   file.read(&checkNumFixedLists, sizeof(long));
   file.read(&checkNumMoveableLists, sizeof(long));
   if(checkMaxFixedRadius!=MAX_FIXED_RADIUS || checkMaxMoveableRadius!=MAX_MOVEABLE_RADIUS || checkMinRadius!=MIN_RADIUS || checkNumFixedLists!=numFixedLists || checkNumMoveableLists!=numMoveableLists)
      return(false);

   for(long i=0; i<numFixedLists; i++)
   {
      BTileSpanList& spanlist=mEntireRadiusLists[i];
      long num=0;
      if(!file.read(&num, sizeof(long)))
         return false;
      if(num>0)
      {
         if(!spanlist.setNumber(num))
            return false;
         if(!file.read(spanlist.getPtr(), sizeof(BTileSpan)*num))
            return false;
      }
   }
   for(long k=0; k<2; k++)
   {
      for(long j=0; j<8; j++)
      {
         BTileSpanList* spanlists = new BTileSpanList[numMoveableLists];
         if(spanlists)
         {
            for(long i=0; i<numMoveableLists; i++)
            {
               BTileSpanList& spanlist=spanlists[i];
               long num=0;
               if(!file.read(&num, sizeof(long)))
                  return false;
               if(num>0)
               {
                  if(!spanlist.setNumber(num))
                     return false;
                  if(!file.read(spanlist.getPtr(), sizeof(BTileSpan)*num))
                     return false;
               }
            }
         }
         if(k==0)
            mAddPartialRadiusLists[j]=spanlists;
         else
            mDelPartialRadiusLists[j]=spanlists;
      }
   }

   return(true);
}

void BVisibleMap::saveSpanlists(long numFixedLists, long numMoveableLists)
{
   BFile file;
   if(!file.openWriteable(cDirData, "vismap.dat", BFILE_OPEN_ENABLE_BUFFERING))
   {
      BASSERT(0);
   }
   else
   {
      file.write(&cMagic, sizeof(cMagic));
      
      long maxFixedRadius=MAX_FIXED_RADIUS;
      long maxMoveableRadius=MAX_MOVEABLE_RADIUS;
      long minRadius=MIN_RADIUS;
      file.write(&maxFixedRadius, sizeof(long));
      file.write(&maxMoveableRadius, sizeof(long));
      file.write(&minRadius, sizeof(long));
      file.write(&numFixedLists, sizeof(long));
      file.write(&numMoveableLists, sizeof(long));
      for(long i=0; i<numFixedLists; i++)
      {
         BTileSpanList& spanlist=mEntireRadiusLists[i];
         long num=spanlist.getNumber();
         file.write(&num, sizeof(long));
         file.write(spanlist.getPtr(), sizeof(BTileSpan)*num);
      }
      for(long k=0; k<2; k++)
      {
         for(long j=0; j<8; j++)
         {
            BTileSpanList* spanlists=(k==0 ? mAddPartialRadiusLists[j] : mDelPartialRadiusLists[j]);
            for(long i=0; i<numMoveableLists; i++)
            {
               BTileSpanList& spanlist=spanlists[i];
               long num=spanlist.getNumber();
               file.write(&num, sizeof(long));
               file.write(spanlist.getPtr(), sizeof(BTileSpan)*num);
            }
         }
      }
      file.close();
   }
}

void BVisibleMap::calcSpanDiff(long radius, long dir, BTileSpanList& tmpList)
{
   tmpList.resize(0);
   
   BTileSpanList &addList = mAddPartialRadiusLists[dir][radius - MIN_RADIUS];
   BTileSpanList &delList = mDelPartialRadiusLists[dir][radius - MIN_RADIUS];
   BTileSpanList &srcList = mEntireRadiusLists[radius - MIN_RADIUS];

   long xDelta = 0, zDelta = 0;

   switch (dir)
   {
   case 0:
      xDelta = -1;
      zDelta = 1;
      break;
   case 1:
      zDelta = 1;
      break;
   case 2:
      xDelta = 1;
      zDelta = 1;
      break;
   case 3:
      xDelta = 1;
      break;
   case 4:
      xDelta = 1;
      zDelta = -1;
      break;
   case 5:
      zDelta = -1;
      break;
   case 6:
      xDelta = -1;
      zDelta = -1;
      break;
   case 7:
      xDelta = -1;
      break;
   }

   long s1, n1, x, s2, n2;
   long count = srcList.getNumber();
   long idx;
   for (idx = 0; idx < count; idx++)
   {
      s1 = srcList[idx].getStartZ() + zDelta;
      n1 = srcList[idx].getNumberTiles();
      x = srcList[idx].getStartX() + xDelta;

      tmpList.addSpan(BTileSpan(x, s1, n1), false);
   }

   long ip1 = 0;
   long ip2 = 0;
   idx = 0;

   // Handle full verticle strips on left edge of processing

   // if new list is to right of source list
   if (xDelta > 0)
   {
      // while the "new" list has greater X than the source
      while ((ip2 < count) && (tmpList[ip1].getStartX() > srcList[ip2].getStartX()))
      {
         // add the spans to the "to-be-deleted" list
         delList.addSpan(srcList[ip2++], false);
      }
      idx = ip2;
   }
   // else if new list is to left of source list
   else if (xDelta < 0)
   {
      // while the "new" list has less X than the source
      while ((ip1 < count) && (tmpList[ip1].getStartX() < srcList[ip2].getStartX()))
      {
         // add the spans to the "to-be-added" list
         addList.addSpan(tmpList[ip1++], false);
      }
      idx = ip1;
   }
   // else no entire verticle strips

   while (idx < count)
   {
      // starting at the current span
      x = tmpList[ip1].getStartX();

      // get the start of the current span, and length
      s1 = tmpList[ip1].getStartZ();
      n1 = s1 + tmpList[ip1++].getNumberTiles();

      // get the start of the source span, and length
      s2 = srcList[ip2].getStartZ();
      n2 = s2 + srcList[ip2++].getNumberTiles();

      // current span is above source
      if (s1 > s2)
      {
         delList.addSpan(BTileSpan(x, s2, s1 - s2), false);
      }
      // current span is below source
      else if (s1 < s2)
      {
         addList.addSpan(BTileSpan(x, s1, s2 - s1), false);
      }
      // else start didn't move

      // current span ends above the source span
      if (n1 > n2)
      {
         addList.addSpan(BTileSpan(x, n2, n1 - n2), false);
      }
      else if (n1 < n2)
      {
         delList.addSpan(BTileSpan(x, n1, n2 - n1), false);
      }

      idx++;
   }

   // finish full verticle spans on right edge of processing

   // finish deleting the spans that were not in the "new" list
   while (ip2 < srcList.getNumber())
   {
      delList.addSpan(srcList[ip2++], false);
   }

   // finish adding the spans that were not in the source list
   while (ip1 < tmpList.getNumber())
   {
      addList.addSpan(tmpList[ip1++], false);
   }
}

//==============================================================================
// Update
//==============================================================================
void BVisibleMap::updateEntireMap(BTeamID teamID)
{
   for (long x = 0; x < mNumXTiles; x++)
   {
      for (long z = 0; z < mNumZTiles; z++)
      {
         explore(x, z, teamID);
         unexplore(x, z, teamID);
      }
   }
}

//==============================================================================
// Explore
//==============================================================================
void BVisibleMap::exploreEntireMap(BTeamID teamID)
{
   for (long x = 0; x < mNumXTiles; x++)
   {
      for (long z = 0; z < mNumZTiles; z++)
      {
         explore(x, z, teamID);
      }
   }
}


//==============================================================================
// Unexplore
//==============================================================================
void BVisibleMap::unexploreEntireMap(BTeamID teamID)
{
   for (long x = 0; x < mNumXTiles; x++)
   {
      for (long z = 0; z < mNumZTiles; z++)
      {
         unexplore(x, z, teamID);
      }
   }
}

//==============================================================================
// Reset black map
//==============================================================================
void BVisibleMap::resetBlackMap(BTeamID teamID)
{
   for (long x = 0; x < mNumXTiles; x++)
   {
      for (long z = 0; z < mNumZTiles; z++)
      {
         resetBlackMap(x, z, teamID);
      }
   }
}

//==============================================================================
// Explore/Unexplore
//==============================================================================
void BVisibleMap::explore(long x, long z, BTeamID teamID)
{
   // Skip if off the map.
   if ((x < 0) || (x >= mNumXTiles) || (z < 0) || (z >= mNumZTiles))
   {
      BDEBUG_ASSERT(0);
      return;
   }

   // Get offset
   long offset = x * mNumZTiles + z;

   // Get team reference count.
   uint16 teamRefCount = mMap[offset].team.getElement(teamID);

   // Check if we are at the sentinel byte which means we are about
   // to wrap around and screw up because too many units owned by this team can see this
   // tile at one time.  Just bail out in this case to minimize the screw up.
   if (teamRefCount == cSentinelValue)
   {
      BFAIL("Too many units seeing a single tile.  Vis map refcount is going to break.");
      return;
   }

   // We just explored this tile for the first time.
   if (teamRefCount == cBlackValue)
   {
      // Make the reference count 1.
      teamRefCount = 1;

      // Mark the tile as no longer black.
      if (mMap[offset].blocked & getTeamBlockedMask(teamID))
         mMap[offset].visibility |= getTeamBlackOffMask(teamID);
      else
         mMap[offset].visibility |= getTeamBlackOffMask(teamID) | getTeamFogOffMask(teamID);
   }
   // This tile was fogged
   else if (teamRefCount == cFogValue)
   {
      // Make the reference count 1.
      teamRefCount = 1;

      // Mark it in the combined map if it's not blocked
      if (!(mMap[offset].blocked & getTeamBlockedMask(teamID)))
         mMap[offset].visibility |= getTeamFogOffMask(teamID);
   }
   else
   {
      teamRefCount++;
   }

   // Store refcount
   mMap[offset].team.setElement(teamID, teamRefCount);
}

void BVisibleMap::unexplore(long x, long z, BTeamID teamID)
{
   // Skip if off the map.
   if ((x < 0) || (x >= mNumXTiles) || (z < 0) || (z >= mNumZTiles))
      return;

   // Get offset.
   long offset = x * mNumZTiles + z;

   // Get team reference count.
   uint16 teamRefCount = mMap[offset].team.getElement(teamID);

   // Check that we are not already in the fog state.  If we are, then something has screwed up
   // and the reference count for this tile has gotten out of whack.  In any case, just bail
   // out in this case so that the map doesn't turn back to black from fogged.
   if (teamRefCount == cFogValue)
   {
      // @TBD: restore this assert when it isn't berserk -- Xemu, 2/11/01
      // jce 11/8/2001 -- turned this back on since Rob can't remember why he turned it off and we
      // were failing to catch problem cases.
      BFAIL("Trying to unexplore an area that is already fogged.  Vis map ref count is busted.");
      return;
   }
   // Similarly assert that we are not currently unexplored.  We should never be unexploring black map.
   if (teamRefCount == cBlackValue)
   {
      // jce 11/8/2001 -- turned this back on too.  We should fix whatever problems we might have at
      // a higher level.  Commenting out these asserts breaks the ref counts on the map without letting us know.
      BFAIL("Trying to unexplore an area that is blackmap.  Vis map ref count is busted.");
      // When you restore the one up there Xemu, put this one back in as well.  I took it out because
      // after resizing the terrain, apparently it's quite possible that you will indeed be unexploring black
      // map.  dlm 3/1/01 
      return;
   }

   // Decrement team reference count.
   teamRefCount--;
   mMap[offset].team.setElement(teamID, teamRefCount);

   // If we are going back to fogged, mark this in the combined map.   
   if (teamRefCount == cFogValue)
   {
      if (mMap[offset].blocked & getTeamBlockedMask(teamID))
         return;

      // Mark the tile as fogged.
      mMap[offset].visibility &= ~getTeamFogOffMask(teamID);      
   }
}

void BVisibleMap::resetBlackMap(long x, long z, BTeamID teamID)
{
   // Skip if off the map.
   if ((x < 0) || (x >= mNumXTiles) || (z < 0) || (z >= mNumZTiles))
      return;

   // Get offset.
   long offset = x * mNumZTiles + z;

   // Get team reference count.
   uint16 teamRefCount = mMap[offset].team.getElement(teamID);

   if (teamRefCount != cFogValue)
      return;

   // Decrement team reference count.
   teamRefCount--;
   mMap[offset].team.setElement(teamID, teamRefCount);

   if (mMap[offset].blocked & getTeamBlockedMask(teamID))
      return;

   // Mark the tile as black map.
   mMap[offset].visibility &= ~getTeamFogOffMask(teamID);
   mMap[offset].visibility &= ~getTeamBlackOffMask(teamID);
}

bool BVisibleMap::updateCircularRegion(long oldX, long oldZ, long newX, long newZ, long radius, BTeamID teamID)
{
   // if this fires, we need to adjust our min/max bounds
   if (radius < MIN_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::updateCircularRegion radius too small.");
      radius = MIN_RADIUS;
   }
   else if (radius > MAX_MOVEABLE_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::updateCircularRegion radius too big.");
      radius = MAX_MOVEABLE_RADIUS;
   }

   long zDelta = newZ - oldZ;
   long xDelta = newX - oldX;

   // if we changed more than one tile in either direction
   // we just use the entire region
   bool partial = ((abs(xDelta) <= 1) && (abs(zDelta) <= 1)) ? true : false;

   // probably doesn't make since to do a partial update for radii less than 5 tiles
   if (!partial || (radius < MIN_RADIUS))
   {
      if (!unexploreCircularRegion(oldX, oldZ, radius, teamID))
         return false;
      if (!exploreCircularRegion(newX, newZ, radius, teamID))
         return false;
   }
   else
   {
      // determine the direction we are moving
      long dir = -1;

      if (zDelta != 0)
      {
         if (xDelta != 0)
         {
            if ((xDelta == -1) && (zDelta == 1))
               dir = 0;
            else if ((xDelta == 1) && (zDelta == 1))
               dir = 2;
            else if ((xDelta == 1) && (zDelta == -1))
               dir = 4;
            else
               dir = 6;
         }
         else if (zDelta == 1)
            dir = 1;
         else
            dir = 5;
      }
      else if (xDelta == 1)
         dir = 3;
      else
         dir = 7;

      BDEBUG_ASSERT(dir != -1); // how the heck did you make it through that mess?

      BTileSpanList &addList = mAddPartialRadiusLists[dir][radius - MIN_RADIUS];
      BTileSpanList &delList = mDelPartialRadiusLists[dir][radius - MIN_RADIUS];

      // prefetch the spanlists
      long oldCount = delList.getNumber();
      long newCount = addList.getNumber();
      long start, end, x, z;
      long offset = 0;
      long size = oldCount * sizeof(BTileSpan);
      do
      {
         __dcbt(offset, delList.getPtr()); // fetch 128b of del list data
         offset += 128;
      } while(offset < size);

      // prefetch some lines into cache
      long loadAhead = min(CACHE_LINES, oldCount);
      long loadAhead2 = 0;
      for (long idx = 0; idx < loadAhead; idx++)
         __dcbt(((delList[idx].getStartX() + oldX) * mNumZTiles + (delList[idx].getStartZ() + oldZ)) * sizeof(BVisibilityTile), mMap); // fetch part of a scan line of mMap

      offset = 0;
      size = newCount * sizeof(BTileSpan);
      do
      {
         __dcbt(offset, addList.getPtr()); // fetch 128b of add list data
         offset += 128;
      } while(offset < size);

      // unexplore the "deleted" tiles
      for (long idx = 0; idx < oldCount; idx++, loadAhead++)
      {
         // prefetch one line into cache
         if (loadAhead < oldCount)
            __dcbt(((delList[loadAhead].getStartX() + oldX) * mNumZTiles + (delList[loadAhead].getStartZ() + oldZ)) * sizeof(BVisibilityTile), mMap); // fetch part of a scan line of mMap
         else if (loadAhead2 < newCount)
         {
            __dcbt(((addList[loadAhead2].getStartX() + oldX) * mNumZTiles + (addList[loadAhead2].getStartZ() + oldZ)) * sizeof(BVisibilityTile), mMap); // fetch part of a scan line of mMap
            loadAhead2++;
         }

         start = delList[idx].getStartZ() + oldZ;
         end = start + delList[idx].getNumberTiles();
         x = delList[idx].getStartX() + oldX;

         // ignore out of bounds tiles
         if ((x < 0) || (x >= mNumXTiles))
            continue;

         if (start < 0)
            start = 0;
         if (end > mNumZTiles)
            end = mNumZTiles;

         for (z = start; z < end; z++)
            unexplore(x, z, teamID);
      }

      // explore the "added" tiles
      for (long idx = 0; idx < newCount; idx++, loadAhead2++)
      {
         // prefetch one line into cache
         if (loadAhead2 < newCount)
            __dcbt(((addList[loadAhead2].getStartX() + oldX) * mNumZTiles + (addList[loadAhead2].getStartZ() + oldZ)) * sizeof(BVisibilityTile), mMap); // fetch part of a scan line of mMap

         start = addList[idx].getStartZ() + oldZ;
         end = start + addList[idx].getNumberTiles();
         x = addList[idx].getStartX() + oldX;

         // ignore out of bounds tiles
         if ((x < 0) || (x >= mNumXTiles))
            continue;

         if (start < 0)
            start = 0;
         if (end > mNumZTiles)
            end = mNumZTiles;

         for (z = start; z < end; z++)
            explore(x, z, teamID);
      }
   }

   return true;
}

bool BVisibleMap::exploreCircularRegion(long x, long z, long radius, BTeamID teamID)
{
   // if this fires, we need to adjust our min/max bounds
   if (radius < MIN_RADIUS)
   {
// rg - DO NOT CHECK RG
      //BASSERTM(0, "BVisibleMap::exploreCircularRegion radius too small.");
      radius = MIN_RADIUS;
   }
   else if (radius > MAX_FIXED_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::exploreCircularRegion radius too big.");
      radius = MAX_FIXED_RADIUS;
   }

   BTileSpanList &spanlist = mEntireRadiusLists[radius - MIN_RADIUS];

   // explore all of the tiles in the spanlist at the new location
   long count = spanlist.getNumber();
   for (long idx = 0; idx < count; idx++)
   {
      long start = spanlist[idx].getStartZ() + z;
      long end = start + spanlist[idx].getNumberTiles();
      long ix = spanlist[idx].getStartX() + x;

      // ignore out of bounds tiles
      if ((ix < 0) || (ix >= mNumXTiles))
         continue;

      if (start < 0)
         start = 0;
      if (end > mNumZTiles)
         end = mNumZTiles;

      for (long iz = start; iz < end; iz++)
         explore(ix, iz, teamID);
   }

   return true;
}

bool BVisibleMap::unexploreCircularRegion(long x, long z, long radius, BTeamID teamID)
{
   // if this fires, we need to adjust our min/max bounds
   if (radius < MIN_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::unexploreCircularRegion radius too small.");
      radius = MIN_RADIUS;
   }
   else if (radius > MAX_FIXED_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::unexploreCircularRegion radius too big.");
      radius = MAX_FIXED_RADIUS;
   }

   BTileSpanList &spanlist = mEntireRadiusLists[radius - MIN_RADIUS];

   // unexplore all of the tiles in the spanlist at the old location
   long count = spanlist.getNumber();
   for (long idx = 0; idx < count; idx++)
   {
      long start = spanlist[idx].getStartZ() + z;
      long end = start + spanlist[idx].getNumberTiles();
      long ix = spanlist[idx].getStartX() + x;

      // ignore out of bounds tiles
      if ((ix < 0) || (ix >= mNumXTiles))
         continue;

      if (start < 0)
         start = 0;
      if (end > mNumZTiles)
         end = mNumZTiles;

      for (long iz = start; iz < end; iz++)
         unexplore(ix, iz, teamID);
   }

   return true;
}

void BVisibleMap::explore(long startX, long startZ, long endX, long endZ, BTeamID teamID)
{
   // Clamp coordinates.
   if (startX < 0)
      startX = 0;
   if (startX >= mNumXTiles)
      startX = mNumXTiles - 1;
   if (endX < 0)
      endX = 0;
   if (endX >= mNumXTiles)
      endX = mNumXTiles - 1;
   if (startZ < 0)
      startZ = 0;
   if (startZ >= mNumZTiles)
      startZ = mNumZTiles - 1;
   if (endZ < 0)
      endZ = 0;
   if (endZ >= mNumZTiles)
      endZ = mNumZTiles - 1;

   // Explore.
   for (long x = startX; x <= endX; x++)
   {
      for (long z = startZ; z <= endZ; z++)
      {
         explore(x, z, teamID);
      }
   }
}

void BVisibleMap::unexplore(long startX, long startZ, long endX, long endZ, BTeamID teamID)
{
   // Clamp coordinates.
   if (startX < 0)
      startX = 0;
   if (startX >= mNumXTiles)
      startX = mNumXTiles - 1;
   if (endX < 0)
      endX = 0;
   if (endX >= mNumXTiles)
      endX = mNumXTiles - 1;
   if (startZ < 0)
      startZ = 0;
   if (startZ >= mNumZTiles)
      startZ = mNumZTiles - 1;
   if (endZ < 0)
      endZ = 0;
   if (endZ >= mNumZTiles)
      endZ = mNumZTiles - 1;

   // unexplore.
   for (long x = startX; x <= endX; x++)
   {
      for (long z = startZ; z <= endZ; z++)
      {
         unexplore(x, z, teamID);
      }
   }
}

void BVisibleMap::exploreHull(const BNonconvexHull &hull, BTeamID teamID)
{
   // Get tiles overlapping the bounding box of the hull.
   long minXTile = (long)(hull.getBoundingMin().x * mRecTileScale);
   long minZTile = (long)(hull.getBoundingMin().z * mRecTileScale);
   long maxXTile = (long)(hull.getBoundingMax().x * mRecTileScale);
   long maxZTile = (long)(hull.getBoundingMax().z * mRecTileScale);

   // Check each tile.
   for (long x = minXTile; x <= maxXTile; x++)
   {
      for (long z = minZTile; z <= maxZTile; z++)
      {
         // See if the hull and the tile overlap.
         bool overlaps = hull.overlapsBox(x * mTileScale, z * mTileScale, (x + 1) * mTileScale, (z + 1) * mTileScale);

         if (overlaps)
            explore(x, z, teamID);
      }
   }
}

void BVisibleMap::unexploreHull(const BNonconvexHull &hull, BTeamID teamID)
{
   // Get tiles overlapping the bounding box of the hull.
   long minXTile = (long)(hull.getBoundingMin().x * mRecTileScale);
   long minZTile = (long)(hull.getBoundingMin().z * mRecTileScale);
   long maxXTile = (long)(hull.getBoundingMax().x * mRecTileScale);
   long maxZTile = (long)(hull.getBoundingMax().z * mRecTileScale);

   // Check each tile.
   for (long x = minXTile; x <= maxXTile; x++)
   {
      for (long z = minZTile; z <= maxZTile; z++)
      {
         // See if the hull and the tile overlap.
         bool overlaps = hull.overlapsBox(x * mTileScale, z * mTileScale, (x + 1) * mTileScale, (z + 1) * mTileScale);

         if (overlaps)
            unexplore(x, z, teamID);
      }
   }
}

BYTE BVisibleMap::checkMask(long x, long z, DWORD mask) const
{
   // Skip if off the map.
   if ((x < 0) || (x >= mNumXTiles) || (z < 0) || (z >= mNumZTiles))
   {
      //DCP: Commented this out to deal with arrows that go off the map.  We may need to augment
      //this paradigm (things off the map are invisible to everyone) at some polong.
      //SLB: Re-enabling
      BASSERT(0);
      return cNotVisible;
   }

   if (!mMap)
   {
      BDEBUG_ASSERT(0);
      return cNotVisible;
   }

   DWORD value = mMap[(x * mNumZTiles) + z].visibility;
   value &= mask;

   if (!(value & cBlackMask))
      return cNotVisible;

   if (!(value & cFogMask))
      return cFogged;

   return cVisible;
}

//No longer used
#if 0  
BYTE BVisibleMap::getSurroundCode(long x, long z, DWORD mask) const
{
   // Skip if off the map.
   if ((x < 0) || (x >= mNumXTiles) || (z < 0) || (z >= mNumZTiles))
   {
      BDEBUG_ASSERT(0);
      return 0;
   }

   // Start code out as 0.
   BYTE code = 0;

   long baseIndex = (x - 1) * mNumZTiles + z;

   // First row.
   if (x > 0)
   {
      if (z > 0)
      {
         if (!(mMap[baseIndex - 1].visibility & mask))
            code |= cTile0Mask;
      }

      if (!(mMap[baseIndex].visibility & mask))
         code |= cTile3Mask;

      if (z < (mNumZTiles - 1))
      {
         if (!(mMap[baseIndex + 1].visibility & mask))
            code |= cTile5Mask;
      }
   }

   // Second row.
   baseIndex += mNumZTiles;
   if (z > 0)
   {
      if (!(mMap[baseIndex - 1].visibility & mask))
         code |= cTile1Mask;
   }

   if (z < (mNumZTiles - 1))
   {
      if (!(mMap[baseIndex + 1].visibility & mask))
         code |= cTile6Mask;
   }

   // Third row.
   baseIndex += mNumZTiles;
   if (x < (mNumXTiles - 1))
   {
      if (z > 0)
      {
         if (!(mMap[baseIndex - 1].visibility & mask))
            code |= cTile2Mask;
      }

      if (!(mMap[baseIndex].visibility & mask))
         code |= cTile4Mask;

      if (z < (mNumZTiles - 1))
      {
         if (!(mMap[baseIndex + 1].visibility & mask))
            code |= cTile7Mask;
      }
   }

   return code;
}
#endif

//Used by the Knowledge Base
bool BVisibleMap::isPositionVisibleToTeam(BVector pos, BTeamID teamID) const
{
   XMVECTOR simPosition = __vctsxs(XMVectorMultiply(pos, XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), 0);
   long simX = simPosition.u[0];
   long simZ = simPosition.u[2];
   DWORD visible = gVisibleMap.getTeamFogOffMask(teamID);
   // DLM sanity check here to not crash game.
   if (simX < 0 || simX >= gTerrainSimRep.getNumXDataTiles())
      return false;
   if (simZ < 0 || simZ >= gTerrainSimRep.getNumXDataTiles())
      return false;
   if (gVisibleMap.getVisibility(simX, simZ) & visible)
      return (true);
   else
      return (false);
}

//Used by the Knowledge Base
bool BVisibleMap::isPositionExploredToTeam(BVector pos, BTeamID teamID) const
{
   XMVECTOR simPosition = __vctsxs(XMVectorMultiply(pos, XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), 0);
   long simX = simPosition.u[0];
   long simZ = simPosition.u[2];
   DWORD visible = gVisibleMap.getTeamBlackOffMask(teamID);
   // DLM sanity check here to not crash game.
   if (simX < 0 || simX >= gTerrainSimRep.getNumXDataTiles())
      return false;
   if (simZ < 0 || simZ >= gTerrainSimRep.getNumXDataTiles())
      return false;
   if (gVisibleMap.getVisibility(simX, simZ) & visible)
      return (true);
   else
      return (false);
}

DWORD BVisibleMap::getCircularEdgeVisibility(long x, long z, long radius)
{
   // if this fires, we need to adjust our min/max bounds
   if (radius < MIN_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::getCircularEdgeVisibility radius too small.");
      radius = MIN_RADIUS;
   }
   else if (radius > MAX_FIXED_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::getCircularEdgeVisibility radius too big.");
      radius = MAX_FIXED_RADIUS;
   }

   DWORD visibility=0;

   BTileSpanList &spanlist = mEntireRadiusLists[radius - MIN_RADIUS];

   // check the visibility of the tiles along the blocker's outer edge
   long count = spanlist.getNumber();
   for (long idx = 0; idx < count; idx++)
   {
      long start = spanlist[idx].getStartZ() + z;
      long end = start + spanlist[idx].getNumberTiles();
      long ix = spanlist[idx].getStartX() + x;

      // ignore out of bounds tiles
      if ((ix < 0) || (ix >= mNumXTiles))
         continue;

      if (start < 0)
         start = 0;
      if (end > mNumZTiles)
         end = mNumZTiles;

      //visibility|=getNoBlockerVisibility(ix, start);
      //visibility|=getNoBlockerVisibility(ix, end);
      visibility|=getVisibility(ix, start);
      visibility|=getVisibility(ix, end);
   }

   return visibility;
}

//==============================================================================
// Block/Unblock
//==============================================================================
void BVisibleMap::block(long x, long z, BTeamID teamID)
{ 
   // Skip if off the map.
   if ((x < 0) || (x >= mNumXTiles) || (z < 0) || (z >= mNumZTiles))
   {
      BDEBUG_ASSERT(0);
      return;
   }

   // Get offset on map.
   long offset = (x * mNumZTiles) + z;

   // Don't do anything if already blocked.
   if ((mMap[offset].blocked & getTeamBlockedMask(teamID)) != 0)
      return;

   // Block the tile for the specified team.
   mMap[offset].blocked |= getTeamBlockedMask(teamID);

   // Fixup the map explore value.
   uint16 teamRefCount = mMap[offset].team.getElement(teamID);

   /* ajl 8/10/06 - Make tiles fogged instead of black
   if (teamRefCount == cFogValue)
   {
      // Mark the tile as black.
      mMap[offset].visibility &= ~getTeamBlackOffMask(teamID);
   }
   else if ((teamRefCount != cBlackValue) && (teamRefCount >= 1))
   {
      // Mark the tile as black and tell the rest of the world about it.
      mMap[offset].visibility &= ~getTeamFogOffMask(teamID);
      mMap[offset].visibility &= ~getTeamBlackOffMask(teamID);
   }
   */
   if ((teamRefCount != cBlackValue) && (teamRefCount >= 1))
   {
      // Mark the tile as fogged.
      mMap[offset].visibility &= ~getTeamFogOffMask(teamID);
   }
}

void BVisibleMap::unblock(long x, long z, BTeamID teamID)
{ 
   // Skip if off the map.
   if ((x < 0) || (x >= mNumXTiles) || (z < 0) || (z >= mNumZTiles))
   {
      BDEBUG_ASSERT(0);
      return;
   }

   // Get offset on map.
   long offset = (x * mNumZTiles) + z;

   // Don't do anything if not blocked.
   if ((mMap[offset].blocked & getTeamBlockedMask(teamID)) == 0)
      return;

   // Unblock the tile for the specified team.
   mMap[offset].blocked &= ~getTeamBlockedMask(teamID);

   // Fixup map explore value.
   uint16 teamRefCount = mMap[offset].team.getElement(teamID);

   /* ajl 8/10/06 - Unblock from fogged instead of from black
   if (teamRefCount == cFogValue)
   {
      if ((mMap[offset].visibility & getTeamBlackOffMask(teamID)) == 0)
      {
         // Leave the tile black
         teamRefCount = cBlackValue;
      }
   }
   else if ((teamRefCount != cBlackValue) && (teamRefCount >= 1))
   {
      if ((mMap[offset].visibility & getTeamBlackOffMask(teamID)) == 0)
      {
         // Mark the tile as visible.
         mMap[offset].visibility |= getTeamBlackOffMask(teamID);
      }
      if ((mMap[offset].visibility & getTeamFogOffMask(teamID)) == 0)
      {
         // Mark the tile as visible.
         mMap[offset].visibility |= getTeamFogOffMask(teamID);
      }
   }
   */
   if (teamRefCount != cBlackValue)
   {
      if ((mMap[offset].visibility & getTeamBlackOffMask(teamID)) == 0)
      {
         // Mark the tile as not black.
         mMap[offset].visibility |= getTeamBlackOffMask(teamID);
      }
      if (teamRefCount >= 1 && (mMap[offset].visibility & getTeamFogOffMask(teamID)) == 0)
      {
         // Mark the tile as not fogged.
         mMap[offset].visibility |= getTeamFogOffMask(teamID);
      }
   }
}

//====================================
// Block the entire map for this team
//====================================
void BVisibleMap::blockEntireMap(BTeamID teamID)
{
   for (long x = 0; x < mNumXTiles; x++)
   {
      for (long z = 0; z < mNumZTiles; z++)
      {
         block(x, z, teamID);
      }
   }
}

//======================================
// Unblock the entire map for this team
//======================================
void BVisibleMap::unblockEntireMap(BTeamID teamID)
{
   for (long x = 0; x < mNumXTiles; x++)
   {
      for (long z = 0; z < mNumZTiles; z++)
      {
         unblock(x, z, teamID);
      }
   }
}

bool BVisibleMap::blockCircularRegion(long x, long z, long radius, BTeamID teamID)
{
   // if this fires, we need to adjust our min/max bounds
   if (radius < MIN_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::blockCircularRegion radius too small.");
      radius = MIN_RADIUS;
   }
   else if (radius > MAX_FIXED_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::blockCircularRegion radius too big.");
      radius = MAX_FIXED_RADIUS;
   }

   BTileSpanList &spanlist = mEntireRadiusLists[radius - MIN_RADIUS];

   // explore all of the tiles in the spanlist at the new location
   long count = spanlist.getNumber();
   for (long idx = 0; idx < count; idx++)
   {
      long start = spanlist[idx].getStartZ() + z;
      long end = start + spanlist[idx].getNumberTiles();
      long ix = spanlist[idx].getStartX() + x;

      // ignore out of bounds tiles
      if ((ix < 0) || (ix >= mNumXTiles))
         continue;

      if (start < 0)
         start = 0;
      if (end > mNumZTiles)
         end = mNumZTiles;

      for (long iz = start; iz < end; iz++)
         block(ix, iz, teamID);
   }

   return true;
}

bool BVisibleMap::unblockCircularRegion(long x, long z, long radius, BTeamID teamID)
{
   // if this fires, we need to adjust our min/max bounds
   if (radius < MIN_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::unblockCircularRegion radius too small.");
      radius = MIN_RADIUS;
   }
   else if (radius > MAX_FIXED_RADIUS)
   {
      BASSERTM(0, "BVisibleMap::unblockCircularRegion radius too big.");
      radius = MAX_FIXED_RADIUS;
   }

   BTileSpanList &spanlist = mEntireRadiusLists[radius - MIN_RADIUS];

   // unexplore all of the tiles in the spanlist at the old location
   long count = spanlist.getNumber();
   for (long idx = 0; idx < count; idx++)
   {
      long start = spanlist[idx].getStartZ() + z;
      long end = start + spanlist[idx].getNumberTiles();
      long ix = spanlist[idx].getStartX() + x;

      // ignore out of bounds tiles
      if ((ix < 0) || (ix >= mNumXTiles))
         continue;

      if (start < 0)
         start = 0;
      if (end > mNumZTiles)
         end = mNumZTiles;

      for (long iz = start; iz < end; iz++)
         unblock(ix, iz, teamID);
   }

   return true;
}

void BVisibleMap::block(long startX, long startZ, long endX, long endZ, BTeamID teamID)
{
   // Clamp coordinates.
   if (startX < 0)
      startX = 0;
   if (startX >= mNumXTiles)
      startX = mNumXTiles - 1;
   if (endX < 0)
      endX = 0;
   if (endX >= mNumXTiles)
      endX = mNumXTiles - 1;
   if (startZ < 0)
      startZ = 0;
   if (startZ >= mNumZTiles)
      startZ = mNumZTiles - 1;
   if (endZ < 0)
      endZ = 0;
   if (endZ >= mNumZTiles)
      endZ = mNumZTiles - 1;

   for (long x = startX; x <= endX; x++)
   {
      for (long z = startZ; z <= endZ; z++)
      {
         block(x, z, teamID);
      }
   }
}

void BVisibleMap::unblock(long startX, long startZ, long endX, long endZ, BTeamID teamID)
{
   // Clamp coordinates.
   if (startX < 0)
      startX = 0;
   if (startX >= mNumXTiles)
      startX = mNumXTiles - 1;
   if (endX < 0)
      endX = 0;
   if (endX >= mNumXTiles)
      endX = mNumXTiles - 1;
   if (startZ < 0)
      startZ = 0;
   if (startZ >= mNumZTiles)
      startZ = mNumZTiles - 1;
   if (endZ < 0)
      endZ = 0;
   if (endZ >= mNumZTiles)
      endZ = mNumZTiles - 1;

   for (long x = startX; x <= endX; x++)
   {
      for (long z = startZ; z <= endZ; z++)
      {
         unblock(x, z, teamID);
      }
   }
}

void BVisibleMap::blockHull(const BConvexHull &hull, BTeamID teamID)
{
   // Get tiles overlapping the bounding box of the hull.
   long minXTile = (long)(hull.getBoundingMin().x * mRecTileScale);
   long minZTile = (long)(hull.getBoundingMin().z * mRecTileScale);
   long maxXTile = (long)(hull.getBoundingMax().x * mRecTileScale);
   long maxZTile = (long)(hull.getBoundingMax().z * mRecTileScale);

   // Check each tile.
   for (long x = minXTile; x <= maxXTile; x++)
   {
      for (long z = minZTile; z <= maxZTile; z++)
      {
         // See if the hull and the tile overlap.
         bool overlaps = hull.overlapsBox(x * mTileScale, z * mTileScale, (x + 1) * mTileScale, (z + 1) * mTileScale);

         if (overlaps)
            block(x, z, teamID);
      }
   }
}

void BVisibleMap::unblockHull(const BConvexHull& hull, BTeamID teamID)
{
   // Get tiles overlapping the bounding box of the hull.
   long minXTile = (long)(hull.getBoundingMin().x * mRecTileScale);
   long minZTile = (long)(hull.getBoundingMin().z * mRecTileScale);
   long maxXTile = (long)(hull.getBoundingMax().x * mRecTileScale);
   long maxZTile = (long)(hull.getBoundingMax().z * mRecTileScale);

   // Check each tile.
   for (long x = minXTile; x <= maxXTile; x++)
   {
      for (long z = minZTile; z <= maxZTile; z++)
      {
         // See if the hull and the tile overlap.
         bool overlaps = hull.overlapsBox(x * mTileScale, z * mTileScale, (x + 1) * mTileScale, (z + 1) * mTileScale);

         if (overlaps)
            unblock(x, z, teamID);
      }
   }
}

//==============================================================================
// Copies mMap to the visibility texture
//==============================================================================
void BVisibleMap::resetVisibilityTexture()
{
   if (gRenderThread.isInsideLevelLoad())
      return;

   long teamID = gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID();
   DWORD maxX = getMaxXTiles();
   DWORD maxZ = getMaxZTiles();
   DWORD mask = getTeamBlackOffMask(teamID);
   DWORD dataLen = maxX * maxZ;

   // Allocate temp buffer
   bool* pVisibilityData = reinterpret_cast<bool*>(gRenderThread.allocateFrameStorage(dataLen));
   BASSERT(pVisibilityData);

   // Copy visibility data
   DWORD offset = 0;
   for (DWORD z = 0; z < maxZ; z++)
   {
      for (DWORD x = 0; x < maxX; x++)
      {
         // black -> false
         // fog -> true
         // visible -> true
         pVisibilityData[offset++] = (getVisibility(x, z) & mask) ? true : false;
      }
   }

   // Send this off to the minimap renderer
   gFlashMinimapRenderer.submitResetVisibility(maxX, maxZ, pVisibilityData);
}

//==============================================================================
// To save time and space, only save off fogged tiles. No black or visible 
// tiles since those are automatically restored upon load. Also only do this 
// for the current user.
//==============================================================================
bool BVisibleMap::save(BStream* pStream, int saveType) const
{
   BUser* pUser = gUserManager.getPrimaryUser();
   BTeamID teamID = pUser->getTeamID();

   GFWRITEVAR(pStream, long, mNumXTiles);
   GFWRITEVAR(pStream, long, mNumZTiles);
   for (long x = 0; x < mNumXTiles; x++)
   {
      bool xWritten = false;
      long zStart = -1;
      for (long z = 0; z < mNumZTiles; z++)
      {
         long offset = x * mNumZTiles + z;
         uint16 teamRefCount = mMap[offset].team.getElement(teamID);
         if (teamRefCount == cFogValue)
         {
            if (zStart == -1)
               zStart = z;
         }
         else
         {
            if (zStart != -1)
            {
               long zLen = z - zStart;
               if (!xWritten)
               {
                  GFWRITEVAL(pStream, uint16, x);
                  xWritten=true;
               }
               GFWRITEVAL(pStream, uint16, zStart);
               GFWRITEVAL(pStream, uint16, zLen);
               zStart = -1;
            }
         }
      }
      if (zStart != -1)
      {
         long zLen = z - zStart;
         if (!xWritten)
         {
            GFWRITEVAL(pStream, uint16, x);
            xWritten = true;
         }
         GFWRITEVAL(pStream, uint16, zStart);
         GFWRITEVAL(pStream, uint16, zLen);
      }
      if (xWritten)
         GFWRITEVAL(pStream, uint16, UINT16_MAX);
   }
   GFWRITEVAL(pStream, uint16, UINT16_MAX);

   return true;
}

//==============================================================================
//==============================================================================
bool BVisibleMap::load(BStream* pStream, int saveType)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   BTeamID teamID = pUser->getTeamID();

   long numXTiles, numZTiles;
   GFREADVAR(pStream, long, numXTiles);
   GFREADVAR(pStream, long, numZTiles);
   bool valid = (numXTiles == mNumXTiles && numZTiles == numZTiles);

   DWORD teamBlackOffMask = getTeamBlackOffMask(teamID);

   for (;;)
   {
      long x;
      GFREADVAL(pStream, uint16, long, x);
      if (x == UINT16_MAX)
         break;
      for (;;)
      {
         long zStart;
         GFREADVAL(pStream, uint16, long, zStart);
         if (zStart == UINT16_MAX)
            break;
         long zLen;
         GFREADVAL(pStream, uint16, long, zLen);
         if (valid)
         {
            long offset = x * mNumZTiles + zStart;
            long end = offset + zLen;
            for ( ; offset < end; offset++)
            {
               mMap[offset].team.setElement(teamID, cFogValue);
               mMap[offset].visibility |= teamBlackOffMask;
            }
         }
      }
   }

   return true;
}
