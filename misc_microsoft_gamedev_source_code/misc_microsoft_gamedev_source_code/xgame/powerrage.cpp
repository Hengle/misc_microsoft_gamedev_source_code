//==============================================================================
// powerrage.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "powerrage.h"
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
#include "squadactionchangemode.h"
#include "squadAI.h"
#include "actionmanager.h"
#include "physics.h"
#include "physicsobject.h"
#include "powerhelper.h"
#include "powerdisruption.h"
#include "entityactionlisten.h"
#include "unitactionrage.h"
#include "achievementmanager.h"

static const float cAudioReactionInterval = 5.0f;
static const long cHintId = 24845;

//==============================================================================
//==============================================================================
bool BPowerUserRage::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mCameraZoom = 0.0f;
   mCameraHeight = 0.0f;
   mTimestampNextCommand = 0;
   mMoveInputDir = cOriginVector;
   mAttackInputDir = cOriginVector;
   mTimeUntilHint = 0.0f;
   mLastMovePos = cOriginVector;
   mLastMoveDir = cOriginVector;
   mLastAttackDir = cOriginVector;
   mCommandInterval = 0;
   mScanRadius = 0.0f;
   mHasMoved = false;
   mHasAttacked = false;
   mHintShown = false;
   mHintCompleted = false;
   mForceCommandNextUpdate = false;
   mMovementProjectionMultiplier = 1.0f;

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserRage::init, mID %d", gWorld->getGametimeFloat(), mID);
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

   const BSquad* pOwnerSquad = gWorld->getSquad(ownerSquadID);
   const BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pLeaderUnit)
      return false;

   if (pLeaderUnit->getFlagDoingFatality() || pLeaderUnit->getFlagFatalityVictim())
      return false;

   bool bSuccess = true;
   float commandInterval = 0.0f;
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CommandInterval", commandInterval));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "ScanRadius", mScanRadius));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "HintTime", mTimeUntilHint));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MovementProjectionMultiplier", mMovementProjectionMultiplier));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CameraZoom", mCameraZoom));

   if (!bSuccess)
   {
      BFAIL("Missing or invalid data attribute.");
      return (false);
   }

   BCamera* pCamera = mpUser->getCamera();
   if (!pCamera)
      return false;
   
   // we need to force update the hover point so the camera effect happens at the right location
   mpUser->setInterpData(0.3f, pLeaderUnit->getPosition());
   mpUser->setFlagNoCameraLimits(true);

   mOwnerSquadID = ownerSquadID;
   mCommandInterval = static_cast<DWORD>(1000.0f * Math::fSelectMax(commandInterval, 0.1f));

   setupUser();

   // other setup
   mTimestampNextCommand = gWorld->getGametime() + mCommandInterval;
   mFlagDestroy = false;
   mElapsed = 0;
   mPowerLevel = powerLevel;

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
   pCommand->setTargetLocation(mpUser->getHoverPoint());
   pCommand->setSquadID(ownerSquadID);
   pCommand->setFlag(BPowerCommand::cNoCost, mFlagNoCost);
   gWorld->getCommandManager()->addCommandToExecute(pCommand);

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserRage::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserRage::shutdown, mID %d, shutting down", gWorld->getGametimeFloat(), mID);
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

   // fix up the user's mode
   mpUser->setFlagNoCameraLimits(false);
   mpUser->unlockUser();

   mFlagDestroy = true;
   return (true);
}

//==============================================================================
//==============================================================================
void BPowerUserRage::getCameraRelativeInputDir(BCamera& camera, float inputX, float inputY, BVector& outDir)
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
}

//==============================================================================
//==============================================================================
bool BPowerUserRage::shouldForceUpdate(const BVector& oldDir, const BVector& newDir)
{
   if (oldDir.length() < cFloatCompareEpsilon || newDir.length() < cFloatCompareEpsilon)
      return true;

   if (fabs(oldDir.angleBetweenVector(newDir)) > cPiOver2)
      return true;

   return false;
}

