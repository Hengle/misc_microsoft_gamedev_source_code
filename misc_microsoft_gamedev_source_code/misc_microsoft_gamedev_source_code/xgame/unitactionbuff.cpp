//==============================================================================
// unitactionbuff.cpp
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
#include "unitactionbuff.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionBuff, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionBuff::init(void)
{
   if (!BAction::init())
      return(false);

   mHealerAnimOppID = BUnitOpp::cInvalidID;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionBuff::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   mHealerAnimOppID = BUnitOpp::cInvalidID;

   setState(cStateWait);
   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionBuff::disconnect()
{
   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionBuff::setState(BActionState state)
{
   return BAction::setState(state);
}


//==============================================================================
//==============================================================================
bool BUnitActionBuff::needsToHeal() const
{
   BASSERT(mpOwner);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (mpProtoAction->getFlagDisabled())
      return (false);

   const BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   const BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return false;
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   BASSERT(pProtoSquad);

   const BEntityIDArray& childUnits = pSquad->getChildList();
   uint numChildren = childUnits.getSize();
   for (uint i=0; i<numChildren; i++)
   {
      const BUnit* pChild = gWorld->getUnit(childUnits[i]);
      if (pChild && pChild->getHitpoints() < pChild->getProtoObject()->getHitpoints())
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
bool BUnitActionBuff::update(float elapsed)
{
   BASSERT(mpOwner);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   // Do the healing.
   switch (mState)
   {
      case cStateWait:
      {
         DWORD minIdleDuration = mpProtoAction->getMinIdleDuration();
         DWORD idleDuration = pSquad->getIdleDuration();
         if (idleDuration >= minIdleDuration && needsToHeal())
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
         DWORD minIdleDuration = mpProtoAction->getMinIdleDuration();
         DWORD idleDuration = pSquad->getIdleDuration();
         if (idleDuration < minIdleDuration || !needsToHeal())
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
bool BUnitActionBuff::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mHealerAnimOppID);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBuff::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mHealerAnimOppID);
   return true;
}
