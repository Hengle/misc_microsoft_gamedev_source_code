//////////////////////////////////////////////////////////////////////////////
//
//	File:		cpwth.cpp
//	Module:		cpwth.exe - Test DetourCreateProcessWithDll function.
//
//  Copyright 1998-2001, Microsoft Corporation
//
//	Microsoft Research Detours Package Version 1.4. (Build 44)
//
//	http://www.research.microsoft.com/sn/detours
//
#include <stdio.h>
#include <windows.h>
#include <detours.h>

#define arrayof(x)		(sizeof(x)/sizeof(x[0]))

//////////////////////////////////////////////////////////////////////////////
//
void PrintUsage(void)
{
	printf("Usage:\n"
		   "    withdll [options] [command line]\n"
		   "Options:\n"
		   "    /d:file.dll  : Start the process with file.dll.\n"
		   "    /?           : This help screen.\n");
}

//////////////////////////////////////////////////////////////////////// main.
//
int CDECL main(int argc, char **argv)
{
	BOOLEAN fNeedHelp = FALSE;
	PCHAR pszDllPath = NULL;

	for (int arg = 1;
		 arg < argc && (argv[arg][0] == '-' || argv[arg][0] == '/');
		 arg++) {
		
        CHAR *argn = argv[arg] + 1;
        CHAR *argp = argn;
        while (*argp && *argp != ':')
            argp++;
        if (*argp == ':')
            *argp++ = '\0';
			
		switch (argn[0]) {
		  case 'd':										// Set DLL Name
		  case 'D':
            pszDllPath = argp;
			break;

		  case 'h':										// Help
		  case 'H':
		  case '?':
			fNeedHelp = TRUE;
			break;
			
		  default:
			fNeedHelp = TRUE;
			printf("Bad argument: %s\n", argv[arg]);
			break;
		}
	}

	if (arg >= argc) {
		fNeedHelp = TRUE;
	}
	
	if (fNeedHelp) {
		PrintUsage();
		return 1;
	}

	//////////////////////////////////////////////////////////////////////////

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	CHAR szCommand[2048];
	CHAR szExe[1024];
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	szCommand[0] = L'\0';
	strcpy(szExe, argv[arg]);
	for (; arg < argc; arg++) {
		if (strchr(argv[arg], ' ') != NULL || strchr(argv[arg], '\t') != NULL) {
			strcat(szCommand, "\"");
			strcat(szCommand, argv[arg]);
			strcat(szCommand, "\"");
		}
		else {
			strcat(szCommand, argv[arg]);
		}
		
		if (arg + 1 < argc)
			strcat(szCommand, " ");
	}
	printf("Loading: `%s'\n\n", szCommand);

	DWORD dwCreationFlags = (CREATE_DEFAULT_ERROR_MODE);

	SetLastError(0);

	CHAR szFullExe[1024] = "\0";
	PCHAR pszFileExe = NULL;
	
	SearchPath(NULL, szExe, ".exe", arrayof(szFullExe), szFullExe, &pszFileExe);
	if (szFullExe[0] == 0) {
		printf("Executable: %s\n", szFullExe);
	}

	if (szFullExe[0] != '\0') {
		if (!DetourCreateProcessWithDll(szFullExe, szCommand, NULL, NULL, TRUE,
                                        dwCreationFlags, NULL, NULL,
                                        &si, &pi, pszDllPath, NULL)) {
			printf("DetourCreateProcessWithDll failed: %d\n", GetLastError());
            ExitProcess(2);
		}
	}
	else {
		if (!DetourCreateProcessWithDll(NULL, szCommand, NULL, NULL, TRUE,
                                        dwCreationFlags, NULL, NULL,
                                        &si, &pi, pszDllPath, NULL)) {
			printf("DetourCreateProcessWithDll failed: %d\n", GetLastError());
            ExitProcess(2);
		}
	}
	
 	
	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD dwResult = 0;
	if (!GetExitCodeProcess(pi.hProcess, &dwResult)) {
		printf("GetExitCodeProcess failed: %d\n", GetLastError());
		dwResult = 3;
	}
	
	return dwResult;
}

//////////////////////////////////////////////////////////////////////////////
// End of File
