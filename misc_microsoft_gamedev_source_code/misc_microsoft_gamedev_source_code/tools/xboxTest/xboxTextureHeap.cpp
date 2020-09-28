//============================================================================
//
//  xboxTextureHeap.cpp
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xboxTextureHeap.h"
#include "memory\regionAllocator.h"

static void* const pPhysHeapStart      = (void*)0xA0000000;
static void* const pPhysHeapEnd        = (void*)0xC0000000;
static const uint cPhysHeapSize        = 512U * 1024U * 1024U;
static const uint cPhysHeapSizeInPages = cPhysHeapSize >> 12;

BXboxTextureHeap::BXboxTextureHeap() :
   mRegionHeapMaxUnusedSize(0),
   mTotalPhysAllocations(0),
   mTotalPhysBytes(0),
   mUnusedRegionBufLargestUnused(0)
{
}

BXboxTextureHeap::~BXboxTextureHeap()
{
   BScopedCriticalSection lock(mValleyMutex);
   
   deinit();
}

void BXboxTextureHeap::init()
{
   BScopedCriticalSection lock(mValleyMutex);
   
   deinit();
   
   mRegionPages.resize(cPhysHeapSizeInPages);
}

void* BXboxTextureHeap::allocatePhysicalMemory(uint size)
{
   void* p = gPhysCachedHeap.AlignedNew(size, 4096);
   if (!p)
      return p;
   
   mTotalPhysAllocations++;
   mTotalPhysBytes += size;
   
   BASSERT(p >= pPhysHeapStart);
   BASSERT((static_cast<uchar*>(p) + size) <= pPhysHeapEnd);
   
   return p;
}

void BXboxTextureHeap::deletePhysicalMemory(void* p, uint size)
{
   if (!p)
      return;

   BASSERT((mTotalPhysAllocations > 0) && (mTotalPhysBytes >= size));
   
   mTotalPhysAllocations--;
   mTotalPhysBytes -= size;
   
   if (!mTotalPhysAllocations)
   {
      BASSERT(!mTotalPhysBytes);
   }
   
   bool success = gPhysCachedHeap.Delete(p);
   BVERIFY(success);
}

void BXboxTextureHeap::deinit()
{
   BScopedCriticalSection lock(mValleyMutex);
   
   for (BValleyHashMap::const_iterator it = mValleyHashMap.begin(); it != mValleyHashMap.end(); ++it)
   {
      const BValley& valley = it->second;
      
      deletePhysicalMemory(valley.mpData, valley.mSize);
   }
   
   BASSERT(!mTotalPhysAllocations && !mTotalPhysBytes);
   
   mValleyHashMap.clear();
   
   mValleyClasses.clear();
   
   mRegionHeap.clear();
   
   mUnusedRegionBuf.clear();
   
   mRegionPages.clear();
   
   mUnusedRegionBufLargestUnused = 0;
   mTotalPhysAllocations = 0;
   mTotalPhysBytes = 0;
   mRegionHeapMaxUnusedSize = 0;
}

