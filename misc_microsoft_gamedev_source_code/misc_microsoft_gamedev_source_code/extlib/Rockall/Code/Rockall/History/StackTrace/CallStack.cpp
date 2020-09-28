// CallStack.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>
#include <dbghelp.h>
#include "callstack.h"
//
// Ensure that the last function call(s) before StackWalk
// are not FPO-optimized.
//
#pragma optimize("y", off)

bool FormatSymbolA(HANDLE hProcess,
                   LPVOID pAddress,
                   LPSTR  rgchSymbol,
                   DWORD  cbSymbol)
{
    bool
        bReturn = true;
    char
        szError[512] = "\0";
    BOOL
        fSym;
    IMAGEHLP_MODULE
        mi = {0};
    CHAR
        rgchSymBuffer[sizeof(IMAGEHLP_SYMBOL) + SYMNAMELENGTH],
        rgchLine[SYMNAMELENGTH],
        rgchSymName[SYMNAMELENGTH],
        *szSymbol = rgchSymbol;
    PIMAGEHLP_SYMBOL
        psym = (PIMAGEHLP_SYMBOL)rgchSymBuffer;   
    LPSTR
        pszT = NULL;
    DWORD
        dwOffset = 0;
    mi.SizeOfStruct  = sizeof(IMAGEHLP_MODULE);

    ZeroMemory(psym, sizeof(IMAGEHLP_SYMBOL) + SYMNAMELENGTH);
    psym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    psym->MaxNameLength = SYMNAMELENGTH;

    if (SymGetModuleInfo(hProcess, (DWORD)pAddress, &mi))
    {
        sprintf(rgchLine, "%s!", mi.ModuleName);
        if (strlen(rgchLine) < cbSymbol)
        {
            strcpy(rgchSymbol, rgchLine);
            cbSymbol -= (ULONG) strlen(rgchLine);
            szSymbol += strlen(rgchLine);
        }
    }
    else
    {
        bReturn = false;
        strcpy(szSymbol, "none!");
        cbSymbol -= (ULONG)strlen(szSymbol);
        szSymbol += strlen(szSymbol);
    }

    if (bReturn)
    {
        fSym = SymGetSymFromAddr(hProcess,
                                 (DWORD)pAddress,
                                 &dwOffset,
                                 psym);
        if (fSym)
        {
            if (!SymUnDName(psym, rgchSymName, sizeof(rgchSymName)))
                lstrcpynA(rgchSymName, &(psym->Name[1]), sizeof(rgchSymName));
            pszT = rgchSymName;
        }
        else
        {
            bReturn = false;
            pszT = NULL;
            sprintf(rgchLine, "0x%p", pAddress);
            if (strlen(rgchLine) < cbSymbol)
            {
                strcpy(szSymbol, rgchLine);
                cbSymbol -= (ULONG) strlen(rgchLine);
                szSymbol += strlen(rgchLine);
            }
        }

        if (pszT)
        {
            if (strlen(pszT) < cbSymbol)
            {
                strcpy(szSymbol, pszT);
                szSymbol += strlen(pszT);
                cbSymbol -= (ULONG)strlen(pszT);
            }
            sprintf(rgchLine, "+0x%x", dwOffset);
            if (strlen(rgchLine) < cbSymbol)
            {
                strcpy(szSymbol, rgchLine);
                szSymbol += strlen(rgchLine);
                cbSymbol -= (ULONG) strlen(rgchLine);
            }
        }
    }
    else
    {
        sprintf(rgchLine, "0x%p", pAddress);
        if (strlen(rgchLine) < cbSymbol)
        {
            strcpy(szSymbol, rgchLine);
            szSymbol += strlen(rgchLine);
            cbSymbol -= (ULONG) strlen(rgchLine);
        }
    }
    return bReturn;
}

bool FormatSymbolW(HANDLE hProcess,
                   LPVOID pAddress,
                   LPWSTR rgwchSymbol,
                   DWORD  cSymbol)
{
    char
        rgchSymbol[SYMNAMELENGTH] = "\0";
    bool
        bReturn = FormatSymbolA(hProcess, pAddress, rgchSymbol, SYMNAMELENGTH);
    DWORD
        cLen = strlen(rgchSymbol);
    if (cLen > cSymbol)
        return false;
    MultiByteToWideChar(CP_ACP, 0, rgchSymbol, -1, rgwchSymbol, cSymbol);
    return bReturn;
}

