//============================================================================
//
//  renderThread.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// xcore
#include "threading\win32Event.h"
#include "threading\win32Semaphore.h"
#include "functor\functor.h"
//#include "threading\interlocked.h"

// local
#include "threading\commandFIFO.h"
#include "threading\synchronizedFIFO.h"
#include "renderCommand.h"
#include "threading\eventDispatcher.h"
#include "memory\blockStackAllocator.h"

#include "BD3D.h"

//============================================================================
// class BRenderGPUFence
// BRenderGPUFence is a legacy thread-safe wrapper around the D3D GPU fence API.
// This object is tricky to use - you cannot quickly free BRenderGPUFence objects directly from the main thread (see allocGPUFences/deleteGPUFences).
// Use the issueGPUFence() API in BRenderThread, which is simpler and safer to use. Or, use GPU frame based fencing (blockOnGPUFrameOrIdle).
//============================================================================
#pragma warning(push)
#pragma warning(disable:4324) // pad warning
__declspec(align(8))
class BRenderGPUFence
{
public:
   friend class BRenderThread;

   // BRenderGPUFence is usable from the main or worker threads.   
   // If it is used from the worker thread, issued fences are immediately submitted to D3D.
   // Otherwise, issued fences are queued for submission into the command buffer.
   BRenderGPUFence();
   
   // The destructor may wait for a while if the fence hasn't been submitted.
   // It's OK if the destructor is not called if the object is being freed by the worker thread.
   ~BRenderGPUFence();
         
   // non-issued -> issued ->submitted -> pending -> non-pending
         
   // Issues the GPU fence request.
   // The worker thread will directly modify this object once it has the D3D fence index.
   void issue(void);
   
   // True if issue() has been called.
   bool getIssued(void) const;
   
   // True if the fence has been submitted to D3D by the worker thread.
   bool getSubmitted(void) const;
   
   // True if the fence is still pending, false if the fence has been passed by the GPU. 
   bool getPending(void) const;
   
   // Returns immediately if the fence was not issued.
   void blockUntilSubmitted(void) const;
   
   // Returns immediately if the fence was not issued.
   void blockWhilePending(void) const;
               
private:
   volatile uint64 mIndex;             // writable by worker thread only, read by main thread
   uint32 mIssueCount;                 // readable/writable by main thread only
};

#pragma warning(pop)

// These macros ensure the current thread matches either the main or worker thread.
#ifdef BUILD_DEBUG
   #define ASSERT_MAIN_THREAD             BDEBUG_ASSERT(GetCurrentThreadId() == gRenderThread.getMainThreadId());
   #define ASSERT_NOT_MAIN_THREAD         BDEBUG_ASSERT(GetCurrentThreadId() != gRenderThread.getMainThreadId());
   #define ASSERT_RENDER_THREAD           BDEBUG_ASSERT(GetCurrentThreadId() == gRenderThread.getWorkerThreadId());
   #define ASSERT_MAIN_OR_WORKER_THREAD   BDEBUG_ASSERT( (GetCurrentThreadId() == gRenderThread.getMainThreadId()) || (GetCurrentThreadId() == gRenderThread.getWorkerThreadId()) );
#else
   #define ASSERT_MAIN_THREAD
   #define ASSERT_NOT_MAIN_THREAD
   #define ASSERT_RENDER_THREAD
   #define ASSERT_MAIN_OR_WORKER_THREAD
#endif

class BRenderCommandListener;

#pragma warning(push)
#pragma warning(disable:4324) // pad warning

