//==============================================================================
// powerWave.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "powerWave.h"
#include "player.h"
#include "world.h"
#include "protopower.h"
#include "database.h"
#include "squad.h"
#include "user.h"
#include "usermanager.h"
#include "commandmanager.h"
#include "commands.h"
#include "commandtypes.h"
#include "powermanager.h"
#include "unitquery.h"
#include "SimTarget.h"
#include "debugprimitives.h"
#include "squadactionmove.h"
#include "squadactionattack.h"
#include "simhelper.h"
#include "squadplotter.h"
#include "tactic.h"
#include "soundmanager.h"
#include "worldsoundmanager.h"
#include "SimOrderManager.h"
#include "render.h"
#include "uimanager.h"
#include "squadAI.h"
#include "actionmanager.h"
#include "physics.h"
#include "powerhelper.h"
#include "physicsworld.h"
#include "physicsgravityballpullaction.h"
#include "unitactionphysics.h"
#include "damagetemplate.h"
#include "unitactionavoidcollisionair.h"
#include "physicsinfo.h"
#include "physicsinfomanager.h"
#include <Physics/Collide/Shape/Convex/Box/hkpBoxShape.h>

static const float cAudioReactionInterval = 5.0f;
static const long cHintId = 24948;
static const uint cMaxUnitArraySize = 32;
static const float cPullingRangeBufferMultiplier = 1.5f;

static const float cBallCenterHeight = 12.5f;
static const float cBallHeightVariance = 2.5f;
static const DWORD cBallPeriodTime = 3000;

static const float cNudgeScalarAir = 0.25f;
static const float cLightningDistanceWeightScalar = 3.0f;

//==============================================================================
//==============================================================================
void BWaveGravityBall::init()
{
   // Make sure all member variables are initialized.
   mBallID = cInvalidObjectID; 
   mState = cNone;
}

//==============================================================================
//==============================================================================
void BWaveGravityBall::shutdown()
{
   BObject* pBallObject = gWorld->getObject(mBallID);
   if (pBallObject)
      pBallObject->kill(false);

   mBallID = cInvalidObjectID;
   mState = cNone;
}

//==============================================================================
//==============================================================================
void BWaveGravityBall::createBall(BPlayerID playerId, const BVector& location, BProtoObjectID ballType, bool sync)
{
   BASSERT(mBallID == cInvalidObjectID);

   BObjectCreateParms parms;
   parms.mPlayerID = playerId;
   parms.mPosition = location;
   parms.mProtoObjectID = ballType;
   if (sync)
   {
      parms.mType = BEntity::cClassTypeUnit;
      BUnit* pUnit = gWorld->createUnit(parms);
      BASSERT(pUnit);
      mBallID = pUnit->getID();
   }
   else
   {
      parms.mType = BEntity::cClassTypeObject;
      BObject* pObject = gWorld->createObject(parms);
      BASSERT(pObject);
      mBallID = pObject->getID();
   }
   setState(cStagnant);
}

//==============================================================================
//==============================================================================
BObject* BWaveGravityBall::getBallObject() const
{
   return gWorld->getObject(mBallID);
}

//==============================================================================
//==============================================================================
void BWaveGravityBall::setState(EState newState)
{
   mState = newState;

   BObject* pBallObject = getBallObject();
   //BASSERT(pBallObject);
   if (!pBallObject)
      return;

   if (newState == cStagnant)
      pBallObject->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeWork, true, true);
   else if (newState == cPulling)
      pBallObject->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeIdle, true, true);
   else if (newState == cPullingFull)
      pBallObject->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeStop, true, true);
   else if (newState == cExploding)
      pBallObject->setAnimationState(BObjectAnimationState::cAnimationStateWork, cAnimTypeAttack, true, true);
   pBallObject->computeAnimation();
}

//==============================================================================
//==============================================================================
bool BPowerUserWave::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mFakeGravityBall.init();
   mHorizontalMoveInputDir = cOriginVector;
   mVerticalMoveInputDir = cOriginVector;
   mLastUpdatePos = cOriginVector;
   mCameraFocusPoint = cOriginVector;
   mTimestampNextCommand = 0;
   mTimeUntilHint = 0.0f;
   mCommandInterval = 0;
   mMinBallDistance = 0.0f;
   mMaxBallDistance = 0.0f;
   mMaxBallSpeedStagnant = 0.0f;
   mMaxBallSpeedPulling = 0.0f;
   mCameraDistance = 0.0f;
   mCameraHeight = 0.0f;
   mCameraHoverPointDistance = 0.0f;
   mCameraMaxBallAngle = 0.0f;
   mPullingRange = 0.0f;
   mPickupShakeDuration = 0.0f;
   mPickupRumbleShakeStrength = 0.0f;
   mPickupCameraShakeStrength = 0.0f;
   mMaxBallHeight = 0.0f;
   mMinBallHeight = 0.0f;
   mExplodeTime = 0.0f;
   mDelayShutdownTimeLeft = -1.0f; 
   mHintShown = false;
   mHintCompleted = false;
   mShuttingDown = false;

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserWave::init, mID %d", gWorld->getGametimeFloat(), mID);
#endif

   if(!pUser)
      return false;

   // return if user is locked
   mpUser = pUser;
   if(mpUser->isUserLocked())
      return false;

   // return if player can't pay cost
   mFlagNoCost = noCost;
   mProtoPowerID = protoPowerID;
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
      return false;
   if(!pUser->getPlayer() || (!mFlagNoCost && !pUser->getPlayer()->canUsePower(mProtoPowerID, ownerSquadID)))
      return false;
   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   mOwnerSquadID = ownerSquadID;

   const BSquad* pOwnerSquad = gWorld->getSquad(ownerSquadID);
   const BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pLeaderUnit)
      return false;

   if (pLeaderUnit->getFlagDoingFatality() || pLeaderUnit->getFlagFatalityVictim())
      return false;

   bool bSuccess = true;
   float commandInterval = 0.0f;
   BProtoObjectID ballProtoID = cInvalidProtoObjectID;

   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CommandInterval", commandInterval));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "HintTime", mTimeUntilHint));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MinBallDistance", mMinBallDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxBallDistance", mMaxBallDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MinBallHeight", mMinBallHeight));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxBallHeight", mMaxBallHeight));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxBallSpeedStagnant", mMaxBallSpeedStagnant));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxBallSpeedPulling", mMaxBallSpeedPulling));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CameraDistance", mCameraDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CameraHeight", mCameraHeight));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CameraHoverPointDistance", mCameraHoverPointDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CameraMaxBallAngle", mCameraMaxBallAngle));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "PickupShakeDuration", mPickupShakeDuration));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "PickupRumbleShakeStrength", mPickupRumbleShakeStrength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "PickupCameraShakeStrength", mPickupCameraShakeStrength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "PullingRange", mPullingRange));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "ExplodeTime", mExplodeTime));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "BallObject", ballProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "BallObject", ballProtoID));

   if (!bSuccess)
   {
      BFAIL("Missing or invalid data attribute.");
      return (false);
   }

   BCamera* pCamera = mpUser->getCamera();
   if (!pCamera)
      return false;

   if (!isPowerValidAtLocataion(mpUser->getHoverPoint()))
      return false;

   mCommandInterval = static_cast<DWORD>(1000.0f * Math::fSelectMax(commandInterval, 0.1f));

   setupUser();

   // other setup
   mHorizontalMoveInputDir.zero();
   mVerticalMoveInputDir.zero();
   mCameraMaxBallAngle *= cRadiansPerDegree;

   mTimestampNextCommand = gWorld->getGametime() + mCommandInterval;
   mFlagDestroy = false;
   mElapsed = 0;
   mPowerLevel = powerLevel;

   // create fake ball
   BVector ballLocation = mpUser->getHoverPoint();
   ballLocation.y += (mMaxBallHeight / 2.0f);
   mFakeGravityBall.createBall(mpUser->getPlayerID(), ballLocation, ballProtoID, false);
   mLastUpdatePos = ballLocation;

   // mark as initialized
   mInitialized = true;

   // Add the command to create the power on the sim side for everyone.
   BPlayerID playerID = mpUser->getPlayerID();
   BPowerCommand* pCommand = (BPowerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandPower);
   pCommand->setSenderType(BCommand::cPlayer);
   pCommand->setSenders(1, &playerID);
   pCommand->setRecipientType(BCommand::cPlayer);
   pCommand->setType(BPowerCommand::cTypeInvokePower2);
   pCommand->setPowerUserID(mID);
   pCommand->setProtoPowerID(protoPowerID);
   pCommand->setPowerLevel(powerLevel);
   pCommand->setTargetLocation(ballLocation);
   pCommand->setSquadID(ownerSquadID);
   pCommand->setFlag(BPowerCommand::cNoCost, mFlagNoCost);
   gWorld->getCommandManager()->addCommandToExecute(pCommand);

   mpUser->setFlagCameraZoomEnabled(false);

   updateCamera();
   float pitch = mpUser->getDefaultPitch();
   float zoom = mpUser->getDefaultZoom();

   mpUser->setInterpData(0.5f, mCameraFocusPoint, NULL, &pitch, &zoom);

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserWave::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserWave::shutdown, mID %d, shutting down", gWorld->getGametimeFloat(), mID);
#endif

   // Clean up our stuff.

   // Unlink ourselves form the sim side of the power

   // make sure we remove the hint
   if (mHintShown && !mHintCompleted)
   {
      BHintMessage* pHintMessage = gWorld->getHintManager()->getHint();
      if (pHintMessage && pHintMessage->getHintStringID() == cHintId)
      {
         gWorld->getHintManager()->removeHint(pHintMessage);
         gUIManager->hideHint();
      }
   }

   // kill our fake object
   mFakeGravityBall.shutdown();

   // fix up the user's mode
   mpUser->unlockUser();

   mpUser->setFlagCameraZoomEnabled(true);

   // get the camera to use the same hover point / direction
   mpUser->setFlagUpdateHoverPoint(true);
   mpUser->updateHoverPoint();

   mFlagDestroy = true;
   return (true);
}

//==============================================================================
//==============================================================================
void BPowerUserWave::onGravityBallStateSet(BWaveGravityBall::EState newState)
{
   mFakeGravityBall.setState(newState);
}

//==============================================================================
//==============================================================================
void BPowerUserWave::onPickupObject()
{
   BObject* pFakeObject = mFakeGravityBall.getBallObject();
   if (!pFakeObject)
      return;

   const BVector& ballLocation = pFakeObject->getPosition();
   gUI.playRumbleEvent(BRumbleEvent::cTypeImpact, BGamepad::cRumbleTypeFixed, mPickupRumbleShakeStrength, BGamepad::cRumbleTypeFixed, mPickupRumbleShakeStrength, mPickupShakeDuration, false);
   gUI.doCameraShake(ballLocation, mPickupCameraShakeStrength, mPickupShakeDuration, false, cInvalidObjectID);
}

//==============================================================================
//==============================================================================
void BPowerUserWave::getCameraRelativeInputDir(BCamera& camera, float inputX, float inputY, BVector& outDir)
{
   BVector forwardDir;
   if(_fabs(camera.getCameraDir().y)<_fabs(camera.getCameraUp().y))
      forwardDir=BVector(camera.getCameraDir().x,0,camera.getCameraDir().z);
   else
      forwardDir=BVector(camera.getCameraUp().x,0,camera.getCameraUp().z);
   forwardDir.normalize();

   BVector rightDir=camera.getCameraRight();
   rightDir.y = 0;
   rightDir.normalize();

   outDir = (inputX * rightDir + inputY * -forwardDir);
   outDir.normalize();
}

