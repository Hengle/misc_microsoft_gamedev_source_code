//==============================================================================
// modecampaign2.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modecampaign2.h"
#include "civ.h"
#include "database.h"
#include "fontSystem2.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "leaders.h"
#include "modemanager.h"
#include "recordgame.h"
#include "render.h"
#include "ui.h"
#include "usermanager.h"
#include "user.h"
#include "configsgame.h"
#include "globalObjects.h"
#include "archiveManager.h"
#include "world.h"
#include "userprofilemanager.h"
#include "UICampaignMenu.h"
#include "UIMoviePlayer.h"
#include "ModePartyRoom2.h"

#ifndef BUILD_FINAL
#include "memory\allocationLogger.h"
#include "xdb\xdbManager.h"
#include "consoleRender.h"
#endif

// xvince
#include "vincehelper.h"

// flash 
#include "reloadManager.h"

#include "protoobject.h"
#include "LiveSystem.h"

#include "xmp.h"

// Constants
static const float cFirstNextSelectionTime=0.5f;
static const float cFastNextSelectionTime=0.1f;
static const float cTitleOffset=0.0f;

//==============================================================================
// BModeCampaign2::BModeCampaign2
//==============================================================================
BModeCampaign2::BModeCampaign2(long modeType) :
   BMode(modeType),
   mState(-1),
   mSpecificNode(-1),
   mNextState(cStateMain),
   mSignInGoldRequired(false),
   mNextStateAfterSignin(-1),
   mSignInRequested(FALSE),
   mLastMainItem(0),
   mpScreenMoviePlayer(NULL),
   mpScreenCampaign(NULL),
   mVideoPaused(false),
   mCurrentVideoHandle(cInvalidVideoHandle),
   mCurrentVideoSpeed(1),
   mBackgroundVideoHandle(cInvalidVideoHandle),
   mForegroundVideoHandle(cInvalidVideoHandle),
   mPlayAllIndex(-1),
   mEndOfCampaign(false),
   mWaitingForEndOfVideo(false),
   mRenderBackgroundVideo(true)
{
   commandListenerInit();
}

//==============================================================================
// BModeCampaign2::~BModeCampaign2
//==============================================================================
BModeCampaign2::~BModeCampaign2()
{
   commandListenerDeinit();
}

//==============================================================================
// BModeCampaign2::enter
//==============================================================================
void BModeCampaign2::enter(BMode* lastMode)
{
   mPreservePartyRoomReenter = false;
   // we need to do this just in case we come here from somewhere where the pregame UI is not loaded.
   bool success = gArchiveManager.beginMenu();
   BVERIFY(success);

   mPlayAllIndex = -1;

   BCampaign *pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNLOBBY);
   if (pCampaign->getPlayContinuous())
   {
      if (gConfig.isDefined(cConfigDemo2) && pCampaign->getCurrentNodeID() == 7)
      {
         createMoviePlayer();
         playMovie(pCampaign->getCurrentNodeID());
         mWaitingForEndOfVideo = true;
         mEndOfCampaign = true;
         pCampaign->setPlayContinuous(false);
         pCampaign->resetCurrentNode();
      }
      else
      {
         // play the next node.
         pCampaign->launchGame(true);
      }
   }
   else
   {
      mNextState = cStateMain;

      createScreen();
      createMoviePlayer();
   }

   if (!gConfig.isDefined(cConfigEnableAttractMode))
   {
      gUserManager.getSecondaryUser()->setFlagUserActive(false);
      gUserManager.setUserPort(BUserManager::cSecondaryUser, -1);
   }

   if (!gConfig.isDefined(cConfigNoMusic))
      gSoundManager.playSoundCueByEnum( BSoundManager::cSoundMusicSetStateCampaignMenu );

   return BMode::enter(lastMode);
}

