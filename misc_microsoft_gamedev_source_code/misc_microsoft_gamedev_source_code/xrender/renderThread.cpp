//============================================================================
//
//  renderThread.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//  rg [1/3/06] - Initial implementation.
//
//============================================================================
#include "xrender.h"

// xcore
#include "math\generalvector.h"
#include "memory\alignedAlloc.h"
#include "threading\setThreadName.h"
#include "threading\workDistributor.h"

// local
#include "renderThread.h"
#include "BD3D.h"
#include "deviceStateDumper.h"
#include "threading\eventDispatcher.h"

#include "..\xGameRender\miniLoadManager.h"

const uint cDefaultGPUFrameStorageSize = (uint)(4.0f * 1024U * 1024U);

//#define TEST_FRAME_STORAGE

#ifndef BUILD_FINAL   
   #define UPDATE_STATS(x, y) mStats.##x += (y);
#else
   #define UPDATE_STATS(x, y)   
#endif

#pragma warning(disable:4509) // warning C4509: nonstandard extension used: 'BRenderThread::workerThreadFunc' uses SEH and 'PIXNamedEvent' has destructor

BRenderThread gRenderThread;

namespace
{
   struct BRenderGPUFenceCommandData
   {
      BRenderGPUFence* mpFence;
      uint mIssueCount;
   };
} // anonymous namespace   

//============================================================================
// BRenderGPUFence::BRenderGPUFence
//============================================================================
BRenderGPUFence::BRenderGPUFence() : 
   mIndex(UINT64_MAX),
   mIssueCount(0)
{
}

//============================================================================
// BRenderGPUFence::~BRenderGPUFence
//============================================================================
BRenderGPUFence::~BRenderGPUFence()
{
   // We can't go out of scope until the fence is triggered, because the worker thread will be modifying this instance's data directly.
   // It shouldn't matter that this method could take a long time, because we shouldn't be freeing GPU fences often.
   blockUntilSubmitted();
}

//============================================================================
// BRenderGPUFence::issue
//============================================================================
void BRenderGPUFence::issue(void) 
{
   mIssueCount++;
   
   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
   {
      // Place a copy of the issue count into the command buffer.
      // This will allow us to tell (from the main thread) if the index is due to the most recent issue.
      // The index and index issue could will be in the same 64-bit value, so it will be written atomically by the worker thread.
      BRenderGPUFenceCommandData* pData = (BRenderGPUFenceCommandData*)gRenderThread.submitCommandBegin(cRCCControl, cRCGPUFence, sizeof(BRenderGPUFenceCommandData));
      pData->mpFence = this;
      pData->mIssueCount = mIssueCount;
      gRenderThread.submitCommandEnd(sizeof(BRenderGPUFenceCommandData));
   }
   else
   {
      ASSERT_RENDER_THREAD
      
      // We're within the worker thread, so instantly submit the fence.
      mIndex = BD3D::mpDev->InsertFence() | (static_cast<uint64>(mIssueCount) << 32);
   }      
}

//============================================================================
// BRenderGPUFence::getIssued
//============================================================================
bool BRenderGPUFence::getIssued(void) const
{ 
   ASSERT_MAIN_OR_WORKER_THREAD
   
   return mIssueCount != 0;
}

//============================================================================
// BRenderGPUFence::getSubmitted
//============================================================================
bool BRenderGPUFence::getSubmitted(void) const 
{ 
   ASSERT_MAIN_OR_WORKER_THREAD

   MemoryBarrier();   
   
   // Check the index issue count.
   return mIssueCount == (mIndex >> 32);
}

//============================================================================
// BRenderGPUFence::getPending
//============================================================================
bool BRenderGPUFence::getPending(void) const
{
   ASSERT_MAIN_OR_WORKER_THREAD
   
   if (!getIssued())
      return false;
            
   if (!getSubmitted())
      return true;
         
   BDEBUG_ASSERT(mIndex != UINT64_MAX);
         
   // This is OK to call from any thread.
   return BD3D::mpDev->IsFencePending((uint)mIndex) != FALSE;
}

//============================================================================
// BRenderGPUFence::blockUntilSubmitted
//============================================================================
void BRenderGPUFence::blockUntilSubmitted(void) const
{
   ASSERT_MAIN_OR_WORKER_THREAD

   if (!getIssued())
      return;
      
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,127,0), "GPUFenceBlockUntilSubmitted");   
   
   // Don't bother kicking unless the fence command hasn't been submitted yet (wasteful).
   if (!getSubmitted())
   {
      // Worker thread fences are always immediately submitted. We can only wait for submission in the main thread.
      ASSERT_MAIN_THREAD
      
      gRenderThread.waitUntilD3DFenceSubmitted(*this);
   }
   else
   {
      // Import barrier.
      MemoryBarrier();
   }

   PIXEndNamedEvent();
}

//============================================================================
// BRenderGPUFence::blockWhilePending
//============================================================================
void BRenderGPUFence::blockWhilePending(void) const
{
   ASSERT_MAIN_OR_WORKER_THREAD
   
   if (!getIssued())
      return;

   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,127,0), "GPUFenceBlockWhilePending");            
      
   blockUntilSubmitted();
            
   BDEBUG_ASSERT(mIndex != UINT64_MAX);
   
   // This is safe to call from the main thread.
   BD3D::mpDev->BlockOnFence((uint)mIndex);
   
   PIXEndNamedEvent();
}

//============================================================================
// BRenderThread::waitUntilD3DFenceSubmitted
//============================================================================
void BRenderThread::waitUntilD3DFenceSubmitted(const BRenderGPUFence& fence)
{
   //ASSERT_MAIN_THREAD

   kickCommands();

   do
   {
      const int status = gEventDispatcher.waitSingle(mGPUFenceSubmittedEvent.getHandle());
      status;
      BDEBUG_ASSERT(status == 0);
      
   } while (!fence.getSubmitted());
}

//============================================================================
// BRenderThread::BRenderThread
//============================================================================
BRenderThread::BRenderThread() :
   mInitialized(false),
   mThreadHandle(INVALID_HANDLE_VALUE),
   mpCmdFIFOSegment(NULL),
   mCmdFIFOSegmentOfs(0),
   mTerminate(FALSE),
   mShuttingDown(FALSE),
   mProcessingCommandsEvent(true),
   mD3DReadyEvent(true),
   mD3DReadyFlag(FALSE),
   mProcessingCommandsFlag(FALSE),
   mCurMainFrame(0),
   mFramesRemaining(cMaxQueuedFrames, cMaxQueuedFrames),
   mFrameStorageCurMainIndex(0),
   mFrameStorageCurWorkerIndex(0),
   mMainThreadId(0),
   mWorkerThreadId(0),
   mInFrame(false),
   mPanicMode(false),
   mNextWorkerFence(0),
   mCurWorkerFence(0),
   mCurWorkerFrame(0),
   mpGPUFrameStorageFences(NULL),
   mMainEventReceiverHandle(cInvalidEventReceiverHandle),
   mWorkerEventReceiverHandle(cInvalidEventReceiverHandle),
   mSingleStep(false),
   mHasD3DOwnership(true),
   mInMiniLoadRenderingMode(false),
   mLastPresentFence(UINT64_MAX),
   mInsideLevelLoad(false),
   mNumFramesRemaining(cMaxQueuedFrames)
{
#ifdef BUILD_DEBUG
   mSubmittingCommand = false;
#endif   

   Utils::ClearObj(mFrameStorageFences);
   
   for (uint i = 0; i < cMaxQueuedGPUFrames; i++)
      mGPUFrameStorageCurOfs[i] = 0;
   
   for (uint i = 0; i < cMaxCommandListeners; i++)
      mpCommandListeners[i] = NULL;

#ifndef BUILD_FINAL
   clearStats();
   
   mFrameStartTime.QuadPart = 0;
   mMinFrameTime = 0;
#endif      
}

//============================================================================
// BRenderThread::~BRenderThread
//============================================================================
BRenderThread::~BRenderThread()
{
   deinit();
}

#ifndef BUILD_FINAL
//============================================================================
// BRenderThread::clearStats
//============================================================================
void BRenderThread::clearStats(void)
{
   Utils::ClearObj(mStats);
}

//============================================================================
// BRenderThread::getStats
//============================================================================
void BRenderThread::getStats(BStats& stats)
{
   ASSERT_MAIN_THREAD;
   stats = mStats;
   clearStats();
}
#endif

#if 0
//============================================================================
// BRenderThread::beginLoad
//============================================================================
void BRenderThread::beginLoad()
{
   if (!mInitialized)
      return;
   
   gRenderThread.blockUntilGPUIdle();
   
   deinitAllFrameStorage();
}

//============================================================================
// BRenderThread::endLoad
//============================================================================
void BRenderThread::endLoad()
{
   if (!mInitialized)
      return;
      
   initAllFrameStorage();
}
#endif

