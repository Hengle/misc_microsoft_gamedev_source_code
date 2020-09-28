//============================================================================
//
//  bfHeapAllocator.cpp
//
//  Copyright (c) 2008, Ensemble Studios
//
//  rg [1/23/08] - Best fit with coalescing allocator, modeled after dlmalloc's
//  smallblock allocator.
//
//============================================================================
#include "xcore.h"
#include "bfHeapAllocator.h"
#include "containers\hashMap.h"

#ifndef BUILD_FINAL
   // These options are VERY slow, only intended for debugging.
   //#define BFHEAP_EXHAUSTIVE_CHECKING
   //#define BFHEAP_SUPER_EXHAUSTIVE_CHECKING  
   //#define BFHEAP_BITMAP_CHECKS
#endif

#ifdef BUILD_DEBUG
   #define BFHEAP_GUARD_TAGS
   #define BFHEAP_FILL_ALLOCATED_AND_FREE_BLOCKS      
#endif      

#ifdef BFHEAP_BITMAP_CHECKS
   #include "containers\sparseBitArray.h"
#endif   

#ifdef BFHEAP_GUARD_TAGS
   #define BFHEAP_FIRST_GUARD_TAG   0xF1794466
   #define BFHEAP_LAST_GUARD_TAG    0xBEFF1234
#endif   

#ifdef BUILD_DEBUG
   #define BFHEAP_DEBUG_ASSERT(x) do { if (!(x)) heapCorruption(#x); } while(0)
#else
   #define BFHEAP_DEBUG_ASSERT(x) ((void)0)
#endif      

//============================================================================
// BBFHeapAllocator::BBFHeapAllocator
//============================================================================
BBFHeapAllocator::BBFHeapAllocator() : 
   mOperationIndex(0)
{
   clear();
}

//============================================================================
// BBFHeapAllocator::clear
//============================================================================
void BBFHeapAllocator::clear()
{
   mOperationIndex++;
   
   for (uint i = 0; i < cNumBins; i++)
      mBins[i].set(&mBins[i], &mBins[i]);
   
   mTotalAllocations = 0;   
   mTotalBytesAllocated = 0;
   mTotalBytesFree = 0;
   mTotalRegionBytes = 0;
      
   mLowestRegionPtr = UINT_MAX;
   mHighestRegionPtr = 0;
   mLowestAllocatedPtr = UINT_MAX;
   mHighestAllocatedPtr = 0;
   
   mpDV = NULL;
   
#ifdef BFHEAP_TRACK_ALLOCATED_BLOCKS
   mAllocatedBlocks.clear();
#endif
}

//============================================================================
// BBFHeapAllocator::heapCorruption
//============================================================================
void BBFHeapAllocator::heapCorruption(const char* pMsg) const
{
   gAssertionSystem.failNoReturn(pMsg, "BBFHeapAllocator: Heap corrupt, show this to RG!", __FILE__, __LINE__, true);
}

//============================================================================
// BBFHeapAllocator::eraseFreeChunk
//============================================================================
void BBFHeapAllocator::eraseFreeChunk(BFreeChunkHeader* pHeader)
{
   if (pHeader == mpDV)
      mpDV = NULL;
   else
      pHeader->mLink.remove();
}

//============================================================================
// BBFHeapAllocator::checkRegion
//============================================================================
bool BBFHeapAllocator::checkRegion(BFreeChunkHeader* pHeader, void*& pRegionStart, uint& regionSize, BFreeChunkHeader*& pBefore, BFreeChunkHeader*& pAfter)
{
   if (!pHeader->isPrevInUse())
      return false;
      
//-- FIXING PREFIX BUG ID 7828
   const uchar* pRegionEnd = static_cast<const uchar*>(pRegionStart) + regionSize;
//--
   
   uint chunkSize = pHeader->getSize();

   if (pRegionStart == (reinterpret_cast<uchar*>(pHeader) + chunkSize)) 
   {
      // Region starts immediately AFTER free chunk.
      if (!pHeader->isLast())   
         return false;
      
      if (pBefore)
         return false;
      pBefore = pHeader;
   }
   else if (pRegionEnd == reinterpret_cast<uchar*>(pHeader))
   {
      // Region starts immediately BEFORE free chunk.

      if (pAfter)
         return false;
      pAfter = pHeader;
   }
   else if (!((pRegionEnd < reinterpret_cast<uchar*>(pHeader)) || (pRegionStart >= (reinterpret_cast<uchar*>(pHeader) + chunkSize))))
      return false;
      
   return true;      
}

