//=============================================================================
//
//  Memory.cpp
//
//  Copyright (c) 1999-2002, Ensemble Studios
//
//=============================================================================
#include "xcore.h"
#include "threading\eventDispatcher.h"
#include "memory\allocationLogger.h"

#ifdef XBOX
#include "xbdm.h"
#endif

#define TRACEMEM_ENABLED 0

#ifndef BUILD_FINAL
//==============================================================================
// traceMem
//==============================================================================
void traceMem(const char* file, long line)
{
   file;
   line;
#if TRACEMEM_ENABLED

   static DWORD traceTime=0;
   static DWORD startTime=0;
   DWORD curTime=GetTickCount();
   if(startTime==0)
      startTime=curTime;
   DWORD totalElapsedTime=curTime-startTime;
   DWORD elapsedTime=0;
   if(traceTime!=0)
      elapsedTime=curTime-traceTime;
   traceTime=curTime;


#ifdef XBOX
   DM_MEMORY_STATISTICS memStats;
   Utils::ClearObj(memStats);
   memStats.cbSize = sizeof(DM_MEMORY_STATISTICS);

   DmQueryTitleMemoryStatistics(&memStats);

   BFixedString<256> buf; 

   buf.format(
      "%s(%d) *TRACEMEM* thread=%u curTime=%u elapsed=%u totalElapsed=%u MBAlloc=%fMB, MBFree=%fMB, VirtualAlloc=%fMB, PhysicalAlloc=%fMB\n", 
      file, line, 
      gEventDispatcher.getThreadIndex(),
      curTime,
      elapsedTime/1000, totalElapsedTime/1000,
      (memStats.TotalPages-memStats.AvailablePages)*4096.0f/(1024.0f*1024.0f),
      memStats.AvailablePages*4096.0f/(1024.0f*1024.0f),
      memStats.VirtualMappedPages*4096.0f/(1024.0f*1024.0f),
      memStats.ContiguousPages*4096.0f/(1024.0f*1024.0f));

   OutputDebugStringA(buf);      

#else

   MEMORYSTATUS status;
   ZeroMemory(&status, sizeof(status));
   GlobalMemoryStatus(&status);

   const uint MB = 1024U*1024U;
   
   BFixedString<256> buf;

   buf.format(
      "%s(%d) *TRACEMEM* thread=%u curTime=%u elapsed=%u totalElapsed=%u curAlloc=%u, physAlloc=%uMB, physFree=%uMB, virtAlloc=%uMB, virtFree=%uMB\n", 
      file, line, 
      gEventDispatcher.getThreadIndex(),
      curTime,
      elapsedTime/1000, totalElapsedTime/1000,
      0, 
      (status.dwTotalPhys - status.dwAvailPhys)/MB, status.dwAvailPhys/MB, 
      (status.dwTotalVirtual-status.dwAvailVirtual)/MB, status.dwAvailVirtual/MB );
      
   OutputDebugStringA(buf);      
#endif
   //gPrimaryHeap.PrintMemStatistics();
#endif   
}
#endif

#if defined(XBOX) && !defined(BUILD_FINAL)
void verifyAllHeaps(void)
{
   trace("Verifying all heaps");
   gPrimaryHeap.verify();
   gSimHeap.verify();
   gStringHeap.verify();
   gRenderHeap.verify();
   gNetworkHeap.verify();
   gFileBlockHeap.verify();
   gPhysCachedHeap.verify();
   gPhysWriteCombinedHeap.verify();
   gSyncHeap.verify();
   trace("Heaps OK");
}
#else
void verifyAllHeaps(void)
{
}
#endif

#ifdef XBOX
#ifndef BUILD_FINAL

#undef XPhysicalAlloc
#undef XPhysicalAllocEx
#undef XPhysicalFree
#undef malloc
#undef calloc
#undef free
#undef realloc
#undef _expand


BMemoryHeapStats gXPhysicalAllocStats("XPhysicalAlloc");

// FIXME: Redirect these calls to our Rockall physical heaps?