//==============================================================================
//==============================================================================
bool BPowerUserWave::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(!mInitialized)
      return false;

   if(!mpUser)
   {
      BFAIL("no mpUser in BPowerUserWave::handleInput");
      return false;
   }

   BCamera* pCamera = mpUser->getCamera();
   if(!pCamera)
   {
      BFAIL("no pCamera available in BPowerUserWave::handleInput");
      return false;
   }

   bool start = (event == cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(mpUser->getOption_ControlScheme());
   bool modifyAction    = mpUser->getFlagModifierAction();

   if (pInputInterface->isFunctionControl( BInputInterface::cInputTranslation, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
      getCameraRelativeInputDir(*pCamera, detail.mX, detail.mY, mHorizontalMoveInputDir);
      return true;
   }
   /*
   else if(pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
   mAttackInputDir = mLastMoveDir;
   mAttackInputDir.y = 0.0f;
   mAttackInputDir.safeNormalize();
   }
   */

   if(mpUser->handleInputScrollingAndCamera(port, event, controlType, detail))
      return(true);

   // Selection or do work
   if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) || 
      pInputInterface->isFunctionControl( BInputInterface::cInputAbility, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) ||
      pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( start )
      {
#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerUserWave::handleInput, mID %d, sending confirm command", gWorld->getGametimeFloat(), mID);
#endif
         cancelPower(); // not a great name in this particular case
      }
      return( true );
   }

   return( false );
}

//==============================================================================
void BPowerUserWave::cancelPower()
{
   gUI.playClickSound();

   BPowerInputCommand* c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( mpUser->getPlayerID(), cCommandPowerInput );
   if( !c )
      return;

   // Set up the command.
   long playerID = mpUser->getPlayerID();
   c->setSenders( 1, &playerID );
   c->setSenderType( BCommand::cPlayer );
   c->setRecipientType( BCommand::cPlayer );
   c->setType( BPowerInputCommand::cTypeConfirm );
   c->setPowerUserID(mID);
   c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
   gWorld->getCommandManager()->addCommandToExecute( c );

   // if we haven't set the delay shutdown yet, set it now
   if (!mShuttingDown)
   {
      mDelayShutdownTimeLeft = mExplodeTime;
      mFakeGravityBall.setState(BWaveGravityBall::cExploding);
      mShuttingDown = true;
   }
}

//==============================================================================
//==============================================================================
void BPowerUserWave::update(float elapsedTime)
{
   // first things first - check the delayed shutdown time
   if (mShuttingDown)
   {
      mDelayShutdownTimeLeft -= elapsedTime;
      if (mDelayShutdownTimeLeft <= 0.0f)
      {
         shutdown();
         return;
      }
   }

   // validate the most basic of basic checks to make sure the squad is alive
   if (!mFlagNoCost && !mShuttingDown)
   {
      BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
      if (!pOwnerSquad || !pOwnerSquad->isAlive() || pOwnerSquad->getFlagAttackBlocked() || pOwnerSquad->isGarrisoned())
      {
         cancelPower();
         return;
      }
   }

   // Always do this.
   mElapsed += elapsedTime;

   // Bomb check.
   BCamera* pCamera = mpUser->getCamera();
   if(!pCamera)
   {
      BFAIL("no pCamera available in BPowerUserWave::update");
      return;
   }

   if (!mHintCompleted)
   {
      if (mTimeUntilHint > 0.0f)
         mTimeUntilHint -= elapsedTime;
      else if (!mHintShown)
      {
         BPlayerIDArray players;
         players.add(mpUser->getPlayerID());

         gWorld->getHintManager()->addHint(cHintId, false, players, 10.0f);
         mHintShown = true;
      }
   }

   updateFakeBall(elapsedTime);
   updateCamera();

   if (gWorld->getGametime() >= mTimestampNextCommand)
   {
      BPowerInputCommand* c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( mpUser->getPlayerID(), cCommandPowerInput );
      if (!c)
         return;

      // Set up the command.
      long playerID = mpUser->getPlayerID();
      c->setSenders( 1, &playerID );
      c->setSenderType( BCommand::cPlayer );
      c->setRecipientType( BCommand::cPlayer );
      c->setType( BPowerInputCommand::cTypePosition );
      // Set the data that will be poked in.
      c->setVector(mLastUpdatePos);
      c->setPowerUserID(mID);
      c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
      // submit command
      gWorld->getCommandManager()->addCommandToExecute( c );

#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerUserCleansing::handleInput, mID %d, sent command to move to %.2f %.2f %.2f", gWorld->getGametimeFloat(), mID, newLoc.x, newLoc.y, newLoc.z);
#endif

      mTimestampNextCommand = gWorld->getGametime() + mCommandInterval;
   }  
}

//==============================================================================
//==============================================================================
void BPowerUserWave::updateFakeBall(float elapsedTime)
{
   // we don't update when the ball is exploding
   if (mFakeGravityBall.getState() == BWaveGravityBall::cExploding)
   {
      mHorizontalMoveInputDir.zero();
      return;
   }

   // Start out with where we were last update.
   BVector newPos = mLastUpdatePos;

   // If we have directional input, process it here then reset it.
   if ((mHorizontalMoveInputDir.x != 0.0f) || (mHorizontalMoveInputDir.z != 0.0f) || mVerticalMoveInputDir.y != 0.0f)
   {
      mHorizontalMoveInputDir.y = 0.0f;
      newPos += (mHorizontalMoveInputDir * getBallSpeed() * elapsedTime);
      newPos.y += (mVerticalMoveInputDir.y * getBallSpeed() * elapsedTime);
      gTerrainSimRep.clampWorld(newPos);
      mHorizontalMoveInputDir.zero();
      mVerticalMoveInputDir.zero();
   }

   // Additional restrictions based on distance from owner, if applicable.
   if(mOwnerSquadID.isValid())
   {
      const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
      if (pOwnerSquad)
      {
         const BVector& ownerPos = pOwnerSquad->getPosition();
         float xzDistance = ownerPos.xzDistance(newPos);
         if (xzDistance < mMaxBallDistance || xzDistance > mMinBallDistance)
         {
            BVector xzDir = newPos - ownerPos;
            xzDir.y = 0.0f;
            xzDir.normalize();

            if(xzDistance > mMaxBallDistance)
               newPos = ownerPos + xzDir * mMaxBallDistance;
            else if(xzDistance < mMinBallDistance)
               newPos = ownerPos + xzDir * mMinBallDistance;
         }
      }
   }

   float terHeight = 0.0f;
   gTerrainSimRep.getHeightRaycast(newPos, terHeight, true);
   float offset = sin((float)(gWorld->getGametime() % cBallPeriodTime) / cBallPeriodTime * cTwoPi);
   newPos.y = terHeight + cBallCenterHeight + (offset * cBallHeightVariance);      

   // Update the fake object if we have one.
   BObject* pFakeObject = mFakeGravityBall.getBallObject();
   if (pFakeObject)
      pFakeObject->setPosition(newPos);

   // Save off the pos for this update.
   mLastUpdatePos = newPos;
}

//==============================================================================
//==============================================================================
void BPowerUserWave::updateCamera()
{
   // if we're interpolating, then bail
   if (mpUser->isInterpolatingCamera())
      return;

   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
   BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;

   BCamera* pCamera = mpUser->getCamera();
   BObject* pFakeObject = mFakeGravityBall.getBallObject();

   if (!pLeaderUnit || !pCamera || !pFakeObject)
      return;

   // see if we need to update our focus point
   const BVector& leaderPosition = pLeaderUnit->getPosition();

   BVector dirToBall = pFakeObject->getPosition() - leaderPosition;
   dirToBall.y = 0.0f;
   float distanceToBall = dirToBall.length();
   dirToBall.normalize();

   // we need to update the hover point
   mCameraFocusPoint = leaderPosition + (dirToBall * distanceToBall / 2.0f);
   BVector pos = mCameraFocusPoint - (pCamera->getCameraDir() * mpUser->getDefaultZoom());
   pCamera->setCameraLoc(pos);
   mpUser->setCameraHoverPoint(mCameraFocusPoint);
}

//==============================================================================
//==============================================================================
void BPowerUserWave::updateUI()
{
#ifndef BUILD_FINAL
   if (gConfig.isDefined("WaveShowDebug"))
   {
      gpDebugPrimitives->addDebugSphere(mLastUpdatePos, mPullingRange, cDWORDBlue);
   }
#endif
}

//==============================================================================
//==============================================================================
float BPowerUserWave::getBallSpeed() const
{
   if (mFakeGravityBall.isPulling())
      return mMaxBallSpeedPulling;
   else if (mFakeGravityBall.getState() == BWaveGravityBall::cStagnant)
      return mMaxBallSpeedStagnant;
   else 
      return 0.0f;
}

