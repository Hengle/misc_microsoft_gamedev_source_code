// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>

#include <assert.h>
#define MaxLineLength 1024

// Exit If Fail
#define EIFAIL(x) {if(!(x)){printf("\nEIF FAILURE\n");__asm{int 3};exit(-1);}}
// Exit If NonZero
#define EINONZERO(x) {if(0!=(x)){printf("\nEINZ FAILURE\n");__asm{int 3};exit(-1);}}
// Exit If Zero
#define EIZERO(x) {if(0==(x)){printf("\nEIZ FAILURE\n");__asm{int 3};exit(-1);}}

const int MINBINSTRUCTUREVALUE = 2;

#if _MSC_VER <= 1310
#pragma message("Defining secureCRT")
#ifndef _SECURE_CRT_
#define _SECURE_CRT_
#define _countof(x) sizeof(x)/sizeof(*(x))
inline int fopen_s(FILE**ppf, const char * path, const char * opt)
{
	*ppf=fopen(path,opt);
	return NULL==(*ppf);
}
#define _strupr_s(x,y) _strupr(x)
#define sprintf_s _snprintf
#define sscanf_s sscanf
#define strcpy_s(dst,size,src) strcpy(dst,src)
#endif
#else
#pragma message("SecureCRT exists")
#endif