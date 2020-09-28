//============================================================================
// flashbackgroundplayer.h
// Ensemble Studios (C) 2007
//============================================================================

#pragma once

#include "gfxMoviePlayer.h"
#include "xmlreader.h"
#include "binkvideo.h"
#include "AtgFont.h"

#include "inputsystem.h"


//============================================================================
//============================================================================
class BFlashBackgroundPlayer : public BEventReceiver, BBinkCaptionHandler, IInputEventHandler
{
   BFlashBackgroundPlayer(const BFlashBackgroundPlayer&);
   BFlashBackgroundPlayer& operator= (const BFlashBackgroundPlayer&);
   
public:
   BFlashBackgroundPlayer();
  ~BFlashBackgroundPlayer();

   enum
   {
      eFBPEventLoad = cEventClassFirstUser,
      eFBPEventUnload,
      eFBPEventActionScript,
      eFBPEventPlay,
      

      eFBPEventTotal
   };

   class BLoadMoviePayload : public BEventPayload
   {
      public:
         BLoadMoviePayload(const BFixedString128& language, uint directory, const BFixedString128& fontDefinitionFile, const BFixedString128& wordWrapMode) : 
            mLanguage(language), 
            mFontDefinitionFile(fontDefinitionFile),
            mWordWrapMode(wordWrapMode),
            mDirectory(directory){ }
      
      BFixedString128 mLanguage;  
      BFixedString128 mFontDefinitionFile;  
      BFixedString128 mWordWrapMode;
      uint mDirectory;

      private:
         virtual void deleteThis(bool delivered) { delivered; delete this; }
   };

   //-- MAIN Thread 
   bool init(long fontDirID);
   void deinit();   

   virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   
   // Pass in NULL to choose a random movie.
   void loadMovie(const char* pMovie, const char * pFontDefinitionFile, const char* pBinkVideo, const char* pCaptionsFile, long dirID, bool noCancelMovieUI, bool useMiniLoad);
   void invokeActionScript(const char* method, const GFxValue* pArgs, int argCount);
   void startMovie();
   void stopMovie(bool waitForBinkMovieToFinish = false);
   
   bool getIsLoaded() const { return mIsLoaded; }
   bool getIsStarted() const { return mIsStarted; }

   //-- Helper Thread     
   BWin32Event& getExitEvent() { return mExitEvent; }
   BWin32Event& getTerminationCompleteEvent() { return mTerminationCompleteEvent; }

   void renderCaption(BBinkVideo * pVideo, const BUString& string, DWORD color);
   void enableCaption(BBinkVideo * pVideo, bool bEnable);

   void setPlayPauseSkipStrings(const BUString& playString, const BUString& pauseString, const BUString& skipString);

   bool getCanPause() { return mCanPause; }
   void setCanPause(bool canPause) { mCanPause = canPause; }

   bool getCanQuickCancel();
   void setCanQuickCancel(bool canQuickCancel, DWORD minVideoTime);

   BSmallDynamicArray<const BUString*>& getTipPointerContainer() { return mRandomTips; }

   bool getIsInMovie() const { return mInsidePlayMovieCallback; }
   void setExternalReadError();
private:

   bool renderMovie();
   bool renderUI();
   bool update();
   void setMoviePlayerUIButtons();
   void displayNextTip();

   /// Call an action script method on the movie
   ///   Can only be called after the movie has been loaded and not after the movie is rendering
   class BFBPActionScriptCommandPayload:public BEventPayload
   {
   public:
      BFBPActionScriptCommandPayload(const char* path) : method(path){ }
      BFixedString128   method;
      GFxValue          args[8];
      int               argCount;

   private:
      virtual void deleteThis(bool delivered) { delivered; delete this; }
   };
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   
   void playMovieCallback(void);
   static void playMovieCallbackFunc(void* pPrivateData);

   static BCriticalSection mCanQuickCancelLock;

   BSmallDynamicArray<const BUString*> mRandomTips;
   BUString                mStringPlay;
   BUString                mStringPause;
   BUString                mStringSkip;

   BEventReceiverHandle    mRenderEventHandle;
   BBinkVideo*             mpBinkVideo;
   BGFXMoviePlayer*        mpMoviePlayer;
   BGFXMoviePlayer*        mpMoviePlayerCancel;
   BGFxMovieManager*       mpMovieManager;
   long                    mFontDirID;
   BWin32Event             mExitEvent;
   BWin32Event             mTerminationCompleteEvent;
   BWin32Event             mPlayMovieCallbackActive;
   BWin32Event             mBinkVideoIsFinished;
   BString                 mMovieToLoad;
   BString                 mCancelMovieToLoad;
   BString                 mBinkVideoToPlay;
   BString                 mBinkVideoCaptionsFile;
   long                    mCaptionsDirID;
   long                    mNextTipIndex;
   volatile long           mExternalReadError;
   DWORD                   mLastInputTime;
   DWORD                   mLastTipTime;
   DWORD                   mMinVideoTime;
   DWORD                   mStartVideoTime;
   uint                    mBinkVideoVolume;
         
   bool                    mIsLoaded : 1;
   bool                    mIsStarted : 1;
   bool                    mInsidePlayMovieCallback : 1;
   bool                    mIsWaitingForBinkMovieToFinish : 1;
   bool                    mStopMovie : 1;
   bool                    mMovieUIVisible : 1;
   bool                    mCaptionsEnabled : 1;
   bool                    mCanPause:1;
   bool                    mCanQuickCancel:1;
   bool                    mUseMiniLoad:1;
};

extern BFlashBackgroundPlayer gFlashBackgroundPlayer;