//==============================================================================
//==============================================================================
bool BPowerUserRage::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(!mInitialized)
      return false;

   if(!mpUser)
   {
      BFAIL("no mpUser in BPowerUserRage::handleInput");
      return false;
   }

   BCamera* pCamera = mpUser->getCamera();
   if(!pCamera)
   {
      BFAIL("no pCamera available in BPowerUserRage::handleInput");
      return false;
   }

   bool start = (event == cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(mpUser->getOption_ControlScheme());
   bool modifyAction    = mpUser->getFlagModifierAction();

   if (pInputInterface->isFunctionControl( BInputInterface::cInputTranslation, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
      BVector newDir = cOriginVector;
      getCameraRelativeInputDir(*pCamera, detail.mX, detail.mY, newDir);
      mForceCommandNextUpdate = shouldForceUpdate(mMoveInputDir, newDir);
      mMoveInputDir = newDir;
      return true;
   }
   else if(pInputInterface->isFunctionControl( BInputInterface::cInputZoom, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
      BVector newDir = cOriginVector;
      getCameraRelativeInputDir(*pCamera, detail.mX, detail.mY, newDir);
      newDir.y = 0.0f;
      newDir.safeNormalize();
      mForceCommandNextUpdate = shouldForceUpdate(mAttackInputDir, newDir);
      mAttackInputDir = newDir;
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
       pInputInterface->isFunctionControl( BInputInterface::cInputAbility, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( start )
      {
#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerUserRage::handleInput, mID %d, sending shutdown command", gWorld->getGametimeFloat(), mID);
#endif
         cancelPower();
      }
      return( true );
   }

   return( false );
}

//==============================================================================
//==============================================================================
void BPowerUserRage::update(float elapsedTime)
{
   // validate the most basic of basic checks to make sure the squad is alive
   if (!mFlagNoCost)
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

   // Additional restrictions based on distance from owner, if applicable.
   const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
   const BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pLeaderUnit)
      return;

   if (!mHintCompleted)
   {
      // refresh has attacked boolean if it's not set
      if (!mHasAttacked && gWorld && gWorld->getPowerManager())
      {
         const BPowerRage* pPowerRage = reinterpret_cast<const BPowerRage*>(gWorld->getPowerManager()->getPowerByUserID(getID()));
         if (pPowerRage && pPowerRage->getHasSuccessfullyAttacked())
            mHasAttacked = true;
      }

      if (mHasMoved && mHasAttacked)
      {
         BHintMessage* pHintMessage = gWorld->getHintManager()->getHint();
         if (pHintMessage && pHintMessage->getHintStringID() == cHintId)
         {
            gWorld->getHintManager()->removeHint(pHintMessage);
            gUIManager->hideHint();
         }
         mHintCompleted = true;
      }

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

   BVector ownerPos = pLeaderUnit->getPosition();
    // Find height at position.
   gTerrainSimRep.getHeightRaycast(ownerPos, ownerPos.y, true);
   //pCamera->lookAtEntity(pLeaderUnit->getForward(), pLeaderUnit->getPosition(), mCameraHeight, mCameraZoom, 0.0f, 0.0f, true, true, true, elapsedTime);
   mpUser->setFlagUpdateHoverPoint(true);

   if (gWorld->getGametime() >= mTimestampNextCommand || mForceCommandNextUpdate)
   {
      mForceCommandNextUpdate = false;

      // If we have move input, process it here then reset it.
      if (!mMoveInputDir.almostEqual(mLastMoveDir))
      {
         mLastMovePos = ownerPos;
         mLastMoveDir = mMoveInputDir;

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
         c->setVector(mMoveInputDir);
         c->setPowerUserID(mID);
         c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
         // submit command
         gWorld->getCommandManager()->addCommandToExecute( c );

#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerUserRage::handleInput, mID %d, sent command to move to %.2f %.2f %.2f", gWorld->getGametimeFloat(), mID, newPos.x, newPos.y, newPos.z);
#endif
         mTimestampNextCommand = gWorld->getGametime() + mCommandInterval;
         mHasMoved = true;
      }

      // If we have attack input, process it here then reset it.
      if (mAttackInputDir.x < -cFloatCompareEpsilon || mAttackInputDir.x > cFloatCompareEpsilon || mAttackInputDir.z < -cFloatCompareEpsilon || mAttackInputDir.z > cFloatCompareEpsilon)
      {
         BPowerInputCommand* c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( mpUser->getPlayerID(), cCommandPowerInput );
         if (!c)
            return;

         mLastAttackDir = mAttackInputDir;

         // Set up the command.
         long playerID = mpUser->getPlayerID();
         c->setSenders( 1, &playerID );
         c->setSenderType( BCommand::cPlayer );
         c->setRecipientType( BCommand::cPlayer );
         c->setType( BPowerInputCommand::cTypeDirection );
         // Set the data that will be poked in.
         c->setVector(mAttackInputDir);
         c->setPowerUserID(mID);
         c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
         // submit command
         gWorld->getCommandManager()->addCommandToExecute( c );

#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerUserRage::handleInput, mID %d, sent command to attack direction %.2f %.2f %.2f", gWorld->getGametimeFloat(), mID, mAttackInputDir.x, mAttackInputDir.y, mAttackInputDir.z);
#endif
         mAttackInputDir.zero();
         mTimestampNextCommand = gWorld->getGametime() + mCommandInterval;
      }
   }  
}

//==============================================================================
//==============================================================================
void BPowerUserRage::updateUI()
{
#ifndef BUILD_FINAL
   if (gConfig.isDefined("rageShowDebug"))
   {
      const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
      const BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
      if (!pLeaderUnit)
         return;

      gpDebugPrimitives->addDebugSphere(mLastMovePos, 3.0f, cDWORDBlue);
      gpDebugPrimitives->addDebugLine(pLeaderUnit->getPosition() + BVector(0, 5.0, 0), pLeaderUnit->getPosition() + (mLastAttackDir * mScanRadius) + BVector(0, 5.0, 0), cDWORDRed);

      BMatrix matrix;
      BSimHelper::calculateDebugRenderMatrix(pLeaderUnit->getPosition(), pLeaderUnit->getForward(), pLeaderUnit->getUp(), pLeaderUnit->getRight(), 1.0f, matrix);
      gpDebugPrimitives->addDebugCircle(matrix, mScanRadius, cDWORDGreen);
   }
#endif

   // Bomb check.
   BCamera* pCamera = mpUser->getCamera();
   if(!pCamera)
   {
      BFAIL("no pCamera available in BPowerUserRage::update");
      return;
   }

   // Additional restrictions based on distance from owner, if applicable.
   const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
   const BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pLeaderUnit)
      return;

   BVector ownerPos = pLeaderUnit->getInterpolatedPosition();
   // Find height at position.
   //gTerrainSimRep.getHeightRaycast(ownerPos, ownerPos.y, true);

   BVector newCameraPos = (ownerPos - (pCamera->getCameraDir() * mCameraZoom));
   pCamera->setCameraLoc(newCameraPos);
   mpUser->setCameraHoverPoint(ownerPos);
}

//==============================================================================
//==============================================================================
void BPowerUserRage::setupUser()
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
bool BPowerRage::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mNextTickTime = 0;
   mTargettedSquad = cInvalidObjectID;
   mLastDirectionInput = cOriginVector;
   mTeleportDestination = cOriginVector;
   mPositionInput = cOriginVector;
   mTimeUntilTeleport = -1.0f;
   mTimeUntilRetarget = -1.0f;
   mAttackSound = cInvalidCueIndex;
   mJumpSplineCurve.reset();
   mCameraEffectData.reset();
   mCostPerTick.zero();
   mCostPerTickAttacking.zero();
   mCostPerJump.zero();
   mTickLength = 0.0f;
   mDamageMultiplier = 0.0f;
   mDamageTakenMultiplier = 0.0f;
   mSpeedMultiplier = 0.0f;
   mNudgeMultiplier = 0.0f;
   mScanRadius = 0.0f;
   mProjectileObject = cInvalidProtoObjectID;
   mHandAttachObject = cInvalidProtoObjectID;
   mTeleportAttachObject = cInvalidProtoObjectID;
   mAudioReactionTimer = 0.0f;
   mTeleportTime = 0.0f;
   mTeleportLateralDistance = 0.0f;
   mTeleportJumpDistance = 0.0f;
   mTimeBetweenRetarget = 0.0f;
   mMotionBlurAmount = 0.0f;
   mMotionBlurDistance = 0.0f;
   mMotionBlurTime = 0.0f;
   mDistanceVsAngleWeight = 0.0f;
   mCompletedInitialization = false;
   mHasSuccessfullyAttacked = false;
   mHealPerKillCombatValue = 0.0f; 
   mAuraRadius = 0.0f;
   mAuraDamageBonus = 0.0f;
   mAuraAttachObjectSmall = cInvalidObjectID;
   mAuraAttachObjectMedium = cInvalidObjectID;
   mAuraAttachObjectLarge = cInvalidObjectID;
   mHealAttachObject = cInvalidObjectID;
   mSquadsInAura.clear();
   mFilterTypeID = cInvalidObjectTypeID;
   mUsePather = false;

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerRage::init, playerID %d, powerUserID %d, ownerSquadID %d", gWorld->getGametimeFloat(), playerID, powerUserID, ownerSquadID);
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
   float suppliesPerTickAttacking = 0.0f;
   float suppliesPerJump = 0.0f;
   mFlagIgnoreAllReqs = ignoreAllReqs;
   bool bSuccess = true;

   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "TickLength", mTickLength));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "SuppliesPerTick", suppliesPerTick));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "SuppliesPerTickAttacking", suppliesPerTickAttacking));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "SuppliesPerJump", suppliesPerJump));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "DamageMultiplier", mDamageMultiplier));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "DamageTakenMultiplier", mDamageTakenMultiplier));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "SpeedMultiplier", mSpeedMultiplier));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "NudgeMultiplier", mNudgeMultiplier));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "ScanRadius", mScanRadius));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "TeleportTime", mTeleportTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "TeleportLateralDistance", mTeleportLateralDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "TeleportJumpDistance", mTeleportJumpDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "TimeBetweenRetarget", mTimeBetweenRetarget));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MotionBlurAmount", mMotionBlurAmount));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MotionBlurDistance", mMotionBlurDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MotionBlurTime", mMotionBlurTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "DistanceVsAngleWeight", mDistanceVsAngleWeight));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "Projectile", mProjectileObject));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "HandAttachObject", mHandAttachObject));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "TeleportAttachObject", mTeleportAttachObject));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "AuraAttachFxSmall", mAuraAttachObjectSmall));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "AuraAttachFxMedium", mAuraAttachObjectMedium));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "AuraAttachFxLarge", mAuraAttachObjectLarge));
   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "HealAttachFx", mHealAttachObject));
   bSuccess = (bSuccess && pProtoPower->getDataObjectType(powerLevel, "AuraFilterType", mFilterTypeID));

   // optional data
   pProtoPower->getDataFloat(powerLevel, "HealPerKillCombatValue", mHealPerKillCombatValue);
   pProtoPower->getDataFloat(powerLevel, "AuraRadius", mAuraRadius);
   pProtoPower->getDataFloat(powerLevel, "AuraDamageBonus", mAuraDamageBonus);
   pProtoPower->getDataSound(powerLevel, "AttackSound", mAttackSound);  //not being used, currently

   BASSERTM(mDistanceVsAngleWeight > 0.0f && mDistanceVsAngleWeight < 1.0f, "DistanceVsAngleWeight must be > 0 and < 1!");

   if (!bSuccess)
   {
      BFAIL("Missing or invalid data attribute.");
      shutdown();
      return (false);
   }

   // fail if squad is in an invalid location
   if(!BPowerHelper::checkPowerLocation(playerID, pOwnerSquad->getPosition(), this, true))
   {
      shutdown();
      return false;
   }

   mCostPerTick.add(gDatabase.getRIDSupplies(), suppliesPerTick);
   mCostPerTickAttacking.add(gDatabase.getRIDSupplies(), suppliesPerTickAttacking);
   mCostPerJump.add(gDatabase.getRIDSupplies(), suppliesPerJump);

   // setup camera effects
   mCameraEffectData.mBlurFactorTable.clear();
   mCameraEffectData.mBlurFactorTable.addKeyValue(0.0f, 0.0f);
   mCameraEffectData.mBlurFactorTable.addKeyValue(mMotionBlurTime, mMotionBlurAmount);
   mCameraEffectData.mBlurFactorTable.addKeyValue(mMotionBlurTime * 2.0f, 0.0f);
   mCameraEffectData.mRadialBlur = false;

   if (mPowerUserID == cInvalidPowerUserID)
      mUsePather = true;

   // basics
   mElapsed = 0.0f;

   if (mUsePather)
      mPositionInput = pOwnerSquad->getPosition();
   else
      mPositionInput = cOriginVector;

   // pay the cost first, bail if we can't
   if (!mFlagIgnoreAllReqs && !pPlayer->canUsePower(mProtoPowerID, ownerSquadID))
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerRage::init, shutting down because player can't pay startup cost", gWorld->getGametimeFloat());
#endif
      shutdown();
      return false;
   }

   pPlayer->usePower(mProtoPowerID, mOwnerID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

   // apply the damage and speed modifications
   if (pOwnerSquad->getParentPlatoon())
      pOwnerSquad->getParentPlatoon()->removeAllOrders();
   pOwnerSquad->removeAllOrders();
   pOwnerSquad->setAbilityDamageModifier(mDamageMultiplier, false);
   pOwnerSquad->setAbilityDamageTakenModifier(mDamageTakenMultiplier, false);
   pOwnerSquad->setAbilityMovementSpeedModifier(mSpeedMultiplier, false);
   pOwnerSquad->setFlagSprinting(true);
   pOwnerSquad->addEventListener(this);

   // we need to manually mark the event listener action as persistent, 
   // because it has to stick around while we're using rage - I would think that it should always be persistent, 
   // but I don't think a change like that is a good idea this late in the game
   BEntityActionListen* pListenAction = reinterpret_cast<BEntityActionListen*>(pOwnerSquad->getActionByType(BAction::cActionTypeEntityListen));
   if (pListenAction)
      pListenAction->setFlagPersistent(true);

   BUnit* pUnit = pOwnerSquad->getLeaderUnit();
   if (pUnit && pUnit->getVisual())
   {
      long boneHandle = pUnit->getVisual()->getBoneHandle("BoneHiltR");
      pUnit->addAttachment(mHandAttachObject, boneHandle); 
      boneHandle = pUnit->getVisual()->getBoneHandle("BoneHiltL");
      pUnit->addAttachment(mHandAttachObject, boneHandle); 
   }

   if (pOwnerSquad->getSquadAI())
      pOwnerSquad->getSquadAI()->setFlagCanWork(false);

   // add in the rage action after the change mode action
   BUnitActionRage* pRageAction = (BUnitActionRage*)gActionManager.createAction(BAction::cActionTypeUnitRage);
   if (!pRageAction || !pUnit || !pUnit->addAction(pRageAction))
   {
      shutdown();
      return false;
   }
   pRageAction->setTeleportObject(mTeleportAttachObject);
   pRageAction->setUsePather(mUsePather);

   // start the change mode action
   BSquadActionChangeMode* pAction=(BSquadActionChangeMode*)gActionManager.createAction(BAction::cActionTypeSquadChangeMode);
   if (!pAction)
   {
      shutdown();
      return false;
   }
   pAction->setSquadMode(BSquadAI::cModePower);
   if (!pOwnerSquad->addAction(pAction))
   {
      shutdown();
      return false;
   }

   mCompletedInitialization = true;
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerRage::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerRage::shutdown, mID %d, mPowerUserID %d", gWorld->getGametimeFloat(), mID, mPowerUserID);
#endif

   // apply the damage and speed modifications
   BSquad* pOwnerSquad=gWorld->getSquad(mOwnerID);
   if (pOwnerSquad && mCompletedInitialization)
   {
      pOwnerSquad->setAbilityDamageModifier(mDamageMultiplier, true);
      pOwnerSquad->setAbilityDamageTakenModifier(mDamageTakenMultiplier, true);
      pOwnerSquad->setAbilityMovementSpeedModifier(mSpeedMultiplier, true);
      pOwnerSquad->setFlagSprinting(false);

      // unset the persist flag on the event listener
      BEntityActionListen* pListenAction = reinterpret_cast<BEntityActionListen*>(pOwnerSquad->getActionByType(BAction::cActionTypeEntityListen));
      if (pListenAction)
         pListenAction->setFlagPersistent(false);
      pOwnerSquad->removeEventListener(this);

      BUnit* pUnit = pOwnerSquad->getLeaderUnit();
      if (pUnit)
      {
         pUnit->removeAttachmentsOfType(mHandAttachObject);
      }

      if (pOwnerSquad->getParentPlatoon())
         pOwnerSquad->getParentPlatoon()->removeAllOrders();
      pOwnerSquad->removeAllOrders();

      if (pOwnerSquad->getSquadAI())
         pOwnerSquad->getSquadAI()->setFlagCanWork(true);

      if (pUnit)
         pUnit->removeAllActionsOfType(BAction::cActionTypeUnitRage);

      if (pUnit && !pUnit->getFlagAnimationDisabled())
      {
         // start the change mode action
         BSquadActionChangeMode* pAction=(BSquadActionChangeMode*)gActionManager.createAction(BAction::cActionTypeSquadChangeMode);
         if (!pAction)
            return false;
         pAction->setSquadMode(BSquadAI::cModeNormal);
         if (!pOwnerSquad->addAction(pAction))
            return false;
      }
      else 
      {
         BASSERT(pOwnerSquad->getSquadAI());
         pOwnerSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);
      }
   }

   handleSquadsLeavingAura(mSquadsInAura);

   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID)
   {
      pUser->getPowerUser()->shutdown();

      // make sure to clear the camera effect so we aren't referencing our camera effect
      if (pUser->getCamera())
         pUser->getCamera()->clearCameraEffect(gWorld->getSubGametime(), 1000, 0);
   }

   mFlagDestroy = true;

   return true;
}

