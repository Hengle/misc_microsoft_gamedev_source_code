//==============================================================================
// unitactiondeflect.cpp
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactiondeflect.h"
#include "unitactiondodge.h"
#include "unit.h"
#include "config.h"
#include "configsgame.h"
#include "world.h"
#include "tactic.h"
#include "visual.h"
#include "grannyinstance.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionDeflect, 5, &gSimHeap);

//==============================================================================
//==============================================================================
BUnitActionDeflect::BUnitActionDeflect()
{
}

//==============================================================================
//==============================================================================
bool BUnitActionDeflect::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return false;

   if (getProtoAction() && getProtoAction()->getProtoObject() != cInvalidObjectID)
   {
      if (getProtoAction()->getFlagDisabled() == false)
         raiseShield();
      else
         mDeflecting = false;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDeflect::init()
{
   if (!BAction::init())
      return(false);

   mDeflectAnimOppID=BUnitOpp::cInvalidID;
   mDeflecteeID=cInvalidObjectID;
   mDeflectCooldownDoneTime=0;
   mDamageDone = 0.0f;
   mDeflecting = true;
   mLastDamageTime = 0;

   return(true);
}


//==============================================================================
//==============================================================================
bool BUnitActionDeflect::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (getProtoAction() && getProtoAction()->getFlagDisabled())
      return true;

   // If we don't have a proto object, no need to run shield logic
   if (getProtoAction() && getProtoAction()->getProtoObject() == cInvalidObjectID)
      return true;

   const DWORD lastDamagedTime = max(mLastDamageTime, pUnit->getParentSquad()->getLastDamagedTime());
   const DWORD gameTime = gWorld->getGametime();
   const DWORD duration = gameTime - lastDamagedTime;

   if (!mDeflecting && (lastDamagedTime == 0 || duration > pUnit->getPlayer()->getShieldRegenDelay() * pUnit->getShieldRegenDelay()))
   {
      mDeflecting = true;
      raiseShield();
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDeflect::canDeflect() const
{
   if (getProtoAction() && getProtoAction()->getFlagDisabled())
      return (false);

   if (!mDeflecting)
      return false;

   // Already deflecting
   if (mDeflecteeID != cInvalidObjectID && mpProtoAction && !mpProtoAction->getFlagMultiDeflect())
      return false;

   // Cooling down from last deflect
   if (gWorld->getGametime() < mDeflectCooldownDoneTime)
      return false;

   if (getProtoAction() && getProtoAction()->getFlagWaitForDodgeCooldown())
   {
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);

      if (pUnit)
      {
         BUnitActionDodge* pAction = reinterpret_cast<BUnitActionDodge*>(pUnit->getActionByType(BAction::cActionTypeUnitDodge));
         if (pAction && pAction->getProtoAction())
         {
            if (gWorld->getGametime() < pAction->getDodgeCooldownDoneTime())
               return false;
         }
      }

   }

   if (getProtoAction() && getProtoAction()->getSquadMode() != -1)
   {
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);

//-- FIXING PREFIX BUG ID 4330
      const BSquad* pSquad = pUnit->getParentSquad();
//--
      BASSERT(pSquad);

      if (pSquad->getSquadMode() != getProtoAction()->getSquadMode())
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDeflect::tryDeflect(BEntityID deflecteeID, BVector trajectory, float damage)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BProtoAction* pPA = const_cast<BProtoAction*>(getProtoAction());
   BASSERT(pPA);
   if (!pPA)
      return false;

   // Check to see if can deflect
   if (!canDeflect())
      return false;

   // Check angle between deflectee trajectory and forward for
   // calculating chance to deflect
   trajectory.y = 0.0f;
   trajectory.normalize();
   BVector fwd = pUnit->getForward();
   fwd.y = 0.0f;
   fwd.normalize();
   float angleBetween = fwd.angleBetweenVector(-trajectory);
   if (angleBetween >= pPA->getDeflectMaxAngle())
      return false;
   float chance = Math::Lerp(pPA->getDeflectChanceMax(), pPA->getDeflectChanceMin(), (angleBetween / pPA->getDeflectMaxAngle()));

   // Check chance to deflect
   float rand = getRandRangeFloat(cSimRand, 0.0f, 1.0f);
   if (rand > chance)
      return false;

   static BSmallDynamicSimArray<uint> validAnims;
   validAnims.clear();

   // Deflect
   setDeflecteeID(deflecteeID);
   mDeflectCooldownDoneTime = gWorld->getGametime() + getProtoAction()->getDeflectCooldown();

   mDamageDone += damage;

   if (mDamageDone > mpProtoAction->getDeflectMaxDmg() && mpProtoAction->getDeflectMaxDmg() > 0.0f)
   {
      lowerShield();

      return false;
   }

   if (pUnit->hasAnimation(cAnimTypeBlock))
      validAnims.add(cAnimTypeBlock);

   // Setup opp to play block anim
   if (validAnims.getNumber() > 0)
   {
      BUnitOpp* pNewOpp=BUnitOpp::getInstance();
      pNewOpp->init();
      //pNewOpp->setTarget(BSimTarget());
      pNewOpp->setSource(pUnit->getID());
      pNewOpp->setNotifySource(true);
      uint8 animIndex = static_cast<uint8>(getRand(cSimRand)%validAnims.getSize());
      pNewOpp->setUserData(static_cast<uint16>(validAnims[animIndex]));
      pNewOpp->setType(BUnitOpp::cTypeEvade);
      pNewOpp->setPreserveDPS(true);
      pNewOpp->setPriority(BUnitOpp::cPriorityCritical);
      pNewOpp->generateID();
      if (!pUnit->addOpp(pNewOpp))
         BUnitOpp::releaseInstance(pNewOpp);

      setDeflectAnimOppID(pNewOpp->getID());
   }

   return true;
}


//==============================================================================
//==============================================================================
void BUnitActionDeflect::raiseShield()
{
   if (getProtoAction() && getProtoAction()->getFlagDisabled())
      return;

   BASSERT(mpProtoAction);
   if (!mpProtoAction)
      return;

   BASSERT(mpOwner);
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   if (pUnit)
   {
      BVisual* pVisual = pUnit->getVisual();
      if (pVisual && pVisual->getProtoVisual())
      {
         BProtoObject* pProto = gDatabase.getGenericProtoObject(mpProtoAction->getProtoObject());
         BASSERT(pProto); // This function should never be called without there being a valid protoobject
         if (pProto)
         {
            pUnit->setVisual(pProto->getProtoVisualIndex(), pProto->getVisualDisplayPriority());
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, mpProtoAction->getStartAnimType());
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionDeflect::lowerShield()
{
   BASSERT(mpProtoAction);
   if (!mpProtoAction)
      return;

   BASSERT(mpOwner);
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   mDeflecting = false;

   if (pUnit && pUnit->getProtoObject())
   {
	   BVisual* pVisual = pUnit->getVisual();
	   if (pVisual && pVisual->getProtoVisual())
	   {
	      pUnit->setVisual(pUnit->getProtoObject()->getProtoVisualIndex(), pUnit->getProtoObject()->getVisualDisplayPriority());
	      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, mpProtoAction->getEndAnimType());
	   }
   }
}


//==============================================================================
//==============================================================================
void BUnitActionDeflect::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if (!validateTag(eventType, senderID, data1, data2))
      return;

   if (getProtoAction() && getProtoAction()->getFlagDisabled())
      return;

   BASSERT(mpOwner);

   if (eventType == BEntity::cEventDamaged)
   {
      mLastDamageTime = gWorld->getGametime();
   }
}


//==============================================================================
//==============================================================================
BEntityID BUnitActionDeflect::getDeflecteeID() const
{
   if (!mpProtoAction || !mpProtoAction->getFlagMultiDeflect())
      return mDeflecteeID;

   return cInvalidObjectID;
}


//==============================================================================
//==============================================================================
bool BUnitActionDeflect::deflectSmallArms() const
{
   BASSERT(mpProtoAction);
   if (!mpProtoAction)
      return false;
   return mpProtoAction->getFlagSmallArms();
}


//==============================================================================
//==============================================================================
bool BUnitActionDeflect::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BUnitOppID, mDeflectAnimOppID);
   GFWRITEVAR(pStream, BEntityID, mDeflecteeID);
   GFWRITEVAR(pStream, DWORD, mDeflectCooldownDoneTime);
   GFWRITEVAR(pStream, float, mDamageDone);
   GFWRITEVAR(pStream, DWORD, mLastDamageTime);
   GFWRITEBITBOOL(pStream, mDeflecting);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDeflect::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BUnitOppID, mDeflectAnimOppID);
   GFREADVAR(pStream, BEntityID, mDeflecteeID);
   GFREADVAR(pStream, DWORD, mDeflectCooldownDoneTime);
   GFREADVAR(pStream, float, mDamageDone);
   GFREADVAR(pStream, DWORD, mLastDamageTime);
   GFREADBITBOOL(pStream, mDeflecting);

   if (BAction::mGameFileVersion < 48)
   {
      bool mShieldStateChanging;
      GFREADBITBOOL(pStream, mShieldStateChanging);
   }
   return true;
}
