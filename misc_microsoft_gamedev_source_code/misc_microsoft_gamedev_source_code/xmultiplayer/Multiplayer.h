//==============================================================================
// Multiplayer.h
//
// Copyright (c) 2003-2004, Ensemble Studios
//==============================================================================

#ifndef _BMultiplayer_H_
#define _BMultiplayer_H_

//==============================================================================
// Includes
#ifndef XBOX
#include "winsock2.h"
#endif

#include "mptypes.h"

//==============================================================================
// Forward declarations


class BMPGame;
class BMPGameView;
class BDataSet;
class BMPSimObject;
class BMPLANIPConnector;
class BMPLiveConnector;
//class BMPChatInterface;
//class MPChat;
class BFileTransferGameInterface;
class BMPVoteNotify;
class MPTaskManager;
class BMPSyncObject;
class BConnectivityObserver;

typedef SOCKADDR_IN SOCKADDR_IN;

//==============================================================================
// Const declarations

enum
{
   cGameStateSetup,
   cGameStateLaunching,
   cGameStatePregame,
   cGameStateInProgress,
   cGameStatePostgame,
   cGameStateFinal,

   cGameStateTotal,
   cGameStateInvalid
};

//==============================================================================
// Abstract
class BMultiplayer
{
   public:
      class GameInterface
      {
         public:
            virtual void setupSync(BMPSyncObject *object) = 0;
            virtual BDataSet *getGameDataSet(void) = 0;
            virtual DWORD getLocalChecksum(void) = 0;
            virtual bool populateDefaultGameInfo(BMPGameView *gameView) = 0;
            virtual bool startGameSync(void) = 0;
            virtual void initCommLogging(void) = 0;
            virtual bool isOOS(void) = 0;
            virtual void networkDisabled(void) = 0;   
            virtual long getDataDirID(void) = 0;
            virtual long getGametime(void) = 0;
      };

      enum
      {
         cDisconnectNormal,
         cDisconnectGameTerminated,
         cDisconnectFailedConnection,
         cDisconnectCRCMismatch,
         cDisconnectFull,
         cDisconnectDeleted,
         cDisconnectMax
      };

      enum
      {
         cGameTypeRandomMap,
         cGameTypeScenario,
         cGameTypeSavedGame,
         cGameTypeTotal,
         cGameTypeInvalid
      };
      enum { cNoConnectType, cLANConnectType, cLiveConnectType };

      enum
      {
         cHandicappingOff = 0,
         cHandicappingFree,
      };

      virtual ~BMultiplayer() = 0 {} 

      virtual void setGameState(long state) = 0;

      //virtual MPChat* getChat(void) = 0;
      virtual bool attachFileXfer(BFileTransferGameInterface *xfer) = 0;
      virtual bool attachVote(BMPVoteNotify* vote) = 0;
      virtual bool detachVote(BMPVoteNotify* vote) = 0;

      virtual bool isStarted() = 0;
      virtual void startup(GameInterface *i) = 0;
      virtual DWORD service(void) = 0;
      virtual void shutdown(void) = 0;

      virtual HRESULT establishConnectivity(BConnectivityObserver* observer=NULL, BSimString* addr = NULL, BSimString* addr2 = NULL) = 0;
      virtual void removeConnectivityObserver(BConnectivityObserver* observer) = 0;
      virtual const SOCKADDR_IN &getLocalAddress(void) = 0;
      virtual const SOCKADDR_IN &getTranslatedLocalAddress(void) = 0;

      // connection methods
      virtual BMPLANIPConnector* getLANIPConnector(void) = 0;
	  virtual BMPLiveConnector* getLiveConnector(void) = 0;
      virtual void releaseLANIPConnector(BMPLANIPConnector* connector) = 0; 
	  virtual void releaseLiveConnector(BMPLiveConnector* connector) = 0; 	 
      virtual void connectLANandDirectIP(void) = 0;
	  virtual void connectLive(void) = 0;
      virtual bool isLoggedIn(void) = 0;
      virtual long getConnectType(void) const = 0;

      virtual long getLocalPlayer(void) const = 0;
      virtual long getControlledPlayer(void) const = 0;
      virtual void setControlledPlayer(long p) = 0;

      virtual HRESULT createGame(BMPGameView **view = NULL) = 0;
      bool gameActive(void) { return isGameActive(); }
      virtual bool isGameActive(void) const = 0;
      virtual DWORD getLocalChecksum(void) = 0;
      
      virtual void populateDefaultGameInfo(BMPGameView *gameView) = 0;
      virtual bool startGameSync(void) = 0;

      virtual HRESULT createGame(BDataSet *dataSet, DWORD checksum) = 0;
      virtual BMPGameView* createView(void) = 0;
      virtual void endActiveGame(void) = 0;
      virtual void resetActiveGame(void) = 0;
      virtual bool doneLoading(void) = 0;
      virtual bool finalizeGame(void) = 0; 
      virtual BMPSimObject* getSimObject(void) = 0;      
      virtual DWORD advanceGameTiming(void) = 0;
      virtual void pauseGame(bool val) = 0;
      virtual long getGametime(void) = 0;
       

      static bool createInstance(void);
      static BMultiplayer *getInstance(void);
      static void destroyInstance(void);

   protected:
      virtual void cleanup(void) = 0;

   private:
}; // BMultiplayer

//==============================================================================
#endif // _BMultiplayer_H_

//==============================================================================
// eof: Multiplayer.h
//==============================================================================