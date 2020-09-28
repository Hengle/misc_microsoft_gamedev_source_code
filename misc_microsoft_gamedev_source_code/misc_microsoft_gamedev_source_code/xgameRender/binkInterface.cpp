//============================================================================
// File: binkInterface.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================
// xgamerender
#include "xgameRender.h"
#include "FontSystem2.h"
#include "render.h"

// xcore
#include "xmlreader.h"
#include "file\win32filestream.h"

#include "bfileStream.h"

// xrender
#include "xrender.h"
#include "renderThread.h"

// local
#include "binkInterface.h"
#include <xaudio.h>
#include <xhv.h>
#include <xaudio2.h>

// sound

#define TALKING_HEAD_VIDEOS_IN_ARCHIVE

BBinkInterface gBinkInterface;

class BBinkCaptionsPayload : public BEventPayload
{
public:
   BBinkCaptionsPayload(BBinkVideoStatus* pCallback, BBinkVideoHandle handle, bool enableCaptions, const BUString& captionString) : 
      mpCallback(pCallback), mVideoHandle(handle), mEnableCaptions(enableCaptions), mCaptionString(captionString) { }

   BBinkVideoStatus* mpCallback;
   BBinkVideoHandle  mVideoHandle;
   bool              mEnableCaptions;
   BUString          mCaptionString;

private:
   virtual void deleteThis(bool delivered) 
   {
      delivered; 
      //CLM sanity check..
      mpCallback = NULL;
      mVideoHandle = cInvalidVideoHandle;
      mEnableCaptions = false;
      mCaptionString.empty();


      delete this; 
   }
};

class BBinkCallbackPayload : public BEventPayload
{
public:
  

   BBinkCallbackPayload(BBinkVideoStatus* pCallback, BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode) : mpCallback(pCallback), mVideoHandle(handle), mPreloadedData(preloadedData), mStatusCode(statusCode) { }
 

   BBinkVideoStatus* mpCallback;
   BBinkVideoHandle  mVideoHandle;
   BByteArray*       mPreloadedData;
   eBinkStatusCode   mStatusCode;

   
private:
   virtual void deleteThis(bool delivered) { delivered; delete this; }
};

//============================================================================
// BLoadParams::BLoadParams
//============================================================================
BBinkInterface::BLoadParams::BLoadParams():
mFilename(""),
mCaptionDirID(-1),
mCaptionFilename(""),
mMaskTextureFilename(""),
mWidthScale(1.0f),
mHeightScale(1.0f),
mWidth(0),
mHeight(0),
mXOffset(0.0f),
mYOffset(0.0f),
mHandle(cInvalidVideoHandle),
mpStatusCallback(NULL),
mpCaptionHandler(NULL),
mIOBufSize(-1),
mFullScreen(false),
mLoopVideo(false),
mFileManagerThrottling(false),
mSoundIndex(cInvalidCueIndex),
mVolume((uint)BBinkInterface::cDEFAULT_VOLUME),
mLoopAtFrame(0),
mNoSound(false),
mDisableXMPMusic(false),
mPreloaded(false)
{
}

//============================================================================
// BBinkInterface::BBinkInterface
//============================================================================
BBinkInterface::BBinkInterface() : 
   mEventHandleSim(cInvalidEventReceiverHandle),
   mFontDirID(-1),
   mPaused(false),
   mVolume(cMAXIMUM_VOLUME),
   mLastGeneratedHandle(1),
   mpXAudio2(NULL)
{
}

//============================================================================
// BBinkInterface::~BBinkInterface
//============================================================================
BBinkInterface::~BBinkInterface()
{
}

//============================================================================
// BBinkInterface::~init
//============================================================================
bool BBinkInterface::init(long fontDirID)
{
   ASSERT_MAIN_THREAD
   
   if (mEventHandleSim == cInvalidEventReceiverHandle)
   {
      mFontDirID = fontDirID;

      // register on the sim thread
      mEventHandleSim = gEventDispatcher.addClient(this, cThreadIndexSim);

      commandListenerInit();
   }      

   return true;
}
//============================================================================
// BBinkInterface::deinit
//============================================================================
bool BBinkInterface::deinit()
{
   if (mEventHandleSim != cInvalidEventReceiverHandle)
   {
      gRenderThread.submitCommand(mCommandHandle, cBVM_DestroyManager);
      
      // Block for safety. 
      gRenderThread.blockUntilGPUIdle();

      commandListenerDeinit();

      gEventDispatcher.removeClientImmediate(mEventHandleSim);
      mEventHandleSim = cInvalidEventReceiverHandle;
   }      

   mRegisteredCallbackHandlers.clear();

   return true;
}

