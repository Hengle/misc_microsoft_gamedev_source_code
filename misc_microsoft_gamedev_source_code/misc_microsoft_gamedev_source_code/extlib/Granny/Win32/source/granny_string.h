#if !defined(GRANNY_STRING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_string.h $
// $DateTime: 2007/08/17 17:38:16 $
// $Change: 15766 $
// $Revision: #16 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(StringGroup);

int32x StringLength(char const *String);

bool StringsAreEqual(char const *StringA, char const *StringB);
bool StringsAreEqual(int32x ALength, char const *StringA, char const *StringB);
EXPAPI GS_SAFE int32x StringDifference(char const *StringA, char const *StringB);
bool LengthStringIsEqualTo(int32x LengthA, char const *StringA, char const *StringB);
bool StringBeginsWith(char const *String, char const *Beginning);
bool StringBeginsWithLowercase(char const *String, char const *Beginning);
bool StringsAreEqualLowercase(char const *StringA, char const *StringB);
bool StringsAreEqualLowercase(char const *StringAStart,
                              char const *StringAEnd,
                              char const *StringB);
bool WildCardMatch(char const *Name, char const *Wildcard, char *Out);
bool IsPlainWildcard(char const *Wildcard);

void StringEquals(char *Dest, int32x MaxChars, char const *Source);
int32x CopyString(char const *From, char *To);
void CopyStringMaxLength(char const *From, char **To, int32x *MaxLength);
int32 AppendStringMaxLength(char const *From, char *To, int32x MaxLength);
char *CloneString(char const *Source);

char const *StringContains(char const * const String,
                           char const * const Substring);
char const *StringContainsLowercase(char const * const String,
                                    char const * const Substring);
char const *FindLastSlash(char const *Filename);

int32 ConvertToInt32(char const * const String);
uint32 ConvertToUInt32(char const * const String);
real32 ConvertToReal32(char const * const String,
                       bool const AllowFractions = true);
real64x ConvertToReal64(char const * const String,
                        bool const AllowFractions = true);

bool IsWhitespace(char const Character);
bool IsAlphabetic(char const Character);
bool IsLowercase(char const Character);
bool IsUppercase(char const Character);
bool IsDecimal(char const Character);
bool IsPunctuation(char const Character);
bool IsHexadecimal(char const Character);
bool IsSlash(char Char);

uint8 ConvertToUInt8(char Character);
char ConvertToLowercase(char Character);
char ConvertToUppercase(char Character);

EXPAPI typedef int32x string_comparison_callback(char const *A, char const *B);
extern string_comparison_callback *StringComparisonCallback;
EXPAPI GS_MODIFY void SetStringComparisonCallback(string_comparison_callback *Callback);

int32x StringDifferenceOrCallback(char const *StringA, char const *StringB);
bool StringsAreEqualOrCallback(char const *StringA, char const *StringB);
bool StringsAreEqualLowercaseOrCallback(char const *StringA, char const *StringB);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STRING_H
#endif
