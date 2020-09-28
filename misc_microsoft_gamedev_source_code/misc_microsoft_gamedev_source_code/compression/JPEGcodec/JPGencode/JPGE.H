// jpge.h

#ifndef JPGE_H
#define JPGE_H

#include "..\JPGmain.h"

class JPEG_Encoder
{
public:

  class Stream
  {
  public:
    Stream() { };
    virtual ~Stream() { };

    // Warning: It is OK for len to be 0
    virtual void put(const void* Pbuf, int len) = 0;
    template<class T> void put(const T& obj) { put(&obj, sizeof(T)); }

    virtual bool status(void) = 0; // return true if the stream is in an error state
  };

	typedef enum 
	{
		Y_ONLY = 0,
		H1V1 = 1,
		H2V1 = 2,
		H2V2 = 3,
		H1V1_RGB = 4,
		
		SubsamplingNum
		
	} Subsampling;

  struct Params
  {
    // Quality: 1-99, higher is better
    int quality;

    // Subsampling: 0-3
    // 0 = Y (greyscale) only
    // 1 = YCbCr, no subsampling (H1V1, YCbCr 1x1x1, 3 blocks per MCU)
    // 2 = YCbCr, H2V1 subsampling (YCbCr 2x1x1, 4 blocks per MCU)
    // 3 = YCbCr, H2V2 subsampling (YCbCr 4x1x1, 6 blocks per MCU-- very common)
    Subsampling subsampling;

    // Adaptive quantization (experimental)
    // 0-99, higher is worse
    int aq_setting;

    // Two pass flag
    // If set to true, the Huffman tables will be optimized.
    // Compression is a bit more than twice as slow, however.
    bool two_pass_flag;

    // Disables CbCr discrimination (experimental)
    // If true, the Y quantization table is also used for the CbCr channels.
    // Not recommended unless you know what you're doing!
    bool no_chroma_discrim_flag;
    
    bool visual_quant;

    Params() :
      quality(85),
      subsampling(H2V2),
      aq_setting(0),
      two_pass_flag(false),
      no_chroma_discrim_flag(false),
      visual_quant(true)
    {
    }

    void check(void) const
    {
      assert(quality >= 1);
      assert(quality <= 99);
      assert(subsampling >= 0);
      assert(subsampling <= (int)SubsamplingNum);
      assert(aq_setting >= 0);
      assert(aq_setting <= 99);
    }
  };

private:
  typedef int32 SAMPLE_ARRAY_TYPE;

  typedef enum
  {
    M_SOF0  = 0xC0,
    M_SOF1  = 0xC1,
    M_SOF2  = 0xC2,
    M_SOF3  = 0xC3,

    M_SOF5  = 0xC5,
    M_SOF6  = 0xC6,
    M_SOF7  = 0xC7,

    M_JPG   = 0xC8,
    M_SOF9  = 0xC9,
    M_SOF10 = 0xCA,
    M_SOF11 = 0xCB,

    M_SOF13 = 0xCD,
    M_SOF14 = 0xCE,
    M_SOF15 = 0xCF,

    M_DHT   = 0xC4,

    M_DAC   = 0xCC,

    M_RST0  = 0xD0,
    M_RST1  = 0xD1,
    M_RST2  = 0xD2,
    M_RST3  = 0xD3,
    M_RST4  = 0xD4,
    M_RST5  = 0xD5,
    M_RST6  = 0xD6,
    M_RST7  = 0xD7,

    M_SOI   = 0xD8,
    M_EOI   = 0xD9,
    M_SOS   = 0xDA,
    M_DQT   = 0xDB,
    M_DNL   = 0xDC,
    M_DRI   = 0xDD,
    M_DHP   = 0xDE,
    M_EXP   = 0xDF,

    M_APP0  = 0xE0,
    M_APP15 = 0xEF,

    M_JPG0  = 0xF0,
    M_JPG13 = 0xFD,
    M_COM   = 0xFE,

    M_TEM   = 0x01,

    M_ERROR = 0x100,

    RST0    = 0xD0

  } JPEG_MARKER;

  enum { DC_LUM_CODES = 12, AC_LUM_CODES = 256 };
  enum { DC_CHROMA_CODES = 12, AC_CHROMA_CODES = 256 };

  enum { NOTENOUGHMEM = -1 };

  // jpgtab.cpp
  static int zag[64];
  static int16 std_lum_quant[64];
  static int16 std_croma_quant[64];
  static int16 dxt_lum_quant[64];
  static int16 dxt_croma_quant[64];
  static uchar dc_lum_bits[17];
  static uchar dc_lum_val[DC_LUM_CODES];
  static uchar ac_lum_bits[17];
  static uchar ac_lum_val[AC_LUM_CODES];
  static uchar dc_chroma_bits[17];
  static uchar dc_chroma_val[DC_CHROMA_CODES];
  static uchar ac_chroma_bits[17];
  static uchar ac_chroma_val[AC_CHROMA_CODES];

