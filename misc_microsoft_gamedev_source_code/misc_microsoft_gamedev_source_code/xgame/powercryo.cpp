//==============================================================================
// powerCryo.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "commandmanager.h"
#include "commands.h"
#include "powercryo.h"
#include "protopower.h"
#include "simhelper.h"
#include "uigame.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "soundmanager.h"
#include "protosquad.h"
#include "powermanager.h"

//#define CONSOLE_OUTPUT

//==============================================================================
//==============================================================================
bool BPowerUserCryo::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mFlagIssueCommandOnCancel=false;
   mHudSounds.clear();

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
   if (pProtoPower->getPowerType() != PowerType::cCryo)
      return (false);

   // Player cannot use power.
   mFlagNoCost = noCost;
   if (!mFlagNoCost && !pUserPlayer->canUsePower(protoPowerID))
      return (false);

   bool bSuccess = true;

   bSuccess = mHudSounds.loadFromProtoPower(pProtoPower, powerLevel);

   if (!bSuccess)
   {
      BFAIL("Failed to load required data for PowerUserCryo!");
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
bool BPowerUserCryo::shutdown()
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
bool BPowerUserCryo::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserCryo - is not initialized but is trying to handle input.");
   if (!mInitialized)
      return (false);
   // No user.
   BASSERTM(mpUser, "BPowerUserCryo - mpUser is NULL.");
   if (!mpUser)
      return (false);
   // No camera.
//-- FIXING PREFIX BUG ID 1313
   const BCamera* pCamera = mpUser->getCamera();
//--
   BASSERTM(pCamera, "BPowerUserCryo - Unable to get the user camera.");
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
         BVector tempVec;
         if(!gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly) ||
            !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
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
void BPowerUserCryo::renderUI()
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserCryo - is not initialized but is trying to render UI.");
   if (!mInitialized)
      return;
   // No user.
   BASSERTM(mpUser, "BPowerUserCryo - mpUser is NULL.");
   if (!mpUser)
      return;

   // Render some UI.
   if(!mpUser->getFlagHaveHoverPoint())
      return;
   BVector tempVec;
   if(gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly) &&
      gWorld->getPowerManager() && gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
      mpUser->renderReticle(BUIGame::cReticlePowerValid);
   else
      mpUser->renderReticle(BUIGame::cReticlePowerInvalid);
}

//==============================================================================
//==============================================================================
void BPowerUserCryo::setupUser()
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
      pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(24755));
   }
}

//==============================================================================
//==============================================================================
bool BPowerCryo::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mIgnoreList.clear();
   mNextTickTime = 0;
   mCryoObjectID = cInvalidObjectID;
   mDirection = cOriginVector;
   mRight = cOriginVector;
   mCryoObjectProtoID = cInvalidProtoObjectID;
   mBomberProtoID = cInvalidProtoObjectID;
   mFilterTypeID = cInvalidObjectTypeID;
   mCryoRadius = 0.0f;
   mMinCryoFalloff = 0.0f;
   mTickDuration = 0;
   mTicksRemaining = 0;
   mCryoAmountPerTick = 0.0f;
   mBomberData.clear();
   mReactionPlayed = false;
   mKillableHpLeft = 0.0f;
   mFreezingThawTime = 1.0f;
   mFrozenThawTime = 1.0f;

#ifdef SYNC_Command
   syncCommandData("BPowerUserCryo::init playerID", playerID);
   syncCommandData("BPowerUserCryo::init powerLevel", (DWORD)powerLevel);
   syncCommandData("BPowerUserCryo::init powerUserID player", powerUserID.getPlayerID());
   syncCommandData("BPowerUserCryo::init powerUserID powerType", (DWORD)powerUserID.getPowerType());
   syncCommandData("BPowerUserCryo::init ownerSquadID", ownerSquadID.asLong());
   syncCommandData("BPowerUserCryo::init targetLocation", targetLocation);
#endif

   // casting variables
   mPlayerID = playerID;
   mPowerUserID = powerUserID;
   mPowerLevel = powerLevel;
   mOwnerID = ownerSquadID;
   mTargetLocation = targetLocation;

   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pProtoPower)
   {
      shutdown();
      return false;
   }

   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   // check the location before anything else
   BVector tempVec;
   if(!gWorld->checkPlacement(-1,  playerID, targetLocation, tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly) ||
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(targetLocation, this))
   {
      shutdown();
      return false;
   }

   mCryoObjectProtoID = cInvalidProtoID;
   float tickduration = 0.0f;
   float effectStartTime = 0.0f;
   int numTicks = 0;
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   mFlagIgnoreAllReqs = ignoreAllReqs;
   bool bSuccess = true;

   bSuccess = (bSuccess && pPlayer != NULL);