//==============================================================================
// BModeCampaign2::leave
//==============================================================================
void BModeCampaign2::leave(BMode* newMode)
{
   gBinkInterface.unregisterValidCallback(this);

   stop();

   destroyScreen();
   destroyMoviePlayer();

   if (!mPreservePartyRoomReenter)
   {
      if (gLiveSystem->isPartySessionActive())
         gLiveSystem->setPartySystemForReenter(false);
   }

   BUser* currentUser = gUserManager.getPrimaryUser();
   if (!currentUser || !currentUser->getFlagUserActive())
   {
      gLiveSystem->setPartySystemForReenter(false);
      gLiveSystem->disposeMPSession();
      gLiveSystem->disposeVoice();
      //Extra line needed in case that system has already been told to recycle and at that time we were trying 
      //  to preserve the party system.  Well... Now we are NOT trying to do that so we need to tell that system to 
      //  shutdown (its safe for that system to recieve that request multiple times).
      if (gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->getPartySession())
      {
         gLiveSystem->getMPSession()->getPartySession()->shutdown();
      }
   }

   return BMode::leave(newMode);
}

//==============================================================================
// BModeCampaign2::update
//==============================================================================
void BModeCampaign2::update()
{
   BMode::update();

   if (mNextState != -1)
   {
      switch(mNextState)
      {
         // if the blade is up, we are waiting for the player to either sign in or cancel the sign in.
         // after the blade goes down, we will double check their choice and then we will determine 
         // what to do.
         case cStateCampaignSignIntoLive:
            {
               // if we are going into multiplayer and the system UI is up, then wait.
               if (gUserManager.isSystemUIShowing())
                  return;

               const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
               BASSERT(pUser);

               if (!pUser->isSignedIntoLive())
               {
                  // the UI is not up any more, and the user failed to sign in, stay in our current state.
                  mNextState = -1;
                  return;
               }

               if (mSignInGoldRequired && !pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
               {
                  // the UI is not up any more, and the user failed to sign in, stay in our current state.
                  mNextState = -1;
                  return;
               }
               // We are logged in, set our next state and go to it.
               mNextState = mNextStateAfterSignin;
               return;
            }
            break;


         case cStateStartSPCampaign:
         case cStateContinueSPCampaign:
         case cStateStartLanCampaign:
         case cStateReturnToPartyRoom:
            break;



         case cStateStartLiveCampaign:
            {
               // do a quick check on the user.
               const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
               BASSERT(pUser);
               if (!pUser->isSignedIntoLive())
               {
                  // the UI is not up any more, and the user failed to sign in, stay in our current state.
                  mNextState = -1;
                  return;
               }
            }
            break;

         case cStatePlayMovie:
            break;

         case cStateGotoMainMenu:
            break;

         case cStateMain:
            break;
      }

      // bounds check and transition into the state.
      if ( (mNextState>=0) && (mNextState<cStateCount))
         mState = mNextState;

      mNextState = -1;
   }

   switch (mState)
   {
      case cStateMain:
         if( mpScreenCampaign && mCurrentVideoHandle == cInvalidVideoHandle )
            mpScreenCampaign->update( 0 );
         break;
      case cStateStartSPCampaign:
         gModeManager.setMode(BModeManager::cModeGame);     // the screen should have set everything up.
         gLiveSystem->setPresenceContext(PROPERTY_GAMETIMEMINUTES, 0, true);
         gLiveSystem->setPresenceContext(PROPERTY_GAMESCORE, 0, true);
         gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNSOLOPLAY);
         break;
      case cStateReturnToPartyRoom:
         {
            // Get the party room mode
            BModePartyRoom2* pMode = (BModePartyRoom2*)gModeManager.getMode(BModeManager::cModePartyRoom2);
            if (!pMode)
            {
               // how did this happen? clean up just in case
               mPreservePartyRoomReenter=false;
               mNextState = cStateMain;
               createScreen();
               break;
            }

            // get the flag to see if we are in LAN mode
            bool isLanMode = gLiveSystem->getMPSession()->isInLANMode();

            if (!isLanMode)
            {
               // if we are NOT in LAN mode, we need to validate that we are still signed into LIVE
               const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
               BASSERT(pUser);
               if (!pUser->isSignedIntoLive())
               {
                  // We are not signed into LIVE, ergo go to the main screen.
                  // the UI is not up any more, and the user failed to sign in, stay in our current state.
                  mPreservePartyRoomReenter=false;
                  mNextState = cStateMain;
                  createScreen();
                  return;
               }
            }

            // We passed our checks, go on to the LIVE screen.

            // set the next mode, and tell them we want the campaign view.
            gModeManager.setMode(BModeManager::cModePartyRoom2); 
            pMode->setInitialMode(BModePartyRoom2::cPartyRoomModeCampaign);
            pMode->setUseLanMode(isLanMode);
            break;
         }
      case cStateStartLanCampaign:
         {
            BModePartyRoom2* pMode = (BModePartyRoom2*)gModeManager.getMode(BModeManager::cModePartyRoom2);
            if (!pMode)
               break;

            // set the next mode, and tell them we want the campaign view.
            gModeManager.setMode(BModeManager::cModePartyRoom2); 
            pMode->setInitialMode(BModePartyRoom2::cPartyRoomModeCampaign);
            pMode->setUseLanMode(true);
            break;
         }
      case cStateStartLiveCampaign:
         {
            BModePartyRoom2* pMode = (BModePartyRoom2*)gModeManager.getMode(BModeManager::cModePartyRoom2);
            if (!pMode)
               break;

            // set the next mode, and tell them we want the campaign view.
            gModeManager.setMode(BModeManager::cModePartyRoom2); 
            pMode->setInitialMode(BModePartyRoom2::cPartyRoomModeCampaign);
         }
         break;
      case cStateGotoMainMenu:
         gModeManager.setMode(BModeManager::cModeMenu);
         break;
      case cStatePlayMovie:
         mpScreenMoviePlayer->update(0);
         break;
   }

   // update end of campaign
   if (mEndOfCampaign && !mWaitingForEndOfVideo)
   {
      if (gConfig.isDefined(cConfigDemo2))
      {
         mEndOfCampaign = false;
         mNextState = cStateMain;         
      }
      else
      {
         BCampaign * pCampaign = gCampaignManager.getCampaign(0);
         BASSERT(pCampaign);

         BCampaignNode* pNode = pCampaign->getNode(pCampaign->getCurrentNodeID());

         while (pNode)
         {
            // if it's a cinematic, qualify that we can play it
            if (pNode->getFlag(BCampaignNode::cCinematic))
            {
               if (!pNode->getFlag(BCampaignNode::cFlagLegendary))
                  break;      // we have a cinematic and it's not legendary, therefore we can play it.

               BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
               if (pProgress != NULL)
               {
                  int lastScenario = pProgress->getNumberOfScenarioNodes()-1;
                  if (pProgress->isScenarioCompletedByProgressIndex(lastScenario, -1, DifficultyType::cLegendary))
                     break;      // it's a legendary clip, but we can play it.
               }
            }

            pNode=NULL;

            // not a cinematic (or we are not supposed to play it), increment the node and get the next one.
            if (!pCampaign->incrementCurrentNode())
            {
               stop();

               gSoundManager.overrideBackgroundMusic(false);

               mEndOfCampaign = false;
               pCampaign->resetCurrentNode();        // for a lack of a better idea, reset our current node to the first one. You'll still have all the nodes unlocked.
               
               // we are all done, do we go back to the party room or go to the menu?
               if (gLiveSystem->isPartySessionActive())
               {
                  returnToPartyRoom();
                  return;
               }

               createScreen();
               // we have no more nodes, let's get out of here.
               break;
            }

            pNode = pCampaign->getNode(pCampaign->getCurrentNodeID());
         }

         if (!pNode)
            mNextState = cStateMain;         
         else
         {
            playMovie(pCampaign->getCurrentNodeID());
            mWaitingForEndOfVideo = true;
         }
      }
   }

   //Is the mission picker screen visible - if so - let it update this presence
   if (mpScreenCampaign && mpScreenCampaign->isMissionPickerVisible())
   {
      return;
   }

   //If we are not in the base mode - then also don't set presence
   if (mState!=cStateMain)
   {
      return;
   }
  
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   if (pCampaign)
   {
      gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_DIFFICULTY, pCampaign->getCurrentDifficulty());
      int currentNode = pCampaign->getCurrentNodeID();
      if (currentNode==-1)
      {
         currentNode = 0;
      }
      BCampaignNode* pNode = pCampaign->getNode(currentNode);
      if (pNode)
      {
         gLiveSystem->setPresenceContext(PROPERTY_MISSIONINDEX, pNode->getLeaderboardLevelIndex(), true);
      }
   }
}

