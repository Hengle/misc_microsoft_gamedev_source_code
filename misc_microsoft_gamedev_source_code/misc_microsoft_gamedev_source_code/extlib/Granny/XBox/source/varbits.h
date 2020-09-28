#ifndef __VARBITSH__
#define __VARBITSH__

#ifndef __RADBASEH__
#include "radbase.h"
#endif

#ifndef __RADMATHH__
#include "radmath.h"
#endif

// variable bit macros
RADDECLAREDATA const RAD_ALIGN(U32,VarBitsLens[33],32);

#if defined(__RADX86__) || defined(__RADPSP__) || defined(__RADNDS__) || defined(__RADSPU__)
RADDECLAREDATA const RAD_ALIGN(U8,_bitlevels[129],32);
#endif

//#define USE64BITVB
#ifdef USE64BITVB

#define BITSTYPE U64
#define BITSTYPELEN 64
#define BITSTYPEBYTES 8

//NOTE this is read-only on USE64BITVB!
#define VarBitsOpen(vb,pointer) { (vb).init=pointer; if (((U32)pointer)&4) { (vb).bits = *((U32* RADRESTRICT )pointer); (vb).cur = ((char*)pointer)+4; (vb).bitlen = 32; } else { (vb).cur=pointer; (vb).bits=(vb).bitlen=0; } }
#define VarBitsLocalOpen(vb,pointer) { if (((U32)pointer)&4) { vb##bits = *((U32 * RADRESTRICT)pointer); vb##cur = ((char*)pointer)+4; vb##bitlen = 32; } else { vb##cur=pointer; vb##bits=vb##bitlen=0; } }

#else

#define BITSTYPE U32
#define BITSTYPELEN 32
#define BITSTYPEBYTES 4
#define VarBitsOpen(vb,pointer) { (vb).cur=(vb).init=pointer; (vb).bits=(vb).bitlen=0; }

#define VarBitsLocalOpen(vb,pointer) { vb##cur=pointer; vb##bits=vb##bitlen=0; }

#endif

#define VARBITSTEMP BITSTYPE

typedef struct _VARBITS
{
  BITSTYPE bits;
  void* RADRESTRICT cur;
  U32 bitlen;
  void* RADRESTRICT init;
} VARBITS;

#define VarBitsPut(vb,val,size) { U32 __s=size; U32 __v=(val)&VarBitsLens[__s]; (vb).bits|=__v<<((vb).bitlen); (vb).bitlen+=__s; if ((vb).bitlen>=32) { *((U32*)(vb).cur)=(vb).bits; (vb).cur=((char*)((vb).cur)+4); (vb).bitlen-=32; (vb).bits=0; if ((vb).bitlen) { (vb).bits=__v>>(__s-(vb).bitlen); } } }
#define VarBitsPut1(vb,boolean) { if (boolean) (vb).bits|=(1<<(vb).bitlen); if ((++(vb).bitlen)==32) { *((U32*)(vb).cur)=(vb).bits; (vb).cur=((char*)((vb).cur)+4); (vb).bits=(vb).bitlen=0; } }
#define VarBitsPuta1(vb) { (vb).bits|=(1<<(vb).bitlen); if ((++(vb).bitlen)==32) { *((U32*)(vb).cur)=(vb).bits; (vb).cur=((char*)((vb).cur)+4); (vb).bits=(vb).bitlen=0; } }
#define VarBitsPuta0(vb) { if ((++(vb).bitlen)==32) { *((U32*)(vb).cur)=(vb).bits; (vb).cur=((char*)((vb).cur)+4); (vb).bits=(vb).bitlen=0; } }
#define VarBitsPutAlign(vb) { U32 __s2=(32-(vb).bitlen)&31; if (__s2) { VarBitsPut((vb),0,__s2);  } }
#define VarBitsFlush(vb) VarBitsPutAlign(vb)
#define VarBitsSize(vb) ((U32)( (((char*)(vb).cur)-((char*)(vb).init))*8 +(vb).bitlen ))

