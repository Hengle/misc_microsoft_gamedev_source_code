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
// 16 = Previous match reference index 0-15, decode len delta -8 to 7, dist deltas -128 to 127
//
//
// Huffman tables:
// Literal Order 1-0 tables
// Control stream table
// Match group table
// Small dist table
// General dist table
// LenDelta table
// DistDelta table

class BLZEBase
{
protected:
   enum 
   { 
      cMaxBufSize = 2*1024*1024,
      cMaxMatchLength = 258,
      cPrevMatchBufSize = 8,
      
      cNumMatchFlagsPerSymbol = 7U,
            
      cTotalSmallMatchSymbols = 6,
      cMaxSmallMatchDist = 128,
      
      cTotalGeneralMatchSymbols = 29,
      cTotalRepeatMatchRefSymbols = cPrevMatchBufSize,
      cTotalPrevMatchRefSymbols = cPrevMatchBufSize,
      
      cTotalMatchSymbols = cTotalSmallMatchSymbols + cTotalGeneralMatchSymbols + cTotalRepeatMatchRefSymbols + cTotalPrevMatchRefSymbols,
      
      cFirstSmallMatchSymbol = 0,
      cFirstGeneralMatchSymbol = cTotalSmallMatchSymbols,
      cFirstRepeatMatchRefSymbol = cTotalSmallMatchSymbols + cTotalGeneralMatchSymbols,
      cFirstPrevMatchRefSymbol = cTotalSmallMatchSymbols + cTotalGeneralMatchSymbols + cTotalRepeatMatchRefSymbols,
      
      cTotalLenDeltaCodes = 32,
      cTotalDistDeltaCodes = 512,
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
static const uchar gSmallDistExtra[] =  { 2, 2, 3, 4, 5, 6 };
static const uchar gSmallDistBase[] = { 0, 4, 8, 16, 32, 64 };
static const uint cNumSmallDistCodes = sizeof(gSmallDistBase)/sizeof(gSmallDistBase[0]);

class BLZEComp : public BLZEBase
{
   enum { cDistLoThreshBits = 10, cDistLoThresh = 1U<<cDistLoThreshBits };
   uchar mDistLoCode[cDistLoThresh];
   uchar mDistHiCode[cMaxBufSize / cDistLoThresh];
   uint mDistCodeBase[cNumDistCodes];

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
      uint mCodeIndex;
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
                  
      uint numLit = 0, numPMR = 0, numGen = 0;
      
      while (bytesLeft)
      {
         BMatch longestMatch, closestMatch;
         int closestMatchIndex = -1;
         getLZ77Match(curOfs, longestMatch, closestMatch, closestMatchIndex, numPrevMatches, prevMatches);
         
         if (longestMatch.mLen)
         {
            BMatch longestMatch1, closestMatch1;
            int closestMatchIndex1 = -1;
            getLZ77Match(curOfs + 1, longestMatch1, closestMatch1, closestMatchIndex1, 0, NULL);
            if (longestMatch1.mLen > longestMatch.mLen)
               longestMatch.mLen = 0;
         }

         BCode& nextCode = *mCodes.enlarge(1);
         nextCode.mOfs = curOfs;
         nextCode.mNumReferers = 0;
         
         uint numBytesEmitted;
         if (longestMatch.mLen < 2)
         {
            nextCode.mMatchDist = -mpData[curOfs] - 1;
            nextCode.mMatchLen = 0;
            nextCode.mPrevMatchIndex = -1;
            nextCode.mLiteral = 1;
            
            numBytesEmitted = 1;
            
            numLit++;
         }
         else
         {
            nextCode.mLiteral = 0;
            
            BMatch chosenMatch;
            int lenDelta = 0, distDelta = 0;
            bool choosePMR = chooseMatch(chosenMatch, longestMatch, closestMatch, closestMatchIndex, prevMatches, &lenDelta, &distDelta);
            
            chosenMatch.mCodeIndex = mCodes.getSize() - 1;
            
            if (choosePMR)
            {
               nextCode.mMatchDist = distDelta;
               nextCode.mMatchLen = (int16)lenDelta;
               nextCode.mPrevMatchIndex = (int8)closestMatchIndex;
               
               mCodes[prevMatches[closestMatchIndex].mCodeIndex].mNumReferers++;
               
               numPMR++;
            }
            else
            {
               nextCode.mMatchDist = chosenMatch.mDist;
               nextCode.mMatchLen = (int16)chosenMatch.mLen;
               nextCode.mPrevMatchIndex = -1;
               
               numGen++;
            }
                        
            for (uint j = cPrevMatchBufSize - 1; j >= 1; j--)
               prevMatches[j] = prevMatches[j - 1];
            prevMatches[0] = chosenMatch;
            if (numPrevMatches < cPrevMatchBufSize)
               numPrevMatches++;
            
            numBytesEmitted = chosenMatch.mLen;
            
            nextCode.mActualMatchDist = chosenMatch.mDist;
            nextCode.mActualMatchLen = chosenMatch.mLen;
         }
         
         BDEBUG_ASSERT(numBytesEmitted <= bytesLeft);

         bytesLeft -= numBytesEmitted;
         curOfs += numBytesEmitted;

         if (mCodes.getSize() > 16384)
         {
            if (!flushCodeBuf(&mOutputBitStream))
               return false;
            
            numPrevMatches = 0;
            
            printf("%u of %u\n", curOfs, dataLen);
         }               
      }     

