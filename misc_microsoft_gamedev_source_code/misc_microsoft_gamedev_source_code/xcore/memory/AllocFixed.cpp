//============================================================================
//
//  AllocFixed.cpp
//
//  Copyright (c) 1999-2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xcore.h"
#include "fatalexit.h"

#define CHECK_THREAD_ID    BDEBUG_ASSERT( mThreadSafe || (mOwnerThreadID == 0) || (mOwnerThreadID == GetCurrentThreadId()) );

//============================================================================
//  STATICS
//============================================================================
#ifdef BUILD_DEBUG
uint BAllocFixed::gNextFreeIndex;
#endif

//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BAllocFixed::BAllocFixed() :
   //-- System Data
   mGrowExponential (false),
   mBlockSize       (0),
   mGrowSize        (0),
   mAllocatedSize   (0),
   mNumAllocations  (0),
   mTotalLocks      (0),
   mCurrentLocks    (0),
   mpFreeHead       (NULL),
   mpAllocationHead (NULL),
   mThreadSafe      (false)
{
#ifdef BUILD_DEBUG
   mOwnerThreadID = 0;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BAllocFixed::~BAllocFixed()
{
   kill();
}


//============================================================================
//  INTERFACE
//============================================================================
//----------------------------------------------------------------------------
//  init()
//  blockSize specifies the size of each lockable piece of memory in bytes.
//  initialNumBlocks specifies how many blocks you want allocated from the heap
//  initially.  growSize specifies how many blocks to allocate each time more
//  blocks are needed.  If growSize is -1, the number of blocks allocated
//  will double each time, starting with the initialNumBlocks.  If growSize is 0
//  no more blocks will ever be allocated and lock() will return NULL when
//  no blocks are left.  pHeap specifies the heap to allocate from.  If pHeap
//  is NULL, the primary heap is used. 
//----------------------------------------------------------------------------
void BAllocFixed::init(long blockSize, long initialNumBlocks, long growSize, bool threadSafe)
{
   //-- Be thread safe.
   mCrit.lock();

   //-- Start anew.
   kill();

   mThreadSafe = threadSafe;

#ifdef DEBUG      
   mOwnerThreadID = GetCurrentThreadId();
#endif
   
   //-- Make sure the block size is usable.  There is no harm in making the
   //-- block size bigger, except that it wastes a few bytes of memory.
   mBlockSize = blockSize;
   if (mBlockSize < sizeof(Block))
      mBlockSize = sizeof(Block);

   //-- Fix up the initial size.
   if (initialNumBlocks < 0)
      initialNumBlocks = 0;

   //-- Set up the grow size.
   mGrowSize = growSize;
   if (mGrowSize < 0)
   {
      mGrowExponential = true;
      mGrowSize = initialNumBlocks;
      if (mGrowSize == 0)
         mGrowSize = 1;
   }

   //-- Allocate the initial blocks.
   newAllocation(initialNumBlocks);
   
   mCrit.unlock();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BAllocFixed::kill(bool ignoreLeaks)
{
   if (!mpAllocationHead)
      return;
      
   CHECK_THREAD_ID
   
   //-- Be thread safe.
   if (mThreadSafe)
      mCrit.lock();
   
   //-- We REALLY shouldn't have ANY locks still active!  But even if we do,
   //-- we gotta shut down anyway.
   if ((!ignoreLeaks) && (!isFatalExit()))
   {
      //  [1/2/2002] Xemu: This just makes sure the heap is empty; if you 
      //  get it, odds are good you have an un-freed new
      // jce [12/18/2003] -- Going to make this a debug assert for now since if playtest gets it,
      // it is usually due to a memory leak somewhere which will be detected later by the memory manager.
      // Once this report gets sent most people abort out and we don't get to see the leak. 
      
      //BDEBUG_ASSERTM(mCurrentLocks == 0, "BAllocFixed::kill -- heap going away, but not empty");
      
      // rg [2/16/06] - FIXME, yes this is lame, but asserting on this is preventing us from fixing more pressing issues during shutdown.
      if (mCurrentLocks != 0)
      {
         trace("ERROR: BAllocFixed::kill -- heap going away, but not empty!");
      }
   }

   //-- Release all allocations.
   Allocation* pKill;
   Allocation* pAllocation = mpAllocationHead;
   while (pAllocation)
   {
      pKill       = pAllocation;
      pAllocation = pAllocation->pNext;
      
      gPrimaryHeap.Delete(pKill->pAllocation);
      gPrimaryHeap.Delete(pKill);
   }

   //-- Reset members.
   mGrowExponential = false;
   mBlockSize       = 0;
   mGrowSize        = 0;
   mAllocatedSize   = 0;
   mNumAllocations  = 0;
   mTotalLocks      = 0;
   mCurrentLocks    = 0;

   mpFreeHead       = NULL;
   mpAllocationHead = NULL;

#ifdef BUILD_DEBUG   
   mOwnerThreadID   = 0;
#endif   
   
   if (mThreadSafe)
      mCrit.unlock();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void* BAllocFixed::lock()
{
   CHECK_THREAD_ID
   
   // -- DO NOT do anything fancy here! This method may be called from a DM notification callback.
   
   //-- Be thread safe.
   if (mThreadSafe)
      mCrit.lock();

   //-- Make sure we are in a valid state.
   BASSERT(isValid());
   if (!isValid())
   {
      if (mThreadSafe)
         mCrit.unlock();
      return NULL;
   }

   //-- See if we need to allocate.
   if (mpFreeHead == NULL)
   {
      //-- If mGrowSize is 0, we can't allocate anymore.
      if (mGrowSize == 0)
      {
         if (mThreadSafe)
            mCrit.unlock();
         return NULL;
      }

      //-- Allocate some blocks.
      newAllocation(mGrowSize);

      //-- Adjust for exponential growth.
      if (mGrowExponential)
         mGrowSize *= 2;

      //-- There's supposed to be available blocks now.
      BASSERT(mpFreeHead != NULL);
      if (mpFreeHead == NULL)
      {
         if (mThreadSafe)
            mCrit.unlock();
         return NULL;
      }
   }

   //-- Remove the free head.
   Block* pBlock = mpFreeHead;

#ifdef BUILD_DEBUG 
   BDEBUG_ASSERT(Block::cMagic == pBlock->mMagic);
   pBlock->mMagic = 0;
#endif   
   
   mpFreeHead = mpFreeHead->pNext;

   //-- Update status.
   mTotalLocks++;
   mCurrentLocks++;

   if (mThreadSafe)
      mCrit.unlock();

   //-- Return the block.
   return pBlock;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void BAllocFixed::unlock(void* pData)
{
   CHECK_THREAD_ID
   
   if (!pData)
      return;
   
   //-- Be thread safe.
   if (mThreadSafe)
      mCrit.lock();

   //-- Make sure we are in a valid state.
   BASSERT(isValid());
   BASSERT(mCurrentLocks);
   if (!isValid())     
   {
      if (mThreadSafe)
         mCrit.unlock();
      return;
   }
   if (!mCurrentLocks) 
   {
      if (mThreadSafe)
         mCrit.unlock();
      return;
   }

   //-- Return it to the free list.
   Block* pBlock = (Block*)pData;

#ifdef BUILD_DEBUG
   if (pBlock->mMagic == Block::cMagic)
   {
      trace("BAllocFixed::unlock: Possible free of already free block at address 0x%X, free index %i", pData, pBlock->mFreeIndex);
   }
      
   pBlock->mMagic = Block::cMagic;
   pBlock->mFreeIndex = (WORD)gNextFreeIndex;
   InterlockedIncrement((volatile LONG*)&gNextFreeIndex);
#endif   
   
   pBlock->pNext = mpFreeHead;
   mpFreeHead    = pBlock;
      
   //-- Update status.
   mCurrentLocks--;
   
   if (mThreadSafe)
      mCrit.unlock();
}

//----------------------------------------------------------------------------
//  unlockAll()
//  This function breaks all existing locks, and adds those locked blocks
//  back to the free list.  It does not return any memory to the heap.
//----------------------------------------------------------------------------
void BAllocFixed::unlockAll()
{
   CHECK_THREAD_ID
   
   //-- Be thread safe.
   if (mThreadSafe)
      mCrit.lock();

   //-- Make sure we are in a valid state.
   BASSERT(isValid());
   if (!isValid())
   {
      if (mThreadSafe)
         mCrit.unlock();
      return;
   }

   //-- Reset the free list, cuz we are about to refill it.
   mpFreeHead    = NULL;
   mCurrentLocks = 0;

   //-- Add all allocations back to the free list.
   Allocation* pAllocation = mpAllocationHead;
   while (pAllocation)
   {
      //-- Add each block to the free list.
      BYTE* pData = (BYTE*)(pAllocation->pAllocation);
      for (long block = 0; block < pAllocation->numBlocks; ++block)
      {
         //-- Get a friendly pointer.
         Block* pBlock = (Block*)pData;

#ifdef BUILD_DEBUG         
         pBlock->mMagic = Block::cMagic;
         pBlock->mFreeIndex = (WORD)gNextFreeIndex;
         InterlockedIncrement((volatile LONG*)&gNextFreeIndex);
#endif         

         //-- Add block to head of free list.
         pBlock->pNext = mpFreeHead;
         mpFreeHead    = pBlock;

         //-- Move to the next block in the allocation.
         pData += mBlockSize;
      }

      pAllocation = pAllocation->pNext;
   }
   
   if (mThreadSafe)
      mCrit.unlock();
}


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
bool BAllocFixed::isValid()
{
   if (mBlockSize <  sizeof(Block)) return false;
   if (mGrowSize  <  0             ) return false;
   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BAllocFixed::newAllocation(long numBlocks)
{
   //-- Make sure we are in a valid state.
   BASSERT(isValid());
   if (!isValid())
      return;

   //-- Make sure we are really trying to allocate something.
   if (numBlocks < 1)
      return;

   //-- Allocate an allocation record.
   long size = sizeof(Allocation);
   Allocation* pAllocation = (Allocation*)gPrimaryHeap.New(size);

   //-- Adjust allocation counts.
   size = numBlocks * mBlockSize;
   mAllocatedSize += size;
   mNumAllocations++;

   //-- Fill out the allocation record.
   if (!pAllocation)
      return;
   pAllocation->numBlocks   = numBlocks;
   pAllocation->pAllocation = gPrimaryHeap.New(size);
   pAllocation->pNext       = mpAllocationHead;
   mpAllocationHead         = pAllocation;

   //-- Add each block to the free list.
   BYTE* pData = (BYTE*)(pAllocation->pAllocation);
   for (long block = 0; block < numBlocks; ++block)
   {
      //-- Get a friendly pointer.
      Block* pBlock = (Block*)pData;

      if (!pBlock)
         break;

#ifdef BUILD_DEBUG           
      //-- Add block to head of free list.
      pBlock->mMagic = Block::cMagic;
      pBlock->mFreeIndex = (WORD)gNextFreeIndex;
      InterlockedIncrement((volatile LONG*)&gNextFreeIndex);
#endif      
      
      pBlock->pNext = mpFreeHead;
      mpFreeHead    = pBlock;

      //-- Move to the next block in the allocation.
      pData += mBlockSize;
   }
}


