//============================================================================
//
//  File: globalObjects.cpp
//  
// Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "globalObjects.h"

#include "threading\eventDispatcher.h"
#include "memory\allocationLogger.h"
#include "threading\helperThread.h"
#include "file\lowLevelFileIO.h"

#ifndef XBOX
   #include "debughelp.h"
#endif

//============================================================================
// BInitialKernelMemoryTracker::BInitialKernelMemoryTracker
//============================================================================
BInitialKernelMemoryTracker::BInitialKernelMemoryTracker()
{
   MEMORYSTATUS status;
   ZeroMemory(&status, sizeof(status));
   GlobalMemoryStatus(&status);
   const uint MB = 1024U * 1024U;
   trace("Initial Kernel Memory Statistics: PhysAlloc: %uMB, PhysFree=%uMB, VirtFree=%uMB", (status.dwTotalPhys - status.dwAvailPhys + (MB / 2)) / MB, status.dwAvailPhys / MB, status.dwAvailVirtual/MB );   

   mInitialPhysicalFree = status.dwAvailPhys;
}

#pragma warning(disable: 4073)
#pragma init_seg(lib)
#pragma warning(default: 4073)

//============================================================================
//  GLOBALS
//============================================================================
// These are initialized in the order they appear and deinitialized in the 
// reverse order. The memory manager is created first and shut down last, 
// ensuring that allocations done by the other globals here are accounted for properly.

BInitialKernelMemoryTracker   gInitialKernelMemoryTracker;

BWin32LowLevelFileIO          gWin32LowLevelFileIO;

// C Run Time library heap
BMemoryHeap                   gCRunTimeHeap(cCRunTimeHeap, true, "CRunTime", false, NULL, false, cAllocLogTypeOther);

// Virtual memory heaps
BMemoryHeap                   gPrimaryHeap(cPrimaryHeapType, true, "Primary", false, NULL, false, cAllocLogTypePrimary);
BMemoryHeap                   gSimHeap(cDefaultHeapType, false, "Sim", false, NULL, false, cAllocLogTypeSim);
BMemoryHeap                   gRenderHeap(cRenderHeapType, true, "Render", false, NULL, false, cAllocLogTypeRender);
BMemoryHeap                   gFileBlockHeap(cDefaultHeapType, true, "FileBlock", false, NULL, false, cAllocLogTypeOther);    //used for allocations from the FileCache
BMemoryHeap                   gStringHeap(cSmallHeapType, true, "String", false, NULL, false, cAllocLogTypeOther);
BMemoryHeap                   gNetworkHeap(cDefaultHeapType, true, "Net", false, NULL, false, cAllocLogTypeOther);

// Physical heaps
BMemoryHeap                   gPhysCachedHeap(cDefaultPhysicalHeapType, true, "PhysC", false, NULL, false, cAllocLogTypeOther);           //used for ECF loading (UGX/UAX)
BMemoryHeap                   gPhysWriteCombinedHeap(cDefaultPhysicalHeapType, true, "PhysWC", true, NULL, false, cAllocLogTypeOther);


BMemoryHeap                   gSyncHeap(cDefaultHeapType, true, "Sync", false, NULL, false, cAllocLogTypeOther);

BAssert                       gAssertionSystem;

#ifndef XBOX
   BDebugHelp                 gDebugHelp;
#endif

BEventDispatcher              gEventDispatcher;

BHelperThread                 gMiscHelperThread;
