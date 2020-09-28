//============================================================================
//
//  LineBreak.h
//
//  This file was acquired from the East Asian Loc team and used "as is".
//  Contact Yutaka Hasegawa (yutakah@microsoft.com) for questions.
//
//  MODIFICATIONS:
//  - Added a user parameter to the GetWidth() callback.
//  - added isAtBOL parameter to the line break function.
//  - added keepTrailingWS parameter to the line break function.
// 
//============================================================================


#include "xsystem.h"
#include "linebreak.h"

#define COUNTOF(a) ( sizeof( a ) / sizeof( (a)[0] ) )


CB_GetWidthW GetWidthW = NULL;
CB_Reserved  Reserved  = NULL;
void*        GetWidthParam = NULL;

void LineBreak_SetCallback( CB_GetWidthW pGetWidthW, void* pParam, CB_Reserved pReserved )
{
	if ( pGetWidthW )
   {
		GetWidthW = pGetWidthW;
	}
	if ( pReserved )
   {
		Reserved = pReserved;
	}
   GetWidthParam = pParam;
}


UINT g_uCP = 1252;
UINT g_uOption = 0;
BOOL g_bIsNT = FALSE;
WCHAR g_szUniErrChar[2] = L"";
CHAR g_szMBErrChar[3] = "";

void LineBreak_SetOption( UINT uCP, UINT uOption )
{
	g_uCP = uCP;
	g_uOption = uOption;

#ifndef XBOX
	// Check NT
	OSVERSIONINFO verinfo;
	verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &verinfo );
	g_bIsNT = ((verinfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) ? TRUE : FALSE);
//	OutputDebugString( TEXT("'g_bIsNT' initialized\r\n") ); // DEBUG

	// Check Unicode Error Char
	CPINFOEX cpinfo;
	if ( GetCPInfoEx( g_uCP, 0, &cpinfo ) == 0 ){ // FAIL ?
		cpinfo.UnicodeDefaultChar = 0;
	}
	BOOL bError;
	g_szUniErrChar[0] = cpinfo.UnicodeDefaultChar;
	g_szUniErrChar[1] = 0;
	g_szMBErrChar[0] = g_szMBErrChar[1] = g_szMBErrChar[2] = 0;
	WideCharToMultiByte( g_uCP, 0, g_szUniErrChar, -1, g_szMBErrChar, sizeof(g_szMBErrChar), NULL, &bError );
//	TCHAR buf[128]; // DEBUG
//	wsprintf( buf, TEXT("g_szUniErrChar=%04X\r\n"), cpinfo.UnicodeDefaultChar ); // DEBUG
//	OutputDebugString( buf ); // DEBUG
//	OutputDebugString( TEXT("g_szMBErrChar=[") ); // DEBUG
//	OutputDebugStringA( g_szMBErrChar ); // DEBUG
//	OutputDebugString( TEXT("]\r\n") ); // DEBUG

#endif
}


#define IsWhiteSpace(c) ( ( c ) == L'\t' || ( c ) == L'\r' || ( c ) == L' ' )
#define IsLineFeed(c) ( ( c ) == L'\n' )

bool LineBreak_IsNonBeginningChar( WCHAR c )
{
	if ( ! (g_uOption & LB_KINSOKU) ) return FALSE;
	
	return (
	( c ) == 0x0021 ||		// Exclamation mark
	( c ) == 0x0029 ||		// Right parenthesis
	( c ) == 0x002c ||		// Comma
	( c ) == 0x002e ||		// Full stop (period)
	( c ) == 0x003f ||		// Question mark
	( c ) == 0x3001 ||		// Ideographic comma
	( c ) == 0x3002 ||		// Ideographic full stop
	( c ) == 0x30fc ||		// Katakana-hiragana prolonged sound mark
	( c ) == 0xff01 ||		// Fullwidth exclamation mark
	( c ) == 0xff09 ||		// Fullwidth right parenthesis
	( c ) == 0xff1f ||		// Fullwidth question mark
	( c ) == 0xff70 ||		// Halfwidth Katakana-hiragana prolonged sound mark
	( c ) == 0xff9e ||		// Halfwidth Katakana voiced sound mark
	( c ) == 0xff9f );		// Halfwidth Katakana semi-voiced sound mark
}

