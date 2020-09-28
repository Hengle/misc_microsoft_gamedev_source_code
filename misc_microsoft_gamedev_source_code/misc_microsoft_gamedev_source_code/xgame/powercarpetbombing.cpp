//==============================================================================
// powercarpetbombing.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "powercarpetbombing.h"
#include "user.h"
#include "usermanager.h"
#include "database.h"
#include "objectmanager.h"
#include "world.h"
#include "commandmanager.h"
#include "commands.h"
#include "commandtypes.h"
#include "config.h"
#include "configsgame.h"
#include "tactic.h"
#include "SimOrderManager.h"
#include "simtypes.h"
#include "protopower.h"
#include "uigame.h"
#include "damagehelper.h"
#include "worldsoundmanager.h"
#include "powermanager.h"
#include "powerhelper.h"
#include "physics.h"
#include "physicsobject.h"

//#define CONSOLE_OUTPUT

//==============================================================================
//==============================================================================
bool BPowerUserCarpetBombing::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized
   initBase();
   mInputState = cWaitingForLocation;
   mPosition = cOriginVector;
   mDesiredForward = cXAxisVector;
   mDesiredScale = 1.0f;
   mCurrentScale = 1.0f;
   mShutdownTime = 0;
   mArrowID = cInvalidObjectID;
   mMaxBombOffset = 0.0f;
   mLengthMultiplier = 0.0f;
   mLOSMode = BWorld::cCPLOSFullVisible;
   mArrowProtoID = cInvalidProtoObjectID;
   mHudSounds.clear();

   // send the command to actually create the real power here or something.

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserCarpetBombing::init, mID %d", gWorld->getGametimeFloat(), mID);
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

   if (!pProtoPower->getDataProtoObject(powerLevel, "Arrow", mArrowProtoID))
   {
      BFAIL("Unable to get arrow data attribute.");
      return (false);
   }

   if(!pProtoPower->getDataFloat(powerLevel, "MaxBombOffset", mMaxBombOffset))
      return false;

   if(!pProtoPower->getDataFloat(powerLevel, "LengthMultiplier", mLengthMultiplier))
      return false;

   if (!mHudSounds.loadFromProtoPower(pProtoPower, powerLevel))
      return false;

   bool requiresLOS = true;
   if (pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS)
      mLOSMode = BWorld::cCPLOSDontCare;

   setupUser();

   // other setup
   mFlagDestroy = false;
   mElapsed = 0;

   mPowerLevel = powerLevel;

   gSoundManager.playCue(mHudSounds.HudUpSound);
   gSoundManager.playCue(mHudSounds.HudStartEnvSound);

   // mark as initialized
   mInitialized = true;
   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserCarpetBombing::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserCarpetBombing::shutdown, mID %d, shutting down", gWorld->getGametimeFloat(), mID);
#endif

   // destroy arrow object
   if(mArrowID.isValid())
   {
      BObject* pArrowObject = gWorld->getObject(mArrowID);
      if(pArrowObject)
      {
         pArrowObject->kill(false);
         pArrowObject = NULL;
         mArrowID = cInvalidObjectID;
      }
   }

   //Turn off the hud environment sound
   gSoundManager.playCue(mHudSounds.HudStopEnvSound);

   // hide power overlay
   BUIContext* pUIContext = mpUser->getUIContext();
   if (!pUIContext)
      return false;
   pUIContext->setPowerOverlayVisible(mProtoPowerID, false);

   // fix up the user's mode
   mpUser->unlockUser();

   mFlagDestroy = true;
   return (true);
}

