// File: lzp.cpp
#include "xcore.h"
#include "huffman.h"
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"
#include "file\win32FileUtils.h"

class BLZPBase
{
protected:
   enum 
   { 
      cTotalContextBits = 20,
      cTotalContexts = 1 << cTotalContextBits,
      cMaxMatchLength = 256,
      cTotalHuffmanSymbols = 256 + cMaxMatchLength + 1
   };
};

class BLZPComp : public BLZPBase
{
public:
   BLZPComp()
   {
   }
   
   bool pack(const void* pData, uint dataLen, BByteArray& compData)
   {
      mpData = static_cast<const BYTE*>(pData);
      mDataLen = dataLen;
      
      mContexts.resize(cTotalContexts);
      mContexts.setAll(-1);
      
      mCodeBuf.resize(0);
      
      mOutputByteStream.resize(0);
      mOutputBitStream.setStream(&mOutputByteStream);
      mOutputBitStream.begin();
      
      mOutputBitStream.putBits(dataLen, 32);
                  
      uint bytesLeft = dataLen;
      uint curOfs = 0;
      
      while (bytesLeft)
      {
         uint context;
         uint matchLen = getMatch(curOfs, &context); // finalLength

         if ((matchLen > 1) && (matchLen < bytesLeft) && (matchLen < cMaxMatchLength))
         { 
            uint finalLength = matchLen;
            
            uint maxValue = finalLength + getMatch(curOfs + matchLen);
            uint counter = 1;
            
            while (counter < matchLen)
            {
               uint temp = counter + getMatch(curOfs + counter);
               if (temp > maxValue)
               { 
                  finalLength = counter;
                  maxValue = temp;
                }
                  
               counter++;
            }
            
            matchLen = finalLength;
         }            
                  
         uint numBytesEmitted;
         if (!matchLen)
         {
            mCodeBuf.pushBack(getDict(curOfs));
            
            numBytesEmitted = 1;
            
            mContexts[context] = curOfs;
         }
         else
         {
            mCodeBuf.pushBack(256 + matchLen - 1);
            
            numBytesEmitted = matchLen;
         }
         
         BDEBUG_ASSERT(numBytesEmitted <= bytesLeft);
         
         bytesLeft -= numBytesEmitted;
         curOfs += numBytesEmitted;
         
         if (mCodeBuf.getSize() >= 4096)
         {
            if (!flushCodeBuf())
               return false;
         }               
      }     
      
      if (!flushCodeBuf())
         return false;
   
      mOutputBitStream.end();
      compData.swap(mOutputByteStream.getBuf());
      
      return true;
   }

private:
   const BYTE*             mpData;
   uint                    mDataLen;
         
   BDynamicArray<int>      mContexts;
   
   BDynamicArray<uint>     mCodeBuf;
   
   BDynamicStream          mOutputByteStream;
   BOutputBitStreamAdapter mOutputBitStream;
   
   uint getContext(uint ofs) const
   {
      uint a, b, c;
      
      if (ofs < 3)
      {
         a = 0;
         b = 0;
         c = 0;
         if (ofs > 1) b = mpData[ofs - 2];
         if (ofs > 0) c = mpData[ofs - 1];
      }
      else
      {
         BDEBUG_ASSERT((ofs - 1) < mDataLen);
         a = mpData[ofs - 3];
         b = mpData[ofs - 2];
         c = mpData[ofs - 1];
      }
      
      uint context =  a ^ (b << 6) ^ (c << 12);
      BDEBUG_ASSERT(context < cTotalContexts);
      return context;
   }
   
   inline uint getDict(uint ofs) const { BDEBUG_ASSERT(ofs < mDataLen); return mpData[ofs]; }
   
   uint getMatch(uint ofs, uint* pContext = NULL) const
   {
      uint context = getContext(ofs);
      if (pContext) 
         *pContext = context;
      
      int matchOfs = mContexts[context];
      if (matchOfs < 0)
         return 0;
            
      const uint maxMatchLen = Math::Min<uint>(cMaxMatchLength, mDataLen - ofs);

      for (uint i = 0; i < maxMatchLen; i++)
         if (getDict(matchOfs + i) != getDict(ofs + i))
            return i;
      
      return maxMatchLen;
   }
   
