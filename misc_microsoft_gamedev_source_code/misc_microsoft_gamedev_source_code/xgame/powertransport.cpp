//==============================================================================
// powerTransport.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "commandmanager.h"
#include "commands.h"
#include "powerTransport.h"
#include "protopower.h"
#include "powerhelper.h"
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
#include "unitquery.h"
#include "SimOrderManager.h"
#include "squadactiontransport.h"
#include "uimanager.h"
#include "decalManager.h"

//#define CONSOLE_OUTPUT

static const BColor cValidColor = BColor(0.3f, 0.7f, 0.9f);
static const BColor cInvalidColor = BColor(0.9f, 0.2f, 0.2f);

//==============================================================================
//==============================================================================
bool BPowerUserTransport::init(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BUser* pUser, BEntityID ownerSquadID, BVector targetLocation, bool noCost /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mHudSounds.clear();
   mSquadsToTransport.clear();
   mTargetedSquads.clear();
   mLOSMode = BWorld::cCPLOSFullVisible;
   mGotPickupLocation = false;
   mBaseDecal = -1;
   mMoverDecal = -1;
   mPickupLocation = cInvalidVector;

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
   if (pProtoPower->getPowerType() != PowerType::cTransport)
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
      BFAIL("Failed to load required data for PowerUserTransport!");
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

   initDecals();

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserTransport::shutdown()
{
   // fix up the user's mode
   mpUser->unlockUser();

   // Turn off the power overlay if any.
   BUIContext* pUIContext = mpUser->getUIContext();
   if (pUIContext)
      pUIContext->setPowerOverlayVisible(mProtoPowerID, false);

   // untarget squads
   for(uint8 i = 0; i < mTargetedSquads.getNumber(); i++)
   {
      BSquad* pSquad = gWorld->getSquad(mTargetedSquads[i]);
      if(!pSquad)
         continue;

      pSquad->setTargettingSelection(false);
   }

   // destroy decals
   if(mBaseDecal != -1)
      gDecalManager.destroyDecal(mBaseDecal);
   if(mMoverDecal != -1)
      gDecalManager.destroyDecal(mMoverDecal);
   mBaseDecal = -1;
   mMoverDecal = -1;

   //Turn off the hud environment sound
   if(mHudSounds.HudStopEnvSound != cInvalidCueIndex)
      gSoundManager.playCue(mHudSounds.HudStopEnvSound);

   mFlagDestroy = true;

   return (true);
}


//==============================================================================
//==============================================================================
bool BPowerUserTransport::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserTransport - is not initialized but is trying to handle input.");
   if (!mInitialized)
      return (false);
   // No user.
   BASSERTM(mpUser, "BPowerUserTransport - mpUser is NULL.");
   if (!mpUser)
      return (false);
   // No camera.
   BCamera* pCamera = mpUser->getCamera();
   BASSERTM(pCamera, "BPowerUserTransport - Unable to get the user camera.");
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
         if (!canUsePowerAtHoverPoint(false))
         {
            gUI.playCantDoSound();
            return false;
         }

         gUI.playClickSound();
         BPlayerID playerID = mpUser->getPlayerID();

         // play the vo
         if (!mGotPickupLocation)
            gSoundManager.playCue(mHudSounds.HudFireSound);
         else
            gSoundManager.playCue(mHudSounds.HudLastFireSound);

         // Add the command to create the power on the sim side for everyone.
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

         if (!mGotPickupLocation)
         {
            mGotPickupLocation = true;
            mPickupLocation = mpUser->getHoverPoint();

            BUIContext* pUIContext = mpUser->getUIContext();
            if (pUIContext)
               pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(25034));
         }
         else
            shutdown();
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
         cancelPower();
      }
      return (true);
   }

   // Didn't handle stuff.
   return (false);
}

