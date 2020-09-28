//==============================================================================
// allocationLogger.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#include "xcore.h"

#undef XPhysicalAlloc
#undef XPhysicalAllocEx
#undef XPhysicalFree
#undef malloc
#undef calloc
#undef free
#undef realloc
#undef _expand


#include "allocationLogger.h"
#include "xdb\xdbManager.h"
#include "threading\eventDispatcher.h"



#if defined(XBOX) && defined(ALLOCATION_LOGGER)

//#define ALLOCATION_LOGGER_DEBUG
//#define ALLOCATION_LOGGER_DEBUG_IMMED

const uint cRemoteHeapDumpSize = 16384;

static uint64 gAllocationLogger[(sizeof(BAllocationLogger) + sizeof(uint64) - 1) / sizeof(uint64)];
static uchar gAllocationLoggerInitialized;

BAllocationLogger& getAllocationLogger()
{
   // This check is not thread safe, but it doesn't need to be.
   if (!gAllocationLoggerInitialized)
   {
      BASSERT(gAllocationLoggerInitialized != 1);
      gAllocationLoggerInitialized = 1;
      Utils::ConstructInPlace<BAllocationLogger>((BAllocationLogger*)gAllocationLogger);
      gAllocationLoggerInitialized = 2;
   }
   
   return *reinterpret_cast<BAllocationLogger*>(gAllocationLogger);
}

#ifdef ALLOCATION_LOGGER_DEBUG
#include "hash\hash.h"

const cHashTableSize = 512*1024;
uint gHashTable[cHashTableSize];

static bool hashAdd(void* p)
{
   uint c = hashFast(&p, sizeof(p)) % cHashTableSize;
   
   for ( ; ; )
   {
      if (gHashTable[c] == (uint)p)
         return false;
      else if ((gHashTable[c] == 0xFFFFFFFF) || (gHashTable[c] == 0))
      {
         gHashTable[c] = (uint)p;
         break;
      }
            
      c++;
      if (c == cHashTableSize) c = 0;
   }
   return true;
}

static bool hashDel(void* p)
{
   uint c = hashFast(&p, sizeof(p)) % cHashTableSize;

   for ( ; ; )
   {
      if (gHashTable[c] == (uint)p)
      {
         gHashTable[c] = 0xFFFFFFFF;
         break;
      }
      else if (gHashTable[c] == 0)
         return false;

      c++;
      if (c == cHashTableSize) c = 0;
   }
   return true;
}

static void checkPacket(void* p)
{
   static DWORD curPacketIndex;
      
   BALPacketBase* pBase = static_cast<BALPacketBase*>(p);
   
   BASSERT(pBase->mPacketPrefix == cALPacketPrefix);
   
   //if (pBase->mPacketIndex != curPacketIndex)
   //   DebugBreak();
      
   curPacketIndex++;

   switch (pBase->mPacketType)
   {
      case cALNew:
      {
         BALPacketNew* pNew = static_cast<BALPacketNew*>(p);
         if (!hashAdd(pNew->mpBlock))
         {
            DebugBreak();
         }
         break;
      }
      case cALResize:
      {
         BALPacketResize* pResize = static_cast<BALPacketResize*>(p);
         if (!hashDel(pResize->mpOrigBlock))
         {
            trace("Warning: Stray resize detected: 0x%08X", pResize->mpOrigBlock);
         }
         if (!hashAdd(pResize->mpNewBlock))
         {
            DebugBreak();
         }
         break;
      }
      case cALDelete:
      { 
         BALPacketDelete* pDel = static_cast<BALPacketDelete*>(p);
         if (!hashDel(pDel->mpBlock))
         {
            trace("Warning: Stray delete detected: 0x%08X", pDel->mpBlock);
         }
         break;
      }
   }
}
#endif

