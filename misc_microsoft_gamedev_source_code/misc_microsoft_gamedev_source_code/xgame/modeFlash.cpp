//==============================================================================
// modeflash.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modeflash.h"
#include "database.h"
#include "fontSystem2.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "modemanager.h"
#include "modemenu.h"
#include "render.h"
#include "ui.h"
#include "configsgame.h"
#include "workdirsetup.h"
#include "reloadManager.h"
#include "flashmanager.h"
#include "world.h"

// xgameRender

//==============================================================================
// BModeFlash::BModeFlash
//==============================================================================
BModeFlash::BModeFlash(long modeType) :
   BMode(modeType),
   BEventReceiver(),
   mState(-1),
   mNextState(cStateMain),
   mLoaded(false),
   mpMovie(NULL)
{
   ASSERT_MAIN_THREAD
   
   eventReceiverInit();
   commandListenerInit();

   BReloadManager::BPathArray paths;
   BString fullname;
   gFileManager.constructQualifiedPath(cDirProduction, "*.swf", fullname);      
   paths.pushBack(fullname);
   gFileManager.constructQualifiedPath(cDirProduction, "*.gfx", fullname);         
   paths.pushBack(fullname);

   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous | BReloadManager::cFlagSubDirs, mEventHandle);
}

//==============================================================================
// BModeFlash::~BModeFlash
//==============================================================================
BModeFlash::~BModeFlash()
{
   ASSERT_MAIN_THREAD
   
   gReloadManager.deregisterClient(mEventHandle);
   
   commandListenerDeinit();
   eventReceiverDeinit();
}

//==============================================================================
// BModeFlash::setup
//==============================================================================
bool BModeFlash::setup()
{
   ASSERT_MAIN_THREAD
   
   return BMode::setup();
}

//==============================================================================
// BModeFlash::shutdown
//==============================================================================
void BModeFlash::shutdown()
{
   ASSERT_MAIN_THREAD
   
   BMode::shutdown();
}

//==============================================================================
// BModeFlash::preEnter
//==============================================================================
void BModeFlash::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
// BModeFlash::enter
//==============================================================================
void BModeFlash::enter(BMode* lastMode)
{
   ASSERT_MAIN_THREAD

   BSimString fileName=gModeManager.getModeMenu()->getSelectedFileName();
   if(fileName.isEmpty())
      fileName="art\\ui\\flash\\test.swf";

   BLoadMovieData loadData;
   loadData.mFilename.set(fileName);
   gRenderThread.submitFunctorWithObject(BRenderThread::BFunctor(this, &BModeFlash::workerLoadMovie), loadData);
      
   return BMode::enter(lastMode);
}

//==============================================================================
// BModeFlash::leave
//==============================================================================
void BModeFlash::leave(BMode* newMode)
{
   ASSERT_MAIN_THREAD
   
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BModeFlash::workerUnloadMovie));
   return BMode::leave(newMode);
}

//==============================================================================
// BModeFlash::update
//==============================================================================
void BModeFlash::update()
{
   ASSERT_MAIN_THREAD
   
   BMode::update();

   if(mNextState!=-1)
   {
      mState=mNextState;
      mNextState=-1;

      BHandle fontHandle;
      fontHandle=gFontManager.getFontDenmark24();
      gFontManager.setFont(fontHandle);

      float yh=gFontManager.getLineHeight();

      mList.setFont(fontHandle);
      mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
      mList.setRowSpacing(yh);
      mList.setColors(cColorWhite, cColorCyan);
      mList.setColumnWidth(gUI.mfSafeWidth/3.0f);
      mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-3);
      mList.setMultiColumn(true);
      mList.setJustifyCenter(false);

      mList.clearItems();
      mList.addItem("Start - Exit");
   }
   else
   {
      switch(mState)
      {
         case cStateMain : 
         {
            mList.update();
            break;
         }
         case cStateExit: 
         {
            mNextState=cStateMain;
            gModeManager.getModeMenu()->setNextState(BModeMenu::cStateFlash);
            gModeManager.setMode(BModeManager::cModeMenu);
            break;
         }
      }
   }
}

//==============================================================================
// BModeFlash::renderBegin
//==============================================================================
void BModeFlash::renderBegin()
{
   ASSERT_MAIN_THREAD

   gRender.beginFrame(1.0f/30.0f);
   gRender.beginViewport(-1);
   
   gRenderDraw.beginScene();
   gRenderDraw.clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
}

