//============================================================================
//
//  File: alignedAlloc.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"

const uint PTR_XOR = 0x7345FF97;

const uint cMaxSupportedAlignment = 128;

//============================================================================
// BAlignedAlloc::Malloc
// This implementation assumes the underlying allocator can't handle aligned allocs.
//============================================================================
void* BAlignedAlloc::Malloc(uint size, uint alignment, BMemoryHeap& heap, bool zero)
{
   BDEBUG_ASSERT((alignment >= 1) && (alignment <= cMaxSupportedAlignment) && Math::IsPow2(alignment));

   // rg [1/23/08] - These checks may seem odd. Either the allocator supports the largest alignment we'll ever need, or it doesn't.
   // In practice, getMaxSupportedAlignment() should return 4 or 16.
   if (heap.getMaxSupportedAlignment() > sizeof(DWORD))
   {
      BASSERT(alignment <= heap.getMaxSupportedAlignment());
      
      size = Utils::AlignUpValue(size, alignment);
      void* p = heap.New(size, NULL, zero);
      if (!p)
      {
         BFATAL_FAIL("BAlignedAlloc::New failed");
      }
      BASSERT( Utils::IsAligned(p, alignment) );
      return p;
   }
            
   if (alignment < sizeof(DWORD))
      alignment = sizeof(DWORD);

   const uint rawSize = size + alignment;
   uchar* pRaw = reinterpret_cast<uchar*>(heap.New(rawSize));
   if (!pRaw)
   {
      BFATAL_FAIL("BAlignedAlloc::New failed");
   }
   
   uchar* pAligned = reinterpret_cast<uchar*>( (reinterpret_cast<DWORD>(pRaw) + alignment) & (~(alignment - 1)) );

   BDEBUG_ASSERT((pAligned - pRaw) >= sizeof(DWORD));
   BDEBUG_ASSERT((uint)((pAligned + size) - pRaw) <= rawSize);

   *(reinterpret_cast<DWORD*>(pAligned) - 1) = reinterpret_cast<DWORD>(pRaw) ^ PTR_XOR;
   
   if (zero)
      Utils::FastMemSet(pAligned, 0, size);

   return pAligned;
}

//============================================================================
// BAlignedAlloc::Free
//============================================================================
void BAlignedAlloc::Free(void* pData, BMemoryHeap& heap)
{
   if (!pData)
      return;
      
   BDEBUG_ASSERT((reinterpret_cast<DWORD>(pData) & 3) == 0);
   
   if (heap.getMaxSupportedAlignment() > sizeof(DWORD))
   {
      bool status = heap.Delete(pData);
      status;
      BDEBUG_ASSERT(status);
      return;
   }
   
   void* pRaw = reinterpret_cast<void*>(*(static_cast<DWORD*>(pData) - 1) ^ PTR_XOR);
   
   BDEBUG_ASSERT(
      (reinterpret_cast<DWORD>(pRaw) >= 65536) && 
      (pRaw < pData) && 
      ((reinterpret_cast<uchar*>(pData) - reinterpret_cast<uchar*>(pRaw)) <= cMaxSupportedAlignment) && 
      ((reinterpret_cast<DWORD>(pRaw) & 3) == 0) );

   bool status = heap.Delete(pRaw);
   status;
   BDEBUG_ASSERT(status);
}

#if 0     
uint nx;
struct Obj
{
   DWORD x;
   //Obj() { printf("Obj::Obj\n"); }
   //~Obj() { printf("Obj::~Obj\n"); }

   Obj() { nx++; }
   ~Obj() { nx--; }
};

#include "math\random.h"
Random rand;
for ( ; ; )
{
   typedef BDynamicArray<Obj*> ObjPtrArray;
   ObjPtrArray array(1024);

   for (uint i = 0; i < 1024; i++)
   {
      uint n = rand.iRand(0, 1024);
      uint a = 1 << rand.iRand(0, 10);

      Obj* p = BAlignedAlloc::NewArray<Obj>(n, a, gSimHeap);
      array[i] = p;
   }
   for (uint i = 0; i < 1024; i++)            
      BAlignedAlloc::DeleteArray(array[i], gSimHeap);

   BDEBUG_ASSERT(0 == nx);         
}      
#endif   