static bool IsNonEndingChar( WCHAR c )
{
	if ( ! (g_uOption & LB_KINSOKU) ) return FALSE;
	
	return (
	( c ) == 0x0028 ||		// Left parenthesis
	( c ) == 0xff08 );		// Fullwidth left parenthesis
}

static bool IsEastAsianChar( WCHAR c )
{
   // MS 7/18/2005: 11069
   if (
      ( 0x1100 <= c ) && ( c <= 0x11FF ) ||        // Hangul Jamo
      ( 0x3130 <= c ) && ( c <= 0x318F ) ||        // Hangul Compatibility Jamo
      ( 0xAC00 <= c ) && ( c <= 0xD7A3 )           // Hangul Syllables
      ) return FALSE;

	return
		( 0x1100 <= c ) && ( c <= 0x11FF ) ||		// Hangul Jamo
		( 0x3000 <= c ) && ( c <= 0xD7AF ) ||		// CJK symbols - Hangul Syllables
		( 0xF900 <= c ) && ( c <= 0xFAFF ) ||		// CJK compat
		( 0xFF00 <= c ) && ( c <= 0xFFDC );			// Halfwidth / Fullwidth
}

#if 1 // 020513 ///////////////////////////////////////////////////////////////
static bool CanBreakLineAt( LPCWSTR psz, LPCWSTR pszStart )
{
	if ( psz == pszStart )
	{
		return false;	// leave at least one character in a line
	}
	return
		( IsWhiteSpace( *psz ) || IsEastAsianChar( *psz ) || IsEastAsianChar( psz[-1] ) ) &&
		!LineBreak_IsNonBeginningChar( *psz ) && !IsNonEndingChar( psz[-1] );
}
#endif

static LPCWSTR FindNonWhiteSpaceForward( LPCWSTR psz )
{
	while ( IsWhiteSpace( *psz ) )
	{
		psz++;
	}
#ifndef NO_RAID_151_152 // May.21,2002 06:04 by yutaka.
	if ( psz && IsLineFeed(*psz) ) psz++;
#endif
	return ( *psz ) ? psz : NULL;
}

static LPCWSTR FindNonWhiteSpaceBackward( LPCWSTR psz, LPCWSTR pszSource )
{
	while ( psz >= pszSource && ( IsWhiteSpace( *psz ) || IsLineFeed( *psz ) ) )
	{
		psz--;
	}
	return ( psz < pszSource ) ? NULL : psz;
}

