// File: packedObjectManager.cpp
#include "compression.h"
#include "packedObjectManager.h"

#include "memoryPacker.h"
#include "dxtBlockPacker.h"

#include "containers\sparseBitArray.h"

BPackedObjectManager::BPackedObjectManager() :
   mpWorkHeap(NULL),
   mpObjectHeap(NULL),
   mMaxObjects(0),
   mTargetCacheSize(0),
   mUnpackedObjectAgeThreshold(0),
   mCurFrame(0),
   mTotalActiveObjects(0),
   mTotalActiveBytes(0),
   mTotalPackedAllocations(0),
   mTotalPackedBytes(0),
   mTotalUnpackedAllocations(0),
   mTotalUnpackedBytes(0),
   mFailIfOutOfMemory(true),
   mEnforceTargetCacheSize(false),
   mpOnPackCallback(NULL),
   mpOnPackCallbackData(NULL),
   mpOnUnpackCallback(NULL),
   mpOnUnpackCallbackData(NULL),
   mpOnDeleteUnpackedDataCallback(NULL),
   mpOnDeleteUnpackedDataCallbackData(NULL)
{
   Utils::ClearObj(mStats);
   mFreeHead.clear();
   mPackedHead.clear();
   mUnpackedHead.clear();

#ifndef BUILD_FINAL   
   mTotalPackedHeapBytes = 0;
   mTotalUnpackedHeapBytes = 0;
#endif   
}

BPackedObjectManager::~BPackedObjectManager()
{
}

void BPackedObjectManager::init(uint maxObjects, uint targetCacheSize, uint unpackedObjectAgeThreshold, bool enforceTargetCacheSize, bool failIfOutOfMemory, BMemoryHeap* pWorkHeap, BMemoryHeap* pObjectHeap)
{
   BDEBUG_ASSERT(maxObjects && targetCacheSize);

   deinit();

   mMaxObjects = maxObjects + 1U;
   mTargetCacheSize = targetCacheSize;
   mUnpackedObjectAgeThreshold = unpackedObjectAgeThreshold;
   mFailIfOutOfMemory = failIfOutOfMemory;
   mEnforceTargetCacheSize = enforceTargetCacheSize;
   mpWorkHeap = pWorkHeap;
   mpObjectHeap = pObjectHeap;
   mStats.mTotalPacks = 0;
   mStats.mTotalBytesPacked = 0;
   mStats.mTotalUnpacks = 0;
   mStats.mTotalBytesUnpacked = 0;

   const uint initialCounter = Math::Max<uint>(1U, GetTickCount() & 0xFFU);

   mpObjectBase = HEAP_NEW_ARRAY(BObjectBase, mMaxObjects, *mpWorkHeap);
   for (uint i = 0; i < mMaxObjects; i++)
   {
      mpObjectBase[i].mCounter = static_cast<uchar>(i ? initialCounter : 0);
      mpObjectBase[i].mFlags = static_cast<uchar>(i ? cFlagFree : 0);
      mpObjectBase[i].mLastFrameUsed = UINT16_MAX;
      mpObjectBase[i].mUnpackedPtr = NULL;
   }

   mObjectDetail.setHeap(mpWorkHeap);
   mObjectDetail.reserve(mMaxObjects);
   mObjectDetail.resize(1);
   
   mObjectDetail[0].clear();

   mObjectsAccessed.setAllocator(BDynamicArrayHeapAllocator<uint, 4>(mpWorkHeap));
   mLowPriorityObjectsAccessed.setAllocator(BDynamicArrayHeapAllocator<uint, 4>(mpWorkHeap));

   mFreeHead.mpPrev = &mFreeHead;
   mFreeHead.mpNext = &mFreeHead;

   mPackedHead.mpPrev = &mPackedHead;
   mPackedHead.mpNext = &mPackedHead;

   mUnpackedHead.mpPrev = &mUnpackedHead;
   mUnpackedHead.mpNext = &mUnpackedHead;
}

void BPackedObjectManager::deinit()
{
   if (!mMaxObjects)
      return;

   for (uint i = 1; i < mObjectDetail.getSize(); i++)
   {
      deletePackedAllocation(mObjectDetail[i]);
      deleteUnpackedAllocation(mpObjectBase[i], mObjectDetail[i]);
   }

   HEAP_DELETE_ARRAY(mpObjectBase, *mpWorkHeap);
   mpObjectBase = NULL;

   mObjectDetail.clear();

   mObjectsAccessed.clear();
   mLowPriorityObjectsAccessed.clear();

   mMaxObjects = 0;
   mpObjectHeap = NULL;
   mpWorkHeap = NULL;

   mTargetCacheSize = 0;
   mUnpackedObjectAgeThreshold = 0;
   mCurFrame = 0;

   mTotalActiveObjects = 0;
   mTotalActiveBytes = 0;
   mTotalPackedAllocations = 0;
   mTotalPackedBytes = 0;
   mTotalUnpackedAllocations = 0;
   mTotalUnpackedBytes = 0;
   
#ifndef BUILD_FINAL   
   mTotalPackedHeapBytes = 0;
   mTotalUnpackedHeapBytes = 0;
#endif   

   mpOnPackCallback = NULL;
   mpOnPackCallbackData = NULL;

   mpOnUnpackCallback = NULL;
   mpOnUnpackCallbackData = NULL;

   mFreeHead.clear();
   mPackedHead.clear();
   mUnpackedHead.clear();
}
      
