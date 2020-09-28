// File: linkedArray.h
#pragma once
#include "fixedBlockAllocator.h"

//typedef BFixedBlockAllocator<128, cNumCacheLines> AllocatorType;

// This container is designed to manage linked lists of blocks, where each block:
// - Fits and is aligned to 128-byte cache lines.
// - Contains a static array of objects.
// - Contains prev/next links.
// Multiple lists can be managed by this class.

template<typename ValueType, typename AllocatorType, bool cExclusiveAllocatorOwnership = false, bool cUseConstructorDestructor = true>
class BLinkedArrayManager
{
public:
   typedef ValueType valueType;
   typedef AllocatorType allocatorType;

   BLinkedArrayManager(AllocatorType& allocator) : 
      mAllocator(allocator)
   {
      BCOMPILETIMEASSERT(AllocatorType::cItemSize >= 16);
   }
   
   ~BLinkedArrayManager()
   {
      freeAllLists(false);
   }
      
   class BLinkedArrayBlock
   {
      friend class BLinkedArrayManager;
      
   public:
      BLinkedArrayBlock()
      {
      }

      BLinkedArrayBlock(uint prevBlock, uint nextBlock) :
         mPrevBlock((ushort)prevBlock),
         mNextBlock((ushort)nextBlock)
      {
      }
      
      uint getPrevBlock(void) const { return mPrevBlock; }             
      uint getNextBlock(void) const { return mNextBlock; }             
      
      uint getNumItems(void) const { return mItems.size(); }
      const ValueType& getItem(uint i) const   { return mItems[i]; }
            ValueType& getItem(uint i)         { return mItems[i]; }

      // Overhead is 8 bytes - 4 for the prev/next block, and 4 for the static array's size.
      enum { cMaxItemsPerBlock = (AllocatorType::cItemSize - 8) / sizeof(ValueType) };
      typedef BStaticArray<ValueType, cMaxItemsPerBlock, false, cUseConstructorDestructor> BStaticArrayType;
      
      const BStaticArrayType& getArray(void) const { return mItems; }
            BStaticArrayType& getArray(void)       { return mItems; }
                                       
   private:
      BStaticArrayType mItems;
      ushort mPrevBlock;
      ushort mNextBlock;
   };
         
   typedef uint64 BItemIter;
   enum { cInvalidItemIter = 0 };
   
   static BLinkedArrayBlock* getBlockPtr(BItemIter iter) { return (BLinkedArrayBlock*)iter; }
   static uint getItemIndex(BItemIter iter) { return (uint)(iter >> 32); }
   
   static BItemIter createIter(const BLinkedArrayBlock* pBlock, uint itemIndex) { return (uint)pBlock | (((uint64)itemIndex) << 32); }
   static BItemIter setItertemIndex(BItemIter iter, uint itemIndex) { return ((uint)iter) | (((uint64)itemIndex) << 32); }
   
   uint getAllocatorItemIndex(BLinkedArrayBlock* pList) { return mAllocator.getItemIndex(pList); }
   BLinkedArrayBlock* getBlockPtrFromAllocatorItemIndex(uint index) { return getBlock(index); }
   const BLinkedArrayBlock* getBlockPtrFromAllocatorItemIndex(uint index) const { return getBlock(index); }
   
   // Created a linked list of blocks.
   // Returns NULL on failure.
   BLinkedArrayBlock* createList(void)
   {
      // rg [5/21/06] - Changed this to <=, not ==
      BCOMPILETIMEASSERT(sizeof(BLinkedArrayBlock) <= AllocatorType::cItemSize);
      
      return allocBlock(NULL);
   }   
   
   // Free a linked list of blocks.
   void freeList(BLinkedArrayBlock* pList)
   {
      BDEBUG_ASSERT(pList);
      const uint headBlockIndex = mAllocator.getItemIndex(pList);

      for ( ; ; )
      {
         const uint nextBlock = pList->mNextBlock;

         if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
         {
            Utils::DestructInPlace(pList);
         }

         mAllocator.free(pList);

         if (nextBlock == headBlockIndex)
            break;

         BLinkedArrayBlock* pNext = getBlock(nextBlock);

         BDEBUG_ASSERT(pNext->mPrevBlock == mAllocator.getItemIndex(pList));

         pList = pNext;
      }
   }
            
