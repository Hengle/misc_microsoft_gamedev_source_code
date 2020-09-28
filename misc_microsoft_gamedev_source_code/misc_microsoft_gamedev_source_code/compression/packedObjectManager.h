// File: packedObjectManager.h
#pragma once

#include "memoryPacker.h"
#include "containers\sparseBitArray.h"
#include "containers\segmentedArray.h"

class BPackedObjectManager;

class BPackedPtrBase
{
public:
   friend class BPackedObjectManager;
   
   BPackedPtrBase() { }
   
   bool operator== (const BPackedPtrBase& rhs) const { return (mCounter == rhs.mCounter) && (mIndex == rhs.mIndex); }
         
   // Gets/sets as a 24-bit handle - can only handle 65535 objects max!
   uint getSmallHandle24() const { BDEBUG_ASSERT(mIndex <= 65535); return mCounter | ((mIndex & 0xFFFF) << 8U); }
   void setFromSmallHandle24(uint handle) { mCounter = handle & 0xFF; mIndex = handle >> 8U; BDEBUG_ASSERT(mIndex); }
   
   inline void clear() { mCounter = 0; mIndex = 0; }
   inline bool isNull() const { return 0 == mIndex; }
   
protected:
   BPackedPtrBase(uint counter, uint index) : mCounter(counter), mIndex(index) { }
   
   DWORD mCounter : 8;
   DWORD mIndex   : 24;
};
 
class BPackedObjectManager
{
   friend class BPackedPtrBase;
   
   BPackedObjectManager(const BPackedObjectManager&);
   BPackedObjectManager& operator= (const BPackedObjectManager&);
   
public:
   BPackedObjectManager();
   ~BPackedObjectManager();
         
   void init(uint maxObjects, uint targetCacheSize, uint unpackedObjectAgeThreshold = 2, bool enforceTargetCacheSize = false, bool failIfOutOfMemory = true, BMemoryHeap* pWorkHeap = &gPrimaryHeap, BMemoryHeap* pObjectHeap = &gPrimaryHeap);
   void deinit();
      
   typedef void (*BCallbackPtr)(BPackedObjectManager* pManager, BPackedPtrBase ptr, void* pData, uint dataSize, void* pPrivateData);
   
   void setOnPackCallback(BCallbackPtr pCallback, void* pPrivateData);
   void setOnUnpackCallback(BCallbackPtr pCallback, void* pPrivateData);
   void setOnDeleteUnpackedDataCallback(BCallbackPtr pCallback, void* pPrivateData);
      
   bool getFailIfOutOfMemory() const { return mFailIfOutOfMemory; }
   void setFailIfOutOfMemory(bool failIfOutOfMemory) { mFailIfOutOfMemory = failIfOutOfMemory; }
   
   enum ePackerType
   {
      cPTGeneralPurpose = 0,
      cPTDXT1,
      cPTDXT5,
      cPTDXN,
      cPTCustom
   };
   
   typedef bool (*BUnpackFuncPtr)(void* pContext, const void* pPackedData, uint packedDataSize, void* pDst, uint dstSize);
   
   bool alloc(
      BPackedPtrBase& ptr, uint num, uint typeSize, uint alignment = 0, ePackerType packerType = cPTGeneralPurpose, 
      BUnpackFuncPtr pUnpackFunc = NULL, void* pUnpackFuncContext = NULL,
      const void* pPackedData = NULL, uint packedDataSize = 0);
      
   #pragma push_macro("free")
   #undef free
   bool free(BPackedPtrBase& ptr);
   #pragma pop_macro("free")
      
   inline bool isValid(BPackedPtrBase ptr)
   {
      if ((!ptr.mIndex) || (ptr.mIndex >= mMaxObjects))
         return false;
      
      BObjectBase& objBase = mpObjectBase[ptr.mIndex];   
      if ((objBase.mFlags & cFlagFree) || (ptr.mCounter != objBase.mCounter))
         return false;
      
      return true;
   }
   
   inline uint32 getUserData(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return 0;
         
      return mObjectDetail[ptr.mIndex].mUserData;
   }
   
   inline bool setUserData(BPackedPtrBase ptr, uint32 userData)
   {
      if (!isValid(ptr))
         return false;
      
      mObjectDetail[ptr.mIndex].mUserData = userData;
      
      return true;
   }
   
