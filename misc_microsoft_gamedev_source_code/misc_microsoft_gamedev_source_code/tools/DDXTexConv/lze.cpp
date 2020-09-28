// File: lze.cpp
#include "xcore.h"
#include "huffman.h"
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"
#include "file\win32FileUtils.h"

// Compresses 1MB blocks at a time
// Control stream - Group of 8 literal/match flags, with RLE of 0 and 255
// Order 1-0 Huffman literal coding
//
// Match groups:
// 8 = Small match: Match len 2, decode small dist <= 255
// 29 = General match: Decode len, decode dist
// 16 = Repeat match reference index 0-15
//
//
// Huffman tables:
// Literal Order 1-0 tables
// Control stream table
// Match group table
// Small dist table
// General dist table

class BLZEBase
{
protected:
   enum 
   { 
      cMaxBufSize = 2*1024*1024,
      cMaxMatchLength = 258,
      cPrevMatchBufSize = 8,
      
      cNumMatchFlagsPerSymbol = 7U,
            
      cTotalSmallMatchSymbols = 4,
      cMaxSmallMatchDist = 32,
            
      cTotalGeneralMatchSymbols = 29,
      cTotalRepeatMatchRefSymbols = cPrevMatchBufSize,
      
      cTotalColorMatchSymbols = 4*4,
      cColorDeltaMin = -1,
      cColorDeltaMax = 1,
      cMaxColorMatchDist = 256,
                        
      cTotalMatchSymbols = cTotalSmallMatchSymbols + cTotalGeneralMatchSymbols + cTotalRepeatMatchRefSymbols + cTotalColorMatchSymbols,
      
      cFirstSmallMatchSymbol = 0,
      cFirstGeneralMatchSymbol = cTotalSmallMatchSymbols,
      cFirstRepeatMatchRefSymbol = cTotalSmallMatchSymbols + cTotalGeneralMatchSymbols,
      
      cFirstColorMatchSymbol = cTotalSmallMatchSymbols + cTotalGeneralMatchSymbols + cTotalRepeatMatchRefSymbols
   };
};

const uint cNumLenCodes = 29;

static const ushort gLenCode[] =
{
   257, 258, 259, 260, 261, 262, 263, 264,   265, 265, 266, 266, 267, 267, 268, 268,   269, 269, 269, 269, 270, 270, 270, 270,   271, 271, 271, 271, 272, 272, 272, 272,
   273, 273, 273, 273, 273, 273, 273, 273,   274, 274, 274, 274, 274, 274, 274, 274,   275, 275, 275, 275, 275, 275, 275, 275,   276, 276, 276, 276, 276, 276, 276, 276,
   277, 277, 277, 277, 277, 277, 277, 277,   277, 277, 277, 277, 277, 277, 277, 277,   278, 278, 278, 278, 278, 278, 278, 278,   278, 278, 278, 278, 278, 278, 278, 278,
   279, 279, 279, 279, 279, 279, 279, 279,   279, 279, 279, 279, 279, 279, 279, 279,   280, 280, 280, 280, 280, 280, 280, 280,   280, 280, 280, 280, 280, 280, 280, 280,
   281, 281, 281, 281, 281, 281, 281, 281,   281, 281, 281, 281, 281, 281, 281, 281,   281, 281, 281, 281, 281, 281, 281, 281,   281, 281, 281, 281, 281, 281, 281, 281,
   282, 282, 282, 282, 282, 282, 282, 282,   282, 282, 282, 282, 282, 282, 282, 282,   282, 282, 282, 282, 282, 282, 282, 282,   282, 282, 282, 282, 282, 282, 282, 282,
   283, 283, 283, 283, 283, 283, 283, 283,   283, 283, 283, 283, 283, 283, 283, 283,   283, 283, 283, 283, 283, 283, 283, 283,   283, 283, 283, 283, 283, 283, 283, 283,
   284, 284, 284, 284, 284, 284, 284, 284,   284, 284, 284, 284, 284, 284, 284, 284,   284, 284, 284, 284, 284, 284, 284, 284,   284, 284, 284, 284, 284, 284, 284, 285
};
static const uchar gLenExtra[] =
{
   0, 0, 0, 0, 0, 0, 0, 0,   1, 1, 1, 1, 1, 1, 1, 1,   2, 2, 2, 2, 2, 2, 2, 2,   2, 2, 2, 2, 2, 2, 2, 2,   3, 3, 3, 3, 3, 3, 3, 3,   3, 3, 3, 3, 3, 3, 3, 3,
   3, 3, 3, 3, 3, 3, 3, 3,   3, 3, 3, 3, 3, 3, 3, 3,   4, 4, 4, 4, 4, 4, 4, 4,   4, 4, 4, 4, 4, 4, 4, 4,   4, 4, 4, 4, 4, 4, 4, 4,   4, 4, 4, 4, 4, 4, 4, 4,
   4, 4, 4, 4, 4, 4, 4, 4,   4, 4, 4, 4, 4, 4, 4, 4,   4, 4, 4, 4, 4, 4, 4, 4,   4, 4, 4, 4, 4, 4, 4, 4,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5,   5, 5, 5, 5, 5, 5, 5, 0
};