// !!!
// the caller is responsible for allocating both the array and each string
// of the array
// !!!
DWORD FormatCallStackA(LPVOID *rgdwCaller,
                       LPSTR  *rgStrings,
                       DWORD   cStrLen,       //the length of each string including '\0'
                       DWORD   cStrings)
{
    DWORD
        cCount = 0;
    HANDLE
        hProcess = GetCurrentProcess();

    if (!rgStrings)
    {
        goto _Error;
    }
    while (rgdwCaller[cCount] && cStrings > 0)
    {
        FormatSymbolA(hProcess,
                      rgdwCaller[cCount],
                      rgStrings[cCount],
                      cStrLen);
        cCount++;
        cStrings--;
    }
_Error:
    return cCount;
}

DWORD GetCallStack(LPVOID *rgdwCaller, DWORD cFind, DWORD cSkipFrames)
{
    STACKFRAME
        stkfrm;
    CONTEXT
        ctxt;
    HANDLE
        hThread,
        hProcess;
    DWORD
        machType,
        cInitial = cFind;

    if (!cFind)
        return NULL;
    
    // initialization
    ZeroMemory(&ctxt, sizeof(CONTEXT));
    ZeroMemory(&stkfrm, sizeof(STACKFRAME));
    ZeroMemory(rgdwCaller, cFind * sizeof(LPVOID));

    hThread = GetCurrentThread();
    hProcess = GetCurrentProcess();
    ctxt.ContextFlags = CONTEXT_FULL;
    GetThreadContext(hThread, &ctxt);

    machType = IMAGE_FILE_MACHINE_I386;
    _asm
    {
        mov stkfrm.AddrStack.Offset, esp
        mov stkfrm.AddrFrame.Offset, ebp
        mov stkfrm.AddrPC.Offset, offset DummyLabel
    DummyLabel:
    }

    stkfrm.AddrPC.Mode = AddrModeFlat;
    stkfrm.AddrStack.Mode = AddrModeFlat;
    stkfrm.AddrFrame.Mode = AddrModeFlat;

    while (cFind > 0)
    {
        if (!StackWalk(machType,
                       hProcess,
                       hThread,
                       &stkfrm,
                       &ctxt,
                       NULL,
                       SymFunctionTableAccess,
                       SymGetModuleBase,
                       NULL))
            break;
        //
        // skip the frame
        //
        if (cSkipFrames)
        {
            cSkipFrames--;
        }
        else
        {
            *rgdwCaller++ = (LPVOID)stkfrm.AddrPC.Offset;
            cFind--;
        }
    }
    return cInitial - cFind;
}


BOOL __stdcall FReloadSymbolsCallback(PSTR szModuleName,
                                      ULONG_PTR ulBaseOfDLL,
                                      ULONG cbSizeOfDLL,
                                      void *pvContext)
{
    char
        szOut[512];
    if (SymGetModuleBase(GetCurrentProcess(), ulBaseOfDLL) == 0)
    {
        if (SymLoadModule(GetCurrentProcess(),
                          NULL,
                          szModuleName,
                          NULL,
                          ulBaseOfDLL,
                          cbSizeOfDLL))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL _ReloadSymbols(LPSTR symPath)
{
    char
        szOut[512];

    if (!SymInitialize(GetCurrentProcess(), symPath, TRUE))
    {
        sprintf(szOut, "Missing or mismatched symbols files: %x\n",
                HRESULT_FROM_WIN32(GetLastError()));
        OutputDebugStringA(szOut);
    }

    if (!EnumerateLoadedModules(GetCurrentProcess(), FReloadSymbolsCallback, NULL))
    {
        sprintf(szOut, "::EnumerateLoadedModules() = %x\n",
                HRESULT_FROM_WIN32(GetLastError()));
        OutputDebugStringA(szOut);
        return FALSE;
    }
    return TRUE;
}