   inline const void* getPtrReadOnly(BPackedPtrBase ptr)
   {
      BDEBUG_ASSERT(ptr.mIndex < mMaxObjects);

      BObjectBase& objBase = mpObjectBase[ptr.mIndex];   

      BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
      BDEBUG_ASSERT(ptr.mCounter == objBase.mCounter);

      if (!objBase.mUnpackedPtr)
      {
         if (!unpack(ptr))
            return NULL;
      }
      else if (objBase.mLastFrameUsed != mCurFrame)
      {
         mObjectsAccessed.pushBack(ptr.mIndex);      
         objBase.mLastFrameUsed = mCurFrame;
      }
            
      return objBase.mUnpackedPtr;
   }
   
   inline void* getPtr(BPackedPtrBase ptr)
   {
      BDEBUG_ASSERT(ptr.mIndex < mMaxObjects);

      BObjectBase& objBase = mpObjectBase[ptr.mIndex];   

      BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
      BDEBUG_ASSERT(ptr.mCounter == objBase.mCounter);
      BDEBUG_ASSERT((objBase.mFlags & cFlagReadOnly) == 0);

      if (!objBase.mUnpackedPtr)
      {
         if (!unpack(ptr))
            return NULL;
      }
      else if (objBase.mLastFrameUsed != mCurFrame)
      {
         mObjectsAccessed.pushBack(ptr.mIndex);      
         objBase.mLastFrameUsed = mCurFrame;
      }
      objBase.mFlags |= cFlagDirty;

      return objBase.mUnpackedPtr;
   }
   
   inline const void* getPtrReadOnly(BPackedPtrBase ptr, uint index, uint objSize)
   {
      BDEBUG_ASSERT((ptr.mIndex > 0) && (ptr.mIndex < mMaxObjects));

      BObjectBase& objBase = mpObjectBase[ptr.mIndex];
      
      BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
      BDEBUG_ASSERT(ptr.mCounter == objBase.mCounter);
      
#ifdef BUILD_DEBUG      
      BObjectDetail& objDetail = mObjectDetail[ptr.mIndex];
      objDetail;
      BDEBUG_ASSERT(objDetail.mTypeSize == objSize);
      BDEBUG_ASSERT(index < objDetail.mNum);
#endif      

      if (!objBase.mUnpackedPtr)
      {
         if (!unpack(ptr))
            return NULL;
      }
      else if (objBase.mLastFrameUsed != mCurFrame)
      {
         mObjectsAccessed.pushBack(ptr.mIndex);      
         objBase.mLastFrameUsed = mCurFrame;
      }

      return static_cast<const uchar*>(objBase.mUnpackedPtr) + (index * objSize);
   }
   
   inline void* getPtr(BPackedPtrBase ptr, uint index, uint objSize)
   {
      BDEBUG_ASSERT((ptr.mIndex > 0) && (ptr.mIndex < mMaxObjects));

      BObjectBase& objBase = mpObjectBase[ptr.mIndex];

      BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
      BDEBUG_ASSERT(ptr.mCounter == objBase.mCounter);
      BDEBUG_ASSERT((objBase.mFlags & cFlagReadOnly) == 0);

#ifdef BUILD_DEBUG      
      BObjectDetail& objDetail = mObjectDetail[ptr.mIndex];
      objDetail;
      BDEBUG_ASSERT(objDetail.mTypeSize == objSize);
      BDEBUG_ASSERT(index < objDetail.mNum);
#endif      

      if (!objBase.mUnpackedPtr)
      {
         if (!unpack(ptr))
            return NULL;
      }
      else if (objBase.mLastFrameUsed != mCurFrame)
      {
         mObjectsAccessed.pushBack(ptr.mIndex);      
         objBase.mLastFrameUsed = mCurFrame;
      }
      
      objBase.mFlags |= cFlagDirty;

      return static_cast<uchar*>(objBase.mUnpackedPtr) + (index * objSize);
   }
   
   bool pack(BPackedPtrBase ptr);
   bool unpack(BPackedPtrBase ptr, bool targetCacheSizeEnforcement = true);
      
   bool pin(BPackedPtrBase ptr);
   bool unpin(BPackedPtrBase ptr);
         
   bool packAll();
   bool unpackAll(bool targetCacheSizeEnforcement = true);
         
   bool tidy(uint bytesToFreeUp = 0, bool enforceUnpackedObjectAgeThreshold = true, bool targetCacheSizeEnforcement = true);
   bool advanceFrame(uint bytesToFreeUp = 0, bool enforceUnpackedObjectAgeThreshold = true, bool targetCacheSizeEnforcement = true);
                  
