//==============================================================================
// kb.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "configsgame.h"
#include "game.h"
#include "kb.h"
#include "kbsquad.h"
#include "player.h"
#include "protoobject.h"
#include "protosquad.h"
#include "team.h"
#include "user.h"
#include "usermanager.h"
#include "visiblemap.h"
#include "world.h"

GFIMPLEMENTVERSION(BKB, 1);

//==============================================================================
//==============================================================================
void BKBPlayer::reset(BPlayerID playerID)
{
   mPlayerID = playerID;
   mKBBaseIDs.clear();
   mKBSquadIDs.clear();
}


//==============================================================================
//==============================================================================
void BKBPlayer::render()
{
   SCOPEDSAMPLE(BKBPlayer_render);
#ifndef BUILD_FINAL

#endif
}


//==============================================================================
//==============================================================================
uint BKBPlayer::getNumberValidKBSquads() const
{
   uint numValidKBSquads = 0;
   uint numKBSquadIDs = mKBSquadIDs.getSize();
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(mKBSquadIDs[i]);
      if (pKBSquad && pKBSquad->getSquad())
         numValidKBSquads++;
   }
   return (numValidKBSquads);
}


//==============================================================================
//==============================================================================
uint BKBPlayer::getNumberEmptyKBBases() const
{
   uint numEmptyKBBases = 0;
   uint numKBBaseIDs = mKBBaseIDs.getSize();
   for (uint i=0; i<numKBBaseIDs; i++)
   {
      const BKBBase* pKBBase = gWorld->getKBBase(mKBBaseIDs[i]);
      if (pKBBase && pKBBase->getKBSquadIDs().getSize() == 0)
         numEmptyKBBases++;
   }
   return (numEmptyKBBases);
}


//==============================================================================
// BKB::BKB()
// Constructor
//==============================================================================
BKB::BKB(BTeamID teamID)
{
   mTeamID = teamID;   

   // Reset our lookup for KB players.
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
      mKBPlayers[i].reset(static_cast<BPlayerID>(i));

   mNextUpdateTime = 0;
   mFlagEnabled = true;
   mFlagPaused = false;
}


//==============================================================================
// BKB::~BKB()
// Destructor
//==============================================================================
BKB::~BKB()
{
   BKBSquadIDArray tempKBSquadIDs = mKBSquadIDs;
   for (uint i=0; i<tempKBSquadIDs.getSize(); i++)
      gWorld->deleteKBSquad(tempKBSquadIDs[i]);

   BKBBaseIDArray tempKBBaseIDs = mKBBaseIDs;
   for (uint i=0; i<tempKBBaseIDs.getSize(); i++)
      gWorld->deleteKBBase(tempKBBaseIDs[i]);
}


//==============================================================================
//==============================================================================
void BKB::update()
{
   SCOPEDSAMPLE(BKB_update);

   // Don't do nothing.
   if (!mFlagEnabled)
      return;
   if (mFlagPaused)
      return;

   // Only update the KB periodically.
   DWORD currentGameTime = gWorld->getGametime();
   if (currentGameTime >= mNextUpdateTime)
   {
      static BKBSquadIDArray tempKBSquadIDs;
      tempKBSquadIDs.assignNoDealloc(mKBSquadIDs);
      uint numKBSquads = mKBSquadIDs.getSize();
      for (uint i=0; i<numKBSquads; i++)
      {
         BKBSquad* pKBSquad = gWorld->getKBSquad(tempKBSquadIDs[i]);
         if (!pKBSquad)
            continue;

         // If we're currently visible, update the entry.
         if (pKBSquad->getCurrentlyVisible())
         {
            updateSquad(pKBSquad->getSquad());
         }
         // If we're not visible, but we can see the last known position, flag it as 'invalidated'
         else if (pKBSquad->getLastKnownPosValid())
         {
            if (gVisibleMap.isPositionVisibleToTeam(pKBSquad->getPosition(), mTeamID))
            {
               pKBSquad->setLastKnownPosValid(false);

               BKBBase* pOldBase = gWorld->getKBBase(pKBSquad->getKBBaseID());
               if (pOldBase)
                  pOldBase->removeKBSquad(tempKBSquadIDs[i]);
            }
         }
      }

      static BKBBaseIDArray tempKBBaseIDs;
      tempKBBaseIDs.assignNoDealloc(mKBBaseIDs);
      uint numKBBases = mKBBaseIDs.getSize();
      for (uint i=0; i<numKBBases; i++)
      {
         BKBBase* pKBBase = gWorld->getKBBase(tempKBBaseIDs[i]);
         if (pKBBase)
         {
            pKBBase->update();

            BKBPlayer* pKBPlayer = getKBPlayer(pKBBase->getPlayerID());
            if (pKBPlayer)
            {
               const BKBBaseIDArray& playerKBBases = pKBPlayer->getKBBaseIDs();
               uint numPlayerBases = playerKBBases.getSize();
               for (uint p=0; p<numPlayerBases; p++)
               {
                  BKBBase* pPlayerBase = gWorld->getKBBase(playerKBBases[p]);
                  if (pPlayerBase && pKBBase->canMergeIntoBase(*pPlayerBase))
                  {
                     pKBBase->mergeIntoBase(*pPlayerBase);
                     break;
                  }
               }
            }
         }
      }


      // Release all our empty bases.
      releaseEmptyBases();

      // Setup for the next update
      mNextUpdateTime = currentGameTime + cKBRefreshPeriod;
   }

   #ifndef BUILD_FINAL
   //debugRender();
   #endif
}


