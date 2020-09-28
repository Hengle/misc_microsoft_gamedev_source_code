//==============================================================================
// battle.cpp
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "common.h"
#include "battle.h"
#include "game.h"
#include "player.h"
#include "world.h"
#include "tactic.h"
#include "squad.h"
#include "gamedirectories.h"
#include "protoobject.h"
#include "uigame.h"
#include "usermanager.h"
#include "user.h"
#include "config.h"
#include "configsgame.h"
#include "vincehelper.h"
#include "uigame.h"
#include "worldsoundmanager.h"
#include "protosquad.h"

//==============================================================================
// Defines
//#define DEBUGBATTLE
//==============================================================================
// BBattlePlayer::BBattlePlayer
//==============================================================================
BBattlePlayer::BBattlePlayer(long id) :
mID(id), mPlayerStartTime(0xFFFFFFFF)
{
   setDefaultFlags();   
}

//==============================================================================
// BBattlePlayer::~BBattlePlayer
//==============================================================================
BBattlePlayer::~BBattlePlayer(void)
{
   //Take our units out of this battle.
   for (long i=0; i < mUnits.getNumber(); i++)
   {
      if(gWorld->getObjectiveManager() == NULL)
         return;
      BUnit *unit=gWorld->getUnit(mUnits[i]);
      if (unit != NULL)
         unit->setBattleID(-1);
   }
   
   //-- Properly remove each unit for bookkeeping
   BEntityIDArray tempArray = mUnits;
   for(uint i = 0; i < tempArray.getSize(); i++)
   {
      removeUnit(tempArray[i]);
   }

}

//==============================================================================
// BBattlePlayer::getUnitID
//==============================================================================
BEntityID BBattlePlayer::getUnitID(long index) const
{
   if ((index < 0) || (index >= mUnits.getNumber()) )
      return(cInvalidObjectID);
   return(mUnits[index]);
}

//==============================================================================
// BBattlePlayer::addUnit
//==============================================================================
bool BBattlePlayer::addUnit(BEntityID unitID)
{
   if(mUnits.getNumber() == 0)
      mPlayerStartTime = gWorld->getGametime();

   if (mUnits.uniqueAdd(unitID) < 0)
      return(false);

   //-- Report to Listener
   if(gWorld->getBattleManager()->getListener())
      gWorld->getBattleManager()->getListener()->unitAddedToBattle();

   return(true);
}

//==============================================================================
// BBattlePlayer::removeUnit
//==============================================================================
bool BBattlePlayer::removeUnit(BEntityID unitID)
{
   bool result = mUnits.remove(unitID);
   BUnit *pUnit = gWorld->getUnit(unitID);
   if(!pUnit || !result)
      return false;

   //-- Report to Listener
   if(gWorld->getBattleManager()->getListener())
      gWorld->getBattleManager()->getListener()->unitRemovedFromBattle();

   return(true);
}

//==============================================================================
// BBattlePlayer::containsUnit
//==============================================================================
bool BBattlePlayer::containsUnit(BEntityID unitID) const
{
   for (long i=0; i < mUnits.getNumber(); i++)
   {
      if (mUnits[i] == unitID)
         return(true);
   }
   return(false);
}

//==============================================================================
// BBattlePlayer::update
//==============================================================================
bool BBattlePlayer::update(BBattle *battle)
{
   //Bomb check.
   if (battle == NULL)
      return(false);

   return(true);
}

//==============================================================================
// BBattlePlayer::setDefaultFlags
//==============================================================================
void BBattlePlayer::setDefaultFlags(void)
{
   mFlagFirstUpdate = true;
}


//==============================================================================//==============================================================================

//==============================================================================
// BBattleUnit::BBattleUnit
//==============================================================================
BBattleUnit::BBattleUnit(void)
{
   reset();
}

//==============================================================================
// BBattleUnit::~BBattleUnit
//==============================================================================
BBattleUnit::~BBattleUnit(void)
{
}

//==============================================================================
// BBattleUnit::startExit
//==============================================================================
void BBattleUnit::startExit(DWORD v)
{
   mFlagExiting = true;
   mExitTime=gWorld->getGametime();
   mExitTime+=v;
}

//==============================================================================
// BBattleUnit::stopExit
//==============================================================================
void BBattleUnit::stopExit(void)
{
   mFlagExiting = false;
   mExitTime=(DWORD)0;
}

//==============================================================================
// BBattleUnit::reset
//==============================================================================
void BBattleUnit::reset(void)
{
   mUnitID=cInvalidObjectID;
   mTargetUnitID=cInvalidObjectID;
   mPlayerID=-1;
   mExitTime=(DWORD)0;
}

