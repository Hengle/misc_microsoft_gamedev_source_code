
#include "stdafx.h"
#include "TraceModule.hpp"
#include <dbghelp.h>

void ModuleUnload(HANDLE hProcess, DWORD64 dwBaseAddr)
{
	SymUnloadModule64( hProcess, dwBaseAddr );
	SymCleanup( hProcess );
	return;
}

//class CLogEntryCallStack
CTraceModule::CTraceModule(char *szName, DWORD64 dwAddress, DWORD64 dwSize)
{
	Set(szName, dwAddress, dwSize);
}
CTraceModule::CTraceModule()
{
	; // does not initialize
}
CTraceModule::~CTraceModule()
{
	if(m_bModuleLoaded)
	{
		ModuleUnload((HANDLE)m_hProcess, m_dwBaseAddress);

	}
}
void CTraceModule::Set(char *szName, DWORD64 dwAddress, DWORD64 dwSize)
{
	// copy the value to the class
	// validate?
	strcpy_s(m_szName, _countof(m_szName), szName);
	m_dwAddress=dwAddress;
	m_dwSize=dwSize;
	m_hProcess=NULL;
	m_bModuleLoaded = false;
	return;
}

void FullPath(char *szBuffer, int nBufferLength, char *szPath, char *szFileName)
{
	int nPathLength=MIN(strlen(szPath), nBufferLength-1);
	strcpy_s(szBuffer, nBufferLength, szPath);
	if('\\'!=szBuffer[nPathLength-1]&&nPathLength<nBufferLength-1) // check for trailing backslash
		{
			szBuffer[nPathLength]='\\'; // overwrite null
			nPathLength++;
		}
	strcpy_s(szBuffer+nPathLength, nBufferLength-nPathLength, szFileName);
	return;
}

// returns path index or -1 if not found
// todo: check without fopen?
int FindFile(char **rgszPath, int nPathCount, char *szFileName)
{
	char szBuffer[1024];
	bool bFound=false;
	int index=0;
	FILE *fp;

	do
	{
		FullPath(szBuffer, 1024, rgszPath[index], szFileName);
		fopen_s(&fp, szBuffer, "r"); // note call failure is handled
		if(NULL!=fp)
		{
			bFound=true;
			fclose(fp);
		}
		else
		{
			index++;
		}
	}while(!bFound && index<nPathCount);
	return bFound?index : -1;
}


void PdbFromModule(char *szBuffer, int nBufferLength, char *szModuleName)
{
	char *szSuffix=".pdb";
	int nSuffixLength=4;
	char *szDot= strchr(szModuleName, '.');
	int nNameLength=MIN(szDot-szModuleName, nBufferLength-nSuffixLength);
	strcpy_s(szBuffer, nBufferLength, szModuleName); 
	strcpy_s(szBuffer+nNameLength, nBufferLength-nNameLength, szSuffix); 
	return;
}


// general - helper
bool ModuleLoad(HANDLE *phProcess, char *szPdbPath, char *szModuleName, DWORD64 dwBase, DWORD64 dwSize, DWORD64 *pdwBaseAddr)
{
	bool succeeded=true;

	if ( !SymInitialize( *phProcess, NULL, FALSE ) )
	{
		succeeded=false;
		goto Exit;
	}

	DWORD dwOptions = SymGetOptions();
	SymSetOptions( dwOptions | SYMOPT_LOAD_ANYTHING | SYMOPT_LOAD_LINES );

	DWORD64 dwBaseAddr = SymLoadModule( *phProcess, NULL, szPdbPath, szModuleName, (DWORD)dwBase, (DWORD)dwSize );

	if ( !dwBaseAddr )
	{
		SymCleanup( *phProcess );
		succeeded=false;
	}
	*pdwBaseAddr = dwBaseAddr;   // pdwBaseAddr undefined on fail
Exit:
	return succeeded;
}
// for class
bool CTraceModule::ModuleLoad(char **rgszModulePath, int nPathCount)
{
	bool succeeded=false;
	int nPathBufferLength=1024;
	char szPathBuffer[1024];
	int nPathIndex = FindFile(rgszModulePath, nPathCount, m_szName);
	if(nPathIndex>=0)
	{
				char *szModulePath = rgszModulePath[nPathIndex];   // shorthand
				int nPathLength=MIN(strlen(szModulePath), nPathBufferLength-1);
				m_dwBaseAddress=0;
				strcpy_s(szPathBuffer, nPathBufferLength, szModulePath);
				// todo: path wrapper
				if('\\'!=szModulePath[nPathLength-1]&&nPathLength<nPathBufferLength-1) // check for trailing backslash
				{
					szPathBuffer[nPathLength]='\\';
					nPathLength++;
				}
				PdbFromModule(szPathBuffer+nPathLength, nPathBufferLength-nPathLength, m_szName);
				HANDLE hHandle = (HANDLE)m_hProcess; // avoid generated temp variable in cast
				succeeded = ::ModuleLoad(&hHandle, szPathBuffer, m_szName, m_dwAddress, m_dwSize, &m_dwBaseAddress);
	}
	if(succeeded)
	{
		m_bModuleLoaded=true;
	}

	return succeeded;
}
