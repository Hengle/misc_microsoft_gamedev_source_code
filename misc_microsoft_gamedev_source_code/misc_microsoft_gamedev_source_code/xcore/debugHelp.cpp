//==============================================================================
// debughelp.cpp
//
// Copyright (c) 2002 Ensemble Studios
//==============================================================================

#include "xcore.h"
#include <dbghelp.h>
#include "debughelp.h"

//==============================================================================
// BDebugHelp::getCriticalSection
//==============================================================================
BCriticalSection& BDebugHelp::getCriticalSection(void)
{
   static BCriticalSection criticalSection;
   return criticalSection;
}

//==============================================================================
// BDebugHelp::BDebugHelp
//==============================================================================
BDebugHelp::BDebugHelp(HANDLE process)
{
   BScopedCriticalSection lock(getCriticalSection());
   
   mTriedToLoad = false;
   if(process == INVALID_HANDLE_VALUE)
      mProcess = GetCurrentProcess();
   else
      mProcess = process;
      
   clearFunctionPointers();
}


//==============================================================================
// BDebugHelp::clearFunctionPointers
//==============================================================================
void BDebugHelp::clearFunctionPointers(void)
{
   BScopedCriticalSection lock(getCriticalSection());
   
   mDbgHelpDLL=NULL;
   mSymInitialize=NULL;
   mSymSetOptions=NULL;
   mSymCleanup=NULL;
   mStackWalk=NULL;
   mSymFunctionTableAccess=NULL;
   mSymGetModuleBase=NULL;
	mSymGetLineFromAddr=NULL;
	mSymGetSymFromAddr=NULL;
	mSymGetModuleInfoW=NULL;
	mSymSetContext=NULL;
}


//==============================================================================
// BDebugHelp::~BDebugHelp
//==============================================================================
BDebugHelp::~BDebugHelp()
{
   // rg [12/31/05] - I'm assuming the destructor will always be executed at exit time by the main thread, and no other threads will try to use DebugHelp.

	// Deinit.
	if(mSymCleanup)
		mSymCleanup(mProcess);

	// Free the DLL.
	if(mDbgHelpDLL)
		FreeLibrary(mDbgHelpDLL);

	// Clear pointers.
	clearFunctionPointers();
}


//==============================================================================
// BDebugHelp::loadDLL
//==============================================================================
bool BDebugHelp::loadDLL(void)
{
   BScopedCriticalSection lock(getCriticalSection());
   
	// If already loaded, we don't need to do it again.
	if(mDbgHelpDLL)
		return(true);

	// If we already tried loading, don't try again.
	if(mTriedToLoad)
		return(false);

	// Try to load the DLL.
	mDbgHelpDLL = LoadLibrary(B("dbghelp.dll"));

	// Remember that we tried no matter the result.
	mTriedToLoad=true;

	// If load failed, bail.
	if(!mDbgHelpDLL)
		return(false);

	// Map the functions we want.
	mSymInitialize=(SYMINITIALIZE_FUNC)GetProcAddress(mDbgHelpDLL, "SymInitialize");
	mSymSetOptions=(SYMSETOPTIONS_FUNC)GetProcAddress(mDbgHelpDLL, "SymSetOptions");
	mSymCleanup=(SYMCLEANUP_FUNC)GetProcAddress(mDbgHelpDLL, "SymCleanup");
	mStackWalk=(SYMSTACKWALK_FUNC)GetProcAddress(mDbgHelpDLL, "StackWalk");
	mSymFunctionTableAccess=(SYMFUNCTIONTABLEACCESS_FUNC)GetProcAddress(mDbgHelpDLL, "SymFunctionTableAccess");
	mSymGetModuleBase=(SYMGETMODULEBASE_FUNC)GetProcAddress(mDbgHelpDLL, "SymGetModuleBase");
	mSymGetLineFromAddr=(SYMGETLINEFROMADDR_FUNC)GetProcAddress(mDbgHelpDLL, "SymGetLineFromAddr");
	mSymGetSymFromAddr=(SYMGETSYMFROMADDR_FUNC)GetProcAddress(mDbgHelpDLL, "SymGetSymFromAddr");
	mSymGetModuleInfoW=(SYMGETMODULEINFOW_FUNC)GetProcAddress(mDbgHelpDLL, "SymGetModuleInfoW");
	mSymSetContext=(SYMSETCONTEXT_FUNC)GetProcAddress(mDbgHelpDLL, "SymSetContext");

	// Make sure we got all the functions we wanted.  Otherwise the DLL is not the one we expected.
	if(!mSymInitialize || !mSymSetOptions || !mSymCleanup || !mStackWalk || !mSymFunctionTableAccess ||
		!mSymGetModuleBase || !mSymGetLineFromAddr || !mSymGetSymFromAddr || !mSymGetModuleInfoW || !mSymSetContext)
		return(false);

	// Initialize dbghelp.
	// First set our options up.
	mSymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

	// Get the directory that the exe is located in (as distinct from working directory which is automatically
	// searched for symbols).
	char cFilename[_MAX_PATH];
	GetModuleFileNameA(NULL, cFilename, _MAX_PATH);
	char *pos=strrchr(cFilename, '\\');
	if(pos)
		*pos=0;

	// Init the symbol parser.
	BOOL ok=mSymInitialize(mProcess, cFilename, TRUE);
	if(!ok)
		return(false);

	// Success.
	return(true);
}


