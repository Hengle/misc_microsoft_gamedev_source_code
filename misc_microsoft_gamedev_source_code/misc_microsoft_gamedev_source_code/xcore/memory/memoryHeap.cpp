//=============================================================================
//
//  memoryHeap.cpp
//
//  Copyright (c) 2006, Ensemble Studios
//
//=============================================================================
#include "xcore.h"
#include "allocationLogger.h"
#include "alignedAlloc.h"
#include "math\randomUtils.h"
#include "memory\dlmalloc.h"
#include "timer.h"

//#define GATHER_HEAP_LOCK_TIME

#ifdef XBOX 
   #define ROCKALL_ENABLED 1
#else
   #define ROCKALL_ENABLED 0
#endif

#ifdef BUILD_DEBUG
   #define FILL_ALLOCATED_BLOCKS_WITH_NOISE 1
#else
   #define FILL_ALLOCATED_BLOCKS_WITH_NOISE 0
#endif

#if ROCKALL_ENABLED
   #if defined(BUILD_DEBUG)
      #pragma comment(lib, "rockall_debug.lib")
      #pragma comment(lib, "heap_debug.lib")
      #pragma comment(lib, "library_debug.lib")
   #elif defined(BUILD_PROFILE)
      #pragma comment(lib, "rockall_profile.lib")
      #pragma comment(lib, "heap_profile.lib")
      #pragma comment(lib, "library_profile.lib")
   #elif defined(BUILD_FINAL) && defined(LTCG)
      #pragma comment(lib, "rockall_releaseLTCG.lib")
      #pragma comment(lib, "heap_releaseLTCG.lib")
      #pragma comment(lib, "library_releaseLTCG.lib")
   #else
      #pragma comment(lib, "rockall_release.lib")
      #pragma comment(lib, "heap_release.lib")
      #pragma comment(lib, "library_release.lib")
   #endif

   #include "../extlib/rockall/code/rockall/interface/DebugHeap.hpp"
   #include "../extlib/rockall/code/rockall/interface/FastHeap.hpp"
   #include "../extlib/rockall/code/rockall/interface/SmallHeap.hpp"
   #include "../extlib/rockall/code/rockall/interface/BlendedHeap.hpp"
   #include "../extlib/rockall/code/rockall/interface/DefaultHeap.hpp"
   #include "../extlib/rockall/code/rockall/interface/PhysicalHeap.hpp"
#endif

// We've modified Rockall to always guarantee up to 16 byte alignment, including 
// the debug heap. (However, size must be a multiple of the desired alignment 
// for this to be guaranteed!)
const uint cRockallHeapAlignmentGuarantee          = 16;
const uint cRockallPhysicalHeapAlignmentGuarantee  = 4096;

static DWORD gRandomFillSeed;

//=============================================================================
// BMemoryHeapStats::BMemoryHeapStats
//=============================================================================
BMemoryHeapStats::BMemoryHeapStats(const char* pName)
{
   clear();
   if (pName)
      strcpy_s(mName, sizeof(mName), pName);
   else
      mName[0] = '\0';
}

//=============================================================================
// BMemoryHeapStats::sum
//=============================================================================
void BMemoryHeapStats::sum(const BMemoryHeapStats& d)
{
   mCurrentAllocations     += d.mCurrentAllocations;
   mCurrentAllocationSize  += d.mCurrentAllocationSize;
   mMostAllocations        += d.mMostAllocations;
   mMostAllocationSize     += d.mMostAllocationSize;
   mTotalAllocations       += d.mTotalAllocations;
   mTotalAllocationSize    += d.mTotalAllocationSize;
   mTotalNews              += d.mTotalNews;
   mTotalDeletes           += d.mTotalDeletes;
   mTotalReallocations     += d.mTotalReallocations; 
   mTotalLockTime          += d.mTotalLockTime;
   mTotalLocks             += d.mTotalLocks;

   for (uint i = 0; i < cMaxThreads; i++)
      mThreadHist[i] += d.mThreadHist[i];
}

