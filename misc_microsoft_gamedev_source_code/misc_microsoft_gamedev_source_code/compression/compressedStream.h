//============================================================================
//
//  compressedStream.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "stream\stream.h"
#include "hash\adler32.h"
#include "deflate.h"
#include "inflate.h"

const DWORD cDeflateStreamEndOfStreamMagic = 0xA5D91776;

#pragma pack(push)
#pragma pack(4)
class BDeflateStreamHeader
{
public:
   enum { cSig = 0xCC34EEAD, cInvertedSig = 0xADEE34CC };

   // mSig and mHeaderAdler32 must be first two DWORD's
   uint mSig;
   uint mHeaderAdler32;
               
   // mSrcBytes is the first member to be Adler32'd
   
   // uncompressed 
   uint64 mSrcBytes;
   uint mSrcAdler32;
   
   // compressed 
   uint64 mDstBytes;
   uint mDstAdler32;
   
   uint mHeaderType;

   BDeflateStreamHeader()
   {
      BCOMPILETIMEASSERT(sizeof(BDeflateStreamHeader) == 36);
      clear();
   }

   void clear(void)
   {
      Utils::ClearObj(*this);
      
      mSrcAdler32 = INIT_ADLER32;
      mDstAdler32 = INIT_ADLER32;
   }
   
   void initSig(void)
   {
      mSig = static_cast<uint>(cSig);
   }

   bool checkSig(void)
   {
      return mSig == cSig;
   }
   
   void endianSwap(void)
   {
      EndianSwitchWorker(this, this + 1, "iiqiqii");
   }
   
   bool isNativeEndian(void)
   {
      return mSig == cSig;
   }
   
   bool isBigEndian(void)
   {
      const bool nativeEndian = (mSig == cSig);
      const bool oppEndian = (mSig == cInvertedSig);
      
      if ((nativeEndian) && (cBigEndianNative))
         return true;
      else if ((oppEndian) && (cLittleEndianNative))
         return true;
         
      return false;
   }
   
   void convertToBigEndian(void)
   {
      if (!isBigEndian())
         endianSwap();
   }
   
   void convertToNativeEndian(void)
   {
      if (!isNativeEndian())
         endianSwap();
   }
   
   void computeAdler32(void)
   {
      convertToBigEndian();
      
      const uint adler32 = calcAdler32(&mSrcBytes, sizeof(BDeflateStreamHeader) - sizeof(uint) * 2);
      
      convertToNativeEndian();
      
      mHeaderAdler32 = adler32;
   }
   
   // true if header Adler32 is OK
   bool checkAdler32(void) 
   {
      convertToBigEndian();
      
      const uint adler32 = calcAdler32(&mSrcBytes, sizeof(BDeflateStreamHeader) - sizeof(uint) * 2);
      
      convertToNativeEndian();
      
      return adler32 == mHeaderAdler32;
   }

   friend BStream& operator<< (BStream& dst, const BDeflateStreamHeader& src)
   {
      return dst << src.mSig << src.mHeaderAdler32 << src.mHeaderType << src.mSrcBytes << src.mDstBytes << src.mSrcAdler32 << src.mDstAdler32;
   }

   friend BStream& operator>> (BStream& src, BDeflateStreamHeader& dst)
   {
      src >> dst.mSig >> dst.mHeaderAdler32 >> dst.mHeaderType >> dst.mSrcBytes >> dst.mDstBytes >> dst.mSrcAdler32 >> dst.mDstAdler32;
      
      if (src.errorStatus())
         dst.mSig = 0;
      
      return src;
   }
};
#pragma pack(pop)

//---------------------------------------------------------------------------------------------------------------------------------
// class BInflateStream
//---------------------------------------------------------------------------------------------------------------------------------
class BInflateStream : public BStream
{
public:
   enum
   {  
      eNoError                = 0,
      eSrcStreamError         = 1,
      eInflateError           = 2,
      eSrcStreamTooSmallError = 3,
      eSrcStreamCorruptError  = 4,
      eHeaderError            = 5,
      eCompAdler32Error       = 6,
      eDecompAdler32Error     = 7,
      eDecompressedStreamTooSmallError = 8
   };
   