//==============================================================================
//==============================================================================
void BKB::releaseEmptyBases()
{
   static BKBBaseIDArray cachedKBBaseIDs;
   cachedKBBaseIDs.assignNoDealloc(mKBBaseIDs);
   uint numKBBases = mKBBaseIDs.getSize();
   for (uint i=0; i<numKBBases; i++)
   {

      BKBBase* pKBBase = gWorld->getKBBase(cachedKBBaseIDs[i]);
      if (pKBBase && pKBBase->getKBSquadIDs().getSize() == 0)
         gWorld->deleteKBBase(cachedKBBaseIDs[i]);
   }
}


//==============================================================================
//==============================================================================
uint BKB::executeKBSquadQuery(const BKBSquadQuery& kbSquadQuery, BKBSquadIDArray& resultsArray, uint flags) const
{
   // Clear our results.
   if (flags & BKB::cClearExistingResults)
      resultsArray.resize(0);

   long numPlayers = gWorld->getNumberPlayers();
   for (long p=0; p<numPlayers; p++)
   {
      // Get our KBPlayer for this player.
      const BKBPlayer* pKBPlayer = getKBPlayer(p);
      if (!pKBPlayer)
         continue;

      if (kbSquadQuery.getFlagPlayerRelation() && kbSquadQuery.getPlayerRelation() != cRelationTypeAny)
      {
         BRelationType relationType = kbSquadQuery.getPlayerRelation();
         if (relationType == cRelationTypeSelf && (p != kbSquadQuery.getPlayerID()))
         {
            continue;
         }
         else if (relationType == cRelationTypeAlly)
         {
            const BPlayer* pRelationPlayer = gWorld->getPlayer(kbSquadQuery.getPlayerID());
            if (!pRelationPlayer->isAlly(p, kbSquadQuery.getSelfAsAlly()))
               continue;
         }
         else if (relationType == cRelationTypeEnemy)
         {
            const BPlayer* pRelationPlayer = gWorld->getPlayer(kbSquadQuery.getPlayerID());
            if (!pRelationPlayer->isEnemy(p))
               continue;
         }
         else if (relationType == cRelationTypeNeutral)
         {
            const BPlayer* pRelationPlayer = gWorld->getPlayer(kbSquadQuery.getPlayerID());
            if (!pRelationPlayer->isNeutral(p))
               continue;
         }
      }

      // We haven't excluded this player yet, so get the possible matches from the player.
      const BKBSquadIDArray& possibleMatches = pKBPlayer->getKBSquadIDs();
      uint numPossibleMatches = possibleMatches.getSize();

      // Now try to exclude the possible matches.
      for (uint i=0; i<numPossibleMatches; i++)
      {
         // Get the KB squad.
         const BKBSquad* pKBSquad = gWorld->getKBSquad(possibleMatches[i]);
         BASSERT(pKBSquad);
         if (!pKBSquad)
            continue;

         // Filter by the currently visible status.
         if (kbSquadQuery.getFlagCurrentlyVisible() && kbSquadQuery.getCurrentlyVisible() != pKBSquad->getCurrentlyVisible())
            continue;

         // Filter by the base of the entry
         if (kbSquadQuery.getFlagBase() && kbSquadQuery.getBaseID() != pKBSquad->getKBBaseID())
            continue;

         // Filter by staleness of the entry
         if (kbSquadQuery.getFlagMaxStaleness() || kbSquadQuery.getFlagMinStaleness())
         {
            DWORD staleness = pKBSquad->getStaleness();
            if (kbSquadQuery.getFlagMaxStaleness() && staleness > kbSquadQuery.getMaxStaleness())
               continue;
            if (kbSquadQuery.getFlagMinStaleness() && staleness < kbSquadQuery.getMinStaleness())
               continue;
         }

         // Filter by include of buildings-ness...?
         if (kbSquadQuery.getFlagIncludeBuildings())
         {
            bool includeBuildings = kbSquadQuery.getIncludeBuildings();
            if (includeBuildings && !pKBSquad->containsBuilding())
               continue;
            if (!includeBuildings && pKBSquad->containsBuilding())
               continue;
         }

         // Filter by point radius.
         if (kbSquadQuery.getFlagPointRadius())
         {
            // no valid last known pos, so don't bother.
            if (!pKBSquad->getCurrentlyVisible() && !pKBSquad->getLastKnownPosValid())
               continue;
            BVector pos = kbSquadQuery.getPosition();
            float rad = kbSquadQuery.getRadius();
            float dist = pKBSquad->getPosition().distance(pos);
            if (dist > rad)
               continue;
         }

         // Filter by object type
         if (kbSquadQuery.getFlagObjectType())
         {
            bool hasOtid = false;
            const BPlayer* pCurrentPlayer = gWorld->getPlayer(kbSquadQuery.getPlayerID());
//-- FIXING PREFIX BUG ID 3401
            const BProtoSquad* pProtoSquad = NULL;
//--
            if (pCurrentPlayer)
               pProtoSquad = pCurrentPlayer->getProtoSquad(pKBSquad->getProtoSquadID());
            else
               pProtoSquad = gDatabase.getGenericProtoSquad(pKBSquad->getProtoSquadID());

            if (pProtoSquad)
            {
               long numUnitNodes = pProtoSquad->getNumberUnitNodes();
               for (long i=0; i<numUnitNodes; i++)
               {
                  const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
//-- FIXING PREFIX BUG ID 3400
                  const BProtoObject* pProtoObject = NULL;
//--
                  if (pCurrentPlayer)
                     pProtoObject = pCurrentPlayer->getProtoObject(node.mUnitType);
                  else
                     pProtoObject = gDatabase.getGenericProtoObject(node.mUnitType);

                  if (!pProtoObject)
                     continue;
                  if (pProtoObject->isType(kbSquadQuery.getObjectTypeID()))
                  {
                     hasOtid = true;
                  }
               }
            }
            if(hasOtid == false)
               continue;

         }

         // Add new KBSquad criteria here...

         // THIS MATCH FITS OUR CRITERIA.  HOORAY!
         if (flags & BKB::cClearExistingResults)
            resultsArray.add(possibleMatches[i]);
         else
            resultsArray.uniqueAdd(possibleMatches[i]);
      }
   }

   // How many did we get?
   return (resultsArray.getSize());
}


