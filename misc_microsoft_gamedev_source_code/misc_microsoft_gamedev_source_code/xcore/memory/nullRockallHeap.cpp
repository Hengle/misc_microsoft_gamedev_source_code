//============================================================================
// nullRockallHeap.cpp
//
// Copyright (c) 2005-2006, Ensemble Studios
//
// Simple replacement of Rockall heap classes
//============================================================================
#include "xcore.h"
#include "nullRockallHeap.h"

//----------------------------------------------------------------------------
// BNullRockallHeap::New
//----------------------------------------------------------------------------
void* BNullRockallHeap::New(int size, int* actualSize, bool zero)
{
   void* pData=alignedMalloc(size);
   if(!pData)
      return NULL;
   if(actualSize) 
      *actualSize=alignedMSize(pData);
   if (zero)
      memset(pData, 0, size);
   return pData;
}

//----------------------------------------------------------------------------
// BNullRockallHeap::Delete
//----------------------------------------------------------------------------
bool BNullRockallHeap::Delete(void* pData, int size)
{
   size;
   if(pData)
   {
      alignedFree(pData);
      return true;
   }
   else
      return false;
}

//----------------------------------------------------------------------------
// BNullRockallHeap::Resize
//----------------------------------------------------------------------------
void* BNullRockallHeap::Resize(void* pData, int newSize, int move, int* actualSize, bool noDelete, bool zero)
{
   move;
   
   void* pNewData;
   pNewData=alignedRealloc(pData, newSize, 0, move != 0);
   if(!pNewData)
      return NULL;
   if(actualSize) 
      *actualSize=alignedMSize(pData);
   if(zero)
      memset(pNewData, 0, newSize);
   if(noDelete)
   {
      BASSERT(0); // noDelete unsupported
   }
   return pNewData;
}

//----------------------------------------------------------------------------
// BNullRockallHeap::Details
//----------------------------------------------------------------------------
bool BNullRockallHeap::Details(void* pData, int* size)
{
   if(!pData)
      return false;
   if(size)
      *size=alignedMSize(pData);
   return true;
}

//----------------------------------------------------------------------------
// BNullRockallHeap::Walk
//----------------------------------------------------------------------------
bool BNullRockallHeap::Walk(bool* active, void** pData, int* size)
{
   //FIXME XBOX
   active;
   pData;
   size;
   return false;
}

//----------------------------------------------------------------------------
// BNullRockallHeap::DeleteAll
//----------------------------------------------------------------------------
void BNullRockallHeap::DeleteAll(bool recycle)
{
   //FIXME XBOX
   recycle;
}

//----------------------------------------------------------------------------
// BNullRockallHeap::Verify
//----------------------------------------------------------------------------
bool BNullRockallHeap::Verify(void* pData, int* size)
{
   if(!pData)
      return false;
   if(size)
      *size=alignedMSize(pData);
   return true;
}

//----------------------------------------------------------------------------
// BNullRockallHeap::HeapLeaks
//----------------------------------------------------------------------------
void BNullRockallHeap::HeapLeaks(void)
{
}