const uint cNumDistCodes = 42;

static const uchar gDistExtra[] = 
{ 
   0, 0, 0, 0, 1, 1, 2, 2, 
   3, 3, 4, 4, 5, 5, 6, 6, 
   7, 7, 8, 8, 9, 9, 10, 10, 
   11, 11, 12, 12, 13, 13, 14, 14, 
   15, 15, 16, 16, 17, 17, 18, 18,
   19, 19
};

//static const uchar gSmallDistExtra[] =  { 2, 2, 3, 4, 5, 6,  6,  6 };
//static const uchar gSmallDistBase[] = { 0, 4, 8, 16, 32, 64, 128, 192 };
static const uchar gSmallDistExtra[] =  { 2, 2, 3, 4 };
static const uchar gSmallDistBase[] = { 0, 4, 8, 16 };
static const uint cNumSmallDistCodes = sizeof(gSmallDistBase)/sizeof(gSmallDistBase[0]);

#include "colorUtils.h"
#include "hash\hash.h"

class BLZEComp : public BLZEBase
{
   enum { cDistLoThreshBits = 10, cDistLoThresh = 1U<<cDistLoThreshBits };
   uchar mDistLoCode[cDistLoThresh];
   uchar mDistHiCode[cMaxBufSize / cDistLoThresh];
   uint mDistCodeBase[cNumDistCodes];
   
   static void unpackColor(const uchar* pSrc, BRGBAColor& a, BRGBAColor& b)
   {
      BColorUtils::unpackColor(pSrc[0] | (pSrc[1] << 8), a, false); a.a = 0;
      BColorUtils::unpackColor(pSrc[2] | (pSrc[3] << 8), b, false); b.a = 0;
   }
   
   static void packColor(uchar* pDst, BRGBAColor& a, BRGBAColor& b) 
   {
      uint q = BColorUtils::packColor(a, false);
      uint r = BColorUtils::packColor(b, false);
      pDst[0] = (uchar)(q & 0xFF);
      pDst[1] = (uchar)((q >> 8) & 0xFF);
      pDst[2] = (uchar)(r & 0xFF);
      pDst[3] = (uchar)((r >> 8) & 0xFF);
   }
   
   static uint hashColor(const uchar* p) 
   {
      BRGBAColor a, b;
      unpackColor(p, a, b);
      
      a.r >>= 3; a.g >>= 3; a.r >>= 3; 
      b.r >>= 3; b.g >>= 3; b.r >>= 3; 
      
      return hashFast(&b, sizeof(b), hashFast(&a, sizeof(a)));
   }
   
   static int compareColor(const uchar* p, const uchar* q) 
   {
      BRGBAColor a, b, c, d;
      unpackColor(p, a, b);
      unpackColor(q, c, d);
      
      int totalDelta = 0;
      for (uint i = 0; i < 3; i++)
      {
         int delta0 = (a[i] - c[i]);
         int delta1 = (b[i] - d[i]);
         
         if ((delta0 < cColorDeltaMin) || (delta0 > cColorDeltaMax)) 
            return -1;
            
         if ((delta1 < cColorDeltaMin) || (delta1 > cColorDeltaMax)) 
            return -1;
         
         totalDelta += delta0*delta0 + delta1*delta1;
      }
      
      return totalDelta;
   }

   void initLookupTables()
   {
      uint curDistBase = 0;
      
      for (uint i = 0; i < cNumDistCodes; i++)
      {
         mDistCodeBase[i] = curDistBase;
         
         uint numExtraBits = gDistExtra[i];
         uint curDistNum = 1 << numExtraBits;
                  
         if ((curDistBase + curDistNum) <= cDistLoThresh)
         {
            for (uint j = curDistBase; j < (curDistBase + curDistNum); j++)
               mDistLoCode[j] = (uchar)i;
         }
         else
         {
            for (uint j = curDistBase >> cDistLoThreshBits; j < ((curDistBase + curDistNum) >> cDistLoThreshBits); j++)
               mDistHiCode[j] = (uchar)i;
         }
         
         curDistBase += curDistNum;
      }
      
      BDEBUG_ASSERT(curDistBase == cMaxBufSize);
      BDEBUG_ASSERT(mDistHiCode[(cMaxBufSize / cDistLoThresh) - 1] == (cNumDistCodes - 1));
   }

public:
   BLZEComp()
   {
      initLookupTables();
   }

   struct BMatch
   {
      uint mLen;
      uint mDist;
      uint mOfs;
   };
   
   bool chooseMatch(BMatch& chosenMatch, const BMatch& longestMatch, const BMatch& closestMatch, int closestMatchIndex, BMatch* pPrevMatches, int* pLenDelta = NULL, int* pDistDelta = NULL)
   {
      if ( (closestMatchIndex >= 0) &&
           (closestMatch.mLen == longestMatch.mLen) && 
           (closestMatch.mLen == pPrevMatches[closestMatchIndex].mLen) &&
           (closestMatch.mOfs == pPrevMatches[closestMatchIndex].mOfs)
         )
      {
         chosenMatch = closestMatch;
         if (pLenDelta) *pLenDelta = 0;
         if (pDistDelta) *pDistDelta = 0;
         return true;
      }
      
      chosenMatch = longestMatch;
      return false;
   }         

