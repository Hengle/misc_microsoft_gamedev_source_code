//============================================================================
//  File: buffercache.h
//
//  Copyright (c) 2007, Ensemble Studios
//============================================================================
#pragma once

//============================================================================
// class BBufferCache
//============================================================================
class BBufferCache
{
   public:
      enum { cDefaultCacheSize = 8192, };

      BBufferCache(uint cacheSize = cDefaultCacheSize);

      // Write focused methods
      uint32 writeToCache(const void* p, uint n);
      uint getCacheBytesAllocated() const { return mWriteIndex; }
      uint getCacheBytesFree() const { return mCache.getSizeInBytes() - mWriteIndex; }
      bool isCacheFull() const;
      bool isCacheEmpty() const;

      // Read focused methods
      uint32 readFromCache(void* pBuffer, uint amtToRead, uint& actual);
      //uint bytesAvailToRead() { return mNumBytesInBuffer - mReadIndex; }
      int32 bytesAvailToRead() { return mWriteIndex - mReadIndex; }
      bool atEOF() { return mEOF; }
      void setReadIndex(int readIndex) { BDEBUG_ASSERT(mReadIndex <= mWriteIndex); mReadIndex = readIndex; }

      // these are set on the filling of the cache
      void setEOF(bool eof) { mEOF = eof; }
      //void setBytesInBuffer(uint bytesInBuffer) { mNumBytesInBuffer = bytesInBuffer; }
      void setBytesInBuffer(int32 bytesInBuffer) { BDEBUG_ASSERT(mWriteIndex <= static_cast<int32>(mCache.getSizeInBytes())); mWriteIndex = bytesInBuffer; }

      // Common methods
      uint getCacheSize() const { return mCache.getSizeInBytes(); }
      void setFilePointer(uint64 filepointer) { mFileOfs = filepointer; }
      uint64 getFilePointer() const { return mFileOfs; }
      void resetCache();

      byte* getPtr() { return mCache.getPtr() + mReadIndex; }

   protected:
      bool           mEOF;                // flag to indicate this buffer represents an EOF
      //int            mNumBytesInBuffer;   // # of valid bytes for a read
      int32          mReadIndex;          // index into the buffer for the next read

      int64          mFileOfs;            // file offset 

      // Cache 
      int32          mWriteIndex;         // pointer into the buffer to write to
      BByteArray     mCache;              // buffer of bytes

}; // class BBufferCache
