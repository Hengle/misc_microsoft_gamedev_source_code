//
//
// File: wordwrap.cpp
//
// Desc.
//
//

#include "xgameRender.h"

#ifndef _XBOX
#include <windows.h>
#else	//_XBOX
#include <xtl.h>
#endif	//_XBOX
#include <malloc.h>
#include "wordwrap.h"

//-----------------------------------------------------------------------------
// defines
//-----------------------------------------------------------------------------

// #define KEYSTONE 1 // uncomment or define this in compiler option in case KEYSTONE.
#define COUNTOF(a) ( sizeof( a ) / sizeof( (a)[0] ) )

struct BreakInfo 
{ 
    WCHAR wch; 
    bool  isNonBeginningChar; 
    bool  isNonEndingChar; 
}; 
//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------

CB_GetWidthW	GetWidthW	= NULL;
CB_Reserved		Reserved	= NULL;

UINT	g_uOption			= WW_PROHIBITION;
#ifndef _XBOX 
UINT	g_uCP				= 1252;
BOOL	g_bIsNT				= FALSE;
WCHAR	g_szUniErrChar[2]	= L"";
CHAR	g_szMBErrChar[3] 	= "";
#endif //_XBOX
#if 1 // 
static CHAR	signature[] = "%%%WORDWRAPLIB:080131:%%%";
#endif
//--------------------------------------------------------------------------------------
// Array of characters that can't be Beginning or ending characters on a line
//--------------------------------------------------------------------------------------
static BreakInfo BreakArray[] = 
{ 
    {0x0021, true,  false}, // 0021: !                                         fr, etc. 
    {0x0024, false, true},  // 0024: $                                         kr 
    {0x0025, true,  false}, // 0025: %                                         tc, kr 
    {0x0027, true,  true},  // 0027: '                                         fr 
    {0x0028, false, true},  // 0028: (                                         tc, kr 
    {0x0029, true,  false}, // 0029: )                                         fr, etc. 
    {0x002E, true,  false}, // 002E: .                                         fr 
    {0x002F, true,  true},  // 002F: /                                         fr, etc. 
    {0x003A, true,  false}, // 003A: :                                         tc, kr 
    {0x003B, true,  false}, // 003B: ;                                         tc, kr 
    {0x003F, true,  false}, // 003F: ?                                         fr 
    {0x005B, false, true},  // 005B: [                                         tc, kr 
    {0x005C, false, true},  // 005C: Reverse Solidus                           kr 
    {0x005D, true,  false}, // 005D: ]                                         tc, kr 
    {0x007B, false, true},  // 007B: {                                         tc, kr 
    {0x007D, true,  false}, // 007D: }                                         tc, kr 
    {0x00A2, true,  false}, // 00A2: Åë  Cent Sign                              etc. 
    {0x00A3, false, true},  // 00A3: Pound Sign                                tc 
    {0x00A5, false, true},  // 00A5: Yen Sign                                  tc 
    {0x00A7, false, true},  // 00A7: Section Sign                              ja 
    {0x00A8, true,  false}, // 00A8: ÅN Diaeresis                              etc. 
    {0x00A9, true,  false}, // 00A9: ccopyright sign                           etc.
    {0x00AE, true,  false}, // 00AE: Rregistered sign                          etc.
    {0x00B0, true,  false}, // 00B0: Degree Sign                               kr 
    {0x00B7, true,  false}, // 00B7: Middle Dot                                tc 
    {0x02C7, true,  false}, // 02C7: ? Caron                                   etc. 
    {0x02C9, true,  false}, // 02C9: ? Modified Letter marcon                  etc. 
    {0x2013, true,  false}, // 2013: En Dash                                   tc 
    {0x2014, true,  false}, // 2014: Em Dash                                   tc 
    {0x2015, true,  false}, // 2015: Å\ Horizon bar                            etc. 
    {0x2016, true,  false}, // 2016: ? Double vertical line                   etc. 
    {0x2018, false, true},  // 2018: Left Single Quotation Mark                tc, kr 
    {0x2019, true,  false}, // 2019: Right Single Quotation Mark               tc, fr, kr 
    {0x201C, false, true},  // 201C: Left Double Quotation Mark                tc, kr 
    {0x201D, true,  false}, // 201D: Right Double Quotation Mark               de, kr 
    {0x2025, true,  false}, // 2025: Two Dot Leader                            tc 
    {0x2026, true,  false}, // 2026: Horizontal Ellpsis                        tc 
    {0x2027, true,  false}, // 2027: Hyphenation Point                         tc 
    {0x2032, true,  false}, // 2032: Prime                                     tc, kr 
    {0x2033, true,  false}, // 2033: Double Prime                              kr 
    {0x2035, false, true},  // 2035: Reversed Prime                            tc 
    {0x2103, true,  false}, // 2103: Degree Celsius                            ja, kr 
    {0x2122, true,  false}, // 2122: trade mark sign 
    {0x2236, true,  false}, // 2236: ? tilde operator                          etc. 
    {0x2574, true,  false}, // 2574: Box Drawings Light Left                   tc 
    {0x266F, false, true},  // 266F: Music Sharp Sign                          ja 
    {0x3001, true,  false}, // 3001: fullwidth ideographic comma               ja, tc 
    {0x3002, true,  false}, // 3002: fullwidth full stop                       ja, tc 
    {0x3005, true,  false}, // 3005: Ideographic Iteration Mark                ja 
    {0x3003, true,  false}, // 3003: ÅVDitto Mark                              etc. 
    {0x3008, false, true},  // 3008: Left Angle Bracket                        tc, kr 
    {0x3009, true,  false}, // 3009: Right Angle Bracket                       tc, kr 
    {0x300A, false, true},  // 300A: Left Double Angle Bracket                 tc, kr 
    {0x300B, true,  false}, // 300B: Right Double Angle Bracket                tc, kr 
    {0x300C, false, true},  // 300C: Left Corner Bracket                       ja, tc, kr 
    {0x300D, true,  false}, // 300D: Right Corner Bracket                      ja, tc, kr 
    {0x300E, false, true},  // 300E: Left White Corner Bracket                 tc, kr 
    {0x300F, true,  false}, // 300F: Rignt White Corner Bracket                tc, kr 
    {0x3010, false, true},  // 3010: Left Black Lenticular Bracket             ja, tc, kr 
    {0x3011, true,  false}, // 3011: right black lenticular bracket            ja, tc, kr 
    {0x3012, false, true},  // 3012: Postal Mark                               ja 
    {0x3014, false, true},  // 3014: Left Tortoise Shell Bracket               tc, kr 
    {0x3015, true,  false}, // 3015: Right Tortoise Shell Bracket              tc, kr 
    {0x3016, false, true }, // 3016: ?                                        etc. 
    {0x3017, true,  false}, // 3017: ?                                        etc. 
    {0x301D, false, true},  // 301D: Reversed Double Prime Quotation Mark      tc 
    {0x301E, true,  false}, // 301E: Double Prime Quotation Mark               tc 
    {0x301F, true,  false}, // 301F: Low Double Prime Quotation Mark           tc 
    {0x3041, true,  false}, // 3041: Hiragana Letter Small A                   ja 
    {0x3043, true,  false}, // 3043: Hiragana Letter Small I                   ja 
    {0x3045, true,  false}, // 3045: Hiragana Letter Small U                   ja 
    {0x3047, true,  false}, // 3047: Hiragana Letter Small E                   ja 
    {0x3049, true,  false}, // 3049: Hiragana Letter Small O                   ja 
    {0x3063, true,  false}, // 3063: Hiragana Letter Small Tu                  ja 
    {0x3083, true,  false}, // 3083: Hiragana Letter Small Ya                  ja 
    {0x3085, true,  false}, // 3085: Hiragana Letter Small Yu                  ja 
    {0x3087, true,  false}, // 3087: Hiragana Letter Small Yo                  ja 
    {0x308E, true,  false}, // 308E: Hiragana Letter Small Wa                  ja 
    {0x3099, true,  false}, // 3099: Combining Katakana-Hiragana Voiced Sound Mark (if necessary) 
    {0x309A, true,  false}, // 309A: Combining Katakana-Hiragana Semi-Voiced Sound Mark (if necessary) 
    {0x309B, true,  false}, // 309B: Katakana-Hiragana Voiced Sound Mark       ja 
    {0x309C, true,  false}, // 309C: Katakana-Hiragana Semi-Voiced Sound Mark  ja 
    {0x309D, true,  false}, // 309D: Hiragana Iteration Mark                   ja 
    {0x309E, true,  false}, // 309E: Hiragana Voiced Iteration Mark            ja 
    {0x30A1, true,  false}, // 30A1: Katakana Letter Small A                   ja 
    {0x30A3, true,  false}, // 30A3: Katakana Letter Small I                   ja 
    {0x30A5, true,  false}, // 30A5: Katakana Letter Small U                   ja 
    {0x30A7, true,  false}, // 30A7: Katakana Letter Small E                   ja 
    {0x30A9, true,  false}, // 30A9: Katakana Letter Small O                   ja 
    {0x30C3, true,  false}, // 30C3: Katakana Letter Small Tu                  ja 
    {0x30E3, true,  false}, // 30E3: Katakana Letter Small Ya                  ja 
    {0x30E5, true,  false}, // 30E5: Katakana Letter Small Yu                  ja 
    {0x30E7, true,  false}, // 30E7: Katakana Letter Small Yo                  ja 
    {0x30EE, true,  false}, // 30EE: Katakana Letter Small Wa                  ja 
    {0x30F5, true,  false}, // 30F5: Katakana Letter Small Ka                  ja 
    {0x30F6, true,  false}, // 30F6: Katakana Letter Small Ke                  ja 
    {0x30FB, true,  false}, // 30FB: katakana middle dot                       ja 
    {0x30FC, true,  false}, // 30FC: Katakana-Hiragana Prolonged Sound Mark    ja 
    {0x30FD, true,  false}, // 30FD: Katakana Iteration Mark                   ja 
    {0x30FE, true,  false}, // 30FE: Katakana Voiced Iteration Mark            ja 
    {0xFE50, true,  false}, // FE50: Small Comma                               tc 
    {0xFE51, true,  false}, // FE51: Small Ideographic Comma                   tc 
    {0xFE52, true,  false}, // FE52: Small Full Stop                           tc 
    {0xFE54, true,  false}, // FE54: Small Semicolon                           tc 
    {0xFE55, true,  false}, // FE55: Small Colon                               tc 
    {0xFE56, true,  false}, // FE56: Small Question Mark                       tc 
    {0xFE57, true,  false}, // FE57: Small Exclamation Mark                    tc 
    {0xFE59, false, true},  // FE59: Small Left Parenthesis                    tc 
    {0xFE5A, true,  false}, // FE5A: Small Right Parenthesis                   tc 
    {0xFE5B, false, true},  // FE5B: Small Left Curly Bracket                  tc 
    {0xFE5C, true,  false}, // FE5C: Small Right Curly Bracket                 tc 
    {0xFE5D, false, true},  // FE5D: Small Left Tortoise Shell Bracket         tc 
    {0xFE5E, true,  false}, // FE5E: Small Right Tortoise Shell Bracket        tc 
    {0xFF01, true,  false}, // FF01: Fullwidth Exclamation Mark                ja, tc 
    {0xFF02, true,  false}, // FF02: ˙W Fullwidth quotation mark               etc. 
    {0xFF04, false, true},  // FF04: Fullwidth Dollar Sign                     kr 
    {0xFF05, true,  false}, // FF05: Fullwidth Percent Sign                    kr 
    {0xFF07, true,  false}, // FF07: ˙V Fullwidth Apos                         etc. 
    {0xFF08, false, true},  // FF08: Fullwidth Left Parenthesis                ja, tc 
    {0xFF09, true,  false}, // FF09: Fullwidth Right Parenthesis               ja, tc 
    {0xFF0C, true,  false}, // FF0C: Fullwidth Comma                           ja, tc, kr 
    {0xFF0E, true,  false}, // FF0E: Fullwidth Full Stop                       ja, tc, kr 
    {0xFF1A, true,  false}, // FF1A: Fullwidth Colon                           ja, tc, kr 
    {0xFF1B, true,  false}, // FF1B: Fullwidth Semicolon                       ja, tc, kr 
    {0xFF1F, true,  false}, // FF1F: Fullwidth Quation Mark                    ja, tc 
    {0xFF20, false, true},  // FF20: Fullwidth Commercial At                   ja 
    {0xFF3B, false, true},  // FF3B: Fullwidth Left Square Bracket             kr 
    {0xFF3D, true,  false}, // FF3D: Fullwidth Right Square Bracket            kr 
    {0xFF40, true,  false}, // FF40: ÅM Fullwidth Grave accent                 etc. 
    {0xFF5B, false, true},  // FF5B: Fullwidth Left Curly Bracket              ja, tc 
    {0xFF5C, true,  false}, // FF5C: Åb Fullwidth Vertical line                etc. 
    {0xFF5D, true,  false}, // FF5D: Fullwidth Right Curly Bracket             kr 
    {0xFF5E, true,  false}, // FF5E: Å` Fullwidth Tilda                        etc. 
    {0xFFE0, true,  true},  // FFE0: Fullwidth Cent Sign                       ja, kr 
    {0xFFE1, false, true},  // FFE1: Fullwidth Pound Sign                      fr, kr 
    {0xFFE5, false, true},  // FFE5: Fullwidth Yen Sign                        ja 
    {0xFFE6, false, true}   // FFE6: Fullwidth Won Sign                        kr 
}; 
const INT g_iNumBreakCharacters = COUNTOF(BreakArray);