//=============================================================================
// BMemoryHeapStats::createDelta
//=============================================================================
void BMemoryHeapStats::createDelta(const BMemoryHeapStats& d)
{
   mCurrentAllocations     -= d.mCurrentAllocations;
   mCurrentAllocationSize  -= d.mCurrentAllocationSize;
   mMostAllocations        -= d.mMostAllocations;
   mMostAllocationSize     -= d.mMostAllocationSize;
   mTotalAllocations       -= d.mTotalAllocations;
   mTotalAllocationSize    -= d.mTotalAllocationSize;
   mTotalNews              -= d.mTotalNews;
   mTotalDeletes           -= d.mTotalDeletes;
   mTotalReallocations     -= d.mTotalReallocations; 
   mTotalLockTime          -= d.mTotalLockTime;
   mTotalLocks             -= d.mTotalLocks;

   for (uint i = 0; i < cMaxThreads; i++)
      mThreadHist[i] -= d.mThreadHist[i];
}     

//=============================================================================
// BMemoryHeapStats::trackNew
//=============================================================================
void BMemoryHeapStats::trackNew(int actualSize)
{
   mCurrentAllocations++;
   mCurrentAllocationSize += actualSize;

   mMostAllocations = max(mMostAllocations, mCurrentAllocations);
   mMostAllocationSize = max(mMostAllocationSize, mCurrentAllocationSize);

   mTotalAllocations++;
   mTotalAllocationSize += actualSize;

   mTotalNews++;
   
   const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   if ((threadIndex + 1) < cMaxThreads)
      mThreadHist[threadIndex + 1]++;
}

//=============================================================================
// BMemoryHeapStats::trackDelete
//=============================================================================
void BMemoryHeapStats::trackDelete(int actualSize)
{
   mCurrentAllocations--;
   mCurrentAllocationSize -= actualSize;

   mTotalDeletes++;
   
   const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   if ((threadIndex + 1) < cMaxThreads)
      mThreadHist[threadIndex + 1]++;
}

//=============================================================================
// BMemoryHeapStats::trackRealloc
//=============================================================================
void BMemoryHeapStats::trackRealloc(int origSize, int newSize)
{
   int delta = newSize - origSize;

   mCurrentAllocationSize += delta;
   mMostAllocationSize = max(mMostAllocationSize, mCurrentAllocationSize);

   mTotalReallocations++;  
   
   const BThreadIndex threadIndex = gEventDispatcher.getThreadIndex();
   if ((threadIndex + 1) < cMaxThreads)
      mThreadHist[threadIndex + 1]++;
}

//=============================================================================
// BMemoryHeap::BMemoryHeap
//=============================================================================
BMemoryHeap::BMemoryHeap() :
   mHeapType(cInvalidHeapType),
   mThreadSafe(false),
   mMaxSupportedAlignment(sizeof(DWORD)),
   mpHeap(NULL),
   mOwnerThread(0),
   mpInterceptor(NULL),
   mDeinitOnDestruction(false),
   mpMSpace(NULL),
   mAllocLogType(cAllocLogTypeAll)
{
   // Warning: Don't do anything fancy in this constructor, it can be called very early in our init process!
   
#ifdef MEMORYHEAP_USE_CS   
#ifdef XBOX   
   InitializeCriticalSectionAndSpinCount(&mMutex, 4096);
#else   
   InitializeCriticalSection(&mMutex);
#endif   
#endif
   
#if FILL_ALLOCATED_BLOCKS_WITH_NOISE
   if (!gRandomFillSeed)
      gRandomFillSeed = (DWORD)RandomUtils::GenerateRandomSeed();
#endif
}

//=============================================================================
// BMemoryHeap::BMemoryHeap
//=============================================================================
BMemoryHeap::BMemoryHeap(eHeapType heapType, bool threadSafe, const char* pName, bool writeCombined, BMemoryHeapInterceptor* pInterceptor, bool deinitOnDestruction, unsigned char allocLogType) :
   mHeapType(cInvalidHeapType),
   mThreadSafe(false),
   mMaxSupportedAlignment(sizeof(DWORD)),
   mpHeap(NULL),
   mOwnerThread(0),
   mpInterceptor(NULL),
   mDeinitOnDestruction(false),
   mpMSpace(NULL),
   mAllocLogType(allocLogType)
{
   // Warning: Don't do anything fancy in this constructor, it can be called very early in our init process!
   pName;
   writeCombined;
   
#ifdef MEMORYHEAP_USE_CS   
#ifdef XBOX   
   InitializeCriticalSectionAndSpinCount(&mMutex, 4096);
#else   
   InitializeCriticalSection(&mMutex);
#endif   
#endif
               
   init(heapType, threadSafe, pName, writeCombined, pInterceptor, deinitOnDestruction, allocLogType);

#if FILL_ALLOCATED_BLOCKS_WITH_NOISE
   if (!gRandomFillSeed)
      gRandomFillSeed = (DWORD)RandomUtils::GenerateRandomSeed();
#endif
}

