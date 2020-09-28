//==============================================================================
// powerrepair.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "commandmanager.h"
#include "commands.h"
#include "powerrepair.h"
#include "protopower.h"
#include "simhelper.h"
#include "uigame.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "powermanager.h"
#include "protosquad.h"
#include "soundmanager.h"

//#define CONSOLE_OUTPUT

//==============================================================================
//==============================================================================
bool BPowerUserRepair::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
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

   if (!mHudSounds.loadFromProtoPower(pProtoPower, powerLevel))
      return false;

   // Wrong power type.
   if (pProtoPower->getPowerType() != PowerType::cRepair)
      return (false);

   // Player cannot use power.
   mFlagNoCost = noCost;
   if (!mFlagNoCost && !pUserPlayer->canUsePower(protoPowerID))
      return (false);

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
bool BPowerUserRepair::shutdown()
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
bool BPowerUserRepair::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserRepair - is not initialized but is trying to handle input.");
   if (!mInitialized)
      return (false);
   // No user.
   BASSERTM(mpUser, "BPowerUserRepair - mpUser is NULL.");
   if (!mpUser)
      return (false);
   // No camera.
//-- FIXING PREFIX BUG ID 1316
   const BCamera* pCamera = mpUser->getCamera();
//--
   BASSERTM(pCamera, "BPowerUserRepair - Unable to get the user camera.");
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

         gSoundManager.playCue(mHudSounds.HudFireSound);

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
void BPowerUserRepair::renderUI()
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserRepair - is not initialized but is trying to render UI.");
   if (!mInitialized)
      return;
   // No user.
   BASSERTM(mpUser, "BPowerUserRepair - mpUser is NULL.");
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
void BPowerUserRepair::setupUser()
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
      pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(24732));
   }
}

