//============================================================================
//
//  encryptedStream.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "encryptedStream.h"

bool BEncryptedStream::open(BStream* pStream, const BString& keyPhrase, const BString& name)
{
   close();

   if (!pStream)
      return false;

   if (keyPhrase.length() < 1)
      return false;

   setName(name);

   setFlags(cSFWritable);
   if (!pStream->getWritable())
      return false;

   teaCryptInitKeys(keyPhrase, mKeys[0], mKeys[1], mKeys[2]);

   mpStream = pStream;   
   mOpened = true;

   return true;
}

bool BEncryptedStream::open(BStream* pStream, uint64 k1, uint64 k2, uint64 k3, const BString& name)
{
   close();

   if (!pStream)
      return false;

   setName(name);

   setFlags(cSFWritable);
   if (!pStream->getWritable())
      return false;

   mKeys[0] = k1;
   mKeys[1] = k2;
   mKeys[2] = k3;

   mpStream = pStream;   
   mOpened = true;

   return true;
}

bool BEncryptedStream::close(void)
{
   if (!mOpened)
      return true;

   bool success = true;

   success = flushBuf();

   mpStream = NULL;
   Utils::ClearObj(mKeys);
   Utils::ClearObj(mBuf);
   mTotalBytesWritten = 0;
   mCurCounter = 0;
   mBufSize = 0;
   mOpened = false;
   mErrorStatus = false;

   return success;
}

uint BEncryptedStream::writeBytes(const void* p, uint n)
{
   if ((!mOpened) || (!n) || (errorStatus()))
      return 0;

   const uchar* pBytes = static_cast<const uchar*>(p);

   const uint totalBytesWritten = n;

   if (mBufSize)
   {
      const uint numBytesToCopy = Math::Min<uint>(cTeaCryptBlockSize - mBufSize, n);
      memcpy(mBuf + mBufSize, pBytes, numBytesToCopy);

      mBufSize += numBytesToCopy;
      pBytes += numBytesToCopy;
      n -= numBytesToCopy;

      if (mBufSize == cTeaCryptBlockSize)
      {
         if (!flushBuf())
         {
            mErrorStatus = true;
            return 0;
         }

         mCurCounter++;
      }

      mTotalBytesWritten += numBytesToCopy;
   }

   const uint cTotalBlocks = 128;
   const uint cTotalBytes = cTotalBlocks * cTeaCryptBlockSize;
   uchar buf[cTotalBytes];

   while (n >= cTeaCryptBlockSize)
   {  
      const uint numBlocks = Math::Min<uint>(n / cTeaCryptBlockSize, cTotalBlocks);
      const uint numBytes = numBlocks * cTeaCryptBlockSize;

      uint curBufOfs = 0;
      for (uint i = 0; i < numBlocks; i++, curBufOfs += cTeaCryptBlockSize)
         teaEncryptBlock64(cDefaultTeaCryptIV, mKeys[0], mKeys[1], mKeys[2], pBytes + curBufOfs, buf + curBufOfs, mCurCounter + i);

      if (mpStream->writeBytes(buf, numBytes) != numBytes)
      {
         mErrorStatus = true;
         return 0;
      }

      pBytes += numBytes;            
      n -= numBytes;

      mTotalBytesWritten += numBytes;
      mCurCounter += numBlocks;
   }

   if (n)
   {
      BDEBUG_ASSERT(!mBufSize);

      memcpy(mBuf, pBytes, n);
      mBufSize = n;

      mTotalBytesWritten += n;
   }

   return totalBytesWritten;
}

bool BEncryptedStream::flushBuf(void)
{
   if (!mOpened) 
      return false;

   if (!mBufSize)
      return true;

   if (mBufSize < cTeaCryptBlockSize)
      memset(mBuf + mBufSize, 0, cTeaCryptBlockSize - mBufSize);

   teaEncryptBlock64(cDefaultTeaCryptIV, mKeys[0], mKeys[1], mKeys[2], mBuf, mBuf, mCurCounter);

   if (mpStream->writeBytes(mBuf, cTeaCryptBlockSize) != cTeaCryptBlockSize)
      return false;

   mBufSize = 0;           

   return true;            
}

