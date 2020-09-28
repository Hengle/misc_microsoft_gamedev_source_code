//==============================================================================
// bitpacker.h
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================
#pragma once

//---------------------------------------------------------------------------------------------------
// class BStaticCoderBuf
//---------------------------------------------------------------------------------------------------
class BStaticCoderBuf
{
public:
   BStaticCoderBuf(uchar* pBuf = NULL, uint bufSize = 0) :
      mpBuf(pBuf),
      mBufSize(bufSize),
      mBufPos(0)
   {
   }

   void setBuf(uchar* pBuf, uint bufSize)
   {
      mpBuf = pBuf;
      mBufSize = bufSize;
      mBufPos = 0;
   }

   // returns false on failure
   bool outputByte(uchar c)
   {
      if (mBufPos >= mBufSize)
         return false;

      mpBuf[mBufPos++] = c;
      return true;
   }

   int inputByte(void) 
   {
      if (mBufPos >= mBufSize)
         return 0;
      return mpBuf[mBufPos++];
   }

   uint getPos(void) const
   {
      return mBufPos;
   }

   uint getSize(void) const
   {
      return mBufSize;
   }

   const uchar* getBuf(void) const 
   {
      return mpBuf;
   }

   uchar* getBuf(void) 
   {
      return mpBuf;
   }

   void setPos(uint pos) 
   {
      mBufPos = pos;
   }

private:
   uchar* mpBuf;
   uint mBufSize;
   uint mBufPos;
};

//---------------------------------------------------------------------------------------------------
// class BDynamicCoderBuf
// This buffer uses a container that grows as bytes are written.
//---------------------------------------------------------------------------------------------------
class BDynamicCoderBuf
{
public:
   BDynamicCoderBuf(int size) : 
      mBufPos(0) 
   { 
      mBuf.resize(size);
   }

   // returns false on failure
   bool outputByte(uchar c)
   {
      if (mBufPos >= mBuf.size())
         mBuf.resize(mBufPos + 1);

      mBuf[mBufPos++] = c;
      return true;
   }

   // returns -1 on failure
   int inputByte(void)
   {
      if (mBufPos >= mBuf.size())
         return 0;
      return mBuf[mBufPos++];
   }

   uint getPos(void) const
   {
      return mBufPos;
   }

   uint getSize(void) const
   {  
      return mBuf.size();
   }

   const uchar* getBuf(void) const 
   {
      return &mBuf[0];
   }

   void setPos(uint pos) 
   {
      mBufPos = pos;
   }

private:
   BDynamicArray<uchar> mBuf;
   uint mBufPos;
};

//---------------------------------------------------------------------------------------------------
// class BBitPacker
// BufferType is the type of buffer object to be used to read/write bytes (for example BStaticCoderBuf or BDynamicCoderBuf). I could have 
// made this an abstract interface but I wanted to avoid the overhead of virtual dispatching.
//---------------------------------------------------------------------------------------------------
template<typename BufferType>
class BBitPacker
{
public:
   typedef BufferType bufferType;
         
   BBitPacker(BufferType* pBuf = NULL) :
      mBitBuf(0),
      mNumBufBits(0),
      mpBuf(pBuf)
   {
   }
   
   BufferType* getBuffer(void) 
   { 
      return mpBuf; 
   }
   
   void setBuffer(BufferType* pBuf)
   {
      mpBuf = pBuf;
   }

   // Starts encoding bits.   
   void encodeStart(void)
   {
      mBitBuf = 0;
      mNumBufBits = 0;
   }
   
