// File: bitcoder.h
#pragma once

class BBitCoder
{
public:
   BBitCoder();

   void encodeStart(void);
   void encodeBit(uint bit);
   void encodeCode(uint code, uint codeSize);
   void encodeEnd(void);
   void encodeAlignToByte(void);
   
   void decodeStart(uint bitsOfs = 0);
   uint decodeBit(void);
   uint decodeCode(uint codeSize);
   uint decodeEnd(void);
   
   void setBuf(BByteArray* pBits);
   
   BByteArray* getBuf(void) const { return mpBits; }

private:
   BByteArray mBits;
   BByteArray* mpBits;
   uint mBitsOfs;
   uint64 mBitBuf;
   uint mBitBufSize;

   void flushBitBuf(bool force = false);
};
