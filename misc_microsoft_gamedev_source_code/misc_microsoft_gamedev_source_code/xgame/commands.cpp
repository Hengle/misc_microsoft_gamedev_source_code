//==============================================================================
// commands.cpp
//
// Copyright (c) 1999-2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "commands.h"

// xgame
#include "ai.h"
#include "alert.h"
#include "army.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "EntityGrouper.h"
#include "game.h"
#include "gamedirectories.h"
#include "generaleventmanager.h"
#include "kb.h"
#include "objectmanager.h"
#include "player.h"
#include "protoobject.h"
#include "protopower.h"
#include "recordgame.h"
#include "squad.h"
#include "techtree.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "world.h"
#include "usermanager.h"
#include "user.h"
#include "minimap.h"
#include "syncmacros.h"
#include "team.h"
#include "ability.h"
#include "uimanager.h"
#include "selectionmanager.h"
#include "visiblemap.h"
#include "unitactionmoveair.h"
#include "SimOrderManager.h"
#include "squadactioncarpetbomb.h"
#include "squadactiontransport.h"
#include "particlegateway.h"
#include "powermanager.h"
#include "power.h"
#include "powertypes.h"
#include "skullmanager.h"
#include "fatalitymanager.h"

// Constants
const float cDefaultDistanceCheck = 1.0f;

//==============================================================================
// BWorkCommand::BWorkCommand
//==============================================================================
BWorkCommand::BWorkCommand(void) :
   BCommand(cCommandWork, cNumberWorkCommandFlags),
   mUnitID(cInvalidObjectID),
   mHitZoneIndex( -1 ),
   mBuildProtoID(-1),
   mRange(0.0f),
   mTerrainPoint(cInvalidVector),
   mSpeedMultiplier(1.0f),
   mSquadMode(-1),
   mAbilityID(-1),
   mAngle(0.0f),
   mFlagOverridePosition(false),
   mFlagOverrideRange(false)
{
   setUrgencyTimeThreshhold(1000);
}

//==============================================================================
// BWorkCommand::~BWorkCommand
//==============================================================================
BWorkCommand::~BWorkCommand(void)
{
}

//==============================================================================
// BWorkCommand::serialize
//==============================================================================
enum
{
   cWrkCmdUnit      = 1,
   cWrkCmdProto     = 2,
   cWrkCmdRange     = 4,
   cWrkCmdAngle    = 8,
   cWrkCmdTerrPt    = 16,
   cWrkCmdSpeed     = 32,
   cWrkCmdSquadMode = 64,
   cWrkCmdAbility   = 128,
   cWrkCmdHitZone   = 256,
   cWrkCmdOverridePosition = 512,
   cWrkCmdOverrideRange = 1024,
};

void BWorkCommand::serialize(BSerialBuffer &sb)
{
   // Set flags that determine how to serialize the data to keep it small.
   WORD flags = 0;

   if(mUnitID!=cInvalidObjectID)
      flags+=cWrkCmdUnit;

   if(mBuildProtoID!=-1)
      flags+=cWrkCmdProto;

   if(mRange!=0.0f)
      flags+=cWrkCmdRange;

   if(mTerrainPoint!=cInvalidVector)
      flags+=cWrkCmdTerrPt;

   if(mSpeedMultiplier!=1.0f)
      flags+=cWrkCmdSpeed;

   if(mSquadMode!=-1)
      flags+=cWrkCmdSquadMode;

   if(mAbilityID!=-1)
      flags+=cWrkCmdAbility;

   if(mAngle!=0.0f)
      flags+=cWrkCmdAngle;

   if( mHitZoneIndex != -1 )
   {
      flags += cWrkCmdHitZone;
   }

   if (mFlagOverridePosition)
   {
      flags += cWrkCmdOverridePosition;
   }

   if (mFlagOverrideRange)
   {
      flags += cWrkCmdOverrideRange;
   }

   // Serialize the data.
   BCommand::serialize(sb);

   sb.add(flags);

   if(flags&cWrkCmdUnit)
      sb.add(mUnitID.asLong());

   if(flags&cWrkCmdProto)
      sb.add(mBuildProtoID);

   if(flags&cWrkCmdRange)
      sb.add(mRange);

   if(flags&cWrkCmdTerrPt)
   {
      sb.add(mTerrainPoint.x);
      sb.add(mTerrainPoint.y);
      sb.add(mTerrainPoint.z);
   }

   if(flags&cWrkCmdSpeed)
      sb.add(mSpeedMultiplier);

   if(flags&cWrkCmdSquadMode)
      sb.add((BYTE)mSquadMode);

   if(flags&cWrkCmdAbility)
      sb.add((BYTE)mAbilityID);

   if(flags&cWrkCmdAngle)
      sb.add(mAngle);

   if( flags & cWrkCmdHitZone )
   {
      sb.add( (BYTE)mHitZoneIndex );
   }
}

//==============================================================================
// BWorkCommand::deserialize
//==============================================================================
void BWorkCommand::deserialize(BSerialBuffer &sb)
{
   BCommand::deserialize(sb);

   WORD flags=0;
   sb.get(&flags);

   if(flags&cWrkCmdUnit)
   {
      long temp;
      sb.get(&temp);
      mUnitID = BEntityID(temp);
   }
   else
      mUnitID=cInvalidObjectID;

   if(flags&cWrkCmdProto)
      sb.get(&mBuildProtoID);
   else
      mBuildProtoID=-1;

   if(flags&cWrkCmdRange)
      sb.get(&mRange);
   else
      mRange=0.0f;
                         
   if (gRecordGame.isPlaying() && gRecordGame.getVersion()<8)
   {
      if (flags & 8) // cWrkCmdFacing
      {
         BVector facing;
         sb.get(&facing.x);
         sb.get(&facing.y);
         sb.get(&facing.z);
      }
   }

   if(flags&cWrkCmdTerrPt)
   {
      sb.get(&mTerrainPoint.x);
      sb.get(&mTerrainPoint.y);
      sb.get(&mTerrainPoint.z);
   }
   else
      mTerrainPoint=cInvalidVector;

   if(flags&cWrkCmdSpeed)
      sb.get(&mSpeedMultiplier);
   else
      mSpeedMultiplier=1.0f;

   if(flags&cWrkCmdSquadMode)
   {
      BYTE val=0;
      sb.get(&val);
      mSquadMode=(long)val;
   }

   if(flags&cWrkCmdAbility)
   {
      BYTE val=0;
      sb.get(&val);
      mAbilityID=(long)val;
   }

   if(flags&cWrkCmdAngle)
      sb.get(&mAngle);
   else
      mAngle=0.0f;

   if( flags & cWrkCmdHitZone )
   {
      BYTE val = 0;
      sb.get( &val );
      mHitZoneIndex = (long)val;
   }
   else
   {
      mHitZoneIndex = -1;
   }

   if (flags & cWrkCmdOverridePosition)
   {
      mFlagOverridePosition = true;
   }

   if (flags & cWrkCmdOverrideRange)
   {
      mFlagOverrideRange = true;
   }
}

//==============================================================================
// BWorkCommand::sync
//==============================================================================
void BWorkCommand::sync()
{
   #ifdef SYNC_Command   
      BCommand::sync();
      syncCommandData("mUnitID", mUnitID.asLong());
      syncCommandData("mRange", mRange);
      syncCommandData("mTerrainPoint", mTerrainPoint);
      syncCommandData("mSpeedMultiplier", mSpeedMultiplier);
   #endif
}

//==============================================================================
// BWorkCommand::meter
//==============================================================================
bool BWorkCommand::meter(BCommand*  pLastCommand)
{
   if (gConfig.isDefined(cConfigDontMeterWorkCommands))
   {
      //gConsole.output(cChannelSim, "  BWC::meter: FALSE1");
      return false;
   }

   if (!pLastCommand)
   {
      //gConsole.output(cChannelSim, "  BWC::meter: FALSE2");
      return false;
   }

   if (pLastCommand->getType() == cCommandWork)
   {
      BWorkCommand *c = (BWorkCommand *)pLastCommand;
      if (c->getUnitID() != mUnitID || c->getAbilityID() != mAbilityID || c->getSquadMode() != mSquadMode || mSquadMode != -1)
      {
         //gConsole.output(cChannelSim, "  BWC::meter: FALSE3");
         return false;
      }
   }

   if (!BCommand::meterRecipientsAndWaypoints(pLastCommand, cDefaultDistanceCheck))
   {
      //gConsole.output(cChannelSim, "  BWC::meter: FALSE3");
      return false;
   }

   return BCommand::meter(pLastCommand);
}