//==============================================================================
//==============================================================================
void BPowerRage::update(DWORD currentGameTime, float lastUpdateLength)
{
   currentGameTime;

   if(getFlagDestroy())
      return;

   // bomb checks
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pPlayer || !pOwnerSquad || !pOwnerSquad->isAlive() || !pLeaderUnit || !pLeaderUnit->isAlive() || pOwnerSquad->getFlagAttackBlocked() || pOwnerSquad->isGarrisoned())
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerRage::update, bomb check failed, shutting down", gWorld->getGametimeFloat());
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
   // fail if squad is in an invalid location
   if(!BPowerHelper::checkPowerLocation(mPlayerID, pOwnerSquad->getPosition(), this, true))
   {
      shutdown();
      return;
   }
   // fail if we can't get the rage action
   BUnitActionRage *pRageAction = reinterpret_cast<BUnitActionRage*>(pLeaderUnit->getActionByType(BAction::cActionTypeUnitRage));
   if (!pRageAction)
   {
      shutdown();
      return;
   }

   if(mAudioReactionTimer <= 0.0f)
   {
      gWorld->createPowerAudioReactions(getPlayerID(), cSquadSoundChatterReactPowRage, mTargetLocation, 60.0f, mOwnerID);
      mAudioReactionTimer = cAudioReactionInterval;
   }
   else
      mAudioReactionTimer -= lastUpdateLength;

   // clear our target squad if it's invalid or dead
   if (mTargettedSquad != cInvalidObjectID)
   {
      BSquad* pTargettedSquad = gWorld->getSquad(mTargettedSquad);
      if (!pTargettedSquad || !pTargettedSquad->isAlive())
         mTargettedSquad = cInvalidObjectID;
   }

   // update our aura
   updateAura(pLeaderUnit->getPosition());

   if (mTimeUntilRetarget > 0.0f)
      mTimeUntilRetarget -= lastUpdateLength;

   // see if we should teleport
   if (mTimeUntilTeleport > 0.0f)
   {
      mTimeUntilTeleport -= lastUpdateLength;

      if (mTimeUntilTeleport <= 0.0f)
      {
         // create the instant damage projectile
         BProtoAction* pProtoAction = getImpactProtoAction();
         BSquad* pTargettedSquad = gWorld->getSquad(mTargettedSquad);
         if (pProtoAction && pTargettedSquad)
         {
            // create the projectile that will do damage
            BObjectCreateParms parms;
            parms.mPlayerID = mPlayerID;
            parms.mProtoObjectID = pProtoAction->getProjectileID();
            parms.mPosition = mTeleportDestination;
            BEntityID projectileID = cInvalidObjectID;
            BUnit* pTargetUnit = pTargettedSquad->getLeaderUnit();
            if (pTargetUnit)
               projectileID = gWorld->launchProjectile(parms, XMVectorZero(), XMVectorZero(), XMVectorZero(), pProtoAction->getDamagePerSecond(), pProtoAction, pProtoAction, pLeaderUnit->getID(), pTargetUnit, -1, false, true, true);
            else
               projectileID = gWorld->launchProjectile(parms, pTargettedSquad->getPosition(), XMVectorZero(), XMVectorZero(), pProtoAction->getDamagePerSecond(), pProtoAction, pProtoAction, pLeaderUnit->getID(), NULL, -1, false, true, true);
            BProjectile* pProj = gWorld->getProjectile(projectileID);
            if(pProj)
            {
               pProj->setOwningPowerID(getID());
               if (pTargetUnit)
                  pProj->stickToTarget(mTeleportDestination, pTargetUnit);
            }
         }

         pLeaderUnit->setFlagSkipMotionExtraction(false);
         pRageAction->teleportToAndAttack(mTeleportDestination, mTargettedSquad);

         if (mUsePather)   
            mPositionInput = mTeleportDestination;
         else
            mPositionInput = cOriginVector;
      }
      else
      {
         // don't teleport and attack on this frame, move the unit closer though
         float parametricProgress = (mTeleportTime - mTimeUntilTeleport) / mTeleportTime;
         // parametricProgress is now a number starting from 0.0 and ending at 1.0, we want it to end at 0.5
         parametricProgress *= 0.5f;
         BVector newPos = mJumpSplineCurve.evaluate(parametricProgress);
         pLeaderUnit->setPosition(newPos);

         // orient correctly
         BVector direction = mTeleportDestination - newPos;
         direction.y = 0;
         direction.normalize();
         if (direction.lengthSquared() > 0)
         {
            pLeaderUnit->setForward(direction);
            pLeaderUnit->calcRight();
            pLeaderUnit->calcUp();
         }
      }
   }

   mLastDirectionInput = cOriginVector;

   // iterate damage ticks
   mElapsed += lastUpdateLength;
   while(mElapsed > mNextTickTime)
   {
      const BCost* pCostThisTick = getCost();

      // we're done if the player can't pay the cost, otherwise pay it
      if(!mFlagIgnoreAllReqs && !pPlayer->checkCost(pCostThisTick))
      {
#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerRage::update, shutting down because player ran out of resources", gWorld->getGametimeFloat());
#endif
         if (BPowerHelper::getPowerUser(*this))
            gUI.playPowerNotEnoughResourcesSound();

         shutdown();
         return;
      }

      if (!mFlagIgnoreAllReqs)
         pPlayer->payCost(pCostThisTick);

      // increment damage time
      mNextTickTime += mTickLength;
   }
}