//============================================================================
// BRenderThread::init
//============================================================================
bool BRenderThread::init(const BD3D::BCreateDeviceParams& initParams, BD3DDeviceInfo* pDeviceInfo, bool panicMode)
{
   if (mInitialized)
      return false;
         
   mInitialized = true;   
   mPanicMode = panicMode;   
      
   mMainThreadId = GetCurrentThreadId();
   mWorkerThreadId = 0;
      
   mMainEventReceiverHandle = gEventDispatcher.addClient(this, cThreadIndexSim);
   mWorkerEventReceiverHandle = gEventDispatcher.addClient(this, cThreadIndexRender);
      
   mpCmdFIFOSegment = NULL;
   mCmdFIFOSegmentOfs = 0;
   
   mTerminate = FALSE;
   mShuttingDown = FALSE;
   mD3DReadyFlag = FALSE;
   mProcessingCommandsFlag = FALSE;
   
   mCurMainFrame = 0;
   mCurWorkerFrame = 0;
   mCurGPUFrame = 0;
   
   mNextWorkerFence = 0;
   mCurWorkerFence = 0;
      
   mCurGPUMainFenceIndex = 0;
   mCurGPUWorkerFenceIndex = 0;
   mNextGPUMainFenceIndex = 0;
   mNextGPUWorkerFenceIndex = 0;
   
#ifdef BUILD_DEBUG
   mSubmittingCommand = false;
#endif   
      
   for (uint i = 0; i < cMaxCommandListeners; i++)
      mpCommandListeners[i] = NULL;
         
   if (!initAllFrameStorage())
      panic("initAllFrameStorage failed");
      
   mD3DInitParams = initParams;
   
#ifndef BUILD_FINAL
   clearStats();
#endif      
         
   // FIXME: change this to beginthreadex
   mThreadHandle = gEventDispatcher.createThread(
      cThreadIndexRender, 
      "RenderThread", 
      // FIXME: Reduce this for final builds!!!!
      16384,//BEventDispatcher::cDefaultEventQueueSize,
      1024,
      NULL, 
      65536, 
      workerThreadFunc, 
      this,
      0, 
      NULL,
      2);

   if (NULL == mThreadHandle)
   {
      BFATAL_FAIL("_beginthread failed");
   }
         
   blockUntilD3DReady();
   
   if (pDeviceInfo)   
   {
      // The data in BD3D should be safe to access from the main thread here, because the Win32 sync API calls in both threads execute memory barriers.
      pDeviceInfo->mpDev = BD3D::mpDev;
      BDEBUG_ASSERT(pDeviceInfo->mpDev);
      pDeviceInfo->mD3DPP = BD3D::mD3DPP;
      pDeviceInfo->mpDevBackBuffer = BD3D::mpDevBackBuffer;
      pDeviceInfo->mpDevFrontBuffer = BD3D::mpDevFrontBuffer;
      pDeviceInfo->mpDevDepthStencil = BD3D::mpDevDepthStencil;
   }

   return true;
}

//============================================================================
// BRenderThread::deinit
//============================================================================
bool BRenderThread::deinit(void)
{
   if ((!mInitialized) || (mPanicMode))
      return false;
      
   ASSERT_MAIN_THREAD
               
   if (INVALID_HANDLE_VALUE != mThreadHandle)
   {
      setSingleStep(false);
      
      // Prevent any more events from being queued up by the render or helper threads.
      // Some events can/will still sneak though.
      Sync::InterlockedExchangeExport(&mShuttingDown, TRUE);
                  
      // Big old hammer here. Let things settle down.
      for (uint i = 0; i < 10; i++)
      {
         kickCommands();
         
         gEventDispatcher.pumpAllThreads(33, 4);
      }
      
      blockUntilGPUIdle();
                                                
      // Submit an exit command. This should cause the worker thread to cleanly die.
      submitCommand(cRCCControl, cRCExit);
      
      // Now process any remaining commands, and the exit command.
      kickCommands();
                                   
      // If for some reason the command buffer isn't being emptied normally, the wait will timeout. 
      // We'll then try setting the terminate flag and wait a bit more.
      if (-1 == gEventDispatcher.pumpAllThreads(8000, 8, mThreadHandle))
      {
         queueTermination();
         
         if (WAIT_TIMEOUT == gEventDispatcher.pumpAllThreads(8000, 8, mThreadHandle))
         {
            // At this point we're screwed - the worker thread is apparently locked up.
            // Don't BFAIL because the delay could have been caused by somebody debugging.
            trace("BRenderThread::deinit: Couldn't terminate worker thread.");
         }
      }
      
      CloseHandle(mThreadHandle);
      mThreadHandle = INVALID_HANDLE_VALUE;
      
      if (mMainEventReceiverHandle != cInvalidEventReceiverHandle)
      {
         gEventDispatcher.removeClientImmediate(mMainEventReceiverHandle); 
         mMainEventReceiverHandle = cInvalidEventReceiverHandle;
      }
           
      if (!deinitAllFrameStorage())
         panic("initAllFrameStorage failed");
   }
   
   mInitialized = false;

   return true;
}

namespace 
{
   //============================================================================
   // class BReleaseThreadOwnershipRCO
   //============================================================================
   class BReleaseThreadOwnershipRCO : public BRenderCommandObjectInterface
   {
   public:
      virtual void processCommand(DWORD data) const
      {
         data;
         BD3D::mpDev->ReleaseThreadOwnership();
      }
   };
   
   //============================================================================
   // class BAcquireThreadOwnershipRCO
   //============================================================================
   class BAcquireThreadOwnershipRCO : public BRenderCommandObjectInterface
   {
   public:
      virtual void processCommand(DWORD data) const
      {
         data;
         BD3D::mpDev->AcquireThreadOwnership();
      }
   };

} // anonymous namespace

//============================================================================
// BRenderThread::releaseThreadOwnership
// Releases the worker thread's ownership of the D3D device.
// This is slow.
//============================================================================
IDirect3DDevice9* BRenderThread::releaseThreadOwnership(void)
{
   ASSERT_MAIN_THREAD
     
   submitCopyOfCommandObject(BReleaseThreadOwnershipRCO());
   
   blockUntilWorkerIdle();
   
   BD3D::mpDev->AcquireThreadOwnership();

   mHasD3DOwnership = false;
         
   return BD3D::mpDev;
}

//============================================================================
// BRenderThread::acquireThreadOwnership
// Causes the worker thread to acquire the D3D device.
//============================================================================
void BRenderThread::acquireThreadOwnership(void)
{
   ASSERT_MAIN_THREAD
  
   BD3D::mpDev->ReleaseThreadOwnership();
   
   submitCopyOfCommandObject(BAcquireThreadOwnershipRCO());
         
   blockUntilWorkerIdle();

   mHasD3DOwnership = true;
   
   mLastPresentFence = UINT64_MAX;
}

//============================================================================
// BRenderThread::dirtyDiskSuspendOwnershipDirect
//============================================================================
void ShowDirtyDiskError(void);
void BRenderThread::dirtyDiskSuspendOwnershipDirect(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   //TODO : this is quite scary.. there's a couple 'what???' methods that we can fall into while we're tossing the device around
   //       What if no one owns the device?
   //       What if someone else intentionally owns the device, and we don't?
   //       What if someone acquires the device immediately before we call this function. (or IN this function??)

   DWORD myID = gEventDispatcher.getThreadId(gEventDispatcher.getThreadIndex());

   if (BD3D::mpDev->QueryThreadOwnership() != myID)
   {
      //if this thread doesn't own the device, try and aquire / suspend it
      // if that doesn't work, then just call the dirty disk error, as someone else
      // should be spinlooping PRESENT for us.
      try
      { 
         //I'm going to attempt to acqurie thread ownwership of D3D
         // in the case where NO ONE owns it, we will aquire it.
         // in the case where I own it, this should be OK.
         // in the case where someone else owns it, this will fail and go to the catch

         
         BD3D::mpDev->AcquireThreadOwnership();
         BD3D::mpDev->Suspend();
      }
      catch( ... )
      {
         //in the case that the acuqire fails, that means someone else owns the device
         //in which case we have to assume that htey are properly pumping out PRESENT calls
         //so we continue forward showing the dirty disk error and it will be displayed properly.
      }
   }
   else
   {
      //I own the device, and I'm on the render thread
      // so suspend it
      BD3D::mpDev->Suspend();
   }


   ShowDirtyDiskError();
}


//============================================================================
// BRenderThread::startMiniLoadRendering
// Puts the render thread into mini load screen rendering mode.  This means the
// render thread will automatically call present from within the main command loop.
//============================================================================
void BRenderThread::startMiniLoadRendering(void)
{
   ASSERT_MAIN_THREAD
   
   submitCommand(cRCCControl, cRCStartMiniLoad);
   
   blockUntilWorkerIdle();
   
   mHasD3DOwnership = false;
}

//============================================================================
// BRenderThread::stopMiniLoadRendering
// Stops the render thread mini load screen rendering mode.  This means the
// render thread will no longer automatically call present  from within the 
// main command loop.
//============================================================================
void BRenderThread::stopMiniLoadRendering(void)
{
   ASSERT_MAIN_THREAD
  
   submitCommand(cRCCControl, cRCStopMiniLoad);

   blockUntilWorkerIdle();

   mHasD3DOwnership = true;
   
   mLastPresentFence = UINT64_MAX;
}

//============================================================================
// BRenderThread::blockUntilD3DReady
//============================================================================
void BRenderThread::blockUntilD3DReady(void)
{
   ASSERT_MAIN_THREAD
         
   const int status = gEventDispatcher.pumpThreadAndWait(cThreadIndexRender, INFINITE, 8, mD3DReadyEvent.getHandle());
   status;
   BDEBUG_ASSERT(status == 0);
}

//============================================================================
// BRenderThread::blockUntilProcessingCommands
//============================================================================
void BRenderThread::blockUntilProcessingCommands(void)
{
   ASSERT_MAIN_THREAD
   
   const int status = gEventDispatcher.pumpThreadAndWait(cThreadIndexRender, INFINITE, 8, mProcessingCommandsEvent.getHandle());
   status;
   BDEBUG_ASSERT(status == 0);
}

//============================================================================
// BRenderThread::getFreeMemory
//============================================================================
uint BRenderThread::getFreeMemory(void)
{
#ifdef BUILD_FINAL
   return 0;
#else
   MEMORYSTATUS stat;
   GlobalMemoryStatus(&stat);   
   return stat.dwAvailPhys / (1024U * 1024U);
#endif   
}

//============================================================================
// BRenderThread::beginLevelLoad
//============================================================================
void BRenderThread::beginLevelLoad(void)
{
   ASSERT_MAIN_THREAD
      
   trace("BRenderThread::beginLevelLoad: start: %uMB free", getFreeMemory());
   
   blockUntilGPUIdle();
   
   BDEBUG_ASSERT(!mInsideLevelLoad);
   mInsideLevelLoad = true;
         
   submitCommand(cRCCControl, cRCBeginLevelLoad);
   
   blockUntilWorkerIdle();
   
   trace("BRenderThread::beginLevelLoad: end: %uMB free", getFreeMemory());
}