//============================================================================
// class BRenderThread
// Most public methods of BRenderThread can be called from the main thread(s). 
// The exception are those that begin with "worker".
// Some methods can be called from the main or worker threads. 
// A few can be called from arbitrary threads.
// IMPORTANT: The worker thread can NEVER block (either directly or indirectly) on any resource held/managed by the
// main thread. This could introduce a deadlock condition if the main thread fills up the command FIFO, which would block
// the main thread until segments are available. Both threads would then wait forever on each other. The worker
// thread must always make forward progress through the command FIFO, or sleep if there's nothing to do. (It's OK, but not ideal,
// if the worker thread blocks on a resource in use by the GPU. Ideally the main and worker threads rarely if ever block due
// to resource contentions.)
//============================================================================
__declspec(align(128))
class BRenderThread : public BEventReceiverInterface
{ 
   friend class BRenderGPUFence; 

public:
   BRenderThread();
   virtual ~BRenderThread();
         
   struct BD3DDeviceInfo
   {
      IDirect3DDevice9*       mpDev;
      D3DPRESENT_PARAMETERS   mD3DPP; 
      IDirect3DBaseTexture9*  mpDevFrontBuffer;
      IDirect3DSurface9*      mpDevBackBuffer;
      IDirect3DSurface9*      mpDevDepthStencil;   
   };

   // init and deinit can be called from the main thread only.
   // init will block until D3D is initialized before returning.
   // 0's set the frame storage defaults.
   bool init(const BD3D::BCreateDeviceParams& initParams, BD3DDeviceInfo* pDeviceInfo = NULL, bool panicMode = false);
   bool deinit(void);
   
   // Releases the worker thread's ownership of the D3D device. This is a very slow operation.
   // The main thread will own the D3D device.
   IDirect3DDevice9* releaseThreadOwnership(void);
   
   // Releases the D3D device from the main thread, then causes the worker thread to acquire the D3D device. This is a very slow operation.
   void acquireThreadOwnership(void);
   
   // These functions start and stop the mini load screen rendering mode.  When the render thread is in this mode,
   // it will automatically render the mini load screen and call present.  This occurs within the main command loop.
   void startMiniLoadRendering(void);
   void stopMiniLoadRendering(void);

   void dirtyDiskSuspendOwnershipDirect(void);

   // Callable from the worker thread only. Returns true if init() has been called.
   bool getInitialized(void) const { return mInitialized; }
   
   // Callable from any thread.
   DWORD getMainThreadId(void) const { return mMainThreadId; }
   DWORD getWorkerThreadId(void) const { return mWorkerThreadId; }
   
   HANDLE getWorkerThreadHandle(void) const { return mThreadHandle; }
   
   // Callable from any thread.
   BWin32Event& getD3DReadyEvent(void) { return mD3DReadyEvent; }   
   BWin32Event& getProcessingCommandsEvent(void) { return mProcessingCommandsEvent; }
   
   // Callable from the main thread.
   void blockUntilD3DReady(void);
   void blockUntilProcessingCommands(void);
   
   void beginLevelLoad(void);
   void endLevelLoad(void);
   bool isInsideLevelLoad() const { return mInsideLevelLoad; }
   
   // Submits a command into the command FIFO. The command data will be copied directly into the command buffer.
   // Submits are callable from the main thread only.
   // The commandClass can either be one of the command classes defined in renderCommand.h, or a command listener handle returned by registerCommandListener().
   void submitCommand(DWORD commandClass, DWORD commandType, DWORD dataLen, const void* pData);
   void submitCommand(DWORD commandClass, DWORD commandType) { return submitCommand(commandClass, commandType, 0, NULL); }
      
   // This places a bitwise copy of the object into the command buffer.
   template<typename T> 
   void submitCommand(DWORD commandClass, DWORD commandType, const T& obj) { submitCommand(commandClass, commandType, sizeof(obj), &obj); }
   
   // Limitations: commandClass must be 16-bits, dataLen must be < segment length
   void* submitCommandBegin(DWORD commandClass, DWORD commandType, DWORD dataLen);
   void submitCommandEnd(DWORD dataLen);

   // Submits a command with aligned data.
   // Important: When you receive the command on the worker thread, the data pointer will NOT be aligned for you. 
   // You must align it yourself.
   // Don't bother calling if alignment is <= sizeof(DWORD), DWORD alignment is always guaranteed by the unaligned submit methods.
   void* submitCommandBegin(DWORD commandClass, DWORD commandType, DWORD dataLen, DWORD alignment);
   void submitCommandEnd(DWORD dataLen, DWORD alignment);
   
