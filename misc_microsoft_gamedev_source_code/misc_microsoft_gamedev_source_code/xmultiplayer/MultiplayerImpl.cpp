//==============================================================================
// MultiplayerImpl.cpp
//
// Copyright (c) 2003-2004, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "multiplayer.h"
#include "MultiplayerImpl.h"
#include "connectivity.h"
#include "mpgame.h"
#include "SocksHelper.h"
#include "mpLANIPConnector.h"
#include "mpLiveConnector.h"
#include "mpsimobject.h"
#include "mpvote.h"
#include "filetransfermgr.h"


static MultiplayerImpl *multiplayerImpl = NULL;

//==============================================================================
// Defines

//==============================================================================
MultiplayerImpl::MultiplayerImpl(void) :
   mShuttingDown(false),
   mInService(false),
   mShutdownRequested(false),
   mStarted(false),
   mConnectivityEstablished(false),
   mGameInterface(NULL),
   mActiveGame(NULL),
   mLogFileID(-1),
   mLANIPConnector(NULL),
   mLiveConnector(NULL),
   mLANIPRefCount(0),
   mLiveRefCount(0),
   mConnectType(cNoConnectType)   
{

} // MultiplayerImpl::MultiplayerImpl

//==============================================================================
MultiplayerImpl::~MultiplayerImpl(void)
{
} // MultiplayerImpl::~MultiplayerImpl

//==============================================================================
bool MultiplayerImpl::createInstance()
{
   multiplayerImpl = new MultiplayerImpl();
   BFATAL_ASSERT(multiplayerImpl);
   if (!multiplayerImpl)
      return false;
   return true;
}

//==============================================================================
MultiplayerImpl *MultiplayerImpl::getInstance(void)
{  
   //BFATAL_ASSERT(multiplayerImpl);
   return multiplayerImpl;
}

//==============================================================================
void MultiplayerImpl::destroyInstance(void)
{
   if (multiplayerImpl)
      delete multiplayerImpl;
   multiplayerImpl = NULL;
}



//==============================================================================
void MultiplayerImpl::startup(GameInterface *i)
{
   if (mStarted)
      return;

   mGameInterface = i;

   initLogging();

   if (BSocksHelper::socksStartup() != S_OK)
   {
      BFAIL("BMultiplayerImpl::startup -- Failed to startup socks layer.");
      return;
   }

   mStarted = true;
  
}

//==============================================================================
DWORD MultiplayerImpl::service(void)
{
   if (!mStarted)
      return BConnectivity::cErrNone;

   mInService = true;

   DWORD dwErr = BConnectivity::getInstance()->service();
   if (dwErr != BConnectivity::cErrNone)
   {
      mInService = false;
      return dwErr;
   }

   BSocksHelper::serviceSockets();

   if (mLANIPConnector)
   {
      if (mLANIPRefCount <= 0)
      {
         delete mLANIPConnector;
         mLANIPConnector = NULL;
         mLANIPRefCount = 0;
      }
      else
         mLANIPConnector->service();
   }

   if (mLiveConnector)
   {
	   if (mLiveRefCount <= 0)
	   {
		   delete mLiveConnector;
		   mLiveConnector = NULL;
		   mLiveRefCount = 0;
	   }
	   else
		   mLiveConnector->service();
   }
 
   long count = mRemoved.getNumber();
   for (long idx=0; idx<count; idx++)
      releaseGame(mRemoved[idx]);
   mRemoved.clear();

   count = mGames.getNumber();
   for (long idx=0; idx<count; idx++)
      mGames[idx]->service();

   mInService = false;

   if (mShutdownRequested)
      shutdown();

   return BConnectivity::cErrNone;
}

//==============================================================================
void MultiplayerImpl::shutdown(void)
{
   if (!mStarted)
      return;

   if (mInService)
   {
      mShutdownRequested = true;
      return;
   }

   // I just can't seem to get this shit right.
   // prevent re-entrancy down below because it can apparently happen.
   if (mShuttingDown)
      return;
   mShuttingDown = true;

   cleanup();

   mInService = false;
   mShutdownRequested = false;
   mStarted = false;
}