void BPackedObjectManager::setOnPackCallback(BCallbackPtr pCallback, void* pPrivateData)
{
   mpOnPackCallback = pCallback;
   mpOnPackCallbackData = pPrivateData;
}

void BPackedObjectManager::setOnUnpackCallback(BCallbackPtr pCallback, void* pPrivateData)
{
   mpOnUnpackCallback = pCallback;
   mpOnUnpackCallbackData = pPrivateData;
}

void BPackedObjectManager::setOnDeleteUnpackedDataCallback(BCallbackPtr pCallback, void* pPrivateData)
{
   mpOnDeleteUnpackedDataCallback = pCallback;
   mpOnDeleteUnpackedDataCallbackData = pPrivateData;
}
      
bool BPackedObjectManager::alloc(
   BPackedPtrBase& ptr, uint num, uint typeSize, uint alignment, ePackerType packerType,
   BUnpackFuncPtr pUnpackFunc, void* pUnpackFuncContext,
   const void* pPackedData, uint packedDataSize)
{
   BDEBUG_ASSERT(num && typeSize);
   if (pPackedData)
   {
      if (packerType != cPTCustom)
         return false;
   }
   else
   {
      if (packerType == cPTCustom)
         return false;
   }
   
   if (!mMaxObjects)
      return false;

   if (mFreeHead.mpNext == &mFreeHead)
   {
      if (mObjectDetail.getSize() == mMaxObjects)
         return false;
   }

   if (!alignment)
   {
      if ((typeSize & 4095) == 0)
         alignment = 4096;
      else if ((typeSize & 15) == 0)  
         alignment = 16;
      else if ((typeSize & 7) == 0)  
         alignment = 8;
      else
         alignment = 4;
   }
   
   alignment = Math::Min(alignment, mpObjectHeap->getMaxSupportedAlignment());

   const uint totalUnpacked = num * typeSize;
   
   BObjectDetail* pObjDetail = mFreeHead.mpNext;
   if (pObjDetail == &mFreeHead)   
      pObjDetail = &mObjectDetail.grow();
   else
      listRemove(pObjDetail);
   
   if (packerType == cPTCustom)
      listInsertAfter(&mPackedHead, pObjDetail);
   else
      listInsertAfter(&mUnpackedHead, pObjDetail);

   const int objIndex = mObjectDetail.findIndex(pObjDetail);
   BDEBUG_ASSERT((objIndex > 0) && (objIndex < (int)mMaxObjects));

   pObjDetail->mTypeSize = typeSize;
   pObjDetail->mNum = num;
   pObjDetail->mPackedPtr = NULL;
   pObjDetail->mPackedSize = 0;
   pObjDetail->mAlignment = static_cast<uint16>(alignment);
   pObjDetail->mUnpackedSize = totalUnpacked;
   pObjDetail->mFrameUnpacked = mCurFrame;
   pObjDetail->mUserData = 0;
   pObjDetail->mPackerType = packerType;
   pObjDetail->mPackedDataIsUncompressed = false;

   void* pUnpacked = NULL;      
   if (packerType == cPTCustom)
   {
      pObjDetail->mpUnpackFunc = pUnpackFunc;
      pObjDetail->mpUnpackFuncContext = pUnpackFuncContext;
      
      pObjDetail->mPackedPtr = (void*)pPackedData;
      pObjDetail->mPackedSize = packedDataSize;
      
      mTotalPackedAllocations++;
      mTotalPackedBytes += pObjDetail->mPackedSize;
      mTotalPackedHeapBytes += pObjDetail->mPackedSize;
   }
   else
   {
      int actualAllocationSize = 0;
      pUnpacked = mpObjectHeap->AlignedNew(totalUnpacked, alignment, &actualAllocationSize);
      if (!pUnpacked)
      {
         if (mFailIfOutOfMemory)
            BFATAL_FAIL("Out of memory");

         return false;
      }
      
      pObjDetail->mpUnpackFunc = NULL;
      pObjDetail->mpUnpackFuncContext = NULL;

      mTotalUnpackedAllocations++;
      mTotalUnpackedBytes += totalUnpacked;
            
#ifndef BUILD_FINAL   
      mTotalUnpackedHeapBytes += actualAllocationSize;
#endif         
   }
   
   BObjectBase& objBase = mpObjectBase[objIndex];

   objBase.mLastFrameUsed = mCurFrame;
   objBase.mFlags = 0;
   objBase.mUnpackedPtr = pUnpacked;

   ptr.mIndex = objIndex;
   ptr.mCounter = objBase.mCounter;

   mTotalActiveObjects++;
   mTotalActiveBytes += totalUnpacked;      
   
   if (packerType == cPTCustom)
      objBase.mFlags |= (cFlagPackedDataPersisted | cFlagReadOnly);
   
   if ((packerType != cPTCustom) && (mTotalUnpackedBytes > mTargetCacheSize))
      tidy();
   
   return true;
}