//============================================================================
// BBinkInterface::clearActiveVideos
//============================================================================
void BBinkInterface::clearActiveVideos()
{
   ASSERT_RENDER_THREAD
   
   for (uint i=0;i<mActiveVideos.getSize();i++)
   {
      HEAP_DELETE(mActiveVideos[i], gRenderHeap);
      mActiveVideos[i] = NULL;
   }      
   
   mActiveVideos.clear();
}

//============================================================================
// BBinkInterface::destroyInternal
//============================================================================
bool  BBinkInterface::destroyInternal()
{
   ASSERT_RENDER_THREAD

   clearActiveVideos();   

   return true;
}

//============================================================================
// BBinkInterface::loadActiveVideoInternal
//============================================================================
void BBinkInterface::loadActiveVideoInternal(const BBinkInterface::BLoadParams* packet)
{
   ASSERT_RENDER_THREAD

   BBinkVideo* p = new(gRenderHeap) BBinkVideo;
   
   p->setVideoStatusCallback(packet->mpStatusCallback);

   if(packet->mpCaptionHandler == NULL)
      p->setCaptionHandler(this);
   else
      p->setCaptionHandler(packet->mpCaptionHandler);

   p->setHandle(packet->mHandle);

   p->setNoSound(packet->mNoSound);

   p->setDisableXMPMusic(packet->mDisableXMPMusic);

   p->setSoundIndex(packet->mSoundIndex);
   
   p->setVolume(packet->mNoSound ? 0 : packet->mVolume);

   if(!p->load(packet->mFilename, packet->mIOBufSize, packet->mFileManagerThrottling, packet->mPreloaded))
   {
      // this failed to load, notify the call via the callback handler.
      issueEndCallback(p);

      HEAP_DELETE(p, gRenderHeap);
      return;
   }
   
   p->setLoopVideo(packet->mLoopVideo);
   p->setLoopAtFrame(packet->mLoopAtFrame);     

   if(packet->mFullScreen)
      p->setFullscreen();
   else
   {
      if (packet->mWidth > 0)
         p->setLocSize(packet->mXOffset,packet->mYOffset,packet->mWidth,packet->mHeight);
      else
         p->setLocScale(packet->mXOffset,packet->mYOffset,packet->mWidthScale,packet->mHeightScale);
   }

   if(packet->mCaptionFilename!="")
      p->loadCaptionFile(packet->mCaptionDirID, packet->mCaptionFilename);

   if (packet->mMaskTextureFilename!="")
      p->loadMaskTexture(packet->mMaskTextureFilename);
   
   mActiveVideos.pushBack(p);
}

//============================================================================
// BBinkInterface::renderCaption
//============================================================================
void BBinkInterface::renderCaption(BBinkVideo* pVideo, const BUString& string, DWORD color)
{
   ASSERT_RENDER_THREAD

   BBinkVideoStatus* pVideoStatus = pVideo->getVideoStatusCallback();
   if (!pVideoStatus)
      return;

   // bundle this up and call the thread switcher
   BBinkCaptionsPayload* payload = new BBinkCaptionsPayload(pVideoStatus, pVideo->getHandle(), true, string);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cVideoCaptionCallbackEvent, 0, 0, payload);

 
}

//============================================================================
// BBinkInterface::enableCaption
//============================================================================
void BBinkInterface::enableCaption(BBinkVideo* pVideo, bool bEnable)
{
   ASSERT_RENDER_THREAD

   if (bEnable)
      return;

   // only call this when we have to turn off the caption.
   BBinkVideoStatus* pVideoStatus = pVideo->getVideoStatusCallback();
   if (!pVideoStatus)
      return;

   BUString emptyString;
   emptyString.set(L"");
   // bundle this up and call the thread switcher
   BBinkCaptionsPayload* payload = new BBinkCaptionsPayload(pVideoStatus, pVideo->getHandle(), false, emptyString);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cVideoCaptionCallbackEvent, 0, 0, payload);
}