LPVOID XPhysicalAlloc_(SIZE_T dwSize, ULONG_PTR ulPhysicalAddress, ULONG_PTR ulAlignment, DWORD flProtect)
{  
   const uint cMaxAllocSize = 512U*1024U*1024U;
   
   if ((!dwSize) || (dwSize > cMaxAllocSize))
   {
      BFATAL_FAIL("XPhysicalAlloc: dwSize is invalid");
   }
   
   LPVOID p = XPhysicalAlloc(dwSize, ulPhysicalAddress, ulAlignment, flProtect);
   
   if (p)
   {
      uint actualSize = Utils::RoundUp(XPhysicalSize(p), 4096);
      gXPhysicalAllocStats.trackNew(actualSize);
            
#ifdef ALLOCATION_LOGGER   
      getAllocationLogger().logNew(XPHYSICALALLOC_HEAP, dwSize, p, actualSize, cAllocLogTypePhysical);
#endif   
   }
         
 
   return p;
}

LPVOID XPhysicalAllocEx_(SIZE_T dwSize, ULONG_PTR ulLowestAcceptableAddress, ULONG_PTR ulHighestAcceptableAddress, ULONG_PTR ulAlignment, DWORD flProtect)
{
   const uint cMaxAllocSize = 512U*1024U*1024U;

   if ((!dwSize) || (dwSize > cMaxAllocSize))
   {
      BFATAL_FAIL("XPhysicalAlloc: dwSize is invalid");
   }

   LPVOID p = XPhysicalAllocEx(dwSize, ulLowestAcceptableAddress, ulHighestAcceptableAddress, ulAlignment, flProtect);
   
   if (p)
   {
      uint actualSize = Utils::RoundUp(XPhysicalSize(p), 4096);
      gXPhysicalAllocStats.trackNew(actualSize);
      
#ifdef ALLOCATION_LOGGER   
      getAllocationLogger().logNew(XPHYSICALALLOC_HEAP, dwSize, p, actualSize, cAllocLogTypePhysical);
#endif         
   }
   
   return p;
}

VOID XPhysicalFree_(LPVOID lpAddress)
{
   if (lpAddress)
   {
      uint64 addressBits = (uint64)lpAddress;
      if ((addressBits & 4095) || (addressBits < 65536))
      {
         BFATAL_FAIL("XPhysicalFree: Invalid address");
      }
      
      uint actualSize = Utils::RoundUp(XPhysicalSize(lpAddress), 4096);
      gXPhysicalAllocStats.trackDelete(actualSize);

#ifdef ALLOCATION_LOGGER   
      getAllocationLogger().logDelete(XPHYSICALALLOC_HEAP, lpAddress, cAllocLogTypePhysical);
#endif         
   
      XPhysicalFree(lpAddress);
   }
}


void *phx_malloc(size_t size)
{
   void *p = malloc(size);
   
#ifdef ALLOCATION_LOGGER   
   if(p)
      getAllocationLogger().logNew(CRUNTIME_HEAP, size, p, size, cAllocLogTypeOther);
#endif   

   return(p);
}


void *phx_calloc(size_t num, size_t size)
{
   void *p = calloc(num, size);
   
#ifdef ALLOCATION_LOGGER   
   if(p)
      getAllocationLogger().logNew(CRUNTIME_HEAP, size, p, size, cAllocLogTypeOther);
#endif   

   return(p);
}


void phx_free(void *memblock)
{
#ifdef ALLOCATION_LOGGER
   if(memblock)
      getAllocationLogger().logDelete(CRUNTIME_HEAP, memblock, cAllocLogTypeOther);
#endif         

   free(memblock);
}


void *phx_realloc(void *memblock, size_t size)
{
   void *p = realloc(memblock, size);

#ifdef ALLOCATION_LOGGER
   if(p)
      getAllocationLogger().logResize(CRUNTIME_HEAP, memblock, size, p, cAllocLogTypeOther);
#endif         
   
   return(p);
}

void *phx_expand(void *memblock, size_t size)
{
   void *p = _expand(memblock, size);
   
#ifdef ALLOCATION_LOGGER   
   if(p)
      getAllocationLogger().logResize(CRUNTIME_HEAP, memblock, size, p, cAllocLogTypeOther);
#endif         


   return(p);
}


#endif
#endif