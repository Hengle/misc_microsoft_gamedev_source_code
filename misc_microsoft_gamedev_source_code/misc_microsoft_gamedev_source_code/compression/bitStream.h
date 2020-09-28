//---------------------------------------------------------------------------------------------------------------------------------------------
// File: bitStream.h
//---------------------------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "stream\stream.h"

//---------------------------------------------------------------------------------------------------------------------------------------------

class BOutputBitStream
{
public:
   virtual ~BOutputBitStream() { }

   virtual bool begin() = 0; 
   virtual bool putBits(uint64 bits, uint numBits) = 0;
   virtual bool alignToByte() = 0;
   virtual bool end() = 0;
};

//---------------------------------------------------------------------------------------------------------------------------------------------

class BInputBitStream
{
public:
   virtual ~BInputBitStream() { }

   virtual bool begin() = 0;       
   virtual bool peekBits(uint& bits, uint numBits) = 0;
   virtual bool peekBits16(uint& bits) = 0;
   virtual bool getBits(uint& bits, uint numBits) = 0;
   virtual bool getBits(uint64& bits, uint numBits) = 0;
   virtual bool removeBits(uint numBits) = 0;
   virtual bool alignToByte() = 0;
   virtual bool end() = 0;   
};

//---------------------------------------------------------------------------------------------------------------------------------------------

class BOutputBitStreamAdapter : public BOutputBitStream
{
public:
   BOutputBitStreamAdapter(BStream* pStream = NULL) : 
      mpStream(pStream), mBitBuf(0), mBitBufSize(0)
   {
   }
   
   virtual ~BOutputBitStreamAdapter()
   {
   }
   
   void setStream(BStream* pStream) { mpStream = pStream; }
   BStream* getStream() const { return mpStream; }
   
   virtual bool begin()
   {
      BDEBUG_ASSERT(mpStream);
      mBitBuf = 0; 
      mBitBufSize = 0;
      
      return true;
   }

   virtual bool putBits(uint64 bits, uint numBits)
   {
      BDEBUG_ASSERT(mpStream);
      
      if (numBits > 64U)
         return false;
      
      BDEBUG_ASSERT( (numBits == 64U) || (bits < Utils::BBitMasks::get64(numBits)) );
      
      mBitBuf |= (((uint64)bits) << mBitBufSize);
      mBitBufSize += numBits;
      
      while (mBitBufSize >= 8)
      {
         if (!mpStream->putch((uint8)(mBitBuf & 0xFF)))
            return false;
         
         mBitBuf >>= 8;
         mBitBufSize -= 8;
      }
      
      return true;
   }
         
   virtual bool alignToByte()
   {
      if (mBitBufSize & 7)
         return putBits(0, 8 - (mBitBufSize & 7));

      return true;         
   }
   
   virtual bool end()
   {
      BDEBUG_ASSERT(mpStream);
      
      while (mBitBufSize)
      {
         if (!mpStream->putch((uint8)(mBitBuf & 0xFF)))
            return false;

         mBitBuf >>= 8;
         if (mBitBufSize < 8)
            mBitBufSize = 0;
         else
            mBitBufSize -= 8;
      }
      
      return true;
   }

private:
   BStream*    mpStream;
   uint64      mBitBuf;
   uint        mBitBufSize;
};

//---------------------------------------------------------------------------------------------------------------------------------------------

class BInputBitStreamAdapter : public BInputBitStream
{
public:
   BInputBitStreamAdapter() : 
      mpStream(NULL), mBitBuf(0), mBitBufSize(0), mNumBytesLeft(0)
   {
   }
   
   BInputBitStreamAdapter(BStream* pStream, uint64 numBytesLeft = 0) : 
      mpStream(NULL), mBitBuf(0), mBitBufSize(0), mNumBytesLeft(0), mNumBitsLeft(0)
   {
      setStream(pStream, numBytesLeft);
   }
   
   virtual ~BInputBitStreamAdapter()
   {
   }
   
   void setStream(BStream* pStream, uint64 numBytesLeft = 0) 
   { 
      mpStream = pStream; 
      mNumBytesLeft = 0;
            
      if (numBytesLeft)
         mNumBytesLeft = numBytesLeft;
      else if (mpStream)
         mNumBytesLeft = mpStream->bytesLeft();
         
      mNumBitsLeft = mNumBytesLeft * 8;
   }
   