//============================================================================
// BBinkInterface::issueEndCallback
//============================================================================
void BBinkInterface::issueEndCallback(BBinkVideo* pVideo)
{
   BBinkVideoStatus* pVideoStatus = pVideo->getVideoStatusCallback();
   if (!pVideoStatus)
      return;
   
   // Retrieve the preloaded data pointer and take ownership of it by NULLing it out in the video itself.
   BByteArray *preloadedData = pVideo->getPreloadedDataAndRelease();

   // bundle this up and call the thread switcher
   BBinkCallbackPayload* payload = new BBinkCallbackPayload(pVideoStatus, pVideo->getHandle(), preloadedData, cBinkStatus_CompleteOK);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cVideoStatusCallbackEvent, 0, 0, payload);
}

//============================================================================
// BBinkInterface::issueReadErrorCallback
//============================================================================
void BBinkInterface::issueReadErrorCallback(BBinkVideo* pVideo)
{
   BBinkVideoStatus* pVideoStatus = pVideo->getVideoStatusCallback();
   if (!pVideoStatus)
      return;

   // Retrieve the preloaded data pointer and take ownership of it by NULLing it out in the video itself.
   BByteArray *preloadedData = pVideo->getPreloadedDataAndRelease();

   // bundle this up and call the thread switcher
   BBinkCallbackPayload* payload = new BBinkCallbackPayload(pVideoStatus, pVideo->getHandle(), preloadedData, cBinkStatus_ReadError);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cVideoStatusCallbackEvent, 0, 0, payload);
}


//============================================================================
// BBinkInterface::loadActiveVideo
//============================================================================
BBinkVideoHandle BBinkInterface::generateHandle()
{
   mLastGeneratedHandle++;
   return mLastGeneratedHandle;
}

//============================================================================
// BBinkInterface::loadActiveVideo
//============================================================================
int BBinkInterface::giveVideoIndexFromHandle(BBinkVideoHandle handle)
{
   ASSERT_RENDER_THREAD
   
   for(uint i = 0; i < mActiveVideos.getSize(); i++)
   {
      if(mActiveVideos[i]->getHandle()==handle)
         return i;
   }
   return cInvalidVideoHandle;
}