//-----------------------------------------------------------------------------
//	program
//-----------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// Name: IsNonBeginningChar
//--------------------------------------------------------------------------------------
static bool IsNonBeginningChar( WCHAR c )
{
	if ( ! (g_uOption & WW_PROHIBITION) ) 
		return FALSE;
	
	INT iLeft = 0;
	INT iRight = g_iNumBreakCharacters;
	INT iMid = 0;

	if(c==0x2122)
	{
		int ir = 1;
		ir++;
	}

	// Binary search through the array of break characters
    while (iLeft <= iRight)
	{
		iMid = ((iRight-iLeft)/2) + iLeft;
		if (BreakArray[iMid].wch == c)
		{
			return BreakArray[iMid].isNonBeginningChar;
		}
		if (c < BreakArray[iMid].wch)
		{
			iRight = iMid - 1;
		}
		else
		{
			iLeft = iMid + 1;
		}

	}
	return false;
}


//--------------------------------------------------------------------------------------
// Name: IsNonEndingChar
//--------------------------------------------------------------------------------------
static bool IsNonEndingChar( WCHAR c )
{
	if ( ! (g_uOption & WW_PROHIBITION) ) 
		return FALSE;
	
	INT iLeft = 0;
	INT iRight = g_iNumBreakCharacters;
	INT iMid = 0;


	// Binary search through the array of break characters
    while (iLeft <= iRight)
	{
		iMid = ((iRight-iLeft)/2) + iLeft;
		if (BreakArray[iMid].wch == c)
		{
			return BreakArray[iMid].isNonEndingChar;
		}
		if (c < BreakArray[iMid].wch)
		{
			iRight = iMid - 1;
		}
		else
		{
			iLeft = iMid + 1;
		}

	}
	return false;
}