//==============================================================================
HRESULT MultiplayerImpl::establishConnectivity(BConnectivityObserver* observer, BSimString* addr /*= NULL*/, BSimString* addr2 /*= NULL*/)
{
   if (!mStarted)
      return E_FAIL;

   BConnectivity::getInstance()->addObserver(observer);
   HRESULT hr = BConnectivity::getInstance()->setupConnectivity(addr, addr2);
   if (FAILED(hr))
   {
      BConnectivity::getInstance()->destroyInstance();
      mConnectivityEstablished = false;
      return(hr);
   }

   return(hr);
}

//==============================================================================
void MultiplayerImpl::removeConnectivityObserver(BConnectivityObserver* observer)
{
   BConnectivity::getInstance()->removeObserver(observer);
}

//==============================================================================
HRESULT MultiplayerImpl::createGame(BMPGameView **view)
{
   if (!mStarted)
      return E_FAIL;

   if (isGameActive())
      return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);

   if (!mConnectivityEstablished)
      return HRESULT_FROM_WIN32(ERROR_NETWORK_ACCESS_DENIED);
   
   if (!mGameInterface) return E_FAIL;

   HRESULT hr = createGame(mGameInterface->getGameDataSet(), mGameInterface->getLocalChecksum());
   if (FAILED(hr))
      return hr;
      
   if (view)
      *view = createView();
   
   return S_OK;
}

//==============================================================================
DWORD MultiplayerImpl::getLocalChecksum(void)
{   
   if (!mStarted || !mGameInterface) return 0;

   return mGameInterface->getLocalChecksum();
}

//==============================================================================
const SOCKADDR_IN &MultiplayerImpl::getLocalAddress(void)
{
   if (!BConnectivity::getInstance()->isNetworkingEnabled())
      blog("MultiplayerImpl::getLocalAddress -- called when Connectivity says network is disabled.");

   return BConnectivity::getInstance()->getLocalAddress();
}

//==============================================================================
const SOCKADDR_IN &MultiplayerImpl::getTranslatedLocalAddress(void)
{
   if (!BConnectivity::getInstance()->isNetworkingEnabled())
      blog("MultiplayerImpl::getTranslatedLocalAddress -- called when Connectivity says network is disabled.");
   
   return BConnectivity::getInstance()->getTranslatedLocalAddress();
}


//==============================================================================
void MultiplayerImpl::populateDefaultGameInfo(BMPGameView *gameView)
{
   if (!mStarted) 
      return;

   BFATAL_ASSERT(mGameInterface);
   if (!mGameInterface) 
      return;

   mGameInterface->populateDefaultGameInfo(gameView);
}
//==============================================================================
bool MultiplayerImpl::startGameSync(void)
{
   if (!mStarted)
      return(false);

   if (!mActiveGame)
      return(false);

   if (!mActiveGame->startGame())
      return(false);

   BFATAL_ASSERT(mGameInterface);
   if (!mGameInterface) return false;

   if (!mGameInterface->startGameSync())
      return false;
   
   if(!mActiveGame)
      return false;

   mActiveGame->gameStartComplete();

   return(true);
}

//==============================================================================
void MultiplayerImpl::cleanup(void)
{

   //-- wmj shouldn't we delete the removed games too?
   long count = mRemoved.getNumber();
   for (long idx=0; idx<count; idx++)
      releaseGame(mRemoved[idx]);
   mRemoved.clear();

   count = mGames.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      delete mGames[idx];
   }
   mGames.setNumber(0);
  
  
  
   gLogManager.closeLogFile(mLogFileID);   

   if (mLANIPConnector)
      delete mLANIPConnector;
   mLANIPConnector = NULL;
   
   if (mLiveConnector)
	   delete mLiveConnector;
   mLiveConnector = NULL;

   BConnectivity::getInstance()->removeObserver(this);

   BSocksHelper::socksCleanup();

   mGameInterface = NULL;
   mActiveGame = NULL;
}