#ifdef SYNC_Command
   syncCommandData("BPowerUserCryo::init pPlayer bSuccess", bSuccess);
#endif

   bSuccess = (bSuccess && pProtoPower != NULL);
#ifdef SYNC_Command
   syncCommandData("BPowerUserCryo::init pProtoPower bSuccess", bSuccess);
#endif

   if (!mFlagIgnoreAllReqs)
   {
      bSuccess = (bSuccess && pPlayer->canUsePower(mProtoPowerID));
#ifdef SYNC_Command
      syncCommandData("BPowerUserCryo::init canUserPower bSuccess", bSuccess);
#endif
   }

   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "CryoObject", mCryoObjectProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CryoRadius", mCryoRadius));
   bSuccess = (bSuccess && mCryoRadius > 0.0f);
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MinCryoFalloff", mMinCryoFalloff));
   bSuccess = (bSuccess && pProtoPower->getDataObjectType(powerLevel, "FilterType", mFilterTypeID));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "TickDuration", tickduration));
   bSuccess = (bSuccess && tickduration > 0.0f);
   bSuccess = (bSuccess && pProtoPower->getDataInt(powerLevel, "NumTicks", numTicks));
   bSuccess = (bSuccess && numTicks > 0);
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "CryoAmountPerTick", mCryoAmountPerTick));
   bSuccess = (bSuccess && mCryoAmountPerTick > 0.0f);
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "EffectStartTime", effectStartTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "MaxKillHp", mKillableHpLeft));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "FreezingThawTime", mFreezingThawTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "FrozenThawTime", mFrozenThawTime));

   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "Bomber", mBomberProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberBombTime", mBomberData.BombTime));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberFlyinDistance", mBomberData.BomberFlyinDistance));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberFlyinHeight", mBomberData.BomberFlyinHeight));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberBombHeight", mBomberData.BomberBombHeight));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberSpeed", mBomberData.BomberSpeed));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "BomberFlyOutTime", mBomberData.FlyoutTime));

#ifdef SYNC_Command
   syncCommandData("BPowerUserCryo::init bSuccess", bSuccess);
   syncCommandData("BPowerUserCryo::init CryoObjectProtoID", mCryoObjectProtoID);
   syncCommandData("BPowerUserCryo::init mCryoRadius", mCryoRadius);
   syncCommandData("BPowerUserCryo::init mFilterTypeID", mFilterTypeID);
   syncCommandData("BPowerUserCryo::init tickduration", tickduration);
   syncCommandData("BPowerUserCryo::init numTicks", numTicks);