//==============================================================================
// BBattleUnit::update
//==============================================================================
void BBattleUnit::update(BBattle *battle)
{
   //Bomb check.
   if (battle == NULL)
      return;

   //Make sure this unit is still viable.
   BUnit *unit=gWorld->getUnit(mUnitID);
   if (unit == NULL)
   {
      mFlagDestroy = true;
      return;
   }

   //Target.  Make sure it's valid.
   mTargetUnitID = unit->getAttackTargetID();
   BUnit *targetUnit=NULL;
   if (mTargetUnitID != cInvalidObjectID)
   {
      targetUnit=gWorld->getUnit(mTargetUnitID);
      if (targetUnit == NULL)
         mTargetUnitID=cInvalidObjectID;
   }

   //Ask the squad if we can attack?
   //BTactic *tactic=unit->getTactic();
   //if ((tactic != NULL) && (tactic->canAttack(unit) == true))
   BSquad *pSquad = unit->getSquad();
   if(pSquad && pSquad->canAttackTarget(mTargetUnitID))
      mFlagCanAttack = true;      
   else
      mFlagCanAttack = false;
   
   //If this unit is exiting and that's up, set the destroy flag.
   if ((mFlagExiting == true) && (gWorld->getGametime() >= mExitTime))
   {
      mFlagDestroy = true;
      return;
   }

   //If this unit is too far out of the battle and it's target is either not in
   //this battle or also out of range, remove him (to be put in his own battle).
   float distanceToBattleSqr=unit->getPosition().xzDistanceSqr(battle->getPosition());
   float distanceDiff=battle->getSizeSqr()-distanceToBattleSqr;
   if (distanceDiff < -cFloatCompareEpsilon)
   {
      #ifdef DEBUGBATTLE
      battle->debug("Unit %d (%s): Out of battle, distanceDiff=%.3f.", unit->getID(), unit->getProtoObject()->getName(), distanceDiff);
      #endif
      bool targetOutOfBattle=true;
      //If we have a target, we can only stay in this battle if it's inside the battle
      //size, too.
      if (targetUnit != NULL)
      {
         #ifdef DEBUGBATTLE
         battle->debug("  Target %d (%s) is valid.", targetUnit->getID(), targetUnit->getProtoObject()->getName());
         #endif
         if (targetUnit->getBattleID() == battle->getID())
         {
            distanceToBattleSqr=targetUnit->getPosition().xzDistanceSqr(battle->getPosition());
            distanceDiff=battle->getSizeSqr()-distanceToBattleSqr;
            #ifdef DEBUGBATTLE
            battle->debug("    Target is in same battle, distanceDiff=%.3f.", distanceDiff);
            #endif
            if (distanceDiff > -cFloatCompareEpsilon)
            {
               #ifdef DEBUGBATTLE
               battle->debug("      Target is not out of battle.");
               #endif
               targetOutOfBattle=false;
            }
            #ifdef DEBUGBATTLE
            else
               battle->debug("      Target is out of battle.");
            #endif
         }
         #ifdef DEBUGBATTLE
         else
            battle->debug("      Target is not in same battle, BID=%d.", targetUnit->getBattleID());
         #endif
      }
      #ifdef DEBUGBATTLE
      else
         battle->debug("      Target is not valid.");
      #endif
      //If our target is out, too, set our split flag.
      if (targetOutOfBattle == true)
      {
         #ifdef DEBUGBATTLE
         battle->debug("  Setting SPLIT.");
         #endif
         mFlagSplit = true;
      }
   }
   #ifdef DEBUGBATTLE
   else
      battle->debug("Unit %d (%s): NOT out of battle, distanceDiff=%.3f.", unit->getID(), unit->getName(), distanceDiff);
   #endif
}

//==============================================================================
// BBattleUnit::setDefaultFlags
//==============================================================================
void BBattleUnit::setDefaultFlags(void)
{
   mFlagExiting = false;
   mFlagDestroy = false;
   mFlagCanAttack = true;
   mFlagSplit = false;   
}

//==============================================================================//==============================================================================

//==============================================================================
// BBattle::BBattle
//==============================================================================
BBattle::BBattle() :
   mID(-1),
   mPosition(cInvalidVector),
   mSize(-1.0f),
   mSizeSqr(-1.0f),
	mEndTime(0),
	mStartTime(0)
   //mHPAverageUpdated(false)
{
   //mLastHPUpdate = gWorld->getGametime();
   setDefaultFlags();
}

//==============================================================================
// BBattle::~BBattle
//==============================================================================
BBattle::~BBattle(void)
{
   //Nuke the players.
   for (long i=0; i < mPlayers.getNumber(); i++)
   {
      if (mPlayers[i] != NULL)
      {
         delete mPlayers[i];
         mPlayers[i]=NULL;
      }
   }
   mPlayers.setNumber(0);

   //Reset the units.
   mUnits.setNumber(0);

   mUnitHistory.setNumber(0);
}

