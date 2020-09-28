//============================================================================
// deflate.h
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#pragma once
#include "threading\stateSwitch.h"

#define DEFL_STATUS_OKAY (0)
#define DEFL_STATUS_DONE (1)
#define DEFL_STATUS_DST_BUF_FULL (2)

#define DEFL_STATIC_BLOCKS  (0)
#define DEFL_DYNAMIC_BLOCKS (1)
#define DEFL_ALL_BLOCKS     (2)

#define DEFL_MIN_MATCH     (3)
#define DEFL_THRESHOLD     (DEFL_MIN_MATCH - 1)
#define DEFL_MAX_MATCH     (258)

#define DEFL_DICT_BITS     (15)
#define DEFL_HASH_BITS     (13)
#define DEFL_SHIFT_BITS    ((DEFL_HASH_BITS + (DEFL_MIN_MATCH - 1)) / DEFL_MIN_MATCH)
#define DEFL_SECTOR_BITS   (12)

#define DEFL_DICT_SIZE     (1 << DEFL_DICT_BITS)
#define DEFL_HASH_SIZE     (1 << DEFL_HASH_BITS)
#define DEFL_SECTOR_SIZE   (1 << DEFL_SECTOR_BITS)

#define DEFL_HASH_FLAG_1   (0x8000)
#define DEFL_HASH_FLAG_2   (0x7FFF)

#define DEFL_NEXT_MASK     (0x7FFF)

#define DEFL_MAX_TOKENS    (12288)

#define DEFL_NUM_SYMBOLS_1 (288)
#define DEFL_NUM_SYMBOLS_2 (32)
#define DEFL_NUM_SYMBOLS_3 (19)
#define DEFL_MAX_SYMBOLS   (288)

class BDeflate
{
public:
      BDeflate();
      ~BDeflate();
         
      // Clears the deflator's entire internal state.
      // Musy be called before calling any of the compress() methods.
      // May be called multiple times.
      void init(void);
   
      // compression() may be called multiple times for large data streams.
      //
      // On entry, _in_buf_ofs and _in_buf_size indicate the location and size of the input buffer. On exit, _in_buf_size indicates
      // the number of bytes read from the input buffer. _out_buf_ofs/_out_buf_size control the output buffer.
      //
      // if _eof_flag is TRUE, there are no more bytes remaining in the input stream, apart from what is available in the input buffer.
      //
      // _max_compares controls how many compares the comparator will use while finding matches. Larger values
      // lead to better, but slower compression and vice versa. Use 500-1000 for best compression.
      //
      // _strategy controls how blocks of LZ77 codes are compressed: 
      // DEFL_ALL_BLOCKS - The compressor will try both static and dynamic codes and choose whatever leads to the smallest file size.
      // DEFL_STATIC_BLOCKS - Always use "static" Huffman codes (practically useless except for tiny files).
      // DEFL_DYNAMIC_BLOCKS - Always use "dynamic" Huffman codes.
      //
      // _greedy_flag - If TRUE, a greedy scheme is used to find match distances. This is faster but leads to less compression.
      //      
      // Returns DEFL_STATUS_OKAY or DEFL_STATUS_DONE.
      int compress(
         const uchar *_in_buf_ofs, int *_in_buf_size,
         uchar *_out_buf_ofs, int *_out_buf_size,
         int _eof_flag,
         int _max_compares = 128, int _strategy = DEFL_ALL_BLOCKS, int _greedy_flag = 0);
         
      // comparessAll() may be called one time.
      // Returns DEFL_STATUS_OKAY, DEFL_STATUS_DONE, or DEFL_STATUS_DST_BUF_FULL.
      int compressAll(
         const uchar* pSrcBuf, int srcBufLen,
         uchar* pDstBuf, int& dstBufLen,
         int _max_compares = 128, int _strategy = DEFL_ALL_BLOCKS, int _greedy_flag = 0);

private:
   uchar  mDict[DEFL_DICT_SIZE + DEFL_SECTOR_SIZE + DEFL_MAX_MATCH];
   ushort mHash[DEFL_HASH_SIZE], mNext[DEFL_DICT_SIZE], mLast[DEFL_DICT_SIZE];