//=============================================================================
// BMemoryHeap::~BMemoryHeap
//=============================================================================
BMemoryHeap::~BMemoryHeap()
{
   if (mDeinitOnDestruction)
      deinit();
}

//=============================================================================
// BMemoryHeap::init
//=============================================================================
void BMemoryHeap::init(eHeapType heapType, bool threadSafe, const char* pName, bool writeCombined, BMemoryHeapInterceptor* pInterceptor, bool deinitOnDestruction, unsigned char allocLogType)
{
   writeCombined;
   
   deinit();
      
   mHeapType = heapType;
   mThreadSafe = threadSafe;
   mMaxSupportedAlignment = sizeof(DWORD);
   mpHeap = NULL;
   mOwnerThread = 0;
   mpInterceptor = pInterceptor;
   mDeinitOnDestruction = deinitOnDestruction;
   mpMSpace = NULL;
   mAllocLogType = allocLogType;
#ifdef TRACK_HEAP_STATS
   mStats.clear();
   strcpy_s(mStats.mName, sizeof(mStats.mName), pName);
#endif   

   switch (mHeapType)
   {
      case cCRunTimeHeap: 
      {
         mpHeap = NULL;
         mMaxSupportedAlignment = 16;   
         break;
      }
      case cDLMallocHeap:
      {
         mpHeap = NULL;
         mMaxSupportedAlignment = 16;
         
         mpMSpace = create_mspace(0, 0);
         BVERIFY(mpMSpace);
         break;
      }
#if ROCKALL_ENABLED      
      case cDebugHeap: 
      {
         mMaxSupportedAlignment = cRockallHeapAlignmentGuarantee;
         mpHeap = (DEBUG_HEAP*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEBUG_HEAP));
         BVERIFY(mpHeap);
         new (static_cast<void*>(mpHeap)) DEBUG_HEAP(0, false, false, false);
         break;
      }
      case cFastHeap:
      {
         mMaxSupportedAlignment = cRockallHeapAlignmentGuarantee;
         mpHeap = (FAST_HEAP*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FAST_HEAP));
         BVERIFY(mpHeap);
         new (static_cast<void*>(mpHeap)) FAST_HEAP((2 * HalfMegabyte), true, false, false);
         break;
      }
      case cPrimaryHeap:
      case cRenderHeap:
      case cSmallHeap:
      {
         mMaxSupportedAlignment = cRockallHeapAlignmentGuarantee;
         mpHeap = (SMALL_HEAP*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SMALL_HEAP));
         BVERIFY(mpHeap);
         new (static_cast<void*>(mpHeap)) SMALL_HEAP(0, false, false, false);
         break;
      }
      case cBlendedHeap: 
      {
         mMaxSupportedAlignment = cRockallHeapAlignmentGuarantee;
         mpHeap = (BLENDED_HEAP*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BLENDED_HEAP));
         BVERIFY(mpHeap);
         new (static_cast<void*>(mpHeap)) BLENDED_HEAP(HalfMegabyte, false, false, false);
         break;
      }
      case cPhysicalHeap:
      {
         mMaxSupportedAlignment = cRockallPhysicalHeapAlignmentGuarantee;
         mpHeap = (PHYSICAL_HEAP*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PHYSICAL_HEAP));
         BVERIFY(mpHeap);
         new (static_cast<void*>(mpHeap)) PHYSICAL_HEAP(256 * 1024, false, false, false, writeCombined);
         break;
      }
      case cSingleRockallHeap:
      {
         mMaxSupportedAlignment = cRockallHeapAlignmentGuarantee;
         // Intended for testing only.
         static SMALL_HEAP singleRockallHeap;
         mpHeap = &singleRockallHeap;
         break;
      }
#endif      
      default:
      {
         BVERIFY(false);
      }
   }

#if ROCKALL_ENABLED      
   if (mpHeap)
      mpHeap->SetOwnerThread(0);
#endif      
   