   // These methods are the same as above, except they call listener.getCommandHandle() for you and pass that for commandClass.
   void submitCommand(BRenderCommandListener& listener, DWORD commandType, DWORD dataLen, const void* pData);
   void submitCommand(BRenderCommandListener& listener, DWORD commandType);
   template<typename T> 
   void submitCommand(BRenderCommandListener& listener, DWORD commandType, const T& obj) { submitCommand(listener.getCommandHandle(), commandType, sizeof(obj), &obj); }
   void* submitCommandBegin(BRenderCommandListener& listener, DWORD commandType, DWORD dataLen);
   
   // This will copy construct the object (which MUST be derived from the 
   // BRenderCommandObjectInterface interface) directly into the command buffer.
   // The object's destructor will NOT be called automatically after the command is processed.
   // If needed, call it yourself at the end of your processCommand() method.
   // TODO: Implement RCO method with dynamic variable length data allocation from the command buffer.
   template<typename T>
   void submitCopyOfCommandObject(const T& obj, DWORD data = 0);
   
   template<typename T>
   void submitPtrToCommandObject(const T& obj, DWORD data = 0);
   
   // Submits a callback with a data pointer. The data must remain stable/valid until the callback is executed on the worker thread.
   void submitCallback(BRenderCommandCallbackPtr pCallback, const void* pData = NULL);
   
   // Submits a callback with associated data that is copied into the command buffer.
   void submitCallbackWithData(BRenderCommandCallbackPtr pCallback, DWORD dataLen, const void* pData, DWORD alignment = 4);
      
   void submitDataCopy(void* pDst, const void* pSrc, uint len);
   void* submitDataCopyBegin(void* pDst, uint len);
   void submitDataCopyEnd(uint len);
   
   void submitDeferredDataCopy(void* pDst, const void* pSrc, uint len);
      
   typedef GF::Functor<void, TYPELIST_1(void*)> BFunctor;
   // This will copy construct the functor object directly in the command buffer, which may be expensive. 
   // The worker thread will in-place destruct the functor.
   void submitFunctor(const BFunctor& functor, void* pData = NULL);
   
   template<typename T>
   void submitFunctorWithObject(const BFunctor& functor, const T& obj, uint alignment = ALIGN_OF(T));
                        
   // Returns the maximum size of a command's data.
   static uint getMaxCommandDataSize(void) { return cSegmentSize - sizeof(BRenderCommandHeader) - sizeof(DWORD); }
   
   // Kicks off all queued commands to the worker thread. (This is done automatically at intervals.)
   // This is only callable from the main thread!
   void kickCommands(void);
         
   // Frame begin must be called before any rendering is done. 
   // This method will block to prevent the main thread from getting too far ahead of the worker thread.
   // Frame storage is recycled here.
   void frameBegin(void);
   
   // Call frame end after submitting all rendering commands.
   // The current "main" thread is incremented here.
   // If dispatchEvents is true, dispatchEvents() will be called to dispatch render events before ending the frame.
   void frameEnd(bool dispatchEvents = true, bool dispatchSynchronousEvents = true);
   
   bool isInFrame(void) const;
   
   // The NoBarrier() version can be called from the main thread only. The barrier version uses an import barrier to get the most up to date frame index.
   DWORD getCurMainFrame(void) const { MemoryBarrier(); return mCurMainFrame; }    
   DWORD getCurMainFrameNoBarrier(void) const { return mCurMainFrame; }    
      
   // These can be called from any thread, but they are only snapshots!
   // Constraints: curMainFrame >= curWorkerFrame >= curGPUFrame
   // The DWORD will roll over at 30Hz after ~4.5 years of execution.
   DWORD getCurWorkerFrame(void) const { MemoryBarrier(); return mCurWorkerFrame; }
   DWORD getCurWorkerFrameNoBarrier(void) const { return mCurWorkerFrame; }
   
