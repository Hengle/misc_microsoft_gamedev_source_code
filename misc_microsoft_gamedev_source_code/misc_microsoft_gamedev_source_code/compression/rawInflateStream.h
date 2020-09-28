//============================================================================
//
//  rawInflateStream.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once

#include "stream\stream.h"
#include "inflate.h"
#include "hash\tiger.h"

//============================================================================
// class BRawInflateStream
//============================================================================
class BRawInflateStream : public BStream
{
public:
   enum
   {  
      cNoError = 0,
      cSrcStreamError,
      cInflateError,
      cSrcStreamTooSmallError,
      cSrcStreamCorruptError,
      cDecompStreamTooSmallError
   };

   BRawInflateStream() :
      BStream(),
      mpSrcStream(NULL),
      mTotalBytesWritten(0),
      mTotalBytesRead(0),
      mErrorStatus(cNoError),
      mInflErrorStatus(0),
      mpInBuf(NULL),
      mInBufSize(0),
      mInBufOfs(0),
      mInBufEOFFlag(false),
      mOpened(false),
      mStreamTerminated(false),
      mCompSize(0),
      mDecompSize(0),
      mComputeDecompTiger(false),
      mInputBuffering(false),
      mDecompTigerValid(false),
      mpOutBuf(NULL),
      mFileOfs(0)
   {
   }

   BRawInflateStream(BStream& srcStream, uint64 compSize, uint64 decompSize, bool computeDecompTiger, uint flags = cSFReadable, const BString& name = "") :
      BStream(),
      mpSrcStream(NULL),
      mTotalBytesWritten(0),
      mTotalBytesRead(0),
      mErrorStatus(cNoError),
      mInflErrorStatus(0),
      mpInBuf(NULL),
      mInBufSize(0),
      mInBufOfs(0),
      mInBufEOFFlag(false),
      mOpened(false),
      mStreamTerminated(false),
      mCompSize(0),
      mDecompSize(0),
      mComputeDecompTiger(false),
      mInputBuffering(false),
      mDecompTigerValid(false),
      mpOutBuf(NULL),
      mFileOfs(0)
   {
      open(srcStream, compSize, decompSize, computeDecompTiger, flags, name);
   }
   
   ~BRawInflateStream()
   {
      close();
   }

   bool open(BStream& srcStream, uint64 compSize, uint64 decompSize, bool computeDecompTiger, uint flags = cSFReadable, const BString& name = "")
   {
      close();
      
      if ((flags & cSFReadable) == 0)
         return false;
         
      if (flags & cSFSeekable)
      {
         const uint cMaxDecompBufSize = 512U * 1024U * 1024U;
         if (decompSize > cMaxDecompBufSize)
            return false;
            
         mpOutBuf = gRenderHeap.New((uint)decompSize);
         if (!mpOutBuf)
            return false;
      }
   
      mInflate.init();

      setName(name);
      setFlags(flags);

      mpSrcStream = &srcStream;
      mTotalBytesWritten = 0;
      mTotalBytesRead = 0;
      mErrorStatus = cNoError;
      mInflErrorStatus = 0;
      mpInBuf = NULL;
      mInBufSize = 0;
      mInBufOfs = 0;
      mInBufEOFFlag = false;
      mOpened = false;
      mStreamTerminated = false;
      mTigerHash.clear();
      mDecompTigerValid = false;
      mFileOfs = 0;
      mDecompSize = decompSize;
      mCompSize = compSize;
      
      if (mComputeDecompTiger)
         mTigerHashGen.clear();
                                          
      if (mpSrcStream->sizeKnown())
      {
         if (mpSrcStream->bytesLeft() < mCompSize)
            mErrorStatus = cSrcStreamTooSmallError;
      }
            
      mpInBuf = NULL;
      if (mCompSize < UINT_MAX)
         mpInBuf = static_cast<const BYTE*>(mpSrcStream->ptr((uint)mCompSize));

      if (mpInBuf)
      {
         mInputBuffering = false;
         mInBufSize = (uint)mCompSize;
         mInBufEOFFlag = true;
      }
      else
      {
         mInputBuffering = true;
         mInBuf.resize(cBufSize);
         mpInBuf = mInBuf.getPtr();
      }
                        
      if (cNoError == mErrorStatus)
         mOpened = true;
      else
         close();

      return !errorStatus();
   }

