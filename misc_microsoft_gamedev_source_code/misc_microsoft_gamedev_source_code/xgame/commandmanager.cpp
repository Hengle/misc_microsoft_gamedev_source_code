//==============================================================================
// commandmanager.cpp
//
// Copyright (c) 1999-2008, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "commandmanager.h"

// xgame
#include "command.h"
#include "commands.h"
#include "configsgame.h"
#include "game.h"
#include "recordgame.h"
#include "serialbuffer.h"
#include "syncmacros.h"
//#include "triggerruntime.h"
#include "unit.h"
#include "world.h"

// xmultiplayer
#include "LiveSystem.h"
//#include "multiplayer.h"
#include "mpcommheaders.h"

// xnetwork
#include "channel.h"
#include "commlog.h"

//==============================================================================
// BCommandManager::BCommandManager
//==============================================================================
BCommandManager::BCommandManager() :   
   //mCommandsToExecute doesn't need any ctor args.
   //mExecutedCommands doesn't need any ctor args.   
   mpLastCommand(0),
   mDeleteLastCommand(FALSE),
   mCommandIDCounter(0),
   mIsMPRunning(FALSE)
{
}

//==============================================================================
// BCommandManager::~BCommandManager
//==============================================================================
BCommandManager::~BCommandManager()
{
}

//==============================================================================
// BCommandManager::setup
//==============================================================================
bool BCommandManager::setup()
{
   BMPSession* pMPSession = gLiveSystem->getMPSession();

   mIsMPRunning = (pMPSession ? pMPSession->isGameRunning() : FALSE);

   if (mIsMPRunning)
   {
      if (pMPSession->getSimObject())   
         pMPSession->getSimObject()->addObserver(this);      
   }

   for (uint i=0; i < cMaximumSupportedPlayers; i++)
   {
      for (uint j=0; j < cMaxCommandedUnitSets; j++)
         mCommandedUnitSets[i][j].mCounter=0;
   }

   return true;
}

//==============================================================================
// BCommandManager::shutdown
//==============================================================================
void BCommandManager::shutdown()
{
   if (mIsMPRunning && gLiveSystem->getMPSession()->getSimObject())
      gLiveSystem->getMPSession()->getSimObject()->removeObserver(this);    

   //Delete the un-executed commands that are still here.
   for (uint i=0; i < mCommandsToExecute.getSize(); i++)
   {
      if ((mCommandsToExecute[i] != NULL) && (mCommandsToExecute[i] != mpLastCommand))
         delete(mCommandsToExecute[i]);
      else if (mCommandsToExecute[i] == mpLastCommand)
         mDeleteLastCommand = TRUE;
   }

   //Delete the executed commands that are still here.
   //for (uint i=0; i < mExecutedCommands.getSize(); i++)
   //{
   //   if (mIsMPRunning)
   //   {
   //      if (mExecutedCommands[i] != NULL)
   //         delete(mExecutedCommands[i]);         
   //   }
   //   else
   //   {
   //      if ((mExecutedCommands[i] != NULL) && (mExecutedCommands[i] != mpLastCommand))
   //         delete(mExecutedCommands[i]);
   //      else if (mExecutedCommands[i] == mpLastCommand)
   //         mDeleteLastCommand = TRUE;
   //   }
   //}      

   if (mpLastCommand && mDeleteLastCommand)
      delete mpLastCommand;
   mpLastCommand = 0;
   mDeleteLastCommand = FALSE;

   for(long i=0; i<cMaximumSupportedPlayers; i++)
   {
      for(long j=0; j<cMaxCommandedUnitSets; j++)
      {
         BCommandedUnitSet& set=mCommandedUnitSets[i][j];
         set.mUnits.setNumber(0);
         set.mCounter=0;
      }
   }
}

//==============================================================================
// BCommandManager::createCommand
//==============================================================================
BCommand* BCommandManager::createCommand(PlayerID playerID, long type, bool /*emptyCommand*/)
{
   BCommand* pCommand = NULL;
   
   switch (type)
   {
      case cCommandWork:
         pCommand = new BWorkCommand();
         break;

      case cCommandPower:
         pCommand = new BPowerCommand();
         break;

      case cCommandPowerInput:
         pCommand = new BPowerInputCommand();
         break;

      case cCommandBuilding:
         pCommand = new BBuildingCommand();
         break;

      case cCommandGame:
         pCommand = new BGameCommand();
         break;

      case cCommandTrigger:
         pCommand = new BTriggerCommand();
         break;

      case cCommandGeneralEvent:
         pCommand = new BGeneralEventCommand();
         break;

      default:
         BASSERT(0); // you need to create a section for your command above
         break;
   }

   //Finish setting up the command.
   if (pCommand != NULL)
   {
      pCommand->setPlayerID(playerID);
   }

   return(pCommand);
}

