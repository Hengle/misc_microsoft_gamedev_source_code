/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    message.c

Abstract:

    This module contains the Win32 Message Management APIs

Author:

    Steve Wood (stevewo) 24-Jan-1991

Revision History:

                  02-May-94 BruceMa Fix FormatMessage to accept Win32 status
                                    codes wrapped as HRESULTS

                  12-Dec-02 ArunKi Remove VirtualBuffer technique in favor of
                                    heap allocation and caching state between RtlFormatMessageEx calls.

                  15-Jun-04 Kazuyuks Ported to Xbox
--*/

#include "XFormatMessage.h"
#include "xfm_internal.h"

DWORD
APIENTRY
XFormatMessageW(
              DWORD dwFlags,
              LPCVOID lpSource,
              DWORD dwMessageId,
              DWORD dwLanguageId,
              PWSTR lpBuffer,
              DWORD nSize,
              va_list *lpArguments
              )
{
    return BaseDllFormatMessage(
                                 dwFlags,
                                 (LPVOID)lpSource,
                                 dwMessageId,
                                 dwLanguageId,
                                 lpBuffer,
                                 nSize,
                                 lpArguments
                               );
}

static DWORD g_dwLastError = STATUS_SUCCESS;
//WINBASEAPI
DWORD
WINAPI
XFormatMessage_GetLastError(
    VOID
    )
{
    return g_dwLastError;
}

//WINBASEAPI
VOID
WINAPI
XFormatMessage_SetLastError(
    DWORD dwErrCode
    )
{
    if (g_dwLastError != dwErrCode)
    {
        g_dwLastError = dwErrCode;
    }
}

#define INITIAL_DLL_FORMAT_MSG_EXPANSION 4
#define MAX_VAL(a,b)  ((a) > (b) ? (a) : (b))
#define MAX_FORMAT_MSG_EXCESS 0x400 // 1K maxmimum wastage allowed 

DWORD
APIENTRY
BaseDllFormatMessage(
                    DWORD dwFlags,
                    LPVOID lpSource,
                    DWORD dwMessageId,
                    DWORD dwLanguageId,
                    PWSTR lpBuffer,
                    DWORD nSize,
                    va_list *arglist
                    )
{
    NTSTATUS Status;
    PVOID FormattedData;
    ULONG BufferSize;
    ULONG MaximumWidth;
    ULONG LengthNeeded, Result;
    PWSTR MessageFormat;
    PWSTR lpAllocedBuffer;
    BOOLEAN IgnoreInserts;
    BOOLEAN ArgumentsAreAnArray;
    PARSE_MESSAGE_CONTEXT ParseContext;

    #define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
    #define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
    if (dwFlags & (FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM)) {
        BaseSetLastNTError( STATUS_INVALID_PARAMETER );
        return 0;
    }

    if (lpBuffer == NULL) {
        BaseSetLastNTError( STATUS_INVALID_PARAMETER );
        return 0;
    }

    lpAllocedBuffer = NULL;
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        *(PVOID *)lpBuffer = NULL;
    }

    RtlZeroMemory(&ParseContext, sizeof(ParseContext));
    
    if (dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS) {
        IgnoreInserts = TRUE;
    } else {
        IgnoreInserts = FALSE;
    }

    if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY) {
        ArgumentsAreAnArray = TRUE;
    } else {
        ArgumentsAreAnArray = FALSE;
    }

    Result = 0;
    lpAllocedBuffer = NULL;
    MaximumWidth = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;

    if (MaximumWidth == FORMAT_MESSAGE_MAX_WIDTH_MASK) {
        MaximumWidth = 0xFFFFFFFF;
    }
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
         MessageFormat = lpSource;
    } else {
            BaseSetLastNTError( STATUS_INVALID_PARAMETER );
            goto DllFormatMsgExit;
    }

    if (!(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)) {
        // The caller supplied the buffer, so we will simply pass it through.
        BufferSize = nSize * sizeof(WCHAR);
        FormattedData = lpBuffer;
    } else { 
        BufferSize = wcslen(MessageFormat);
        // Even if insertion is ignored, then expansion may still occur unless the maximum
        // width is specified. In the common case this will not be an issue; however, in the
        // event the prediciton is incorrect, the code will fall back to buffer expansion.
        if (!IgnoreInserts) {
            BufferSize *= INITIAL_DLL_FORMAT_MSG_EXPANSION;
        }
        nSize++;        // Correct for the case the caller used -1 as the input size (wrap to zero)
        BufferSize = (MAX_VAL(BufferSize+1, nSize))*sizeof(WCHAR);
        FormattedData = lpAllocedBuffer = (PWSTR)LocalAlloc( LMEM_FIXED, BufferSize);
        if (NULL == lpAllocedBuffer) {
            BaseSetLastNTError( STATUS_NO_MEMORY );
            goto DllFormatMsgExit;
        }
    }

retryMessageFormat:
    Status = RtlFormatMessageEx( MessageFormat,
                                   MaximumWidth,
                                   IgnoreInserts,
                                   ArgumentsAreAnArray,
                                   arglist,
                                   FormattedData,
                                   BufferSize,
                                   &LengthNeeded,
                                   &ParseContext);

    if (NT_SUCCESS( Status )) {
        Result = (LengthNeeded - sizeof( WCHAR )) / sizeof( WCHAR );
        if ((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER)) {
            // Determine the size to which we could trim the buffer, if
            // the remainder is excessive in size.
            if (LengthNeeded < (nSize)*sizeof(WCHAR)) {
                LengthNeeded = (nSize)*sizeof(WCHAR);
            }
            ASSERT(BufferSize >= LengthNeeded);
            // Note by trimming the size we may introduce heap fragmentation. However,
            // this is a theoretical problem only since by nature a formatted message is
            // transient in nature. As a compromise, the buffer is trimmed if the wastage exceeds
            // a threshold, or over half the allocated buffer is empty.
            if (((BufferSize - LengthNeeded >= MAX_FORMAT_MSG_EXCESS) || 
                (BufferSize/2 > LengthNeeded)) && 
                ((FormattedData = LocalReAlloc(lpAllocedBuffer, LengthNeeded, LMEM_MOVEABLE)) != NULL)) {
                lpAllocedBuffer = FormattedData;
            }
        }    
    } else if (STATUS_BUFFER_OVERFLOW == Status) {
        if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
            // Try reallocation.
            BufferSize = 2*BufferSize -sizeof(WCHAR);
            FormattedData = LocalReAlloc(lpAllocedBuffer, BufferSize, LMEM_MOVEABLE);
            if (FormattedData != NULL) {
                lpAllocedBuffer = FormattedData;
                goto retryMessageFormat;
            }
            BaseSetLastNTError( STATUS_NO_MEMORY );
        } else {
            // Preserve the original behavior when the caller's buffer is too small. Also, in case
            // the caller attempts to wcslen on the buffer, null-terminate the buffer at the end.
            if (nSize > 0) {
                lpBuffer[nSize-1] = L'\0';
            }
            BaseSetLastNTError( STATUS_BUFFER_TOO_SMALL );
        }
     } else {
        BaseSetLastNTError( Status );
     }

DllFormatMsgExit:
    
    if (lpAllocedBuffer != NULL) {
        if (Result) {
            *(PVOID *)lpBuffer = lpAllocedBuffer;
        } else {
            LocalFree( lpAllocedBuffer );
        }
    }
    
    return ( Result );
}
