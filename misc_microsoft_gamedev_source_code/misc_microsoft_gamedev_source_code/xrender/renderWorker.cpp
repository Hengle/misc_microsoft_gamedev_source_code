// File: renderWorker.cpp
#include "xcore.h"

// xcore
#include "threading\CriticalSection.h"
#include "threading\event.h"
#include "threading\semaphore.h"
#include "math\math.h"
#include "utils\utils.h"
#include "math\generalvector.h"

// local
#include "commandFIFO.h"
#include "renderWorker.h"

#include <xboxmath.h>
#include <process.h>

BRenderWorker gRenderWorker;

BRenderWorker::BRenderWorker() :
   mpD3D(NULL),
   mpD3DDev(NULL),
   mThreadHandle(INVALID_HANDLE_VALUE),
   mpTriangleRenderer(NULL)
{
   Utils::ClearObj(mD3DPP);
}

BRenderWorker::~BRenderWorker()
{
   delete mpTriangleRenderer;
}

bool BRenderWorker::init(void)
{
   if (INVALID_HANDLE_VALUE == mThreadHandle)
   {
      uintptr_t threadHandle = _beginthread(workerThreadFunc, 0, this);

      if (-1L == threadHandle)
         BFAIL("_beginthread failed");
      
      mThreadHandle = reinterpret_cast<HANDLE>(threadHandle);
   }

   return true;
}

bool BRenderWorker::deinit(void)
{
   if (INVALID_HANDLE_VALUE != mThreadHandle)
   {

   }

   return true;
}

void BRenderWorker::submitCommand(DWORD commandClass, DWORD commandType, DWORD commandLen, const void* pCommandData)
{
   const DWORD cmdSize = Utils::RoundUp(sizeof(BCommandHeader) + commandLen, sizeof(DWORD));

   if (!mpCmdBufSegment)
   {
      mpCmdBufSegment = mCmdBuf.getBackPtr()->mData;
      BASSERT(mpCmdBufSegment);
      mCmdBufSegmentOfs = sizeof(DWORD);
   }

   DWORD cmdBufLeft = SegmentSize - mCmdBufSegmentOfs;

   if (cmdSize > cmdBufLeft)
   {
      kickCommands();

      mpCmdBufSegment = mCmdBuf.getBackPtr()->mData;
      BASSERT(mpCmdBufSegment);
      mCmdBufSegmentOfs = sizeof(DWORD);

      cmdBufLeft = SegmentSize - mCmdBufSegmentOfs;
   }

   BASSERT(cmdSize <= cmdBufLeft);

   BCommandHeader* pHeader = reinterpret_cast<BCommandHeader*>(mpCmdBufSegment + mCmdBufSegmentOfs);
   pHeader->mMagic = static_cast<WORD>(BCommandHeader::cCommandHeaderMagic);
   pHeader->mClass = static_cast<WORD>(commandClass);
   pHeader->mType = static_cast<WORD>(commandType);
   pHeader->mLen = static_cast<WORD>(commandLen);

   if (commandLen)
   {
      BASSERT(pCommandData);

      memcpy(mpCmdBufSegment + mCmdBufSegmentOfs + sizeof(BCommandHeader), pCommandData, commandLen);
   }

   mCmdBufSegmentOfs += cmdSize;
}

void BRenderWorker::kickCommands(void)
{
   if (!mpCmdBufSegment)
   {
      mpCmdBufSegment = mCmdBuf.getBackPtr()->mData;
      BASSERT(mpCmdBufSegment);
      mCmdBufSegmentOfs = sizeof(DWORD);
   }

   *reinterpret_cast<DWORD*>(mpCmdBufSegment) = mCmdBufSegmentOfs;

   mCmdBuf.pushBack();

   mpCmdBufSegment = NULL;
   mCmdBufSegmentOfs = 0;
}


void BRenderWorker::workerThreadFunc(void* pData)
{
   BRenderWorker* pRenderer = reinterpret_cast<BRenderWorker*>(pData);
   pRenderer->workerThread();
}

void BRenderWorker::panic(const char* pMsg, ...)
{
   BFAIL("Renderer panicked");
}