void BXboxTextureHeap::getTextureRegions(const IDirect3DTexture9* pHeader, BDynamicCHeapArray<uint>& regions, uint& allocSize, uint& unusedSize)
{
   enum { cMaxBaseRegions = 256, cMaxMipRegions = 256 };

   XGLAYOUT_REGION baseRegions[cMaxBaseRegions];
   UINT baseRegionCount = cMaxBaseRegions;
   
   XGLAYOUT_REGION mipRegions[cMaxMipRegions];
   UINT mipRegionCount = cMaxMipRegions;
      
   UINT baseData = 0;
   UINT baseSize = 0;
   UINT mipData = 0;
   UINT mipSize = 0;
   
   XGGetTextureLayout((D3DBaseTexture*)pHeader, &baseData, &baseSize, baseRegions, &baseRegionCount, 8, &mipData, &mipSize, mipRegions, &mipRegionCount, 8);

   allocSize = mipSize + baseSize;

   unusedSize = allocSize;
            
   BRegionAllocator::BReservedRegion reservedRegions[cMaxBaseRegions + cMaxMipRegions];
   uint numReservedRegions = 0;
   
   for (uint i = 0; i < baseRegionCount; i++)
   {
      const uint size = baseRegions[i].EndOffset - baseRegions[i].StartOffset;
      unusedSize -= size;
      reservedRegions[numReservedRegions++].set(baseRegions[i].StartOffset, baseRegions[i].EndOffset);
   }
   
   for (uint i = 0; i < mipRegionCount; i++)
   {
      const uint size = mipRegions[i].EndOffset - mipRegions[i].StartOffset;
      unusedSize -= size;
      reservedRegions[numReservedRegions++].set(mipRegions[i].StartOffset + baseSize, mipRegions[i].EndOffset + baseSize);
   }
   
   // This uses the primary heap!         
   BRegionAllocator regionAlloc;
   bool success = regionAlloc.init((void*)0x10000, allocSize, reservedRegions, numReservedRegions);
   BVERIFY(success);
   
   regions.pushBack(regionAlloc.getRegions().getPtr(), regionAlloc.getRegions().getSize());
}

uint BXboxTextureHeap::getOrCreateValleyClass(BDynamicCHeapArray<uint>& regions, uint allocSize, uint unusedSize)
{
   uint classIndex;

   for (classIndex = 0; classIndex < mValleyClasses.getSize(); classIndex++)
   {
      const BValleyClass& valleyClass = mValleyClasses[classIndex];

      if ((valleyClass.mAllocSize == allocSize) && (valleyClass.mUnusedSize == unusedSize) && (valleyClass.mRegions == valleyClass.mRegions))
         break;
   }

   if (classIndex == mValleyClasses.getSize())
   {
      BValleyClass& newClass = *mValleyClasses.enlarge(1);
      newClass.mAllocSize = allocSize;
      newClass.mUnusedSize = unusedSize;
      newClass.mRegions.swap(regions);
   }
   
   return classIndex;
}

bool BXboxTextureHeap::reserveValleys(uint num, uint width, uint height, uint depth, uint levels, D3DFORMAT fmt, D3DRESOURCETYPE textureType)
{
   BScopedCriticalSection lock(mValleyMutex);
   
   IDirect3DTexture9 header;
   Utils::ClearObj(header);
   uint baseSize = 0;
   uint mipSize = 0;
   
   switch (textureType)
   {
      case D3DRTYPE_TEXTURE:
      {
         XGSetTextureHeader(width, height, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, &header, &baseSize, &mipSize);
         break;
      }
      case D3DRTYPE_VOLUMETEXTURE:
      {
         XGSetVolumeTextureHeader(width, height, depth, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, (D3DVolumeTexture*)&header, &baseSize, &mipSize);
         break;
      }
      case D3DRTYPE_CUBETEXTURE:
      {
         BASSERT(width == height);
         XGSetCubeTextureHeader(width, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, (D3DCubeTexture*)&header, &baseSize, &mipSize);
         break;
      }
      case D3DRTYPE_ARRAYTEXTURE:
      {
         XGSetArrayTextureHeader(width, height, depth, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, (D3DArrayTexture*)&header, &baseSize, &mipSize);
         break;
      }
      default:
      {
         BASSERT(0);
         return false;
      }
   }
   
   BDynamicCHeapArray<uint> regions;
   uint allocSize, unusedSize;
   getTextureRegions(&header, regions, allocSize, unusedSize);
         
   const uint classIndex = getOrCreateValleyClass(regions, allocSize, unusedSize);
               
   for (uint i = 0; i < num; i++)
      if (!createNewValley(classIndex, cVSAvailable))
         return false;
   
   return true;
}

