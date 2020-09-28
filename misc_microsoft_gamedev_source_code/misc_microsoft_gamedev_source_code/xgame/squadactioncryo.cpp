//==============================================================================
// squadactionCryo.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionCryo.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "SimOrderManager.h"
#include "ability.h"
#include "tactic.h"
#include "protosquad.h"
#include "physicsinfomanager.h"
#include "UnitActionPhysics.h"
#include "physics.h"
#include "damagetemplatemanager.h"
#include "visual.h"
#include "damagetemplate.h"
#include "damageaction.h"
#include "object.h"
#include "powerhelper.h"
#include "achievementmanager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionCryo, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionCryo::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(pOwner);

//-- FIXING PREFIX BUG ID 2032
   const BSquad* pSquad = pOwner->getSquad();
//--
   BASSERT(pSquad);

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   BASSERT(pProtoSquad);

   mMaxCryoPoints = pProtoSquad->getCryoPoints();
   mCryoPoints = mMaxCryoPoints;

   // listen for events
   for (uint i = 0; i < pSquad->getNumberChildren(); ++i)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
         pUnit->addEventListener(this);
   }

   return BAction::connect(pOwner, pOrder);
}
//==============================================================================
//==============================================================================
void BSquadActionCryo::disconnect()
{
   // this will correctly end the current state
   setCryoState(CryoStateNone);

   // purge all the damage tracker stuff
   updateCryoPoints(mMaxCryoPoints);

   // be sure to remove any textures
   BSquad* pSquad = (mpOwner) ? mpOwner->getSquad() : NULL;
   if (pSquad)
   {
      const BEntityIDArray& childUnitIDs = pSquad->getChildList();
      uint numChildren = childUnitIDs.getSize();
      for (uint j=0; j<numChildren; j++)
      {
         BUnit* pUnit = gWorld->getUnit(childUnitIDs[j]);

         if (pUnit)
         {
            BAdditionalTexture* pIceTexture = pUnit->getAdditionalTexture(cATLerp);
            if (pIceTexture && pIceTexture->Texture == BMeshEffectTextures::cTTIce)
               pUnit->removeAdditionalTexture(cATLerp);
            pUnit->removeEventListener(this);
         }
      }
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionCryo::init()
{
   if (!BAction::init())
      return(false);

   mCryoPoints = 0.0f;
   mMaxCryoPoints = 0.0f;
   mCryoState = CryoStateNone;
   mTimeUntilThaw = 0.0f;

   mFreezingThawTime = gDatabase.getTimeFreezingToThaw();
   mFrozenThawTime = gDatabase.getTimeFrozenToThaw();
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionCryo::setState(BActionState state)
{
   return (BAction::setState(state));
}
//==============================================================================
//==============================================================================
bool BSquadActionCryo::update(float elapsed)
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   BASSERT(pProtoSquad);

   syncUnitActionData("BSquadActionCryo::update owner id", mpOwner->getID().asLong());

   impulseAirUnits(*pSquad, elapsed);

   if (mCryoState == CryoStateThawing)
   {
      updateCryoPoints(mCryoPoints + (gDatabase.getDefaultThawSpeed() * elapsed));

      if (mCryoPoints >= mMaxCryoPoints)
         setCryoState(CryoStateNone);
   }
   else if (mCryoState == CryoStateFreezing || mCryoState == CryoStateFrozen)
   {
      mTimeUntilThaw -= elapsed;
      if (mTimeUntilThaw <= 0.0f)
         setCryoState(CryoStateThawing);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::addCryo(float cryoAmount)
{
//-- FIXING PREFIX BUG ID 2034
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   BASSERT(pProtoSquad);

   // we can't cryo anything playing it's birth anim
   if (!pSquad->hasCompletedBirth())
      return;

   if (mCryoPoints > 0.0f)
      updateCryoPoints(mCryoPoints - cryoAmount);

   if (mCryoState == CryoStateFreezing)
   {
      if (mCryoPoints <= 0.0f)
         setCryoState(CryoStateFrozen);

      mTimeUntilThaw = mFreezingThawTime;
   }
   else if (mCryoState == CryoStateFrozen)
   {
      mTimeUntilThaw = mFrozenThawTime;
   }
   else if (mCryoState == CryoStateThawing || mCryoState == CryoStateNone)
   {
      if (mCryoPoints <= 0.0f)
      {
         setCryoState(CryoStateFrozen);
         mTimeUntilThaw = mFrozenThawTime;
      }
      else
      {
         setCryoState(CryoStateFreezing);
         mTimeUntilThaw = mFreezingThawTime;
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::setCryoState(BCryoState state)
{
   if (mCryoState == state)
      return;

   if (mCryoState == CryoStateFreezing)
      endFreezing();
   else if (mCryoState == CryoStateFrozen)
      endFrozen();
   else if (mCryoState == CryoStateThawing)
      endThawing();

   if (state == CryoStateFreezing)
      startFreezing();
   else if (state == CryoStateFrozen)
      startFrozen();
   else if (state == CryoStateThawing)
      startThawing();
   else if (state == CryoStateNone)
      setState(cStateDone);

   mCryoState = state;
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::startFreezing()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   // set the movement modifier 
   pSquad->setAbilityMovementSpeedModifier(gDatabase.getFreezingSpeedModifier(), false);

   // set the damage multiplier
   pSquad->setAbilityDamageTakenModifier(gDatabase.getFreezingDamageModifier(), false);

   mTimeUntilThaw = gDatabase.getTimeFreezingToThaw();
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::endFreezing()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   // unset the movement modifier 
   pSquad->setAbilityMovementSpeedModifier(gDatabase.getFreezingSpeedModifier(), true);

   // unset the damage multiplier
   pSquad->setAbilityDamageTakenModifier(gDatabase.getFreezingDamageModifier(), true);
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::startFrozen()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   // immobilize the squad
   pSquad->setFlagCryoFrozen(true);
   pSquad->setFlagAttackBlocked(true);

   // set the larger damage multiplier
   pSquad->setAbilityDamageTakenModifier(gDatabase.getFrozenDamageModifier(), false);

   // add the frozen particle and freeze the animation state
   const BEntityIDArray& childUnitIDs = pSquad->getChildList();
   uint numChildren = childUnitIDs.getSize();
   for (uint j=0; j<numChildren; j++)
   {
      BUnit* pUnit = gWorld->getUnit(childUnitIDs[j]);
      if (pUnit)
      {
         pUnit->setFlagShatterOnDeath(true);
         pUnit->setAnimationEnabled(false, true);
         pUnit->toggleAddResource(false);

         if (numChildren > mSnowMounds.getSize() && pUnit->getProtoObject() && pUnit->getProtoObject()->getMovementType() != cMovementTypeAir)
         {
            BObjectCreateParms parms;
            parms.mProtoObjectID = getSnowMoundProtoId(*pUnit);
            parms.mPosition = pUnit->getPosition();
            parms.mForward = pUnit->getForward();
            parms.mRight = pUnit->getRight();
            parms.mPlayerID = pUnit->getPlayerID();
            BObject* pSnowMoundObject = gWorld->createObject(parms);
            if (pSnowMoundObject)
            {
               mSnowMounds.add(pSnowMoundObject->getID());
               pSnowMoundObject->setAnimationState(BObjectAnimationState::cAnimationStateTrain, cAnimTypeTrain, true);
               pSnowMoundObject->computeAnimation();
            }
         }
      }
   }

   mTimeUntilThaw = gDatabase.getTimeFrozenToThaw();
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::endFrozen()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   // mobilize the squad
   pSquad->setFlagCryoFrozen(false);
   pSquad->setFlagAttackBlocked(false);

   // unset the larger damage multiplier
   pSquad->setAbilityDamageTakenModifier(gDatabase.getFrozenDamageModifier(), true);

   // remove the frozen particle and unfreeze the animation state
   const BEntityIDArray& childUnitIDs = pSquad->getChildList();
   uint numChildren = childUnitIDs.getSize();
   for (uint j=0; j<numChildren; j++)
   {
      BUnit* pUnit = gWorld->getUnit(childUnitIDs[j]);
      if (pUnit)
      {
         pUnit->setFlagShatterOnDeath(false);
         pUnit->setAnimationEnabled(true, true);
         pUnit->toggleAddResource(true);
      }
   }

   for (uint i = 0; i < mSnowMounds.getSize(); ++i)
   {
      BObject* pObject = gWorld->getObject(mSnowMounds[i]);
      if (pObject)
      {
         pObject->setLifespan(2500);
         pObject->setAnimationState(BObjectAnimationState::cAnimationStateDeath, cAnimTypeDeath);
         pObject->computeAnimation();
      }
   }
   mSnowMounds.clear();
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::startThawing()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   // set the movement modifier 
   pSquad->setAbilityMovementSpeedModifier(gDatabase.getFreezingSpeedModifier(), false);

   // set the damage multiplier
   pSquad->setAbilityDamageTakenModifier(gDatabase.getFreezingDamageModifier(), false);

   mTimeUntilThaw = 0.0f;
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::endThawing()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   // unset the movement modifier 
   pSquad->setAbilityMovementSpeedModifier(gDatabase.getFreezingSpeedModifier(), true);

   // unset the damage multiplier
   pSquad->setAbilityDamageTakenModifier(gDatabase.getFreezingDamageModifier(), true);
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::cryoKillSquad(BPlayer *pPowerPlayer, BEntityID killerID)
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();

   if (mCryoPoints <= cFloatCompareEpsilon && pProtoSquad && pProtoSquad->getFlagDiesWhenFrozen())
   {
      // freeze death for air units
      const BEntityIDArray& childUnitIDs = pSquad->getChildList();
      uint numChildren = childUnitIDs.getSize();
      for (uint j=0; j<numChildren; j++)
      {
         BObject* pPhysReplacement = NULL;
         BUnit* pUnit = gWorld->getUnit(childUnitIDs[j]);
         if (pUnit)
         {
            const BProtoObject* pProtoObject = pUnit->getProtoObject();
            if (pProtoObject &&  pProtoObject->getMovementType() == cMovementTypeAir)
            {
               BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pProtoObject->getPhysicsReplacementInfoID(), true);
               if (pInfo)
               {
                  // Create replacement object
                  pPhysReplacement = pUnit->createPhysicsReplacement();
                  if (pPhysReplacement && pPhysReplacement->getPhysicsObject())
                  {
                     BUnit* pPhysReplacementUnit = pPhysReplacement->getUnit();

                     // silly that we need to do this manually... physics code SO needs cleanup
                     pPhysReplacementUnit->setFlagSelectable(false);
                     pPhysReplacementUnit->setAnimationEnabled(false, true);
                     pPhysReplacementUnit->setFlagShatterOnDeath(true);

                     // give the physics replacement a max lifespan, just in case
                     pPhysReplacementUnit->setLifespan(5000);

                     BUnitActionPhysics* pAction = reinterpret_cast<BUnitActionPhysics*>(pPhysReplacementUnit->getActionByType(BAction::cActionTypeUnitPhysics));
                     if (pAction)
                     {
                        pAction->setFlagCompleteOnInactivePhysics(false);
                        pAction->setFlagCompleteOnFirstCollision(true);
                        pPhysReplacementUnit->getPhysicsObject()->addGameCollisionListener(pAction);
                        pAction->setFlagCollisionListener(true);
                     }

                     // Destroy this original unit   
                     pUnit->setKilledByID(killerID);
                     pUnit->setFlagKilledByLeaderPower(true);
                     pUnit->kill(true);

                     // Make sure this object gets updated now that we need to destroy it
                     pUnit->setFlagNoUpdate(false);
                  }
               }
            }

            if (!pPhysReplacement)
            {
               pUnit->setKilledByID(killerID);
               pUnit->setFlagKilledByLeaderPower(true);
               pUnit->kill(false);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::updateUnitCryoEffect(BEntityID unitId, float currentCryoPercent, float newCryoPercent, bool affectTexture)
{
   BUnit* pUnit = gWorld->getUnit(unitId);
   const BDamageTemplate *pTemplate = (pUnit && pUnit->getVisual()) ? gDamageTemplateManager.getDamageTemplate(pUnit->getVisual()->getDamageTemplateID()) : NULL;

   if (pUnit)
   {
      if (affectTexture)
      {
         // update the actual cryo texture swap before anything else 
         BAdditionalTexture* pIceTexture = pUnit->getAdditionalTexture(cATLerp);
         if (!pIceTexture)
         {
            BAdditionalTexture texture;
            texture.RenderType = cATLerp;
            texture.Texture = BMeshEffectTextures::cTTIce;
            texture.TexUVScale = 0.1f;
            texture.ShouldBeCopied = true;
            pIceTexture = pUnit->addAdditionalTexture(texture);
         }
         BASSERT(pIceTexture);
         pIceTexture->TexInten = Math::Clamp(1.0f - newCryoPercent, 0.0f, 1.0f);
         if (newCryoPercent >= cFloatCompareEpsilon)
            pIceTexture->TexInten *= 0.75f;
      }

      if (pTemplate)
      {
         long totalPercentageBasedEvents = pTemplate->getCryoPercentageBasedEventCount();

         long newPercentageBasedEvents = ceil(newCryoPercent * (totalPercentageBasedEvents - 1));
         long currentPercentageBasedEvents = ceil(currentCryoPercent * (totalPercentageBasedEvents - 1));

#ifdef SYNC_UnitDetail
         syncUnitDetailData("BSquadActionCryo::updateCryoPoints currentPercentageBasedEvents", currentPercentageBasedEvents);
         syncUnitDetailData("BSquadActionCryo::updateCryoPoints newPercentageBasedEvents", newPercentageBasedEvents);
#endif
         if(newPercentageBasedEvents < currentPercentageBasedEvents)
         {
            // We are taking damage - need to execute events
            //
            for(long i = currentPercentageBasedEvents; i > newPercentageBasedEvents; i--)
            {
               const BDamageEvent* pEvent = pTemplate->getCryoPercentageBasedEvent(totalPercentageBasedEvents - i);

               if(pEvent)
               {
                  // Execute all actions on this event
                  long actioncount = pEvent->getActionCount();

#ifdef SYNC_UnitDetail
                  syncUnitDetailData("BSquadActionCryo::updateCryoPoints actioncount", actioncount);
#endif

                  for (long actionindex = 0; actionindex < actioncount; actionindex++)
                  {
                     const BDamageAction* pAction = pEvent->getAction(actionindex);

                     // 1000.0f is the magic number? WTF? Mirroring what unit.cpp does for now... :( 
                     pAction->execute(pUnit, true, NULL, 1000.0f, true, NULL);
                  }
               }
            }
         }
         else if (newPercentageBasedEvents > currentPercentageBasedEvents)
         {
            // We are healing - need to restore events
            //
            for(long i = currentPercentageBasedEvents; i < newPercentageBasedEvents; i++)
            {
               const BDamageEvent* pEvent = pTemplate->getCryoPercentageBasedEvent(totalPercentageBasedEvents - i - 1);

               if(pEvent)
               {
                  // Undo Execute all actions on this event
                  long actioncount = pEvent->getActionCount();

#ifdef SYNC_UnitDetail
                  syncUnitDetailData("BDamageTracker::updatePercentageBaseDamage actioncount", actioncount);
#endif

                  for (long actionindex = 0; actionindex < actioncount; actionindex++)
                  {
                     const BDamageAction* pAction = pEvent->getAction(actionindex);

                     pAction->undoExecuteSilent(pUnit);
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::updateCryoPoints(float newCryoPoints)
{
   // this can get called on disconnect, so be safe and don't assert - just bail
   if (!mpOwner)
      return;

   BSquad* pSquad = mpOwner->getSquad();
   if (!pSquad)
      return;

   if (newCryoPoints < 0.0f)
      newCryoPoints = 0.0f;
   if (newCryoPoints > mMaxCryoPoints)
      newCryoPoints = mMaxCryoPoints;

   if(mCryoPoints == newCryoPoints)
      return;

   float currentCryoPercent = mCryoPoints / mMaxCryoPoints;
   float newCryoPercent = newCryoPoints / mMaxCryoPoints;

#ifdef SYNC_UnitDetail
   syncUnitDetailData("BSquadActionCryo::updateCryoPoints currentCryoPercent", currentCryoPercent);
   syncUnitDetailData("BSquadActionCryo::updateCryoPoints newCryoPercent", newCryoPercent);
#endif

   const BEntityIDArray& childUnitIDs = pSquad->getChildList();
   uint numChildren = childUnitIDs.getSize();
   for (uint j=0; j<numChildren; j++)
      updateUnitCryoEffect(childUnitIDs[j], currentCryoPercent, newCryoPercent, true);

   mCryoPoints = newCryoPoints;
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::impulseAirUnits(BSquad& squad, float elapsedTime)
{
   // 1.5 per second on average if frozen, 1 per second otherwise (freezing)
   int pctChance = (mCryoState == CryoStateFrozen) ? 150 : 100;
   pctChance *= elapsedTime;

   // if we fail the random, we're done
   if(getRandRange(cSimRand, 0, 100) > pctChance)
      return;

   uint numChildren = squad.getNumberChildren();
   for (uint i = 0; i < numChildren; ++i)
   {
      BUnit* pChildUnit = gWorld->getUnit(squad.getChild(i));
      if (pChildUnit)
         BPowerHelper::impulsePhysicsAirUnit(*pChildUnit, 0.5f);
   }
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::onUnitAdded(BEntityID unitId)
{
   // if a unit got added and we're frozen, freeze them too
   if (mCryoState == CryoStateFrozen || mCryoState == CryoStatePendingFrozenKill)
      updateUnitCryoEffect(unitId, 0.01f, 0.0f, true);
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::onUnitKilled(BEntityID unitId)
{
   // if we lost a unit and we're frozen, unapply the last frozen effect
   if (mCryoState == CryoStateFrozen || mCryoState == CryoStatePendingFrozenKill)
      updateUnitCryoEffect(unitId, 0.0f, 0.01f, false);
}

//==============================================================================
//==============================================================================
BProtoObjectID BSquadActionCryo::getSnowMoundProtoId(const BUnit& unit)
{
   float obsSize = unit.getObstructionRadius();
   if (obsSize < 2.9f)
      return gDatabase.getSmallSnowMoundId();
   else if (obsSize < 7.9f)
      return gDatabase.getMediumSnowMoundId();
   else
      return gDatabase.getLargeSnowMoundId();
}

//==============================================================================
//==============================================================================
void BSquadActionCryo::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!mpOwner)
      return;

   switch (eventType)
   {
      case BEntity::cEventSquadUnitAdded:
      {
         onUnitAdded((BEntityID)data);
         break;
      }
      case BEntity::cEventKilled:
      {
         onUnitKilled(senderID);
         break;
      }
      default:
      {
         break;
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionCryo::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mCryoPoints);
   GFWRITEVAR(pStream, float, mMaxCryoPoints);
   GFWRITEVAR(pStream, BCryoState, mCryoState);
   GFWRITEVAR(pStream, float, mTimeUntilThaw);
   GFWRITEVAR(pStream, float, mFreezingThawTime);
   GFWRITEVAR(pStream, float, mFrozenThawTime);
   GFWRITEARRAY(pStream, BEntityID, mSnowMounds, uint8, 200);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionCryo::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mCryoPoints);
   GFREADVAR(pStream, float, mMaxCryoPoints);
   GFREADVAR(pStream, BCryoState, mCryoState);
   GFREADVAR(pStream, float, mTimeUntilThaw);
   if (BAction::mGameFileVersion >= 6)
   {
      GFREADVAR(pStream, float, mFreezingThawTime);
      GFREADVAR(pStream, float, mFrozenThawTime);
   }
   else 
   {
      mFreezingThawTime = 1.0f;
      mFrozenThawTime = 1.0f;
   }
   if (BAction::mGameFileVersion >= 11)
      GFREADARRAY(pStream, BEntityID, mSnowMounds, uint8, 200);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionCryo::postLoad(int saveType)
{
   if (!BAction::postLoad(saveType))
      return false;

   BSquad* pSquad = (mpOwner) ? mpOwner->getSquad() : NULL;
   if (pSquad)
   {
      const BEntityIDArray& childUnitIDs = pSquad->getChildList();
      uint numChildren = childUnitIDs.getSize();
      for (uint j=0; j<numChildren; j++)
      {
         BUnit* pUnit = gWorld->getUnit(childUnitIDs[j]);
         if (pUnit)
            pUnit->addEventListener(this);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionCryo::savePtr(BStream* pStream) const
{
   GFWRITEACTIONPTR(pStream, this);
   return true;
}
