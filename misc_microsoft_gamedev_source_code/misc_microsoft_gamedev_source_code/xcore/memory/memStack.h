//------------------------------------------------------------------------------
//  File: memStack.h
//
//  Copyright (c) 2004-2007, Ensemble Studios
//
//------------------------------------------------------------------------------
#pragma once
#include "memoryHeap.h"
//------------------------------------------------------------------------------
// class BMemStack
// BMemStack is also a valid Allocator object.
// Also see the allocator healper macros/functions in allocators.h
//------------------------------------------------------------------------------
class BMemStack
{
   // purposely undefined
   BMemStack& operator= (const BMemStack& other);
   BMemStack(const BMemStack& other); 

protected:
   BMemoryHeap*   mpHeap;
   uchar*         mpBuf;
   uint           mBufSize;
   uchar*         mpBufNext;
   uchar*         mpBufEnd;

public:
   enum 
   { 
      cCanResizeBuf = false
   };
   
   BMemStack(void* pBuf, uint bufSize) :
      mpHeap(NULL),
      mpBuf(static_cast<uchar*>(pBuf)),
      mBufSize(bufSize)
   {
      BDEBUG_ASSERT(bufSize > 0);
      
      mpBufNext = mpBuf;
      mpBufEnd = mpBuf + bufSize;
      
      Utils::FastMemSet(mpBuf, 0, mBufSize);
   }
   
   BMemStack(BMemoryHeap* pHeap, uint bufSize) :
      mpHeap(pHeap),
      mBufSize(bufSize)
   {
      const uint alignment = 16;
      BDEBUG_ASSERT(Math::IsPow2(alignment));
      BDEBUG_ASSERT(bufSize >= alignment);
      
      mpBuf = static_cast<uchar*>(mpHeap->AlignedNew(mBufSize, alignment, NULL, true));
      if (!mpBuf)
      {
         BFATAL_FAIL("BMemStack::BMemStack: Out of memory!");
      }
      
      mpBufNext = mpBuf;
      mpBufEnd = mpBuf + bufSize;
   }
   
   ~BMemStack()
   {
      if (mpHeap)
      {
         mpHeap->Delete(mpBuf);
         mpBuf = NULL;
         mpHeap = NULL;
      }
   }

   void* alignedAlloc(uint size, uint alignment = 0, uint* pActualSize = NULL, bool zero = false)
   {
      if (!size) 
         size = 1;
         
      if (!alignment)
      {
         if ((size & 127) == 0)
            alignment = 128;
         else if ((size & 63) == 0)
            alignment = 64;
         else if ((size & 31) == 0)
            alignment = 32;
         else if ((size & 15) == 0)
            alignment = 16;
         else if ((size & 7) == 0)
            alignment = 8;
         else if ((size & 3) == 0)
            alignment = 4;
         else if ((size & 1) == 0)
            alignment = 2;
         else
            alignment = 1;
      }
            
      BDEBUG_ASSERT(Math::IsPow2(alignment));

      size = Utils::AlignUpValue(size, alignment);

      uchar* pStart = Utils::AlignUp(mpBufNext, alignment);
      const int left = mpBufEnd - pStart;
      if (left < (int)size)
         return NULL;

      if ((zero) && (pStart > mpBufNext) )
         memset(mpBufNext, 0, pStart - mpBufNext);

      mpBufNext = pStart + size;
      
      if (zero)
         Utils::FastMemSet(pStart, 0, size);
         
      if (pActualSize)
         *pActualSize = size;
         
      return pStart;
   }
      
   void* alloc(uint size, uint* pActualSize = NULL, bool zero = false)
   {
      return alignedAlloc(size, 0, pActualSize, zero);
   }
   
   void dealloc(void* p)
   {
      p;
   }
   
   void* resize(void* p, uint newSize, bool move = true, uint* pActualSize = NULL)
   {
      p;
      newSize;
      move;
      pActualSize;
      
      if (!move)
         return NULL;
      
      void* q = alloc(newSize, pActualSize);
      
      Utils::FastMemCpy(q, p, newSize);
      
      return q;
   }
   
   void swap(BMemStack& other)
   {
      std::swap(other.mpHeap, mpHeap);
      std::swap(other.mpBuf, mpBuf);
      std::swap(other.mBufSize, mBufSize);
      std::swap(other.mpBufNext, mpBufNext);
      std::swap(other.mpBufEnd, mpBufEnd);
   }

   bool operator== (const BMemStack& other) const
   {
      other;
      return true;
   }
   
   uint getOffset(const void* p) const
   {
      BDEBUG_ASSERT((p >= mpBuf) && (p < mpBufEnd));
      return static_cast<const uchar*>(p) - mpBuf;
   }
   
   BMemoryHeap* getMemoryHeap(void) const { return mpHeap; }
   uchar* getBasePtr(void) const { return mpBuf; }
   uint getSize(void) const { return mBufSize; }
   uint getAllocatedBytes(void) const { return mpBufNext - mpBuf; }
   
   class BScopedState
   {
      BMemStack& mMemStack;
      uchar* mpTop;   
      
   public:
      BScopedState(BMemStack& memStack) : 
         mMemStack(memStack),
         mpTop(memStack.mpBufNext) 
      { 
      }
      
      ~BScopedState()
      {
         mMemStack.mpBufNext = mpTop;
      }

      BMemStack& getMemStack(void) const { return mMemStack; }
      
      uchar* getPtr(void) const { return mpTop; }
      uint getOffset(void) const { return mpTop - mMemStack.mpBuf; }
   };
};

inline void* __cdecl operator new  (size_t s, BMemStack& ms) { return ms.alloc(s); }
inline void* __cdecl operator new[] (size_t s, BMemStack& ms) { return ms.alloc(s); }



