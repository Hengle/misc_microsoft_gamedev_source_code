// File: bitcoder.cpp
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include "bitcoder.h"

BBitCoder::BBitCoder() :
   mBitBuf(0),
   mBitBufSize(0),
   mBitsOfs(0),
   mpBits(&mBits)
{
}

void BBitCoder::encodeStart(void)
{
   mBitBuf = 0;
   mBitBufSize = 0;
}

void BBitCoder::encodeBit(uint bit)
{
   BDEBUG_ASSERT(bit < 2);
   
   mBitBuf |= (static_cast<uint64>(bit) << mBitBufSize);
   mBitBufSize++;
   if (mBitBufSize == 32)
      flushBitBuf();
}

void BBitCoder::encodeCode(uint code, uint codeSize)
{
   BDEBUG_ASSERT((codeSize > 0) && (codeSize <= 32));
   BDEBUG_ASSERT(code < (1U << codeSize));

   mBitBuf |= (static_cast<uint64>(code) << mBitBufSize);
   mBitBufSize += codeSize;
   if (mBitBufSize >= 32)
      flushBitBuf();
}
   
void BBitCoder::encodeEnd(void)
{
   flushBitBuf(true);
}

void BBitCoder::encodeAlignToByte(void)
{
   if (mBitBufSize & 7)
      encodeCode(0, 8 - (mBitBufSize & 7));
}

void BBitCoder::setBuf(BByteArray* pBits)
{ 
   mpBits = pBits ? pBits : &mBits; 
}

void BBitCoder::decodeStart(uint bitsOfs)
{
   mBitsOfs = bitsOfs;
   mBitBuf = 0;
   mBitBufSize = 0;
}

uint BBitCoder::decodeBit(void)
{
   if (mBitBufSize == 0)
   {
      uchar c = 0;
      if (mBitsOfs < mpBits->size())
         c = mBits[mBitsOfs++];
      mBitBuf = c;
      mBitBufSize = 8;
   }

   const uint result = static_cast<uint>(mBitBuf & 1);
   mBitBuf >>= 1;
   mBitBufSize--;

   return result;
}

uint BBitCoder::decodeCode(uint codeSize)
{  
   BDEBUG_ASSERT((codeSize > 0) && (codeSize <= 32));

   while (mBitBufSize < codeSize)
   {
      uchar c = 0;
      if (mBitsOfs < mpBits->size())
         c = mBits[mBitsOfs++];
      mBitBuf |= (static_cast<uint64>(c) << mBitBufSize);
      mBitBufSize += 8;
   }

   const uint result = static_cast<uint>(mBitBuf & ((1 << codeSize) - 1));
   mBitBuf >>= codeSize;
   mBitBufSize -= codeSize;

   return result;
}

uint BBitCoder::decodeEnd(void)
{
   return mBitsOfs;
}

void BBitCoder::flushBitBuf(bool force)
{
   while (mBitBufSize >= 8)
   {
      mpBits->pushBack(static_cast<uchar>(mBitBuf & 0xFF));
      mBitBuf >>= 8;
      mBitBufSize -= 8;
   }

   if ((force) && (mBitBufSize))
   {
      mpBits->pushBack(static_cast<uchar>(mBitBuf & 0xFF));
      mBitBuf = 0;
      mBitBufSize = 0;
   }
}

