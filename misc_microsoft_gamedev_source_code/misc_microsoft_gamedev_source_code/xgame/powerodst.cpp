//==============================================================================
// powerODST.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "aimission.h"
#include "commandmanager.h"
#include "commands.h"
#include "powerODST.h"
#include "protopower.h"
#include "simhelper.h"
#include "uigame.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "soundmanager.h"
#include "powermanager.h"
#include "tactic.h"
#include "protosquad.h"
#include "TerrainSimRep.h"

//#define CONSOLE_OUTPUT

//==============================================================================
//==============================================================================
bool BPowerUserODST::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mHelpString.empty();
   mHudSounds.clear();
   mLOSMode = BWorld::cCPLOSFullVisible;
   mODSTProtoSquadID = gDatabase.getProtoSquad("unsc_inf_odst_01");
   mODSTProtoObjectID = gDatabase.getProtoObject("unsc_inf_odst_01");
   mCanFire = cCannotFireGeneric;
   mValidDropLocation = cOriginVector;

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
   if (pProtoPower->getPowerType() != PowerType::cODST)
      return (false);

   // Player cannot use power.
   mFlagNoCost = noCost;
   if (!mFlagNoCost && !pUserPlayer->canUsePower(protoPowerID))
      return (false);

   bool bSuccess = true;

   bSuccess = mHudSounds.loadFromProtoPower(pProtoPower, powerLevel);

   bool requiresLOS = true;
   if(pProtoPower->getDataBool(powerLevel, "RequiresLOS", requiresLOS) && !requiresLOS)
      mLOSMode = BWorld::cCPLOSDontCare;

   if (!bSuccess)
   {
      BFAIL("Failed to load required data for PowerUserODST!");
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
bool BPowerUserODST::shutdown()
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
bool BPowerUserODST::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserODST - is not initialized but is trying to handle input.");
   if (!mInitialized)
      return (false);
   // No user.
   BASSERTM(mpUser, "BPowerUserODST - mpUser is NULL.");
   if (!mpUser)
      return (false);
   // No player.
   BPlayer* pPlayer = mpUser->getPlayer();
   BASSERTM(pPlayer, "BPowerUserODST - pPlayer is NULL.");
   if (!pPlayer)
      return false;
   BPlayerID playerID = pPlayer->getID();
   // No camera.
   BCamera* pCamera = mpUser->getCamera();
   BASSERTM(pCamera, "BPowerUserODST - Unable to get the user camera.");
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
         if (mCanFire == cCannotFireGeneric)
         {
            gUI.playCantDoSound();
            return false;
         }
         else if (mCanFire == cCannotFireODST)
         {
            gSoundManager.playCue(mHudSounds.HudLastFireSound);
            return false;
         }
         else if (mCanFire == cCannotFirePop)
         {
            gSoundManager.playCue("play_vog_spc_population");
            return false;
         }
         else if (mCanFire == cCannotFireResources)
         {
            gSoundManager.playCue("play_vog_spc_resources");
            return false;
         }

         BASSERT(mCanFire == cCanFire);

         // set the can fire to false so that it can be updated later
         mCanFire = cCannotFireGeneric;

         gUI.playClickSound();

         // play the vo
         gSoundManager.playCue(mHudSounds.HudFireSound);

         // Add the command to create the power on the sim side for everyone.
         BPowerInputCommand* pCommand = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandPowerInput);
         if (pCommand)
         {
            pCommand->setSenders(1, &playerID);
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setRecipientType(BCommand::cPlayer);
            pCommand->setType(BPowerInputCommand::cTypeConfirm);
            pCommand->setVector(mValidDropLocation);
            pCommand->setPowerUserID(mID);
            pCommand->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
         }
      }
      return (true);
   }

   // Did the user just cancel the power?
   if (pInputInterface->isFunctionControl(BInputInterface::cInputClear, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (event == cInputEventControlStart)
      {
#ifdef CONSOLE_OUTPUT
         gConsoleOutput.status("%.2f: BPowerUserCarpetBombing::handleInput, mID %d, sending shutdown command", gWorld->getGametimeFloat(), mID);
#endif
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
void BPowerUserODST::updateUI()
{
   // compute a valid drop location - for some messed up reason, 
   // the drop location needs to be computed via this stack. 
   // It scares me why this is the only place this works
   mCanFire = canFire(mValidDropLocation);

   // update the help string
   BUIContext* pUIContext = mpUser->getUIContext();
   BPlayer* pPlayer = mpUser->getPlayer();
   BPowerEntry* pPowerEntry = (pPlayer) ? pPlayer->findPowerEntry(mProtoPowerID) : NULL;
   if (pUIContext && pPlayer && pPowerEntry && pPowerEntry->mItems.getSize() > 0)
   {
      mHelpString.locFormat(gDatabase.getLocStringFromID(25023), pPowerEntry->mItems[0].mUsesRemaining);
      pUIContext->setPowerOverlayText(mProtoPowerID, mHelpString);
   }
}

//==============================================================================
//==============================================================================
void BPowerUserODST::renderUI()
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserODST - is not initialized but is trying to render UI.");
   if (!mInitialized)
      return;
   // No user.
   BASSERTM(mpUser, "BPowerUserODST - mpUser is NULL.");
   if (!mpUser)
      return;

   // Render some UI.
   if(!mpUser->getFlagHaveHoverPoint())
      return;

   if(mCanFire == cCanFire)
      mpUser->renderReticle(BUIGame::cReticlePowerValid);
   else
      mpUser->renderReticle(BUIGame::cReticlePowerInvalid);
}

//==============================================================================
//==============================================================================
long BPowerUserODST::canFire(BVector& suggestedLocation)
{
   // return if we're trying to cast out of los
   DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPCheckSquadObstructions | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain | BWorld::cCPLOSCenterOnly | BWorld::cCPSetPlacementSuggestion;
   if(!mpUser->getFlagHaveHoverPoint())
      return cCannotFireGeneric;

   BPlayer* pUserPlayer = mpUser->getPlayer();
   if (!pUserPlayer)
      return cCannotFireGeneric;

   if (!pUserPlayer->canAffordToCastPower(mProtoPowerID))
      return cCannotFireResources;

   BProtoSquad* pProtoSquad = pUserPlayer->getProtoSquad(mODSTProtoSquadID);
   if (!pProtoSquad)
      return cCannotFireGeneric;

   // bail if we don't have enough pop
   BPopArray pops;
   pProtoSquad->getPops(pops);
   if (!pUserPlayer->checkPops(&pops))
      return cCannotFirePop;

   if (!pUserPlayer->hasAvailablePowerEntryUses(mProtoPowerID))
      return cCannotFireODST;

   // Player cannot use power.
   if (!pUserPlayer->canUsePower(mProtoPowerID))
      return cCannotFireGeneric;

   const BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pProtoPower)
      return cCannotFireGeneric;

   long searchScale = Math::Max(1, Math::FloatToIntTrunc(pProtoPower->getUIRadius() * gTerrainSimRep.getReciprocalDataTileScale()));

   BVector tempSuggestedLocation = XMVectorReplicate(-1.0f);
   if(!gWorld->checkPlacement(mODSTProtoObjectID,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempSuggestedLocation, cZAxisVector, mLOSMode, flags, searchScale, NULL, NULL, -1, mODSTProtoSquadID, true) ||
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerLocation(mpUser->getHoverPoint(), this))
      return cCannotFireGeneric;

   if (tempSuggestedLocation == XMVectorReplicate(-1.0f))
      suggestedLocation = mpUser->getHoverPoint();
   else
      suggestedLocation = tempSuggestedLocation;

   return cCanFire;
}

//==============================================================================
//==============================================================================
void BPowerUserODST::setupUser()
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
      BPlayer* pUserPlayer = mpUser->getPlayer();
      BPowerEntry* pPowerEntry = (pUserPlayer) ? pUserPlayer->findPowerEntry(mProtoPowerID) : NULL;
      if (pPowerEntry && pPowerEntry->mItems.getSize() > 0)
      {
         mHelpString.locFormat(gDatabase.getLocStringFromID(25023), pPowerEntry->mItems[0].mUsesRemaining);
         pUIContext->setPowerOverlayText(mProtoPowerID, mHelpString);
      }
   }
}