#endif

   // Did we succeed on all our setup?
   if (!bSuccess)
   {
      BFAIL("Failed to load required data for PowerCryo");
      shutdown();
      return (false);
   }

   // create the bomber
   mDirection = BVector(getRandRangeFloat(cSimRand, -1.0f, 1.0f), 0.0f, getRandRangeFloat(cSimRand, -1.0f, 1.0f));
   if (mDirection.lengthSquared() <= cFloatCompareEpsilon)
      mDirection = cXAxisVector;
   mDirection.normalize();

   // calc cross product
   mRight.assignCrossProduct(cYAxisVector, mDirection);
   mRight.normalize();

   mBomberData.BomberId = BPowerHelper::createBomber(mBomberProtoID, mBomberData, mTargetLocation, mDirection, mPlayerID);

   mTickDuration = static_cast<DWORD>(1000.0f * tickduration);
   mTicksRemaining = static_cast<uint>(numTicks);
   mNextTickTime = static_cast<DWORD>(gWorld->getGametime() + static_cast<DWORD>(1000.0f * effectStartTime) + mTickDuration);
   mElapsed = 0;
   mIgnoreList.reserve(32);
   mIgnoreList.resize(0);

   // Pay the cost now.
   pPlayer->usePower(mProtoPowerID, cInvalidObjectID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

   // All initialized.
   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerCryo::shutdown()
{
   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if (pUser)
   {
      BPowerUser* pPowerUser = pUser->getPowerUser();
      if (pPowerUser && pPowerUser->getID() == mPowerUserID)
         pPowerUser->shutdown();
   }

   // we don't kill the cryo object, because it will fade out as its lifetime expires

   // delete the bomber if we created one
   BObject* pBomberUnit = gWorld->getObject(mBomberData.BomberId);
   if (pBomberUnit)
      pBomberUnit->kill(true);

   mFlagDestroy = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerCryo::submitInput(BPowerInput powerInput)
{
   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerCryo::submitInput() received powerInput of unsupported type.");
   return (false);
}


//==============================================================================
//==============================================================================
void BPowerCryo::update(DWORD currentGameTime, float lastUpdateLength)
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

   // create the cryo object if we haven't yet
   if (mCryoObjectID == cInvalidObjectID)
   {
      BObjectCreateParms parms;
      parms.mPlayerID = mPlayerID;
      parms.mPosition = mTargetLocation;
      parms.mForward = -mDirection;
      parms.mRight = -mRight;
      parms.mType = BEntity::cClassTypeObject;
      parms.mProtoObjectID = mCryoObjectProtoID;
//-- FIXING PREFIX BUG ID 1312
      const BObject* pObject = gWorld->createObject(parms);
//--
      BASSERTM(pObject, "Failed to create cryo object!");
      mCryoObjectID = pObject->getID();
   }

   // Don't bother yet
   if (currentGameTime < mNextTickTime)
      return;

   // Do a query for allied squads in the heal zone only once per update (even though we may heal multiple ticks worth.)
   BUnitQuery unitQuery(mTargetLocation, mCryoRadius, true);
   unitQuery.setRelation(playerID, cRelationTypeAny);
   if (mFilterTypeID != cInvalidObjectTypeID)
      unitQuery.addObjectTypeFilter(mFilterTypeID);

   BEntityIDArray squadsInArea(0, 32);
   gWorld->getSquadsInArea(&unitQuery, &squadsInArea, false);
   uint numSquadsInArea = squadsInArea.getSize();

   BSimHelper::sortEntitiesBasedOnDistance(squadsInArea, mTargetLocation, squadsInArea);

   // Do something here ea
   while ((mTicksRemaining > 0) && (currentGameTime >= mNextTickTime))
   {
      BSmallDynamicSimArray<BSquad*> cryoKillableSquads;

      // Grab the current tick time, then increment the tick.
      --mTicksRemaining;
      mNextTickTime += mTickDuration;

      bool cryoed = false;

      // Determine our Cryoable squads.
      for (uint i=0; i<numSquadsInArea; i++)
      {
         // Get a valid squad.
         BSquad* pSquad = gWorld->getSquad(squadsInArea[i]);
         if (!pSquad)
            continue;

         // Otherwise, if it was already in the ignore list, skip it.
         if (isSquadIgnored(squadsInArea[i]))
            continue;

         // figure out the cryo amount based off of distance
         float cryoPct = 1.0f - ((pSquad->getPosition() - mTargetLocation).length() / mCryoRadius);
         cryoPct = Math::Clamp(cryoPct, 0.0f, 1.0f);
         cryoPct *= (1.0f - mMinCryoFalloff);         
         cryoPct += mMinCryoFalloff;

         // We can Cryo this one.
         pSquad->addCryo(pSquad->getProtoSquad()->getCryoPoints(), &mFreezingThawTime, &mFrozenThawTime);
         ignoreSquad(squadsInArea[i], cMaximumDWORD);

         // see if we can kill it, and if so, force a freeze and kill
         if (pSquad->isFrozen() && pSquad->getProtoSquad() && pSquad->getProtoSquad()->getFlagDiesWhenFrozen())
         {
            // in the case of transports, always kill them, never check the hp, 
            // because we don't want them to get into a bad frozen state
            BUnit* pLeaderUnit = pSquad->getLeaderUnit();
            if (pLeaderUnit && pLeaderUnit->isType(gDatabase.getOTIDTransporter()))
            {
               float hp = pSquad->getCurrentHP();
               pSquad->forceCryoFrozenKill(pPlayer, mCryoObjectID);
               mKillableHpLeft -= hp;
            }
            else if (mKillableHpLeft > 0.0f && pSquad->getCurrentHP() < mKillableHpLeft)
            {
               uint insertIndex = 0;
               float hp = pSquad->getCurrentHP();
               while (insertIndex < cryoKillableSquads.getSize() && cryoKillableSquads[insertIndex] && hp > cryoKillableSquads[insertIndex]->getCurrentHP())
                  ++insertIndex;

               cryoKillableSquads.insertAtIndex(pSquad, insertIndex);
            }
         }

         if(mReactionPlayed == false)
         {
            gWorld->createPowerAudioReactions(getPlayerID(), cSquadSoundChatterReactPowCryo, mTargetLocation, 60.0f);
            mReactionPlayed = true;
         }

         cryoed = true;
         break;
      }

      // go through the squads we might be able to kill and kill them until we run out of hp to kill with
      for (uint i = 0; i < cryoKillableSquads.getSize(); ++i)
      {
         BSquad* pSquad = cryoKillableSquads[i];
         if (!pSquad)
            continue;

         float hp = pSquad->getCurrentHP();
         if (hp < mKillableHpLeft)
         {
            pSquad->forceCryoFrozenKill(pPlayer, mCryoObjectID);
            mKillableHpLeft -= hp;
         }
      }

      if(cryoed)
         break;
   }

   // If we have spent all our ticks, return.
   if (mTicksRemaining == 0)
   {
      shutdown();
      return;
   }
}

//==============================================================================
//==============================================================================
void BPowerCryo::ignoreSquad(BEntityID squadID, DWORD expirationTime)
{
   uint numSquads = mIgnoreList.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      // We already have an entry, so just refresh it and return.
      if (mIgnoreList[i].first == squadID)
      {
         mIgnoreList[i].second = expirationTime;
         return;
      }
   }

   // We didn't find an entry, so add a new one.
   BEntityTimePair pairToAdd;
   pairToAdd.first = squadID;
   pairToAdd.second = expirationTime;
   mIgnoreList.add(pairToAdd);
}


