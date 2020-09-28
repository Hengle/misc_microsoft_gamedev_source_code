#ifndef __RADDEBUGH__
#define __RADDEBUGH__

#include "radbase.h"


RADDEFSTART

/* 32 bit implementations */

#ifndef radassert
  #if defined(_DEBUG) && !defined(NDEBUG)
    #ifdef __RADNGC__
      #include <dolphin.h>
      #define radassert( cond ) { if (!(cond)) { OSReport("Assert: %s File: %s Line %i\n",#cond,__FILE__, __LINE__); BreakPoint(); } }
      #define radassertfail() { OSReport("Assert: AlwaysFail File: %s Line %i\n",__FILE__, __LINE__); BreakPoint(); }
    #elif defined(__RADWII__)
      #include <revolution.h>
      #define radassert( cond ) { if (!(cond)) { OSReport("Assert: %s File: %s Line %i\n",#cond,__FILE__, __LINE__); BreakPoint(); } }
      #define radassertfail() { OSReport("Assert: AlwaysFail File: %s Line %i\n",__FILE__, __LINE__); BreakPoint(); }
    #elif defined(__RADNDS__)
      #include <nitro/misc.h>
      #define radassert     SDK_ASSERT
      #define radassertfail() SDK_ASSERT(0)
    #else
      #define radassert( cond ) { if (!(cond)) BreakPoint(); }
      #define radassertfail() { BreakPoint(); }
    #endif
  #else
    #define radassert( cond )
    #define radassertfail()
  #endif
#endif

#ifndef radcassert
  #define radcassert(cond) { typedef char rad_assert_dummy[ ( cond ) ? 1 : -1 ]; }
#endif

#ifdef __RAD32__

  #ifdef __RADNGC__

    #define BreakPoint() asm(" .long 0x00000001")

  #elif defined(__RADWII__)

    #define BreakPoint() __asm__ volatile("trap");

  #elif defined(__RADNDS__)

    #define BreakPoint() asm("BKPT 0")

  #elif defined(__RADPS2__)

    #define BreakPoint() __asm__ volatile("break");

  #elif defined(__RADPSP__)

    #define BreakPoint() __asm__ volatile("break");

  #elif defined(__RADSPU__)

    #define BreakPoint() __asm volatile ("stopd 0,1,1");
    
  #elif defined(__RADPS3__)

    #define BreakPoint() __asm__ volatile("trap");

  #elif defined(__RADMAC__) && !defined(__RADX86__)

    #if defined(__GNUG__) || defined(__GNUC__)
      #include <assert.h>
      #define BreakPoint() assert( 0 )
    #else
      #ifdef __RADMACH__
        void DebugStr(unsigned char const *);
      #else
        void pascal DebugStr(unsigned char const *);
      #endif
      #define BreakPoint() DebugStr("\pBreakPoint() was called")
    #endif

  #elif defined(__RADXENON__)

    void DebugBreak(void);
    #define BreakPoint() {DebugBreak();}

  #else

    #ifdef __RADLINUX__
      #include <assert.h>
      #define BreakPoint() assert( 0 )   

    #elif defined(__WATCOMC__)

      char bkbhit();
      #pragma aux bkbhit = "mov ah,1" "int 0x16" "lahf" "shr eax,14" "and eax,1" "xor al,1" ;

      char bgetch();
      #pragma aux bgetch = "xor ah,ah" "int 0x16" "test al,0xff" "jnz done" "mov al,ah" "or al,0x80" "done:" modify [AX];

      U32 DOSOut(const char* str);
      #pragma aux DOSOut = "cld" "mov ecx,0xffffffff" "xor eax,eax" "mov edx,edi" "repne scasb" "not ecx" "dec ecx" "mov ebx,1" "mov ah,0x40" "int 0x21" parm [EDI] modify [EAX EBX ECX EDX EDI] value [ecx];

      void DOSOutNum(const char* str,U32 len);
      #pragma aux DOSOutNum = "mov ah,0x40" "mov ebx,1" "int 0x21" parm [edx] [ecx] modify [eax ebx];

      U32 ErrOut(const char* str);
      #pragma aux ErrOut = "cld" "mov ecx,0xffffffff" "xor eax,eax" "mov edx,edi" "repne scasb" "not ecx" "dec ecx" "xor ebx,ebx" "mov ah,0x40" "int 0x21" parm [EDI] modify [EAX EBX ECX EDX EDI] value [ecx];

      void ErrOutNum(const char* str,U32 len);
      #pragma aux ErrOutNum = "mov ah,0x40" "xor ebx,ebx" "int 0x21" parm [edx] [ecx] modify [eax ebx];

      void BreakPoint( void );
      #pragma aux BreakPoint = "int 0x3";

      u8 radinp(u16 p);
      #pragma aux radinp = "in al,dx" parm [DX];

      void radoutp(u16 p,u8 v);
      #pragma aux radoutp = "out dx,al" parm [DX] [AL];

    #elif defined(__RADX64__)

      #if !defined(_DEBUG) || defined(NDEBUG)
        #undef radassert
        #undef radassertfail
        #define radassert(cond) __assume((cond))
        #define radassertfail() __assume((0))
      #endif
      #include <assert.h>
      #define BreakPoint() assert(0)

    #else
      #if !defined(_DEBUG) || defined(NDEBUG)
        #undef radassert
        #undef radassertfail
        #define radassert(cond) __assume((cond))
        #define radassertfail() __assume((0))
      #endif
      #define BreakPoint() __asm {int 3}

    #endif

  #endif

