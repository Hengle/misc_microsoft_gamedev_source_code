//============================================================================
//
//  sfHeapAllocator.cpp
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "sfHeapAllocator.h"

BSFHeapAllocator::BSFHeapAllocator() :
   mTotalBytesAllocated(0),
   mTotalBytesFree(0),
   mTotalRegionBytes(0),
   mLowestUsedBinIndex(cNumBins),
   mOperationIndex(0)
{
}

BSFHeapAllocator::~BSFHeapAllocator()
{
}

void BSFHeapAllocator::addRegion(void* pRegion, uint regionSize)
{
   mOperationIndex++;
   
   BASSERT(regionSize >= cAllocAlignment);
   BASSERT((regionSize & (cAllocAlignment - 1)) == 0);
   BASSERT(Utils::IsAligned(pRegion, cAllocAlignment));
      
   coaleseFreeChunk(pRegion, regionSize);
   addFreeChunk(pRegion, regionSize);
   
   mTotalBytesFree += regionSize;   
   
   mTotalRegionBytes += regionSize;
}

void BSFHeapAllocator::clear()
{
   mOperationIndex++;
      
   mBins.setMaxEntries(0);
   mAllocatedChunks.setMaxEntries(0);
   mFreeChunks.setMaxEntries(0);
   
   mTotalBytesAllocated = 0;
   mTotalBytesFree = 0;
   mTotalRegionBytes = 0;
   mLowestUsedBinIndex = cNumBins;
}

void BSFHeapAllocator::getAllocInfo(uint& totalNumAllocations, uint& totalBytesAllocated, uint& totalBytesFree) const
{
   totalNumAllocations = mAllocatedChunks.getSize();
   totalBytesAllocated = mTotalBytesAllocated;
   totalBytesFree = mTotalBytesFree;
}

void BSFHeapAllocator::getMemoryInfo(uint& totalFreeChunks, uint& largestFreeChunk, uint& totalBins, uint& totalOverhead) const
{
   BASSERT((mFreeChunks.getSize() & 1) == 0);
   totalFreeChunks = mFreeChunks.getSize() >> 1;
   
   totalOverhead = sizeof(BSFHeapAllocator);
   totalOverhead += mBins.getAllocationSize();
   totalOverhead += mFreeChunks.getAllocationSize();
   totalOverhead += mAllocatedChunks.getAllocationSize();
      
   for (BBinHashMap::const_iterator it = mBins.begin(); it != mBins.end(); ++it)
      totalOverhead += it->second.mChunks.getSize() * sizeof(BChunk);
   
   largestFreeChunk = 0;
   for (BPtrSizeHashMap::const_iterator it = mFreeChunks.begin(); it != mFreeChunks.end(); ++it)
      largestFreeChunk = Math::Max(largestFreeChunk, it->second);

   totalBins = mBins.getSize();
}