//==============================================================================
// BDebugHelp::stackWalk
//==============================================================================
bool BDebugHelp::stackWalk(BDebugCallstack &stack, long numToSkip, CONTEXT &context, HANDLE procHandle, HANDLE threadHandle)
{
	// Load the DLL if necessary.
	if(!loadDLL())
		return(false);

	// We always want to skip THIS function.
	numToSkip++;

	// Set up a stackframe.  We need to populate with some info about the current location
	// in the code to get things started.
	STACKFRAME stackFrame;
	memset(&stackFrame, 0, sizeof(stackFrame));
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Mode = AddrModeFlat;

	stackFrame.AddrPC.Offset=context.Eip;
	stackFrame.AddrStack.Offset=context.Esp;
	stackFrame.AddrFrame.Offset = context.Ebp;

	// Set up a buffer for symbol info.
	const long symBufferSize=256+sizeof(IMAGEHLP_SYMBOL);
	char symBuffer[symBufferSize];
	PIMAGEHLP_SYMBOL symInfo=(PIMAGEHLP_SYMBOL)symBuffer;
	memset(&symBuffer, 0, symBufferSize);
	symInfo->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL);
	symInfo->MaxNameLength=256;

	// Set up struct for module info.
	IMAGEHLP_MODULEW modInfo;
	memset(&modInfo, 0, sizeof(modInfo));
	modInfo.SizeOfStruct=sizeof(modInfo);

	// Set up struct for line info.
	IMAGEHLP_LINE lineInfo;
	memset(&lineInfo, 0, sizeof(lineInfo));
	lineInfo.SizeOfStruct=sizeof(lineInfo);

	CONTEXT previousContext;

	long level=0;
	BOOL ok=TRUE;
	do
	{
		// Copy the context since it gets changed by the stackwalk call to the NEXT context.
		previousContext = context;

		// Next thing down the stack.
		ok=mStackWalk(IMAGE_FILE_MACHINE_I386, procHandle, threadHandle, &stackFrame, &context, 
			NULL, mSymFunctionTableAccess, mSymGetModuleBase, NULL);

		// If we're done skipping, grab info and append to string.
		if(ok && level>=numToSkip)
		{
			// Expand stack array.
			BDebugCallStackEntry *entry = stack.grow();
			if(!entry)
				return(false);

			// Copy context.
			memcpy(&entry->mContext, &previousContext, sizeof(previousContext));

			// Get the module info.
			BOOL symOk=mSymGetModuleInfoW(procHandle, stackFrame.AddrPC.Offset, (IMAGEHLP_MODULEW*)&modInfo);
			if(symOk)
			{
				// Copy module name.
            StringCchCopyNExW(entry->mModule, BDebugCallStackEntry::cMaxStringSize, modInfo.ImageName, BDebugCallStackEntry::cMaxStringSize, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
				entry->mModule[BDebugCallStackEntry::cMaxStringSize-1]=0;
			}

			// Get function name.
			DWORD displacement=0;
			symOk=mSymGetSymFromAddr(procHandle, stackFrame.AddrPC.Offset, &displacement, symInfo);
			if(symOk)
			{
				// Copy symbol name.
				MultiByteToWideChar(CP_ACP, 0, symInfo->Name, -1, entry->mFunctionName, BDebugCallStackEntry::cMaxStringSize);

				// Grab offset.
				entry->mFunctionOffset=displacement;
			}


			// Get file and line.
			DWORD offset=0;
			symOk=mSymGetLineFromAddr(procHandle, stackFrame.AddrPC.Offset, &offset, &lineInfo);
			if(symOk)
			{
				// Copy filename.
				MultiByteToWideChar(CP_ACP, 0, lineInfo.FileName, -1, entry->mFile, BDebugCallStackEntry::cMaxStringSize);

				// Grab line number.
				entry->mLine=lineInfo.LineNumber;

				// Grab line offset.
				entry->mLineOffset=offset;
			}
		}
		level++;
	} while(ok && level<500);

	return(true);
}