   // Adds object to a block list.
   // Returns cInvalidItemIter on failure;
   BItemIter insert(BLinkedArrayBlock* pList, const ValueType& obj)
   {
      BDEBUG_ASSERT(pList);
      
      BLinkedArrayBlock* pTail = getBlock(pList->mPrevBlock);
      
      if (pTail->mItems.isFull())
      {
         pTail = allocBlock(pTail);
         if (!pTail)
            return cInvalidItemIter;
         
         BDEBUG_ASSERT(pTail == getBlock(pList->mPrevBlock));
      }
         
      const uint itemIndex = pTail->mItems.getSize();
      
      pTail->mItems.pushBack(obj);
            
      return createIter(pTail, itemIndex);
   }
   
   // Removes object from a block list.
   BItemIter erase(BLinkedArrayBlock* pList, BItemIter iter, bool unordered = true, bool freeEmptyBlock = true)
   {
      BDEBUG_ASSERT(pList && (iter != cInvalidItemIter));
      
      BLinkedArrayBlock* pBlock = getBlockPtr(iter);
      const uint itemIndex = getItemIndex(iter);
            
      BDEBUG_ASSERT(pBlock && (itemIndex < pBlock->mItems.getSize()));
            
      if (unordered)
         pBlock->mItems.eraseUnordered(itemIndex);
      else
         pBlock->mItems.erase(itemIndex);
      
      if (itemIndex >= pBlock->mItems.getSize())
         iter = nextItem(pList, iter);
            
      if (pBlock->mItems.getEmpty()) 
      {
         if ((freeEmptyBlock) && (pBlock != pList))
            freeBlock(pBlock);
      }
      
      return iter;
   }
   
   // Returns iterator of an item in a block list.
   BItemIter getListItemIterByIndex(const BLinkedArrayBlock* pList, uint itemIndex) const
   {
      BDEBUG_ASSERT(pList);
      const uint headBlockIndex = mAllocator.getItemIndex(pList);

      uint totalItems = 0;

      for ( ; ; )
      {
         const uint nextBlock = pList->mNextBlock;

         const uint prevTotalItems = totalItems;
         totalItems += pList->mItems.getSize();
         
         if (itemIndex < totalItems)
         {
            return createIter(pList, itemIndex - prevTotalItems);
         }

         if (nextBlock == headBlockIndex)
            break;

         const BLinkedArrayBlock* pNext = getBlock(nextBlock);

         BDEBUG_ASSERT(pNext->mPrevBlock == mAllocator.getItemIndex(pList));

         pList = pNext;
      }

      return cInvalidItemIter;
   }
         
   // Returns the size of a block list.
   uint getListSize(const BLinkedArrayBlock* pList) const
   {
      BDEBUG_ASSERT(pList);
      const uint headBlockIndex = mAllocator.getItemIndex(pList);
      
      uint totalItems = 0;

      for ( ; ; )
      {
         const uint nextBlock = pList->mNextBlock;

         totalItems += pList->mItems.getSize();

         if (nextBlock == headBlockIndex)
            break;

         const BLinkedArrayBlock* pNext = getBlock(nextBlock);

         BDEBUG_ASSERT(pNext->mPrevBlock == mAllocator.getItemIndex(pList));

         pList = pNext;
      }
      
      return totalItems;
   }
         
   // Frees all block lists.
   // This assumes the allocator is used exclusively by this object.
   void freeAllLists(bool resetAllocator = true)
   {
      if (!cExclusiveAllocatorOwnership)
         return;
      
      if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
      {
         for (uint i = 0; i < mAllocator.getMaxAllocatedItems(); i++)
            if (mAllocator.isItemAllocated(i))
               Utils::DestructInPlace(getBlock(i));
      }         

      if (resetAllocator)
         mAllocator.freeAll();
   }
      
   static ValueType& getItem(BItemIter iter) { BDEBUG_ASSERT(iter != cInvalidItemIter); return getBlockPtr(iter)->mItems[getItemIndex(iter)]; }
            
   // Returns iter of first item in a block list.
   // Returns cInvalidItemIter if list is empty.
   BItemIter firstItem(const BLinkedArrayBlock* pList) const
   {
      BDEBUG_ASSERT(pList);
      const uint headBlockIndex = mAllocator.getItemIndex(pList);

      for ( ; ; )
      {
         const uint nextBlock = pList->mNextBlock;

         if (pList->mItems.getSize())
            break;

         if (nextBlock == headBlockIndex)
            return cInvalidItemIter;

         const BLinkedArrayBlock* pNext = getBlock(nextBlock);

         BDEBUG_ASSERT(pNext->mPrevBlock == mAllocator.getItemIndex(pList));

         pList = pNext;
      }

      return createIter(pList, 0);
   }

