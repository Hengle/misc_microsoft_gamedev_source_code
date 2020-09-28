#ifndef __RADMEMH__
  #define __RADMEMH__

  #ifndef __RADBASEH__
    #include "radbase.h"
  #endif

  RADDEFSTART

  typedef void PTR4* (RADLINK PTR4* RADMEMALLOC) (U32 bytes);
  typedef void       (RADLINK PTR4* RADMEMFREE)  (void PTR4* ptr);

  RADDEFFUNC void RADLINK RADSetMemory(RADMEMALLOC a,RADMEMFREE f);
  RADDEFFUNC void PTR4* RADLINK radmalloc(U32 numbytes);
  RADDEFFUNC void RADLINK radfree(void PTR4* ptr);

  RADDEFEND

#endif
