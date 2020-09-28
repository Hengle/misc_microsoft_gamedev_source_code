// File: memoryPacker.cpp
#include "compression.h"
#include "memoryPacker.h"

uint BMemoryPackerBase::numDstDWORDs(const void* pCompressedData)
{
   uint headerFlags = *reinterpret_cast<const uint*>(pCompressedData);
   if ((headerFlags & cHeaderFlagsMagicMask) != cHeaderFlagsMagic)
      return 0;

   const uint numUncompressedDWORDs = reinterpret_cast<const uint*>(pCompressedData)[1];
   return numUncompressedDWORDs;
}

#define encodeBits(result, b, n) \
{ \
   uint64 bits = b; uint num = n; \
   result = true; \
   do \
   { \
      curBits |= (bits << curNumBits); \
      const uint numBitsLeft = 64U - curNumBits; \
      const uint numBitsEncoded = Math::Min<uint>(numBitsLeft, num); \
      curNumBits += numBitsEncoded; \
      num -= numBitsEncoded; \
      bits >>= numBitsEncoded; \
      if (curNumBits == 64U) \
      { \
         if ((uchar*)pDstBits >= ((uchar*)pUnmatched - sizeof(uint))) \
            result = false; \
         else \
            *pDstBits++ = curBits; \
         curBits = 0; \
         curNumBits = 0; \
      } \
   } while (num > 0); \
}

#define flushBits(result) \
{ \
   result = true; \
   if (curNumBits) \
   { \
      if ((uchar*)pDstBits >= ((uchar*)pUnmatched - sizeof(uint))) \
         result = false; \
      else \
         *pDstBits++ = curBits; \
      curBits = 0; \
      curNumBits = 0; \
   } \
}