//==============================================================================
//==============================================================================
bool BPowerUserWave::isPowerValidAtLocataion(const BVector& location)
{
   // fail if is in an invalid location
   BVector tempVec;
   if(!mpUser || !gWorld->checkPlacement(-1,  mpUser->getPlayerID(), location, tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly) ||
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(location, this))
      return false;

   // check range from squad (should be moved to sync code in static func >_<
   const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
   if (pOwnerSquad)
   {
      const BVector& ownerPos = pOwnerSquad->getPosition();
      float xzDistance = ownerPos.xzDistance(location);
      if (xzDistance > mMaxBallDistance || xzDistance < mMinBallDistance)
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
void BPowerUserWave::setupUser()
{
   BASSERT(mpUser);
   if (!mpUser)
      return;

   // lock user
   mpUser->lockUser(cInvalidTriggerScriptID);
   mpUser->changeMode(BUser::cUserModePower);
   mpUser->setUIPowerRadius(0.0f);
}

//==============================================================================
//==============================================================================
bool BPowerWave::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mNextTickTime = 0;
   mRealGravityBall.init();
   mDesiredBallPosition = cOriginVector;
   mCapturedUnits.empty();
   mExplodeCooldownLeft = -1.0f;
   mUnitsToPull.clear();
   mQueuedPickupObjects.clear();
   mCostPerTick.zero();
   mTickLength = 0.0f;
   mBallProtoID = cInvalidProtoObjectID;
   mLightningProtoID = cInvalidProtoObjectID;
   mLightningBeamVisualProtoID = cInvalidProtoObjectID;
   mDebrisProtoID = cInvalidProtoObjectID;
   mExplodeProtoID = cInvalidProtoObjectID;
   mPickupAttachmentProtoID = cInvalidProtoObjectID;
   mAudioReactionTimer = 0.0f;
   mLeaderAnimOrderID = 0;
   mMaxBallSpeedStagnant = 0.0f;
   mMaxBallSpeedPulling = 0.0f;
   mExplodeTime = 0.0f;
   mPullingRange = 0.0f;
   mExplosionForceOnDebris = 0.0f;
   mHealthToCapture = 0.0f;
   mNudgeStrength = 0.0f;
   mInitialLateralPullStrength = 0.0f;
   mCapturedRadialSpacing = 0.0f;
   mCapturedSpringStrength = 0.0f;
   mCapturedSpringRestLength = 0.0f;
   mCapturedSpringDampening = 0.0f;
   mCapturedMinLateralSpeed = 0.0f;
   mRipAttachmentChancePulling = 0.0f;
   mPickupObjectRate = 0.0f;
   mDebrisAngularDamping = 0.0f;
   mMaxCapturedObjects = 0;
   mNudgeChancePulling = 0;
   mThrowPartChancePulling = 0;
   mLightningChancePulling = 0;
   mpExplodeProtoAction = NULL;
   mpLightningProtoAction = NULL;
   mpDebrisProtoAction = NULL;
   mCompletedInitialization = false;
   mThrowUnitsOnExplosion = false;
   mExplodeSound = cInvalidCueIndex;
   mMinDamageBankPercentToThrow = 0.0f;
   mRevealedTeamIDs.clear();

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerWave::init, playerID %d, powerUserID %d, ownerSquadID %d", gWorld->getGametimeFloat(), playerID, powerUserID, ownerSquadID);
#endif

   // casting variables
   mPlayerID = playerID;
   mPowerUserID = powerUserID;
   mPowerLevel = powerLevel;
   mOwnerID = ownerSquadID;
   mTargetLocation = targetLocation;

   // bomb check
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
   {
      shutdown();
      return false;
   }

   BSquad* pOwnerSquad=gWorld->getSquad(ownerSquadID);
   if(!pOwnerSquad)
   {
      shutdown();
      return false;
   }

   BUnit* pLeaderUnit = pOwnerSquad->getLeaderUnit();
   if (pLeaderUnit && (pLeaderUnit->getFlagDoingFatality() || pLeaderUnit->getFlagFatalityVictim()))
   {
      shutdown();
      return false;
   }

   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      shutdown();
      return false;
   }
   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   float suppliesPerTick = 0.0f;
   mFlagIgnoreAllReqs = ignoreAllReqs;
   bool bSuccess = true;
   int tempInt = 0;
   float tempFloat = 0.0f;
   bool tempBool = false;

   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "TickLength", mTickLength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "SuppliesPerTick", suppliesPerTick));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxBallSpeedStagnant", mMaxBallSpeedStagnant));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxBallSpeedPulling", mMaxBallSpeedPulling));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "ExplodeTime", mExplodeTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "PullingRange", mPullingRange));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "ExplosionForceOnDebris", mExplosionForceOnDebris));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "HealthToCapture", mHealthToCapture));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "NudgeStrength", mNudgeStrength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "InitialLateralPullStrength", mInitialLateralPullStrength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CapturedRadialSpacing", mCapturedRadialSpacing));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CapturedSpringStrength", mCapturedSpringStrength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CapturedSpringDampening", mCapturedSpringDampening));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CapturedSpringRestLength", mCapturedSpringRestLength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CapturedMinLateralSpeed", mCapturedMinLateralSpeed));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "RipAttachmentChancePulling", mRipAttachmentChancePulling));
   mRipAttachmentChancePulling /= 100.0f;
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "DebrisAngularDamping", mDebrisAngularDamping));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxExplosionDamageBankPerCaptured", mMaxExplosionDamageBankPerCaptured));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "ExplosionDamageBankPerTick", mExplosionDamageBankPerTick));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CommandInterval", tempFloat));
   mCommandInterval = static_cast<DWORD>(tempFloat * 1000.0f);
   bSuccess = (bSuccess && (mCommandInterval > 0));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MinBallDistance", mMinBallDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxBallDistance", mMaxBallDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "PickupObjectRate", mPickupObjectRate));
   bSuccess = (bSuccess && pProtoPower->getDataInt(powerLevel, "MaxCapturedObjects", mMaxCapturedObjects));
   bSuccess = (bSuccess && pProtoPower->getDataInt(powerLevel, "LightningPerTick", mLightningPerTick));
   bSuccess = (bSuccess && pProtoPower->getDataInt(powerLevel, "NudgeChancePulling", tempInt));
   mNudgeChancePulling = tempInt * 255 / 100;
   bSuccess = (bSuccess && pProtoPower->getDataInt(powerLevel, "ThrowPartChancePulling", tempInt));
   mThrowPartChancePulling = tempInt * 255 / 100;
   bSuccess = (bSuccess && pProtoPower->getDataInt(powerLevel, "LightningChancePulling", tempInt));
   mLightningChancePulling = tempInt * 255 / 100;
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "BallObject", mBallProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "LightningProjectile", mLightningProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "ExplodeProjectile", mExplodeProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "DebrisProjectile", mDebrisProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "PickupAttachment", mPickupAttachmentProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataBool(powerLevel, "ThrowUnitsOnExplosion", tempBool));
   mThrowUnitsOnExplosion = tempBool;
   bSuccess = (bSuccess && pProtoPower->getDataSound(powerLevel, "ExplodeSound", mExplodeSound));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MinDamageBankPercentToThrow", mMinDamageBankPercentToThrow));

   // optional data
   pProtoPower->getDataProtoObject(powerLevel, "LightningBeamVisual", mLightningBeamVisualProtoID);

   if (!bSuccess)
   {
      BFAIL("Missing or invalid data attribute.");
      shutdown();
      return (false);
   }

   // fail if squad is in an invalid location
   bool powerValid = BPowerHelper::checkPowerLocation(mPlayerID, pOwnerSquad->getPosition(), this, true);
   powerValid &= BPowerHelper::checkPowerLocation(mPlayerID, targetLocation, this, true);
   if(!powerValid)
   {
      shutdown();
      return false;
   }

   mCostPerTick.add(gDatabase.getRIDSupplies(), suppliesPerTick);
   mMaxPossibleExplosionDamageBank = mMaxCapturedObjects * mMaxExplosionDamageBankPerCaptured;

   // basics
   mElapsed = 0.0f;

   mpExplodeProtoAction = BPowerHelper::getFirstProtoAction(*pPlayer, mExplodeProtoID);
   mpLightningProtoAction = BPowerHelper::getFirstProtoAction(*pPlayer, mLightningProtoID);
   mpDebrisProtoAction = BPowerHelper::getFirstProtoAction(*pPlayer, mDebrisProtoID);
   if (!mpExplodeProtoAction || !mpLightningProtoAction || !mpDebrisProtoAction)
   {
      shutdown();
      return false;
   }

   // pay the cost first, bail if we can't
   if (!mFlagIgnoreAllReqs && !pPlayer->canUsePower(mProtoPowerID, ownerSquadID))
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerWave::init, shutting down because player can't pay startup cost", gWorld->getGametimeFloat());
#endif
      shutdown();
      return false;
   }

   pPlayer->usePower(mProtoPowerID, mOwnerID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

   // apply the damage and speed modifications
   if (pOwnerSquad->getParentPlatoon())
      pOwnerSquad->getParentPlatoon()->removeAllOrders();
   pOwnerSquad->removeAllOrders();

   // queue the blocking animation order
   BSimOrder* pOrder = gSimOrderManager.createOrder();
   if (pOrder)
   {
      BSimTarget target;
      target.setID(ownerSquadID);
      pOrder->setTarget(target);
      pOrder->setPriority(BSimOrder::cPriorityUser);
      pOrder->setMode(cAnimTypePowerIdle);
      pOrder->setOwnerID(ownerSquadID);
      if (pOwnerSquad->queueOrder(pOrder, BSimOrder::cTypePlayBlockingAnimation, false))
         mLeaderAnimOrderID = pOrder->getID();
   }

   // this is so ridiculous, since play blocking animation doesn't actually play a blocking animation... 
   if (pLeaderUnit)
   {
      pLeaderUnit->setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateMisc, cAnimTypePowerIdle, true, true, -1, true);
      pLeaderUnit->computeAnimation();
      pLeaderUnit->lockAnimation(10000000, false);
   }

   BSquadAI* pSquadAI = pOwnerSquad->getSquadAI();
   if (pSquadAI)
      pSquadAI->setMode(BSquadAI::cModePower);

   // create real ball
   mRealGravityBall.createBall(mPlayerID, mTargetLocation, mBallProtoID, true);
   BObject* pBallObject = mRealGravityBall.getBallObject();
   BASSERT(pBallObject);
   mDesiredBallPosition = pBallObject->getPosition();

   // notify power user instance about our newly created ball
   BPowerUserWave* pWavePowerUser = getPowerUser();
   if (pWavePowerUser && !gConfig.isDefined("waveShowRealBall"))
      pBallObject->setFlagNoRenderForOwner(true);

   startPulling();

   mCompletedInitialization = true;
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerWave::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerWave::shutdown, mID %d, mPowerUserID %d", gWorld->getGametimeFloat(), mID, mPowerUserID);
#endif

   mRealGravityBall.shutdown();

   // apply the damage and speed modifications
   BSquad* pOwnerSquad=gWorld->getSquad(mOwnerID);
   if (pOwnerSquad && mCompletedInitialization)
   {
      if (pOwnerSquad->getParentPlatoon())
         pOwnerSquad->getParentPlatoon()->removeAllOrders();
      pOwnerSquad->removeAllOrders();

      // Remove the anim order from the leader unit
      BSimOrder* pOrder = pOwnerSquad->getOrderByID(mLeaderAnimOrderID);
      if (pOrder)
         pOwnerSquad->removeOrder(pOrder, false, false);

      // this is so ridiculous, since play blocking animation doesn't actually play a blocking animation... 
      BUnit* pLeaderUnit = pOwnerSquad->getLeaderUnit();
      if (pLeaderUnit)
      {
         pLeaderUnit->unlockAnimation();
         pLeaderUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         pLeaderUnit->computeAnimation();
      }

      BSquadAI* pSquadAI = pOwnerSquad->getSquadAI();
      if (pSquadAI)
         pSquadAI->setMode(BSquadAI::cModeNormal);

      //remove any remaining revealers
      BUnit *pOwnerUnit = pOwnerSquad->getLeaderUnit();
      if(pOwnerUnit)
      {
         for( int i=0; i<mRevealedTeamIDs.getNumber(); i++ )
         {
            pOwnerUnit->removeReveal(mRevealedTeamIDs.get(i));
            //gConsoleOutput.debug("Final Revealer Removed: %d", mRevealedTeamIDs.get(i));
         }
      }
   }

   BUnit* pUnit = NULL;
   for (uint i = 0; i < mUnitsToPull.getSize(); ++i)
   {
      pUnit = gWorld->getUnit(mUnitsToPull[i]);
      if (pUnit)
         pUnit->removeEventListener(this);
   }
   mUnitsToPull.clear();
   mCapturedUnits.empty();

   // shutdown our power user if there is one
   BPowerUserWave* pPowerUser = getPowerUser();
   if (pPowerUser)
      pPowerUser->shutdown();

   mFlagDestroy = true;

   return true;
}

//==============================================================================
//==============================================================================
void BPowerWave::attemptShutdown()
{
   // if we're pulling, explode and let that cooldown kill us when it runs out
   // if we're not pulling, then we should be exploding, so if that cooldown is up, shut us down
   if (mRealGravityBall.isPulling())
      explode();
   else if (mExplodeCooldownLeft <= 0.0f)
      shutdown();
}