#ifdef ALLOCATION_LOGGER
   getAllocationLogger().registerHeap(pName, this, (mHeapType == cPhysicalHeap));
#endif   
}
//=============================================================================
// BMemoryHeap::deinit
//=============================================================================
void BMemoryHeap::deinit()
{
   claimLock();

#if ROCKALL_ENABLED   
   if (mpHeap)
   {
      if (mHeapType != cSingleRockallHeap)
      {
         Utils::DestructInPlace(mpHeap);
         HeapFree(GetProcessHeap(), 0, mpHeap);
         mpHeap = NULL;
      }         
   }
   else 
#endif   
   if (mpMSpace)
   {
      destroy_mspace((mspace)mpMSpace);
      mpMSpace = NULL;
   }
   else
   {
      // nothing to do - CRT
   }
   
   mHeapType = cInvalidHeapType;
   
   releaseLock();
}

//=============================================================================
// BMemoryHeap::setInterceptor
//=============================================================================
void BMemoryHeap::setInterceptor(BMemoryHeapInterceptor* pInterceptor)
{
   claimLock();

   mpInterceptor = pInterceptor;

   releaseLock();
}

//=============================================================================
// BMemoryHeap::claimLock
//=============================================================================
void BMemoryHeap::claimLock(void) 
{ 
   if (mThreadSafe) 
   {
#if !defined(BUILD_FINAL) && defined(GATHER_HEAP_LOCK_TIME)
      BTimer timer;
      timer.start();
#endif   

#ifdef MEMORYHEAP_USE_CS         
      EnterCriticalSection(&mMutex); 
#else
      mMutex.lock(2048);
#endif      
      
#if defined(TRACK_HEAP_STATS) && defined(GATHER_HEAP_LOCK_TIME)
      mStats.mTotalLockTime += timer.getElapsedSeconds();
      mStats.mTotalLocks++;
#endif         
   }
   else
   {
      BDEBUG_ASSERT(!mOwnerThread || (GetCurrentThreadId() == mOwnerThread));
   }
}

//=============================================================================
// BMemoryHeap::releaseLock
//=============================================================================
void BMemoryHeap::releaseLock(void) 
{ 
   if (mThreadSafe) 
   {
#ifdef MEMORYHEAP_USE_CS      
      LeaveCriticalSection(&mMutex);
#else
      mMutex.unlock();
#endif
   }
}   

//=============================================================================
// BMemoryHeap::New
//=============================================================================
void* BMemoryHeap::New(int size, int* actualSize, bool zero, bool tryInterceptorOnly)
{
   BDEBUG_ASSERT(mHeapType != cInvalidHeapType);
   
   if (size < sizeof(DWORD))
      size = sizeof(DWORD);
      
#if CLEAR_ALL_ALLOCATED_BLOCKS
   zero = true;
#endif

   void* p = NULL;
   
   claimLock();
         
   if (mpInterceptor)
   {
      p = mpInterceptor->New(size, actualSize, zero);
      
      // jce [11/19/2008] -- Elsewhere we insist on the conditions that a valid allocation pointer is 
      // DWORD aligned and that it is not less than 0x10000 (however NULL is ok), so I'm checking this here
      // for sanity.
      BASSERT((p == NULL || (uint)p >= 0x10000) && (((uint)p) & 3) == 0);
   }
         
   if ((!p) && (!tryInterceptorOnly))
   {
#if ROCKALL_ENABLED
      if (mpHeap)
      {
         p = mpHeap->New(size, actualSize, zero);
      }
      else
#endif
      if (mpMSpace)
      {
         p = mspace_malloc(mpMSpace, size);
         if (p)
         {
            const int allocatedSize = mspace_usable_size(p);
            
            if (actualSize)
               *actualSize = allocatedSize;
            if (zero)
               Utils::FastMemSet(p, 0, allocatedSize);
         }
      }
      else
      {
         p = alignedMalloc(size);
         if (p)
         {
            const int allocatedSize = alignedMSize(p);
            
            if (actualSize) 
               *actualSize = allocatedSize;
            if (zero)
               Utils::FastMemSet(p, 0, allocatedSize);
         }         
      }      
   }      
   
   if (!p)
   {
      releaseLock();
      return NULL;
   }
   
   int blockSize = 0;
   blockSize;
  
#if defined(TRACK_HEAP_STATS) || defined(ALLOCATION_LOGGER)
   const bool success = DetailsInternal(p, &blockSize);
   BVERIFY(success);
   BDEBUG_ASSERT(blockSize >= size);
#endif
   
#ifdef TRACK_HEAP_STATS
   mStats.trackNew(blockSize);
#endif      

//   uint64 t; QueryPerformanceCounter((LARGE_INTEGER*)&t);
//   trace("NEW: 0x%08X Size: %u TIME: %u", p, blockSize, (uint)t);

#ifdef ALLOCATION_LOGGER
   getAllocationLogger().logNew(this, size, p, blockSize, mAllocLogType);
#endif 

   releaseLock();
   
#ifndef BUILD_FINAL
   if (mMaxSupportedAlignment > sizeof(DWORD))
   {
      if ((size & 15) == 0)
      {
         BASSERT(Utils::IsAligned(p, 16));
      }
      else if ((size & 7) == 0)
      {
         BASSERT(Utils::IsAligned(p, 8));
      }
   }
#endif

#if FILL_ALLOCATED_BLOCKS_WITH_NOISE
   if (!zero)
   {
      // This is not thread safe, but it shouldn't really matter, right?
      gRandomFillSeed = RandomUtils::RandomFill32(p, size, gRandomFillSeed);
   }
#endif

   // jce [11/19/2008] -- Elsewhere we insist on the conditions that a valid allocation pointer is 
   // DWORD aligned and that it is not less than 0x10000 (NULL should have been returned already), so I'm checking this here
   // for sanity.
   BASSERT( (uint)p >= 0x10000 && (((uint)p) & 3) == 0);

   return p;
}

