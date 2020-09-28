//============================================================================
//
//  renderControl.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"

#if 0
#include "renderControl.h"
#include "renderThread.h"
#include "BD3D.h"


BRenderControl gRenderControl;

BRenderControl::BRenderControl() :
   mCurFence(0),
   mCurWorkerFrame(0)
{
}

bool BRenderControl::init(void)
{
   ASSERT_WORKER_THREAD
   

   
   return true;
}

bool BRenderControl::deinit(void)
{
   ASSERT_WORKER_THREAD
   return true;
}

void BRenderControl::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_WORKER_THREAD
   switch (header.mType)
   {
      case cRCFrameBegin:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(DWORD));
         InterlockedExchange((LONG*)&mCurWorkerFrame, *reinterpret_cast<const DWORD*>(pData));
         mCurWorkerFrameChanged.set();
         
         PIXSetMarker(D3DCOLOR_ARGB(255,255,255,255), "CurWorkerFrame: %i", mCurWorkerFrame);
         
         for (uint i = 0; i < gRenderThread.getMaxCommandListeners(); i++)
         {
            BRenderCommandListenerInterface* pListener = gRenderThread.getCommandListenerByIndex(i);
            if (pListener)
               pListener->frameBegin();
         }

         break;
      }
             
      case cRCKickPushBuffer:
      {
         BD3D::mpDev->InsertFence();
         break;
      }
      
      case cRCExit:
      {
         gRenderThread.queueTermination();
         break;
      }
      case cRCFence:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         InterlockedExchange((LONG*)&mCurFence, *reinterpret_cast<const LONG*>(pData));
         mFenceChangedEvent.set();
         break;
      }
      case cRCGPUFence:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderGPUFence*));
         BRenderGPUFence* pGPUFence = *reinterpret_cast<BRenderGPUFence* const*>(pData);
                  
         PIXSetMarker(D3DCOLOR_ARGB(255,0,255,128), "BRenderControlInsertFence");
                           
         uint index = BD3D::mpDev->InsertFence();
         while (index >= (UINT_MAX - 1))
            index = BD3D::mpDev->InsertFence();
     
         InterlockedExchange((LONG*)&pGPUFence->mIndex, index);
         mGPUFenceSubmittedEvent.set();
               
         break;
      }
      case cRCFrameEnd:
      {
         for (uint i = 0; i < gRenderThread.getMaxCommandListeners(); i++)
         {
            BRenderCommandListenerInterface* pListener = gRenderThread.getCommandListenerByIndex(i);
            if (pListener)
               pListener->frameEnd();
         }
            
         break;
      }      
      default:
      {
         BVERIFY(false);
         break;
      }
   }
}
#endif
