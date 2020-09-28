// fastInflator.cpp    
// Deprecated! Use the regular inflator now on 360. 
#include "xcore.h"

#include "fastInflator.h"

namespace 
{
   template<class T>

   static T DebugRangeCheck(T i, T l, T h)
   {
      l;
      h;
      assert(i >= l);
      assert(i < h);
      return i;
   }

   static const int LOOK_UP_BITS = 9;      // 9 or 10 is the sweet spot
   static const int LOOK_UP_NUM = 1 << LOOK_UP_BITS;
   static const int LOOK_UP_MASK = LOOK_UP_NUM - 1;
   
   static const int MIN_MATCH = 3;
   static const int MAX_MATCH = 258;
   static const int DICT_SIZE = 32768;
   
   static const uchar bit_length_order[] = { 16, 17, 18, 0, 8,  7,  9, 6, 10,  5, 11, 4, 12,  3, 13, 2, 14,  1, 15 };
   static const int length_extra[] = { 0,0,0,0, 0,0,0,0, 1,1,1,1, 2,2,2,2, 3,3,3,3, 4,4,4,4, 5,5,5,5, 0,0,0,0 };
   static const int length_range[] = { 3,4,5,6, 7,8,9,10, 11,13,15,17, 19,23,27,31, 35,43,51,59, 67,83,99,115, 131,163,195,227, 258,0,0,0 };
   static const int dist_extra[] = { 0,0,0,0, 1,1,2,2, 3,3,4,4, 5,5,6,6, 7,7,8,8, 9,9,10,10, 11,11,12,12, 13,13,0,0 };
   static const int dist_range[] = { 1,2,3,4, 5,7,9,13, 17,25,33,49, 65,97,129,193, 257,385,513,769, 1025,1537,2049,3073, 4097,6145,8193,12289, 16385, 24577, 0, 0 };
   
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