#else

  #ifdef __WATCOMC__

    char bkbhit();
    #pragma aux bkbhit = "mov ah,1" "int 0x16" "lahf" "shr eax,14" "and eax,1" "xor al,1" ;

    char bgetch();
    #pragma aux bgetch = "xor ah,ah" "int 0x16" "test al,0xff" "jnz done" "mov al,ah" "or al,0x80" "done:" modify [AX];

    U32 DOSOut(const char far* dest);
    #pragma aux DOSOut = "cld" "and edi,0xffff" "mov dx,di" "mov ecx,0xffffffff" "xor eax,eax" 0x67 "repne scasb" "not ecx" "dec ecx" "mov bx,1" "push ds" "push es" "pop ds" "mov ah,0x40" "int 0x21" "pop ds" "movzx eax,cx" "shr ecx,16" \
       parm [ES DI] modify [AX BX CX DX DI ES] value [CX AX];

    void DOSOutNum(const char far* str,U16 len);
    #pragma aux DOSOutNum = "push ds" "mov ds,cx" "mov cx,bx" "mov ah,0x40" "mov bx,1" "int 0x21" "pop ds" parm [cx dx] [bx] modify [ax bx cx];

    U32 ErrOut(const char far* dest);
    #pragma aux ErrOut = "cld" "and edi,0xffff" "mov dx,di" "mov ecx,0xffffffff" "xor eax,eax" 0x67 "repne scasb" "not ecx" "dec ecx" "xor bx,bx" "push ds" "push es" "pop ds" "mov ah,0x40" "int 0x21" "pop ds" "movzx eax,cx" "shr ecx,16" \
       parm [ES DI] modify [AX BX CX DX DI ES] value [CX AX];

    void ErrOutNum(const char far* str,U16 len);
    #pragma aux ErrOutNum = "push ds" "mov ds,cx" "mov cx,bx" "mov ah,0x40" "xor bx,bx" "int 0x21" "pop ds" parm [cx dx] [bx] modify [ax bx cx];

    void BreakPoint( void );
    #pragma aux BreakPoint = "int 0x3";

    u8 radinp(u16 p);
    #pragma aux radinp = "in al,dx" parm [DX];

    void radoutp(u16 p,u8 v);
    #pragma aux radoutp = "out dx,al" parm [DX] [AL];

  #endif

#endif

RADDEFEND

#endif

