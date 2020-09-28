//============================================================================
//
//  BStringManager.cpp
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xcore.h"

#if 0

//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
static const long FIXED_HEAP_INITIAL_SIZE   = 16;
static const long VARIABLE_HEAP_GRANULARITY = 256;

//============================================================================
//  PRIVATE GLOBALS
//============================================================================

static ushort sgFixedHeapSizes     [NUM_FIXED_HEAPS] = { 16, 32, 64, 128, 256, 512 };
static ushort sgFixedHeapGrowSizes [NUM_FIXED_HEAPS] = { 256, 256, 128, 32, 8, 4 };

//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BStringManager::BStringManager()
{
#if BUSTRING_ENABLE_TRACKING
   mNumLoadStringCalls = 0;
#endif

   //-- Init the heaps.
   for (long heap = 0; heap < NUM_FIXED_HEAPS; ++heap)
   {
      const long size = sgFixedHeapSizes[heap] + SIZEOF_BUSTRING_HEADER;
      mFixedHeaps[heap].init(size, 0, sgFixedHeapGrowSizes[heap]);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BStringManager::~BStringManager()
{
#if BUSTRING_ENABLE_TRACKING
   dumpStringsToFile(B("c:\\out.txt"), false);
#endif

   for (long heap = 0; heap < NUM_FIXED_HEAPS; ++heap)
      mFixedHeaps[heap].kill();
}


//============================================================================
//  INTERFACE
//============================================================================
BUStringHeader* BStringManager::New(uint numChars, uint bytesPerCharLog2, ushort& bufferChars)
{
   const uint numBytes = numChars << bytesPerCharLog2;
         
   //-- Find the most suitable heap.
   for (int heap = 0; heap < NUM_FIXED_HEAPS; ++heap)   
   {
      if (sgFixedHeapSizes[heap] >= numBytes)
      {
         //-- Allocate.
         BUStringHeader* pData = (BUStringHeader*)mFixedHeaps[heap].lock();
         BASSERT(pData);
         if (!pData)
            continue;

         //-- Set up.
         //pData->setDataLength(0);
         //pData->setDataSize(sgFixedHeapSizes[heap] >> bytesPerCharLog2);
         bufferChars = static_cast<ushort>(sgFixedHeapSizes[heap] >> bytesPerCharLog2);
                  
         //-- Add the string to the used list.
         #if BUSTRING_ENABLE_TRACKING
         {
            BScopedCriticalSection lock(mTrackingCriticalSection);
            mStringList.addToTail(pData);
            pData->mHeap = heap;
            pData->mFromStringDLL = false;
         }            
         #endif

         //-- Return it.
         return pData;
      }
   }

   //-- Use the variable size heap.
   const long numBlocks   = (numBytes / VARIABLE_HEAP_GRANULARITY) + 1;
   const long newNumBytes = (numBlocks * VARIABLE_HEAP_GRANULARITY);
   long totalSize   = newNumBytes + SIZEOF_BUSTRING_HEADER;
   BUStringHeader* pData = (BUStringHeader*)gPrimaryHeap.New(totalSize);

   if (!pData)
   {
      BASSERT(0);
      return 0;
   }

   //-- Set up.
   //pData->setDataLength(0);
   //pData->setDataSize(newNumBytes >> bytesPerCharLog2);
   bufferChars = static_cast<ushort>(newNumBytes >> bytesPerCharLog2);
      
   //-- Add the string to the used list.
   #if BUSTRING_ENABLE_TRACKING
   {
      BScopedCriticalSection lock(mTrackingCriticalSection);
      mStringList.addToTail(pData);
      pData->mHeap = NUM_FIXED_HEAPS; 
      pData->mFromStringDLL = false;
   }
   #endif

   //-- Return it.
   return pData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BStringManager::Delete(BUStringHeader* pData, uint bytesPerCharLog2, uint dataSize)
{
   //-- Make sure pData is valid.
   if (!pData)
      return;
         
   //-- Remove the string from the used list.
   #if BUSTRING_ENABLE_TRACKING
   {
      BScopedCriticalSection lock(mTrackingCriticalSection);
      BHandle hNode = mStringList.findPointerForward(pData);
      if (hNode)
         mStringList.remove(hNode);
   }         
   #endif
   
   const uint numBytes = dataSize << bytesPerCharLog2;

   //-- See which heap it belongs to.
   for (int heap = 0; heap < NUM_FIXED_HEAPS; ++heap)   
   {
      if (numBytes == sgFixedHeapSizes[heap])
      {
         mFixedHeaps[heap].unlock(pData);
         return;
      }
   }
   
   BASSERT(numBytes > sgFixedHeapSizes[NUM_FIXED_HEAPS - 1]);

   //-- It must belong to the variable heap.
   gPrimaryHeap.Delete(pData);
}

//============================================================================
//  PERFORMANCE INTERFACE
//============================================================================
void BStringManager::getFixedHeapStats(long heapIndex, long& characterSize, long& allocationCount, long& allocationSize)
{
   //-- Clear stuff out before we can return.
   characterSize   = 0;
   allocationCount = 0;
   allocationSize  = 0;

   //-- Make sure its a valid heap index.
   if (heapIndex < 0 || heapIndex >= NUM_FIXED_HEAPS)
      return;

   //-- Return the stats.
   characterSize   = sgFixedHeapSizes[heapIndex];
   allocationCount = mFixedHeaps[heapIndex].getCurrentLocks();
   allocationSize  = mFixedHeaps[heapIndex].getLockedSize();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BStringManager::getVariableHeapStats(long& allocationCount, long& allocationSize)
{
   //-- Return the stats.
   allocationCount = 0;//mVariableHeap.getCurrentAllocations();
   allocationSize  = 0;//mVariableHeap.getCurrentAllocationSize();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BStringManager::copyHeader(BUStringHeader *pNewHeader, BUStringHeader *pOldHeader)
{
   UNREFERENCED_PARAMETER(pNewHeader);
   UNREFERENCED_PARAMETER(pOldHeader);
#if BUSTRING_ENABLE_TRACKING
   if (!pNewHeader)
      return;
   if (!pOldHeader)
      return;

   CopyMemory(pNewHeader->mStringCreator, pOldHeader->mStringCreator, sizeof(pNewHeader->mStringCreator));
#endif   
   return;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BStringManager::dumpStringsToFile(const BCHAR_T* pFileName, bool sorted)
{
   sorted;

   if (!pFileName)
      return false;

#if BUSTRING_ENABLE_TRACKING
   BScopedCriticalSection lock(mTrackingCriticalSection);
   
   //-- Try to open the file.
   HANDLE hFile = CreateFile(pFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
      return false;
   
   //-- Write the Unicode token.
   DWORD bytesWritten;

#ifdef UNICODE   
   if (!WriteFile(hFile, &UNICODE_TOKEN, sizeof(UNICODE_TOKEN), &bytesWritten, NULL))
   {
      CloseHandle(hFile);
      return false;
   }
#endif   

   //-- Sort the list.
   BPointerList<BUStringHeader> list = mStringList;
   //if (sorted)
   //   list.quickSort(stringSortFunc, NULL);

   long count = 0;
   long memoryUsed = 0;
   long heapDistribution[NUM_FIXED_HEAPS+1];
   long fromStringDLL = 0;

   for (int i=0; i< (NUM_FIXED_HEAPS+1); i++)
   {
      heapDistribution[i] = 0;
   }
   

   BCHAR_T temp[256];
   //-- Write each string (in unicode format).
   BHandle         hHeader;
   BUStringHeader* pHeader = list.getHead(hHeader);
   while (pHeader)
   {
      // Write the String ID if appropriate
      if (pHeader->mFromStringDLL)
      {
         strFormat(temp, sizeof(temp), B("%d :: "), pHeader->mStringDLLId);
         if (!WriteFile(hFile, temp, strLength(temp)*sizeof(BCHAR_T), &bytesWritten, NULL))
         {
            CloseHandle(hFile);
            return false;
         }
      }


      //-- Write the string.
      // rg [9/7/05] - FIX for Unicode strings!!
      BCHAR_T* pString = (BCHAR_T*)(pHeader + 1);
      long   length  = strLength(pString);
      long   size    = length * sizeof(BCHAR_T);
      if (!WriteFile(hFile, pString, size, &bytesWritten, NULL))
      {
         CloseHandle(hFile);
         return false;
      }


      // Write a separator
      if (!WriteFile(hFile, B("::"), sizeof(BCHAR_T)*2, &bytesWritten, NULL))
      {
         CloseHandle(hFile);
         return false;
      }

      // Write the line info'
      pString = pHeader->mStringCreator;
      long headerLength  = strLength(pString);
      size    = headerLength * sizeof(BCHAR_T);
      if (!WriteFile(hFile, pString, size, &bytesWritten, NULL))
      {
         CloseHandle(hFile);
         return false;
      }


      //-- Write the new line.
      BCHAR_T* pNewLine = B("\r\n");
      if (!WriteFile(hFile, pNewLine, sizeof(BCHAR_T)*2, &bytesWritten, NULL))
      {
         CloseHandle(hFile);
         return false;
      }

      //-- Get String Stats
      count++;
      memoryUsed += length;
      heapDistribution[pHeader->mHeap]++;
      if (pHeader->mFromStringDLL)
         fromStringDLL++;


      //-- Get the next string.
      pHeader = list.getNext(hHeader);
   }

   //-- Write out the summary
   BCHAR_T summary[256];
   strFormat(summary, 256, B("String Count: %d, Memory Allocated: %d, From StringTable: %d"), count, memoryUsed, fromStringDLL);
   if (!WriteFile(hFile, summary, strLength(summary)*sizeof(BCHAR_T), &bytesWritten, NULL))
   {
      CloseHandle(hFile);
      return false;
   }

   //-- Write the new line.
   BCHAR_T* pNewLine = B("\r\n");
   if (!WriteFile(hFile, pNewLine, sizeof(BCHAR_T)*2, &bytesWritten, NULL))
   {
      CloseHandle(hFile);
      return false;
   }


   //-- Close the file.   
   if (!CloseHandle(hFile))
      return false;
   
   //-- All done.
   return true;

#else
   
   //-- Do nothing.
   pFileName;
   sorted;
   return false;

#endif
}

#endif