//============================================================================
//============================================================================
BByteArray* BBinkInterface::preloadBinkFile(const BString& filename)
{
#ifndef TALKING_HEAD_VIDEOS_IN_ARCHIVE
   uint streamFlags = (eStreamFlags)(cSFReadable | cSFOpenExisting | cSFDiscardOnClose);

   BWin32FileStream *pStream = new BWin32FileStream;

   if (!pStream)
      return(NULL);
   
   if(pStream->open(filename, (eStreamFlags)streamFlags, &gWin32LowLevelFileIO))
#else   

   // Remove "game:\" from filename
   BString archiveFilename;
   archiveFilename.set(filename);
   archiveFilename.substring(6, archiveFilename.length());

   BFile fileStream;
   if(!fileStream.open(cDirProduction, archiveFilename, BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      return(NULL);
   }

   BStream* pStream = fileStream.getStream();

   if(pStream)
#endif
   {
      // Allocate the byte array we'll hand back.
      BByteArray *fileData = new BByteArray;

      if (pStream->sizeKnown())      
      {
         uint64 fileSize = pStream->size();
         if (fileSize >= 256U*1024U*1024U)
         {
            delete fileData;
            return(NULL);
         }
            
         fileData->resize((uint)fileSize);
         
         if (pStream->readBytes(fileData->getPtr(), fileData->getSizeInBytes()) != fileData->getSizeInBytes())
         {
            delete fileData;
            return(NULL);
         }
      }
      else
      {  
         const uint cBufSize = 4096;
         BYTE buf[cBufSize];
         for ( ; ; )
         {
            const uint bytesRead = pStream->readBytes(buf, cBufSize);
            if (bytesRead < cBufSize)
            {
               if (pStream->errorStatus())
               {
                  delete fileData;
                  return(NULL);
               }
               
               if (!bytesRead)
                  break;
            }
            
            fileData->pushBack(buf, bytesRead);
         }
      }

#ifndef TALKING_HEAD_VIDEOS_IN_ARCHIVE
      delete pStream;
#else
      fileStream.close();
#endif

      // We have to endian swap the file data because bink treats in-memory files
      // as already swapped whereas on disk files are assumed not to be swapped
      EndianSwitchDWords((DWORD*) fileData->getPtr(), fileData->getSize() / sizeof(DWORD));

      return fileData;
   }

#ifdef TALKING_HEAD_VIDEOS_IN_ARCHIVE
   fileStream.close();
#endif
   
   return NULL;
}

//============================================================================
// BBinkInterface::loadActiveVideo
//============================================================================
BBinkVideoHandle BBinkInterface::loadActiveVideo(const BLoadParams& parms)
{
   ASSERT_MAIN_THREAD

   BBinkVideoHandle handle = generateHandle();

   BLoadParams *lp = reinterpret_cast<BLoadParams *>(gRenderThread.submitCommandBegin(mCommandHandle, cBVM_Load, sizeof(BLoadParams)));

   lp->mFilename.set(parms.mFilename);
   lp->mCaptionDirID = parms.mCaptionDirID; 

   lp->mFullScreen = parms.mFullScreen;
   if(parms.mFullScreen)
   {
      lp->mWidthScale = 1;
      lp->mHeightScale = 1;
      lp->mXOffset = 0;
      lp->mYOffset = 0;
   }
   else
   {
      lp->mWidthScale = parms.mWidthScale;
      lp->mHeightScale = parms.mHeightScale;
      lp->mWidth = parms.mWidth;
      lp->mHeight = parms.mHeight;
      lp->mXOffset = parms.mXOffset;
      lp->mYOffset = parms.mYOffset;
   }
   lp->mCaptionFilename.set(parms.mCaptionFilename);
   lp->mMaskTextureFilename.set(parms.mMaskTextureFilename);
   lp->mpStatusCallback = parms.mpStatusCallback;
   lp->mpCaptionHandler = parms.mpCaptionHandler;
   lp->mIOBufSize = parms.mIOBufSize;
   lp->mLoopVideo = parms.mLoopVideo;
   lp->mLoopAtFrame = parms.mLoopAtFrame;
   lp->mFileManagerThrottling = parms.mFileManagerThrottling;
   lp->mNoSound = parms.mNoSound;
   lp->mHandle = handle;
   lp->mSoundIndex = parms.mSoundIndex;
   lp->mVolume = parms.mVolume == cDEFAULT_VOLUME ? mVolume : parms.mVolume;
   lp->mDisableXMPMusic = parms.mDisableXMPMusic;
   lp->mPreloaded = parms.mPreloaded;

   gRenderThread.submitCommandEnd(sizeof(BLoadParams));
   
   if( mPaused )
   {
      decompressVideo( handle );
      pauseVideo( handle );
   }

   return handle;
}

//============================================================================
// BBinkInterface::setVolume
//============================================================================
void BBinkInterface::setVolume(uint vol)
{
   ASSERT_MAIN_THREAD

   mVolume = vol;
}

//============================================================================
// BBinkInterface::decompressActiveVideos
//============================================================================
void BBinkInterface::decompressAllVideos()
{
   ASSERT_MAIN_THREAD
   
   gRenderThread.submitCommand(mCommandHandle,cBVM_DecompressAll);
}
//============================================================================
// BBinkInterface::renderActiveVideos
//============================================================================
void BBinkInterface::renderAllVideos()
{
   ASSERT_MAIN_THREAD
   
   gRenderThread.submitCommand(mCommandHandle,cBVM_RenderAll);
}
//============================================================================
// BBinkInterface::advanceActiveVideos
//============================================================================
void BBinkInterface::advanceAllVideos()
{
   ASSERT_MAIN_THREAD
   gRenderThread.submitCommand(mCommandHandle,cBVM_AdvanceAll);
}
//============================================================================
// BBinkInterface::advanceActiveVideos
//============================================================================
void BBinkInterface::stopAllVideos(bool issueEndCallback)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cBVM_Stop_All, sizeof(BControlPacket)));
   packet->mHandle = 0;
   packet->mExtraData0 = issueEndCallback;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}