bool BDecryptedStream::open(BStream* pStream, const BString& keyPhrase, const BString& name)
{
   close();

   if (!pStream)
      return false;

   if (pStream->sizeKnown())
   {
      if (pStream->size() < cTeaCryptBlockSize)
         return false;
   }

   if (!pStream->getReadable())
      return false;

   if (keyPhrase.length() < 1)
      return false;

   setName(name);

   if (pStream->getSeekable())
      setFlags(cSFReadable | cSFSeekable);
   else
      setFlags(cSFReadable);

   teaCryptInitKeys(keyPhrase, mKeys[0], mKeys[1], mKeys[2]);

   mpStream = pStream;
   mOpened = true;

   return true;
}

bool BDecryptedStream::open(BStream* pStream, uint64 k1, uint64 k2, uint64 k3, const BString& name)
{
   close();

   if (!pStream)
      return false;

   if (pStream->sizeKnown())
   {
      if (pStream->size() < cTeaCryptBlockSize)
         return false;
   }

   if (!pStream->getReadable())
      return false;

   setName(name);

   if (pStream->getSeekable())
      setFlags(cSFReadable | cSFSeekable);
   else
      setFlags(cSFReadable);

   mKeys[0] = k1;
   mKeys[1] = k2;
   mKeys[2] = k3;

   mpStream = pStream;
   mOpened = true;

   return true;
}

bool BDecryptedStream::close(void)
{
   if (!mOpened)
      return true;

   mpStream = NULL;
   Utils::ClearObj(mKeys);
   Utils::ClearObj(mBuf);
   mCurFileOfs = 0;
   mBufFileOfs = UINT64_MAX;
   mOpened = false;
   mErrorStatus = false;

   return true; 
}

uint BDecryptedStream::readBytes(void* p, uint n)
{
   if ((!mOpened) || (!n) || (mErrorStatus))
      return 0;

   if (mpStream->sizeKnown())
   {
      if (n > bytesLeft())
         n = (uint)bytesLeft();
   }         

   uchar* pBytes = reinterpret_cast<uchar*>(p);
   
   uint totalBytesRead = 0;

   if (mBufFileOfs != UINT64_MAX)
   {
      if ((mCurFileOfs >= mBufFileOfs) && (mCurFileOfs < mBufFileOfs + cTeaCryptBlockSize))
      {
         const uint bytesToRead = Math::Min<uint>(n, cTeaCryptBlockSize - (uint)(mCurFileOfs - mBufFileOfs));

         memcpy(pBytes, mBuf + (uint)(mCurFileOfs - mBufFileOfs), bytesToRead);

         pBytes         += bytesToRead;
         n              -= bytesToRead;
         mCurFileOfs    += bytesToRead;
         totalBytesRead += bytesToRead;

         if (!n)
            return totalBytesRead;
      }
   }

   if ((mCurFileOfs % cTeaCryptBlockSize) != 0)
   {
      const uint64 blockIndex = mCurFileOfs / cTeaCryptBlockSize;
      if (!readBlock(blockIndex))
         return totalBytesRead;

      const uint bytesToRead = Math::Min<uint>(n, cTeaCryptBlockSize - (uint)(mCurFileOfs - mBufFileOfs));

      memcpy(pBytes, mBuf + (uint)(mCurFileOfs - mBufFileOfs), bytesToRead);

      pBytes         += bytesToRead;
      n              -= bytesToRead;
      mCurFileOfs    += bytesToRead;
      totalBytesRead += bytesToRead;

      if (!n)
         return totalBytesRead;
   }

   BDEBUG_ASSERT((mCurFileOfs % cTeaCryptBlockSize) == 0);

   if (!seekOrSkip(mCurFileOfs))
   {
      if (mpStream->errorStatus())
         mErrorStatus = true;
      return totalBytesRead;
   }
   
   const uint cTotalBlocks = 128;
   const uint cTotalBytes = cTotalBlocks * cTeaCryptBlockSize;
   uchar buf[cTotalBytes];

   while (n >= cTeaCryptBlockSize) 
   {
      uint numBlocks = Math::Min<uint>(n / cTeaCryptBlockSize, cTotalBlocks);
      uint numBytes = numBlocks * cTeaCryptBlockSize;

      uint bytesRead = mpStream->readBytes(buf, numBytes);
      if (bytesRead != numBytes)
      {
         if ((mpStream->errorStatus()) || (bytesRead < cTeaCryptBlockSize))
         {
            mErrorStatus = true;
            return totalBytesRead;
         }
         else
         {
            numBlocks = bytesRead / cTeaCryptBlockSize;
            numBytes = numBlocks * cTeaCryptBlockSize;
            
            n = numBytes;
         }
      }

      uint curBufOfs = 0;
      uint curCounter = (uint)(mCurFileOfs / cTeaCryptBlockSize);
      for (uint i = 0; i < numBlocks; i++, curBufOfs += cTeaCryptBlockSize)
         teaDecryptBlock64(cDefaultTeaCryptIV, mKeys[0], mKeys[1], mKeys[2], buf + curBufOfs, pBytes + curBufOfs, curCounter + i);

      pBytes         += numBytes;            
      n              -= numBytes;
      mCurFileOfs    += numBytes;
      totalBytesRead += numBytes;
   }

   BDEBUG_ASSERT((mCurFileOfs % cTeaCryptBlockSize) == 0);

   if (n)
   {
      const uint64 blockIndex = mCurFileOfs / cTeaCryptBlockSize;
      if (!readBlock(blockIndex))
         return totalBytesRead;

      const uint bytesToRead = Math::Min<uint>(n, cTeaCryptBlockSize - (uint)(mCurFileOfs - mBufFileOfs));

      memcpy(pBytes, mBuf + (uint)(mCurFileOfs - mBufFileOfs), bytesToRead);

      pBytes         += bytesToRead;
      n              -= bytesToRead;
      mCurFileOfs    += bytesToRead;
      totalBytesRead += bytesToRead;
      
      BDEBUG_ASSERT(!n);
   }      

   return totalBytesRead;
}

