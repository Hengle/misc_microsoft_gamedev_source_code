//============================================================================
//
// File: physicalAllocator.cpp
//  
// Copyright (c) 2006, Ensemble Studios
//
// This allocator is designed for small numbers of very large blocks.
// It's not very scalable on heaps with many allocated blocks.
//
//============================================================================
#include "xrender.h"
#include "physicalAllocator.h"

//============================================================================
// BPhysicalAllocator::createBlock
//============================================================================
BPhysicalAllocator::BBlock* BPhysicalAllocator::createBlock(uint blockSize, BBlock* pPrevBlock)
{
   BDEBUG_ASSERT(blockSize > 0);
   
   BBlock* pBlock = HEAP_NEW(BBlock, gRenderHeap);
   if (!pBlock)
   {
      trace("BPhysicalAllocator::init: Unable to allocate block");
      return NULL;
   }
   
   BNode* pNode = (BNode*)mNodeAllocator.lock();
   if (!pNode)
   {
      trace("BPhysicalAllocator::init: mNodeAllocator.lock() failed");
      HEAP_DELETE(pBlock, gRenderHeap);
      return NULL;
   }
   
   blockSize = Utils::AlignUpValue(blockSize, 4096);
         
   void* p;
   if (mpHeap == cXPhysicalAllocDummyHeapPtr)
      p = XPhysicalAlloc(blockSize, MAXULONG_PTR, 0, mProtect);
   else if (mpHeap == cVirtualAllocDummyHeapPtr)
   {
      blockSize = Utils::AlignUpValue(blockSize, 65536);
      p = VirtualAlloc(NULL, blockSize, MEM_COMMIT | MEM_NOZERO | MEM_LARGE_PAGES, mProtect);
   }
   else
   {
      int actualSize;
      p = mpHeap->New(blockSize, &actualSize);
      blockSize = actualSize;
   }
      
   if (!p)
   {
      trace("BPhysicalAllocator::init: Allocation failed");
      HEAP_DELETE(pBlock, gRenderHeap);
      mNodeAllocator.unlock(pNode);      
      return NULL;
   }
   
   pBlock->clear();
   pBlock->setPtr(p);
   pBlock->setSize(blockSize);
   pBlock->setTotalFree(blockSize);
   pBlock->mNodeHead.setPrev(pNode);
   pBlock->mNodeHead.setNext(pNode);
      
   pNode->clear();
   pNode->setPtr(p);
   pNode->setFree(true);
   pNode->setSize(blockSize);
   pNode->setPrev(&pBlock->mNodeHead);
   pNode->setNext(&pBlock->mNodeHead);
         
   if (pPrevBlock)
   {
      BDEBUG_ASSERT(pPrevBlock->getPrev());
                  
      BBlock* pNextBlock = pPrevBlock->getNext();
            
      BDEBUG_ASSERT(pNextBlock && (pNextBlock->getPrev() == pPrevBlock));
      
      pPrevBlock->setNext(pBlock);
      pNextBlock->setPrev(pBlock);
      
      pBlock->setPrev(pPrevBlock);
      pBlock->setNext(pNextBlock);
   }
   
   return pBlock;
}

//============================================================================
// BPhysicalAllocator::freeBlock
//============================================================================
void BPhysicalAllocator::freeBlock(BBlock* pBlock)
{
   BDEBUG_ASSERT(pBlock && pBlock->getPtr() && pBlock->getSize());
   
   if (mpHeap == cXPhysicalAllocDummyHeapPtr)
      XPhysicalFree(pBlock->getPtr());
   else if (mpHeap == cVirtualAllocDummyHeapPtr)
   {
      BOOL success = VirtualFree(pBlock->getPtr(), 0, MEM_RELEASE);
      BVERIFY(success);
   }
   else
   {
      bool success = mpHeap->Delete(pBlock->getPtr());
      BVERIFY(success);
   }
   
   uint totalNodeSize = 0;
   
   BNode* pCurNode = pBlock->mNodeHead.getNext();  
   while (pCurNode != &pBlock->mNodeHead)
   {
      BNode* pNextNode = pCurNode->getNext();
      BDEBUG_ASSERT(pNextNode->getPrev() == pCurNode);
      
      totalNodeSize += pCurNode->getSize();
      
      mNodeAllocator.unlock(pCurNode);
      
      pCurNode = pNextNode;
   }
   
   BDEBUG_ASSERT(totalNodeSize == pBlock->mSize);
         
   BBlock* pPrevBlock = pBlock->getPrev();
   BBlock* pNextBlock = pBlock->getNext();
   BDEBUG_ASSERT(pPrevBlock->getNext() == pBlock);
   BDEBUG_ASSERT(pNextBlock->getPrev() == pBlock);
   pPrevBlock->setNext(pNextBlock);
   pNextBlock->setPrev(pPrevBlock);
   
   pBlock->clear();
   
   HEAP_DELETE(pBlock, gRenderHeap);
}

