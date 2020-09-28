//============================================================================
//
//  BStringManager.h
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================
#pragma once 

#if 0

//----------------------------------------------------------------------------
//  Build Options
//----------------------------------------------------------------------------
#define BUSTRING_ENABLE_TRACKING 0

#include "memory\AllocFixed.h"
#include "containers\PointerList.h"

//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
const long  NUM_FIXED_HEAPS = 6;
const long  NUM_LANG_DLLS = 2;


//----------------------------------------------------------------------------
//  Forward Declarations
//----------------------------------------------------------------------------
class BGameStrings;

//----------------------------------------------------------------------------
//  Public Structs
//----------------------------------------------------------------------------
struct BUStringHeader
{
#if BUSTRING_ENABLE_TRACKING
   long   mHeap;
   bool   mFromStringDLL;
   long   mStringDLLId;
   char   mStringCreator[256];
#endif 
};

#define SIZEOF_BUSTRING_HEADER ((sizeof(BUStringHeader)>1) ? sizeof(BUStringHeader) : 0);

//----------------------------------------------------------------------------
//  Class BStringManager
//----------------------------------------------------------------------------
class BStringManager
{
public:
   //-- Construction/Destruction
   BStringManager();
   ~BStringManager();

   // numChars - number of characters to allocate
   // bytesPerCharLog2 - 0 for chars, 1 for WCHAR's
   // bufferChars is the size of the buffer in characters
   BUStringHeader* New             (uint numChars, uint bytesPerCharLog2, ushort& bufferChars);
   
   void            Delete          (BUStringHeader* pData, uint bytesPerCharLog2, uint dataSize);
   
   //-- Performance and Debug Interface
   void getFixedHeapStats   (long heapIndex, long& characterSize, long& allocationCount, long& allocationSize);
   void getVariableHeapStats(long& allocationCount, long& allocationSize);
   bool dumpStringsToFile   (const BCHAR_T* pFileName, bool sorted);
   bool dumpStringsToLog    ();

   void copyHeader(BUStringHeader *pNewHeader, BUStringHeader *pOldHeader);

private:
   //-- Private Data
   BAllocFixed mFixedHeaps[NUM_FIXED_HEAPS];
   
   //-- String Tracking
   #if BUSTRING_ENABLE_TRACKING
   BPointerList<BUStringHeader> mStringList;
   long        mNumLoadStringCalls;
   BCriticalSection mTrackingCriticalSection;
   #endif

   //-- Disable copy constructor and assignment operator.
   BStringManager(const BStringManager& manager);
   const BStringManager& operator = (const BStringManager& manager);
};

#endif