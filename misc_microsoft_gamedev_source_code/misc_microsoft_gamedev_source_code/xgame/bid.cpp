//==============================================================================
// bid.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "bid.h"
#include "bidMgr.h"
#include "kb.h"
#include "player.h"
#include "protosquad.h"
#include "prototech.h"
#include "protopower.h"
#include "simhelper.h"
#include "team.h"
#include "unitquery.h"
#include "world.h"

GFIMPLEMENTVERSION(BBid, 1);

//==============================================================================
//==============================================================================
BBid::BBid()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
void BBid::resetNonIDData()
{
   mPlayerID = cInvalidPlayerID;
   mLastPurchaseTime = gWorld->getGametime();
   mWantToBuy = false;
   mbUseBuilderID = false;
   mbUseTargetLocation = false;
   mBuilderID = cInvalidObjectID;
   mSquadToBuy = cInvalidProtoSquadID;
   mTechToBuy = cInvalidTechID;
   mBuildingToBuy = cInvalidProtoObjectID;
   mScore = 0.0f;
   mPriority = 0.0f;
   mEffectivePriority = 0.0f;
   mPadSupplies = 0.0f;
   mState = BidState::cInactive;
   mType = BidType::cNone;

   #ifndef BUILD_FINAL
      mDebugID = 0;
   #endif
}


//==============================================================================
//==============================================================================
void BBid::getCost(BCost& cost) const
{
   // Zero it out to start with.
   cost.zero();

   // Get the player.
   BPlayerID playerID = getPlayerID();
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   // Get the cost for the appropriate type.
   if (isType(BidType::cSquad))
   {
      const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(mSquadToBuy);
      if (pProtoSquad)
         cost.set(pProtoSquad->getCost());
   }
   else if (isType(BidType::cTech))
   {
      const BProtoTech* pProtoTech = pPlayer->getProtoTech(mTechToBuy);
      if (pProtoTech)
         cost.set(pProtoTech->getCost());
   }
   else if (isType(BidType::cBuilding))
   {
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(mBuildingToBuy);
      if (pProtoObject)
      {
         pProtoObject->getCost(pPlayer, &cost, 0);
         const BCost* pCaptureCost = pProtoObject->getCaptureCost(pPlayer);
         if(pCaptureCost != NULL)
         {
            cost.add(pCaptureCost);
         }
      }
   }
   else if (isType(BidType::cPower))
   {
      BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mPowerToCast);
      if (pProtoPower && pProtoPower->getCost(NULL))
         cost.set(pProtoPower->getCost(NULL));
   }

   // Add our pad cost supplies
   cost.add(gDatabase.getRIDSupplies(), mPadSupplies);
}


//==============================================================================
//==============================================================================
float BBid::getTotalCost() const
{
   BCost tempCost;
   getCost(tempCost);
   return (tempCost.getTotal());
}


//==============================================================================
// BBid::~setWantToBuy()
// Clears the want to buy flag, sets status to inactive.  DOES NOT clear the 
// squad / tech / building fields.
//==============================================================================
void BBid::setWantToBuy(bool newValue)
{
   mWantToBuy = newValue;
   mState = BidState::cInactive;
}


//==============================================================================
// BBid::setUnitToBuy()
// Set this bid to buy one of protosquad specified, clear other fields,
// set WantToBuy flag.
//==============================================================================
void BBid::setSquadToBuy(BProtoSquadID protoSquadID)
{
   mSquadToBuy = protoSquadID;
   mTechToBuy = cInvalidTechID;
   mBuildingToBuy = cInvalidProtoObjectID;
   mPowerToCast = cInvalidProtoPowerID;
   mWantToBuy = true;
   mState = BidState::cWaiting;
   mType = BidType::cSquad;
}