void WordWrap_SetCallback( CB_GetWidthW pGetWidthW, CB_Reserved pReserved )
{
	if ( pGetWidthW ){
		GetWidthW = pGetWidthW;
	}
	if ( pReserved ){
		Reserved = pReserved;
	}
}

void WordWrap_SetOption( UINT uOption )
{
	g_uOption = uOption;
}

#if !defined( _XBOX )
void WordWrap_SetOption( UINT uCP, UINT uOption )
{
	WordWrap_SetOption( uOption );
	g_uCP = uCP;
	
	// Check NT
	OSVERSIONINFO verinfo;
	verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &verinfo );
	g_bIsNT = ((verinfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) ? TRUE : FALSE);
	
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
}
#endif

static bool IsEastAsianChar( WCHAR c )
{
#if 1 // Mar.17,2004 15:21 by yutaka.
if ( g_uOption & WW_NOHANGULWRAP ){
	if ( 
		( 0x1100 <= c ) && ( c <= 0x11FF ) ||		// Hangul Jamo
		( 0x3130 <= c ) && ( c <= 0x318F ) ||		// Hangul Compatibility Jamo
#if 1 // mfuse : 2004.3.23 11:39
		( 0xAC00 <= c ) && ( c <= 0xD7A3 )			// Hangul Syllables
#else
		( 0xAC00 <= c ) && ( c <= 0xF8FF )			// Hangul Syllables
#endif
       ) return FALSE;
}
#endif
	return
		( 0x1100 <= c ) && ( c <= 0x11FF ) ||		// Hangul Jamo
		( 0x3000 <= c ) && ( c <= 0xD7AF ) ||		// CJK symbols - Hangul Syllables
		( 0xF900 <= c ) && ( c <= 0xFAFF ) ||		// CJK compat
		( 0xFF00 <= c ) && ( c <= 0xFFDC );			// Halfwidth / Fullwidth
}