   static inline uint bitmask(int bits)
   {
      return bitmasks[DebugRangeCheck(bits, 0, 32)];
   }
}
//-----------------------------------------------------------------------------
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
//-----------------------------------------------------------------------------
void BFastInflator::build_huffman_decoder_tables(
   int num_symbols, char* RESTRICT code_size,
  Look_Up_Type** RESTRICT _look_up, Tree_Type** RESTRICT _tree)
{
  int status = 0;
  uint i, j;
  uint total;
  uint old_code, new_code;
  int next_free_entry, current_entry;
  int num_codes[17];
  uint next_code[17];
  uint *code = NULL;
  Look_Up_Type* look_up = NULL;
  Tree_Type* tree = NULL;

  delete [] *_look_up; *_look_up = NULL;
  delete [] *_tree; *_tree = NULL;

  if ( ((code    = new uint[num_symbols]) == NULL) ||
       ((look_up = new Look_Up_Type[LOOK_UP_NUM]) == NULL) ||
       ((tree    = new Tree_Type[num_symbols * 2]) == NULL) )
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

  for (i = 0; i < LOOK_UP_NUM; i++)
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

      if (code_size[i] <= LOOK_UP_BITS)
      {
        j = 1 << code_size[i];

        while (new_code < LOOK_UP_NUM)
        {
          look_up[new_code] = (Look_Up_Type)i;

          new_code += j;
        }
      }
      else
      {
        if ((current_entry = look_up[new_code & LOOK_UP_MASK]) == 0)
        {
          look_up[new_code & LOOK_UP_MASK] = (Look_Up_Type)next_free_entry;
          current_entry = next_free_entry;

          next_free_entry -= 2;
        }

        new_code >>= (LOOK_UP_BITS - 1);

        for (j = code_size[i]; j > (LOOK_UP_BITS + 1); j--)
        {
          current_entry -= ((new_code >>= 1) & 1);

          if (tree[-current_entry - 1] == 0)
          {
            tree[-current_entry - 1] = (Look_Up_Type)next_free_entry;

            current_entry = next_free_entry;

            next_free_entry -= 2;
          }
          else
            current_entry = tree[-current_entry - 1];
        }

        current_entry -= ((new_code >>= 1) & 1);

        tree[-current_entry - 1] = (Look_Up_Type)i;
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

  status = INFLATOR_MEM_ERROR;
  goto exit;

code_set_error:

  status = INFLATOR_CODE_SET_ERROR;
  goto exit;
}
//-----------------------------------------------------------------------------
__declspec(noreturn) void BFastInflator::error(int status)
{
   trace("BFastInflator:: decompression failed, status %i", status);
      
   if (!Perror_handler)
      exit(EXIT_FAILURE);

   (*Perror_handler)(status);
   
   // Shouldn't reach here.
   abort();
}
//-----------------------------------------------------------------------------
inline void BFastInflator::remove_bits(int num_bits)
{
   in_buf_bit_ofs += num_bits;
   in_buf_cur_ofs += (in_buf_bit_ofs >> 3);
   in_buf_bit_ofs &= 7;
}
//-----------------------------------------------------------------------------
inline uint BFastInflator::peek_bits(int num_bits) // must be 25 or less
{
#ifdef XBOX
   const uint i = __loadwordbytereverse(0, in_buf_cur_ofs);
#else   
   const uint i = in_buf_cur_ofs[0] | (in_buf_cur_ofs[1] << 8) | (in_buf_cur_ofs[2] << 16) | (in_buf_cur_ofs[3] << 24);
#endif
   return (i >> in_buf_bit_ofs) & bitmask(num_bits); 
}
//-----------------------------------------------------------------------------
inline uint BFastInflator::peek_bits(void)
{
#ifdef XBOX
   const uint i = __loadwordbytereverse(0, in_buf_cur_ofs);
#else
   const uint i = in_buf_cur_ofs[0] | (in_buf_cur_ofs[1] << 8) | (in_buf_cur_ofs[2] << 16) | (in_buf_cur_ofs[3] << 24);   
#endif   
   return i >> in_buf_bit_ofs;
}
//-----------------------------------------------------------------------------
inline uint64 BFastInflator::peek_bits64(void)
{
#ifdef XBOX
   const uint64 i = __loadwordbytereverse(0, in_buf_cur_ofs) | ((uint64)__loadwordbytereverse(4, in_buf_cur_ofs) << 32);
#else 
   const uint64 i = ((uint64)in_buf_cur_ofs[0]) | ((uint64)(in_buf_cur_ofs[1]) << 8U) | ((uint64)(in_buf_cur_ofs[2]) << 16U) | ((uint64)(in_buf_cur_ofs[3]) << 24U) |
      ((uint64)(in_buf_cur_ofs[4]) << 32U) | ((uint64)(in_buf_cur_ofs[5]) << 40U) | ((uint64)(in_buf_cur_ofs[6]) << 48U) | ((uint64)(in_buf_cur_ofs[7]) << 56U);
#endif   
   return i >> in_buf_bit_ofs;
}
//-----------------------------------------------------------------------------
inline uint BFastInflator::get_bits(int num_bits)
{
   uint bits = peek_bits(num_bits);
   remove_bits(num_bits);
   return bits;
}
//-----------------------------------------------------------------------------
void BFastInflator::align_to_byte_boundary(void)
{
   remove_bits((8 - in_buf_bit_ofs) & 7);
}
//-----------------------------------------------------------------------------
void BFastInflator::decompress_block(void)
{
  build_huffman_decoder_tables(INFLATOR_NUM_SYMBOLS_1, code_size_1, &look_up_1, &tree_1);
  build_huffman_decoder_tables(INFLATOR_NUM_SYMBOLS_2, code_size_2, &look_up_2, &tree_2);

  const uchar* RESTRICT init_out_buf_cur_ofs = out_buf_cur_ofs;
  
  for ( ; ; )
  {
      uint64 bit_buf;
      int symbol;

#ifdef XBOX            
      __dcbt(256, in_buf_cur_ofs);               
#endif      
                              
      for ( ; ; )
      {
         bit_buf = peek_bits64(); // will return at least 57 bits

         symbol = look_up_1[bit_buf & LOOK_UP_MASK];
         if (symbol < 0)   // relatively rare
         {
            int temp = (int)(bit_buf >> (LOOK_UP_BITS - 1));
            do {
               symbol = tree_1[~symbol + ((temp >>= 1) & 1)];
            } while (symbol < 0);
         }
         
         uint code_size = code_size_1[symbol];
         uint total_bits = code_size;
         bit_buf >>= code_size;
                  
         if (symbol > 255)
         {
            remove_bits(total_bits); 
            break;
         }
                  
         out_buf_cur_ofs[0] = (uchar)symbol;
         
         //----

         symbol = look_up_1[bit_buf & LOOK_UP_MASK];
         if (symbol < 0)   // relatively rare
         {
            int temp = (int)(bit_buf >> (LOOK_UP_BITS - 1));
            do {
               symbol = tree_1[~symbol + ((temp >>= 1) & 1)];
            } while (symbol < 0);
         }

         code_size = code_size_1[symbol];
         total_bits += code_size;
         bit_buf >>= code_size;
         
         if (symbol > 255)
         {
            remove_bits(total_bits); 
            out_buf_cur_ofs += 1;
            break;
         }

         out_buf_cur_ofs[1] = (uchar)symbol;
                                                                               
         //----
         
         symbol = look_up_1[bit_buf & LOOK_UP_MASK];
         if (symbol < 0)   // relatively rare
         {
            int temp = (int)(bit_buf >> (LOOK_UP_BITS - 1));
            do {
               symbol = tree_1[~symbol + ((temp >>= 1) & 1)];
            } while (symbol < 0);
         }

         code_size = code_size_1[symbol];
         total_bits += code_size;
         
         remove_bits(total_bits); 
         
         if (symbol > 255)
         {
            bit_buf >>= code_size;
            out_buf_cur_ofs += 2;
            break;
         }

         out_buf_cur_ofs[2] = (uchar)symbol;
         out_buf_cur_ofs += 3;
      }         

      if (symbol == 0x100)
         break;

      int num_extra_bits = length_extra[DebugRangeCheck<int>(symbol - 257, 0, ARRAY_SIZE(length_extra))];
      int match_len = length_range[DebugRangeCheck<int>(symbol - 257, 0, ARRAY_SIZE(length_range))] + 
         (int)(bit_buf & bitmask(num_extra_bits)); 
      
      // code_size + num_extra_bits will always be <= (15+5)
      remove_bits(num_extra_bits);

      bit_buf = peek_bits();  // will return at least 25 bits

      symbol = look_up_2[bit_buf & LOOK_UP_MASK];
      if (symbol < 0)   // relatively rare
      {
         int temp = (int)(bit_buf >> (LOOK_UP_BITS - 1));
         do {
            symbol = tree_2[~symbol + ((temp >>= 1) & 1)];
         } while (symbol < 0);
      }

      num_extra_bits = dist_extra[DebugRangeCheck<int>(symbol, 0, ARRAY_SIZE(dist_extra))];
      int match_dist = dist_range[DebugRangeCheck<int>(symbol, 0, ARRAY_SIZE(dist_range))];
                  
      // could special case to directly read from bit_buf if 
      // code_size+num_extra_bits<=25 (is it worth it?)
      uint code_size = code_size_2[symbol];
      if ((code_size + num_extra_bits) <= 25)
      {
         remove_bits(code_size + num_extra_bits);
         match_dist += (int)((bit_buf >> code_size) & bitmask(num_extra_bits));
      }
      else
      {
         // very rare
         remove_bits(code_size);
         match_dist += get_bits(num_extra_bits);
      }

      const uchar* RESTRICT dict_src_ofs = out_buf_cur_ofs - match_dist;                
               
      if (match_len > match_dist)
      {
         if (match_dist == 1)
         {
            Utils::FastMemSet(out_buf_cur_ofs, dict_src_ofs[0], match_len);
            out_buf_cur_ofs += match_len;
         }
         else
         {
            for ( ; match_len; match_len--)
               *out_buf_cur_ofs++ = *dict_src_ofs++;
         }               
      }
      else 
      {
         Utils::FastMemCpy(out_buf_cur_ofs, dict_src_ofs, match_len);
         out_buf_cur_ofs += match_len;
      }
   }

   out_buf_left -= (out_buf_cur_ofs - init_out_buf_cur_ofs);
   if (out_buf_left < 0)
   {
      // The stream was corrupted, and memory has been overwritten!
      error(INFLATOR_TOO_MUCH_ERROR);
   }
}
//-----------------------------------------------------------------------------
void BFastInflator::static_block(void)
{
   Utils::FastMemSet(code_size_1, 8, 0x90);
   Utils::FastMemSet(code_size_1 + 0x90, 9, 0x70);
   Utils::FastMemSet(code_size_1 + (0x90 + 0x70), 7, 0x18);
   Utils::FastMemSet(code_size_1 + (0x90 + 0x70 + 0x18), 8, 0x8);
   Utils::FastMemSet(code_size_2, 5, 0x20);

   decompress_block();
}
//-----------------------------------------------------------------------------
inline int BFastInflator::get_symbol(
   char* RESTRICT Pcode_size, 
   Look_Up_Type* RESTRICT Plook_up, 
   Tree_Type* RESTRICT Ptree)
{
   int symbol;

   uint bit_buf = peek_bits();

   if ((symbol = Plook_up[bit_buf & LOOK_UP_MASK]) >= 0)
   {
      remove_bits(Pcode_size[symbol]);
      return (symbol);
   }

   int ofs = LOOK_UP_BITS;
   do {
      symbol = Ptree[~symbol + ((bit_buf >> ofs++) & 1)];
   } while (symbol < 0);

   remove_bits(ofs);
   return symbol;
}
//-----------------------------------------------------------------------------
void BFastInflator::dynamic_block(void)
{
   int num_lit_codes, num_dst_codes, total_codes;
   char code_sizes[INFLATOR_NUM_SYMBOLS_1 + INFLATOR_NUM_SYMBOLS_2];

   Utils::FastMemSet(code_size_1, 0, INFLATOR_NUM_SYMBOLS_1);
   Utils::FastMemSet(code_size_2, 0, INFLATOR_NUM_SYMBOLS_2);
   Utils::FastMemSet(code_size_3, 0, INFLATOR_NUM_SYMBOLS_3);

   if ((num_lit_codes = get_bits(5) + 257) > 286)
      error(INFLATOR_DYN_BLOCK_ERROR);

   if ((num_dst_codes = get_bits(5) + 1) > 30)
      error(INFLATOR_DYN_BLOCK_ERROR);

   total_codes = num_dst_codes + num_lit_codes;

   for (int i = 0, j = get_bits(4) + 4; i < j; i++)
      code_size_3[bit_length_order[DebugRangeCheck<int>(i, 0, ARRAY_SIZE(bit_length_order))]] = (uchar)get_bits(3);

   build_huffman_decoder_tables(INFLATOR_NUM_SYMBOLS_3, code_size_3, &look_up_3, &tree_3);

   for (int cur_code = 0; cur_code < total_codes; )
   {
      int code_size = get_symbol(code_size_3, look_up_3, tree_3), rep_len = 0;
      signed char rep_code_size = 0;

      if (code_size <= 15)
      {
         if ((cur_code + 1) > total_codes)
            error(INFLATOR_DYN_BLOCK_ERROR);
               
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
         error(INFLATOR_DYN_BLOCK_ERROR);

      for ( ; rep_len; rep_len--)
         code_sizes[cur_code++] = rep_code_size;
   }

   Utils::FastMemCpy(code_size_1, code_sizes, num_lit_codes);
   Utils::FastMemCpy(code_size_2, code_sizes + num_lit_codes, num_dst_codes);

   decompress_block();
}
//-----------------------------------------------------------------------------
void BFastInflator::raw_block(void)
{
   uint block_len, not_block_len;

   align_to_byte_boundary();

   block_len = get_bits(16);
   not_block_len = get_bits(16);

   if (((~block_len) & 0xFFFF) != not_block_len)
      error(INFLATOR_RAW_BLOCK_ERROR);

   out_buf_left -= block_len;
   if (out_buf_left < 0)
      error(INFLATOR_TOO_MUCH_ERROR);

   Utils::FastMemCpy(out_buf_cur_ofs, in_buf_cur_ofs, block_len);
   in_buf_cur_ofs += block_len;
   out_buf_cur_ofs += block_len;
}
//-----------------------------------------------------------------------------
void BFastInflator::inflate_block(void)
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
         error(INFLATOR_BAD_BLOCK_ERROR);
      }
   }
}
//-----------------------------------------------------------------------------
uint BFastInflator::inflator_main(void)
{
  int last_block_flag;  

   do
   {
      last_block_flag = get_bits(1);

      inflate_block();

   } while (!last_block_flag);
      
   return out_buf_cur_ofs - Pout_buf;
}
//-----------------------------------------------------------------------------
BFastInflator::BFastInflator() 
{
   Perror_handler = NULL;
   look_up_1 = look_up_2 = look_up_3 = NULL;
   tree_1 = tree_2 = tree_3 = NULL;
}
//-----------------------------------------------------------------------------
BFastInflator::~BFastInflator()
{
   delete [] look_up_1; 
   delete [] look_up_2; 
   delete [] look_up_3; 
   delete [] tree_1;    
   delete [] tree_2;    
   delete [] tree_3;    
}
//-----------------------------------------------------------------------------
uint BFastInflator::decompress(const uchar* RESTRICT Psrc, int src_len, uchar* RESTRICT Pdst, int dst_len, BErrorHandler& error_handler) 
{
   src_len;
   
   Perror_handler = &error_handler;

   Pin_buf = Psrc;
   in_buf_cur_ofs = Psrc;
   in_buf_bit_ofs = 0;

   Pout_buf = Pdst;
   out_buf_cur_ofs = Pdst;
   out_buf_left = dst_len;
   
   uint bytesDecompressed = inflator_main();

   Perror_handler = NULL;
   
   return bytesDecompressed;
}
//-----------------------------------------------------------------------------