//==============================================================================
//==============================================================================
void BPowerUserCarpetBombing::update(float elapsedTime)
{
   mElapsed += elapsedTime;
   if(mInputState == cWaitingForDirection)
   {
      // update the arrow
      if(!mArrowID.isValid() || !gWorld->getObject(mArrowID))
         return;

      BObject* pArrowObject = gWorld->getObject(mArrowID);

      // dampen rotation
      static const float cRotateGain = 13.0f;
      float desiredAngle = mDesiredForward.getAngleAroundY();
      float currentAngle = pArrowObject->getForward().getAngleAroundY();
      float angleDelta = desiredAngle - currentAngle;
      if(fabsf(angleDelta - cTwoPi) < fabsf(angleDelta))
         angleDelta = angleDelta - cTwoPi;
      if(fabsf(angleDelta + cTwoPi) < fabsf(angleDelta))
         angleDelta = angleDelta + cTwoPi;
      BVector newForward = pArrowObject->getForward();
      float rotationMult = 1.0f;
      if(mDesiredScale < mCurrentScale * 0.8f)
      {
         rotationMult = 0.1f;
         mDesiredForward.rotateXZ(-angleDelta * rotationMult);
      }
      newForward.rotateXZ(angleDelta * min(1.0f, elapsedTime * cRotateGain * rotationMult));
      newForward.normalize();

      // dampen scale
      static const float cScaleGain = 8.0f;
      mCurrentScale += (mDesiredScale - mCurrentScale) * min(1.0f, elapsedTime * cScaleGain);

      // calc right vec
      BVector right;
      right.assignCrossProduct(cYAxisVector, newForward);

      BMatrix mtx;
      mtx.makeIdentity();
      static const float cScaleFactor = 0.25f;
      static const float cMinScaleMult = 0.2f;
      float scaleMult = (cMinScaleMult + (1 - cMinScaleMult) * fabsf(newForward.dot(mDesiredForward))) * (mLengthMultiplier/50.0f);
      mtx.multScale(1.0f * (mMaxBombOffset / 8.0f), 1.0f, (1.0f + mCurrentScale * cScaleFactor) * scaleMult);
      mtx.multOrient(newForward, cYAxisVector, right);
      mtx.multTranslate(pArrowObject->getPosition().x, pArrowObject->getPosition().y, pArrowObject->getPosition().z);
      pArrowObject->setWorldMatrix(mtx);

   }
   else if(mInputState == cDone)
   {
      if(mElapsed > mShutdownTime)
         shutdown();
   }
}

//==============================================================================
//==============================================================================
bool BPowerUserCarpetBombing::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(!mInitialized)
      return false;

   if(!mpUser)
   {
      BFAIL("no mpUser in BPowerUserCarpetBombing::handleInput");
      return false;
   }

//-- FIXING PREFIX BUG ID 1360
   const BCamera* pCamera = mpUser->getCamera();