//==============================================================================
//==============================================================================
void BPowerWave::updateRevealers(BUnit& ownerUnit, const BObject& ballObject)
{
   //////
   // See which units are near the beam and add a revealer on the controlling unit (prophet) for those teams.
   BDynamicSimArray<BTeamID>     previousRevealedTeamIDs;
   previousRevealedTeamIDs = mRevealedTeamIDs;
   //clear current.
   mRevealedTeamIDs.clear();

   // Halwes - 7/21/2008 - There may not be a valid owner squad if all requirements are ignored
   for (int i = 0; i < gWorld->getNumberTeams(); i++)
   {
      if (ballObject.isVisible(i))
         mRevealedTeamIDs.uniqueAdd(i);
   }

   //for all in previous array that aren't in the new, remove the reveal
   for (int i = 0; i < previousRevealedTeamIDs.getNumber(); i++)
   {
      if (!mRevealedTeamIDs.contains(previousRevealedTeamIDs.get(i)))
      {
         ownerUnit.removeReveal(previousRevealedTeamIDs.get(i));
         //gConsoleOutput.debug("Revealer Removed: %d", previousRevealedTeamIDs.get(i));
      }
   }

   //For all in the new array that aren't in the previous, add the reveal
   for (int i = 0; i < mRevealedTeamIDs.getNumber(); i++)
   {
      if (!previousRevealedTeamIDs.contains(mRevealedTeamIDs.get(i)))
      {
         ownerUnit.addReveal(mRevealedTeamIDs.get(i));
         //gConsoleOutput.debug("Revealer Added: %d", mRevealedTeamIDs.get(i));
      }
   }
}

//==============================================================================
//==============================================================================
void BPowerWave::update(DWORD currentGameTime, float lastUpdateLength)
{
   if(getFlagDestroy())
      return;

   mElapsed += lastUpdateLength;

   // audio reaction
   if(mAudioReactionTimer <= 0.0f)
   {
      gWorld->createPowerAudioReactions(getPlayerID(), cSquadSoundChatterReactPowWave, mTargetLocation, 60.0f, mOwnerID);
      mAudioReactionTimer = cAudioReactionInterval;
   }
   else
      mAudioReactionTimer -= lastUpdateLength;

   // check the explosion cooldown - has to be before all other bombs
   if (mExplodeCooldownLeft > 0.0f)
   {
      mExplodeCooldownLeft -= lastUpdateLength;

      if (mExplodeCooldownLeft <= 0.0f)
      {
         BASSERT(mRealGravityBall.getState() == BWaveGravityBall::cExploding);
         shutdown();
      }
      return;
   }

   // bomb checks
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pPlayer || !pOwnerSquad || !pOwnerSquad->isAlive() || !pLeaderUnit || !pLeaderUnit->isAlive() || pOwnerSquad->getFlagAttackBlocked() || pOwnerSquad->isGarrisoned())
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerWave::update, bomb check failed, shutting down", gWorld->getGametimeFloat());
#endif
      attemptShutdown();
      return;
   }
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      attemptShutdown();
      return;
   }

   // fail if squad is in an invalid location
   BObject* pBallObject = mRealGravityBall.getBallObject();
   if(!pBallObject)
   {
      attemptShutdown();
      return;
   }

   bool powerValid = BPowerHelper::checkPowerLocation(mPlayerID, pOwnerSquad->getPosition(), this, true);
   powerValid &= (pBallObject && BPowerHelper::checkPowerLocation(mPlayerID, pBallObject->getPosition(), this, true));
   if(!powerValid)
   {
      attemptShutdown();
      return;
   }

   updateRevealers(*pLeaderUnit, *pBallObject);

   // move the beam to the appropriate position
   if(pBallObject->getProtoObject())
   {
      const BVector& currentPosition = pBallObject->getPosition();      
      float currentDistance = currentPosition.distance(mDesiredBallPosition);
      float distanceThisFrame = (lastUpdateLength * getBallSpeed());
      BVector newPos = mDesiredBallPosition;

      // if we can travel far enough in this frame, take that position
      if (currentDistance >= distanceThisFrame)
      {
         BVector currentDirection = (mDesiredBallPosition - currentPosition);
         currentDirection.y = 0.0f;
         currentDirection.normalize();
         newPos = (currentPosition + (currentDirection * distanceThisFrame));
      }

      float terHeight = 0.0f;
      gTerrainSimRep.getHeightRaycast(newPos, terHeight, true);
      float offset = sin((float)(gWorld->getGametime() % cBallPeriodTime) / cBallPeriodTime * cTwoPi);
      newPos.y = terHeight + cBallCenterHeight + (offset * cBallHeightVariance);      

      BSimString debugStr;
      float delta = newPos.distance(pBallObject->getPosition());
      float dt = lastUpdateLength;
      float speed = 0.0f;
      if (dt > 0.0f)
         speed = delta / dt;

      debugStr.format("REAL BALL POS moved %f DIST, over %f TIME, overall SPEED = %f", delta, dt, speed);
      gConsole.output(cChannelAI, debugStr);

      // clamp to terrain edges
      gTerrainSimRep.clampWorld(newPos);
      pBallObject->setPosition(newPos);

      // orient the brute to face the ball 
      BVector dirToBall = newPos - pLeaderUnit->getPosition();
      dirToBall.y = 0.0f;
      dirToBall.normalize();
      pLeaderUnit->setForward(dirToBall);
      pLeaderUnit->setUp(cYAxisVector);
      pLeaderUnit->calcRight();
      pLeaderUnit->calcUp();
   }

   if (mRealGravityBall.isPulling())
      updatePulling(lastUpdateLength);
}

//==============================================================================
//==============================================================================
bool BPowerWave::submitInput(BPowerInput powerInput)
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerWave::submitInput, mID %d, mPowerUserID %d, input type %d", gWorld->getGametimeFloat(), mID, mPowerUserID, powerInput.mType);
#endif

   if (powerInput.mType == PowerInputType::cUserCancel)
   {
      // The user cancelled the power.
      attemptShutdown();
      return (true);
   }

   // we process no input other than cancel while we are changing modes or in the explosion state
   if (mRealGravityBall.getState() == BWaveGravityBall::cExploding)
      return true;

   if (powerInput.mType == PowerInputType::cPosition)
   {
      mDesiredBallPosition = powerInput.mVector;
      return true;
   }
   else if (powerInput.mType == PowerInputType::cUserOK)
   {
      if (mRealGravityBall.getState() == BWaveGravityBall::cStagnant)
         startPulling();
      else if (mRealGravityBall.isPulling())
         explode();
      return true;
   }

   // Insert handling for additional power input types here.

   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerWave::submitInput() received powerInput of unsupported type.");
   return (false);
}

//==============================================================================
//==============================================================================
void BPowerWave::startStagnant()
{
   BFAIL("We should never get into the stagnant state, since we auto start in the pulling state");
   setGravityBallState(BWaveGravityBall::cStagnant);

   // do anything else here? 
}

//==============================================================================
//==============================================================================
void BPowerWave::startPulling()
{
   setGravityBallState(BWaveGravityBall::cPulling);

   mCurrentExplosionDamageBank = 0.0f;
   mNextTickTime = mElapsed;
   
   // clear out our captured list (which should be empty) 
   mCapturedUnits.empty();
   mUnitsToPull.clear();
}

//==============================================================================
//==============================================================================
bool BPowerWave::attemptPullUnit(BUnit& unit, const BEntityID& leaderUnitId, const BEntityID& ballUnitId, const BVector& ballPosition)
{
   // return true if we successfully pulled the unit and no longer want to process it

   if (unit.getID() == leaderUnitId || unit.getID() == ballUnitId)
      return true;

   // completely ignore invulnerable units
   if (unit.getFlagInvulnerable())
      return true;

   // bail if we already have this unit
   if (mCapturedUnits.contains(unit.getID()))
      return true;

   // check the xyz distance
   float xyzDistance = (unit.getPosition() - ballPosition).length();
   xyzDistance -= unit.getObstructionRadius();
   
   // if we're outside of the pulling range + buffer, bail and no longer process 
   if (xyzDistance > (mPullingRange * cPullingRangeBufferMultiplier))
      return true;

   // if we're just outside of the pulling range, bail, but continue to process
   if (xyzDistance > mPullingRange)
      return false;

   // see if we can capture this unit
   if (captureUnit(unit))
      return true;

   // if the unit isn't alive, we're done
   if (!unit.getFlagAlive())
      return false;

   // only do the other effects every so often
   // why we only get 16 bits of random, I don't know
   long rand = getRand(cSimRand);

   // rip parts off of alive units
   if (rand % 255 < mThrowPartChancePulling)
      ripPartOffUnit(unit);

   // impulse units 
   // if ((rand >> 8) % 255 < mNudgeChancePulling)
   //   nudgeUnit(unit);

   return false;
}

//==============================================================================
//==============================================================================
void BPowerWave::updatePulling(float elapsedTime)
{
   // we always update our pull queue, even if we don't tick
   updateQueuedObjects();

   validateCapturedObjects();

   // update the pulling state (set state bails if already in that state) 
   if (mCapturedUnits.getNumItems() >= (uint)mMaxCapturedObjects && mCurrentExplosionDamageBank >= mMaxPossibleExplosionDamageBank)
      setGravityBallState(BWaveGravityBall::cPullingFull);
   else
      setGravityBallState(BWaveGravityBall::cPulling);

   // if it's not time for an update, get out of here
   if (mElapsed <= mNextTickTime)
      return;

   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   BEntityID leaderUnitID = cInvalidObjectID;
   if (pLeaderUnit)
      leaderUnitID = pLeaderUnit->getID();
   BObject* pBallObject = mRealGravityBall.getBallObject();
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   const BVector& ballPosition = pBallObject->getPosition();

   // only need to do this once per update
   pullCorpses();

   // scan live units
   BEntityIDArray unitsInArea(0, cMaxUnitArraySize);
   BEntityIDArray leavingUnits(0, cMaxUnitArraySize);
   BEntityIDArray arrivingUnits(0, cMaxUnitArraySize);

   // search a slightly bigger range, in case we'll need to check units we're getting close to
   // this needs to be a get units in area check, because debris are picked up and don't have a parent squad
   BUnitQuery query(ballPosition, mPullingRange * cPullingRangeBufferMultiplier, true);
   gWorld->getUnitsInArea(&query, &unitsInArea);

   BSimHelper::diffEntityIDArrays(mUnitsToPull, unitsInArea, &leavingUnits, NULL, NULL);

   // this frame, we affect all units in our area as well as all units that left that are dead
   // to catch units that were in our area, but died and are no longer in the obstruction manager
   BEntityIDArray unitsToPullNext;
   uint numUnitsInArea = (uint)unitsInArea.getNumber();
   uint numUnitsLeaving = (uint)leavingUnits.getNumber();
   BUnit* pUnit = NULL;
   bool firstLoop = true;
   bool pulledUnit = false;
   while(mElapsed > mNextTickTime)
   {
      for (uint i = 0; i < numUnitsInArea; i++)
      {
         pUnit = gWorld->getUnit(unitsInArea[i]);
         if (pUnit)
         {
            pulledUnit = attemptPullUnit(*pUnit, leaderUnitID, pBallObject->getID(), ballPosition);
            if (firstLoop && !pulledUnit)
               unitsToPullNext.add(unitsInArea[i]);
            }
         }

      // for leaving units, we only care about the dead ones (the ones that wouldn't have been
      // found in the get units in area, but we still care about)
      // or the thrown objects that may have gotten added via events
      for (uint i = 0; i < numUnitsLeaving; i++)
      {
         pUnit = gWorld->getUnit(leavingUnits[i]);
         if (pUnit && (!pUnit->getFlagAlive() || pUnit->getProtoID() == gDatabase.getPOIDPhysicsThrownObject()))
         {
            pulledUnit = attemptPullUnit(*pUnit, leaderUnitID, pBallObject->getID(), ballPosition);
            if (firstLoop && !pulledUnit)
               unitsToPullNext.add(leavingUnits[i]);
            }
         }

      mNextTickTime += mTickLength;
      firstLoop = false;

      bool didDamage = updateLightning(unitsToPullNext);

      if (didDamage)
      {
         // bump up the damage bank if we can
         if (mCurrentExplosionDamageBank < mMaxPossibleExplosionDamageBank)
         {
            mCurrentExplosionDamageBank += mExplosionDamageBankPerTick;
            mCurrentExplosionDamageBank = Math::Clamp(mCurrentExplosionDamageBank, 0.0f, mMaxPossibleExplosionDamageBank);
         }

         // we're done if the player can't pay the cost, otherwise pay it
         if(!mFlagIgnoreAllReqs)
         {
            if (!pPlayer->checkCost(&mCostPerTick))
            {
               if (BPowerHelper::getPowerUser(*this))
                  gUI.playPowerNotEnoughResourcesSound();

               attemptShutdown();
               return;
            }

            pPlayer->payCost(&mCostPerTick);
         }
      }
   }

   // diff the units to pull and units to pull next and attach / detach the event listeners
   BSimHelper::diffEntityIDArrays(mUnitsToPull, unitsToPullNext, &leavingUnits, &arrivingUnits, NULL);

   for (uint i = 0; i < leavingUnits.getSize(); ++i)
   {
      pUnit = gWorld->getUnit(leavingUnits[i]);
      if (pUnit)
      {
         pUnit->removeEventListener(this);
         
         BUnitActionAvoidCollisionAir* pAvoidCollisionAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
         if (pAvoidCollisionAction)
            pAvoidCollisionAction->setCanKamikaze(true);
      }
   }
   for (uint i = 0; i < arrivingUnits.getSize(); ++i)
   {
      pUnit = gWorld->getUnit(arrivingUnits[i]);
      if (pUnit)
      {
         pUnit->addEventListener(this);

         BUnitActionAvoidCollisionAir* pAvoidCollisionAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
         if (pAvoidCollisionAction)
            pAvoidCollisionAction->setCanKamikaze(false);
      }
   }

   mUnitsToPull = unitsToPullNext;
}

