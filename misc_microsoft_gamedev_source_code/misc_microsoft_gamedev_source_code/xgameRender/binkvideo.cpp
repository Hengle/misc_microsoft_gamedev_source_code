// File: binkVideo.cpp
#include "xgameRender.h"
#include "binkVideo.h"
#include "BD3D.h"
#include "renderThread.h"
#include "xmlReader.h"
#include "fileManager.h"
#include "D3DTextureManager.h"
#include "binkInterface.h"

#define XBOX360
#define AKSOUNDENGINE_STATIC
#include "..\xsound\soundmanager.h"


#include <xaudio.h>
#include <xmp.h>

//#define DISABLE_BINK_SOUND

#define BINK_ASYNC_THREAD_Y 1
#define BINK_ASYNC_THREAD_CRCB_SOUND 3

//============================================================================
// BBinkVideo::BBinkVideo
//============================================================================
BBinkVideo::BBinkVideo() :
   mBinkVideoHandle(NULL),
   mHeightScale(1.0f),
   mWidthScale(1.0f),
   mXOffset(0),
   mYOffset(0),
   mPlayState(ePlayState_Normal),
   mPlaySpeed(1),
   mpStatusCallback(NULL),
   mpCaptionHandler(NULL),
   mLoopVideo(false),
   mLoopAtFrame(0),
   mPlayedLastFrame(false),
   mFileManagerThrottling(false),
   mFileManagerIsPaused(false),
   mpMaskTexture(NULL),
   mVideoHandle(cInvalidVideoHandle),
   mDeferredDelete(false),
   mNoSound(false),
   mSoundIndex(cInvalidCueIndex),
   mVolume(BBinkInterface::cMAXIMUM_VOLUME),
   mStartedSound(false),
   mCaptionsVisible(false),
   mIsVisible(true),
   mDisableXMPMusic(false),
   mReadErrorOccured(false),
   mHaveDecompressedFrame(false),
   mPreloadedData(NULL)
{
   Utils::ClearObj(mTextureSet);
}

//============================================================================
// BBinkVideo::operator=
//============================================================================
BBinkVideo& BBinkVideo::operator= (const BBinkVideo& vid)
{
   mBinkVideoHandle=vid.mBinkVideoHandle;
   mHeightScale=vid.mHeightScale;
   mWidthScale=vid.mWidthScale;
   mXOffset=vid.mXOffset;
   mYOffset=vid.mYOffset;
   mPlayState=vid.mPlayState;
   mPlaySpeed = vid.mPlaySpeed;
   mpStatusCallback = vid.mpStatusCallback;
   mpCaptionHandler = vid.mpCaptionHandler;
   mLoopVideo = vid.mLoopVideo;
   mLoopAtFrame = vid.mLoopAtFrame;
   mNoSound = vid.mNoSound;
   mPlayedLastFrame = vid.mPlayedLastFrame;
   mFileManagerThrottling = vid.mFileManagerThrottling;
   mFileManagerIsPaused = vid.mFileManagerIsPaused;
   mpMaskTexture = vid.mpMaskTexture;
   mVideoHandle = vid.mVideoHandle;
   mDeferredDelete = false;
   mReadErrorOccured = vid.mReadErrorOccured;
   mSoundIndex = vid.mSoundIndex;
   mVolume = vid.mVolume;
   mStartedSound = vid.mStartedSound;
   mCaptionsVisible = vid.mCaptionsVisible;
   mDisableXMPMusic = vid.mDisableXMPMusic;
   mHaveDecompressedFrame = vid.mHaveDecompressedFrame;
   mPreloadedData = vid.mPreloadedData;         // jce [11/21/2008] -- yeah this isn't right
   BFAIL("wtf");
   return(*this);
}

