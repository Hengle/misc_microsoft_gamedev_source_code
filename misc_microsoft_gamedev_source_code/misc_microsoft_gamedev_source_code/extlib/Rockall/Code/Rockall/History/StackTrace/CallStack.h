#ifndef __CALLSTACK_H__
#define __CALLSTACK_H__

#define SYMNAMELENGTH   512

bool FormatSymbolA(HANDLE, LPVOID , LPSTR, DWORD);
bool FormatSymbolW(HANDLE, LPVOID, LPWSTR, DWORD);

#ifdef UNICODE
#define FormatSymbol  FormatSymbolW
#else
#define FormatSymbol  FormatSymbolA
#endif // !UNICODE

DWORD FormatCallStackA(LPVOID *rgdwCaller, // the stack (an array of Return Addresses)
                       LPSTR  *rgStrings,  // the formatted stack returned in this array of strings
                       DWORD   cStrLen,    // the length of each string
                       DWORD   cStrings);  // number of entries(strings) in the rgStrings array

DWORD GetCallStack(LPVOID *, DWORD, DWORD = 1);
BOOL _ReloadSymbols(LPSTR);

#endif
