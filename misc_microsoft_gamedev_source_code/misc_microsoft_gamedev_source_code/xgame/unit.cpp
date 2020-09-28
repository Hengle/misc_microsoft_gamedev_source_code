//==============================================================================
// unit.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "unit.h"
#include "action.h" 
#include "actionlist.h"
#include "game.h"
#include "generaleventmanager.h"
#include "kb.h"
#include "unitactionmove.h"
#include "syncmacros.h"
#include "world.h"
#include "obstructionmanager.h"
#include "minimap.h"
#include "protoobject.h"
#include "protosquad.h"
#include "protopower.h"
#include "squad.h"
#include "actionmanager.h" 
#include "physics.h"
#include "techtree.h"
#include "triggermanager.h"
#include "unitactionbuilding.h"
#include "unitactionrevive.h"
#include "visualmanager.h"
#include "visual.h"
#include "worldsoundmanager.h"
#include "usermanager.h"
#include "user.h"
#include "dopple.h"
#include "config.h"
#include "configsgame.h"
#include "tactic.h"
#include "HPBar.h"
#include "placementrules.h"
#include "visiblemap.h" 
#include "team.h"
#include "unitactionammoregen.h"
#include "entityactionidle.h"
#include "selectionmanager.h"
#include "xgranny.h"
#include "unitactionunderattack.h"
#include "unitactionungarrison.h"
#include "decalManager.h"
#include "battle.h"
#include "unitactionavoidcollisionair.h"
#include "unitactionplayblockinganimation.h"
#include "uimanager.h"
#include "renderDraw.h"
#include "damagetemplatemanager.h"
#include "damagetemplate.h"
#include "damageaction.h"
#include "physicsinfomanager.h"
#include "physicsinfo.h"
#include "unitactionchangemode.h"
#include "unitactionDOT.h"
#include "unitactionmoveair.h"
#include "unitactionmovewarthog.h"
#include "unitactiongarrison.h"
#include "unitactionphysics.h"
#include "unitactiondeath.h"
#include "unitactionrevealtoteam.h"
#include "corpsemanager.h"
#include "unitactioninfectdeath.h"
#include "unitactionhitch.h"
#include "unitactionunhitch.h"
#include "unitactionthrown.h"
#include "unitactiondodge.h"
#include "unitactiondeflect.h"
#include "unitactionplasmashieldgen.h"
#include "unitactionhotdrop.h"
#include "physicsobjectblueprint.h"
#include "gamedirectories.h"
#include "ability.h"
#include "unitactionplayattachmentanims.h"
#include "SimOrderManager.h"
#include "unitactionrangedattack.h"
#include "unitactiondetonate.h"
#include "squadactionmove.h"
#include "weapontype.h"
#include "particlegateway.h"
#include "unitactionherodeath.h"
#include "unitactionjump.h"
#include "unitactionpointblankattack.h"
#include "scenario.h"
#include "deathmanager.h"
#include "squadactionwork.h"
#include "unitactionchargedrangedattack.h"
#include "unitactioncoreslide.h"
#include "simhelper.h"
#include "pather.h"
#include "alert.h"
#include "achievementmanager.h"
#include "scoremanager.h"
#include "skullmanager.h"
#include "gamesettings.h"

// xvince
#include "vincehelper.h"

GFIMPLEMENTVERSION(BUnit, 7);
enum 
{
   cSaveMarkerUnit1=10000,
};

//#define DEBUGOPPS

#ifndef _MOVE4
#define _MOVE4
#endif

#ifndef BUILD_FINAL
BEntityID sDebugUnitTempID;

//#define DEBUG_MOVE4
#endif

#ifdef DEBUG_MOVE4
#define debugMove4 sDebugUnitTempID=mID, dbgUnitInternalTempID
#else
#define debugMove4 __noop
#endif

//==============================================================================
// syncAllBonesTest()
//==============================================================================
void syncAllBonesTest(BVisualItem *pVisual)
{
   int numBones = pVisual->getNumBones();
   BVector pos;
   for(long i=0; i<numBones; i++)
   {
      pVisual->getBone(i, &pos);

      #ifdef SYNC_Anim
         syncAnimData("BUnit::update bonePosition", pos);
      #endif
   }

   // get all attachments
   int numAttachments = pVisual->mAttachments.getNumber();
   for(int i = 0; i < numAttachments; i++)
   {
      BVisualItem *pAttachment = pVisual->mAttachments[i];
      syncAllBonesTest(pAttachment);
   }
}

//==============================================================================
// BUnit::updatePreAsync
//==============================================================================
bool BUnit::updatePreAsync(float elapsedTime)
{
   #ifdef SYNC_FinalDetail
   syncFinalDetailData("FRU ID", mID.asLong());
   syncFinalDetailData("FRU PID", mProtoID);
   syncFinalDetailData("FRU P", mPosition);
   syncFinalDetailData("FRU H", mHitpoints);
   #endif



   #ifdef SYNC_UnitDetail
   syncUnitDetailData("BUnit::update mHitpoints", mHitpoints);
   syncUnitDetailData("BUnit::update mShieldpoints", mShieldpoints);
   syncUnitDetailData("BUnit::update mResourceAmount", mResourceAmount);
   syncUnitDetailData("BUnit::update cFlagAlive", getFlagAlive());
   #endif

   //-- now update ourselves
   if (getFlagAlive())
   {
      if (mFlagTakeInfectionForm)
         takeInfectionForm();
      else if (mFlagFloodControl)
      {
         mFlagFloodControl = false;
         BSquad* pSquad = getParentSquad();
         BASSERT(pSquad);
         if(pSquad)
         {
            setFlagRemainVisible(false);
            const BProtoSquad* pOldPS = pSquad->getProtoSquad();
            pSquad->changeOwner(mInfectionPlayerID);
            const BProtoSquad* pNewPS = pSquad->getProtoSquad();
            pSquad->transform(pOldPS, pNewPS, true);
         }
      }
      else if (getFlagDiesAtZeroHP() && !getFlagDoingFatality() && mHitpoints <= 0.0f)
      {
#ifdef SYNC_Unit
         syncUnitData("BUnit::update dieAtZeroHP", mID.asLong());
#endif
         kill(false);
      }
      else if (getFlagDieAtZeroResources() && !getFlagUnlimitedResources() && mResourceAmount <= 0.0f)
      {
#ifdef SYNC_Unit
         syncUnitData("BUnit::update dieAtZeroResources", mID.asLong());
#endif
         kill(false);
      }
   }

   //if (getProtoObject()->getTurnRate() > 0.0f )
   //   updateTurn(elapsedTime);

   //-- update the root object
   if (!BObject::updatePreAsync(elapsedTime))
      return (false);

   return true;
}


//==============================================================================
// BUnit::update
//==============================================================================
bool BUnit::update( float elapsedTime ) 
{
   //-- update the root object
   if (!BObject::update(elapsedTime))
      return (false);

   // These functions only care if the unit has moved
   bool moved = (mFlagMoving || mFlagMoved);
   if (moved)
   {
      // Update corpse collision detection
      if (mFlagAlive && !mFlagFlying && mpObstructionNode)
         updateCorpseCollisions();      
   }   
   
   // Update hitched units
   if (mFlagHasHitched)
      updateHitchedUnit(elapsedTime);

   // Garrison unit?
   if (mFlagIsTypeGarrison)
   {
      // Do we contain anything?
      if (mFlagHasGarrisoned)
      {      
         // Update contained units if we must
         if (mFlagForceUpdateContainedUnits || moved || mFlagParentSquadChangingMode)
            updateContainedUnits(elapsedTime);

         // Cover unit?
         if (mFlagIsTypeCover)
            updateCover();
      }      
      // No contained units so update garrison timer
      else 
      {
         mGarrisonTime += elapsedTime;
         mContainedPop = 0.0f;
      }
   }
      
   setFlagTeleported(false );

   //Update opps.
   updateOpps(elapsedTime);

   //Check idle.
   if (mFlagAlive && !mActions.hasConflictsWithType(BAction::cActionTypeEntityIdle) && !isAnimationLocked())
   {
      if (!mpPhysicsObject || mpPhysicsObject->getType() != BPhysicsObject::cClamshell)
         doIdle();
   }

   #ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigDebugVisualSync))
   {
      if(mpVisual)
      {
         for(long i=0; i<2; i++)
         {
            long pointType=(i==0 ? cVisualPointLaunch : cVisualPointImpact);
            long pointHandle=-1;
            for(;;)
            {
               pointHandle=mpVisual->getNextPointHandle(cActionAnimationTrack, pointHandle, pointType);
               if(pointHandle==-1)
                  break;
               BVector pos;
               if(mpVisual->getPointPosition(cActionAnimationTrack, pointHandle, pos))
               {
                  #ifdef SYNC_Anim
                     syncAnimData("BUnit::update pointPoint", pointHandle);
                     syncAnimData("BUnit::update pointPosition", pos);
                  #endif
               }
            }
         }
      }
      // sync all bones 
      if(mpVisual)
      {
         syncAllBonesTest(mpVisual);
      }
   }
   #endif

   updateAttackTimers(elapsedTime);

   updateDamageTracker(elapsedTime);

   #if DPS_TRACKER
   mDmgUpdateTimer -= elapsedTime;
   if(mDmgUpdateTimer <= 0.0f)
   {
      mDmgUpdateTimer = 1.0f;
      updateDamageHistory();
   }   
   #endif

   updateFade(elapsedTime);

   // Update death replacment healing flag
   if (mFlagDeathReplacementHealing)
   {
      if (mHitpoints >= getProtoObject()->getHitpoints())
      {
         mFlagDeathReplacementHealing = false;
         if (!(getProtoObject()->getFlagInvulnerable() || (mPlayerID == 0 && getProtoObject()->getFlagInvulnerableWhenGaia())))
            mFlagInvulnerable=false;
         gWorld->notify(BEntity::cEventFullyHealed, mID, 0, 0);
      }
   }

   return (true);
}

//==============================================================================
// BUnit::updateFade
//==============================================================================
void BUnit::updateFade(float elapsedTime)
{
   float fadeDuration = getAlphaFadeDuration();
#ifdef SYNC_Unit
   syncUnitData("BUnit::updateFade fadeDuration", fadeDuration);
#endif
   if (fadeDuration > cFloatCompareEpsilon)
   {
      float percentSink = 0.5f;
      gConfig.get( cConfigPercentFadeTimeCorpseSink, &percentSink);
      float currentAlphaTime = getCurrentAlphaTime();
      if ((1.0f - (currentAlphaTime / fadeDuration)) < percentSink)
      {
         // get y extent - if we have a physics object, use that, otherwise the bb is fine
         float yExtent = 1.0;
         if (getPhysicsObject())
         {
            // Set to keyframing so physics object doesn't freak out when going into the ground
            setPhysicsKeyFramed(true);
            float yExtent = 0.0f;

            // This vis bb is going OOS right now, use the physics one below
            // Get world y extent of the physics aabb
//-- FIXING PREFIX BUG ID 4453
            const BPhysicsObject* pPO = getPhysicsObject();
//--
            if (pPO && pPO->getRigidBody())
            {
               const hkpCollidable* pCollidable = pPO->getRigidBody()->getCollidable();
               if (pCollidable && pCollidable->getShape())
               {
                  hkTransform identXform;
                  identXform.setIdentity();
                  hkAabb aabb;
                  pCollidable->getShape()->getAabb(identXform, 0.0f, aabb);
                  yExtent = aabb.m_max(1) - aabb.m_min(1);
               }
            }

#ifdef SYNC_Unit
            syncUnitData("BUnit updateFade physics object yExtent", yExtent);
#endif
         }
         else
         {
            const BBoundingBox* bbox = getVisualBoundingBox();
            yExtent = bbox->getExtents()[1];

#ifdef SYNC_Unit
            syncUnitData("BUnit updateFade visual bounding box yExtent", yExtent);
#endif
         }

         // sink
         float sinkSpeed = 2.0f;
         gConfig.get( cConfigCorpseSinkSpeed, &sinkSpeed );
         if (sinkSpeed > cFloatCompareEpsilon)
            moveWorldUp(-sinkSpeed * yExtent * elapsedTime / fadeDuration);

         // scale down
         float minScale = 0.5f;
         gConfig.get( cConfigCorpseMinScale, &minScale );
         minScale = Math::Max(minScale, 0.01f);

         // scale down the object as well - get a number from 1.0 to 0.0
         float scaleDuration = fadeDuration * percentSink;
         float scaleMultiplier = (fadeDuration - currentAlphaTime) / scaleDuration;
         scaleMultiplier = Math::Clamp(scaleMultiplier, 0.01f, 1.0f);

         float newScale = (1.0f - minScale) * scaleMultiplier;
         newScale += minScale;
         scaleMultiplier = Math::Clamp(newScale, minScale, 1.0f);

         // these must be normalized
         BVector normForward = getForward();
         BVector normUp = getUp();
         BVector normRight = getRight();
         normForward.normalize();
         normUp.normalize();
         normRight.normalize();

         BMatrix mtx;
         mtx.makeScale(newScale, newScale, newScale);
         mtx.multOrient(normForward, normUp, normRight);
         mtx.multTranslate(getPosition().x, getPosition().y, getPosition().z);
         setWorldMatrix(mtx);
      }
   }
}

//==============================================================================
// BUnit::shatter
//==============================================================================
void BUnit::shatter()
{
   // code to do whatever awesome shatter effects we want here - 
   // might just be driven by a specialized animation, though
   kill(false);
}

//==============================================================================
// BUnit::onPhysicsCompleted
//==============================================================================
bool BUnit::physicsCompleted()
{
   //======================================================================================
   // SLB: Reset bounding box hack to work around a bug. Remove this when the bug is fixed.
   BVisual *pVisual = getVisual();
   if (pVisual)
   {
      if (getProtoID() != gDatabase.getPOIDPhysicsThrownObject())
         computeAnimation();
      pVisual->resetCombinedBoundingBox();
      pVisual->computeCombinedBoundingBox();
      updateBoundingBox();
   }
   //======================================================================================

   setFlagPhysicsControl(false);
   setFlagObscurable(false);
   //pUnit->changeOwner(0);
   // Don't update animation on dead clamshells
   BPhysicsObject* pPhysicsObject = getPhysicsObject();
   BASSERT(pPhysicsObject);
   if (pPhysicsObject->getType() == BPhysicsObject::cClamshell)
      setAnimationEnabled(false, true);

   if (mFlagShatterOnDeath)
   {
      setAnimationEnabled(true, true);
      shatter();

      return true;
   }
   else if (gConfig.isDefined(cConfigEnableCorpses) && (getProtoObject()->getObjectClass() != cObjectClassBuilding) && !getProtoObject()->isType(gDatabase.getOTIDGatherable()))
   {
      // We don't want corpses to remain visible, we want them to dopple when appropriate, otherwise it could give away enemy troop positions.
      setFlagRemainVisible(false);
      //setFlagDopples(true);
#ifdef SYNC_UnitAction
      syncUnitCode("BUnitActionPhysics::setState registerCorpse");
#endif
      // Register the unit with the corpse manager. It'll tell us when it's time to bury it.
      gCorpseManager.registerCorpse(getID());
      //pUnit->revealPosition(2.0f);

      return true;
   }

   return false;
}

//==============================================================================
// BUnit::render
//==============================================================================
void BUnit::render()
{
   if (getFlagHasHPBar() && (getFlagDisplayHP() || getFlagForceDisplayHP()))
   {
      if (getFlagAlive())
         gHPBar.displayHP(*this);     

      setFlagForceDisplayHP(false);
   }

   BObject::render();
}

//==============================================================================
// Update the positions of the contained units
//==============================================================================
void BUnit::updateContainedUnits(float elapsedTime)
{
   // Update contained units
   uint numRefs = getNumberEntityRefs();
   int coverPointHandle = -1;
//-- FIXING PREFIX BUG ID 4457
   const BVisual* pVisual = getVisual();
//--

   for (uint i = 0; i < numRefs; i++)
   {
//-- FIXING PREFIX BUG ID 4456
      const BEntityRef* pRef = getEntityRefByIndex(i);
//--
      if (pRef && (pRef->mType == BEntityRef::cTypeContainUnit))
      {
         BUnit* pContainedUnit = gWorld->getUnit(pRef->mID);
         if (pContainedUnit)
         {
            if (pContainedUnit->getFlagInCover() && pVisual)
            {
               coverPointHandle = pVisual->getNextPointHandle(cActionAnimationTrack, coverPointHandle, cVisualPointCover);
               BVector result = cInvalidVector;
               BVector coverPos = cInvalidVector;
               if (pVisual->getPointPosition(cActionAnimationTrack, coverPointHandle, result))
               {
                  BMatrix mat;
                  getWorldMatrix(mat);
                  mat.transformVectorAsPoint(result, coverPos);                        
               }
            
               if (coverPos != cInvalidVector)
               {
                  #ifdef SYNC_Unit
                     syncUnitData("BUnit::updateContainedUnits 1", coverPos);
                  #endif
                  pContainedUnit->setPosition(coverPos);
               }
               else
               {
                  #ifdef SYNC_Unit
                     syncUnitData("BUnit::updateContainedUnits 2", getPosition());
                  #endif
                  pContainedUnit->setPosition(getPosition());
                  pContainedUnit->setFlagInCover(false);
               }
            }
            else
            {
               #ifdef SYNC_Unit
                  syncUnitData("BUnit::updateContainedUnits 3", getPosition());
               #endif
               pContainedUnit->setPosition(getPosition());
            }
            pContainedUnit->updateObstruction();
         }
         else  // if we have an invalid entity ref, unload it
            unloadUnit(i, false);
      }
   }
   
   const BProtoObject* pProtoObject = getProtoObject();
   bool forceUpdate = pProtoObject ? pProtoObject->getFlagForceUpdateContainedUnits() : false;
   setFlagForceUpdateContainedUnits(forceUpdate);
}

//==============================================================================
// Get the pop for the squad
//==============================================================================
float BUnit::getSquadPop() const
{
   BSquad* pParentSquad = getParentSquad();

   if (!pParentSquad)
      return 0;

   const BProtoSquad* pProtoSquad = pParentSquad->getProtoSquad();

   if (!pProtoSquad)
      return 0;
   
   if (!getPlayer())
      return 0;

   uint unitNodeCount = (uint)pProtoSquad->getNumberUnitNodes();
   float popCost = 0.0f;

   for (uint i = 0; i < unitNodeCount; i++)
   {
      const BProtoSquadUnitNode& unitNode = pProtoSquad->getUnitNode(i);
      const BProtoObject* pProtoObject = getPlayer()->getProtoObject(unitNode.mUnitType);
      uint unitPopCount = (uint)pProtoObject->getNumberPops();
      for (uint j = 0; j < unitPopCount; j++)
      {
         BPop pop = pProtoObject->getPop(j);
         popCost = popCost + (pop.mCount * unitNode.mUnitCount);
      }
   }
   popCost = floorf(popCost + 0.5f);

   return popCost;
}

//==============================================================================
// Get the pop for the squad
//==============================================================================
void BUnit::calculateContainedPop()
{
   static BSmallDynamicArray<BSquad*> containedSquads;

   containedSquads.setNumber(0);
   mContainedPop = 0;

   uint numRefs = getNumberEntityRefs();

   for (uint i = 0; i < numRefs; i++)
   {
      const BEntityRef* pRef = getEntityRefByIndex(i);
      //--
      if (pRef && (pRef->mType == BEntityRef::cTypeContainUnit))
      {
         BUnit* pContainedUnit = gWorld->getUnit(pRef->mID);
         if (pContainedUnit)
         {
            BSquad* pSquad = pContainedUnit->getParentSquad();

            if (pSquad && !containedSquads.contains(pSquad))
            {
               containedSquads.add(pSquad);
            }
         }
      }
   }

   for (uint j = 0; j < containedSquads.size(); j++)
   {
      BUnit* pLeaderUnit = containedSquads[j]->getLeaderUnit();

      if (pLeaderUnit)
      {
         mContainedPop += pLeaderUnit->getSquadPop();
      }
   }
}

//==============================================================================
// Update the position of the hitched unit
//==============================================================================
void BUnit::updateHitchedUnit(float elapsedTime)
{
   // Update hitched unit
   BEntityID hitchedUnitID = getHitchedUnit();
   BUnit* pHitchedUnit = gWorld->getUnit(hitchedUnitID);
   if (pHitchedUnit)
   {
      // Just use hitched unit's parent squad's position and orientation
//-- FIXING PREFIX BUG ID 4458
      const BSquad* pHitchedSquad = pHitchedUnit->getParentSquad();
//--
      if (pHitchedSquad)
      {
         // Don't update the unit directly if it uses squad turn radius + physics to move the unit
         if (pHitchedSquad->getFlagUpdateTurnRadius())
         {
            BPhysicsObject* pPO = pHitchedUnit->getPhysicsObject();
            if (pPO)
               pPO->forceActivate();
         }
         else
         {
            #ifdef SYNC_Unit
               syncUnitData("BUnit::updateHitchedUnit", pHitchedSquad->getPosition());
            #endif
            pHitchedUnit->setPosition(pHitchedSquad->getPosition());
            pHitchedUnit->setForward(pHitchedSquad->getForward());
            pHitchedUnit->setRight(pHitchedSquad->getRight());
            pHitchedUnit->setUp(pHitchedSquad->getUp());
            if (pHitchedUnit->getProtoObject()->getFlagOrientUnitWithGround())
            {
               pHitchedUnit->orientWithGround();
            }  
            pHitchedUnit->updateObstruction();
         }
      }

      pHitchedUnit->update(elapsedTime);
   }
   // Invalid hitched unit so clean everything up
   else
   {
      // Clean up the ref
      removeEntityRef(BEntityRef::cTypeHitchUnit, hitchedUnitID);                  

      // Reset the flag
      setFlagHasHitched(false);
   }
}

//==============================================================================
//==============================================================================
void BUnit::updateOpps(float elapsedTime)
{
   //Rip through and remove any completed opps.
   for (int i=0; i < mOpps.getNumber(); i++)
   {
      BUnitOpp* pOpp=mOpps[i];
      if (pOpp->getComplete())
      {
         completeOppForReal(pOpp);
         i--;
      }
   }   

   //Create some opps (maybe).
   if (mFlagAlive && !getFlagDoingFatality())
      createOpps();

   //-- Create an array where we can store the highest priority we're currently working on for each opp type.
   BUnitOppType oppPriorities[BUnitOpp::cNumberTypes];
   memset(oppPriorities, BUnitOpp::cPriorityNone, BUnitOpp::cNumberTypes*sizeof(BUnitOppType));

   // TRB 3/5/08
   // Attack opps were getting killed if processed during a reload.  This is a special
   // fix to prevent that.  May want to make a generic fix to allow attacks or other opps
   // to hang around if a higher priority opp is running even if not all controllers are being used.
   uint8 reloadPriority = BUnitOpp::cPriorityNone;

#ifndef BUILD_FINAL
   // Trigger timings
   setFlagIsTriggered(false);
#endif

   //First pass.
   for (int i=0; i < mOpps.getNumber(); i++)
   {
      BUnitOpp* pOpp=mOpps[i];

      // Don't process new attack opps yet if a reload is currently running because this will kill the attack opp.
      if ((pOpp->getType() == BUnitOpp::cTypeAttack) || (pOpp->getType() == BUnitOpp::cTypeSecondaryAttack))
      {
         if (!pOpp->getEvaluated() && (reloadPriority > pOpp->getPriority()))
            continue;
      }

      //-- Generic version of what's above
      //-- This allows us to queue up opps of the same type, so for example if you add an anim opp of a lesser pri
      //-- than the anim opp that's currently running, we'll just leave it in the list, and evaluate it once the current
      //-- anim opp to completes 
      if(!pOpp->getEvaluated() && oppPriorities[pOpp->getType()] >= pOpp->getPriority())
         continue;
   
      //We're evaluating this opp now.
      pOpp->setEvaluated(true);

      uint doResult=cDoOppFail;   
      switch (pOpp->getType())
      {
         case BUnitOpp::cTypeMove:                        
            doResult=doMove(pOpp);            
            break;
   
         case BUnitOpp::cTypeAttack:
         case BUnitOpp::cTypeSecondaryAttack:            
            doResult=doAttack(pOpp);
            break;
            
         case BUnitOpp::cTypeCapture:            
            doResult=doCapture(pOpp);
            break;

         case BUnitOpp::cTypeJoin:
            doResult=doJoin(pOpp);
            break;

         case BUnitOpp::cTypeMines:                       
            doResult=doMines(pOpp);
            break;
            
         case BUnitOpp::cTypeChangeMode:            
            doResult=doChangeMode(pOpp);
            break;

         case BUnitOpp::cTypeGarrison:            
            doResult=doGarrison(pOpp);
            break;

         case BUnitOpp::cTypeEvade:
         case BUnitOpp::cTypeCheer:
         case BUnitOpp::cTypeRetreat:
         case BUnitOpp::cTypeReload:
         case BUnitOpp::cTypeRepair:
         case BUnitOpp::cTypeHeal:
         case BUnitOpp::cTypeAnimation:
         case BUnitOpp::cTypeUnpack:
         case BUnitOpp::cTypeTransport:            
            doResult=doAnimation(pOpp);
            if(doResult == cDoOppInProgress)
            {
               oppPriorities[BUnitOpp::cTypeEvade]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeCheer]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeRetreat]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeReload]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeRepair]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeHeal]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeAnimation]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeUnpack]  = pOpp->getPriority();
               oppPriorities[BUnitOpp::cTypeTransport]  = pOpp->getPriority();
            }
            break;

         case BUnitOpp::cTypeUngarrison:                        
            doResult=doUngarrison(pOpp);
            break;

         case BUnitOpp::cTypeGather:
            doResult=doGather(pOpp);
            break;

         case BUnitOpp::cTypeDeath:
            doResult=doDeath(pOpp);
            break;

         case BUnitOpp::cTypeInfectDeath:
            doResult=doInfectDeath(pOpp);
            break;

         case BUnitOpp::cTypeHeroDeath:
            doResult = doHeroDeath(pOpp);
            break;

         case BUnitOpp::cTypeHitch:
            doResult = doHitch(pOpp);
            break;

         case BUnitOpp::cTypeUnhitch:            
            doResult = doUnhitch(pOpp);
            break;

         case BUnitOpp::cTypeThrown:
            doResult = doThrown(pOpp);
            break;

         case BUnitOpp::cTypeDetonate:
            doResult = doDetonate(pOpp);
            break;

         case BUnitOpp::cTypeJumpPull:
         case BUnitOpp::cTypeJumpAttack:
         case BUnitOpp::cTypeJumpGarrison:
         case BUnitOpp::cTypeJumpGather:
         case BUnitOpp::cTypeJump:            
            doResult = doJump(pOpp);
            break;

         case BUnitOpp::cTypePointBlankAttack:
            doResult = doPointBlankAttack(pOpp);
            break;

         // These are always persistent, so we should never get an opp for them
         case BUnitOpp::cTypeEnergyShield:
         case BUnitOpp::cTypeInfantryEnergyShield:
         default:
            BASSERT(0);
            break;
      }
      
      //Deal with instant completes/fails.
      if ((doResult != cDoOppInProgress) && pOpp->getAllowComplete())
      {
         //If we're supposed to notify the source, do that.
         BEntity* pSource=gWorld->getEntity(pOpp->getSource());
         if (pSource && pOpp->getNotifySource())
         {
            if (doResult == cDoOppFail)
               pSource->notify(BEntity::cEventOppComplete, mID, pOpp->getID(), false);
            else
               pSource->notify(BEntity::cEventOppComplete, mID, pOpp->getID(), true);
         }
         //Remove this opp.
         mOpps.removeIndex(i);
         //Remove our actions.
         if (pOpp->getRemoveActions())
            removeActionsForOpp(pOpp);
         BUnitOpp::releaseInstance(pOpp);
         i--;
         //Just continue because we haven't done anything to the controllers.
         continue;
      }

      //-- Save off the priority of this type
      if (doResult != cDoOppFail)
         oppPriorities[pOpp->getType()] = pOpp->getPriority();

      // Save off priority if this is a reload opp
      if ((pOpp->getType() == BUnitOpp::cTypeReload) && !pOpp->getComplete())
         reloadPriority = pOpp->getPriority();

      //DCPTODO: This isn't the most intelligent code, but it's good enough for now.
      //If we're down below the threshold priorities for our controllers and
      //they're taken, we're done.  Else, keep on chugging.
      bool done=true;
      for (uint j=0; j < BActionController::cNumberControllers; j++)
      {
         //If the controller isn't taken, we're not done.
         if (mControllers[j].getActionID() == cInvalidActionID)
         {
            done=false;
            break;
         }
         //If the controller is taken by a lower priority action, we're not done.
         uint controllerPriority=getOppPriority(mControllers[j].getOppID());
         if (controllerPriority < pOpp->getPriority())
         {
            done=false;
            break;
         }
      }

#ifndef BUILD_FINAL
      // Trigger timings
      if (pOpp->getEvaluated() && !pOpp->getComplete() && (pOpp->getPriority() == BUnitOpp::cPriorityTrigger))
      {
         setFlagIsTriggered(true);   
      }
#endif

      //If we're done, bail.
      if (done)
         break;
   }

   //Remove any opps that were only supposed to be here for one update or only
   //until evaluated.
   for (int i=0; i < mOpps.getNumber(); i++)
   {
      BUnitOpp* pOpp=mOpps[i];

      if (pOpp->getExistForOneUpdate() ||
         (pOpp->getExistUntilEvaluated() && pOpp->getEvaluated()) )
      {
         mOpps.removeIndex(i);
         //Remove our actions.
         if (pOpp->getRemoveActions())
            removeActionsForOpp(pOpp);
         BUnitOpp::releaseInstance(pOpp);
         i--;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::createOpps()
{
   // If we are a contained, attached, or hitched unit bail
   if (getFlagGarrisoned() || getFlagAttached() || getFlagHitched())
      return;

   BSquad* pSquad = getParentSquad();
   if (!pSquad)
      return;

   //Early out if we are not leashing (this only happens during wow-moments)
   if (pSquad->getFlagIgnoreLeash())
      return;
   // Early out if our squad is garrisoning, ungarrisoning, hitching, or unhitching because our squad position and leash position is being modified
   if (pSquad->getFlagIsUngarrisoning() || pSquad->getFlagIsGarrisoning() || pSquad->getFlagIsHitching() || pSquad->getFlagIsUnhitching())
      return;
   //Early out if I can't move.
   if (!canMove())
      return;
   // Early out if unit in a 1-unit physics movement squad.  For now, just check for physics control.
   // MPB TODO - We may eventually want leashed physics objects
   if (pSquad->isSquadAPhysicsVehicle())
      return;

   //See if we are doing some location-y thing for our squad (defined by the orient controller).
   bool doingSomethingForSquad=false;
   if (!isControllerFree(BActionController::cControllerOrient))
   {
      const BUnitOpp* pOpp=getOppByID(mControllers[BActionController::cControllerOrient].getOppID());
      if (pOpp &&
         ((pOpp->getSource() == pSquad->getID()) ||
         ((pOpp->getPriority() >= BUnitOpp::cPriorityCommand) && (pOpp->getPriority() < BUnitOpp::cPriorityLeash)) ))
      {
         doingSomethingForSquad=true;

         // Enable this if dodging still has a problem of moving guys too far from their original position.
         // Exception:  Attack actions optionally grab the orient controller, but it's usually safe to take it from it.
         // Dodging can move a unit outside of range which is ok.  If distance gets to be too far then leash it back.
         /*
         const BAction *pAction = findActionByID(mControllers[BActionController::cControllerOrient].getActionID());
         if (pAction && (pAction->getType() == BAction::cActionTypeUnitRangedAttack) && !hasPersistentMoveAction())
         {
            if (!pSquad->isChildLeashed(mID, gDatabase.getUnitLeashLength() * 2.0f))
               doingSomethingForSquad = false;
         }
         */
      }
   }

   //See if we have a leash opp already.  We assume we only have one.
//-- FIXING PREFIX BUG ID 4334
   const BUnitOpp* pLeashOpp = NULL;
//--
   uint numOpps = mOpps.getSize();
   for (uint i = 0; i < numOpps; i++)
   {
      if (mOpps[i]->getLeash())
      {
         pLeashOpp=mOpps[i];
         break;
      }
   }

   //See how long we haven't been doing something for the squad.  We need some lag time in
   //here to avoid one-update ping-ponging, etc.
   DWORD leashTimeout=0;
   if (doingSomethingForSquad)
      mLeashTimer=gWorld->getGametime();
   else
      leashTimeout=gWorld->getGametime()-mLeashTimer;

   //Check leash.
   if (!doingSomethingForSquad && !pLeashOpp && (leashTimeout > 1000))
   {
      BVector leashLocation;
      if (!pSquad->isChildLeashed(mID, 0.0f, leashLocation))
      {
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         pNewOpp->init();
         pNewOpp->setSource(mID);
         BSimTarget leashTarget(leashLocation, 0.0f);
         pNewOpp->setTarget(leashTarget);
         pNewOpp->setType(BUnitOpp::cTypeMove);
         pNewOpp->setLeash(true);
         pNewOpp->setNotifySource(false);
         pNewOpp->generateID();
         addOpp(pNewOpp);
      }
   }
   //If we have a leash opp and we are moving for an order, kill the leash opp.  Don't kill force leash
   //opps.
   if (pLeashOpp && doingSomethingForSquad && !pLeashOpp->getForceLeash())
      removeOpp(pLeashOpp->getID());
}

//==============================================================================
//==============================================================================
bool BUnit::doIdle(void)
{
   BASSERT(mFlagAlive);

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doIdle mID", mID.asLong());
   #endif
   if (!isAlive())
      return (false);

   BAction* pAction=gActionManager.createAction(BAction::cActionTypeEntityIdle);
   if (!pAction)
      return(false);
   addAction(pAction);
   return(true);
}

//==============================================================================
//==============================================================================
uint BUnit::doMove(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   const BSquad* pSquad = getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return cDoOppFail;

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doMove mID", mID.asLong());
   #endif

   // Temporary immobility
   if (pSquad->getFlagNonMobile() && !getFlagNonMobile())
      return cDoOppInProgress;

   //Fail if we can't move.
   if (!canMove())
      return (cDoOppFail);

   //If this move is from our parent squad, it's a "Squad Move".
   bool squadMove = false;
   if (pOpp->getSource() == mParentID)
      squadMove = true;      

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
      return (cDoOppFail);

   //If we have the right action, update it.
   BActionType actionType=getMoveActionType();
   BUnitActionMove* pAction = reinterpret_cast<BUnitActionMove*>(mActions.getActionByType(actionType));
   if (pAction)
   {
      // Squad moves are higher priority than unit moves, so fail if trying to reuse a squad move
      // for a non-squad move
      if ((pAction->getOppID() != pOpp->getID()) && pAction->getFlagSquadMove() && !squadMove)
         return cDoOppFail;

      pAction->setTarget(target);
      pAction->setOppID(pOpp->getID());
      pAction->setFlagSquadMove(squadMove);
      return (cDoOppInProgress);
   }

   // Make sure unit isn't already in range.  This will prevent recreating the move action
   // again if the unit has already reached the target but the opportunity is still around.
   if (squadMove)
   {
      BSquad* pSquad = getParentSquad();
      if (pSquad != NULL)
      {
         BVector targetPos;
         #ifdef _MOVE4
         // In Move4, check to see if our squad is still moving, and if not, then
         // see if we're already where the squad wants us to be. 
         BSquadActionMove *pMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
         if (!pMoveAction)
            return cDoOppComplete;
         if (pMoveAction->getState() != BAction::cStateWorking)
         {
            if (pSquad->getDesiredChildLocation(getID(), targetPos))
            {
               float distanceRemaining = calcDistObstructionToPoint(getPosition(), targetPos);
               if (distanceRemaining <= cFloatCompareEpsilon)
                  return (cDoOppComplete);
            }
         }
         #else
         float timeNeeded;
         if (pSquad->getFutureDesiredChildLocation(getID(), 1000.0f, targetPos, timeNeeded))
         {
#ifdef SYNC_UnitAction
         syncUnitActionData("BUnit::doMove targetPos", targetPos);
#endif
            float distanceRemaining = calcDistObstructionToPoint(getPosition(), targetPos);
            if (distanceRemaining <= cFloatCompareEpsilon)
               return (cDoOppComplete);
         }
         #endif
      }
   }
   else if (target.isRangeValid())
   {
      if (target.getID().isValid())
      {
//-- FIXING PREFIX BUG ID 4336
         const BEntity* pTarget = gWorld->getEntity(target.getID());
//--
         if (pTarget)
         {
            float distanceToTarget = calcDistRadiusToObstruction(getPosition(), pTarget);
            if (distanceToTarget <= target.getRange())
               return (cDoOppComplete);
         }
      }
      else if (target.isPositionValid())
      {
         float distanceToTarget = getPosition().xzDistance(target.getPosition());
         if (distanceToTarget <= target.getRange())
            return (cDoOppComplete);
      }
   }

   // Create the action
   pAction=reinterpret_cast<BUnitActionMove*>(gActionManager.createAction(actionType));
   pAction->setTarget(target);
   pAction->setOppID(pOpp->getID());
   pAction->setFlagSquadMove(squadMove);
   if (!addAction(pAction, NULL))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doAttack(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doAttack mID", mID.asLong());
      syncUnitActionData("BUnit::doAttack targetID", pOpp->getTarget().getID().asLong());
   #endif
   #ifdef DEBUGOPPS
   if (mPlayerID == 1)
      debug("DoAttack: OppID=%d, Target=%d", pOpp->getID(), pOpp->getTarget().getID());
   #endif

//-- FIXING PREFIX BUG ID 4339
   const BUnitActionRangedAttack* pAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(getActionByType(BAction::cActionTypeUnitRangedAttack));
//--
   if (pAttackAction)
   {
      //If we have an attack action with this opp ID, skip.
      if((pAttackAction->getOppID() == pOpp->getID()))
      {
         #ifdef SYNC_UnitAction
            syncUnitActionCode("BUnit::doAttack opp in progress");
         #endif
         return (cDoOppInProgress);
      }

   }

   //-- If we've decided to do a secondary attack, wait for 1 updates.
   //-- This gives time for another primary attack opp to show up before we jump
   //-- right on the first secondary attack that just happens to be lying around.
   if(pOpp->getType() == BUnitOpp::cTypeSecondaryAttack)
   {  
      // SLB: Make sure lower priority secondary attacks don't bump our current attack action.
      if (pAttackAction && (pAttackAction->getPriority() >= pOpp->getPriority()))
      {
         return (cDoOppInProgress);
      }

      if(pOpp->getWaitCount() < 1)
      {
         pOpp->setWaitCount(pOpp->getWaitCount() + 1);
         return (cDoOppInProgress);         
      }
      else
         pOpp->setWaitCount(0);      
   }

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
   {
      #ifdef SYNC_UnitAction
         syncUnitActionCode("BUnit::doAttack determineTarget");
      #endif
      return (cDoOppFail);
   }
//-- FIXING PREFIX BUG ID 4340
   const BUnit* pTarget=gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
   {
      #ifdef SYNC_UnitAction
         syncUnitActionCode("BUnit::doAttack validate target");
      #endif
      return (cDoOppFail);
   }

   //If don't have a valid action, this fails.
   BTactic* pTactic=getTactic();
   if (!pTactic)
   {
      #ifdef SYNC_UnitAction
         syncUnitActionCode("BUnit::doAttack tactic");
      #endif
      return (cDoOppFail);
   }
//-- FIXING PREFIX BUG ID 4341
   const BProtoAction* pProtoAction=NULL;
//--
   if (pTarget)
      pProtoAction=pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitRangedAttack);
   else
      pProtoAction=pTactic->getProtoAction(mTacticState, NULL, target.getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitRangedAttack);
   if (!pProtoAction)
   {
      #ifdef SYNC_UnitAction
         syncUnitActionCode("BUnit::doAttack proto action");
      #endif
      return (cDoOppFail);
   }
   BASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitRangedAttack);

   //If we have an action of the right type with the right target, make sure
   //its OppID gets updated.
   BAction* pAction=getActionByType(pProtoAction->getActionType());
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      #ifdef SYNC_UnitAction
         syncUnitActionCode("BUnit::doAttack action already exists for target");
      #endif
      return (cDoOppInProgress);
   }

   // Adjust height of SimTarget if protoAction targets a location in the air
   if (!pTarget && pProtoAction->targetsAir())
      return (cDoOppFail);

   //Else, create the action.
   pAction=gActionManager.createAction(pProtoAction->getActionType());
   pAction->setProtoAction(pProtoAction);
   pAction->setTarget(target);
   pAction->setHitZoneIndex(-1);
   pAction->setOppID(pOpp->getID());
   if (!addAction(pAction))
   {
      #ifdef SYNC_UnitAction
         syncUnitActionCode("BUnit::doAttack addAction");
      #endif
      return (cDoOppFail);
   }
   #ifdef SYNC_UnitAction
      syncUnitActionCode("BUnit::doAttack success");
   #endif
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doCapture(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doCapture mID", mID.asLong());
      syncUnitActionData("BUnit::doCapture targetID", pOpp->getTarget().getID().asLong());
   #endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4342
   const BUnit* pTarget=gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
      return (cDoOppFail);

   //If don't have a valid action, this fails.
   BTactic* pTactic=getTactic();
   if (!pTactic)
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4343
   const BProtoAction* pProtoAction=NULL;
//--
   if (pTarget)
      pProtoAction=pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(),
         getPlayer(), getPosition(), getID(), -1, false, BAction::cActionTypeUnitCapture);
   else
      pProtoAction=pTactic->getProtoAction(mTacticState, NULL, target.getPosition(),
         getPlayer(), getPosition(), getID(), -1, false, BAction::cActionTypeUnitCapture);
   if (!pProtoAction)
      return (cDoOppFail);
   BASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitCapture);

   //If we have an action of the right type with the right target, make sure
   //its OppID gets updated.
   BAction* pAction=getActionByType(pProtoAction->getActionType());
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      return (cDoOppInProgress);
   }

   //Else, create the action.
   pAction=gActionManager.createAction(pProtoAction->getActionType());
   pAction->setProtoAction(pProtoAction);
   pAction->setTarget(target);
   pAction->setOppID(pOpp->getID());
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doJoin(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::doJoin mID", mID.asLong());
   syncUnitActionData("BUnit::doJoin targetID", pOpp->getTarget().getID().asLong());
#endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4344
   const BSquad* pTarget=gWorld->getSquad(target.getID());
//--
   if (!pTarget)
      return (cDoOppFail);
   int abilityID = (target.isAbilityIDValid() ? target.getAbilityID() : -1);

   //If don't have a valid action, this fails.
   BTactic* pTactic=getTactic();
   if (!pTactic)
      return (cDoOppFail);

   BUnitActionJoin* pAction=reinterpret_cast<BUnitActionJoin*>(getActionByType(BAction::cActionTypeUnitJoin));
   if (pAction)
   {
      //If we have an action of the right type with the right target, make sure
      //its OppID gets updated.
      if ((pAction->getTarget()->getID() == target.getID()))
      {
         pAction->setOppID(pOpp->getID());
         return (cDoOppInProgress);
      }

      // If the target has a join action and its target is the same as our target, we're good
      BSquad* pTarget = gWorld->getSquad(target.getID());
      if (pTarget && pTarget->getLeaderUnit())
      {
         BUnitActionJoin* pTargetAction=reinterpret_cast<BUnitActionJoin*>(pTarget->getLeaderUnit()->getActionByType(BAction::cActionTypeUnitJoin));
         if (pTargetAction && pTargetAction->getTarget()->getID() == pAction->getTarget()->getID())
            return (cDoOppInProgress);
      }
   }

//-- FIXING PREFIX BUG ID 4345
   const BProtoAction* pProtoAction=pTactic->getProtoAction(mTacticState, pTarget->getLeaderUnit(), pTarget->getPosition(), getPlayer(), getPosition(), getID(), abilityID, false, BAction::cActionTypeUnitJoin);
//--
   if (!pProtoAction)
      return (cDoOppFail);
   BASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitJoin);

   //Else, create the action.
   pAction=reinterpret_cast<BUnitActionJoin*>(gActionManager.createAction(pProtoAction->getActionType()));
   pAction->setProtoAction(pProtoAction);
   pAction->setTarget(target);
   pAction->setOppID(pOpp->getID());
   
   if (pOpp->getUserData() == 1)
   {
      pAction->setAllowMultiple(true);
   }

   if (!addAction(pAction))
      return (cDoOppFail);
   // Opp can go away (but not the action) after it has started for board or merge types
   if ((pProtoAction->getJoinType() == BUnitActionJoin::cJoinTypeBoard) ||
       (pProtoAction->getJoinType() == BUnitActionJoin::cJoinTypeMerge))
   {
      pOpp->setExistForOneUpdate(true);
      pOpp->setRemoveActions(false);
   }
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doMines(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doMines mID", mID.asLong());
      syncUnitActionData("BUnit::doMines targetID", pOpp->getTarget().getID().asLong());
   #endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4346
   const BUnit* pTarget=gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
      return (cDoOppFail);

   //If don't have a valid action, this fails.
   BTactic* pTactic=getTactic();
   if (!pTactic)
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4347
   const BProtoAction* pProtoAction=NULL;
//--
   if (pTarget)
      pProtoAction=pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitMines);
   else
      pProtoAction=pTactic->getProtoAction(mTacticState, NULL, target.getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitMines);
   if (!pProtoAction)
      return (cDoOppFail);
   BASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitMines);

   //If we have an action of the right type with the right target, make sure
   //its OppID gets updated.
   BAction* pAction=getActionByType(pProtoAction->getActionType());
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      return (cDoOppInProgress);
   }

   //Else, create the action.
   pAction=gActionManager.createAction(pProtoAction->getActionType());
   pAction->setProtoAction(pProtoAction);
   pAction->setTarget(target);
   pAction->setOppID(pOpp->getID());
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doChangeMode(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::doChangeMode mID", mID.asLong());
#endif

   uint8 squadMode = pOpp->getUserData();

   // If we are a unit like the Elephant that can show a circle menu while locked down,
   // cancel all train/research/etc commands when switching out of lockdown mode.
   if (squadMode != BSquadAI::cModeLockdown && getProtoObject()->getFlagLockdownMenu())
   {
//-- FIXING PREFIX BUG ID 4348
      const BSquad* pSquad = getParentSquad();
//--
      if (pSquad && pSquad->getSquadMode() == BSquadAI::cModeLockdown)
      {
         BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
         if (pAction)
            pAction->clearQueue();
      }
   }

   //If we have an action of the right type with the right target, make sure
   //its OppID gets updated.
   BUnitActionChangeMode* pAction = reinterpret_cast<BUnitActionChangeMode*>(mActions.getActionByType(BAction::cActionTypeUnitChangeMode));
   if (pAction)
   {
      pAction->setOppID(pOpp->getID());
      pAction->setSquadMode(squadMode);
      return (cDoOppInProgress);
   }

   //Else, create the action.
   pAction=reinterpret_cast<BUnitActionChangeMode*>(gActionManager.createAction(BAction::cActionTypeUnitChangeMode));
   pAction->setSquadMode(squadMode);
   pAction->setOppID(pOpp->getID());
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doGarrison(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doGarrison mID", mID.asLong());
      syncUnitActionData("BUnit::doGarrison targetID", pOpp->getTarget().getID().asLong());
   #endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
   {
      return (cDoOppFail);
   }

//-- FIXING PREFIX BUG ID 4349
   const BUnit* pTarget = gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
   {
      return (cDoOppFail);
   }

   // If don't have a valid action, this fails.
   BTactic* pTactic = getTactic();
   if (!pTactic)
   {
      return (cDoOppFail);
   }

//-- FIXING PREFIX BUG ID 4350
   const BProtoAction* pProtoAction = NULL;
//--
   if (pTarget)
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(), getPlayer(), getPosition(), getID(), -1, false);
   }
   else
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, NULL, target.getPosition(), getPlayer(), getPosition(), getID(), -1, false);
   }
   // Halwes - 6/19/2007 - It feels like this is correct, but many units are not getting an action returned that I know garrison.
   //if (!pProtoAction)
   //{
   //   return (false);
   //}

   // If we have an action of the right type with the right target, make sure
   // its OppID gets updated.
   BAction* pAction = getActionByType(BAction::cActionTypeUnitGarrison);
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      return (cDoOppInProgress);
   }

   // Else, create the action.
   pAction = gActionManager.createAction(BAction::cActionTypeUnitGarrison);      
   pAction->setOppID(pOpp->getID());      
   pAction->setTarget(target);
   BUnitActionGarrison* pUAG = reinterpret_cast<BUnitActionGarrison*>(pAction);
   if (pOpp->getUserDataSet())
      pUAG->setIgnoreRange(pOpp->getUserData() != 0);

   // If valid proto action
   if (pProtoAction && (pProtoAction->getActionType() == BAction::cActionTypeUnitGarrison))
   {
      pAction->setProtoAction(pProtoAction);         
   }   
   else
   {      
      pAction->setProtoAction(NULL);
   }
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doUngarrison(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doUngarrison mID", mID.asLong());
      syncUnitActionData("BUnit::doUngarrison targetID", pOpp->getTarget().getID().asLong());
   #endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
   {
      return (cDoOppFail);
   }

//-- FIXING PREFIX BUG ID 4351
   const BUnit* pTarget = gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
   {
      return (cDoOppFail);
   }

   // If don't have a valid action, this fails.
   BTactic* pTactic = getTactic();
   if (!pTactic)
   {
      return (cDoOppFail);
   }
   
//-- FIXING PREFIX BUG ID 4352
   const BProtoAction* pProtoAction = NULL;
//--
   if (pTarget)
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(), getPlayer(), getPosition(), getID(), gDatabase.getAIDUngarrison(), false);
   }
   else
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, NULL, target.getPosition(), getPlayer(), getPosition(), getID(), gDatabase.getAIDUngarrison(), false);
   }   

   // If we have an action of the right type with the right target, make sure
   // its OppID gets updated.      
   BUnitActionUngarrison* pAction = reinterpret_cast<BUnitActionUngarrison*>(mActions.getActionByType(BAction::cActionTypeUnitUngarrison));   
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      pAction->setExitDirection(pOpp->getUserData());
      if (pOpp->getPath().getNumber() > 0)
         pAction->setSpawnPoint(pOpp->getPath()[0]);
      return (cDoOppInProgress);
   }

   // Else, create the action.
   pAction = reinterpret_cast<BUnitActionUngarrison*>(gActionManager.createAction(BAction::cActionTypeUnitUngarrison));
   pAction->setTarget(target);
   pAction->setOppID(pOpp->getID());      
   pAction->setExitDirection(pOpp->getUserData());
   if (pOpp->getPath().getNumber() > 0)
      pAction->setSpawnPoint(pOpp->getPath()[0]);
   pAction->setIgnoreSpawnPoint(pOpp->getUserData2() != 0);
   pAction->setSourceID(pOpp->getSource());

   // If valid proto action
   if (pProtoAction && (pProtoAction->getActionType() == BAction::cActionTypeUnitUngarrison))
   {
      pAction->setProtoAction(pProtoAction);         
   }   
   else
   {      
      pAction->setProtoAction(NULL);
   }
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
// Instantiate and add the unit hitch action
//==============================================================================
uint BUnit::doHitch(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doHitch mID", mID.asLong());
      syncUnitActionData("BUnit::doHitch targetID", pOpp->getTarget().getID().asLong());
   #endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
   {
      return (cDoOppFail);
   }

//-- FIXING PREFIX BUG ID 4354
   const BUnit* pTarget = gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
   {
      return (cDoOppFail);
   }

   // If don't have a valid action, this fails.
   BTactic* pTactic = getTactic();
   if (!pTactic)
   {
      return (cDoOppFail);
   }

//-- FIXING PREFIX BUG ID 4355
   const BProtoAction* pProtoAction = NULL;
//--
   if (pTarget)
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(), getPlayer(), getPosition(), getID(), -1, false);
   }
   else
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, NULL, target.getPosition(), getPlayer(), getPosition(), getID(), -1, false);
   }
   if (!pProtoAction)
   {
      return (false);
   }

   // If we have an action of the right type with the right target, make sure
   // its OppID gets updated.
   BAction* pAction = getActionByType(BAction::cActionTypeUnitHitch);
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      return (cDoOppInProgress);
   }

   // Else, create the action.
   pAction = gActionManager.createAction(BAction::cActionTypeUnitHitch);      
   pAction->setOppID(pOpp->getID());      
   pAction->setTarget(target);

   // If valid proto action
   if (pProtoAction && (pProtoAction->getActionType() == BAction::cActionTypeUnitHitch))
   {
      pAction->setProtoAction(pProtoAction);         
   }   
   else
   {      
      pAction->setProtoAction(NULL);
   }
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doUnhitch(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doUnhitch mID", mID.asLong());
      syncUnitActionData("BUnit::doUnhitch targetID", pOpp->getTarget().getID().asLong());
   #endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
   {
      return (cDoOppFail);
   }

//-- FIXING PREFIX BUG ID 4357
   const BUnit* pTarget = gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
   {
      return (cDoOppFail);
   }

   // If don't have a valid action, this fails.
   BTactic* pTactic = getTactic();
   if (!pTactic)
   {
      return (cDoOppFail);
   }

//-- FIXING PREFIX BUG ID 4358
   const BProtoAction* pProtoAction = NULL;
//--
   if (pTarget)
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(), getPlayer(), getPosition(), getID(), gDatabase.getAIDUnhitch(), false);
   }
   else
   {
      pProtoAction = pTactic->getProtoAction(mTacticState, NULL, target.getPosition(), getPlayer(), getPosition(), getID(), gDatabase.getAIDUnhitch(), false);
   }
   if (!pProtoAction)
   {
      return (cDoOppFail);
   }

   // If we have an action of the right type with the right target, make sure
   // its OppID gets updated.
   BAction* pAction = getActionByType(BAction::cActionTypeUnitUnhitch);
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      return (cDoOppInProgress);
   }

   // Else, create the action.
   pAction = gActionManager.createAction(BAction::cActionTypeUnitUnhitch);      
   pAction->setOppID(pOpp->getID());      
   pAction->setTarget(target);

   // If valid proto action
   if (pProtoAction && (pProtoAction->getActionType() == BAction::cActionTypeUnitUnhitch))
   {
      pAction->setProtoAction(pProtoAction);         
   }   
   else
   {      
      pAction->setProtoAction(NULL);
   }
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doAnimation(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doAnimation mID", mID.asLong());
   #endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
      return (cDoOppFail);

   //If we have this action already doing this opp, then skip.  It doesn't make
   //sense to update anything in this type of action.
   BUnitActionPlayBlockingAnimation* pAction = reinterpret_cast<BUnitActionPlayBlockingAnimation*>(mActions.getActionByType(BAction::cActionTypeUnitPlayBlockingAnimation));
   if (pAction)
   {
      if (pAction->getOppID() == pOpp->getID())
         return (cDoOppInProgress);
      else
      {
         if (getOppPriority(pAction->getOppID()) >= pOpp->getPriority())
         {
            // This can happen if a new opp of higher priority is added and it takes over the
            // action from the old opp.  The old opp should return fail and go away.
            return cDoOppFail;
         }
      }
   }

   //Else, create the action.
   pAction=reinterpret_cast<BUnitActionPlayBlockingAnimation*>(gActionManager.createAction(BAction::cActionTypeUnitPlayBlockingAnimation));
   pAction->setAnimationState(BObjectAnimationState::cAnimationStateMisc, pOpp->getUserData(), true);
   pAction->setTarget(target);

   if(pOpp->getPreserveDPS())
      pAction->setFlagPreserveDPS(true);

   if (pOpp->getType() == BUnitOpp::cTypeRetreat)
      pAction->setFlagTargetInvert(true);
   if (pOpp->getType() == BUnitOpp::cTypeRepair || pOpp->getType() == BUnitOpp::cTypeHeal)
      pAction->setFlagLoop(true);
   else
      pAction->setFlagLoop(false);
   if ((pOpp->getType() == BUnitOpp::cTypeReload) || (pOpp->getType() == BUnitOpp::cTypeEvade) || (pOpp->getType() == BUnitOpp::cTypeRetreat))
      pAction->setFlagAllowMove(true);
   pAction->setOppID(pOpp->getID());
   if (!addAction(pAction))
      return (cDoOppFail);

   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doGather(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::doGather mID", mID.asLong());
   syncUnitActionData("BUnit::doGather targetID", pOpp->getTarget().getID().asLong());
#endif

   //Figure our actual target.
   BSimTarget target;
   if (!determineTarget(pOpp, target))
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4359
   const BUnit* pTarget=gWorld->getUnit(target.getID());
//--
   if (!pTarget && !target.isPositionValid())
      return (cDoOppFail);

   //If don't have a valid action, this fails.
   BTactic* pTactic=getTactic();
   if (!pTactic)
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4360
   const BProtoAction* pProtoAction=NULL;
//--
   if (pTarget)
      pProtoAction=pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(),
         getPlayer(), getPosition(), getID(), -1, false, BAction::cActionTypeUnitGather);
   else
      pProtoAction=pTactic->getProtoAction(mTacticState, NULL, target.getPosition(),
         getPlayer(), getPosition(), getID(), -1, false, BAction::cActionTypeUnitGather);
   if (!pProtoAction)
      return (cDoOppFail);
   BASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitGather);

   //If we have an action of the right type with the right target, make sure
   //its OppID gets updated.
   BAction* pAction=getActionByType(pProtoAction->getActionType());
   if (pAction && (*(pAction->getTarget()) == target))
   {
      pAction->setOppID(pOpp->getID());
      return (cDoOppInProgress);
   }

   //Else, create the action.
   pAction=gActionManager.createAction(pProtoAction->getActionType());
   pAction->setProtoAction(pProtoAction);
   pAction->setTarget(target);
   pAction->setOppID(pOpp->getID());
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doDeath(BUnitOpp* pOpp)
{
#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::doDeath mID", mID.asLong());
#endif

//-- FIXING PREFIX BUG ID 4361
   const BAction* pAction=getActionByType(BAction::cActionTypeUnitDeath);
//--
   if (pAction && (pAction->getOppID() == pOpp->getID()))
      return (cDoOppInProgress);

   //-- Add Unit Death Action      
   BUnitActionDeath* pDeathAction = reinterpret_cast<BUnitActionDeath*>(gActionManager.createAction(BAction::cActionTypeUnitDeath));
   BASSERT(pDeathAction);
   pDeathAction->setKillingEntity(mKilledByID);
   pDeathAction->setKillingWeaponType(mKilledByWeaponType);
   pDeathAction->setOppID(pOpp->getID());
   pDeathAction->setFlagDoingFatality((pOpp->getUserData() != 0));
   if (!addAction(pDeathAction)) //-- Destroy will be called when the death action ends
   {
      BFAIL("Couldn't add death action");
      return (cDoOppFail);
   }
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doHeroDeath(BUnitOpp* pOpp)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doHeroDeath mID", mID.asLong());
   #endif

//-- FIXING PREFIX BUG ID 4515
      const BAction* pAction = getActionByType(BAction::cActionTypeUnitHeroDeath);
//--
      if (pAction && (pAction->getOppID() == pOpp->getID()))
         return (cDoOppInProgress);

      //-- Add Unit Hero Death Action      
      BUnitActionHeroDeath* pHeroDeathAction = reinterpret_cast<BUnitActionHeroDeath*>(gActionManager.createAction(BAction::cActionTypeUnitHeroDeath));
      BASSERT(pHeroDeathAction);
      pHeroDeathAction->setKillingEntity(mKilledByID);
      pHeroDeathAction->setKillingWeaponType(mKilledByWeaponType);
      pHeroDeathAction->setOppID(pOpp->getID());
      //pHeroDeathAction->setFlagDoingFatality((pOpp->getUserData() != 0));
      if (!addAction(pHeroDeathAction)) //-- Destroy will be called when the death action ends
      {
         BFAIL("Couldn't add hero death action");
         return (cDoOppFail);
      }

      return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doInfectDeath(BUnitOpp* pOpp)
{
#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::doDeath mID", mID.asLong());
#endif

   if (mFlagTakeInfectionForm)
      return (cDoOppFail);

//-- FIXING PREFIX BUG ID 4516
   const BAction* pAction=getActionByType(BAction::cActionTypeUnitInfectDeath);
//--
   if (pAction && (pAction->getOppID() == pOpp->getID()))
      return (cDoOppInProgress);

   //-- Add Unit Infect Death Action      
   BUnitActionInfectDeath* pDeathAction = reinterpret_cast<BUnitActionInfectDeath*>(gActionManager.createAction(BAction::cActionTypeUnitInfectDeath));
   BASSERT(pDeathAction);
   pDeathAction->setOppID(pOpp->getID());
   pDeathAction->setInfectionPlayerID(pOpp->getUserData());

   // [4/18/2008 CJS] Make sure we have a parent squad set
   if (mFormerParentSquad == cInvalidObjectID)
      mFormerParentSquad = getParentID();

   pDeathAction->setFormerSquad(mFormerParentSquad);

   pDeathAction->setSkipDeathAnim(false);
   if (!addAction(pDeathAction)) //-- Destroy will be called when the death action ends
   {
      // BSR [8/29/08]: Bypassing BFAIL() here since this is the expected behavior if the player resigns when a squad is in the infection transition.
//      BFAIL("Couldn't add death action");
      return (cDoOppFail);
   }
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doThrown(BUnitOpp* pOpp)
{
   BASSERT(mFlagAlive);
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doThrown mID", mID.asLong());
   #endif

   //If we have this action already doing this opp, then skip.  It doesn't make
   //sense to update anything in this type of action.
   BUnitActionThrown* pAction = reinterpret_cast<BUnitActionThrown*>(mActions.getActionByType(BAction::cActionTypeUnitThrown));
   if (pAction && (pAction->getOppID() == pOpp->getID()))
      return (cDoOppInProgress);

   //Else, create the action.
   pAction=reinterpret_cast<BUnitActionThrown*>(gActionManager.createAction(BAction::cActionTypeUnitThrown));
   /*
   pAction->setAnimationState(BObjectAnimationState::cAnimationStateMisc, pOpp->getUserData(), true);
   pAction->setTarget(target);
   if (pOpp->getType() == BUnitOpp::cTypeRetreat)
      pAction->setFlagTargetInvert(true);
   if (pOpp->getType() == BUnitOpp::cTypeRepair)
      pAction->setFlagLoop(true);
   if (pOpp->getType() == BUnitOpp::cTypeReload)
      pAction->setFlagAllowMove(true);
   */
   pAction->setOppID(pOpp->getID());
   pAction->setThrowerID(pOpp->getTarget().getID());
   if(pOpp->getUserDataSet())
      pAction->setThrowerProtoActionID(pOpp->getUserData());
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doDetonate(BUnitOpp* pOpp)
{
   BUnitActionDetonate* pAction = reinterpret_cast<BUnitActionDetonate*>(mActions.getActionByType(BAction::cActionTypeUnitDetonate));
   if (pAction && (pAction->getOppID() == pOpp->getID()))
      return (cDoOppInProgress);

   // If we're set to ignore target, then don't check target
   BSimTarget target;
//-- FIXING PREFIX BUG ID 4517
   const BUnit* pTarget = NULL;
//--
   if (pOpp->getUserData2() != 1)
   {
      //Figure our actual target.
      if (!determineTarget(pOpp, target))
         return (cDoOppFail);
      pTarget=gWorld->getUnit(target.getID());
      if (!pTarget && !target.isPositionValid())
         return (cDoOppFail);
   }

   //If don't have a valid action, this fails.
   BTactic* pTactic=getTactic();
   if (!pTactic)
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4518
   const BProtoAction* pProtoAction=NULL;
//--
   if (pTarget)
      pProtoAction=pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitDetonate);
   else
      pProtoAction=pTactic->getProtoAction(mTacticState, NULL, target.getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitDetonate);
   if (!pProtoAction)
      return (cDoOppFail);
   BASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitDetonate);

   pAction = reinterpret_cast<BUnitActionDetonate*>(gActionManager.createAction(BAction::cActionTypeUnitDetonate));

   if (pOpp->getUserData() == 0)
      pAction->setImmediateTrigger();
   else
      pAction->setDeathTrigger();
   //BWeapon* pWeapon = (BWeapon*)pTactic->getWeapon(pProtoAction->getWeaponID());
   //pAction->setProximityTrigger(pWeapon->mAOERadius);

   pAction->setOppID(pOpp->getID());
   pAction->setProtoAction(pProtoAction);
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
uint BUnit::doJump(BUnitOpp* pOpp, BAction* pParentAction /* = NULL */)
{
   if (pOpp->getType() == BUnitOpp::cTypeJumpPull)
   {
      BASSERT(mFlagAlive);
      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnit::pullUnit mID", mID.asLong());
      #endif

      //If we have this action already doing this opp, then skip.  It doesn't make
      //sense to update anything in this type of action.
      BUnitActionJump* pAction = reinterpret_cast<BUnitActionJump*>(mActions.getActionByType(BAction::cActionTypeUnitJump));
      if (pAction)
         return (cDoOppInProgress);

      //Else, create the action.
      pAction=reinterpret_cast<BUnitActionJump*>(gActionManager.createAction(BAction::cActionTypeUnitJump));
      pAction->setParentAction(pParentAction);
      pAction->setTarget(pOpp->getTarget());
      pAction->setOppID(pOpp->getID());
      pAction->setNoOrient();
      if(pOpp->getUserDataSet())
         pAction->setThrowerProtoActionID(pOpp->getUserData());
      pAction->setJumpType(BUnitOpp::cTypeJumpPull);
      if (!addAction(pAction)) 
         return (cDoOppFail);
      return (cDoOppInProgress);
   }
   else
   {
      BUnitActionJump* pAction = reinterpret_cast<BUnitActionJump*>(mActions.getActionByType(BAction::cActionTypeUnitJump));
      if (pAction && (pAction->getOppID() == pOpp->getID()))
         return (cDoOppInProgress);

      //Figure our actual target.
      BSimTarget target;
      if (!determineTarget(pOpp, target))
         return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4519
      const BUnit* pTarget=gWorld->getUnit(target.getID());
//--
      if (!pTarget && !target.isPositionValid())
         return (cDoOppFail);

      //If don't have a valid action, this fails.
      BTactic* pTactic=getTactic();
      if (!pTactic)
         return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4520
      const BProtoAction* pProtoAction=NULL;
//--
      BUnitOppType protoActionType = BAction::cActionTypeUnitJump;
      if (pOpp->getType() == BUnitOpp::cTypeJumpGather)
         protoActionType = BAction::cActionTypeUnitJumpGather;
      else if (pOpp->getType() == BUnitOpp::cTypeJumpGarrison)
         protoActionType = BAction::cActionTypeUnitJumpGarrison;
      else if (pOpp->getType() == BUnitOpp::cTypeJumpAttack)
         protoActionType = BAction::cActionTypeUnitJumpAttack;

      if (pTarget)
         pProtoAction=pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(),
            getPlayer(), getPosition(), getID(), target.getAbilityID(), false, protoActionType);
      else
         pProtoAction=pTactic->getProtoAction(mTacticState, NULL, target.getPosition(),
            getPlayer(), getPosition(), getID(), target.getAbilityID(), false, protoActionType);
      if (!pProtoAction)
         return (cDoOppFail);
      BASSERT(pProtoAction->getActionType() == protoActionType);

      pAction = reinterpret_cast<BUnitActionJump*>(gActionManager.createAction(BAction::cActionTypeUnitJump));

      pAction->setParentAction(pParentAction);
      pAction->setTarget(pOpp->getTarget());
      pAction->setOppID(pOpp->getID());
      pAction->setProtoAction(pProtoAction);
      pAction->setJumpType(pOpp->getType());

      if (!addAction(pAction))
         return (cDoOppFail);
      return (cDoOppInProgress);
   }
}

//==============================================================================
//==============================================================================
uint BUnit::doPointBlankAttack(BUnitOpp* pOpp)
{
   //BUnitActionPointBlankAttack* pAction = reinterpret_cast<BUnitActionPointBlankAttack*>(mActions.getActionByType(BAction::cActionTypeUnitPointBlankAttack));
   //getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitPointBlankAttack);

   BUnitActionPointBlankAttack* pAction = reinterpret_cast<BUnitActionPointBlankAttack*>(mActions.getActionByType(BAction::cActionTypeUnitPointBlankAttack));
   if (pAction && (pAction->getOppID() == pOpp->getID()))
      return (cDoOppInProgress);

   BSimTarget target;
//-- FIXING PREFIX BUG ID 4521
   const BUnit* pTarget = NULL;
//--
   //Figure our actual target.
   if (!determineTarget(pOpp, target))
      return (cDoOppFail);

   //If don't have a valid action, this fails.
   BTactic* pTactic=getTactic();
   if (!pTactic)
      return (cDoOppFail);
//-- FIXING PREFIX BUG ID 4522
   const BProtoAction* pProtoAction=NULL;
//--
   if (pTarget)
      pProtoAction=pTactic->getProtoAction(mTacticState, pTarget, pTarget->getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitPointBlankAttack);
   else
      pProtoAction=pTactic->getProtoAction(mTacticState, NULL, target.getPosition(),
         getPlayer(), getPosition(), getID(), target.getAbilityID(), false, BAction::cActionTypeUnitPointBlankAttack);
   if (!pProtoAction)
      return (cDoOppFail);
   BASSERT(pProtoAction->getActionType() == BAction::cActionTypeUnitPointBlankAttack);

   pAction = reinterpret_cast<BUnitActionPointBlankAttack*>(gActionManager.createAction(BAction::cActionTypeUnitPointBlankAttack));

   pAction->setOppID(pOpp->getID());
   pAction->setProtoAction(pProtoAction);
   if (!addAction(pAction))
      return (cDoOppFail);
   return (cDoOppInProgress);
}

//==============================================================================
//==============================================================================
BActionType BUnit::getMoveActionType() const
{
   const BProtoObject* pProtoObject = getProtoObject();
   BDEBUG_ASSERT(pProtoObject);
   if (pProtoObject->getPhysicsInfoID() >= 0)
   {
//-- FIXING PREFIX BUG ID 4523
      const BPhysicsInfo* pInfo = gPhysicsInfoManager.get(pProtoObject->getPhysicsInfoID(), true);
//--
      if (pInfo->getVehicleType() == BPhysicsInfo::cWarthog)
         return (BAction::cActionTypeUnitMoveWarthog);
      else if (pInfo->getVehicleType() == BPhysicsInfo::cGhost)
         return (BAction::cActionTypeUnitMoveGhost);
      else if ((pInfo->getVehicleType() == BPhysicsInfo::cHawk) ||
         (pInfo->getVehicleType() == BPhysicsInfo::cHornet) ||
         (pInfo->getVehicleType() == BPhysicsInfo::cVulture) ||
         (pInfo->getVehicleType() == BPhysicsInfo::cBanshee) ||
         (pInfo->getVehicleType() == BPhysicsInfo::cVampire) ||
         (pInfo->getVehicleType() == BPhysicsInfo::cSentinel))
         return (BAction::cActionTypeUnitMoveGhost);
   }
   //XXXHalwes - 7/10/2009 - Just test for shortsword for now, until all fliers use this action.
   else if (gConfig.isDefined(cConfigEnableFlight) && pProtoObject->getFlagFlying())
      return (BAction::cActionTypeUnitMoveAir);
   else if (pProtoObject->getFlagAirMovement())
      return (BAction::cActionTypeUnitMoveAir);

   return (BAction::cActionTypeUnitMove);
}

//==============================================================================
//==============================================================================
uint BUnit::getNumberTacticStates() const
{
//-- FIXING PREFIX BUG ID 4524
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
      return (0);
   return (pTactic->getNumberStates());
}

//==============================================================================
//==============================================================================
void BUnit::setTacticState(uint8 v)
{
   if (v == mTacticState)
      return;
//-- FIXING PREFIX BUG ID 4525
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
      return;
   if (v >= pTactic->getNumberStates())
      return;
   mTacticState=v;
   mAnimationState.setDirty();
}

//==============================================================================
//==============================================================================
void BUnit::setRandomTacticState()
{
//-- FIXING PREFIX BUG ID 4526
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic || (pTactic->getNumberStates() <= 1))
      clearTacticState();
   else
   {
      uint8 v = getRand(cSimRand)%pTactic->getNumberStates();
      if (v != mTacticState)
      {
         mTacticState = v;
         mAnimationState.setDirty();
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::clearTacticState()
{
   if (mTacticState != -1)
   {
      mTacticState = -1;
      mAnimationState.setDirty();
   }
}

//==============================================================================
//==============================================================================
long BUnit::getIdleAnim() const
{
//-- FIXING PREFIX BUG ID 4527
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
      return (mFlagRecovering ? cAnimTypeRecover : cAnimTypeIdle);
   const BTacticState* pTacticState=pTactic->getState(mTacticState);
   if (!pTacticState || pTacticState->mIdleAnim == -1)
      return (mFlagRecovering ? cAnimTypeRecover : cAnimTypeIdle);
   return (pTacticState->mIdleAnim);
}

//==============================================================================
//==============================================================================
long BUnit::getWalkAnim() const
{
   if (mFlagInfected)
      return (cAnimTypeFloodDeathJog);

//-- FIXING PREFIX BUG ID 4528
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
      return (cAnimTypeWalk);

   const BTacticState* pTacticState=pTactic->getState(mTacticState);
   if (!pTacticState || pTacticState->mWalkAnim == -1)
      return (cAnimTypeWalk);
   return (pTacticState->mWalkAnim);
}

//==============================================================================
//==============================================================================
long BUnit::getJogAnim() const
{
   if (mFlagInfected)
      return (cAnimTypeFloodDeathJog);

//-- FIXING PREFIX BUG ID 4529
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
      return (cAnimTypeJog);

   const BTacticState* pTacticState=pTactic->getState(mTacticState);
   if (!pTacticState || pTacticState->mJogAnim == -1)
      return (cAnimTypeJog);
   return (pTacticState->mJogAnim);
}

//==============================================================================
//==============================================================================
long BUnit::getRunAnim() const
{
   if (mFlagInfected)
      return (cAnimTypeFloodDeathJog);

//-- FIXING PREFIX BUG ID 4530
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
      return (mFlagSprinting ? cAnimTypeSprint : cAnimTypeRun);

   const BTacticState* pTacticState=pTactic->getState(mTacticState);
   if (!pTacticState || pTacticState->mRunAnim == -1)
      return (mFlagSprinting ? cAnimTypeSprint : cAnimTypeRun);
   return (pTacticState->mRunAnim);
}

//==============================================================================
//==============================================================================
long BUnit::getDeathAnim() const
{
//-- FIXING PREFIX BUG ID 4531
   const BTactic* pTactic=getTactic();
//--
   if (pTactic)
   {
      const BTacticState* pTacticState=pTactic->getState(mTacticState);
      if (pTacticState && pTacticState->mDeathAnim != -1)
         return (pTacticState->mDeathAnim);
   }

   if (mFlagShatterOnDeath && hasAnimation(cAnimTypeShatterDeath))
      return cAnimTypeShatterDeath;

   return (cAnimTypeDeath);
}

//==============================================================================
//==============================================================================
const BUnitOpp* BUnit::getOppByID(BUnitOppID oppID) const
{
   for (uint i=0; i < mOpps.getSize(); i++)
   {
      if (mOpps[i]->getID() == oppID)
         return (mOpps[i]);
   }
   return (NULL);
}

//==============================================================================
//==============================================================================
uint8 BUnit::getOppPriority(BUnitOppID oppID) const
{
   for (uint i=0; i < mOpps.getSize(); i++)
   {
      if (mOpps[i]->getID() == oppID)
         return (mOpps[i]->getPriority());
   }
   return (0);
}

//==============================================================================
//==============================================================================
/*int __cdecl oppPrioritySort(const void *a, const void *b)
{
   BUnitOpp* o1=(BUnitOpp*)a;
   BUnitOpp* o2=(BUnitOpp*)b;
   if (o1->getPriority() < o2->getPriority())
      return (1);
   else if (o1->getPriority() > o2->getPriority())
      return (-1);
   return (0);
}*/

//==============================================================================
//==============================================================================
bool BUnit::addOpp(BUnitOpp* pOpp)
{
   #ifdef DEBUGOPPS
   if (mPlayerID == 1)
      debug("AddOpp: OppID=%d, Target=%d", pOpp->getID(), pOpp->getTarget().getID());
   #endif

   //Lots of ASSERTs to catch undesired behavior.
   //Must be alive.
   BASSERT(pOpp);
   BASSERT(mFlagAlive || (pOpp->getType() == BUnitOpp::cTypeDeath) || (pOpp->getType() == BUnitOpp::cTypeInfectDeath) || (pOpp->getType() == BUnitOpp::cTypeHeroDeath));
   //We only allow target types that are None, Units, or Squads.
   BASSERT(!pOpp->getTarget().getID().isValid() ||
      (pOpp->getTarget().getID().getType() == BEntity::cClassTypeUnit) ||
      (pOpp->getTarget().getID().getType() == BEntity::cClassTypeSquad));
   //Only death, leash and trigger opps can be at that priority.
   BASSERT((pOpp->getType() == BUnitOpp::cTypeDeath) || (pOpp->getType() == BUnitOpp::cTypeInfectDeath) || (pOpp->getType() == BUnitOpp::cTypeHeroDeath) ||
           (pOpp->getType() == BUnitOpp::cTypeThrown) || (pOpp->getType() == BUnitOpp::cTypeEvade) || (pOpp->getType() == BUnitOpp::cTypeRetreat) ||
           (pOpp->getPriority() < BUnitOpp::cPriorityTrigger) || pOpp->getLeash() || pOpp->getTrigger());

   BASSERT(pOpp->getPriority() != BUnitOpp::cPriorityNone);
   //debug("AddOpp:  ID=%d, Type=%s, Target=%s (Range=%.2f), Pri=%d.", pOpp->getID(), pOpp->getTypeName(),
   //   pOpp->getTarget().getID().getDebugString().getPtr(), pOpp->getTarget().getRange(),
   //   pOpp->getPriority());
   // Physics controlled units shouldn't get unit move actions, only squad move actions for now
   // We're just assuming single-unit squads for units with physics, that's why they shouldn't need a unitMoveAction
   //if (getFlagPhysicsControl())
// #ifndef _MOVE4
   {
      BPhysicsInfo *pInfo = gPhysicsInfoManager.get(getProtoObject()->getPhysicsInfoID(), true);
      BASSERT(!((pOpp->getType() == BUnitOpp::cTypeMove) && pInfo && pInfo->isVehicle()));
   }
// #endif

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::addOpp pOpp ID", (int)pOpp->getID());
   syncUnitActionData("BUnit::addOpp pOpp Type", (int)pOpp->getType());
#endif

   //Spotty opp validation.  Don't accept things we know we can't do.  But, we only
   //check for really explicit failures.
   //Move and animation opps don't need a tactic; other opps do.
//-- FIXING PREFIX BUG ID 4532
   const BTactic* pTactic = getTactic();
//--
   BUnitOppType oppType = pOpp->getType();
   if (!pTactic && ((oppType != BUnitOpp::cTypeMove) && (oppType != BUnitOpp::cTypeAnimation) && (oppType != BUnitOpp::cTypeDeath) && (oppType != BUnitOpp::cTypeInfectDeath) && (oppType != BUnitOpp::cTypeHeroDeath) && (oppType != BUnitOpp::cTypeChangeMode)))
      return (false);
   switch (pOpp->getType())
   {
      case BUnitOpp::cTypeCapture:
         if (!pTactic->can(mTacticState, BAction::cActionTypeUnitCapture))
            return (false);
         break;
      case BUnitOpp::cTypeJoin:
         if (!pTactic->can(mTacticState, BAction::cActionTypeUnitJoin))
            return (false);
         break;
      case BUnitOpp::cTypeMines:
         if (!pTactic->can(mTacticState, BAction::cActionTypeUnitMines))
            return (false);
         break;
      case BUnitOpp::cTypeGather:
         if (!pTactic->can(mTacticState, BAction::cActionTypeUnitGather))
            return (false);
         break;
   }

   //Figure out where to add it.  Also, make sure we don't have any dupes.
   uint insertIndex=mOpps.getSize();
   for (uint i=0; i < mOpps.getSize(); i++)
   {
      //See if we match.  If we do, fail.
      if (mOpps[i]->addEqual(pOpp))
         return (false);
      //Once we've hit something of lower priority than the new opp, this
      //is where we want to add it since we're past the higher/equal priority
      //opps.  Obviously, we don't need to finish checking the list for dupes
      //either since we can't dupe a different priority.
      if (mOpps[i]->getPriority() < pOpp->getPriority())
      {
         if(!(mOpps[i]->getEvaluated() == true && mOpps[i]->getMustComplete()))
         {
            insertIndex=i;
            break;
         }
      }
   }
   
   //If we're here, add it.
   if (insertIndex < mOpps.getSize())
      mOpps.insert(insertIndex, pOpp);
   else
      mOpps.add(pOpp);
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnit::removeOpp(BUnitOppID oppID, bool removeActions)
{
   #ifdef DEBUGOPPS
   if (mPlayerID == 1)
      debug("RemoveOpp: OppID=%d, RemoveActions=%d.", oppID, removeActions);
   #endif
   if (oppID == BUnitOpp::cInvalidID)
      return (false);

   //NOTE: This can now remove actions, etc.  Know what you're doing when you call it.

   //debug("RemoveOpp:  ID=%d.", oppID);
   for (uint i=0; i < mOpps.getSize(); i++)
   {
      BUnitOpp* pOpp=mOpps[i];
      if (pOpp->getID() == oppID)
      {
         mOpps.removeIndex(i);
         //Remove our actions.
         if (removeActions && pOpp->getRemoveActions())
            removeActionsForOpp(pOpp);
         BUnitOpp::releaseInstance(pOpp);
         return (true);
      }
   }
   
   return (false);
}

//==============================================================================
//==============================================================================
void BUnit::removeOpps(bool notifySources)
{
   //If we're allowed to notify our opp sources, see if we have any to notify:)
   if (notifySources)
   {
      for (uint i=0; i < mOpps.getSize(); i++)
      {
         BUnitOpp* pOpp=mOpps[i];
         //Notify our source.
         if (pOpp->getNotifySource())
         {
            BEntity* pSource=gWorld->getEntity(pOpp->getSource());
            if (pSource)
               pSource->notify(BEntity::cEventOppComplete, mID, pOpp->getID(), false);
         }
         //Remove our actions.
         if (pOpp->getRemoveActions())
            removeActionsForOpp(pOpp);
         //Nuke.
         BUnitOpp::releaseInstance(pOpp);
      }
   }
   
   mOpps.clear();
}

//==============================================================================
//==============================================================================
void BUnit::removeActionsForOpp(BUnitOpp* pOpp)
{
   #ifdef DEBUGOPPS
   if (mPlayerID == 1)
      debug("RemoveActionForOpp: OppID=%d, Target=%d", pOpp->getID(), pOpp->getTarget().getID());
   #endif

   BASSERT(pOpp);
   mActions.removeAllActionsForOpp(pOpp);
}

//==============================================================================
//==============================================================================
bool BUnit::completeOpp(BUnitOppID oppID, bool success)
{
   #ifdef DEBUGOPPS
   if (mPlayerID == 1)
      debug("CompleteOpp: OppID=%d, Success=%d.", oppID, success);
   #endif

   for (uint i=0; i < mOpps.getSize(); i++)
   {
      BUnitOpp* pOpp=mOpps[i];
      if (pOpp->getID() == oppID)
      {
         //If this opp isn't allowed to be completed, bail now.
         if (!pOpp->getAllowComplete())
            return (false);
         //Mark it.
         pOpp->setComplete(true);
         pOpp->setCompleteValue(success);
         return (true);
      }
   }
   return (false);
}

//==============================================================================
//==============================================================================
bool BUnit::completeOppForReal(BUnitOpp* pOpp)
{
   #ifdef DEBUGOPPS
   if (mPlayerID == 1)
      debug("CompleteOppForReal: OppID=%d.", pOpp->getID());
   #endif

   //NOTE: This can now remove actions, etc.  Know what you're doing when you call it.
   BDEBUG_ASSERT(pOpp);

   //If this opp isn't allowed to be completed, bail now.
   if (!pOpp->getAllowComplete())
      return (false);
      
   //E3: Reload.
   if ((pOpp->getType() == BUnitOpp::cTypeReload) && pOpp->getCompleteValue())
      resetVisualAmmo(pOpp->getUserData2());
   //Notify our source.      
   BEntity* pSource=gWorld->getEntity(pOpp->getSource());
   if (pSource && pOpp->getNotifySource())
      pSource->notify(BEntity::cEventOppComplete, mID, pOpp->getID(), pOpp->getCompleteValue());
   //Remove our actions.
   if (pOpp->getRemoveActions())
      removeActionsForOpp(pOpp);
   //Nuke it.
   mOpps.remove(pOpp);
   BUnitOpp::releaseInstance(pOpp);
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnit::determineTarget(BUnitOpp* pOpp, BSimTarget& target)
{
   BASSERT(pOpp);
   target=pOpp->getTarget();
   //Make sure we end up with a unit target.
   if (pOpp->getTarget().getID().isValid())
   {
      bool bValidOpp = false;
      //If we already have a unit, we're good.
      if (pOpp->getTarget().getID().getType() == BEntity::cClassTypeUnit)
      {
         BUnit* pTargetUnit = gWorld->getUnit(pOpp->getTarget().getID());

         bool boarded = (pTargetUnit && pTargetUnit->getFlagBeingBoarded());

         // If attacking see if the target unit has garrisoned enemies in cover
         if (pOpp->getType() == BUnitOpp::cTypeAttack && !boarded)
         {
            bool meleeOnly = getProtoObject() ? getProtoObject()->getFlagRegularAttacksMeleeOnly() : false;

            if (pTargetUnit && pTargetUnit->hasGarrisonedEnemies(getPlayerID()) && !meleeOnly)
            {
               BEntityIDArray coverUnits = pTargetUnit->getCoverUnits();
               uint numCoverUnits = coverUnits.getSize();
               if (numCoverUnits > 0)
               {
                  uint randomIndex = getRandRange(cSimRand, 0, (numCoverUnits - 1));
                  target.setID(coverUnits[randomIndex]);
               }
            }
         }
         // If joining, target needs to be a squad.
         else if (pOpp->getType() == BUnitOpp::cTypeJoin)
         {
            if (pTargetUnit && pTargetUnit->getParentID() != cInvalidObjectID)
               target.setID(pTargetUnit->getParentID());
            else
               return (false);
         }
         // [11-12-08 CJS] If acting on a unit being jacked, target the jacking unit
         else if (boarded)
         {
         	// Find boarding ref
            const BEntityRef *pRef = pTargetUnit->getFirstEntityRefByType(BEntityRef::cTypeBoardingUnit);
            if (pRef && pRef->mID != cInvalidObjectID)
            {
               target.setID(pRef->mID);
            }
         }
         bValidOpp = true;
      }
      //Else, if we have a squad, get a unit.
      else if (pOpp->getTarget().getID().getType() == BEntity::cClassTypeSquad)
      {
         // If joining, target needs to be a squad.
         if (pOpp->getType() == BUnitOpp::cTypeJoin)
            return (true);

         BSquad* pTargetSquad = gWorld->getSquad(pOpp->getTarget().getID());
         BDEBUG_ASSERT(getParentSquad());
         //BSquad* pParentSquad=getParentSquad(); mrh 7/109/07 - removed warning. 
         //BDEBUG_ASSERT(pParentSquad);

         bool boarded = (pTargetSquad && pTargetSquad->getLeaderUnit() && pTargetSquad->getLeaderUnit()->getFlagBeingBoarded());

         if (pTargetSquad)
         {
            bool meleeOnly = getProtoObject() ? getProtoObject()->getFlagRegularAttacksMeleeOnly() : false;

            // If attacking see if the target squad has garrisoned enemies in cover
            if ((pOpp->getType() == BUnitOpp::cTypeAttack) && pTargetSquad->hasGarrisonedEnemies(getPlayerID()) && !meleeOnly && !boarded)
            {
               BEntityIDArray coverUnits = pTargetSquad->getCoverUnits();
               uint numCoverUnits = coverUnits.getSize();
               if (numCoverUnits > 0)
               {
                  uint randomIndex = getRandRange(cSimRand, 0, (numCoverUnits - 1));
                  target.setID(coverUnits[randomIndex]);
               }
               else
               {
                  target.setID(pTargetSquad->getRandomUnitID(this, cPi));
               }
            }
            // [11-12-08 CJS] If acting on a unit being jacked, target the jacking unit
            else if (boarded)
            {
               // Find boarding ref
               const BEntityRef *pRef = pTargetSquad->getLeaderUnit()->getFirstEntityRefByType(BEntityRef::cTypeBoardingUnit);
               if (pRef && pRef->mID != cInvalidObjectID)
                  target.setID(pRef->mID);
               else
                  target.setID(pTargetSquad->getRandomUnitID(this, cPi));   
            }
            else
            {
               target.setID(pTargetSquad->getRandomUnitID(this, cPi));
            }
            bValidOpp = true;
         }
      }
      // Fail if Opp not Valid..
      if (!bValidOpp)
         return (false);
      #ifdef _MOVE4
      // DLM - 5/7/8 We don't give the target to the unit action, because for squad moves it's just going
      // to follow the squad anyway.  At least, I hope.
      if (pOpp->getType() == BUnitOpp::cTypeMove && pOpp->getSource().getType() == BEntity::cClassTypeSquad)
      {
         BSquad *pSquad = reinterpret_cast<BSquad *>(gWorld->getEntity(pOpp->getSource()));
         if (pSquad)
         {     
            BSimTarget unitTarget;
            BVector newPosition;
            //pSquad->getChildTargetLocation_4(getID(), newPosition);
            pSquad->getDesiredChildLocation(getID(), newPosition);
            unitTarget.setPosition(newPosition);
            pOpp->setTarget(unitTarget);
         }
      }
      #endif
   }
#ifdef _MOVE4
   else
   {
      // If we don't have a target id, but are just moving, we still need to update
      // our target pos each update, as the squad's target pos is updated each update.
      if (pOpp->getType() == BUnitOpp::cTypeMove && pOpp->getSource().getType() == BEntity::cClassTypeSquad)
      {
         BSquad *pSquad = reinterpret_cast<BSquad *>(gWorld->getEntity(pOpp->getSource()));
         if (pSquad)
         {            
            BVector newPosition;
            //pSquad->getChildTargetLocation_4(getID(), newPosition);
            pSquad->getDesiredChildLocation(getID(), newPosition);
            target.setPosition(newPosition);
            pOpp->setTarget(target);
         }
      }
   }
#endif
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnit::grabController(uint index, BAction* pAction, BUnitOppID oppID)
{ 
   BASSERT(pAction);
   BASSERT(index < BActionController::cNumberControllers);

   if (getFlagDoingFatality())
      return false;

   // SLB: Uh, we can't animate. Why let something grab the controller?
   if ((index == BActionController::cControllerAnimation) && getFlagAnimationDisabled())
      return false;

#ifdef SYNC_UnitAction
   // AJL 2/14/08 - Removed action ID sync check as they can get OOS since the action ID's are used up by both sync'd and non-sync'd entities.
   // DJB 2/28/08 - Shouldn't action ID's always be synched? Addeding back action ID synching to see what's causing this.
   syncUnitActionData("BUnit::grabHardpoint pAction ID", (int)pAction->getID());
   syncUnitActionData("BUnit::grabHardpoint pAction type", (int)pAction->getType());
   syncUnitActionData("BUnit::grabHardpoint controllers action id", (int)mControllers[index].getActionID());
   syncUnitActionData("BUnit::grabHardpoint oppID", (int)oppID);
#endif

   //Return true if this action already has it.
   if (mControllers[index].getActionID() == pAction->getID())
      return (true);
   //Fails if it's not free.
   // But a persistent action can always grab the controller.
   // FIXME BSR 9/19/07: and so can a moveAir user...
//-- FIXING PREFIX BUG ID 4536
   const BUnitActionMoveAir* pAirAction=(BUnitActionMoveAir*)getActionByType(BAction::cActionTypeUnitMoveAir);
//--
   bool bUsesMoveAir = false;
   if (pAirAction)
      bUsesMoveAir = true;

   uint currentPriority=getOppPriority(mControllers[index].getOppID());
   uint newPriority=getOppPriority(oppID);

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::grabHardpoint bUsesMoveAir", bUsesMoveAir);
   syncUnitActionData("BUnit::grabHardpoint persistent", pAction->getFlagPersistent());
   syncUnitActionData("BUnit::grabHardpoint currentPriority", (DWORD)currentPriority);
   syncUnitActionData("BUnit::grabHardpoint newPriority", (DWORD)newPriority);
#endif

   if (!bUsesMoveAir && !pAction->getFlagPersistent() && currentPriority >= newPriority)
      return (false);
//-- FIXING PREFIX BUG ID 4537
   const BAction* pCurrentAction = mActions.getActionByID(mControllers[index].getActionID());
//--

#ifdef SYNC_UnitAction
   if (pCurrentAction)
   {
      syncUnitActionData("BUnit::grabHardpoint isInterruptible", pCurrentAction->isInterruptible());
   }
   else
   {
      syncUnitActionCode("BUnit::grabHardpoint pCurrentAction null");
   }
#endif

   if ((pCurrentAction != NULL) && (!pCurrentAction->isInterruptible()))
      return (false);
   //Take it.
   mControllers[index].setActionID(pAction->getID());
   mControllers[index].setOppID(oppID);

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnit::grabHardpoint controllers action id", (int)mControllers[index].getActionID());
   syncUnitActionData("BUnit::grabHardpoint controllers oppID", (int)mControllers[index].getOppID());
#endif

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnit::releaseController(uint index, BAction* pAction)
{
   BASSERT(pAction);
   BASSERT(index < BActionController::cNumberControllers);
   //Only release it if we own it.
   if (mControllers[index].getActionID() != pAction->getID())
      return (false);
   mControllers[index].init();
   return (true);
}

//==============================================================================
//==============================================================================
void BUnit::updateControllerOppIDs(BUnitOppID oldID, BUnitOppID newID)
{
   for (uint i=0; i < BActionController::cNumberControllers; i++)
   {
      if (mControllers[i].getOppID() == oldID)
         mControllers[i].setOppID(newID);
   }
}

//==============================================================================
//==============================================================================
bool BUnit::isControllerFree(uint index, BUnitOppID oppID) const
{
   BASSERT(index < BActionController::cNumberControllers);
   //Succeed if this opp already has this.
   if (oppID == mControllers[index].getOppID())
      return (true);
   uint currentPriority=getOppPriority(mControllers[index].getOppID());
   uint newPriority=getOppPriority(oppID);
   return (currentPriority < newPriority);
}

//==============================================================================
// BUnit::beginPlayBlockingAnimation
//==============================================================================
bool BUnit::beginPlayBlockingAnimation(long state, long type, bool applyInstantly, bool useMaxHeight, bool forceReset, long forceAnimID,
                                       BProtoVisualAnimExitAction* pOverrideExitAction, bool loop, bool disableMotionExtractionCollisions,
                                       bool clearUnitNoRenderFlag)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::beginPlayBlockingAnimation mID", mID.asLong());
   #endif
   if (!getFlagAlive())
      return false;

   setFlagUseMaxHeight(useMaxHeight);

   //-- Create the action
   BUnitActionPlayBlockingAnimation *pAction = (BUnitActionPlayBlockingAnimation *) gActionManager.createAction(BAction::cActionTypeUnitPlayBlockingAnimation);
   pAction->setAnimationState(state, type, applyInstantly, forceReset, forceAnimID);
   pAction->setFlagLoop(loop);
   if (pOverrideExitAction)
      pAction->overrideExitAction(pOverrideExitAction);
   pAction->setDisableMotionExtractionCollisions(disableMotionExtractionCollisions);
   pAction->setClearUnitNoRenderFlag(clearUnitNoRenderFlag);
   addAction(pAction);

   return (true);
}

//==============================================================================
// BUnit::endPlayBlockingAnimation()
//==============================================================================
bool BUnit::endPlayBlockingAnimation( void )
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::endPlayBlockingAnimation mID", mID.asLong());
   #endif

   setFlagUseMaxHeight(false);

   bool result = true;
   result |= mActions.removeAllActionsOfType(BAction::cActionTypeUnitPlayBlockingAnimation);
   return result;
}

//==============================================================================
// BUnit::getTrainLimit
//==============================================================================
long BUnit::getTrainLimit(BPlayerID playerID, long id, bool squad) const
{
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      return pAction->getTrainLimit(playerID, id, squad);
   return -1;
}

//==============================================================================
// BUnit::doSelfDestruct
//==============================================================================
void BUnit::doSelfDestruct(bool cancel)
{
   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      pAction->doSelfDestruct(cancel);
}

//==============================================================================
// BUnit::isSelfDestructing
//==============================================================================
bool BUnit::isSelfDestructing(float& timeRemaining) const
{
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      return pAction->isSelfDestructing(timeRemaining);
   return false;
}

//==============================================================================
// BUnit::getRecharging
//==============================================================================
bool BUnit::getRecharging(bool squad, int protoID, float* pTimeRemaining) const
{
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      return pAction->getRecharging(squad, protoID, pTimeRemaining);
   return false;
}

//==============================================================================
// BUnit::setRecharge
//==============================================================================
void BUnit::setRecharge(bool squad, int protoID, float timeRemaining)
{
   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      pAction->setRecharge(squad, protoID, timeRemaining);
}

//==============================================================================
// BUnit::onBuilt
//==============================================================================
void BUnit::onBuilt(bool fromChangeOwner, bool physicsReplacement, bool bStartExistSound, bool fromSave)
{
   setFlagBuilt(true);
   BPlayer *pPlayer = getPlayer();
   const BProtoObject *pProtoObject = getProtoObject();

   // Pop cap additions
   long popCount=pProtoObject->getNumberPopCapAdditions();
   if(popCount>0)
   {
      for(long i=0; i<popCount; i++)
      {
         BPop pop=pProtoObject->getPopCapAddition(i);
         pPlayer->adjustPopCap(pop.mID, pop.mCount);
      }
   }

   toggleAddResource(true);

   // Player rate adjustment
   int rateID = pProtoObject->getRateID();
   if (rateID != -1)
      pPlayer->adjustRateAmount(rateID, pProtoObject->getRateAmount());

   // Child object damage taken scalar
   updateChildObjectDamageTakenScalar(false);

   // Persistent actions
   if (!physicsReplacement)
      createPersistentActions();

   // Block LOS
   if(pProtoObject->getFlagBlockLOS())
   {
      long radius = getSimLOS();
      BTeamID teamID = pPlayer->getTeamID();
      for(long i=1; i<gWorld->getNumberTeams(); i++)
      {
         if(i!=teamID)
            gVisibleMap.blockCircularRegion(mSimX, mSimZ, radius, i);
      }
      setFlagBlockLOS(true);
   }

   if (mFlagFirstBuilt)
   {
      //gWorld->notify2(BEntity::cEventBuiltUnit, mPlayerID, pProtoObject->getID());
      gWorld->notify(BEntity::cEventBuilt, mID, mPlayerID, 0);

      mFlagFirstBuilt=false;
   }

   if (!fromChangeOwner)
   {
      // Auto-train on built
      if (pProtoObject->getAutoTrainOnBuiltID() != -1)
         doTrain(mPlayerID, pProtoObject->getAutoTrainOnBuiltID(), 1, true, true);
   }

   // Create child objects (sockets, parking lot, etc)
   if (!fromSave)
      createChildObjects();

   // Initialize group weapon range calculation data (if applicable)
   initGroupRangePointsForWeapon();

   setAnimationRate(1.0f);

   // mrh - Apply UNSC & COV dynamic work rate modifiers.  This replaces a trigger script that was running continuously.
   recalculateSupplyPadWorkRateModifiers();

   // Start the exist sound, only after the unit is built
   if (bStartExistSound)
      startExistSound();
}


//==============================================================================
// Code side implementation to avoid spamming trigger scripts all over
//==============================================================================
void BUnit::recalculateSupplyPadWorkRateModifiers()
{
   // If we have no player, return.
//-- FIXING PREFIX BUG ID 4541
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
//--
   BASSERT(pPlayer);
   if (!pPlayer)
      return;

   // Set up some stuff to loop.
   enum { cUnsc = 0, cCov = 1, cNumSupplyPadOTIDs = 2, };
   BObjectTypeID supplyPadOTIDs[cNumSupplyPadOTIDs];
   supplyPadOTIDs[cUnsc] = gDatabase.getOTIDUnscSupplyPad();
   supplyPadOTIDs[cCov] = gDatabase.getOTIDCovSupplyPad();

   // Do this in a loop because it's almost identical code for each.
   for (uint otid_idx=cUnsc; otid_idx<cNumSupplyPadOTIDs; otid_idx++)
   {
      // If we are not this object type id... skip.
      if (!isType(supplyPadOTIDs[otid_idx]))
         continue;

      // We are this type, but don't have any... return.
      BEntityIDArray possibleSupplyPads;
      uint numPossible = pPlayer->getUnitsOfType(supplyPadOTIDs[otid_idx], possibleSupplyPads);
      if (numPossible == 0)
         return;

      // Go through the possible supply pads and add any valid ones to the valid list.
      BSmallDynamicSimArray<BUnit*> validSupplyPads(0, numPossible);
      for (uint i=0; i<numPossible; i++)
      {
         // Bad supply pad doesn't count.
         BUnit* pUnit = gWorld->getUnit(possibleSupplyPads[i]);
         if (!pUnit)
            continue;
         // Supply pad still building doesn't count.
         if (!pUnit->mFlagIsDoneBuilding)
            continue;
         // Dead supply pad doesn't count.
         if (!pUnit->mFlagAlive)
            continue;

         // Ok we count.
         validSupplyPads.add(pUnit);
      }

      // If we had some supply pads of this type, but none are valid... return.
      uint numValidSupplyPads = validSupplyPads.getSize();
      if (numValidSupplyPads == 0)
         return;

      // We have some valid... Calculate the new modifier with the appropriate function.
      float modifier = 1.0f;
      if (otid_idx == cUnsc)
         modifier = gDatabase.calculateUnscSupplyPadModifier(static_cast<float>(numValidSupplyPads));
      else if (otid_idx == cCov)
         modifier = gDatabase.calculateCovSupplyPadModifier(static_cast<float>(numValidSupplyPads));

      // Apply the modifier to all valid supply pads of the type.
      for (uint i=0; i<numValidSupplyPads; i++)
         validSupplyPads[i]->setWorkRateScalar(modifier);

      // We can return and do not have to iterate any more.
      return;   
   }
}


//==============================================================================
// BUnit::onKillOrTransform
//==============================================================================
void BUnit::onKillOrTransform(long transformToProtoID)
{
   BPlayer* pPlayer = getPlayer();
   BASSERT(pPlayer);

   if(getFlagBuilt())
   {
      // Remove pop cap additions
      const BProtoObject* pProtoObject=getProtoObject();
      long popCount=pProtoObject->getNumberPopCapAdditions();
      if(popCount>0)
      {
         for(long i=0; i<popCount; i++)
         {
            BPop pop=pProtoObject->getPopCapAddition(i);
            pPlayer->adjustPopCap(pop.mID, -pop.mCount);
         }
      }

      toggleAddResource(false);

      // Remove player rate adjustment
      int rateID = pProtoObject->getRateID();
      if (rateID != -1)
         pPlayer->adjustRateAmount(rateID, -pProtoObject->getRateAmount());

      // Child object damage taken scalar
      updateChildObjectDamageTakenScalar(true);

      // Revoke power
      long protoPowerID = pProtoObject->getProtoPowerID();
      if (protoPowerID != -1)
      {
//-- FIXING PREFIX BUG ID 4544
         const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
         if (pProtoPower)
         {
//-- FIXING PREFIX BUG ID 4543
            const BSquad* pParentSquad = getParentSquad();
//--
            if (pParentSquad && (transformToProtoID != -1))
            {
               getPlayer()->removePowerEntry(protoPowerID, getParentID());
            }
         }
      }

      // Unblock LOS
      if(pProtoObject->getFlagBlockLOS())
      {
         long radius = getSimLOS();
         BTeamID teamID = getPlayer()->getTeamID();
         for(long i=1; i<gWorld->getNumberTeams(); i++)
         {
            if(i!=teamID)
               gVisibleMap.unblockCircularRegion(mSimX, mSimZ, radius, i);
         }
      }
   }

   // Remove from placement rules tracking
   const BProtoObject *pProto = getProtoObject();
   if (pProto->getFlagTrackPlacement())
      BPlacementRules::getUnitsToTrack().remove(mID);
}

//==============================================================================
// BUnit::doUnload
//==============================================================================
//bool BUnit::doUnload(void)
//{
//   //XXXHalwes - 7/17/2007 - We shouldn't be calling this anymore in the new sim.  The unit should have an ungarrison opp or if it is
//   //                        a special case use unloadUnit function.
//   BASSERT(0);
//
//   //#ifdef SYNC_Unit
//   //   syncUnitData("BUnit::doUnload mID", mID.asLong());
//   //#endif
//
//   //gWorld->notify(BEntity::cEventUnloaded, mID, 0, 0);
//
//   //bool haveSquadPos = false;
//   //BVector squadPosition;
//   //bool unloaded = false;
//
//   //long numEntityRefs = getNumberEntityRefs();
//   //for (long i = numEntityRefs - 1; i >= 0; i--)
//   //{
//   //   BEntityRef *pEntityRef = getEntityRefByIndex(i);
//   //   if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
//   //   {
//   //      // Cache this off because unloadUnit destroys the ref.
//   //      BEntityID entityRefID = pEntityRef->mID;
//
//   //      // Unload unit
//   //      unloadUnit(i, false);   // REMOVES THE ENTITY REF
//   //      unloaded = true;
//
//   //      BUnit *pUnit = gWorld->getUnit(entityRefID);
//   //      if (pUnit)
//   //      {
//-- FIXING PREFIX BUG ID 4534
//   //         const BSquad* pSquad = gWorld->getSquad(pUnit->getParentID());
//--
//   //         if(pSquad)
//   //         {
//   //            // Calculate the squad position
//   //            if (!haveSquadPos)
//   //            {
//   //               squadPosition = pSquad->getPosition();
//   //               haveSquadPos = true;
//
//   //               if(mpObstructionNode)
//   //               {
//   //                  BOPQuadHull* hull = (BOPQuadHull*) mpObstructionNode->getHull(); // Cast to remove const
//   //                  if(hull)
//   //                  {
//   //                     BVector position = squadPosition;    
//   //                     // AJL FIXME 1/23/07 - Shouldn't the call below use pUnit's obstruction radius instead of the values from the container unit?
//   //                     hull->suggestPlacement(position, mObstructionRadiusX, mObstructionRadiusZ, squadPosition);
//   //                     BVector direction = squadPosition - position;
//   //                     if(direction.safeNormalize())
//   //                     {
//   //                        direction.scale(pSquad->getObstructionRadius());
//   //                        squadPosition += direction;
//   //                     }
//   //                  }
//   //               }
//   //            }
//
//   //            // Update squad
//   //            if (!pSquad->isGarrisoned())
//   //            {
//   //               pSquad->setPosition(squadPosition);
//   //               pSquad->noStopSettle();
//   //            }
//   //         }
//   //      }
//   //   }
//   //}
//
//   //return unloaded;
//
//   return (false);
//}

//==============================================================================
// BUnit::doUnattach
//==============================================================================
//bool BUnit::doUnattach(bool offset /*= true*/)
//{
//   //XXXHalwes - 7/17/2007 - We shouldn't be calling this anymore in the new sim.  The unit should have an ungarrison opp or if it is
//   //                        a special case use unattachObject function.
//   BASSERT(0);
//
//   //#ifdef SYNC_Unit
//   //   syncUnitData("BUnit::doUnattach mID", mID.asLong());
//   //#endif
//
//   //bool haveSquadPos = false;
//   //BVector squadPosition = cInvalidVector;
//   //bool unattached = false;
//
//   //uint numEntityRefs = getNumberEntityRefs();
//   //for (int i = numEntityRefs - 1; i >= 0; i--)
//   //{
//   //   BEntityRef* pEntityRef = getEntityRefByIndex(i);
//   //   if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeAttachObject))
//   //   {
//   //      BEntityID entityRefID = pEntityRef->mID; // cache this off
//
//   //      // Unattach the unit
//   //      unattachObject(entityRefID);     // DESTROYS the entity ref
//   //      unattached = true;
//
//   //      BUnit* pUnit = gWorld->getUnit(entityRefID);
//   //      if (pUnit)
//   //      {
//-- FIXING PREFIX BUG ID 4535
//   //         const BSquad* pSquad = gWorld->getSquad(pUnit->getParentID());
//--
//   //         if (pSquad)
//   //         {
//   //            // Calculate the squad position with offset
//   //            if (!haveSquadPos && offset)
//   //            {
//   //               squadPosition = pSquad->getPosition();
//   //               haveSquadPos = true;
//
//   //               if (mpObstructionNode)
//   //               {
//   //                  BOPQuadHull* hull = (BOPQuadHull*)mpObstructionNode->getHull(); // Cast to remove const
//   //                  if (hull)
//   //                  {
//   //                     BVector position = squadPosition;
//   //                     // AJL FIXME 1/23/07 - Shouldn't the call below use pUnit's obstruction radius instead of the values from the container unit?
//   //                     hull->suggestPlacement(position, mObstructionRadiusX, mObstructionRadiusZ, squadPosition);
//   //                     BVector direction = squadPosition - position;
//   //                     if (direction.safeNormalize())
//   //                     {
//   //                        direction.scale(pSquad->getObstructionRadius());
//   //                        squadPosition += direction;
//   //                     }
//   //                  }
//   //               }
//   //            }
//   //            // Calculate the squad position with no offset
//   //            else if (!haveSquadPos && !offset)
//   //            {
//   //               squadPosition = pUnit->getPosition();
//   //               haveSquadPos = true;
//   //            }
//
//   //            // Update squad
//   //            if (!pSquad->isAttached())
//   //            {
//   //               pSquad->setPosition(squadPosition);
//   //               pSquad->noStopSettle();
//   //            }
//   //         }
//   //      }
//   //   }
//   //}
//
//   //return (unattached);
//   return (false);
//}


//==============================================================================
//==============================================================================
bool BUnit::doDodge(BEntityID dodgeeID, BVector trajectory)
{
   BUnitActionDodge* pAction = reinterpret_cast<BUnitActionDodge*>(getActionByType(BAction::cActionTypeUnitDodge));
   if (!pAction)
      return false;

   return pAction->tryDodge(dodgeeID, trajectory);
}

//==============================================================================
//==============================================================================
bool BUnit::isDodging(BEntityID dodgeeID)
{
   // Get dodge action
//-- FIXING PREFIX BUG ID 4546
   const BUnitActionDodge* pAction = reinterpret_cast<BUnitActionDodge*>(getActionByType(BAction::cActionTypeUnitDodge));
//--
   if (!pAction)
      return false;

   // If no dodgeeID is passed in and the dodge is still in progress, we just want to know if this unit is dodging *anything*
   if ((dodgeeID == cInvalidObjectID) && (pAction->getDodgeeID() != cInvalidObjectID) && ((pAction->getDodgeInitTime() + 2000) >= gWorld->getGametime()))
      return true;

   // Check that the object being dodged (the 'dodgee') matches the passed in id
   if ((dodgeeID != cInvalidObjectID) && (pAction->getDodgeeID() == dodgeeID))
      return true;

   return false;
}

//=============================================================================
//=============================================================================
bool BUnit::canDodge() const
{
//-- FIXING PREFIX BUG ID 4548
   const BUnitActionDodge* pAction = reinterpret_cast<const BUnitActionDodge*>(getActionByTypeConst(BAction::cActionTypeUnitDodge));
//--
   if (!pAction)
      return false;

   if (pAction->getDodgeeID() != cInvalidObjectID)
      return false;

   // Don't dodge if in cover
   if (isInCover())
      return false;

   // [7/3/2008 xemu] don't allow dodging if we have an active join action
   const BUnitActionJoin *pJoinAction = reinterpret_cast<const BUnitActionJoin*>(getActionByTypeConst(BAction::cActionTypeUnitJoin));
   if ((pJoinAction != NULL) && (pJoinAction->getFlagActive()))
      return false;

   // [7/3/2008 xemu] also we could just have a pending join action in our squad's work
   BSquad *pSquad = getParentSquad();
   if (pSquad != NULL)
   {
      BSquadActionWork *pSquadWork = reinterpret_cast<BSquadActionWork*>(pSquad->getActionByType(BAction::cActionTypeSquadWork));
      // [7/3/2008 xemu] we divine this inner secret by looking at the opptype 
      if ((pSquadWork != NULL) && (pSquadWork->getUnitOppType() == BUnitOpp::cTypeJoin))
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnit::doDeflect(BEntityID deflecteeID, BVector trajectory, float damage)
{
   BUnitActionDeflect* pAction = reinterpret_cast<BUnitActionDeflect*>(getActionByType(BAction::cActionTypeUnitDeflect));
   if (!pAction)
      return false;

   return pAction->tryDeflect(deflecteeID, trajectory, damage);
}

//==============================================================================
//==============================================================================
bool BUnit::isDeflecting(BEntityID deflecteeID)
{
   // Get deflect action
//-- FIXING PREFIX BUG ID 4551
   const BUnitActionDeflect* pAction = reinterpret_cast<BUnitActionDeflect*>(getActionByType(BAction::cActionTypeUnitDeflect));
//--
   if (!pAction)
      return false;

   // Check that the object being deflected (the 'deflectee') matches the passed in id
   if ((deflecteeID != cInvalidObjectID) && (pAction->getDeflecteeID() == deflecteeID))
      return true;

   return false;
}

//=============================================================================
//=============================================================================
bool BUnit::canDeflect() const
{
//-- FIXING PREFIX BUG ID 4409
   const BUnitActionDeflect* pAction = reinterpret_cast<const BUnitActionDeflect*>(getActionByTypeConst(BAction::cActionTypeUnitDeflect));
//--
   if (!pAction)
      return false;

   if (pAction->deflectSmallArms())
      return false;

   if (pAction->getDeflecteeID() != cInvalidObjectID)
      return false;

   return true;
}

//=============================================================================
//=============================================================================
bool BUnit::canDeflectSmallArms() const
{
   const BUnitActionDeflect* pAction = reinterpret_cast<const BUnitActionDeflect*>(getActionByTypeConst(BAction::cActionTypeUnitDeflect));
   if (!pAction)
      return false;

   if (!pAction->deflectSmallArms())
      return false;

   if (pAction->getDeflecteeID() != cInvalidObjectID)
      return false;

   return true;
}

//=============================================================================
//=============================================================================
void BUnit::resetDeflectID()
{
   BUnitActionDeflect* pAction = reinterpret_cast<BUnitActionDeflect*>(getActionByType(BAction::cActionTypeUnitDeflect));
   if (pAction)
   {
      pAction->setDeflecteeID(cInvalidObjectID);
   }
}
//=============================================================================
//=============================================================================
void BUnit::setTrainLock(BPlayerID playerID, bool locked)
{
   BSquad* pSquad = getParentSquad();
   if (pSquad && pSquad->isFrozen())
      return;

   BEntityID parkingLot = getAssociatedParkingLot();
   if (parkingLot != cInvalidObjectID && parkingLot != mID)
   {
      BUnit* pParkingLot = gWorld->getUnit(parkingLot);
      if (pParkingLot)
         pParkingLot->setTrainLock(playerID, locked);
      return;
   }

   if (pSquad && pSquad->getFlagChangingMode())
      return;

   bool wasLocked = false;

//-- FIXING PREFIX BUG ID 4410
   const BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeTrainLock);
//--
   if (pRef)
   {
      wasLocked = true;
      if (!locked)
         removeEntityRefByType(BEntityRef::cTypeTrainLock);
   }
   else
   {
      if (locked)
         addEntityRef(BEntityRef::cTypeTrainLock, cInvalidObjectID, (short)playerID, 0);
   }

   if (locked != wasLocked)
   {
      if (pSquad)
      {
         BSimOrder* pOrder = gSimOrderManager.createOrder();
         if (pOrder)
         {
            pOrder->setOwnerID(pOrder->getOwnerID());
            pOrder->setPriority(pOrder->getPriority());
            if (locked)
               pOrder->setMode((int8)BSquadAI::cModeLockdown);
            else
               pOrder->setMode((int8)BSquadAI::cModeNormal);
            pSquad->doChangeMode(pOrder, NULL);
         }
      }

      gWorld->notify(BEntity::cEventTrainLock, mID, 0, 0);
   }
}

//=============================================================================
//=============================================================================
bool BUnit::getTrainLock(BPlayerID playerID) const
{
//-- FIXING PREFIX BUG ID 4412
   const BSquad* pSquad = getParentSquad();
//--
   if(pSquad && (pSquad->getSquadMode() == BSquadAI::cModeLockdown) && (getProtoObject() != NULL) && (getProtoObject()->getObjectClass() == cObjectClassBuilding))
      return true;

   BEntityID parkingLot = getAssociatedParkingLot();
   if (parkingLot != cInvalidObjectID && parkingLot != mID)
   {
//-- FIXING PREFIX BUG ID 4411
      const BUnit* pParkingLot = gWorld->getUnit(parkingLot);
//--
      if (pParkingLot)
         return pParkingLot->getTrainLock(playerID);
      return false;
   }

   if (pSquad && pSquad->getSquadMode()!=BSquadAI::cModeLockdown)
      return false;

   const BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeTrainLock);
   if (pRef)
      return true;
   else
      return false;
}

//==============================================================================
//==============================================================================
void BUnit::queueTrainedSquad(BSquad* pSquad, bool playSound)
{
   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      pAction->queueTrainedSquad(pSquad, playSound);
}

//=============================================================================
//=============================================================================
BSquad* BUnit::getParentSquad() const
{
   if (mpCachedParentSquad && mpCachedParentSquad->getID() == mParentID) 
      return mpCachedParentSquad;
   return NULL;
}

//=============================================================================
//=============================================================================
void BUnit::refreshParentSquad()
{
   mpCachedParentSquad = gWorld->getSquad(mParentID);
}

//=============================================================================
//=============================================================================
BEntityID BUnit::getBuiltByUnitID(void) const
{
   const BEntityRef* pRef=getFirstEntityRefByType(BEntityRef::cTypeBuiltByUnit);
   if (pRef)
   {
      BEntityID id(pRef->mID);
      return id;
   }
   else
      return cInvalidObjectID;
}

//==============================================================================
// BUnit::carryObject
//==============================================================================
bool BUnit::carryObject(BEntityID id, long fromBoneHandle, const BVector* const worldSpaceOffset)
{
   // destroy carried object if there is one
   if (mCarriedObject.isValid())
      destroyCarriedObject();

   // get the object we're going to attach
   BObject* pAttachObject = gWorld->getObject(id);

   // if we have no object, fail
   if (!pAttachObject)
      return false;

   // attach the new object
//-- FIXING PREFIX BUG ID 4414
   const BVisual* pTargetVisual = getVisual();
//--
   BASSERT(pTargetVisual);

   // get the carry bone handle on our mesh
   long pointHandle = pTargetVisual->getPointHandle(cActionAnimationTrack, cVisualPointCarry);
   const BProtoVisualPoint* carryPointData = pTargetVisual->getPointProto(cActionAnimationTrack, pointHandle);
   BASSERT(carryPointData);
   long toBoneHandle = carryPointData->mBoneHandle;
   BASSERT(toBoneHandle != -1);

   // get the carry bone's location
   BMatrix worldMatrix;
   BVector attachPos;
   getWorldMatrix(worldMatrix);
   pTargetVisual->getBone(toBoneHandle, &attachPos, NULL, NULL, &worldMatrix, false);                        

   // set the attach object to the desired attach position
   if (worldSpaceOffset && fromBoneHandle == -1)
   {
      BMatrix matrix;
      BVector modelSpaceOffset;
      pAttachObject->getInvWorldMatrix(matrix);
      matrix.transformVectorAsPoint((pAttachObject->getPosition() + *worldSpaceOffset), modelSpaceOffset);
      pAttachObject->setCenterOffset(-modelSpaceOffset);
   }

   #ifdef SYNC_Unit
      if (pAttachObject->isClassType(BEntity::cClassTypeUnit))
         syncUnitData("BUnit::carryObject", attachPos);
   #endif
   pAttachObject->setPosition(attachPos, true);

   // do the attachment
   if (fromBoneHandle != -1)
      attachObject(pAttachObject->getID(), toBoneHandle, fromBoneHandle);
   else
      attachObject(pAttachObject->getID(), toBoneHandle, -1, false);

   // set the carried object and fire off a notify that it was picked up
   mCarriedObject = pAttachObject->getID();
   pAttachObject->notify(cEventPickedUp, getID(), 0, 0);

   // this totally sucks, setting a squad level concept at the unit level, 
   // but the squad mode is currently the only place to hold state for movement modifiers
   // this is especially hacky because we're looking up the ability movement modifier here... :'(
   BSquad* pParentSquad = getParentSquad();
   if (pParentSquad && pParentSquad->getSquadAI())
   {
      int abilityID = getProtoObject()->getAbilityCommand();
      if (abilityID != -1)
      {
//-- FIXING PREFIX BUG ID 4413
         const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--
         if (pAbility && pAbility->getMovementModifierType() == BAbility::cMovementModifierMode)
               pParentSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, pAbility->getMovementSpeedModifier(), false);
      }
      pParentSquad->getSquadAI()->setMode(BSquadAI::cModeCarryingObject);
   }

   return true;
}

//==============================================================================
// BUnit::destroyCarriedObject
//==============================================================================
void BUnit::destroyCarriedObject()
{
   if (mCarriedObject.isValid())
   {
      unattachObject(mCarriedObject);
      BEntity* pAttachEntity = gWorld->getEntity(mCarriedObject);
      if (pAttachEntity)
         pAttachEntity->kill(true);
      mCarriedObject.invalidate();

      // this totally sucks, setting a squad level concept at the unit level, 
      // but the squad mode is currently the only place to hold state for movement modifiers
      BSquad* pParentSquad = getParentSquad();
      if (pParentSquad && pParentSquad->getSquadAI())
         pParentSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);
   }
}

//==============================================================================
// BUnit::doTrain
//==============================================================================
bool BUnit::doTrain(BPlayerID playerID, long id, long count, bool squad, bool noCost, BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID, bool forcePlaySound)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doTrain mID", mID.asLong());
      syncUnitActionData("BUnit::doTrain train id", id);
      syncUnitActionData("BUnit::doTrain count", count);
      syncUnitActionData("BUnit::doTrain squad", squad);
      syncUnitActionData("BUnit::doTrain noCost", noCost);
   #endif

   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(!pAction)
      return false;
   pAction->addTrain(playerID, id, count, squad, noCost, triggerScriptID, triggerVarID, forcePlaySound);
   return true;
}

//==============================================================================
// BUnit::getTrainCount
//==============================================================================
long BUnit::getTrainCount(BPlayerID playerID, long id) const
{
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      return pAction->getTrainCount(playerID, id);
   return 0;
}

//==============================================================================
// BUnit::getTrainCount
//==============================================================================
void BUnit::getTrainQueue(BPlayerID playerID, long*  pCount, float*  pTotalPoints) const
{
   *pCount = 0;
   *pTotalPoints = 0;
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      pAction->getTrainQueue(playerID, pCount, pTotalPoints);
}
//==============================================================================
// BUnit::doResearch
//==============================================================================
bool BUnit::doResearch(BPlayerID playerID, long techID, long count, bool noCost, BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doResearch mID", mID.asLong());
      syncUnitActionData("BUnit::doResearch playerID", playerID);
      syncUnitActionData("BUnit::doResearch techID", techID);
      syncUnitActionData("BUnit::doResearch count", count);
   #endif

   if(count<0)
   {
      BPlayer* pPlayer=gWorld->getPlayer(playerID);
      long buildingID=pPlayer->getTechTree()->getResearchBuilding(techID, mID.asLong());
      if(buildingID!=-1)
      {
         BUnit* pBuilding=gWorld->getUnit(buildingID);
         if(pBuilding)
         {
            BUnitActionBuilding* pAction=(BUnitActionBuilding*)pBuilding->getActionByType(BAction::cActionTypeUnitBuilding);
            if(pAction)
            {
               pAction->addResearch(playerID, techID, count, noCost, triggerScriptID, triggerVarID);
               return true;
            }
         }
      }
      return false;
   }

   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(!pAction)
      return false;

   pAction->addResearch(playerID, techID, count, noCost, triggerScriptID, triggerVarID);
   return true;
}

//==============================================================================
// BUnit::getResearchCount
//==============================================================================
long BUnit::getResearchCount(BPlayerID playerID, long id) const
{
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      return pAction->getResearchCount(playerID, id);
   return 0;
}

//==============================================================================
// BUnit::doBuild
//==============================================================================
bool BUnit::doBuild(BPlayerID playerID, bool cancel, bool noCost, bool fromSave)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doBuild mID", mID.asLong());
      syncUnitActionData("BUnit::doBuild playerID", playerID);
      syncUnitActionData("BUnit::doBuild cancel", cancel);
   #endif

   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
   {
      pAction->doBuild(playerID, cancel, noCost, fromSave);
      return true;
   }
   return false;
}

//==============================================================================
// BUnit::getBuildPercent
//==============================================================================
float BUnit::getBuildPercent() const
{
   if(getFlagBuilt())
      return 1.0f;
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(!pAction)
      return 0.0f;
   return pAction->getBuildPercent();
}

//==============================================================================
// BUnit::addBuildPoints
//==============================================================================
void BUnit::addBuildPoints(float points)
{
   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      pAction->addBuildPoints(points);
}

//==============================================================================
// BUnit::forceDopple
//==============================================================================
void BUnit::forceDoppleAllTeams()
{
   // why do we start with 1? I guess we never need to dopple to gaia
   for (int i = 1; i < gWorld->getNumberTeams(); i++)
   {
      // if this is our own team, do nothing
      if (i == getTeamID())
         continue;

      assertVisibility();
      makeSoftDopple(i);   
      assertVisibility();

      // call force dopple on all our child objects as well
      int numEntityRefs = getNumberEntityRefs();
      for (int entityRefIndex = numEntityRefs - 1; entityRefIndex >= 0; entityRefIndex-- )
      {
         BEntityRef* pEntityRef = getEntityRefByIndex(entityRefIndex);
         if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeAssociatedSocket || pEntityRef->mType == BEntityRef::cTypeAssociatedFoundation))
         {
            BUnit* pChildUnit = gWorld->getUnit(pEntityRef->mID);
            if (pChildUnit)
               pChildUnit->forceDoppleAllTeams();
         }
      }      
   }
}

//==============================================================================
// BUnit::doBuildOther
//==============================================================================
bool BUnit::doBuildOther(BPlayerID playerID, BPlayerID purchasingPlayerID, int id, bool cancel, bool noCost, bool doppleOnStart, BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doBuildOther mID", mID.asLong());
      syncUnitActionData("BUnit::doBuildOther playerID", playerID);
      syncUnitActionData("BUnit::doBuildOther id", id);
      syncUnitActionData("BUnit::doBuildOther cancel", cancel);
   #endif
   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
   {
      pAction->doBuildOther(playerID, purchasingPlayerID, id, cancel, noCost, doppleOnStart, triggerScriptID, triggerVarID);
      return true;
   }
   return false;
}


//==============================================================================
// BUnit::doCustomCommand
//==============================================================================
bool BUnit::doCustomCommand(BPlayerID playerID, int commandID, bool cancel)
{
   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnit::doCustomCommand mID", mID.asLong());
      syncUnitActionData("BUnit::doCustomCommand playerID", playerID);
      syncUnitActionData("BUnit::doCustomCommand commandID", commandID);
      syncUnitActionData("BUnit::doCustomCommand cancel", cancel);
   #endif
   BUnitActionBuilding* pAction=(BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if (!pAction)
   {
      pAction = (BUnitActionBuilding*)gActionManager.createAction(BAction::cActionTypeUnitBuilding);
      if (!pAction)
         return false;
      addAction(pAction);
      setFlagNoUpdate(false);
   }
   pAction->doCustomCommand(playerID, commandID, cancel);
   return true;
}

//==============================================================================
// BUnit::getCustomCommandCount
//==============================================================================
int BUnit::getCustomCommandCount(BPlayerID playerID, int commandID) const
{
   const BUnitActionBuilding* pAction=(const BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if(pAction)
      return pAction->getCustomCommandCount(playerID, commandID);
   return 0;
}

//==============================================================================
// BUnit::init
//==============================================================================
void BUnit::init( void )
{
   // jce [9/29/2008] -- entity's init should be setting this up, but defaulting it to zero here first just to be paranoid
   mLastMoveTime = 0;

   //-- call the base func
   BObject::init(); 

   clearFlags();

   // Flags
   mFlagAlive=true;
   mFlagDiesAtZeroHP=true;
   mFlagDieAtZeroResources=false;
   mFlagUnlimitedResources=false;
   mFlagRallyPoint=false;   
   mFlagRallyPoint2=false;   
   mFlagHasHPBar=false;
   mFlagDisplayHP=false;
   mFlagForceDisplayHP=false;   
   mFlagHasShield=false;
   mFlagFullShield=false;
   mFlagUsesAmmo=false;
   mFlagHasGarrisoned=false;
   mFlagHasAttached=false;
   mFlagAttackBlocked=false;
   mFlagHasGoalVector=false;
   mFlagHasFacingCommand=false;
   mFlagIgnoreUserInput=false;         
   mFlagFirstBuilt=true;
   mFlagInfected=false;
   mFlagTakeInfectionForm=false;
   mFlagFloodControl=false;
   mFlagHasHitched = false;
   mFlagReverseMove = false;
   mFlagReverseMoveHasMoved = false;
   mFlagBuildingConstructionPaused = false;
   mFlagBuildOtherQueue = false;
   mFlagBuildOtherQueue2 = false;
   mFlagPreserveDPS = false;
   mFlagDoingFatality = false;
   mFlagFatalityVictim = false;
   mFlagDeathReplacementHealing = false;
   mFlagForceUpdateContainedUnits = false;
   mFlagRecycled = false;
   mFlagDown = false;
   mFlagNoCorpse = false;
   mFlagNotAttackable = false;
   mFlagSelectTypeTarget = false;
   mFlagBlockContain = false;
   mFlagIsTypeGarrison = false;
   mFlagIsTypeCover = false;
   mFlagParentSquadChangingMode = false;
   mFlagShatterOnDeath = false;
   mFlagAllowStickyCam = true;
   mFlagBeingBoarded = false;
   mFlagAddResourceEnabled = false;
   mFlagIsHibernating = false;
   mFlagUnhittable = false;
   mFlagDestroyedByBaseDestruction = false;
   mFlagKilledByLeaderPower = false;

   // Base Flags Manipulation
   setFlagVisibility(true);
   setFlagLOS(true);   

   mpCachedParentSquad = NULL;
   mMaxHPContained = 0.0f;
   mRallyPoint.init();
   mRallyPoint2.init();
   mShieldpoints = 0.0f;
   mHitpoints = 0.0f;
   mKilledByID = cInvalidObjectID;
   mKilledByWeaponType = -1;
   mRallyObject = cInvalidObjectID;
   mRallyObject2 = cInvalidObjectID;
   mCarriedObject = cInvalidObjectID;
   mMultiTargets = BEntityIDArray(0, 10);
   mResourceAmount = 0.0f;
   mGatherers = 0;
   //mFlippedTime = 0.0f;
   mDamageModifier = 1.0f;
   mVelocityScalar = 1.0f;
   mAccuracyScalar = 1.0f;
   mWorkRateScalar = 1.0f;
   mWeaponRangeScalar = 1.0f;
   mDamageTakenScalar = 1.0f;
   mDodgeScalar = 1.0f;
   mDamageTracker = NULL;
   mParentID = cInvalidObjectID;
   mAmmunition = 0.0f;
   mCapturePoints = 0.0f;
   mCapturePlayerID = -1;
   mAttackWaitTimer.clear();
   mCurrentTurnRate = 0.0f;
   mGoalVector.zero();
   //mPosHistory[0].zero();
   //mPosHistory[1].zero();
   mBattleID = -1;
   mSelectionDecal = -1;
   mpHitZoneList = NULL;
   mAttackActionRefCount = 0;
   mLeashTimer=0;
   mpGroupHull = NULL;
   mGroupDiagSpan = 0.0f;
   mGarrisonTime = 0.0f;
   mLastDPSRampValue = 0.0f;
   mShieldRegenRate = 1.0f;
   mShieldRegenDelay = 1.0f;
   mBaseNumber = -1;

   //Init the controllers.
   for (uint i=0; i < BActionController::cNumberControllers; i++)
      mControllers[i].init();

#if DPS_TRACKER
   clearDamageHistory();
   mDmgHistoryIndex = 0;   
   mDmgDealtBuffer = 0.0f;
   mDmgUpdateTimer = 0;
#endif
   
   //Clear our opps.
   mOpps.clear();
   
   //Init the visual ammo.
   mVisualAmmo.clear();
   
   clearTacticState();

   mInfectionPlayerID=0;
   mFormerParentSquad=cInvalidObjectID;

   mContainedPop = 0.0f;

   mMultiTargets.clear();

#ifndef BUILD_FINAL
   //////////////////////////////////////////////////////////////////////////
   // SLB: temporary sync debugging stuff
   mUnitDamageTemplateID = -1;
   mVisualDamageTemplateID = -1;
   //////////////////////////////////////////////////////////////////////////
#endif
}

//==============================================================================
//==============================================================================
void BUnit::onRelease()
{
   mOpps.clear();
   mVisualAmmo.clear();
   mMultiTargets.clear();
   if (mpHitZoneList)
   {
      delete mpHitZoneList;
      mpHitZoneList=NULL;
   }

   if (mDamageTracker)
   {
      delete mDamageTracker;
      mDamageTracker=NULL;
   }

   if (mSelectionDecal != -1)
      gDecalManager.destroyDecal(mSelectionDecal);
   mSelectionDecal = -1;

   if (mpGroupHull)
      delete mpGroupHull;
   mpGroupHull=NULL;

   mpCachedParentSquad = NULL;

   BObject::onRelease();
}

//==============================================================================
// BUnit::initFromProtoObject
//==============================================================================
bool BUnit::initFromProtoObject(const BProtoObject* pProto, BObjectCreateParms& parms)
{
   // Store some information about ourselves... on ourselves...
   if (parms.mBuiltByUnitID!=cInvalidObjectID)
   {
      // Remember the unit that built us
      addEntityRef(BEntityRef::cTypeBuiltByUnit, parms.mBuiltByUnitID, 0, 0);

      // Setup train limit references
      BUnit* pBuilder=gWorld->getUnit(parms.mBuiltByUnitID);
      if (pBuilder)
      {
         uint8 trainLimitBucket=0;
         if (pBuilder->getProtoObject()->getTrainLimit(pProto->getID(), false, &trainLimitBucket)!=-1)
         {
            addEntityRef(BEntityRef::cTypeTrainLimitBuilding, pBuilder->getID(), 0, 0);
            pBuilder->addEntityRef(BEntityRef::cTypeTrainLimitUnit, mID, pProto->getID(), trainLimitBucket);
         }
      }
   }

   // If this is a base or settlement, add those references right now.
   bool creatingSettlement = pProto->isType(gDatabase.getOTIDSettlement());
   bool creatingBase = pProto->isType(gDatabase.getOTIDBase());

   if (creatingSettlement)
      setAssociatedSettlement(mID);
   if (creatingBase)
      setAssociatedBase(mID);

   // We're building on some kind of socket (could be settlement or normal socket.)
   if (parms.mSocketUnitID != cInvalidObjectID)
   {
      // Add socket refs
      BUnit* pParentSocket = gWorld->getUnit(parms.mSocketUnitID);
      if (pParentSocket)
      {
         // SLB: Dopple the parent socket if we need to.
         pParentSocket->computeDopple();

         // ajl 9/26/08 - phx-10261 - moved setting of no render flag to unitactionbuilding.cpp to prevent
         // flicker of socket going away before new building is rendered.
         // turn off the parent socket rendering at this point.
         //pParentSocket->setFlagNoRender(true);

         // create the parent/child socket/plug relationship
         // Note that both normal buildings (barracks) and normal sockets have this relationship...
         // but so do the settlement (base socket) and base (firebase) pairs...
         setParentSocket(parms.mSocketUnitID);
         pParentSocket->setSocketPlug(mID);          

         BEntityID parkingLot = cInvalidObjectID;
         if (pProto->getFlagSelfParkingLot())
         {
            // Parking lot is ourself.
            parkingLot = mID;
            setAssociatedParkingLot(parkingLot);
            pParentSocket->setAssociatedParkingLot(parkingLot);
         }
         else
         {
            // Carry over the socket's parking lot to this new building
            parkingLot = pParentSocket->getAssociatedParkingLot();
            if (parkingLot != cInvalidObjectID)
            {
               BUnit* pParkingLot = gWorld->getUnit(parkingLot);
               if (pParkingLot)
               {
                  setAssociatedParkingLot(parkingLot);
                  if (creatingBase)
                     pParkingLot->setAssociatedBase(mID);
               }
            }
         }

         BEntityID settlement = pParentSocket->getAssociatedSettlement();
         if (settlement != cInvalidObjectID)
         {
            BUnit* pSettlement = gWorld->getUnit(settlement);
            if (pSettlement)
            {
               if (creatingBase)
               {
                  // Link the settlement to this base.
                  setAssociatedSettlement(settlement);
                  pSettlement->setAssociatedBase(mID);

                  // Copy all existing socket and building references from the settlement to this base.
                  uint numRefs = pSettlement->getNumberEntityRefs();
                  for (uint i=0; i<numRefs; i++)
                  {
//-- FIXING PREFIX BUG ID 4415
                     const BEntityRef* pRef = pSettlement->getEntityRefByIndex(i);
//--
                     if (pRef->mType == BEntityRef::cTypeAssociatedSocket || pRef->mType == BEntityRef::cTypeAssociatedBuilding)
                     {
                        BUnit* pAssocBldgOrSocket = gWorld->getUnit(pRef->mID);
                        if (pAssocBldgOrSocket)
                        {
                           pAssocBldgOrSocket->setAssociatedBase(mID);
                           if (pRef->mType == BEntityRef::cTypeAssociatedSocket)
                              addAssociatedSocket(pRef->mID);
                           else
                              addAssociatedBuilding(pRef->mID);
                        }
                     }
                  }
               }
               else
               {
                  // Establish a relationship between the settlement and the buildings around the base.
                  setAssociatedSettlement(settlement);
                  pSettlement->addAssociatedBuilding(mID);
               }
            }
         }

         if (!creatingBase)
         {
            // Establish a relationship between the base and the buildings around the base.
            BEntityID base = pParentSocket->getAssociatedBase();
            if (base != cInvalidObjectID)
            {
               BUnit* pBaseUnit = gWorld->getUnit(base);
               if (pBaseUnit)
               {
                  setAssociatedBase(base);
                  pBaseUnit->addAssociatedBuilding(mID);
               }
            }
         }
      }
   }

   if(parms.mStartBuilt)
      setFlagBuilt(true);
   // Set our unit KBID then increment the counter.
   //setUnitKBID(gWorld->getNextUnitKBID());
   //gWorld->incrementNextUnitKBID();

   if (!BObject::initFromProtoObject(pProto, parms))
      return (false);

   mShieldpoints = 0.0f; // ajl 8/27/07 - Tim wants shields to start out at 0 and start charging immediately
   mHitpoints = pProto->getHitpoints();
   mResourceAmount = pProto->getResourceAmount();     
   
//-- FIXING PREFIX BUG ID 4417
   const BHitZoneArray* pHitZoneList = ((BProtoObject*)pProto)->getHitZoneList();
//--
   if (pHitZoneList->getNumber() > 0)
   {
      mpHitZoneList = new BHitZoneArray();
      *mpHitZoneList = *pHitZoneList;
      if (mpVisual)
      {
         long hitZoneCount = mpHitZoneList->getNumber();
         for (long i = 0; i < hitZoneCount; i++)
         {
            BHitZone& zone = (*mpHitZoneList)[i];
            zone.setAttachmentHandle(mpVisual->getAttachmentHandle(zone.getAttachmentName()));
         }
      }
   }

   if (pProto->getFlagStartAtMaxAmmo())
      mAmmunition=pProto->getMaxAmmo();
   else
      mAmmunition = 0.0f; //-- Start at 0 per Tim's request.
   if(pProto->getMaxAmmo() > 0.0f)
      setFlagUsesAmmo(true);
   
   setFlagHasHPBar(pProto->getFlagHasHPBar());
   setFlagDopples(pProto->getFlagDopples());
   setFlagAllowStickyCam(pProto->getFlagAllowStickCam());

   // Halwes - 10/24/2008 - If a campaign scenario and we do not want to gray map dopple this unit then override
   BGameSettings* pSettings = gDatabase.getGameSettings();
   BASSERT(pSettings);
   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   BASSERT(result);
   bool grayMapDopples = pProto->getFlagGrayMapDopples();
   if ((gameType == BGameSettings::cGameTypeCampaign) && pProto->getFlagNoGrayMapDoppledInCampaign())
   {
      grayMapDopples = false;
   }   
   setFlagGrayMapDopples(grayMapDopples);

   setFlagDieAtZeroResources(pProto->getFlagDieAtZeroResources());
   setFlagUnlimitedResources(pProto->getFlagUnlimitedResources());
   setFlagForceUpdateContainedUnits(pProto->getFlagForceUpdateContainedUnits());
   setFlagIsTypeGarrison(pProto->isType(gDatabase.getOTIDGarrison()));
   setFlagIsTypeCover(pProto->isType(gDatabase.getOTIDCover()));

   setFlagInvulnerable(pProto->getFlagInvulnerable());
   if (mPlayerID == 0 && pProto->getFlagInvulnerableWhenGaia())
      setFlagInvulnerable(true);

   setFlagHasShield(pProto->getFlagHasShield() || pProto->getFlagIsExternalShield());
   setFlagFullShield(pProto->getFlagFullShield());

   mMaxTurnRate = pProto->getTurnRate();

   BPlayer* pPlayer = getPlayer();

   // Create any needed unique tech tree nodes
   uint commandCount=pProto->getNumberCommands();
   if(commandCount>0)
   {
//-- FIXING PREFIX BUG ID 4416
      const BTeam* pTeam=pPlayer->getTeam();
//--
      int playerCount=pTeam->getNumberPlayers();
      long unitID=mID.asLong();
      for(uint i=0; i<commandCount; i++)
      {
         BProtoObjectCommand command=pProto->getCommand(i);
         if(command.getType()==BProtoObjectCommand::cTypeResearch)
         {
            if (gWorld->getFlagCoop() && pPlayer->isHuman())
            {
               for (int j=0; j<playerCount; j++)
                  gWorld->getPlayer(pTeam->getPlayerID(j))->getTechTree()->addUnitRef(command.getID(), unitID);
            }
            else
               pPlayer->getTechTree()->addUnitRef(command.getID(), unitID);
         }
      }
   }

   if(pProto->getFlagBuildingCommands() || pProto->getFlagBuild() || pProto->getFlagUseBuildingAction())
   {
      // Auto create the persistent building action
      BUnitActionBuilding* pAction=(BUnitActionBuilding*)gActionManager.createAction(BAction::cActionTypeUnitBuilding);
      if(pAction)
      {
         addAction(pAction);
      }
   }

   if(getFlagUsesAmmo())
   {
      // Auto create the persistent ammo regen action
      BUnitActionAmmoRegen* pAction=(BUnitActionAmmoRegen*)gActionManager.createAction(BAction::cActionTypeUnitAmmoRegen);
      if(pAction)
      {
         addAction(pAction);
      }
   }

   // Ability trigger scripts.  Probably broken on transform.  Fix it.
   uint numAbilityTriggerScripts = pProto->getNumAbilityTriggerScripts();
   for (uint i=0; i<numAbilityTriggerScripts; i++)
   {
      BTriggerScriptID newTriggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, pProto->getAbilityTriggerScript(i));
      gTriggerManager.addExternalUnitID(newTriggerScriptID, getID());
      gTriggerManager.activateTriggerScript(newTriggerScriptID);
   }

   // Adjust unit count
   pPlayer->addUnitToProtoObject(this, mProtoID);
      

   pPlayer->getTechTree()->checkUnitPrereq(mProtoID); // THIS MODIFIES STUFF

   // Goto base tracking
   if (pProto->getGotoType()==cGotoTypeBase)
      pPlayer->addGotoBase(mID);

   if(pProto->getFlagBuild())
   {
      #ifdef SYNC_Unit
         syncUnitData("BUnit::initFromProtoObject mID", mID.asLong());
      #endif
      doBuild(parms.mCreatedByPlayerID, false, parms.mNoCost, false);
   }
   else
      onBuilt(false, parms.mPhysicsReplacement, parms.mStartExistSound, false);

   if (pProto->getFlagTrackPlacement())
      BPlacementRules::getUnitsToTrack().add(mID);

   if (pProto->getSelectType() == cSelectTypeUnit && pProto->getFlagUIDecal())
   {
      mSelectionDecal = gDecalManager.createDecal();
   }   

   //Init the visual ammo.
   initVisualAmmo();

   // Init attackWaitTimers
//-- FIXING PREFIX BUG ID 4418
   const BTactic* pTactic = getTactic();
//--
   if (pTactic != NULL)
   {
      int weaponIndex;
      for (weaponIndex = 0; weaponIndex < pTactic->getNumberWeapons(); weaponIndex++)
      {
         mAttackWaitTimer.add(BAttackTimer());
      }
   }

   // Thrown objects don't die and don't reveal LOS
   if (getProtoID() == gDatabase.getPOIDPhysicsThrownObject())
   {
      setFlagDiesAtZeroHP(false);
      setFlagLOS(false);
   }

   mGarrisonTime = pProto->getGarrisonTime() + 1.0f; // Ensure that we always start as garrisonable


   //E3.
   //BTactic* pTactic=getTactic();
   //if (pTactic && (pTactic->getNumberStates() >= 2))
   //   mTacticState=getRand(cSimRand)%2;

   if (parms.mLevel > 0)
      upgradeLevel(0, parms.mLevel, false, NULL);

   // Add in AI player damage modifiers
   adjustDamageModifier(pPlayer->getAIDamageMultiplier());
   adjustDamageTakenScalar(pPlayer->getAIDamageTakenMultiplier());

   // Clear the area of corpses and debris
   if (mFlagAlive && !mFlagFlying && mpObstructionNode)
      updateCorpseCollisions();

   BASSERTM(!(parms.mPhysicsReplacement ^ mFlagIsPhysicsReplacement), "parms.mPhysicsReplacement was set to true, but mFlagIsPhysicsReplacement is still false!");

   return (true);
}

//==============================================================================
//==============================================================================
void BUnit::createPersistentActions()
{
   const BProtoObject* pProtoObject = getProtoObject();
   BTactic* pTactic = pProtoObject->getTactic();
   if (pTactic)
   {
      for (long i = 0; i < pTactic->getNumberPersistentActions(); i++)
      {         
//-- FIXING PREFIX BUG ID 4539
         const BProtoAction* pProtoAction = pTactic->getPersistentAction(i);
//--
         if(!pProtoAction)
            continue;

         //-- See if there is a persistent action type specified. 
         //-- (Right now this is only used for secondary turret attack weirdness, 
         //-- since the secondary turret attack is persistent, but has the same 
         //-- protoaction as the primary attack)         
         BActionType actionType = pProtoAction->getPersistentActionType();
         if(actionType == BAction::cActionTypeInvalid)
            actionType = pProtoAction->getActionType();

         // TRB 11/17/08 - Tower wall action sets itself to not be destroyed when persistent actions
         // are removed.  So check whether it exists here before trying to create it again.
         if (actionType == BAction::cActionTypeUnitTowerWall)
         {
            BAction* pTowerWallAction = getActionByType(actionType);
            if (pTowerWallAction)
            {
               // Fixup proto action pointer
               pTowerWallAction->setProtoAction(pProtoAction);
               continue;
            }
         }

         BAction* pAction = NULL;
         pAction = gActionManager.createAction(actionType);
         if(pAction)
         {
            pAction->setProtoAction(pProtoAction);
            //pAction->setFlagAuto(true);
            pAction->setFlagPersistent(true);
            pAction->setFlagFromTactic(true);
            addAction(pAction);
         }
      }

      // Recreate any squad persistent actions--we have to do this from all children, since we removed them from all children
      if (getParentSquad())
      {
         BSquad* pSquad = getParentSquad();
         uint count = pSquad->getNumberChildren();

         for (uint i = 0; i < count; ++i)
         {
            BUnit* pChild = gWorld->getUnit(pSquad->getChild(i));

            if (pChild)
               getParentSquad()->addPersistentSquadActions(pChild);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnit::canJump() const
{
   if (getProtoObject()->getPhysicsInfoID() < 0)
      return (false);

//-- FIXING PREFIX BUG ID 4419
   const BPhysicsInfo* pInfo=gPhysicsInfoManager.get(getProtoObject()->getPhysicsInfoID(), true);
//--
   if (pInfo->getVehicleType() == BPhysicsInfo::cWarthog)
      return (true);

   return (false);
}

//==============================================================================
// BUnit::isAlive
//==============================================================================
//DCP: Moved to header.
//bool BUnit::isAlive(void) const
//{
//   return (getFlagAlive());  
//}

//==============================================================================
// BUnit::isDamaged
//==============================================================================
bool BUnit::isDamaged(void) const
{
   const BProtoObject* pProtoObject=getProtoObject();
   if(mHitpoints<pProtoObject->getHitpoints() || mShieldpoints<pProtoObject->getShieldpoints())
      return true;
   else
      return false;
}

//==============================================================================
//==============================================================================
//DCP: Moved to header.
//bool BUnit::isBuilding() const
//{
//   return (false);
//}

//==============================================================================
// BUnit::isCapturing
//==============================================================================
bool BUnit::isCapturing() const
{
   const BAction *pAction = mActions.getActionByType(BAction::cActionTypeUnitCapture);
   if (pAction && pAction->getState()==BAction::cStateWorking)
      return (true);
   return (false);
}

//==============================================================================
// BUnit::isGathering
//==============================================================================
bool BUnit::isGathering() const
{
   const BAction *pAction = mActions.getActionByType(BAction::cActionTypeUnitGather);
   if (pAction && pAction->getState()==BAction::cStateWorking)
      return (true);
   return (false);
}

//==============================================================================
// BUnit::isWorking
//==============================================================================
bool BUnit::isWorking() const
{
   if (isBuilding() || isCapturing() || isGathering())
      return true;
   else
      return false;
}

//==============================================================================
// BUnit::isVisibleOnScreen
//==============================================================================
bool BUnit::isVisibleOnScreen() const
{
   const BBoundingBox* pBoundingBox = getVisualBoundingBox();
   return gRenderDraw.getMainActiveVolumeCuller().isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius());
}

//==============================================================================
//==============================================================================
bool BUnit::haveRallyPoint(BPlayerID plyr) const
{ 
   BEntityID parkingLot = getAssociatedParkingLot();
   if (parkingLot != cInvalidObjectID && parkingLot != mID)
   {
//-- FIXING PREFIX BUG ID 4421
      const BUnit* pParkingLot = gWorld->getUnit(parkingLot);
//--
      if (pParkingLot)
         return pParkingLot->haveRallyPoint(plyr);
      return false;
   }

   if ((plyr == getPlayerID()) || (plyr == -1))
      return getFlagRallyPoint();
   else
      return getFlagRallyPoint2();
}

//==============================================================================
//==============================================================================
BVector BUnit::getRallyPoint(BPlayerID plyr) const
{
   BEntityID parkingLot = getAssociatedParkingLot();
   if (parkingLot != cInvalidObjectID && parkingLot != mID)
   {
//-- FIXING PREFIX BUG ID 4422
      const BUnit* pParkingLot = gWorld->getUnit(parkingLot);
//--
      if (pParkingLot)
         return pParkingLot->getRallyPoint(plyr);
      return cOriginVector;
   }

   if ((plyr == getPlayerID()) || (plyr == -1))
   {
      // Rally point attached to an entity
      if (mRallyPoint.mEntityID != cInvalidObjectID)
      {
//-- FIXING PREFIX BUG ID 4423
         const BEntity* pEntity = gWorld->getEntity(mRallyPoint.mEntityID);
//--
         if (pEntity)
            return (pEntity->getPosition());
      }

      BVector rallyPointPos(mRallyPoint.x, mRallyPoint.y, mRallyPoint.z);
      return rallyPointPos;
   }
   else // Get the Co-op player's rally point
   {
      // Rally point attached to an entity
      if (mRallyPoint2.mEntityID != cInvalidObjectID)
      {
//-- FIXING PREFIX BUG ID 4424
         const BEntity* pEntity = gWorld->getEntity(mRallyPoint2.mEntityID);
//--
         if (pEntity)
            return (pEntity->getPosition());
      }

      BVector rallyPointPos(mRallyPoint2.x, mRallyPoint2.y, mRallyPoint2.z);
      return rallyPointPos;
   }
}

//==============================================================================
//==============================================================================
void BUnit::getRallyPoint(BVector& rallyPoint, BEntityID& rallyPointEntityID, BPlayerID plyr)
{
   if ((plyr == getPlayerID()) || (plyr == -1))
   {
      rallyPoint.set(mRallyPoint.x, mRallyPoint.y, mRallyPoint.z);
      rallyPointEntityID = mRallyPoint.mEntityID;
   }
   else
   {
      rallyPoint.set(mRallyPoint2.x, mRallyPoint2.y, mRallyPoint2.z);
      rallyPointEntityID = mRallyPoint2.mEntityID;
   }
}

//==============================================================================
// [8/27/2008 JRuediger] This will also set the Rally point via setRallyPoint()
//==============================================================================
void BUnit::calculateDefaultRallyPoint(void)
{
   //BSquad* pParentSquad = getParentSquad();
   //if(pParentSquad == NULL)
      //return;

   //-- Find a point behind the Unit.
   long obstructionQuadTrees = BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeAllCollidableSquads | BObstructionManager::cIsNewTypeBlockLandUnits;
   
   BVector newDirection = mForward;
   newDirection.scale(-1.0f);
   
   BVector newpoint = cOriginVector;
   float rotateValue = 0.0f;
   static BEntityIDArray ignoreList;
   static BPath          tempPath;       // Current High Level Path

   tempPath.reset();
   ignoreList.resize(0);
   ignoreList.add(getID());
   
   for(long i=0; i<8; i++)
   {
      if(rotateValue>0)
         newDirection.rotateXZ(rotateValue);
      
      rotateValue+=cPiOver8;

      newpoint = newDirection;
      newpoint.safeNormalize();
      newpoint.scale(50.0f);
      newpoint.assignSum( newpoint, mPosition );

      // Test exit position
      BOOL bObstructed = gObsManager.testObstructions( newpoint, 1.0f, 1.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, mPlayerID );
      if(bObstructed)
         continue;
      
      // Normal Begin Pathing Call, no entity information..
      if (gPather.beginPathing(-1L, -1L, &gObsManager, &ignoreList, 5.0f, true, true, false, -1, BPather::cLandPath) != BPather::cInitialized)
            continue;

      // -1 for entity id, as we're just running a generic path, with a prescribed
      // radius, to a specific target.
      tempPath.reset();
      long lResult=gPather.findPath(-1L, mPosition, newpoint, 1.0f, &tempPath, BPather::cLongRange, -1);

      gPather.endPathing();

      if(lResult == BPath::cFull)
      {
         setRallyPoint(newpoint, cInvalidObjectID, mPlayerID);
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::setRallyPoint(BVector point, BEntityID entityID, BPlayerID plyr)
{ 
   BEntityID parkingLot = getAssociatedParkingLot();
   if (parkingLot != cInvalidObjectID && parkingLot != mID)
   {
      BUnit* pParkingLot = gWorld->getUnit(parkingLot);
      if (pParkingLot)
         pParkingLot->setRallyPoint(point, entityID, plyr);
      return;
   }

   if ((plyr == mPlayerID) || (plyr == -1))
   {
      mRallyPoint.mEntityID = cInvalidObjectID;

      if (entityID != cInvalidObjectID)
      {
//-- FIXING PREFIX BUG ID 4426
         const BEntity* pEntity = gWorld->getEntity(entityID);
//--
         if (pEntity)
         {
            mRallyPoint.mEntityID = entityID;
            point = pEntity->getPosition();
         }
      }

      mRallyPoint.x = point.x;
      mRallyPoint.y = point.y;
      mRallyPoint.z = point.z;

      setFlagRallyPoint(true);

      BSquad* pSquad=gWorld->getSquad(mRallyObject);
      if (pSquad)
      {
         pSquad->setPosition(point);
         pSquad->settle();
      }
      else
      {
         BObject* pObject=gWorld->getObject(mRallyObject);
         if (pObject)
         {
            pObject->setPosition(point);
         }
         else
         {
            long protoID=getPlayer()->getCiv()->getLocalRallyPointObjectID();
            if(protoID!=-1)
               mRallyObject = gWorld->createEntity(protoID, false, getPlayerID(), point, cZAxisVector, cXAxisVector, true);
         }
      }
   }
   else // Use the Co-op player's rally point
   {
      mRallyPoint2.mEntityID = cInvalidObjectID;

      if (entityID != cInvalidObjectID)
      {
//-- FIXING PREFIX BUG ID 4427
         const BEntity* pEntity = gWorld->getEntity(entityID);
//--
         if (pEntity)
         {
            mRallyPoint2.mEntityID = entityID;
            point = pEntity->getPosition();
         }
      }

      mRallyPoint2.x = point.x;
      mRallyPoint2.y = point.y;
      mRallyPoint2.z = point.z;

      setFlagRallyPoint2(true);

      BSquad* pSquad=gWorld->getSquad(mRallyObject2);
      if (pSquad)
      {
         pSquad->setPosition(point);
         pSquad->settle();
      }
      else
      {
         BObject* pObject=gWorld->getObject(mRallyObject2);
         if (pObject)
         {
            pObject->setPosition(point);
         }
         else
         {
            long protoID=getPlayer()->getCiv()->getLocalRallyPointObjectID();
            if(protoID!=-1)
               mRallyObject2 = gWorld->createEntity(protoID, false, plyr, point, cZAxisVector, cXAxisVector, true);
         }
      }

   }

   // If all existing bases have a rally point set, then clear the global rally point.
   getPlayer()->checkRallyPoint();
}

//==============================================================================
// BUnit::setRallyPointVisible
//==============================================================================
void BUnit::setRallyPointVisible(bool val, BPlayerID plyr)
{
   BEntityID parkingLot = getAssociatedParkingLot();
   if (parkingLot != cInvalidObjectID && parkingLot != mID)
   {
      BUnit* pParkingLot = gWorld->getUnit(parkingLot);
      if (pParkingLot)
         pParkingLot->setRallyPointVisible(val, plyr);
      return;
   }

   if ((plyr == getPlayerID()) || (plyr == -1))
   {
      if(mRallyObject!=cInvalidObjectID)
      {
         BObject* pObject=NULL;
         BSquad* pSquad=gWorld->getSquad(mRallyObject);
         if (pSquad)
            pObject=pSquad->getLeaderUnit();
         else
            pObject=gWorld->getObject(mRallyObject);
         if (pObject)
            pObject->setFlagNoRender(!val);
      }
   }
   else
   {
      if(mRallyObject2!=cInvalidObjectID)
      {
         BObject* pObject=NULL;
         BSquad* pSquad=gWorld->getSquad(mRallyObject2);
         if (pSquad)
            pObject=pSquad->getLeaderUnit();
         else
            pObject=gWorld->getObject(mRallyObject2);
         if (pObject)
         {
            pObject->setPlayerID(plyr);
            pObject->setFlagNoRender(!val);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::clearRallyPoint(BPlayerID plyr)
{ 
   BEntityID parkingLot = getAssociatedParkingLot();
   if (parkingLot != cInvalidObjectID && parkingLot != mID)
   {
      BUnit* pParkingLot = gWorld->getUnit(parkingLot);
      if (pParkingLot)
         pParkingLot->clearRallyPoint(plyr);
      return;
   }

   if ((plyr == getPlayerID()) || (plyr == -1))
   {
      mRallyPoint.init();
      setFlagRallyPoint(false);
      if(mRallyObject!=cInvalidObjectID)
      {
         BSquad* pSquad=gWorld->getSquad(mRallyObject);
         if (pSquad)
            pSquad->kill(true);
         else
         {
            BObject* pObject=gWorld->getObject(mRallyObject);
            if (pObject)
               pObject->kill(true);
         }
         mRallyObject=cInvalidObjectID;
      }
   }
   else
   {
      mRallyPoint2.init();
      setFlagRallyPoint2(false);
      if(mRallyObject2!=cInvalidObjectID)
      {
         BSquad* pSquad=gWorld->getSquad(mRallyObject2);
         if (pSquad)
            pSquad->kill(true);
         else
         {
            BObject* pObject=gWorld->getObject(mRallyObject2);
            if (pObject)
               pObject->kill(true);
         }
         mRallyObject2=cInvalidObjectID;
      }

   }
}


//==============================================================================
// BUnit::damage
//==============================================================================
float BUnit::damage( BDamage &dmg )
{
   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::damage mID", mID.asLong());
      syncUnitDetailData("BUnit::damage mDamage", dmg.mDamage);
      syncUnitDetailData("BUnit::damage mDamageMultiplier", dmg.mDamageMultiplier);
//      syncUnitData("BUnit::damage mShieldDamageMultiplier", dmg.mShieldDamageMultiplier);
      syncUnitDetailData("BUnit::damage mShieldpoints", mShieldpoints);
      syncUnitDetailData("BUnit::damage mHitpoints", mHitpoints);
      syncUnitDetailData("BUnit::damage mAttackerID", dmg.mAttackerID.asLong());
   #endif

   // If our squad has a damage proxy, pass the damage along to it
   if (getParentSquad() && getParentSquad()->hasDamageProxy())
   {
//-- FIXING PREFIX BUG ID 4428
      const BSquad* pDmgProxy = gWorld->getSquad(getParentSquad()->getDamageProxy());
//--
      if (pDmgProxy && pDmgProxy->isAlive() && pDmgProxy->getNumberChildren() > 0)
      {
         BUnit* pUnit = gWorld->getUnit(pDmgProxy->getChild(0));
         BASSERT(pUnit);
         pUnit->damage(dmg);
         return 0.0f;
      }
   }

   // If damage is set to override revive action, check for revive action - and override
   BUnitActionRevive* pReviveAction = reinterpret_cast<BUnitActionRevive*>(getActionByType(BAction::cActionTypeUnitRevive));
   if (pReviveAction)
   {
      if (dmg.mOverrideRevive && (mHitpoints <= 0.0f))
         setFlagDiesAtZeroHP(true);

      pReviveAction->setDamageDealt();
   }

   if (!getFlagAlive())
      return(0.0f);

   if (getFlagDown())
   {
      return (0.0f);
   }

   if ((mHitpoints <= 0.0f) && getFlagDiesAtZeroHP())
      return(0.0f);

   // Route damage to linked socket buildings first (such as extractors on capture points).
   if (mpEntityRefs && getProtoObject()->getFlagDamageLinkedSocketsFirst())
   {
      uint numSocketRefs = getNumberEntityRefs();
      for (uint i=0; i<numSocketRefs; i++)
      {
         const BEntityRef* pSocketRef=getEntityRefByIndex(i);
         if (pSocketRef->mType==BEntityRef::cTypeGatherResource)
         {
            const BUnit* pSocketUnit = gWorld->getUnit(pSocketRef->mID);
            if (pSocketUnit)
            {
               const BEntityRef* pChildRef = pSocketUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug);
               if (pChildRef)
               {
                  BUnit* pChildUnit = gWorld->getUnit(pChildRef->mID);
                  if (pChildUnit && pChildUnit->isAlive())
                     return pChildUnit->damage(dmg);
               }
            }
         }
      }
   }

   // If this unit is specially flagged to be the 'last man standing' in the squad, damage someone else if someone is available.
   if (getProtoObject()->getFlagDieLast())
   {
//-- FIXING PREFIX BUG ID 4429
      const BSquad* pParentSquad = getParentSquad();
//--
      if (pParentSquad)
      {
         //BUnit* pBestSquadmateToDamage = NULL;
         //float bestSquadmateToDamagePercentHP = 1.0f;
         const BEntityIDArray& squadmateIDs = pParentSquad->getChildList();
         BStaticArray<BUnit*, 8> validSquadmates;
         for (uint i=0; i<squadmateIDs.getSize(); i++)
         {
            if (squadmateIDs[i] == mID)
               continue;
            BUnit* pSquadMate = gWorld->getUnit(squadmateIDs[i]);
            if (!pSquadMate)
               continue;
            if (pSquadMate->getProtoObject()->getFlagDieLast())
               continue;
            validSquadmates.add(pSquadMate);
         }         
         if (validSquadmates.getSize() > 0)
         {
            uint unitToDamageIndex = getRandRange(cSimRand, 0, validSquadmates.getSize()-1);
            return (validSquadmates[unitToDamageIndex]->damage(dmg));
         }
      }
   }

   // SLB: Early out if our damage is going to get scaled down to 0.0f anyway and avoid divisions by 0.0f.
   if (mDamageTakenScalar <= cFloatCompareEpsilon)
      return 0.0f;

#if 0 // SLB: Tim doesn't want to units to reveal each other because of accidental damage
   // Add damage entry
   damageBy(dmg.mAttackerID, dmg.mAttackerTeamID);
#endif

   float recDamageMultiplier = 1.0f / dmg.mDamageMultiplier;
//   float recShieldDamageMultiplier = 1.0f / dmg.mShieldDamageMultiplier;

   dmg.mKilled = false;
   if (!getFlagInvulnerable())
   {
      // Has damage been assigned a valid hit zone index
      if( mpHitZoneList && ( dmg.mHitZoneIndex != -1 ) && ( dmg.mHitZoneIndex < mpHitZoneList->getNumber() ) )
      {         
         BHitZone& zone = (*mpHitZoneList)[dmg.mHitZoneIndex];
         float sp = zone.getShieldpoints();                  
         float hp = zone.getHitpoints();
         // Does hit zone have shields
         if( sp > 0.0f )
         {
//            sp -= Math::Min( dmg.mDamage * dmg.mShieldDamageMultiplier, sp );
            sp -= Math::Min( dmg.mDamage * dmg.mDamageMultiplier, sp );
         }
         else
         {            
            hp -= Math::Min( dmg.mDamage * dmg.mDamageMultiplier, hp );
         }

         // Is hit zone destroyed
         if( hp <= 0.0f )
         {
            zone.setTargeted( false );
            zone.setActive( false );
            zone.setHitpoints( 0.0f );
            zone.setShieldpoints( 0.0f );
         }
         else
         {
            zone.setHitpoints( hp );
            zone.setShieldpoints( sp );
         }
      }

      // Halwes 1/26/2007 - For now damage to hit zone is independent of unit's overall damage.
      if (!getFlagBuilt())         
      {
         dmg.mDamageDealt = dmg.mDamage * dmg.mDamageMultiplier * gDatabase.getConstructionDamageMultiplier(); 
         recDamageMultiplier /= gDatabase.getConstructionDamageMultiplier();
      }
      else
      {
         if (getFlagHasShield() && (getFlagFullShield() || !dmg.mDirectional || (dmg.mDirection.dot(mForward) <= 0.0f)))
         {
//            dmg.mShieldDamageDealt = Math::Min(dmg.mDamage * dmg.mShieldDamageMultiplier * mDamageTakenScalar, mShieldpoints);
            dmg.mShieldDamageDealt = Math::Min(dmg.mDamage * dmg.mDamageMultiplier * mDamageTakenScalar, mShieldpoints);
            
            mShieldpoints -= dmg.mShieldDamageDealt;

            //-- If we are going to < 50% shield points, play the shield depleted sound
            if(dmg.mShieldDamageDealt > 0.0f)
            {
               float lowThreshold = getProtoObject()->getShieldpoints() * 0.50;
               if(mShieldpoints <= 0.0f)
                  playShieldSound(cObjectSoundShieldDepleted);
               else if(mShieldpoints <= lowThreshold && (mShieldpoints+dmg.mShieldDamageDealt) >= lowThreshold)
                  playShieldSound(cObjectSoundShieldLow);
            }            

            #ifdef SYNC_UnitDetail
               syncUnitDetailData("BUnit::damage mShieldpoints", mShieldpoints);
            #endif            
//               dmg.mDamageDealt = (dmg.mDamage - (dmg.mShieldDamageDealt * recShieldDamageMultiplier)) * dmg.mDamageMultiplier;
               //dmg.mDamageDealt = (dmg.mDamage - (dmg.mShieldDamageDealt * recDamageMultiplier)) * dmg.mDamageMultiplier;
               float div = dmg.mDamageMultiplier * mDamageTakenScalar;
               BASSERT(div >= cFloatCompareEpsilon); // prevent division by 0
               float d = Math::Max(dmg.mDamage - (dmg.mShieldDamageDealt / div), 0.0f); // floating point precision errors sometime cause this to be negative
               dmg.mDamageDealt = d * dmg.mDamageMultiplier;

               BASSERT(dmg.mDamageDealt >= -cFloatCompareEpsilon);
         }
         else            
            dmg.mDamageDealt = dmg.mDamage * dmg.mDamageMultiplier;
      }
      dmg.mDamageDealt *= mDamageTakenScalar;
      recDamageMultiplier /= mDamageTakenScalar;

      // Cap damage to half a kill if specified
      if (dmg.mHalfKill)
      {
         if ((dmg.mDamageDealt >= mHitpoints) && ((dmg.mHalfKillCutoffFactor <= 0.0f) || (dmg.mDamageDealt < mHitpoints * dmg.mHalfKillCutoffFactor)))
            dmg.mDamageDealt = mHitpoints * 0.5f;
      }

      // Never take more damage than there are hitpoints
      if (dmg.mDamageDealt > mHitpoints)
         dmg.mDamageDealt = mHitpoints;

      BASSERT(dmg.mDamageDealt >= -cFloatCompareEpsilon);

      adjustHitpoints(-dmg.mDamageDealt);

      BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
      if ((mHitpoints <= 1.0f) && !getFlagDiesAtZeroHP() && pUnitAvoidAction)
      {
         setHitpoints(1.0f);
         // No more dodging
         BUnitActionDodge* pDodgeAction = reinterpret_cast<BUnitActionDodge*>(getActionByType(BAction::cActionTypeUnitDodge));
         if (pDodgeAction)
            pDodgeAction->setFlagDestroy(true);

         bool noKamikaze = (pUnitAvoidAction->getProtoAction() && pUnitAvoidAction->getProtoAction()->getDetonateOnDeath());

         if (!noKamikaze && pUnitAvoidAction->findKamikazeTarget(gWorld->getUnit(dmg.mAttackerID)))  // Choose a kamikaze crash target location
         {
            // Shrink LOS
            // TRB 11/19/08 - Don't do this!  The BUnitActionAvoidCollisionAir controlling this might kill the unit before it has a chance
            // to notify the visibility system of this LOS change.  In this case the kill will unexplore using the new visibility and leave
            // a permanent LOS ring where it can see enemy units.
            //setLOSScalar(0.1f);
         }
         else
         {
            // MS 12/4/2008: PHX-18973
            setHitpoints(0.0f);

            setFlagDiesAtZeroHP(true);
         }
      }

#ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigNoDamage))
         adjustHitpoints(dmg.mDamageDealt); 
      if (gConfig.isDefined(cConfigNoShieldDamage))
         mShieldpoints += dmg.mShieldDamageDealt;
#endif


      #ifdef SYNC_UnitDetail
         syncUnitDetailData("BUnit::damage mHitpoints", mHitpoints);
      #endif


      // If we have destruction logic in the visual, it needs to be notified that the unit has
      // taken damage.
      if(mpVisual && mpVisual->getProtoVisual() && mpVisual->getProtoVisual()->getFlag(BProtoVisual::cFlagHasDestructionLogic))
      {
         gWorld->notify(BEntity::cEventBuildPercent, getID(), 0, 0); // SLB: FIXME I mess up animations
      }

      if (gConfig.isDefined(cConfigVeterancy) && gScenario.getFlagAllowVeterancy())
      {
         // Squad XP
         BUnit* pAttackerUnit = gWorld->getUnit(dmg.mAttackerID);
         if (pAttackerUnit)
         {
            float pctDmg = dmg.mDamageDealt / getProtoObject()->getHitpoints();
            if (pctDmg > 1.0f)
               pctDmg = 1.0f;

            float bounty = getProtoObject()->getBounty();
            if (bounty > 0.0f)
            {
               float xp = pctDmg * bounty;
               BSquad* pAttackerSquad = pAttackerUnit->getParentSquad();
               if (pAttackerSquad)
                  pAttackerSquad->addBankXP(xp);
            }
         }
      }
   }

   const BEntity *pAttacker = gWorld->getEntity(dmg.mAttackerID);
   BEntityID attackerID = pAttacker ? dmg.mAttackerID : cInvalidObjectID;   
   if ((mHitpoints <= cFloatCompareEpsilon) && getFlagDiesAtZeroHP())
   {
      setHitpoints(0.0f);
      dmg.mKilled = true;
      mKilledByID = attackerID;
      mKilledByWeaponType = dmg.mWeaponType;
   }

   //-- notify the squad that we have been damaged
   sendEvent(mID, mID, BEntity::cEventDamaged, attackerID.asLong(), dmg.mDamage * dmg.mDamageMultiplier);

   // Bounty
   // XXX this was getting called multiple times from AOE projectiles
   if(dmg.mKilled)
   {
      //// send off the unit killed event because the ::kill() method will not be called if we're about to throw units
      //BPlayerID killerPlayerID = cInvalidPlayerID;
      //long killerProtoID = cInvalidProtoID;

      //if (mKilledByID != cInvalidObjectID)
      //{
      //   BEntity* killerEntity = gWorld->getEntity(mKilledByID.asLong());
      //   if (killerEntity)
      //   {
      //      switch (killerEntity->getClassType())
      //      {
      //      case BEntity::cClassTypeObject:
      //      case BEntity::cClassTypeUnit:
      //         {
      //            BObject* pObject = killerEntity->getObject();
      //            if (pObject)
      //            {
      //               killerPlayerID = pObject->getPlayerID();
      //               killerProtoID = pObject->getProtoID();
      //            }
      //         }
      //         break;
      //      case BEntity::cClassTypeSquad:
      //         {
      //            BSquad* pSquad = killerEntity->getSquad();
      //            if (pSquad)
      //            {
      //               killerPlayerID = pSquad->getPlayerID();
      //               killerProtoID = pSquad->getProtoObjectID();
      //            }
      //         }
      //         break;
      //      }
      //   }
      //}

      //// testing a new event that contains the playerIDs of the unit and the killer
      //// along with the protoids for storage purposes
      //gWorld->notify2(BEntity::cEventKilledUnit, mPlayerID, mProtoID, killerPlayerID, killerProtoID);

      //-- Notify the attacker that he made a kill
      BSquad* pAttackerSquad=NULL;
      if(pAttacker && pAttacker->getUnit())
      {
         pAttackerSquad = pAttacker->getUnit()->getParentSquad();
         if(pAttackerSquad && getParentSquad())
            pAttackerSquad->playChatterSound(cSquadSoundChatterKilledEnemy, getParentSquad()->getProtoID(), false);

         if (pAttackerSquad)
            pAttackerSquad->notify(BEntity::cEventKilledUnit, getID(), NULL, NULL);
      }

      //-- Notify the defender that his squad lost a man
      BSquad* pSquad = getParentSquad();
      if(pSquad && pAttackerSquad)
         pSquad->playChatterSound(cSquadSoundChatterAllyKilled, pAttackerSquad->getProtoID());

      BPlayer *pPlayer = pAttacker ? gWorld->getPlayer(pAttacker->getPlayerID()) : NULL;

      if(pAttacker)
      {
         if(pAttacker->getProjectile() && pAttacker->getProjectile()->getFlagFromLeaderPower()) //catches leader powers: mac blast, carpet bomb
            setFlagKilledByLeaderPower(true);
         else if(pAttacker->getParent() && pAttacker->getParent()->getSquad() && (pAttacker->getParent()->getSquad()->getSquadMode() == BSquadAI::cModePower)) //catches leader powers: cleansing, rage, wave
            setFlagKilledByLeaderPower(true);
      }

      if (pPlayer)
      {
         // Player bounty
         if (pPlayer->getFlagBountyResource())
         {
            long resourceID=pPlayer->getBountyResource();
            if(resourceID!=-1)
            {
               float bounty=getProtoObject()->getBounty();
               if(bounty!=0.0f)
               {
#ifdef SYNC_Player
                  syncPlayerCode("BUnit::damage pPlayer->addResrouce");
#endif
                  pPlayer->addResource(resourceID, bounty, BPlayer::cFlagFromBounty);
                  if(gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() == pPlayer->getID() || (gGame.isSplitScreen() && gUserManager.getUser(BUserManager::cSecondaryUser)->getPlayerID() == pPlayer->getID()))
                     generateFloatingTextForResourceGather(resourceID);
               }
            }
         }
      }

      // [7/7/2008 xemu] make sure we get to see this area during and after the death 
      revealPosition(4.0f);

      
      BObject* pPhysReplacement = NULL;
      if (dmg.mForceThrow)
      {
         BPhysicsInfo *pInfo = gPhysicsInfoManager.get(getProtoObject()->getPhysicsReplacementInfoID(), true);
         if (pInfo && !getFlagDestroy())
         {
            pPhysReplacement = killAndThrowPhysicsReplacement(dmg.mDirection, dmg.mDamagePos, dmg.mPhysReplacementMatchBone);
         }
      }
      else
      {
         //-- Determine if this unit's methane tank is going to explode
         bool rocketOnDeath = getFlagDoingFatality() ? false : getProtoObject()->getFlagRocketOnDeath();
         if(rocketOnDeath)
         {  
            //if (getRandRangeFloat(cSimRand, 0.0f, 1.0f) <= gDatabase.getChanceToRocket()))
            if (gDeathManager.checkDeathChanceSpecial(BDeathSpecial::cDeathSpecialRocket))
            {
               rocketOnDeath = true;            
            }
            else
            {
               rocketOnDeath = false;
            }
         }

         //Infection tracking.
         if (dmg.mpDamageInfo && dmg.mpDamageInfo->getInfection() && !mFlagFatalityVictim && !mFlagDoingFatality)
         {
            BProtoObjectID infectedPOID;
            BProtoSquadID infectedPSID;
            if (gDatabase.getInfectedForm(mProtoID, infectedPOID, infectedPSID))
               infect(pAttacker ? pAttacker->getPlayerID() : 0);
         }

         // [7/2/2008 xemu] Hunter supa-physics destruction 
         if ((dmg.mpDamageInfo != NULL) && (getVisual() != NULL))
         {
            bool physicsExplosion = dmg.mpDamageInfo->getCausePhysicsExplosion();
            uint32 numAttachments = static_cast<uint32>(getVisual()->mAttachments.getNumber());
            
            BObjectTypeID explodeType = dmg.mpDamageInfo->getPhysicsExplosionVictimType();

            if (physicsExplosion && mDamageTracker && (numAttachments > 0) && 
                ((explodeType == -1) || getProtoObject()->isType(explodeType)))
            {
               // [7/2/2008 xemu] create a throw attachment effect for each bone in the target, then execute it
               BDamageActionThrowAttachment throwAction;
               const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(mDamageTracker->getDamageTemplateID());
               if (pDT)
               {
                  long modelIndex = pDT->getModelIndex();
                  BGrannyModel *pGrannyModel = gGrannyManager.getModel(modelIndex, true);
                  if (pGrannyModel)
                  {
                     const static long normalBpID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate("part");
                     const static long iceBpID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate("icePart");
                     long bpID = (getFlagShatterOnDeath()) ? iceBpID : normalBpID;

                     float force;
                     BVector forceDir;
                     calculateDamageForce(dmg, &force, &forceDir);

                     // [7/9/2008 xemu] bonus force on physics explosion
                     force = force * 2.0f;

                     // [7/2/2008 xemu] iterate over the bones in the model
                     BSmallDynamicSimArray<long> boneHandles;
                     pGrannyModel->getBoneHandles(&boneHandles);
                     int numBones = boneHandles.getNumber();
                     int i;
                     for (i=0; i < numBones; i++)
                     {
                        throwAction.reset();
                        throwAction.setBPID(bpID);
                        //long boneHandle = pGrannyModel->getBoneHandle("Bone Tread FL");
                        throwAction.setBoneHandle(boneHandles[i]);


                        throwAction.execute(this, true, NULL, force, true, &forceDir);
                     }

                     // [7/2/2008 xemu] put the special physics-explosion particle on too, but only on the first one
                     if (numBones > 0)
                     {
                        BMatrix worldMatrix;
                        getWorldMatrix(worldMatrix);

                        DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

                        ((BVisualItem*)getVisual())->addAttachment(cVisualAssetParticleSystem,   //visualAssetType
                           0,                            //animationTrack
                           1,                            //animType
                           0.0f,                         //tweenTime
                           dmg.mpDamageInfo->getPhysicsExplosionParticle(), boneHandles[0], 3.0f, false, playerColor, worldMatrix, NULL /*&tranformMatrix*/, true);
                     }
                  }
               }

            }
         }

         // Throw a physics replacement (which destroys the original unit)
         if (!mFlagInfected && !getFlagDoingFatality() && dmg.mpDamageInfo && (dmg.mpDamageInfo->getThrowUnits() || rocketOnDeath))
         {
            float forceScaler = 1.0f;

            // Does the direction of this damage matter for throwing?
            float forceMaxAngle = dmg.mpDamageInfo->getPhysicsForceMaxAngle();
            if(forceMaxAngle > 0.0f) // Yes it does
            {
               // Determine the angle between damage dir and direction of point of impact to target
               BVector direction = dmg.mDirection;
               direction.y = 0;
               direction.normalize();

               BVector dmgPosToTarget = mPosition - dmg.mDamagePos;
               if(dmgPosToTarget.length() >= getObstructionRadiusX() + 0.1f)
               {
                  dmgPosToTarget.normalize();
                  float dot = direction.dot(dmgPosToTarget);
                  float angle = (float) acos( dot );
                  float angleDiff = abs(angle) - forceMaxAngle;
                  if(angleDiff > 0)
                  {
                     //The angle is greater than the specified max angle. So we reduce the force 
                     forceScaler = (1 - (angleDiff / cPi));
                     if(forceScaler < 0.0f) //This should never happen
                        forceScaler = 0.0f;
                  }
               }
            }

            // If this damage info is set to PhysicsLaunchAxial, pass the attacker's velocity direction instead of the point of impact
            BVector throwDirection = dmg.mDamagePos;
            if (dmg.mpDamageInfo->getPhysicsLaunchAxial() && pAttacker)
            {
               throwDirection = pAttacker->getVelocity();
               throwDirection.safeNormalize();
            }

            pPhysReplacement = createAndThrowPhysicsReplacement(dmg.mpDamageInfo, throwDirection, dmg.mDistanceFactor, rocketOnDeath, forceScaler, dmg.mAttackerID);
         }
      }

      // if we want to guarantee a physics replacement and we didn't create one, make one here
      if (getProtoObject()->getFlagPhysicsDetonateOnDeath())
      {
         if (!pPhysReplacement)
         {
            BPhysicsInfo *pInfo = gPhysicsInfoManager.get(getProtoObject()->getPhysicsReplacementInfoID(), true);
            BASSERTM(pInfo, "Object specified to physics detonate on death but doesn't have a replacement info id!");

            // Create replacement object
            pPhysReplacement = createPhysicsReplacement();
            if (pPhysReplacement && pPhysReplacement->getPhysicsObject())
            {
               // Destroy this original unit   
               kill(true);

               // Make sure this object gets updated now that we need to destroy it
               setFlagNoUpdate(false);
            }
         }

         BUnit* pPhysReplacementUnit = (pPhysReplacement) ? pPhysReplacement->getUnit() : NULL;
         if (pPhysReplacementUnit)
         {
            // we need to find a detonate action to use 
            BTactic* pTactic = pPhysReplacementUnit->getTactic();
            if (pTactic)
            {
//-- FIXING PREFIX BUG ID 4433
               const BProtoAction* pDetonateProtoAction = NULL;
//--
               BProtoAction* pTempAction = NULL;
               for (long i = 0; i < pTactic->getNumberProtoActions(); ++i)
               {
                  pTempAction = pTactic->getProtoAction(i);
                  if (pTempAction->getActionType() == BAction::cActionTypeUnitDetonate)
                  {
                     pDetonateProtoAction = pTempAction;
                     break;
                  }
               }

               if (pDetonateProtoAction)
               {
                  BUnitActionDetonate* pUnitActionDetonate = reinterpret_cast<BUnitActionDetonate*>(pPhysReplacementUnit->getActionByType(BAction::cActionTypeUnitDetonate));
                  if (!pUnitActionDetonate)
                  {
                     pUnitActionDetonate = reinterpret_cast<BUnitActionDetonate*>(gActionManager.createAction(BAction::cActionTypeUnitDetonate));
                     BASSERT(pUnitActionDetonate);
                     pUnitActionDetonate->setProtoAction(pDetonateProtoAction);

                     // for the sake of friendly fire, we want to make sure to know who started the damage
                     BEntity* pAttackingEntity = gWorld->getEntity(dmg.mAttackerID);
                     if (pAttackingEntity)
                     {
                        // recursively check to see if we were chain exploded - still need to know who to protect for friendly fire
//-- FIXING PREFIX BUG ID 4432
                        const BUnitActionDetonate* pAttackerActionDetonate = reinterpret_cast<BUnitActionDetonate*>(pAttackingEntity->getActionByType(BAction::cActionTypeUnitDetonate));
//--
                        if (pAttackerActionDetonate)
                           pUnitActionDetonate->setDetonationInstigator(pAttackerActionDetonate->getDetonationInstigator());
                        else
                           pUnitActionDetonate->setDetonationInstigator(pAttackingEntity->getPlayerID());
                     }

                     pPhysReplacementUnit->addAction(pUnitActionDetonate);

                     pPhysReplacement->setFlagSelectable(false);

                     // so our action doesn't vanish underneath us
                     pPhysReplacementUnit->setHitpoints(1.0);
                     pPhysReplacementUnit->setFlagAlive(true);

                     pPhysReplacementUnit->setFlagDopples(false);
                     pPhysReplacementUnit->setFlagGrayMapDopples(false);
                     pPhysReplacementUnit->setFlagNotDoppleFriendly(true);
                  }
               }
            }
         }
      }

      // shatter if we're supposed to
      if (mFlagShatterOnDeath)
         shatter();

      // VAT: 11/07/08
      // if we're a main base and we got killed, we need to kill the base here
      // so that the delay death kicks off before we update the damage tracker on 
      // the next update
      if (isMainBase())
         handleDelayKillBase();
   }

   if(!dmg.mKilled)
   {
      //Halwes - 7/16/2007 - Left over from E3, but "may" be used in some form or another so leaving for reference as per Mike B.
      //// MPB E3 Non death physics impulses on ghost
      //if (gDatabase.getPOIDE3Ghost() == getProtoID())
      //{
      //   if (dmg.mpDamageInfo && dmg.mpDamageInfo->getThrowUnits())
      //   {
      //      if (getPhysicsObject())
      //      {
      //         float forceFactor = dmg.mDistanceFactor + getRandRange(cSimRand, 0.0f, 0.1f);
      //         forceFactor = Math::Clamp(forceFactor, 0.0f, 1.0f);
      //         //float force = dmg.mpDamageInfo->getPhysicsForceMin() + forceFactor * (dmg.mpDamageInfo->getPhysicsForceMax() - dmg.mpDamageInfo->getPhysicsForceMin());
      //         const float minForce = 300.0f;
      //         const float maxForce = 500.0f;
      //         float force = minForce + forceFactor * (maxForce - minForce);

      //         // calculate launch direction using launch angle
      //         // Launch angle is lerped on distance factor then add 0-10%
      //         //float launchFactor = dmg.mDistanceFactor + getRandRange(cSimRand, 0.0f, 0.1f);
      //         //launchFactor = Math::Clamp(launchFactor, 0.0f, 1.0f);
      //         //float launchAngle = dmg.mpDamageInfo->getPhysicsLaunchAngleMin() + launchFactor * (dmg.mpDamageInfo->getPhysicsLaunchAngleMax() - dmg.mpDamageInfo->getPhysicsLaunchAngleMin());
      //         const float hackLaunch = 10.0f;
      //         float launchAngle = DEGREES_TO_RADIANS(hackLaunch);

      //         BVector launchDir = mPosition - dmg.mDamagePos;
      //         launchDir *= cosf(launchAngle);
      //         launchDir.y = sinf(launchAngle);
      //         BVector impulse = force * launchDir;

      //         const float maxLateralOffset = 0.1f;
      //         const float maxVertOffset = 0.8f;
      //         BVector impulseOffset = BVector(getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset),
      //                                         getRandRangeFloat(cSimRand, 0.0f, maxVertOffset),
      //                                         getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset));
      //         BVector impactPoint = getPhysicsObject()->getRigidBody()->getPosition();
      //         impactPoint += impulseOffset;

      //         getPhysicsObject()->applyPointImpulse(impulse, impactPoint);
      //      }
      //   }
      //}


      float rand = getRandRangeFloat(cUnsyncedRand, 0.0f, 1.0f);
      if(dmg.mIsDOTDamage == false && rand < gWorld->getWorldSoundManager()->getPainRate())
      {
         //-- Play the pain world sound
         const BProtoObject* pProto = getProtoObject();
         if(pProto)
         {
             BCueIndex index = pProto->getSound(cObjectSoundPain);
             gWorld->getWorldSoundManager()->addSound(this, -1, index, true, cInvalidCueIndex, true, true);
         }         
      }
   }

   // Damage contained units
   if (getFlagHasGarrisoned() && getProtoObject()->getFlagDamageGarrisoned())
   {
      int numEntityRefs = getNumberEntityRefs();
      if (numEntityRefs > 0)
      {
         BDamage unitDamage;
         unitDamage.mAttackerID = dmg.mAttackerID;
         unitDamage.mDamage = dmg.mDamage * gDatabase.getGarrisonDamageMultiplier();
         unitDamage.mDamageMultiplier = dmg.mDamageMultiplier;
//         unitDamage.mShieldDamageMultiplier = dmg.mShieldDamageMultiplier;
         for (int i = numEntityRefs - 1; i >= 0; i--)
         {
//-- FIXING PREFIX BUG ID 4434
            const BEntityRef* pEntityRef = getEntityRefByIndex(i);
//--
            if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeContainUnit))
            {
               BEntityID entityRefID = pEntityRef->mID;
               BUnit* pUnit = gWorld->getUnit(entityRefID);
               if (pUnit)
               {
                  unitDamage.mDamageDealt = 0.0f;
                  unitDamage.mShieldDamageDealt = 0.0f;
                  unitDamage.mAttackerTeamID = (pAttacker ? pAttacker->getTeamID() : -1);
                  unitDamage.mDamage -= pUnit->damage(unitDamage);
                  if (unitDamage.mKilled)
                     unloadUnit(i, true); // Destroys the entity ref!!

                  if (unitDamage.mDamage <= 0.0f)
                     break;
               }
               else
               {
                  removeEntityRef(i);
                  //BASSERTM(0, "BUnit::damage - Invalid unit in entityRefs list.");
               }
            }
         }
      }
   }

   createImpactEffect(dmg);
   applyDamageOverTimeEffect(dmg);

//   return dmg.mDamageDealt * recDamageMultiplier + dmg.mShieldDamageDealt * recShieldDamageMultiplier;
   return dmg.mDamageDealt * recDamageMultiplier + dmg.mShieldDamageDealt * recDamageMultiplier;
}

//=============================================================================
// BUnit::calculateDamageForce
//=============================================================================
void BUnit::calculateDamageForce(BDamage &dmg, float *pForce, BVector *pForceDir)
{
   BASSERT(pForce);
   BASSERT(pForceDir);

   // Get attacking players force multiplier
   float playerForceMultiplier = 1.0f;
   BEntity* pAttacker = gWorld->getEntity(dmg.mAttackerID);
   if (pAttacker)
   {
      const BPlayer* pAttackingPlayer = pAttacker->getPlayer();
      if (pAttackingPlayer)
         playerForceMultiplier = pAttackingPlayer->getWeaponPhysicsMultiplier();
   }

   // [7/2/2008 xemu] use the distance factor as a center point for the randomness, rather than a minimum 
   float forceFactor = dmg.mDistanceFactor + getRandRangeFloat(cSimRand, -0.5f, 0.5f);
   forceFactor = Math::Clamp(forceFactor, 0.0f, 1.0f);
   *pForce = Math::Lerp(dmg.mpDamageInfo->getPhysicsForceMin() * playerForceMultiplier, dmg.mpDamageInfo->getPhysicsForceMax() * playerForceMultiplier, forceFactor);
   //force *= forceScaler;

   // calculate launch direction using launch angle
   // Launch angle is lerped on distance factor then add 0-10%
   float launchFactor = dmg.mDistanceFactor + getRandRangeFloat(cSimRand, -0.5f, 0.5f);
   launchFactor = Math::Clamp(launchFactor, 0.0f, 1.0f);
   float launchAngle = dmg.mpDamageInfo->getPhysicsLaunchAngleMin() + launchFactor * (dmg.mpDamageInfo->getPhysicsLaunchAngleMax() - dmg.mpDamageInfo->getPhysicsLaunchAngleMin());

   BVector launchDir = getPosition() - dmg.mDamagePos;
   launchDir.y = 0.0f;
   if (!launchDir.safeNormalize())
      launchDir = cXAxisVector;
   launchDir *= cosf(launchAngle * cPi / 180.0f);
   launchDir.y = sinf(launchAngle * cPi / 180.0f);
   *pForceDir = launchDir;
}

//=============================================================================
// BUnit::createImpactEffect
//=============================================================================
void  BUnit::createImpactEffect(BDamage &dmg)
{
   // Early out if destructions are disabled
   if (gConfig.isDefined(cConfigNoDestruction))
      return;

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::createImpactEffect mID", mID.asLong());
   #endif

   if(!mDamageTracker)
      return;

   #ifdef SYNC_UnitDetail
      syncUnitDetailCode("BUnit::createImpactEffect 1");
   #endif

   if(!mpVisual)
      return;

   long id = mpVisual->getDamageTemplateID();

   const BDamageTemplate *pTemplate = gDamageTemplateManager.getDamageTemplate(id);
   if (!pTemplate)
      return;

   #ifdef SYNC_UnitDetail
      syncUnitDetailCode("BUnit::createImpactEffect 2");
   #endif

   // Skip doing damage based on hitpoints, so the damage is all spread out 
   // evenly through out the health bar.
   float hp = getHPPercentage();

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::createImpactEffect hp", hp);
   #endif

   // Calculate force vector
   BVector forceDir(0.0f, 1.0f, 0.0f);
   float force = 1000.0f;

   bool overrideForce = false;
   if (dmg.mpDamageInfo && dmg.mpDamageInfo->getThrowDamageParts())
   {
      overrideForce = true;

      calculateDamageForce(dmg, &force, &forceDir);
   }

   // Do percentagebased damage
   //

   if (hp < cFloatCompareEpsilon && getFlagShatterOnDeath() && pTemplate->getShatterDeathEvent())
   {
      if (overrideForce)
         mDamageTracker->shatterDeath(pTemplate, this, &forceDir, force);
      else
         mDamageTracker->shatterDeath(pTemplate, this, NULL, 1000.0f);
   }
   else
   {
      if (overrideForce)
         mDamageTracker->updatePercentageBaseDamage(hp, pTemplate, this, &forceDir, force);
      else
         mDamageTracker->updatePercentageBaseDamage(hp, pTemplate, this, NULL, 1000.0f);
   }


   // Do impactpointbased damage
   //

   long totalBreakableParts = pTemplate->getImpactPointCount();
   long mustKeepBrekableParts = (hp * totalBreakableParts);

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::createImpactEffect totalBreakableParts", totalBreakableParts);
      syncUnitDetailData("BUnit::createImpactEffect aliveImpactPointCount", mDamageTracker->getAliveImpactPointCount());
   #endif

   if(mDamageTracker->getAliveImpactPointCount() <= mustKeepBrekableParts)
   {
      return;
   }

   #ifdef SYNC_UnitDetail
      syncUnitDetailCode("BUnit::createImpactEffect 3");
   #endif

   // Compute model space impact position
   BVector impactPosWorldSpace = dmg.mDamagePos;
   BVector impactPosModelSpace;
   BMatrix matrix;
   getInvWorldMatrix(matrix);
   matrix.transformVectorAsPoint(impactPosWorldSpace, impactPosModelSpace);

   long impactBoneHandle = pTemplate->chooseClosestImpactPoint(mDamageTracker, mpVisual, impactPosModelSpace);


   const BDamageImpactPoint* pImpactPoint = pTemplate->getImpactPoint(impactBoneHandle);
   if(!pImpactPoint)
   {
      return;
   }

   long totalEvents = pImpactPoint->getEventCount();

   float impactPointHP = ((hp * totalBreakableParts) - mustKeepBrekableParts);
   long mustKeepEvents = (impactPointHP * totalEvents);

   long aliveCount = mDamageTracker->getAliveEventCountForImpactPoint(impactBoneHandle);

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::createImpactEffect totalEvents hp", hp);
      syncUnitDetailData("BUnit::createImpactEffect totalEvents totalBreakableParts", totalBreakableParts);
      syncUnitDetailData("BUnit::createImpactEffect totalEvents mustKeepBrekableParts", mustKeepBrekableParts);
      syncUnitDetailData("BUnit::createImpactEffect totalEvents impactPointHP", impactPointHP);
      syncUnitDetailData("BUnit::createImpactEffect totalEvents totalEvents", totalEvents);
      syncUnitDetailData("BUnit::createImpactEffect totalEvents mustKeepEvents", mustKeepEvents);
      syncUnitDetailData("BUnit::createImpactEffect totalEvents aliveCount", aliveCount);
   #endif

   if(aliveCount <= mustKeepEvents)
      return;

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::createImpactEffect overrideForce", overrideForce);
   #endif

   if (overrideForce)
      damageImpactPoint(impactBoneHandle, force, &forceDir, 100.0f);
   else
      damageImpactPoint(impactBoneHandle, 1000.0f, NULL, 100.0f);
}

//=============================================================================
// BUnit::damageImpactPoint
//=============================================================================
void BUnit::damageImpactPoint(long impactBoneHandle, float force, const BVector* pOverrideForceDir, float damage, bool bDeath, BEntityID* pOutEntityID)
{
   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::damageImpactPoint force", force);
   #endif

   if(!mpVisual)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BUnit::damageImpactPoint no visual");
      #endif
      return;
   }

   long id = mpVisual->getDamageTemplateID();

   if (id == -1)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BUnit::damageImpactPoint template id -1");
      #endif
      return;
   }

   const BDamageTemplate *pTemplate = gDamageTemplateManager.getDamageTemplate(id);
   if (!pTemplate)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BUnit::damageImpactPoint no template");
      #endif
      return;
   }

   /*
   // MS 9/14/2005: trees normally only go through BUnit::shatterImpactPoint, but
   // if they do come in here, their damage template should not be synced -- it
   // can and will be different between LP and HP. This is okay because a tree's
   // damage template is never supposed to be used to obtain dummy objects or
   // related synchronous things.
   #ifdef SYNC_UnitDetail
      if(!isAbstractType(cAbstractUnitTypeTree))
         syncUnitDetailData("   damage template=", pTemplate->getFileName().asANSI());
   #endif
   */

   // use first impact point for now
   bool bVisible = true;
   bool ballisticHit = true;

   const BDamageImpactPoint *pRef = NULL;
   pRef = pTemplate->getImpactPoint(impactBoneHandle);

   if(!pRef)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BUnit::damageImpactPoint no ref");
      #endif
      return;
   }

   //-- make sure that the impact point pre-reqs are met
   if (pRef->getPrereqID() != -1)
   {
      /*
      #ifdef SYNC_UnitDetail
         syncUnitDetailData("   pRef->getPrereqID()=", pRef->getPrereqID());
      #endif
      */
      if (mDamageTracker)
      {
         if(!mDamageTracker->isDestroyed(pRef->getPrereqID()))
         {
            /*
            #ifdef SYNC_UnitDetail
               syncUnitDetailCode("   prerequisite not yet destroyed, returning");
            #endif
            */
            #ifdef SYNC_UnitDetail
               syncUnitDetailCode("BUnit::damageImpactPoint prereq not destroyed");
            #endif
            return;
         }
      }
      /*
      else
      {
         #ifdef SYNC_UnitDetail
            syncUnitDetailData("   no damage tracker, unitID=", mID);
         #endif
      }
      */
   }

   /*
   //-- do we have a shape replacement?
   if (pRef->getReplaceShapeID() != -1)
   {
      BShape *pShape = gPhysics->getShapeManager().get(pRef->getReplaceShapeID(), true);
      if (mPhysicsObject && pShape)
         mPhysicsObject->setShape(*pShape, pRef->getReplaceShapeID());
   }
   */

   const float cMaxOffset = 0.2f;
   BVector offset(getRandRangeFloat(cSimRand, -cMaxOffset, cMaxOffset), getRandRangeFloat(cSimRand, -cMaxOffset, cMaxOffset), getRandRangeFloat(cSimRand, -cMaxOffset, cMaxOffset));
   shatterImpactPoint(pRef, bVisible, &offset, force, pOverrideForceDir, NULL, false, ballisticHit, pOutEntityID);
}


//=============================================================================
// BUnit::shatterImpactPoint
//=============================================================================
void BUnit::shatterImpactPoint(const BDamageImpactPoint *pRef, bool bVisible, const BVector *pModelSpacePoint /*= NULL*/, float force/*=0.0f*/, const BVector* pOverrideForceDir /*= NULL*/,BPhysicsCollisionListener *pListener /*= NULL*/ , bool forceCompleteDestruction /*=false*/, bool ballisticHit /*=true*/, BEntityID* pOutEntityID /*= NULL*/)
{
   syncUnitDetailData("BUnit::shatterImpactPoint bVisible", bVisible);

   if(!mpVisual || !pRef)
   {
      BASSERT(0);
      return;
   }

   if(!mDamageTracker)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BUnit::shatterImpactPoint no mDamageTracker");
      #endif
      return;
   }

   // notify our damage tracker
   int eventId = mDamageTracker->impactPointHit(pRef->getIndex());

   /*
   // MS 12/10/2004: threw a part off, so adjust our threshold
   long templateID = mpVisual->getDamageTemplateID();
   const BDamageTemplate *pTemplate = gDamageTemplateManager.getDamageTemplate(templateID);

   if(pTemplate && pTemplate->getImpactPointCount() > 0)
   {
      const float cIntervalMultiplier = 0.5f;
      const float cMaxInterval = 200.0f;
      float interval = cIntervalMultiplier * getMaximumHitpoints() / pTemplate->getImpactPointCount();
      interval = interval > cMaxInterval ? cMaxInterval : interval;
      mImpactEffectHPThreshold = mHitpoints - interval;
   }
   else
      mImpactEffectHPThreshold = cMaximumFloat;
   */


   const BDamageEvent* pEvent = pRef->getEvent(eventId);

   if(!pEvent)
   {
      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BUnit::shatterImpactPoint no pEvent");
      #endif
      return;
   }

   // Execute all actions on this event
   long actioncount = pEvent->getActionCount();

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::shatterImpactPoint actioncount", actioncount);
   #endif

   for (long actionindex = 0; actionindex < actioncount; actionindex++)
   {
      const BDamageAction* pAction = pEvent->getAction(actionindex);

      #ifdef SYNC_UnitDetail
         syncUnitDetailData("BUnit::shatterImpactPoint action type", pAction->getType());
      #endif

      pAction->execute(this, bVisible, pModelSpacePoint, force, ballisticHit, pOverrideForceDir, pOutEntityID);
   }
}


//=============================================================================
// BUnit::createAndThrow
//
// returns the entity id of the created object, or cInvalidObjectID if failed to create
//=============================================================================
BEntityID BUnit::createAndThrow( const BVector &impulseForceWorldSpace, 
                                      const BVector &impulsePointWorldSpace,
                                      const BPhysicsObjectParams &physicsparams,
                                      const BBitArray& mask, 
                                      float lifespan /*= -1.0f*/)
{
   // Create a new BObject from the visual item
   BObjectCreateParms params;
   params.mPosition = getPosition();
   params.mForward = getForward();
   params.mRight = getRight();
   params.mNoTieToGround = true;
   params.mPhysicsReplacement = true;
   params.mPlayerID = getPlayerID();
   params.mProtoObjectID = gDatabase.getPOIDPhysicsThrownObject();
   params.mProtoSquadID = -1;
   params.mStartBuilt = false;
   params.mType = BEntity::cClassTypeObject;
   params.mMultiframeTextureIndex = getMultiframeTextureIndex();

   //BObject* pObject = gWorld->createVisPhysicsObject(params, pVisItem, physicsInfo, pBPOverrides, true);
   BObject* pObject = gWorld->createVisPhysicsObjectDirect(params, mpVisual, physicsparams, true);

   if(!pObject)
      return cInvalidObjectID;


   /*
   // Only enable the render mask for the part that we are spawning
   BBitArray renderMask = ((BGrannyInstance*)pObject->getVisual()->mpInstance)->getMeshRenderMask();
   renderMask.clear();
   renderMask.setBit(meshIndex);
   */
   ((BGrannyInstance*)pObject->getVisual()->mpInstance)->setMeshRenderMask(mask);

   pObject->copyAdditionalTextures(this);

   if (lifespan > 0.0f)
   {
      pObject->setLifespan((DWORD)(1000.0f * lifespan));
   }

   // Factor in unit velocity
   BVector velocity = getVelocity();
   velocity.scale(150);

   BVector finalImpulse = impulseForceWorldSpace + velocity;

   pObject->getPhysicsObject()->applyPointImpulse(finalImpulse, impulsePointWorldSpace);

   return(pObject->getID());
}


//=============================================================================
// BUnit::applyDamageOverTimeEffect
//=============================================================================
void  BUnit::applyDamageOverTimeEffect(BDamage &dmg)
{
   bool bHaveDOTFromThisSource = false;

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnit::applyDamageOverTimeEffect dmg.mDOTrate", dmg.mDOTrate);
   #endif

   if (dmg.mDOTrate <= 0.0f)
      return;

   BUnitActionDOT* pActionDOT = (BUnitActionDOT*)mActions.getActionByType(BAction::cActionTypeUnitDOT);

   #ifdef SYNC_UnitDetail
      bool bFoundAction = (pActionDOT != NULL);
      syncUnitDetailData("BUnit::applyDamageOverTimeEffect bFoundAction", bFoundAction);
   #endif

   if (pActionDOT)
   {
      int numActions = mActions.getNumberActions();
      #ifdef SYNC_UnitDetail
         syncUnitDetailData("BUnit::applyDamageOverTimeEffect numActions", numActions);
      #endif
      for (int i = 0; i < numActions; i++)
      {
         BAction* pAction = mActions.getAction(i);
         if (pAction && pAction->getType() == BAction::cActionTypeUnitDOT)
         {
            #ifdef SYNC_UnitDetail
               syncUnitDetailData("BUnit::applyDamageOverTimeEffect i", i);
            #endif
            BUnitActionDOT* pDotAction = static_cast<BUnitActionDOT*>(pAction);
            if (pDotAction && pDotAction->hasSource(dmg.mAttackerID))
            {
               // refresh DOT duration
               #ifdef SYNC_UnitDetail
                  syncUnitDetailData("BUnit::applyDamageOverTimeEffect refresh getGametime", gWorld->getGametime());
               #endif
               pDotAction->refreshStack(dmg.mAttackerID, dmg.mAttackerTeamID, gWorld->getGametime() + (dmg.mDOTduration * 1000));
               bHaveDOTFromThisSource = true;
               break;
            }
            // If it's the same DOT type, add a new stack
            else if (pDotAction && (*pDotAction == dmg))
            {
               #ifdef SYNC_UnitDetail
                  syncUnitDetailData("BUnit::applyDamageOverTimeEffect add getGametime", gWorld->getGametime());
               #endif
               pDotAction->addStack(dmg.mAttackerID, dmg.mAttackerTeamID, gWorld->getGametime() + (dmg.mDOTduration * 1000));
               bHaveDOTFromThisSource = true;
            }
         }
      }
   }

   if (!bHaveDOTFromThisSource)
   {
      pActionDOT = (BUnitActionDOT*)gActionManager.createAction(BAction::cActionTypeUnitDOT);
      if (pActionDOT)
      {
         #ifdef SYNC_UnitDetail
            syncUnitDetailData("BUnit::applyDamageOverTimeEffect dmg.mDOTrate", dmg.mDOTrate);
            syncUnitDetailData("BUnit::applyDamageOverTimeEffect dmg.mDOTrate", dmg.mDamageMultiplier);
         #endif

         pActionDOT->addStack(dmg.mAttackerID, dmg.mAttackerTeamID, gWorld->getGametime() + (dmg.mDOTduration * 1000));
         pActionDOT->setDOTrate(dmg.mDOTrate);
         pActionDOT->setDamageMultiplier(dmg.mDamageMultiplier);
         pActionDOT->setDOTeffect(dmg.mDOTEffect);

         // Find location to attach DOT effect
         if (dmg.mDOTEffect != cInvalidObjectID)
         {
            const BProtoObject* pProto = getProtoObject();

            if (pProto && (pProto->isType(gDatabase.getOTIDBuilding()) || pProto->isType(gDatabase.getOTIDGroundVehicle())))
               pActionDOT->setEffectLocation(dmg.mDamagePos);
            else
               pActionDOT->setEffectLocation(cOriginVector);
         }
         addAction(pActionDOT);
      }
   }
}

//=============================================================================
// BUnit::forceThrowPartImpactPoint
//=============================================================================
BEntityID BUnit::forceThrowPartImpactPoint(const BVector& impactPosWorldSpace)
{
   // attempt to find a throw part impact action 

   syncUnitDetailCode("BUnit::forceThrowPartImpactPoint");

   if(!mpVisual)
      return cInvalidObjectID;

   long id = mpVisual->getDamageTemplateID();

   const BDamageTemplate *pTemplate = gDamageTemplateManager.getDamageTemplate(id);
   if (!pTemplate)
      return cInvalidObjectID;

   // Compute model space impact position
   BVector impactPosModelSpace;
   BMatrix matrix;
   getInvWorldMatrix(matrix);
   matrix.transformVectorAsPoint(impactPosWorldSpace, impactPosModelSpace);

   long impactBoneHandle = pTemplate->chooseClosestImpactPoint(NULL, mpVisual, impactPosModelSpace, true);

   const BDamageImpactPoint* pImpactPoint = pTemplate->getImpactPoint(impactBoneHandle);
   if(!pImpactPoint)
      return cInvalidObjectID;
   
   float force = 1000.0f;
   BEntityID returnId = cInvalidObjectID;

   // go ahead and smash the impact point once
   damageImpactPoint(impactBoneHandle, force, NULL, 0.0f, false, &returnId);

   // if we got a return id from damaging the impact point, use that - no need to find a final throw part event
   if (returnId != cInvalidObjectID)
      return returnId;

   const BDamageEvent* pEvent = pImpactPoint->getFinalThrowPartEvent();
   if(!pEvent)
      return cInvalidObjectID;

   // Execute all actions on this event, but no throw part actions after we have a valid return object
   long actioncount = pEvent->getActionCount();

   for (long actionindex = 0; actionindex < actioncount; actionindex++)
   {
      const BDamageAction* pAction = pEvent->getAction(actionindex);
      const BDamageActionThrowPart* pThrowPartAction = static_cast<const BDamageActionThrowPart*>(pAction);
      if (returnId == cInvalidObjectID)
         pAction->execute(this, true, &impactPosModelSpace, force, true, NULL, &returnId);
      else if (!pThrowPartAction)
         pAction->execute(this, true, &impactPosModelSpace, force, true, NULL, NULL);
   }

   return returnId;
}

//=============================================================================
//=============================================================================
BEntityID BUnit::throwRandomPartImpactPoint()
{
   // attempt to find a throw part impact action 
   syncUnitDetailCode("BUnit::throwRandomPartImpactPoint");

   if(!mpVisual)
      return cInvalidObjectID;

   long id = mpVisual->getDamageTemplateID();

   const BDamageTemplate *pTemplate = gDamageTemplateManager.getDamageTemplate(id);
   if (!pTemplate)
      return cInvalidObjectID;

   long impactPointCount = pTemplate->getImpactPointCount();
   if (impactPointCount <= 0)
      return cInvalidObjectID;

   long index = getRand(cSimRand);
   index %= impactPointCount;
   const BDamageImpactPoint* pImpactPoint = pTemplate->getImpactPointByIndex(index);
   if(!pImpactPoint)
      return cInvalidObjectID;

   float force = 1000.0f;

   const BDamageEvent* pEvent = pImpactPoint->getFinalThrowPartEvent();
   if(!pEvent)
      return cInvalidObjectID;

   // Execute the throw part action
   long actioncount = pEvent->getActionCount();
   BEntityID returnId = cInvalidObjectID;

   for (long actionindex = 0; actionindex < actioncount; actionindex++)
   {
      const BDamageAction* pAction = static_cast<const BDamageActionThrowPart*>(pEvent->getAction(actionindex));
      if (pAction && returnId == cInvalidObjectID)
      {
         pAction->execute(this, true, &cOriginVector, force, true, NULL, &returnId);
         if (returnId != cInvalidObjectID)
            break;
      }
   }

   return returnId;
}

//=============================================================================
// BUnit::updateDamageTracker
//=============================================================================
void BUnit::updateDamageTracker(float elapsed)
{
   if(!mpVisual)
      return;

   // Thrown objects seem to cause this function to go OOS. We don't need damage trackers on thrown objects so early out.
   if (getProtoID() == gDatabase.getPOIDPhysicsThrownObject())
      return;

#ifndef BUILD_FINAL
   //////////////////////////////////////////////////////////////////////////
   // SLB: temporary sync debugging stuff
   long newUnitDamageTemplateID = mDamageTracker ? mDamageTracker->getDamageTemplateID() : -1;
   long newVisualDamageTemplateID = mpVisual->getDamageTemplateID();

   #ifdef SYNC_UnitDetail
      if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncUnitDetailData("BUnit::updateDamageTracker newUnitDamageTemplateID", newUnitDamageTemplateID);
         syncUnitDetailData("BUnit::updateDamageTracker newVisualDamageTemplateID", newVisualDamageTemplateID);
      }

      syncUnitDetailData("BUnit::updateDamageTracker unit damage template diff:", (mUnitDamageTemplateID == newUnitDamageTemplateID));
      syncUnitDetailData("BUnit::updateDamageTracker visual damage template diff:", (mVisualDamageTemplateID == newVisualDamageTemplateID));
   #endif

   mUnitDamageTemplateID = newUnitDamageTemplateID;
   mVisualDamageTemplateID = newVisualDamageTemplateID;
   //////////////////////////////////////////////////////////////////////////
#endif

   #ifdef SYNC_UnitDetail
      syncUnitDetailCode("BUnit::updateDamageTracker 1");
   #endif

   if(mDamageTracker)
   {
      if(getProtoID() != gDatabase.getPOIDPhysicsThrownObject() && !getFlagIsPhysicsReplacement())
      {
         #ifdef SYNC_UnitDetail
            syncUnitDetailCode("BUnit::updateDamageTracker 2");
         #endif
         long damageTemplateID = mpVisual->getDamageTemplateID();
         const BDamageTemplate *pTemplate = gDamageTemplateManager.getDamageTemplate(damageTemplateID);
         if (pTemplate)
         {
            float hpPct = getHPPercentage();
            #ifdef SYNC_UnitDetail
               syncUnitDetailData("BUnit::updateDamageTracker hpPct", hpPct);
            #endif

            mDamageTracker->updatePercentageBaseDamageSilent(hpPct, pTemplate, this);
         }

         mDamageTracker->updateThrownParts(elapsed);
      }
   }

   #ifdef SYNC_UnitDetail
      if (mDamageTracker)
      {
         const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(mDamageTracker->getDamageTemplateID());
         if (pDT)
         {
            syncUnitDetailData("BUnit::updateDamageTracker unit damage template", pDT->getFileName().getPtr());
            syncUnitDetailData("BUnit::updateDamageTracker unit damage template impact point count", pDT->getImpactPointCount());
            syncUnitDetailData("BUnit::updateDamageTracker unit damage template percentage based event count", pDT->getPercentageBasedEventCount());
            syncUnitDetailData("BUnit::updateDamageTracker unit damage template cryo based event count", pDT->getCryoPercentageBasedEventCount());
         }
      }
   #endif

   long damageTemplateID = mpVisual->getDamageTemplateID();
   if(damageTemplateID < 0)
   {
      if(mDamageTracker)
      {
         #ifdef SYNC_UnitDetail
            syncUnitDetailCode("BUnit::updateDamageTracker delete mDamageTracker 1");
         #endif

         delete mDamageTracker;//delete *&mDamageTracker; //AMG
         mDamageTracker = NULL;
      }
   }
   else
   {
      #ifdef SYNC_UnitDetail
      {
         const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(damageTemplateID);
         if (pDT)
         {
            syncUnitDetailData("BUnit::updateDamageTracker visual damage template", pDT->getFileName().getPtr());
            syncUnitDetailData("BUnit::updateDamageTracker visual damage template impact point count", pDT->getImpactPointCount());
            syncUnitDetailData("BUnit::updateDamageTracker visual damage template percentage based event count", pDT->getPercentageBasedEventCount());
            syncUnitDetailData("BUnit::updateDamageTracker visual damage template cryo based event count", pDT->getCryoPercentageBasedEventCount());
         }
      }
      #endif

      #ifdef SYNC_UnitDetail
         syncUnitDetailCode("BUnit::updateDamageTracker damageTemplateID >= 0");
      #endif

      if(mDamageTracker)
      {
         if(damageTemplateID == mDamageTracker->getDamageTemplateID())
         {
            #ifdef SYNC_UnitDetail
               syncUnitDetailCode("BUnit::updateDamageTracker damageTemplateID unchanged");
            #endif
            return;
         }
         else
         {
            #ifdef SYNC_UnitDetail
               syncUnitDetailCode("BUnit::updateDamageTracker delete mDamageTracker 2");
            #endif
            delete mDamageTracker;//delete *&mDamageTracker; //AMG
            mDamageTracker = NULL;
         }
      }

      mDamageTracker = new BDamageTracker();
      if(!mDamageTracker)
      {
         #ifdef SYNC_UnitDetail
               syncUnitDetailCode("BUnit::updateDamageTracker new mDamageTracker failed");
         #endif
         return;
      }

      mDamageTracker->init(damageTemplateID);

#ifndef BUILD_FINAL
      //////////////////////////////////////////////////////////////////////////
      // SLB: temporary sync debugging stuff
      mUnitDamageTemplateID = mDamageTracker ? mDamageTracker->getDamageTemplateID() : -1;
      mVisualDamageTemplateID = damageTemplateID;
      //////////////////////////////////////////////////////////////////////////
#endif

      #ifdef SYNC_UnitDetail
      {
         const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(mDamageTracker->getDamageTemplateID());
         if (pDT)
         {
            syncUnitDetailData("BUnit::updateDamageTracker new unit damage template", pDT->getFileName().getPtr());
            syncUnitDetailData("BUnit::updateDamageTracker new unit damage template impact point count", pDT->getImpactPointCount());
            syncUnitDetailData("BUnit::updateDamageTracker new unit damage template percentage based event count", pDT->getPercentageBasedEventCount());
            syncUnitDetailData("BUnit::updateDamageTracker new unit damage template cryo based event count", pDT->getCryoPercentageBasedEventCount());
         }
      }
      #endif
   }
}


#ifndef BUILD_FINAL
//=============================================================================
// BUnit::reInitDamageTracker
//=============================================================================
void BUnit::reInitDamageTracker()
{
   #ifdef SYNC_UnitDetail
      syncUnitDetailCode("BUnit::reInitDamageTracker");
   #endif

   // This function only gets called when dmg files get reloaded
   //

   if(mDamageTracker != NULL)
   {
      delete mDamageTracker; //delete *&mDamageTracker;
      mDamageTracker = NULL;
   }
   

   if(!mpVisual)
      return;

   long damageTemplateID = mpVisual->getDamageTemplateID();
   if(damageTemplateID >= 0)
   {
      mDamageTracker = new BDamageTracker();
      if(!mDamageTracker)
         return;

      mDamageTracker->init(damageTemplateID);

      // Undo all damage
      BGrannyInstance *pGrannyInstance = mpVisual->getGrannyInstance();

      setMultiframeTextureIndex(0);
      if (pGrannyInstance)
         pGrannyInstance->setMeshRenderMaskToUndamageState();

      // Remove all user attachments
      int numAttachments = mpVisual->mAttachments.getNumber();
      for (int i = numAttachments - 1; i >= 0; i--)
      {
         BVisualItem* pCurAttachment = mpVisual->mAttachments[i];
         if(pCurAttachment && pCurAttachment->getFlag(BVisualItem::cFlagUser))
         {
            BVisualItem::releaseInstance(pCurAttachment);
            mpVisual->mAttachments.removeIndex(i, false);
         }
      }
   }
}
#endif


//==============================================================================
//==============================================================================
int BUnit::getDamageTypeMode() const
{
   int mode = 0;
   const BSquad* pSquad = getParentSquad();
   if (pSquad)
   {
      int squadMode = pSquad->getSquadMode();
      if (squadMode != BSquadAI::cModeNormal && squadMode != -1 && squadMode == getProtoObject()->getSecondaryDamageTypeMode())
         mode = 1;
   }
   return mode;
}

//==============================================================================
//==============================================================================
void BUnit::addThrownPart(const BEntityID& newPart)
{
   if (mDamageTracker && newPart != cInvalidObjectID)
      mDamageTracker->addThrownPart(newPart);
}

//==============================================================================
// BUnit::destroy
//==============================================================================
void BUnit::destroy()
{
   #ifdef SYNC_Unit
      syncUnitData("BUnit::destroy mID", mID.asLong());
   #endif

   BASSERTM(getFlagAlive() == false, "Destroy is being called on a unit that has not been killed.  Don't call destroy directly.  Call kill(true) instead!");
   BASSERTM(getParentID() == cInvalidObjectID, "Destroy is being called on a unit that has a parent squad ID!  Doh!");
   
   // Remove the unit from the squad, if he's currently in a squad.
   // Note: Must call this in destroy as well as kill, becasue we can destroy a unit directly and it was being left in its squad.
   //safeDesquad();

   //getPlayer()->removeUnitFromProtoObject(this, mProtoID);
   mpCachedParentSquad = NULL;

   BObject::destroy();

   //gWorld->updateAllKBsForUnit(this);
}

//==============================================================================
// BUnit::notify
//==============================================================================
void BUnit::notify(DWORD eventType, BEntityID sender, DWORD data, DWORD data2)
{
   if (eventType == cEventRecoverSet)
   {
      int abilityID = (int)data;
      //int recoverType = (int)data2;
      if (abilityID != -1)
      {
//-- FIXING PREFIX BUG ID 4362
         const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--
         if (pAbility && pAbility->getRecoverAnimAttachment()!=-1)
         {
            if (pAbility->getRecoverStartAnim() != -1)
               playAttachmentAnim(pAbility->getRecoverAnimAttachment(), pAbility->getRecoverStartAnim());
            if (pAbility->getRecoverEndAnim() != -1)
               playAttachmentAnimOnEvent(pAbility->getRecoverAnimAttachment(), pAbility->getRecoverEndAnim(), cEventRecoverSet, -1, -1, true, true);
         }
      }
   }
   else if (eventType == cEventCorpseRemove)
   {
      // corpse manager wants us to die - we need to remove all particle effects from ourselves
      if (getVisual())
         getVisual()->removeAttachmentsOfAssetType(cVisualAssetParticleSystem);
   }
   else if (eventType == cEventRecomputeVisualStarting)
   {
      // we need to unlock the ability attachment anim, 
      // so that recompute anim can update the attachment if necessary 
      setAbilityRecoverAttachmentAnimationLock(false);
   }
   else if (eventType == cEventRecomputeVisualCompleted)
   {
      // we need to relock the ability attachment anim, 
      // since we unlocked it for the recompute visual
      // if we are recovering, we already have an event to play the 
      // recover ended animation, so make sure the attachment has the right
      // anim set. If not, we need to replay the animation
      bool playedAnim = false;
/*
      // VAT: 11/19/08 - would be awesome if this worked, but for some reason
      // casues the anim to loop infinitely
      BSquad* pParentSquad = getParentSquad();
      if (pParentSquad && pParentSquad->getFlagRecovering())
      {
         const BProtoObject* pProtoObject = getProtoObject();
         if (pProtoObject)
         {
            int abilityID = pProtoObject->getAbilityCommand();
            if (abilityID != -1)
            {
               // if we have an ability that has an anim attachment, and a start anim
               const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
               if (pAbility && pAbility->getRecoverAnimAttachment() != -1 && pAbility->getRecoverStartAnim() != -1)
               {
                  BVisual* pVisual = getVisual();
                  if (pVisual)
                  {
                     // grab the attachment and verify that the recover start anim is set
                     // this is trusting that once we play a recover start anim, that should be set
                     // until the recover is complete - which is certainly the case now
                     BVisualItem* pAttachment = pVisual->getAttachment(pAbility->getRecoverAnimAttachment());
                     if (pAttachment && pAttachment->getAnimationType(cActionAnimationTrack) != pAbility->getRecoverStartAnim())
                     {
                        playAttachmentAnim(pAbility->getRecoverAnimAttachment(), pAbility->getRecoverStartAnim());
                        playedAnim = true;
                     }
                  }
               }
            }
         }
      }
*/

      if (!playedAnim)
      {
         // fail safe, if we're not recovering or failed to play the anim
         // just lock the ability attachment for consistency
         setAbilityRecoverAttachmentAnimationLock(true);
      }
   }

   BObject::notify(eventType, sender, data, data2);
}

//==============================================================================
//==============================================================================
void BUnit::setAbilityRecoverAttachmentAnimationLock(bool newVal)
{
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject)
   {
      int abilityID = pProtoObject->getAbilityCommand();
      if (abilityID != -1)
      {
         const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
         if (pAbility)
         {
            BVisual* pVisual = getVisual();
            if (pVisual)
            {
               BVisualItem* pAttachment = pVisual->getAttachment(pAbility->getRecoverAnimAttachment());
               if (pAttachment)
               {
                  pAttachment->setAnimationLock(cActionAnimationTrack, newVal);
                  pAttachment->setAnimationLock(cMovementAnimationTrack, newVal);
               }
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::adjustVelocityScalar(float adjust)
{
   syncUnitData("adjustVelocityScalar", adjust);
   
   mVelocityScalar *= adjust; 
}


//==============================================================================
//==============================================================================
void BUnit::setVelocityScalar(float scalar)
{
   syncUnitData("setVelocityScalar", scalar);

   mVelocityScalar = scalar;
}


//==============================================================================
//==============================================================================
bool BUnit::canMove(bool allowAutoUnlock) const
{
   //Check us.
   if (!BEntity::canMove(allowAutoUnlock))
      return (false);

   //Check our squad.
//-- FIXING PREFIX BUG ID 4363
   const BSquad* pSquad=getParentSquad();
//--
   BDEBUG_ASSERT(pSquad);
   if (!pSquad)
      return false;
   return (pSquad->canMove(allowAutoUnlock));
}

//==============================================================================
//==============================================================================
void BUnit::safeDesquad()
{
   if (mParentID.isValid())
   {
      BSquad *pSquad = getParentSquad();
      if (pSquad)
         pSquad->removeChild(mID, true);

      setParentID(cInvalidObjectID);
   }
}

//==============================================================================
//==============================================================================
void BUnit::determineKiller(BPlayerID& killerPlayerID, long& killerProtoID, long& killerProtoSquadID, bool& killedByRecycler)
{
   killedByRecycler = mFlagRecycled;

   queryEntityID(mKilledByID, killerPlayerID, killerProtoID, killerProtoSquadID);
}

//==============================================================================
//==============================================================================
void BUnit::kill(bool bKillImmediately)
{
   // SLB: Revealers die immediately
   if (getFlagIsRevealer())
      bKillImmediately = true;

   // Check if unit is not being destroyed and needs to perform a hero death
   if (!bKillImmediately)
   {
      const BProtoObject* pProtoObject = getProtoObject();
      if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDHeroDeath()))
      {
         killHero();
         return;
      }
   }

   // If we have a flood poof player set, set us up to poof
   BPlayer* pPlayer = gWorld->getPlayer(getPlayerID());

   if (pPlayer && pPlayer->getFloodPoofPlayer() != cInvalidPlayerID && !mFlagFatalityVictim && !mFlagDoingFatality)
   {
      mInfectionPlayerID = pPlayer->getFloodPoofPlayer();
      mFlagInfected = true;
   }

   // SLB - MPB: Don't kill units that already have physics actions that are going to kill the units for us, unless we want to kill immediately.
   if (!bKillImmediately)
   {
      const BUnitActionPhysics* pAction = reinterpret_cast<const BUnitActionPhysics*>(getActionByTypeConst(BAction::cActionTypeUnitPhysics));
      if (pAction)
      {
         if (pAction->getFlagCompleteOnInactivePhysics())
            return;

         // or, if we are supposed to have a physics detonate, and we haven't detonated yet, don't kill
         if (getProtoObject()->getFlagPhysicsDetonateOnDeath())
         {
            const BUnitActionDetonate* pDetonateAction = reinterpret_cast<const BUnitActionDetonate*>(getActionByTypeConst(BAction::cActionTypeUnitDetonate));
            if (pDetonateAction && !pDetonateAction->getDetonated())
               return;
         }
      }
   }

   // VAT: 11/06/08 if we're being destroyed by a parent base, we don't die immediately
   if (handleDelayKillBase())
      return;

   // if I'm marked as a building going through a base destroy,
   // don't do an insta-kill and clear the lifespan, 
   // since we're not killing immediately
   if (getFlagDestroyedByBaseDestruction())
   {
      enableLifespan(false);
      bKillImmediately = false;
   }

   #ifdef SYNC_Unit
      syncUnitData("BUnit::kill mID", mID.asLong());
   #endif

   BEntityID parkingLot = getAssociatedParkingLot();
   if (mFlagRallyPoint && (parkingLot == cInvalidObjectID || parkingLot == mID))
      clearRallyPoint(getPlayerID());
   if (mFlagRallyPoint2 && (parkingLot == cInvalidObjectID || parkingLot == mID))
      clearRallyPoint(getPlayer()->getCoopID());
   stopAllHardpointSounds();

   // Unload dying cover unit
   bool inCover = false;
   if (getFlagGarrisoned() && getFlagInCover())
   {
//-- FIXING PREFIX BUG ID 4369
      const BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeContainingUnit);
//--
      if (pRef)
      {
         BUnit* pUnit = gWorld->getUnit(pRef->mID);
         if (pUnit)
         {
            pUnit->unloadUnit(getID(), true);
            inCover = true;
         }
      }
   }

   // Unload or kill garrisoned (contained or attached) units
   if (getFlagHasGarrisoned() || getFlagHasAttached())
   {
      BVector forward=mForward, right=mRight, pos=mPosition;
      bool findPos = (getFirstEntityRefByType(BEntityRef::cTypeParentSocket) != NULL) || (getProtoObject() && getProtoObject()->getDeathSpawnSquad());
      int numEntityRefs = getNumberEntityRefs();
      for (int i = numEntityRefs - 1; i >= 0; i--)
      {
//-- FIXING PREFIX BUG ID 4370
         const BEntityRef* pEntityRef = getEntityRefByIndex(i);
//--
         if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeContainUnit || pEntityRef->mType == BEntityRef::cTypeAttachObject))
         {
            BEntityID entityRefID = pEntityRef->mID;
            BUnit* pUnit = gWorld->getUnit(entityRefID);
            if (!pUnit)
               continue;
            BProtoObject* pProtoObject = const_cast<BProtoObject*>(pUnit->getProtoObject());
            BSquad* pSquad = pUnit->getParentSquad();
            BProtoSquad* pProtoSquad = (pSquad ? const_cast<BProtoSquad*>(pSquad->getProtoSquad()) : NULL);
            bool gotPreferredPosition = false;
            if (findPos)
               pos = gWorld->getSquadPlacement(this, NULL, pProtoSquad, pProtoObject, &forward, &right, -1, gotPreferredPosition);
            if (pEntityRef->mType == BEntityRef::cTypeContainUnit)
            {
               if (pSquad)
               {
                  pSquad->setPosition(pos, true);
                  pSquad->setLeashPosition(pos);
               }
               #ifdef SYNC_Unit
                  syncUnitData("BUnit::kill", pos);
               #endif
               pUnit->setPosition(pos, true);
               pUnit->setForward(forward);
               pUnit->setRight(right);                     
               pUnit->calcUp();
               pUnit->clearGoalVector();
               if (pSquad)
                  pSquad->settle();
            }
            if (getProtoObject()->getFlagKillGarrisoned())
            {
               // Halwes - 8/21/2008 - Special case for heroes garrisoned inside of a dying transporter
               if (isType(gDatabase.getOTIDTransporter()) && (pUnit->isType(gDatabase.getOTIDHeroDeath()) || pUnit->isType(gDatabase.getPOIDForgeWarthog())))
               {
                  if (pEntityRef->mType == BEntityRef::cTypeContainUnit)
                  {
                     unloadUnit(entityRefID, false); // Destroys the entity ref!
                  }
                  else if (pEntityRef->mType == BEntityRef::cTypeAttachObject)
                  {
                     unattachObject(entityRefID); // Destroys the entity ref!
                  }

                  // VAT: 11/12/08: Since we don't actually ungarrison the unit in this case, we need to clear this flag here
                  // otherwise, when the unit gets back up, it will never be transportable again, since this flag is set
                  if (pSquad)
                     pSquad->setFlagIsTransporting(false);
                  pUnit->setKilledByID(mKilledByID);
                  pUnit->kill(false);
               }
               else
               {
                  if (pEntityRef->mType == BEntityRef::cTypeContainUnit)
                     unloadUnit(entityRefID, true); // Destroys the entity ref!
                  else if (pEntityRef->mType == BEntityRef::cTypeAttachObject)
                     unattachObject(entityRefID); // Destroys the entity ref!

                  pUnit->setKilledByID(mKilledByID);
                  pUnit->kill(bKillImmediately);
               }                  
            }
            else
            {
               if (pEntityRef->mType == BEntityRef::cTypeContainUnit)
               {
                  unloadUnit(entityRefID, false); // Destroys the entity ref!                  
               }
               else if (pEntityRef->mType == BEntityRef::cTypeAttachObject)
               {
                  unattachObject(entityRefID); // Destroys the entity ref!                  
                  BUnit* pUnit = gWorld->getUnit(entityRefID);
                  if (pUnit)
                  {
                     pUnit->clearGoalVector();
                     BSquad* pSquad = pUnit->getParentSquad();
                     if (pSquad)
                     {
                        pSquad->settle();
                     }
                  }
               }
            }
         }
      }
   }

   // Unhitch the hitched unit
   if (getFlagHasHitched())
   {
      BEntityID hitchedUnitID = getHitchedUnit();
      BUnit* pHitched = gWorld->getUnit(hitchedUnitID);
      if (pHitched)
      {
         unhitchUnit(hitchedUnitID);
         // If unit has a hitched unit play the hitched unit's unhitch animation
         if (pHitched->isAnimationLocked())
         {
            pHitched->unlockAnimation();
         }
         pHitched->setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeUnhitch, true, true);
         pHitched->computeAnimation();
         BASSERT((pHitched->getAnimationState() == BObjectAnimationState::cAnimationStateMisc) && pHitched->isAnimationApplied());
      }
   }

   //DCP 07/24/07:  If we're not alive but we've somehow called this again with
   //a killImm flag, just call destroy and be done with it.
   if (!getFlagAlive())
   {
      if (bKillImmediately)
         destroy();
      return;
   }

   // if our animation is disabled because we're frozen, unset it
   if (getFlagShatterOnDeath())
      setAnimationEnabled(true, true);

   // mrh - Apply UNSC & COV dynamic work rate modifiers.  This replaces a trigger script that was running continuously.
   recalculateSupplyPadWorkRateModifiers();

   // SLB: We don't allow this object to become invisible while it's animating. Let players see the entire death animation.
   setFlagRemainVisible(true);

   // [10/8/2008 xemu] death explosion skull support
   if ((gCollectiblesManager.getDeathExplodeChance() > 0.0f) && isType(gCollectiblesManager.getDeathExplodeObjectType()))
   {
      // [10/8/2008 xemu] affects only non-human players
      if (!pPlayer->isHuman())
      {
         float randVal = getRandRangeFloat(cSimRand, 0.0f, 1.0f);
         if (randVal < gCollectiblesManager.getDeathExplodeChance())
         {
            doDeathExplode(mKilledByID);
         }
      }
   }

   // SLB: Disable LOS. The dead can't see.
   if (getFlagLOSMarked())
      markLOSOff();
   setFlagLOS(false);

   bool notifySent = false;
   long deathReplacementId =getProtoObject()->getDeathReplacement();
   bool ignoreDeathReplacement = (getProtoObject()->getFlagShatterDeathReplacement() && !getFlagShatterOnDeath());
   bool createdDeathReplacement = false;
   if(deathReplacementId != -1 && !ignoreDeathReplacement)
   {
      // we need everything to run as if we didn't replace the unit if we're doing a shatter
      if (!getProtoObject()->getFlagShatterDeathReplacement())
         createdDeathReplacement = true;

      // Transform into the death replacement instead of killing the unit
      if (deathReplacementId != mProtoID)
         transform(deathReplacementId);
      if (getProtoObject()->getFlagForceToGaiaPlayer())
      {
         BSquad* pParentSquad = getParentSquad();
         BASSERT(pParentSquad);
         if (pParentSquad)
         {
            BASSERT(pParentSquad->getNumberChildren() == 1);
            pParentSquad->changeOwner(0);
            pParentSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);
         }
         setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateIdle, -1, true, true);
         computeAnimation();
         BASSERT(isAnimationApplied());
      }
      if (getProtoObject()->getFlagDamagedDeathReplacement())
      {
         setHitpoints(1.0f);
         mFlagDeathReplacementHealing=true;
         mFlagInvulnerable=true;
      }
      else
         setHitpoints(getProtoObject()->getHitpoints());
      setShieldpoints(getProtoObject()->getShieldpoints());
      setCapturePoints(0.0f, 0.0f);
      setCapturePlayerID(cInvalidPlayerID);
      gWorld->notify(BEntity::cEventDeathReplacement, mID, 0, 0);

      // Clear Opportunities
      removeOpps();
   }
   else
   {
      // Handle Infection.  We only do this if we have something to infect into.
      // Halwes - 7/7/2008 - Don't infect if in cover
      if (mFlagInfected && !inCover)
      {
         BProtoObjectID infectedPOID;
         BProtoSquadID infectedPSID;
         if (gDatabase.getInfectedForm(mProtoID, infectedPOID, infectedPSID))
         {
            // Instead of dying, temporarily change to gaia, form a new squad and initiate an infection action (unless we already did)
            if (mPlayerID != 0)
            {
               stop();
               mActions.clearActions();
               //Remove our opps in case anyone is listening for opp notifications.
               removeOpps();

               mFormerParentSquad = getParentID();
               //Remove us from our squad.
               safeDesquad();

               //Give us a new squad.
               BObjectCreateParms objectParms;
               objectParms.mPlayerID=mPlayerID;
               objectParms.mStartBuilt=true;
               objectParms.mProtoObjectID=mProtoID;
               objectParms.mProtoSquadID=infectedPSID;
               objectParms.mPosition=mPosition;
               objectParms.mForward=mForward;
               objectParms.mRight=mRight;
               BEntityIDArray units;
               units.setNumber(1);
               units[0]=mID;
               BSquad* pSquad = gWorld->createSquad(objectParms, &units);

               //Convert to gaia.
               pSquad->changeOwner(0);
               if (getParentSquad())
                  getParentSquad()->getSquadAI()->setMode(BSquadAI::cModeNormal);
               setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateIdle, -1, true, true);

               //-- Add Unit InfectDeath Opportunity
               BUnitOpp* pNewOpp=BUnitOpp::getInstance();
               pNewOpp->init();
               pNewOpp->setType(BUnitOpp::cTypeInfectDeath);
               pNewOpp->setPriority(BUnitOpp::cPriorityDeath);

               pNewOpp->setUserData(static_cast<uint16>(0));
               pNewOpp->generateID();
               if (!addOpp(pNewOpp))
               {
                  BFAIL("Couldn't add an infection death opportunity");
                  BUnitOpp::releaseInstance(pNewOpp);
               }
            }
            return;
         }
      }

      onKillOrTransform(-1);

      BPlayer* pPlayer = getPlayer();
      if (pPlayer)
      {
         pPlayer->removeUnitFromProtoObject(this, mProtoID);
         pPlayer->getTechTree()->checkUnitPrereq(mProtoID); // THIS MODIFIES STUFF
         pPlayer->adjustDeadUnitCount(mProtoID, 1);

         //Test for a power achievement
         if( getFlagKilledByLeaderPower() && isType(gAchievementManager.getFinalKillUnitType()) )
         {
            BEntity *killerEnt = gWorld->getEntity( getKilledByID() );
            if( killerEnt && killerEnt->getPlayerID() != getPlayerID() )
            {
               int unitCount = pPlayer->getNumUnitsOfType(gAchievementManager.getFinalKillUnitType());
               if (unitCount == 0)
               {
                  BPlayer *pKillerPlayer = gWorld->getPlayer(killerEnt->getPlayerID());
                  // [10/7/2008 xemu] note that we are counting the VICTIM's number of units but giving the achievement to the ATTACKER (in this case the player using the power)
                  if (pKillerPlayer)
                     gAchievementManager.updateMisc(pKillerPlayer->getUser(), cMiscATFinalKillByPower);
               }
            }
         }

         // Goto base tracking
         if (getProtoObject()->getGotoType()==cGotoTypeBase)
            pPlayer->removeGotoBase(mID);
         else if (getProtoObject()->getFlagLockdownMenu())
         {
            BSquad* pSquad = getParentSquad();
            if (pSquad && pSquad->getSquadMode()==BSquadAI::cModeLockdown)
               pPlayer->removeGotoBase(mID);
         }

         //Somewhat hacky.  If we lost a goto base, check if it was a real base (not a mobile base)
         if (gUserManager.getPrimaryUser() && gUserManager.getPrimaryUser()->getPlayer() == pPlayer)
         {
            if (getProtoObject()->getGotoType()==cGotoTypeBase)
            {
               //Count up the number of real bases left
               long realBaseCount = 0;
               for(uint i=0; i<pPlayer->getNumberGotoBases(); i++)
               {
                  BEntity *ent = gWorld->getEntity(pPlayer->getGotoBase(i));
                  if( ent && ent->getProtoObject() && ent->getProtoObject()->getGotoType() == cGotoTypeBase )
                     realBaseCount++;
               }
               //If we still have at least one real base left, play the VOG that we lost a base.  If we don't have any left, a different VOG and countdown timer will play elsewhere.
               if (realBaseCount > 0)
               {
                  gSoundManager.playSoundCueByEnum(BSoundManager::cSoundVOGBaseDestroyed);
               }
            }
         }
      }

      // send a notify before we clobber the event listener
      sendEvent(mID, mID, BEntity::cEventKilled, 0);

      // WMJ TODO: make this an event that is fired
      // the world will catch it and log a kill request.
      stop();
      mActions.clearActions();
      //Remove our opps in case anyone is listening for opp notifications.
      removeOpps();

      // Released unitActionPhysics, so no need to keep the physics object around
      releasePhysicsObject();
   }
   setFlagAlive(false);

   // cache the proto squad ID before we de-squad the unit
   // used in the world notify event below
   long protoSquadID = cInvalidProtoID;
   BSquad* pSquad = getParentSquad();
   if (pSquad && pSquad->getNumberChildren() == 1)
      protoSquadID = pSquad->getProtoSquadID();

   // See if anyone wants to respond to our death
   if(pSquad)
      pSquad->createAudioReaction(cSquadSoundChatterReactDeath);

   if (!createdDeathReplacement)
   {
      // Remove the unit from the squad, if he's currently in a squad.
      safeDesquad();
   }

   //-- send an event
   if (!notifySent)
   {
      sendEvent(mID, mID, BEntity::cEventKilled, 0);
      notifySent = true;
   }

   // DPM 7/29/07 - need to insure that kill is accurately called for all unit deaths
   // or stats gathering will contain holes

   // determine the killer of this unit
   /*
   BPlayerID killerPlayerID = cInvalidPlayerID;
   long killerProtoID = cInvalidProtoID;
   long killerProtoSquadID = cInvalidProtoID;

   if (mKilledByID != cInvalidObjectID)
   {
      BEntity* killerEntity = gWorld->getEntity(mKilledByID.asLong());
      if (killerEntity)
      {
         switch (killerEntity->getClassType())
         {
            case BEntity::cClassTypeObject:
            case BEntity::cClassTypeUnit:
               {
                  BObject* pObject = killerEntity->getObject();
                  if (pObject)
                  {
                     killerPlayerID = pObject->getPlayerID();
                     killerProtoID = pObject->getProtoID();
                     BUnit* pKillerUnit = killerEntity->getUnit();
                     if (pKillerUnit)
                     {
                        BSquad* pKillerSquad = pKillerUnit->getParentSquad();
                        if (pKillerSquad)
                           killerProtoSquadID = pKillerSquad->getProtoSquadID();
                     }
                  }
               }
               break;
            case BEntity::cClassTypeSquad:
               {
                  BSquad* pSquad = killerEntity->getSquad();
                  if (pSquad)
                  {
                     killerPlayerID = pSquad->getPlayerID();
                     killerProtoID = pSquad->getProtoObjectID();
                     killerProtoSquadID = pSquad->getProtoSquadID();
                  }
               }
               break;
         }
      }
   }
   */

   // testing a new event that contains the playerIDs of the unit and the killer
   // along with the protoids for storage purposes
   //if (!mFlagRecycled)
   //   gWorld->notify2(BEntity::cEventKilledUnit, mPlayerID, mProtoID, protoSquadID, killerPlayerID, killerProtoID, killerProtoSquadID);

   gWorld->notify(BEntity::cEventKilled, mID, (DWORD)mKilledByID.asLong(), 0);

   //gWorld->updateAllKBsForUnit(this);

   blogtrace("Kill %u hp(%u)", getID(), getHitpoints());
   MVinceEventSync_UnitKilled(this, mKilledByID.asLong());

   gGeneralEventManager.eventTrigger(BEventDefinitions::cGameEntityKilled, mPlayerID, mID, mKilledByID ); 

   // [10/31/2008 xemu] ok, this is kind of awful, but for arcane reasons projectiles pass all unit filters, so we need to filter in code directly.
   // [10/31/2008 xemu] This is specifically to support the "killed by" trigger template used to detect kills-by-Scarab for a scenario-specific Achievement  
   
   //BEntity *pKillerEntity = gWorld->getEntity(mKilledByID);

   if (mKilledByID.getType() != BEntity::cClassTypeProjectile)
      gGeneralEventManager.eventTrigger(BEventDefinitions::cGameEntityKilledByNonProjectile, mPlayerID, mID, mKilledByID ); 

   if(!bKillImmediately)
   {
      //-- Play the death sound
      BCueIndex cueIndex=getProtoObject()->getSound(cObjectSoundDeath);
      if(cueIndex!=cInvalidCueIndex)         
         gWorld->getWorldSoundManager()->addSound(getPosition(), cueIndex, true, cInvalidCueIndex, true, true);         
   }

   // Halwes - 7/7/2008 - Do not do death animation if in cover.
   if(!bKillImmediately && !createdDeathReplacement && !inCover)
   {
      // SLB: This is to prevent objects masquerading as units from using death actions (I'm looking at you "sys_thrownobject").
      BASSERT(!(!getProtoObject() || (getProtoObject()->getProtoVisualIndex() == -1)));

      //-- Add Unit Death Opportunity
      BUnitOpp* pNewOpp=BUnitOpp::getInstance();
      pNewOpp->init();
      pNewOpp->setType(BUnitOpp::cTypeDeath);
      pNewOpp->setPriority(BUnitOpp::cPriorityDeath);
      pNewOpp->generateID();
      pNewOpp->setUserData(getFlagFatalityVictim() ? 1 : 0);
      if (!addOpp(pNewOpp))
      {
         BFAIL("Couldn't add a death opportunity");
         BUnitOpp::releaseInstance(pNewOpp);
      }

      setFlagNoUpdate(false);
   }

   //-- Remove us from any battles we're in
   if (getBattleID() >= 0)
      unitLeftCombat();

   if (!createdDeathReplacement)
   {
      //-- Select socket if this is a building tied to a socket
      BEntityID socketID = cInvalidObjectID;
      const BEntityRef* pSocketRef = getFirstEntityRefByType(BEntityRef::cTypeParentSocket);
      if (pSocketRef)
      {
         socketID = pSocketRef->mID;
         BUnit* pSocketUnit=gWorld->getUnit(socketID);
         if (pSocketUnit)
         {
            // turn on the parent socket rendering at this point.
            pSocketUnit->setFlagNoRender(false);
            // set socket unit visibility
            long numTeams = gWorld->getNumberTeams();
            for (long i = 1; i < numTeams; i++)
            {
               if (isVisible(i))
                  pSocketUnit->makeVisible(i);
               else
                  pSocketUnit->makeInvisible(i);
            }
            // Play fade in animation if this is a settlement
            if (pSocketUnit->isType(gDatabase.getOTIDSettlement()))
            {
               pSocketUnit->setAnimationEnabled(true, true);
               pSocketUnit->unlockAnimation();
               pSocketUnit->setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateResearch, -1, true, true, -1, true);
               pSocketUnit->computeAnimation();
               BASSERT((pSocketUnit->getAnimationState() == BObjectAnimationState::cAnimationStateResearch) && pSocketUnit->isAnimationApplied());
               pSocketUnit->setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateIdle);
            }
         }

      }

      if ( (socketID != cInvalidObjectID) && gUserManager.getPrimaryUser()->getSelectionManager()->isUnitSelected(mID))
      {
//-- FIXING PREFIX BUG ID 4373
         const BUnit* pSocketUnit=gWorld->getUnit(socketID);
//--
         if (pSocketUnit && (pSocketUnit->getPlayerID() == gUserManager.getPrimaryUser()->getPlayerID()))
         {
            // only select the socket if it is our socket
            gUserManager.getPrimaryUser()->getSelectionManager()->selectUnit(socketID);
            if (gUserManager.getPrimaryUser()->getCommandObject() == mID)
            {
               gUserManager.getPrimaryUser()->setCommandObject(socketID);
               gUserManager.getPrimaryUser()->setFlagCommandMenuRefresh(true);
               gUserManager.getPrimaryUser()->setFlagCommandMenuRefreshTrainProgressOnly(false);
            }
         }
      }
      if (socketID != cInvalidObjectID && gGame.isSplitScreen() && gUserManager.getSecondaryUser()->getSelectionManager()->isUnitSelected(mID))
      {
         BUnit* pSocketUnit=gWorld->getUnit(socketID);
         if (pSocketUnit && (pSocketUnit->getPlayerID() == gUserManager.getSecondaryUser()->getPlayerID()))
         {
            // only select the socket if it is our socket
            gUserManager.getSecondaryUser()->getSelectionManager()->selectUnit(socketID);
            if (gUserManager.getSecondaryUser()->getCommandObject() == mID)
            {
               gUserManager.getSecondaryUser()->setCommandObject(socketID);
               gUserManager.getSecondaryUser()->setFlagCommandMenuRefresh(true);
            }
         }
      }

      // destroy carried object if there is one
      if (mCarriedObject.isValid())
         destroyCarriedObject();

      //-- Object kill
      BObject::kill(bKillImmediately);

      //-- update the obstruction
      deleteObstruction();

      if (mSelectionDecal != -1)
         gDecalManager.destroyDecal(mSelectionDecal);
      mSelectionDecal = -1;
      
      // Halwes - 7/7/2008 - Go ahead and kill us if we are in cover.
      if (bKillImmediately || inCover)
      {
         destroy();
      }

      //-- Stop the exist sound
      BEntityID squadID = cInvalidObjectID;
      if(pSquad)
         squadID = pSquad->getID();
      stopExistSound(squadID);
   }

   if (!bKillImmediately && mDamageTracker && mpVisual && (getProtoID() != gDatabase.getPOIDPhysicsThrownObject()))
   {
      const BDamageTemplate *pTemplate = gDamageTemplateManager.getDamageTemplate(mpVisual->getDamageTemplateID());
      if (pTemplate)
      {
         computeDopple();

         // VAT: 11/06/08 if this is a building destroyed by a parent base, we only 
         // execute the final death event so we don't have lots of unnecessary particles / etc
         bool onlyFinalEvent = getFlagDestroyedByBaseDestruction();

         if (getFlagShatterOnDeath() && pTemplate->getShatterDeathEvent())
            mDamageTracker->shatterDeath(pTemplate, this, NULL, 1000.0f);
         else 
            mDamageTracker->updatePercentageBaseDamage(0.0f, pTemplate, this, NULL, 1000.0f, onlyFinalEvent);
         setHitpoints(0.0f);
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::killHero()
{
   //XXXHalwes - 5/2/2008 - ?
   //const BUnitActionPhysics* pAction = reinterpret_cast<const BUnitActionPhysics*>(getActionByTypeConst(BAction::cActionTypeUnitPhysics));
   //if (pAction && pAction->getFlagCompleteOnInactivePhysics())
   //   return;

   #ifdef SYNC_Unit
      syncUnitData("BUnit::killHero mID", mID.asLong());
   #endif

   //XXXHalwes - 5/2/2008 - ?
   //stopAllHardpointSounds();

   //XXXHalwes - 5/2/2008 - How to handle garrisoned/cover heroes?
   // Unload dying cover unit
   if (getFlagGarrisoned() && getFlagInCover())
   {
//-- FIXING PREFIX BUG ID 4376
      const BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeContainingUnit);
//--
      if (pRef)
      {
         BUnit* pUnit = gWorld->getUnit(pRef->mID);
         if (pUnit)
         {
            pUnit->unloadUnit(getID(), false);
         }
      }
   }

   mActions.clearActions();
   // Remove our opps in case anyone is listening for opp notifications.
   removeOpps();

   // See if anyone wants to respond to our death
   BSquad* pParentSquad = getParentSquad();
   if (pParentSquad)
   {
      pParentSquad->createAudioReaction(cSquadSoundChatterReactDeath);
   }

   //XXXHalwes - 5/2/2008 - Do we need a hero killed event?
   // Send an event
   sendEvent(mID, mID, BEntity::cEventKilled, 0);
   gWorld->notify(BEntity::cEventKilled, mID, (DWORD)mKilledByID.asLong(), 0);
   blogtrace("Kill %u hp(%u)", getID(), getHitpoints());
   MVinceEventSync_UnitKilled(this, mKilledByID.asLong());
   gGeneralEventManager.eventTrigger(BEventDefinitions::cGameEntityKilled, mPlayerID, mID, mKilledByID ); 

   // Play the death sound
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject)
   {
      BCueIndex cueIndex = pProtoObject->getSound(cObjectSoundDeath);
      if (cueIndex != cInvalidCueIndex)
      {
         gWorld->getWorldSoundManager()->addSound(this, -1, cueIndex, true, cInvalidCueIndex, true, true);
      }
   }

   // Add Unit HeroDeath Opportunity
   BUnitOpp* pNewOpp = BUnitOpp::getInstance();
   pNewOpp->init();
   pNewOpp->setType(BUnitOpp::cTypeHeroDeath);
   pNewOpp->setPriority(BUnitOpp::cPriorityDeath);
   pNewOpp->generateID();
   //pNewOpp->setUserData(getFlagFatalityVictim() ? 1 : 0);
   if (!addOpp(pNewOpp))
   {
      BFAIL("Couldn't add a hero death opportunity");
      BUnitOpp::releaseInstance(pNewOpp);
   }

   setFlagNoUpdate(false);

   // Remove us from any battles we're in
   if (getBattleID() >= 0)
      unitLeftCombat();

   // Remove from selection
   BPlayerID playerID = getPlayerID();
   BUser* pUser = gUserManager.getUserByPlayerID(playerID);
   if (pUser)
   {
      BSelectionManager* pSelectionManager = pUser->getSelectionManager();
      if (pSelectionManager && pSelectionManager->isUnitSelected(mID))
      {
         pSelectionManager->unselectUnit(mID);
      }
   }

   // Turn selection off
   setFlagIgnoreUserInput(true);   

   // Set HP to 1
   setHitpoints(1.0f);
   
   
   if ( getFlagHasShield())
   {
      setShieldpoints(0.0f);

      if (pParentSquad)
      {
         pParentSquad->setFlagShieldDamaged(true);
      }
   }

   // Have squad AI ignore hero as a target
   setFlagDontAutoAttackMe(true);

   // Reduce LOS for downed hero
   float LOS = getLOS();
   if (LOS > cFloatCompareEpsilon)
   {
      setLOSScalar(gDatabase.getHeroDownedLOS() / LOS);
   }

   setFlagDown(true);

   // Make sure we're in a pathable position, teleport us to one otherwise
   BSimHelper::findPosForAirborneSquad(pParentSquad, gDatabase.getHeroMaxDeadTransportDist(), true);
}

//==============================================================================
// BUnit::isType
//==============================================================================
bool BUnit::isType(long type) const
{
   return getProtoObject()->isType(type);
}

//==============================================================================
// BUnit::getAttackRating
//==============================================================================
float BUnit::getAttackRating(BDamageTypeID damageType) const
{
   return getProtoObject()->getAttackRating(damageType);
}

//==============================================================================
// BUnit::getDefenseRating
//==============================================================================
float BUnit::getDefenseRating() const
{
   return getProtoObject()->getDefenseRating();
}

//==============================================================================
//==============================================================================
uint BUnit::getAttackGrade(BDamageTypeID damageType) const
{
   return getProtoObject()->getAttackGrade(damageType);
}

//==============================================================================
//==============================================================================
/*long BUnit::getAutoSquadMode() const
{
   BSquad* pSourceSquad=getParentSquad();
   if(pSourceSquad)
      return pSourceSquad->getAutoSquadMode();
   else
      return -1;
}*/

 float cfFriction = 1.25f * 2.0f;
 float cRange = 2.0f;
 float cSlowRange = 8.0f;
 float cfMaxFlipTime = 2.0f;

//=============================================================================
//=============================================================================
/*void BUnit::updateVehicleSimulation( float elapsedTime )
{
   elapsedTime; //currently not used

   if (getFlagPhysicsControl())
   {
      if (mPhysicsVehicleID != -1)
      {
         BSquad *pSquad = gWorld->getSquad(getParentID());
         if (!pSquad)
            return;

         BVehicleControlData data(mPhysicsVehicleID);
         data.mbReverse = false;
        

         //-- try to steer towards our location
         BVector squadTarget = pSquad->getFormationPositionAtTarget(mID);
         float distanceRemaining = mTargetLocation.xzDistance(mPosition);
         float distanceRemainingToFinal = squadTarget.xzDistance(mPosition);


         bool noTurn = false;
         bool doBackup = false;
         bool doWait = false;
         bool roughTerrain = false;


         BVector vel;
         mpPhysicsObject->getLinearVelocity(vel);
         float velocityLen = vel.length();

         BVector angularVel;
         mpPhysicsObject->getAngularVelocity(angularVel);
      

        
         float minStoppingDistance = velocityLen / (cfFriction);
         if (minStoppingDistance == 0.0f)
            minStoppingDistance = 0.01f;

         //-- we might be in rough terrain
         float angle = getUp().angleBetweenVector(cYAxisVector);
         if (angle > cPiOver12)
            roughTerrain = true; 

         //-- we might be flipped over
         if ((cYAxisVector.dot(getUp()) < 0.0f))
         {
            mFlippedTime += elapsedTime;
            
            if (mFlippedTime > cfMaxFlipTime)
            {
               //-- unflip us
               BPhysicsMatrix rot;
               mpPhysicsObject->getRotation(rot);

               BVector forward = rot.getForward();
               BVector right = cYAxisVector.cross(forward);
               forward = right.cross(cYAxisVector);

               rot.setUp(cYAxisVector);
               rot.setRight(right);
               rot.setForward(forward);
               mpPhysicsObject->setRotation(rot);
               return;
            }
          
         }


         //-- we slow down and tell our squad we are in position through end move. 
         //-- for smooth motion, we hope to stay behind our target position, but this
         //-- does not always happen
         if (( distanceRemaining <= minStoppingDistance) || (distanceRemainingToFinal < cRange) || !isMoving())
         {

            
            if (distanceRemainingToFinal < cRange || !isMoving())
            {
               //-- really need to stop here
               data.mbHandbrakePressed = true;
               data.mInputX = 0.0f;
               data.mInputY = 0.0f;
              
               endMove();
            }
            else
            {
               //-- slow down, prepare to stop
               data.mbHandbrakePressed = false;
               data.mInputY = 1.0f - (distanceRemaining/minStoppingDistance);
               data.mInputX = 0.0f;
             
            }
          
         }
         else   //-- we actually have to try to reach the goal
         {

            bool slowDown = false;
            if (distanceRemainingToFinal < cSlowRange)
            { 
              slowDown = true;
            }

            //-- make sure the brake is off
            data.mbHandbrakePressed = false;

            //-- fix this!  What if the vectors are the same?
            //-- direction we need to go
            BVector newXZDir = mTargetLocation - mPosition;
            newXZDir.y = 0.0f;
            newXZDir.normalize();

            //-- direction we are heading now
            BVector currentXZDir = getForward();
            currentXZDir.y = 0.0f;
            currentXZDir.normalize();

            //-- we want to turn towards that position
            //-- what's the angle?
            float yawAngle = newXZDir.angleBetweenVector(currentXZDir);

            //-- this is our margin of error for forward motion
            if (yawAngle < cPi/16)
               noTurn = true;

            //-- we may want to wait
            if (yawAngle > (cPiOver2))
            {
              
               if (pSquad->getPosition().almostEqual(pSquad->getTargetLocation()))
                  doBackup = true;
               else
                  doWait = true;
            }

            BVector fCross=newXZDir.cross(currentXZDir);
            //If the cross product is positive in the Y direction, then the new
            //orientation is counter clockwise from the current orientation (so we
            //want to yaw in the negative direction).
            if (fCross.y > 0.0f)
               yawAngle=-yawAngle;


            //-- assume that we are not going to have to turn
            data. mInputX = 0.0f;

            //-- assume that we are moving at a slow speed
            if (doWait)
            {
               data.mInputY = -0.20f;
            }
            else
               data.mInputY = -0.35;

            //-- if we are allowed to turn, then we calculate which way and how much
            //-- to turn the wheel
            if (!noTurn)
            {

               if (yawAngle > cFloatCompareEpsilon)
               {
                  data.mInputX = 1.0f;
               }
               else if (yawAngle < cFloatCompareEpsilon)
               {
                  data.mInputX = -1.0f;
               }
            }
            else //-- we are moving in a straight line (generally), so hit the gas
            {
               if (!roughTerrain && !doBackup && !slowDown)
                  data.mInputY = -1.0f;
            }
         }

         if (doBackup && isMoving())
         {
            data.mInputY =0.5f;
            data.mInputX = -data.mInputX;
            data.mbHandbrakePressed = false;
            data.mbReverse = true;
         }


         //-- actually tell the vehicle structure what has changed
         gWorld->getPhysicsWorld()->updateTestVehicle(&data);       

      }
   }
}*/

//=============================================================================
// BUnit::updateTurn
//=============================================================================
void BUnit::updateTurn( float elapsedTime )
{
   elapsedTime;
   
   //DCP 05/16/07: Turning this off until we get the new movement done.
   /*

   // General:
   // Bring the unit's forward vector toward the goal vector while obeying maximum turn rate limits.
   // For simplicity, make rotational accel/decel rate be proportional to the unit's maximum turn rate.

   // Implementation:
   // Dot the current forward vector with the normalized goal vector to find the cosine of the angular error,
   // then find the error angle with arc cosine. Cross the current forward vector with the goal forward vector.
   // The sign of the y component gives the turn direction. Calculate the current maximum allowable turn rate
   // (capped at maxTurnRate) to avoid overshooting the goal, given accel limits. Attempt to match the target
   // turn rate without exceeding angular accel limit. Update the unit's orientation
   if (!isAlive())
      return;

   if (!getParentSquad())
      return;

   // If the goal vector is still zero'ed out - bail. Nothing to see here.
   if (!getFlagHasGoalVector())
      return;

   // Figure out whether to turn toward or away from the goal vector since it may be more appropriate
   // to back into position. Consider: distance to next waypoint, current relative bearing to goal vector, and outbound heading
   // from next point (either to the succeeding point or to squad facing if the next point is the last)
   BVector goalHeading = mGoalVector;

   // Project into the XZ plane by zeroing out the Y components
   goalHeading.y = 0.0f;
   goalHeading.normalize();

   BVector forwardXZ = mForward;
   forwardXZ.y = 0.0f;
   forwardXZ.normalize();

   float forwardDotGoal = forwardXZ.dot(goalHeading);

   // Reverse goal heading if vehicle is going to back up   
   if(!getFlagHasFacingCommand() && getProtoObject()->getFlagOrientUnitWithGround())
   {
      BSquad* pSquad = getParentSquad();
      if (pSquad)
      {
         float maxBackUpDist = 6.0f * getObstructionRadius();

         BEntity* pEntity;
         if(mMovementData == BEntity::cMovementDataUnderParentControl)
            pEntity = pSquad;
         else
            pEntity = this;

         BPath* pPath = pEntity->getPath();
         long numWaypoints = pPath->getNumberWaypoints();
         long currentWaypointIndex = pEntity->getCurrentWaypoint();

         BVector nextLeg;
         if( numWaypoints > (currentWaypointIndex + 1) ) // We have another waypoint beyond the next one
         {
            BVector nextWaypoint = pPath->getWaypoint(currentWaypointIndex + 1);
            if(pEntity == pSquad)
               pSquad->getFormationPositionAtTarget(mID, nextWaypoint);
            nextLeg = nextWaypoint - mTargetLocation;
         }
         else
         {
            nextLeg = pSquad->getTargetFacing();
            if(nextLeg == cInvalidVector)
               nextLeg = -goalHeading;
         }
         nextLeg.y = 0.0f;
         nextLeg.normalize();

         float goalDotNextLeg = goalHeading.dot(nextLeg);

         float distToNextPoint = distanceToCurrentWaypoint();
         if ((distToNextPoint < maxBackUpDist) && (forwardDotGoal < 0.0f) && (goalDotNextLeg < 0.0f))
         {
            goalHeading *= -1.0f;
            forwardDotGoal = forwardXZ.dot(goalHeading);
         }
      }
   }

   // Calculate the angle error
   float angleError = (float) acos( forwardDotGoal );

   if (fabs(angleError) < 0.01f)
   {
      // Clear the goal vector if it was set due to a facing command so that we stop turning
      if (getFlagHasFacingCommand())
         clearGoalVector();
      return;
   }

   BVector axisOfRotation = forwardXZ.cross(goalHeading);

   float maxTurnRateRadians = mMaxTurnRate * (3.14f / 180.0f);
   float angAccel = 2.0f * maxTurnRateRadians; // Coefficient arbitrarily selected - probably not necessary to expose to design
   float timeToTurn = (float) sqrt(2.0f * angleError / angAccel);

   float goalTurnRate = 0.5f * angAccel * timeToTurn;
   if (goalTurnRate > maxTurnRateRadians)
      goalTurnRate = maxTurnRateRadians;

   float turnDir = 1.0f;
   if (axisOfRotation.y < 0.0f)
   {
      angleError *= -1.0f;
      goalTurnRate *= -1.0f;
      turnDir *= -1.0f;
   }

   float goalDeltaV = goalTurnRate - mCurrentTurnRate;
   float maxDeltaV = angAccel * elapsedTime;

   if( fabs(goalDeltaV) <= maxDeltaV)
      mCurrentTurnRate = goalTurnRate;
   else
      mCurrentTurnRate += turnDir * maxDeltaV;

   if (mCurrentTurnRate > maxTurnRateRadians)
      mCurrentTurnRate = maxTurnRateRadians;
   else if (mCurrentTurnRate < -maxTurnRateRadians)
      mCurrentTurnRate = -maxTurnRateRadians;

   float turnAngle = mCurrentTurnRate * elapsedTime;
   if ( (turnAngle > 0.0f && angleError > 0.0f && turnAngle > angleError) || (turnAngle < 0.0f && angleError < 0.0f && turnAngle < angleError) )
      turnAngle = angleError;

   if (fabs(turnAngle) > cFloatCompareEpsilon) 
   {
      yaw(turnAngle);
      mRight = mUp.cross(mForward);
   }*/
}

/*
//==============================================================================
// BUnit::updateGarrisoned
//==============================================================================
void BUnit::updateGarrisoned(bool moving)
{
   if(!getFlagHasGarrisoned() || !moving || !mpEntityRefs)
      return;
   long count=mpEntityRefs->getNumber();
   for(long i=0; i<count; i++)
   {
      BEntityRef ref=(*mpEntityRefs)[i];
      if(ref.mType!=BEntityRef::cTypeContainUnit)
         continue;
      BUnit* pUnit=gWorld->getUnit(ref.mID);
      if(pUnit)
      {
         pUnit->setPosition(mPosition);
         pUnit->updateObstruction();
         BSquad* pSquad=pUnit->getParentSquad();
         if(pSquad)
         {
            pSquad->setPosition(mPosition);
            pSquad->updateObstruction();
         }
      }
   }
}
*/


//=============================================================================
// BUnit::toggleAddResource
// Function to enable / disable the add resources for this object
//=============================================================================
void BUnit::toggleAddResource(bool enable)
{
   if (mFlagAddResourceEnabled == enable)
      return;

   mFlagAddResourceEnabled = enable;

   BPlayer *pPlayer = getPlayer();
   const BProtoObject *pProtoObject = getProtoObject();
   if (!pProtoObject || !pPlayer)
      return;

   // Resource additions
   long resourceID = pProtoObject->getAddResourceID();
   if (resourceID!=-1)
   {
#ifdef SYNC_Player
      syncPlayerCode("BUnit::onBuilt pPlayer->addResrouce");
#endif
      float amount = (enable) ? pProtoObject->getAddResourceAmount() : -pProtoObject->getAddResourceAmount();
      if (gWorld->getFlagCoop() && pPlayer->isHuman() && !gConfig.isDefined(cConfigCoopSharedResources))
      {
         // Co-op support: Give resource to all players on the team
         if (gDatabase.getResourceDeductable(resourceID))
            amount *= gDatabase.getCoopResourceSplitRate();
         //-- FIXING PREFIX BUG ID 4538
         const BTeam* pTeam=pPlayer->getTeam();
         //--
         int playerCount=pTeam->getNumberPlayers();
         for (int i=0; i<playerCount; i++)
         {
            BPlayer* pTeamPlayer=gWorld->getPlayer(pTeam->getPlayerID(i));
            pTeamPlayer->addResource(resourceID, amount, BPlayer::cFlagNormal, true);
            gWorld->notify(BEntity::cEventBuildingResource, mID, resourceID, pTeamPlayer->getID());
         }
      }
      else
      {
         pPlayer->addResource(resourceID, amount, BPlayer::cFlagNormal, true);
         gWorld->notify(BEntity::cEventBuildingResource, mID, resourceID, pPlayer->getID());
      }
   }
}

//=============================================================================
// BUnit::transform
// Helper version of function for transforming to new proto object type.
//=============================================================================
void BUnit::transform(long newProtoObjectID, bool techUpgrade)
{
   const BProtoObject *pOldProtoObject = this->getProtoObject();
   BProtoObject *pNewProtoObject = getPlayer()->getProtoObject(newProtoObjectID);
   if (pOldProtoObject && pNewProtoObject && pOldProtoObject != pNewProtoObject)
   {
      //#ifndef BUILD_FINAL
      //   BASSERTM(pOldProtoObject->getFlagKBAware() == pNewProtoObject->getFlagKBAware(), "Possibly bad stuff!  Transforming a unit between types that are KB aware and types that are not!");
      //#endif
      this->transform(pOldProtoObject, pNewProtoObject, techUpgrade);
   }
}

//=============================================================================
// BUnit::transform
//=============================================================================
void BUnit::transform(const BProtoObject* pOldProto, const BProtoObject* pNewProto, bool techUpgrade)
{
   BASSERT(pOldProto);
   BUnitTransformData transformData;
   preTransform(pOldProto, pNewProto, techUpgrade, transformData);
   postTransform(pNewProto, techUpgrade, transformData, pOldProto->getID());
}

//=============================================================================
// BUnit::preTransform
//=============================================================================
void BUnit::preTransform(const BProtoObject* pOldProto, const BProtoObject* pNewProto, bool techUpgrade, BUnitTransformData& transformData)
{
   // SLB: Create dopples to hide transformation
   computeDopple();

   #ifdef SYNC_Unit
      syncUnitData("BUnit::preTransform mID", mID.asLong());
      syncUnitData("BUnit::preTransform oldProto", pOldProto->getName());
      syncUnitData("BUnit::preTransform newProto", pNewProto->getName());
   #endif

   BProtoObjectID oldProtoID = pOldProto->getID();
   BProtoObjectID newProtoID = pNewProto->getID();

   onKillOrTransform(newProtoID);

   //-- remove old los
   transformData.mUpdateLOS = (getFlagLOSMarked() && (pOldProto->getProtoSimLOS() != pNewProto->getProtoSimLOS()));
   if (transformData.mUpdateLOS)
      markLOSOff();

   BPlayer* pPlayer = getPlayer();

   if (!techUpgrade)
      pPlayer->removeUnitFromProtoObject(this, oldProtoID);

   // Goto base tracking
   if (pOldProto->getGotoType()==cGotoTypeBase && pNewProto->getGotoType()!=cGotoTypeBase)
      pPlayer->removeGotoBase(mID);
   else if (pOldProto->getGotoType()!=cGotoTypeBase && pOldProto->getFlagLockdownMenu())
   {
      //-- FIXING PREFIX BUG ID 4378
      const BSquad* pSquad = getParentSquad();
      //--
      if (pSquad && pSquad->getSquadMode()==BSquadAI::cModeLockdown)
         pPlayer->removeGotoBase(mID);
   }

   //-- update the obstruction
   deleteObstruction();

   //-- physics object
   releasePhysicsObject();

   //-- Stop our old exist sound
   stopExistSound();

   //-- Ratio for hit point adjustment
   if (pOldProto->getHitpoints() != 0.0f)
      transformData.mHpRatio = mHitpoints / pOldProto->getHitpoints();

   //-- Ratio for resource adjustment
   if (pOldProto->getResourceAmount() != 0.0f)
      transformData.mResourceRatio = mResourceAmount / pOldProto->getResourceAmount();

   //-- Ratio for ammunition adjustment
   transformData.mAmmoRatio = 1.0f;              //Set to 1.0 if we don't have a maxAmmo.  MWC
   if (pOldProto->getMaxAmmo() != 0.0f)
      transformData.mAmmoRatio = mAmmunition / pOldProto->getMaxAmmo();

   // The unit may be performing an action that needs to change in some way - the next section will reinitialize any actions as required.
   // Preserve the opportunity array, Remove existing actions, Ensure Opportunities are unchanged, and update the opportunities.
   // TRB 10/2/08 - Remove the actions before caching the opps in case the actions had generated some of the opps.  Once the action goes
   // away, the opps they generated will be invalid objects and could cause asserts or crashes if used later.
   removeActions();

   // SLB: Remove all unit move opportunities if we're becoming a physics vehicle
   BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pNewProto->getPhysicsInfoID(), true);
   if(pInfo && pInfo->isVehicle())
   {
      for (long i = mOpps.getSize() - 1; i >= 0; i--)
      {
         const BUnitOpp* pOpp = mOpps[i];
         if (pOpp->getType() == BUnitOpp::cTypeMove)
            mOpps.removeIndex(i);
      }
   }

   transformData.mOpps = mOpps;
}

//=============================================================================
// BUnit::postTransform
//
// Note that oldProtoID is only for the non techUpgrade where the old proto data actually exists.
// In the techUpgrades we completely overwrite the old data, and pass it in as the modified proto.
//=============================================================================
void BUnit::postTransform(const BProtoObject* pModifiedProto, bool techUpgrade, BUnitTransformData& transformData, BProtoObjectID oldProtoID)
{
   //only specify an oldProtoID if this is not a techUpgrade
   BASSERT((techUpgrade && (oldProtoID == -1)) || (!techUpgrade && (oldProtoID != -1)));

   BProtoObjectID protoID = pModifiedProto->getID();

   BPlayer* pPlayer = getPlayer();

   //-- update the proto ID
   if (!techUpgrade)
      setProtoID(protoID);

   if (!techUpgrade)
   {
      pPlayer->addUnitToProtoObject(this, protoID);
      pPlayer->getTechTree()->checkUnitPrereq(oldProtoID);
      pPlayer->getTechTree()->checkUnitPrereq(protoID);
   }

   // Goto base tracking
   if (pModifiedProto->getGotoType()==cGotoTypeBase)
   {
      pPlayer->addGotoBase(mID);
   }
   else if (pModifiedProto->getGotoType()!=cGotoTypeBase && pModifiedProto->getFlagLockdownMenu())  //What is this for, and does this safely add bases without duplicates?  MWC
   {
//-- FIXING PREFIX BUG ID 4379
      const BSquad* pSquad = getParentSquad();
//--
      if (pSquad && pSquad->getSquadMode()==BSquadAI::cModeLockdown)
         pPlayer->addGotoBase(mID);
   }

   //-- set new los
   if (transformData.mUpdateLOS)
      markLOSOn();

   //-- transform our squad's proto squad if it's a auto-created unit proto squad
   BSquad* pSquad=getParentSquad();

   if (pSquad && !techUpgrade)
   {
      if (pSquad->getProtoSquad()->getProtoObjectID() == oldProtoID)
      {
         pPlayer->removeSquadFromProtoSquad(pSquad, pSquad->getProtoSquadID());
         pSquad->setProtoID(pModifiedProto->getProtoSquadID());
         pPlayer->addSquadToProtoSquad(pSquad, pSquad->getProtoSquadID());
      }
   }

   setFlagRotateObstruction(pModifiedProto->getFlagRotateObstruction());
   if (getFlagAttached() || getFlagGarrisoned())
      setFlagCollidable(false);
   else
      setFlagCollidable(pModifiedProto->getFlagCollidable());
   setFlagTiesToGround(!pModifiedProto->getFlagNoTieToGround());

   setFlagHasHPBar(pModifiedProto->getFlagHasHPBar());
   setFlagDopples(pModifiedProto->getFlagDopples());
   setFlagDieAtZeroResources(pModifiedProto->getFlagDieAtZeroResources());
   setFlagUnlimitedResources(pModifiedProto->getFlagUnlimitedResources());

   setFlagInvulnerable(pModifiedProto->getFlagInvulnerable());
   if (mPlayerID == 0 && pModifiedProto->getFlagInvulnerableWhenGaia())
      setFlagInvulnerable(true);

   setFlagHasShield(pModifiedProto->getFlagHasShield());
   setFlagFullShield(pModifiedProto->getFlagFullShield());

   if (getFlagHasShield())
   {
      if (pSquad)
      {
         pSquad->setFlagHasShield(true);
         BAction* pAction = pSquad->getActionByType(BAction::cActionTypeSquadShieldRegen);
         if (!pAction)
         {
            pAction = gActionManager.createAction(BAction::cActionTypeSquadShieldRegen);
            if (pAction)
               pSquad->addAction(pAction);
         }
         //Compare the proto shield points with the units current shield points incase we increased the shields.  MWC
         if (getShieldpoints() < pModifiedProto->getShieldpoints() && !pSquad->getFlagShieldDamaged())  
            pSquad->setFlagShieldDamaged(true);
      }
      else
      {
         BAction* pAction = getActionByType(BAction::cActionTypeUnitShieldRegen);
         if (!pAction)
         {
            pAction = gActionManager.createAction(BAction::cActionTypeUnitShieldRegen);
            if (pAction)
               addAction(pAction);
         }
      }
   }

   //-- Update our new obstruction radii
   mObstructionRadiusX = pModifiedProto->getObstructionRadiusX();
   mObstructionRadiusY = pModifiedProto->getObstructionRadiusY();
   mObstructionRadiusZ = pModifiedProto->getObstructionRadiusZ();

   createObstruction(pModifiedProto->getFlagPlayerOwnsObstruction());

   //-- sim position
   // SLB: We don't want to do this
   //updateSimPosition();

   //-- sim bounding box
   float yOffset = mObstructionRadiusY;
   if (pModifiedProto->getMovementType() == cMovementTypeAir)
      yOffset = 0.0f;

   BVector minPoint(-mObstructionRadiusX, -mObstructionRadiusY + yOffset, -mObstructionRadiusZ);
   BVector maxPoint(mObstructionRadiusX, mObstructionRadiusY + yOffset, mObstructionRadiusZ);
   mSimBoundingBox.initialize(minPoint, maxPoint);
   mSimBoundingBox.translate(mPosition);

   if (pModifiedProto->getPhysicsInfoID() != -1)
      createPhysicsObject(pModifiedProto->getPhysicsInfoID(), NULL, false, false);

   //-- adjust hit points
   setHitpoints(pModifiedProto->getHitpoints() * transformData.mHpRatio);


   // Hit zones ?
//-- FIXING PREFIX BUG ID 4383
   const BHitZoneArray* pHitZoneList = ((BProtoObject*)pModifiedProto)->getHitZoneList();
//--
   if (pHitZoneList->getNumber() > 0)
   {
      if (!mpHitZoneList)
         mpHitZoneList = new BHitZoneArray();
      *mpHitZoneList = *pHitZoneList;
   }
   else
   {
      if (mpHitZoneList)
      {
         delete mpHitZoneList;
         mpHitZoneList=NULL;
      }
   }

   //-- adjust resource amount
   mResourceAmount = pModifiedProto->getResourceAmount() * transformData.mResourceRatio;

   //-- adjust ammunition
   if (pModifiedProto->getMaxAmmo() > 0.0f)
   {
      mAmmunition = pModifiedProto->getMaxAmmo() * transformData.mAmmoRatio;
      setFlagUsesAmmo(true);

      // Auto create the persistent ammo regen action
      if (getActionByTypeConst(BAction::cActionTypeUnitAmmoRegen) == NULL)
      {
         BUnitActionAmmoRegen* pAction = (BUnitActionAmmoRegen*)gActionManager.createAction(BAction::cActionTypeUnitAmmoRegen);
         if (pAction)
         {
            addAction(pAction);
         }
      }
   }
   else
   {
      setFlagUsesAmmo(false);
   }

   ///////////////////////////////////////////////////////////////////////////
   //DJBFIXME:
   //This should be moved to a build tool when we get one, there is no reason
   //to do these calculations in-game since all the data is static!
   ///////////////////////////////////////////////////////////////////////////
   if (pModifiedProto->getTactic() && pModifiedProto->getTactic()->getAnimInfoLoaded() == false)
   {
      loadTacticAnimInfo(pModifiedProto);
   }

   // Create any needed unique tech tree nodes
   uint commandCount = pModifiedProto->getNumberCommands();
   if (commandCount > 0)
   {
      BPlayer* pPlayer = getPlayer();
//-- FIXING PREFIX BUG ID 4380
      const BTeam* pTeam=pPlayer->getTeam();
//--
      int playerCount=pTeam->getNumberPlayers();
      long unitID = mID.asLong();
      for (uint i = 0; i < commandCount; i++)
      {
         BProtoObjectCommand command = pModifiedProto->getCommand(i);
         if (command.getType() == BProtoObjectCommand::cTypeResearch)
         {
            if (gWorld->getFlagCoop() && pPlayer->isHuman())
            {
               for (int j=0; j<playerCount; j++)
                  gWorld->getPlayer(pTeam->getPlayerID(j))->getTechTree()->addUnitRef(command.getID(), unitID);
            }
            else
               pPlayer->getTechTree()->addUnitRef(command.getID(), unitID);
         }
      }
   }

   if (pModifiedProto->getFlagBuildingCommands() || pModifiedProto->getFlagBuild() || pModifiedProto->getFlagUseBuildingAction())
   {
      if (getActionByTypeConst(BAction::cActionTypeUnitBuilding) == NULL)  //Is this the correct way to test for existence?  Are there any units where this would apply that we can test? MWC
      {
         // Auto create the persistent building action
         BUnitActionBuilding* pAction = (BUnitActionBuilding*)gActionManager.createAction(BAction::cActionTypeUnitBuilding);
         if (pAction)
         {
            addAction(pAction);
         }
      }
   }


   // Resolve damage tracker 
   //
   // Transform the damage tracker from one unit to the next.  Check to see if the both units
   // have damage template ID.  If they do then we assume that units have the same hierarchy 
   // and consequently the mesh render mask can be copied over and the damage tracker can persist.  
   // Else the mask isn't copied and the damage tracker is cleared for a new one to be created 
   // next frame.

   //-- get original template ID
   long originalDamageTemplateID = -1;
   long newDamageTemplateID = -1;
   long originalModelIndex = -1;
   BBitArray originalMask;

   BVisual *pVisual = getVisual();
   if (pVisual)
   {
      originalDamageTemplateID = pVisual->getDamageTemplateID();
      BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
      if (pGrannyInstance)
      {
         originalMask = pGrannyInstance->getMeshRenderMask();
         originalModelIndex = pGrannyInstance->getModelIndex();
      }
   }


   //-- visual
   setVisual(pModifiedProto->getProtoVisualIndex(), pModifiedProto->getVisualDisplayPriority());


   //-- get new template ID
   pVisual = getVisual();
   if (pVisual)
   {
      newDamageTemplateID = pVisual->getDamageTemplateID();
   }

   // Transform damage tracker accordingly 
   if((originalDamageTemplateID == newDamageTemplateID) &&
      (originalDamageTemplateID != -1))
   {
      // Damage templates are the same, copy the mesh render mask
      BGrannyInstance* pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
      if (pGrannyInstance && (pGrannyInstance->getModelIndex() == originalModelIndex))
      {
         pGrannyInstance->setMeshRenderMask(originalMask);
      }
   }
   else
   {
      // Damage templates are different to clear out the damage tracke
      if(mDamageTracker)
      {
         delete mDamageTracker;
         mDamageTracker=NULL;
      }
   }




   if (pModifiedProto->getFlagTrackPlacement())
      BPlacementRules::getUnitsToTrack().add(mID);

   //-- Play our new exist sound
   startExistSound(pModifiedProto);
  
   if (getFlagBuilt())
   {
      // Clear all persistent actions for the old tactic.  New persistent actions
      // for the new tactic will get created right after this.
      removePersistentTacticActions();

      if (getParentSquad())
         getParentSquad()->removePersistentTacticActions();

      onBuilt(false, false, true, false);
   }   

   //Init the visual ammo.
   initVisualAmmo();

   // Make sure we have enough attack timers
//-- FIXING PREFIX BUG ID 4384
   const BTactic* pTactic = pModifiedProto->getTactic();
//--
   if (pTactic != NULL)
   {
      int newWeaponCount = pTactic->getNumberWeapons() - mAttackWaitTimer.getNumber();
      if (newWeaponCount > 0)
      {
         for (int i=0; i<newWeaponCount; i++)
            mAttackWaitTimer.add(BAttackTimer());
      }
   }

   // Make sure we have the right number of hard point states
   long hpcount = pModifiedProto->getNumberHardpoints();
   mHardpointState.setNumber(hpcount);

   // The unit may be performing an action that needs to change in some way - the next section will reinitialize any actions as required.
   // Preserve the opportunity array, Remove existing actions, Ensure Opportunities are unchanged, and update the opportunities.
      mOpps = transformData.mOpps;
         updateOpps(cFloatCompareEpsilon);

   // MS 5/29/2008: JIRA PHX-6911, we need to add a power entry if this unit grants a power
   const BProtoObject* pProtoObject=getProtoObject();
   long protoPowerID = pProtoObject->getProtoPowerID();
   if(protoPowerID != -1)
   {
//-- FIXING PREFIX BUG ID 4382
      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
      if (pProtoPower)
      {
//-- FIXING PREFIX BUG ID 4381
         const BSquad* pParentSquad = getParentSquad();
//--
         if (pParentSquad)
         {
            getPlayer()->addPowerEntry(protoPowerID, getParentID(), pProtoPower->getUseLimit(), 1, false);
         }
      }
   }
}

//==============================================================================
// Change unit's player ownership
//==============================================================================
void BUnit::changeOwner(BPlayerID newPlayerID, BProtoObjectID newPOID)
{
   BPlayerID oldPlayerID = getPlayerID();

   #ifdef SYNC_Unit
      syncUnitData("BUnit::changeOwner mID", mID.asLong());
      syncUnitData("BUnit::changeOwner oldPlayerID", oldPlayerID);
      syncUnitData("BUnit::changeOwner newPlayerID", newPlayerID);
   #endif

   if (newPlayerID == oldPlayerID)
   {
      return;
   }

   BPlayer* pOldPlayer = gWorld->getPlayer(oldPlayerID);
   BPlayer* pNewPlayer = gWorld->getPlayer(newPlayerID);
   if (!pOldPlayer || !pNewPlayer)
   {
      return;
   }

   const BProtoObject* pOldPO = getProtoObject();
   if (!isAlive() || !getFlagDiesAtZeroHP())
   {
      if (!getFlagDiesAtZeroHP())
      {
         int protoID = getProtoID();
         pOldPlayer->removeUnitFromProtoObject(this, protoID);
         pOldPlayer->getTechTree()->checkUnitPrereq(protoID); // THIS MODIFIES STUFF
      }

      // If this had a unique proto object, set it to the base type on death (no need to create a new unique proto object for dead unit just
      // because it changed players.....hopefully)
      BProtoObjectID newProtoObjectId = cInvalidProtoObjectID;
      if (pOldPO->getFlagUniqueInstance())
         newProtoObjectId = pOldPO->getBaseType();

      BObject::changeOwner(newPlayerID, newProtoObjectId);

      if (!getFlagDiesAtZeroHP())
      {
         int protoID = getProtoID();
         pNewPlayer->addUnitToProtoObject(this, protoID);
         pNewPlayer->getTechTree()->checkUnitPrereq(protoID); // THIS MODIFIED STUFF
      }

      return;
   }

   // SLB: Create dopples to hide change of ownership
   computeDopple();

   onKillOrTransform(-1);

   //DCP 07/24/07: Remove all non-persistent actions and opps.
   removeOpps();
   removeActions();

   // Remove persistent actions that were created from the tactic
   removePersistentTacticActions();

   if (getParentSquad())
      getParentSquad()->removePersistentTacticActions();

   int protoID = getProtoID();
   pOldPlayer->removeUnitFromProtoObject(this, protoID);
   pOldPlayer->getTechTree()->checkUnitPrereq(protoID); // THIS MODIFIES STUFF

   // Goto base tracking
   if (getProtoObject()->getGotoType()==cGotoTypeBase)
      pOldPlayer->removeGotoBase(mID);
   else if (getProtoObject()->getFlagLockdownMenu())
   {
//-- FIXING PREFIX BUG ID 4386
      const BSquad* pSquad = getParentSquad();
//--
      if (pSquad && pSquad->getSquadMode()==BSquadAI::cModeLockdown)
         pOldPlayer->removeGotoBase(mID);
   }

   // Set the new proto id if one specified
   if (newPOID != cInvalidProtoObjectID)
   {
      protoID = newPOID;
   }
   // Otherwise, if the current protoObject is a unique one, we need to make a new unique one specific to the new player
   else if (pOldPO->getFlagUniqueInstance())
   {
      //-- FIXING PREFIX BUG ID 4388
      const BProtoObject* pUniquePO = pNewPlayer->allocateUniqueProtoObject(pOldPO, pOldPlayer, getID());
      //--
      if (pUniquePO)
      {
         protoID = pUniquePO->getID();
      }
   }


   // Remove existing building action and re-create
   bool onBuiltCalled = false;
   bool bHadActionBuilding = false;
   BUnitActionBuilding* pActionBuilding = (BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if (pActionBuilding)
   {
      bHadActionBuilding = true;
      int numActions = mActions.getNumberActions();
      for (int i = 0; i < numActions; i++)
      {
         //-- FIXING PREFIX BUG ID 4390
         const BAction* pAction = mActions.getAction(i);
         //--
         if (pAction && pAction->getType() == BAction::cActionTypeUnitBuilding)
         {
            mActions.removeActionByIndex(i);
            break;
         }
      }
   }

   BObject::changeOwner(newPlayerID, protoID);
   
   // refresh the proto id, just in case it changed
   protoID = getProtoID();

   if (bHadActionBuilding)
   {
      pActionBuilding = (BUnitActionBuilding*)gActionManager.createAction(BAction::cActionTypeUnitBuilding);
      if (pActionBuilding)
      {
         addAction(pActionBuilding);
#ifdef SYNC_Unit
         syncUnitData("BUnit::changeOwner mID", mID.asLong());
#endif
         pActionBuilding->doBuild(mPlayerID, false, true, false);
         onBuiltCalled = true;
      }
   }




   // Create any needed unique tech tree nodes
   const BProtoObject* pProtoObject = getProtoObject();
   uint commandCount = pProtoObject->getNumberCommands();
   if (commandCount>0)
   {
//-- FIXING PREFIX BUG ID 4387
      const BTeam* pTeam=pNewPlayer->getTeam();
//--
      int playerCount=pTeam->getNumberPlayers();
      long unitID = mID.asLong();
      for (uint i = 0; i < commandCount; i++)
      {
         BProtoObjectCommand command = pProtoObject->getCommand(i);
         if (command.getType() == BProtoObjectCommand::cTypeResearch)
         {
            if (gWorld->getFlagCoop() && pNewPlayer->isHuman())
            {
               for (int j=0; j<playerCount; j++)
                  gWorld->getPlayer(pTeam->getPlayerID(j))->getTechTree()->addUnitRef(command.getID(), unitID);
            }
            else
               pNewPlayer->getTechTree()->addUnitRef(command.getID(), unitID);
         }
      }
   }

   pNewPlayer->addUnitToProtoObject(this, protoID);
   pNewPlayer->getTechTree()->checkUnitPrereq(protoID); // THIS MODIFIED STUFF

   // Goto base tracking
   if (getProtoObject()->getGotoType()==cGotoTypeBase)
      pNewPlayer->addGotoBase(mID);
   else if (getProtoObject()->getFlagLockdownMenu())
   {
//-- FIXING PREFIX BUG ID 4389
      const BSquad* pSquad = getParentSquad();
//--
      if (pSquad && pSquad->getSquadMode()==BSquadAI::cModeLockdown)
         pNewPlayer->addGotoBase(mID);
   }

   // mrh - Moved this to before we change the player, so the future counts can be subtracted from the correct PlayerID
   // Remove existing building action and re-create
   /*bool onBuiltCalled = false;
   BUnitActionBuilding* pActionBuilding = (BUnitActionBuilding*)mActions.getActionByType(BAction::cActionTypeUnitBuilding);
   if (pActionBuilding)
   {
      int numActions = mActions.getNumberActions();
      for (int i = 0; i < numActions; i++)
      {
//-- FIXING PREFIX BUG ID 4390
         const BAction* pAction = mActions.getAction(i);
//--
         if (pAction && pAction->getType() == BAction::cActionTypeUnitBuilding)
         {
            mActions.removeActionByIndex(i);
            break;
         }
      }

      pActionBuilding = (BUnitActionBuilding*)gActionManager.createAction(BAction::cActionTypeUnitBuilding);
      if (pActionBuilding)
      {
         addAction(pActionBuilding);
         #ifdef SYNC_Unit
            syncUnitData("BUnit::changeOwner mID", mID.asLong());
         #endif
         pActionBuilding->doBuild(mPlayerID, false, true, false);
         onBuiltCalled = true;
      }
   }*/

   if (!onBuiltCalled)
      onBuilt(true, getFlagIsPhysicsReplacement(), true, false);

   if (pProtoObject->getFlagInvulnerableWhenGaia())
   {
      if (newPlayerID == 0)
         setFlagInvulnerable(true);
      else if (oldPlayerID == 0)
         setFlagInvulnerable(pProtoObject->getFlagInvulnerable());
   }

   // Change the owner of linked objects (such as sockets)
   uint numRefs=getNumberEntityRefs();
   for (uint i=0; i<numRefs; i++)
   {
//-- FIXING PREFIX BUG ID 4391
      const BEntityRef* pRef=getEntityRefByIndex(i);
//--
      if (pRef->mType==BEntityRef::cTypeGatherResource)
      {
         BUnit* pRefUnit=gWorld->getUnit(pRef->mID);
         if (pRefUnit)
         {
            BPlayerID refPlayerID=pRefUnit->getPlayerID();
            if (refPlayerID==oldPlayerID)
            {
               BSquad* pParentSquad = pRefUnit->getParentSquad();
               if (pParentSquad)
                  pParentSquad->changeOwner(newPlayerID);
            }
         }
      }
   }

   //This is done here because the unit's anim info isn't loaded until the first time
   //it's init from the protoObject in non-archive builds.
   const BProtoObject* pProto = getProtoObject();
   if (pProto && pProto->getTactic() && (pProto->getTactic()->getAnimInfoLoaded() == false))
   {
      loadTacticAnimInfo(pProto);
   }

   // if we're marked down, we need a hero death opportunity
   if (getFlagDown())
   {
      // re-enable animations because the action will fail if we don't.
      setAnimationEnabled(true,true);

      BUnitOpp* pHeroDeathOpp = NULL;
      pHeroDeathOpp=BUnitOpp::getInstance();
      pHeroDeathOpp->init();
      pHeroDeathOpp->setType(BUnitOpp::cTypeHeroDeath);
      pHeroDeathOpp->setPriority(BUnitOpp::cPriorityDeath);
      pHeroDeathOpp->generateID();
      addOpp(pHeroDeathOpp);
   }

   //-- Play ui capture complete sound
   if (newPlayerID == gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() || (gGame.isSplitScreen() && newPlayerID == gUserManager.getUser(BUserManager::cSecondaryUser)->getPlayerID()))
   {
      //-- Play the capture complete ui sound
      BCueIndex cue = getProtoObject()->getSound(cObjectSoundCaptureComplete);
      if(cue != cInvalidCueIndex)
         gSoundManager.playCue(cue);
   }

   // Remove from selection
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser->getPlayerID() == oldPlayerID)
   {
      BSelectionManager* pSelectionManager = pUser->getSelectionManager();
      if (pSelectionManager && pSelectionManager->isUnitSelected(mID))
         pSelectionManager->unselectUnit(mID);
   }

   // Adjust damage modifiers (divide out old players and mult in new one's)
   float tempMult;
   if (pOldPlayer->getAIDamageMultiplier() != 0.0f)
      tempMult = pNewPlayer->getAIDamageMultiplier() / pOldPlayer->getAIDamageMultiplier();
   else
      tempMult = pNewPlayer->getAIDamageMultiplier();
   adjustDamageModifier(tempMult);

   if (pOldPlayer->getAIDamageTakenMultiplier() != 0.0f)
      tempMult = pNewPlayer->getAIDamageTakenMultiplier() / pOldPlayer->getAIDamageTakenMultiplier();
   else
      tempMult = pNewPlayer->getAIDamageTakenMultiplier();
   adjustDamageTakenScalar(tempMult);
}

//=============================================================================
// BUnit::updateVisibleLists
//=============================================================================
void BUnit::updateVisibleLists(void)
{
   SCOPEDSAMPLE(BUnitUpdateVisibleLists);

   DWORD newVisibility = getNewVisibility();
   updateVisibility(newVisibility);
   if (getFlagAlive())
   {
      mDoppleBits.setAll(mDoppleBits.getValue() & 0x00FFFFFF); // Unreveal

      //-- send our position to the minimap
      BTeamID teamID = gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID();


      BTeamID secondaryTeamID = cInvalidTeamID;
//-- FIXING PREFIX BUG ID 4392
      const BUser* pSecondaryUser = gUserManager.getSecondaryUser();
//--
      if (pSecondaryUser)
         secondaryTeamID = pSecondaryUser->getTeamID();

      // mrh 8/2/07 - Be sure to check for the proto object until we make sure all objects have one.
      const BProtoObject* pProtoObject = getProtoObject();
      if (pProtoObject)
      {
         if (gWorld->getFlagAllVisible() || (isVisibleOrDoppled(teamID) && !hasDoppleObject(teamID) && !getFlagNoRender()) || pProtoObject->getFlagAlwaysVisibleOnMinimap())
         {
            // if we're not the leader unit or
            // we're a cloaked unit and the user team id isn't the same as ours
            // don't update the minimap
            bool addUnitToMinimap = true;
            BSquad* pParentSquad = getParentSquad();
            if (pParentSquad)
            {
               if ( pParentSquad->getLeader() != getID() || 
                    (pParentSquad->getFlagCloaked() && !pParentSquad->getFlagCloakDetected() && teamID != getTeamID()) )
                  addUnitToMinimap = false;
            }

            if (addUnitToMinimap)
            {
               //if (!gConfig.isDefined(cConfigFlashGameUI))
               //   gMiniMap.addUnit(*this);
               //else
               gUIManager->addMinimapIcon(this);
            }
         }
         else if (gGame.isSplitScreen())
         {
            if (isVisibleOrDoppled(secondaryTeamID))
            {
               if (!hasDoppleObject(secondaryTeamID))
               {
                  if (!getFlagNoRender())
                  {
                     // if we're not the leader unit or
                     // we're a cloaked unit and the user team id isn't the same as ours
                     // don't update the minimap
                     bool addUnitToMinimap = true;
                     BSquad* pParentSquad = getParentSquad();
                     if (pParentSquad)
                     {
                        if ( pParentSquad->getLeader() != getID() || 
                             (pParentSquad->getFlagCloaked() && !pParentSquad->getFlagCloakDetected() && secondaryTeamID != getTeamID()) )
                           addUnitToMinimap = false;
                     }

                     if (addUnitToMinimap)
                     {
                        //if (!gConfig.isDefined(cConfigFlashGameUI))
                        //   gMiniMap.addUnit(*this);
                        //else
                        gUIManager->addMinimapIcon(this);
                     }
                  }
               }
            }
         }
      }
   }
}

//=============================================================================
// BUnit::addToMaxHPContained
//=============================================================================
void BUnit::addToMaxHPContained(BUnit *pUnit)
{
   if(pUnit)
      mMaxHPContained += pUnit->getProtoObject()->getHitpoints();
}

//=============================================================================
// BUnit::removeFromMaxHPContained
//=============================================================================
void BUnit::removeFromMaxHPContained(BUnit *pUnit)
{
   if(pUnit)
      mMaxHPContained -= pUnit->getProtoObject()->getHitpoints();
}

//=============================================================================
// BUnit::getHPMax
//=============================================================================
float BUnit::getHPMax( void ) const
{
   // Use contained units' hitpoints if this unit is invulnerable and damage is passed to contained units
   if (getFlagInvulnerable() && getFlagHasGarrisoned() && getProtoObject() && getProtoObject()->getFlagDamageGarrisoned())
   {
      long numEntityRefs = getNumberEntityRefs();
      for( long i = numEntityRefs - 1; i >= 0; i-- )
      {
         BEntityRef *pEntityRef = getEntityRefByIndex(i);
         if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
         {
            BUnit *pUnit = gWorld->getUnit(pEntityRef->mID);
            if (pUnit)
               return(getMaxHPContained());
         }
      }
   }

   return( getProtoObject()->getHitpoints() );
}

//=========================================================================
// Get the maximum number of shield hit points possible for this unit type
//=========================================================================
float BUnit::getSPMax( void ) const
{
   if (getFlagHasShield())
   {        
      return (getProtoObject()->getShieldpoints());
   }

   return (0.0f);
}

//=============================================================================
// BUnit::getAmmoMax
//=============================================================================
float BUnit::getAmmoMax( void ) const
{
   if( getFlagUsesAmmo() )
   {      
      return( getProtoObject()->getMaxAmmo() );
   }

   return( 0.0f );
}

//=============================================================================
// BUnit::getHPPercentage
//=============================================================================
float BUnit::getHPPercentage( void ) const
{
   bool  hpRef         = false;
   float hpTotal       = 0.0f;

   // Use contained units' hitpoints if this unit is invulnerable and damage is passed to contained units
   if (getFlagInvulnerable() && getFlagHasGarrisoned() && getProtoObject() && getProtoObject()->getFlagDamageGarrisoned())
   {
      long  numEntityRefs = getNumberEntityRefs();
      for( long i = numEntityRefs - 1; i >= 0; i-- )
      {
         BEntityRef *pEntityRef = getEntityRefByIndex(i);
         if(pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit )
         {
            BUnit *pUnit = gWorld->getUnit(pEntityRef->mID);
            if (pUnit)
            {
               hpTotal += pUnit->getHitpoints();
               hpRef   =  true;
            }
         }
      }
   }

   if( !hpRef )
   {
      hpTotal = getHitpoints();
   }

   float hpMax = getHPMax();
   if( hpMax >= cFloatCompareEpsilon )
   {
      return( hpTotal / hpMax );
   }
      
   return( 0.0f );
}

//=====================================================================
// Calculate the percentage of the current amount of shield hit points
//=====================================================================
float BUnit::getSPPercentage() const
{   
   if (getFlagHasShield())
   {        
      float spMax = getSPMax();
      if (spMax >= cFloatCompareEpsilon)
      {
         return (getShieldpoints() / getSPMax());
      }
   }

   return (0.0f);
}

//=============================================================================
// BUnit::getAmmoPercentage
//=============================================================================
float BUnit::getAmmoPercentage( void ) const
{
   if( getFlagUsesAmmo() )
   {      
      float ammoMax = getAmmoMax();
      if( ammoMax >= cFloatCompareEpsilon )
      {
         return( getAmmunition() / ammoMax );
      }
   }

   return( 0.0f );
}

//=============================================================================
// BUnit::isPositionVisible
//=============================================================================
bool BUnit::isPositionVisible(BTeamID teamID) const
{
   DWORD visible = gVisibleMap.getTeamFogOffMask(teamID);
   long simX, simZ;
   getSimXZ(simX, simZ);
   return (gVisibleMap.getVisibility(simX, simZ) & visible) ? true : false;
}

//==============================================================================
// BUnit::createObstruction
//==============================================================================
void BUnit::createObstruction(bool playerOwnsObstruction)
{
   const BProtoObject* pProtoObject=getProtoObject();
   if(pProtoObject->getFlagBlockLOS())
   {
      const BProtoObject* pBlockMovementProto=getPlayer()->getProtoObject(pProtoObject->getBlockMovementObject());
      if(pBlockMovementProto)
      {
         // Create 4 obstructions rotated around to block an octagon area (approximating the circular blocked area).
         // For this to work, the blocker obstruction's mObstructionRadiusX should be equal to the main blocker objects 
         // obstruction radius and its mObstructionRadiusZ should be 1/3 of that.
         float angle=0.0f;
         BObjectCreateParms objectParms;
         objectParms.mProtoObjectID=pBlockMovementProto->getID();
         objectParms.mPlayerID=getPlayerID();
         objectParms.mPosition=mPosition;
         objectParms.mStartBuilt=true;
         for(long i=0; i<4; i++)
         {
            BVector forward=cZAxisVector;
            BVector right=cXAxisVector;
            if(i>0)
            {
               forward.rotateXZ(angle);
               right.rotateXZ(angle);
               forward.normalize();
               right.normalize();
            }
            objectParms.mForward=forward;
            objectParms.mRight=right;
//-- FIXING PREFIX BUG ID 4393
            const BUnit* pUnit=gWorld->createUnit(objectParms);
//--
            if(pUnit)
               addEntityRef(BEntityRef::cTypeBlockerChild, pUnit->getID(), 0, 0);
            angle+=45.0f*cRadiansPerDegree;
         }
      }
   }
   else
      BEntity::createObstruction(playerOwnsObstruction);
}

//==============================================================================
// BUnit::adjustAmmunition
//==============================================================================
void BUnit::adjustAmmunition(float delta)
{
   mAmmunition += delta;
   if(mAmmunition <= 0.0f)
      mAmmunition = 0.0f;
}

//==============================================================================
//==============================================================================
void BUnit::adjustCapturePoints(float points, float elapsed)
{
   setCapturePoints(mCapturePoints + points, elapsed);
}

//==============================================================================
//==============================================================================
void BUnit::setCapturePoints(float points, float elapsed)
{
   mCapturePoints = points;
   if (mCapturePoints < cFloatCompareEpsilon)
   {
      mCapturePoints = 0.0f;
      mCapturePlayerID = cInvalidPlayerID;

      if (getAnimationState() == BObjectAnimationState::cAnimationStateTrain)
      {
         // Stop playing capture animation
         setAnimationRate(1.0f);
         setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateIdle);
      }
   }
   else if (mCapturePoints >= getProtoObject()->getBuildPoints())
   {
      mCapturePoints = getProtoObject()->getBuildPoints();

      if (getAnimationState() == BObjectAnimationState::cAnimationStateTrain)
      {
         // Stop playing capture animation
         setAnimationRate(1.0f);
         setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateIdle);
      }
   }
   else
   {
      BVisual *pVisual = getVisual();
      if (pVisual)
      {
         if (getAnimationState() != BObjectAnimationState::cAnimationStateTrain)
         {
            // Start playing capture animation
            setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateTrain);
         }

         float percent = getCapturePercent();
         float duration = getAnimationDuration(cActionAnimationTrack);
         float position = pVisual->getAnimationPosition(cActionAnimationTrack) / duration;
         float change = percent - position;
         float rate = change * duration / elapsed;
         setAnimationRate(rate);
      }
   }
}
 
//==============================================================================
// BUnit::getCapturePercent
//==============================================================================
float BUnit::getCapturePercent() const
{
   float maxPoints=getProtoObject()->getBuildPoints();
   if(maxPoints==0.0f)
      return 1.0f;
   return mCapturePoints/maxPoints;
}

//==============================================================================
// BUnit::containUnit
//==============================================================================
void BUnit::containUnit(BEntityID id, bool noChangeOwner)
{
   BUnit* pUnit = gWorld->getUnit(id);
   if (!pUnit)
      return;

   // We need to start updating this unit
   setFlagNoUpdate(false);

   // If we don't own the containing unit, then take it over unless it is Gaia and force Gaia
   const BProtoObject* pProtoObject = getProtoObject();
   bool forceGaia = false;
   if (pProtoObject)
   {
      forceGaia = pProtoObject->getFlagForceToGaiaPlayer();
   }
   if (!noChangeOwner && (getPlayerID() != pUnit->getPlayerID()) && !forceGaia)
   {
      BPlayerID newPlayerID = pUnit->getPlayerID();
      if (gWorld->getFlagCoop())
      {
         BPlayerID coopPlayerID = pUnit->getPlayer()->getCoopID();
         if (coopPlayerID != cInvalidPlayerID)
            newPlayerID = coopPlayerID;
      }
      BSquad* pParentSquad = getParentSquad();
      if (pParentSquad)
         pParentSquad->changeOwner(newPlayerID);
   }

   // Add a contain reference to this unit
   addEntityRef(BEntityRef::cTypeContainUnit, id, 0, 0);

   // Use cover?
   BVector coverPoint = getNextAvailableCoverPoint();
   const BProtoObject* pUnitProtoObject = pUnit->getProtoObject();
   bool useCover = (pUnitProtoObject && pUnitProtoObject->isType(gDatabase.getOTIDInfantry()) && (coverPoint != cInvalidVector));

   // Contain/garrison the unit
   pUnit->stop();
   if (pUnit->getFlagLOSMarked() && !useCover)
      pUnit->markLOSOff();
   if (useCover)
   {
      pUnit->setForward(mForward);
      pUnit->setRight(mRight);
      pUnit->setUp(mUp);
      #ifdef SYNC_Unit
         syncUnitData("BUnit::containUnit 1", coverPoint);
      #endif
      pUnit->setPosition(coverPoint);      
      //XXXHalwes - 2/5/2008 - Shouldn't non-mobile be set for ALL contained units?
      pUnit->setFlagNonMobile(true);
      pUnit->setFlagInCover(true);
   }
   else
   {
      #ifdef SYNC_Unit
         syncUnitData("BUnit::containUnit 2", mPosition);
      #endif
      if (!pUnit->getFlagFlying())
         pUnit->setPosition(mPosition);
      pUnit->setAnimationEnabled(false);
   }
   pUnit->setFlagGarrisoned(true);
   pUnit->setFlagPassiveGarrisoned(getProtoObject()->getFlagPassiveGarrisoned());
   pUnit->setFlagCollidable(false);
   pUnit->updateObstruction();
   pUnit->setPhysicsKeyFramed(true, true);
   pUnit->addEntityRef(BEntityRef::cTypeContainingUnit, getID(), 0, 0);

   BSquad* pSquad = pUnit->getParentSquad();
   if (pSquad)
   {
      // DLM 5/28/08 - We need to make the squad noncollidable as well
      #ifdef _MOVE4
      pSquad->setFlagCollidable(false);
      pSquad->updateObstruction();
      #endif
      pSquad->updateGarrisonedFlag();

      // Remove this squad from it's platoon if it is fully garrisoned
      if (pSquad->getFlagGarrisoned())
      {
         BPlatoon* pParentPlatoon = reinterpret_cast<BPlatoon*>(pSquad->getParent());
         if (pParentPlatoon)
            pParentPlatoon->removeChild(pSquad);
      }

      // clear the squad mode if they are in the carrying object squad mode
      BSquadAI* pSquadAI = pSquad->getSquadAI();
      if (pSquadAI && pSquadAI->getMode() == BSquadAI::cModeCarryingObject)
         pSquadAI->setMode(BSquadAI::cModeNormal);
   }

   // Set our has garrisoned flag
   setFlagHasGarrisoned(true);

   // Update max HP contained pool
   BUnit* pThisUnit = getUnit();
   if (pThisUnit)
      pThisUnit->addToMaxHPContained(pUnit);

   setFlagForceUpdateContainedUnits(true);

   // Update contained pop, which needs to be done by squad even though garrison pop cap is a unit level concept
   calculateContainedPop();

   gWorld->notify(BEntity::cEventContainedUnit, mID, 0, 0);
}

//==============================================================================
// BUnit::unloadUnit
//==============================================================================
void BUnit::unloadUnit(BEntityID id, bool death)
{
   int numEntityRefs = getNumberEntityRefs();
   for (int i = numEntityRefs - 1; i >= 0; i-- )
   {
//-- FIXING PREFIX BUG ID 4394
      const BEntityRef* pEntityRef = getEntityRefByIndex(i);
//--
      if (pEntityRef && (pEntityRef->mID == id) && (pEntityRef->mType == BEntityRef::cTypeContainUnit))
      {
         unloadUnit(i, death);
         return;
      }
   }
}

//==============================================================================
// BUnit::unloadUnit
//==============================================================================
void BUnit::unloadUnit(int index, bool death)
{
//-- FIXING PREFIX BUG ID 4398
   const BEntityRef* pEntityRef = getEntityRefByIndex(index);
   BUnit* pUnit = pEntityRef ? gWorld->getUnit(pEntityRef->mID) : NULL;
   if (pEntityRef)
      removeEntityRef(index);

   // Uncontain/ungarrison the unit
   if (pUnit)
   {
      BSquad* pSquad = pUnit->getParentSquad();

      pUnit->setFlagGarrisoned(false);
      pUnit->setFlagPassiveGarrisoned(false);      
      // hate this - if we're frozen, don't enable anims
      if (!pSquad || !pSquad->isFrozen())
         pUnit->setAnimationEnabled(true);
      const BProtoObject* pProtoObject = pUnit->getProtoObject();
      if (pProtoObject)
      {
         pUnit->setFlagCollidable(pProtoObject->getFlagCollidable());
         pUnit->setFlagNonMobile(pProtoObject->getFlagNonMobile());
         if (!pProtoObject->getFlagNoTieToGround())
         {
            pUnit->tieToGround();
         }
      }
      pUnit->updateObstruction(); 
      pUnit->setPhysicsKeyFramed(false);

      if (pUnit->getFlagLOS() && !pUnit->getFlagLOSMarked() && !pUnit->getFlagInCover())
         pUnit->markLOSOn();

      pUnit->setFlagInCover(false);
      pUnit->removeEntityRef(BEntityRef::cTypeContainingUnit, getID());

      if(pSquad)
      {
         // DLM 5/29/08
         // The Squad comes with the unit.. 
         #ifdef _MOVE4
         // Use the unit's collidable state as the squad's collidable status.  
         // This could cause problems if we mix collidable and noncollidable units in the same squad, lord help us.
         pSquad->setFlagCollidable(pProtoObject->getFlagCollidable());
         pSquad->updateObstruction();
         #endif
         // Don't do this if a unit is ungarrisoned because it died. If it's the last unit, the squad will go away anyway.
         if (!death)
         {
            pSquad->updateGarrisonedFlag();
         }
      }
   }

   calculateContainedPop();

   // Update our has garrisoned flag
//-- FIXING PREFIX BUG ID 4404
   const BEntityRef* pContainUnitRef = getFirstEntityRefByType(BEntityRef::cTypeContainUnit);
//--
   if (!pContainUnitRef)
   {
      setFlagHasGarrisoned(false);
      mGarrisonTime = 0.0f;
   }

   // Possibly go back to being gaia
   if (!pContainUnitRef && getProtoObject()->getFlagUngarrisonToGaia())
   {
      BSquad* pParentSquad = getParentSquad();
      if (pParentSquad)
      {
         pParentSquad->changeOwner(0);
      }
      else
      {
         changeOwner(0);
      }
   }

   // Update max HP contained pool
   if (!pContainUnitRef)
      clearMaxHPContained();
   else if (!death)
      removeFromMaxHPContained(pUnit);

   // Notify world when we are fully unloaded
   if (!pContainUnitRef)
      gWorld->notify(BEntity::cEventUnloaded, mID, 0, 0);
}

//==============================================================================
// Check to see if the unit can contain the squad's pop count
//==============================================================================
bool BUnit::canContain(const BSquad* pSquad, float testContainedPop /*= 0.0f*/) const
{
   if (mFlagBlockContain)
      return (false);

   if (!pSquad)
      return (false);

   const BProtoObject* pProtoObject = getProtoObject();
   if (!pProtoObject)
      return (false);

   // We cannot contain this squad if we already have enemies garrisoned inside of us
   if (hasGarrisonedEnemies(pSquad->getPlayerID(), true))
      return (false);

   // Make sure that the ungarrison timer is up
   if (!isReadyToContain())
      return (false);

   // Make sure that the given squad is the same as the one we already have
   if (pProtoObject->getFlagOneSquadContainment() && getNumberEntityRefs() > 0)
   {
//-- FIXING PREFIX BUG ID 4406
      const BEntityRef* pRef = getEntityRefByIndex(0);
//--
      if (pRef && (pRef->mType == BEntityRef::cTypeContainUnit))
      {
         BUnit* pContainedUnit = gWorld->getUnit(pRef->mID);
//-- FIXING PREFIX BUG ID 4405
         const BSquad* pContainedSquad = pContainedUnit->getParentSquad();
//--
         if (!pContainedSquad || pContainedSquad != pSquad)
         {
            return (false);
         }
      }
   }

   float maxContained = (float)pProtoObject->getMaxContained();
   if (maxContained <= 0.0f)
      return (true);

   const BProtoObject* pSquadProtoObject = pSquad->getProtoObject();
   if (!pSquadProtoObject)
      return (false);

   const BObjectTypeIDArray& contains = pProtoObject->getContains();   
   uint numContains = contains.getSize();
   if (numContains > 0)
   {
      bool passed = false;
      for (uint i = 0; i < numContains; i++)
      {
         if (pSquadProtoObject->isType(contains[i]))
         {
            passed = true;
            break;
         }
      }

      if (!passed)
         return (false);
   }
   
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return (false);

   BPopArray pops;
   pProtoSquad->getPops(pops);
   if (pops.getNumber() == 0)
      return (true);
   float squadPop = pops[0].mCount;

   // Subtract unit pop from our squad pop for units already contained
   const BEntityIDArray squadChildren = pSquad->getChildList();
   uint numSquadChildren = squadChildren.getSize();
   uint numEntityRefs = getNumberEntityRefs();
   for (uint i = 0; i < numEntityRefs; i++)
   {
//-- FIXING PREFIX BUG ID 4401
      const BEntityRef* pEntityRef = getEntityRefByIndex(i);
//--
      if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeContainUnit))
      {
         for (uint j = 0; j < numSquadChildren; j++)
         {
            if (squadChildren[j] == pEntityRef->mID)
            {
               BUnit* pChildUnit = gWorld->getUnit(squadChildren[j]);
               if (pChildUnit)
               {
                  BPopArray* unitPops = pChildUnit->getProtoObject()->getPops();
                  if (unitPops->getNumber() > 0)
                  {
                     squadPop -= unitPops->get(0).mCount;
                  }
               }

               break;
            }
         }
      }
   }

   float containedPop = (testContainedPop > 0.0f) ? testContainedPop : mContainedPop;
   if ((squadPop + containedPop) > maxContained)
      return (false);

   return (true);
}

//==============================================================================
// Check to see if the unit can contain the units's pop count
//==============================================================================
bool BUnit::canContain(const BUnit* pUnit) const
{
   if (mFlagBlockContain)
      return (false);

   if (!pUnit)
      return (false);

   const BProtoObject* pProtoObject = getProtoObject();
   if (!pProtoObject)
      return (false);

   // We cannot contain this unit if we already have enemies garrisoned inside of us
   if (hasGarrisonedEnemies(pUnit->getPlayerID(), true))
      return (false);

   // Make sure that the ungarrison timer is up
   if (!isReadyToContain())
      return (false);

   // Make sure that the given squad is the same as the one we already have
   if (pProtoObject->getFlagOneSquadContainment() && getNumberEntityRefs() > 0)
   {
//-- FIXING PREFIX BUG ID 4408
      const BEntityRef* pRef = getEntityRefByIndex(0);
//--
      if (pRef && (pRef->mType == BEntityRef::cTypeContainUnit))
      {
         BUnit* pContainedUnit = gWorld->getUnit(pRef->mID);
//-- FIXING PREFIX BUG ID 4407
         const BSquad* pContainedSquad = pContainedUnit->getParentSquad();
//--

         if (!pContainedSquad || pContainedSquad != pUnit->getParentSquad())
            return (false);
      }
   }

   float maxContained = (float)pProtoObject->getMaxContained();
   if (maxContained <= 0.0f)
      return (true);

   const BProtoObject* pUnitProtoObject = pUnit->getProtoObject();
   if (!pUnitProtoObject)
      return (false);

   const BObjectTypeIDArray& contains = pProtoObject->getContains();   
   uint numContains = contains.getSize();
   if (numContains > 0)
   {
      bool passed = false;
      for (uint i = 0; i < numContains; i++)
      {
         if (pUnitProtoObject->isType(contains[i]))
         {
            passed = true;
            break;
         }
      }

      if (!passed)
         return (false);
   }

   // If we already have a reference for this unit, fail out.
   uint numRefs = getNumberEntityRefs();

   for (uint i = 0; i < numRefs; i++)
   {
      const BEntityRef* pRef = getEntityRefByIndex(i);
      //--
      if (pRef && (pRef->mType == BEntityRef::cTypeContainUnit))
      {
         BUnit* pContainedUnit = gWorld->getUnit(pRef->mID);
         //-- FIXING PREFIX BUG ID 4405

         if (pContainedUnit && pUnit->getID() == pContainedUnit->getID())
         {
            return false;
         }     
      }
   }

   if (pProtoObject->getNumberPops() <= 0)
      return (true);    

   float popCost = pUnit->getSquadPop();  
      
   if ((popCost + mContainedPop) > maxContained)
   {
      uint numRefs = getNumberEntityRefs();

      for (uint i = 0; i < numRefs; i++)
      {
         const BEntityRef* pEntityRef = getEntityRefByIndex(i);
         if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeContainUnit))
         {
            BUnit* pContainedUnit = gWorld->getUnit(pEntityRef->mID); 

            if (pContainedUnit && pUnit->getParentID() == pContainedUnit->getParentID())
               return true;
         }
      }
      return (false);
   }

   return (true);
}

//==============================================================================
// BUnit::isReadyToContain
//==============================================================================
bool BUnit::isReadyToContain() const
{
   BSquad* pParent = getParentSquad();

   // Can't be locked down
   if (pParent && pParent->getSquadMode() != BSquadAI::cModeNormal)
      return false;
   
   const BProtoObject* pProtoObject = getProtoObject();
   if (!pProtoObject)
      return (false);

   if (pProtoObject->getGarrisonTime() > 0.0f && mGarrisonTime < pProtoObject->getGarrisonTime())
      return (false);

   return (true);
}

//==============================================================================
// Returns true if any member from the given squad is contained
//==============================================================================
bool BUnit::doesContain(const BSquad* pSquad) const
{
   if (!pSquad)
      return false;

   uint numChildren = pSquad->getNumberChildren();
   uint numEntityRefs = getNumberEntityRefs();
   for (uint i = 0; i < numChildren; ++i)
   {
      BEntityID id = pSquad->getChild(i);

      for (uint j = 0; j < numEntityRefs; j++)
      {
         const BEntityRef* pEntityRef = getEntityRefByIndex(j);
         if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeContainUnit))
         {
            if (pEntityRef->mID == id)
               return true;
         }
      }
   }
   
   return false;
}

//==============================================================================
//==============================================================================
bool BUnit::doesContain(const BUnit* pUnit) const
{
   if (!pUnit)
      return false;

   uint numEntityRefs = getNumberEntityRefs();
   for (uint i = 0; i < numEntityRefs; i++)
   {
      const BEntityRef* pEntityRef = getEntityRefByIndex(i);
      if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeContainUnit))
      {
         if (pEntityRef->mID == pUnit->getID())
            return true;
      }
   }

   return false;
}

//==============================================================================
// BUnit::attachObject
//==============================================================================
void BUnit::attachObject(BEntityID id, long toBoneHandle, long fromBoneHandle, bool useOffset)
{
   BObject* pObject = gWorld->getObject(id);
   if (!pObject)
      return;

   // Attach the object
   addEntityRef(BEntityRef::cTypeAttachObject, id, 0, 0);
   pObject->stop();
   pObject->setFlagAttached(true);
   pObject->setFlagCollidable(false);
   pObject->updateObstruction();
   pObject->setPhysicsKeyFramed(true, true);
   pObject->setFlagPassiveGarrisoned(getProtoObject()->getFlagPassiveGarrisoned());
   addAttachment(pObject, toBoneHandle, fromBoneHandle, true, useOffset);
   pObject->addEntityRef(BEntityRef::cTypeAttachedToObject, getID(), 0, 0);

   const BUnit* pUnit = pObject->getUnit();
   if (pUnit)
   {
      BSquad* pSquad = pUnit->getParentSquad();
      if (pSquad)
      {
         pSquad->updateAttachedFlag();
         // DLM 5/28/08 - We need to make the squad noncollidable as well
         #ifdef _MOVE4
            if (pSquad->getFlagAttached())
            {
               pSquad->setFlagCollidable(false);
               pSquad->updateObstruction();
               BPlatoon* pParentPlatoon = reinterpret_cast<BPlatoon*>(pSquad->getParent());
               if (pParentPlatoon)
                  pParentPlatoon->removeChild(pSquad);
            }
         #endif
      }
      gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager()->unselectUnit(id);
      if (gGame.isSplitScreen())
         gUserManager.getUser(BUserManager::cSecondaryUser)->getSelectionManager()->unselectUnit(id);
   }

   // Set our has attached flag
   setFlagHasAttached(true);
}

//==============================================================================
// BUnit::unattachObject
//==============================================================================
void BUnit::unattachObject(BEntityID id)
{
   BObject* pObject = gWorld->getObject(id);
   if (pObject)
   {
      long numEntityRefs = getNumberEntityRefs();
      for (long i = 0; i < numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 4403
         const BEntityRef* pEntityRef = getEntityRefByIndex(i);
//--
         if (pEntityRef && (pEntityRef->mID == id.asLong()))
         {
            removeEntityRef(i);

            // Unattach the unit
            removeAttachment(id);
            pObject->setFlagAttached(false);
            pObject->setFlagCollidable(getProtoObject()->getFlagCollidable());
            pObject->updateObstruction();
            pObject->setPhysicsKeyFramed(false);
            pObject->setFlagPassiveGarrisoned(false);
            pObject->removeEntityRef(BEntityRef::cTypeAttachedToObject, getID());

            // [5/15/2008 xemu] make this selectable again now that it is no longer a child object 
            if (pObject->getProtoObject()->getFlagNotSelectableWhenChildObject())
            {
               pObject->setFlagSelectable(true);
            }

            const BUnit* pUnit = pObject->getUnit();
            if (pUnit)
            {
               BSquad* pSquad = pUnit->getParentSquad();
               if (pSquad)
               {
                  BVector unitPos = pUnit->getPosition();
                  pSquad->setPosition(unitPos);
                  pSquad->setLeashPosition(unitPos);
                  pSquad->setTurnRadiusPos(unitPos);
                  pSquad->setTurnRadiusFwd(pUnit->getForward());
                  pSquad->updateAttachedFlag();
                  // DLM 5/29/08
                  // The Squad comes with the unit.. 
                  #ifdef _MOVE4
                     if (!pSquad->getFlagAttached())
                     {
                        // Use the unit's collidable state as the squad's collidable status.  
                        // This could cause problems if we mix collidable and noncollidable units in the same squad, lord help us.
                        pSquad->setFlagCollidable(getProtoObject()->getFlagCollidable());
                        pSquad->updateObstruction();
                     }
                  #endif
               }
            }

            // Update our has attached flag
//-- FIXING PREFIX BUG ID 4487
            const BEntityRef* pAttachObjectRef = getFirstEntityRefByType(BEntityRef::cTypeAttachObject);
//--
            if (!pAttachObjectRef)
               setFlagHasAttached(false);

            return;
         }
      }
   }
}

//==============================================================================
// Hitch a "Towable" type unit to a "TowTruck" type unit
//==============================================================================
void BUnit::hitchUnit(BEntityID id)
{
   BUnit* pUnit = gWorld->getUnit(id);
   if (!pUnit)
      return;

   // Hitch the unit
   addEntityRef(BEntityRef::cTypeHitchUnit, id, 0, 0);
   pUnit->stop();
   pUnit->setFlagHitched(true);
   pUnit->setFlagNoWorldUpdate(true);
   pUnit->updateObstruction();
   pUnit->addEntityRef(BEntityRef::cTypeHitchedToUnit, getID(), 0, 0);

   BSquad* pSquad = pUnit->getParentSquad();
   if (pSquad)
      pSquad->setFlagHitched(true);

   BUser* pUser = gUserManager.getUserByPlayerID(pUnit->getPlayerID());
   if (pUser)
   {
      pUser->getSelectionManager()->unselectUnit(id);
   }
   
   // Set our hitched flag
   setFlagHasHitched(true);
}

//==============================================================================
// Unhitch a "Towable" type unit from a "TowTruck" type unit
//==============================================================================
void BUnit::unhitchUnit(BEntityID id)
{
   BUnit* pUnit = gWorld->getUnit(id);
   if (pUnit)
   {
      // Remove the ref to the hitched unit
//-- FIXING PREFIX BUG ID 4488
      const BEntityRef* pEntityRef = getFirstEntityRefByType(BEntityRef::cTypeHitchUnit);
//--
      if (pEntityRef)
      {
         removeEntityRef(BEntityRef::cTypeHitchUnit, pEntityRef->mID);
      }

      pUnit->setFlagHitched(false);
      pUnit->setFlagNoWorldUpdate(false);
      pUnit->updateObstruction();

      BSquad* pSquad = pUnit->getParentSquad();
      if (pSquad)
      {
         pSquad->setFlagHitched(false);
      }
      
      // Remove the hitched unit's ref to us
      pEntityRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeHitchedToUnit);
      if (pEntityRef)
      {
         pUnit->removeEntityRef(BEntityRef::cTypeHitchedToUnit, pEntityRef->mID);
      }

      BUnitActionCoreSlide* pUACS = reinterpret_cast<BUnitActionCoreSlide*>(pUnit->getActionByType(BAction::cActionTypeUnitCoreSlide));
      if (pUACS)
         pUACS->setState(BAction::cStateWorking);

      // Update our has hitched flag
      setFlagHasHitched(false);
   }
}

//==============================================================================
// Get the hitched unit
//==============================================================================
BEntityID BUnit::getHitchedUnit()
{
   BEntityID hitchedUnitID = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 4490
   const BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeHitchUnit);
//--
   if (pRef)
   {
      hitchedUnitID = pRef->mID;
   }

   return (hitchedUnitID);
}

//==============================================================================
//==============================================================================
BEntityID BUnit::getAttackTargetID() const
{
   //Stand-in during the conversion.
   const BAction* pAction=mActions.getActionByType(BAction::cActionTypeUnitRangedAttack);
   if (pAction)
   {
      const BSimTarget *pTarget=pAction->getTarget();
      if (pTarget)
         return(pTarget->getID());
      return(cInvalidObjectID);
   }
   return(cInvalidObjectID);
}


//==============================================================================
//==============================================================================
int BUnit::getSelectType(BTeamID teamId) const
{
   if (!isSelectable(teamId))
      return (cSelectTypeNone);

   if (mFlagSelectTypeTarget)
      return (cSelectTypeTarget);

   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject && pProtoObject->getFlagLockdownMenu())
   {
//-- FIXING PREFIX BUG ID 4491
      const BSquad* pSquad = getParentSquad();
//--
      if (pSquad && pSquad->getFlagChangingMode())
         return cSelectTypeNone;
      else if (pSquad && (pSquad->getSquadMode()==BSquadAI::cModeLockdown))
         return cSelectTypeCommand;
   }

   // If cover object with no units garrisoned inside send back unit select type
   if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDCover()) && !getFlagHasGarrisoned())
   {
      return (cSelectTypeSingleUnit);
   }   

   return BObject::getSelectType(teamId);
}

//==============================================================================
//==============================================================================
int BUnit::getGotoType() const
{
   if (getProtoObject()->getFlagLockdownMenu())
   {
//-- FIXING PREFIX BUG ID 4492
      const BSquad* pSquad = getParentSquad();
//--
      if (pSquad && pSquad->getSquadMode()==BSquadAI::cModeLockdown)
         return cGotoTypeBase;
   }
   return BObject::getGotoType();
}

//==============================================================================
// BUnit::getMaxHitZoneHitpoints
//==============================================================================
float BUnit::getMaxHitZoneHitpoints( long index ) const
{
   float          result            = -1.0f;
   BProtoObject*  pProtoObject      = (BProtoObject*)getProtoObject();
   BHitZoneArray* pProtoHitZoneList = pProtoObject->getHitZoneList();
   if( pProtoHitZoneList && ( index < pProtoHitZoneList->getNumber() ) )
   {
      result = pProtoHitZoneList->get( index ).getHitpoints();
   }

   return( result );
}

//==============================================================================
// BUnit::getMaxHitZoneShieldpoints
//==============================================================================
float BUnit::getMaxHitZoneShieldpoints( long index ) const
{
   float          result            = -1.0f;
   BProtoObject*  pProtoObject      = (BProtoObject*)getProtoObject();
   BHitZoneArray* pProtoHitZoneList = pProtoObject->getHitZoneList();
   if( pProtoHitZoneList && ( index < pProtoHitZoneList->getNumber() ) )
   {
      result = pProtoHitZoneList->get( index ).getShieldpoints();
   }

   return( result );
}

//==============================================================================
// BUnit::getHitZonePosition
//==============================================================================
bool BUnit::getHitZonePosition( long index, BVector& position ) const
{
   position = cInvalidVector;

   if( mpHitZoneList && mpVisual && ( index != -1 ) && ( index < mpHitZoneList->getNumber() ) )
   {            
      BHitZone& zone = (*mpHitZoneList)[index];
//-- FIXING PREFIX BUG ID 4493
      const BVisualItem* pAttachment = mpVisual->getAttachment( zone.getAttachmentHandle() );
//--
      if( pAttachment )
      {
         BMatrix worldMatrix;
         getWorldMatrix( worldMatrix );                  

         // These are in model space
         BVector min = pAttachment->getMinCorner();
         BVector max = pAttachment->getMaxCorner();

         position = ( min + max ) * 0.5f;

         // Halwes - 1/26/2007 - Do I need the concat matrix from the hierarchy?
         // Transform to world space
         worldMatrix.transformVectorAsPoint( position, position );

         return( true );
      }         
   }

   return( false );
}

//==============================================================================
// BUnit::getHitZoneOBB
//==============================================================================
bool BUnit::getHitZoneOBB( long index, BBoundingBox& obb ) const
{
   if( mpHitZoneList && mpVisual && ( index != -1 ) && ( index < mpHitZoneList->getNumber() ) )
   {            
      BHitZone& zone = (*mpHitZoneList)[index];
//-- FIXING PREFIX BUG ID 4494
      const BVisualItem* pAttachment = mpVisual->getAttachment(zone.getAttachmentHandle());
//--
      if( pAttachment )
      {
         BMatrix worldMatrix;
         getWorldMatrix( worldMatrix );                  

         // These are in model space
         BVector min = pAttachment->getMinCorner();
         BVector max = pAttachment->getMaxCorner();
         
         obb.initialize( min, max );               
         obb.transform( pAttachment->getTransform() );               

         // Halwes - 1/25/2007 - Do I need the total concat matrix from the bone hierarchy?
         // Transform to world space
         obb.transform( worldMatrix );

         return( true );
      }         
   }

   return( false );   
}

//==============================================================================
// BUnit::getTargetedHitZone
//==============================================================================
bool BUnit::getTargetedHitZone( long& index ) const
{
   index = -1;

   if (!mpHitZoneList)
      return false;

   long numHitZones = mpHitZoneList->getNumber();
   for( long i = 0; i < numHitZones; i++ )
   {
      BHitZone& zone = (*mpHitZoneList)[i];
      if( zone.getTargeted() )
      {
         index = i;
         return( true );
      }
   }

   return( false );
}

//==============================================================================
// BUnit::clearTargetedHitZone
//==============================================================================
void BUnit::clearTargetedHitZone()
{
   if (!mpHitZoneList)
      return;
   long numHitZones = mpHitZoneList->getNumber();
   for( long i = 0; i < numHitZones; i++ )
   {
      BHitZone& zone = (*mpHitZoneList)[i];
      zone.setTargeted( false );

      // Halwes - 2/2/2007 - This is temporary to draw targeted hit zone visuals with different tint colors.   
      BVisualItem* pVisualItem = mpVisual->getAttachment( zone.getAttachmentHandle() );
      if( pVisualItem )
      {
         pVisualItem->setFlag( BVisualItem::cFlagHitZone, false );
      }
   }
}

//==============================================================================
// BUnit::getHitZoneIndex
//==============================================================================
bool BUnit::getHitZoneIndex( BSimString name, long& index ) const
{
   index = -1;
   if (!mpHitZoneList)
      return false;
   long numHitZones = mpHitZoneList->getNumber();
   for( long i = 0; i < numHitZones; i++ )
   {
      BHitZone& zone = (*mpHitZoneList)[i];
      if( zone.getAttachmentName() == name )
      {
         index = i;
         return( true );
      }
   }

   return( false );
}

//==============================================================================
// BUnit::getHitZoneOffset
//==============================================================================
float BUnit::getHitZoneOffset( long index ) const
{
   float        result = 0.0f;
   BBoundingBox obb;
   if( getHitZoneOBB( index, obb ) )
   {
      const float* extents = obb.getExtents();

      result = extents[1] * 0.5f;
   }

   return( result );
}

// Halwes - 2/2/2007 - This is temporary to draw targeted hit zone visuals with different tint colors.   
//==============================================================================
// BUnit::setTargetedHitZone
//==============================================================================
void BUnit::setTargetedHitZone( long index, bool targeted )
{
   if( mpHitZoneList && ( index != -1 ) && ( index < mpHitZoneList->getNumber() ) )
   {
      BHitZone& zone = (*mpHitZoneList)[index];

      zone.setTargeted( targeted );

      BVisualItem* pVisualItem = mpVisual->getAttachment( zone.getAttachmentHandle() );
      if( pVisualItem )
      {
         pVisualItem->setFlag( BVisualItem::cFlagHitZone, targeted );      
      }
   }
}

/*
//==============================================================================
// unitSyncCheckVisualGetDataFunc
//==============================================================================
void unitSyncCheckVisualGetDataFunc(long dataType, float floatVal, long longVal, void* pUserData)
{
   #ifdef SYNC_UnitDetail
      switch(dataType)
      {
         case 0: syncUnitDetailData("controlIndex", longVal); break;
         case 1: syncUnitDetailData("CurrentClock", floatVal); break;
         case 2: syncUnitDetailData("LocalClock", floatVal); break;
         case 3: syncUnitDetailData("Speed", floatVal); break;
         case 4: syncUnitDetailData("LocalDuration", floatVal); break;
         case 5: syncUnitDetailData("LoopIndex", longVal); break;
         case 6: syncUnitDetailData("CurrentWeight", floatVal); break;
         case 7: syncUnitDetailData("EaseInStartClock", floatVal); break;
         case 8: syncUnitDetailData("EaseInEndClock", floatVal); break;
         case 9: syncUnitDetailData("LoopIndex", longVal); break;
         case 10: syncUnitDetailData("ControlWeight", floatVal); break;
         case 11: syncUnitDetailData("LocalClock", floatVal); break;
         case 12: syncUnitDetailData("Speed", floatVal); break;
         case 13: syncUnitDetailData("LocalDuration", floatVal); break;
         case 14: syncUnitDetailData("UnderflowLoop", longVal); break;
         case 15: syncUnitDetailData("OverflowLoop", longVal); break;
         case 16: syncUnitDetailData("trackMaskWeight", floatVal); break;
         case 17: syncUnitDetailData("modelMaskWeight", floatVal); break;
         case 18: syncUnitDetailData("Track.LODError", floatVal); break;
         case 19: syncUnitDetailData("SampledTransform.Flags", longVal); break;
         case 20: syncUnitDetailData("SampledTransform.Position", floatVal); break;
         case 21: syncUnitDetailData("SampledTransform.Orientation", floatVal); break;
         case 22: syncUnitDetailData("SampledTransform.ScaleShear", floatVal); break;
         //case 23: syncUnitDetailData("PositionCurve.Format", longVal); break;
         //case 24: syncUnitDetailData("PositionCurve.Degree", longVal); break;
         //case 25: syncUnitDetailData("PositionCurve.Controls", floatVal); break;
         //case 26: syncUnitDetailData("PositionCurve.Knots", floatVal); break;
         //case 27: syncUnitDetailData("OrientationCurve.Format", longVal); break;
         //case 28: syncUnitDetailData("OrientationCurve.Degree", longVal); break;
         //case 29: syncUnitDetailData("OrientationCurve.Controls", floatVal); break;
         //case 30: syncUnitDetailData("OrientationCurve.Knots", floatVal); break;
         //case 31: syncUnitDetailData("ScaleShearCurve.Format", longVal); break;
         //case 32: syncUnitDetailData("ScaleShearCurve.Degree", longVal); break;
         //case 33: syncUnitDetailData("ScaleShearCurve.Controls", floatVal); break;
         //case 34: syncUnitDetailData("ScaleShearCurve.Knots", floatVal); break;
         case 35: syncUnitDetailData("pState->AccumulationMode", longVal); break;
         case 36: syncUnitDetailData("Track.QuaternionMode", longVal); break;
         case 37: syncUnitDetailData("LocalPoseBoneIndex", longVal); break;
         case 38: syncUnitDetailData("LoopCount", longVal); break;
         case 39: syncUnitDetailData("LoopIndex", longVal); break;
         case 40: syncUnitDetailData("dTLocalClockPending", floatVal); break;
         case 41: syncUnitDetailData("TimeStep", floatVal); break;
      }
   #endif
}

//==============================================================================
// BUnit::syncCheckVisual
//==============================================================================
void BUnit::syncCheckVisual()
{
   #ifdef SYNC_UnitDetail
      if(!mpVisual || !mpVisual->mpInstance || mpVisual->mModelAsset.mType!=cVisualAssetGrannyModel)
         return;
      syncUnitDetailData("ID", mID.asLong());
      syncUnitDetailData("Visual", mpVisual->getProtoVisual()->getName());
      BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpVisual->mpInstance;
      GrannyGetSyncData(pGrannyInstance->getModelInstance(), unitSyncCheckVisualGetDataFunc, NULL);
   #endif
}
*/

//==============================================================================
// BUnit::attackActionStarted
//==============================================================================
void BUnit::attackActionStarted(BEntityID targetID)
{
   if(mAttackActionRefCount++ == 0)
   {
      unitEnteredCombat(targetID);
   }   
}

//==============================================================================
// BUnit::attackActionEnded
//==============================================================================
void BUnit::attackActionEnded(void)
{
   if(--mAttackActionRefCount == 0)
   {
      unitLeftCombat();
   }

#ifndef BUILD_FINAL
   if(mAttackActionRefCount < 0)
   {
      mAttackActionRefCount = 0;
      gConsole.output(cMsgDebug, "Trying to set mAttackAcitonRefCount < 0!");
   }
#endif 
}

//==============================================================================
// BUnit::unitEnteredCombat
//==============================================================================
void BUnit::unitEnteredCombat(BEntityID targetID)
{
   if(gWorld->getBattleManager())
      gWorld->getBattleManager()->unitEnteredCombat(mID, targetID);
}

//==============================================================================
// BUnit::unitLeftCombat
//==============================================================================
void BUnit::unitLeftCombat(void)
{
   if(gWorld->getBattleManager() && mBattleID != -1)
      gWorld->getBattleManager()->removeUnitFromBattle(this, mBattleID);
}

//==============================================================================
// BUnit::playSound
//==============================================================================
bool BUnit::playUISound(BSoundType soundType, bool suppressBankLoad, long squadID, long actionID) const
{
   //-- Only play ui sounds for the primary player or the secondary player
   if(getPlayer()->getID() != gUserManager.getPrimaryUser()->getPlayerID() && 
      getPlayer()->getID() != gUserManager.getSecondaryUser()->getPlayerID())
      return false;

   const BProtoObject* pProto = getProtoObject();;

   //-- Check to see if this unit is jacked!
   const BSquad* pSquad = getParentSquad();
   if(pSquad)
   {
      const BSquad* pSpartanSquad = pSquad->getGarrisonedSpartanSquad();
      if(pSpartanSquad)
      {
         pProto = pSpartanSquad->getProtoObject();

         // If we're trying to play an AckAbility, replace with AckAbilityJacked
         if (soundType == cObjectSoundAckAbility)
         {
            soundType = cObjectSoundAckAbilityJacked;
            squadID = pSquad->getProtoID();
            actionID = -1;

            //If we are commadering a vehicle, it is no longer a base protosquad.  find the base protosquad for comparison purposes
            if (!gDatabase.isValidProtoSquad(squadID) && (pSquad->getProtoSquad() != NULL))
            {
               squadID = pSquad->getProtoSquad()->getBaseType();
               BASSERT(gDatabase.isValidProtoSquad(squadID));
            }
         }
      }
   }

   
   //-- Play the sound
   return pProto ? pProto->playUISound(soundType, suppressBankLoad, getPosition(), squadID, actionID) : false;
}

//==============================================================================
// Test whether the unit is an external shield unit
//==============================================================================
bool BUnit::isExternalShield() const
{ 
   return (getProtoObject()->getFlagIsExternalShield()); 
}

//==============================================================================
//==============================================================================
BObject* BUnit::createAndThrowPhysicsReplacement(IDamageInfo* pDamageInfo, const BVector damagePos, float distanceFactor, bool rocketOnDeath, float forceScaler, BEntityID attackerID)
{
   if (isGarrisoned() && !isInCover())
      return NULL;

   BPhysicsInfo *pInfo = gPhysicsInfoManager.get(getProtoObject()->getPhysicsReplacementInfoID(), true);
   if (!pInfo)
      return NULL;

   // Only throw the unit if it's physics file says so
   if (!pInfo->isThrownByProjectiles())
      return NULL;

   if(getFlagDestroy())
      return NULL;

   // Create and throw replacement object
   BObject* pPhysReplacement = createPhysicsReplacement();
   if (!pPhysReplacement || !pPhysReplacement->getPhysicsObject())
      return NULL;

   // DJBFIXME: Data drive all of this.   
   if(rocketOnDeath)
   {
      // Set the flag
      BUnitActionPhysics *pPhysAction = reinterpret_cast<BUnitActionPhysics *>(pPhysReplacement->getActionByType(BAction::cActionTypeUnitPhysics));
      if(pPhysAction)
      {
         pPhysAction->setupRocketOnDeath();
         rocketOnDeath = true;

         // Play Rocket Death Sound
         BCueIndex startIndex  = getProtoObject()->getSound(cObjectSoundRocketStart);
         BCueIndex stopIndex = getProtoObject()->getSound(cObjectSoundRocketEnd);
         gWorld->getWorldSoundManager()->addSound(pPhysReplacement, -1, startIndex, true, stopIndex, true, true);
      }
   }

   // Apply impulse
   // If set to use PhysicsLaunchAxial, damagePos is the horizontal direction of launch rather than an impact point
   BVector dir;
   if (pDamageInfo->getPhysicsLaunchAxial())
      dir = damagePos;
   else
      dir = getPosition() - damagePos;

   dir.y = 0;
   if(dir.lengthSquared() < cFloatCompareEpsilon)
      dir = cXAxisVector;
   dir.normalize();


   // Get attacking players force multiplier
   float playerForceMultiplier = 1.0f;
   BEntity* pAttacker = gWorld->getEntity(attackerID);
   if (pAttacker)
   {
      const BPlayer* pAttackingPlayer = pAttacker->getPlayer();
      if (pAttackingPlayer)
         playerForceMultiplier = pAttackingPlayer->getWeaponPhysicsMultiplier();
   }

   // figure out force (lerp based on distance factor then add 0-10%
   float forceFactor = distanceFactor + getRandRangeFloat(cSimRand, 0.0f, 0.1f);
   forceFactor = Math::Clamp(forceFactor, 0.0f, 1.0f);
   float force = Math::Lerp(pDamageInfo->getPhysicsForceMin() * playerForceMultiplier, pDamageInfo->getPhysicsForceMax() * playerForceMultiplier, forceFactor);
   force *= forceScaler;

   // calculate launch direction using launch angle
   // Launch angle is lerped on distance factor then add 0-10%
   float launchFactor = distanceFactor + getRandRangeFloat(cSimRand, 0.0f, 0.1f);
   launchFactor = Math::Clamp(launchFactor, 0.0f, 1.0f);
   float launchAngle = pDamageInfo->getPhysicsLaunchAngleMin() + launchFactor * (pDamageInfo->getPhysicsLaunchAngleMax() - pDamageInfo->getPhysicsLaunchAngleMin());

   BVector launchDir = dir;
   launchDir *= cosf(launchAngle * cPi / 180.0f);
   launchDir.y = sinf(launchAngle * cPi / 180.0f);
   BVector impulse = force * launchDir;

   BVector impactPoint = getPosition();
   impactPoint.y += getObstructionRadiusY() * 0.5f;
 
   // Clamshell impulses / anim
   if (pPhysReplacement->getPhysicsObject()->getType() == BPhysicsObject::cClamshell)
   {
      BClamshellPhysicsObject *pCPO = static_cast<BClamshellPhysicsObject*>(pPhysReplacement->getPhysicsObject());

      BVector impulse2 = impulse * 1.5f;

      float maxLateralOffset = 0.04f;
      float maxVertOffset = 0.8f;
      BVector impulseOffset = BVector(getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset),
                                      getRandRangeFloat(cSimRand, 0.0f, maxVertOffset),
                                      getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset));

      pCPO->applyPointImpulses(impulse, impulse, impulse2, impulseOffset);

      // Clamshell anim
      if(rocketOnDeath) // If this is a rocket death then play the anim that has the smoke trails on it.
         pPhysReplacement->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeClamshellRocket, true);
      else
      {
         if (pDamageInfo->getFlailThrownUnits())
            pPhysReplacement->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeFlail, true);
         else
            pPhysReplacement->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeClamshell, true);
      }
   }
   // Standard impulses
   else
   {
      pPhysReplacement->getPhysicsObject()->applyPointImpulse(impulse, impactPoint);
   }

   pPhysReplacement->setFlagSelectable(false);

   // clear the hitpoints
   BUnit* pPhysReplacementUnit = pPhysReplacement->getUnit();
   if (pPhysReplacementUnit)
   {
      pPhysReplacementUnit->setHitpoints(0.0);
      pPhysReplacementUnit->setFlagAlive(false);
   }

   //-- Play the death sound
   long cueIndex=getProtoObject()->getSound(cObjectSoundDeath);
   if(cueIndex!=cInvalidCueIndex)
      gWorld->getWorldSoundManager()->addSound(pPhysReplacement, -1, cueIndex, true, cInvalidCueIndex, true, true);

   // Throw attachments
   throwAttachments(pPhysReplacement, impulse);

   // Destroy this original unit   
   kill(true);
   
   // Make sure this object gets updated now that we need to destroy it
   setFlagNoUpdate(false);

   return pPhysReplacement;
}

//==============================================================================
//==============================================================================
bool BUnit::hasThrowableAttachments()
{
   BVisual* pVis = getVisual();
   if (!pVis)
      return false;

   uint32 numAttachments = static_cast<uint32>(pVis->mAttachments.getNumber());
   for (uint32 i = 0; i < numAttachments; i++)
   {
      BVisualItem* pAttachment = pVis->mAttachments[i];
      if (pAttachment && pAttachment->mModelAsset.mType == cVisualAssetGrannyModel && pAttachment->getFlag(BVisualItem::cFlagVisible) == true)
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BUnit::throwAttachments(BObject* pObj, BVector impulse, float pctChanceThrow, BEntityIDArray* pOutIds, int numAttachmentsToThrow)
{
   if (!pObj)
      return;
   BVisual* pVis = pObj->getVisual();
   if (!pVis)
      return;

   // Get part blueprint
   const static long normalBpID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate("part");
   const static long iceBpID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate("icePart");
   long partBPID = (getFlagShatterOnDeath()) ? iceBpID : normalBpID;

//-- FIXING PREFIX BUG ID 4500
   const BPhysicsObjectBlueprint* pPartBP = gPhysics->getPhysicsObjectBlueprintManager().get(partBPID, false);
//--
   if (!pPartBP)
      return;

   // Unit collision filter
   long collisionFilter = gWorld->getPhysicsWorld()->getUncollidableCollisionFilterInfo();
   if (pObj->getPhysicsObject())
      collisionFilter = pObj->getPhysicsObject()->getCollisionFilterInfo();

   // Unit matrix
   BMatrix worldMat;
   worldMat.makeOrient(pObj->getForward(), pObj->getUp(), pObj->getRight());
   worldMat.multTranslate(pObj->getPosition().x, pObj->getPosition().y, pObj->getPosition().z);

   uint32 numAttachments = static_cast<uint32>(pVis->mAttachments.getNumber());
   for (uint32 i = 0; i < numAttachments; i++)
   {
      // if we've thrown all the attachments we can, bail
      if (numAttachmentsToThrow == 0)
         break;

      BVisualItem* pAttachment = pVis->mAttachments[i];

      // Early out if the attachment is not a model (prevents throwing lights and particle effects) or visible
      if (pAttachment->mModelAsset.mType != cVisualAssetGrannyModel || pAttachment->getFlag(BVisualItem::cFlagVisible) == false)
         continue;

      // if we're counting, decrement
      if (numAttachmentsToThrow > 0)
         --numAttachmentsToThrow;

      // Random chance of throwing
      if(getRandRangeFloat(cSimRand, 0.0f, 1.0f) > pctChanceThrow)
         continue;

      BEntityID returnedId = createAndThrowAttachment(pAttachment, impulse, pObj->getPlayerID(), partBPID, worldMat, collisionFilter);

      // Turn off rendering of original attachment
      pAttachment->setFlag(BVisualItem::cFlagVisible, false);

      if (returnedId != cInvalidObjectID)
      {
         if (pOutIds)
            pOutIds->add(returnedId);

         notify(BEntity::cEventUnitAttachmentThrown, mID, (DWORD)returnedId, 0);
      }
   }
}


//=============================================================================
// BUnit::createAndThrowAttachment
//
// returns the entity id of the created object, or cInvalidObjectID if failed to create
//=============================================================================
BEntityID BUnit::createAndThrowAttachment(BVisualItem* pAttachment, BVector impulse, const BPhysicsObjectParams &physicsparams, BPlayerID playerID, const BMatrix& unitWorldMat, BVector centerOffset, float lifespan)
{
   // Build matrix for attachment
   BMatrix finalMatrix;
   finalMatrix.mult(pAttachment->mMatrix, unitWorldMat);

   // Create a new BObject from the visual item
   BObjectCreateParms params;
   finalMatrix.getTranslation(params.mPosition);
   finalMatrix.getForward(params.mForward);
   finalMatrix.getRight(params.mRight);
   params.mNoTieToGround = true;
   params.mPhysicsReplacement = true;
   params.mPlayerID = playerID;
   params.mProtoObjectID = gDatabase.getPOIDPhysicsThrownObject();
   params.mProtoSquadID = -1;
   params.mStartBuilt = false;
   params.mType = BEntity::cClassTypeObject;
   params.mMultiframeTextureIndex = getMultiframeTextureIndex();

   // [6-7-08 CJS] Adding the "true" parameter to strip off attachments--if attachments coming off of units start
   // acting funny, this is the place to change back (this was originally changed to stop turrets from firing when
   // a unit died).
   //BObject* pObject = gWorld->createVisItemPhysicsObject(params, pAttachment, pPhysicsInfo, pBPOverrides, true);
   BObject* pObject = gWorld->createVisItemPhysicsObjectDirect(params, pAttachment, physicsparams, true);

   if(!pObject)
      return cInvalidObjectID;

   pObject->copyAdditionalTextures(this);

   // Apply impulse
   if (pObject->getPhysicsObject())
   {
      const float cOffsetMax = 0.17f;
      BVector randOffset(getRandRangeFloat(cSimRand, -cOffsetMax, cOffsetMax), getRandRangeFloat(cSimRand, -cOffsetMax, cOffsetMax), getRandRangeFloat(cSimRand, -cOffsetMax, cOffsetMax));

      pObject->getPhysicsObject()->applyPointImpulse(impulse, params.mPosition + centerOffset + randOffset);
   }

   if (lifespan > 0.0f)
      pObject->setLifespan((DWORD)(1000.0f * lifespan));

   return(pObject->getID());
}

//=============================================================================
// BUnit::createAndThrowAttachment
//
// returns the entity id of the created object, or cInvalidObjectID if failed to create
//=============================================================================
BEntityID BUnit::createAndThrowAttachment(BVisualItem* pAttachment, BVector impulse, const BPhysicsInfo * pPhysicsInfo, const BPhysicsObjectBlueprintOverrides* pBPOverrides, BPlayerID playerID, const BMatrix& unitWorldMat, float lifespan)
{
   // Build matrix for attachment
   BMatrix finalMatrix;
   finalMatrix.mult(pAttachment->mMatrix, unitWorldMat);

   // Create a new BObject from the visual item
   BObjectCreateParms params;
   finalMatrix.getTranslation(params.mPosition);
   finalMatrix.getForward(params.mForward);
   finalMatrix.getRight(params.mRight);
   params.mNoTieToGround = true;
   params.mPhysicsReplacement = true;
   params.mPlayerID = playerID;
   params.mProtoObjectID = gDatabase.getPOIDPhysicsThrownObject();
   params.mProtoSquadID = -1;
   params.mStartBuilt = false;
   params.mType = BEntity::cClassTypeObject;

   // [6-7-08 CJS] Adding the "true" parameter to strip off attachments--if attachments coming off of units start
   // acting funny, this is the place to change back (this was originally changed to stop turrets from firing when
   // a unit died).
   BObject* pObject = gWorld->createVisItemPhysicsObject(params, pAttachment, pPhysicsInfo, pBPOverrides, true);

   if(!pObject)
      return cInvalidObjectID;

   pObject->copyAdditionalTextures(this);

   // Apply impulse
   if (pObject->getPhysicsObject())
   {
      const float cOffsetMax = 0.17f;
      BVector randOffset(getRandRangeFloat(cSimRand, -cOffsetMax, cOffsetMax), getRandRangeFloat(cSimRand, -cOffsetMax, cOffsetMax), getRandRangeFloat(cSimRand, -cOffsetMax, cOffsetMax));

      pObject->getPhysicsObject()->applyPointImpulse(impulse, params.mPosition + randOffset);
   }

   if (lifespan > 0.0f)
      pObject->setLifespan((DWORD)(1000.0f * lifespan));

   return(pObject->getID());
}

//=============================================================================
// BUnit::createAndThrowAttachment
//
// returns the entity id of the created object, or cInvalidObjectID if failed to create
//=============================================================================
BEntityID BUnit::createAndThrowAttachment(BVisualItem* pAttachment, BVector impulse, BPlayerID playerID, long partID, const BMatrix& unitWorldMat, long collisionFilter, float lifespan)
{
   // build physicsInfo
   const float cMinPhysicsDimension = 0.1f;
   BVector diff = pAttachment->mMaxCorner - pAttachment->mMinCorner;
   BVector halfDims;
   halfDims.x = _fabs(diff.x * 0.5f);
   halfDims.y = _fabs(diff.y * 0.5f);
   halfDims.z = _fabs(diff.z * 0.5f);
   if (halfDims.x < cMinPhysicsDimension)  halfDims.x = cMinPhysicsDimension;
   if (halfDims.y < cMinPhysicsDimension)  halfDims.y = cMinPhysicsDimension;
   if (halfDims.z < cMinPhysicsDimension)  halfDims.z = cMinPhysicsDimension;

   BVector center = (pAttachment->mMaxCorner + pAttachment->mMinCorner) * 0.5f;

   #ifdef SYNC_Unit
      if (getProtoObject())
      {
         syncUnitActionData("BUnit::createAndThrowAttachment unit name", getProtoObject()->getName());
      }
      else
      {
         syncUnitActionData("BUnit::createAndThrowAttachment PID", getProtoID());
      }
      syncUnitActionData("BUnit::createAndThrowAttachment attachment name", pAttachment->mpName ? *(pAttachment->mpName) : "NULL");
      syncUnitActionData("BUnit::createAndThrowAttachment minCorner", pAttachment->mMinCorner);
      syncUnitActionData("BUnit::createAndThrowAttachment maxCorner", pAttachment->mMaxCorner);
      syncUnitActionData("BUnit::createAndThrowAttachment forward", pAttachment->mMatrix.getRow(2));
   #endif

   BPhysicsInfo physInfo;
   BPhysicsObjectBlueprintOverrides bpOverrides;
   bpOverrides.setHalfExtents(halfDims);
   physInfo.setCenterOffset(center);
   physInfo.setThrownByProjectiles(true);
   physInfo.setBlueprintID(0, partID);
   if (collisionFilter >= 0)
      bpOverrides.setCollisionFilterInfo(collisionFilter);

   return(createAndThrowAttachment(pAttachment, impulse, &physInfo, &bpOverrides, playerID, unitWorldMat, lifespan));
}

//==============================================================================
//==============================================================================
BObject* BUnit::killAndThrowPhysicsReplacement(BVector dir, BVector pos, long matchBoneHandle)
{
   // This is almost identical to createAndThrowPhysicsReplacement with some slight simplifications
   // This should only be called for killAndThrow anim tags.

   // Create and throw replacement object
   BObject* pPhysReplacement = createPhysicsReplacement();
   if (pPhysReplacement && pPhysReplacement->getPhysicsObject())
   {
      const float fudgeFactor = 0.5f;
      BVector impulse = dir * (pPhysReplacement->getPhysicsObject()->getMass() * fudgeFactor);

      // Clamshell impulses / anim
      if (pPhysReplacement->getPhysicsObject()->getType() == BPhysicsObject::cClamshell)
      {
         // Flail anim
         pPhysReplacement->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeFlail, true, true);
         // Compute the animation so that it is initialized properly and we can get the matrix for the bone to match
         pPhysReplacement->computeAnimation();

         // Set pos/rot of new replacement to match this unit
         if (pPhysReplacement->getVisual())
         {
            // Get model space bone of replacement unit
            BMatrix replacementBoneMtxMS;
            if (pPhysReplacement->getVisual()->getBone(matchBoneHandle, NULL, &replacementBoneMtxMS, NULL, NULL))
            {
               // Get world space bone of current unit
               BMatrix currentWorldMatrix, currentBoneMatrixWS;
               getWorldMatrix(currentWorldMatrix);
               getVisual()->getBone(matchBoneHandle, NULL, &currentBoneMatrixWS, NULL, &currentWorldMatrix);

               // Calc and set new replacement unit world matrix so that the match bone's are identical between
               // the old and new.
               replacementBoneMtxMS.invert();
               BMatrix newWorldMatrix;
               newWorldMatrix.mult(replacementBoneMtxMS, currentBoneMatrixWS);
               pPhysReplacement->setWorldMatrix(newWorldMatrix);
            }
         }

         // Apply the impulse.  Don't add in any of the randomness because we want the velocity to match the anim
         BVector impulse2 = impulse * 1.5f;
         /*
         float maxLateralOffset = 0.04f;
         float maxVertOffset = 0.8f;
         BVector impulseOffset = BVector(getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset),
            getRandRangeFloat(cSimRand, 0.0f, maxVertOffset),
            getRandRangeFloat(cSimRand, -maxLateralOffset, maxLateralOffset));
         */
         BVector impulseOffset = cOriginVector;

         BClamshellPhysicsObject *pCPO = static_cast<BClamshellPhysicsObject*>(pPhysReplacement->getPhysicsObject());
         pCPO->applyPointImpulses(impulse, impulse, impulse2, impulseOffset);
      }
      // Standard impulses
      else
      {
         pPhysReplacement->getPhysicsObject()->applyPointImpulse(impulse, pos);
      }

      //-- Play the death sound
      long cueIndex=getProtoObject()->getSound(cObjectSoundDeath);
      if(cueIndex!=cInvalidCueIndex)
         gWorld->getWorldSoundManager()->addSound(pPhysReplacement, -1, cueIndex, true, cInvalidCueIndex, true, true);

      // Throw attachments
      throwAttachments(pPhysReplacement, impulse);

      // Destroy this original unit   
      setFlagNoRender(true);
      kill(true);

      // Make sure this object gets updated now that we need to destroy it
      setFlagNoUpdate(false);
   }

   return pPhysReplacement;
}

//==============================================================================
//==============================================================================
BSimCollisionResult BUnit::checkForCollisions(const BEntityIDArray& ignoreUnits,
                                              BVector pos1, BVector pos2, DWORD lastPathTime, bool collideWithUnits,
                                              BEntityIDArray& collideUnits, BVector* intersectionPoint, bool checkInfantryOnly,
                                              bool checkMovingUnits, bool checkSquads, bool checkBuildings)
{
   static BObstructionNodePtrArray collisionObs;
   collisionObs.setNumber(0);
   BVector tempIntersectionPoint(0.0f);

   collideUnits.clear();

   setFlagMotionCollisionChecked(true);

   //Actually do the collision check.  Check any terrain or non-moving units.
   // jce [10/10/2008] -- corrected flags here.  Changed to using AllCollideableUnits and take out moving if needed.
   long lObOptions = BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeBlockLandUnits;
   if (!checkMovingUnits)
      lObOptions &= ~BObstructionManager::cIsNewTypeCollidableMovingUnit;
   if (checkSquads)
      lObOptions |= BObstructionManager::cIsNewTypeAllCollidableSquads;
   long lObNodeType = BObstructionManager::cObsNodeTypeAll;

   // Oh snap!  We're out of the playable bounds we better path like we have wings.
   if (getFlagFlying() || (getPlayer() && !getPlayer()->isHuman() && isOutsidePlayableBounds()))
   {
      lObOptions = 0;
   }

   // Do the obstruction check
   gObsManager.begin(BObstructionManager::cBeginEntity, getID().asLong(), getClassType(), lObOptions, 
      lObNodeType, 0, cDefaultRadiusSofteningFactor, &ignoreUnits, canJump());
   bool intersectionFound = gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, pos1, pos2, 
      true, tempIntersectionPoint, collisionObs);
   gObsManager.end();

   // Save intersection point
   if (intersectionPoint != NULL)
   {
      if (intersectionFound)
         *intersectionPoint = tempIntersectionPoint;
      else
         *intersectionPoint = pos2;
   }

   // Check the type of collisions returned
   uint obIndex;
   uint obCount = collisionObs.getNumber();
   for (obIndex = 0; obIndex < obCount; obIndex++)
   {
      BOPObstructionNode* pObstructionNode = collisionObs[obIndex];
      if (pObstructionNode == NULL)
         continue;

      BEntity* pObject = pObstructionNode->mObject;
      if (pObject == NULL)
      {
         return cSimCollisionTerrain;
      }
      if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeEdgeofMap)
      {
         return cSimCollisionEdgeOfMap;
      }
      else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeTerrain)
      {
         return cSimCollisionTerrain;
      }
      else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeUnit)
      {
//-- FIXING PREFIX BUG ID 4504
         const BUnit* pCollisionUnit = pObject->getUnit();
//--

         // If unit hasn't moved since path was made, ignore this unit collision
         if (pCollisionUnit && (pCollisionUnit->getLastMoveTime() < lastPathTime))
         {
            continue;
         }

         //-- Don't collide with out own moving units
         //DCP 07/17/07: Turn this off given the temp (?) change above with the flags.
         //if (pObject->isMoving() && pObject->getPlayerID() == getPlayerID())
         //   continue;

         if (pObstructionNode->getHull()->inside(pos1))
            continue;

         //Add this to the return list 
         bool isActualUnit=false;
         if (pCollisionUnit && pCollisionUnit->getProtoObject())
         {
            // Are we supposed to collide with infantry only? (run-over attack)
            if (checkInfantryOnly && !pCollisionUnit->getProtoObject()->isType(gDatabase.getObjectType("Infantry")))
               continue;

            long objectClass = pCollisionUnit->getProtoObject()->getObjectClass();
            if ((objectClass == cObjectClassUnit) ||
                (objectClass == cObjectClassSquad) ||
                (checkBuildings && (objectClass == cObjectClassBuilding)))
            {
               isActualUnit=true;
               collideUnits.add(pObject->getID());
            }
         }

         //If we're colliding with units or the thing we hit isn't a unit, bail now.
         if (collideWithUnits || !isActualUnit)
            return cSimCollisionUnit;
      }
      else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeSquad)
      {
//-- FIXING PREFIX BUG ID 4505
         const BSquad* pCollisionSquad = pObject->getSquad();
//--

         // If squad hasn't moved since path was made, ignore this unit collision
         if (pCollisionSquad && (pCollisionSquad->getLastMoveTime() < lastPathTime))
            continue;

         //if (pObstructionNode->getHull()->inside(pos1))
         //   continue;

         collideUnits.add(pObject->getID());
      }
   }

   return cSimCollisionNone;
}

//==============================================================================
//==============================================================================
float BUnit::getPathingRange() const
{
   float range = 0.0f;
//-- FIXING PREFIX BUG ID 4506
   const BTactic* pTactic = getTactic();
//--
   if (pTactic != NULL)
   {
      int weaponIndex;
      for (weaponIndex = 0; weaponIndex < pTactic->getNumberWeapons(); weaponIndex++)
      {
         const BWeapon* pWeapon = pTactic->getWeapon(weaponIndex);
         if (pWeapon != NULL)
         {
            range = Math::Max(range, pWeapon->mMaxRange);
         }
      }
   }

   return range;
}

#if 0 // SLB: Tim doesn't want to units to reveal eachother because of accidental damage
//==============================================================================
//==============================================================================
void BUnit::damageBy(BEntityID attackingUnitID, BTeamID attackingTeamID)
{
   BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<BUnitActionUnderAttack*>(getActionByType(BAction::cActionTypeUnitUnderAttack));
   if (!pUnitActionUnderAttack)
   {
      pUnitActionUnderAttack = reinterpret_cast<BUnitActionUnderAttack*>(gActionManager.createAction(BAction::cActionTypeUnitUnderAttack));
      addAction(pUnitActionUnderAttack);
   }

   pUnitActionUnderAttack->addDamage(attackingUnitID, attackingTeamID, gWorld->getGametime() + gDatabase.getAttackedRevealerLifespan());
}


//==============================================================================
//==============================================================================
bool BUnit::wasDamagedBy(BEntityID attackingUnitID, BTeamID attackingTeamID) const
{
   const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<const BUnitActionUnderAttack*>(getActionByTypeConst(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
   {
      uint index;
      return pUnitActionUnderAttack->wasDamagedBy(attackingUnitID, attackingTeamID, index);
   }

   return false;
}
#endif

//==============================================================================
// Get the number of garrisoned units that are in cover
//==============================================================================
int BUnit::getNumCoverUnits() const
{
   int result = 0;

   // No contained units no update
   if (!getFlagHasGarrisoned())
      return (result);

   // Look through contained units
   uint numRefs = getNumberEntityRefs();
   for (uint i = 0; i < numRefs; i++)
   {
//-- FIXING PREFIX BUG ID 4508
      const BEntityRef* pRef = getEntityRefByIndex(i);
//--
      if (pRef && (pRef->mType == BEntityRef::cTypeContainUnit))
      {
//-- FIXING PREFIX BUG ID 4507
         const BUnit* pContainedUnit = gWorld->getUnit(pRef->mID);
//--
         if (pContainedUnit && pContainedUnit->getFlagInCover())
         {
            result++;
         }
      }
   }   

   return (result);
}

//==============================================================================
// Get the units that are in cover
//==============================================================================
BEntityIDArray BUnit::getCoverUnits(BPlayerID thisPlayerOnly /*= cInvalidPlayerID*/)
{
   // Find contained or attached units
   BEntityIDArray unitList;
   if (getFlagHasGarrisoned())
   {
      uint numEntityRefs = getNumberEntityRefs();
      for (uint i = 0; i < numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 4510
         const BEntityRef* pRef = getEntityRefByIndex(i);
//--
         if (pRef && (pRef->mType == BEntityRef::cTypeContainUnit))
         {
            BEntityID unitID = pRef->mID;
//-- FIXING PREFIX BUG ID 4509
            const BUnit* pContainUnit = gWorld->getUnit(unitID);
//--
            if (pContainUnit && pContainUnit->getFlagInCover())
            {
               if ((thisPlayerOnly == cInvalidPlayerID) || (pContainUnit->getPlayerID() == thisPlayerOnly))
               {
                  unitList.uniqueAdd(unitID);
               }
            }
         }
      }
   }

   return (unitList);
}

//==============================================================================
// Determine if this unit has any available cover points
//==============================================================================
bool BUnit::hasAvailableCover()
{
   bool result = false;
//-- FIXING PREFIX BUG ID 4400
   const BVisual* pVisual = getVisual();
//--
   if (pVisual)
   {
      int numCoverPoints = pVisual->getNumberPoints(cActionAnimationTrack, cVisualPointCover);
      int numUsedCoverPoints = getNumCoverUnits();
      result = ((numCoverPoints > 0) && (numUsedCoverPoints < numCoverPoints));      
   }

   return (result);
}

//==============================================================================
// Get the next available cover point in world space
//==============================================================================
BVector BUnit::getNextAvailableCoverPoint()
{
   BVector coverPos = cInvalidVector;   
   if (hasAvailableCover())
   {
//-- FIXING PREFIX BUG ID 4511
      const BVisual* pVisual = getVisual();
//--
      int numUsedCoverPoints = getNumCoverUnits();

      // Find cover point
      int coverPointHandle = -1;
      for (int i = 0; i <= numUsedCoverPoints; i++)
      {
         coverPointHandle = pVisual->getNextPointHandle(cActionAnimationTrack, coverPointHandle, cVisualPointCover);
      }

      BVector result = cInvalidVector;
      if (pVisual->getPointPosition(cActionAnimationTrack, coverPointHandle, result))
      {
         BMatrix mat;
         getWorldMatrix(mat);
         mat.transformVectorAsPoint(result, coverPos);                        
      }
   }

   return (coverPos);
}

//===================================================================================================================
// If a cover point becomes available and there are garrisoned units that can take it assign a unit to a cover point
//===================================================================================================================
void BUnit::updateCover()
{
   // If cover is available
   if (hasAvailableCover())
   {  
      // If the number of contained units equals the number of cover units then bail
      BEntityIDArray containedUnits = getGarrisonedUnits();
      uint numContainedUnits = containedUnits.getSize();      
      BEntityIDArray coverUnits = getCoverUnits();
      uint numCoverUnits = coverUnits.getSize();
      if (numContainedUnits == numCoverUnits)
      {
         return;
      }

      // Check whether there really is another available cover point.  If the vis file specifies more cover
      // points than the number of bones in the model file then the hasAvailableCover check will always return
      // true.  This check will fix that case and prevent cover units' orientation from being set every update,
      // which prevented them from attacking.
      // This whole system should probably be changed to know which unit is at which cover point so it doesn't ever
      // reset units who are already in cover.
      BVector tempCoverPoint = getNextAvailableCoverPoint();         
      if (tempCoverPoint.almostEqual(cInvalidVector))
         return;

      // If some cover points are already taken we need to reset the units in cover
      for (uint i = 0; i < numCoverUnits; i++)
      {
         BUnit* pCoverUnit = gWorld->getUnit(coverUnits[i]);
         if (pCoverUnit)
         {
            // Ignore units whose parent squad is in the process of garrisoning or ungarrisoning
//-- FIXING PREFIX BUG ID 4513
            const BSquad* pParentSquad = pCoverUnit->getParentSquad();
//--
            if (pParentSquad && (pParentSquad->getFlagIsGarrisoning() || pParentSquad->getFlagIsUngarrisoning()))
            {
               continue;
            }

            if (pCoverUnit->getFlagLOSMarked())
            {
               pCoverUnit->markLOSOff();
            }
            #ifdef SYNC_Unit
               syncUnitData("BUnit::updateCover 1", mPosition);
            #endif
            pCoverUnit->setPosition(mPosition);
            const BProtoObject* pCoverProtoObject = pCoverUnit->getProtoObject();
            if (pCoverProtoObject)
            {
               pCoverUnit->setFlagNonMobile(pCoverProtoObject->getFlagNonMobile());
            }
            pCoverUnit->setFlagInCover(false);
            pCoverUnit->updateObstruction();
         }
      }

      //XXXHalwes - 2/22/2008 - Cover priority assignment logic goes here if there is ever a design for that.

      // Re-assign the cover points
      for (uint i = 0; i < numContainedUnits; i++)
      {
         BUnit* pContainedUnit = gWorld->getUnit(containedUnits[i]);
         if (pContainedUnit)
         {
            // Ignore units whose parent squad is in the process of garrisoning or ungarrisoning
//-- FIXING PREFIX BUG ID 4514
            const BSquad* pParentSquad = pContainedUnit->getParentSquad();
//--
            if (pParentSquad && (pParentSquad->getFlagIsGarrisoning() || pParentSquad->getFlagIsUngarrisoning()))
            {
               continue;
            }

            const BProtoObject* pContainedProtoObject = pContainedUnit->getProtoObject();
            if (pContainedProtoObject && pContainedProtoObject->isType(gDatabase.getOTIDInfantry()))
            {
               BVector coverPoint = getNextAvailableCoverPoint();         
               if (coverPoint != cInvalidVector)
               {
                  if (pContainedUnit->getFlagLOS() && !pContainedUnit->getFlagLOSMarked())
                  {
                     pContainedUnit->markLOSOn();
                  }
                  pContainedUnit->setForward(mForward);
                  pContainedUnit->setRight(mRight);
                  pContainedUnit->setUp(mUp);
                  #ifdef SYNC_Unit
                     syncUnitData("BUnit::updateCover 2", coverPoint);
                  #endif
                  pContainedUnit->setPosition(coverPoint);      
                  pContainedUnit->setFlagNonMobile(true);
                  pContainedUnit->setFlagInCover(true);
                  pContainedUnit->updateObstruction();
                  if (!pParentSquad || !pParentSquad->isFrozen())
                     pContainedUnit->setAnimationEnabled(true);

                  if (!hasAvailableCover())
                  {
                     break;
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::addReveal(BTeamID teamID)
{
   BUnitActionRevealToTeam* pUnitActionRevealToTeam = reinterpret_cast<BUnitActionRevealToTeam*>(getActionByType(BAction::cActionTypeUnitRevealToTeam));
   if (!pUnitActionRevealToTeam)
   {
      pUnitActionRevealToTeam = reinterpret_cast<BUnitActionRevealToTeam*>(gActionManager.createAction(BAction::cActionTypeUnitRevealToTeam));
      addAction(pUnitActionRevealToTeam);
   }

   pUnitActionRevealToTeam->addReveal(teamID);
}


//==============================================================================
//==============================================================================
void BUnit::removeReveal(BTeamID teamID)
{
   BUnitActionRevealToTeam* pUnitActionRevealToTeam = reinterpret_cast<BUnitActionRevealToTeam*>(getActionByType(BAction::cActionTypeUnitRevealToTeam));
   if (pUnitActionRevealToTeam)
      pUnitActionRevealToTeam->removeReveal(teamID);
}


//==============================================================================
//==============================================================================
void BUnit::addAttackingUnit(BEntityID attackingUnitID, BActionID attackingActionID)
{
   if (!gWorld->getUnit(attackingUnitID))
      return;

   BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<BUnitActionUnderAttack*>(getActionByType(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
   {
      pUnitActionUnderAttack->addAttackingUnit(attackingUnitID, attackingActionID);
   }
   else
   {
      pUnitActionUnderAttack = reinterpret_cast<BUnitActionUnderAttack*>(gActionManager.createAction(BAction::cActionTypeUnitUnderAttack));
      addAction(pUnitActionUnderAttack);
      pUnitActionUnderAttack->addAttackingUnit(attackingUnitID, attackingActionID);
   }
}


//==============================================================================
//==============================================================================
void BUnit::removeAttackingUnit(BEntityID attackingUnitID, BActionID attackingActionID)
{
   if (gWorldReset)
      return;
   BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<BUnitActionUnderAttack*>(getActionByType(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
   {
      pUnitActionUnderAttack->removeAttackingUnit(attackingUnitID, attackingActionID);
   }
}


//==============================================================================
//==============================================================================
bool BUnit::isBeingAttackedByUnit(BEntityID unitID) const
{
   const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<const BUnitActionUnderAttack*>(getActionByTypeConst(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
      return (pUnitActionUnderAttack->isBeingAttackedByUnit(unitID));
   else
      return (false);
}


//==============================================================================
//==============================================================================
bool BUnit::isBeingAttacked() const
{
   const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<const BUnitActionUnderAttack*>(getActionByTypeConst(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
      return (pUnitActionUnderAttack->isBeingAttacked());
   else
      return (false);
}


//==============================================================================
//==============================================================================
uint BUnit::getNumberAttackingUnits() const
{
   const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<const BUnitActionUnderAttack*>(getActionByTypeConst(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
      return (pUnitActionUnderAttack->getNumberAttackingUnits());
   else
      return (0);
}


//==============================================================================
//==============================================================================
BEntityID BUnit::getAttackingUnitByIndex(uint index) const
{
   const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<const BUnitActionUnderAttack*>(getActionByTypeConst(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
      return (pUnitActionUnderAttack->getAttackingUnitByIndex(index));
   else
      return (cInvalidObjectID);
}


//==============================================================================
//==============================================================================
uint BUnit::getAttackingUnits(BEntityIDArray& attackingUnits) const
{
   const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<const BUnitActionUnderAttack*>(getActionByTypeConst(BAction::cActionTypeUnitUnderAttack));
   if (pUnitActionUnderAttack)
      return (pUnitActionUnderAttack->getAttackingUnits(attackingUnits));
   else
   {
      attackingUnits.resize(0);
      return (0);
   }
}

//==============================================================================
//==============================================================================
void BUnit::initVisualAmmo()
{
//-- FIXING PREFIX BUG ID 4459
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
   {
      mVisualAmmo.clear();
      return;
   }

   mVisualAmmo.setNumber(pTactic->getNumberWeapons());
   for (uint i=0; i < mVisualAmmo.getSize(); i++)
   {
      const BWeapon* pWeapon=pTactic->getWeapon(i);
      BDEBUG_ASSERT(pWeapon);
      mVisualAmmo[i]=pWeapon->mpStaticData->mVisualAmmo;
   }
}

//==============================================================================
// Clears invalid and dead target ids from the multi target array
//==============================================================================
void BUnit::clearInvalidMultiTargets(BUnit* pTargetUnit)
{
   int numTargets = mMultiTargets.getNumber();
   for (int i = (numTargets - 1); i >= 0; i--)
   {
//-- FIXING PREFIX BUG ID 4460
      const BUnit* pUnit = gWorld->getUnit(mMultiTargets[i]);
//--
      if (pUnit && pUnit->isAlive() && pTargetUnit && (pTargetUnit->getProtoID() == pUnit->getProtoID()))
      {         
         continue;
      }

      mMultiTargets.removeIndex(i);
   }
}

//==============================================================================
// Get the contained or attached units
//==============================================================================
BEntityIDArray BUnit::getGarrisonedUnits(BPlayerID thisPlayerOnly /*= cInvalidPlayerID*/)
{
   // Find contained or attached units
   BEntityIDArray unitList;
   if (getFlagHasGarrisoned() || getFlagHasAttached())
   {
      uint numEntityRefs = getNumberEntityRefs();
      for (uint i = 0; i < numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 4462
         const BEntityRef* pRef = getEntityRefByIndex(i);
//--
         if (pRef && ((pRef->mType == BEntityRef::cTypeContainUnit) || (pRef->mType == BEntityRef::cTypeAttachObject)))
         {
            BEntityID unitID = pRef->mID;
//-- FIXING PREFIX BUG ID 4461
            const BUnit* pContainUnit = gWorld->getUnit(unitID);
//--
            if (pContainUnit)
            {
               if ((thisPlayerOnly == cInvalidPlayerID) || (pContainUnit->getPlayerID() == thisPlayerOnly))
               {
                  unitList.uniqueAdd(unitID);
               }
            }
         }
      }
   }

   return (unitList);
}

//====================================================================================================
// See if this unit has any garrisoned or attached units that are enemies of the provided player ID.
// Note: This will only return true if the garrisoned units are visible to the player, unless the
// ignore visibility parameter is set.
//====================================================================================================
bool BUnit::hasGarrisonedEnemies(BPlayerID playerID, bool ignoreVisibility /*= false*/) const
{
   bool result = false;
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer && (getFlagHasGarrisoned() || getFlagHasAttached()))
   {
      BTeamID playerTeamID = pPlayer->getTeamID();
      uint numEntityRefs = getNumberEntityRefs();
      for (uint i = 0; i < numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 4464
         const BEntityRef* pRef = getEntityRefByIndex(i);
//--
         if (pRef && ((pRef->mType == BEntityRef::cTypeContainUnit) || (pRef->mType == BEntityRef::cTypeAttachObject)))
         {
            BEntityID unitID = pRef->mID;
//-- FIXING PREFIX BUG ID 4463
            const BUnit* pContainUnit = gWorld->getUnit(unitID);
//--
            if (pContainUnit && pPlayer->isEnemy(pContainUnit->getPlayerID()) && (ignoreVisibility || pContainUnit->isVisibleOrDoppled(playerTeamID)))
            {               
               result = true;
               break;
            }
         }
      }
   }

   return (result);
}

//==============================================================================
// Get the team ID for the garrisoned units
//==============================================================================
BTeamID BUnit::getGarrisonedTeam()
{
   BTeamID result = cInvalidTeamID;
   BEntityIDArray garrisonedUnits = getGarrisonedUnits();
   uint numGarrisonedUnits = garrisonedUnits.getSize();
   for (uint i = 0; i < numGarrisonedUnits; i++)
   {
//-- FIXING PREFIX BUG ID 4465
      const BUnit* pGarrisonedUnit = gWorld->getUnit(garrisonedUnits[i]);
//--
      if (pGarrisonedUnit)
      {
         result = pGarrisonedUnit->getTeamID();
         break;
      }
   }

   return (result);
}

//==============================================================================
//==============================================================================
bool BUnit::hasVisualAmmo(uint index) const
{
   BDEBUG_ASSERT(index < mVisualAmmo.getSize());
   return (mVisualAmmo[index] > 0);
}

//==============================================================================
//==============================================================================
uint BUnit::getVisualAmmo(uint index) const
{
   BDEBUG_ASSERT(index < mVisualAmmo.getSize());
   return (mVisualAmmo[index]);
}

//==============================================================================
//==============================================================================
void BUnit::incrementVisualAmmo(uint index, uint v)
{
   BDEBUG_ASSERT(index < mVisualAmmo.getSize());
   mVisualAmmo[index]+=v;
}

//==============================================================================
//==============================================================================
void BUnit::decrementVisualAmmo(uint index, uint v)
{
   BDEBUG_ASSERT(index < mVisualAmmo.getSize());
   if (mVisualAmmo[index] < v)
      mVisualAmmo[index]=0;
   else
      mVisualAmmo[index]-=v;
}

//==============================================================================
//==============================================================================
void BUnit::setVisualAmmo(uint index, uint v)
{
   BDEBUG_ASSERT(index < mVisualAmmo.getSize());
   mVisualAmmo[index]=v;
}

//==============================================================================
//==============================================================================
void BUnit::resetVisualAmmo(uint index)
{
//-- FIXING PREFIX BUG ID 4466
   const BTactic* pTactic=getTactic();
//--
   if (!pTactic)
   {
      mVisualAmmo.clear();
      return;
   }

   const BWeapon* pWeapon=pTactic->getWeapon(index);
   if (!pWeapon)
   {
      mVisualAmmo.clear();
      return;
   }
   BDEBUG_ASSERT(pWeapon);
   if (index < mVisualAmmo.getSize())
      mVisualAmmo[index]=pWeapon->mpStaticData->mVisualAmmo;
   else
      mVisualAmmo.clear();
}

//==============================================================================
//==============================================================================
void BUnit::recomputeVisual()
{
   if (!mpVisual)
      return;
//-- FIXING PREFIX BUG ID 4467
   const BProtoVisual* pProtoVisual=mpVisual->getProtoVisual();
//--
   if (!pProtoVisual)
      return;

   // Send notification in case any actions need to respond
   notify(cEventRecomputeVisualStarting, mID, 0, 0);

   BMatrix worldMatrix;
   getWorldMatrix(worldMatrix);

   DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

   // Any clones?
   long clone = -1;
   for (long track = 0; track < cNumAnimationTracks; track++)
   {
      if (mpVisual->mAnimationTrack[track].mIsClone)
      {
         clone = track;
         break;
      }
   }

   // Yes. Set animation of master than copy it to the clone.
   if (clone != -1)
   {
      long master = 1 - clone;
      mpVisual->setAnimation(master, mpVisual->getAnimationType(master), false, playerColor, worldMatrix, mpVisual->getAnimationPosition(master), mAnimationState.getForceAnim());
      mpVisual->copyAnimationTrack(master, clone, false, playerColor, worldMatrix);
   }
   // No. Just set animations.
   else
   {
      for (long track = 0; track < cNumAnimationTracks; track++)
      {
         mpVisual->setAnimation(track, mpVisual->getAnimationType(track), false, playerColor, worldMatrix, mpVisual->getAnimationPosition(track), mAnimationState.getForceAnim());
      }
   }

   mpVisual->validateAnimationTracks();
   updateVisualVisibility();

   // Send notification in case any actions need to respond
   notify(cEventRecomputeVisualCompleted, mID, 0, 0);
}

//==============================================================================
//==============================================================================
void BUnit::removePersistentTacticActions()
{
   mActions.clearPersistentTacticActions();
}

//==============================================================================
// BUnit::updateCorpseCollisions
//==============================================================================
void BUnit::updateCorpseCollisions()
{
   BObstructionNodePtrArray obstructions;
   float obstructionRadius = getObstructionRadius();
   if (isType(gDatabase.getOTIDBuilding()))
   {
      obstructionRadius *= 1.5f; // Make sure debris is clear of buildings
   }
   float x1 = mPosition.x - obstructionRadius;
   float z1 = mPosition.z - obstructionRadius;
   float x2 = mPosition.x + obstructionRadius;
   float z2 = mPosition.z + obstructionRadius;
   gObsManager.findObstructions(x1, z1, x2, z2, 0.0f, BObstructionManager::cIsNewTypeNonCollidableUnit, BObstructionManager::cObsNodeTypeCorpse, -1, false, obstructions);

   long numObstructions = obstructions.getNumber();
   for (long i = 0; i < numObstructions; i++)
   {
      const BOPObstructionNode* pNode = obstructions.get(i);
      BDEBUG_ASSERT(pNode);
      BDEBUG_ASSERT(pNode->mObject);

//-- FIXING PREFIX BUG ID 4468
      const BUnit* pUnit = pNode->mObject->getUnit();
//--

      if (pUnit)
      {
         #ifdef SYNC_Unit
            syncUnitCode("BUnit::updateCorpseCollisions removeCorpse");
         #endif

// SLB: We have large thrown objects now, so this no longer makes sense.
//          if (isType(gDatabase.getOTIDInfantry()))
//          {
//             if (pUnit->getProtoID() != gDatabase.getPOIDPhysicsThrownObject())
//                gCorpseManager.removeCorpse(pUnit->getID());
//          }
//          else
            gCorpseManager.removeCorpse(pUnit->getID());
      }
   }
}

//==============================================================================
// BUnit::takeInfectionForm
//==============================================================================
void BUnit::takeInfectionForm()
{
   BProtoObjectID infectedPOID;
   BProtoSquadID infectedPSID;
   if (gDatabase.getInfectedForm(getProtoID(), infectedPOID, infectedPSID))
   {
      stop();
      mActions.clearActions();
      //Remove our opps in case anyone is listening for opp notifications.
      removeOpps();

      //Transform into the flood thing.
      transform(infectedPOID);
      setHitpoints(getProtoObject()->getHitpoints());
      setShieldpoints(getProtoObject()->getShieldpoints());

      setFlagTakeInfectionForm(false);
      mFlagInfected = false;
      mAnimationState.setDirty();

      //-- Add Unit Death Opportunity
      BUnitOpp* pNewOpp=BUnitOpp::getInstance();
      pNewOpp->init();
      pNewOpp->setType(BUnitOpp::cTypeInfectDeath);
      pNewOpp->setPriority(BUnitOpp::cPriorityDeath);
      pNewOpp->setUserData(static_cast<uint16>(mInfectionPlayerID));
      pNewOpp->generateID();
      if (!addOpp(pNewOpp))
      {
         BFAIL("Couldn't add an infection death opportunity");
         BUnitOpp::releaseInstance(pNewOpp);
      }
   }
}

//==============================================================================
// BUnit::infect
//==============================================================================
void BUnit::infect(BPlayerID attackerPlayerID)
{
   mFlagInfected = true;
   mInfectionPlayerID = attackerPlayerID;
   mAnimationState.setDirty();
}

//==============================================================================
// BUnit::isInfectable
//==============================================================================
bool BUnit::isInfectable()
{
   BProtoObjectID infectedPOID;
   BProtoSquadID infectedPSID;

   return (gDatabase.getInfectedForm(mProtoID, infectedPOID, infectedPSID));
/*
   if (!hasAnimation(cAnimTypeFloodDeath))
      return false;

   if (this->getProtoObject() && getProtoObject()->isType(gDatabase.getOTIDInfantry()))
      return true;

   return false;
*/
}

//==============================================================================
// BUnit::setAnimation
//==============================================================================
bool BUnit::setAnimation(BActionID requesterActionID, long state, long animType, bool applyInstantly, bool reset, long forceAnimID, bool lock, bool failOnCollision, const BProtoVisualAnimExitAction* pOverrideExitAction)
{
   BActionID controllerActionID = mControllers[BActionController::cControllerAnimation].getActionID();
   if ((controllerActionID == requesterActionID) || (controllerActionID == cInvalidActionID))
   {
      if (failOnCollision)
      {
         // Get animation data
         if (forceAnimID == -1)
         {
            BVisualAnimationData animationData = getVisual()->getAnimationData(cActionAnimationTrack, animType);
            forceAnimID = animationData.mAnimAsset.mIndex;
         }

         // Will it collide with anything?
//-- FIXING PREFIX BUG ID 4470
         const BGrannyAnimation* pGrannyAnim = gGrannyManager.getAnimation(forceAnimID);
//--
         if (pGrannyAnim->getMotionExtractionMode() != GrannyNoAccumulation)
         {
            BVector intersection;
            BVector estimatedEndPosition;
            BMatrix worldMatrix;
            getWorldMatrix(worldMatrix);
            worldMatrix.transformVectorAsPoint(pGrannyAnim->getTotalMotionExtraction(), estimatedEndPosition);
            if (getNearestCollision(getPosition(), estimatedEndPosition, true, true, false, intersection))
               return false;
         }
      }

      // Set animation
      setAnimationState(state, animType, applyInstantly, reset, forceAnimID, lock);
      if (pOverrideExitAction)
         overrideExitAction(pOverrideExitAction);

      return true;
   }

   return false;
}

//==============================================================================
// BUnit::setPostAnim
// SLB: Don't change this unless you know what you're doing
//==============================================================================
bool BUnit::setPostAnim(BActionID requesterActionID, const BProtoAction* pProtoAction, bool applyInstantly)
{
   const BVisual* pVisual = getVisual();
   long endAnimType = pProtoAction->getEndAnimType();
   bool playEndAnim = (pVisual && (endAnimType != -1) && (!pProtoAction->getPullUnits())) ? true : false;
   long playThisAnimType = playEndAnim ? endAnimType : -1;
   long animState = playEndAnim ? BObjectAnimationState::cAnimationStatePostAnim : BObjectAnimationState::cAnimationStateIdle;
   bool retVal = setAnimation(requesterActionID, animState, playThisAnimType, applyInstantly);

   if(playEndAnim)
   {
      // If we're playing a blocking post animation, lock it, but don't let it lock movement
      if (pProtoAction->isEndAnimNoInterrupt())
      {
         computeAnimation();
         if (isAnimationSet(animState, playThisAnimType))
         {
            lockAnimation((long)((getAnimationDuration(cActionAnimationTrack) - pVisual->getAnimationTweenTime(cActionAnimationTrack)) * 1000.0f), false);
            setFlagDontLockMovementAnimation(true);
         }
         else
         {
            retVal = setAnimation(requesterActionID, BObjectAnimationState::cAnimationStateIdle, -1, applyInstantly);
         }
      }

      //-- Play stop sound on root bone, if it is specified.
      if(pProtoAction->getEndAnimSoundCue() != cInvalidCueIndex)
      {            
         gWorld->getWorldSoundManager()->addSound(this, -1, pProtoAction->getEndAnimSoundCue(), false, cInvalidCueIndex, false, false);
      }            
   }

   return retVal;
}

//==============================================================================
// BUnit::isAnimationSet
//==============================================================================
bool BUnit::isAnimationSet(long state, long animType)
{
   if (state == getAnimationState())
      return (animType == getAnimationType(cActionAnimationTrack));

   return false;
}


//==============================================================================
//==============================================================================
bool BUnit::isNewBuildingConstructionAllowed(const BUnit* pBaseBuilding, BEntityID* pInProgressBuilding, bool secondaryQueue) const
{
   if (gConfig.isDefined(cConfigDisableOneBuilding))
      return true;

   if (!pBaseBuilding)
   {
      if (!getProtoObject()->getFlagSingleSocketBuilding())
         return true;
      pBaseBuilding = gWorld->getUnit(getBaseBuilding());
   }
      
   if (pBaseBuilding)
   {
      uint count = pBaseBuilding->getNumberEntityRefs();
      for (uint i=0; i<count; i++)
      {
         const BEntityRef* pChildBuildingRef = pBaseBuilding->getEntityRefByIndex(i);
         if (pChildBuildingRef->mType == BEntityRef::cTypeAssociatedBuilding)
         {
            const BUnit* pChildBuildingUnit = gWorld->getUnit(pChildBuildingRef->mID);
            if (pChildBuildingUnit && !pChildBuildingUnit->getFlagBuilt() && pChildBuildingUnit->isAlive() && !pChildBuildingUnit->getFlagBuildingConstructionPaused())
            {
               bool childSecondaryQueue = false;
               const BEntityRef* pSocketRef = pChildBuildingUnit->getFirstEntityRefByType(BEntityRef::cTypeParentSocket);
               if (pSocketRef)
               {
                  const BUnit* pSocketUnit = gWorld->getUnit(pSocketRef->mID);
                  if (pSocketUnit)
                     childSecondaryQueue = pSocketUnit->getProtoObject()->getFlagSecondaryBuildingQueue();
               }
               if (childSecondaryQueue==secondaryQueue)
               {
                  if (pInProgressBuilding)
                     *pInProgressBuilding = pChildBuildingUnit->getID();
                  return false;
               }
            }
         }
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
BEntityID BUnit::getBaseBuilding() const
{
   const BEntityRef* pBaseRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedBase);
   if (pBaseRef)
      return pBaseRef->mID;
   else
      return cInvalidObjectID;
}

//==============================================================================
//==============================================================================
void BUnit::queueBuildOther(BEntityID buildingID, bool secondaryQueue)
{
   BUnit* pBuilding = gWorld->getUnit(buildingID);
   if (pBuilding)
   {
      if (secondaryQueue)
         mFlagBuildOtherQueue2=true;
      else
         mFlagBuildOtherQueue=true;
      addEntityRef(BEntityRef::cTypeBuildQueueChild, buildingID, pBuilding->getProtoID(), secondaryQueue);
      pBuilding->addEntityRef(BEntityRef::cTypeBuildQueueParent, mID, 0, 0);
      pBuilding->setFlagBuildingConstructionPaused(true);
      gWorld->notify(BEntity::cEventTrainQueued, mID, 0, 0);
   }
}

//==============================================================================
//==============================================================================
void BUnit::updateBuildOtherQueue()
{
   for (int i=0; i<2; i++)
   {
      bool secondaryQueue = (i==0);
      if (secondaryQueue)
      {
         if (!mFlagBuildOtherQueue2)
            continue;
      }
      else
      {
         if (!mFlagBuildOtherQueue)
            continue;
      }

      if (!isNewBuildingConstructionAllowed(this, NULL, secondaryQueue))
         continue;

      const BEntityRef* pRef = getFirstEntityRefByData2(BEntityRef::cTypeBuildQueueChild, secondaryQueue);
      if (!pRef)
      {
         if (secondaryQueue)
            mFlagBuildOtherQueue2 = false;
         else
            mFlagBuildOtherQueue = false;
         continue;
      }

      BUnit* pUnit = gWorld->getUnit(pRef->mID);
      if (!pUnit || !pUnit->isAlive())
      {
         removeEntityRef(BEntityRef::cTypeBuildQueueChild, pRef->mID);
         continue;
      }

      pUnit->setFlagBuildingConstructionPaused(false);
      removeEntityRef(BEntityRef::cTypeBuildQueueChild, pRef->mID);
      pUnit->removeEntityRef(BEntityRef::cTypeBuildQueueParent, mID);
      gWorld->notify(BEntity::cEventTrainQueued, mID, 0, 0);
   }
}

#if DPS_TRACKER
//==============================================================================
//==============================================================================
void BUnit::damageDealt(float dmg)
{
   mDmgDealtBuffer += dmg;
}

//==============================================================================
//==============================================================================
void BUnit::clearDamageHistory()
{
   for(uint i=0; i < cMaxDamageHistory; i++)
      mDmgHistory[i] = -1.0f;   
}

//==============================================================================
//==============================================================================
void BUnit::updateDamageHistory()
{
   //-- Gets called once every second
   mDmgHistory[mDmgHistoryIndex] = mDmgDealtBuffer;

   mDmgHistoryIndex++;
   mDmgHistoryIndex =  mDmgHistoryIndex % cMaxDamageHistory;   

   mDmgDealtBuffer = 0.0f;
}

//==============================================================================
//==============================================================================
float BUnit::getTrackedDPS()
{
   float total = 0.0f;
   uint count = 0;
   for(uint i = 0; i < cMaxDamageHistory; i++)
   {
      if(mDmgHistory[i] != -1.0f)
      {
         count++;
         total += mDmgHistory[i];
      }
   }

   if(count < cMinDamageHistory)
      return -1.0f;

   total /= count;
   return total;
}
#endif

//==============================================================================
//==============================================================================
BUnitActionSecondaryTurretAttack* BUnit::getSecondaryTurretAttack(long hardpointID) const
{
   long id = getHardpointController(hardpointID);

   if (id != -1)
   {
      BAction* pAction = mActions.getActionByID(id);
      if (pAction && (pAction->getType() == BAction::cActionTypeUnitSecondaryTurretAttack))
         return reinterpret_cast<BUnitActionSecondaryTurretAttack*>(pAction);
   }

   return NULL;
}

//==============================================================================
//==============================================================================
void BUnit::upgradeLevel(int startLevel, int toLevel, bool doEffects, const BProtoObject* pOverrideProtoObject)
{
   int unitLevel=startLevel;
   while (unitLevel < toLevel)
   {
      unitLevel++;
      const BProtoObject* pProtoObject = pOverrideProtoObject ? pOverrideProtoObject : getProtoObject();
      int objectLevelCount = pProtoObject->getNumberLevels();
      for (int k=0; k<objectLevelCount; k++)
      {
         const BProtoObjectLevel* pLevel = pProtoObject->getLevel(k);
         if (pLevel->mLevel == unitLevel)
         {
            if (pLevel->mDamage != 1.0f)
               adjustDamageModifier(pLevel->mDamage);
            if (pLevel->mVelocity != 1.0f)
               adjustVelocityScalar(pLevel->mVelocity);
            if (pLevel->mAccuracy != 1.0f)
               adjustAccuracyScalar(pLevel->mAccuracy);
            if (pLevel->mWorkRate != 1.0f)
               adjustWorkRateScalar(pLevel->mWorkRate);
            if (pLevel->mWeaponRange != 1.0f)
               adjustWeaponRangeScalar(pLevel->mWeaponRange);
            if (pLevel->mDamageTaken != 1.0f)
               adjustDamageTakenScalar(pLevel->mDamageTaken);

            if (doEffects)
            {
               int effectID = pProtoObject->getLevelUpEffect();
               if (effectID != -1)
                  addAttachment(effectID);
            }
            break;
         }
         else if (pLevel->mLevel > unitLevel)
            break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::setAttackWaitTimer(int index, float val)
{ 
   if(index < 0 || index >= mAttackWaitTimer.getNumber()) 
      return; 

   mAttackWaitTimer[index].mTimer = val; 
}
//==============================================================================
//==============================================================================
float BUnit::getAttackWaitTimer(int index) const
{ 
   if(index < 0 || index >= mAttackWaitTimer.getNumber()) 
      return 0.0f; 
   
   return mAttackWaitTimer[index].mTimer; 
}
//==============================================================================
//==============================================================================
void BUnit::setPreAttackWaitTimer(int index, float val)
{ 
   if(index < 0 || index >= mAttackWaitTimer.getNumber()) 
      return; 

   mAttackWaitTimer[index].mPreAttackTimer = val; 
}
//==============================================================================
//==============================================================================
float BUnit::getPreAttackWaitTimer(int index) const
{ 
   if(index < 0 || index >= mAttackWaitTimer.getNumber()) 
      return 0.0f; 
   
   return mAttackWaitTimer[index].mPreAttackTimer; 
}
//==============================================================================
//==============================================================================
bool BUnit::attackWaitTimerOn(int index) 
{ 
   if(index < 0 || index >= mAttackWaitTimer.getNumber()) 
      return false; 
   
   return(mAttackWaitTimer[index].mTimer > 0.0f || mAttackWaitTimer[index].mPreAttackTimer > 0.0f);
}

//==============================================================================
//==============================================================================
bool BUnit::getPreAttackWaitTimerSet(int index) const
{
   if(index < 0 || index >= mAttackWaitTimer.getNumber()) 
      return false; 

   return mAttackWaitTimer[index].mPreAttackTimerSet;
}
//==============================================================================
//==============================================================================
void BUnit::setPreAttackWaitTimerSet(int index, bool val)
{
   if(index < 0 || index >= mAttackWaitTimer.getNumber()) 
      return; 

   mAttackWaitTimer[index].mPreAttackTimerSet = val;
}

//==============================================================================
// BUnit::createChildObjects
//==============================================================================
void BUnit::createChildObjects()
{
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject->getNumberChildObjects() > 0)
   {
      BSmallDynamicSimArray<BChildObjectData> childObjects;
      if (gWorld->getChildObjects(mPlayerID, pProtoObject->getID(), mPosition, mForward, childObjects))
      {
         BEntityID settlement = getAssociatedSettlement();
         BEntityID parkingLot = getAssociatedParkingLot();
         BUnit* pSettlement = gWorld->getUnit(settlement);
         BUnit* pParkingLot = gWorld->getUnit(parkingLot);
         uint startNumRefs = getNumberEntityRefs();
         BMatrix rotMat;
         bool doRallyPoint = false;
         BVector rallyPoint = cOriginVector;

         if (pProtoObject->isType(gDatabase.getOTIDBase()))
         {
            if (getExplorationGroup() != -1 && getPlayerID() != cInvalidPlayerID)
               gWorld->resetExplorationGroupTeamData(getExplorationGroup(), getPlayerID());
         }

         int count=childObjects.getNumber();
         for (int i=0; i<count; i++)
         {
//-- FIXING PREFIX BUG ID 4476
            const BChildObjectData& childData = childObjects[i];
//--

            // Don't create object if a user civ is specified and the user's civ doesn't match.
            if (childData.mUserCiv != -1)
            {
               if (gUserManager.getPrimaryUser()->getPlayer()->getCivID() != (int)childData.mUserCiv)
                  continue;
            }

            // Create squads that should be created one time only per player
            if (childData.mChildType == BProtoObjectChildObject::cTypeOneTimeSpawnSquad)
            {
               // Object type is a proto squad ID
               BProtoSquad* pProtoSquad = getPlayer()->getProtoSquad(childData.mObjectType);
               if (pProtoSquad && pProtoSquad->getFlagAvailable() && !pProtoSquad->getFlagForbid() && !pProtoSquad->getFlagOneTimeSpawnUsed())
               {
                  if (doTrain(mPlayerID, childData.mObjectType, 1, true, true, cInvalidTriggerScriptID, cInvalidTriggerVarID, true ))
                     pProtoSquad->setFlagOneTimeSpawnUsed(true);
               }
               continue;
            }

            if (childData.mChildType == BProtoObjectChildObject::cTypeParkingLot)
            {
               // Skip if parking lot already exists.
               if (pParkingLot)
                  continue;
            }
            else if (childData.mChildType == BProtoObjectChildObject::cTypeRally)
            {
               // Set base rally point.
               doRallyPoint = true;
               rallyPoint = childData.mPosition;
               continue;
            }
            else if (childData.mChildType == BProtoObjectChildObject::cTypeSocket)
            {
               // See if this socket already exists.
               bool found=false;
               for (uint j=0; j<startNumRefs; j++)
               {
//-- FIXING PREFIX BUG ID 4474
                  const BEntityRef* pSocketRef = getEntityRefByIndex(j);
//--
                  if (pSocketRef->mType== BEntityRef::cTypeAssociatedSocket)
                  {
                     BUnit* pSocket = gWorld->getUnit(pSocketRef->mID);
                     if (pSocket)
                     {
                        if (pSocket->getPosition().distance(childData.mPosition) < 1.0f)
                        {
                           found=true;
                           addAssociatedSocket(pSocket->getID());
                           pSocket->setAssociatedBase(mID);

                           // Also link the new base to the building plugged into this socket.
//-- FIXING PREFIX BUG ID 4473
                           const BEntityRef* pBuildingRef = pSocket->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug);
//--
                           if (pBuildingRef)
                           {
                              BUnit* pBuilding = gWorld->getUnit(pBuildingRef->mID);
                              if (pBuilding)
                              {
                                 addAssociatedBuilding(pBuilding->getID());
                                 pBuilding->setAssociatedBase(mID);
                              }
                           }
                           break;
                        }
                     }
                  }
               }
               if (found)
                  continue;
            }
            else if (childData.mChildType == BProtoObjectChildObject::cTypeFoundation)
            {
               // if we found a foundation, kill it off and remove it's reference
               // since it's about to be replaced
               for (uint j=0; j<startNumRefs; j++)
               {
                  const BEntityRef* pFoundationRef = getEntityRefByIndex(j);
                  if (pFoundationRef->mType== BEntityRef::cTypeAssociatedFoundation)
                  {
                     BUnit* pFoundation = gWorld->getUnit(pFoundationRef->mID);
                     if (pFoundation)
                     {
                        if (pFoundation->getPosition().distance(childData.mPosition) < 1.0f)
                        {
                           removeAssociatedFoundation(pFoundation->getID());
                           if (pSettlement)
                              pSettlement->removeAssociatedFoundation(pFoundation->getID());
                           pFoundation->kill(true);
                           break;
                        }
                     }
                  }
               }
            }
            else
            {
               // See if this object already exists.
               bool found=false;
               for (uint j=0; j<startNumRefs; j++)
               {
//-- FIXING PREFIX BUG ID 4475
                  const BEntityRef* pBuildingRef = getEntityRefByIndex(j);
//--
                  if (pBuildingRef->mType== BEntityRef::cTypeAssociatedBuilding)
                  {
                     BObject* pBuilding = gWorld->getObject(pBuildingRef->mID);
                     if (pBuilding && pBuilding->getProtoID() == childData.mObjectType)
                     {
                        if (pBuilding->getPosition().distance(childData.mPosition) < 1.0f)
                        {
                           found=true;
                           addAssociatedBuilding(pBuilding->getID());
                           pBuilding->setAssociatedBase(mID);
                           break;
                        }
                     }
                  }
               }
               if (found)
                  continue;
            }

            // Calculate object rotation.
            BVector forward, right;
            if (childData.mRotation == 0.0f)
            {
               forward = mForward;
               right = mRight;
            }
            else
            {
               rotMat.makeRotateY(childData.mRotation);
               rotMat.transformVector(mForward, forward);
               rotMat.transformVector(mRight, right);
            }

            // Create the child object, socket, or parking lot.
            BEntityID createdEntityID = gWorld->createEntity(childData.mObjectType, false, mPlayerID, childData.mPosition, forward, right, true, false, false, cInvalidObjectID, cInvalidPlayerID, mID);
            BSquad* pSquad = gWorld->getSquad(createdEntityID);
            if (pSquad)
            {
               BUnit* pObject = pSquad->getLeaderUnit();
               if (pObject)
               {
                  if (childData.mChildType == BProtoObjectChildObject::cTypeParkingLot)
                  {
                     parkingLot = pObject->getID();
                     setAssociatedParkingLot(parkingLot);
                     if (pSettlement)
                        pSettlement->setAssociatedParkingLot(parkingLot);
                  }
                  else if (childData.mChildType == BProtoObjectChildObject::cTypeSocket)
                  {
                     if (parkingLot != cInvalidObjectID)
                        pObject->setAssociatedParkingLot(parkingLot);
                     addAssociatedSocket(pObject->getID());
                     if (pSettlement)
                        pSettlement->addAssociatedSocket(pObject->getID());
                  }
                  else if (childData.mChildType == BProtoObjectChildObject::cTypeFoundation)
                  {
                     if (parkingLot != cInvalidObjectID)
                        pObject->setAssociatedParkingLot(parkingLot);
                     addAssociatedFoundation(pObject->getID());
                     if (pSettlement)
                        pSettlement->addAssociatedFoundation(pObject->getID());
                  }
                  else if (childData.mChildType == BProtoObjectChildObject::cTypeUnit)
                  {
                     addEntityRef(BEntityRef::cTypeAssociatedUnit, pObject->getID(), 0, 0);
                  }
                  else
                  {
                     if (parkingLot != cInvalidObjectID)
                        pObject->setAssociatedParkingLot(parkingLot);
                     addAssociatedBuilding(pObject->getID());
                     if (pSettlement)
                        pSettlement->addAssociatedBuilding(pObject->getID());
                  }
                  pObject->setAssociatedBase(mID);
                  if (settlement != cInvalidObjectID)
                     pObject->setAssociatedSettlement(settlement);

                  // copy over the exploration group if we have an exploration group and 
                  // the child is a socket 
                  if (getExplorationGroup() != -1 && childData.mChildType == BProtoObjectChildObject::cTypeSocket)
                     pObject->setExplorationGroup(getExplorationGroup());

                  if (pObject->getProtoObject()->getFlagNotSelectableWhenChildObject())
                  {
                     pObject->setFlagSelectable(false);
                  }

                  if (!childData.mAttachBoneName.isEmpty())
                  {
                     long boneHandle = getVisual()->getBoneHandle(childData.mAttachBoneName);
                     if (boneHandle != -1)
                        attachObject(pObject->getID(), boneHandle);
                  }

               }
            }
            else
            {
               BObject* pObject = gWorld->getObject(createdEntityID);
               if (pObject)
               {
                  if (childData.mChildType == BProtoObjectChildObject::cTypeObject)
                     addEntityRef(BEntityRef::cTypeAssociatedObject, pObject->getID(), 0, 0);
               }
            }
         }

         if (doRallyPoint && !getPlayer()->getFlagRallyPoint() && !haveRallyPoint(getPlayer()->getID()))
         {
            pParkingLot = gWorld->getUnit(getAssociatedParkingLot());
            if (pParkingLot)
               pParkingLot->setRallyPoint(rallyPoint, cInvalidObjectID, mPlayerID);
            else
               setRallyPoint(rallyPoint, cInvalidObjectID, mPlayerID);

            if (gWorld->getFlagCoop())
            {
               BTeamID unitTeamID = getPlayer()->getTeamID();
               BPlayerID coopPlayerID = cInvalidPlayerID;

               long numPlayers = gWorld->getNumberPlayers();

               for (long i=0; i<numPlayers; i++)
               {
                  BPlayer* pPlayer = gWorld->getPlayer(i);
                  if (!pPlayer)
                     continue;

                  if ((pPlayer->getID() != mPlayerID) && (pPlayer->getTeamID() == unitTeamID))
                  {
                     coopPlayerID = pPlayer->getID();
                     break;
                  }
               }

               if (coopPlayerID != cInvalidPlayerID)
               {
                  if (pParkingLot)
                     pParkingLot->setRallyPoint(rallyPoint, cInvalidObjectID, coopPlayerID);
                  else
                     setRallyPoint(rallyPoint, cInvalidObjectID, coopPlayerID);
               }
            }
         }
      }
   }

   // Update NoRender flag after child objects are created in case the flag needs to be carried forward to the child objects.
   setFlagNoRender(mFlagNoRender);
}

//==============================================================================
// BUnit::initGroupRangePointsForWeapon
//==============================================================================
void BUnit::initGroupRangePointsForWeapon()
{
   bool useGroupRange = false;
   const BProtoObject* pProtoObject = getProtoObject();
   const BTactic* pTactic = pProtoObject->getTactic();
   if (pTactic && pTactic->getNumberWeapons() > 0)
   {
      for (int i=0; i<pTactic->getNumberWeapons(); i++)
      {
         if (pTactic->getWeapon(i)->getFlagUseGroupRange())
         {
            useGroupRange = true;
            break;
         }
      }
   }
   if (!useGroupRange)
      return;

   bool bFail = false;
   int turretIndex=0;
   BVector corner[4];
//-- FIXING PREFIX BUG ID 4480
   const BEntityRef* pParentRef = getFirstEntityRefByType(BEntityRef::cTypeParentSocket);
//--
   if (pParentRef)
   {
//-- FIXING PREFIX BUG ID 4479
      const BUnit* pParentSocket = gWorld->getUnit(pParentRef->mID);
//--
      if (pParentSocket)
      {
         BEntityID settlement = pParentSocket->getAssociatedSettlement();
         if (settlement == cInvalidObjectID)
            bFail = true;

         BUnit* pSettlementUnit = gWorld->getUnit(settlement);
         if (pSettlementUnit)
         {
            uint numEntityRefs = pSettlementUnit->getNumberEntityRefs();
            for (uint i=0; i<numEntityRefs; i++)
            {
//-- FIXING PREFIX BUG ID 4478
               const BEntityRef* pRef = pSettlementUnit->getEntityRefByIndex(i);
//--
               if (pRef)
               {
//-- FIXING PREFIX BUG ID 4477
                  const BUnit* pRefUnit = gWorld->getUnit(pRef->mID);
//--
                  if (pRefUnit && pRefUnit->getProtoObject()->isType(gDatabase.getObjectType("TurretSocket")))
                  {
                     corner[turretIndex] = pRefUnit->getPosition();
                     turretIndex++;
                  }
               }
            }
         }
         else
            bFail = true;
      }
      else
         bFail = true;
   }
   else
      bFail = true;
   
   if (turretIndex != 4)
      bFail = true;

   if (!bFail)
   {
      // Figure out relationships between corner so we can fix winding
      BVector correctedWinding[4];

      correctedWinding[0] = corner[0];
      float dist = 0.0f;
      int oppositeVertIndex = 0;
      for (int i=1; i<4; i++)
      {
         float testDist = correctedWinding[0].distance(corner[i]);
         if (testDist > dist)
         {
            dist = testDist;
            oppositeVertIndex = i;
         }
      }

      BVector oppositeVert = corner[oppositeVertIndex];
      correctedWinding[2] = oppositeVert;

      int testVertIndex = 1;
      int remainingVertIndex = 3;
      if (testVertIndex == oppositeVertIndex)
         testVertIndex = 2;
      if (oppositeVertIndex == 3)
         remainingVertIndex = 2;

      BVector startVertToOpposite = oppositeVert - correctedWinding[0];
      BVector startVertToTestVert = corner[testVertIndex] - correctedWinding[0];
      BVector testCross = startVertToOpposite.cross(startVertToTestVert);
      if (testCross.y < 0.0f)
      {
         correctedWinding[1] = corner[testVertIndex];
         correctedWinding[3] = corner[remainingVertIndex];
      }
      else
      {
         correctedWinding[1] = corner[remainingVertIndex];
         correctedWinding[3] = corner[testVertIndex];
      }

      mpGroupHull = new BOPQuadHull();
      if(mpGroupHull && mpGroupHull->createSimpleHull(correctedWinding))
      {
         float diag1 = correctedWinding[0].distance(corner[2]);
         float diag2 = correctedWinding[1].distance(corner[3]);
         mGroupDiagSpan = Math::fSelectMax(diag1, diag2);
      }
   }
}


//==============================================================================
//==============================================================================
void BUnit::playAttachmentAnim(int attachmentHandle, int animType)
{
   if (mpVisual)
   {
      BVisualItem* pAttachment = mpVisual->getAttachment(attachmentHandle);
      if (pAttachment)
      {        
         pAttachment->setAnimationLock(cActionAnimationTrack, false);
         pAttachment->setAnimationLock(cMovementAnimationTrack, false);
         BMatrix worldMatrix; 
         getWorldMatrix(worldMatrix);

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

         mpVisual->setAnimation(cActionAnimationTrack, animType, false, playerColor, worldMatrix, 0.0f, -1, false, pAttachment);

         // AJL 11/12/08 - Changed back to using copyAnimationTrack since that's safer. Though now calling copyAnimationTrack on pAttachment
         // directly instead of calling it on the mpVisual and passing in the attachment to start on (the last parameter).
         pAttachment->copyAnimationTrack(cActionAnimationTrack, cMovementAnimationTrack, mpVisual->getProtoVisual(), mpVisual->getUserData(), false, playerColor, worldMatrix, NULL);

         pAttachment->setAnimationLock(cActionAnimationTrack, true);
         pAttachment->setAnimationLock(cMovementAnimationTrack, true);

         pAttachment->validateAnimationTracks();
         pAttachment->updateVisibility(getVisualIsVisible());
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::resetAttachmentAnim(int attachmentHandle)
{
   if (mpVisual)
   {
      BVisualItem* pAttachment = mpVisual->getAttachment(attachmentHandle);
      if (pAttachment)
      {        
         pAttachment->setAnimationLock(cActionAnimationTrack, false);
         pAttachment->setAnimationLock(cMovementAnimationTrack, false);

         BMatrix worldMatrix; getWorldMatrix(worldMatrix);
         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

         // Any clones?
         long clone = -1;
         for (long track = 0; track < cNumAnimationTracks; track++)
         {
            if (mpVisual->mAnimationTrack[track].mIsClone)
            {
               clone = track;
               break;
            }
         }

         // Yes. Set animation of master than copy it to the clone.
         if (clone != -1)
         {
            long master = 1 - clone;
            BVisualAnimationData &masterTrackData = mpVisual->mAnimationTrack[master];

            mpVisual->setAnimation(master, masterTrackData.mAnimType, false, playerColor, worldMatrix, masterTrackData.mPosition * masterTrackData.mDuration, masterTrackData.mAnimAsset.mIndex, false, pAttachment);
            mpVisual->copyAnimationTrack(master, clone, false, playerColor, worldMatrix, pAttachment);
         }
         // No. Set both animations.
         else
         {
            BVisualAnimationData &actionTrackData = mpVisual->mAnimationTrack[cActionAnimationTrack];
            BVisualAnimationData &movementTrackData = mpVisual->mAnimationTrack[cMovementAnimationTrack];
            mpVisual->setAnimation(cActionAnimationTrack, actionTrackData.mAnimType, false, playerColor, worldMatrix, actionTrackData.mPosition * actionTrackData.mDuration, actionTrackData.mAnimAsset.mIndex, false, pAttachment);
            mpVisual->setAnimation(cMovementAnimationTrack, movementTrackData.mAnimType, false, playerColor, worldMatrix, movementTrackData.mPosition * movementTrackData.mDuration, movementTrackData.mAnimAsset.mIndex, false, pAttachment);
         }

         pAttachment->validateAnimationTracks();
         pAttachment->updateVisibility(getVisualIsVisible());
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::playAttachmentAnimOnEvent(int attachmentHandle, int animType, int eventType, int data1, int data2, bool useData1, bool useData2)
{
   BUnitActionPlayAttachmentAnims* pAction = (BUnitActionPlayAttachmentAnims*)mActions.getActionByType(BAction::cActionTypeUnitPlayAttachmentAnims);
   if (!pAction)
   {
      pAction = (BUnitActionPlayAttachmentAnims*)gActionManager.createAction(BAction::cActionTypeUnitPlayAttachmentAnims);
      if (pAction)
         addAction(pAction);
   }
   if (pAction)
      pAction->addOnEventAction(BUnitActionPlayAttachmentAnims::cActionPlayAnim, attachmentHandle, animType, eventType, data1, data2, useData1, useData2);
}

//==============================================================================
//==============================================================================
void BUnit::resetAttachmentAnimOnEvent(int attachmentHandle, int animType, int eventType, int data1, int data2, bool useData1, bool useData2)
{
   BUnitActionPlayAttachmentAnims* pAction = (BUnitActionPlayAttachmentAnims*)mActions.getActionByType(BAction::cActionTypeUnitPlayAttachmentAnims);
   if (!pAction)
   {
      pAction = (BUnitActionPlayAttachmentAnims*)gActionManager.createAction(BAction::cActionTypeUnitPlayAttachmentAnims);
      if (pAction)
         addAction(pAction);
   }
   if (pAction)
      pAction->addOnEventAction(BUnitActionPlayAttachmentAnims::cActionResetAnim, attachmentHandle, animType, eventType, data1, data2, useData1, useData2);
}

//==============================================================================
//==============================================================================
void BUnit::updateAttackTimers(float elapsedTime)
{
   //-- Update attack timers
   for (uint i=0; i<mAttackWaitTimer.getSize(); i++)
   {
      if (mAttackWaitTimer[i].mTimer > 0.0f)
      {
         mAttackWaitTimer[i].mTimer -= elapsedTime;
         if (mAttackWaitTimer[i].mTimer <= 0.0f)
            mAttackWaitTimer[i].mTimer = 0.0f;

      }
      if (mAttackWaitTimer[i].mPreAttackTimer > 0.0f)
      {
         mAttackWaitTimer[i].mPreAttackTimer -= elapsedTime;
         if (mAttackWaitTimer[i].mPreAttackTimer <= 0.0f)
            mAttackWaitTimer[i].mPreAttackTimer = 0.0f;
      }
   }
}

//==============================================================================
//==============================================================================
const BUnitOpp* BUnit::getOppByTypeAndTarget(BUnitOppType type, const BSimTarget* target)
{   
   for(uint oppIdx=0; oppIdx < getNumberOpps(); oppIdx++)
   {
      const BUnitOpp* pOpp = getOppByIndex(oppIdx);
      if(!pOpp)
         continue;

      if(pOpp->getType() != type)
         continue;

      BEntity* pTarget = gWorld->getEntity(target->getID());
//-- FIXING PREFIX BUG ID 4481
      const BEntity* pOppTarget = gWorld->getEntity(pOpp->getTarget().getID());
//--
      if(pTarget && pOppTarget)
      {
         if(pTarget->isSameUnitOrSquad(pOppTarget))
         {                  
            return(pOpp);
         }            
      }
   }

   return NULL;
}

//==============================================================================
//==============================================================================
void BUnit::updateChildObjectDamageTakenScalar(bool death)
{                                              
   BEntityID baseID = getAssociatedBase();
   if (baseID != cInvalidObjectID)
   {
      BUnit* pBase = gWorld->getUnit(baseID);
      if (pBase)
      {
         float damageTakenScalar = pBase->getProtoObject()->getChildObjectDamageTakenScalar();
         if (damageTakenScalar > 0.0f)
         {
            // Get thew old count.
            BEntityRef* pBaseRef = pBase->getFirstEntityRefByType(BEntityRef::cTypeAssociatedBase);
            if (!pBaseRef)
               return;
            int oldCount = pBaseRef->mData1;

            // Calculate new count.
            int newCount = 0;
            uint numRefs = pBase->getNumberEntityRefs();
            for (uint i=0; i<numRefs; i++)
            {
//-- FIXING PREFIX BUG ID 4483
               const BEntityRef* pRef = pBase->getEntityRefByIndex(i);
//--
               if (pRef->mType == BEntityRef::cTypeAssociatedBuilding)
               {
//-- FIXING PREFIX BUG ID 4482
                  const BUnit* pChild = gWorld->getUnit(pRef->mID);
//--
                  if (pChild && pChild->isAlive() && pChild->getFlagBuilt() && pChild->getProtoObject()->getFlagChildForDamageTakenScalar())
                     newCount++;
               }
            }
            if (death)
               newCount--;

            if (newCount != oldCount)
            {
               // First remove old scalar.
               if (oldCount > 0)
               {
                  float oldScalar = 1.0f / ((oldCount + 1) * damageTakenScalar);
                  pBase->adjustDamageTakenScalar(1.0f / oldScalar);
               }

               // Now add new scalar.
               if (newCount > 0)
               {
                  float newScalar = 1.0f / ((newCount + 1) * damageTakenScalar);
                  pBase->adjustDamageTakenScalar(newScalar);
               }

               // Save off the new scalar.
               pBaseRef->mData1 = newCount;
            }
         }
      }
   }
}
//==============================================================================
//==============================================================================
void BUnit::playMeleeAttackSound(const BUnit* pTargetUnit, bool ramming, float velocity)
{
   //-- Play the impact sound for the killer clap!
   //-- We'll want to put this info on the action if we eventually need different sounds for different melee actions.
   const BProtoObject* pProto = getProtoObject();
   if(pProto && pTargetUnit)
   {
      BImpactSoundInfo soundInfo;      
      bool result = pProto->getImpactSoundCue(pTargetUnit->getProtoObject()->getSurfaceType(), soundInfo);      
      if(result)
      {
         if(soundInfo.mSoundCue != cInvalidCueIndex)
         {
            if( ramming )
            {
               //gConsoleOutput.debug("Ramming velocity: %f", velocity);
               BRTPCInitArray rtpc;
               rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::RAM_VELOCITY, velocity) );
               gWorld->getWorldSoundManager()->addSound(this, -1, soundInfo.mSoundCue, true, cInvalidCueIndex, soundInfo.mCheckSoundRadius, true, &rtpc);
            }
            else
            {
               gWorld->getWorldSoundManager()->addSound(this, -1, soundInfo.mSoundCue, true, cInvalidCueIndex, soundInfo.mCheckSoundRadius, true);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnit::isPhysicsAircraft() const
{
   const BProtoObject* pPO = getProtoObject();
   if (!pPO)
      return false;

   BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pPO->getPhysicsInfoID(), true);
   if (!pInfo)
      return false;

   if (pInfo->isAircraft() && (pPO->getMovementType() == cMovementTypeAir) && getPhysicsObject())
      return true;

   return false;
}

//==============================================================================
//==============================================================================
void BUnit::playShieldSound(BSoundType soundEnum)
{
   //BCueIndex cueIndex = gSoundManager.getCueIndexByEnum(soundEnum);      
   //gWorld->getWorldSoundManager()->addSound(this, -1, cueIndex, true, cInvalidCueIndex);
   BCueIndex index = getProtoObject()->getSound(soundEnum);
   if(index != cInvalidCueIndex)
      gWorld->getWorldSoundManager()->addSound(this, -1, index, true, cInvalidCueIndex, true, true);        
}

//==============================================================================
//==============================================================================
bool BUnit::isAttackable(const BEntity* attacker, bool allowFriendly)
{
   const BPlayer* pPlayer = attacker ? attacker->getPlayer() : NULL;
   bool isEnemy = pPlayer ? pPlayer->isEnemy(getPlayer()) : false;
   BObject* pAttackerObj = attacker ? const_cast<BEntity*>(attacker)->getObject() : NULL;
   return ((isEnemy || allowFriendly || getPlayer()->isGaia()) &&
           isAlive() &&
           !getFlagDestroy() &&
           !getFlagInvulnerable() &&
           !getFlagDown() &&
           !getFlagFatalityVictim() &&
           (!getFlagAttached() || !isAttachedToObject(pAttackerObj)) &&
           getParentSquad() &&
           (!isGarrisoned() || isInCover()));
}

//==============================================================================
//==============================================================================
void BUnit::prePositionChanged(BVector& hardpointPos, int& hardpointID)
{
   hardpointPos = cInvalidVector;
   hardpointID = -1;

   //-- Do we have a target, and does our current hardpoint need to not rotate when the unit rotates?
   //-- DJBFIXME: Probably need to modify this to work for secondary turrets as well.
   const BAction* pAction=mActions.getActionByType(BAction::cActionTypeUnitRangedAttack);
   if (pAction)
   {
      const BProtoAction* pProto = pAction->getProtoAction();
      if(pProto)
      {
         const BHardpoint* pHardpoint=getProtoObject()->getHardpoint(pProto->getHardpointID());
         if (pHardpoint && pHardpoint->getFlagInfiniteRateWhenHasTarget() == true)
         {
            const BSimTarget *pTarget=pAction->getTarget();
            if(pTarget && pTarget->getID() != cInvalidObjectID)
            {
               hardpointID = pProto->getHardpointID();
               getHardpointYawTargetLocation(hardpointID, hardpointPos);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::postPositionChanged(const BVector oldHardpointPos, const int hardpoinID)
{
   if(oldHardpointPos != cInvalidVector)
   {
      //-- Set the yaw point
      yawHardpointToWorldPos(hardpoinID, oldHardpointPos, 10000.0);               
      
      //-- Update the attachment transform
      BVisual* pVisual = getVisual();
      if(pVisual)
      {
         pVisual->updateAttachmentTransforms();
         pVisual->computeCombinedBoundingBox();
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnit::save(BStream* pStream, int saveType) const
{
   if (!BObject::save(pStream, saveType))
      return false;

   // mOpps;
   uint oppCount = mOpps.size();
   GFWRITEVAL(pStream, uint8, oppCount);
   GFVERIFYCOUNT(oppCount, 20);
   for (uint i=0; i<oppCount; i++)
      GFWRITEFREELISTITEMPTR(pStream, BUnitOpp, mOpps[i]);

   GFWRITEARRAY(pStream, uint, mVisualAmmo, uint8, 100);

   // mControllers
   GFWRITEVAL(pStream, uint8, BActionController::cNumberControllers);
   for (uint i=0; i<BActionController::cNumberControllers; i++)
      GFWRITEVAR(pStream, BActionController, mControllers[i]);

   GFWRITEVAR(pStream, BRallyPoint, mRallyPoint);
   GFWRITEVAR(pStream, BRallyPoint, mRallyPoint2);
   GFWRITEVECTOR(pStream, mGoalVector);
   GFWRITEVAR(pStream, BEntityID, mKilledByID);
   GFWRITEVAR(pStream, long, mKilledByWeaponType);
   GFWRITEVAR(pStream, BEntityID, mRallyObject);
   GFWRITEVAR(pStream, BEntityID, mRallyObject2);
   GFWRITEVAR(pStream, BEntityID, mCarriedObject);
   GFWRITEARRAY(pStream, BEntityID, mMultiTargets, uint8, 250);
   GFWRITEVAR(pStream, float, mMaxHPContained);
   GFWRITEVAR(pStream, float, mShieldpoints);
   GFWRITEVAR(pStream, float, mHitpoints);
   GFWRITEVAR(pStream, float, mResourceAmount);
   GFWRITEVAR(pStream, int, mGatherers);
   GFWRITEVAR(pStream, float, mDamageModifier);
   GFWRITEVAR(pStream, float, mVelocityScalar);
   GFWRITEVAR(pStream, float, mAccuracyScalar);
   GFWRITEVAR(pStream, float, mWorkRateScalar);
   GFWRITEVAR(pStream, float, mWeaponRangeScalar);
   GFWRITEVAR(pStream, float, mDamageTakenScalar);
   GFWRITEVAR(pStream, float, mDodgeScalar);

   // mDamageTracker
   GFWRITEVAL(pStream, bool, (mDamageTracker != NULL));
   if (mDamageTracker)
      GFWRITECLASSPTR(pStream, saveType, mDamageTracker);

   GFWRITEARRAY(pStream, BAttackTimer, mAttackWaitTimer, uint8, 100);
   GFWRITEVAR(pStream, float, mMaxTurnRate);
   GFWRITEVAR(pStream, float, mCurrentTurnRate);
   GFWRITEVAR(pStream, float, mAcceleration);
   GFWRITEVAR(pStream, long , mBattleID);
   GFWRITEVAR(pStream, float, mAmmunition);     
   GFWRITEVAR(pStream, float, mContainedPop);
   GFWRITEVAR(pStream, float, mCapturePoints);
   GFWRITEVAR(pStream, BPlayerID, mCapturePlayerID);
   //int mSelectionDecal;

   // mpHitZoneList
   GFWRITEVAL(pStream, bool, (mpHitZoneList != NULL));
   if (mpHitZoneList)
      GFWRITECLASSARRAY(pStream, saveType, *mpHitZoneList, uint8, 25);

   GFWRITEVAR(pStream, long, mAttackActionRefCount);
   GFWRITEVAR(pStream, DWORD, mLastMoveTime);
   GFWRITEVAR(pStream, DWORD, mLeashTimer);
   GFWRITEVAR(pStream, int, mTacticState);
   GFWRITEVAR(pStream, uint8, mInfectionPlayerID);
   GFWRITEVAR(pStream, BEntityID, mFormerParentSquad);
   //BOPQuadHull* mpGroupHull;
   GFWRITEVAR(pStream, float, mGroupDiagSpan);
   GFWRITEVAR(pStream, float, mGarrisonTime);
   GFWRITEVAR(pStream, float, mLastDPSRampValue);
   GFWRITEVAR(pStream, DWORD, mLastDPSUpdate);
   GFWRITEVAR(pStream, float, mShieldRegenRate);
   GFWRITEVAR(pStream, float, mShieldRegenDelay);
   GFWRITEVAR(pStream, short, mBaseNumber);

   GFWRITEBITBOOL(pStream, mFlagAlive);
   GFWRITEBITBOOL(pStream, mFlagDiesAtZeroHP);
   GFWRITEBITBOOL(pStream, mFlagDieAtZeroResources);
   GFWRITEBITBOOL(pStream, mFlagUnlimitedResources);
   GFWRITEBITBOOL(pStream, mFlagRallyPoint);
   GFWRITEBITBOOL(pStream, mFlagRallyPoint2);
   GFWRITEBITBOOL(pStream, mFlagHasHPBar);
   GFWRITEBITBOOL(pStream, mFlagDisplayHP);
   GFWRITEBITBOOL(pStream, mFlagForceDisplayHP);      
   GFWRITEBITBOOL(pStream, mFlagHasShield);
   GFWRITEBITBOOL(pStream, mFlagFullShield);
   GFWRITEBITBOOL(pStream, mFlagUsesAmmo);      
   GFWRITEBITBOOL(pStream, mFlagHasGarrisoned);
   GFWRITEBITBOOL(pStream, mFlagHasAttached);
   GFWRITEBITBOOL(pStream, mFlagAttackBlocked);
   GFWRITEBITBOOL(pStream, mFlagHasGoalVector);
   GFWRITEBITBOOL(pStream, mFlagHasFacingCommand);
   GFWRITEBITBOOL(pStream, mFlagIgnoreUserInput);
   GFWRITEBITBOOL(pStream, mFlagFirstBuilt);
   GFWRITEBITBOOL(pStream, mFlagInfected);
   GFWRITEBITBOOL(pStream, mFlagTakeInfectionForm);
   GFWRITEBITBOOL(pStream, mFlagFloodControl);
   GFWRITEBITBOOL(pStream, mFlagHasHitched);
   GFWRITEBITBOOL(pStream, mFlagReverseMove);
   GFWRITEBITBOOL(pStream, mFlagReverseMoveHasMoved);
   GFWRITEBITBOOL(pStream, mFlagBuildingConstructionPaused);
   GFWRITEBITBOOL(pStream, mFlagBuildOtherQueue);
   GFWRITEBITBOOL(pStream, mFlagBuildOtherQueue2);
   GFWRITEBITBOOL(pStream, mFlagPreserveDPS);
   GFWRITEBITBOOL(pStream, mFlagDoingFatality);
   GFWRITEBITBOOL(pStream, mFlagFatalityVictim);
   GFWRITEBITBOOL(pStream, mFlagDeathReplacementHealing);
   GFWRITEBITBOOL(pStream, mFlagForceUpdateContainedUnits);
   GFWRITEBITBOOL(pStream, mFlagRecycled);
   GFWRITEBITBOOL(pStream, mFlagDown);
   GFWRITEBITBOOL(pStream, mFlagNoCorpse);
   GFWRITEBITBOOL(pStream, mFlagNotAttackable);
   GFWRITEBITBOOL(pStream, mFlagSelectTypeTarget);
   GFWRITEBITBOOL(pStream, mFlagBlockContain);
   GFWRITEBITBOOL(pStream, mFlagIsTypeGarrison);
   GFWRITEBITBOOL(pStream, mFlagIsTypeCover);
   GFWRITEBITBOOL(pStream, mFlagParentSquadChangingMode);
   GFWRITEBITBOOL(pStream, mFlagShatterOnDeath);
   GFWRITEBITBOOL(pStream, mFlagAllowStickyCam);
   GFWRITEBITBOOL(pStream, mFlagBeingBoarded);
   GFWRITEBITBOOL(pStream, mFlagAddResourceEnabled);
   GFWRITEBITBOOL(pStream, mFlagIsHibernating);
   GFWRITEBITBOOL(pStream, mFlagUnhittable);
   GFWRITEBITBOOL(pStream, mFlagDestroyedByBaseDestruction);
   GFWRITEBITBOOL(pStream, mFlagKilledByLeaderPower);

   GFWRITEMARKER(pStream, cSaveMarkerUnit1);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnit::load(BStream* pStream, int saveType)
{
   if (!BObject::load(pStream, saveType))
      return false;

   // mOpps;
   uint oppCount;
   GFREADVAL(pStream, uint8, uint, oppCount);
   GFVERIFYCOUNT(oppCount, 20);
   for (uint i=0; i<oppCount; i++)
   {
      BUnitOpp* pOpp = NULL;
      GFREADFREELISTITEMPTR(pStream, BUnitOpp, pOpp);
      if (pOpp)
         mOpps.add(pOpp);
   }

   GFREADARRAY(pStream, uint, mVisualAmmo, uint8, 100);

   // mControllers
   uint controllerCount;
   GFREADVAL(pStream, uint8, uint, controllerCount);
   GFVERIFYCOUNT(controllerCount, 10);
   for (uint i=0; i<controllerCount; i++)
   {
      if (i < BActionController::cNumberControllers)
         GFREADVAR(pStream, BActionController, mControllers[i])
      else
         GFREADTEMPVAL(pStream, BActionController)
   }

   GFREADVAR(pStream, BRallyPoint, mRallyPoint);
   GFREADVAR(pStream, BRallyPoint, mRallyPoint2);
   GFREADVECTOR(pStream, mGoalVector);
   GFREADVAR(pStream, BEntityID, mKilledByID);
   GFREADVAR(pStream, long, mKilledByWeaponType);
   GFREADVAR(pStream, BEntityID, mRallyObject);
   GFREADVAR(pStream, BEntityID, mRallyObject2);
   GFREADVAR(pStream, BEntityID, mCarriedObject);
   GFREADARRAY(pStream, BEntityID, mMultiTargets, uint8, 250);
   GFREADVAR(pStream, float, mMaxHPContained);
   GFREADVAR(pStream, float, mShieldpoints);
   GFREADVAR(pStream, float, mHitpoints);
   GFREADVAR(pStream, float, mResourceAmount);
   GFREADVAR(pStream, int, mGatherers);
   GFREADVAR(pStream, float, mDamageModifier);
   GFREADVAR(pStream, float, mVelocityScalar);
   GFREADVAR(pStream, float, mAccuracyScalar);
   GFREADVAR(pStream, float, mWorkRateScalar);
   GFREADVAR(pStream, float, mWeaponRangeScalar);
   GFREADVAR(pStream, float, mDamageTakenScalar);
   GFREADVAR(pStream, float, mDodgeScalar);

   // mDamageTracker
   bool damageTracker;
   GFREADVAR(pStream, bool, damageTracker);
   if (damageTracker)
   {
      mDamageTracker = new BDamageTracker();
      GFREADCLASSPTR(pStream, saveType, mDamageTracker);
   }

   GFREADARRAY(pStream, BAttackTimer, mAttackWaitTimer, uint8, 100);
   GFREADVAR(pStream, float, mMaxTurnRate);
   GFREADVAR(pStream, float, mCurrentTurnRate);
   GFREADVAR(pStream, float, mAcceleration);
   GFREADVAR(pStream, long , mBattleID);
   GFREADVAR(pStream, float, mAmmunition);     
   GFREADVAR(pStream, float, mContainedPop);
   GFREADVAR(pStream, float, mCapturePoints);
   GFREADVAR(pStream, BPlayerID, mCapturePlayerID);
   //int mSelectionDecal;

   // mpHitZoneList
   bool hitZoneList;
   GFREADVAR(pStream, bool, hitZoneList);
   if (hitZoneList)
   {
      mpHitZoneList = new BHitZoneArray();
      GFREADCLASSARRAY(pStream, saveType, *mpHitZoneList, uint8, 25);
   }

   GFREADVAR(pStream, long, mAttackActionRefCount);
   GFREADVAR(pStream, DWORD, mLastMoveTime);
   GFREADVAR(pStream, DWORD, mLeashTimer);
   GFREADVAR(pStream, int, mTacticState);
   GFREADVAR(pStream, uint8, mInfectionPlayerID);
   GFREADVAR(pStream, BEntityID, mFormerParentSquad);
   //BOPQuadHull* mpGroupHull;
   GFREADVAR(pStream, float, mGroupDiagSpan);
   GFREADVAR(pStream, float, mGarrisonTime);
   GFREADVAR(pStream, float, mLastDPSRampValue);
   GFREADVAR(pStream, DWORD, mLastDPSUpdate);
   GFREADVAR(pStream, float, mShieldRegenRate);
   GFREADVAR(pStream, float, mShieldRegenDelay);
   GFREADVAR(pStream, short, mBaseNumber);

   GFREADBITBOOL(pStream, mFlagAlive);
   GFREADBITBOOL(pStream, mFlagDiesAtZeroHP);
   GFREADBITBOOL(pStream, mFlagDieAtZeroResources);
   GFREADBITBOOL(pStream, mFlagUnlimitedResources);
   GFREADBITBOOL(pStream, mFlagRallyPoint);
   GFREADBITBOOL(pStream, mFlagRallyPoint2);
   GFREADBITBOOL(pStream, mFlagHasHPBar);
   GFREADBITBOOL(pStream, mFlagDisplayHP);
   GFREADBITBOOL(pStream, mFlagForceDisplayHP);      
   GFREADBITBOOL(pStream, mFlagHasShield);
   GFREADBITBOOL(pStream, mFlagFullShield);
   GFREADBITBOOL(pStream, mFlagUsesAmmo);      
   GFREADBITBOOL(pStream, mFlagHasGarrisoned);
   GFREADBITBOOL(pStream, mFlagHasAttached);
   GFREADBITBOOL(pStream, mFlagAttackBlocked);
   GFREADBITBOOL(pStream, mFlagHasGoalVector);
   GFREADBITBOOL(pStream, mFlagHasFacingCommand);
   GFREADBITBOOL(pStream, mFlagIgnoreUserInput);
   GFREADBITBOOL(pStream, mFlagFirstBuilt);
   GFREADBITBOOL(pStream, mFlagInfected);
   GFREADBITBOOL(pStream, mFlagTakeInfectionForm);
   GFREADBITBOOL(pStream, mFlagFloodControl);
   GFREADBITBOOL(pStream, mFlagHasHitched);
   GFREADBITBOOL(pStream, mFlagReverseMove);
   GFREADBITBOOL(pStream, mFlagReverseMoveHasMoved);
   GFREADBITBOOL(pStream, mFlagBuildingConstructionPaused);
   GFREADBITBOOL(pStream, mFlagBuildOtherQueue);
   GFREADBITBOOL(pStream, mFlagBuildOtherQueue2);
   GFREADBITBOOL(pStream, mFlagPreserveDPS);
   GFREADBITBOOL(pStream, mFlagDoingFatality);
   GFREADBITBOOL(pStream, mFlagFatalityVictim);
   GFREADBITBOOL(pStream, mFlagDeathReplacementHealing);
   GFREADBITBOOL(pStream, mFlagForceUpdateContainedUnits);
   GFREADBITBOOL(pStream, mFlagRecycled);
   GFREADBITBOOL(pStream, mFlagDown);
   GFREADBITBOOL(pStream, mFlagNoCorpse);
   GFREADBITBOOL(pStream, mFlagNotAttackable);
   GFREADBITBOOL(pStream, mFlagSelectTypeTarget);
   GFREADBITBOOL(pStream, mFlagBlockContain);
   GFREADBITBOOL(pStream, mFlagIsTypeGarrison);
   GFREADBITBOOL(pStream, mFlagIsTypeCover);
   GFREADBITBOOL(pStream, mFlagParentSquadChangingMode);
   GFREADBITBOOL(pStream, mFlagShatterOnDeath);
   GFREADBITBOOL(pStream, mFlagAllowStickyCam);

   if(mGameFileVersion >= 2)
      GFREADBITBOOL(pStream, mFlagBeingBoarded);

   if(mGameFileVersion >= 3)
      GFREADBITBOOL(pStream, mFlagAddResourceEnabled);

   if(mGameFileVersion >= 4)
      GFREADBITBOOL(pStream, mFlagIsHibernating);

   if (mGameFileVersion >= 5)
      GFREADBITBOOL(pStream, mFlagUnhittable);

   if (mGameFileVersion >= 6)
      GFREADBITBOOL(pStream, mFlagDestroyedByBaseDestruction);

   if (mGameFileVersion >= 7)
      GFREADBITBOOL(pStream, mFlagKilledByLeaderPower);

   setupVehiclePhysics();

   GFREADMARKER(pStream, cSaveMarkerUnit1);

   if (BWorld::mGameFileVersion < 7)
   {
      // ajl 9/15/08 - Dead units were not handled properly in old save games, so destroy on load
      if (!mFlagAlive && !mFlagDestroy)
      {
         mFlagNoRender = true;
         destroy();
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnit::postLoad(int saveType)
{
   if (!BObject::postLoad(saveType))
      return false;

   BPlayer* pPlayer = getPlayer();
   if (!pPlayer)
      return false;

   // make sure we update our parent squad pointer
   refreshParentSquad();

   const BProtoObject* pProtoObject = getProtoObject();
   if (!pProtoObject)
      return false;

   // mpHitZoneList
   BHitZoneArray* pHitZoneList = ((BProtoObject*)pProtoObject)->getHitZoneList();
   if (pHitZoneList->getNumber() > 0)
   {
      BHitZoneArray oldHitZoneList;
      if (mpHitZoneList)
         oldHitZoneList = *mpHitZoneList;
      else
         mpHitZoneList = new BHitZoneArray();
      *mpHitZoneList = *pHitZoneList;
      if (mpVisual)
      {
         long hitZoneCount = mpHitZoneList->getNumber();
         for (long i = 0; i < hitZoneCount; i++)
         {
            BHitZone& zone = (*mpHitZoneList)[i];
            zone.setAttachmentHandle(mpVisual->getAttachmentHandle(zone.getAttachmentName()));
         }
      }
      uint oldCount = oldHitZoneList.size();
      uint newCount = mpHitZoneList->size();
      for (uint i=0; i<oldCount; i++)
      {
         BHitZone& oldZone = oldHitZoneList[i];
         for (uint j=0; j<newCount; j++)
         {
            BHitZone& newZone = mpHitZoneList->get(j);
            if (newZone.getAttachmentName() == oldZone.getAttachmentName())
            {
               newZone.setFlags(oldZone.getFlags());
               newZone.setHitpoints(oldZone.getHitpoints());
               newZone.setShieldpoints(oldZone.getShieldpoints());
               break;
            }
         }
      }
   }

   /*
   if (pProtoObject->getFlagBuildingCommands() || pProtoObject->getFlagBuild() || pProtoObject->getFlagUseBuildingAction())
      addAction(gActionManager.createAction(BAction::cActionTypeUnitBuilding));

   if (getFlagUsesAmmo())
      addAction(gActionManager.createAction(BAction::cActionTypeUnitAmmoRegen));
   */

   pPlayer->addUnitToProtoObject(this, mProtoID);

   if (!(pProtoObject->getFlagBuild() && !mFlagBuilt))
   {
      //createPersistentActions();

      if (mFlagBlockLOS)
      {
         long radius = getSimLOS();
         BTeamID teamID = pPlayer->getTeamID();
         for (long i=1; i<gWorld->getNumberTeams(); i++)
         {
            if (i != teamID)
               gVisibleMap.blockCircularRegion(mSimX, mSimZ, radius, i);
         }
      }

      initGroupRangePointsForWeapon();
   }

   if (pProtoObject->getFlagTrackPlacement())
      BPlacementRules::getUnitsToTrack().add(mID);

   if (pProtoObject->getSelectType() == cSelectTypeUnit && pProtoObject->getFlagUIDecal())
      mSelectionDecal = gDecalManager.createDecal();

   if (isAlive() && mProtoID != gDatabase.getPOIDPhysicsThrownObject())
   {
      doBuildingTerrainFlatten();
      doBuildingTerrainAlpha();
   }

   if (mpPhysicsObject && mpPhysicsObject->getLoadedKeyframed())
      mpPhysicsObject->setKeyframed(true);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnit::canBeThrown() const
{
   if(getFlagAlive() == false)
      return false;

   // check the various modes where you can't be thrown
   BSquad* pParentSquad = getParentSquad();
   BSquadAI* pSquadAI = (pParentSquad) ? pParentSquad->getSquadAI() : NULL;
   long mode = (pSquadAI) ? pSquadAI->getMode() : NULL;
   if (mode == BSquadAI::cModeLockdown || mode == BSquadAI::cModePower || mode == BSquadAI::cModeCover)
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnit::throwUnit(BEntityID attackerID, int32 attackerProtoActionID, float throwVelocityScalar)
{
   if (!canBeThrown())
      return false;

   BSimTarget simTarget;
   simTarget.setID(attackerID);

   BUnitActionThrown* pAction = reinterpret_cast<BUnitActionThrown*>(getActionByType(BAction::cActionTypeUnitThrown));
   // Add new impulse to action
   if (pAction)
   {
      pAction->setThrowerID(simTarget.getID());
      pAction->addImpulse();
      if(attackerProtoActionID != -1)
         pAction->setThrowerProtoActionID(attackerProtoActionID);         
      pAction->setThrowVelocityScalar(throwVelocityScalar);
   }
   // Otherwise, create opp to make action
   else
   {
      BUnitOpp* pNewOpp=BUnitOpp::getInstance();
      pNewOpp->init();
      //BSimTarget simTarget(pUnit->getID());
      pNewOpp->setTarget(simTarget);
      pNewOpp->setSource(this->getID());
      pNewOpp->setType(BUnitOpp::cTypeThrown);
      pNewOpp->setPriority(BUnitOpp::cPriorityDeath);
      pNewOpp->generateID();
      if(attackerProtoActionID != -1)
         pNewOpp->setUserData(attackerProtoActionID);
      if (!addOpp(pNewOpp))
         BUnitOpp::releaseInstance(pNewOpp);
      else
      {
         // Remove any evade or retreat opps from this unit
         const BUnitOpp* pEvadeOpp = getOppByTypeAndTarget(BUnitOpp::cTypeEvade, &simTarget);
         if (pEvadeOpp)
            removeOpp(pEvadeOpp->getID(), true);
         const BUnitOpp* pRetreatOpp = getOppByTypeAndTarget(BUnitOpp::cTypeRetreat, &simTarget);
         if (pRetreatOpp )
            removeOpp(pRetreatOpp->getID(), true);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnit::revealPosition(float time) const
{
   long numTeams = gWorld->getNumberTeams();
   for (long i = 1; i < numTeams; i++)
   {
      if (isVisible(i))
      {
         gWorld->createRevealer(i, getPosition(), Math::Max(getObstructionRadius() * 2.0f, 1.0f), (DWORD)(time * 1000.0f), 0.0f);
      }
   }
}

//==============================================================================
//==============================================================================
float BUnit::getDPSRampValue() const
{
   if (gWorld->getGametime() - mLastDPSUpdate > 1000)
      return 0.0f;
   else
      return mLastDPSRampValue;
}

//==============================================================================
//==============================================================================
void BUnit::setDPSRampValue(float val)
{
   mLastDPSRampValue = val;
   mLastDPSUpdate = gWorld->getGametime();
}

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void dbgUnitInternal(BEntityID squadID, const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   long lSpecificSquad = -1;

   gConfig.get(cConfigDebugSpecificSquad, &lSpecificSquad);
   bool bMatch = false;

   if (lSpecificSquad == -1 || ((lSpecificSquad != -1) && (lSpecificSquad == squadID.asLong())))
      bMatch = true;

   if (!bMatch)
      return;

   // Output.
   gConsole.output(cChannelSim, "SQUAD %d (%d): %s", squadID.asLong(), squadID.getIndex(), buf);
}


//==============================================================================
//==============================================================================
void dbgUnitInternalTempID(const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   // Call with preset ID.
   dbgUnitInternal(sDebugUnitTempID, buf);
}
#endif


//==============================================================================
//==============================================================================
void BUnit::doBuildingTerrainFlatten()
{
   const BProtoObject* pProto = getProtoObject();
   if (pProto->getFlagFlattenTerrain())
   {
      // Flatten the ground around the building
      //get our XZ in relative position of the map 
      BTerrainFlattenRegion flattenRegion0 = pProto->getFlattenRegion(0);
      BTerrainFlattenRegion flattenRegion1 = pProto->getFlattenRegion(1);

      float expandDist = 8.0f;

      BVector terrainMax = gWorld->getTerrainMax();

      //CLM [09.26.07]
      //Choose the AVERAGE point that our footprint occupies. Otherwise we may be clipping into the terrain.
      //Scan the area we're influencing, find the AVERAGE point. (Ala AGE3)
      float xPercMin = (mPosition.x + flattenRegion0.mMinX - expandDist)/ terrainMax.x; //divide our interception location by the total size 
      float xPercMax = (mPosition.x + flattenRegion0.mMaxX + expandDist)/ terrainMax.x;
      float zPercMin = (mPosition.z + flattenRegion0.mMinZ - expandDist)/ terrainMax.z;
      float zPercMax = (mPosition.z + flattenRegion0.mMaxZ + expandDist)/ terrainMax.z;

      int minX = (int)floor(gTerrainSimRep.getNumXHeightVerts() * xPercMin);
      int maxX = (int)ceil(gTerrainSimRep.getNumXHeightVerts() * xPercMax);
      int minZ = (int)floor(gTerrainSimRep.getNumXHeightVerts() * zPercMin);
      int maxZ = (int)ceil(gTerrainSimRep.getNumXHeightVerts() * zPercMax);

      BVector vm = mPosition;
      float sum =0;
      int count =0;
      for(int x=minX;x<maxX;x++)
      {
         for(int z=minZ;z<maxZ;z++)
         {
            float height =0;
            gTerrainSimRep.getHeight(x,z,height,true);
            sum += height;
            count++;
         }
      }
      vm.y = sum / count;
#ifdef SYNC_Unit
      syncUnitData("BUnit::doMapChangesOnBuilt", vm);
#endif
      setPosition(vm);
      float desHeight = mPosition.y; //in world space

      if (flattenRegion0.mMinX != 0.0f || flattenRegion0.mMaxX != 0.0f) // We have a non-zeroed region0
      {
         float xPercMin = (mPosition.x + flattenRegion0.mMinX - expandDist)/ terrainMax.x; //divide our interception location by the total size 
         float xPercMax = (mPosition.x + flattenRegion0.mMaxX + expandDist)/ terrainMax.x;
         float zPercMin = (mPosition.z + flattenRegion0.mMinZ - expandDist)/ terrainMax.z;
         float zPercMax = (mPosition.z + flattenRegion0.mMaxZ + expandDist)/ terrainMax.z;
         gTerrainSimRep.flattenInstant(xPercMin, xPercMax, zPercMin, zPercMax, desHeight); 
      }
      if (flattenRegion1.mMinX != 0.0f || flattenRegion1.mMaxX != 0.0f) // We have a non-zeroed region1
      {
         float xPercMin = (mPosition.x + flattenRegion1.mMinX - expandDist)/ terrainMax.x; //divide our interception location by the total size 
         float xPercMax = (mPosition.x + flattenRegion1.mMaxX + expandDist)/ terrainMax.x;
         float zPercMin = (mPosition.z + flattenRegion1.mMinZ - expandDist)/ terrainMax.z;
         float zPercMax = (mPosition.z + flattenRegion1.mMaxZ + expandDist)/ terrainMax.z;
         gTerrainSimRep.flattenInstant(xPercMin, xPercMax, zPercMin, zPercMax, desHeight);
      }
      // If no flatten region is defined, just use the obstruction definition
      if (flattenRegion0.mMinX == 0.0f && flattenRegion0.mMaxX == 0.0f && flattenRegion1.mMinX == 0.0f && flattenRegion1.mMaxX == 0.0f)
      {
         float xPercMin = (mPosition.x - mObstructionRadiusX - expandDist)/ terrainMax.x; //divide our interception location by the total size 
         float xPercMax = (mPosition.x + mObstructionRadiusX + expandDist)/ terrainMax.x;
         float zPercMin = (mPosition.z - mObstructionRadiusZ - expandDist)/ terrainMax.z;
         float zPercMax = (mPosition.z + mObstructionRadiusZ + expandDist)/ terrainMax.z;
         gTerrainSimRep.flattenInstant(xPercMin, xPercMax, zPercMin, zPercMax, desHeight); 
      }
      // Flatten any parking lot too... 
      if (pProto->getParkingMinX() != 0.0f || pProto->getParkingMaxX() != 0.0f)
      {
         float xPercMin = (mPosition.x + pProto->getParkingMinX() - expandDist)/ terrainMax.x; //divide our interception location by the total size 
         float xPercMax = (mPosition.x + pProto->getParkingMaxX() + expandDist)/ terrainMax.x;
         float zPercMin = (mPosition.z + pProto->getParkingMinZ() - expandDist)/ terrainMax.z;
         float zPercMax = (mPosition.z + pProto->getParkingMaxZ() + expandDist)/ terrainMax.z;
         gTerrainSimRep.flattenInstant(xPercMin, xPercMax, zPercMin, zPercMax, desHeight);
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::doBuildingTerrainAlpha()
{
   if (mpVisual)
   {
      BProtoVisual* pProtoVisual=mpVisual->getProtoVisual();
      if (pProtoVisual)
      {
         BProtoVisualTagPtrArray tags;
         pProtoVisual->getBuildingConstructionTags(tags,cAnimEventAlphaTerrain, cAnimTypeIdle);
         if (tags.getSize() > 0)
         {
            for (uint i=0; i<tags.getSize(); i++)
            {
               gWorld->handleAnimEvent(-1, -1, cAnimEventAlphaTerrain, mID, tags[i], mpVisual);
            }
         }
         tags.clear();
         pProtoVisual->getBuildingConstructionTags(tags,cAnimEventBuildingDecal, cAnimTypeIdle);
         if (tags.getSize() > 0)
         {
            for (uint i=0; i<tags.getSize(); i++)
            {
               gWorld->handleAnimEvent(-1, -1, cAnimEventBuildingDecal, mID, tags[i], mpVisual);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnit::undoBuildingTerrainAlpha()
{
   if (mpVisual)
   {
      BProtoVisual* pProtoVisual=mpVisual->getProtoVisual();
      if (pProtoVisual)
      {
         BProtoVisualTagPtrArray tags;
         pProtoVisual->getBuildingConstructionTags(tags,cAnimEventAlphaTerrain, cAnimTypeDeath);
         if (tags.getSize() > 0)
         {
            for (uint i=0; i<tags.getSize(); i++)
            {
               gWorld->handleAnimEvent(-1, -1, cAnimEventAlphaTerrain, mID, tags[i], mpVisual);
            }
         }
         tags.clear();
         pProtoVisual->getBuildingConstructionTags(tags,cAnimEventBuildingDecal, cAnimTypeDeath);
         if (tags.getSize() > 0)
         {
            for (uint i=0; i<tags.getSize(); i++)
            {
               gWorld->handleAnimEvent(-1, -1, cAnimEventBuildingDecal, mID, tags[i], mpVisual);
            }
         }
      }
   }
}

//==============================================================================
// BUnit::doDeathExplode
//==============================================================================
void BUnit::doDeathExplode(BEntityID killerID)
{
   BEntity *pKiller = gWorld->getEntity(killerID);
   BPlayer *pPlayer = NULL;
   if (pKiller != NULL)
      pPlayer = pKiller->getPlayer();

   BProtoObjectID explodeProto = gCollectiblesManager.getDeathExplodeProtoObject();
   BProtoObject* pProjProtoObject = (pPlayer ? pPlayer->getProtoObject(explodeProto) : gDatabase.getGenericProtoObject(explodeProto));
   if (!pProjProtoObject)
      return;
   BTactic* pProjTactic = pProjProtoObject->getTactic();
   if (!pProjTactic)
      return;
   BProtoAction* pProtoAction = pProjTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false);
   if (!pProtoAction)
      return;

   // Create the projectile
   BObjectCreateParms parms;
   if (pKiller != NULL)
      parms.mPlayerID = pKiller->getPlayerID();
   else
      parms.mPlayerID = 0;
   parms.mProtoObjectID = pProtoAction->getProjectileID();
   BVector pos = getPosition();
   pos.y = pos.y + 2.0f;
   parms.mPosition = pos;

   BVector targetLoc = getPosition();
   float dps = pProtoAction->getDamagePerSecond();
   gWorld->launchProjectile(parms, targetLoc, XMVectorZero(), XMVectorZero(), dps, pProtoAction, pProtoAction, cInvalidObjectID, NULL, -1, true);
}

//==============================================================================
bool BUnit::handleDelayKillBase()
{
   // VAT: 11/06/08 delay the kills of these units for better perf when destroying the bases
   if (getFlagDestroyedByBaseDestruction())
      return false;

   if (!isMainBase())
      return false;

   BDynamicSimArray<BUnit*> delayedBuildingKills;
   BDynamicSimArray<BUnit*> delayedTurretKills;
   uint numEntityRefs = getNumberEntityRefs();

   for (uint i=0; i<numEntityRefs; i++)
   {
      const BEntityRef* pRef = getEntityRefByIndex(i);
      if (!pRef || pRef->mID == mID)
         continue;

      bool isBuilding = (pRef->mType == BEntityRef::cTypeAssociatedBuilding);
      bool isSocket = (pRef->mType == BEntityRef::cTypeAssociatedSocket || pRef->mType == BEntityRef::cTypeSocketPlug);
      if (!isBuilding && !isSocket)
         continue;

      BUnit* pAssocItem = gWorld->getUnit(pRef->mID);
      if (!pAssocItem)
         continue;

      // all sockets and buildings should be marked as non selectable
      pAssocItem->setFlagSelectable(false);

      if (!isBuilding)
         continue;

      pAssocItem->setKilledByID(getKilledByID());
      pAssocItem->setFlagDestroyedByBaseDestruction(true);
      pAssocItem->setFlagNotAttackable(true);
      pAssocItem->setFlagAttackBlocked(true);
      pAssocItem->setFlagInvulnerable(true);
      pAssocItem->removeActions(false);
      BSquad* pSquad = pAssocItem->getParentSquad();
      if (pSquad)
         pSquad->setFlagHasHPBar(false);

      if (pAssocItem->isType(gDatabase.getOTIDTurretBuilding()))
         delayedTurretKills.add(pAssocItem);
      else
         delayedBuildingKills.add(pAssocItem);

      // MS 12/1/2008: PHX-18790
      if(pAssocItem->getParentSquad())
         pAssocItem->getParentSquad()->determineKillerForDelayedBaseDestruction();
   }

   // actually do the delayed kills
   static const float cMinDelay = 0.80;
   static const float cMaxDelay = 1.60;
   // start with a .5 sec delay so the revealer can complete
   float currentBuildingDelay = 0.5f;
   float currentTurretDelay = 0.5f;
   bool delayMainBase = false;

   while (!delayedBuildingKills.isEmpty())
   {
      uint index = getRandRange(cSimRand, 0, delayedBuildingKills.getSize() - 1);
      delayedBuildingKills[index]->setLifespan(currentBuildingDelay * 1000);
      delayedBuildingKills.removeIndex(index);
      delayMainBase = true;
      if (!delayedBuildingKills.isEmpty())
         currentBuildingDelay += getRandRangeFloat(cSimRand, cMinDelay, cMaxDelay);
   }
   while (!delayedTurretKills.isEmpty())
   {
      uint index = getRandRange(cSimRand, 0, delayedTurretKills.getSize() - 1);
      delayedTurretKills[index]->setLifespan(currentTurretDelay * 1000);
      delayedTurretKills.removeIndex(index);
      delayMainBase = true;
      if (!delayedTurretKills.isEmpty())
         currentTurretDelay += getRandRangeFloat(cSimRand, cMinDelay, cMaxDelay);
   }

   // we need to set our hitpoints to something positive, so we don't 
   // preemptively play the damage effects
   setHitpoints(1.0f);
   setFlagDestroyedByBaseDestruction(true);
   setFlagSelectable(false);
   setFlagNotAttackable(true);
   setFlagAttackBlocked(true);
   setFlagInvulnerable(true);
   removeActions(false);
   BSquad* pSquad = getParentSquad();
   if (pSquad)
   {
      pSquad->setFlagHasHPBar(false);

      // MS 12/1/2008: PHX-18790, set up "killed by" stuff here
      setKilledByID(getKilledByID());
      pSquad->determineKillerForDelayedBaseDestruction();
   }

   float mainBaseDelay = 0.0f;
   if (delayMainBase)
   {
      mainBaseDelay = Math::Max(currentBuildingDelay, currentTurretDelay);
      mainBaseDelay += getRandRangeFloat(cSimRand, cMinDelay, cMaxDelay);
   }
   setLifespan(mainBaseDelay * 1000);

   // add in a revealer that starts at the obstruction radius and 
   // expands to 1.5 the obstruction radius' size
   DWORD revealTime = (DWORD)((mainBaseDelay + 2.5f) * 1000);
   float revealRadiusMultiplier = 1.5f;
   long numTeams = gWorld->getNumberTeams();
   for (long i = 1; i < numTeams; i++)
   {
      if (isVisible(i))
      {
         BObject* pRevealer = gWorld->createRevealer(i, getPosition(),  Math::Max(getObstructionRadius() * revealRadiusMultiplier, 1.0f), revealTime);
         if (pRevealer)
            pRevealer->setRevealOverTimePercent(1.0f / revealRadiusMultiplier);
      }
   }

   // we need to start the base rebuild timer now
   const BEntityRef* pSettlementRef = getFirstEntityRefByType(BEntityRef::cTypeAssociatedSettlement);
   if (pSettlementRef)
   {
      BUnit* pSettlement = gWorld->getUnit(pSettlementRef->mID);
      if (pSettlement)
      {
         if (gDatabase.getBaseRebuildTimer() > 0.0f && getFlagBuilt())
         {
            BUnitActionBuilding* pAction = (BUnitActionBuilding*)pSettlement->getActionByType(BAction::cActionTypeUnitBuilding);
            if (pAction)
               pAction->setRebuildTimer(gDatabase.getBaseRebuildTimer());
         }
      }
   }

   return true;
}

//==============================================================================
void BUnit::doDeathSpawnSquad()
{
   BPlayerID spawnPlayer = getPlayerID();

   const BProtoObject* pProtoObject = getProtoObject();
   BProtoSquadID deathSpawnSquad = pProtoObject->getDeathSpawnSquad();

   // [5/8/2008 xemu] ok, if we are spawning a squad whose units need to belong to gaia, then make the squad belong to gaia.
   //-- FIXING PREFIX BUG ID 4962
   const BProtoSquad* pProtoSquad = NULL;
   //--
   BPlayer *pPlayer = gWorld->getPlayer(getPlayerID());
   pProtoSquad = pPlayer->getProtoSquad(deathSpawnSquad);
   BASSERT(pProtoSquad);
   const BDynamicSimArray<BProtoSquadUnitNode> &squadUnitNodes = pProtoSquad->getUnitNodes();
   int i;
   for (i=0; i < squadUnitNodes.getNumber(); i++)
   {
      BProtoObject *pProtoObject = pPlayer->getProtoObject(squadUnitNodes[i].mUnitType);
      BASSERT(pProtoObject);
      if (pProtoObject->getFlagForceToGaiaPlayer())
      {
         spawnPlayer = cGaiaPlayer;
      }
   }

   bool spawn = true;
   if (pProtoObject->getFlagCheckPos())
   {
      if (gPather.isObstructedTile(getPosition()))
      {
         spawn = false;
      }
   }

   if (pProtoObject->getMaxPopCount() > 0)
   {
      BEntityIDArray ids;
      int count = static_cast<int> (pPlayer->getSquadsOfType(deathSpawnSquad, ids)) ;
      if (count >= pProtoObject->getMaxPopCount())
         spawn = false;
   }

   if (spawn)
   {
      BEntityID deathSpawnSquadID = gWorld->createEntity(deathSpawnSquad, true, spawnPlayer, getPosition(), getForward(), getRight(), true);
      BSquad* pDeathSpawnSquad = gWorld->getSquad(deathSpawnSquadID);
      if (pDeathSpawnSquad)
      {
         // Check to see if death spawn squad is a hero.  If so go ahead and kill it to put it in hero death mode.
         const BProtoObject* pDeathSpawnSquadProtoObject = pDeathSpawnSquad->getProtoObject();
         if (pDeathSpawnSquadProtoObject && pDeathSpawnSquadProtoObject->isType(gDatabase.getOTIDHeroDeath()))
         {
            pDeathSpawnSquad->kill(false);
         }
      }
   }
}
//==============================================================================
