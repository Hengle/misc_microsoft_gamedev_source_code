#include "varbits.h"

#if defined(_XBOX) || defined(__WIN32__) || defined(_WIN32)
#pragma code_seg("BINK")
#pragma data_seg("BINKDATA")
#pragma const_seg("BINKCONST")
#pragma const_seg("BINKBSS")
#endif


RADDEFINEDATA const RAD_ALIGN(U32, VarBitsLens[ 33 ], 32 ) =
{
           0,         1,           3,          7,
         0xf,      0x1f,        0x3f,       0x7f,
        0xff,     0x1ff,       0x3ff,      0x7ff,
       0xfff,    0x1fff,      0x3fff,     0x7fff,
      0xffff,    0x1ffff,    0x3ffff,    0x7ffff,
     0xfffff,   0x1fffff,   0x3fffff,   0x7fffff,
    0xffffff,  0x1ffffff,  0x3ffffff,  0x7ffffff,
   0xfffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
  0xffffffff
};

#if defined(__RADX86__)

RADDEFFUNC void VarBitsCopy(VARBITS* dest,VARBITS* src,U32 size)
{
  U32 val;
  while (size>=8) {
    VarBitsGet(val,U32,*src,8);
    VarBitsPut(*dest,val,8);
    size-=8;
  }

  if (size) {
    VarBitsGet(val,U32,*src,size);
    VarBitsPut(*dest,val,size);
  }
}

#endif

#if defined(__RADX86__) || defined(__RADPSP__) || defined(__RADNDS__) || defined(__RADSPU__)
//                                                                                                                                                                                                                               1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
//                                           1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3 3 3 4 4 4 4 4 4 4 4 4 4 5 5 5 5 5 5 5 5 5 5 6 6 6 6 6 6 6 6 6 6 7 7 7 7 7 7 7 7 7 7 8 8 8 8 8 8 8 8 8 8 9 9 9 9 9 9 9 9 9 9 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2
//                       0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8,9,0,1,2,3,4,5,6,7,8
RADDEFINEDATA const RAD_ALIGN(U8,_bitlevels[129],32)={0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8};
#endif