//==============================================================================
// BWorkCommand::processArmy
//==============================================================================
bool BWorkCommand::processArmy(BArmy* pArmy)
{
   if (pArmy == NULL)
      return(false); 

   if (mBuildProtoID!=-1)
   {
      BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
      if(!pPlayer)
         return(false);

//-- FIXING PREFIX BUG ID 2375
      const BProtoObject* pProtoBuilding=pPlayer->getProtoObject(mBuildProtoID);
//--
      if(!pProtoBuilding || !pPlayer->checkCost(pProtoBuilding->getCost()) || mWaypoints.getNumber()<1)
         return(false);

      mUnitID=gWorld->createEntity(mBuildProtoID, false, mPlayerID, mWaypoints[0], cZAxisVector, cXAxisVector, false);
      if(mUnitID==cInvalidObjectID)
         return(false);
   }

//-- FIXING PREFIX BUG ID 2376
   const BEntity* pTargetEntity=gWorld->getObjectManager()->getEntity(getTargetID());
//--
   if (!pTargetEntity && (getNumberWaypoints() <= 0))
      return(false);

   // Validate target location
   if (pTargetEntity)
   {
      if (!isValidTerrainPosition(pTargetEntity->getPosition()))
         return false;
   }
   else if (getNumberWaypoints() > 0)
   {
      if (!isValidTerrainPosition(getWaypoint(getNumberWaypoints() - 1)))
         return false;
   }
   
   // MS 10/14/2008: nuke all of our squads from any transport actions
   for(uint i = 0; i < pArmy->getNumberChildren(); i++)
   {
      BPlatoon* pPlatoon = gWorld->getPlatoon(pArmy->getChild(i));
      if(!pPlatoon)
         continue;

      for(uint j = 0; j < pPlatoon->getNumberChildren(); j++)
      {
         BSquad* pSquad = gWorld->getSquad(pPlatoon->getChild(j));
         if(!pSquad || !pSquad->getFlagIsTransporting())
            continue;

         for(uint k = 0; k < BSquadActionTransport::mFreeList.getSize(); k++)
         {
            if(!BSquadActionTransport::mFreeList.isInUse(k))
               continue;

            BSquadActionTransport* pSAT = BSquadActionTransport::mFreeList.get(k);
            if(pSAT)
               pSAT->notifySquadWasCommanded(pSquad->getID());
         }
      }
   }


   pArmy->queueOrder(this);
   return(true);
}


//==============================================================================
// BPowerCommand::BPowerCommand
//==============================================================================
BPowerCommand::BPowerCommand(void) :
   BCommand(cCommandPower, cNumberPowerCommandFlags),
   mType(BPowerCommand::cTypeUndefined),
   mNumUses(0),
   mProtoPowerID(-1),
   mAbilityID(-1),
   mSquadID(cInvalidObjectID),
   mPowerUserID(cInvalidPowerUserID),
   mTargetLocation(cInvalidVector)
{
   mAbilitySquads.clear();
   mPowerUnits.clear();
   mTargetLocations.clear();
} 

//==============================================================================
// BPowerCommand::serialize
//==============================================================================
void BPowerCommand::serialize(BSerialBuffer &sb)
{
   BCommand::serialize(sb);

   sb.add(mType);
   sb.add(mNumUses);
   sb.add(mProtoPowerID);
   sb.add(mPowerLevel);
   sb.add(mAbilityID);
   sb.add(mSquadID.asLong());
   uint32 powerUserID = mPowerUserID;
   sb.add(powerUserID);
   uint numSquads = mAbilitySquads.getSize();
   sb.add(numSquads);
   for (uint i = 0; i < numSquads; i++)
   {
      sb.add(mAbilitySquads[i].asLong());      
   }
   uint numLocs = mTargetLocations.getSize();
   sb.add(numLocs);
   for (uint i = 0; i < numLocs; i++)
   {
      sb.add(mTargetLocations[i].x);
      sb.add(mTargetLocations[i].y);
      sb.add(mTargetLocations[i].z);
      sb.add(mTargetLocations[i].w);
   }
   sb.add(mTargetLocation.x);
   sb.add(mTargetLocation.y);
   sb.add(mTargetLocation.z);
   sb.add(mTargetLocation.w);
   uint numUnits = mPowerUnits.getSize();
   sb.add(numUnits);
   for (uint i = 0; i < numUnits; i++)
   {
      sb.add(mPowerUnits[i].asLong());
   }
}

//==============================================================================
// BPowerCommand::deserialize
//==============================================================================
void BPowerCommand::deserialize(BSerialBuffer &sb)
{
   BCommand::deserialize(sb);

   sb.get(&mType);
   sb.get(&mNumUses);
   sb.get(&mProtoPowerID);
   sb.get(&mPowerLevel);
   sb.get(&mAbilityID);
   if (gRecordGame.isPlaying() && gRecordGame.getVersion()<11)
      mSquadID=cInvalidObjectID;
   else
   {
      int temp;
      sb.get(&temp);
      mSquadID.set(temp);
   }

   uint32 powerUserID;
   sb.get(&powerUserID);
   mPowerUserID = powerUserID;

   uint numSquads = 0;
   sb.get(&numSquads);
   mAbilitySquads.clear();
   for (uint i = 0; i < numSquads; i++)
   {
      long entityID;
      sb.get(&entityID);
      mAbilitySquads.add(BEntityID(entityID));
   }
   uint numLocs = 0;
   sb.get(&numLocs);
   mTargetLocations.clear();
   for (uint i = 0; i < numLocs; i++)
   {
      BVector loc;
      sb.get(&loc.x);
      sb.get(&loc.y);
      sb.get(&loc.z);
      sb.get(&loc.w);
      mTargetLocations.add(loc);
   }
   sb.get(&mTargetLocation.x);
   sb.get(&mTargetLocation.y);
   sb.get(&mTargetLocation.z);
   sb.get(&mTargetLocation.w);
   uint numUnits = 0;
   sb.get(&numUnits);
   mPowerUnits.clear();
   for (uint i = 0; i < numUnits; i++)
   {
      long entityID;
      sb.get(&entityID);
      mPowerUnits.add(BEntityID(entityID));
   }
} 