//==============================================================================
// BCommandManager::sendCommandMP
//==============================================================================
void BCommandManager::sendCommandMP(BCommand* pCommand)
{
   if (!pCommand)
      return;

   if (mIsMPRunning && gLiveSystem->getMPSession()->getSimObject())   
      gLiveSystem->getMPSession()->getSimObject()->sendCommand(*pCommand);
}

//==============================================================================
// BCommandManager::service
//==============================================================================
void BCommandManager::service()
{
   if (mpLastCommand && mpLastCommand->getUrgencyCount() && mpLastCommand->timeHasExpired())
   {
      if (mIsMPRunning)
      {
         // shouldSendTimedUrgent() is currently always false
         if (mpLastCommand->shouldSendTimedUrgent())
            sendCommandMP(mpLastCommand);

         if (mDeleteLastCommand)
            delete mpLastCommand;
         mpLastCommand = 0;
         mDeleteLastCommand = FALSE;
      }
      else
      {
         // shouldSendTimedUrgent() is currently always false
         if (mpLastCommand->shouldSendTimedUrgent())
            mCommandsToExecute.add(mpLastCommand);
         else if (mDeleteLastCommand)
            delete mpLastCommand;
         mpLastCommand = 0;
         mDeleteLastCommand = FALSE;
      }         
   }
}

//==============================================================================
// BCommandManager::addCommandToExecute
//==============================================================================
bool BCommandManager::addCommandToExecute(BCommand* pCommand, bool receivedOrReplayed)
{
   if (pCommand == NULL)
      return(false);   

   if (gRecordGame.isPlaying())
   {
      delete pCommand;
      return false;
   }

   //fixme
   /*
	//-- In MP Games, eat commands from AIs who can't issue commands.
	if(gWorld && BMultiplayer::getInstance()->isGameActive() && !receivedOrReplayed)
	{
		BPlayer* pCmdPlayer = gWorld->getPlayer(command->getPlayerID());
		if(pCmdPlayer != NULL && pCmdPlayer->isComputer() == true && pCmdPlayer->getCanIssueAICommands() == false)
		{
			delete command;
			return(false);
		}
	}
   */
     
   // if we're connected to the network, then pass the command if it's an outgoing command
   if (mIsMPRunning && !receivedOrReplayed)
   {  
      if (pCommand->meter(mpLastCommand))
      {
         if (mpLastCommand && mDeleteLastCommand)
            delete mpLastCommand;
         mpLastCommand = pCommand;
         mDeleteLastCommand = TRUE;

         return false;
      }
      else
      {  
         // send if we have an urgency count, and this command gets sent when the timer expires, or the count has expired
         //
         // [10/9/2008 DPM] This check will always be false since shoulSendTimedUrgent() and countHasExpired() will always return false
         if (mpLastCommand && mpLastCommand->getUrgencyCount() && (mpLastCommand->shouldSendTimedUrgent() || mpLastCommand->countHasExpired()) &&
               (
                  mpLastCommand->isTimeMetered() ||
                  mpLastCommand->countHasExpired()
               )
            )
            sendCommandMP(mpLastCommand); // send out the last command again, if it has an urgency count

         if (mpLastCommand && mDeleteLastCommand)
            delete mpLastCommand;
         mpLastCommand = pCommand;
         mDeleteLastCommand = TRUE;

         sendCommandMP(pCommand);

         pCommand->sent();

         return false;
      }
   }
   else
   {
      if (!receivedOrReplayed && pCommand->meter(mpLastCommand))
      {
         if (mpLastCommand && mDeleteLastCommand)
            delete mpLastCommand;
         mpLastCommand = pCommand;
         mDeleteLastCommand = TRUE;

         return false;
      }
      else
      {
         if (!receivedOrReplayed)
         {
            // send if we have an urgency count, and this command gets sent when the timer expires, or the count has expired
            //
            // [10/9/2008 DPM] This check will always be false since shoulSendTimedUrgent() and countHasExpired() will always return false
            if (mpLastCommand && mpLastCommand->getUrgencyCount() && (mpLastCommand->shouldSendTimedUrgent() || mpLastCommand->countHasExpired()) &&
                  (
                     mpLastCommand->isTimeMetered() ||
                     mpLastCommand->countHasExpired()
                  )
               )
               mCommandsToExecute.add(mpLastCommand); // queue up the last command, if it has an urgency count
            else
            {
               if (mpLastCommand && mDeleteLastCommand)
                  delete mpLastCommand;
            }

            mpLastCommand = pCommand;
            mDeleteLastCommand = FALSE;

            pCommand->sent();
         }
         return(mCommandsToExecute.add(pCommand) != -1);
      }
   }   
}