//==============================================================================
//==============================================================================
uint BKB::executeKBSquadQueryClosest(const BKBSquadQuery& kbSquadQuery, BVector testLocation, BKBSquadIDArray& resultsArray) const
{
   // Clear our results.
   resultsArray.clear();
   float closestDistSqr = cMaximumFloat;
   BKBSquadID closestKBSquad = cInvalidKBSquadID;

   BKBSquadIDArray preliminaryResults;
   uint numPreliminaryResults = executeKBSquadQuery(kbSquadQuery, preliminaryResults, BKB::cClearExistingResults);
   for (uint i=0; i<numPreliminaryResults; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(preliminaryResults[i]);
      if (!pKBSquad)
         continue;
      if (!pKBSquad->getCurrentlyVisible() && !pKBSquad->getLastKnownPosValid())
         continue;
      float testDistSqr = pKBSquad->getPosition().distanceSqr(testLocation);
      if (testDistSqr < closestDistSqr)
      {
         closestDistSqr = testDistSqr;
         closestKBSquad = pKBSquad->getID();
      }
   }

   if (closestKBSquad != cInvalidKBSquadID)
      resultsArray.add(closestKBSquad);
   return (resultsArray.getSize());
}


//==============================================================================
//==============================================================================
uint BKB::executeKBBaseQuery(const BKBBaseQuery& kbBaseQuery, BKBBaseIDArray& resultsArray, uint flags) const
{
   // Clear our results.
   if (flags & BKB::cClearExistingResults)
      resultsArray.clear();

   long numPlayers = gWorld->getNumberPlayers();
   for (long i=0; i<numPlayers; i++)
   {
      // Get our KBPlayer for this player.
      const BKBPlayer* pKBPlayer = getKBPlayer(i);
      if (!pKBPlayer)
         continue;

      if (kbBaseQuery.getFlagPlayerRelation() && kbBaseQuery.getPlayerRelation() != cRelationTypeAny)
      {
         BRelationType relationType = kbBaseQuery.getPlayerRelation();
         if (relationType == cRelationTypeSelf && (i != kbBaseQuery.getPlayerID()))
         {
            continue;
         }
         else if (relationType == cRelationTypeAlly)
         {
            const BPlayer* pRelationPlayer = gWorld->getPlayer(kbBaseQuery.getPlayerID());
            if (!pRelationPlayer->isAlly(i, kbBaseQuery.getSelfAsAlly()))
               continue;
         }
         else if (relationType == cRelationTypeEnemy)
         {
            const BPlayer* pRelationPlayer = gWorld->getPlayer(kbBaseQuery.getPlayerID());
            if (!pRelationPlayer->isEnemy(i))
               continue;
         }
         else if (relationType == cRelationTypeNeutral)
         {
            const BPlayer* pRelationPlayer = gWorld->getPlayer(kbBaseQuery.getPlayerID());
            if (!pRelationPlayer->isNeutral(i))
               continue;
         }
      }

      // We haven't excluded this player yet, so get the possible matches from the player.
      const BKBBaseIDArray& possibleMatches = pKBPlayer->getKBBaseIDs();
      uint numPossibleMatches = possibleMatches.getSize();

      // Now try to exclude the possible matches.
      for (uint i=0; i<numPossibleMatches; i++)
      {
         // Get the KBBase
         const BKBBase* pKBBase = gWorld->getKBBase(possibleMatches[i]);
         BASSERT(pKBBase);
         if (!pKBBase)
            continue;

         // Filter by requiring buildings.
         if (kbBaseQuery.getFlagRequireBuildings())
         {
            if (pKBBase->getBuildingCount() == 0)
               continue;
         }

         // Filter by min staleness.
         if (kbBaseQuery.getFlagMinStaleness())
         {
            DWORD minStaleness = kbBaseQuery.getMinStaleness();
            if (pKBBase->getStaleness() < minStaleness)
               continue;
         }

         // Filter by max staleness
         if (kbBaseQuery.getFlagMaxStaleness())
         {
            DWORD maxStaleness = kbBaseQuery.getMaxStaleness();
            if (pKBBase->getStaleness() > maxStaleness)
               continue;
         }

         // Filter by point radius.
         if (kbBaseQuery.getFlagPointRadius())
         {
            BVector pos = kbBaseQuery.getPosition();
            float rad = kbBaseQuery.getRadius();
            float dist = pKBBase->getPosition().distance(pos);
            if (dist > rad)
               continue;
         }

         // Add new KBBase criteria here...

         // THIS MATCH FITS OUR CRITERIA.  HOORAY!
         if (flags & BKB::cClearExistingResults)
            resultsArray.add(possibleMatches[i]);
         else
            resultsArray.uniqueAdd(possibleMatches[i]);
      }
   }

   // How many did we get?
   return (resultsArray.getSize());
}


