//============================================================================
// flashbackgroundplayer.h
// 
// Ensemble Studios (C) 2007
//============================================================================
#include "xgameRender.h"
#include "flashbackgroundplayer.h"
#include "renderThread.h"
#include "threading\eventDispatcher.h"
#include "BD3D.h"
#include "xmlreader.h"
#include "renderHelperThread.h"
#include "threading\workDistributor.h"
#include "renderDraw.h"
#include "binkVideo.h"
#include "render.h"
#include "color.h"
#include "inputsystem.h"
#include "..\xgame\LocaleManager.h"
#include "..\xgame\gamedirectories.h"
#include "config.h"
#include "configsgamerender.h"
#include "miniLoadManager.h"
#include "binkInterface.h"

BFlashBackgroundPlayer gFlashBackgroundPlayer;
BCriticalSection BFlashBackgroundPlayer::mCanQuickCancelLock(10000);

//============================================================================
//============================================================================
BFlashBackgroundPlayer::BFlashBackgroundPlayer():
   mIsLoaded(false),
   mIsStarted(false),
   mInsidePlayMovieCallback(false),
   mIsWaitingForBinkMovieToFinish(false),
   mpMoviePlayer(NULL),
   mpMoviePlayerCancel(NULL),
   mRenderEventHandle(cInvalidEventReceiverHandle),
   mCaptionsEnabled(false),
   mpBinkVideo(NULL),
   mpMovieManager(NULL),
   mFontDirID(0),
   mCaptionsDirID(0),
   mNextTipIndex(0),
   mLastInputTime(0),
   mLastTipTime(0),
   mMinVideoTime(0),
   mStartVideoTime(0),
   mBinkVideoVolume(BBinkInterface::cMAXIMUM_VOLUME),
   mStopMovie(false),
   mMovieUIVisible(false),
   mCanPause(true),
   mCanQuickCancel(false),
   mExternalReadError(0),
   mUseMiniLoad(false)
{
}

//============================================================================
//============================================================================
BFlashBackgroundPlayer::~BFlashBackgroundPlayer()
{
}