//==============================================================================
// BAllocationLogger::BAllocationLogger
// WARNING: This method may be called before the CRT finishes constructing 
// global objects! DO NOT allocate any memory from tracked heaps here!
//==============================================================================
BAllocationLogger::BAllocationLogger() :
   mpStream(NULL),
   mpRawPacketQueue(NULL),
   mpPacketQueue(NULL),
   mpRawFlushPacketQueue(NULL),
   mInitialized(false),
   mEnabled(false),
   mLoggingTypeFilter(cAllocLogTypeAll),
   mStreamError(false),
   mWorkerThreadActive(FALSE),
   mRemoteEnabled(false),
   mSocket(INVALID_SOCKET),
   mLockCount(0),
   mNextPacketIndex(0),
   mpOutputBuffer(NULL),
   mOutputBufferOfs(0)
{
   if (!mEnabled)
      enableTracking(true);
}

//==============================================================================
// BAllocationLogger::~BAllocationLogger
//==============================================================================
BAllocationLogger::~BAllocationLogger()
{
}

//==============================================================================
// BAllocationLogger::enableTracking
// WARNING: This method may be called before the CRT finishes constructing 
// global objects! DO NOT allocate any memory from tracked heaps here!
//==============================================================================
void BAllocationLogger::enableTracking(bool enabled) 
{ 
   BScopedLightWeightMutex lock(mLoggerLock);
   
   mEnabled = enabled; 
         
   if (mEnabled)
   {
      if (!mpPacketQueue)
      {
         const uint cAllocAlignment = 128;
         mpRawPacketQueue = malloc(sizeof(BPacketQueue) + cAllocAlignment - 1);
         BPacketQueue* p = static_cast<BPacketQueue*>(Utils::AlignUp(mpRawPacketQueue, cAllocAlignment));
         
         new (p) BPacketQueue(cPacketQueueSize, &gCRunTimeHeap);
         
         mpPacketQueue = p;
      }
      if (!mpRawFlushPacketQueue)
      {
         if (mRemoteEnabled)
         {
            mpRawFlushPacketQueue = malloc(cRemoteHeapDumpSize);
         }
      }
   }
   else
   {
      if (mpPacketQueue)
      {  
         gEventDispatcher.sleep(250);
         
         BPacketQueue* pPacketQueue = mpPacketQueue;
         void* pRawPacketQueue = mpRawPacketQueue;
         
         // This is playing with fire - a worker thread could try to queue up an allocation packet while we're deleting!
         // So do this in stages.
         mpPacketQueue = NULL;
         mpRawPacketQueue = NULL;
                  
         gEventDispatcher.sleep(250);
         
         Utils::DestructInPlace(pPacketQueue);
         
         free(pRawPacketQueue);
      }
      
      if (mRemoteEnabled)
      {
         gEventDispatcher.sleep(250);

         void* pRawPacketQueue = mpRawFlushPacketQueue;

         // This is playing with fire - a worker thread could try to queue up an allocation packet while we're deleting!
         // So do this in stages.
         mpPacketQueue = NULL;
         mpRawPacketQueue = NULL;

         gEventDispatcher.sleep(250);

         free(pRawPacketQueue);
      }
   }
}