   bool pack(const void* pData, uint dataLen, BByteArray& compData)
   {
      mpData = static_cast<const BYTE*>(pData);
      mDataLen = dataLen;
      
      initHash();

      mCodes.clear();

      mOutputByteStream.resize(0);
      mOutputBitStream.setStream(&mOutputByteStream);
      mOutputBitStream.begin();

      mOutputBitStream.putBits(dataLen, 32);

      uint bytesLeft = dataLen;
      uint curOfs = 0;

      uint numPrevMatches = 0;
      BMatch prevMatches[cPrevMatchBufSize];
            
      uint totalMatchBytes = 0;
      
      uint codeBufStartOfs = 0;
      
      while (bytesLeft)
      {
         int colorMatchOfs = getColorMatch(curOfs);
         
         BMatch chosenMatch;
         int prevMatchRef;
         getLZ77Match(curOfs, chosenMatch, prevMatchRef, numPrevMatches, prevMatches);

         if (chosenMatch.mLen >= 2)
         {
            BMatch chosenMatch1;
            int prevMatchRef1;
            getLZ77Match(curOfs + 1, chosenMatch1, prevMatchRef1, numPrevMatches, prevMatches);
            
            if (chosenMatch1.mLen > chosenMatch.mLen)
               chosenMatch.mLen = 0;
         }

         BCode& nextCode = *mCodes.enlarge(1);
         nextCode.mOfs = curOfs;
                           
         uint numBytesEmitted;
                  
         if ( (colorMatchOfs >= 0) && (chosenMatch.mLen < 12))
         //if (colorMatchOfs >= 0)
         {
            nextCode.mMatchDist = curOfs - colorMatchOfs;
            nextCode.mMatchLen = 4;
            nextCode.mPrevMatchIndex = -1;
            nextCode.mType = BCode::cColor;

            numBytesEmitted = 4;
         }             
         else if (chosenMatch.mLen < 2)
         {
            nextCode.mMatchDist = -mpData[curOfs] - 1;
            nextCode.mMatchLen = 0;
            nextCode.mPrevMatchIndex = -1;
            nextCode.mType = BCode::cLiteral;
            
            numBytesEmitted = 1;
         }
         else
         {
            nextCode.mType = BCode::cMatch;
            nextCode.mMatchDist = chosenMatch.mDist;
            nextCode.mMatchLen = (int16)chosenMatch.mLen;
            nextCode.mPrevMatchIndex = (int8)prevMatchRef;

#ifdef BUILD_DEBUG            
            {
               uint matchOfs = curOfs - chosenMatch.mDist;
               uint i;
               for (i = 0; i < chosenMatch.mLen; i++)
               {
                  if (getDict(curOfs + i) != getDict(matchOfs + i))
                     DebugBreak();
               }
            }
#endif
                                                            
            for (uint j = cPrevMatchBufSize - 1; j >= 1; j--)
               prevMatches[j] = prevMatches[j - 1];
            prevMatches[0] = chosenMatch;
            if (numPrevMatches < cPrevMatchBufSize)
               numPrevMatches++;
            
            numBytesEmitted = chosenMatch.mLen;
            
            totalMatchBytes += chosenMatch.mLen;
         }
         
         BDEBUG_ASSERT(numBytesEmitted <= bytesLeft);

         bytesLeft -= numBytesEmitted;
         curOfs += numBytesEmitted;

         if (mCodes.getSize() > 16384)
         {
            if (!flushCodeBuf(&mOutputBitStream, codeBufStartOfs, curOfs - codeBufStartOfs))
               return false;
            
            numPrevMatches = 0;
            codeBufStartOfs = curOfs;
            
            printf("%u of %u\n", curOfs, dataLen);
         }               
      }     

      printf("Total match bytes: %u\n", totalMatchBytes);
      
      if (!flushCodeBuf(&mOutputBitStream, codeBufStartOfs, curOfs - codeBufStartOfs))
         return false;

      mOutputBitStream.end();
      compData.swap(mOutputByteStream.getBuf());

      return true;
   }

private:
   const BYTE*             mpData;
   uint                    mDataLen;
   
   BDynamicArray<int>      mNext;
   
   BDynamicArray<int>      mColorNext;
            
   struct BCode
   {
      uint  mOfs;
      int   mMatchDist;
            
      int16 mMatchLen;
                  
      int8  mPrevMatchIndex;
      
      enum { cMatch = 0, cLiteral = 1, cColor = 2 };
      uint8 mType;
   };
   
   BDynamicArray<BCode>    mCodes;
      
   BDynamicStream          mOutputByteStream;
   BOutputBitStreamAdapter mOutputBitStream;

   inline uint getDict(uint ofs) const { BDEBUG_ASSERT(ofs < mDataLen); return mpData[ofs]; }
   