//============================================================================
//============================================================================
bool BFlashBackgroundPlayer::init(long fontDirID)
{
   // fixme - when we move to a flash based solution, we can get rid of this stuff.
   mFontDirID = fontDirID;

   mRandomTips.clear();    // we don't own the pointers.

   eventReceiverInit(cThreadIndexRenderHelper);
   
   if (mRenderEventHandle == cInvalidEventReceiverHandle)
      mRenderEventHandle = gEventDispatcher.addClient(this, cThreadIndexRender);
         
   return true;
}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::deinit()
{
   mRandomTips.clear();    // we don't own the pointers.

   if (mpMoviePlayer)
   {
      mpMoviePlayer->Deinit();
      delete mpMoviePlayer;
      mpMoviePlayer = NULL;
   }

   if (mpMoviePlayerCancel)
   {
      mpMoviePlayerCancel->Deinit();
      delete mpMoviePlayerCancel;
      mpMoviePlayerCancel = NULL;
   }

   if (mpMovieManager)
   {
      mpMovieManager->deinit();
      delete mpMovieManager;
      mpMovieManager = NULL;
   }

   if (mRenderEventHandle != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientDeferred(mRenderEventHandle, true);
      mRenderEventHandle = cInvalidEventReceiverHandle;
   }
         
   eventReceiverDeinit();
}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::setExternalReadError()
{
   Sync::InterlockedExchangeExport(&mExternalReadError,0xFFFFFFFF);
}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::loadMovie(const char* pMovie, const char * pFontDefinitionFile, const char* pBinkVideo, const char* pCaptionsFile, long captionsDirID,bool noCancelMovieUI, bool useMiniLoad)
{
   SCOPEDSAMPLE(BFlashBackgroundPlayer_loadMovie)
   ASSERT_MAIN_THREAD

   if (mIsLoaded)
      return;

   gRenderDraw.unsetResources(BRenderDraw::cUnsetTextures | BRenderDraw::cUnsetIndices | BRenderDraw::cUnsetStreams | BRenderDraw::cUnsetShaders);
            
   // release from render thread to this thread (the main thread)
   gRenderThread.releaseThreadOwnership();

   // store if we are using the mini load screen or not
   mUseMiniLoad = useMiniLoad;

   // we are using the mini load screen.  We must first release the device as the
   // mini load screen will require the device once it has been started.
   // this was added for TCR #6 compliance (11/21/2008 Jira 18462)
   if (mUseMiniLoad)
   {
      // release from the main thread
      BD3D::mpDev->ReleaseThreadOwnership();

      // start the mini load screen
      gMiniLoadManager.start();
   }
   else
   {
      //we're not using the mini load screen so just simply release to the system
      //11/14/2008 - This was changed from ReleaseThreadOwnership() to Suspend() for
      //TCR #22 compliance.  We are required to call present every 66ms.  When we call Suspend() 
      //we allow the system control of the D3D device so it can call present for us.  (See Jira 10256)
      BD3D::mpDev->Suspend();
   }

   
   mMovieToLoad.set(pMovie ? pMovie : "");
   mBinkVideoVolume = gBinkInterface.getVolume();
     
   mBinkVideoToPlay.set(pBinkVideo ? pBinkVideo : "");
   mBinkVideoCaptionsFile.set(pCaptionsFile ? pCaptionsFile : "");
   mCaptionsDirID=captionsDirID;

   // hack, hack, hack
   if (!noCancelMovieUI && mBinkVideoToPlay.length() > 0)
      mCancelMovieToLoad.set("art\\ui\\flash\\backgroundplayer\\cinematicCancel.swf");
   else
      mCancelMovieToLoad.set("");

   
   mExitEvent.reset();
   mTerminationCompleteEvent.reset();
   mPlayMovieCallbackActive.reset();
   mBinkVideoIsFinished.reset();

   BFixedString128 language = gLocaleManager.getLanguageString();

   BFixedString128 fontDefinitionFile;
   fontDefinitionFile.set(pFontDefinitionFile ? pFontDefinitionFile : "");
   
   BSimString tempStr;
   gConfig.get(cConfigFlashCustomWordWrappingMode, tempStr);
   BFixedString128 wordWrapMode;   
   wordWrapMode=tempStr;

   gEventDispatcher.send(mEventHandle, mEventHandle, eFBPEventLoad, 0, 0, new BLoadMoviePayload(language, cDirData, fontDefinitionFile, wordWrapMode), 0);
   
   mIsLoaded = true;
}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::invokeActionScript(const char* method, const GFxValue* pArgs, int argCount)
{
   ASSERT_MAIN_THREAD
   if (!mIsLoaded)
      return;

   // fixme - use defined constant
   debugRangeCheck<int, int>(argCount, 8);

   BFBPActionScriptCommandPayload* pPayload = new BFBPActionScriptCommandPayload(BStrConv::toA(method));

   pPayload->method = method;
   pPayload->argCount = argCount;
   for (int i=0; i<argCount; i++)
   {      
      if (pArgs[i].GetType() == GFxValue::VT_String)
         pPayload->args[i].SetString(pArgs[i].GetString());
      else if (pArgs[i].GetType() == GFxValue::VT_StringW)
         pPayload->args[i].SetStringW(pArgs[i].GetStringW());
      else
         pPayload->args[i] = pArgs[i];
   }   

   gEventDispatcher.send(mEventHandle, mEventHandle, eFBPEventActionScript, 0, 0, pPayload);
}


//============================================================================
//============================================================================
void BFlashBackgroundPlayer::startMovie()
{
   ASSERT_MAIN_THREAD
   if ((!mIsLoaded) || (mIsStarted))
      return;
      
   gEventDispatcher.send(mEventHandle, mEventHandle, eFBPEventPlay, 0, 0, NULL);
   
   mIsStarted = true;
}