//==============================================================================
//==============================================================================
void BPowerRage::updateAura(const BVector& location)
{
   if (mAuraRadius < cFloatCompareEpsilon)
      return;

   BUnitQuery unitQuery(location, mAuraRadius, true);
   unitQuery.setRelation(mPlayerID, cRelationTypeAlly);

   if (mFilterTypeID != cInvalidObjectTypeID)
      unitQuery.addObjectTypeFilter(mFilterTypeID);

   BEntityIDArray squadsInArea(0, 32);
   gWorld->getSquadsInArea(&unitQuery, &squadsInArea, false);

   // Some other helper arrays.
   BEntityIDArray enteringSquads(0, 32);
   BEntityIDArray leavingSquads(0, 32); 

   BSimHelper::diffEntityIDArrays(squadsInArea, mSquadsInAura, &enteringSquads, &leavingSquads, NULL);
   mSquadsInAura = squadsInArea;
   handleSquadsEnteringAura(enteringSquads);
   handleSquadsLeavingAura(leavingSquads);
}

//==============================================================================
//==============================================================================
void BPowerRage::handleSquadsEnteringAura(const BEntityIDArray& squads)
{
   for (uint i = 0; i < squads.getSize(); ++i)
   {
      if (squads[i] == mOwnerID)
         continue;

      BSquad* pSquad = gWorld->getSquad(squads[i]);
      if (!pSquad)
         continue;

      pSquad->setAbilityDamageModifier(mAuraDamageBonus, false);
      
      for (uint j = 0; j < pSquad->getNumberChildren(); ++j)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
         if (!pUnit)
            continue;

         // /cry :'(
         BProtoObjectID auraId = cInvalidObjectID;
         float obsSize = pUnit->getObstructionRadius();
         if (obsSize < 2.9f)
            auraId = mAuraAttachObjectSmall;
         else if (obsSize < 7.9f)
            auraId = mAuraAttachObjectMedium;
         else
            auraId = mAuraAttachObjectLarge;

         BEntityID attachmentID = gWorld->createEntity(auraId, false, mPlayerID, pUnit->getSimCenter(), pUnit->getForward(), pUnit->getRight(), true);
         BObject* pAttachmentObject = gWorld->getObject(attachmentID);
         BASSERT(pAttachmentObject);
         pUnit->addAttachment(pAttachmentObject, -1, -1, false, true);
      }
   }
}

