#ifndef __RADMATHH__
#define __RADMATHH__

#ifndef __RADBASEH__
#include "radbase.h"
#endif

#include <math.h>

RADDEFSTART

#ifdef __RAD32__

  #ifdef __RADNGC__

    #if !defined(__MWERKS__)

      static inline S32 __cntlzw( S32 arg )
      {
        U32 val;
        asm (
          "cntlzw %0,%1"
          : "=r" (val)
          : "r" (arg) );
        return val;
      }
	  
      static inline float __frsqrte( float arg )
      {
        float val;
        asm (
          "frsqrte %0,%1"
          : "=f" (val)
          : "f" (arg) );
        return val;
      }

      static RADINLINE U32 __lwbrx( const void *rB )
      {
        U32 val;
  
        asm
        (
          "lwbrx %0,0,%1"
          : "=r" (val)
          : "r" (rB)
        );
  
        return( val );
      }
  
    #endif // __MWERKS__

    U32 mult64anddiv( U32 mt1, U32 mt2, U32 d );
    U32 mult64andshift( U32 mt1, U32 mt2, U32 shift );

    static inline float radfsqrt( float val )
    {
      if( val > 0.0f )
      {
        double est = __frsqrte( val );  // estimate
        est = est * 0.5 * ( 3.0 - est * est * val ); //12
        est = est * 0.5 * ( 3.0 - est * est * val ); //24
        est = est * 0.5 * ( 3.0 - est * est * val ); //32
        return( (float) ( est * val ) );
      }
      return( val );
    }

    #define radabs abs

    #define radcos( val ) cosf( val )
    #define radsin( val ) sinf( val )
    #define radatan( val ) atanf( val )
    #define radatan2( val1, val2 ) atanf2( val1, val2 )
    #define radpow( val1, val2 ) powf( val1, val2 )
    #define radlog( val ) logf( val )
    #define radlog10( val ) log10f( val )
    #define radexp( val ) expf( val )
    #define radfabs( val ) fabsf( val )

    #define radfloor( val ) floorf( val )
    #define radceil( val ) ceilf( val )

    #define LN2 0.693147181F

    static F64 inline radlog2( F64 X )
    {
      return( radlog( X ) / LN2 );
    }

    #define rlwinm24023(d,s)          \
      __asm__ ("rlwinm %0,%1,24,0,23" \
        : "=r" (d)                      \
        : "r" (s))

    #define rlwinm82431(d,s)          \
      __asm__ ("rlwinm %0,%1,8,24,31" \
        : "=r" (d)                      \
        : "r" (s))

    #define rlwimi8815(d,s)           \
      __asm__ ("rlwimi %0,%1,8,8,15"  \
        : "=r" (d)                      \
        : "r" (s))

    static U32 RADINLINE radloadu32(register U32 x) 
    {
      register U32 t1, t2;
      rlwinm24023(t1,x);
      rlwinm82431(t2,x);
      rlwimi8815(t1,x);
      return( t1 | t2 );
    }

    #define radloadu32ptr(p) (U32) __lwbrx((p))

  #elif defined(__RADWII__)

    U32 mult64anddiv( U32 mt1, U32 mt2, U32 d );
    U32 mult64andshift( U32 mt1, U32 mt2, U32 shift );

    static inline float radfsqrt( float val )
    {
      if( val > 0.0f )
      {
        double est = __frsqrte( val );  // estimate
        est = est * 0.5 * ( 3.0 - est * est * val ); //12
        est = est * 0.5 * ( 3.0 - est * est * val ); //24
        est = est * 0.5 * ( 3.0 - est * est * val ); //32
        return( (float) ( est * val ) );
      }
      return( val );
    }

    #define radabs __labs

    #define radcos( val ) cosf( val )
    #define radsin( val ) sinf( val )
    #define radatan( val ) atanf( val )
    #define radatan2( val1, val2 ) atanf2( val1, val2 )
    #define radpow( val1, val2 ) powf( val1, val2 )
    #define radlog( val ) logf( val )
    #define radlog10( val ) log10f( val )
    #define radexp( val ) expf( val )
    #define radfabs( val ) fabsf( val )

    #define radfloor( val ) floorf( val )
    #define radceil( val ) ceilf( val )

    #define LN2 0.693147181F

    static F64 inline radlog2( F64 X )
    {
      return( radlog( X ) / LN2 );
    }

    #define rlwinm24023(d,s)          \
      __asm__ ("rlwinm %0,%1,24,0,23" \
        : "=r" (d)                      \
        : "r" (s))

    #define rlwinm82431(d,s)          \
      __asm__ ("rlwinm %0,%1,8,24,31" \
        : "=r" (d)                      \
        : "r" (s))

    #define rlwimi8815(d,s)           \
      __asm__ ("rlwimi %0,%1,8,8,15"  \
        : "=r" (d)                      \
        : "r" (s))

    static U32 RADINLINE radloadu32(register U32 x) 
    {
      register U32 t1, t2;
      rlwinm24023(t1,x);
      rlwinm82431(t2,x);
      rlwimi8815(t1,x);
      return( t1 | t2 );
    }

    #define radloadu32ptr(p) (U32) __lwbrx((void*)(p), 0)

  #elif defined(__RADNDS__)

      U32 mult64anddiv( U32 mt1, U32 mt2, U32 div );
      U32 mult64andshift( U32 mt1, U32 mt2, U32 shift );
      #define radcos( val ) (float)cosf( val )
      #define radsin( val ) (float)sinf( val )
      #define radatan( val ) (float)atanf( val )
      #define radatan2( val1, val2 ) (float)atan2f( val1, val2 )
      #define radpow( val1, val2 ) (float)powf( val1, val2 )
      #define radfsqrt( val ) (float)sqrtf( val )
      #define radlog( val ) (float)logf( val )
      #define radlog10( val ) (float)log10f( val )
      #define radexp( val ) (float)expf( val )

  #elif defined( __RADPS2__ )

      U32 mult64anddiv( U32 mt1, U32 mt2, U32 div );
      U32 mult64andshift( U32 mt1, U32 mt2, U32 shift );
      F32 radfsqrt( F32 x );
      F32 radcos( F32 x );
      F32 radsin( F32 x );
      F32 radfloor( F32 x );
      F32 radceil( F32 x );
      F32 radatan( F32 x );

      #define radatan2( val1, val2 ) atanf2( val1, val2 )
      #define radpow( val1, val2 ) powf( val1, val2 )
      #define radlog( val ) logf( val )
      #define radlog10( val ) log10f( val )
      #define radexp( val ) expf( val )
      #define radfabs( val ) fabsf( val )
      F32 radlog2( F32 val );

      #define radabs abs

      #define radloadu32(a) ((U32)(a))
      #define radloadu32ptr(p) *((U32*)(p))

  #elif defined( __RADPSP__ )
      #include <stdlib.h>

      U32 mult64anddiv( U32 mt1, U32 mt2, U32 div );
      U32 mult64andshift( U32 mt1, U32 mt2, U32 shift );

      #define radfsqrt( val ) (float)sqrtf( val )
      #define radfloor( val ) floorf( val )
      #define radceil( val ) ceilf( val )
      #define radcos( val ) cosf( val )
      #define radsin( val ) sinf( val )
      #define radatan2( val1, val2 ) atanf2( val1, val2 )
      #define radpow( val1, val2 ) powf( val1, val2 )
      #define radlog( val ) logf( val )
      #define radlog10( val ) log10f( val )
      #define radexp( val ) expf( val )
      #define radfabs( val ) fabsf( val )

      #define radabs abs

      #define radloadu32(a) ((U32)(a))
      #define radloadu32ptr(p) *((U32*)(p))

  #elif defined( __RADPS3__ )
      #include <stdlib.h>

      #define mult64anddiv( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) / ( (U64) c ) ) )
      #define mult64andshift( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) >> ( (U64) c ) ) )

      static inline float __frsqrte( float arg )
      {
        float val;
        asm (
          "frsqrte %0,%1"
          : "=f" (val)
          : "f" (arg) );
        return val;
      }

      static inline float radfsqrt( float val )
      {
        if( val > 0.0f )
        {
          double est = __frsqrte( val );  // estimate
          est = est * 0.5 * ( 3.0 - est * est * val ); //12
          est = est * 0.5 * ( 3.0 - est * est * val ); //24
          est = est * 0.5 * ( 3.0 - est * est * val ); //32
          return( (float) ( est * val ) );
        }
        return( val );
      }

      #define radfloor( val ) floorf( val )
      #define radceil( val ) ceilf( val )
      #define radcos( val ) cosf( val )
      #define radsin( val ) sinf( val )
      #define radatan2( val1, val2 ) atanf2( val1, val2 )
      #define radpow( val1, val2 ) powf( val1, val2 )
      #define radlog( val ) logf( val )
      #define radlog10( val ) log10f( val )
      #define radexp( val ) expf( val )
      #define radfabs( val ) fabsf( val )
      F32 radlog2( F32 val );

      #define radabs abs

      static RADINLINE U32 __lwbrx( const void *rB )
      {
        U32 val;

        asm (
          "lwbrx %0,0,%1"
          : "=r" (val)
          : "r" (rB)
        );

        return( val );
      }

      #define rlwinm24023(d,s)                \
        __asm__ ("rlwinm %0,%1,24,0,23" \
          : "=r" (d)                      \
          : "r" (s))

      #define rlwinm82431(d,s)          \
        __asm__ ("rlwinm %0,%1,8,24,31" \
          : "=r" (d)                      \
          : "r" (s))

      #define rlwimi8815(d,s)           \
        __asm__ ("rlwimi %0,%1,8,8,15"  \
          : "=r" (d)                      \
          : "r" (s))

      static __inline U32 radloadu32(register U32 x) 
      {
        register U32 t1, t2;
        rlwinm24023(t1,x);
        rlwinm82431(t2,x);
        rlwimi8815(t1,x);
        return( t1 | t2 );
      }

      #define radloadu32ptr(p) (U32) __lwbrx((p))

  #elif defined( __RADSPU__ )

      #define mult64anddiv( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) / ( (U64) c ) ) )
      #define mult64andshift( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) >> ( (U64) c ) ) )

      static inline vector float VecInvSquareRootFast(vector float Value)
      {
          vector float Half   = spu_splats(0.5f);
          vector float Three  = spu_splats(3.0f);

          vector float Vec = spu_rsqrte(Value);
          Vec = Vec * Half * ( Three - Vec * Vec * Value ); //12
          Vec = Vec * Half * ( Three - Vec * Vec * Value ); //24

          return Vec;
      }

      static inline float radfsqrt( float val )
      {
        return val * VecInvSquareRootFast(spu_splats( val ))[0];
      }

      #define radfloor( val ) floorf( val )
      #define radceil( val ) ceilf( val )
      #define radcos( val ) cosf( val )
      #define radsin( val ) sinf( val )
      #define radatan2( val1, val2 ) atanf2( val1, val2 )
      #define radpow( val1, val2 ) powf( val1, val2 )
      #define radlog( val ) logf( val )
      #define radlog10( val ) log10f( val )
      #define radexp( val ) expf( val )
      #define radfabs( val ) fabsf( val )
      F32 radlog2( F32 val );

      #define radabs abs

  #elif defined( __RADMAC__ ) && !defined(__RADX86__)

    #if defined(__GNUG__) || defined(__GNUC__)
      #define radcos( val ) (float)cos( val )
      #define radsin( val ) (float)sin( val )
      #define radatan( val ) (float)atan( val )
      #define radatan2( val1, val2 ) (float)atan2( val1, val2 )
      #define radpow( val1, val2 ) (float)pow( val1, val2 )
      #define radfsqrt( val ) (float)sqrt( val )
      #define radlog( val ) (float)log( val )
      #define radlog10( val ) (float)log10( val )
      #define radexp( val ) (float)exp( val )
    #else
      #define radcos( val ) cosf( val )
      #define radsin( val ) sinf( val )
      #define radatan( val ) atanf( val )
      #define radatan2( val1, val2 ) atanf2( val1, val2 )
      #define radpow( val1, val2 ) powf( val1, val2 )
      #define radfsqrt( val ) sqrtf( val )
      #define radlog( val ) logf( val )
      #define radlog10( val ) log10f( val )
      #define radexp( val ) expf( val )
    #endif

    #define radfabs( val ) fabsf( val )

    #define radfloor( val ) floorf( val )
    #define radceil( val ) ceilf( val )

    void radconv32a(void* p, U32 n);

    U32 radloadu32ptr(U32* p);

    #ifdef __RAD68K__
      #pragma parameter radconv32a(__A0,__D0)
      void radconv32a(void* p,U32 n) ={0x4A80,0x600C,0x2210,0xE059,0x4841,0xE059,0x20C1,0x5380,0x6EF2};
      // tst.l d0  bra.s @loope  @loop:  move.l (a0),d1  ror.w #8,d1  swap d1 ror.w #8,d1  move.l d1,(a0)+  sub.l #1,d0  bgt.s @loop  @loope:
    #endif

    #if defined __RADPPC__ 
      #if defined(__GNUC__)

      static RADINLINE U32 __lwbrx( const void *rB )
      {
        U32 val;

        asm
        (
          "lwbrx %0,0,%1"
          : "=r" (val)
          : "r" (rB)
        );

        return( val );
      }

        #define rlwinm24023(d,s)          \
          __asm__ ("rlwinm %0,%1,24,0,23" \
            : "=r" (d)                      \
            : "r" (s))

        #define rlwinm82431(d,s)          \
          __asm__ ("rlwinm %0,%1,8,24,31" \
            : "=r" (d)                      \
            : "r" (s))

        #define rlwimi8815(d,s)           \
          __asm__ ("rlwimi %0,%1,8,8,15"  \
            : "=r" (d)                      \
            : "r" (s))

        static U32 RADINLINE radloadu32(register U32 x) 
        {
          register U32 t1, t2;
          rlwinm24023(t1,x);
          rlwinm82431(t2,x);
          rlwimi8815(t1,x);
          return( t1 | t2 );
        }

      #else
    
        static U32 RADINLINE radloadu32(register U32 x) {
          register U32 t1, t2;
          asm {                   // x  = aa bb cc dd
            rlwinm t1,x,24,0,23   // t1 = dd aa bb 00
            rlwinm t2,x,8,24,31   // t2 = 00 00 00 aa
            rlwimi t1,x,8, 8,15   // t1 = dd cc bb 00
            or x,t1,t2            // x  = dd cc bb aa
          }
          return x;
        }
      #endif
    #elif defined(__RADX86__)
      static U32 RADINLINE radloadu32(register U32 x) { return x; }
    #else
      static U32 RADINLINE radloadu32(register U32 x) {
        return (((x << 24) & 0xFF000000) |
                ((x <<  8) & 0x00FF0000) |
                ((x >>  8) & 0x0000FF00) |
                ((x >> 24) & 0x000000FF));
      }
    #endif

    #if defined(__RADPPC__)
      #define radloadu32ptr(p) (U32) __lwbrx((p))
    #else
      #define radloadu32ptr(p) radloadu32(*(U32*)(p))
    #endif

    #ifdef __RADALLOWINLINES__
      static U32 RADINLINE radsqr(U32 a) {  return(a*a);  }
    #endif

    #ifdef __RAD68K__
      #pragma parameter __D0 mult64anddiv(__D0,__D1,__D2)
      U32 mult64anddiv(U32 mt1,U32 mt2,U32 d) ={0x4C01,0x0C01,0x4C42,0x0C01};
      //  muls.l d1,d1:d0  divs.l d2,d1:d0
    #else
      U32 mult64anddiv( U32 mt1, U32 mt2, U32 d );
      U32 mult64andshift( U32 mt1, U32 mt2, U32 shift );
    #endif

    #if defined(__RADPPC__) && !defined(__GNUC__) && !defined(__RADXENON__)
      #define radabs(ab) __abs((S32)(ab))
    #else
      static S32 RADINLINE radabs(S32 ab) { return( ( ab ^ (ab>>31) ) - (ab>>31) ); }
    #endif

    #define radlog2( x ) log2( x )
    
    #define LN2 0.693147181F

  #elif defined(__RADXENON__)
    #include <ppcintrinsics.h>
  
    #define mult64anddiv( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) / ( (U64) c ) ) )
    #define mult64andshift( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) >> ( (U64) c ) ) )
     
    #define __cntlzw _CountLeadingZeros
 
    static __inline float radfsqrt( float val )
    {
      if( val > 0.0f )
      {
        double est = __frsqrte( val );  // estimate
        est = est * 0.5 * ( 3.0 - est * est * val ); //12
        est = est * 0.5 * ( 3.0 - est * est * val ); //24
        est = est * 0.5 * ( 3.0 - est * est * val ); //32
        return( (float) ( est * val ) );
      }
      return( val );
    }

    #define radabs abs

    #define radcos( val ) cosf( val )
    #define radsin( val ) sinf( val )
    #define radatan( val ) atanf( val )
    #define radatan2( val1, val2 ) atanf2( val1, val2 )
    #define radpow( val1, val2 ) powf( val1, val2 )
    #define radlog( val ) log( val )
    #define radlog10( val ) log10f( val )
    #define radexp( val ) expf( val )
    #define radfabs( val ) fabsf( val )

    #define radfloor( val ) floorf( val )
    #define radceil( val ) ceilf( val )

    #define LN2 0.693147181F

    static F64 __inline radlog2( F64 X )
    {
      return( radlog( X ) / LN2 );
    }
    static U32 RADINLINE radloadu32(register U32 x) 
    {
      return( _byteswap_ulong( x ) );
    }

    #define radloadu32ptr(p) (U32) __loadwordbytereverse(0,(p))

  #elif defined( __RADX64__ )

    #pragma intrinsic(abs, log, fabs, sqrt, fmod, sin, cos, tan, asin, acos, atan, atan2, exp)

    #define radcos( val ) cos( (float)(val) )
    #define radsin( val ) sin( (float)(val) )
    #define radatan( val ) atan( (float)(val) )
    #define radatan2( val1, val2 ) atan2( (float)(val1), (float)(val2) )
    #define radpow( val1, val2 ) pow( (float)(val1), (float)(val2) )
    #define radfsqrt( val ) sqrt( (float)(val) )
    #define radlog( val ) log( (float)(val) )
    #define radlog10( val ) log10( (float)(val) )
    #define radexp( val ) exp( (float)(val) )

    #define radfabs( val ) fabs( (float)(val) )

    #define radfloor( val ) floor( (float)(val) )
    #define radceil( val ) ceil( (float)(val) )

    #define radconv32a(p,n) ((void)0)

    #define radloadu32(a) ((U32)(a))

    #define radloadu32ptr(p) *((U32*)(p))


    #define mult64anddiv( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) / ( (U64) c ) ) )
    #define mult64andshift( a, b, c )  ( (U32) ( ( ( (U64) a ) * ( (U64) b ) ) >> ( (U64) c ) ) )

    #define radabs abs

    #define LN2 0.693147181F

    static F64 __inline radlog2( F64 X )
    {
      return( radlog( X ) / LN2 );
    }

    #define __cntlzw _CountLeadingZeros

  #elif defined( __RADX86__ )

    #define radcos( val ) cos( (float)(val) )
    #define radsin( val ) sin( (float)(val) )
    #define radatan( val ) atan( (float)(val) )
    #define radatan2( val1, val2 ) atan2( (float)(val1), (float)(val2) )
    #define radpow( val1, val2 ) pow( (float)(val1), (float)(val2) )
    #define radfsqrt( val ) sqrt( (float)(val) )
    #define radlog( val ) log( (float)(val) )
    #define radlog10( val ) log10( (float)(val) )
    #define radexp( val ) exp( (float)(val) )

    #define radfabs( val ) fabs( (float)(val) )

    #define radfloor( val ) floor( (float)(val) )
    #define radceil( val ) ceil( (float)(val) )

    #define radconv32a(p,n) ((void)0)

    #define radloadu32(a) ((U32)(a))

    #define radloadu32ptr(p) *((U32*)(p))


    #ifdef __RADLINUX__
      #include <stdlib.h>
      #define radabs abs
      #define LN2 0.693147181F

      static F64 inline radlog2( F64 X )
      {
        return( radlog( X ) / LN2 );
      }
    #endif


    #ifdef __WATCOMC__

      U32 radsqr(S32 a);
      #pragma aux radsqr = "mul eax" parm [eax] modify [EDX eax];

      U32 mult64anddiv(U32 mt1,U32 mt2,U32 d);
      #pragma aux mult64anddiv = "mul ecx" "div ebx" parm [eax] [ecx] [ebx] modify [EDX eax];

      U32 mult64andshift(U32 mt1,U32 mt2,U32 shift);
      #pragma aux mult64andshift = "mul ebx" "shrd eax,edx,cl" parm [eax] [ebx] [ecx] modify [EDX eax];

      S32 radabs(S32 ab);
      #pragma aux radabs = "cdq" "xor eax,edx" "sub eax,edx" parm [eax] modify [edx eax];

      #define radisqrt(num) ((U32)sqrt((double)(U32)(num)))
    #else

      #if defined(_MSC_VER) || defined(__RADMAC__)

        #pragma warning( disable : 4035)
        #pragma warning( disable : 4514) // unreferenced inline function removed.

        #pragma intrinsic(abs, log, fabs, sqrt, fmod, sin, cos, tan, asin, acos, atan, atan2, exp)

        static U32 __inline radsqr(U32 m)
        {
          __asm
          {
            mov eax,[m]
            mul eax
          }
        }

        static U32 __inline mult64anddiv( U32 mt1, U32 mt2, U32 d )
        {
          __asm
          {
            mov eax,[mt1]
            mov ecx,[mt2]
            mul ecx
            mov ecx,[d]
            div ecx
          }
        }

        static U32 __inline mult64andshift( U32 mt1, U32 mt2, U32 shift )
        {
          __asm
          {
            mov eax,[mt1]
            mov ecx,[mt2]
            mul ecx
            mov ecx,[shift]
            shrd eax,edx,cl
          }
        }

        #define radabs abs

        #if !defined(__RADMAC__)
                static U32 __inline __stdcall raddoubletoint( F64 val )
        {
          __asm
          {
            fild dword ptr [val]
            fistp dword ptr [val]
            mov eax,dword ptr [val]
          }
        }

        static U32 __inline __stdcall radisqrt( U32 sq )
        {
          __asm
          {
            fild dword ptr [sq]
            fsqrt
            fistp word ptr [sq]
            movzx eax,word ptr [sq]
          }
        }
                #endif

        static F64 __inline radlog2( F64 X )
        {
          F64 Result;
          __asm
          {
            fld1
            fld X
            fyl2x
            fstp Result
          }
          return( Result );
        }

        #pragma warning( default : 4035)

      #endif

    #endif

  #endif

