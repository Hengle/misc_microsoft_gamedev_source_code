//------------------------------------------------------------------------------
// inlines-- moved from .h file for clarity
//------------------------------------------------------------------------------
// Retrieve one character from the input stream.
inline uint jpeg_decoder::get_char(void)
{
  // Any bytes remaining in buffer?
  if (!in_buf_left)
  {
    // Try to get more bytes.
    prep_in_buffer();
    // Still nothing to get?
    if (!in_buf_left)
    {
      // Padd the end of the stream with 0xFF 0xD9 (EOI marker)
      // FIXME: Is there a better padding pattern to use?
      int t = tem_flag;
      tem_flag ^= 1;
      if (t)
        return (0xD9);
      else
        return (0xFF);
    }
  }

  uint c = *Pin_buf_ofs++;
  in_buf_left--;

  return (c);
}
//------------------------------------------------------------------------------
// Same as previus method, except can indicate if the character is
// a "padd" character or not.
inline uint jpeg_decoder::get_char(bool *Ppadding_flag)
{
  if (!in_buf_left)
  {
    prep_in_buffer();
    if (!in_buf_left)
    {
      *Ppadding_flag = true;
      int t = tem_flag;
      tem_flag ^= 1;
      if (t)
        return (0xD9);
      else
        return (0xFF);
    }
  }

  *Ppadding_flag = false;

  uint c = *Pin_buf_ofs++;
  in_buf_left--;

  return (c);
}
//------------------------------------------------------------------------------
// Inserts a previously retrieved character back into the input buffer.
inline void jpeg_decoder::stuff_char(uchar q)
{
  *(--Pin_buf_ofs) = q;
  in_buf_left++;
}
//------------------------------------------------------------------------------
// Retrieves one character from the input stream, but does
// not read past markers. Will continue to return 0xFF when a
// marker is encountered.
// FIXME: Bad name?
inline uchar jpeg_decoder::get_octet(void)
{
  bool padding_flag;
  int c = get_char(&padding_flag);

  if (c == 0xFF)
  {
    if (padding_flag)
      return (0xFF);

    c = get_char(&padding_flag);
    if (padding_flag)
    {
      stuff_char(0xFF);
      return (0xFF);
    }

    if (c == 0x00)
      return (0xFF);
    else
    {
      stuff_char(static_cast<uchar>(c));
      stuff_char(0xFF);
      return (0xFF);
    }
  }

  return static_cast<uchar>(c);
}
//------------------------------------------------------------------------------
// Retrieves a variable number of bits from the input stream.
// Does not recognize markers.
inline uint jpeg_decoder::get_bits_1(int num_bits)
{
	if (!num_bits)
		return 0;

	uint i = bit_buf >> (32 - num_bits);

  if ((bits_left -= num_bits) <= 0)
  {
    bit_buf <<= (num_bits += bits_left);
		
		uint c1 = get_char();
		uint c2 = get_char();
		bit_buf = (bit_buf & 0xFFFF0000) | (c1 << 8) | c2;
		
    bit_buf <<= -bits_left;

    bits_left += 16;

		assert(bits_left >= 0);
  }
  else
		bit_buf <<= num_bits;

  return i;	
}
//------------------------------------------------------------------------------
// Retrieves a variable number of bits from the input stream.
// Markers will not be read into the input bit buffer. Instead,
// an infinite number of all 1's will be returned when a marker
// is encountered.
// FIXME: Is it better to return all 0's instead, like the older implementation?
inline uint jpeg_decoder::get_bits_2(int num_bits)
{
	if (!num_bits)
		return 0;

	uint i = bit_buf >> (32 - num_bits);

  if ((bits_left -= num_bits) <= 0)
  {
    bit_buf <<= (num_bits += bits_left);
		
		if ((in_buf_left < 2) || (Pin_buf_ofs[0] == 0xFF) || (Pin_buf_ofs[1] == 0xFF))
		{		
	    uint c1 = get_octet();
			uint c2 = get_octet();
			bit_buf |= (c1 << 8) | c2;
		}
		else
		{
			bit_buf |= ((uint)Pin_buf_ofs[0] << 8) | Pin_buf_ofs[1];
			in_buf_left -= 2;	
			Pin_buf_ofs += 2;
		}

    bit_buf <<= -bits_left;

    bits_left += 16;

		assert(bits_left >= 0);
  }
  else
		bit_buf <<= num_bits;

  return i;
}
//------------------------------------------------------------------------------
// Decodes a Huffman encoded symbol.
inline int jpeg_decoder::huff_decode(Phuff_tables_t Ph)
{
  int symbol;

  // Check first 8-bits: do we have a complete symbol?
  if ((symbol = Ph->look_up[bit_buf >> 24]) < 0)
  {
    // Decode more bits, use a tree traversal to find symbol.
		int ofs = 23;
		do
    {
      symbol = Ph->tree[-(int)(symbol + ((bit_buf >> ofs) & 1))];
			ofs--;
    } while (symbol < 0);

		get_bits_2(8 + (23 - ofs));
  }
  else
    get_bits_2(Ph->code_size[symbol]);

  return symbol;
}
//------------------------------------------------------------------------------
// Decodes a Huffman encoded symbol.
inline int jpeg_decoder::huff_decode(Phuff_tables_t Ph, int& extra_bits)
{
  int symbol;

  // Check first 8-bits: do we have a complete symbol?
  if ((symbol = Ph->look_up2[bit_buf >> 24]) < 0)
  {
    // Use a tree traversal to find symbol.
		int ofs = 23;
		do
    {
      symbol = Ph->tree[-(int)(symbol + ((bit_buf >> ofs) & 1))];
			ofs--;
    } while (symbol < 0);

		get_bits_2(8 + (23 - ofs));
		
		extra_bits = get_bits_2(symbol & 0xF);
  }
  else
	{
		assert(((symbol >> 8) & 31) == Ph->code_size[symbol & 255] + ((symbol & 0x8000) ? (symbol & 15) : 0));

		if (symbol & 0x8000)
		{
			get_bits_2((symbol >> 8) & 31);
			extra_bits = symbol >> 16;
		}
		else 
		{
			int code_size = (symbol >> 8) & 31;
			int num_extra_bits = symbol & 0xF;
			int bits = code_size + num_extra_bits;
			if (bits <= (bits_left + 16))
				extra_bits = get_bits_2(bits) & ((1 << num_extra_bits) - 1);
			else
			{
				get_bits_2(code_size);
				extra_bits = get_bits_2(num_extra_bits);
			}
		}
					
		symbol &= 0xFF;
	}

  return symbol;
}
//------------------------------------------------------------------------------
// Tables and macro used to fully decode the DPCM differences.
// (Note: In x86 asm this can be done without using tables.)
const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