//============================================================================
// BBFHeapAllocator::addRegion
//============================================================================
bool BBFHeapAllocator::addRegion(void* pRegionStart, uint regionSize)
{
   mOperationIndex++;
   
   if ((!pRegionStart) || ((uint)pRegionStart & (cAllocAlignment - 1)) || (regionSize < cMinChunkSize) || (regionSize & (cAllocAlignment - 1)))
      return false;
   
   mTotalRegionBytes += regionSize;
   mTotalBytesFree += regionSize;
   
   mLowestRegionPtr = Math::Min(mLowestRegionPtr, (uint)pRegionStart);
   mHighestRegionPtr = Math::Max(mHighestRegionPtr, (uint)pRegionStart + regionSize);
   
   BFreeChunkHeader* pBefore = NULL;
   BFreeChunkHeader* pAfter = NULL;
      
   BFreeChunkLink* pFirstLink = mBins;
   for (uint binIndex = 0; binIndex < cNumBins; binIndex++, pFirstLink++)
   {
      BFreeChunkLink* pCurLink;
      BFreeChunkLink* pNextLink = pFirstLink->mpNextFree;;

      while (pNextLink != pFirstLink)
      {
         pCurLink = pNextLink;
         pNextLink = pCurLink->mpNextFree;;
         
         BFreeChunkHeader* pHeader = pCurLink->getHeaderPtr();
         if (!pHeader->isFree())
            return false;
            
         if (!checkRegion(pHeader, pRegionStart, regionSize, pBefore, pAfter))
            return false;
         
         if ((pBefore) && (pAfter))
            break;
      }
      
      if ((pBefore) && (pAfter))
         break;
   }
   
   uint regionChunkSizeField;
   if ((pBefore) || (pAfter))
   {
      if (pBefore)
      {
         eraseFreeChunk(pBefore);
         pRegionStart = pBefore;
         regionSize += pBefore->getSize(); 
      }
      
      if (pAfter)
      {
         eraseFreeChunk(pAfter);
         regionSize += pAfter->getSize(); 
      }
      
      regionChunkSizeField = regionSize | cPrevInUseFlag;
      if (pAfter)
      {
         if (pAfter->isLast())
            regionChunkSizeField |= cLastFlag;
      }
      else
         regionChunkSizeField |= cLastFlag;
   }         
   else
      regionChunkSizeField = regionSize | cLastFlag | cPrevInUseFlag;   
   
   BFreeChunkHeader* pRegionChunkHeader = reinterpret_cast<BFreeChunkHeader*>(pRegionStart);
   pRegionChunkHeader->mSize = regionChunkSizeField;
   *reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pRegionChunkHeader) + regionSize - sizeof(uint)) = regionSize;
   
   addToBin(pRegionChunkHeader, regionSize);
   
   return true;
}

//============================================================================
// BBFHeapAllocator::doRegionsIntersect
// Do region1 and region2 intersect?
//============================================================================
bool BBFHeapAllocator::doRegionsIntersect(void* pRegion1Start, uint region1Size, void* pRegion2Start, uint region2Size)
{
//-- FIXING PREFIX BUG ID 7824
   const void* pRegion1End = static_cast<uchar*>(pRegion1Start) + region1Size;
//--
//-- FIXING PREFIX BUG ID 7825
   const void* pRegion2End = static_cast<uchar*>(pRegion2Start) + region2Size;
//--
   
   if ((pRegion2End < pRegion1Start) || (pRegion2Start >= pRegion1End))
      return false;
   
   return true;
}

//============================================================================
// BBFHeapAllocator::isRegionContained
// Is region2 completely within region1?
//============================================================================
bool BBFHeapAllocator::isRegionContained(void* pRegion1Start, uint region1Size, void* pRegion2Start, uint region2Size)
{
//-- FIXING PREFIX BUG ID 7826
   const void* pRegion1End = static_cast<uchar*>(pRegion1Start) + region1Size;
//--
//-- FIXING PREFIX BUG ID 7827
   const void* pRegion2End = static_cast<uchar*>(pRegion2Start) + region2Size;
//--
   
   if ((pRegion2Start >= pRegion1Start) && (pRegion2End <= pRegion1End))
      return true;
   
   return false;
}

//============================================================================
// BBFHeapAllocator::isRegionFree
//============================================================================
bool BBFHeapAllocator::isRegionFree(void* pRegionStart, uint regionSize)
{
   if ((!pRegionStart) || ((uint)pRegionStart & (cAllocAlignment - 1)) || (regionSize < cMinChunkSize) || (regionSize & (cAllocAlignment - 1)))
      return false;
      
   if (mpDV)
   {
      if (isRegionContained(mpDV, mpDV->getSize(), pRegionStart, regionSize))
         return true;
   }

   BFreeChunkLink* pFirstLink = mBins;
   for (uint binIndex = 0; binIndex < cNumBins; binIndex++, pFirstLink++)
   {
      BFreeChunkLink* pCurLink;
      BFreeChunkLink* pNextLink = pFirstLink->mpNextFree;;

      while (pNextLink != pFirstLink)
      {
         pCurLink = pNextLink;
         pNextLink = pCurLink->mpNextFree;;

         BFreeChunkHeader* pHeader = pCurLink->getHeaderPtr();
         if (pHeader->isFree())
         {
            if (isRegionContained(pHeader, pHeader->getSize(), pRegionStart, regionSize))
               return true;
         }               
      }
   }               

   return false;
}

//============================================================================
// BBFHeapAllocator::removeFreeRegions
//============================================================================
bool BBFHeapAllocator::removeFreeRegions(const BRegion* pRegions, uint numRegions)
{
   mOperationIndex++;
   
   if ((!pRegions) || (!numRegions))
      return false;
      
   uint numRegionsFound = 0;
         
   if (mpDV)
   {
      for (uint i = 0; i < numRegions; i++)
      {
         if (pRegions[i].mpStart == (void*)mpDV)
         {
            if (pRegions[i].mSize != mpDV->getSize())
               return false;
               
            if ((!mpDV->isLast()) || (!mpDV->isPrevInUse()))
               return false;
               
            numRegionsFound++;
            break;
         }
      }
   }      
   
   BFreeChunkLink* pFirstLink = mBins;
   for (uint binIndex = 0; binIndex < cNumBins; binIndex++, pFirstLink++)
   {
      BFreeChunkLink* pCurLink;
      BFreeChunkLink* pNextLink = pFirstLink->mpNextFree;;

      while (pNextLink != pFirstLink)
      {
         pCurLink = pNextLink;
         pNextLink = pCurLink->mpNextFree;;

         BFreeChunkHeader* pHeader = pCurLink->getHeaderPtr();
         if (pHeader->isFree())
         {
            for (uint i = 0; i < numRegions; i++)
            {
               if (pRegions[i].mpStart == (void*)pHeader)
               {
                  if (pRegions[i].mSize != pHeader->getSize())
                     return false;

                  if ((!pHeader->isLast()) || (!pHeader->isPrevInUse()))
                     return false;
                  
                  numRegionsFound++;   
                  break;
               }
            }
            
            if (numRegionsFound == numRegions)
               break;
         }               
      }
      
      if (numRegionsFound == numRegions)
         break;
   }               
   
   if (numRegionsFound != numRegions)
      return false;
      
   for (uint i = 0; i < numRegions; i++)
   {
      BFreeChunkHeader* pHeader = static_cast<BFreeChunkHeader*>(pRegions[i].mpStart);
      const uint size = pHeader->getSize();
      
      BDEBUG_ASSERT(size == pRegions[i].mSize);
      
      if ((!pHeader->isFree()) || (!pHeader->isLast()) || (!pHeader->isPrevInUse()))
         return false;

      eraseFreeChunk(pHeader);
      
      if ((mTotalRegionBytes < size) || (mTotalBytesFree < size))
         return false;
         
      mTotalRegionBytes -= size;
      mTotalBytesFree -= size;
   }         

   return true;
}