//==============================================================================
// BDebugHelp::stackWalk
//==============================================================================
bool BDebugHelp::stackWalk(BDebugCallstack &stack, long numToSkip)
{
	// Grab our context info ... instruction pointer/stack info.
#define push_eip __asm _emit 0e8h __asm _emit 00h __asm _emit 00h __asm _emit 00h __asm _emit 00h
	DWORD saveEIP;
	DWORD saveEBP;
	DWORD saveESP;
	push_eip
		__asm pop eax
		__asm mov saveEIP, eax;
	__asm mov saveEBP, ebp;
	__asm mov saveESP, esp;

	// Fill in context.
	CONTEXT context;
	memset(&context, 0, sizeof(context));
	context.Ebp=saveEBP;
	context.Eip=saveEIP;
	context.Esp=saveESP;

	return(stackWalk(stack, numToSkip, context, GetCurrentProcess(), GetCurrentThread()));
}


//==============================================================================
// BDebugHelp::stackWalk
//==============================================================================
bool BDebugHelp::stackWalkCaller(BDebugCallStackEntry &entry, long stackEntry)
{
	// Load the DLL if necessary.
	if(!loadDLL())
		return(false);

	// We always want to skip THIS function.
	stackEntry++;

	// Set up a stackframe.  We need to populate with some info about the current location
	// in the code to get things started.
	STACKFRAME stackFrame;
	memset(&stackFrame, 0, sizeof(stackFrame));
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Mode = AddrModeFlat;

	// Grab our context info ... instruction pointer/stack info.
#define push_eip __asm _emit 0e8h __asm _emit 00h __asm _emit 00h __asm _emit 00h __asm _emit 00h
	DWORD saveEIP;
	DWORD saveEBP;
	DWORD saveESP;
	push_eip
		__asm pop eax
		__asm mov saveEIP, eax;
	__asm mov saveEBP, ebp;
	__asm mov saveESP, esp;
	stackFrame.AddrPC.Offset=saveEIP;
	stackFrame.AddrStack.Offset=saveESP;
	stackFrame.AddrFrame.Offset = saveEBP;

	// Fill some of this in too -- though I'm not sure if it's necessary.
	CONTEXT context;
	memset(&context, 0, sizeof(context));
	context.Ebp=saveEBP;
	context.Eip=saveEIP;
	context.Esp=saveESP;

	// Set up a buffer for symbol info.
	const long symBufferSize=256+sizeof(IMAGEHLP_SYMBOL);
	char symBuffer[symBufferSize];
	PIMAGEHLP_SYMBOL symInfo=(PIMAGEHLP_SYMBOL)symBuffer;
	memset(&symBuffer, 0, symBufferSize);
	symInfo->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL);
	symInfo->MaxNameLength=256;

	// Set up struct for module info.
	IMAGEHLP_MODULEW modInfo;
	memset(&modInfo, 0, sizeof(modInfo));
	modInfo.SizeOfStruct=sizeof(modInfo);

	// Set up struct for line info.
	IMAGEHLP_LINE lineInfo;
	memset(&lineInfo, 0, sizeof(lineInfo));
	lineInfo.SizeOfStruct=sizeof(lineInfo);

	long level=0;
	BOOL ok=TRUE;
	bool foundEntry = false;
	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();
	do
	{
		// Next thing down the stack.
		ok=mStackWalk(IMAGE_FILE_MACHINE_I386, process, thread, &stackFrame, &context, 
			NULL, mSymFunctionTableAccess, mSymGetModuleBase, NULL);

		// If we're done skipping, grab info and append to string.
		if(ok && level==stackEntry)
		{

			// Copy context.
			memcpy(&entry.mContext, &context, sizeof(context));

			// Get the module info.
			BOOL symOk=mSymGetModuleInfoW(process, stackFrame.AddrPC.Offset, (IMAGEHLP_MODULEW*)&modInfo);
			if(symOk)
			{
				// Copy module name.
				StringCchCopyNExW(entry.mModule, BDebugCallStackEntry::cMaxStringSize, modInfo.ImageName, BDebugCallStackEntry::cMaxStringSize, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
				entry.mModule[BDebugCallStackEntry::cMaxStringSize-1]=0;
			}

			// Get function name.
			DWORD displacement=0;
			symOk=mSymGetSymFromAddr(process, stackFrame.AddrPC.Offset, &displacement, symInfo);
			if(symOk)
			{
				// Copy symbol name.
				MultiByteToWideChar(CP_ACP, 0, symInfo->Name, -1, entry.mFunctionName, BDebugCallStackEntry::cMaxStringSize);

				// Grab offset.
				entry.mFunctionOffset=displacement;
			}


			// Get file and line.
			DWORD offset=0;
			symOk=mSymGetLineFromAddr(process, stackFrame.AddrPC.Offset, &offset, &lineInfo);
			if(symOk)
			{
				// Copy filename.
				MultiByteToWideChar(CP_ACP, 0, lineInfo.FileName, -1, entry.mFile, BDebugCallStackEntry::cMaxStringSize);

				// Grab line number.
				entry.mLine=lineInfo.LineNumber;

				// Grab line offset.
				entry.mLineOffset=offset;
			}
			foundEntry = true;
			break;
		}
		level++;
	} while(ok && level<500);

	return(foundEntry);
}