void* BSFHeapAllocator::alloc(uint size, uint* pActualSize)
{
   mOperationIndex++;
   
   if (mTotalBytesFree < size)
   {
      if (pActualSize) 
         *pActualSize = 0;
      return NULL;
   }
   
   size = Math::Max<uint>(size, cAllocAlignment);
   size = (size + cAllocAlignment - 1) & (~(cAllocAlignment - 1)); 
   
   BBinHashMap::iterator binIt;
   
   const BBinIndex initialBinIndex = getBinIndex(size);
   BBinIndex binIndex;
   int chunkIndex = -1;
   for (binIndex = Math::Max(mLowestUsedBinIndex, initialBinIndex); binIndex < cNumBins; binIndex++)
   {
      binIt = mBins.find(binIndex);
      if (binIt == mBins.end())
      {
         if (initialBinIndex <= mLowestUsedBinIndex)
         {
            if (binIndex == mLowestUsedBinIndex)
               mLowestUsedBinIndex++;
         }
         continue;
      }
      
      const BChunkArray& chunks = binIt->second.mChunks;
            
      if (chunks.isEmpty())
         continue;
                  
      int first = 0;
      int count = chunks.getSize();

      for ( ; 0 < count; )
      {	
         // divide and conquer, find half that contains answer
         int count2 = count / 2;
         int mid = first + count2;

         if (chunks[mid].mSize < size)
         {
            first = ++mid;
            count -= count2 + 1;
         }
         else
            count = count2;
      }
      if (first < static_cast<int>(chunks.getSize()))
      {
         chunkIndex = first;
         break;
      }
   }
       
   if (binIndex == cNumBins)
   {
      if (pActualSize) 
         *pActualSize = 0;
      return NULL;
   }
            
   BBin& bin = binIt->second;
   BChunkArray& chunks = bin.mChunks;
   BChunk& chunk = chunks[chunkIndex];
   BASSERT(chunk.mSize >= size);
   
   if (chunkIndex > 0)
   {
      BASSERT(chunks[chunkIndex - 1].mSize < size);
   }
   
   void* pChunk = chunk.mPtr;
   uint chunkSize = chunk.mSize;
   
   void* pLeftOver = (void*)((uint)pChunk + size);
   const uint leftOverSize = chunkSize - size;
   
   bool wasFound = mFreeChunks.erase((uint)pChunk);
   wasFound;
   BASSERT(wasFound);

   if ((leftOverSize >= cAllocAlignment) && (getBinIndex(leftOverSize) == binIndex))
   {
      BPtrSizeHashMap::iterator it = mFreeChunks.find((uint)pChunk + (chunkSize - 1));
      BASSERT(it != mFreeChunks.end());
      it->second = leftOverSize;
      
      mFreeChunks.insert((uint)pLeftOver, leftOverSize);
      
      chunk.mPtr = pLeftOver;
      chunk.mSize = leftOverSize;      
      
      while (chunkIndex > 0)
      {
         BChunk& curChunk = chunks[chunkIndex];
         BChunk& prevChunk = chunks[chunkIndex - 1];
         if (curChunk.mSize >= prevChunk.mSize)
            break;
         std::swap(prevChunk, curChunk);
         chunkIndex--;
      }
   }
   else
   {
      wasFound = mFreeChunks.erase((uint)pChunk + (chunkSize - 1));
      BASSERT(wasFound);
         
      chunks.erase(chunkIndex);
      if (chunks.isEmpty())
      {
         mBins.erase(binIt);
         if (binIndex == mLowestUsedBinIndex)
            mLowestUsedBinIndex++;
      }
         
      BASSERT(chunkSize >= size);
      if (leftOverSize >= cAllocAlignment)
         addFreeChunk(pLeftOver, leftOverSize);
      else
         size = chunkSize; 
   }
   
   mAllocatedChunks.insert((uint)pChunk, size);
   
   mTotalBytesAllocated += size;
   BASSERT(mTotalBytesFree >= size);
   mTotalBytesFree -= size;
   
   BASSERT((mTotalBytesFree + mTotalBytesAllocated) == mTotalRegionBytes);
   
   if (pActualSize) 
      *pActualSize = size;
            
   return pChunk;
}

void BSFHeapAllocator::eraseFreeChunk(void* pChunk, uint chunkSize)
{
   bool wasFound = mFreeChunks.erase((uint)pChunk);
   wasFound;
   BASSERT(wasFound);
   
   wasFound = mFreeChunks.erase((uint)pChunk + (chunkSize - 1));
   BASSERT(wasFound);

   BBinIndex binIndex = getBinIndex(chunkSize);
   
   BBinHashMap::iterator binIt(mBins.find(binIndex));
   if (binIt == mBins.end())
   {
      BASSERT(0);
      return;
   }   
      
   BBin& existingBin = binIt->second;
   BChunkArray& chunks = existingBin.mChunks;
   
   int l = 0;
   int r = chunks.getSize() - 1;
   int m = 0;
   int compResult = 0;
   while (r >= l)
   {
      m = (l + r) >> 1;
      const BChunk& compChunk = chunks[m];

      compResult = compChunk.mSize - chunkSize;

      if (!compResult)
         break;
      else if (compResult > 0)
         r = m - 1;
      else
         l = m + 1;
   }
   
   if (compResult != 0)
   {
      BASSERT(0);
      return;
   }
   
   l = m;
   while (l >= 0)
   {
      if (chunks[l].mSize != chunkSize)
         break;
      else if (chunks[l].mPtr == pChunk)
      {
         chunks.erase(l);
         if (chunks.isEmpty())
         {
            mBins.erase(binIndex);
            if (binIndex == mLowestUsedBinIndex)
               mLowestUsedBinIndex++;
         }
         return;
      }
      l--;
   }
   
   l = m + 1;
   while (static_cast<uint>(l) < chunks.getSize())
   {
      if (chunks[l].mSize != chunkSize)
         break;
      else if (chunks[l].mPtr == pChunk)
      {
         chunks.erase(l);
         if (chunks.isEmpty())
         {
            mBins.erase(binIndex);
            if (binIndex == mLowestUsedBinIndex)
               mLowestUsedBinIndex++;
         }
         return;
      }
      l++;
   }
   
   BASSERT(0);
}

