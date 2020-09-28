#ifndef _XFORMATMESSAGE_
#define _XFORMATMESSAGE_
#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
// The following two flags are reserved
//#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
//#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000
#define FORMAT_MESSAGE_MAX_WIDTH_MASK  0x000000FF

//#define FormatMessage  FormatMessageW

#ifdef __cplusplus
extern "C" {
#endif
//WINBASEAPI
DWORD
WINAPI
XFormatMessageW(
    IN DWORD dwFlags,
    IN LPCVOID lpSource,
    IN DWORD dwMessageId,
    IN DWORD dwLanguageId,
    OUT LPWSTR lpBuffer,
    IN DWORD nSize,
    IN va_list *Arguments
    );

//WINBASEAPI
DWORD
WINAPI
XFormatMessage_GetLastError(
    VOID
    );

//WINBASEAPI
VOID
WINAPI
XFormatMessage_SetLastError(
    DWORD dwErrCode
    );
#ifdef __cplusplus
}
#endif
#endif