#ifdef KEYSTONE
bool WordWrap_CanBreakLineAt( LPCWSTR psz, LPCWSTR pszStart )
{
	if ( psz == pszStart )
	{
		return false;	// leave at least one character in a line
	}
	if(WordWrap_IsWhiteSpace( *psz ) && IsNonBeginningChar( psz[1] ) )
	{
		return 	false;	
	}
	return
		// The current character *psz can be a new token if the following
		// condition holds. 
		//
		( IsEastAsianChar( *psz ) || 
		  IsEastAsianChar( psz[-1] ) || 
		  WordWrap_IsWhiteSpace(psz[-1]) || ( 0x3000 == psz[-1] ) || // 0x3000 - Ideographic Space
		  ( psz[-1] == L'-' ) ) &&	
		  !IsNonBeginningChar( *psz ) && !IsNonEndingChar( psz[-1] ) && 
		  !WordWrap_IsWhiteSpace(*psz) && ( 0x3000 != *psz );
}
#else // KEYSTONE
bool WordWrap_CanBreakLineAt( LPCWSTR psz, LPCWSTR pszStart )
{
	if ( psz == pszStart )
	{
		return false;	// leave at least one character in a line
	}
	if(WordWrap_IsWhiteSpace( *psz ) && IsNonBeginningChar( psz[1] ) )
	{
		return 	false;	
	}

	if(psz - pszStart >1)	// Do not word wrap when current character is IsEastAsianChar , the previus character as " and before it was space.
	{
		if(WordWrap_IsWhiteSpace( psz[-2] ) && psz[-1]==L'\"' && !WordWrap_IsWhiteSpace( *psz ) ) 
		// Do not leave " at end of line when the leading character is not space.
		{
			return false;	
		}
	}
	if(!WordWrap_IsWhiteSpace( psz[-1] ) && *psz==L'\"' && WordWrap_IsWhiteSpace( psz[1] ) )
		// Do not put " at top of line when the " is closing word and leading space.
	{
		return false;
	}



	return
		( WordWrap_IsWhiteSpace( *psz ) || IsEastAsianChar( *psz ) || IsEastAsianChar( psz[-1] ) || psz[-1] == L'-') &&
		!IsNonBeginningChar( *psz ) && !IsNonEndingChar( psz[-1] );

}