//==============================================================================
// BAllocationLogger::init
//==============================================================================
void BAllocationLogger::init(const char* ipAddress)
{
   ASSERT_THREAD(cThreadIndexSim);

   deinit();

   if (mRemoteEnabled)
   {
      SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      if (s == INVALID_SOCKET)
      {
         BASSERT(0);
         return;
      }

      const WORD cAllocLogPort = 2887;

      SOCKADDR_IN sa;
      memset(&sa, 0, sizeof(sa));
      sa.sin_family = AF_INET;
      sa.sin_port = htons(cAllocLogPort);
      sa.sin_addr.s_addr = inet_addr(ipAddress);//()"10.10.36.71");

      if (connect(s, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR)
      {
         int error = WSAGetLastError();
         trace("connect error: %d", error);
         closesocket(s);
         mInitialized = false;
         return;
      }

      mSocket = s;
   }

   mInitialized = true;

   mpStream = NULL;

   eventReceiverInit(cThreadIndexMisc);

   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = mEventHandle;
   handleEvent.mToHandle = mEventHandle;
   handleEvent.mEventClass = cECService;
   gEventDispatcher.registerHandleWithEvent(mTimer.getHandle(), handleEvent);
         
   mNextPacketIndex = 0;

   mTimer.set(100);
   
   gEventDispatcher.waitUntilThreadQueueEmpty(cThreadIndexMisc);

   sendVersion();
}

//==============================================================================
// BAllocationLogger::init
//==============================================================================
void BAllocationLogger::init(BStream* pStream)
{
   ASSERT_THREAD(cThreadIndexSim);
   BDEBUG_ASSERT(pStream);
   
   mRemoteEnabled = false;

   deinit();
   
   mInitialized = true;
            
   mpStream = pStream;
   
   mpOutputBuffer = (BYTE*)malloc(cOutputBufferSize);
   mOutputBufferOfs = 0;
            
   eventReceiverInit(cThreadIndexMisc);
            
   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = mEventHandle;
   handleEvent.mToHandle = mEventHandle;
   handleEvent.mEventClass = cECService;
   gEventDispatcher.registerHandleWithEvent(mTimer.getHandle(), handleEvent);

   mTimer.set(100);
   
   sendVersion();
}

//==============================================================================
// BAllocationLogger::deinit
//==============================================================================
void BAllocationLogger::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   if (!mInitialized)
      return;
      
   mTimer.cancel();
   
   gEventDispatcher.sleep(250);
         
   sendEOF();
   
   gEventDispatcher.send(mEventHandle, mEventHandle, cECDeinit, 0, 0, NULL, BEventDispatcher::cSendWaitForDelivery);
         
   eventReceiverDeinit();   
   
   gEventDispatcher.sleep(250);
           
   BDEBUG_ASSERT(!mWorkerThreadActive);
   
   if (mpOutputBuffer)
   {
      if ((mOutputBufferOfs) && (!mStreamError) && (mpStream))
         mpStream->writeBytes(mpOutputBuffer, mOutputBufferOfs);
         
      free(mpOutputBuffer);
      mpOutputBuffer = NULL;
      mOutputBufferOfs = 0;
   }
         
   mStreamError = false;
   mInitialized = false;
   
   MemoryBarrier();

   if (mRemoteEnabled)
   {
      if (mSocket != INVALID_SOCKET)
      {
         //  sendDisconnect();

         closesocket(mSocket);
         mSocket = INVALID_SOCKET;
      }
   }
}

//==============================================================================
// BAllocationLogger::fillContext
//==============================================================================
void BAllocationLogger::captureContext(BALContext& context)
{
   context.mThreadIndex = static_cast<BYTE>(gEventDispatcher.getThreadIndex());
   
   QueryPerformanceCounter((LARGE_INTEGER*)&context.mTime);

   BXStackTrace stackTrace;
   stackTrace.capture();

   const int numLevelsToSkip = 2;
   const int numLevels = Math::Clamp<int>(stackTrace.getNumLevels() - numLevelsToSkip, 0, BALContext::cMaxBackTrace);

   Utils::ClearObj(context.mBackTrace);

   context.mBackTraceSize = static_cast<BYTE>(numLevels);
   memcpy(context.mBackTrace, stackTrace.getBackTraceArrayPtr() + numLevelsToSkip, numLevels * sizeof(DWORD));
}