//============================================================================
// BBinkInterface::advanceActiveVideos
//============================================================================
void BBinkInterface::pauseAllVideos()
{
   ASSERT_MAIN_THREAD
   
   gRenderThread.submitCommand(mCommandHandle,cBVM_Pause_All);
   mPaused = true;
}
//============================================================================
// BBinkInterface::playAllVideos
//============================================================================
void BBinkInterface::playAllVideos()
{
   ASSERT_MAIN_THREAD
   
   gRenderThread.submitCommand(mCommandHandle,cBVM_Play_All);
   mPaused = false;
}
//============================================================================
// BBinkInterface::playAllVideos
//============================================================================
void BBinkInterface::decompressVideo(BBinkVideoHandle handle)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_Decompress, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = 0;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}

//============================================================================
// BBinkInterface::renderVideo
//============================================================================
void BBinkInterface::renderVideo(BBinkVideoHandle handle)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_Render, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = 0;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}

//============================================================================
// BBinkInterface::advanceVideo
//============================================================================
void BBinkInterface::advanceVideo(BBinkVideoHandle handle)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_Advance, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = 0;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}

//============================================================================
// BBinkInterface::playVideo
//============================================================================
void BBinkInterface::playVideo(BBinkVideoHandle handle)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_Play, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = 0;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}

//============================================================================
// BBinkInterface::stopVideo
//============================================================================
void BBinkInterface::stopVideo(BBinkVideoHandle handle, bool issueEndCallback)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_Stop, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = issueEndCallback;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}

//============================================================================
// BBinkInterface::pauseVideo
//============================================================================
void BBinkInterface::pauseVideo(BBinkVideoHandle handle)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_Pause, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = 0;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}

//============================================================================
// BBinkInterface::fastForwardVideo
//============================================================================
void BBinkInterface::fastForwardVideo(BBinkVideoHandle handle, uint speed/* = 2*/)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_FF, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = speed;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}

//============================================================================
// BBinkInterface::rewindVideo
//============================================================================
void BBinkInterface::rewindVideo(BBinkVideoHandle handle, uint speed/* = 2*/)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_RW, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = speed;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}
//============================================================================
// BBinkInterface::setVisibleVideo
//============================================================================
void BBinkInterface::setVisibleVideo(BBinkVideoHandle handle, bool isVisible)
{
   ASSERT_MAIN_THREAD

   BControlPacket *packet = reinterpret_cast<BControlPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, eBVM_Visible, sizeof(BControlPacket)));
   packet->mHandle = handle;
   packet->mExtraData0 = (int)isVisible;
   gRenderThread.submitCommandEnd(sizeof(BControlPacket));
}
//============================================================================
// BBinkInterface::endRemoveVideo
//============================================================================
void BBinkInterface::endRemoveVideo(uint index, bool issueCallback)
{
   ASSERT_RENDER_THREAD
   
   if (index >= mActiveVideos.getSize())
      return;

   if(issueCallback)
   {
      if(mActiveVideos[index]->didErrorOccur())
         issueReadErrorCallback(mActiveVideos[index]);
      else
         issueEndCallback(mActiveVideos[index]);
   }
   
   HEAP_DELETE(mActiveVideos[index], gRenderHeap);
      
   mActiveVideos.removeIndex(index);
}

//============================================================================
// BBinkInterface::endRemoveVideo
//============================================================================
void BBinkInterface::endRemoveVideos(bool issueEndCallback)
{
   ASSERT_RENDER_THREAD
   
   for (uint i=0;i<mActiveVideos.getSize();i++)
   {
      if(mActiveVideos[i]->isMarkedForDeferredDelete())
      {
         endRemoveVideo(i,issueEndCallback);
         
         i--;
      }
   }
}

