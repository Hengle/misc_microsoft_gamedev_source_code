//=============================================================================
// Copyright (c) 2006 Ensemble Studios
//
// Visible map
//=============================================================================

#pragma once

//=============================================================================
// Includes
//=============================================================================
#include "MaximumSupportedPlayers.h"
#include "simtypes.h"
#include "bitelement.h"
#include "gamefilemacros.h"

//=============================================================================
// Definitions
//=============================================================================
#define BVISIBLEMAP_MAX_RADIUS   32
#define cFogMask                 0xFFFF0000
#define cBlackMask               0x0000FFFF
#define cRefCountNumBits         9

typedef BBitElement16<cRefCountNumBits, cMaximumSupportedTeams> BRefCountElements;

//=============================================================================
// Forward declarations
//=============================================================================
class BNonconvexHull;
class BConvexHull;
class BTileSpanList;

//=============================================================================
// Class definition
//=============================================================================
class BVisibleMap
{

public:

   enum
   {
      cNotVisible,
      cVisible,
      cFogged
   };

   enum
   {
      cTile0Mask = 0x01,
      cTile1Mask = 0x02,
      cTile2Mask = 0x04,
      cTile3Mask = 0x08,
      cTile4Mask = 0x10,
      cTile5Mask = 0x20,
      cTile6Mask = 0x40,
      cTile7Mask = 0x80
   };

   // Constructor/Destructor
   BVisibleMap();
   ~BVisibleMap();

   // Initializer/Deinitializer
   bool  setup();
   void  shutdown();

   void  initMap(long numXTiles, long numZTiles);
   void  deinitMap(void);

   // Update
   void  updateEntireMap(BTeamID teamID);
   bool  updateCircularRegion(long oldX, long oldZ, long newX, long newZ, long radius, BTeamID teamID);

   // Explore/Unexplore
   void  explore(long x, long z, BTeamID teamID);
   void  unexplore(long x, long z, BTeamID teamID);
   void  resetBlackMap(long x, long z, BTeamID teamID);
   void  exploreEntireMap(BTeamID teamID);
   void  unexploreEntireMap(BTeamID teamID);
   void  resetBlackMap(BTeamID teamID);
   bool  exploreCircularRegion(long x, long z, long radius, BTeamID teamID);
   bool  unexploreCircularRegion(long x, long z, long radius, BTeamID teamID);
   void  explore(long startX, long startZ, long endX, long endZ, BTeamID teamID);
   void  unexplore(long startX, long startZ, long endX, long endZ, BTeamID teamID);
   void  exploreHull(const BNonconvexHull &hull, BTeamID teamID);
   void  unexploreHull(const BNonconvexHull &hull, BTeamID teamID);

   // Block/Unblock
   void  block(long x, long z, BTeamID teamID);
   void  unblock(long x, long z, BTeamID teamID);
   void  blockEntireMap(BTeamID teamID);
   void  unblockEntireMap(BTeamID teamID);
   bool  blockCircularRegion(long x, long z, long radius, BTeamID teamID);
   bool  unblockCircularRegion(long x, long z, long radius, BTeamID teamID);
   void  block(long startX, long startZ, long endX, long endZ, BTeamID teamID);
   void  unblock(long startX, long startZ, long endX, long endZ, BTeamID teamID);
   void  blockHull(const BConvexHull &hull, BTeamID teamID);
   void  unblockHull(const BConvexHull &hull, BTeamID teamID);

   // Accessors
   inline DWORD getVisibility(long x, long z) const;
   DWORD getCircularEdgeVisibility(long x, long z, long radius);
   //BYTE  getSurroundCode(long x, long z, DWORD mask) const;
   long  getMaxXTiles(void) const                       { return mNumXTiles; }
   long  getMaxZTiles(void) const                       { return mNumZTiles; }
   DWORD  getTeamBlackOffMask(BTeamID teamID) const      { return (0x01 << teamID); }
   DWORD  getTeamFogOffMask(BTeamID teamID) const        { return (0x10000 << teamID); }
   DWORD  getTeamMask(BTeamID teamID) const              { return (0x10001 << teamID); }
   WORD   getTeamBlockedMask(short teamID) const         { return (0x01 << teamID); }
   WORD   getTeamBlockedMask(BTeamID teamID) const       { return (0x01 << short(teamID)); } // shut the compiler up

   bool isPositionVisibleToTeam(BVector pos, BTeamID teamID) const;
   bool isPositionExploredToTeam(BVector pos, BTeamID teamID) const;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   void resetVisibilityTexture();
   BYTE  checkMask(long x, long z, DWORD mask) const;

protected:
   void generateSpanlists(long numFixedLists, long numMoveableLists);
   bool loadSpanlists(long numFixedLists, long numMoveableLists);
   void saveSpanlists(long numFixedLists, long numMoveableLists);

   struct BVisibilityTile
   {
      DWORD    visibility;
      WORD     blocked;
      BRefCountElements team;
   };

   void calcSpanDiff(long radius, long dir, BTileSpanList& tmpList);

   long                 mNumXTiles, mNumZTiles;
   float                mTileScale, mRecTileScale;
   BTileSpanList        *mEntireRadiusLists;
   BTileSpanList        **mAddPartialRadiusLists;
   BTileSpanList        **mDelPartialRadiusLists;
   BVisibilityTile      *mMap;
};

inline DWORD BVisibleMap::getVisibility(long x, long z) const
{
   return mMap[(x * mNumZTiles) + z].visibility;
}

extern BVisibleMap gVisibleMap;