//==============================================================================
// BPowerCommand::processPlayer
//==============================================================================
bool BPowerCommand::processPlayer(void)
{
   #ifdef SYNC_Command
      syncCommandData("BPowerCommand::processPlayer mType", mType);
   #endif

   if (mType == BPowerCommand::cTypeGrantPower)
   {
      if (mProtoPowerID < 0)
         return (false);
//-- FIXING PREFIX BUG ID 2377
      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
//--
      if (!pProtoPower)
         return (false);
      BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
      if (!pPlayer)
         return (false);

      pPlayer->addPowerEntry(mProtoPowerID, cInvalidObjectID, mNumUses, -1);

      return (true);
   }
   else if (mType == BPowerCommand::cTypeInvokePower)
   {
      if (mProtoPowerID < 0)
         return (false);
//-- FIXING PREFIX BUG ID 2380
      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
//--
      if (!pProtoPower)
         return (false);
      BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
      if (!pPlayer)
         return (false);
      // Last chance to invalidate this casting.
      if (!getFlag(BPowerCommand::cNoCost) && !pPlayer->canUsePower(mProtoPowerID))
         return (false);

      if (pProtoPower->getFlagUnitPower())
      {
         // Make sure a squad is available for this power.
         BEntityID squadID = cInvalidObjectID;
         BPowerEntry* pPowerEntry = pPlayer->findPowerEntry(mProtoPowerID);
         if (!pPowerEntry)
            return (false);
         uint numItems = pPowerEntry->mItems.getSize();
         for (uint i = 0; i < numItems; i++)
         {
//-- FIXING PREFIX BUG ID 2378
            const BPowerEntryItem& item = pPowerEntry->mItems[i];
//--
            if ((item.mSquadID != cInvalidObjectID) && (item.mUsesRemaining > 0))
            {
               squadID = item.mSquadID;
               break;
            }
         }
         if (squadID == cInvalidObjectID)
            return (false);
      }
      else if (pProtoPower->getFlagMultiRechargePower())
      {
         // Make sure a power entry item is available for this power.
         bool itemAvail = false;
         BPowerEntry* pPowerEntry = pPlayer->findPowerEntry(mProtoPowerID);
         if (!pPowerEntry)
            return (false);
         uint numItems = pPowerEntry->mItems.getSize();
         for (uint i = 0; i < numItems; i++)
         {
//-- FIXING PREFIX BUG ID 2379
            const BPowerEntryItem& item = pPowerEntry->mItems[i];
//--
            if (item.mUsesRemaining > 0)
            {
               itemAvail = true;
               break;
            }
         }
         if (!itemAvail)
            return (false);
      }

      // Clone the trigger script and activate it.
      const BSimString& powerTriggerScript = pProtoPower->getTriggerScript();
      BTriggerScriptID newTriggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, powerTriggerScript);
      if (newTriggerScriptID == cInvalidTriggerScriptID)
      {
         BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
         if ((pUser->getUserMode()==BUser::cUserModeInputUILocation || pUser->getUserMode()==BUser::cUserModeInputUILocationMinigame) && 
            pUser->getUIProtoPowerID() == mProtoPowerID && pUser->getPlayerID() == mPlayerID)
         {
            pUser->changeMode(BUser::cUserModeNormal);
         }
         return (false);
      }
      gTriggerManager.addExternalPlayerID(newTriggerScriptID, mPlayerID);
      gTriggerManager.addExternalProtoPowerID(newTriggerScriptID, mProtoPowerID);
      if (mTargetLocation != cInvalidVector)
      {
         gTriggerManager.addExternalLocation(newTriggerScriptID, mTargetLocation);
      }
      if (mTargetLocations.getSize() > 0)
      {
         gTriggerManager.addExternalLocationList(newTriggerScriptID, mTargetLocations);
      }
      if (mAbilitySquads.getSize() > 0)
         gTriggerManager.addExternalSquadIDs(newTriggerScriptID, mAbilitySquads);
      gTriggerManager.addExternalSquadID(newTriggerScriptID, mSquadID);
      if (mPowerUnits.getSize() > 0)
      {
         gTriggerManager.addExternalUnitIDs(newTriggerScriptID, mPowerUnits);
      }
      gTriggerManager.addExternalFlag(newTriggerScriptID, 0, getFlag(BPowerCommand::cGeneric0));
      gTriggerManager.addExternalFlag(newTriggerScriptID, 1, getFlag(BPowerCommand::cGeneric1));
      gTriggerManager.activateTriggerScript(newTriggerScriptID);

      return (true);
   }
   else if(mType == BPowerCommand::cTypeInvokeAbility)
   {
      BASSERTM(0, "Trigger script abilities aren't supported anymore.  This shouldn't have happened.");
      /*
//-- FIXING PREFIX BUG ID 2381
      const BAbility* pAbility = gDatabase.getAbilityFromID(mAbilityID);
//--
      if (!pAbility)
      {
         BASSERTM(false, "Valid ability not found!");
         return (false);
      }

      // Clone the trigger script and activate it.
      const BSimString& abilityTriggerScript = pAbility->getTriggerScript();
      BTriggerScriptID newTriggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, abilityTriggerScript);
      gTriggerManager.addExternalPlayerID(newTriggerScriptID, mPlayerID);
      gTriggerManager.addExternalSquadID(newTriggerScriptID, mSquadID);
      if (mAbilitySquads.getSize() > 0)
         gTriggerManager.addExternalSquadIDs(newTriggerScriptID, mAbilitySquads);
      if (mTargetLocation != cInvalidVector)
      {
         gTriggerManager.addExternalLocation(newTriggerScriptID, mTargetLocation);
      }
      gTriggerManager.activateTriggerScript(newTriggerScriptID);
      */
      return (true);
   }
   else if (mType == BPowerCommand::cTypeInvokePower2)
   {
      #ifdef SYNC_Command
         syncCommandData("BPowerCommand::processPlayer mProtoPowerID", mProtoPowerID);
         syncCommandData("BPowerCommand::processPlayer mPlayerID", mPlayerID);
         syncCommandData("BPowerCommand::processPlayer power type", (DWORD)mPowerUserID.getPowerType());
      #endif

      // this is just bad...
      if (!gWorld->getPowerManager())
      {
         #ifdef SYNC_Command
            syncCommandCode("BPowerCommand::processPlayer !gWorld->getPowerManager()");
         #endif
         return (false);
      }

//-- FIXING PREFIX BUG ID 2382
      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mProtoPowerID);
//--
      if (!pProtoPower)
      {
         #ifdef SYNC_Command
            syncCommandCode("BPowerCommand::processPlayer !pProtoPower");
         #endif
         gWorld->getPowerManager()->cancelPowerUser(mPowerUserID);
         return (false);
      }
      BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
      if (!pPlayer)
      {
         #ifdef SYNC_Command
            syncCommandCode("BPowerCommand::processPlayer !pPlayer");
         #endif
         gWorld->getPowerManager()->cancelPowerUser(mPowerUserID);
         return (false);
      }
      // Last chance to invalidate this casting.
      if (!getFlag(BPowerCommand::cNoCost) && !pPlayer->canUsePower(mProtoPowerID))
      {
         #ifdef SYNC_Command
            syncCommandCode("BPowerCommand::processPlayer !canUsePower");
         #endif
         gWorld->getPowerManager()->cancelPowerUser(mPowerUserID);
         return (false);
      }
      BASSERT(pProtoPower->getPowerType() == mPowerUserID.getPowerType());
      if (pProtoPower->getPowerType() != mPowerUserID.getPowerType())
      {
         #ifdef SYNC_Command
            syncCommandCode("BPowerCommand::processPlayer wrong power type");
         #endif
         gWorld->getPowerManager()->cancelPowerUser(mPowerUserID);
         return (false);
      }
      BASSERT(mPlayerID == mPowerUserID.getPlayerID());
      if (mPlayerID != mPowerUserID.getPlayerID())
      {
         #ifdef SYNC_Command
            syncCommandCode("BPowerCommand::processPlayer wrong mPlayerID");
         #endif
         gWorld->getPowerManager()->cancelPowerUser(mPowerUserID);
         return (false);
      }

      // We can invoke the power I guess here.
      BPowerType powerType = mPowerUserID.getPowerType();
      BASSERTM(powerType != PowerType::cInvalid, "Trying to invoke a power of type invalid!");
      if (powerType != PowerType::cInvalid)
      {
         BPower* pNewPower = gWorld->getPowerManager()->createNewPower(mProtoPowerID);
         if (pNewPower)
         {
            pNewPower->init(mPlayerID, mPowerLevel, mPowerUserID, mSquadID, mTargetLocation, getFlag(BPowerCommand::cNoCost));
         }
         else
         {
            #ifdef SYNC_Command
               syncCommandCode("BPowerCommand::processPlayer couldn't create power");
            #endif
         }
      }
      else
      {
         #ifdef SYNC_Command
            syncCommandCode("BPowerCommand::processPlayer invalid powerType");
         #endif
      }

//      gConsoleOutput.status("%.2f: BPowerCommand::processPlayer invoking power, mPowerUserID %d", gWorld->getGametimeFloat(), mPowerUserID);

      return (true);
   }

   // Uh oh.
   return(false);
}

//==============================================================================
// BPowerInputCommand::BPowerInputCommand
//==============================================================================
BPowerInputCommand::BPowerInputCommand(void) :
BCommand(cCommandPowerInput, cNumberPowerInputCommandFlags),
mType(BPowerInputCommand::cTypeUndefined),
mVector(cInvalidVector)
{
} 

//==============================================================================
// BPowerInputCommand::serialize
//==============================================================================
void BPowerInputCommand::serialize(BSerialBuffer &sb)
{
   BCommand::serialize(sb);

   sb.add(mType);
   sb.add(mVector.x);
   sb.add(mVector.y);
   sb.add(mVector.z);
   sb.add(mVector.w);

   // power user ID
   sb.add(mPowerUserID.getPlayerID());
   sb.add(mPowerUserID.getPowerType());
   sb.add(mPowerUserID.getRefCount());
}

//==============================================================================
// BPowerInputCommand::deserialize
//==============================================================================
void BPowerInputCommand::deserialize(BSerialBuffer &sb)
{
   BCommand::deserialize(sb);

   sb.get(&mType);
   sb.get(&mVector.x);
   sb.get(&mVector.y);
   sb.get(&mVector.z);
   sb.get(&mVector.w);
   
   // power user ID
   BPlayerID playerID;
   BPowerType powerType;
   uint refCount;
   sb.get(&playerID);
   sb.get(&powerType);
   sb.get(&refCount);
   mPowerUserID.set(playerID, powerType, refCount);
} 

