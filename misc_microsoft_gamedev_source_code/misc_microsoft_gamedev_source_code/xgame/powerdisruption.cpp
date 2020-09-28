//==============================================================================
// powerDisruption.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "commandmanager.h"
#include "commands.h"
#include "powerDisruption.h"
#include "protopower.h"
#include "simhelper.h"
#include "uigame.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "soundmanager.h"
#include "powermanager.h"
#include "worldsoundmanager.h"

//#define CONSOLE_OUTPUT

//==============================================================================
//==============================================================================
bool BPowerUserDisruption::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mFlagIssueCommandOnCancel=false;
   mLOSMode = BWorld::cCPLOSFullVisible;
   mHudSounds.clear();
   mDisruptionObjectProtoID = cInvalidProtoObjectID;

   //-------------------------------------------------------
   // Handle all possible cases where the init should fail
   //-------------------------------------------------------   
   // No user
   if (!pUser)
      return (false);

   // User is locked.
   if(pUser->isUserLocked())
      return false;

   // No user player
   BPlayer* pUserPlayer = pUser->getPlayer();
   if (!pUserPlayer)
      return (false);

   // No proto power
   const BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);

   // Wrong power type.
   if (pProtoPower->getPowerType() != PowerType::cDisruption)
      return (false);

   // Player cannot use power.
   mFlagNoCost = noCost;
   if (!mFlagNoCost && !pUserPlayer->canUsePower(protoPowerID))
      return (false);

   bool bSuccess = true;

   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "DisruptionObject", mDisruptionObjectProtoID));
   bSuccess = mHudSounds.loadFromProtoPower(pProtoPower, powerLevel);

   bool requiresLOS = true;
   if(pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS)
      mLOSMode = BWorld::cCPLOSDontCare;

   if (!bSuccess)
   {
      BFAIL("Failed to load required data for PowerUserDisruption!");
      shutdown();
      return false;
   }

   // other setup
   mFlagDestroy = false;

   // mark as initialized
   mInitialized = true;

   // Cache off some member variables.
   mpUser = pUser;
   mProtoPowerID = protoPowerID;
   mPowerLevel = powerLevel;
   mOwnerSquadID = ownerSquadID;
   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   setupUser();

   gSoundManager.playCue(mHudSounds.HudUpSound);

   if(mHudSounds.HudStartEnvSound != cInvalidCueIndex)
      gSoundManager.playCue(mHudSounds.HudStartEnvSound);

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserDisruption::shutdown()
{
   // fix up the user's mode
   mpUser->unlockUser();

   // Turn off the power overlay if any.
   BUIContext* pUIContext = mpUser->getUIContext();
   if (pUIContext)
      pUIContext->setPowerOverlayVisible(mProtoPowerID, false);

   //Turn off the hud environment sound
   if(mHudSounds.HudStopEnvSound != cInvalidCueIndex)
      gSoundManager.playCue(mHudSounds.HudStopEnvSound);

   mFlagDestroy = true;

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserDisruption::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserDisruption - is not initialized but is trying to handle input.");
   if (!mInitialized)
      return (false);
   // No user.
   BASSERTM(mpUser, "BPowerUserDisruption - mpUser is NULL.");
   if (!mpUser)
      return (false);
   // No camera.
   BCamera* pCamera = mpUser->getCamera();
   BASSERTM(pCamera, "BPowerUserDisruption - Unable to get the user camera.");
   if (!pCamera)
      return (false);
   // Should never happen?
   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(mpUser->getOption_ControlScheme());
   if (!pInputInterface)
      return (false);

   // We want to update the hover point.
   mpUser->setFlagUpdateHoverPoint(true);

   // Handle camera scrolling.
   if (mpUser->handleInputScrollingAndCamera(port, event, controlType, detail))
      return(true);

   // Handle target selection.
   if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (event == cInputEventControlStart)
      {
         // return if we're trying to cast out of los
         if (!canFire())
         {
            gUI.playCantDoSound();
            return false;
         }

         gUI.playClickSound();
         BPlayerID playerID = mpUser->getPlayerID();

         // Add the command to create the power on the sim side for everyone.
         BPowerCommand* pCommand = (BPowerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandPower);
         if (pCommand)
         {
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setSenders(1, &playerID);
            pCommand->setRecipientType(BCommand::cPlayer);
            pCommand->setType(BPowerCommand::cTypeInvokePower2);
            pCommand->setPowerUserID(mID);
            pCommand->setProtoPowerID(mProtoPowerID);
            pCommand->setPowerLevel(mPowerLevel);
            pCommand->setTargetLocation(mpUser->getHoverPoint());
            pCommand->setSquadID(mOwnerSquadID);
            pCommand->setFlag(BPowerCommand::cNoCost, mFlagNoCost);
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
         }

         gSoundManager.playCue(mHudSounds.HudFireSound);

         shutdown();
      }
      return (true);
   }

   // Did the user just cancel the power?
   if (pInputInterface->isFunctionControl(BInputInterface::cInputClear, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (event == cInputEventControlStart)
      {
         gSoundManager.playCue(mHudSounds.HudAbortSound);

         cancelPower();
      }
      return (true);
   }

   // Didn't handle stuff.
   return (false);
}

