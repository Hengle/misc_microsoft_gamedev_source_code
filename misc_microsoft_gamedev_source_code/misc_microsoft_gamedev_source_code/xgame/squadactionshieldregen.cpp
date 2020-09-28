//==============================================================================
// squadactionshieldregen.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "ActionManager.h"
#include "squadactionshieldregen.h"
#include "squad.h"
#include "unit.h"
#include "world.h"



//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionShieldRegen, 5, &gSimHeap);


//==============================================================================
//==============================================================================
bool BSquadActionShieldRegen::init()
{
   if (!BAction::init())
      return(false);
      
   setFlagPersistent(true);
   mFlagConflictsWithIdle=false;
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionShieldRegen::update(float elapsed)
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (pSquad->getFlagShieldDamaged() && !pSquad->getFlagStopShieldRegen())
   {
      const DWORD lastDamagedTime = pSquad->getLastDamagedTime();
      const DWORD gameTime = gWorld->getGametime();
      const DWORD duration = gameTime - lastDamagedTime;

      DWORD x = pSquad->getPlayer()->getShieldRegenDelay();
      float y = 1.0f;
      BUnit *pLeaderUnit = pSquad->getLeaderUnit();
      if (pLeaderUnit)
         y = pLeaderUnit->getShieldRegenDelay();

      if (lastDamagedTime == 0 || duration > x * y)
      {
         bool repairing = false;

         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            BUnit *pChild = gWorld->getUnit(pSquad->getChild(i));
            if (pChild && pChild->getFlagHasShield() && pChild->isAlive() && !pChild->getFlagDown())
            {
               BAction* pAction = gActionManager.createAction(BAction::cActionTypeUnitShieldRegen);
               pChild->addAction(pAction);
               repairing = true;
            }
         }

         pSquad->setFlagShieldDamaged(!repairing);
      }
   }

   return (true);
}