//============================================================================
// BBinkVideo::~BBinkVideo
//============================================================================
BBinkVideo::BBinkVideo(const BBinkVideo& vid)
{
   mBinkVideoHandle=vid.mBinkVideoHandle;
   mHeightScale=vid.mHeightScale;
   mWidthScale=vid.mWidthScale;
   mXOffset=vid.mXOffset;
   mYOffset=vid.mYOffset;
   mPlayState=vid.mPlayState;
   mPlaySpeed = vid.mPlaySpeed;
   mpStatusCallback = vid.mpStatusCallback;
   mpCaptionHandler = vid.mpCaptionHandler;
   mLoopVideo = vid.mLoopVideo;
   mLoopAtFrame = vid.mLoopAtFrame;
   mPlayedLastFrame = vid.mPlayedLastFrame;
   mFileManagerThrottling = vid.mFileManagerThrottling;
   mFileManagerIsPaused = vid.mFileManagerIsPaused;
   mpMaskTexture = vid.mpMaskTexture;
   mVideoHandle = vid.mVideoHandle;
   mNoSound = vid.mNoSound;
   mDeferredDelete = false;
   mReadErrorOccured = vid.mReadErrorOccured;
   mSoundIndex = vid.mSoundIndex;
   mVolume = vid.mVolume;
   mStartedSound = vid.mStartedSound;
   mCaptionsVisible = vid.mCaptionsVisible;
   mDisableXMPMusic = vid.mDisableXMPMusic;
   mHaveDecompressedFrame = vid.mHaveDecompressedFrame;
   mPreloadedData = vid.mPreloadedData;         // jce [11/21/2008] -- yeah this isn't right
   BFAIL("wtf");
}

//============================================================================
// BBinkVideo::~BBinkVideo
//============================================================================
BBinkVideo::~BBinkVideo()
{
   destroy();
}

//============================================================================
// BBinkVideo::destroy
//============================================================================
void BBinkVideo::destroy()
{
   if (!mBinkVideoHandle)
      return;
      
   if (mDisableXMPMusic)
   {
      XMPRestoreBackgroundMusic();
   }

   // IMPORTANT: You MUST call resumeIO() if you have paused the file manager! Otherwise all archive I/O will be haulted forever.
   if (mFileManagerIsPaused)
   {
      gFileManager.resumeIO();
      mFileManagerIsPaused = false;
   }
      
   Free_Bink_textures(BD3D::mpDev, &mTextureSet);

   BinkClose(mBinkVideoHandle);
   mBinkVideoHandle = NULL;
         
   mHeightScale = 1.0f;
   mWidthScale = 1.0f;
   mXOffset = 0;
   mYOffset = 0;
   mLoopVideo = false;
   mLoopAtFrame = 0;
   mNoSound = false;
   mPlayedLastFrame = false;
   mFileManagerThrottling = false;
   mSoundIndex = cInvalidCueIndex;
   mVolume = BBinkInterface::cMAXIMUM_VOLUME;
   mStartedSound = false;
   mCaptionData.clear();
   mCaptionsVisible = false;
   mDisableXMPMusic = false;
   mHaveDecompressedFrame = false;
   
   gConsoleOutput.resource("Destroyed Bink video: %s\n", mFilename.getPtr());
   mFilename.empty();

   if (mpMaskTexture)
   {
      mpMaskTexture->release();
      mpMaskTexture=NULL;
   }
   
   if(mPreloadedData)
   {
      //BFAIL("Should not have lingering preloaded data!");
      
      // jce [11/21/2008] -- Deleting but this might be a double delete since this should never happen.
      // jce [11/24/2008] -- Actually it's correct that this is non-null in the non-callback case of endVideos since
      // no one is going to be getting a callback to delete this.
      delete mPreloadedData;
      mPreloadedData = NULL;
   }
}

