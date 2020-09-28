// File: dxtBlockPacker.cpp
#include "compression.h"
#include "dxtBlockPacker.h"

uint BDXTBlockPackerBase::numDstDWORDs(const void* pCompressedData)
{
   uint headerFlags = *reinterpret_cast<const uint*>(pCompressedData);
   if ((headerFlags & cHeaderFlagsMagicMask) != cHeaderFlagsMagic)
      return 0;

   const uint numUncompressedDWORDs = reinterpret_cast<const uint*>(pCompressedData)[1];
   return numUncompressedDWORDs;
}

#define encodeBits(b, n) do { \
   uint64 bits = b; uint num = n; \
   BDEBUG_ASSERT( (num >= 1U) && (num <= 64U) ); \
   BDEBUG_ASSERT( (num == 64) || (bits < (((uint64)1U) << num)) ); \
   do \
   { \
      curBits |= (bits << curNumBits); \
      const uint numBitsLeft = 64U - curNumBits; \
      const uint numBitsEncoded = Math::Min<uint>(numBitsLeft, num); \
      curNumBits += numBitsEncoded; \
      num -= numBitsEncoded; \
      bits >>= numBitsEncoded; \
      if (curNumBits == 64U) { \
         if ((uchar*)pBits >= ((uchar*)pUnmatched - sizeof(uint))) return false; \
         *pBits++ = curBits; \
         curBits = 0; \
         curNumBits = 0; \
      } \
   } while (num > 0); \
} while(0)

#define encodeBits2(b0, n0, b1, n1) do { \
   uint64 bits0 = b0; uint num0 = n0; \
   uint64 bits1 = b1; uint num1 = n1; \
   encodeBits(bits0 | (bits1 << num0), num0 + num1); \
} while(0);

#define flushBits() do { \
   if ((uchar*)pBits >= ((uchar*)pUnmatched - sizeof(uint))) return false; \
   if (curNumBits) \
   { \
      *pBits++ = curBits; \
      curBits = 0; \
      curNumBits = 0; \
   } \
} while(0)

bool BDXTBlockPacker::pack(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, uint dataType, bool flushDictionary)
{
   switch (dataType)
   {
      case cDataTypeDXT1:
         return packDXT1(pSrc, numSrcDWORDs, pDst, numDstDWORDs, flushDictionary);
      case cDataTypeDXT5:
         return packDXT5(pSrc, numSrcDWORDs, pDst, numDstDWORDs, flushDictionary);
      case cDataTypeDXN:
         return packDXN(pSrc, numSrcDWORDs, pDst, numDstDWORDs, flushDictionary);
   }
   return false;
}