//==============================================================================
// BCommandManager::commandReceived
//==============================================================================
void BCommandManager::commandReceived(const void* pData, DWORD size)
{   
   // deserialize the command and add it to the local queue
   //DCP 06/15/00:  Since we really don't want the playerID to be valid, we pass in
   //-1 here instead of 0.
   BCommand* pCommand = createCommand(-1, BCommand::getCommandType(pData), true);
   if (!pCommand)
   {
      BASSERT(0);
      return;
   }
   pCommand->deserializeFrom(pData, size);

   addCommandToExecute(pCommand, true);
}

//==============================================================================
// BCommandManager::getCommandToExecute
//==============================================================================
//BCommand* BCommandManager::getCommandToExecute(long id) const
//{
//   for (long i=0; i < mCommandsToExecute.getNumber(); i++)
//   {
//      if ((mCommandsToExecute[i] != NULL) && (mCommandsToExecute[i]->getID() == id))
//         return(mCommandsToExecute[i]);
//   }
//   return(NULL);
//}

//==============================================================================
// BCommandManager::getCommandToExecuteByIndex
//==============================================================================
//BCommand* BCommandManager::getCommandToExecuteByIndex(long index) const
//{
//   if ((index < 0) || (index >= mCommandsToExecute.getNumber()) )
//      return(NULL);
//   return(mCommandsToExecute[index]);
//}

//==============================================================================
// BCommandManager::getExecutedCommand
//==============================================================================
//BCommand* BCommandManager::getExecutedCommand(long id) const
//{
//   for (long i=0; i < mExecutedCommands.getNumber(); i++)
//   {
//      if ((mExecutedCommands[i] != NULL) && (mExecutedCommands[i]->getID() == id))
//         return(mExecutedCommands[i]);
//   }
//   return(NULL);
//}

//==============================================================================
// BCommandManager::getCommand
//==============================================================================
//BCommand* BCommandManager::getCommand(long id) const
//{
//   //DCP 09/17/02:  If we have a negative command ID, we don't really want a command
//   //to come back.
//   if (id < 0)
//      return(NULL);
//   BCommand* pCommand = getExecutedCommand(id);
//   return(pCommand);
//}

//==============================================================================
// BCommandManager::decrementCommandReferenceCount
//==============================================================================
//bool BCommandManager::decrementCommandReferenceCount(long id)
//{
//   BCommand* pCommand = getCommand(id);
//   if (c == NULL)
//      return(false);
//   pCommand->decrementReferenceCount();
//
//   //fixme
//   /*
//   //DCP TODO 04/11/01: Find a better place to put this.
//   if ((c->getReferenceCount() <= 0) && (gWorld->getTriggerRuntime()))
//         gWorld->getTriggerRuntime()->addEvent(c->getEventID());
//   */
//
//   return(true);
//}

//==============================================================================
// BCommandManager::incrementCommandReferenceCount
//==============================================================================
//bool BCommandManager::incrementCommandReferenceCount(long id)
//{
//   BCommand* pCommand = getCommand(id);
//   if (pCommand == NULL)
//      return(false);
//   pCommand->incrementReferenceCount();
//   return(true);
//}

//==============================================================================
// BCommandManager::decrementQueuedCommandReferenceCount
//==============================================================================
//bool BCommandManager::decrementQueuedCommandReferenceCount(long id)
//{
//   BCommand* pCommand = getCommand(id);
//   if (pCommand == NULL)
//      return(false);
//   pCommand->decrementQueueReferenceCount();
//   return(true);
//}

//==============================================================================
// BCommandManager::incrementQueuedCommandReferenceCount
//==============================================================================
//bool BCommandManager::incrementQueuedCommandReferenceCount(long id)
//{
//   BCommand* pCommand = getCommand(id);
//   if (pCommand == NULL)
//      return(false);
//   pCommand->incrementQueueReferenceCount();
//   return(true);
//}

