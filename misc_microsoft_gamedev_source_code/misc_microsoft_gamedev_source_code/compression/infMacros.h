#undef READ_FOUR_BYTES
#undef REMOVE_BITS
#undef GET_BITS
#undef GET_SYMBOL
//-----------------------------------------------------------------------------------------------------------
#define READ_FOUR_BYTES() do  \
{                             \
   uint64 newBits;            \
   if ((inBufLeft -= 4) < 0)  \
   {                          \
      inBufLeft += 4;         \
      STORE_BUF_LOCALS();     \
      newBits = get_byte();   \
      newBits |= (get_byte() << 8); \
      newBits |= (get_byte() << 16); \
      newBits |= (get_byte() << 24); \
      LOAD_BUF_LOCALS();      \
   }                          \
   else                       \
   {                          \
      newBits = READ_LE_DWORD(pInBuf); \
      pInBuf += 4;            \
   }                          \
   bitBufLen += 32;           \
   bitBuf |= (((uint64)newBits) << bitBufLen); \
} while(0)
//-----------------------------------------------------------------------------------------------------------
#define REMOVE_BITS(num_bits) do \
{                                \
   bitBuf >>= (num_bits);        \
   bitBufLen -= (num_bits);      \
   if (bitBufLen < 0)            \
      READ_FOUR_BYTES();         \
} while(0)
//-----------------------------------------------------------------------------------------------------------  
#define GET_BITS(num_bits, bits) do \
{                                   \
   bits = (uint)(bitBuf & bitmasks[num_bits]); \
   REMOVE_BITS(num_bits);           \
} while(0)
//-----------------------------------------------------------------------------------------------------------
#define GET_SYMBOL(look_up, tree, symbol) do          \
{                                                     \
   if ((symbol = look_up[bitBuf & 0xFF]) < 0)         \
   {                                                  \
      uint temp = (uint)(bitBuf >> 8);                \
      do                                              \
      {                                               \
         symbol = tree[(-symbol - 1) + (temp & 1)];   \
         temp >>= 1;                                  \
      } while (symbol < 0);                           \
   }                                                  \
   uint num_bits = symbol >> 16;                      \
   symbol &= 0xFFFF;                                  \
   REMOVE_BITS(num_bits);                             \
} while(0)
//-----------------------------------------------------------------------------------------------------------