bool BRenderWorker::workerThread(void)
{
   if (!initD3D())
      panic("initD3D failed");

   if (!initData())
      panic("initData failed");

   if (!commandLoop())
      panic("commandLoop failed");

   if (!deinitData())
      panic("deinitData failed");

   return true;
}

bool BRenderWorker::initD3D(void)
{
   // Create the D3D object.
   mpD3D = Direct3DCreate9( D3D_SDK_VERSION );

   // Set up the structure used to create the D3DDevice.
   XVIDEO_MODE VideoMode;
   ZeroMemory( &VideoMode, sizeof(VideoMode) );
   XGetVideoMode( &VideoMode );
   BOOL bEnable720p = (VideoMode.dwDisplayHeight >= 720) ? TRUE : FALSE;

   // Set up the structure used to create the D3DDevice.

   ZeroMemory( &mD3DPP, sizeof(mD3DPP) );
   mD3DPP.BackBufferWidth        = bEnable720p ? 1280 : 640;
   mD3DPP.BackBufferHeight       = bEnable720p ? 720  : 480;
   mD3DPP.BackBufferFormat       = D3DFMT_A8R8G8B8;
   mD3DPP.MultiSampleType        = D3DMULTISAMPLE_NONE;
   mD3DPP.MultiSampleQuality     = 0;
   mD3DPP.BackBufferCount        = 1;
   mD3DPP.EnableAutoDepthStencil = TRUE;
   mD3DPP.AutoDepthStencilFormat = D3DFMT_D24S8;
   mD3DPP.SwapEffect             = D3DSWAPEFFECT_DISCARD;
   mD3DPP.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

   // Create the Direct3D device.
   mpD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &mD3DPP, &mpD3DDev);

   return true;
}      

bool BRenderWorker::initData(void)
{
   mpTriangleRenderer = new BTriangleRenderer();

   if (!mpTriangleRenderer->init())
      return false;
       
   return true;
}

bool BRenderWorker::deinitData(void)
{
   if (!mpTriangleRenderer->deinit())
      return false;
      
   return true;
}

bool BRenderWorker::commandLoop(void)
{
   for ( ; ; )
   {
      if (mTerminate)
         return true;

      const uchar* pSegment = mCmdBuf.getFrontPtr()->mData;

      DWORD segmentTotalLen = *reinterpret_cast<const DWORD*>(pSegment);
      BASSERT(segmentTotalLen >= sizeof(DWORD));

      DWORD segmentOfs = sizeof(DWORD);
      while (segmentOfs < segmentTotalLen)
      {
         const BCommandHeader* pCmdHeader = reinterpret_cast<const BCommandHeader*>(pSegment + segmentOfs);
         BASSERT(pCmdHeader->mMagic == BCommandHeader::cCommandHeaderMagic);

         processCommand(*pCmdHeader, pSegment + segmentOfs + sizeof(BCommandHeader));

         if (mTerminate)
            return true;

         DWORD cmdLen = Utils::RoundUp(sizeof(BCommandHeader) + pCmdHeader->mLen, 4);
         segmentOfs += cmdLen;
      }

      mCmdBuf.popFront();      
   }

   return true;
}

void BRenderWorker::processCommand(const BCommandHeader& header, const uchar* pData)
{
   switch (header.mClass)
   {
      case cRCCControl:
      {
         processControlCommand(header, pData);
         break;
      }
      case cRCCRender:
      {
         processRenderCommand(header, pData);
         break;
      }
      default:
         BASSERT(false);
   }  
}

void BRenderWorker::processControlCommand(const BCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cRCTBeginScene:
      {
         mpD3DDev->BeginScene();
         break;
      }            
      case cRCTClear:
      {
         mpD3DDev->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, D3DCOLOR_ARGB(255,20,20,20), 1.0f, 0);
         break;
      }            
      case cRCTEndScene:
      {
         mpD3DDev->EndScene();
         break;
      }            
      case cRCTPresent:
      {
         mpD3DDev->Present(NULL, NULL, NULL, NULL);
         break;
      }            
      case cRCTExit:
      {
         mTerminate = true;
         break;
      }            
   }
}

void BRenderWorker::processRenderCommand(const BCommandHeader& header, const uchar* pData)
{
   mpTriangleRenderer->render(header.mType, pData);
}