//============================================================================
// There is much dark mojo here. Beware of modifying it.
//============================================================================
void BFlashBackgroundPlayer::stopMovie(bool waitForBinkMovieToFinish)
{
   SCOPEDSAMPLE(BFlashBackgroundPlayer_stopMovie)

   ASSERT_MAIN_THREAD
   if (!mIsLoaded)
      return;
      
   trace("BFlashBackgroundPlayer::stopMovie: Waiting for render thread to idle");
   
   // Must block here because the flash player is still loading on the render thread, and we want the background movie to continue playing.
   gRenderThread.blockUntilWorkerIdle();
   
   trace("BFlashBackgroundPlayer::stopMovie: Waiting for render helper queue to empty");
      
   gEventDispatcher.waitUntilThreadQueueEmpty(cThreadIndexRenderHelper);
   
   if (mIsStarted)
   {
      trace("BFlashBackgroundPlayer::stopMovie: Waiting for movie callback to be active");
      
      gEventDispatcher.waitSingle(mPlayMovieCallbackActive);   
      
      if ((waitForBinkMovieToFinish) && (!mBinkVideoToPlay.isEmpty()))
      {
         mIsWaitingForBinkMovieToFinish = true;
         
         trace("BFlashBackgroundPlayer::stopMovie: Waiting for bink movie to finish");
         
         gWorkDistributor.waitSingle(mBinkVideoIsFinished);
         
         mIsWaitingForBinkMovieToFinish = false;
      }
   
      mExitEvent.set();
   
      trace("BFlashBackgroundPlayer::stopMovie: Waiting for movie callback terminate");
            
      gEventDispatcher.waitSingle(mTerminationCompleteEvent);
      
      mIsStarted = false;
   }
                    
   // We called Suspend() at the end of the function 'playMovieCallback' so we need
   // to call Resume here.  This will acquire ownership onto this thread (the main thread) from the system.
   BD3D::mpDev->Resume();
   
   // Give ownership from this thread back to the render thread
   gRenderThread.acquireThreadOwnership();
   
   trace("BFlashBackgroundPlayer::stopMovie: Waiting for render thread to idle");
   
   // Must wait for the render thread to acquire the D3D device before continuing.
   gRenderThread.blockUntilWorkerIdle();
   
   gEventDispatcher.send(mEventHandle, mRenderEventHandle, eFBPEventUnload, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);
   
   trace("BFlashBackgroundPlayer::stopMovie: done");
      
   mIsLoaded = false;
}
   
