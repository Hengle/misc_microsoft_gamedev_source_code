//==============================================================================
// squadactiondaze.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactiondaze.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "SimOrderManager.h"
#include "ability.h"
#include "tactic.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionDaze, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionDaze::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   return BAction::connect(pOwner, pOrder);
}
//==============================================================================
//==============================================================================
void BSquadActionDaze::disconnect()
{
//-- FIXING PREFIX BUG ID 1683
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   for (uint i = 0; i < mFXAttachment.size(); ++i)
   {
      if (mFXAttachment[i].isValid())
      {
         BEntity *pAttachEntity = gWorld->getEntity(mFXAttachment[i]);
         if ( pAttachEntity)
            pAttachEntity->kill(false);
         mFXAttachment[i].invalidate();
      }
   }
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionDaze::init()
{
   if (!BAction::init())
      return(false);

   mDazedDuration = 10000;
   mMovementModifier = 1.0f;
   mSmartTarget = false;
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionDaze::setState(BActionState state)
{
//-- FIXING PREFIX BUG ID 1684
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   syncUnitActionData("BSquadActionDaze::setState owner id", mpOwner->getID().asLong());
   syncUnitActionData("BSquadActionDaze::setState setState", state);

   switch (state)
   {
   case cStateWorking:
      mFXAttachment.resize(pSquad->getNumberChildren());
      daze();
      break;
   case cStateDone:
      undaze();
      break;
   }
   return (BAction::setState(state));
}
//==============================================================================
//==============================================================================
void BSquadActionDaze::resetDaze(float dazeDuration)
{
   syncUnitActionData("BSquadActionDaze::resetDaze owner id", mpOwner->getID().asLong());
   syncUnitActionData("BSquadActionDaze::resetDaze dazeDuration", dazeDuration);
   mDazedDuration = (DWORD)dazeDuration;
}

//==============================================================================
//==============================================================================
bool BSquadActionDaze::update(float elapsed)
{
   BASSERT(mpOwner);

   syncUnitActionData("BSquadActionDaze::update owner id", mpOwner->getID().asLong());
   syncUnitActionData("BSquadActionDaze::update mDazedDuration", mDazedDuration);

   switch (mState)
   {
   case cStateNone:
      setState(cStateWorking);
      break;

   case cStateWorking:
      {
         DWORD lastUpdateLength = gWorld->getLastUpdateLength();

         if( mDazedDuration <= lastUpdateLength )
         {
            mDazedDuration = 0;
            setState(cStateDone);
         }
         else
         {
            mDazedDuration -= lastUpdateLength;
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
void BSquadActionDaze::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   
}

//==============================================================================
//==============================================================================
void BSquadActionDaze::setupDazeAttributes(float duration, float movementModifier, bool smartTarget)
{
   syncUnitActionData("BSquadActionDaze::setupDazeAttributes duration", duration);
   syncUnitActionData("BSquadActionDaze::setupDazeAttributes movementModifier", movementModifier);

   mDazedDuration = (DWORD)duration;
   mMovementModifier = movementModifier;
   mSmartTarget = smartTarget;
}
//==============================================================================
//==============================================================================
void BSquadActionDaze::daze()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);
   
   syncUnitActionData("BSquadActionDaze::daze owner id", mpOwner->getID().asLong());

   if (mMovementModifier < cFloatCompareEpsilon)
      pSquad->setFlagDazeImmobilized(true);
   else
      pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierAbility, mMovementModifier, false);

   pSquad->setFlagAttackBlocked(true);

   for (uint i = 0; i < pSquad->getNumberChildren(); ++i)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (!pUnit)
         continue;

      if (mFXAttachment[i].isValid())
      {
         BEntity *pAttachEntity = gWorld->getEntity(mFXAttachment[i]);
         if (pAttachEntity != NULL)
            pAttachEntity->kill(false);
         mFXAttachment[i].invalidate();
      }

      const BProtoObject *pSquadProtoObject = pSquad->getProtoObject();
      bool isInfantry = (pSquadProtoObject)?pSquadProtoObject->isType(gDatabase.getOTIDInfantry()):false;

      int dazeProtoID;
      if (isInfantry)
         dazeProtoID = gDatabase.getOTIDStun();//gDatabase.getProtoObject("fx_stunEffect");
      else
         dazeProtoID = gDatabase.getOTIDEmp();//gDatabase.getProtoObject("fx_empEffect");

      mFXAttachment[i] = pUnit->addAttachment(dazeProtoID);

      if (pUnit->hasAnimation(cAnimTypeStunned))
      {
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeStunned);
         pUnit->computeAnimation();
      }
   }

   // DMG NOTE: Need to add an effect here for being dazed...
   // DMG NOTE: Need to add an animation for being dazed
}

//==============================================================================
//==============================================================================
void BSquadActionDaze::undaze()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   syncUnitActionData("BSquadActionDaze::undaze owner id", mpOwner->getID().asLong());

   if (mMovementModifier < cFloatCompareEpsilon)
      pSquad->setFlagDazeImmobilized(false);
   else
      pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierAbility, mMovementModifier, true);

   pSquad->setFlagAttackBlocked(false);

   if (mSmartTarget)
   {
      pSquad->decrementSmartTargetReference(BSquad::cSmartTargetDaze);
   }

   for (uint i = 0; i < pSquad->getNumberChildren(); ++i)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, pUnit->getIdleAnim());
         pUnit->computeAnimation();
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionDaze::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, DWORD, mDazedDuration);
   GFWRITEARRAY(pStream, BEntityID, mFXAttachment, uint8, 10);
   GFWRITEVAR(pStream, float, mMovementModifier);
   GFWRITEBITBOOL(pStream, mSmartTarget);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionDaze::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, DWORD, mDazedDuration);
   GFREADARRAY(pStream, BEntityID, mFXAttachment, uint8, 10);
   GFREADVAR(pStream, float, mMovementModifier);
   GFREADBITBOOL(pStream, mSmartTarget);
   return true;
}