//==============================================================================
// BBattle::createPlayer
//==============================================================================
BBattlePlayer* BBattle::createPlayer(long playerID)
{
   //Make sure we don't already have a BP for this player.
   if (getPlayerByID(playerID) != NULL)
      return(NULL);

   //Create.
   BBattlePlayer *newBP=new BBattlePlayer(playerID);
   if (newBP == NULL)
      return(NULL);
   //Create space in the list.
   if (mPlayers.setNumber(mPlayers.getNumber()+1) == false)
   {
      delete newBP;
      return(NULL);
   }
   //Save it.
   mPlayers[mPlayers.getNumber()-1]=newBP;

   //Done.
   return(newBP);
}

//==============================================================================
// BBattle::getPlayerByID
//==============================================================================
BBattlePlayer* BBattle::getPlayerByID(long id) const
{
   for (long i=0; i < mPlayers.getNumber(); i++)
   {
      if (mPlayers[i]->getID() == id)
         return(mPlayers[i]);
   }
   return(NULL);
}

//==============================================================================
// BBattle::getPlayerByIndex
//==============================================================================
BBattlePlayer* BBattle::getPlayerByIndex(long index) const
{
   if ((index < 0) || (index >= mPlayers.getNumber()) )
      return(NULL);
   return(mPlayers[index]);
}

//==============================================================================
// BBattle::getEnemyPlayer
//==============================================================================
BBattlePlayer* BBattle::getEnemyPlayer(long playerID) const
{
   for (long i=0; i < mPlayers.getNumber(); i++)
   {
      BPlayer *player=gWorld->getPlayer(mPlayers[i]->getID());
      if (player == NULL)
         continue;
      if (player->isEnemy(playerID) == true)
         return(mPlayers[i]);
   }
   return(NULL);
}

//==============================================================================
// BBattle::create
//==============================================================================
bool BBattle::create(const BEntityIDArray &units)
{
   //Bail.
   if (units.getNumber() <= 0)
      return(false);

   //Add the units.  This will create the players, too.  Track some stats, too.
   for (long i=0; i < units.getNumber(); i++)
   {
      //Get the unit.
      BUnit *unit=gWorld->getUnit(units[i]);
      if ((unit == NULL) || (unit->isAlive() == false))
         continue;
      //Add it.
      if (addUnit(unit, cInvalidObjectID) == false)
         continue;
   }
   //If we have no players, bail.
   if (mPlayers.getNumber() <= 0)
      return(false);

   //Done.   
   return(true);
}

//==============================================================================
// BBattle::containsUnit
//==============================================================================
bool BBattle::containsUnit(BEntityID unitID) const
{
   long unitIndex=getUnitIndexConst(unitID);
   if (unitIndex < 0)
      return(false);
   return(true);
}

//==============================================================================
// BBattle::addUnit
//==============================================================================
bool BBattle::addUnit(BUnit *unit, BEntityID targetUnitID, bool createPlayerIfNeeded)
{
   //Bomb check.
   if ((unit == NULL) || (unit->isAlive() == false))
      return(false);

   bool inOtherBattle = false;
      
   //If the unit is in another battle, remove it.
   if (unit->getBattleID() >= 0)
   {
      inOtherBattle = true;

      //Skip if it's in this battle.
      if (unit->getBattleID() == mID)
      {
         long unitIndex=getUnitIndex(unit->getID());
         if (unitIndex >= 0)
         {            
            unit->setBattleID(mID);
            //Stop any exiting.
            mUnits[unitIndex].stopExit();
            return(true);
         }
      }

      BBattle *oldBattle=gWorld->getBattleManager()->getBattleByID(unit->getBattleID());
      if (oldBattle != NULL)
      {
         //Remove it.
         if (oldBattle->removeUnit(unit->getID(), (DWORD)0, false) == false)
            return(false);
      }
   }

   //Add it.  Get the player.
   BBattlePlayer *bp=getPlayerByID(unit->getPlayerID());
   if (bp == NULL)
   {
      if (createPlayerIfNeeded == false)
         return(false);
      bp=createPlayer(unit->getPlayerID());
      if (bp == NULL)
         return(false);
   }
   
   //Add it to the player.
   if (bp->addUnit(unit->getID()) == false)
      return(false);

   //Add the unit entry.
   long newIndex=mUnits.getNumber();
   if (mUnits.setNumber(newIndex+1) == false)
   {
      bp->removeUnit(unit->getID());
      return(false);
   }
   mUnits[newIndex].reset();
   mUnits[newIndex].setUnitID(unit->getID());
   mUnits[newIndex].setTargetUnitID(targetUnitID);
   mUnits[newIndex].setPlayerID(bp->getID());
   //Set the proper battle ID into the unit.
   unit->setBattleID(mID);
   
   //-- See who wants to react to us joining the battle?
   bool wasInBattlePrevious = false;
   if(inOtherBattle == false)
   {
      //-- Only create join reactions if we've never been in this particular battle before.
      uint temp;
      wasInBattlePrevious = mUnitHistory.find(unit->getParentID(), temp);
      if(!wasInBattlePrevious)
      {
         //-- Only use the 0th unit to cause a battle join alert
         BSquad *pSquad = unit->getParentSquad();
         if(pSquad)
         {
            if(pSquad->getChild(0) == unit->getID())
            {
               pSquad->createAudioReaction(cSquadSoundChatterReactJoinBattle);
            }
         }
      }
   }

   //-- Track our battle's unit history
   if(wasInBattlePrevious == false && unit->getParentID() != cInvalidObjectID)
      mUnitHistory.add(unit->getParentID());

   
   //Done.
   return(true);
}

