// fastInflator.h
// Deprecated! Use the regular inflator now on 360. 
#pragma once

#define INFLATOR_OKAY             ( 0)
#define INFLATOR_DONE             ( 1)
#define INFLATOR_CODE_SET_ERROR   (-1)
#define INFLATOR_MEM_ERROR        (-2)
#define INFLATOR_BAD_BLOCK_ERROR  (-3)
#define INFLATOR_RAW_BLOCK_ERROR  (-4)
#define INFLATOR_DYN_BLOCK_ERROR  (-5)
#define INFLATOR_BAD_CODE_ERROR   (-6)
#define INFLATOR_TOO_MUCH_ERROR   (-7)
#define INFLATOR_INCOMPLETE_ERROR (-8)

#define INFLATOR_NUM_SYMBOLS_1    (288)
#define INFLATOR_NUM_SYMBOLS_2    (32)
#define INFLATOR_NUM_SYMBOLS_3    (19)

class BFastInflator
{
public:

	struct BErrorHandler
	{
		virtual void operator() (int status) = 0;
	};

	struct BGenericErrorHandler : BErrorHandler
	{
		virtual void operator() (int status)
		{
		   status;
			exit(EXIT_FAILURE);
		}
	};

   BFastInflator();

	// dst_len must be the size of the decompressed data.
	// If the compressed data is corrupted, it's possible for the dst buffer to be overwritten!
	// Returns the number of bytes written to the destination buffer.
	uint decompress(const uchar* RESTRICT Psrc, int src_len, uchar* RESTRICT Pdst, int dst_len, BErrorHandler& error_handler = static_cast<BErrorHandler&>(BGenericErrorHandler()));
	
	~BFastInflator();

private:

   BErrorHandler* Perror_handler;
   	
   char code_size_1[INFLATOR_NUM_SYMBOLS_1];
   char code_size_2[INFLATOR_NUM_SYMBOLS_2];
   char code_size_3[INFLATOR_NUM_SYMBOLS_3];

   typedef short Look_Up_Type;
   typedef short Tree_Type;

   Look_Up_Type* RESTRICT look_up_1;
   Look_Up_Type* RESTRICT look_up_2;
   Look_Up_Type* RESTRICT look_up_3;

   Tree_Type* RESTRICT tree_1;
   Tree_Type* RESTRICT tree_2;
   Tree_Type* RESTRICT tree_3;

   const uchar* RESTRICT Pin_buf;
   const uchar* RESTRICT in_buf_cur_ofs;
   int in_buf_bit_ofs;

   uchar* RESTRICT Pout_buf;
   uchar* RESTRICT out_buf_cur_ofs;
   int out_buf_left;

	void build_huffman_decoder_tables(int num_symbols, char* RESTRICT code_size,
		                                Look_Up_Type** RESTRICT _look_up, Tree_Type** RESTRICT _tree);

	__declspec(noreturn) void error(int status);

	inline void remove_bits(int num_bits);
	inline uint get_bits(int num_bits);
	inline uint peek_bits(int num_bits);
	inline uint peek_bits(void);
	inline uint64 peek_bits64(void);
	void align_to_byte_boundary(void);
	void raw_block(void);
	void decompress_block(void);
	void static_block(void);
	inline int get_symbol(char* RESTRICT Pcode_size, Look_Up_Type* RESTRICT Plook_up, Tree_Type* RESTRICT Ptree);
	void dynamic_block(void);
	void inflate_block(void);
	uint inflator_main(void);
};


