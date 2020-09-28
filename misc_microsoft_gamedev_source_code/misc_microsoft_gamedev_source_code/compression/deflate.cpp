//============================================================================
// deflate.cpp
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#include "compression.h"
#include "xcore.h"
#include "deflate.h"

// #define USE_SIMPLE_COMPARATOR
#define DEFL_NIL           (0xFFFF)

//-------------------------------------------------------------------------------------------------------------
#include "deflateTables.inc"
//-------------------------------------------------------------------------------------------------------------
static inline uint read_word(const void* pSrc)
{
#ifdef _XBOX
   const uchar* pByteSrc = (uchar*)pSrc;
   return pByteSrc[0] | (pByteSrc[1] << 8);
#else
   return *static_cast<const ushort*>(pSrc);
#endif   
}
//-------------------------------------------------------------------------------------------------------------
static inline void write_word(void* pDst, uint w)
{
#ifdef _XBOX
   uchar* pByteDst = (uchar*)pDst;
   pByteDst[0] = (uchar)w;
   pByteDst[1] = (uchar)(w>>8);
#else
   *static_cast<ushort*>(pDst) = static_cast<ushort>(w);
#endif   
}
//-------------------------------------------------------------------------------------------------------------
static void int_set(int *dst, int dat, int len)
{
   while (len)
   {
      *dst++ = dat;
      len--;
   }
}
//-------------------------------------------------------------------------------------------------------------
static void int_move(int *dst, const int *src, int len)
{
   Utils::FastMemCpy(dst, src, len * sizeof(int));
}
//-------------------------------------------------------------------------------------------------------------
static void mem_copy(uchar *dst, const uchar *src, int len)
{
   Utils::FastMemCpy(dst, src, len);
}
//-------------------------------------------------------------------------------------------------------------
static void mem_set(uchar *dst, uchar c, int len)
{
   Utils::FastMemSet(dst, c, len);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::return_buf_sizes(void)
{
   *mpInBufSizeOfs  = mInBufSize  - mInBufLeft;
   *mpOutBufSizeOfs = mOutBufSize - mOutBufLeft;
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::send_message(int status)
{
   return_buf_sizes();

   mSSD.setReturnStatus(status);

   mSSD.ret();
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::refill_buf(void)
{
   send_message(DEFL_STATUS_OKAY);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::huff_down_heap(int *mHeap, int *sym_freq, int heap_len, int i)
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
//-------------------------------------------------------------------------------------------------------------
void BDeflate::huff_code_sizes(int num_symbols, int *sym_freq, int *code_sizes)
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
      huff_down_heap(mHeap, sym_freq, heap_len, j--);

   do
   {
      i = mHeap[1];

      mHeap[1] = mHeap[heap_len--];

      huff_down_heap(mHeap, sym_freq, heap_len, 1);

      j = mHeap[1];

      sym_freq[j] += sym_freq[i];

      huff_down_heap(mHeap, sym_freq, heap_len, 1);

      do { code_sizes[k = j]++; } while ((j = mOthers[j]) != -1);

      mOthers[k] = i;

      do { code_sizes[i]++; } while ((i = mOthers[i]) != -1);

   } while (heap_len != 1);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::huff_sort_code_sizes(int num_symbols, int *code_sizes)
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
//-------------------------------------------------------------------------------------------------------------
void BDeflate::huff_fix_code_sizes(int max_code_size)
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
//-------------------------------------------------------------------------------------------------------------
void BDeflate::huff_make_codes(
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
//-------------------------------------------------------------------------------------------------------------
#define PUT_BYTE(c) { while (--mOutBufLeft < 0) { mOutBufLeft++; refill_buf(); } *mpOutBufCurOfs++ = (c); }
//-------------------------------------------------------------------------------------------------------------
void BDeflate::put_bits(uint bits, int len)
{
   if (mBitBufTotalFlag)
      goto mBitBufTotal;

   mBitBuf |= (bits << mBitBufLen);

   if ((mBitBufLen += len) < 8)
      return;

   if (mBitBufLen >= 16)
      goto flush_word;

   if (--mOutBufLeft < 0)
      goto flush_byte;

   *mpOutBufCurOfs++ = static_cast<uchar>(mBitBuf & 0xFF);

   mBitBuf >>= 8;
   mBitBufLen -= 8;

   return;

flush_byte:

   mOutBufLeft++;

   PUT_BYTE(static_cast<uchar>(mBitBuf & 0xFF))

   mBitBuf >>= 8;
   mBitBufLen -= 8;

   return;

flush_word:
   
   PUT_BYTE(static_cast<uchar>(mBitBuf & 0xFF));
   PUT_BYTE(static_cast<uchar>((mBitBuf >> 8) & 0xFF));

   mBitBuf >>= 16;
   mBitBufLen -= 16;

   return;

mBitBufTotal:

   mBitBufTotal += len;

   return;
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::flush_bits(void)
{
   put_bits(0, 7);

   mBitBufLen = 0;
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::delete_data(uint dict_pos)
{
   register uint i, j;
   uint k;

   k = dict_pos + DEFL_SECTOR_SIZE;

   for (i = dict_pos; i < k; i++)
   {
      if ((j = mLast[i]) & DEFL_HASH_FLAG_1)
      {
         if (j != DEFL_NIL)
            mHash[j & DEFL_HASH_FLAG_2] = DEFL_NIL;
      }
      else
         mNext[j] = DEFL_NIL;
   }
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::hash_data(uint dict_pos, int bytes_to_do)
{
   uint i, j, k;

   for (i = max(0, bytes_to_do - DEFL_THRESHOLD); i < (uint)bytes_to_do; i++)
      mNext[dict_pos + i] = mLast[dict_pos + i] = DEFL_NIL;

   if (bytes_to_do <= DEFL_THRESHOLD)
      return;

   j = (((uint)mDict[dict_pos]) << DEFL_SHIFT_BITS) ^ mDict[dict_pos + 1];

   k = dict_pos + bytes_to_do - DEFL_THRESHOLD;

   for (i = dict_pos; i < k; i++)
   {
      mLast[i] = static_cast<ushort>((j = (((j << DEFL_SHIFT_BITS) & (DEFL_HASH_SIZE - 1)) ^ mDict[i + DEFL_THRESHOLD])) | DEFL_HASH_FLAG_1);

      if ((mNext[i] = mHash[j]) != DEFL_NIL)
         mLast[mNext[i]] = static_cast<ushort>(i);

      mHash[j] = static_cast<ushort>(i);
   }
}
//-------------------------------------------------------------------------------------------------------------
#ifdef USE_SIMPLE_COMPARATOR
//-------------------------------------------------------------------------------------------------------------
void BDeflate::find_match(uint dict_pos)
{
   int probe_len, compares_left = mMaxCompares;
   uint probe_pos = dict_pos & (DEFL_DICT_SIZE - 1);
   uchar l0, l1, *p, *q, *r = mDict + dict_pos;

   l0 = mDict[dict_pos + mMatchLen];
   l1 = mDict[dict_pos + mMatchLen - 1];

   for ( ; ; )
   {
      for ( ; ; )
      {
         if (--compares_left == 0)
            return;

         if ((probe_pos = mNext[probe_pos]) == DEFL_NIL)
            return;

         if (mDict[probe_pos + mMatchLen] == l0)
            break;
      }

      if (mDict[probe_pos + mMatchLen - 1] != l1)
         continue;

      p = r;
      q = mDict + probe_pos;

      for (probe_len = 0; probe_len < DEFL_MAX_MATCH; probe_len++)
         if (*p++ != *q++)
            break;

      if (probe_len > mMatchLen)
      {
         mMatchPos = probe_pos;

         if ((mMatchLen = probe_len) == DEFL_MAX_MATCH)
            return;

         l0 = mDict[dict_pos + mMatchLen];
         l1 = mDict[dict_pos + mMatchLen - 1];
      }
   }
}
//-------------------------------------------------------------------------------------------------------------
#else
//-------------------------------------------------------------------------------------------------------------
void BDeflate::find_match(uint dict_pos)
{
   int probe_len, compares_left = mMaxCompares;
   uchar *s;
   register uint probe_pos = dict_pos & (DEFL_DICT_SIZE - 1);
   register ushort *p, *q;
   register uint l;
   uint m;
   ushort *r = (ushort *)&mDict[dict_pos];

   l = read_word(&mDict[dict_pos + mMatchLen - 1]);
   m = read_word(r);
   s = &mDict[mMatchLen - 1];

   for ( ; ; )
   {
      for ( ; ; )
      {
         if (compares_left <= 0)
            return;

         compares_left--;
         if ((probe_pos = mNext[probe_pos]) == DEFL_NIL) return;
         if (read_word(&s[probe_pos]) == l) break;

         compares_left--;
         if ((probe_pos = mNext[probe_pos]) == DEFL_NIL) return;
         if (read_word(&s[probe_pos]) == l) break;

         compares_left--;
         if ((probe_pos = mNext[probe_pos]) == DEFL_NIL) return;
         if (read_word(&s[probe_pos]) == l) break;

         compares_left--;
         if ((probe_pos = mNext[probe_pos]) == DEFL_NIL) return;
         if (read_word(&s[probe_pos]) == l) break;
      }

      if (read_word(&mDict[probe_pos]) != m)
         continue;

      p = r;
      q = (ushort *)(mDict + probe_pos);

      probe_len = 32;

      do
      {
      } while ( (read_word(++p) == read_word(++q)) &&
               (read_word(++p) == read_word(++q)) &&
               (read_word(++p) == read_word(++q)) &&
               (read_word(++p) == read_word(++q)) &&
               (--probe_len > 0) );

      if (probe_len == 0)
         goto max_match;

      probe_len = ((p - r) * 2) + (read_word(p) == read_word(q));

      if (probe_len > mMatchLen)
      {
         mMatchPos = probe_pos;
         mMatchLen = probe_len;

         l = read_word(&mDict[dict_pos + mMatchLen - 1]);
         s = &mDict[mMatchLen - 1];
      }
   }

   return;

   max_match:

   mMatchPos = probe_pos;
   mMatchLen = DEFL_MAX_MATCH;

   return;
}
//-------------------------------------------------------------------------------------------------------------
#endif
//-------------------------------------------------------------------------------------------------------------
void BDeflate::code_block(void)
{
   int tokens_left;
   uchar *token_ptr = mTokenBuf;
   int flag_left = 0;
   uint flag = 0, *flag_buf_ptr = mFlagBuf;

   for (tokens_left = mTokenBufLen; tokens_left > 0; tokens_left--)
   {
      if (!flag_left)
      {
         flag = *flag_buf_ptr++;
         flag_left = 32;
      }

      if (flag & 0x80000000)
      {
         int mMatchLen = *token_ptr;
         uint match_dist = read_word(token_ptr + 1) - 1;

         put_bits(mCode1[gLenCode[mMatchLen]], mSize1[gLenCode[mMatchLen]]);
         put_bits(mMatchLen & gLenMask[mMatchLen], gLenExtra[mMatchLen]);

         if (match_dist < 512)
         {
            put_bits(mCode2[gDistLoCode[match_dist]], mSize2[gDistLoCode[match_dist]]);
            put_bits(match_dist & gDistLoMask[match_dist], gDistLoExtra[match_dist]);
         }
         else
         {
            uint match_dist_hi = match_dist >> 8;

            put_bits(mCode2[gDistHiCode[match_dist_hi]], mSize2[gDistHiCode[match_dist_hi]]);
            put_bits(match_dist & gDistHiMask[match_dist_hi], gDistHiExtra[match_dist_hi]);
         }

         token_ptr += 3;
      }
      else
      {
         uchar c = *token_ptr++;

         put_bits(mCode1[c], mSize1[c]);
      }

      flag <<= 1;
      flag_left--;
   }

   put_bits(mCode1[256], mSize1[256]);
   }
//-------------------------------------------------------------------------------------------------------------
void BDeflate::init_static_block(void)
{
   int_set(mSize1 + 0x00,  8, 0x90);
   int_set(mSize1 + 0x90,  9, 0x70);
   int_set(mSize1 + 0x100, 7, 0x18);
   int_set(mSize1 + 0x118, 8, 0x8);

   huff_sort_code_sizes(DEFL_NUM_SYMBOLS_1, mSize1);
   huff_make_codes(DEFL_NUM_SYMBOLS_1, mSize1, 15, mCode1);

   int_set(mSize2, 5, 0x20);

   huff_sort_code_sizes(DEFL_NUM_SYMBOLS_2, mSize2);
   huff_make_codes(DEFL_NUM_SYMBOLS_2, mSize2, 15, mCode2);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::send_static_block(void)
{
   put_bits(1, 2);
}
//-------------------------------------------------------------------------------------------------------------
int * BDeflate::repeat_last(int *dst, int size, int run_len)
{
   if (run_len < 3)
   {
      mFreq3[size] += run_len;

      while (run_len--)
         *dst++ = size;
   }
   else
   {
      mFreq3[16]++;

      *dst++ = 16;
      *dst++ = run_len - 3;
   }

   return (dst);
}
//-------------------------------------------------------------------------------------------------------------
int * BDeflate::repeat_zero(int *dst, int run_len)
{
   if (run_len < 3)
   {
      mFreq3[0] += run_len;

      while (run_len--)
         *dst++ = 0;
   }
   else if (run_len <= 10)
   {
      mFreq3[17]++;

      *dst++ = 17;
      *dst++ = run_len - 3;
   }
   else
   {
      mFreq3[18]++;

      *dst++ = 18;
      *dst++ = run_len - 11;
   }

   return (dst);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::init_compress_code_sizes(void)
{
   int *src, *dst, codes_left;
   int size, last_size = 0xFF;
   int run_len_z = 0, run_len_nz = 0;

   int_set(mFreq3, 0, DEFL_NUM_SYMBOLS_3);

   for (mUsedLitCodes = 285; mUsedLitCodes >= 0; mUsedLitCodes--)
      if (mSize1[mUsedLitCodes])
         break;

   mUsedLitCodes = max(257, (mUsedLitCodes + 1));

   for (mUsedDistCodes = 29; mUsedDistCodes >= 0; mUsedDistCodes--)
      if (mSize2[mUsedDistCodes])
         break;

   mUsedDistCodes = max(1, (mUsedDistCodes + 1));

   int_move(mBundledSizes, mSize1, mUsedLitCodes);
   int_move(mBundledSizes + mUsedLitCodes, mSize2, mUsedDistCodes);

   src = mBundledSizes;
   dst = mCodeSizes;

   for (codes_left = mUsedLitCodes + mUsedDistCodes; codes_left > 0; codes_left--)
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

            mFreq3[size]++;

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

   mpCodedSizesEnd = dst;

   huff_code_sizes(DEFL_NUM_SYMBOLS_3, mFreq3, mSize3);
   huff_sort_code_sizes(DEFL_NUM_SYMBOLS_3, mSize3);
   huff_fix_code_sizes(7);
   huff_make_codes(DEFL_NUM_SYMBOLS_3, mSize3, 7, mCode3);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::compress_code_sizes(void)
{
   int i, bit_lengths, *src;

   put_bits(mUsedLitCodes - 257, 5);
   put_bits(mUsedDistCodes - 1, 5);

   for (bit_lengths = 18; bit_lengths >= 0; bit_lengths--)
      if (mSize3[gBitLengthOrder[bit_lengths]])
         break;

   bit_lengths = max(4, (bit_lengths + 1));

   put_bits(bit_lengths - 4, 4);

   for (i = 0; i < bit_lengths; i++)
      put_bits(mSize3[gBitLengthOrder[i]], 3);

   src = mCodeSizes;

   while (src < mpCodedSizesEnd)
   {
      i = *src++;

      put_bits(mCode3[i], mSize3[i]);

      if (i == 16)
         put_bits(*src++, 2);
      else if (i == 17)
         put_bits(*src++, 3);
      else if (i == 18)
         put_bits(*src++, 7);
   }
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::init_dynamic_block(void)
{
   int tokens_left;
   uchar *token_ptr = mTokenBuf;
   int flag_left = 0;
   uint flag = 0, *flag_buf_ptr = mFlagBuf;

   int_set(mFreq1, 0, DEFL_NUM_SYMBOLS_1);
   int_set(mFreq2, 0, DEFL_NUM_SYMBOLS_2);

   for (tokens_left = mTokenBufLen; tokens_left > 0; tokens_left--)
   {
      if (!flag_left)
      {
         flag = *flag_buf_ptr++;
         flag_left = 32;
      }

      if (flag & 0x80000000)
      {
         uint match_dist = read_word(token_ptr + 1) - 1;

         mFreq1[gLenCode[*token_ptr]]++;

         if (match_dist < 512)
            mFreq2[gDistLoCode[match_dist]]++;
         else
            mFreq2[gDistHiCode[match_dist >> 8]]++;

         token_ptr += 3;
      }
      else
         mFreq1[*token_ptr++]++;

      flag <<= 1;
      flag_left--;
   }

   mFreq1[256]++;

   huff_code_sizes(DEFL_NUM_SYMBOLS_1, mFreq1, mSize1);
   huff_sort_code_sizes(DEFL_NUM_SYMBOLS_1, mSize1);
   huff_fix_code_sizes(15);
   huff_make_codes(DEFL_NUM_SYMBOLS_1, mSize1, 15, mCode1);

   huff_code_sizes(DEFL_NUM_SYMBOLS_2, mFreq2, mSize2);
   huff_sort_code_sizes(DEFL_NUM_SYMBOLS_2, mSize2);
   huff_fix_code_sizes(15);
   huff_make_codes(DEFL_NUM_SYMBOLS_2, mSize2, 15, mCode2);

   init_compress_code_sizes();
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::send_dynamic_block(void)
{
   put_bits(2, 2);

   compress_code_sizes();
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::send_raw_block(void)
{
   int len, src;

   put_bits(0, 2);

   flush_bits();

   PUT_BYTE(static_cast<uchar>(mTokenBufBytes & 0xFF))
   PUT_BYTE(static_cast<uchar>(mTokenBufBytes >> 8))

   PUT_BYTE(static_cast<uchar>((~mTokenBufBytes) & 0xFF))
   PUT_BYTE(static_cast<uchar>((~mTokenBufBytes) >> 8))

   for (src = mTokenBufStart, len = mTokenBufBytes; len > 0; len--)
   {
      PUT_BYTE(mDict[src++])

      src &= (DEFL_DICT_SIZE - 1);
   }
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::code_token_buf(int last_block_flag)
{
   mTokenBufEnd = mSearchOffset;

   if (mTokenBufLen)
   {
      put_bits(0, 1);

      if (mStategy == DEFL_STATIC_BLOCKS)
      {
         init_static_block();
         send_static_block();
         code_block();
      }
      else if (mStategy == DEFL_DYNAMIC_BLOCKS)
      {
         init_dynamic_block();
         send_dynamic_block();
         code_block();
      }
      else if (mTokenBufLen < 128)
      {
         uint static_bits, dynamic_bits, raw_bits;

         mBitBufTotalFlag = TRUE;

         mBitBufTotal = 0;
         init_static_block();
         send_static_block();
         code_block();
         static_bits = mBitBufTotal;

         mBitBufTotal = 0;
         init_dynamic_block();
         send_dynamic_block();
         code_block();
         dynamic_bits = mBitBufTotal;

         mBitBufTotalFlag = FALSE;

         raw_bits = 2 + 32 + (mTokenBufBytes << 3);

         if ((mBitBufLen + 2) & 7)
         raw_bits += (8 - ((mBitBufLen + 2) & 7));

         if ((raw_bits < static_bits) &&  (raw_bits < dynamic_bits))
            send_raw_block();
         else if (static_bits < dynamic_bits)
         {
            init_static_block();
            send_static_block();
            code_block();
         }
         else
         {
            send_dynamic_block();
            code_block();
         }
      }
      // 01/02/02 check to see if the block had many matches, if so don't try a raw block
		else if (mTokenBufBytes >= (DEFL_MAX_TOKENS + (DEFL_MAX_TOKENS / 5)))
      {
         init_dynamic_block();
         send_dynamic_block();
         code_block();
      }
      else
      {
         uint dynamic_bits, raw_bits;

         mBitBufTotalFlag = TRUE;

         mBitBufTotal = 0;
         init_dynamic_block();
         send_dynamic_block();
         code_block();
         dynamic_bits = mBitBufTotal;

         mBitBufTotalFlag = FALSE;

         raw_bits = 2 + 32 + (mTokenBufBytes << 3);

         if ((mBitBufLen + 2) & 7)
            raw_bits += (8 - ((mBitBufLen + 2) & 7));

         if (raw_bits < dynamic_bits)
            send_raw_block();
         else
         {
            send_dynamic_block();
            code_block();
         }
      }
   }

   mpFlagBufOfs  = mFlagBuf;
   mFlagBufLeft = 32;
   mpTokenBufOfs = mTokenBuf;
   mTokenBufLen = 0;

   mTokenBufBytes = 0;
   mTokenBufStart = mTokenBufEnd;

   if (last_block_flag)
   {
      put_bits(1, 1);

      init_static_block();
      send_static_block();
      code_block();
   }
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::empty_flag_buf(void)
{
   mpFlagBufOfs++;
   mFlagBufLeft = 32;

   if ((mTokenBufLen += 32) == DEFL_MAX_TOKENS)
      code_token_buf(FALSE);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::flush_flag_buf(void)
{
   if (mFlagBufLeft != 32)
   {
      mTokenBufLen += (32 - mFlagBufLeft);

      while (mFlagBufLeft)
      {
         *mpFlagBufOfs <<= 1;
         mFlagBufLeft--;
      }

      mpFlagBufOfs++;
      mFlagBufLeft = 32;
   }

   code_token_buf(TRUE);
}
//-------------------------------------------------------------------------------------------------------------
#define FLAG(i) { \
   *mpFlagBufOfs = (*mpFlagBufOfs << 1) | (i); \
   if (--mFlagBufLeft == 0) empty_flag_buf(); }
//-------------------------------------------------------------------------------------------------------------
#define CHAR { \
   *mpTokenBufOfs++ = mDict[mSearchOffset++]; \
   mSearchBytesLeft--; \
   mTokenBufBytes++; \
   FLAG(0); }
//-------------------------------------------------------------------------------------------------------------
#define MATCH(len, dist) { \
   *mpTokenBufOfs++ = static_cast<uchar>(((len) - DEFL_MIN_MATCH)); \
   write_word(mpTokenBufOfs, dist); \
   mpTokenBufOfs += 2; \
   mSearchOffset += (len); \
   mSearchBytesLeft -= (len); \
   mTokenBufBytes += (len); \
   FLAG(1); }
//-------------------------------------------------------------------------------------------------------------
void BDeflate::dict_search_lazy(void)
{
   int match_dist, match_len_cur;
   uint match_pos_cur;

   while ((mSearchBytesLeft) && (mSearchOffset < mSearchThreshold))
   {
      if (mNext[mSearchOffset & (DEFL_DICT_SIZE - 1)] == DEFL_NIL)
      {
         CHAR

         continue;
      }

      mMatchLen = DEFL_THRESHOLD;

      find_match(mSearchOffset);

      if (mMatchLen == DEFL_THRESHOLD)
      {
         CHAR

         continue;
      }

      match_len_cur = mMatchLen;
      match_pos_cur = mMatchPos;

      while (match_len_cur < 128)
      {
         /* mMatchLen is already set to match_len_cur here */

         if (mNext[(mSearchOffset + 1) & (DEFL_DICT_SIZE - 1)] != DEFL_NIL)
            find_match(mSearchOffset + 1);
         else
            break;

         if (mMatchLen > (mSearchBytesLeft - 1))
            mMatchLen = mSearchBytesLeft - 1;

         if (mMatchLen <= match_len_cur)
            break;

         match_len_cur = mMatchLen;
         match_pos_cur = mMatchPos;

         CHAR
      }

      if (match_len_cur > mSearchBytesLeft)
      {
         if ((match_len_cur = mSearchBytesLeft) <= DEFL_THRESHOLD)
         {
            CHAR

            continue;
         }
      }

      match_dist = (mSearchOffset - match_pos_cur) & (DEFL_DICT_SIZE - 1);

      if ((match_len_cur == DEFL_MIN_MATCH) && (match_dist >= 16384))
         CHAR
      else
         MATCH(match_len_cur, match_dist)
   }

   mSearchOffset &= (DEFL_DICT_SIZE - 1);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::dict_search_greedy(void)
{
   int match_dist;

   while ((mSearchBytesLeft) && (mSearchOffset < mSearchThreshold))
   {
      if (mNext[mSearchOffset & (DEFL_DICT_SIZE - 1)] == DEFL_NIL)
      {
         CHAR

         continue;
      }

      mMatchLen = DEFL_THRESHOLD;

      find_match(mSearchOffset);

      if (mMatchLen == DEFL_THRESHOLD)
      {
         CHAR

         continue;
      }

      if (mMatchLen > mSearchBytesLeft)
      {
         if ((mMatchLen = mSearchBytesLeft) <= DEFL_THRESHOLD)
         {
            CHAR

            continue;
         }
      }

      match_dist = (mSearchOffset - mMatchPos) & (DEFL_DICT_SIZE - 1);

      if ((mMatchLen == DEFL_MIN_MATCH) && (match_dist >= 16384))
         CHAR
      else
         MATCH(mMatchLen, match_dist)
   }

   mSearchOffset &= (DEFL_DICT_SIZE - 1);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::dict_search(void)
{
   if (mGreedyFlag)
      dict_search_greedy();
   else
      dict_search_lazy();
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::dict_search_main(uint dict_ofs)
{
   uint search_gap_bytes = ((dict_ofs - mSearchOffset) & (DEFL_DICT_SIZE - 1));

   mSearchBytesLeft += search_gap_bytes;

   mSearchThreshold = mSearchOffset + search_gap_bytes + (DEFL_SECTOR_SIZE - (DEFL_MAX_MATCH + 1));

   dict_search();
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::dict_search_eof(uint dict_ofs)
{
   uint search_gap_bytes = ((dict_ofs - mSearchOffset) & (DEFL_DICT_SIZE - 1));

   mSearchBytesLeft += search_gap_bytes;
   
   mSearchThreshold = 0xFFFF;

   dict_search();
}
//-------------------------------------------------------------------------------------------------------------
int BDeflate::dict_fill(uint dict_ofs, int bytes_left)
{
   int bytes_written = 0, bytes_to_read;

   while (bytes_left)
   {
      while ((bytes_to_read = min(mInBufLeft, bytes_left)) == 0)
      {
         if (mEOFFlag)
            goto dict_fill_done;

         refill_buf();
      }

      mem_copy(mDict + dict_ofs, mpInBufCurOfs, bytes_to_read);

      mpInBufCurOfs += bytes_to_read;
      mInBufLeft    -= bytes_to_read;
      dict_ofs      += bytes_to_read;
      bytes_left    -= bytes_to_read;
      bytes_written += bytes_to_read;
   }

dict_fill_done:

   mem_set(mDict + dict_ofs, 0, bytes_left);

   return (bytes_written);
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::deflate_main_init(void)
{
#if 0
   int i;
   
   for (i = 0; i < DEFL_DICT_SIZE; i++)
   {
      mLast[i] = DEFL_NIL;
      mNext[i] = DEFL_NIL;
   }

   for (i = 0; i < DEFL_HASH_SIZE; i++)
      mHash[i] = DEFL_NIL;
#endif

   Utils::FastMemSet(mLast, 0xFF, DEFL_DICT_SIZE * sizeof(mLast[0]));      
   Utils::FastMemSet(mNext, 0xFF, DEFL_DICT_SIZE * sizeof(mNext[0]));      
   Utils::FastMemSet(mHash, 0xFF, DEFL_HASH_SIZE * sizeof(mHash[0]));

   mpFlagBufOfs = mFlagBuf;
   mFlagBufLeft = 32;
   mpTokenBufOfs = mTokenBuf;
}
//-----------------------------------------------------------------------------------------------------------------------
void BDeflate::deflate_main(void)  
{
   int delete_flag = FALSE;
   uint dict_pos = 0;

   deflate_main_init();

   for ( ; ; )
   {
      if (delete_flag)
         delete_data(dict_pos);

      const int bytesRead = dict_fill(dict_pos, DEFL_SECTOR_SIZE);
      mSearchBytesLeft = bytesRead;
      if (!bytesRead)
         break;

      assert(((dict_pos & (DEFL_SECTOR_SIZE - 1)) == 0));
      
      hash_data(dict_pos, mSearchBytesLeft);

      if (dict_pos == 0)
         mem_copy(mDict + DEFL_DICT_SIZE, mDict, DEFL_SECTOR_SIZE + DEFL_MAX_MATCH);

      dict_search_main(dict_pos);
               
      if ((dict_pos += bytesRead) == DEFL_DICT_SIZE)
      {
         dict_pos = 0;
         delete_flag = TRUE;
      }
      
      if ((mInBufLeft == 0) && (mEOFFlag))
      {
         // mSearchBytesLeft must reflect the # of bytes copied into the current sector, not the # of actual bytes in the dict left to compress!
         mSearchBytesLeft = 0;
         break;
      }
   }

   dict_search_eof(dict_pos);

   flush_flag_buf();

   flush_bits();

   for ( ; ; )
      send_message(DEFL_STATUS_DONE);
}
//-----------------------------------------------------------------------------------------------------------------------
void BDeflate::deflate_main_func(void* privateData)
{
   BDeflate* pDefl = reinterpret_cast<BDeflate*>(privateData);
   pDefl->deflate_main();
}
//-------------------------------------------------------------------------------------------------------------
int BDeflate::compress(
   const uchar *_in_buf_ofs, int *_in_buf_size,
   uchar *_out_buf_ofs, int *_out_buf_size,
   int _eof_flag,
   int _max_compares, int _strategy, int _greedy_flag)
{
   assert(_in_buf_ofs);
   assert(_out_buf_ofs);
   assert(_in_buf_size);
   assert(_out_buf_size);
   assert(*_in_buf_size >= 0);
   assert(*_out_buf_size >= 0);
   assert(_max_compares > 0);
   
   int status;

#if 0
#ifdef _DEBUG
   mDict = NULL;
   mHash = NULL;
   mLast = NULL;
   mNext = NULL;
   mMaxCompares = 0;
   mMatchLen = 0;
   mMatchPos = 0;
   mpOutBufCurOfs = NULL;
   mOutBufLeft = 0;
   mBitBuf = 0;
   mBitBufLen = 0;
   mBitBufTotalFlag = FALSE;
   int_set(mHeap, 0, DEFL_MAX_SYMBOLS + 1);
   int_set(mOthers, 0, DEFL_MAX_SYMBOLS);
   int_set(mCodeList, 0, DEFL_MAX_SYMBOLS);
   mCodeListLen = 0;
   int_set(mNumCodes, 0, 33);
   int_set(mNextCode, 0, 33);
   int_set(mNewCodeSizes, 0, DEFL_MAX_SYMBOLS);
#endif
#endif      
   
   mMaxCompares     = _max_compares;
   mStategy         = _strategy;
   mGreedyFlag      = _greedy_flag;

   mpInBufOfs       = _in_buf_ofs;
   mpInBufSizeOfs   = _in_buf_size;
   mInBufSize       = *_in_buf_size;
   mpInBufCurOfs    = mpInBufOfs;
   mInBufLeft       = mInBufSize;

   mpOutBufOfs      = _out_buf_ofs;
   mpOutBufSizeOfs  = _out_buf_size;
   mOutBufSize      = *_out_buf_size;
   mpOutBufCurOfs   = mpOutBufOfs;
   mOutBufLeft      = mOutBufSize;

   mEOFFlag         = _eof_flag;
         
   mMatchLen        = mSavedMatchLen;
   mMatchPos        = mSavedMatchPos;

   mBitBuf          = mSavedBitBuf;
   mBitBufLen       = mSavedBitBufLen;
   mBitBufTotalFlag = mSavedBitBufTotalFlag;

   BDEBUG_ASSERT(mSSD.getFiber());
   status = mSSD.begin();

   mSavedMatchLen   = mMatchLen;
   mSavedMatchPos   = mMatchPos;

   mSavedBitBuf     = mBitBuf;
   mSavedBitBufLen  = mBitBufLen;
   mSavedBitBufTotalFlag = mBitBufTotalFlag;

   return (status);
}
//-------------------------------------------------------------------------------------------------------------
BDeflate::BDeflate()
{
}
//-------------------------------------------------------------------------------------------------------------
BDeflate::~BDeflate()
{
}
//-------------------------------------------------------------------------------------------------------------
void BDeflate::init(void)
{
   mSSD.deinit();
         
   // just being safe
   Utils::FastMemSet(this, 0, sizeof(BDeflate));

   #if 0    
   mpTokenBufOfs = 0;
   mTokenBufLen = 0;

   mpFlagBufOfs = 0;
   mFlagBufLeft = 0;

   mTokenBufStart = 0;
   mTokenBufEnd = 0;
   mTokenBufBytes = 0;

   mpCodedSizesEnd = 0;

   mUsedLitCodes = 0;
   mUsedDistCodes = 0;

   mSavedBitBuf = 0;
   mSavedBitBufLen = 0;
   mSavedBitBufTotalFlag = 0;
   mBitBufTotal = 0;

   mSearchOffset = 0;
   mSearchBytesLeft = 0;
   mSearchThreshold = 0;

   mSavedMatchLen = 0;
   mSavedMatchPos = 0;

   mMaxCompares = 0;

   mStategy = 0;

   mGreedyFlag = 0;

   mEOFFlag = 0;

   mpInBufCurOfs = 0;
   mInBufLeft = 0;

   mpInBufOfs = 0;
   mInBufSize = 0;
   mpInBufSizeOfs = 0;

   mpOutBufOfs = 0;
   mOutBufSize = 0;
   mpOutBufSizeOfs = 0;
   #endif

   mSSD.init(deflate_main_func, reinterpret_cast<void*>(this));
}
//-------------------------------------------------------------------------------------------------------------
int BDeflate::compressAll(
   const uchar* pSrcBuf, int srcBufLen,
   uchar* pDstBuf, int& dstBufLen,
   int _max_compares, int _strategy, int _greedy_flag)
{
   assert(pSrcBuf);
   assert(pDstBuf);
   assert(dstBufLen);
   assert(_max_compares > 0);
   
   int srcOfs = 0;
   int dstOfs = 0;
   
   int status;
   do
   {
      int inBufSize = srcBufLen - srcOfs;
      int outBufSize = dstBufLen - dstOfs;
      
      status = compress(
         pSrcBuf + srcOfs, &inBufSize, 
         pDstBuf + dstOfs, &outBufSize,
         TRUE,
         _max_compares, _strategy, _greedy_flag);
      
      srcOfs += inBufSize;
      dstOfs += outBufSize;
      dstBufLen = dstOfs;
      
      if ((DEFL_STATUS_DONE != status) && (dstOfs == dstBufLen))
         status = DEFL_STATUS_DST_BUF_FULL;
         
   } while (DEFL_STATUS_OKAY == status);
   
   return status;
}      