//--
   if(!pCamera)
   {
      BFAIL("no pCamera available in BPowerUserCarpetBombing::handleInput");
      return false;
   }

   bool start = (event == cInputEventControlStart);
   bool stop = (event == cInputEventControlStop);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(mpUser->getOption_ControlScheme());

   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      shutdown();
      return false;
   }

   if (pInputInterface->isFunctionControl( BInputInterface::cInputTranslation, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
      switch(mInputState)
      {
         case cWaitingForDirection:
         {
            // figure out new arrow direction and scale
            if(!stop)
            {
               // calc camera dir only if we haven't stopped inputting with left stick
               BVector dir;
               if(_fabs(pCamera->getCameraDir().y)<_fabs(pCamera->getCameraUp().y))
                  dir=BVector(pCamera->getCameraDir().x,0,pCamera->getCameraDir().z);
               else
                  dir=BVector(pCamera->getCameraUp().x,0,pCamera->getCameraUp().z);
               dir.normalize();

               // calc forward
               BVector forward = detail.mX * pCamera->getCameraRight() + detail.mY * -dir;
               forward.normalize();
               mDesiredForward = forward;
            }

            // calc scale
            mDesiredScale = BVector(detail.mX, detail.mY, 0).length();

            return true;

            break;
         }
         case cDone:
         {
            return true;
         }
      }

      return mpUser->handleInputScrollingAndCamera(port, event, controlType, detail);
   }
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
      if(start)
      {
         switch(mInputState)
         {
            case cWaitingForLocation:
            {
               // return if we're trying to cast out of los
               if(!mpUser->getFlagHaveHoverPoint())
               {
                  gUI.playCantDoSound();
                  return false;
               }
               BVector tempVec;
               if(!gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, BWorld::cCPLOSCenterOnly) ||
                  !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
               {
                  gUI.playCantDoSound();
                  return false;
               }

               gUI.playClickSound();

               // store position
               mPosition = mpUser->getHoverPoint();

               // play the vo
               gSoundManager.playCue(mHudSounds.HudFireSound);

               // create arrow
               BObjectCreateParms parms;
               parms.mPlayerID = mpUser->getPlayerID();
               parms.mPosition = mpUser->getHoverPoint();
               parms.mType = BEntity::cClassTypeObject;
               parms.mProtoObjectID = mArrowProtoID;
               BObject* pObject = gWorld->createObject(parms);
               if(pObject)
               {
                  mArrowID = pObject->getID();
                  mDesiredForward = pObject->getForward();
                  mDesiredScale = 0.0;
                  pObject->setFlagDontInterpolate(true);
                  pObject->setFlagNearLayer(true);
               }

               BUIContext* pUIContext = mpUser->getUIContext();
               if (pUIContext)
               {
                  pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(24734));
               }

               // switch input state
               mInputState = cWaitingForDirection;

               return true;
            }
            case cWaitingForDirection:
            {
               // get arrow object
               if(!mArrowID.isValid() || !gWorld->getObject(mArrowID))
               {
                  shutdown();
                  return false;
               }
//-- FIXING PREFIX BUG ID 1359
               const BObject* pArrowObject = gWorld->getObject(mArrowID);
//--
               BVector tempVec;
               if(!gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, BWorld::cCPLOSCenterOnly) ||
                  !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLineSegment(mpUser->getHoverPoint(), mpUser->getHoverPoint() + (pArrowObject->getForward() * mLengthMultiplier), this))
               {
                  gUI.playCantDoSound();
                  return false;
               }

               gUI.playClickSound();

			      // play the vo
               gSoundManager.playCue(mHudSounds.HudLastFireSound);

               // send invoke command
               BPlayerID playerID = mpUser->getPlayerID();
               BPowerCommand* pCommand = (BPowerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandPower);
               if(!pCommand)
                  return true;

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

               // send position input command
               BPowerInputCommand* c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( playerID, cCommandPowerInput );
               if( !c )
                  return( true );
               c->setSenders( 1, &playerID );
               c->setSenderType( BCommand::cPlayer );
               c->setRecipientType( BCommand::cPlayer );
               c->setType( BPowerInputCommand::cTypePosition );
               c->setVector(mPosition);
               c->setPowerUserID(mID);
               c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
               gWorld->getCommandManager()->addCommandToExecute( c );

               // send direction input command
               c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( playerID, cCommandPowerInput );
               if( !c )
                  return( true );
               c->setSenders( 1, &playerID );
               c->setSenderType( BCommand::cPlayer );
               c->setRecipientType( BCommand::cPlayer );
               c->setType( BPowerInputCommand::cTypeDirection );
               c->setVector(pArrowObject->getForward());
               c->setPowerUserID(mID);
               c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
               gWorld->getCommandManager()->addCommandToExecute( c );

               // destroy arrow object
               if(mArrowID.isValid())
               {
                  BObject* pArrowObject = gWorld->getObject(mArrowID);
                  if(pArrowObject)
                  {
                     pArrowObject->kill(false);
                     pArrowObject = NULL;
                     mArrowID = cInvalidObjectID;
                  }
               }


               // switch input state
               mInputState = cDone;

               // set shutdown time
               mShutdownTime = mElapsed + 0.25;

               return true;
            }
            case cDone:
            {
               return true;
            }
         }
      }
   }
   else if(pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      // this power isn't valid if the power is gone, so allos us to shut it down
      bool powerExists = (gWorld->getPowerManager() && gWorld->getPowerManager()->getPowerByUserID(mID));
      if( start && (mInputState != cDone || !powerExists))
      {
#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerUserCarpetBombing::handleInput, mID %d, sending shutdown command", gWorld->getGametimeFloat(), mID);
#endif
         gSoundManager.playCue(mHudSounds.HudAbortSound);

         cancelPower();
      }
      return( true );
   }

   return mpUser->handleInputScrollingAndCamera(port, event, controlType, detail);
}

//==============================================================================
//==============================================================================
void BPowerUserCarpetBombing::renderUI()
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserCarpetBombing - is not initialized but is trying to render UI.");
   if (!mInitialized)
      return;
   // No user.
   BASSERTM(mpUser, "BPowerUserCarpetBombing - mpUser is NULL.");
   if (!mpUser)
      return;

   // Render some UI.
   if(mInputState == cWaitingForLocation)
   {
      // return if we're trying to cast out of los
      DWORD flags = BWorld::cCPLOSCenterOnly;
      if(!mpUser->getFlagHaveHoverPoint())
         return;
      BVector tempVec;
      if(gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, flags) &&
         gWorld->getPowerManager() && gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
         mpUser->renderReticle(BUIGame::cReticlePowerValid);
      else
         mpUser->renderReticle(BUIGame::cReticlePowerInvalid);
   }
}

//==============================================================================
//==============================================================================
void BPowerUserCarpetBombing::setupUser()
{
   BASSERT(mpUser);
   if (!mpUser)
      return;

   // Lock the user.
   mpUser->lockUser(cInvalidTriggerScriptID);
   mpUser->changeMode(BUser::cUserModePower);
   mpUser->setUIPowerRadius(0.0f);

   // show power overlay
   BUIContext* pUIContext = mpUser->getUIContext();
   if (!pUIContext)
      return;
   pUIContext->setPowerOverlayVisible(mProtoPowerID, true);         
   pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(24731));
}