RADDEFFUNC void VarBitsCopy(VARBITS* dest,VARBITS* src,U32 size);


#if defined(__RADPPC__) 

  #if defined(__GNUC__)
    #define count_leading_zeros(count, x) \
      __asm__ ("{cntlz|cntlzw} %0,%1"     \
         : "=r" (count)                   \
          : "r" (x))

    static RADINLINE U32 getbitlevel128( register U32 n )
    {
      count_leading_zeros( n, n );
      return( 32 - n );
    }
  
    static RADINLINE U32 getbitlevel( register U32 n )
    {
      n = n & 0xffff;
      count_leading_zeros( n, n );
      return( 32 - n );
    }
  
  #else

    #define getbitlevel128(n) (U32) (32 - __cntlzw(n))
    #define getbitlevel(n)    (U32) (32 - __cntlzw((n) & 0x0000FFFF))

  #endif
  
#elif defined(__RADPS2__)

    #define count_leading_zeros(count, x) \
      __asm__ ("plzcw %0,%1"              \
         : "=r" (count)                   \
         : "r" (x))

    static RADINLINE U32 getbitlevel128( register U32 n )
    {
      count_leading_zeros( n, n );
      return( 31 - (n & 0xFF) );
    }

    static RADINLINE U32 getbitlevel( register U32 n )
    {
      n = n & 0xffff;
      count_leading_zeros( n, n );
      return( 31 - (n & 0xFF) );
    }

#else

#define getbitlevel128(n) (_bitlevels[n])

#define getbitlevel(level)      \
(                               \
  ((level)<=128)?               \
    (getbitlevel128(level))     \
  :                             \
  (                             \
    ((level)>=2048)?            \
    (                           \
      ((level)>=8192)?          \
      (                         \
        ((level)>=16384)?       \
          15                    \
        :                       \
          14                    \
      )                         \
      :                         \
      (                         \
        ((level)>=4096)?        \
          13                    \
        :                       \
          12                    \
      )                         \
    )                           \
    :                           \
    (                           \
      ((level)>=512)?           \
      (                         \
        ((level)>=1024)?        \
          11                    \
        :                       \
          10                    \
      )                         \
      :                         \
      (                         \
        ((level)>=256)?         \
          9                     \
        :                       \
          8                     \
      )                         \
    )                           \
  )                             \
)

#endif


#define VarBitsGetAlign(vb) { (vb).bitlen=0; }
#define VarBitsPos(vb) ((U32)( (((U8*)(vb).cur)-((U8*)(vb).init))*8-(vb).bitlen ))

// don't pass zero to this function
#define GetBitsLen(val) (((U32)0xffffffff)>>(U32)(32-(val)))
// for debugging: causes crash on zero: #define GetBitsLen(val) (((val)==0)?(((U8*)val)[0]=0):(0xffffffffL>>(U32)(32-(val))))

#define VarBitsGet1(vb,i)             \
(                                     \
  ((vb).bitlen==0)?                   \
  (                                   \
    i=*((BITSTYPE* RADRESTRICT)((vb).cur)),       \
    ((vb).cur)=((char*)((vb).cur))+BITSTYPEBYTES, \
    ((vb).bits)=((BITSTYPE)i)>>1,     \
    ((vb).bitlen)=(BITSTYPELEN-1)     \
  ):(                                 \
    i=((vb).bits),                    \
    ((vb).bits)>>=1,                  \
    --((vb).bitlen)                   \
  ),(i&1)                             \
)

//not USE64BITVB safe!
#define VarBitsGet1LE(vb,i)           \
(                                     \
  ((vb).bitlen==0)?                   \
  (                                   \
    i=radloadu32ptr((vb).cur),        \
    ((vb).cur)=((char*)((vb).cur))+4, \
    ((vb).bits)=((U32)i)>>1,          \
    ((vb).bitlen)=31                  \
  ):(                                 \
    i=((vb).bits),                    \
    ((vb).bits)>>=1,                  \
    --((vb).bitlen)                   \
  ),(i&1)                             \
)


