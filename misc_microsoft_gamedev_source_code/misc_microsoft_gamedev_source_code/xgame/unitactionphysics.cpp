//==============================================================================
// unitactionphysics.cpp
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "config.h"
#include "configsgame.h"
#include "econfigenum.h"
#include "database.h"
#include "physics.h"
#include "physicsinfomanager.h"
#include "physicsinfo.h"
#include "protoobject.h"
#include "unit.h"
#include "unitactionphysics.h"
#include "visual.h"
#include "xgranny.h"
#include "world.h"
#include "soundmanager.h"
#include "worldsoundmanager.h"
#include "usermanager.h"
#include "user.h" 
#include "camera.h"
#include "corpsemanager.h"
#include "configsgame.h"
#include "unitactionrangedattack.h"
#include "prepost.h"
#include "ai.h"
#include "terraineffectmanager.h"
#include "terraineffect.h"

//#include "Physics/collide/query/collector/bodypaircollector/hkpallcdbodypaircollector.h"
#include <Physics/Collide/Query/Collector/BodyPairCollector/hkpAllCdBodyPairCollector.h>
#include <Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h>

//==============================================================================
//Constants
//==============================================================================
//DJBFIXME: Data drive this after E3
static const DWORD cMaxRocketTime = 5000;
static const DWORD cMinRocketTime = 750;
static const DWORD cSecondaryMinRocketTime = 1750;
static const DWORD cSecondaryMaxRocketTime = 1750;

static const float cChanceOfSecondaryRocket = 0.3f;
static const float cRocketUpForce = 250.0f;
static const float cInitialRocketUpForceMin = 1200.0f;
static const float cInitialRocketUpForceMax = 3500.0f;
static const float cRocketForce = 3000.0f;
static const float cSpinForce = 800.0f;
static const float cFlyTowardCameraForce = 2000.0f;
static const float cDefaultRocketAngularDamping = 10.0f;