void BSFHeapAllocator::coaleseFreeChunk(void*& pChunk, uint& chunkSize)
{
   const uint leftPtr = (uint)pChunk - 1;
   BPtrSizeHashMap::iterator leftIt = mFreeChunks.find(leftPtr);
   const bool hasLeft = (leftIt != mFreeChunks.end());
   
   const uint rightPtr = (uint)pChunk + chunkSize;
   BPtrSizeHashMap::iterator rightIt = mFreeChunks.find(rightPtr);
   const bool hasRight = (rightIt != mFreeChunks.end());
            
   if (hasLeft)
   {
      BASSERT(leftPtr == (uint)leftIt->first);
      
      void* pLeft = (void*)(leftPtr - (leftIt->second - 1));
      uint leftSize = leftIt->second;
      
      BASSERT(((uint)pLeft + leftSize) == (uint)pChunk);
      
      eraseFreeChunk(pLeft, leftSize);
                  
      pChunk = (void*)pLeft;
      chunkSize += leftSize;
   }
   
   if (hasRight)
   {
      BASSERT(rightPtr == (uint)rightIt->first);
      
      void* pRight = (void*)rightPtr;
      uint rightSize = rightIt->second;
      
      BASSERT(((uint)pChunk + chunkSize) == (uint)pRight);
      
      eraseFreeChunk(pRight, rightSize);
      
      chunkSize += rightSize;
   }
}

void BSFHeapAllocator::addFreeChunk(void* pChunk, uint chunkSize)
{
   BASSERT(chunkSize >= cAllocAlignment);
   BASSERT((chunkSize & (cAllocAlignment - 1)) == 0);
   
   BBinIndex binIndex = getBinIndex(chunkSize);
         
   BBinHashMap::iterator it(mBins.find(binIndex));
   if (it == mBins.end())
   {
      BBin newBin;
      newBin.mChunks.pushBack(BChunk(pChunk, chunkSize));
      mBins.insert(binIndex, newBin);
   }
   else
   {
      BBin& existingBin = it->second;
      BChunkArray& chunks = existingBin.mChunks;
      
      if (chunks.isEmpty())
         chunks.pushBack(BChunk(pChunk, chunkSize));
      else
      {
         int l = 0;
         int r = chunks.getSize() - 1;
         int m = 0;
         int compResult = 0;
         while (r >= l)
         {
            m = (l + r) >> 1;
            const BChunk& compChunk = chunks[m];

            compResult = compChunk.mSize - chunkSize;

            if (!compResult)
               break;
            else if (compResult > 0)
               r = m - 1;
            else
               l = m + 1;
         }

         int result = m;
         if (compResult >= 0)
            result = m;
         else
            result = m + 1;
     
         chunks.insert(result, BChunk(pChunk, chunkSize));
      }
   }
   
   mLowestUsedBinIndex = Math::Min(mLowestUsedBinIndex, binIndex);
   
   BPtrSizeHashMap::InsertResult result(mFreeChunks.insert((uint)pChunk, chunkSize));
   BASSERT(result.second);
   
   result = mFreeChunks.insert((uint)pChunk + chunkSize - 1, chunkSize);
   BASSERT(result.second);
}

void BSFHeapAllocator::free(void* p)
{
   if (!p)
      return;
 
   mOperationIndex++;     
   
   if ((uint)p & (cAllocAlignment - 1))
   {
      BASSERT(0);
      return;  
   }
   
   if (!mTotalBytesAllocated)
   {
      BASSERT(0);
      return;  
   }
         
   BPtrSizeHashMap::iterator it(mAllocatedChunks.find((uint)p));
   if (it == mAllocatedChunks.end())
   {
      BASSERT(0);
      return;
   }

   uint chunkSize = it->second;
   
   mAllocatedChunks.erase(it);
   
   BASSERT(mTotalBytesAllocated >= chunkSize);
   mTotalBytesAllocated -= chunkSize;
   mTotalBytesFree += chunkSize;
   BASSERT(mTotalBytesFree <= mTotalRegionBytes);
   
   coaleseFreeChunk(p, chunkSize);
   addFreeChunk(p, chunkSize);
}

uint BSFHeapAllocator::getSize(void* p) const
{
   if (!p)
      return 0;
      
   BPtrSizeHashMap::const_iterator it(mAllocatedChunks.find((uint)p));
   if (it == mAllocatedChunks.end())
      return 0;
      
   return it->second;
}

BSFHeapAllocator::BBinIndex BSFHeapAllocator::getBinIndex(uint size) const
{
   BASSERT(size >= cAllocAlignment);
   
   size -= cAllocAlignment;
   
   // 0-63, multiples of 64
   if (size < 4096)
      return size >> 6;
   size -= 4096;
   
   // 64-191, multiples of 256
   if (size < 32768)
      return 64 + (size >> 8);
   size -= 32768;
   
   // 192-255, multiples of 16384
   BSFHeapAllocator::BBinIndex binIndex = Math::Min<int>(cNumBins - 1, 192 + (size >> 14));
   BASSERT(binIndex < cNumBins);
   return binIndex;
}