//==============================================================================
void BPowerUserTransport::cancelPower()
{
   gUI.playClickSound();

   BPowerInputCommand* c = (BPowerInputCommand*)gWorld->getCommandManager()->createCommand( mpUser->getPlayerID(), cCommandPowerInput );
   if( c )
   {
      // Set up the command.
      long playerID = mpUser->getPlayerID();
      c->setSenders( 1, &playerID );
      c->setSenderType( BCommand::cPlayer );
      c->setRecipientType( BCommand::cPlayer );
      c->setType( BPowerInputCommand::cTypeShutdown );
      c->setPowerUserID(mID);
      c->setFlag(BPowerInputCommand::cNoCost, mFlagNoCost);
      gWorld->getCommandManager()->addCommandToExecute( c );
   }

   if (mGotPickupLocation)
   {
      mGotPickupLocation = false;

      BUIContext* pUIContext = mpUser->getUIContext();
      if (pUIContext)
         pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(25033));
   }
   else
   {
      gSoundManager.playCue(mHudSounds.HudAbortSound);

      shutdown();
   }
}


//==============================================================================
//==============================================================================
void BPowerUserTransport::updateUI()
{
   // Not initialized
   BASSERTM(mInitialized, "BPowerUserTransport - is not initialized but is trying to render UI.");
   if (!mInitialized)
      return;
   // No user.
   BASSERTM(mpUser, "BPowerUserTransport - mpUser is NULL.");
   if (!mpUser)
      return;

   const BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pProtoPower)
      return;

   bool valid = canUsePowerAtHoverPoint(false);
   const BVector& hoverPoint = mpUser->getHoverPoint();

   updateDecals(valid);

   if (!mGotPickupLocation)
   {
/*
      // the pickup visuals go here
      float thickness = 0.6f;
      if (pProtoPower->getUIRadius() > 0.0f)
         gTerrainSimRep.addDebugThickCircleOverTerrain(mpUser->getHoverPoint(), pProtoPower->getUIRadius(), thickness, valid ? cDWORDGreen : cDWORDRed, 0.1f);
*/

      // arrows
      BMatrix matrix;
      BVector right = mpUser->getCamera()->getCameraRight();
      BVector forward;
      forward.assignCrossProduct(right, cYAxisVector);
      forward.normalize();
      matrix.makeOrient(cYAxisVector, forward, right);
      static const float cArrowOffset = 5.0f;
      static const float cArrowAnimMagnitude = 0.6f;
      static const float cArrowAnimRate = 5.5f;
      float arrowOffset = (cArrowAnimMagnitude * sinf(cArrowAnimRate * (float)gWorld->getGametimeFloat())) + cArrowOffset;
      matrix.multTranslate(hoverPoint.x, hoverPoint.y + arrowOffset, hoverPoint.z);
      gpDebugPrimitives->addDebugArrow(matrix, BVector(2.0f, 3.0f, 2.0f), valid ? packRGBA(cValidColor, 1.0f) : cDWORDRed);
   }
   else
   {
/*
      // the dropoff visuals go here
      float thickness = 0.6f;
      if (pProtoPower->getUIRadius() > 0.0f)
         gTerrainSimRep.addDebugThickCircleOverTerrain(mpUser->getHoverPoint(), pProtoPower->getUIRadius(), thickness, valid ? cDWORDGreen : cDWORDRed, 0.1f);
*/

      // arrows
      BMatrix matrix;
      BVector right = -mpUser->getCamera()->getCameraRight();
      BVector forward;
      forward.assignCrossProduct(right, -cYAxisVector);
      forward.normalize();
      matrix.makeOrient(-cYAxisVector, forward, right);
      static const float cArrowOffset = 5.0f;
      static const float cArrowAnimMagnitude = 1.0f;
      static const float cArrowAnimRate = 9.0f;
      float arrowOffset = (cArrowAnimMagnitude * sinf(cArrowAnimRate * (float)gWorld->getGametimeFloat())) + cArrowOffset;
      matrix.multTranslate(hoverPoint.x, hoverPoint.y + arrowOffset, hoverPoint.z);
      gpDebugPrimitives->addDebugArrow(matrix, BVector(2.0f, 3.0f, 2.0f), valid ? packRGBA(cValidColor, 1.0f) : cDWORDRed);
   }

   if (valid)
      mpUser->renderReticle(BUIGame::cReticlePowerValid);
   else
      mpUser->renderReticle(BUIGame::cReticlePowerInvalid);
}

