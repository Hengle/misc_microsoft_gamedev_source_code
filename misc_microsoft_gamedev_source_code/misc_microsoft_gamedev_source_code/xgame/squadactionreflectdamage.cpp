//==============================================================================
// squadactionreflectdamage.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionReflectDamage.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "SimOrderManager.h"
#include "ability.h"
#include "visual.h"
#include "grannyinstance.h"
#include "usermanager.h"
#include "user.h"
#include "worldsoundmanager.h"
#include "tactic.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionReflectDamage, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionReflectDamage::connect(BEntity* pOwner, BSimOrder* pOrder)
{  
   if (!BAction::connect(pOwner, pOrder))
      return false;

   return true;
}
//==============================================================================
//==============================================================================
void BSquadActionReflectDamage::disconnect()
{
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionReflectDamage::init()
{
   if (!BAction::init())
      return(false);

   return(true);
}


//==============================================================================
//==============================================================================
void BSquadActionReflectDamage::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BAction::notify(eventType, senderID, data1, data2);

   BASSERT(mpProtoAction);
   if (mpProtoAction->getFlagDisabled())
      return;

//-- FIXING PREFIX BUG ID 1717
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   switch (eventType)
   {
      case BEntity::cEventDamaged:
         {
//-- FIXING PREFIX BUG ID 1716
            const BUnit* pAttacker = gWorld->getUnit(data1);
//--

            if  (pAttacker)
            {
               float dmg = data2 * mpProtoAction->getWorkRate();
               BDamageHelper::doDamageWithWeaponType(pSquad->getPlayerID(), pSquad->getTeamID(), pAttacker->getID(), NULL, dmg, -1, false, cInvalidVector, 1.0f, cInvalidVector, cInvalidObjectID);
            }
         }
         break;
   }  
}
