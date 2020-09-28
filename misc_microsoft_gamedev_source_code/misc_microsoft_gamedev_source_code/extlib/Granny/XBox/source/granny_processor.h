#if !defined(GRANNY_PROCESSOR_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_processor.h $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #16 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

// Clear all processor flags
#define PROCESSOR_X86 0
#define PROCESSOR_X64 0
#define PROCESSOR_POWERPC 0
#define PROCSESOR_68K 0
#define PROCESSOR_ALPHA 0
#define PROCESSOR_MIPS 0
#define PROCESSOR_ALLEGREX 0
#define PROCESSOR_CELL 0
#define PROCESSOR_CELL_SPU 0
#define PROCESSOR_GEKKO 0
#define PROCESSOR_BROADWAY 0


// Clear all endianness flags
#define PROCESSOR_LITTLE_ENDIAN 0
#define PROCESSOR_BIG_ENDIAN 0

// Clear all pointer size flags
#define PROCESSOR_32BIT_POINTER 0
#define PROCESSOR_64BIT_POINTER 0
#if defined(_PSX2)
#define _M_MIPS 1
#endif

#if defined(__psp__)
#undef  PROCESSOR_ALLEGREX
#undef  PROCESSOR_LITTLE_ENDIAN
#define PROCESSOR_ALLEGREX 1
#define PROCESSOR_LITTLE_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#if defined(__PPU__)
#undef  PROCESSOR_CELL
#undef  PROCESSOR_BIG_ENDIAN
#define PROCESSOR_CELL 1
#define PROCESSOR_BIG_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#if defined(__SPU__)
#undef  PROCESSOR_CELL_SPU
#undef  PROCESSOR_BIG_ENDIAN
#define PROCESSOR_CELL_SPU 1
#define PROCESSOR_BIG_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#ifdef _M_IX86
#undef PROCESSOR_X86
#define PROCESSOR_X86 1
#undef PROCESSOR_LITTLE_ENDIAN
#define PROCESSOR_LITTLE_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#ifdef _X86_NOASM
#define PROCESSOR_X86_NO_ASM 1
#undef PROCESSOR_LITTLE_ENDIAN
#define PROCESSOR_LITTLE_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#ifdef _M_X64
#undef PROCESSOR_X64
#define PROCESSOR_X64 1
#undef PROCESSOR_LITTLE_ENDIAN
#define PROCESSOR_LITTLE_ENDIAN 1
#undef PROCESSOR_64BIT_POINTER
#define PROCESSOR_64BIT_POINTER 1
#endif

// Handle powerpc and intel macintoshes
#if defined(_MACOSX)

#if defined(__ppc__)
// TODO: The PowerPC can actually be either Endian, so it'd be better
// to have some more coherent checking here.
#undef PROCESSOR_POWERPC
#undef PROCESSOR_BIG_ENDIAN
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_POWERPC 1
#define PROCESSOR_BIG_ENDIAN 1
#define PROCESSOR_32BIT_POINTER 1

#elif defined(__i386__)
#undef PROCESSOR_X86_NO_ASM
#undef PROCESSOR_LITTLE_ENDIAN
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_X86_NO_ASM 1
#define PROCESSOR_LITTLE_ENDIAN 1
#define PROCESSOR_32BIT_POINTER 1

#else
#error "No processor define found for macosx"
#endif

#endif

#if defined(_GAMECUBE)
#undef PROCESSOR_GEKKO
#undef PROCESSOR_BIG_ENDIAN
#define PROCESSOR_GEKKO 1
#define PROCESSOR_BIG_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#if defined(REVOLUTION)
#undef PROCESSOR_BROADWAY
#undef PROCESSOR_BIG_ENDIAN
#define PROCESSOR_BROADWAY 1
#define PROCESSOR_BIG_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#if defined(_XENON)
  #if !defined(_XBOX)
    #error I expected Xenon to also define _XBOX (granny_processor.h)
  #endif
#undef PROCESSOR_POWERPC
#undef PROCESSOR_BIG_ENDIAN
#define PROCESSOR_POWERPC 1
#define PROCESSOR_BIG_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#if defined(__MC68K__)
#undef PROCESSOR_68K
#undef PROCESSOR_BIG_ENDIAN
#define PROCESSOR_POWERPC 1
#define PROCESSOR_BIG_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

#ifdef _M_ALPHA
#undef PROCESSOR_ALPHA
#undef PROCESSOR_LITTLE_ENDIAN
#define PROCESSOR_ALPHA 1
#define PROCESSOR_LITTLE_ENDIAN 1
#undef PROCESSOR_64BIT_POINTER
#define PROCESSOR_64BIT_POINTER 1
#endif

#ifdef _M_MIPS
#undef  PROCESSOR_MIPS
#undef  PROCESSOR_LITTLE_ENDIAN
#define PROCESSOR_MIPS 1
#define PROCESSOR_LITTLE_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif

// only x86 for now, we should expand this to cover some other common
// platform types
#ifdef LINUX
#undef  PROCESSOR_X86_NO_ASM
#undef  PROCESSOR_LITTLE_ENDIAN
#define PROCESSOR_X86_NO_ASM 1
#define PROCESSOR_LITTLE_ENDIAN 1
#undef PROCESSOR_32BIT_POINTER
#define PROCESSOR_32BIT_POINTER 1
#endif


#if (!PROCESSOR_X86 && !PROCESSOR_X64 && !PROCESSOR_POWERPC &&      \
     !PROCESSOR_ALPHA && !PROCESSOR_MIPS && !PROCESSOR_ALLEGREX &&  \
     !PROCESSOR_68K && !PROCESSOR_GEKKO && !PROCESSOR_X86_NO_ASM && \
     !PROCESSOR_CELL && !PROCESSOR_CELL_SPU && !PROCESSOR_BROADWAY)
#error Unrecognized processor (see granny_processor.h)
#endif

#if (PROCESSOR_LITTLE_ENDIAN + PROCESSOR_BIG_ENDIAN)>1
#error Incorrect endianness (see granny_processor.h)
#endif

#if !PROCESSOR_LITTLE_ENDIAN && !PROCESSOR_BIG_ENDIAN
#error Unknown endianness (see granny_processor.h)
#endif

#if (PROCESSOR_32BIT_POINTER + PROCESSOR_64BIT_POINTER)>1
#error Incorrect pointer size (see granny_processor.h)
#endif

#if !PROCESSOR_32BIT_POINTER && !PROCESSOR_64BIT_POINTER
#error Unknown pointer size (see granny_processor.h)
#endif

#include "header_postfix.h"
#define GRANNY_PROCESSOR_H
#endif