#pragma push_macro("free")
#undef free
bool BPackedObjectManager::free(BPackedPtrBase& ptr)
#pragma pop_macro("free")
{
   if (!ptr.mIndex)
      return true;

   if ((ptr.mIndex >= mMaxObjects) || (!mTotalActiveObjects))
      return false;

   BObjectBase& objBase = mpObjectBase[ptr.mIndex];   
   if ((objBase.mFlags & cFlagFree) || (ptr.mCounter != objBase.mCounter))
      return false;
   
   flushAccessedObjects();
      
   BObjectDetail& objDetail = mObjectDetail[ptr.mIndex];

   BDEBUG_ASSERT(mTotalActiveBytes >= objDetail.mUnpackedSize);
   mTotalActiveObjects--;
   mTotalActiveBytes -= objDetail.mUnpackedSize;

   deletePackedAllocation(objDetail);
   deleteUnpackedAllocation(objBase, objDetail);

   listRemove(&objDetail);
   listInsertAfter(&mFreeHead, &objDetail);

   objBase.mFlags = cFlagFree;
   objBase.mCounter++;
   if (!objBase.mCounter)
      objBase.mCounter++;

   ptr.clear();

   return true;
}


bool BPackedObjectManager::pack(BPackedPtrBase ptr)
{
   if ((!ptr.mIndex) || (ptr.mIndex >= mMaxObjects))
      return false;

   BObjectBase& objBase = mpObjectBase[ptr.mIndex];

   if ((objBase.mFlags & cFlagFree) || (ptr.mCounter != objBase.mCounter))
      return false;

   if (!objBase.mUnpackedPtr)
      return true;

   if (objBase.mFlags & cFlagPinned)
      return false;

   BObjectDetail& objDetail = mObjectDetail[ptr.mIndex];

   if ((!objDetail.mPackedPtr) || (objBase.mFlags & cFlagDirty))
   {
      if (!packData(objBase, objDetail))
         return false;
   }

   deleteUnpackedAllocation(objBase, objDetail);

   listRemove(&objDetail);
   listInsertAfter(&mPackedHead, &objDetail);

   return true;
}

bool BPackedObjectManager::unpack(BPackedPtrBase ptr, bool targetCacheSizeEnforcement)
{
   if ((!ptr.mIndex) || (ptr.mIndex >= mMaxObjects))
      return false;

   BObjectBase& objBase = mpObjectBase[ptr.mIndex];

   if ((objBase.mFlags & cFlagFree) || (ptr.mCounter != objBase.mCounter))
      return false;

   if (objBase.mUnpackedPtr)
      return true;

   BObjectDetail& objDetail = mObjectDetail[ptr.mIndex];

   if ((targetCacheSizeEnforcement) && (mEnforceTargetCacheSize))
   {
      if ((mTotalUnpackedBytes + objDetail.mUnpackedSize) > mTargetCacheSize)
         return false;
   }

   listRemove(&objDetail);
   listInsertAfter(&mUnpackedHead, &objDetail);

   objBase.mLastFrameUsed = mCurFrame;

   if (!unpackData(objBase, objDetail))
      return false;

   if ((objBase.mFlags & cFlagPackedDataPersisted) == 0)
      deletePackedAllocation(objDetail);

   return true;
}

bool BPackedObjectManager::pin(BPackedPtrBase ptr)
{
   if (!isValid(ptr))
      return false;

   if (!unpack(ptr))
      return false;

   mpObjectBase[ptr.mIndex].mFlags |= cFlagPinned;
   return true;
}

bool BPackedObjectManager::unpin(BPackedPtrBase ptr)
{
   if (!isValid(ptr))
      return false;

   mpObjectBase[ptr.mIndex].mFlags &= ~cFlagPinned;
   return true;
}

bool BPackedObjectManager::packAll()
{
   if (!mMaxObjects)
      return false;

   BObjectDetail* pCur = mUnpackedHead.mpNext;

   while (pCur != &mUnpackedHead)
   {
      BObjectDetail* pNext = pCur->mpNext;

      const int objIndex = mObjectDetail.findIndex(pCur);
      BDEBUG_ASSERT((objIndex > 0) && (objIndex < (int)mMaxObjects));

//-- FIXING PREFIX BUG ID 6278
      const BObjectBase& objBase = mpObjectBase[objIndex];
//--

      BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
      BDEBUG_ASSERT(objBase.mUnpackedPtr);

      if ((objBase.mFlags & cFlagPinned) == 0)
      {
         if (!pack(BPackedPtrBase(objBase.mCounter, objIndex)))
            return false;
      }               

      pCur = pNext;
   }

   return true;
}