//==============================================================================
// BPowerInputCommand::processPlayer
//==============================================================================
bool BPowerInputCommand::processPlayer(void)
{
#ifdef SYNC_Command   
   syncCommandData("mPowerUserID playerID", (long)mPowerUserID.getPlayerID());
   syncCommandData("mPowerUserID powerType", (long)mPowerUserID.getPowerType());
   syncCommandData("mPowerUserID refCount", (long)mPowerUserID.getRefCount());
#endif

   if(!gWorld->getPowerManager())
      return false;

//   gConsoleOutput.status("%.2f: BPowerInputCommand::processPlayer, mPowerUserID %d, mType %d", gWorld->getGametimeFloat(), mPowerUserID, mType);

   BPower* pPower = gWorld->getPowerManager()->getPowerByUserID(mPowerUserID);
   if(!pPower)
   {
//      gConsoleOutput.status("%.2f:   BPowerInputCommand::processPlayer, could not find power", gWorld->getGametimeFloat());
      return false;
   }

   switch(mType)
   {
   case BPowerInputCommand::cTypeConfirm:
      {
         BPowerInput pi;
         pi.mType = PowerInputType::cUserOK;
         pi.mVector = mVector;
         pPower->submitInput(pi);
         break;
      }
      case BPowerInputCommand::cTypePosition:
      {
         BPowerInput pi;
         pi.mType = PowerInputType::cPosition;
         pi.mVector = mVector;
         pPower->submitInput(pi);
         break;
      }
      case BPowerInputCommand::cTypeDirection:
      {
         BPowerInput pi;
         pi.mType = PowerInputType::cDirection;
         pi.mVector = mVector;
         pPower->submitInput(pi);
         break;
      }
      case BPowerInputCommand::cTypeShutdown:
      {
         BPowerInput pi;
         pi.mType = PowerInputType::cUserCancel;
         pPower->submitInput(pi);
         break;
      }
      default:
      {
         BFAIL("Invalid power input command type.");
         return false;
      }
   }

   // Uh oh.
   return(false);
}

//==============================================================================
// BTriggerCommand::BTriggerCommand
//==============================================================================
BTriggerCommand::BTriggerCommand(void) :
BCommand(cCommandTrigger, cNumberTriggerCommandFlags),
mType(BTriggerCommand::cTypeUndefined),
mTriggerScriptID(cInvalidTriggerScriptID),
mTriggerID(cInvalidTriggerID),
mConditionID(-1),
mTriggerVarID(BTriggerVar::cVarIDInvalid),
mInputResult(-1),
mInputActionModifier(false),
mInputSpeedModifier(false),
mInputQuality(0.0f),
mAsyncCondition(false)
{
   mTriggerScript = "";
   mInputLocation.zero();
   mInputEntity.invalidate();
   mInputUnit.invalidate();
   mInputSquad.invalidate();
   mInputSquadList.clear();
} 

//==============================================================================
// BTriggerCommand::serialize
//==============================================================================
void BTriggerCommand::serialize(BSerialBuffer &sb)
{
   BCommand::serialize(sb);
   sb.add(mType);
   sb.add(mTriggerScriptID);
   sb.add(mTriggerScript);
   sb.add(mTriggerID);
   sb.add(mConditionID);
   sb.add(mTriggerVarID);
   sb.add(mInputResult);
   sb.add(mInputLocation.x);
   sb.add(mInputLocation.y);
   sb.add(mInputLocation.z);
   sb.add(mInputQuality);
   sb.add(mInputEntity.asLong());
   sb.add(mInputUnit.asLong());
   sb.add(mInputSquad.asLong());
   sb.add(mInputActionModifier);
   sb.add(mInputSpeedModifier);
   sb.add(mAsyncCondition);
   uint numSquads = mInputSquadList.getSize();
   sb.add(numSquads);
   for (uint i = 0; i < numSquads; i++)
   {
      sb.add(mInputSquadList[i].asLong());
   }
}

//==============================================================================
// BTriggerCommand::deserialize
//==============================================================================
void BTriggerCommand::deserialize(BSerialBuffer &sb)
{
   BCommand::deserialize(sb);
   sb.get(&mType);
   sb.get(&mTriggerScriptID);
   sb.get(&mTriggerScript);
   sb.get(&mTriggerID);
   sb.get(&mConditionID);
   sb.get(&mTriggerVarID);
   sb.get(&mInputResult);
   sb.get(&mInputLocation.x);
   sb.get(&mInputLocation.y);
   sb.get(&mInputLocation.z);
   sb.get(&mInputQuality);
   long entityID;
   sb.get(&entityID);
   mInputEntity = BEntityID(entityID);
   sb.get(&entityID);
   mInputUnit = BEntityID(entityID);
   sb.get(&entityID);
   mInputSquad = BEntityID(entityID);
   sb.get(&mInputActionModifier);
   sb.get(&mInputSpeedModifier);
   sb.get(&mAsyncCondition);
   uint numSquads = 0;
   sb.get(&numSquads);
   for (uint i = 0; i < numSquads; i++)
   {
      long entityID = cInvalidObjectID;
      sb.get(&entityID);
      mInputSquadList.add(BEntityID(entityID));
   }
} 

//==============================================================================
// BTriggerCommand::processPlayer
//==============================================================================
bool BTriggerCommand::processPlayer(void)
{
   if (mType == BTriggerCommand::cTypeBroadcastAsyncCondition)
   {
      gTriggerManager.setAsyncTriggerConditionState(mTriggerScriptID, mTriggerID, mConditionID, mAsyncCondition);
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeActivateTriggerScript)
   {
      // Create the trigger script and activate it.
      BTriggerScriptID newTriggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, mTriggerScript);
      gTriggerManager.addExternalPlayerID(newTriggerScriptID, mPlayerID);
      gTriggerManager.addExternalSquadID(newTriggerScriptID, getInputSquad());
      gTriggerManager.addExternalSquadIDs(newTriggerScriptID, getInputSquadList());
      gTriggerManager.addExternalUnitID(newTriggerScriptID, getInputUnit());
      gTriggerManager.addExternalLocation(newTriggerScriptID, getInputLocation());
      gTriggerManager.activateTriggerScript(newTriggerScriptID);
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeActivateTrigger)
   {
      gTriggerManager.activateTrigger(mTriggerScriptID, mTriggerID);
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeDeactivateTrigger)
   {
      gTriggerManager.deactivateTrigger(mTriggerScriptID, mTriggerID);
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeBroadcastInputUILocationResult)
   {
      BTriggerScript *pScript = gTriggerManager.getTriggerScript(mTriggerScriptID);
      if (pScript)
      {
         BTriggerVar *pVar = pScript->getTriggerVar(mTriggerVarID);
         // ajl 10/9/06 - disabling for now so it doesn't happen in playtest
         //BASSERT(pVar);
         if(pVar)
         {
            pVar->asUILocation()->writeResult(mInputResult);
            pVar->asUILocation()->writeLocation(mInputLocation);
         }
      }
      if(gRecordGame.isPlaying())
      {
         BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
         if((pUser->getUserMode()==BUser::cUserModeInputUILocation || pUser->getUserMode()==BUser::cUserModeInputUILocationMinigame) && 
            pUser->getTriggerScriptID()==mTriggerScriptID && pUser->getTriggerVarID()==mTriggerVarID)
         {
            pUser->changeMode(BUser::cUserModeNormal);
         }
      }
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeBroadcastInputUILocationMinigameResult)
   {
      BTriggerScript *pScript = gTriggerManager.getTriggerScript(mTriggerScriptID);
      if (pScript)
      {
         BTriggerVar *pVar = pScript->getTriggerVar(mTriggerVarID);
         // ajl 10/9/06 - disabling for now so it doesn't happen in playtest
         //BASSERT(pVar);
         if(pVar)
         {
            pVar->asUILocationMinigame()->writeResult(mInputResult);
            pVar->asUILocationMinigame()->writeLocation(mInputLocation);
            pVar->asUILocationMinigame()->writeData(mInputQuality);
         }
      }
      if(gRecordGame.isPlaying())
      {
         BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
         if((pUser->getUserMode()==BUser::cUserModeInputUILocation || pUser->getUserMode()==BUser::cUserModeInputUILocationMinigame) && 
            pUser->getTriggerScriptID()==mTriggerScriptID && pUser->getTriggerVarID()==mTriggerVarID)
         {
            pUser->changeMode(BUser::cUserModeNormal);
         }
      }
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeBroadcastInputUIUnitResult)
   {
      BTriggerScript *pScript = gTriggerManager.getTriggerScript(mTriggerScriptID);
      if (pScript)
      {
         BTriggerVar *pVar = pScript->getTriggerVar(mTriggerVarID);
         // ajl 10/9/06 - disabling for now so it doesn't happen in playtest
         //BASSERT(pVar);
         if(pVar)
         {
            pVar->asUIUnit()->writeResult(mInputResult);
            pVar->asUIUnit()->writeUnit(mInputUnit);
         }
      }
      if(gRecordGame.isPlaying())
      {
         BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
         if(pUser->getUserMode()==BUser::cUserModeInputUIUnit && pUser->getTriggerScriptID()==mTriggerScriptID && pUser->getTriggerVarID()==mTriggerVarID)
            pUser->changeMode(BUser::cUserModeNormal);
      }
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeBroadcastInputUISquadResult)
   {
      BTriggerScript *pScript = gTriggerManager.getTriggerScript(mTriggerScriptID);
      if (pScript)
      {
         BTriggerVar *pVar = pScript->getTriggerVar(mTriggerVarID);
         // ajl 10/9/06 - disabling for now so it doesn't happen in playtest
         //BASSERT(pVar);
         if(pVar)
         {
            pVar->asUISquad()->writeResult(mInputResult);
            pVar->asUISquad()->writeSquad(mInputSquad);
         }
      }
      if(gRecordGame.isPlaying())
      {
         BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
         if(pUser->getUserMode()==BUser::cUserModeInputUISquad && pUser->getTriggerScriptID()==mTriggerScriptID && pUser->getTriggerVarID()==mTriggerVarID)
            pUser->changeMode(BUser::cUserModeNormal);
      }
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeBroadcastInputUISquadListResult)
   {
      BTriggerScript* pScript = gTriggerManager.getTriggerScript(mTriggerScriptID);
      if (pScript)
      {
         BTriggerVar* pVar = pScript->getTriggerVar(mTriggerVarID);
         // ajl 10/9/06 - disabling for now so it doesn't happen in playtest
         //BASSERT(pVar);
         if (pVar)
         {
            pVar->asUISquadList()->writeResult(mInputResult);
            pVar->asUISquadList()->writeSquadList(mInputSquadList);            
         }
      }
      if (gRecordGame.isPlaying())
      {
         BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
         if ((pUser->getUserMode() == BUser::cUserModeInputUISquadList) && (pUser->getTriggerScriptID() == mTriggerScriptID) && (pUser->getTriggerVarID() == mTriggerVarID))
            pUser->changeMode(BUser::cUserModeNormal);
      }
      return (true);
   }
   else if (mType == BTriggerCommand::cTypeBroadcastInputUIButtonResult)
   {
      BTriggerScript *pScript = gTriggerManager.getTriggerScript(mTriggerScriptID);
      if (pScript)
      {
         BTriggerVar *pVar = pScript->getTriggerVar(mTriggerVarID);
         if(pVar)
         {
            pVar->asUIButton()->writeResult(mInputResult, mInputLocation.x, mInputLocation.y, mInputLocation.z, mInputActionModifier, mInputSpeedModifier);
         }
      }
      return(true);
   }
   else if (mType == BTriggerCommand::cTypeBroadcastInputUIPlaceSquadListResult)
   {
      if ((mInputSquadList.getSize() > 0) && 
          ((mInputResult == BTriggerVarUILocation::cUILocationResultCancel) || (mInputResult == BTriggerVarUILocation::cUILocationResultUILockError)))
      {
         BSquad* pSquad = gWorld->getSquad(mInputSquadList[0]);
         if (pSquad)
         {
            BArmy* pArmy = pSquad->getParentArmy();
            if (pArmy)
            {
               pArmy->rePlatoon();
            }
         }
      }
      BTriggerScript* pScript = gTriggerManager.getTriggerScript(mTriggerScriptID);
      if (pScript)
      {
         BTriggerVar* pVar = pScript->getTriggerVar(mTriggerVarID);
         if (pVar)
         {
            pVar->asUILocation()->writeResult(mInputResult);
            pVar->asUILocation()->writeLocation(mInputLocation);
         }
      }
      if (gRecordGame.isPlaying())
      {
         BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
         if ((pUser->getUserMode() == BUser::cUserModeInputUIPlaceSquadList) && (pUser->getTriggerScriptID() == mTriggerScriptID) && (pUser->getTriggerVarID() == mTriggerVarID))
            pUser->changeMode(BUser::cUserModeNormal);
      }
      return (true);
   }

   // Uh oh.
   return(false);
}