//============================================================================
// BBinkVideo::setFullscreen
//============================================================================
void BBinkVideo::setFullscreen()
{
   D3DSURFACE_DESC desc;
   BD3D::mpDevBackBuffer->GetDesc(&desc);

   //Always scale the width to take up the full back buffer
   mWidthScale = (float) desc.Width / (float) getVideoWidth();

   //Calculate the height of the video if we scale it the same as the width which
   //would preserve aspect ratio
   unsigned int heightScaledToWidth = (getVideoHeight() * desc.Width) / getVideoWidth();

   //If the calculated height is larger then the screen, then we are not going to see the
   //whole video, so we should squish it down by scaling it independently (this is what
   //should happen for the background UI videos since they are 3:2).  Otherwise, we scale
   //the height the same as the width in order to preserve aspect ratio (this is what
   //should happen for blur and wow cinematics)
   if (heightScaledToWidth > desc.Height)
   {
      mHeightScale = (float) desc.Height / (float) getVideoHeight();
   }
   else
   {
      mHeightScale = (float) desc.Width / (float) getVideoWidth();
   }

   //Calculate offsets so that the video is centered
   mXOffset = ( desc.Width - ( mWidthScale * getVideoWidth() ) ) / 2 ;
   mYOffset = ( desc.Height - ( mHeightScale * getVideoHeight() ) ) / 2;
}
//============================================================================
// BBinkVideo::setLocScale
//============================================================================
void BBinkVideo::setLocScale(float xLoc, float yLoc, float xScale, float yScale)
{
   mXOffset = xLoc;
   mYOffset = yLoc;
   mWidthScale = xScale;
   mHeightScale = yScale;
}
//============================================================================
// BBinkVideo::setLocSize
//============================================================================
void BBinkVideo::setLocSize(float xLoc, float yLoc, uint widthInPixels, uint heightInPixels)
{
   mXOffset = xLoc;
   mYOffset = yLoc;

   mWidthScale = (float) widthInPixels / (float) getVideoWidth();
   mHeightScale = (float) heightInPixels / (float) getVideoHeight();
}

//============================================================================
// void setSoundIndex(BCueIndex index)
//============================================================================
void BBinkVideo::setSoundIndex(BCueIndex index)
{
   mSoundIndex = index;
   mStartedSound = false;
}

