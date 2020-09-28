//==============================================================================
// powerorbital.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "powerorbital.h"
#include "user.h"
#include "usermanager.h"
#include "database.h"
#include "objectmanager.h"
#include "world.h"
#include "commandmanager.h"
#include "commands.h"
#include "commandtypes.h"
#include "config.h"
#include "tactic.h"
#include "SimOrderManager.h"
#include "simhelper.h"
#include "simtypes.h"
#include "uigame.h"
#include "protopower.h"
#include "unitquery.h"
#include "physics.h"
#include "damageaction.h"
#include "damagetemplate.h"
#include "damagetemplatemanager.h"
#include "TerrainSimRep.h"
#include "soundmanager.h"
#include "worldsoundmanager.h"
#include "powermanager.h"
#include "powerhelper.h"
#include "generaleventmanager.h"

//#define CONSOLE_OUTPUT

//==============================================================================
//==============================================================================
bool BPowerUserOrbital::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mHudSounds.clear();
   mHelpString.empty();
   mFakeTargettingLaserID = cInvalidObjectID;
   mRealTargettingLaserID = cInvalidObjectID;
   mTargettedSquadID = cInvalidObjectID;
   mShotsRemaining = 0;
   mLastCommandSent = 0.0f;   
   mCommandInterval = 0.0f;
   mLastShotSent = 0.0f;
   mShotInterval = 0.0f;
   mLOSMode = BWorld::cCPLOSFullVisible;
   mFlagLastCameraAutoZoomInstantEnabled = false;
   mFlagLastCameraAutoZoomEnabled = false;
   mFlagLastCameraZoomEnabled = false;
   mFlagLastCameraYawEnabled = false;

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
   if (pProtoPower->getPowerType() != PowerType::cOrbital)
      return (false);
   // Player cannot use power.
   mFlagNoCost = noCost;
   if (!mFlagNoCost && !pUserPlayer->canUsePower(protoPowerID))
      return (false);

   BProtoObjectID targetBeamID;
   if (!pProtoPower->getDataProtoObject(powerLevel, "TargetBeam", targetBeamID))
      return (false);

   int numShots = 0;
   if (!pProtoPower->getDataInt(powerLevel, "NumShots", numShots))
      return (false);
   if (!pProtoPower->getDataFloat(powerLevel, "CommandInterval", mCommandInterval))
      return (false);
   if (!pProtoPower->getDataFloat(powerLevel, "ShotInterval", mShotInterval))
      return (false);
   if (numShots <= 0)
      return (false);

   if (!mHudSounds.loadFromProtoPower(pProtoPower, powerLevel))
      return false;

   bool requiresLOS = true;
   if(pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS)
      mLOSMode = BWorld::cCPLOSDontCare;

   // Halwes - 8/19/2008 - HACK!  Scenario 6 has a special needs version of the orbital bombard with some pesky camera requirements so I need to shutdown
   // some of the camera controls to keep the camera at its initial height.
   mFlagLastCameraAutoZoomInstantEnabled = pUser->getFlagCameraAutoZoomInstantEnabled();
   mFlagLastCameraAutoZoomEnabled = pUser->getFlagCameraAutoZoomEnabled();
   mFlagLastCameraZoomEnabled = pUser->getFlagCameraZoomEnabled();
   mFlagLastCameraYawEnabled = pUser->getFlagCameraYawEnabled();
   if (powerLevel == 66)
   {      
      pUser->setFlagCameraAutoZoomInstantEnabled(false);
      pUser->setFlagCameraAutoZoomEnabled(false);      
      pUser->setFlagCameraZoomEnabled(false);
      pUser->setFlagCameraYawEnabled(false);
   }

   // Initialize some BPowerUserCleansing stuff.
   mShotsRemaining = static_cast<uint>(numShots);

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

   // create fake beam
   BObjectCreateParms parms;
   parms.mPlayerID = mpUser->getPlayerID();
   parms.mPosition = mpUser->getHoverPoint();
   parms.mProtoObjectID = targetBeamID;
   parms.mType = BEntity::cClassTypeObject;
   BObject* fakeObject = gWorld->createObject(parms);
   if(!fakeObject)
      return false;
   if (fakeObject->getVisual())
      fakeObject->getVisual()->setTintColor(gWorld->getPlayerColor(mpUser->getPlayerID(), BWorld::cPlayerColorContextObjects));
   mFakeTargettingLaserID = fakeObject->getID();
   fakeObject->setFlagDontInterpolate(true);

   // Add the command to create the power on the sim side for everyone.
   BPlayerID playerID = mpUser->getPlayerID();
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

   gSoundManager.playCue(mHudSounds.HudUpSound);

   if(mHudSounds.HudStartEnvSound != cInvalidCueIndex)
      gSoundManager.playCue(mHudSounds.HudStartEnvSound);

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserOrbital::shutdown()
{
   // Halwes - 8/19/2008 - HACK!  Scenario 6 has a special needs version of the orbital bombard with some pesky camera requirements so I need to shutdown
   // some of the camera controls to keep the camera at its initial height.
   if (mPowerLevel == 66)
   {      
      mpUser->setFlagCameraAutoZoomInstantEnabled(mFlagLastCameraAutoZoomInstantEnabled);
      mpUser->setFlagCameraAutoZoomEnabled(mFlagLastCameraAutoZoomEnabled);
      mpUser->setFlagCameraZoomEnabled(mFlagLastCameraZoomEnabled);
      mpUser->setFlagCameraYawEnabled(mFlagLastCameraYawEnabled);      
   }

   // fix up the user's mode
   mpUser->unlockUser();

   // Turn off the power overlay if any.
   BUIContext* pUIContext = mpUser->getUIContext();
   if (pUIContext)
      pUIContext->setPowerOverlayVisible(mProtoPowerID, false);

   // kill our fake object
   BObject* pFakeObject = gWorld->getObject(mFakeTargettingLaserID);
   if(pFakeObject)
   {
      pFakeObject->kill(false);
      mFakeTargettingLaserID = cInvalidObjectID;
   }

   BSquad* pTargtettedSquad = gWorld->getSquad(mTargettedSquadID);
   if (pTargtettedSquad)
   {
      pTargtettedSquad->setTargettingSelection(false);
      mTargettedSquadID = cInvalidObjectID;
   }

   //Turn off the hud environment sound
   if(mHudSounds.HudStopEnvSound != cInvalidCueIndex)
      gSoundManager.playCue(mHudSounds.HudStopEnvSound);

   mFlagDestroy = true;

   gGeneralEventManager.eventTrigger(BEventDefinitions::cPowerOrbitalComplete, mpUser->getPlayerID());

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserOrbital::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserOrbital - is not initialized but is trying to handle input.");
   if (!mInitialized)
      return (false);
   // No user.
   BASSERTM(mpUser, "BPowerUserOrbital - mpUser is NULL.");
   if (!mpUser)
      return (false);
   // No camera.
//-- FIXING PREFIX BUG ID 1487
   const BCamera* pCamera = mpUser->getCamera();
//--
   BASSERTM(pCamera, "BPowerUserOrbital - Unable to get the user camera.");
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
      return true;

   // Handle target selection.
   if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (event == cInputEventControlStart)
      {
         // it's possible to get in here with 0 shots remaining if we've already fired our
         // last shot (asych) but are still waiting for confirmation from the sync code before we shut down
         if (mShotsRemaining <= 0)
            return false;

         // return if we're trying to cast out of los
         DWORD flags = BWorld::cCPLOSCenterOnly;
         if(!mpUser->getFlagHaveHoverPoint())
         {
            gUI.playCantDoSound();
            return false;
         }
         BVector tempVec;
         if(!gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, flags) ||
            !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
         {
            gUI.playCantDoSound();
            return false;
         }

         // Should have been invoked now, so just send a shot.
         if (gWorld->getGametime() > (mLastShotSent + mShotInterval))
         {
            gUI.playClickSound();
            BPlayerID playerID = mpUser->getPlayerID();

            // play the vo
            if (mShotsRemaining == 1 && mLastShotSent > 0.0f)
               gSoundManager.playCue(mHudSounds.HudLastFireSound);
            else
               gSoundManager.playCue(mHudSounds.HudFireSound);

            BPowerInputCommand* pCommand = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandPowerInput);
            if (pCommand)
            {
               pCommand->setSenders(1, &playerID);
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setRecipientType(BCommand::cPlayer);
               pCommand->setType(BPowerInputCommand::cTypeConfirm);
               pCommand->setVector(mpUser->getHoverPoint());
               pCommand->setPowerUserID(mID);
               pCommand->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
            }

            // we decrement the counter here, 
            // but we do work based off of it once the sync code tells us we succeeded
            --mShotsRemaining;

            mLastShotSent = (float)gWorld->getGametime();
         }         
      }
      return (true);
   }

   // Did the user just cancel the power?
   if (pInputInterface->isFunctionControl(BInputInterface::cInputClear, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      // if mShotsRemaining == 0, then our sync callback should shut us down, so ignore this
      // also, this power user is never valid without a corresponding power, so allow cancellation if the 
      // power is gone
      bool powerExists = (gWorld->getPowerManager() && gWorld->getPowerManager()->getPowerByUserID(mID));
      if (event == cInputEventControlStart && (mShotsRemaining > 0 || !powerExists))
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
void BPowerUserOrbital::onShotFired(bool succeeded)
{
   // if we didn't succeed in the sync code, we need to refund a shot locally since we already decremented it
   if (!succeeded)
      ++mShotsRemaining;

   // We don't have any left to send, we can clean up now - the sim side will take care of itself.
   if (mShotsRemaining == 0)
   {
      // Shutdown our poweruser
      shutdown();
   }

   // Update the help text
   BUIContext* pUIContext = mpUser->getUIContext();
   if (pUIContext)
   {
      mHelpString.locFormat(gDatabase.getLocStringFromID(24730), mShotsRemaining);
      pUIContext->setPowerOverlayText(mProtoPowerID, mHelpString);
   }
}

//==============================================================================
//==============================================================================
void BPowerUserOrbital::update(float elapsedTime)
{
   mElapsed += elapsedTime;

   // get a new location
   BVector newLoc = mpUser->getHoverPoint();

   // clamp to terrain edges
   gTerrainSimRep.clampWorld(newLoc);

   // find height at position
   newLoc.y = 1000.0f;
   float height;
   gTerrainSimRep.getHeightRaycast(newLoc, height, true);
   newLoc.y = height;

   const float cMinDistanceToCorrect = 1.0f;

   // update the targetted object
   BEntityID targettedId = mpUser->getHoverObject();
   BUnit* pTargettedUnit = gWorld->getUnit(targettedId);
   BSquad* pTargettedSquad = (pTargettedUnit) ? pTargettedUnit->getParentSquad() : NULL;

   if (!pTargettedSquad)
   {
      // we have no hover squad, so reset our target squad
      if (mTargettedSquadID != cInvalidObjectID)
      {
         BSquad* pOldTargettedSquad = gWorld->getSquad(mTargettedSquadID);
         if (pOldTargettedSquad)
            pOldTargettedSquad->setTargettingSelection(false);
         mTargettedSquadID = cInvalidObjectID;
      }
   }
   else if (pTargettedSquad->getID() != mTargettedSquadID)
   {
      // we have a hover squad, but it's not the same as the one we had before
      BSquad* pOldTargettedSquad = gWorld->getSquad(mTargettedSquadID);
      if (pOldTargettedSquad)
         pOldTargettedSquad->setTargettingSelection(false);

      pTargettedSquad->setTargettingSelection(true);
      mTargettedSquadID = pTargettedSquad->getID();
   }

   // update the fake object
   BObject* pFakeObject = gWorld->getObject(mFakeTargettingLaserID);
   if (pFakeObject)
      pFakeObject->setPosition(newLoc);

   // check the real object
   BObject* pRealObject = gWorld->getObject(mRealTargettingLaserID);
   if(pRealObject)
   {
      BVector difference = (pRealObject->getPosition() - newLoc);

      if (difference.length() > cMinDistanceToCorrect)
      {
         if(gWorld->getGametimeFloat() > mLastCommandSent + mCommandInterval)
         {
            BPowerInputCommand* c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( mpUser->getPlayerID(), cCommandPowerInput );
            if(c)
            {
               // Set up the command.
               long playerID = mpUser->getPlayerID();
               c->setSenders( 1, &playerID );
               c->setSenderType( BCommand::cPlayer );
               c->setRecipientType( BCommand::cPlayer );
               c->setType( BPowerInputCommand::cTypePosition );
               // Set the data that will be poked in.
               c->setVector(newLoc);
               c->setPowerUserID(mID);
               c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
               // submit command
               gWorld->getCommandManager()->addCommandToExecute( c );

               mLastCommandSent = (float)gWorld->getGametimeFloat();
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BPowerUserOrbital::updateUI()
{
   // get a new location
   BVector newLoc = mpUser->getHoverPoint();

   // clamp to terrain edges
   gTerrainSimRep.clampWorld(newLoc);

   // find height at position
   newLoc.y = 1000.0f;
   float height;
   gTerrainSimRep.getHeightRaycast(newLoc, height, true);
   newLoc.y = height;

   // update the fake object
   BObject* pFakeObject = gWorld->getObject(mFakeTargettingLaserID);
   if (pFakeObject)
      pFakeObject->setPosition(newLoc);
}

//==============================================================================
//==============================================================================
void BPowerUserOrbital::renderUI()
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserOrbital - is not initialized but is trying to render UI.");
   if (!mInitialized)
      return;
   // No user.
   BASSERTM(mpUser, "BPowerUserOrbital - mpUser is NULL.");
   if (!mpUser)
      return;

   // Render some UI.
   if(!mpUser->getFlagHaveHoverPoint())
      return;
   BVector tempVec;
   if(!gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, BWorld::cCPLOSCenterOnly) ||
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
      mpUser->renderReticle(BUIGame::cReticlePowerInvalid);
}

//==============================================================================
//==============================================================================
void BPowerUserOrbital::setupUser()
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
      mHelpString.locFormat(gDatabase.getLocStringFromID(24730), mShotsRemaining);
      pUIContext->setPowerOverlayText(mProtoPowerID, mHelpString);
   }
}

//==============================================================================
//==============================================================================
BOrbitalShotInfo::BOrbitalShotInfo()
{
   mLaunchPos.zero();
   mTargetPos.zero();
   mLaunchTime = 0;
   mLaserObj = cInvalidObjectID;
}


//==============================================================================
//==============================================================================
bool BPowerOrbital::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mRealTargettingLaserID = cInvalidObjectID;
   mDesiredTargettingPosition = cOriginVector;
   mShots.clear();
   mFiredInitialShot = false;
   mShotsRemaining = 0;
   mImpactsToProcess = 0;
   mTargetBeamID = cInvalidProtoObjectID;
   mProjectileID = cInvalidProtoObjectID;
   mEffectProtoID = cInvalidProtoObjectID;
   mRockSmallProtoID = cInvalidProtoObjectID;
   mRockMediumProtoID = cInvalidProtoObjectID;
   mRockLargeProtoID = cInvalidProtoObjectID;
   mFiredSound = cInvalidCueIndex;
   mTargetingDelay = 0;
   mAutoShotDelay = 0;
   mAutoShotInnerRadius = 0.0f;
   mAutoShotOuterRadius = 0.0f;
   mXOffset = 0.0f;
   mYOffset = 0.0f;
   mZOffset = 0.0f;
   mLOSMode = BWorld::cCPLOSFullVisible;
   mElapsed = 0.0f;

   // casting variables
   mPlayerID = playerID;
   mPowerUserID = powerUserID;
   mPowerLevel = powerLevel;
   mOwnerID = ownerSquadID;
   mTargetLocation = targetLocation;

   //-------------------------------------------------
   // Handle all possible error cases.
   //-------------------------------------------------
   // No player
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
   {
      shutdown();
      return (false);
   }
   // No proto power.
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pProtoPower)
   {
      shutdown();
      return (false);
   }

   mFlagIgnoreAllReqs = ignoreAllReqs;
   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   // Player fails can use power test.
   if (!mFlagIgnoreAllReqs && !pPlayer->canUsePower(mProtoPowerID))
   {
      shutdown();
      return (false);
   }

   if (!pProtoPower->getDataProtoObject(powerLevel, "TargetBeam", mTargetBeamID))
   {
      shutdown();
      return (false);
   }

   if (!pProtoPower->getDataProtoObject(powerLevel, "Projectile", mProjectileID))
   {
      shutdown();
      return (false);
   }

   if (!pProtoPower->getDataProtoObject(powerLevel, "Effect", mEffectProtoID))
   {
      shutdown();
      return (false);
   }

   if (!pProtoPower->getDataProtoObject(powerLevel, "RockSmall", mRockSmallProtoID))
   {
      shutdown();
      return (false);
   }

   if (!pProtoPower->getDataProtoObject(powerLevel, "RockMedium", mRockMediumProtoID))
   {
      shutdown();
      return (false);
   }

   if (!pProtoPower->getDataProtoObject(powerLevel, "RockLarge", mRockLargeProtoID))
   {
      shutdown();
      return (false);
   }

   int numShots = 0;
   if (!pProtoPower->getDataInt(powerLevel, "NumShots", numShots))
   {
      shutdown();
      return (false);
   }
   if (numShots <= 0)
   {
      shutdown();
      return (false);
   }

   float targetingDelay = 0.0f;
   pProtoPower->getDataFloat(powerLevel, "TargetingDelay", targetingDelay);
   mTargetingDelay = static_cast<DWORD>(1000.0f * targetingDelay);

   float autoShotDelay = 0.0f;
   pProtoPower->getDataFloat(powerLevel, "AutoShotDelay", autoShotDelay);
   mAutoShotDelay = static_cast<DWORD>(1000.0f * autoShotDelay);

   mAutoShotInnerRadius = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "AutoShotInnerRadius", mAutoShotInnerRadius))
   {
      BFAIL("Could not get autoshotinnerradius attribute.");
   }

   mAutoShotOuterRadius = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "AutoShotOuterRadius", mAutoShotOuterRadius))
   {
      BFAIL("Could not get autoshotouterradius attribute.");
   }

   mXOffset = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "XOffset", mXOffset))
   {
      BFAIL("Could not get xoffset attribute.");
   }

   mYOffset = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "YOffset", mYOffset))
   {
      BFAIL("Could not get yoffset attribute.");
   }

   mZOffset = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "ZOffset", mZOffset))
   {
      BFAIL("Could not get zoffset attribute.");
   }

   mFiredSound = cInvalidCueIndex;
   if (!pProtoPower->getDataSound(powerLevel, "FiredSound", mFiredSound))
   {
      BFAIL("Could not get fired sound attribute.");
   }

   bool requiresLOS = true;
   if (mFlagIgnoreAllReqs || (pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS))
      mLOSMode = BWorld::cCPLOSDontCare;

   // Init some BPowerOrbital data here.
   mShotsRemaining = static_cast<uint>(numShots);
   mDesiredTargettingPosition = targetLocation;

   // create the initial targetting laser
   BObjectCreateParms parms;
   parms.mPlayerID = mPlayerID;
   parms.mPosition = mTargetLocation;
   parms.mProtoObjectID = mTargetBeamID;
   parms.mType = BEntity::cClassTypeObject;
   BObject* pLaserObject = gWorld->createObject(parms);
   BASSERTM(pLaserObject, "Couldn't create laser object for orbital power!");
   if (pLaserObject->getVisual())
      pLaserObject->getVisual()->setTintColor(gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextObjects));
   mRealTargettingLaserID = pLaserObject->getID();

   if(!gConfig.isDefined("macShowRealBeam"))
      pLaserObject->setFlagNoRenderForOwner(true);

   // notify power user instance about our newly created beam
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID)
   {
      BPowerUserOrbital* pOrbitalPowerUser = static_cast<BPowerUserOrbital*>(pUser->getPowerUser());
      if(pOrbitalPowerUser)
         pOrbitalPowerUser->setRealTargettingLaserID(mRealTargettingLaserID);
   }

   // All initialized.
   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerOrbital::shutdown()
{
   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if (pUser)
   {
      BPowerUser* pPowerUser = pUser->getPowerUser();
      if (pPowerUser && pPowerUser->getID() == mPowerUserID)
         pPowerUser->shutdown();
   }

   // kill our targetting beam
   BObject* pTargettingLaser = gWorld->getObject(mRealTargettingLaserID);
   if(pTargettingLaser)
   {
      pTargettingLaser->kill(false);
      mRealTargettingLaserID = cInvalidObjectID;
   }

   mFlagDestroy = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerOrbital::submitInput(BPowerInput powerInput)
{
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      shutdown();
      return false;
   }

   if (powerInput.mType == PowerInputType::cUserCancel)
   {
      // MS 7/7/2008: design request to make people lose their extra shots if they cancel.
      // Leaving other code in here because I'm about 98% sure it's coming right back. :p
      mShotsRemaining = 0;

/*
      // If we've already fired one shot, and didn't spend all our shots, just randomize the rest of them here...
      if (mFiredInitialShot && mShotsRemaining > 0)
      {
         DWORD nextTargetTime = gWorld->getGametime();
         while (mShotsRemaining > 0)
         {
            nextTargetTime += mAutoShotDelay;
            BVector newTargetPos = BSimHelper::randomCircularDistribution(mTargetLocation, mAutoShotOuterRadius, mAutoShotInnerRadius);
            BOrbitalShotInfo &rShot = mShots.grow();
            // For the laser.
            rShot.mCreateLaserTime = nextTargetTime;
            rShot.mLaserObj = cInvalidObjectID;
            rShot.mbLaserCreated = false;
            // For the projectile.
            rShot.mTargetPos = newTargetPos;
            rShot.mLaunchPos = newTargetPos + BVector(mXOffset, mYOffset, mZOffset);
            rShot.mLaunchTime = nextTargetTime + mTargetingDelay;

            // We've put another shot into the pipeline
            mShotsRemaining--;
         }
      }
*/

      // delete the targetting beam (and let it fade out) 
      BObject* pTargettingLaser = gWorld->getObject(mRealTargettingLaserID);
      if(pTargettingLaser)
      {
         pTargettingLaser->kill(false);
         mRealTargettingLaserID = cInvalidObjectID;
      }


      // The user cancelled the power.
      return (true);
   }
   else if (powerInput.mType == PowerInputType::cPosition)
   {
      mDesiredTargettingPosition = powerInput.mVector;
      return true;
   }
   else if (powerInput.mType == PowerInputType::cUserOK)
   {
      // fail if trying to cast outside of los. we are being aggressive about this to prevent command spoofing,
      // but there might be some way to grant users some leeway (e.g., if they're trying to fire at the edge of
      // fog of war in a situation where units are moving or dying).
      DWORD flags = BWorld::cCPLOSCenterOnly;
      BVector tempVec;
      if(!gWorld->checkPlacement(-1,  mPlayerID, powerInput.mVector, tempVec, cZAxisVector, mLOSMode, flags) ||
         !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(powerInput.mVector, this))
      {
         // we've got an invalid location - this can happen if disruption started after the user fired the shot
         // but before the synchronous command was processed. if this happens, don't shutdown, just reject the shot
         // tell the power user the synchronous code failed to fire a shot
         BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
         if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID)
         {
            BPowerUserOrbital* pOrbitalPowerUser = static_cast<BPowerUserOrbital*>(pUser->getPowerUser());
            if(pOrbitalPowerUser)
               pOrbitalPowerUser->onShotFired(false);
         }

         return false;
      }

      BASSERTM(mShotsRemaining > 0, "BPowerOrbital - Processing a shot in excess of max shots.  How did this happen?");
      if (mShotsRemaining > 0)
      {
         BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
         if (pPlayer)
         {
            // if this is our first shot, pay the cost now, otherwise, reset the recharge
            if (!mFiredInitialShot)
                pPlayer->usePower(mProtoPowerID, cInvalidObjectID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);
            else
               pPlayer->restartPowerRecharge(mProtoPowerID, cInvalidObjectID);
         }
         mFiredInitialShot = true;

         // play the firing sound
         if(mFiredSound != cInvalidCueIndex)
            gWorld->getWorldSoundManager()->addSound(powerInput.mVector, mFiredSound, true, cInvalidCueIndex, true, true);

         BOrbitalShotInfo &rShot = mShots.grow();
         // For the laser.
         rShot.mCreateLaserTime = gWorld->getGametime();
         rShot.mLaserObj = cInvalidObjectID;
         rShot.mbLaserCreated = false;
         // For the projectile
         rShot.mTargetPos = powerInput.mVector;
         rShot.mLaunchPos = powerInput.mVector + BVector(mXOffset, mYOffset, mZOffset);
         rShot.mLaunchTime = gWorld->getGametime() + mTargetingDelay;

         // We've put another shot into the pipeline
         mShotsRemaining--;
         mImpactsToProcess++;

         // tell the power user the synchronous code fired a shot
         BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
         if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID)
         {
            BPowerUserOrbital* pOrbitalPowerUser = static_cast<BPowerUserOrbital*>(pUser->getPowerUser());
            if(pOrbitalPowerUser)
               pOrbitalPowerUser->onShotFired(true);
         }

         // if this was our last shot, delete the targetting beam (immediately)
         if (mShotsRemaining == 0)
         {
            BObject* pTargettingLaser = gWorld->getObject(mRealTargettingLaserID);
            if(pTargettingLaser)
            {
               pTargettingLaser->kill(true);
               mRealTargettingLaserID = cInvalidObjectID;
            }
         }

         mDesiredTargettingPosition = powerInput.mVector;
      }

      return (true);
   }

   // Insert handling for additional power input types here.

   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerOrbital::submitInput() received powerInput of unsupported type.");
   return (false);
}