//==============================================================================
// BAllocationLogger::flushPacket
//==============================================================================
bool BAllocationLogger::flushPacket(const BALPacketBase* pPacket)
{
   BDEBUG_ASSERT(mLockCount);
   
   if (!mpPacketQueue)
   {
      mNumDroppedPackets++;
      return false;
   }
   
   BPacketBuf buf;
   BDEBUG_ASSERT(pPacket->mPacketSize <= BPacketBuf::cBufSize);
   memcpy(&buf, pPacket, pPacket->mPacketSize);
   
   //((BALPacketBase*)&buf)->mPacketIndex = mNextPacketIndex;

   const uint cMaxTries = 8;
   
   uint loopCount;
   for (loopCount = 0; loopCount < cMaxTries; loopCount++)
   {
      if (mpPacketQueue->pushBack(buf))
      {
         mNextPacketIndex++;
         break;
      }
                     
      if (!mWorkerThreadActive)
      {
         //trace("ERROR: BAllocationLogger::flushPacket: Packet queue full before init!");
         mNumDroppedPackets++;
         return false;
      }
      
      flushQueuedPackets();
   }

#ifndef BUILD_FINAL   
   if (loopCount == cMaxTries)
   {
      trace("ERROR: BAllocationLogger::flushPacket: Packet queue full!");
      mNumDroppedPackets++;
      return false;
   }
#endif   

   return true;
}

//==============================================================================
// BAllocationLogger::sendVersion
//==============================================================================
void BAllocationLogger::sendVersion(void)
{
   BALPacketVersion packet;
      
   packet.mVersion = cALStreamVersion;
   packet.mXEXChecksum = gXDBManager.getInitialized() ? gXDBManager.getXEXChecksum() : 0;
   packet.mXEXBaseAddress = gXDBManager.getXEXInfo().getValid() ? gXDBManager.getXEXInfo().getBaseAddress() : 0;
   QueryPerformanceFrequency((LARGE_INTEGER*)&packet.mTimerFreq);

   claimLock();   
   
   flushPacket(&packet);
   
   releaseLock();
}

//==============================================================================
// BAllocationLogger::sendEOF
//==============================================================================
void BAllocationLogger::sendEOF(void)
{
   BALPacketEOF packet;

   claimLock();   
   
   flushPacket(&packet);
   
   releaseLock();
}

//==============================================================================
// BAllocationLogger::registerHeap
//==============================================================================
void BAllocationLogger::registerHeap(const char* pName, void* pHeap, bool physicalFlag)
{
   if (!mEnabled)
      return;
            
   BALPacketRegisterHeap packet;
      
   packet.mPtr = pHeap;
   packet.mFlags = physicalFlag ? BALPacketRegisterHeap::cFlagPhysical : 0;
   packet.mName.set(pName);   
 
   claimLock();     
   
   flushPacket(&packet);   
   
   releaseLock();
}

//==============================================================================
// BAllocationLogger::logNew
//==============================================================================
void BAllocationLogger::logNew(void* pHeap, uint size, void* pBlock, uint blockSize, uchar loggingType)
{
   #ifdef ENABLE_ALLOCATION_TRACKER
   mTracker.trackNew(pBlock, blockSize, 0);
   #endif
   
   if (!mEnabled)
      return;
   if ((mLoggingTypeFilter & loggingType) == 0)
      return;
            
   BALPacketNew packet;
      
   packet.mpHeap = pHeap;
   packet.mSize = size;
   packet.mpBlock = pBlock;
   packet.mBlockSize = blockSize;
   
   captureContext(packet.mContext);
   
   claimLock();
   
   flushPacket(&packet);
   
   releaseLock();

   //trace("NEW 0x%08X", pBlock);
   
#ifdef ALLOCATION_LOGGER_DEBUG_IMMED
   if (!hashAdd(pBlock))
      DebugBreak();
#endif
}