//==============================================================================
// BBid::setTechToBuy()
// Set this bid to buy the tech specified, clear other fields,
// set WantToBuy flag.
//==============================================================================
void BBid::setTechToBuy(BProtoTechID techID)
{
   mSquadToBuy = cInvalidProtoSquadID;
   mTechToBuy = techID;
   mBuildingToBuy = cInvalidProtoObjectID;
   mPowerToCast = cInvalidProtoPowerID;
   mWantToBuy = true;
   mState = BidState::cWaiting;
   mType = BidType::cTech;
}


//==============================================================================
// BBid::setBuildingToBuy()
// Set this bid to buy the building specified, clear other fields,
// set WantToBuy flag.
//==============================================================================
void BBid::setBuildingToBuy(BProtoObjectID protoUnitID)
{
   mSquadToBuy = cInvalidProtoSquadID;
   mTechToBuy = cInvalidTechID;
   mBuildingToBuy = protoUnitID;
   mPowerToCast = cInvalidProtoPowerID;
   mWantToBuy = true;
   mState = BidState::cWaiting;
   mType = BidType::cBuilding;
}


//==============================================================================
// BBid::setPowerToCast()
// set WantToBuy flag.
//==============================================================================
void BBid::setPowerToCast(BProtoPowerID protoPowerID)
{
   mSquadToBuy = cInvalidProtoSquadID;
   mTechToBuy = cInvalidTechID;
   mBuildingToBuy = cInvalidProtoObjectID;
   mPowerToCast = protoPowerID;
   mWantToBuy = true;
   mState = BidState::cWaiting;
   mType = BidType::cPower;
}


//==============================================================================
// BBid::setPriority()
// Convert the 0.0f - 1.0f range into something more dynamic, so that pri .80 >> pri .70.  
// Starting with 2 to the 10p, so if P2-P1 = .40, P2 is 16 times higher. 
// May need to adjust it to 10 to the 10p if we need more dynamic range, so a P 100. totally dominates a P 0.90.
//==============================================================================
void BBid::setPriority(float priority)
{
   BASSERT(priority >= 0.0f);
   BASSERT(priority <= 100.0f);
   mPriority = priority;
   mEffectivePriority = powf(2.0f, (priority * 10.0f));   // Exponential, 2 to the (priority*10)
}


//==============================================================================
// BBid::clear()
// Clears Squad, Tech, Building and Priority fields
//==============================================================================
void BBid::clear()
{
   mSquadToBuy = cInvalidProtoSquadID;
   mTechToBuy = cInvalidTechID;
   mBuildingToBuy = cInvalidProtoObjectID;
   mPowerToCast = cInvalidProtoPowerID;
   mPriority = 0.0f;
   mEffectivePriority = 0.0f;
   mPadSupplies = 0.0f;
   mState = BidState::cInactive;
   mPriority = BidType::cNone;
   mbUseBuilderID = false;
   mbUseTargetLocation = false;
   mBuilderID = cInvalidObjectID;
}


//==============================================================================
//==============================================================================
bool BBid::purchaseTech(BVector& resultLocation)
{
   // Get the player.
   BPlayerID playerID = getPlayerID();
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);

   // Check affordability.
   BCost bidCost;
   getCost(bidCost);
   if (!pPlayer->checkCost(&bidCost))
      return (false);

   // Get the thing we are "building" out of.  (this sucks.)
   BUnit* pBuilder = NULL;
   if (mbUseBuilderID)
      pBuilder = gWorld->getUnit(mBuilderID);
   if (!pBuilder)
   {
      if (mbUseTargetLocation)
         pBuilder = pPlayer->getBuildingToResearchTech(mTechToBuy, &mTargetLocation);
      else
         pBuilder = pPlayer->getBuildingToResearchTech(mTechToBuy, NULL);
   }

   // We need a building to research from
   if (!pBuilder)
      return (false);

   // But I don't own it!
   if (pBuilder->getPlayerID() != playerID)
      return (false);

   // Research!
   bool researchSuccess = pBuilder->doResearch(playerID, mTechToBuy, 1);
   //BASSERT(researchSuccess);
   mTechToBuy = cInvalidTechID;
   mState = BidState::cInactive;
   mLastPurchaseTime = gWorld->getGametime();
   resultLocation = pBuilder->getPosition();
   return (researchSuccess);
}