//==============================================================================
//==============================================================================
bool BPowerCarpetBombing::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized
   initBase();
   mBombExplodeInfos.clear();
   mNudgedUnits.empty();
   mStartLocation = cOriginVector;
   mStartDirection = cOriginVector;
   mRightVector = cOriginVector;
   mState = cWaitingForInputs;
   mGotStartLocation = false;
   mGotStartDirection = false;
   mTickLength = 0.067;
   mNextBombTime = 0;
   mLastBombTime = 0;
   mNumBombClustersDropped = 0;
   mProjectileProtoID = cInvalidProtoObjectID;
   mImpactProtoID = cInvalidProtoObjectID;
   mExplosionProtoID = cInvalidProtoObjectID;
   mBomberProtoID = cInvalidProtoObjectID;
   mInitialDelay = 0.0f;
   mFuseTime = 0.0f;
   mMaxBombs = 0;
   mMaxBombOffset = 0.0f;
   mBombSpacing = 0.0f;
   mLengthMultiplier = 0.0f;
   mWedgeLengthMultiplier = 0.0f;
   mWedgeMinOffset = 0.0f;
   mNudgeMultiplier = 0.0f;
   mBomberData.clear();
   mLOSMode = BWorld::cCPLOSFullVisible;
   mReactionPlayed = false;

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerCarpetBombing::init, playerID %d, powerUserID %d, ownerSquadID %d", gWorld->getGametimeFloat(), playerID, powerUserID, ownerSquadID);
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

   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      shutdown();
      return false;
   }
   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   bool bSuccess = true;
   mFlagIgnoreAllReqs = ignoreAllReqs;

   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "Projectile", mProjectileProtoID);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "Impact", mImpactProtoID);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "Explosion", mExplosionProtoID);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "Bomber", mBomberProtoID);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "InitialDelay", mInitialDelay);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "FuseTime", mFuseTime);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
   {
      int maxBombs = 0;
      bSuccess = pProtoPower->getDataInt(powerLevel, "MaxBombs", maxBombs);
      if (bSuccess && maxBombs >= 0)
         mMaxBombs = maxBombs;
      else
         mMaxBombs = 0;
   }
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "MaxBombOffset", mMaxBombOffset);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "BombSpacing", mBombSpacing);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "LengthMultiplier", mLengthMultiplier);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "WedgeLengthMultiplier", mWedgeLengthMultiplier);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "WedgeMinOffset", mWedgeMinOffset);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "NudgeMultiplier", mNudgeMultiplier);
   BASSERTM(bSuccess, "Unable to get required data attribute.");

   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "BomberFlyinDistance", mBomberData.BomberFlyinDistance);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "BomberFlyinHeight", mBomberData.BomberFlyinHeight);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "BomberBombHeight", mBomberData.BomberBombHeight);
   BASSERTM(bSuccess, "Unable to get required data attribute.");
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "BomberSpeed", mBomberData.BomberSpeed);
   BASSERTM(bSuccess, "Unable to get required data attribute.");

   if (bSuccess)
   {
      bool requiresLOS = true;
      if (mFlagIgnoreAllReqs || (pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS))
         mLOSMode = BWorld::cCPLOSDontCare;
   }
   BASSERTM(bSuccess, "Unable to get required data attribute.");

   if (!bSuccess)
   {
      BFAIL("Unable to get required data attribute.");
      shutdown();
      return (false);
   }

   // fail if trying to cast outside of los
   DWORD flags = BWorld::cCPLOSCenterOnly;
   BVector tempVec;
   if(!gWorld->checkPlacement(-1,  playerID, targetLocation, tempVec, cZAxisVector, mLOSMode, flags) || 
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(targetLocation, this))
   {
      shutdown();
      return false;
   }

   // basics
   mElapsed = 0.0;

   mBomberData.BombTime = mInitialDelay;
   mBomberData.FlyoutTime = (mInitialDelay + (mMaxBombs * (float)mTickLength));
   mBomberData.BomberId = cInvalidObjectID;

   // pay the cost first, bail if we can't
   if(!mFlagIgnoreAllReqs && !pPlayer->canUsePower(mProtoPowerID, mOwnerID))
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerCarpetBombing::init, shutting down because player can't pay startup cost", gWorld->getGametimeFloat());
#endif
      shutdown();
      return false;
   }

   pPlayer->usePower(mProtoPowerID, mOwnerID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCarpetBombing::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerCarpetBombing::shutdown, mID %d, mPowerUserID %d", gWorld->getGametimeFloat(), mID, mPowerUserID);
#endif

   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID)
      pUser->getPowerUser()->shutdown();

   // delete the bomber if we created one
   BObject* pBomberUnit = gWorld->getObject(mBomberData.BomberId);
   if (pBomberUnit)
      pBomberUnit->kill(true);

   mFlagDestroy = true;

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCarpetBombing::submitInput(BPowerInput powerInput)
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerCarpetBombing::submitInput, mID %d, mPowerUserID %d, input type %d", gWorld->getGametimeFloat(), mID, mPowerUserID, powerInput.mType);
#endif

   if (powerInput.mType == PowerInputType::cUserCancel)
   {
      // do nothing - this is a fire and forget power, once invoked, no reason to cancel
      return (true);
   }
   else if (powerInput.mType == PowerInputType::cPosition)
   {
      // verify state
      if(mState != cWaitingForInputs)
         return true;

      // assign
      mStartLocation = powerInput.mVector;

      // set position as received
      mGotStartLocation = true;

      // switch state from "waiting for inputs" to "active" if we have a position and direction
      if(mGotStartLocation && mGotStartDirection)
         setState(cActive);

      return true;
   }
   else if (powerInput.mType == PowerInputType::cDirection)
   {
      // verify state
      if(mState != cWaitingForInputs)
         return true;

      // assign
      mStartDirection = powerInput.mVector;

      // set direction as received
      mGotStartDirection = true;

      // switch state from "waiting for inputs" to "active" if we have a position and direction
      if(mGotStartLocation && mGotStartDirection)
         setState(cActive);

      return true;
   }
   // Insert handling for additional power input types here.

   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerCarpetBombing::submitInput() received powerInput of unsupported type.");
   return (false);
}