bool BDXTBlockPacker::packDXT1(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   if (numDstDWORDs < getMinimumCompressedDWORDs(numSrcDWORDs))
      return false;
   
   if (numSrcDWORDs & 1)
      return false;

   uint64* pBits;
   uint32* pUnmatched;
   uint64 curBits;
   uint curNumBits;
   
   pBits = reinterpret_cast<uint64*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));
   pUnmatched = reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pDst) + (numDstDWORDs - 1) * sizeof(DWORD));            

   curBits = 0;
   curNumBits = 0;

   if (flushDictionary)
      clearColorDict();
    
   uint32 prevColor = 0;
   uint32 prevSelector = 0; 
      
   for (uint srcIndex = 0; srcIndex < numSrcDWORDs; srcIndex += 2)
   {
      const uint srcColor = pSrc[srcIndex];
      const uint srcSelector = pSrc[srcIndex + 1];

      if ((srcColor == prevColor) && (srcSelector == prevSelector))
      {
         encodeBits(cRepeatCode, cRepeatCodeSize);
      }
      else
      {
         const uint colorHash = hashDXT1(srcColor);
         
         if (mColorDict[colorHash] == srcColor)
         {
            encodeBits2(cFullMatchCode, cFullMatchCodeSize, colorHash, cDictBits);
         }
         else
         {
            mColorDict[colorHash] = srcColor;

            encodeBits(cUnmatchedCode, cUnmatchedCodeSize);
               
            if ((uchar*)pBits >= ((uchar*)pUnmatched - sizeof(uint)))
               break;
               
            *pUnmatched-- = srcColor;
         }
         
         const uint selectorHash = hashDXT1(srcSelector);
         
         if (mColorSelectorDict[selectorHash] == srcSelector)
         {
            encodeBits2(0, 1, selectorHash, cDictBits);
         }
         else
         {
            mColorSelectorDict[selectorHash] = srcSelector;

            encodeBits(1, 1);

            if ((uchar*)pBits >= ((uchar*)pUnmatched - sizeof(uint)))
               break;

            *pUnmatched-- = srcSelector;
         }
         
         prevColor = srcColor;
         prevSelector = srcSelector;
      }
   }
   
   flushBits();
      
   const uint unmatchedSizeInBytes = reinterpret_cast<uchar*>(reinterpret_cast<uchar*>(pDst) + (numDstDWORDs - 1) * sizeof(DWORD)) - reinterpret_cast<uchar*>(pUnmatched);
   const uint bitsSizeInBytes = reinterpret_cast<uchar*>(pBits) - reinterpret_cast<uchar*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));

   uint srcSize = numSrcDWORDs * sizeof(DWORD);
   uint compSize = sizeof(BCompressedHeader) + bitsSizeInBytes + unmatchedSizeInBytes;
   if (compSize >= srcSize)
      return false;

   BCompressedHeader* pHeader = reinterpret_cast<BCompressedHeader*>(pDst);
   pHeader->mFlags = cHeaderFlagsMagic | (cDataTypeDXT1 << 16U);
   pHeader->mUncompressedDWORDs = numSrcDWORDs;
   pHeader->mNumBitsQWORDs = bitsSizeInBytes / sizeof(uint64);

   const uint numUnmatchedDWORDs = unmatchedSizeInBytes / sizeof(uint32);
   pHeader->mNumUnmatchedDWORDs = numUnmatchedDWORDs;

   uint* pUnmatchedDWORDs = reinterpret_cast<uint*>(pBits);
   memmove(pUnmatchedDWORDs, pUnmatched + 1, unmatchedSizeInBytes);

   numDstDWORDs = compSize / sizeof(DWORD);

   return true;
}

