// jpgwrite.cpp                                                              

/*----------------------------------------------------------------------------*/
#include "jpge.h"
#include "jpghuff.h"
#include "fdct.h"
/*----------------------------------------------------------------------------*/
//#define SCALED_DCT
/*----------------------------------------------------------------------------*/
namespace {

  const int Y_B   = (0);
  const int CB_B  = ( Y_B + 256);
  const int CR_B  = (CB_B + 256);

  const int Y_G   = (CR_B + 256);
  const int CB_G  = ( Y_G + 256);
  const int CR_G  = (CB_G + 256);

  const int Y_R   = (CR_G + 256);
  const int CB_R  = ( Y_R + 256);
  const int CR_R  = (CB_R + 256);

  int32 FIX(float x)
  {
    return (int32)((x * 65536) + .5f);
  }

  int clamp(int i)
  {
//    if (i & 0xFFFFFF00)
//      return ((~i) >> 31) & 0xFF;
//    return i;
    return i;
  }

  void RGB_to_YCC(uchar* Pdst, const uchar *src, const long *xlt, int num_pixels)
  {
    while (num_pixels)
    {
      int r = src[0];
      int g = src[1];
      int b = src[2];

      int y  = clamp((xlt[ Y_B + b] + xlt[ Y_G + g] + xlt[ Y_R + r]) >> 16);
      int cb = clamp((xlt[CB_B + b] + xlt[CB_G + g] + xlt[CB_R + r]) >> 16);
      int cr = clamp((xlt[CR_B + b] + xlt[CR_G + g] + xlt[CR_R + r]) >> 16);

      Pdst[0] = (uchar)y;
      Pdst[1] = (uchar)cb;
      Pdst[2] = (uchar)cr;
      Pdst += 3;

      src += 3;
      num_pixels--;
    }
  }

  void RGB_to_Y(uchar* Pdst, const uchar *src, const long *xlt, int num_pixels)
  {
    while (num_pixels)
    {
      int r = src[0];
      int g = src[1];
      int b = src[2];

      int y  = clamp((xlt[Y_B + b] + xlt[Y_G + g] + xlt[Y_R + r]) >> 16);

      *Pdst++ = (uchar)y;

      src += 3;
      num_pixels--;
    }
  }

  void Y_to_YCC(uchar* Pdst, const uchar* src, int num_pixels)
  {
    while (num_pixels)
    {
      Pdst[0] = src[0];
      Pdst[1] = 0;
      Pdst[2] = 0;
      Pdst += 3;

      src++;
      num_pixels--;
    }
  }