//============================================================================
// BBFHeapAllocator::getAllocInfo
//============================================================================
void BBFHeapAllocator::getAllocInfo(uint& totalRegionBytes, uint& totalNumAllocations, uint& totalBytesAllocated, uint& totalBytesFree, uint& totalOperations) const
{
   totalRegionBytes = mTotalRegionBytes;
   totalNumAllocations = mTotalAllocations;
   totalBytesAllocated = mTotalBytesAllocated;
   totalBytesFree = mTotalBytesFree;
   totalOperations = mOperationIndex;
}

//============================================================================
// BBFHeapAllocator::getMemoryInfo
//============================================================================
void BBFHeapAllocator::getMemoryInfo(uint& totalFreeChunks, uint& largestFreeChunk, uint& totalBins, uint& totalOverhead, uint& dvSize) const
{
   totalFreeChunks = 0;
   largestFreeChunk = 0;
   totalBins = 0;
   dvSize = 0;
   totalOverhead = sizeof(BBFHeapAllocator) + mTotalAllocations * sizeof(uint);
   
   if (mpDV)
   {
      totalFreeChunks++;
      largestFreeChunk = mpDV->getSize();
      dvSize = mpDV->getSize();
   }
   
   const BFreeChunkLink* pFirstLink = mBins;
   for (uint binIndex = 0; binIndex < cNumBins; binIndex++, pFirstLink++)
   {
      const BFreeChunkLink* pCurLink;
      const BFreeChunkLink* pNextLink = pFirstLink->mpNextFree;;

      if (pNextLink != pFirstLink)
         totalBins++;
      
      while (pNextLink != pFirstLink)
      {
         pCurLink = pNextLink;
         pNextLink = pCurLink->mpNextFree;
                  
         const BFreeChunkHeader* pHeader = pCurLink->getHeaderPtr();
         if (pHeader->isFree())
         {
            totalFreeChunks++;
            
            largestFreeChunk = Math::Max(largestFreeChunk, pHeader->getSize());
         }
      }
   }
}

//============================================================================
// BBFHeapAllocator::addToBin
//============================================================================
void BBFHeapAllocator::addToBin(BFreeChunkHeader* pChunkHeader, uint chunkSize)
{
   uint binIndex = getBinIndex(chunkSize);
   
   BFreeChunkLink* pFirstLink = &mBins[binIndex];
   BFreeChunkLink* pCurLink = pFirstLink;
   
   if (binIndex >= cNumExactChunkBins)
   {
      BFreeChunkLink* pNextLink = pCurLink->mpNextFree;
      
      while (pNextLink != pFirstLink)
      {
         if (pNextLink->getHeaderPtr()->getSize() >= chunkSize)
            break;

         pCurLink = pNextLink;
         pNextLink = pCurLink->mpNextFree;
      }     
   }      

   pCurLink->insertAfter(&pChunkHeader->mLink);
}

//============================================================================
// BBFHeapAllocator::findFreeChunk
//============================================================================
BBFHeapAllocator::BFreeChunkLink* BBFHeapAllocator::findFreeChunk(uint size) const
{
   uint initialBinIndex = getBinIndex(size);

   BFreeChunkLink* pCurLink = NULL;

   uint binIndex;
   const BFreeChunkLink* pFirstLink = &mBins[initialBinIndex];
   
   if ((initialBinIndex < cNumExactChunkBins) && (pFirstLink->mpNextFree != pFirstLink))
      return pFirstLink->mpNextFree;
      
   if ((mpDV) && (mpDV->getSize() >= size))
      return &mpDV->mLink;
      
   for (binIndex = initialBinIndex; binIndex < cNumBins; binIndex++, pFirstLink++)
   {
      pCurLink = pFirstLink->mpNextFree;

      if (pCurLink == pFirstLink)
         continue;

      if (binIndex < cNumExactChunkBins)
         return pCurLink;

      do  
      {
         const BFreeChunkHeader* pHeader = pCurLink->getHeaderPtr();
         BFHEAP_DEBUG_ASSERT(pHeader->isFree());

         if (pHeader->getSize() >= size)
            return pCurLink;

         pCurLink = pCurLink->mpNextFree;

      } while (pCurLink != pFirstLink);
   }

   return NULL;
}

