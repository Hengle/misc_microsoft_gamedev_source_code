//==============================================================================
// unitactionambientlifespawner.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionambientlifespawner.h"
#include "squad.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "simhelper.h"
#include "unitquery.h"
#include "math.h"
#include "tactic.h"
#include "protosquad.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionAmbientLifeSpawner, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionAmbientLifeSpawner::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(mpProtoAction);
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   mSquadType = mpProtoAction->getSquadType();
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionAmbientLifeSpawner::disconnect()
{
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionAmbientLifeSpawner::init()
{
   if (!BAction::init())
      return(false);

   mOppTimer = (DWORD)gDatabase.getALSpawnerCheckFrequency();

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAmbientLifeSpawner::update(float elapsed)
{
   BASSERT(mpProtoAction);

   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 4916
   const BUnit* pUnit = static_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   DWORD lastUpdateLength = gWorld->getLastUpdateLength();

   switch (mState)
   {
   case cStateNone:
      {
         setState(cStateWorking);
         break;
      }

   case cStateWorking:
      {
         if ( mOppTimer <= lastUpdateLength)
         {
            mOppTimer = (DWORD)gDatabase.getALSpawnerCheckFrequency();
            updateOpps();
         }
         else
         {
            mOppTimer -= lastUpdateLength;
         }
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionAmbientLifeSpawner::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   BASSERT(mpOwner);

   BAction::notify(eventType, senderID, data, data2);
}

//==============================================================================
//==============================================================================
void BUnitActionAmbientLifeSpawner::updateOpps()
{
   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 4917
   const BUnit* pUnit = static_cast<BUnit*>(mpOwner);
//--

   // Need a new target...
   BUnitQuery query(pUnit->getPosition(), gDatabase.getALOppCheckRadius(), false);            
   query.setFlagIgnoreDead(true);

   BEntityIDArray results(0, 100);
   long numResults = gWorld->getSquadsInArea(&query, &results);

   if (numResults > 0)
   {
      BSquad* pEnemySquad = gWorld->getSquad(results[0]);

      // Do Some spawnin....
      BPlayer*  pPlayer = mpOwner->getPlayer();
      BProtoSquad*  pTrainSquadProto = pPlayer->getProtoSquad(mSquadType);
      BProtoObject*  pTrainProto = pPlayer->getProtoObject(pTrainSquadProto->getUnitNode(0).mUnitType);

      BVector pos, forward, right;

      {
         //FIXME AJL 5/9/06 - Temp code to calculate random location for placing new unit
         float obstructionRadius = pTrainProto->getObstructionRadius();
         BVector dir = cZAxisVector;
         dir.rotateXZ(getRandRangeFloat(cSimRand, 0.0f, cTwoPi));
         dir.normalize();

         pos = mpOwner->getPosition() + (dir * (mpOwner->getObstructionRadius() + obstructionRadius * getRandRangeFloat(cSimRand, 2.0f, 4.0f)));
         forward = dir;
         right.assignCrossProduct(cYAxisVector, forward);
         right.normalize();
      }

      // Create the new squad
      BEntityID   spawnedSquadID = gWorld->createEntity(mSquadType, true, mpOwner->getPlayerID(), pos, forward, right, true);
      BSquad      *pSpawnedSquad = gWorld->getSquad(spawnedSquadID);
      BDEBUG_ASSERT(pSpawnedSquad);

      pSpawnedSquad->fleeMap(pEnemySquad);

      // Play create sound
      //if (pPlayer->getID() == gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() || (gGame.isSplitScreen() && pPlayer->getID() == gUserManager.getUser(BUserManager::cSecondaryUser)->getPlayerID()))
      //   gSoundManager.playCue(pTrainProto->getSound(cObjectSoundCreate));

      setState(cStateDone);
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionAmbientLifeSpawner::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, long, mSquadType);
   GFWRITEVAR(pStream, BEntityID, mSpawnedUnit);
   GFWRITEVAR(pStream, DWORD, mOppTimer);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionAmbientLifeSpawner::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, long, mSquadType);
   GFREADVAR(pStream, BEntityID, mSpawnedUnit);
   GFREADVAR(pStream, DWORD, mOppTimer);
   return true;
}
