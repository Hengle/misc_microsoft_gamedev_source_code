//=============================================================================
//
//  memoryHeap.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//=============================================================================
#pragma once
#include "heapTypes.h"

#if TRACK_HEAP_UTILIZATION
// Define this to true to more accurately track memory allocation utilization.
   #define CLEAR_ALL_ALLOCATED_BLOCKS 1
#endif

#ifdef BUILD_FINAL
   #undef CLEAR_ALL_ALLOCATED_BLOCKS
   #define CLEAR_ALL_ALLOCATED_BLOCKS 0
#endif

// [11/5/2008 xemu] allow stat tracking in all builds but final, but we allow it anyways if we are doing FPS (and mem) logging 
#ifndef BUILD_FINAL
#define TRACK_HEAP_STATS
#endif

// [11/5/2008 xemu] doing it this way so hopefully when we search for ENABLE_FPS_LOG_FILE we will find it here as well 
#ifndef LTCG
#define ENABLE_FPS_LOG_FILE
#endif

#ifdef ENABLE_FPS_LOG_FILE
#define TRACK_HEAP_STATS
#endif

class ROCKALL_FRONT_END;

//=============================================================================
// class BMemoryHeapStats
//=============================================================================
class BMemoryHeapStats
{
public:
   BMemoryHeapStats() { clear(); }
   BMemoryHeapStats(const char* pName);
      
   void clear(void) { ZeroMemory(this, sizeof(*this)); }
   
   void sum(const BMemoryHeapStats& d);
   void createDelta(const BMemoryHeapStats& d);
   void trackNew(int actualSize);
   void trackDelete(int actualSize);
   void trackRealloc(int origSize, int newSize);
      
   char           mName[16];
   int            mCurrentAllocations;
   int            mCurrentAllocationSize;
   int            mMostAllocations;
   int            mMostAllocationSize;
   int            mTotalAllocations;
   __int64        mTotalAllocationSize;
   int            mTotalNews;
   int            mTotalDeletes;
   int            mTotalReallocations;  
   
   double         mTotalLockTime;
   unsigned int   mTotalLocks;
   
   enum { cMaxThreads = 8 };
   unsigned int   mThreadHist[cMaxThreads];
};

//=============================================================================
// class BMemoryHeapInterceptor
//=============================================================================
class BMemoryHeapInterceptor
{
public:
   virtual ~BMemoryHeapInterceptor() { }
   
   virtual void* New(int size, int* pActualSize, bool zero) = 0;
   virtual bool Delete(void* pData, int size) = 0;
   virtual bool Details(void* pData, int* pSize) = 0;
   virtual void* Resize(void* pData, int newSize, int move, int* pActualSize, bool noDelete, bool zero);
   virtual bool verify(void) { return true; }
};

//=============================================================================
// class BMemoryHeap
//=============================================================================
class BMemoryHeap
{
public:
   // writeCombined is ignored unless heapType is cPhysicalHeap
   BMemoryHeap();
   BMemoryHeap(eHeapType heapType, bool threadSafe, const char* pName, bool writeCombined, BMemoryHeapInterceptor* pInterceptor, bool deinitOnDestruction, unsigned char allocLogType);
   ~BMemoryHeap();
   
   void init(eHeapType heapType, bool threadSafe, const char* pName, bool writeCombined, BMemoryHeapInterceptor* pInterceptor, bool deinitOnDestruction, unsigned char allocLogType);
   void deinit();
   
   void setInterceptor(BMemoryHeapInterceptor* pInterceptor);
   BMemoryHeapInterceptor* getInterceptor(void) { return mpInterceptor; }
   
   eHeapType getHeapType(void) const { return mHeapType; }
   
   // Warning: All Rockall heaps created by the class are (currently) not threadsafe! You must call lock()/unlock() before using native Rockall API's!
   // rg [1/25/08] - Rockall's built-in thread safety mechanisms are NOT completely functional on 360 (and may never be).
   ROCKALL_FRONT_END* getHeapPtr(void) const { return mpHeap; }
   
   // Returns the DLMalloc mspace pointer, or NULL if this heap is not a DLMalloc heap.
   void* getMSpace(void) const { return mpMSpace; }
   
   // If true, the Rockall heap will be created as thread safe, otherwise we do our own locking.
   bool usingRockallLocks(void) const { return false; }
   
   bool getThreadSafe(void) const { return mThreadSafe; }

   void claimLock(void);
   void releaseLock(void);
   
   // Returns NULL on failure
   void* New       (int size, int* actualSize = 0, bool zero = false, bool tryInterceptorOnly = false);
   
   // AlignedNew will assert if alignment is > getMaxSupportedAlignment()!
   void* AlignedNew(int size, int alignment = 4, int* actualSize = 0, bool zero = false, bool tryInterceptorOnly = false);
   
   bool  Delete    (void* pData, int size = -1);
   
   // Returns NULL on failure
   void* Resize    (void* pData, int newSize, int move = 1, int* actualSize = 0, bool noDelete = false, bool zero = false);
   
   bool  Details   (void* pData, int* size = 0);
      
