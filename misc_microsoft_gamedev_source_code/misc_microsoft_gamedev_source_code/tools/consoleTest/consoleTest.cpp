//============================================================================
//
//  File: consoleTest.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xcorelib.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"
#include "file\win32FindFiles.h"
#include "file\win32FileUtils.h"
#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "utils\endianSwitch.h"
#include "resource\ecfUtils.h"
#include "resource\ecfHeaderIDs.h"
#include "resource\resourceTag.h"
#include "file\win32FileStream.h"
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"
#include "containers\hashMap.h"
#include "ecfArchiver.h"
#include "xml\xmlDocument.h"
#include "utils\spawn.h"
#include "timer.h"

#define PROGRAM_TITLE "ConsoleTest"

const uint cMinMatchSize = 3U;
const uint cMaxMatchSize = 258U;

#define mix3(a,b,c) \
{ \
   a -= b; a -= c; a ^= (c>>13); \
   b -= c; b -= a; b ^= (a<<8); \
   c -= a; c -= b; c ^= (b>>13); \
   a -= b; a -= c; a ^= (c>>12);  \
   b -= c; b -= a; b ^= (a<<16); \
   c -= a; c -= b; c ^= (b>>5); \
   a -= b; a -= c; a ^= (c>>3);  \
   b -= c; b -= a; b ^= (a<<10); \
   c -= a; c -= b; c ^= (b>>15); \
}   

class BDictionary
{
public:
   BDictionary()
   {
   }
   
   BDictionary(uint numBytes, const uchar* pBytes) : mBytes(numBytes, pBytes)
   {
   }
   
   void set(uint numBytes, const uchar* pBytes)
   {
      mBytes.resize(0);
      mBytes.pushBack(pBytes, numBytes);
   }
   
   const BByteArray& getBytes() const  { return mBytes; }
         BByteArray& getBytes()        { return mBytes; }
   
   uint getSize() const { return mBytes.getSize(); }
   
   uint operator[] (uint ofs) const { return mBytes[ofs]; } 
   
   const uchar* getPtr(uint ofs) const { return &mBytes[ofs]; }
      
   uint getMaxMatchSize(uint ofs) const { BDEBUG_ASSERT(ofs < mBytes.getSize()); return Math::Min(cMaxMatchSize, mBytes.getSize() - ofs); }

   void hash(BHash& hash, uint ofs, uint size) const
   {
      BDEBUG_ASSERT(getMaxMatchSize(ofs) >= size);
      
      if (size > 3)
      {
         hash.clear();
         hash.update(getPtr(ofs), size);
      }
      else
      {
         uint c0 = mBytes[ofs];
         uint c1 = mBytes[ofs + 1];
         uint c2 = mBytes[ofs + 2];
               
         DWORD a = 0x9E3779B9 + c0 + (c1 << 8U) + (c2 << 16U);
         DWORD b = 0x71E4923E;
         DWORD c = 0xA234CE97 + c2 + (c1 << 8U) + (c0 << 16U);
         
         mix3(a, b, c);
         
         hash[0] = a;
         hash[1] = b;
         hash[2] = c;
      }         
   }
         
   int comp(const uint l, const uchar* pR, uint rSize) const
   {
      BDEBUG_ASSERT(rSize <= cMaxMatchSize);
      
      const uchar* pL = &mBytes[l];
      const uint lSize = getMaxMatchSize(l);

      const uint bytesToCompare = Math::Min(lSize, rSize);
      const int compResult = memcmp(pL, pR, bytesToCompare);
      if (compResult)
         return compResult;
      
      if (lSize < rSize)
         return -1;
      else if (lSize > rSize)
         return 1;
      
      return 0;
   }
   
   int comp(const uint l, const uint r) const
   {
      const uchar* pR = &mBytes[r];
      const uint rSize = getMaxMatchSize(r);
      return comp(l, pR, rSize);
   }
   
   bool operator() (const uint l, const uint r) const
   {
      return comp(l, r) < 0;
   }
   