//============================================================================
// BRenderThread::endLevelLoad
//============================================================================
void BRenderThread::endLevelLoad(void)
{
   ASSERT_MAIN_THREAD
   
   BDEBUG_ASSERT(mInsideLevelLoad);
         
   trace("BRenderThread::endLevelLoad: start: %u MB free", getFreeMemory());
   
   blockUntilGPUIdle();
         
   submitCommand(cRCCControl, cRCEndLevelLoad);
   
   blockUntilWorkerIdle();
      
   mInsideLevelLoad = false;
   
   trace("BRenderThread::endLevelLoad: end: %uMB free", getFreeMemory());
}

//============================================================================
// BRenderThread::submitCommand
//============================================================================
void BRenderThread::submitCommand(DWORD commandClass, DWORD commandType, DWORD dataLen, const void* pData)
{
   ASSERT_MAIN_THREAD
   
   void* pDst = submitCommandBegin(commandClass, commandType, dataLen);
      
   if (dataLen)
   {
      BASSERT(pData);

      XMemCpy(pDst, pData, dataLen);
   }
   
   submitCommandEnd(dataLen);
}

//============================================================================
// BRenderThread::beginProducerSegment
//============================================================================
void BRenderThread::beginProducerSegment(void)
{
   // ASSERT_MAIN_THREAD
   
   UPDATE_STATS(mSegmentsOpened, 1);
      
   const int status = gEventDispatcher.waitSingle(mCmdFIFO.getBackSemaphore().getHandle());
   status;
   BDEBUG_ASSERT(status == 0);
      
   BSegment* pSegment = mCmdFIFO.acquireBackPtr();
   
   BDEBUG_ASSERT(Utils::IsAligned(pSegment, 128));
   
   for (uint i = 0; i < cSegmentSize; i += 128)
      __dcbz128(i, pSegment);
   
   mpCmdFIFOSegment = pSegment->mData;
   
   BDEBUG_ASSERT(mpCmdFIFOSegment);
   mCmdFIFOSegmentOfs = sizeof(DWORD);
}

//============================================================================
// BRenderThread::submitCommandBegin
//============================================================================
void* BRenderThread::submitCommandBegin(DWORD commandClass, DWORD commandType, DWORD dataLen)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT((dataLen <= USHRT_MAX) && (commandClass <= USHRT_MAX));
         
#ifdef BUILD_DEBUG
   BDEBUG_ASSERT(!mSubmittingCommand);
   mSubmittingCommand = true;
   mSubmittingCommandDataLen = dataLen;
#endif   
      
   const DWORD cmdSize = Utils::RoundUp(sizeof(BRenderCommandHeader) + dataLen, sizeof(DWORD));

   if (!mpCmdFIFOSegment)
      beginProducerSegment();
      
   UPDATE_STATS(mNumCommands, 1);
   UPDATE_STATS(mCommandBytes, cmdSize);

   DWORD cmdBufLeft = cSegmentSize - mCmdFIFOSegmentOfs;

   if (cmdSize > cmdBufLeft)
   {
      kickCommands();

      beginProducerSegment();
            
      cmdBufLeft = cSegmentSize - mCmdFIFOSegmentOfs;
   }

   BASSERT(cmdSize <= cmdBufLeft);
   
   BRenderCommandHeader* pHeader = reinterpret_cast<BRenderCommandHeader*>(mpCmdFIFOSegment + mCmdFIFOSegmentOfs);
   pHeader->mClass = static_cast<WORD>(commandClass);
   pHeader->mType = static_cast<DWORD>(commandType);
   pHeader->mLen = static_cast<WORD>(dataLen);
   
   void* p = mpCmdFIFOSegment + mCmdFIFOSegmentOfs + sizeof(BRenderCommandHeader);

#ifndef BUILD_FINAL
   memset(p, 0xFE, dataLen);
#endif

   return p;
}

//============================================================================
// BRenderThread::submitCommandEnd
//============================================================================
void BRenderThread::submitCommandEnd(DWORD dataLen)
{
   ASSERT_MAIN_THREAD
      
#ifdef BUILD_DEBUG
   BDEBUG_ASSERT(mSubmittingCommand && (dataLen == mSubmittingCommandDataLen));
   mSubmittingCommand = false;
#endif 
   
   const DWORD cmdSize = Utils::RoundUp(sizeof(BRenderCommandHeader) + dataLen, sizeof(DWORD));
         
   mCmdFIFOSegmentOfs += cmdSize;
   
#ifndef BUILD_FINAL
   static bool insideSubmitCommandEnd;
   if ((!insideSubmitCommandEnd) && (mSingleStep))
   {
      insideSubmitCommandEnd = true;
      blockUntilWorkerIdle();
      insideSubmitCommandEnd = false;
   }
#endif   
}

//============================================================================
// BRenderThread::submitCommandBegin
//============================================================================
void* BRenderThread::submitCommandBegin(DWORD commandClass, DWORD commandType, DWORD dataLen, DWORD alignment)
{
   if (alignment <= sizeof(DWORD))
      return submitCommandBegin(commandClass, commandType, dataLen);
      
   void* p = submitCommandBegin(commandClass, commandType, dataLen + alignment - 1);
   return Utils::AlignUp(p, alignment);
}

//============================================================================
// BRenderThread::submitCommandEnd
//============================================================================
void BRenderThread::submitCommandEnd(DWORD dataLen, DWORD alignment)
{
   if (alignment <= sizeof(DWORD))
      submitCommandEnd(dataLen);
   else
      submitCommandEnd(dataLen + alignment - 1);
}

//============================================================================
// BRenderThread::submitCallback
//============================================================================
void BRenderThread::submitCallback(BRenderCommandCallbackPtr pCallback, const void* pData)
{
   ASSERT_MAIN_THREAD
   
   // pData will be copied into the command buffer as a DWORD size object.
   submitCommand(cRCCCommandCallback, reinterpret_cast<DWORD>(pCallback), pData);
}

//============================================================================
// BRenderThread::submitCallbackWithData
//============================================================================
void BRenderThread::submitCallbackWithData(BRenderCommandCallbackPtr pCallback, DWORD dataLen, const void* pData, DWORD alignment)
{
   ASSERT_MAIN_THREAD

   void* pDst = submitCommandBegin(cRCCCommandCallbackWithData, reinterpret_cast<DWORD>(pCallback), dataLen, alignment);
         
   memcpy(pDst, pData, dataLen);
      
   submitCommandEnd(dataLen);
}

//============================================================================
// BRenderThread::submitFunctor
//============================================================================
void BRenderThread::submitFunctor(const BFunctor& functor, void* pData)
{
   ASSERT_MAIN_THREAD
   
   const uint functorSize = sizeof(functor);
   
   BFunctor* p = static_cast<BFunctor*>(submitCommandBegin(cRCCCommandFunctor, (uint)pData, functorSize, ALIGN_OF(BFunctor)));
   
   Utils::ConstructInPlace(p, functor);
   
   submitCommandEnd(functorSize, ALIGN_OF(BFunctor));
}

//============================================================================
// struct BDataCopyDesc
//============================================================================
struct BDataCopyDesc
{
   void* mpDst;
   const void* mpSrc;
   uint mLen;
};

//============================================================================
// BRenderThread::submitDataCopy
//============================================================================
void BRenderThread::submitDataCopy(void* pDst, const void* pSrc, uint len)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pDst && pSrc && len);

   if (len >= (cSegmentSize / 2))
   {
      void* p = allocateFrameStorage(len, 16);
      BVERIFY(p);
      Utils::FastMemCpy(p, pSrc, len);
      
      BDataCopyDesc& dataCopy = *static_cast<BDataCopyDesc*>(submitCommandBegin(cRCCControl, cRCCDataCopy, sizeof(BDataCopyDesc)));

      dataCopy.mpDst = pDst;
      dataCopy.mpSrc = p;
      dataCopy.mLen = len;

      submitCommandEnd(sizeof(BDataCopyDesc));
   }
   else
   {
      const uint totalSize = sizeof(BDataCopyDesc) + len;
      BDataCopyDesc& dataCopy = *static_cast<BDataCopyDesc*>(submitCommandBegin(cRCCControl, cRCCDataCopy, totalSize));
      
      Utils::FastMemCpy(&dataCopy + 1, pSrc, len);
      
      dataCopy.mpDst = pDst;
      dataCopy.mpSrc = &dataCopy + 1;
      dataCopy.mLen = len;
      
      submitCommandEnd(totalSize);
   }
}

//============================================================================
// BRenderThread::submitDataCopyBegin
//============================================================================
void* BRenderThread::submitDataCopyBegin(void* pDst, uint len)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pDst && len);

   void* p;
   
   if (len >= (cSegmentSize / 2))
   {
      p = allocateFrameStorage(len, 16);
      BVERIFY(p);
            
      BDataCopyDesc& dataCopy = *static_cast<BDataCopyDesc*>(submitCommandBegin(cRCCControl, cRCCDataCopy, sizeof(BDataCopyDesc)));

      dataCopy.mpDst = pDst;
      dataCopy.mpSrc = p;
      dataCopy.mLen = len;
   }
   else
   {
      const uint totalSize = sizeof(BDataCopyDesc) + len;
      BDataCopyDesc& dataCopy = *static_cast<BDataCopyDesc*>(submitCommandBegin(cRCCControl, cRCCDataCopy, totalSize));
      
      p = &dataCopy + 1;
      
      dataCopy.mpDst = pDst;
      dataCopy.mpSrc = p;
      dataCopy.mLen = len;
   }
   
   return p;
}

//============================================================================
// BRenderThread::submitDataCopyEnd
//============================================================================
void BRenderThread::submitDataCopyEnd(uint len)
{
   if (len >= (cSegmentSize / 2))
   {
      submitCommandEnd(sizeof(BDataCopyDesc));
   }
   else
   {
      submitCommandEnd(sizeof(BDataCopyDesc) + len);
   }
}