//==============================================================================
// BAllocationLogger::logResize
//==============================================================================
void BAllocationLogger::logResize(void* pHeap, void* pOrigBlock, uint newSize, void* pNewBlock, uchar loggingType)
{
   #ifdef ENABLE_ALLOCATION_TRACKER
   mTracker.trackResize(pOrigBlock, pNewBlock, newSize);
   #endif

   if (!mEnabled)
      return;
   if ((mLoggingTypeFilter & loggingType) == 0)
      return;
      
   BALPacketResize packet;
   
   packet.mpHeap = pHeap;
   packet.mpOrigBlock = pOrigBlock;
   packet.mNewSize = newSize;
   packet.mpNewBlock = pNewBlock;

   captureContext(packet.mContext);

   claimLock();
   
   flushPacket(&packet);
   
   releaseLock();

   //trace("RESIZE 0x%08X 0x%08X", pOrigBlock, pNewBlock);
   
#ifdef ALLOCATION_LOGGER_DEBUG_IMMED
   if (!hashDel(pOrigBlock))
      trace("Stray resize: 0x%08X", pOrigBlock);
   if (!hashAdd(pNewBlock))
      DebugBreak();
#endif   
}

//==============================================================================
// BAllocationLogger::logDelete
//==============================================================================
void BAllocationLogger::logDelete(void* pHeap, void* pBlock, uchar loggingType)
{
   #ifdef ENABLE_ALLOCATION_TRACKER
   mTracker.trackDelete(pBlock);
   #endif

   if (!mEnabled)
      return;
   if ((mLoggingTypeFilter & loggingType) == 0)
      return;

   BALPacketDelete packet;
   
   packet.mpHeap = pHeap;
   packet.mpBlock = pBlock;
   
   captureContext(packet.mContext);

   claimLock();
   
   flushPacket(&packet);
   
   releaseLock();

   //trace("DELETE 0x%08X", pBlock);
   
#ifdef ALLOCATION_LOGGER_DEBUG_IMMED
   if (!hashDel(pBlock))
      trace("Stray delete: 0x%08X", pBlock);
#endif
}

//==============================================================================
// BAllocationLogger::logSnapshot
//==============================================================================
void BAllocationLogger::logSnapshot(uint index)
{
   if (!mEnabled)
      return;
      
   BALPacketSnapshot packet;
   packet.mIndex = index;

   claimLock();
   
   flushPacket(&packet);
   
   releaseLock();
}

//==============================================================================
// BAllocationLogger::logFrame
//==============================================================================
void BAllocationLogger::logFrame(uint index)
{
   if (!mEnabled)
      return;
      
   BALPacketFrame packet;
   packet.mIndex = index;
   QueryPerformanceCounter((LARGE_INTEGER*)&packet.mTime);

   claimLock();
      
   flushPacket(&packet);
   
   releaseLock();
}

//==============================================================================
// BAllocationLogger::logIgnoreLeaf
//==============================================================================
void BAllocationLogger::logIgnoreLeaf(const char* pName)
{
   if (!mEnabled)
      return;
      
   BALPacketIgnoreLeaf packet;
   packet.mSymbol.set(pName);

   claimLock();
   
   flushPacket(&packet);
   
   releaseLock();
}

//==============================================================================
// BAllocationLogger::sendPacket
//==============================================================================
void BAllocationLogger::sendPacket(void* pData, uint dataSize)
{
   if ((!mRemoteEnabled) || (pData == NULL) || (dataSize == 0))
      return;
   
   if (mSocket == INVALID_SOCKET)
   {
      trace("WARNING: BAllocationLogger::sendPacket: Not connected!");
      return;
   }

   int totalBytesLeft = dataSize;
   char* pDest = reinterpret_cast<char*>(mpRawFlushPacketQueue);
   while (totalBytesLeft > 0)
   {
      int retval = send(mSocket, pDest, totalBytesLeft, 0);
      if (retval == 0 || retval == SOCKET_ERROR)
      {
         mStreamError = true;
         return;
      }

      totalBytesLeft -= retval;
      pDest+= retval;
   }
}

