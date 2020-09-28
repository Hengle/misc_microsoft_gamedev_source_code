//==============================================================================
// xgamerender.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "xgameRender.h"
#include "configsgamerender.h"
#include "render.h"
#include "renderViewParams.h"
#include "temporalAA.h"
#include "flashallocator.h"
#include "flashbackgroundplayer.h"

// Globals
static XGameRenderInfo gXGameRenderInfo;
static long gXGameRenderRefCount=0;

//==============================================================================
// XGameRenderCreate
//==============================================================================
bool XGameRenderCreate(XGameRenderInfo* info)
{
   if(gXGameRenderRefCount==0)
   {
      InitGFXAllocator();
      
      if(info)
         gXGameRenderInfo=*info;

      TRACEMEM
      
      // Renderer
      gRender.create();
      
      TRACEMEM

      XVIDEO_MODE videoMode;
      ZeroMemory(&videoMode, sizeof(videoMode));
      XGetVideoMode(&videoMode);

      int aspectRatioMode = videoMode.fIsWideScreen ? BRender::cAspectRatioMode16x9 : BRender::cAspectRatioMode4x3;
                  
      trace("Resolution Info :");
      trace("   Resolution  : %d x %d", videoMode.dwDisplayWidth, videoMode.dwDisplayHeight);
      trace("   HiDef       : %s     ", videoMode.fIsHiDef ? "TRUE" : "FALSE");
      trace("   WideScreen  : %s     ", videoMode.fIsWideScreen ? "TRUE" : "FALSE");
      trace("   Interlaced  : %s     ", videoMode.fIsInterlaced ? "TRUE" : "FALSE");      
            
      uint width = 1280;
      uint height = 720;

      // TCR #19 Requirement
      if (gConfig.isDefined(cConfigForce720p) || videoMode.dwDisplayWidth >= 1280 || videoMode.fIsWideScreen)
      {
         width = 1280;
         height = 720;
         aspectRatioMode = BRender::cAspectRatioMode16x9;
      }
      else
      {         
         width = 640;
         height = 480;

         aspectRatioMode = videoMode.fIsWideScreen ? BRender::cAspectRatioMode16x9 : BRender::cAspectRatioMode4x3;

         if (videoMode.dwDisplayWidth > 640  && videoMode.dwDisplayWidth < 1280)
         {
            width = videoMode.dwDisplayWidth;
            height = videoMode.dwDisplayHeight;
         }         
      }
      
      trace("Chosen Resolution Settings");
      trace("Resolution  : %d x %d", width, height);
      trace("Aspect Ratio: %s", (aspectRatioMode == BRender::cAspectRatioMode16x9) ? "16:9" : "4:3");

      //640 x 480 -- 4:3
      //848 x 480 -- 16:9
      //1024 x 768 -- 4:3
      //1280 x 720 -- 16:9
      //1280 x 768
      //1280 x 1024
      //1360 x 768
      //1920 x 1080            

      DWORD presentInterval = D3DPRESENT_INTERVAL_TWO;

      if (gConfig.isDefined(cConfigVSync)) 
      {
         long vsync;
         gConfig.get(cConfigVSync, &vsync);

         if (vsync == 1)
            presentInterval = D3DPRESENT_INTERVAL_ONE;
         else if (vsync == 2)
            presentInterval = D3DPRESENT_INTERVAL_TWO;
         else if (vsync == 3)
            presentInterval = D3DPRESENT_INTERVAL_THREE;
         else
            presentInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
      }

      uint presentImmediateThreshold = 40;
      uint presentImmediateThresholdSD = 15;
      long threshold;
      if (gConfig.get(cConfigPresentImmediateThreshold, &threshold))
         presentImmediateThreshold = (uint) Math::Clamp(threshold, 0L, 100L);
      if (gConfig.get(cConfigPresentImmediateThresholdSD, &threshold))
         presentImmediateThresholdSD = (uint) Math::Clamp(threshold, 0L, 100L);
            
      BD3D::BCreateDeviceParams createDeviceParams;
      createDeviceParams.mBackBufferWidth = width;
      createDeviceParams.mBackBufferHeight = height;
      createDeviceParams.mPresentInterval = presentInterval;
      createDeviceParams.mFrontBufferFormat = D3DFMT_LE_X2R10G10B10;
      createDeviceParams.mBuffer2Frames = true;
      createDeviceParams.mPresentImmediateThreshold = presentImmediateThreshold;
      createDeviceParams.mPresentImmediateThresholdSD = presentImmediateThresholdSD;
      createDeviceParams.mAspectRatioMode = aspectRatioMode;

      TRACEMEM
      
      gRender.start(createDeviceParams, gXGameRenderInfo.mDirStartup, gXGameRenderInfo.mDirArt, gXGameRenderInfo.mDirEffects, gXGameRenderInfo.mDirFonts, gXGameRenderInfo.mDirData);
      
      TRACEMEM
   }

   gXGameRenderRefCount++;

   return true;
}