//==============================================================================
//==============================================================================
void BPowerWave::pullCorpses()
{
   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   BObject* pBallObject = mRealGravityBall.getBallObject();
   const BVector& ballPosition = pBallObject->getPosition();

   BObstructionNodePtrArray obstructions;
   float x1 = ballPosition.x - mPullingRange;
   float z1 = ballPosition.z - mPullingRange;
   float x2 = ballPosition.x + mPullingRange;
   float z2 = ballPosition.z + mPullingRange;
   long obstructionQuadTrees =
      BObstructionManager::cIsNewTypeNonCollidableUnit;          // Other corpses
   long obstructionTypes = 
      BObstructionManager::cObsNodeTypeCorpse;
   gObsManager.findObstructions(x1, z1, x2, z2, 0.0f, obstructionQuadTrees, obstructionTypes, -1, false, obstructions);

   long numObstructions = obstructions.getNumber();
   for (long i = 0; i < numObstructions; i++)
   {
      const BOPObstructionNode* pNode = obstructions.get(i);
      BDEBUG_ASSERT(pNode);
      BDEBUG_ASSERT(pNode->mObject);

      BUnit* pUnit = pNode->mObject->getUnit();
      if (pUnit)
      {
         BEntityID leaderUnitID = cInvalidObjectID;
         if (pLeaderUnit)
            leaderUnitID = pLeaderUnit->getID();
         attemptPullUnit(*pUnit, leaderUnitID, pBallObject->getID(), ballPosition);
      }
   }
}

//==============================================================================
//==============================================================================
void BPowerWave::failPulling()
{
   // drop all the units we have captured
   for (uint i = 0; i < mCapturedUnits.getNumItems(); ++i)
   {
      BUnit* pUnit = gWorld->getUnit(mCapturedUnits.getItem(i));
      BPhysicsObject* pPhysicsObject = (pUnit) ? pUnit->getPhysicsObject() : NULL;
      if (!pPhysicsObject)
         continue;

      // remove the ball actions
      for (uint j = 0; j < pPhysicsObject->getNumActions(); ++j)
      {
         BPhysicsGravityBallPullAction* pAction = static_cast<BPhysicsGravityBallPullAction*>(pPhysicsObject->getAction(j));
         if (pAction)
            gWorld->getPhysicsWorld()->getHavokWorld()->removeAction(pAction);
      }

      // set it to die when phys completes
      BUnitActionPhysics* pPhysicsAction = reinterpret_cast<BUnitActionPhysics*>(pUnit->getActionByType(BAction::cActionTypeUnitPhysics));
      if (pPhysicsAction)
         pPhysicsAction->setFlagCompleteOnInactivePhysics(true);

      pUnit->removeAttachmentsOfType(mPickupAttachmentProtoID);
   }
   
   BUnit* pUnit = NULL;
   for (uint i = 0; i < mUnitsToPull.getSize(); ++i)
   {
      pUnit = gWorld->getUnit(mUnitsToPull[i]);
      if (pUnit)
         pUnit->removeEventListener(this);
   }

   mUnitsToPull.clear();
   mCapturedUnits.empty();

   //startStagnant();
}

//==============================================================================
//==============================================================================
bool BPowerWave::updateLightning(const BEntityIDArray& unitsToTarget)
{
   // this is so horribly inefficient, it's not even funny :( 
   BObject* pBallObject = mRealGravityBall.getBallObject();
   const BVector& ballPosition = pBallObject->getPosition();

   float totalWeight = 0.0f;
   BSmallDynamicSimArray< std::pair<BEntityID, float> > squadsByWeight;

   for (uint i = 0; i < unitsToTarget.getSize(); ++i)
   {
      BUnit* pUnit = gWorld->getUnit(unitsToTarget[i]);
      if (!pUnit)
         continue;

      // don't include units that we can't attack
      if (!pUnit->isAttackable(pBallObject, true))
         continue;

      // check the xyz distance
      BVector toBall = pUnit->getPosition() - ballPosition;
      float xyzDistance = toBall.length();
      xyzDistance -= pUnit->getObstructionRadius();
      // if we're just outside of the pulling range, bail
      if (xyzDistance > mPullingRange)
         continue;

      BSquad* pSquad = pUnit->getParentSquad();
      if (!pSquad)
         continue;

      // insert it into our sorted list
      float combatValue = pSquad->getCombatValue();
      toBall.y = 0.0f;
      float distanceWeight = Math::Clamp(toBall.length() / mPullingRange, 0.0f, 1.0f);
      float weight = combatValue * (Math::Lerp(cLightningDistanceWeightScalar, 1.0f, distanceWeight));
      
      bool found = false;
      uint insertIndex = 0;
      for (; insertIndex < squadsByWeight.getSize(); ++insertIndex)
      {
         if (squadsByWeight[insertIndex].first == pSquad->getID())
         {
            found = true;
            break;
         }

         if (weight > squadsByWeight[insertIndex].second)
            break;
      }
      if (!found)
      {
         squadsByWeight.insert(insertIndex, std::pair<BEntityID, float>(pSquad->getID(), weight));
         totalWeight += weight;
      }
   }

   // strike lightning until we run out, randomly select target, weighted by combat value and distance to center
   int lightningBoltsLeft = mLightningPerTick;
   while (lightningBoltsLeft > 0 && squadsByWeight.getSize() > 0)
   {
      float randCombatVal = getRandRangeFloat(cSimRand, 0.0f, totalWeight);
      float combatValLeft = totalWeight;

      BSquad* pSquadToStrike = gWorld->getSquad(squadsByWeight[0].first);
      for (uint i = 0; i < squadsByWeight.getSize(); ++i)
      {
         combatValLeft -= squadsByWeight[i].second;
         if (combatValLeft <= randCombatVal)
         {
            pSquadToStrike = gWorld->getSquad(squadsByWeight[i].first);
            break;
         }
      }

      // we should always have found a squad, otherwise we shouldn't be in here
      BASSERT(pSquadToStrike);
      
      //DCP/VT:  If we somehow get a bogus squad here, let's just not deref the NULL ptr.  We'll dec
      //the bolts and let that run the loop out.
      --lightningBoltsLeft;
      if (!pSquadToStrike)
         continue;

      hitLightning(*pSquadToStrike);
   }

   return (lightningBoltsLeft != mLightningPerTick);
}

//==============================================================================
//==============================================================================
void BPowerWave::explode()
{
   BASSERT(mpExplodeProtoAction);

   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   BEntityID leaderId = (pLeaderUnit) ? pLeaderUnit->getID() : cInvalidObjectID;

   BObject* pBallObject = mRealGravityBall.getBallObject();
   BASSERT(pBallObject);

   // deal damage
   BObjectCreateParms parms;
   parms.mPlayerID = mPlayerID;
   parms.mProtoObjectID = mpExplodeProtoAction->getProjectileID();
   parms.mPosition = pBallObject->getPosition();

   // bump up the damage bank if we can
   float maxPossible = mCapturedUnits.getNumItems() * mMaxExplosionDamageBankPerCaptured;
   float clampedDamageBank = Math::Clamp(mCurrentExplosionDamageBank, 0.0f, maxPossible);

   float dps = mpExplodeProtoAction->getDamagePerSecond() + clampedDamageBank;
   BEntityID projId = gWorld->launchProjectile(parms, pBallObject->getPosition(), XMVectorZero(), XMVectorZero(), dps, mpExplodeProtoAction, mpExplodeProtoAction, leaderId, NULL, -1, true, true, true);

   // set flag so that projectile explodes in the air
   BProjectile* pExplodeProjectile = gWorld->getProjectile(projId);
   if (pExplodeProjectile)
      pExplodeProjectile->setFlagDetonate(true);

   // fling out all objects we have contained as projectiles
   float debrisDps = mpDebrisProtoAction->getDamagePerSecond();
   for (uint i = 0; i < mCapturedUnits.getNumItems(); ++i)
   {
      BUnit* pDebrisUnit = gWorld->getUnit(mCapturedUnits.getItem(i));
      BPhysicsObject* pPhysicsObject = (pDebrisUnit) ? pDebrisUnit->getPhysicsObject() : NULL;
      if (!pPhysicsObject)
         continue;

      // this should really never ever happen, but let's be defensive and safe and not destroy the ball
      if (pDebrisUnit->getID() == pBallObject->getID())
      {
         BFAIL("Somehow the vortex managed to capture itself!");
         continue;
      }

      // launch a debris projectile
      BObjectCreateParms parms;
      parms.mProtoObjectID = mpDebrisProtoAction->getProjectileID();
      pPhysicsObject->getCenterOfMassLocation(parms.mPosition);
      parms.mSourceVisual = pDebrisUnit->getID();
      parms.mVisualVariationIndex = pDebrisUnit->getVisualVariationIndex();
      parms.mCreatedByPlayerID = mPlayerID;
      parms.mPlayerID = pDebrisUnit->getColorPlayerID();

      BEntityID projectileId = gWorld->launchProjectile(parms, cOriginVector, XMVectorZero(), XMVectorZero(), debrisDps, mpDebrisProtoAction, mpDebrisProtoAction, leaderId, NULL, -1, true, true, true);

      BProjectile* pProjectile = gWorld->getProjectile(projectileId);
      if (pProjectile)
      {
         pProjectile->setForward(pDebrisUnit->getForward());
         pProjectile->setRight(pDebrisUnit->getRight());
         pProjectile->setUp(pDebrisUnit->getUp());
         pProjectile->setCenterOffset(-pPhysicsObject->getCenterOffset());

         BVector velocity;
         pPhysicsObject->getLinearVelocity(velocity);

         // add in explosion velocity
         BVector dirFromExplosion = pProjectile->getPosition() - pBallObject->getPosition();
         dirFromExplosion.normalize();
         velocity += (dirFromExplosion * mExplosionForceOnDebris);

         pProjectile->setVelocity(velocity);
         pProjectile->setGravity(gDatabase.getProjectileGravity());

         pProjectile->addAttachment(mPickupAttachmentProtoID);

         pDebrisUnit->kill(true);
      }
   }

   // throw units, scale based off of damage bank
   float throwScalar = clampedDamageBank / mMaxPossibleExplosionDamageBank;
   if (mThrowUnitsOnExplosion && throwScalar > mMinDamageBankPercentToThrow)
   {
      BEntityIDArray results(0, cMaxUnitArraySize);       
      BUnitQuery query(pBallObject->getPosition(), mpExplodeProtoAction->getAOERadius(), true);
      gWorld->getSquadsInArea(&query, &results, false);

      uint numSquads = (uint)results.getNumber();
      BSquad* pSquad = NULL;
      for (uint i = 0; i < numSquads; ++i)
      {
         pSquad = gWorld->getSquad(results[i]);
         if (!pSquad)
            continue;

         const BEntityIDArray& children = pSquad->getChildList();
         uint numChildren = children.getSize();
         BUnit* pUnit = NULL;
         for (uint j = 0; j < numChildren; ++j)
         {
            pUnit = gWorld->getUnit(children[j]);
            if (!pUnit)
               continue;

            if (pUnit->getID() == leaderId || pUnit->getID() == pBallObject->getID())
               continue;

            pUnit->throwUnit(pBallObject->getID(), mpExplodeProtoAction->getID(), throwScalar);
         }
      }
   }

   gWorld->getWorldSoundManager()->addSound(pBallObject->getPosition(), mExplodeSound, true, cInvalidCueIndex, true, true);

   mExplodeCooldownLeft = mExplodeTime;
   setGravityBallState(BWaveGravityBall::cExploding);
}