//==============================================================================
//==============================================================================
void BPowerRage::handleSquadsLeavingAura(const BEntityIDArray& squads)
{
   for (uint i = 0; i < squads.getSize(); ++i)
   {
      if (squads[i] == mOwnerID)
         continue;

      BSquad* pSquad = gWorld->getSquad(squads[i]);
      if (!pSquad)
         continue;

      pSquad->setAbilityDamageModifier(mAuraDamageBonus, true);

      for (uint j = 0; j < pSquad->getNumberChildren(); ++j)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
         if (!pUnit)
            continue;

         pUnit->removeAttachmentsOfType(mAuraAttachObjectSmall);
         pUnit->removeAttachmentsOfType(mAuraAttachObjectMedium);
         pUnit->removeAttachmentsOfType(mAuraAttachObjectLarge);
      }
   }
}

//==============================================================================
//==============================================================================
bool BPowerRage::submitInput(BPowerInput powerInput)
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerRage::submitInput, mID %d, mPowerUserID %d, input type %d", gWorld->getGametimeFloat(), mID, mPowerUserID, powerInput.mType);
#endif

   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return false;

   BSquad* pOwnerSquad=gWorld->getSquad(mOwnerID);
   if (!pOwnerSquad)
      return false;

   BUnit* pLeaderUnit=pOwnerSquad->getLeaderUnit();
   if (!pLeaderUnit)
      return false;

   if (powerInput.mType == PowerInputType::cUserCancel)
   {
      // The user cancelled the power.
      shutdown();
      return (true);
   }

   BUnitActionRage *pRageAction = reinterpret_cast<BUnitActionRage*>(pLeaderUnit->getActionByType(BAction::cActionTypeUnitRage));
   if (!pRageAction)
      return false;

   // we process no input other than cancel while we are changing modes
   if (pOwnerSquad->getFlagChangingMode())
      return true;

   if (powerInput.mType == PowerInputType::cDirection)
   {
      mLastDirectionInput = powerInput.mVector;
      if (mTimeUntilTeleport <= 0.0f && mTimeUntilRetarget <= 0.0f)
      {
         // if you can't afford to jump, bail
         if (!mFlagIgnoreAllReqs)
         {
            if (!pPlayer->checkCost(&mCostPerJump))
            {
               if (BPowerHelper::getPowerUser(*this))
                  gUI.playPowerNotEnoughResourcesSound();

               shutdown();
               return false;
            }
            else
            {
               if (getNewTarget())
                  pPlayer->payCost(&mCostPerJump);
            }
         }
         else
         {
            getNewTarget();
         }
      }
      return true;
   }
   else if (powerInput.mType == PowerInputType::cPosition)
   {
      mPositionInput = powerInput.mVector;
      // we're not teleporting, so allow controls
      if (mTimeUntilTeleport <= 0.0f)
      {
         // do move controls
         pOwnerSquad->removeAllActionsOfType(BAction::cActionTypeSquadAttack);
         pOwnerSquad->removeAllActionsOfType(BAction::cActionTypeSquadMove);
         mTargettedSquad = cInvalidObjectID;

         if (mUsePather)
            pRageAction->setMoveTarget(mPositionInput);
         else
         {
            if (mPositionInput.length() < cFloatCompareEpsilon)
               pRageAction->stopMoving();
            pRageAction->setMoveDirection(mPositionInput);
         }
      }

      return true;
   }
   // Insert handling for additional power input types here.

   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerRage::submitInput() received powerInput of unsupported type.");
   return (false);
}

