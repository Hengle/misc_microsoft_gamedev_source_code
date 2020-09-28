//==============================================================================
// musicManager.cpp
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "common.h"
#include "musicManager.h"
#include "world.h"
#include "battle.h"
#include "soundmanager.h"
#include "config.h"
#include "configsgame.h"
#include "user.h"
#include "usermanager.h"
#include "camera.h"
#include "render.h"
#include "tactic.h"
#include "gamesettings.h"
#include "unitquery.h"

//==============================================================================
const static DWORD cNoBattleTimer = 10000; //-- 10 seconds
const static DWORD cWaitToAssessBattle = 6000; //-- 6 seconds

//==============================================================================
//==============================================================================
BMusicManager::BMusicManager(void)
{   
   reset();   
}

//==============================================================================
//==============================================================================
BMusicManager::~BMusicManager(void)
{
   reset();
}

//==============================================================================
//==============================================================================
void BMusicManager::reset(void)
{
   mModeLockedTimer = 0;
   mModeSetTime = 0;
   mCurrentState = cStateNone;
   mQueuedState = cStateNone;

   mStandardEverSet = false;

   mGameStartDelay = false;

   mMaxTargetDist = 0.0f;   

   mUpdateCount = 0;
   mTargetsWhichNeedsToBeDestroyed.clear();
   
   mDisabled = false;
   mLoadedFromSavegame = false;

   //-- Wait and see stuff
   mBattleEndedTimer = 0;
   mBattleEndedTime = 0;
   mQueuedBattleEndedState = BMusicManager::cStateNone;

   //-- Turn ourself off for campaign games.
   long gameType;
//-- FIXING PREFIX BUG ID 3412
   const BGameSettings* pSettings=gDatabase.getGameSettings();
//--
   if(pSettings)
   {
      pSettings->getLong(BGameSettings::cGameType, gameType);
      if(gameType == BGameSettings::cGameTypeCampaign)
         mDisabled = true;
   }
}


//==============================================================================
//==============================================================================
void BMusicManager::update(void)
{
   if(mGameStartDelay == true)
      return;

   mUpdateCount++;

   if(isEnabled() == false)
      return;

   if( !mLoadedFromSavegame )
   {
      if(mUpdateCount == 2)
      {
         //-- Start Music
         if (!gConfig.isDefined(cConfigNoMusic))
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicStopPreGame);
         setNewState(cStateWorldIntro, true, 1000);      
         if (!gConfig.isDefined(cConfigNoMusic))
            gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicPlayInGame);
      }
      else if(mUpdateCount == 3)
      {
         setNewState(cStateStandard, false);
      }
   }

   //-- If there is a state queued, see if it's ready to go
   if(mQueuedState != cStateNone)
   {
      setNewState(mQueuedState);
   }   

   if(mCurrentState == cStateBattle1 && mQueuedState == cStateNone && mQueuedBattleEndedState == cStateNone)
   {
      //-- Did we just get into this state?
      if(mModeSetTime + cWaitToAssessBattle < gWorld->getGametime())
      {
         int32 battleState = getBattleStatus();
         if(battleState == cStateBattleWon)
         {
            setNewState(cStateBattleWon, true);
            setNewState(cStateStandard);
            mTargetsWhichNeedsToBeDestroyed.clear();

            //-- Reset battle ended stuff 
            mQueuedBattleEndedState = cStateNone;
            mBattleEndedTime = gWorld->getGametime();
         }

         //-- Set the timer to reeval and make sure the battle is actually over.
         if(battleState == cStateBattleLost && mQueuedBattleEndedState == cStateNone)
         {
            mQueuedBattleEndedState = cStateBattleLost;
            mBattleEndedTime = gWorld->getGametime();
         }
      }
   }

   //-- Check to see if we need to pop a win/loss
   if(mQueuedBattleEndedState != BMusicManager::cStateNone)
   {
      checkBattleEndedTimer();
   }
}