//=============================================================================
// BMemoryHeap::AlignedNew
//=============================================================================
void* BMemoryHeap::AlignedNew (int size, int alignment, int* actualSize, bool zero, bool tryInterceptorOnly)
{
   BDEBUG_ASSERT(mHeapType != cInvalidHeapType);
   BASSERT((alignment >= 1) && Math::IsPow2(alignment));
   BASSERT(alignment <= (int)mMaxSupportedAlignment);
   
   if (size < sizeof(DWORD))
      size = sizeof(DWORD);
    
   size = Utils::AlignUpValue(size, alignment);
   
   void* p = New(size, actualSize, zero, tryInterceptorOnly);
   if (p)
   {
      BASSERT(Utils::IsAligned(p, alignment));
   }
   
   return p;
}

//=============================================================================
// BMemoryHeap::Delete
//=============================================================================
bool BMemoryHeap::Delete(void* pData, int size)
{
   BDEBUG_ASSERT(mHeapType != cInvalidHeapType);
   
   size;
   
   if (!pData)
      return true;

   // All valid heap pointers must be at least DWORD aligned.
   BASSERT( ((uint)pData & 3) == 0 );
   
   // A valid heap pointer can't be below 0x10000 on Win32/360.
   BASSERT( (uint)pData >= 0x10000 );
      
   bool status = false;
   
   claimLock();
         
#ifdef TRACK_HEAP_STATS
   {
      int blockSize = 0;
      const bool success = DetailsInternal(pData, &blockSize);
      BVERIFY(success);
      mStats.trackDelete(blockSize);
   }      
#endif   

#ifdef ALLOCATION_LOGGER
   getAllocationLogger().logDelete(this, pData, mAllocLogType);
#endif

   if (mpInterceptor)
   {
      if (mpInterceptor->Delete(pData, size))
         status = true;
   }
   
   if (!status)
   {
 #if ROCKALL_ENABLED  
      if (mpHeap)
      {
         status = mpHeap->Delete(pData, size);
      }
      else
#endif
      if (mpMSpace)
      {
         mspace_free(mpMSpace, pData);
         status = true;
      }
      else
      {
         alignedFree(pData);
         status = true;
      }
   }      

//   uint64 t; QueryPerformanceCounter((LARGE_INTEGER*)&t);
//   trace("DEL: 0x%08X TIME: %u", pData, (uint)t);

   releaseLock();
         
   return status;
}