//==============================================================================
//==============================================================================
void BPowerCarpetBombing::update(DWORD currentGameTime, float lastUpdateLength)
{
   currentGameTime;

   mElapsed += lastUpdateLength;

   if(getFlagDestroy())
      return;

   // bomb checks
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerCarpetBombing::update, bomb check failed, shutting down", gWorld->getGametimeFloat());
#endif
      shutdown();
      return;
   }
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      shutdown();
      return;
   }

   BProtoAction* pProtoAction = BPowerHelper::getFirstProtoAction(*pPlayer, mProjectileProtoID);
   if (!pProtoAction)
   {
      shutdown();
      return;
   }

   switch(mState)
   {
      case cWaitingForInputs:
      {
         if(mGotStartDirection && mGotStartLocation)
            setState(cActive);
         break;
      }

      case cActive:
      {
         // spawn bomb cluster if it's time
         float bombOffset1, bombOffset2, currentMaxOffset, positionMult;
         BASSERT(mMaxBombOffset > mBombSpacing);
         BObjectCreateParms ocp;
         BUnit* pBomb = NULL;
         float dps;
         BBombExplodeInfo bei;
         byte surfaceImpactType = 1;
         BImpactSoundInfo soundInfo;
         bool result;
         while(mElapsed > mNextBombTime && mNumBombClustersDropped < mMaxBombs)
         {
            // bomb 1
            positionMult = ((float)mNumBombClustersDropped/mMaxBombs * mLengthMultiplier) - mWedgeLengthMultiplier;
            if(positionMult >= 0)
               currentMaxOffset = mMaxBombOffset;
            else
               currentMaxOffset = (mWedgeMinOffset * (-positionMult/mWedgeLengthMultiplier)) + (mMaxBombOffset * (-(positionMult+mWedgeLengthMultiplier)/mWedgeLengthMultiplier));
#ifdef CONSOLE_OUTPUT
            gConsoleOutput.status("%.2f: mNumBombClustersDropped %d    positionMult %.2f    currentMaxOffset %.2f", gWorld->getGametimeFloat(), mNumBombClustersDropped, positionMult, currentMaxOffset);
#endif
            bombOffset1 = getRandRangeFloat(cSimRand, -currentMaxOffset, 0);
            ocp.mPlayerID = mPlayerID;
            ocp.mPosition = mStartLocation + (mStartDirection * positionMult) + (bombOffset1 * mRightVector);
            ocp.mForward = -mStartDirection;
            ocp.mRight = -mRightVector;
            ocp.mType = BEntity::cClassTypeUnit;
            ocp.mProtoObjectID = mImpactProtoID;
            pBomb = gWorld->createUnit(ocp);
            if(!pBomb)
               break;
            pBomb->tieToGround();
#ifdef CONSOLE_OUTPUT
            gConsoleOutput.status("%.2f: dropped bomb at %.2f %.2f %.2f", gWorld->getGametimeFloat(), pBomb->getPosition().x, pBomb->getPosition().y, pBomb->getPosition().z);
#endif

            // add bomb info
            bei.mExplodeTime = mElapsed + mFuseTime;
            bei.mPosition = ocp.mPosition;
            mBombExplodeInfos.add(bei);

            // play sound
            surfaceImpactType = gTerrainSimRep.getTileType(ocp.mPosition);
            if (surfaceImpactType != -1 )
            {
               result = pBomb->getProtoObject()->getImpactSoundCue(surfaceImpactType, soundInfo);
               if (result && (soundInfo.mSoundCue != cInvalidCueIndex))
                  gWorld->getWorldSoundManager()->addSound(pBomb->getPosition(), soundInfo.mSoundCue, true, cInvalidCueIndex, soundInfo.mCheckSoundRadius, true);            
            }

            // create revealer
            gWorld->createRevealer(pPlayer->getTeamID(), bei.mPosition, 10.0f, 5000);

            // bomb 2
            bombOffset2 = getRandRangeFloat(cSimRand, bombOffset1 + mBombSpacing, currentMaxOffset);
            ocp.mPosition = mStartLocation + (mStartDirection * positionMult) + (bombOffset2 * mRightVector);
            ocp.mForward = -mStartDirection;
            ocp.mRight = -mRightVector;
            ocp.mType = BEntity::cClassTypeUnit;
            ocp.mProtoObjectID = mImpactProtoID;
            pBomb = gWorld->createUnit(ocp);
            if(!pBomb)
               break;
            pBomb->tieToGround();
#ifdef CONSOLE_OUTPUT
            gConsoleOutput.status("%.2f: dropped bomb at %.2f %.2f %.2f", gWorld->getGametimeFloat(), pBomb->getPosition().x, pBomb->getPosition().y, pBomb->getPosition().z);
#endif
            mNumBombClustersDropped++;

            // add bomb info
            bei.mExplodeTime = mElapsed + mFuseTime;
            bei.mPosition = ocp.mPosition;
            mBombExplodeInfos.add(bei);

            // play sound
            surfaceImpactType = gTerrainSimRep.getTileType(ocp.mPosition);
            if (surfaceImpactType != -1 )
            {
               result = pBomb->getProtoObject()->getImpactSoundCue(surfaceImpactType, soundInfo);
               if (result && (soundInfo.mSoundCue != cInvalidCueIndex))
                  gWorld->getWorldSoundManager()->addSound(pBomb->getPosition(), soundInfo.mSoundCue, true, cInvalidCueIndex, soundInfo.mCheckSoundRadius, true);            
            }

            // create revealer
            gWorld->createRevealer(pPlayer->getTeamID(), bei.mPosition, 10.0f, 5000);

            // record the last successfully dropped bomb time
            mLastBombTime = mElapsed;

            // update next bomb time
            mNextBombTime += mTickLength;
         }

         // check fuses and explode bombs if needed
         BObjectCreateParms projParms;
         BUnit* pExplosion = NULL;
         for(int i = 0; i < mBombExplodeInfos.getNumber(); i++)
         {
            bei = mBombExplodeInfos[i];
            if(bei.mExplodeTime > mElapsed)
               break;

            // spawn explosion
            ocp.mPlayerID = mPlayerID;
            ocp.mPosition = bei.mPosition;
            ocp.mForward = -mStartDirection;
            ocp.mRight = -mRightVector;
            ocp.mType = BEntity::cClassTypeUnit;
            ocp.mProtoObjectID = mExplosionProtoID;
            pExplosion = gWorld->createUnit(ocp);
            if(!pExplosion)
               break;
            pExplosion->tieToGround();

            // do damage
            projParms.mPlayerID = mPlayerID;
            projParms.mProtoObjectID = pProtoAction->getProjectileID();
            projParms.mPosition = bei.mPosition + BVector(0,5,0);
            projParms.mForward = -mStartDirection;
            projParms.mRight = -mRightVector;
            dps = pProtoAction->getDamagePerSecond();
            BEntityID projectileID = gWorld->launchProjectile(projParms, bei.mPosition, XMVectorZero(), XMVectorZero(), dps, pProtoAction, pProtoAction, cInvalidObjectID, NULL, -1, true, true, true);
            BProjectile* pProj = gWorld->getProjectile(projectileID);
            if(pProj)
               pProj->setOwningPowerID(getID());

#ifdef CONSOLE_OUTPUT
            gConsoleOutput.status("%.2f: dropping projectile from %.2f %.2f %.2f", gWorld->getGametimeFloat(), projParms.mPosition.x, projParms.mPosition.y, projParms.mPosition.z);
#endif

            // play sound
            surfaceImpactType = gTerrainSimRep.getTileType(projParms.mPosition);
            if (surfaceImpactType != -1 )
            {
               result = pExplosion->getProtoObject()->getImpactSoundCue(surfaceImpactType, soundInfo);
               if (result && (soundInfo.mSoundCue != cInvalidCueIndex))
                  gWorld->getWorldSoundManager()->addSound(pExplosion->getPosition(), soundInfo.mSoundCue, true, cInvalidCueIndex, soundInfo.mCheckSoundRadius, true);            
            }

            mBombExplodeInfos.removeIndex(i--);            

            if(mReactionPlayed == false)
            {
               gWorld->createPowerAudioReactions(getPlayerID(), cSquadSoundChatterReactPowCarpet, mTargetLocation, 60.0f);
               mReactionPlayed = true;
            }
         }

         // manually move the bomber, no worrying about pathing here 
         BPowerHelper::updateBomberPosition(mBomberData, (float)mElapsed, lastUpdateLength);

         // bail if we're done bombing
         if(mNumBombClustersDropped >= mMaxBombs && mBombExplodeInfos.getNumber() == 0)
         {
            shutdown();
            return;
         }

         break;
      }
   }
}