//============================================================================
// BPhysicalAllocator::initBlockHead
//============================================================================
void BPhysicalAllocator::initBlockHead(void)
{
   mBlockHead.clear();
   mBlockHead.setNext(&mBlockHead);
   mBlockHead.setPrev(&mBlockHead);
}

//============================================================================
// BPhysicalAllocator::BPhysicalAllocator
//============================================================================
BPhysicalAllocator::BPhysicalAllocator() :
   mpHeap(NULL),
   mInitialBlockSize(0),
   mMinBlockGrowSize(0),
   mProtect(0),
   mInitialized(false),
   mTotalAllocationCount(0),
   mTotalAllocationBytes(0),
   mMaxAllocationCount(0),
   mMaxAllocationBytes(0)
{
   initBlockHead();
}

//============================================================================
// BPhysicalAllocator::~BPhysicalAllocator
//============================================================================
BPhysicalAllocator::~BPhysicalAllocator()
{
   kill();
}

//============================================================================
// BPhysicalAllocator::init
//============================================================================
bool BPhysicalAllocator::init(uint initialBlockSize, uint minBlockGrowSize, DWORD protect, BMemoryHeap* pHeap)
{
   BDEBUG_ASSERT(initialBlockSize || minBlockGrowSize);
   
   kill();
   
   mpHeap = pHeap;
         
   mInitialBlockSize = initialBlockSize;
   mMinBlockGrowSize = minBlockGrowSize;
   mProtect = protect;

   const uint cInitialAllocFixedNodes = 256;   
   const uint cInitialAllocFixedGrowSize = 256;   
   mNodeAllocator.init(sizeof(BNode), cInitialAllocFixedNodes, cInitialAllocFixedGrowSize, false);

   initBlockHead();

   if (initialBlockSize)
   {
//-- FIXING PREFIX BUG ID 7110
      const BBlock* p = createBlock(initialBlockSize, &mBlockHead);
//--
      if (!p)
      {
         trace("BPhysicalAllocator::init: Can't create initial block");
         return false;
      }
   }
   
   mInitialized = true;
   
   return true;
}

//============================================================================
// BPhysicalAllocator::kill
//============================================================================
void BPhysicalAllocator::kill(void)
{
   BBlock* pCurBlock = mBlockHead.getNext();

   while (pCurBlock != &mBlockHead)
   {
      BBlock* pNextBlock = pCurBlock->getNext();
      BDEBUG_ASSERT(pNextBlock && (pNextBlock->getPrev() == pCurBlock));

      if (mpHeap == cXPhysicalAllocDummyHeapPtr)
         XPhysicalFree(pCurBlock->mPtr);
      else if (mpHeap == cVirtualAllocDummyHeapPtr)
      {
         BOOL success = VirtualFree(pCurBlock->getPtr(), 0, MEM_RELEASE);
         BVERIFY(success);
      }
      else
      {
         bool success = mpHeap->Delete(pCurBlock->mPtr);
         BVERIFY(success);
      }
      
      HEAP_DELETE(pCurBlock, gRenderHeap);

      pCurBlock = pNextBlock;
   }
   
   mNodeAllocator.kill(true);
   
   initBlockHead();
   
   resetStats();
   
   mInitialized = false;
}

