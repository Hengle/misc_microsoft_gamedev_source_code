      //============================================================================
// File: binkInterface.h
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================
#pragma once

// xgamerender
#include "binkVideo.h"
#include "AtgFont.h"

// xcore
#include "threading\eventDispatcher.h"
#include "containers\staticArray.h"
// xrender
#include "renderThread.h"


struct IXAudio2;

//-------------------------------------------
class BBinkInterface : public BRenderCommandListener, BEventReceiverInterface, BBinkCaptionHandler
{
   BBinkInterface(const BBinkInterface&);
   BBinkInterface& operator= (const BBinkInterface&);
   
public:
   BBinkInterface();
   ~BBinkInterface();

   bool init(long fontDirID);
   bool deinit();
   
   class BLoadParams
   {
   public:
      BLoadParams();

      BFixedStringMaxPath     mFilename;
      long                    mCaptionDirID;
      BFixedStringMaxPath     mCaptionFilename;
      BFixedStringMaxPath     mMaskTextureFilename;
      float                   mWidthScale;            //not used in fullscreen
      float                   mHeightScale;           //not used in fullscreen
      long                    mWidth;
      long                    mHeight;
      float                   mXOffset;               //not used in fullscreen
      float                   mYOffset;               //not used in fullscreen
      BBinkVideoHandle        mHandle;
      BBinkVideoStatus*       mpStatusCallback;
      BBinkCaptionHandler*    mpCaptionHandler;
      int                     mIOBufSize;
      uint                    mLoopAtFrame;
      BCueIndex               mSoundIndex;
      uint                    mVolume;

      bool                    mFullScreen : 1;
      bool                    mLoopVideo : 1;
      bool                    mFileManagerThrottling : 1;
      bool                    mNoSound : 1;
      bool                    mDisableXMPMusic : 1;
      bool                    mPreloaded : 1;
   };

   BBinkVideoHandle loadActiveVideo(const BLoadParams& parms); 
   BByteArray*      preloadBinkFile(const BString& filename);

   //control systems
   void decompressVideo(BBinkVideoHandle handle);
   void renderVideo(BBinkVideoHandle handle);
   void advanceVideo(BBinkVideoHandle handle);

   void playVideo(BBinkVideoHandle handle);
   void stopVideo(BBinkVideoHandle handle, bool issueEndCallback);
   void pauseVideo(BBinkVideoHandle handle);
   void fastForwardVideo(BBinkVideoHandle handle, uint speed = 2);
   void rewindVideo(BBinkVideoHandle handle, uint speed = 2);
   void setVisibleVideo(BBinkVideoHandle handle, bool isVisible);

   void setVolume(uint vol); // Global volume
   uint getVolume() { return mVolume; }

   //batch operations on all loaded videos
   void decompressAllVideos();
   void renderAllVideos();
   void advanceAllVideos();

   void playAllVideos();
   void pauseAllVideos();
   void stopAllVideos(bool issueEndCallback);


   void registerValidCallback(BBinkVideoStatus* callbackPointer);
   void unregisterValidCallback(BBinkVideoStatus* callbackPointer);
   bool isRegisteredCallback(BBinkVideoStatus* callbackPointer);

   enum 
   { 
      cDEFAULT_VOLUME = UINT_MAX, // "Magic" number.  Means to use the volume set in the BinkInterface
      cNORMAL_VOLUME = 32768,     // "Normal" per Bink docs
      cMAXIMUM_VOLUME = 65536     // "Max" per Bink docs
   }; 


protected:
   enum
   {
      cVideoStatusCallbackEvent = cEventClassFirstUser,
      cVideoCaptionCallbackEvent,
   };

   BEventReceiverHandle    mEventHandleSim;
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void issueEndCallback(BBinkVideo* pVideo);
   void issueReadErrorCallback(BBinkVideo* pVideo);
   


private:

   bool destroyInternal();
   void endRemoveVideo(uint index, bool issueCallback);
   void endRemoveVideos(bool issueEndCallback);

   
   void loadActiveVideoInternal(const BBinkInterface::BLoadParams *packet);
   
   BDynamicRenderArray<BBinkVideo*>   mActiveVideos;
   
   void clearActiveVideos();
   
   ATG::Font      mCaptionFont;
   long           mFontDirID;
   bool           mPaused;

   uint           mVolume;

   IXAudio2* mpXAudio2;

   // handle generation
   long           mLastGeneratedHandle;
   BBinkVideoHandle generateHandle();
   int      giveVideoIndexFromHandle(BBinkVideoHandle handle);



   BDynamicArray<BBinkVideoStatus*>  mRegisteredCallbackHandlers;

private://BRenderCommandListenerInterface

   struct BControlPacket
   {
      BBinkVideoHandle  mHandle;
      uint              mExtraData0;
   };

   enum
   {
      cBVM_DestroyManager=0,
      cBVM_Load,

      cBVM_DecompressAll,
      cBVM_RenderAll,
      cBVM_AdvanceAll,

      //Useful if we need to intercept the blade, etc
      cBVM_Play_All,
      cBVM_Stop_All,
      cBVM_Pause_All,

      //Per video instances, expects a handle to come with it
      eBVM_Decompress,
      eBVM_Render,
      eBVM_Advance,
      eBVM_Play,
      eBVM_Stop,
      eBVM_Pause,
      eBVM_FF,
      eBVM_RW,
      eBVM_Visible,
   };

   //BRenderCommandListenerInterface
   virtual void               processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void               frameBegin(void);
   virtual void               frameEnd(void);
   virtual void               initDeviceData(void);
   virtual void               deinitDeviceData(void);

   virtual void               renderCaption(BBinkVideo* pVideo, const BUString& string, DWORD color);
   virtual void               enableCaption(BBinkVideo* pVideo, bool bEnable);
};

extern BBinkInterface gBinkInterface;