      if (!flushCodeBuf(&mOutputBitStream))
         return false;

      mOutputBitStream.end();
      compData.swap(mOutputByteStream.getBuf());

      return true;
   }

private:
   const BYTE*             mpData;
   uint                    mDataLen;

   enum { cNumChains = 65536 };
   BDynamicArray<int>      mChains;
   BDynamicArray<int>      mNext;
         
   struct BCode
   {
      uint  mOfs;
      int   mMatchDist;
      
      uint16 mNumReferers;
      int16 mMatchLen;
      
      int mActualMatchDist;
      int16 mActualMatchLen;
      
      int8  mPrevMatchIndex;
      uint8 mLiteral;
   };
   
   BDynamicArray<BCode>    mCodes;
      
   BDynamicStream          mOutputByteStream;
   BOutputBitStreamAdapter mOutputBitStream;

   inline uint getDict(uint ofs) const { BDEBUG_ASSERT(ofs < mDataLen); return mpData[ofs]; }
   
   void initHash(void)
   {
      mChains.resize(cNumChains);
      mChains.setAll(-1);

      mNext.resize(mDataLen);
      mNext.setAll(-1);

      for (int i = 0; i < ((int)mDataLen - 1); i++)
      {
         uint a = mpData[i];
         uint b = mpData[i + 1];
         
         uint hash = a | (b << 8);

         mNext[i] = mChains[hash];
         mChains[hash] = i;
      }
   }
      
   void getLZ77Match(uint ofs, BMatch& longestMatch, BMatch& closestMatch, int& closestMatchIndex, uint numPrevMatches, const BMatch* pPrevMatches) const
   {
      longestMatch.mLen = 0;
      
      closestMatch.mLen = 0;
      float closestMatchScore = 1e+12f;
            
      const uint maxMatchLen = Math::Min<uint>(cMaxMatchLength, mDataLen - ofs);

      int matchOfs = mNext[ofs];

      uint count = 0;

      while (matchOfs != -1)
      {
         uint i;
         for (i = 0; i < maxMatchLen; i++)
            if (getDict(matchOfs + i) != getDict(ofs + i))
               break;
      
         if (i >= 2)
         {
            uint matchDist = ofs - matchOfs;

#if 1            
            for (uint j = 0; j < numPrevMatches; j++)
            {
               int lenDelta = (pPrevMatches[j].mLen - i);
               int distDelta = (pPrevMatches[j].mOfs - matchOfs);

               float score = i * 3.0f + j * 6.0f + 10.0f * (float)lenDelta * lenDelta + (float)distDelta * distDelta;

               if (score < closestMatchScore)
               {
                  closestMatchScore = score;
                  closestMatchIndex = j;

                  closestMatch.mLen = i;
                  closestMatch.mOfs = matchOfs;
                  closestMatch.mDist = ofs - matchOfs;
               }
            }
#endif            
            
            bool skip = false;
                        
            if (i == 2)
            {
               if (matchDist > cMaxSmallMatchDist)
                  skip = true;
            }
            else if (i == 3)
            {
               if (matchDist > 8192)
                  skip = true;
            }
            else if (i == 4)
            {
               if (matchDist > 262144)
                  skip = true;
            }
            
            if (!skip)
            {         
               if (i > longestMatch.mLen)
               {
                  longestMatch.mLen = i;
                  longestMatch.mOfs = matchOfs;
                  longestMatch.mDist = ofs - matchOfs;
                  
                  //if (i == maxMatchLen)
                  //   break;
               }
            }               
         }            

         matchOfs = mNext[matchOfs];

         count++;
         if (count > 8192)
            break;
      }
   }

