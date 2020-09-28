// File: tiger.cpp
// From: http://www.cs.technion.ac.il/~biham/Reports/Tiger/
// This is in the public domain.

#include "xcore.h"
#include "tiger.h"

typedef unsigned long word32;
typedef unsigned char byte;

/* Big endian:                                         */
#ifdef XBOX
   #define BIG_ENDIAN
#endif

/* The following macro denotes that an optimization    */
/* for Alpha is required. It is used only for          */
/* optimization of time. Otherwise it does nothing.    */
#ifdef XBOX
#define OPTIMIZE_FOR_ALPHA
#endif

/* NOTE that this code is NOT FULLY OPTIMIZED for any  */
/* machine. Assembly code might be much faster on some */
/* machines, especially if the code is compiled with   */
/* gcc.                                                */

/* The number of passes of the hash function.          */
/* Three passes are recommended.                       */
/* Use four passes when you need extra security.       */
/* Must be at least three.                             */
#define PASSES 3

extern word64 tigerTable[4*256];

#define t1 (tigerTable)
#define t2 (tigerTable+256)
#define t3 (tigerTable+256*2)
#define t4 (tigerTable+256*3)

#define save_abc \
      aa = a; \
      bb = b; \
      cc = c;

#ifdef OPTIMIZE_FOR_ALPHA
/* This is the official definition of round */
#define round(a,b,c,x,mul) \
      c ^= x; \
      a -= t1[((c)>>(0*8))&0xFF] ^ t2[((c)>>(2*8))&0xFF] ^ \
      t3[((c)>>(4*8))&0xFF] ^ t4[((c)>>(6*8))&0xFF] ; \
      b += t4[((c)>>(1*8))&0xFF] ^ t3[((c)>>(3*8))&0xFF] ^ \
      t2[((c)>>(5*8))&0xFF] ^ t1[((c)>>(7*8))&0xFF] ; \
      b *= mul;
#else
/* This code works faster when compiled on 32-bit machines */
/* (but works slower on Alpha) */
#define round(a,b,c,x,mul) \
      c ^= x; \
      a -= t1[(byte)(c)] ^ \
           t2[(byte)(((word32)(c))>>(2*8))] ^ \
      t3[(byte)((c)>>(4*8))] ^ \
           t4[(byte)(((word32)((c)>>(4*8)))>>(2*8))] ; \
      b += t4[(byte)(((word32)(c))>>(1*8))] ^ \
           t3[(byte)(((word32)(c))>>(3*8))] ^ \
      t2[(byte)(((word32)((c)>>(4*8)))>>(1*8))] ^ \
           t1[(byte)(((word32)((c)>>(4*8)))>>(3*8))]; \
      b *= mul;
#endif

#define pass(a,b,c,mul) \
      round(a,b,c,x0,mul) \
      round(b,c,a,x1,mul) \
      round(c,a,b,x2,mul) \
      round(a,b,c,x3,mul) \
      round(b,c,a,x4,mul) \
      round(c,a,b,x5,mul) \
      round(a,b,c,x6,mul) \
      round(b,c,a,x7,mul)

#define key_schedule \
      x0 -= x7 ^ 0xA5A5A5A5A5A5A5A5LL; \
      x1 ^= x0; \
      x2 += x1; \
      x3 -= x2 ^ ((~x1)<<19); \
      x4 ^= x3; \
      x5 += x4; \
      x6 -= x5 ^ ((~x4)>>23); \
      x7 ^= x6; \
      x0 += x7; \
      x1 -= x0 ^ ((~x7)<<19); \
      x2 ^= x1; \
      x3 += x2; \
      x4 -= x3 ^ ((~x2)>>23); \
      x5 ^= x4; \
      x6 += x5; \
      x7 -= x6 ^ 0x0123456789ABCDEFLL;

#define feedforward \
      a ^= aa; \
      b -= bb; \
      c += cc;

#ifdef OPTIMIZE_FOR_ALPHA
/* The loop is unrolled: works better on Alpha */
#define compress \
      save_abc \
      pass(a,b,c,5) \
      key_schedule \
      pass(c,a,b,7) \
      key_schedule \
      pass(b,c,a,9) \
      for(pass_no=3; pass_no<PASSES; pass_no++) { \
        key_schedule \
   pass(a,b,c,9) \
   tmpa=a; a=c; c=b; b=tmpa;} \
      feedforward
#else
/* loop: works better on PC and Sun (smaller cache?) */
#define compress \
      save_abc \
      for(pass_no=0; pass_no<PASSES; pass_no++) { \
        if(pass_no != 0) {key_schedule} \
   pass(a,b,c,(pass_no==0?5:pass_no==1?7:9)); \
   tmpa=a; a=c; c=b; b=tmpa;} \
      feedforward
#endif

#define tiger_compress_macro(str, state) \
{ \
  register word64 a, b, c, tmpa; \
  word64 aa, bb, cc; \
  register word64 x0, x1, x2, x3, x4, x5, x6, x7; \
  register word32 i; \
  int pass_no; \
\
  i; \
  a = state[0]; \
  b = state[1]; \
  c = state[2]; \
\
  x0=str[0]; x1=str[1]; x2=str[2]; x3=str[3]; \
  x4=str[4]; x5=str[5]; x6=str[6]; x7=str[7]; \
\
  compress; \
\
  state[0] = a; \
  state[1] = b; \
  state[2] = c; \
}