//==============================================================================
// BBattle::removeUnit
//==============================================================================
bool BBattle::removeUnit(BEntityID unitID, DWORD exitTime, bool overwrite)
{
   //Get the unit.
   BUnit *unit=gWorld->getUnit(unitID);
   //If the unit is valid, make sure the unit is in this battle.
   if ((unit != NULL) && (unit->getBattleID() != mID))
      return(false);

   //Get our index.
   long unitIndex=getUnitIndex(unitID);
   if (unitIndex < 0)
      return(false);

   //If this unit is given an exit time, just set that and return (as we're not
   //really removing it right away.
   if (exitTime > (DWORD)0)
   {
      //If this unit is already exiting and we're not overwriting, skip.
      if ((mUnits[unitIndex].getFlagExiting() == true) && (overwrite == false))
         return(true);
      mUnits[unitIndex].startExit(exitTime);
      return(true);
   }

   //Remove it from the player.
   BBattlePlayer *bp=getPlayerByID(mUnits[unitIndex].getPlayerID());
   if (bp != NULL)
      bp->removeUnit(unitID);
   //Remove it from our list.
   mUnits.removeIndex(unitIndex);

   //Set the unit's battle to -1.
   if (unit != NULL)
      unit->setBattleID(-1);
   
   //Done.
   return(true);
}

//==============================================================================
// BBattle::stopUnits
//==============================================================================
void BBattle::stopUnits(bool cheer)
{  
   bool didCheer = false;

//-- FIXING PREFIX BUG ID 2559
   const BBattlePlayer* pPlayer=NULL;
//--
   for(uint i = 0; i < mPlayers.getSize(); i++)
   {
      pPlayer = mPlayers[i];
      if(!pPlayer)
         continue;

      //-- Does this player haver units in the battle? / Remove the unit from the battle      
      for(long j = 0; j < pPlayer->getNumberUnits(); j++)
      {
         BUnit* pUnit=gWorld->getUnit(pPlayer->getUnitID(j));
         if (pUnit == NULL)
            continue;

         //-- remove the unit from the battle.
		   pUnit->setBattleID(-1);         

         // Cheer, only have a single squad from the winning battle do a cheer, in this case the first dude.
         
         if (cheer && !didCheer)
         {
//-- FIXING PREFIX BUG ID 2557
            const BSquad* pSquad = pUnit->getParentSquad();
//--
            if (pSquad && (pSquad->getLeaderUnit() == pUnit))
            {
               didCheer = gWorld->createPowerAudioReactions(pSquad->getPlayerID(), cSquadSoundChatterCheer, pSquad->getPosition(), pSquad->getLOS(), pSquad->getID());               
            }
         }
      }
   }
}

//==============================================================================
// BBattle::update
//==============================================================================
bool BBattle::update()
{
	//Check for first update.
   firstUpdate();   

   //Check on our units.
   long firstPlayerID=-1;
   bool alliedPlayers=true;
   bool noTargets=true;
   for (long i=0; i < mUnits.getNumber(); i++)
   {
      //Update the unit.
      mUnits[i].update(this);
      //If we're supposed to nuke it, do so. 
      if (mUnits[i].getFlagDestroy() == true)
      {
         //Remove it.
         if (removeUnit(mUnits[i].getUnitID(), (DWORD)0, false) == true)
            i--;
         continue;
      }
      
      //If this unit is requesting to be split out, add it's ID to the battle manager's split list.
      if (mUnits[i].getFlagSplit() == true)
         gWorld->getBattleManager()->addUnitToSplit(mUnits[i].getUnitID());

      //Track player data.
      BPlayer *player=gWorld->getPlayer(mUnits[i].getPlayerID());
      if (firstPlayerID < 0)
         firstPlayerID=mUnits[i].getPlayerID();
      else if ((player != NULL) && (player->isAlly(firstPlayerID) == false))
         alliedPlayers=false;

      //See if it has a target.
      if (mUnits[i].getTargetUnitID() != cInvalidObjectID)
         noTargets=false;                     
   }   
   //-- Update the players
   for (long i=0; i < mPlayers.getNumber(); i++)
   {
      mPlayers[i]->update(this);
   }

   //If we have all allied players and everyone w/o a target, we're done.
   if ((alliedPlayers == true) && (noTargets == true))
   {
      stopUnits(true);
      mFlagDone = true;
		mEndTime = gWorld->getGametime();
      return(true);
   }
	
   //Done.
   return(true);
}