void BPowerCarpetBombing::setState(EState newState)
{
   switch(mState)
   {
      case cWaitingForInputs:
      {
         if(newState == cActive)
         {
            // normalize direction
            mStartDirection.normalize();

            // calc cross product
            mRightVector.assignCrossProduct(cYAxisVector, mStartDirection);
            mRightVector.normalize();

            // set up damage time
            mNextBombTime = mElapsed + mInitialDelay;

            // set up bomber
            mBomberData.BomberId = BPowerHelper::createBomber(mBomberProtoID, mBomberData, mStartLocation, mStartDirection, mPlayerID);
         }
         break;
      }
   }

   mState = newState;
}

//==============================================================================
//==============================================================================
void BPowerCarpetBombing::projectileImpact(BEntityID id, const BEntityIDArray& killedUnits, const BEntityIDArray& damagedUnits)
{
   // nudge damaged units
   BProjectile* pProjectile = gWorld->getProjectile(id);
   BASSERT(pProjectile);

   const BVector& projectilePos = pProjectile->getPosition();

   for (uint i = 0; i < damagedUnits.getSize(); ++i)
   {
      BUnit* pUnit = gWorld->getUnit(damagedUnits[i]);
      if (!pUnit)
         continue;
      
      if (!pUnit->getPhysicsObject())
         continue;

      // if we're already nudged this unit with this carpet bomb, leave it alone
      if (mNudgedUnits.contains(damagedUnits[i]))
         continue;

      BVector dirFromProj = pUnit->getPosition() - projectilePos;
      dirFromProj.normalize();
      BVector nudgeLocation;
      pUnit->getPhysicsObject()->getCenterOfMassLocation(nudgeLocation);
      nudgeLocation -= (dirFromProj * pUnit->getObstructionRadius());

      BVector impulseForce = dirFromProj;
      impulseForce.y = 0.0f;
      impulseForce.normalize();
      impulseForce.y = 1.0f;
      impulseForce *= pUnit->getPhysicsObject()->getMass();
      impulseForce *= mNudgeMultiplier;
      pUnit->getPhysicsObject()->applyPointImpulse(impulseForce, nudgeLocation);

      mNudgedUnits.add(damagedUnits[i]);
   }
}