bool BPackedObjectManager::unpackAll(bool targetCacheSizeEnforcement)
{
   if (!mMaxObjects)
      return false;

   BObjectDetail* pCur = mPackedHead.mpNext;

   while (pCur != &mPackedHead)
   {
      BObjectDetail* pNext = pCur->mpNext;

      const int objIndex = mObjectDetail.findIndex(pCur);
      BDEBUG_ASSERT((objIndex > 0) && (objIndex < (int)mMaxObjects));

//-- FIXING PREFIX BUG ID 6279
      const BObjectBase& objBase = mpObjectBase[objIndex];
//--

      BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
      if (!objBase.mUnpackedPtr)
      {
         if (!unpack(BPackedPtrBase(objBase.mCounter, objIndex), targetCacheSizeEnforcement))
            return false;
      }            

      pCur = pNext;
   }

   return true;
}

bool BPackedObjectManager::tidy(uint bytesToFreeUp, bool enforceUnpackedObjectAgeThreshold, bool targetCacheSizeEnforcement)
{
   if (!mMaxObjects)
      return false;

   flushAccessedObjects();
      
   if (bytesToFreeUp >= mTargetCacheSize)
      bytesToFreeUp = mTargetCacheSize;
      
   if ( (!targetCacheSizeEnforcement) || (mTotalUnpackedBytes <= (mTargetCacheSize - bytesToFreeUp)) )
      return true;

   BObjectDetail* pCur = mUnpackedHead.mpPrev;

   uint16 prevFramesSinceLastUse = UINT16_MAX;

   while (pCur != &mUnpackedHead)
   {
      BObjectDetail* pPrev = pCur->mpPrev;

      const int objIndex = mObjectDetail.findIndex(pCur);
      BDEBUG_ASSERT((objIndex > 0) && (objIndex < (int)mMaxObjects));

//-- FIXING PREFIX BUG ID 6281
      const BObjectBase& objBase = mpObjectBase[objIndex];
//--

      BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
      BDEBUG_ASSERT(objBase.mUnpackedPtr);

      if (objBase.mFlags & cFlagPinned)
      {
         pCur = pPrev;
         continue;
      }

      uint16 framesSinceLastUse = mCurFrame - objBase.mLastFrameUsed;
      BDEBUG_ASSERT(framesSinceLastUse <= prevFramesSinceLastUse);
      prevFramesSinceLastUse = framesSinceLastUse;

      if (enforceUnpackedObjectAgeThreshold)
      {
         if (framesSinceLastUse < mUnpackedObjectAgeThreshold)
            break;
      }               

      if (!pack(BPackedPtrBase(objBase.mCounter, objIndex)))
         return false;

      if (mTotalUnpackedBytes <= (mTargetCacheSize - bytesToFreeUp))
         break;

      pCur = pPrev;
   }

   return true;
}

bool BPackedObjectManager::advanceFrame(uint bytesToFreeUp, bool enforceUnpackedObjectAgeThreshold, bool targetCacheSizeEnforcement)
{
   if (!mMaxObjects)
      return false;

   mCurFrame++;

   return tidy(bytesToFreeUp, enforceUnpackedObjectAgeThreshold, targetCacheSizeEnforcement);         
}

bool BPackedObjectManager::markAsUsed(BPackedPtrBase ptr, bool lowPriority)
{
   if (!isValid(ptr))
      return false;
      
   BDEBUG_ASSERT(ptr.mIndex < mMaxObjects);

   BObjectBase& objBase = mpObjectBase[ptr.mIndex];   

   BDEBUG_ASSERT((objBase.mFlags & cFlagFree) == 0);
   BDEBUG_ASSERT(ptr.mCounter == objBase.mCounter);

   if (!objBase.mUnpackedPtr)
      return false;
      
   if (objBase.mLastFrameUsed != mCurFrame)
   {
      if (lowPriority)
         mLowPriorityObjectsAccessed.pushBack(ptr.mIndex);
      else
         mObjectsAccessed.pushBack(ptr.mIndex);      
         
      objBase.mLastFrameUsed = mCurFrame;
   }

   return true;
}

bool BPackedObjectManager::markAsUnused(BPackedPtrBase ptr)
{
   if (!isValid(ptr))
      return false;

   flushAccessedObjects();

   if (!mpObjectBase[ptr.mIndex].mUnpackedPtr)
      return false;

   BObjectBase& objectBase = mpObjectBase[ptr.mIndex];
   BObjectDetail& objectDetail = mObjectDetail[ptr.mIndex];

   listRemove(&objectDetail);

//-- FIXING PREFIX BUG ID 6282
   const BObjectDetail* pLast = mUnpackedHead.mpPrev;
//--

   if (pLast != &mUnpackedHead)
   {
      const int lastObjectIndex = mObjectDetail.findIndex(pLast);
      BDEBUG_ASSERT(lastObjectIndex > 0);

      objectBase.mLastFrameUsed = mpObjectBase[lastObjectIndex].mLastFrameUsed;
   }
   else
      objectBase.mLastFrameUsed = static_cast<uint16>(mCurFrame - mUnpackedObjectAgeThreshold);

   listInsertBefore(&mUnpackedHead, &objectDetail);

   return true;
}