//==============================================================================
//==============================================================================
bool BBid::purchaseSquad(BVector& resultLocation)
{
   // Get the player.
   BPlayerID playerID = getPlayerID();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return (false);
   BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return (false);

   // Check affordability.
   BCost bidCost;
   getCost(bidCost);
   if (!pPlayer->checkCost(&bidCost))
      return (false);

   // Get the thing we are "building" out of.  (this sucks.)
   BUnit* pBuilder = NULL;
   if (mbUseBuilderID)
      pBuilder = gWorld->getUnit(mBuilderID);
   if (!pBuilder)
   {
      if (mbUseTargetLocation)
         pBuilder = pPlayer->getBuildingToTrainSquad(mSquadToBuy, &mTargetLocation);
      else
         pBuilder = pPlayer->getBuildingToTrainSquad(mSquadToBuy, NULL);
   }

   // We need a building to research from
   if (!pBuilder)
      return (false);

   // But I don't own it!
   if (pBuilder->getPlayerID() != playerID)
      return (false);


   long count;
   float time;
   pBuilder->getTrainQueue(playerID, &count, &time);
   if(count > 1 && time > pBidManager->getSquadTrainQueueLimit()) 
   {
      return false;
   }


   // Train!
   bool trainSuccess = pBuilder->doTrain(playerID, mSquadToBuy, 1, true, false);
   //BASSERT(trainSuccess);
   mSquadToBuy = cInvalidTechID;
   mState = BidState::cInactive;
   mLastPurchaseTime = gWorld->getGametime();
   resultLocation = pBuilder->getPosition();
   return (trainSuccess);
}

