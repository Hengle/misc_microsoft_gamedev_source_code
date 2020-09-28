// File: xcorelib.cpp
#include "xcore.h"
#include "xcorelib.h"

#include "threading\eventDispatcher.h"
#include "threading\setThreadName.h"
#include "threading\workDistributor.h"

#include "memory\allocationLogger.h"

static uint gRefCount;

#pragma data_seg(".ENS$XDA") 
BXCoreInitFuncPtr gBeginDeinit[] = { (BXCoreInitFuncPtr)0xFFFFFFFF }; 

#pragma data_seg(".ENS$XDZ") 
BXCoreInitFuncPtr gEndDeinit[] = { (BXCoreInitFuncPtr)0xFFFFFFFF }; 

#pragma data_seg(".ENS$XIA") 
BXCoreInitFuncPtr gBeginInit[] = { (BXCoreInitFuncPtr)0xFFFFFFFF }; 

#pragma data_seg(".ENS$XIZ") 
BXCoreInitFuncPtr gEndInit[] = { (BXCoreInitFuncPtr)0xFFFFFFFF }; 

#pragma data_seg() 

//#pragma comment(linker, "/merge:.ENS=.data")

static void executeFuncPtrArray(BXCoreInitFuncPtr* pBegin, BXCoreInitFuncPtr* pEnd, bool param)
{
   for (BXCoreInitFuncPtr* p = pBegin; p < pEnd; p++)
   {
      if ((*p) && (*p != (BXCoreInitFuncPtr)0xFFFFFFFF))
      {
         bool status = (*p)(param);
         if (!status)
            abort();
      }         
   }
}

void XCoreCreate(void)
{
   if (gRefCount == 0)
   {
      SetThreadName(GetCurrentThreadId(), "MainThread");
      
      gSimHeap.SetOwnerThread(GetCurrentThreadId());
      
      gWorkDistributor.init();
                  
      executeFuncPtrArray(gBeginInit, gEndInit, true);

#ifdef XBOX      
      gMiscHelperThread.init(cThreadIndexMisc, 5, "MiscHelper", NULL, 0, BEventDispatcher::cDefaultEventQueueSize, BEventDispatcher::cDefaultSyncEventQueueSize, true);
#endif      

#ifdef ALLOCATION_LOGGER
      getAllocationLogger().registerHeap("XMemPhysical", XMEMPHYSICAL_HEAP, true);
      getAllocationLogger().registerHeap("XPhysicalAlloc", XPHYSICALALLOC_HEAP, true);
#endif
   }

   gRefCount++;      
}

void XCoreRelease(void)
{
   BASSERT(gRefCount >= 1);

   gRefCount--;

   if (gRefCount == 0)
   {
#ifdef XBOX         
      gEventDispatcher.pumpAllThreads(100, 10);

      if (gEventDispatcher.getThreadId(cThreadIndexMisc))
            gEventDispatcher.pumpUntilThreadQueueEmpty(cThreadIndexMisc);
            
      gMiscHelperThread.deinit();
#endif      
      
      executeFuncPtrArray(gBeginDeinit, gEndDeinit, false);
      
      gWorkDistributor.deinit();
   }      
}

