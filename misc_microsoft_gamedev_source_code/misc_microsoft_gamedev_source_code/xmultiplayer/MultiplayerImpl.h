//==============================================================================
// MultiplayerImpl.h
//
// Copyright (c) 2003-2004, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef _MultiplayerImpl_H_
#define _MultiplayerImpl_H_

//==============================================================================
// Includes

#include "Multiplayer.h"
#ifdef XBOX
#include "winsockx.h"
#else
#include "winsock2.h"
#endif
#include "connectivity.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Forward declarations


class BMPGame;


//==============================================================================
// Const declarations

//==============================================================================
class MultiplayerImpl : public BMultiplayer, public BConnectivityObserver
{
   public:      
      virtual ~MultiplayerImpl( void );

      static bool createInstance(void);         
      static MultiplayerImpl *getInstance(void);
      static void destroyInstance(void);

      virtual void setGameState(long state);


      virtual long getLocalPlayer(void) const;
      virtual long getControlledPlayer(void) const;
      virtual void setControlledPlayer(long p);


      virtual bool isStarted(){ return mStarted; }
      virtual void startup(GameInterface *i);
      virtual DWORD service(void);
      virtual void shutdown(void);

      virtual HRESULT establishConnectivity(BConnectivityObserver* observer=NULL, BSimString* addr = NULL, BSimString* addr2 = NULL);
      virtual void removeConnectivityObserver(BConnectivityObserver* observer);
      virtual const SOCKADDR_IN &getLocalAddress(void);
      virtual const SOCKADDR_IN &getTranslatedLocalAddress(void);

      // connection methods
      virtual BMPLANIPConnector* getLANIPConnector(void);
	  virtual BMPLiveConnector* getLiveConnector(void);
      virtual void releaseLANIPConnector(BMPLANIPConnector* connector);
	  virtual void releaseLiveConnector(BMPLiveConnector* connector); 
      virtual void connectLANandDirectIP(void);
	  virtual void connectLive(void);
      virtual bool isLoggedIn(void);
      virtual long getConnectType(void) const { return(mConnectType); }

      //virtual MPChat* getChat(void);
      virtual bool attachFileXfer(BFileTransferGameInterface *xfer);
      virtual bool attachVote(BMPVoteNotify* vote);
      virtual bool detachVote(BMPVoteNotify* vote);

      virtual HRESULT createGame(BMPGameView **view = NULL);
      virtual bool isGameActive(void) const { return mActiveGame?true:false; }
      virtual DWORD getLocalChecksum(void);
      virtual void populateDefaultGameInfo(BMPGameView *gameView);
      virtual bool startGameSync(void);

      virtual HRESULT createGame(BDataSet *dataSet, DWORD checksum) ;
      virtual BMPGameView* createView(void) ;
      virtual void endActiveGame(void) ;
      virtual void resetActiveGame(void);
      virtual bool doneLoading(void) ;
      virtual bool finalizeGame(void) ;
      virtual BMPSimObject* getSimObject(void) ;      
      virtual DWORD advanceGameTiming(void) ;
      virtual void pauseGame(bool val) ;
      virtual long getGametime(void);
    

      // BConnectivityObserver
      virtual void connectivityState(long state, HRESULT hr=S_OK);
      virtual void networkDisabled();

   protected:      
      virtual void cleanup(void);

    
      friend class BMPGame;
   

      GameInterface *mGameInterface;

   private:      
      void releaseGame(BMPGame* game);
      MultiplayerImpl( void );
      void initLogging(void);
         
      bool mShuttingDown;
      bool mStarted;
      bool mInService;
      bool mShutdownRequested;
      bool mConnectivityEstablished;      // This is meant to be internal, don't make it external.
      BDynamicSimArray<BMPGame*> mGames;
      BDynamicSimArray<BMPGame*> mRemoved;
      BMPGame *mActiveGame;      
      long mLogFileID;
      long mLANIPRefCount;  
	  long mLiveRefCount;
      long mConnectType;
      BMPLANIPConnector *mLANIPConnector;
	  BMPLiveConnector  *mLiveConnector;
     
      
}; 
// MultiplayerImpl

//==============================================================================
#endif // _MultiplayerImpl_H_

//==============================================================================
// eof: MultiplayerImpl.h
//==============================================================================