//==============================================================================
// BModeCampaign2::renderBegin
//==============================================================================
void BModeCampaign2::renderBegin()
{
   gRender.beginFrame(1.0f/30.0f);
   gRender.beginViewport(-1);
   
   gRenderDraw.beginScene();
   gRenderDraw.clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
}

//==============================================================================
// BModeCampaign2::render
//==============================================================================
void BModeCampaign2::render()
{
   if (mCurrentVideoHandle == cInvalidVideoHandle)
   {
      // [7/30/2008 xemu] added this to differentiate between background and foreground videos 
      if (mRenderBackgroundVideo)
         renderVideo(mBackgroundVideoHandle);

      // renderBegin called just before this.
      if (mpScreenCampaign)
         mpScreenCampaign->render();

      renderVideo(mForegroundVideoHandle);
   }
   else
      renderVideo(mCurrentVideoHandle);

   #ifndef BUILD_FINAL
      gConsoleRender.renderSubmit();
   #endif


   // render this after the videos so it sits on top
   if (mpScreenMoviePlayer)
      mpScreenMoviePlayer->render();

   renderSafeAreas();
   // renderEnd called after this
}

//==============================================================================
// BModeCampaign2::renderEnd
//==============================================================================
void BModeCampaign2::renderEnd()
{
   gRenderDraw.endScene();
   gRenderDraw.present(NULL, NULL);
   
   gRender.endViewport();
   gRender.endFrame();
   // Let's sleep for a bit here because I doubt the campaign sim update will take much time.
   gEventDispatcher.sleep(1);
}