//============================================================================
//============================================================================   
void BFlashBackgroundPlayer::playMovieCallback(void)
{
   SCOPEDSAMPLE(BFlashBackgroundPlayer_playMovieCallback)
   mPlayMovieCallbackActive.set();
   
   // Disallow reentrant callbacks.
   mInsidePlayMovieCallback = true;
      
   // if we are using the mini load screen we need to stop it.  We will have also given
   // it the device so we must take the device back from it.  If we are not using the
   // mini load screen we will just need to take the device back from the system
   if (mUseMiniLoad)
   {
      //End the mini load thread
      gMiniLoadManager.stop();

      //Acquire the device
      BD3D::mpDev->AcquireThreadOwnership();
   }
   else
   {
      //Acquire D3D thread ownership back from the system
      BD3D::mpDev->Resume();
   }
   
   DWORD savedPresentInterval;
   BD3D::mpDev->GetRenderState(D3DRS_PRESENTINTERVAL, &savedPresentInterval);
   BD3D::mpDev->SetRenderState(D3DRS_PRESENTINTERVAL, D3DPRESENT_INTERVAL_TWO);

   mpBinkVideo = NULL;
   if (!mBinkVideoToPlay.isEmpty())
   {
      mpBinkVideo = new(gRenderHeap) BBinkVideo();
      mpBinkVideo->setCaptionHandler(this);

      //Turn xbox media player music off while playing fullscreen cinematics
      //Must be called before BBinkVideo::load() to have any effect
      //Should only be called for non-interactive FMV sequences
      mpBinkVideo->setDisableXMPMusic(true);

      mpBinkVideo->setVolume(mBinkVideoVolume);

      const int cBinkVideoBufferSize = 16U * 1024U * 1024U;
      bool success = mpBinkVideo->load(mBinkVideoToPlay.getPtr(), cBinkVideoBufferSize, true);
      //bool success = mpBinkVideo->load(mBinkVideoToPlay.getPtr(), -1, true);
      if (!success)
      {
         heapDelete(mpBinkVideo, gRenderHeap);
         mpBinkVideo = NULL;
         
         mBinkVideoIsFinished.set();
      }
      else
      {
         if (mBinkVideoCaptionsFile.length() > 0)
            mpBinkVideo->loadCaptionFile(mCaptionsDirID, mBinkVideoCaptionsFile.getPtr());

         mpBinkVideo->setLoopVideo(false);         
         mpBinkVideo->setFullscreen();
         mStartVideoTime = timeGetTime();
      }
   }
   
   SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

   mMovieUIVisible=false;
   mStopMovie=false;

   mNextTipIndex=0;
   mLastTipTime=timeGetTime();
   displayNextTip();

   uint frameIndex = 0;
   for ( ; ; )
   {
      // CLM [11.21.08] If there's been a disk read error just spin loop so someone else can show the DirtyDisk error
      if(mExternalReadError)
      {
         BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
         BD3D::mpDev->Present(0, 0, 0, 0);
         continue;
      }

      BD3D::mpDev->BeginScene();
      BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
      
      gRenderDraw.workerSetDefaultRenderStates();
      gRenderDraw.workerSetDefaultSamplerStates();

      // Render any cinematics we have
      renderMovie();

      // If the video is over and the game is loaded, then we want to stop.
      const bool gameLoadDone = mIsWaitingForBinkMovieToFinish;
      if (gameLoadDone && !mpBinkVideo)
         break;

      // Render any UI
      renderUI();

      // update other things
      update();

      BD3D::mpDev->EndScene();
      
      gRenderDraw.checkForFailOrMeltdown();
                     
      BD3D::mpDev->Present(0, 0, 0, 0);
            
      frameIndex++;
      if (gEventDispatcher.waitSingle(mExitEvent.getHandle(), 0, true) == 0)
         break;
   }
   
   SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
         
   BD3D::mpDev->BeginScene();
   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
   BD3D::mpDev->EndScene();
   gRenderDraw.checkForFailOrMeltdown();
   BD3D::mpDev->Present(0, 0, 0, 0);
   
   if (mpBinkVideo)
   {
      heapDelete(mpBinkVideo, gRenderHeap);
      mpBinkVideo = NULL;
      
      mBinkVideoIsFinished.set();
   }

   mRandomTips.clear();    // we don't own the pointers, but we want to clear our array.

   BD3D::mpDev->SetRenderState(D3DRS_PRESENTINTERVAL, savedPresentInterval);
   
   gRenderDraw.unsetResources(BRenderDraw::cUnsetTextures | BRenderDraw::cUnsetIndices | BRenderDraw::cUnsetStreams | BRenderDraw::cUnsetShaders);
         
   BD3D::mpDev->BlockUntilIdle();
   
   //Suspend D3D thread ownership back to the system
   //11/14/2008 - This was changed from ReleaseThreadOwnership() to Suspend() for
   //TCR #22 compliance.  We are required to call present every 66ms.  When we call Suspend() 
   //we allow the system control of the D3D device so it can call present for us.  (See Jira 10256)
   //11/21/2008 - We don't need to go back to the mini loading screen here as the time it takes
   //to exit the flash background player is very quick.
   BD3D::mpDev->Suspend();
   
   mInsidePlayMovieCallback = false;
   
   // Always do this last
   mTerminationCompleteEvent.set();
}   