//==============================================================================
//==============================================================================
bool BPowerRepair::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized
   initBase();
   mSquadsRepairing.clear();
   mIgnoreList.clear();
   mNextTickTime = 0;
   mRepairObjectID = cInvalidObjectID;
   mRepairAttachmentProtoID = cInvalidProtoObjectID;
   mFilterTypeID = cInvalidObjectTypeID;
   mRepairRadius = 0.0f;
   mTickDuration = 0;
   mRepairCombatValuePerTick = 0.0f;
   mCooldownTimeIfDamaged = 0;
   mTicksRemaining = 0;
   mSpreadAmongSquads = false;
   mAllowReinforce = false;
   mIgnorePlacement = false;
   mHealAny = false;
   mNeverStops = false;

   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);

   if (pProtoPower)
   {
      pProtoPower->getDataBool(powerLevel, "IgnorePlacement", mIgnorePlacement);
      mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();
   }

   #ifdef SYNC_Command
      syncCommandData("BPowerRepair::init playerID", playerID);
      syncCommandData("BPowerRepair::init powerLevel", (DWORD)powerLevel);
      syncCommandData("BPowerRepair::init powerUserID player", powerUserID.getPlayerID());
      syncCommandData("BPowerRepair::init powerUserID powerType", (DWORD)powerUserID.getPowerType());
      syncCommandData("BPowerRepair::init ownerSquadID", ownerSquadID.asLong());
      syncCommandData("BPowerRepair::init targetLocation", targetLocation);
      syncCommandData("BPowerRepair::init mIgnorePlacement", mIgnorePlacement);
   #endif

   BVector tempVec;
   bool placement = true;
   if (!mIgnorePlacement)
   {
      placement = gWorld->checkPlacement(-1,  playerID, targetLocation, tempVec, cZAxisVector, BWorld::cCPLOSFullVisible, BWorld::cCPLOSCenterOnly);
      #ifdef SYNC_Command
         syncCommandData("BPowerRepair::init placement", placement);
      #endif
   }

   bool powerManager = (gWorld->getPowerManager() != NULL);
   #ifdef SYNC_Command
      syncCommandData("BPowerRepair::init powerManager", powerManager);
   #endif

   bool validPowerLoc = true;
   if (powerManager)
   {
      validPowerLoc = gWorld->getPowerManager()->isValidPowerLocation(targetLocation, this);
      #ifdef SYNC_Command
         syncCommandData("BPowerRepair::init validPowerLoc", validPowerLoc);
      #endif
   }

   if (!placement || !powerManager || !validPowerLoc)
   {
      shutdown();
      return false;
   }

   BProtoObjectID repairObjectProtoID = cInvalidProtoID;
   float cooldown = 0.0f;
   float tickduration = 0.0f;
   int numTicks = 0;
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   mFlagIgnoreAllReqs = ignoreAllReqs;
   bool bSuccess = true;

   if (bSuccess)
      bSuccess = (pPlayer != NULL);
   #ifdef SYNC_Command
      syncCommandData("BPowerRepair::init pPlayer bSuccess", bSuccess);
   #endif

   if (bSuccess)
      bSuccess = (pProtoPower != NULL);
   #ifdef SYNC_Command
      syncCommandData("BPowerRepair::init pProtoPower bSuccess", bSuccess);
   #endif

   if (!mFlagIgnoreAllReqs && bSuccess)
   {
      bSuccess = pPlayer->canUsePower(mProtoPowerID);
      #ifdef SYNC_Command
         syncCommandData("BPowerRepair::init canUserPower bSuccess", bSuccess);
      #endif
   }

   if (bSuccess)
      bSuccess = pProtoPower->getDataProtoObject(powerLevel, "RepairObject", repairObjectProtoID);
   if (bSuccess)
   {
      mRepairAttachmentProtoID = cInvalidProtoObjectID;
      pProtoPower->getDataProtoObject(powerLevel, "RepairAttachment", mRepairAttachmentProtoID);
   }
   mNeverStops = false;
   if (bSuccess)
      pProtoPower->getDataBool(powerLevel, "NeverStops", mNeverStops);

   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "RepairRadius", mRepairRadius);
   if (bSuccess)
      bSuccess = (mRepairRadius > 0.0f);
   if (bSuccess)
      bSuccess = pProtoPower->getDataObjectType(powerLevel, "FilterType", mFilterTypeID);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "TickDuration", tickduration);
   if (bSuccess)
      bSuccess = (tickduration > 0.0f);
   if (bSuccess && !mNeverStops)
      bSuccess = pProtoPower->getDataInt(powerLevel, "NumTicks", numTicks);
   if (bSuccess)
      bSuccess = (mNeverStops || numTicks > 0);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "RepairCombatValuePerTick", mRepairCombatValuePerTick);
   if (bSuccess)
      bSuccess = (mRepairCombatValuePerTick > 0.0f);
   if (bSuccess)
      bSuccess = pProtoPower->getDataBool(powerLevel, "SpreadAmongSquads", mSpreadAmongSquads);
   if (bSuccess)
      bSuccess = pProtoPower->getDataBool(powerLevel, "AllowReinforce", mAllowReinforce);
   if (bSuccess)
      bSuccess = pProtoPower->getDataFloat(powerLevel, "CooldownTimeIfDamaged", cooldown);
   if (bSuccess)
      bSuccess = (cooldown >= 0.0f);
   if (bSuccess)
      bSuccess = pProtoPower->getDataBool(powerLevel, "HealAny", mHealAny);

   #ifdef SYNC_Command
      syncCommandData("BPowerRepair::init bSuccess", bSuccess);
      syncCommandData("BPowerRepair::init repairObjectProtoID", repairObjectProtoID);
      syncCommandData("BPowerRepair::init mRepairAttachmentProtoID", mRepairAttachmentProtoID);
      syncCommandData("BPowerRepair::init mRepairRadius", mRepairRadius);
      syncCommandData("BPowerRepair::init mFilterTypeID", mFilterTypeID);
      syncCommandData("BPowerRepair::init tickduration", tickduration);
      syncCommandData("BPowerRepair::init numTicks", numTicks);
      syncCommandData("BPowerRepair::init mRepairCombatValuePerTick", mRepairCombatValuePerTick);
      syncCommandData("BPowerRepair::init mSpreadAmongSquads", mSpreadAmongSquads);
      syncCommandData("BPowerRepair::init mAllowReinforce", mAllowReinforce);
      syncCommandData("BPowerRepair::init cooldown", cooldown);
      syncCommandData("BPowerRepair::init mIgnorePlacement", mIgnorePlacement);
      syncCommandData("BPowerRepair::init mHealAny", mHealAny);
      syncCommandData("BPowerRepair::init mNeverStops", mNeverStops);
   #endif

   if (bSuccess)
   {
      // create arrow
      BObjectCreateParms parms;
      parms.mPlayerID = playerID;
      parms.mPosition = targetLocation;
      parms.mType = BEntity::cClassTypeObject;
      parms.mProtoObjectID = repairObjectProtoID;
//-- FIXING PREFIX BUG ID 1317
      const BObject* pObject = gWorld->createObject(parms);
//--
      if (pObject)
         mRepairObjectID = pObject->getID();
      else
         bSuccess = false;
      syncCommandData("BPowerRepair::init create arrow bSuccess", bSuccess);
   }

   // Did we succeed on all our setup?
   if (!bSuccess)
   {
      shutdown();
      return (false);
   }
   
   mTickDuration = static_cast<DWORD>(1000.0f * tickduration);
   mCooldownTimeIfDamaged = static_cast<DWORD>(1000.0f * cooldown);
   mTicksRemaining = static_cast<uint>(numTicks);
   mNextTickTime = gWorld->getGametime() + mTickDuration;
   mSquadsRepairing.reserve(32);
   mSquadsRepairing.resize(0);
   mIgnoreList.reserve(32);
   mIgnoreList.resize(0);

   // Pay the cost now.
   pPlayer->usePower(mProtoPowerID, cInvalidObjectID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

   // casting variables
   mPlayerID = playerID;
   mPowerUserID = powerUserID;
   mPowerLevel = powerLevel;
   mOwnerID = ownerSquadID;
   mTargetLocation = targetLocation;

   // All initialized.
   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerRepair::shutdown()
{
   // Remove the refcount for any squads still getting repaired when the power stopped.
   handleLeavingSquads(mSquadsRepairing);

   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if (pUser)
   {
      BPowerUser* pPowerUser = pUser->getPowerUser();
      if (pPowerUser && pPowerUser->getID() == mPowerUserID)
         pPowerUser->shutdown();
   }

   // Kill the green heal thing.
   if(mRepairObjectID.isValid())
   {
      BObject* pRepairObject = gWorld->getObject(mRepairObjectID);
      if (pRepairObject)
      {
         pRepairObject->kill(false);
         mRepairObjectID = cInvalidObjectID;
      }
   }

   mFlagDestroy = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerRepair::submitInput(BPowerInput powerInput)
{
   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerRepair::submitInput() received powerInput of unsupported type.");
   return (false);
}


//==============================================================================
//==============================================================================
void BPowerRepair::update(DWORD currentGameTime, float lastUpdateLength)
{
   // Already marked for destroy.
   if (mFlagDestroy)
      return;

   BPlayerID playerID = mPlayerID;

   // Don't bother yet
   if (currentGameTime < mNextTickTime)
      return;

   // Bomb check.
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pPlayer || !pProtoPower)
   {
      shutdown();
      return;
   }

   // valid position check
   BVector tempVec;
   if(!gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mTargetLocation, this))
   {
      shutdown();
      return;
   }

   // Do a query for allied squads in the heal zone only once per update (even though we may heal multiple ticks worth.)
   BUnitQuery unitQuery(mTargetLocation, mRepairRadius, true);
   if (mHealAny)
      unitQuery.setRelation(playerID, cRelationTypeAny);
   else
      unitQuery.setRelation(playerID, cRelationTypeAlly);

   if (mFilterTypeID != cInvalidObjectTypeID)
      unitQuery.addObjectTypeFilter(mFilterTypeID);

   BEntityIDArray squadsInArea(0, 32);
   gWorld->getSquadsInArea(&unitQuery, &squadsInArea, false);
   uint numSquadsInArea = squadsInArea.getSize();

   // Some other helper arrays.
   BEntityIDArray repairableSquads(0, 32);  // All possible squads who can be repaired from this power.
   BEntityIDArray arrivingSquads(0, 32);    // Squads who are just starting to be repaired.
   BEntityIDArray leavingSquads(0, 32);     // Squads who are just finishing being repaired.

   // Do something here ea
   while ((mTicksRemaining > 0 || mNeverStops) && (currentGameTime >= mNextTickTime))
   {
      // Grab the current tick time, then increment the tick.
      DWORD currentTickTime = mNextTickTime;
      if (!mNeverStops)
         mTicksRemaining--;
      mNextTickTime += mTickDuration;

      // Clear out our ignored squads whose penalty has expired this tick.
      if (mCooldownTimeIfDamaged > 0)
         clearExpiredIgnores(currentTickTime);

      // Determine our repairable squads.
      repairableSquads.resize(0);
      for (uint i=0; i<numSquadsInArea; i++)
      {
         // Get a valid squad.
         const BSquad* pSquad = gWorld->getSquad(squadsInArea[i]);
         if (!pSquad)
            continue;

         // If this power is equipped with a penalty for being attacked...
         if (mCooldownTimeIfDamaged > 0)
         {
            // If it has been attacked during the last tick, add it to the ignored list and skip.
            DWORD lastDamagedTime = pSquad->getLastDamagedTime();
            int damageToTickDelta = currentTickTime - lastDamagedTime;
            if (damageToTickDelta < (int) mTickDuration)
            {
               ignoreSquad(squadsInArea[i], currentTickTime + mCooldownTimeIfDamaged);
               continue;
            }

            // Otherwise, if it was already in the ignore list, skip it.
            if (isSquadIgnored(squadsInArea[i]))
               continue;
         }

         // If it is at full health, skip it.
         if (pSquad->getHPPercentage() >= 1.0f)
            continue;

         // We can repair this one.
         repairableSquads.add(squadsInArea[i]);
      }

      // Find out who is just starting to be repaired and who is just finishing.
      BSimHelper::diffEntityIDArrays(repairableSquads, mSquadsRepairing, &arrivingSquads, &leavingSquads, NULL);

      // Refresh the list of repairing squads.
      mSquadsRepairing = repairableSquads;

      // Repair the squads.
      BSimHelper::repairByCombatValue(mSquadsRepairing, mRepairCombatValuePerTick, mSpreadAmongSquads, mAllowReinforce);

      // Handle arriving squads (adding refcounts, attaching visuals.)
      handleArrivingSquads(arrivingSquads);

      // Handle leaving squads (removing refcounts, destroying attached visuals.)
      handleLeavingSquads(leavingSquads);
   }

   // If we have spent all our ticks, return.
   if (mTicksRemaining == 0 && !mNeverStops)
   {
      shutdown();
      return;
   }
}