//==============================================================================
//==============================================================================
bool BPowerODST::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mSquadSpawnDelay = 0.0f;
   mActiveDrops.clear();
   mProjectileProtoID = cInvalidProtoObjectID;
   mODSTProtoSquadID = gDatabase.getProtoSquad("unsc_inf_odst_01");
   mAddToMissionID = cInvalidAIMissionID;
   mReadyForShutdown = false;

#ifdef SYNC_Command
   syncCommandData("BPowerUserODST::init playerID", playerID);
   syncCommandData("BPowerUserODST::init powerLevel", (DWORD)powerLevel);
   syncCommandData("BPowerUserODST::init powerUserID player", powerUserID.getPlayerID());
   syncCommandData("BPowerUserODST::init powerUserID powerType", (DWORD)powerUserID.getPowerType());
   syncCommandData("BPowerUserODST::init ownerSquadID", ownerSquadID.asLong());
   syncCommandData("BPowerUserODST::init targetLocation", targetLocation);
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
   syncCommandData("BPowerUserODST::init pPlayer bSuccess", bSuccess);
#endif

   bSuccess = (bSuccess && pProtoPower != NULL);
#ifdef SYNC_Command
   syncCommandData("BPowerUserODST::init pProtoPower bSuccess", bSuccess);
#endif

   if (!mFlagIgnoreAllReqs)
   {
      bSuccess = (bSuccess && pPlayer->canUsePower(mProtoPowerID));
#ifdef SYNC_Command
      syncCommandData("BPowerUserODST::init canUserPower bSuccess", bSuccess);
#endif
   }

   bSuccess = (bSuccess && pProtoPower->getDataProtoObject(powerLevel, "Projectile", mProjectileProtoID));
   bSuccess = (bSuccess && pProtoPower->getDataFloat(powerLevel, "SquadSpawnDelay", mSquadSpawnDelay));