//============================================================================
// BPhysicalAllocator::releaseUnusedBlocks
//============================================================================
bool BPhysicalAllocator::releaseUnusedBlocks(void)
{
   if (!mInitialized)
      return false;

   BBlock* pCurBlock = mBlockHead.getNext();

   while (pCurBlock != &mBlockHead)
   {
      BBlock* pNextBlock = pCurBlock->getNext();
      BDEBUG_ASSERT(pNextBlock && (pNextBlock->getPrev() == pCurBlock));

//-- FIXING PREFIX BUG ID 7111
      const BNode* pFirstNode = pCurBlock->mNodeHead.getNext();
//--
      BDEBUG_ASSERT(pFirstNode != &pCurBlock->mNodeHead);
      
      if ((pFirstNode->getFree()) && (pFirstNode->getSize() == pCurBlock->getSize()))
      {
         BDEBUG_ASSERT(pCurBlock->getTotalFree() == pFirstNode->getSize());
         
         freeBlock(pCurBlock);
      }

      pCurBlock = pNextBlock;
   }      
        
   return true;
}

//============================================================================
// BPhysicalAllocator::updateStats
//============================================================================
void BPhysicalAllocator::updateStats(int allocationCountDelta, int allocationSizeDelta)
{
   mTotalAllocationCount += allocationCountDelta;
   mTotalAllocationBytes += allocationSizeDelta;
   BDEBUG_ASSERT((mTotalAllocationCount >= 0) && (mTotalAllocationBytes >= 0));
   mMaxAllocationCount = Math::Max(mMaxAllocationCount, mTotalAllocationCount);
   mMaxAllocationBytes = Math::Max(mMaxAllocationBytes, mTotalAllocationBytes);
}

//============================================================================
// BPhysicalAllocator::computeTotalBytesNeeded
//============================================================================
uint BPhysicalAllocator::computeTotalBytesNeeded(BNode* pNode, uint size, uint alignment, DWORD flags)
{
   uint totalBytesNeeded = 0;
   
   if (flags & cTopToBottom)
   {
      if (size >= pNode->getSize())
         return UINT_MAX;
         
      uchar* pEnd = (uchar*)pNode->getPtr() + pNode->getSize();

      uchar* p = pEnd;

      p -= size;

      p = (uchar*)((uint)p & (~(alignment - 1)));
      
      if (p < pNode->getPtr())
         return UINT_MAX;

      totalBytesNeeded = pEnd - p;
   }
   else
   {
      totalBytesNeeded = Utils::BytesToAlignUp(pNode->getPtr(), alignment) + size;
   }
   
   return totalBytesNeeded;
}