//============================================================================
// BRenderThread::submitDeferredDataCopy
//============================================================================
void BRenderThread::submitDeferredDataCopy(void* pDst, const void* pSrc, uint len)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pDst && pSrc && len);
   
   BDataCopyDesc& dataCopy = *static_cast<BDataCopyDesc*>(submitCommandBegin(cRCCControl, cRCCDataCopy, sizeof(BDataCopyDesc)));

   dataCopy.mpDst = pDst;
   dataCopy.mpSrc = pSrc;
   dataCopy.mLen = len;

   submitCommandEnd(sizeof(BDataCopyDesc));
}


//============================================================================
// BRenderThread::kickCommands
//============================================================================
void BRenderThread::kickCommands(void)
{
   ASSERT_MAIN_THREAD
   
   UPDATE_STATS(mNumCommandKicks, 1);
   
   if ((!mpCmdFIFOSegment) || (mCmdFIFOSegmentOfs == sizeof(DWORD)))
   {
      // Nothing to kick off.
      return;
   }
            
   PIXSetMarker(D3DCOLOR_ARGB(255,128,128,255), "RenderKickCommands");

   *reinterpret_cast<DWORD*>(mpCmdFIFOSegment) = mCmdFIFOSegmentOfs;
   
   const uint bytesToFlush = (mCmdFIFOSegmentOfs + 127) & ~127;
   
   BDEBUG_ASSERT(Utils::IsAligned(mpCmdFIFOSegment, 128));
   
   for (uint i = 0; i < bytesToFlush; i += 128)
       __dcbf(i, mpCmdFIFOSegment);
   
   mCmdFIFO.pushBack();

   mpCmdFIFOSegment = NULL;
   mCmdFIFOSegmentOfs = 0;
}

//============================================================================
// BRenderThread::waitUntilFrameAvailable
//============================================================================
void BRenderThread::waitUntilFrameAvailable(void)
{
   ASSERT_MAIN_THREAD

   //PIXBeginNamedEvent(0xFFFFFFFF, "WaitUntilFrameAvailable");
   SCOPEDSAMPLE(WaitUntilFrameAvailable);
         
   // rg [1/16/08] - Can't dispatch sync events here, because we haven't rendered the frame yet! If we reload something the renderer needs, we're going to crash.
   //const int status = gEventDispatcher.pumpAndWaitSingle(INFINITE, 8, mFramesRemaining.getHandle());
   const int status = gEventDispatcher.waitSingle(mFramesRemaining.getHandle(), INFINITE);
   status;
   BDEBUG_ASSERT(status == 0);

   if(mNumFramesRemaining>0)
      InterlockedDecrement(&mNumFramesRemaining);
   
   PIXEndNamedEvent();
}   
   
//============================================================================
// BRenderThread::frameBegin
//============================================================================
void BRenderThread::frameBegin(void)
{
   ASSERT_MAIN_THREAD
      
   SCOPEDSAMPLE(BRender_frameBegin);

   BDEBUG_ASSERT(!mInFrame);
   mInFrame = true;
   
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,255,255), "RenderFrameBegin: %i", mCurMainFrame);
   
   waitUntilFrameAvailable();  
            
   submitCommand(cRCCControl, cRCFrameBegin, mCurMainFrame);
      
   {
      SCOPEDSAMPLE(BRender_recycleStorage);
      recycleFrameStorage();
      recycleGPUFrameStorage();
   }

}

//============================================================================
// BRenderThread::frameEnd
//============================================================================
void BRenderThread::frameEnd(bool dispatchEvents, bool dispatchSynchronousEvents)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(mInFrame);
   
   if (dispatchEvents)
      gEventDispatcher.dispatchEvents();
      
   mInFrame = false;
   
   PIXSetMarker(D3DCOLOR_ARGB(255,255,255,255), "RenderFrameEnd: %i", mCurMainFrame);
   
   mCurMainFrame++;
  
   submitCommand(cRCCControl, cRCFrameEnd, mCurMainFrame);
   
   if (dispatchSynchronousEvents)
      gEventDispatcher.dispatchSynchronousEvents();

}

//============================================================================
// BRenderThread::isInFrame
//============================================================================
bool BRenderThread::isInFrame(void) const 
{ 
   ASSERT_MAIN_THREAD; 
   return mInFrame; 
}

//============================================================================
// BRenderThread::workerThreadFunc
//============================================================================
DWORD BRenderThread::workerThreadFunc(void* pData)
{
   __try
   {
      SCOPEDSAMPLE(BRenderThread_workerThreadFunc);
      
      BRenderThread* pRenderer = reinterpret_cast<BRenderThread*>(pData);
      pRenderer->workerThread();
   }  
   // Catch any assertion exceptions.
   __except(gAssertionSystem.xboxHandleAssertException(GetExceptionInformation()))
   {
   }
   
   return 0;    
}

//============================================================================
// BRenderThread::panic
//============================================================================
void __declspec(noreturn) BRenderThread::panic(const char* pMsg, ...)
{
   BFixedString256 buf;
   va_list va;
   va_start(va, pMsg);
   buf.formatArgs(pMsg, va);
   va_end(va);
   
   BFATAL_FAIL(buf);
   
   for ( ; ;)
   {
   }
}

//============================================================================
// BRenderThread::workerThread
//============================================================================
bool BRenderThread::workerThread(void)
{
   mWorkerThreadId = GetCurrentThreadId();
      
   gRenderHeap.SetOwnerThread(mWorkerThreadId);
   MemoryBarrier();
         
   //SetThreadName(mWorkerThreadId, "RenderThread");
   //gEventDispatcher.setThreadId(cThreadIndexRender, mWorkerThreadId);
      
   if (!initD3D())
      panic("initD3D failed");
         
   if (!commandLoop())
      panic("commandLoop failed");
         
   if (!deinitD3D())
      panic("deinitD3D failed");      
      
   gRenderHeap.SetOwnerThread(mMainThreadId);
   MemoryBarrier();
   
   //gEventDispatcher.setThreadId(cThreadIndexRender, 0);
         
   return true;
}

//============================================================================
// BRenderThread::initD3D
//============================================================================
bool BRenderThread::initD3D(void)
{
   ASSERT_RENDER_THREAD
   
   if (!BD3D::init(mD3DInitParams))
      return false;
   
   InterlockedExchange(&mD3DReadyFlag, TRUE);
   mD3DReadyEvent.set();
                              
   return true;      
}      

//============================================================================
// BRenderThread::deinitD3D
//============================================================================
bool BRenderThread::deinitD3D(void)
{
   ASSERT_RENDER_THREAD
   
   deinitAllCommandListeners();
   
   InterlockedExchange(&mD3DReadyFlag, FALSE);         
   mD3DReadyEvent.reset();
            
   return BD3D::deinit();
}      

//============================================================================
// BRenderThread::waitForConsumerSegment
//============================================================================
const uchar* BRenderThread::waitForConsumerSegment(DWORD timeout)
{
   SCOPEDSAMPLE(RenderThread_waitForConsumerSegment)
   // ASSERT_RENDER_THREAD

   const int status = gEventDispatcher.waitSingle(mCmdFIFO.getFrontSemaphore().getHandle(), timeout);
   
   if (status < 0)
      return NULL;
   else
   {
      BDEBUG_ASSERT(status == 0);
   }
   
   const BSegment* pSegment = mCmdFIFO.acquireFrontPtr();
       
   return pSegment->mData;
}

//============================================================================
// BRenderThread::commandLoop
//============================================================================
bool BRenderThread::commandLoop(void)
{
   ASSERT_RENDER_THREAD
   
   InterlockedExchange(&mProcessingCommandsFlag, TRUE);   
   mProcessingCommandsEvent.set();
   
   for ( ; ; )
   {
      if (mTerminate)
         break;
      
      const uchar* pSegment = NULL;

      // If we are in mini load rendering mode, only wait on commands for 25 milliseconds since we
      // need to call drawTimed to render the mini load screen.
      if (mInMiniLoadRenderingMode)
      {
         pSegment = waitForConsumerSegment(25);
         gMiniLoadManager.drawTimed(); 
      }
      else
      {
         pSegment = waitForConsumerSegment(100);
      }

      if (!pSegment)
         continue;


      
      DWORD segmentTotalLen = *reinterpret_cast<const DWORD*>(pSegment);
      BASSERT( (segmentTotalLen >= sizeof(DWORD)) && (segmentTotalLen <= cSegmentSize) );

      DWORD segmentOfs = sizeof(DWORD);
      
      const uchar* pSegmentEnd = pSegment + segmentTotalLen;
      
      Utils::BPrefetchState prefetchState = Utils::BeginPrefetchLargeStruct(pSegment, pSegmentEnd, 4);
      
      {
         SCOPEDSAMPLE(BRenderThread_commandLoop_processSegmentCommands)
         while (segmentOfs < segmentTotalLen)
         {
            prefetchState = Utils::UpdatePrefetchLargeStruct(pSegment + segmentOfs, pSegmentEnd, 4);
            
            const BRenderCommandHeader* pCmdHeader = reinterpret_cast<const BRenderCommandHeader*>(pSegment + segmentOfs);
            
            BDEBUG_ASSERT((segmentOfs & 3) == 0);
            
            processCommand(*pCmdHeader, pSegment + segmentOfs + sizeof(BRenderCommandHeader));

            if (mTerminate)
            {
               mCmdFIFO.popFront();      
               goto exit;
            }

            DWORD cmdLen = Utils::RoundUp(sizeof(BRenderCommandHeader) + pCmdHeader->mLen, 4);
            segmentOfs += cmdLen;

            // We could be in this loop for a while so do a draw if we are in mini load rendering mode
            if (mInMiniLoadRenderingMode)
               gMiniLoadManager.drawTimed(); 
         }
      }

      mCmdFIFO.popFront();      
   }

exit:   
   InterlockedExchange(&mProcessingCommandsFlag, FALSE);
   mProcessingCommandsEvent.reset();
   
   return true;
}

