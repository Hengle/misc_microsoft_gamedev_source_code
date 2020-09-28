#include "formatstring.h"
#include "xformatmessage.h"
#define ALLOW_SPEC_WITH_NO_ORDINAL

#ifdef _XBOX
#define MyGetLastError() 0
#define MySetLastError(error)
#else
#define MyGetLastError ::GetLastError
#define MySetLastError ::SetLastError
#endif

static CHAR signature[] = "%%%FORMATSTRINGW:050908%%%";

#define MIN_PWSELECTORLEN 5   // minimum length of postpositional word selector
static struct PWSELECTOR
{
	// Selector format: "[<first form>/<second form>]"
	inline int Len() const { return nLen1 + nLen2 + 3; };	// length of selector
	inline LPCWSTR szPw1() const { return szSelector + 1; };	// pointer to the first form
	inline LPCWSTR szPw2() const { return szSelector + nLen1 + 2; }; // pointer to the second form
    LPCWSTR szSelector;	// selector
    int nLen1;		// length of first form
	int nLen2;		// length of second form
} sPostposition[] = 
{
	// UNICODE values of Korean postpositional word selector
// jaeyounk 4/4/05
// -Start--------------------------------------------------
    L"\x005B\xC774\x002F\x005D", 1, 0,				// [I/]
// -End----------------------------------------------------
    L"\x005B\xC740\x002F\xB294\x005D", 1, 1,		// [EUN/NEUN]
    L"\x005B\xC774\x002F\xAC00\x005D", 1, 1,		// [I/GA]
    L"\x005B\xC744\x002F\xB97C\x005D", 1, 1,		// [EUL/REUL]
    L"\x005B\xACFC\x002F\xC640\x005D", 1, 1,		// [GWA/WA]
    L"\x005B\xC544\x002F\xC57C\x005D", 1, 1,		// [A/YA]
    L"\x005B\xC73C\xB85C\x002F\xB85C\x005D", 2, 1,	// [EURO/RO]
};

static const PWSELECTOR* CheckSelector( LPCWSTR pszPtr )
{
    for( int i = 0; i < sizeof(sPostposition)/sizeof(sPostposition[0]); i++ )
	{
		if( !wcsncmp( sPostposition[i].szSelector, pszPtr, sPostposition[i].Len() ) )
		{
			return sPostposition + i;
		}
	}
	return NULL;
}

// jaeyounk 4/4/05
// -Start--------------------------------------------------
static WCHAR FindPrevious(LPWSTR pStr, int nPos)
{
	for(int n = nPos; n >= 0; n--) {
		if(pStr[n] >= 0xAC00 && pStr[n] <= 0xD7A3) { // UNICODE HANGUL
			return pStr[n];
		}
		else if((pStr[n] >= L'A' && pStr[n] <= L'Z') || (pStr[n] >= L'a' && pStr[n] <= L'z')) {
			return pStr[n];
		}
		else if(pStr[n] >= L'0' && pStr[n] <= L'9') {
			return pStr[n];
		}
		else if(pStr[n] == L'>' && n > 2) { // The min length of HTML close tag: 3
			int m;
			for(m = n-1; m >= 0; m--) {
// #313: jaeyounk 4/7/05 - add digits to tag name for xml case
// -Start--------------------------------------------------
//(old)			// if not Alphabet, then break
//				if(!(pStr[m] >= L'A' && pStr[m] <= L'Z') && !(pStr[m] >= L'a' && pStr[m] <= L'z')) {
				// if not XML tag character, then break
				if(!(pStr[m] >= L'A' && pStr[m] <= L'Z') && !(pStr[m] >= L'a' && pStr[m] <= L'z') 
						&& !(pStr[m] >= L'0' && pStr[m] <= L'9')
						&& !(pStr[m] == L'.' || pStr[m] == L'-' || pStr[m] == L'_' || pStr[m] == L':')) {
// -End----------------------------------------------------
					break;
				}
			}
			if(m > 0 && pStr[m] == L'/' && pStr[m-1] == L'<') {
				n = m;
			}
		}
// #312: jaeyounk 4/7/05 - add 'else if' for a space character
// -Start--------------------------------------------------
//(add)
		else if(pStr[n] == L' ') { // if meet a space, then stop searching
			break;
		}
// -End----------------------------------------------------
	}
	return pStr[nPos];
}
// -End----------------------------------------------------