   uint match(const uint l, const uchar* pR, uint rSize) const
   {
      const uchar* pL = &mBytes[l];
      
      const uint bytesToCompare = Math::Min3(cMaxMatchSize, mBytes.getSize() - l, rSize);

      for (uint i = 0; i < bytesToCompare; i++)
         if (pL[i] != pR[i])
            return i;

      return bytesToCompare;
   }

   uint match(const uint l, const uint r, uint matchLenToBeat = 0) const
   {
      const uchar* pL = &mBytes[l];
      const uint lSize = mBytes.getSize() - l;
      
      const uchar* pR = &mBytes[r];
      const uint rSize = mBytes.getSize() - r;

      const uint bytesToCompare = Math::Min3(cMaxMatchSize, lSize, rSize);

      if (matchLenToBeat)
      {
         if (bytesToCompare < matchLenToBeat)
            return 0;
         
         if (matchLenToBeat >= 3)
         {
            if (pL[matchLenToBeat - 3] != pR[matchLenToBeat - 3])
               return 0;
         }
            
         if (matchLenToBeat >= 2)
         {
            if (pL[matchLenToBeat - 2] != pR[matchLenToBeat - 2])
               return 0;
         }
         
         if (pL[matchLenToBeat - 1] != pR[matchLenToBeat - 1])
            return 0;
      }            

      for (uint i = 0; i < bytesToCompare; i++)
         if (pL[i] != pR[i])
            return i;

      return bytesToCompare;
   }

private:   
   BByteArray mBytes;
};

class BSegment
{
public:
   BSegment()
   {
   }
   
   BSegment(uint numBytes, const uchar* pBytes)
   {
      set(numBytes, pBytes);
   }
   
   void set(uint numBytes, const uchar* pBytes)
   {
      mDictionary.set(numBytes, pBytes);
      
      initSuffixArray();
   }
         
   bool findLargestMatch(uint dictPos, uint minDictPos, uint& matchDictPos, uint& matchLen)
   {
      matchDictPos = 0;
      matchLen = 0;
      
      const uint maxMatchLen = mDictionary.getMaxMatchSize(dictPos);
      if (maxMatchLen < cMinMatchSize)
         return false;
            
      const uint sortPos = mInvSuffixArray[dictPos];
                  
      bool success = findLargestMatch(sortPos, minDictPos, dictPos, matchDictPos, matchLen);
      if ((!success) || (matchLen < cMinMatchSize))
         return false;
         
      return true;
   }
   
   uint getSize() const { return mDictionary.getSize(); }
   const BDictionary& getDictionary() const { return mDictionary; }
   
private:
   BDictionary    mDictionary;
   UIntArray      mSuffixArray;
   UIntArray      mInvSuffixArray;
   
   class BSortComparer
   {
      const BDictionary& mDictionary;
      
   public:
      BSortComparer(const BDictionary& dictionary) : mDictionary(dictionary) { }
      
      bool operator()(uint l, uint r) const
      {
         return mDictionary(l, r);
      }
   };
   
   void initSuffixArray()
   {
      mSuffixArray.resize(mDictionary.getSize());
      for (uint i = 0; i < mDictionary.getSize(); i++)
         mSuffixArray[i] = i;

      BTimer timer;
      timer.start();
      std::sort(mSuffixArray.begin(), mSuffixArray.end(), BSortComparer(mDictionary));
      gConsoleOutput.printf("Sort time: %f seconds\n", timer.getElapsedSeconds());
      
      for (uint i = 1; i < mDictionary.getSize(); i++)
      {
         int comp = mDictionary.comp(mSuffixArray[i - 1], mSuffixArray[i]);
         BVERIFY(comp <= 0);
      }
      
      mInvSuffixArray.resize(mSuffixArray.getSize());
      for (uint i = 0; i < mSuffixArray.getSize(); i++)
         mInvSuffixArray[mSuffixArray[i]] = i;
   }