//============================================================================
// BBinkVideo::load
//============================================================================
bool BBinkVideo::load(const char* pFilename, int ioBufSize, bool fileManagerThrottling, bool preloaded)
{
   destroy();
      
   mFilename.set(pFilename);
   mFileManagerThrottling = fileManagerThrottling;
      
   U32 openFlags = BINKNOFRAMEBUFFERS;
   if (!preloaded)
   {
      if (ioBufSize > 0)
      {
         BinkSetIOSize(ioBufSize);
         
         openFlags |= BINKIOSIZE;
      }
   }

   openFlags |= BINKSNDTRACK;
   
   if (mFileManagerThrottling)
   {
      gFileManager.pauseIO();
      mFileManagerIsPaused = true;
   }
      
   // If this array is changed, then you also need to update
   // BBinkVideo::setVolume
   U32 TrackIDsToPlay[ 4 ] = { 0, 1, 2, 3 };
   BinkSetSoundTrack( 4, TrackIDsToPlay );

   BinkStartAsyncThread(BINK_ASYNC_THREAD_Y, 0);
   BinkStartAsyncThread(BINK_ASYNC_THREAD_CRCB_SOUND, 0);

   if (preloaded)
   {
      // Preloaded videos are in memory and the address is passed via the ioBufSize param
      openFlags |= BINKFROMMEMORY;
      
      mPreloadedData = (BByteArray*)ioBufSize;
      mBinkVideoHandle = BinkOpen((const char*)mPreloadedData->getPtr(), openFlags);
   }
   else
      mBinkVideoHandle = BinkOpen(pFilename, openFlags);

   if (mBinkVideoHandle)
   {
      // Ask Bink for the buffer details in this new file
      BinkGetFrameBuffersInfo( mBinkVideoHandle, &mTextureSet.bink_buffers );

      // Try to create textures for Bink to use.
      if (Create_Bink_textures( BD3D::mpDev, &mTextureSet ))
      {
         BinkRegisterFrameBuffers( mBinkVideoHandle, &mTextureSet.bink_buffers );// Register our locked texture pointers with Bink
      }
      else
      {
         BinkClose(mBinkVideoHandle);
         mBinkVideoHandle = NULL;
      }
#ifndef DISABLE_BINK_SOUND
      //all of our bink files are 5.1 surround sound
      U32 bins[ 2 ];
      bins[ 0 ] = XAUDIOSPEAKER_FRONTLEFT;
      bins[ 1 ] = XAUDIOSPEAKER_FRONTRIGHT;
      BinkSetMixBins( mBinkVideoHandle, 0, bins, 2 );
      bins[ 0 ] = XAUDIOSPEAKER_FRONTCENTER;
      BinkSetMixBins( mBinkVideoHandle, 1, bins, 1 );
      bins[ 0 ] = XAUDIOSPEAKER_LOWFREQUENCY;
      BinkSetMixBins( mBinkVideoHandle, 2, bins, 1  );
      bins[ 0 ] = XAUDIOSPEAKER_BACKLEFT;
      bins[ 1 ] = XAUDIOSPEAKER_BACKRIGHT;
      BinkSetMixBins( mBinkVideoHandle, 3, bins, 2 );
#endif

      setVolume(mNoSound ? 0 : mVolume);

      if (mDisableXMPMusic)
      {
         XMPOverrideBackgroundMusic();
      }

      gConsoleOutput.resource("Loaded Bink video: %s\n", pFilename);

#ifdef DISABLE_BINK_SOUND
      for (uint i = 0; i < 5; i++)
         BinkSetVolume(mBinkVideoHandle, i, 0);
#endif      
   }
   else
   {
      // IMPORTANT: You MUST call resumeIO() if you have paused the file manager! Otherwise all archive I/O will be haulted forever.
      gFileManager.resumeIO();
      mFileManagerIsPaused = false;            
      
      mFilename.empty();
      mFileManagerThrottling = false;
      
      gConsoleOutput.error("Failed loading Bink video: %s\n", pFilename);

      //-- Play the sound even if the video fails. This will ensure that the chat correctly finishes.
      if (!mStartedSound && (mSoundIndex != cInvalidCueIndex))
      {
         BSoundManager::threadSafePlayCue(mSoundIndex);
         mStartedSound = true;
      }
   }
      
   return mBinkVideoHandle!=NULL;
}
//============================================================================
// BBinkVideo::loadCaptionFile
//============================================================================
bool BBinkVideo::loadCaptionFile(long dirID, const char* pFilename)
{
   BString pathXML("cinimaticCaptions\\");
   
   pathXML += pFilename;

   BXMLReader reader;
   if (!reader.load(dirID, pathXML))
      return (false); 

   BXMLNode rootNode(reader.getRootNode());

   BASSERT(rootNode.getName() == "cinimaticCaption");

   mCaptionData.resize(0);
   
   for (long i=0; i < rootNode.getNumberChildren(); i++)
   {
      BXMLNode node(rootNode.getChild(i));

      if (node.getName().compare("caption") == 0)
      {
         BCaptionLine cl;
         node.getAttribValueAsUInt("startFrame",cl.mStartFrame);
         node.getAttribValueAsUInt("endFrame",cl.mEndFrame);
         node.getAttribValueAsDWORD("color",cl.mColor);
         node.getText(cl.mPrintString);
         mCaptionData.push_back(cl);
      }
   }

   return true;
}

//============================================================================
// BBinkVideo::loadMaskTexture
//============================================================================
bool BBinkVideo::loadMaskTexture(const char* pFilename)
{
   BString filepath = pFilename;
   
   filepath.removeExtension();
   filepath.append(".ddx");

   mpMaskTexture = gD3DTextureManager.getOrCreate(filepath.getPtr());
   if (!mpMaskTexture)
      return false;

   return true;
}

//============================================================================
// BBinkVideo::onReadError
//============================================================================
void BBinkVideo::onReadError()
{  
   mReadErrorOccured = true;
   mDeferredDelete = true;
   mPlayedLastFrame = true;
   setPlayState(ePlayState_Stop);

   BASSERT("Bink video read error");
   // FIXME: Make this TCR compliant, or skip the movie, etc.
   //BFATAL_FAIL("Bink video read error");
}

//============================================================================
// BBinkVideo::binkPercentageFull
//============================================================================
U32 BBinkVideo::binkPercentageFull()
{  
   if (!mBinkVideoHandle)
      return 100;
      
   BINKREALTIME rt;

   BinkGetRealtime(mBinkVideoHandle, &rt, 0);
   if (!rt.ReadBufferSize)
      return 100;
      
   return ((rt.ReadBufferUsed * 100U) / rt.ReadBufferSize);
}