//==============================================================================
//==============================================================================
bool BPowerUserTransport::initDecals()
{
   const BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pProtoPower)
      return false;


   // create decals
   mBaseDecal = gDecalManager.createDecal();
   if(mBaseDecal == cInvalidDecalHandle)
      return false;
   mMoverDecal = gDecalManager.createDecal();
   if(mMoverDecal == cInvalidDecalHandle)
      return false;


   // "base" texture
   BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mBaseDecal);
   if (!pDecalAttributes)
      return false;
   BManagedTextureHandle textureBase;
   if(!pProtoPower->getDataTexture(mPowerLevel, "Base", textureBase) || textureBase == cInvalidManagedTextureHandle)
      return false;

   pDecalAttributes->setForward(BVec3(cXAxisVector.x, cXAxisVector.y, cXAxisVector.z));
   pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
   pDecalAttributes->setEnabled(true);
   pDecalAttributes->setIntensity(1.0f);
   pDecalAttributes->setRenderOneFrame(false);
   pDecalAttributes->setColor(packRGBA(cValidColor, 1.0f));
   pDecalAttributes->setSizeX(pProtoPower->getUIRadius());
   pDecalAttributes->setSizeZ(pProtoPower->getUIRadius());
   pDecalAttributes->setYOffset(0.0f);
   pDecalAttributes->setFlashMovieIndex(-1);
   pDecalAttributes->setTextureHandle(textureBase);
   pDecalAttributes->setConformToTerrain(true);
   pDecalAttributes->setPos(BVec3(mpUser->getHoverPoint().x, mpUser->getHoverPoint().y, mpUser->getHoverPoint().z));


   // "mover" texture
   pDecalAttributes = gDecalManager.getDecal(mMoverDecal);
   if (!pDecalAttributes)
      return false;
   BManagedTextureHandle textureMover;
   if(!pProtoPower->getDataTexture(mPowerLevel, "Mover", textureMover) || textureMover == cInvalidManagedTextureHandle)
      return false;

   pDecalAttributes->setForward(BVec3(cXAxisVector.x, cXAxisVector.y, cXAxisVector.z));
   pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
   pDecalAttributes->setEnabled(true);
   pDecalAttributes->setIntensity(1.0f);
   pDecalAttributes->setRenderOneFrame(false);
   pDecalAttributes->setColor(packRGBA(cValidColor, 1.0f));
   pDecalAttributes->setSizeX(pProtoPower->getUIRadius());
   pDecalAttributes->setSizeZ(pProtoPower->getUIRadius());
   pDecalAttributes->setYOffset(0.5f);
   pDecalAttributes->setFlashMovieIndex(-1);
   pDecalAttributes->setTextureHandle(textureMover);
   pDecalAttributes->setConformToTerrain(true);
   pDecalAttributes->setPos(BVec3(mpUser->getHoverPoint().x, mpUser->getHoverPoint().y, mpUser->getHoverPoint().z));


   // done
   return true;
}