bool BDXTBlockPacker::packDXT5(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   if (numDstDWORDs < getMinimumCompressedDWORDs(numSrcDWORDs))
      return false;

   if (numSrcDWORDs & 3)
      return false;

   uint64* pBits;
   uint32* pUnmatched;
   uint64 curBits;
   uint curNumBits;
   
   pBits = reinterpret_cast<uint64*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));
   pUnmatched = reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pDst) + (numDstDWORDs - 1) * sizeof(DWORD));            

   curBits = 0;
   curNumBits = 0;

   if (flushDictionary)
   {
      clearColorDict();
      clearAlphaDict();
   }

   uint32 prevColor = 0;
   uint32 prevSelector = 0; 
   uint32 prevAlpha = 0;
   uint64 prevAlphaSelector = 0; 

   for (uint srcIndex = 0; srcIndex < numSrcDWORDs; srcIndex += 4)
   {
      const uint16* pSrc16 = reinterpret_cast<const uint16*>(pSrc + srcIndex);
      const uint32 srcAlpha = pSrc16[0];
      const uint64 srcAlphaSelector = ((uint64)((uint32)pSrc16[1])) | (((uint64)((uint32)pSrc16[2])) << 16U) | (((uint64)((uint32)pSrc16[3])) << 32U);
            
      if ((srcAlpha == prevAlpha) && (srcAlphaSelector == prevAlphaSelector))
      {
         encodeBits(cRepeatCode, cRepeatCodeSize);
      }
      else
      {
         const uint alphaHash = hashDXT5Alpha(srcAlpha);

         if (mAlphaDict[alphaHash] == srcAlpha)
         {
            encodeBits2(cFullMatchCode, cFullMatchCodeSize, alphaHash, cDictBits);
         }
         else
         {
            mAlphaDict[alphaHash] = (uint16)srcAlpha;
                        
            encodeBits2(cUnmatchedCode, cUnmatchedCodeSize, srcAlpha, 16);
         }

         const uint alphaSelectorHash = hashDXT5AlphaSelector(srcAlphaSelector);

         if (mAlphaSelectorDict[alphaSelectorHash] == srcAlphaSelector)
         {
            encodeBits2(0, 1, alphaSelectorHash, cDictBits);
         }
         else
         {
            mAlphaSelectorDict[alphaSelectorHash] = srcAlphaSelector;
                        
            encodeBits2(1, 1, srcAlphaSelector, 48);
         }

         prevAlpha = srcAlpha;
         prevAlphaSelector = srcAlphaSelector;
      }         

      const uint srcColor = pSrc[srcIndex + 2];
      const uint srcSelector = pSrc[srcIndex + 3];
      
      if ((srcColor == prevColor) && (srcSelector == prevSelector))
      {
         encodeBits(cRepeatCode, cRepeatCodeSize);
      }
      else
      {
         const uint colorHash = hashDXT1(srcColor);

         if (mColorDict[colorHash] == srcColor)
         {
            encodeBits2(cFullMatchCode, cFullMatchCodeSize, colorHash, cDictBits);
         }
         else
         {
            mColorDict[colorHash] = srcColor;

            encodeBits(cUnmatchedCode, cUnmatchedCodeSize);

            if ((uchar*)pBits >= ((uchar*)pUnmatched - sizeof(uint)))
               break;

            *pUnmatched-- = srcColor;
         }

         const uint selectorHash = hashDXT1(srcSelector);

         if (mColorSelectorDict[selectorHash] == srcSelector)
         {
            encodeBits2(0, 1, selectorHash, cDictBits);
         }
         else
         {
            mColorSelectorDict[selectorHash] = srcSelector;

            encodeBits(1, 1);

            if ((uchar*)pBits >= ((uchar*)pUnmatched - sizeof(uint)))
               break;

            *pUnmatched-- = srcSelector;
         }

         prevColor = srcColor;
         prevSelector = srcSelector;
      }
   }

   flushBits();
      
   const uint unmatchedSizeInBytes = reinterpret_cast<uchar*>(reinterpret_cast<uchar*>(pDst) + (numDstDWORDs - 1) * sizeof(DWORD)) - reinterpret_cast<uchar*>(pUnmatched);
   const uint bitsSizeInBytes = reinterpret_cast<uchar*>(pBits) - reinterpret_cast<uchar*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));

   uint srcSize = numSrcDWORDs * sizeof(DWORD);
   uint compSize = sizeof(BCompressedHeader) + bitsSizeInBytes + unmatchedSizeInBytes;
   if (compSize >= srcSize)
      return false;

   BCompressedHeader* pHeader = reinterpret_cast<BCompressedHeader*>(pDst);
   pHeader->mFlags = cHeaderFlagsMagic | (cDataTypeDXT5 << 16U);
   pHeader->mUncompressedDWORDs = numSrcDWORDs;
   pHeader->mNumBitsQWORDs = bitsSizeInBytes / sizeof(uint64);

   const uint numUnmatchedDWORDs = unmatchedSizeInBytes / sizeof(uint32);
   pHeader->mNumUnmatchedDWORDs = numUnmatchedDWORDs;

   uint* pUnmatchedDWORDs = reinterpret_cast<uint*>(pBits);
   memmove(pUnmatchedDWORDs, pUnmatched + 1, unmatchedSizeInBytes);

   numDstDWORDs = compSize / sizeof(DWORD);

   return true;
}

