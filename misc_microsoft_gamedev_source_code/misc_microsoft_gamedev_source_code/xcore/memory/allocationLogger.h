//==============================================================================
// allocationLogger.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#pragma push_macro("XPhysicalAlloc")
#pragma push_macro("XPhysicalAllocEx")
#pragma push_macro("XPhysicalFree")
#pragma push_macro("malloc")
#pragma push_macro("calloc")
#pragma push_macro("free")
#pragma push_macro("realloc")
#pragma push_macro("_expand")

#undef XPhysicalAlloc
#undef XPhysicalAllocEx
#undef XPhysicalFree
#undef malloc
#undef calloc
#undef free
#undef realloc
#undef _expand

#include "allocationtracker.h"
#include "threading\eventDispatcher.h"
#include "containers\queue.h"
#include "allocationLoggerPackets.h"
#include "stream\bufferStream.h"


// Allocation logging filters.  Put it outside of ifdef so the code is less ugly when setting
// the logging filter for the BMemoryHeaps.
enum
{
   cAllocLogTypePhysical      = 0x01,
   cAllocLogTypePrimary       = 0x02,
   cAllocLogTypeRender        = 0x04,
   cAllocLogTypeSim           = 0x08,
   cAllocLogTypeParticles     = 0x10,
   cAllocLogTypeOther         = 0x20,
   cAllocLogTypeAll           = 0xFF
};

#if defined(XBOX) && defined(ALLOCATION_LOGGER)

//==============================================================================
// class BAllocationLogger
//==============================================================================
class BAllocationLogger : public BEventReceiver
{
   BAllocationLogger(const BAllocationLogger&);
   BAllocationLogger& operator= (const BAllocationLogger&);
   
public:
   // Sim thread.
   BAllocationLogger();
   ~BAllocationLogger();
   
   void init(BStream* pStream);
   void init(const char* ipAddress);
   // Avoid calling deinit() when there are active worker threads that could allocate!
   void deinit(void);
   
   bool getInitialized(void) const { return mInitialized; }
   BStream* getStream(void) const { return mpStream; }   
   
   // Don't call enableTracking() if there are active worker threads that could allocate!
   void enableTracking(bool enabled);
   bool getTrackingEnabled(void) const { return mEnabled; }
      
   // Used to filter out certain types of allocations
   uchar getLoggingTypeFilter() const          { return mLoggingTypeFilter; }
   void  setLoggingTypeFilter(uchar filter)    { mLoggingTypeFilter = filter; }
         
   // Callable from any thread.
   // IMPORTANT: Caller must have claimed the logger's lock by calling claimLock() before calling any of these methods!
   void logNew(void* pHeap, uint size, void* pBlock, uint blockSize, uchar loggingType);
   void logResize(void* pHeap, void* pOrigBlock, uint newSize, void* pNewBlock, uchar loggingType);
   void logDelete(void* pHeap, void* pBlock, uchar loggingType);
   
   // It's OK to call these methods without claiming the logger's lock.
   void registerHeap(const char* pName, void* pHeap, bool physicalFlag);
   void logSnapshot(uint index);
   void logFrame(uint index);
   void logIgnoreLeaf(const char* pName);
       
   inline void enableRemoteLogging(bool onOff) { mRemoteEnabled = onOff; }
   
   bool writeTrackerStats(const char *filename, bool snapshot);
   void clearTrackerSnapshot();
         
private:
#ifdef ENABLE_ALLOCATION_TRACKER
   BAllocationTracker   mTracker;      // jce [11/17/2008] -- adding simple tracking for current status
#endif

   BStream*             mpStream;
      
   // Read/written by misc thread
   struct BPacketBuf
   {
      enum { cBufSize = cALMaxPacketSize };
      uchar mBuf[cBufSize];
   };
   
   BLightWeightMutex    mLoggerLock;

   typedef BQueue<BPacketBuf> BPacketQueue;
   enum { cPacketQueueSize = 16384U };
   void*                mpRawPacketQueue;
   int                  mRawPacketQueueByteCount;
   BPacketQueue*        mpPacketQueue;
         
   BWin32WaitableTimer  mTimer;
   
   enum { cOutputBufferSize = 256U*1024U };
   BYTE*                mpOutputBuffer;
   uint                 mOutputBufferOfs;

   SOCKET               mSocket;          //for remote tracking
   bool                 mRemoteEnabled;   //for remote tracking
   void*                mpRawFlushPacketQueue;
         
   void                 sendPacket(void* pData, uint dataSize);
   
   uint                 mLockCount;
   
   uint                 mNumDroppedPackets;
   
   DWORD                mNextPacketIndex;

   bool                 mStreamError : 1;
   bool                 mInitialized : 1;
   bool                 mEnabled : 1;
   uchar                mLoggingTypeFilter;
   
   enum cEventClass
   {
      cECInit = cEventClassFirstUser,
      cECDeinit,
      cECService
   };
   
   volatile LONG mWorkerThreadActive;

   bool flushPacket(const BALPacketBase* pPacket);
   void captureContext(BALContext& context);
   void sendVersion(void);
   void sendEOF(void);
   void flushQueuedPackets(void);
   
   inline void claimLock() { mLoggerLock.lock(); mLockCount++; }
   inline void releaseLock() { BDEBUG_ASSERT(mLockCount > 0); mLockCount--; mLoggerLock.unlock(); }
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

BAllocationLogger& getAllocationLogger();

#endif

#pragma pop_macro("XPhysicalAlloc")
#pragma pop_macro("XPhysicalAllocEx")
#pragma pop_macro("XPhysicalFree")
#pragma pop_macro("malloc")
#pragma pop_macro("calloc")
#pragma pop_macro("free")
#pragma pop_macro("realloc")
#pragma pop_macro("_expand")