bool BDecryptedStream::seekOrSkip(uint64 ofs)
{
   const uint64 curOfs = mpStream->curOfs() - mBaseOfs;
   if (curOfs == ofs)
      return true;
      
   if (mpStream->getSeekable())
   {
      if ((uint)mpStream->seek(ofs + mBaseOfs) != ofs)
         return false;
         
      return true;
   }
   
   if (ofs < curOfs)
      return false;
   
   const uint64 numBytesToSkip = ofs - curOfs;
   uint64 totalSkipped = mpStream->skipBytes(numBytesToSkip);
   if (totalSkipped != numBytesToSkip)
      return false;
   
   BDEBUG_ASSERT(mpStream->curOfs() - mBaseOfs == ofs);
   
   return true;
}

bool BDecryptedStream::readBlock(uint64 blockIndex)
{ 
   const uint64 blockFileOfs = blockIndex * cTeaCryptBlockSize;
   if (mBufFileOfs == blockFileOfs)
      return true;

   if (!seekOrSkip(blockFileOfs))
   {
      if (mpStream->errorStatus())
         mErrorStatus = true;
         
      return false;
   }
   
   mBufFileOfs = blockFileOfs;

   uint bytesRead = mpStream->readBytes(mBuf, cTeaCryptBlockSize);
   if (bytesRead != cTeaCryptBlockSize)
   {
      mBufFileOfs = UINT64_MAX;

      if (mpStream->errorStatus())
         mErrorStatus = true;
         
      return false;
   }   

   const uint counter = (uint)blockIndex;
   teaDecryptBlock64(cDefaultTeaCryptIV, mKeys[0], mKeys[1], mKeys[2], mBuf, mBuf, counter);

   return true;
}

int64 BDecryptedStream::seek(int64 ofs, bool absolute)
{
   if (!mpStream)
      return INT64_MIN;
   
   if (!absolute)
      ofs += mCurFileOfs; 

   if (ofs < 0)
      return INT64_MIN;
      
   if (mBufFileOfs != UINT64_MAX)
   {
      if (((uint64)ofs >= mBufFileOfs) && ((uint64)ofs < (mBufFileOfs + cTeaCryptBlockSize)))
      {
         mCurFileOfs = ofs;
         return ofs;
      }
   }
      
   if (!getSeekable())
   {
      if ((uint64)ofs < mCurFileOfs)
         return INT64_MIN;
   }

   if (mpStream->sizeKnown())
   {
      // purposely >, not >=
      if ((uint64)ofs > mpStream->size())
         return INT64_MIN;
   }

   mCurFileOfs = ofs;
   mBufFileOfs = UINT64_MAX;

   const uint64 blockIndex = ofs / cTeaCryptBlockSize;
   const uint64 blockFileOfs = blockIndex * cTeaCryptBlockSize;

   if (!seekOrSkip(blockFileOfs))
   {
      if (mpStream->errorStatus())
         mErrorStatus = true;

      return INT64_MAX;
   }

   return mCurFileOfs;
}