bool BDXTBlockPacker::packDXN(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   if (numDstDWORDs < getMinimumCompressedDWORDs(numSrcDWORDs))
      return false;

   if (numSrcDWORDs & 1)
      return false;

   uint64* pBits;
   uint32* pUnmatched;
   uint64 curBits;
   uint curNumBits;
   
   pBits = reinterpret_cast<uint64*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));
   pUnmatched = reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pDst) + (numDstDWORDs - 1) * sizeof(DWORD));            

   curBits = 0;
   curNumBits = 0;

   if (flushDictionary)
      clearAlphaDict();

   uint32 prevAlpha = 0;
   uint64 prevAlphaSelector = 0; 
      
   for (uint srcIndex = 0; srcIndex < numSrcDWORDs; srcIndex += 2)
   {
      const uint16* pSrc16 = reinterpret_cast<const uint16*>(pSrc + srcIndex);
      const uint32 srcAlpha = pSrc16[0];
      const uint64 srcAlphaSelector = ((uint64)((uint32)pSrc16[1])) | (((uint64)((uint32)pSrc16[2])) << 16U) | (((uint64)((uint32)pSrc16[3])) << 32U);

      if ((srcAlpha == prevAlpha) && (srcAlphaSelector == prevAlphaSelector))
      {
         encodeBits(cRepeatCode, cRepeatCodeSize);
      }
      else
      {
         const uint alphaHash = hashDXT5Alpha(srcAlpha);

         if (mAlphaDict[alphaHash] == srcAlpha)
         {
            encodeBits2(cFullMatchCode, cFullMatchCodeSize, alphaHash, cDictBits);
         }
         else
         {
            mAlphaDict[alphaHash] = (uint16)srcAlpha;

            encodeBits2(cUnmatchedCode, cUnmatchedCodeSize, srcAlpha, 16);
         }

         const uint alphaSelectorHash = hashDXT5AlphaSelector(srcAlphaSelector);

         if (mAlphaSelectorDict[alphaSelectorHash] == srcAlphaSelector)
         {
            encodeBits2(0, 1, alphaSelectorHash, cDictBits);
         }
         else
         {
            mAlphaSelectorDict[alphaSelectorHash] = srcAlphaSelector;

            encodeBits2(1, 1, srcAlphaSelector, 48);
         }

         prevAlpha = srcAlpha;
         prevAlphaSelector = srcAlphaSelector;
      }
   }

   flushBits();
      
   const uint bitsSizeInBytes = reinterpret_cast<uchar*>(pBits) - reinterpret_cast<uchar*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));

   uint srcSize = numSrcDWORDs * sizeof(DWORD);
   uint compSize = sizeof(BCompressedHeader) + bitsSizeInBytes;
   if (compSize >= srcSize)
      return false;
      
   BCompressedHeader* pHeader = reinterpret_cast<BCompressedHeader*>(pDst);
   pHeader->mFlags = cHeaderFlagsMagic | (cDataTypeDXN << 16U);
   pHeader->mUncompressedDWORDs = numSrcDWORDs;
   pHeader->mNumBitsQWORDs = bitsSizeInBytes / sizeof(uint64);

   pHeader->mNumUnmatchedDWORDs = 0;

   numDstDWORDs = compSize / sizeof(DWORD);

   return true;
}

#define GET_BIT(result) \
if (!curBitsLeft) \
{ \
   curBits = *pSrcBits++; \
   curBitsLeft = 64U; \
} \
result = (uint)(curBits & 1); \
curBits >>= 1; \
curBitsLeft--;

#define GET_BITS32(result, mask, num) \
if (curBitsLeft >= num) \
{ \
   result = static_cast<uint>(curBits) & mask; \
   curBits >>= num; \
   curBitsLeft -= num; \
} else { \
   result = static_cast<uint>(curBits); \
   uint numBitsRemaining = num - curBitsLeft; \
   curBits = *pSrcBits++; \
   result = (uint)((result | (curBits << curBitsLeft)) & mask); \
   curBitsLeft = 64U - numBitsRemaining; \
   curBits >>= numBitsRemaining; \
}

#define GET_BITS64(result, mask, num) \
if (curBitsLeft >= num) \
{ \
   result = curBits & mask; \
   curBits >>= num; \
   curBitsLeft -= num; \
} else { \
   result = curBits; \
   uint numBitsRemaining = num - curBitsLeft; \
   curBits = *pSrcBits++; \
   result = (result | (curBits << curBitsLeft)) & mask; \
   curBitsLeft = 64U - numBitsRemaining; \
   curBits >>= numBitsRemaining; \
}