#define VarBitsGet(v,typ,vb,len)                                \
{                                                               \
  if (((vb).bitlen)<(len)) {                                    \
    register BITSTYPE nb=*((BITSTYPE* RADRESTRICT)((vb).cur));  \
    v=(typ)((((vb).bits)|(nb<<((vb).bitlen)))&GetBitsLen(len)); \
    ((vb).bits)=nb>>((len)-((vb).bitlen));                      \
    ((vb).bitlen)=((vb).bitlen)+BITSTYPELEN-(len);              \
    ((vb).cur)=((char*)((vb).cur))+BITSTYPEBYTES;               \
  } else {                                                      \
    v=(typ)(((vb).bits)&GetBitsLen(len));                       \
    ((vb).bits)>>=(len);                                        \
    ((vb).bitlen)-=(len);                                       \
  }                                                             \
}

#define VarBitsGetWithCheck(v,typ,vb,len,endp,dowhat)           \
{                                                               \
  if (((vb).bitlen)<(len)) {                                    \
    register BITSTYPE nb;                                       \
    if ( ( (U8*)((vb).cur) ) >= ( (U8*) (endp) ) ) dowhat       \
    nb=*((BITSTYPE* RADRESTRICT)((vb).cur));                    \
    v=(typ)((((vb).bits)|(nb<<((vb).bitlen)))&GetBitsLen(len)); \
    ((vb).bits)=nb>>((len)-((vb).bitlen));                      \
    ((vb).bitlen)=((vb).bitlen)+BITSTYPELEN-(len);              \
    ((vb).cur)=((char*)((vb).cur))+BITSTYPEBYTES;               \
  } else {                                                      \
    v=(typ)(((vb).bits)&GetBitsLen(len));                       \
    ((vb).bits)>>=(len);                                        \
    ((vb).bitlen)-=(len);                                       \
  }                                                             \
}

#define VarBitsPeek(v,typ,vb,len)                               \
{                                                               \
  if (((vb).bitlen)<(len)) {                                    \
    register BITSTYPE nb=*((BITSTYPE* RADRESTRICT)((vb).cur));              \
    v=(typ)((((vb).bits)|(nb<<((vb).bitlen)))&GetBitsLen(len)); \
  } else {                                                      \
    v=(typ)(((vb).bits)&GetBitsLen(len));                       \
  }                                                             \
}

//not USE64BITVB safe!
#define VarBitsGetLE(v,typ,vb,len)                              \
{                                                               \
  if (((vb).bitlen)<(len)) {                                    \
    register U32 nb=radloadu32ptr((vb).cur);                    \
    v=(typ)((((vb).bits)|(nb<<((vb).bitlen)))&GetBitsLen(len)); \
    ((vb).bits)=nb>>((len)-((vb).bitlen));                      \
    ((vb).bitlen)=((vb).bitlen)+32-(len);                       \
    ((vb).cur)=((char*)((vb).cur))+4;                           \
  } else {                                                      \
    v=(typ)(((vb).bits)&GetBitsLen(len));                       \
    ((vb).bits)>>=(len);                                        \
    ((vb).bitlen)-=(len);                                       \
  }                                                             \
}


#define VarBitsUse(vb,len)                                      \
{                                                               \
  if (((vb).bitlen)<(len)) {                                    \
    register BITSTYPE nb=*((BITSTYPE* RADRESTRICT)((vb).cur));  \
    ((vb).bits)=nb>>((len)-((vb).bitlen));                      \
    ((vb).bitlen)=((vb).bitlen)+BITSTYPELEN-(len);              \
    ((vb).cur)=((char*)((vb).cur))+BITSTYPEBYTES;               \
  } else {                                                      \
    ((vb).bits)>>=(len);                                        \
    ((vb).bitlen)-=(len);                                       \
  }                                                             \
}