//==============================================================================
//==============================================================================
uint BKB::executeKBBaseQueryClosest(const BKBBaseQuery& kbBaseQuery, BVector testLocation, BKBBaseIDArray& resultsArray) const
{
   // Clear our results.
   resultsArray.clear();
   float closestDistSqr = cMaximumFloat;
   BKBBaseID closestKBBaseID = cInvalidKBBaseID;

   BKBBaseIDArray preliminaryResults;
   uint numPreliminaryResults = executeKBBaseQuery(kbBaseQuery, preliminaryResults, BKB::cClearExistingResults);
   for (uint i=0; i<numPreliminaryResults; i++)
   {
      const BKBBase* pKBBase = gWorld->getKBBase(preliminaryResults[i]);
      if (!pKBBase)
         continue;
      float testDistSqr = pKBBase->getPosition().distanceSqr(testLocation);
      if (testDistSqr < closestDistSqr)
      {
         closestDistSqr = testDistSqr;
         closestKBBaseID = preliminaryResults[i];
      }
   }

   if (closestKBBaseID != cInvalidKBBaseID)
      resultsArray.add(closestKBBaseID);
   return (resultsArray.getSize());
}


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void BKB::debugRender() const
{
   SCOPEDSAMPLE(BKB_render);

   if (gGame.getAIDebugType() == AIDebugType::cNone)
      return;

   if (getTeamID() != gUserManager.getPrimaryUser()->getTeamID())
      return;

   uint numKBBases = mKBBaseIDs.getSize();
   for (uint i=0; i<numKBBases; i++)
   {
      const BKBBase* pKBBase = gWorld->getKBBase(mKBBaseIDs[i]);
      if (pKBBase)
         pKBBase->debugRender();
   }

   uint numKBSquads = mKBSquadIDs.getSize();
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(mKBSquadIDs[i]);
      if (pKBSquad)
         pKBSquad->debugRender();
   }   
}
#endif


