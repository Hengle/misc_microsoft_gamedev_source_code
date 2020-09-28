//============================================================================
//
//  File: blockStackAllocator.cpp
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "blockStackAllocator.h"

//============================================================================
// BBlockStackAllocator::BBlockStackAllocator
//============================================================================
BBlockStackAllocator::BBlockStackAllocator() :
   mpHeap(NULL),
   mInitialSize(0),
   mGrowSize(0),
   mCurFrame(0),
   mMaxUnusedBlockAge(0),
   mNumAllocations(0),
   mTotalBytesAllocated(0),
   mLargestAllocation(0),
   mMaxTotalBytesAllocated(0),
   mMaxLargestAllocation(0),
   mpDV(NULL),
   mTotalBlockBytesAllocated(0)
{
}

//============================================================================
// BBlockStackAllocator::~BBlockStackAllocator
//============================================================================
BBlockStackAllocator::~BBlockStackAllocator()
{
   kill();
}

//============================================================================
// BBlockStackAllocator::init
//============================================================================
bool BBlockStackAllocator::init(BMemoryHeap* pHeap, uint initialSize, uint growSize, uint maxUnusedBlockAge)
{
   kill();
   
   if (!pHeap)
      return false;
   
   mpHeap = pHeap;
   mInitialSize = initialSize;
   mGrowSize = growSize;
   mMaxUnusedBlockAge = maxUnusedBlockAge;
      
   if (initialSize)
   {
      if (!newBlock(initialSize, cMaxAligment))
      {
         kill();
         return false;
      }
   }

   return true;
}

//============================================================================
// BBlockStackAllocator::kill
//============================================================================
void BBlockStackAllocator::kill()
{
   if (!mpHeap)
      return;
      
   for (uint i = 0; i < mBlocks.getSize(); i++)
   {
      bool success = mpHeap->Delete(mBlocks[i].mPtr);
      BVERIFY(success);
   }
      
   mpHeap = NULL;
   mBlocks.clear();
   mInitialSize = 0;
   mGrowSize = 0;
   mNumAllocations = 0;
   mTotalBytesAllocated = 0;
   mLargestAllocation = 0;
   mMaxUnusedBlockAge = 0;
   mMaxTotalBytesAllocated = 0;
   mMaxLargestAllocation = 0;
   mCurFrame = 0;
   mpDV = NULL;
   mTotalBlockBytesAllocated = 0;
}

//============================================================================
// BBlockStackAllocator::newBlock
//============================================================================
BBlockStackAllocator::BBlock* BBlockStackAllocator::newBlock(uint size, uint alignment)
{
   const uint maxSupportedAlignment = mpHeap->getMaxSupportedAlignment();
   if (alignment > maxSupportedAlignment)
      size += (alignment - 1);
   
   int actualSize;
   void* p = mpHeap->AlignedNew(size, maxSupportedAlignment, &actualSize);
   if (!p)
      return NULL;
      
   BDEBUG_ASSERT((uint)actualSize >= size);      
   
   BBlock* pBlock = &mBlocks.grow();
   pBlock->mPtr = p;
   pBlock->mSize = actualSize;
   pBlock->mNext = 0;
   pBlock->mLastFrameUsed = mCurFrame;
   
   mTotalBlockBytesAllocated += actualSize;
   
   return pBlock;
}