#if 0
#include "math\random.h"
#include "math\randomUtils.h"
#include "stream\dynamicStream.h"

int encryptedStreamTest(void)
{
   Random rand;
   uint seed = 1;
   
   for ( ; ; )
   {
      gConsoleOutput.printf("Seed: %u\n", seed);
      rand.setSeed(seed); seed++;
         
      const uint totalSize = rand.iRand(1, 256*1024);
      
      gConsoleOutput.printf("Size: %u\n", totalSize);
      
      BDynamicStream stream;
            
      BEncryptedStream encryptedStream;
      if (!encryptedStream.open(&stream, "Password2"))
         return false;
      
      BByteArray buf(totalSize);
      RandomUtils::RandomFill32(buf.getPtr(), totalSize, seed);
      
      uint bytesLeft = totalSize;
      uint curOfs = 0;
      while (bytesLeft > 0)
      {
         uint n = rand.iRand(1, bytesLeft + 1);
                           
         BVERIFY(encryptedStream.writeBytes(buf.getPtr() + curOfs, n) == n);
         
         curOfs += n;
         bytesLeft -= n;            
      }
      
      BVERIFY(encryptedStream.close());
                  
      BVERIFY((stream.size() % cTeaCryptBlockSize) == 0);
      BVERIFY(stream.size() >= totalSize);
            
      stream.seek(0);
      //stream.setSeekable(false);
      
      BDecryptedStream decryptedStream;
      if (!decryptedStream.open(&stream, "Password2"))
         return false;
         
      BByteArray buf2(totalSize);         
      if (decryptedStream.readBytes(buf2.getPtr(), totalSize) != totalSize)
         return false;
      
      BVERIFY(buf2 == buf);
            
      //stream.setSeekable(true);
      //decryptedStream.setSeekable(true);
      BVERIFY(decryptedStream.seek(0) == 0);
      //stream.setSeekable(false);
      //decryptedStream.setSeekable(false);
            
      curOfs = 0;
      
      bytesLeft = totalSize;
      while (bytesLeft > 0)
      {
         uint n = rand.iRand(1, bytesLeft + 1);

         BVERIFY(decryptedStream.readBytes(buf2.getPtr() + curOfs, n) == n);

         curOfs += n;
         bytesLeft -= n;            
      }
      
      BVERIFY(buf2 == buf);
      
      //stream.setSeekable(true);
      //decryptedStream.setSeekable(true);
      BVERIFY(decryptedStream.seek(0) == 0);
      //stream.setSeekable(false);
      //decryptedStream.setSeekable(false);
     
      if (decryptedStream.getSeekable())
      {
         for (uint i = 0; i < 16; i++)
         {
            uint ofs = rand.iRand(0, totalSize);
            uint len = 1 + rand.iRand(0, totalSize - ofs);
            
            BVERIFY(decryptedStream.seek(ofs) == ofs);
            
            BVERIFY(decryptedStream.readBytes(buf2.getPtr(), len) == len);
            
            BVERIFY(memcmp(buf2.getPtr(), buf.getPtr() + ofs, len) == 0);
            
            ofs += len;
            if (ofs == totalSize)
               continue;
               
            len = 1 + rand.iRand(0, totalSize - ofs);
            
            BVERIFY(decryptedStream.readBytes(buf2.getPtr(), len) == len);

            BVERIFY(memcmp(buf2.getPtr(), buf.getPtr() + ofs, len) == 0);
         }
      }
      else
      {
         for (uint i = 0; i < 16; i++)
         {
            uint ofs = rand.iRand(decryptedStream.curOfs(), totalSize);
            uint len = rand.iRand(1, totalSize - ofs + 1);

            BVERIFY(decryptedStream.seek(ofs) == ofs);

            BVERIFY(decryptedStream.readBytes(buf2.getPtr(), len) == len);

            BVERIFY(memcmp(buf2.getPtr(), buf.getPtr() + ofs, len) == 0);

            ofs += len;
            if (ofs >= totalSize)
               break;

            len = rand.iRand(1, totalSize - ofs + 1);

            BVERIFY(decryptedStream.readBytes(buf2.getPtr(), len) == len);

            BVERIFY(memcmp(buf2.getPtr(), buf.getPtr() + ofs, len) == 0);
            
            if (decryptedStream.curOfs() >= (totalSize - 1))
               break;
         }
      }         

      BVERIFY(decryptedStream.close());
   }      
}   
#endif