//==============================================================================
// BBuildingCommand::serialize
//==============================================================================
void BBuildingCommand::serialize(BSerialBuffer &sb)
{
   BCommand::serialize(sb);
   sb.add(mType);
   sb.add(mTargetID);
   sb.add(mTargetPosition.x);
   sb.add(mTargetPosition.y);
   sb.add(mTargetPosition.z);
   sb.add(mCount);
   sb.add(mSocketID.asLong());
}

//==============================================================================
// BBuildingCommand::deserialize
//==============================================================================
void BBuildingCommand::deserialize(BSerialBuffer &sb)
{
   BCommand::deserialize(sb);
   sb.get(&mType);
   sb.get(&mTargetID);
   sb.get(&mTargetPosition.x);
   sb.get(&mTargetPosition.y);
   sb.get(&mTargetPosition.z);
   sb.get(&mCount);
   if (gRecordGame.isPlaying() && gRecordGame.getVersion()<7)
      mSocketID=cInvalidObjectID;
   else
   {
      long socketID=-1;
      sb.get(&socketID);
      mSocketID.set(socketID);
   }
}

//==============================================================================
// BBuildingCommand::processUnit
//==============================================================================
bool BBuildingCommand::processUnit(BUnit* pUnit)
{
   if(!pUnit)
      return false;

   //FIXME AJL 5/4/06 - Need to add resource cost checking

   switch(mType)
   {
      case BProtoObjectCommand::cTypeResearch:
      {
         const BProtoObject* pProtoObject=pUnit->getProtoObject();
         uint numCommands = pProtoObject->getNumberCommands();
         for(uint i=0; i<numCommands; i++)
         {
            BProtoObjectCommand command=pProtoObject->getCommand(i);
            if(command.getType()==mType && command.getID()==mTargetID)
            {
               pUnit->doResearch(mPlayerID, mTargetID, mCount);
               
               if(mCount > 0)
               {
                  gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandResearch, mPlayerID, pUnit->getID(), mTargetID );
               }
               else 
               {
                  gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandResearchCancel, mPlayerID, pUnit->getID(), mTargetID );
               }
               break;
            }
         }
         break;
      }

      case BProtoObjectCommand::cTypeTrainUnit:
      {
         const BProtoObject* pProtoObject=pUnit->getProtoObject();
         uint numCommands = pProtoObject->getNumberCommands();
         for(uint i=0; i<numCommands; i++)
         {
            BProtoObjectCommand command=pProtoObject->getCommand(i);
            if(command.getType()==mType && command.getID()==mTargetID)
            {
               pUnit->doTrain(mPlayerID, mTargetID, mCount, false, false);
               break;
            }
         }
         break;
      }

      case BProtoObjectCommand::cTypeTrainSquad:
      {
         const BProtoObject* pProtoObject=pUnit->getProtoObject();
         uint numCommands = pProtoObject->getNumberCommands();
         for(uint i=0; i<numCommands; i++)
         {
            BProtoObjectCommand command=pProtoObject->getCommand(i);
            if(command.getType()==mType && command.getID()==mTargetID)
            {
               pUnit->doTrain(mPlayerID, mTargetID, mCount, true, false);
               
               if(mCount > 0)
               {
                  gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandTrainSquad, mPlayerID, pUnit->getID(), mTargetID );
               }
               else
               {
                  gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandTrainSquadCancel, mPlayerID, pUnit->getID(), mTargetID );
               }

               break;
            }
         }
         break;
      }

      case BProtoObjectCommand::cTypeBuild:
         if(mCount==-1)
            pUnit->doBuild(mPlayerID, true, false, false);
         else
         {
            BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
//-- FIXING PREFIX BUG ID 2384
            const BProtoObject* pProtoBuilding=pPlayer->getProtoObject(mTargetID);
//--
            if(pProtoBuilding && 
               pPlayer->checkCost(pProtoBuilding->getCost()) && 
               pUnit->getTrainLimit(mPlayerID, mTargetID, false)!=0 &&
               pPlayer->checkPops(pProtoBuilding->getPops()))
            {
               const BProtoObject* pProtoObject=pUnit->getProtoObject();
               uint numCommands = pProtoObject->getNumberCommands();
               for(uint i=0; i<numCommands; i++)
               {
                  BProtoObjectCommand command=pProtoObject->getCommand(i);
                  if(command.getType()==mType && command.getID()==mTargetID && pProtoObject->getCommandAvailable(i))
                  {
                     gWorld->createEntity(mTargetID, false, pUnit->getPlayerID(), mTargetPosition, cZAxisVector, cXAxisVector, false, false, false, cInvalidObjectID, mPlayerID, pUnit->getID(), mSocketID);
                     break;
                  }
               }
            }
         }
         break;

      case BProtoObjectCommand::cTypeBuildOther:
      {
         if(mCount==-1)
         {
            pUnit->doBuildOther(mPlayerID, mPlayerID, mTargetID, true, false);
         }
         else
         {
            BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
//-- FIXING PREFIX BUG ID 2385
            const BProtoObject* pProtoBuilding=pPlayer->getProtoObject(mTargetID);
//--
            if (pProtoBuilding && pPlayer->checkCost(pProtoBuilding->getCost()) && 
               pUnit->getTrainLimit(mPlayerID, mTargetID, false)!=0 &&
               pPlayer->checkPops(pProtoBuilding->getPops()))
            {
               const BProtoObject* pProtoObject=pUnit->getProtoObject();
               uint numCommands = pProtoObject->getNumberCommands();
               for(uint i=0; i<numCommands; i++)
               {
                  BProtoObjectCommand command=pProtoObject->getCommand(i);
                  if(command.getType()==mType && command.getID()==mTargetID && pProtoObject->getCommandAvailable(i))
                  {
                     BPlayerID playerID = mPlayerID;
                     if (pPlayer->getCoopID() != -1)
                        playerID = pPlayer->getCoopID();
                     pUnit->doBuildOther(playerID, mPlayerID, mTargetID, false, false);

                     gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandBuildOther, mPlayerID, pUnit->getID(), mTargetID );
                     break;
                  }
               }
            }
         }
         break;
      }

      case BProtoObjectCommand::cTypeUnloadUnits:
      {         
         BSquad* pSquad = pUnit->getParentSquad();
         if (pSquad)
         {
            BEntityIDArray garrisonedSquads;
            pSquad->queueUnload(BSimOrder::cPriorityUser, garrisonedSquads, false, getPlayerID());
         }
         break;
      }

      case BProtoObjectCommand::cTypeKill:
      case BProtoObjectCommand::cTypeDestroyBase:
      {
         // (Jira 17652) Confirm unit has a self destruct command.  It was somehow possible for users to send
         // a self destruct command to sockets when destroying a building.
         bool okToSelfDestruct = true;
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (pProtoObject)
         {
            okToSelfDestruct = false;
            uint numCommands = pProtoObject->getNumberCommands();
            for (uint i = 0; i < numCommands; i++)
            {
               BProtoObjectCommand command = pProtoObject->getCommand(i);
               long commandType = command.getType();
               if ((commandType == BProtoObjectCommand::cTypeKill) || (commandType == BProtoObjectCommand::cTypeDestroyBase))
               {
                  okToSelfDestruct = true;
                  break;
               }
            }
         }

         if (okToSelfDestruct)
         {
            gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandRecycle, mPlayerID, pUnit->getID() );
            pUnit->doSelfDestruct(false);
         }
         break;
      }

      case BProtoObjectCommand::cTypeCancelKill:
      case BProtoObjectCommand::cTypeCancelDestroyBase:
      {
         gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandRecycleCancel, mPlayerID, pUnit->getID() );

         pUnit->doSelfDestruct(true);
         break;
      }

      case BProtoObjectCommand::cTypeCustomCommand:
      {
//-- FIXING PREFIX BUG ID 2386
         const BCustomCommand* pCustomCommand=gWorld->getCustomCommand(mTargetID);
//--
         if (pCustomCommand && pCustomCommand->mUnitID == pUnit->getID())
            pUnit->doCustomCommand(mPlayerID, mTargetID, mCount==-1);
         break;
      }

      case BProtoObjectCommand::cTypeTrainLock:
         pUnit->setTrainLock(mPlayerID, true);
         break;

      case BProtoObjectCommand::cTypeTrainUnlock:
         pUnit->setTrainLock(mPlayerID, false);
         break;

      case BProtoObjectCommand::cTypeChangeMode:
      {
         BSquad* pSquad = pUnit->getParentSquad();
         if (pSquad)
         {
            BSimOrder* pOrder=gSimOrderManager.createOrder();
            if (pOrder)
            {
               pOrder->setMode((int8)mTargetID);
               pOrder->setPriority(BSimOrder::cPriorityUser);
               if (!pSquad->queueOrder(pOrder, BSimOrder::cTypeChangeMode, false))
                  gSimOrderManager.markForDelete(pOrder);
               else
               {
                  BUser* pUser = gUserManager.getPrimaryUser();
                  if (pUser->getUserMode() == BUser::cUserModeCommandMenu && pUser->getCommandObject() == pUnit->getID())
                  {
                     pUser->resetUserMode();
                     pUser->getSelectionManager()->selectUnit(pUnit->getID());
                  }
                  if (gGame.isSplitScreen())
                  {
                     pUser = gUserManager.getSecondaryUser();
                     if (pUser->getUserMode() == BUser::cUserModeCommandMenu && pUser->getCommandObject() == pUnit->getID())
                     {
                        pUser->resetUserMode();
                        pUser->getSelectionManager()->selectUnit(pUnit->getID());
                     }
                  }
               }
            }
         }
         break;
      }
      case BProtoObjectCommand::cTypeClearRallyPoint:
         pUnit->clearRallyPoint(mPlayerID);
         break;
   }

   return true;
}