//=============================================================================
// BMemoryHeap::Resize
//=============================================================================
void* BMemoryHeap::Resize(void* pData, int newSize, int move, int* actualSize, bool noDelete, bool zero)
{
   BDEBUG_ASSERT(mHeapType != cInvalidHeapType);
   
   if (!pData)
      return NULL;
      
   // All valid heap pointers must be at least DWORD aligned.
   BASSERT( ((uint)pData & 3) == 0 );

   // A valid heap pointer can't be below 0x10000 on Win32/360.
   BASSERT( (uint)pData >= 0x10000 );
   
   if (newSize < sizeof(DWORD))
      newSize = sizeof(DWORD);
      
   void* p = NULL;
         
   claimLock();

#ifdef TRACK_HEAP_STATS
   int origSize = 0;
   bool success = DetailsInternal(pData, &origSize);
   BVERIFY(success);
#endif   

   bool intercepted = false;
   
   if (mpInterceptor)
   {
      int dataSize;
      if (mpInterceptor->Details(pData, &dataSize))
      {
         intercepted = true;
         
         int actualNewSize = 0;
         p = mpInterceptor->Resize(pData, newSize, move, &actualNewSize, noDelete, zero);
                  
         if ((!p) && (move))
         {
#if ROCKALL_ENABLED   
            if (mpHeap)
            {
               p = mpHeap->New(newSize, &actualNewSize, false);
            }
            else
#endif      
            if (mpMSpace)
            {
               p = mspace_malloc(mpMSpace, newSize);
               if (p)
                  actualNewSize = mspace_usable_size(p);
            }
            else
            {
               p = alignedMalloc(newSize);
               if (p)
                  actualNewSize = alignedMSize(p);
            }
                          
            if (p)
            {
#ifndef TRACK_HEAP_STATS
               int origSize = 0;
               DetailsInternal(pData, &origSize);
#endif   

               Utils::FastMemCpy(p, pData, Math::Min(actualNewSize, origSize));
               if ((zero) && (actualNewSize > origSize))
                  Utils::FastMemSet(static_cast<uchar*>(p) + origSize, 0, actualNewSize - origSize);
               
               if (!noDelete)
               {
                  bool success = mpInterceptor->Delete(pData, -1);
                  BVERIFY(success);
               }
            }
         }
         
         if (actualSize) *actualSize = actualNewSize;
      }
   }

   if (!intercepted)
   {
#if ROCKALL_ENABLED   
      if (mpHeap)
      {  
         p = mpHeap->Resize(pData, newSize, move, actualSize, noDelete, zero);
      }
      else
#endif 
      if (mpMSpace)
      {
#ifndef TRACK_HEAP_STATS      
         int origSize = mspace_usable_size(pData);
#endif         
                  
         if (!move)  
            p = mspace_realloc_nomove(mpMSpace, pData, newSize);
         else
            p = mspace_realloc(mpMSpace, pData, newSize);
         if (p)
         {
            const int actualNewSize = mspace_usable_size(p);
            if (actualSize) 
               *actualSize = actualNewSize;
            
            if ((zero) && (actualNewSize > origSize))
               Utils::FastMemSet(static_cast<uchar*>(p) + origSize, 0, actualNewSize - origSize);                
         }
      }
      else  
      {
#ifndef TRACK_HEAP_STATS
         int origSize = 0;
         DetailsInternal(pData, &origSize);
#endif   
         if (move)
            p = alignedRealloc(pData, newSize);
         else
            p = alignedExpand(pData, newSize);
         
         if (p)
         {
            const int actualNewSize = alignedMSize(p);
            if (actualSize) 
               *actualSize = actualNewSize;
            
            if ((zero) && (actualNewSize > origSize))
               Utils::FastMemSet(static_cast<uchar*>(p) + origSize, 0, actualNewSize - origSize);
               
            if (noDelete)
            {
               BVERIFY(!"noDelete unsupported");
            }
         }         
      }
   }      

#if defined(TRACK_HEAP_STATS) || defined(ALLOCATION_LOGGER)
   int curSize = 0;
   if (p)
   {
      success = DetailsInternal(p, &curSize);
      BVERIFY(success);
   }
#endif   

#ifdef TRACK_HEAP_STATS
   if ((p) || (newSize == 0))
      mStats.trackRealloc(origSize, curSize);
#endif

 //  uint64 t; QueryPerformanceCounter((LARGE_INTEGER*)&t);
//   trace("RES: PREV: 0x%08X CUR: 0x%08X CURSIZE: %u TIME: %u", pData, p, curSize, (uint)t);

#ifdef ALLOCATION_LOGGER
   if ((p) || (newSize == 0))
      getAllocationLogger().logResize(this, pData, curSize, p, mAllocLogType);
#endif

   releaseLock();
            
   return p;
}