//==============================================================================
//==============================================================================
BProtoAction* BPowerOrbital::getProtoAction(void) const
{
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return(NULL);

   return BPowerHelper::getFirstProtoAction(*pPlayer, mProjectileID);
}

//==============================================================================
//==============================================================================
void BPowerOrbital::update(DWORD currentGameTime, float lastUpdateLength)
{
   currentGameTime;

   // Already marked for destroy.
   if (mFlagDestroy)
      return;
   // No player.
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
   {
      shutdown();
      return;
   }

   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      shutdown();
      return;
   }

   mElapsed += lastUpdateLength;

   // update the targetting laser based off of it's proto speed 
   BObject* pTargettingLaser = gWorld->getObject(mRealTargettingLaserID);
   if(pTargettingLaser && pTargettingLaser->getProtoObject())
   {
      const BVector& currentLaserPosition = pTargettingLaser->getPosition();
      float currentDistance = currentLaserPosition.xzDistance(mDesiredTargettingPosition);
      float xzDistanceThisFrame = (lastUpdateLength * pTargettingLaser->getProtoObject()->getMaxVelocity());
      BVector newPos = mDesiredTargettingPosition;

      // if we can travel far enough in this frame, take that position
      if (currentDistance >= xzDistanceThisFrame)
      {
         BVector currentDirection = (mDesiredTargettingPosition - currentLaserPosition);
         currentDirection.y = 0.0f;
         currentDirection.normalize();
         newPos = (currentLaserPosition + (currentDirection * xzDistanceThisFrame));
         gTerrainSimRep.getHeightRaycast(newPos, newPos.y, true);
      }

      // clamp to terrain edges
      gTerrainSimRep.clampWorld(newPos);
      pTargettingLaser->setPosition(newPos);
   }

   uint i = 0;
   while (i < mShots.getSize())
   {
      // Paint the laser if we need to...
      if (!mShots[i].mbLaserCreated && currentGameTime >= mShots[i].mCreateLaserTime)
      {
         mShots[i].mbLaserCreated = true;
         BObjectCreateParms parms;
         parms.mPlayerID = mPlayerID;
         parms.mPosition = mShots[i].mTargetPos;
         parms.mProtoObjectID = mTargetBeamID;
         parms.mType = BEntity::cClassTypeObject;
//-- FIXING PREFIX BUG ID 1490
         const BObject* pLaserObject = gWorld->createObject(parms);
//--
         if (pLaserObject)
            mShots[i].mLaserObj = pLaserObject->getID();
      }

      // Launch this shot and nuke it.
      if (currentGameTime >= mShots[i].mLaunchTime)
      {
         // Get rid of our laser object.
         BObject* pLaserObject = gWorld->getObject(mShots[i].mLaserObj);
         if (pLaserObject)
            pLaserObject->kill(true);

         BProtoAction* pProtoAction = getProtoAction();
         if (!pProtoAction)
         {
            mShots.removeIndex(i, false);
            continue;
         }

         // create the projectile that will do damage
         BObjectCreateParms parms;
         parms.mPlayerID = mPlayerID;
         parms.mProtoObjectID = pProtoAction->getProjectileID();
         parms.mPosition = mShots[i].mLaunchPos;
         parms.mForward = -cYAxisVector;
         parms.mRight = cXAxisVector;
         BEntityID projectileID = gWorld->launchProjectile(parms, mShots[i].mTargetPos, XMVectorZero(), XMVectorZero(), pProtoAction->getDamagePerSecond(), pProtoAction, pProtoAction, cInvalidObjectID, NULL, -1, false, true, true);
         BProjectile* pProj = gWorld->getProjectile(projectileID);
         if(pProj)
            pProj->setOwningPowerID(getID());

         // create the visual
         parms.mPosition = mShots[i].mTargetPos;
         parms.mForward = cZAxisVector;
         parms.mRight = cXAxisVector;
         parms.mProtoObjectID = mEffectProtoID;
         gWorld->createObject(parms);

         // create revealer
         gWorld->createRevealer(pPlayer->getTeamID(), mShots[i].mTargetPos, 12.0f, 10000);

         mShots.removeIndex(i, false);

         gWorld->createPowerAudioReactions(getPlayerID(), cSquadSoundChatterReactPowOrbital, mTargetLocation, 60.0f);
      }
      else
      {
         // Not ready to go yet...
         i++;
      }
   }

   // We're done processing shots and not waiting for any more.
   // Time to clean up.
   if (mShots.getSize() == 0 && mShotsRemaining == 0 && mImpactsToProcess == 0)
   {
      shutdown();
      return;
   }
}