//==============================================================================
//==============================================================================
void BKB::acquireVisToSquad(BSquad* pSquad)
{
   // Don't do nothing.
   if (!mFlagEnabled)
      return;
   if (mFlagPaused)
      return;
   BASSERT(pSquad);
   if (!pSquad)
      return;
   BKBSquad* pKBSquad = gWorld->getKBSquad(pSquad->getKBSquadID(mTeamID));
   if (pKBSquad)
   {
      pKBSquad->setCurrentlyVisible(true);
      BKBBase* pKBBase = gWorld->getKBBase(pKBSquad->getKBBaseID());
      if (pKBBase && pKBBase->getVisibleSquadCount() >= 0)
         pKBBase->setLastSeenTime(gWorld->getGametime());
   }
}


//==============================================================================
//==============================================================================
void BKB::loseVisToSquad(BSquad* pSquad)
{
   // Don't do nothing.
   if (!mFlagEnabled)
      return;
   if (mFlagPaused)
      return;
   BASSERT(pSquad);
   if (!pSquad)
      return;

   BKBSquad* pKBSquad = gWorld->getKBSquad(pSquad->getKBSquadID(mTeamID));
   if (pKBSquad)
   {
      DWORD gameTime = gWorld->getGametime();
      pKBSquad->setLastSeenTime(gameTime);
      pKBSquad->setCurrentlyVisible(false);
      BKBBase* pKBBase = gWorld->getKBBase(pKBSquad->getKBBaseID());
      if (pKBBase && pKBBase->getVisibleSquadCount() == 0)
         pKBBase->setLastSeenTime(gameTime);
   }
}


//==============================================================================
// The main update loop for the KB on a particular squad.
//==============================================================================
void BKB::updateSquad(BSquad* pSquad)
{
   SCOPEDSAMPLE(BKB_updateSquad);
   
   // Don't do nothing.
   if (!mFlagEnabled)
      return;
   if (mFlagPaused)
      return;

   BASSERT(pSquad);
   if (!pSquad || !pSquad->isKBAware())
      return;
   
   // If this squad does not have a BKBSquad entry in this team's KB, create one first...
   BKBSquad* pKBSquad = gWorld->getKBSquad(pSquad->getKBSquadID(mTeamID));
   if (!pKBSquad && isSquadValidForKBSquad(pSquad))
   {
      pKBSquad = gWorld->createKBSquad(mTeamID);
      if (pKBSquad)
         pSquad->setKBSquadID(pKBSquad->getID(), mTeamID);
   }

   // If we don't have a KB squad entry at this point it is because it's not a KB aware thing.  Skip it.
   if (pKBSquad)
      updateKBSquadEntry(pSquad, *pKBSquad);
}


//==============================================================================
//==============================================================================
bool BKB::isSquadValidForKBSquad(const BSquad* pSquad) const
{
   if (!pSquad)
      return (false);
   if (!pSquad->isAlive())
      return (false);
   if (pSquad->getFlagDestroy())
      return (false);
   if (!pSquad->getLeaderUnit())
      return (false);
   if (!pSquad->isKBAware())
      return (false);

   // Add type specific checks here.

   return (true);
}


