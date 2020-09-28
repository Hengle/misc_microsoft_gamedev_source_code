//============================================================================
//
//  File: ecfFileData.cpp
//
//  Copyright (c) 2006-2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "ecfFileData.h"

BECFFileData::BECFFileData() 
{
   Utils::ClearObj(mTotalAllocated);
}

BECFFileData::~BECFFileData()
{
   if (getValid())
      discardAllChunks();
}

void BECFFileData::clear(void)
{
   if (getValid())
      discardAllChunks();
      
   mChunkAllocations.clear();

   mFileStream.close();   
}

bool BECFFileData::load(BStream* pStream, bool ignoreAlignmentErrors, bool storePhysicalCacheLines, bool okayToAllocWriteCombinedAsCached)
{
   clear();
   
   if (!pStream)
      return false;
   
   if (!mFileStream.open(pStream))
   {
      mFileStream.close();
      return false;
   }
   
   mChunkAllocations.resize(mFileStream.getNumChunks());

   BDynamicArray<BYTE, 16, BDynamicArrayRenderHeapAllocator> buf;
   
   const uint cBufSize = 8192;
   
   for (uint i = 0; i < mFileStream.getNumChunks(); i++)
   {
      const BECFChunkHeader& chunkHeader = mFileStream.getChunkHeader(i);
      
      const uint chunkLen = getChunkDataLen(i);
      uint chunkAlign = getChunkDataAlignment(i);         
            
      if (!chunkLen)
         continue;

      BMemoryHeap* pHeap = &gRenderHeap;
      eChunkDataMemProtect protect = cCached;

      if (chunkHeader.getResourceBitFlag(cECFChunkResFlagContiguous))
      {
         if (chunkHeader.getResourceBitFlag(cECFChunkResFlagWriteCombined))
         {
            pHeap = &gPhysWriteCombinedHeap;
            protect = cPhysicalWriteCombined;
         }
         else
         {
            pHeap = &gPhysCachedHeap;
            protect = cPhysicalCached;
         }
      }
                 
      void* pAllocation = NULL;
                  
      if ((okayToAllocWriteCombinedAsCached) && (chunkHeader.getResourceBitFlag(cECFChunkResFlagContiguous)) && (chunkAlign <= 32))
      {
         // rg [1/26/08] - HACK HACK- change gr2ugx so it doesn't request unnecessarily large alignments for indices and vertices!      
         if (chunkAlign == 32) 
            chunkAlign = 16;
            
         pAllocation = gRenderHeap.AlignedNew(chunkLen, chunkAlign, NULL, false, true);
         if (pAllocation)
         {
            static void* const pPhysCachedHeapStart      = (void*)0xA0000000;
            static void* const pPhysCachedHeapEnd        = (void*)0xC0000000;
            BDEBUG_ASSERT((pAllocation >= pPhysCachedHeapStart) && (pAllocation < pPhysCachedHeapEnd));
            
            pHeap = &gRenderHeap;      
            protect = cPhysicalCached;
         }
      }
      else if (chunkAlign > pHeap->getMaxSupportedAlignment())
      {
         if (!ignoreAlignmentErrors)
         {
            clear();
            return false;
         }
         
         chunkAlign = pHeap->getMaxSupportedAlignment();
      }

      if (!pAllocation)                  
      {
         pAllocation = pHeap->AlignedNew(chunkLen, chunkAlign);
         if (!pAllocation)
         {
            clear();
            return false;
         }
      }         
      
      BDEBUG_ASSERT(Utils::IsAligned(pAllocation, chunkAlign));
      
      mChunkAllocations[i].mpData = pAllocation;
      mChunkAllocations[i].mpHeap = pHeap;
      mChunkAllocations[i].mProtect = protect;
      
      if (!mFileStream.seekToChunk(i, true))
      {
         clear();
         return false;
      }
                  
      if (cPhysicalWriteCombined == protect)
      {
         if (buf.empty())
            buf.resize(cBufSize);
      
         uint bytesLeft = chunkLen;
         uint curOfs = 0;
         
         do
         {
            const uint bytesToRead = Math::Min(cBufSize, bytesLeft);
            
            if (bytesToRead != pStream->readBytes(buf.getPtr(), bytesToRead))
            {
               clear();
               return false;
            }  
            
            memcpy(static_cast<uchar*>(pAllocation) + curOfs, buf.getPtr(), bytesToRead);
                        
            bytesLeft -= bytesToRead;
            curOfs += bytesToRead;
         } while (bytesLeft);     
      }
      else
      {
         if (chunkLen != pStream->readBytes(pAllocation, chunkLen))
         {
            clear();
            return false;
         }
         
         if ((storePhysicalCacheLines) && (cPhysicalCached == protect))
         {
            Utils::StoreCacheLines(pAllocation, chunkLen);
         }
      }
      
      int actualAllocated = 0;
      bool success = pHeap->Details(pAllocation, &actualAllocated);
      success;
      BDEBUG_ASSERT(success);
      
      mTotalAllocated[protect] += actualAllocated;
   }
   
   mFileStream.closeStream();
   
   return true;
}