//============================================================================
//============================================================================
bool BFlashBackgroundPlayer::update()
{
   // process the input
   gInputSystem.update(this);

   // if the movie UI is up 
   if (mMovieUIVisible)
   {
      // if the movie is up and paused, then don't turn off the UI.
      if (mpBinkVideo && mpBinkVideo->isPaused())
         return true;

      // If input been idle for at least 3 seconds?
         // hide the UI
      DWORD now = timeGetTime();
      if ((mLastInputTime+3000)<now)
      {
         mMovieUIVisible=false;
         mpMoviePlayerCancel->Invoke("easeOut", NULL, 0);
      }
   }

   if (!mpBinkVideo)
   {
      DWORD now = timeGetTime();
      // if the movie isn't up, then check the tips
      if ((mLastTipTime+10000)<now)
      {
         displayNextTip();
         mLastTipTime=now;
      }
   }

   return true;   
}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::displayNextTip()
{
   if (mRandomTips.getNumber() == 0)
      return;

   if (mNextTipIndex >= mRandomTips.getNumber())
      mNextTipIndex=0;

   GFxValue values[2];
   values[0].SetString("mTips");
   values[1].SetStringW( mRandomTips[mNextTipIndex]->getPtr() );
   mpMoviePlayer->Invoke("setText", values, 2);

   mNextTipIndex++;
}

//============================================================================
//============================================================================
bool BFlashBackgroundPlayer::renderUI()
{
   // if we are playing a movie, then we want to render one UI, otherwise render the loading screen.

   BD3D::mpDev->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );

   if (mpBinkVideo)
   {
      if (mpMoviePlayerCancel)
         mpMoviePlayerCancel->Tick();
   }
   else
   {
      if (mpMoviePlayer)
         mpMoviePlayer->Tick();
   }

   return true;
}

//============================================================================
//============================================================================
bool BFlashBackgroundPlayer::renderMovie()
{
   if (!mpBinkVideo)
      return false;

   // decompress the frame
   bool success = mpBinkVideo->decompressFrame();

   // render the frames and captions
   if (success)
   {
      mpBinkVideo->renderFrame();
      mpBinkVideo->renderCaptions();
      mpBinkVideo->advanceFrame();
   }

   // do we need to stop our movie (failure, input, movie ended)
   if (!success || mStopMovie || mpBinkVideo->isEndOfMovie())
   {
      heapDelete(mpBinkVideo, gRenderHeap);
      mpBinkVideo = NULL;

      // notify the other thread that we are done loading the movie
      mBinkVideoIsFinished.set();
   }

   return true;
}


//============================================================================
//============================================================================
void BFlashBackgroundPlayer::playMovieCallbackFunc(void* pPrivateData)
{
   static_cast<BFlashBackgroundPlayer*>(pPrivateData)->playMovieCallback();
}

