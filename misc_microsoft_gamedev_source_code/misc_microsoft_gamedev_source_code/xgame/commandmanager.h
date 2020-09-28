//==============================================================================
// commandmanager.h
//
// Copyright (c) 1999-2008, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
#include "command.h"
#include "mpSimDataObject.h"
#include "mptypes.h"
#include "maximumsupportedplayers.h"

// Forward declarations
class BSerialBuffer;
class BCommandChannel;

//==============================================================================
// BCommandedUnitSet
//==============================================================================
class BCommandedUnitSet
{
   public:
      BEntityIDArray  mUnits;
      DWORD                mCounter;
};

//==============================================================================
// BCommandManager
//==============================================================================
class BCommandManager : public BMPSimDataObject::BMPSimObserver
{
   // This class acts as a FIFO buffer for commands, as well as providing other 
   // command management functionality
   public:
      enum
      {
         cMaxCommandedUnitSets=10,
      };

                                 BCommandManager();
      virtual                    ~BCommandManager();

      bool                       setup();
      void                       shutdown();

      //Gets/Sets.
      long                       getCommandIDCounter() const { return(mCommandIDCounter); }

      //Creates a new Command of the given type, for the given player.
      static BCommand*           createCommand(PlayerID playerID, long type, bool emptyCommand = false);

      //Commands to execute are commands that have not been executed yet.
      bool                       addCommandToExecute(BCommand* pCommand, bool receivedOrReplayed = false);
      //long                       getNumberCommandsToExecute( void ) const { return(mCommandsToExecute.getNumber()); }
      //BCommand*                  getCommandToExecute( long id ) const;
      //BCommand*                  getCommandToExecuteByIndex( long index ) const;

      //Executed commands are commands that have been executed already.
      //BCommand*                  getExecutedCommand( long id ) const;

      //Misc command methods.
      //BCommand*                  getCommand( long id ) const;
      //bool                       decrementCommandReferenceCount( long id );
      //bool                       incrementCommandReferenceCount( long id );
      //bool                       decrementQueuedCommandReferenceCount( long id );
      //bool                       incrementQueuedCommandReferenceCount( long id );

      //processCommands simply runs/executes/processes all of the pending commands.
      void                       processCommands();

      // must be called every update/frame
      void                       service();

      // wacky code to clear all of the event IDs on the executed commands so that when we 
      // abort a cinematic, we don't get stray events firing.
      //void                       invalidateExecutedCommandEvents(void);

      // Cached off commanded unit set stuff.
      long                       getCommandedUnitSet(long player, const BEntityIDArray& units);
      void                       cacheCommandedUnits(long player, const BEntityIDArray& units);
      void                       getCommandedUnits(long player, long unitSetID, BEntityIDArray& units);

      // Sync stuff
      //void                       syncCommands();

      // BMPSimObject::BMPSimObserver
      virtual void               commandReceived(const void* pData, DWORD size);

      // debug helper
      bool                       isLastCommand(const BCommand* pCommand) const { return (mpLastCommand == pCommand); }

   protected:
      //Helper methods.
      //bool                       addExecutedCommand(BCommand* pCommand);
      void                       sendCommandMP(BCommand* pCommand);

      // Variables
      long                       mCommandIDCounter;
      BCommandPointerArray       mCommandsToExecute;
      //BCommandPointerArray       mExecutedCommands;
      BCommand*                  mpLastCommand;
      BOOL                       mDeleteLastCommand;

      BOOL                       mIsMPRunning;

      // Cached off commanded unit set stuff.
      BCommandedUnitSet          mCommandedUnitSets[cMaximumSupportedPlayers][cMaxCommandedUnitSets];

};