   DWORD getCurGPUFrame(void) const { MemoryBarrier(); return mCurGPUFrame; }
   DWORD getCurGPUFrameNoBarrier(void) const { return mCurGPUFrame; }
      
   // -- Worker Thread Fences --
   
   // Inserts a render thread fence (not a GPU fence) into the command buffer. 
   // This does not kick off queued commands.
   uint insertWorkerFence(void);
   
   // Returns index of current fence.
   uint getCurrentWorkerFence(void) const { MemoryBarrier(); return mCurWorkerFence; }
   
   bool isWorkerFencePending(uint fenceIndex) const { return static_cast<int>(getCurrentWorkerFence() - fenceIndex) < 0; }
         
   void blockOnWorkerFence(uint fenceIndex);
   
   // Very expensive!
   void blockUntilWorkerIdle(void);
   
   // -- GPU Fences --
   // These GPU fences are implemented using DPC time callbacks, not through the D3D fence API.
   // These fences allow you to wait on an event object, the D3D fence API does not (it wants to wait for you).
   
   // Main/worker threads.
   void blockUntilGPUIdle(void);
   
   // Render thread
   // Blocks until the GPU passes the fence inserted into the command buffer after the previous call to processPresent().
   void blockUntilGPUPresent(void);
      
   // Inserts a D3DCALLBACK_IMMEDIATE GPU callback into the command buffer that will set the specified 
   // Win-32 event object. The event object cannot be destroyed until the GPU passes the callback!
   // Main/worker threads.
   void insertGPUEventFence(HANDLE eventHandle);
      
   // Returns the thread specific Win-32 event object that gets signaled whenever the GPU frame changes.
   // Main/worker threads.
   BWin32Event& getGPUFrameChangedEvent(void);
   
   // This GPU fence API uses DPC callbacks instead of D3D fences.
   // Note that the returned fence indices/Win-32 event objects are thread 
   // specific and cannot be shared between threads.
   // Main/worker threads.
   DWORD issueGPUFence(void);
   DWORD getCurGPUFence(void);
   DWORD getCurGPUFenceNoBarrier(void);
   bool isGPUFencePending(DWORD index);
   bool isGPUFencePendingNoBarrier(DWORD index);
   void blockOnGPUFence(DWORD index);
   BWin32Event& getGPUFenceChangedEvent(void);
   
   // Blocks until the GPU has passed the specified frame, or until the GPU has idled.
   // The event dispatcher handles the wait, so events can still be dispatched.
   void blockOnGPUFrameOrIdle(DWORD frame);
   
   // Returns true if the GPU frame has passed the specified frame. If this method
   // returns false, blockOnGPUFrameOrIdle() will block on the specified frame.
   bool hasGPUFrameBeenPassed(DWORD frame);
      
   BOOL isSimThread(void) const { return GetCurrentThreadId() == mMainThreadId; }
   BOOL isRenderThread(void) const { return GetCurrentThreadId() == mWorkerThreadId; }
            
   // Submits a sleep command to the worker thread.
   void sleep(uint ms);

   // Causes the worker thread to dump the D3D state.
   void dumpState(void);

   // Submits a CPU breakpoint command.
   void breakPoint(void);

   // Can be called from any thread on a fatal error. Used by the worker thread.         
   void __declspec(noreturn) panic(const char* pMsg, ...);
      
   void queueTermination(void) { Sync::InterlockedExchangeExport(&mTerminate, TRUE); }
   
   // Conceptually, the returned memory will be recycled on the next call to frameBegin(). 
   // So it may be safely referenced by any commands in the current frame only.
   // Callable from the main thread only.
   // Returns NULL if frame storage is full if failIfFull==false.
   void* allocateFrameStorage(uint len, uint alignment = sizeof(DWORD), bool failIfFull = true, bool simThread = true);