//============================================================================
// BBFHeapAllocator::alloc
//============================================================================
void* BBFHeapAllocator::alloc(uint requestedSize, uint* pActualSize)
{
#ifdef BFHEAP_GUARD_TAGS
   uint rawSize;
   uchar* p = (uchar*)allocInternal(requestedSize + sizeof(DWORD) * 2, &rawSize);
   if (!p)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
   
   const uint blockSize = rawSize - sizeof(DWORD) * 2;
   if (pActualSize) *pActualSize = blockSize;
   
   *reinterpret_cast<DWORD*>(p) = BFHEAP_FIRST_GUARD_TAG;
   *reinterpret_cast<DWORD*>(p + sizeof(DWORD) + blockSize) = BFHEAP_LAST_GUARD_TAG;

#ifdef BFHEAP_FILL_ALLOCATED_AND_FREE_BLOCKS         
   Utils::FastMemSet(p + sizeof(DWORD), 0xCD, blockSize);
#endif   
   
   void* pResult = p + sizeof(DWORD);
   
   BDEBUG_ASSERT(getSize(pResult) == blockSize);
   
   return pResult;
#else
   return allocInternal(requestedSize, pActualSize);
#endif
}

//============================================================================
// BBFHeapAllocator::free
//============================================================================
bool BBFHeapAllocator::free(void* const p)
{
#ifdef BFHEAP_GUARD_TAGS
   if (!p)
      return false;
   
   uchar* pRaw = static_cast<uchar*>(p) - sizeof(DWORD);
   if (*(DWORD*)pRaw != BFHEAP_FIRST_GUARD_TAG)
      heapCorruption("BFHeapAllocator::free: First guard tag overwritten");
      
   const uint rawSize = getSizeInternal(pRaw);
   if (!rawSize)      
      heapCorruption("BFHeapAllocator::free: getSizeInternal failed");
      
   if (*(DWORD*)(pRaw + rawSize - sizeof(DWORD)) != BFHEAP_LAST_GUARD_TAG)
      heapCorruption("BFHeapAllocator::free: Last guard tag overwritten");
      
   return freeInternal(pRaw);
#else
   return freeInternal(p);
#endif
}

//============================================================================
// BBFHeapAllocator::getSize
//============================================================================
uint BBFHeapAllocator::getSize(void* p) const
{
#ifdef BFHEAP_GUARD_TAGS
   if (!p)
      return false;

   uchar* pRaw = static_cast<uchar*>(p) - sizeof(DWORD);
   if (*(DWORD*)pRaw != BFHEAP_FIRST_GUARD_TAG)
      heapCorruption("BFHeapAllocator::getSize: First guard tag overwritten");

   const uint rawSize = getSizeInternal(pRaw);
   if (!rawSize)      
      heapCorruption("BFHeapAllocator::getSize: getSizeInternal failed");

   if (*(DWORD*)(pRaw + rawSize - sizeof(DWORD)) != BFHEAP_LAST_GUARD_TAG)
      heapCorruption("BFHeapAllocator::getSize: Last guard tag overwritten");

   return rawSize - sizeof(DWORD) * 2;
#else
   return getSizeInternal(p);
#endif
}

//============================================================================
// BBFHeapAllocator::allocInternal
//============================================================================
void* BBFHeapAllocator::allocInternal(uint requestedSize, uint* pActualSize)
{
#ifdef BFHEAP_SUPER_EXHAUSTIVE_CHECKING  
   BVERIFY(check());
#endif

   mOperationIndex++;
         
   uint neededChunkSize = requestedSize;
   neededChunkSize = Utils::AlignUpValue(sizeof(uint) + neededChunkSize, cAllocAlignment);
   if (neededChunkSize < cMinChunkSize)
      neededChunkSize = cMinChunkSize;
         
   if (mTotalBytesFree < neededChunkSize)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
   
   BFreeChunkLink* pLink = findFreeChunk(neededChunkSize);
   if (!pLink)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
      
   BFreeChunkHeader* pHeader = pLink->getHeaderPtr();
   uint foundChunkSize = pHeader->getSize();
   
   BFHEAP_DEBUG_ASSERT(pHeader->isFree() && pHeader->isPrevInUse());
   BFHEAP_DEBUG_ASSERT(foundChunkSize >= cMinChunkSize);
   BFHEAP_DEBUG_ASSERT(foundChunkSize >= neededChunkSize);
   BFHEAP_DEBUG_ASSERT(foundChunkSize <= mTotalRegionBytes);
   BFHEAP_DEBUG_ASSERT(*reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pHeader) + foundChunkSize - sizeof(uint)) == foundChunkSize);
            
   void* pResult = &pHeader->mLink;
      
   uint leftoverSize = foundChunkSize - neededChunkSize;   
   BFHEAP_DEBUG_ASSERT((leftoverSize & (cAllocAlignment - 1)) == 0);
   
   if (leftoverSize < cMinChunkSize)
   {
      pHeader->mSize |= cInUseFlag;
      
      if (!pHeader->isLast())   
      {
         BFreeChunkHeader* pNextHeader = reinterpret_cast<BFreeChunkHeader*>(reinterpret_cast<uchar*>(pHeader) + foundChunkSize);

         BFHEAP_DEBUG_ASSERT(!pNextHeader->isPrevInUse());
         BFHEAP_DEBUG_ASSERT(pNextHeader->isInUse());
         
         pNextHeader->mSize |= cPrevInUseFlag;
      }
      
      eraseFreeChunk(pHeader);
   }
   else
   {
      const uint origChunkSize = pHeader->mSize;
      
      pHeader->mSize = neededChunkSize | cInUseFlag | cPrevInUseFlag;
                                    
      BFreeChunkHeader* pNewHeader = reinterpret_cast<BFreeChunkHeader*>(reinterpret_cast<uchar*>(pHeader) + neededChunkSize);
      pNewHeader->mSize = cPrevInUseFlag | (origChunkSize & cLastFlag) | leftoverSize;
      *reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pNewHeader) + leftoverSize - sizeof(uint)) = leftoverSize;
            
      if (pHeader != mpDV)
      {
         pLink->remove();
         
         if (mpDV)
            addToBin(mpDV, mpDV->getSize());
      }  
      
      mpDV = pNewHeader;       
      