   bool flushCodeBuf(void) 
   {
      if (mCodeBuf.isEmpty())
         return true;
      
      BHuffmanCodes huffCodes(cTotalHuffmanSymbols);
      
      for (uint i = 0; i < mCodeBuf.getSize(); i++)
         huffCodes.incFreq(mCodeBuf[i]);  

      huffCodes.incFreq(cTotalHuffmanSymbols - 1);
      
      if (!huffCodes.create())
         return false;
         
      BHuffmanEnc huffEnc;
      if (!huffEnc.init(&mOutputBitStream, &huffCodes))
         return false;
      
      for (uint i = 0; i < mCodeBuf.getSize(); i++)
         if (!huffEnc.encode(mCodeBuf[i]))
            return false;

      if (!huffEnc.encode(cTotalHuffmanSymbols - 1))
         return false;
      
      mCodeBuf.resize(0);
      
      return true;
   }
};

class BLZPDecomp : public BLZPBase
{
public:
   BLZPDecomp()
   {
   }
   
   bool unpack(const void* pData, uint dataLen, BByteArray& decompData)
   {
      if (!dataLen)
         return false;
 
      mContexts.resize(cTotalContexts);
      mContexts.setAll(-1);
           
      mInputByteStream.set(pData, dataLen, cSFReadable | cSFSeekable);
      mInputBitStream.setStream(&mInputByteStream);
   
      if (!mInputBitStream.getBits(mUnpackedSize, 32))
         return false;
      
      BHuffmanDec huffDec;
      if (!huffDec.init(&mInputBitStream))
         return false;
                     
      mUnpackedData.resize(mUnpackedSize);
                  
      uint curOfs = 0;
      uint bytesLeft = mUnpackedSize;
      
      while (bytesLeft)
      {
         uint sym;
         if (!huffDec.getNextSymbol(sym))
            return false;
         
         if (sym == (cTotalHuffmanSymbols - 1))
         {
            if (!huffDec.init(&mInputBitStream))
               return false;
            
            if (!huffDec.getNextSymbol(sym))
               return false;
            
            if (sym == (cTotalHuffmanSymbols - 1))
               return false;
         }
         
         uint context = getContext(curOfs);
         
         uint numBytesDecoded;
         if (sym < 256)
         {
            mUnpackedData[curOfs] = (BYTE)sym;   
            mContexts[context] = curOfs;
           
            numBytesDecoded = 1;
         }
         else
         {
            uint matchLen = sym - 255;
            if (matchLen > bytesLeft)
               return false;
               
            int matchOfs = mContexts[context];
            if (matchOfs < 0)
               return false;
                        
            for (uint i = 0; i < matchLen; i++)
               mUnpackedData[curOfs + i] = mUnpackedData[matchOfs + i];
            
            numBytesDecoded = matchLen;
         }
         
         curOfs += numBytesDecoded;
         bytesLeft -= numBytesDecoded;
      }
      
      decompData.swap(mUnpackedData);
      
      return true;
   }
   
private:
   uint                    mUnpackedSize;
   BByteArray              mUnpackedData;
   
   BByteStream             mInputByteStream;
   BInputBitStreamAdapter  mInputBitStream;
      
   BDynamicArray<int>      mContexts;
   
   uint getContext(uint ofs) const
   {
      uint a, b, c;

      if (ofs < 3)
      {
         a = 0;
         b = 0;
         c = 0;
         if (ofs > 1) b = mUnpackedData[ofs - 2];
         if (ofs > 0) c = mUnpackedData[ofs - 1];
      }
      else
      {
         a = mUnpackedData[ofs - 3];
         b = mUnpackedData[ofs - 2];
         c = mUnpackedData[ofs - 1];
      }

      uint context =  a ^ (b << 6) ^ (c << 12);
      BDEBUG_ASSERT(context < cTotalContexts);
      return context;
   }
   
   

};

int lzpTest(int argC, const char* argV[])
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
      
      BByteArray compData;
      BLZPComp comp;
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
      BLZPDecomp decomp;
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