   // Allocate frame storage from worker thread.
   // Returns NULL if frame storage is full if failIfFull==false.
   void* workerAllocateFrameStorage(uint len, uint alignment = sizeof(DWORD), bool failIfFull = true) { return allocateFrameStorage(len, alignment, failIfFull, false); }
                  
   template<class T, bool construct> T* allocateFrameStorageObj(uint num = 1, const T& obj = T(), uint alignment = (__alignof(T) ? __alignof(T) : sizeof(DWORD)));
   template<class T, bool construct> T* workerAllocateFrameStorageObj(uint num = 1, const T& obj = T(), uint alignment = (__alignof(T) ? __alignof(T) : sizeof(DWORD)));
      
   // rg [2/17/06] - The minimum alignment for GPU frame storage is always cGPUFrameStorageMinAlignment. If the requested alignment is greater than 
   // this, the actual allocation will be quite wasteful unless len is large relative to the requested alignment.
   
   // Callable from main thread.
   // Be sure to invalidate the GPU cache's view of this memory!
   // Returns NULL if frame storage is full if failIfFull==false.
   void* allocateGPUFrameStorage(uint len, uint alignment = cGPUFrameStorageMinAlignment, bool failIfFull = true, bool simThread = true);
   
   // Callable from worker thread.
   // Be sure to invalidate the GPU cache's view of this memory!
   // Returns NULL if frame storage is full if failIfFull==false.
   void* workerAllocateGPUFrameStorage(uint len, uint alignment = cGPUFrameStorageMinAlignment, bool failIfFull = true) { return allocateGPUFrameStorage(len, alignment, failIfFull, false); }
   
   // Returns the maximum amount of frame storage remaining. Note that the returned value is only a snapshot!
   uint getAvailableFrameStorage(void);
   uint workerGetAvailableFrameStorage(void);
   uint getAvailableGPUFrameStorage(void);
   uint workerGetAvailableGPUFrameStorage(void);

   bool isFrameAvailable(void) const {return mNumFramesRemaining > 0;}

#if 0      
   void* getGPUFrameStorageBase(bool cachedReadOnlyView = false);
   void* workerGetGPUFrameStorageBase(bool cachedReadOnlyView = false);
   uint getGPUFrameStorageSize(void) const { return mGPUFrameStorageSize; }   
#endif   
               
   // Registers a render command listener. The handle is a valid commandClass for submit().
   // You must call freeCommandListener() before the object pointed to be pListenerObject goes out of scope!
   // Only callable from the main thread.           
   // This will queue up a call to initDeviceObjects() on your interface on the worker object.
   BCommandListenerHandle registerCommandListener(BRenderCommandListenerInterface* pListenerObject);
   
   // Callable from the main or worker threads.
   // freeCommandListener calls blockUntilIdle() - do not use this often.
   // Before returning, this will call the deinitDeviceObjects() method on your object on the worker thread.
   void freeCommandListener(BCommandListenerHandle handle);
   
   // Callable from the worker thread only.
   uint getMaxCommandListeners(void) const;
   
   // Callable from the worker thread only.
   BRenderCommandListenerInterface* getCommandListenerByIndex(uint index) const;
     
   // Callable from main thread only.
   bool getInFrame(void) const;

   bool getHasD3DOwnership(void) const;
   
   // Callable from worker thread - call after present().
   void processPresent(void);
      
   // Free memory allocated by BAlignedAlloc::Free from the worker thread.
   // Callable from any thread, but the free can be queued to be freed in the worker thread from the main thread.
   void deferredAlignedFree(void* pData);
   
   template<class T> void deferredAlignedDelete(T* p);
   
   // Allocates an array of 1 or more GPU fences.
   // Callable from any thread.
   BRenderGPUFence* allocGPUFences(uint numFences);
   
   // Submits a command to free the GPU fences on the worker thread.
   // Callable from main or worker threads only.
   void deleteGPUFences(BRenderGPUFence* pFences);
   