/* The compress function is a function. Requires smaller cache?    */
void tiger_compress(const word64 *str, word64 state[3])
{
  tiger_compress_macro(((word64*)str), ((word64*)state));
}

#ifdef OPTIMIZE_FOR_ALPHA
/* The compress function is inlined: works better on Alpha.        */
/* Still leaves the function above in the code, in case some other */
/* module calls it directly.                                       */
#define tiger_compress(str, state) \
  tiger_compress_macro(((word64*)str), ((word64*)state))
#endif

void tiger(const void* p, word64 length, word64 res[3])
{
  const word64 *str = static_cast<const word64*>(p);
  register word64 i, j;
  unsigned char temp[64];

  res[0]=0x0123456789ABCDEFULL;
  res[1]=0xFEDCBA9876543210ULL;
  res[2]=0xF096A5B4C3B2E187ULL;

  for(i=length; i>=64; i-=64)
    {
#ifdef BIG_ENDIAN
      for(j=0; j<64; j++)
         temp[j^7] = ((byte*)str)[j];
      tiger_compress(((word64*)temp), res);
#else
      tiger_compress(str, res);
#endif
      str += 8;
    }

#ifdef BIG_ENDIAN
  for(j=0; j<i; j++)
    temp[j^7] = ((byte*)str)[j];

  temp[j^7] = 0x01;
  j++;
  for(; j&7; j++)
    temp[j^7] = 0;
#else
  for(j=0; j<i; j++)
    temp[j] = ((byte*)str)[j];

  temp[j++] = 0x01;
  for(; j&7; j++)
    temp[j] = 0;
#endif
  if(j>56)
    {
      for(; j<64; j++)
   temp[j] = 0;
      tiger_compress(((word64*)temp), res);
      j=0;
    }

  for(; j<56; j++)
    temp[j] = 0;
  ((word64*)(&(temp[56])))[0] = ((word64)length)<<3;
  tiger_compress(((word64*)temp), res);
}

void BTigerHashGen::clear(void)
{
   mRes[0] = 0x0123456789ABCDEFULL;
   mRes[1] = 0xFEDCBA9876543210ULL;
   mRes[2] = 0xF096A5B4C3B2E187ULL; 
   mBufSize = 0;
   mLength = 0;
}

BTigerHashGen& BTigerHashGen::update(const void* p, uint len)
{
   BDEBUG_ASSERT(p);

   if (!len)
      return *this;

   BDEBUG_ASSERT(UINT64_MAX != mLength);
   mLength += len;

   const uchar* pBytes = static_cast<const uchar*>(p);

   if (mBufSize)
   {
      const uint bytesToCopy = Math::Min(64U - mBufSize, len);
      memcpy(mBuf + mBufSize, pBytes, bytesToCopy);

      mBufSize += bytesToCopy;
      pBytes += bytesToCopy;
      len -= bytesToCopy;

      if (mBufSize == 64)
      {
         compressBuf(reinterpret_cast<const word64*>(mBuf));
         mBufSize = 0;
      }
   }
   
   uint64 res[3];
   res[0] = mRes[0];
   res[1] = mRes[1];
   res[2] = mRes[2];

   while (len >= 64)
   {
      const word64* pStr = reinterpret_cast<const word64*>(pBytes);
      
#ifdef BIG_ENDIAN
      uchar temp[64];
      for(int j = 0; j < 64; j++)
         temp[j ^ 7] = ((const byte*)pStr)[j];
      tiger_compress(((word64*)temp), res);
#else
      tiger_compress(pStr, res);
#endif   
      
      pBytes += 64;
      len -= 64;
   }
   
   mRes[0] = res[0];
   mRes[1] = res[1];
   mRes[2] = res[2];

   if (len)
   {
      BDEBUG_ASSERT(!mBufSize);

      memcpy(mBuf, pBytes, len);
      mBufSize = len;
   }
   
   return *this;
}

BTigerHash BTigerHashGen::finalize(void)
{
   uchar temp[64];
   int i = mBufSize;
   int j;
   word64* str = (word64*)mBuf;

#ifdef BIG_ENDIAN
   for(j=0; j<i; j++)
      temp[j^7] = ((byte*)str)[j];

   temp[j^7] = 0x01;
   j++;
   for(; j&7; j++)
      temp[j^7] = 0;
#else
   for(j=0; j<i; j++)
      temp[j] = ((byte*)str)[j];

   temp[j++] = 0x01;
   for(; j&7; j++)
      temp[j] = 0;
#endif
   if(j>56)
   {
      for(; j<64; j++)
         temp[j] = 0;
      tiger_compress(((word64*)temp), mRes);
      j=0;
   }

   for(; j<56; j++)
      temp[j] = 0;
   ((word64*)(&(temp[56])))[0] = ((word64)mLength)<<3;
   tiger_compress(((word64*)temp), mRes);

   mLength = UINT64_MAX;
   return BTigerHash(mRes[0], mRes[1], mRes[2]);
}

inline void BTigerHashGen::compressBuf(const word64* pStr)
{
#ifdef BIG_ENDIAN
   uchar temp[64];
   for(int j = 0; j < 64; j++)
      temp[j ^ 7] = ((const byte*)pStr)[j];
   tiger_compress(((word64*)temp), mRes);
#else
   tiger_compress(pStr, mRes);
#endif   
}