#endif // KEYSTONE


int WordWrap_FindNonWhiteSpaceForward_Pos( LPCWSTR psz )
{
   int curPos = 0;
	while ( WordWrap_IsWhiteSpace( *psz ) )
	{
		psz++;
      curPos++;
	}
#ifndef NO_RAID_151_152 // May.21,2002 06:04 by yutaka.
	if ( psz && WordWrap_IsLineFeed(*psz) ) 
   {
      psz++;
      curPos++;
   }
#endif // NO_RAID_151_152
	return ( *psz ) ? curPos : 0;
}

LPCWSTR WordWrap_FindNonWhiteSpaceForward( LPCWSTR psz )
{
	while ( WordWrap_IsWhiteSpace( *psz ) )
	{
		psz++;
	}
#ifndef NO_RAID_151_152 // May.21,2002 06:04 by yutaka.
	if ( psz && WordWrap_IsLineFeed(*psz) ) psz++;
#endif // NO_RAID_151_152
	return ( *psz ) ? psz : NULL;
}

static LPCWSTR FindNonWhiteSpaceBackward( LPCWSTR psz, LPCWSTR pszSource )
{
	while ( psz >= pszSource && ( WordWrap_IsWhiteSpace( *psz ) || WordWrap_IsLineFeed( *psz ) ) )
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
LPCWSTR WordWrap_FindNextLineW( LPCWSTR pszSource, UINT uWidth, LPCWSTR* ppszEOL )
{
	if ( GetWidthW == NULL || pszSource == NULL || !pszSource[0] )
	{
		*ppszEOL = NULL;
		return NULL;
	}
    
	LPCWSTR psz = pszSource;
	UINT uCurrent;
    
	for ( uCurrent = 0; *psz && !WordWrap_IsLineFeed( *psz ); psz++ )
	{
		uCurrent += GetWidthW( *psz );
		if ( uCurrent > uWidth )
			break;
	}
	if ( pszSource == psz )
	{
		// Break at second character if line width is less than character width
		*ppszEOL = FindNonWhiteSpaceBackward( psz, pszSource );
		return WordWrap_FindNonWhiteSpaceForward( psz + 1 );
	}
	if ( WordWrap_IsLineFeed( *psz ) )
	{
		psz++;
	}
	if ( uCurrent <= uWidth )
	{
		// The line is shorter than the width
		*ppszEOL = FindNonWhiteSpaceBackward( psz - 1, pszSource );
#ifndef NO_RAID_151_152 // May.21,2002 06:04 by yutaka.
		if ( psz-1 >= pszSource && WordWrap_IsLineFeed( *(psz-1) ) ){
			return psz;
		}
#endif // NO_RAID_151_152
		return ( *psz ) ? WordWrap_FindNonWhiteSpaceForward( psz ) : NULL;
	}
    
	LPCWSTR pszOverflow = psz;

	while ( psz > pszSource )
	{
		if ( WordWrap_IsWhiteSpace( *psz ) )
		{
			*ppszEOL = FindNonWhiteSpaceBackward( psz, pszSource );
			if ( !*ppszEOL )
			{
				return WordWrap_FindNonWhiteSpaceForward( psz + 1 );
			}
			psz = *ppszEOL + 1;
		}
		if ( WordWrap_CanBreakLineAt( psz, pszSource ) )
		{
			break;
		}
		psz--;
	}
	if ( psz <= pszSource )
	{
		// couldn't find any character to break the line
		*ppszEOL = pszOverflow - 1;
		return pszOverflow;
	}
	*ppszEOL = psz - 1;
	return WordWrap_FindNonWhiteSpaceForward( psz );
}

#ifndef _XBOX
LPCSTR WordWrap_FindNextLineA( LPCSTR pszSource, UINT uWidth, LPCSTR* ppszEOL )
{
	if ( GetWidthW == NULL || pszSource == NULL || !pszSource[0] )
	{
		*ppszEOL = NULL;
		return NULL;
	}
	
	// As long as 1 line doesn't exceed following buffer size (=char num), 
	// string is processed correctly.
	static WCHAR szTemp[1024*8];
#ifndef NO_RAID_156
	BOOL bIsInsufficientBuffer = FALSE;
#endif // NO_RAID_156
	
#ifndef NO_RAID_156
	int ret = MultiByteToWideChar( g_uCP, 0, pszSource, -1, szTemp, COUNTOF(szTemp) );
	if ( ret == 0 ){
		if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ){
			ret = COUNTOF(szTemp)-1;
			bIsInsufficientBuffer = TRUE;
		}
	}
#else // NO_RAID_156
	int ret = MultiByteToWideChar( g_uCP, 0, pszSource, -1, szTemp, COUNTOF(szTemp)-1 );
#endif // NO_RAID_156
	if ( ret == 0 ){
		*ppszEOL = NULL;
		return NULL;
	}
#ifndef NO_RAID_156
	szTemp[COUNTOF(szTemp)-1] = 0;
#else // NO_RAID_156
	szTemp[ret] = 0;
#endif // NO_RAID_156
	
	if ( !(g_uOption & WW_NOMBCSSP) && (!g_bIsNT) && g_szUniErrChar[0] ){ // 2002.05.28.0001
		
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
					*/
					
					if ( g_uCP == 1252 ){
						szTemp[i] = 0x003F; // L'W'; // replace with the wider char for USA
					} else {
						szTemp[i] = 0x4E9E; // L'ò±'; // replace with the wider char for EA
						// Unicode		U+4E9E
						// JPN(SJIS)	0x98B1
						// CHT(BIG5)	0xE4AC
						// CHS(BG2312)	0xA8C8
					}
				}
			}
			pszTemp = CharNextExA( (WORD)g_uCP, pszTemp, 0 );
		}
	}
	
	LPCWSTR pszEOL = NULL;
	LPCWSTR pszNext = WordWrap_FindNextLineW( szTemp, uWidth, &pszEOL );
	
	LPCSTR pszRet = NULL;