//==============================================================================
// BDebugHelp::stackWalk
//==============================================================================
bool BDebugHelp::stackWalkOffsets(DWORD* pData, long maxCallStackSize, long numToSkip, long allocSize, STACKWALKFUNC* pFunc, void* pParam, DWORD &outUsedCount)
{
   // Nothing used yet.
   outUsedCount = 0;
   
	// Load the DLL if necessary.
	if(!loadDLL())
		return(false);

	HANDLE procHandle = GetCurrentProcess();
	HANDLE threadHandle = GetCurrentThread();


	// We always want to skip THIS function.
	numToSkip++;

	// Set up a stackframe.  We need to populate with some info about the current location
	// in the code to get things started.
	STACKFRAME stackFrame;
	memset(&stackFrame, 0, sizeof(stackFrame));
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Mode = AddrModeFlat;

	// Grab our context info ... instruction pointer/stack info.
#define push_eip __asm _emit 0e8h __asm _emit 00h __asm _emit 00h __asm _emit 00h __asm _emit 00h
	DWORD saveEIP;
	DWORD saveEBP;
	DWORD saveESP;
	push_eip
		__asm pop eax
		__asm mov saveEIP, eax;
	__asm mov saveEBP, ebp;
	__asm mov saveESP, esp;
	stackFrame.AddrPC.Offset=saveEIP;
	stackFrame.AddrStack.Offset=saveESP;
	stackFrame.AddrFrame.Offset = saveEBP;

	// Fill some of this in too -- though I'm not sure if it's necessary.
	CONTEXT context;
	long size = sizeof(context);
	memset(&context, 0, size);
	context.Ebp=saveEBP;
	context.Eip=saveEIP;
	context.Esp=saveESP;

	long level=0;
	BOOL ok=FALSE;
	do
	{
		// Next thing down the stack.
		ok=mStackWalk(IMAGE_FILE_MACHINE_I386, procHandle, threadHandle, &stackFrame, &context, 
			NULL, mSymFunctionTableAccess, mSymGetModuleBase, NULL);

		// If we're done skipping, grab info and append to string.
		if(ok && level>=numToSkip)
		{
			if(pFunc)
				pFunc((long)stackFrame.AddrPC.Offset, allocSize, pParam);
			
			memcpy(pData, &stackFrame.AddrPC.Offset, sizeof(DWORD));
			pData++;
			maxCallStackSize--;
			outUsedCount++;
		}
		level++;
	} while(ok && level<500 && maxCallStackSize>0);

	return(true);
}

