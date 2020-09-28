//============================================================================
// miniLoadManager.cpp
// 
// Ensemble Studios (C) 2007
//============================================================================
#include "xgameRender.h"
#include "miniLoadManager.h"
#include "BD3D.h"
#include "gfxMoviePlayer.h"
#include "renderThread.h"

#define DRAW_TIMED_MILLISECONDS 30

//============================================================================
//============================================================================

BCriticalSection MiniLoadManager::mShouldContinueLock(10000);

//============================================================================
//============================================================================
MiniLoadManager::MiniLoadManager() :
   mThreadHandle(NULL),
   mShouldContinue(true),
   mpMoviePlayerMiniLoad(NULL),
   mpMovieManagerMiniLoad(NULL)
{

}

//============================================================================
//============================================================================
   
void MiniLoadManager::init()
{
   if (!mpMoviePlayerMiniLoad)
   {
      // release from render thread to this thread (the main thread)
      gRenderThread.releaseThreadOwnership();

      // Create our own movie manager
      mpMovieManagerMiniLoad = new BGFxMovieManager();
      FxPlayerSettings settings;     
      mpMovieManagerMiniLoad->init(settings, "", cDirProduction, "", "");

      // Create the flash movie player object
      mpMoviePlayerMiniLoad  = new BGFXMoviePlayer();
      mpMoviePlayerMiniLoad->Init(mpMovieManagerMiniLoad);

      // Load the flash movie
      bool success = mpMoviePlayerMiniLoad->LoadMovie(cDirProduction, "art\\ui\\flash\\load\\MiniLoad\\MiniLoad.swf");
      if (!success)
      {
         trace("Failed loading flash movie art\\ui\\flash\\load\\MiniLoad\\MiniLoad.swf");
         gConsoleOutput.error("Failed loading flash movie art\\ui\\flash\\load\\MiniLoad\\MiniLoad.swf");
      }

      // Give ownership from this thread back to the render thread
      gRenderThread.acquireThreadOwnership();
   }
}

//============================================================================
//============================================================================

void MiniLoadManager::deinit()
{
   // release from render thread to this thread (the main thread)
   gRenderThread.releaseThreadOwnership();

   // Free the movie player and movie manager
   if (mpMoviePlayerMiniLoad)
   {
      mpMoviePlayerMiniLoad->UnloadMovie();
      mpMoviePlayerMiniLoad->Deinit();
      delete mpMoviePlayerMiniLoad;
      mpMoviePlayerMiniLoad = NULL;
   }

   if (mpMovieManagerMiniLoad)
   {
      mpMovieManagerMiniLoad->deinit();
      delete mpMovieManagerMiniLoad;
      mpMovieManagerMiniLoad = NULL;
   }

   // Give ownership from this thread back to the render thread
   gRenderThread.acquireThreadOwnership();
}

//============================================================================
//============================================================================

void MiniLoadManager::start()
{
   // initialize the flash objects
   //init();

   // start the thread that contains the render loop
   mThreadHandle = CreateThread(NULL, 0, &MiniLoadManagerThreadCallback, NULL, 0, NULL);

   // use core 2 hw thread 0
   XSetThreadProcessor(mThreadHandle, 4);
}

//============================================================================
//============================================================================

void MiniLoadManager::stop()
{
   // Tell the render loop thread that we created that it needs to stop
   setShouldContinue(false);

   while (true)
   {
      // Loop untill the thread has exited
      DWORD exitCode = STILL_ACTIVE;
      GetExitCodeThread(mThreadHandle, &exitCode);
      if (exitCode != STILL_ACTIVE)
      {
         // We need to aquire the device on this thread so we can draw
         BD3D::mpDev->AcquireThreadOwnership();

         // Once we have exited reset our variables and deinit our flash objects
         mThreadHandle = NULL;
         setShouldContinue(true);
         //deinit();

         // Release the device and then exit
         BD3D::mpDev->ReleaseThreadOwnership();

         return;
      }
   }
}

//============================================================================
//============================================================================

void MiniLoadManager::setShouldContinue(bool shouldCont)
{
   BScopedCriticalSection lock(mShouldContinueLock);
   mShouldContinue = shouldCont;
}

//============================================================================
//============================================================================

bool MiniLoadManager::getShouldContinue()
{
   BScopedCriticalSection lock(mShouldContinueLock);
   return mShouldContinue;
}

//============================================================================
//============================================================================

void MiniLoadManager::draw()
{
   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_ALLTARGETS, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
   if (mpMoviePlayerMiniLoad)
   {
      mpMoviePlayerMiniLoad->Tick();
   }
   BD3D::mpDev->Present(0, 0, 0, 0);
}

//============================================================================
//============================================================================

void MiniLoadManager::drawTimed()
{
   // Call draw only if DRAW_TIMED_MILLISECONDS have elapsed since the last call to draw
   static DWORD timeOfLastPresent = 0;
   if ((GetTickCount() - timeOfLastPresent) > DRAW_TIMED_MILLISECONDS)
   {
      draw();
      timeOfLastPresent = GetTickCount();
   }
}

//============================================================================
//============================================================================

MiniLoadManager gMiniLoadManager;

//============================================================================
//============================================================================

DWORD __stdcall MiniLoadManagerThreadCallback(LPVOID lpThreadParameter)
{
   // We need to aquire the device on this thread so we can draw
   BD3D::mpDev->AcquireThreadOwnership();

   // Loop until we are told to quit
   while (gMiniLoadManager.getShouldContinue())
   {
      gMiniLoadManager.draw();
   }

   // Release the device and then exit
   BD3D::mpDev->ReleaseThreadOwnership();

   return 99;
}

