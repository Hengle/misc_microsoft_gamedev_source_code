//============================================================================
//
//  xboxTextureHeap.h
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma once
#include "containers\sparseBitArray.h"
#include "containers\hashMap.h"
#include "memory\bfHeapAllocator.h"

class BXboxTextureHeap;

class BXboxTextureHeapMemoryHeapInterceptor : public BMemoryHeapInterceptor
{
   BXboxTextureHeap* mpHeap;

public:
   BXboxTextureHeapMemoryHeapInterceptor(BXboxTextureHeap* pHeap = NULL) : mpHeap(pHeap) { }

   virtual ~BXboxTextureHeapMemoryHeapInterceptor() { }
   
   void setXboxTextureHeap(BXboxTextureHeap* pHeap) { mpHeap = pHeap; }
   BXboxTextureHeap* getXboxTextureHeap() const { return mpHeap; }

   virtual void* New(int size, int* pActualSize, bool zero);
   
   virtual bool Delete(void* pData, int size);

   virtual bool Details(void* pData, int* pSize);

   virtual bool verify(void);
};

class BXboxTextureHeap
{
public:
   BXboxTextureHeap();
   ~BXboxTextureHeap();
   
   void init(bool enableHeapInterception, bool willBeAllocatingMipsOnly);
   void deinit();
   
   bool getInitialized() const { return mInitialized; }
         
   // The valley API's are thread safe - they lock the valley mutex.

   bool reserveValleys(uint num, uint width, uint height, uint depth, uint levels, D3DFORMAT fmt, D3DRESOURCETYPE textureType, bool mipsOnly = false);
   
   // freeUnusedValleys() is NOT thread safe vs. the alloc(), free(), or getSize() API's!
   // This alloc(), free(), and getSize() methods cannot be called while freeUnusedValleys() is active!
   void freeUnusedValleys(bool forcablyFreeReservedAllocations = false);
               
   // The texture header must have been created using XGSetTextureHeader(), etc. NOT by D3D. All 360 texture types are supported.
   // The usage must be D3DUSAGE_CPU_CACHED_MEMORY, and the mipoffset must be XGHEADER_CONTIGUOUS_MIP_OFFSET.
   // After calling getValley(), the caller can offset the resource using the returned pointer by calling XGOffsetResourceAddress().
   // It is up to the user to manage the CPU's cached view of the returned physical memory.
   void* getValley(const IDirect3DBaseTexture9* pTex, uint* pAllocSize, bool addUnusedRegionsToHeap, bool isLongTermAllocation, bool mipsOnly = false, uint* pActualUsedSize = NULL);
#ifndef BUILD_FINAL   
   bool setValleyTextureDetails(void* pPhysicalAlloc, const char* pManager, const char* pName, uint arrayIndex);
#endif   
   bool releaseValley(void* p);
   bool releaseValley(const IDirect3DBaseTexture9* pTex, bool mipsOnly = false);
   
   // The heap API is NOT thread safe. It is up to the caller to ensure this API is called in a thread safe manner!
   // These API's may occasionally take the valley mutex.
   
   // Supports 4, 8, and 16 byte alignment. size must be a multiple of the required alignment!
   void* alloc(uint size, uint* pActualSize = NULL);
   // Returns true if the block belonged to the texture manager and was successfully freed.
   bool free(void* p);
   
   // Returns the size of an allocated block, or 0 if the block was not allocated by this API.
   uint getSize(void* p);
   
   bool check() const;
         
   struct BValleyStats
   {
      uint mNumValleyClasses;
      uint mNumValleys;
      
      uint mNumAvailable;
      uint mTotalAvailableBytes;
                  
      uint mNumOccupied;
      uint mTotalOccupiedBytes;
      
      uint mTotalAllocatedBytes;
      uint mTotalRegionBytes;
      uint mLargestRegionSize;
      
      uint mTotalReserved;
      uint mTotalLongTerm;
      uint mTotalUsingUnusedRegions;
      uint mTotalOKToUseUnusedRegions;
            
      void clear() { Utils::ClearObj(*this); }
   };
   void getValleyStats(BValleyStats& stats) const;
   
   struct BHeapStats
   {
      uint mTotalRegionBytes;
      uint mTotalNumAllocations;
      uint mTotalBytesAllocated;
      uint mTotalBytesFree;
      uint mTotalFreeChunks;
      uint mLargestFreeChunk;
      uint mTotalBins;
      uint mTotalOverhead;
      uint mTotalOperations;
      uint mDVSize;