//============================================================================
// BBinkVideo::tickIO
//============================================================================
void BBinkVideo::tickIO()
{
   if (!mBinkVideoHandle || mReadErrorOccured)
      return;
      
   if (mBinkVideoHandle->ReadError)
   {
      onReadError();     
      return;
   }
      
   if (!mFileManagerThrottling)
      return;
      
   if (mPlayedLastFrame)
   {
      if (mFileManagerIsPaused)  
      {
         gFileManager.resumeIO();
         mFileManagerIsPaused = false;  
      }
      
      return;
   }
      
   const uint binkBufferPercentage = binkPercentageFull();
   
   //trace("BufferPercent: %u", binkBufferPercentage);
   
   static uint cResumeThreshold = 70;
   static uint cPauseThreshold = 30;
   
   if (mFileManagerIsPaused)
   {
      if (binkBufferPercentage > cResumeThreshold)
      {
         BinkControlBackgroundIO(mBinkVideoHandle, BINKBGIOSUSPEND);
         
         gFileManager.resumeIO();
               
         mFileManagerIsPaused = false;
      }
   }
   else
   {
      if (binkBufferPercentage < cPauseThreshold)
      {
         gFileManager.pauseIO();
      
         mFileManagerIsPaused = true;
         
         BinkControlBackgroundIO(mBinkVideoHandle, BINKBGIORESUME);
      }
   }
}

//============================================================================
// BBinkVideo::setVolume
//============================================================================
void BBinkVideo::setVolume(uint vol)
{
   mVolume = vol;
   if(mBinkVideoHandle)
   {
      // This loop assumes that only channels 0-3 were set in BinkSetSoundTrack
      // BinkSetSoundTrack is called in BBinkVideo::load
      // If the tracks to play are changed, this loop needs to be updated too.
      for(uint i=0;i<4;i++)
      {
         BinkSetVolume(mBinkVideoHandle, i, vol);
      }
   }
}

//============================================================================
// BBinkVideo::decompressFrame
//============================================================================
bool BBinkVideo::decompressFrame()
{
   BDEBUG_ASSERT(mBinkVideoHandle);
   if (!mBinkVideoHandle || mReadErrorOccured)
      return false;
      
   if (mPlayedLastFrame)
      return true;

   if (mHaveDecompressedFrame && isPaused())
      return true;

   bool continueMovie = true;
      
   if (!mStartedSound && (mSoundIndex != cInvalidCueIndex))
   {
      BSoundManager::threadSafePlayCue(mSoundIndex);
      mStartedSound = true;
   }
      
   // wait for the GPU to finish the previous frame
   Wait_for_Bink_textures(&mTextureSet);
      
   // decompress the next frame (but we don't show it yet)
   BinkDoFrameAsync(mBinkVideoHandle, BINK_ASYNC_THREAD_Y, BINK_ASYNC_THREAD_CRCB_SOUND);
   BinkDoFrameAsyncWait(mBinkVideoHandle, -1);
               
   // do we need to skip a frame?
   switch(mPlayState)
   {

   case ePlayState_FF:
   case ePlayState_RW:
   case ePlayState_Stop:
   case ePlayState_Pause:
      break;

   case ePlayState_Normal:
   default:
      while (BinkShouldSkip(mBinkVideoHandle))
      {
         //support rewind / fast forward
   
         if ((mLoopVideo) || (mBinkVideoHandle->FrameNum < mBinkVideoHandle->Frames))
         {
            BinkNextFrame(mBinkVideoHandle);
            BinkDoFrameAsync(mBinkVideoHandle, BINK_ASYNC_THREAD_Y, BINK_ASYNC_THREAD_CRCB_SOUND);
            BinkDoFrameAsyncWait(mBinkVideoHandle, -1);
         }
         else
         {
            mPlayedLastFrame = true;
            continueMovie = false;
            break;
         }
      }
   }

   mHaveDecompressedFrame = true;
         
   return continueMovie;
}