//==============================================================================
// BBattle::updateParameters
//==============================================================================
void BBattle::updateParameters(void)
{
   //Find our centroid.
   BVector centroid(0.0f);
   long count=0;
   for (long i=0; i < mUnits.getNumber(); i++)
   {
      BUnit *unit=gWorld->getUnit(mUnits[i].getUnitID());
      if (unit != NULL)
      {
         count++;
         centroid+=unit->getPosition();
      }
      BUnit *targetUnit=gWorld->getUnit(mUnits[i].getTargetUnitID());
      if (targetUnit != NULL)
      {
         count++;
         centroid+=targetUnit->getPosition();
      }
   }
   if (count > 0)
   {
      centroid/=(float)count;
      mPosition=centroid;
   }

   //Now, go back to figure out our size (radius).
   /*mSize=0.0f;
   for (i=0; i < mUnits.getNumber(); i++)
   {
      BUnit *unit=gWorld->getUnit(mUnits[i].getUnitID());
      if (unit != NULL)
      {
         float unitDistance=unit->getPosition().xzDistance(mPosition);
         if (unitDistance > mSize)
            mSize=unitDistance;
      }
      BUnit *targetUnit=gWorld->getUnit(mUnits[i].getTargetUnitID());
      if (targetUnit != NULL)
      {
         float targetUnitDistance=targetUnit->getPosition().xzDistance(mPosition);
         if (targetUnitDistance > mSize)
            mSize=targetUnitDistance;
      }
   }*/
   mSize=gWorld->getBattleManager()->getBattleSize();
   mSizeSqr=mSize*mSize;
}   

//==============================================================================
// BBattle::render
//==============================================================================
bool BBattle::render(void)
{
   //Draw our position.
   gTerrainSimRep.addDebugLineOverTerrain(mPosition, mPosition, cDWORDWhite, cDWORDWhite, 1.0f);
   //Draw our size.
   gTerrainSimRep.addDebugCircleOverTerrain(mPosition, mSize, cDWORDRed, 1.0f);

   //Render units.
   for (long i=0; i < mUnits.getNumber(); i++)
   {
      BUnit *unit=gWorld->getUnit(mUnits[i].getUnitID());
      if (unit == NULL)
         continue;
      DWORD foo=cDWORDWhite;
      switch (i%4)
      {
         case 0:     foo=cDWORDBlue;      break;
         case 1:     foo=cDWORDYellow;    break;
         case 2:     foo=cDWORDMagenta;   break;
         case 3:     foo=cDWORDCyan;      break;
      }
      gTerrainSimRep.addDebugLineOverTerrain(mPosition, unit->getPosition(), cDWORDCyan, cDWORDCyan, 1.0f);
      
      //Target.
      BUnit *targetUnit=gWorld->getUnit(mUnits[i].getTargetUnitID());
      if (targetUnit != NULL)
         gTerrainSimRep.addDebugLineOverTerrain(unit->getPosition(), targetUnit->getPosition(), cDWORDRed, cDWORDRed, 1.0f);
   }

   return(true);
}

//==============================================================================
// BBattle::debug
//==============================================================================
void BBattle::debug(char* v, ...)
{
   #ifdef FINAL_RELEASE
   return;
   #else
   static char out[BLogManager::cMaxLogTextLength];
   va_list va;
   va_start(va, v);
   bvsnprintf(out, sizeof(out), v, va);
   static char out2[BLogManager::cMaxLogTextLength*2];
   bsnprintf(out2, sizeof(out2), "BATTLE #%5d: %s", mID, out);
   gConsole.output(cMsgDebug, out2);
   #endif
}

//==============================================================================
// BBattle::getText
//==============================================================================
void BBattle::getText(BSimString& text) const
{
	/*text.format(("ID: %d, Pos(%.2f, %.2f), Size: %.2f, MU: %d, TW: %d, PW: %d, Awarded: %.2f, LUT: %d"), 
		mID,
		mPosition.x, mPosition.z,
		mSize,
		mUnits.getSize(),		
		mEndTime);*/
}

//==============================================================================
// BBattle::firstUpdate
//==============================================================================
bool BBattle::firstUpdate(void)
{
   //We only do this once.
   if (mFlagFirstUpdate == false)
      return(false);

   //Update parms.
   updateParameters();

   //Done.
   mFlagFirstUpdate = false;

	mStartTime = gWorld->getGametime();
	return(true);
}

//==============================================================================
// BBattle::getUnitIndexConst
//==============================================================================
long BBattle::getUnitIndexConst(BEntityID unitID) const
{
   for (long i=0; i < mUnits.getNumber(); i++)
   {
      if (mUnits[i].getUnitID() == unitID)
         return(i);
   }
   return(-1);
}

//==============================================================================
// BBattle::getUnitIndex
//==============================================================================
long BBattle::getUnitIndex(BEntityID unitID)
{
   for (long i=0; i < mUnits.getNumber(); i++)
   {
      if (mUnits[i].getUnitID() == unitID)
         return(i);
   }
   return(-1);
}

