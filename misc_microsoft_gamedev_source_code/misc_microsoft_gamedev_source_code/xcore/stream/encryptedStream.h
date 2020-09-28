//============================================================================
//
//  encryptedStream.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "hash\teaCrypt.h"
#include "stream\stream.h"

// BEncryptedStream/BDecrypted stream use the tea crypt functions in CTR (Counter) Block Mode. 
// Currently, encrypted streams don't support random seeks, but decrypted streams do.
// Encrypted streams are always padded to cTeaCryptBlockSize bytes!
// Encrypted streams do not contain a header, or any kind of identifier, or any MAC/checksum/CRC. 
// Just the raw encrypted bits. 

class BEncryptedStream : public BStream
{
   BEncryptedStream(const BEncryptedStream&);
   BEncryptedStream& operator= (const BEncryptedStream&);
   
public:
   BEncryptedStream() :
      BStream(),
      mpStream(NULL),
      mBufSize(0),
      mOpened(false),
      mTotalBytesWritten(0),
      mCurCounter(0),
      mErrorStatus(false)
   {
      Utils::ClearObj(mKeys);
      Utils::ClearObj(mBuf);
   }      
   
   BEncryptedStream(BStream* pStream, const BString& keyPhrase, const BString& name = "") :
      BStream(),
      mpStream(pStream),
      mBufSize(0),
      mOpened(false),
      mTotalBytesWritten(0),
      mCurCounter(0),
      mErrorStatus(false)
   {
      Utils::ClearObj(mBuf);
      Utils::ClearObj(mKeys);
      
      open(pStream, keyPhrase, name);   
   }

   BEncryptedStream(BStream* pStream, uint64 k1, uint64 k2, uint64 k3, const BString& name = "") :
      BStream(),
      mpStream(pStream),
      mBufSize(0),
      mOpened(false),
      mTotalBytesWritten(0),
      mCurCounter(0),
      mErrorStatus(false)
   {
      Utils::ClearObj(mBuf);
      Utils::ClearObj(mKeys);
      
      open(pStream, k1, k2, k3, name);   
   }

   ~BEncryptedStream() { close(); }
   
   static uint getBlockSize(void) { return cTeaCryptBlockSize; }
   
   bool open(BStream* pStream, const BString& keyPhrase, const BString& name = "");

   bool open(BStream* pStream, uint64 k1, uint64 k2, uint64 k3, const BString& name = "");

   virtual uint64 size(void) const { if (!mOpened) return 0; return mTotalBytesWritten; }
   
   virtual bool close(void);
            
   virtual uint readBytes(void* p, uint n) { p; n; return 0; }

   virtual uint writeBytes(const void* p, uint n);
   
   virtual uint64 curOfs(void) const { if (!mOpened) return 0; return mTotalBytesWritten; }

   virtual uint64 bytesLeft(void) const { return 0; }

   virtual int64 seek(int64 ofs, bool absolute = true) { ofs; absolute; return -1; }

   BStream* getStream(void) const { return mpStream; }

   virtual bool errorStatus(void) const { if (!mpStream) return false; return mErrorStatus || mpStream->errorStatus(); }

   virtual bool setWritable(bool f) { f; return false; }
   virtual bool setSeekable(bool f) { f; return false; }

private:
   BStream* mpStream;
      
   uint64 mKeys[3];
   
   uint64 mTotalBytesWritten;
   uint mCurCounter;
      
   uchar mBuf[cTeaCryptBlockSize];
   uint mBufSize;
   
   bool mOpened;
   bool mErrorStatus;
   
   bool flushBuf(void);
};

class BDecryptedStream : public BStream
{
public:
   BDecryptedStream() :
      BStream(),
      mpStream(NULL),
      mBaseOfs(0),
      mCurFileOfs(0),
      mBufFileOfs(UINT64_MAX),
      mOpened(false),
      mErrorStatus(false)
   {
      Utils::ClearObj(mKeys);
      Utils::ClearObj(mBuf);
   }      

   BDecryptedStream(BStream* pStream, const BString& keyPhrase, const BString& name = "") :
      BStream(),
      mpStream(pStream),
      mBaseOfs(0),
      mCurFileOfs(0),
      mBufFileOfs(UINT64_MAX),
      mOpened(false),
      mErrorStatus(false)
   {
      Utils::ClearObj(mBuf);
      Utils::ClearObj(mKeys);

      open(pStream, keyPhrase, name);
   }

   BDecryptedStream(BStream* pStream, uint64 k1, uint64 k2, uint64 k3, const BString& name = "") :
      BStream(),
      mpStream(pStream),
      mBaseOfs(0),
      mCurFileOfs(0),
      mBufFileOfs(UINT64_MAX),
      mOpened(false),
      mErrorStatus(false)
   {
      Utils::ClearObj(mBuf);
      Utils::ClearObj(mKeys);

      open(pStream, k1, k2, k3, name);
   }

   ~BDecryptedStream() { close(); }
   
   static uint getBlockSize(void) { return cTeaCryptBlockSize; }
   
   bool open(BStream* pStream, const BString& keyPhrase, const BString& name = "");

   bool open(BStream* pStream, uint64 k1, uint64 k2, uint64 k3, const BString& name = "");
      
   virtual uint64 size(void) const { if (!mpStream) return 0; return mpStream->size(); }
   
   virtual bool close(void);
            
   virtual uint readBytes(void* p, uint n);

   virtual uint writeBytes(const void* p, uint n) { p; n; return 0; }
   
   virtual uint64 curOfs(void) const { if (!mOpened) return 0; return mCurFileOfs; }

   virtual uint64 bytesLeft(void) const 
   { 
      if (!mpStream) 
         return 0; 
      else if (mpStream->sizeKnown())
         return mpStream->size() - mBaseOfs - mCurFileOfs; 
      else
         return BSTREAM_UNKNOWN_SIZE;
   }

   virtual int64 seek(int64 ofs, bool absolute = true);

   BStream* getStream(void) const { return mpStream; }

   virtual bool errorStatus(void) const { if (!mpStream) return false; return mErrorStatus || mpStream->errorStatus(); }

   virtual bool setWritable(bool f) { f; return false; }
   
   virtual bool setSeekable(bool f) 
   { 
      if (f == getSeekable())
         return true;
         
      if (f)
      {
         if (!mpStream->getSeekable())
            return false;
      }            
         
      return BStream::setSeekable(f);
   }

   void setBaseOfs(uint64 ofs) { mBaseOfs = ofs; }

private:
   BStream* mpStream;
   
   uint64 mKeys[3];

   uint64 mBaseOfs;

   uint64 mCurFileOfs;

   uint64 mBufFileOfs;
   uchar mBuf[cTeaCryptBlockSize];
         
   bool mOpened;
   bool mErrorStatus;
   
   bool readBlock(uint64 blockIndex);
   bool seekOrSkip(uint64 ofs);
};

#if 0
int encryptedStreamTest(void);
#endif