   void initHash(void)
   {
      enum { cNumChains = 65536 };
      BDynamicArray<int> chains;
      
      chains.resize(cNumChains);
      chains.setAll(-1);

      mNext.resize(mDataLen);
      mNext.setAll(-1);

      for (int i = 0; i < ((int)mDataLen - 1); i++)
      {
         uint a = mpData[i];
         uint b = mpData[i + 1];
         
         uint hash = a | (b << 8);

         mNext[i] = chains[hash];
         chains[hash] = i;
      }
      
      chains.setAll(-1);
      
      mColorNext.resize(mDataLen);
      mColorNext.setAll(-1);
      
      for (int i = 0; i < ((int)mDataLen - 3); i++)
      {
         uint hash = hashColor(mpData + i) & (cNumChains - 1);

         mColorNext[i] = chains[hash];
         chains[hash] = i;
      }
   }
   
   static bool isValidMatch(uint len, uint dist) 
   {
      if (len == 2)
      {
         if (dist > cMaxSmallMatchDist)
            return false;
      }
      else if (len == 3)
      {
         if (dist > 8192)
            return false;
      }
      else if (len == 4)
      {
         if (dist > 262144)
            return false;
      }
      
      return true;
   }      
      
   int getColorMatch(uint ofs) const
   {
      int matchOfs = mColorNext[ofs];

      uint count = 0;

      int bestColorDelta = INT_MAX;
      int bestMatchOfs = -1;
      while (matchOfs != -1)
      {
         if ((ofs - matchOfs) > cMaxColorMatchDist)
            break;
            
         if ( ((ofs - matchOfs) & 7) == 0 )
         {
            int colorDelta = compareColor(mpData + ofs, mpData + matchOfs);
            if ((colorDelta >= 0) && (colorDelta < bestColorDelta))
            {
               bestColorDelta = colorDelta;
               bestMatchOfs = matchOfs;
            }
         }            

         matchOfs = mColorNext[matchOfs];
         
         count++;
         if (count > 1024)
            break;
      }

      return bestMatchOfs;      
   }
   
   void getLZ77Match(uint ofs, BMatch& bestMatch, int& prevMatchRef, uint numPrevMatches, const BMatch* pPrevMatches) const
   {
      bestMatch.mLen = 1;
      prevMatchRef = -1;
           
      const uint maxMatchLen = Math::Min<uint>(cMaxMatchLength, mDataLen - ofs);

      int matchOfs;      
      for (uint i = 0; i < numPrevMatches; i++)
      {
         matchOfs = pPrevMatches[i].mOfs;
         uint prevMatchLen = pPrevMatches[i].mLen;
         
         if ((prevMatchLen > maxMatchLen) || (prevMatchLen < bestMatch.mLen))
            continue;
                  
         if (getDict(matchOfs + prevMatchLen - 1) != getDict(ofs + prevMatchLen - 1))
            continue;
         
         uint matchLen;
         for (matchLen = 0; matchLen < prevMatchLen; matchLen++)
            if (getDict(matchOfs + matchLen) != getDict(ofs + matchLen))
               break;
            
         if (matchLen == prevMatchLen)
         {
            uint matchDist = ofs - matchOfs;
            
            bestMatch.mLen = matchLen;
            bestMatch.mDist = matchDist;
            bestMatch.mOfs = matchOfs;
            prevMatchRef = i;
            
            if (matchLen == maxMatchLen)
               return;
         }            
      }

      matchOfs = mNext[ofs];

      uint count = 0;

      while (matchOfs != -1)
      {
         if (getDict(matchOfs + bestMatch.mLen) == getDict(ofs + bestMatch.mLen))
         {
            uint matchLen;
            for (matchLen = 0; matchLen < maxMatchLen; matchLen++)
               if (getDict(matchOfs + matchLen) != getDict(ofs + matchLen))
                  break;
         
            //if ((matchLen >= 3) && (matchLen > bestMatch.mLen))
            if (matchLen > bestMatch.mLen)
            {
               uint matchDist = ofs - matchOfs;

               if (isValidMatch(matchLen, matchDist))
               {         
                  bestMatch.mLen = matchLen;
                  bestMatch.mOfs = matchOfs;
                  bestMatch.mDist = ofs - matchOfs;
                  prevMatchRef = -1;
                  
                  if (matchLen == maxMatchLen)
                     break;
               }               
            }            
         }            

         matchOfs = mNext[matchOfs];

         count++;
         if (count > 16384)
            break;
      }
   }
   
   void getSmallDistCodes(uint dist, uint& sym, uint& extra, uint& numExtra) const
   {
      for (sym = cNumSmallDistCodes - 1; sym > 0; sym--)
         if ((dist - 1) >= gSmallDistBase[sym])
            break;
      
      numExtra = gSmallDistExtra[sym];
      if (!numExtra)
         extra = 0;
      else
         extra = dist - 1U - gSmallDistBase[sym];
      
      BDEBUG_ASSERT(extra < (1 << numExtra));
   }         

