//==============================================================================
// game.h
//
// Copyright (c) Ensemble Studios, 2005-2007
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "mpSession.h"
#include "soundmanager.h"
#include "UIGlobals.h"

// Forward declarations
class BGame;

// Global variable for the one BGame object
extern BGame gGame;


//==============================================================================
// BGame
//==============================================================================
class BGame : 
   public IInputEventHandler,
   public BMPSession::GameInterface,
   public BUIGlobals::yornHandlerInterface
{
   BGame(const BGame&);
   BGame& operator= (const BGame&);
   
   public:      

                        BGame();
                        ~BGame();

      bool              setup(const char* title, int commandShow, bool childWindowMode);
      int               run();
      bool              update();
      void              exit();
      void              shutdown();

      DWORD             getTotalTime() const { return mTotalTime; }
      DWORD             getFrameTime() const { return mFrameTime; }

      void              setActive(bool active) { setFlagActive(active); }
      bool              getActive() const { return getFlagActive(); }   

      bool              isSplitScreenAvailable();
      void              setSplitScreen(bool val);
      bool              isSplitScreen() const { return mFlagSplitScreen; }
      bool              isVerticalSplit() const { return mFlagVerticalSplit; }
      bool              isVinceLogOpen() const { return mFlagVinceLogOpen; }

      void              openVinceLog();
      void              closeVinceLog();

      void              startMusic();
      void              stopMusic();
      
      // IInputEventHandler
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      // BMPSession::GameInterface
      virtual void      setupSync(BMPSyncObject* object);
      virtual BDataSet* getGameDataSet();
      virtual DWORD     getLocalChecksum();
      virtual bool      populateDefaultGameInfo(BMPGameView* gameView) { gameView; return true; }
      virtual bool      startGameSync();
      virtual void      initCommLogging();
      virtual bool      isOOS();
      virtual void      networkDisabled();
      virtual long      getDataDirID();
      virtual long      getGametime();
      BUIGlobals*       getUIGlobals() { return mpUIGlobals; }

      //Debugging Pathing
      // Pathing Quads
      long              getRenderPathingQuad() const {return mRenderPathingQuad;}
      void              setRenderPathingQuad(int quad) {mRenderPathingQuad = quad;}
      void              toggleRenderPathingQuad(void) {mRenderPathingQuad++; if (mRenderPathingQuad > 4) mRenderPathingQuad = 0;}
      long              getRenderLRPTreeType() const {return mRenderLRPTreeType;}
      void              toggleRenderLRPTreeType(void) {mRenderLRPTreeType++; if (mRenderLRPTreeType > 2) mRenderLRPTreeType = 0;}
      // Obstructions
      int               getObstructionRenderMode( void ) const { return mObstructionRenderMode; }
      void              setObstructionRenderMode( int v ) {mObstructionRenderMode = v; }
      void              incrementObstructionRenderMode( void ) { mObstructionRenderMode++; if (mObstructionRenderMode > 4) mObstructionRenderMode = 0;}
      // Paths
      long              getShowPaths() const { return mRenderShowPaths; }
      void              setShowPaths(int pathLevel) { mRenderShowPaths = pathLevel; }
      void              incrementShowPaths() { mRenderShowPaths++; if (mRenderShowPaths > 3) mRenderShowPaths = 0; }
      // AI
      int               getAIDebugType() const { return (mAIDebugType); }
      void              setAIDebugType(int v);
      void              incAIDebugType();
      void              decAIDebugType();


      //-- Flags
      void              clearFlags();
      bool              getFlagActive() const { return(mFlagActive); }
      void              setFlagActive(bool v) { mFlagActive=v; }
      bool              getFlagExit() const { return(mFlagExit); }
      void              setFlagExit(bool v) { mFlagExit=v; }
      bool              getFlagGameSetup() const { return(mFlagGameSetup); }
      void              setFlagGameSetup(bool v) { mFlagGameSetup=v; }
      bool              getFlagSystemSetup() const { return(mFlagSystemSetup); }
      void              setFlagSystemSetup(bool v) { mFlagSystemSetup=v; }
      bool              getFlagRenderSetup() const { return(mFlagRenderSetup); }
      void              setFlagRenderSetup(bool v) { mFlagRenderSetup=v; }
      bool              getFlagSoundSetup() const { return(mFlagSoundSetup); }
      void              setFlagSoundSetup(bool v) { mFlagSoundSetup=v; }
      bool              getFlagInputSystemSetup() const { return(mFlagInputSystemSetup); }
      void              setFlagInputSystemSetup(bool v) { mFlagInputSystemSetup=v; }
      bool              getFlagMultiplayerSetup() const { return(mFlagMultiplayerSetup); }
      void              setFlagMultiplayerSetup(bool v) { mFlagMultiplayerSetup=v; }
      bool              getFlagVisualSetup() const { return(mFlagVisualSetup); }
      void              setFlagVisualSetup(bool v) { mFlagVisualSetup=v; }
      bool              getFlagPhysicsSetup() const { return(mFlagPhysicsSetup); }
      void              setFlagPhysicsSetup(bool v) { mFlagPhysicsSetup=v; }

#ifndef BUILD_FINAL
      void              outputBuildInfo(void);
#endif

      int               getGameCount() const { return mGameCount; }
      void              incGameCount() { mGameCount++; }

      // yorn user context enum
      enum
      {
         cGameYornAcceptInvite,
      };

      // UI Globals yorn box
      void yornResult(uint result, DWORD userContext, int port);

      void              updateRender();

      void              acceptOtherControllerInvite();

   protected:
      void              updateTime();
      void              emptyThreadQueues(void);

      HWND              mWindowHandle;
      DWORD             mLastTime;
      DWORD             mTotalTime;
      DWORD             mFrameTime;
      DWORD             mBuildChecksum;
      BCueHandle        mMusicCue;

      BUIGlobals*       mpUIGlobals;
            
      BHandle           mRootFileCacheHandle;

      int               mRenderPathingQuad; //Debugging Pathing
      int               mRenderLRPTreeType;
      int               mObstructionRenderMode;
      int               mRenderShowPaths;
      int               mAIDebugType;

      int               mGameCount;

      //-- Flags
      bool              mFlagActive:1;
      bool              mFlagExit:1;
      bool              mFlagGameSetup:1;
      bool              mFlagSystemSetup:1;
      bool              mFlagRenderSetup:1;
      bool              mFlagSoundSetup:1;
      bool              mFlagInputSystemSetup:1;
      bool              mFlagMultiplayerSetup:1;
      bool              mFlagVisualSetup:1;
      bool              mFlagPhysicsSetup:1;
      bool              mFlagSplitScreen:1;
      bool              mFlagVerticalSplit:1;
      bool              mFlagVinceLogOpen:1;


   private:

#ifndef BUILD_FINAL   
      bool              checkSystemVersion(const char* pTitle);
#endif      

      void              handleAcceptedInvite();
      void              handleOtherControllerAcceptedInvite();


};