//============================================================================
// BRenderThread::getInFrame
//============================================================================
bool BRenderThread::getInFrame(void) const
{
   ASSERT_MAIN_THREAD
   return mInFrame;
}

//============================================================================
// BRenderThread::getHasD3DOwnership
//============================================================================
bool BRenderThread::getHasD3DOwnership(void) const
{
   return mHasD3DOwnership;
}

//============================================================================
// BRenderThread::processPresent
//============================================================================
void BRenderThread::processPresent(void)
{
   ASSERT_RENDER_THREAD
   mFramesRemaining.release();
   InterlockedIncrement(&mNumFramesRemaining);
   
   mLastPresentFence = BD3D::mpDev->InsertFence();
}

//============================================================================
// BRenderThread::blockUntilGPUPresent
// This block purposely prevents the render thread from getting too far 
// ahead of the GPU. It should be called after a lot of work has been kicked 
// off to the GPU to prevent it from going idle.
// We're using D3DCREATE_BUFFER_2_FRAMES to prevent D3D from throttling us 
// too early. Unfortunately, this allows us to get too far ahead of the GPU, 
// introducing too much latency.
//============================================================================
void BRenderThread::blockUntilGPUPresent(void)
{
   ASSERT_RENDER_THREAD
   
   if (mLastPresentFence == UINT64_MAX)
      return;
   
   if (BD3D::mpDev->IsFencePending((uint)mLastPresentFence))
   {
      SCOPEDSAMPLE(BRenderThread_blockUntilGPUPresent)
      
      // Is this necessary?
      BD3D::mpDev->InsertFence(); 
      
      const DWORD startTime = GetTickCount();
      while (BD3D::mpDev->IsFencePending((uint)mLastPresentFence))
      {
         // Safety factor in case IsFencePending() fails (I've seen this happen in a long game).
         if (!BD3D::mpDev->IsBusy())
            break;
            
         gWorkDistributor.wait(0, NULL, 1);
                              
         // Exit if we've waited too long
         const DWORD curWaitTime = GetTickCount() - startTime;
         if (curWaitTime > 33)
            break;
      }
   }
   
   mLastPresentFence = UINT64_MAX;
}

//============================================================================
// BRenderThread::getGPUFrameChangedEvent
//============================================================================
BWin32Event& BRenderThread::getGPUFrameChangedEvent(void)
{
   if (GetCurrentThreadId() == mMainThreadId)
      return mCurGPUFrameChangedMain;

   ASSERT_RENDER_THREAD;

   return mCurGPUFrameChangedWorker;
}

//============================================================================
// BRenderThread::gpuFrameCallback
//============================================================================
void BRenderThread::gpuFrameCallback(DWORD curFrame)
{
   // Executed at DPC time, don't do much here!
   // No need to sync, the sets() will do this for us.
   gRenderThread.mCurGPUFrame = curFrame;
   gRenderThread.mCurGPUFrameChangedMain.set();
   gRenderThread.mCurGPUFrameChangedWorker.set();
}

//============================================================================
// BRenderThread::processControlCommand
//============================================================================
void BRenderThread::processControlCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD
   
   switch (header.mType)
   {
      case cRCFrameBegin:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(DWORD));

         #ifndef BUILD_FINAL
         QueryPerformanceCounter(&mFrameStartTime);
         #endif

#if 0                  
         // rg [2/4/08] - We set the worker frame index at the END of the frame. I don't remember why this code existed.
         const DWORD curFrame = *reinterpret_cast<const DWORD*>(pData);

         mCurWorkerFrame = curFrame;
         mCurWorkerFrameChanged.set();
         BD3D::mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE, gpuFrameCallback, curFrame);
#endif         

         PIXSetMarker(D3DCOLOR_ARGB(255,255,255,255), "CurWorkerFrame: %i", mCurWorkerFrame);

         for (uint i = 0; i < gRenderThread.getMaxCommandListeners(); i++)
         {
            BRenderCommandListenerInterface* pListener = getCommandListenerByIndex(i);
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
         if (mWorkerEventReceiverHandle != cInvalidEventReceiverHandle)
         {
            gEventDispatcher.removeClientImmediate(mWorkerEventReceiverHandle);
            mWorkerEventReceiverHandle = cInvalidEventReceiverHandle;
         }
         
         queueTermination();
         break;
      }
      case cRCWorkerFence:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(DWORD));
         
         // Export barrier.
         MemoryBarrier();
         
         mCurWorkerFence = *reinterpret_cast<const DWORD*>(pData);
         
         mFenceChangedEvent.set();
         break;
      }
      case cRCGPUFence:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderGPUFenceCommandData));
         const BRenderGPUFenceCommandData& data = *reinterpret_cast<const BRenderGPUFenceCommandData*>(pData);

         PIXSetMarker(D3DCOLOR_ARGB(255,0,255,128), "RenderControlInsertFence");
                  
         uint index = BD3D::mpDev->InsertFence();
         if (0 == index)
            index = BD3D::mpDev->InsertFence();

         // Export barrier.
         MemoryBarrier();
                           
         data.mpFence->mIndex = (index | (((uint64)data.mIssueCount) << 32));
                  
         mGPUFenceSubmittedEvent.set();

         break;
      }
      case cRCFrameEnd:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(DWORD));

         // This is the NEXT frame's index.
         const DWORD curFrame = *reinterpret_cast<const DWORD*>(pData);

         mCurWorkerFrame = curFrame;
         mCurWorkerFrameChanged.set();

         BD3D::mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE, gpuFrameCallback, curFrame);
                           
         for (uint i = 0; i < getMaxCommandListeners(); i++)
         {
            BRenderCommandListenerInterface* pListener = getCommandListenerByIndex(i);
            if (pListener)
               pListener->frameEnd();
         }
         
         BD3D::mpDev->InsertFence();
                                            
         #ifndef BUILD_FINAL
         LARGE_INTEGER currTime;
         QueryPerformanceCounter(&currTime);
         LARGE_INTEGER freq;
         QueryPerformanceFrequency(&freq);
         DWORD delta = DWORD(float(currTime.QuadPart - mFrameStartTime.QuadPart)*1000.0f/float(freq.QuadPart));
         if(delta < mMinFrameTime)
         {
            DWORD sleepTime = mMinFrameTime - delta;
            Sleep(sleepTime);
         }
         #endif

         gEventDispatcher.dispatchEvents();
                                                      
         break;
      }      
      case cRCBeginLevelLoad:
      {
         // Sim is idling during this code
         for (uint i = 0; i < getMaxCommandListeners(); i++)
         {
            BRenderCommandListenerInterface* pListener = getCommandListenerByIndex(i);
            if (pListener)
               pListener->beginLevelLoad();
         }
         
         deinitAllFrameStorage();
                           
         break;
      }
      case cRCEndLevelLoad:
      {
         // Sim is idling during this code
         initAllFrameStorage();
                              
         for (uint i = 0; i < getMaxCommandListeners(); i++)
         {
            BRenderCommandListenerInterface* pListener = getCommandListenerByIndex(i);
            if (pListener)
               pListener->endLevelLoad();
         }
         break;
      }
      case cRCStartMiniLoad:
      {
         //Turn mini load rendering on.  commandLoop() will call draw and present.
         //gMiniLoadManager.init();
         mInMiniLoadRenderingMode = true;
         break;
      }
      case cRCStopMiniLoad:
      {
         //Turn suspend mode off.   commandLoop() will no longer call draw and present.
         mInMiniLoadRenderingMode = false;
         //gMiniLoadManager.deinit();
         break;
      }
      case cRCChangeFrameStorageIndex:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint* pNewIndex = reinterpret_cast<const uint*>(pData);
         mFrameStorageCurWorkerIndex = *pNewIndex;
         
         break;         
      }
      case cRCChangeGPUFrameStorageIndex:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint* pNewIndex = reinterpret_cast<const uint*>(pData);
         mGPUFrameStorageCurWorkerIndex = *pNewIndex;
         
         {
            BScopedLightWeightMutex lock(mGPUFrameStorageMutex[mFrameStorageCurWorkerIndex]);
            
            BBlockStackAllocator& allocator = mGPUFrameStorageAllocators[mFrameStorageCurWorkerIndex];
            for (uint i = 0; i < allocator.getNumBlocks(); i++)
            {
               BD3D::mpDev->InvalidateGpuCache(allocator.getBlockPtr(i), allocator.getBlockSize(i), 0);
               
               // rg [8/2/08] - Why did I use the GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS macro? 
               // Why did I do this at all? This is write combined RAM!
               //Utils::FlushCacheLines(GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(allocator.getBlockPtr(i)), allocator.getBlockSize(i));
            }
         }
         
         break;         
      }
#ifndef BUILD_FINAL  
      case cRCCBreakPoint:
      {
         DebugBreak();
         break;
      }
      case cRCCDumpState:
      {
         BD3DDeviceStateDumper::dumpState();
         break;
      }
#endif      
      case cRCCSleep:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint ms = *reinterpret_cast<const uint*>(pData);
         Sleep(ms);
         break;
      }      
      case cRCCFreeAlignedMemory:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(void*));
         BAlignedAlloc::Free(*reinterpret_cast<void* const *>(pData));
         break;
      }
      case cRCCDispatchEvents:
      {
         gEventDispatcher.dispatchEvents();
         break;
      }
      case cRCCSetWin32Event:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(HANDLE));
         SetEvent(*reinterpret_cast<const HANDLE*>(pData));
         break;
      }
      case cRCCWaitOnHandle:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(HANDLE));
         WaitForSingleObject(*reinterpret_cast<const HANDLE*>(pData), INFINITE);
         break;
      }
      case cRCCSetWin32EventAndWait:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(HANDLE) * 2);
         
         const HANDLE* pHandles = reinterpret_cast<const HANDLE*>(pData);
         
         HANDLE setHandle = pHandles[0];
         HANDLE waitHandle = pHandles[1];

         if (INVALID_HANDLE_VALUE != setHandle)
            SetEvent(setHandle);

         if (INVALID_HANDLE_VALUE != waitHandle)
            WaitForSingleObject(waitHandle, INFINITE);
         break;
      }
      case cRCCGPUEventFence:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(DWORD));
         BD3D::mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE, gpuEventCallback, *reinterpret_cast<const DWORD*>(pData));
         BD3D::mpDev->InsertFence();
         break;
      }
      case cRCCGPUMainFence:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(DWORD));
         BD3D::mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE, gpuMainFenceCallback, *reinterpret_cast<const DWORD*>(pData));
         BD3D::mpDev->InsertFence();
         break;
      }
      case cRCCDataCopy:
      {
         const BDataCopyDesc& desc = *reinterpret_cast<const BDataCopyDesc*>(pData);
                           
         BDEBUG_ASSERT(desc.mpSrc && desc.mpDst && desc.mLen);
         
         Utils::FastMemCpy(desc.mpDst, desc.mpSrc, desc.mLen);
         
         break;
      }
      default:
      {
         BVERIFY(false);
         break;
      }