//============================================================================
// BBinkVideo::renderFrame
//============================================================================
void BBinkVideo::renderFrame()
{
   BDEBUG_ASSERT(mBinkVideoHandle);
   if (!mBinkVideoHandle || mReadErrorOccured)
      return;

   //11/17/2008 - Fix jira bug 18174, we were presenting the bink textures before 
   //uncompressing any frames.  Now we call decompressFrame at least once before rendering.  
   if (!mHaveDecompressedFrame)
   {
      decompressFrame();
   }

   IDirect3DTexture9* pMask = NULL;
   if (mpMaskTexture)
      pMask = mpMaskTexture->getD3DTexture().getTexture();

   if(mIsVisible)
   {
      Draw_Bink_textures( BD3D::mpDev,
         &mTextureSet,
         mBinkVideoHandle->Width, mBinkVideoHandle->Height,
         mXOffset, mYOffset,
         mWidthScale, mHeightScale, pMask );
   
      renderCaptions();
   }
   
   BD3D::mpDev->SetTexture(0,NULL);
   BD3D::mpDev->SetTexture(1,NULL);
   BD3D::mpDev->SetTexture(2,NULL);
   BD3D::mpDev->SetTexture(3,NULL);
   BD3D::mpDev->SetTexture(4,NULL);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);
   
   // wait/input loop: wait until it's time to show the frame
   //if the video is paused, BinkWait will stall, so just skip this..
   if(!mBinkVideoHandle->Paused)
      for ( ; ; )
      {
         if (!BinkWait(mBinkVideoHandle))
            break;

         gEventDispatcher.sleep(1);
      }
   
   tickIO();
}

//============================================================================
// BBinkVideo::renderCaptions
//============================================================================
void BBinkVideo::renderCaptions()
{
   BDEBUG_ASSERT(mBinkVideoHandle);
   if (!mBinkVideoHandle || mReadErrorOccured)
      return;
      
   if (!mpCaptionHandler)
      return;

   if (mCaptionData.isEmpty())
      return;

   uint currFrame = mBinkVideoHandle->FrameNum;
   bool bFound = false;
   int  index  = -1;
   for(uint i = 0; !bFound && i < mCaptionData.size(); i++)
   {
      if(mCaptionData[i].mStartFrame <= currFrame && 
         mCaptionData[i].mEndFrame > currFrame)
      {  
         bFound = true;
         index = i;         
      }               
   }

   if (bFound)
   {
      debugRangeCheck(index, mCaptionData.size());
      if (!mCaptionData[index].mAlreadyPlayed)
      {
         mpCaptionHandler->enableCaption(this, true);
         mpCaptionHandler->renderCaption(this, mCaptionData[index].mPrintString, mCaptionData[index].mColor);
         mCaptionData[index].mAlreadyPlayed = true;
         mCaptionsVisible=true;
      }
   }   
   else
   {
      if (mCaptionsVisible)
      {
         mpCaptionHandler->enableCaption(this, false);
         mCaptionsVisible=false;
      }
   }
}

//============================================================================
// BBinkVideo::advanceFrame
// BEWARE! This will loop back to frame 0 if called on the end frame automatically!!!
//============================================================================
void BBinkVideo::advanceFrame()
{
   BDEBUG_ASSERT(mBinkVideoHandle);
   if (!mBinkVideoHandle || mReadErrorOccured)
      return;
      
   //support rewind / fast forward
   switch(mPlayState)
   {

   case ePlayState_FF:
      {
         const uint frameNum = mBinkVideoHandle->FrameNum + mPlaySpeed;

         // [7/28/2008 xemu] Check for target-frame looping, and zip around to frame zero if needed
         if ((mLoopAtFrame > 0) && (mBinkVideoHandle->FrameNum > mLoopAtFrame))
            BinkGoto(mBinkVideoHandle, 0, 0);
         else if ((mLoopVideo) || (frameNum < mBinkVideoHandle->Frames))
            BinkGoto(mBinkVideoHandle, frameNum, 0);
         else
            mPlayedLastFrame = true;

         
         break;
      }

   case ePlayState_RW:
      {
         const uint frameNum = mBinkVideoHandle->FrameNum - mPlaySpeed;
         if(frameNum < 0)
         {
            //CLM we need to use the GOTOQUICK flag here, otherwise we'll get caught
            // seeking to the keyframe, and interpolating towards our frame
            BinkGoto(mBinkVideoHandle, 1, BINKGOTOQUICK);
            setPlayState(ePlayState_Stop);
         }
         else
            BinkGoto(mBinkVideoHandle, frameNum, BINKGOTOQUICK);

         break;
      }

   case ePlayState_Stop:
      {
         BinkGoto(mBinkVideoHandle, 1, BINKGOTOQUICK);
         mPlayedLastFrame = true;


         break;
      }
   case ePlayState_Pause:
      {
        
         break;
      }
   case ePlayState_Normal:
   default:
      {
         // [7/28/2008 xemu] Check for target-frame looping, and zip around to frame zero if needed
         if ((mLoopAtFrame > 0) && (mBinkVideoHandle->FrameNum > mLoopAtFrame))
            BinkGoto(mBinkVideoHandle, 0, 0);
         else if ((mLoopVideo) || (mBinkVideoHandle->FrameNum < mBinkVideoHandle->Frames))
            BinkNextFrame(mBinkVideoHandle);
         else
            mPlayedLastFrame = true;
      }
   };
}