//==============================================================================
// BCommandManager::processCommands
//==============================================================================
void BCommandManager::processCommands()
{
   if(gRecordGame.isRecording())
      gRecordGame.recordCommands(mCommandsToExecute);
   else if(gRecordGame.isPlaying())
      gRecordGame.playCommands(mCommandsToExecute);

   //Process all of the commands that are waiting to be processed.
   for (long i=0; i < mCommandsToExecute.getNumber(); i++)
   {
      if (mCommandsToExecute[i] == NULL)
         continue;

      BCommand* pCommand = mCommandsToExecute[i];
      //"Take" the command out of the to execute queue.
      mCommandsToExecute[i] = NULL;

      //Reset the reference count in the command before we execute it.
      //mCommandsToExecute[i]->setReferenceCount(0);
      pCommand->setExecuteTime(gWorld->getGametime());
      //Give it a unique, synchronous ID.
      pCommand->setID(mCommandIDCounter++);

      //Add this to the executed command list (since we may need to do lookups on it
      //during the execute call).
      //if (addExecutedCommand(mCommandsToExecute[i]) == false)
      //{
      //   // this would be really bad
      //   BASSERT(0);
      //   if (mIsMPRunning)
      //   {
      //      delete mCommandsToExecute[i];
      //      mCommandsToExecute[i] = NULL;
      //      continue;
      //   }
      //   else
      //   {
      //      if (mCommandsToExecute[i] != mpLastCommand)
      //      {
      //         delete mCommandsToExecute[i];
      //         mCommandsToExecute[i] = NULL;
      //         continue;
      //      }
      //      else
      //         mDeleteLastCommand = TRUE;
      //   }
      //   continue;
      //}

      syncCommandData("possibly executing command", i);
      syncCommandData("  type", pCommand->getType());

      pCommand->sync();

      BOOL hackExecute = TRUE;
      //fixme
      /*
      if(command->getType() != cCommandResign)
      {
         if (gWorld && gWorld->getPlayer(command->getPlayerID()) && 
              (!gWorld->getPlayer(command->getPlayerID())->isActive() || 
               !gWorld->getPlayer(command->getPlayerID())->getCanIssueCommands()) )
         {
            hackExecute=false;
            syncCommandCode("  not actually executing it");
         }
      }
      */

      // MPB 1/18/08 - Super hack to disable active abilities besides lockdown
      if (!gConfig.isDefined(cConfigActiveAbilities) && 
          ((pCommand->getSquadMode() != -1) &&
           (pCommand->getSquadMode() != BSquadAI::cModeNormal) &&
           (pCommand->getSquadMode() != BSquadAI::cModeLockdown)))
      {
         hackExecute = FALSE;
      }

      //Execute the command.  The command knows whether or not it's a
      //queued command, so it will deal with queueing or immediately
      //executing appropriately.
      if (hackExecute)
      {
         syncCommandCode("  executing it");
         pCommand->execute();
      }

      if (mpLastCommand == pCommand)
         mDeleteLastCommand = TRUE;
      else
         delete pCommand;

      if (!gWorld || (this != gWorld->getCommandManager())) // did we re-create the world coz of that command? if so, drop out
         return;
   }
   // We've dealt with all of the commands, so zero that number out.
   mCommandsToExecute.setNumber(0);

   // Remove any executed commands with a 0 reference count and a 0 queue reference count.
   //bool anyEC = false;
   //for (long i=0; i < mExecutedCommands.getNumber(); i++)
   //{
   //   if (mExecutedCommands[i] == NULL)
   //      continue;

   //   // it appears that nobody is incrementing the reference count of queue reference count
   //   // so for now this will never be true, but we should double check!
   //   //if ((mExecutedCommands[i]->getReferenceCount() > 0) ||
   //   //   (mExecutedCommands[i]->getQueueReferenceCount() > 0))
   //   //{
   //   //   anyEC = true;
   //   //   continue;
   //   //}

   //   if (mIsMPRunning)
   //   {
   //      delete mExecutedCommands[i];
   //   }
   //   else
   //   {
   //      if (mExecutedCommands[i] != mpLastCommand)
   //         delete mExecutedCommands[i];
   //      else
   //         mDeleteLastCommand = TRUE;
   //   }

   //   mExecutedCommands[i] = NULL;
   //}
   ////if (anyEC == false)
   ////   mExecutedCommands.setNumber(0);
   //mExecutedCommands.setNumber(0);

   // we set mDeleteLastCommand to TRUE because if it does exist, it will be orphaned
}

//==============================================================================
// BCommandManager::addExecutedCommand
//==============================================================================
//bool BCommandManager::addExecutedCommand(BCommand* pCommand)
//{
//   //HACK: Not done yet.
//   for (long i=0; i < mExecutedCommands.getNumber(); i++)
//   {
//      if (mExecutedCommands[i] == NULL)
//      {
//         mExecutedCommands[i] = pCommand;
//         return(true);
//      }
//   }
//
//   //If we're here, we have to add it.
//   return(mExecutedCommands.add(pCommand) != -1);
//}