bool BMemoryPacker::pack(const DWORD* pSrc, uint numSrcDWORDs, DWORD* pDst, uint& numDstDWORDs, bool flushDictionary, bool failIfUncompressible)
{
   if (numDstDWORDs < getMinimumCompressedDWORDs(numSrcDWORDs))
      return false;
      
   uint64* pDstBits = reinterpret_cast<uint64*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));
   uint32* pUnmatched = reinterpret_cast<uint*>(reinterpret_cast<uchar*>(pDst) + (numDstDWORDs - 1) * sizeof(DWORD));            

   uint64 curBits = 0;
   uint curNumBits = 0;

   if (flushDictionary)
   {
      Utils::ClearObj(mDictPartial);
      Utils::ClearObj(mDictFull);
   }

   Utils::BPrefetchState srcPrefetcher(Utils::BeginPrefetch(pSrc, 3));
   Utils::BPrefetchState dstPrefetcher(Utils::BeginPrefetch(pDst, 3));
   Utils::BPrefetchState unmatchedPrefetcher(Utils::BeginPrefetch(pUnmatched, 3));
   
   for (uint srcIndex = 0; srcIndex < numSrcDWORDs; srcIndex++)
   {
      srcPrefetcher = Utils::UpdatePrefetch(srcPrefetcher, &pSrc[srcIndex], &pSrc[numSrcDWORDs], 3);
      dstPrefetcher = Utils::UpdatePrefetch(dstPrefetcher, pDstBits, 3);
      unmatchedPrefetcher = Utils::UpdatePrefetch(unmatchedPrefetcher, pUnmatched, 3);
      
      const uint srcValue = pSrc[srcIndex];

      if (!srcValue)
      {
         bool result;
         encodeBits(result, cZeroValue, cCodeBits); 
         if (!result) 
            break;
      }
      else
      {
         uint partialHash = (srcValue >> cPartialMatchBitsLow);
         uint fullHash = srcValue;

         partialHash ^= (partialHash >> 16U);
         partialHash -= (partialHash >> 8U);
         partialHash ^= (partialHash >> 4U);
         partialHash &= cDictSizeMask;

         fullHash ^= (fullHash >> 16U);
         fullHash -= (fullHash >> 8U);
         fullHash ^= (fullHash >> 4U);
         fullHash &= cDictSizeMask;

         if (mDictFull[fullHash] == srcValue)
         {
            bool result;
            encodeBits(result, cFullMatch, cCodeBits);
            if (!result)
               break;

            encodeBits(result, fullHash, cDictBits);
            if (!result)
               break;
         }
         else if (mDictPartial[partialHash] == (srcValue & cPartialMatchMaskHigh))
         {
            bool result;
            encodeBits(result, cPartialMatch, cCodeBits);
            if (!result)
               break;
               
            encodeBits(result, partialHash, cDictBits);
            if (!result)
               break;

            encodeBits(result, srcValue & cPartialMatchMaskLow, cPartialMatchBitsLow);
            if (!result)
               break;

            mDictFull[fullHash] = srcValue;
         }
         else
         {
            mDictPartial[partialHash] = srcValue & cPartialMatchMaskHigh;
            mDictFull[fullHash] = srcValue;

            bool result;
            encodeBits(result, cUnmatched, cCodeBits);
            if (!result)
               break;

            if ((uchar*)pDstBits >= ((uchar*)pUnmatched - sizeof(uint)))
               break;
               
            *pUnmatched-- = srcValue;
         }
      }
   }

   bool failed = (uchar*)pDstBits >= ((uchar*)pUnmatched - sizeof(uint));

   if (!failed)
   {
      bool result;
      flushBits(result);
      if (!result)
         failed = true;
   }      

   const uint unmatchedSizeInBytes = reinterpret_cast<uchar*>(reinterpret_cast<uchar*>(pDst) + (numDstDWORDs - 1) * sizeof(DWORD)) - reinterpret_cast<uchar*>(pUnmatched);
   const uint bitsSizeInBytes = reinterpret_cast<uchar*>(pDstBits) - reinterpret_cast<uchar*>(reinterpret_cast<uchar*>(pDst) + sizeof(BCompressedHeader));

   uint srcSize = numSrcDWORDs * sizeof(DWORD);
   uint compSize = sizeof(BCompressedHeader) + bitsSizeInBytes + unmatchedSizeInBytes;
   if (failed || (compSize >= srcSize))
   {
      if (failIfUncompressible)
      {
         numDstDWORDs = 0;
         return false;
      }
      
      uint rawSize = sizeof(BUncompressedHeader) + numSrcDWORDs * sizeof(DWORD);
      if (rawSize / sizeof(DWORD) > numDstDWORDs)
      {  
         numDstDWORDs = rawSize / sizeof(DWORD);
         return false;
      }

      BUncompressedHeader* pHeader = reinterpret_cast<BUncompressedHeader*>(pDst);
      pHeader->mFlags = cHeaderFlagsMagic;
      pHeader->mUncompressedDWORDs = numSrcDWORDs;

      Utils::FastMemCpy(pHeader + 1, pSrc, numSrcDWORDs * sizeof(DWORD));

      numDstDWORDs = rawSize / sizeof(DWORD);
      return true;
   }

   BCompressedHeader* pHeader = reinterpret_cast<BCompressedHeader*>(pDst);
   pHeader->mFlags = cHeaderFlagsMagic | cHeaderFlagsCompressed;
   pHeader->mUncompressedDWORDs = numSrcDWORDs;
   pHeader->mNumBitsQWORDs = bitsSizeInBytes / sizeof(uint64);

   const uint numUnmatchedDWORDs = unmatchedSizeInBytes / sizeof(uint32);
   pHeader->mNumUnmatchedDWORDs = numUnmatchedDWORDs;

   uint* pUnmatchedDWORDs = reinterpret_cast<uint*>(pDstBits);
   memmove(pUnmatchedDWORDs, pUnmatched + 1, unmatchedSizeInBytes);

   numDstDWORDs = compSize / sizeof(DWORD);

   return true;
}