   void setWin32Event(HANDLE eventHandle);
   void waitOnHandle(HANDLE waitHandle);
   void setWin32EventAndWait(HANDLE eventHandle, HANDLE waitHandle);
      
   BEventReceiverHandle getMainEventReceiverHandle(void) const { return mMainEventReceiverHandle; }
   BEventReceiverHandle getWorkerEventReceiverHandle(void) const { return mWorkerEventReceiverHandle; }
   
   // The getting of single step is not thread safe, but it shouldn't matter because it's only for hardcore debugging.
   void setSingleStep(bool singleStep) { mSingleStep = singleStep; }
   bool getSingleStep(void) const { return mSingleStep; }
         
#ifndef BUILD_FINAL
   // These methods issue PIX calls on the worker thread. 
   // The name strings are copied into the command buffer.
   // Callable from main thread. 
   void submitPIXBeginNamedEvent(const char* pName);
   void submitPIXEndNamedEvent(void);
   void submitPIXSetMarker(const char* pName);
#endif

#ifndef BUILD_FINAL   
   struct BStats
   {
      uint mNumCommands;
      uint mNumCommandKicks;
      uint mCommandBytes;
      uint mSegmentsOpened;
            
      uint mCPUFrameStorageBytes;
      uint mMainCPUFrameStorageAllocs;
      
      uint mGPUFrameStorageBytes;
      uint mMainGPUFrameStorageAllocs;
   }; 
   
   void getStats(BStats& stats);      
   void setMinFrameTime(DWORD time) {mMinFrameTime = time;}
   DWORD getMinFrameTIme() const {return(mMinFrameTime);}                                                       
#endif   

#if 0
   void beginLoad();
   void endLoad();
#endif   
      
private:
   // cMaxQueuedGPUFrames was 3, but that was causing blocking after I increased the # of frames D3D lets us get ahead to 2.
   //enum { cMaxQueuedFrames = 2, cMaxQueuedGPUFrames = 4 };
   enum { cMaxQueuedFrames = 2, cMaxQueuedGPUFrames = 3 };

   BD3D::BCreateDeviceParams mD3DInitParams;
      
   DWORD mMainThreadId;
   DWORD mWorkerThreadId;
   HANDLE mThreadHandle;

#ifndef BUILD_FINAL   
   BStats mStats;
   void clearStats(void);
#endif   
         
   uint mNextWorkerFence;
   BWin32Event mFenceChangedEvent;
      
   BWin32Event mCurWorkerFrameChanged;
   
   BWin32Event mCurGPUFrameChangedMain;
   BWin32Event mCurGPUFrameChangedWorker;
      
   BWin32Event mGPUFenceSubmittedEvent;
   
   // Private main/worker fences are implemented using DPC callbacks, to work around problems with the crappy D3D fence API.
   DWORD mNextGPUMainFenceIndex;
   DWORD mNextGPUWorkerFenceIndex;
   BWin32Event mGPUMainFenceIndexChanged;
   BWin32Event mGPUWorkerFenceIndexChanged;
      
   uint64 mLastPresentFence;
   
   DWORD mCurMainFrame;                  
   
   enum { cNumSegments = 64, cSegmentSize = 8192 };

   struct BSegment
   {
      uchar mData[cSegmentSize];
   };

   typedef BCommandFIFO<BSegment, cNumSegments> BCommandBuffer;
   
   uchar* mpCmdFIFOSegment;
   DWORD mCmdFIFOSegmentOfs;

   BWin32Event mProcessingCommandsEvent;
   BWin32Event mD3DReadyEvent;
   BWin32Semaphore mFramesRemaining;
   volatile LONG mNumFramesRemaining;
   
   BLightWeightMutex       mFrameStorageMutex[cMaxQueuedFrames];
   BBlockStackAllocator    mFrameStorageAllocators[cMaxQueuedFrames];
   uint                    mFrameStorageFences[cMaxQueuedFrames];     // read/write by main thread
   uint                    mFrameStorageCurMainIndex;    // only read/written by main thread
   uint                    mFrameStorageCurWorkerIndex;  // only read/written by worker thread
               