#ifdef BUILD_DEBUG
      mpDV->mLink.clear();
#endif
      
      foundChunkSize = neededChunkSize;
   }
   
#ifdef BUILD_DEBUG
   pLink->clear();
#endif   
   
   if (pActualSize) *pActualSize = foundChunkSize - sizeof(uint);
   
   mTotalAllocations++;
   mTotalBytesAllocated += foundChunkSize;
   BFHEAP_DEBUG_ASSERT(mTotalBytesFree >= foundChunkSize);
   mTotalBytesFree -= foundChunkSize;
   
   mLowestAllocatedPtr = Math::Min(mLowestAllocatedPtr, (uint)pResult);
   mHighestAllocatedPtr = Math::Max(mHighestAllocatedPtr, (uint)pResult);

#ifdef BFHEAP_TRACK_ALLOCATED_BLOCKS
   BAllocatedBlackHashMap::InsertResult result(mAllocatedBlocks.insert(pResult, foundChunkSize));
   BASSERT(result.second);
#endif

#ifdef BFHEAP_EXHAUSTIVE_CHECKING  
   BVERIFY(check());
#endif
   
   return pResult;
}

//============================================================================
// BBFHeapAllocator::freeInternal
//============================================================================
bool BBFHeapAllocator::freeInternal(void* const p)
{
#ifdef BFHEAP_SUPER_EXHAUSTIVE_CHECKING  
   BVERIFY(check());
#endif

   mOperationIndex++;
            
   if (!p)  
      return false;
      
   if (((uint)p < mLowestAllocatedPtr) || ((uint)p > mHighestAllocatedPtr))
      return false;
   
   if (!mTotalAllocations)
      return false;
   
   uint* pChunkSize = reinterpret_cast<uint*>(p) - 1;
   
   if (!Utils::IsAligned(pChunkSize, cAllocAlignment))
      return false;
      
   if ((*pChunkSize & cInUseFlag) == 0)
      return false;
   
   uint chunkSize = *pChunkSize & cSizeMask; 
   if ((chunkSize < cMinChunkSize) || (chunkSize > mTotalBytesAllocated))
      return false;

#ifdef BFHEAP_TRACK_ALLOCATED_BLOCKS
   BAllocatedBlackHashMap::const_iterator findIt(mAllocatedBlocks.find(p));
   BASSERT(findIt != mAllocatedBlocks.end());
   BASSERT((findIt->first == p) && (findIt->second == chunkSize));
#endif      

   mTotalBytesAllocated -= chunkSize;
   mTotalAllocations--;
   mTotalBytesFree += chunkSize;
   if (mTotalBytesFree > mTotalRegionBytes)
      return false;

#ifdef BFHEAP_FILL_ALLOCATED_AND_FREE_BLOCKS      
   Utils::FastMemSet(pChunkSize + 1, 0xFE, chunkSize - sizeof(uint));
#endif   
         
   BFreeChunkHeader* pNewFreeChunk;
   
   if (*pChunkSize & cPrevInUseFlag)
   {
      pNewFreeChunk = reinterpret_cast<BFreeChunkHeader*>(pChunkSize);

      pNewFreeChunk->mSize &= ~cInUseFlag;
   }
   else
   {
      uint prevChunkSize = *(reinterpret_cast<uint*>(p) - 2);
      
      BFreeChunkHeader* pPrevFreeChunk = reinterpret_cast<BFreeChunkHeader*>(reinterpret_cast<uchar*>(p) - sizeof(uint) - prevChunkSize);
      
      if (pPrevFreeChunk->getSize() != prevChunkSize)
         return false;
      if (pPrevFreeChunk->isLast())
         return false;
      if (pPrevFreeChunk->isInUse())
         return false;
      if (!pPrevFreeChunk->isPrevInUse())
         return false;
      
      pPrevFreeChunk->setSize(pPrevFreeChunk->getSize() + chunkSize);
      if (*pChunkSize & cLastFlag)
         pPrevFreeChunk->mSize |= cLastFlag;
         
      if (pPrevFreeChunk != mpDV)
         pPrevFreeChunk->mLink.remove();
      
      pNewFreeChunk = pPrevFreeChunk;
   }
   
   if (!pNewFreeChunk->isLast())
   {
      BFreeChunkHeader* pNextChunk = reinterpret_cast<BFreeChunkHeader*>(reinterpret_cast<uchar*>(pNewFreeChunk) + pNewFreeChunk->getSize());
      BFHEAP_DEBUG_ASSERT(pNextChunk->isPrevInUse());
                  
      if (pNextChunk->isFree())
      {
         uint nextFreeChunkFooter = *reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pNextChunk) + pNextChunk->getSize() - sizeof(uint));
         if (nextFreeChunkFooter != pNextChunk->getSize())
            return false;
         
         BFHEAP_DEBUG_ASSERT( (pNextChunk->getSize() >= cMinChunkSize) && (pNextChunk->getSize() <= mTotalBytesFree) );
                           
         pNewFreeChunk->setSize(pNewFreeChunk->getSize() + pNextChunk->getSize());
         if (pNextChunk->isLast())
            pNewFreeChunk->mSize |= cLastFlag;
            
         if (pNextChunk == mpDV)
            mpDV = pNewFreeChunk;
         else
            pNextChunk->mLink.remove();
      }            
      else
      {
         BFHEAP_DEBUG_ASSERT( (pNextChunk->getSize() >= cMinChunkSize) && (pNextChunk->getSize() <= mTotalBytesAllocated) );
         
         pNextChunk->mSize &= ~cPrevInUseFlag;
      }
   }
   
   const uint newFreeChunkSize = pNewFreeChunk->getSize();
   BFHEAP_DEBUG_ASSERT((newFreeChunkSize & (cAllocAlignment - 1)) == 0);
   
   BFHEAP_DEBUG_ASSERT(newFreeChunkSize >= cMinChunkSize);
   BFHEAP_DEBUG_ASSERT(pNewFreeChunk->isPrevInUse());
   BFHEAP_DEBUG_ASSERT(pNewFreeChunk->isFree());
      
   *reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pNewFreeChunk) + newFreeChunkSize - sizeof(uint)) = newFreeChunkSize;
   
   if (mpDV != pNewFreeChunk)
      addToBin(pNewFreeChunk, newFreeChunkSize);
      