// used by huff_extend()
const int extend_mask[] =
{
  0,
  (1<<0), (1<<1), (1<<2), (1<<3),
  (1<<4), (1<<5), (1<<6), (1<<7),
  (1<<8), (1<<9), (1<<10), (1<<11),
  (1<<12), (1<<13), (1<<14), (1<<15),
  (1<<16),
};

#define HUFF_EXTEND_TBL(x,s) ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

#ifdef SUPPORT_X86ASM
// Use the inline ASM version instead to prevent jump misprediction issues
  #define HUFF_EXTEND(x,s) huff_extend(x, s)
  #define HUFF_EXTEND_P(x,s) Pd->huff_extend(x, s)
#else
  #define HUFF_EXTEND(x,s) HUFF_EXTEND_TBL(x,s)
  #define HUFF_EXTEND_P(x,s) HUFF_EXTEND_TBL(x,s)
#endif
//------------------------------------------------------------------------------
#ifdef SUPPORT_X86ASM
// This code converts the raw unsigned coefficient bits
// read from the data stream to the proper signed range.
// There are many ways of doing this, see the HUFF_EXTEND_TBL
// macro for an alternative way.
// It purposelly avoids any decision making that requires jumping.
inline uint jpeg_decoder::huff_extend(uint i, int c)
{
  _asm
  {
    mov ecx, c
    mov eax, i
    cmp eax, [ecx*4+extend_mask]
    sbb edx, edx
    shl edx, cl
    adc eax, edx
  }
}
#endif
//------------------------------------------------------------------------------
// Clamps a value between 0-255.
inline uchar jpeg_decoder::clamp(int i)
{
  if (i & 0xFFFFFF00)
    i = (((~i) >> 31) & 0xFF);

  return static_cast<uchar>(i);
}
//------------------------------------------------------------------------------