//==============================================================================
// BModeCampaign2::handleInput
//==============================================================================
bool BModeCampaign2::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   port; detail;

   if (!gConfig.isDefined(cConfigEnableAttractMode))
   {
//-- FIXING PREFIX BUG ID 1496
      const BUser* pUser = gUserManager.getPrimaryUser();
//--
      if (port != pUser->getPort())
      {
         if (gGame.isSplitScreenAvailable())
         {
            BUser* pUser2 = gUserManager.getSecondaryUser();
            if (mState == cStateMain)
            {
               if (pUser2->getFlagUserActive())
               {
                  if (controlType == cButtonB)
                  {
                     gUserManager.setUserPort(BUserManager::cSecondaryUser, -1);
                     pUser2->setFlagUserActive(false);
                     return true;
                  }
               }
               else
               {
                  if (controlType == cButtonA)
                  {
                     gUserManager.setUserPort(BUserManager::cSecondaryUser, port);
                     pUser2->setFlagUserActive(true);
                     return true;
                  }
               }
            }
         }
         return false;
      }
   }

   // let our screen see the input
   bool handled = false;

   switch (mState)
   {
      case cStateMain:
         // ajl 11/18/08 - Don't handle input while we are creating the save file since that's done in the background.
         if (gSaveGame.getSaveRequestInProgress() || gUserManager.getShowingDeviceSelector())
            return true;
         if (mpScreenCampaign)
            handled = mpScreenCampaign->handleInput(port, (BInputEventType)event, (BInputControlType)controlType, detail);
         break;

      case cStatePlayMovie:
         if (mpScreenMoviePlayer)
            handled = mpScreenMoviePlayer->handleInput(port, (BInputEventType)event, (BInputControlType)controlType, detail);
         break;
   }

   return handled;
}

