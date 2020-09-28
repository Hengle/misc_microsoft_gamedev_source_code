//==============================================================================
// unitactionenergyshield.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unit.h"
#include "unitactionEnergyShield.h"
#include "squad.h"
#include "objectmanager.h"
#include "world.h"
#include "commands.h"
#include "squadplotter.h"
#include "unitquery.h"
#include "SimOrderManager.h"
#include "tactic.h"
#include "physics.h"
#include "physicsobject.h"
#include "simhelper.h"
#include "visual.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionEnergyShield, 4, &gSimHeap);


//==============================================================================
//==============================================================================
bool BUnitActionEnergyShield::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionEnergyShield::disconnect()
{
   if (mpOwner)
   {
      BUnit* pUnit = mpOwner->getUnit();
      BASSERT(pUnit);

      if (mAttachedShieldID != cInvalidObjectID)
         pUnit->removeAttachment(mAttachedShieldID);
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionEnergyShield::init()
{
   if (!BAction::init())
      return false;

   mShieldStatus = cShieldDown;
   mAttachedShieldID = cInvalidObjectID;
   mHit = false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionEnergyShield::setState(BActionState state)
{
   return BAction::setState(state);
}

//==============================================================================
//==============================================================================
bool BUnitActionEnergyShield::update(float elapsed)
{
   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 4902
   const BUnit* pUnit = mpOwner->getUnit();
//--
   BASSERT(pUnit);

   if (mShieldStatus == cShieldDown && pUnit->getShieldpoints() > 0.0f)
   {
      raiseShields(); // RED ALERT! :)
   }
   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionEnergyShield::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BASSERT(mpOwner);

   if (eventType == BEntity::cEventDamaged || eventType == BEntity::cEventKilled)
   {
//-- FIXING PREFIX BUG ID 4903
      const BUnit* pUnit = mpOwner->getUnit();
//--
      BASSERT(pUnit);

      if (mShieldStatus == cShieldUp && (pUnit->getShieldpoints() <= 0.0f || !pUnit->isAlive()))
      {
         lowerShields();
      }
      else if (mShieldStatus == cShieldUp && pUnit->getShieldpoints() > 0.0f)
      {
         shieldsHit();
      }
   }
   else if (eventType == BEntity::cEventAnimEnd || eventType == BEntity::cEventAnimLoop || eventType == BEntity::cEventAnimChain)
   {
      BObject* pShield = gWorld->getObject(mAttachedShieldID);

      if (!pShield)
         return;

      // Shield down anim finished, remove shield object
      if (mShieldStatus == cShieldDown)
      {
         BASSERT(mpOwner);
         BUnit* pUnit = mpOwner->getUnit();
         BASSERT(pUnit);

         pShield->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeIdle);

         pUnit->removeAttachment(mAttachedShieldID);

         mAttachedShieldID = cInvalidObjectID;
      }
      else if (mHit)
      {
         mHit = false;
         
         pShield->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeIdle);
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionEnergyShield::raiseShields()
{
   BASSERT(mpOwner);
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   BASSERT(mpProtoAction);

   mShieldStatus = cShieldUp;

   long boneHandle = pUnit->getVisual()->getBoneHandle(mpProtoAction->getBoneName());

   if (mAttachedShieldID == cInvalidObjectID)
      mAttachedShieldID = pUnit->addAttachment(mpProtoAction->getProtoObject(), boneHandle);

   BObject* pShield = gWorld->getObject(mAttachedShieldID);

   if (pShield)
      pShield->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeIdle);
}

//==============================================================================
//==============================================================================
void BUnitActionEnergyShield::lowerShields()
{
   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 4904
   const BUnit* pUnit = mpOwner->getUnit();
//--
   BASSERT(pUnit);

   mShieldStatus = cShieldDown;

   if (mAttachedShieldID != cInvalidObjectID)
   {
      BObject* pShield = gWorld->getObject(mAttachedShieldID);

      if (pShield)
         pShield->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeDeath);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionEnergyShield::shieldsHit()
{
   mHit = true;

   BObject* pShield = gWorld->getObject(mAttachedShieldID);

   if (pShield)
      pShield->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeIncoming);
}

//==============================================================================
//==============================================================================
bool BUnitActionEnergyShield::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, long, mShieldStatus);
   GFWRITEVAR(pStream, BEntityID, mAttachedShieldID);
   GFWRITEBITBOOL(pStream, mHit);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionEnergyShield::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, long, mShieldStatus);
   GFREADVAR(pStream, BEntityID, mAttachedShieldID);
   GFREADBITBOOL(pStream, mHit);
   return true;
}