//==============================================================================
//==============================================================================
void BPowerUserTransport::updateDecals(bool valid)
{
   BDecalAttribs* pDABase = gDecalManager.getDecal(mBaseDecal);
   BDecalAttribs* pDAMover = gDecalManager.getDecal(mMoverDecal);
   if (!pDAMover || !pDABase)
      return;

   if(!mGotPickupLocation)
   {
      // update "base" decal
      float rotationRate = valid ? 1.0f : 0.2f;
      BVec3 fwd = pDABase->getForward();
      BVector dir = BVector(fwd.getX(), fwd.getY(), fwd.getZ());
      dir.rotateXZ(rotationRate * gWorld->getLastSubUpdateLengthFloat());
      dir.normalize();
      pDABase->setForward(BVec3(dir.x, dir.y, dir.z));
      pDABase->setPos(BVec3(mpUser->getHoverPoint().x, mpUser->getHoverPoint().y, mpUser->getHoverPoint().z));
      BColor color(pDABase->getColor());
      BColor targetColor = valid ? cValidColor : cInvalidColor;
      static const float cLerpGain = 7.0f;
      color.lerp(color, targetColor, Math::Min(1.0f, gWorld->getLastSubUpdateLengthFloat() * cLerpGain));
      pDABase->setColor(packRGBA(color, 1.0f));

      // update "mover" decal
      pDAMover->setEnabled(false);
   }
   else
   {
      // update "mover" decal
      pDAMover->setEnabled(true);
      pDAMover->setPos(BVec3(mpUser->getHoverPoint().x, mpUser->getHoverPoint().y, mpUser->getHoverPoint().z));
      BColor color(pDAMover->getColor());
      BColor targetColor = valid ? cValidColor : cInvalidColor;
      static const float cLerpGain = 7.0f;
      color.lerp(color, targetColor, Math::Min(1.0f, gWorld->getLastSubUpdateLengthFloat() * cLerpGain));
      pDAMover->setColor(packRGBA(color, 1.0f));

      // update decal rotations so they aim at each other
      BVector basePos = BVector(pDABase->getPos().getX(), pDABase->getPos().getY(), pDABase->getPos().getZ());
      BVector moverPos = BVector(pDAMover->getPos().getX(), pDAMover->getPos().getY(), pDAMover->getPos().getZ());
      BVector fwd = moverPos - basePos;
      if(fwd.length() > cFloatCompareEpsilon)
      {
         fwd.rotateXZ(-cPiOver4);
         fwd.normalize();
         pDABase->setForward(BVec3(fwd.x, fwd.y, fwd.z));
         pDAMover->setForward(BVec3(fwd.x, fwd.y, fwd.z));
      }
      else
         pDAMover->setForward(pDABase->getForward());
   }
}

//==============================================================================
//==============================================================================
bool BPowerUserTransport::canUsePowerAtHoverPoint(bool draw)
{
   // return if we're trying to cast out of los
   if(!mpUser->getFlagHaveHoverPoint() || !mpUser->getFlagHoverPointOverTerrain())
      return false;

   BPlayer* pUserPlayer = mpUser->getPlayer();
   if (!pUserPlayer)
      return (false);

   // Player cannot use power.
   if (!pUserPlayer->canUsePower(mProtoPowerID))
      return (false);

   const BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
   if (!pProtoPower)
      return (false);

   BVector tempVec;
   DWORD flags = BWorld::cCPLOSCenterOnly;
   if(!gWorld->checkPlacement(-1,  mpUser->getPlayerID(), mpUser->getHoverPoint(), tempVec, cZAxisVector, mLOSMode, flags) ||
      !gWorld->getPowerManager() || !gWorld->getPowerManager()->isValidPowerCircle(mpUser->getHoverPoint(), pProtoPower->getUIRadius(), this))
      return false;

   // if we have a pickup location, we need to check placement, if we don't, we need to make sure there are squads to pickup 
   if (!mGotPickupLocation)
   {
      // get transportable squads
      BEntityIDArray outSquads;
      BPowerHelper::getTransportableSquads(mpUser->getPlayerID(), mpUser->getHoverPoint(), pProtoPower->getUIRadius(), outSquads);


      // limit and filter
      if(outSquads.getNumber() > 0)
      {
         // sort by distance
         BSimHelper::sortEntitiesBasedOnDistance(outSquads, mpUser->getHoverPoint(), outSquads);

         // limit by type
         BPowerHelper::BLimitDataArray mLimitData;
         BPowerHelper::BLimitData ld;
         int temp;
         if(pProtoPower->getDataInt(mPowerLevel, "MaxGroundVehicles", temp))
         {
            ld.mOTID = gDatabase.getOTIDGroundVehicle();
            ld.mLimit = (uint)temp;
            mLimitData.add(ld);
         }
         if(pProtoPower->getDataInt(mPowerLevel, "MaxInfantryUnits", temp))
         {
            ld.mOTID = gDatabase.getOTIDInfantry();
            ld.mLimit = (uint)temp;
            mLimitData.add(ld);
         }

         // filter
         outSquads = BPowerTransport::filterSquads(outSquads, mpUser->getHoverPoint(), mLimitData);
      }


      // update targeting visualization
      BEntityIDArray newTargetedSquads;
      BEntityIDArray noLongerTargetedSquads;
      BSimHelper::diffEntityIDArrays(outSquads, mTargetedSquads, &newTargetedSquads, &noLongerTargetedSquads, NULL);

      BSquad* pSquad = NULL;
      for(uint8 i = 0; i < newTargetedSquads.getNumber(); i++)
      {
         pSquad = gWorld->getSquad(newTargetedSquads[i]);
         if(!pSquad)
            continue;

         pSquad->setTargettingSelection(true);
      }

      for(uint8 i = 0; i < noLongerTargetedSquads.getNumber(); i++)
      {
         pSquad = gWorld->getSquad(noLongerTargetedSquads[i]);
         if(!pSquad)
            continue;

         pSquad->setTargettingSelection(false);
      }

      mTargetedSquads.empty();
      mTargetedSquads = outSquads;

      return (mTargetedSquads.getNumber() > 0);
   }
   else
   {
      float minDist = pProtoPower->getUIRadius();
      pProtoPower->getDataFloat(mPowerLevel, "MinTransportDistance", minDist);
      if((mPickupLocation - mpUser->getHoverPoint()).length() < minDist)
         return false;

      if (!BPowerHelper::checkSquadPlacement(*mpUser->getPlayer(), mpUser->getHoverPoint(), mSquadsToTransport))
         return false;
   }

   return true;
}