//==============================================================================
//==============================================================================
bool BPowerRage::getNewTarget()
{
   // this is a direction to search for a squad to attack 
   BSquad* pOwnerSquad=gWorld->getSquad(mOwnerID);
   if (!pOwnerSquad)
      return false;

   BUnitQuery unitQuery(pOwnerSquad->getPosition(), mScanRadius, true);
   unitQuery.setRelation(mPlayerID, cRelationTypeEnemy);
   BEntityIDArray squadsInArea(0, 32);
   gWorld->getSquadsInArea(&unitQuery, &squadsInArea, false);

   // if we don't have a direction, grab the highest combat value 
   BVector ownerPos = pOwnerSquad->getPosition();
   BSquad* pBestSquad = NULL;
   float bestWeight = 1.0f;
   float maxAngle = cPi / 2.0f;
   float highestCombatValue = -1.0f;
   bool checkAngle = (mLastDirectionInput.length() > 0.01);

   // iterate through them to find the one closest to the 
   // direction and distance of our desired direction
   uint numSquadsInArea = squadsInArea.getSize();
   for (uint i = 0; i < numSquadsInArea; ++i)
   {
      BSquad* pTargetSquad = gWorld->getSquad(squadsInArea[i]);
      if (pTargetSquad && pOwnerSquad->canAttackTarget(pTargetSquad->getLeader()))
      {
         if (checkAngle) 
         {
            // check angle compared to user's input direction
            float distance = pOwnerSquad->calculateXZDistance(pTargetSquad);
            if (distance < mScanRadius)
            {
               BVector xzDirectionToSquad = pTargetSquad->getPosition() - ownerPos;
               xzDirectionToSquad.y = 0.0f;
               float angle = fabs(xzDirectionToSquad.angleBetweenVector(mLastDirectionInput));
               if (angle < maxAngle)
               {
                  // compute the weight and see if it's better than what we've got
                  float weight = ((distance / mScanRadius) * mDistanceVsAngleWeight) + ((angle / maxAngle) * (1.0f - mDistanceVsAngleWeight));
                  if (weight < bestWeight)
                  {
                     bestWeight = weight;
                     pBestSquad = pTargetSquad;
                  }
               }
            }
         }
         else 
         {
            // check combat value
            float combatValue = pTargetSquad->getCombatValue();
            if (combatValue > highestCombatValue)
            {
               highestCombatValue = combatValue;
               pBestSquad = pTargetSquad;
            }
         }
      }
   }

   if (!pBestSquad)
   {
      mTargettedSquad = cInvalidObjectID;
      return false;
   }

   if (pBestSquad->getID() == mTargettedSquad)
      return false;

   BUnit* pLeaderUnit = pOwnerSquad->getLeaderUnit();
   if (!pLeaderUnit)
      return false;

   BUnitActionRage *pRageAction = reinterpret_cast<BUnitActionRage*>(pLeaderUnit->getActionByType(BAction::cActionTypeUnitRage));
   if (!pRageAction)
      return false; 

   bool canBoard = pRageAction->canBoard(*pBestSquad);

   // if you can board this target, don't do all the placement checks
   BVector targetPos = cInvalidVector;
   if (!canBoard)
   {
      // Use the squad plotter to figure out where we are going to stand if we attack this target
      static BEntityIDArray squadIDs;
      squadIDs.resize(1);
      squadIDs[0] = pOwnerSquad->getID();
      static BDynamicSimVectorArray waypoints;
      waypoints.resize(1);
      waypoints[0] = pLeaderUnit->getPosition();

      bool haveValidPlots = gSquadPlotter.plotSquads(squadIDs, mPlayerID, pBestSquad->getID(), waypoints, pBestSquad->getPosition(), -1, BSquadPlotter::cSPFlagForceLandMovementType);
      const BDynamicSimArray<BSquadPlotterResult>& results = gSquadPlotter.getResults();
      if (!haveValidPlots || results.getSize() < 1)
         return false;

      bool foundValidResult = false;
      for (uint i = 0; i < results.getSize(); ++i)
      {
         if (!results[i].getDefaultDesiredPos())
         {
            targetPos = results[i].getDesiredPosition();
            foundValidResult = true;
            break;
         }
      }

      if (!foundValidResult)
         return false;

      // hacktastic hack to avoid getting in the middle of things when the squad plotter gives us a terrible location
      float smallestObstructionSize = Math::Min(pBestSquad->getObstructionRadiusX(), pBestSquad->getObstructionRadiusZ());
      if ((pBestSquad->getPosition() - targetPos).length() < smallestObstructionSize)
      {
         // force to make sure we don't overlap 
         // if we're in a bad place, pick the safest distance - both of the obstruction radii
         float distance = pBestSquad->getObstructionRadius() + pOwnerSquad->getObstructionRadius();
         BVector direction = pLeaderUnit->getPosition() - pBestSquad->getPosition();
         direction.normalize();
         targetPos = pBestSquad->getPosition() + (direction * distance);
      }
   }
   else
   {
      const BUnit* pTargetLeaderUnit = pBestSquad->getLeaderUnit();
      if (pTargetLeaderUnit)
         targetPos = pTargetLeaderUnit->getPosition();
      else
         targetPos = pBestSquad->getPosition();
   }

   // if we found a target, attack it
   mTargettedSquad = pBestSquad->getID();

   pRageAction->stopMoving();

   // make sure our target pos is correct and on the terrain 
   gTerrainSimRep.getHeightRaycast(targetPos, targetPos.y, true);

   mTeleportDestination = targetPos;
   mTimeUntilTeleport = mTeleportTime;
   if (pOwnerSquad->getParentPlatoon())
      pOwnerSquad->getParentPlatoon()->removeAllOrders();
   pOwnerSquad->removeAllOrders();
   pLeaderUnit->removeActions();
   long boneHandle = pLeaderUnit->getVisual()->getBoneHandle("Bip01");
   pLeaderUnit->addAttachment(mTeleportAttachObject, boneHandle);

   // setup the spline for the jump
   BVector direction = mTeleportDestination - pLeaderUnit->getPosition();
   direction.y = 0;
   float xzDistanceToMiddle = Math::Min(mTeleportLateralDistance, (direction.length() / 2));
   direction.normalize();

   BObject* pAttachedObject = pLeaderUnit->getAttachedToObject();
   BVector startPosition = (pAttachedObject) ? pAttachedObject->getPosition() : pLeaderUnit->getPosition();

   BVector middlePosition = pLeaderUnit->getPosition() + (direction * xzDistanceToMiddle);
   middlePosition.y = Math::Max(pBestSquad->getPosition().y, middlePosition.y);
   middlePosition.y += mTeleportJumpDistance;
   mJumpSplineCurve.init(startPosition, middlePosition, pBestSquad->getPosition());

   if (direction.length() > cFloatCompareEpsilon)
   {
      pLeaderUnit->setForward(direction);
      pLeaderUnit->calcRight();
      pLeaderUnit->calcUp();
   }

   // play the attacking sound
   if(mAttackSound != cInvalidCueIndex)
      gWorld->getWorldSoundManager()->addSound(pLeaderUnit->getPosition(), mAttackSound, true, cInvalidCueIndex, true, false);

   // play the "in air" animation we want
   BSimOrder* pOrder = gSimOrderManager.createOrder();
   if (pOrder)
   {
      pOwnerSquad->removeAllOrders();
      pLeaderUnit->clearAnimationState();

      // no motion extraction while we're doing this, in case movement gets in the way for some reason
      pLeaderUnit->setFlagSkipMotionExtraction(true);
      pLeaderUnit->setAnimationRate(1.0f);
      pRageAction->startJump(mTeleportDestination, mTargettedSquad);
   }

   mTimeUntilRetarget = mTimeBetweenRetarget;

   // do the camera effect if we are the right user
   doDirectionalCameraBlurForOwner(pLeaderUnit->getPosition(), mTeleportDestination);

   mHasSuccessfullyAttacked = true;

   return true;
}

//==============================================================================
//==============================================================================
BProtoAction* BPowerRage::getImpactProtoAction() const
{
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
   {
      return(NULL);
   }

   BProtoObject* pProjProtoObj = pPlayer->getProtoObject(mProjectileObject);
   if (!pProjProtoObj)
   {
      return(NULL);
   }

   BTactic* pTactic = pProjProtoObj->getTactic();
   if (!pTactic)
   {
      return(NULL);
   }

   return(pTactic->getProtoAction(0, NULL, XMVectorZero(), NULL, XMVectorZero(), cInvalidObjectID, -1, false));
}

//==============================================================================
//==============================================================================
const BCost* BPowerRage::getCost() const
{
   BSquad* pOwnerSquad=gWorld->getSquad(mOwnerID);
   BASSERT(pOwnerSquad);

   // if the arbiter is attacking, then return the attacking cost, otherwise the default cost
   bool tempBool = false;
   if (isAttackingTarget(tempBool))
      return &mCostPerTickAttacking;
   else
      return &mCostPerTick;
}

//==============================================================================
//==============================================================================
void BPowerRage::doDirectionalCameraBlurForOwner(const BVector& startWorldPos, const BVector& endWorldPos) 
{
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID && pUser->getCamera())
   {
      // compute blurX and blurY
      BRenderViewParams view(gRender.getViewParams());
      BMatrix mat;
      pUser->getCamera()->getViewMatrix(mat);
      view.setViewMatrix(mat);

      BVector startScreen, endScreen;
      view.calculateWorldToScreen(startWorldPos, startScreen.x, startScreen.y);
      view.calculateWorldToScreen(endWorldPos, endScreen.x, endScreen.y);

      BVector blurVector = cOriginVector;
      blurVector.x = (endScreen.x - startScreen.x);
      blurVector.y = (endScreen.y - startScreen.y);

      blurVector.normalize();
      blurVector *= mMotionBlurDistance;

      mCameraEffectData.mBlurXYTable.clear();
      mCameraEffectData.mBlurXYTable.addKeyValue(0.0f, BVector2(0.0f, 0.0f));
      mCameraEffectData.mBlurXYTable.addKeyValue(mMotionBlurTime, BVector2(blurVector.x, blurVector.y));
      mCameraEffectData.mBlurXYTable.addKeyValue(mMotionBlurTime * 2.0f, BVector2(0.0f, 0.0f));

      pUser->getCamera()->beginCameraEffect(gWorld->getSubGametime(), &mCameraEffectData, &startWorldPos, 0);
   }
}