#ifdef BFHEAP_TRACK_ALLOCATED_BLOCKS
   mAllocatedBlocks.erase(p);
#endif      

#ifdef BFHEAP_EXHAUSTIVE_CHECKING  
   BVERIFY(check());
#endif
   
   return true;
}

//============================================================================
// BBFHeapAllocator::getSizeInternal
//============================================================================
uint BBFHeapAllocator::getSizeInternal(void* p) const
{
   if (!p)  
      return 0;

   if (!mTotalAllocations)
      return 0;
   
   if (((uint)p < mLowestAllocatedPtr) || ((uint)p > mHighestAllocatedPtr))
      return 0;
   
   uint* pChunkSize = reinterpret_cast<uint*>(p) - 1;

   if (!Utils::IsAligned(pChunkSize, cAllocAlignment))
      return false;
      
   if ((*pChunkSize & cInUseFlag) == 0)
      return false;

   uint chunkSize = *pChunkSize & cSizeMask; 
   if ((chunkSize < cMinChunkSize) || (chunkSize > mTotalBytesAllocated))
      return false;

   return chunkSize - sizeof(uint);
}

//============================================================================
// BBFHeapAllocator::alignedAlloc
//============================================================================
void* BBFHeapAllocator::alignedAlloc(uint requestedSize, uint* pActualSize)
{
   if (requestedSize < sizeof(uint))
      requestedSize = sizeof(uint);

   requestedSize = Utils::AlignUpValue(requestedSize, sizeof(uint));

   uint alignment = 4;
   if ((requestedSize & 7) == 0)
   {
      if ((requestedSize & 15) == 0)   
         alignment = 16;
      else
         alignment = 8;
   }

   uint rawSize = requestedSize + (alignment - sizeof(uint));

#ifdef BFHEAP_GUARD_TAGS   
   rawSize += sizeof(uint);
#endif

   uint actualRawSize;         
   void* p = alloc(rawSize, &actualRawSize);
   if (!p)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }

   BDEBUG_ASSERT((uint(p) & 3) == 0);
   BCOMPILETIMEASSERT(BBFHeapAllocator::cInUseFlag == 2);
      
#ifdef BFHEAP_GUARD_TAGS
   uchar* pAligned = (uchar*)Utils::AlignDown(static_cast<uchar*>(p) + alignment, alignment);
   *(reinterpret_cast<DWORD*>(pAligned) - 1) = reinterpret_cast<DWORD>(p);
#else
   uchar* pAligned = (uchar*)Utils::AlignUp(p, alignment);
   if (alignment >= 8)
      *(reinterpret_cast<DWORD*>(pAligned) - 1) = reinterpret_cast<DWORD>(p);
   BDEBUG_ASSERT((pAligned - (uchar*)p) <= (int)(alignment - sizeof(uint)));      
#endif      
   
   BDEBUG_ASSERT((uint)((pAligned + requestedSize) - (uchar*)p) <= rawSize);

   if (pActualSize) *pActualSize = actualRawSize - (pAligned - (uchar*)p);
   
   BDEBUG_ASSERT(Utils::IsAligned(pAligned, alignment));

#ifdef BUILD_DEBUG   
   {
      void* pRaw = getRawHeapPtr(pAligned);
      pRaw;
      BDEBUG_ASSERT(pRaw == p);
      BDEBUG_ASSERT(getSize(pRaw) == actualRawSize);
      if (pActualSize)
      {  
         BDEBUG_ASSERT(alignedGetSize(pAligned) == *pActualSize);
      }
   }
#endif   
   
   return pAligned;
}

//============================================================================
// BBFHeapAllocator::getRawHeapPtr
//============================================================================
void* BBFHeapAllocator::getRawHeapPtr(void* p)
{
   void* pRaw = p;
   DWORD sizeField = *(static_cast<DWORD*>(p) - 1);

#ifdef BFHEAP_GUARD_TAGS
   pRaw = (void*)sizeField;      
#else
   if ((sizeField & BBFHeapAllocator::cInUseFlag) == 0)
      pRaw = (void*)sizeField;
#endif   

   BDEBUG_ASSERT(
      (reinterpret_cast<DWORD>(pRaw) >= 65536) && 
      (pRaw <= p) && 
      ((reinterpret_cast<uchar*>(p) - reinterpret_cast<uchar*>(pRaw)) <= 16) && 
      ((reinterpret_cast<DWORD>(pRaw) & 3) == 0) );

   return pRaw;      
}

//============================================================================
// BBFHeapAllocator::alignedFree
//============================================================================
bool BBFHeapAllocator::alignedFree(void* p)
{
   if ((!p) || ((uint)p & 3))
      return false;

   return free(getRawHeapPtr(p));
}