   enum 
   {
      cGPUFrameStorageMinAlignment = 32,
      cGPUFrameStorageMinAlignmentMask = cGPUFrameStorageMinAlignment - 1
   };
            
   BLightWeightMutex       mGPUFrameStorageMutex[cMaxQueuedGPUFrames];
   // [11/14/2008 xemu] temp exposing this for CSV mem logging 
public:
   BBlockStackAllocator    mGPUFrameStorageAllocators[cMaxQueuedGPUFrames];

private:
   BRenderGPUFence*        mpGPUFrameStorageFences;    // read/write by main thread
   uint                    mGPUFrameStorageCurMainIndex;   // read/written by main thread
   uint                    mGPUFrameStorageCurWorkerIndex; // read/written by worker thread
      
   BEventReceiverHandle    mMainEventReceiverHandle;
   BEventReceiverHandle    mWorkerEventReceiverHandle;

#ifndef BUILD_FINAL
   LARGE_INTEGER           mFrameStartTime;
   DWORD                   mMinFrameTime;
#endif   
   
#ifdef BUILD_DEBUG
   bool                    mSubmittingCommand : 1;
   DWORD                   mSubmittingCommandDataLen;
#endif   

   bool                    mInitialized : 1;
   bool                    mInFrame : 1;
   bool                    mPanicMode : 1;
   bool                    mSingleStep : 1;
   bool                    mHasD3DOwnership : 1;
   bool                    mInMiniLoadRenderingMode : 1;
   bool                    mInsideLevelLoad : 1;
   
   __declspec(align(128)) BCommandBuffer mCmdFIFO;
                        
   // 128 byte aligned variables follow.
   // All vars that are the targets of interlocked ops are placed here to avoid perf. issues with non-interlocked variables
   // being accessed in the same cache line as interlocked variables.
   __declspec(align(128)) volatile LONG mFrameStorageCurOfs[cMaxQueuedFrames];     // changed by both threads
   __declspec(align(128)) volatile LONG mGPUFrameStorageCurOfs[cMaxQueuedGPUFrames]; // changed by both threads
        
   enum { cCommandListenerSlotOccupiedPtr = 1 };      
   
   class BInitCommandListenerRCO : public BRenderCommandObjectInterface
   {
   public:
      BInitCommandListenerRCO(BRenderCommandListenerInterface* pListener) : mpListener(pListener) { }

      virtual void processCommand(DWORD data) const 
      { 
         MemoryBarrier();
         BDEBUG_ASSERT(*(DWORD*)data == BRenderThread::cCommandListenerSlotOccupiedPtr);
         Sync::InterlockedExchangeExport((LONG*)data, (LONG)mpListener); 
         mpListener->initDeviceData();
      }
   private:
      BRenderCommandListenerInterface* mpListener;
   };

   class BDeinitCommandListenerRCO : public BRenderCommandObjectInterface
   {
   public:
      virtual void processCommand(DWORD data) const 
      { 
         MemoryBarrier();
         (*(BRenderCommandListenerInterface**)data)->deinitDeviceData(); 
         Sync::InterlockedExchangeExport((LONG*)data, NULL); 
      }
   };
   
   BLightWeightMutex mCommandListenerMutex;
         
   // mpCommandListeners contains pointers to the command listeners for the frame being rendered by the WORKER thread.
   // Pointers set to cCommandListenerSlotOccupiedPtr indicate slots that have been reserved for use by a future frame.
   // The worker thread manipulates this array, not the main thread, except for reserving slots.
   // This way, we can always consult this array in the worker thread to determine the active listeners.
   enum { cMaxCommandListeners = 64 };
   __declspec(align(128)) BRenderCommandListenerInterface* volatile mpCommandListeners[cMaxCommandListeners];
   
   __declspec(align(128)) volatile DWORD mCurGPUFrame;
   
