//	DisplayXe.cpp : Class to manage graphics for survey display
//  This class provides a generic interface for displaying
//  survey questions regardless of the platform (PC or Xbox).
//  A single header file is used for all project versions,
//  although platform specific .cpp files are implemented as
//  required. This is the Xenon version.
//
//	Created 2004/03/22 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_
#ifndef NO_VINCE_SURVEYS

#include "Vince.h"
#include "Display.h"
#include "TnTUtil.h"

//--------------------------------------------------------------------------------------
// Vertex and pixel shaders for background image rendering. Since the rendering code
// can add to the image size of VINCE, particularly in a debug build, the VINCE_NO_BACKGROUNDS
// flag is provided to disable relevent code from the executable. Note that the increase
// in image size is due to resulting bloat in the D3DX library, not VINCE itself.
//--------------------------------------------------------------------------------------

#ifndef NO_VINCE_BACKGROUNDS
typedef struct _SB_VERTEX
{
    FLOAT x,y,z,w;
    FLOAT u,v;
} SB_VERTEX;

static const SB_VERTEX verts[] =
{
    { -1.f, +1.f, 0.f, 1.f,  0.f, 0.f },
    { +1.f, +1.f, 0.f, 1.f,  1.f, 0.f },
    { -1.f, -1.f, 0.f, 1.f,  0.f, 1.f },
    { +1.f, -1.f, 0.f, 1.f,  1.f, 1.f },
};

static const char g_strVertexShaderProgram[] = 
" struct VS_DATA                               "  
" {                                            " 
"     float4 ObjPos   : POSITION;              "  // Object space position 
"     float2 T0       : TEXCOORD0;             "
" };                                           " 
"                                              " 
" VS_DATA main( VS_DATA In )                   "  
" {                                            "  
"     return In;                               "
" }                                            ";

static const char g_strPixelShaderProgram[] = 
" sampler s0 : register( s0 );                 "  
"                                              "  
" struct PS_IN                                 "
" {                                            "
"     float2 t0 : TEXCOORD0;                   "  // Interpolated color from                      
" };                                           "  // the vertex shader
"                                              "  
" float4 main( PS_IN In ) : COLOR              "  
" {                                            "  
"     return tex2D( s0, In.t0);                "  // Output color
" }                                            "; 
#endif // NO_VINCE_BACKGROUNDS

namespace Vince
{

	CDisplay* CDisplay::s_pInstance = 0;// initialize pointer

	CDisplay* CDisplay::Instance()
	{
		if (s_pInstance == 0)  // is it the first call?
		{  
			s_pInstance = new CDisplay(); // create sole instance
		}
		return s_pInstance; // address of sole instance
	}

	void CDisplay::DestroyInstance() 
	{
		if (s_pInstance != 0)  // has it been initialized?
		{  
			delete s_pInstance; // free memory
			s_pInstance = 0;	// clear instance pointer
		}
	}

	CDisplay::CDisplay()
	{
		//... perform necessary instance initializations 
		m_SelectedFont = -1;
		m_SelectedImage = -1;
		//m_fSaveNear = -D3DZ_MAX_D24S8;
		//m_fSaveFar = D3DZ_MAX_D24S8;
		m_fXScale = 1.0f;
		m_fYScale = 1.0f;
		m_StateBlock = NULL;
		m_pd3dDevice = NULL;
		m_pPresentationParameters = NULL;
		m_NumFonts = 0;
		m_NumImages = 0;
        m_hWnd = NULL;
	}

	CDisplay::~CDisplay()
	{
		for (int i = 0; i < m_NumFonts; i++)
		{
			SAFE_DELETE_ARRAY( m_FontName[i] );
			SAFE_DELETE( m_Font[i] );
		}
		m_NumFonts = 0;

		for (int i = 0; i < m_NumImages; i++)
		{
			SAFE_DELETE_ARRAY( m_ImageName[i] );
		}
		m_NumImages = 0;
	}

	// Retrieve the display window handle
    HWND CDisplay::GetHWND()
    {
        return(m_hWnd);
    }

	// Save the display window handle
	void CDisplay::SetHWND( HWND hWnd )
	{
        m_hWnd = hWnd;
    }

	// Retrieve the D3D device pointer and cast as void pointer
	void* CDisplay::GetDevice()
	{
		return (void*) m_pd3dDevice;
	}

	// Save the D3D device pointer
	void CDisplay::SetDevice( void* pDevice )
	{
		// If the previous pointer was not null, and this is
		// a new pointer, we must update all fonts to refer
		// to the new device pointer
		if ( NULL != m_pd3dDevice && pDevice != m_pd3dDevice && pDevice )
		{
			for (int i = 0; i < m_NumFonts; i++)
			{
				m_Font[i]->SetDevice( (LPDIRECT3DDEVICE9) pDevice );
			}
		}
		m_pd3dDevice = (LPDIRECT3DDEVICE9) pDevice;
	}

