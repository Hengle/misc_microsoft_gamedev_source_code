//============================================================================
//
// File: synchronizedBlockAllocator.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "synchronizedBlockAllocator.h"

#include "threading\simpleThreadManager.h"

#ifndef BUILD_FINAL
void BSynchronizedBlockAllocatorTest::ThreadTestFunc(void* pData1, void* pData2)
{
   pData2;
   
   BBlockAllocator* pBlockAlloc = (BBlockAllocator*)(pData1);

   uint q = 0;
   for ( ; ; )
   {  
      const uint numEntries = 1+(rand()%4096);

      BDynamicArray<BBlah*> pointers(numEntries);

      uint sum = 0;
      for (uint i = 0; i < numEntries; i++)
      {
         uint val = rand();
         sum += val;
         pointers[i] = pBlockAlloc->alloc(BBlah(val));
      }

      uint checkSum = 0;
      for (uint i = 0; i < numEntries; i++)
         checkSum += pointers[i]->mVal;

      BVERIFY(checkSum == sum);

      for (uint i = 0; i < numEntries; i++)          
      {
         uint r = i + (rand() % (numEntries - i));
         std::swap(pointers[i], pointers[r]);
      }

      for (uint i = 0; i < numEntries; i++)          
         pBlockAlloc->free(pointers[i]);

      if ((++q % 100) == 0)
         tracenocrlf(",");
   }        
}

void BSynchronizedBlockAllocatorTest::test(void)
{
   BSimpleThreadManager threadMan;
   threadMan.init(6);
   for (uint i = 0; i < 6; i++)
   threadMan.setThreadProcessor(i, i);

   BBlockAllocator* pBlockAlloc = new BBlockAllocator;

   threadMan.run(6, ThreadTestFunc, pBlockAlloc);

   uint q = 0;
   for ( ; ; )
   {  
      const uint numEntries = 1+(rand()%4096);

      BDynamicArray<BBlah*> pointers(numEntries);

      uint sum = 0;
      for (uint i = 0; i < numEntries; i++)
      {
         uint val = rand();
         sum += val;
         pointers[i] = pBlockAlloc->alloc(BBlah(val));
      }

      uint checkSum = 0;
      for (uint i = 0; i < numEntries; i++)
         checkSum += pointers[i]->mVal;

      BVERIFY(checkSum == sum);

      for (uint i = 0; i < numEntries; i++)          
      {
         uint r = i + (rand() % (numEntries - i));
         std::swap(pointers[i], pointers[r]);
      }

      for (uint i = 0; i < numEntries; i++)          
         pBlockAlloc->free(pointers[i]);

      if ((++q % 100) == 0)
         tracenocrlf(".");
   }      
}
#endif

uint gSynchronizedBlockAllocatorDummy;