//==============================================================================
//==============================================================================
void BPowerWave::setGravityBallState(BWaveGravityBall::EState newState)
{
   if (mRealGravityBall.getState() == newState)
      return;

   BPowerUserWave* pWavePowerUser = getPowerUser();
   if(pWavePowerUser)
      pWavePowerUser->onGravityBallStateSet(newState);

   mRealGravityBall.setState(newState);
}

//==============================================================================
//==============================================================================
BPowerUserWave* BPowerWave::getPowerUser() const
{
   return static_cast<BPowerUserWave*>(BPowerHelper::getPowerUser(*this));
}

//==============================================================================
//==============================================================================
float BPowerWave::getBallSpeed() const
{
   if (mRealGravityBall.isPulling())
      return mMaxBallSpeedPulling;
   else if (mRealGravityBall.getState() == BWaveGravityBall::cStagnant)
      return mMaxBallSpeedStagnant;
   else 
      return 0.0f;
}

//==============================================================================
//==============================================================================
void BPowerWave::ripPartOffUnit(BUnit& unit)
{
   BEntityID thrownPart = unit.throwRandomPartImpactPoint();
   BUnit* pThrownPart = gWorld->getUnit(thrownPart);
   grabFakeObject(pThrownPart);
}

//==============================================================================
//==============================================================================
bool BPowerWave::captureUnit(BUnit& unit)
{
   // return true if we did capture the unit and don't care about it anymore

   // first, grab any thrown parts we can 
   const BDamageTracker* pDamageTracker = unit.getDamageTracker();
   if (pDamageTracker)
   {
      const BEntityIDArray& recentParts = pDamageTracker->getRecentlyThrownParts();
      for (uint i = 0; i < recentParts.getSize(); ++i)
         queueAddObject(gWorld->getObject(recentParts[i]));
   }

   // we only care about dead units, or ones under our health threshold
   if (unit.getFlagAlive() && unit.getHitpoints() > mHealthToCapture)
      return false;

   // if we are a physics replacement, add ourselves and be done
   if (unit.getFlagIsPhysicsReplacement())
   {
      queueAddObject(&unit);
      return true;
   }

   // if we have an attachment we can throw, throw one
   // also, don't do this for buildings, which has some large attachments, 
   // we only care about the parts they may throw
   bool hasAttachments = unit.hasThrowableAttachments();
   if (hasAttachments && !unit.isType(gDatabase.getOTIDBuilding()))
   {
      BEntityIDArray attachments;
      unit.throwAttachments(&unit, cOriginVector, mRipAttachmentChancePulling, &attachments, 1);
      for (uint i = 0; i < attachments.getSize(); ++i)
         queueAddObject(gWorld->getObject(attachments[i]));
   }

   // no attachments left to throw, do the last thing
   // create a physics replacement if we can
   BObject* pPhysReplacement = NULL;
   if (unit.getProtoObject() && unit.getProtoObject()->getPhysicsReplacementInfoID() != -1)
   {
      pPhysReplacement = unit.createPhysicsReplacement();
      queueAddObject(pPhysReplacement);
   }
   if (pPhysReplacement)
   {
      unit.setFlagKilledByLeaderPower(true);
      unit.kill(true);
      return true;
   }
   else if (unit.getFlagAlive())
   {
      unit.setFlagKilledByLeaderPower(true);
      unit.kill(false);
   }

   // if there are attachments still, we return false, meaning we keep processing the unit
   // else we return true, since we are done with it
   return !hasAttachments;
}

//==============================================================================
//==============================================================================
void BPowerWave::hitLightning(BSquad& squad)
{
   BASSERT(mpLightningProtoAction);

   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   BUnit* pOwnerLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   BEntityID leaderId = (pOwnerLeaderUnit) ? pOwnerLeaderUnit->getID() : cInvalidObjectID;

   BObject* pBallObject = mRealGravityBall.getBallObject();

   // hit the unit with a bolt of lightning
   BObjectCreateParms parms;
   parms.mPlayerID = mPlayerID;
   parms.mProtoObjectID = mpLightningProtoAction->getProjectileID();
   parms.mPosition = pBallObject->getPosition();
   float dps = mpLightningProtoAction->getDamagePerSecond();
   BVector strikeOffset(getRandRangeFloat(cSimRand, -1.0f, 1.0f), 0.0f, getRandRangeFloat(cSimRand, -1.0f, 1.0f));
   BUnit* pLeaderUnit = squad.getLeaderUnit();
   BVector strikeLocationWorld;
   if (pLeaderUnit)
   {
      strikeOffset *= Math::Min(pLeaderUnit->getObstructionRadiusX(), pLeaderUnit->getObstructionRadiusZ());
      strikeLocationWorld = pLeaderUnit->getPosition() + strikeOffset;
      gWorld->launchProjectile(parms, strikeOffset, XMVectorZero(), XMVectorZero(), dps, mpLightningProtoAction, mpLightningProtoAction, leaderId, pLeaderUnit, -1, true, true, true);
   }
   else
   {
      strikeOffset *= Math::Min(squad.getObstructionRadiusX(), squad.getObstructionRadiusZ());
      strikeLocationWorld = squad.getPosition() + strikeOffset;
      gWorld->launchProjectile(parms, strikeLocationWorld, XMVectorZero(), XMVectorZero(), dps, mpLightningProtoAction, mpLightningProtoAction, leaderId, NULL, -1, true, true, true);
   }

   // see if we also need to launch a beam visual
   if (mLightningBeamVisualProtoID != cInvalidObjectID)
   {
      // Create beam projectile
      parms.mProtoObjectID = mLightningBeamVisualProtoID;
      parms.mStartBuilt = true;
      parms.mNoTieToGround = true;
      parms.mForward = strikeLocationWorld; // Used to set the target location when creating the beam

      BProjectile* pProjectile = gWorld->createProjectile(parms);
      if (pProjectile)
      {
         pProjectile->setFlagBeam(true);
         pProjectile->setParentID(pBallObject->getID());
         pProjectile->setDamage(0.0f);
         pProjectile->setTargetLocation(strikeLocationWorld);
         pProjectile->setFlagFromLeaderPower(true);
      }
   }

   // do a nudge towards the ball
   long rand = getRand(cSimRand);
   if (pLeaderUnit && rand % 255 < mNudgeChancePulling)
      nudgeUnit(*pLeaderUnit, strikeLocationWorld);
}

//==============================================================================
//==============================================================================
void BPowerWave::nudgeUnit(BUnit& unit, const BVector& strikeLocation)
{
   if (!unit.getPhysicsObject())
      return;

   BObject* pBallObject = mRealGravityBall.getBallObject();

   //unit.throwUnit(pBallObject->getID());

   BVector nudgeLocation;
   unit.getPhysicsObject()->getCenterOfMassLocation(nudgeLocation);
   BVector dirToNudge = (strikeLocation - nudgeLocation);
   dirToNudge.normalize();
   if (unit.isPhysicsAircraft())
      nudgeLocation += dirToNudge;
   else
      nudgeLocation += (dirToNudge * unit.getObstructionRadius());

   BVector dirFromBall = unit.getPosition() - pBallObject->getPosition();
   dirFromBall.normalize();
   BVector impulseForce = -dirFromBall;
   impulseForce.normalize();
   impulseForce *= mNudgeStrength;
   impulseForce *= unit.getPhysicsObject()->getMass();

   if (unit.isPhysicsAircraft())
      impulseForce *= cNudgeScalarAir;

   unit.getPhysicsObject()->applyPointImpulse(impulseForce, strikeLocation);
}

//==============================================================================
//==============================================================================
void BPowerWave::queueAddObject(BObject* pObject)
{
   if (!pObject)
      return;

   // if we're already full, bail
   if (mCapturedUnits.getNumItems() >= (uint)mMaxCapturedObjects)
      return;

   // we can't grab items we already have
   if (mCapturedUnits.contains(pObject->getID()))
      return;

   BPhysicsObject* pPhysicsObject = pObject->getPhysicsObject();
   if (!pPhysicsObject)
      return;

   // check physics for entire unit
   if(!physicsValidForPulling(pPhysicsObject) && !pObject->getProtoObject()->isType(gDatabase.getOTIDInfantry()))
      return;

   // for clamshell units, set some damping values
   BPhysicsInfo* pInfo = gPhysicsInfoManager.get(pPhysicsObject->getInfoID());
   if(pInfo && pInfo->isClamshell())
   {
      BClamshellPhysicsObject* pCPO = static_cast<BClamshellPhysicsObject*>(pPhysicsObject);
      if(pCPO)
         pCPO->setLinearDamping(0.1f);
   }

   for (uint i = 0; i < mQueuedPickupObjects.getSize(); ++i)
      if (mQueuedPickupObjects[i].ObjectID == pObject->getID())
         return;

   // we know we're picking this up - make sure it doesn't have a lifespan
   pObject->enableLifespan(false);
   
   // we really don't have a queue type in this engine? 
   float lastAddTime = (mQueuedPickupObjects.empty()) ? mElapsed : mQueuedPickupObjects.back().AddTime;
   
   mQueuedPickupObjects.grow();
   BQueuedObject& queuedObject = mQueuedPickupObjects.back();
   queuedObject.ObjectID = pObject->getID();
   queuedObject.AddTime = lastAddTime + getRandRangeFloat(cSimRand, 0.0f, mPickupObjectRate);
}

//==============================================================================
//==============================================================================
void BPowerWave::updateQueuedObjects()
{
   uint i = 0;
   uint size = mQueuedPickupObjects.getSize();
   for ( ; i < size; ++i)
   {
      // if we found one that's not ready, we're done
      if (mQueuedPickupObjects[i].AddTime > mElapsed)
         break;

      addObjectToBall(gWorld->getObject(mQueuedPickupObjects[i].ObjectID));
   }
   mQueuedPickupObjects.erase(0, i);
}