   BInflateStream() :
      BStream(),
      mpSrcStream(NULL),
      mBytesWritten(0),
      mBytesRead(0),
      mReadAdler32(INIT_ADLER32),
      mWriteAdler32(INIT_ADLER32),
      mErrorStatus(eNoError),
      mInflErrorStatus(0),
      mInBufSize(0),
      mInBufOfs(0),
      mInBufEOFFlag(false),
      mOpened(false),
      mStreamTerminated(false)
   {
   }
   
   BInflateStream(BStream& srcStream, uint flags = cSFReadable, const BString& name = "") :
      BStream(),
      mpSrcStream(NULL),
      mBytesWritten(0),
      mBytesRead(0),
      mReadAdler32(INIT_ADLER32),
      mWriteAdler32(INIT_ADLER32),
      mErrorStatus(eNoError),
      mInflErrorStatus(0),
      mInBufSize(0),
      mInBufOfs(0),
      mInBufEOFFlag(false),
      mOpened(false),
      mStreamTerminated(false)
   {
      open(srcStream, flags, name);
   }
   
   bool open(BStream& srcStream, uint flags = cSFReadable, const BString& name = "")
   {
      if ((flags & cSFReadable) == 0)
         return false;
      
      // No need to close the old stream here.
      
      mInflate.init();
      
      setName(name);
      setFlags(flags);
      
      mpSrcStream = &srcStream;
      mBytesWritten = 0;
      mBytesRead = 0;
      mReadAdler32 = INIT_ADLER32;
      mWriteAdler32 = INIT_ADLER32;
      mErrorStatus = eNoError;
      mInflErrorStatus = 0;
      mInBufSize = 0;
      mInBufOfs = 0;
      mInBufEOFFlag = false;
      mOpened = false;
      mStreamTerminated = false;
         
      if (!readHeader())
      {
         mErrorStatus = eHeaderError;
      }
      else if (mSizeKnown)
      {
         // extra DWORD is the stream end magic value
         if (mpSrcStream->bytesLeft() < mHeader.mDstBytes + sizeof(DWORD))
            mErrorStatus = eSrcStreamTooSmallError;
      }  

      if (eNoError == mErrorStatus)
         mOpened = true;
      
      return !errorStatus();
   }
                           
   virtual uint64 size(void) const
   {
      if (!mOpened)
         return 0;
         
      if (!mSizeKnown)
         return BSTREAM_UNKNOWN_SIZE;
         
      return mHeader.mSrcBytes;
   }
   
   // true on success
   virtual bool close(void)
   {
      if (!mOpened)
         return true;
      
      mOpened = false;
                        
      if (mSizeKnown)
         return termStreamSizeKnown();
      else
         return termStreamSizeUnknown();
   }
               
   virtual uint readBytes(void* p, uint n)
   {
      if (!mOpened)
         return 0;

      if (mSizeKnown)
         return readBytesSizeKnown(p, n);
      else
         return readBytesSizeUnknown(p, n);
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
         
      return mBytesWritten;
   }

   // Returns BSTREAM_UNKNOWN_SIZE if the size of the uncompressed data is unknown.
   virtual uint64 bytesLeft(void) const
   {
      if (!mSizeKnown)
         return BSTREAM_UNKNOWN_SIZE; //0x7FFFFFFFFFFFFFFF;
                  
      if (mBytesWritten > mHeader.mSrcBytes)
         return 0;
         
      return mHeader.mSrcBytes - mBytesWritten;
   }

   // Seeking isn't supported, by you can always call BStream's skipBytes().
   virtual int64 seek(int64 ofs, bool absolute = true)
   {
      ofs;
      absolute;
      return -1;
   }

   BStream* getSrcStream(void) const 
   {
      return mpSrcStream;
   }

   const BDeflateStreamHeader& getHeader(void) const
   {
      return mHeader;
   }
   
   int getInflateErrorValue(void) const { return mInflErrorStatus; }
   int getErrorStatusValue(void) const { return mErrorStatus; }

   virtual bool errorStatus(void) const { return mErrorStatus != eNoError; }
   
   virtual bool setWritable(bool f) { f; return false; }
   virtual bool setSeekable(bool f) { f; return false; }

   BInflate& getInflate(void) { return mInflate; }

private:
   BInflateStream(const BInflateStream& b);
   BInflateStream& operator= (const BInflateStream& rhs);

   BInflate mInflate;
   
   BStream* mpSrcStream;
   BDeflateStreamHeader mHeader;