#define VARBITSLOCAL(name) void * name##cur; BITSTYPE name##bits; U32 name##bitlen


#define VarBitsLocalGet(v,typ,vb,len)                           \
{                                                               \
  if ((vb##bitlen)<len) {                                       \
    register BITSTYPE nb=*((BITSTYPE* RADRESTRICT)(vb##cur));   \
    v=(typ)(((vb##bits)|(nb<<(vb##bitlen)))&GetBitsLen(len));   \
    (vb##bits)=nb>>((len)-(vb##bitlen));                        \
    (vb##bitlen)=(vb##bitlen)+BITSTYPELEN-(len);                \
    (vb##cur)=((char*)(vb##cur))+BITSTYPEBYTES;                 \
  } else {                                                      \
    v=(typ)((vb##bits)&GetBitsLen(len));                        \
    (vb##bits)>>=(len);                                         \
    (vb##bitlen)-=(len);                                        \
  }                                                             \
}

#define VarBitsLocalGetWithCheck(v,typ,vb,len,endp,dowhat)      \
{                                                               \
  if ((vb##bitlen)<len) {                                       \
    register BITSTYPE nb;                                       \
    if ( ( (U8*)(vb##cur) ) >= ( (U8*) (endp) ) ) dowhat        \
    nb=*((BITSTYPE* RADRESTRICT)(vb##cur));                     \
    v=(typ)(((vb##bits)|(nb<<(vb##bitlen)))&GetBitsLen(len));   \
    (vb##bits)=nb>>((len)-(vb##bitlen));                        \
    (vb##bitlen)=(vb##bitlen)+BITSTYPELEN-(len);                \
    (vb##cur)=((char*)(vb##cur))+BITSTYPEBYTES;                 \
  } else {                                                      \
    v=(typ)((vb##bits)&GetBitsLen(len));                        \
    (vb##bits)>>=(len);                                         \
    (vb##bitlen)-=(len);                                        \
  }                                                             \
}


#define VarBitsLocalGet1WithCheck(v,vb,endp,dowhat)             \
{                                                               \
  if ((vb##bitlen)==0) {                                        \
    if ( ( (U8*)(vb##cur) ) >= ( (U8*) (endp) ) ) dowhat        \
    (vb##bits)=*((BITSTYPE* RADRESTRICT)(vb##cur));             \
    (vb##cur)=((char*)(vb##cur))+BITSTYPEBYTES;                 \
    (vb##bitlen)=(BITSTYPELEN);                                 \
  }                                                             \
  --(vb##bitlen);                                               \
  v=(vb##bits);                                                 \
  (vb##bits)>>=1;                                               \
  v&=1;                                                         \
}


// get the value, if the next bit is X (mask bits with mask)
//   usually, you will use the two wrapper macros below

#define VarBitsLocalGetIfxSM(v,typ,vb,len,i,x,mask)            \
  (vb##bitlen==0)?                                             \
  (                                                            \
    i=*((BITSTYPE* RADRESTRICT)(vb##cur)),                     \
    (vb##cur)=((char*)(vb##cur))+BITSTYPEBYTES,                \
    ( ( i & 1 ) == x ) ?                                       \
    (                                                          \
      v = (typ)( ( i >> 1 ) mask ),                            \
      (vb##bits)=((BITSTYPE)i)>>(len+1),                       \
      (vb##bitlen)=(BITSTYPELEN - 1 - len),                    \
      x                                                        \
    ):(                                                        \
      (vb##bits)=(((BITSTYPE)i)>>1),                           \
      (vb##bitlen)=(BITSTYPELEN-1),                            \
      !x                                                       \
    )                                                          \
  ):(                                                          \
    --(vb##bitlen),                                            \
    ( ( (vb##bits) & 1 ) == x ) ?                              \
    (                                                          \
      ( ( vb##bitlen ) < len )?                                \
      (                                                        \
        i=*((BITSTYPE* RADRESTRICT)(vb##cur)),                 \
       (vb##cur)=(((char*)(vb##cur))+BITSTYPEBYTES ),          \
        v=(typ)(((vb##bits>>1)|(i<<vb##bitlen)) mask ),        \
       (vb##bits)=(i>>((len)-vb##bitlen)),                     \
       (vb##bitlen)=((vb##bitlen)+BITSTYPELEN-(len)),          \
       x                                                       \
      ):(                                                      \
        v = (typ) ( ( (vb##bits) >> 1 ) mask ),                \
        (vb##bits)>>=(len+1),                                  \
        (vb##bitlen)-=len,                                     \
        x                                                      \
      )                                                        \
    ):(                                                        \
      (vb##bits)>>=1,                                          \
      !x                                                       \
    )                                                          \
  )                                                            \


// get the value, if the next bit is x (0 or 1)

#define VarBitsLocalGetIfx(v,typ,vb,len,i,x)            \
  VarBitsLocalGetIfxSM(v,typ,vb,len,i,x,&GetBitLen(len))

// get the value, if the next bit is x (0 or 1), but don't
//   mask off the high bits of the value 

#define VarBitsLocalGetIfxNM(v,typ,vb,len,i,x)            \
  VarBitsLocalGetIfxSM(v,typ,vb,len,i,x, )


#define VarBitsLocalPeek(v,typ,vb,len)                          \
{                                                               \
  if ((vb##bitlen)<(len)) {                                     \
    register BITSTYPE nb=*((BITSTYPE* RADRESTRICT)(vb##cur));   \
    v=(typ)(((vb##bits)|(nb<<(vb##bitlen)))&GetBitsLen(len));   \
  } else {                                                      \
    v=(typ)((vb##bits)&GetBitsLen(len));                        \
  }                                                             \
}

#define VarBitsLocalGet1(vb,i)                                 \
(                                                              \
  (vb##bitlen==0)?                                             \
  (                                                            \
    i=*((BITSTYPE* RADRESTRICT)(vb##cur)),                     \
    (vb##cur)=((char*)(vb##cur))+BITSTYPEBYTES,                \
    (vb##bits)=((BITSTYPE)i)>>1,                               \
    (vb##bitlen)=(BITSTYPELEN-1)                               \
  ):(                                                          \
    i=(vb##bits),                                              \
    (vb##bits)>>=1,                                            \
    --(vb##bitlen)                                             \
  ),(i&1)                                                      \
)

#define VarBitsLocalUse(vb,len)                                \
{                                                              \
  if ((vb##bitlen)<(len)) {                                    \
    register BITSTYPE nb=*((BITSTYPE* RADRESTRICT)(vb##cur));  \
    (vb##bits)=nb>>((len)-(vb##bitlen));                       \
    (vb##bitlen)=(vb##bitlen)+BITSTYPELEN-(len);               \
    (vb##cur)=((char*)(vb##cur))+BITSTYPEBYTES;                \
  } else {                                                     \
    (vb##bits)>>=(len);                                        \
    (vb##bitlen)-=(len);                                       \
  }                                                            \
}



#define VarBitsCopyToLocal( local, vb ) local##cur = (vb).cur; local##bits = (vb).bits; local##bitlen = (vb).bitlen;

#define VarBitsCopyFromLocal( vb, local )  (vb).cur = local##cur;  (vb).bits = local##bits;  (vb).bitlen = local##bitlen;




// classifies a signed value into: 0 = zero, 1 = neg, 2 = pos
#define CLASSIFY_SIGN( val )  ( (((U32)((S32)(val))) >> 31 ) + ((((U32)(-(S32)(val))) >> 30 ) & 2 ) )

#endif
