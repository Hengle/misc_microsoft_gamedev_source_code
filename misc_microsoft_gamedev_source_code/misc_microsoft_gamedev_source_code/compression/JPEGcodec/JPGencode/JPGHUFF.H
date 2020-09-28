// jpghuff.h

#ifndef JPGHUFF_H
#define JPGHUFF_H

#include "..\JPGmain.h"

#define MEMERROR (-1)

int huffman_code_sizes(int num_symbols, uint *sym_freq,
                       int * *pcode_sizes);

int huffman_sort_code_sizes(int num_symbols, int *code_sizes,
                            int * *pnum_codes,
                            int * *pcode_list, int *pcode_list_len);

void huffman_fix_code_sizes(int *num_codes,
                            int code_list_len,
                            int max_code_size);

#endif