//==============================================================================
// BGameCommand::serialize
//==============================================================================
enum
{
   cGameCmdData      = 1,
   cGameCmdData2     = 2,
   cGameCmdPosX      = 4,
   cGameCmdPosY      = 8,
   cGameCmdPosZ      = 16,
   cGameCmdPos2      = 32,
};
void BGameCommand::serialize(BSerialBuffer &sb)
{
   // Set flags that determine how to serialize the data to keep it small.
   BYTE flags = 0;

   if(mData!=0)
      flags+=cGameCmdData;

   if(mData2!=0)
      flags+=cGameCmdData2;

   if(mPosition.x!=0.0f)
      flags+=cGameCmdPosX;

   if(mPosition.y!=0.0f)
      flags+=cGameCmdPosY;

   if(mPosition.z!=0.0f)
      flags+=cGameCmdPosZ;

   if(mPosition2!=cOriginVector)
      flags+=cGameCmdPos2;

   // Serialize the data.
   BCommand::serialize(sb);

   BYTE type = (BYTE)mType;
   sb.add(type);

   sb.add(flags);

   if(flags&cGameCmdData)
      sb.add(mData);

   if(flags&cGameCmdData2)
      sb.add(mData2);

   if(flags&cGameCmdPosX)
      sb.add(mPosition.x);

   if(flags&cGameCmdPosY)
      sb.add(mPosition.y);

   if(flags&cGameCmdPosZ)
      sb.add(mPosition.z);

   if(flags&cGameCmdPos2)
   {
      sb.add(mPosition2.x);
      sb.add(mPosition2.y);
      sb.add(mPosition2.z);
   }
}

//==============================================================================
// BGameCommand::deserialize
//==============================================================================
void BGameCommand::deserialize(BSerialBuffer &sb)
{
   BCommand::deserialize(sb);

   if(gRecordGame.isPlaying() && gRecordGame.getVersion()<6)
   {
      sb.get(&mType);
      if(gRecordGame.isPlaying() && gRecordGame.getVersion()<2)
         mData=0;
      else
         sb.get(&mData);
      if(gRecordGame.isPlaying() && gRecordGame.getVersion()<5)
         mData2=0;
      else
         sb.get(&mData2);
      sb.get(&mPosition.x);
      sb.get(&mPosition.y);
      sb.get(&mPosition.z);
      if(gRecordGame.isPlaying() && gRecordGame.getVersion()<9)
         mPosition2=cOriginVector;
      else
      {
         sb.get(&mPosition2.x);
         sb.get(&mPosition2.y);
         sb.get(&mPosition2.z);
      }
   }
   else
   {
      BYTE type=(BYTE)-1;
      sb.get(&type);
      if (type==(BYTE)-1)
         mType = -1;
      else
         mType = type;

      BYTE flags=0;
      sb.get(&flags);

      mData = 0;
      if(flags&cGameCmdData)
         sb.get(&mData);

      mData2 = 0;
      if(flags&cGameCmdData2)
         sb.get(&mData2);

      mPosition = cOriginVector;
      if(flags&cGameCmdPosX)
         sb.get(&mPosition.x);
      if(flags&cGameCmdPosY)
         sb.get(&mPosition.y);
      if(flags&cGameCmdPosZ)
         sb.get(&mPosition.z);

      if(flags&cGameCmdPos2)
      {
         sb.get(&mPosition2.x);
         sb.get(&mPosition2.y);
         sb.get(&mPosition2.z);
      }
   }
}