#ifdef SYNC_Command
   syncCommandData("BPowerUserODST::init bSuccess", bSuccess);
   syncCommandData("BPowerUserODST::init Projectile", mProjectileProtoID);
#endif

   // Did we succeed on all our setup?
   if (!bSuccess)
   {
      BFAIL("Failed to load required data for PowerODST");
      shutdown();
      return (false);
   }
   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   mElapsed = 0.0f;

   // All initialized.
   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerODST::shutdown()
{
   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if (pUser)
   {
      BPowerUser* pPowerUser = pUser->getPowerUser();
      if (pPowerUser && pPowerUser->getID() == mPowerUserID)
         pPowerUser->shutdown();
   }

   // if we have projectiles left (somehow?) 

   mFlagDestroy = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerODST::submitInput(BPowerInput powerInput)
{
   BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if(!pProtoPower)
   {
      shutdown();
      return false;
   }

   BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
   {
      shutdown();
      return false;
   }

   if (powerInput.mType == PowerInputType::cUserOK)
   {
      // Player cannot use power.
      if (!pPlayer->canUsePower(mProtoPowerID))
         return true;
      
      // bail if we don't have enough pop
      BPopArray pops;
      BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(mODSTProtoSquadID);
      pProtoSquad->getPops(pops);
      if (!pPlayer->checkPops(&pops))
         return true;

      pPlayer->usePower(mProtoPowerID, cInvalidObjectID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

      BODSTDrop& odstDrop = mActiveDrops.grow();
      odstDrop.SpawnSquadTime = mElapsed + mSquadSpawnDelay;

      // spawn projectile
      BObjectCreateParms projParms;
      projParms.mPlayerID = mPlayerID;
      projParms.mProtoObjectID = mProjectileProtoID;
      projParms.mPosition = powerInput.mVector + BVector(10.0f, 80.0f, 10.0f);
      BProtoAction* pProtoAction = BPowerHelper::getFirstProtoAction(*pPlayer, mProjectileProtoID);

      BEntityID projectileId = gWorld->launchProjectile(projParms, powerInput.mVector, XMVectorZero(), XMVectorZero(), 0.0f, pProtoAction, pProtoAction, cInvalidObjectID, NULL, -1, false, true, true);
      BASSERT(projectileId != cInvalidObjectID);

      // spawn and hide squad
      BEntityID createdSquadID = gWorld->createEntity(mODSTProtoSquadID, true, mPlayerID, powerInput.mVector, cZAxisVector, cXAxisVector, true);
      BSquad* pSquad = gWorld->getSquad(createdSquadID);
      BASSERT(pSquad);
      odstDrop.SquadId = createdSquadID;

      // If we set a mission to add the odst to, do that.
      if (mAddToMissionID != cInvalidAIMissionID)
      {
         BAIMission* pMission = gWorld->getAIMission(mAddToMissionID);
         if (pMission)
            pMission->addSquad(createdSquadID);
      }

      pSquad->setFlagSelectable(false);
      BEntityIDArray childList = pSquad->getChildList();
      for (uint i = 0; i < childList.getSize(); ++i)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
         {
            pUnit->setFlagNoRender(true);
            pUnit->setFlagInvulnerable(true);
         }
      }

      return true;
   }
   else if (powerInput.mType == PowerInputType::cUserCancel)
   {
      mReadyForShutdown = true;

      return true;
   }

   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerODST::submitInput() received powerInput of unsupported type.");
   return (false);
}


//==============================================================================
//==============================================================================
void BPowerODST::update(DWORD currentGameTime, float lastUpdateLength)
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

   uint i = 0;
   while (i < mActiveDrops.getSize())
   {
      if (mElapsed > mActiveDrops[i].SpawnSquadTime)
      {
         BSquad* pSquad = gWorld->getSquad(mActiveDrops[i].SquadId);
         BASSERT(pSquad);

         // unhide squad
         pSquad->setFlagSelectable(true);
         BEntityIDArray childList = pSquad->getChildList();
         for (uint j = 0; j < childList.getSize(); ++j)
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(j));
            if (pUnit)
            {
               pUnit->setFlagNoRender(false);
               pUnit->setFlagInvulnerable(false);
            }
         }

         mActiveDrops.removeIndex(i);
         continue;
      }

      ++i;
   }

   if (mReadyForShutdown && mActiveDrops.getSize() == 0)
      shutdown();
}

