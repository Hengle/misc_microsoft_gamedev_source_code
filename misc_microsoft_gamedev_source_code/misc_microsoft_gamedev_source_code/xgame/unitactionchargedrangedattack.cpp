//==============================================================================
// unitactionchargedrangedattack.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "tactic.h"
#include "unitactionchargedrangedattack.h"
#include "entity.h"
#include "unit.h"
#include "visual.h"
#include "world.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionChargedRangedAttack, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::init()
{
   if (!BAction::init())
      return false;

   mCharge = 0.0f;
   mChargedEffect = cInvalidObjectID;
   mFlagReset = false;

   return true;
}

//==============================================================================
//==============================================================================
float BUnitActionChargedRangedAttack::getDamageCharge() const
{
   return mpProtoAction->getDamageCharge();
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::isCharged() const
{
   return (mCharge >= getDamageCharge() && getDamageCharge() >= 0.0f);
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::update(float elapsed)
{
   if (!BAction::update(elapsed))
      return false;

   // Update charge timer
   if (mpProtoAction && !mpProtoAction->getFlagDisabled())
   {
      mCharge += elapsed;
      if (mCharge > (getDamageCharge() + cFloatCompareEpsilon))
         mCharge = getDamageCharge() + cFloatCompareEpsilon;
   }

   if (isCharged() && mChargedEffect == cInvalidObjectID)
   {
      BUnit* pUnit = mpOwner->getUnit();
      if (pUnit && pUnit->getVisual() && mpProtoAction && mpProtoAction->getProtoObject() != cInvalidObjectID)
      {
         long boneHandle = pUnit->getVisual()->getBoneHandle(mpProtoAction->getBoneName());
         mChargedEffect = pUnit->addAttachment(mpProtoAction->getProtoObject(), boneHandle);
      }
   }
   else if (!isCharged() && mChargedEffect != cInvalidObjectID)
   {
      BObject* pObject = gWorld->getObject(mChargedEffect);
      if (pObject)
      {
         pObject->kill(false);
         mChargedEffect = cInvalidObjectID;
      }
   }

   if (mFlagReset)
   {
      mCharge = 0.0f;
      mFlagReset = false;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mCharge);
   GFWRITEBITBOOL(pStream, mFlagReset);
   GFWRITEVAR(pStream, BEntityID, mChargedEffect);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mCharge);
   GFREADBITBOOL(pStream, mFlagReset);

   if (BAction::mGameFileVersion >= 22)
      GFREADVAR(pStream, BEntityID, mChargedEffect);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::isValidPullTarget(const BObject* pObject, const BProtoAction* pProtoAction) const
{
   if (!pObject)
      return false;

   const BProtoObject* pProto = pObject->getProtoObject();
   if (!pProto)
      return false;

   if (!pProto->isType(gDatabase.getOTIDGroundVehicle()) &&
       !pProto->isType(gDatabase.getOTIDInfantry()) &&
       !pProto->isType(gDatabase.getOTIDFlood()))
      return false;
   if (pObject->getFlagNonMobile())
      return false;

   if (pProtoAction && pProtoAction->getInvalidTargets().find(pObject->getProtoID()) >= 0)
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::isValidPullTarget(const BUnit* pUnit, const BProtoAction* pProtoAction) const
{
   if (!pUnit)
      return false;

   const BProtoObject* pProto = pUnit->getProtoObject();
   if (!pProto)
      return false;

   if (!pProto->isType(gDatabase.getOTIDGroundVehicle()) &&
       !pProto->isType(gDatabase.getOTIDInfantry()) &&
       !pProto->isType(gDatabase.getOTIDFlood()))
      return false;
   if (pUnit->getFlagNonMobile())
      return false;

   if (pProtoAction && pProtoAction->getInvalidTargets().find(pUnit->getProtoID()) >= 0)
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::isValidPullTarget(const BSquad* pSquad, const BProtoAction* pProtoAction) const
{
   if (!pSquad)
      return false;

   BUnit* pUnit = pSquad->getLeaderUnit();

   return isValidPullTarget(pUnit, pProtoAction);
}

//==============================================================================
//==============================================================================
bool BUnitActionChargedRangedAttack::canPull(const BProtoAction* pProtoAction, const BUnit* pTarget) const
{
   bool pull = true;

   BASSERT(mpOwner);
   if (!mpOwner)
      return false;

   const BUnit* pUnit = mpOwner->getUnit();

   BASSERT(pUnit);
   if (!pUnit)
      return false;

   if (!isCharged())
      return false;

   if (pProtoAction)
   {
      if (pProtoAction->getInvalidTargets().size() > 0)
      {
         pull = isValidPullTarget(pTarget, pProtoAction);
      }
   }

   // Only pull if target is outside of normal attack range
   if (pull)
   {
      float dist = pUnit->getPosition().xzDistance(pTarget->getPosition()) - (pUnit->getObstructionRadius() + pTarget->getObstructionRadius());
      float normalRange = pProtoAction->getMaxRange(pTarget, true, false);
      if (dist <= normalRange * 2.5f)
         pull = false;
   }

   return pull;
}