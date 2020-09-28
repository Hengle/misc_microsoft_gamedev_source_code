// File: dxtBlockPacker.h
#pragma once

// File: memoryPacker.h
#pragma once

class BDXTBlockPackerBase
{
public:
   static uint getMinimumCompressedDWORDs(uint numSrcDWORDs) { return Math::Max(numSrcDWORDs, 2U); }

   static uint numDstDWORDs(const void* pCompressedData);

protected:
   BDXTBlockPackerBase()
   {
   }
   
   void clearColorDict()
   {
      Utils::ClearObj(mColorDict);
      Utils::ClearObj(mColorSelectorDict);   
   }
   
   void clearAlphaDict()
   {
      Utils::ClearObj(mAlphaDict);
      Utils::ClearObj(mAlphaSelectorDict);   
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
      cHeaderFlagsMagic      = 0x0000156E,
      cHeaderFlagsMagicMask  = 0x0000FFFF,
   };    

   enum
   {
      cRepeatValue       = 0,     // 10
      cRepeatCode        = 1,
      cRepeatCodeSize    = 2,
      
      cFullMatch         = 1,     // 0
      cFullMatchCode     = 3,
      cFullMatchCodeSize = 2,
      
      cUnmatched         = 2,     // 11
      cUnmatchedCode     = 0,
      cUnmatchedCodeSize = 1
   };

   struct BCompressedHeader
   {
      uint mFlags;
      uint mUncompressedDWORDs;
      uint mNumBitsQWORDs;
      uint mNumUnmatchedDWORDs;
   };
   
   enum 
   { 
      cDictBits = 7U, 
      cDictSize = 1U << cDictBits,
      cDictSizeMask = cDictSize - 1U
   };

   uint mColorDict[cDictSize];
   uint mColorSelectorDict[cDictSize];
   
   uint16 mAlphaDict[cDictSize];
   uint64 mAlphaSelectorDict[cDictSize];
   
   static inline uint hashDXT1(uint value)
   {
      value ^= (value >> 16);
      value -= (value >> 8);
      value = ((value >> 7) ^ value) & cDictSizeMask;
      return value;
   }
   
   static inline uint hashDXT5Alpha(uint value)
   {
      value -= (value >> 8);
      value = ((value >> 7) ^ value) & cDictSizeMask;
      return value;
   }
   
   static inline uint hashDXT5AlphaSelector(uint64 value)
   {
      value ^= (value >> 32);
      value ^= (value >> 16);
      value -= (value >> 8);
      value = ((value >> 7) ^ value) & cDictSizeMask;
      return (uint)value;
   }
      
   //inline bool encodeBits(uint64 bits, uint num);
   //inline bool flushBits();
};

class BDXTBlockPacker : public BDXTBlockPackerBase
{
public:
   BDXTBlockPacker() : BDXTBlockPackerBase() { }
   
   enum 
   {
      cDataTypeDXT1,
      cDataTypeDXT5,
      cDataTypeDXN
   };

   bool pack(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, uint dataType, bool flushDictionary = true);
   bool unpack(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary = true);
   
private:
   bool packDXT1(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary);   
   bool packDXT5(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary);   
   bool packDXN(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary);   
   
   bool unpackDXT1(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary);
   bool unpackDXT5(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary);
   bool unpackDXN(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary);
};