//==============================================================================
//==============================================================================
bool BPowerUserODST::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITESTRING(pStream, BUString, mHelpString, 1000);
   GFWRITECLASS(pStream, saveType, mHudSounds);
   GFWRITEVAR(pStream, long, mLOSMode);
   GFWRITEVAR(pStream, BProtoSquadID, mODSTProtoSquadID);
   GFWRITEVAR(pStream, BProtoObjectID, mODSTProtoObjectID);
   GFWRITEVAR(pStream, long, mCanFire);
   GFWRITEVECTOR(pStream, mValidDropLocation);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserODST::load(BStream* pStream, int saveType)
{
   if (!BPowerUser::load(pStream, saveType))
      return false;
   GFREADSTRING(pStream, BUString, mHelpString, 1000);
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
   GFREADVAR(pStream, long, mLOSMode);
   GFREADVAR(pStream, BProtoSquadID, mODSTProtoSquadID);
   GFREADVAR(pStream, BProtoObjectID, mODSTProtoObjectID);
   if (BPowerUser::mGameFileVersion >= 4)
   {
      GFREADVAR(pStream, long, mCanFire);
   }
   else
   {
      bool tempBool = false;
      GFREADVAR(pStream, bool, tempBool);
      mCanFire = (tempBool) ? cCanFire : cCannotFireGeneric;
   }
   GFREADVECTOR(pStream, mValidDropLocation);
   gSaveGame.remapProtoSquadID(mODSTProtoSquadID);
   gSaveGame.remapProtoObjectID(mODSTProtoObjectID);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerODST::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mSquadSpawnDelay);
   GFWRITEARRAY(pStream, BODSTDrop, mActiveDrops, uint8, 40);
   GFWRITEVAR(pStream, BProtoObjectID, mProjectileProtoID); 
   GFWRITEVAR(pStream, BProtoSquadID, mODSTProtoSquadID);
   GFWRITEVAR(pStream, BAIMissionID, mAddToMissionID);
   GFWRITEVAR(pStream, bool, mReadyForShutdown);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerODST::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mSquadSpawnDelay);
   GFREADARRAY(pStream, BODSTDrop, mActiveDrops, uint8, 40);
   if (BPower::mGameFileVersion <= 9)
   {
      BCueIndex tempCue;
      GFREADVAR(pStream, BCueIndex, tempCue);
   }
   GFREADVAR(pStream, BProtoObjectID, mProjectileProtoID); 
   GFREADVAR(pStream, BProtoSquadID, mODSTProtoSquadID);
   mAddToMissionID = cInvalidAIMissionID;
   if (BPower::mGameFileVersion >= 18)
      GFREADVAR(pStream, BAIMissionID, mAddToMissionID);
   GFREADVAR(pStream, bool, mReadyForShutdown);
   gSaveGame.remapProtoObjectID(mProjectileProtoID);
   gSaveGame.remapProtoSquadID(mODSTProtoSquadID);
   return true;
}
