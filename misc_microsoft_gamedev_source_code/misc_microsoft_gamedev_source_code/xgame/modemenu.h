//==============================================================================
// modemenu.h
//
// Copyright (c) 2005-2008 Ensemble Studios
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
#include "binkInterface.h"
#include "timer.h"
#include "UIGlobals.h"
#include "UIButtonBar.h"
#include "simtypes.h"
#include "UIScreen.h"
#include "AsyncTaskManager.h"

class BUIMainMenuScreen;
class BBackgroundMovies;

//==============================================================================
// BModeMenu
//==============================================================================
class BModeMenu : public BMode, public BBinkVideoStatus, public BUIGlobals::yornHandlerInterface, public IUIScreenHandler, public BAsyncNotify
{
   public:
      enum
      {
         cFlagModeUnused,
      };

      enum 
      {
         cModeMenuYornLiveSignin,
         //cModeMenuYornAuthSignin,
         //cModeMenuYornAuthSigninFail,
         cModeMenuYornExit,
         cModeMenuOkayLoadError,
         cModeMenuYornChooseDevice,
         cModeMenuYornChooseDeviceNone,
      };

      enum
      {
         cModeMenuMovieTypeDefault,
         cModeMenuMovieTypeCampaign,
         cModeMenuMovieTypeSpecific,
         cModeMenuMovieCredits,        // the credits movie
      };

      enum
      {
         cStateIntroCinematic,

         //cStateInitialSignin,
         //cStateAuthorizing,
         //cStateAuthorizationFailed,

         cStateIntroCinematicEnded,

         cStateAttractMode,

         cStateMain,

         cStateSignIntoLive,

         // Main Menu Items
         cStateGotoCampaign,
         cStateSkirmish,
         cStateGotoMultiplayer,
         cStateGotoSystemLink,
         cStateLeaderboards,
         cStateOptions,
         cStateSkirmishSetup,
         cStateTimeline,
         cStateCreditsMovie,
         cStateStartCampaign,
         cStateContinueCampaign,

         // Options Menu Items         
         cStateMultiplayerMenu,

         // Extras Menu Items
         cStateRecordGame,
         cStateSaveGame,
         cStateCredits,

         cStateWaitForDevice,

         // Other Menu Items
         cStateScenario,
         cStatePlayerSettings,
         cStateFlash,
         cStateCinematic,
         cStateModelView,
         cStateGotoGame,
         cStateGotoViewer,
         cStateGotoCalibrate,
         cStateGotoRecordGame,
         cStateGotoSaveGame,
         cStateGotoFlash,
         cStateGotoCinematic,
         cStateGotoModelView,
         //cStatePlayDemoMovie,             //E3 crap - eric
         cStateExit,
         cStateReallyExit, // cStateExit only prompts us with a dialog, this really exits         
         cStateServiceRecord,
         cStateLoadError,
         cStateInviteFromOtherController,
      };

                        BModeMenu(long modeType);
      virtual           ~BModeMenu();

      virtual bool      setup();
      virtual void      shutdown();

      virtual void      preEnter(BMode* lastMode);
      virtual void      enter(BMode* lastMode);
      virtual void      leave(BMode* newMode);

      virtual void      renderBegin();
      virtual void      render();
      virtual void      renderEnd();
      virtual void      update();
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      long              getState() { return mState; }
      void              setNextState(long state);
      long              getNextState() const { return mNextState; }
      void              setNextStateWithLiveCheck(long nextState, bool requireGold);


      BSimString&       getSelectedFileName() { return mSelectedFileName; }
      void              setSelectedFileName(const char* filename) { mSelectedFileName = filename; }

      // Bink Callback
      void              onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode);
      void              onVideoCaptions(BBinkVideoHandle handle, bool enableCaptions, const BUString& caption);

      void              startDemo();
      void              startTutorial();

      enum ETutorial { cTutorialBasic, cTutorialAdvanced };
      void playTutorial( ETutorial cTutorial, long cStartContext = 0 /*BGameSettings::cGameStartContextNone*/ );

      // yorn handler
      virtual void yornResult(uint result, DWORD userContext, int port);

      virtual void handleUIScreenResult( BUIScreen* pScreen, long result );
      
      void setMenuItemFocus(int menuItemFocus) { mMenuItemFocus = menuItemFocus; }
      void loadBackgroundMovie(int movieType, const char * pMovieName=NULL, bool loopVideo=true, const char * pCaptionFileName=NULL );

      void play();
      void pause();
      void togglePause();
      const bool isPaused() const { return mVideoPaused; }

      virtual void notify( DWORD eventID, void* pTask );

      
   protected:

      bool checkSaveDevice();

      void controlStart(long direction);
      void controlUpdate();
      void controlStop();
      void controlChangeSelection();
      void controlChangeSkirmishGameSettingsSelection();    // control selection changing for game settings
      void controlChangePlayerSettingsSelection();          // control selection changing for player settings

      void renderVideos();
      void updateExit();

      void signIn();
      //void checkAlphaSignIn();

      void pickNextLeader(long* pLeaderID, long dir);

      void clearLeaders();

      void updateScenarioList();

      bool getDifficultyValueForType( long type, float& value );

      BString           mCurrentPlayingMovie;

      BUString          mMessageString;
      BOOL              mSignInRequested;
      bool              mSignInGoldRequired;

      long              mState;
      long              mNextState;
      long              mNextStateAfterSignin;
      long              mGotoGameFromState;
      BUIList           mList;
      BDynamicSimArray<BSimString>  mFileList;
      BDynamicSimArray<int>         mFileNameIDList;
      BDynamicSimArray<int>         mFileInfoIDList;
      long              mScenarioFilter;

      //-- Menu Item indexes
      int               mMenuItemFocus;

      long              mLastScenarioItem;
      long              mLastFlashItem;
      long              mLastCinematicItem;
      long              mLastModelViewItem;

      bool              mVideoPaused;
      BSimString        mSelectedFileName;
      BBinkVideoHandle  mBackgoundVideoHandle;
      BBinkVideoHandle  mDemoVideoHandle;

      enum { cSettingPlayers, cSettingMap, cSettingGameMode, cSettingRecordGame, cSettingPlayer1, cSettingPlayer2, cSettingPlayer3, cSettingPlayer4, cSettingPlayer5, cSettingPlayer6 };
      long        mSetting;
      long        mSettingCurrentPlayer;
      long        mCurrentPlayerSetting;
      long        mSettingPlayers;
      BSimString  mSettingMap;
      long        mSettingGameMode;
      bool        mSettingRecordGame;

      enum { cPlayerSettingTeam, cPlayerSettingDifficulty, cPlayerSettingCiv, cPlayerSettingLeader };
      float             mPlayerSettingDifficulty;
      BDifficultyType   mPlayerSettingDifficultyType;
      long        mPlayerSettingTeam;
      long        mPlayerSettingCiv;
      long        mPlayerSettingLeader;

      DWORD mIntroVideoMinTime;
      bool  mIntroVideoDone;

      bool  mControlling;
      long  mControlDirection;
      float mControlTime;
      bool  mControlFast;
      DWORD mLastTime;

      uint mRecordingsPort;

      //-- Flash UI 
      BUIScreen*                 mpUIScreen;
      BUIMainMenuScreen*         mpMainMenuScreen;
      BBackgroundMovies*         mpBackgroundMovies;

      uint8                      mLastRecordGameUpdate;

      void displayDebugInfo();
};
