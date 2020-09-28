//==============================================================================
// bidmgr.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once


// xgame
#include "bid.h"
#include "player.h"
#include "gamefilemacros.h"


// forward declarations
class BBid;


//==============================================================================
// class BBidManager
// Manages all active bids for one player
//==============================================================================
class BBidManager
{
public:

   BBidManager();
   ~BBidManager();

   BPlayerID getPlayerID() const { return(mPlayerID); }
   void setPlayerID(BPlayerID playerID) { mPlayerID = playerID; }
   void update();

   const BBidIDArray& getBidIDs() const { return (mBidIDs); }

   void addBid(BBidID bidID) { mBidIDs.add(bidID); }
   void removeBid(BBidID bidID) { mBidIDs.remove(bidID); }
   bool containsBid(BBidID bidID) { return (mBidIDs.contains(bidID)); }

   void setQueueSettings(float squadGlobalQueueLimit, float squadTrainQueueLimit);
   void setBlockedBuildings(const BEntityIDArray &blockedBuilders);
   void clearBlockedBuildings(){mBlockedBuilders.clear();}
   const BEntityIDArray &getBlockedBuilders(){return mBlockedBuilders;}

   float getSquadTrainQueueLimit() {return mSquadTrainQueueLimit;}

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BBidIDArray mBidIDs;       // Dynamic array of in-use bids.
   BPlayerID mPlayerID;       // The player that owns this list of bids
   //DWORD mLastUpdateTime;     // Last time the bid list was updated, in milliseconds of game time.

   float mSquadGlobalQueueLimit;
   float mSquadTrainQueueLimit;
   BEntityIDArray mBlockedBuilders;

   #ifndef BUILD_FINAL
      uint mNextDebugID;
   #endif
};