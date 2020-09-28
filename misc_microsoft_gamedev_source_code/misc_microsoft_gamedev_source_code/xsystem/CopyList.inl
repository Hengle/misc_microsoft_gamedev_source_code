//============================================================================
//
//  CopyList.inl
//  
//  Copyright (c) 1999-2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
const unsigned long BCLIST_QSORT_CUTOFF = 32;


//============================================================================
//  PRIVATE MACROS
//============================================================================
#if DEBUG_VALIDATE_LIST_HANDLES
   #define BCLIST_VALIDATE_HANDLE(handle) BFATAL_ASSERT(isValidHandle(handle))
#else
   #define BCLIST_VALIDATE_HANDLE(handle)
#endif


//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
template <class T> BCopyList<T>::BCopyList(unsigned long initialSize, unsigned long growSize) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL)
{

   reset(initialSize, growSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BCopyList<T>::BCopyList(const BCopyList<T>& list) :
   mSize            (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mGrowExponential (false),
   mpBlocks         (NULL),
   mpFreeHead       (NULL),
   mpUsedHead       (NULL),
   mpUsedTail       (NULL)
{
   *this = list;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BCopyList<T>::~BCopyList()
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
//  BCLIST_GROW_EXPONENTIAL the amount the list grows each time will double.
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::reset(unsigned long initialSize, unsigned long growSize)
{
   //-- Release the old memory.
   BCBlock* pBlock = mpBlocks;
   while (pBlock)
   {
      BCBlock* pKill = pBlock;
      pBlock = pBlock->pNextBlock;
      delete [] pKill->pNodes;
      delete pKill;
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
   mGrowExponential = (mGrowSize == BCLIST_GROW_EXPONENTIAL);
   
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
template <class T> void BCopyList<T>::empty()
{
   //-- Give all the nodes back to the free list.
   BCNode* pNode = mpUsedTail;
   while (pNode)
   {
      BCNode* pKill = pNode;
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
template <class T> void BCopyList<T>::optimize()
{
   //-- Allocate the block.
   BCBlock* pNewBlock = new BCBlock;
   pNewBlock->numNodes   = mSize;
   pNewBlock->pNextBlock = NULL;
   pNewBlock->pNodes     = new BCNode [mSize];

   //-- Fill the list.
   unsigned long  index = 0;
   BCNode* pNode = mpUsedHead;
   while (pNode)
   {
      pNewBlock->pNodes[index].item = pNode->item;
      pNewBlock->pNodes[index].pPrev = (index == 0)         ? NULL : &pNewBlock->pNodes[index - 1];
      pNewBlock->pNodes[index].pNext = (index == mSize - 1) ? NULL : &pNewBlock->pNodes[index + 1];
      index++;

      pNode = pNode->pNext;
   }

   //-- Release the old memory.
   BCBlock* pBlock = mpBlocks;
   while (pBlock)
   {
      BCBlock* pKill = pBlock;
      pBlock = pBlock->pNextBlock;
      delete [] pKill->pNodes;
      delete pKill;
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
//  grow.  If growSize == BCLIST_GROW_EXPONENTIAL, the number of nodes that it
//  grows by will be set to the list's current allocated size and double each
//  time the list grows.
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::setGrowSize(unsigned long growSize)
{
   mGrowSize        = growSize;
   mGrowExponential = (mGrowSize == BCLIST_GROW_EXPONENTIAL);
   
   if (mGrowExponential)
      mGrowSize = (mAllocatedSize == 0) ? 1 : mAllocatedSize;
}


//============================================================================
//  LIST INFO
//============================================================================
template <class T> bool BCopyList<T>::isEmpty() const
{
   return (mSize == 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> bool BCopyList<T>::isGrowExponential() const
{
   return mGrowExponential;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BCopyList<T>::getSize() const
{
   return mSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BCopyList<T>::getGrowSize() const
{
   return mGrowSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> unsigned long BCopyList<T>::getAllocatedSize() const
{
   return mAllocatedSize;
}


//============================================================================
//  ADDING ITEMS
//============================================================================
template <class T> BHandle BCopyList<T>::addToHead(const T& item)
{
   //-- Get a node off the free list.
   BCNode* pNode = getFreeNode();
   pNode->item = item;

   //-- Add it.
   linkToHead(pNode);

   //-- Increase size.
   mSize++;

   //-- Give it back.
   return (BHandle)pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BCopyList<T>::addToTail(const T& item)
{
   //-- Get a node off the free list.
   BCNode* pNode = getFreeNode();
   if (!pNode)
   {
      BFATAL_ASSERT(0);
      return 0;
   }
   pNode->item = item;

   //-- Add it.
   linkToTail(pNode);

   //-- Increase.
   mSize++;

   //-- Give it back.
   return (BHandle)pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BCopyList<T>::addBefore(const T& item, BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

   //-- Get a node off the free list.
   BCNode* pNode = getFreeNode();
   pNode->item = item;

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
template <class T> BHandle BCopyList<T>::addAfter(const T& item, BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

   //-- Get a node off the free list.
   BCNode* pNode = getFreeNode();
   pNode->item = item;

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
template <class T> BHandle BCopyList<T>::addSorted(const T& item, bool ascending)
{
   //-- Get a node off the free list.
   BCNode* pNode = getFreeNode();
   pNode->item = item;

   //-- Handle an empty list.
   if (mSize == 0)
   {
      mpUsedHead = pNode;
      mpUsedTail = pNode;
      mSize      = 1;
      return (BHandle)pNode;
   }

   //-- Find the spot in the list to put it.
   BCNode* pRefNode = mpUsedHead;
   if (ascending)
   {
      //-- Find the first node that is greater than or equal to item.
      while (pRefNode && (pRefNode->item < item))
         pRefNode = pRefNode->pNext;
   }
   else
   {
      //-- Find the first node that is less than or equal to item.
      while (pRefNode && (pRefNode->item > item))
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
template <class T> BHandle BCopyList<T>::addSorted(const T& item, COMPARE_FUNC* pFunc, void* pParam, bool ascending)
{
   BFATAL_ASSERT(pFunc);

   //-- Get a node off the free list.
   BCNode* pNode = getFreeNode();
   pNode->item = item;

   //-- Handle an empty list.
   if (mSize == 0)
   {
      mpUsedHead = pNode;
      mpUsedTail = pNode;
      mSize      = 1;
      return (BHandle)pNode;
   }

   //-- Find the spot in the list to put it.
   BCNode* pRefNode = mpUsedHead;
   if (ascending)
   {
      //-- Find the first node that is greater than or equal to item.
      while (pRefNode && (pFunc(pRefNode->item, item, pParam) < 0))
         pRefNode = pRefNode->pNext;
   }
   else
   {
      //-- Find the first node that is less than or equal to item.
      while (pRefNode && (pFunc(pRefNode->item, item, pParam) > 0))
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
template <class T> void BCopyList<T>::removeHead()
{
   //-- Be nice and don't BFATAL_ASSERT.  So it can be used in a loop like:
   //-- while (p = list.removeHead()) ...
   if (mpUsedHead == NULL)
      return;

   //-- Remove it from the list.
   BCNode* pOldNode = mpUsedHead;
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
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::removeTail()
{
   //-- Be nice and don't BFATAL_ASSERT.  So it can be used in a loop like:
   //-- while (p = list.removeTail()) ...
   if (mpUsedTail == NULL)
      return;

   //-- Remove it from the list.
   BCNode* pOldNode = mpUsedTail;
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
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::remove(BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

   //-- Unlink it.
   unlink(pRefNode);

   //-- Add it to the free head.
   addToFreeHead(pRefNode);
   mSize--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::removeAndGetPrev(BHandle& hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

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
   T* pItem = 0;
   if (pRefNode->pPrev)
   {
      pItem = &pRefNode->pPrev->item;
      hItem = (BHandle)pRefNode->pPrev;
   }
   else
   {
      item = NULL;
      hItem = NULL;
   }

   //-- Add it to the free head.
   addToFreeHead(pRefNode);
   mSize--;
   return pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::removeAndGetNext(BHandle& hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

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
      pItem = &pRefNode->pNext->item;
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
template <class T> void BCopyList<T>::moveToHead(BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

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
template <class T> void BCopyList<T>::moveToTail(BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

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
template <class T> T* BCopyList<T>::getItem(BHandle hItem)
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;
   return &pRefNode->item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::getItem(unsigned long index) const
{
   if (index >= mSize)
      return NULL;

   BCNode* pNode = mpUsedHead;

   for (unsigned long item = 0; item < index; ++item)
      pNode = pNode->pNext;

   return &pNode->item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::getHead(BHandle& hItem) const
{
   if (mpUsedHead == NULL)
   {
      hItem = NULL;
      return NULL;
   }

   hItem = (BHandle)mpUsedHead;
   return &mpUsedHead->item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::getTail(BHandle& hItem) const
{
   if (mpUsedTail == NULL)
   {
      hItem = NULL;
      return NULL;
   }

   hItem = (BHandle)mpUsedTail;
   return &mpUsedTail->item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::getPrev(BHandle& hItem) const
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

   //-- Update hItem and return item.
   hItem = (BHandle)pRefNode->pPrev;
   if (pRefNode->pPrev == NULL)
      return NULL;
   return &pRefNode->pPrev->item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::getNext(BHandle& hItem) const
{
   //-- Get the node.
   BFATAL_ASSERT(hItem);
   BCLIST_VALIDATE_HANDLE(hItem);
   BCNode* pRefNode = (BCNode*)hItem;

   //-- Update hItem and return item.
   hItem = (BHandle)pRefNode->pNext;
   if (pRefNode->pNext == NULL)
      return NULL;
   return &pRefNode->pNext->item;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> T* BCopyList<T>::getPrevWithWrap(BHandle& hItem) const
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
template <class T> T* BCopyList<T>::getNextWithWrap(BHandle& hItem) const
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
template <class T> BHandle BCopyList<T>::findItemForward(const T& item, BHandle hStart) const
{
   //-- Get the node we are starting from.
   BCNode* pNode;
   if (hStart != NULL)
   {
      BCLIST_VALIDATE_HANDLE(hStart);
      pNode = (BCNode*)hStart;
   }
   else
   {
      pNode = mpUsedHead;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pNode->item == item)
         return (BHandle)pNode;
      pNode = pNode->pNext;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BCopyList<T>::findItemBackward(const T& item, BHandle hStart) const
{
   //-- Get the node we are starting from.
   BCNode* pNode;
   if (hStart != NULL)
   {
      BCLIST_VALIDATE_HANDLE(hStart);
      pNode = (BCNode*)hStart;
   }
   else
   {
      pNode = mpUsedTail;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pNode->item == item)
         return (BHandle)pNode;
      pNode = pNode->pPrev;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BCopyList<T>::findItemForward(COMPARE_FUNC* pFunc, void* pParam, const T& item, BHandle hStart) const
{
   BFATAL_ASSERT(pFunc);

   //-- Get the node we are starting from.
   BCNode* pNode;
   if (hStart != NULL)
   {
      BCLIST_VALIDATE_HANDLE(hStart);
      pNode = (BCNode*)hStart;
   }
   else
   {
      pNode = mpUsedHead;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pFunc(pNode->item, item, pParam) == 0)
         return (BHandle)pNode;
      pNode = pNode->pNext;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> BHandle BCopyList<T>::findItemBackward(COMPARE_FUNC* pFunc, void* pParam, const T& item, BHandle hStart) const
{
   BFATAL_ASSERT(pFunc);

   //-- Get the node we are starting from.
   BCNode* pNode;
   if (hStart != NULL)
   {
      BCLIST_VALIDATE_HANDLE(hStart);
      pNode = (BCNode*)hStart;
   }
   else
   {
      pNode = mpUsedTail;
   }

   //-- Search the list.
   while (pNode)
   {
      if (pFunc(pNode->item, item, pParam) == 0)
         return (BHandle)pNode;
      pNode = pNode->pPrev;
   }
   return NULL;
}

//----------------------------------------------------------------------------
//  quickSort()
//  Sorts a list really fast.  Unlike BPointerList::quickSort(),
//  BCopyList::quickSort() does not cause existing list handles to become
//  invalid.  You should use insertionSort() instead if you know the list is
//  mostly sorted already. You must have the < and > operators defined in
//  class T.  
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::quickSort(bool ascending)
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
template <class T> void BCopyList<T>::insertionSort(bool ascending)
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
template <class T> void BCopyList<T>::quickSort(COMPARE_FUNC* pFunc, void* pParam, bool ascending)
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
template <class T> void BCopyList<T>::insertionSort(COMPARE_FUNC* pFunc, void* pParam, bool ascending)
{
   insertionSort(ascending, mSize, mpUsedHead, mpUsedTail, pFunc, pParam);
}


//============================================================================
//  HELPER FUNCTIONS
//============================================================================
template <class T> bool BCopyList<T>::isValidHandle(BHandle hItem) const
{
   //-- Scan the used list, looking for this node.
   BCNode* pNode = mpUsedHead;
   while (pNode)
   {
      if (pNode == (BCNode*)hItem)
         return true;
      pNode = pNode->pNext;
   }
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::addToFreeHead(BCNode* pNode)
{
   BFATAL_ASSERT(pNode);

   pNode->pPrev = NULL;
   pNode->pNext = mpFreeHead;
   if (mpFreeHead)
      mpFreeHead->pPrev = pNode;
   mpFreeHead = pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::addBlock(unsigned long numNodes)
{
   BFATAL_ASSERT(numNodes > 0);
   BFATAL_ASSERT(numNodes != BCLIST_GROW_EXPONENTIAL);

   //-- Allocate the block.
   BCBlock* pBlock = new BCBlock;
   if (!pBlock)
   {
      BFATAL_ASSERT(0);
      return;
   }
// -e671
   pBlock->pNodes = new BCNode [numNodes];
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
template <class T> typename BCopyList<T>::BCNode* BCopyList<T>::getFreeNode()
{
   //-- Make sure we have a free node.
   if (mpFreeHead == NULL) 
   {
      //-- Grow the list.
      BFATAL_ASSERT(mGrowSize != 0);
      BFATAL_ASSERT(mGrowSize != BCLIST_GROW_EXPONENTIAL);
      addBlock(mGrowSize);
      if (mGrowExponential)
         mGrowSize *= 2;
      BFATAL_ASSERT(mpFreeHead);
   }

   //-- Pop it.
   BCNode* pNode = mpFreeHead;
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
   BFATAL_ASSERT(pNode->pPrev == NULL);
   return pNode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::swapNodes(BCNode*& pNode1, BCNode*& pNode2)
{
   BFATAL_ASSERT(pNode1);
   BFATAL_ASSERT(pNode2);
   BFATAL_ASSERT(mpUsedHead);
   BFATAL_ASSERT(mpUsedTail);

   if (pNode1 == pNode2)
      return;

   //-- Store the node to insert pNode1 before.
   BCNode* pRefNode;

   //-- Put pNode2 right before pNode1.
   if (pNode2->pNext != pNode1)
   {
      pRefNode = pNode2->pNext;

      //-- Remove pNode2 from the list.
      if (pNode2->pPrev == NULL)
      {
         BFATAL_ASSERT(mpUsedHead == pNode2);
         mpUsedHead = pNode2->pNext;
      }
      else
      {
         pNode2->pPrev->pNext = pNode2->pNext;
      }
      if (pNode2->pNext == NULL)
      {
         BFATAL_ASSERT(mpUsedTail == pNode2);
         mpUsedTail = pNode2->pPrev;
      }
      else
      {
         pNode2->pNext->pPrev = pNode2->pPrev;
      }
   
      //-- Insert pNode2 before pNode1.
      pNode2->pNext = pNode1;
      pNode2->pPrev = pNode1->pPrev;
      if (pNode1->pPrev)
      {
         pNode1->pPrev->pNext = pNode2;
      }
      else
      {
         BFATAL_ASSERT(mpUsedHead == pNode1);
         mpUsedHead = pNode2;
      }
      pNode1->pPrev = pNode2;
   }
   else
   {
      pRefNode = pNode2;
   }

   //-- Put pNode1 right before pRefNode.
   if (pNode1->pNext != pRefNode)
   {
      //-- Remove pNode1 from the list.
      if (pNode1->pPrev == NULL)
      {
         BFATAL_ASSERT(mpUsedHead == pNode1);
         mpUsedHead = pNode1->pNext;
      }
      else
      {
         pNode1->pPrev->pNext = pNode1->pNext;
      }
      if (pNode1->pNext == NULL)
      {
         BFATAL_ASSERT(mpUsedTail == pNode1);
         mpUsedTail = pNode1->pPrev;
      }
      else
      {
         pNode1->pNext->pPrev = pNode1->pPrev;
      }

      //-- Insert pNode1 back in the list.
      if (pRefNode == NULL)
      {
         //-- Add it to the tail.
         pNode1->pPrev     = mpUsedTail;
         pNode1->pNext     = NULL;
         mpUsedTail->pNext = pNode1;
         mpUsedTail        = pNode1;
      }
      else
      {
         //-- Insert pNode1 before pRefNode.
         pNode1->pNext = pRefNode;
         pNode1->pPrev = pRefNode->pPrev;
         if (pRefNode->pPrev)
         {
            pRefNode->pPrev->pNext = pNode1;
         }
         else
         {
            BFATAL_ASSERT(mpUsedHead == pRefNode);
            mpUsedHead = pNode1;
         }
         pRefNode->pPrev = pNode1;
      }
   }

   //-- Swap the node pointers.
   BCNode* pTemp = pNode1;
   pNode1 = pNode2;
   pNode2 = pTemp;
/*
   //-- Validate list integrity.
   unsigned long count = 0;
   BCNode* p = mpUsedHead;
   while (p)
   {
      if (p->pNext) BFATAL_ASSERT(p->pNext->pPrev == p);
      if (p->pPrev) BFATAL_ASSERT(p->pPrev->pNext == p);
      count++;
      p = p->pNext;
   }
   BFATAL_ASSERT(count == mSize);
*/
//   T temp = pNode1->item;
//   pNode1->item = pNode2->item;
//   pNode2->item = temp;
}

//----------------------------------------------------------------------------
//  linkToHead()
//  Note that this only links the node in.  It does not update the list size
//  or do any other housekeeping.
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::linkToHead(BCNode* pNode)
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
template <class T> void BCopyList<T>::linkToTail(BCNode* pNode)
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
template <class T> void BCopyList<T>::unlink(BCNode* pNode)
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
template <class T> typename BCopyList<T>::BCNode* BCopyList<T>::getPivot(bool ascending, unsigned long numNodes, BCNode*& pFirst, BCNode*& pLast)
{
   BFATAL_ASSERT(numNodes > 3);
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);

   if (!pFirst || !pLast)
      return 0; 

   //-- Determine the pivot.  Oh my god, getting the third value from the
   //-- center of the list (as opposed to a neighbor of pFirst or pLast)
   //-- appears to be slow and horrible. But doing this little bit of work
   //-- here REALLY saves us big if the list is mostly sorted.  As for
   //-- normal cases, it costs a little but not that much.  It still blows
   //-- the doors off of qsort().
   BCNode* pPivot   = pFirst;
   unsigned long  numSteps = numNodes / 2;
   for (unsigned long step = 0; step < numSteps; ++step)
      pPivot = pPivot->pNext;

   //-- Sort the 3 nodes.
   if (ascending)
   {
      if (pFirst->item > pPivot->item) swapNodes(pFirst, pPivot);
      if (pFirst->item > pLast ->item) swapNodes(pFirst, pLast );
      if (pPivot->item > pLast ->item) swapNodes(pPivot, pLast );
   }
   else
   {
      if (pFirst->item < pPivot->item) swapNodes(pFirst, pPivot);
      if (pFirst->item < pLast ->item) swapNodes(pFirst, pLast );
      if (pPivot->item < pLast ->item) swapNodes(pPivot, pLast );
   }

   //-- Move the pivot node out of the way, cuz we don't need to sort it.
   //-- We'll move it back after we are done partitioning.
   BCNode* pTemp = pLast->pPrev;
   swapNodes(pPivot, pTemp);
   pPivot = pTemp;
   return pPivot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::quickSort(bool ascending, unsigned long numNodes, BCNode* pFirst, BCNode* pLast)
{
   //-- See if its time to hand over to insertion sort.
   if (numNodes <= BCLIST_QSORT_CUTOFF)
   {
      insertionSort(ascending, numNodes, pFirst, pLast);
      return;
   }

   //-- Determine the pivot node.
   if (!pFirst || !pLast)
   {
      BFATAL_ASSERT(0);
      return;
   }
   BCNode* pPivot = getPivot(ascending, numNodes, pFirst, pLast);

   //-- Partition the list.
   unsigned long  left   = 0;
   unsigned long  right  = numNodes - 2;
   BCNode* pLeft  = pFirst;
   BCNode* pRight = pLast->pPrev;
   BASSERT(pRight);

   if (ascending)
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev;

         while (pLeft->item  < pPivot->item) { ++left;  pLeft  = pLeft->pNext;  }
         while (pRight->item > pPivot->item) { --right; pRight = pRight->pPrev; }

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
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev; 

         while (pLeft->item  > pPivot->item) { ++left;  pLeft  = pLeft->pNext;  }
         while (pRight->item < pPivot->item) { --right; pRight = pRight->pPrev; }

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
   unsigned long numLeftNodes  = left;
   unsigned long numRightNodes = numNodes - left - 1;
   quickSort(ascending, numLeftNodes,  pFirst,       pLeft->pPrev);
   quickSort(ascending, numRightNodes, pLeft->pNext, pLast);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::insertionSort(bool ascending, unsigned long numNodes, BCNode* pFirst, BCNode* pLast)
{
   //-- Gotta have enough nodes to sort.
   if (numNodes < 2)
      return;

   //-- Set up the start and end nodes.
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BCNode* pLeftTerminator  = pFirst->pPrev;
   BCNode* pRightTerminator = pLast->pNext;
   BCNode* pNode            = pFirst->pNext;
   if (ascending)
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->pPrev are not NULL, so we
         //-- don't need to check.
         BCNode* pNextNode = pNode->pNext;
         if (pNode->item < pNode->pPrev->item)
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
            BCNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (!(pRefNode->item > pNode->item))
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
         BCNode* pNextNode = pNode->pNext;
         if (pNode->item > pNode->pPrev->item)
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
            BCNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (!(pRefNode->item < pNode->item))
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
template <class T> typename BCopyList<T>::BCNode* BCopyList<T>::getPivot(bool ascending, unsigned long numNodes, BCNode*& pFirst, BCNode*& pLast, COMPARE_FUNC* pFunc, void* pParam)
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
   BCNode* pPivot   = pFirst;
   unsigned long  numSteps = numNodes / 2;
   for (unsigned long step = 0; step < numSteps; ++step)
      pPivot = pPivot->pNext;

   //-- Sort the 3 nodes.
   if (ascending)
   {
      if (pFunc(pFirst->item, pPivot->item, pParam) > 0) swapNodes(pFirst, pPivot);
      if (pFunc(pFirst->item, pLast ->item, pParam) > 0) swapNodes(pFirst, pLast );
      if (pFunc(pPivot->item, pLast ->item, pParam) > 0) swapNodes(pPivot, pLast );
   }
   else
   {
      if (pFunc(pFirst->item, pPivot->item, pParam) < 0) swapNodes(pFirst, pPivot);
      if (pFunc(pFirst->item, pLast ->item, pParam) < 0) swapNodes(pFirst, pLast );
      if (pFunc(pPivot->item, pLast ->item, pParam) < 0) swapNodes(pPivot, pLast );
   }

   //-- Move the pivot node out of the way, cuz we don't need to sort it.
   //-- We'll move it back after we are done partitioning.
   BCNode* pTemp = pLast->pPrev;
   swapNodes(pPivot, pTemp);
   pPivot = pTemp;
   return pPivot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::quickSort(bool ascending, unsigned long numNodes, BCNode* pFirst, BCNode* pLast, COMPARE_FUNC* pFunc, void* pParam)
{
   BFATAL_ASSERT(pFunc);

   //-- See if its time to hand over to insertion sort.
   if (numNodes <= BCLIST_QSORT_CUTOFF)
   {
      insertionSort(ascending, numNodes, pFirst, pLast, pFunc, pParam);
      return;
   }

   //-- Determine the pivot node.
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BCNode* pPivot = getPivot(ascending, numNodes, pFirst, pLast, pFunc, pParam);

   //-- Partition the list.
   unsigned long  left   = 0;
   unsigned long  right  = numNodes - 2;
   BCNode* pLeft  = pFirst;
   BCNode* pRight = pLast->pPrev;
   BASSERT(pRight);

   if (ascending)
   {
      for (;;)
      {
         ++left;
         --right;
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev; 

         while (pFunc(pLeft->item,  pPivot->item, pParam) < 0) { ++left;  pLeft  = pLeft->pNext;  }
         while (pFunc(pRight->item, pPivot->item, pParam) > 0) { --right; pRight = pRight->pPrev; }

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
         pLeft  = pLeft->pNext;
         pRight = pRight->pPrev; 

         while (pFunc(pLeft->item,  pPivot->item, pParam) > 0) { ++left;  pLeft  = pLeft->pNext;  }
         while (pFunc(pRight->item, pPivot->item, pParam) < 0) { --right; pRight = pRight->pPrev; }

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
   unsigned long numLeftNodes  = left;
   unsigned long numRightNodes = numNodes - left - 1;
   quickSort(ascending, numLeftNodes,  pFirst,       pLeft->pPrev, pFunc, pParam);
   quickSort(ascending, numRightNodes, pLeft->pNext, pLast,        pFunc, pParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <class T> void BCopyList<T>::insertionSort(bool ascending, unsigned long numNodes, BCNode* pFirst, BCNode* pLast, COMPARE_FUNC* pFunc, void* pParam)
{
   BFATAL_ASSERT(pFunc);

   //-- Gotta have enough nodes to sort.
   if (numNodes < 2)
      return;

   //-- Set up the start and end nodes.
   BFATAL_ASSERT(pFirst);
   BFATAL_ASSERT(pLast);
   BCNode* pLeftTerminator  = pFirst->pPrev;
   BCNode* pRightTerminator = pLast->pNext;
   BCNode* pNode            = pFirst->pNext;
   if (ascending)
   {
      //-- Do an ascending sort.
      while (pNode != pRightTerminator)
      {
         //-- See if we need to displace this node.  Note that inside this
         //-- loop we know that pNode and pNode->pPrev are not NULL, so we
         //-- don't need to check.
         BCNode* pNextNode = pNode->pNext;
         if (pFunc(pNode->item, pNode->pPrev->item, pParam) < 0)
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
            BCNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (pFunc(pRefNode->item, pNode->item, pParam) <= 0)
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
         BCNode* pNextNode = pNode->pNext;
         if (pFunc(pNode->item, pNode->pPrev->item, pParam) > 0)
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
            BCNode* pRefNode = pNode->pPrev->pPrev;
            while (pRefNode != pLeftTerminator)
            {
               if (pFunc(pRefNode->item, pNode->item, pParam) >= 0)
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
template <class T> BCopyList<T>& BCopyList<T>::operator = (const BCopyList<T>& list)
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
      addToTail(*pNode);
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
#undef BCLIST_VALIDATE_HANDLE




