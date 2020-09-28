//==============================================================================
// modecampaign2.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mode.h"
#include "ui.h"
#include "uilist.h"
#include "renderThread.h"
#include "threading\eventDispatcher.h"
#include "flashmanager.h"
#include "flashgateway.h"
#include "campaignmanager.h"
#include "binkInterface.h"
#include "user.h"
#include "UIMoviePlayer.h"
#include "AsyncTaskManager.h"

class BUICampaignMenu;
class BUIMoviePlayer;

//==============================================================================
// BModeCampaign2
//==============================================================================
class BModeCampaign2 : public BMode, public BRenderCommandListener, public BBinkVideoStatus, public BMovieHandler, public BUIGlobals::yornHandlerInterface, public BAsyncNotify
{
   public:
      enum
      {
         cFlagModeUnused,
      };

      enum
      {
         cModeCampaignYornLiveSignin,
      };

      enum
      {
         cStateMain,
         cStateCampaignSignIntoLive,
         cStatePlayMovie,
         cStateStartSPCampaign,
         cStateContinueSPCampaign,
         cStateStartLanCampaign,
         cStateStartLiveCampaign,
         cStateGotoMainMenu,
         cStateReturnToPartyRoom,

         cStateCount,
      };

      enum
      {
         cInputContinueCampaign=0,
         cInputNewSPCampaign,
         cInputNewMPCampaign,
         cInputPlaySpecific,
         cInputIncrementNode,          // temp hack
         cInputResetNode,              // temp hack
         cInputNumStates,
      };

                        BModeCampaign2(long modeType);
      virtual           ~BModeCampaign2();

      // mode management functions
      virtual void      enter(BMode* lastMode);
      virtual void      leave(BMode* newMode);

      // mode rendering functions
      virtual void      renderBegin();
      virtual void      render();
      virtual void      renderEnd();


      // update and input methods
      virtual void      update();
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      // we had this on the screen, but really need it here because the screen class can go away, this class won't
      void notify( DWORD eventID, void* pTask );

      // BBinkVideoStatus listener callback methods
      void              onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode);
      void              onVideoCaptions(BBinkVideoHandle handle, bool enableCaptions, const BUString& caption);


      void              setNextState(long state) { mNextState=state; }
      void              setNextStateWithLiveCheck(long nextState, bool requireGold);

      // UI Globals yorn box
      void yornResult(uint result, DWORD userContext, int port);


      void updateSaveGameData();

      // play a movie from the campaign
      void playMovie(int campaignNode);

      // BMovieHandler interface
      void play();
      void pause();
      void togglePause();
      void stop();
      void skipBack();
      void rewind();
      void fastForward();
      void skipForward();
      bool isPaused() { return mVideoPaused; }

      void advanceMovie();

      void setBackgroundVideoHandle(BBinkVideoHandle vh) { mBackgroundVideoHandle = vh; }
      void setForegroundVideoHandle(BBinkVideoHandle vh) { mForegroundVideoHandle = vh; }

      void renderBackgroundVideos(bool bVisible) { mRenderBackgroundVideo = bVisible; }

      int  getPlayAllIndex() const { return mPlayAllIndex; }

      void endOfCampaign();
       
   protected:

      void signIn();
      void returnToPartyRoom();

      void createMoviePlayer();
      void destroyMoviePlayer();
      void createScreen();
      void destroyScreen();

      BUICampaignMenu*  mpScreenCampaign;
      BUIMoviePlayer*   mpScreenMoviePlayer;

      void renderVideo(BBinkVideoHandle videoHandle);

      long              mState;
      long              mNextState;
      long              mNextStateAfterSignin;
      BOOL              mSignInRequested;
      bool              mSignInGoldRequired;

      BUIList           mList;
      long              mLastMainItem;
      long              mSpecificNode;
      bool              mVideoPaused;
      BBinkVideoHandle  mCurrentVideoHandle;
      uint              mCurrentVideoSpeed;
      int               mPlayAllIndex;
      bool              mEndOfCampaign;
      bool              mWaitingForEndOfVideo;
      bool              mRenderBackgroundVideo;
      bool              mPreservePartyRoomReenter;



      BBinkVideoHandle  mBackgroundVideoHandle;
      BBinkVideoHandle  mForegroundVideoHandle;
};