#ifndef NO_RAID_156
	if ( !pszNext && bIsInsufficientBuffer ){
		pszNext = szTemp + COUNTOF(szTemp)-1;
	}
#endif // NO_RAID_156
	if ( pszNext ){
		int n = pszNext - szTemp;
		pszRet = pszSource;
		while ( n-- ){
			pszRet = CharNextExA( (WORD)g_uCP, pszRet, 0 );
		}
	}
	
#ifdef NO_RAID_154 // 2002.05.28.0001
	*ppszEOL = pszSource;
#endif // NO_RAID_154
	if ( pszEOL ){
#ifndef NO_RAID_154 // 2002.05.28.0001
		*ppszEOL = pszSource;
#endif // NO_RAID_154
		int n = pszEOL - szTemp;
		if ( n ){
			while ( n-- ){
				*ppszEOL = CharNextExA( (WORD)g_uCP, *ppszEOL, 0 );
			}
		}
	}
	
	return pszRet;
}
#endif //_XBOX

#if defined( WORDWRAP_TOKENIZE )
//
// WordWrap_Tokenize
//
// Desc.	Tokenizes an input string.  Each East Asian character is
//			a token on its own unless it is a punctuation.  All other languages
//			are tokenized based on having a space between sets of letters.
//
// param:	pszSource: (in) NULL terminated wide character string
//			ppszDest:  (out) NULL terminated list of strings; each string is a 
//			NULL terminated wide characters.  If NULL is passed, the function
//			returns the number of tokens that will be created.
//	
// return:	Number of tokens that will be created.  This is meaningful only if
//			ppszDest is NULL
//
// Note:	Caller is responsible for allocating the memory for the array of pointers.
//			The function will allocate the memory for the each string in the array,
//			but the caller is responsible for freeing each of strings.  Use 
//			WordWrap_ClearTokens to free the memory used by the list of string pointers.
//
int WordWrap_TokenizeW( LPCWSTR pszSource, LPWSTR* ppszDest)
{
	if( !pszSource ) {
		return 0;
	}

	// Start from the second character because whether or not a character can be
	// tokenized depends on character preceeding the current character.
	//
	LPCWSTR pToken = pszSource + 1; 
	LPCWSTR pBegin = pszSource;
	int iTokens = 0;
	int nLength = wcslen(pszSource);
	for(int i = 0; i < nLength; i++) {
		if( WordWrap_CanBreakLineAt( pToken,NULL ) || !*pToken ) {
			if( ppszDest ) {
				*ppszDest = new WCHAR[pToken-pBegin+1];
				wcsncpy(*ppszDest,pBegin,pToken-pBegin);
				(*ppszDest)[pToken-pBegin] = 0;
				++ppszDest;
			}
			++iTokens;
			pBegin = pToken;
		}
		++pToken;
	}

	return iTokens;
}

