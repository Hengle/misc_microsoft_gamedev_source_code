//============================================================================
// File: flashallocator.h
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#pragma once
#include "scaleformIncludes.h"

#define SCALEFORM_HEAP gRenderHeap

//============================================================================
// Globals
//============================================================================
static uint gNumActiveFlashAllocs;

//============================================================================
//============================================================================
class BRenderGAllocatorHeapStats : public BEventPayload
{
   public:
      BRenderGAllocatorHeapStats() { clear() ; }
      void clear(void) { ZeroMemory(this, sizeof(*this)); }

      void sum(const BRenderGAllocatorHeapStats& d);
      void createDelta(const BRenderGAllocatorHeapStats& d);
      void trackNew(int actualSize);
      void trackDelete(int actualSize);
      void trackRealloc(int origSize, int newSize);

      virtual void deleteThis(bool delivered) { delete this; }
      
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

//============================================================================
// class BRenderGAllocator
//============================================================================
class BRenderGAllocator : public GAllocator
{
public:
   BRenderGAllocator()
   {
   }
   
   virtual ~BRenderGAllocator() { }

   virtual void    Free(void *pmemBlock) 
   { 
      if (pmemBlock)      
      {
         SCALEFORM_HEAP.Delete(pmemBlock); 
      
         gNumActiveFlashAllocs--;
         mStats.trackDelete(0);
      }
   }

   virtual void*   Alloc(UPInt size) 
   { 
      gNumActiveFlashAllocs++;
                 
      void* p = SCALEFORM_HEAP.AlignedNew(size, 16);
      if (!p)
      {
         BFATAL_FAIL("BRenderGAllocator::Alloc: Out of memory!");
      }
      
      BDEBUG_ASSERT(Utils::IsAligned(p, 16));
      mStats.trackNew(size);      
      return p;
   }

   virtual void*   Realloc(void *pmemBlock, UPInt newSize) 
   {       
      newSize = Math::Max(4U, Utils::AlignUpValue(newSize, 16U));

      void* p;
      if (pmemBlock == NULL)
      {
         gNumActiveFlashAllocs++;
         p = SCALEFORM_HEAP.New(newSize);

         mStats.trackNew(newSize);
      }
      else
      {
         p = SCALEFORM_HEAP.Resize(pmemBlock, newSize);

         mStats.trackRealloc(0, newSize);
      }
         
      if (!p)
      {
         BFATAL_FAIL("BRenderGAllocator::Realloc: Out of memory!");
      }
                
      BDEBUG_ASSERT(Utils::IsAligned(p, 16));
      return p;
   }

   // WTF is extra anywhere? Where they smoking crack?
   virtual void*   AllocAligned(UPInt size, UPInt align, UPInt extra = 0)
   {      
      gNumActiveFlashAllocs++;

      if (align > 16)
      {
         //trace("Flash Warning: Forcing to AllocAligned to 16 Byte alignment");
         align = 16;
      }
      
      void* p = SCALEFORM_HEAP.AlignedNew(size + extra, align);      
      if (!p)
      {
         BFATAL_FAIL("BRenderGAllocator::AllocAligned: Out of memory!");
      }

      mStats.trackNew(size+extra);

      return p;
   }

   // Frees memory allocated by AllocAligned; the original returned value
   // must be specified.
   virtual void    FreeAligned(void *pmemBlock)
   {
      if (pmemBlock)
      {
         gNumActiveFlashAllocs--;
      
         SCALEFORM_HEAP.Delete(pmemBlock);

         mStats.trackDelete(0);
      }
   }

   virtual const GAllocatorStats*   GetStats() const
   { 
      GASSERT(0); 
      return 0; 
   }

   const BRenderGAllocatorHeapStats& getHeapStats() const { return mStats; }

   BRenderGAllocatorHeapStats mStats;   
};

//============================================================================
// class BRenderGAllocator
//============================================================================
class BRenderGDebugAllocator : public GDebugAllocator
{
public:
   BRenderGDebugAllocator()
   {
   }

   virtual ~BRenderGDebugAllocator() { }


   virtual void    Free(void *pmemBlock, 
      int blocktype, const char* pfilename, 
      int line, const char *pclassname=0)
   {
      if (pmemBlock)      
      {
         SCALEFORM_HEAP.Delete(pmemBlock);
      
         gNumActiveFlashAllocs--;
      }
   }

   virtual void*   Alloc(UPInt size,
      int blocktype, const char* pfilename, 
      int line, const char *pclassname=0)
   {
      gNumActiveFlashAllocs++;
      
      void* p = SCALEFORM_HEAP.AlignedNew(size, 16);
      if (!p)
      {
         BFATAL_FAIL("BRenderGDebugAllocator::Alloc: Out of memory!");
      }
      
      BDEBUG_ASSERT(Utils::IsAligned(p, 16));
      
      return p;
   }

   virtual void*   Realloc(void *pmemBlock, UPInt newSize,
      int blocktype, const char* pfilename, 
      int line, const char *pclassname=0)
   {
      newSize = Math::Max(4U, Utils::AlignUpValue(newSize, 16U));

      void* p;
      if (pmemBlock == NULL)
      {
         gNumActiveFlashAllocs++;
         p = SCALEFORM_HEAP.New(newSize);
      }
      else
      {
         p = SCALEFORM_HEAP.Resize(pmemBlock, newSize);
      }

      if (!p)
      {
         BFATAL_FAIL("BRenderGDebugAllocator::Realloc: Out of memory!");
      }

      BDEBUG_ASSERT(Utils::IsAligned(p, 16));
      return p;
   }

   // *** Aligned Allocation APIs

   // Aligned allocation - same requirements apply as outlined in GAllocator.
   virtual void    FreeAligned(void *pmemBlock, 
      int blocktype, const char* pfilename, 
      int line, const char *pclassname=0)
   {
      if (pmemBlock)
      {
         gNumActiveFlashAllocs--;
         
         SCALEFORM_HEAP.Delete(pmemBlock);
      }
   }

   virtual void*   AllocAligned(UPInt size, UPInt align, UPInt extra,
      int blocktype, const char* pfilename, 
      int line, const char *pclassname=0)
   {      
      gNumActiveFlashAllocs++;

      if (align > 16)
      {
         //trace("Flash Warning: %s (line:%d) tried to allocate memory that aligned higher than 16Bytes -- Forcing to 16 Byte alignment", pfilename, line);
         align = 16;
      }

      void* p = SCALEFORM_HEAP.AlignedNew(size + extra, align);
      if (!p)
      {
         BFATAL_FAIL("BRenderGDebugAllocator::AllocAligned: Out of memory!");
      }
      
      BDEBUG_ASSERT(Utils::IsAligned(p, 16));
      
      return p;
   }

   virtual const GAllocatorStats*   GetStats() const
   {
      GASSERT(0);
      return 0;
   }

   virtual void Dump()
   {
      return;
   }
};

void InitGFXAllocator(void);
void GetFlashAllocationStats(uint& numAllocs);

extern BRenderGAllocator gRenderGAllocator;
extern BRenderGDebugAllocator gRenderGDebugAllocator;