struct BFoundChunk
{
   BFoundChunk() { }
   BFoundChunk(void* p, uint size, bool free) : mPtr(p), mSize(size), mFree(free) { }
   
   void* mPtr;
   uint mSize;
   bool mFree;
   
   bool operator== (const BFoundChunk& rhs) const { return (uint)mPtr == (uint)rhs.mPtr; }
   bool operator< (const BFoundChunk& rhs) const { return (uint)mPtr < (uint)rhs.mPtr; }
};

bool BSFHeapAllocator::check(void) const
{
#define CHECK(x) do { if (!(x)) { DebugBreak(); return false; } } while(0)

   if (!mTotalRegionBytes)
   {
      CHECK(!mTotalBytesAllocated);
      CHECK(!mTotalBytesFree);
      CHECK(!mBins.getSize());
      CHECK(!mFreeChunks.getSize());
      CHECK(!mAllocatedChunks.getSize());
      return true;
   }
   
   CHECK((mTotalBytesFree + mTotalBytesAllocated) == mTotalRegionBytes);
   
   CHECK((mFreeChunks.getSize() & 1) == 0);
   
   BDynamicArray<BFoundChunk> foundChunks;
   
   uint totalFreeBytesFound = 0;
   uint totalChunksFound = 0;
   
   CHECK(mLowestUsedBinIndex <= cNumBins);
   
   for (uint binIndex = 0; binIndex < mLowestUsedBinIndex; binIndex++)
   {
      BBinHashMap::const_iterator binIt = mBins.find(binIndex);
      CHECK(binIt == mBins.end());
   }
   
   for (BBinHashMap::const_iterator binIt = mBins.begin(); binIt != mBins.end(); ++binIt)
   {
      const BBinIndex binIndex = binIt->first;
      const BBin& bin = binIt->second;
            
      const BChunkArray& chunks = bin.mChunks;
      CHECK(!chunks.isEmpty());
      
      int prevSize = 0;
      for (uint chunkIndex = 0; chunkIndex < chunks.getSize(); chunkIndex++)
      {
         const BChunk& chunk = chunks[chunkIndex];
         CHECK((int)chunk.mSize >= prevSize);
         CHECK(chunk.mSize <= mTotalRegionBytes);
         CHECK((chunk.mSize & (cAllocAlignment - 1)) == 0);
         CHECK(chunk.mSize >= cAllocAlignment);
         
         CHECK(getBinIndex(chunk.mSize) == binIndex);
         totalFreeBytesFound += chunk.mSize;
         
         prevSize = chunk.mSize;
         
         totalChunksFound++;
         
         BPtrSizeHashMap::const_iterator it(mFreeChunks.find((uint)chunk.mPtr));
         CHECK(it != mFreeChunks.end());
         CHECK(it->second == chunk.mSize);
         
         it = mFreeChunks.find((uint)chunk.mPtr + chunk.mSize - 1);
         CHECK(it != mFreeChunks.end());
         CHECK(it->second == chunk.mSize);
         
         foundChunks.pushBack(BFoundChunk(chunk.mPtr, chunk.mSize, true));
      }
   }
   
   CHECK(totalFreeBytesFound <= mTotalRegionBytes);
   CHECK(totalFreeBytesFound == mTotalBytesFree);
   CHECK((uint)mFreeChunks.getSize() == totalChunksFound * 2U);
   
   uint totalAllocatedBytesFound = 0;
   for (BPtrSizeHashMap::const_iterator it = mAllocatedChunks.begin(); it != mAllocatedChunks.end(); ++it)
   {
      totalAllocatedBytesFound += it->second;
      
      CHECK(it->second <= mTotalRegionBytes);
      CHECK((it->second & (cAllocAlignment - 1)) == 0);
      CHECK(it->second >= cAllocAlignment);
      
      foundChunks.pushBack(BFoundChunk((void*)it->first, it->second, false));
   }
   
   if (!foundChunks.isEmpty())
      std::sort(foundChunks.begin(), foundChunks.end());

   CHECK(totalAllocatedBytesFound == mTotalBytesAllocated);
   CHECK(totalAllocatedBytesFound <= mTotalRegionBytes);      
   
   CHECK((totalAllocatedBytesFound + totalFreeBytesFound) == mTotalRegionBytes);
      
   for (int i = 0; i < static_cast<int>(foundChunks.getSize()) - 1; i++)
   {
      const BFoundChunk& first = foundChunks[i];
      const BFoundChunk& second = foundChunks[i + 1];
      
      CHECK(((uint)first.mPtr + first.mSize) == (uint)second.mPtr);
      
      CHECK((!first.mFree) || (!second.mFree));
   }
   
   return true;
   
#undef CHECK
}