//============================================================================
//============================================================================
bool BFlashBackgroundPlayer::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   // Be careful what you do in here! We could be playing back a movie within the play movie callback.
   switch (event.mEventClass)
   {
      case eFBPEventLoad:
      {
         BFlashBackgroundPlayer::BLoadMoviePayload* pPayload = static_cast<BFlashBackgroundPlayer::BLoadMoviePayload*>(event.mpPayload);

         // The main loading screen movie
         if (mpMoviePlayer)
         {
            mpMoviePlayer->Deinit();
            delete mpMoviePlayer;
            mpMoviePlayer = NULL;
         }

         if (mpMovieManager)
         {
            mpMovieManager->deinit();
            delete mpMovieManager;
            mpMovieManager = NULL;
         }
              
         if (mMovieToLoad.length() <= 0)
            break;

         mpMovieManager = new BGFxMovieManager();

         FxPlayerSettings settings;
         mpMovieManager->init(settings, pPayload->mLanguage, pPayload->mDirectory, pPayload->mFontDefinitionFile, pPayload->mWordWrapMode);

         mpMoviePlayer = new BGFXMoviePlayer();

         //-- Init the player         
         mpMoviePlayer->Init(mpMovieManager);         

         //-- Load the movie
         bool success = mpMoviePlayer->LoadMovie(cDirProduction, mMovieToLoad.getPtr());
         if (!success)
         {
            trace("Failed loading flash movie %s", mMovieToLoad.getPtr());

            gConsoleOutput.error("Failed loading flash movie %s", mMovieToLoad.getPtr());
         }
         else
         {
            trace("Loaded flash movie %s", mMovieToLoad.getPtr());

            gConsoleOutput.resource("Loaded flash movie %s", mMovieToLoad.getPtr());
         }

         if (mCancelMovieToLoad.length() <= 0)
            break;

         // The cancel button movie
         if (mpMoviePlayerCancel)
         {
            mpMoviePlayerCancel->Deinit();
            delete mpMoviePlayerCancel;
         }
            
         mpMoviePlayerCancel = new BGFXMoviePlayer();
         
         //-- Init the player                  
         mpMoviePlayerCancel->Init(mpMovieManager);

         //-- Load the movie
         success = mpMoviePlayerCancel->LoadMovie(cDirProduction, mCancelMovieToLoad.getPtr());
         if (!success)
         {
            trace("Failed loading flash movie %s", mCancelMovieToLoad.getPtr());
            gConsoleOutput.error("Failed loading flash movie %s", mCancelMovieToLoad.getPtr());
         }
         else
         {
            trace("Loaded flash movie %s", mCancelMovieToLoad.getPtr());
            gConsoleOutput.resource("Loaded flash movie %s", mCancelMovieToLoad.getPtr());
         }
         
         break;                  
      }
      case eFBPEventActionScript:
      {
         //-- Call action script on the movie
         BFBPActionScriptCommandPayload* pData = reinterpret_cast<BFBPActionScriptCommandPayload*>(event.mpPayload);         
         BDEBUG_ASSERT(pData!=NULL);

         BDEBUG_ASSERT(mpMoviePlayer!=NULL);
                        
         mpMoviePlayer->Invoke(pData->method, pData->args, pData->argCount);

         break;
      }
      case eFBPEventUnload:
      {
         if (!mInsidePlayMovieCallback)
         {            
            if (mpMoviePlayer)
            {
               mpMoviePlayer->UnloadMovie();
               mpMoviePlayer->Deinit();
               delete mpMoviePlayer;
               mpMoviePlayer = NULL;
            }

            // unload the cancel movie if necessary
            if (mpMoviePlayerCancel)
            {
               mpMoviePlayerCancel->UnloadMovie();
               mpMoviePlayerCancel->Deinit();
               delete mpMoviePlayerCancel;
               mpMoviePlayerCancel = NULL;
            }          

            if (mpMovieManager)
            {
               mpMovieManager->deinit();
               delete mpMovieManager;
               mpMovieManager = NULL;
            }
         }
         
         break;
      }
      case eFBPEventPlay:
      {
         if (!mInsidePlayMovieCallback)
            gRenderHelperThread.invokeCallback(playMovieCallbackFunc, this);
         break;
      }
   }

   return false;
}

//============================================================================
// BFlashBackgroundPlayer::renderCaption
//============================================================================
void BFlashBackgroundPlayer::renderCaption(BBinkVideo * pVideo, const BUString& string, DWORD color)
{
   GFxValue value;
   value.SetStringW(string.getPtr());
   mpMoviePlayerCancel->Invoke("setCaptionText", &value, 1);

}

//============================================================================
// BFlashBackgroundPlayer::enableCaption
//============================================================================
void BFlashBackgroundPlayer::enableCaption(BBinkVideo * pVideo, bool bEnable)
{
   if (mCaptionsEnabled == bEnable)
      return;

   mCaptionsEnabled = bEnable;
   GFxValue value;
   value.SetBoolean(mCaptionsEnabled);

   mpMoviePlayerCancel->Invoke("showCaptions", &value, 1);
}