   virtual uint64 size(void) const
   {
      if (!mOpened)
         return 0;
      
      return mDecompSize;
   }

   // true on success
   virtual bool close(void)
   {
      if (mpOutBuf)
      {
         bool success = gRenderHeap.Delete(mpOutBuf);
         success;
         BDEBUG_ASSERT(success);
         
         mpOutBuf = NULL;
      }
      
      if (!mOpened)
         return true;

      mOpened = false;
            
      return termStream();
   }

   virtual uint readBytes(void* p, uint n)
   {
      if (!mOpened)
         return 0;
         
      n = (uint)Math::Min<uint64>(n, mDecompSize - mFileOfs);
      if (!n)
         return 0;
         
      if (!seekable())
      {
         const uint bytesDecompressed = decompressBytes(p, n);
         mFileOfs += bytesDecompressed;
         return bytesDecompressed;
      }
      
      BDEBUG_ASSERT(mpOutBuf);
                       
      BYTE* pDst = static_cast<BYTE*>(p);
      
      uint totalBytesWritten = 0;

      for ( ; ; )
      {
         if (mFileOfs < mTotalBytesWritten)
         {
            const uint bytesToCopy = (uint)Math::Min<uint64>(mTotalBytesWritten - mFileOfs, n);
            
            Utils::FastMemCpy(pDst, (const BYTE*)mpOutBuf + mFileOfs, bytesToCopy);
            
            pDst              += bytesToCopy;
            n                 -= bytesToCopy;
            
            mFileOfs          += bytesToCopy;
            totalBytesWritten += bytesToCopy;
            
            if (!n)
               break;
         }
         
         BDEBUG_ASSERT(mFileOfs >= mTotalBytesWritten);
         
         uint64 bytesToDecompress = (mFileOfs + n) - mTotalBytesWritten;
         bytesToDecompress = Math::Max<uint64>(8192U, bytesToDecompress);
         bytesToDecompress = Math::Min<uint64>(bytesToDecompress, mDecompSize - mTotalBytesWritten);
         
         uint decompressedBytes = decompressBytes((BYTE*)mpOutBuf + mTotalBytesWritten, (uint)bytesToDecompress);
         if (decompressedBytes != bytesToDecompress)
            break;
      }      
      
      return totalBytesWritten;
   }

   virtual uint writeBytes(const void* p, uint n)
   {
      p;
      n;
      return 0;
   }

   virtual uint64 curOfs(void) const
   {
      if (!mOpened)
         return 0;

      return mFileOfs;
   }

   virtual uint64 bytesLeft(void) const
   {
      return mDecompSize - mFileOfs;
   }
   
   virtual int64 seek(int64 ofs, bool absolute = true)
   {
      int64 newFileOfs = absolute ? ofs : (mFileOfs + ofs);
      
      if (newFileOfs < 0)
         newFileOfs = 0;
      else if (newFileOfs > (int64)mDecompSize)
         newFileOfs = mDecompSize;

      if (seekable())
      {
         mFileOfs = newFileOfs;
      }
      else
      {
         if (newFileOfs < (int64)mFileOfs)
            return -1;
         else if (newFileOfs > (int64)mFileOfs)
         {
            const uint64 bytesToSkip = newFileOfs - mFileOfs;
            const uint64 totalBytesSkipped = skipBytes(bytesToSkip);
            
            if (bytesToSkip != totalBytesSkipped)
               return -1;
         }
      }
      
      return mFileOfs;
   }
   
   virtual bool errorStatus(void) const { return mErrorStatus != cNoError; }

   virtual bool setWritable(bool f) { f; return false; }
   virtual bool setSeekable(bool f) { f; return false; }
   
   BStream* getSrcStream(void) const { return mpSrcStream; }

   uint64 getDecompSize(void) const { return mDecompSize; }
   uint64 getCompSize(void) const { return mCompSize; }

   const BTigerHash* getDecompTigerHash(void) const { if (!mDecompTigerValid) return NULL; return &mTigerHash; }

