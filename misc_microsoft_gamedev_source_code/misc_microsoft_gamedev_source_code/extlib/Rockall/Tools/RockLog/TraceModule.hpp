
#include "stdafx.h"

// does not allocate
class CTraceModule
{
public:
	CTraceModule();
	~CTraceModule();
	CTraceModule(char *szName, DWORD64 dwAddress, DWORD64 dwSize);
	void Set(char *szName, DWORD64 dwAddress, DWORD64 dwSize);
	char m_szName[ModuleNameMaxLength];
	DWORD64 m_dwAddress;
	DWORD64 m_dwSize;	
	int m_hProcess; // unique identifier only - non-null does not indicate valid handle.  Type int not type HANDLE
	bool m_bModuleLoaded; // module is loaded
	DWORD64 m_dwBaseAddress; // remove?
	bool ModuleLoad(char **rgszModulePath, int nPathCount);
};