//==============================================================================
long MultiplayerImpl::getLocalPlayer(void) const
{
   if (!mStarted || !mActiveGame)
      return(-1);
   
   return(mActiveGame->getLocalPlayer());
}

//==============================================================================
long MultiplayerImpl::getControlledPlayer(void) const
{ 
   if (!mStarted || !mActiveGame)
      return(-1);

   return(mActiveGame->getControlledPlayer());
}

//==============================================================================
void MultiplayerImpl::setControlledPlayer(long p) 
{ 
   if (!mStarted)
   {
      BFAIL("MultiplayerImpl::setControlledPlayer -- Multiplayer::startup not called yet.");
      return;
   }

   if (mActiveGame)
      mActiveGame->setControlledPlayer(p);
}

//==============================================================================
HRESULT MultiplayerImpl::createGame(BDataSet *dataSet, DWORD checksum)
{
   if (!mStarted)
      return E_FAIL;

   if (!BConnectivity::getInstance()->isNetworkingEnabled())
      return E_FAIL;

   BMPGame* pGame = new BMPGame(BConnectivity::getInstance());
   mGames.add(pGame);

   BFATAL_ASSERTM(mGameInterface, "Invalid mGameInterface"); // gotta set this up at the dawn of time - call setSyncInterface
   if (!mGameInterface) return E_FAIL;
   // just assume the game is going to by synced for now
   mGameInterface->setupSync(pGame);

   mActiveGame = pGame;
   pGame->initialize(dataSet, checksum);

   return S_OK;
}

//==============================================================================
BMPGameView* MultiplayerImpl::createView(void)
{
   if (!mStarted || !mActiveGame)
      return NULL;

   return mActiveGame->createView();
}

//==============================================================================
void MultiplayerImpl::releaseGame(BMPGame* game)
{
   if (!mStarted || !game)
      return;

   if (mActiveGame == game)
      mActiveGame = NULL;

   mGames.remove(game);   
   delete game;
}

//==============================================================================
void MultiplayerImpl::endActiveGame(void)
{
   if (!mStarted)
      return;

   if (mActiveGame)
   {
      mRemoved.add(mActiveGame);

      if (mGameInterface)
         mGameInterface->setupSync(NULL);
   }
   mActiveGame = NULL;   
}

//==============================================================================
void MultiplayerImpl::resetActiveGame(void)
{
   if (!mStarted)
      return;

   if (mActiveGame)
   {
      mActiveGame->stopGame();

      if (mGameInterface)
         mGameInterface->setupSync(mActiveGame);
   }
}

//==============================================================================
bool MultiplayerImpl::doneLoading(void)
{
   if (!mStarted || !mActiveGame)
      return(false);

   if (!mActiveGame->waitingOnOtherPlayers())
      return(false);

   return(true);
}

//==============================================================================
bool MultiplayerImpl::finalizeGame(void)
{
   if (!mStarted || !mActiveGame)
      return(false);

   if (!mActiveGame->finalizeGame())
      return(false);

   return(true);
}

//==============================================================================
BMPSimObject*  MultiplayerImpl::getSimObject(void)
{
   if (!mStarted || !mActiveGame)
      return(NULL);

   return(mActiveGame->getSimObject());
}
      
//==============================================================================
DWORD MultiplayerImpl::advanceGameTiming(void)
{
   if (!mStarted || !mActiveGame || !mActiveGame->getSimObject())
      return(0);

   return(mActiveGame->getSimObject()->advanceGameTime());
}

//==============================================================================
void MultiplayerImpl::pauseGame(bool val)
{
   if (!mStarted || !mActiveGame || !mActiveGame->getSimObject())
      return;

   mActiveGame->getSimObject()->setPaused(val);
}



//==============================================================================
long MultiplayerImpl::getGametime(void)
{
   if(!mGameInterface)
      return 0;
   else
      return mGameInterface->getGametime();
}