#ifndef BUILD_FINAL      
      case cRCCPIXBeginNamedEvent:
      {
         PIXBeginNamedEvent(0xFFFFFFFF, (const char*)pData);
         break;
      }
      case cRCCPIXEndNamedEvent:
      {
         PIXEndNamedEvent();
         break;
      }
      case cRCCPIXSetMarker:
      {
         PIXSetMarker(0xFFFFFFFF, (const char*)pData);
         break;
      }
#endif      
   }
}

//============================================================================
// BRenderThread::processCommand
//============================================================================
void BRenderThread::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD
   
   switch (header.mClass)
   {
      case cRCCNop:
      {
         break;
      }
      case cRCCControl:
      {
         processControlCommand(header, pData);
                                                
         break;
      }
      case cRCCCommandObjPtr:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(const BRenderCommandObjectInterface*));

         const BRenderCommandObjectInterface* pCommandObj = *reinterpret_cast<const BRenderCommandObjectInterface* const*>(pData);

         pCommandObj->processCommand(header.mType);
         
         break;
      }         
      case cRCCCommandObjCopy:      
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BRenderCommandObjectInterface));
         
         const BRenderCommandObjectInterface* pCommandObj = reinterpret_cast<const BRenderCommandObjectInterface*>(pData);
         
         pCommandObj->processCommand(header.mType);
                           
         break;
      }
      case cRCCCommandCallback:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderCommandCallbackPtr));
         
         BRenderCommandCallbackPtr pCallbackFunc = reinterpret_cast<BRenderCommandCallbackPtr>(header.mType);
                  
         pCallbackFunc(*reinterpret_cast<void* const*>(pData));
         
         break;
      }
      case cRCCCommandCallbackWithData:
      {
         BRenderCommandCallbackPtr pCallbackFunc = reinterpret_cast<BRenderCommandCallbackPtr>(header.mType);

         pCallbackFunc(const_cast<uchar*>(pData));

         break;
      }
      case cRCCCommandFunctor:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BFunctor));
         
         BFunctor* pFunctor = reinterpret_cast<BFunctor*>(const_cast<uchar*>(Utils::AlignUp(pData, ALIGN_OF(BFunctor))));
      
         (*pFunctor)((void*)header.mType);
         
         Utils::DestructInPlace(pFunctor);
      
         break;
      }
      default:
      {
         if ((header.mClass >= cRCCMax) && (header.mClass <= (cRCCMax + cMaxCommandListeners - 1)))
         {
            const uint index = header.mClass - cRCCMax;
            BDEBUG_ASSERT(mpCommandListeners[index]);
            mpCommandListeners[index]->processCommand(header, pData);
         }
         else
         {
            BFATAL_FAIL("Invalid command class");
         }
      }
   }
}

//============================================================================
// BRenderThread::sleep
//============================================================================
void BRenderThread::sleep(uint ms)
{
   ASSERT_MAIN_THREAD
   submitCommand(cRCCControl, cRCCSleep, ms);
}

//============================================================================
// BRenderThread::dumpState
//============================================================================
void BRenderThread::dumpState(void)
{
   ASSERT_MAIN_THREAD
#ifndef BUILD_FINAL   
   submitCommand(cRCCControl, cRCCDumpState);
#endif   
}

//============================================================================
// BRenderThread::breakPoint
//============================================================================
void BRenderThread::breakPoint(void)
{
   ASSERT_MAIN_THREAD
#ifndef BUILD_FINAL   
   submitCommand(cRCCControl, cRCCBreakPoint);
#endif   
}

//============================================================================
// BRenderThread::insertWorkerFence
//============================================================================
uint BRenderThread::insertWorkerFence(void)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(mProcessingCommandsFlag);
   
   mNextWorkerFence++;
   if (!mNextWorkerFence)
      mNextWorkerFence++;
      
   const uint curFence = mNextWorkerFence;
      
   submitCommand(cRCCControl, cRCWorkerFence, sizeof(uint), &curFence);
   
   return curFence;   
}

//============================================================================
// BRenderThread::blockOnWorkerFence
//============================================================================
void BRenderThread::blockOnWorkerFence(uint fenceIndex)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(mProcessingCommandsFlag);

   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,0,127), "RenderBlockOnFence");   

   kickCommands();
   
   for ( ; ; )
   {
      if (!isWorkerFencePending(fenceIndex))
         break;
                       
      const int status = gEventDispatcher.waitSingle(mFenceChangedEvent.getHandle());
      status;
      BDEBUG_ASSERT(status == 0);
   }

   // Import barrier.
   MemoryBarrier();
   
   PIXEndNamedEvent();
}

//============================================================================
// BRenderThread::blockUntilWorkerIdle
//============================================================================
void BRenderThread::blockUntilWorkerIdle(void)
{
   SCOPEDSAMPLE(BRenderThread_blockUntilWorkerIdle)
   ASSERT_MAIN_THREAD
   
   if ((INVALID_HANDLE_VALUE == mThreadHandle) || (!mProcessingCommandsFlag))
      return;
         
   blockOnWorkerFence(insertWorkerFence());
}

//============================================================================
// BRenderThread::blockUntilGPUIdle
//============================================================================
void BRenderThread::blockUntilGPUIdle(void)
{
   SCOPEDSAMPLE(BRenderThread_blockUntilGPUIdle)
   if ((!mInitialized) || (!mProcessingCommandsFlag) || (!BD3D::mpDev))
      return;
      
   //BDEBUG_ASSERT(mProcessingCommandsFlag);

   if (GetCurrentThreadId() == mMainThreadId)   
   {
      BRenderGPUFence gpuFence;
      gpuFence.issue();
      gpuFence.blockWhilePending();
   }
   else
   {
      BD3D::mpDev->BlockUntilIdle();
   }
}

//============================================================================
// BRenderThread::gpuEventCallback
//============================================================================
void BRenderThread::gpuEventCallback(DWORD context)
{
   // Executed at DPC time, don't do much here!
   HANDLE eventHandle = (HANDLE)context;
   SetEvent(eventHandle);
}

//============================================================================
// BRenderThread::insertGPUEventFence
//============================================================================
void BRenderThread::insertGPUEventFence(HANDLE eventHandle)
{
   if (GetCurrentThreadId() == mMainThreadId)
      submitCommand(cRCCControl, cRCCGPUEventFence, eventHandle);
   else 
   {
      ASSERT_RENDER_THREAD
      
      BD3D::mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE, gpuEventCallback, (DWORD)eventHandle);
      BD3D::mpDev->InsertFence();
   }
}

//============================================================================
// BRenderThread::gpuMainFenceCallback
//============================================================================
void BRenderThread::gpuMainFenceCallback(DWORD fenceIndex)
{
   // No need to sync, the event set will do this for us.
   gRenderThread.mCurGPUMainFenceIndex = fenceIndex;
   gRenderThread.mGPUMainFenceIndexChanged.set();
}

//============================================================================
// BRenderThread::gpuWorkerFenceCallback
//============================================================================
void BRenderThread::gpuWorkerFenceCallback(DWORD fenceIndex)
{
   // No need to sync, the event set will do this for us.
   gRenderThread.mCurGPUWorkerFenceIndex = fenceIndex;
   gRenderThread.mGPUWorkerFenceIndexChanged.set();
}

//============================================================================
// BRenderThread::issueGPUFence
//============================================================================
DWORD BRenderThread::issueGPUFence(void)
{
   DWORD curFenceIndex;
   
   if (GetCurrentThreadId() == mMainThreadId)
   {
      mNextGPUMainFenceIndex++;
      
      curFenceIndex = mNextGPUMainFenceIndex;   
      
      submitCommand(cRCCControl, cRCCGPUMainFence, curFenceIndex);
            
      kickCommands();
   }
   else 
   {
      ASSERT_RENDER_THREAD
      
      mNextGPUWorkerFenceIndex++;   
      
      curFenceIndex = mNextGPUWorkerFenceIndex;   
      
      BD3D::mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE, gpuWorkerFenceCallback, curFenceIndex);
      BD3D::mpDev->InsertFence();
   }
   
   return curFenceIndex;
}

//============================================================================
// BRenderThread::getCurGPUFenceNoBarrier
//============================================================================
DWORD BRenderThread::getCurGPUFenceNoBarrier(void) 
{
   DWORD curFenceIndex;
            
   if (GetCurrentThreadId() == mMainThreadId)
      curFenceIndex = mCurGPUMainFenceIndex;   
   else 
   {
      ASSERT_RENDER_THREAD
      
      curFenceIndex = mCurGPUWorkerFenceIndex;   
   }
   
   return curFenceIndex;
}

//============================================================================
// BRenderThread::getCurGPUFence
//============================================================================
DWORD BRenderThread::getCurGPUFence(void) 
{
   MemoryBarrier();
   return getCurGPUFenceNoBarrier();
}

