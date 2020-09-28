//==============================================================================
// file: XboxMemory.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "common.h"
#include "memory\allocationLogger.h"

#ifndef BUILD_FINAL  
BMemoryHeapStats mNewDeleteStats("NewDelete");
BMemoryHeapStats mXMemCachedHeapStats("XMemCached");
BMemoryHeapStats mXMemPhysicalHeapStats("XMemPhysical");
#endif

#if !defined(CODE_ANALYSIS_ENABLED)
//==============================================================================
// operator new
//==============================================================================
void* __cdecl operator new(size_t size)
{
   if (size >= 512U * 1024U * 1024U)
   {
      BFATAL_FAIL("operator new: size is out of range");   
   }

   void* p = gPrimaryHeap.New(static_cast<long>(size));
   if (!p)
   {
      BFATAL_FAIL("operator new: Out of memory");
   }
   
#ifndef BUILD_FINAL  
   int actualSize;
   gPrimaryHeap.Details(p, &actualSize);
   mNewDeleteStats.trackNew(actualSize);
#endif   
      
   return p;
}

//==============================================================================
// operator delete
//==============================================================================
void __cdecl operator delete(void* data)
{
   if (data)
   {
#ifndef BUILD_FINAL  
      int actualSize;
      gPrimaryHeap.Details(data, &actualSize);
      mNewDeleteStats.trackDelete(actualSize);
#endif   
   
      const bool success = gPrimaryHeap.Delete(data);
      success;
      BDEBUG_ASSERT(success);
   }
}

//==============================================================================
// operator new[]
//==============================================================================
void* __cdecl operator new[](size_t size)
{
   if (size >= 512U * 1024U * 1024U)
   {
      BFATAL_FAIL("operator new[]: size is out of range");   
   }

   void* p = gPrimaryHeap.New(static_cast<long>(size));
   if (!p)
   {
      BFATAL_FAIL("operator new[]: Out of memory");   
   }
   
#ifndef BUILD_FINAL  
   int actualSize;
   gPrimaryHeap.Details(p, &actualSize);
   mNewDeleteStats.trackNew(actualSize);
#endif   
   
   return p;
}

//==============================================================================
// operator delete
//==============================================================================
void __cdecl operator delete[](void* data)
{ 
   if (data)
   {
#ifndef BUILD_FINAL  
      int actualSize;
      gPrimaryHeap.Details(data, &actualSize);
      mNewDeleteStats.trackDelete(actualSize);
#endif   

      const bool success = gPrimaryHeap.Delete(data);
      success;
      BDEBUG_ASSERT(success);
   }
}

#endif // CODE_ANALYSIS_ENABLED

//==============================================================================
// XMemAlloc
//==============================================================================
LPVOID WINAPI XMemAlloc(
                        SIZE_T dwSize,
                        DWORD dwAllocAttributes
                        )
{
   XALLOC_ATTRIBUTES* pAttr = (XALLOC_ATTRIBUTES*) &dwAllocAttributes;

   if (pAttr->dwMemoryType == XALLOC_MEMTYPE_HEAP && !pAttr->dwHeapTracksAttributes)
   {
      uint align = sizeof(DWORD);
      switch (align)
      {
         case XALLOC_ALIGNMENT_4:
         {
            align = 4;
            break;
         }
         case XALLOC_ALIGNMENT_8:
         {
            align = 8;
            break;
         }
         case XALLOC_ALIGNMENT_16:
         {
            align = 16;
            break;
         }
      }
      
      if (dwSize < sizeof(DWORD))
         dwSize = sizeof(DWORD);
      
      dwSize = Utils::AlignUpValue(dwSize, align);

      // This uses the render heap because most XTL allocations are from D3D.
      void* pAddr = gRenderHeap.New(dwSize);
      if (!pAddr)
      {
         BFATAL_FAIL("XMemAlloc: Out of memory");   
      }
      
      BDEBUG_ASSERT(Utils::IsAligned(pAddr, align));

      if (pAttr->dwZeroInitialize)
         XMemSet(pAddr, 0, dwSize);

#if !defined(BUILD_FINAL) || defined(ALLOCATION_LOGGER)      
      int actualSize;
      gRenderHeap.Details(pAddr, &actualSize);
#endif      
         
#ifndef BUILD_FINAL  
      mXMemCachedHeapStats.trackNew(actualSize);
#endif            


      return pAddr;
   }

   LPVOID p = XMemAllocDefault(dwSize, dwAllocAttributes);

#if !defined(BUILD_FINAL) || defined(ALLOCATION_LOGGER)      
   uint actualSize = 0;
   if (p)
      actualSize = XMemSizeDefault(p, dwAllocAttributes);  
#endif         

#ifndef BUILD_FINAL 
   mXMemPhysicalHeapStats.trackNew(actualSize);
#endif   

#ifdef ALLOCATION_LOGGER
   getAllocationLogger().logNew(XMEMPHYSICAL_HEAP, dwSize, p, Utils::RoundUp(actualSize, 4096), cAllocLogTypePhysical);
#endif
   
   return p;
}                        

//==============================================================================
// XMemFree
//==============================================================================
VOID WINAPI XMemFree(
                     PVOID pAddress,
                     DWORD dwAllocAttributes
                     )
{
   if (!pAddress)
      return;

   XALLOC_ATTRIBUTES* pAttr = (XALLOC_ATTRIBUTES*) &dwAllocAttributes;
      
   if (pAttr->dwMemoryType == XALLOC_MEMTYPE_HEAP && !pAttr->dwHeapTracksAttributes)
   {
#ifndef BUILD_FINAL   
      int actualSize;
      gRenderHeap.Details(pAddress, &actualSize);
      mXMemCachedHeapStats.trackDelete(actualSize);
#endif

      const bool success = gRenderHeap.Delete(pAddress);
      success;
      BDEBUG_ASSERT(success);
      
      return;
   }

#ifndef BUILD_FINAL   
   const uint actualSize = XMemSizeDefault(pAddress, dwAllocAttributes);
   mXMemPhysicalHeapStats.trackDelete(actualSize);
#endif   

#ifdef ALLOCATION_LOGGER
   getAllocationLogger().logDelete(XMEMPHYSICAL_HEAP, pAddress, cAllocLogTypePhysical);
#endif

   return XMemFreeDefault(pAddress, dwAllocAttributes);
}                     

//==============================================================================
// XMemSize
//==============================================================================
SIZE_T WINAPI XMemSize(
                       PVOID pAddress,
                       DWORD dwAllocAttributes
                       )
{
   XALLOC_ATTRIBUTES* pAttr = (XALLOC_ATTRIBUTES*) &dwAllocAttributes;

   if (pAttr->dwMemoryType == XALLOC_MEMTYPE_HEAP && !pAttr->dwHeapTracksAttributes)
   {
      int size = 0;
      const bool success = gRenderHeap.Details(pAddress, &size);
      success;
      BDEBUG_ASSERT(success);

      return size;
   }
   
   return XMemSizeDefault(pAddress, dwAllocAttributes);
}    

