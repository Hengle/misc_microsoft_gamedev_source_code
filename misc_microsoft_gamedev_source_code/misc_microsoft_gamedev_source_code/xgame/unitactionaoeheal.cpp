//==============================================================================
// unitactionaoeheal.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "civ.h"
#include "config.h"
#include "configsgame.h"
#include "entity.h"
#include "protoobject.h"
#include "protosquad.h"
#include "squad.h"
#include "unitactionaoeheal.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"
#include "power.h"
#include "powermanager.h"
#include "powerrepair.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionAoeHeal, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionAoeHeal::init(void)
{
   if (!BAction::init())
      return(false);

   mHealerAnimOppID = BUnitOpp::cInvalidID;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAoeHeal::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   mHealerAnimOppID = BUnitOpp::cInvalidID;
   mPowerID = cInvalidPowerID;

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAoeHeal::update(float elapsed)
{
   BASSERT(mpOwner);
   if (!mpOwner)
      return (true);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (!mpProtoAction)
      return (true);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   switch (mState)
   {
      case cStateNone:
      {
         long ppid = gDatabase.getPPIDHookRepair();

         const BPlayer *pPlayer = gWorld->getPlayer(mpOwner->getPlayerID());
         if (!pPlayer)
            return true;

         // Cast power, switch to working
         BPower* pRepair = gWorld->getPowerManager()->createNewPower(ppid);
         if (pRepair && pRepair->init(mpOwner->getPlayerID(), 0, cInvalidPowerUserID, cInvalidObjectID, mpOwner->getPosition(), true))
         {
            mPowerID = pRepair->getID();
            setState(cStateWait);
         }
         else
         {
            pRepair->shutdown();
         }
      }
      break;

      case cStateWait:
      {
         BASSERT(mPowerID != cInvalidPowerID);
         BPower* pPower = gWorld->getPowerManager()->getPowerByID(mPowerID);
         BASSERT(pPower);
         BASSERT(pPower->getType() == PowerType::cRepair);
         BPowerRepair* pRepair = static_cast<BPowerRepair*>(pPower);
         BASSERT(pRepair);
         BUnit *pUnit = mpOwner->getUnit();
         if (!pUnit)
            return true;

         // Start anim
         if (pRepair->getCurrentRepairCount() > 0 && mHealerAnimOppID == BUnitOpp::cInvalidID)
         {
            BUnitOpp* pNewOpp = BUnitOpp::getInstance();
            pNewOpp->init();
            pNewOpp->setType(BUnitOpp::cTypeHeal);
            pNewOpp->setPriority(BUnitOpp::cPriorityHigh);
            pNewOpp->setSource(pUnit->getID());
            pNewOpp->setUserData(cAnimTypeHeal);
            pNewOpp->generateID();
            if (pUnit->addOpp(pNewOpp))
            {
               mHealerAnimOppID = pNewOpp->getID();
            }
            else
            {
               BUnitOpp::releaseInstance(pNewOpp);
               mHealerAnimOppID = BUnitOpp::cInvalidID;
            }
         }
         // Stop anim
         else if (pRepair->getCurrentRepairCount() == 0 && mHealerAnimOppID != BUnitOpp::cInvalidID)
         {
            pUnit->removeOpp(mHealerAnimOppID);
            mHealerAnimOppID = BUnitOpp::cInvalidID;
         }
      }
      break;
   }

   if(!BAction::update(elapsed))
      return (false);
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAoeHeal::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, BUnitOppID, mHealerAnimOppID);
   GFWRITEVAR(pStream, BPowerID, mPowerID);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionAoeHeal::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, BUnitOppID, mHealerAnimOppID);
   GFREADVAR(pStream, BPowerID, mPowerID);

   return true;
}