//==============================================================================
// BBattle::setDefaultFlags
//==============================================================================
void BBattle::setDefaultFlags(void)
{
   mFlagFirstUpdate = true;
   mFlagDone = false;
}

//==============================================================================//==============================================================================

//==============================================================================
// BBattleManager::BBattleManager
//==============================================================================
BBattleManager::BBattleManager(void) :
mNextBattleID(0),
mBattleExitTime((DWORD)5000),
mBattleSize(150.0f),
mBattleSizeSqr(mBattleSize*mBattleSize),
mpBattleManagerListener(NULL)

{
}

//==============================================================================
// BBattleManager::~BBattleManager
//==============================================================================
BBattleManager::~BBattleManager(void)
{
   cleanUp();
}

//==============================================================================
// BBattleManager::init
//==============================================================================
bool BBattleManager::init(void)
{
   //Clean up.
   cleanUp();

   //Read the file.
   readXML(L"battle.xml", cDirData);


   return(true);
}

//==============================================================================
// BBattleManager::createBattle
//==============================================================================
BBattle* BBattleManager::createBattle(void)
{
   //Create the battle.
   BBattle *newBattle=new BBattle();
   if (newBattle == NULL)
      return(NULL);
   newBattle->setID(mNextBattleID);

   //Create space.
   long newIndex=mBattles.getNumber();
   if (mBattles.setNumber(newIndex+1) == false)
   {
      delete newBattle;
      return(NULL);
   }
   //Save it.
   mBattles[newIndex]=newBattle;

   //Increment the ID.
   mNextBattleID++;

   MVinceEventSync_BattleStarted();

   //Done.
   return(mBattles[newIndex]);
}

//==============================================================================
// BBattleManager::createBattle
//==============================================================================
BBattle* BBattleManager::createBattle(const BEntityIDArray &units)
{
   //Create the battle.
   BBattle *newBattle=new BBattle();
   if (newBattle == NULL)
      return(NULL);
   newBattle->setID(mNextBattleID);

   //Create it.
   if (newBattle->create(units) == false)
   {
      delete newBattle;
      return(NULL);
   }

   //Create space.
   long newIndex=mBattles.getNumber();
   if (mBattles.setNumber(newIndex+1) == false)
   {
      delete newBattle;
      return(NULL);
   }
   //Save it.
   mBattles[newIndex]=newBattle;

   //Increment the ID.
   mNextBattleID++;

   MVinceEventSync_BattleStarted();

   //Done.
   return(mBattles[newIndex]);
}

//==============================================================================
// BBattleManager::getBattleByID
//==============================================================================
BBattle* BBattleManager::getBattleByID(long id) const
{
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      if (mBattles[i] == NULL)
         continue;
      if (mBattles[i]->getID() == id)
         return(mBattles[i]);
   }
   return(NULL);
}

//==============================================================================
// BBattleManager::getBattleByIndex
//==============================================================================
BBattle* BBattleManager::getBattleByIndex(long index) const
{
   if ((index < 0) || (index >= mBattles.getNumber()) )
      return(NULL);
   return(mBattles[index]);
}

//==============================================================================
// BBattleManager::destroyBattle
//==============================================================================
bool BBattleManager::destroyBattle(long id)
{
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      if (mBattles[i] == NULL)
         continue;
      if (mBattles[i]->getID() == id)
      {
         delete mBattles[i];
         mBattles.removeIndex(i);
         return(true);
      }
   }
   return(false);
}

//==============================================================================
// BBattleManager::destroyAllBattles
//==============================================================================
void BBattleManager::destroyAllBattles(void)
{
   //Battles.
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      if (mBattles[i] != NULL)
      {
         delete mBattles[i];
         mBattles[i]=NULL;
      }
   }
   mBattles.setNumber(0);
   mNextBattleID=0;

}

//=============================================================================
// BBattleManager::findBattle
//=============================================================================
BBattle* BBattleManager::findBattle(const BUnit *unit, const BUnit *targetUnit)
{
   //Bomb check.
   if ((unit == NULL) || (targetUnit == NULL))
      return(NULL);

   //Setup our distance trackers.
   BBattle *closestBattles[3];
   memset(closestBattles, 0, sizeof(BBattle*)*3);
   float closestBattleDistances[3];
   closestBattleDistances[0]=cMaximumFloat;
   closestBattleDistances[1]=cMaximumFloat;
   closestBattleDistances[2]=cMaximumFloat;
   BVector positions[3];
   positions[0]=unit->getPosition()+targetUnit->getPosition();
   positions[0]/=2.0f;
   positions[1]=unit->getPosition();
   positions[2]=targetUnit->getPosition();

   //Find the closest battles in all categories.
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      //Get the battle.
      BBattle *battle=mBattles[i];
      if (battle == NULL)
      {
         BASSERT(0);
         continue;
      }
      //If the battle is on it's first update, update it's parameters so we get a reasonably
      //accurate position.
      if (battle->getFlagFirstUpdate() == true)
         battle->updateParameters();

      //Check distances.
      for (long j=0; j < 3; j++)
      {
         float tempDistance=battle->getPosition().xzDistanceSqr(positions[j]);
         if ((closestBattles[j] == NULL) || (tempDistance < closestBattleDistances[j]))
         {
            closestBattles[j]=mBattles[i];
            closestBattleDistances[j]=tempDistance;
         }
      }
   }

   //Take the first battle in the list (if we have any).
   for (long i=0; i < 3; i++)
   {
      if (closestBattles[i] != NULL)
      {
         //If the closest battle is close enough, return that.
         closestBattleDistances[i]=float(sqrt(closestBattleDistances[i]));
         if (closestBattleDistances[i] <= mBattleSize)
            return(closestBattles[i]);
      }
   }

   //Else, return nothing.
   return(NULL);
}