#ifndef BUILD_FINAL
#define CHECK(x) do { if (!(x)) { BASSERTM(0, #x); return false; } } while(0)
#else
#define CHECK(x) do { if (!(x)) return false; } while(0)
#endif

bool BPackedObjectManager::check() 
{
   if (!mMaxObjects)
      return false;

   flushAccessedObjects();

   CHECK(mpWorkHeap && mpObjectHeap);
   CHECK(mTotalUnpackedAllocations <= mTotalActiveObjects);
   CHECK(mTotalPackedAllocations <= mTotalActiveObjects);
   CHECK(mObjectDetail.getSize() >= 1);
   CHECK(mObjectDetail.getSize() <= mMaxObjects);
   CHECK(mpObjectBase);

   uint totalFreeFound = 0;
   uint totalActiveFound = 0;
   uint totalActiveBytes = 0;
   uint totalUnpackedFound = 0;
   uint totalUnpackedBytes = 0;
   uint totalPackedFound = 0;
   uint totalPackedBytes = 0;

   CHECK(!mpObjectBase[0].mUnpackedPtr);

   for (uint i = 1; i < mMaxObjects; i++)
   {
      const BObjectBase& objectBase = mpObjectBase[i];

      if (objectBase.mFlags & cFlagFree)
         totalFreeFound++;
      else
      {
         CHECK(mObjectDetail.getSize() > i);

         const BObjectDetail& objectDetail = mObjectDetail[i];

         CHECK(objectDetail.mUnpackedSize);
         CHECK(objectDetail.mNum);
         CHECK(objectDetail.mTypeSize);
         CHECK(objectDetail.mTypeSize * objectDetail.mNum == objectDetail.mUnpackedSize);
         CHECK(objectDetail.mPackedPtr || objectBase.mUnpackedPtr);

         totalActiveFound++;
         totalActiveBytes += objectDetail.mUnpackedSize;

         if (objectBase.mUnpackedPtr)
         {
            int size = 0;
            bool success = mpObjectHeap->Details(objectBase.mUnpackedPtr, &size);
            CHECK(success);
            CHECK(size >= (int)objectDetail.mUnpackedSize);

            totalUnpackedFound++;
            totalUnpackedBytes += objectDetail.mUnpackedSize;
         }

         if (objectDetail.mPackedPtr)
         {
            int size = 0;
            bool success = mpWorkHeap->Details(objectDetail.mPackedPtr, &size);
            CHECK(success);
            CHECK(objectDetail.mPackedSize);
            CHECK(size >= (int)objectDetail.mPackedSize);

            totalPackedFound++;
            totalPackedBytes += objectDetail.mPackedSize;
         }
      }
   }

   CHECK(mTotalActiveObjects == totalActiveFound);
   CHECK(mTotalPackedAllocations == totalPackedFound);
   CHECK(mTotalPackedBytes == totalPackedBytes);
   CHECK(mTotalUnpackedAllocations == totalUnpackedFound);
   CHECK(mTotalUnpackedBytes == totalUnpackedBytes);

   BSparseBitArray foundEntry;
   foundEntry.resize(mMaxObjects);

   const BObjectDetail* pCur = mFreeHead.mpNext;

   for (uint i = mObjectDetail.getSize(); i < mMaxObjects; i++)
      foundEntry.setBit(i);

   uint totalFreelistFound = mMaxObjects - mObjectDetail.getSize();
   while (pCur != &mFreeHead)
   {
      CHECK(pCur);

      int objectIndex = mObjectDetail.findIndex(pCur);
      CHECK((objectIndex >= 1) && (objectIndex < (int)mMaxObjects));
      const BObjectBase& objectBase = mpObjectBase[objectIndex];

      CHECK(!foundEntry.isBitSet(objectIndex));
      foundEntry.setBit(objectIndex);

      CHECK(objectBase.mFlags & cFlagFree);
      CHECK(!objectBase.mUnpackedPtr);

      totalFreelistFound++;
      CHECK(totalFreelistFound <= totalFreeFound);

      pCur = pCur->mpNext;
   }

   CHECK(totalFreelistFound == totalFreeFound);

   pCur = mPackedHead.mpNext;

   uint totalPackedListFound = 0;
   while (pCur != &mPackedHead)
   {
      CHECK(pCur);

      int objectIndex = mObjectDetail.findIndex(pCur);
      CHECK((objectIndex >= 1) && (objectIndex < (int)mMaxObjects));
      const BObjectBase& objectBase = mpObjectBase[objectIndex];

      CHECK(!foundEntry.isBitSet(objectIndex));
      foundEntry.setBit(objectIndex);

      CHECK(!objectBase.mUnpackedPtr);
      CHECK(pCur->mPackedSize && pCur->mPackedPtr);

      totalPackedListFound++;
      CHECK(totalPackedListFound <= totalPackedFound);

      pCur = pCur->mpNext;
   }

   CHECK(totalPackedListFound == totalPackedFound);


   pCur = mUnpackedHead.mpNext;

   uint totalUnpackedListFound = 0;
   uint16 lastObjectAge = 0;
   while (pCur != &mUnpackedHead)
   {
      CHECK(pCur);

      int objectIndex = mObjectDetail.findIndex(pCur);
      CHECK((objectIndex >= 1) && (objectIndex < (int)mMaxObjects));
      const BObjectBase& objectBase = mpObjectBase[objectIndex];

      CHECK(!foundEntry.isBitSet(objectIndex));
      foundEntry.setBit(objectIndex);

      CHECK(objectBase.mUnpackedPtr);

      totalUnpackedListFound++;
      CHECK(totalUnpackedListFound <= totalUnpackedFound);

      uint16 objectAge = (uint16)(mCurFrame - objectBase.mLastFrameUsed);
      CHECK(objectAge >= lastObjectAge);
      lastObjectAge = objectAge;

      pCur = pCur->mpNext;
   }

   CHECK(totalUnpackedListFound == totalUnpackedFound);

   for (uint i = 1; i < mMaxObjects; i++)
   {  
      CHECK(foundEntry.isBitSet(i));
   }

   return true;
}