   // threadID must be the current thread's id!
   void  SetOwnerThread(DWORD threadID);
   DWORD GetOwnerThread(void) const { return mOwnerThread; }

#ifdef TRACK_HEAP_STATS
   const BMemoryHeapStats& getStats(void) const { return mStats; }
#endif
   
   unsigned int getMaxSupportedAlignment(void) const { return mMaxSupportedAlignment; }
   
   void verify(void);
               
private:
   BMemoryHeap(const BMemoryHeap&);
   BMemoryHeap& operator= (const BMemoryHeap&);
   
   ROCKALL_FRONT_END*      mpHeap;
   
   DWORD                   mOwnerThread;
   
#ifdef TRACK_HEAP_STATS
   BMemoryHeapStats        mStats;
#endif

   unsigned int            mMaxSupportedAlignment;
   
   BMemoryHeapInterceptor* mpInterceptor;

   eHeapType               mHeapType;
   
   void*                   mpMSpace;
   
   unsigned char           mAllocLogType;
   bool                    mThreadSafe : 1;
   bool                    mDeinitOnDestruction : 1;

#ifdef MEMORYHEAP_USE_CS   
   CRITICAL_SECTION        mMutex;
#else   
   BLightWeightMutex       mMutex;   
#endif   

   bool  DetailsInternal(void* pData, int* size = 0);
};

// These dummy heap pointer constants are used with the BMemoryHeapAllocation class.
__declspec(selectany) BMemoryHeap* cNoOpDummyHeapPtr            = (BMemoryHeap*)0x01;
__declspec(selectany) BMemoryHeap* cVirtualAllocDummyHeapPtr    = (BMemoryHeap*)0x02;
#ifdef XBOX
__declspec(selectany) BMemoryHeap* cXPhysicalAllocDummyHeapPtr  = (BMemoryHeap*)0x03;
#endif

inline bool isDummyHeapPtr(BMemoryHeap* p) { return p <= (BMemoryHeap*)0x07; }

//=============================================================================
// class BMemoryHeapAllocation
// This class DOES NOT do any sort of lifetime management -- that is your 
// responsibility! This class does not "own" the allocated block. It doesn't 
// automatically free the block when destructed, or when the allocation changes.
//=============================================================================
class BMemoryHeapAllocation
{
public:
   BMemoryHeapAllocation() : 
      mpData(NULL), 
      mpHeap(NULL),
      mSize(0)
   {
   }
         
   // Copy constructor sets the destination to NOT own the block.
   BMemoryHeapAllocation(const BMemoryHeapAllocation& other) :
      mpHeap(other.mpHeap),
      mpData(other.mpData),
      mSize(other.mSize)
   {
   }
   
   // Equals operator sets the destination to NOT own the block.
   BMemoryHeapAllocation& operator= (const BMemoryHeapAllocation& other) 
   {
      if (this == &other)
         return *this;
         
      mpHeap = other.mpHeap;
      mpData = other.mpData;
      mSize = other.mSize;
            
      return *this;
   }
   
   void clear(void)
   {
      mpHeap = NULL;
      mpData = NULL;
      mSize = 0;
   }
               
   bool set(BMemoryHeap* pHeap, void* pData, DWORD size)
   {
      mpHeap               = pHeap;
      mpData               = pData;
      mSize                = size;
            
      return true;
   }
   
   bool alloc(DWORD size, BMemoryHeap* pHeap, DWORD allocatorFlags = 0)
   {
      mpHeap = pHeap;
      mpData = NULL;
      mSize = size;
      
      if (mpHeap == cNoOpDummyHeapPtr)
         return false;
      else if (mpHeap == cVirtualAllocDummyHeapPtr)
         mpData = VirtualAlloc(NULL, size, MEM_COMMIT | allocatorFlags, PAGE_READWRITE);
#ifdef XBOX         
      else if (mpHeap == cXPhysicalAllocDummyHeapPtr)
         mpData = XPhysicalAlloc(size, MAXULONG_PTR, 0, PAGE_READWRITE | allocatorFlags);
#endif         
      else if (mpHeap)
         mpData = mpHeap->New(size);
      
      if (!mpData)
         return false;
                  
      return true;
   }
  
   bool free(void)
   {
      if (!mpData)
         return true;
           
      if (!mpHeap)
      {
         mpData = NULL;
         return false;
      }
      
      if (mpHeap == cNoOpDummyHeapPtr)
         mpData = NULL;
      else if (mpHeap == cVirtualAllocDummyHeapPtr)
      {
         const BOOL status = VirtualFree(mpData, 0, MEM_RELEASE);
         if (!status)
            return false;
      }
#ifdef XBOX      
      else if (mpHeap == cXPhysicalAllocDummyHeapPtr)
      {
         XPhysicalFree(mpData);
      }
#endif      
      else
      {
         if (!mpHeap->Delete(mpData))
            return false;
      }
      
      mpData = NULL;
      
      return true;
   }
   
   BMemoryHeap*   getHeap(void) const { return mpHeap; }
   void*          getPtr(void) const { return mpData; }
   DWORD          getSize(void) const { return mSize; }
      
private:
   BMemoryHeap*            mpHeap;
   void*                   mpData;
   DWORD                   mSize;
};