   uint findInsertionPoint(const uchar* p, uint maxMatchLen)
   {   
      int l = 0;
      int r = mDictionary.getSize() - 1;
      int m = 0;
      int compResult = 0;
      while (r >= l)
      {
         m = (l + r) >> 1;

         compResult = mDictionary.comp(mSuffixArray[m], p, maxMatchLen);

         if (!compResult)
            break;
         else if (compResult > 0)
            r = m - 1;
         else
            l = m + 1;
      }

      uint result;
      if (compResult >= 0)
         result = m;
      else
         result = m + 1;   
         
      if (result > 0)
      {
         BDEBUG_ASSERT(mDictionary.comp(mSuffixArray[result - 1], p, maxMatchLen) <= 0);
      }
      
      BDEBUG_ASSERT(mDictionary.comp(mSuffixArray[result], p, maxMatchLen) >= 0);
      
      if (result < (mDictionary.getSize() - 1))
      {
         BDEBUG_ASSERT(mDictionary.comp(mSuffixArray[result + 1], p, maxMatchLen) >= 0);
      }

      return result;         
   }
   
   bool findLargestMatch(
      uint sortPos, 
      uint minDictPos, uint maxDictPos,
      uint& outMatchDictPos, uint& outMatchLen)      
   {
      const uint dictPos = mSuffixArray[sortPos];

      uint bestMatchLen = 0;
      int bestMatchDictPos = -1;
      uint bestMatchDist = UINT_MAX;

      int compSortPos = sortPos - 1;
      while (compSortPos >= 0)
      {
         const uint compDictPos = mSuffixArray[compSortPos];
         if ((compDictPos < minDictPos) || (compDictPos >= maxDictPos))
         {
            compSortPos--;
            continue;
         }

         const uint matchLen = mDictionary.match(dictPos, compDictPos, bestMatchLen);
         if (matchLen < cMinMatchSize)
            break;

         const uint compMatchDist = maxDictPos - compDictPos;

         BDEBUG_ASSERT((!bestMatchLen) || (matchLen <= bestMatchLen));

         if (matchLen < bestMatchLen)
            break;
         bestMatchLen = matchLen;
         if (compMatchDist < bestMatchDist)
         {
            bestMatchDictPos = compDictPos;
            bestMatchDist = compMatchDist;
         }

         compSortPos--;
      }

      compSortPos = sortPos + 1;
      while (compSortPos < (int)mDictionary.getSize())
      {
         const uint compDictPos = mSuffixArray[compSortPos];
         if ((compDictPos < minDictPos) || (compDictPos >= maxDictPos))
         {
            compSortPos++;
            continue;
         }

         const uint matchLen = mDictionary.match(dictPos, compDictPos, bestMatchLen);
         if (matchLen < cMinMatchSize)
            break;

         const uint compMatchDist = maxDictPos - compDictPos;

         if (matchLen < bestMatchLen)
            break;
         else if (matchLen > bestMatchLen)
         {
            bestMatchLen = matchLen;
            bestMatchDictPos = compDictPos;
            bestMatchDist = compMatchDist;
         }
         else if (compMatchDist < bestMatchDist)
         {
            bestMatchDictPos = compDictPos;
            bestMatchDist = compMatchDist;
         }
         
         compSortPos++;
      }

      if (!bestMatchLen)
         return false;

      outMatchDictPos = bestMatchDictPos;
      outMatchLen = bestMatchLen;

      return true;
   }
};

template<uint cMaxSymbols>
class BHuffmanCodeBuilder
{
public:
   BHuffmanCodeBuilder()
   {
      clear();
   }
   
   void clear()
   {
      Utils::ClearObj(*this);
   }
   
   void init(uint numSymbols, uint maxCodeSize)
   {
      BDEBUG_ASSERT(numSymbols <= cMaxSymbols);
      
      clear();
      
      mNumSymbols = numSymbols;
      mMaxCodeSize = maxCodeSize;
   }
   