// ANSI version of the above function
//
#ifndef _XBOX
int WordWrap_TokenizeA( LPCSTR pszSource, LPSTR* ppszDest)
{
	if( !pszSource ) {
		return 0;
	}

	int iTokens = 0;
	int iSize = MultiByteToWideChar( g_uCP, 0, pszSource, -1, NULL, 0 );

	// This memory only needs to exists during the lifespan of this
	// function.  So, allocate the memory on stack
	//
	LPWSTR pwch = (LPWSTR) _alloca( sizeof(WCHAR) * iSize );

	// If there is not enough memory in stack, use the heap instead.
	//
	bool bHeapUsed = false;
	if(!pwch) {
		bHeapUsed = true;
		pwch = new WCHAR [iSize];
	}
	
	if(MultiByteToWideChar( g_uCP, 0, pszSource, -1, pwch, iSize )) {
		LPCWSTR pToken = pwch + 1; 
		LPCWSTR pBegin = pwch;
		int nLength = wcslen(pwch);
		for(int i = 0; i < nLength; i++) {
			if( WordWrap_CanBreakLineAt( pToken,NULL ) || !*pToken ) {
				if( ppszDest ) {
					int iBytes = WideCharToMultiByte( g_uCP, 0, pBegin, pToken - pBegin, 
						                              NULL, 0, NULL, NULL );
					*ppszDest = new char [iBytes + 1];
					WideCharToMultiByte( g_uCP, 0, pBegin, pToken - pBegin,
						                 *ppszDest, iBytes, NULL, NULL );
					(*ppszDest)[iBytes] = 0;
					++ppszDest;

				}
				++iTokens;
				pBegin = pToken;
			}
			++pToken;
		}
	}

	if(bHeapUsed) {
		delete [] pwch;
	}

	return iTokens;
}
#endif //_XBOX

//
// WordWrap_ClearTokens
//
// Desc.	Deallocates the memory used by the token
//
// param:	ppszTokens	Null terminated list of NULL terminated strings
//	
void WordWrap_ClearTokensW( LPWSTR* ppszTokens )
{
	if(ppszTokens) {
		LPWSTR* ppSave = ppszTokens;
		while(*ppszTokens) {
			delete [] *ppszTokens;
			++ppszTokens;
		}
		delete [] ppSave;
	}
}

// ANSI version of the above function
//
#ifndef _XBOX
void WordWrap_ClearTokensA( LPSTR* ppszTokens )
{
	if(ppszTokens) {
		LPSTR* ppSave = ppszTokens;
		while(*ppszTokens) {
			delete [] *ppszTokens;
			++ppszTokens;
		}
		delete [] ppSave;
	}
}
#endif //_XBOX

//
// WordWrap_InitTokens
//
// Desc.	Initializes the NULL terminated list of NULL terminated strings by allocating
//			memory, and putting NULL at the end of the list to denote NULL.
//
// param:	ppszTokens	Reference to the list of NULL terminated NULL terminated strings
//			iSize		Number of elements (strings) in the list
//
void WordWrap_InitTokensW( LPWSTR* &ppszTokens, int iSize )
{
	if(ppszTokens)
		return;
	ppszTokens = new LPWSTR [iSize+1];
	ppszTokens[iSize] = 0;
}

// ANSI version of the above function
//
#if !defined( _XBOX )
void WordWrap_InitTokensA( LPSTR* &ppszTokens, int iSize )
{
	if(ppszTokens)
		return;
	ppszTokens = new LPSTR [iSize+1];
	ppszTokens[iSize] = 0;
}
#endif //_XBOX
#endif //WORDWRAP_TOKENIZE


#if defined( WORDWRAP_TOKENIZE ) // mfuse : import v-sbshik's Tokenize : 2004.1.30 10:37
// WordWrap_String member functions definitions

WordWrap_String::WordWrap_String()
{
	m_pszSource = NULL;
	m_iTokens = 0;
	m_pszFirstToken = NULL;
}

WordWrap_String::WordWrap_String(LPCTSTR pszSource)
{
	m_pszSource = pszSource;
	m_iTokens = 0;
	m_pszFirstToken = NULL;
}

WordWrap_String::~WordWrap_String()
{
}