//==============================================================================
//==============================================================================
void BPowerRage::onUnitKilled(const BEntityID& unitId)
{
   BSquad* pOwnerSquad=gWorld->getSquad(mOwnerID);
   BASSERT(pOwnerSquad);

   // award repair if we have that on
   BUnit* pKilledUnit = gWorld->getUnit(unitId);
   if (mHealPerKillCombatValue > cFloatCompareEpsilon && pKilledUnit && pKilledUnit->getProtoObject())
   {
      float tempVal;
      pOwnerSquad->repairHitpoints(mHealPerKillCombatValue * pKilledUnit->getProtoObject()->getCombatValue(), true, tempVal);

      BUnit* pLeaderUnit = pOwnerSquad->getLeaderUnit();
      if (pLeaderUnit)
      {
         BEntityID attachmentID = gWorld->createEntity(mHealAttachObject, false, mPlayerID, pLeaderUnit->getSimCenter(), pLeaderUnit->getForward(), pLeaderUnit->getRight(), true);
         BObject* pAttachmentObject = gWorld->getObject(attachmentID);
         BASSERT(pAttachmentObject);
         pLeaderUnit->addAttachment(pAttachmentObject, -1, -1, false, true);
      }
   }

}

//==============================================================================
//==============================================================================
void BPowerRage::projectileImpact(BEntityID id, const BEntityIDArray& killedUnits, const BEntityIDArray& damagedUnits)
{
   BProjectile* pProjectile = gWorld->getProjectile(id);
   BASSERT(pProjectile);
   if (!pProjectile)
      return;

   BUnit* pProjTargetUnit = gWorld->getUnit(pProjectile->getTargetObjectID());
   if (!pProjTargetUnit)
      return;

   BSquad* pTargettedSquad = pProjTargetUnit->getParentSquad();
   if (!pTargettedSquad)
      return;

   // nudge the units in the squad
   for (uint i = 0; i < pTargettedSquad->getNumberChildren(); ++i)
   {
      BUnit* pTargettedUnit = gWorld->getUnit(pTargettedSquad->getChild(i));
      if (!pTargettedUnit)
         continue;

      if (!pTargettedUnit->getPhysicsObject())
         continue;

      BVector dirFromImpact = pTargettedUnit->getPosition() - mTeleportDestination;
      dirFromImpact.normalize();
      BVector nudgeLocation;
      pTargettedUnit->getPhysicsObject()->getCenterOfMassLocation(nudgeLocation);
      nudgeLocation -= (dirFromImpact * 10.0f / pTargettedUnit->getObstructionRadius());

      BVector impulseForce = dirFromImpact;
      impulseForce.y = 0.0f;
      impulseForce.normalize();
      impulseForce.y = 1.0f;
      impulseForce *= pTargettedUnit->getPhysicsObject()->getMass();
      impulseForce *= mNudgeMultiplier;
      pTargettedUnit->getPhysicsObject()->applyPointImpulse(impulseForce, nudgeLocation);
   }

}

//==============================================================================
//==============================================================================
bool BPowerRage::isAttackingTarget(bool& boarded) const
{
   const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   const BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pLeaderUnit)
      return false;

   const BUnitActionRage *pRageAction = reinterpret_cast<const BUnitActionRage*>(pLeaderUnit->getActionByTypeConst(BAction::cActionTypeUnitRage));
   if (!pRageAction)
      return false;

   boarded = pRageAction->isBoardedAttacking();
   return (boarded || pRageAction->isGroundAttacking());
}

//==============================================================================
//==============================================================================
bool BPowerRage::isJumping() const
{
   const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   const BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!pLeaderUnit)
      return false;

   const BUnitActionRage *pRageAction = reinterpret_cast<const BUnitActionRage*>(pLeaderUnit->getActionByTypeConst(BAction::cActionTypeUnitRage));
   if (!pRageAction)
      return false;

   return (pRageAction->isJumping());
}

//==============================================================================
//==============================================================================
void BPowerRage::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   // if we killed a unit, try and heal
   if (eventType == BEntity::cEventKilledUnit)
      onUnitKilled(senderID);
}

