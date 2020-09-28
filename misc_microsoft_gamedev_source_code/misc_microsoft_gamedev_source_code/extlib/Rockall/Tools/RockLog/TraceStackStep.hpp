#include "stdafx.h"

class CTraceStackStep
{
public:
	CTraceStackStep();
	~CTraceStackStep();
	CTraceStackStep(char *szModuleName, DWORD64 dwAddress, DWORD64 dwOffset);
	bool Set(char *szName, DWORD64 dwAddress, DWORD64 dwSize);
	bool Set(char *szName, int nNameLength, DWORD64 dwAddress, DWORD64 dwSize);
	bool Set(char *szName, int nNameLength, char *szAddress, int nAddressLength, char *szSize, int nSizeLength); // for parsing
	char m_szModuleName[ModuleNameMaxLength];
	char m_szFunctionName[FunctionNameMaxLength];
	char m_szFileName[MAX_PATH];
	int m_nLineNumber;
	DWORD64 m_dwAddress;
	DWORD64 m_dwOffset;	
};

__int64 ReadHex(char *szHex);
__int64 ReadHex(char *szHex, int nLength);