//==============================================================================
//==============================================================================
void BPowerOrbital::projectileImpact(BEntityID id, const BEntityIDArray& killedUnits, const BEntityIDArray& damagedUnits)
{
   mImpactsToProcess--;


//-- FIXING PREFIX BUG ID 1494
   const BProtoObject* pRockProto = NULL;
//--

   uint numLarge = getRandRange(cSimRand, 2, 4);
   uint numMedium = getRandRange(cSimRand, 4, 7);
   uint numSmall = getRandRange(cSimRand, 7, 12);
   uint numToThrow = numLarge + numMedium + numSmall;

   const float cLinearImpulseLarge = 650.0f;
   const float cLinearImpulseMedium = 500.0f;
   const float cLinearImpulseSmall = 350.0f;
   float linearImpulse;

   ///const float cPointImpulseMax = 150.0f;
   const BVector cPointImpulseOffset = BVector(0, 1, 0);

   const float cImpulseYRangeLarge = 0.05f;
   const float cImpulseYRangeMedium = 0.1f;
   const float cImpulseYRangeSmall = 0.15f;
   float impulseYRange;

   BVector impulseDir;
   float impulse;
//-- FIXING PREFIX BUG ID 1495
   const BEntity* pEntity;
//--
   BUnit* pUnit;

   for(uint i = 0; i < numToThrow; i++)
   {
      if(i < numLarge)
      {
         pRockProto = gDatabase.getGenericProtoObject(mRockLargeProtoID);
         linearImpulse = cLinearImpulseLarge;
         impulseYRange = cImpulseYRangeLarge;
      }
      else if(i < numMedium)
      {
         pRockProto = gDatabase.getGenericProtoObject(mRockMediumProtoID);
         linearImpulse = cLinearImpulseMedium;
         impulseYRange = cImpulseYRangeMedium;
      }
      else
      {
         pRockProto = gDatabase.getGenericProtoObject(mRockSmallProtoID);
         linearImpulse = cLinearImpulseSmall;
         impulseYRange = cImpulseYRangeSmall;
      }

      if(pRockProto)
      {
         pEntity = gWorld->getEntity(id);
         if(!pEntity)
            break;

         BObjectCreateParms parms;
         parms.mPosition = pEntity->getPosition() + BVector(0,2,0);
         parms.mForward = pEntity->getForward();
         parms.mRight = pEntity->getRight();
         parms.mNoTieToGround = true;
         parms.mPhysicsReplacement = true;
         parms.mPlayerID = pEntity->getPlayerID();
         parms.mProtoObjectID = pRockProto->getID();
         parms.mProtoSquadID = -1;
         parms.mStartBuilt = false;
         parms.mType = BEntity::cClassTypeUnit;

         pUnit = gWorld->createUnit(parms);
         if(!pUnit)
            continue;

         impulseDir = BVector(getRandRangeFloat(cSimRand, -1, 1), getRandRangeFloat(cSimRand,  1, 1+impulseYRange), getRandRangeFloat(cSimRand, -1, 1));
         impulseDir.normalize();
         impulse = getRandRangeFloat(cSimRand, linearImpulse*0.5f, linearImpulse);
         
//-- FIXING PREFIX BUG ID 1493
         const BProtoVisual* pPV = gVisualManager.getProtoVisual(pRockProto->getProtoVisualIndex(), true);
//--
         if(pPV && pPV->getDefaultModel())
         {
            const BDamageTemplate* pDT = gDamageTemplateManager.getDamageTemplate(pPV->getDefaultModel()->mAsset.mDamageAssetIndex);
            if(pDT)
            {
               const BDamageImpactPoint* pDIP = pDT->getImpactPointByIndex(0);
               if(pDIP)
               {
                  for(long i = 0; i < pDIP->getEventCount(); i++)
                  {
                     const BDamageEvent* pDE = pDIP->getEvent(i);
                     if(pDE)
                     {
                        for(long j = 0; j < pDE->getActionCount(); j++)
                        {
                           const BDamageAction* pDA = pDE->getAction(0);
                           if(pDA)
                           {
                              pDA->execute(pUnit, true, NULL, impulse, true, &impulseDir);
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BPowerUserOrbital::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mHudSounds);
   GFWRITESTRING(pStream, BUString, mHelpString, 1000);
   GFWRITEVAR(pStream, BEntityID, mFakeTargettingLaserID);
   GFWRITEVAR(pStream, BEntityID, mRealTargettingLaserID);
   GFWRITEVAR(pStream, BEntityID, mTargettedSquadID);
   GFWRITEVAR(pStream, uint, mShotsRemaining);
   GFWRITEVAR(pStream, float, mLastCommandSent);
   GFWRITEVAR(pStream, float, mCommandInterval);
   GFWRITEVAR(pStream, float, mLastShotSent);
   GFWRITEVAR(pStream, float, mShotInterval);
   GFWRITEVAR(pStream, long, mLOSMode);
   GFWRITEBITBOOL(pStream, mFlagLastCameraAutoZoomInstantEnabled);
   GFWRITEBITBOOL(pStream, mFlagLastCameraAutoZoomEnabled);
   GFWRITEBITBOOL(pStream, mFlagLastCameraZoomEnabled);
   GFWRITEBITBOOL(pStream, mFlagLastCameraYawEnabled);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserOrbital::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;
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
   GFREADSTRING(pStream, BUString, mHelpString, 1000);
   GFREADVAR(pStream, BEntityID, mFakeTargettingLaserID);
   GFREADVAR(pStream, BEntityID, mRealTargettingLaserID);
   GFREADVAR(pStream, BEntityID, mTargettedSquadID);
   GFREADVAR(pStream, uint, mShotsRemaining);
   GFREADVAR(pStream, float, mLastCommandSent);
   GFREADVAR(pStream, float, mCommandInterval);
   GFREADVAR(pStream, float, mLastShotSent);
   GFREADVAR(pStream, float, mShotInterval);
   GFREADVAR(pStream, long, mLOSMode);
   GFREADBITBOOL(pStream, mFlagLastCameraAutoZoomInstantEnabled);
   GFREADBITBOOL(pStream, mFlagLastCameraAutoZoomEnabled);
   GFREADBITBOOL(pStream, mFlagLastCameraZoomEnabled);
   GFREADBITBOOL(pStream, mFlagLastCameraYawEnabled);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerOrbital::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, BEntityID, mRealTargettingLaserID);
   GFWRITEVECTOR(pStream, mDesiredTargettingPosition);
   GFWRITECLASSARRAY(pStream, saveType, mShots, uint8, 100);
   GFWRITEBITBOOL(pStream, mFiredInitialShot);
   GFWRITEVAR(pStream, uint, mShotsRemaining);
   GFWRITEVAR(pStream, uint, mImpactsToProcess);
   GFWRITEVAR(pStream, BProtoObjectID, mTargetBeamID);
   GFWRITEVAR(pStream, BProtoObjectID, mProjectileID);
   GFWRITEVAR(pStream, BProtoObjectID, mEffectProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mRockSmallProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mRockMediumProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mRockLargeProtoID);
   GFWRITEVAR(pStream, BCueIndex, mFiredSound);
   GFWRITEVAR(pStream, DWORD, mTargetingDelay);
   GFWRITEVAR(pStream, DWORD, mAutoShotDelay);
   GFWRITEVAR(pStream, float, mAutoShotInnerRadius);
   GFWRITEVAR(pStream, float, mAutoShotOuterRadius);
   GFWRITEVAR(pStream, float, mXOffset);
   GFWRITEVAR(pStream, float, mYOffset);
   GFWRITEVAR(pStream, float, mZOffset);
   GFWRITEVAR(pStream, long, mLOSMode);   
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerOrbital::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, BEntityID, mRealTargettingLaserID);
   GFREADVECTOR(pStream, mDesiredTargettingPosition);
   GFREADCLASSARRAY(pStream, saveType, mShots, uint8, 100);
   GFREADBITBOOL(pStream, mFiredInitialShot);
   GFREADVAR(pStream, uint, mShotsRemaining);
   GFREADVAR(pStream, uint, mImpactsToProcess);
   GFREADVAR(pStream, BProtoObjectID, mTargetBeamID);
   GFREADVAR(pStream, BProtoObjectID, mProjectileID);
   GFREADVAR(pStream, BProtoObjectID, mEffectProtoID);
   GFREADVAR(pStream, BProtoObjectID, mRockSmallProtoID);
   GFREADVAR(pStream, BProtoObjectID, mRockMediumProtoID);
   GFREADVAR(pStream, BProtoObjectID, mRockLargeProtoID);
   GFREADVAR(pStream, BCueIndex, mFiredSound);
   GFREADVAR(pStream, DWORD, mTargetingDelay);
   GFREADVAR(pStream, DWORD, mAutoShotDelay);
   GFREADVAR(pStream, float, mAutoShotInnerRadius);
   GFREADVAR(pStream, float, mAutoShotOuterRadius);
   GFREADVAR(pStream, float, mXOffset);
   GFREADVAR(pStream, float, mYOffset);
   GFREADVAR(pStream, float, mZOffset);
   GFREADVAR(pStream, long, mLOSMode);   

   gSaveGame.remapProtoObjectID(mTargetBeamID);
   gSaveGame.remapProtoObjectID(mProjectileID);
   gSaveGame.remapProtoObjectID(mEffectProtoID);
   gSaveGame.remapProtoObjectID(mRockSmallProtoID);
   gSaveGame.remapProtoObjectID(mRockMediumProtoID);
   gSaveGame.remapProtoObjectID(mRockLargeProtoID);

   return true;
}

//==============================================================================
//==============================================================================
bool BOrbitalShotInfo::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mLaunchPos);
   GFWRITEVECTOR(pStream, mTargetPos);
   GFWRITEVAR(pStream, DWORD, mLaunchTime);
   GFWRITEVAR(pStream, DWORD, mCreateLaserTime);
   GFWRITEVAR(pStream, BEntityID, mLaserObj);
   GFWRITEVAR(pStream, bool, mbLaserCreated);
   return true;
}

//==============================================================================
//==============================================================================
bool BOrbitalShotInfo::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mLaunchPos);
   GFREADVECTOR(pStream, mTargetPos);
   GFREADVAR(pStream, DWORD, mLaunchTime);
   GFREADVAR(pStream, DWORD, mCreateLaserTime);
   GFREADVAR(pStream, BEntityID, mLaserObj);
   GFREADVAR(pStream, bool, mbLaserCreated);
   return true;
}