//==============================================================================
//==============================================================================
void BPowerRepair::handleArrivingSquads(const BEntityIDArray& squadIDs)
{
   uint numSquads = squadIDs.getSize();
   if (numSquads == 0)
      return;

   BRefCountType regenRefType = gDatabase.getRCTRegen();
   for (uint i=0; i<numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;

      BEntityRef* pEntityRef = pSquad->getFirstEntityRefByType(regenRefType);
      if (pEntityRef)
      {
         // This squad already has the regen ref count, so just increment.
         pEntityRef->mData1 += 1;
      }
      else
      {
         // This squad did not have the regen ref count, so add it.
         pSquad->addEntityRef((short)regenRefType, cInvalidObjectID, 1, 0);
         BVector offset =  cOriginVector;
         if (pSquad->getProtoSquad())
            offset = pSquad->getProtoSquad()->getHPBarOffset();

         // Also add the visual effect attachment.
         if (mRepairAttachmentProtoID != cInvalidProtoObjectID)
         {
            BUnit* pUnit = pSquad->getLeaderUnit();
            if (pUnit)
            {
               BEntityID attachmentID = gWorld->createEntity(mRepairAttachmentProtoID, false, getPlayerID(), pUnit->getPosition() + offset, pUnit->getForward(), pUnit->getRight(), true);
               BObject* pObject = gWorld->getObject(attachmentID);
               if (pObject)
                  pUnit->addAttachment(pObject, -1, -1, false, true);
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BPowerRepair::handleLeavingSquads(const BEntityIDArray& squadIDs)
{
   uint numSquads = squadIDs.getSize();
   if (numSquads == 0)
      return;

   BRefCountType regenRefType = gDatabase.getRCTRegen();
   for (uint i=0; i<numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;

      // Handle the entity ref adjustment & determine whether to remove the attachments.
      bool removeAttachments = false;
      BEntityRef* pEntityRef = pSquad->getFirstEntityRefByType(regenRefType);
      if (pEntityRef)
      {
         if (pEntityRef->mData1 <= 1)
         {
            pSquad->removeEntityRef(pEntityRef->mType, pEntityRef->mID);
            removeAttachments = true;
         }
         else
         {
            pEntityRef->mData1 -= 1;
         }
      }
      else
      {
         removeAttachments = true;
      }

      // Also add the visual effect attachment.
      if (removeAttachments && (mRepairAttachmentProtoID != cInvalidProtoObjectID))
      {
         const BEntityIDArray& childUnitIDs = pSquad->getChildList();
         uint numChildren = childUnitIDs.getSize();
         for (uint j=0; j<numChildren; j++)
         {
            BUnit* pUnit = gWorld->getUnit(childUnitIDs[j]);
            if (pUnit)
               pUnit->removeAttachmentsOfType(mRepairAttachmentProtoID);
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BPowerRepair::ignoreSquad(BEntityID squadID, DWORD expirationTime)
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
void BPowerRepair::clearExpiredIgnores(DWORD currentGameTime)
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
bool BPowerRepair::isSquadIgnored(BEntityID squadID) const
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
bool BPowerUserRepair::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mHudSounds);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserRepair::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;
   if (BPowerUser::mGameFileVersion >= 5)
   {
      GFREADCLASS(pStream, saveType, mHudSounds);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerRepair::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEARRAY(pStream, BEntityID, mSquadsRepairing, uint8, 200);
   GFWRITEARRAY(pStream, BEntityTimePair, mIgnoreList, uint8, 200);
   GFWRITEVAR(pStream, DWORD, mNextTickTime);
   GFWRITEVAR(pStream, BEntityID, mRepairObjectID);
   GFWRITEVAR(pStream, BProtoObjectID, mRepairAttachmentProtoID);
   GFWRITEVAR(pStream, BObjectTypeID, mFilterTypeID);
   GFWRITEVAR(pStream, float, mRepairRadius);
   GFWRITEVAR(pStream, DWORD, mTickDuration);
   GFWRITEVAR(pStream, float, mRepairCombatValuePerTick);
   GFWRITEVAR(pStream, DWORD, mCooldownTimeIfDamaged);
   GFWRITEVAR(pStream, uint, mTicksRemaining);
   GFWRITEVAR(pStream, bool, mSpreadAmongSquads);
   GFWRITEVAR(pStream, bool, mAllowReinforce);
   GFWRITEVAR(pStream, bool, mIgnorePlacement);
   GFWRITEVAR(pStream, bool, mHealAny);
   GFWRITEVAR(pStream, bool, mNeverStops);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerRepair::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADARRAY(pStream, BEntityID, mSquadsRepairing, uint8, 200);
   GFREADARRAY(pStream, BEntityTimePair, mIgnoreList, uint8, 200);
   GFREADVAR(pStream, DWORD, mNextTickTime);
   GFREADVAR(pStream, BEntityID, mRepairObjectID);
   GFREADVAR(pStream, BProtoObjectID, mRepairAttachmentProtoID);
   GFREADVAR(pStream, BObjectTypeID, mFilterTypeID);
   GFREADVAR(pStream, float, mRepairRadius);
   GFREADVAR(pStream, DWORD, mTickDuration);
   GFREADVAR(pStream, float, mRepairCombatValuePerTick);
   GFREADVAR(pStream, DWORD, mCooldownTimeIfDamaged);
   GFREADVAR(pStream, uint, mTicksRemaining);
   GFREADVAR(pStream, bool, mSpreadAmongSquads);
   GFREADVAR(pStream, bool, mAllowReinforce);
   GFREADVAR(pStream, bool, mIgnorePlacement);
   GFREADVAR(pStream, bool, mHealAny);
   GFREADVAR(pStream, bool, mNeverStops);
   gSaveGame.remapProtoObjectID(mRepairAttachmentProtoID);
   gSaveGame.remapObjectType(mFilterTypeID);
   return true;
}