//==============================================================================
//==============================================================================
bool BPowerUserRage::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mCameraZoom);
   GFWRITEVAR(pStream, float, mCameraHeight);
   GFWRITEVAR(pStream, DWORD, mTimestampNextCommand);
   GFWRITEVECTOR(pStream, mMoveInputDir);
   GFWRITEVECTOR(pStream, mAttackInputDir);
   GFWRITEVAR(pStream, float, mTimeUntilHint);
   GFWRITEVECTOR(pStream, mLastMovePos);
   GFWRITEVECTOR(pStream, mLastMoveDir);
   GFWRITEVECTOR(pStream, mLastAttackDir);
   GFWRITEVAR(pStream, DWORD, mCommandInterval);
   GFWRITEVAR(pStream, float, mScanRadius);
   GFWRITEVAR(pStream, float, mMovementProjectionMultiplier);
   GFWRITEBITBOOL(pStream, mHasMoved);
   GFWRITEBITBOOL(pStream, mHasAttacked);
   GFWRITEBITBOOL(pStream, mHintShown);
   GFWRITEBITBOOL(pStream, mHintCompleted);
   GFWRITEBITBOOL(pStream, mForceCommandNextUpdate);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserRage::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mCameraZoom);
   GFREADVAR(pStream, float, mCameraHeight);
   GFREADVAR(pStream, DWORD, mTimestampNextCommand);
   GFREADVECTOR(pStream, mMoveInputDir);
   GFREADVECTOR(pStream, mAttackInputDir);
   GFREADVAR(pStream, float, mTimeUntilHint);
   GFREADVECTOR(pStream, mLastMovePos);
   GFREADVECTOR(pStream, mLastMoveDir);
   GFREADVECTOR(pStream, mLastAttackDir);
   GFREADVAR(pStream, DWORD, mCommandInterval);
   GFREADVAR(pStream, float, mScanRadius);
   if (BPowerUser::mGameFileVersion >= 3)
   {
      GFREADVAR(pStream, float, mMovementProjectionMultiplier);
   }
   else
   {
      mMovementProjectionMultiplier = 1.0f;
   }
   GFREADBITBOOL(pStream, mHasMoved);
   GFREADBITBOOL(pStream, mHasAttacked);
   GFREADBITBOOL(pStream, mHintShown);
   GFREADBITBOOL(pStream, mHintCompleted);
   if (BPowerUser::mGameFileVersion >= 6)
   {
      GFREADBITBOOL(pStream, mForceCommandNextUpdate);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerRage::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, double, mNextTickTime);
   GFWRITEVAR(pStream, BEntityID, mTargettedSquad);
   GFWRITEVECTOR(pStream, mLastDirectionInput);
   GFWRITEVECTOR(pStream, mTeleportDestination);
   GFWRITEVECTOR(pStream, mPositionInput);
   GFWRITEVAR(pStream, float, mTimeUntilTeleport);
   GFWRITEVAR(pStream, float, mTimeUntilRetarget);
   GFWRITEVAR(pStream, BCueIndex, mAttackSound);
   GFWRITECLASS(pStream, saveType, mJumpSplineCurve);
   GFWRITECLASS(pStream, saveType, mCameraEffectData);
   GFWRITECLASS(pStream, saveType, mCostPerTick);
   GFWRITECLASS(pStream, saveType, mCostPerTickAttacking);
   GFWRITECLASS(pStream, saveType, mCostPerJump);
   GFWRITEVAR(pStream, float, mTickLength);
   GFWRITEVAR(pStream, float, mDamageMultiplier);
   GFWRITEVAR(pStream, float, mDamageTakenMultiplier);
   GFWRITEVAR(pStream, float, mSpeedMultiplier);
   GFWRITEVAR(pStream, float, mNudgeMultiplier);
   GFWRITEVAR(pStream, float, mScanRadius);
   GFWRITEVAR(pStream, BProtoObjectID, mProjectileObject);
   GFWRITEVAR(pStream, BProtoObjectID, mHandAttachObject);
   GFWRITEVAR(pStream, BProtoObjectID, mTeleportAttachObject);
   GFWRITEVAR(pStream, float, mAudioReactionTimer);
   GFWRITEVAR(pStream, float, mTeleportTime);
   GFWRITEVAR(pStream, float, mTeleportLateralDistance);
   GFWRITEVAR(pStream, float, mTeleportJumpDistance);
   GFWRITEVAR(pStream, float, mTimeBetweenRetarget);
   GFWRITEVAR(pStream, float, mMotionBlurAmount);
   GFWRITEVAR(pStream, float, mMotionBlurDistance);
   GFWRITEVAR(pStream, float, mMotionBlurTime);
   GFWRITEVAR(pStream, float, mDistanceVsAngleWeight);
   GFWRITEVAR(pStream, float, mHealPerKillCombatValue);
   GFWRITEVAR(pStream, float, mAuraRadius);
   GFWRITEVAR(pStream, float, mAuraDamageBonus);
   GFWRITEVAR(pStream, BProtoObjectID, mAuraAttachObjectSmall);
   GFWRITEVAR(pStream, BProtoObjectID, mAuraAttachObjectMedium);
   GFWRITEVAR(pStream, BProtoObjectID, mAuraAttachObjectLarge);
   GFWRITEVAR(pStream, BProtoObjectID, mHealAttachObject);
   GFWRITEARRAY(pStream, BEntityID, mSquadsInAura, uint8, 200);
   GFWRITEVAR(pStream, BObjectTypeID, mFilterTypeID);
   GFWRITEBITBOOL(pStream, mCompletedInitialization);
   GFWRITEBITBOOL(pStream, mHasSuccessfullyAttacked);
   GFWRITEBITBOOL(pStream, mUsePather);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerRage::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, double, mNextTickTime);
   GFREADVAR(pStream, BEntityID, mTargettedSquad);
   GFREADVECTOR(pStream, mLastDirectionInput);
   GFREADVECTOR(pStream, mTeleportDestination);
   GFREADVECTOR(pStream, mPositionInput);
   GFREADVAR(pStream, float, mTimeUntilTeleport);
   GFREADVAR(pStream, float, mTimeUntilRetarget);
   GFREADVAR(pStream, BCueIndex, mAttackSound);
   GFREADCLASS(pStream, saveType, mJumpSplineCurve);
   GFREADCLASS(pStream, saveType, mCameraEffectData);
   GFREADCLASS(pStream, saveType, mCostPerTick);
   GFREADCLASS(pStream, saveType, mCostPerTickAttacking);
   GFREADCLASS(pStream, saveType, mCostPerJump);
   GFREADVAR(pStream, float, mTickLength);
   GFREADVAR(pStream, float, mDamageMultiplier);
   GFREADVAR(pStream, float, mDamageTakenMultiplier);
   GFREADVAR(pStream, float, mSpeedMultiplier);
   GFREADVAR(pStream, float, mNudgeMultiplier);
   GFREADVAR(pStream, float, mScanRadius);
   GFREADVAR(pStream, BProtoObjectID, mProjectileObject);
   GFREADVAR(pStream, BProtoObjectID, mHandAttachObject);
   GFREADVAR(pStream, BProtoObjectID, mTeleportAttachObject);
   GFREADVAR(pStream, float, mAudioReactionTimer);
   GFREADVAR(pStream, float, mTeleportTime);
   GFREADVAR(pStream, float, mTeleportLateralDistance);
   GFREADVAR(pStream, float, mTeleportJumpDistance);
   GFREADVAR(pStream, float, mTimeBetweenRetarget);
   GFREADVAR(pStream, float, mMotionBlurAmount);
   GFREADVAR(pStream, float, mMotionBlurDistance);
   GFREADVAR(pStream, float, mMotionBlurTime);
   GFREADVAR(pStream, float, mDistanceVsAngleWeight);
   if (BPower::mGameFileVersion >= 9)
   {
      GFREADVAR(pStream, float, mHealPerKillCombatValue);
      GFREADVAR(pStream, float, mAuraRadius);
      GFREADVAR(pStream, float, mAuraDamageBonus);
      if (BPower::mGameFileVersion >= 11)
      {
         GFREADVAR(pStream, BProtoObjectID, mAuraAttachObjectSmall);
         GFREADVAR(pStream, BProtoObjectID, mAuraAttachObjectMedium);
         GFREADVAR(pStream, BProtoObjectID, mAuraAttachObjectLarge);
      }
      else
      {
         GFREADVAR(pStream, BProtoObjectID, mAuraAttachObjectMedium);
         mAuraAttachObjectSmall = mAuraAttachObjectMedium;
         mAuraAttachObjectLarge = mAuraAttachObjectMedium;
      }
      GFREADVAR(pStream, BProtoObjectID, mHealAttachObject);
      GFREADARRAY(pStream, BEntityID, mSquadsInAura, uint8, 200);
   }
   if (BPower::mGameFileVersion >= 12)
   {
      GFREADVAR(pStream, BObjectTypeID, mFilterTypeID);
   }
   GFREADBITBOOL(pStream, mCompletedInitialization);
   GFREADBITBOOL(pStream, mHasSuccessfullyAttacked);
   if (BPower::mGameFileVersion >= 18)
   {
      GFREADBITBOOL(pStream, mUsePather);
   }
   gSaveGame.remapProtoObjectID(mProjectileObject);
   gSaveGame.remapProtoObjectID(mHandAttachObject);
   gSaveGame.remapProtoObjectID(mTeleportAttachObject);

   if (!mFlagDestroy && mCompletedInitialization)
   {
      BSquad* pOwnerSquad=gWorld->getSquad(mOwnerID);
      if (pOwnerSquad)
         pOwnerSquad->addEventListener(this);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerRage::savePtr(BStream* pStream) const
{
   GFWRITEVAR(pStream, BPowerID, mID);
   return true;
}