   inline uint getPackedSize(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return 0;

      if (!mObjectDetail[ptr.mIndex].mPackedPtr)
         return 0;
      
      return mObjectDetail[ptr.mIndex].mPackedSize;
   }
   
   inline uint getUnpackedSize(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return 0;
      
      return mObjectDetail[ptr.mIndex].mUnpackedSize;
   }
   
   inline bool isPacked(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      return mObjectDetail[ptr.mIndex].mPackedPtr != NULL;
   }
   
   inline bool isUnpacked(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      return mpObjectBase[ptr.mIndex].mUnpackedPtr != NULL;
   }

   inline bool isReadOnly(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      return (mpObjectBase[ptr.mIndex].mFlags & cFlagReadOnly) != 0;
   }

   inline bool isPackedDataPersisted(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      return (mpObjectBase[ptr.mIndex].mFlags & cFlagPackedDataPersisted) != 0;
   }

   inline bool isDirty(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      return (mpObjectBase[ptr.mIndex].mFlags & cFlagDirty) != 0;
   }

   inline bool isPinned(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      return (mpObjectBase[ptr.mIndex].mFlags & cFlagPinned) != 0;
   }
   
   inline bool setReadOnly(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;
      
      mpObjectBase[ptr.mIndex].mFlags |= cFlagReadOnly;
      return true;
   }
   
   inline bool setPackedDataPersisted(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      mpObjectBase[ptr.mIndex].mFlags |= cFlagPackedDataPersisted;
      return true;
   }
   
   inline bool setDirty(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return false;

      if (mpObjectBase[ptr.mIndex].mFlags & cFlagReadOnly)
         return false;

      if (mpObjectBase[ptr.mIndex].mUnpackedPtr)
         mpObjectBase[ptr.mIndex].mFlags |= cFlagDirty;
         
      return true;
   }
   
   inline uint getArraySize(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return 0;
      return mObjectDetail[ptr.mIndex].mNum;     
   }
   
   inline uint getFramesSinceLastUse(BPackedPtrBase ptr)
   {
      if (!isValid(ptr))
         return 0;
      
      if (!mpObjectBase[ptr.mIndex].mUnpackedPtr)
         return 0;
      
      uint16 framesSinceLastUse = mCurFrame - mpObjectBase[ptr.mIndex].mLastFrameUsed;
      return framesSinceLastUse;
   }
      
   bool markAsUsed(BPackedPtrBase ptr, bool lowPriority);
   bool markAsUnused(BPackedPtrBase ptr);
   
   bool check();
   
   inline uint getTargetCacheSize() const { return mTargetCacheSize; }
   inline uint getTotalPackedBytes() const { return mTotalPackedBytes; }
   inline uint getTotalUnpackedBytes() const { return mTotalUnpackedBytes; }
   
   struct BStats 
   {
      uint mTotalActiveObjects;
      uint mTotalActiveBytes;
      uint mTotalUnpackedAllocs;
      uint mTotalUnpackedBytes;
      uint mTotalPackedAllocs;
      uint mTotalPackedBytes;
      uint mTotalPacks;
      uint mTotalBytesPacked;
      uint mTotalUnpacks;
      uint mTotalBytesUnpacked;
   };

   void getStats(BStats& stats, bool clear = true);

private:
   enum 
   {
      cFlagFree                = 1,
      cFlagPinned              = 2,
      cFlagReadOnly            = 4,
      cFlagPackedDataPersisted = 8,
      cFlagDirty               = 16
   };
   
   struct BObjectBase
   {
      void*          mUnpackedPtr;
      uint16         mLastFrameUsed;
      uchar          mCounter;
      uchar          mFlags;
                  
      void clear() { Utils::ClearObj(*this); }
   };
   
   struct BObjectDetail
   {
      uint32         mUserData;
      
      BObjectDetail* mpPrev;
      BObjectDetail* mpNext;
      
      void*          mPackedPtr;
      uint           mPackedSize;
      
      uint           mNum;
      uint           mTypeSize;
      uint           mUnpackedSize;
      
      BUnpackFuncPtr mpUnpackFunc;
      void*          mpUnpackFuncContext;
                        
      uint16         mFrameUnpacked;
      
      uint16         mAlignment;
            
      ePackerType    mPackerType;
      bool           mPackedDataIsUncompressed : 1;
            
