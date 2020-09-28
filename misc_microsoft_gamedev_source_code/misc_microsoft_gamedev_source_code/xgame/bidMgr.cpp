//==============================================================================
// bidmgr.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "aidebug.h"
#include "bid.h"
#include "bidmgr.h"
#include "protosquad.h"
#include "world.h"


// xcore
#include "containers\staticArray.h"


GFIMPLEMENTVERSION(BBidManager, 1);

//==============================================================================
// BBidManager::BBidManager()
// Constructor
//==============================================================================
BBidManager::BBidManager()
{
   mPlayerID = cInvalidPlayerID;
   mSquadGlobalQueueLimit = 1000000;
   mSquadTrainQueueLimit = 45;

   #ifndef BUILD_FINAL
      mNextDebugID = 1;
   #endif
}


//==============================================================================
// BBidManager::~BBidManager() 
// Destructor
//==============================================================================
BBidManager::~BBidManager()
{
   
}

//============================================================================
// class BBidScoreSortFunctor
//============================================================================
class BBidScoreSortFunctor
{
public:
   BBidScoreSortFunctor() {}
   bool operator() (const BBid* pLeft, const BBid* pRight) const
   {
      return (pLeft->getScore() > pRight->getScore());
   }
};

//==============================================================================
// BBidManager::update()
// Update all bids with bidScore, sort, set status.
//==============================================================================
void BBidManager::update()
{
   // Perform the update
   BStaticSimArray<BBid*, 64> workingBidList;

   float approvedSquadTimes = 0;
   // Set bidScores

   uint numBids = mBidIDs.getSize();
   for (uint i=0; i<numBids; i++)
   {
      BBid* pBid = gWorld->getBid(mBidIDs[i]);
      if (!pBid)
         return;

      BBidState bidState = pBid->getState();
      if ((bidState == BidState::cWaiting) || (bidState == BidState::cApproved))
	   {
         float totalCost = pBid->getTotalCost();
         float newScore = pBid->getEffectivePriority() * pBid->getElapsedTime();

         // Just check for >= 1.0f... can't have 0.5 resources...
         if (totalCost >= 1.0f)
            newScore /= totalCost;

		   pBid->setScore(newScore);

         //if(pBid->getType() == BidType::cSquad || (bidState == BidState::cApproved))
         //{
         //   BProtoSquadID squadID = pBid->getSquadToBuy();
//-- FIXING PREFIX BUG ID 4105
         //   const BProtoSquad* pProtoSquad = gDatabase.getProtoSquad(squadID);
//--
         //   if(pProtoSquad != NULL)
         //   {
         //      approvedSquadTimes += pProtoSquad->getBuildPoints();
         //   }
         //}
	   }

      workingBidList.add(pBid);
   }


   bool skipSquads = false;
   float queueTimes = 0;
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   pPlayer->getTrainQueueTimes(&queueTimes);
   if(mSquadGlobalQueueLimit < queueTimes + approvedSquadTimes)
   {
      skipSquads = true;
   }


   // Sort the array.
   std::sort(workingBidList.begin(), workingBidList.end(), BBidScoreSortFunctor());

   // Debug:
   //BBid* pBid0 = NULL;
   //BBid* pBid1 = NULL;
   //BBid* pBid2 = NULL;
   //if (workingBidList.getSize() > 0)
   //   pBid0 = workingBidList[0];
   //if (workingBidList.getSize() > 1)
   //   pBid1 = workingBidList[1];
   //if (workingBidList.getSize() > 2)
   //   pBid2 = workingBidList[2];

   // Set status and reservations
   BCost remaining = gWorld->getPlayer(mPlayerID)->getResources();

   // Iterate through list of bids, from highest bid score down.
   // Approve those that can be serviced.
   // Reserve resources for those that can't, while some resources remain.
   // Mark bids waiting or inactive if they're unable to be filled.
   uint numWorkingBids = workingBidList.getSize();
   for (uint i=0; i<numWorkingBids; i++)
   {
      BBid* pBid = workingBidList[i];
      BASSERT(pBid);
	   
      if (pBid->getWantToBuy() && (pBid->getState() != BidState::cInactive))
	   {
		   bool someRemain = false;
		   for (long resIndex=0; resIndex < remaining.getNumberResources(); resIndex++)
		   {
			   if (remaining.get(resIndex) > 0.0)
				   someRemain = true;
		   }
		   // If some resources remain, check if there's enough for this bid.
		   // If enough, set it to approved and remove it's cost from "remaining".
		   // If not enough, set it to waiting.
         BCost bidCost;
         pBid->getCost(bidCost);


         //check how much we are allowed to queue up production
         if(mSquadGlobalQueueLimit < queueTimes + approvedSquadTimes)
         {
            skipSquads = true;
         }
         //skip consideration of this squad if we are not allowed to build it right now
         if(skipSquads && pBid->getType() == BidType::cSquad)
            continue;


		   if (remaining.isAtLeast(bidCost))
		   {
            pBid->setState(BidState::cApproved);
			   remaining.subtractDeductableOnly(&bidCost);

            //Add train time burden to approvedSquadTimes
            if(pBid->getType() == BidType::cSquad)
            {
               BProtoSquadID squadID = pBid->getSquadToBuy();
               BProtoSquad* pProtoSquad = pPlayer ? pPlayer->getProtoSquad(squadID) : gDatabase.getGenericProtoSquad(squadID);
               if(pProtoSquad != NULL)
               {
                  approvedSquadTimes += pProtoSquad->getBuildPoints();
               }
            }
		   }
		   else
		   {
            pBid->setState(BidState::cWaiting);
			   remaining.subtractDeductableOnly(&bidCost);	// Reserve resources, even if total goes negative.

											// This prevents lower-score bids from spending stuff this one needs.
		   }
	   }
	   else
	   {
         pBid->setState(BidState::cInactive);	// Doesn't want to purchase anything, state is automatically inactive.
	   }
   }

}


//==============================================================================
// BBidManager::setQueueSettings
// sets global queuesettings
//==============================================================================
void BBidManager::setQueueSettings(float squadGlobalQueueLimit, float squadTrainQueueLimit)
{
   mSquadGlobalQueueLimit = squadGlobalQueueLimit;
   mSquadTrainQueueLimit = squadTrainQueueLimit;

}

//==============================================================================
// BBidManager::setBlockedBuildings
// sets buildings that are blocked from build consideration
//==============================================================================
void BBidManager::setBlockedBuildings(const BEntityIDArray &blockedBuilders)
{
   //mBlockedBuilders.clear();
   mBlockedBuilders.add(blockedBuilders.getPtr(),blockedBuilders.size());   
}

//==============================================================================
// BBidManager::save
//==============================================================================
bool BBidManager::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BBidID, mBidIDs, uint16, 500);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, float, mSquadGlobalQueueLimit);
   GFWRITEVAR(pStream, float, mSquadTrainQueueLimit);
   GFWRITEARRAY(pStream, BEntityID, mBlockedBuilders, uint16, 500);
   return true;
}

//==============================================================================
// BBidManager::load
//==============================================================================
bool BBidManager::load(BStream* pStream, int saveType)
{  
   GFREADARRAY(pStream, BBidID, mBidIDs, uint16, 500);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, float, mSquadGlobalQueueLimit);
   GFREADVAR(pStream, float, mSquadTrainQueueLimit);
   GFREADARRAY(pStream, BEntityID, mBlockedBuilders, uint16, 500);
   return true;
}