	// Retrieve the D3D device pointer and cast as void pointer
	void* CDisplay::GetPresentationParameters()
	{
		return (void*) m_pPresentationParameters;
	}

	// Save the game's presentation parameters
	void CDisplay::SavePresentationParameters( void* pParameters )
	{
		// We are going to copy these eventually, but just cast for now
		D3DPRESENT_PARAMETERS* pTemp = (D3DPRESENT_PARAMETERS*) pParameters;

		// If we have not already cached parameters, allocate some memory
		if ( NULL == m_pPresentationParameters )
		{
			m_pPresentationParameters = new D3DPRESENT_PARAMETERS;
		}

		// Save presentation parameters for later restoration
		memcpy(m_pPresentationParameters, pTemp, sizeof(D3DPRESENT_PARAMETERS));
	}

	// We save the state block to restore later. We no longer check for a NULL
	// VertexDeclaration. This had been necessary because of a Xenon XDK bug, but
	// the bug was fixed for the April XDK.
	// We also now check to see if a set of Presentation Parameters have been cached.
	// If so, we load our own set, and will restore the orignals later.

	void CDisplay::Begin(CVince* vince)
	{
		// Attempt to create the State Block each time.
		if ( D3D_OK == m_pd3dDevice->CreateStateBlock(D3DSBT_ALL, &m_StateBlock) )
		{
			// Capture current state
			m_StateBlock->Capture();
		}
		else
		{
			// log error and exit
			m_StateBlock = NULL;
			vince->pLogWriter->WriteError("Display", "Begin", "Problem saving State Block");
			return;
		}

		SaveClipPlanes();
		PreparePresentation();
	}

	// Restore previous state block
	void CDisplay::End(CVince* vince)
	{
		RestorePresentation();
		if ( NULL != m_StateBlock )
		{
			if ( D3D_OK != m_StateBlock->Apply() )
			{
				// log error
				vince->pLogWriter->WriteError("Display", "End", "Problem Restoring State Block");
			}
			// Since we are recreating each time, release and deallocate
			if ( D3D_OK != m_StateBlock->Release() )
			{
				// log error
				vince->pLogWriter->WriteError("Display", "End", "Problem Releasing State Block");
			}
			m_StateBlock = NULL;
		}
		RestoreClipPlanes();
	}

	// Set up presentation parameters that are compatible with VINCE surveys
	void CDisplay::PreparePresentation()
	{
		// Only do this if a set of game parameters have been cached
		if ( NULL != m_pPresentationParameters)
		{
			D3DPRESENT_PARAMETERS ppTemp = 
			{
				640,                // BackBufferWidth;
				480,                // BackBufferHeight;
				D3DFMT_A8R8G8B8,    // BackBufferFormat;
				1,                  // BackBufferCount;
				D3DMULTISAMPLE_NONE,// MultiSampleType;
				0,                  // MultiSampleQuality;
				D3DSWAPEFFECT_DISCARD, // SwapEffect;
				NULL,               // hDeviceWindow;
				FALSE,              // Windowed;
				TRUE,               // EnableAutoDepthStencil;
				D3DFMT_D24S8,       // AutoDepthStencilFormat;
				0,                  // Flags;
				0,                  // FullScreen_RefreshRateInHz;
				D3DPRESENT_INTERVAL_IMMEDIATE, // FullScreen_PresentationInterval;
			};
			m_pd3dDevice->Reset( &ppTemp);
		}
	}

	// Restore the game's presentation parameters that were cached
	void CDisplay::RestorePresentation()
	{
		// Only do this if a set of game parameters have been cached
		if ( NULL != m_pPresentationParameters)
		{
			m_pd3dDevice->Reset( m_pPresentationParameters );
		}
	}

	// Save current depth clip planes and reset to make sure text shows up
	void CDisplay::SaveClipPlanes()
	{
		// Clip planes are handled completely differently in DX9 than in DX8,
		// so for the time being, we do nothing here in the Xenon version
	}

	// Restore depth clip planes to original state
	void CDisplay::RestoreClipPlanes()
	{
		// Clip planes are handled completely differently in DX9 than in DX8,
		// so for the time being, we do nothing here in the Xenon version
	}