//============================================================================
// BPhysicalAllocator::alloc
//============================================================================
void* BPhysicalAllocator::alloc(uint size, uint alignment, uint flags)
{
   BDEBUG_ASSERT((alignment >= 32) && (alignment <= 4096) && Math::IsPow2(alignment));
   
   if (!size)
   {
      trace("BPhysicalAllocator::alloc: Can't alloc 0 bytes");
      return NULL;
   }
   
   if (!mInitialized)   
   {
      trace("BPhysicalAllocator::alloc: Not initialized");
      return NULL;
   }
   
   size = Utils::AlignUpValue(size, 32);

   BBlock* pCurBlock = (flags & cTopToBottom) ? mBlockHead.getPrev() : mBlockHead.getNext();
   
   BNode* pBestNode = NULL;
   BBlock* pBestBlock = NULL;
   uint bestNodeLeftOver = UINT_MAX;
      
   while (pCurBlock != &mBlockHead)
   {
      BBlock* pNextBlock;
      if (flags & cTopToBottom) 
      {
         pNextBlock = pCurBlock->getPrev();
         BDEBUG_ASSERT(pNextBlock && (pNextBlock->getNext() == pCurBlock));
      }
      else
      {
         pNextBlock = pCurBlock->getNext();
         BDEBUG_ASSERT(pNextBlock && (pNextBlock->getPrev() == pCurBlock));
      }
      
      if (pCurBlock->getTotalFree() < size)
      {
         pCurBlock = pNextBlock;
         continue;
      }

//-- FIXING PREFIX BUG ID 7112
      const BNode* pNodeHead = &pCurBlock->mNodeHead;
//--
      BNode* pCurNode = (flags & cTopToBottom) ? pCurBlock->mNodeHead.getPrev() : pCurBlock->mNodeHead.getNext();  
      
      while (pCurNode != pNodeHead)
      {
         BNode* pNextNode;
         if (flags & cTopToBottom)
         {
            pNextNode = pCurNode->getPrev();
            BDEBUG_ASSERT(pNextNode->getNext() == pCurNode);
         }
         else
         {
            pNextNode = pCurNode->getNext();
            BDEBUG_ASSERT(pNextNode->getPrev() == pCurNode);
         }
       
         if (!pCurNode->getFree())
         {
            pCurNode = pNextNode;
            continue;
         }
         
         const uint totalBytesNeeded = computeTotalBytesNeeded(pCurNode, size, alignment, flags);
                     
         if (pCurNode->getSize() >= totalBytesNeeded)
         {
            const uint nodeLeftOver = pCurNode->getSize() - totalBytesNeeded;
            
            if (nodeLeftOver < bestNodeLeftOver)
            {
               pBestNode = pCurNode;
               pBestBlock = pCurBlock;
               bestNodeLeftOver = nodeLeftOver;  
            }
            
            if ((flags & cFirstFit) || (bestNodeLeftOver == 0))
               break;
         }
         
         pCurNode = pNextNode;
      }
      
      if (pBestNode)
      {
         if ((bestNodeLeftOver == 0) || (flags & cFirstFit))
            break;
      }            

      pCurBlock = pNextBlock;
   }   
   
   if (!pBestNode)
   {
      uint blockSize = Math::Max<uint>(mMinBlockGrowSize, size);
      
      if ((mBlockHead.getNext() == &mBlockHead) && (mInitialBlockSize))
         blockSize = Math::Max<uint>(mInitialBlockSize, size);
      else if (!mMinBlockGrowSize)
      {
         trace("BPhysicalAllocator::alloc: Alloc of %u bytes alignment %u failed", size, alignment);
         return NULL;
      }
                  
      BBlock* pNewBlock = createBlock(blockSize, mBlockHead.getPrev());
      if (!pNewBlock)
      {
         trace("BPhysicalAllocator::alloc: Alloc of %u bytes alignment %u failed", size, alignment);
         return NULL;
      }
      
      pBestNode = pNewBlock->mNodeHead.getNext();
      
      pBestBlock = pNewBlock;
   }
   
   BDEBUG_ASSERT(pBestNode->getFree());
         
   const uint totalBytesNeeded = computeTotalBytesNeeded(pBestNode, size, alignment, flags);
   BDEBUG_ASSERT(pBestNode->getSize() >= totalBytesNeeded);
         
   if (pBestNode->getSize() == totalBytesNeeded)
   {
      updateStats(1, pBestNode->getSize());
      
      pBestBlock->mTotalFree -= pBestNode->getSize();
      
      pBestNode->setFree(false);
      
      return Utils::AlignUp(pBestNode->getPtr(), alignment);
   }
   
   BNode* pNewNode = (BNode*)mNodeAllocator.lock();
   if (!pNewNode)
   {
      trace("BPhysicalAllocator::alloc: mNodeAllocator.lock() failed");
      return NULL;
   }
         
   pNewNode->clear();
   
   const uint numBytesLeft = pBestNode->getSize() - totalBytesNeeded;
      
   BNode* pPrevNode = pBestNode->getPrev();
   BNode* pNextNode = pBestNode->getNext();
   
   pBestNode->setFree(false); 
   
   BDEBUG_ASSERT(pBestBlock->mTotalFree >= totalBytesNeeded);
   pBestBlock->mTotalFree -= totalBytesNeeded;
                              
   void* pPtr;
         
   if (flags & cTopToBottom)
   {
      pBestNode->setPrev(pNewNode);
      pPrevNode->setNext(pNewNode);
                              
      pNewNode->setPrev(pPrevNode);
      pNewNode->setNext(pBestNode);
      pNewNode->setPtr(pBestNode->getPtr());
      pNewNode->setFree(true);
      pNewNode->setSize(numBytesLeft);
   
      pPtr = (uchar*)pBestNode->getPtr() + pBestNode->getSize() - totalBytesNeeded;
      
      pBestNode->setPtr(pPtr);
      pBestNode->setSize(totalBytesNeeded);
      
      BDEBUG_ASSERT(!pNewNode->getPrev()->getFree());
      BDEBUG_ASSERT(!pNewNode->getNext()->getFree());
   }
   else
   {
      pBestNode->setSize(totalBytesNeeded);
      
      pBestNode->setNext(pNewNode);
      pNextNode->setPrev(pNewNode);
      
      pNewNode->setPrev(pBestNode);
      pNewNode->setNext(pNextNode);
      pNewNode->setPtr((uchar*)pBestNode->getPtr() + totalBytesNeeded);
      pNewNode->setFree(true);
      pNewNode->setSize(numBytesLeft);
            
      BDEBUG_ASSERT(!pNewNode->getPrev()->getFree());
      BDEBUG_ASSERT(!pNewNode->getNext()->getFree());
                              
      pPtr = Utils::AlignUp(pBestNode->getPtr(), alignment);
      
      uint bytesToAlignUp = (uchar*)pPtr - (uchar*)pBestNode->getPtr();
      if (bytesToAlignUp)
      {
         BNode* pNewNode2 = (BNode*)mNodeAllocator.lock();
         if (!pNewNode2)
         {
            updateStats(1, pBestNode->getSize());
            
            trace("BPhysicalAllocator::alloc: mNodeAllocator.lock() failed");
            return pPtr;
         }
         
         pNewNode2->clear();
         pNewNode2->setPtr(pBestNode->getPtr());
         pNewNode2->setFree(true);
         pNewNode2->setSize(bytesToAlignUp);
         pNewNode2->setPrev(pBestNode->getPrev());
         pNewNode2->setNext(pBestNode);
         pBestNode->getPrev()->setNext(pNewNode2);
         pBestNode->setPrev(pNewNode2);
                  
         BDEBUG_ASSERT((totalBytesNeeded - size) == bytesToAlignUp);
         
         pBestNode->setPtr(pPtr);
         pBestNode->setSize(size);
         
         pBestBlock->mTotalFree += bytesToAlignUp;
         
         coalesceFreeNode(pBestBlock, pNewNode2);
      }
   }
   
   updateStats(1, pBestNode->getSize());
   
   BDEBUG_ASSERT( ((uint)pPtr & (alignment - 1)) == 0);
      
   return pPtr;
}