   uint64 mBytesRead;
   uint64 mBytesWritten;
   
   uint mReadAdler32;
   uint mWriteAdler32;
   int mErrorStatus;
   int mInflErrorStatus;
         
   enum { cBufSize = 8192 };
   uchar mInBuf[cBufSize];
   
   uint mInBufSize;
   uint mInBufOfs;
   
   bool mInBufEOFFlag : 1;

   bool mOpened : 1;

   bool mSizeKnown : 1;
   
   bool mStreamTerminated : 1;
         
   // true on success
   bool readHeader(void)
   {
      *mpSrcStream >> mHeader;

      if ((mpSrcStream->errorStatus()) || (!mHeader.checkSig()) || (!mHeader.checkAdler32()))
      {
         mErrorStatus = eHeaderError;
         return false;
      }

      if (mHeader.mHeaderType > 1)
         return false;

      mSizeKnown = (0 == mHeader.mHeaderType);
      return true;
   }

   // true on success
   bool refreshInputBufferSizeKnown(void)
   {
      BDEBUG_ASSERT(mSizeKnown);

      mInBufOfs = 0;
      mInBufSize = (uint)Math::Min<uint64>(mHeader.mDstBytes - mBytesRead, cBufSize);

      if (mInBufSize != mpSrcStream->readBytes(mInBuf, mInBufSize))
      {  
         mErrorStatus = eSrcStreamError;
         return false;
      }

      mInBufEOFFlag = (mHeader.mDstBytes - (mBytesRead + mInBufSize)) == 0;

      return true;
   }
   
   // true on success
   bool termStreamSizeKnown(void)
   {
      BDEBUG_ASSERT(mSizeKnown);
      
      if (mStreamTerminated)
         return true;
      mStreamTerminated = true;
                        
      enum { cOutBufSize = 8192 };
      uchar outBuf[cOutBufSize];

      if (mInflErrorStatus >= 0)
      {
         while (mInflErrorStatus != INFL_STATUS_DONE)
         {
            if (mInBufOfs == mInBufSize)
            {
               if (!refreshInputBufferSizeKnown())
                  return false;
            }

            int inBufBytes = mInBufSize - mInBufOfs;
            int outBufBytes = cOutBufSize;

            mInflErrorStatus = mInflate.decompress(
               mInBuf + mInBufOfs, &inBufBytes,
               outBuf, &outBufBytes,
               mInBufEOFFlag);

            mReadAdler32 = calcAdler32(mInBuf + mInBufOfs, inBufBytes, mReadAdler32);
            mBytesRead += inBufBytes;

            mWriteAdler32 = calcAdler32(outBuf, outBufBytes, mWriteAdler32);
            mBytesWritten += outBufBytes;

            mInBufOfs += inBufBytes;

            if (mInflErrorStatus < 0)
            {
               mErrorStatus = eInflateError;
               return false;
            }
            
            if (mBytesWritten > mHeader.mSrcBytes)
            {
               mErrorStatus = eSrcStreamCorruptError;
               return false;
            }
         };
      }            

      if ( (!mpSrcStream->errorStatus()) && (mHeader.checkSig()) && (mHeader.checkAdler32()) )
      {
         while (mBytesRead < mHeader.mDstBytes)
         {
            if (mInBufOfs == mInBufSize)
            {
               if (!refreshInputBufferSizeKnown())
                  return false;
            }                  

            const int inBufBytes = mInBufSize - mInBufOfs;

            mReadAdler32 = calcAdler32(mInBuf + mInBufOfs, inBufBytes, mReadAdler32);

            mBytesRead += inBufBytes;
            mInBufOfs += inBufBytes;
         }
      }
      
      DWORD endMagic = 0;
      *mpSrcStream >> endMagic;
      
      if (endMagic != cDeflateStreamEndOfStreamMagic)
      {
         mErrorStatus = eSrcStreamCorruptError;
         return false;
      }

      if (
            (mBytesRead != mHeader.mDstBytes) || 
            (mReadAdler32 != mHeader.mDstAdler32) 
         )
      {
         mErrorStatus = eCompAdler32Error;
         return false;
      }
      
      if (
            (mBytesWritten != mHeader.mSrcBytes) ||
            (mWriteAdler32 != mHeader.mSrcAdler32)
         )
      {
         mErrorStatus = eDecompAdler32Error;
         return false;
      }               

      return true;
   }

