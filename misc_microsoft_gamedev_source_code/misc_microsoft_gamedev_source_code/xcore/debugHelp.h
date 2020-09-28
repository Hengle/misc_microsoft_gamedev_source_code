//==============================================================================
// debughelp.h
//
// Copyright (c) 2000-2006, Ensemble Studios
//==============================================================================

#pragma once

#ifndef _DEBUGHELP_H_
#define _DEBUGHELP_H_

#ifdef XBOX
#error Don't include this on Xbox!
#endif

#include <dbghelp.h>
#include "debugcallstack.h"

//==============================================================================
// typedefs for functions we'll import
//==============================================================================
typedef BOOL (__stdcall *SYMINITIALIZE_FUNC)(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess);
typedef DWORD (__stdcall *SYMSETOPTIONS_FUNC)(DWORD SymOptions);
typedef BOOL (__stdcall *SYMCLEANUP_FUNC)(HANDLE hProcess);
typedef BOOL (__stdcall *SYMSTACKWALK_FUNC)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME StackFrame, PVOID ContextRecord, 
  PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine, 
  PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);
typedef PVOID (__stdcall *SYMFUNCTIONTABLEACCESS_FUNC)(HANDLE hProcess, DWORD AddrBase);
typedef DWORD (__stdcall *SYMGETMODULEBASE_FUNC)(HANDLE hProcess, DWORD dwAddr);
typedef BOOL (__stdcall *SYMGETLINEFROMADDR_FUNC)(HANDLE hProcess, DWORD dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE Line);
typedef BOOL (__stdcall *SYMGETSYMFROMADDR_FUNC)(HANDLE hProcess, DWORD Address, PDWORD Displacement, PIMAGEHLP_SYMBOL Symbol);
typedef BOOL (__stdcall *SYMGETMODULEINFOW_FUNC)(HANDLE hProcess, DWORD dwAddr, PIMAGEHLP_MODULEW ModuleInfo);
typedef BOOL (__stdcall *SYMSETCONTEXT_FUNC)(HANDLE hProcess, LPSTACKFRAME StackFrame, PVOID ContextRecord);

typedef void (STACKWALKFUNC)(long offset, long allocSize, void* pParam);


//==============================================================================
// class BDebugHelp
//==============================================================================

// rg [12/31/05] - I've made a quick attempt to make this class thread safe. 
// I'm assuming there will always be just one instance of this object, and it will 
// always be constructed/destructed by the main thread.
class BDebugHelp
{
   public:
                              BDebugHelp(HANDLE process = INVALID_HANDLE_VALUE);
                              ~BDebugHelp();

      bool                    stackWalk(BDebugCallstack &stack, long numToSkip);
      bool                    stackWalk(BDebugCallstack &stack, long numToSkip, CONTEXT &context, HANDLE procHandle, HANDLE threadHandle);
      bool                    stackWalkCaller(BDebugCallStackEntry &entry, long stackEntry);
		bool							stackWalkOffsets(DWORD* pData, long maxCallStackSize, long numToSkip, long allocSize, STACKWALKFUNC* pFunc, void* pParam, DWORD &outUsedCount);
		bool							resolveSymbols(DWORD offset, BDebugCallStackEntry &entry);
		bool							resolveSymbolsOld(DWORD* pContexts, long numCalls, BDebugCallstack &stack);

   protected:
      void                    clearFunctionPointers(void);
      bool                    loadDLL(void);

      // Module info for the DLL
      HMODULE                 mDbgHelpDLL;
      bool                    mTriedToLoad;
      
      HANDLE                  mProcess;

      // Function pointers.
      SYMINITIALIZE_FUNC      mSymInitialize;
      SYMSETOPTIONS_FUNC      mSymSetOptions;
      SYMCLEANUP_FUNC         mSymCleanup;
      SYMSTACKWALK_FUNC     mStackWalk;
      SYMFUNCTIONTABLEACCESS_FUNC mSymFunctionTableAccess;
      SYMGETMODULEBASE_FUNC mSymGetModuleBase;
      SYMGETLINEFROMADDR_FUNC mSymGetLineFromAddr;
      SYMGETSYMFROMADDR_FUNC  mSymGetSymFromAddr;
      SYMGETMODULEINFOW_FUNC  mSymGetModuleInfoW;
		SYMSETCONTEXT_FUNC		mSymSetContext;
		
		static BCriticalSection& getCriticalSection(void);
};


extern BDebugHelp gDebugHelp;

#endif