   BStream* getStream() const { return mpStream; }
   uint64 getNumBytesLeft() const { return mNumBytesLeft; }
   uint64 getNumBitsLeft() const { return mNumBitsLeft; }

   virtual bool begin()
   {
      BDEBUG_ASSERT(mpStream);
      
      mBitBuf = 0;
      mBitBufSize = 0;
            
      return true;
   }
   
   virtual bool peekBits(uint& bits, uint numBits)
   {
      BDEBUG_ASSERT(numBits <= 32);
      
      while (mBitBufSize < numBits)
      {
         uchar c;
         if (!mNumBytesLeft)
            c = 0;
         else 
         {  
            if (mpStream->readBytes(&c, 1) != 1)
               return false;
            mNumBytesLeft--;
         }
                  
         mBitBuf |= (((uint64)c) << mBitBufSize);
         mBitBufSize += 8;
      }
   
      bits = (uint)(mBitBuf & (Utils::BBitMasks::get64(numBits) - 1)); 
      return true;
   }
   
   virtual bool peekBits16(uint& bits)
   {
      while (mBitBufSize < 16)
      {
         uchar c;
         if (!mNumBytesLeft)
            c = 0;
         else 
         {  
            if (mpStream->readBytes(&c, 1) != 1)
               return false;
            mNumBytesLeft--;
         }

         mBitBuf |= (((uint64)c) << mBitBufSize);
         mBitBufSize += 8;
      }

      bits = (uint)(mBitBuf & 0xFFFF);
      return true;
   }
   
   virtual bool getBits(uint& bits, uint numBits)
   {
      BDEBUG_ASSERT(numBits <= 32U);
      
      if (numBits > mNumBitsLeft)
         return false;
      mNumBitsLeft -= numBits;
      
      while (mBitBufSize < numBits)
      {
         uchar c;
         if (!mNumBytesLeft)
            c = 0;
         else 
         {  
            if (mpStream->readBytes(&c, 1) != 1)
               return false;
            mNumBytesLeft--;
         }

         mBitBuf |= (((uint64)c) << mBitBufSize);
         mBitBufSize += 8;
      }
      
      bits = (uint)(mBitBuf & (Utils::BBitMasks::get64(numBits) - 1));   
      mBitBuf >>= numBits;
      mBitBufSize -= numBits;
      
      return true;
   }
   
   virtual bool getBits(uint64& bits, uint numBits)
   {
      BDEBUG_ASSERT(numBits <= 64U);
      
      if (numBits > mNumBitsLeft)
         return false;
      mNumBitsLeft -= numBits;

      while (mBitBufSize < numBits)
      {
         uchar c;
         if (!mNumBytesLeft)
            c = 0;
         else 
         {  
            if (mpStream->readBytes(&c, 1) != 1)
               return false;
            mNumBytesLeft--;
         }

         mBitBuf |= (((uint64)c) << mBitBufSize);
         mBitBufSize += 8;
      }

      bits = (uint)(mBitBuf & (Utils::BBitMasks::get64(numBits) - 1));   
      mBitBuf >>= numBits;
      mBitBufSize -= numBits;

      return true;
   }
   
   virtual bool removeBits(uint numBits)
   {
      BDEBUG_ASSERT(numBits <= 64U);
      
      if (numBits > mNumBitsLeft)
         return false;
      mNumBitsLeft -= numBits;
      
      while (mBitBufSize < numBits)
      {
         uchar c;
         if (!mNumBytesLeft)
            c = 0;
         else 
         {  
            if (mpStream->readBytes(&c, 1) != 1)
               return false;
            mNumBytesLeft--;
         }

         mBitBuf |= (((uint64)c) << mBitBufSize);
         mBitBufSize += 8;
      }

      mBitBuf >>= numBits;
      mBitBufSize -= numBits;

      return true;
   }
   
   virtual bool alignToByte()
   {
      return removeBits(mBitBufSize & 7);
   }
   
   virtual bool end()
   {
      return true;
   }

private:
   BStream*    mpStream;
   uint64      mBitBuf;
   uint        mBitBufSize;
   uint64      mNumBytesLeft;
   uint64      mNumBitsLeft;
};
