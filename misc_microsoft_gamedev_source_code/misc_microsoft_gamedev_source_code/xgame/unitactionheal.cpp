//==============================================================================
// unitactionheal.cpp
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
#include "unitactionheal.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionHeal, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionHeal::init(void)
{
   if (!BAction::init())
      return(false);

   mHealerAnimOppID = BUnitOpp::cInvalidID;
   mTarget.reset();

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionHeal::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   mHealerAnimOppID = BUnitOpp::cInvalidID;
     

   setState(cStateWait);
   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionHeal::disconnect()
{
   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionHeal::setState(BActionState state)
{
   return BAction::setState(state);
}


//==============================================================================
//==============================================================================
bool BUnitActionHeal::needsToHeal() const
{
   BASSERT(mpOwner);
   if (!mpOwner)
      return (false);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (!mpProtoAction)
      return (false);
   if (mpProtoAction->getFlagDisabled())
      return (false);

   const BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   if (!pUnit)
      return (false);

//-- FIXING PREFIX BUG ID 1210
   const BSquad* pSquad = NULL;
//--
   if (mpProtoAction->getHealTarget())
      pSquad = gWorld->getSquad(mTarget.getID());
   
   if (!pSquad)
      pSquad = pUnit->getParentSquad();

   BASSERT(pSquad);
   if (!pSquad)
      return (false);

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   BASSERT(pProtoSquad);
   if (!pProtoSquad)
      return (false);

   const BEntityIDArray& childUnits = pSquad->getChildList();
   uint numChildren = childUnits.getSize();
   for (uint i=0; i<numChildren; i++)
   {
      const BUnit* pChild = gWorld->getUnit(childUnits[i]);
      if (!pChild)
         continue;
      if (pChild->getHitpoints() < pChild->getProtoObject()->getHitpoints())
         return (true);
   }

   if (mpProtoAction->getAllowReinforce())
   {
      uint maxNumChildren = 0;
      for (long i=0; i<pProtoSquad->getNumberUnitNodes(); i++)
         maxNumChildren += pProtoSquad->getUnitNode(i).mUnitCount;

      if (pSquad->getNumberChildren() < maxNumChildren)
         return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
bool BUnitActionHeal::timeToHeal(const BSquad* pSquad) const
{
   const BSquad* pTargetSquad = pSquad;

   if (mpProtoAction->getHealTarget())
      pTargetSquad = gWorld->getSquad(mTarget.getID());

   if(!pTargetSquad || !mpProtoAction)
      return(false);

   DWORD minIdleDuration = mpProtoAction->getMinIdleDuration();
   DWORD idleDuration = pTargetSquad->getIdleDuration();
   DWORD lastdamagedDuration = gWorld->getGametime() - pTargetSquad->getLastDamagedTime();
   DWORD lastAttacedDuration = gWorld->getGametime() - pTargetSquad->getLastAttackedTime();
   
   // [8/27/2008 JRuediger] Must be idle for minIdleDuration before its time to heal again.
   if(idleDuration < minIdleDuration || lastdamagedDuration < minIdleDuration || lastAttacedDuration < minIdleDuration)
   {
      return(false);
   }

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionHeal::update(float elapsed)
{
   BASSERT(mpOwner);
   if (!mpOwner)
      return (true);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (!mpProtoAction)
      return (true);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   if (!pUnit)
      return (true);
   BSquad* pSquad = NULL;

   if (mpProtoAction->getHealTarget())
      pSquad = gWorld->getSquad(mTarget.getID());
   
   if (!pSquad)
      pSquad = pUnit->getParentSquad();

   BASSERT(pSquad);
   if (!pSquad)
      return (true);

   // Do the healing.
   switch (mState)
   {
      case cStateWait:
      {
         if (timeToHeal(pSquad) && needsToHeal())
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
            setState(cStateWorking);
         }
         break;
      }
      case cStateWorking:
      {
        
         if (!timeToHeal(pSquad) || !needsToHeal())
         {
            if (mHealerAnimOppID != BUnitOpp::cInvalidID)
            {
               pUnit->removeOpp(mHealerAnimOppID);
               mHealerAnimOppID = BUnitOpp::cInvalidID;
            }
            setState(cStateWait);
         }
         else
         {
            float hpPerSec = mpProtoAction->getWorkRate();
            float repairHP = hpPerSec * elapsed;
            float excessHP = 0.0f;
            bool bAllowReinforce = mpProtoAction->getAllowReinforce();
               pSquad->repairHitpoints(repairHP, bAllowReinforce, excessHP);
         }
         break;
      }
   }

   if(!BAction::update(elapsed))
      return (false);
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionHeal::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeSquad));
   mTarget = target;
}

//==============================================================================
//==============================================================================
bool BUnitActionHeal::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mHealerAnimOppID);
   GFWRITECLASS(pStream, saveType, mTarget);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionHeal::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mHealerAnimOppID);
   GFREADCLASS(pStream, saveType, mTarget);
   return true;
}
