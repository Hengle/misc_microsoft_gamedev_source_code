//============================================================================
//
//  linkedList.h
//  
//  Copyright (c) 1999-2007, Ensemble Studios
//
//============================================================================


//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
const uint LLIST_QSORT_CUTOFF = 32;

//============================================================================
//  PRIVATE MACROS
//============================================================================
#define DEBUG_FULL_ITERATOR_VALIDATION 0
#define BLLIST_VALIDATE_ITERATOR(iterator) BDEBUG_ASSERT(isValidIterator(iterator))

//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
template <class T, class Allocator> BLinkedList<T, Allocator>::BLinkedList(uint initialSize, uint growSize, Allocator& alloc) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL),
   mAllocator(alloc),
   mUseBlockAllocator(true)
{
#ifdef BUILD_DEBUG
   mNumValidObjects = 0;
#endif   
   reset(initialSize, growSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> BLinkedList<T, Allocator>::BLinkedList(const BLinkedList<T, Allocator>& list) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL),
   mUseBlockAllocator(true)
{
#ifdef BUILD_DEBUG
   mNumValidObjects = 0;
#endif   

   *this = list;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<class T, class Allocator> BLinkedList<T, Allocator>::BLinkedList(Allocator& alloc) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL),
   mAllocator(alloc),
   mUseBlockAllocator(true)
{
#ifdef BUILD_DEBUG
   mNumValidObjects = 0;
#endif   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> BLinkedList<T, Allocator>::~BLinkedList()
{
   reset(0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::destructAllNodes(void)
{
   if (BIsBuiltInType<T>::Flag)
   {
      updateValidObjects(-(int)mSize);
   }
   else
   {
      BNode* pNode = mpUsedHead;
      while (pNode)
      {
         BNode* pKill = pNode;
         pNode = pNode->mpNext;

         pKill->destruct();
         
         updateValidObjects(-1);
      }
   }      

#ifdef BUILD_DEBUG   
   BDEBUG_ASSERT(mNumValidObjects == 0);
#endif   
}

//============================================================================
//  LIST CONTROL
//============================================================================
//----------------------------------------------------------------------------
//  reset()
//  Releases all memory and re-initializes the list.  initialSize specifies
//  the number of nodes to pre-allocate.  growSize specifies how many nodes
//  to allocate each time the list needs to grow.  If growSize equals
//  cGrowExponential the amount the list grows each time will double.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::reset(uint initialSize, uint growSize)
{
   destructAllNodes();
         
   //-- Release the old memory.
   BBlock* pBlock = mpBlocks;
   while (pBlock)
   {
      BBlock* pKill = pBlock;
      pBlock = pBlock->mpNextBlock;
      mAllocator.dealloc(pKill->mpNodes);
      ALLOCATOR_DELETE(pKill, mAllocator);
   }

   //-- Reset the list pointers.
   mpBlocks   = NULL;
   mpFreeHead = NULL;
   mpUsedHead = NULL;
   mpUsedTail = NULL;

   //-- Set the new sizes.
   mSize            = 0;
   mGrowSize        = growSize;
   mAllocatedSize   = 0;
   mGrowExponential = (mGrowSize == cGrowExponential);
   
   if (mGrowExponential)
      mGrowSize = (initialSize == 0) ? 1 : initialSize;
      
   mUseBlockAllocator = true;      

   if ((!mGrowExponential) && (initialSize <= 1) && (mGrowSize == 1))
      mUseBlockAllocator = false;
      
   //-- Allocate the initial block.
   if ((mUseBlockAllocator) && (initialSize))
      addBlock(initialSize);
}

//----------------------------------------------------------------------------
//  empty()
//  Releases all the used nodes back to the free list.  
// This does not change the current grow size.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::empty()
{
   //-- Give all the nodes back to the free list.
   BNode* pNode = mpUsedTail;
   while (pNode)
   {
      BNode* pKill = pNode;
      pNode = pNode->mpPrev;

      //-- Add the node to the free head.
      addToFreeHead(pKill, true);
   }

#ifdef BUILD_DEBUG
   BDEBUG_ASSERT(mNumValidObjects == 0);
#endif   

   //-- Reset the list data.
   mpUsedHead = NULL;
   mpUsedTail = NULL;
   mSize      = 0;
}

//----------------------------------------------------------------------------
//  optimize()
//  Condenses all the nodes into a single optimally sized contigous block.
//  This does not change the current grow size.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::optimize()
{
   if (mSize <= 1)
      return;
      
   //-- Allocate the block.
   BBlock* pNewBlock = ALLOCATOR_NEW(BBlock, mAllocator);
   pNewBlock->mNumNodes   = mSize;
   pNewBlock->mpNextBlock = NULL;
   pNewBlock->mpNodes     = static_cast<BNode*>(mAllocator.alloc(sizeof(BNode) * mSize));
   if (!pNewBlock->mpNodes)
   {
      BFATAL_FAIL("BLinkedList::optimize: alloc failed");
   }

   //-- Fill the list.
   uint  index = 0;
   BNode* pNode = mpUsedHead;
   while (pNode)
   {
      pNewBlock->mpNodes[index].construct(pNode->mItem);
      pNode->destruct();
      
      pNewBlock->mpNodes[index].mpPrev = (index == 0)         ? NULL : &pNewBlock->mpNodes[index - 1];
      pNewBlock->mpNodes[index].mpNext = (index == mSize - 1) ? NULL : &pNewBlock->mpNodes[index + 1];
      index++;

      pNode = pNode->mpNext;
   }

   //-- Release the old memory.
   if (!mUseBlockAllocator)
   {
      BDEBUG_ASSERT(!mpBlocks);
      
      BNode* pNode = mpUsedTail;
      while (pNode)
      {
         BNode* pKill = pNode;
         pNode = pNode->mpPrev;
         
         mAllocator.dealloc(pKill);
                  
         BDEBUG_ASSERT(mAllocatedSize > 0);
         mAllocatedSize--;
      }
      
      BDEBUG_ASSERT(mAllocatedSize == 0);
   }
   else
   {
      BBlock* pBlock = mpBlocks;
      while (pBlock)
      {
         BBlock* pKill = pBlock;
         pBlock = pBlock->mpNextBlock;
         
         BDEBUG_ASSERT(mAllocatedSize >= pKill->mNumNodes );
         mAllocatedSize -= pKill->mNumNodes;
         
         mAllocator.dealloc(pKill->mpNodes);
         ALLOCATOR_DELETE(pKill, mAllocator);
      }
      
      BDEBUG_ASSERT(mAllocatedSize == 0);
   }      

   //-- Reset the list pointers.
   mpBlocks   = pNewBlock;
   mpFreeHead = NULL;
   mpUsedHead = &pNewBlock->mpNodes[0];
   mpUsedTail = &pNewBlock->mpNodes[mSize - 1];
   
   mUseBlockAllocator = true;
   mGrowSize = 1;
   mGrowExponential = true;
      
   //-- Set the new size.
   mAllocatedSize = mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> bool BLinkedList<T, Allocator>::validate(void) const
{
   if (!mpUsedTail)
   {
      if (mSize)
         return false;
         
      if (mpUsedHead)
         return false;
      
      if (!mUseBlockAllocator)
      {
         if (mpFreeHead)
            return false;
      }
      else
      {
         if (mpBlocks)
         {
            if (!mAllocatedSize)
               return false;
         }
      }
      
      if (mNumValidObjects)
         return false;
      
      return true;
   }

   if (!mpUsedHead)      
      return false;

#ifdef BUILD_DEBUG      
   if (mNumValidObjects != (int)mSize)
      return false;
#endif

   BDynamicArray<BPointerRange> blocks;
   if (mUseBlockAllocator)
   {
      uint totalNodes = 0;
      
      BBlock* pBlock = mpBlocks;
      while (pBlock)
      {
         BDEBUG_ASSERT(!IsBadWritePtr(pBlock, sizeof(BBlock)));
         if ((pBlock->mNumNodes == 0) || (!pBlock->mpNodes))
            return false;
         totalNodes += pBlock->mNumNodes;
            
         blocks.pushBack(BPointerRange(pBlock->mpNodes, pBlock->mpNodes + pBlock->mNumNodes - 1));
         pBlock = pBlock->mpNextBlock;
         if (pBlock == mpBlocks)
            return false;
      }            
      
      BDEBUG_ASSERT(mAllocatedSize == totalNodes);
      
      blocks.sort();
   }      
   else
   {
      BDEBUG_ASSERT(mAllocatedSize == mSize);
   }
   
   BDynamicArray<BNode*> freePointers;
   BNode* pNode = mpFreeHead;
   while (pNode)
   {
      BDEBUG_ASSERT(!IsBadWritePtr(pNode, sizeof(BNode)));
      
      if (mUseBlockAllocator)
      {
         if (cInvalidIndex == blocks.binarySearch(BPointerRange(pNode, pNode)))
            return false;
      }            
                  
      freePointers.pushBack(pNode);
      pNode = pNode->mpNext;
      if ((pNode == mpFreeHead) || (pNode == mpUsedHead) || (pNode == mpUsedTail))
         return false;
   }
   freePointers.sort();
   
   pNode = mpUsedHead;
   uint numFound = 0;
   while (pNode)
   {
      BDEBUG_ASSERT(!IsBadWritePtr(pNode, sizeof(BNode)));
      
      if (freePointers.binarySearch(pNode) != cInvalidIndex)
         return false;
         
      BNode* pPrev = pNode->mpPrev;
      BNode* pNext = pNode->mpNext;
      
      if ((!numFound) && (pPrev != NULL))
         return false;
         
      if ((numFound == (mSize - 1)) && (pNext != NULL))
         return false;
         
      if (pPrev)
      {
         if (freePointers.binarySearch(pPrev) != cInvalidIndex)
            return false;
            
         if (pPrev->mpNext != pNode)
            return false;
      }
      
      if (pNext)
      {
         if (freePointers.binarySearch(pNext) != cInvalidIndex)
            return false;
            
         if (pNext->mpPrev != pNode)
            return false;
      }
      
      numFound++;
      if (numFound > mSize)
         return false;

      if (mUseBlockAllocator)
      {
         if (cInvalidIndex == blocks.binarySearch(BPointerRange(pNode, pNode)))
            return false;  
      }            
         
      if (mSize == numFound)
      {
         if (pNode != mpUsedTail)
            return false;
      }
         
      pNode = pNode->mpNext;
      if (pNode == mpUsedHead)
         return false;
   }
   
   if (numFound != mSize)
      return false;

   return true;
}


//============================================================================
//  LIST INFO
//============================================================================
template <class T, class Allocator> bool BLinkedList<T, Allocator>::isEmpty() const
{
   return (mSize == 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> bool BLinkedList<T, Allocator>::isGrowExponential() const
{
   return mGrowExponential;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> bool BLinkedList<T, Allocator>::isUsingBlockAllocator() const
{
   return mUseBlockAllocator;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> uint BLinkedList<T, Allocator>::getSize() const
{
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> uint BLinkedList<T, Allocator>::getGrowSize() const
{
   return mGrowSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> uint BLinkedList<T, Allocator>::getAllocatedSize() const
{
   return mAllocatedSize;
}


//============================================================================
//  ADDING ITEMS
//============================================================================
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::addToHead(const T& item)
{
   //-- Get a node off the free list.
   BNode* pNode = getFreeNode();
   pNode->construct(item);
   updateValidObjects(1);

   //-- Add it.
   linkToHead(pNode);

   //-- Increase size.
   mSize++;

   //-- Give it back.
   return iterator(this, &pNode->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::addToTail(const T& item)
{
   //-- Get a node off the free list.
   BNode* pNode = getFreeNode();
   pNode->construct(item);
   updateValidObjects(1);

   //-- Add it.
   linkToTail(pNode);

   //-- Increase.
   mSize++;

   //-- Give it back.
   return iterator(this, &pNode->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::addBefore(const T& item, iterator hItem)
{
   //-- Get the node.
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem.getNode();
      
   if (isEnd(pRefNode))
      return addToTail(item);
      
   //-- Get a node off the free list.
   BNode* pNode = getFreeNode();
   pNode->construct(item);
   updateValidObjects(1);
            
   pNode->mpNext = pRefNode;
   pNode->mpPrev = pRefNode->mpPrev;
   if (pRefNode->mpPrev)
   {
      pRefNode->mpPrev->mpNext = pNode;
   }
   else
   {
      BDEBUG_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pNode;
   }
   pRefNode->mpPrev = pNode;
   mSize++;
   return iterator(this, &pNode->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::addAfter(const T& item, iterator hItem)
{
   //-- Get the node.
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem.getNode();
   
   BDEBUG_ASSERT(!isEnd(pRefNode));

   //-- Get a node off the free list.
   BNode* pNode = getFreeNode();
   pNode->construct(item);
   updateValidObjects(1);

   //-- Add it to the list.
   pNode->mpPrev = pRefNode;
   pNode->mpNext = pRefNode->mpNext;
   if (pRefNode->mpNext)
   {
      pRefNode->mpNext->mpPrev = pNode;
   }
   else
   {
      BDEBUG_ASSERT(mpUsedTail == pRefNode);
      mpUsedTail = pNode;
   }
   pRefNode->mpNext = pNode;
   mSize++;
   return iterator(this, &pNode->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::addSorted(const T& item, bool ascending)
{
   //-- Get a node off the free list.
   BNode* pNode = getFreeNode();
   pNode->construct(item);
   updateValidObjects(1);

   //-- Handle an empty list.
   if (mSize == 0)
   {
      mpUsedHead = pNode;
      mpUsedTail = pNode;
      mSize      = 1;
      return iterator(this, &pNode->mItem);
   }

   //-- Find the spot in the list to put it.
   BNode* pRefNode = mpUsedHead;
   if (ascending)
   {
      //-- Find the first node that is greater than or equal to item.
      while (pRefNode && (pRefNode->mItem < item))
         pRefNode = pRefNode->mpNext;
   }
   else
   {
      //-- Find the first node that is less than or equal to item.
      while (pRefNode && (pRefNode->mItem > item))
         pRefNode = pRefNode->mpNext;
   }

   //-- If pRefNode is NULL, we add it to the tail.
   if (pRefNode == NULL)
   {
      pNode->mpPrev      = mpUsedTail;
      mpUsedTail->mpNext = pNode;
      mpUsedTail        = pNode;
      mSize++;
      return iterator(this, &pNode->mItem);
   }

   //-- Add the new node before pRefNode.
   pNode->mpNext = pRefNode;
   pNode->mpPrev = pRefNode->mpPrev;
   if (pRefNode->mpPrev)
   {
      pRefNode->mpPrev->mpNext = pNode;
   }
   else
   {
      BDEBUG_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pNode;
   }
   pRefNode->mpPrev = pNode;

   mSize++;
   return iterator(this, &pNode->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::addSorted(const T& item, BCompareFunc* pFunc, void* pParam, bool ascending)
{
   BDEBUG_ASSERT(pFunc);

   //-- Get a node off the free list.
   BNode* pNode = getFreeNode();
   pNode->construct(item);
   updateValidObjects(1);

   //-- Handle an empty list.
   if (mSize == 0)
   {
      mpUsedHead = pNode;
      mpUsedTail = pNode;
      mSize      = 1;
      return iterator(this, &pNode->mItem);
   }

   //-- Find the spot in the list to put it.
   BNode* pRefNode = mpUsedHead;
   if (ascending)
   {
      //-- Find the first node that is greater than or equal to item.
      while (pRefNode && (pFunc(pRefNode->mItem, item, pParam) < 0))
         pRefNode = pRefNode->mpNext;
   }
   else
   {
      //-- Find the first node that is less than or equal to item.
      while (pRefNode && (pFunc(pRefNode->mItem, item, pParam) > 0))
         pRefNode = pRefNode->mpNext;
   }

   //-- If pRefNode is NULL, we add it to the tail.
   if (pRefNode == NULL)
   {
      pNode->mpPrev      = mpUsedTail;
      mpUsedTail->mpNext = pNode;
      mpUsedTail        = pNode;
      mSize++;
      return iterator(this, &pNode->mItem);
   }

   //-- Add the new node before pRefNode.
   pNode->mpNext = pRefNode;
   pNode->mpPrev = pRefNode->mpPrev;
   if (pRefNode->mpPrev)
   {
      pRefNode->mpPrev->mpNext = pNode;
   }
   else
   {
      BDEBUG_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pNode;
   }
   pRefNode->mpPrev = pNode;

   mSize++;
   return iterator(this, &pNode->mItem);
}


//============================================================================
//  REMOVING ITEMS
//============================================================================
template <class T, class Allocator> void BLinkedList<T, Allocator>::removeHead()
{
   //-- Be nice and don't BDEBUG_ASSERT.  So it can be used in a loop like:
   //-- while (p = list.removeHead()) ...
   if (mpUsedHead == NULL)
      return;

   //-- Remove it from the list.
   BNode* pOldNode = mpUsedHead;
   mpUsedHead = mpUsedHead->mpNext;
   if (mpUsedHead == NULL)
   {
      BDEBUG_ASSERT(mpUsedTail == pOldNode);
      BDEBUG_ASSERT(mSize      == 1);
      mpUsedTail = NULL;
   }
   else
   {
      mpUsedHead->mpPrev = NULL;
   }

   //-- Add it to the free head.
   addToFreeHead(pOldNode, true);
   mSize--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::removeTail()
{
   //-- Be nice and don't BDEBUG_ASSERT.  So it can be used in a loop like:
   //-- while (p = list.removeTail()) ...
   if (mpUsedTail == NULL)
      return;

   //-- Remove it from the list.
   BNode* pOldNode = mpUsedTail;
   mpUsedTail = mpUsedTail->mpPrev;
   if (mpUsedTail == NULL)
   {
      BDEBUG_ASSERT(mpUsedHead == pOldNode);
      BDEBUG_ASSERT(mSize      == 1);
      mpUsedHead = NULL;
   }
   else
   {
      mpUsedTail->mpNext = NULL;
   }

   //-- Add it to the free head.
   addToFreeHead(pOldNode, true);
   mSize--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::remove(iterator hItem)
{
   //-- Get the node.
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem.getNode();
   
   BDEBUG_ASSERT(!isEnd(pRefNode));
      
   //-- Unlink it.
   unlink(pRefNode);

   //-- Add it to the free head.
   addToFreeHead(pRefNode, true);
   mSize--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::removeAndGetPrev(iterator hItem)
{
   //-- Get the node.
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem.getNode();
   
   BDEBUG_ASSERT(!isEnd(pRefNode));
   
   //-- Remove it from the list.
   if (pRefNode->mpPrev == NULL)
   {
      BDEBUG_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pRefNode->mpNext;
   }
   else
   {
      pRefNode->mpPrev->mpNext = pRefNode->mpNext;
   }
   if (pRefNode->mpNext == NULL)
   {
      BDEBUG_ASSERT(mpUsedTail == pRefNode);
      mpUsedTail = pRefNode->mpPrev;
   }
   else
   {
      pRefNode->mpNext->mpPrev = pRefNode->mpPrev;
   }

   //-- Get the prev node item and handle.
   T* pItem = getLLEndItem();
   if (pRefNode->mpPrev)
      pItem = &pRefNode->mpPrev->mItem;

   //-- Add it to the free head.
   addToFreeHead(pRefNode, true);
   mSize--;
   return iterator(this, pItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::removeAndGetNext(iterator hItem)
{
   //-- Get the node.
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem.getNode();
   
   BDEBUG_ASSERT(!isEnd(pRefNode));
   
   //-- Remove it from the list.
   if (pRefNode->mpPrev == NULL)
   {
      BDEBUG_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pRefNode->mpNext;
   }
   else
   {
      pRefNode->mpPrev->mpNext = pRefNode->mpNext;
   }
   if (pRefNode->mpNext == NULL)
   {
      BDEBUG_ASSERT(mpUsedTail == pRefNode);
      mpUsedTail = pRefNode->mpPrev;
   }
   else
   {
      pRefNode->mpNext->mpPrev = pRefNode->mpPrev;
   }

   //-- Get the next node item and handle.
   T* pItem = getLLEndItem();
   if (pRefNode->mpNext)
      pItem = &pRefNode->mpNext->mItem;

   //-- Add it to the free head.
   addToFreeHead(pRefNode, true);
   mSize--;
   return iterator(this, pItem);
}


//============================================================================
//  RELOCATION
//============================================================================
//----------------------------------------------------------------------------
//  moveToHead()
//  Moves the node referenced by hItem to the head.  All existing list
//  handles remain valid.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::moveToHead(iterator hItem)
{
   //-- Get the node.
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem->getNode();

   //-- If we are already at the head, do nothing.  This will prevent an
   //-- invalid assertion from firing in linkToHead() when there is only
   //-- one node in the list.
   if (pRefNode == mpUsedHead)
      return;

   //-- Unlink from present spot in the list.
   unlink(pRefNode);

   //-- Link it back up at the head.
   linkToHead(pRefNode);      
}

//----------------------------------------------------------------------------
//  moveToTail()
//  Moves the node referenced by hItem to the tail.  All existing list
//  handles remain valid.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::moveToTail(iterator hItem)
{
   //-- Get the node.
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem->getNode();

   //-- If we are already at the tail, do nothing.  This will prevent an
   //-- invalid assertion from firing in linkToTail() when there is only
   //-- one node in the list.
   if (pRefNode == mpUsedTail)
      return;

   //-- Unlink from present spot in the list.
   unlink(pRefNode);

   //-- Link it back up at the tail.
   linkToTail(pRefNode);      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::swap(iterator a, iterator b)
{
   BLLIST_VALIDATE_ITERATOR(a);
   BLLIST_VALIDATE_ITERATOR(b);
   BDEBUG_ASSERT(mpUsedHead);
   BDEBUG_ASSERT(mpUsedTail);
   
   BDEBUG_ASSERT(!a.isEnd() && !b.isEnd());
   
   BNode* pA = a.getNode();
   BNode* pB = b.getNode();
   
   BNode* pAPrev = pA->mpPrev; 
   BNode* pANext = pA->mpNext;
   BNode* pBPrev = pB->mpPrev;
   BNode* pBNext = pB->mpNext;

   if (pANext == pB)
   {
      // other A B other
      if (pAPrev)
         pAPrev->mpNext = pB;
      else
         mpUsedHead = pB;
      pB->mpPrev = pAPrev;
      pB->mpNext = pA;
      
      pA->mpPrev = pB;
      pA->mpNext = pBNext;
      if (pBNext)
         pBNext->mpPrev = pA;
      else
         mpUsedTail = pA;
   }
   else if (pBNext == pA)
   {
      // other B A other
      if (pBPrev)
         pBPrev->mpNext = pA;
      else
         mpUsedHead = pA;
      pA->mpPrev = pBPrev;
      pA->mpNext = pB;

      pB->mpPrev = pA;
      pB->mpNext = pANext;
      if (pANext)
         pANext->mpPrev = pB;
      else
         mpUsedTail = pB;
   }
   else
   {
      // other A other   other B other
      
      if (pAPrev)
         pAPrev->mpNext = pB;
      else
         mpUsedHead = pB;
      
      pB->mpPrev = pAPrev;
      pB->mpNext = pANext;
      
      if (pANext)
         pANext->mpPrev = pB;
      else
         mpUsedTail = pB;
         
      if (pBPrev)
         pBPrev->mpNext = pA;
      else
         mpUsedHead = pA;
       
      pA->mpPrev = pBPrev;
      pA->mpNext = pBNext;
      
      if (pBNext)
         pBNext->mpPrev = pA;
      else
         mpUsedTail = pA;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::getItem(uint index) const
{
   if (index >= mSize)
      return end();

   BNode* pNode = mpUsedHead;

   for (uint item = 0; item < index; ++item)
      pNode = pNode->mpNext;

   return constIterator(this, &pNode->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::getItem(uint index) 
{
   if (index >= mSize)
      return end();

   BNode* pNode = mpUsedHead;

   for (uint item = 0; item < index; ++item)
      pNode = pNode->mpNext;

   return iterator(this, &pNode->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::getHead(void) const
{
   if (!mpUsedHead)
      return end();
   return constIterator(this, &mpUsedHead->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::getHead(void)
{
   if (!mpUsedHead)
      return end();
   return iterator(this, &mpUsedHead->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::getTail(void) const
{
   if (!mpUsedHead)
      return end();
   return constIterator(this, &mpUsedTail->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::getTail(void) 
{
   if (!mpUsedHead)
      return end();
   return iterator(this, &mpUsedTail->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::getPrev(constIterator hItem) const
{
   BLLIST_VALIDATE_ITERATOR(hItem);
   const BNode* pRefNode = hItem.getNode();

   if (isEnd(pRefNode))
      return getTail();
   
   if (pRefNode->mpPrev == NULL)
      return end();
      
   return constIterator(this, &pRefNode->mpPrev->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::getPrev(iterator hItem) 
{
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem.getNode();

   if (isEnd(pRefNode))
      return getTail();

   if (pRefNode->mpPrev == NULL)
      return end();

   return iterator(this, &pRefNode->mpPrev->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::getNext(constIterator hItem) const
{
   BLLIST_VALIDATE_ITERATOR(hItem);
   const BNode* pRefNode = hItem.getNode();
      
   if (isEnd(pRefNode))
      return end();

   if (pRefNode->mpNext == NULL)
      return end();

   return constIterator(this, &pRefNode->mpNext->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::getNext(iterator hItem) 
{
   BLLIST_VALIDATE_ITERATOR(hItem);
   BNode* pRefNode = hItem.getNode();
   
   if (isEnd(pRefNode))
      return end();

   if (pRefNode->mpNext == NULL)
      return end();

   return iterator(this, &pRefNode->mpNext->mItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::getPrevWithWrap(constIterator hItem) const
{
   if (hItem == begin())
      return getTail();
   return getPrev(hItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::getPrevWithWrap(iterator hItem) 
{
   if (hItem == begin())
      return getTail();
   return getPrev(hItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::getNextWithWrap(constIterator hItem) const
{
   constIterator it(getNext(hItem));
   if (it.isEnd())
      return getHead();
   return it;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::iterator BLinkedList<T, Allocator>::getNextWithWrap(iterator hItem) 
{
   iterator it(getNext(hItem));
   if (it.isEnd())
      return getHead();
   return it;
}


//============================================================================
//  SEARCHING
//============================================================================
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::findItemForward(const T& item, constIterator hStart) const
{
   return findItemForward(NULL, NULL, item, hStart);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::findItemBackward(const T& item, constIterator hStart) const
{
   return findItemBackward(NULL, NULL, item, hStart);  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::findItemForward(BCompareFunc* pFunc, void* pParam, const T& item, constIterator hStart) const
{
   BNode* pNode;

   if (hStart)
   {
      BLLIST_VALIDATE_ITERATOR(hStart);
      pNode = hStart.getNode();

      if (isEnd(pNode))
         return end();
   }
   else
   {
      pNode = mpUsedHead;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pFunc)
      {
         if (pFunc(pNode->mItem, item, pParam))
            return constIterator(this, &pNode->mItem);
      }
      else if (pNode->mItem == item)
         return constIterator(this, &pNode->mItem);
         
      pNode = pNode->mpNext;
   }

   return end();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::constIterator BLinkedList<T, Allocator>::findItemBackward(BCompareFunc* pFunc, void* pParam, const T& item, constIterator hStart) const
{
   //-- Get the node we are starting from.
   BNode* pNode;
   if (hStart)
   {
      BLLIST_VALIDATE_ITERATOR(hStart);
      pNode = hStart.getNode();

      if (isEnd(pNode))
         pNode = mpUsedTail;
   }
   else
   {
      pNode = mpUsedTail;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pFunc)
      {
         if (pFunc(pNode->mItem, item, pParam))
            return constIterator(this, &pNode->mItem);
      }
      else if (pNode->mItem == item)
         return constIterator(this, &pNode->mItem);
      pNode = pNode->mpPrev;
   }
   return end();
}

//============================================================================
//  HELPER FUNCTIONS
//============================================================================
template <class T, class Allocator> bool BLinkedList<T, Allocator>::isValidIterator(constIterator hItem) const
{
   if (hItem.isNull())
      return false;
   else if (hItem.isEnd())
      return true;

#if DEBUG_FULL_ITERATOR_VALIDATION      
   //-- Scan the used list, looking for this node.
   BNode* pNode = mpUsedHead;
   while (pNode)
   {
      if (pNode == hItem.getNode())
         return true;
      pNode = pNode->mpNext;
   }
   
   return false;
#else
   return true;   
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::addToFreeHead(BNode* pNode, bool destruct)
{
   BDEBUG_ASSERT(pNode);
   
   if (destruct)
   {
      pNode->destruct();
      updateValidObjects(-1);
   }
   
   if (!mUseBlockAllocator)
   {
      ALLOCATOR_DELETE(pNode, mAllocator);
      BDEBUG_ASSERT(mAllocatedSize);
      mAllocatedSize--;
   }
   else
   {
      pNode->mpPrev = NULL;
      pNode->mpNext = mpFreeHead;
      if (mpFreeHead)
         mpFreeHead->mpPrev = pNode;
      mpFreeHead = pNode;
   }      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::addBlock(uint numNodes)
{
   BDEBUG_ASSERT(mUseBlockAllocator);
   BDEBUG_ASSERT(numNodes > 0);
   BDEBUG_ASSERT(numNodes != cGrowExponential);

   //-- Allocate the block.
   BBlock* pBlock = ALLOCATOR_NEW(BBlock, mAllocator);
   if (!pBlock)
   {
      BDEBUG_ASSERT(0);
      return;
   }

   pBlock->mpNodes = static_cast<BNode*>(mAllocator.alloc(sizeof(BNode) * numNodes));
   if (!pBlock->mpNodes)
   {
      BFATAL_FAIL("BLinkedList::addBlock: alloc failed");
   }

   //-- Add it to the block list.
   pBlock->mpNextBlock = mpBlocks;
   mpBlocks = pBlock;

   //-- Add all the nodes to the free list.
   pBlock->mNumNodes = numNodes;
   for (uint node = numNodes; node > 0; --node)
      addToFreeHead(&pBlock->mpNodes[node - 1], false);

   //-- Adjust the allocated size.
   mAllocatedSize += numNodes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::BNode* BLinkedList<T, Allocator>::getFreeNode()
{
   if (!mpFreeHead) 
   {
      if (!mUseBlockAllocator)
      {
         BNode* pNode = (BNode*)mAllocator.alloc(sizeof(BNode));
         if (!pNode)
         {
            BFATAL_FAIL("BLinkedList::getFreeNode: alloc failed");
         }
         
         pNode->mpNext = NULL;
         pNode->mpPrev = NULL;
         
         mAllocatedSize++;
         
         return pNode;
      }
      else
      {
         //-- Grow the list.
         BDEBUG_ASSERT(mGrowSize != 0);
         BDEBUG_ASSERT(mGrowSize != cGrowExponential);
         addBlock(mGrowSize);
         if (mGrowExponential)
            mGrowSize *= 2;
      }
   }
   
   BDEBUG_ASSERT(mpFreeHead);
   BNode* pNode = mpFreeHead;
   mpFreeHead = pNode->mpNext;
   if (mpFreeHead)
      mpFreeHead->mpPrev = NULL;

   //-- Return it.
   pNode->mpNext = NULL;
   BDEBUG_ASSERT(pNode->mpPrev == NULL);
   return pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::swapNodes(BNode*& pNode1, BNode*& pNode2)
{
   BDEBUG_ASSERT(pNode1);
   BDEBUG_ASSERT(pNode2);
   BDEBUG_ASSERT(mpUsedHead);
   BDEBUG_ASSERT(mpUsedTail);

   if (pNode1 == pNode2)
      return;

   //-- Store the node to insert pNode1 before.
   BNode* pRefNode;

   //-- Put pNode2 right before pNode1.
   if (pNode2->mpNext != pNode1)
   {
      pRefNode = pNode2->mpNext;

      //-- Remove pNode2 from the list.
      if (pNode2->mpPrev == NULL)
      {
         BDEBUG_ASSERT(mpUsedHead == pNode2);
         mpUsedHead = pNode2->mpNext;
      }
      else
      {
         pNode2->mpPrev->mpNext = pNode2->mpNext;
      }
      if (pNode2->mpNext == NULL)
      {
         BDEBUG_ASSERT(mpUsedTail == pNode2);
         mpUsedTail = pNode2->mpPrev;
      }
      else
      {
         pNode2->mpNext->mpPrev = pNode2->mpPrev;
      }
   
      //-- Insert pNode2 before pNode1.
      pNode2->mpNext = pNode1;
      pNode2->mpPrev = pNode1->mpPrev;
      if (pNode1->mpPrev)
      {
         pNode1->mpPrev->mpNext = pNode2;
      }
      else
      {
         BDEBUG_ASSERT(mpUsedHead == pNode1);
         mpUsedHead = pNode2;
      }
      pNode1->mpPrev = pNode2;
   }
   else
   {
      pRefNode = pNode2;
   }

   //-- Put pNode1 right before pRefNode.
   if (pNode1->mpNext != pRefNode)
   {
      //-- Remove pNode1 from the list.
      if (pNode1->mpPrev == NULL)
      {
         BDEBUG_ASSERT(mpUsedHead == pNode1);
         mpUsedHead = pNode1->mpNext;
      }
      else
      {
         pNode1->mpPrev->mpNext = pNode1->mpNext;
      }
      if (pNode1->mpNext == NULL)
      {
         BDEBUG_ASSERT(mpUsedTail == pNode1);
         mpUsedTail = pNode1->mpPrev;
      }
      else
      {
         pNode1->mpNext->mpPrev = pNode1->mpPrev;
      }

      //-- Insert pNode1 back in the list.
      if (pRefNode == NULL)
      {
         //-- Add it to the tail.
         pNode1->mpPrev     = mpUsedTail;
         pNode1->mpNext     = NULL;
         mpUsedTail->mpNext = pNode1;
         mpUsedTail        = pNode1;
      }
      else
      {
         //-- Insert pNode1 before pRefNode.
         pNode1->mpNext = pRefNode;
         pNode1->mpPrev = pRefNode->mpPrev;
         if (pRefNode->mpPrev)
         {
            pRefNode->mpPrev->mpNext = pNode1;
         }
         else
         {
            BDEBUG_ASSERT(mpUsedHead == pRefNode);
            mpUsedHead = pNode1;
         }
         pRefNode->mpPrev = pNode1;
      }
   }

   //-- Swap the node pointers.
   BNode* pTemp = pNode1;
   pNode1 = pNode2;
   pNode2 = pTemp;
/*
   //-- Validate list integrity.
   uint count = 0;
   BNode* p = mpUsedHead;
   while (p)
   {
      if (p->mpNext) BDEBUG_ASSERT(p->mpNext->mpPrev == p);
      if (p->mpPrev) BDEBUG_ASSERT(p->mpPrev->mpNext == p);
      count++;
      p = p->mpNext;
   }
   BDEBUG_ASSERT(count == mSize);
*/
//   T temp = pNode1->mItem;
//   pNode1->mItem = pNode2->mItem;
//   pNode2->mItem = temp;
}

//----------------------------------------------------------------------------
//  linkToHead()
//  Note that this only links the node in.  It does not update the list size
//  or do any other housekeeping.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::linkToHead(BNode* pNode)
{
   BDEBUG_ASSERT(pNode);

   //-- Special case for empty list.
   if (mpUsedHead == NULL)
   {
      BDEBUG_ASSERT(mpUsedTail == NULL);
      BDEBUG_ASSERT(mSize      == 0);
      mpUsedHead   = pNode;
      mpUsedTail   = pNode;
      pNode->mpPrev = NULL;
      pNode->mpNext = NULL;
      return;
   }

   //-- General case.
   pNode->mpPrev      = NULL;
   pNode->mpNext      = mpUsedHead;
   mpUsedHead->mpPrev = pNode;
   mpUsedHead        = pNode;
}

//----------------------------------------------------------------------------
//  linkToTail()
//  Note that this only links the node in.  It does not update the list size
//  or do any other housekeeping.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::linkToTail(BNode* pNode)
{
   BDEBUG_ASSERT(pNode);

   //-- Special case for empty list.
   if (mpUsedTail == NULL)
   {
      BDEBUG_ASSERT(mpUsedHead == NULL);
      BDEBUG_ASSERT(mSize      == 0);
      mpUsedHead   = pNode;
      mpUsedTail   = pNode;
      pNode->mpPrev = NULL;
      pNode->mpNext = NULL;
      return;
   }

   //-- General case.
   pNode->mpPrev      = mpUsedTail;
   pNode->mpNext      = NULL;
   mpUsedTail->mpNext = pNode;
   mpUsedTail         = pNode;
}

//----------------------------------------------------------------------------
//  unlink()
//  Note that this simply unlinks the node from the list.  It does not update
//  the list size or do any other housekeeping.
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::unlink(BNode* pNode)
{
   BDEBUG_ASSERT(pNode);

   if (pNode->mpPrev == NULL)
   {
      BDEBUG_ASSERT(mpUsedHead == pNode);
      mpUsedHead = pNode->mpNext;
   }
   else
   {
      BDEBUG_ASSERT(pNode->mpPrev->mpNext == pNode);
      pNode->mpPrev->mpNext = pNode->mpNext;
   }

   if (pNode->mpNext == NULL)
   {
      BDEBUG_ASSERT(mpUsedTail == pNode);
      mpUsedTail = pNode->mpPrev;
   }
   else
   {
      BDEBUG_ASSERT(pNode->mpNext->mpPrev == pNode);
      pNode->mpNext->mpPrev = pNode->mpPrev;
   }
}


//============================================================================
//  SORTING FUNCTIONS
//============================================================================

//----------------------------------------------------------------------------
//  quickSort()
//  Sorts a list really fast.  Unlike BPointerList::quickSort(),
//  BLinkedList::quickSort() does not cause existing list handles to become
//  invalid.  You should use insertionSort() instead if you know the list is
//  mostly sorted already. You must have the < and > operators defined in
//  class T.  
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::quickSort(bool ascending)
{
   quickSort(ascending, mSize, mpUsedHead, mpUsedTail);
}

//----------------------------------------------------------------------------
//  insertionSort()
//  Slower than quickSort(), but has two advantages:
//  1)  insertionSort() doesn't invalidate existing handles.
//  2)  is faster than quickSort() for lists that are "mostly" sorted.
//  You must have the < and > operators defined in class T.  
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::insertionSort(bool ascending)
{
   insertionSort(ascending, mSize, mpUsedHead, mpUsedTail);
}

//----------------------------------------------------------------------------
//  quickSort()
//  Just like the quickSort() above, except you don't need the < and >
//  operators defined in class T.  Instead you pass in a compare function 
//  [pFunc(item1, item2, pParam)] that returns:
//   < 0 if item1  < item2
//   > 0 if item1  > item2
//  == 0 if item1 == item2
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::quickSort(BCompareFunc* pFunc, void* pParam, bool ascending)
{
   quickSort(ascending, mSize, mpUsedHead, mpUsedTail, pFunc, pParam);
}

//----------------------------------------------------------------------------
//  insertionSort()
//  Just like the insertionSort() above, except you don't need the < and >
//  operators defined in class T.  Instead you pass in a compare function 
//  [pFunc(item1, item2, pParam)] that returns:
//   < 0 if item1  < item2
//   > 0 if item1  > item2
//  == 0 if item1 == item2
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::insertionSort(BCompareFunc* pFunc, void* pParam, bool ascending)
{
   insertionSort(ascending, mSize, mpUsedHead, mpUsedTail, pFunc, pParam);
}

//----------------------------------------------------------------------------
//  getPivot()
//  This function finds a node to partition the list around.  Ideally this
//  node would be the median of the items between pFirst and pLast so that
//  the two resulting partitions would be equal in size.  But that is very
//  impractical to compute.  So what we do is use the median of 3 good values:
//  the first, middle and last nodes in the list.  Choosing these nodes gives
//  us good performance for both sorted and unsorted lists, which is VERY
//  important.
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::BNode* BLinkedList<T, Allocator>::getPivot(bool ascending, uint numNodes, BNode*& pFirst, BNode*& pLast)
{
   BDEBUG_ASSERT(numNodes > 3);
   BDEBUG_ASSERT(pFirst);
   BDEBUG_ASSERT(pLast);

   if (!pFirst || !pLast)
      return 0; 

   //-- Determine the pivot.  Oh my god, getting the third value from the
   //-- center of the list (as opposed to a neighbor of pFirst or pLast)
   //-- appears to be slow and horrible. But doing this little bit of work
   //-- here REALLY saves us big if the list is mostly sorted.  As for
   //-- normal cases, it costs a little but not that much.  It still blows
   //-- the doors off of qsort().
   BNode* pPivot   = pFirst;
   uint  numSteps = numNodes / 2;
   for (uint step = 0; step < numSteps; ++step)
      pPivot = pPivot->mpNext;

   //-- Sort the 3 nodes.
   if (ascending)
   {
      if (pFirst->mItem > pPivot->mItem) swapNodes(pFirst, pPivot);
      if (pFirst->mItem > pLast ->mItem) swapNodes(pFirst, pLast );
      if (pPivot->mItem > pLast ->mItem) swapNodes(pPivot, pLast );
   }
   else
   {
      if (pFirst->mItem < pPivot->mItem) swapNodes(pFirst, pPivot);
      if (pFirst->mItem < pLast ->mItem) swapNodes(pFirst, pLast );
      if (pPivot->mItem < pLast ->mItem) swapNodes(pPivot, pLast );
   }

   //-- Move the pivot node out of the way, cuz we don't need to sort it.
   //-- We'll move it back after we are done partitioning.
   BNode* pTemp = pLast->mpPrev;
   swapNodes(pPivot, pTemp);
   pPivot = pTemp;
   return pPivot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::quickSort(bool ascending, uint numNodes, BNode* pFirst, BNode* pLast)
{
   //-- See if its time to hand over to insertion sort.
   if (numNodes <= LLIST_QSORT_CUTOFF)
   {
      insertionSort(ascending, numNodes, pFirst, pLast);
      return;
   }

   //-- Determine the pivot node.
   if (!pFirst || !pLast)
   {
      BDEBUG_ASSERT(0);
      return;
   }
   BNode* pPivot = getPivot(ascending, numNodes, pFirst, pLast);

   //-- Partition the list.
   uint  left   = 0;
   uint  right  = numNodes - 2;
   BNode* pLeft  = pFirst;
   BNode* pRight = pLast->mpPrev;
   BASSERT(pRight);

   if (ascending)
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->mpNext;
         pRight = pRight->mpPrev;

         while (pLeft->mItem  < pPivot->mItem) { ++left;  pLeft  = pLeft->mpNext;  }
         while (pRight->mItem > pPivot->mItem) { --right; pRight = pRight->mpPrev; }

         if (left < right)
         {
            if (pFirst == pLeft ) pFirst = pRight;
            if (pLast  == pRight) pLast  = pLeft;
            swapNodes(pLeft, pRight);
         }
         else
         {
            break;
         }
      }
   }
   else
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->mpNext;
         pRight = pRight->mpPrev; 

         while (pLeft->mItem  > pPivot->mItem) { ++left;  pLeft  = pLeft->mpNext;  }
         while (pRight->mItem < pPivot->mItem) { --right; pRight = pRight->mpPrev; }

         if (left < right)
         {
            if (pFirst == pLeft ) pFirst = pRight;
            if (pLast  == pRight) pLast  = pLeft;
            swapNodes(pLeft, pRight);
         }
         else
         {
            break;
         }
      }
   }

   //-- Swap the pivot node back to its proper spot.
   swapNodes(pLeft, pPivot);

   //-- Recursively partition our partitions.
   uint numLeftNodes  = left;
   uint numRightNodes = numNodes - left - 1;
   quickSort(ascending, numLeftNodes,  pFirst,       pLeft->mpPrev);
   quickSort(ascending, numRightNodes, pLeft->mpNext, pLast);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::insertionSort(bool ascending, uint numNodes, BNode* pFirst, BNode* pLast)
{
   //-- Gotta have enough nodes to sort.
   if (numNodes < 2)
      return;

   //-- Set up the start and end nodes.
   BDEBUG_ASSERT(pFirst);
   BDEBUG_ASSERT(pLast);
   BNode* pLeftTerminator  = pFirst->mpPrev;
   BNode* pRightTerminator = pLast->mpNext;
   BNode* pNode            = pFirst->mpNext;
   if (ascending)
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->mpPrev are not NULL, so we
         //-- don't need to check.
         BNode* pNextNode = pNode->mpNext;
         if (pNode->mItem < pNode->mpPrev->mItem)
         {
            //-- Remove the node.
            if (pNode->mpNext == NULL)
            {
               BDEBUG_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->mpPrev;
            }
            else
            {
               pNode->mpNext->mpPrev = pNode->mpPrev;
            }
            pNode->mpPrev->mpNext = pNode->mpNext;

            //-- Find the first node that is <= to pNode.
            BNode* pRefNode = pNode->mpPrev->mpPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (!(pRefNode->mItem > pNode->mItem))
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->mpNext is not NULL.
                  pNode->mpNext           = pRefNode->mpNext;
                  pNode->mpPrev           = pRefNode;
                  pRefNode->mpNext->mpPrev = pNode;
                  pRefNode->mpNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->mpPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BDEBUG_ASSERT(mpUsedHead == pFirst);
                  pNode ->mpNext = pFirst;
                  pNode ->mpPrev = NULL;
                  pFirst->mpPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->mpPrev is not NULL.
                  pNode ->mpNext        = pFirst;
                  pNode ->mpPrev        = pFirst->mpPrev;
                  pFirst->mpPrev->mpNext = pNode;
                  pFirst->mpPrev        = pNode;
               }
               pFirst = pNode;
            }
         }
         pNode = pNextNode;
      }
   }
   else
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->mpPrev are not NULL, so we
         //-- don't need to check.
         BNode* pNextNode = pNode->mpNext;
         if (pNode->mItem > pNode->mpPrev->mItem)
         {
            //-- Remove the node.
            if (pNode->mpNext == NULL)
            {
               BDEBUG_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->mpPrev;
            }
            else
            {
               pNode->mpNext->mpPrev = pNode->mpPrev;
            }
            pNode->mpPrev->mpNext = pNode->mpNext;

            //-- Find the first node that is >= to pNode.
            BNode* pRefNode = pNode->mpPrev->mpPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (!(pRefNode->mItem < pNode->mItem))
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->mpNext is not NULL.
                  pNode->mpNext           = pRefNode->mpNext;
                  pNode->mpPrev           = pRefNode;
                  pRefNode->mpNext->mpPrev = pNode;
                  pRefNode->mpNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->mpPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BDEBUG_ASSERT(mpUsedHead == pFirst);
                  pNode ->mpNext = pFirst;
                  pNode ->mpPrev = NULL;
                  pFirst->mpPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->mpPrev is not NULL.
                  pNode ->mpNext        = pFirst;
                  pNode ->mpPrev        = pFirst->mpPrev;
                  pFirst->mpPrev->mpNext = pNode;
                  pFirst->mpPrev        = pNode;
               }
               pFirst = pNode;
            }
         }
         pNode = pNextNode;
      }
   }
}

//----------------------------------------------------------------------------
//  getPivot()
//  This function finds a node to partition the list around.  Ideally this
//  node would be the median of the items between pFirst and pLast so that
//  the two resulting partitions would be equal in size.  But that is very
//  impractical to compute.  So what we do is use the median of 3 good values:
//  the first, middle and last nodes in the list.  Choosing these nodes gives
//  us good performance for both sorted and unsorted lists, which is VERY
//  important.
//----------------------------------------------------------------------------
template <class T, class Allocator> typename BLinkedList<T, Allocator>::BNode* BLinkedList<T, Allocator>::getPivot(bool ascending, uint numNodes, BNode*& pFirst, BNode*& pLast, BCompareFunc* pFunc, void* pParam)
{
   BDEBUG_ASSERT(numNodes > 3);
   BDEBUG_ASSERT(pFirst);
   BDEBUG_ASSERT(pLast);
   BDEBUG_ASSERT(pFunc);

   //-- Determine the pivot.  Oh my god, getting the third value from the
   //-- center of the list (as opposed to a neighbor of pFirst or pLast)
   //-- appears to be slow and horrible. But doing this little bit of work
   //-- here REALLY saves us big if the list is mostly sorted.  As for
   //-- normal cases, it costs a little but not that much.  It still blows
   //-- the doors off of qsort().
   BNode* pPivot   = pFirst;
   uint  numSteps = numNodes / 2;
   for (uint step = 0; step < numSteps; ++step)
      pPivot = pPivot->mpNext;

   //-- Sort the 3 nodes.
   if (ascending)
   {
      if (pFunc(pFirst->mItem, pPivot->mItem, pParam) > 0) swapNodes(pFirst, pPivot);
      if (pFunc(pFirst->mItem, pLast ->mItem, pParam) > 0) swapNodes(pFirst, pLast );
      if (pFunc(pPivot->mItem, pLast ->mItem, pParam) > 0) swapNodes(pPivot, pLast );
   }
   else
   {
      if (pFunc(pFirst->mItem, pPivot->mItem, pParam) < 0) swapNodes(pFirst, pPivot);
      if (pFunc(pFirst->mItem, pLast ->mItem, pParam) < 0) swapNodes(pFirst, pLast );
      if (pFunc(pPivot->mItem, pLast ->mItem, pParam) < 0) swapNodes(pPivot, pLast );
   }

   //-- Move the pivot node out of the way, cuz we don't need to sort it.
   //-- We'll move it back after we are done partitioning.
   BNode* pTemp = pLast->mpPrev;
   swapNodes(pPivot, pTemp);
   pPivot = pTemp;
   return pPivot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::quickSort(bool ascending, uint numNodes, BNode* pFirst, BNode* pLast, BCompareFunc* pFunc, void* pParam)
{
   BDEBUG_ASSERT(pFunc);

   //-- See if its time to hand over to insertion sort.
   if (numNodes <= LLIST_QSORT_CUTOFF)
   {
      insertionSort(ascending, numNodes, pFirst, pLast, pFunc, pParam);
      return;
   }

   //-- Determine the pivot node.
   BDEBUG_ASSERT(pFirst);
   BDEBUG_ASSERT(pLast);
   BNode* pPivot = getPivot(ascending, numNodes, pFirst, pLast, pFunc, pParam);

   //-- Partition the list.
   uint  left   = 0;
   uint  right  = numNodes - 2;
   BNode* pLeft  = pFirst;
   BNode* pRight = pLast->mpPrev;
   BASSERT(pRight);

   if (ascending)
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->mpNext;
         pRight = pRight->mpPrev; 

         while (pFunc(pLeft->mItem,  pPivot->mItem, pParam) < 0) { ++left;  pLeft  = pLeft->mpNext;  }
         while (pFunc(pRight->mItem, pPivot->mItem, pParam) > 0) { --right; pRight = pRight->mpPrev; }

         if (left < right)
         {
            if (pFirst == pLeft ) pFirst = pRight;
            if (pLast  == pRight) pLast  = pLeft;
            swapNodes(pLeft, pRight);
         }
         else
         {
            break;
         }
      }
   }
   else
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->mpNext;
         pRight = pRight->mpPrev; 

         while (pFunc(pLeft->mItem,  pPivot->mItem, pParam) > 0) { ++left;  pLeft  = pLeft->mpNext;  }
         while (pFunc(pRight->mItem, pPivot->mItem, pParam) < 0) { --right; pRight = pRight->mpPrev; }

         if (left < right)
         {
            if (pFirst == pLeft ) pFirst = pRight;
            if (pLast  == pRight) pLast  = pLeft;
            swapNodes(pLeft, pRight);
         }
         else
         {
            break;
         }
      }
   }

   //-- Swap the pivot node back to its proper spot.
   swapNodes(pLeft, pPivot);

   //-- Recursively partition our partitions.
   uint numLeftNodes  = left;
   uint numRightNodes = numNodes - left - 1;
   quickSort(ascending, numLeftNodes,  pFirst,       pLeft->mpPrev, pFunc, pParam);
   quickSort(ascending, numRightNodes, pLeft->mpNext, pLast,        pFunc, pParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T, class Allocator> void BLinkedList<T, Allocator>::insertionSort(bool ascending, uint numNodes, BNode* pFirst, BNode* pLast, BCompareFunc* pFunc, void* pParam)
{
   BDEBUG_ASSERT(pFunc);

   //-- Gotta have enough nodes to sort.
   if (numNodes < 2)
      return;

   //-- Set up the start and end nodes.
   BDEBUG_ASSERT(pFirst);
   BDEBUG_ASSERT(pLast);
   BNode* pLeftTerminator  = pFirst->mpPrev;
   BNode* pRightTerminator = pLast->mpNext;
   BNode* pNode            = pFirst->mpNext;
   if (ascending)
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->mpPrev are not NULL, so we
         //-- don't need to check.
         BNode* pNextNode = pNode->mpNext;
         if (pFunc(pNode->mItem, pNode->mpPrev->mItem, pParam) < 0)
         {
            //-- Remove the node.
            if (pNode->mpNext == NULL)
            {
               BDEBUG_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->mpPrev;
            }
            else
            {
               pNode->mpNext->mpPrev = pNode->mpPrev;
            }
            pNode->mpPrev->mpNext = pNode->mpNext;

            //-- Find the first node that is <= to pNode.
            BNode* pRefNode = pNode->mpPrev->mpPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (pFunc(pRefNode->mItem, pNode->mItem, pParam) <= 0)
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->mpNext is not NULL.
                  pNode->mpNext           = pRefNode->mpNext;
                  pNode->mpPrev           = pRefNode;
                  pRefNode->mpNext->mpPrev = pNode;
                  pRefNode->mpNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->mpPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BDEBUG_ASSERT(mpUsedHead == pFirst);
                  pNode ->mpNext = pFirst;
                  pNode ->mpPrev = NULL;
                  pFirst->mpPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->mpPrev is not NULL.
                  pNode ->mpNext        = pFirst;
                  pNode ->mpPrev        = pFirst->mpPrev;
                  pFirst->mpPrev->mpNext = pNode;
                  pFirst->mpPrev        = pNode;
               }
               pFirst = pNode;
            }
         }
         pNode = pNextNode;
      }
   }
   else
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->mpPrev are not NULL, so we
         //-- don't need to check.
         BNode* pNextNode = pNode->mpNext;
         if (pFunc(pNode->mItem, pNode->mpPrev->mItem, pParam) > 0)
         {
            //-- Remove the node.
            if (pNode->mpNext == NULL)
            {
               BDEBUG_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->mpPrev;
            }
            else
            {
               pNode->mpNext->mpPrev = pNode->mpPrev;
            }
            pNode->mpPrev->mpNext = pNode->mpNext;

            //-- Find the first node that is >= to pNode.
            BNode* pRefNode = pNode->mpPrev->mpPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (pFunc(pRefNode->mItem, pNode->mItem, pParam) >= 0)
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->mpNext is not NULL.
                  pNode->mpNext           = pRefNode->mpNext;
                  pNode->mpPrev           = pRefNode;
                  pRefNode->mpNext->mpPrev = pNode;
                  pRefNode->mpNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->mpPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BDEBUG_ASSERT(mpUsedHead == pFirst);
                  pNode ->mpNext = pFirst;
                  pNode ->mpPrev = NULL;
                  pFirst->mpPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->mpPrev is not NULL.
                  pNode ->mpNext        = pFirst;
                  pNode ->mpPrev        = pFirst->mpPrev;
                  pFirst->mpPrev->mpNext = pNode;
                  pFirst->mpPrev        = pNode;
               }
               pFirst = pNode;
            }
         }
         pNode = pNextNode;
      }
   }
}


//============================================================================
//  OPERATORS
//============================================================================
template <class T, class Allocator> BLinkedList<T, Allocator>& BLinkedList<T, Allocator>::operator = (const BLinkedList<T, Allocator>& list)
{
   if (this == &list)
   {
      return *this;
   }
   
   //-- Reset this list with the other lists size.
   reset(list.mSize, 0);

   //-- Add all the elements from the other list.

   T*      pNode = list.getHead();
   while (pNode)
   {
      addToTail(*pNode);
      pNode = list.getNext(hNode);
   }

   //-- Copy the other list's grow size.
   mGrowSize        = list.mGrowSize;
   mGrowExponential = list.mGrowExponential;

   return *this;
}

//============================================================================
// updateValidObjects
//============================================================================
template <class T, class Allocator> void BLinkedList<T, Allocator>::updateValidObjects(int delta)
{
#ifdef BUILD_DEBUG
   mNumValidObjects += delta;
   BDEBUG_ASSERT(mNumValidObjects >= 0);
#endif
}

//============================================================================
//  UNDEFINE PRIVATE MACROS
//============================================================================
#undef BLLIST_VALIDATE_ITERATOR