//=============================================================================
// BMemoryHeap::DetailsInternal
//=============================================================================
bool BMemoryHeap::DetailsInternal(void* pData, int* size)
{
   BDEBUG_ASSERT(mHeapType != cInvalidHeapType);
   
   bool status = false;
   
   if (size)
      *size = 0;
   
   if (!pData)
      return false;
    
   BDEBUG_ASSERT( ((uint)pData & 3) == 0 );

   if (mpInterceptor)
      status = mpInterceptor->Details(pData, size);

   if (!status)
   {
   #if ROCKALL_ENABLED   
      if (mpHeap)
      {
         status = mpHeap->Details(pData, size);
      }
      else
   #endif   
      if (mpMSpace)
      {
         int usableSize = mspace_usable_size(pData);
         status = (usableSize != 0);
         if (size)
            *size = usableSize;
      }
      else
      {
         if (size)
         {
            *size = alignedMSize(pData);
            status = true;
         }
      }
   }      
      
   return status;
}

//=============================================================================
// BMemoryHeap::Details
//=============================================================================
bool BMemoryHeap::Details(void* pData, int* size)
{
   BDEBUG_ASSERT(mHeapType != cInvalidHeapType);
   
   claimLock();
   
   bool result = DetailsInternal(pData, size);
   
   releaseLock();
   
   return result;
}

//=============================================================================
// BMemoryHeap::SetOwnerThread
//=============================================================================
void BMemoryHeap::SetOwnerThread(DWORD threadID)
{
   mOwnerThread = threadID;
}

//=============================================================================
// BMemoryHeap::verify
//=============================================================================
void BMemoryHeap::verify(void)
{
   BDEBUG_ASSERT(mHeapType != cInvalidHeapType);

#if !defined(BUILD_FINAL)
   claimLock();
   
   if (mpInterceptor)
   {
      if (!mpInterceptor->verify())
      {
         BFixedString256 buf;
         buf.format("BMemoryHeap::verify: Interceptor of heap %s is corrupt", mStats.mName);
         BFATAL_FAIL(buf);
      }
   }

#if ROCKALL_ENABLED
   if (mpHeap)
   {
      // Try to walk the entire heap.
      __try
      {
         bool activeFlag = false;
         void* pAddress = NULL;
         int space = 0;

         for ( ; ; )
         {
            const bool moreFlag = mpHeap->Walk(&activeFlag, &pAddress, &space);
            if (!moreFlag)
            {
               break;
            }
         }
      }         
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         BFixedString256 buf;
         buf.format("BMemoryHeap::verify: Heap %s is corrupt", mStats.mName);
         BFATAL_FAIL(buf);
      }         
   }      
#endif   

   releaseLock();
#endif      
}

//=============================================================================
// BMemoryHeapInterceptor::Resize
//=============================================================================
void* BMemoryHeapInterceptor::Resize(void* pData, int newSize, int move, int* pActualSize, bool noDelete, bool zero)
{
   if (pActualSize) *pActualSize = 0;
   
   int curSize;
   if (!Details(pData, &curSize))
      return NULL;

   if (newSize <= curSize)
   {
      const uint bytesShrinking = curSize - newSize;
      if (bytesShrinking < 16)
      {
         if (pActualSize) *pActualSize = curSize;
         return pData;  
      }
   }      

   if (!move)
   {
      if (pActualSize) *pActualSize = curSize;
      return NULL;
   }

   int actualNewSize;
   void* pNewBlock = New(newSize, &actualNewSize, false);
   if (!pNewBlock)
      return NULL;
      
   BDEBUG_ASSERT(actualNewSize >= newSize);      

   Utils::FastMemCpy(pNewBlock, pData, Math::Min(actualNewSize, curSize));
   
   if ((zero) && (actualNewSize > curSize))
      Utils::FastMemSet(static_cast<uchar*>(pNewBlock) + curSize, 0, actualNewSize - curSize);
   
   if (pActualSize) *pActualSize = actualNewSize;

   if (!noDelete)
      Delete(pData, curSize);

   return pNewBlock;
}
