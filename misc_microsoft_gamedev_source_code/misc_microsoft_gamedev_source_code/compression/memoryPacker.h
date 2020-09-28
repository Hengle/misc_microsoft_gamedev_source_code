// File: memoryPacker.h
#pragma once

class BMemoryPackerBase
{
public:
   static uint getMinimumCompressedDWORDs(uint numSrcDWORDs) { return sizeof(BUncompressedHeader) / sizeof(DWORD) + Math::Max(numSrcDWORDs, 2U); }
   
   static uint numDstDWORDs(const void* pCompressedData);
      
protected:
   BMemoryPackerBase()
   {
      Utils::ClearObj(mDictPartial);
      Utils::ClearObj(mDictFull);
   }

   enum 
   {
      cPartialMatchBitsLow = 10U,
      cPartialMatchBitsHigh = 32U - cPartialMatchBitsLow,
      cPartialMatchMaskLow = (1U << cPartialMatchBitsLow) - 1U,
      cPartialMatchMaskHigh = ~cPartialMatchMaskLow
   };

   enum
   {
      cHeaderFlagsMagic      = 0x0000127E,
      cHeaderFlagsMagicMask  = 0x0000FFFF,
      cHeaderFlagsCompressed = 0x00010000
   };    

   enum
   {
      cCodeBits      = 2,
      cCodeMask      = ((1U << cCodeBits) - 1U),
      cZeroValue     = 0,
      cFullMatch     = 1,
      cPartialMatch  = 2,
      cUnmatched     = 3
   };

   struct BCompressedHeader
   {
      uint mFlags;
      uint mUncompressedDWORDs;
      uint mNumBitsQWORDs;
      uint mNumUnmatchedDWORDs;
      
      void endianSwitch(void)
      {
         EndianSwitchDWords((DWORD*)this, 4);
      }
   };

   struct BUncompressedHeader
   {
      uint mFlags;
      uint mUncompressedDWORDs;
   };

   enum 
   { 
      cDictBits = 4U, 
      cDictSize = 1U << cDictBits,
      cDictSizeMask = cDictSize - 1U
   };

   DWORD mDictPartial[cDictSize];
   DWORD mDictFull[cDictSize];
};

class BMemoryPacker : public BMemoryPackerBase
{
public:
   BMemoryPacker() : BMemoryPackerBase() { }

   bool pack(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary = true, bool failIfUncompressible = false);
};

class BMemoryUnpacker : public BMemoryPackerBase
{
public:
   BMemoryUnpacker() : BMemoryPackerBase() { }

   bool unpack(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary = true);
   
   bool unpackEndianSwap(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary = true);
}; 