   int getInflateErrorValue(void) const { return mInflErrorStatus; }
   int getErrorStatusValue(void) const { return mErrorStatus; }

   BInflate& getInflate(void) { return mInflate; }

private:
   BRawInflateStream(const BRawInflateStream& b);
   BRawInflateStream& operator= (const BRawInflateStream& rhs);

   BInflate mInflate;
   
   BTigerHashGen mTigerHashGen;
   BTigerHash mTigerHash;
   
   uint64 mCompSize;
   uint64 mDecompSize;

   BStream* mpSrcStream;
   
   uint64 mTotalBytesRead;
   uint64 mTotalBytesWritten;
   
   int mErrorStatus;
   int mInflErrorStatus;

   enum { cBufSize = 8192 };
   
   BByteArray mInBuf;
   
   const BYTE* mpInBuf;
   uint mInBufSize;
   uint mInBufOfs;
   
   void* mpOutBuf;
   uint64 mFileOfs;
      
   bool mInBufEOFFlag         : 1;
   bool mOpened               : 1;
   bool mInputBuffering       : 1;
   bool mStreamTerminated     : 1;
   bool mComputeDecompTiger   : 1;
   bool mDecompTigerValid     : 1;
   
   // true on success
   bool refreshInputBufferSizeKnown(void)
   {
      if (mInputBuffering)
      {
         mInBufOfs = 0;
         mInBufSize = (uint)Math::Min<uint64>(mCompSize - mTotalBytesRead, cBufSize);

         if (mInBufSize != mpSrcStream->readBytes((BYTE*)mpInBuf, mInBufSize))
         {  
            mErrorStatus = cSrcStreamError;
            return false;
         }
         
         mInBufEOFFlag = (mTotalBytesRead + (mInBufSize - mInBufOfs)) == mCompSize;
      }
                        
      return true;
   }
   
   bool termStream(void)
   {
      mStreamTerminated = true;
      return true;
   }

   uint decompressBytes(void* p, uint n)
   {
      BDEBUG_ASSERT(p);

      n = (uint)Math::Min<uint64>(n, mDecompSize - mTotalBytesWritten);

      if (!n)
         return 0;

      if ((cNoError != mErrorStatus) || (mStreamTerminated) || (!mpSrcStream))
         return 0;

      if (mpSrcStream->errorStatus())
      {
         mErrorStatus = cSrcStreamError;
         return 0;
      }

      // disable buffering if the client is decompressing the entire stream in one shot
      if ((mTotalBytesWritten == 0) && (n == mDecompSize))
         mInflate.setBuffered(false);

      uint outBufOfs = 0;
      while (outBufOfs < n)
      {
         if (mInBufOfs == mInBufSize)
         {
            if (!refreshInputBufferSizeKnown())
               return 0;
         }

         int inBufBytes = mInBufSize - mInBufOfs;
         int outBufBytes = n - outBufOfs;

         mInflErrorStatus = mInflate.decompress(
            mpInBuf + mInBufOfs, &inBufBytes,
            static_cast<uchar*>(p) + outBufOfs, &outBufBytes,
            mInBufEOFFlag);
         
         mTotalBytesRead += inBufBytes;
         mTotalBytesWritten += outBufBytes;
         
         if (mComputeDecompTiger)
         {
            mTigerHashGen.update(static_cast<uchar*>(p) + outBufOfs, outBufBytes);
            
            if (mTotalBytesWritten == mDecompSize)
            {
               mTigerHash = mTigerHashGen.finalize();
               mDecompTigerValid = true;
            }
         }
         
         mInBufOfs += inBufBytes;
         outBufOfs += outBufBytes;

         if (mInflErrorStatus < 0)
         {
            mErrorStatus = cInflateError;
            return 0;
         }

         if (INFL_STATUS_DONE == mInflErrorStatus)
         {
            if (outBufOfs != n)
            {
               mErrorStatus = cDecompStreamTooSmallError;
               return 0;
            }

            if (!termStream())
               return 0;
         }
      }

      if (mTotalBytesWritten == mDecompSize)
      {
         if (!termStream())
            return 0;
      }

      return outBufOfs;
   }
  
};