//==============================================================================
// BAllocationLogger::flushQueuedPackets
//==============================================================================
void BAllocationLogger::flushQueuedPackets(void)
{
   BDEBUG_ASSERT(mLockCount);
   
   if (!mpPacketQueue)
      return;

   BScopedLightWeightMutex lock(mLoggerLock);      
   
   BPacketBuf packetBuf;
   const BALPacketBase* pPacketBase = reinterpret_cast<const BALPacketBase*>(&packetBuf);
   
   BOOL result = mpPacketQueue->popFront(packetBuf);
   if (!result)
      return;

#ifdef ALLOCATION_LOGGER_DEBUG      
   checkPacket(&packetBuf);
#endif   
      
   if (mRemoteEnabled)
   {
      const uint cMaxPacketSize = cRemoteHeapDumpSize;
      Utils::FastMemSet(mpRawFlushPacketQueue, 0, cMaxPacketSize);
      char* pDest = reinterpret_cast<char*>(mpRawFlushPacketQueue);
      uint numBytesUsed = 0;

      for ( ; ; )
      {
         if (numBytesUsed + pPacketBase->mPacketSize > cMaxPacketSize)
         {
            if (!mStreamError)
            {
               sendPacket(mpRawFlushPacketQueue, numBytesUsed);
            }

            numBytesUsed = 0;
            pDest = reinterpret_cast<char*>(mpRawFlushPacketQueue);
         }

         memcpy(pDest, pPacketBase, pPacketBase->mPacketSize);
         pDest += pPacketBase->mPacketSize;
         numBytesUsed += pPacketBase->mPacketSize;
         
         BOOL result = mpPacketQueue->popFront(packetBuf);
         if (!result)
            break;
         
#ifdef ALLOCATION_LOGGER_DEBUG                  
         checkPacket(&packetBuf);
#endif         
      }

      if (!mStreamError && numBytesUsed!=0)
      {
         sendPacket(mpRawFlushPacketQueue, numBytesUsed);
      }
   }
   else
   {
      for ( ; ; )
      {
         if ((!mStreamError) && (mpOutputBuffer))
         {
            if ((mOutputBufferOfs + pPacketBase->mPacketSize) > cOutputBufferSize)
            {
               if (mpStream)
               {
                  if (mpStream->writeBytes(mpOutputBuffer, mOutputBufferOfs) != mOutputBufferOfs)
                     mStreamError = true;
               }
                                    
               mOutputBufferOfs = 0;
            }
               
            memcpy(mpOutputBuffer + mOutputBufferOfs, pPacketBase, pPacketBase->mPacketSize);
            mOutputBufferOfs += pPacketBase->mPacketSize;
         }

         BOOL result = mpPacketQueue->popFront(packetBuf);
         if (!result)
            break;
         
#ifdef ALLOCATION_LOGGER_DEBUG                  
         checkPacket(&packetBuf);
#endif         
      }
   }
}

//==============================================================================
// BAllocationLogger::receiveEvent
//==============================================================================
bool BAllocationLogger::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassClientAdded:
      {
         InterlockedExchange(&mWorkerThreadActive, TRUE);
         break;
      }
      case cEventClassClientRemove:
      case cEventClassThreadIsTerminating:
      {
         InterlockedExchange(&mWorkerThreadActive, FALSE);
         break;
      }
      case cECInit:
      {
         break;
      }
      case cECDeinit:
      {
         claimLock();
         
         flushQueuedPackets();
                           
         InterlockedExchange(&mWorkerThreadActive, FALSE);
         
         releaseLock();
         
         break;
      }
      case cECService:
      {
         claimLock();
         
         flushQueuedPackets();
         
         releaseLock();
         
         break;
      }
   }      

   return false;
}


//==============================================================================
//==============================================================================
bool BAllocationLogger::writeTrackerStats(const char *filename, bool snapshot)
{
   #ifdef ENABLE_ALLOCATION_TRACKER
      return(mTracker.writeStats(filename, snapshot));
   #else
      filename;
      return(false);
   #endif
}

//==============================================================================
//==============================================================================
void BAllocationLogger::clearTrackerSnapshot()
{
   #ifdef ENABLE_ALLOCATION_TRACKER
      mTracker.clearSnapshot();
   #endif
}


#else
uint gAllocationLoggerDummy;
#endif