//==============================================================================
//==============================================================================
bool BPowerUserDisruption::canFire()
{
   DWORD flags = BWorld::cCPLOSCenterOnly | BWorld::cCPCheckObstructions;
   if(!mpUser->getFlagHaveHoverPoint())
      return false;

   BVector tempVec;
   if(!gWorld->checkPlacement(mDisruptionObjectProtoID,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, flags) ||
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BPowerUserDisruption::renderUI()
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserDisruption - is not initialized but is trying to render UI.");
   if (!mInitialized)
      return;
   // No user.
   BASSERTM(mpUser, "BPowerUserDisruption - mpUser is NULL.");
   if (!mpUser)
      return;

   // Render some UI.
   if (canFire())
      mpUser->renderReticle(BUIGame::cReticlePowerValid);
   else
      mpUser->renderReticle(BUIGame::cReticlePowerInvalid);
}

//==============================================================================
//==============================================================================
void BPowerUserDisruption::setupUser()
{
   BASSERT(mpUser);
   if (!mpUser)
      return;

   // Lock the user.
   mpUser->lockUser(cInvalidTriggerScriptID);
   mpUser->changeMode(BUser::cUserModePower);
   mpUser->setUIPowerRadius(0.0f);

   // Turn on the power overlay if any.
   BUIContext* pUIContext = mpUser->getUIContext();
   if (pUIContext)
   {
      pUIContext->setPowerOverlayVisible(mProtoPowerID, true);
      pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(24795));
   }
}

//==============================================================================
//==============================================================================
bool BPowerDisruption::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mDisruptionObjectID = cInvalidObjectID;
   mDisruptionRadius = 0.0f;
   mDisruptionRadiusSqr = 0.0f;
   mTimeRemainingSec = 0.0f;
   mDisruptionStartTime = 0.0f;
   mDirection = cOriginVector;
   mRight = cOriginVector;
   mBomberProtoID = cInvalidProtoObjectID;
   mDisruptionObjectProtoID = cInvalidProtoObjectID;
   mPulseObjectProtoID = cInvalidProtoObjectID;
   mStrikeObjectProtoID = cInvalidProtoObjectID;
   mBomberData.clear();
   mNextPulseTime = 0.0f;
   mPulseSpacing = 0.0f;
   mNumPulses = 0;
   mPulseSound = cInvalidCueIndex;

#ifdef SYNC_Command
   syncCommandData("BPowerUserDisruption::init playerID", playerID);
   syncCommandData("BPowerUserDisruption::init powerLevel", (DWORD)powerLevel);
   syncCommandData("BPowerUserDisruption::init powerUserID player", powerUserID.getPlayerID());
   syncCommandData("BPowerUserDisruption::init powerUserID powerType", (DWORD)powerUserID.getPowerType());
   syncCommandData("BPowerUserDisruption::init ownerSquadID", ownerSquadID.asLong());
   syncCommandData("BPowerUserDisruption::init targetLocation", targetLocation);
