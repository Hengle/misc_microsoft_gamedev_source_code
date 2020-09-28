//
//
// File: wordwrap.h
//
//

#ifndef __WORDWRAP_H
#define __WORDWRAP_H

typedef UINT (*CB_GetWidthW)( WCHAR );
typedef UINT (*CB_Reserved)( VOID );

void 	WordWrap_SetOption( UINT uOption );
#if !defined( _XBOX )
void 	WordWrap_SetOption( UINT uCP, UINT uOption );
#endif
void	WordWrap_SetCallback( CB_GetWidthW pGetWidthW, CB_Reserved pReserved );

LPCWSTR	WordWrap_FindNextLineW( LPCWSTR pszSource, UINT uWidth, LPCWSTR* ppszEOL );

#if !defined( _XBOX )
LPCSTR	WordWrap_FindNextLineA( LPCSTR pszSource, UINT uWidth, LPCSTR* ppszEOL );
#endif

#if defined( WORDWRAP_TOKENIZE ) // mfuse : import v-sbshik's Tokenize : 2004.1.30 10:24
int		WordWrap_TokenizeW( LPCWSTR pszSource, LPWSTR* ppszDest);
void	WordWrap_ClearTokensW( LPWSTR* ppszTokens );
void	WordWrap_InitTokensW( LPWSTR* &ppszTokens, int iSize );
#if !defined( _XBOX )
int		WordWrap_TokenizeA( LPCSTR pszSource, LPSTR* ppszDest);
void	WordWrap_ClearTokensA( LPSTR* ppszTokens );
void	WordWrap_InitTokensA( LPSTR* &ppszTokens, int iSize );
#endif

class WordWrap_String {

	private:
		LPCTSTR m_pszSource;
		int		m_iTokens;
		LPTSTR	m_pszFirstToken;

	// Constructors and Destructor
	//
	public:
		WordWrap_String();
		WordWrap_String(LPCTSTR pszSource);
		~WordWrap_String();

		int		Tokenize( LPTSTR* ppszDest);
		int		TokenizeW( LPWSTR* ppszDest );
#if !defined( _XBOX )
		int		TokenizeA( LPSTR* ppszDest );
#endif
		void	ClearTokens( LPTSTR* ppszTokens );
		void	InitTokens( LPTSTR* &ppszTokens, int iSize );
};
#endif // WORDWRAP_TOKENIZE

//#if defined( UNICODE ) | defined( _XBOX )
 #if defined( _XBOX )
  //#define WordWrap_CharNext(p)		((p)+1)
 #else
  #define WordWrap_CharNext(cp,p)	CharNext(p)
 #endif
 #define WordWrap_FindNextLine		WordWrap_FindNextLineW
 #define WordWrap_Tokenize			WordWrap_TokenizeW
 #define WordWrap_ClearTokens		WordWrap_ClearTokensW
 #define WordWrap_InitTokens		WordWrap_InitTokensW
//#else
// #define WordWrap_FindNextLine		WordWrap_FindNextLineA
// #define WordWrap_CharNext(cp,p)	CharNextExA(cp,p,0)
// #define WordWrap_Tokenize			WordWrap_TokenizeA
// #define WordWrap_ClearTokens		WordWrap_ClearTokensA
// #define WordWrap_InitTokens		WordWrap_InitTokensA
//#endif

#define	WW_PROHIBITION	0x00000001
#define	WW_NOMBCSSP		0x00000002 // specify when disable MBCS SP Process
#if 1 // Mar.17,2004 15:25 by yutaka.
#define	WW_NOHANGULWRAP 0x00000004 // disable Hangul Character's WordWrap
#endif

//
// for compatibillity
//
#define LB_KINSOKU					WW_PROHIBITION
#define	LB_NOMBCSSP					WW_NOMBCSSP

#define LineBreak_SetOption			WordWrap_SetOption
//#define LineBreak_SetCallback		WordWrap_SetCallback
#define LineBreak_SetCallback		WordWrap_SetCallback

#define LineBreak_FindNextLineW		WordWrap_FindNextLineW
#define LineBreak_FindNextLineA		WordWrap_FindNextLineA
#define LineBreak_TokenizeW			WordWrap_TokenizeW
#define LineBreak_TokenizeA			WordWrap_TokenizeA
#define LineBreak_ClearTokensW		WordWrap_ClearTokensW
#define LineBreak_ClearTokensA		WordWrap_ClearTokensA
#define LineBreak_InitTokensW		WordWrap_InitTokensW
#define LineBreak_InitTokensA		WordWrap_InitTokensA

//#if defined( UNICODE )
 #define LineBreak_FindNextLine 	WordWrap_FindNextLineW
 #define LineBreak_CharNext(cp,p) 	CharNext(p)
 #define LineBreak_Tokenize			WordWrap_TokenizeW
 #define LineBreak_ClearTokens		WordWrap_ClearTokensW
 #define LineBreak_InitTokens		WordWrap_InitTokensW
//#else
// #define LineBreak_FindNextLine		WordWrap_FindNextLineA
// #define LineBreak_CharNext(cp,p)	CharNextExA(cp,p,0)
// #define LineBreak_Tokenize			WordWrap_TokenizeA
// #define LineBreak_ClearTokens		WordWrap_ClearTokensA
// #define LineBreak_InitTokens		WordWrap_InitTokensA
//#endif

#if 1 // mfuse : 2003.6.17 03:57
bool WordWrap_CanBreakLineAt( LPCWSTR psz, LPCWSTR pszStart );
LPCWSTR WordWrap_FindNonWhiteSpaceForward( LPCWSTR psz );
int WordWrap_FindNonWhiteSpaceForward_Pos( LPCWSTR psz );

#define WordWrap_IsWhiteSpace(c) ( ( c ) == L'\t' || ( c ) == L'\r' || ( c ) == L' ' || ( c ) == 0x3000 ) // RAID#210: Add full-width space(0x3000) : Jan.20,2003 00:20 by yutaka.
#define WordWrap_IsLineFeed(c) ( ( c ) == L'\n' )
#endif
#endif //__WORDWRAP_H
