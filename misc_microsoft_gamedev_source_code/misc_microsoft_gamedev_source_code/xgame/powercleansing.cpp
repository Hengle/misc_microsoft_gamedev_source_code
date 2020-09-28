//==============================================================================
// powercleansing.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "powercleansing.h"
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
#include "simtypes.h"
#include "protopower.h"
#include "unitquery.h"
#include "physics.h"
#include "powermanager.h"
#include "powerhelper.h"
#include "powerdisruption.h"
#include "unitactionavoidcollisionair.h"

//#define CONSOLE_OUTPUT
static const float cAudioReactionInterval = 3.0f;

//==============================================================================
//==============================================================================
bool BPowerUserCleansing::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mInputDir = cOriginVector;
   mLastUpdatePos = cOriginVector;
   mRealBeamID = cInvalidObjectID;
   mFakeBeamID = cInvalidObjectID;
   mAirImpactObjectID = cInvalidObjectID;
   mTimestampNextCommand = 0;
   mCommandInterval = 0;
   mMinBeamDistance = 0.0f;
   mMaxBeamDistance = 0.0f;
   mMaxBeamSpeed = 0.0f;
   mLOSMode = BWorld::cCPLOSFullVisible;

   // Create our fake beam or whatever.

   // send the command to actually create the real power here or something.

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserCleansing::init, mID %d", gWorld->getGametimeFloat(), mID);
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

   BProtoObjectID beamProtoID = cInvalidProtoObjectID;
   bool bSuccess = true;
   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "Beam", beamProtoID);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "MinBeamDistance", mMinBeamDistance);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "MaxBeamDistance", mMaxBeamDistance);
   if (bSuccess)
   {
      float commandInterval = 0.0f;
      bSuccess = pProtoPower->getDataFloat(powerLevel, "CommandInterval", commandInterval);
      if (bSuccess)
         mCommandInterval = static_cast<DWORD>(1000.0f * Math::fSelectMax(commandInterval, 0.1f));
   }
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "MaxBeamSpeed", mMaxBeamSpeed);
   if (bSuccess)
   {
      bool requiresLOS = true;
      if(pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS)
         mLOSMode = BWorld::cCPLOSDontCare;
   }
   if (!bSuccess)
   {
      BFAIL("Missing or invalid data attribute.");
      return (false);
   }

   if (mMinBeamDistance < 0.0f)
      mMinBeamDistance = 0.0f;
   if (mMaxBeamSpeed < 0.0f)
      mMaxBeamSpeed = 0.0f;
   if (mMaxBeamDistance < mMinBeamDistance)
      mMaxBeamDistance = mMinBeamDistance;

   mOwnerSquadID = ownerSquadID;
   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
   if(!pOwnerSquad || !BPowerCleansing::canSquadUsePower(*pOwnerSquad))
      return false;

   BUnit* pLeaderUnit = pOwnerSquad->getLeaderUnit();
   if (pLeaderUnit  && (pLeaderUnit->getFlagDoingFatality() || pLeaderUnit->getFlagFatalityVictim()))
      return false;

   // return if we're trying to cast out of los
   if(!mpUser->getFlagHaveHoverPoint())
      return false;
   BVector tempVec;
   if(!gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, BWorld::cCPLOSCenterOnly) || 
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this) || !gWorld->getPowerManager()->isValidPowerLocation(pOwnerSquad->getPosition(), this))
      return false;

   // check range
   BVector ownerPos = pOwnerSquad->getPosition();
   BVector posDiff = targetLocation - ownerPos;
   float posDiffLength = posDiff.length();
   if(posDiffLength > mMaxBeamDistance || posDiffLength < mMinBeamDistance)
      return false;

   setupUser();

   // create fake beam
   BObjectCreateParms parms;
   parms.mPlayerID = mpUser->getPlayerID();
   parms.mPosition = mpUser->getHoverPoint();
   parms.mProtoObjectID = beamProtoID;
   parms.mType = BEntity::cClassTypeObject;
   parms.mStartExistSound = false;
//-- FIXING PREFIX BUG ID 1290
   const BObject* fakeObject = gWorld->createObject(parms);
