//---------------------------------------------------------------------------------------------------------------------------------------------
// File: huffman.h
//---------------------------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "bitStream.h"

//---------------------------------------------------------------------------------------------------------------------------------------------

class BHuffmanCodes
{
public:
   BHuffmanCodes();
   BHuffmanCodes(const BHuffmanCodes& other);
   BHuffmanCodes(uint numSymbols);
   ~BHuffmanCodes();
   
   BHuffmanCodes& operator= (const BHuffmanCodes& rhs);

   void init(uint numSymbols);

   void setFreq(uint numSymbols, const int* pFreq);

   inline uint getNumSymbols() const { return mNumSymbols; }

   void incFreq(uint symbolIndex, uint val = 1);
   
   inline uint getFreq(uint symbolIndex) const { return mFreq[symbolIndex]; }
   inline void setFreq(uint symbolIndex, uint freq) { mFreq[symbolIndex] = freq; }

   inline const BDynamicArray<int>& getFreq() const { mFreq; }

   inline int getCodeSize(uint symbolIndex) const { return mCodeSizes[symbolIndex]; }
   inline uint getCode(uint symbolIndex) const { return mCodes[symbolIndex]; }

   inline const BDynamicArray<int>& getCodeSizes() const { return mCodeSizes; }
   inline const BDynamicArray<uint>& getCodes() const { return mCodes; }

   bool create(uint maxCodeSize = 15);
   bool hasCodes() const { return mCodeSizes.getSize() > 0; }
   
   uint computeTotalBits() const;

private:
   uint                 mNumSymbols;

   BDynamicArray<int>   mFreq;

   BDynamicArray<int>   mHeap;     //[cMaxSymbols + 1];
   BDynamicArray<int>   mOthers;   //[cMaxSymbols];
   BDynamicArray<int>   mCodeList; //[cMaxSymbols];
   int                  mCodeListLen;
   int                  mNumCodes[33];
   int                  mNextCode[33];
   BDynamicArray<int>   mNewCodeSizes;//[cMaxSymbols];

   BDynamicArray<int>   mCodeSizes;
   BDynamicArray<uint>  mCodes;

   void downHeap(int *mHeap, int *sym_freq, int heap_len, int i);
   void codeSizes(int num_symbols, int *sym_freq, int *codeSizes);
   void sortCodeSizes(int num_symbols, int *codeSizes);
   void fixCodeSizes(int max_code_size);
   void makeCodes(int num_symbols, int *codeSizes, int max_code_size, uint *codes);
};

//---------------------------------------------------------------------------------------------------------------------------------------------

class BHuffmanEnc
{
public:
   BHuffmanEnc();
            
   inline uint getTotalBits() { return mTotalBits; }
   inline void clearTotalBits() { mTotalBits = 0;}

   bool init(BOutputBitStream* pBitStream, const BHuffmanCodes* pCodes);
   
   inline void setBitStream(BOutputBitStream* pBitStream) { mpBitStream = pBitStream; }
   inline BOutputBitStream* getBitStream() { return mpBitStream; }

   bool encode(uint symbolIndex);
      
private:
   BOutputBitStream*    mpBitStream;
      
   uint                 mTotalBits;

   const BHuffmanCodes* mpCodes;

   bool putBits(uint bits, uint numBits);

   static int* repeatLast(int *dst, int size, int run_len, BHuffmanCodes& codes);
   static int* repeatZero(int *dst, int run_len, BHuffmanCodes& codes);
   
   bool compressCodeSizes(void);
};

//---------------------------------------------------------------------------------------------------------------------------------------------

class BHuffmanDec
{
public:
   BHuffmanDec();
               
   bool init(BInputBitStream* pBitStream);
   
   inline void setBitStream(BInputBitStream* pBitStream) { mpBitStream = pBitStream; }
   inline BInputBitStream* getBitStream() { return mpBitStream; }
   
   inline uint getNumSymbols() const { return mNumSymbols; }
   
   bool getNextSymbol(uint& symbolIndex);
         
private:
   BInputBitStream*     mpBitStream;
      
   BDynamicArray<uint8> mCodeSize;
   
   uint                 mNumSymbols;
   BDynamicArray<int>   mLookUp;
   BDynamicArray<int>   mTree;

   int getNextSymbolInternal(const int* pLookUp, const int* pTree);
   bool buildDecoderTables(int num_symbols, const uint8* pCodeSize, BDynamicArray<int>& look_up, BDynamicArray<int>& tree);
};

//---------------------------------------------------------------------------------------------------------------------------------------------

class BUniversalCodec
{
public:
   BUniversalCodec();
      
   bool encodeOmega(BOutputBitStream* pBitStream, uint val);
   bool encodeOmegaSigned(BOutputBitStream* pBitStream, int val);

   bool decodeOmega(BInputBitStream* pBitStream, uint& value);
   bool decodeOmegaSigned(BInputBitStream* pBitStream, int& value);

private:
   uchar mReverseByteTable[256];
   uchar mSigBits[256];
   
   struct BOmegaCode
   {
      ushort mCode;
      uchar mLen;
   }; 

   enum { cOmegaCodeTableSize = 256 };
   BOmegaCode mOmegaCodeTable[cOmegaCodeTableSize];

   void initOmegaCodeTable(void);
   void initReverseBits(void);

   DWORD reverseDWORD(DWORD i) const;

   int numBits(uint val) const;
   uint64 getOmegaCode(int& len, uint val) const;
};
