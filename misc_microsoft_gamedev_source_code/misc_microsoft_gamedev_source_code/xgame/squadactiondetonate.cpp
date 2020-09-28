//==============================================================================
// squadactiondetonate.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionDetonate.h"
#include "squadactionmove.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "tactic.h"
#include "visual.h"
#include "protosquad.h"
#include "worldsoundmanager.h"
#include "wwise_ids.h"
#include "unitactioncollisionattack.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionDetonate, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionDetonate::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
   {
      return (false);
   }

   BASSERT(mpOwner);
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   pSquad->setFlagAttackBlocked(true);

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionDetonate::disconnect()
{
   BASSERT(mpOwner);

   stopGlowy();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionDetonate::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;
   mbGlowy = false;
   mDetonateOppID = cInvalidActionID;
   mMoveID = cInvalidActionID;

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionDetonate::setState(BActionState state)
{
   BASSERT(mpOwner);
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (!pSquad || !mpProtoAction)
   {
      disconnect();
      return true;
   }

   BSquad *pTarget = gWorld->getSquad(mAttackTarget.getID());

   // No target, it must have been destroyed, so stop.
   if (!pTarget && state != cStateDone)
   {
      setState(cStateDone);
      return true;
   }

   switch (state)
   {
      case cStateMoving:
         {
            // Move squad near target
            mAttackTarget.setRange(mpProtoAction->getWorkRange() / 2.0f);
            BActionID moveID = pSquad->doMove(mpOrder, this, &mAttackTarget, true, false, true);

            if (moveID == cInvalidActionID)
            {
               setState(cStateFailed);
               return (true);
            }
            mMoveID = moveID;
         }
         break;

      case cStateAttacking:
         {
            pSquad->setPosition(pTarget->getPosition());
            pSquad->setLeashPosition(pTarget->getPosition());

            // Set in hit-and-run mode to enable collision damage
            pSquad->getSquadAI()->setMode(BSquadAI::cModeHitAndRun);

            int count = pSquad->getNumberChildren();
            for (int i = 0; i < count; ++i)
            {
               BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));

               if (!pUnit)
                  continue;
            
               // We don't need this running
               BUnitActionCollisionAttack* pUACA = reinterpret_cast<BUnitActionCollisionAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitCollisionAttack));
               if (pUACA)
               {
                  pUACA->setState(cStateDone);
               }

               // we also need to clear the leash timer, otherwise the units will stand around
               // waiting to go to their squad leash position
               pUnit->resetLeashTimer();
            }
         }
         break;

      case cStateDone:
      case cStateFailed:
         pSquad->removeActionByID(mMoveID);
         mMoveID = cInvalidActionID;
         removeChildActions();
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionDetonate::update(float elapsed)
{
   BASSERT(mpOwner);

   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (!pSquad || !mpProtoAction)
   {
      disconnect();
      return true;
   }

   // If target has been destroyed, shut us down
   BSquad *pTarget = gWorld->getSquad(mAttackTarget.getID());

   if (!pTarget)
   {
      setState(cStateDone);
      return true;
   }

   switch (mState)
   {
      case cStateNone:
         setState(cStateMoving);
         break;

      case cStateMoving:
         {
            // If we're close enough, go into "glowy mode" and attach grenades to both hands
            if (inGlowyRange() && !mbGlowy)
            {
               startGlowy();

               setState(cStateWait);
            }
         }
         break;

      case cStateWait:
         {
            // Make sure target is still close.  If not, repath
            if (mbGlowy && !inGlowyRange())
            {
               pSquad->stop();
               setState(cStateDone);
               break;
            }

            float dist = pSquad->getPosition().distanceSqr(pTarget->getPosition());
            float maxDist = pTarget->getObstructionRadius() + mpProtoAction->getWorkRange() / 2.0f;
            if (dist <= maxDist * maxDist)
               setState(cStateAttacking);
         }
         break;

      case cStateAttacking:
         {
            // Make sure target is still close.  If not, repath
            if (!inGlowyRange())
            {
               pSquad->stop();
               setState(cStateDone);
               break;
            }

            // Check to see if any units are inside target
            int count = pSquad->getNumberChildren();
            if (!mAttackTarget.isIDValid())
            {
               setState(cStateFailed);
               break;
            }
            BEntity *pEntity = gWorld->getEntity(mAttackTarget.getID());
            if (!pEntity)
            {
               setState(cStateFailed);
               break;
            }

            BOPQuadHull opHull;
            pEntity->getObstructionHull(opHull);

            for (int i = 0; i < count; ++i)
            {
               BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));

               if (pUnit)
               {
                  BVector unitPosition;
                  if (pUnit->getPhysicsObject())
                     unitPosition=pUnit->getPhysicsPosition();
                  else
                     unitPosition=pUnit->getPosition();
                  if (opHull.inside(unitPosition))
                  {
                     BUnitOpp* pNewOpp = BUnitOpp::getInstance();
                     BASSERT(pNewOpp);
                     createDetonateOpp(false, *pNewOpp);

                     if (!pUnit->addOpp(pNewOpp))
                        BUnitOpp::releaseInstance(pNewOpp);
                  }
                  /*
                  const BEntityIDArray& ignoreUnits=pSquad->getChildList();
                  static BEntityIDArray collisions;

                  //Get our velocity.
                  BVector unitVelocity;
                  BVector unitForward;
                  BVector unitPosition;
                  if (pUnit->getPhysicsObject())
                  {
                     unitVelocity=pUnit->getPhysicsVelocity();
                     unitForward=pUnit->getPhysicsForward();
                     unitPosition=pUnit->getPhysicsPosition();
                  }
                  else
                  {
                     unitVelocity=pUnit->getForwardVelocity();
                     unitForward=pUnit->getForward();
                     unitPosition=pUnit->getPosition();
                  }

                  BVector newPosition;
                  if (pUnit->getPhysicsObject())
                     newPosition = unitPosition + unitVelocity;
                  else
                  {
                     float moveDistance=unitVelocity.length()*1.0f;
                     newPosition = unitPosition + unitForward*moveDistance;
                  }

                  pUnit->checkForCollisions(ignoreUnits, unitPosition, newPosition, 0, false, collisions, NULL, false, true, false, true);

                  // If inside target (or really close), explode
                  if (collisions.size() > 0)
                  {
                     uint count = collisions.size();
                     for (uint j = 0; j < count; ++j)
                     {
                        if (pTarget->containsChild(collisions[j]))
                        {
                           BUnitOpp* pNewOpp = BUnitOpp::getInstance();
                           BASSERT(pNewOpp);
                           createDetonateOpp(false, *pNewOpp);

                           if (!pUnit->addOpp(pNewOpp))
                              BUnitOpp::releaseInstance(pNewOpp);
                        }
                     }
                  }
                  */
               }
            }
         }
         break;
   }

   #if !defined (BUILD_FINAL)
      if (gConfig.isDefined(cConfigRenderSimDebug))
      {      
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
            if (pUnit)
               pUnit->debugRender();
         }

         pSquad->debugRender();

         BPlatoon* pPlatoon = pSquad->getParentPlatoon();
         if (pPlatoon)
            pPlatoon->debugRender();
      }
   #endif

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionDetonate::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   if (eventType == BEntity::cEventActionDone)
   {
      if (mState == cStateMoving || mState == cStateWait)
      {
         // We're there, blow up
         removeChildActions();
         setState(cStateAttacking);
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionDetonate::setTarget(BSimTarget target)
{
   // If target is a base shield, target the base instead
   BSquad *pTargetSquad = gWorld->getSquad(target.getID());
   if (pTargetSquad && pTargetSquad->getLeaderUnit())
   {
      BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();
      if (pTargetUnit->isType(gDatabase.getOTIDBaseShield()))
      {
         BEntityID base = pTargetUnit->getBaseBuilding();
         if (base != cInvalidObjectID)
         {
            BUnit* pBase = gWorld->getUnit(base);
            if (pBase && pBase->getParentSquad())
            {
               target.setID(pBase->getParentSquad()->getID());
            }
         }
      }
   }

   mAttackTarget = target;
}

//==============================================================================
// Remove cached child action
//==============================================================================
void BSquadActionDetonate::removeChildActions()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (!pSquad)
      return;

   if (mDetonateOppID != cInvalidActionID)
   {
      pSquad->removeOppFromChildren(mDetonateOppID);

      mDetonateOppID = cInvalidActionID;
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionDetonate::inGlowyRange()
{
//-- FIXING PREFIX BUG ID 4886
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--

   if (pSquad && mpProtoAction)
   {
      float distance = mpProtoAction->getDodgeChanceMax() + 1.0f;
      BVector pos = pSquad->getAveragePosition(); // Have to use this since the squad is moved when going into hit and run mode

      BEntity* pTarget = gWorld->getEntity(mAttackTarget.getID());
      if (pTarget)
      {
         distance = pos.xzDistance(pTarget->getPosition());
         distance -= pTarget->getObstructionRadius();
      }
      else
         distance = pos.xzDistance(mAttackTarget.getPosition());

      distance -= pSquad->getObstructionRadius();

      if (distance <= mpProtoAction->getDodgeChanceMax())
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BSquadActionDetonate::createDetonateOpp(bool onDeath, BUnitOpp& opp)
{
   BSimTarget actualTarget;
//-- FIXING PREFIX BUG ID 4888
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BASSERT(pSquad);

   if (!pSquad)
   {
      disconnect();
      return;
   }

   //Pass down a squad target.
   if (mAttackTarget.getID().isValid() && (mAttackTarget.getID().getType() == BEntity::cClassTypeUnit))
   {
//-- FIXING PREFIX BUG ID 4880
      const BUnit* pTargetUnit=gWorld->getUnit(mAttackTarget.getID());
//--
      if (!pTargetUnit || !pTargetUnit->getParentSquad())
      {
         setState(cStateFailed);
         return;
      }
      actualTarget.setID(pTargetUnit->getParentSquad()->getID());
   }
   else
      actualTarget.setID(mAttackTarget.getID());
   if (mAttackTarget.isAbilityIDValid())
      actualTarget.setAbilityID(mAttackTarget.getAbilityID());
   // Some squads attack locations so pass through the position and range if it is valid
   if (mAttackTarget.isPositionValid())
      actualTarget.setPosition(mAttackTarget.getPosition());
   if (mAttackTarget.isRangeValid())
      actualTarget.setRange(mAttackTarget.getRange());

   // Setup detonate opportunity
   opp.init();

   opp.setTarget(actualTarget);
   opp.setType(BUnitOpp::cTypeDetonate);
   opp.setSource(pSquad->getID());
   opp.setPriority(BUnitOpp::cPriorityCommand);
   opp.generateID();

   mDetonateOppID = opp.getID();

   if (onDeath)
      opp.setUserData(1);
   // Set for immediate detonate
   else
      opp.setUserData(0);
}

//==============================================================================
//==============================================================================
void BSquadActionDetonate::startGlowy()
{
   BASSERT(mpOwner);

   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (!pSquad || !mpProtoAction)
   {
      disconnect();
      return;
   }

   uint count = pSquad->getNumberChildren();
   for (uint ofs = 0; ofs < count; ++ofs)
   {
      BUnit *pUnit = gWorld->getUnit(pSquad->getChild(ofs));
      BASSERT(pUnit);

      for (int i = 0; i < 2; ++i)
      {
         long boneHandle;

         if (i == 0)
            boneHandle = pUnit->getVisual()->getBoneHandle("Bip01 R Forearm");
         else
            boneHandle = pUnit->getVisual()->getBoneHandle("Bip01 L Forearm");

         BEntityID id;

         BASSERT(boneHandle != -1);
         
         id = pUnit->addAttachment(mpProtoAction->getProjectileID(), boneHandle);

         mGrenades.add(id);
      }

   }

   // Play scream sound
   BCueIndex cueIndex = cInvalidCueIndex;
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if(pProtoSquad)
      cueIndex = pProtoSquad->getSound(cSquadSoundKamikaze);

   if(cueIndex != cInvalidCueIndex)
   {
      BRTPCInitArray rtpc;
      rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
      gWorld->getWorldSoundManager()->addSound(pSquad, -1, cueIndex, false, cInvalidCueIndex, false, true, &rtpc);
   }

   // Set speed boost on all units in squad
   count = pSquad->getNumberChildren();
   for (uint i = 0; i < count; ++i)
   {
      BEntityID id = pSquad->getChild(i);
      BUnit* pUnit = gWorld->getUnit(id);

      if (pUnit)
      {
         pUnit->setVelocityScalar(mpProtoAction->getVelocityScalar());

         // Move us into the "suicidal" tactical state
         if (pUnit->getNumberTacticStates() > 0)
         {
            pUnit->setTacticState(0);
            pUnit->computeAnimation();
         }
      }
   }

   // [8-21-08 CJS] Cause the move command to re-evaluate squad speed
   pSquad->pauseMovement_4(1);

   mbGlowy = true;
}

//==============================================================================
//==============================================================================
void BSquadActionDetonate::stopGlowy()
{
   removeChildActions();

   // Destroy any attached grenades
   long grenadeCount = mGrenades.getNumber();
   for (long i = 0; i < grenadeCount; i++)
   {
      BEntity *pEntity = gWorld->getEntity(mGrenades[i]);
      if (pEntity)
         pEntity->kill(true);
   }

   mGrenades.clear();

   // Unset speed boost on all units in squad
   BASSERT(mpOwner);
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (!pSquad)
      return;

   //Remove the child action.
   pSquad->removeActionByID(mMoveID);
   mMoveID = cInvalidActionID;

   int count = pSquad->getNumberChildren();
   for (int i = 0; i < count; ++i)
   {
      BEntityID id = pSquad->getChild(i);
      BUnit* pUnit = gWorld->getUnit(id);

      if (pUnit)
      {
         pUnit->setVelocityScalar(1.0f);
         pUnit->clearTacticState();
         pUnit->computeAnimation();
         pUnit->setFlagNoCorpse(false);

         // Reset idle anim
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, cAnimTypeIdle, true, false);
      }
   }

   mbGlowy = false;

   pSquad->setFlagAttackBlocked(false);

   pSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);

   pSquad->setPosition(pSquad->getAveragePosition());
   pSquad->setLeashPosition(pSquad->getPosition());

   count = pSquad->getNumberChildren();
   for (int i = 0; i < count; ++i)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));

      if (!pUnit)
         continue;
   
      BUnitActionCollisionAttack* pUACA = reinterpret_cast<BUnitActionCollisionAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitCollisionAttack));
      if (pUACA)
      {
         pUACA->setState(cStateDone);
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionDetonate::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mAttackTarget);
   GFWRITEVAR(pStream, BUnitOppID, mDetonateOppID);
   GFWRITEARRAY(pStream, BEntityID, mGrenades, uint8, 40);
   GFWRITEBITBOOL(pStream, mbGlowy);
   GFWRITEVAR(pStream, BActionID, mMoveID);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionDetonate::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mAttackTarget);
   GFREADVAR(pStream, BUnitOppID, mDetonateOppID);
   GFREADARRAY(pStream, BEntityID, mGrenades, uint8, 40);
   GFREADBITBOOL(pStream, mbGlowy);
   GFREADVAR(pStream, BActionID, mMoveID);
   return true;
}