//==============================================================================
//==============================================================================
int32 BMusicManager::getBattleStatus()
{
   if(isEnabled() == false)
      return mCurrentState;


   //-- We're in a battle, did the things we need to kill die?    
   BUnit* pUnit = NULL;
   BUnit* pValidUnit = NULL;
   for(int i=mTargetsWhichNeedsToBeDestroyed.getNumber()-1; i >= 0; i--)
   {           
      pUnit = gWorld->getUnit(mTargetsWhichNeedsToBeDestroyed[i]);
      if(!pUnit || pUnit->isAlive() == false)
         mTargetsWhichNeedsToBeDestroyed.removeIndex(i);
      else if(pUnit && !pValidUnit)
         pValidUnit = pUnit;
   }

   if(mTargetsWhichNeedsToBeDestroyed.getSize() == 0)
      return cStateBattleWon;

   //-- Its still alive, did we lose?
   
//-- FIXING PREFIX BUG ID 3414
   const BSquad* pSquad = NULL;
//--
   if(pValidUnit)
      pSquad = pValidUnit->getParentSquad();

   if(!pSquad)
      return cStateBattleLost;

   if(pSquad->getLastDamagedTime() + cNoBattleTimer < gWorld->getGametime())
   {
      bool lostBattle = false;

      //-- It hasn't been attacked for a while, is there at least fighting going on nearby with us in it?
//-- FIXING PREFIX BUG ID 3413
      const BBattle* pBattle = gWorld->getBattleManager()->findNearestBattle(pSquad->getPosition());
//--
      if(pBattle == NULL)
      {
         lostBattle = true;
      }
      //-- Is the battle close enough to consider the fighting still going on?
      else if(pSquad->calculateXZDistance(pBattle->getPosition()) > gWorld->getBattleManager()->getBattleSize())
      {
         lostBattle = true;
      }               

      if(lostBattle)
      {
        return cStateBattleLost;
      }
   }

   return mCurrentState;
}