//============================================================================
// BBFHeapAllocator::alignedGetSize
//============================================================================
uint BBFHeapAllocator::alignedGetSize(void* p) const
{
   if ((!p) || ((uint)p & 3))
      return 0;

   uchar* pRawHeapPtr = static_cast<uchar*>(getRawHeapPtr(p));
   uint rawSize = getSize(pRawHeapPtr);
   if (rawSize)
   {
      uint overhead = static_cast<uchar*>(p) - pRawHeapPtr;
      if (rawSize < overhead)
         return 0;
      rawSize -= overhead;
   }
   
   return rawSize;
}

#ifndef BUILD_FINAL
   #define CHECK(x) do { if (!(x)) { BASSERTM(0, #x); return false; } } while(0)
#else
   #define CHECK(x) do { if (!(x)) return false; } while(0)
#endif

//============================================================================
// BBFHeapAllocator::isValidLinkPtr
//============================================================================
bool BBFHeapAllocator::isValidLinkPtr(const BFreeChunkLink* pLink) const
{
   if ( (pLink >= mBins) && (pLink < &mBins[cNumBins]) )
      return true;
      
   if ( ((void*)pLink >= (void*)mLowestRegionPtr) && ((void*)pLink < (void*)mHighestRegionPtr) )
      return true;
   
   return false;
}

//============================================================================
// BBFHeapAllocator::checkFreeChunk
//============================================================================
bool BBFHeapAllocator::checkFreeChunk(BFreeChunkLink* pCurLink) const
{
   CHECK(isValidLinkPtr(pCurLink));
   
   if (pCurLink->getHeaderPtr() != mpDV)
   {
      if (pCurLink->mpPrevFree == pCurLink->mpNextFree)
      {
         CHECK( (pCurLink->mpPrevFree >= mBins) && (pCurLink->mpPrevFree < &mBins[cNumBins]) );
      }
      CHECK(isValidLinkPtr(pCurLink->mpPrevFree));
      CHECK(isValidLinkPtr(pCurLink->mpNextFree));
   }      
   
   const BFreeChunkHeader* pHeader = pCurLink->getHeaderPtr();

   CHECK(pHeader->isFree());
   CHECK(pHeader->isPrevInUse());

   const uint chunkSize = pHeader->getSize();
   
   CHECK(chunkSize >= cMinChunkSize);
   CHECK(chunkSize <= mTotalBytesFree);
   CHECK(chunkSize <= mTotalRegionBytes);

   CHECK((uint)pHeader >= mLowestRegionPtr);
   CHECK(((uint64)((uint)pHeader) + (uint64)chunkSize) <= (uint64)mHighestRegionPtr);
   
   uint chunkFooter = *reinterpret_cast<const uint*>(reinterpret_cast<const uchar*>(pHeader) + chunkSize - sizeof(uint));
   CHECK(chunkFooter == chunkSize);

   if (!pHeader->isLast())
   {
      const BFreeChunkHeader* pNextHeader = reinterpret_cast<const BFreeChunkHeader*>(reinterpret_cast<const uchar*>(pHeader) + chunkSize);
            
      CHECK(((uint)pNextHeader + sizeof(uint)) <= mHighestRegionPtr);
            
      const uint nextChunkSize = pNextHeader->getSize();
      
      CHECK(((uint64)((uint)pNextHeader) + (uint64)nextChunkSize) <= (uint64)mHighestRegionPtr);

      CHECK(!pNextHeader->isPrevInUse());
      CHECK(pNextHeader->isInUse());
      CHECK(nextChunkSize >= cMinChunkSize);
      CHECK(nextChunkSize <= mTotalBytesAllocated);
      CHECK(((uint)pNextHeader + sizeof(uint)) >= mLowestAllocatedPtr);
      CHECK(((uint)pNextHeader + sizeof(uint)) <= mHighestAllocatedPtr);
   }

   return true;
}

