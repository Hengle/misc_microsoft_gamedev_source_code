// RockLog.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TraceModule.hpp"
#include "TraceStackStep.hpp"
#include <dbghelp.h>

#pragma pack(push,1)
struct SYMBOL_INFO_ext : public SYMBOL_INFO
{
	char m_nameExtra[1024];
	void Init()
	{
		SizeOfStruct = sizeof(SYMBOL_INFO);
		MaxNameLen = sizeof(m_nameExtra);
	}
};
#pragma pack(pop)

bool ValidateInput(int argc, char *argv[])
{
	bool valid=true;
	printf("\nRockLog\n");
	printf("Parses Rockall logs to provide callstack symbols\n");
	if(argc < 4)
	{
		printf("\narguments:\n");
		printf("  1. Log file to process\n");
		printf("  2. Output file \n");
		printf("  3. Module path\n");
		printf("  ...Module path\n");
		printf("ex. RockLog c:\\Rockall.log c:\\RockallWithSymbols.log c:\\foo c:\\bar\n");
		valid=false;
	}
	else
	{
		printf("\n");
		printf("  Log file to process: %s\n", argv[1]);
		printf("  Output log file: %s\n", argv[2]);
		for(int j=3;j<argc;j++)
		{
			printf("  Module Path %02d: %s\n", j-2, argv[j]);
		}
	}
	return valid;
}



bool ParseModuleList(FILE *streamLog, CTraceModule **prgLEM, int *pnLEMCount)
{
	char szLine[MaxLineLength]; 
	szLine[0]=NULL;

	*prgLEM = new CTraceModule[MaxModuleCount];  // todo: 2-pass alloc

	int nStepCount=0;
	const char *szStart="Module list start";
	const char *szEnd = "Module list end";
	int nStartLen = (int)strlen(szStart);
	int nEndLen=(int)strlen(szEnd);

	char szName[128];
	char szAddress[128];
	char szSize[128];
	while(0!=strncmp(szStart, szLine, nStartLen))
	{
	fgets(szLine, _countof(szLine), streamLog);
	}

	while(fgets(szLine, MaxLineLength, streamLog) && 0!=strncmp(szEnd, szLine, nEndLen) && nStepCount < MaxModuleCount) // at end of list, a line with only \n
	{
		if(3==sscanf3s_s(szLine, "%128s %128s %128s", szName, _countof(szName), szAddress, _countof(szAddress), szSize, _countof(szSize)))
		{
			(*prgLEM)[nStepCount].Set(szName, ReadHex(szAddress), ReadHex(szSize) );
			nStepCount++;
		}
	}
	*pnLEMCount=nStepCount;
	return (nStepCount>0);
}

bool ParseStack(FILE *streamLog, CTraceStackStep **prgLES, int *pnLESCount)
{
	bool succeeded=false;
	char szReadLine[MaxLineLength];  // todo
	szReadLine[0]=NULL;

	// assert(NULL==*prgLES);
	*prgLES = new CTraceStackStep[MaxStackSize];  // todo: 2-pass alloc

	int nStepCount=0;
	const char *szStackStart="Call Stack at Allocation";
	int nStackStartLen = (int)strlen(szStackStart);

	char *szLine;
	do
	{
		szLine = fgets(szReadLine, 128, streamLog);
	}while(0!=strncmp(szStackStart, szReadLine, nStackStartLen) && NULL!=szLine);

	while(fgets(szReadLine, 1024, streamLog) &&strlen(szReadLine)>1) // at end of list, a line with only \n
	{
		char *szSeparator1=strchr(szReadLine, '@');
		char *szSeparator2=strchr(szReadLine, '(');
		char *szSeparator3=strchr(szReadLine, ')');
		if(NULL != szSeparator1 && szSeparator1 < szSeparator2 && szSeparator2 < szSeparator3 && nStepCount<MaxStackSize)
		{
			(*prgLES)[nStepCount].Set(szReadLine, (int)(szSeparator1-szReadLine), szSeparator1+1, (int)(szSeparator2-szSeparator1-1), szSeparator2+2, (int)(szSeparator3-szSeparator2-2)); // format is (+0x00000000)
			nStepCount++;
			succeeded=true; // stack found
		}
	}
	*pnLESCount=nStepCount;
	return succeeded; // todo
}



void SymbolLookup(HANDLE hProcess, DWORD64 dwAddress, char *szSymbolName, int nSymbolNameMaxLength, char *szFileName, int nFileNameMaxLength, int *pLineNumber)
{
	BOOL b;
	DWORD64 dwDisplacement = 0;
	SYMBOL_INFO_ext symbol;
	symbol.Init();
	b=SymFromAddr( hProcess, dwAddress, &dwDisplacement, &symbol );
	strcpy_s(szSymbolName, nSymbolNameMaxLength, symbol.Name);

	IMAGEHLP_LINE line;
	char fname[MAX_PATH] = {0};
	line.SizeOfStruct = sizeof(line);
	line.FileName = fname;
	DWORD dwLineDisplacement;
	
	if (SymGetLineFromAddr( hProcess, (DWORD)dwAddress, &dwLineDisplacement, &line ) )
	{
		strcpy_s(szFileName, nFileNameMaxLength, line.FileName);  // note no _countof
		*pLineNumber = line.LineNumber;
	}
	return;
}