//==============================================================================
//==============================================================================
void BPowerUserTransport::onSquadsFoundToTransport(const BEntityIDArray& squadsToTransport)
{
   // if the sync code told us that we didn't find any squads, then go back to picking a pickup location again
   if (squadsToTransport.getSize() == 0)
   {
      if (mGotPickupLocation)
      {
         mGotPickupLocation = false;

         BUIContext* pUIContext = mpUser->getUIContext();
         if (pUIContext)
            pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(25033));
      }
   }
   else
   {
      mSquadsToTransport.clear();
      mSquadsToTransport.append(squadsToTransport);
   }
}

//==============================================================================
//==============================================================================
void BPowerUserTransport::setupUser()
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
      pUIContext->setPowerOverlayText(mProtoPowerID, gDatabase.getLocStringFromID(25033));
   }
}

//==============================================================================
//==============================================================================
bool BPowerUserTransport::postLoad(int saveType)
{
   initDecals();  
   if (mGotPickupLocation)
   {
      BDecalAttribs* pDABase = gDecalManager.getDecal(mBaseDecal);
      if (pDABase)
         pDABase->setPos(BVec3(mPickupLocation.x, mPickupLocation.y, mPickupLocation.z));
   }

   return BPowerUser::postLoad(saveType);
}

//==============================================================================
//==============================================================================
bool BPowerTransport::init(BPlayerID playerID, BPowerLevel powerLevel, BPowerUserID powerUserID, BEntityID ownerSquadID, BVector targetLocation, bool ignoreAllReqs /*= false*/)
{
   // Make sure all member variables are initialized.
   initBase();
   mPickupLocation = cOriginVector;
   mSquadsToTransport.clear();
   mGotPickupLocation = false;

#ifdef SYNC_Command
   syncCommandData("BPowerUserTransport::init playerID", playerID);
   syncCommandData("BPowerUserTransport::init powerLevel", (DWORD)powerLevel);
   syncCommandData("BPowerUserTransport::init powerUserID player", powerUserID.getPlayerID());
   syncCommandData("BPowerUserTransport::init powerUserID powerType", (DWORD)powerUserID.getPowerType());
   syncCommandData("BPowerUserTransport::init ownerSquadID", ownerSquadID.asLong());
   syncCommandData("BPowerUserTransport::init targetLocation", targetLocation);
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

   if (!pProtoPower)
   {
      shutdown();
      return false;
   }

   mFlagCheckPowerLocation = pProtoPower->getFlagAffectedByDisruption();

   bSuccess = (bSuccess && pPlayer != NULL);
#ifdef SYNC_Command
   syncCommandData("BPowerUserTransport::init pPlayer bSuccess", bSuccess);
#endif

   bSuccess = (bSuccess && pProtoPower != NULL);
#ifdef SYNC_Command
   syncCommandData("BPowerUserTransport::init pProtoPower bSuccess", bSuccess);
#endif

   if (!mFlagIgnoreAllReqs)
   {
      bSuccess = (bSuccess && pPlayer->canUsePower(mProtoPowerID));
#ifdef SYNC_Command
      syncCommandData("BPowerUserTransport::init canUserPower bSuccess", bSuccess);
#endif
   }

#ifdef SYNC_Command
   syncCommandData("BPowerUserTransport::init bSuccess", bSuccess);
#endif

   // Did we succeed on all our setup?
   if (!bSuccess)
   {
      BFAIL("Failed to load required data for PowerTransport");
      shutdown();
      return (false);
   }

   mElapsed = 0.0f;

   // All initialized.
   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerTransport::shutdown()
{
   // shutdown our power user if there is one
   BUser* pUser = gUserManager.getUserByPlayerID(mPowerUserID.getPlayerID());
   if (pUser)
   {
      BPowerUser* pPowerUser = pUser->getPowerUser();
      if (pPowerUser && pPowerUser->getID() == mPowerUserID)
         pPowerUser->shutdown();
   }

   mFlagDestroy = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BPowerTransport::submitInput(BPowerInput powerInput)
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
      if (!mGotPickupLocation)
      {
         BPowerHelper::getTransportableSquads(mPlayerID, powerInput.mVector, pProtoPower->getUIRadius(), mSquadsToTransport);

         // sort by distance
         BSimHelper::sortEntitiesBasedOnDistance(mSquadsToTransport, powerInput.mVector, mSquadsToTransport);

         // limit by type
         BPowerHelper::BLimitDataArray mLimitData;
         BPowerHelper::BLimitData ld;
         int temp;
         if(pProtoPower->getDataInt(mPowerLevel, "MaxGroundVehicles", temp))
         {
            ld.mOTID = gDatabase.getOTIDGroundVehicle();
            ld.mLimit = (uint)temp;
            mLimitData.add(ld);
         }
         if(pProtoPower->getDataInt(mPowerLevel, "MaxInfantryUnits", temp))
         {
            ld.mOTID = gDatabase.getOTIDInfantry();
            ld.mLimit = (uint)temp;
            mLimitData.add(ld);
         }
         mSquadsToTransport = BPowerTransport::filterSquads(mSquadsToTransport, powerInput.mVector, mLimitData);

         // do stuff
         BPowerUserTransport* pPowerUser = getPowerUser();
         if (pPowerUser)
            pPowerUser->onSquadsFoundToTransport(mSquadsToTransport);

         // if we didn't get any squads, don't proceed
         if (mSquadsToTransport.getSize() != 0)
         {
            mGotPickupLocation = true;
            mPickupLocation = powerInput.mVector;
         }
      }
      else
      {
         pPlayer->usePower(mProtoPowerID, cInvalidObjectID, cInvalidObjectID, 1.0f, mFlagIgnoreAllReqs);

         // actually do the transport
         BSimOrder* pOrder = gSimOrderManager.createOrder();
         if (!pOrder)  
            return false;
         pOrder->setPriority(BSimOrder::cPriorityUser);

         BPowerHelper::checkSquadPlacement(*pPlayer, powerInput.mVector, mSquadsToTransport);

         BSquadActionTransport::transportSquads(pOrder, mSquadsToTransport, powerInput.mVector, mPlayerID, gWorld->getPlayer(mPlayerID)->getCiv()->getTransportProtoID());

         shutdown();
      }

      return true;
   }
   else if (powerInput.mType == PowerInputType::cUserCancel)
   {
      if (!mGotPickupLocation)
      {
         shutdown();
      }
      else 
      {
         mGotPickupLocation = false;
         mSquadsToTransport.clear();
      }

      return true;
   }

   // Do we want to gracefully ignore bad inputs?  I'm going to say no for now, we shouldn't be getting any.
   BFAIL("BPowerTransport::submitInput() received powerInput of unsupported type.");
   return (false);
}


