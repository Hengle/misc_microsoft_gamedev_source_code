// File: memoryHeapAnalyzer.cpp
#include "xcore.h"
#include "memoryHeapAnalyzer.h"

#if defined(XBOX) && !defined(BUILD_FINAL)

#include "../extlib/rockall/code/rockall/interface/RockallFrontEnd.hpp"

const uint cMaxPageBits = 32768;
const uint cMaxPageBytes = cMaxPageBits >> 3;

BMemoryHeapAnalyzer::BMemoryHeapAnalyzer() : 
   mpHeap(NULL), 
   mpRockall(NULL),
   mBegun(false)
{
}   

bool BMemoryHeapAnalyzer::begin(BMemoryHeap* pHeap)
{
   if (mBegun)
      return false;
   
   if (!pHeap->getHeapPtr())
      return false;
   
   mpHeap = pHeap;
   mpRockall = mpHeap->getHeapPtr();
      
   mPageMap.resize(cMaxPageBytes);
   mPageRead.resize(cMaxPageBytes);
   mPageWritten.resize(cMaxPageBytes);
   mPageFailed.resize(cMaxPageBytes);
   
   mpHeap->claimLock();
   
   {
      // Try to walk the entire heap.
      __try
      {
         bool activeFlag = false;
         void* pAddress = NULL;
         int space = 0;

         for ( ; ; )
         {
            const bool moreFlag = mpRockall->Walk(&activeFlag, &pAddress, &space);
            if (!moreFlag)
               break;
            
            const uint firstPageIndex = (uint)pAddress >> 16;
            const uint lastPageIndex = ((uint)pAddress + space - 1) >> 16;
            BDEBUG_ASSERT(lastPageIndex < 0x80000000);
            
            for (uint i = firstPageIndex; i <= lastPageIndex; i++)
               setBit(mPageMap, i, true);
         }
      }         
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         BFixedString256 buf;
         buf.format("BMemoryHeap::verify: Heap %s is corrupt", mpHeap->getStats().mName);
         BFATAL_FAIL(buf);
      }         
   }
   
   gAssertionSystem.xboxAddExceptionFilterFunc(exceptionFilterFuncPtr, (DWORD)this);
   
   for (uint i = 0; i < cMaxPageBits; i++)
   {
      if (!isBitSet(mPageMap, i))
         continue;
         
      DWORD oldProtect;
      BOOL success;
      success = VirtualProtect((LPVOID)(i << 16), 65536, PAGE_READONLY | PAGE_GUARD, &oldProtect);
      BDEBUG_ASSERT(success && (oldProtect == PAGE_READWRITE));
   }
      
   mpHeap->releaseLock();
   
   mBegun = true;
      
   return true;
}

bool BMemoryHeapAnalyzer::end(void)
{
   if (!mBegun)
      return true;
      
   uint numPagesFailed = 0;
   
   for (uint i = 0; i < cMaxPageBits; i++)
   {
      if (!isBitSet(mPageMap, i))
         continue;

      DWORD oldProtect;
      BOOL success = VirtualProtect((LPVOID)(i << 16), 65536, PAGE_READWRITE, &oldProtect);
      if (!success)
         numPagesFailed++;
   }
   
   gAssertionSystem.xboxRemoveExceptionFilterFunc(exceptionFilterFuncPtr, (DWORD)this);
   
   createResults(numPagesFailed);
   
   mPageMap.resize(0);
   mPageRead.resize(0);
   mPageWritten.resize(0);
   mPageFailed.resize(0);
   
   mpHeap = NULL;
   mpRockall = NULL;
   
   mBegun = false;
   
   return true;
}

void BMemoryHeapAnalyzer::createResults(uint numUnprotectFailed)
{
   uint totalPages = 0;
   uint numPagesReadAndWritten = 0;
   uint numPagesRead = 0;
   uint numPagesFailed = 0;
   
   for (uint pageIndex = 0; pageIndex < cMaxPageBits; pageIndex++)
   {
      if (isBitSet(mPageMap, pageIndex))
      {
         totalPages++;
         if (isBitSet(mPageWritten, pageIndex)) numPagesReadAndWritten++;
         if (isBitSet(mPageRead, pageIndex)) numPagesRead++;
         if (isBitSet(mPageFailed, pageIndex)) numPagesFailed++;
      }
   }

   trace("                 Total pages: %u (%u bytes)", totalPages, totalPages * 65536);
   trace("  Num pages read and written: %u (%u bytes)", numPagesReadAndWritten, numPagesReadAndWritten * 65535);
   trace("              Num pages read: %u (%u bytes)", numPagesRead, numPagesRead * 65535);
   trace("            Num pages failed: %u", numPagesFailed);
   trace("  Num pages unprotect failed: %u", numUnprotectFailed);
}

int BMemoryHeapAnalyzer::exceptionFilterFuncPtr(_EXCEPTION_POINTERS* pExcept, DWORD data)
{
   if ( ((pExcept->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) || (pExcept->ExceptionRecord->ExceptionCode == EXCEPTION_GUARD_PAGE)) && 
        (pExcept->ExceptionRecord->NumberParameters >= 2) )
   {
      const DWORD type = pExcept->ExceptionRecord->ExceptionInformation[0];
      if (type > 1)
         return EXCEPTION_CONTINUE_SEARCH;
         
      const DWORD addr = pExcept->ExceptionRecord->ExceptionInformation[1];
      
      if ((addr < 0x10000) || (addr >= 0x80000000))
         return EXCEPTION_CONTINUE_SEARCH;
         
      BMemoryHeapAnalyzer& analyzer = *(BMemoryHeapAnalyzer*)data;
      
      const uint pageIndex = addr >> 16;
      if (!isBitSet(analyzer.mPageMap, pageIndex))
         return EXCEPTION_CONTINUE_SEARCH;
      
      LPVOID pageAddr = (LPVOID)(pageIndex << 16);
      
      DWORD oldProtect;
      BOOL success;
      if (type)
         success = VirtualProtect((LPVOID)pageAddr, 65536, PAGE_READWRITE, &oldProtect);
      else
         success = VirtualProtect((LPVOID)pageAddr, 65536, PAGE_READONLY, &oldProtect);
         
      {
         BScopedLightWeightMutex lock(analyzer.mMutex);      
         
         if (type)
            setBit(analyzer.mPageWritten, pageIndex, true);
         else
            setBit(analyzer.mPageRead, pageIndex, true);
            
         if (!success)
            setBit(analyzer.mPageFailed, pageIndex, true);
      }            
                     
      return success ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
   }
   
   return EXCEPTION_CONTINUE_SEARCH;
}

void BMemoryHeapAnalyzer::setBit(BDynamicCHeapArray<BYTE>& bytes, uint bitIndex, bool value)
{
   const uint bitMask = Utils::BBitMasks::get(bitIndex & 7);
   const uint byteIndex = bitIndex >> 3;
   
   uint curVal = bytes[byteIndex];
   
   if (value)
      curVal |= bitMask;
   else
      curVal &= ~bitMask;
      
   bytes[byteIndex] = static_cast<BYTE>(curVal);
}

bool BMemoryHeapAnalyzer::isBitSet(BDynamicCHeapArray<BYTE>& bytes, uint bitIndex)
{
   const uint bitMask = Utils::BBitMasks::get(bitIndex & 7);
   const uint byteIndex = bitIndex >> 3;
   return (bytes[byteIndex] & bitMask) != 0;
}

#endif