      void clear() { Utils::ClearObj(*this); }
   };
      
   uint                                mTargetCacheSize;
   uint                                mUnpackedObjectAgeThreshold;
      
   BMemoryHeap*                        mpWorkHeap;
   BMemoryHeap*                        mpObjectHeap;
      
   uint                                mMaxObjects;
   BObjectBase*                        mpObjectBase;
   BSegmentedArray<BObjectDetail, 9>   mObjectDetail;
   
   BDynamicArray<uint, 4, BDynamicArrayHeapAllocator> mObjectsAccessed;
   BDynamicArray<uint, 4, BDynamicArrayHeapAllocator> mLowPriorityObjectsAccessed;
         
   BObjectDetail                       mFreeHead;
   BObjectDetail                       mPackedHead;
   BObjectDetail                       mUnpackedHead;
   
   uint                                mTotalActiveObjects;
   uint                                mTotalActiveBytes;
   
   uint                                mTotalPackedAllocations;
   uint                                mTotalPackedBytes;
      
   uint                                mTotalUnpackedAllocations;
   uint                                mTotalUnpackedBytes;
   
   uint                                mTotalPackedHeapBytes;
   uint                                mTotalUnpackedHeapBytes;
   
   BCallbackPtr                        mpOnPackCallback;
   void*                               mpOnPackCallbackData;
   
   BCallbackPtr                        mpOnUnpackCallback;
   void*                               mpOnUnpackCallbackData;
   
   BCallbackPtr                        mpOnDeleteUnpackedDataCallback;
   void*                               mpOnDeleteUnpackedDataCallbackData;
   
   struct 
   {
      uint                                mTotalPacks;
      uint                                mTotalBytesPacked;
      uint                                mTotalUnpacks;
      uint                                mTotalBytesUnpacked;
   } mStats;
         
   uint16                              mCurFrame;
      
   bool                                mFailIfOutOfMemory;
   bool                                mEnforceTargetCacheSize;
   
   static inline void listRemove(BObjectDetail* p)
   {
      BObjectDetail* pPrev = p->mpPrev;
      BObjectDetail* pNext = p->mpNext;
      BDEBUG_ASSERT(pPrev->mpNext == p);
      BDEBUG_ASSERT(pNext->mpPrev == p);
      pPrev->mpNext = pNext;
      pNext->mpPrev = pPrev; 
   }

   static inline void listInsertAfter(BObjectDetail* pList, BObjectDetail* pObj)
   {
      BObjectDetail* pListNext = pList->mpNext;
      BDEBUG_ASSERT(pListNext->mpPrev == pList);
      pList->mpNext = pObj;
      pObj->mpPrev = pList;
      pObj->mpNext = pListNext;
      pListNext->mpPrev = pObj;
   }

   static inline void listInsertBefore(BObjectDetail* pList, BObjectDetail* pObj)
   {
      BObjectDetail* pListPrev = pList->mpPrev;
      BDEBUG_ASSERT(pListPrev->mpNext == pList);
      pList->mpPrev = pObj;
      pObj->mpNext = pList;
      pObj->mpPrev = pListPrev;
      pListPrev->mpNext = pObj;
   }
   
   void deleteUnpackedAllocation(BObjectBase& objBase, BObjectDetail& objDetail);
   void deletePackedAllocation(BObjectDetail& objDetail);
   bool packData(BObjectBase& objBase, BObjectDetail& objDetail);
   bool unpackData(BObjectBase& objBase, BObjectDetail& objDetail);
   void flushAccessedObjectsArray(BDynamicArray<uint, 4, BDynamicArrayHeapAllocator>& objectsAccessed);
   void flushAccessedObjects();
};

template<class ObjType, class PtrManager>
class BPackedPtr : public BPackedPtrBase, public PtrManager
{
public:
   typedef ObjType BObjType;
   typedef PtrManager BPtrManager;
   
   inline BPackedPtr(uint counter = 0, uint index = 0) : BPackedPtrBase(counter, index) { }

   inline BPackedPtr(const BPackedPtr& other) : BPackedPtrBase(other) { }

   inline BPackedPtr& operator= (const BPackedPtr& rhs) { mCounter = rhs.mCounter; mIndex = rhs.mIndex; return *this; } 
      
   inline operator bool() const { return mIndex != 0; }