DWORD BECFFileData::getHeaderID(void) const
{
   if (!getValid())
      return 0;
      
   return mFileStream.getHeader().getID();
}

bool BECFFileData::getValid(void) const
{
   return mFileStream.getValid();
}

int BECFFileData::findChunkByID(uint64 ID, uint startIndex) const
{
   BDEBUG_ASSERT(getValid());
   return mFileStream.findChunkByID(ID, startIndex);
}

uint BECFFileData::getNumChunks(void) const
{
   BDEBUG_ASSERT(getValid());
   return mFileStream.getNumChunks();
}

uint BECFFileData::getChunkDataLen(uint chunkIndex) const
{
   BDEBUG_ASSERT(getValid());
   return mFileStream.getChunkHeader(chunkIndex).getSize();
}

int BECFFileData::getChunkDataLenByID(uint64 ID) const
{
   BDEBUG_ASSERT(getValid());
   
   int chunkIndex = findChunkByID(ID);
   if (cInvalidIndex == chunkIndex)
      return -1;
   
   return mFileStream.getChunkHeader(chunkIndex).getSize();
}

uint BECFFileData::getChunkDataAlignment(uint chunkIndex) const
{
   BDEBUG_ASSERT(getValid());
   return 1U << mFileStream.getChunkHeader(chunkIndex).getAlignmentLog2();
}

void* BECFFileData::getChunkDataPtr(uint chunkIndex) const
{
   BDEBUG_ASSERT(getValid());
   return mChunkAllocations[chunkIndex].mpData;
}

void* BECFFileData::getChunkDataPtrByID(uint64 ID) const
{
   BDEBUG_ASSERT(getValid());
   
   int chunkIndex = findChunkByID(ID);
   if (cInvalidIndex == chunkIndex)
      return NULL;
      
   return mChunkAllocations[chunkIndex].mpData;
}

void BECFFileData::discardChunkData(uint chunkIndex) 
{
   BDEBUG_ASSERT(getValid());
   
   void* p = mChunkAllocations[chunkIndex].mpData;
   
   if (!p)
      return;
         
   bool success = getChunkDataHeap(chunkIndex)->Delete(p);
   success;
   BDEBUG_ASSERT(success);
   
   mChunkAllocations[chunkIndex].mpData = NULL;
   
   mTotalAllocated[getChunkDataMemProtect(chunkIndex)] -= getChunkDataLen(chunkIndex);
}

void BECFFileData::acquireChunkDataOwnership(uint chunkIndex)
{
   BDEBUG_ASSERT(getValid());
   
   if (mChunkAllocations[chunkIndex].mpData)
   {
      mChunkAllocations[chunkIndex].mpData = NULL;
   
      mTotalAllocated[getChunkDataMemProtect(chunkIndex)] -= getChunkDataLen(chunkIndex);
   }
}

void BECFFileData::discardAllChunks(void)
{
   BDEBUG_ASSERT(getValid());
   
   for (uint i = 0; i < mChunkAllocations.getSize(); i++)
      discardChunkData(i);
}

BECFFileData::eChunkDataMemProtect BECFFileData::getChunkDataMemProtect(uint chunkIndex) const
{
   BDEBUG_ASSERT(getValid());
   
   return mChunkAllocations[chunkIndex].mProtect;
}

BMemoryHeap* BECFFileData::getChunkDataHeap(uint chunkIndex) const
{
   BDEBUG_ASSERT(getValid());
   
   return mChunkAllocations[chunkIndex].mpHeap;
}