//============================================================================
// BPhysicalAllocator::findBlockFromPtr
//============================================================================
BPhysicalAllocator::BBlock* BPhysicalAllocator::findBlockFromPtr(void* p)
{
   BBlock* pCurBlock = mBlockHead.getNext();

   while (pCurBlock != &mBlockHead)
   {
      BBlock* pNextBlock = pCurBlock->getNext();
      BDEBUG_ASSERT(pNextBlock && (pNextBlock->getPrev() == pCurBlock));

//-- FIXING PREFIX BUG ID 7113
      const void* pBegin = pCurBlock->getPtr();
//--
//-- FIXING PREFIX BUG ID 7114
      const void* pEnd = (uchar*)pCurBlock->getPtr() + pCurBlock->getSize();
//--
      if ((p >= pBegin) && (p < pEnd))
         return pCurBlock;

      pCurBlock = pNextBlock;
   }
   
   return NULL;
}

//============================================================================
// BPhysicalAllocator::findNodeFromPtr
//============================================================================
BPhysicalAllocator::BNode* BPhysicalAllocator::findNodeFromPtr(BBlock* pBlock, void* p)
{
//-- FIXING PREFIX BUG ID 7115
   const BNode* pHeadNode = &pBlock->mNodeHead;         
//--

   BNode* pCurNode = pBlock->mNodeHead.getNext();
   while (pCurNode != pHeadNode)
   {
      BNode* pNextNode = pCurNode->getNext();
      BDEBUG_ASSERT(pNextNode->getPrev() == pCurNode);

      if ( (p >= pCurNode->getPtr()) && (p < ((uchar*)pCurNode->getPtr() + pCurNode->getSize())) )
         return pCurNode;

      pCurNode = pNextNode;
   }
   
   return NULL;
}