//==============================================================================
//==============================================================================
void BPowerWave::validateCapturedObjects()
{
   // clear out any invalid captured objects
   uint i = 0;
   while (i < mCapturedUnits.getNumItems())
   {
      if (!gWorld->getUnit(mCapturedUnits.getItem(i)))
         mCapturedUnits.remove(i);
      else 
         ++i;
   }
}

//==============================================================================
//==============================================================================
void BPowerWave::addObjectToBall(BObject* pObject)
{
   if (!pObject)
      return;

   // we can't grab items we already have
   if (mCapturedUnits.contains(pObject->getID()))
      return;

   BObject* pBallObject = mRealGravityBall.getBallObject();
   BPhysicsObject* pPhysicsObject = pObject->getPhysicsObject();
   BUnit* pNewUnit = pObject->getUnit();

   if (pPhysicsObject)
   {
      // spawn an attachment
      BVector centerOfMass;
      pPhysicsObject->getCenterOfMassLocation(centerOfMass);
      BEntityID attachmentID = gWorld->createEntity(mPickupAttachmentProtoID, false, getPlayerID(), centerOfMass, pObject->getForward(), pObject->getRight(), true);
      BObject* pAttachmentObject = gWorld->getObject(attachmentID);
      if (pAttachmentObject)
         pObject->addAttachment(pAttachmentObject, -1, -1, false, true);

      // make sure the physics objects is active, and that the unit action physics is in the right state
      // can this ever even be NULL, since our physics objects are all units now? 
      if (pNewUnit)
      {
         BUnitActionPhysics* pPhysicsAction = reinterpret_cast<BUnitActionPhysics*>(pNewUnit->getActionByType(BAction::cActionTypeUnitPhysics));
         if (pPhysicsAction)
         {
            pPhysicsAction->setState(BAction::cStateNone);
            pPhysicsAction->setFlagCompleteOnInactivePhysics(false);
         }
      }
      if (!pPhysicsObject->isActive())
         pPhysicsObject->forceActivate();

      // the object should have no obstruction 
      pObject->deleteObstruction();

      // set the player id and the color id correctly
      BPlayerID colorPlayer = pObject->getColorPlayerID();
      pObject->changeOwner(mPlayerID);
      pObject->setColorPlayerID(colorPlayer);
      pObject->setFlagGrayMapDopples(false);
      pObject->setFlagDopples(false);

      pPhysicsObject->setAngularDamping(mDebrisAngularDamping);

      // determine whether we need some extra bodies added to the gravity ball action
      hkArray<hkpEntity*> additionalBodies;
      BPhysicsInfo* pInfo = gPhysicsInfoManager.get(pPhysicsObject->getInfoID());
      if(pInfo && pInfo->isClamshell())
      {
         BClamshellPhysicsObject* pCPO = static_cast<BClamshellPhysicsObject*>(pPhysicsObject);
         if(pCPO)
         {      
            additionalBodies.pushBack(pCPO->getUpperBody()->getRigidBody());
            additionalBodies.pushBack(pCPO->getLowerBody()->getRigidBody());
         }
      }

      // set up gravity ball action
      BPhysicsGravityBallPullAction* pGravAction = new BPhysicsGravityBallPullAction(pPhysicsObject->getRigidBody(), pBallObject, additionalBodies);
      pGravAction->setRestLength(mCapturedSpringRestLength + mCapturedUnits.getNumItems() * mCapturedRadialSpacing);
      pGravAction->setStrength(mCapturedSpringStrength);
      pGravAction->setDamping(mCapturedSpringDampening);
      pGravAction->setMinLateralSpeed(mCapturedMinLateralSpeed);
      pGravAction->setOnCompression(false);
      pGravAction->setOnExtension(true);
      gWorld->getPhysicsWorld()->getHavokWorld()->addAction(pGravAction);
      pGravAction->removeReference();


      mCapturedUnits.add(pObject->getID());

      // try and get some random lateral movement as well
      BVector ballDir = pBallObject->getPosition() - pObject->getPosition();
      ballDir.y = 0.0f;
      ballDir.normalize();
      BVector ballLat = ballDir.cross(cYAxisVector);
      ballLat.x *= getRandRangeFloat(cSimRand, -mInitialLateralPullStrength, mInitialLateralPullStrength) * pPhysicsObject->getMass();
      ballLat.y = getRandRangeFloat(cSimRand, 0.0f, mInitialLateralPullStrength) * pPhysicsObject->getMass();
      ballLat.z *= getRandRangeFloat(cSimRand, -mInitialLateralPullStrength, mInitialLateralPullStrength) * pPhysicsObject->getMass();
      pPhysicsObject->applyPointImpulse(ballLat, pObject->getPosition() + ballDir);
   }

   BPowerUserWave* pPowerUser = getPowerUser();
   if (pPowerUser)
      pPowerUser->onPickupObject();
}

//==============================================================================
//==============================================================================
void BPowerWave::grabFakeObject(BObject* pObject)
{
   if (!pObject)
      return;

   BObject* pBallObject = mRealGravityBall.getBallObject();
   BPhysicsObject* pPhysicsObject = pObject->getPhysicsObject();
   BUnit* pNewUnit = pObject->getUnit();

   static const float cFakeObjectLifespan = 0.3f;
   static const float cFakeObjectAlphaFade = 0.45f;

   pObject->setLifespan(static_cast<DWORD>(1000.0f * cFakeObjectLifespan));
   if (cFakeObjectLifespan > cFloatCompareEpsilon)
      pObject->enableAlphaFade(true, cFakeObjectAlphaFade);
   pObject->setFlagNoUpdate(false);

   if (pPhysicsObject)
   {
      // make sure the physics objects is active, and that the unit action physics is in the right state
      // can this ever even be NULL, since our physics objects are all units now? 
      if (pNewUnit)
      {
         BUnitActionPhysics* pPhysicsAction = reinterpret_cast<BUnitActionPhysics*>(pNewUnit->getActionByType(BAction::cActionTypeUnitPhysics));
         if (pPhysicsAction)
         {
            pPhysicsAction->setState(BAction::cStateNone);
            pPhysicsAction->setFlagCompleteOnInactivePhysics(true);
         }
      }
      if (!pPhysicsObject->isActive())
         pPhysicsObject->forceActivate();

      // the object should have no obstruction 
      pObject->deleteObstruction();

      // set the player id and the color id correctly
      BPlayerID colorPlayer = pObject->getColorPlayerID();
      pObject->changeOwner(mPlayerID);
      pObject->setColorPlayerID(colorPlayer);
      pObject->setFlagGrayMapDopples(false);
      pObject->setFlagDopples(false);

      pPhysicsObject->setAngularDamping(mDebrisAngularDamping);

      // set up gravity ball action
      hkArray<hkpEntity*> additionalBodies;
      BPhysicsGravityBallPullAction* pGravAction = new BPhysicsGravityBallPullAction(pPhysicsObject->getRigidBody(), pBallObject, additionalBodies);
      pGravAction->setRestLength(mCapturedSpringRestLength + mCapturedUnits.getNumItems() * mCapturedRadialSpacing);
      pGravAction->setStrength(mCapturedSpringStrength);
      pGravAction->setDamping(mCapturedSpringDampening);
      pGravAction->setMinLateralSpeed(mCapturedMinLateralSpeed);
      pGravAction->setOnCompression(false);
      pGravAction->setOnExtension(true);
      gWorld->getPhysicsWorld()->getHavokWorld()->addAction(pGravAction);
      pGravAction->removeReference();
   }
}

//==============================================================================
//==============================================================================
void BPowerWave::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   // check to see if something we care about got replaced with a physics 
   // replacement if so, add that to our list of things to check
   if (eventType == BEntity::cEventUnitPhysicsReplacement)
   {
      BEntityID newId(data);
      if (newId != cInvalidObjectID)
      {
         mUnitsToPull.uniqueAdd(newId);
         mUnitsToPull.remove(senderID);
      }
   }
   else if (eventType == BEntity::cEventUnitAttachmentThrown)
   {
      BEntityID newId(data);
      if (newId != cInvalidObjectID)
         mUnitsToPull.uniqueAdd(newId);
   }
}


//==============================================================================
//==============================================================================
bool BPowerWave::physicsValidForPulling(const BPhysicsObject* pPO)
{
   if(!pPO)
      return true;

   const hkpShape* pShape = pPO->getRigidBody()->getCollidable()->getShape();
   BVector halfExtents(0,0,0);
   if(pShape->getType() == HK_SHAPE_BOX)
   {
      hkpBoxShape* pBoxShape = (hkpBoxShape*)pShape;
      BPhysics::convertModelSpacePoint(pBoxShape->getHalfExtents(), halfExtents);
      float sum = halfExtents.x + halfExtents.y + halfExtents.z;
      if(sum > 1.0f && sum < 8.0f)
      {
         //gConsole.output(cMsgDebug, "will add attachment %s with half extents %.2f %.2f %.2f", pEntity->getName()->getPtr(), halfExtents.x, halfExtents.y, halfExtents.z);
         return true;
      }
      else
      {
         //gConsole.output(cMsgDebug, "not adding attachment %s with half extents %.2f %.2f %.2f", pEntity->getName()->getPtr(), halfExtents.x, halfExtents.y, halfExtents.z);
         return false;
      }
   }
   else
   {
      //gConsole.output(cMsgDebug, "not adding attachment %s", pEntity->getName()->getPtr());
      return false;
   }

}