IMPLEMENT_FREELIST(BClamshellCollisionListener, 4, &gSimHeap);

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionPhysics, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionPhysics::connect(BEntity* pOwner, BSimOrder* pOrder)
{   
   if (!BAction::connect(pOwner, pOrder))
      return(false);   

   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionPhysics::disconnect()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);

   if (mpClamshellCollisionListener)
   {
      BASSERT(pUnit);
      BASSERT(pUnit && pUnit->getPhysicsObject());
      BASSERT(pUnit && pUnit->getPhysicsObject() && pUnit->getPhysicsObject()->getRigidBody());
      if (pUnit && pUnit->getPhysicsObject() && pUnit->getPhysicsObject()->getRigidBody())
      {
         pUnit->getPhysicsObject()->removeHavokCollisionListener(mpClamshellCollisionListener);
      }
      BClamshellCollisionListener::releaseInstance(mpClamshellCollisionListener);
      mpClamshellCollisionListener = NULL;
   }

   if (pUnit)
   {
      BPhysicsObject* pPhysicsObject = pUnit->getPhysicsObject();
      if (pPhysicsObject)
      {
         pPhysicsObject->removeGameCollisionListener(this);
         mFlagCollisionListener = false;
      }
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionPhysics::init()
{
   if (!BAction::init())
      return(false);

   mPhysicsInfoID = -1;
   mpClamshellCollisionListener = NULL;
   mImpactSoundSet = -1;

   mRocketRotation = 0.0f;
   mRocketTime = 0;
   mStartRocketTime = 0;
   mTerrainEffect = -1;

   mGotoWaitTimer = 0.0f;

   mFlagFirstUpdate=true;
   mFlagRocketOnDeath=false;
   mFlagRocketExplode=false;
   mFlagRocketExplode=false;
   mFlagCompleteOnInactivePhysics = false;
   mFlagCompleteOnFirstCollision = false;
   mFlagEndMoveOnInactivePhysics = false;

   //Capps test
   mLastPosition=cInvalidVector;
   mLastPositionValid=false;

   mImpactPosition.zero();
   mImpactSurfaceType = 0;
   mFlagImpactSound = false;
   mFlagTerrainEffect = false;
   mFlagHadCollision = false;
   mFlagRemoveCollisionListener = false;
   mFlagCollisionListener = false;

   mFlagDynamicCollisionFilter = false;
   mParentEntityID = cInvalidObjectID;
   mCollisionFilterInfo = -1;

   mFlagEarlyTerminate = false;
   mFlagAllowEarlyTerminate = false;

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionPhysics::setState(BActionState state)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (state)
   {
      case cStateDone:
      {
         break;
      }

      case cStateWait:
      {
         if (!pUnit->physicsCompleted())
            setState(cStateFading);
         break;
      }

      case cStateFading:
      {
         pUnit->setFlagFadeOnDeath(true);
         pUnit->setLifespan(0, true);
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionPhysics::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // SLB: Disable motion extraction if we're thrown
   if (mFlagCompleteOnInactivePhysics)
      pUnit->setFlagSkipMotionExtraction(true);

   switch (mState)
   {
      case cStateWait:
         break;

      case cStateFading:
      {
         if (pUnit->getFlagIsFading() && pUnit->isFullyFaded())
            setState(cStateDone);
         break;
      }

      default:
      {
         // ajl 8/25/08 - hack to get around save game load problem where clamshell physics object won't go inactive for some reason
         if (mGotoWaitTimer > 0.0f)
         {
            mGotoWaitTimer -= elapsed;
            if (mGotoWaitTimer <= 0.0f)
            {
               mGotoWaitTimer = 0.0f;
               setState(cStateWait);
               break;
            }
         }

         BPhysicsObject* pPhysicsObject = pUnit->getPhysicsObject();
         BASSERT(pPhysicsObject);

         if (mFlagFirstUpdate && (pPhysicsObject->getType() == BPhysicsObject::cClamshell))
         {
            if (pUnit->getAnimationType(0) == cAnimTypeFlail)
            {
               mpClamshellCollisionListener = BClamshellCollisionListener::getInstance();
               pPhysicsObject->addHavokCollisionListener(mpClamshellCollisionListener);

               #ifdef SYNC_UnitAction
                  syncUnitActionCode("BUnitActionPhysics::update addCollisionListener");
               #endif
            }
         }

         // Handle clamshell collision
         if (mpClamshellCollisionListener && mpClamshellCollisionListener->hasCollided() && !mpClamshellCollisionListener->getCollisionProcessed())
         {
            if (pUnit)
               pUnit->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeClamshell, false);
            mpClamshellCollisionListener->setCollisionProcessed(true);
         }

         if (pPhysicsObject->isActive())
         {
            SCOPEDSAMPLE(BEntityUpdatePhysics);

            #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionPhysics::update getPosition 1", pUnit->getPosition());
            #endif

            // get rid of our collision listener - we only support callbacks on first collision right now
            if (mFlagRemoveCollisionListener)
            {
               mFlagRemoveCollisionListener = false;
               pPhysicsObject->removeGameCollisionListener(this);
               mFlagCollisionListener = false;
            }

            // If we have an impact sound to play, then play it
            if (mFlagImpactSound)
            {
               mFlagImpactSound = false;

               const BImpactSoundInfo* pImpactSoundInfo = gDatabase.getImpactSound(mImpactSoundSet, mImpactSurfaceType);
               if (pImpactSoundInfo)
                  gWorld->getWorldSoundManager()->addSound(mImpactPosition, pImpactSoundInfo->mSoundCue, true, cInvalidCueIndex, pImpactSoundInfo->mCheckSoundRadius, true);
            }

            // ditto for terrain effect
            if (mFlagTerrainEffect)
            {
               mFlagTerrainEffect = false;

               BTerrainEffect* pTE = gTerrainEffectManager.getTerrainEffect(mTerrainEffect, true);
               if(pTE)
                  pTE->instantiateEffect(pUnit->getPosition(), pUnit->getForward(), true);
            }


            // Changed the collision filter to the new filter when the shape of this units does not penetrate the
            // parent shape, or when the parent is dead.  This is used to resolve issues with penetration when 
            // a part of a building is spawned within the building itself.
            if(mFlagDynamicCollisionFilter)
            {
               bool bMakeCollidable = false;

               const BEntity* mParentEntity = gWorld->getEntity(mParentEntityID);

               if(mParentEntity)
               {
                  const BPhysicsObject* pParentPhysicsObject = mParentEntity->getPhysicsObject();

                  if(pParentPhysicsObject)
                  {
                     BASSERT(pParentPhysicsObject->getRigidBody());

                     hkpCollidable *pPartCollidable = pPhysicsObject->getRigidBody()->getCollidableRw();
                     hkpCollidable *pBuildingCollidable = pParentPhysicsObject->getRigidBody()->getCollidableRw();

                     hkpCollisionInput input = *gWorld->getPhysicsWorld()->getHavokWorld()->getCollisionInput();
                     hkpCollisionDispatcher* dispatcher = input.m_dispatcher;
                     hkpAllCdBodyPairCollector collector;

                     BASSERT(pPartCollidable->getShape());
                     BASSERT(pBuildingCollidable->getShape());

                     hkpCollisionDispatcher::GetPenetrationsFunc getPenetrationsFunc = dispatcher->getGetPenetrationsFunc(pPartCollidable->getShape()->getType(), pBuildingCollidable->getShape()->getType());

                     (*getPenetrationsFunc)( *pPartCollidable, *pBuildingCollidable, input, collector );
                     if ( collector.getEarlyOut() )
                     {
	                     break;
                     }

                     int numHits = collector.getHits().getSize();

                     if(numHits == 0)
                     {
                        bMakeCollidable = true;
                     }
                  }
                  else
                  {
                     bMakeCollidable = true;
                  }
               }
               else
               {
                  bMakeCollidable = true;
               }


               if(bMakeCollidable)
               {
                  // make collidable now
                  pPhysicsObject->setCollisionFilterInfo(mCollisionFilterInfo);
                  pPhysicsObject->updateCollisionFilter();
                  mFlagDynamicCollisionFilter = false;
               }
            }

            BVector centerOffset = pPhysicsObject->getCenterOffset();

            BVector physicsPos;
            pPhysicsObject->getPosition(physicsPos);

            BPhysicsMatrix physicsRot;
            pPhysicsObject->getRotation(physicsRot);
            
            //-- Store the old movement data
            BPrePost prePostInfo(*pUnit);
            prePostInfo.prePositionChanged();

            // Update position
            BVector worldOffset;
            physicsRot.transformVector(centerOffset, worldOffset);
            BVector position = physicsPos - worldOffset;
            #ifdef SYNC_Unit
               syncUnitData("BUnitActionPhysics 1", position);
            #endif
            BVector oldPos = pUnit->getPosition();
            pUnit->setPosition(position, false);

            // Sync walk/turn anim rate to motion
            const BProtoObject* pPO = pUnit->getProtoObject();
            if (pPO && pPO->getFlagSyncAnimRateToPhysics())
               setAnimRate(elapsed, oldPos, pUnit->getForward(), physicsRot.getForward());

            #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionPhysics::update centerOffset 1", centerOffset);
               syncUnitActionData("BUnitActionPhysics::update physicsPos 1", physicsPos);
               syncUnitActionData("BUnitActionPhysics::update physicsRot.getForward 1", physicsRot.getForward());
               syncUnitActionData("BUnitActionPhysics::update setPosition 1", position);
            #endif

            // Update rotation                        
            pUnit->setForward(physicsRot.getForward());
            pUnit->setUp(physicsRot.getUp());
            pUnit->setRight(physicsRot.getRight());

            //-- notify that the movement is complete
            prePostInfo.postPositionChanged();

            // Update velocity
            BVector velocity;
            pPhysicsObject->getLinearVelocity(velocity);
            pUnit->setVelocity(velocity);

            pUnit->setFlagMoved(true);

            // Clamshell animation processing
            BVisual* pVis = pUnit->getVisual();
            if (pVis && (pPhysicsObject->getType() == BPhysicsObject::cClamshell) &&
               (pVis->getAnimationType(0) == cAnimTypeClamshell))
            {
//-- FIXING PREFIX BUG ID 1544
               const BClamshellPhysicsObject* pCPO = static_cast<BClamshellPhysicsObject*>(pPhysicsObject);
//--

               BVector pos = pUnit->getPosition();
               BVector fwd, up, right;
               float angle = 0.0f;
               pCPO->getClamshellData(pos, fwd, up, right, angle);
               #ifdef SYNC_Unit
                  syncUnitData("BUnitActionPhysics 2", pos);
               #endif
               pUnit->setPosition(pos, false);
               pUnit->setForward(fwd);
               pUnit->setUp(up);
               pUnit->setRight(right);

               #ifdef SYNC_UnitAction
                  syncUnitActionData("BUnitActionPhysics::update position 2", pos)
               #endif

               // Set the animation position
               static float cAnimZeroPos = 0.20f;
               float animPos = Math::Clamp(angle + cAnimZeroPos, 0.0f, 1.0f);
               float animDuration = pVis->getAnimationDuration(cActionAnimationTrack);
               animPos *= animDuration;

               if (pVis->mpInstance && pVis->mModelAsset.mType==cVisualAssetGrannyModel &&
                  ((pVis->mAnimationTrack[cActionAnimationTrack].mAnimAsset.mType==cVisualAssetGrannyAnim) || (pVis->mAnimationTrack[cMovementAnimationTrack].mAnimAsset.mType==cVisualAssetGrannyAnim)))
               {
                  BGrannyInstance* pGI = static_cast<BGrannyInstance*>(pVis->mpInstance);
                  if (gEnableSubUpdating)
                  {
                     pGI->setClockOverride(animPos, animDuration, pVis->mAnimationTrack[cActionAnimationTrack].mAnimAsset.mIndex, 
                        pVis->mAnimationTrack[cMovementAnimationTrack].mAnimAsset.mIndex, elapsed);
                  }
                  granny_model_control_binding* pBinding=GrannyModelControlsBegin(pGI->getModelInstance());
                  while (pBinding != GrannyModelControlsEnd(pGI->getModelInstance()))
                  {
                     granny_control* control=GrannyGetControlFromBinding(pBinding);
                     if (control)
                     {
                        void **userData = GrannyGetControlUserDataArray(control);
                        if(userData != NULL)
                        {
                           long userDataAnimIndex = (long)userData[GrannyControlUserData_AnimIndex];
                           long userDataTrackIndex = (long)userData[GrannyControlUserData_AnimationTrack];
                           if (((userDataTrackIndex == cActionAnimationTrack) && (userDataAnimIndex == pVis->mAnimationTrack[cActionAnimationTrack].mAnimAsset.mIndex)) ||
                              ((userDataTrackIndex == cMovementAnimationTrack) && (userDataAnimIndex == pVis->mAnimationTrack[cMovementAnimationTrack].mAnimAsset.mIndex)))
                           {
                              GrannySetControlSpeed(control, 0.0f);
                              GrannyFreeControlOnceUnused(control);
                              GrannySetControlLoopCount(control, 1);
                              GrannyEaseControlIn(control, 0.0f, false);
                              float localClock = GrannyGetControlRawLocalClock(control);
                              float newLocalClock = localClock + Math::Clamp((animPos - localClock), elapsed * -4.0f, elapsed * 4.0f); // clamp change to 4x regular anim speed
                              newLocalClock = Math::Clamp(newLocalClock, 0.0f, animDuration);
                              GrannySetControlRawLocalClock(control, newLocalClock);
                           }
                        }
                     }

                     // Get next control
                     pBinding = GrannyModelControlsNext(pBinding);
                  }                 
               }
            }

            if(mFlagRocketOnDeath)
            {
               if(mFlagFirstUpdate)
               {
                  //-- Set Angular Dampening
                  pPhysicsObject->setAngularDamping(cDefaultRocketAngularDamping);            
               }

               if(gWorld->getGametime() - mStartRocketTime < mRocketTime)            
               {

                  //-- Apply rocket force
                  BVector rocketForce = pUnit->getUp();
                  rocketForce.scale(cRocketForce * elapsed);
                  pPhysicsObject->applyImpulse(rocketForce);

                  //-- Apply a small upward force
                  if(mFlagFirstUpdate)
                  {
                     BVector upwardForce = BVector(0,1,0);
                     float launchForceScale = getRandRangeFloat(cSimRand, cInitialRocketUpForceMin, cInitialRocketUpForceMax);
                     upwardForce.scale(launchForceScale);
                     pPhysicsObject->applyImpulse(upwardForce);
                  }
                  else
                  {
                     BVector upwardForce = BVector(0,1,0);
                     upwardForce.scale(cRocketUpForce * elapsed);
                     pPhysicsObject->applyImpulse(upwardForce);
                  }

                  //-- I know this is the correct way to do this, but for now I like the variation of the incorrect way below.
                  //-- Apply rotation force
                  /*long boneHandle = pUnit->getVisual()->getBoneHandle("Bip01 Head");
                  BVector bonePos;
                  BMatrix worldMatrix;
                  pUnit->getWorldMatrix(worldMatrix);
                  pUnit->getVisual()->getBone(boneHandle, &bonePos, NULL, NULL, &worldMatrix);
                  BVector spinForce = -pUnit->getForward();            
                  spinForce.scale(cSpinForce * elapsed);                       
                  pPhysicsObject->applyPointImpulse(spinForce, bonePos); */

                  //-- Apply rotation force
                  long boneHandle = pUnit->getVisual()->getBoneHandle("Bip01 Head");

                  // Sorry, epic mega hax :(
                  if (boneHandle == -1)
                     boneHandle = pUnit->getVisual()->getBoneHandle("BoneBubble02");

                  BVector bonePos;
                  if (pUnit->getVisual()->getBone(boneHandle, &bonePos))
                  {
                     BVector spinForce = -pUnit->getForward();            
                     spinForce.scale(cSpinForce * elapsed);           
                     BVector physPos;
                     pPhysicsObject->getPosition(physPos);
                     pPhysicsObject->applyPointImpulse(spinForce, bonePos+physPos); 
                  }
                  else
                  {
                     // [7/1/2008 xemu] if we get here, we attempted to do this on something without an appropriate bone! 
                     BASSERT(false);
                  }

                  /* AJL 1/11/08 - Can't do stuff based on the current user since doing that will cause the game
                                   to OOS since physics objects are now units. */
                  // [7/1/2008 xemu] rewrote this to use the "AI" "focus" "position" (ie, tracking hover pts for players and AIs) but not using it for now. 
                  /*
                  if(mFlagFlyTowardCamera)
                  {
                     BEntity *pKiller = gWorld->getEntity(pUnit->getKilledByID().asLong());
                     if (pKiller != NULL)
                     {
                        BVector eyePos = pKiller->getPlayer()->getAI()->getFocusPosition();
                        BVector physicsPos;
                        pPhysicsObject->getPosition(physicsPos);
                        BVector dirTowardCamera = eyePos - physicsPos;
                        dirTowardCamera.normalize();
                        dirTowardCamera.scale(cFlyTowardCameraForce);
                        pPhysicsObject->applyImpulse(dirTowardCamera * elapsed);
                     }
                  }
                  */
               }
               else if(mFlagRocketExplode)
               {
                  pUnit->explode(cExplodeTypeMethane);
                  pUnit->setLifespan(0);
                  setState(BAction::cStateDone);

                  // [11/10/2008 xemu] and handle any death spawn (like for the grunt confetti effect) 
                  const BProtoObject* pProtoObject = pUnit->getProtoObject();
                  if (pProtoObject)
                  {
                     BProtoSquadID deathSpawnSquad = pProtoObject->getDeathSpawnSquad();
                     if (deathSpawnSquad != cInvalidProtoSquadID)
                     {
                        pUnit->doDeathSpawnSquad();
                     }
                  }

               }
            }

            //Capps -- Testing warthog jump
            testForJumpLine(pUnit, pPhysicsObject);

            //Save our position.
            mLastPosition=pUnit->getPosition();
            mLastPositionValid=true;

            // DLM 10/29/08 - Test for early termination.. 
            
            if (mFlagEarlyTerminate)
            {
               pPhysicsObject->setLinearVelocity(BVector(0.0f, 0.0f, 0.0f));
               pPhysicsObject->setAngularVelocity(BVector(0.0f, 0.0f, 0.0f));
               pPhysicsObject->forceDeactivate();
               mFlagEarlyTerminate = false;
               mFlagAllowEarlyTerminate = false;
            }            
         }
         else
         {
            // DLM 10/28/08 If ever we're terminating for any reason, then reset early terminate.
            mFlagEarlyTerminate = false;
            mFlagAllowEarlyTerminate = false;

            // If requested update moving flag/obstruction (via endMove) when physics goes inactive
            if (mFlagEndMoveOnInactivePhysics)
            {
               pUnit->endMove();
               mFlagEndMoveOnInactivePhysics = false;
            }

            // If this object has become inactive and the proper flag is set, turn it off
            if (mFlagCompleteOnInactivePhysics)//(pPhysicsObject->getRigidBody() && (pPhysicsObject->getRigidBody()->getNumActions() <= 0))
            {
               // If there's an active thrown action, physics is still active, so don't put in wait state
               //if (!pUnit->getActionByType(BAction::cActionTypeUnitThrown))
                  setState(BAction::cStateWait);
            }

            // if our physics unit is inactive, and we're only waiting to complete on first collision, then reactivate
            if (mFlagCompleteOnFirstCollision)
               pPhysicsObject->forceActivate();
         }

         // see if we need to complete this update
         if (mFlagCompleteOnFirstCollision && mFlagHadCollision)
            setState(BAction::cStateWait);

         break;
      }
   }

   mFlagFirstUpdate = false;

   return (true);
}


//==============================================================================
//==============================================================================
void BUnitActionPhysics::setupRocketOnDeath()
{
   mFlagRocketOnDeath = true;
   mFlagRocketExplode = true;

   mStartRocketTime = gWorld->getGametime();      
   mRocketRotation = getRandRangeFloat(cSimRand, 0, cTwoPi);
   
   /* AJL 1/11/08 - OOS fix
   if(getRandRangeFloat(cSimRand,0,1) < cChanceOfSecondaryRocket)
   {
      mFlagFlyTowardCamera = true;      
      mRocketTime = getRandRange(cSimRand, cSecondaryMinRocketTime, cSecondaryMaxRocketTime);
   }
   else
   */
   {
      mFlagFlyTowardCamera = false;
      mRocketTime = getRandRange(cSimRand, cMinRocketTime, cMaxRocketTime);      
   }
}

//=============================================================================
// BUnitActionPhysics::collision
//=============================================================================
bool BUnitActionPhysics::collision(const BPhysicsCollisionData &collisionData)
{
   // Only play sound once
   if (mFlagHadCollision)
      return false;

//-- FIXING PREFIX BUG ID 1545
   const BUnit* pUnitWeCollidedWith  = static_cast<BUnit*>(collisionData.mpObjectB);
//--
   bool doImpact = false;
   byte surfaceType = 0;

   // Get surface type of the unit we collided with
   if (pUnitWeCollidedWith)
   {
      const BProtoObject* pProto = pUnitWeCollidedWith->getProtoObject();
      if (pProto)
      {
         surfaceType = pProto->getSurfaceType();
         doImpact = true;
      }
   }
   // Get surface type of terrain tile we collided with
   else
   {
      surfaceType = gTerrainSimRep.getTileType(collisionData.mLocation);
      doImpact = true;
   }

   // Play impact sound
   if (doImpact)
   {
      mImpactPosition = collisionData.mLocation;
      mImpactSurfaceType = surfaceType;
      mFlagImpactSound = true;
      mFlagTerrainEffect = true;
      mFlagHadCollision = true;
      mFlagRemoveCollisionListener = true;
   }

   return false;
}

//==============================================================================
// BUnitActionPhysics::notify
//==============================================================================
void BUnitActionPhysics::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (eventType)
   {
      case BEntity::cEventCorpseRemove:
         pUnit->computeDopple();
         pUnit->setFlagDopples(false);
         setState(cStateFading); // Corpse manager is telling us to go away
         break;
   }
}

//==============================================================================
// BUnitActionPhysics::testForJumpLine
//==============================================================================
bool BUnitActionPhysics::testForJumpLine(BUnit *pUnit, BPhysicsObject *pPhysicsObject)
{
   //First check our requirements.
   BASSERT(pUnit);
   BASSERT(pPhysicsObject);

   //Make sure we are a warthog
   if(!pUnit->canJump())
      return false;

   //We need the DesignObjectManager, and it should have some lines in it.
   BDesignObjectManager* pDOM=gWorld->getDesignObjectManager();
   if (!pDOM || (pDOM->getDesignLineCount() <= 0))
      return false;

   //Make sure we have a valid last position, and make sure we have moved.
   BVector currentPosition=pUnit->getPosition();
   if (!mLastPositionValid || (currentPosition == mLastPosition))
      return false;

   //Go through the design lines, looking for ones designated as jump lines (currently mValue==999 and mValues[1]==1)
   for (uint i=0; i < pDOM->getDesignLineCount(); i++)
   {
      //Get the line.  Ignore non-physics lines.
      BDesignLine& line=pDOM->getDesignLine(i);
      if (!line.mDesignData.isPhysicsLine() || (line.mDesignData.mValues[1] != 1)) //1 = designation for a jump line, 2 = land line
         continue;

      //See if we intersect.
      BVector intersectionPoint;
      if (line.imbeddedIncidenceIntersects(pUnit->getForward(), mLastPosition, currentPosition, intersectionPoint))
      {
         //gConsoleOutput.debug("Crossed Line: %d\n", i);

         int lineID = line.mDesignData.mValues[0];  //mValues[0] stores an identifying number so we can match it to the landing line

         //Find the matching landing line
         for (uint i=0; i < pDOM->getDesignLineCount(); i++)
         {
            //Get the line.  Ignore non-physics lines.
            BDesignLine& line2=pDOM->getDesignLine(i);
            if (!line2.mDesignData.isPhysicsLine() || (line2.mDesignData.mValues[0] != lineID) || (line2.mDesignData.mValues[1] != 2))
               continue;

            //Found the landing line, now find the best spot on it, given the angle;
            BVector projectionPoint = pUnit->getVelocity();
            projectionPoint.y = 0.0f;
            projectionPoint.safeNormalizeEstimate();
            projectionPoint.scale(200.0f);
            projectionPoint.assignSum(projectionPoint, currentPosition);  //This point is 200 meters out from the unit's current position, in the xz direction of the velocity

            BVector destinationPoint;
            bool ret = line2.intersects(currentPosition, projectionPoint, destinationPoint);  //find where our projected path crosses the landing line
            if( ret == false )
               line2.closestPointToLine(currentPosition, pUnit->getVelocity(), destinationPoint);  //if it doesn't cross, find the closest end point.

            //Now calculate some distances and velocities
            float jumpDistance = destinationPoint.xzDistance( currentPosition );
            float heightDifference = currentPosition.y - destinationPoint.y;
            //float currentVelocity = pUnit->getVelocity().length();
            float gravity = float(gWorld->getPhysicsWorld()->getHavokWorld()->getGravity().length3()) * (4.0f + 1.0f);  //in physicswarthogaction, cExtraGravityFactor = 4.0f

            //We're choosing an angle based on the difference in height of the jump.  I tried another calculation, but it didn't look as good, and this is simpler.
            float angleNeeded;
            if (heightDifference > 5.0f)  //positive, going off cliff
               angleNeeded = 0.1745f;        //10 degrees
            else if (heightDifference < -5.0f)  //negative, jumping up a cliff
               angleNeeded = 0.7854f;        //45 degrees
            else                           //close to 0, mostly flat
               angleNeeded = 0.5236f;        //30 degrees

            //gConsoleOutput.debug("angleNeeded: %f   heightDifference: %f", angleNeeded, heightDifference);

            //The function to calculate the velocity (v) needed at launch given the angle (Theta), height difference (Y0) and distance (d) is:
            // sqrt( ( gravity * d^2 ) / ( 2 * cos^2(Theta) * (Y0 + d * tan(Theta) )
            float cosAngle = Math::fFastCos( angleNeeded );
            float sinAngle = Math::fFastSin( angleNeeded );
            float newVelocity = sqrtf( gravity * jumpDistance * jumpDistance / (2.0f * cosAngle * (cosAngle * heightDifference + jumpDistance * sinAngle)));
            //gConsoleOutput.debug("angleNeeded: %f   newVelocity: %f   currVelocity: %f", angleNeeded, newVelocity, currentVelocity);


            //Now create the new velocity vector needed to hit the destination point
            BVector newVelocityVec = destinationPoint - currentPosition;
            newVelocityVec.y = 0;  //getXZ vector
            newVelocityVec.normalize();
            newVelocityVec.y = tanf(angleNeeded);
            newVelocityVec.normalize();
            newVelocityVec.scale(newVelocity);

            //Now set the linear velocity of the physics object
            //BVector oldVelocityVec;
            //pPhysicsObject->getLinearVelocity(oldVelocityVec);

#ifdef SYNC_UnitAction
            syncUnitActionData("BUnitActionPhysics::testForJumpLine setLinearVelocity", newVelocityVec);
#endif
            pPhysicsObject->setLinearVelocity(newVelocityVec);
            pPhysicsObject->setLinearDamping(0.0f);
            //gConsoleOutput.debug("Old Velocity: (%f, %f, %f)  New Velocity: (%f, %f, %f)\n", oldVelocityVec.x, oldVelocityVec.y, oldVelocityVec.z, newVelocityVec.x, newVelocityVec.y, newVelocityVec.z );

            //We found the line and the landing line, and set the velocity, so return true.
            return true;
         }
      }
   }
   return false;
}


//==============================================================================
// BUnitActionPhysics::enableDynamicCollisionFilter
//
// Makes the collision filter change to collisionFilter once the shape of the
// unit that has this action is outside the parentID unit.  [This is used when 
// parts break of of buildings and we want the part and the building to be 
// uncollidable until the part is completelly outside the shape of the building, 
// else we get interpenetration and the parts ends up flying off really fast into 
// space].
//==============================================================================
void BUnitActionPhysics::enableDynamicCollisionFilter(long collisionFilter, BEntityID parentID)
{
   mCollisionFilterInfo = collisionFilter;
   mParentEntityID = parentID;

   mFlagDynamicCollisionFilter = true;
}

//==============================================================================
//==============================================================================
void BUnitActionPhysics::setAnimRate(float elapsedTime, BVector oldPosition, BVector oldFwd, BVector newFwd)
{
   BUnit* pUnit = mpOwner->getUnit();
   BVisual* pVisual = pUnit->getVisual();
   BVector newPosition = pUnit->getPosition();

   // Calculate SIM velocity
   BVector simTranslation;
   simTranslation.assignDifference(newPosition, oldPosition);
   simTranslation.y = 0.0f;
   BVector simVelocity;
   float oneOverTime = 1.0f / elapsedTime;
   simVelocity.assignProduct(oneOverTime, simTranslation);
   pUnit->setVelocity(simVelocity);
   //float simScalarVelocity = simVelocity.length();
   float simScalarVelocity = simVelocity.dot(pUnit->getForward());

   if ((elapsedTime > cFloatCompareEpsilon) && pVisual && (pVisual->mModelAsset.mType == cVisualAssetGrannyModel) && (gConfig.isDefined(cConfigEnableMotionExtraction) || gWorld->isPlayingCinematic()))
   {
      BGrannyInstance* pGrannyInstance = (BGrannyInstance *) pVisual->mpInstance;

      // It is. Scale the animation appropriately.
      if (pGrannyInstance && pGrannyInstance->hasMotionExtraction())
      {
         float animationDuration = pUnit->getAnimationDuration(cMovementAnimationTrack);
         if (animationDuration > cFloatCompareEpsilon)
         {
            // SLB: TODO precalc this
            // Calculate motion extraction velocity
            BMatrix extractedMotion;
            extractedMotion.makeIdentity();
            pGrannyInstance->getExtractedMotion(animationDuration, extractedMotion);

            long currentAnimType = pUnit->getAnimationType(cMovementAnimationTrack);
            if (pUnit->isMoveAnimType(currentAnimType))
            {
               BVector extractedTranslation;
               extractedMotion.getTranslation(extractedTranslation);
               syncUnitActionData("BUnitActionMove::advanceToNewPosition extractedMotion", extractedTranslation);
               extractedTranslation.y = 0.0f;
               BVector extractedVelocity;
               extractedVelocity.assignProduct(1.0f / (animationDuration * pUnit->getAnimationRate()), extractedTranslation);
               float extractedScalarVelocity = extractedVelocity.dot(cZAxisVector); // project to forward vector

               if (extractedScalarVelocity > cFloatCompareEpsilon)
               {
                  // Set animation rate
                  float animationRate = simScalarVelocity / extractedScalarVelocity;
                  float clampedAnimationRate = Math::Clamp(animationRate, 0.5f, 2.0f);
                  pUnit->setAnimationRate(clampedAnimationRate);
               }
            }
            else if (pUnit->isTurnAnimType(currentAnimType))
            {
               BVector extractedYaw;
               extractedMotion.getForward(extractedYaw);
               syncUnitActionData("BUnitActionMove::advanceToNewPosition extractedYaw", extractedYaw);
               float extractedScalarVelocity = cZAxisVector.angleBetweenVector(extractedYaw) / (animationDuration * pUnit->getAnimationRate());

               oldFwd.y = 0.0f;
               newFwd.y = 0.0f;
               float simScalarVelocity = oldFwd.angleBetweenVector(newFwd) / elapsedTime;
               if (extractedScalarVelocity > cFloatCompareEpsilon)
               {
                  // Set animation rate
                  float animationRate = simScalarVelocity / extractedScalarVelocity;
                  float clampedAnimationRate = Math::Clamp(animationRate, 0.5f, 2.0f);
                  pUnit->setAnimationRate(clampedAnimationRate);
               }
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionPhysics::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   bool clamshellCollisionListener = (mpClamshellCollisionListener != NULL);
   GFWRITEVAR(pStream, bool, clamshellCollisionListener);
   if (clamshellCollisionListener)
      GFWRITEVAL(pStream, bool, mpClamshellCollisionListener->getCollisionProcessed());

   GFWRITEVAR(pStream, long, mPhysicsInfoID);
   GFWRITEVECTOR(pStream, mImpactPosition);
   GFWRITEVECTOR(pStream, mLastPosition);
   GFWRITEVAR(pStream, long, mParentEntityID);
   GFWRITEVAR(pStream, long, mCollisionFilterInfo);
   GFWRITEVAR(pStream, float, mRocketRotation);
   GFWRITEVAR(pStream, DWORD, mRocketTime);
   GFWRITEVAR(pStream, DWORD, mStartRocketTime);
   GFWRITEVAR(pStream, long, mTerrainEffect);
   GFWRITEVAR(pStream, float, mGotoWaitTimer);
   GFWRITEVAR(pStream, int8, mImpactSoundSet);
   GFWRITEVAR(pStream, byte, mImpactSurfaceType);
   GFWRITEVAR(pStream, bool, mLastPositionValid);
   GFWRITEBITBOOL(pStream, mFlagFirstUpdate);
   GFWRITEBITBOOL(pStream, mFlagRocketOnDeath);      
   GFWRITEBITBOOL(pStream, mFlagRocketExplode);
   GFWRITEBITBOOL(pStream, mFlagFlyTowardCamera);
   GFWRITEBITBOOL(pStream, mFlagCompleteOnInactivePhysics);
   GFWRITEBITBOOL(pStream, mFlagEndMoveOnInactivePhysics);
   GFWRITEBITBOOL(pStream, mFlagImpactSound);
   GFWRITEBITBOOL(pStream, mFlagDynamicCollisionFilter);
   GFWRITEBITBOOL(pStream, mFlagTerrainEffect);
   GFWRITEBITBOOL(pStream, mFlagCompleteOnFirstCollision);
   GFWRITEBITBOOL(pStream, mFlagHadCollision);
   GFWRITEBITBOOL(pStream, mFlagRemoveCollisionListener);
   GFWRITEBITBOOL(pStream, mFlagCollisionListener);
   GFWRITEBITBOOL(pStream, mFlagEarlyTerminate);
   GFWRITEBITBOOL(pStream, mFlagAllowEarlyTerminate);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPhysics::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   bool clamshellCollisionListener;
   GFREADVAR(pStream, bool, clamshellCollisionListener);
   if (clamshellCollisionListener)
   {
      bool collisionProcessed;
      GFREADVAR(pStream, bool, collisionProcessed);
      mpClamshellCollisionListener = BClamshellCollisionListener::getInstance();
      mpClamshellCollisionListener->setCollisionProcessed(collisionProcessed);
      BPhysicsObject* pPhysicsObject = mpOwner->getPhysicsObject();
      if (pPhysicsObject && pPhysicsObject->getRigidBody())
         pPhysicsObject->addHavokCollisionListener(mpClamshellCollisionListener);
   }

   GFREADVAR(pStream, long, mPhysicsInfoID);
   GFREADVECTOR(pStream, mImpactPosition);
   GFREADVECTOR(pStream, mLastPosition);
   if (BAction::mGameFileVersion >= 34)
   {
      GFREADVAR(pStream, long, mParentEntityID);
   }
   else
   {
      float dummy;
      GFREADVAR(pStream, float, dummy);
   }
   GFREADVAR(pStream, long, mCollisionFilterInfo);
   GFREADVAR(pStream, float, mRocketRotation);
   GFREADVAR(pStream, DWORD, mRocketTime);
   GFREADVAR(pStream, DWORD, mStartRocketTime);
   GFREADVAR(pStream, long, mTerrainEffect);
   GFREADVAR(pStream, float, mGotoWaitTimer);
   GFREADVAR(pStream, int8, mImpactSoundSet);
   GFREADVAR(pStream, byte, mImpactSurfaceType);
   GFREADVAR(pStream, bool, mLastPositionValid);
   GFREADBITBOOL(pStream, mFlagFirstUpdate);
   GFREADBITBOOL(pStream, mFlagRocketOnDeath);      
   GFREADBITBOOL(pStream, mFlagRocketExplode);
   GFREADBITBOOL(pStream, mFlagFlyTowardCamera);
   GFREADBITBOOL(pStream, mFlagCompleteOnInactivePhysics);
   GFREADBITBOOL(pStream, mFlagEndMoveOnInactivePhysics);
   GFREADBITBOOL(pStream, mFlagImpactSound);
   GFREADBITBOOL(pStream, mFlagDynamicCollisionFilter);
   GFREADBITBOOL(pStream, mFlagTerrainEffect);
   GFREADBITBOOL(pStream, mFlagCompleteOnFirstCollision);
   GFREADBITBOOL(pStream, mFlagHadCollision);
   GFREADBITBOOL(pStream, mFlagRemoveCollisionListener);
   bool collisionListener;
   GFREADBITBOOL(pStream, collisionListener);

   if (collisionListener && mpOwner->getPhysicsObject())
   {
      mpOwner->getPhysicsObject()->addGameCollisionListener(this);
      mFlagCollisionListener=true;
   }

   // DLM - early terminate flag
   if (BAction::mGameFileVersion >= 37)
   {
      GFREADBITBOOL(pStream, mFlagEarlyTerminate);
   }
   else
   {
      mFlagEarlyTerminate = false;
   }

   if (BAction::mGameFileVersion >= 45)
   {
      GFREADBITBOOL(pStream, mFlagAllowEarlyTerminate);
   }
   else
   {
      mFlagAllowEarlyTerminate = false;
   }

   // ajl 8/25/08 - i'm having a problem restoring the physics object correctly (specifically clamshells) where it doesn't automatically go 
   // inactive like it does in the orinal game, so force it to go inactive on the first collision here. otherwise the physics objects gets
   // unstable and shoots off into the air.
   if (mFlagCompleteOnInactivePhysics)
   {
      if (mState != cStateWait)
         mGotoWaitTimer=2.0f;
   }

   return true;
}
