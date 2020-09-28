//	Display.h : Class to manage graphics for survey display.
//  This class provides a generic interface for displaying
//  survey questions regardless of the platform (PC or Xbox).
//  A single header file is used for all project versions,
//  although platform specific .cpp files are implemented as
//  required.
//
//	Created 2004/03/22 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

// The following includes and definitions will depend on the platform

#ifdef _XBOX
	#include <XTL.h>
	#if _XBOX_VER >= 200 // Xenon
		#include "VinceFontXe.h"
		#define DEVICE_POINTER LPDIRECT3DDEVICE9
		#define STATE_BLOCK IDirect3DStateBlock9*
	#else				 // Xbox
		#include "VinceFont.h"
		#define DEVICE_POINTER void*
		#define STATE_BLOCK DWORD
	#endif
	#define FOOTER_TEXT_STANDARD L"Press " GLYPH_A_BUTTON L" to select, or " GLYPH_B_BUTTON L" to cancel"
	#define FOOTER_TEXT_MULTI_SELECT L"Press " GLYPH_X_BUTTON L" to check, " GLYPH_A_BUTTON L" to accept, " GLYPH_B_BUTTON L" to cancel"
#else					// Windows
	#include <D3DX9.h>
	#include "VinceFontDX9.h"
	#define DEVICE_POINTER LPDIRECT3DDEVICE9
    #define STATE_BLOCK IDirect3DStateBlock9*
	#define FOOTER_TEXT_STANDARD L"Arrow Keys + Enter to select, or Escape to cancel"
	#define FOOTER_TEXT_MULTI_SELECT L"Arrow Keys + Space to check, Enter to accept, Escape to cancel"
#endif


#define MAX_FONTS 5
#define MAX_IMAGES 5

namespace Vince
{
	class CVince;

	// CDisplay is a singleton class used for survey display
	class CDisplay
	{
	public:
		static CDisplay* Instance();
		static void DestroyInstance();
		HWND GetHWND();
		void SetHWND( HWND hWnd );
		void* GetDevice();
		void SetDevice( void* pDevice );
		void* GetPresentationParameters();
		void SavePresentationParameters( void* pParameters );
		void Clear(DWORD dwBackgroundColor);
		int LoadFont(CVince* vince, const char* cstrFontName);
		void SetFont(int intFontNumber);
		void Begin(CVince* vince);
		void End(CVince* vince);
		void BeginText();
		void ScaleFont(float xScale, float yScale);
		float GetTextWidth( const WCHAR* wstrText );
		void DrawText(float xPos, float yPos, DWORD dwColor, const WCHAR* wstrText);
		void DrawBox(float xLeft, float yTop, float xRight, float yBottom, DWORD dwOutlineColor, DWORD dwFillColor);
		void ShowBackground(const char* cstrImageFileName);
		void EndText();
		void Present();

	protected:
		CDisplay();
		~CDisplay();
		CDisplay(const CDisplay&);
		CDisplay& operator= (const CDisplay&);

		void SaveClipPlanes();
		void RestoreClipPlanes();
		void PreparePresentation();
		void RestorePresentation();

		int m_SelectedFont;
		float m_fXScale;
		float m_fYScale;
		STATE_BLOCK m_StateBlock;
		float m_fSaveNear;
		float m_fSaveFar;

        HWND m_hWnd;
		DEVICE_POINTER CDisplay::m_pd3dDevice;
		D3DPRESENT_PARAMETERS* m_pPresentationParameters;

		CVinceFont* m_Font[MAX_FONTS];
		const char* m_FontName[MAX_FONTS];
		int m_NumFonts;

		int m_SelectedImage;
		const char* m_ImageName[MAX_IMAGES];
		int m_NumImages;

	private:
		static CDisplay* s_pInstance;
	};
}
