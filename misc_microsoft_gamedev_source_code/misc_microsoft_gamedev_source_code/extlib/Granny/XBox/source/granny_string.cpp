// ========================================================================
// $File: //jeffr/granny/rt/granny_string.cpp $
// $DateTime: 2007/08/17 17:38:16 $
// $Change: 15766 $
// $Revision: #23 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

string_comparison_callback *StringComparisonCallback = 0;

END_GRANNY_NAMESPACE;

int32x GRANNY
StringLength(char const *String)
{
    int32x Length = 0;
    if(String)
    {
        while(*String++)
        {
            ++Length;
        }
    }

    return(Length);
}

bool GRANNY
StringsAreEqual(char const *StringA, char const *StringB)
{
    if(StringA && StringB)
    {
        while(*StringA && *StringB && (*StringA == *StringB))
        {
            ++StringA;
            ++StringB;
        }

        return(*StringA == *StringB);
    }
    else
    {
        return(StringA == StringB);
    }
}

bool GRANNY
StringsAreEqual(int32x ALength, char const *StringA, char const *StringB)
{
    if(StringA && StringB)
    {
        while(ALength && *StringB && (*StringA == *StringB))
        {
            ++StringA;
            ++StringB;
            --ALength;
        }

        return((ALength == 0) && (*StringB == 0));
    }
    else
    {
        return(StringA == StringB);
    }
}