#else

  #ifdef __WATCOMC__

    U32 radsqr(S32 a);
    #pragma aux radsqr = "shl edx,16" "mov dx,ax" "mov eax,edx" "xor edx,edx" "mul eax" "shld edx,eax,16" parm [dx ax] modify [DX ax] value [dx ax];

    S16 radabs(S16 ab);
    #pragma aux radabs = "test ax,ax" "jge skip" "neg ax" "skip:" parm [ax] value [ax];

    S32 radabs32(S32 ab);
    #pragma aux radabs32 = "test dx,dx" "jge skip" "neg dx" "neg ax" "sbb dx,0" "skip:" parm [dx ax] value [dx ax];

    U32 mult64anddiv(U32 mt1,U32 mt2,U32 d);
    #pragma aux mult64anddiv = "shl ecx,16" "mov cx,ax" "shrd eax,edx,16" "mov ax,si" "mul ecx" "shl edi,16" "mov di,bx" "div edi" "shld edx,eax,16" "and edx,0xffff" "and eax,0xffff" parm [cx ax] [dx si] [di bx] \
      modify [ax bx cx dx si di] value [dx ax];

    U32 mult64andshift(U32 mt1,U32 mt2,U8 shift);
    #pragma aux mult64anddiv = "shl ebx,16" "mov bx,ax" "shrd eax,edx,16" "mov ax,si" "mul ebx" "shrd eax,edx,cl" "shld edx,eax,16" "and edx,0xffff" "and eax,0xffff" parm [bx ax] [dx si] [cl] \
      modify [ax bx cx dx si di] value [dx ax];

  #endif

#endif

RADDEFEND

#endif