//==============================================================================
//==============================================================================
void BKB::updateKBSquadEntry(BSquad *pSquad, BKBSquad& rKBSquadEntry)
{
   SCOPEDSAMPLE(BKB_updateKBSquadEntry);
   BASSERT(pSquad);  // Don't bail on invalid pSquad... Not sure but this might be ok.

   bool bValid = isSquadValidForKBSquad(pSquad);
   if (!bValid)
   {
      gWorld->deleteKBSquad(rKBSquadEntry.getID());
      return;
   }

   // Get the BEFORE data.
   BPlayerID oldPlayerID = rKBSquadEntry.getPlayerID();
   BProtoSquadID oldProtoSquadID = rKBSquadEntry.getProtoSquadID();

   // Update the squad entry.
   rKBSquadEntry.update(this, pSquad);

   // Get the AFTER data
   BPlayerID newPlayerID = rKBSquadEntry.getPlayerID();
   BProtoSquadID newProtoSquadID = rKBSquadEntry.getProtoSquadID();

   // If anything changed, refresh!
   // Note: it will be changed on the first update because it is initialized with the cInvalid values...
   if ((oldPlayerID != newPlayerID) || (oldProtoSquadID != newProtoSquadID))
   {
      // Remove from old KBPlayer.
      BKBPlayer* pOldKBPlayer = getKBPlayer(oldPlayerID);
      if (pOldKBPlayer)
         pOldKBPlayer->removeKBSquad(rKBSquadEntry.getID());

      // Add to new KBPlayer.
      BKBPlayer* pNewKBPlayer = getKBPlayer(newPlayerID);
      if (pNewKBPlayer)
         pNewKBPlayer->addKBSquad(rKBSquadEntry.getID());

      // Remove from old base.
      BKBBase* pOldBase = gWorld->getKBBase(rKBSquadEntry.getKBBaseID());
      if (pOldBase)
         pOldBase->removeKBSquad(rKBSquadEntry.getID());
   }

   // Put in correct base.
   putInCorrectBase(rKBSquadEntry);
}



//==============================================================================
//==============================================================================
const BKBPlayer* BKB::getKBPlayer(BPlayerID playerID) const
{
   if (playerID >= 0 && playerID < cMaximumSupportedPlayers)
   {
      BASSERT(gWorld->getPlayer(playerID));
      return (&mKBPlayers[playerID]);
   }
   else
   {
      return (NULL);
   }
}


//==============================================================================
//==============================================================================
BKBPlayer* BKB::getKBPlayer(BPlayerID playerID)
{
   if (playerID >= 0 && playerID < cMaximumSupportedPlayers)
   {
      BASSERT(gWorld->getPlayer(playerID));
      return (&mKBPlayers[playerID]);
   }
   else
   {
      return (NULL);
   }
}


//==============================================================================
//==============================================================================
uint BKB::getNumberBases() const
{
   return (mKBBaseIDs.getSize());
}


//==============================================================================
//==============================================================================
uint BKB::getNumberBases(BPlayerID playerID) const
{
   const BKBPlayer* pKBPlayer = getKBPlayer(playerID);
   if (pKBPlayer)
      return (pKBPlayer->getKBBaseIDs().getSize());
   else
      return (0);
}


//==============================================================================
//==============================================================================
uint BKB::getNumberKBSquads() const
{
   return (mKBSquadIDs.getSize());
}


//==============================================================================
//==============================================================================
uint BKB::getNumberKBSquads(BPlayerID playerID) const
{
   const BKBPlayer* pKBPlayer = getKBPlayer(playerID);
   if (pKBPlayer)
      return (pKBPlayer->getNumberKBSquads());
   else
      return (0);
}