//==============================================================================
//==============================================================================
void BModeCampaign2::returnToPartyRoom()
{
   mPreservePartyRoomReenter=true;
   //gLiveSystem->setPartySystemForReenter(true);

   // set our next state
   mNextState = cStateReturnToPartyRoom;

   //Tell mpSession to send a message that we have left the game session (lol)
   if (gLiveSystem->getMPSession())
   {
      gLiveSystem->getMPSession()->sendBelayedGameDisconnectEvent();
   }
}


//==============================================================================
// BModeCampaign2::cinematicEnded, BBinkVideoStatus interface method
//==============================================================================
void BModeCampaign2::onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode)
{
   // If for some reason we got preloaded data back (not expected here at the moment), delete it since otherwise
   // it would be orphaned.
   if(preloadedData)
   {
      delete preloadedData;
      preloadedData = NULL;
   }
      
   if (mEndOfCampaign)
   {
      mWaitingForEndOfVideo = false;

      BCampaign * pCampaign = gCampaignManager.getCampaign(0);
      BASSERT(pCampaign);

      if (gConfig.isDefined(cConfigDemo2) || !pCampaign->incrementCurrentNode())
      {
         stop();

         gSoundManager.overrideBackgroundMusic(false);
         XMPRestoreBackgroundMusic();
         XEnableScreenSaver(true);

         mEndOfCampaign = false;
         pCampaign->resetCurrentNode();        // for a lack of a better idea, reset our current node to the first one. You'll still have all the nodes unlocked.

         // we are all done, do we go back to the party room or go to the menu?
         if (gLiveSystem->isPartySessionActive())
         {
            returnToPartyRoom();
            return;
         }
         createScreen();
      }
   }
   else
   {
      advanceMovie();
   }
}

//==============================================================================
//==============================================================================
void BModeCampaign2::onVideoCaptions(BBinkVideoHandle handle, bool enableCaptions, const BUString& caption)
{
   // if there is a movie player screen up, then send the captioning data to it
   if (mpScreenMoviePlayer)
      mpScreenMoviePlayer->setCaptionData(enableCaptions, caption);
}



//==============================================================================
// BModeGame::RenderVideos
//==============================================================================
void BModeCampaign2::renderVideo(BBinkVideoHandle videoHandle)
{
   if (videoHandle == cInvalidVideoHandle)
      return;

   if (!mVideoPaused)
      gBinkInterface.decompressVideo(videoHandle);

   gBinkInterface.renderVideo(videoHandle);

   if (!mVideoPaused)
      gBinkInterface.advanceVideo(videoHandle);
}

//============================================================================
//============================================================================
void BModeCampaign2::playMovie(int nodeIndex)
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   // [8/25/2008 xemu] suppress the menu background music
   gSoundManager.overrideBackgroundMusic(true);
   XMPOverrideBackgroundMusic();
   XEnableScreenSaver(false);

   int useIndex = nodeIndex;
   // [8/12/2008 xemu] if we are in "play all" mode, then just use our bookmark position
   if (nodeIndex == -1)
   {
      if (mPlayAllIndex == -1)
         mPlayAllIndex = mpScreenCampaign->getNextPlayAllIndex();
      useIndex = mPlayAllIndex;
   }

//-- FIXING PREFIX BUG ID 1497
   const BCampaignNode* pNode = pCampaign->getNode(useIndex);
//--
   if (pNode)
   {
      BSimString captionFile;

      bool showSubtitles=true;
      // check to see if subtitles are turned on.
      if (gUserManager.getPrimaryUser())
         showSubtitles=gUserManager.getPrimaryUser()->getOption_SubtitlesEnabled();

      if (showSubtitles)
         captionFile = pNode->getCaptionsFile();

      BBinkInterface::BLoadParams lp;
      lp.mFilename.set(pNode->getFilename());
      lp.mCaptionDirID = cDirData;
      lp.mCaptionFilename.set(captionFile.getPtr());
      lp.mpStatusCallback = this;
      lp.mLoopVideo = false;
      lp.mFullScreen = true;
      //lp.mDisableXMPMusic = true;  //Turn xbox media player music off while playing fullscreen cinematics
      mCurrentVideoHandle =gBinkInterface.loadActiveVideo(lp); 
      gBinkInterface.registerValidCallback(this);

     // mCurrentVideoHandle = gBinkInterface.loadActiveVideoFullScreen(pNode->getFilename(), cDirData, captionFile.getPtr(), "", this, false);
      mVideoPaused = false;

      gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_MOVIENAME, pNode->getPresenceMovieIndex());
   }

   mpScreenMoviePlayer->setVisible(true);
   mpScreenMoviePlayer->clearCaptionText();
   mpScreenMoviePlayer->doHide();
   gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_WATCHINGMOVIE);

   mNextState=cStatePlayMovie;
}