   __declspec(align(128)) volatile DWORD mCurWorkerFence;  
   __declspec(align(128)) volatile DWORD mCurWorkerFrame;
         
   __declspec(align(128)) volatile DWORD mCurGPUMainFenceIndex;
   __declspec(align(128)) volatile DWORD mCurGPUWorkerFenceIndex;
         
   __declspec(align(128)) volatile LONG mD3DReadyFlag;
   __declspec(align(128)) volatile LONG mProcessingCommandsFlag;      
   
   // mTerminate can be used to shut down the worker thread. 
   // It's always checked at least once a second, even if the command buffer blows up.
   __declspec(align(128)) volatile LONG mTerminate;
   
   __declspec(align(128)) volatile LONG mShuttingDown;
               
   // methods
            
   static DWORD __stdcall workerThreadFunc(void* pData);
   
   bool workerThread(void);

   bool initD3D(void);
   bool deinitD3D(void);
   bool initGPUFrameStorage(void);
   bool deinitGPUFrameStorage(void);
   bool initAllFrameStorage(void);
   bool deinitAllFrameStorage(void);
   void recycleFrameStorage(void);
   void recycleGPUFrameStorage(void);
   
   bool commandLoop(void);
   void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   void processRenderCommand(const BRenderCommandHeader& header, const uchar* pData);
   void deinitAllCommandListeners(void);
   void processControlCommand(const BRenderCommandHeader& header, const uchar* pData);
   void waitUntilFrameAvailable(void);
   void waitUntilD3DFenceSubmitted(const BRenderGPUFence& fence);
   void beginProducerSegment(void);
   const uchar* waitForConsumerSegment(DWORD timeout);
   static DWORD createXAllocAttr(void);
   static uint getFreeMemory(void);

   static void gpuFrameCallback(DWORD curFrame);
   static void gpuEventCallback(DWORD context);
   static void gpuMainFenceCallback(DWORD fenceIndex);
   static void gpuWorkerFenceCallback(DWORD fenceIndex);
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};
#pragma warning(pop)

//============================================================================
// externs
//============================================================================
extern BRenderThread gRenderThread;

//============================================================================
// class BRenderCommandListener
// Helper class to simplify the creation of classes that process commands.
//============================================================================
class BRenderCommandListener : public BRenderCommandListenerInterface
{
public:
   inline BRenderCommandListener() : 
      mCommandHandle(cInvalidCommandListenerHandle)
   {
   }

   inline void commandListenerInit(void)
   {
      if (cInvalidCommandListenerHandle == mCommandHandle)
         mCommandHandle = gRenderThread.registerCommandListener(this);
   }

   // If you call this from the render thread, BE SURE the sim thread is not currently (and will not in the future) be submitting commands to your handle!!
   inline void commandListenerDeinit(void)
   {
      if (cInvalidCommandListenerHandle != mCommandHandle)
      {
         gRenderThread.freeCommandListener(mCommandHandle);
         mCommandHandle = cInvalidCommandListenerHandle;
      }
   }
   
   inline BCommandListenerHandle getCommandHandle(void) const { return mCommandHandle; }

protected:
   BCommandListenerHandle mCommandHandle;
};

//============================================================================
// struct BScopedPIXNamedEventRender
// Only callable from the main thread! Use BScopedPIXNamedEvent on the render thread.
//============================================================================
struct BScopedPIXNamedEventRender
{
   BScopedPIXNamedEventRender(const char* pName, DWORD color = 0xFFFFFFFF)
   {
#ifndef BUILD_FINAL   
      PIXBeginNamedEvent(color, pName);
      gRenderThread.submitPIXBeginNamedEvent(pName);
#endif      
   }

   ~BScopedPIXNamedEventRender()
   {
#ifndef BUILD_FINAL   
      PIXEndNamedEvent();
      gRenderThread.submitPIXEndNamedEvent();
#endif      
   }
};

#include "renderThread.inl"