//--
   if(!fakeObject)
      return false;
   mFakeBeamID = fakeObject->getID();

   // other setup
   mInputDir.zero();
   mLastUpdatePos = mpUser->getHoverPoint();
   mRealBeamID = cInvalidObjectID;

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
bool BPowerUserCleansing::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerUserCleansing::shutdown, mID %d, shutting down", gWorld->getGametimeFloat(), mID);
#endif

   // Clean up our stuff.

   // Unlink ourselves form the sim side of the power

   // kill our fake object
   BObject* pFakeObject = gWorld->getObject(mFakeBeamID);
   if(pFakeObject)
      pFakeObject->kill(false);

   BObject* pAirImpactObject = gWorld->getObject(mAirImpactObjectID);
   if(pAirImpactObject)
      pAirImpactObject->kill(false);
   mAirImpactObjectID = cInvalidObjectID;

   // fix up the user's mode
   mpUser->unlockUser();

   mFlagDestroy = true;
   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserCleansing::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(!mInitialized)
      return false;

   if(!mpUser)
   {
      BFAIL("no mpUser in BPowerUserCleansing::handleInput");
      return false;
   }

   BCamera* pCamera = mpUser->getCamera();
   if(!pCamera)
   {
      BFAIL("no pCamera available in BPowerUserCleansing::handleInput");
      return false;
   }

   bool start = (event == cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(mpUser->getOption_ControlScheme());


   if (pInputInterface->isFunctionControl( BInputInterface::cInputTranslation, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
      BVector forwardDir;
      if(_fabs(pCamera->getCameraDir().y)<_fabs(pCamera->getCameraUp().y))
         forwardDir=BVector(pCamera->getCameraDir().x,0,pCamera->getCameraDir().z);
      else
         forwardDir=BVector(pCamera->getCameraUp().x,0,pCamera->getCameraUp().z);
      forwardDir.normalize();

      BVector rightDir=pCamera->getCameraRight();
      rightDir.y = 0;
      rightDir.normalize();

      BVector totalDir = (detail.mX * rightDir + detail.mY * -forwardDir);
      //totalDir.normalize(); // MS 10/22/2008: this is bad because it nukes the amount of joystick deflection, preventing beam movement speed modulation
      mInputDir = totalDir;

      // update hover point
      mpUser->setFlagUpdateHoverPoint(true);

      return true;
   }
   else if(pInputInterface->isFunctionControl( BInputInterface::cInputZoom, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
   {
      mpUser->handleInputScrollingAndCamera(port, event, controlType, detail);

      if(mFakeBeamID.isValid())
      {
//-- FIXING PREFIX BUG ID 1291
         const BObject* pFakeObject=gWorld->getObject(mFakeBeamID);
//--
         if(pFakeObject)
            pCamera->lookAtPosFixedY(pFakeObject->getPosition());
      }

      return true;
   }

   if(mpUser->handleInputScrollingAndCamera(port, event, controlType, detail))
      return(true);

   bool             modifyAction    = mpUser->getFlagModifierAction();

   // Selection or do work
   if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) || 
       pInputInterface->isFunctionControl( BInputInterface::cInputAbility, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( start )
      {
#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerUserCleansing::handleInput, mID %d, sending shutdown command", gWorld->getGametimeFloat(), mID);
#endif
         cancelPower();
      }
      return( true );
   }

   return( false );
}


//==============================================================================
//==============================================================================
void BPowerUserCleansing::update(float elapsedTime)
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

   // Bomb check.
   BCamera* pCamera = mpUser->getCamera();
   if(!pCamera)
   {
      BFAIL("no pCamera available in BPowerUserCleansing::update");
      return;
   }

   // Start out with where we were last update.
   BVector newPos = mLastUpdatePos;

   // If we have directional input, process it here then reset it.
   if ((mInputDir.x != 0.0f) || (mInputDir.z != 0.0f))
   {
      newPos += (mInputDir * mMaxBeamSpeed * elapsedTime);
      gTerrainSimRep.clampWorld(newPos);
      mInputDir.zero();
   }

   // Additional restrictions based on distance from owner, if applicable.
   if(mOwnerSquadID.isValid())
   {
      const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerSquadID);
      if (pOwnerSquad)
      {
         BVector ownerPos = pOwnerSquad->getPosition();
         BVector posDiff = newPos - ownerPos;
         BVector dir = posDiff;
         dir.normalize();
         float posDiffLength = posDiff.length();
         if(posDiffLength > mMaxBeamDistance)
         {
            newPos = ownerPos + dir * mMaxBeamDistance;
         }
         else if(posDiffLength < mMinBeamDistance)
         {
            newPos = ownerPos + dir * mMinBeamDistance;
         }
      }
   }

   // Find height at position.
   gTerrainSimRep.getHeightRaycast(newPos, newPos.y, true);

   // Update the camera.
   mpUser->setCameraHoverPoint(newPos);

   // update air impact object
   BProtoObjectID airImpactProtoObjectID = cInvalidProtoObjectID;
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(pProtoPower && pProtoPower->getDataProtoObject(mPowerLevel, "AirImpactObject", airImpactProtoObjectID))
      BPowerCleansing::updateAirImpactObject(mpUser->mPlayerID, airImpactProtoObjectID, newPos, mAirImpactObjectID, false);


   // Update the fake object if we have one.
   BObject* pFakeObject = gWorld->getObject(mFakeBeamID);
   if (pFakeObject)
      pFakeObject->setPosition(newPos);

   // Save off the pos for this update.
   mLastUpdatePos = newPos;

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
      c->setVector(newPos);
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
void BPowerUserCleansing::setupUser()
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
bool BPowerCleansing::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mWaypoints.clear();
   mBeamID = cInvalidObjectID;
   mAirImpactObjectID = cInvalidObjectID;
   mNextDamageTime = 0;
   mLeaderAnimOrderID = 0;
   mDesiredBeamPosition = cOriginVector;
   mBeamPath.clear();
   mRevealedTeamIDs.clear();
   mCostPerTick.zero();
   mProjectile = cInvalidProtoObjectID;
   mTickLength = 0.0f;
   mMinBeamDistance = 0.0f;
   mMaxBeamDistance = 0.0f;
   mCommandInterval = 0;
   mMaxBeamSpeed = 0.0f;
   mLOSMode = BWorld::cCPLOSFullVisible;
   mFlagUsePath = false;
   mAudioReactionTimer = 0.0f;

#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerCleansing::init, playerID %d, powerUserID %d, ownerSquadID %d", gWorld->getGametimeFloat(), playerID, powerUserID, ownerSquadID);
#endif

   // casting variables
   mPlayerID = playerID;
   mPowerUserID = powerUserID;
   mPowerLevel = powerLevel;
   mOwnerID = ownerSquadID;
   mTargetLocation = targetLocation;
   mFlagIgnoreAllReqs = ignoreAllReqs;

   // bomb check
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
   {
      shutdown();
      return false;
   }
   
   BSquad* pOwnerSquad = gWorld->getSquad(ownerSquadID);
   if (!mFlagIgnoreAllReqs && !pOwnerSquad)
   {
      shutdown();
      return false;
   }

   BUnit* pLeaderUnit = (pOwnerSquad ? pOwnerSquad->getLeaderUnit() : NULL);
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
   BProtoObjectID beamProtoID = cInvalidProtoObjectID;   
   bool bSuccess = true;
   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "Beam", beamProtoID);
   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "Projectile", mProjectile);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "TickLength", mTickLength);
   if (bSuccess)
      bSuccess = (mTickLength > 0.0f);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "SuppliesPerTick", suppliesPerTick);
   if (bSuccess)
      bSuccess = (suppliesPerTick > 0.0f);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "MinBeamDistance", mMinBeamDistance);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "MaxBeamDistance", mMaxBeamDistance);
   if (bSuccess)
   {
      float commandInterval = 0.0f;
      bSuccess = pProtoPower->getDataFloat(powerLevel, "CommandInterval", commandInterval);
      if (bSuccess)
         mCommandInterval = static_cast<DWORD>(1000.0f * Math::fSelectMax(commandInterval, 0.1f));
   }
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "MaxBeamSpeed", mMaxBeamSpeed);
   if (bSuccess)
   {
      bool requiresLOS = true;
      if (mFlagIgnoreAllReqs || (pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS))
         mLOSMode = BWorld::cCPLOSDontCare;
   }
   if (!bSuccess)
   {
      BFAIL("Missing or invalid data attribute.");
      shutdown();
      return (false);
   }

   if (mMinBeamDistance < 0.0f)
      mMinBeamDistance = 0.0f;
   if (mMaxBeamSpeed < 0.0f)
      mMaxBeamSpeed = 0.0f;
   if (mMaxBeamDistance < mMinBeamDistance)
      mMaxBeamDistance = mMinBeamDistance;

   // fail if trying to cast outside of los
   BVector tempVec;
   if (!mFlagIgnoreAllReqs)
   {
      bool powerValid = BPowerHelper::checkPowerLocation(mPlayerID, pOwnerSquad->getPosition(), this, true);
      powerValid &= BPowerHelper::checkPowerLocation(mPlayerID, targetLocation, this, true);
      if (!powerValid)
      {
         shutdown();
         return false;
      }
   }

   mCostPerTick.add(gDatabase.getRIDSupplies(), suppliesPerTick);

   // basics
   mElapsed = 0.0;

   // pay the cost first, bail if we can't
   if(!mFlagIgnoreAllReqs && !pPlayer->canUsePower(mProtoPowerID, ownerSquadID))
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerCleansing::init, shutting down because player can't pay startup cost", gWorld->getGametimeFloat());
#endif
      shutdown();
      return false;
   }

   pPlayer->usePower(mProtoPowerID, mOwnerID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

   // create real beam
   mBeamID = gWorld->createEntity(beamProtoID, false, mPlayerID, mTargetLocation, cZAxisVector, cXAxisVector, true, false, false, cInvalidObjectID, cInvalidPlayerID, cInvalidObjectID, cInvalidObjectID, true, 1.0f, false, 0, -1, -1, false);
//-- FIXING PREFIX BUG ID 1294
   const BSquad* pBeamSquad = gWorld->getSquad(mBeamID);
//--
   BASSERT(pBeamSquad);
   mDesiredBeamPosition = pBeamSquad->getPosition();
   // Halwes - 7/21/2008 - Path is used by trigger script
   mBeamPath.clear();
   if (mFlagUsePath)
   {
      mBeamPath.add(mDesiredBeamPosition);
      // Halwes - 7/21/2008 - LOS hack to make beam more visible
      //BUnit* pBeamUnit = pBeamSquad->getLeaderUnit();
      //if (pBeamUnit)
      //{
      //   pBeamUnit->setLOSScalar(5.0f);
      //}
   }

   // notify power user instance about our newly created beam
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID)
   {
      BPowerUserCleansing* pCleansingPowerUser = static_cast<BPowerUserCleansing*>(pUser->getPowerUser());
      if(pCleansingPowerUser)
      {
         pCleansingPowerUser->setRealBeamID(mBeamID);

         // set main beam invisible to owner
         // mrh 6/26/08 - Only do this for players with a BPowerUser (so AI players can see it when we switch to them.)
         if(pBeamSquad && !gConfig.isDefined("cleansingShowRealBeam"))
         {
            for(uint8 i = 0; i < pBeamSquad->getNumberChildren(); i++)
            {
               BEntityID id = pBeamSquad->getChild(i);
               BObject* pObject = gWorld->getObject(id);
               if(pObject)
               {
                  pObject->setFlagNoRenderForOwner(true);
               }
            }
         }
      }
   }

   // Halwes - 7/21/2008 - There may not be a valid owner squad if all requirements are ignored
   if (pOwnerSquad)
   {
      BSimOrder* pOrder = gSimOrderManager.createOrder();
      if (pOrder)
      {
         if(pOwnerSquad->getParentPlatoon())
            pOwnerSquad->getParentPlatoon()->removeAllOrders();

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
      BUnit* pLeaderUnit = pOwnerSquad->getLeaderUnit();
      if (pLeaderUnit)
      {
         pLeaderUnit->setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateMisc, cAnimTypePowerIdle, true, true, -1, true);
         pLeaderUnit->computeAnimation();
         pLeaderUnit->lockAnimation(10000000, false);
      }

      BSquadAI* pSquadAI = pOwnerSquad->getSquadAI();
      if (pSquadAI)
         pSquadAI->setMode(BSquadAI::cModePower);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCleansing::shutdown()
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerCleansing::shutdown, mID %d, mPowerUserID %d", gWorld->getGametimeFloat(), mID, mPowerUserID);
#endif

   if(mBeamID.isValid())
   {
      BSquad* pBeam = gWorld->getSquad(mBeamID);
      if(pBeam)
         pBeam->kill(false);
   }

   if(mAirImpactObjectID.isValid())
   {
      BObject* pAirImpactObject = gWorld->getObject(mAirImpactObjectID);
      if(pAirImpactObject)
         pAirImpactObject->kill(false);
   }

   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if(pUser && pUser->getPowerUser() && pUser->getPowerUser()->getID() == mPowerUserID)
      pUser->getPowerUser()->shutdown();

   // Remove the anim order from the leader unit
   BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
   if (pOwnerSquad)
   {
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


   mFlagDestroy = true;

   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCleansing::submitInput(BPowerInput powerInput)
{
#ifdef CONSOLE_OUTPUT
   gConsoleOutput.status("%.2f: BPowerCleansing::submitInput, mID %d, mPowerUserID %d, input type %d", gWorld->getGametimeFloat(), mID, mPowerUserID, powerInput.mType);
#endif

   if (powerInput.mType == PowerInputType::cUserCancel)
   {
      // The user cancelled the power.
      shutdown();
      return (true);
   }
   else if (powerInput.mType == PowerInputType::cPosition)
   {
      // Halwes - 7/21/2008 - Path is used by trigger script
      if (mFlagUsePath)
      {
         mBeamPath.add(powerInput.mVector);
      }
      else
      {
         mDesiredBeamPosition = powerInput.mVector;
      }
      return true;
   }
   // Insert handling for additional power input types here.

   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerCleansing::submitInput() received powerInput of unsupported type.");
   return (false);
}

//==============================================================================
//==============================================================================
bool BPowerCleansing::canSquadUsePower(const BSquad& squad)
{
   BUnit* pLeaderUnit = squad.getLeaderUnit();
   if (!pLeaderUnit)
      return false;

   if (!squad.isAlive())
      return false;

   if (squad.getFlagAttackBlocked())
      return false;

   if (squad.isGarrisoned())
      return false;

   BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pLeaderUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BPowerCleansing::update(DWORD currentGameTime, float lastUpdateLength)
{
   currentGameTime;

   if(getFlagDestroy())
      return;

   // bomb checks
   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   BSquad* pBeamSquad = gWorld->getSquad(mBeamID);
   BUnit* pBeamUnit = (pBeamSquad) ? pBeamSquad->getLeaderUnit() : NULL;   
   if (!pPlayer || !pBeamSquad || !pBeamUnit)
   {
#ifdef CONSOLE_OUTPUT
      gConsoleOutput.status("%.2f: BPowerCleansing::update, bomb check failed, shutting down", gWorld->getGametimeFloat());
#endif
      shutdown();
      return;
   }

//-- FIXING PREFIX BUG ID 1297
   const BSquad* pOwnerSquad = gWorld->getSquad(mOwnerID);
//--
   BUnit* pLeaderUnit = (pOwnerSquad) ? pOwnerSquad->getLeaderUnit() : NULL;
   if (!mFlagIgnoreAllReqs && (!pOwnerSquad || !pLeaderUnit || !canSquadUsePower(*pOwnerSquad)))
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

   BProtoAction* pProtoAction = BPowerHelper::getFirstProtoAction(*pPlayer, mProjectile);
   if (!pProtoAction)
   {
      shutdown();
      return;
   }

   // if the owning squad or the beam is in an invalid location, bail
   if (!mFlagIgnoreAllReqs)
   {
      bool powerValid = BPowerHelper::checkPowerLocation(mPlayerID, pOwnerSquad->getPosition(), this, true);
      powerValid &= BPowerHelper::checkPowerLocation(mPlayerID, pBeamUnit->getPosition(), this, true);
      if (!powerValid)
      {
         shutdown();
         return;
      }
   }

   // move the beam to the appropriate position
   if(pBeamSquad && pBeamUnit && pBeamUnit->getProtoObject())
   {
      const BVector& currentLaserPosition = pBeamUnit->getPosition();      
      // Halwes - 7/21/2008 - Path is used by trigger script
      if (mFlagUsePath)
      {
         uint numTargetPositions = mBeamPath.getSize();
         BASSERTM((numTargetPositions > 0), "Target position array is empty!");
         bool arrived = currentLaserPosition.almostEqual(mBeamPath[0]);
         if (arrived && numTargetPositions > 1)
         {
            mBeamPath.removeIndex(0);
            mDesiredBeamPosition = mBeamPath[0];
         }
         else if (arrived && (numTargetPositions == 1))
         {
            shutdown();
            return;
         }         
      }
      float currentDistance = currentLaserPosition.xzDistance(mDesiredBeamPosition);
      float xzDistanceThisFrame = (lastUpdateLength * mMaxBeamSpeed);
      BVector newPos = mDesiredBeamPosition;

      // if we can travel far enough in this frame, take that position
      if (currentDistance >= xzDistanceThisFrame)
      {
         BVector currentDirection = (mDesiredBeamPosition - currentLaserPosition);
         currentDirection.y = 0.0f;
         currentDirection.normalize();
         newPos = (currentLaserPosition + (currentDirection * xzDistanceThisFrame));
         gTerrainSimRep.getHeightRaycast(newPos, newPos.y, true);
      }

      BSimString debugStr;
      float delta = newPos.xzDistance(pBeamUnit->getPosition());
      float dt = lastUpdateLength;
      float speed = 0.0f;
      if (dt > 0.0f)
         speed = delta / dt;

      debugStr.format("REAL BEAM POS moved %f DIST, over %f TIME, overall SPEED = %f", delta, dt, speed);
      gConsole.output(cChannelAI, debugStr);

      // clamp to terrain edges
      gTerrainSimRep.clampWorld(newPos);
      pBeamUnit->setPosition(newPos);
      pBeamSquad->setPosition(newPos);
      pBeamSquad->setLeashPosition(newPos); // MS 10/30/2008: PHX-16813


      // MS 10/8/2008: air impact thing
      BProtoObjectID poID;
      if(pProtoPower->getDataProtoObject(mPowerLevel, "AirImpactObject", poID))
         BPowerCleansing::updateAirImpactObject(mPlayerID, mPowerLevel, newPos, mAirImpactObjectID, true);

      //////
      // See which units are near the beam and add a revealer on the controlling unit (prophet) for those teams.
      BDynamicSimArray<BTeamID>     previousRevealedTeamIDs;
      previousRevealedTeamIDs = mRevealedTeamIDs;
      //clear current.
      mRevealedTeamIDs.clear();

      // Halwes - 7/21/2008 - There may not be a valid owner squad if all requirements are ignored
      BUnit* pOwnerUnit = pOwnerSquad ? pOwnerSquad->getLeaderUnit() : pBeamSquad->getLeaderUnit();
      if (pOwnerUnit)
      {
         for (int i = 0; i < gWorld->getNumberTeams(); i++)
         {
            if (pBeamUnit->isVisible(i))
               mRevealedTeamIDs.uniqueAdd(i);
         }

         //for all in previous array that aren't in the new, remove the reveal
         for (int i = 0; i < previousRevealedTeamIDs.getNumber(); i++)
         {
            if (!mRevealedTeamIDs.contains(previousRevealedTeamIDs.get(i)))
            {
               pOwnerUnit->removeReveal(previousRevealedTeamIDs.get(i));
               //gConsoleOutput.debug("Revealer Removed: %d", previousRevealedTeamIDs.get(i));
            }
         }

         //For all in the new array that aren't in the previous, add the reveal
         for (int i = 0; i < mRevealedTeamIDs.getNumber(); i++)
         {
            if (!previousRevealedTeamIDs.contains(mRevealedTeamIDs.get(i)))
            {
               pOwnerUnit->addReveal(mRevealedTeamIDs.get(i));
               //gConsoleOutput.debug("Revealer Added: %d", mRevealedTeamIDs.get(i));
            }
         }
      }
   }

   if(mAudioReactionTimer <= 0.0f)
   {
      gWorld->createPowerAudioReactions(getPlayerID(), cSquadSoundChatterReactPowCleansing, mTargetLocation, 60.0f, mOwnerID);
      mAudioReactionTimer = cAudioReactionInterval;
   }
   else
      mAudioReactionTimer -= lastUpdateLength;


   // iterate damage ticks
   mElapsed += lastUpdateLength;
   while(mElapsed > mNextDamageTime)
   {
      // we're done if the player can't pay the cost, otherwise pay it
      if(!mFlagIgnoreAllReqs && !pPlayer->checkCost(&mCostPerTick))
      {
#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerCleansing::update, shutting down because player ran out of resources", gWorld->getGametimeFloat());
#endif
         if (BPowerHelper::getPowerUser(*this))
            gUI.playPowerNotEnoughResourcesSound();

         shutdown();
         return;
      }

      if (!mFlagIgnoreAllReqs)
         pPlayer->payCost(&mCostPerTick);

      // physics
      BPowerHelper::impulsePhysicsAirUnits(pBeamUnit->getPosition(), 10.0f, 20);

      // deal damage
      BObjectCreateParms parms;
      parms.mPlayerID = mPlayerID;
      parms.mProtoObjectID = pProtoAction->getProjectileID();
      parms.mPosition = pBeamSquad->getPosition() + BVector(0,5,0);
      float dps = pProtoAction->getDamagePerSecond();
      BEntityID leaderUnitID = cInvalidObjectID;
      if (pLeaderUnit)
         leaderUnitID = pLeaderUnit->getID();
      gWorld->launchProjectile(parms, pBeamSquad->getPosition(), XMVectorZero(), XMVectorZero(), dps, pProtoAction, pProtoAction, leaderUnitID, NULL, -1, true, true, true);

      // increment damage time
      mNextDamageTime += mTickLength;
   }
}

//==============================================================================
//==============================================================================
BEntityID BPowerCleansing::getBeamID() const
{
   return (mBeamID);
}

//==============================================================================
//==============================================================================
const BSquad* BPowerCleansing::getBeamSquad() const
{
   return (gWorld->getSquad(mBeamID));
}

//==============================================================================
//==============================================================================
const BUnit* BPowerCleansing::getBeamUnit() const
{
   const BSquad* pBeamSquad = gWorld->getSquad(mBeamID);
   if (pBeamSquad)
      return (pBeamSquad->getLeaderUnit());
   else
      return (NULL);
}

//==============================================================================
//==============================================================================
void BPowerCleansing::updateAirImpactObject(BPlayerID playerID, BProtoObjectID protoObjectID, BVector pos, BEntityID& airImpactObjectID, bool noRenderForOwner)
{
   // MS 10/7/2008: show/hide air impact object
   BUnitQuery unitQuery(pos, 3.0f, true);
   unitQuery.setRelation(playerID, cRelationTypeAny);
   BEntityIDArray squadsInArea(0, 32);
   gWorld->getSquadsInArea(&unitQuery, &squadsInArea, false);
   uint numSquadsInArea = squadsInArea.getSize();
   bool needImpactObject = false;
   if(numSquadsInArea > 0)
   {
      // determine whether we need to have an object, and at what height
      float maxHeight = pos.y;
      for(uint i = 0; i < numSquadsInArea; i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadsInArea[i]);
         if(pSquad &&
            pSquad->getProtoObject() &&
            pSquad->getProtoObject()->getMovementType() == cMovementTypeAir &&
            pSquad->getPosition().y > maxHeight)
         {
            BUnit* pUnit = pSquad->getLeaderUnit();
            if(pUnit)
            {
               const BBoundingBox* pBBox = pUnit->getVisualBoundingBox();
               float intersectDistSqr, intersectDistSqrDown, intersectDistSqrUp;
               static const float cRaycastStartHeight = 100.0f;
               static const float cShrunkScale = 0.8f;
               if(pBBox->raySegmentIntersects(BVector(pos.x, cRaycastStartHeight, pos.z), BVector(0, -1.0f, 0), false, NULL, cShrunkScale, intersectDistSqr) &&
                  pBBox->raySegmentIntersectsFirst(BVector(pos.x, cRaycastStartHeight, pos.z), BVector(0, -1.0f, 0), false, NULL, intersectDistSqrDown) &&
                  pBBox->raySegmentIntersectsFirst(BVector(pos.x, -cRaycastStartHeight, pos.z), BVector(0, 1.0f, 0), false, NULL, intersectDistSqrUp))
               {
                  const float cProportion = 0.8f;
                  maxHeight = (cProportion * (cRaycastStartHeight - Math::fSqrt(intersectDistSqrDown))) + ((1.0f - cProportion) * (Math::fSqrt(intersectDistSqrUp) - cRaycastStartHeight));
                  needImpactObject = true;
               }
            }
         }
      }

      if(needImpactObject)
      {
         // create air impact object if needed
         BObject* pAirImpactObject = NULL;
         if(airImpactObjectID == cInvalidObjectID)
         {
            BObjectCreateParms parms;
            parms.mPlayerID = playerID;
            parms.mPosition = pos;
            parms.mProtoObjectID = protoObjectID;
            parms.mType = BEntity::cClassTypeObject;
            pAirImpactObject = gWorld->createObject(parms);
            airImpactObjectID = pAirImpactObject->getID();

            if(noRenderForOwner && !gConfig.isDefined("cleansingShowRealBeam"))
               pAirImpactObject->setFlagNoRenderForOwner(true);
         }
         else
            pAirImpactObject = gWorld->getObject(airImpactObjectID);
         BASSERT(pAirImpactObject);

         pAirImpactObject->setPosition(BVector(pos.x, maxHeight, pos.z));
      }
   }

   if(!needImpactObject)
   {
      BObject* pAirImpactObject = gWorld->getObject(airImpactObjectID);
      if(pAirImpactObject)
         pAirImpactObject->kill(false);
      airImpactObjectID = cInvalidObjectID;
   }
}

//==============================================================================
//==============================================================================
bool BPowerUserCleansing::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITEVECTOR(pStream, mInputDir);
   GFWRITEVECTOR(pStream, mLastUpdatePos);
   GFWRITEVAR(pStream, BEntityID, mRealBeamID);
   GFWRITEVAR(pStream, BEntityID, mFakeBeamID);
   GFWRITEVAR(pStream, BEntityID, mAirImpactObjectID);
   GFWRITEVAR(pStream, DWORD, mTimestampNextCommand);
   GFWRITEVAR(pStream, DWORD, mCommandInterval);
   GFWRITEVAR(pStream, float, mMinBeamDistance);
   GFWRITEVAR(pStream, float, mMaxBeamDistance);
   GFWRITEVAR(pStream, float, mMaxBeamSpeed);
   GFWRITEVAR(pStream, long, mLOSMode);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserCleansing::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;
   GFREADVECTOR(pStream, mInputDir);
   GFREADVECTOR(pStream, mLastUpdatePos);
   GFREADVAR(pStream, BEntityID, mRealBeamID);
   GFREADVAR(pStream, BEntityID, mFakeBeamID);
   if(BPowerUser::mGameFileVersion >= 7)
   {
      GFREADVAR(pStream, BEntityID, mAirImpactObjectID);
   }
   GFREADVAR(pStream, DWORD, mTimestampNextCommand);
   GFREADVAR(pStream, DWORD, mCommandInterval);
   GFREADVAR(pStream, float, mMinBeamDistance);
   GFREADVAR(pStream, float, mMaxBeamDistance);
   GFREADVAR(pStream, float, mMaxBeamSpeed);
   GFREADVAR(pStream, long, mLOSMode);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCleansing::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEVECTORARRAY(pStream, mWaypoints, uint8, 200);
   GFWRITEVAR(pStream, BEntityID, mBeamID);
   GFWRITEVAR(pStream, BEntityID, mAirImpactObjectID);
   GFWRITEVAR(pStream, double, mNextDamageTime);
   GFWRITEVAR(pStream, uint, mLeaderAnimOrderID);
   GFWRITEVECTOR(pStream, mDesiredBeamPosition);
   GFWRITEVECTORARRAY(pStream, mBeamPath, uint8, 200);
   GFWRITEARRAY(pStream, BTeamID, mRevealedTeamIDs, uint8, 16);
   GFWRITECLASS(pStream, saveType, mCostPerTick);
   GFWRITEVAR(pStream, BProtoObjectID, mProjectile);
   GFWRITEVAR(pStream, float, mTickLength);
   GFWRITEVAR(pStream, float, mMinBeamDistance);
   GFWRITEVAR(pStream, float, mMaxBeamDistance);
   GFWRITEVAR(pStream, DWORD, mCommandInterval);
   GFWRITEVAR(pStream, float, mMaxBeamSpeed);   
   GFWRITEVAR(pStream, long, mLOSMode);
   GFWRITEBITBOOL(pStream, mFlagUsePath);
   GFWRITEVAR(pStream, float, mAudioReactionTimer);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCleansing::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADVECTORARRAY(pStream, mWaypoints, uint8, 200);
   GFREADVAR(pStream, BEntityID, mBeamID);
   if(BPower::mGameFileVersion >= 20)
   {
      GFREADVAR(pStream, BEntityID, mAirImpactObjectID);
   }
   GFREADVAR(pStream, double, mNextDamageTime);
   GFREADVAR(pStream, uint, mLeaderAnimOrderID);
   GFREADVECTOR(pStream, mDesiredBeamPosition);
   GFREADVECTORARRAY(pStream, mBeamPath, uint8, 200);
   GFREADARRAY(pStream, BTeamID, mRevealedTeamIDs, uint8, 16);
   GFREADCLASS(pStream, saveType, mCostPerTick);
   GFREADVAR(pStream, BProtoObjectID, mProjectile);
   GFREADVAR(pStream, float, mTickLength);
   GFREADVAR(pStream, float, mMinBeamDistance);
   GFREADVAR(pStream, float, mMaxBeamDistance);
   GFREADVAR(pStream, DWORD, mCommandInterval);
   GFREADVAR(pStream, float, mMaxBeamSpeed);   
   GFREADVAR(pStream, long, mLOSMode);
   GFREADBITBOOL(pStream, mFlagUsePath);
   GFREADVAR(pStream, float, mAudioReactionTimer);
   return true;
}
