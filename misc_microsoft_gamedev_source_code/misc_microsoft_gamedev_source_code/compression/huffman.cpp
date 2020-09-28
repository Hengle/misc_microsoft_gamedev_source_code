//---------------------------------------------------------------------------------------------------------------------------------------------
// File: huffman.cpp
//---------------------------------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "huffman.h"

//---------------------------------------------------------------------------------------------------------------------------------------------

BHuffmanCodes::BHuffmanCodes() :
   mNumSymbols(0),
   mCodeListLen(0)
{
   Utils::ClearObj(mNumCodes);
   Utils::ClearObj(mNextCode);
}

//---------------------------------------------------------------------------------------------------------------------------------------------

BHuffmanCodes::BHuffmanCodes(const BHuffmanCodes& other)
{
   *this = other;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

BHuffmanCodes::BHuffmanCodes(uint numSymbols) :
   mNumSymbols(0),
   mCodeListLen(0)
{
   Utils::ClearObj(mNumCodes);
   Utils::ClearObj(mNextCode);
   
   init(numSymbols);
}

//---------------------------------------------------------------------------------------------------------------------------------------------

BHuffmanCodes::~BHuffmanCodes()
{
   mFreq.clear();
   mHeap.clear();    
   mOthers.clear();  
   mCodeList.clear();
   mNewCodeSizes.clear();
   mCodeSizes.clear();
   mCodes.clear();
}

//---------------------------------------------------------------------------------------------------------------------------------------------

BHuffmanCodes& BHuffmanCodes::operator= (const BHuffmanCodes& rhs)
{
   if (this == &rhs)
      return *this;

   mNumSymbols = rhs.mNumSymbols;
   mFreq = rhs.mFreq;
   mHeap = rhs.mHeap;    
   mOthers = rhs.mOthers;
   mCodeList = rhs.mCodeList; 
   mCodeListLen = rhs.mCodeListLen;
   memcpy(mNumCodes, rhs.mNumCodes, sizeof(mNumCodes));
   memcpy(mNextCode, rhs.mNextCode, sizeof(mNextCode));
   mNewCodeSizes = rhs.mNewCodeSizes;
   mCodeSizes = rhs.mCodeSizes;
   mCodes = rhs.mCodes;
   
   return *this;
}


//---------------------------------------------------------------------------------------------------------------------------------------------

void BHuffmanCodes::init(uint numSymbols)
{
   mNumSymbols = numSymbols;
   
   mFreq.clear();
   mFreq.resize(numSymbols);
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
void BHuffmanCodes::setFreq(uint numSymbols, const int* pFreq)
{
   BDEBUG_ASSERT(numSymbols);
   BDEBUG_ASSERT(pFreq);
   
   mNumSymbols = numSymbols;
   
   mFreq.clear();
   mFreq.pushBack(pFreq, numSymbols);
}

//---------------------------------------------------------------------------------------------------------------------------------------------

void BHuffmanCodes::incFreq(uint symbolIndex, uint val) 
{ 
   if (symbolIndex >= mNumSymbols)
   {
      mNumSymbols = symbolIndex + 1;
      
      mFreq.resize(mNumSymbols);
   }
   
   mFreq[symbolIndex] += val; 
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
bool BHuffmanCodes::create(uint maxCodeSize)
{
   if (mNumSymbols < 1)
      return false;

   maxCodeSize = Math::Min<uint>(maxCodeSize, 15U);            
   
   mHeap.resize(mNumSymbols + 1);         
   mHeap.setAll(0);
   
   mOthers.resize(mNumSymbols);
   mOthers.setAll(0);
   
   mCodeList.resize(mNumSymbols);
   mCodeList.setAll(0);
   
   mNewCodeSizes.resize(mNumSymbols);
   mNewCodeSizes.setAll(0);
   
   mCodeSizes.resize(mNumSymbols);
   mCodeSizes.setAll(0);
   
   mCodes.resize(mNumSymbols);
   mCodes.setAll(0);
   
   BDynamicArray<int> freq(mFreq);
   
   codeSizes(mNumSymbols, freq.getPtr(), mCodeSizes.getPtr());
   sortCodeSizes(mNumSymbols, mCodeSizes.getPtr());
   fixCodeSizes(maxCodeSize);
   makeCodes(mNumSymbols, mCodeSizes.getPtr(), maxCodeSize, mCodes.getPtr());
   
   mHeap.clear();
   mOthers.clear();
   mCodeList.clear();
   mNewCodeSizes.clear();
   
   return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

uint BHuffmanCodes::computeTotalBits() const
{
   uint totalBits = 0;
   
   for (uint i = 0; i < mNumSymbols; i++)
      totalBits += mFreq[i] * mCodeSizes[i];
   
   return totalBits;
}

//---------------------------------------------------------------------------------------------------------------------------------------------
      
void BHuffmanCodes::downHeap(int *mHeap, int *sym_freq, int heap_len, int i)
{
   int j, k;

   k = mHeap[i];

   while ((j = 2 * i) <= heap_len)
   {
      if (j < heap_len)
      {
         if (sym_freq[mHeap[j]] > sym_freq[mHeap[j + 1]])
            j++;
         else if (sym_freq[mHeap[j]] == sym_freq[mHeap[j + 1]])
         {
            if (mHeap[j] < mHeap[j + 1])
               j++;
         }
      }

      if (sym_freq[k] < sym_freq[mHeap[j]])
         break;
      else if (sym_freq[k] == sym_freq[mHeap[j]])
      {
         if (k > mHeap[j])
            break;
      }

      mHeap[i] = mHeap[j]; i = j;
   }

   mHeap[i] = k;
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
void BHuffmanCodes::codeSizes(int num_symbols, int *sym_freq, int *codeSizes)
{
   int i, j, k, heap_len;

   for (i = 0; i < num_symbols; i++)
      mOthers[i] = -1;

   for (i = 0; i < num_symbols; i++)
      codeSizes[i] = 0;

   heap_len = 1;

   for (i = 0; i < num_symbols; i++)
      if (sym_freq[i] != 0)
         mHeap[heap_len++] = i;

   heap_len--;

   if (heap_len <= 1)
   {
      if (heap_len == 0)
         return;

      codeSizes[mHeap[1]] = 1;

      return;
   }

   j = (heap_len >> 1);

   while (j != 0)
      downHeap(mHeap.getPtr(), sym_freq, heap_len, j--);

   do
   {
      i = mHeap[1];

      mHeap[1] = mHeap[heap_len--];

      downHeap(mHeap.getPtr(), sym_freq, heap_len, 1);

      j = mHeap[1];

      sym_freq[j] += sym_freq[i];

      downHeap(mHeap.getPtr(), sym_freq, heap_len, 1);

      do { codeSizes[k = j]++; } while ((j = mOthers[j]) != -1);

      mOthers[k] = i;

      do { codeSizes[i]++; } while ((i = mOthers[i]) != -1);

   } while (heap_len != 1);
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
void BHuffmanCodes::sortCodeSizes(int num_symbols, int *codeSizes)
{
   int i, j;

   mCodeListLen = 0;

   for (uint i = 0; i < 33; i++)
      mNumCodes[i] = 0;

   for (i = 0; i < num_symbols; i++)
      mNumCodes[codeSizes[i]]++;

   for (i = 1, j = 0; i <= 32; i++)
   {
      mNextCode[i] = j;
      j += mNumCodes[i];
   }

   for (i = 0; i < num_symbols; i++)
   {
      if ((j = codeSizes[i]) != 0)
      {
         mCodeList[mNextCode[j]++] = i;
         mCodeListLen++;
      }
   }
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
void BHuffmanCodes::fixCodeSizes(int max_code_size)
{
   int i;
   uint total;

   if (mCodeListLen > 1)
   {
      for (i = max_code_size + 1; i <= 32; i++)
         mNumCodes[max_code_size] += mNumCodes[i];

      total = 0;
      for (i = max_code_size; i > 0; i--)
         total += ( ((uint)mNumCodes[i]) << (max_code_size - i) );

      while (total != (1UL << max_code_size))
      {
         mNumCodes[max_code_size]--;

         for (i = max_code_size - 1; i > 0; i--)
         {
            if (mNumCodes[i])
            {
               mNumCodes[i]--;
               mNumCodes[i + 1] += 2;

               break;
            }
         }

         total--;
      }
   }
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
void BHuffmanCodes::makeCodes(
   int num_symbols, int *codeSizes,
   int max_code_size, uint *codes)
{
   int i, l;
   uint j, k;

   if (!mCodeListLen)
      return;

   for (i = 0; i < num_symbols; i++)
      codes[i] = 0;

   for (i = 1, k = 0; i <= max_code_size; i++)
      for (l = mNumCodes[i]; l > 0; l--)
         mNewCodeSizes[k++] = i;

   mNextCode[1] = j = 0;
   for (i = 2; i <= max_code_size; i++)
      mNextCode[i] = j = ((j + mNumCodes[i - 1]) << 1);

   for (i = 0; i < mCodeListLen; i++)
      codeSizes[mCodeList[i]] = mNewCodeSizes[i];

   for (i = 0; i < num_symbols; i++)
   {
      if (codeSizes[i])
      {
         j = mNextCode[codeSizes[i]]++;

         for (l = codeSizes[i], k = 0; l > 0; l--)
         {
            k = (k << 1) | (j & 1);
            j >>= 1;
         }

         codes[i] = k;
      }
   }
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
BHuffmanEnc::BHuffmanEnc() : 
   mpBitStream(NULL),
   mpCodes(NULL), 
   mTotalBits(0)
{
}

//---------------------------------------------------------------------------------------------------------------------------------------------
         
bool BHuffmanEnc::init(BOutputBitStream* pBitStream, const BHuffmanCodes* pCodes)
{
   BDEBUG_ASSERT(pBitStream);
   BDEBUG_ASSERT(pCodes);
   BDEBUG_ASSERT(pCodes->getNumSymbols() >= 1);
   BDEBUG_ASSERT(pCodes->hasCodes());
         
   mpBitStream = pBitStream;
   
   mpCodes = pCodes;
   
   return compressCodeSizes();
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
bool BHuffmanEnc::encode(uint symbolIndex)
{
   BDEBUG_ASSERT(mpCodes->getCodeSize(symbolIndex));
   return putBits(mpCodes->getCode(symbolIndex), mpCodes->getCodeSize(symbolIndex));
}

//---------------------------------------------------------------------------------------------------------------------------------------------
      
bool BHuffmanEnc::putBits(uint bits, uint numBits)
{
   mTotalBits += numBits;
   
   if (mpBitStream)
      return mpBitStream->putBits(bits, numBits);
   
   return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
int* BHuffmanEnc::repeatLast(int *dst, int size, int run_len, BHuffmanCodes& codes)
{
   if (run_len < 3)
   {
      codes.incFreq(size, run_len);

      while (run_len--)
         *dst++ = size;
   }
   else
   {
      codes.incFreq(16);

      *dst++ = 16;
      *dst++ = run_len - 3;
   }

   return (dst);
}

//---------------------------------------------------------------------------------------------------------------------------------------------
   
int* BHuffmanEnc::repeatZero(int *dst, int run_len, BHuffmanCodes& codes)
{
   if (run_len < 3)
   {
      codes.incFreq(0, run_len);

      while (run_len--)
         *dst++ = 0;
   }
   else if (run_len <= 10)
   {
      codes.incFreq(17);

      *dst++ = 17;
      *dst++ = run_len - 3;
   }
   else
   {
      codes.incFreq(18);

      *dst++ = 18;
      *dst++ = run_len - 11;
   }

   return (dst);
}

//---------------------------------------------------------------------------------------------------------------------------------------------
            
bool BHuffmanEnc::compressCodeSizes(void)
{
   const int *src;
   int *dst;
   
   int size, last_size = 0xFF;
   int run_len_z = 0, run_len_nz = 0;
   
   BHuffmanCodes codeLengthCodes;
         
   const uint cCLNumSymbols = 19;
   codeLengthCodes.init(cCLNumSymbols);
   
   int numValidCodes;
   for (numValidCodes = mpCodes->getNumSymbols() - 1; numValidCodes >= 0; numValidCodes--)
      if (mpCodes->getCodeSize(numValidCodes))
         break;
   
   numValidCodes = Math::Max(1, numValidCodes + 1);
                     
   src = mpCodes->getCodeSizes().getPtr();
   
   BDynamicArray<int> clSymbols(numValidCodes);
   dst = clSymbols.getPtr();

   for (int codes_left = numValidCodes; codes_left > 0; codes_left--)
   {
      if ((size = *src++) == 0)
      {
         if (run_len_nz)
         {
            dst = repeatLast(dst, last_size, run_len_nz, codeLengthCodes);
            run_len_nz = 0;
         }

         if (++run_len_z == 138)
         {
            dst = repeatZero(dst, run_len_z, codeLengthCodes);
            run_len_z = 0;
         }
      }
      else
      {
         if (run_len_z)
         {
            dst = repeatZero(dst, run_len_z, codeLengthCodes);
            run_len_z = 0;
         }

         if (size != last_size)
         {
            if (run_len_nz)
            {
               dst = repeatLast(dst, last_size, run_len_nz, codeLengthCodes);
               run_len_nz = 0;
            }

            codeLengthCodes.incFreq(size);
            
            *dst++ = size;
         }
         else if (++run_len_nz == 6)
         {
            dst = repeatLast(dst, last_size, run_len_nz, codeLengthCodes);
            run_len_nz = 0;
         }
      }

      last_size = size;
   }

   if (run_len_nz)
      dst = repeatLast(dst, last_size, run_len_nz, codeLengthCodes);
   else if (run_len_z)
      dst = repeatZero(dst, run_len_z, codeLengthCodes);

   const int* pCLSymbolsEnd = dst;
   const uint numCLCSymbols = dst - clSymbols.getPtr();
   numCLCSymbols;
   BDEBUG_ASSERT(numCLCSymbols <= clSymbols.getSize());
   
   codeLengthCodes.create(7);
   
   //-----
   
   int c = Math::CodeSize(numValidCodes);
   if (c >= 0xF) return false;
   if (!putBits(c, 4)) return false;
   if (!putBits(numValidCodes, c)) return false;
         
   static const uchar bitLengthOrder[] = { 16, 17, 18, 0, 8,  7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

   int bit_lengths;
   for (bit_lengths = 18; bit_lengths >= 0; bit_lengths--)
      if (codeLengthCodes.getCodeSize(bitLengthOrder[bit_lengths]))
         break;

   bit_lengths = Math::Max(4, (bit_lengths + 1));

   if (!putBits(bit_lengths - 4, 4)) 
      return false;

   int i;
   for (i = 0; i < bit_lengths; i++)
   {
      if (!putBits(codeLengthCodes.getCodeSize(bitLengthOrder[i]), 3)) 
         return false;
   }

   src = clSymbols.getPtr();
   
   while (src < pCLSymbolsEnd)
   {
      i = *src++;

      if (!putBits(codeLengthCodes.getCode(i), codeLengthCodes.getCodeSize(i))) 
         return false;

      if (i == 16)
      {
         if (!putBits(*src++, 2)) 
            return false;
      }
      else if (i == 17)
      {
         if (!putBits(*src++, 3)) 
            return false;
      }
      else if (i == 18)
      {
         if (!putBits(*src++, 7)) 
            return false;
      }
   }
   
   BDEBUG_ASSERT(src == pCLSymbolsEnd);
   
   return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

BHuffmanDec::BHuffmanDec() :
   mpBitStream(NULL),
   mNumSymbols(0)
{
}

//---------------------------------------------------------------------------------------------------------------------------------------------

bool BHuffmanDec::init(BInputBitStream* pBitStream)
{
   BDEBUG_ASSERT(pBitStream);
   mpBitStream = pBitStream;
      
   uint c;
   if (!mpBitStream->getBits(c, 4))
      return false;
   
   if (!c)
      return false;
      
   if (!mpBitStream->getBits(mNumSymbols, c))
      return false;
   if (!mNumSymbols)
      return false;
   
   BDynamicArray<int> clLookUp;   
   BDynamicArray<int> clTree;
   
   uint numCLCodes;
   if (!mpBitStream->getBits(numCLCodes, 4))
      return false;
   numCLCodes += 4;
   
   static const uchar bitLengthOrder[] = { 16, 17, 18, 0, 8,  7,  9, 6, 10,  5, 11, 4, 12,  3, 13, 2, 14,  1, 15 };
   
   const uint cCLNumSymbols = 19;
   BDynamicArray<uint8> clCodeSize(cCLNumSymbols);
      
   for (uint i = 0; i < numCLCodes; i++)
   {
      uint l;
      if (!mpBitStream->getBits(l, 3))
         return false;
      clCodeSize[bitLengthOrder[i]] = (uint8)l;
   }

   if (!buildDecoderTables(cCLNumSymbols, clCodeSize.getPtr(), clLookUp, clTree))
      return false;
   
   mCodeSize.resize(mNumSymbols);   
         
   for (uint cur_sym = 0; cur_sym < mNumSymbols; )
   {
      int code_size = getNextSymbolInternal(clLookUp.getPtr(), clTree.getPtr());
      if (code_size < 0)
         return false;
         
      uint rep_len = 0;
      signed char rep_code_size = 0;

      if (code_size <= 15)
      {
         if ((cur_sym + 1) > mNumSymbols)
            return false;

         mCodeSize[cur_sym++] = (uchar)code_size;
         continue;
      }

      switch (code_size)
      {
         case 16:
         {
            if (!mpBitStream->getBits(rep_len, 2))
               return false;
            rep_len += 3;
            
            if (cur_sym)
               rep_code_size = mCodeSize[cur_sym - 1];
            else
               rep_code_size = 0;
            break;
         }
         case 17:
         {
            if (!mpBitStream->getBits(rep_len, 3))
               return false;
            rep_len += 3;
            
            rep_code_size = 0;
            break;
         }
         case 18:
         {
            if (!mpBitStream->getBits(rep_len, 7))
               return false;
            rep_len += 11;
            
            rep_code_size = 0;
            break;
         }
      }

      if ((cur_sym + rep_len) > mNumSymbols)
         return false;

      for ( ; rep_len; rep_len--)
         mCodeSize[cur_sym++] = rep_code_size;
   }

   return buildDecoderTables(mNumSymbols, mCodeSize.getPtr(), mLookUp, mTree);
}

//-----------------------------------------------------------------------------------------------------------

int BHuffmanDec::getNextSymbolInternal(const int* pLookUp, const int* pTree)
{
   uint bits;
   if (!mpBitStream->peekBits16(bits))
      return -1;
      
   int symbol;
   if ((symbol = pLookUp[bits & 0xFF]) < 0)
   {
      uint temp = (uint)(bits >> 8);
      do
      {
         symbol = pTree[(-symbol - 1) + (temp & 1)];
         temp >>= 1;
      } while (symbol < 0);
   }

   uint num_bits = symbol >> 16;
   symbol &= 0xFFFF;

   if (!mpBitStream->removeBits(num_bits))
      return -1;
      
   return symbol;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

bool BHuffmanDec::getNextSymbol(uint& symbolIndex)
{
   int result = getNextSymbolInternal(mLookUp.getPtr(), mTree.getPtr());
   if (result < 0)
      return false;
   
   symbolIndex = result;
   return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

bool BHuffmanDec::buildDecoderTables(
   int num_symbols, const uint8* pCodeSize,
   BDynamicArray<int>& look_up, BDynamicArray<int>& tree)
{
   uint i, j;
   uint total;
   uint old_code, new_code;
   int next_free_entry, current_entry;
   int num_codes[17];
   uint next_code[17];
   
   look_up.resize(256);
   look_up.setAll(0);
   
   tree.resize(num_symbols * 2);
   tree.setAll(0);
   
   BDynamicArray<uint> code(num_symbols);

   for (i = 0; i <= 16; i++)
      num_codes[i] = 0;

   for (i = 0; i < ((uint)num_symbols); i++)
      num_codes[pCodeSize[i]]++;

   for (i = 1, next_code[0] = next_code[1] = 0, total = 0; i <= 15; i++)
      next_code[i + 1] = (uint)(total = ((total + ((uint)num_codes[i])) << 1));

   if (total != 0x10000L)
   {
      for (i = 16, j = 0; i != 0; i--)
         if ((j += num_codes[i]) > 1)
            return false;
   }

   for (i = 0; i < ((uint)num_symbols); i++)
      code[i] = next_code[pCodeSize[i]]++;

   for (i = 0; i < 256; i++)
      look_up[i] = 0;

   for (i = 0; i < (((uint)num_symbols) << 1); i++)
      tree[i] = 0;

   next_free_entry = -1;

   for (i = 0; i < ((uint)num_symbols); i++)
   {
      if (pCodeSize[i] != 0)
      {
         old_code = code[i];

         new_code = 0;

         for (j = pCodeSize[i]; j != 0; j--)
         {
            new_code = (new_code << 1) | (old_code & 1);
            old_code >>= 1;
         }

         if (pCodeSize[i] <= 8)
         {
            j = 1 << pCodeSize[i];

            while (new_code < 256)
            {
               look_up[new_code] = i | (pCodeSize[i] << 16);

               new_code += j;
            }
         }
         else
         {
            if ((current_entry = look_up[new_code & 0xFF]) == 0)
            {
               look_up[new_code & 0xFF] = current_entry = next_free_entry;

               next_free_entry -= 2;
            }

            new_code >>= 7;

            for (j = pCodeSize[i]; j > 9; j--)
            {
               current_entry -= ((new_code >>= 1) & 1);

               if (tree[-current_entry - 1] == 0)
               {
                  tree[-current_entry - 1] = next_free_entry;

                  current_entry = next_free_entry;

                  next_free_entry -= 2;
               }
               else
                  current_entry = tree[-current_entry - 1];
            }

            current_entry -= ((new_code >>= 1) & 1);

            tree[-current_entry - 1] = i | (pCodeSize[i] << 16);
         }
      }
   }

   return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

BUniversalCodec::BUniversalCodec() 
{
   initReverseBits();
   initOmegaCodeTable();
}

//---------------------------------------------------------------------------------------------------------------------------------------------

bool BUniversalCodec::encodeOmega(BOutputBitStream* pBitStream, uint val)
{
   if (val < cOmegaCodeTableSize)
      return pBitStream->putBits(mOmegaCodeTable[val].mCode, mOmegaCodeTable[val].mLen);

   int len;
   uint64 code = getOmegaCode(len, val);

   return pBitStream->putBits(code, len);
}

//---------------------------------------------------------------------------------------------------------------------------------------------

bool BUniversalCodec::encodeOmegaSigned(BOutputBitStream* pBitStream, int val)
{
   if (!pBitStream->putBits((val < 0) ? 1 : 0, 1))
      return false;

   if (val < 0)
      val = -val - 1;

   return encodeOmega(pBitStream, val);
}

//---------------------------------------------------------------------------------------------------------------------------------------------

bool BUniversalCodec::decodeOmega(BInputBitStream* pBitStream, uint& value)
{
   uint bit;
   if (!pBitStream->peekBits(bit, 1))
      return false;
      
   if (!bit)
   {  
      value = 0;
      return pBitStream->removeBits(1);
   }

   uint n;
   if (!pBitStream->getBits(n, 2))
      return false;

   n = mReverseByteTable[n << 6];

   for ( ; ; )
   {
      if (!pBitStream->peekBits(bit, 1))
         return false;
               
      if (!bit)
         break;
      
      const int len = n + 1;
      if (!pBitStream->getBits(n, len))
         return false;

      n = reverseDWORD(n << (32 - len));
   }      

   BDEBUG_ASSERT(n != 0);
   value = n - 1;
   return pBitStream->removeBits(1);
}

//---------------------------------------------------------------------------------------------------------------------------------------------

bool BUniversalCodec::decodeOmegaSigned(BInputBitStream* pBitStream, int& value)
{
   uint signFlag;
   if (!pBitStream->getBits(signFlag, 1))
      return false;

   uint uvalue;
   if (!decodeOmega(pBitStream, uvalue))
      return false;

   value = uvalue;
   if (signFlag)
      value = -value - 1;

   return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------

void BUniversalCodec::initOmegaCodeTable(void)
{
   for (int i = 0; i < cOmegaCodeTableSize; i++)
   {
      int len;
      uint64 code = getOmegaCode(len, i);
      BDEBUG_ASSERT(len <= 16);
      mOmegaCodeTable[i].mCode = static_cast<ushort>(code);
      mOmegaCodeTable[i].mLen = static_cast<uchar>(len);
   }
}

//---------------------------------------------------------------------------------------------------------------------------------------------

void BUniversalCodec::initReverseBits(void)
{
   for (int i = 0; i < 256; i++)
   {
      int r = 0;
      int v = i;

      for (int j = 0; j < 8; j++)
      {
         r <<= 1;
         r |= (v & 1);
         v >>= 1;
      }

      mReverseByteTable[i] = static_cast<uchar>(r);

      int val = i;
      int l = 0;
      while (val)
      {
         val >>= 1;
         l++;
      }
      mSigBits[i] = static_cast<uchar>(l);
   }
}

//---------------------------------------------------------------------------------------------------------------------------------------------

DWORD BUniversalCodec::reverseDWORD(DWORD i) const
{
   return (mReverseByteTable[ i        & 0xFF] << 24) | 
          (mReverseByteTable[(i >>  8) & 0xFF] << 16) |
          (mReverseByteTable[(i >> 16) & 0xFF] <<  8) |
          (mReverseByteTable[(i >> 24) & 0xFF]);
}

//---------------------------------------------------------------------------------------------------------------------------------------------

int BUniversalCodec::numBits(uint val) const
{
   int l = 0;
   if (val & 0xFF000000)
   {
      l = 24;
      val >>= 24;
   }
   else if (val & 0x00FF0000)
   {
      l = 16;
      val >>= 16;
   }
   else if (val & 0x0000FF00)
   {
      l = 8;
      val >>= 8;
   }

   return l + mSigBits[val];
}

//---------------------------------------------------------------------------------------------------------------------------------------------

uint64 BUniversalCodec::getOmegaCode(int& len, uint val) const
{
   BDEBUG_ASSERT(val != UINT_MAX);
   uint valueToCode = val + 1;

   uint64 code = 0;
   int bitPos = 63;

   while (valueToCode != 1)
   {
      BDEBUG_ASSERT(valueToCode > 0);
      const int valueToCodeBits = numBits(valueToCode);

      bitPos -= valueToCodeBits;
      BDEBUG_ASSERT(bitPos >= 0);

      valueToCode = reverseDWORD(valueToCode) >> (32 - valueToCodeBits);
      code |= (static_cast<uint64>(valueToCode) << bitPos);

      valueToCode = valueToCodeBits - 1;
   }

   BDEBUG_ASSERT(bitPos <= 64);

   code >>= bitPos;
   len = 64 - bitPos;

   return code;
}

//---------------------------------------------------------------------------------------------------------------------------------------------


/*
// Example/Test code:
#include "huffman.h"
#include "stream\dynamicStream.h"

void test()
{
   BHuffmanCodes codes;
   codes.init(3);
   codes.incFreq(0, 2);
   codes.incFreq(1, 3);
   codes.incFreq(2, 1);
   codes.create();

   BDynamicStream bytes;
   BOutputBitStreamAdapter outputBitStream;
   outputBitStream.setStream(&bytes);
   outputBitStream.begin();

   BHuffmanEnc encoder;
   encoder.init(&outputBitStream, &codes);

   encoder.encode(0);
   encoder.encode(1);
   encoder.encode(1); 
   encoder.encode(0); 
   encoder.encode(1); 
   encoder.encode(2); 

   outputBitStream.end();

   bytes.seek(0);

   BInputBitStreamAdapter inputBitStream;
   inputBitStream.setStream(&bytes);
   bool result = inputBitStream.begin();

   BHuffmanDec decoder;
   result = decoder.init(&inputBitStream);

   for ( ; ; )
   {
      uint symbolIndex;
      result = decoder.getNextSymbol(symbolIndex);
      if ((!result) || (symbolIndex == 2))
      break;
   }

   inputBitStream.end();
}
*/

#if 0
#include "huffman.h"
#include "stream\dynamicStream.h"
#include "math\random.h"

void huffTest()
{  
   Random rand;

   uint counter = 0;
   for ( ; ; )
   {
      BHuffmanCodes codes;
      const uint numSymbols = rand.iRand(1, 1024);

      codes.init(numSymbols);
      for (uint i = 0; i < numSymbols; i++)
      {
         uint f = rand.iRand(0, 65536);
         codes.incFreq(i, f);
      }
      bool result = codes.create();
      BVERIFY(result);

      BDynamicStream bytes;
      BOutputBitStreamAdapter outputBitStream;
      outputBitStream.setStream(&bytes);
      outputBitStream.begin();

      BHuffmanEnc encoder;
      result = encoder.init(&outputBitStream, &codes);
      BVERIFY(result);

      Random rand2;
      rand2.setSeed(counter);

      const uint x = rand.iRand(1, 10000);

      printf("Unique Symbols: %u, Encoding %u symbols\n", numSymbols, x);

      for (uint i = 0; i < x; i++)
      {
         uint s = rand2.iRand(0, numSymbols);
         if (codes.getCodeSize(s))
            encoder.encode(s);
      }
      outputBitStream.end();

      bytes.seek(0);

      BInputBitStreamAdapter inputBitStream;
      inputBitStream.setStream(&bytes);
      result = inputBitStream.begin();
      BVERIFY(result);

      BHuffmanDec decoder;
      result = decoder.init(&inputBitStream);
      BVERIFY(result);

      rand2.setSeed(counter);
      for (uint i = 0; i < x; i++)
      {
         uint s = rand2.iRand(0, numSymbols);
         if (!codes.getCodeSize(s))
            continue;

         uint symbolIndex;
         result = decoder.getNextSymbol(symbolIndex);
         BVERIFY(result);
         BVERIFY(symbolIndex == s);
      }

      inputBitStream.end();

      counter++;
   }      
}

#endif