//==============================================================================
//==============================================================================
bool BPowerUserCarpetBombing::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITEVAL(pStream, int8, mInputState);
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVECTOR(pStream, mDesiredForward);
   GFWRITEVAR(pStream, float, mDesiredScale);
   GFWRITEVAR(pStream, float, mCurrentScale);
   GFWRITEVAR(pStream, double, mShutdownTime);
   GFWRITEVAR(pStream, BEntityID, mArrowID);
   GFWRITEVAR(pStream, float, mMaxBombOffset);
   GFWRITEVAR(pStream, float, mLengthMultiplier);
   GFWRITEVAL(pStream, int8, mLOSMode);
   GFWRITEVAR(pStream, BProtoObjectID, mArrowProtoID);
   GFWRITECLASS(pStream, saveType, mHudSounds);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserCarpetBombing::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;

   GFREADVAL(pStream, int8, EInputState, mInputState);
   GFREADVECTOR(pStream, mPosition);
   GFREADVECTOR(pStream, mDesiredForward);
   GFREADVAR(pStream, float, mDesiredScale);
   GFREADVAR(pStream, float, mCurrentScale);
   GFREADVAR(pStream, double, mShutdownTime);
   GFREADVAR(pStream, BEntityID, mArrowID);
   GFREADVAR(pStream, float, mMaxBombOffset);
   GFREADVAR(pStream, float, mLengthMultiplier);
   GFREADVAL(pStream, int8, long, mLOSMode);
   GFREADVAR(pStream, BProtoObjectID, mArrowProtoID);
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
   
   gSaveGame.remapProtoObjectID(mArrowProtoID);

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCarpetBombing::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   //BSmallDynamicSimArray<BVector> mWaypoints;
   GFWRITEARRAY(pStream, BBombExplodeInfo, mBombExplodeInfos, uint8, 200);

   // mNudgedUnits
   uint count = mNudgedUnits.getNumItems();
   GFWRITEVAR(pStream, uint, count);
   GFVERIFYCOUNT(count, 400);
   for (uint i=0; i<count; i++)
      GFWRITEVAL(pStream, BEntityID, mNudgedUnits.getItem(i));

   GFWRITEVECTOR(pStream, mStartLocation);
   GFWRITEVECTOR(pStream, mStartDirection);
   GFWRITEVECTOR(pStream, mRightVector);
   GFWRITEVAL(pStream, int8, mState);
   GFWRITEBITBOOL(pStream, mGotStartLocation);
   GFWRITEBITBOOL(pStream, mGotStartDirection);
   GFWRITEVAR(pStream, double, mTickLength);
   GFWRITEVAR(pStream, double, mNextBombTime);
   GFWRITEVAR(pStream, double, mLastBombTime);
   GFWRITEVAR(pStream, uint, mNumBombClustersDropped);
   GFWRITEVAR(pStream, BProtoObjectID, mProjectileProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mImpactProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mExplosionProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mBomberProtoID);
   GFWRITEVAR(pStream, float, mInitialDelay);
   GFWRITEVAR(pStream, float, mFuseTime);
   GFWRITEVAR(pStream, uint, mMaxBombs);
   GFWRITEVAR(pStream, float, mMaxBombOffset);
   GFWRITEVAR(pStream, float, mBombSpacing);
   GFWRITEVAR(pStream, float, mLengthMultiplier);
   GFWRITEVAR(pStream, float, mWedgeLengthMultiplier);
   GFWRITEVAR(pStream, float, mWedgeMinOffset);
   GFWRITEVAR(pStream, float, mNudgeMultiplier);
   GFWRITECLASS(pStream, saveType, mBomberData);
   GFWRITEVAL(pStream, int8, mLOSMode);
   GFWRITEVAR(pStream, bool, mReactionPlayed);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCarpetBombing::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   //BSmallDynamicSimArray<BVector> mWaypoints;
   GFREADARRAY(pStream, BBombExplodeInfo, mBombExplodeInfos, uint8, 200);

   // mNudgedUnits
   uint count;
   GFREADVAR(pStream, uint, count);
   GFVERIFYCOUNT(count, 400);
   for (uint i=0; i<count; i++)
   {
      BEntityID entityID;
      GFREADVAR(pStream, BEntityID, entityID);
      mNudgedUnits.add(entityID);
   }

   GFREADVECTOR(pStream, mStartLocation);
   GFREADVECTOR(pStream, mStartDirection);
   GFREADVECTOR(pStream, mRightVector);
   GFREADVAL(pStream, int8, EState, mState);
   GFREADBITBOOL(pStream, mGotStartLocation);
   GFREADBITBOOL(pStream, mGotStartDirection);
   GFREADVAR(pStream, double, mTickLength);
   GFREADVAR(pStream, double, mNextBombTime);
   GFREADVAR(pStream, double, mLastBombTime);
   GFREADVAR(pStream, uint, mNumBombClustersDropped);
   GFREADVAR(pStream, BProtoObjectID, mProjectileProtoID);
   GFREADVAR(pStream, BProtoObjectID, mImpactProtoID);
   GFREADVAR(pStream, BProtoObjectID, mExplosionProtoID);
   GFREADVAR(pStream, BProtoObjectID, mBomberProtoID);
   if (BPower::mGameFileVersion <= 9)
   {
      BCueIndex tempCue;
      GFREADVAR(pStream, BCueIndex, tempCue);
   }
   GFREADVAR(pStream, float, mInitialDelay);
   GFREADVAR(pStream, float, mFuseTime);
   GFREADVAR(pStream, uint, mMaxBombs);
   GFREADVAR(pStream, float, mMaxBombOffset);
   GFREADVAR(pStream, float, mBombSpacing);
   GFREADVAR(pStream, float, mLengthMultiplier);
   GFREADVAR(pStream, float, mWedgeLengthMultiplier);
   GFREADVAR(pStream, float, mWedgeMinOffset);
   GFREADVAR(pStream, float, mNudgeMultiplier);
   GFREADCLASS(pStream, saveType, mBomberData);
   GFREADVAL(pStream, int8, long, mLOSMode);
   GFREADVAR(pStream, bool, mReactionPlayed);
   return true;
}