//==============================================================================
// BModeFlash::render
//==============================================================================
void BModeFlash::render()
{
   // renderBegin called just before this

   ASSERT_MAIN_THREAD
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BModeFlash::workerRenderMovie));

   BHandle fontHandle;
   fontHandle=gFontManager.getFontDenmark24();
   gFontManager.setFont(fontHandle);

   int width = 1024;
   int height = 1024;
   int x = (gUI.mlWidth - width) / 2;
   int y = (gUI.mlHeight - height) / 2;
   x;
   y;
      
   float sx=gUI.mfSafeX1;
   float yh=gFontManager.getLineHeight();
   float by=gUI.mfSafeY2-yh;

   BHandle fontHandle2=gFontManager.getFontDenmark24();
   gFontManager.setFont(fontHandle2);

   mList.render(0, 0);
   
   gFontManager.drawText(gFontManager.getFontDenmark24(), sx, by-yh, mHUDText.getPtr(), cDWORDGreen);

   gFontManager.render2D();

   // renderEnd called just after this.
}

//==============================================================================
// BModeFlash::renderEnd
//==============================================================================
void BModeFlash::renderEnd()
{
   gRenderDraw.endScene();
   gRenderDraw.present(NULL, NULL);
   
   gRender.endViewport();
   gRender.endFrame();
   gEventDispatcher.sleep(16);
}

//==============================================================================
// BModeFlash::handleInput
//==============================================================================
bool BModeFlash::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   ASSERT_MAIN_THREAD
   
   port; detail;

   if(event==cInputEventControlRepeat)
   {
      switch(controlType)
      {
         case cDpadLeft:
         case cDpadRight:
         {
            
            break;
         }
      }         
   }      
   else if(event==cInputEventControlStart)
   {
      switch(controlType)
      {
         case cButtonStart:
         {
            gUI.playClickSound();
            mNextState = cStateExit;
            
            return true;
         }
         case cButtonA:
         {
            gUI.playClickSound();

            if(mState==cStateMain)
            {
            }

            return true;
         }
         case cButtonB:
         {
            gUI.playClickSound();
            
            return true;
         }
      }
   }

   if(mList.handleInput(port, event, controlType, detail))
      return true;

   return false;
}

void BModeFlash::initDeviceData(void)
{
   ASSERT_RENDER_THREAD
   //FxPlayerSettings settings;
   //mRenderMoviePlayer.Init(settings);
}

void BModeFlash::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}

void BModeFlash::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD
}

void BModeFlash::frameEnd(void)
{
   // HACK HACK - Both BMode and BRenderCommandListener define this virtual method!
   //ASSERT_RENDER_THREAD
}

void BModeFlash::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
   //mRenderMoviePlayer.Deinit();
}

bool BModeFlash::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_MAIN_THREAD
   
   switch (event.mEventClass)
   {
      case cECChangeState:
      {
         const BChangeStatePayload* pPayload = static_cast<const BChangeStatePayload*>(event.mpPayload);
         
         mHUDText = pPayload->mHUDText;
         mLoaded = pPayload->mLoaded;
                           
         break;
      }
      case cEventClassReloadNotify:
      {
//-- FIXING PREFIX BUG ID 2407
         const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--

         gConsoleOutput.status("Reloading Flash: %s", pPayload->mPath.getPtr());
         
         BLoadMovieData loadData;
         loadData.mFilename.set(pPayload->mPath.getPtr());
         gRenderThread.submitFunctorWithObject(BRenderThread::BFunctor(this, &BModeFlash::workerLoadMovie), loadData);
         
         break;
      }
   }

   return false;
}

void BModeFlash::workerLoadMovie(void* pData)
{
   ASSERT_RENDER_THREAD
   
   const BLoadMovieData* pLoadData = static_cast<const BLoadMovieData*>(pData);
   
   int index = -1;
   BString filename = pLoadData->mFilename.getPtr();
   gFlashManager.getData(filename, cFlashAssetCategoryCommon, &index);
   if (index == -1)
      return;

   bool success = false;
   mpMovie = gFlashManager.createMovie(index, false);
   if (mpMovie)
      success = true;

   //success = mRenderMoviePlayer.LoadMovie(cDirProduction, pLoadData->mFilename.getPtr());
   
   BString msg;
   BConsoleMessageCategory msgCat = cMsgError;
   if (!success)
   {
      msg.format("Couldn't load movie %s", pLoadData->mFilename.getPtr());
   }
   else
   {
      msg.format("Loaded movie %s", pLoadData->mFilename.getPtr());
      msgCat = cMsgDebug;
   }
   gConsoleOutput.output(msgCat, msg);
   
   BChangeStatePayload* pPayload = new BChangeStatePayload(msg, success);
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cECChangeState, 0, 0, pPayload, BEventDispatcher::cSendSynchronousDispatch);
}

void BModeFlash::workerUnloadMovie(void* pData)
{
   ASSERT_RENDER_THREAD

   gFlashManager.releaseMovie(mpMovie);
   
   //mRenderMoviePlayer.UnloadMovie();
}

void BModeFlash::workerRenderMovie(void* pData)
{
   ASSERT_RENDER_THREAD

   gFlashManager.renderBegin();
   if (mpMovie) 
      mpMovie->render();
   gFlashManager.renderEnd();
}
