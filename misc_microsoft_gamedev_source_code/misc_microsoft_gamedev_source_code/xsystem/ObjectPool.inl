//=============================================================================
//
//  ObjectPool.inl
//  
//  Copyright 2003, Ensemble Studios
//
//=============================================================================


//=============================================================================
//  CONSTRUCTION/DESTRUCTION
//=============================================================================
template <class T> BObjectPool<T>::BObjectPool(unsigned long initialSize, unsigned long growSize)
{
   //-- Init all members
   mSize          = 0;
   mGrowSize      = 0;
   mAllocatedSize = 0;
   mpFreeList     = NULL;
   mpUsedList     = NULL;
   mpBlocks       = NULL;

   //-- Set up initial allocation.  This won't allocate anything if
   //-- initialSize is 0.
   reset(initialSize, growSize);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> BObjectPool<T>::~BObjectPool()
{
   //-- Release memory.
   reset(0, 0);
}


//=============================================================================
//  POOL OPERATIONS
//=============================================================================
template <class T> T* BObjectPool<T>::createObject()
{
   //-- See if we need to grow.
   if (mpFreeList == NULL)
      grow();

   //-- Pop off the head of the list.
   BFATAL_ASSERT(mpFreeList);
   Node* pNode = mpFreeList;
   mpFreeList = mpFreeList->mpNext;
   if (mpFreeList)
      mpFreeList->mpPrev = NULL;

   //-- Add it to the used list.
   BFATAL_ASSERT(pNode->mpPrev == NULL);
   pNode->mpNext = mpUsedList;
   if (pNode->mpNext)
      pNode->mpNext->mpPrev = pNode;
   mpUsedList = pNode;
   mSize++;
   
   //-- Construct it in place.
   #undef new
   T* pObject = ::new(&pNode->mItem) T;
#if MEMORY_SYSTEM_ENABLE
#if MEMORY_MANAGER_ENABLE_TRACKING   
   #define new      BANG_NEW
   #define BANG_NEW new(__FILE__, __LINE__)
#endif
#endif

   //-- Return it.
   return pObject;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> void BObjectPool<T>::destroyObject(T* pObject)
{
   //-- Make sure we have an item.
   BASSERT(pObject);
   if (!pObject)
      return;

   //-- Destruct it in place.
   pObject->~T();

   //-- Remove it from the used list.
   long offset = sizeof(Node) - sizeof(T);
   Node* pNode  = (Node*)(((BYTE*)pObject) - offset);
   if (pNode->mpPrev)
   {
      BFATAL_ASSERT(pNode != mpUsedList);
      pNode->mpPrev->mpNext = pNode->mpNext;
   }
   else
   {
      BFATAL_ASSERT(pNode == mpUsedList);
      mpUsedList = mpUsedList->mpNext;
   }
   if (pNode->mpNext)
      pNode->mpNext->mpPrev = pNode->mpPrev;

   //-- Add it back to the free list.
   BFATAL_ASSERT(mSize);
   mSize--;
   pNode->mpNext = mpFreeList;
   pNode->mpPrev = NULL;
   mpFreeList    = pNode;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> void BObjectPool<T>::enumObjects(ENUM_OBJECT_FUNC* pFunc, void* pParam) const
{
   //-- Can't do much without a callback.
   BASSERT(pFunc);
   if (!pFunc)
      return;

   //-- Call the callback for each object.  Stop enumerating when the callback
   //-- returns false.
   Node* pNode = mpUsedList;
   while (pNode)
   {
      T* pObject = (T*)(&pNode->mItem);
      if (!pFunc(pObject, pParam))
         return;
      pNode = pNode->mpNext;
   }
}


//=============================================================================
//  POOL CONTROL
//=============================================================================
template <class T> void BObjectPool<T>::reset(unsigned long initialSize, unsigned long growSize)
{
   //-- Release the old data.
   releaseBlocks();

   //-- Reset members.
   mSize          = 0;
   mGrowSize      = growSize;
   mAllocatedSize = 0;
   mpFreeList     = NULL;
   mpUsedList     = NULL;
   mpBlocks       = NULL;

   //-- Allocate storage if necessary.
   if (initialSize)
   {
      mGrowSize = initialSize;
      grow();
      mGrowSize = growSize;
   }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> void BObjectPool<T>::setGrowSize(unsigned long growSize)
{
   mGrowSize = growSize;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> unsigned long BObjectPool<T>::getSize() const
{
   return mSize;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> unsigned long BObjectPool<T>::getGrowSize() const
{
   return mGrowSize;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> unsigned long BObjectPool<T>::getAllocatedSize() const
{
   return mAllocatedSize;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> bool BObjectPool<T>::isGrowExponential() const
{
   return (mGrowSize == BOBJECT_POOL_GROW_EXPONENTIAL);
}


//=============================================================================
//  PRIVATE FUNCTIONS
//=============================================================================
template <class T> void BObjectPool<T>::grow()
{
   BFATAL_ASSERT(mpFreeList == NULL);
   BFATAL_ASSERT(mSize == mAllocatedSize);
   BFATAL_ASSERT(mGrowSize > 0);
   BFATAL_ASSERT((mAllocatedSize == 0 && mpBlocks == NULL) || (mAllocatedSize > 0 && mpBlocks != NULL));

   //-- Determine the size of the block.
   unsigned long size = (mGrowSize == BOBJECT_POOL_GROW_EXPONENTIAL) ? max(mAllocatedSize, 1) : mGrowSize;

   //-- Make a new block.
   Block* pNewBlock = new Block;
   pNewBlock->mpNext = mpBlocks;
   pNewBlock->mpNodes = new Node [size];
   mpBlocks = pNewBlock;

   //-- Adjust the allocated size.
   mAllocatedSize += size;

   //-- Add the new nodes to the free list.
   for (unsigned long index = 1; index < (size - 1); ++index)
   {
      mpBlocks->mpNodes[index].mpNext = &mpBlocks->mpNodes[index + 1];
      mpBlocks->mpNodes[index].mpPrev = &mpBlocks->mpNodes[index - 1];
   }
   if (size == 1)
   {
      mpBlocks->mpNodes[0].mpNext = NULL;
      mpBlocks->mpNodes[0].mpPrev = NULL;
   }
   else
   {
      mpBlocks->mpNodes[0       ].mpNext = &mpBlocks->mpNodes[1];
      mpBlocks->mpNodes[0       ].mpPrev = NULL;
      mpBlocks->mpNodes[size - 1].mpNext = NULL;
      mpBlocks->mpNodes[size - 1].mpPrev = &mpBlocks->mpNodes[size - 2];
   }
   mpFreeList = &mpBlocks->mpNodes[0];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <class T> void BObjectPool<T>::releaseBlocks()
{
   //-- First we must deconstruct all active objects.
   Node* pNode = mpUsedList;
   while (pNode)
   {
      T* pObject = (T*)(&pNode->mItem);
      pObject->~T();
      pNode = pNode->mpNext;
   }
   mpUsedList = NULL;

   //-- Now we can delete the blocks.
   Block* pBlock = mpBlocks;
   while (pBlock)
   {
      Block* pKill = pBlock;
      delete [] pKill->mpNodes;
      pBlock = pBlock->mpNext;
      delete pKill;
      
   }
   mpBlocks   = NULL;
}