  const int OUT_BUF_SIZE = 2048;
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_marker(int marker)
{
  m_stream.put(uchar(0xFF));
  m_stream.put(uchar(marker));
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_byte(uchar i)
{
  m_stream.put(i);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_word(uint i)
{
  m_stream.put(uchar(i >> 8));
  m_stream.put(uchar(i & 0xFF));

  //bio_char_write(handle, i >> 8);
  //bio_char_write(handle, i & 0xFF);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_jfif_app0(void)
{
  emit_marker(M_APP0);
  emit_word(2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1);
  emit_byte(0x4A);   /* Identifier: ASCII "JFIF" */
  emit_byte(0x46);
  emit_byte(0x49);
  emit_byte(0x46);
  emit_byte(0);
  emit_byte(1);      /* Major version */
  emit_byte(1);      /* Minor version */
  emit_byte(0);      /* Density unit */
  emit_word(1);
  emit_word(1);
  emit_byte(0);      /* No thumbnail image */
  emit_byte(0);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_dqt(void)
{
  int i, j;

  for (i = 0; i < ((num_components == 3) ? 2 : 1); i++)
  {
    emit_marker(M_DQT);
    
    bool byteValues = true;
    for (j = 0; j < 64; j++)
    {
       if (quantization_tables[i][j] > 255)
       {
         byteValues = false;
         break;
       }
    }
    
    if (byteValues)
    {
      emit_word(64 + 1 + 2);
      emit_byte(static_cast<uchar>(i));
      
      for (j = 0; j < 64; j++)
         emit_byte(static_cast<uchar>(quantization_tables[i][j]));
    }
    else
    {
       emit_word(64 * 2 + 1 + 2);
       emit_byte(static_cast<uchar>(i | 0x10));
       
       for (j = 0; j < 64; j++)
          emit_word(quantization_tables[i][j]);
    }
    
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_sof(void)
{
  int i;

  emit_marker(M_SOF0);                           /* baseline */
  emit_word(3 * num_components + 2 + 5 + 1);
  emit_byte(8);                                  /* precision */
  emit_word(image_y);
  emit_word(image_x);
  emit_byte(num_components);

  for (i = 0; i < num_components; i++)
  {
    emit_byte(static_cast<uchar>(i + 1));                                   /* component ID     */
    emit_byte(static_cast<uchar>((comp_h_samp[i] << 4) + comp_v_samp[i]));  /* h and v sampling */
    emit_byte(static_cast<uchar>(i > 0));                                   /* quant. table num */
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_dht(uchar *bits, uchar *val, int index, int ac_flag)
{
  int i, length;

  emit_marker(M_DHT);

  length = 0;

  for (i = 1; i <= 16; i++)
    length += bits[i];

  emit_word(length + 2 + 1 + 16);
  emit_byte(static_cast<uchar>(index + (ac_flag << 4)));

  for (i = 1; i <= 16; i++)
    emit_byte(bits[i]);

  for (i = 0; i < length; i++)
    emit_byte(val[i]);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_dhts(void)
{
  emit_dht(huff_bits[0+0], huff_val[0+0], 0, FALSE);
  emit_dht(huff_bits[2+0], huff_val[2+0], 0, TRUE);

  if (num_components == 3)
  {
    emit_dht(huff_bits[0+1], huff_val[0+1], 1, FALSE);
    emit_dht(huff_bits[2+1], huff_val[2+1], 1, TRUE);
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_sos(void)
{
  int i;

  emit_marker(M_SOS);
  emit_word(2 * num_components + 2 + 1 + 3);
  emit_byte(num_components);

  for (i = 0; i < num_components; i++)
  {
    emit_byte(static_cast<uchar>(i + 1));

    if (i == 0)
      emit_byte((0 << 4) + 0);
    else
      emit_byte((1 << 4) + 1);
  }

  emit_byte(0);     /* spectral selection */
  emit_byte(63);
  emit_byte(0);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::emit_markers(void)
{
  emit_marker(M_SOI);

  emit_jfif_app0();

  emit_dqt();

  emit_sof();

  emit_dhts();

  emit_sos();
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::compute_huffman_table(
  uint * *codes,
  uchar * *code_sizes,
  uchar *bits,
  uchar *val)
{
  int p, i, l, last_p, si;
  uchar huff_size[257];
  uint huff_code[257];
  uint code;

  p = 0;

  for (l = 1; l <= 16; l++)
    for (i = 1; i <= bits[l]; i++)
      huff_size[p++] = (char)l;

  huff_size[p] = 0; last_p = p;

  code = 0; si = huff_size[0]; p = 0;

  while (huff_size[p])
  {
    while (huff_size[p] == si)
      huff_code[p++] = code++;

    code <<= 1;
    si++;
  }

  for (p = 0, i = 0; p < last_p; p++)
    i = max(i, val[p]);

  *codes      = reinterpret_cast<uint*>(cmalloc_msg((i + 1) * sizeof(uint)));
  *code_sizes = reinterpret_cast<uchar*>(cmalloc_msg((i + 1) * sizeof(uchar)));

  for (p = 0; p < last_p; p++)
  {
    (*codes)[val[p]]      = huff_code[p];
    (*code_sizes)[val[p]] = huff_size[p];
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::compute_quant_table(int32 *dst, int16 *src)
{
  int i;
  long q;

  if (m_params.quality < 50)
    q = 5000 / m_params.quality;
  else
    q = 200 - m_params.quality * 2;

  for (i = 0; i < 64; i++)
  {
    long j = *src++;

    j = (j * q + 50L) / 100L;

    if (j < 1)
      j = 1;
    else if (j > 255)
      j = 255;

    *dst++ = j;
  }
}
/*----------------------------------------------------------------------------*/
static int32 dct_scale[64] =
{
 1048576, 1454417, 1370031, 1232995, 1048576,  823861,  567485,  289301,
 1454417, 2017334, 1900287, 1710213, 1454417, 1142728,  787125,  401273,
 1370031, 1900287, 1790031, 1610985, 1370031, 1076426,  741455,  377991,
 1232995, 1710213, 1610985, 1449849, 1232995,  968758,  667292,  340183,
 1048576, 1454417, 1370031, 1232995, 1048576,  823861,  567485,  289301,
  823861, 1142728, 1076426,  968758,  823861,  647303,  445870,  227303,
  567485,  787125,  741455,  667292,  567485,  445870,  307121,  156569,
  289301,  401273,  377991,  340183,  289301,  227303,  156569,   79818,
};
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::adjust_quant_table(int32 *dst, int32 *src)
{
  int i;

  for (i = 0; i < 64; i++)
    dst[i] = (src[i] * dct_scale[zag[i]]) >> 8;
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::compute_ycc_lookups(void)
{
  int i;

  for (i = 0; i < 256; i++)
  {

//    y  = ((( FIX(.29900) * r) + (FIX(.58700) * g) + (FIX(.11400) * b) + 32768) >> 16);
//    cb = (((-FIX(.16874) * r) - (FIX(.33126) * g) + (FIX(.50000) * b) + FIX(128)) >> 16);
//    cr = ((( FIX(.50000) * r) - (FIX(.41869) * g) - (FIX(.08131) * b) + FIX(128)) >> 16);

    ycc_xlt[i +  Y_R] = (FIX(.29900f) * i);
    ycc_xlt[i +  Y_G] = (FIX(.58700f) * i);
    ycc_xlt[i +  Y_B] = (FIX(.11400f) * i) + FIX(.5f);

    ycc_xlt[i + CB_R] = (-FIX(.16874f) * i);
    ycc_xlt[i + CB_G] = (-FIX(.33126f) * i);
    ycc_xlt[i + CB_B] = (FIX(.50000f) * i) + FIX(128); // 02/21/01: should this round too?

    ycc_xlt[i + CR_R] = (FIX(.50000f) * i);
    ycc_xlt[i + CR_G] = (-FIX(.41869f) * i);
    ycc_xlt[i + CR_B] = (-FIX(.08131f) * i) + FIX(128); // 02/21/01: should this round too?
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::first_pass_init(void)
{
  bit_buffer = 0; bits_in = 0;

  memset(last_dc_val, 0, 3 * sizeof(int));

  mcu_y_ofs = 0;

  pass_num = 1;
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::second_pass_init(void)
{
  compute_huffman_table(&huff_codes[0+0], &huff_code_sizes[0+0], huff_bits[0+0], huff_val[0+0]);
  compute_huffman_table(&huff_codes[2+0], &huff_code_sizes[2+0], huff_bits[2+0], huff_val[2+0]);
  compute_huffman_table(&huff_codes[0+1], &huff_code_sizes[0+1], huff_bits[0+1], huff_val[0+1]);
  compute_huffman_table(&huff_codes[2+1], &huff_code_sizes[2+1], huff_bits[2+1], huff_val[2+1]);

  first_pass_init();

  emit_markers();

  pass_num = 2;
}
/*----------------------------------------------------------------------------*/
int JPEG_Encoder::jpg_open(int p_x_res, int p_y_res, int src_channels)
{
  int i;

  switch (m_params.subsampling)
  {
    case Y_ONLY:
    {
      num_components = 1;
      comp_h_samp[0] = 1;
      comp_v_samp[0] = 1;
      mcu_x          = 8;
      mcu_y          = 8;
      break;
    }
    case H1V1_RGB:
    case H1V1:
    {
      num_components = 3;
      comp_h_samp[0] = 1;
      comp_v_samp[0] = 1;
      comp_h_samp[1] = 1;
      comp_v_samp[1] = 1;
      comp_h_samp[2] = 1;
      comp_v_samp[2] = 1;
      mcu_x          = 8;
      mcu_y          = 8;
      break;
    }
    case H2V1:
    {
      num_components = 3;
      comp_h_samp[0] = 2;
      comp_v_samp[0] = 1;
      comp_h_samp[1] = 1;
      comp_v_samp[1] = 1;
      comp_h_samp[2] = 1;
      comp_v_samp[2] = 1;
      mcu_x          = 16;
      mcu_y          = 8;
      break;
    }
//    case H2V2:
    default:
    {
      num_components = 3;
      comp_h_samp[0] = 2;
      comp_v_samp[0] = 2;
      comp_h_samp[1] = 1;
      comp_v_samp[1] = 1;
      comp_h_samp[2] = 1;
      comp_v_samp[2] = 1;
      mcu_x          = 16;
      mcu_y          = 16;
    }
  }

  image_x        = p_x_res;
  image_y        = p_y_res;
  image_bpp      = src_channels;
  image_bpl      = image_x * src_channels;

  image_x_mcu    = (image_x + mcu_x - 1) & (~(mcu_x - 1));
  image_y_mcu    = (image_y + mcu_y - 1) & (~(mcu_y - 1));
  image_bpl_xlt  = image_x * num_components;
  image_bpl_mcu  = image_x_mcu * num_components;
  mcus_per_row   = image_x_mcu / mcu_x;

  mcu_lines = reinterpret_cast<uchar**>(cmalloc_msg(mcu_y * sizeof(uchar *)));

  for (i = 0; i < mcu_y; i++)
    mcu_lines[i] = reinterpret_cast<uchar*>(malloc_msg(image_bpl_mcu));

  sample_array = reinterpret_cast<SAMPLE_ARRAY_TYPE*>(malloc_msg(64 * sizeof(SAMPLE_ARRAY_TYPE)));
  coefficient_array = reinterpret_cast<int16*>(malloc_msg(64 * sizeof(int16)));

  quantization_tables[0]        = reinterpret_cast<int32*>(malloc_msg(64 * sizeof(int32)));
  quantization_tables[1]        = reinterpret_cast<int32*>(malloc_msg(64 * sizeof(int32)));
  scaled_quantization_tables[0] = reinterpret_cast<int32*>(malloc_msg(64 * sizeof(int32)));
  scaled_quantization_tables[1] = reinterpret_cast<int32*>(malloc_msg(64 * sizeof(int32)));

  if (m_params.visual_quant)
  {
    compute_quant_table(quantization_tables[0], std_lum_quant);
    compute_quant_table(quantization_tables[1], m_params.no_chroma_discrim_flag ? std_lum_quant : std_croma_quant);
  }
  else
  {
     compute_quant_table(quantization_tables[0], dxt_lum_quant);
     compute_quant_table(quantization_tables[1], dxt_croma_quant);
  }
  
  adjust_quant_table(scaled_quantization_tables[0], quantization_tables[0]);
  adjust_quant_table(scaled_quantization_tables[1], quantization_tables[1]);

  out_buf_ofs = out_buf = reinterpret_cast<uchar*>(malloc_msg(out_buf_left = OUT_BUF_SIZE));
  ycc_xlt = reinterpret_cast<long*>(malloc_msg(9*256*sizeof(long)));

  compute_ycc_lookups();

  if (m_params.aq_setting)
  {
    adjusted_aq_setting = (100 + (m_params.aq_setting * 8));
    aq_coefficient_scale = max(1, (((99 - m_params.aq_setting) * 63 + 49) / 99));
  }
  else
  {
    adjusted_aq_setting = 0;
    aq_coefficient_scale = 0;
  }

  huff_bits[0+0] = reinterpret_cast<uchar*>(cmalloc_msg(17));
  huff_val [0+0] = reinterpret_cast<uchar*>(cmalloc_msg(DC_LUM_CODES));
  huff_bits[2+0] = reinterpret_cast<uchar*>(cmalloc_msg(17));
  huff_val [2+0] = reinterpret_cast<uchar*>(cmalloc_msg(AC_LUM_CODES));
  huff_bits[0+1] = reinterpret_cast<uchar*>(cmalloc_msg(17));
  huff_val [0+1] = reinterpret_cast<uchar*>(cmalloc_msg(DC_CHROMA_CODES));
  huff_bits[2+1] = reinterpret_cast<uchar*>(cmalloc_msg(17));
  huff_val [2+1] = reinterpret_cast<uchar*>(cmalloc_msg(AC_CHROMA_CODES));

  if (m_params.two_pass_flag)
  {
    huff_count[0+0] = reinterpret_cast<uint*>(cmalloc_msg(DC_LUM_CODES    * sizeof(uint)));
    huff_count[2+0] = reinterpret_cast<uint*>(cmalloc_msg(AC_LUM_CODES    * sizeof(uint)));
    huff_count[0+1] = reinterpret_cast<uint*>(cmalloc_msg(DC_CHROMA_CODES * sizeof(uint)));
    huff_count[2+1] = reinterpret_cast<uint*>(cmalloc_msg(AC_CHROMA_CODES * sizeof(uint)));

    first_pass_init();
  }
  else
  {
    memcpy(huff_bits[0+0], dc_lum_bits,   17);
    memcpy(huff_val [0+0], dc_lum_val,    DC_LUM_CODES);
    memcpy(huff_bits[2+0], ac_lum_bits,   17);
    memcpy(huff_val [2+0], ac_lum_val,    AC_LUM_CODES);
    memcpy(huff_bits[0+1], dc_chroma_bits, 17);
    memcpy(huff_val [0+1], dc_chroma_val,  DC_CHROMA_CODES);
    memcpy(huff_bits[2+1], ac_chroma_bits, 17);
    memcpy(huff_val [2+1], ac_chroma_val,  AC_CHROMA_CODES);

    // in effect, skip over the first pass
    second_pass_init();
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::load_block_8_8_grey(int x)
{
  int i;
  uchar *src;
  SAMPLE_ARRAY_TYPE *dst = sample_array;

  x <<= 3;

  for (i = 0; i < 8; i++, dst += 8)
  {
    src = mcu_lines[i] + x;

    dst[0] = src[0] - 128;
    dst[1] = src[1] - 128;
    dst[2] = src[2] - 128;
    dst[3] = src[3] - 128;
    dst[4] = src[4] - 128;
    dst[5] = src[5] - 128;
    dst[6] = src[6] - 128;
    dst[7] = src[7] - 128;
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::load_block_8_8(int x, int y, int c)
{
  int i;
  uchar *src;
  SAMPLE_ARRAY_TYPE *dst = sample_array;

  x = (x * (8 * 3)) + c;
  y <<= 3;

  for (i = 0; i < 8; i++, dst += 8)
  {
    src = mcu_lines[y + i] + x;

    dst[0] = src[0 * 3] - 128;
    dst[1] = src[1 * 3] - 128;
    dst[2] = src[2 * 3] - 128;
    dst[3] = src[3 * 3] - 128;
    dst[4] = src[4 * 3] - 128;
    dst[5] = src[5 * 3] - 128;
    dst[6] = src[6 * 3] - 128;
    dst[7] = src[7 * 3] - 128;
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::load_block_16_8(int x, int c)
{
  int i;
  uchar *src1, *src2;
  SAMPLE_ARRAY_TYPE *dst = sample_array;

  x = (x * (16 * 3)) + c;

  for (i = 0; i < 16; i += 2, dst += 8)
  {
    src1 = mcu_lines[i + 0] + x;
    src2 = mcu_lines[i + 1] + x;

    dst[0] = ((src1[ 0 * 3] + src1[ 1 * 3] +
               src2[ 0 * 3] + src2[ 1 * 3]) >> 2) - 128;

    dst[1] = ((src1[ 2 * 3] + src1[ 3 * 3] +
               src2[ 2 * 3] + src2[ 3 * 3]) >> 2) - 128;

    dst[2] = ((src1[ 4 * 3] + src1[ 5 * 3] +
               src2[ 4 * 3] + src2[ 5 * 3]) >> 2) - 128;

    dst[3] = ((src1[ 6 * 3] + src1[ 7 * 3] +
               src2[ 6 * 3] + src2[ 7 * 3]) >> 2) - 128;

    dst[4] = ((src1[ 8 * 3] + src1[ 9 * 3] +
               src2[ 8 * 3] + src2[ 9 * 3]) >> 2) - 128;

    dst[5] = ((src1[10 * 3] + src1[11 * 3] +
               src2[10 * 3] + src2[11 * 3]) >> 2) - 128;

    dst[6] = ((src1[12 * 3] + src1[13 * 3] +
               src2[12 * 3] + src2[13 * 3]) >> 2) - 128;

    dst[7] = ((src1[14 * 3] + src1[15 * 3] +
               src2[14 * 3] + src2[15 * 3]) >> 2) - 128;
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::load_block_16_8_8(int x, int c)
{
  int i;
  uchar *src1;
  SAMPLE_ARRAY_TYPE *dst = sample_array;

  x = (x * (16 * 3)) + c;

  for (i = 0; i < 8; i++, dst += 8)
  {
    src1 = mcu_lines[i + 0] + x;

    dst[0] = ((src1[ 0 * 3] + src1[ 1 * 3]) >> 1) - 128;
    dst[1] = ((src1[ 2 * 3] + src1[ 3 * 3]) >> 1) - 128;
    dst[2] = ((src1[ 4 * 3] + src1[ 5 * 3]) >> 1) - 128;
    dst[3] = ((src1[ 6 * 3] + src1[ 7 * 3]) >> 1) - 128;
    dst[4] = ((src1[ 8 * 3] + src1[ 9 * 3]) >> 1) - 128;
    dst[5] = ((src1[10 * 3] + src1[11 * 3]) >> 1) - 128;
    dst[6] = ((src1[12 * 3] + src1[13 * 3]) >> 1) - 128;
    dst[7] = ((src1[14 * 3] + src1[15 * 3]) >> 1) - 128;
  }
}
/*----------------------------------------------------------------------------*/
long JPEG_Encoder::sample_block_variance(void)
{
  int i;
  long s = 0, s2 = 0;

  for (i = 0; i < 64; i++)  /* fixed from 63 */
  {
    s += sample_array[i];
    s2 += (sample_array[i] * sample_array[i]);
  }

  return (s2 / 64) - ((s / 64) * (s / 64));
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::load_coefficients(int component_num, long block_variance)
{
  int i;
#ifdef SCALED_DCT
  int32 *q = scaled_quantization_tables[component_num > 0];
#else
  int32 *q = quantization_tables[component_num > 0];
#endif

  int16 *dst = coefficient_array;

  if (block_variance < adjusted_aq_setting)
  {
    long mq_scale = (3 + ((m_params.quality * 5) / 99)) * (long)((adjusted_aq_setting - 1) - block_variance);
    long mq_div = (adjusted_aq_setting - 1) * (long)aq_coefficient_scale;

    for (i = 0; i < 64; i++)
    {
      SAMPLE_ARRAY_TYPE j = sample_array[zag[i]];

      if (j < 0)
      {
        if ((j = -j + (*q >> 1)) < *q)
          *dst++ = 0;
        else
        {
          *dst++ = static_cast<int16>(-(j / *q));

          if (abs(dst[-1]) < ((mq_scale * min(i, aq_coefficient_scale) + (mq_div >> 1)) / mq_div))
            dst[-1] = 0;
        }
      }
      else
      {
        if ((j = j + (*q >> 1)) < *q)
          *dst++ = 0;
        else
        {
          *dst++ = static_cast<int16>(j / *q);

          if (abs(dst[-1]) < ((mq_scale * min(i, aq_coefficient_scale) + (mq_div >> 1)) / mq_div))
            dst[-1] = 0;
        }
      }

      q++;
    }
  }
  else
  {
    for (i = 0; i < 64; i++)
    {
      SAMPLE_ARRAY_TYPE j = sample_array[zag[i]];

      if (j < 0)
      {
        if ((j = -j + (*q >> 1)) < *q)
          *dst++ = 0;
        else
          *dst++ = static_cast<int16>(-(j / *q));
      }
      else
      {
        if ((j = j + (*q >> 1)) < *q)
          *dst++ = 0;
        else
          *dst++ = static_cast<int16>(j / *q);
      }

      q++;
    }
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::flush_output_buffer(void)
{
  if (out_buf_left != OUT_BUF_SIZE)
    m_stream.put(out_buf, OUT_BUF_SIZE - out_buf_left);

  //bio_block_write(handle, out_buf, OUT_BUF_SIZE - out_buf_left);

  out_buf_ofs  = out_buf;
  out_buf_left = OUT_BUF_SIZE;
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::put_bits(uint bits, uint len)
{
  //if ((len == 0) || (len > 16))
  //  abort();
  assert(len > 0);
  assert(len <= 16);

  bits &= ((1UL << len) - 1);

  bits_in += len;

  bit_buffer |= ((uint)bits << (24 - bits_in));

  while (bits_in >= 8)
  {
    uchar c;

    #define put_byte(c) { *out_buf_ofs++ = (c); if (--out_buf_left == 0) flush_output_buffer(); }

    put_byte( c = (uchar)((bit_buffer >> 16) & 0xFF) );

    if (c == 0xFF)
      put_byte(0);

    bit_buffer <<= 8;

    bits_in -= 8;
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::code_coefficients_pass_two(int component_num)
{
  int i, j, run_len, nbits, temp1, temp2;
  int16 *src = coefficient_array;
  uint *codes[2];
  uchar *code_sizes[2];

  if (component_num == 0)
  {
    codes[0]       = huff_codes[0 + 0];
    codes[1]       = huff_codes[2 + 0];
    code_sizes[0]  = huff_code_sizes[0 + 0];
    code_sizes[1]  = huff_code_sizes[2 + 0];
  }
  else
  {
    codes[0]       = huff_codes[0 + 1];
    codes[1]       = huff_codes[2 + 1];
    code_sizes[0]  = huff_code_sizes[0 + 1];
    code_sizes[1]  = huff_code_sizes[2 + 1];
  }

  temp1 = temp2 = src[0] - last_dc_val[component_num];
  last_dc_val[component_num] = src[0];

  if (temp1 < 0)
  {
    temp1 = -temp1;
    temp2--;
  }

  nbits = 0;
  while (temp1)
  {
    nbits++;
    temp1 >>= 1;
  }

  put_bits(codes[0][nbits], code_sizes[0][nbits]);
  if (nbits)
    put_bits(temp2, nbits);

  for (run_len = 0, i = 1; i < 64; i++)
  {
    if ((temp1 = coefficient_array[i]) == 0)
      run_len++;
    else
    {
      while (run_len >= 16)
      {
        put_bits(codes[1][0xF0], code_sizes[1][0xF0]);
        run_len -= 16;
      }

      if ((temp2 = temp1) < 0)
      {
        temp1 = -temp1;
        temp2--;
      }

      nbits = 1;
      while (temp1 >>= 1)
        nbits++;

      j = (run_len << 4) + nbits;

      put_bits(codes[1][j], code_sizes[1][j]);
      put_bits(temp2, nbits);

      run_len = 0;
    }
  }

  if (run_len)
    put_bits(codes[1][0], code_sizes[1][0]);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::code_coefficients_pass_one(int component_num)
{
  int i, run_len, nbits, temp1;
  int16 *src = coefficient_array;
  uint *dc_count;
  uint *ac_count;

  if (component_num == 0)
  {
    dc_count       = huff_count[0 + 0];
    ac_count       = huff_count[2 + 0];
  }
  else
  {
    dc_count       = huff_count[0 + 1];
    ac_count       = huff_count[2 + 1];
  }

  temp1 = src[0] - last_dc_val[component_num];
  last_dc_val[component_num] = src[0];

  if (temp1 < 0)
    temp1 = -temp1;

  nbits = 0;
  while (temp1)
  {
    nbits++;
    temp1 >>= 1;
  }

  dc_count[nbits]++;

  for (run_len = 0, i = 1; i < 64; i++)
  {
    if ((temp1 = coefficient_array[i]) == 0)
      run_len++;
    else
    {
      while (run_len >= 16)
      {
        ac_count[0xF0]++;
        run_len -= 16;
      }

      if (temp1 < 0)
        temp1 = -temp1;

      nbits = 1;
      while (temp1 >>= 1)
        nbits++;

      ac_count[(run_len << 4) + nbits]++;

      run_len = 0;
    }
  }

  if (run_len)
    ac_count[0]++;
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::code_block(int component_num)
{
  long block_variance;

  if (adjusted_aq_setting)
    block_variance = sample_block_variance();
  else
    block_variance = 0;

#ifdef SCALED_DCT
  Scaled_DCT(sample_array);
#else
  dct(sample_array);
#endif

  load_coefficients(component_num, block_variance);

  if (pass_num == 1)
    code_coefficients_pass_one(component_num);
  else
    code_coefficients_pass_two(component_num);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::process_mcu_row(void)
{
  int i;

  if (num_components == 1)
  {
    for (i = 0; i < mcus_per_row; i++)
    {
      load_block_8_8_grey(i);
      code_block(0);
    }
  }
  else if ((comp_h_samp[0] == 1) && (comp_v_samp[0] == 1))
  {
    for (i = 0; i < mcus_per_row; i++)
    {
      load_block_8_8(i, 0, 0);
      code_block(0);

      load_block_8_8(i, 0, 1);
      code_block(1);

      load_block_8_8(i, 0, 2);
      code_block(2);
    }
  }
  else if ((comp_h_samp[0] == 2) && (comp_v_samp[0] == 1))
  {
    for (i = 0; i < mcus_per_row; i++)
    {
      load_block_8_8(i * 2 + 0, 0, 0);
      code_block(0);

      load_block_8_8(i * 2 + 1, 0, 0);
      code_block(0);

      load_block_16_8_8(i, 1);
      code_block(1);

      load_block_16_8_8(i, 2);
      code_block(2);
    }
  }
  else if ((comp_h_samp[0] == 2) && (comp_v_samp[0] == 2))
  {
    for (i = 0; i < mcus_per_row; i++)
    {
      load_block_8_8(i * 2 + 0, 0, 0);
      code_block(0);

      load_block_8_8(i * 2 + 1, 0, 0);
      code_block(0);

      load_block_8_8(i * 2 + 0, 1, 0);
      code_block(0);

      load_block_8_8(i * 2 + 1, 1, 0);
      code_block(0);

      load_block_16_8(i, 1);
      code_block(1);

      load_block_16_8(i, 2);
      code_block(2);
    }
  }
  else
  {
    assert(false);
  }
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::optimize_huffman_table(int table_num, int table_len)
{
  int i, j;
  int code_list_len;
  uint *sym_freq = NULL;
  int *code_sizes = NULL;
  int *num_codes  = NULL;
  int *code_list  = NULL;
  
   sym_freq = reinterpret_cast<uint*>(cmalloc_msg(257 * sizeof(uint)));

   memcpy(sym_freq, huff_count[table_num], table_len * sizeof(uint));

   sym_freq[256] = 1;    /* assures that no valid code contains all 1's */

   huffman_code_sizes(257, sym_freq, &code_sizes);
   huffman_sort_code_sizes(257, code_sizes, &num_codes, &code_list, &code_list_len);

   huffman_fix_code_sizes(num_codes, code_list_len, 16);

   memset(huff_bits[table_num], 0, 17);
   memset(huff_val[table_num], 0, table_len);

   for (j = 0, i = 1; i <= 16; i++)
   j += (huff_bits[table_num][i] = static_cast<uchar>(num_codes[i]));

   assert(j == code_list_len);                     /* debugging */
   assert(code_list[code_list_len - 1] == 256);

   //if (j != code_list_len)                     /* debugging */
   //  abort();

   //if (code_list[code_list_len - 1] != 256)    /* debugging */
   //  abort();

   for (i = 16; i >= 1; i--)
      if (huff_bits[table_num][i])
      {
         huff_bits[table_num][i]--;
         break;
      }

   for (i = 0; i < code_list_len - 1; i++)
      huff_val[table_num][i] = static_cast<uchar>(code_list[i]);
    
  free(sym_freq);
  free(code_sizes);
  free(num_codes);
  free(code_list);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::terminate_pass_one(void)
{
  optimize_huffman_table(0+0, DC_LUM_CODES);
  optimize_huffman_table(2+0, AC_LUM_CODES);
  
  if (num_components > 1)
  {
    optimize_huffman_table(0+1, DC_CHROMA_CODES);
    optimize_huffman_table(2+1, AC_CHROMA_CODES);
  }

  second_pass_init();
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::terminate_pass_two(void)
{
  put_bits(0x7F, 7);

  flush_output_buffer();

  emit_marker(M_EOI);
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::process_end_of_image(void)
{
  int i;

  if (mcu_y_ofs)
  {
    for (i = mcu_y_ofs; i < mcu_y; i++)
      memcpy(mcu_lines[i], mcu_lines[mcu_y_ofs - 1], image_bpl_mcu);

    process_mcu_row();
  }

  if (pass_num == 1)
    terminate_pass_one();
  else
    terminate_pass_two();
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::load_mcu(const void *src)
{
  const uchar* Psrc = reinterpret_cast<const uchar*>(src);

  // OK to write up to image_bpl_xlt bytes to Pdst
  uchar* Pdst = mcu_lines[mcu_y_ofs];

  if (num_components == 1)
  {
    if (image_bpp == 3)
      RGB_to_Y(Pdst, Psrc, ycc_xlt, image_x);
    else
      memcpy(Pdst, Psrc, image_x);
  }
  else
  {
    if (image_bpp == 3)
    {
      if (m_params.subsampling == H1V1_RGB)
         memcpy(Pdst, Psrc, image_x * 3);
      else
         RGB_to_YCC(Pdst, Psrc, ycc_xlt, image_x);
    }
    else
      Y_to_YCC(Pdst, Psrc, image_x);
  }

  if (num_components == 1)
    memset(mcu_lines[mcu_y_ofs] + image_bpl_xlt, Pdst[image_bpl_xlt - 1], image_x_mcu - image_x);
  else
  {
    int i;
    uchar y, cb, cr;
    uchar *dst;

    y  = Pdst[image_bpl_xlt - 3 + 0];
    cb = Pdst[image_bpl_xlt - 3 + 1];
    cr = Pdst[image_bpl_xlt - 3 + 2];

    dst = mcu_lines[mcu_y_ofs] + image_bpl_xlt;

    for (i = image_x; i < image_x_mcu; i++)
    {
      *dst++ = y;
      *dst++ = cb;
      *dst++ = cr;
    }
  }

  if (++mcu_y_ofs == mcu_y)
  {
    process_mcu_row();

    mcu_y_ofs = 0;
  }
}
/*----------------------------------------------------------------------------*/
bool JPEG_Encoder::jpg_data(const void* p)
{
  if (!p)
    process_end_of_image();
  else
    load_mcu(p);

  return (m_stream.status());
}
/*----------------------------------------------------------------------------*/
void JPEG_Encoder::jpg_close(void)
{
  int i;

  if (mcu_lines)
  {
    for (i = 0; i < mcu_y; i++)
      free(mcu_lines[i]);

    free(mcu_lines); mcu_lines = NULL;
  }

  free(sample_array); sample_array = NULL;
  free(coefficient_array); coefficient_array = NULL;

  free(quantization_tables[0]); quantization_tables[0] = NULL;
  free(quantization_tables[1]); quantization_tables[1] = NULL;

  free(scaled_quantization_tables[0]); scaled_quantization_tables[0] = NULL;
  free(scaled_quantization_tables[1]); scaled_quantization_tables[1] = NULL;

  for (i = 0; i < 4; i++)
  {
    free(huff_codes[i]);      huff_codes[i]      = NULL;
    free(huff_code_sizes[i]); huff_code_sizes[i] = NULL;
    free(huff_bits[i]);       huff_bits[i]       = NULL;
    free(huff_val[i]);        huff_val[i]        = NULL;
    free(huff_count[i]);      huff_count[i]      = NULL;
  }

  free(out_buf); out_buf = NULL;

  free(ycc_xlt); ycc_xlt = NULL;
}
/*----------------------------------------------------------------------------*/
namespace {
  template <class T> void clear(T& o) { memset(&o, 0, sizeof(o)); }
}
/*----------------------------------------------------------------------------*/
JPEG_Encoder::JPEG_Encoder(
  Stream& stream, const Params& params,
  int width, int height, int src_channels) :
    m_stream(stream),
    m_params(params),
    num_components(0),
    image_x(0), image_y(0),
    image_bpp(0), image_bpl(0),
    image_x_mcu(0), image_y_mcu(0),
    image_bpl_xlt(0), image_bpl_mcu(0),
    mcus_per_row(0), mcu_x(0), mcu_y(0),
    mcu_lines(NULL),
    mcu_y_ofs(0),
    sample_array(NULL),
    coefficient_array(NULL),
    out_buf(NULL),
    out_buf_ofs(NULL),
    out_buf_left(0),
    bit_buffer(0),
    bits_in(0),
    ycc_xlt(NULL),
    adjusted_aq_setting(0),
    aq_coefficient_scale(0),
    pass_num(0)
{
  clear(comp_h_samp);
  clear(comp_v_samp);
  clear(quantization_tables);
  clear(scaled_quantization_tables);
  clear(huff_codes);
  clear(huff_code_sizes);
  clear(huff_bits);
  clear(huff_val);
  clear(huff_count);
  clear(last_dc_val);

  assert(width > 0);
  assert(height > 0);
  assert(((src_channels == 1) || (src_channels == 3)));

  m_params.check();

  if (jpg_open(width, height, src_channels))
    out_of_memory();
}
/*----------------------------------------------------------------------------*/
JPEG_Encoder::~JPEG_Encoder()
{
  jpg_close();
}
/*----------------------------------------------------------------------------*/
bool JPEG_Encoder::scanline(const void* Pscanline)
{
  return (jpg_data(Pscanline));
}
/*----------------------------------------------------------------------------*/

