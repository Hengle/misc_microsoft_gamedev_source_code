//============================================================================
//
// File: synchronizedBlockAllocator.h
// Copyright (c) 2005-2006, Ensemble Studios
// Simple lock-free freelist using SList API's
//
//============================================================================
#pragma once

//============================================================================
// class BSynchronizedBlockAllocator
//============================================================================
template<typename T, uint cNumSlots, bool UseConstructorDestructor = true>
class BSynchronizedBlockAllocator
{
public:
   BSynchronizedBlockAllocator()
   {
      clear();
   }
      
   ~BSynchronizedBlockAllocator()
   {
   }
   
   void clear(void)
   {
      InitializeSListHead(&mListHead);

      for (int i = cNumSlots - 1; i >= 0; i--)
         InterlockedPushEntrySList(&mListHead, &mSlots[i].mItemEntry); 
   }
         
   T* alloc(void)
   {
      PSLIST_ENTRY pListEntry = InterlockedPopEntrySList(&mListHead);
      if (!pListEntry)
         return NULL;

      // Ensure all writes to the node are imported here (not that we care about this node's data, but I want to 
      // make sure this memory is free of any pending writes before this thread uses it).
      MemoryBarrier();

      BSlotEntry* pSlotEntry = reinterpret_cast<BSlotEntry*>(pListEntry);
      
      if (UseConstructorDestructor)
         Utils::ConstructInPlace(pSlotEntry->getObjPtr());
      
      return pSlotEntry->getObjPtr();
   }
   
   T* alloc(uint& slotIndex)
   {
      PSLIST_ENTRY pListEntry = InterlockedPopEntrySList(&mListHead);
      if (!pListEntry)
         return NULL;
         
      MemoryBarrier();

      BSlotEntry* pSlotEntry = reinterpret_cast<BSlotEntry*>(pListEntry);
      slotIndex = pSlotEntry - &mSlots[0];

      if (UseConstructorDestructor)
         Utils::ConstructInPlace(pSlotEntry->getObjPtr());

      return pSlotEntry->getObjPtr();
   }
   
   T* alloc(const T& objToCopy)
   {
      PSLIST_ENTRY pListEntry = InterlockedPopEntrySList(&mListHead);
      if (!pListEntry)
         return NULL;
         
      MemoryBarrier();         

      BSlotEntry* pSlotEntry = reinterpret_cast<BSlotEntry*>(pListEntry);
      if (UseConstructorDestructor)
         Utils::ConstructInPlace(pSlotEntry->getObjPtr(), objToCopy);
      else
         Utils::FastMemCpy(pSlotEntry->getObjPtr(), &objToCopy, sizeof(objToCopy));
         
      return pSlotEntry->getObjPtr();
   }
   
   T* alloc(uint& slotIndex, const T& objToCopy)
   {
      PSLIST_ENTRY pListEntry = InterlockedPopEntrySList(&mListHead);
      if (!pListEntry)
         return NULL;

      MemoryBarrier();
      
      BSlotEntry* pSlotEntry = reinterpret_cast<BSlotEntry*>(pListEntry);
      slotIndex = pSlotEntry - &mSlots[0];
      
      if (UseConstructorDestructor)
         Utils::ConstructInPlace(pSlotEntry->getObjPtr(), objToCopy);
      else
         Utils::FastMemCpy(pSlotEntry->getObjPtr(), &objToCopy, sizeof(objToCopy));

      return pSlotEntry->getObjPtr();
   }
         
   void free(T* pObj)
   {
      if (!pObj)
         return;
         
      BSlotEntry* pSlotEntry = reinterpret_cast<BSlotEntry*>((uchar*)pObj - sizeof(SLIST_ENTRY));
      BDEBUG_ASSERT(pSlotEntry >= &mSlots[0] && pSlotEntry <= &mSlots[cNumSlots]);
      
      if (UseConstructorDestructor)      
         Utils::DestructInPlace(pObj);
      
      // Ensure all writes to this node are exported before freeing.
      MemoryBarrier();
      
      InterlockedPushEntrySList(&mListHead, &pSlotEntry->mItemEntry);        
   }
         
   void free(uint slotIndex)
   {
      BDEBUG_ASSERT(slotIndex < cNumSlots);

      BSlotEntry* pSlotEntry = &mSlots[slotIndex];
      T* pObj = pSlotEntry->getObjPtr();

      if (UseConstructorDestructor)
         Utils::DestructInPlace(pObj);

      MemoryBarrier();
      
      InterlockedPushEntrySList(&mListHead, &pSlotEntry->mItemEntry);        
   }
   
   uint getSlotIndex(const T* pObj) const
   {
      BDEBUG_ASSERT(pObj);
      const BSlotEntry* pSlotEntry = reinterpret_cast<const BSlotEntry*>(reinterpret_cast<const uchar*>(pObj) - sizeof(SLIST_ENTRY));
      const uint slotIndex = pSlotEntry - &mSlots[0];
      BDEBUG_ASSERT(slotIndex < cNumSlots);
      return slotIndex;
   }
   
   const T* getSlotPtr(uint slotIndex) const
   {
      BDEBUG_ASSERT(slotIndex < cNumSlots);
      return mSlots[slotIndex].getObjPtr();
   }
   
   T* getSlotPtr(uint slotIndex) 
   {
      BDEBUG_ASSERT(slotIndex < cNumSlots);
      return mSlots[slotIndex].getObjPtr();
   }
            
private:
   struct BSlotEntry
   {
      SLIST_ENTRY mItemEntry;
      
      // rg [1/13/05] - It might be better to move the mObjBuf entries to a separate array, to enforce alignment.
      uchar mObjBuf[sizeof(T)];
      
      const T* getObjPtr(void) const   { return reinterpret_cast<T*>(&mObjBuf[0]); }
            T* getObjPtr(void)         { return reinterpret_cast<T*>(&mObjBuf[0]); }
   };
   
   SLIST_HEADER mListHead;
   BSlotEntry mSlots[cNumSlots];
};

#ifndef BUILD_FINAL
class BSynchronizedBlockAllocatorTest
{
public:
   static void test(void);
   
private:
   struct BBlah
   {
      uint mVal;

      BBlah(uint val) : mVal(val) { }
      ~BBlah() { mVal = 0; }

   };

   typedef BSynchronizedBlockAllocator<BBlah, 4096*10, true> BBlockAllocator;

   static void ThreadTestFunc(void* pData1, void* pData2);
};   
#endif
