// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_msvc_stubs.cpp $
// $DateTime: 2007/03/20 11:44:04 $
// $Change: 14575 $
// $Revision: #10 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4725) // inaccurate on some pentiums

#if defined(_DEBUG) && !defined(DEBUG)
#error "If _DEBUG is defined, DEBUG must be defined"
#elif defined(DEBUG) && !defined(_DEBUG)
#error "If DEBUG is defined, _DEBUG must be defined"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

// Sigh.  We have to link against the CRT for dynamic builds in 
#if _MSC_VER < 1400 && !BUILDING_GRANNY_STATIC

extern "C"
{
    int _except_list = 0;
    int _fltused = 0x9876;
    int ldused = 0x9876;

    typedef void (__cdecl *_PVFV)(void);
    int __cdecl atexit (_PVFV func )
    {
        // TODO: Do I actually have to do anything here?
        return(0);
    }

    void __declspec(naked) _CIsqrt(double Value)
    {
#if !DEBUG
        // There should be no calls to _CIsqrt in a release build
        __asm {int 3}
#endif
        __asm
        {
            fsqrt
            ret
        }
    }

    void __declspec(naked) _CIexp(double Value)
    {
#if !DEBUG
        // There should be no calls to _CIexp in a release build!
        __asm {int 3}
#endif
        __asm
        {
            fldl2e
            fmul     st(0), st(1)
            fld      st(0)
            frndint
            fxch     st(1)
            fsub     st(0), st(1)
            f2xm1
            fld1
            faddp    st(1), st(0)
            fscale
            ret
        }
    }

    void __declspec(naked) _CIatan2(double Value)
    {
#if !DEBUG
        // There should be no calls to _CIatan2 in a release build
        __asm {int 3}
#endif
        __asm
        {
            fpatan
            ret
        }
    }

    void __declspec(naked) _CIsin(double Value)
    {
#if !DEBUG
        // There should be no calls to _CIsin in a release build
        __asm {int 3}
#endif
        __asm
        {
            fsin
            ret
        }
    }

    void __declspec(naked) _CIcos(double Value)
    {
#if !DEBUG
        // There should be no calls to _CIcos in a release build
        __asm {int 3}
#endif
        __asm
        {
            fcos
            ret
        }
    }

    void __declspec(naked) _chkesp(void)
    {
#if !DEBUG
        // There should be no calls to _chkesp in a release build
        __asm {int 3}
#endif
        __asm
        {
            jne StackMessedUp
            ret
        StackMessedUp:
            push ebp
            mov ebp, esp
            sub esp, 0
            push eax
            push edx
            push ebx
            push esi
            push edi
            int 3
            pop edi
            pop esi
            pop ebx
            pop edx
            pop eax
            mov esp, ebp
            pop ebp
            ret
        }
    };


    void * __cdecl memmove (
        void * dst,
        const void * src,
        size_t count
        )
    {
        void * ret = dst;

        if (dst <= src || (char *)dst >= ((char *)src + count)) {
            /*
             * Non-Overlapping Buffers
             * copy from lower addresses to higher addresses
             */
            while (count--) {
                *(char *)dst = *(char *)src;
                dst = (char *)dst + 1;
                src = (char *)src + 1;
            }
        }
        else {
            /*
             * Overlapping Buffers
             * copy from higher addresses to lower addresses
             */
            dst = (char *)dst + count - 1;
            src = (char *)src + count - 1;

            while (count--) {
                *(char *)dst = *(char *)src;
                dst = (char *)dst - 1;
                src = (char *)src - 1;
            }
        }

        return(ret);
    }

#if DEBUG
    void * __cdecl memset (
        void *dst,
        int val,
        size_t count
        )
    {
        void *start = dst;

        while (count--) {
            *(char *)dst = (char)val;
            dst = (char *)dst + 1;
        }

        return(start);
    }

    void * __cdecl memcpy (
        void * dst,
        const void * src,
        size_t count
        )
    {
        void * ret = dst;

        while (count--) {
            *(char *)dst = *(char *)src;
            dst = (char *)dst + 1;
            src = (char *)src + 1;
        }

        return(ret);
    }
#endif
}

#endif // _MSC_VER < 1300

