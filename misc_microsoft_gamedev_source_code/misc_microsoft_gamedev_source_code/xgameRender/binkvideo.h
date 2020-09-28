//============================================================================
//
// File: binkVideo.h
//  
// Copyright (c) 2007-2008, Ensemble Studios
//
//============================================================================
#pragma once

#include "bink.h"
#include "binktextures.h"
#include "D3DTextureManager.h"
#include "..\xsound\xsound.h"

typedef uint BBinkVideoHandle;
const BBinkVideoHandle cInvalidVideoHandle = UINT_MAX;

class BBinkVideo;

enum eBinkStatusCode
{
   cBinkStatus_CompleteOK  =0,
   cBinkStatus_ReadError   =1     
};
//-------------------------------------------

class BBinkCaptionHandler
{
public:
   virtual void enableCaption(BBinkVideo* pVideo, bool bEnable) = 0;
   virtual void renderCaption(BBinkVideo* pVideo, const BUString& string, DWORD color) = 0;
};

//-------------------------------------------

class BBinkVideoStatus
{
public:
   virtual void onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode) = 0;
   virtual void onVideoCaptions(BBinkVideoHandle handle, bool enableCaptions, const BUString& caption) {};
};

//-------------------------------------------
class BBinkVideo
{
public:

   BBinkVideo(const BBinkVideo&);
   BBinkVideo& operator= (const BBinkVideo&);
   

   BBinkVideo();
   ~BBinkVideo();
         
   bool load(const char* pFilename, int ioBufSize = -1, bool fileManagerThrottling = false, bool preloaded = false);
   bool loadCaptionFile(long dirID, const char* pFilename);
   bool loadMaskTexture(const char* pFilename);
   void destroy();

   void setVolume(uint vol);

   bool decompressFrame();
   void renderFrame();
   void renderCaptions();
   void advanceFrame();
   
   
  
   //size
   void setLocSize(float xLoc, float yLoc, uint widthInPixels, uint heightInPixels);
   void setLocScale(float xLoc, float yLoc, float xScale, float yScale);
   void setFullscreen();
   void setSoundIndex(BCueIndex index);
   BCueIndex getSoundIndex() const { return mSoundIndex; };


   //standard controls
   void play();       
   void pause();
   void stop()       { setPlayState(ePlayState_Stop);}
   void fastForward(uint playSpeed = 2){ setPlayState(ePlayState_FF); mPlaySpeed = playSpeed;}
   void rewind(uint playSpeed = 2)     { setPlayState(ePlayState_RW); mPlaySpeed = playSpeed;}

   bool isPaused() { return mPlayState == ePlayState_Pause || mPlaySpeed ==0;}

   void skipForwardFrames(uint numFrames);
   void skipForwardSeconds(uint numSeconds);
   void skipBackwardFrames(uint numFrames);
   void skipBackwardSeconds(uint numSeconds);

   void rewindToStart();
   void setLoopVideo(bool loopVideo) { mLoopVideo = loopVideo; }
   bool getLoopVideo() const { return mLoopVideo; }

   void setLoopAtFrame(uint loopFrame) { mLoopAtFrame = loopFrame; }
   uint getLoopAtFrame() const { return mLoopAtFrame; }

   void setNoSound(bool noSound) { mNoSound = noSound; }
   bool getNoSound() const { return mNoSound; }

   void setVisible(bool isVisible) { mIsVisible = isVisible;};

   void setDisableXMPMusic(bool disableXMPMusic) { mDisableXMPMusic = disableXMPMusic; }

   //query
   bool isEndOfMovie();
   int getFrameNum() const { if (mBinkVideoHandle) return mBinkVideoHandle->FrameNum; return -1; }
   int getNumberOfFrames() const { if (mBinkVideoHandle) return mBinkVideoHandle->Frames; return 0; }
   int getVideoWidth() const { if (mBinkVideoHandle) return mBinkVideoHandle->Width; return 0; }
   int getVideoHeight() const { if (mBinkVideoHandle) return mBinkVideoHandle->Height; return 0; };
         
   void setVideoStatusCallback(BBinkVideoStatus* pStatusCallback) { mpStatusCallback = pStatusCallback; }
   BBinkVideoStatus* getVideoStatusCallback() { return mpStatusCallback; }
   
   void setCaptionHandler(BBinkCaptionHandler* pCaptionHandler) { mpCaptionHandler = pCaptionHandler; }
   BBinkCaptionHandler* getCaptionHandler() { return mpCaptionHandler; }

   const HBINK* getBinkHandle() const {return &mBinkVideoHandle;};

   const BBinkVideoHandle getHandle(){return mVideoHandle;}
   void setHandle(BBinkVideoHandle hdl){mVideoHandle = hdl;}

   void markForDeferredDelete(){mDeferredDelete = true;}
   bool isMarkedForDeferredDelete(){return mDeferredDelete;}

   bool didErrorOccur() { return mReadErrorOccured;};
   
   BByteArray* getPreloadedDataAndRelease() {BByteArray *temp = mPreloadedData; mPreloadedData=NULL; return(temp);}
   
private:
   enum ePlayState
   {
      ePlayState_Normal = 1,
      ePlayState_Pause = 2,
      ePlayState_Stop = 3,
      ePlayState_FF = 4,
      ePlayState_RW = 5,

      eForceDWORD = 0xFFFFFFFF
   };
   void setPlayState(ePlayState playState)   { mPlayState = playState;   }
   ePlayState getPlayState() { return mPlayState; } 


   

   void onReadError();
   U32 binkPercentageFull();
   void tickIO();
   
   BString                       mFilename;
   
   float                         mHeightScale;
   float                         mWidthScale;
   float                         mXOffset;
   float                         mYOffset;

   ePlayState                    mPlayState;
   int                           mPlaySpeed;

   BBinkVideoHandle              mVideoHandle;     //This is passed to us by the BBinkInterface
                                                   //if you're not using that, then don't worry about this.
      
   BBinkVideoStatus*             mpStatusCallback;
   BBinkCaptionHandler*          mpCaptionHandler;

   HBINK                         mBinkVideoHandle;
   BINKTEXTURESET                mTextureSet;
   BD3DTextureManager::BManagedTexture* mpMaskTexture;
      
   BCueIndex                     mSoundIndex;
   uint                          mVolume;
      
   struct BCaptionLine
   {
      BCaptionLine() : mStartFrame(0), mEndFrame(0), mColor(0xFFFFFFFF), mAlreadyPlayed(false) {};
      uint32 mStartFrame;
      uint32 mEndFrame;
      DWORD mColor;
      BUString mPrintString;
      bool mAlreadyPlayed : 1;
   };
   BDynamicRenderArray<BCaptionLine>   mCaptionData;
   
   BByteArray*                   mPreloadedData;
   
   uint                          mLoopAtFrame;
   
   bool                          mLoopVideo : 1;
   bool                          mPlayedLastFrame : 1;
   bool                          mFileManagerThrottling : 1;
   bool                          mFileManagerIsPaused : 1;
   bool                          mDeferredDelete : 1;
   bool                          mStartedSound : 1;
   bool                          mNoSound : 1;
   bool                          mIsVisible : 1;
   bool                          mCaptionsVisible : 1;
   bool                          mDisableXMPMusic : 1;
   bool                          mReadErrorOccured : 1;
   bool                          mHaveDecompressedFrame : 1;

};