#define GET_ENCODING_TYPE(encodingType) \
{ \
   encodingType = cUnmatched; \
   uint firstBit; \
   GET_BIT(firstBit); \
   if (firstBit) { \
      uint secondBit; \
      GET_BIT(secondBit); \
      encodingType = secondBit ? cFullMatch : cRepeatValue; \
   } \
} 

bool BDXTBlockPacker::unpack(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   if (numSrcDWORDs < (sizeof(BCompressedHeader) / sizeof(DWORD)))
   {
      numDstDWORDs = 0;
      return false;
   }
   
   const uint numUncompressedDWORDs = reinterpret_cast<const uint*>(pSrc)[1];
   if (numUncompressedDWORDs > numDstDWORDs)
   {
      numDstDWORDs = numUncompressedDWORDs;
      return false;
   }
      
   const BCompressedHeader* pHeader = reinterpret_cast<const BCompressedHeader*>(pSrc);
   if (!pHeader->mNumBitsQWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }
   
   if ((pHeader->mFlags & cHeaderFlagsMagicMask) != cHeaderFlagsMagic)
   {
      numDstDWORDs = 0;
      return false;
   }
      
   if (pHeader->mNumUnmatchedDWORDs > numUncompressedDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }
      
   if (numDstDWORDs & 1)
   {
      numDstDWORDs = 0;
      return false;
   }
   
   switch (pHeader->mFlags >> 16)
   {
      case cDataTypeDXT1: return unpackDXT1(pSrc, numSrcDWORDs, pDst, numDstDWORDs, flushDictionary); 
      case cDataTypeDXT5: return unpackDXT5(pSrc, numSrcDWORDs, pDst, numDstDWORDs, flushDictionary); 
      case cDataTypeDXN: return unpackDXN(pSrc, numSrcDWORDs, pDst, numDstDWORDs, flushDictionary); 
   }
   
   return false;
}

bool BDXTBlockPacker::unpackDXT1(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   const BCompressedHeader* pHeader = reinterpret_cast<const BCompressedHeader*>(pSrc);
   const uint numUncompressedDWORDs = reinterpret_cast<const uint*>(pSrc)[1];
   
   const uint64* pSrcBits = reinterpret_cast<const uint64*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader));
   const uint* pUnmatched = reinterpret_cast<const uint*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader) + pHeader->mNumBitsQWORDs * sizeof(uint64)) + pHeader->mNumUnmatchedDWORDs - 1;
   if (((reinterpret_cast<const uchar*>(pUnmatched) - reinterpret_cast<const uchar*>(pSrc)) / sizeof(DWORD)) >= numSrcDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   uint* pDstDWORD = reinterpret_cast<uint*>(pDst);
   
   uint numDWORDsLeft = numUncompressedDWORDs;

   uint64 curBits = 0;
   uint curBitsLeft = 0;

   if (flushDictionary)
      clearColorDict();

   uint32 prevColor = 0;
   uint32 prevColorSelector = 0;
   
   while (numDWORDsLeft)
   {
      // Color
      
      uint encodingType;
      GET_ENCODING_TYPE(encodingType);
      
      if (encodingType == cRepeatValue)
      {
         *pDstDWORD++ = prevColor;
         *pDstDWORD++ = prevColorSelector;
      }
      else
      {
         uint colorValue;
         if (encodingType == cFullMatch)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            colorValue = mColorDict[dictIndex];
         }
         else
         {
            // unmatched
            colorValue = *pUnmatched--;
            uint colorHash = hashDXT1(colorValue);
            mColorDict[colorHash] = colorValue;
         }
         
         *pDstDWORD++ = colorValue;
         prevColor = colorValue;
      
         // Selectors
         GET_BIT(encodingType);
         uint colorSelectorValue;
         if (!encodingType)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            colorSelectorValue = mColorSelectorDict[dictIndex];
         }
         else
         {
            // unmatched
            colorSelectorValue = *pUnmatched--;
            uint colorSelectorHash = hashDXT1(colorSelectorValue);
            mColorSelectorDict[colorSelectorHash] = colorSelectorValue;
         }
         
         *pDstDWORD++ = colorSelectorValue;
         prevColorSelector = colorSelectorValue;
      }
      
      numDWORDsLeft -= 2;
   }

   numDstDWORDs = numUncompressedDWORDs;
   return true;
}

