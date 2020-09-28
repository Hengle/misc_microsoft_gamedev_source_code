/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    message.c

Abstract:

    Message table resource accessing functions

Author:

    Steve Wood (stevewo) 10-Sep-1991

Revision History:
                  15-Jun-2004 Kazuyuks Ported to Xbox
				  xx-May-2005 Kazuyuks Added floating point support

--*/

#include <stdio.h> // for _snwprintf
#include "xfm_internal.h"

#define ALLOW_FLOATING_POINT_SPEC

#define MAX_INSERTS 200
#define MAX_WIDTH 0xFFFFFFFF
#define FORMAT_STRING_LENGTH 32
#define INDEX_SENTINEL ((SIZE_T)~0)

NTSTATUS
RtlFormatMessageEx(
    IN PWSTR MessageFormat,
    IN ULONG MaximumWidth OPTIONAL,
    IN BOOLEAN IgnoreInserts,
    IN BOOLEAN ArgumentsAreAnArray,
    IN va_list *Arguments,
    OUT PWSTR Buffer,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL,
    OUT PPARSE_MESSAGE_CONTEXT ParseContext OPTIONAL
    )
{
    ULONG Column;
    int cchRemaining, cchWritten;
    PULONG_PTR ArgumentsArray = (PULONG_PTR)Arguments;
    ULONGLONG rgInserts[ MAX_INSERTS ];
#ifdef ALLOW_FLOATING_POINT_SPEC
	BOOL abIsArgWide[ MAX_INSERTS ];
	ULONG i;
#endif
    ULONG cSpaces;
    ULONG MaxInsert, CurInsert;
    ULONG PrintParameterCount;
    ULONG_PTR PrintParameter1;
    ULONG_PTR PrintParameter2;
    WCHAR PrintFormatString[ FORMAT_STRING_LENGTH ];
    BOOLEAN DefaultedFormatString;
    WCHAR c;
    PWSTR s, s1, s2, s3;
    PWSTR lpDst, lpDstBeg, lpDstLastSpace, lpParseStart;
    BOOLEAN WideArg = FALSE;

    MaxInsert = 0;
    cchRemaining = Length / sizeof(WCHAR);
     if ( ARGUMENT_PRESENT(ParseContext) &&
            TEST_PARSE_MESSAGE_CONTEXT_FLAG(ParseContext, ParseContextValid)) {
        CLEAR_PARSE_MESSAGE_CONTEXT_FLAG(ParseContext, ParseContextValid);
        lpParseStart = s = MessageFormat + ParseContext->iwSrc;
        Column = ParseContext->cwSavColumn;
        ParseContext->iwSrc = lpParseStart - MessageFormat;
        lpDst = Buffer + ParseContext->iwDst;
        lpDstLastSpace = (INDEX_SENTINEL == ParseContext->iwDstSpace ?
                                    NULL : Buffer + ParseContext->iwDstSpace);
        cchRemaining -= (ULONG)ParseContext->iwDst;
        if (!ArgumentsAreAnArray && ARGUMENT_PRESENT(Arguments)) {
            ASSERT(ParseContext->lpvArgStart);
            *Arguments = ParseContext->lpvArgStart;
        }
        if (TEST_PARSE_MESSAGE_CONTEXT_FLAG(ParseContext, ParseContextWrapping)) {
            CLEAR_PARSE_MESSAGE_CONTEXT_FLAG(ParseContext, ParseContextWrapping);
            goto redoWrap;
        }
    } else {
        lpDst = Buffer;
        lpDstLastSpace = NULL;
        Column = 0;
        lpParseStart = s = MessageFormat;
        if (!ArgumentsAreAnArray && ARGUMENT_PRESENT(Arguments) &&
            ARGUMENT_PRESENT(ParseContext)) {
            // On the initial call, the parse context will be invalid, so we take the opportunity
            // to note the start of the variable arguments (for subsequent restoration).
            ParseContext->lpvArgStart = *Arguments;
        }
    }

#ifdef ALLOW_FLOATING_POINT_SPEC
	if (!IgnoreInserts) {
	PWSTR sSave = s;
	ZeroMemory( abIsArgWide, sizeof(abIsArgWide) );
    while (*s != UNICODE_NULL) {
        if (*s == L'%') {
            s++;
            if (*s >= L'1' && *s <= L'9') {
                CurInsert = *s++ - L'0';
                if (*s >= L'0' && *s <= L'9') {
                    CurInsert = (CurInsert * 10) + (*s++ - L'0');
                    if (*s >= L'0' && *s <= L'9') {
                        CurInsert = (CurInsert * 10) + (*s++ - L'0');
                        if (*s >= L'0' && *s <= L'9') {
                            return( STATUS_INVALID_PARAMETER );
                        }
                    }
                }
                CurInsert -= 1;

                PrintParameterCount = 0;
                WideArg = FALSE;
                if (*s == L'!') {
                    s1 = PrintFormatString + 1;
                    s++;
					s2 = s;
                    while (*s != L'!') {
                        if (*s != UNICODE_NULL) {
                            if (s1 >= &PrintFormatString[ 31 ]) {
                                return( STATUS_INVALID_PARAMETER );
                            }
							if (*s == L'*') {
                                if (PrintParameterCount++ > 1) {
                                    return( STATUS_INVALID_PARAMETER );
                                }
                            }
                            s1++;
							s++;
                        } else {
                            return( STATUS_INVALID_PARAMETER );
                        }
                    }

                    s++;
					switch (s[-2])
					{
					case L'e':
					case L'E':
					case L'f':
					case L'g':
					case L'G':
						WideArg = TRUE;
						break;
					default:
						// look for I64 format string
						while (s2 != s && !WideArg) {
							if (s2[0] == L'I' && s2[1] == L'6' && s2[2] == L'4') {
								WideArg = TRUE;
								break;
							}
							s2++;
						}
						break;
					};
                }
				else
				{
					s++;
				}

                if (ARGUMENT_PRESENT( Arguments )) {
                    if ((CurInsert+PrintParameterCount) >= MAX_INSERTS) {
                        return( STATUS_INVALID_PARAMETER );
                    }
					abIsArgWide[ CurInsert + PrintParameterCount ] = WideArg;
					if ( CurInsert + PrintParameterCount + 1 > MaxInsert )
					{
						MaxInsert = CurInsert + PrintParameterCount + 1;
					}
                } else {
                    return( STATUS_INVALID_PARAMETER );
                }
            } else if (*s == L'0') {
                break;
            } else if (!*s) {
                return( STATUS_INVALID_PARAMETER );
            } else {
                s++;
            }
        } else {
            s++;
        }
	}
	for ( i = 0; i < MaxInsert; i++ )
	{
		WideArg = abIsArgWide[ i ];
		if (ArgumentsAreAnArray) {
			rgInserts[ i ] = *((PULONG_PTR)Arguments)++;
			if (WideArg) {
				// to do - consider endianness.
				((PULONG_PTR*)&rgInserts[ i ])[1] = *((PULONG_PTR)Arguments)++;
			}
		} else {
			if (WideArg) {
				rgInserts[ i ] = va_arg( *Arguments, ULONGLONG );
			} else {
				rgInserts[ i ] = va_arg( *Arguments, ULONG_PTR );
			}
		}
	}
	MaxInsert = 0;
	s = sSave;
	}
#endif //ALLOW_FLOATING_POINT_SPEC

    while (*s != UNICODE_NULL) {
        if (*s == L'%') {
            s++;
            lpDstBeg = lpDst;
            if (*s >= L'1' && *s <= L'9') {
                CurInsert = *s++ - L'0';
                if (*s >= L'0' && *s <= L'9') {
                    CurInsert = (CurInsert * 10) + (*s++ - L'0');
                    if (*s >= L'0' && *s <= L'9') {
                        CurInsert = (CurInsert * 10) + (*s++ - L'0');
                        if (*s >= L'0' && *s <= L'9') {
                            return( STATUS_INVALID_PARAMETER );
                        }
                    }
                }
                CurInsert -= 1;

                PrintParameterCount = 0;
                if (*s == L'!') {
                    DefaultedFormatString = FALSE;
                    s1 = PrintFormatString;
                    *s1++ = L'%';
                    s2 = s1;
                    s++;
                    while (*s != L'!') {
                        if (*s != UNICODE_NULL) {
                            if (s1 >= &PrintFormatString[ 31 ]) {
                                return( STATUS_INVALID_PARAMETER );
                            }

                            if (*s == L'*') {
                                if (PrintParameterCount++ > 1) {
                                    return( STATUS_INVALID_PARAMETER );
                                }
                            }

                            *s1++ = *s++;
                        } else {
                            return( STATUS_INVALID_PARAMETER );
                        }
                    }

                    s++;
                    *s1 = UNICODE_NULL;
                    WideArg = FALSE;
#if !defined(_WIN64)
                    // look for I64 format string
                    s3 = s2;
                    while (*s3 && !WideArg) {
                        if (s3[0] == L'I' && s3[1] == L'6' && s3[2] == L'4') {
                            WideArg = TRUE;
                        }
                        s3 += 1;
                    }
#endif
                } else {
                    DefaultedFormatString = TRUE;
                    wcscpy( PrintFormatString, L"%s" );
                    s1 = PrintFormatString + wcslen( PrintFormatString );
                    WideArg = FALSE;
                }

                if (IgnoreInserts) {
                    if (!wcscmp( PrintFormatString, L"%s" )) {
                        cchWritten = _snwprintf( lpDst,
                                                 cchRemaining,
                                                 L"%%%u",
                                                 CurInsert+1
                                               );
                    } else {
                        cchWritten = _snwprintf( lpDst,
                                                 cchRemaining,
                                                 L"%%%u!%s!",
                                                 CurInsert+1,
                                                 &PrintFormatString[ 1 ]
                                               );
                    }

                    if (cchWritten == -1) {
                        goto captureParseContext;
                    }
                } else if (ARGUMENT_PRESENT( Arguments )) {
                    if ((CurInsert+PrintParameterCount) >= MAX_INSERTS) {
                        return( STATUS_INVALID_PARAMETER );
                    }
#ifndef ALLOW_FLOATING_POINT_SPEC
                    while (CurInsert >= MaxInsert) {
                        if (ArgumentsAreAnArray) {
                            rgInserts[ MaxInsert++ ] = *((PULONG_PTR)Arguments)++;
                        } else {
                            if (WideArg) {
                                rgInserts[ MaxInsert++ ] = va_arg( *Arguments, ULONGLONG );
                            } else {
                                rgInserts[ MaxInsert++ ] = va_arg( *Arguments, ULONG_PTR );
                            }
                        }
                    }

                    s1 = (PWSTR)rgInserts[ CurInsert ];
                    PrintParameter1 = 0;
                    PrintParameter2 = 0;
                    if (PrintParameterCount > 0) {
                        if (ArgumentsAreAnArray) {
                            rgInserts[ MaxInsert++ ] = PrintParameter1 = *((PULONG_PTR)Arguments)++;
                        } else {
							// 24 may 2005 kazuyuks
                            //PrintParameter1 = va_arg( *Arguments, ULONG_PTR );
                            rgInserts[ MaxInsert++ ] = PrintParameter1 = va_arg( *Arguments, ULONG_PTR );
                        }

                        if (PrintParameterCount > 1) {
                            if (ArgumentsAreAnArray) {
                                rgInserts[ MaxInsert++ ] = PrintParameter2 = *((PULONG_PTR)Arguments)++;
                            } else {
                                rgInserts[ MaxInsert++ ] = PrintParameter2 = va_arg( *Arguments, ULONG_PTR );
                            }
                        }
                    }

                    if (WideArg) {
                        cchWritten = _snwprintf(
                            lpDst,
                            cchRemaining,
                            PrintFormatString,
                            rgInserts[CurInsert],
                            PrintParameter1,
                            PrintParameter2
                            );
                    } else {
                        cchWritten = _snwprintf(
                            lpDst,
                            cchRemaining,
                            PrintFormatString,
                            s1,
                            PrintParameter1,
                            PrintParameter2
                            );
                    }
#else
					if (PrintParameterCount > 0) {
                        PrintParameter1 = (ULONG_PTR)rgInserts[ CurInsert ];
                        if (PrintParameterCount > 1) {
                            PrintParameter2 = (ULONG_PTR)rgInserts[ CurInsert + 1 ];
							if (abIsArgWide[CurInsert+2]) {
								cchWritten = _snwprintf(lpDst, cchRemaining, PrintFormatString,
									PrintParameter1, PrintParameter2, rgInserts[CurInsert+2]);
							} else {
								cchWritten = _snwprintf(lpDst, cchRemaining, PrintFormatString,
									PrintParameter1, PrintParameter2, (PWSTR)rgInserts[CurInsert+2]);
							}
                        }
						else
						{
							if (abIsArgWide[CurInsert+1]) {
								cchWritten = _snwprintf(lpDst, cchRemaining, PrintFormatString,
									PrintParameter1, rgInserts[CurInsert+1]);
							} else {
								cchWritten = _snwprintf(lpDst, cchRemaining, PrintFormatString,
									PrintParameter1, (PWSTR)rgInserts[CurInsert+1]);
							}
						}
                    }
					else
					{
						if (abIsArgWide[CurInsert]) {
							cchWritten = _snwprintf(lpDst, cchRemaining, PrintFormatString,
								rgInserts[CurInsert]);
						} else {
							cchWritten = _snwprintf(lpDst, cchRemaining, PrintFormatString,
								(PWSTR)rgInserts[CurInsert]);
						}
					}
#endif
                    if (cchWritten == -1) {
                        goto captureParseContext;
                    }
                } else {
                    return( STATUS_INVALID_PARAMETER );
                }

                if ((cchRemaining -= cchWritten) < 0) {
                    goto captureParseContext;
                }

                lpDst += cchWritten;
            } else if (*s == L'0') {
                break;
            } else if (!*s) {
                return( STATUS_INVALID_PARAMETER );
            } else if (*s == L'r') {
                if ((cchRemaining -= 1) < 0) {
                    goto captureParseContext;
                }

                *lpDst++ = L'\r';
                s++;
                lpDstBeg = NULL;
            } else if (*s == L'n') {
                if ((cchRemaining -= 2) < 0) {
                    goto captureParseContext;
                }

                *lpDst++ = L'\r';
                *lpDst++ = L'\n';
                s++;
                lpDstBeg = NULL;
            } else if (*s == L't') {
                if ((cchRemaining -= 1) < 0) {
                    goto captureParseContext;
                }

                if (Column % 8) {
                    Column = (Column + 7) & ~7;
                } else {
                    Column += 8;
                }

                lpDstLastSpace = lpDst;
                *lpDst++ = L'\t';
                s++;
            } else if (*s == L'b') {
                if ((cchRemaining -= 1) < 0) {
                    goto captureParseContext;
                }

                lpDstLastSpace = lpDst;
                *lpDst++ = L' ';
                s++;
            } else if (IgnoreInserts) {
                if ((cchRemaining -= 2) < 0) {
                    goto captureParseContext;
                }

                *lpDst++ = L'%';
                *lpDst++ = *s++;
            } else {
                if ((cchRemaining -= 1) < 0) {
                    goto captureParseContext;
                }

                *lpDst++ = *s++;
            }

            if (lpDstBeg == NULL) {
                lpDstLastSpace = NULL;
                Column = 0;
            } else {
                Column += (ULONG)(lpDst - lpDstBeg);
            }
        } else {
            c = *s++;
            if (c == L'\r' || c == L'\n') {
                if ((c == L'\n' && *s == L'\r') ||
                    (c == L'\r' && *s == L'\n')
                   )
                {
                    s++;
                }

                if (MaximumWidth != 0) {
                    lpDstLastSpace = lpDst;
                    c = L' ';
                } else {
                    c = L'\n';
                }
            }

            if (c == L'\n') {
                if ((cchRemaining -= 2) < 0) {
                    goto captureParseContext;
                }

                *lpDst++ = L'\r';
                *lpDst++ = L'\n';
                lpDstLastSpace = NULL;
                Column = 0;
            } else {
                if ((cchRemaining -= 1) < 0) {
                    goto captureParseContext;
                }

                if (c == L' ') {
                    lpDstLastSpace = lpDst;
                }

                *lpDst++ = c;
                Column += 1;
            }
        }

        // Update pointer for next iteration before attempting to wrap the parsed data.
        lpParseStart = s;

        if (MaximumWidth != 0 &&
            MaximumWidth != MAX_WIDTH &&
            Column >= MaximumWidth
           ) {
redoWrap:
            if (lpDstLastSpace != NULL) {
                lpDstBeg = lpDstLastSpace;
                while (*lpDstBeg == L' ' || *lpDstBeg == L'\t') {
                    lpDstBeg += 1;
                    if (lpDstBeg == lpDst) {
                        break;
                        }
                    }
                while (lpDstLastSpace > Buffer) {
                    if (lpDstLastSpace[ -1 ] == L' ' || lpDstLastSpace[ -1 ] == L'\t') {
                        lpDstLastSpace -= 1;
                        }
                    else {
                        break;
                        }
                    }

                cSpaces = (ULONG)(lpDstBeg - lpDstLastSpace);
                if (cSpaces == 1) {
                    if ((cchRemaining -= 1) < 0) {
                        if (ARGUMENT_PRESENT(ParseContext)) {
                            SET_PARSE_MESSAGE_CONTEXT_FLAG(ParseContext,
                                ParseContextWrapping);
                        }
                        goto captureParseContext;
                    }
                } else
                if (cSpaces > 2) {
                    cchRemaining += (cSpaces - 2);
                    }

                memmove( lpDstLastSpace + 2,
                         lpDstBeg,
                         (ULONG) ((lpDst - lpDstBeg) * sizeof( WCHAR ))
                       );
                *lpDstLastSpace++ = L'\r';
                *lpDstLastSpace++ = L'\n';
                Column = (ULONG)(lpDst - lpDstBeg);
                lpDst = lpDstLastSpace + Column;
                lpDstLastSpace = NULL;
                }
            else {
                if ((cchRemaining -= 2) < 0) {
                    if (ARGUMENT_PRESENT(ParseContext)) {
                            SET_PARSE_MESSAGE_CONTEXT_FLAG(ParseContext,
                                ParseContextWrapping);
                    }
                    goto captureParseContext;
                 }

                *lpDst++ = L'\r';
                *lpDst++ = L'\n';
                lpDstLastSpace = NULL;
                Column = 0;
                }
            }
        }

    // Only note the error if we have less than one character remaining.
    if (cchRemaining < 1) {
        goto captureParseContext;
    }

    *lpDst++ = '\0';
    if ( ARGUMENT_PRESENT(ReturnLength) ) {
        *ReturnLength = (ULONG)(lpDst - Buffer) * sizeof( WCHAR );
    }

    return( STATUS_SUCCESS );

captureParseContext:
    if ( ARGUMENT_PRESENT(ParseContext) ) {
        ParseContext->cwSavColumn= Column;
        ParseContext->iwSrc = lpParseStart - MessageFormat;
        ParseContext->iwDst = lpDst -Buffer;
        ASSERT(ParseContext->iwDst <= ULONG_MAX);
        ParseContext->iwDstSpace = (lpDstLastSpace ?
                                                        lpDstLastSpace-Buffer : INDEX_SENTINEL);
        SET_PARSE_MESSAGE_CONTEXT_FLAG(ParseContext, ParseContextValid);
    }

    return STATUS_BUFFER_OVERFLOW;
}

NTSTATUS
RtlFormatMessage(
    IN PWSTR MessageFormat,
    IN ULONG MaximumWidth OPTIONAL,
    IN BOOLEAN IgnoreInserts,
    IN BOOLEAN ArgumentsAreAnsi,
    IN BOOLEAN ArgumentsAreAnArray,
    IN va_list *Arguments,
    OUT PWSTR Buffer,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    return RtlFormatMessageEx(MessageFormat, MaximumWidth, IgnoreInserts,
                ArgumentsAreAnArray, Arguments, Buffer, Length,
                ReturnLength, NULL);
}