//============================================================================
// BBinkInterface::receiveEvent
//============================================================================
bool BBinkInterface::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (threadIndex)
   {
      case cThreadIndexSim:
      {
         switch (event.mEventClass)
         {
            case cVideoStatusCallbackEvent:
               {
                  // call the callback on the sim thread
                  const BBinkCallbackPayload* pPayload = static_cast<const BBinkCallbackPayload*>(event.mpPayload);
                  bool calledBack = false;
                  if(pPayload != NULL && pPayload->mpCallback!=NULL)
                  {
                     if(isRegisteredCallback(pPayload->mpCallback))
                     {
                        pPayload->mpCallback->onVideoEnded(pPayload->mVideoHandle, pPayload->mPreloadedData, pPayload->mStatusCode);
                        calledBack = true;
                     }
                  } 
                  
                  // If no callback was made, no is left in ownership of the preloaded data and we need to handle deleting it.
                  if(pPayload && !calledBack)
                     delete pPayload->mPreloadedData;
               }
               break;

            case cVideoCaptionCallbackEvent:
               {
                  // call the callback on the sim thread
                  const BBinkCaptionsPayload* pPayload = static_cast<const BBinkCaptionsPayload*>(event.mpPayload);
                  if(pPayload != NULL && pPayload->mpCallback!=NULL)
                  {
                     if(isRegisteredCallback(pPayload->mpCallback))
                     {
                        pPayload->mpCallback->onVideoCaptions(pPayload->mVideoHandle, pPayload->mEnableCaptions, pPayload->mCaptionString);
                        
                     }
                  }
               }
               break;
         }
      }
      break;
   }
   return false;
}

//============================================================================
// BBinkInterface::processCommand
//============================================================================
void BBinkInterface::registerValidCallback(BBinkVideoStatus* callbackPointer)
{
   ASSERT_THREAD(cThreadIndexSim);

   if(callbackPointer == NULL)
      return;

   if(isRegisteredCallback(callbackPointer))
      return;


   mRegisteredCallbackHandlers.add(callbackPointer);
   
}

//============================================================================
// BBinkInterface::processCommand
//============================================================================
void BBinkInterface::unregisterValidCallback(BBinkVideoStatus* callbackPointer)
{
   ASSERT_THREAD(cThreadIndexSim);

   if(callbackPointer == NULL)
      return;

   
   for(uint i =0; i < mRegisteredCallbackHandlers.getSize();i++)
   {
      if(mRegisteredCallbackHandlers[i] == callbackPointer)
      {
         mRegisteredCallbackHandlers.removeIndex(i);
         return;
      }
   }
}

//============================================================================
// BBinkInterface::processCommand
//============================================================================
bool BBinkInterface::isRegisteredCallback(BBinkVideoStatus* callbackPointer)
{
   ASSERT_THREAD(cThreadIndexSim);

   if(callbackPointer == NULL)
      return false;


   for(uint i =0; i < mRegisteredCallbackHandlers.getSize();i++)
   {
      if(mRegisteredCallbackHandlers[i] == callbackPointer)
         return true;
   }

   return false;
}



