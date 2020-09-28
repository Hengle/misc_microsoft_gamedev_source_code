//============================================================================
//
//  renderCommand.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// Render command classes
enum eRCClass
{
   cRCCNop,
   cRCCControl,
   
   cRCCCommandObjCopy,
   cRCCCommandObjPtr,
   cRCCCommandCallback,
   cRCCCommandCallbackWithData,
   cRCCCommandFunctor,

#ifndef BUILD_FINAL
   // cRCCBreakPoint triggers a call to DebugBreak() in the worker thread.
   cRCCBreakPoint,

   // cRCCDumpState calls the device state dumper to dump the current D3D state.
   cRCCDumpState,
#endif   

   // Causes the worker thread to sleep by the specified number of ms (uint param).
   cRCCSleep,

   // Frees aligned memory allocated with the BAlignedAlloc::Malloc method.
   cRCCFreeAlignedMemory,
   
   // Causes all queued worker thread events to be dispatched through the gEventDispatcher.
   // This also happens at the end of a frame.
   cRCCDispatchEvents,

   // Sets the supplied Win-32 event object.
   cRCCSetWin32Event,
      
   // Calls WaitForSingleObject() on the supplied HANDLE object. Does NOT dispatch events during this time.
   cRCCWaitOnHandle,
   
   // Calls SetEvent on the first supplied handle, then calls WaitForSingleObject on the second supplied handle.
   // Two handles must be inserted into the command stream. Any handle that is INVALID_HANDLE_OBJECT is ignored.
   cRCCSetWin32EventAndWait,
   
   cRCCGPUEventFence,
   
   cRCCGPUMainFence,

#ifndef BUILD_FINAL   
   cRCCPIXBeginNamedEvent,
   cRCCPIXEndNamedEvent,
   cRCCPIXSetMarker,
#endif   

   cRCCDataCopy,
         
   cRCCMax
};

// Render commands
enum eRCCommands
{
   cRCFrameBegin = 100,
   cRCFrameEnd,      

   cRCKickPushBuffer,
   cRCWorkerFence,
   cRCGPUFence,
   
   cRCChangeFrameStorageIndex,
   cRCChangeGPUFrameStorageIndex,
   
   cRCExit,
   
   cRCBeginLevelLoad,
   cRCEndLevelLoad,

   cRCStartMiniLoad,
   cRCStopMiniLoad,

   cRTMax
};

struct BRenderCommandHeader
{
   WORD mClass;
   WORD mLen;
   DWORD mType;
};

class __declspec(novtable) BRenderCommandObjectInterface
{
public:
   // This method will be directly called by the renderer's worker thread.
   virtual void processCommand(DWORD data) const = 0;
};

typedef void (*BRenderCommandCallbackPtr)(void* pData);

class BRenderCommandListenerInterface
{
public:
   // init will be called from the worker thread after the D3D device is initialized.
   virtual void initDeviceData(void) { }
   
   // Called from worker thread.
   virtual void frameBegin(void) { }
   
   // Called from the worker thread to process commands.
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData) { }
   
   // Called from worker thread.
   virtual void frameEnd(void) { }
   
   // deinit will be called from the worker thread before the RCL is freed, but always before the D3D device is release.
   virtual void deinitDeviceData(void) { }
   
   // Called at the start of a scenario load. Free any work allocations that are only required for rendering here.
   // Guaranteed that the sim is idling during this call.
   virtual void beginLevelLoad(void) { }
   
   // Called at the end of a scenario load. Reallocate any work allocations that are required for rendering here.
   // Guaranteed that the sim is idling during this call.
   virtual void endLevelLoad(void) { }
};

enum { cInvalidCommandListenerHandle = 0 };
typedef DWORD BCommandListenerHandle;