#endif

   // casting variables
   mPlayerID = playerID;
   mPowerUserID = powerUserID;
   mPowerLevel = powerLevel;
   mOwnerID = ownerSquadID;
   mTargetLocation = targetLocation;

   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   mFlagIgnoreAllReqs = ignoreAllReqs;
   bool bSuccess = true;

   bSuccess = (bSuccess && pPlayer != NULL);
#ifdef SYNC_Command
   syncCommandData("BPowerUserDisruption::init pPlayer bSuccess", bSuccess);
#endif

   bSuccess = (bSuccess && pProtoPower != NULL);
#ifdef SYNC_Command
   syncCommandData("BPowerUserDisruption::init pProtoPower bSuccess", bSuccess);
#endif

   if (!mFlagIgnoreAllReqs)
   {
      bSuccess = (bSuccess && pPlayer->canUsePower(mProtoPowerID));
#ifdef SYNC_Command
      syncCommandData("BPowerUserDisruption::init canUserPower bSuccess", bSuccess);
#endif
   }

   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "DisruptionObject", mDisruptionObjectProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "PulseObject", mPulseObjectProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "StrikeObject", mStrikeObjectProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "DisruptionRadius", mDisruptionRadius));
   bSuccess = (bSuccess && mDisruptionRadius > 0.0f);
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "DisruptionTimeSec", mTimeRemainingSec));
   bSuccess = (bSuccess && mTimeRemainingSec > 0.0f);
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "DisruptionStartTime", mDisruptionStartTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "PulseSpacing", mPulseSpacing));
   bSuccess = (bSuccess && pProtoPower->getDataSound(powerLevel, "PulseSound", mPulseSound));

   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "Bomber", mBomberProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberBombTime", mBomberData.BombTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberFlyinDistance", mBomberData.BomberFlyinDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberFlyinHeight", mBomberData.BomberFlyinHeight));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberBombHeight", mBomberData.BomberBombHeight));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberSpeed", mBomberData.BomberSpeed));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberFlyOutTime", mBomberData.FlyoutTime));

   mDisruptionRadiusSqr = (mDisruptionRadius * mDisruptionRadius);

#ifdef SYNC_Command
   syncCommandData("BPowerUserDisruption::init bSuccess", bSuccess);
   syncCommandData("BPowerUserDisruption::init DisruptionObjectProtoID", mDisruptionObjectProtoID);
   syncCommandData("BPowerUserDisruption::init mDisruptionRadius", mDisruptionRadiusSqr);
   syncCommandData("BPowerUserDisruption::init mTimeRemainingSec", mTimeRemainingSec);