//==============================================================================
//==============================================================================
bool BPowerUserWave::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BWaveGravityBall, mFakeGravityBall);
   GFWRITEVECTOR(pStream, mHorizontalMoveInputDir);
   GFWRITEVECTOR(pStream, mVerticalMoveInputDir);
   GFWRITEVECTOR(pStream, mLastUpdatePos);
   GFWRITEVECTOR(pStream, mCameraFocusPoint);
   GFWRITEVAR(pStream, DWORD, mTimestampNextCommand);
   GFWRITEVAR(pStream, float, mTimeUntilHint);
   GFWRITEVAR(pStream, DWORD, mCommandInterval);
   GFWRITEVAR(pStream, float, mMinBallDistance);
   GFWRITEVAR(pStream, float, mMaxBallDistance);
   GFWRITEVAR(pStream, float, mMinBallHeight);
   GFWRITEVAR(pStream, float, mMaxBallHeight);
   GFWRITEVAR(pStream, float, mMaxBallSpeedStagnant);
   GFWRITEVAR(pStream, float, mMaxBallSpeedPulling);
   GFWRITEVAR(pStream, float, mCameraDistance);
   GFWRITEVAR(pStream, float, mCameraHeight);
   GFWRITEVAR(pStream, float, mCameraHoverPointDistance);
   GFWRITEVAR(pStream, float, mCameraMaxBallAngle);
   GFWRITEVAR(pStream, float, mPullingRange);
   GFWRITEVAR(pStream, float, mPickupShakeDuration);
   GFWRITEVAR(pStream, float, mPickupRumbleShakeStrength);
   GFWRITEVAR(pStream, float, mPickupCameraShakeStrength);
   GFWRITEVAR(pStream, float, mExplodeTime);
   GFWRITEVAR(pStream, float, mDelayShutdownTimeLeft);
   GFWRITEBITBOOL(pStream, mHintShown);
   GFWRITEBITBOOL(pStream, mHintCompleted);
   GFWRITEBITBOOL(pStream, mShuttingDown);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserWave::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BWaveGravityBall, mFakeGravityBall);
   GFREADVECTOR(pStream, mHorizontalMoveInputDir);
   GFREADVECTOR(pStream, mVerticalMoveInputDir);
   GFREADVECTOR(pStream, mLastUpdatePos);
   GFREADVECTOR(pStream, mCameraFocusPoint);
   GFREADVAR(pStream, DWORD, mTimestampNextCommand);
   GFREADVAR(pStream, float, mTimeUntilHint);
   GFREADVAR(pStream, DWORD, mCommandInterval);
   GFREADVAR(pStream, float, mMinBallDistance);
   GFREADVAR(pStream, float, mMaxBallDistance);
   GFREADVAR(pStream, float, mMinBallHeight);
   GFREADVAR(pStream, float, mMaxBallHeight);
   GFREADVAR(pStream, float, mMaxBallSpeedStagnant);
   GFREADVAR(pStream, float, mMaxBallSpeedPulling);
   GFREADVAR(pStream, float, mCameraDistance);
   GFREADVAR(pStream, float, mCameraHeight);
   GFREADVAR(pStream, float, mCameraHoverPointDistance);
   GFREADVAR(pStream, float, mCameraMaxBallAngle);
   GFREADVAR(pStream, float, mPullingRange);
   GFREADVAR(pStream, float, mPickupShakeDuration);
   GFREADVAR(pStream, float, mPickupRumbleShakeStrength);
   GFREADVAR(pStream, float, mPickupCameraShakeStrength);
   if (BPowerUser::mGameFileVersion >= 9)
   {
      GFREADVAR(pStream, float, mExplodeTime);
      GFREADVAR(pStream, float, mDelayShutdownTimeLeft);
   }
   GFREADBITBOOL(pStream, mHintShown);
   GFREADBITBOOL(pStream, mHintCompleted);
   if (BPowerUser::mGameFileVersion >= 9)
   {
      GFREADBITBOOL(pStream, mShuttingDown);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerWave::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, double, mNextTickTime);
   GFWRITEVAR(pStream, BWaveGravityBall, mRealGravityBall);
   GFWRITEVECTOR(pStream, mDesiredBallPosition);

   // mCapturedUnits
   uint count = mCapturedUnits.getNumItems();
   GFWRITEVAR(pStream, uint, count);
   GFVERIFYCOUNT(count, 400);
   for (uint i=0; i<count; i++)
      GFWRITEVAL(pStream, BEntityID, mCapturedUnits.getItem(i));

   GFWRITEVAR(pStream, float, mExplodeCooldownLeft);
   GFWRITEARRAY(pStream, BEntityID, mUnitsToPull, uint8, 200);
   GFWRITEARRAY(pStream, BQueuedObject, mQueuedPickupObjects, uint8, 200);
   GFWRITECLASS(pStream, saveType, mCostPerTick);
   GFWRITEVAR(pStream, float, mTickLength);
   GFWRITEVAR(pStream, BProtoObjectID, mBallProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mLightningProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mLightningBeamVisualProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mDebrisProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mExplodeProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mPickupAttachmentProtoID);
   GFWRITEVAR(pStream, float, mAudioReactionTimer);
   GFWRITEVAR(pStream, uint, mLeaderAnimOrderID);
   GFWRITEVAR(pStream, float, mMaxBallSpeedStagnant);
   GFWRITEVAR(pStream, float, mMaxBallSpeedPulling);
   GFWRITEVAR(pStream, float, mExplodeTime);
   GFWRITEVAR(pStream, float, mPullingRange);
   GFWRITEVAR(pStream, float, mExplosionForceOnDebris);
   GFWRITEVAR(pStream, float, mHealthToCapture);
   GFWRITEVAR(pStream, float, mNudgeStrength);
   GFWRITEVAR(pStream, float, mInitialLateralPullStrength);
   GFWRITEVAR(pStream, float, mCapturedRadialSpacing);
   GFWRITEVAR(pStream, float, mCapturedSpringStrength);
   GFWRITEVAR(pStream, float, mCapturedSpringDampening);
   GFWRITEVAR(pStream, float, mCapturedSpringRestLength);
   GFWRITEVAR(pStream, float, mCapturedMinLateralSpeed);
   GFWRITEVAR(pStream, float, mRipAttachmentChancePulling);
   GFWRITEVAR(pStream, float, mPickupObjectRate);
   GFWRITEVAR(pStream, float, mDebrisAngularDamping);
   GFWRITEVAR(pStream, float, mCurrentExplosionDamageBank);
   GFWRITEVAR(pStream, float, mMaxPossibleExplosionDamageBank);
   GFWRITEVAR(pStream, float, mMaxExplosionDamageBankPerCaptured);
   GFWRITEVAR(pStream, float, mExplosionDamageBankPerTick);
   GFWRITEVAR(pStream, DWORD, mCommandInterval);
   GFWRITEVAR(pStream, float, mMinBallDistance);
   GFWRITEVAR(pStream, float, mMaxBallDistance);
   GFWRITEVAR(pStream, int, mLightningPerTick);
   GFWRITEVAR(pStream, int, mMaxCapturedObjects);
   GFWRITEVAR(pStream, byte, mNudgeChancePulling);
   GFWRITEVAR(pStream, byte, mThrowPartChancePulling);
   GFWRITEVAR(pStream, byte, mLightningChancePulling);
   GFWRITEVAR(pStream, BCueIndex, mExplodeSound);
   GFWRITEVAR(pStream, float, mMinDamageBankPercentToThrow);
   GFWRITEARRAY(pStream, BTeamID, mRevealedTeamIDs, uint8, 16);
   GFWRITEPROTOACTIONPTR(pStream, mpExplodeProtoAction);
   GFWRITEPROTOACTIONPTR(pStream, mpLightningProtoAction);
   GFWRITEPROTOACTIONPTR(pStream, mpDebrisProtoAction);
   GFWRITEBITBOOL(pStream, mCompletedInitialization);
   GFWRITEBITBOOL(pStream, mThrowUnitsOnExplosion);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerWave::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, double, mNextTickTime);
   GFREADVAR(pStream, BWaveGravityBall, mRealGravityBall);
   GFREADVECTOR(pStream, mDesiredBallPosition);

   // mCapturedUnits
   uint count;
   GFREADVAR(pStream, uint, count);
   GFVERIFYCOUNT(count, 400);
   for (uint i=0; i<count; i++)
   {
      BEntityID entityID;
      GFREADVAR(pStream, BEntityID, entityID);
      mCapturedUnits.add(entityID);
   }

   GFREADVAR(pStream, float, mExplodeCooldownLeft);
   GFREADARRAY(pStream, BEntityID, mUnitsToPull, uint8, 200);
   GFREADARRAY(pStream, BQueuedObject, mQueuedPickupObjects, uint8, 200);
   GFREADCLASS(pStream, saveType, mCostPerTick);
   GFREADVAR(pStream, float, mTickLength);
   GFREADVAR(pStream, BProtoObjectID, mBallProtoID);
   GFREADVAR(pStream, BProtoObjectID, mLightningProtoID);
   GFREADVAR(pStream, BProtoObjectID, mLightningBeamVisualProtoID);
   GFREADVAR(pStream, BProtoObjectID, mDebrisProtoID);
   GFREADVAR(pStream, BProtoObjectID, mExplodeProtoID);
   GFREADVAR(pStream, BProtoObjectID, mPickupAttachmentProtoID);
   GFREADVAR(pStream, float, mAudioReactionTimer);
   GFREADVAR(pStream, uint, mLeaderAnimOrderID);
   GFREADVAR(pStream, float, mMaxBallSpeedStagnant);
   GFREADVAR(pStream, float, mMaxBallSpeedPulling);
   GFREADVAR(pStream, float, mExplodeTime);
   GFREADVAR(pStream, float, mPullingRange);
   GFREADVAR(pStream, float, mExplosionForceOnDebris);
   GFREADVAR(pStream, float, mHealthToCapture);
   GFREADVAR(pStream, float, mNudgeStrength);
   GFREADVAR(pStream, float, mInitialLateralPullStrength);
   GFREADVAR(pStream, float, mCapturedRadialSpacing);
   GFREADVAR(pStream, float, mCapturedSpringStrength);
   GFREADVAR(pStream, float, mCapturedSpringDampening);
   GFREADVAR(pStream, float, mCapturedSpringRestLength);
   GFREADVAR(pStream, float, mCapturedMinLateralSpeed);
   GFREADVAR(pStream, float, mRipAttachmentChancePulling);
   GFREADVAR(pStream, float, mPickupObjectRate);
   GFREADVAR(pStream, float, mDebrisAngularDamping);
   GFREADVAR(pStream, float, mCurrentExplosionDamageBank);
   GFREADVAR(pStream, float, mMaxPossibleExplosionDamageBank);
   GFREADVAR(pStream, float, mMaxExplosionDamageBankPerCaptured);
   GFREADVAR(pStream, float, mExplosionDamageBankPerTick);
   if (BPower::mGameFileVersion >= 5)
   {
      GFREADVAR(pStream, DWORD, mCommandInterval);
      GFREADVAR(pStream, float, mMinBallDistance);
      GFREADVAR(pStream, float, mMaxBallDistance);
   }
   else
   {
      mCommandInterval = 200;
      mMinBallDistance = 10.0f;
      mMaxBallDistance = 65.0f;
   }
   GFREADVAR(pStream, int, mLightningPerTick);
   GFREADVAR(pStream, int, mMaxCapturedObjects);
   GFREADVAR(pStream, byte, mNudgeChancePulling);
   GFREADVAR(pStream, byte, mThrowPartChancePulling);
   GFREADVAR(pStream, byte, mLightningChancePulling);
   if (BPower::mGameFileVersion >= 15)
   {
      GFREADVAR(pStream, BCueIndex, mExplodeSound);
   }
   if (BPower::mGameFileVersion >=17)
   {
      GFREADVAR(pStream, float, mMinDamageBankPercentToThrow);
   }
   if (BPower::mGameFileVersion >= 16)
   {
      GFREADARRAY(pStream, BTeamID, mRevealedTeamIDs, uint8, 16);
   }
   GFREADPROTOACTIONPTR(pStream, mpExplodeProtoAction);
   GFREADPROTOACTIONPTR(pStream, mpLightningProtoAction);
   GFREADPROTOACTIONPTR(pStream, mpDebrisProtoAction);
   GFREADBITBOOL(pStream, mCompletedInitialization);
   if (BPower::mGameFileVersion >= 13)
   {
      GFREADBITBOOL(pStream, mThrowUnitsOnExplosion);
   }
   gSaveGame.remapProtoObjectID(mBallProtoID);
   gSaveGame.remapProtoObjectID(mLightningProtoID);
   gSaveGame.remapProtoObjectID(mLightningBeamVisualProtoID);
   gSaveGame.remapProtoObjectID(mDebrisProtoID);
   gSaveGame.remapProtoObjectID(mExplodeProtoID);
   gSaveGame.remapProtoObjectID(mPickupAttachmentProtoID);

   if (!mFlagDestroy)
   {
      BUnit* pUnit = NULL;
      for (uint i = 0; i < mUnitsToPull.getSize(); ++i)
      {
         pUnit = gWorld->getUnit(mUnitsToPull[i]);
         if (pUnit)
            pUnit->addEventListener(this);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerWave::savePtr(BStream* pStream) const
{
   GFWRITEVAR(pStream, BPowerID, mID);
   return true;
}