//==============================================================================
//==============================================================================
bool BBid::purchaseBuilding(BVector& resultLocation)
{
   // Get the player.
   BPlayerID playerID = getPlayerID();
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return (false);
   BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return (false);

   // This would be stupid.
//-- FIXING PREFIX BUG ID 1586
   const BProtoObject* pStructureProtoID = pPlayer->getProtoObject(mBuildingToBuy);
//--
   if (!pStructureProtoID)
      return (false);

   // Check affordability.
   BCost bidCost;
   getCost(bidCost);
   if (!pPlayer->checkCost(&bidCost))
      return (false);

   const BEntityIDArray& blockedBuilders =  pBidManager->getBlockedBuilders();

   // Get the thing we are "building" out of.  (this sucks.)
   BUnit* pBuilder = NULL;
   if (mbUseBuilderID)
      pBuilder = gWorld->getUnit(mBuilderID);
   if (!pBuilder)
   {
      if (mbUseTargetLocation)
         pBuilder = pPlayer->getBuildingToBuildStructure(mBuildingToBuy, &mTargetLocation, blockedBuilders);
      else
         pBuilder = pPlayer->getBuildingToBuildStructure(mBuildingToBuy, NULL, blockedBuilders);
   }

   // We need a building to research from
   if (!pBuilder)
      return (false);

   bool isFlagCommandableByAnyPlayer = pBuilder->getProtoObject()->getFlagCommandableByAnyPlayer();

   // But I don't own it!
   if (!isFlagCommandableByAnyPlayer && pBuilder->getPlayerID() != playerID)
      return (false);

   // Construct!
   int socketTypeID = pStructureProtoID->getSocketID();

   BVector centerPoint;
   if (mbUseTargetLocation && socketTypeID == -1)
      centerPoint = mTargetLocation;
   else
      centerPoint = pBuilder->getPosition();

   BVector placementSuggestion  = cInvalidVector;
   bool placementResult = false;
   BEntityID socketUnitID = cInvalidObjectID;
   long searchScale = 1;
   DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain | BWorld::cCPSetPlacementSuggestion;

   if (socketTypeID == -1)
   {
      float outerRadius = 100.0f;
      float innerRadius = 15.0f;
      for(int tries = 0; tries < 40; tries++)
      {
         BVector randomLoc = BSimHelper::randomCircularDistribution(centerPoint, outerRadius, innerRadius);
         placementResult = gWorld->checkPlacement(mBuildingToBuy, playerID, randomLoc, placementSuggestion, cZAxisVector, BWorld::cCPLOSDontCare, flags, searchScale, pBuilder, &socketUnitID);
         if (placementResult && placementSuggestion != cInvalidVector)
            break;
      }
   }
   else
   {
      BEntityIDArray sockets;
      gWorld->getBuildSockets(playerID, mBuildingToBuy, pBuilder, cOriginVector, 0.0f, true, true, sockets);      
      uint numAvailableSockets = sockets.getSize();

      BUnit* pClosestBuilder = NULL;
      float bestDist = cMaximumFloat;

      for (uint i=0; i<numAvailableSockets; i++)
      {
         BUnit* pSocketUnit = gWorld->getUnit(sockets[i]);
         if (!pSocketUnit)
            continue;
         placementResult = gWorld->checkPlacement(mBuildingToBuy, playerID, pSocketUnit->getPosition(), placementSuggestion, cZAxisVector, BWorld::cCPLOSDontCare, flags, searchScale, pBuilder, &socketUnitID);
         pBuilder = pSocketUnit;

         if (placementResult)
         {
            if (mbUseTargetLocation == false) 
            {
               break;
            }
            else
            {
               const BVector pos = pSocketUnit->getPosition();
               float dist = pos.distanceSqr(mTargetLocation);
               if(dist < bestDist)
               {
                  pClosestBuilder = pSocketUnit;
                  bestDist = dist;
               }
            }
         }
      }
      if(mbUseTargetLocation == true) 
      {
         pBuilder = pClosestBuilder;
      }
   }

   if (!placementResult)
      return (false);

   BASSERT(placementSuggestion != cInvalidVector);
   const BProtoObject* pProtoObject = pBuilder->getProtoObject();
   BASSERT(pProtoObject);
   uint numCommands = pProtoObject->getNumberCommands();
   bool buildSuccess = false;
   for (uint i=0; i<numCommands; i++)
   {
      BProtoObjectCommand command = pProtoObject->getCommand(i);
      if (command.getType() == BProtoObjectCommand::cTypeBuild && command.getID() == mBuildingToBuy)
      {
         BEntityID entityID = gWorld->createEntity(mBuildingToBuy, false, playerID, placementSuggestion, cZAxisVector, cXAxisVector, false, false, false, cInvalidObjectID, cInvalidPlayerID, pBuilder->getID(), socketUnitID);
         if (gWorld->getEntity(entityID))
            buildSuccess = true;
         else
            buildSuccess = false;
         break;
      }
      //Build other is used for nodes
      if (command.getType() == BProtoObjectCommand::cTypeBuildOther && command.getID() == mBuildingToBuy)
      {
         pBuilder->doBuildOther(playerID, playerID, command.getID(), false, false);
         buildSuccess = true;
      }
   }
   if (!buildSuccess)
      return (false);
   
   mState = BidState::cInactive;
   mBuildingToBuy = cInvalidProtoObjectID;
   mLastPurchaseTime = gWorld->getGametime();	// Resets timer to lower priority.
   resultLocation = placementSuggestion;
   return (true);
}


//==============================================================================
//==============================================================================
bool BBid::purchasePower(BVector& resultLocation)
{
   // Get the power.
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mPowerToCast);
   if (!pProtoPower)
      return (false);

   // Get the player.
   BPlayerID playerID = getPlayerID();
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);

   // Check affordability.
   BCost bidCost;
   getCost(bidCost);
   if (!pPlayer->checkCost(&bidCost))
      return (false);

   // Purchase the power.
   mState = BidState::cInactive;
   mPowerToCast = cInvalidProtoPowerID;
   mLastPurchaseTime = gWorld->getGametime();
   resultLocation = cOriginVector;
   return (true);
}