//============================================================================
// BBinkVideo::play
//============================================================================
void BBinkVideo::play()
{
   BinkPause(mBinkVideoHandle,0);   //incase we're paused
   setPlayState(ePlayState_Normal); 
   mPlaySpeed = 1;

   if (!mStartedSound && mSoundIndex != cInvalidCueIndex)
      BSoundManager::threadSafePlayCue(mSoundIndex);
}

//============================================================================
// BBinkVideo::skipForwardFrames
//============================================================================
void BBinkVideo::pause()      
{
   if(mPlayState == ePlayState_Pause)
   {
      setPlayState(ePlayState_Normal);
      BinkPause(mBinkVideoHandle,0);
   }
   else if(mPlayState != ePlayState_Pause)
   {
      setPlayState(ePlayState_Pause);
      BinkPause(mBinkVideoHandle,1);
   }
}

//============================================================================
// BBinkVideo::skipForwardFrames
//============================================================================
void BBinkVideo::skipForwardFrames(uint numFrames)
{
   const uint frameNum = mBinkVideoHandle->FrameNum + numFrames;

   if ((frameNum < mBinkVideoHandle->Frames))
   {
      BinkGoto(mBinkVideoHandle, frameNum, 0);
   }
   else
   {
      BinkGoto(mBinkVideoHandle, mBinkVideoHandle->Frames-1, 0);
      mPlayedLastFrame = true;
   }
}

//============================================================================
// BBinkVideo::skipForwardSeconds
//============================================================================
void BBinkVideo::skipForwardSeconds(uint numSeconds)
{
   const uint numFrameSec = mBinkVideoHandle->FrameRateDiv;
   const uint numFramesToSkip = numFrameSec * numSeconds;

   skipForwardFrames(numFramesToSkip);
}

//============================================================================
// BBinkVideo::skipBackwardFrames
//============================================================================
void BBinkVideo::skipBackwardFrames(uint numFrames)
{
   const int frameNum = mBinkVideoHandle->FrameNum - numFrames;

   if (frameNum > 0)
   {
      BinkGoto(mBinkVideoHandle, frameNum, 0);
   }
   else
   {
      BinkGoto(mBinkVideoHandle, 1, 0);
   }
}

//============================================================================
// BBinkVideo::skipBackwardSeconds
//============================================================================
void BBinkVideo::skipBackwardSeconds(uint numSeconds)
{
   const uint numFrameSec = mBinkVideoHandle->FrameRateDiv;
   const uint numFramesToSkip = numFrameSec * numSeconds;

   skipBackwardFrames(numFramesToSkip);
}

//============================================================================
// BBinkVideo::isEndOfMovie
//============================================================================
bool BBinkVideo::isEndOfMovie()
{
   BDEBUG_ASSERT(mBinkVideoHandle);
   if (!mBinkVideoHandle)
      return true;
      
   return mBinkVideoHandle->FrameNum == mBinkVideoHandle->Frames;
}

//============================================================================
// BBinkVideo::rewindToStart
//============================================================================
void BBinkVideo::rewindToStart()
{
   BDEBUG_ASSERT(mBinkVideoHandle);
   if (!mBinkVideoHandle)
      return;
      
   BinkGoto(mBinkVideoHandle, 1, 0);
   mPlayedLastFrame = false;
}

