#if !defined(GRANNY_STRING_FORMATTING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_string_formatting.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_C_ARGUMENT_LIST_H)
#include "granny_c_argument_list.h"
#endif

BEGIN_GRANNY_NAMESPACE;

extern char const UpperCaseDigitTable[];
extern char const LowerCaseDigitTable[];

// VERY IMPORTANT: By convention, these return the StringLength() value,
// which is ONE MINUS THE ACTUAL NUMBER OF CHARACTERS USED, because the
// null terminator is not counted.  DO NOT FORGET THAT or you will
// be an unhappy camper.

uint32 ConvertCharToString(uint32 OutputStringLength, char *OutputString,
                           char const Char);
uint32 ConvertInt32ToString(uint32 OutputStringLength, char *OutputString,
                            int32 Int32, uint32 const Basis,
                            char const *DigitTable = LowerCaseDigitTable);
uint32 ConvertUInt32ToString(uint32 OutputStringLength, char *OutputString,
                             uint32 const UInt32, uint32 const Basis,
                             char const *DigitTable = LowerCaseDigitTable);
uint32 ConvertInt64ToString(uint32 OutputStringLength, char *OutputString,
                            int64 Int64, uint32 const Basis,
                            char const *DigitTable = LowerCaseDigitTable);
uint32 ConvertUInt64ToString(uint32 OutputStringLength, char *OutputString,
                             uint64 const UInt64, uint32 const Basis,
                             char const *DigitTable = LowerCaseDigitTable);
uint32 ConvertReal64ToString(uint32 OutputStringLength, char *OutputString,
                             real64x const Real64, real64x const Basis,
                             uint32 Precision,
                             char const *DigitTable = LowerCaseDigitTable);
uint32 ConvertToStringList(uint32 OutputStringLength, char *OutputString,
                           char const * const FormatString,
                           c_argument_list &List);
uint32 ConvertToStringVar(uint32 OutputStringLength, char *OutputString,
                          char const * const FormatString, ...);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STRING_FORMATTING_H
#endif