   uint readBytesSizeKnown(void* p, uint n)
   {
      BDEBUG_ASSERT(mSizeKnown);
      BDEBUG_ASSERT(p);

      n = (uint)Math::Min<uint64>(n, bytesLeft());

      if (!n)
         return 0;

      if ((eNoError != mErrorStatus) || (mStreamTerminated) || (!mpSrcStream))
         return 0;

      if (mpSrcStream->errorStatus())
      {
         mErrorStatus = eSrcStreamError;
         return 0;
      }
      
      // disable buffering if the client is decompressing the entire stream in one shot
      if ((mBytesWritten == 0) && (n == mHeader.mSrcBytes))
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
            mInBuf + mInBufOfs, &inBufBytes,
            reinterpret_cast<uchar*>(p) + outBufOfs, &outBufBytes,
            mInBufEOFFlag);

         mReadAdler32 = calcAdler32(mInBuf + mInBufOfs, inBufBytes, mReadAdler32);
         mBytesRead += inBufBytes;

         mWriteAdler32 = calcAdler32(reinterpret_cast<uchar*>(p) + outBufOfs, outBufBytes, mWriteAdler32);
         mBytesWritten += outBufBytes;

         mInBufOfs += inBufBytes;
         outBufOfs += outBufBytes;

         if (mInflErrorStatus < 0)
         {
            mErrorStatus = eInflateError;
            return 0;
         }

         if (INFL_STATUS_DONE == mInflErrorStatus)
         {
            if (outBufOfs != n)
            {
               mErrorStatus = eDecompressedStreamTooSmallError;
               return 0;
            }

            if (!termStreamSizeKnown())
               return 0;
         }
         