//============================================================================
//============================================================================
void BModeCampaign2::play()
{
   mCurrentVideoSpeed = 1;
   gBinkInterface.playVideo(mCurrentVideoHandle);
   mVideoPaused = false;
}

//============================================================================
//============================================================================
void BModeCampaign2::pause()
{
   mVideoPaused = true;
   gBinkInterface.pauseVideo(mCurrentVideoHandle);
}

//============================================================================
void BModeCampaign2::togglePause()
{
   if (!mVideoPaused)
      pause();
   else
      play();
}

//============================================================================
//============================================================================
void BModeCampaign2::stop()
{
   mPlayAllIndex = -1;

   gBinkInterface.stopVideo(mCurrentVideoHandle, true);        // we need to be notified when the video is complete because we play a series of videos at the end of the campaign.
   mVideoPaused=false;
   mCurrentVideoHandle = cInvalidVideoHandle;

   // go back to our regular state.
   if (mpScreenMoviePlayer)
      mpScreenMoviePlayer->setVisible(false);      // fixme - if we do an action script call here, we probably want to make this transition in update through the state system.
   if (mState==cStatePlayMovie)
      gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNLOBBY);
   mNextState=cStateMain;

}

//============================================================================
//============================================================================
void BModeCampaign2::skipBack()
{
   mVideoPaused = true;
}

//============================================================================
//============================================================================
void BModeCampaign2::rewind()
{
   mCurrentVideoSpeed+=mCurrentVideoSpeed;
   gBinkInterface.rewindVideo(mCurrentVideoHandle,mCurrentVideoSpeed);
   mVideoPaused=false;
}

//============================================================================
//============================================================================
void BModeCampaign2::fastForward()
{
   mCurrentVideoSpeed+=mCurrentVideoSpeed;
   gBinkInterface.fastForwardVideo(mCurrentVideoHandle,mCurrentVideoSpeed);
   mVideoPaused=false;
}

//============================================================================
//============================================================================
void BModeCampaign2::skipForward()
{
   mVideoPaused = true;
}

//============================================================================
//============================================================================
void BModeCampaign2::advanceMovie()
{
   // [8/12/2008 xemu] query for the next video to play, if we are in play all mode 
   if (mPlayAllIndex != -1)
      mPlayAllIndex = mpScreenCampaign->getNextPlayAllIndex();

   // [8/12/2008 xemu] terminate if done.  note this can be set as a result of getNextPlayAllIndex if we are at the end of the unlocked videos. 
   if (mPlayAllIndex == -1)
   {
      // go back to our regular state.
      if (mpScreenMoviePlayer != NULL)
         mpScreenMoviePlayer->setVisible(false);      // fixme - if we do an action script call here, we probably want to make this transition in update through the state system.
      gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNLOBBY);

      // [9/24/2008 xemu] make sure any in-progress movie was cleaned up
      stop();

      // [8/25/2008 xemu] restore the menu background music
      gSoundManager.overrideBackgroundMusic(false);
      XMPRestoreBackgroundMusic();
      XEnableScreenSaver(true);

	  // Make sure we check for null, as mpScreenCampaign is freed in destroyScreen(), which can be called before
	  // this function if we are terminating playback due to the active user signing out.
	  if (mpScreenCampaign != NULL)
	      mpScreenCampaign->refreshMoviePicker();

      mNextState=cStateMain;
   }
   else
   {
      // [9/30/2008 xemu] kill the old movie to avoid audio starvation 
      gBinkInterface.stopVideo(mCurrentVideoHandle, false);
      mVideoPaused=false;
      mCurrentVideoHandle = cInvalidVideoHandle;

      // [8/13/2008 xemu] play the next video!
      playMovie(mPlayAllIndex);
   }
}