void* BXboxTextureHeap::createNewValley(uint valleyClassIndex, eValleyState initialState)
{
   BValleyClass& valleyClass = mValleyClasses[valleyClassIndex];
   
   void* p = allocatePhysicalMemory(valleyClass.mAllocSize);
   if (!p)
      return NULL;

   BValley valley;

   BDEBUG_ASSERT(valleyClassIndex <= UINT16_MAX);
   valley.mClassIndex = static_cast<uint16>(valleyClassIndex);
   valley.mState = initialState;
   valley.mpData = p;
   valley.mSize = valleyClass.mAllocSize;
   valley.mUnusedRegionsInUse = false;
   
   BValleyHashMap::InsertResult insertRes(mValleyHashMap.insert(p, valley));
   BASSERT(insertRes.second);

   if (initialState == cVSAvailable)
   {
      valleyClass.mFreePtrs.pushBack(p);
      
      queueUnusedRegions(valley);
   }
   else
   {
      valleyClass.mUsedPtrs.pushBack(p);
   }

   return p;      
}   

void BXboxTextureHeap::queueUnusedRegions(const BValley& valley)
{
   const BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];
   
   uint largestUnusedSize = 0;
   
   const BDynamicCHeapArray<uint>& regionSizes = valleyClass.mRegions;
   
   uint curOfs = 0;
   for (uint i = 0; i < regionSizes.getSize(); i++)
   {
      const uint regionSize = regionSizes[i] & BRegionAllocator::cSizeMask;
      
      if (regionSizes[i] & BRegionAllocator::cFreeFlag)
      {
         largestUnusedSize = Math::Max(largestUnusedSize, regionSize);
         
         mUnusedRegionBuf.pushBack(BUnusedRegion(static_cast<uchar*>(valley.mpData) + curOfs, regionSize));         
      }
      
      curOfs += regionSize;
   }
   
   mUnusedRegionBufLargestUnused = Math::Max(mRegionHeapMaxUnusedSize, largestUnusedSize);
}

void* BXboxTextureHeap::getValley(const IDirect3DTexture9* pTex, uint* pAllocSize)
{
   if (!pTex)
      return NULL;
   
   if (pTex->Common & D3DCOMMON_D3DCREATED)  
      return false;
      
   if ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0)
      return false;
      
   BScopedCriticalSection lock(mValleyMutex);
   
   BDynamicCHeapArray<uint> regions;
   uint allocSize, unusedSize;
   getTextureRegions(pTex, regions, allocSize, unusedSize);
   if (pAllocSize) *pAllocSize = allocSize;
   
   const uint classIndex = getOrCreateValleyClass(regions, allocSize, unusedSize);

   BValleyClass& valleyClass = mValleyClasses[classIndex];      
   
   void* p;
   if (valleyClass.mFreePtrs.getSize())
   {
      p = valleyClass.mFreePtrs.back();
      
      valleyClass.mFreePtrs.popBack();
      valleyClass.mUsedPtrs.pushBack(p);
   }
   else
   {
      p = createNewValley(classIndex, cVSLocked);
      
      if (!p)
         return NULL;
   }
         
   BValleyHashMap::iterator it = mValleyHashMap.find(p);
   BDEBUG_ASSERT(it != mValleyHashMap.end());
   BDEBUG_ASSERT(it->first == p);
   
   BValley& newValley = it->second;
   BDEBUG_ASSERT(newValley.mSize == allocSize);
   
   newValley.mState = cVSLocked;

   return p;
}

bool BXboxTextureHeap::unlockValley(const IDirect3DTexture9* pTex)
{
   if (!pTex)
      return false;
   
   if (pTex->Common & D3DCOMMON_D3DCREATED)  
      return false;

   if ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0)
      return false;
   
   void* pPhysicalAlloc = reinterpret_cast<void*>(reinterpret_cast<const D3DBaseTexture*>(pTex)->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT);
   if (pPhysicalAlloc < pPhysHeapStart)
      return false;
            
   BScopedCriticalSection lock(mValleyMutex);
         
   BValleyHashMap::iterator it = mValleyHashMap.find(pPhysicalAlloc);
   if (it == mValleyHashMap.end())
      return false;
      
   BDEBUG_ASSERT(it->first == pPhysicalAlloc);

   BValley& valley = it->second;

   if (valley.mState != cVSLocked)
      return false;
      
   valley.mState = cVSOccupied;
   
   if (!valley.mUnusedRegionsInUse)
   {
      valley.mUnusedRegionsInUse = true;
      queueUnusedRegions(valley);
   }
   
   return true;
}