//============================================================================
// BRenderThread::isGPUFencePendingNoBarrier
//============================================================================
bool BRenderThread::isGPUFencePendingNoBarrier(DWORD index) 
{
   const int delta = static_cast<int>(getCurGPUFenceNoBarrier() - index);
   return delta < 0;
}

//============================================================================
// BRenderThread::isGPUFencePending
//============================================================================
bool BRenderThread::isGPUFencePending(DWORD index) 
{
   const int delta = static_cast<int>(getCurGPUFence() - index);
   return delta < 0;
}

//============================================================================
// BRenderThread::blockOnGPUFence
//============================================================================
void BRenderThread::blockOnGPUFence(DWORD index)
{
   if (!isGPUFencePending(index))
      return;

   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,127,0), "RenderBlockOnGPUFence");   
         
   do
   {
      gEventDispatcher.waitSingle(getGPUFenceChangedEvent().getHandle(), INFINITE);
   } while (isGPUFencePendingNoBarrier(index));
   
   PIXEndNamedEvent();
}

//============================================================================
// BRenderThread::getGPUFenceChangedEvent
//============================================================================
BWin32Event& BRenderThread::getGPUFenceChangedEvent(void) 
{
   if (GetCurrentThreadId() == mMainThreadId)
      return mGPUMainFenceIndexChanged;
   
   ASSERT_RENDER_THREAD

   return mGPUWorkerFenceIndexChanged;
}

//============================================================================
// BRenderThread::blockOnGPUFrameOrIdle
//============================================================================
void BRenderThread::blockOnGPUFrameOrIdle(DWORD frame)
{  
   ASSERT_MAIN_OR_WORKER_THREAD
   
   // Has the GPU passed this frame already?
   const int delta = static_cast<int>(getCurGPUFrame() - frame);
   if (delta > 0)
      return;
      
   PIXBeginNamedEvent(D3DCOLOR_ARGB(255,255,127,0), "RenderBlockOnGPUFrameOrIdle");   
                  
   // Issue a GPU fence to detect if the GPU idles.
   const DWORD gpuFenceIndex = issueGPUFence();
      
   // Now wait on the frame changed and fence changed handles for this thread.
   HANDLE handles[2] = { getGPUFrameChangedEvent().getHandle(), getGPUFenceChangedEvent().getHandle() };
   
   for ( ; ; )
   {
      gEventDispatcher.wait(2, handles, INFINITE);
      
      const int delta = static_cast<int>(mCurGPUFrame - frame);
      if (delta > 0)
         break;
         
      if (!isGPUFencePending(gpuFenceIndex))         
         break;
   }
   
   PIXEndNamedEvent();
}

//============================================================================
// BRenderThread::hasGPUFrameBeenPassed
//============================================================================
bool BRenderThread::hasGPUFrameBeenPassed(DWORD frame)
{
   // Has the GPU passed this frame already?
   const int delta = static_cast<int>(getCurGPUFrame() - frame);
   return (delta > 0);
}

//============================================================================
// BRenderThread::initGPUFrameStorage
//============================================================================
bool BRenderThread::initGPUFrameStorage(void)
{
   for (uint i = 0; i < cMaxQueuedGPUFrames; i++)
      mGPUFrameStorageAllocators[i].init(&gPhysWriteCombinedHeap, 256*1024, 256*1024, 6);

   mGPUFrameStorageCurMainIndex = 0;
   mGPUFrameStorageCurWorkerIndex = 0;

   if (!mpGPUFrameStorageFences)
      mpGPUFrameStorageFences = new BRenderGPUFence[cMaxQueuedGPUFrames];

   return true;
}

//============================================================================
// BRenderThread::initAllFrameStorage
//============================================================================
bool BRenderThread::initAllFrameStorage(void)
{
   // Init CPU frame storage
   Utils::ClearObj(mFrameStorageFences);
   mFrameStorageCurMainIndex = 0;
   mFrameStorageCurWorkerIndex = 0;
   for (uint i = 0; i < cMaxQueuedFrames; i++)
      mFrameStorageAllocators[i].init(&gRenderHeap, 32768, 32768, 6);

   initGPUFrameStorage();
                     
   return true;
}

//============================================================================
// BRenderThread::deinitGPUFrameStorage
//============================================================================
bool BRenderThread::deinitGPUFrameStorage(void)
{
   for (uint i = 0; i < cMaxQueuedGPUFrames; i++)
      mGPUFrameStorageAllocators[i].kill();

   mGPUFrameStorageCurMainIndex = 0;
   mGPUFrameStorageCurWorkerIndex = 0;

   delete[] mpGPUFrameStorageFences;
   mpGPUFrameStorageFences = NULL;

   return true;
}

//============================================================================
// BRenderThread::deinitAllFrameStorage
//============================================================================
bool BRenderThread::deinitAllFrameStorage(void)
{
   Utils::ClearObj(mFrameStorageFences);
   mFrameStorageCurMainIndex = 0;
   mFrameStorageCurWorkerIndex = 0;
         
   for (uint i = 0; i < cMaxQueuedFrames; i++)
      mFrameStorageAllocators[i].kill();

   deinitGPUFrameStorage();
         
   return true;
}

//============================================================================
// BRenderThread::recycleFrameStorage
//============================================================================
void BRenderThread::recycleFrameStorage(void)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(!mInsideLevelLoad);
   
   mFrameStorageFences[mFrameStorageCurMainIndex] = insertWorkerFence();
   
   mFrameStorageCurMainIndex++;
   if (cMaxQueuedFrames == mFrameStorageCurMainIndex)
      mFrameStorageCurMainIndex = 0;
            
   // Now switch to the new segment -- block until the worker thread is done with it.
   if (mFrameStorageFences[mFrameStorageCurMainIndex])
      blockOnWorkerFence(mFrameStorageFences[mFrameStorageCurMainIndex]);
   
   {
      BScopedLightWeightMutex lock(mFrameStorageMutex[mFrameStorageCurMainIndex]);
      
      UPDATE_STATS(mCPUFrameStorageBytes, mFrameStorageAllocators[mFrameStorageCurMainIndex].getTotalBytesAllocated());
      
      BDEBUG_ASSERT(mFrameStorageAllocators[mFrameStorageCurMainIndex].getHeap());
      mFrameStorageAllocators[mFrameStorageCurMainIndex].freeAll();
            
#ifdef TEST_FRAME_STORAGE
      mFrameStorageAllocators[mFrameStorageCurMainIndex].setAllBlocks(0xFE);
#endif   
   }

   // Tell the worker thread to use the new segment for its future allocations.
   submitCommand(cRCCControl, cRCChangeFrameStorageIndex, mFrameStorageCurMainIndex);           
}

//============================================================================
// BRenderThread::recycleGPUFrameStorage
//============================================================================
void BRenderThread::recycleGPUFrameStorage(void)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(!mInsideLevelLoad);
   
   // rg [2/17/06] - This is also implicitly a worker thread fence.
   mpGPUFrameStorageFences[mGPUFrameStorageCurMainIndex].issue();

   mGPUFrameStorageCurMainIndex++;
   if (cMaxQueuedGPUFrames == mGPUFrameStorageCurMainIndex)
      mGPUFrameStorageCurMainIndex = 0;

   BDEBUG_ASSERT(mpGPUFrameStorageFences);
   
   // rg [2/17/06] - Not only will this block wait for the GPU to be done with this segment, but this also implicitly waits for the worker thread too.
   mpGPUFrameStorageFences[mGPUFrameStorageCurMainIndex].blockWhilePending();
   
   {
      BScopedLightWeightMutex lock(mGPUFrameStorageMutex[mGPUFrameStorageCurMainIndex]);
      
      UPDATE_STATS(mGPUFrameStorageBytes, mGPUFrameStorageAllocators[mGPUFrameStorageCurMainIndex].getTotalBytesAllocated());

      BDEBUG_ASSERT(mGPUFrameStorageAllocators[mGPUFrameStorageCurMainIndex].getHeap());            
      mGPUFrameStorageAllocators[mGPUFrameStorageCurMainIndex].freeAll();

#ifdef TEST_FRAME_STORAGE
      mGPUFrameStorageAllocators[mGPUFrameStorageCurMainIndex].setAllBlocks(0xFE);
#endif 
   }
        
   // Tell the worker thread to use the new segment for its future allocations.
   submitCommand(cRCCControl, cRCChangeGPUFrameStorageIndex, mGPUFrameStorageCurMainIndex);
}

//============================================================================
// BRenderThread::allocateFrameStorage
//============================================================================
void* BRenderThread::allocateFrameStorage(uint len, uint alignment, bool failIfFull, bool simThread)
{
   BDEBUG_ASSERT(!mInsideLevelLoad);
   
   uchar* ptr;
   
   const uint index = simThread ? mFrameStorageCurMainIndex : mFrameStorageCurWorkerIndex;
   
   {
      BScopedLightWeightMutex lock(mFrameStorageMutex[index]);
   
      UPDATE_STATS(mMainCPUFrameStorageAllocs, 1);
      BDEBUG_ASSERT(mFrameStorageAllocators[index].getHeap());
                     
      ptr = static_cast<uchar*>(mFrameStorageAllocators[index].alloc(len, alignment));
   }
         
   if (!ptr)
   {
      if (failIfFull)
      {
         BFATAL_FAIL("BRenderThread::allocateFrameStorage: Out of memory");
      }
      return NULL;
   }
   
   if (len >= 256)
   {
//-- FIXING PREFIX BUG ID 7093
      const uchar* pEnd = ptr + len;
//--
      uchar* pFirstCacheLineToClear = Utils::AlignUp(ptr, 128);
      
      if (pFirstCacheLineToClear < pEnd)
      {
         const uint totalBytesToClear = (pEnd - pFirstCacheLineToClear) & ~127;
         
         BDEBUG_ASSERT( (pFirstCacheLineToClear >= ptr) && (pFirstCacheLineToClear + totalBytesToClear - 1) < pEnd );
                  
         for (uint i = 0; i < totalBytesToClear; i += 128)
            __dcbz128(i, pFirstCacheLineToClear);
      }            
   }         

   return ptr;
}