//=============================================================================
// BBattleManager::findNearestBattle
//=============================================================================
BBattle* BBattleManager::findNearestBattle( BVector location)
{
   float nearestBattleDist = cMaximumFloat;
   BBattle *nearestBattle = NULL;
   
    //Find the closest battle
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      //Get the battle.
      BBattle *battle=mBattles[i];
      if (battle == NULL)
      {
         BASSERT(0);
         continue;
      }
      //If the battle is on it's first update, update it's parameters so we get a reasonably
      //accurate position.
      if (battle->getFlagFirstUpdate() == true)
         battle->updateParameters();

      //Check distances.
      for (long j=0; j < 3; j++)
      {
         float tempDistance=battle->getPosition().xzDistanceSqr(location);
         if(tempDistance < nearestBattleDist)
         {
            nearestBattle = battle;
         }         
      }
   }

   return(nearestBattle);
}

//=============================================================================
// BBattleManager::removeUnitFromBattle
//=============================================================================
bool BBattleManager::removeUnitFromBattle(BUnit *unit, long battleID)
{
   BBattle *b=getBattleByID(battleID);
   if (b != NULL)
   {      
      if(unit->isAlive())
         return (b->removeUnit(unit->getID(), getBattleExitTime(), false));
      else
         return(b->removeUnit(unit->getID(), (DWORD)0, false));
   }
   return(false);
}

//=============================================================================
// BBattleManager::addUnitToSplit
//=============================================================================
void BBattleManager::addUnitToSplit(BEntityID unitID)
{
   mUnitsToSplit.uniqueAdd(unitID);
}

//=============================================================================
// BBattleManager::updateBattles
//=============================================================================
void BBattleManager::updateBattles()
{
   //Update our battles.
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      BBattle *battle=mBattles[i];
      if (battle == NULL)
         continue;
      battle->update();
      if (battle->getFlagDone() == true)
      {
         mBattles.removeIndex(i);
         delete battle;
         i--;
      }
   }

   //Handle split units.
   for (long i=0; i < mUnitsToSplit.getNumber(); i++)
   {
      //Get the unit.
      BUnit *unit=gWorld->getUnit(mUnitsToSplit[i]);
      if (unit == NULL)
         continue;
#ifdef DEBUGBATTLEMANAGER
      debug("SplitUnit %d (%s):", unit->getID(), unit->getName());
#endif

      //Get the target..
      BUnit *targetUnit=gWorld->getUnit(unit->getAttackTargetID());
#ifdef DEBUGBATTLEMANAGER
      if (targetUnit != NULL)
         debug("  TargetUnit is %d (%s).", targetUnit->getID(), targetUnit->getName());
      else
         debug("  TargetUnit is NULL.");
#endif

      //If we have no target unit, we really just want to remove us from our current
      //battle.
      if (targetUnit == NULL)
      {
#ifdef DEBUGBATTLEMANAGER
         debug("    Removing us from our BID=%d.", unit->getBattleID());
#endif
         BBattle *b=getBattleByID(unit->getBattleID());
         if (b != NULL)
            b->removeUnit(unit->getID(), (DWORD)0, false);
         continue;
      }

      //Sanity check #2.  If the target is in the same battle, that target must also
      //be in this list.
      if (targetUnit->getBattleID() == unit->getBattleID())
      {
         if (mUnitsToSplit.contains(targetUnit->getID()) == false)
         {
#ifdef DEBUGBATTLEMANAGER
            debug("  Target unit is in same battle, but not in the units to split.");
#endif
            continue;
         }
      }

      //So, we're good to put this unit into a new battle.  First, if the
      //target is in a different battle, put the unit in there.
      BBattle *battle=NULL;
      if ((targetUnit->getBattleID() >= 0) &&
         (targetUnit->getBattleID() != unit->getBattleID()) )
      {
#ifdef DEBUGBATTLEMANAGER
         debug("  Getting target unit's BID=%d.", targetUnit->getBattleID());
#endif
         battle=getBattleByID(targetUnit->getBattleID());
      }
      //Else, try to find a battle for us to join.
      if (battle == NULL)
      {
#ifdef DEBUGBATTLEMANAGER
         debug("  Finding Battle.");
#endif
         battle=findBattle(unit, targetUnit);
      }
      //Else, create a battle.
      if (battle == NULL)
      {
#ifdef DEBUGBATTLEMANAGER
         debug("  Creating Battle.");
#endif
         battle=createBattle();
      }

      //If we found a valid battle, add us.
      if (battle != NULL)
      {
#ifdef DEBUGBATTLEMANAGER
         debug("  Adding Unit to BID=%d.", battle->getID());
#endif
         battle->addUnit(unit, targetUnit->getID());
      }
   }
   //Reset our split units.
   mUnitsToSplit.setNumber(0);
}

