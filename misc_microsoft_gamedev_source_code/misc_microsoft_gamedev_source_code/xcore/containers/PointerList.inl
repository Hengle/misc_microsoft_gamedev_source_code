//============================================================================
//
//  PointerList.inl
//  
//  Copyright (c) 1999-2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
const unsigned long BPLIST_QSORT_CUTOFF = 32;


//============================================================================
//  PRIVATE MACROS
//============================================================================
#define BPLIST_NODE_SWAP(p1, p2) { T* pTemp = p1->pItem; p1->pItem = p2->pItem; p2->pItem = pTemp; }

#if DEBUG_VALIDATE_LIST_HANDLES
   #define BPLIST_VALIDATE_HANDLE(handle) BFATAL_ASSERT(isValidHandle(handle))
#else
   #define BPLIST_VALIDATE_HANDLE(handle)
#endif


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
template <class T> BPointerList<T>::BPointerList(BMemoryHeap* pHeap) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL),
   mpHeap           (pHeap)
{
   reset(0, BPLIST_GROW_EXPONENTIAL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BPointerList<T>::BPointerList(unsigned long initialSize, unsigned long growSize, BMemoryHeap* pHeap) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL),
   mpHeap           (pHeap)
{
   reset(initialSize, growSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BPointerList<T>::BPointerList(const BPointerList<T>& list, BMemoryHeap* pHeap) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL),
   mpHeap           (pHeap)
{
   *this = list;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BPointerList<T>::~BPointerList()
{
   reset(0, 0);
}


//============================================================================
//  LIST CONTROL
//============================================================================
//----------------------------------------------------------------------------
//  reset()
//  Releases all memory and re-initializes the list.  initialSize specifies
//  the number of nodes to pre-allocate.  growSize specifies how many nodes
//  to allocate each time the list needs to grow.  If growSize equals
//  BPLIST_GROW_EXPONENTIAL the amount the list grows each time will double.
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::reset(unsigned long initialSize, unsigned long growSize)
{
   //-- Release the old memory.
   BPBlock* pBlock = mpBlocks;
   while (pBlock)
   {
      BPBlock* pKill = pBlock;
      pBlock = pBlock->pNextBlock;
      ALIGNED_DELETE_ARRAY(pKill->pNodes, *mpHeap);
      ALIGNED_DELETE(pKill, *mpHeap);
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
   mGrowExponential = (mGrowSize == BPLIST_GROW_EXPONENTIAL);
   
   if (mGrowExponential)
      mGrowSize = (initialSize == 0) ? 1 : initialSize;
      
   //-- Allocate the initial block.
   if (initialSize)
      addBlock(initialSize);
}

//----------------------------------------------------------------------------
//  empty()
//  Releases all the used nodes back to the free list.  This does not release
//  any memory or change the current grow size.
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::empty()
{
   //-- Give all the nodes back to the free list.
   BPNode* pNode = mpUsedTail;
   while (pNode)
   {
      BPNode* pKill = pNode;
      pNode = pNode->pPrev;

      //-- Add the node to the free head.
      addToFreeHead(pKill);
   }

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
template <class T> void BPointerList<T>::optimize()
{
   //-- Allocate the block.
   BPBlock* pNewBlock = ALIGNED_NEW(BPBlock, *mpHeap);
   pNewBlock->numNodes   = mSize;
   pNewBlock->pNextBlock = NULL;
   pNewBlock->pNodes     = ALIGNED_NEW_ARRAY(BPNode, mSize, *mpHeap);

   //-- Fill the list.
   unsigned long  index = 0;
   BPNode* pNode = mpUsedHead;
   while (pNode)
   {
      pNewBlock->pNodes[index].pItem = pNode->pItem;
      pNewBlock->pNodes[index].pPrev = (index == 0)         ? NULL : &pNewBlock->pNodes[index - 1];
      pNewBlock->pNodes[index].pNext = (index == mSize - 1) ? NULL : &pNewBlock->pNodes[index + 1];
      index++;

      pNode = pNode->pNext;
   }

   //-- Release the old memory.
   BPBlock* pBlock = mpBlocks;
   while (pBlock)
   {
      BPBlock* pKill = pBlock;
      pBlock = pBlock->pNextBlock;
      ALIGNED_DELETE_ARRAY(pKill->pNodes, *mpHeap);
      ALIGNED_DELETE(pKill, *mpHeap);
   }

   //-- Reset the list pointers.
   mpBlocks   = pNewBlock;
   mpFreeHead = NULL;
   mpUsedHead = &pNewBlock->pNodes[0];
   mpUsedTail = &pNewBlock->pNodes[mSize - 1];

   //-- Set the new size.
   mAllocatedSize = mSize;
}

//----------------------------------------------------------------------------
//  setGrowSize()
//  Sets the number of nodes that will be allocated when the list needs to
//  grow.  If growSize == BPLIST_GROW_EXPONENTIAL, the number of nodes that it
//  grows by will be set to the list's current allocated size and double each
//  time the list grows.
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::setGrowSize(unsigned long growSize)
{
   mGrowSize        = growSize;
   mGrowExponential = (mGrowSize == BPLIST_GROW_EXPONENTIAL);
   
   if (mGrowExponential)
      mGrowSize = (mAllocatedSize == 0) ? 1 : mAllocatedSize;
}


//============================================================================
//  LIST INFO
//============================================================================
template <class T> bool BPointerList<T>::isEmpty() const
{
   return (mSize == 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BPointerList<T>::isGrowExponential() const
{
   return mGrowExponential;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BPointerList<T>::getSize() const
{
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BPointerList<T>::getGrowSize() const
{
   return mGrowSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BPointerList<T>::getAllocatedSize() const
{
   return mAllocatedSize;
}


//============================================================================
//  ADDING ITEMS
//============================================================================
template <class T> BHandle BPointerList<T>::addToHead(T* pItem)
{
   //-- Make sure we have an item.
   BFATAL_ASSERT(pItem);

   //-- Get a node off the free list.
   BPNode* pNode = getFreeNode();
   if (!pNode)
   {
      BFATAL_ASSERT(0);
      return 0;
   }
   pNode->pItem = pItem;

   //-- Add it.
   linkToHead(pNode);

   //-- Increase size.
   mSize++;

   //-- Give it back.
   return (BHandle)pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::addToTail(T* pItem)
{
   //-- Make sure we have an item.
   BFATAL_ASSERT(pItem);

   //-- Get a node off the free list.
   BPNode* pNode = getFreeNode();
   if (!pNode)
   {
      BFATAL_ASSERT(0);
      return 0;
   }
   pNode->pItem = pItem;

   //-- Add it.
   linkToTail(pNode);

   //-- Increase.
   mSize++;

   //-- Give it back.
   return (BHandle)pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::addBefore(T* pItem, BHandle hItem)
{
   //-- Make sure we have an item.
   BFATAL_ASSERT(pItem);

   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

   //-- Get a node off the free list.
   BPNode* pNode = getFreeNode();
   pNode->pItem = pItem;

   //-- Add it to the list.
   pNode->pNext = pRefNode;
   pNode->pPrev = pRefNode->pPrev;
   if (pRefNode->pPrev)
   {
      pRefNode->pPrev->pNext = pNode;
   }
   else
   {
      BFATAL_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pNode;
   }
   pRefNode->pPrev = pNode;
   mSize++;
   return (BHandle)pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::addAfter(T* pItem, BHandle hItem)
{
   //-- Make sure we have an item.
   BFATAL_ASSERT(pItem);

   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

   //-- Get a node off the free list.
   BPNode* pNode = getFreeNode();
   pNode->pItem = pItem;

   //-- Add it to the list.
   pNode->pPrev = pRefNode;
   pNode->pNext = pRefNode->pNext;
   if (pRefNode->pNext)
   {
      pRefNode->pNext->pPrev = pNode;
   }
   else
   {
      BFATAL_ASSERT(mpUsedTail == pRefNode);
      mpUsedTail = pNode;
   }
   pRefNode->pNext = pNode;
   mSize++;
   return (BHandle)pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::addSorted(T* pItem, bool ascending)
{
   //-- Make sure we have an item.
   BFATAL_ASSERT(pItem);

   //-- Get a node off the free list.
   BPNode* pNode = getFreeNode();
   if (!pNode)
   {
      BFATAL_ASSERT(0);
      return 0;
   }
   pNode->pItem = pItem;

   //-- Handle an empty list.
   if (mSize == 0)
   {
      mpUsedHead = pNode;
      mpUsedTail = pNode;
      mSize      = 1;
      return (BHandle)pNode;
   }

   //-- Find the spot in the list to put it.
   BPNode* pRefNode = mpUsedHead;
   if (ascending)
   {
      //-- Find the first node that is greater than or equal to pItem.
      while (pRefNode && (*pRefNode->pItem < *pItem))
         pRefNode = pRefNode->pNext;
   }
   else
   {
      //-- Find the first node that is less than or equal to pItem.
      while (pRefNode && (*pRefNode->pItem > *pItem))
         pRefNode = pRefNode->pNext;
   }

   //-- If pRefNode is NULL, we add it to the tail.
   if (pRefNode == NULL)
   {
      pNode->pPrev      = mpUsedTail;
      mpUsedTail->pNext = pNode;
      mpUsedTail        = pNode;
      mSize++;
      return (BHandle)pNode;
   }

   //-- Add the new node before pRefNode.
   pNode->pNext = pRefNode;
   pNode->pPrev = pRefNode->pPrev;
   if (pRefNode->pPrev)
   {
      pRefNode->pPrev->pNext = pNode;
   }
   else
   {
      BFATAL_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pNode;
   }
   pRefNode->pPrev = pNode;

   mSize++;
   return (BHandle)pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::addSorted(T* pItem, COMPARE_FUNC* pFunc, void* pParam, bool ascending)
{
   //-- Make sure we have an item.
   BFATAL_ASSERT(pItem);
   BFATAL_ASSERT(pFunc);

   //-- Get a node off the free list.
   BPNode* pNode = getFreeNode();
   pNode->pItem = pItem;

   //-- Handle an empty list.
   if (mSize == 0)
   {
      mpUsedHead = pNode;
      mpUsedTail = pNode;
      mSize      = 1;
      return (BHandle)pNode;
   }

   //-- Find the spot in the list to put it.
   BPNode* pRefNode = mpUsedHead;
   if (ascending)
   {
      //-- Find the first node that is greater than or equal to pItem.
      while (pRefNode && (pFunc(*pRefNode->pItem, *pItem, pParam) < 0))
         pRefNode = pRefNode->pNext;
   }
   else
   {
      //-- Find the first node that is less than or equal to pItem.
      while (pRefNode && (pFunc(*pRefNode->pItem, *pItem, pParam) > 0))
         pRefNode = pRefNode->pNext;
   }

   //-- If pRefNode is NULL, we add it to the tail.
   if (pRefNode == NULL)
   {
      pNode->pPrev      = mpUsedTail;
      mpUsedTail->pNext = pNode;
      mpUsedTail        = pNode;
      mSize++;
      return (BHandle)pNode;
   }

   //-- Add the new node before pRefNode.
   pNode->pNext = pRefNode;
   pNode->pPrev = pRefNode->pPrev;
   if (pRefNode->pPrev)
   {
      pRefNode->pPrev->pNext = pNode;
   }
   else
   {
      BFATAL_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pNode;
   }
   pRefNode->pPrev = pNode;

   mSize++;
   return (BHandle)pNode;
}


//============================================================================
//  REMOVING ITEMS
//============================================================================
template <class T> T* BPointerList<T>::removeHead()
{
   //-- Be nice and don't BFATAL_ASSERT.  So it can be used in a loop like:
   //-- while (p = list.removeHead()) ...
   if (mpUsedHead == NULL)
      return NULL;

   //-- Remove it from the list.
   T*      pItem    = mpUsedHead->pItem;
   BPNode* pOldNode = mpUsedHead;
   mpUsedHead = mpUsedHead->pNext;
   if (mpUsedHead == NULL)
   {
      BFATAL_ASSERT(mpUsedTail == pOldNode);
      BFATAL_ASSERT(mSize      == 1);
      mpUsedTail = NULL;
   }
   else
   {
      mpUsedHead->pPrev = NULL;
   }

   //-- Add it to the free head.
   addToFreeHead(pOldNode);
   mSize--;
   return pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::removeTail()
{
   //-- Be nice and don't BFATAL_ASSERT.  So it can be used in a loop like:
   //-- while (p = list.removeTail()) ...
   if (mpUsedTail == NULL)
      return NULL;

   //-- Remove it from the list.
   T*      pItem    = mpUsedTail->pItem;
   BPNode* pOldNode = mpUsedTail;
   mpUsedTail = mpUsedTail->pPrev;
   if (mpUsedTail == NULL)
   {
      BFATAL_ASSERT(mpUsedHead == pOldNode);
      BFATAL_ASSERT(mSize      == 1);
      mpUsedHead = NULL;
   }
   else
   {
      mpUsedTail->pNext = NULL;
   }

   //-- Add it to the free head.
   addToFreeHead(pOldNode);
   mSize--;
   return pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::remove(BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

   //-- Remove it from the list.
   if (pRefNode->pPrev == NULL)
   {
      BFATAL_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pRefNode->pNext;
   }
   else
   {
      pRefNode->pPrev->pNext = pRefNode->pNext;
   }
   if (pRefNode->pNext == NULL)
   {
      BFATAL_ASSERT(mpUsedTail == pRefNode);
      mpUsedTail = pRefNode->pPrev;
   }
   else
   {
      pRefNode->pNext->pPrev = pRefNode->pPrev;
   }

   //-- Add it to the free head.
   T* pItem = pRefNode->pItem;
   addToFreeHead(pRefNode);
   mSize--;
   return pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::removeAndGetPrev(BHandle& hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

   //-- Remove it from the list.
   if (pRefNode->pPrev == NULL)
   {
      BFATAL_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pRefNode->pNext;
   }
   else
   {
      pRefNode->pPrev->pNext = pRefNode->pNext;
   }
   if (pRefNode->pNext == NULL)
   {
      BFATAL_ASSERT(mpUsedTail == pRefNode);
      mpUsedTail = pRefNode->pPrev;
   }
   else
   {
      pRefNode->pNext->pPrev = pRefNode->pPrev;
   }

   //-- Get the prev node item and handle.
   T* pItem;
   if (pRefNode->pPrev)
   {
      pItem = pRefNode->pPrev->pItem;
      hItem = (BHandle)pRefNode->pPrev;
   }
   else
   {
      pItem = NULL;
      hItem = NULL;
   }

   //-- Add it to the free head.
   addToFreeHead(pRefNode);
   mSize--;
   return pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::removeAndGetNext(BHandle& hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

   //-- Remove it from the list.
   if (pRefNode->pPrev == NULL)
   {
      BFATAL_ASSERT(mpUsedHead == pRefNode);
      mpUsedHead = pRefNode->pNext;
   }
   else
   {
      pRefNode->pPrev->pNext = pRefNode->pNext;
   }
   if (pRefNode->pNext == NULL)
   {
      BFATAL_ASSERT(mpUsedTail == pRefNode);
      mpUsedTail = pRefNode->pPrev;
   }
   else
   {
      pRefNode->pNext->pPrev = pRefNode->pPrev;
   }

   //-- Get the next node item and handle.
   T* pItem;
   if (pRefNode->pNext)
   {
      pItem = pRefNode->pNext->pItem;
      hItem = (BHandle)pRefNode->pNext;
   }
   else
   {
      pItem = NULL;
      hItem = NULL;
   }

   //-- Add it to the free head.
   addToFreeHead(pRefNode);
   mSize--;
   return pItem;
}


//============================================================================
//  RELOCATION
//============================================================================
//----------------------------------------------------------------------------
//  moveToHead()
//  Moves the node referenced by hItem to the head.  All existing list
//  handles remain valid.
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::moveToHead(BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

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
template <class T> void BPointerList<T>::moveToTail(BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

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


//============================================================================
//  NAVIGATION   
//============================================================================
template <class T> T* BPointerList<T>::getItem(BHandle hItem) const
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;
   return pRefNode->pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::getItem(unsigned long index) const
{
   if (index >= mSize)
      return NULL;

   BPNode* pNode = mpUsedHead;

   for (unsigned long item = 0; item < index; ++item)
      pNode = pNode->pNext;

   return pNode->pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::getHead(BHandle& hItem) const
{
   if (mpUsedHead == NULL)
   {
      hItem = NULL;
      return NULL;
   }

   hItem = (BHandle)mpUsedHead;
   return mpUsedHead->pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::getTail(BHandle& hItem) const
{
   if (mpUsedTail == NULL)
   {
      hItem = NULL;
      return NULL;
   }

   hItem = (BHandle)mpUsedTail;
   return mpUsedTail->pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::getPrev(BHandle& hItem) const
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

   //-- Update hItem and return pItem.
   hItem = (BHandle)pRefNode->pPrev;
   if (pRefNode->pPrev == NULL)
      return NULL;
   return pRefNode->pPrev->pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::getNext(BHandle& hItem) const
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BPLIST_VALIDATE_HANDLE(hItem);
   BPNode* pRefNode = (BPNode*)hItem;

   //-- Update hItem and return pItem.
   hItem = (BHandle)pRefNode->pNext;
   if (pRefNode->pNext == NULL)
      return NULL;
   return pRefNode->pNext->pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::getPrevWithWrap(BHandle& hItem) const
{
   // Try to get prev normally.
   T* pVal=getPrev(hItem);
   if(hItem)
      return(pVal);

   // Ok, get the tail of the list instead.
   return(getTail(hItem));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BPointerList<T>::getNextWithWrap(BHandle& hItem) const
{
   // Try to get prev normally.
   T* pVal=getNext(hItem);
   if(hItem)
      return(pVal);

   // Ok, get the head of the list instead.
   return(getHead(hItem));
}


//============================================================================
//  SEARCHING
//============================================================================
template <class T> BHandle BPointerList<T>::findPointerForward(const T* pItem, BHandle hStart) const
{
   //-- Get the node we are starting from.
   BPNode* pNode;
   if (hStart != NULL)
   {
      BPLIST_VALIDATE_HANDLE(hStart);
      pNode = (BPNode*)hStart;
   }
   else
   {
      pNode = mpUsedHead;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pNode->pItem == pItem)
         return (BHandle)pNode;
      pNode = pNode->pNext;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::findPointerBackward(const T* pItem, BHandle hStart) const
{
   //-- Get the node we are starting from.
   BPNode* pNode;
   if (hStart != NULL)
   {
      BPLIST_VALIDATE_HANDLE(hStart);
      pNode = (BPNode*)hStart;
   }
   else
   {
      pNode = mpUsedTail;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pNode->pItem == pItem)
         return (BHandle)pNode;
      pNode = pNode->pPrev;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::findItemForward(const T* pItem, BHandle hStart) const
{
   //-- Get the node we are starting from.
   BPNode* pNode;
   if (hStart != NULL)
   {
      BPLIST_VALIDATE_HANDLE(hStart);
      pNode = (BPNode*)hStart;
   }
   else
   {
      pNode = mpUsedHead;
   }

   //-- Search the list.
   while (pNode)
   {
      if (*pNode->pItem == *pItem)
         return (BHandle)pNode;
      pNode = pNode->pNext;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::findItemBackward(const T* pItem, BHandle hStart) const
{
   //-- Get the node we are starting from.
   BPNode* pNode;
   if (hStart != NULL)
   {
      BPLIST_VALIDATE_HANDLE(hStart);
      pNode = (BPNode*)hStart;
   }
   else
   {
      pNode = mpUsedTail;
   }

   //-- Search the list.
   while (pNode)
   {
      if (*pNode->pItem == *pItem)
         return (BHandle)pNode;
      pNode = pNode->pPrev;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::findItemForward(COMPARE_FUNC* pFunc, void* pParam, const T* pItem, BHandle hStart) const
{
   BFATAL_ASSERT(pFunc);

   //-- Get the node we are starting from.
   BPNode* pNode;
   if (hStart != NULL)
   {
      BPLIST_VALIDATE_HANDLE(hStart);
      pNode = (BPNode*)hStart;
   }
   else
   {
      pNode = mpUsedHead;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pFunc(*pNode->pItem, *pItem, pParam) == 0)
         return (BHandle)pNode;
      pNode = pNode->pNext;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BPointerList<T>::findItemBackward(COMPARE_FUNC* pFunc, void* pParam, const T* pItem, BHandle hStart) const
{
   BFATAL_ASSERT(pFunc);

   //-- Get the node we are starting from.
   BPNode* pNode;
   if (hStart != NULL)
   {
      BPLIST_VALIDATE_HANDLE(hStart);
      pNode = (BPNode*)hStart;
   }
   else
   {
      pNode = mpUsedTail;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pFunc(*pNode->pItem, *pItem, pParam) == 0)
         return (BHandle)pNode;
      pNode = pNode->pPrev;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//  quickSort()
//  Sorts a list really fast.  You should use insertionSort() instead if:
//  1) you know the list is mostly sorted already.
//  2) you have external handles from this list laying around that you need
//     to preserve.  quickSort() causes old handles to become invalid.
//  You must have the < and > operators defined in class T.  
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::quickSort(bool ascending)
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
template <class T> void BPointerList<T>::insertionSort(bool ascending)
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
template <class T> void BPointerList<T>::quickSort(COMPARE_FUNC* pFunc, void* pParam, bool ascending)
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
template <class T> void BPointerList<T>::insertionSort(COMPARE_FUNC* pFunc, void* pParam, bool ascending)
{
   insertionSort(ascending, mSize, mpUsedHead, mpUsedTail, pFunc, pParam);
}


//============================================================================
//  HELPER FUNCTIONS
//============================================================================
template <class T> bool BPointerList<T>::isValidHandle(BHandle hItem) const
{
   //-- Scan the used list, looking for this node.
   BPNode* pNode = mpUsedHead;
   while (pNode)
   {
      if (pNode == (BPNode*)hItem)
         return true;
      pNode = pNode->pNext;
   }
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::addToFreeHead(BPNode* pNode)
{
   BFATAL_ASSERT(pNode);

   pNode->pItem = NULL;
   pNode->pPrev = NULL;
   pNode->pNext = mpFreeHead;
   if (mpFreeHead)
      mpFreeHead->pPrev = pNode;
   mpFreeHead = pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::addBlock(unsigned long numNodes)
{
   BFATAL_ASSERT(numNodes > 0);
   BFATAL_ASSERT(numNodes != BPLIST_GROW_EXPONENTIAL);

   //-- Allocate the block.
   BPBlock* pBlock = ALIGNED_NEW(BPBlock, *mpHeap);
   if (!pBlock)
   {
      BFATAL_ASSERT(0);
      return;
   }
// -e671
   pBlock->pNodes = ALIGNED_NEW_ARRAY(BPNode, numNodes, *mpHeap);
// +e671

   //-- Add it to the block list.
   pBlock->pNextBlock = mpBlocks;
   mpBlocks = pBlock;

   //-- Add all the nodes to the free list.
   pBlock->numNodes = numNodes;
   for (unsigned long node = numNodes; node > 0; --node)
      addToFreeHead(&pBlock->pNodes[node - 1]);

   //-- Adjust the allocated size.
   mAllocatedSize += numNodes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> typename BPointerList<T>::BPNode* BPointerList<T>::getFreeNode()
{
   //-- Make sure we have a free node.
   if (mpFreeHead == NULL) 
   {
      //-- Grow the list.
      BFATAL_ASSERT(mGrowSize != 0);
      BFATAL_ASSERT(mGrowSize != BPLIST_GROW_EXPONENTIAL);
      addBlock(mGrowSize);
      if (mGrowExponential)
         mGrowSize *= 2;
      BFATAL_ASSERT(mpFreeHead);
   }

   //-- Pop it.
   BPNode* pNode = mpFreeHead;
   if (!pNode)
   {
      BFATAL_ASSERT(0);
      return 0;
   }
   mpFreeHead = pNode->pNext;
   if (mpFreeHead)
      mpFreeHead->pPrev = NULL;

   //-- Return it.
   pNode->pNext = NULL;
   
   //-- These fire all the time from the unicode string manager.  And I have
   //-- no idea why.  All I can think is that there is some funky threading
   //-- issue.
//   BFATAL_ASSERT(pNode->pItem == NULL);
//   BFATAL_ASSERT(pNode->pPrev == NULL);
   return pNode;
}

//----------------------------------------------------------------------------
//  linkToHead()
//  Note that this only links the node in.  It does not update the list size
//  or do any other housekeeping.
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::linkToHead(BPNode* pNode)
{
   BFATAL_ASSERT(pNode);

   //-- Special case for empty list.
   if (mpUsedHead == NULL)
   {
      BFATAL_ASSERT(mpUsedTail == NULL);
      BFATAL_ASSERT(mSize      == 0);
      mpUsedHead   = pNode;
      mpUsedTail   = pNode;
      pNode->pPrev = NULL;
      pNode->pNext = NULL;
      return;
   }

   //-- General case.
   pNode->pPrev      = NULL;
   pNode->pNext      = mpUsedHead;
   mpUsedHead->pPrev = pNode;
   mpUsedHead        = pNode;
}

//----------------------------------------------------------------------------
//  linkToTail()
//  Note that this only links the node in.  It does not update the list size
//  or do any other housekeeping.
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::linkToTail(BPNode* pNode)
{
   BFATAL_ASSERT(pNode);

   //-- Special case for empty list.
   if (mpUsedTail == NULL)
   {
      BFATAL_ASSERT(mpUsedHead == NULL);
      BFATAL_ASSERT(mSize      == 0);
      mpUsedHead   = pNode;
      mpUsedTail   = pNode;
      pNode->pPrev = NULL;
      pNode->pNext = NULL;
      return;
   }

   //-- General case.
   pNode->pPrev      = mpUsedTail;
   pNode->pNext      = NULL;
   mpUsedTail->pNext = pNode;
   mpUsedTail        = pNode;
}

//----------------------------------------------------------------------------
//  unlink()
//  Note that this simply unlinks the node from the list.  It does not update
//  the list size or do any other housekeeping.
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::unlink(BPNode* pNode)
{
   BFATAL_ASSERT(pNode);

   if (pNode->pPrev == NULL)
   {
      BFATAL_ASSERT(mpUsedHead == pNode);
      mpUsedHead = pNode->pNext;
   }
   else
   {
      pNode->pPrev->pNext = pNode->pNext;
   }

   if (pNode->pNext == NULL)
   {
      BFATAL_ASSERT(mpUsedTail == pNode);
      mpUsedTail = pNode->pPrev;
   }
   else
   {
      pNode->pNext->pPrev = pNode->pPrev;
   }
}


//============================================================================
//  SORTING FUNCTIONS
//============================================================================
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
template <class T> typename BPointerList<T>::BPNode* BPointerList<T>::getPivot(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast)
{
   BFATAL_ASSERT(numNodes > 3);
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);

   //-- Determine the pivot.  Oh my god, getting the third value from the
   //-- center of the list (as opposed to a neighbor of pFirst or pLast)
   //-- appears to be slow and horrible. But doing this little bit of work
   //-- here REALLY saves us big if the list is mostly sorted.  As for
   //-- normal cases, it costs a little but not that much.  It still blows
   //-- the doors off of qsort().
   BPNode* pPivot   = pFirst;
   unsigned long  numSteps = numNodes / 2;
   for (unsigned long step = 0; step < numSteps; ++step)
      pPivot = pPivot->pNext;

   //-- Sort the 3 nodes.
   if (ascending)
   {
      if (*pFirst->pItem > *pPivot->pItem) BPLIST_NODE_SWAP(pFirst, pPivot)
      if (*pFirst->pItem > *pLast ->pItem) BPLIST_NODE_SWAP(pFirst, pLast )
      if (*pPivot->pItem > *pLast ->pItem) BPLIST_NODE_SWAP(pPivot, pLast )
   }
   else
   {
      if (*pFirst->pItem < *pPivot->pItem) BPLIST_NODE_SWAP(pFirst, pPivot)
      if (*pFirst->pItem < *pLast ->pItem) BPLIST_NODE_SWAP(pFirst, pLast )
      if (*pPivot->pItem < *pLast ->pItem) BPLIST_NODE_SWAP(pPivot, pLast )
   }

   //-- Move the pivot node out of the way, cuz we don't need to sort it.
   //-- We'll move it back after we are done partitioning.
   BPLIST_NODE_SWAP(pPivot, pLast->pPrev)
   pPivot = pLast->pPrev;
   return pPivot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::quickSort(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast)
{
   //-- See if its time to hand over to insertion sort.
   if (numNodes <= BPLIST_QSORT_CUTOFF)
   {
      insertionSort(ascending, numNodes, pFirst, pLast);
      return;
   }

   //-- Determine the pivot node.
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BPNode* pPivot    = getPivot(ascending, numNodes, pFirst, pLast);
   T*      pPivotVal = pPivot->pItem;

   //-- Partition the list.
   unsigned long  left   = 0;
   unsigned long  right  = numNodes - 2;
   BPNode* pLeft  = pFirst;
   BPNode* pRight = pLast->pPrev;

   if (ascending)
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev;

         while (*pLeft->pItem  < *pPivotVal) { ++left;  pLeft  = pLeft->pNext;  }
         while (*pRight->pItem > *pPivotVal) { --right; pRight = pRight->pPrev; }

         if (left < right)
            BPLIST_NODE_SWAP(pLeft, pRight)
         else
            break;
      }
   }
   else
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev; 

         while (*pLeft->pItem  > *pPivotVal) { ++left;  pLeft  = pLeft->pNext;  }
         while (*pRight->pItem < *pPivotVal) { --right; pRight = pRight->pPrev; }

         if (left < right)
            BPLIST_NODE_SWAP(pLeft, pRight)
         else
            break;
      }
   }

   //-- Swap the pivot node back to its proper spot.
   BPLIST_NODE_SWAP(pLeft, pPivot)

   //-- Recursively partition our partitions.
   unsigned long numLeftNodes  = left;
   unsigned long numRightNodes = numNodes - left - 1;
   quickSort(ascending, numLeftNodes,  pFirst,       pLeft->pPrev);
   quickSort(ascending, numRightNodes, pLeft->pNext, pLast);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::insertionSort(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast)
{
   //-- Gotta have enough nodes to sort.
   if (numNodes < 2)
      return;

   //-- Set up the start and end nodes.
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BPNode* pLeftTerminator  = pFirst->pPrev;
   BPNode* pRightTerminator = pLast->pNext;
   BPNode* pNode            = pFirst->pNext;
   if (ascending)
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->pPrev are not NULL, so we
         //-- don't need to check.
         BPNode* pNextNode = pNode->pNext;
         if (*pNode->pItem < *pNode->pPrev->pItem)
         {
            //-- Remove the node.
            if (pNode->pNext == NULL)
            {
               BFATAL_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->pPrev;
            }
            else
            {
               pNode->pNext->pPrev = pNode->pPrev;
            }
            pNode->pPrev->pNext = pNode->pNext;

            //-- Find the first node that is <= to pNode.
            BPNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (!(*pRefNode->pItem > *pNode->pItem))
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->pNext is not NULL.
                  pNode->pNext           = pRefNode->pNext;
                  pNode->pPrev           = pRefNode;
                  pRefNode->pNext->pPrev = pNode;
                  pRefNode->pNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->pPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BFATAL_ASSERT(mpUsedHead == pFirst);
                  pNode ->pNext = pFirst;
                  pNode ->pPrev = NULL;
                  pFirst->pPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->pPrev is not NULL.
                  pNode ->pNext        = pFirst;
                  pNode ->pPrev        = pFirst->pPrev;
                  pFirst->pPrev->pNext = pNode;
                  pFirst->pPrev        = pNode;
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
         //-- loop we know that pNode and pNode->pPrev are not NULL, so we
         //-- don't need to check.
         BPNode* pNextNode = pNode->pNext;
         if (*pNode->pItem > *pNode->pPrev->pItem)
         {
            //-- Remove the node.
            if (pNode->pNext == NULL)
            {
               BFATAL_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->pPrev;
            }
            else
            {
               pNode->pNext->pPrev = pNode->pPrev;
            }
            pNode->pPrev->pNext = pNode->pNext;

            //-- Find the first node that is >= to pNode.
            BPNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (!(*pRefNode->pItem < *pNode->pItem))
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->pNext is not NULL.
                  pNode->pNext           = pRefNode->pNext;
                  pNode->pPrev           = pRefNode;
                  pRefNode->pNext->pPrev = pNode;
                  pRefNode->pNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->pPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BFATAL_ASSERT(mpUsedHead == pFirst);
                  pNode ->pNext = pFirst;
                  pNode ->pPrev = NULL;
                  pFirst->pPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->pPrev is not NULL.
                  pNode ->pNext        = pFirst;
                  pNode ->pPrev        = pFirst->pPrev;
                  pFirst->pPrev->pNext = pNode;
                  pFirst->pPrev        = pNode;
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
template <class T> typename BPointerList<T>::BPNode* BPointerList<T>::getPivot(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast, COMPARE_FUNC* pFunc, void* pParam)
{
   BFATAL_ASSERT(numNodes > 3);
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BFATAL_ASSERT(pFunc);

   //-- Determine the pivot.  Oh my god, getting the third value from the
   //-- center of the list (as opposed to a neighbor of pFirst or pLast)
   //-- appears to be slow and horrible. But doing this little bit of work
   //-- here REALLY saves us big if the list is mostly sorted.  As for
   //-- normal cases, it costs a little but not that much.  It still blows
   //-- the doors off of qsort().
   BPNode* pPivot   = pFirst;
   unsigned long  numSteps = numNodes / 2;
   for (unsigned long step = 0; step < numSteps; ++step)
      pPivot = pPivot->pNext;

   //-- Sort the 3 nodes.
   if (ascending)
   {
      if (pFunc(*pFirst->pItem, *pPivot->pItem, pParam) > 0) BPLIST_NODE_SWAP(pFirst, pPivot)
      if (pFunc(*pFirst->pItem, *pLast ->pItem, pParam) > 0) BPLIST_NODE_SWAP(pFirst, pLast )
      if (pFunc(*pPivot->pItem, *pLast ->pItem, pParam) > 0) BPLIST_NODE_SWAP(pPivot, pLast )
   }
   else
   {
      if (pFunc(*pFirst->pItem, *pPivot->pItem, pParam) < 0) BPLIST_NODE_SWAP(pFirst, pPivot)
      if (pFunc(*pFirst->pItem, *pLast ->pItem, pParam) < 0) BPLIST_NODE_SWAP(pFirst, pLast )
      if (pFunc(*pPivot->pItem, *pLast ->pItem, pParam) < 0) BPLIST_NODE_SWAP(pPivot, pLast )
   }

   //-- Move the pivot node out of the way, cuz we don't need to sort it.
   //-- We'll move it back after we are done partitioning.
   BPLIST_NODE_SWAP(pPivot, pLast->pPrev)
   pPivot = pLast->pPrev;
   return pPivot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::quickSort(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast, COMPARE_FUNC* pFunc, void* pParam)
{
   BFATAL_ASSERT(pFunc);

   //-- See if its time to hand over to insertion sort.
   if (numNodes <= BPLIST_QSORT_CUTOFF)
   {
      insertionSort(ascending, numNodes, pFirst, pLast, pFunc, pParam);
      return;
   }

   //-- Determine the pivot node.
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BPNode* pPivot    = getPivot(ascending, numNodes, pFirst, pLast, pFunc, pParam);
   T*      pPivotVal = pPivot->pItem;

   //-- Partition the list.
   unsigned long  left   = 0;
   unsigned long  right  = numNodes - 2;
   BPNode* pLeft  = pFirst;
   BPNode* pRight = pLast->pPrev;

   if (ascending)
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev; 

         while (pFunc(*pLeft->pItem,  *pPivotVal, pParam) < 0) { ++left;  pLeft  = pLeft->pNext;  }
         while (pFunc(*pRight->pItem, *pPivotVal, pParam) > 0) { --right; pRight = pRight->pPrev; }

         if (left < right)
            BPLIST_NODE_SWAP(pLeft, pRight)
         else
            break;
      }
   }
   else
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev; 

         while (pFunc(*pLeft->pItem,  *pPivotVal, pParam) > 0) { ++left;  pLeft  = pLeft->pNext;  }
         while (pFunc(*pRight->pItem, *pPivotVal, pParam) < 0) { --right; pRight = pRight->pPrev; }

         if (left < right)
            BPLIST_NODE_SWAP(pLeft, pRight)
         else
            break;
      }
   }

   //-- Swap the pivot node back to its proper spot.
   BPLIST_NODE_SWAP(pLeft, pPivot)

   //-- Recursively partition our partitions.
   unsigned long numLeftNodes  = left;
   unsigned long numRightNodes = numNodes - left - 1;
   quickSort(ascending, numLeftNodes,  pFirst,       pLeft->pPrev, pFunc, pParam);
   quickSort(ascending, numRightNodes, pLeft->pNext, pLast,        pFunc, pParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BPointerList<T>::insertionSort(bool ascending, unsigned long numNodes, BPNode* pFirst, BPNode* pLast, COMPARE_FUNC* pFunc, void* pParam)
{
   BFATAL_ASSERT(pFunc);

   //-- Gotta have enough nodes to sort.
   if (numNodes < 2)
      return;

   //-- Set up the start and end nodes.
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BPNode* pLeftTerminator  = pFirst->pPrev;
   BPNode* pRightTerminator = pLast->pNext;
   BPNode* pNode            = pFirst->pNext;
   if (ascending)
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->pPrev are not NULL, so we
         //-- don't need to check.
         BPNode* pNextNode = pNode->pNext;
         if (pFunc(*pNode->pItem, *pNode->pPrev->pItem, pParam) < 0)
         {
            //-- Remove the node.
            if (pNode->pNext == NULL)
            {
               BFATAL_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->pPrev;
            }
            else
            {
               pNode->pNext->pPrev = pNode->pPrev;
            }
            pNode->pPrev->pNext = pNode->pNext;

            //-- Find the first node that is <= to pNode.
            BPNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (pFunc(*pRefNode->pItem, *pNode->pItem, pParam) <= 0)
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->pNext is not NULL.
                  pNode->pNext           = pRefNode->pNext;
                  pNode->pPrev           = pRefNode;
                  pRefNode->pNext->pPrev = pNode;
                  pRefNode->pNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->pPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BFATAL_ASSERT(mpUsedHead == pFirst);
                  pNode ->pNext = pFirst;
                  pNode ->pPrev = NULL;
                  pFirst->pPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->pPrev is not NULL.
                  pNode ->pNext        = pFirst;
                  pNode ->pPrev        = pFirst->pPrev;
                  pFirst->pPrev->pNext = pNode;
                  pFirst->pPrev        = pNode;
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
         //-- loop we know that pNode and pNode->pPrev are not NULL, so we
         //-- don't need to check.
         BPNode* pNextNode = pNode->pNext;
         if (pFunc(*pNode->pItem, *pNode->pPrev->pItem, pParam) > 0)
         {
            //-- Remove the node.
            if (pNode->pNext == NULL)
            {
               BFATAL_ASSERT(mpUsedTail == pNode);
               mpUsedTail = pNode->pPrev;
            }
            else
            {
               pNode->pNext->pPrev = pNode->pPrev;
            }
            pNode->pPrev->pNext = pNode->pNext;

            //-- Find the first node that is >= to pNode.
            BPNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (pFunc(*pRefNode->pItem, *pNode->pItem, pParam) >= 0)
               {  
                  //-- Insert pNode after pRefNode.  Note that we know that
                  //-- pRefNode->pNext is not NULL.
                  pNode->pNext           = pRefNode->pNext;
                  pNode->pPrev           = pRefNode;
                  pRefNode->pNext->pPrev = pNode;
                  pRefNode->pNext        = pNode;
                  break;
               }
               pRefNode = pRefNode->pPrev;
            }

            //-- See if we have to add it the end of the segment.
            if (pRefNode == pLeftTerminator)
            {
               if (pLeftTerminator == NULL)
               {
                  //-- Handle the head case.
                  BFATAL_ASSERT(mpUsedHead == pFirst);
                  pNode ->pNext = pFirst;
                  pNode ->pPrev = NULL;
                  pFirst->pPrev = pNode;
                  mpUsedHead    = pNode;
               }
               else
               {
                  //-- Insert pNode before pFirst.  Note that we know that
                  //-- pFirst->pPrev is not NULL.
                  pNode ->pNext        = pFirst;
                  pNode ->pPrev        = pFirst->pPrev;
                  pFirst->pPrev->pNext = pNode;
                  pFirst->pPrev        = pNode;
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
template <class T> BPointerList<T>& BPointerList<T>::operator = (const BPointerList<T>& list)
{
   if (this == &list)
   {
      BFATAL_ASSERT(0);
      return *this;
   }
   //-- Reset this list with the other lists size.
   reset(list.mSize, 0);

   //-- Add all the elements from the other list.
   BHandle hNode;
   T*      pNode = list.getHead(hNode);
   while (pNode)
   {
      addToTail(pNode);
      pNode = list.getNext(hNode);
   }

   //-- Copy the other list's grow size.
   mGrowSize        = list.mGrowSize;
   mGrowExponential = list.mGrowExponential;

   return *this;
}


//============================================================================
//  UNDEFINE PRIVATE MACROS
//============================================================================
#undef BPLIST_NODE_SWAP
#undef BPLIST_VALIDATE_HANDLE


