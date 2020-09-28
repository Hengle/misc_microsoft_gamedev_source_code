//============================================================================
// inflate.h
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#pragma once

#include "threading\stateSwitch.h"

#define INFL_STATUS_OKAY             ( 0)
#define INFL_STATUS_DONE             ( 1)
#define INFL_STATUS_CODE_SET_ERROR   (-1)
#define INFL_STATUS_MEM_ERROR        (-2)
#define INFL_STATUS_BAD_BLOCK_ERROR  (-3)
#define INFL_STATUS_RAW_BLOCK_ERROR  (-4)
#define INFL_STATUS_DYN_BLOCK_ERROR  (-5)
#define INFL_STATUS_BAD_CODE_ERROR   (-6)
#define INFL_STATUS_TOO_MUCH_ERROR   (-7)
//#define INFL_STATUS_INCOMPLETE_ERROR (-8)
#define INFL_STATUS_UNKNOWN_ERROR    (-9)
#define INFL_STATUS_CORRUPTED_ERROR  (-10)

#define INFL_DICT_SIZE               (32768)

#define INFL_NUM_SYMBOLS_1           (288)
#define INFL_NUM_SYMBOLS_2           (32)
#define INFL_NUM_SYMBOLS_3           (19)

class BInflate
{
public:
   BInflate();

   // init() must be called before calling any of the decompress() methods.
   // May be called multiple times.      
   //
   // If useFastMemCpy is true, XMemCpy() is used to copy data to the output buffer, otherwise memcpy().
   //
   // If clearDictionary is true, the 32K dictionary is cleared to 0's.
   // Clearing the dictionary should only be necessary with compressed streams containing matches that are 
   // before the start of the output stream, which I've never seen in practice.
   // PKWare's appnote.txt doesn't mention this possibility with Deflate, but they do mention it with Implode.
   // RFC 1951 doesn't mention this possibility.
   void init(bool useFastMemCpy = true, bool clearDictionary = false);
   
   void deinit(void);
      
   // decompress() is callable multiple times. 
   //
   // On entry, *pInBufBytes and *pOutBufBytes indicate the number of bytes available in each buffer.
   // On return, they indicate the number of bytes read from or written to each buffer.
   // If the return status is INFL_STATUS_OKAY, it is possible that *pInBufBytes could be negative, which indicates
   // the inflator read too many bytes into its bit buffer before determining the stream had ended.
   //
   // If eofFlag is true, there are no more bytes remaining in the input stream beyond what is available in the input buffer.
   //
   // If buffered is true:
   // The decompressor will decompress into an internal 32K buffer and memcpy into the supplied output buffer.
   // pOutBufStart may be NULL.
   //
   // If buffered is false:
   // The decompressor will decompress directly to the output buffer. The output buffer must be in cachable memory.
   // The total size of the output buffer must be >= the size of the decompressed data. 
   // (However, it's fine if *pOutBufBytes is less than the total size of the decompressed data, as long as the remaining space is large 
   // enough to hold the remaining bytes to decompress.)
   // pOutBufStart must point to the start of the destination buffer. This prevents the  decompressor from reading 
   // before the start of the destination buffer in case it receives an invalid match distance.
   //
   // Return status:
   // INFL_STATUS_OKAY indicates the decompressor has paused due to either an empty input buffer or a completely full output buffer.
   // INFL_STATUS_DONE indicates the decompressor has finished, but this does not mean that the output stream is valid. 
   // It's up to the caller to ensure that the expected number of bytes have been decompressed, and to check the integrity of the output
   // data with a CRC, Adler-32, etc.
   // Status < 0 indicates an error. 
   
   int decompress(
      const uchar* pInBuf, int* pInBufBytes,
      uchar* pOutBuf, int* pOutBufBytes,
      bool eofFlag,
      bool buffered = true,
      uchar* pOutBufStart = NULL);

   struct BBufferRefiller
   {
      // Returns number of bytes written to buffer, 0 for EOF, or -1 for error.
      virtual int operator() (uchar* Pbuf, int buf_size) = 0;
   };

   class BDataBufferRefiller : public BBufferRefiller
   {
   public:
      BDataBufferRefiller(const uchar* p, int size) : mpSrc(p), mLeft(size) { }

      // returns bytes written, or 0 for EOF
      int operator() (uchar* Pbuf, int buf_size)
      {
         int bytes_to_write = min(buf_size, mLeft);
         if (bytes_to_write == 0)
            return 0;
         
         Utils::FastMemCpy(Pbuf, mpSrc, bytes_to_write);

         mpSrc += bytes_to_write;
         mLeft -= bytes_to_write;
         return bytes_to_write;
      }
      
   private:
      const uchar* mpSrc;
      int mLeft;
   };

   // decompressAll() can be called a single time.
   // The refiller object is called to refill an internal 4k buffer, as needed.
   // dstLen must be >= the size of the decompressed data. 
   // srcRead will be the total number of bytes actually read, which may be 
   // less than the total number of bytes requested through the refiller.
   int decompressAll(
      BBufferRefiller& refiller,
      int& srcRead,
      uchar *pDst, int dstLen, int& dstWritten);
   
   // Memory to memory decompression. Can be called a single time.
   // srcLen must be >= the size of the compressed data.
   // dstLen must be >= the size of the decompressed data. 
   int decompressAll(
      const uchar* Psrc, int srcLen,
      uchar *pDst, int dstLen,
      int& srcRead,
      int& dstWritten);

   ~BInflate();
   
   bool buffered(void) const { return mBuffered; }
   
   // setBuffered() can only be called at the start of the stream!
   void setBuffered(bool buffered) { mBuffered = buffered; }

private:
   uint64 mBitBuf;
         
   int    mBitBufLen;
   uint   mNumEOFPadBytes;

   signed char mCodeSize1[INFL_NUM_SYMBOLS_1];
   signed char mCodeSize2[INFL_NUM_SYMBOLS_2];
   signed char mCodeSize3[INFL_NUM_SYMBOLS_3];

   int* mLookUp1;
   int* mLookUp2;
   int* mLookUp3;

   int* mTree1;
   int* mTree2;
   int* mTree3;
               
   int* mPInBufBytes;
   int* mPOutBufBytes;
   
   const uchar* RESTRICT mpInBuf;
   int    mInBufLeft;
         
   uchar* RESTRICT mpOutBufStart;
   uchar* RESTRICT mpOutBuf;
   int    mOutBufLeft;
                           
   BStateSwitcher mSSD;
         
   uchar mDict[INFL_DICT_SIZE];
   int mDictOfs;
   
   bool   mEOFFlag : 1;
   bool   mBuffered : 1;
   bool   mUseFastMemCpy : 1;
   
   void build_huffman_decoder_tables(int num_symbols, signed char* code_size,
                                    int** _look_up, int** _tree);

   void return_status(int status);
   __declspec(noreturn) void error(int status);
   inline uint get_byte(void);
   inline uint get_dword(void);
   inline void remove_bits(int num_bits);
   inline uint get_bits(int num_bits);
   inline int get_symbol(int*  look_up, int*  tree);
   void raw_block_buffered(void);
   void raw_block_unbuffered(void);
   void raw_block(void);
   void decompress_block_buffered(void);
   void decompress_block_unbuffered(void);
   void decompress_block(void);
   void static_block(void);
   void dynamic_block(void);
   void inflate_block(void);
   __declspec(noreturn)void inflate_main(void);
   __declspec(noreturn)static void inflate_main_func(void* funcData);
   void flushDict(void);
};
