//============================================================================
//
//  AllocFixed.h
//
//  Copyright (c) 1999-2006, Ensemble Studios
// 
//  rg [2/16/06] - Added threadSafe flag, multiple free detection in debug
//  and custom new operator/delete function.
//
//============================================================================
#pragma once

//----------------------------------------------------------------------------
//  Class BAllocFixed
//----------------------------------------------------------------------------
class BAllocFixed
{
public:
   //-- Construction/Destruction
   BAllocFixed();
   ~BAllocFixed();

   //-- Interface
   void  init     (long blockSize, long initialNumBlocks = 0, long growSize = -1, bool threadSafe = true);
   void  kill     (bool ignoreLeaks = false);
   void* lock     ();
   void  unlock   (void* pData);
   void  unlockAll();

   //-- Status Interface
   inline long getNumAllocations() const { return mNumAllocations;                               }
   inline long getBlockSize     () const { return mBlockSize;                                    }
   inline long getAllocatedSize () const { return mAllocatedSize;                                }
   inline long getLockedSize    () const { return mCurrentLocks * mBlockSize;                    }
   inline long getUnlockedSize  () const { return mAllocatedSize - (mCurrentLocks * mBlockSize); }
   inline long getTotalLocks    () const { return mTotalLocks;                                   }
   inline long getCurrentLocks  () const { return mCurrentLocks;                                 }

private:
   //-- Private Structs
   struct Block
   {
#ifdef BUILD_DEBUG
      enum { cMagic = 0xFE37 };
      WORD mMagic;
      WORD mFreeIndex;
#endif      
      Block* pNext;
   };
      
   struct Allocation
   {
      long        numBlocks;
      void*       pAllocation;
      Allocation* pNext;
   };

   //-- System Data
   long             mBlockSize;
   long             mGrowSize;
   long             mAllocatedSize;
   long             mNumAllocations;
   long             mTotalLocks;
   long             mCurrentLocks;
   
   Block*           mpFreeHead;
   Allocation*      mpAllocationHead;
   BLightWeightMutex mCrit;
   
   bool             mThreadSafe;
   bool             mGrowExponential;

   //-- Disable copy constructor and equals operator.
   BAllocFixed(const BAllocFixed&);
   const BAllocFixed& operator = (const BAllocFixed&);

   //-- Private Functions
   bool isValid      ();
   void newAllocation(long numBlocks);

#ifdef BUILD_DEBUG   
   DWORD             mOwnerThreadID;
   static uint       gNextFreeIndex;
#endif   
};

inline void* operator new  (size_t s, BAllocFixed& allocFixed) 
{ 
   s;
   BDEBUG_ASSERT(s <= (size_t)allocFixed.getBlockSize());
   void* p = allocFixed.lock();
   if (!p)
      BFATAL_FAIL("BAllocFixed operator new failed");
   return p;
}

template<class T>
inline void fixedDelete(T* p, BAllocFixed& allocFixed)
{
   if (p)
   {
      Utils::DestructInPlace(p);   
      allocFixed.unlock(p);
   }
}
