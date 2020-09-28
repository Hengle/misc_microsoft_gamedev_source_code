//============================================================================
//
//  File: regionAllocator.cpp
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "regionAllocator.h"

//============================================================================
// BRegionAllocator::BRegionAllocator
//============================================================================
BRegionAllocator::BRegionAllocator() :
   mpStart(NULL), 
   mSize(0), 
   mAvail(0),
   mTotalAllocatedBlocks(0),
   mTotalAllocatedBytes(0),
   mTotalFreeBytes(0)
{
}

//============================================================================
// BRegionAllocator::clear
//============================================================================
void BRegionAllocator::clear()
{
   mpStart = NULL;
   mSize = 0;
   mAvail = 0;
   mRegions.clear();
   mTotalAllocatedBlocks = 0;
   mTotalAllocatedBytes = 0;
   mTotalFreeBytes = 0;
}

//============================================================================
// BRegionAllocator::locateRegion
//============================================================================
int BRegionAllocator::locateRegion(uint ofs, uint* pStartOfs) const
{
   uint curOfs = 0;
   const uint numRegions = mRegions.getSize();
   for (uint i = 0; i < numRegions; i++)
   {
      const uint endOfs = curOfs + (mRegions[i] & cSizeMask);
      if ((ofs >= curOfs) && (ofs < endOfs))
      {
         if (pStartOfs) *pStartOfs = curOfs;
         return i;
      }
      curOfs = endOfs;
   }
   if (pStartOfs) *pStartOfs = 0;
   return -1;
}

//============================================================================
// BRegionAllocator::carveReservedRegion
//============================================================================
bool BRegionAllocator::carveReservedRegion(uint startOfs, uint size)
{
   uint regionOfs;
   int regionIndex = locateRegion(startOfs, &regionOfs);
   if (regionIndex < 0)
      return false;
   
   if (startOfs < regionOfs)
      return false;
   
   uint regionSize = getRegionSize(regionIndex);
   if (!isFreeRegion(regionIndex))
      return false;
         
   if (startOfs > regionOfs)
   {
      mRegions[regionIndex] = cFreeFlag | (startOfs - regionOfs);
      
      regionIndex++;
      
      regionSize -= (startOfs - regionOfs);
      regionOfs = startOfs;
      
      mRegions.insert(regionIndex, cFreeFlag | regionSize);
   }
         
   if (size > regionSize)
      return false;
   
   mRegions[regionIndex] = cReservedFlag | size;
   if (size < regionSize)
      mRegions.insert(regionIndex + 1, cFreeFlag | (regionSize - size));
      
   return true;
}

//============================================================================
// BRegionAllocator::init
//============================================================================
bool BRegionAllocator::init(void* pStart, uint size, const BReservedRegion* pReservedRegions, uint numReservedRegions)
{
   clear();
   
   if ((!pStart) || (!size) || (((uint)pStart & 3) != 0) || ((size & 3) != 0))
      return false;
      
   mpStart = pStart;
   mSize = size;
   mAvail = size;
         
   mRegions.pushBack(cFreeFlag | mSize);
         
   for (uint i = 0; i < numReservedRegions; i++)
   {
      if ((pReservedRegions[i].mStartOfs & 3) || (pReservedRegions[i].mEndOfs & 3))
      {
         clear();
         return false;
      }
      
      if (pReservedRegions[i].mEndOfs <= pReservedRegions[i].mStartOfs)
      {  
         clear();
         return false;
      }
      
      const uint usedRegionSize = pReservedRegions[i].mEndOfs - pReservedRegions[i].mStartOfs;
      if (!carveReservedRegion(pReservedRegions[i].mStartOfs, usedRegionSize))
      {
         clear();
         return false;
      }
      
      if (mAvail < usedRegionSize)
      {
         clear();
         return false;
      }
            
      mAvail -= usedRegionSize;
   }
   
   mTotalFreeBytes = mAvail;
   
   return true;
}

//============================================================================
// BRegionAllocator::getAllocStats
//============================================================================
void BRegionAllocator::getAllocStats(uint& totalAllocatedBlocks, uint& totalAllocatedBytes, uint& totalFreeBytes) const
{
   totalAllocatedBlocks = mTotalAllocatedBlocks;
   totalAllocatedBytes = mTotalAllocatedBytes;
   totalFreeBytes = mTotalFreeBytes;
}

//============================================================================
// BRegionAllocator::getLargestFreeRegion
//============================================================================
bool BRegionAllocator::getLargestFreeRegion(uint& largestFreeRegion) const
{
   largestFreeRegion = 0;
   
   uint curOfs = 0;
   for (uint i = 0; i < mRegions.getSize(); i++)
   {
      const uint regionSize = getRegionSize(i);
      
      if (isFreeRegion(i))
         largestFreeRegion = Math::Max(largestFreeRegion, regionSize);
      
      curOfs += regionSize;
   }
         
   return (curOfs == mSize);
}