         if (mBytesWritten > mHeader.mSrcBytes)
         {
            mErrorStatus = eSrcStreamCorruptError;
            return false;
         }
      }

      if (0 == bytesLeft())
      {
         if (!termStreamSizeKnown())
            return 0;
      }

      return outBufOfs;
   }
   
   // true on success
   bool refreshInputBufferSizeUnknown(void)
   {
      if (mInBufEOFFlag)
         return true;
      
      mInBufOfs = 0;
         
      mInBufSize = mpSrcStream->readValue<ushort>();
      
      const ushort checkVal = mpSrcStream->readValue<ushort>();
      if (mInBufSize != (uint)(checkVal ^ 0xFFFFU))
      {
         mErrorStatus = eSrcStreamCorruptError;
         return false;
      }
      
      if (mInBufSize == 0)
      {
         mInBufEOFFlag = true;
         return true;
      }
      
      BVERIFY(mInBufSize <= cBufSize);
      
      if (mInBufSize != mpSrcStream->readBytes(mInBuf, mInBufSize))
      {  
         mErrorStatus = eSrcStreamError;
         return false;
      }
      
      return true;
   }
   
   bool termStreamSizeUnknown(void)
   {
      if (mStreamTerminated)
         return true;
      mStreamTerminated = true;
      
      enum { cOutBufSize = 8192 };
      uchar outBuf[cOutBufSize];

      if (mInflErrorStatus >= 0)
      {
         while (mInflErrorStatus != INFL_STATUS_DONE)
         {
            if (mInBufOfs == mInBufSize)
            {
               if (!refreshInputBufferSizeUnknown())
                  return false;
            }

            int inBufBytes = mInBufSize - mInBufOfs;
            int outBufBytes = cOutBufSize;

            mInflErrorStatus = mInflate.decompress(
               mInBuf + mInBufOfs, &inBufBytes,
               outBuf, &outBufBytes,
               mInBufEOFFlag);

            mReadAdler32 = calcAdler32(mInBuf + mInBufOfs, inBufBytes, mReadAdler32);
            mBytesRead += inBufBytes;

            mWriteAdler32 = calcAdler32(outBuf, outBufBytes, mWriteAdler32);
            mBytesWritten += outBufBytes;

            mInBufOfs += inBufBytes;

            if (mInflErrorStatus < 0)
            {
               mErrorStatus = eInflateError;
               return false;
            }
         };
      }

      while (!mInBufEOFFlag)
      {
         if (mInBufOfs == mInBufSize)
         {
            if (!refreshInputBufferSizeUnknown())
               return false;
         }                  

         const int inBufBytes = mInBufSize - mInBufOfs;

         mReadAdler32 = calcAdler32(mInBuf + mInBufOfs, inBufBytes, mReadAdler32);

         mBytesRead += inBufBytes;
         mInBufOfs += inBufBytes;
      }
      
      DWORD endMagic = 0;
      *mpSrcStream >> endMagic;

      if (endMagic != cDeflateStreamEndOfStreamMagic)
      {
         mErrorStatus = eSrcStreamCorruptError;
         return false;
      }
      
      *mpSrcStream >> mHeader;
      if ((!mHeader.checkSig()) || (!mHeader.checkAdler32()))
      {
         mErrorStatus = eHeaderError;
         return false;
      }
      
      mSizeKnown = true;
      
      if (
            (mBytesRead != mHeader.mDstBytes) || 
            (mReadAdler32 != mHeader.mDstAdler32) 
         )
      {
         mErrorStatus = eCompAdler32Error;
         return false;
      }

      if (
            (mBytesWritten != mHeader.mSrcBytes) ||
            (mWriteAdler32 != mHeader.mSrcAdler32)
         )
      {
         mErrorStatus = eDecompAdler32Error;
         return false;
      }               

      return true;
   }
   
   uint readBytesSizeUnknown(void* p, uint n)
   {
      BDEBUG_ASSERT(p);
      
      if ((!n) || (eNoError != mErrorStatus) || (!mpSrcStream) || (mpSrcStream->errorStatus()) || (mStreamTerminated))
         return 0;
      
      uint outBufOfs = 0;
      while (outBufOfs < n)
      {
         if (mInBufOfs == mInBufSize)
         {
            if (!refreshInputBufferSizeUnknown())
               return 0;
         }

         int inBufBytes = mInBufSize - mInBufOfs;
         int outBufBytes = n - outBufOfs;

         mInflErrorStatus = mInflate.decompress(
            mInBuf + mInBufOfs, &inBufBytes,
            reinterpret_cast<uchar*>(p) + outBufOfs, &outBufBytes,
            mInBufEOFFlag);

         mReadAdler32 = calcAdler32(mInBuf + mInBufOfs, inBufBytes, mReadAdler32);
         mBytesRead += inBufBytes;

         mWriteAdler32 = calcAdler32(reinterpret_cast<uchar*>(p) + outBufOfs, outBufBytes, mWriteAdler32);
         mBytesWritten += outBufBytes;

         mInBufOfs += inBufBytes;
         outBufOfs += outBufBytes;

         if (mInflErrorStatus < 0)
         {
            mErrorStatus = eInflateError;
            return 0;
         }

         if (INFL_STATUS_DONE == mInflErrorStatus)
         {
            if (!termStreamSizeUnknown())
               return 0;
            break;
         }
      }

      return outBufOfs;
   }
};

//---------------------------------------------------------------------------------------------------------------------------------
// class BDeflateStream
//---------------------------------------------------------------------------------------------------------------------------------
class BDeflateStream : public BStream
{
   BDeflateStream(const BDeflateStream& b);
   BDeflateStream& operator= (const BDeflateStream& rhs);
         
   BStream* mpDstStream;
   BDeflateStreamHeader mHeader;
   uint64 mHeaderOfs;
   BDeflate mDeflate;
   bool mOpened;
   
   enum { cOutBufSize = 8192 };
   uchar mOutBuf[cOutBufSize];
   uint mOutBufOfs;
   
   int mMaxCompares;
   int mStrategy;
   bool mGreedyFlag;
   
   bool mErrorFlag;
   
public:
   BDeflateStream(void) :
      BStream(),
      mpDstStream(NULL),
      mOpened(false),
      mOutBufOfs(0),
      mMaxCompares(0),
      mStrategy(0),
      mGreedyFlag(false),
      mErrorFlag(false)
   {
   }         
               
   BDeflateStream(BStream& dstStream, uint flags = cSFWritable, int maxCompares = 500, int strategy = DEFL_ALL_BLOCKS, bool greedyFlag = false, const BString& name = "") :
      BStream(),
      mpDstStream(NULL),
      mOpened(false),
      mOutBufOfs(0),
      mMaxCompares(0),
      mStrategy(0),
      mGreedyFlag(false),
      mErrorFlag(false)
   {
      open(dstStream, flags, maxCompares, strategy, greedyFlag, name); 
   }
   