//==============================================================================
//==============================================================================
// BBid::purchase()
// Used at the instant the tech/item has been purchased.
// Must specify a building to be used
// or a location (which will be used to find a building if needed).
// For building purchases, location marks the center of the placement search
// area.  If no location is specified, the building's location will be used.
// This function clears the "to buy" member variable, the cost variables, and
// resets the timer ONLY if successful.  If it fails, the bid will remain approved.
//==============================================================================
bool BBid::purchase(BVector& resultLocation)
{
   if (isType(BidType::cSquad))
      return (purchaseSquad(resultLocation));
   else if (isType(BidType::cTech))
      return (purchaseTech(resultLocation));
   else if (isType(BidType::cBuilding))
      return (purchaseBuilding(resultLocation));
   else if (isType(BidType::cPower))
      return (purchasePower(resultLocation));
   else
   {
      BASSERTM(false, "Trying to purchase a bid of unknown type.");
      return (false);
   }
}


//==============================================================================
// BBid::getElapsedTime()
//==============================================================================
DWORD BBid::getElapsedTime() const
{
	return(gWorld->getGametime() - mLastPurchaseTime);
}

//==============================================================================
// BBid::save
//==============================================================================
bool BBid::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mTargetLocation);
   GFWRITEVAR(pStream, BEntityID, mBuilderID);
   GFWRITEVAR(pStream, BBidID, mID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, DWORD, mLastPurchaseTime);
   GFWRITEVAR(pStream, float, mScore);
   GFWRITEVAR(pStream, float, mPriority);
   GFWRITEVAR(pStream, float, mEffectivePriority);
   GFWRITEVAR(pStream, float, mPadSupplies);
   GFWRITEVAR(pStream, BProtoSquadID, mSquadToBuy);
   GFWRITEVAR(pStream, BProtoTechID, mTechToBuy);
   GFWRITEVAR(pStream, BProtoObjectID, mBuildingToBuy);
   GFWRITEVAR(pStream, BProtoPowerID, mPowerToCast);

   GFWRITEVAR(pStream, BBidState, mState);
   GFWRITEVAR(pStream, BBidType, mType);

   GFWRITEBITBOOL(pStream, mWantToBuy);
   GFWRITEBITBOOL(pStream, mbUseTargetLocation);
   GFWRITEBITBOOL(pStream, mbUseBuilderID);

#ifndef BUILD_FINAL
   GFWRITEVAR(pStream, uint, mDebugID);
#else
   GFWRITEVAL(pStream, uint, 0);
#endif

   return true;
}

//==============================================================================
// BBid::load
//==============================================================================
bool BBid::load(BStream* pStream, int saveType)
{  
   GFREADVECTOR(pStream, mTargetLocation);
   GFREADVAR(pStream, BEntityID, mBuilderID);
   GFREADVAR(pStream, BBidID, mID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, DWORD, mLastPurchaseTime);
   GFREADVAR(pStream, float, mScore);
   GFREADVAR(pStream, float, mPriority);
   GFREADVAR(pStream, float, mEffectivePriority);
   GFREADVAR(pStream, float, mPadSupplies);
   GFREADVAR(pStream, BProtoSquadID, mSquadToBuy);
   GFREADVAR(pStream, BProtoTechID, mTechToBuy);
   GFREADVAR(pStream, BProtoObjectID, mBuildingToBuy);
   GFREADVAR(pStream, BProtoPowerID, mPowerToCast);

   GFREADVAR(pStream, BBidState, mState);
   GFREADVAR(pStream, BBidType, mType);

   GFREADBITBOOL(pStream, mWantToBuy);
   GFREADBITBOOL(pStream, mbUseTargetLocation);
   GFREADBITBOOL(pStream, mbUseBuilderID);

#ifndef BUILD_FINAL
   GFREADVAR(pStream, uint, mDebugID);
#else
   GFREADTEMPVAL(pStream, uint);
#endif

   return true;
}