#undef CHECK   

void BPackedObjectManager::deleteUnpackedAllocation(BObjectBase& objBase, BObjectDetail& objDetail)
{
   if (objBase.mUnpackedPtr)
   {
      if (mpOnDeleteUnpackedDataCallback)
      {
         (*mpOnDeleteUnpackedDataCallback)(this, BPackedPtrBase(objBase.mCounter, &objBase - mpObjectBase), objBase.mUnpackedPtr, objDetail.mUnpackedSize, mpOnDeleteUnpackedDataCallbackData);
      }

#ifndef BUILD_FINAL
      {
         int actualAllocationSize = 0;
         bool success = mpObjectHeap->Details(objBase.mUnpackedPtr, &actualAllocationSize);
         BVERIFY(success);
         BDEBUG_ASSERT((int)mTotalUnpackedHeapBytes >= actualAllocationSize);
         mTotalUnpackedHeapBytes -= actualAllocationSize;
      }
#endif      
         
      bool success = mpObjectHeap->Delete(objBase.mUnpackedPtr);
      BVERIFY(success);

      BDEBUG_ASSERT(mTotalUnpackedAllocations && mTotalUnpackedBytes >= objDetail.mUnpackedSize);
      mTotalUnpackedAllocations--;
      mTotalUnpackedBytes -= objDetail.mUnpackedSize;

      objBase.mUnpackedPtr = NULL;
   }

   objBase.mFlags &= ~cFlagDirty;
}

void BPackedObjectManager::deletePackedAllocation(BObjectDetail& objDetail)
{
   if (!objDetail.mPackedPtr)
      return;
      
   if (objDetail.mPackerType == cPTCustom)      
   {
      BDEBUG_ASSERT((int)mTotalPackedHeapBytes >= objDetail.mPackedSize);
      mTotalPackedHeapBytes -= objDetail.mPackedSize;
      
      (*objDetail.mpUnpackFunc)(objDetail.mpUnpackFuncContext, objDetail.mPackedPtr, objDetail.mPackedSize, NULL, 0);
      
      objDetail.mpUnpackFunc = NULL;
      objDetail.mpUnpackFuncContext = NULL;
   }
   else
   {
#ifndef BUILD_FINAL
      {
         int actualAllocationSize = 0;
         bool success = mpWorkHeap->Details(objDetail.mPackedPtr, &actualAllocationSize);
         BVERIFY(success);
         BDEBUG_ASSERT((int)mTotalPackedHeapBytes >= actualAllocationSize);
         mTotalPackedHeapBytes -= actualAllocationSize;
      }
#endif
      
      bool success = mpWorkHeap->Delete(objDetail.mPackedPtr);
      BVERIFY(success);
   }  
   
   BDEBUG_ASSERT(mTotalPackedAllocations && (mTotalPackedBytes >= objDetail.mPackedSize));
   mTotalPackedAllocations--;
   
   mTotalPackedBytes -= objDetail.mPackedSize;

   objDetail.mPackedPtr = NULL;
   objDetail.mPackedSize = 0;    
}