   void resetSymbolFreq()
   {
      Utils::ClearObj(mFrequencies);
   }
   
   uint getSymbolFreq(uint symbolIndex) const { BDEBUG_ASSERT(symbolIndex < mNumSymbols); return mFrequencies[symbolIndex]; }
   void addSymbolFreq(uint symbolIndex, int freq) { BDEBUG_ASSERT(symbolIndex < mNumSymbols); mFrequencies[symbolIndex] += freq; }
   void incSymbolFreq(uint symbolIndex) { addSymbolFreq(symbolIndex, 1U); }
   
   void createCodes()
   {
      BDEBUG_ASSERT(mNumSymbols);
      
      int frequencies[cMaxSymbols];
      memcpy(frequencies, mFrequencies, sizeof(mFrequencies));
      
      code_sizes(mNumSymbols, frequencies, mCodeSizes);
      sort_code_sizes(mNumSymbols, mCodeSizes);
      fix_code_sizes(mMaxCodeSize);
      make_codes(mNumSymbols, mCodeSizes, mMaxCodeSize, mCodes);
   }
   
   uint getNumSymbols() const { return mNumSymbols; }
   uint getCodeSize(uint symbolIndex) const { return mCodeSizes[symbolIndex]; }
   uint getCode(uint symbolIndex) const { return mCodes[symbolIndex]; }
   
   const int* getCodeSizeArray() const { return mCodeSizes; }
   const uint* getCodesArray() const { return mCodes; }
   const int getFrequenciesArray() const { return mFrequencies; }

private:
   uint     mNumSymbols;
   uint     mMaxCodeSize;
   int      mCodeSizes[cMaxSymbols];
   uint     mCodes[cMaxSymbols];
   int      mFrequencies[cMaxSymbols];
   
   int      mHeap[cMaxSymbols + 1];
   int      mOthers[cMaxSymbols];
   int      mCodeList[cMaxSymbols];
   int      mCodeListLen;
   int      mNumCodes[33];
   int      mNextCode[33];
   int      mNewCodeSizes[cMaxSymbols];
   
   static void int_set(int *dst, int dat, int len)
   {
      while (len)
      {
         *dst++ = dat;
         len--;
      }
   }

   static void down_heap(int *pHeap, int *sym_freq, int heap_len, int i)
   {
      int j, k;

      k = pHeap[i];

      while ((j = 2 * i) <= heap_len)
      {
         if (j < heap_len)
         {
            if (sym_freq[pHeap[j]] > sym_freq[pHeap[j + 1]])
               j++;
            else if (sym_freq[pHeap[j]] == sym_freq[pHeap[j + 1]])
            {
               if (pHeap[j] < pHeap[j + 1])
                  j++;
            }
         }

         if (sym_freq[k] < sym_freq[pHeap[j]])
            break;
         else if (sym_freq[k] == sym_freq[pHeap[j]])
         {
            if (k > pHeap[j])
               break;
         }

         pHeap[i] = pHeap[j]; i = j;
      }

      pHeap[i] = k;
   }
   
   void code_sizes(int num_symbols, int *sym_freq, int *code_sizes)
   {
      int i, j, k, heap_len;

      for (i = 0; i < num_symbols; i++)
         mOthers[i] = -1;

      for (i = 0; i < num_symbols; i++)
         code_sizes[i] = 0;

      heap_len = 1;

      for (i = 0; i < num_symbols; i++)
         if (sym_freq[i] != 0)
            mHeap[heap_len++] = i;

      heap_len--;

      if (heap_len <= 1)
      {
         if (heap_len == 0)
            return;

         code_sizes[mHeap[1]] = 1;

         return;
      }

      j = (heap_len >> 1);

      while (j != 0)
         down_heap(mHeap, sym_freq, heap_len, j--);

      do
      {
         i = mHeap[1];

         mHeap[1] = mHeap[heap_len--];

         down_heap(mHeap, sym_freq, heap_len, 1);

         j = mHeap[1];

         sym_freq[j] += sym_freq[i];

         down_heap(mHeap, sym_freq, heap_len, 1);

         do { code_sizes[k = j]++; } while ((j = mOthers[j]) != -1);

         mOthers[k] = i;

         do { code_sizes[i]++; } while ((i = mOthers[i]) != -1);

      } while (heap_len != 1);
   }
   