      void clear() { Utils::ClearObj(*this); }
   };
   void getHeapStats(BHeapStats& stats) const;

#ifndef BUILD_FINAL   
   void dumpValleyInfo(BStream* pStream);
#endif   
   
private:
   struct BValleyClass
   {
      uint                       mAllocSize;
      uint                       mUnusedSize;
      
      BDynamicCHeapArray<uint>   mRegions;
      
      BDynamicCHeapArray<void*>  mFreePtrs;
      BDynamicCHeapArray<void*>  mUsedPtrs;
      
      // This info is intended for informational/debugging purposes only.
      D3DFORMAT                  mFormat;
      uint16                     mWidth;
      uint16                     mHeight;
      uint16                     mDepth;
      uchar                      mLevels;
      uchar                      mResourceType;
      
      bool                       mMipsOnly;
      
#ifndef BUILD_FINAL
      struct BTextureDetails
      {
         BCRunTimeString   mManager;
         BCRunTimeString   mName;
         void*             mPtr;
         uint              mArrayIndex;
         
         bool operator== (const BTextureDetails& rhs) const { return (mManager == rhs.mManager) && (mName == rhs.mName) && (mPtr == rhs.mPtr) && (mArrayIndex == rhs.mArrayIndex); }
         
         bool operator< (const BTextureDetails& rhs) const 
         { 
            if (mManager < rhs.mManager) 
               return true;
            else if (mManager == rhs.mManager) 
               return mName < rhs.mName;
            return false;
         }
      };
      
      typedef BDynamicCHeapArray<BTextureDetails> BUsedTexturesArray;
      BUsedTexturesArray         mTextureDetails;
      
      BDynamicCHeapArray<BCRunTimeString> mManagers;
      void addTextureDetails(const BTextureDetails& details);
      bool removeTextureDetails(void* pPhysicalAlloc);
#endif      
      
      bool operator< (const BValleyClass& rhs) const;
   };
   
   enum eValleyState
   {
      cVSInvalid  = 0,
      cVSAvailable,
      cVSOccupied
   };
   
   struct BValley
   {
      uint16         mClassIndex;
      
      eValleyState   mState;
      
      void*          mpData;
      uint           mSize;
      
      bool           mUnusedRegionsInUse : 1;
      bool           mAddUnusedRegionsToHeap : 1;
      bool           mIsReservedAllocation : 1;
      bool           mIsLongTermAllocation : 1;
   };
   
   // Protected by mValleyMutex
   mutable BCriticalSection            mValleyMutex;
   
   BDynamicCHeapArray<BValleyClass>    mValleyClasses;
   
   typedef BHashMap<void*, BValley, BHasher<void*>, BEqualTo<void*>, true, BCRunTimeFixedHeapAllocator> BValleyHashMap;
   BValleyHashMap                      mValleyHashMap;
   
   struct BUnusedRegion
   {
      BUnusedRegion() { }
      BUnusedRegion(void* pStart, uint size) : mpStart(pStart), mSize(size) { }
      
      void* mpStart;
      uint mSize;
   };
   
   BDynamicCHeapArray<BUnusedRegion>   mUnusedRegionBuf;
   
   uint                                mUnusedRegionBufLargestUnused;
      
   // Unprotected - I'm assuming the caller will be locking before calling alloc or free
   BSparseBitArray                     mRegionPages;
   
   BBFHeapAllocator                    mRegionHeap;
   
   uint                                mRegionHeapMaxUnusedSize;
      
   uint                                mTotalPhysAllocations;
   uint                                mTotalPhysBytes;
   
   BXboxTextureHeapMemoryHeapInterceptor mMemoryHeapInterceptor;
   
   bool                                mInitialized;
   
   void*                               allocatePhysicalMemory(uint size);
   void                                deletePhysicalMemory(void* p, uint size);
   
   bool                                getTextureRegions(const IDirect3DBaseTexture9* pHeader, bool mipsOnly, BDynamicCHeapArray<uint>& regions, uint& allocSize, uint& unusedSize);
   uint                                getOrCreateValleyClass(const IDirect3DBaseTexture9* pTex, bool mipsOnly, BDynamicCHeapArray<uint>& regions, uint allocSize, uint unusedSize);
   void*                               createNewValley(uint valleyClassIndex, eValleyState initialState, bool isReservedAllocation, bool addUnusedRegionsToHeap, bool isLongTermAllocation);
   
   void                                queueUnusedRegions(BValley& valley);
   void                                flushUnusedRegions();
   void                                clearValleyTextureData(BValley& valley);
};

extern BXboxTextureHeap* gpXboxTextureHeap;