   // Advances an item iter.        
   // Returns cInvalidItemIter at end of list.  
   BItemIter nextItem(const BLinkedArrayBlock* pList, BItemIter iter) const
   {
      if (!iter)
         return cInvalidItemIter;
         
      const BLinkedArrayBlock* pBlock = getBlockPtr(iter);
      uint itemIndex = getItemIndex(iter);
      itemIndex++;
      
      if (itemIndex >= pBlock->mItems.getSize())
      {
         for ( ; ; )
         {
            pBlock = getBlock(pBlock->mNextBlock);
            
            if (pBlock == pList)
               return cInvalidItemIter;
            
            if (!pBlock->mItems.isEmpty())                 
               break;
         }
         
         itemIndex = 0;
      }         
      
      return createIter(pBlock, itemIndex);
   }
   
   const BLinkedArrayBlock* getPrevBlock(const BLinkedArrayBlock* pBlock) const  { return getBlock(pBlock->mPrevBlock); }
         BLinkedArrayBlock* getPrevBlock(BLinkedArrayBlock* pBlock)              { return getBlock(pBlock->mPrevBlock); }
   
   const BLinkedArrayBlock* getNextBlock(const BLinkedArrayBlock* pBlock) const  { return getBlock(pBlock->mNextBlock); }
         BLinkedArrayBlock* getNextBlock(BLinkedArrayBlock* pBlock)              { return getBlock(pBlock->mNextBlock); }
   
   enum 
   {
      cItemSize = sizeof(ValueType), 
      cMaxItemsPerBlock = BLinkedArrayBlock::cMaxItemsPerBlock, 
   };
         
   const AllocatorType& getAllocator(void) const { return mAllocator; }
         AllocatorType& getAllocator(void)       { return mAllocator; }
   
private:
   AllocatorType& mAllocator;
   
   const BLinkedArrayBlock* getBlock(uint index) const { return reinterpret_cast<const BLinkedArrayBlock*>(mAllocator.getItem(index)); }
         BLinkedArrayBlock* getBlock(uint index)       { return reinterpret_cast<BLinkedArrayBlock*>(mAllocator.getItem(index)); }
                  
   // Returns NULL on failure
   BLinkedArrayBlock* allocBlock(BLinkedArrayBlock* pPrev = NULL)
   {
      BLinkedArrayBlock* pBlock = (BLinkedArrayBlock*)mAllocator.alloc(false);
      if (!pBlock)
         return NULL;
         
      Utils::ConstructInPlace(pBlock);
      
      if (!pPrev)
      {
         const uint blockIndex = mAllocator.getItemIndex(pBlock);
         pBlock->mNextBlock = (ushort)blockIndex;
         pBlock->mPrevBlock = (ushort)blockIndex;
      }
      else
      {
         const uint prevBlockIndex = mAllocator.getItemIndex(pPrev);
         const uint nextBlockIndex = pPrev->mNextBlock;

         pBlock->mPrevBlock = (ushort)prevBlockIndex;
         pBlock->mNextBlock = (ushort)nextBlockIndex;
         
         const uint blockIndex = mAllocator.getItemIndex(pBlock);

         pPrev->mNextBlock = (ushort)blockIndex;
         
         BLinkedArrayBlock* pNext = getBlock(nextBlockIndex);
         BDEBUG_ASSERT(pNext->mPrevBlock == prevBlockIndex);

         pNext->mPrevBlock = (ushort)blockIndex;
      }

      return pBlock;
   }   

   void freeBlock(BLinkedArrayBlock* pBlock)
   {
      if (!pBlock)
         return;

      const uint blockIndex = mAllocator.getItemIndex(pBlock);
      const uint prevBlockIndex = pBlock->mPrevBlock;
      const uint nextBlockIndex = pBlock->mNextBlock;

      if (prevBlockIndex != blockIndex)
      {
         BLinkedArrayBlock* pPrev = getBlock(prevBlockIndex);
         BDEBUG_ASSERT(pPrev->mNextBlock == blockIndex);
         pPrev->mNextBlock = (ushort)nextBlockIndex;
      }      

      if (nextBlockIndex != blockIndex)
      {
         BLinkedArrayBlock* pNext = getBlock(nextBlockIndex);
         BDEBUG_ASSERT(pNext->mPrevBlock == blockIndex);
         pNext->mPrevBlock = (ushort)prevBlockIndex;
      }

      if ((cUseConstructorDestructor) && (!BIsBuiltInType<ValueType>::Flag))
         Utils::DestructInPlace(pBlock);

      mAllocator.free(pBlock);
   }
  
};

