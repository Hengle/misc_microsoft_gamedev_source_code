#include "stdafx.h"
#include "TraceStackStep.hpp"
#include <assert.h>

// helpers

// parse hex value - does not require null termination
__int64 ReadHex(char *szHex, int nLength)
{
	__int64 nValue=0;
	char buffer[128];
	strncpy_s(buffer, _countof(buffer), szHex, nLength);
	sscanf_s(buffer, "%x", &nValue);

	return nValue;
}
__int64 ReadHex(char *szHex)
{
	// todo: validate
	return ReadHex(szHex, (int)strlen(szHex));
}

CTraceStackStep::CTraceStackStep()
{
	m_szModuleName[0]=NULL;
	m_szFunctionName[0]=NULL;
	m_szFileName[0]=NULL;
	m_nLineNumber=0;
}
CTraceStackStep::~CTraceStackStep()
{
	;
}
bool CTraceStackStep::Set(char *szName, DWORD64 dwAddress, DWORD64 dwOffset)
{
	return Set(szName, (int)strlen(szName), dwAddress, dwOffset);
}
bool CTraceStackStep::Set(char *szName, int nNameLength, DWORD64 dwAddress, DWORD64 dwOffset)
{
	// copy the value to the class
	// validate?
	strncpy_s(m_szModuleName, _countof(m_szModuleName), szName, nNameLength); // explicit string length not buffer length - requires truncation
	m_dwAddress=dwAddress;
	m_dwOffset=dwOffset;
	return true; // todo
}
bool CTraceStackStep::Set(char *szName, int nNameLength, char *szAddress, int nAddressLength, char *szOffset, int nOffsetLength)
{
	bool success=false;
	success = Set(szName, nNameLength, ReadHex(szAddress, nAddressLength), ReadHex(szOffset, nOffsetLength));
	return success;
}