//============================================================================
//============================================================================
void BModeCampaign2::endOfCampaign()
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   int currentNode = pCampaign->getCurrentNodeID();
   if (currentNode < 0)
      currentNode=0;

   // make sure the movie player exists
   createMoviePlayer();

   mWaitingForEndOfVideo=false;
   mEndOfCampaign = true;
}

//============================================================================
//============================================================================
void BModeCampaign2::createMoviePlayer()
{
   if (!mpScreenMoviePlayer)
   {
      mpScreenMoviePlayer=new BUIMoviePlayer();
      mpScreenMoviePlayer->init("art\\ui\\flash\\pregame\\MoviePlayer\\UIMoviePlayerScreen.gfx", "art\\ui\\flash\\pregame\\MoviePlayer\\UIMoviePlayerScreenData.xml");
      mpScreenMoviePlayer->setMovieHandler(this);

#ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigShowSafeArea))
         mpScreenMoviePlayer->showSafeAreaGuide(true);
#endif
      mpScreenMoviePlayer->setVisible(false);
   }
}

//============================================================================
//============================================================================
void BModeCampaign2::destroyMoviePlayer()
{
   if (mpScreenMoviePlayer)
   {
      mpScreenMoviePlayer->deinit();
      delete mpScreenMoviePlayer;
      mpScreenMoviePlayer=NULL;
   }
}

//============================================================================
//============================================================================
void BModeCampaign2::createScreen()
{
   if (!mpScreenCampaign)
   {
      mpScreenCampaign = new BUICampaignMenu();
      mpScreenCampaign->init("art\\ui\\flash\\pregame\\CampaignMenu\\UICampaignScreen.gfx", "art\\ui\\flash\\pregame\\CampaignMenu\\UICampaignScreenData.xml");
      mpScreenCampaign->setVisible(true);
#ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigShowSafeArea))
         mpScreenCampaign->showSafeAreaGuide(true);
#endif

   }

   //-- Campaign menu music
   if (!gConfig.isDefined(cConfigNoMusic))
      gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateCampaignMenu);

}

//============================================================================
//============================================================================
void BModeCampaign2::destroyScreen()
{
   if (mpScreenCampaign)
   {
      mpScreenCampaign->deinit();
      delete mpScreenCampaign;
      mpScreenCampaign=NULL;
   }
}

//==============================================================================
//==============================================================================
void BModeCampaign2::setNextStateWithLiveCheck(long nextState, bool requireGold)
{
   const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   BASSERT(pUser);

   BUIGlobals* puiGlobals = gGame.getUIGlobals();

   mSignInGoldRequired = requireGold;

   // 24058 An Xbox LIVE Gold Membership or permission for Online Gameplay is required. Press $$A$$ to sign into an appropriate gamer profile.
   // 25546 An Xbox LIVE gamer profile is required. Press $$A$$ to sign into an appropriate gamer profile.
   // 25566 Your gamer profile is not signed into Xbox LIVE. Press $$A$$ to sign-in now.

   if (!pUser->isSignedIn())
   {
      // show the yorn box to ask if they want to sign in.
      if (puiGlobals)
      {
         mNextStateAfterSignin = nextState;
         if (requireGold)
            puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24058), BUIGlobals::cDialogButtonsOKCancel, cModeCampaignYornLiveSignin);
         else
            puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(25546), BUIGlobals::cDialogButtonsOKCancel, cModeCampaignYornLiveSignin);
      }

      return;
   }

   // if the user is signed-in, they still may not be signed into LIVE

   // if they are LIVE enabled but not signed into LIVE
   if (pUser->isLiveEnabled() && !pUser->isSignedIntoLive())
   {
      mNextStateAfterSignin = nextState;
      puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(25566), BUIGlobals::cDialogButtonsOKCancel, cModeCampaignYornLiveSignin);
      return;
   }
   else if (!pUser->isLiveEnabled())
   {
      // if they're not LIVE enabled, ask them to choose a different profile

      //If their current account is not ENABLED for Live access at all (either by priv or by being Silver) then ask them if they want to switch
      mNextStateAfterSignin = nextState;
      puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24058), BUIGlobals::cDialogButtonsOKCancel, cModeCampaignYornLiveSignin);
      return;
   }

   // otherwise the user is signed into LIVE by default

   // if we don't require a Gold gamer profile or they have the MP privilege, then set the next state and return
   if (!requireGold || pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS))
   {
      mNextState = nextState;
      return;
   }

   // otherwise we require a Gold gamer profile and they lack the MP privilege
   // either because it's a Silver profile or parental controls

   mNextStateAfterSignin = nextState;
   puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(24058), BUIGlobals::cDialogButtonsOKCancel, cModeCampaignYornLiveSignin);
   return;
}