bool BPackedObjectManager::packData(BObjectBase& objBase, BObjectDetail& objDetail)
{  
   if (mpOnPackCallback)
   {
      (*mpOnPackCallback)(this, BPackedPtrBase(objBase.mCounter, &objBase - mpObjectBase), objBase.mUnpackedPtr, objDetail.mUnpackedSize, mpOnPackCallbackData);
   }
   
   if (objDetail.mPackerType == cPTCustom)
   {
      objBase.mFlags &= ~cFlagDirty;
      return true;
   }
   
   if (objDetail.mPackedPtr)
      deletePackedAllocation(objDetail);

   const uint unpackedDWORDs = (objDetail.mUnpackedSize + 3U) >> 2U;
   const uint tempBufDWORDs = Math::Max(2U, unpackedDWORDs);

   void* pTempBuf = mpWorkHeap->AlignedNew(tempBufDWORDs << 2U, sizeof(uint64));
   if (!pTempBuf)
   {
      if (mFailIfOutOfMemory)
         BFATAL_FAIL("Out of memory");
      return false;
   }
   
   bool success = false;
   
   uint packedDWORDs = tempBufDWORDs;
   
   switch (objDetail.mPackerType)
   {
      case cPTGeneralPurpose:
      {
         BMemoryPacker packer;
         success = packer.pack(static_cast<const DWORD*>(objBase.mUnpackedPtr), unpackedDWORDs, static_cast<DWORD*>(pTempBuf), packedDWORDs, true, true);
         break;
      }
      case cPTDXT1:
      case cPTDXT5:
      case cPTDXN:
      {
         uint dataType = BDXTBlockPacker::cDataTypeDXT1;
         if (objDetail.mPackerType == cPTDXT5)
            dataType = BDXTBlockPacker::cDataTypeDXT5;
         else if (objDetail.mPackerType == cPTDXN)
            dataType = BDXTBlockPacker::cDataTypeDXN;
         
         BDXTBlockPacker dxtBlockPacker;   
         success = dxtBlockPacker.pack(static_cast<const DWORD*>(objBase.mUnpackedPtr), unpackedDWORDs, static_cast<DWORD*>(pTempBuf), packedDWORDs, dataType, true);

         break;
      }
   }
   
   if (!success)
   {
      Utils::FastMemCpy(pTempBuf, objBase.mUnpackedPtr, objDetail.mUnpackedSize);
      
      packedDWORDs = unpackedDWORDs;
      
      objDetail.mPackedPtr = pTempBuf;
      objDetail.mPackedDataIsUncompressed = true;
   }
   else
   {
      objDetail.mPackedPtr = pTempBuf;
      objDetail.mPackedDataIsUncompressed = false;

      if (packedDWORDs < tempBufDWORDs)
      {
         objDetail.mPackedPtr = mpWorkHeap->Resize(pTempBuf, Utils::AlignUpValue(packedDWORDs << 2U, sizeof(uint64)));
         if (!objDetail.mPackedPtr)
         {
            mpWorkHeap->Delete(pTempBuf);
            if (mFailIfOutOfMemory)
               BFATAL_FAIL("Out of memory");
            return false;
         }
      }
   }
   
   mStats.mTotalPacks++;
   mStats.mTotalBytesPacked += objDetail.mUnpackedSize;
   
#ifndef BUILD_FINAL
   {
      int actualAllocationSize = 0;
      bool success = mpWorkHeap->Details(objDetail.mPackedPtr, &actualAllocationSize);
      BVERIFY(success);
      mTotalPackedHeapBytes += actualAllocationSize;
   }
#endif   

   objDetail.mPackedSize = packedDWORDs << 2U;

   mTotalPackedAllocations++;
   mTotalPackedBytes += objDetail.mPackedSize;

   objBase.mFlags &= ~cFlagDirty;
   
//   trace("Packed from %u to %u bytes", objDetail.mUnpackedSize, objDetail.mPackedSize);

   return true;
} 

bool BPackedObjectManager::unpackData(BObjectBase& objBase, BObjectDetail& objDetail)
{
   int actualAllocationSize = 0;
   objBase.mUnpackedPtr = mpObjectHeap->AlignedNew(objDetail.mUnpackedSize, objDetail.mAlignment, &actualAllocationSize);
   if (!objBase.mUnpackedPtr)            
   {
      if (mFailIfOutOfMemory)
         BFATAL_FAIL("Out of memory");
      return false;
   }

#ifndef BUILD_FINAL
   mTotalUnpackedHeapBytes += actualAllocationSize;
#endif       

   if (objDetail.mPackerType == cPTCustom)
   {
      if (!(*objDetail.mpUnpackFunc)(objDetail.mpUnpackFuncContext, objDetail.mPackedPtr, objDetail.mPackedSize, objBase.mUnpackedPtr, objDetail.mUnpackedSize))
         return false;
   }
   else if (objDetail.mPackedDataIsUncompressed)
   {
      Utils::FastMemCpy(objBase.mUnpackedPtr, objDetail.mPackedPtr, objDetail.mUnpackedSize);
   }
   else
   {
      bool success = false;
      
      uint numDstDWORDs = (objDetail.mUnpackedSize + 3U) >> 2U;
      
      switch (objDetail.mPackerType)
      {
         case cPTGeneralPurpose:
         {
            BMemoryUnpacker unpacker;
            success = unpacker.unpack(objDetail.mPackedPtr, objDetail.mPackedSize >> 2U, objBase.mUnpackedPtr, numDstDWORDs);
            break;
         }
         case cPTDXT1:
         case cPTDXT5:
         case cPTDXN:
         {
            BDXTBlockPacker dxtBlockPacker;   
            success = dxtBlockPacker.unpack(objDetail.mPackedPtr, objDetail.mPackedSize >> 2U, objBase.mUnpackedPtr, numDstDWORDs);
            break;
         }               
      }     
      
      BVERIFY( success && (numDstDWORDs == ((objDetail.mUnpackedSize + 3U) >> 2U)) );       
   }      
   
   mStats.mTotalUnpacks++;
   mStats.mTotalBytesUnpacked += objDetail.mUnpackedSize;

   objDetail.mFrameUnpacked = mCurFrame;
   objBase.mFlags &= ~cFlagDirty;

   mTotalUnpackedAllocations++;
   mTotalUnpackedBytes += objDetail.mUnpackedSize;
   
//   trace("Unpacked from %u to %u bytes", objDetail.mPackedSize, objDetail.mUnpackedSize);

   if (mpOnUnpackCallback)
   {
      (*mpOnUnpackCallback)(this, BPackedPtrBase(objBase.mCounter, &objBase - mpObjectBase), objBase.mUnpackedPtr, objDetail.mUnpackedSize, mpOnUnpackCallbackData);
   }         

   return true;
}