bool BDXTBlockPacker::unpackDXT5(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   const BCompressedHeader* pHeader = reinterpret_cast<const BCompressedHeader*>(pSrc);
   const uint numUncompressedDWORDs = reinterpret_cast<const uint*>(pSrc)[1];
   
   const uint64* pSrcBits = reinterpret_cast<const uint64*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader));
   const uint* pUnmatched = reinterpret_cast<const uint*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader) + pHeader->mNumBitsQWORDs * sizeof(uint64)) + pHeader->mNumUnmatchedDWORDs - 1;
   if (((reinterpret_cast<const uchar*>(pUnmatched) - reinterpret_cast<const uchar*>(pSrc)) / sizeof(DWORD)) >= numSrcDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   uint* pDstDWORD = reinterpret_cast<uint*>(pDst);
   uint numDWORDsLeft = numUncompressedDWORDs;
   if (numDstDWORDs & 3)
   {
      numDstDWORDs = 0;
      return false;
   }

   uint64 curBits = 0;
   uint curBitsLeft = 0;

   if (flushDictionary)
   {
      clearColorDict();
      clearAlphaDict();
   }

   uint32 prevColor = 0;
   uint32 prevColorSelector = 0;
   uint32 prevAlpha = 0;
   uint64 prevAlphaSelector = 0;
   
   while (numDWORDsLeft)
   {
      uint16* pDst16 = reinterpret_cast<uint16*>(pDstDWORD);
      
      // Alpha
      uint encodingType;
      GET_ENCODING_TYPE(encodingType);
      if (encodingType == cRepeatValue)
      {
         *pDst16++ = (uint16)prevAlpha;
         *pDst16++ = (uint16)prevAlphaSelector;
         *pDst16++ = (uint16)(prevAlphaSelector >> 16U);
         *pDst16++ = (uint16)(prevAlphaSelector >> 32U);
      }
      else
      {
         uint alphaValue;
         if (encodingType == cFullMatch)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            
            alphaValue = mAlphaDict[dictIndex];
         }
         else
         {
            // unmatched
            GET_BITS32(alphaValue, 0xFFFF, 16);
            
            uint alphaHash = hashDXT5Alpha(alphaValue);
            mAlphaDict[alphaHash] = (uint16)alphaValue;
         }
         *pDst16++ = (uint16)alphaValue;
         prevAlpha = alphaValue;
                  
         GET_BIT(encodingType);
         
         uint64 alphaSelectorValue;
         if (!encodingType)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            alphaSelectorValue = mAlphaSelectorDict[dictIndex];
         }
         else
         {
            // unmatched
            GET_BITS64(alphaSelectorValue, 0xFFFFFFFFFFFFULL, 48U);
            uint alphaSelectorHash = hashDXT5AlphaSelector(alphaSelectorValue);
            mAlphaSelectorDict[alphaSelectorHash] = alphaSelectorValue;
         }
         
         *pDst16++ = (uint16)alphaSelectorValue;
         *pDst16++ = (uint16)(alphaSelectorValue >> 16U);
         *pDst16++ = (uint16)(alphaSelectorValue >> 32U);

         prevAlphaSelector = alphaSelectorValue;
      }
      
      pDstDWORD += 2;
   
      // Color
      GET_ENCODING_TYPE(encodingType);
      if (encodingType == cRepeatValue)
      {
         *pDstDWORD++ = prevColor;
         *pDstDWORD++ = prevColorSelector;
      }
      else
      {
         uint colorValue;
         if (encodingType == cFullMatch)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            colorValue = mColorDict[dictIndex];
         }
         else
         {
            // unmatched
            colorValue = *pUnmatched--;
            uint colorHash = hashDXT1(colorValue);
            mColorDict[colorHash] = colorValue;
         }
         
         *pDstDWORD++ = colorValue;
         prevColor = colorValue;
      
         // Selectors
         GET_BIT(encodingType);
         uint colorSelectorValue;
         if (!encodingType)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            colorSelectorValue = mColorSelectorDict[dictIndex];
         }
         else
         {
            // unmatched
            colorSelectorValue = *pUnmatched--;
            uint colorSelectorHash = hashDXT1(colorSelectorValue);
            mColorSelectorDict[colorSelectorHash] = colorSelectorValue;
         }
         
         *pDstDWORD++ = colorSelectorValue;
         prevColorSelector = colorSelectorValue;
      }
            
      numDWORDsLeft -= 4;
   }

   numDstDWORDs = numUncompressedDWORDs;
   return true;
}