  Stream& m_stream;
  const Params& m_params;

  uchar num_components;
  uchar comp_h_samp[3], comp_v_samp[3];

  int image_x, image_y, image_bpp, image_bpl;
  int image_x_mcu, image_y_mcu;
  int image_bpl_xlt, image_bpl_mcu;
  int mcus_per_row;
  int mcu_x, mcu_y;

  uchar * *mcu_lines;
  uchar mcu_y_ofs;

  SAMPLE_ARRAY_TYPE *sample_array;
  int16 *coefficient_array;

  int32 *quantization_tables[2];
  int32 *scaled_quantization_tables[2];

  uint  *huff_codes[4];
  uchar *huff_code_sizes[4];
  uchar *huff_bits[4];
  uchar *huff_val[4];
  uint *huff_count[4];

  int last_dc_val[3];

  uchar *out_buf;
  uchar *out_buf_ofs;
  uint out_buf_left;

  uint bit_buffer;
  uint bits_in;

  long *ycc_xlt;

  int adjusted_aq_setting, aq_coefficient_scale;

  uchar pass_num;

  // jpgwrite.cpp
  void emit_marker(int marker);
  void emit_byte(uchar i);
  void emit_word(uint i);
  void emit_jfif_app0(void);
  void emit_dqt(void);
  void emit_sof(void);
  void emit_dht(uchar *bits, uchar *val, int index, int ac_flag);
  void emit_dhts(void);
  void emit_sos(void);
  void emit_markers(void);
  void compute_huffman_table(
    uint * *codes,
    uchar * *code_sizes,
    uchar *bits,
    uchar *val);
  void compute_quant_table(int32 *dst, int16 *src);
  void adjust_quant_table(int32 *dst, int32 *src);
  void compute_ycc_lookups(void);
  void first_pass_init(void);
  void second_pass_init(void);
  int jpg_open(int p_x_res, int p_y_res, int src_channels);
  void load_block_8_8_grey(int x);
  void load_block_8_8(int x, int y, int c);
  void load_block_16_8(int x, int c);
  void load_block_16_8_8(int x, int c);
  long sample_block_variance(void);
  void load_coefficients(int component_num, long block_variance);
  void flush_output_buffer(void);
  void put_bits(uint bits, uint len);
  void code_coefficients_pass_two(int component_num);
  void code_coefficients_pass_one(int component_num);
  void code_block(int component_num);
  void process_mcu_row(void);
  void optimize_huffman_table(int table_num, int table_len);
  void terminate_pass_one(void);
  void terminate_pass_two(void);
  void process_end_of_image(void);
  void load_mcu(const void* src);
  bool jpg_data(const void* p);
  void jpg_close(void);

  // jpghuff.cpp
  void down_heap(int *heap, uint *sym_freq, int heap_len, int i);

  void huffman_code_sizes(int num_symbols, uint *sym_freq,
                         int * *pcode_sizes);

  void huffman_sort_code_sizes(int num_symbols, int *code_sizes,
                              int * *pnum_codes,
                              int * *pcode_list, int *pcode_list_len);

  void huffman_fix_code_sizes(int *num_codes,
                              int code_list_len,
                              int max_code_size);

  // helpers
  void out_of_memory(void)
  {
    // throw an exception if malloc ever returns NULL
    // the original implementation used returned error codes
    throw "jpeg_encoder::jpg_open: Out of memory!";
  }

  void* malloc_msg(size_t len)
  {
    void*p = malloc(len);

    if (!p)
      out_of_memory();

    return p;
  }

  void* cmalloc_msg(size_t len)
  {
    void*p = malloc_msg(len);
    memset(p, 0, len);
    return p;
  }

public:
  // stream - The stream object to use for outputting data.
  // params - Compression parameters.
  // width, height  - Dimensions of image
  // channels - May be 1, or 3. 1 indicates greyscale, 3 indicates BGR.
  // An exception will be thrown on out of memory conditions (see
  // the out_of_memory() method).
  JPEG_Encoder(
    Stream& stream,
    const Params& params,
    int width, int height,
    int channels);

  ~JPEG_Encoder();

  // Call this method with each scanline.
  // width * channels bytes per scanline is expected (BGR or Y format).
  // You must call with NULL after all scanlines.
  // If params.two_pass_flag was set to true, you must do this two times.
  // Returns true if the stream object's status() method returns true.
  // An exception will be thrown on out of memory conditions (see
  // the out_of_memory() method).
  bool scanline(const void* Pscanline);
};

#endif