   bool flushCodeBuf(BOutputBitStream* pOutputBitStream) 
   {
      BDynamicArray<uint16> litHist(256*256);
      BDynamicArray<uint> matchFlags;
                  
      BHuffmanCodes matchFlagsCodes(1U << cNumMatchFlagsPerSymbol);
            
      uint curMatchFlags = 0, numMatchFlags = 0;
      
      uint maxLitSymIndex = 0;
      
      for (uint i = 0; i < mCodes.getSize(); i++)
      {
         const BCode& code = mCodes[i];
         if (code.mLiteral)
         {
            uint curC = -code.mMatchDist - 1;
            maxLitSymIndex = Math::Max(maxLitSymIndex, curC);
            
            BDEBUG_ASSERT(curC == mpData[code.mOfs]);
            
            uint prevC = mpData[code.mOfs - 1];
            
            litHist[prevC * 256 + curC]++;
         }
         else
         {
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
         
         //if ((numUniqueSymbols < 1) || (totalHist * 2.0f < 720))
         if (1)
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
      
      BDynamicArray<BHuffmanEnc> litEncoders(litCodes.getSize());
      for (uint i = 0; i < litCodes.getSize(); i++)
      {
         if (!litCodes[i].create())
            return false;
         
         if (!litEncoders[i].init(pOutputBitStream, &litCodes[i]))
            return false;
      }
      
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

      BHuffmanCodes matchCodes(cTotalMatchSymbols);
      BHuffmanCodes generalDistCodes(cNumDistCodes);
      BHuffmanCodes lenDeltaCodes(cTotalLenDeltaCodes);
      BHuffmanCodes distDeltaCodes(cTotalDistDeltaCodes);
      
      for (uint i = 0; i < mCodes.getSize(); i++)
      {
         const BCode& code = mCodes[i];
                           
         if (!code.mLiteral)
         {
            if (code.mPrevMatchIndex >= 0)
            {
               if ((code.mMatchDist == 0) && (code.mMatchLen == 0))            
               {
                  BDEBUG_ASSERT(code.mPrevMatchIndex < cPrevMatchBufSize);
                  matchCodes.incFreq(cFirstRepeatMatchRefSymbol + code.mPrevMatchIndex);
               }
               else
               {
                  BDEBUG_ASSERT(code.mPrevMatchIndex < cPrevMatchBufSize);
                  matchCodes.incFreq(cFirstPrevMatchRefSymbol + code.mPrevMatchIndex);
                  
                  int lenSymbolIndex = (int)code.mMatchLen + (int)(cTotalLenDeltaCodes / 2);
                  int distSymbolIndex = (int)code.mMatchDist + (int)(cTotalDistDeltaCodes / 2);
                  lenDeltaCodes.incFreq(lenSymbolIndex);
                  distDeltaCodes.incFreq(distSymbolIndex);
               }
            }
            else if (code.mMatchLen == 2)
            {
               BDEBUG_ASSERT((code.mMatchDist >= 1) && (code.mMatchDist <= cMaxSmallMatchDist));
               
               uint matchDistCodeIndex;
               for (matchDistCodeIndex = cNumSmallDistCodes - 1; matchDistCodeIndex > 0; matchDistCodeIndex--)
                  if ((code.mMatchDist - 1) >= gSmallDistBase[matchDistCodeIndex])
                     break;
                                       
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
      if (!lenDeltaCodes.create()) return false;
      if (!distDeltaCodes.create()) return false;
      
      BHuffmanEnc matchCodesEnc;
      BHuffmanEnc generalDistCodesEnc;
      BHuffmanEnc lenDeltaCodesEnc;
      BHuffmanEnc distDeltaCodesEnc;
      
      if (!matchCodesEnc.init(pOutputBitStream, &matchCodes)) return false;
      if (!generalDistCodesEnc.init(pOutputBitStream, &generalDistCodes)) return false;
      if (!lenDeltaCodesEnc.init(pOutputBitStream, &lenDeltaCodes)) return false;
      if (!distDeltaCodesEnc.init(pOutputBitStream, &distDeltaCodes)) return false;
      
      uint numMatchFlagsLeft = 0;
      uint curMatchFlagOfs = 0;
     
      uint totalPMRRejected = 0;
      uint totalPMRAccepted = 0;
       
      for (uint i = 0; i < mCodes.getSize(); i++)
      {
         if (!numMatchFlagsLeft)
         {
            if (!matchFlagEnc.encode(matchFlags[curMatchFlagOfs])) return false;
            curMatchFlagOfs++;
            numMatchFlagsLeft = cNumMatchFlagsPerSymbol;
         }
         
         BCode code(mCodes[i]);
         
         uint srcBits;
         uint dstBits = 0;
         if (code.mLiteral)
         {
            srcBits = 8;
            
            uint prevC = mpData[code.mOfs - 1];
            uint curC = -code.mMatchDist - 1;
            
            uint litTableIndex = prevCharToLitTable[prevC];
            
            if (!litEncoders[litTableIndex].encode(curC)) return false;
            
            dstBits = litCodes[litTableIndex].getCodeSize(curC);
         }
         else
         {
            srcBits = code.mActualMatchLen * 8;
                                    
            if (code.mPrevMatchIndex >= 0)
            {
               if ((code.mMatchDist == 0) && (code.mMatchLen == 0))            
               {
                  BDEBUG_ASSERT(code.mPrevMatchIndex < cPrevMatchBufSize);
                  if (!matchCodesEnc.encode(cFirstRepeatMatchRefSymbol + code.mPrevMatchIndex)) return false;
                  
                  dstBits = matchCodes.getCodeSize(cFirstRepeatMatchRefSymbol + code.mPrevMatchIndex);
               }
               else
               {
                  BDEBUG_ASSERT(code.mPrevMatchIndex < cPrevMatchBufSize);
                  if (!matchCodesEnc.encode(cFirstPrevMatchRefSymbol + code.mPrevMatchIndex)) return false;
                  
                  dstBits = matchCodes.getCodeSize(cFirstPrevMatchRefSymbol + code.mPrevMatchIndex);

                  int lenSymbolIndex = (int)code.mMatchLen + (int)(cTotalLenDeltaCodes / 2);
                  int distSymbolIndex = (int)code.mMatchDist + (int)(cTotalDistDeltaCodes / 2);
                  
                  //if (!lenDeltaCodesEnc.encode(lenSymbolIndex)) return false;
                  //dstBits += lenDeltaCodes.getCodeSize(lenSymbolIndex);
                  
                  if (!distDeltaCodesEnc.encode(distSymbolIndex)) return false;
                  dstBits += distDeltaCodes.getCodeSize(distSymbolIndex);
               }
            }
            else if (code.mMatchLen == 2)
            {
               BDEBUG_ASSERT((code.mMatchDist >= 1) && (code.mMatchDist <= cMaxSmallMatchDist));

               uint matchDistCodeIndex;
               for (matchDistCodeIndex = cNumSmallDistCodes - 1; matchDistCodeIndex > 0; matchDistCodeIndex--)
                  if ((code.mMatchDist - 1) >= gSmallDistBase[matchDistCodeIndex])
                     break;

               if (!matchCodesEnc.encode(cFirstSmallMatchSymbol + matchDistCodeIndex)) return false;
               dstBits = matchCodes.getCodeSize(cFirstSmallMatchSymbol + matchDistCodeIndex);
               
               if (gSmallDistExtra[matchDistCodeIndex])
               {
                  uint bitMask = (1U << gSmallDistExtra[matchDistCodeIndex]) - 1U;
                  uint ofs = code.mMatchDist - 1U - gSmallDistBase[matchDistCodeIndex];
                  BDEBUG_ASSERT(ofs <= bitMask);
                  
                  if (!pOutputBitStream->putBits(ofs, gSmallDistExtra[matchDistCodeIndex])) return false;
                  dstBits += gSmallDistExtra[matchDistCodeIndex];
               }
            }
            else
            {
               BDEBUG_ASSERT(code.mMatchLen >= 3 && code.mMatchLen <= cMaxMatchLength);
               
               uint minMatch = 3;
               
               if (code.mMatchDist > 262144)
                  minMatch = 5;
               else if (code.mMatchDist > 8192)
                  minMatch = 4;
               BDEBUG_ASSERT(code.mMatchLen >= minMatch);
               uint matchLenIndex = gLenCode[code.mMatchLen - minMatch] - 257;
               if (!matchCodesEnc.encode(cFirstGeneralMatchSymbol + matchLenIndex)) return false;
               dstBits = matchCodes.getCodeSize(cFirstGeneralMatchSymbol + matchLenIndex);
               
               uint lenExtra = gLenExtra[code.mMatchLen - minMatch];
               if (lenExtra)
               {
                  uint bitMask = (1U << lenExtra) - 1U;
                  if (!pOutputBitStream->putBits((code.mMatchLen - minMatch) & bitMask, lenExtra)) return false;
                  dstBits += lenExtra;
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
               
               uint distExtra = gDistExtra[matchDistIndex];
               if (distExtra)
               {
                  uint bitMask = (1U << distExtra) - 1U;
                  if (!pOutputBitStream->putBits(dist & bitMask, distExtra)) return false;
                  
                  dstBits += distExtra;
               }
            }
         }
         
         numMatchFlagsLeft--;
      }
         
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