   bool flushCodeBuf(BOutputBitStream* pOutputBitStream, uint startOfs, uint totalBytes) 
   {
      BDynamicArray<uint16> litHist(256*256);
      BDynamicArray<uint> matchFlags;
                  
      BHuffmanCodes matchFlagsCodes(1U << cNumMatchFlagsPerSymbol);
            
      uint curMatchFlags = 0, numMatchFlags = 0;
      
      uint maxLitSymIndex = 0;
      
      uint totalCodedBytes = 0;
      for (uint i = 0; i < mCodes.getSize(); i++)
      {
         const BCode& code = mCodes[i];
         if (code.mType == BCode::cLiteral)
         {
            totalCodedBytes++;
            
            uint curC = -code.mMatchDist - 1;
            maxLitSymIndex = Math::Max(maxLitSymIndex, curC);
            
            BDEBUG_ASSERT(curC == mpData[code.mOfs]);
            
            uint prevC = mpData[code.mOfs - 1];
            
            litHist[prevC * 256 + curC]++;
            
            prevC = curC;
         }
         else
         {
            totalCodedBytes += code.mMatchLen;
            
            curMatchFlags |= (1 << numMatchFlags);
         }
         
         numMatchFlags++;
         if (numMatchFlags == cNumMatchFlagsPerSymbol)
         {
            numMatchFlags = 0;
            
            matchFlags.pushBack(curMatchFlags);
            matchFlagsCodes.incFreq(curMatchFlags);
            curMatchFlags = 0;
         }
      }
      
      if (totalBytes != totalCodedBytes)
         return false;
      
      if (numMatchFlags)
      {
         matchFlags.pushBack(curMatchFlags);
         matchFlagsCodes.incFreq(curMatchFlags);
      }
      
      if (!matchFlagsCodes.create())
         return false;
      
      BHuffmanEnc matchFlagEnc;
      if (!matchFlagEnc.init(pOutputBitStream, &matchFlagsCodes))
         return false;
                        
      BDynamicArray<uint> prevCharToLitTable(256);
      
      BDynamicArray<int> litTableToPrevChar;
      litTableToPrevChar.pushBack(-1);
      
      BDynamicArray<BHuffmanCodes> litCodes;
      litCodes.reserve(257);
      
      litCodes.enlarge(1);
                                    
      for (uint i = 0; i < 256; i++)
      {
         uint numUniqueSymbols = 0;
         uint totalHist = 0;
         for (uint j = 0; j < 256; j++)
            if (litHist[i*256 + j])
            {
               totalHist += litHist[i*256 + j];
               numUniqueSymbols++;
            }
         
         if (totalHist < 512)
         //if (totalHist < 128)
            prevCharToLitTable[i] = 0;
         else
         {
            prevCharToLitTable[i] = litTableToPrevChar.getSize();
            litTableToPrevChar.pushBack(i);
            
            litCodes.enlarge(1);
         }
         
         for (uint j = 0; j < 256; j++)
         {
            if (litHist[i*256 + j])
               litCodes[prevCharToLitTable[i]].incFreq(j, litHist[i*256 + j]);
         }
      }
      
      printf("Lit coders: %u\n", litCodes.getSize());
      
      BUniversalCodec universalCodec;

      uint curRunLen = 0;
      uint prevBit = 0;
      for (uint i = 0; i < 256; i++)
      {
         uint bit = (prevCharToLitTable[i] > 0);

         if (bit == prevBit)
            curRunLen++;
         else
         {
            if (!universalCodec.encodeOmega(pOutputBitStream, curRunLen))
               return false;

            curRunLen = 1;
            prevBit = !prevBit;
         }
      }      

      if (!universalCodec.encodeOmega(pOutputBitStream, curRunLen))
         return false;
      
      BDynamicArray<BHuffmanEnc> litEncoders(litCodes.getSize());
      for (uint i = 0; i < litCodes.getSize(); i++)
      {
         if (!litCodes[i].create(15))
            return false;
         
         if (!litEncoders[i].init(pOutputBitStream, &litCodes[i]))
            return false;
      }
            
      BHuffmanCodes matchCodes(cTotalMatchSymbols);
      BHuffmanCodes generalDistCodes(cNumDistCodes);
      const uint cColorDeltaRange = cColorDeltaMax - cColorDeltaMin + 1;
      BHuffmanCodes colorDeltaCodes(cColorDeltaRange * cColorDeltaRange * cColorDeltaRange);
                                    
      for (uint i = 0; i < mCodes.getSize(); i++)
      {
         const BCode& code = mCodes[i];
                           
         if (code.mType != BCode::cLiteral)
         {
            if (code.mType == BCode::cColor)
            {
               BRGBAColor a, b, c, d;
               unpackColor(mpData + code.mOfs, a, b);
               unpackColor(mpData + code.mOfs - code.mMatchDist, c, d);
                                 
               uint q = 0, r = 0;
               for (uint i = 0; i < 3; i++)
               {
                  q = q * cColorDeltaRange + (-cColorDeltaMin + a[i] - c[i]);
                  r = r * cColorDeltaRange + (-cColorDeltaMin + b[i] - d[i]);
               }
               
               uint symIndex = 0;
               if (a == c)
                  symIndex += cTotalColorMatchSymbols / 2;
               else
                  colorDeltaCodes.incFreq(q);
                  
               if (b == d)
                  symIndex += cTotalColorMatchSymbols / 4;
               else
                  colorDeltaCodes.incFreq(r);
                                                                           
               BDEBUG_ASSERT((code.mMatchDist & 7) == 0);
               uint dist = code.mMatchDist/8;
               
               uint matchDistCodeIndex;
               uint extra, numExtra;
               getSmallDistCodes(dist, matchDistCodeIndex, extra, numExtra);
               
               BDEBUG_ASSERT(matchDistCodeIndex < cTotalColorMatchSymbols);
               matchCodes.incFreq(cFirstColorMatchSymbol + matchDistCodeIndex + symIndex);
            }
            else if (code.mPrevMatchIndex >= 0)
            {
               BDEBUG_ASSERT(code.mPrevMatchIndex < cPrevMatchBufSize);
               matchCodes.incFreq(cFirstRepeatMatchRefSymbol + code.mPrevMatchIndex);
            }
            else if (code.mMatchLen == 2)
            {
               BDEBUG_ASSERT((code.mMatchDist >= 1) && (code.mMatchDist <= cMaxSmallMatchDist));
               
               uint matchDistCodeIndex;
               uint extra, numExtra;
               getSmallDistCodes(code.mMatchDist, matchDistCodeIndex, extra, numExtra);
                                                      
               matchCodes.incFreq(cFirstSmallMatchSymbol + matchDistCodeIndex);
            }
            else
            {
               uint minMatch = 3;
               if (code.mMatchDist > 262144)
                  minMatch = 5;
               else if (code.mMatchDist > 8192)
                  minMatch = 4;
               BDEBUG_ASSERT(code.mMatchLen >= minMatch);
               uint matchLenIndex = gLenCode[code.mMatchLen - minMatch] - 257;
               matchCodes.incFreq(cFirstGeneralMatchSymbol + matchLenIndex);
               
               uint dist = code.mMatchDist - 1;
               BDEBUG_ASSERT((dist >= 0) && (dist < cMaxBufSize));
               
               uint matchDistIndex;
               if (dist < cDistLoThresh)
                  matchDistIndex = mDistLoCode[dist];
               else
                  matchDistIndex = mDistHiCode[dist >> cDistLoThreshBits];
               
               generalDistCodes.incFreq(matchDistIndex);
            }
         }
      }
      
      if (!matchCodes.create()) return false;
      if (!generalDistCodes.create()) return false;
      if (!colorDeltaCodes.create()) return false;
            
      BHuffmanEnc matchCodesEnc;
      BHuffmanEnc generalDistCodesEnc;
      BHuffmanEnc colorDeltaCodesEnc;
            
      if (!matchCodesEnc.init(pOutputBitStream, &matchCodes)) return false;
      if (!generalDistCodesEnc.init(pOutputBitStream, &generalDistCodes)) return false;
      if (!colorDeltaCodesEnc.init(pOutputBitStream, &colorDeltaCodes)) return false;
            
      uint numMatchFlagsLeft = 0;
      uint curMatchFlagOfs = 0;
                  
      uint prevMatchFlagCodeSize = 0;
      
      uint totalLiterals = 0, totalSmallMatches = 0, totalGenMatches = 0, totalPMRs = 0, totalColor = 0;
      uint numLiteralsBreakeven = 0, numLiteralsExpanded = 0;
      uint numSmallBreakeven = 0, numSmallExpanded = 0;
      uint numGenBreakeven = 0, numGenExpanded = 0;
      uint numPMRBreakeven = 0, numPMRExpanded = 0;
      uint numColorBreakeven = 0, numColorExpanded = 0;
      
      uint totalLitBytes = 0, totalSmallBytes = 0, totalGenBytes = 0, totalPMRBytes = 0, totalColorBytes = 0;
      
      double totalLitBits = 0, totalSmallBits = 0, totalGenBits = 0, totalPMRBits = 0, totalColorBits = 0;
      uint totalGenLenBits = 0, totalGenDistBits = 0;
      
      for (uint i = 0; i < mCodes.getSize(); i++)
      {
         if (!numMatchFlagsLeft)
         {
            if (!matchFlagEnc.encode(matchFlags[curMatchFlagOfs])) return false;
            
            prevMatchFlagCodeSize = matchFlagsCodes.getCodeSize(matchFlags[curMatchFlagOfs]);
                        
            curMatchFlagOfs++;
            numMatchFlagsLeft = cNumMatchFlagsPerSymbol;
         }
         
         BCode code(mCodes[i]);
         
         uint srcBits;
         float dstBits = (float)prevMatchFlagCodeSize / cNumMatchFlagsPerSymbol;
         if (code.mType == BCode::cLiteral)
         {  
            totalLiterals++;
            
            srcBits = 8;
            
            uint prevC = mpData[code.mOfs - 1];
            uint curC = -code.mMatchDist - 1;
            
            uint litTableIndex = prevCharToLitTable[prevC];
            
            if (!litEncoders[litTableIndex].encode(curC)) return false;
            
            dstBits += litCodes[litTableIndex].getCodeSize(curC);
            
            if (srcBits == dstBits)
               numLiteralsBreakeven++;
            else if (srcBits < dstBits)
               numLiteralsExpanded++;
            
            totalLitBits += dstBits;
            totalLitBytes++;
         }
         else
         {
            srcBits = code.mMatchLen * 8;
                                    
            if (code.mType == BCode::cColor)
            {
               totalColor++;
               totalColorBytes += 4;
               
               BRGBAColor a, b, c, d;
               unpackColor(mpData + code.mOfs, a, b);
               unpackColor(mpData + code.mOfs - code.mMatchDist, c, d);

               uint symIndex = 0;
               if (a == c)
                  symIndex += cTotalColorMatchSymbols / 2;

               if (b == d)
                  symIndex += cTotalColorMatchSymbols / 4;
               
               uint dist = code.mMatchDist/8;
               
               uint matchDistCodeIndex;
               uint extra, numExtra;
               getSmallDistCodes(dist, matchDistCodeIndex, extra, numExtra);

               symIndex += cFirstColorMatchSymbol + matchDistCodeIndex;
               if (!matchCodesEnc.encode(symIndex)) return false;
               dstBits += matchCodes.getCodeSize(symIndex);

               if (numExtra)
               {
                  if (!pOutputBitStream->putBits(extra, numExtra)) return false;
                  dstBits += numExtra;
               }
               
               uint q = 0, r = 0;
               for (uint i = 0; i < 3; i++)
               {
                  q = q * cColorDeltaRange + (-cColorDeltaMin + a[i] - c[i]);
                  r = r * cColorDeltaRange + (-cColorDeltaMin + b[i] - d[i]);
               }
               
               if (a != c)
               {  
                  if (!colorDeltaCodesEnc.encode(q)) return false;
                  dstBits += colorDeltaCodes.getCodeSize(q);
               }
               
               if (b != d)
               {  
                  if (!colorDeltaCodesEnc.encode(r)) return false;
                  dstBits += colorDeltaCodes.getCodeSize(r);
               }                  
                                             
               totalColorBits += dstBits;
            }
            else if (code.mPrevMatchIndex >= 0)
            {
               totalPMRs++;
               
               BDEBUG_ASSERT(code.mPrevMatchIndex < cPrevMatchBufSize);
               if (!matchCodesEnc.encode(cFirstRepeatMatchRefSymbol + code.mPrevMatchIndex)) return false;
               
               dstBits += matchCodes.getCodeSize(cFirstRepeatMatchRefSymbol + code.mPrevMatchIndex);
               
               if (srcBits == dstBits)
                  numPMRBreakeven++;
               else if (srcBits < dstBits)
                  numPMRExpanded++;
                  
               totalPMRBits += dstBits;
               totalPMRBytes += code.mMatchLen;
            }
            else if (code.mMatchLen == 2)
            {
               totalSmallMatches++;
               
               BDEBUG_ASSERT((code.mMatchDist >= 1) && (code.mMatchDist <= cMaxSmallMatchDist));

               uint matchDistCodeIndex;
               uint extra, numExtra;
               getSmallDistCodes(code.mMatchDist, matchDistCodeIndex, extra, numExtra);

               if (!matchCodesEnc.encode(cFirstSmallMatchSymbol + matchDistCodeIndex)) return false;
               dstBits += matchCodes.getCodeSize(cFirstSmallMatchSymbol + matchDistCodeIndex);
                              
               if (numExtra)
               {
                  if (!pOutputBitStream->putBits(extra, numExtra)) return false;
                  dstBits += numExtra;
               }
               
               if (srcBits == dstBits)
                  numSmallBreakeven++;
               else if (srcBits < dstBits)
                  numSmallExpanded++;
               
               totalSmallBytes += 2;
               totalSmallBits += dstBits;
            }
            else
            {
               totalGenMatches++;
               
               BDEBUG_ASSERT(code.mMatchLen >= 3 && code.mMatchLen <= cMaxMatchLength);
               
               uint minMatch = 3;
               
               if (code.mMatchDist > 262144)
                  minMatch = 5;
               else if (code.mMatchDist > 8192)
                  minMatch = 4;
               BDEBUG_ASSERT(code.mMatchLen >= minMatch);
               uint matchLenIndex = gLenCode[code.mMatchLen - minMatch] - 257;
               if (!matchCodesEnc.encode(cFirstGeneralMatchSymbol + matchLenIndex)) return false;
               dstBits += matchCodes.getCodeSize(cFirstGeneralMatchSymbol + matchLenIndex);
               totalGenLenBits += matchCodes.getCodeSize(cFirstGeneralMatchSymbol + matchLenIndex);
               
               uint lenExtra = gLenExtra[code.mMatchLen - minMatch];
               if (lenExtra)
               {
                  uint bitMask = (1U << lenExtra) - 1U;
                  if (!pOutputBitStream->putBits((code.mMatchLen - minMatch) & bitMask, lenExtra)) return false;
                  dstBits += lenExtra;
                  totalGenLenBits += lenExtra;
               }

               uint dist = code.mMatchDist - 1;
               BDEBUG_ASSERT((dist >= 0) && (dist < cMaxBufSize));

               uint matchDistIndex;
               if (dist < cDistLoThresh)
                  matchDistIndex = mDistLoCode[dist];
               else
                  matchDistIndex = mDistHiCode[dist >> cDistLoThreshBits];

               if (!generalDistCodesEnc.encode(matchDistIndex)) return false;
               dstBits += generalDistCodes.getCodeSize(matchDistIndex);
               totalGenDistBits += generalDistCodes.getCodeSize(matchDistIndex);
               
               uint distExtra = gDistExtra[matchDistIndex];
               if (distExtra)
               {
                  uint bitMask = (1U << distExtra) - 1U;
                  if (!pOutputBitStream->putBits(dist & bitMask, distExtra)) return false;
                  
                  dstBits += distExtra;
                  totalGenDistBits += distExtra;
               }
               
               if (srcBits == dstBits)
                  numGenBreakeven++;
               else if (srcBits < dstBits)
                  numGenExpanded++;
               
               totalGenBits += dstBits;
               totalGenBytes += code.mMatchLen;
            }
         }
                           
         numMatchFlagsLeft--;
      }
            
      printf("         Total Codes: %u, SrcBytes: %u\n", mCodes.getSize(), totalCodedBytes);
      printf("      Total Literals: %u, Breakeven: %u, Expanded: %u, SrcBytes: %u, CompBytes: %6.1f\n", totalLiterals, numLiteralsBreakeven, numLiteralsExpanded, totalLitBytes, totalLitBits / 8.0f);
      printf(" Total Small Matches: %u, Breakeven: %u, Expanded: %u, SrcBytes: %u, CompBytes: %6.1f\n", totalSmallMatches, numSmallBreakeven, numSmallExpanded, totalSmallBytes, totalSmallBits / 8.0f);
      printf("Total Normal Matches: %u, Breakeven: %u, Expanded: %u, SrcBytes: %u, CompBytes: %6.1f, LenBits: %u, DistBits: %u\n", totalGenMatches, numGenBreakeven, numGenExpanded, totalGenBytes, totalGenBits / 8.0f, totalGenLenBits, totalGenDistBits);
      printf("          Total PMRs: %u, Breakeven: %u, Expanded: %u, SrcBytes: %u, CompBytes: %6.1f\n", totalPMRs, numPMRBreakeven, numPMRExpanded, totalPMRBytes, totalPMRBits / 8.0f);
      printf("         Total Color: %u, Breakeven: %u, Expanded: %u, SrcBytes: %u, CompBytes: %6.1f\n", totalColor, numColorBreakeven, numColorExpanded, totalColorBytes, totalColorBits / 8.0f);
      
      mCodes.clear();
      
      return true;
   }
};

