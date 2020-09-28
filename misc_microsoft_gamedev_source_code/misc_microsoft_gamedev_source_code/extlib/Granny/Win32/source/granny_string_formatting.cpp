// ========================================================================
// $File: //jeffr/granny/rt/granny_string_formatting.cpp $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #19 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_FLOATS_H)
#include "granny_floats.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

char const GRANNY UpperCaseDigitTable[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char const GRANNY LowerCaseDigitTable[] = "0123456789abcdefghijklmnopqrstuvwxyz";

uint32 GRANNY
ConvertCharToString(uint32 OutputStringLength, char *OutputString,
                    char const Char)
{
    uint32 SpaceUsed = 0;

    if(OutputStringLength > 1)
    {
        *OutputString = Char;
        ++SpaceUsed;
    }

    return(SpaceUsed);
}

uint32 GRANNY
ConvertInt64ToString(uint32 OutputStringLength, char *OutputString,
                     int64 Int64, uint32 const Basis,
                     char const *DigitTable)
{
    uint32 SpaceUsed = 0;
    if(Int64 < 0)
    {
        SpaceUsed += ConvertCharToString(OutputStringLength,
                                         OutputString, '-');
        // TODO: This is not actually going to work correctly for the
        // 0xFFFFFFFFFFFFFFFF case, I don't think...
        // Er... no, it should work fine. The positive half of the int64
        // range fits easily within the range of a uint64.
        Int64 = -Int64;
    }

    SpaceUsed += ConvertUInt64ToString(OutputStringLength - SpaceUsed,
                                       OutputString + SpaceUsed,
                                       (uint64)Int64, Basis, DigitTable);

    return(SpaceUsed);
}

// TODO: move to rad shared code once this is "seasoned" a while
inline static void SmallDivisorDivMod(uint64  dividend,
                                      uint32  divisor,
                                      uint64* quotient,
                                      uint32* remainder)
{
#if PLATFORM_WIN32
    // Prevent this from using div functions in the crt.  Note that
    // this is a super lame version of this.  Here's what's going on.
    // We're essentially transforming (dividend / divisor) into 2 divs
    // by treating dividend as (divhigh * 4G + divlow), where divhigh
    // and divlow are 32bit values, and 4G = 1<<32.  We can then treat
    // the divide as being: quotient = (divhigh/divisor) * 4G +
    // (divlow/divisor) and with a little twiddling, the below falls
    // out.
    __asm {
        push    ebx
        mov     ecx, DWORD PTR[divisor]
        mov     eax, DWORD PTR[dividend + 4]
        xor     edx,edx
        div     ecx
        mov     ebx,eax
        mov     eax, DWORD PTR[dividend + 0]
        div     ecx                              ; at this point, eax = quotient low, ebx = quotient high, edx = remainder
        mov     ecx, remainder
        mov     DWORD PTR[ecx], edx
        mov     ecx, quotient
        mov     DWORD PTR[ecx + 0], eax
        mov     DWORD PTR[ecx + 4], ebx
        pop     ebx
   };
#else
    *quotient  = dividend / divisor;
    *remainder = (uint32)(dividend % divisor);
#endif
}


uint32 GRANNY
ConvertUInt64ToString(uint32 OutputStringLength,
                      char * const OutputString,
                      uint64 const UInt64, uint32 const Basis,
                      char const *DigitTable)
{
    // Guard against degenerate bases
    Assert(Basis > 1);

    uint32 Used = 0;
    if(UInt64 != 0)
    {
        // Output the string in reverse-order
        uint64 Value = UInt64;
        while(Value > 0)
        {
            uint64 quotient;
            uint32 remainder;
            SmallDivisorDivMod(Value, Basis, &quotient, &remainder);

            Assert(remainder < Basis);
            uint8 const Character = (uint8)(remainder);
            Value = quotient;

            Used += ConvertCharToString(OutputStringLength - Used,
                                        OutputString + Used,
                                        DigitTable[Character]);
        }

        if(Used > 0)
        {
            // Reverse the string so it reads correctly
            char *BackToFront = (OutputString + Used) - 1;
            {for(char *FrontToBack = OutputString;
                 FrontToBack < BackToFront;
                 ++FrontToBack, --BackToFront)
            {
                char Temp = *BackToFront;
                *BackToFront = *FrontToBack;
                *FrontToBack = Temp;
            }}
        }
    }
    else
    {
        Used += ConvertCharToString(OutputStringLength,
                                    OutputString,
                                    DigitTable[0]);
    }

    return(Used);
}

uint32 GRANNY
ConvertReal64ToString(uint32 OutputStringLength, char *OutputString,
                      real64x const Real64, real64x const Basis,
                      uint32 Precision, char const *DigitTable)
{
    // Guard against degenerate bases
    Assert(Basis > 1);

    if(Real64 == 0.0f)
    {
        // TODO: distinguish between +0 and -0?
        uint32 Used = ConvertCharToString(OutputStringLength, OutputString,
                                          DigitTable[0]);
        OutputString += Used;
        OutputStringLength -= Used;

        Used = ConvertCharToString(OutputStringLength, OutputString, '.');
        OutputString += Used;
        OutputStringLength -= Used;

        while(Precision--)
        {
            Used = ConvertCharToString(OutputStringLength, OutputString,
                                       DigitTable[0]);
            OutputString += Used;
            OutputStringLength -= Used;
        }

        return(Used);
    }
    else if(Real64 == GetReal64Infinity())
    {
        return(ConvertToStringVar(OutputStringLength, OutputString,
                                  "(infinity)"));
    }
    else if(Real64 == -GetReal64Infinity())
    {
        return(ConvertToStringVar(OutputStringLength, OutputString,
                                  "(-infinity)"));
    }
    else if(Real64 == GetReal64QuietNaN())
    {
        return(ConvertToStringVar(OutputStringLength, OutputString,
                                  "(qnan)"));
    }
    else if(Real64 == GetReal64SignalingNaN())
    {
        return(ConvertToStringVar(OutputStringLength, OutputString,
                                  "(snan)"));
    }

    real64x Value = Real64;
    uint32 Used = 0;
    if(Value < 0)
    {
        Used += ConvertCharToString(OutputStringLength, OutputString, '-');
        Value = -Value;
    }

    // Output the string in reverse-order
    if(Value >= 1)
    {
        char * const BeginReversal = OutputString + Used;

        while(Value >= 1)
        {
            // TODO: this doesn't work for real64xs that don't fit in a real32!
            uint8 const Character = (uint8)
                Floor((real32)Modulus((real32)Value, (real32)Basis));

            Used += ConvertCharToString(OutputStringLength - Used,
                                        OutputString + Used,
                                        DigitTable[Character]);

            Value /= Basis;
        }

        // Reverse the string so it reads correctly
        char *BackToFront = (OutputString + Used) - 1;
        {for(char *FrontToBack = BeginReversal;
             FrontToBack < BackToFront;
             ++FrontToBack, --BackToFront)
        {
            char Temp = *BackToFront;
            *BackToFront = *FrontToBack;
            *FrontToBack = Temp;
        }}
    }
    else
    {
        Used += ConvertCharToString(OutputStringLength - Used,
                                    OutputString + Used,
                                    DigitTable[0]);
    }

    // TODO: this doesn't work for real64xs that don't fit in a real32!
    Value = Modulus((real32)Real64, 1.0f);
    if(Value < 0)
    {
        Value = -Value;
    }

    Used += ConvertCharToString(OutputStringLength - Used,
                                OutputString + Used,
                                '.');
    while(Precision--)
    {
        Value *= Basis;
        // TODO: this doesn't work for real64xs that don't fit in a real32!
        uint8 const Character = (uint8)
            Floor((real32)Modulus((real32)Value, (real32)Basis));

        Used += ConvertCharToString(OutputStringLength - Used,
                                    OutputString + Used,
                                    DigitTable[Character]);
    }

    return(Used);
}



uint32 GRANNY
ConvertInt32ToString(uint32 OutputStringLength, char *OutputString,
                     int32 Int32, uint32 const Basis,
                     char const *DigitTable)
{
    return ConvertInt64ToString ( OutputStringLength, OutputString, (int64)Int32, Basis, DigitTable );
}

uint32 GRANNY
ConvertUInt32ToString(uint32 OutputStringLength,
                      char * const OutputString,
                      uint32 const UInt32, uint32 const Basis,
                      char const *DigitTable)
{
    return ConvertUInt64ToString ( OutputStringLength, OutputString, (uint64)UInt32, Basis, DigitTable );
}


uint32 GRANNY
ConvertToStringList(uint32 OutputStringLength, char *OutputString,
                    char const * const FormatString,
                    c_argument_list &List)
{
    // Loop over the string
    uint32 SpaceUsed = 0;
    {for(char const *Character = FormatString;
         (*Character) && (SpaceUsed < OutputStringLength);
         ++Character)
    {
        bool OutputCharacter = false;
        if(*Character == '%')
        {
            // Ignored parameters left in for future versions
            //bool AlignLeft = false;
            //bool ForceSign = false;
            //char PadCharacter = ' ';
            //bool UseNotationPrefix = false;
            //bool ForceDecimalPoint = false;
            //bool TruncateTrailingZeros = true;
            //bool ForceWidth = false;
            //int32x Width = 0;

            int32x ArgumentBitSize = 32;
            int32x Precision = 6;

            bool Parsing = true;
            while(Parsing)
            {
                ++Character;
                switch(*Character)
                {
                    //
                    // Flags
                    //

                    case '-': // Left-aligned result
                    {
                        Assert(!"Align left (-) not supported by granny formatted output");
                        //AlignLeft = true;
                    } break;

                    case '+': // Use a + or - prefix if a signed type
                    {
                        Assert(!"Force signed (+) not supported by granny formatted output");
                        //ForceSign = true;
                    } break;

                    case '0': // Pad with 0's
                    {
                        Assert(!"Pad characters not supported by granny formatted output");
                        //PadCharacter = '0';
                    } break;

                    case ' ': // Pad with spaces
                    {
                        Assert(!"Pad characters not supported by granny formatted output");
                        //PadCharacter = ' ';
                    } break;

                    case '#': // Random-ass flag that isn't well documented
                    {
                        Assert(!"(#) modifier not supported by granny formatted output");
                        //UseNotationPrefix = true;
                        //ForceDecimalPoint = true;
                        //TruncateTrailingZeros = false;
                    } break;

                    //
                    // Precision
                    //

                    case '.': // Precision specifier
                    {
                        ++Character;
                        Precision = ConvertToUInt32(Character);
                        while(IsDecimal(*Character)) {++Character;}
                        --Character;
                    } break;

                    case 'I':
                    {
                        // prefix to specify size of ints.
                        // %I64i = int64
                        // %I32i = int32
                        // %Ii = intaddrx, i.e. an int as big as a pointer.
                        if ( ( Character[1] >= '0' ) && ( Character[1] <= '9' ) )
                        {
                            // It's a number
                            if ( ( Character[1] == '3' ) && ( Character[2] == '2' ) )
                            {
                                Character += 2;
                                ArgumentBitSize = 32;
                            }
                            else if ( ( Character[1] == '6' ) && ( Character[2] == '4' ) )
                            {
                                Character += 2;
                                ArgumentBitSize = 64;
                            }
                            else
                            {
                                Assert ( !"Unknown or unsupported width specifier" );
                                Parsing = false;
                            }
                        }
                        else
                        {
                            // sizeof(ptr)
#if PROCESSOR_32BIT_POINTER
                            ArgumentBitSize = 32;
#elif PROCESSOR_64BIT_POINTER
                            ArgumentBitSize = 64;
#else
#error Unknown pointer size (set it up in granny_processor.h)
#endif
                        }
                    } break;

                    //
                    // Width
                    //

                    case '*': // Indirect width specification
                    {
                        Assert(!"Specified field width not supported by granny formatted output");
                        //ForceWidth = true;
                        //Width = ArgumentListArgument(List, uint32);
                        SkipArgumentListArgument(List, uint32);
                    } break;

                    case '1': // Direct width specification
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    {
                        Assert(!"Specified field width not supported by granny formatted output");
                        //ForceWidth = true;
                        //Width = ConvertToUInt32(Character);

                        // Advance past the width specification
                        while(IsDecimal(*Character)) {++Character;}
                        --Character;
                    } break;

                    case 'c': // int (print single-byte character)
                    {
                        char C = (char)(ArgumentListArgument(List, int32));
                        SpaceUsed += ConvertCharToString(
                            OutputStringLength - SpaceUsed,
                            OutputString + SpaceUsed, C);
                        Parsing = false;
                    } break;

                    case 'd': // int (print signed decimal)
                    case 'i': // int (print signed decimal)
                    {
                        switch ( ArgumentBitSize )
                        {
                            case 32:
                            {
                                int32 D = ArgumentListArgument(List, int32);
                                SpaceUsed += ConvertInt32ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, D, 10);
                            } break;
                            case 64:
                            {
                                int64 D = ArgumentListArgument(List, int64);
                                SpaceUsed += ConvertInt64ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, D, 10);
                            } break;
                            default:
                                Assert ( !"Unknown size" );
                                break;
                        }
                        Parsing = false;
                    } break;

                    case 'o': // int (print unsigned octal)
                    {
                        switch ( ArgumentBitSize )
                        {
                            case 32:
                            {
                                uint32 O = ArgumentListArgument(List, uint32);
                                SpaceUsed += ConvertUInt32ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, O, 8);
                            } break;
                            case 64:
                            {
                                uint64 O = ArgumentListArgument(List, uint64);
                                SpaceUsed += ConvertUInt64ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, O, 8);
                            }   break;
                            default:
                                Assert ( !"Unknown size" );
                                break;
                        }
                        Parsing = false;
                    } break;

                    case 'u': // int (print unsigned decimal)
                    {
                        switch ( ArgumentBitSize )
                        {
                            case 32:
                            {
                                uint32 U = ArgumentListArgument(List, uint32);
                                SpaceUsed += ConvertUInt32ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, U, 10);
                            }   break;
                            case 64:
                            {
                                uint64 U = ArgumentListArgument(List, uint64);
                                SpaceUsed += ConvertUInt64ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, U, 10);
                            }   break;
                            default:
                                Assert ( !"Unknown size" );
                                break;
                        }
                        Parsing = false;
                    } break;

                    case 'x': // int (print lower-case hexadecimal)
                    {
                        switch ( ArgumentBitSize )
                        {
                            case 32:
                            {
                                uint32 X = ArgumentListArgument(List, uint32);
                                SpaceUsed += ConvertUInt32ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, X, 16,
                                    LowerCaseDigitTable);
                            }   break;
                            case 64:
                            {
                                uint64 X = ArgumentListArgument(List, uint64);
                                SpaceUsed += ConvertUInt64ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, X, 16,
                                    LowerCaseDigitTable);
                            }   break;
                            default:
                                Assert ( !"Unknown size" );
                                break;
                        }
                        Parsing = false;
                    } break;

                    case 'X': // int (print upper-case hexadecimal)
                    {
                        switch ( ArgumentBitSize )
                        {
                            case 32:
                            {
                                uint32 X = ArgumentListArgument(List, uint32);
                                SpaceUsed += ConvertUInt32ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, X, 16,
                                    UpperCaseDigitTable);
                            }   break;
                            case 64:
                            {
                                uint64 X = ArgumentListArgument(List, uint64);
                                SpaceUsed += ConvertUInt64ToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed, X, 16,
                                    UpperCaseDigitTable);
                            }   break;
                            default:
                                Assert ( !"Unknown size" );
                                break;
                        }
                        Parsing = false;
                    } break;

                    case 'e': // double (print scientific notation with e)
                    {
                        // TODO: Implement
                        Parsing = false;
                    } break;

                    case 'E': // double (print scientific notation with E)
                    {
                        // TODO: Implement
                        Parsing = false;
                    } break;

                    case 'f': // double (print floating point decimal)
                    {
                        real64x F = ArgumentListArgument(List, real64x);
                        SpaceUsed += ConvertReal64ToString(
                            OutputStringLength - SpaceUsed,
                            OutputString + SpaceUsed, F, 10,
                            Precision, LowerCaseDigitTable);
                        Parsing = false;
                    } break;

                    case 'n': // pointer to int (write back number of bytes written)
                    {
                        // What size is the return value?
                        Assert (!"Not sure if this works with 64-bit pointers");
#if !COMPILER_GCC
                        ArgumentListArgument(List, intaddrx) = SpaceUsed;
#else
                        // TODO: Is this still true?
#ifdef GRANNY_INTERNAL_BUILD
#warning "This compiler doesn't properly support assignable var-args"
#endif
#endif
                        Parsing = false;
                    } break;

                    case 's': // string (print single-byte character string)
                    {
                        char const *InputString = ArgumentListArgument(List, char const *);
                        if(InputString == 0)
                        {
                            InputString = "<null>";
                        }
                        else
                        {
                            while(*InputString)
                            {
                                SpaceUsed += ConvertCharToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed,
                                    *InputString++);
                            }
                        }

                        Parsing = false;
                    } break;

                    case 'l': // length string (print single-byte character string)
                    {
                        // What size is the "length" value going to be - 32 ro 64 bits?
                        Assert (!"Not sure how this works in 64-bit mode");
                        int32x Length = ArgumentListArgument(List, int);
                        char const *InputString = ArgumentListArgument(List, char const *);
                        if(InputString == 0)
                        {
                            InputString = "<null>";
                        }
                        else
                        {
                            while(*InputString && Length--)
                            {
                                SpaceUsed += ConvertCharToString(
                                    OutputStringLength - SpaceUsed,
                                    OutputString + SpaceUsed,
                                    *InputString++);
                            }
                        }

                        Parsing = false;
                    } break;

                    //
                    // Unsupported
                    //

                    case 'C': // int (print wide character)
                    case 'S': // string (print wide character string)
                    {
                        // Wide character strings aren't supported
                        // TODO: Implement
                        Parsing = false;
                    } break;

                    case 'g': // double (pick more compact of f or e)
                    case 'G': // double (pick more compact of f or E)
                    {
                        // Picking functions not supported
                        // TODO: Implement
                        Parsing = false;
                    } break;

                    default:
                    {
                        Parsing = false;
                        OutputCharacter = true;
                    } break;
                }
            }
        }
        else
        {
            OutputCharacter = true;
        }

        if(OutputCharacter)
        {
            SpaceUsed += ConvertCharToString(OutputStringLength - SpaceUsed,
                                             OutputString + SpaceUsed,
                                             *Character);
        }
    }}

    // Ensure a null terminator
    if(OutputStringLength)
    {
        Assert(SpaceUsed <= OutputStringLength);
        if(OutputStringLength == SpaceUsed)
        {
            --SpaceUsed;
        }

        OutputString[SpaceUsed] = '\0';
    }

    return(SpaceUsed);
}

uint32 GRANNY
ConvertToStringVar(uint32 OutputStringLength, char *OutputString,
                   char const * const FormatString, ...)
{
    c_argument_list List;
    OpenArgumentList(List, FormatString);

    uint32 const Used =
        ConvertToStringList(OutputStringLength, OutputString, FormatString, List);

    CloseArgumentList(List);
    return(Used);
}