//==============================================================================
// BDebugHelp::stackWalk
//==============================================================================
bool BDebugHelp::resolveSymbols(DWORD offset, BDebugCallStackEntry &entry)
{
	// Load the DLL if necessary.
	if(!loadDLL())
		return(false);

	HANDLE procHandle = GetCurrentProcess();

	CONTEXT context;
	long size = sizeof(context);
	memset(&context, 0, size);
	if(size <= 0)
		return(false);

	// Set up a buffer for symbol info.
	const long symBufferSize=256+sizeof(IMAGEHLP_SYMBOL);
	char symBuffer[symBufferSize];
	PIMAGEHLP_SYMBOL symInfo=(PIMAGEHLP_SYMBOL)symBuffer;
	memset(&symBuffer, 0, symBufferSize);
	symInfo->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL);
	symInfo->MaxNameLength=256;

	// Set up struct for module info.
	IMAGEHLP_MODULEW modInfo;
	memset(&modInfo, 0, sizeof(modInfo));
	modInfo.SizeOfStruct=sizeof(modInfo);

	// Set up struct for line info.
	IMAGEHLP_LINE lineInfo;
	memset(&lineInfo, 0, sizeof(lineInfo));
	lineInfo.SizeOfStruct=sizeof(lineInfo);

   // Clear result before we fill anything in (or fail to do so).
   ZeroMemory(&entry, sizeof(entry));

	// Context?

	// Get the module info.
	BOOL symOk=mSymGetModuleInfoW(procHandle, offset, (IMAGEHLP_MODULEW*)&modInfo);
	if(symOk)
	{
		// Copy module name.
		StringCchCopyNExW(entry.mModule, BDebugCallStackEntry::cMaxStringSize, modInfo.ImageName, BDebugCallStackEntry::cMaxStringSize, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
		entry.mModule[BDebugCallStackEntry::cMaxStringSize-1]=0;
	}

	// Get function name.
	DWORD displacement=0;
	symOk=mSymGetSymFromAddr(procHandle, offset, &displacement, symInfo);
	if(symOk)
	{
		// Copy symbol name.
		MultiByteToWideChar(CP_ACP, 0, symInfo->Name, -1, entry.mFunctionName, BDebugCallStackEntry::cMaxStringSize);

		// Grab offset.
		entry.mFunctionOffset=displacement;
	}

	// Get file and line.
	DWORD lineOffset=0;
	symOk=mSymGetLineFromAddr(procHandle, offset, &lineOffset, &lineInfo);
	if(symOk)
	{
		// Copy filename.
		MultiByteToWideChar(CP_ACP, 0, lineInfo.FileName, -1, entry.mFile, BDebugCallStackEntry::cMaxStringSize);

		// Grab line number.
		entry.mLine=lineInfo.LineNumber;

		// Grab line offset.
		entry.mLineOffset=lineOffset;
	}

	return(true);
}


bool BDebugHelp::resolveSymbolsOld(DWORD* pContexts, long numCalls, BDebugCallstack &stack)
{
	// Load the DLL if necessary.
	if(!loadDLL())
		return(false);

	HANDLE procHandle = GetCurrentProcess();

	CONTEXT context;
	long size = sizeof(context);
	memset(&context, 0, size);
	if(size <= 0)
		return(false);

	// Set up a buffer for symbol info.
	const long symBufferSize=256+sizeof(IMAGEHLP_SYMBOL);
	char symBuffer[symBufferSize];
	PIMAGEHLP_SYMBOL symInfo=(PIMAGEHLP_SYMBOL)symBuffer;
	memset(&symBuffer, 0, symBufferSize);
	symInfo->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL);
	symInfo->MaxNameLength=256;

	// Set up struct for module info.
	IMAGEHLP_MODULEW modInfo;
	memset(&modInfo, 0, sizeof(modInfo));
	modInfo.SizeOfStruct=sizeof(modInfo);

	// Set up struct for line info.
	IMAGEHLP_LINE lineInfo;
	memset(&lineInfo, 0, sizeof(lineInfo));
	lineInfo.SizeOfStruct=sizeof(lineInfo);

	for(long i=numCalls-1; i>=0; i--)
	{
		// Expand stack array.
		BDebugCallStackEntry *entry = stack.grow();
		if(!entry)
			return(false);

		// Context?

		// Get the module info.
		BOOL symOk=mSymGetModuleInfoW(procHandle, pContexts[i], (IMAGEHLP_MODULEW*)&modInfo);
		if(symOk)
		{
			// Copy module name.
			StringCchCopyNExW(entry->mModule, BDebugCallStackEntry::cMaxStringSize, modInfo.ImageName, BDebugCallStackEntry::cMaxStringSize, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
			entry->mModule[BDebugCallStackEntry::cMaxStringSize-1]=0;
		}

		// Get function name.
		DWORD displacement=0;
		symOk=mSymGetSymFromAddr(procHandle, pContexts[i], &displacement, symInfo);
		if(symOk)
		{
			// Copy symbol name.
			MultiByteToWideChar(CP_ACP, 0, symInfo->Name, -1, entry->mFunctionName, BDebugCallStackEntry::cMaxStringSize);

			// Grab offset.
			entry->mFunctionOffset=displacement;
		}

		// Get file and line.
		DWORD offset=0;
		symOk=mSymGetLineFromAddr(procHandle, pContexts[i], &offset, &lineInfo);
		if(symOk)
		{
			// Copy filename.
			MultiByteToWideChar(CP_ACP, 0, lineInfo.FileName, -1, entry->mFile, BDebugCallStackEntry::cMaxStringSize);

			// Grab line number.
			entry->mLine=lineInfo.LineNumber;


			// Grab line offset.
			entry->mLineOffset=offset;
		}
   }
   
	return(true);
}