//============================================================================
// BBlockStackAllocator::alloc
//============================================================================
void* BBlockStackAllocator::alloc(uint size, uint alignment, bool growIfNeeded)
{
   size = Utils::AlignUpValue(size, sizeof(uint));
   if (!size)
      size = sizeof(DWORD);
      
   if (alignment < sizeof(DWORD)) 
      alignment = sizeof(DWORD);
      
   BDEBUG_ASSERT(Math::IsPow2(alignment) && (alignment <= cMaxAligment));
      
   BBlock* pBlock = NULL;
   if ((mpDV) && (mpDV->getBytesAvailable(alignment) >= size))
      pBlock = mpDV;   
   else
   {
      uint bestBytesAvail = UINT_MAX;
      for (uint i = 0; i < mBlocks.getSize(); i++)
      {
         const uint bytesAvail = mBlocks[i].getBytesAvailable(alignment);
         if (bytesAvail < size)
            continue;
            
         bool accept = false;
         if ( (bytesAvail < bestBytesAvail) && ((bestBytesAvail - bytesAvail) >= 64) )
         {
            // Better fit
            accept = true;
         }
         else 
         {
            // Tie - prefer youngest
            BDEBUG_ASSERT(pBlock);
            
            const uint bestAge = mCurFrame - pBlock->mLastFrameUsed;
            const uint compAge = mCurFrame - mBlocks[i].mLastFrameUsed;
            
            if (compAge < bestAge)
            {
               accept = true;     
            }
         }
         
         if (accept)
         {
            bestBytesAvail = bytesAvail;
            pBlock = &mBlocks[i];
         }
      }
   }   
   
   if (!pBlock)   
   {
      if ((!mGrowSize) || (!growIfNeeded))
         return NULL;
         
      pBlock = newBlock(Math::Max(mGrowSize, size), alignment);
      
      if (!pBlock)
         return NULL;
      
      BDEBUG_ASSERT(pBlock->getBytesAvailable(alignment) >= size);
   }
      
   uint bytesToAlignUp = Utils::BytesToAlignUp(static_cast<uchar*>(pBlock->mPtr) + pBlock->mNext, alignment);
   void* pResult = static_cast<uchar*>(pBlock->mPtr) + pBlock->mNext + bytesToAlignUp;

   pBlock->mLastFrameUsed = mCurFrame;
   pBlock->mNext += (bytesToAlignUp + size);
   BDEBUG_ASSERT(pBlock->mNext <= pBlock->mSize);
   
   if (pBlock->getBytesAvailable())
      mpDV = pBlock;
   
   mNumAllocations++;
   mTotalBytesAllocated += bytesToAlignUp + size;
   mLargestAllocation = Math::Max(mLargestAllocation, size);
   
   BDEBUG_ASSERT(Utils::IsAligned(pResult, alignment));
      
   return pResult;
}

//============================================================================
// BBlockStackAllocator::freeAll
//============================================================================
void BBlockStackAllocator::freeAll(bool deleteAllBlocks)
{
   // JCE, MPB [11/15/2008]
   // Just deleting all here because the usage case for this system doesn't
   // make sense for the classes using it.  It is holding on to old blocks
   // that may not be reused efficiently.  This causes it to spike up in 
   // memory usage.  So, we're just ignoring that here.  Yay.
   deleteAllBlocks = true;

   for (int i = 0; i < (int)mBlocks.getSize(); i++)
   {
      mBlocks[i].mNext = 0;
                  
      if ((!deleteAllBlocks) && (mInitialSize) && (i == 0))
         continue;
      
      const uint framesSinceLastUse = mCurFrame - mBlocks[i].mLastFrameUsed;
      if ((deleteAllBlocks) || (framesSinceLastUse > mMaxUnusedBlockAge))
      {
         BDEBUG_ASSERT(mTotalBlockBytesAllocated >= mBlocks[i].mSize);
         mTotalBlockBytesAllocated -= mBlocks[i].mSize;
         
         bool success = mpHeap->Delete(mBlocks[i].mPtr);
         BVERIFY(success);
         
         mBlocks[i].mPtr = NULL;
         
         mBlocks.eraseUnordered(i);
         i--;
      }
   }

   mMaxTotalBytesAllocated = Math::Max(mMaxTotalBytesAllocated, mTotalBytesAllocated);
   mMaxLargestAllocation = Math::Max(mMaxLargestAllocation, mLargestAllocation);
   mNumAllocations = 0;
   mLargestAllocation = 0;
   mTotalBytesAllocated = 0;      
   mCurFrame++;
   mpDV = NULL;
}

//============================================================================
// BBlockStackAllocator::setAllBlocks
//============================================================================
void BBlockStackAllocator::setAllBlocks(uint c)
{
   for (int i = 0; i < (int)mBlocks.getSize(); i++)  
      Utils::FastMemSet(mBlocks[i].mPtr, c, mBlocks[i].mSize);
}
