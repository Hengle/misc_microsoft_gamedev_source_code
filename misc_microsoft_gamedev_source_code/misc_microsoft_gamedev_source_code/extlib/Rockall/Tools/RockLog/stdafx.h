// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#include <iostream>
#include <tchar.h>
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>
//#include "xbdm.h"

// TODO: reference additional headers your program requires here

const int ModuleNameMaxLength=64;
const int FunctionNameMaxLength=1024;

#define EIF(x) {if(!(x)) goto Exit;}
#define MIN(x,y) (int)( (__int64)(x)>(__int64)(y)?(__int64)(y):(__int64)(x) )
typedef unsigned __int64 DWORD64, *PDWORD64;
// typedef unsigned __int64 HANDLE_PTR;
typedef void *HANDLE;
#define FALSE 0
#define TRUE 1

const int MaxStackSize=20;
const int MaxModuleCount=20;
const int MaxLineLength=1024;

// Exit If False
#define EIFALSE(x) {if(!(x)){__asm{int 3};exit(-1);}}
// Exit If NonZero
#define EINONZERO(x) {if(0!=(x)){__asm{int 3};exit(-1);}}
// Exit If Zero
#define EIZERO(x) {if(0==(x)){__asm{int 3};exit(-1);}}



#if _MSC_VER <= 1310
#pragma message("Defining secureCRT")
#ifndef _SECURE_CRT_
#define _SECURE_CRT_
#if !defined(_countof)
#if !defined(__cplusplus)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else
extern "C++"
{
	template <typename _CountofType, size_t _SizeOfArray>
	char (*__countof_helper(UNALIGNED _CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
#define _countof(_Array) sizeof(*__countof_helper(_Array))
}
#endif
#endif
inline int fopen_s(FILE**ppf, const char * path, const char * opt)
{
	*ppf=fopen(path,opt);
	return NULL==(*ppf);
}
#pragma warning(push)
#pragma warning(disable:4100)
// explicitly terminated in vc7 case
inline int strcpy_s(char *szDestination,int nSize, const char *const szSource)
{
	int toReturn=(int)strcpy(szDestination, szSource);
	szDestination[nSize-1]=NULL;
	return toReturn;
}
#define sscanf_s sscanf
inline int sscanf3s_s(char *szBuffer,char *szFormat, char *sz1, int n1, char *sz2, int n2, char *sz3, int n3)
{
	return sscanf(szBuffer, szFormat, sz1, sz2, sz3);
}
#pragma warning(default:4100) 
#pragma warning(pop) 

#define _strupr_s(x,y) _strupr(x)
// explicitly terminated in vc7 case
#define strncpy_s(dst,size,src,count) strncpy(dst,src,count);dst[MIN(count,size-1)]=NULL;
#endif
#else
#pragma message("SecureCRT exists")
// non variable parameter list sscanf for vc71 and vc8
#define sscanf3s_s sscanf_s
#endif