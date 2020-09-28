//============================================================================
//
//  Memory.h
//
//  Copyright (c) 1999-2001, Ensemble Studios
//
//============================================================================
#pragma once

#ifndef BUILD_FINAL
   void traceMem(const char* file, long line);
   #define TRACEMEM  traceMem(__FILE__, __LINE__);
#else
   #define TRACEMEM
#endif

#include "memoryHeap.h"

void verifyAllHeaps(void);

#ifdef XBOX
   //-- System Options
   #ifndef MEMORY_SYSTEM_ENABLE
      #define MEMORY_SYSTEM_ENABLE                      0
   #endif

   #if MEMORY_SYSTEM_ENABLE
//      #pragma message("Memory System Enabled.")
   #else
//      #pragma message("Memory System Disabled.")
   #endif
      
   //-- Memory Manager Options
   #define MEMORY_MANAGER_ENABLE_TRACKING            0
   
   // This tells Rockall we're using it as a lib, not a DLL.
   #ifndef COMPILING_ROCKALL_LIBRARY
      #define COMPILING_ROCKALL_LIBRARY
   #endif      
   
   //----------------------------------------------------------------------------
   //  Includes
   //----------------------------------------------------------------------------
   
   // These operators should be defined in the app's project for reliable linking with both Incredilink and regular link.
#if !defined(CODE_ANALYSIS_ENABLED)           
   void* __cdecl operator new(size_t);
   void* __cdecl operator new[](size_t);
   void  __cdecl operator delete(void*);
   void  __cdecl operator delete[](void*);
#endif   
   #if MEMORY_SYSTEM_ENABLE
      #if defined(BUILD_DEBUG) && !defined(BUILD_CHECKED)
//         #pragma message("Selecting Debug Heap")   
         const eHeapType cPrimaryHeapType = cDebugHeap;
         const eHeapType cRenderHeapType = cDebugHeap;  
         const eHeapType cDefaultHeapType = cDebugHeap;
         const eHeapType cSmallHeapType = cDebugHeap;
         const eHeapType cDefaultPhysicalHeapType = cPhysicalHeap;
      #else
//         #pragma message("Selecting Fast Heap")
         const eHeapType cPrimaryHeapType = cPrimaryHeap;
         const eHeapType cRenderHeapType = cRenderHeap;
         const eHeapType cDefaultHeapType = cSmallHeap;
         const eHeapType cSmallHeapType = cSmallHeap;         
         const eHeapType cDefaultPhysicalHeapType = cPhysicalHeap;
      #endif
   #else
//      #pragma message("Selecting Pass Through Heap")
      const eHeapType cPrimaryHeapType = cCRunTimeHeap;
      const eHeapType cRenderHeapType = cCRunTimeHeap;
      const eHeapType cDefaultHeapType = cCRunTimeHeap;
      const eHeapType cSmallHeapType = cCRunTimeHeap;
      const eHeapType cDefaultPhysicalHeapType = cPhysicalHeap;
   #endif
         
#else  // #ifdef XBOX
//   #pragma message("Selecting Pass Through Heap")
   const eHeapType cPrimaryHeapType = cCRunTimeHeap;
   const eHeapType cRenderHeapType = cCRunTimeHeap;
   const eHeapType cDefaultHeapType = cCRunTimeHeap;
   const eHeapType cSmallHeapType = cCRunTimeHeap;
   const eHeapType cDefaultPhysicalHeapType = cCRunTimeHeap;
#endif // XBOX

#ifdef XBOX
   const bool cMemoryAllocatorsSupportAlignment = true;
#else
   const bool cMemoryAllocatorsSupportAlignment = false;
#endif

// Thread-safe primary heap
extern BMemoryHeap gPrimaryHeap;

// Single threaded sim heap
extern BMemoryHeap gSimHeap;

// Single threaded network heap
extern BMemoryHeap gNetworkHeap;

// Thread-safe render heap
extern BMemoryHeap gRenderHeap;

// Thread-safe file block heap (used during archive loading to minimize memory fragmentation)
extern BMemoryHeap gFileBlockHeap;

// C Run Time library heap
extern BMemoryHeap gCRunTimeHeap;

// Physical Heaps
extern BMemoryHeap gPhysCachedHeap;
extern BMemoryHeap gPhysWriteCombinedHeap;

extern BMemoryHeap gSyncHeap;

#ifdef XBOX
#ifndef BUILD_FINAL
extern BMemoryHeapStats gXPhysicalAllocStats;

LPVOID XPhysicalAlloc_(SIZE_T dwSize, ULONG_PTR ulPhysicalAddress, ULONG_PTR ulAlignment, DWORD flProtect);
LPVOID XPhysicalAllocEx_(SIZE_T dwSize, ULONG_PTR ulLowestAcceptableAddress, ULONG_PTR ulHighestAcceptableAddress, ULONG_PTR ulAlignment, DWORD flProtect);
VOID XPhysicalFree_(LPVOID lpAddress);


void *phx_malloc(size_t size);
void *phx_calloc(size_t num, size_t size);
void phx_free(void *memblock);
void *phx_realloc(void *memblock, size_t size);
void *phx_expand(void *memblock, size_t size);

#define XPhysicalAlloc     XPhysicalAlloc_
#define XPhysicalAllocEx   XPhysicalAllocEx_
#define XPhysicalFree      XPhysicalFree_
#define malloc phx_malloc
#define calloc phx_calloc
#define free phx_free
#define realloc phx_realloc
#define _expand phx_expand


#endif
#endif