//
// FindNextLine
// param:	pszSource: (in) the line to break
//			uWidth: (in) width to wrap the line
//			ppszEOL: (out) receive the pointer to the last non-white space char in the line. 
// return:	The beginning of next line. NULL if the line fits within the specified width.
//			*ppszEOL will be NULL if the wrapped line consists of only white space chars, 
//
// Case / Expected
// - Line width less than a character width / The line will contain at least one character
// - All white spaces ||
// - All unbreakable character ||
// - All non-starting characters ||
// - All non-terminating characters / Break at the character that overflows the width
// - Zero length line / return NULL w/ EOL NULL
//
LPCWSTR LineBreak_FindNextLineW( LPCWSTR pszSource, UINT uWidth, LPCWSTR* ppszEOL , bool isAtBOL, bool keepTrailingWS )
{
   if (ppszEOL == NULL)
      return NULL;

	if ( GetWidthW == NULL || pszSource == NULL || !pszSource[0] )
	{
		*ppszEOL = NULL;
		return NULL;
	}

	LPCWSTR psz = pszSource;
	UINT uCurrent;

	for ( uCurrent = 0; *psz && !IsLineFeed( *psz ); psz++ )
	{
		uCurrent += GetWidthW( *psz , GetWidthParam);
		if ( uCurrent > uWidth )
			break;
	}
	if ( pszSource == psz )
	{
      if (IsLineFeed(*psz))
      {
         *ppszEOL = NULL;
         return (psz + 1);
      }

      if (isAtBOL)
      {
         // Break at second character if line width is less than character width
		   *ppszEOL = FindNonWhiteSpaceBackward( psz, pszSource );
//   		OutputDebugString( TEXT("[1]\r\n") ); // DEBUG
		   return FindNonWhiteSpaceForward( psz + 1 );
      }
      else
      {
         *ppszEOL = NULL;
		   return FindNonWhiteSpaceForward( psz );
      }
	}
	if ( IsLineFeed( *psz ) )
	{
		psz++;
	}
	if ( uCurrent <= uWidth )
	{
		// The line is shorter than the width
		if (!keepTrailingWS)
         *ppszEOL = FindNonWhiteSpaceBackward( psz - 1, pszSource );
      else
         *ppszEOL = psz - 1;
//		OutputDebugString( TEXT("[2]\r\n") ); // DEBUG
#ifndef NO_RAID_151_152 // May.21,2002 06:04 by yutaka.
		if ( psz-1 >= pszSource && IsLineFeed( *(psz-1) ) ){
			return psz;
		}
#endif
		return ( *psz ) ? FindNonWhiteSpaceForward( psz ) : NULL;
	}

	LPCWSTR pszOverflow = psz;

#if 1 // 020513 ///////////////////////////////////////////////////////////////
	while ( psz > pszSource )
	{
		if ( IsWhiteSpace( *psz ) )
		{
			*ppszEOL = FindNonWhiteSpaceBackward( psz, pszSource );
			if ( !*ppszEOL )
			{
//				OutputDebugString( TEXT("[3]\r\n") ); // DEBUG
				return FindNonWhiteSpaceForward( psz + 1 );
			}
			psz = *ppszEOL + 1;
		}
		if ( CanBreakLineAt( psz, pszSource ) )
		{
			break;
		}
		psz--;
	}
	if ( psz <= pszSource )
	{
      if (isAtBOL)
      {
		   // couldn't find any character to break the line
		   *ppszEOL = pszOverflow - 1;
//   		OutputDebugString( TEXT("[4]\r\n") ); // DEBUG
		   return pszOverflow;
      }
      else
      {
		   *ppszEOL = NULL;
		   return FindNonWhiteSpaceForward(pszSource);
      }
	}
	*ppszEOL = psz - 1;
//	OutputDebugString( TEXT("[5]\r\n") ); // DEBUG
	return FindNonWhiteSpaceForward( psz );
#else // 020513 ///////////////////////////////////////////////////////////////
	while ( psz > pszSource )
	{
		if ( IsWhiteSpace( *psz ) || IsEastAsianChar( *psz ) || IsEastAsianChar( psz[-1] ) )
		{
			LPCWSTR pszNextLine = FindNonWhiteSpaceForward( psz );
			psz = FindNonWhiteSpaceBackward( psz - 1, pszSource );
			if ( ( !psz || !IsNonEndingChar( *psz ) ) && ( !pszNextLine || !LineBreak_IsNonBeginningChar( *pszNextLine ) ) )
			{
//				OutputDebugString( TEXT("[3']\r\n") ); // DEBUG
				*ppszEOL = psz;
				return pszNextLine;
			}
		}
		else
		{
			psz--;
		}
	}
	// couldn't find any character to break the line
	*ppszEOL = pszOverflow - 1;
//	OutputDebugString( TEXT("[4']\r\n") ); // DEBUG
	return pszOverflow;
#endif // 020513 //////////////////////////////////////////////////////////////
}
/*
LPCSTR LineBreak_FindNextLineA( LPCSTR pszSource, UINT uWidth, LPCSTR* ppszEOL )
{
	if ( GetWidthW == NULL || pszSource == NULL || !pszSource[0] )
	{
		*ppszEOL = NULL;
		return NULL;
	}
	
#ifdef TEST_LBBUF10
	static WCHAR szTemp[10]; // just for testing use
#else
	// As long as 1 line doesn't exceed following buffer size (=char num), 
	// string is processed correctly.
	static WCHAR szTemp[1024*8];
#endif
#ifndef NO_RAID_156
	BOOL bIsInsufficientBuffer = FALSE;
#endif
	
#ifndef NO_RAID_156
	int ret = MultiByteToWideChar( g_uCP, 0, pszSource, -1, szTemp, COUNTOF(szTemp) );
	if ( ret == 0 ){
		if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ){
			ret = COUNTOF(szTemp)-1;
			bIsInsufficientBuffer = TRUE;
		}
	}
#else
	int ret = MultiByteToWideChar( g_uCP, 0, pszSource, -1, szTemp, COUNTOF(szTemp)-1 );
#endif
//	int ret = MultiByteToWideChar( g_uCP, MB_USEGLYPHCHARS, pszSource, -1, szTemp, COUNTOF(szTemp)-1 ); // no effect for '?'(0x3F)
//	int ret = MultiByteToWideChar( g_uCP, MB_ERR_INVALID_CHARS, pszSource, -1, szTemp, COUNTOF(szTemp)-1 ); // err if error char found
	if ( ret == 0 ){
		*ppszEOL = NULL;
		return NULL;
	}
#ifndef NO_RAID_156
	szTemp[COUNTOF(szTemp)-1] = 0;
#else
	szTemp[ret] = 0;
#endif
	
	if ( !(g_uOption & LB_NOMBCSSP) && (!g_bIsNT) && g_szUniErrChar[0] ){ // 2002.05.28.0001
		
		LPCSTR pszTemp = (LPCSTR)pszSource;
		for ( int i=0; szTemp[i]; i++ ){
			if ( szTemp[i] == g_szUniErrChar[0] ){
				if ( pszTemp[0]==g_szMBErrChar[0] && 
					(g_szMBErrChar[1]==0 || (pszTemp[1]==g_szMBErrChar[1] && g_szMBErrChar[2]==0)) ){
					// do nothing.
				} else {
					// Conversion Error Char
					
					/*
					Win9x's TextOut() shows Error Char as a dot as it's 
					defined in SJIS font, and it's not necessarily as same as 
					Unicode Error Character = U+30FB(SJIS=0x8145).
					
					Further, these width are different.
					On Win9x, U+30FB is narrower than the dot which is 
					defined in SJIS font.
					
					I'm adjusting it to replacing the character internally 
					to fit the dot, by setting wider (and closer) character 
					here.
					*
					
					if ( g_uCP == 1252 ){
						szTemp[i] = 0x003F; // L'W'; // replace with the wider char for USA
					} else {
						szTemp[i] = 0x4E9E; // L'˜±'; // replace with the wider char for EA
						// Unicode		U+4E9E
						// JPN(SJIS)	0x98B1
						// CHT(BIG5)	0xE4AC
						// CHS(BG2312)	0xA8C8
					}
				}
			}
			pszTemp = CharNextExA( g_uCP, pszTemp, 0 );
		}
	}
	
#if 0 // DEBUG
	{
		OutputDebugString( TEXT(">>>") );
		for ( int i=0; szTemp[i] ; i++ ){
			TCHAR buf[128];
			wsprintf( buf, TEXT("[%04X]"), szTemp[i] );
			OutputDebugString( buf );
		}
		OutputDebugString( TEXT("\r\n") );
	}
#endif // DEBUG
	
	LPCWSTR pszEOL = NULL;
	LPCWSTR pszNext = LineBreak_FindNextLineW( szTemp, uWidth, &pszEOL );
	
	LPCSTR pszRet = NULL;
#ifndef NO_RAID_156
	if ( !pszNext && bIsInsufficientBuffer ){
		pszNext = szTemp + COUNTOF(szTemp)-1;
	}
#endif
	if ( pszNext ){
		int n = pszNext - szTemp;
		pszRet = pszSource;
		while ( n-- ){
			pszRet = CharNextExA( g_uCP, pszRet, 0 );
		}
	}
	
#ifdef NO_RAID_154 // 2002.05.28.0001
	*ppszEOL = pszSource;
#endif
	if ( pszEOL ){
#ifndef NO_RAID_154 // 2002.05.28.0001
		*ppszEOL = pszSource;
#endif
		int n = pszEOL - szTemp;
		if ( n ){
			while ( n-- ){
				*ppszEOL = CharNextExA( g_uCP, *ppszEOL, 0 );
			}
		}
	}
	
	return pszRet;
}
*/
