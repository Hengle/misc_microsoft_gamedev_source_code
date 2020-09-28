//============================================================================
// inflate.cpp
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#include "compression.h"
#include "xcore.h"
#include "inflate.h"

#if defined(XBOX) && !defined(BUILD_FINAL)
   #include <tracerecording.h>
   #pragma comment(lib, "tracerecording.lib")
   #pragma comment(lib, "xbdm.lib")

   #define TRACE_RECORDING 0
#endif

//-----------------------------------------------------------------------------------------------------------
#define INFL_MIN_MATCH                (3)
#define INFL_MAX_MATCH                (258)
//-----------------------------------------------------------------------------------------------------------
#ifdef _XBOX
   #define READ_LE_DWORD(p) __loadwordbytereverse(0, p)
#else
   #define READ_LE_DWORD(p) *reinterpret_cast<const DWORD*>(p)
#endif   
//-----------------------------------------------------------------------------------------------------------
static const uchar bit_length_order[] =
{
  16, 17, 18, 0, 8,  7,  9, 6, 10,  5, 11, 4, 12,  3, 13, 2, 14,  1, 15
};
//-----------------------------------------------------------------------------------------------------------
static const int length_extra[] =
{
  0,0,0,0, 0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4, 5,5,5,5, 0, 0,0, 0
};
//-----------------------------------------------------------------------------------------------------------
static const int length_range[] =
{
  3,4,5,6, 7,8,9,10, 11,13,15,17, 19,23,27,31, 35,43,51,59, 67,83,99,115, 131,163,195,227, 258, 0,0, 0
};
//-----------------------------------------------------------------------------------------------------------
static const int dist_extra[] =
{
  0,0,0,0, 1,1,2,2, 3,3,4,4, 5,5,6,6, 7,7,8,8, 9,9,10,10, 11,11,12,12, 13,13, 0, 0
};
//-----------------------------------------------------------------------------------------------------------
static const int dist_range[] =
{
  1,2,3,4, 5,7,9,13, 17,25,33,49, 65,97,129,193, 257,385,513,769, 1025,1537,2049,3073, 4097,6145,8193,12289, 16385,24577, 0, 0
};
//-----------------------------------------------------------------------------------------------------------
static const uint bitmasks[] = 
{
   (1U<<0)-1,   (1U<<1)-1,   (1U<<2)-1,   (1U<<3)-1,
   (1U<<4)-1,   (1U<<5)-1,   (1U<<6)-1,   (1U<<7)-1,
   (1U<<8)-1,   (1U<<9)-1,   (1U<<10)-1,  (1U<<11)-1,
   (1U<<12)-1,  (1U<<13)-1,  (1U<<14)-1,  (1U<<15)-1,
   (1U<<16)-1,  (1U<<17)-1,  (1U<<18)-1,  (1U<<19)-1,
   (1U<<20)-1,  (1U<<21)-1,  (1U<<22)-1,  (1U<<23)-1,
   (1U<<24)-1,  (1U<<25)-1,  (1U<<26)-1,  (1U<<27)-1,
   (1U<<28)-1,  (1U<<29)-1,  (1U<<30)-1,  (1U<<31)-1,
   0xFFFFFFFF
};
//-----------------------------------------------------------------------------------------------------------
void BInflate::build_huffman_decoder_tables(
   int num_symbols, signed char* code_size,
   int** _look_up, int** _tree)
{
  int status = 0;
  uint i, j;
  uint total;
  uint old_code, new_code;
  int next_free_entry, current_entry;
  int num_codes[17];
  uint next_code[17];
  uint *code = NULL;
  int* look_up = NULL;
  int* tree = NULL;

  delete [] *_look_up; *_look_up = NULL;
  delete [] *_tree; *_tree = NULL;

  if ( ((code    = new uint[num_symbols]) == NULL) ||
       ((look_up = new int[256]) == NULL) ||
       ((tree    = new int[num_symbols * 2]) == NULL) )
    goto mem_error;

  for (i = 0; i <= 16; i++)
    num_codes[i] = 0;

  for (i = 0; i < ((uint)num_symbols); i++)
    num_codes[code_size[i]]++;

  for (i = 1, next_code[0] = next_code[1] = 0, total = 0; i <= 15; i++)
    next_code[i + 1] = (uint)(total = ((total + ((uint)num_codes[i])) << 1));

  if (total != 0x10000L)
  {
    for (i = 16, j = 0; i != 0; i--)
      if ((j += num_codes[i]) > 1)
        goto code_set_error;
  }

  for (i = 0; i < ((uint)num_symbols); i++)
    code[i] = next_code[code_size[i]]++;

  for (i = 0; i < 256; i++)
    look_up[i] = 0;

  for (i = 0; i < (((uint)num_symbols) << 1); i++)
    tree[i] = 0;

  next_free_entry = -1;

  for (i = 0; i < ((uint)num_symbols); i++)
  {
    if (code_size[i] != 0)
    {
      old_code = code[i];

      new_code = 0;

      for (j = code_size[i]; j != 0; j--)
      {
        new_code = (new_code << 1) | (old_code & 1);
        old_code >>= 1;
      }

      if (code_size[i] <= 8)
      {
        j = 1 << code_size[i];

        while (new_code < 256)
        {
          look_up[new_code] = i | (code_size[i] << 16);

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

        for (j = code_size[i]; j > 9; j--)
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

        tree[-current_entry - 1] = i | (code_size[i] << 16);
      }
    }
  }

  status = 0;

exit:

  delete [] code;

  if (status)
  {
    delete [] look_up;
    delete [] tree;

    look_up = NULL;
    tree = NULL;
  }

  *_look_up = look_up;
  *_tree    = tree;

  if (status)
    error(status);

  return;

mem_error:

  status = INFL_STATUS_MEM_ERROR;
  goto exit;

code_set_error:

  status = INFL_STATUS_CODE_SET_ERROR;
  goto exit;
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::return_status(int status)
{
   *mPInBufBytes = *mPInBufBytes - mInBufLeft;
   *mPOutBufBytes = *mPOutBufBytes - mOutBufLeft;
      
   mSSD.setReturnStatus(status);
   mSSD.ret();
}
//-----------------------------------------------------------------------------------------------------------
__declspec(noreturn) void BInflate::error(int status)
{
   if (!status)
      status = INFL_STATUS_UNKNOWN_ERROR;
      
   return_status(status);
   
   // should never get here!
   abort();
}
//-----------------------------------------------------------------------------------------------------------
inline uint BInflate::get_byte(void)
{
   while (--mInBufLeft < 0)
   {
      mInBufLeft++;

      if (mEOFFlag)
      {
         mNumEOFPadBytes++;

         // rg [3/17/08] - Something is definitely wrong if we've read this much beyond the EOF.
         // Should be able to use something less than 16 (but more than 8), but I'm just being safe.
         if (mNumEOFPadBytes > 16)
         {
            for ( ; ; )
            {
               return_status(INFL_STATUS_CORRUPTED_ERROR);
            }
         }
         
         // It shouldn't matter what we return.
         return mNumEOFPadBytes & 0xFF;
      }

      return_status(INFL_STATUS_OKAY);
   }

   return *mpInBuf++;
}
//-----------------------------------------------------------------------------------------------------------
inline uint BInflate::get_dword(void)
{
   if ((mInBufLeft -= 4) < 0)
   {
      mInBufLeft += 4;

      uint val = get_byte();
      val |= (get_byte() << 8);
      val |= (get_byte() << 16);
      val |= (get_byte() << 24);
            
      return val;
   }

   uint val = READ_LE_DWORD(mpInBuf);

   mpInBuf += 4;
   return val;
}
//-----------------------------------------------------------------------------------------------------------
inline void BInflate::remove_bits(int num_bits)
{
   mBitBuf >>= num_bits;
   mBitBufLen -= num_bits;
   
   if (mBitBufLen >= 0)
      return;

   uint64 newBits;
   
   if ((mInBufLeft -= 4) < 0)
   {
      mInBufLeft += 4;
      newBits = get_byte();
      newBits |= (get_byte() << 8);
      newBits |= (get_byte() << 16);
      newBits |= (get_byte() << 24);
   }
   else
   {
      newBits = READ_LE_DWORD(mpInBuf);
      mpInBuf += 4;
   }
      
   mBitBufLen += 32;      
   mBitBuf |= (((uint64)newBits) << mBitBufLen);
}
//-----------------------------------------------------------------------------------------------------------
inline uint BInflate::get_bits(int num_bits)
{
   uint bits = (uint)(mBitBuf & bitmasks[num_bits]);

   mBitBuf >>= num_bits;
   mBitBufLen -= num_bits;

   if (mBitBufLen >= 0)
      return bits;

   uint64 newBits;

   if ((mInBufLeft -= 4) < 0)
   {
      mInBufLeft += 4;
      newBits = get_byte();
      newBits |= (get_byte() << 8);
      newBits |= (get_byte() << 16);
      newBits |= (get_byte() << 24);
   }
   else
   {
      newBits = READ_LE_DWORD(mpInBuf);
      mpInBuf += 4;
   }

   mBitBufLen += 32;      
   mBitBuf |= (newBits << mBitBufLen);

   return bits;
}
//-----------------------------------------------------------------------------------------------------------
inline int BInflate::get_symbol(int* look_up, int* tree)
{
   int symbol;
   if ((symbol = look_up[mBitBuf & 0xFF]) < 0)
   {
      uint temp = (uint)(mBitBuf >> 8);
      do
      {
         symbol = tree[(-symbol - 1) + (temp & 1)];
         temp >>= 1;
      } while (symbol < 0);
   }

   uint num_bits = symbol >> 16;
   symbol &= 0xFFFF;

   mBitBuf >>= num_bits;
   mBitBufLen -= num_bits;

   if (mBitBufLen < 0)
   {
      uint64 newBits;

      if ((mInBufLeft -= 4) < 0)
      {
         mInBufLeft += 4;
         newBits = get_byte();
         newBits |= (get_byte() << 8);
         newBits |= (get_byte() << 16);
         newBits |= (get_byte() << 24);
      }
      else
      {
         newBits = READ_LE_DWORD(mpInBuf);
         mpInBuf += 4;
      }

      mBitBufLen += 32;      
      mBitBuf |= (newBits << mBitBufLen);
   }      

   return symbol;
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::flushDict(void)
{
   int dictBytesLeft = mDictOfs;
   int dictOfs = 0;

   while (dictBytesLeft)
   {
      const int bytesToCopy = min(dictBytesLeft, mOutBufLeft);

      if (mUseFastMemCpy)
         Utils::FastMemCpy(mpOutBuf, mDict + dictOfs, bytesToCopy);
      else
         memcpy(mpOutBuf, mDict + dictOfs, bytesToCopy);

      mpOutBuf += bytesToCopy;
      mOutBufLeft -= bytesToCopy;

      dictBytesLeft -= bytesToCopy;
      dictOfs += bytesToCopy;

      if (0 == mOutBufLeft)
      {
         return_status(INFL_STATUS_OKAY);
         
         if ((dictBytesLeft > 0) && (0 == mOutBufLeft))
         {
            // There's more to flush, but the client is indicating that it won't let us write any more.
            return_status(INFL_STATUS_TOO_MUCH_ERROR);
         }
      }
   }

   mDictOfs = 0;
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::raw_block_buffered(void)
{
   uint block_len, not_block_len;

   remove_bits(mBitBufLen & 7);

   block_len = get_bits(16);
   not_block_len = get_bits(16);

   if (((~block_len) & 0xFFFF) != not_block_len)
      error(INFL_STATUS_RAW_BLOCK_ERROR);

#if 0      
   while (block_len)
   {
      mDict[mDictOfs] = (uchar)get_bits(8);
      mDictOfs++;
      
      if (INFL_DICT_SIZE == mDictOfs)
         flushDict();
         
      block_len--;
   }
#endif

   int bitBufBitsLeft = mBitBufLen + 32;

   while (block_len)
   {
      if (bitBufBitsLeft)
      {
         mDict[mDictOfs] = (uchar)mBitBuf;
         mDictOfs++;
         
         if (INFL_DICT_SIZE == mDictOfs)
            flushDict();

         mBitBuf >>= 8;
         bitBufBitsLeft -= 8;

         block_len--;
      }
      else
      {
         while (mInBufLeft == 0)
         {
            if (mEOFFlag)
               error(INFL_STATUS_RAW_BLOCK_ERROR);
            else
               return_status(INFL_STATUS_OKAY);
         }

         uint bytesToCopy = Math::Min3<uint>(block_len, mInBufLeft, INFL_DICT_SIZE - mDictOfs);
         
         Utils::FastMemCpy(mDict + mDictOfs, mpInBuf, bytesToCopy);
         mDictOfs += bytesToCopy;
                           
         mpInBuf += bytesToCopy;
         mInBufLeft -= bytesToCopy;
         
         if (INFL_DICT_SIZE == mDictOfs)
            flushDict();

         block_len -= bytesToCopy;
      }
   }

   while (bitBufBitsLeft < 32)
   {
      mBitBuf |= (get_byte() << bitBufBitsLeft);
      bitBufBitsLeft += 8;
   }

   mBitBufLen = bitBufBitsLeft - 32;   
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::raw_block_unbuffered(void)
{
   uint block_len, not_block_len;

   remove_bits(mBitBufLen & 7);

   block_len = get_bits(16);
   not_block_len = get_bits(16);

   if (((~block_len) & 0xFFFF) != not_block_len)
      error(INFL_STATUS_RAW_BLOCK_ERROR);

   if (block_len > static_cast<uint>(mOutBufLeft))
      error(INFL_STATUS_TOO_MUCH_ERROR);
     
   int bitBufBitsLeft = mBitBufLen + 32;
      
   while (block_len)
   {
      if (bitBufBitsLeft)
      {
         *mpOutBuf++ = (uchar)mBitBuf;
         mOutBufLeft--;
                  
         mBitBuf >>= 8;
         bitBufBitsLeft -= 8;
         
         block_len--;
      }
      else
      {
         while (mInBufLeft == 0)
         {
            if (mEOFFlag)
               error(INFL_STATUS_RAW_BLOCK_ERROR);
            else
               return_status(INFL_STATUS_OKAY);
         }
            
         uint bytesToCopy = Math::Min<uint>(block_len, mInBufLeft);
                  
         Utils::FastMemCpy(mpOutBuf, mpInBuf, bytesToCopy);
         mpOutBuf += bytesToCopy;
         mOutBufLeft -= bytesToCopy;
         
         mpInBuf += bytesToCopy;
         mInBufLeft -= bytesToCopy;
         
         block_len -= bytesToCopy;
      }
   }
   
   while (bitBufBitsLeft < 32)
   {
      mBitBuf |= (get_byte() << bitBufBitsLeft);
      bitBufBitsLeft += 8;
   }
   
   mBitBufLen = bitBufBitsLeft - 32;
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::raw_block(void)
{
   if (mBuffered)
      raw_block_buffered();
   else
      raw_block_unbuffered();
}
//-----------------------------------------------------------------------------------------------------------
#define STORE_BUF_LOCALS() do \
{                             \
   mDictOfs = dictOfs;        \
   mpInBuf = pInBuf;          \
   mInBufLeft = inBufLeft;    \
} while(0)
//-----------------------------------------------------------------------------------------------------------
#define LOAD_BUF_LOCALS() do  \
{                             \
   dictOfs = mDictOfs;        \
   pInBuf = mpInBuf;          \
   inBufLeft = mInBufLeft;    \
} while(0)
//-----------------------------------------------------------------------------------------------------------
#include "infMacros.h"
//-----------------------------------------------------------------------------------------------------------
void BInflate::decompress_block_buffered(void)
{
   uint64 bitBuf = mBitBuf;
   int bitBufLen = mBitBufLen;
   
   int dictOfs;
   const uchar* RESTRICT pInBuf;
   int inBufLeft;   
   
   LOAD_BUF_LOCALS();
   
   uint prev_bits = 7;  

#ifdef _XBOX  
   Utils::BPrefetchState srcPrefetchState = Utils::BeginPrefetchLargeStruct(pInBuf, 3);
   Utils::BPrefetchState dstPrefetchState = Utils::BeginPrefetchLargeStruct(mDict + dictOfs, 3);
#endif
   
   for ( ; ; )
   {
      int code;
      uint total_bits;

#ifdef _XBOX  
      srcPrefetchState = Utils::UpdatePrefetchLargeStruct(srcPrefetchState, pInBuf, 3);
      dstPrefetchState = Utils::UpdatePrefetchLargeStruct(dstPrefetchState, mDict + dictOfs, 3);
#endif  

      for ( ; ; )
      {
         // Speculatively hide the microcoded shift.
         uint bits = (uint)(bitBuf >> prev_bits);

         if ((code = mLookUp1[bitBuf & 0xFF]) < 0)
         {
            uint temp = (uint)(bitBuf >> 8);
            do
            {
               code = mTree1[(-code - 1) + (temp & 1)];
               temp >>= 1;
            } while (code < 0);
         }

         uint num_bits = code >> 16;
         total_bits = num_bits;
         code &= 0xFFFF;

         if (code >= 256)
            break;

         // ----

         if (num_bits != prev_bits)
            bits = (uint)(bitBuf >> num_bits);

         int code1;

         if ((code1 = mLookUp1[bits & 0xFF]) < 0)
         {
            uint temp = (uint)(bits >> 8);
            do
            {
               code1 = mTree1[(-code1 - 1) + (temp & 1)];
               temp >>= 1;
            } while (code1 < 0);
         }

         prev_bits = code1 >> 16;
         total_bits += prev_bits;
         code1 &= 0xFFFF;

         if (code1 >= 256)
         {
            mDict[dictOfs] = (uchar)code;
            dictOfs++;
            if (INFL_DICT_SIZE == dictOfs)
            {
               STORE_BUF_LOCALS();
               flushDict();
               LOAD_BUF_LOCALS();
            }
            
            code = code1;
            break;
         }

         bitBuf >>= total_bits;
         bitBufLen -= total_bits;
                  
         if (dictOfs >= INFL_DICT_SIZE - 2)
         {
            mDict[dictOfs] = (uchar)code;
            dictOfs++;
            if (dictOfs == INFL_DICT_SIZE)
            {
               STORE_BUF_LOCALS();
               flushDict();
               LOAD_BUF_LOCALS();
            }
            
            mDict[dictOfs] = (uchar)code1;
            dictOfs++;
            if (dictOfs == INFL_DICT_SIZE)
            {
               STORE_BUF_LOCALS();
               flushDict();
               LOAD_BUF_LOCALS();
            }
         }
         else
         {
            mDict[dictOfs] = (uchar)code;
            mDict[dictOfs + 1] = (uchar)code1;
            dictOfs += 2;
         }
         
         if (bitBufLen < 0)
         {
            READ_FOUR_BYTES(); 
         }            
      }

      REMOVE_BITS(total_bits);      

      if (code == 256)
         break;

      int match_len = length_range[code - 257];

      if (code >= 265)
      {
         uint length_extra_bits;
         int num_length_extra_bits = length_extra[code - 257];
         GET_BITS(num_length_extra_bits, length_extra_bits);
         match_len += length_extra_bits;
      }

      GET_SYMBOL(mLookUp2, mTree2, code);
                  
      uint dist_extra_bits;
      int num_dist_extra_bits = dist_extra[code];
      GET_BITS(num_dist_extra_bits, dist_extra_bits);
      int match_dist = dist_range[code] + dist_extra_bits;

      if ((match_dist > dictOfs) || ((dictOfs + match_len) >= INFL_DICT_SIZE))
      {
         while (match_len > 0)
         {
            mDict[dictOfs] = mDict[(dictOfs - match_dist) & (INFL_DICT_SIZE - 1)];
            dictOfs++;
            
            if (INFL_DICT_SIZE == dictOfs)
            {
               STORE_BUF_LOCALS();
               flushDict();
               LOAD_BUF_LOCALS();
            }
         
            match_len--;
         }
      }
      else
      {
         uchar* dict_dst_ofs = mDict + dictOfs;
         uchar* dict_src_ofs = dict_dst_ofs - match_dist;
         
         dictOfs += match_len;
            
         if (match_len > match_dist)
         {
            if (match_dist == 1)
            {
               Utils::FastMemSet(dict_dst_ofs, dict_src_ofs[0], match_len);
            }
            else
            {
               for (int i = match_len; i > 0; i--)
                  *dict_dst_ofs++ = *dict_src_ofs++;
            }               
         }
         else
         {
            Utils::FastMemCpy(dict_dst_ofs, dict_src_ofs, match_len);
         }
      }               
   }
   
   STORE_BUF_LOCALS();

   mBitBuf = bitBuf;
   mBitBufLen = bitBufLen;
}
//-----------------------------------------------------------------------------------------------------------
#undef STORE_BUF_LOCALS
#define STORE_BUF_LOCALS() do \
{                             \
   mpOutBuf = pOutBuf;        \
   mOutBufLeft = outBufLeft;  \
   mpInBuf = pInBuf;          \
   mInBufLeft = inBufLeft;    \
} while(0)
//-----------------------------------------------------------------------------------------------------------
#undef LOAD_BUF_LOCALS
#define LOAD_BUF_LOCALS() do  \
{                             \
   pOutBuf = mpOutBuf;        \
   outBufLeft = mOutBufLeft;  \
   pInBuf = mpInBuf;          \
   inBufLeft = mInBufLeft;    \
} while(0)
//-----------------------------------------------------------------------------------------------------------
#include "infMacros.h"
//-----------------------------------------------------------------------------------------------------------
void BInflate::decompress_block_unbuffered(void)
{
   uint64 bitBuf = mBitBuf;
   int bitBufLen = mBitBufLen;
   
   uchar* RESTRICT pOutBuf;
   int outBufLeft;
   const uchar* RESTRICT pInBuf;
   int inBufLeft;
   
   LOAD_BUF_LOCALS();

#ifdef _XBOX  
   Utils::BPrefetchState srcPrefetchState = Utils::BeginPrefetchLargeStruct(pInBuf, 3);
   Utils::BPrefetchState dstPrefetchState = Utils::BeginPrefetchLargeStruct(pOutBuf, 3);
#endif

   uint prev_bits = 7;  
   
   for ( ; ; )
   {
      int code;
      uint total_bits;
      
#ifdef _XBOX  
      srcPrefetchState = Utils::UpdatePrefetchLargeStruct(srcPrefetchState, pInBuf, 3);
      dstPrefetchState = Utils::UpdatePrefetchLargeStruct(dstPrefetchState, pOutBuf, 3);
#endif  
                              
      for ( ; ; )
      {
         // Speculatively hide the microcoded shift.
         uint bits = (uint)(bitBuf >> prev_bits);
                                             
         if ((code = mLookUp1[bitBuf & 0xFF]) < 0)
         {
            uint temp = (uint)(bitBuf >> 8);
            do
            {
               code = mTree1[(-code - 1) + (temp & 1)];
               temp >>= 1;
            } while (code < 0);
         }
         
         uint num_bits = code >> 16;
         total_bits = num_bits;
         code &= 0xFFFF;
                  
         if (code >= 256)
            break;
                           
         // ----
         
         if (num_bits != prev_bits)
            bits = (uint)(bitBuf >> num_bits);
                           
         int code1;
         
         if ((code1 = mLookUp1[bits & 0xFF]) < 0)
         {
            uint temp = (uint)(bits >> 8);
            do
            {
               code1 = mTree1[(-code1 - 1) + (temp & 1)];
               temp >>= 1;
            } while (code1 < 0);
         }
                           
         prev_bits = code1 >> 16;
         total_bits += prev_bits;
         code1 &= 0xFFFF;
                  
         if (code1 >= 256)
         {
            outBufLeft--;
            if (outBufLeft < 0) 
            {
               outBufLeft++;
               STORE_BUF_LOCALS();
               error(INFL_STATUS_TOO_MUCH_ERROR);
            }
            *pOutBuf++ = (uchar)code;
                        
            code = code1;
            break;
         }
         
         bitBuf >>= total_bits;
         bitBufLen -= total_bits;
      
         outBufLeft -= 2;
         if (outBufLeft < 0)
         {
            outBufLeft += 2;
            STORE_BUF_LOCALS();
            error(INFL_STATUS_TOO_MUCH_ERROR);
         }
            
         pOutBuf[0] = (uchar)code;
         pOutBuf[1] = (uchar)code1;
         pOutBuf += 2;
                
         if (bitBufLen < 0)
         {
            READ_FOUR_BYTES(); 
         }            
      }
      
      REMOVE_BITS(total_bits);
            
      if (code == 256)
         break;

      int match_len = length_range[code - 257];

      if (code >= 265)
      {
         uint length_extra_bits;
         int num_length_extra_bits = length_extra[code - 257];
         GET_BITS(num_length_extra_bits, length_extra_bits);
         match_len += length_extra_bits;
      }

//      if ((match_len < INFL_MIN_MATCH) || (match_len > INFL_MAX_MATCH))
//         error(INFL_STATUS_BAD_CODE_ERROR);

      if (match_len > outBufLeft)
      {
         STORE_BUF_LOCALS();
         error(INFL_STATUS_TOO_MUCH_ERROR);
      }
         
      GET_SYMBOL(mLookUp2, mTree2, code);

      uint dist_extra_bits;
      int num_dist_extra_bits = dist_extra[code];
      GET_BITS(num_dist_extra_bits, dist_extra_bits);
      int match_dist = dist_range[code] + dist_extra_bits;
            
      uchar* dict_dst_ofs = pOutBuf;
      uchar* dict_src_ofs = pOutBuf - match_dist;
      if (dict_src_ofs < mpOutBufStart)
      {
         STORE_BUF_LOCALS();
         error(INFL_STATUS_BAD_CODE_ERROR);
      }
      
      if (match_len > match_dist)
      {
         if (match_dist == 1)
         {
            Utils::FastMemSet(dict_dst_ofs, dict_src_ofs[0], match_len);
         }
         else
         {
            for (int i = match_len; i > 0; i--)
               *dict_dst_ofs++ = *dict_src_ofs++;
         }               
      }
      else if (match_len > 8)
      {
         Utils::FastMemCpy(dict_dst_ofs, dict_src_ofs, match_len);
      }
      else
      {
         for (int i = match_len; i > 0; i--)
            *dict_dst_ofs++ = *dict_src_ofs++;
      }
      
      pOutBuf += match_len;
      outBufLeft -= match_len;
   }
   
   STORE_BUF_LOCALS();
   
   mBitBuf = bitBuf;
   mBitBufLen = bitBufLen;
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::decompress_block(void)
{
   build_huffman_decoder_tables(INFL_NUM_SYMBOLS_1, mCodeSize1, &mLookUp1, &mTree1);
   build_huffman_decoder_tables(INFL_NUM_SYMBOLS_2, mCodeSize2, &mLookUp2, &mTree2);

#if TRACE_RECORDING
   XTraceStartRecording("e:\\decompress.bin");
#endif   
      
   if (mBuffered)
      decompress_block_buffered();
   else
      decompress_block_unbuffered();
      
#if TRACE_RECORDING
   XTraceStopRecording();      
   DebugBreak();
#endif   
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::static_block(void)
{
   Utils::FastMemSet(mCodeSize1, 8, 0x90);
   Utils::FastMemSet(mCodeSize1 + 0x90, 9, 0x70);
   Utils::FastMemSet(mCodeSize1 + (0x90 + 0x70), 7, 0x18);
   Utils::FastMemSet(mCodeSize1 + (0x90 + 0x70 + 0x18), 8, 0x8);
   Utils::FastMemSet(mCodeSize2, 5, 0x20);

   decompress_block();
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::dynamic_block(void)
{
   int num_lit_codes, num_dst_codes, total_codes;
   signed char code_sizes[INFL_NUM_SYMBOLS_1 + INFL_NUM_SYMBOLS_2];

   Utils::FastMemSet(mCodeSize1, 0, INFL_NUM_SYMBOLS_1);
   Utils::FastMemSet(mCodeSize2, 0, INFL_NUM_SYMBOLS_2);
   Utils::FastMemSet(mCodeSize3, 0, INFL_NUM_SYMBOLS_3);

   if ((num_lit_codes = get_bits(5) + 257) > 286)
      error(INFL_STATUS_DYN_BLOCK_ERROR);

   if ((num_dst_codes = get_bits(5) + 1) > 30)
      error(INFL_STATUS_DYN_BLOCK_ERROR);

   total_codes = num_dst_codes + num_lit_codes;

   for (int i = 0, j = get_bits(4) + 4; i < j; i++)
      mCodeSize3[bit_length_order[i]] = (uchar)get_bits(3);

   build_huffman_decoder_tables(INFL_NUM_SYMBOLS_3, mCodeSize3, &mLookUp3, &mTree3);

   for (int cur_code = 0; cur_code < total_codes; )
   {
      int code_size = get_symbol(mLookUp3, mTree3), rep_len = 0;
      signed char rep_code_size = 0;

      if (code_size <= 15)
      {
         if ((cur_code + 1) > total_codes)
            error(INFL_STATUS_DYN_BLOCK_ERROR);

         code_sizes[cur_code++] = (uchar)code_size;
         continue;
      }

      switch (code_size)
      {
         case 16:
         {
            rep_len = get_bits(2) + 3;
            if (cur_code)
               rep_code_size = code_sizes[cur_code - 1];
            else
               rep_code_size = 0;
            break;
         }
         case 17:
         {
            rep_len = get_bits(3) + 3;
            rep_code_size = 0;
            break;
         }
         case 18:
         {
            rep_len = get_bits(7) + 11;
            rep_code_size = 0;
            break;
         }
      }

      if ((cur_code + rep_len) > total_codes)
         error(INFL_STATUS_DYN_BLOCK_ERROR);

      for ( ; rep_len; rep_len--)
         code_sizes[cur_code++] = rep_code_size;
   }

   Utils::FastMemCpy(mCodeSize1, code_sizes, num_lit_codes);
   Utils::FastMemCpy(mCodeSize2, code_sizes + num_lit_codes, num_dst_codes);

   decompress_block();
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::inflate_block(void)
{
   switch (get_bits(2))
   {
      case 0:
      {
         raw_block();
         break;
      }
      case 1:
      {
         static_block();
         break;
      }
      case 2:
      {
         dynamic_block();
         break;
      }
      case 3:
      {
         error(INFL_STATUS_BAD_BLOCK_ERROR);
      }
   }
}
//-----------------------------------------------------------------------------------------------------------
__declspec(noreturn) void BInflate::inflate_main(void)
{
   mBitBuf = get_dword();
   mBitBuf |= (((uint64)get_dword()) << 32U);
   mBitBufLen = 32;

   int last_block_flag;  

   do
   {
      last_block_flag = get_bits(1);

      inflate_block();

   } while (!last_block_flag);
   
   if (mBuffered)
      flushDict();

   remove_bits(mBitBufLen & 7);
   
   int numBytesInBitBuffer = ((mBitBufLen + 32) >> 3) - mNumEOFPadBytes;
   BDEBUG_ASSERT(numBytesInBitBuffer >= 0);
      
   *mPInBufBytes = (*mPInBufBytes - mInBufLeft);
   // This "gives back" the bits in the bitbuffer, but *mPInBufBytes could go negative in the input buffer is tiny!
   *mPInBufBytes -= numBytesInBitBuffer;
   
   *mPOutBufBytes = *mPOutBufBytes - mOutBufLeft;

   mSSD.setReturnStatus(INFL_STATUS_DONE);
   mSSD.ret();
   
   // Shouldn't reach here.
   abort();
}
//-----------------------------------------------------------------------------------------------------------
__declspec(noreturn) void BInflate::inflate_main_func(void* funcData)
{
   BInflate* pInflate = reinterpret_cast<BInflate*>(funcData);
   pInflate->inflate_main();
}
//-----------------------------------------------------------------------------------------------------------
BInflate::BInflate()
{
   mLookUp1 = mLookUp2 = mLookUp3 = NULL;

   mTree1 = mTree2 = mTree3 = NULL;
}
//-----------------------------------------------------------------------------------------------------------
BInflate::~BInflate()
{
   deinit();
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::deinit(void)
{
   mSSD.deinit();
   
   delete [] mLookUp1; mLookUp1 = NULL;
   delete [] mLookUp2; mLookUp2 = NULL;
   delete [] mLookUp3; mLookUp3 = NULL;
   delete [] mTree1; mTree1 = NULL;
   delete [] mTree2; mTree2 = NULL; 
   delete [] mTree3; mTree3 = NULL;
}
//-----------------------------------------------------------------------------------------------------------
void BInflate::init(bool useFastMemCpy, bool clearDictionary)
{
   deinit();
   
   if (clearDictionary)
      Utils::FastMemSet(mDict, 0, INFL_DICT_SIZE);
   
   mUseFastMemCpy = useFastMemCpy;
   
   mDictOfs = 0;
   
   mNumEOFPadBytes = 0;
         
   mSSD.init(inflate_main_func, reinterpret_cast<void*>(this));
}
//-----------------------------------------------------------------------------------------------------------
int BInflate::decompress(
   const uchar* pInBuf, int* pInBufBytes,
   uchar* pOutBuf, int* pOutBufBytes,
   bool eofFlag,
   bool buffered,
   uchar* pOutBufStart)
{
   BDEBUG_ASSERT(mSSD.getFiber());
   
   mEOFFlag = eofFlag;
   mBuffered = buffered;
   
   mPInBufBytes = pInBufBytes;
   mpInBuf = pInBuf;
   mInBufLeft = *pInBufBytes;
         
   mPOutBufBytes = pOutBufBytes;
   mpOutBufStart = pOutBufStart ? pOutBufStart : pOutBuf;
   mpOutBuf = pOutBuf;
   mOutBufLeft = *pOutBufBytes;
      
   return mSSD.begin();
}  
//-----------------------------------------------------------------------------------------------------------
int BInflate::decompressAll(
   BBufferRefiller& refiller,
   int& srcRead,
   uchar *pDst, int dstLen, int& dstWritten)
{
   int outBufOfs = 0;
     
   int status = INFL_STATUS_UNKNOWN_ERROR;
   
   srcRead = 0;
   do
   {
      const int cInBufSize = 4096;
      uchar inBuf[cInBufSize];
      
      int inBufLen = refiller(inBuf, cInBufSize);
      if (inBufLen < 0)
      {
         status = INFL_STATUS_UNKNOWN_ERROR;
         break;
      }
      
      int inBufOfs = 0;
      
      bool eofFlag = (0 == inBufLen);
      
      while ((inBufOfs < inBufLen) || (eofFlag))
      {
         eofFlag = (inBufLen == inBufOfs);
         
         int inBufBytes = inBufLen - inBufOfs;
         int outBufBytes = dstLen - outBufOfs;      
         status = decompress(inBuf + inBufOfs, &inBufBytes, pDst + outBufOfs, &outBufBytes, eofFlag, false, pDst);
         
         srcRead += inBufBytes;
         inBufOfs += inBufBytes;
         outBufOfs += outBufBytes;
         
         if (INFL_STATUS_OKAY != status)
            break;
      };
   } while ((INFL_STATUS_OKAY == status));
   
   dstWritten = outBufOfs;
   
   BDEBUG_ASSERT(srcRead >= 0);
   
   return status;
}
//-----------------------------------------------------------------------------------------------------------
int BInflate::decompressAll(
   const uchar* Psrc, int srcLen,
   uchar *pDst, int dstLen,
   int& srcRead,
   int& dstWritten)
{
   int srcBytes = srcLen;
   int dstBytes = dstLen;
   const int status = decompress(Psrc, &srcBytes, pDst, &dstBytes, true, false);

   srcRead = srcBytes;
   dstWritten = dstBytes;
   
   return status;
}