//==============================================================================
void MultiplayerImpl::initLogging(void)
{
   BFATAL_ASSERT(mGameInterface);
   if (!mGameInterface) 
      return;

   mGameInterface->initCommLogging();
}

//==============================================================================
BMPLANIPConnector* MultiplayerImpl::getLANIPConnector(void)
{
   if (!mStarted)
      return(NULL);

   if (!BConnectivity::getInstance()->isNetworkingEnabled())
      return(NULL);
   
   BFATAL_ASSERT(mGameInterface);
   if (!mGameInterface) 
      return(NULL);

   if (!mLANIPConnector)
      mLANIPConnector = new BMPLANIPConnector(getLocalAddress(), getTranslatedLocalAddress(),
                                              mGameInterface->getLocalChecksum());

   mLANIPRefCount++;
   return(mLANIPConnector);
}

//==============================================================================
void MultiplayerImpl::releaseLANIPConnector(BMPLANIPConnector* connector)
{
   if (connector != mLANIPConnector)
   {
      BFAIL("BMultiplayerImpl::releaseLANIPConnector -- wtf?");
      return;
   }

   mLANIPRefCount--;
}

//==============================================================================
void MultiplayerImpl::connectLANandDirectIP(void)
{
   mConnectType=cLANConnectType;
}

//==============================================================================
BMPLiveConnector* MultiplayerImpl::getLiveConnector(void)
{
	if (!mStarted)
		return(NULL);

	if (!BConnectivity::getInstance()->isNetworkingEnabled())
		return(NULL);

	BFATAL_ASSERT(mGameInterface);
	if (!mGameInterface) 
		return(NULL);

	if (!mLiveConnector)
    {
		mLiveConnector = new BMPLiveConnector(getLocalAddress(), getTranslatedLocalAddress(),
            mGameInterface->getLocalChecksum());
    }
    

	mLiveRefCount++;
	return(mLiveConnector);
}

//==============================================================================
void MultiplayerImpl::releaseLiveConnector(BMPLiveConnector* connector)
{
	if (connector != mLiveConnector)
	{
		BFAIL("BMultiplayerImpl::releaseLiveIPConnector -- wtf?");
		return;
	}

	mLiveRefCount--;
}

//==============================================================================
void MultiplayerImpl::connectLive(void)
{
	mConnectType=cLiveConnectType;
}

//==============================================================================
bool MultiplayerImpl::isLoggedIn()
{
   if (mConnectType==cLANConnectType)
      return(true);

   return(false);
}

//==============================================================================
bool MultiplayerImpl::attachFileXfer(BFileTransferGameInterface *xfer)
{
   if (!mStarted || !mActiveGame)
      return(false);

   mActiveGame->attachFileXfer(xfer);
   
   return(true);
}

//==============================================================================
bool MultiplayerImpl::attachVote(BMPVoteNotify* vote)
{
   if (!mStarted || !mActiveGame)
      return(false);

   mActiveGame->attachVoteNotify(vote);
   return(true);
}

//==============================================================================
bool MultiplayerImpl::detachVote(BMPVoteNotify* vote)
{
   if (!mStarted || !mActiveGame)
      return(false);

   mActiveGame->detachVoteNotify(vote);
   return(true);
}

//==============================================================================
void MultiplayerImpl::setGameState(long state)
{
   if (!mStarted || !mActiveGame)
      return;

   mActiveGame->setGameState(state);
}

//==============================================================================
void MultiplayerImpl::connectivityState(long state, HRESULT hr/*=S_OK*/)
{
   hr;
   if (state == BConnectivityObserver::cStateComplete)
   {
      mConnectivityEstablished = true;
   }
   else if (state == BConnectivityObserver::cStateFailed)
   {
      mConnectivityEstablished = false;
   }
}

//==============================================================================
void MultiplayerImpl::networkDisabled()
{
   mConnectivityEstablished = false;

   if (mGameInterface)
      mGameInterface->networkDisabled();
}

//==============================================================================
// eof: MultiplayerImpl.cpp
//==============================================================================