//fixme
/*
//==============================================================================
// BCommandManager::invalidateExecutedCommandEvents
//==============================================================================
void BCommandManager::invalidateExecutedCommandEvents(void)
{
   long count = mExecutedCommands.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (!mExecutedCommands[idx])
         continue;

      mExecutedCommands[idx]->setEventID(-1);
   }
} 
*/

//==============================================================================
// BCommandManager::getCommandedUnitSet
//==============================================================================
long BCommandManager::getCommandedUnitSet(long player, const BEntityIDArray& units)
{
   if(units.getNumber()<3)
      return -1;

   if(player<0 || player>=cMaximumSupportedPlayers)
      return -1;

   long unitCount=units.getNumber();
   for(long j=0; j<cMaxCommandedUnitSets; j++)
   {
      BCommandedUnitSet& set=mCommandedUnitSets[player][j];
      if(set.mUnits.getNumber()==unitCount)
      {
         long k;
         for(k=0; k<unitCount; k++)
         {
            if(set.mUnits[k]!=units[k])
               break;
         }
         if(k==unitCount)
            return j;
      }
   }

   return -1;
}

//==============================================================================
// BCommandManager::cacheCommandedUnits
//==============================================================================
void BCommandManager::cacheCommandedUnits(long player, const BEntityIDArray& units)
{
   if(units.getNumber()<3)
      return;

   if(player<0 || player>=cMaximumSupportedPlayers)
      return;

   BCommandedUnitSet* oldestSet=mCommandedUnitSets[player];
   DWORD highCounter=0;
   for(long j=1; j<cMaxCommandedUnitSets; j++)
   {
      BCommandedUnitSet* set=mCommandedUnitSets[player]+j;
      if(set->mCounter<oldestSet->mCounter)
         oldestSet=set;
      if(set->mCounter>highCounter)
         highCounter=set->mCounter;
   }

   long unitCount=units.getNumber();
   if(!oldestSet->mUnits.setNumber(unitCount))
      return;
   for(long j=0; j<unitCount; j++)
      oldestSet->mUnits[j]=units[j];

   oldestSet->mCounter=highCounter+1;
}

//==============================================================================
// BCommandManager::getCommandedUnits
//==============================================================================
void BCommandManager::getCommandedUnits(long player, long unitSetID, BEntityIDArray& units)
{
   units.setNumber(0);

   if(player<0 || player>=cMaximumSupportedPlayers)
      return;

   if(unitSetID<0 || unitSetID>=cMaxCommandedUnitSets)
      return;

   BCommandedUnitSet* set=mCommandedUnitSets[player]+unitSetID;

   long unitCount=set->mUnits.getNumber();
   if(!units.setNumber(unitCount))
      return;

   for(long j=0; j<unitCount; j++)
      units[j]=set->mUnits[j];
}

//==============================================================================
// BCommandManager::syncCommands
//==============================================================================
//void BCommandManager::syncCommands()
//{
//   long total=mExecutedCommands.getNumber();
//   syncCommandCode("BCommandManager::syncCommands");
//   
//   syncCommandData(" BCommandManager::syncCommands - total executed Commands", total);
//   for (long i=0; i < total; i++)
//   {
//      if (mExecutedCommands[i] != NULL)
//      {
//         syncCommandData("   BCommandManager::syncCommands - type", mExecutedCommands[i]->getType());
//         syncCommandData("   BCommandManager::syncCommands - id", mExecutedCommands[i]->getID());
//         syncCommandData("   BCommandManager::syncCommands - playerID", mExecutedCommands[i]->getPlayerID());
//      }
//      else
//         syncCommandData("   BCommandManager::syncCommands - Command is NULL, index=", i);
//   }
//
//   /*total=mCommandsToExecute.getNumber();
//   syncCommandData(" BCommandManager::syncCommands - total commands to execute", total);
//   for (i=0; i < total; i++)
//   {
//      if (mCommandsToExecute[i] != NULL)
//      {
//         syncCommandData("   BCommandManager::syncCommands - type", mCommandsToExecute[i]->getType());
//         syncCommandData("   BCommandManager::syncCommands - id", mCommandsToExecute[i]->getID());
//         syncCommandData("   BCommandManager::syncCommands - playerID", mCommandsToExecute[i]->getPlayerID());
//      }
//      else
//         syncCommandData("   BCommandManager::syncCommands - Command is NULL, index=", i);
//   }*/
//}