   uchar  mTokenBuf[DEFL_MAX_TOKENS * 3], *mpTokenBufOfs;
   int    mTokenBufLen;

   uint   mFlagBuf[DEFL_MAX_TOKENS >> 5], *mpFlagBufOfs;
   int    mFlagBufLeft;

   uint   mTokenBufStart;
   uint   mTokenBufEnd;
   int    mTokenBufBytes;

   int    mFreq1[DEFL_NUM_SYMBOLS_1], mFreq2[DEFL_NUM_SYMBOLS_2], mFreq3[DEFL_NUM_SYMBOLS_3];
   int    mSize1[DEFL_NUM_SYMBOLS_1], mSize2[DEFL_NUM_SYMBOLS_2], mSize3[DEFL_NUM_SYMBOLS_3];
   uint   mCode1[DEFL_NUM_SYMBOLS_1], mCode2[DEFL_NUM_SYMBOLS_2], mCode3[DEFL_NUM_SYMBOLS_3];

   int    mBundledSizes[DEFL_NUM_SYMBOLS_1 + DEFL_NUM_SYMBOLS_2];
   int    mCodeSizes[DEFL_NUM_SYMBOLS_1 + DEFL_NUM_SYMBOLS_2], *mpCodedSizesEnd;

   int    mUsedLitCodes, mUsedDistCodes;

   uint   mSavedBitBuf;
   int    mSavedBitBufLen;
   int    mSavedBitBufTotalFlag;
   uint   mBitBufTotal;

   uint   mSearchOffset;
   int    mSearchBytesLeft;
   uint   mSearchThreshold;

   int    mSavedMatchLen;
   int    mSavedMatchPos;

   int    mMaxCompares;

   int    mStategy;

   int    mGreedyFlag;

   int    mEOFFlag;

   const uchar *mpInBufCurOfs;
   int    mInBufLeft;

   const uchar *mpInBufOfs;
   int    mInBufSize;
   int   *mpInBufSizeOfs;

   uchar *mpOutBufOfs;
   int    mOutBufSize;
   int   *mpOutBufSizeOfs;

   BStateSwitcher mSSD;
         
   int     mMatchLen;
   uint    mMatchPos;
   uchar  *mpOutBufCurOfs;
   int     mOutBufLeft;

   uint    mBitBuf;
   int     mBitBufLen;
   int     mBitBufTotalFlag;

   int     mHeap[DEFL_MAX_SYMBOLS + 1];
   int     mOthers[DEFL_MAX_SYMBOLS];
   int     mCodeList[DEFL_MAX_SYMBOLS];
   int     mCodeListLen;
   int     mNumCodes[33];
   int     mNextCode[33];
   int     mNewCodeSizes[DEFL_MAX_SYMBOLS];
   
   void return_buf_sizes(void);
   void send_message(int status);
   void refill_buf(void);
   void huff_down_heap(int *mHeap, int *sym_freq, int heap_len, int i);
   void huff_code_sizes(int num_symbols, int *sym_freq, int *code_sizes);
   void huff_sort_code_sizes(int num_symbols, int *code_sizes);
   void huff_fix_code_sizes(int max_code_size);
   void huff_make_codes(int num_symbols, int *code_sizes, int max_code_size, uint *codes);
   void put_bits(uint bits, int len);
   void flush_bits(void);
   void delete_data(uint dict_pos);
   void hash_data(uint dict_pos, int bytes_to_do);
   void find_match(uint dict_pos);
   void code_block(void);
   void init_static_block(void);
   void send_static_block(void);
   int * repeat_last(int *dst, int size, int run_len);
   int * repeat_zero(int *dst, int run_len);
   void init_compress_code_sizes(void);
   void compress_code_sizes(void);
   void init_dynamic_block(void);
   void send_dynamic_block(void);
   void send_raw_block(void);
   void code_token_buf(int last_block_flag);
   void empty_flag_buf(void);
   void flush_flag_buf(void);
   void dict_search_lazy(void);
   void dict_search_greedy(void);
   void dict_search(void);
   void dict_search_main(uint dict_ofs);
   void dict_search_eof(uint dict_ofs);
   int dict_fill(uint dict_ofs, int bytes_left);
   void deflate_main_init(void);
   void deflate_main(void);
   static void deflate_main_func(void* privateData);
};