   void sort_code_sizes(int num_symbols, int *code_sizes)
   {
      int i, j;

      mCodeListLen = 0;

      int_set(mNumCodes, 0, 33);

      for (i = 0; i < num_symbols; i++)
         mNumCodes[code_sizes[i]]++;

      for (i = 1, j = 0; i <= 32; i++)
      {
         mNextCode[i] = j;
         j += mNumCodes[i];
      }

      for (i = 0; i < num_symbols; i++)
      {
         if ((j = code_sizes[i]) != 0)
         {
            mCodeList[mNextCode[j]++] = i;
            mCodeListLen++;
         }
      }
   }
   
   void fix_code_sizes(int max_code_size)
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
   
   void make_codes(
      int num_symbols, int *code_sizes,
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
         code_sizes[mCodeList[i]] = mNewCodeSizes[i];

      for (i = 0; i < num_symbols; i++)
      {
         if (code_sizes[i])
         {
            j = mNextCode[code_sizes[i]]++;

            for (l = code_sizes[i], k = 0; l > 0; l--)
            {
               k = (k << 1) | (j & 1);
               j >>= 1;
            }

            codes[i] = k;
         }
      }
   }
};

class BOutputBitStream
{
public:
   BOutputBitStream(BStream* pOutputStream = NULL)
   {
      init(pOutputStream);
   }
   
   void init(BStream* pOutputStream)
   {
      mpOutputStream = pOutputStream;
      mTotalBytesWritten = 0;
      mBitBuf = 0;
      mBitBufSize = 0;
   }
   
   void writeBit(uint bit)
   {
      BDEBUG_ASSERT(bit < 2);

      mBitBuf |= (static_cast<uint64>(bit) << mBitBufSize);
      mBitBufSize++;
      if (mBitBufSize >= 32)
         flushBitBuf();
   }
   
   void writeBits(uint code, uint codeSize)
   {
      BDEBUG_ASSERT((codeSize > 0) && (codeSize <= 32));
      BDEBUG_ASSERT(code < (1U << codeSize));

      mBitBuf |= (static_cast<uint64>(code) << mBitBufSize);
      mBitBufSize += codeSize;
      if (mBitBufSize >= 32)
         flushBitBuf();
   }
   
   void flush()
   {
      flushBitBuf(true);
   }
   
   uint getTotalBytesWritten() const { return mTotalBytesWritten; }

private:
   uint     mTotalBytesWritten;
   uint64   mBitBuf;
   uint     mBitBufSize;
   BStream* mpOutputStream;
   
   void flushBitBuf(bool force = false)
   {
      while (mBitBufSize >= 8)
      {
         if (mpOutputStream)
            mpOutputStream->putch(static_cast<uchar>(mBitBuf & 0xFF));
         
         mTotalBytesWritten++;
         
         mBitBuf >>= 8;
         mBitBufSize -= 8;
      }

      if ((force) && (mBitBufSize))
      {
         if (mpOutputStream)
            mpOutputStream->putch(static_cast<uchar>(mBitBuf & 0xFF));
         
         mTotalBytesWritten++;
         
         mBitBuf = 0;
         mBitBufSize = 0;
      }
   }
   
};

template<uint cMaxSymbols>
class BCodeSizeCompressor
{
   enum { cNumCodeSizeSymbols = 19U };

public:
   BCodeSizeCompressor()
   {
      clear();
   }
   
   void clear()
   {
      mCodeBuilder.clear();
      Utils::ClearObj(mOutputSymbols);
      mpOutputSymbolsEnd = NULL;
   }
   