bool BDXTBlockPacker::unpackDXN(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   const BCompressedHeader* pHeader = reinterpret_cast<const BCompressedHeader*>(pSrc);
   const uint numUncompressedDWORDs = reinterpret_cast<const uint*>(pSrc)[1];
   
   const uint64* pSrcBits = reinterpret_cast<const uint64*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader));
   const uint* pUnmatched = reinterpret_cast<const uint*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader) + pHeader->mNumBitsQWORDs * sizeof(uint64)) + pHeader->mNumUnmatchedDWORDs - 1;
   if (((reinterpret_cast<const uchar*>(pUnmatched) - reinterpret_cast<const uchar*>(pSrc)) / sizeof(DWORD)) >= numSrcDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   uint numDWORDsLeft = numUncompressedDWORDs;

   uint64 curBits = 0;
   uint curBitsLeft = 0;

   if (flushDictionary)
      clearAlphaDict();

   uint32 prevAlpha = 0;
   uint64 prevAlphaSelector = 0;
   
   uint16* pDst16 = reinterpret_cast<uint16*>(pDst);
   
   while (numDWORDsLeft)
   {
      // Alpha
      uint encodingType;
      GET_ENCODING_TYPE(encodingType);
      if (encodingType == cRepeatValue)
      {
         *pDst16++ = (uint16)prevAlpha;
         *pDst16++ = (uint16)prevAlphaSelector;
         *pDst16++ = (uint16)(prevAlphaSelector >> 16U);
         *pDst16++ = (uint16)(prevAlphaSelector >> 32U);
      }
      else
      {
         uint alphaValue;
         if (encodingType == cFullMatch)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            alphaValue = mAlphaDict[dictIndex];
         }
         else
         {
            // unmatched
            GET_BITS32(alphaValue, 0xFFFF, 16);
            uint alphaHash = hashDXT5Alpha(alphaValue);
            mAlphaDict[alphaHash] = (uint16)alphaValue;
         }
         *pDst16++ = (uint16)alphaValue;
         prevAlpha = alphaValue;
                  
         GET_BIT(encodingType);
         
         uint64 alphaSelectorValue;
         if (!encodingType)
         {
            // full
            uint dictIndex;
            GET_BITS32(dictIndex, cDictSizeMask, cDictBits);
            alphaSelectorValue = mAlphaSelectorDict[dictIndex];
         }
         else
         {
            // unmatched
            GET_BITS64(alphaSelectorValue, 0xFFFFFFFFFFFFULL, 48U);
            uint alphaSelectorHash = hashDXT5AlphaSelector(alphaSelectorValue);
            mAlphaSelectorDict[alphaSelectorHash] = alphaSelectorValue;
         }
         
         *pDst16++ = (uint16)alphaSelectorValue;
         *pDst16++ = (uint16)(alphaSelectorValue >> 16U);
         *pDst16++ = (uint16)(alphaSelectorValue >> 32U);
         prevAlphaSelector = alphaSelectorValue;
      }
                             
      numDWORDsLeft -= 2;
   }

   numDstDWORDs = numUncompressedDWORDs;
   return true;
}