//============================================================================
// BPhysicalAllocator::deleteNode
//============================================================================
void BPhysicalAllocator::deleteNode(BNode* pNode)
{
   BNode* pPrev = pNode->getPrev();
   BNode* pNext = pNode->getNext();
   
   BDEBUG_ASSERT(pPrev->getNext() == pNode);
   BDEBUG_ASSERT(pNext->getPrev() == pNode);
   
   pPrev->setNext(pNext);
   pNext->setPrev(pPrev);
   
   mNodeAllocator.unlock(pNode);
}

//============================================================================
// BPhysicalAllocator::coalesceFreeNode
//============================================================================
void BPhysicalAllocator::coalesceFreeNode(BBlock* pBlock, BNode* pNode)
{
//-- FIXING PREFIX BUG ID 7117
   const BNode* pHeadNode = &pBlock->mNodeHead;
//--
   BNode* pPrevNode = pNode->getPrev();
   BNode* pNextNode = pNode->getNext();
   
   BDEBUG_ASSERT(pNode->getFree());
   BDEBUG_ASSERT(pNode != pHeadNode);
   BDEBUG_ASSERT(pPrevNode->getNext() == pNode);
   BDEBUG_ASSERT(pNextNode->getPrev() == pNode);
   
//-- FIXING PREFIX BUG ID 7118
   const void* pPrevPtr = pPrevNode->getPtr();
//--
   void* pPtr     = pNode->getPtr();
//-- FIXING PREFIX BUG ID 7119
   const void* pNextPtr = pNextNode->getPtr();
//--
   
   pPrevPtr;
   pNextPtr;
         
   if ((pPrevNode != pHeadNode) && (pPrevNode->getFree()))
   {
      BDEBUG_ASSERT(pPrevNode < pPtr);
      
      BDEBUG_ASSERT( ((uchar*)pPrevPtr + pPrevNode->getSize()) == pPtr );
      
      pPtr = pPrevNode->getPtr();
      pNode->setPtr(pPtr);
        
      pNode->setSize(pNode->getSize() + pPrevNode->getSize());
      
      deleteNode(pPrevNode);
      pPrevNode = NULL;
   }
   
   if ((pNextNode != pHeadNode) && (pNextNode->getFree()))
   {
      BDEBUG_ASSERT(pNextPtr > pPtr);
      
      BDEBUG_ASSERT( ((uchar*)pPtr + pNode->getSize()) == pNextPtr );
      
      pNode->setSize(pNode->getSize() + pNextNode->getSize());
      
      deleteNode(pNextNode);
   }
}

//============================================================================
// BPhysicalAllocator::free
//============================================================================
bool BPhysicalAllocator::free(void* p)
{
   if (!p)
      return false;
      
   if (!mInitialized)   
   {
      trace("BPhysicalAllocator::free: Not initialized");
      return false;
   }
               
   BBlock* pBlock = findBlockFromPtr(p);
   if (!pBlock)
   {
      trace("BPhysicalAllocator::free: Bad pointer %p", p);
      return false;
   }
   
   BNode* pNode = findNodeFromPtr(pBlock, p);
   if (!pNode)
   {
      trace("BPhysicalAllocator::free: Bad pointer %p", p);
      return false;
   }
   
   if (pNode->getFree())
   {
      trace("BPhysicalAllocator::free: Alloc %p not allocated", p);
      return false;
   }
         
   pNode->setFree(true);
   
   pBlock->mTotalFree += pNode->getSize();
   
   updateStats(-1, -(int)pNode->getSize());
   
   coalesceFreeNode(pBlock, pNode);
         
   return true;
}