	// Clear the screen
	void CDisplay::Clear( DWORD dwBackgroundColor )
	{
		// Simple display clear. However, whether we can clear the Z-Buffer or not
		// Depends on whether it exists or is enabled, so we check for that.

		DWORD dwZEnabled;
		DWORD dwFlags = D3DCLEAR_TARGET;

		m_pd3dDevice->GetRenderState(D3DRS_ZENABLE, &dwZEnabled);
		if (dwZEnabled)
		{
			dwFlags |= D3DCLEAR_ZBUFFER;
		}
		m_pd3dDevice->Clear(0L, NULL, dwFlags, dwBackgroundColor, 1.0f, 0L);
	}

	// Finds named font or loads new font
	int CDisplay::LoadFont(CVince* vince, const char* cstrFontName)
	{

		// First we see if this font has already been loaded.
		for ( int i = 0; i < m_NumFonts; i++ )
		{
			if ( 0 == stricmp(cstrFontName, m_FontName[i]) )
			{
				return i;
			}
		}
		
		// Make sure we have room for another font
		if ( m_NumFonts >= MAX_FONTS )
		{
			vince->pLogWriter->WriteError("Display", "LoadFont", "Maximum number of fonts exceeded");
		}
		else
		{
			m_Font[m_NumFonts] = new CVinceFont();

			if ( NULL != m_Font[m_NumFonts] )
			{
				// Initialize the font.

				m_Font[m_NumFonts]->SetDevice(m_pd3dDevice);
				if ( S_OK == m_Font[m_NumFonts]->Create(cstrFontName) )
				{
					m_SelectedFont = m_NumFonts;
					m_FontName[m_NumFonts++] = SAFE_COPY(cstrFontName);
					return m_NumFonts - 1;
				}
				else
				{
					char buffer[120];
					_snprintf(buffer, 119, "Could not load requested font: %s", cstrFontName);
					buffer[119] = '\0';
					vince->pLogWriter->WriteError("Display", "LoadFont", buffer);
					SAFE_DELETE(m_Font[m_NumFonts]);
				}	// not S_OK
			}	// not NULL
		}	// not m_NumFonts >= MAX_FONTS

		// If we haven't returned yet, something went wrong.
		// Use another font if available; otherwise return -1
		if ( m_NumFonts < 1 )
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}


	void CDisplay::SetFont(int intFontNumber)
	{
		m_SelectedFont = intFontNumber;
	}


	// Prepare to write text
	void CDisplay::BeginText()
	{
		m_Font[m_SelectedFont]->Begin();
	}

	// Set horizontal and vertical font scales
	void CDisplay::ScaleFont(float xScale, float yScale)
	{
		m_Font[m_SelectedFont]->SetScaleFactors(xScale, yScale);
	}

	// Return width of string in screen units.
	float CDisplay::GetTextWidth( const WCHAR* wstrText )
	{
		return m_Font[m_SelectedFont]->GetTextWidth( wstrText );
	}

	// Render text to screen
	void CDisplay::DrawText(float xCenter, float yCenter, DWORD dwColor, const WCHAR* wstrText)
	{
		m_Font[m_SelectedFont]->DrawText( xCenter,  yCenter, dwColor, wstrText,
										  FONT_CENTER_X + FONT_CENTER_Y );
	}

	// Draw box on screen
	void CDisplay::DrawBox(float xLeft, float yTop, float xRight, float yBottom, DWORD dwOutlineColor, DWORD dwFillColor)
	{
		m_Font[m_SelectedFont]->DrawBox( xLeft,  yTop, xRight, yBottom,
										  dwOutlineColor, dwFillColor );
	}