//BDEBUG_ASSERT( (num >= 1U) && (num <= 64U) );
//BDEBUG_ASSERT( (num == 64) || (bits < (((uint64)1U) << num)) );
//BDEBUG_ASSERT( (num & 1) == 0 );

#undef encodeBits
#undef flushBits
   
bool BMemoryUnpacker::unpack(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   if (numSrcDWORDs < (sizeof(BUncompressedHeader) / sizeof(DWORD)))
   {
      numDstDWORDs = 0;
      return false;
   }

   uint headerFlags = *reinterpret_cast<const uint*>(pSrc);
   if ((headerFlags & cHeaderFlagsMagicMask) != cHeaderFlagsMagic)
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

   if ((headerFlags & cHeaderFlagsCompressed) == 0)
   {
      const uint cUncompressedHeaderDWORDs = sizeof(BUncompressedHeader) / sizeof(DWORD);

      if (numSrcDWORDs < (numUncompressedDWORDs + cUncompressedHeaderDWORDs))
      {
         numDstDWORDs = 0;
         return false;
      }

      Utils::FastMemCpy(pDst, reinterpret_cast<const uint*>(pSrc) + cUncompressedHeaderDWORDs, numUncompressedDWORDs * sizeof(DWORD));
      numDstDWORDs = numUncompressedDWORDs;
      return true;
   }

   if (numSrcDWORDs < (sizeof(BCompressedHeader) / sizeof(DWORD)))
   {
      numDstDWORDs = 0;
      return false;
   }

   const BCompressedHeader* pHeader = reinterpret_cast<const BCompressedHeader*>(pSrc);
   if (!pHeader->mNumBitsQWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   const uint64* pSrcBits = reinterpret_cast<const uint64*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader));
   const uint* pUnmatched = reinterpret_cast<const uint*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader) + pHeader->mNumBitsQWORDs * sizeof(uint64)) + pHeader->mNumUnmatchedDWORDs - 1;
   if (((reinterpret_cast<const uchar*>(pUnmatched) - reinterpret_cast<const uchar*>(pSrc)) / sizeof(DWORD)) >= numSrcDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   if (pHeader->mNumUnmatchedDWORDs > numUncompressedDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   uint* pDstDWORD = reinterpret_cast<uint*>(pDst);
   uint numDWORDsLeft = numUncompressedDWORDs;

   uint64 curBits = 0;
   uint curBitsLeft = 0;

   if (flushDictionary)
   {
      Utils::ClearObj(mDictPartial);
      Utils::ClearObj(mDictFull);
   }

#define GET_BITS(result, mask, num) \
if (curBitsLeft >= num) \
   { \
   result = static_cast<uint>(curBits) & mask; \
   curBits >>= num; \
   curBitsLeft -= num; \
   } else { \
   result = static_cast<uint>(curBits); \
   uint numBitsRemaining = num - curBitsLeft; \
   curBits = *pSrcBits++; \
   result = (result | (static_cast<uint>(curBits) << curBitsLeft)) & mask; \
   curBitsLeft = 64U - numBitsRemaining; \
   curBits >>= numBitsRemaining; \
   }
   while (numDWORDsLeft)
   {
      BDEBUG_ASSERT((curBitsLeft & 1) == 0);

      if (!curBitsLeft)
      {
         curBits = *pSrcBits++;
         curBitsLeft = 64U;   
      }

      const uint encodingType = static_cast<uint>(curBits) & cCodeMask;
      curBits >>= cCodeBits;
      curBitsLeft -= cCodeBits;

      switch (encodingType)
      {
         case cZeroValue:
         {
            // zero
            *pDstDWORD++ = 0;
            break;
         }
         case cFullMatch:
         {
            // full
            uint dictIndex;
            GET_BITS(dictIndex, cDictSizeMask, cDictBits);

            *pDstDWORD++ = mDictFull[dictIndex];
            break;
         }
         case cPartialMatch:
         {
            // partial
            uint dictIndex;
            GET_BITS(dictIndex, cDictSizeMask, cDictBits);

            uint lowValue;               
            GET_BITS(lowValue, cPartialMatchMaskLow, cPartialMatchBitsLow);

            uint highValue = mDictPartial[dictIndex];

            uint value = lowValue | highValue;
            *pDstDWORD++ = value;

            uint fullHash = value;
            fullHash ^= (fullHash >> 16U);
            fullHash -= (fullHash >> 8U);
            fullHash ^= (fullHash >> 4U);
            fullHash &= cDictSizeMask;
            mDictFull[fullHash] = value;

            break;
         }
         case cUnmatched:
         {
            // unmatched
            uint unmatchedValue = *pUnmatched--;
            *pDstDWORD++ = unmatchedValue;

            uint partialHash = (unmatchedValue >> cPartialMatchBitsLow);
            uint fullHash = unmatchedValue;

            partialHash ^= (partialHash >> 16U);
            partialHash -= (partialHash >> 8U);
            partialHash ^= (partialHash >> 4U);
            partialHash &= cDictSizeMask;

            fullHash ^= (fullHash >> 16U);
            fullHash -= (fullHash >> 8U);
            fullHash ^= (fullHash >> 4U);
            fullHash &= cDictSizeMask;

            mDictPartial[partialHash] = unmatchedValue & cPartialMatchMaskHigh;
            mDictFull[fullHash] = unmatchedValue;

            break;
         }
      }

      numDWORDsLeft--;
   }

#undef GET_BITS      

   numDstDWORDs = numUncompressedDWORDs;
   return true;
}

bool BMemoryUnpacker::unpackEndianSwap(const void* pSrc, uint numSrcDWORDs, void* pDst, uint& numDstDWORDs, bool flushDictionary)
{
   if (numSrcDWORDs < (sizeof(BUncompressedHeader) / sizeof(DWORD)))
   {
      numDstDWORDs = 0;
      return false;
   }

   uint headerFlags = *reinterpret_cast<const uint*>(pSrc);
   EndianSwitchDWords((DWORD*)&headerFlags, 1);
   if ((headerFlags & cHeaderFlagsMagicMask) != cHeaderFlagsMagic)
   {
      numDstDWORDs = 0;
      return false;
   }

   uint numUncompressedDWORDs = reinterpret_cast<const uint*>(pSrc)[1];
   EndianSwitchDWords((DWORD*)&numUncompressedDWORDs , 1);
   if (numUncompressedDWORDs > numDstDWORDs)
   {
      numDstDWORDs = numUncompressedDWORDs;
      return false;
   }

   if ((headerFlags & cHeaderFlagsCompressed) == 0)
   {
      const uint cUncompressedHeaderDWORDs = sizeof(BUncompressedHeader) / sizeof(DWORD);

      if (numSrcDWORDs < (numUncompressedDWORDs + cUncompressedHeaderDWORDs))
      {
         numDstDWORDs = 0;
         return false;
      }

      Utils::FastMemCpy(pDst, reinterpret_cast<const uint*>(pSrc) + cUncompressedHeaderDWORDs, numUncompressedDWORDs * sizeof(DWORD));
      numDstDWORDs = numUncompressedDWORDs;
      return true;
   }

   if (numSrcDWORDs < (sizeof(BCompressedHeader) / sizeof(DWORD)))
   {
      numDstDWORDs = 0;
      return false;
   }

   BCompressedHeader header(*reinterpret_cast<const BCompressedHeader*>(pSrc));
   header.endianSwitch();
   if (!header.mNumBitsQWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   const uint64* pSrcBits = reinterpret_cast<const uint64*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader));
   const uint* pUnmatched = reinterpret_cast<const uint*>(reinterpret_cast<const uchar*>(pSrc) + sizeof(BCompressedHeader) + header.mNumBitsQWORDs * sizeof(uint64)) + header.mNumUnmatchedDWORDs - 1;
   if (((reinterpret_cast<const uchar*>(pUnmatched) - reinterpret_cast<const uchar*>(pSrc)) / sizeof(DWORD)) >= numSrcDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   if (header.mNumUnmatchedDWORDs > numUncompressedDWORDs)
   {
      numDstDWORDs = 0;
      return false;
   }

   uint* pDstDWORD = reinterpret_cast<uint*>(pDst);
   uint numDWORDsLeft = numUncompressedDWORDs;

   uint64 curBits = 0;
   uint curBitsLeft = 0;

   if (flushDictionary)
   {
      Utils::ClearObj(mDictPartial);
      Utils::ClearObj(mDictFull);
   }

#define GET_BITS(result, mask, num) \
if (curBitsLeft >= num) \
   { \
   result = static_cast<uint>(curBits) & mask; \
   curBits >>= num; \
   curBitsLeft -= num; \
   } else { \
   result = static_cast<uint>(curBits); \
   uint numBitsRemaining = num - curBitsLeft; \
   curBits = *pSrcBits++; \
   EndianSwitchQWords(&curBits, 1); \
   result = (result | (static_cast<uint>(curBits) << curBitsLeft)) & mask; \
   curBitsLeft = 64U - numBitsRemaining; \
   curBits >>= numBitsRemaining; \
   }
   while (numDWORDsLeft)
   {
      BDEBUG_ASSERT((curBitsLeft & 1) == 0);

      if (!curBitsLeft)
      {
         curBits = *pSrcBits++;
         EndianSwitchQWords(&curBits, 1);
         curBitsLeft = 64U;   
      }

      const uint encodingType = static_cast<uint>(curBits) & cCodeMask;
      curBits >>= cCodeBits;
      curBitsLeft -= cCodeBits;

      switch (encodingType)
      {
         case cZeroValue:
         {
            // zero
            *pDstDWORD++ = 0;
            break;
         }
         case cFullMatch:
         {
            // full
            uint dictIndex;
            GET_BITS(dictIndex, cDictSizeMask, cDictBits);

            *pDstDWORD++ = mDictFull[dictIndex];
            break;
         }
         case cPartialMatch:
         {
            // partial
            uint dictIndex;
            GET_BITS(dictIndex, cDictSizeMask, cDictBits);

            uint lowValue;               
            GET_BITS(lowValue, cPartialMatchMaskLow, cPartialMatchBitsLow);

            uint highValue = mDictPartial[dictIndex];

            uint value = lowValue | highValue;
            uint fullHash = value;
            
            EndianSwitchDWords((DWORD*)&value, 1);
            *pDstDWORD++ = value;
            
            fullHash ^= (fullHash >> 16U);
            fullHash -= (fullHash >> 8U);
            fullHash ^= (fullHash >> 4U);
            fullHash &= cDictSizeMask;
                        
            mDictFull[fullHash] = value;

            break;
         }
         case cUnmatched:
         {
            // unmatched
            uint unmatchedValue = *pUnmatched--;
            *pDstDWORD++ = unmatchedValue;
            
            uint origUnmatchedValue = unmatchedValue;
            EndianSwitchDWords((DWORD*)&unmatchedValue, 1);

            uint partialHash = (unmatchedValue >> cPartialMatchBitsLow);
            uint fullHash = unmatchedValue;

            partialHash ^= (partialHash >> 16U);
            partialHash -= (partialHash >> 8U);
            partialHash ^= (partialHash >> 4U);
            partialHash &= cDictSizeMask;

            fullHash ^= (fullHash >> 16U);
            fullHash -= (fullHash >> 8U);
            fullHash ^= (fullHash >> 4U);
            fullHash &= cDictSizeMask;

            mDictPartial[partialHash] = unmatchedValue & cPartialMatchMaskHigh;
            
            mDictFull[fullHash] = origUnmatchedValue;

            break;
         }
      }

      numDWORDsLeft--;
   }

#undef GET_BITS      

   numDstDWORDs = numUncompressedDWORDs;
   return true;
}