int WordWrap_String::Tokenize( LPTSTR* ppszDest)
{
	if( !m_pszSource ) {
		return 0;
	}

#if defined( UNICODE ) | defined( _XBOX )
	return TokenizeW( (LPWSTR*)ppszDest );
#else  
	return TokenizeA( ppszDest );
#endif 
}

int WordWrap_String::TokenizeW( LPWSTR* ppszDest )
{
	// If ppszDest is not NULL, then it must the be second time this function is
	// called with the known number of tokens.  Then, allocate enough chunck of
	// memory so that all the strings will be contiguous.
	if( ppszDest )
	{
		if(m_pszFirstToken) {
			delete m_pszFirstToken;
		}

		// We need to allocate m_iTokens many more characters because each of the tokens
		// are NULL terminated, adding an extra character per token.
		//
		m_pszFirstToken = (LPTSTR) new WCHAR [wcslen((wchar_t*)m_pszSource) + m_iTokens];
	}

	// Start from the second character because whether or not a character can be
	// tokenized depends on character preceeding the current character.
	//
	LPCWSTR pToken = (LPCWSTR) (m_pszSource + 1); 
	LPCWSTR pBegin = (LPCWSTR) m_pszSource;
	LPWSTR pszFirstToken = (LPWSTR) m_pszFirstToken;
	int iTokens = 0;
	int nLength = wcslen((wchar_t*)m_pszSource);
	for(int i = 0; i < nLength; i++) {
		if( WordWrap_CanBreakLineAt( pToken,NULL ) || !*pToken ) {
			if( ppszDest ) {
				*ppszDest = pszFirstToken;
				wcsncpy(*ppszDest,pBegin,pToken-pBegin);
				(*ppszDest)[pToken-pBegin] = 0;
				++ppszDest;
				pszFirstToken += pToken - pBegin + 1;
			}
			else
			++iTokens;
			pBegin = pToken;
		}
		++pToken;
	}

	return iTokens;
}

#ifndef _XBOX
int WordWrap_String::TokenizeA( LPSTR* ppszDest )
{
	// If ppszDest is not NULL, then it must the be second time this function is
	// called with the known number of tokens.  Then, allocate enough chunck of
	// memory so that all the strings will be contiguous.
	if( ppszDest )
	{
		if(m_pszFirstToken) {
			delete m_pszFirstToken;
		}

		// We need to allocate m_iTokens many more characters because each of the tokens
		// are NULL terminated, adding an extra character per token.
		//
		m_pszFirstToken = new char [strlen(m_pszSource) + m_iTokens];
	}

	int iTokens = 0;
	int iSize = MultiByteToWideChar( g_uCP, 0, m_pszSource, -1, NULL, 0 );

	// This memory only needs to exists during the lifespan of this
	// function.  So, allocate the memory on stack
	//
	LPWSTR pwch = (LPWSTR) _alloca( sizeof(WCHAR) * iSize );

	// If there is not enough memory in stack, use the heap instead.
	//
	bool bHeapUsed = false;
	if(!pwch) {
		bHeapUsed = true;
		pwch = new WCHAR [iSize];
	}
	
	if(MultiByteToWideChar( g_uCP, 0, m_pszSource, -1, pwch, iSize )) {
		LPCWSTR pToken = pwch + 1; 
		LPCWSTR pBegin = pwch;
		LPSTR pszFirstToken = m_pszFirstToken;
		int nLength = wcslen(pwch);
		for(int i = 0; i < nLength; i++) {
			if( WordWrap_CanBreakLineAt( pToken,NULL ) || !*pToken ) {
				if( ppszDest ) {
					int iBytes = WideCharToMultiByte( g_uCP, 0, pBegin, pToken - pBegin, 
						                              NULL, 0, NULL, NULL );
					*ppszDest = pszFirstToken;
					WideCharToMultiByte( g_uCP, 0, pBegin, pToken - pBegin,
						                 *ppszDest, iBytes, NULL, NULL );
					(*ppszDest)[iBytes] = 0;
					++ppszDest;
					pszFirstToken += iBytes + 1;

				}
				++iTokens;
				pBegin = pToken;
			}
			++pToken;
		}
	}

	if(bHeapUsed) {
		delete [] pwch;
	}

	return iTokens;
}
#endif // _XBOX

void WordWrap_String::ClearTokens( LPTSTR* ppszTokens )
{
	delete m_pszFirstToken;
	delete [] ppszTokens;
}

void WordWrap_String::InitTokens( LPTSTR* &ppszTokens, int iSize )
{
	if(ppszTokens)
		return;
	ppszTokens = new LPTSTR [iSize+1];
	ppszTokens[iSize] = 0;
	m_iTokens = iSize;
}
#endif // WORDWRAP_TOKENIZE
