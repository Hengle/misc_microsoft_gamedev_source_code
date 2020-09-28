//==============================================================================
// unitactionrevive.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "config.h"
#include "configsgame.h"
#include "entity.h"
#include "protoobject.h"
#include "unitactionrevive.h"
#include "unitactionunderattack.h"
#include "tactic.h"
#include "uimanager.h"
#include "unit.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionRevive, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionRevive::init(void)
{
   if (!BAction::init())
      return(false);

   mReviveTimer = 0.0f;
   mHibernateTimer = 0.0f;
   mDamageDealt = true;
   mInTransition = false;
   mHealthState = -1;
   mDormantCalloutID = -1;
   mNumHealthStates = 0;
   mNextHealthState = 0;

   mHealthAnim[0] = gVisualManager.getAnimType("AHealthIdle");
   mHealthAnim[1] = gVisualManager.getAnimType("BHealthIdle");
   mHealthAnim[2] = gVisualManager.getAnimType("CHealthIdle");
   mHealthAnim[3] = gVisualManager.getAnimType("DHealthIdle");
   mHealthAnim[4] = gVisualManager.getAnimType("EHealthIdle");
   mHealthAnim[5] = gVisualManager.getAnimType("FHealthIdle");

   mShrinkAnim[0] = gVisualManager.getAnimType("ShrinkAToB");
   mShrinkAnim[1] = gVisualManager.getAnimType("ShrinkBToC");
   mShrinkAnim[2] = gVisualManager.getAnimType("ShrinkCToD");
   mShrinkAnim[3] = gVisualManager.getAnimType("ShrinkDToE");
   mShrinkAnim[4] = gVisualManager.getAnimType("ShrinkEToF");

   mGrowAnim[0] = gVisualManager.getAnimType("GrowBToA");
   mGrowAnim[1] = gVisualManager.getAnimType("GrowCToB");
   mGrowAnim[2] = gVisualManager.getAnimType("GrowDToC");
   mGrowAnim[3] = gVisualManager.getAnimType("GrowEToD");
   mGrowAnim[4] = gVisualManager.getAnimType("GrowFToE");

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionRevive::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);     

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());
   pUnit->setFlagIsHibernating(false);

   setState(cStateNone);
   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionRevive::disconnect()
{
   BUnit* pUnit = (mpOwner) ? mpOwner->getUnit() : NULL;
   if (pUnit)
      pUnit->setFlagIsHibernating(false);

   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionRevive::setState(BActionState state)
{
   return BAction::setState(state);
}


//==============================================================================
//==============================================================================
bool BUnitActionRevive::update(float elapsed)
{
   BASSERT(mpOwner);
   if (!mpOwner)
      return (true);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);


   switch (mState)
   {
   case cStateNone:
      {
         // Set the FlagDiesAtZeroHP to false (once, in the first update) and then leave it alone
         pUnit->setFlagDiesAtZeroHP(false);
         setState(cStateWorking);

         // Count the legitimate health states on this owner
         for(int i=0; i<MAX_HEALTH_STATES; i++)
         {
            if (pUnit->hasAnimation(mHealthAnim[i]))
               mNumHealthStates++;
         }

         // Set initial health state
         if (mHealthState == -1)
         {
            float currHealthRatio = pUnit->getHPPercentage();
            float healthStateFraction = 1.0f / ((float) (mNumHealthStates - 1));

            // Start idle animation based on current health ratio
            for (int i=0; i<mNumHealthStates; i++)
            {
               if ((currHealthRatio >= (1.0f - (float)(i+1)*healthStateFraction)) && pUnit->hasAnimation(mHealthAnim[i]))
               {
                  pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, mHealthAnim[i], false, false, -1, false);
                  mHealthState = mNextHealthState = i;
                  break;
               }
            }
         }
         break;
      }
   case cStateWorking:
      {
         if (mDamageDealt && (pUnit->getHitpoints() < pUnit->getHPMax()))
         {
            if (pUnit->getHitpoints() <= 0.0f)
            {
               mHibernateTimer = mpProtoAction->getHibernateDelay();
               pUnit->setFlagIsHibernating(true);
            }

            mReviveTimer = mpProtoAction->getReviveDelay();
            setState(cStateWait);
         }

         if (pUnit->getHitpoints() < pUnit->getHPMax())
         {
            float hp = pUnit->getHitpoints() + (mpProtoAction->getReviveRate() * elapsed);
            if (hp > pUnit->getHPMax())
               hp = pUnit->getHPMax();

            pUnit->setHitpoints(hp);
         }
         break;
      }
   case cStateWait:
      {
         if (mDamageDealt)
         {
            mReviveTimer = mpProtoAction->getReviveDelay();

            if (pUnit->getHitpoints() <= 0.0f)
            {
               mHibernateTimer = mpProtoAction->getHibernateDelay();
               pUnit->setFlagNotAttackable(true);
               pUnit->setFlagIsHibernating(true);
            }
         }
         else
         {
            if (mpProtoAction->getReviveDelay() > 0.0f)
               mReviveTimer -= elapsed;
            if (mpProtoAction->getHibernateDelay() > 0.0f)
               mHibernateTimer -= elapsed;
         }

         bool doneHibernating = (mHibernateTimer <= 0.0f) && (mpProtoAction->getHibernateDelay() > 0.0f);
         if (pUnit->isHibernating() && doneHibernating)
         {
            pUnit->setFlagIsHibernating(false);
            gUIManager->getCalloutUI()->removeCallout(mDormantCalloutID);
         }

         if ((mReviveTimer <= 0.0f) && doneHibernating && (mpProtoAction->getReviveDelay() > 0.0f))
         {
            setState(cStateWorking);
            pUnit->setFlagNotAttackable(false);
         }

         break;
      }
   }

   // Manage animation transitions.  Remove this call if idle anims shouldn't be interrupted.
   if (!mInTransition)
      handleAnimationState();

   mDamageDealt = false;

   if(!BAction::update(elapsed))
      return (false);
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRevive::handleAnimationState()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   float currHealthRatio = pUnit->getHPPercentage();
   float healthStateFraction = 1.0f / ((float) (mNumHealthStates - 1));
   long newAnimType = -1;

   // Start idle animation based on current health ratio
   if (mInTransition)
   {
      mInTransition = false;

      mHealthState = mNextHealthState;
      BASSERT(mHealthState < mNumHealthStates);
      newAnimType = mHealthAnim[mHealthState];
   }

   // Determine whether transition anim is needed.  This is allowed to override the idle anim set above.
   if ( ((currHealthRatio <= cFloatCompareEpsilon) || (currHealthRatio < (1.0f - (float)(mHealthState+1)*healthStateFraction - cFloatCompareEpsilon))) && ((mHealthState+1) < mNumHealthStates) )
   {
      // Shrink
      newAnimType = mShrinkAnim[mHealthState];
      mNextHealthState = mHealthState + 1;
      mInTransition = true;
   }
   else if ((currHealthRatio > (1.0f - (float)(mHealthState)*healthStateFraction + cFloatCompareEpsilon)) && (mHealthState > 0))
   {
      // Grow
      newAnimType = mGrowAnim[mHealthState-1];
      mNextHealthState = mHealthState - 1;
      mInTransition = true;
   }

   // Set new anim
   if (newAnimType != -1)
   {
      BASSERT(pUnit->hasAnimation(newAnimType));
      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, newAnimType, false, false, -1, false);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRevive::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;


   switch (eventType)
   {
      case BEntity::cEventAnimEnd:
      case BEntity::cEventAnimLoop:
      {
         handleAnimationState();
      }
      default:
         break;
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionRevive::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mReviveTimer);
   GFWRITEVAR(pStream, float, mHibernateTimer);
   GFWRITEVAR(pStream, long, mHealthState);
   if (BAction::mGameFileVersion >= 28)
      GFWRITEVAR(pStream, long, mNextHealthState);
   GFWRITEVAR(pStream, long, mNumHealthStates);

   // Don't need to save data that's initialized when the action is created and never changes
   //long mHealthAnim[MAX_HEALTH_STATES];
   //long mShrinkAnim[MAX_HEALTH_STATES-1];
   //long mGrowAnim[MAX_HEALTH_STATES-1];

   GFWRITEBITBOOL(pStream, mDamageDealt);
   GFWRITEBITBOOL(pStream, mInTransition);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRevive::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mReviveTimer);
   GFREADVAR(pStream, float, mHibernateTimer);
   GFREADVAR(pStream, long, mHealthState);
   if (BAction::mGameFileVersion >= 28)
      GFREADVAR(pStream, long, mNextHealthState);
   GFREADVAR(pStream, long, mNumHealthStates);

   // Don't need to save data that's initialized when the action is created and never changes
   //long mHealthAnim[MAX_HEALTH_STATES];
   //long mShrinkAnim[MAX_HEALTH_STATES-1];
   //long mGrowAnim[MAX_HEALTH_STATES-1];

   GFREADBITBOOL(pStream, mDamageDealt);
   bool tempBool = false;
   if (BAction::mGameFileVersion < 43)
      GFREADBITBOOL(pStream, tempBool);

   GFREADBITBOOL(pStream, mInTransition);
   return true;
}