void BPackedObjectManager::flushAccessedObjectsArray(BDynamicArray<uint, 4, BDynamicArrayHeapAllocator>& objectsAccessed)
{   
   if (objectsAccessed.getSize())
   {
      for (uint i = 0; i < objectsAccessed.getSize(); i++)
      {
         const uint objIndex = objectsAccessed[i];  
//-- FIXING PREFIX BUG ID 6284
         const BObjectBase& objectBase = mpObjectBase[objIndex];
//--
         if ((objectBase.mFlags & cFlagFree) || (!objectBase.mUnpackedPtr))
            continue;

         BObjectDetail& objectDetail = mObjectDetail[objIndex];

         listRemove(&objectDetail);
         listInsertAfter(&mUnpackedHead, &objectDetail);
      }

      objectsAccessed.resize(0);
   }      
}

void BPackedObjectManager::flushAccessedObjects()
{
   if (!mMaxObjects)
      return;
      
   flushAccessedObjectsArray(mObjectsAccessed);
   
   if (mLowPriorityObjectsAccessed.getSize())
   {
      uint frameIndex = 0;
      
      for (uint i = 0; i < mLowPriorityObjectsAccessed.getSize(); i++)
      {
         const uint objIndex = mLowPriorityObjectsAccessed[i];  
//-- FIXING PREFIX BUG ID 6285
         const BObjectBase& objectBase = mpObjectBase[objIndex];
//--
         if ((objectBase.mFlags & cFlagFree) || (!objectBase.mUnpackedPtr))
            continue;

         BObjectDetail& objectDetail = mObjectDetail[objIndex];
                  
         listRemove(&objectDetail);
         
         frameIndex = objectBase.mLastFrameUsed;
      }
                  
      BObjectDetail* pPrev = &mUnpackedHead;
      BObjectDetail* pCur = pPrev->mpNext;
      
      while (pCur != &mUnpackedHead)
      {
         int objectIndex = mObjectDetail.findIndex(pCur);
         BDEBUG_ASSERT((objectIndex > 0) && (objectIndex < (int)mMaxObjects));

         if (mpObjectBase[objectIndex].mLastFrameUsed != frameIndex)
            break;
         
         pPrev = pCur;
         pCur = pCur->mpNext;
      }
      
      BObjectDetail* pObjectToInsertAfter = pPrev;
      
      for (uint i = 0; i < mLowPriorityObjectsAccessed.getSize(); i++)
      {
         const uint objIndex = mLowPriorityObjectsAccessed[i];  
//-- FIXING PREFIX BUG ID 6286
         const BObjectBase& objectBase = mpObjectBase[objIndex];
//--
         if ((objectBase.mFlags & cFlagFree) || (!objectBase.mUnpackedPtr))
            continue;

         BObjectDetail& objectDetail = mObjectDetail[objIndex];

         listInsertAfter(pObjectToInsertAfter, &objectDetail);
      }
               
      mLowPriorityObjectsAccessed.resize(0);
   }      
}

void BPackedObjectManager::getStats(BStats& stats, bool clear)
{
   Utils::ClearObj(stats);
   
   stats.mTotalActiveObjects = mTotalActiveObjects;
   stats.mTotalActiveBytes = mTotalActiveBytes;
   stats.mTotalPackedAllocs = mTotalPackedAllocations;
   stats.mTotalPackedBytes = mTotalPackedBytes;
   stats.mTotalUnpackedAllocs = mTotalUnpackedAllocations;
   stats.mTotalUnpackedBytes = mTotalUnpackedBytes;
   stats.mTotalPacks = mStats.mTotalPacks;
   stats.mTotalBytesPacked = mStats.mTotalBytesPacked;
   stats.mTotalUnpacks = mStats.mTotalUnpacks;
   stats.mTotalBytesUnpacked = mStats.mTotalBytesUnpacked;

   if (clear)
      Utils::ClearObj(mStats);
}