   inline const ObjType& operator*()  const   { const ObjType* p = static_cast<const ObjType*>(getManager()->getPtrReadOnly(*this)); BDEBUG_ASSERT(p); return *p; }
   inline       ObjType& operator*()          {       ObjType* p = static_cast<      ObjType*>(getManager()->getPtr(*this)); BDEBUG_ASSERT(p); return *p; }

   inline const ObjType* operator->() const   { return static_cast<const ObjType*>(getManager()->getPtrReadOnly(*this)); }
   inline       ObjType* operator->()         { return static_cast<      ObjType*>(getManager()->getPtr(*this)); }

   inline const ObjType* getPtrReadOnly() const  { return static_cast<const ObjType*>(getManager()->getPtrReadOnly(*this)); }
   
   inline const ObjType* getPtr() const          { return static_cast<const ObjType*>(getManager()->getPtrReadOnly(*this)); }
   inline       ObjType* getPtr()                { return static_cast<      ObjType*>(getManager()->getPtr(*this)); }

   inline const ObjType& operator[] (uint i) const  { return *static_cast<const ObjType*>(getManager()->getPtrReadOnly(*this, i, sizeof(ObjType))); }
   inline       ObjType& operator[] (uint i)        { return *static_cast<      ObjType*>(getManager()->getPtr(*this, i, sizeof(ObjType))); }

   inline bool pin() { return getManager()->pin(*this); }
   inline bool unpin() { return getManager()->unpin(*this); };

   inline bool pack() { return getManager()->pack(*this); };
   inline bool unpack() { return getManager()->unpack(*this); };
   
   inline bool setDirty() { return getManager()->setDirty(*this); };
   inline bool isDirty() { return getManager()->isDirty(*this); };
   
   inline bool setReadOnly() { return getManager()->setReadOnly(*this); };
   inline bool isReadOnly() { return getManager()->isReadOnly(*this); };
   
   inline bool setPackedDataPersisted() { return getManager()->setPackedDataPersisted(*this); };
   inline bool isPackedDataPersisted() { return getManager()->isPackedDataPersisted(*this); };
};

template<class Type, class PtrManager> 
inline BPackedPtr<Type, PtrManager> packedNew()
{
   typedef BPackedPtr<Type, PtrManager> PackedPtrType;
   
   PackedPtrType ptr;
   
   bool success = PackedPtrType::getManager()->alloc(ptr, 1, sizeof(PackedPtrType::BObjType));
   BVERIFY(success);
   
   if (!BIsBuiltInType<PackedPtrType::BObjType>::Flag) 
      Utils::ConstructInPlace(ptr.getPtr());
   
   return ptr;
}

template<class Type, class PtrManager> 
inline BPackedPtr<Type, PtrManager> packedNew(const Type& init)
{
   typedef BPackedPtr<Type, PtrManager> PackedPtrType;

   PackedPtrType ptr;

   bool success = PackedPtrType::getManager()->alloc(ptr, 1, sizeof(PackedPtrType::BObjType));
   BVERIFY(success);

   if (!BIsBuiltInType<PackedPtrType::BObjType>::Flag) 
      Utils::ConstructInPlace(ptr.getPtr(), init);

   return ptr;
}

template<class Type, class PtrManager> 
inline BPackedPtr<Type, PtrManager> packedNewArray(uint num)
{
   BDEBUG_ASSERT(num > 0);
   
   typedef BPackedPtr<Type, PtrManager> PackedPtrType;

   PackedPtrType ptr;

   bool success = PackedPtrType::getManager()->alloc(ptr, num, sizeof(PackedPtrType::BObjType));
   BVERIFY(success);

   if (!BIsBuiltInType<PackedPtrType::BObjType>::Flag) 
      Utils::ConstructArrayInPlace(ptr.getPtr(), num);

   return ptr;
}

template<class PackedPtrType> inline void packedDelete(PackedPtrType ptr)
{
   bool success = PackedPtrType::getManager()->free(ptr);
   BVERIFY(success);
}

template<class PackedPtrType> inline void packedDeleteArray(PackedPtrType ptr)
{
   if (ptr.isNull())
      return;
      
   if (!BIsBuiltInType<PackedPtrType::BObjType>::Flag) 
   {
      const uint num = PackedPtrType::getManager()->getArraySize(ptr);
      BVERIFY(num);
         
      Utils::DestructArrayInPlace(ptr.getPtr(), num);
   }
      
   bool success = PackedPtrType::getManager()->free(ptr);
   BVERIFY(success);
}