//==============================================================================
// BBattleManager::render
//==============================================================================
void BBattleManager::render(void)
{
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      if (mBattles[i] != NULL)
         mBattles[i]->render();
   }
}

//==============================================================================
// BBattleManager::debug
//==============================================================================
void BBattleManager::debug(char* v, ...)
{
#ifdef FINAL_RELEASE
   return;
#else
   static char out[BLogManager::cMaxLogTextLength];
   va_list va;
   va_start(va, v);
   bvsnprintf(out, sizeof(out), v, va);
   static char out2[BLogManager::cMaxLogTextLength*2];
   bsnprintf(out2, sizeof(out2), "BATTLEManager: %s", out);
   gConsole.output(cMsgDebug, out2);
#endif
}


//==============================================================================
// BBattleManager::readXML
//==============================================================================
bool BBattleManager::readXML(const BSimString &filename, long directoryID)
{
   mFilename=filename;

   //Parse it.
   BXMLReader reader;
   bool ok=reader.load(directoryID, mFilename); //XML_READER_LOAD_DISCARD_ON_CLOSE);
   if (!ok)
   {
      BASSERTM(0, "Could not find battle.xml file.");
      {setBlogError(0000); blogerror("Error loading %s during BBattleManager::init.", mFilename.getPtr());}
      return(false);
   }
   //Get root node.
   BXMLNode root(reader.getRootNode());
   if ((!root.getValid()) || (root.getName().compare(B("Battles")) != 0))
   {
      BASSERTM(0, "Could not parse battle.xml file.");
      {setBlogError(0000); blogerror("Error parsing %s during BBattleManager::init.", mFilename.getPtr());}
      return(false);
   }

   //Go through the entries.
   for (long i=0; i < root.getNumberChildren(); i++)
   {
      //Get child node.
      BXMLNode node(root.getChild(i));
            
      //Battle Exit Time.
      if (node.getName().compare(B("BattleExitTime")) == 0)
      {
         long time=1;
         node.getTextAsLong(time);
         mBattleExitTime=(DWORD)time;
      }
      //Battle Size.
      else if (node.getName().compare(B("BattleSize")) == 0)
      {
         node.getTextAsFloat(mBattleSize);
         mBattleSizeSqr=mBattleSize*mBattleSize;
      }            
   }

   return(true);
}

//==============================================================================
// BBattleManager::cleanUp
//==============================================================================
void BBattleManager::cleanUp(void)
{
   //Battles.
   for (long i=0; i < mBattles.getNumber(); i++)
   {
      if (mBattles[i] != NULL)
      {
         delete mBattles[i];
         mBattles[i]=NULL;
      }
   }
   mBattles.setNumber(0);
   mNextBattleID=0;
}

//==============================================================================
// BBattleManager::unitEnteredCombat
//==============================================================================
void BBattleManager::unitEnteredCombat(BEntityID unitID, BEntityID targetID)
{
   BUnit* unit = gWorld->getUnit(unitID);
   if(unit == NULL)
      return;

   //Find us a battle to create/join.
   BUnit *targetUnit=gWorld->getUnit(targetID);
   BBattleManager *bm = gWorld->getBattleManager();
   if ((targetUnit != NULL) && (bm != NULL))
   {
      BBattle *battle=NULL;
      if (unit->getBattleID() >= 0)   
         battle=bm->getBattleByID(unit->getBattleID());
      else if (targetUnit->getBattleID() >= 0)
         battle=bm->getBattleByID(targetUnit->getBattleID());
      else
      {
         battle=bm->findBattle(unit, targetUnit);
         if (battle == NULL)
            battle=bm->createBattle();
      }

      //Add us to the battle.
      if (battle != NULL)
      {
         battle->addUnit(unit, targetUnit->getID());         
      }
   }
   //Remove us from the battle.
   else if ((targetID == cInvalidObjectID) && (unit != NULL) && (bm != NULL))
   {
      BBattle *battle=bm->getBattleByID(unit->getBattleID());
      if (battle != NULL)
         battle->removeUnit(unit->getID(), bm->getBattleExitTime(), false);
   }
}