//==============================================================================
// XGameRenderRelease
//==============================================================================
void XGameRenderRelease()
{
   if(gXGameRenderRefCount==0)
   {
      BASSERT(0);
      return;
   }

   gXGameRenderRefCount--;

   if(gXGameRenderRefCount==0)
   {
      gRender.destroy();
   }
}


//==============================================================================
// ShowDirtyDiskError
//==============================================================================
void ShowDirtyDiskError()
{
   /*
   The 360 keeps it's peeps around in terms of signed in users, NOT controllers
   but the user index DOES NOT correspond to the controller port (ie controller 4 could have signed in first)
   the difficulty here is popping up the dirty disk error on a valid controller.

   The documents don't seem to care if it's controller 0 , or the least most controller etc.
   */
   XINPUT_STATE state;
   XUID xuid;
   for (long UserIndex = 0; UserIndex < XUSER_MAX_COUNT; UserIndex++)
   {
      // check for signed in user
      if (XUserGetXUID(UserIndex, &xuid) == ERROR_SUCCESS)
      {
         if (XInputGetState(UserIndex, &state) != ERROR_DEVICE_NOT_CONNECTED)
         {
            XShowDirtyDiscErrorUI(UserIndex);
         }
      }
   }

   //if we didn't find any active users, WTF?
   XShowDirtyDiscErrorUI(0);

}


//==============================================================================
// DiskReadFailAlert
//==============================================================================
 
void DiskReadFailAlert(void)
{
#ifdef BUILD_FINAL

   // Case 0 : Cold Boot until D3D Initialized
   // if render thread hasn't been initialized yet, then we can just show this immediately
   if(!gRenderThread.getInitialized())
   {
      ShowDirtyDiskError();
      return;
   }

   // Case 1 : Loading screen
   // IF the flash background player is going, then we need to tell it to stop, and just present
   gFlashBackgroundPlayer.setExternalReadError();
   if(gFlashBackgroundPlayer.getIsInMovie())
   {
      ShowDirtyDiskError();
      return;
   }

   
   // Case 2 : everything else
   
   
   if(gEventDispatcher.getThreadIndex() != cThreadIndexRender)
   {   
      //the current thread is not the render thread, so we need to submit a functor to allow our context to exist on the render thread
      //in order to properly juggle the device for showing of this blade function.
      gEventDispatcher.submitFunctor(cThreadIndexRender,  BEventDispatcher::BFunctor(&gRenderThread, &BRenderThread::dirtyDiskSuspendOwnershipDirect));   
      
   }
   else
   {
      //if we are on the render thread, call the juggling directly.
      ASSERT_THREAD(cThreadIndexRender);
      gRenderThread.dirtyDiskSuspendOwnershipDirect();
   }


#else
    BFATAL_FAIL("Archive failed to load");
#endif
}