//
//  int MGS::FormatStringW( DWORD dwFlags, LPWSTR pszBuffer, UINT uSize, LPCWSTR pszFormat, ... );
//      This function formats a string. 
//      pszFormat may contain the inserts for FormatMessage Win32 API function. (e.g. "%1!s!")
//      It may also contain special sequence (selector) for Korean postpositional words.
//      See PWSELECTOR structure in this file for the Korean postpostional word selectors.
//
//  Parameters:
//      dwFlags
//          [in] The following flags are supported.
//          They're passed to FormatMessage Win32 API function internally.
//          Refer to Win32 SDK document for the detail.
//              FORMAT_MESSAGE_ALLOCATE_BUFFER
//              FORMAT_MESSAGE_IGNORE_INSERTS
//      pszBuffer
//          [out] The output string. If uSize is zero, this is not used.
//          If FORMAT_MESSAGE_ALLOCATE_BUFFER is specified in flags, 
//          it's a pointer to pointer to Unicode string (i.e. LPWSTR*).
//          The returned LPWSTR must be freed with LocalFree() Win32 API function.
//      uSize
//          [in] The length of pszBuffer. If this is zero, 
//          the function returns the number of characters required to hold formatted string.
//      pszFormat
//          [in] The format string. The format string can contain the inserts described in 
//          FormatMessage Win32 API function, and Korean postpostional word selectors.
//      c
//          [in] Specifies one or more optional arguments.
//
//  Return Value:
//      If uSize is zero and FORMAT_MESSAGE_ALLOCATE_BUFFER isn't specified,
//      the function returns the number of characters required to hold the formatted string.
//      If uSize is non-zero or FORMAT_MESSAGE_ALLOCATE_BUFFER is specified,
//      the function returns the number of characters stored in the output buffer
//      including the terminating null character.
//      If the function fails, it returns zero.
//
//	Remarks:
//      The following shows the valid combinations of the arguments.
//          Usage                        flag                pszBuffer      uSize
//          ---------------------------+-------------------+--------------+-----------
//          Query length               | !alloc_buffer     | (not used)   | zero
//          Format string w/o alloc    | !alloc_buffer     | non null     | non zero
//          Format string w/ alloc     | alloc_buffer      | non null     | (not used)
//
int MGS::FormatStringW( DWORD dwFlags, LPWSTR pszBuffer, UINT uSize, LPCWSTR pszFormat, ... )
{
	va_list pArg;
	va_list ap;
	va_start(ap, pszFormat);
	if ( dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY )
	{
		pArg = (va_list)va_arg( ap, DWORD* );
	}
	else
	{
		pArg = (va_list)&ap;
	}
	return _FormatStringW(dwFlags,pszBuffer,uSize,NULL,0,pszFormat,(va_list*)pArg);
	va_end(ap);
}

int MGS::FormatStringSW( DWORD dwFlags, LPWSTR pszBuffer, UINT uSize, LPWSTR pszBuffer2, UINT uSize2, LPCWSTR pszFormat, ... )
{
	va_list pArg;
	va_list ap;
	va_start(ap, pszFormat);
	if ( dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY )
	{
		pArg = (va_list)va_arg( ap, DWORD* );
	}
	else
	{
		pArg = (va_list)&ap;
	}
	return _FormatStringW(dwFlags,pszBuffer,uSize,pszBuffer2,uSize2,pszFormat,(va_list*)pArg);
	va_end(ap);
}