//============================================================================
// BBFHeapAllocator::check
//============================================================================
bool BBFHeapAllocator::check(void) const
{
   CHECK(mTotalBytesFree + mTotalBytesAllocated == mTotalRegionBytes);

   BDynamicArray<const BFreeChunkHeader*> freeChunks;
   typedef BHashMap<const BFreeChunkHeader*, BEmptyStruct, BBitHasher<const BFreeChunkHeader*> > BFreeChunkHeaderPtrHashMap;
   BFreeChunkHeaderPtrHashMap foundChunks;
         
   uint totalFreeBytes = 0;
   uint largestFreeChunk = 0;
   
   if (mpDV)
   {
      CHECK(Utils::IsAligned(mpDV, 8));
      
      if (!checkFreeChunk(&mpDV->mLink))
         return false;
         
      largestFreeChunk = mpDV->getSize();
      freeChunks.push_back(mpDV);
      foundChunks.insert(mpDV);
      
      totalFreeBytes += mpDV->getSize();
      CHECK(totalFreeBytes <= mTotalBytesFree);
      CHECK(totalFreeBytes <= mTotalRegionBytes);
   }
   
   uint binSize[cNumBins];
   Utils::ClearObj(binSize);

#ifdef BFHEAP_BITMAP_CHECKS   
   BSparseBitArray isAreaFree(&gCRunTimeHeap);
   isAreaFree.resize((mHighestRegionPtr - mLowestRegionPtr + 7) >> 3);
   
   BSparseBitArray isAreaAllocated(&gCRunTimeHeap);
   isAreaAllocated.resize((mHighestRegionPtr - mLowestRegionPtr + 7) >> 3);
#endif   
            
   const BFreeChunkLink* pFirstLink = mBins;
   for (uint binIndex = 0; binIndex < cNumBins; binIndex++, pFirstLink++)
   {
      BFreeChunkLink* pCurLink;
      BFreeChunkLink* pNextLink = pFirstLink->mpNextFree;;
      
      uint prevChunkSize = 0;

      while (pNextLink != pFirstLink)
      {
         binSize[binIndex]++;
         
         pCurLink = pNextLink;
         pNextLink = pCurLink->mpNextFree;
                  
         CHECK(pNextLink->mpPrevFree == pCurLink);
         CHECK(pCurLink->mpPrevFree->mpNextFree == pCurLink);      
         
         if (!checkFreeChunk(pCurLink))
            return false;
         
         const BFreeChunkHeader* pHeader = pCurLink->getHeaderPtr();
                           
         const uint chunkSize = pHeader->getSize();
         
         CHECK(Utils::IsAligned(pHeader, 8));
                  
         CHECK(getBinIndex(pHeader->getSize()) == binIndex);
         
         totalFreeBytes += chunkSize;         
         CHECK(totalFreeBytes <= mTotalBytesFree);
         CHECK(totalFreeBytes <= mTotalRegionBytes);
         
         BFreeChunkHeaderPtrHashMap::InsertResult insertResult(foundChunks.insert(pHeader));
         CHECK(insertResult.second);
         
         freeChunks.pushBack(pHeader);

#ifdef BFHEAP_BITMAP_CHECKS         
         const uint firstAreaIndex = ((uint)pHeader - mLowestRegionPtr) >> 3;
         const uint lastAreaIndex = ((uint)pHeader - mLowestRegionPtr + chunkSize) >> 3;
         for (uint i = firstAreaIndex; i < lastAreaIndex; i++)
         {
            CHECK(!isAreaFree.isBitSet(i));
            isAreaFree.setBit(i);
         }
#endif         
         
         largestFreeChunk = Math::Max(largestFreeChunk, chunkSize);
       
         CHECK(chunkSize >= prevChunkSize);
         prevChunkSize = chunkSize;
      }
   }
   
   CHECK(totalFreeBytes == mTotalBytesFree);
   CHECK((mTotalBytesFree + mTotalBytesAllocated) == mTotalRegionBytes);
   
   freeChunks.sort();
   
   if (totalFreeBytes == mTotalRegionBytes)
   {
      for (uint i = 0; i < freeChunks.getSize(); i++)
      {
         const BFreeChunkHeader* pChunkHeader = freeChunks[i];
         CHECK(pChunkHeader->isLast());
      }
   }   
   
   for (int i = 0; i < static_cast<int>(freeChunks.getSize()) - 1; i++)
   {
      const BFreeChunkHeader* pCurChunkHeader = freeChunks[i];
      uint curChunkSize = pCurChunkHeader->getSize();
      
      const BFreeChunkHeader* pNextChunkHeader = freeChunks[i + 1];
      //uint nextChunkSize = pNextChunkHeader->getSize();
      
      // Can't do this because regions may be added at any time.
      //CHECK((reinterpret_cast<const uchar*>(pCurChunkHeader) + curChunkSize) <= (reinterpret_cast<const uchar*>(pNextChunkHeader) - cMinChunkSize));
      CHECK((reinterpret_cast<const uchar*>(pCurChunkHeader) + curChunkSize) <= (reinterpret_cast<const uchar*>(pNextChunkHeader)));
   }   
   
   if (freeChunks.getSize())
   {
      const BFreeChunkHeader* pFront = freeChunks.front();
      //const BFreeChunkHeader* pBack = freeChunks.back();
      CHECK(pFront->isPrevInUse());
   }
   
#ifdef BFHEAP_TRACK_ALLOCATED_BLOCKS
//static uint q;
//q++;
//if (q == 256)
//{
//q=0;

   for (BAllocatedBlackHashMap::const_iterator it = mAllocatedBlocks.begin(); it != mAllocatedBlocks.end(); ++it)
   {
      const uint* pSize = static_cast<const uint*>(it->first) - 1;
      uint chunkSize = it->second;
      
      DWORD sizeBits = *pSize;
                  
      CHECK((sizeBits & cSizeMask) == chunkSize);
      CHECK(sizeBits & cInUseFlag);
      
#ifdef BFHEAP_GUARD_TAGS
      CHECK(pSize[1] == BFHEAP_FIRST_GUARD_TAG);
      CHECK( *(DWORD*)(((uchar*)pSize) + chunkSize - sizeof(DWORD)) == BFHEAP_LAST_GUARD_TAG );
#endif

#ifdef BFHEAP_BITMAP_CHECKS
      const uint firstAreaIndex = ((uint)pSize - mLowestRegionPtr) >> 3;
      const uint lastAreaIndex = ((uint)pSize - mLowestRegionPtr + chunkSize) >> 3;
      for (uint i = firstAreaIndex; i < lastAreaIndex; i++)
      {
         CHECK(!isAreaFree.isBitSet(i));
         CHECK(!isAreaAllocated.isBitSet(i));
         isAreaAllocated.setBit(i);
      }
#endif      
   }
//}   
#endif  
   
   return true;
}

#undef CHECK

//============================================================================
// BBFHeapAllocator::getBinIndex
//============================================================================
uint BBFHeapAllocator::getBinIndex(uint size) const
{
   BDEBUG_ASSERT(size >= cMinChunkSize);

   size -= cMinChunkSize;
   size >>= cSmallChunkShift;
   
   if (size < 32)
      return size;

   size >>= 1; if (size < 8) return 32 + size;
   size >>= 1; if (size < 8) return 32 + 8*1 + size;
   size >>= 1; if (size < 8) return 32 + 8*2 + size;
   size >>= 1; if (size < 8) return 32 + 8*3 + size;
   
   size >>= 1; if (size < 8) return 32 + 8*4 + size;
   size >>= 1; if (size < 8) return 32 + 8*5 + size;
   size >>= 1; if (size < 8) return 32 + 8*6 + size;
   size >>= 1; if (size < 8) return 32 + 8*7 + size;
         
   return cNumBins - 1;
}