   bool open(BStream& dstStream, uint flags = cSFWritable, int maxCompares = 500, int strategy = DEFL_ALL_BLOCKS, bool greedyFlag = false, const BString& name = "")
   { 
      if ((flags & cSFWritable) == 0)
         return false;
         
      if (flags & cSFSeekable)
         return false;         
         
      mDeflate.init();
      
      setName(name);
      setFlags(flags);
      
      mpDstStream = &dstStream;
      mOpened = true;
      mOutBufOfs = 0;
      mMaxCompares = maxCompares;
      mStrategy = strategy;
      mGreedyFlag = greedyFlag;
      mErrorFlag = false;
               
      mHeaderOfs = mpDstStream->curOfs();
      
      mHeader.clear();

      if (!mpDstStream->seekable())
      {
         mHeader.initSig();
         mHeader.mHeaderType = 1;
         mHeader.computeAdler32();
      }

      *mpDstStream << mHeader;
      
      return !mpDstStream->errorStatus();
   }
   
   ~BDeflateStream()
   {
      close();
   }
   
   // true on success
   virtual bool close(void)
   {
      if (!mOpened)
         return true;
         
      mOpened = false;
      
      if ((mErrorFlag) || (mpDstStream->errorStatus()))
      {
         mErrorFlag = true;
         return false;
      }

      int status;
      do
      {
         int inBufBytes = 0;
         int outBufBytes = cOutBufSize - mOutBufOfs;
                     
         status = mDeflate.compress(
            // &status is just a dummy buffer
            reinterpret_cast<const uchar*>(&status), &inBufBytes, 
            mOutBuf + mOutBufOfs, &outBufBytes,
            1,
            mMaxCompares, mStrategy, mGreedyFlag);

         BVERIFY((status == DEFL_STATUS_OKAY) || (status == DEFL_STATUS_DONE));
         BVERIFY(0 == inBufBytes);
         BVERIFY((uint)outBufBytes <= cOutBufSize - mOutBufOfs);

         mOutBufOfs += outBufBytes;

         if ((cOutBufSize == mOutBufOfs) || (status == DEFL_STATUS_DONE))
         {
            if (mOutBufOfs)
            {
               if (!mpDstStream->seekable())
               {
                  mpDstStream->writeValue<ushort>((ushort)mOutBufOfs);
                  mpDstStream->writeValue<ushort>((ushort)mOutBufOfs ^ 0xFFFFU);
               }
                              
               const uint bytesWritten = mpDstStream->writeBytes(mOutBuf, mOutBufOfs);
               if (bytesWritten != mOutBufOfs)
               {
                  mErrorFlag = true;
                  return false;
               }
                                                         
               mHeader.mDstBytes += mOutBufOfs;
               mHeader.mDstAdler32 = calcAdler32(mOutBuf, mOutBufOfs, mHeader.mDstAdler32);
            
               mOutBufOfs = 0;             
            }
         }               
      } while (DEFL_STATUS_DONE != status);
      
      if (!mpDstStream->seekable())
      {
         mpDstStream->writeValue<ushort>(0);
         mpDstStream->writeValue<ushort>(0xFFFF);
      }
      
      *mpDstStream << static_cast<DWORD>(cDeflateStreamEndOfStreamMagic);
      
      if (mpDstStream->seekable())                  
      {
         if (mpDstStream->seek((int64)mHeaderOfs) != (int64)mHeaderOfs)
         {
            mErrorFlag = true;
            return false;
         }
      }
      else
         mHeader.mHeaderType = 2;

      mHeader.initSig();
      mHeader.computeAdler32();
      *mpDstStream << mHeader;
      
      if (mpDstStream->errorStatus())
      {
         mErrorFlag = true;
         return false;
      }
      
      return true;
   }

   // Returns the destination stream's current size
   virtual uint64 size(void) const
   {
      if ((!mOpened) || (!mpDstStream))
         return 0;
         
      return mpDstStream->size();
   }
               
   virtual uint readBytes(void* p, uint n)
   {
      p;
      n;
      return 0;
   }
         