void WriteStack(FILE *streamLog, CTraceStackStep *rgLES, int nLESCount)
{
	char *szStackPrefix = "Call Stack With Symbols - Start (ModuleName Address OffsetFromBase SymbolName FileName LineNumber) \n";
	char *szStackSuffix = "Call Stack With Symbols - End\n";
	char *szNoSymbol="(no symbols)";
	char *szNoFilename="(no filename)";
	fprintf(streamLog, "%s%",szStackPrefix);

	for(int j=0;j<nLESCount;j++)
	{
		// todo: class method
		char *szSymbol = ( strlen(rgLES[j].m_szFunctionName)>0 ? rgLES[j].m_szFunctionName : szNoSymbol);
		char *szFilename = ( strlen(rgLES[j].m_szFileName)>0 ? rgLES[j].m_szFileName : szNoFilename);
		fprintf(streamLog, "%s 0x%08x ", rgLES[j].m_szModuleName, rgLES[j].m_dwAddress);
		fprintf(streamLog, "(0x%08x)  ", rgLES[j].m_dwOffset); 
		fprintf(streamLog, "%s ", szSymbol);
		fprintf(streamLog, "%s ", szFilename);
		fprintf(streamLog, "%d\n", rgLES[j].m_nLineNumber);
	}

	fprintf(streamLog, "%s%",szStackSuffix);
	return;
}

// resets 
bool StreamCopy(FILE *streamDestination, FILE *streamSource, fpos_t *pposStart)
{
	fpos_t posSourceCurrentPosition;
	fpos_t posSourceCursor;
	fgetpos(streamSource, &posSourceCurrentPosition); // todo: error
	fsetpos(streamSource, pposStart);
	fgetpos(streamSource, &posSourceCursor); // use local 
	char cSource=1; // initial value not used

	// must check if initial fgetc returns EOF
	while(posSourceCurrentPosition != posSourceCursor && cSource!=EOF)   //todo:  check for <= on streamoff 
	{
		cSource=(char)fgetc(streamSource);
		fputc(cSource, streamDestination);
		fgetpos(streamSource, &posSourceCursor); // todo: stream pointer math
	}
	// reset file position info
	fsetpos(streamSource, &posSourceCurrentPosition);
	fgetpos(streamSource, pposStart);
	return true;
}



bool LoadAllModules(CTraceModule *rgLEM, int nLEMCount, char **rgszModulePath, int nPathCount)
{
	for(int j=0;j<nLEMCount;j++)
	{
		rgLEM[j].m_hProcess=32*(j+1); // unique identifier only
		rgLEM[j].ModuleLoad(rgszModulePath, nPathCount);
	}
	return true; // todo
}

// returns -1 if module not found
int FindModule(char *szName, CTraceModule *rgLEM, int nLEMCount)
{
	bool bFound=false;
	int nIndex=0;

	do{
		if(0==strncmp(szName, rgLEM[nIndex].m_szName, ModuleNameMaxLength))
		{
			bFound=true;
		}
		else
		{
			nIndex++;
		}

		}while(!bFound && nIndex<nLEMCount);
	return bFound?nIndex : -1;
}
bool ProcessLog(char *szLogName, char *szNewLogName, char **rgszModulePath, int nPathCount)
{
	int nLEMCount=0;
	int nLESCount=0;
	CTraceModule *rgLEM=NULL;
	CTraceStackStep *rgLES=NULL;

	fpos_t posLogPosition;

	FILE *streamLog=NULL;
	FILE *streamNewLog=NULL;

	EINONZERO(fopen_s(&streamLog, szLogName,"r"));
	EINONZERO(fopen_s(&streamNewLog, szNewLogName, "w"));  // overwrite

	fseek(streamLog, SEEK_SET, 0);
	fgetpos(streamLog, &posLogPosition);

	ParseModuleList(streamLog, &rgLEM, &nLEMCount);
	LoadAllModules(rgLEM, nLEMCount, rgszModulePath, nPathCount);

	// todo: load modules once across all stack traces
	fseek(streamLog, SEEK_SET, 0);
	while(ParseStack(streamLog, &rgLES, &nLESCount))
	{
		for(int nStepIndex=0;nStepIndex<nLESCount;nStepIndex++)
		{
			int nModuleIndex=FindModule(rgLES[nStepIndex].m_szModuleName, rgLEM, nLEMCount);
			if(nModuleIndex>=0 && rgLEM[nModuleIndex].m_bModuleLoaded)
			{
				SymbolLookup((HANDLE)rgLEM[nModuleIndex].m_hProcess, 
					rgLES[nStepIndex].m_dwAddress, 
					rgLES[nStepIndex].m_szFunctionName,
					_countof(rgLES[nStepIndex].m_szFunctionName),
					rgLES[nStepIndex].m_szFileName,
					_countof(rgLES[nStepIndex].m_szFileName),
					&(rgLES[nStepIndex].m_nLineNumber)
					); 
			}
		}
		StreamCopy(streamNewLog, streamLog, &posLogPosition);
		WriteStack(streamNewLog, rgLES, nLESCount);
		if(NULL!=rgLES)
		{
			delete[] rgLES;
			rgLES=NULL;
		}
	}
	StreamCopy(streamNewLog, streamLog, &posLogPosition);  // tail portion of log
#include<stdlib.h>
	// ensure not leaked
	if(NULL!=rgLEM)
	{
		delete[] rgLEM;
		rgLEM=NULL;
	}
	if(NULL!=rgLES)
	{
		delete[] rgLES;
		rgLES=NULL;
	}
	fclose(streamLog);
	fclose(streamNewLog);
	return true;
}


int _tmain(int argc, _TCHAR* argv[])
{
	bool succeeded=false;

	if(!ValidateInput(argc, argv))
	{
		goto Exit;
	}
	char *szLogName = argv[1];
	char *szNewLogName = argv[2];
	char **rgszModulePath=argv+3;
	succeeded = ProcessLog(szLogName, szNewLogName, rgszModulePath, argc-3);
	if(succeeded)
	{
		printf("success\n");
	}
	else
	{
		printf("failed\n");// todo: diagnostics on fail?
	}
Exit:
	return 0;
}