//==============================================================================
// BGameCommand::processGame
//==============================================================================
bool BGameCommand::processGame()
{
   switch(mType)
   {
      case cTypeQuickBuild:
      {
         #ifndef BUILD_FINAL            
            gWorld->setFlagQuickBuild(!gWorld->getFlagQuickBuild());            
            BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
            if(pUser)
               pUser->multiplayerCheatNotification(mPlayerID, "Quick Build");
         #endif
         break;
      }

      case cTypeSwitchPlayer:
      {
         // Halwes - 9/26/2008 - Switching the player for now is used in the advanced tutorial
         //#ifndef BUILD_FINAL
            if(gRecordGame.isPlaying())
            {
               gRecordGame.setCurrentPlayer(mData);
               if(!gRecordGame.isViewLocked())
                  break;
            }
            BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
            if(pUser)
               pUser->switchPlayer(mData);
         //#endif
         break;
      }

      case cTypeAddResources:
      {
         #ifndef BUILD_FINAL
            BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
            if(pPlayer)
            {
               if (mData==gDatabase.getLeaderPowerChargeResourceID())
                  pPlayer->addResource(mData, pPlayer->getLeaderPowerChargeCost());
               else if (!gDatabase.getResourceDeductable(mData))
               {
                  pPlayer->addResource(mData, 10.0f);
                  gWorld->notify(BEntity::cEventBuildingResource, cInvalidObjectID, mData, pPlayer->getID());
               }
               else
                  pPlayer->addResource(mData, 10000.0f);
               BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
               if(pUser)
                  pUser->multiplayerCheatNotification(mPlayerID, "Add Resource");
            }
         #endif
         break;
      }

      case cTypeAddPopulation:
      {
         #ifndef BUILD_FINAL
            BPlayer *pPlayer=gWorld->getPlayer(mPlayerID);
            if(pPlayer)
            {
               float popMax = pPlayer->getPopMax(mData);
               float newPop = pPlayer->getPopCap(mData);
               newPop += 20;
               if(newPop > popMax)
                  break;

               pPlayer->setPopCap(mData, newPop);

               BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
               if(pUser)
                  pUser->multiplayerCheatNotification(mPlayerID, "Add Pop");
            }
         #endif
         break;
      }

      case cTypeFogOfWar:
      {
         #ifndef BUILD_FINAL
            if(gRecordGame.isPlaying())
            {
               gRecordGame.setFogOfWar((mData?true:false));
               if(!gRecordGame.isViewLocked())
                  break;
            }
            if(mData)
               gConfig.remove(cConfigNoFogMask);
            else
               gConfig.define(cConfigNoFogMask);
            BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
            if(pUser)
               pUser->multiplayerCheatNotification(mPlayerID, "Fog of War");
         #endif
         break;
      }

      case cTypeDestroySquad:
      {
         #ifndef BUILD_FINAL
            BUnit* pUnit=gWorld->getUnit(mData);
            if(pUnit)
            {
               #ifdef SYNC_Squad
                  syncSquadData("BGameCommand::processGame mID", mData);
               #endif
               BSquad *pSquad = gWorld->getSquad(pUnit->getParentID());
               if (pSquad)
               {
                  // ajl 6/18/07 - don't call BSquad::kill directly because the units may not actually they are killed
                  // (such as when they are transformed into their death replacement). BSquad::kill will automatically
                  // get called when the squad updates and it doesn't have any children left.
                  int count=(int)pSquad->getNumberChildren();
                  for (int i=count-1; i>=0; i--)
                  {
                     BUnit* pChildUnit=gWorld->getUnit(pSquad->getChild(i));
                     if (pChildUnit)
                        pChildUnit->kill(false);
                  }
               }
               else
                  pUnit->kill(false);
               BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
               if(pUser)
                  pUser->multiplayerCheatNotification(mPlayerID, "Destroy");
            }
         #endif
         break;
      }

      case cTypeDestroyUnit:
      {
         #ifndef BUILD_FINAL
            BUnit* pUnit=gWorld->getUnit(mData);
            if(pUnit)
            {
               #ifdef SYNC_Unit
                  syncUnitData("BGameCommand::processGame mID", mData);
               #endif
               pUnit->kill(false);
               BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
               if(pUser)
                  pUser->multiplayerCheatNotification(mPlayerID, "Destroy");
            }
         #endif
         break;
      }

      case cTypeResign:
      {
         BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
         if(pPlayer)
            pPlayer->doResign();
         break;
      }

      case cTypeDisconnect:
      {
         BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
         if(pPlayer)
            pPlayer->doDisconnect();
         break;
      }

      case cTypeFlare:
      {
         long numPlayers = gWorld->getNumberPlayers();
         for (long i=0; i<numPlayers; i++)
         {
            const BPlayer* pPlayer = gWorld->getPlayer(i);
            if (pPlayer && pPlayer->isAlly(mPlayerID))
               pPlayer->getAlertManager()->createFlareAlert(mPosition, mPlayerID);
         }
         break;
      }

      case cTypeSetGlobalRallyPoint:
      {
         // Validate position
         if (!isValidTerrainPosition(mPosition))
            return false;

         BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
         if(pPlayer)
            pPlayer->setRallyPoint(mPosition, BEntityID(mData));
         break;
      }

      case cTypeClearGlobalRallyPoint:
      {
         BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
         if(pPlayer)
            pPlayer->clearRallyPoint();
         break;
      }

      case cTypeSetBuildingRallyPoint:
      {
         // Validate position
         if (!isValidTerrainPosition(mPosition))
            return false;

         BUnit* pUnit=gWorld->getUnit(mData);
         if(pUnit)
            pUnit->setRallyPoint(mPosition, BEntityID(mData2), mPlayerID);
         break;
      }

      case cTypeClearBuildingRallyPoint:
      {
         BUnit* pUnit=gWorld->getUnit(mData);
         if(pUnit)
            pUnit->clearRallyPoint(mPlayerID);
         break;
      }

      case cTypeSelectPower:
      {
         uint powerIndex=(uint)mPosition.x;
         uint subPowerIndex=(uint)mPosition.y;
         int iconLocation=(int)mPosition.z;
         BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
         if(pPlayer && pPlayer->getSupportPowerStatus(powerIndex)==BPlayer::cSupportPowerStatusAvailable)
         {
            const BLeaderSupportPower* pSupportPower=pPlayer->getSupportPower(powerIndex);
            if(pSupportPower)
            {
               int protoPowerID = pSupportPower->mPowers[subPowerIndex];
               pPlayer->addPowerEntry(protoPowerID, cInvalidObjectID, 1, iconLocation);
               pPlayer->setSupportPowerStatus(powerIndex, BPlayer::cSupportPowerStatusSelected);

               if(!gRecordGame.isPlaying())
               {
                  BUser* pUser = pPlayer->getUser();
                  if (pUser)
                     pUser->invokePower(protoPowerID, cInvalidObjectID);
               }
            }
         }
         break;
      }

      case cTypeRevealMap:
      {
         #ifndef BUILD_FINAL
         bool useFog = (mData==0);
         bool useSkirt = (useFog ? false : true);
         bool onlyVisual = false;
         gWorld->setFogOfWar(useFog, useSkirt, onlyVisual);
         #endif
         break;
      }

      case cTypeCreateSquad:
      {
         #ifndef BUILD_FINAL
            for (int i = 0; i < mData2; i++)
               gWorld->createEntity(mData, true, mPlayerID, mPosition, cZAxisVector, cXAxisVector, true);
         #endif
         break;
      }

      case cTypeCreateObject:
      {
         #ifndef BUILD_FINAL
            for (int i = 0; i < mData2; i++)
               gWorld->createEntity(mData, false, mPlayerID, mPosition, cZAxisVector, cXAxisVector, true);
         #endif
         break;
      }

      case cTypeTribute:
      {
         BPlayer* pSendingPlayer=gWorld->getPlayer(mPlayerID);
         BPlayer* pReceivingPlayer=gWorld->getPlayer(mData);
         if (pSendingPlayer && pReceivingPlayer && pSendingPlayer->getPlayerState()==BPlayer::cPlayerStatePlaying && pReceivingPlayer->getPlayerState()==BPlayer::cPlayerStatePlaying)
         {
            float tributeAmount=gDatabase.getTributeAmount();
            int resourceID=mData2;
            float tributeCost=tributeAmount+(tributeAmount*pSendingPlayer->getTributeCost());
            if (resourceID>=0 && pSendingPlayer->getResource(resourceID)>=tributeCost)
            {
               pSendingPlayer->addResource(resourceID, -tributeCost, BPlayer::cFlagFromTribute, true);
               pReceivingPlayer->addResource(resourceID, tributeAmount, BPlayer::cFlagFromTribute, true);
               gWorld->notify(BEntity::cEventTribute, cInvalidObjectID, mPlayerID, mData<<16|resourceID);
            }
         }
         break;
      }

      case cTypeRepair:
      {
         BCost totalCost, squadCost;
         uint squadCount=mRecipients.getNumber();
         for (uint i=0; i<squadCount; i++)
         {
            const BSquad* pSquad=gWorld->getSquad(mRecipients[i]);
            if (pSquad && !pSquad->getFlagIsRepairing())
            {
               float temp;
               if (pSquad->getRepairCost(squadCost, temp))
                  totalCost.add(&squadCost);
            }
         }
         if (gWorld->getPlayer(mPlayerID)->payCost(&totalCost))
         {
            mRecipientType = cArmy;
            processArmies();
         }
         break;
      }

      case cTypeAttackMove:
      {
         //We need to see if the squads in the recipient list are still in the same army.  If not,
         //we just ignore this.  If so, we propagate the attack move down the chain.
         BEntityID commonArmyID=BEntityGrouper::commonArmyIDForSquads(mRecipients);
         BArmy* pArmy=gWorld->getArmy(commonArmyID);
         if (pArmy)
            pArmy->propagateAttackMove();
         break;
      }

      case cTypeUnitPower:
      {
         BEntityID unitID(mData);
         BUnit* pUnit = gWorld->getUnit(unitID);
         if (pUnit)
         {
//-- FIXING PREFIX BUG ID 2387
            const BSquad* pSquad = pUnit->getParentSquad();
//--
            if (pSquad)
            {
               BEntityID squadID = pSquad->getID();
               int protoPowerID = pUnit->getProtoObject()->getProtoPowerID();
               if (protoPowerID != -1)
               {
//-- FIXING PREFIX BUG ID 2390
                  const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
                  if (pProtoPower && pProtoPower->getFlagUnitPower())
                  {
                     BPlayer* pPlayer = pUnit->getPlayer();
                     if (pPlayer->canUsePower(protoPowerID, squadID))
                     {
                        bool unitAvail = true;
                        int actionType = pProtoPower->getActionType();
                        if (actionType == BAction::cActionTypeSquadCarpetBomb)
                        {
//-- FIXING PREFIX BUG ID 2389
                           const BUnitActionMoveAir* pAirMoveAction = reinterpret_cast<BUnitActionMoveAir*>(pUnit->getActionByType(BAction::cActionTypeUnitMoveAir));
//--
                           if (pAirMoveAction && pAirMoveAction->getState()!=BAction::cStateNone)
                              unitAvail = false;
                        }
                        if (unitAvail)
                        {
                           const BSimString& commandTriggerScript = pProtoPower->getCommandTriggerScript();
                           if (!commandTriggerScript.isEmpty())
                           {
                              if (pPlayer->usePower(protoPowerID, squadID, cInvalidObjectID))
                              {
                                 BTriggerScriptID newTriggerScriptID = gTriggerManager.createTriggerScriptFromFile(cDirTriggerScripts, commandTriggerScript);
                                 gTriggerManager.addExternalPlayerID(newTriggerScriptID, mPlayerID);
                                 gTriggerManager.addExternalProtoPowerID(newTriggerScriptID, protoPowerID);
                                 gTriggerManager.addExternalLocation(newTriggerScriptID, mPosition);
                                 gTriggerManager.addExternalSquadID(newTriggerScriptID, squadID);
                                 gTriggerManager.activateTriggerScript(newTriggerScriptID);
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
         break;
      }

      case cTypeLookAtPosBroadcast:
      {
         if (mSenders.size() > 0)
         {
            BPlayer* pPlayer = gWorld->getPlayer(mSenders[0]);
            BASSERT(pPlayer);
            if (pPlayer)
               pPlayer->setLookAtPos(mPosition);
         }
         break;
      }

      case cTypeGameSpeed:
      {
         #ifndef BUILD_FINAL            
            gConfig.set(cConfigGameSpeed, mDataFloat);
            gParticleGateway.setTimeSpeed(mDataFloat);
            BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
            if(pUser)
               pUser->multiplayerCheatNotification(mPlayerID, "Game Speed");
         #endif
         break;
      }

      case cTypeToggleAI:
      {
         #ifndef BUILD_FINAL
         long numTeams = gWorld->getNumberTeams();
         for (long i=0; i<numTeams; i++)
         {
            BKB* pKB = gWorld->getKB(i);
            if (pKB)
               pKB->toggleFlagPaused();
         }
         long numPlayers = gWorld->getNumberPlayers();
         for (long i=0; i<numPlayers; i++)
         {
            BAI* pAI = gWorld->getAI(i);
            if (pAI)
            {
               pAI->toggleFlagPaused();
               BTriggerScript* pTS = gTriggerManager.getTriggerScript(pAI->getAITriggerScriptID());
               if (pTS)
                  pTS->toggleFlagPaused();
            }
         }
         #endif

         break;
      }

      case cTypeReverseHotDrop:
      {
         // Get hotdrop and leader unit
         BUnit* pHotDropUnit = gWorld->getUnit(mData);
         BUnit* pLeaderUnit = gWorld->getUnit(mData2);
         BASSERT(pHotDropUnit);
         BASSERT(pLeaderUnit);
         if (!pHotDropUnit || !pLeaderUnit)
            break;

         BPlayer* pPlayer = pLeaderUnit->getPlayer();
         if (pHotDropUnit && !pHotDropUnit->getFlagBlockContain() && pLeaderUnit && pPlayer && pPlayer->checkCost(pPlayer->getLeader()->getReverseHotDropCost()))
         {
            BSquad* pLeaderSquad = pLeaderUnit->getParentSquad();
            if (pLeaderSquad)
            {
               // Already garrisoning, stop
               if (pLeaderSquad->getActionByType(BAction::cActionTypeSquadGarrison))
                  break;

               // MS 11/13/2008: PHX-17878, not if dude is doing a fatality
               if(gFatalityManager.getFatality(pLeaderUnit->getID()))
                  break;

               BSimTarget target(pHotDropUnit->getID());
               BSimOrder* pNewOrder = gSimOrderManager.createOrder();
               BASSERT(pNewOrder);
               pNewOrder->setPriority(BSimOrder::cPriorityUser);

               // Cancel garrison orders for all units attempting to garrison in the hotdrop
               BSquad* pSquad = NULL;
               BEntityHandle handle = cInvalidObjectID;
               while ((pSquad = gWorld->getNextSquad(handle)) != NULL)
               {
                  if (pSquad == pLeaderSquad)
                     continue;

                  uint count = pSquad->getNumberOrders();
                  for (uint i = 0; i < count; ++i)
                  {
                     const BSimOrderEntry* pOrder = pSquad->getOrderEntry(i);

                     if (pOrder && pOrder->getType() == BSimOrder::cTypeGarrison)
                     {
                        if (pOrder->getOrder() && pOrder->getOrder()->getTarget().getID() == target.getID())
                        {
                           pSquad->removeOrder(pOrder->getOrder(), true, true);
                        }
                     }
                  }
               }

               // Garrison the leader
               BActionID actionID = pLeaderSquad->doGarrison(NULL, NULL, &target, true, true);
               if (actionID != cInvalidActionID)
                  pPlayer->payCost(pPlayer->getLeader()->getReverseHotDropCost());
            }
         }

         break;
      }

      case cTypeActivateSkull:
      {
         gCollectiblesManager.setSkullActivationInternal(mData, (mData2!=0), mPlayerID);
         break;
      }

      case cTypeSubUpdating:
         gWorld->setSubUpdating((mData == 1), gWorld->getAlternateSubUpdating(), gWorld->getDynamicSubUpdateTime(), gDecoupledUpdate);
         break;
   }
   return true;
}

//==============================================================================
// BGameCommand::processArmy
//==============================================================================
bool BGameCommand::processArmy(BArmy* pArmy)
{
   if (pArmy == NULL)
      return(false); 
   pArmy->queueOrder(this);
   return(true);
}


//==============================================================================
// BGeneralEventCommand::serialize
//==============================================================================
void BGeneralEventCommand::serialize(BSerialBuffer &sb)
{
   BCommand::serialize(sb);
   sb.add(mType);
   uint numEvents = mSubscribers.getSize();
   sb.add(numEvents);
   for (uint i = 0; i < numEvents; i++)
   {
      sb.add(mSubscribers[i]);
   }
}

//==============================================================================
// BGeneralEventCommand::deserialize
//==============================================================================
void BGeneralEventCommand::deserialize(BSerialBuffer &sb)
{
   BCommand::deserialize(sb);
   sb.get(&mType);
   uint numEvents = 0;
   sb.get(&numEvents);
   for (uint i = 0; i < numEvents; i++)
   {
      uint subscriberID;
      sb.get(&subscriberID);
      mSubscribers.add(subscriberID);
   }
} 


//==============================================================================
// BGeneralEventCommand::processGame
//==============================================================================
bool BGeneralEventCommand::processGame()
{
   switch(mType)
   {
      case cEventFired:
      {
         for (uint i = 0; i < mSubscribers.getSize(); i++)
         {
            gGeneralEventManager.subscriberFired(mSubscribers[i]);
         }
         break;
      }
   }   
   return true;
}

//==============================================================================
// BGeneralEventCommand::processGame
//==============================================================================
void  BGeneralEventCommand::addSubscriber(uint subscriber)
{
   mSubscribers.add(subscriber);
}