#endif

   // Did we succeed on all our setup?
   if (!bSuccess)
   {
      BFAIL("Failed to load required data for PowerDisruption");
      shutdown();
      return (false);
   }
   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   // create the bomber
   mDirection = BVector(getRandRangeFloat(cSimRand, -1.0f, 1.0f), 0.0f, getRandRangeFloat(cSimRand, -1.0f, 1.0f));
   if (mDirection.lengthSquared() <= cFloatCompareEpsilon)
      mDirection = cXAxisVector;
   mDirection.normalize();

   // calc cross product
   mRight.assignCrossProduct(cYAxisVector, mDirection);
   mRight.normalize();

   mBomberData.BomberId = BPowerHelper::createBomber(mBomberProtoID, mBomberData, mTargetLocation, mDirection, mPlayerID);
   mElapsed = 0.0f;

   // Pay the cost now.
   pPlayer->usePower(mProtoPowerID, cInvalidObjectID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

   // All initialized.
   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerDisruption::shutdown()
{
   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if (pUser)
   {
      BPowerUser* pPowerUser = pUser->getPowerUser();
      if (pPowerUser && pPowerUser->getID() == mPowerUserID)
         pPowerUser->shutdown();
   }

   // Kill the green heal thing.
   if(mDisruptionObjectID.isValid())
   {
      BObject* pDisruptionObject = gWorld->getObject(mDisruptionObjectID);
      if (pDisruptionObject)
      {
         pDisruptionObject->setLifespan(2500);
         pDisruptionObject->setAnimationState(BObjectAnimationState::cAnimationStateDeath, cAnimTypeDeath);
         pDisruptionObject->computeAnimation();         
         mDisruptionObjectID = cInvalidObjectID;
      }
   }

   // delete the bomber if we created one
   BObject* pBomberUnit = gWorld->getObject(mBomberData.BomberId);
   if (pBomberUnit)
      pBomberUnit->kill(true);

   mFlagDestroy = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerDisruption::submitInput(BPowerInput powerInput)
{
   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerDisruption::submitInput() received powerInput of unsupported type.");
   return (false);
}


//==============================================================================
//==============================================================================
void BPowerDisruption::update(DWORD currentGameTime, float lastUpdateLength)
{
   // Already marked for destroy.
   if (mFlagDestroy)
      return;

   BPlayerID playerID = mPlayerID;

   // Bomb check.
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pPlayer || !pProtoPower)
   {
      shutdown();
      return;
   }

   mElapsed += lastUpdateLength;

   // manually move the bomber, no worrying about pathing here 
   BPowerHelper::updateBomberPosition(mBomberData, (float)mElapsed, lastUpdateLength);

   // if we haven't dropped the bomb yet, then don't do anything
   if (mElapsed < mBomberData.BombTime)
      return;

   if (mDisruptionObjectID == cInvalidObjectID)
   {
      // create object
      BObjectCreateParms parms;
      parms.mPlayerID = playerID;
      parms.mPosition = mTargetLocation;
      parms.mForward = -mDirection;
      parms.mRight = -mRight;
      parms.mType = BEntity::cClassTypeObject;
      parms.mProtoObjectID = mDisruptionObjectProtoID;
      BObject* pObject = gWorld->createObject(parms);
      if (pObject)
         mDisruptionObjectID = pObject->getID();
   }

   // don't do any work if we're inactive
   if (!isActive())
      return;

   // update pulses
   if (mElapsed > mNextPulseTime)
   {
      BObject* pDisruptionObject = gWorld->getObject(mDisruptionObjectID);
      if (pDisruptionObject)
      {
         pDisruptionObject->addAttachment(mPulseObjectProtoID);
         gWorld->getWorldSoundManager()->addSound(pDisruptionObject->getPosition(), mPulseSound, true, cInvalidCueIndex, true, true);
      }
      ++mNumPulses;
      mNextPulseTime = (float)mElapsed + (mNumPulses * mPulseSpacing);
   }

   mTimeRemainingSec -= lastUpdateLength;

   // If we have no time left, bail
   if (mTimeRemainingSec <= 0)
   {
      shutdown();
      return;
   }
}

//==============================================================================
//==============================================================================
void BPowerDisruption::strikeLocation(const BVector& targetLocation, BUser* pUserCameraControl) const
{
   BObject* pDisruptionObject = gWorld->getObject(mDisruptionObjectID);
   if (!pDisruptionObject)
      return;

   // see if we also need to launch a beam visual
   if (mStrikeObjectProtoID != cInvalidObjectID)
   {
      // Create beam projectile
      BObjectCreateParms parms;
      parms.mPlayerID = mPlayerID;
      parms.mPosition = mTargetLocation + BVector(0.0f, 5.0f, 0.0f);
      parms.mProtoObjectID = mStrikeObjectProtoID;
      parms.mStartBuilt = true;
      parms.mNoTieToGround = true;
      parms.mForward = targetLocation; // Used to set the target location when creating the beam

      BProjectile* pProjectile = gWorld->createProjectile(parms);
      if (pProjectile)
      {
         if (pDisruptionObject->getVisual())
         {
            long boneHandle = pDisruptionObject->getVisual()->getBoneHandle("bip01 light01");
            pProjectile->setParentID(mDisruptionObjectID);
            pProjectile->setBoneHandle(boneHandle);
         }

         pProjectile->setFlagBeam(true);
         pProjectile->setDamage(0.0f);
         pProjectile->setTargetLocation(targetLocation);
         pProjectile->setFlagFromLeaderPower(true);
      }
   }

   if (pUserCameraControl)
   {
      // disable the camera control stuff for now 
/*
      float yaw = pUserCameraControl->getCameraYaw();
      BVector dir = targetLocation - mTargetLocation;
      float targetYaw = dir.getAngleAroundY() * cDegreesPerRadian;

      if (fabs(targetYaw - yaw) < 90.0f)
      {
         pUserCameraControl->setInterpData(0.15, mTargetLocation, &targetYaw);
      }
      else
      {
         targetYaw += 180.0f;
         pUserCameraControl->setInterpData(0.15, targetLocation, &targetYaw);
      }
*/

      // if we got a pUser control, then we need to play the sound as well
      gSoundManager.playCue("play_vog_powerterminated");
   }
}

//==============================================================================
//==============================================================================
bool BPowerUserDisruption::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, long, mLOSMode);
   GFWRITEVAR(pStream, BProtoObjectID, mDisruptionObjectProtoID);
   GFWRITECLASS(pStream, saveType, mHudSounds);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserDisruption::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, long, mLOSMode);
   if (BPowerUser::mGameFileVersion >= 2)
   {
      GFREADVAR(pStream, BProtoObjectID, mDisruptionObjectProtoID);
      gSaveGame.remapProtoObjectID(mDisruptionObjectProtoID);
   }
   else
   {
      mDisruptionObjectProtoID = cInvalidProtoObjectID;
   }
   if (BPowerUser::mGameFileVersion >= 5)
   {
      GFREADCLASS(pStream, saveType, mHudSounds);
   }
   else
   {
      BCueIndex tempCue;
      GFREADVAR(pStream, BCueIndex, tempCue);
      GFREADVAR(pStream, BCueIndex, tempCue);
      GFREADVAR(pStream, BCueIndex, tempCue);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerDisruption::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BEntityID, mDisruptionObjectID);
   GFWRITEVAR(pStream, float, mDisruptionRadius);
   GFWRITEVAR(pStream, float, mDisruptionRadiusSqr);
   GFWRITEVAR(pStream, float, mTimeRemainingSec);
   GFWRITEVAR(pStream, float, mDisruptionStartTime);
   GFWRITEVECTOR(pStream, mDirection);
   GFWRITEVECTOR(pStream, mRight);
   GFWRITEVAR(pStream, BProtoObjectID, mBomberProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mDisruptionObjectProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mPulseObjectProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mStrikeObjectProtoID);
   GFWRITEVAR(pStream, float, mPulseSpacing);
   GFWRITEVAR(pStream, float, mNextPulseTime);
   GFWRITEVAR(pStream, int, mNumPulses);
   GFWRITECLASS(pStream, saveType, mBomberData);
   GFWRITEVAR(pStream, BCueIndex, mPulseSound);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerDisruption::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, BEntityID, mDisruptionObjectID);
   GFREADVAR(pStream, float, mDisruptionRadius);
   GFREADVAR(pStream, float, mDisruptionRadiusSqr);
   GFREADVAR(pStream, float, mTimeRemainingSec);
   GFREADVAR(pStream, float, mDisruptionStartTime);
   GFREADVECTOR(pStream, mDirection);
   GFREADVECTOR(pStream, mRight);
   GFREADVAR(pStream, BProtoObjectID, mBomberProtoID);
   GFREADVAR(pStream, BProtoObjectID, mDisruptionObjectProtoID);
   GFREADVAR(pStream, BProtoObjectID, mPulseObjectProtoID);
   if (BPower::mGameFileVersion >= 3)
   {
      GFREADVAR(pStream, BProtoObjectID, mStrikeObjectProtoID);
   }
   else
   {
      mStrikeObjectProtoID = cInvalidProtoObjectID;
   }
   GFREADVAR(pStream, float, mPulseSpacing);
   GFREADVAR(pStream, float, mNextPulseTime);
   GFREADVAR(pStream, int, mNumPulses);
   GFREADCLASS(pStream, saveType, mBomberData);
   if (BPower::mGameFileVersion >= 8)
   {
      GFREADVAR(pStream, BCueIndex, mPulseSound);
   }
   else
   {
      mPulseSound = cInvalidCueIndex;
   }

   gSaveGame.remapProtoObjectID(mBomberProtoID);
   gSaveGame.remapProtoObjectID(mDisruptionObjectProtoID);
   gSaveGame.remapProtoObjectID(mPulseObjectProtoID);
   gSaveGame.remapProtoObjectID(mStrikeObjectProtoID);
   return true;
}