//==============================================================================
// Assumes pKBSquad has already been validated higher up the callstack.
//==============================================================================
void BKB::putInCorrectBase(BKBSquad& rKBSquadEntry)
{
   SCOPEDSAMPLE(BKB_putInCorrectBase);

   // KB Squads without a valid position (either currently visible, or last known) do not belong in a KB Base
   BVector kbSquadEntryPosition;
   bool hasValidPos = rKBSquadEntry.getValidKnownPosition(kbSquadEntryPosition);
   if (!hasValidPos)
   {
      BKBBase* pCurrentBase = gWorld->getKBBase(rKBSquadEntry.getKBBaseID());
      if (pCurrentBase)
         pCurrentBase->removeKBSquad(rKBSquadEntry.getID());
      return;
   }

   // The KB Squad is in a base already.
   BKBBase* pOldBase = gWorld->getKBBase(rKBSquadEntry.getKBBaseID());
   if (pOldBase)
   {
      // If it still looks valid, return.
      if (pOldBase->containsPosition(kbSquadEntryPosition))
         return;
      
      // Otherwise remove from the old base.  We'll find a valid one, or create one.
      pOldBase->removeKBSquad(rKBSquadEntry.getID());
   }

   // Sanity check.
   BASSERTM(gWorld->getKBBase(rKBSquadEntry.getKBBaseID()) == NULL, "At this point we should not be contained within any KB Base.");
   BPlayerID kbSquadEntryPlayerID = rKBSquadEntry.getPlayerID();
   BKBPlayer* pKBPlayer = getKBPlayer(kbSquadEntryPlayerID);
   BASSERT(pKBPlayer);
   if (!pKBPlayer)
      return;

   // Go through all bases on this player and see if there is one we can be added to.
   const BKBBaseIDArray& potentialBases = pKBPlayer->getKBBaseIDs();
   uint numPotentialBases = potentialBases.getSize();
   BKBBase* pBestBase = NULL;
   for (uint i=0; i<numPotentialBases; i++)
   {
      // TO be considered the kb squad's position must be contained within the base radius.
      BKBBase* pPossibleBase = gWorld->getKBBase(potentialBases[i]);
      if (!pPossibleBase || !pPossibleBase->containsPosition(kbSquadEntryPosition))
         continue;

      // If we don't have a best base yet, first one is best.
      if (!pBestBase)
      {
         pBestBase = pPossibleBase;
         continue;
      }

      // If the possible base has more buildings than the best base, it wins.
      if (pPossibleBase->getBuildingCount() > pBestBase->getBuildingCount())
      {
         pBestBase = pPossibleBase;
         continue;
      }

      // If the possible base has more squads than the best base, it wins.
      if (pPossibleBase->getKBSquadIDs().getSize() > pBestBase->getKBSquadIDs().getSize())
      {
         pBestBase = pPossibleBase;
         continue;
      }
   }

   // If we found a best base to be put in, do that.
   if (pBestBase)
   {
      pBestBase->addKBSquad(rKBSquadEntry.getID()); 
   }
   else
   {
      BKBBase* pNewBase = gWorld->createKBBase(mTeamID, kbSquadEntryPlayerID, kbSquadEntryPosition, cDefaultBKBBaseRadius);
      if (pNewBase)
         pNewBase->addKBSquad(rKBSquadEntry.getID());
   }
}

//==============================================================================
// BKB::save
//==============================================================================
bool BKB::save(BStream* pStream, int saveType) const
{
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
      GFWRITECLASS(pStream, saveType, mKBPlayers[i]);
   GFWRITEARRAY(pStream, BKBSquadID, mKBSquadIDs, uint16, 10000);
   GFWRITEARRAY(pStream, BKBBaseID, mKBBaseIDs, uint16, 1000);
   GFWRITEVAR(pStream, BTeamID, mTeamID);
   GFWRITEVAR(pStream, DWORD, mNextUpdateTime);
   GFWRITEBITBOOL(pStream, mFlagEnabled);
   GFWRITEBITBOOL(pStream, mFlagPaused);
   return true;
}

//==============================================================================
// BKB::load
//==============================================================================
bool BKB::load(BStream* pStream, int saveType)
{  
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
      GFREADCLASS(pStream, saveType, mKBPlayers[i]);
   GFREADARRAY(pStream, BKBSquadID, mKBSquadIDs, uint16, 10000);
   GFREADARRAY(pStream, BKBBaseID, mKBBaseIDs, uint16, 1000);
   GFREADVAR(pStream, BTeamID, mTeamID);
   GFREADVAR(pStream, DWORD, mNextUpdateTime);
   GFREADBITBOOL(pStream, mFlagEnabled);
   GFREADBITBOOL(pStream, mFlagPaused);
   return true;
}

//==============================================================================
// BKBPlayer::save
//==============================================================================
bool BKBPlayer::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BKBSquadID, mKBSquadIDs, uint16, 10000);
   GFWRITEARRAY(pStream, BKBBaseID, mKBBaseIDs, uint16, 1000);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   return true;
}

//==============================================================================
// BKBPlayer::load
//==============================================================================
bool BKBPlayer::load(BStream* pStream, int saveType)
{  
   GFREADARRAY(pStream, BKBSquadID, mKBSquadIDs, uint16, 10000);
   GFREADARRAY(pStream, BKBBaseID, mKBBaseIDs, uint16, 1000);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   return true;
}