int MGS::_FormatStringW( DWORD dwFlags, LPWSTR pszBuffer, UINT uSize, LPWSTR pszBuffer2, UINT uSize2, LPCWSTR pszFormat, va_list *pArg )
{
	bool bfAlloc, bfIgnore;
	bfAlloc = ( dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER ) ? true : false;
	// check error cases
	if( pszFormat == NULL || 
		// pszBuffer can be NULL only when querying length
		( pszBuffer == NULL && !( !bfAlloc && uSize == 0 ) ) 
	  )
	{
		MySetLastError( ERROR_INVALID_PARAMETER );
		return 0;
	}

	// ignore unsupported flags
	dwFlags &= FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS
		| FORMAT_MESSAGE_ARGUMENT_ARRAY;

	LPWSTR pszWork;
	int iRc;
	DWORD dwError = ERROR_SUCCESS;
#ifdef ALLOW_SPEC_WITH_NO_ORDINAL
	BOOL fOrdinalInserted = FALSE;
	if(pszBuffer2)
	{
		pszFormat = InsertOrdinalForFormatSpecificationS( pszBuffer2,uSize2,pszFormat, &fOrdinalInserted );
	} else
	{
		pszFormat = InsertOrdinalForFormatSpecification( pszFormat, &fOrdinalInserted );
	}
	if ( !pszFormat )
	{
		MySetLastError( ERROR_INVALID_PARAMETER );
		return 0;
	}
#endif

#if 1 // optimization for the case where no Korean selector is in the format string
	// check if the format string contains selector
	pszWork = (LPWSTR)pszFormat;
	while( pszWork )
	{
		pszWork = wcschr( pszWork, L'[' );
		if( pszWork )
		{
			if( CheckSelector( pszWork ) )
				break;
			else
				pszWork++;
		}
	}

	if( !pszWork )
	{
		// no selector is found
		LPWSTR tmpBuffer = NULL;
		bool bQuerySize = !bfAlloc && ( uSize == 0 );
		if( bQuerySize )	// in case of querying size of formatted string
			dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
		iRc = XFormatMessageW( dwFlags | FORMAT_MESSAGE_FROM_STRING, pszFormat, 0, 0,
			(!bQuerySize ? pszBuffer : (LPWSTR)&tmpBuffer), uSize, (va_list*)pArg );
		dwError = MyGetLastError();
		if( tmpBuffer )
			LocalFree( tmpBuffer );
		MySetLastError( dwError );
#ifdef ALLOW_SPEC_WITH_NO_ORDINAL
		if ( fOrdinalInserted && !pszBuffer2)
		{
			LocalFree( (HLOCAL)pszFormat );
		}
#endif
		return iRc;
	}
#endif
	// selector is found
	LPWSTR sBuffer = NULL; // The string with inserts resolved
	LPWSTR sBuffer2 = NULL;	// The string with inserts unmodified
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING;
	if(dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
		flags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
	int nFormattedLength = XFormatMessageW( flags, pszFormat, 0, 0, (LPWSTR)&sBuffer, 1, (va_list*)pArg );
	if( nFormattedLength == 0 )
	{
		dwError = MyGetLastError();
		goto errorExit;
	}

	bfIgnore = ( dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS ) ? true : false;
	if( bfIgnore )
	{
		iRc = XFormatMessageW( flags | FORMAT_MESSAGE_IGNORE_INSERTS,
			pszFormat, 0, 0, (LPWSTR)&sBuffer2, 1, (va_list*)pArg );
		dwError = MyGetLastError();
		if( iRc == 0 )
		{
			goto errorExit;
		}
	}

	LPWSTR pszResult = bfIgnore ? sBuffer2 : sBuffer;
	pszWork = pszResult;

	for(int n = 1; n <= nFormattedLength-MIN_PWSELECTORLEN; n++) {
		// find selector in sBuffer
		if( sBuffer[n] != L'[' )
			continue;
		const PWSELECTOR* pSelector = CheckSelector( sBuffer + n );
		if( !pSelector )
			continue;
		// the detault is to use the first postposition
		BOOL bIsFirst = TRUE;

// jaeyounk 4/4/05
// -Start--------------------------------------------------
//(old)	WCHAR wcPrev = sBuffer[n-1];
		WCHAR wcPrev = FindPrevious(sBuffer, n-1);
// -End----------------------------------------------------

		if(wcPrev >= 0xAC00 && wcPrev <= 0xD7A3) { // UNICODE HANGUL
			if(((wcPrev-0xAC00) % 28) == 0) { // no last consonant
				// use the second postposition
				bIsFirst = FALSE;
			}
		}
		else { // not HANGUL
			switch(wcPrev) {
				case L'l': case L'm': case L'n': case L'r': 
				case L'L': case L'M': case L'N': case L'R':
				case L'1': case L'3': case L'6': case L'7':
// jaeyounk 4/4/05
// -Start--------------------------------------------------
//(old)			case L'8': case L'0': case L'#': case L'~':
				case L'8': case L'0':
// -End----------------------------------------------------
					bIsFirst = TRUE;
					break;
				default:
					bIsFirst = FALSE;
					break;
			}
		}

		// replace the selector with appropriate character
		LPCWSTR pszResultEnd = pszResult + wcslen(pszResult) - MIN_PWSELECTORLEN;
		for(; pszWork <= pszResultEnd; pszWork++) {
			int nLen = pSelector->Len();
			if(!wcsncmp(pSelector->szSelector, pszWork, nLen)) {
				int iFormLen;
				LPCWSTR pszPw;
				if(bIsFirst) {
					pszPw = pSelector->szPw1();
					iFormLen = pSelector->nLen1;
				}
				else {
					pszPw = pSelector->szPw2();
					iFormLen = pSelector->nLen2;
				}
				wcsncpy(pszWork, pszPw, iFormLen);
				wcscpy(pszWork + iFormLen, pszWork + nLen);
				if( pszResult == sBuffer )
					nFormattedLength -= pSelector->Len() - iFormLen;
				break;
			}
		}
	}

	iRc = lstrlenW( pszResult ) + 1;	// +1 for null terminator

	if( !bfAlloc && uSize && (int)uSize < iRc )
	{
		dwError = ERROR_INSUFFICIENT_BUFFER;
		goto errorExit;
	}

	bool bFreeMem1 = true;
	bool bFreeMem2 = true;
	if( !bfAlloc )
	{
		if( uSize )
		{
			lstrcpynW( pszBuffer, pszResult, uSize );
		}
		// else it's query of the string length
	}
	else
	{
		if( bfIgnore )
		{
			*(LPWSTR*)pszBuffer = sBuffer2;
			bFreeMem2 = false;
		}
		else
		{
			*(LPWSTR*)pszBuffer = sBuffer;
			bFreeMem1 = false;
		}
	}
	if( bFreeMem1 )
		LocalFree( sBuffer );
	if( bFreeMem2 )
		LocalFree( sBuffer2 );

#ifdef ALLOW_SPEC_WITH_NO_ORDINAL
	if ( fOrdinalInserted && !pszBuffer2)
	{
		LocalFree( (HLOCAL)pszFormat );
	}
#endif
	return iRc;

errorExit:
	if( sBuffer )
		LocalFree( sBuffer );
	if( sBuffer2 )
		LocalFree( sBuffer2 );
	MySetLastError( dwError );
#ifdef ALLOW_SPEC_WITH_NO_ORDINAL
	if ( fOrdinalInserted && !pszBuffer2)
	{
		LocalFree( (HLOCAL)pszFormat );
	}
#endif
	return 0;
}

#define ORDINAL_INSERTION_SIZE 5	// maximum size of oridinal insertion
#define ORDINAL_INSERTION_REALLOC (3*9+4) // extra memory for first 10 insertions of ordinals
// has to be equal or larger than ORDINAL_INSERTION_SIZE

LPWSTR MGS::InsertOrdinalForFormatSpecification( LPCWSTR pszFormat, BOOL* pfInserted )
{
	return _InsertOrdinalForFormatSpecification(NULL,0,pszFormat,pfInserted);
}

LPWSTR MGS::InsertOrdinalForFormatSpecificationS( LPWSTR pszBuffer2, UINT uSize2,LPCWSTR pszFormat, BOOL* pfInserted )
{
	return _InsertOrdinalForFormatSpecification(pszBuffer2,uSize2,pszFormat,pfInserted);
}

LPWSTR MGS::_InsertOrdinalForFormatSpecification( LPWSTR pszBuffer2, UINT uSize2,LPCWSTR pszFormat, BOOL* pfInserted )
{
	UINT uOrdinal = 0;
	LPWSTR pszOutput = NULL;
	UINT uOutputSize = 0;
	UINT uWritten = 0;
	LPCWSTR pszToBeWritten = pszFormat;
	LPCWSTR pszWork = pszFormat;
	LPCWSTR pszFormatEnd;

	*pfInserted = FALSE;
	do
	{
		pszWork = wcschr( pszWork, L'%' );
		if ( pszWork )
		{
			pszWork++;
			// ignore FormatMessage specific escape sequences which don't need ordinal
			switch ( *pszWork )
			{
				case L'0':
					// check if it's printf width field
					if ( L'1' <= pszWork[1] && pszWork[1] <= L'9' || pszWork[1] == L'.' )
						break;
				case L'%':
				case L'r':
				case L'n':
				case L't':
				case L'b':
					pszWork++;
					continue;
			}
			LPCWSTR szTypeChars = L"cCdiouxXeEfgGnpsS";
			size_t index = 0;
			// if numbers follow, it can be either ordinal for FormatMessage, or printf width field
			bool bNumber = false;
			while ( L'0' <= pszWork[index] && pszWork[index]<= L'9' )
			{
				bNumber = true;
				index++;
			}
			if ( pszWork[index] == 0 )
			{
				if ( bNumber )
					break;	// it's FormatMessage ordinal
				goto error;
			}
			bool bPrintf = false;
			bool bCheckPrintfTypeChar = false;
			if ( pszWork[index] == L'.' )
			{
				if ( pszWork[index+1] < L'0' || L'9' < pszWork[index] )
				{
					if ( !bNumber )
						continue;	// it's single dot
					else
						goto error;
				}
				// it must be printf precision field
				bCheckPrintfTypeChar = true;
			}
			else switch ( pszWork[index] )
			{
			case L'h':
			case L'l':
			case L'I':
			case L' ':
			case L'+':
			case L'-':
			case L'#':
				// it's either flags or size prefixes of printf format spec
				bCheckPrintfTypeChar = true;
				break;
			}
			if ( bCheckPrintfTypeChar )
			{
				// there must be printf type field char
				index = wcscspn( pszWork, szTypeChars );
				if ( pszWork[index] == 0 )
				{
					goto error;
				}
				bPrintf = true;
			}
			else
			{
				// is it printf type field char?
				LPCWSTR psz = szTypeChars;
				while ( *psz )
				{
					if ( pszWork[index] == *psz++ )
					{
						bPrintf = true;
						break;
					}
				}
			}
			if ( bPrintf )
			{
				// It's printf specification
				uOrdinal++;
				if ( uOrdinal > 999 )
				{
					goto error;
				}
				// memory (re)allocation
				if ( !pszOutput && pszBuffer2==NULL)
				{
					uOutputSize = lstrlenW( pszFormat ) + 1 + ORDINAL_INSERTION_REALLOC;
					pszOutput = (LPWSTR)LocalAlloc( LPTR, uOutputSize * sizeof(WCHAR) );
					pszFormatEnd = pszFormat + lstrlenW( pszFormat );
					*pfInserted = TRUE;
				}
				else if(pszBuffer2 &&  uOutputSize - uWritten < 
					(UINT)( pszFormatEnd - pszToBeWritten + ORDINAL_INSERTION_SIZE + 1 ) )
				{
					UINT uOrgSize = uOutputSize;
					pszFormatEnd = pszFormat + lstrlenW( pszFormat );
					uOutputSize += (UINT)( pszFormatEnd - pszToBeWritten + ORDINAL_INSERTION_REALLOC + 1 );
					if(uOutputSize >  uSize2)
					{
						goto error;
					}
				}
				else if ( uOutputSize - uWritten < 
					(UINT)( pszFormatEnd - pszToBeWritten + ORDINAL_INSERTION_SIZE + 1 ) )
				{
					UINT uOrgSize = uOutputSize;
					uOutputSize += (UINT)( pszFormatEnd - pszToBeWritten + ORDINAL_INSERTION_REALLOC + 1 );
					LPWSTR psz = NULL;
					psz = (LPWSTR)LocalReAlloc( pszOutput, uOutputSize * sizeof(WCHAR), 0 );
					if(!psz)
					{
						psz = (LPWSTR)LocalAlloc( LPTR, uOutputSize * sizeof(WCHAR));
						if ( !psz )
						{
							goto error;
						}
						memcpy(psz,pszOutput,uOrgSize * sizeof(WCHAR));
						LocalFree((HLOCAL)pszOutput);
					}
					pszOutput = psz;
				}
				// write to output buffer with ordinal
				if(pszBuffer2)
				{
					memcpy( pszBuffer2 + uWritten, pszToBeWritten, ( pszWork - pszToBeWritten ) * sizeof(WCHAR) );
					uWritten += (UINT)( pszWork - pszToBeWritten );
					uWritten += wsprintfW( pszBuffer2+ uWritten, L"%d!", uOrdinal );
					memcpy( pszBuffer2 + uWritten, pszWork, ( index + 1 ) * sizeof(WCHAR) );
					uWritten += (UINT)( index + 1 );
					pszWork += index + 1;
					pszToBeWritten = pszWork;
					pszBuffer2[uWritten++] = L'!';
				} else
				{
					memcpy( pszOutput + uWritten, pszToBeWritten, ( pszWork - pszToBeWritten ) * sizeof(WCHAR) );
					uWritten += (UINT)( pszWork - pszToBeWritten );
					uWritten += wsprintfW( pszOutput + uWritten, L"%d!", uOrdinal );
					memcpy( pszOutput + uWritten, pszWork, ( index + 1 ) * sizeof(WCHAR) );
					uWritten += (UINT)( index + 1 );
					pszWork += index + 1;
					pszToBeWritten = pszWork;
					pszOutput[uWritten++] = L'!';
				}
			}
			else 
			{
				pszWork += index;
				// is it FormatMessage specification?
				if ( *pszWork == L'!' )
				{
					// specifier with type
					pszWork = wcschr( pszWork + 1, L'!' );
					if ( !pszWork )
					{
						goto error;
					}
					pszWork++;
				}
				else if ( !bNumber )
				{
					// it's single escape character
					pszWork++;
				}

			}
		}
	} while ( pszWork );

	if ( !*pfInserted && !pszBuffer2)
	{
			return (LPWSTR)pszFormat;
    }
	if(pszBuffer2)
	{
		lstrcpyW( pszBuffer2 + uWritten, pszToBeWritten );
		return pszBuffer2;
	} else
	{
		lstrcpyW( pszOutput + uWritten, pszToBeWritten );
		return pszOutput;
	}

error:
	if ( pszOutput && !pszBuffer2)
		LocalFree( pszOutput );
	*pfInserted = FALSE;
	return NULL;
}