//============================================================================
// BPhysicalAllocator::getSize
//============================================================================
bool BPhysicalAllocator::getSize(void* p, uint& size)
{
   size = 0;
   
   if (!p)
      return false;

   if (!mInitialized)   
   {
      trace("BPhysicalAllocator::free: Not initialized");
      return false;
   }

   BBlock* pBlock = findBlockFromPtr(p);
   if (!pBlock)
   {
      trace("BPhysicalAllocator::free: Bad pointer %p", p);
      return false;
   }

   BNode* pNode = findNodeFromPtr(pBlock, p);
   if (!pNode)
   {
      trace("BPhysicalAllocator::free: Bad pointer %p", p);
      return false;
   }

   if (pNode->getFree())
   {
      trace("BPhysicalAllocator::free: Alloc %p not allocated", p);
      return false;
   }

   size = ((uchar*)pNode->getPtr() + pNode->getSize()) - (uchar*)p;
   
   return true;
}

//============================================================================
// BPhysicalAllocator::check
//============================================================================
bool BPhysicalAllocator::check(BHeapStats& stats)
{
#define CHECK(x) if (!(x)) return false;

   stats.clear();
   
   if (!mInitialized)   
      return false;
      
   BBlock* pCurBlock = mBlockHead.getNext();
           
   while (pCurBlock != &mBlockHead)
   {
      stats.totalBlocks++;
            
      CHECK((pCurBlock->getPtr() != NULL) && (pCurBlock->getSize() != 0));
      
      stats.largestBlock = Math::Max(stats.largestBlock, pCurBlock->getSize());
                  
      BBlock* pNextBlock = pCurBlock->getNext();
      CHECK(pNextBlock && (pNextBlock->getPrev() == pCurBlock));
                  
      BNode* pNodeHead = &pCurBlock->mNodeHead;
      BNode* pCurNode = pNodeHead->getNext();  
      
      CHECK(pNodeHead->getPtr() == NULL);
      CHECK(pNodeHead->getSize() == 0);

      uint totalBlockBytesFound = 0;
      uint totalFreeBlockBytesFound = 0;
      
//-- FIXING PREFIX BUG ID 7120
      const void* pPrevPtr = NULL;
//--
      
      CHECK(pCurNode != pNodeHead);
      CHECK(pCurNode->getPtr() == pCurBlock->getPtr());
      
      while (pCurNode != pNodeHead)
      {
         stats.totalNodes++;
         
         CHECK((pCurNode->getPtr() != NULL) && (pCurNode->getSize() != 0));
         
         BNode* pNextNode = pCurNode->getNext();
         CHECK(pNextNode->getPrev() == pCurNode);
         
         CHECK(pCurNode->getPtr());
         CHECK(pCurNode->getPtr() >= pPrevPtr);
                           
         if (pNextNode != pNodeHead)
         {
            CHECK( ((uchar*)pCurNode->getPtr() + pCurNode->getSize()) == pNextNode->getPtr() );
         }
         
         totalBlockBytesFound += pCurNode->getSize();
                  
         if (pCurNode->getFree())
         {
            if (pNextNode != pNodeHead)
            {
               CHECK(!pNextNode->getFree());
            }
            
            totalFreeBlockBytesFound += pCurNode->getSize();
         
            stats.totalFreeNodes++;
            
            stats.totalBytesFree += pCurNode->getSize();
            
            stats.largestFreeNode = Math::Max(stats.largestFreeNode, pCurNode->getSize());
         }
         else
         {
            stats.totalBytesAllocated += pCurNode->getSize();
            
            stats.largestAllocatedNode = Math::Max(stats.largestAllocatedNode, pCurNode->getSize());
         }
                           
         pPrevPtr = pCurNode->getPtr();
         
         pCurNode = pNextNode;
      }
      
      CHECK(totalBlockBytesFound == pCurBlock->getSize());
      CHECK(totalFreeBlockBytesFound == pCurBlock->getTotalFree());

      pCurBlock = pNextBlock;
   }  
   
   CHECK((uint)mNodeAllocator.getCurrentLocks() == stats.totalNodes);

#if 0   
   trace("Blocks: %u, Nodes: %u, Free Nodes: %u, Bytes: %u, Allocated Bytes: %u, Free Bytes: %u", 
      stats.totalBlocks,
      stats.totalNodes,
      stats.totalFreeNodes,
      stats.totalBytesAllocated + stats.totalBytesFree,
      stats.totalBytesAllocated,
      stats.totalBytesFree); 
   
   trace("Largest Block: %u, Largest Allocated Node: %u, Largest Free Node: %u",
      stats.largestBlock,
      stats.largestAllocatedNode,
      stats.largestFreeNode);
#endif
      
   return true;      

#undef CHECK   
}

