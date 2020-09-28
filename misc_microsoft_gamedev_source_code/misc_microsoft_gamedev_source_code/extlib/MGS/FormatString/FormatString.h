#pragma once
#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif
namespace MGS
{
	int FormatStringW( DWORD flags, LPWSTR pszBuffer, UINT uSize, LPCWSTR pszFormat, ... );
	int FormatStringSW( DWORD flags, LPWSTR pszBuffer, UINT uSize,LPWSTR pszBuffer2, UINT uSize2,LPCWSTR pszFormat, ... );
	int _FormatStringW( DWORD flags, LPWSTR pszBuffer, UINT uSize, LPWSTR pszBuffer2, UINT uSize2, LPCWSTR pszFormat,va_list *Arguments  );
	LPWSTR InsertOrdinalForFormatSpecification( LPCWSTR pszFormat, BOOL* pfInserted );
	LPWSTR InsertOrdinalForFormatSpecificationS( LPWSTR pszBuffer2, UINT uSize2,LPCWSTR pszFormat, BOOL* pfInserted );
	LPWSTR _InsertOrdinalForFormatSpecification( LPWSTR pszBuffer2, UINT uSize2,LPCWSTR pszFormat, BOOL* pfInserted );
};