   virtual uint writeBytes(const void* p, uint n)
   {
      BDEBUG_ASSERT(p);
      if (!n)
         return 0;

      if ((!mOpened) || (mErrorFlag) || (mpDstStream->errorStatus()))
      {
         mErrorFlag = true;
         return 0;
      }
                  
      mHeader.mSrcBytes += n;
      mHeader.mSrcAdler32 = calcAdler32(p, n, mHeader.mSrcAdler32);
      
      uint inBufOfs = 0;
      while (inBufOfs < n)
      {
         int inBufBytes = n - inBufOfs;
         int outBufBytes = cOutBufSize - mOutBufOfs;
         
         // Returns DEFL_STATUS_OKAY or DEFL_STATUS_DONE.
         int status = mDeflate.compress(
            reinterpret_cast<const uchar*>(p) + inBufOfs, &inBufBytes, 
            mOutBuf + mOutBufOfs, &outBufBytes,
            0,
            mMaxCompares, mStrategy, mGreedyFlag);
         
         BVERIFY(status == DEFL_STATUS_OKAY);
         BVERIFY(inBufBytes <= (int)(n - inBufOfs));
         BVERIFY(outBufBytes <= (int)(cOutBufSize - mOutBufOfs));
         
         inBufOfs += inBufBytes;
         mOutBufOfs += outBufBytes;
         
         if (cOutBufSize == mOutBufOfs)
         {
            if (!mpDstStream->seekable())
            {
               mpDstStream->writeValue<ushort>((ushort)mOutBufOfs);
               mpDstStream->writeValue<ushort>((ushort)mOutBufOfs ^ 0xFFFF);
            }
            
            const uint bytesWritten = mpDstStream->writeBytes(mOutBuf, mOutBufOfs);
            if (bytesWritten != mOutBufOfs)
            {
               mErrorFlag = true;
               return 0;
            }
            
            mHeader.mDstBytes += mOutBufOfs;
            mHeader.mDstAdler32 = calcAdler32(mOutBuf, mOutBufOfs, mHeader.mDstAdler32);
            
            mOutBufOfs = 0;             
         }               
      }               

      return n;
   }

   // Returns the destination stream's current offset         
   virtual uint64 curOfs(void) const
   {
      if ((!mOpened) || (!mpDstStream))
         return 0;
         
      return mpDstStream->curOfs();
   }

   virtual uint64 bytesLeft(void) const
   {
      return 0;
   }

   virtual int64 seek(int64 ofs, bool absolute = true)
   {
      ofs;
      absolute;
      return -1;
   }
   
   BStream* getDstStream(void) const 
   {
      return mpDstStream;
   }
   
   const BDeflateStreamHeader& getHeader(void) const
   {
      return mHeader;
   }
   
   uint64 compressedSize(void) const
   {
      return mHeader.mDstBytes;
   }
   
   virtual bool opened(void) const { return mOpened; }
   
   virtual bool errorStatus(void) const { return mErrorFlag; }
   
   virtual bool setReadable(bool f) { f; return false; }
   virtual bool setSeekable(bool f) { f; return false; }
};   


#if 0
#include "math\random.h"
#include "compressedStream.h"
#include "stream\dynamicStream.h"
void test(void)
{
   //   BECFUtils::test();

//#define CHECK BASSERT

   Random random;

   for ( ; ; )
   {
      BDynamicStream compData;

      if (random.uRand() & 1)
         compData.setSeekable(false);

      BDeflateStream deflStream;
      CHECK(deflStream.open(compData));

      const uint l = random.iRand(0, 655360);
      BByteArray bytes(l);

      for (uint i = 0; i < l; i++)
         bytes[i] = (uchar)random.uRand();

      printf("%u\n", l);

      uint left = l;
      uint ofs = 0;
      while (left)
      {
         uint n = random.iRand(0, left + 1);
         CHECK(deflStream.writeBytes(bytes.getPtr() + ofs, n) == n);
         left -= n;
         ofs += n;
      }

      CHECK(deflStream.close());

      compData.setSeekable(true);
      CHECK(compData.seek(0) == 0);

      BInflateStream inflStream;
      CHECK(inflStream.open(compData));

      BByteArray buf(l);

      left = l;
      ofs = 0;
      while (left)
      {
         uint n = random.iRand(0, left + 1);
         CHECK(inflStream.readBytes(buf.getPtr() + ofs, n) == n);

         ofs += n;
         left -= n;
      }

      CHECK( memcmp(buf.getPtr(), bytes.getPtr(), l) == 0 );

      CHECK(inflStream.close());
   }      


}
#endif