//============================================================================
// BPhysicalAllocator::resetStats
//============================================================================
void BPhysicalAllocator::resetStats(void)
{
   mTotalAllocationCount = 0;
   mTotalAllocationBytes = 0;
   mMaxAllocationCount = 0;
   mMaxAllocationBytes = 0;
}

#ifndef BUILD_FINAL
#include "math\random.h"
//============================================================================
// BPhysicalAllocator::test
//============================================================================
void BPhysicalAllocator::test(void)
{
   BPhysicalAllocator physAlloc;

   physAlloc.init(1*1024*1024, 1*1024*1024);
   bool success;

   Random rand;

   BDynamicArray<void*> allocs;

   uint totalAllocated = 0;
   
   uint opCount = 0;

   for ( ; ; )
   {
      opCount++;
      
      if (rand.fRand() >= .9999f)
      {
         for (uint i = 0; i < allocs.getSize(); i++)
         {
            success = physAlloc.free(allocs[i]);
            BASSERT(success);
         }

         allocs.resize(0);
         
         physAlloc.releaseUnusedBlocks();
         
         totalAllocated = 0;
      }
      else if ( ((rand.fRand() >= .95f) && (allocs.getSize())) || ((allocs.getSize()) && (totalAllocated >= 128*1024*1024)) )
      {
         const uint allocIndex = rand.iRand(0, allocs.getSize());

         uint allocSize = 0;
         success = physAlloc.getSize(allocs[allocIndex], allocSize);
         BASSERT(success);
         totalAllocated -= allocSize;

         success = physAlloc.free(allocs[allocIndex]);
         BASSERT(success);

         allocs.removeIndex(allocIndex);
         
         physAlloc.releaseUnusedBlocks();
      }
      else
      {
         uint size = rand.iRand(32, 65536);
         uint align = 1 << rand.iRand(5, 12);

         uint flags = 0;

         if (rand.uRand() & 1) flags |= BPhysicalAllocator::cTopToBottom;
         if (rand.uRand() & 2) flags |= BPhysicalAllocator::cFirstFit;
         
         //flags |= BPhysicalAllocator::cTopToBottom;
         //flags |= BPhysicalAllocator::cFirstFit;
                  
         void* p = physAlloc.alloc(size, align, flags);
         BASSERT(p);

         allocs.pushBack(p);

         totalAllocated += size;
      }
            
      if ((rand.uRand() & 1023) < 100)
      {
         trace("opCount: %u", opCount);
         BPhysicalAllocator::BHeapStats stats;
         success = physAlloc.check(stats);
         BASSERT(success);
      }
   }

#if 0   
   success = physAlloc.check();
   BASSERT(success);

   void* p0 = physAlloc.alloc(32);
   BASSERT(p0);

   void* p1 = physAlloc.alloc(256, 4096, BPhysicalAllocator::cTopToBottom);
   BASSERT(p1);

   void* p2 = physAlloc.alloc(512, 4096, BPhysicalAllocator::cTopToBottom);
   BASSERT(p2);

   success = physAlloc.check();
   BASSERT(success);

   success = physAlloc.free(p0);
   BASSERT(success);

   success = physAlloc.check();
   BASSERT(success);

   success = physAlloc.free(p1);
   BASSERT(success);

   success = physAlloc.check();
   BASSERT(success);

   success = physAlloc.free(p2);
   BASSERT(success);
#endif   

   BPhysicalAllocator::BHeapStats stats;
   success = physAlloc.check(stats);
   BASSERT(success);

   physAlloc.kill();
}
#endif