   // Encodes 0 to 16 bits.
   // false on failure
   bool encodeSmall(ushort bits, int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));
      BDEBUG_ASSERT(mpBuf);
      
      mBitBuf |= (bits << mNumBufBits);
      mNumBufBits += numBits;
      
      while (mNumBufBits >= 8)
      {
         if (!mpBuf->outputByte(static_cast<uchar>(mBitBuf)))
            return false;
            
         mBitBuf >>= 8;
         mNumBufBits -= 8;
      }

      return true;
   }
   
   // Encodes 0-32 bits.
   // false on failure
   bool encode(uint bits, int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 32));
            
      if (numBits <= 16)
         return encodeSmall(static_cast<ushort>(bits), numBits);
      
      if (!encodeSmall(static_cast<ushort>(bits), 16))
         return false;
         
      return encodeSmall(static_cast<ushort>(bits >> 16), numBits - 16);
   }
   
   // Encodes 0-64 bits.
   // false on failure
   bool encode(uint64 bits, int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 64));

      if (numBits <= 32)
         return encode(static_cast<uint>(bits), numBits);

      if (!encode(static_cast<uint>(bits), 32))
         return false;

      return encode(static_cast<uint>(bits >> 32), numBits - 32);
   }

   // Ends bit encoding by flushing the bit buffer to the next byte boundary, if necessary.
   // false on failure
   bool encodeEnd(void)
   {
      if (!encodeSmall(0, 7))
         return false;
         
      mBitBuf = 0;
      mNumBufBits = 0;
      return true;
   }
   
   // Starts decoding by priming the bit buffer.
   // false on failure
   bool decodeStart(void)
   {
      BDEBUG_ASSERT(mpBuf);
      
      mBitBuf = 0;
      mNumBufBits = 0;
      
      for (int i = 0; i < 4; i++)
      {
         int b = mpBuf->inputByte();
         if (b < 0)
            return false;
            
         mBitBuf |= (b << mNumBufBits);
         mNumBufBits += 8;
      }
      
      mNumBufBits = 16;
      
      return true;
   }
         
   // Removes 0-16 bits from the stream.
   // false on failure.
   bool decodeRemoveBits(int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));
      BDEBUG_ASSERT(mpBuf);
      
      if ((mNumBufBits -= numBits) >= 0)
         mBitBuf >>= numBits;
      else
      {
         mBitBuf >>= (numBits + mNumBufBits);

         const int b0 = mpBuf->inputByte();
         if (b0 < 0)
            return false;
            
         const int b1 = mpBuf->inputByte();
         if (b1 < 0)
            return false;
            
         mBitBuf |= (b0 << 16) | (b1 << 24);

         mBitBuf >>= (-mNumBufBits);

         mNumBufBits += 16;
      }
      
      return true;
   }
   
   // Syncs up the decoder to the next byte boundary
   // false on failure
   bool decodeSyncToNextByte(void)
   {
      if (mNumBufBits & 7)
         return decodeRemoveBits( 7 - (mNumBufBits & 7) );
      return true;
   }
         
   // Decodes and removes the next 0-16 bits.
   // false on failure
   bool decodeSmall(uint& bits, int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));
      
      bits = mBitBuf & ((1 << numBits) - 1);

      return decodeRemoveBits(numBits);
   }
   
   // Look ahead the next 0-16 bits.
   uint decodeLookahead(int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 16));

      return mBitBuf & ((1 << numBits) - 1);
   }
   
   // Decodes and removes 0-32 bits.
   // false on failure
   bool decode(uint& bits, int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 32));
      
      if (numBits <= 16)
         return decodeSmall(bits, numBits);
                       
      if (!decodeSmall(bits, 16))
         return false;
      
      uint upper;
      if (!decodeSmall(upper, numBits - 16))
         return false;
      
      bits |= (upper << 16);
      
      return true;  
   }
   
   // Decodes and removes 0-64 bits.
   // false on failure
   bool decode(uint64& bits, int numBits)
   {
      BDEBUG_ASSERT((numBits >= 0) && (numBits <= 64));

      uint l, h;
      if (numBits <= 32)
      {
         if (!decode(l, numBits))
            return false;
         
         bits = l;
         return true;
      }
      
      if (!decode(l, 32))
         return false;
         
      if (!decode(h, numBits - 32))
         return false;
      
      bits = l | (static_cast<uint64>(h) << 32);

      return true;  
   }

private:
   BufferType* mpBuf;
   uint mBitBuf;
   int mNumBufBits;
};
