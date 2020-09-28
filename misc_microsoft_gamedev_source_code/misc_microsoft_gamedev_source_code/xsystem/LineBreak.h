//============================================================================
//
//  LineBreak.h
//
//  This file was acquired from the East Asian Loc team and used "as is".
//  Contact Yutaka Hasegawa (yutakah@microsoft.com) for questions.
//
//============================================================================

typedef UINT (*CB_GetWidthW)( WCHAR, void* );
typedef UINT (*CB_Reserved)( VOID );

void LineBreak_SetOption( UINT uCP, UINT uOption );
//void LineBreak_SetCallback( (UINT (*)( WCHAR )) );
void LineBreak_SetCallback( CB_GetWidthW pGetWidthW, void* pParam, CB_Reserved pReserved );

LPCWSTR LineBreak_FindNextLineW( LPCWSTR pszSource, UINT uWidth, LPCWSTR* ppszEOL , bool isAtBOL, bool keepTrailingWS );
LPCSTR LineBreak_FindNextLineA( LPCSTR pszSource, UINT uWidth, LPCSTR* ppszEOL );

bool LineBreak_IsNonBeginningChar( WCHAR c );

#ifdef UNICODE
#define LineBreak_FindNextLine LineBreak_FindNextLineW
#define LineBreak_CharNext(cp,p) CharNext(p)
#else
#define LineBreak_FindNextLine LineBreak_FindNextLineA
#define LineBreak_CharNext(cp,p) CharNextExA(cp,p,0)
#endif

#define	LB_KINSOKU		0x00000001
#define	LB_NOMBCSSP		0x00000002 // specify when disable MBCS SP Process