//==============================================================================
//==============================================================================
void BPowerTransport::update(DWORD currentGameTime, float lastUpdateLength)
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
}

//==============================================================================
//==============================================================================
BPowerUserTransport* BPowerTransport::getPowerUser() const
{
   return static_cast<BPowerUserTransport*>(BPowerHelper::getPowerUser(*this));
}

//==============================================================================
//==============================================================================
bool BPowerUserTransport::save(BStream* pStream, int saveType) const
{
   if (!BPowerUser::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mHudSounds);
   GFWRITEARRAY(pStream, BEntityID, mSquadsToTransport, uint8, 200);
   GFWRITEARRAY(pStream, BEntityID, mTargetedSquads, uint8, 200);
   GFWRITEVAR(pStream, long, mLOSMode);
   GFWRITEBITBOOL(pStream, mGotPickupLocation);
   GFWRITEVECTOR(pStream, mPickupLocation);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerUserTransport::load(BStream* pStream, int saveType)
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
   GFREADARRAY(pStream, BEntityID, mSquadsToTransport, uint8, 200);
   GFREADARRAY(pStream, BEntityID, mTargetedSquads, uint8, 200);
   GFREADVAR(pStream, long, mLOSMode);
   GFREADBITBOOL(pStream, mGotPickupLocation);
   GFREADVECTOR(pStream, mPickupLocation);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerTransport::save(BStream* pStream, int saveType) const
{
   if (!BPower::save(pStream, saveType))
      return false;
   GFWRITEVECTOR(pStream, mPickupLocation);
   GFWRITEARRAY(pStream, BEntityID, mSquadsToTransport, uint8, 50);
   GFWRITEBITBOOL(pStream, mGotPickupLocation);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerTransport::load(BStream* pStream, int saveType)
{
   if (!BPower::load(pStream, saveType))
      return false;
   GFREADVECTOR(pStream, mPickupLocation);
   GFREADARRAY(pStream, BEntityID, mSquadsToTransport, uint8, 50);
   GFREADBITBOOL(pStream, mGotPickupLocation);
   return true;
}

//==============================================================================
//==============================================================================
BEntityIDArray BPowerTransport::filterSquads(BEntityIDArray squads, BVector refVector, BPowerHelper::BLimitDataArray& limitDataArray)
{
   // filter squads based on power level limitations (e.g., only 1 vehicle and 4 infantry squads allowed)
   BSquad* pSquad = NULL;
   BEntityIDArray filteredSquads;
   filteredSquads.clear();

   BDynamicSimUIntArray currentCounts;
   for(long i = 0; i < limitDataArray.getNumber(); i++)
      currentCounts.add(0);

   for(long i = 0; i < squads.getNumber(); i++)
   {
      pSquad = gWorld->getSquad(squads[i]);
      if(!pSquad)
         continue;

      for(long j = 0; j < limitDataArray.getNumber(); j++)
      {
         if(pSquad->getProtoObject()->isType(limitDataArray[j].mOTID))
         {
            if(currentCounts[j] >= limitDataArray[j].mLimit)
               break;

            filteredSquads.add(squads[i]);
            currentCounts[j]++;
            break;
         }
      }
   }

   return filteredSquads;
}