bool BXboxTextureHeap::releaseValley(const IDirect3DTexture9* pTex)
{
   if (!pTex)
      return false;

   if (pTex->Common & D3DCOMMON_D3DCREATED)  
      return false;

   if ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0)
      return false;

   void* pPhysicalAlloc = reinterpret_cast<void*>(reinterpret_cast<const D3DBaseTexture*>(pTex)->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT);
   if (pPhysicalAlloc < pPhysHeapStart)
      return false;
      
   BScopedCriticalSection lock(mValleyMutex);

   BValleyHashMap::iterator it = mValleyHashMap.find(pPhysicalAlloc);
   if (it == mValleyHashMap.end())
      return false;

   BDEBUG_ASSERT(it->first == pPhysicalAlloc);

   BValley& valley = it->second;

   if (valley.mState == cVSAvailable)
      return false;
   
   if (!valley.mUnusedRegionsInUse)
   {
      valley.mUnusedRegionsInUse = true;
      queueUnusedRegions(valley);
   }
   
   valley.mState = cVSAvailable;
   
   BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];
   
   bool foundPtr = valleyClass.mUsedPtrs.removeValue(pPhysicalAlloc, false);
   foundPtr;
   BDEBUG_ASSERT(foundPtr);
   
   valleyClass.mFreePtrs.pushBack(pPhysicalAlloc);
   
   return true;
}

void BXboxTextureHeap::flushUnusedRegions()
{
   BScopedCriticalSection lock(mValleyMutex);
   
   for (uint i = 0; i < mUnusedRegionBuf.getSize(); i++)
   {
      const BUnusedRegion& region = mUnusedRegionBuf[i];
      
      bool success = mRegionHeap.addRegion(region.mpStart, region.mSize);
      BVERIFY(success);
      
      mRegionHeapMaxUnusedSize = Math::Max(mRegionHeapMaxUnusedSize, region.mSize);
   }
   
   mUnusedRegionBuf.resize(0);
   
   mUnusedRegionBufLargestUnused = 0;
}

const uint cMaxAllocSize = 16384;

void* BXboxTextureHeap::alloc(uint size, uint* pActualSize)
{
   if (size < sizeof(uint))
      size = sizeof(uint);

   size = Utils::AlignUpValue(size, sizeof(uint));
   
   uint rawSize = size;

   if (rawSize > cMaxAllocSize)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
            
   uint actualRawSize;         
   void* p = mRegionHeap.alignedAlloc(rawSize, &actualRawSize);
   if (!p)
   {
      if (static_cast<uint>(mUnusedRegionBufLargestUnused) >= rawSize)
         flushUnusedRegions();
         
      p = mRegionHeap.alignedAlloc(rawSize, &actualRawSize);
   }
   
   if (!p)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
         
   const uint pageIndex = ((uint)p - (uint)pPhysHeapStart) >> 12;
   mRegionPages.setBit(pageIndex);
   
   return p;
}

bool BXboxTextureHeap::free(void* p)
{
   if (p < pPhysHeapStart)
      return false;
   
   const uint pageIndex = ((uint)p - (uint)pPhysHeapStart) >> 12;
   if (pageIndex >= cPhysHeapSizeInPages)
      return false;
      
   if (!mRegionPages.isBitSet(pageIndex))
      return false;
   
   return mRegionHeap.alignedFree(p);
}

uint BXboxTextureHeap::getSize(void* p)
{
   if (p < pPhysHeapStart)
      return 0;

   const uint pageIndex = ((uint)p - (uint)pPhysHeapStart) >> 12;
   if (pageIndex >= cPhysHeapSizeInPages)
      return 0;

   if (!mRegionPages.isBitSet(pageIndex))
      return 0;

   return mRegionHeap.alignedGetSize(p);
}

bool BXboxTextureHeap::check(void)
{
   return mRegionHeap.check();
}