   void init(const int* pSymbolCodeSizes, int numSymbols)
   {
      clear();
      init_compress_code_sizes(pSymbolCodeSizes, numSymbols);
   }
   
   void code(BOutputBitStream& outputStream)
   {
      BDEBUG_ASSERT(mpOutputSymbolsEnd);
      compress_code_sizes(outputStream);
   }
   
private:
   BHuffmanCodeBuilder<cNumCodeSizeSymbols> mCodeBuilder;
   int mOutputSymbols[cMaxSymbols];
   int *mpOutputSymbolsEnd;
   
   int* repeat_last(int *dst, int size, int run_len)
   {
      if (run_len < 3)
      {
         mCodeBuilder.addSymbolFreq(size, run_len);

         while (run_len--)
            *dst++ = size;
      }
      else
      {
         mCodeBuilder.incSymbolFreq(16);

         *dst++ = 16;
         *dst++ = run_len - 3;
      }

      return (dst);
   }
   
   int* repeat_zero(int *dst, int run_len)
   {
      if (run_len < 3)
      {
         mCodeBuilder.addSymbolFreq(0, run_len);

         while (run_len--)
            *dst++ = 0;
      }
      else if (run_len <= 10)
      {
         mCodeBuilder.incSymbolFreq(17);

         *dst++ = 17;
         *dst++ = run_len - 3;
      }
      else
      {
         mCodeBuilder.incSymbolFreq(18);

         *dst++ = 18;
         *dst++ = run_len - 11;
      }

      return (dst);
   }
   
   void init_compress_code_sizes(const int* pSymbolCodeSizes, int numSymbols)
   {
      const int *src;
      int *dst, codes_left;
      int size, last_size = 0xFF;
      int run_len_z = 0, run_len_nz = 0;

      mCodeBuilder.init(cNumCodeSizeSymbols, 7);
      
      src = pSymbolCodeSizes;
      dst = mOutputSymbols;

      for (codes_left = numSymbols; codes_left > 0; codes_left--)
      {
         if ((size = *src++) == 0)
         {
            if (run_len_nz)
            {
               dst = repeat_last(dst, last_size, run_len_nz);
               run_len_nz = 0;
            }

            if (++run_len_z == 138)
            {
               dst = repeat_zero(dst, run_len_z);
               run_len_z = 0;
            }
         }
         else
         {
            if (run_len_z)
            {
               dst = repeat_zero(dst, run_len_z);
               run_len_z = 0;
            }

            if (size != last_size)
            {
               if (run_len_nz)
               {
                  dst = repeat_last(dst, last_size, run_len_nz);
                  run_len_nz = 0;
               }

               mCodeBuilder.incSymbolFreq(size);

               *dst++ = size;
            }
            else if (++run_len_nz == 6)
            {
               dst = repeat_last(dst, last_size, run_len_nz);
               run_len_nz = 0;
            }
         }

         last_size = size;
      }

      if (run_len_nz)
         dst = repeat_last(dst, last_size, run_len_nz);
      else if (run_len_z)
         dst = repeat_zero(dst, run_len_z);

      mpOutputSymbolsEnd = dst;
      
      BDEBUG_ASSERT((mpOutputSymbolsEnd - mOutputSymbols) <= cMaxSymbols);

      mCodeBuilder.createCodes();
   }
   