//============================================================================
//============================================================================
bool BFlashBackgroundPlayer::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // we don't handle any input after the movie is done
   if (!mpBinkVideo)
      return true;   // eat the input

   // are we allowed to quick cancel the video?
   if (getCanQuickCancel())
   {
      if(event==cInputEventControlStart)
      {
         switch(controlType)
         {
            case cButtonStart:
            case cButtonA:
            case cButtonB:
            case cButtonX:
            case cButtonY:
               if (mMinVideoTime == 0 || timeGetTime() - mStartVideoTime >= mMinVideoTime)
                  mStopMovie=true;
               break;
         }
      }
      return true;
   }

   // are we allowed to pause and cancel the video?
   if (mMovieUIVisible)
   {
      // movie UI is up.
      if(event==cInputEventControlStart)
      {
         mLastInputTime = timeGetTime();
         switch(controlType)
         {
            case cButtonStart:
            case cButtonA:
               {
                  // If we can't cancel, then just 
                  if (!getCanPause())
                     break;

                  // change the state of the video
                  if (mpBinkVideo->isPaused())
                     mpBinkVideo->play();
                  else
                     mpBinkVideo->pause();

                  setMoviePlayerUIButtons();
               }
               break;

            case cButtonX:
               {
/*
                  const bool gameLoadDone = mIsWaitingForBinkMovieToFinish;
                  if (!gameLoadDone)
                     break;                  // this shouldn't work until the game is loaded.
*/
                  // always let them stop the movie now.

                  mStopMovie=true;
               }
               break;
         }
      }

      return true;
   }

   // Movie UI is not up
   if(event==cInputEventControlStart)
   {
      mLastInputTime = timeGetTime();
      switch(controlType)
      {
         case cButtonStart:
            {
               // pause the movie, show the buttons
               if (mpMoviePlayerCancel)
               {
                  if (getCanPause())
                     mpBinkVideo->pause();

                  setMoviePlayerUIButtons();

                  // show the status.
                  mpMoviePlayerCancel->Invoke("show", NULL, 0);
                  mMovieUIVisible=true;
               }
            }

         case cButtonA:
         case cButtonB:
         case cButtonX:
         case cButtonY:
         case cButtonBack:
         case cDpad:
         case cStickLeft:
         case cStickRight:
         case cButtonShoulderRight:
         case cButtonShoulderLeft:
         case cTriggerLeft:
         case cTriggerRight:
            // make a call to the movie to show the ui
            if (mpMoviePlayerCancel)
            {
               setMoviePlayerUIButtons();

               mpMoviePlayerCancel->Invoke("show", NULL, 0);
               mMovieUIVisible=true;
            }

         break;
      }
   }
   return true;
}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::setMoviePlayerUIButtons()
{
   // call the action script to change the text on the buttons
   GFxValue values[2];
   if (mpBinkVideo->isPaused())
      values[0].SetStringW(mStringPlay.getPtr());
   else
      values[0].SetStringW(mStringPause.getPtr());

   values[1].SetStringW(mStringSkip.getPtr());
   mpMoviePlayerCancel->Invoke("setButtonText", values, 2);

   // show, hide the buttons
   if (getCanPause())
      values[0].SetBoolean(true);
   else
      values[0].SetBoolean(false);

   // const bool gameLoadDone = mIsWaitingForBinkMovieToFinish;
   // values[1].SetBoolean(gameLoadDone);
   values[1].SetBoolean(true);   // always now letting X cancel the video.
   mpMoviePlayerCancel->Invoke("setButtonsActive", values, 2);

}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::setPlayPauseSkipStrings(const BUString& playString, const BUString& pauseString, const BUString& skipString)
{
   mStringPlay = playString;
   mStringPause = pauseString;
   mStringSkip = skipString;

}

//============================================================================
//============================================================================
bool BFlashBackgroundPlayer::getCanQuickCancel() 
{ 
   BScopedCriticalSection lock(mCanQuickCancelLock);
   return mCanQuickCancel; 
}

//============================================================================
//============================================================================
void BFlashBackgroundPlayer::setCanQuickCancel(bool canQuickCancel, DWORD minVideoTime) 
{ 
   BScopedCriticalSection lock(mCanQuickCancelLock);
   mCanQuickCancel = canQuickCancel; 
   mMinVideoTime = mMinVideoTime; 
}