int32x GRANNY
StringDifference(char const *StringA, char const *StringB)
{
    if ( StringA == NULL )
    {
        if ( StringB == NULL )
        {
            // Equally NULL.
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        if ( StringB == NULL )
        {
            return 1;
        }
        else
        {
            // Both are actual strings.
            while(*StringA && *StringB && (*StringA == *StringB))
            {
                ++StringA;
                ++StringB;
            }

            return(*StringA - *StringB);
        }
    }
}

bool GRANNY
LengthStringIsEqualTo(int32x LengthA, char const *StringA, char const *StringB)
{
    if(StringA && StringB)
    {
        while(LengthA && *StringA && *StringB && (*StringA == *StringB))
        {
            --LengthA;
            ++StringA;
            ++StringB;
        }

        return((LengthA == 0) && (*StringB == 0));
    }
    else
    {
        return(StringA == StringB);
    }
}

bool GRANNY
StringBeginsWith(char const *String, char const *Beginning)
{
    if(String && Beginning)
    {
        while(*String && *Beginning && (*String == *Beginning))
        {
            ++String;
            ++Beginning;
        }

        return(!*Beginning);
    }
    else
    {
        return(String == Beginning);
    }
}

bool GRANNY
StringBeginsWithLowercase(char const *String, char const *Beginning)
{
    if(String && Beginning)
    {
        while(*String && *Beginning &&
              (ConvertToLowercase(*String) ==
               ConvertToLowercase(*Beginning)))
        {
            ++String;
            ++Beginning;
        }

        return(!*Beginning);
    }
    else
    {
        return(String == Beginning);
    }
}

bool GRANNY
StringsAreEqualLowercase(char const *StringA, char const *StringB)
{
    if(StringA && StringB)
    {
        while(*StringA && *StringB &&
              (ConvertToLowercase(*StringA) == ConvertToLowercase(*StringB)))
        {
            ++StringA;
            ++StringB;
        }

        return(*StringA == *StringB);
    }
    else
    {
        return(StringA == StringB);
    }
}

bool GRANNY
StringsAreEqualLowercase(char const *StringA,
                         char const *StringAEnd,
                         char const *StringB)
{
    if(StringA && StringB)
    {
        while((StringA != StringAEnd) && *StringB &&
              (ConvertToLowercase(*StringA) == ConvertToLowercase(*StringB)))
        {
            ++StringA;
            ++StringB;
        }

        return((StringA == StringAEnd) && !(*StringB));
    }
    else
    {
        return(StringA == StringB);
    }
}


int32x GRANNY
StringDifferenceOrCallback(char const *StringA, char const *StringB)
{
    if ( StringComparisonCallback )
    {
        return StringComparisonCallback ( StringA, StringB );
    }
    else
    {
        return StringDifference ( StringA, StringB );
    }
}

bool GRANNY
StringsAreEqualOrCallback(char const *StringA, char const *StringB)
{
    if ( StringComparisonCallback )
    {
        return ( 0 == StringComparisonCallback ( StringA, StringB ) );
    }
    else
    {
        return StringsAreEqual ( StringA, StringB );
    }
}

bool GRANNY
StringsAreEqualLowercaseOrCallback(char const *StringA, char const *StringB)
{
    if ( StringComparisonCallback )
    {
        return ( 0 == StringComparisonCallback ( StringA, StringB ) );
    }
    else
    {
        return StringsAreEqualLowercase ( StringA, StringB );
    }
}



void GRANNY
StringEquals(char *Dest, int32x MaxChars, char const *Source)
{
    if(Dest && Source)
    {
        while(*Source && (MaxChars-- > 1))
        {
            *Dest++ = *Source++;
        }
    }

    *Dest = '\0';

    if((MaxChars == 1) && *Source)
    {
        Log2(WarningLogMessage, StringLogMessage,
             "String overflow in StringEquals (Source is %d characters, but "
             "dest can only hold %d)", StringLength(Source), MaxChars);
    }
}

char const *GRANNY
StringContains(char const * const String,
               char const * const Substring)
{
    char const *StringPointer = String;

    bool Occurs = false;
    do
    {
        Occurs = StringBeginsWith(StringPointer, Substring);
    } while(!Occurs && *StringPointer++);

    return(Occurs ? StringPointer : 0);
}

char const *GRANNY
StringContainsLowercase(char const * const String,
                        char const * const Substring)
{
    char const *StringPointer = String;

    bool Occurs = false;
    do
    {
        Occurs = StringBeginsWithLowercase(StringPointer, Substring);
    } while(!Occurs && *StringPointer++);

    return(Occurs ? StringPointer : 0);
}

char const *GRANNY
FindLastSlash(char const *Filename)
{
    char const *Result = Filename;
    while(*Filename)
    {
        if((*Filename == '\\') ||
           (*Filename == '/'))
        {
            Result = Filename;
        }

        ++Filename;
    }

    return(Result);
}

int32x GRANNY
CopyString(char const *From, char *To)
{
    int32x Length = 1;

    while(true)
    {
        char c = *From++;
        *To++ = c;
        if ( c == '\0' )
        {
            break;
        }
        Length++;
    }

    return(Length);
}

char *GRANNY
CloneString(char const *Source)
{
    char *Result = 0;

    if(Source)
    {
        int32x Length = StringLength(Source) + 1;
        Result = AllocateArray(Length, char);
        Copy(Length, Source, Result);
    }

    return(Result);
}

// Note that this uses the length of the string NOT including the /0
// This is different from CopyString. It allows you to do things like:
// int SpaceLeft = 16;
// char TempBuffer[16+1];
// char *TempPtr = TempBuffer;
// CopyStringMaxLength ( "This is some ", &TempBuffer, &SpaceLeft );
// CopyStringMaxLength ( "text that is ", &TempBuffer, &SpaceLeft );
// CopyStringMaxLength ( "far too long ", &TempBuffer, &SpaceLeft );
// CopyStringMaxLength ( "to fit.", &TempBuffer, &SpaceLeft );
//
// ...and still have a decent string.
void GRANNY
CopyStringMaxLength(char const *From, char **To, int32x *MaxLength)
{
    if ( *MaxLength <= 0 )
    {
        return;
    }

    char *To2 = *To;

    while(true)
    {
        char c = *From++;
        *To2++ = c;
        if ( c == '\0' )
        {
            break;
        }

        (*MaxLength)--;
        if ( *MaxLength <= 0 )
        {
            *To = To2;
            *To2++ = '\0';
            return;
        }
    }
    *To = To2 - 1;
    return;
}

// Append the source string to the existing string.
// Returns the length of the new composite string.
int32 GRANNY
AppendStringMaxLength(char const *From, char *To, int32x MaxLength)
{
    int32x CurrentLength = StringLength ( To );
    int32x RemainingLength = MaxLength - CurrentLength;
    if ( RemainingLength <= 0 )
    {
        return CurrentLength;
    }
    To += CurrentLength;
    CopyStringMaxLength ( From, &To, &RemainingLength );
    return ( MaxLength - RemainingLength );
}


int32 GRANNY
ConvertToInt32(char const * const String)
{
    // TODO: Use Real64's once there's a conversion routine
    return(RoundReal32ToInt32(ConvertToReal32(String, false)));
}

uint32 GRANNY
ConvertToUInt32(char const * const String)
{
    // TODO: Use Real64's once there's a conversion routine
    return(RoundReal64ToUInt32(ConvertToReal64(String, false)));
}

real32 GRANNY
ConvertToReal32(char const * const String, bool const AllowFractions)
{
    return(real32(ConvertToReal64(String, AllowFractions)));
}

real64x GRANNY
ConvertToReal64(char const * const StringInit, bool const AllowFractions)
{
    real64x Negate = 1.0;
    char const *String = StringInit;
    if(*String == '-')
    {
        Negate = -1.0;
        ++String;
    }
    else if(*String == '+')
    {
        ++String;
    }

    bool PastPeriod = false;
    real64x Multiplier = 1;
    real64x Real64 = 0;
    {for(char const *StringPointer = String;
         *StringPointer;
         ++StringPointer)
    {
        if(IsDecimal(*StringPointer))
        {
            if(PastPeriod)
            {
                Real64 += Multiplier * real64x(ConvertToUInt8(*StringPointer));
                Multiplier *= .1;
            }
            else
            {
                Real64 *= 10.0;
                Real64 += real64x(ConvertToUInt8(*StringPointer));
            }
        }
        else if(!PastPeriod && (*StringPointer == '.') && AllowFractions)
        {
            PastPeriod = true;
            Multiplier = .1;
        }
        else
        {
            break;
        }
    }}

    return(Real64 * Negate);
}

uint8 const Control = 0x01;
uint8 const Whitespace = 0x02;
uint8 const Punctuation = 0x04;
uint8 const Uppercase = 0x08;
uint8 const Lowercase = 0x10;
uint8 const Hexadecimal = 0x20;
uint8 const Decimal = 0x40;
static uint8 CharacterAttributes[256] =
{
    Control,                     // NUL   (00)
    Control,                     // SOH   (01)
    Control,                     // STX   (02)
    Control,                     // ETX   (03)
    Control,                     // EOT   (04)
    Control,                     // ENQ   (05)
    Control,                     // ACK   (06)
    Control,                     // BEL   (07)
    Control,                     // BS    (08)
    Control | Whitespace,        // HT    (09)
    Control | Whitespace,        // LF    (0A)
    Control | Whitespace,        // VT    (0B)
    Control | Whitespace,        // FF    (0C)
    Control | Whitespace,        // CR    (0D)
    Control,                     // SI    (0E)
    Control,                     // SO    (0F)
    Control,                     // DLE   (10)
    Control,                     // DC1   (11)
    Control,                     // DC2   (12)
    Control,                     // DC3   (13)
    Control,                     // DC4   (14)
    Control,                     // NAK   (15)
    Control,                     // SYN   (16)
    Control,                     // ETB   (17)
    Control,                     // CAN   (18)
    Control,                     // EM    (19)
    Control,                     // SUB   (1A)
    Control,                     // ESC   (1B)
    Control,                     // FS    (1C)
    Control,                     // GS    (1D)
    Control,                     // RS    (1E)
    Control,                     // US    (1F)
    Whitespace,                  // SPACE (20)
    Punctuation,                 // !     (21)
    Punctuation,                 // "     (22)
    Punctuation,                 // #     (23)
    Punctuation,                 // $     (24)
    Punctuation,                 // %     (25)
    Punctuation,                 // &     (26)
    Punctuation,                 // '     (27)
    Punctuation,                 // (     (28)
    Punctuation,                 // )     (29)
    Punctuation,                 // *     (2A)
    Punctuation,                 // +     (2B)
    Punctuation,                 // ,     (2C)
    Punctuation,                 // -     (2D)
    Punctuation,                 // .     (2E)
    Punctuation,                 // /     (2F)
    Decimal | Hexadecimal,       // 0     (30)
    Decimal | Hexadecimal,       // 1     (31)
    Decimal | Hexadecimal,       // 2     (32)
    Decimal | Hexadecimal,       // 3     (33)
    Decimal | Hexadecimal,       // 4     (34)
    Decimal | Hexadecimal,       // 5     (35)
    Decimal | Hexadecimal,       // 6     (36)
    Decimal | Hexadecimal,       // 7     (37)
    Decimal | Hexadecimal,       // 8     (38)
    Decimal | Hexadecimal,       // 9     (39)
    Punctuation,                 // :     (3A)
    Punctuation,                 // ;     (3B)
    Punctuation,                 // <     (3C)
    Punctuation,                 // =     (3D)
    Punctuation,                 // >     (3E)
    Punctuation,                 // ?     (3F)
    Punctuation,                 // @     (40)
    Uppercase | Hexadecimal,     // A     (41)
    Uppercase | Hexadecimal,     // B     (42)
    Uppercase | Hexadecimal,     // C     (43)
    Uppercase | Hexadecimal,     // D     (44)
    Uppercase | Hexadecimal,     // E     (45)
    Uppercase | Hexadecimal,     // F     (46)
    Uppercase,                   // G     (47)
    Uppercase,                   // H     (48)
    Uppercase,                   // I     (49)
    Uppercase,                   // J     (4A)
    Uppercase,                   // K     (4B)
    Uppercase,                   // L     (4C)
    Uppercase,                   // M     (4D)
    Uppercase,                   // N     (4E)
    Uppercase,                   // O     (4F)
    Uppercase,                   // P     (50)
    Uppercase,                   // Q     (51)
    Uppercase,                   // R     (52)
    Uppercase,                   // S     (53)
    Uppercase,                   // T     (54)
    Uppercase,                   // U     (55)
    Uppercase,                   // V     (56)
    Uppercase,                   // W     (57)
    Uppercase,                   // X     (58)
    Uppercase,                   // Y     (59)
    Uppercase,                   // Z     (5A)
    Punctuation,                 // [     (5B)
    Punctuation,                 // \     (5C)
    Punctuation,                 // ]     (5D)
    Punctuation,                 // ^     (5E)
    Punctuation,                 // _     (5F)
    Punctuation,                 // `     (60)
    Lowercase | Hexadecimal,     // a     (61)
    Lowercase | Hexadecimal,     // b     (62)
    Lowercase | Hexadecimal,     // c     (63)
    Lowercase | Hexadecimal,     // d     (64)
    Lowercase | Hexadecimal,     // e     (65)
    Lowercase | Hexadecimal,     // f     (66)
    Lowercase,                   // g     (67)
    Lowercase,                   // h     (68)
    Lowercase,                   // i     (69)
    Lowercase,                   // j     (6A)
    Lowercase,                   // k     (6B)
    Lowercase,                   // l     (6C)
    Lowercase,                   // m     (6D)
    Lowercase,                   // n     (6E)
    Lowercase,                   // o     (6F)
    Lowercase,                   // p     (70)
    Lowercase,                   // q     (71)
    Lowercase,                   // r     (72)
    Lowercase,                   // s     (73)
    Lowercase,                   // t     (74)
    Lowercase,                   // u     (75)
    Lowercase,                   // v     (76)
    Lowercase,                   // w     (77)
    Lowercase,                   // x     (78)
    Lowercase,                   // y     (79)
    Lowercase,                   // z     (7A)
    Punctuation,                 // {     (7B)
    Punctuation,                 // |     (7C)
    Punctuation,                 // }     (7D)
    Punctuation,                 // ~     (7E)
    Control,                     // DEL   (7F)
};

bool GRANNY
IsWhitespace(char const Character)
{
    return((CharacterAttributes[(unsigned char)Character] & Whitespace) == Whitespace);
}

bool GRANNY
IsAlphabetic(char const Character)
{
    return(IsLowercase(Character) ||
           IsUppercase(Character));
}

bool GRANNY
IsLowercase(char const Character)
{
    return((CharacterAttributes[(unsigned char)Character] & Lowercase) == Lowercase);
}

bool GRANNY
IsUppercase(char const Character)
{
    return((CharacterAttributes[(unsigned char)Character] & Uppercase) == Uppercase);
}

bool GRANNY
IsDecimal(char const Character)
{
    return((CharacterAttributes[(unsigned char)Character] & Decimal) == Decimal);
}

bool GRANNY
IsPunctuation(char const Character)
{
    return((CharacterAttributes[(unsigned char)Character] & Punctuation) == Punctuation);
}

bool GRANNY
IsHexadecimal(char const Character)
{
    return((CharacterAttributes[(unsigned char)Character] & Hexadecimal) == Hexadecimal);
}

bool GRANNY
IsSlash(char Char)
{
    return((Char == '\\') || (Char == '/'));
}

uint8 GRANNY
ConvertToUInt8(char Character)
{
    if((Character >= '0') && (Character <= '9'))
    {
        return(uint8)(Character - '0');
    }
    else
    {
        return(0);
    }
}

char GRANNY
ConvertToLowercase(char Character)
{
    if((Character >= 'A') && (Character <= 'Z'))
    {
        Character += 'a' - 'A';
    }

    return(Character);
}

char GRANNY
ConvertToUppercase(char Character)
{
    if((Character >= 'a') && (Character <= 'z'))
    {
        Character += 'A' - 'a';
    }

    return(Character);
}

static bool
SubStringsAreEqual(char const *StringA, char const *StringB,
                   intaddrx Length)
{
    if(StringA && StringB)
    {
        while(Length && *StringA && *StringB && (*StringA == *StringB))
        {
            ++StringA;
            ++StringB;
            --Length;
        }

        return((Length == 0) || (*StringA == *StringB));
    }
    else
    {
        return(StringA == StringB);
    }
}


// This function is a modified version of the one written by Jeff for
// cdep.  Jeff is one of the best programmers in the world, but also
// one of the most terse, hence the short variable names :)
bool GRANNY
WildCardMatch(char const *Name, char const *Wildcard, char *Out)
{
    // TODO: This is not string-length safe (Out can overflow)

    char const * n=Name;
    char const * w=Wildcard;

    if ( w == NULL )
    {
        // No wildcard - treated the same as a wildcard of "*"
        // This behaviour is slightly Granny-specific, but it's
        // not well-defined in general, so I don't think it's
        // a particularly bad thing to put it here.
        CopyString(n, Out);
        return true;
    }

    while (1)
    {
        if ((*w)=='?')
        {
            // skip the check on the next character
            if (*n)
            {
                *Out++ = *n++;
            }
            ++w;
        }
        else if ((*w)=='*')
        {
            char const * e=++w;

            // find the end of the other side of the wildcard (the anchor)
            while (((*e)!=0) && ((*e)!=';') && ((*e)!='?') && ((*e)!='*'))
                ++e;

            if ((e-w)==0)
            {
                Out += CopyString(n, Out);
                return(true);
            }

            while (*n)
            {
                // scan through the string, if the anchors match, then recursively call
                //   this routine to try to match the next chunk of the string.
                if (SubStringsAreEqual(w,n,e-w))
                {
                    if (WildCardMatch(n,w,Out))
                    {
                        return(true);
                    }
                }

                *Out++ = *n++;
            }
        }
        else if (*w)
        {
            if (ConvertToLowercase(*w)!=ConvertToLowercase(*n))
            {
                return(false);
            }
            else
            {
                ++n;
            }
            ++w;
        }
        else
        {
            // we've hit the end of the wildcard string, if we are at the end of
            //   the name string too, then we have a match, otherwise, it's a mismatch
            *Out = '\0';
            return( (*n)==0 );
        }
    }

    // won't get here
}

bool GRANNY
IsPlainWildcard(char const *Wildcard)
{
    return(!Wildcard ||
           ((*Wildcard == '*') && (*(Wildcard + 1) == '\0')));
}


void GRANNY
SetStringComparisonCallback(string_comparison_callback *Callback)
{
    StringComparisonCallback = Callback;
}