   void compress_code_sizes(BOutputBitStream& outputStream)
   {
      static const uchar bitLengthOrder[] = { 16, 17, 18, 0, 8,  7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
      
      int i, bit_lengths, *src;
      
      for (bit_lengths = 18; bit_lengths >= 0; bit_lengths--)
         if (mCodeBuilder.getCodeSize(bitLengthOrder[bit_lengths]))
            break;

      bit_lengths = max(4, (bit_lengths + 1));

      outputStream.writeBits(bit_lengths - 4, 4);

      for (i = 0; i < bit_lengths; i++)
         outputStream.writeBits(mCodeBuilder.getCodeSize(bitLengthOrder[i]), 3);

      src = mOutputSymbols;

      while (src < mpOutputSymbolsEnd)
      {
         i = *src++;

         outputStream.writeBits(mCodeBuilder.getCode(i), mCodeBuilder.getCodeSize(i));

         if (i == 16)
            outputStream.writeBits(*src++, 2);
         else if (i == 17)
            outputStream.writeBits(*src++, 3);
         else if (i == 18)
            outputStream.writeBits(*src++, 7);
      }
   }

};


static bool test(BCommandLineParser::BStringArray args)
{
   BHuffmanCodeBuilder<256> huffEncode;
   huffEncode.init(10, 16);
   huffEncode.addSymbolFreq(0, 100);
   huffEncode.addSymbolFreq(1, 20);
   huffEncode.addSymbolFreq(2, 20);
   huffEncode.addSymbolFreq(3, 1);
   huffEncode.createCodes();
   
   BDynamicStream dynStream;
   BOutputBitStream bitStream(&dynStream);
   
   BCodeSizeCompressor<256> codeSizeCompressor;
   codeSizeCompressor.init(huffEncode.getCodeSizeArray(), huffEncode.getNumSymbols());
   codeSizeCompressor.code(bitStream);
   
   
   
   if (args.getSize() < 2)
      return false;
      
   BByteArray fileData;
   if (!BWin32FileUtils::readFileData(args[1], fileData))
      return false;
            
   if (fileData.isEmpty())             
      return false;

   //fileData.resize(Math::Min(fileData.getSize(), 65536U));
   
   gConsoleOutput.printf("Sorting\n");
   BSegment segment(fileData.getSize(), fileData.getPtr());

   gConsoleOutput.printf("Matching\n");
   
   BTimer timer;
   timer.start();
   
   UIntArray bestMatchPos(segment.getSize());
   UIntArray bestMatchLen(segment.getSize());
   
   uint64 sum = 0;
   
   for (uint pos = 0; pos < segment.getSize(); pos++)
   {
      uint matchDictPos;
      uint matchLen;
      const bool foundMatch = segment.findLargestMatch(pos, 0, matchDictPos, matchLen);
      
      if (!foundMatch)
      {
         bestMatchLen[pos] = 1;
         
         sum += 1;
      }
      else
      {
         BDEBUG_ASSERT((matchLen >= cMinMatchSize) && (matchLen <= cMaxMatchSize));
         BDEBUG_ASSERT(matchDictPos < pos);
         
         bestMatchPos[pos] = matchDictPos;
         bestMatchLen[pos] = matchLen;
         sum += matchDictPos;
         sum += matchLen;
         
         BDEBUG_ASSERT(segment.getDictionary().match(pos, matchDictPos) == matchLen);
      }
   }
      
   gConsoleOutput.printf("Match time: %f seconds\n", timer.getElapsedSeconds());
   gConsoleOutput.printf("%I64u\n", sum);
   
   
   return true;
}



static int mainInternal(int argC, const char* argV[])
{
   XCoreCreate();

   BConsoleAppHelper::setup();

   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argC, argV))
   {
      BConsoleAppHelper::deinit();

      XCoreRelease();
      return EXIT_FAILURE;
   }

   gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s\n", __DATE__, __TIME__);

   bool success = test(args);

   if (success)
      gConsoleOutput.printf(PROGRAM_TITLE ": Done\n");
   else
      gConsoleOutput.error(PROGRAM_TITLE ": Failed\n");
   
   XCoreRelease();
  
   if (!success)
      return EXIT_FAILURE;

   return EXIT_SUCCESS;
}

int main(int argC, const char* argV[])
{
   int status;

#ifndef BUILD_DEBUG   
   __try
#endif   
   {
      status = mainInternal(argC, argV);
   }
#ifndef BUILD_DEBUG   
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      fprintf(stderr, PROGRAM_TITLE ": Unhandled exception!");
      return 1;
   }
#endif   

   return status;
}










