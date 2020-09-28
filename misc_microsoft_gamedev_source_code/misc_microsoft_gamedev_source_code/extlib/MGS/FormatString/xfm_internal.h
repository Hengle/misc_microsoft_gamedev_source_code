#ifndef _XFM_INTERNAL_
#define _XFM_INTERNAL_
#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif

// from public/sdk/inc/ntdef.h
typedef LONG NTSTATUS;
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)((ULONG_PTR)(ArgumentPointer)) != (CHAR *)(NULL) )
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

// from public/sdk/inc/ntstatus.h
#define STATUS_SUCCESS                          ((NTSTATUS)0x00000000L) // ntsubauth
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

// from public/sdk/inc/ntrtl.h
typedef enum {
    ParseContextValid = 0x1,
    ParseContextWrapping = 0x2
} ParseContextFlags;

typedef struct _PARSE_MESSAGE_CONTEXT {
    ULONG fFlags;
    ULONG cwSavColumn;
    SIZE_T iwSrc;
    SIZE_T iwDst;
    SIZE_T iwDstSpace;
    va_list lpvArgStart;
} PARSE_MESSAGE_CONTEXT, *PPARSE_MESSAGE_CONTEXT;

#define TEST_PARSE_MESSAGE_CONTEXT_FLAG(a,b) ((a)->fFlags & (b))
#define SET_PARSE_MESSAGE_CONTEXT_FLAG(a,b) ((a)->fFlags |= (b))
#define CLEAR_PARSE_MESSAGE_CONTEXT_FLAG(a,b) ((a)->fFlags &= ~(b))

// ntos_message.c
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
    );

// win32_message.c
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
                    );

#define ASSERT(x)
#define BaseSetLastNTError(x) XFormatMessage_SetLastError(x)

#endif