//============================================================================
// BRenderThread::allocateGPUFrameStorage
//============================================================================
void* BRenderThread::allocateGPUFrameStorage(uint len, uint alignment, bool failIfFull, bool simThread)
{
   BDEBUG_ASSERT(!mInsideLevelLoad);
   
   alignment = Math::Max<uint>(alignment, cGPUFrameStorageMinAlignment);
   
   const uint index = simThread ? mGPUFrameStorageCurMainIndex : mGPUFrameStorageCurWorkerIndex;
   
   uchar* ptr;
   
   {
      BScopedLightWeightMutex lock(mGPUFrameStorageMutex[index]);

      UPDATE_STATS(mMainGPUFrameStorageAllocs, 1);
      BDEBUG_ASSERT(mGPUFrameStorageAllocators[index].getHeap());
      
      ptr = static_cast<uchar*>(mGPUFrameStorageAllocators[index].alloc(len, alignment));
   }

   if (!ptr)
   {
      if (failIfFull)
      {
         BFATAL_FAIL("BRenderThread::allocateGPUFrameStorage: Out of memory");
      }
      return NULL;
   }
   
   return ptr;
}

//============================================================================
// BRenderThread::getAvailableFrameStorage
//============================================================================
uint BRenderThread::getAvailableFrameStorage(void)
{
   return 0xFFFFFFFF;
}

//============================================================================
// BRenderThread::workerGetAvailableFrameStorage
//============================================================================
uint BRenderThread::workerGetAvailableFrameStorage(void)
{
   return 0xFFFFFFFF;
}

//============================================================================
// BRenderThread::getAvailableGPUFrameStorage
//============================================================================
uint BRenderThread::getAvailableGPUFrameStorage(void)
{
   return 0xFFFFFFFF;
}

//============================================================================
// BRenderThread::workerGetAvailableGPUFrameStorage
//============================================================================
uint BRenderThread::workerGetAvailableGPUFrameStorage(void)
{
   return 0xFFFFFFFF;
}

//============================================================================
// BRenderThread::registerCommandListener
//============================================================================
BCommandListenerHandle BRenderThread::registerCommandListener(BRenderCommandListenerInterface* pListenerObject)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   
   BDEBUG_ASSERT(mProcessingCommandsFlag);

   mCommandListenerMutex.lock();
   
   uint i;
   for (i = 0; i < cMaxCommandListeners; i++)
      if (!mpCommandListeners[i])
         break;

   if (cMaxCommandListeners == i)
   {
      BFATAL_FAIL("Too many command listeners - increase cMaxCommandListeners");
   }
   
   BCommandListenerHandle handle = cRCCMax + i;
   BDEBUG_ASSERT((handle >= cRCCMax) && (handle <= (cRCCMax + cMaxCommandListeners - 1)) );
   
   if (GetCurrentThreadId() == mMainThreadId)        
   {
      // Write dummy pointer into slot - indicates that this slot is used, so future calls won't use this slot for a handle.
      // This will prevent the worker thread from immediately sending beginFrame(), etc. commands to the listener.
      // Other than this, this is the only write to the mpCommandListeners array from the main thread after init.
      mpCommandListeners[i] = (BRenderCommandListenerInterface*)cCommandListenerSlotOccupiedPtr;
      
      mCommandListenerMutex.unlock();
            
      // Ask the worker thread to write the slot pointer.   
      submitCopyOfCommandObject(BInitCommandListenerRCO(pListenerObject), (DWORD)&mpCommandListeners[i]);
   }
   else
   {
      mpCommandListeners[i] = pListenerObject;
      
      mCommandListenerMutex.unlock();
      
      pListenerObject->initDeviceData();
   }      
   
   return handle;
}

//============================================================================
// BRenderThread::freeCommandListener
//============================================================================
void BRenderThread::freeCommandListener(BCommandListenerHandle handle)
{
   ASSERT_MAIN_OR_WORKER_THREAD

   if (cInvalidCommandListenerHandle == handle)
      return;
      
   BDEBUG_ASSERT(mProcessingCommandsFlag);
   BDEBUG_ASSERT((handle >= cRCCMax) && (handle <= (cRCCMax + cMaxCommandListeners - 1)) );
      
   const uint index = handle - cRCCMax;

   MemoryBarrier();         
   
   if (GetCurrentThreadId() == mMainThreadId)      
   {
      BDEBUG_ASSERT(mpCommandListeners[index]);
      
      // Ask the worker thread to call deinit() on the object, then set the slot to NULL.
      submitCopyOfCommandObject(BDeinitCommandListenerRCO(), (DWORD)&mpCommandListeners[index]);
   
      // We have to block until the deinit() callback is executed - otherwise, if we immediately returned the object could go 
      // out of scope before the worker thread calls deinit() on it.
      blockUntilWorkerIdle();
   }
   else
   {
      mpCommandListeners[index]->deinitDeviceData(); 
      
      Sync::InterlockedExchangeExport((LONG*)&mpCommandListeners[index], NULL); 
   }
}

//============================================================================
// BRenderThread::getMaxCommandListeners
//============================================================================
uint BRenderThread::getMaxCommandListeners(void) const
{
    return cMaxCommandListeners;
}

//============================================================================
// BRenderThread::getCommandListenerByIndex
//============================================================================
BRenderCommandListenerInterface* BRenderThread::getCommandListenerByIndex(uint index) const
{
   ASSERT_RENDER_THREAD;
   BASSERT(index < cMaxCommandListeners);
   BRenderCommandListenerInterface* pListener = mpCommandListeners[index];
   // The main thread may have reserved this slot before we've seen the command to set it to a valid pointer.
   if ((DWORD)pListener == cCommandListenerSlotOccupiedPtr)
      pListener = NULL;
   return pListener;
}

//============================================================================
// BRenderThread::deinitAllCommandListeners
//============================================================================
void BRenderThread::deinitAllCommandListeners(void)
{
   ASSERT_RENDER_THREAD
   
   BScopedLightWeightMutex lock(mCommandListenerMutex);
   
   for (uint i = 0; i < cMaxCommandListeners; i++)
      if (mpCommandListeners[i])
      {
         mpCommandListeners[i]->deinitDeviceData();
         Sync::InterlockedExchangeExport((LONG*)&mpCommandListeners[i], NULL);
      }
}

//============================================================================
// BRenderThread::deferredAlignedFree
//============================================================================
void BRenderThread::deferredAlignedFree(void* pData)
{
   if (!pData)
      return;
      
   if (GetCurrentThreadId() == mMainThreadId)
     submitCommand(cRCCControl, cRCCFreeAlignedMemory, pData);  
   else 
      BAlignedAlloc::Free(pData);
}

//============================================================================
// BRenderThread::allocGPUFences
//============================================================================
BRenderGPUFence* BRenderThread::allocGPUFences(uint numFences)
{
   BDEBUG_ASSERT(numFences);
   
   BRenderGPUFence* pFences = reinterpret_cast<BRenderGPUFence*>(BAlignedAlloc::Malloc(sizeof(BRenderGPUFence) * numFences));
   for (uint i = 0; i < numFences; i++)
      Utils::ConstructInPlace(pFences + i);

   return pFences;
}

//============================================================================
// BRenderThread::deleteGPUFences
//============================================================================
void BRenderThread::deleteGPUFences(BRenderGPUFence* pFences)
{
   // The fences will not be destructed properly. 
   // The destructor is only called to ensure the fence's memory is no longer in use by the worker thread.
   // This is fine, because we know for sure the fences will have been used by the worker thread before the memory
   // blocking them is freed.
   deferredAlignedFree(pFences);
}

//============================================================================
// BRenderThread::setWin32Event
//============================================================================
void BRenderThread::setWin32Event(HANDLE eventHandle)
{
   ASSERT_MAIN_THREAD
   submitCommand(cRCCControl, cRCCSetWin32Event, eventHandle);
}

//============================================================================
// BRenderThread::waitOnHandle
//============================================================================
void BRenderThread::waitOnHandle(HANDLE waitHandle)
{
   ASSERT_MAIN_THREAD
   submitCommand(cRCCControl, cRCCWaitOnHandle, waitHandle);
}

//============================================================================
// BRenderThread::setWin32EventAndWait
//============================================================================
void BRenderThread::setWin32EventAndWait(HANDLE eventHandle, HANDLE waitHandle)
{
   ASSERT_MAIN_THREAD
   
   HANDLE* pHandles = reinterpret_cast<HANDLE*>(submitCommandBegin(cRCCSetWin32EventAndWait, 0, sizeof(HANDLE) * 2));

   pHandles[0] = eventHandle;
   pHandles[1] = waitHandle;

   submitCommandEnd(sizeof(HANDLE) * 2);
}

//============================================================================
// BRenderThread::receiveEvent
//============================================================================
bool BRenderThread::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{      
   return false;
}

#ifndef BUILD_FINAL
//============================================================================
// BRenderThread::submitPIXBeginNamedEvent
//============================================================================
void BRenderThread::submitPIXBeginNamedEvent(const char* pName)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pName);

   const uint len = strlen(pName);

   Utils::FastMemCpy(submitCommandBegin(cRCCControl, cRCCPIXBeginNamedEvent, len + 1), pName, len + 1);

   submitCommandEnd(len + 1);
}

//============================================================================
// BRenderThread::submitPIXEndNamedEvent
//============================================================================
void BRenderThread::submitPIXEndNamedEvent(void)
{
   ASSERT_MAIN_THREAD

   submitCommand(cRCCControl, cRCCPIXEndNamedEvent);
};

//============================================================================
// BRenderThread::submitPIXSetMarker
//============================================================================
void BRenderThread::submitPIXSetMarker(const char* pName)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pName);

   const uint len = strlen(pName);

   Utils::FastMemCpy(submitCommandBegin(cRCCControl, cRCCPIXSetMarker, len + 1), pName, len + 1);

   submitCommandEnd(len + 1);
}
#endif
