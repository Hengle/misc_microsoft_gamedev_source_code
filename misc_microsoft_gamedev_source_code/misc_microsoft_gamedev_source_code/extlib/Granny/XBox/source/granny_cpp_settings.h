#if !defined(GRANNY_CPP_SETTINGS_H)
// #include "header_preamble.h" NO PREFIX FOR THIS FILE
// ========================================================================
// $File: //jeffr/granny/rt/granny_cpp_settings.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #2 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

// This should always be the last header included in any Granny source
// file.  The intention is to set any packing/warning etc. settings
// that are required for the environment

#if !defined(GRANNY_COMPILER_H)
#include "granny_compiler.h"
#endif

// Pack structures to 4-bytes...
#if COMPILER_MSVC
#pragma pack(4)
#endif

// #include "header_postfix.h"  NO POSTFIX FOR THIS FILE
#define GRANNY_CPP_SETTINGS_H
#endif
