/*----------------------------------------------------------------------------*/
/* fdct.c                                                                     */
/*----------------------------------------------------------------------------*/
#include "..\JPGmain.h"
#include "fdct.h"
/*----------------------------------------------------------------------------*/
#define CONST_BITS  13
#define PASS1_BITS  2
#define ONE ((int32) 1)
#define CONST_SCALE (ONE << CONST_BITS)
#define FIX(x) ((int32) ((x) * CONST_SCALE + 0.5))
#define FIX_0_298631336 ((int32) 2446)    /* FIX(0.298631336) */
#define FIX_0_390180644 ((int32) 3196)    /* FIX(0.390180644) */
#define FIX_0_541196100 ((int32) 4433)    /* FIX(0.541196100) */
#define FIX_0_765366865 ((int32) 6270)    /* FIX(0.765366865) */
#define FIX_0_899976223 ((int32) 7373)    /* FIX(0.899976223) */
#define FIX_1_175875602 ((int32) 9633)    /* FIX(1.175875602) */
#define FIX_1_501321110 ((int32) 12299)   /* FIX(1.501321110) */
#define FIX_1_847759065 ((int32) 15137)   /* FIX(1.847759065) */
#define FIX_1_961570560 ((int32) 16069)   /* FIX(1.961570560) */
#define FIX_2_053119869 ((int32) 16819)   /* FIX(2.053119869) */
#define FIX_2_562915447 ((int32) 20995)   /* FIX(2.562915447) */
#define FIX_3_072711026 ((int32) 25172)   /* FIX(3.072711026) */
#define DESCALE(x, n) (((x) + (ONE << ((n) - 1))) >> (n))
#define MULTIPLY(var, const)  (((int16) (var)) * ((int32) (const)))
/*----------------------------------------------------------------------------*/
void dct(int32 *data)
{
  int32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  int32 tmp10, tmp11, tmp12, tmp13;
  int32 z1, z2, z3, z4, z5;
  register int32 *data_ptr;
  int row_counter;

  data_ptr = data;

  for (row_counter = 7; row_counter >= 0; row_counter--)
  {
    tmp0 = data_ptr[0] + data_ptr[7];
    tmp7 = data_ptr[0] - data_ptr[7];

    tmp1 = data_ptr[1] + data_ptr[6];
    tmp6 = data_ptr[1] - data_ptr[6];
    tmp2 = data_ptr[2] + data_ptr[5];
    tmp5 = data_ptr[2] - data_ptr[5];
    tmp3 = data_ptr[3] + data_ptr[4];
    tmp4 = data_ptr[3] - data_ptr[4];

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    data_ptr[0] = (int32) ((tmp10 + tmp11) << PASS1_BITS);
    data_ptr[4] = (int32) ((tmp10 - tmp11) << PASS1_BITS);

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    data_ptr[2] = (int32) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865), CONST_BITS-PASS1_BITS);
    data_ptr[6] = (int32) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065), CONST_BITS-PASS1_BITS);

    z1 = tmp4 + tmp7;
    z2 = tmp5 + tmp6;
    z3 = tmp4 + tmp6;
    z4 = tmp5 + tmp7;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);

    tmp4 = MULTIPLY(tmp4, FIX_0_298631336);
    tmp5 = MULTIPLY(tmp5, FIX_2_053119869);
    tmp6 = MULTIPLY(tmp6, FIX_3_072711026);
    tmp7 = MULTIPLY(tmp7, FIX_1_501321110);
    z1   = MULTIPLY(z1, - FIX_0_899976223);
    z2   = MULTIPLY(z2, - FIX_2_562915447);
    z3   = MULTIPLY(z3, - FIX_1_961570560);
    z4   = MULTIPLY(z4, - FIX_0_390180644);

    z3 += z5;
    z4 += z5;

    data_ptr[7] = (int32) DESCALE(tmp4 + z1 + z3, CONST_BITS-PASS1_BITS);
    data_ptr[5] = (int32) DESCALE(tmp5 + z2 + z4, CONST_BITS-PASS1_BITS);
    data_ptr[3] = (int32) DESCALE(tmp6 + z2 + z3, CONST_BITS-PASS1_BITS);
    data_ptr[1] = (int32) DESCALE(tmp7 + z1 + z4, CONST_BITS-PASS1_BITS);

    data_ptr += 8;
  }

  data_ptr = data;

  for (row_counter = 7; row_counter >= 0; row_counter--)
  {
    tmp0 = data_ptr[8*0] + data_ptr[8*7];
    tmp7 = data_ptr[8*0] - data_ptr[8*7];
    tmp1 = data_ptr[8*1] + data_ptr[8*6];
    tmp6 = data_ptr[8*1] - data_ptr[8*6];
    tmp2 = data_ptr[8*2] + data_ptr[8*5];
    tmp5 = data_ptr[8*2] - data_ptr[8*5];
    tmp3 = data_ptr[8*3] + data_ptr[8*4];
    tmp4 = data_ptr[8*3] - data_ptr[8*4];

    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    data_ptr[8*0] = (int32) DESCALE(tmp10 + tmp11, PASS1_BITS+3);
    data_ptr[8*4] = (int32) DESCALE(tmp10 - tmp11, PASS1_BITS+3);

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    data_ptr[8*2] = (int32) DESCALE(z1 + MULTIPLY(tmp13, FIX_0_765366865), CONST_BITS+PASS1_BITS+3);
    data_ptr[8*6] = (int32) DESCALE(z1 + MULTIPLY(tmp12, - FIX_1_847759065), CONST_BITS+PASS1_BITS+3);

    z1 = tmp4 + tmp7;
    z2 = tmp5 + tmp6;
    z3 = tmp4 + tmp6;
    z4 = tmp5 + tmp7;
    z5 = MULTIPLY(z3 + z4, FIX_1_175875602);

    tmp4 = MULTIPLY(tmp4, FIX_0_298631336);
    tmp5 = MULTIPLY(tmp5, FIX_2_053119869);
    tmp6 = MULTIPLY(tmp6, FIX_3_072711026);
    tmp7 = MULTIPLY(tmp7, FIX_1_501321110);
    z1   = MULTIPLY(z1, - FIX_0_899976223);
    z2   = MULTIPLY(z2, - FIX_2_562915447);
    z3   = MULTIPLY(z3, - FIX_1_961570560);
    z4   = MULTIPLY(z4, - FIX_0_390180644);

    z3 += z5;
    z4 += z5;

    data_ptr[8*7] = (int32) DESCALE(tmp4 + z1 + z3, CONST_BITS + PASS1_BITS + 3);
    data_ptr[8*5] = (int32) DESCALE(tmp5 + z2 + z4, CONST_BITS + PASS1_BITS + 3);
    data_ptr[8*3] = (int32) DESCALE(tmp6 + z2 + z3, CONST_BITS + PASS1_BITS + 3);
    data_ptr[8*1] = (int32) DESCALE(tmp7 + z1 + z4, CONST_BITS + PASS1_BITS + 3);

    data_ptr++;
  }
}
/*----------------------------------------------------------------------------*/