//==============================================================================
//==============================================================================
void BPowerCryo::clearExpiredIgnores(DWORD currentGameTime)
{
   uint i = 0;
   while (i < mIgnoreList.getSize())
   {
      if (currentGameTime >= mIgnoreList[i].second)
         mIgnoreList.removeIndex(i, false);
      else
         i++;
   }
}


//==============================================================================
//==============================================================================
bool BPowerCryo::isSquadIgnored(BEntityID squadID) const
{
   // Nothing in the list.
   uint numSquads = mIgnoreList.getSize();
   if (numSquads == 0)
      return (false);

   for (uint i=0; i<numSquads; i++)
   {
      // Is it in the ignore list?
      if (mIgnoreList[i].first == squadID)
         return (true);
   }

   // Not in the list.
   return (false);
}

//==============================================================================
//==============================================================================
bool BPowerUserCryo::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mHudSounds);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserCryo::load(BStream* pStream, int saveType)
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
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCryo::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEARRAY(pStream, BEntityTimePair, mIgnoreList, uint8, 200);
   GFWRITEVAR(pStream, DWORD, mNextTickTime);
   GFWRITEVAR(pStream, BEntityID, mCryoObjectID);
   GFWRITEVECTOR(pStream, mDirection);
   GFWRITEVECTOR(pStream, mRight);
   GFWRITEVAR(pStream, BProtoObjectID, mCryoObjectProtoID);
   GFWRITEVAR(pStream, BProtoObjectID, mBomberProtoID);
   GFWRITEVAR(pStream, BObjectTypeID, mFilterTypeID);
   GFWRITEVAR(pStream, float, mCryoRadius);
   GFWRITEVAR(pStream, float, mMinCryoFalloff);
   GFWRITEVAR(pStream, DWORD, mTickDuration);
   GFWRITEVAR(pStream, uint, mTicksRemaining);
   GFWRITEVAR(pStream, float, mCryoAmountPerTick);
   GFWRITEVAR(pStream, float, mKillableHpLeft);
   GFWRITEVAR(pStream, float, mFreezingThawTime);   
   GFWRITEVAR(pStream, float, mFrozenThawTime);   
   GFWRITECLASS(pStream, saveType, mBomberData);
   GFWRITEBITBOOL(pStream, mReactionPlayed);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerCryo::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADARRAY(pStream, BEntityTimePair, mIgnoreList, uint8, 200);
   GFREADVAR(pStream, DWORD, mNextTickTime);
   GFREADVAR(pStream, BEntityID, mCryoObjectID);
   GFREADVECTOR(pStream, mDirection);
   GFREADVECTOR(pStream, mRight);
   GFREADVAR(pStream, BProtoObjectID, mCryoObjectProtoID);
   GFREADVAR(pStream, BProtoObjectID, mBomberProtoID);
   GFREADVAR(pStream, BObjectTypeID, mFilterTypeID);
   GFREADVAR(pStream, float, mCryoRadius);
   if (BPower::mGameFileVersion >= 4)
   {
      GFREADVAR(pStream, float, mMinCryoFalloff);
   }
   else
   {
      mMinCryoFalloff = 0.0f;
   }
   GFREADVAR(pStream, DWORD, mTickDuration);
   GFREADVAR(pStream, uint, mTicksRemaining);
   GFREADVAR(pStream, float, mCryoAmountPerTick);
   GFREADVAR(pStream, float, mKillableHpLeft);
   if (BPower::mGameFileVersion >= 2)
   {
      GFREADVAR(pStream, float, mFreezingThawTime);   
      GFREADVAR(pStream, float, mFrozenThawTime);   
   }
   else
   {
      mFreezingThawTime = 1.0f;
      mFrozenThawTime = 1.0f;
   }
   GFREADCLASS(pStream, saveType, mBomberData);
   GFREADBITBOOL(pStream, mReactionPlayed);
   gSaveGame.remapProtoObjectID(mCryoObjectProtoID);
   gSaveGame.remapProtoObjectID(mBomberProtoID);
   gSaveGame.remapObjectType(mFilterTypeID);
   return true;
}