//============================================================================
// BBinkInterface::processCommand
//============================================================================
void BBinkInterface::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      case cBVM_Load:
         {
            loadActiveVideoInternal(reinterpret_cast<const BLoadParams*>(pData));
            break; 
         }
      case cBVM_DestroyManager:
         {
            destroyInternal();
            break;
         }
      case cBVM_DecompressAll:
         {
            for (uint i=0;i<mActiveVideos.getSize();i++)
            {
               if(!mActiveVideos[i]->decompressFrame())
               {
                  mActiveVideos[i]->markForDeferredDelete();
               } 
            }
            endRemoveVideos(true);
            break;
         }
      case cBVM_RenderAll:
         {
            for (uint i=0;i<mActiveVideos.getSize();i++)
            {
               mActiveVideos[i]->renderFrame();
            }
            break;
         }
      case cBVM_AdvanceAll:
         {
            for (uint i = 0; i < mActiveVideos.getSize(); i++)
            {
               mActiveVideos[i]->advanceFrame();

               if ((!mActiveVideos[i]->getLoopVideo()) && (mActiveVideos[i]->isEndOfMovie()))
               {
                  mActiveVideos[i]->markForDeferredDelete();
               }
            }
            endRemoveVideos(true);
            break;
         }
      case cBVM_Stop_All:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);
            for (uint i=0;i<mActiveVideos.getSize();i++)
            {
               mActiveVideos[i]->stop();
               mActiveVideos[i]->markForDeferredDelete();
            }
            endRemoveVideos(cp->mExtraData0==1);
            break;
         }
      case cBVM_Pause_All:
         {
            for (uint i=0;i<mActiveVideos.getSize();i++)
               mActiveVideos[i]->pause();
            
            break;
         }
      case cBVM_Play_All:
         {
            for (uint i=0;i<mActiveVideos.getSize();i++)
               mActiveVideos[i]->play();
            
            break;
         }
      case eBVM_Decompress:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
            {
               if(!mActiveVideos[idx]->decompressFrame())
                  endRemoveVideo(idx, true);
               
            }
               
            break;
         }
      case eBVM_Render:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
               mActiveVideos[idx]->renderFrame();


            break;
         }
      case eBVM_Advance:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
            {
               mActiveVideos[idx]->advanceFrame();

               if ((!mActiveVideos[idx]->getLoopVideo()) && (mActiveVideos[idx]->isEndOfMovie()))
               {
                  endRemoveVideo(idx,true);
               }
            }

            break;
         }
      case eBVM_Play:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
               mActiveVideos[idx]->play();

            break;
         }
      case eBVM_Pause:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
               mActiveVideos[idx]->pause();

            break;
         }
      case eBVM_Stop:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
            {
               mActiveVideos[idx]->stop();
               endRemoveVideo(idx,cp->mExtraData0==1);
            }

            break;
         }
      case eBVM_FF:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
               mActiveVideos[idx]->fastForward(cp->mExtraData0);

            break;
         }
      case eBVM_RW:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
               mActiveVideos[idx]->rewind(cp->mExtraData0);

            break;
         }
      case eBVM_Visible:
         {
            const BControlPacket* cp = reinterpret_cast<const BControlPacket*>(pData);

            int idx = giveVideoIndexFromHandle(cp->mHandle);
            if(idx != cInvalidVideoHandle)
               mActiveVideos[idx]->setVisible(cp->mExtraData0==0? false : true);

            break;
         }

         
   }
};  
//============================================================================
// BBinkInterface::frameBegin
//============================================================================
void BBinkInterface::frameBegin(void)
{
   ASSERT_RENDER_THREAD
     
}; 
//============================================================================
//  BBinkInterface::frameEnd
//============================================================================
void BBinkInterface::frameEnd(void)
{
   ASSERT_RENDER_THREAD
};

//============================================================================
// BBinkInterface::initDeviceData
//============================================================================
void BBinkInterface::initDeviceData(void)
{
   ASSERT_RENDER_THREAD
   Create_Bink_shaders( BD3D::mpDev );

   //We still need regular XAudio for liveVoice
   XAUDIOENGINEINIT EngineInit = { 0 };
   EngineInit.pEffectTable = &XAudioDefaultEffectTable;
   EngineInit.MaxVoiceChannelCount = XHV_XAUDIO_OUTPUT_CHANNEL_COUNT;
   EngineInit.SubmixStageCount = XHV_XAUDIO_SUBMIX_STAGE_COUNT;
   EngineInit.pMasteringVoiceInit = NULL;
   EngineInit.ThreadUsage = XAUDIOTHREADUSAGE_THREAD5;  // Both threads on core 2
   XAudioInitialize( &EngineInit );

   //Use XAudio2 for Bink
   XAudio2Create( &mpXAudio2, 0, XboxThread5 );
   IXAudio2MasteringVoice* pMasteringVoice = NULL;
   mpXAudio2->CreateMasteringVoice( &pMasteringVoice );
   BinkSoundUseXAudio2( mpXAudio2 );

   if (gRender.getWidth() == 640)
      mCaptionFont.Create(mFontDirID, "courier_12_640.xpr");
   else
      mCaptionFont.Create(mFontDirID, "courier_12.xpr");
}

//============================================================================
// BBinkInterface::deinitDeviceData
//============================================================================
void BBinkInterface::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
   Free_Bink_shaders( );  

   XAudioShutDown();
   
   mpXAudio2->Release();
   mpXAudio2 = NULL;

   mCaptionFont.Destroy();
}