//==============================================================================
//==============================================================================
void BModeCampaign2::yornResult(uint result, DWORD userContext, int port)
{
   switch (userContext)
   {
   case cModeCampaignYornLiveSignin:
      {
         switch(result)
         {
         case BUIGlobals::cDialogResultOK:
            signIn();
            mNextState = cStateCampaignSignIntoLive;
            break;
         case BUIGlobals::cDialogResultCancel:
            break;
         }
      }
      break;
   }
}

//==============================================================================
//==============================================================================
void BModeCampaign2::signIn()
{
   // sign-in UI will always display if needed
   const BUser* const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   BASSERT(pUser);

   // if the current user is not signed into LIVE or a gold account is required and they do not have the MP privilege
   // then show the sign-in UI
   if (!pUser->isSignedIntoLive() || (mSignInGoldRequired && !pUser->checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS)))
   {
      gUserManager.showSignInUI();

      mSignInRequested = TRUE;
   }
}


//==============================================================================
//==============================================================================
void BModeCampaign2::updateSaveGameData()
{
   if (mpScreenCampaign && mpScreenCampaign->getVisible())
      mpScreenCampaign->updateSaveGameData();
}

//============================================================================
//============================================================================
void BModeCampaign2::notify( DWORD eventID, void* pTask )
{
   // default
   if( !pTask || !mpScreenCampaign)
   {
      gGame.getUIGlobals()->setWaitDialogVisible(false);
      return;
   }

   // BSelectDeviceAsyncTask* pSelectDeviceTask = reinterpret_cast<BSelectDeviceAsyncTask*>( pTask );
   XCONTENTDEVICEID deviceID = gUserManager.getPrimaryUser()->getDefaultDevice();

   if( deviceID == XCONTENTDEVICE_ANY )
   {
      gGame.getUIGlobals()->setWaitDialogVisible(false);

      // Show a dialog 
      gGame.getUIGlobals()->showYornBox(this, gDatabase.getLocStringFromID(25986), BUIGlobals::cDialogButtonsOK);
      return;
   }

   // Give the blade some time to animate closing
   DWORD startTime = timeGetTime();
   while (timeGetTime() - startTime < 300)
   {
      gGame.updateRender();
      Sleep(1);
   }

   if (!gSaveGame.getCampaignSaveExists(NULL))
   {
      if (!gSaveGame.createNewCampaignFile())
      {
         // Show a dialog warning save file could not be created.
         gGame.getUIGlobals()->setWaitDialogVisible(false);
         gGame.getUIGlobals()->showYornBox( this, gDatabase.getLocStringFromID(25997), BUIGlobals::cDialogButtonsOK);
         return;
      }
   }

   // query for save game info
   mpScreenCampaign->saveGameExists(); // get save game info
   gGame.getUIGlobals()->setWaitDialogVisible(false);
}