//============================================================================
// BRegionAllocator::alloc
//============================================================================
void* BRegionAllocator::alloc(uint size, uint* pActualSize)
{
   size = (size + 3) & ~3;      
   if (!size)
      size = 4;
   
   if (size > mTotalFreeBytes)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }

   int bestIndex = -1;
   uint bestLeftover = UINT_MAX;
   uint bestOfs = 0;
   
   uint curOfs = 0;
   const uint numRegions = mRegions.getSize();
   for (uint i = 0; i < numRegions; i++)
   {
      uint regionSize = mRegions[i];
      const uint isRegionFree = (regionSize & cFreeFlag);
      
      regionSize &= cSizeMask;
            
      if (isRegionFree && (regionSize >= size))
      {
         const uint leftover = regionSize - size;
         if (leftover < bestLeftover)
         {
            bestLeftover = leftover;
            bestIndex = i;
            bestOfs = curOfs;
            if (!bestLeftover)
               break;
         }
      }         
      
      curOfs += regionSize;
   }
    
   if (bestIndex < 0)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
      
   mTotalAllocatedBlocks++;
   mTotalAllocatedBytes += size;      
   mTotalFreeBytes -= size;
      
   if (pActualSize) *pActualSize = size;
   
   mRegions[bestIndex] = size;
   
   if (bestLeftover)
      mRegions.insert(bestIndex + 1, bestLeftover | cFreeFlag);
   
   return static_cast<uchar*>(mpStart) + bestOfs;
}

//============================================================================
// BRegionAllocator::free
//============================================================================
bool BRegionAllocator::free(void* p)
{
   if ((!p) || (!mpStart) || ((uint)p & 3) || ((p < mpStart) || (p >= getEnd())) || (!mTotalAllocatedBlocks))
      return false;

   const uint ptrOfs = (uchar*)p - (uchar*)mpStart;
   uint startOfs;
   int index = locateRegion(ptrOfs, &startOfs);

   if ((index < 0) || (startOfs != ptrOfs))
      return false;

   uint regionSize = getRegionSize(index);
   if (regionSize & (cFreeFlag | cReservedFlag))
      return false;

   if (mTotalAllocatedBytes < regionSize)
      return false;
      
   mTotalAllocatedBlocks--;
   mTotalAllocatedBytes -= regionSize;
   mTotalFreeBytes += regionSize;
      
   setRegionFree(index);
         
   if (index > 0)
   {
      if (isFreeRegion(index - 1))
      {
         regionSize += getRegionSize(index - 1);
         mRegions[index - 1] = regionSize | cFreeFlag;
         mRegions.erase(index);
         index--;
      }
   }
   
   if (index < (int)(mRegions.getSize() - 1))
   {
      if (isFreeRegion(index + 1))
      {
         mRegions[index] = (getRegionSize(index + 1) + regionSize) | cFreeFlag;
         mRegions.erase(index + 1);
      }
   }
   
   if (mTotalFreeBytes > mAvail)
      return false;
         
   return true;
}

//============================================================================
// BRegionAllocator::getSize
//============================================================================
uint BRegionAllocator::getSize(void* p) const
{
   if ((!p) || (!mpStart) || ((uint)p & 3) || ((p < mpStart) || (p >= getEnd())))
      return 0;
   
   const uint ptrOfs = (uchar*)p - (uchar*)mpStart;
   uint startOfs;
   const int index = locateRegion(ptrOfs, &startOfs);
   
   if ((index < 0) || (startOfs != ptrOfs))
      return 0;
   
   if (!isUsedRegion(index))
      return 0;
   
   return getRegionSize(index);
}

//============================================================================
// BRegionAllocator::check
//============================================================================
bool BRegionAllocator::check(void) const
{
#define CHECK(x) do { if (!(x)) { DebugBreak(); return false; } } while(0)
   
   if (!mpStart)
   {
      CHECK(mSize == 0);
      CHECK(mAvail == 0);
      CHECK(mRegions.isEmpty());
      CHECK(mTotalAllocatedBlocks == 0);
      CHECK(mTotalAllocatedBytes == 0);
      CHECK(mTotalFreeBytes == 0);
      return true;
   }
   
   CHECK((mTotalFreeBytes + mTotalAllocatedBytes) == mAvail);
   
   CHECK(mAvail <= mSize);
   CHECK(mTotalFreeBytes <= mAvail);
   CHECK((mTotalFreeBytes + mTotalAllocatedBytes) == mAvail);
   CHECK(mTotalAllocatedBlocks <= mRegions.getSize());
   
   uint freeBlocksFound = 0;
   uint usedBlocksFound = 0;
   uint reservedBlocksFound = 0;
   uint freeBytesFound = 0;
   uint usedBytesFound = 0;
   uint reservedBytesFound = 0;
   
   uint curOfs = 0;
   for (uint i = 0; i < mRegions.getSize(); i++)
   {
      const uint regionSize = getRegionSize(i);
      const bool isFree = isFreeRegion(i);
      const bool isReserved = isReservedRegion(i);
      const bool isUsed = isUsedRegion(i);
      
      CHECK(isFree || isReserved || isUsed);
      CHECK(regionSize && ((regionSize & 3) == 0));
      CHECK(regionSize <= mSize);
      
      if (isFree)
      {
         if (i > 0)
         {
            CHECK(!isFreeRegion(i - 1));
         }
         
         freeBlocksFound++;
         freeBytesFound += regionSize;
      }
      else if (isReserved)
      {
         reservedBlocksFound++;
         reservedBytesFound += regionSize;
      }
      else 
      {
         usedBlocksFound++;
         usedBytesFound += regionSize;
      }

      curOfs += regionSize;
      
      CHECK(curOfs <= mSize);
   }

   CHECK(curOfs == mSize);
   CHECK(mTotalFreeBytes == freeBytesFound);
   CHECK(mTotalAllocatedBytes == usedBytesFound);
   CHECK(mTotalAllocatedBlocks == usedBlocksFound);
   
   CHECK((freeBytesFound + reservedBytesFound + usedBytesFound) == mSize);
   
   return true;
#undef CHECK
}














