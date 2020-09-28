//==============================================================================
// squadactiondetect.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactioncloakdetect.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "SimOrderManager.h"
#include "unitquery.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionCloakDetect, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionCloakDetect::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   return BAction::connect(pOwner, pOrder);
}
//==============================================================================
//==============================================================================
void BSquadActionCloakDetect::disconnect()
{
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionCloakDetect::init()
{
   if (!BAction::init())
      return(false);

   mDetectionTimer = gDatabase.getCloakDetectFrequency();
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionCloakDetect::setState(BActionState state)
{

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionCloakDetect::update(float elapsed)
{
   BASSERT(mpOwner);

   switch (mState)
   {
   case cStateNone:
      setState(cStateWorking);
      break;

   case cStateWorking:
      {
         DWORD lastUpdateLength = gWorld->getLastUpdateLength();


         if( mDetectionTimer <= lastUpdateLength )
         {
            mDetectionTimer = gDatabase.getCloakDetectFrequency();
            detectionSearch();
         }
         else
         {
            mDetectionTimer -= lastUpdateLength;
         }
         break;
      }

   case cStateDone:

      break;
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionCloakDetect::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{

}

//==============================================================================
//==============================================================================
void BSquadActionCloakDetect::detectionSearch()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BEntityIDArray results(0, 100);
   BUnitQuery query(pSquad->getPosition(), pSquad->getLOS(), true);
   query.setFlagIgnoreDead(true);
   query.setRelation(pSquad->getPlayerID(), cRelationTypeEnemy);
   gWorld->getSquadsInArea(&query, &results);
   if (results.getNumber() > 0)
   {
      for (uint i = 0; i < results.getSize(); i++)
      {
         BSquad* pTarget = gWorld->getSquad(results[i]);
         if (!pTarget)
            continue;
         pTarget->sendEvent(pTarget->getID(), pTarget->getID(), BEntity::cEventDetected, mpOwner->getID().asLong());
      }
   }  
}

//==============================================================================
//==============================================================================
bool BSquadActionCloakDetect::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, DWORD, mDetectionTimer);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionCloakDetect::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, DWORD, mDetectionTimer);
   return true;
}