//==============================================================================
//==============================================================================
bool BMusicManager::loadXML(BXMLNode root)
{
   int32 battleSev;     
   bool result;

   int32 modeIndex = cStateNone;

   int32 nodeCount=root.getNumberChildren();
   for(int32 i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName()); 
      if(name=="BattleMusicCue")
      {  
         result = node.getAttribValueAsInt32("sev", battleSev);
         if(!result)
            continue;
         int32 index = cBattleStart + battleSev-1;
         if(index < 0 || index >= cMusicStateMax)
            continue;
         
         //-- Commanded Battle Values         
         result = node.getAttribValueAsFloat("commandedAllyValue", mMusicState[index].mCommandedAllyCombatValue);
         if(!result)
            continue;
         result = node.getAttribValueAsFloat("commandedEnemyValue", mMusicState[index].mCommandedEnemyCombatValue);
         if(!result)
            continue;


         long numChildren = node.getNumberChildren();
         for(long j=0; j < numChildren; j++)
         {
            BXMLNode child = node.getChild(j);       
            if(child.getName() == "Cue")
            {
               BSimString str;
               child.getText(str);
               mMusicState[index].mSoundCues.add(gSoundManager.getCueIndex(str));
            }
         }

         //-- Shuffle the sound cues
         uint numCues = mMusicState[index].mSoundCues.getSize();
         for(uint j=0; j < numCues; j++)
         {
            BCueIndex temp = mMusicState[index].mSoundCues[j];
            uint swapIndex = getRandRange(cSoundRand, 0, numCues-1);
            mMusicState[index].mSoundCues[j] = mMusicState[index].mSoundCues[swapIndex];
            mMusicState[index].mSoundCues[swapIndex] = temp;
         }

         modeIndex = i;
      }
      else if(name=="StandardCue")
      {
         modeIndex = cStateStandard;
         BSimString tempStr;
         mMusicState[modeIndex].mSoundCues.add(gSoundManager.getCueIndex(node.getTextPtr(tempStr)));
         
      }
      else if(name == "Battle")
      {
         BSimString tempStr;
         mMusicState[cStateBattle].mSoundCues.add(gSoundManager.getCueIndex(node.getTextPtr(tempStr)));
         modeIndex = cStateBattleWon;
      }
      else if(name=="BattleWon")
      {
         BSimString tempStr;
         mMusicState[cStateBattleWon].mSoundCues.add(gSoundManager.getCueIndex(node.getTextPtr(tempStr)));
         modeIndex = cStateBattleWon;
      }
      else if(name=="BattleLost")
      {
         modeIndex = cStateBattleLost;
         BSimString tempStr;
         mMusicState[modeIndex].mSoundCues.add(gSoundManager.getCueIndex(node.getTextPtr(tempStr)));
         
      }
      else if(name=="WorldIntro")
      {
         modeIndex = cStateWorldIntro;
         BSimString tempStr;
         mMusicState[modeIndex].mSoundCues.add(gSoundManager.getCueIndex(node.getTextPtr(tempStr)));         
      }
      else if(name=="MaxTargetDist")
      {
         node.getTextAsFloat(mMaxTargetDist);
      }        
      else if (name == "BattleEndedWaitTimer")
      {
         node.getTextAsDWORD(mBattleEndedTimer);
      }

      //-- Load switch timer if it is specified
      if(modeIndex != cStateNone)
      {
         DWORD switchTimer=0;
         bool result = node.getAttribValueAsDWORD("switchTimer", switchTimer);
         if(result)  
            mMusicState[modeIndex].mLockedToModeTimer = switchTimer;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void  BMusicManager::squadsCommandedToAttack(const BEntityIDArray& squads, BPlayerID playerID, BVector armyLocation, BEntityID targetID)
{

    if(isEnabled() == false)
      return;

   if(playerID != gUserManager.getPrimaryUser()->getPlayerID())
      return;

   //-- Need at least 2 or more units to trigger the music
   if(squads.getSize() <= 1)
      return;

   //-- Determine the number combat value of the squads commanded to attack
   float commandedCombatValue = 0.0f;
   for (uint squadIdx=0; squadIdx<squads.getSize(); squadIdx++)
   {
//-- FIXING PREFIX BUG ID 3418
      const BSquad* pSquad = gWorld->getSquad(squads[squadIdx]);
//--
      if(pSquad)
      {         
//-- FIXING PREFIX BUG ID 3417
         const BProtoAction* pAction = pSquad->getProtoActionForTarget(targetID, cInvalidVector, -1, false);
//--
            if(!pAction)
               continue;           

         if(pAction->getActionType() == BAction::cActionTypeSquadAttack || pAction->getActionType() == BAction::cActionTypeUnitRangedAttack)
         {
            commandedCombatValue += pSquad->getCombatValue();
         }
      }
   }   

   if(commandedCombatValue == 0)
      return;


   bool targetTriggersBattleMusic=false;

   //-- Determine enemy battle value
   float enemyCombatValue = 0.0f;

   BPlayerID enemyPlayerID = -1;
   BVector enemyLocation = cInvalidVector;
   float LOS = 0.0f;
   BEntityID entityID = cInvalidObjectID;

//-- FIXING PREFIX BUG ID 3419
   const BUnit* pTargetUnit = NULL;
//--
   if(targetID.getType() == BEntity::cClassTypeSquad)
   {
      BSquad* pSquad = gWorld->getSquad(targetID);
      if(pSquad)
       pTargetUnit =  gWorld->getUnit(pSquad->getChild(0));  
   }
   else
      pTargetUnit = gWorld->getUnit(targetID);

   if(pTargetUnit)
   {
      targetTriggersBattleMusic = pTargetUnit->getProtoObject()->getFlagTriggersBattleMusicWhenAttacked();
      if(!targetTriggersBattleMusic)
      {
         //-- See if the our associated base triggers the battle music?
         BEntityID associatedBaseID = pTargetUnit->getAssociatedBase();
         if(associatedBaseID != cInvalidObjectID)
         {
            BUnit* pAssociatedBase = gWorld->getUnit(associatedBaseID);
            if(pAssociatedBase && pAssociatedBase->getProtoObject()->getFlagTriggersBattleMusicWhenAttacked())
            {
               pTargetUnit = pAssociatedBase;
               targetTriggersBattleMusic = true;
            }
         }
      }

      enemyPlayerID = pTargetUnit->getPlayerID();  
      enemyLocation = pTargetUnit->getPosition();
      LOS = pTargetUnit->getLOS();
      
      entityID = pTargetUnit->getID();
   }

   //-- Make sure that the target is one which is flagged to start the battle music
   if(!targetTriggersBattleMusic)
      return;

   //-- If the target is too far from the camera, then ignore it
   if(armyLocation.xzDistance(enemyLocation) > mMaxTargetDist)
      return;

   if(enemyPlayerID != -1 && enemyLocation != cInvalidVector)
   {      
      enemyCombatValue = gWorld->getAreaCombatValue(enemyLocation, LOS, enemyPlayerID, cRelationTypeAlly);
   }

   //-- Determine what severity this calls for
   int32 desiredBattleSev = -1;
   for (uint i =cBattleStart; i < cBattleEnd; i++)
   {
      if(enemyCombatValue >= mMusicState[i].mCommandedEnemyCombatValue && 
         commandedCombatValue >= mMusicState[i].mCommandedAllyCombatValue)
      {         
         desiredBattleSev = i;
      }
   }

   if(desiredBattleSev != -1)
   {
      if(desiredBattleSev > mCurrentState)
      {
         //-- Battle music time!
         setNewState(desiredBattleSev);

         //-- Find all the units in the area which need to die along with this one
         BUnit* pTarget = gWorld->getUnit(entityID);
         if(!pTarget) {BASSERT(0);return;}

         BUnitQuery query(pTarget->getPosition(), 75.0f, true);        
         query.setRelation(playerID, cRelationTypeEnemy);
         BEntityIDArray results;
         gWorld->getSquadsInArea(&query, &results, false);
         uint numResults = results.getSize();
         BSquad* pSquad = NULL;
         for (uint i = 0; i < numResults; ++i)
         {
            pSquad = gWorld->getSquad(results[i]);
            if (!pSquad)
               continue;

            const BEntityIDArray& children = pSquad->getChildList();
            uint numChildren = children.getSize();

            BUnit* pUnit = NULL;
            for(uint j = 0; j < numChildren; ++j)
            {
               pUnit = gWorld->getUnit(children[j]);
               if(!pUnit)
                  continue;
               if(pUnit->getID() == entityID)
                  continue;
               if(!pUnit->getProtoObject())
                  continue;
               if(!pUnit->getProtoObject()->getFlagTriggersBattleMusicWhenAttacked())
                  continue;

               //-- If we got there, then we need to kill this unit as well.
               mTargetsWhichNeedsToBeDestroyed.uniqueAdd(pUnit->getID());
            }
         }

         //-- Add the unit we targeted
         mTargetsWhichNeedsToBeDestroyed.uniqueAdd(entityID);
      }
   }
}

//==============================================================================
//==============================================================================
void BMusicManager::setNewState(int32 state, bool force, DWORD overWriteTimer)
{ 
  if(isEnabled() == false)
     return;

  if(mCurrentState == state)
     return;
   
   //-- Has it been long enough to do a new state?
   if(gWorld->getGametime() > mModeLockedTimer || force)
   {      
      mCurrentState = (BMusicStateType)state;

      //-- Clear out the queued state   
      mQueuedState = cStateNone;

      
      //-- Play the cue
      uint index = mMusicState[mCurrentState].mCueIndex;
      uint size = mMusicState[mCurrentState].mSoundCues.getSize();
      if(index < size)
         gSoundManager.playCue(mMusicState[mCurrentState].mSoundCues[index]);

      //-- Increment the next cue
      if(size != 0)
         mMusicState[mCurrentState].mCueIndex = ((index + 1) % size);
      
      DWORD timer = mMusicState[mCurrentState].mLockedToModeTimer;
      if(overWriteTimer != 0)
         timer = overWriteTimer;

      mModeLockedTimer = gWorld->getGametime() + timer;
      mModeSetTime = gWorld->getGametime();
   }
   else //-- Hasn't been long enough, just do it next

   {
      if(mQueuedState != (BMusicStateType)state)
         mQueuedState = (BMusicStateType)state;
   }

   //--  Flag that standard has been called
   if(mCurrentState == cStateStandard)
   {
      if(mStandardEverSet == false)
      {         
         mStandardEverSet = true;
      }
   }
}

//==============================================================================
//==============================================================================
void BMusicManager::checkBattleEndedTimer()
{
   if(isEnabled() == false)
      return;

   if(mBattleEndedTime + mBattleEndedTimer < gWorld->getGametime())
   {
      //-- Make sure we're still in the state we thought we were, otherwise just ignore and reset the queued battle end state.
      int32 battleStatus = getBattleStatus();
      if(battleStatus == mQueuedBattleEndedState)
      {
         setNewState(mQueuedBattleEndedState, true);
         setNewState(cStateStandard);
         mTargetsWhichNeedsToBeDestroyed.clear();
      }

      mQueuedBattleEndedState = BMusicManager::cStateNone;
      mBattleEndedTime = 0;
   }
}

//==============================================================================
//==============================================================================
void BMusicManager::getCurrentState(BSimString &currentState)
{
   lookupStateString(mCurrentState, currentState);
}

//==============================================================================
//==============================================================================
void BMusicManager::getQueuedState(BSimString &queuedState)
{
   lookupStateString(mQueuedState, queuedState);
}

//==============================================================================
//==============================================================================
void BMusicManager::lookupStateString(int32 state, BSimString& string)
{
   switch(state)
   {
      case BMusicManager::cStateWorldIntro:
         string = "World Intro";
         break;
      case BMusicManager::cStateStandard:
         string = "Standard";
         break;
      case BMusicManager::cStateBattle1:
         string = "Battle1";
         break;
      case BMusicManager::cStateBattle2:
         string = "Battle2";
         break;
      case BMusicManager::cStateBattle3:
         string = "Battle3";
         break;
      case BMusicManager::cStateBattleWon:
         string = "Battle Won";
         break;
      case BMusicManager::cStateBattleLost:
         string = "Battle Lost";
         break;
      case BMusicManager::cStateNone:
         string = "None";
         break;
      default:
         string = "Unknown";

   }
}

//==============================================================================
//==============================================================================
DWORD BMusicManager::getSwitchTimer() const
{ 
   if(mModeLockedTimer < gWorld->getGametime())
      return 0;
   else 
      return mModeLockedTimer-gWorld->getGametime();
}

//==============================================================================
//==============================================================================
bool BMusicManager::inBattleState ()
{
   if(mCurrentState >= cStateBattle1 && mCurrentState <= cStateBattle3)
      return true;
   else
      return false;
}

//==============================================================================
// BMusicManager::setGameStartDelay
//==============================================================================
void BMusicManager::setGameStartDelay(bool val)
{
   mGameStartDelay = val;
}

//==============================================================================
// BMusicManager::setGameStartDelay
//==============================================================================
bool BMusicManager::isEnabled()
{
   if(mDisabled || gConfig.isDefined(cConfigEnableMusic) == false || mGameStartDelay)
      return false;
   else
      return true;
}

