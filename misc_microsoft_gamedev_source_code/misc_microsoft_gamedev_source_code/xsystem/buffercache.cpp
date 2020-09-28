//============================================================================
//  File: buffercache.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "xsystem.h"
#include "buffercache.h"
  
//============================================================================
//============================================================================
BBufferCache::BBufferCache(uint cacheSize) :
   mFileOfs(0),
   mWriteIndex(0),
//   mNumBytesInBuffer(0),
   mReadIndex(0),
   mEOF(false)
{
   mCache.resize(cacheSize);
}

//============================================================================
//============================================================================
uint32 BBufferCache::writeToCache(const void* p, uint n)
{
   // can we add more in?
   if (isCacheFull())
      return 0;

   uint bytesToCopy = Math::Min<uint>(n, getCacheBytesFree() );

   // Copy to the cache, update the cache offset
   memcpy(mCache.getPtr() + mWriteIndex, p, bytesToCopy);
   mWriteIndex += bytesToCopy;

   return bytesToCopy;
}

//============================================================================
//============================================================================
bool BBufferCache::isCacheFull() const
{
   if (mWriteIndex == (int)mCache.getSizeInBytes())
      return true;

   return false;
}

//============================================================================
//============================================================================
bool BBufferCache::isCacheEmpty() const
{
   if (mWriteIndex != 0)
      return false;

   return true;
}

//============================================================================
//============================================================================
void BBufferCache::resetCache()
{
   mWriteIndex=0;
   //mNumBytesInBuffer=0;
   mReadIndex=0;
}

//============================================================================
//============================================================================
uint32 BBufferCache::readFromCache(void* pBuffer, uint amtToRead, uint& actual)
{
   uint amount = min(static_cast<uint>(bytesAvailToRead()), amtToRead);

   if (amount > 0)
      memcpy(pBuffer, mCache.getPtr() + mReadIndex, amount);

   actual = amount;              // return the amount we actually read
   mReadIndex += amount;         // move our index along
   return amount;
}
