//==============================================================================
// kb.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

//xgame
#include "entityid.h"
#include "aitypes.h"
#include "gamefilemacros.h"


// Forward declarations
class BKBBase;
class BKBBaseQuery;
class BPlayer;
class BSquad;
class BKBSquadQuery;
class BTeam;
class BUnit;


//==============================================================================
// class BKBPlayer
// The BKBPlayer class is info about a player from the perspective of another player.
// In other words, Each player has a list of BKBPlayers on its BKB that describe
// what it thinks every other player in the game has.
//==============================================================================
class BKBPlayer
{
public:

   // Constructor / Destructor
   BKBPlayer(){}
   ~BKBPlayer(){}

   void reset(BPlayerID playerID);
   void render();

   long getPlayerID() const { return(mPlayerID); }
   uint getNumberKBSquads() const { return (mKBSquadIDs.getSize()); }
   uint getNumberValidKBSquads() const;
   uint getNumberEmptyKBBases() const;
   const BKBBaseIDArray& getKBBaseIDs() const { return (mKBBaseIDs); }
   const BKBSquadIDArray& getKBSquadIDs() const { return (mKBSquadIDs); }


   void addKBSquad(BKBSquadID kbSquadID) { mKBSquadIDs.add(kbSquadID); }
   void removeKBSquad(BKBSquadID kbSquadID) { mKBSquadIDs.remove(kbSquadID); }
   bool containsKBSquad(BKBSquadID kbSquadID) { return (mKBSquadIDs.contains(kbSquadID)); }
   void addKBBase(BKBBaseID kbBaseID) { mKBBaseIDs.add(kbBaseID); }
   void removeKBBase(BKBBaseID kbBaseID) { mKBBaseIDs.remove(kbBaseID); }
   bool containsKBBase(BKBBaseID kbBaseID) { return (mKBBaseIDs.contains(kbBaseID)); }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BKBSquadIDArray mKBSquadIDs;              // All the squads owned by this player, uncategorized.
   BKBBaseIDArray mKBBaseIDs;                // All the bases owned by this player, uncategoriezed.
   BPlayerID mPlayerID;                      // This player.  :)
};



//==============================================================================
// class BKB
// Knowledge base of all the players, units, etc. that a player knows about.
//==============================================================================
class BKB
{
public:

   // Constructor / Destructor
   BKB(BTeamID teamID);
   BKB() {};
   ~BKB();

   enum
   {
      cClearExistingResults = 1 << 0,
   };

   void update();
   void releaseEmptyBases();
   uint executeKBSquadQuery(const BKBSquadQuery& kbSquadQuery, BKBSquadIDArray& resultsArray, uint flags) const;
   uint executeKBSquadQueryClosest(const BKBSquadQuery& kbSquadQuery, BVector testLocation, BKBSquadIDArray& resultsArray) const;
   uint executeKBBaseQuery(const BKBBaseQuery& kbBaseQuery, BKBBaseIDArray& resultsArray, uint flags) const;
   uint executeKBBaseQueryClosest(const BKBBaseQuery& kbBaseQuery, BVector testLocation, BKBBaseIDArray& resultsArray) const;

   BTeamID getTeamID() const { return(mTeamID); }

   // Do some updating.
   void acquireVisToSquad(BSquad* pSquad);
   void loseVisToSquad(BSquad* pSquad);
   void updateSquad(BSquad* pSquad);
   bool isSquadValidForKBSquad(const BSquad* pSquad) const;

   uint getNumberKBSquads() const;
   uint getNumberKBSquads(BPlayerID playerID) const;
   const BKBPlayer* getKBPlayer(BPlayerID playerID) const;
   BKBPlayer* getKBPlayer(BPlayerID playerID);
   uint getNumberBases() const;
   uint getNumberBases(BPlayerID playerID) const;


   #ifndef BUILD_FINAL
   void debugRender() const;
   #endif

   bool getFlagPaused() const { return (mFlagPaused); }
   void setFlagPaused(bool v) { mFlagPaused = v; }
   void toggleFlagPaused() { mFlagPaused = !mFlagPaused; }

   void addKBSquad(BKBSquadID kbSquadID) { mKBSquadIDs.add(kbSquadID); }
   void removeKBSquad(BKBSquadID kbSquadID) { mKBSquadIDs.remove(kbSquadID); }
   bool containsKBSquad(BKBSquadID kbSquadID) { return (mKBSquadIDs.contains(kbSquadID)); }

   void addKBBase(BKBBaseID kbBaseID) { mKBBaseIDs.add(kbBaseID); }
   void removeKBBase(BKBBaseID kbBaseID) { mKBBaseIDs.remove(kbBaseID); }
   bool containsKBBase(BKBBaseID kbBaseID) { return (mKBBaseIDs.contains(kbBaseID)); }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   void updateKBSquadEntry(BSquad* pSquad, BKBSquad& rKBSquadEntry);
   void putInCorrectBase(BKBSquad& rKBSquadEntry);

   // Data   
   BKBPlayer mKBPlayers[cMaximumSupportedPlayers];
   BKBSquadIDArray mKBSquadIDs;
   BKBBaseIDArray mKBBaseIDs;

   BTeamID mTeamID;                       // What team is the player this KB represents?

   DWORD mNextUpdateTime;

   bool mFlagEnabled : 1;
   bool mFlagPaused : 1;


   static const DWORD cKBRefreshPeriod = 0; // = 250;
};