class BLZEDecomp : public BLZEBase
{
public:
   BLZEDecomp()
   {
   }

   bool unpack(const void* pData, uint dataLen, BByteArray& decompData)
   {
      if (!dataLen)
         return false;

      mInputByteStream.set(pData, dataLen, cSFReadable | cSFSeekable);
      mInputBitStream.setStream(&mInputByteStream);

      if (!mInputBitStream.getBits(mUnpackedSize, 32))
         return false;

      BHuffmanDec huffDec;
      if (!huffDec.init(&mInputBitStream))
         return false;

      mUnpackedData.resize(mUnpackedSize);

      return true;
   }

private:
   uint                    mUnpackedSize;
   BByteArray              mUnpackedData;

   BByteStream             mInputByteStream;
   BInputBitStreamAdapter  mInputBitStream;
  

};

int lzeTest(int argC, const char* argV[])
{
   if (argC != 4)
   {
      printf("Usage [e/c] infilename outfilename\n");
      return EXIT_FAILURE;
   }

   if (tolower(argV[1][0]) == 'e')
   {
      BByteArray rawData;
      if (!BWin32FileUtils::readFileData(argV[2], rawData))
      {
         printf("Unable to read file: %s\n", argV[2]);
         return EXIT_FAILURE;
      }
      if (rawData.getSize() > 2*1024*1024)
         rawData.resize(2*1024*1024);

      BByteArray compData;
      BLZEComp comp;
      if (!comp.pack(rawData.getPtr(), rawData.getSize(), compData))
      {
         printf("Pack failed\n");
         return EXIT_FAILURE;
      }

      if (!BWin32FileUtils::writeFileData(argV[3], compData))
      {
         printf("Write to file failed: %s\n", argV[3]);
         return EXIT_FAILURE;
      }

      printf("Packed %u to %u bytes, %1.3fbpb\n", rawData.getSize(), compData.getSize(), (compData.getSize() * 8.0f) / rawData.getSize());
   }
   else if (tolower(argV[1][0]) == 'd')
   {
      BByteArray compData;
      if (!BWin32FileUtils::readFileData(argV[2], compData))
      {
         printf("Unable to read file: %s\n", argV[2]);
         return EXIT_FAILURE;
      }

      BByteArray rawData;
      BLZEDecomp decomp;
      if (!decomp.unpack(compData.getPtr(), compData.getSize(), rawData))
      {
         printf("Unpack failed\n");
         return EXIT_FAILURE;
      }

      if (!BWin32FileUtils::writeFileData(argV[3], rawData))
      {
         printf("Write to file failed: %s\n", argV[3]);
         return EXIT_FAILURE;
      }

      printf("Unpacked %u to %u bytes\n", compData.getSize(), rawData.getSize());
   }
   else
   {
      printf("Invalid mode\n");
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