	// Render survey background to screen
	void CDisplay::ShowBackground(const char* cstrImageFileName)
	{
// Image size can be reduced by excluding this code from the build if VINCE
// surveys do not use background images. Behavior should degrade gracefully
// and simply not show backgrounds, but produce no errors.
#ifndef NO_VINCE_BACKGROUNDS

		// There is a lot of work required here for Xenon. For now, we will
		// do everything on each call, but we may want to look at the best
		// way to initialize some of this overhead only once.

		LPD3DXBUFFER pShaderCode = NULL;
		LPD3DXBUFFER pErrorMsg = NULL;
		LPDIRECT3DVERTEXSHADER9 pVertexShader = NULL;
		LPDIRECT3DPIXELSHADER9 pPixelShader = NULL;
		LPDIRECT3DVERTEXDECLARATION9 pDeclaration = NULL;

		HRESULT hr = 0;

		// First step is compiling the shader code and creating the shaders
		// ----------------------------------------------------------------
		if( SUCCEEDED( D3DXCompileShader( g_strVertexShaderProgram, 
										sizeof( g_strVertexShaderProgram ),
										NULL, 
										NULL, 
										"main", 
										"vs_2_0", 
										0, 
										&pShaderCode, 
										&pErrorMsg, 
										NULL ) ) )
		{
			// Create vertex shader.
			hr = m_pd3dDevice->CreateVertexShader( (DWORD*)pShaderCode->GetBufferPointer(), 
                                        &pVertexShader );
		}
		// Clean up
		if( pShaderCode )
			pShaderCode->Release();
		pShaderCode = NULL;
		if( pErrorMsg )
			pErrorMsg->Release();
		pErrorMsg = NULL;

		if( SUCCEEDED( hr ) )
		{
			if( SUCCEEDED( hr = D3DXCompileShader( g_strPixelShaderProgram, 
									sizeof( g_strPixelShaderProgram ),
									NULL, 
									NULL, 
									"main", 
									"ps_2_0", 
									0, 
									&pShaderCode, 
									&pErrorMsg,
									NULL ) ) )
			{
				// Create pixel shader.
				hr = m_pd3dDevice->CreatePixelShader( (DWORD*)pShaderCode->GetBufferPointer(), 
												&pPixelShader );
			}
			// Clean up
			if( pShaderCode )
				pShaderCode->Release();
			pShaderCode = NULL;
			if( pErrorMsg )
				pErrorMsg->Release();
			pErrorMsg = NULL;
		}

		// Next step is to define the vertex elements and
		// Create a vertex declaration from the element descriptions.
		// ----------------------------------------------------------
		if( SUCCEEDED( hr ) )
		{
			static const D3DVERTEXELEMENT9 VertexElements[] =
			{
				{ 0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
				{ 0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
				D3DDECL_END()
			};
			hr = m_pd3dDevice->CreateVertexDeclaration( VertexElements, &pDeclaration );
		}

		// Now we load the texture to the file and render it.
		if( SUCCEEDED( hr ) )
		{
			LPDIRECT3DTEXTURE9 pTexture = NULL;
			if( SUCCEEDED( D3DXCreateTextureFromFileExA( m_pd3dDevice,
														cstrImageFileName,
														D3DX_DEFAULT,
														D3DX_DEFAULT,
														1,
														0,
														D3DFMT_UNKNOWN,
														D3DPOOL_MANAGED,
														D3DX_FILTER_POINT,
														D3DX_FILTER_POINT,
														0,
														NULL,
														NULL,
														&pTexture ) ) )
			{
				m_pd3dDevice->SetTexture( 0, pTexture );
				m_pd3dDevice->SetVertexShader( pVertexShader );
				m_pd3dDevice->SetPixelShader( pPixelShader );
				m_pd3dDevice->SetVertexDeclaration( pDeclaration );
	            
				// An awful lot of state setting needs to be done, because
				// the application could have changed states (eg, the state is unknown)
				// We will make these settings for starters

				// Set "Safe" render states...
				m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
				m_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
				m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
				m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
				m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
				m_pd3dDevice->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL );

				// Some additional settings may be required, e.g.
				// - set "Safe" Texture Stage States
				// - set "Safe" Sampler States
				// ... etc

				m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, verts, sizeof( SB_VERTEX ) );
				m_pd3dDevice->SetTexture( 0, NULL );
				pTexture->Release();        
			}
		}

		// Final clean up
		m_pd3dDevice->SetVertexDeclaration( NULL );
		if( pDeclaration )
			pDeclaration->Release();

		m_pd3dDevice->SetVertexShader( NULL );
		if( pVertexShader )
			pVertexShader->Release();

		m_pd3dDevice->SetPixelShader( NULL );
		if( pPixelShader )
			pPixelShader->Release();
#endif // NO_VINCE_BACKGROUNDS
	}

	// Wrap up text output
	void CDisplay::EndText()
	{
		m_Font[m_SelectedFont]->End();
	}

	void CDisplay::Present()
	{
		// The standard display presentation may not work, depending on how the game is using the back buffers.
		// In such cases, it may be necessary to customize this function, such as in the following code from
		// PGR3. However, this customization is also difficult to generalize and requires additional game specific
		// classes, name spaces and header files. It is included only for illustrative purposes.
		//m_pd3dDevice->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, XBoxApp::GetFrontBuffer(), NULL, 0, 0, NULL, 1.0f, 0L, NULL );
		//IDirect3DBaseTexture9 *pFrontBuffer;
		//m_pd3dDevice->GetFrontBuffer(&pFrontBuffer);
		//m_pd3dDevice->SynchronizeToPresentationInterval();
		//m_pd3dDevice->Swap(pFrontBuffer, NULL);
		m_pd3dDevice->Present(0, 0, 0, 0);
	}

}

#endif // !NO_VINCE_SURVEYS
#endif // _VINCE_