//--------------------------------------------------------------------------------------
// AtgFont.cpp
//
// Font class for samples. For details, see header.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "xgameRender.h"
#include "AtgFont.h"
#include "renderDraw.h"
#include "renderThread.h"
#include "BD3D.h"
#include "deviceStateDumper.h"

#include "AtgFontVertexShader.hlsl.h"
#include "AtgFontPixelShader.hlsl.h"

namespace ATG
{


static D3DVertexDeclaration* g_pFontVertexDecl   = NULL;
static D3DVertexShader*      g_pFontVertexShader = NULL;
static D3DPixelShader*       g_pFontPixelShader  = NULL;


//--------------------------------------------------------------------------------------
// Name: CreateFontShaders()
// Desc: Creates the global font shaders
//--------------------------------------------------------------------------------------
static HRESULT CreateFontShaders()
{
    // Create vertex declaration
    if( NULL == g_pFontVertexDecl )
    {   
        D3DVERTEXELEMENT9 decl[] = 
        {
            { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };

        if( FAILED( gRenderDraw.createVertexDeclaration( decl, &g_pFontVertexDecl ) ) )
            return E_FAIL;
    }
    else
    {
        g_pFontVertexDecl->AddRef();
    }

    // Create vertex shader
    if( NULL == g_pFontVertexShader )
    {
        if( FAILED( gRenderDraw.createVertexShader( g_xvs_AtgFontVertexShader, &g_pFontVertexShader ) ) )
            return E_FAIL;
    }
    else
    {
        g_pFontVertexShader->AddRef();
    }
    
    // Create pixel shader.
    if( NULL == g_pFontPixelShader )
    {
        if( FAILED( gRenderDraw.createPixelShader( g_xps_AtgFontPixelShader, &g_pFontPixelShader ) ) )
            return E_FAIL;
    }
    else
    {
        g_pFontPixelShader->AddRef();
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Font()
// Desc: Constructor
//--------------------------------------------------------------------------------------
Font::Font()
{
    m_pFontTexture       = NULL;

    m_dwNumGlyphs        = 0L;
    m_Glyphs             = NULL;

    m_state.m_fXScaleFactor = 1.0f;
    m_state.m_fYScaleFactor = 1.0f;
    m_state.m_fSlantFactor  = 0.0f;

    m_cMaxGlyph          = 0;
    m_TranslatorTable    = NULL;
    
    m_dwNestedBeginCount = 0L;
}


//--------------------------------------------------------------------------------------
// Name: ~Font()
// Desc: Destructor
//--------------------------------------------------------------------------------------
Font::~Font()
{
    Destroy();
}


//--------------------------------------------------------------------------------------
// Name: Create()
// Desc: Create the font's internal objects (texture and array of glyph info)
//       using the XPR packed resource file
//--------------------------------------------------------------------------------------
HRESULT Font::Create( long dirID, const CHAR* strFontFileName )
{
    // Create the font
    if( FAILED( m_xprResource.Create( dirID, strFontFileName ) ) )
        return E_FAIL;

    return Create( m_xprResource.GetTexture("FontTexture"),
                   m_xprResource.GetData("FontData") );
}


//--------------------------------------------------------------------------------------
// Name: Create()
// Desc: Create the font's internal objects (texture and array of glyph info)
//--------------------------------------------------------------------------------------
HRESULT Font::Create( IDirect3DTexture9* pFontTexture, const VOID* pFontData )
{
    BDEBUG_ASSERT(!m_pFontTexture);
    
    // Save a copy of the texture
    m_pFontTexture = pFontTexture;

    // Check version of file (to make sure it matches up with the FontMaker tool)
    const BYTE* pData = (BYTE*)pFontData;
    DWORD dwFileVersion = *((DWORD*)pData); pData += sizeof(DWORD);
    
    if( dwFileVersion == 0x00000005 )
    {
        // Parse the font data
        m_fFontHeight        = *((FLOAT*)pData); pData += sizeof(FLOAT);
        m_fFontTopPadding    = *((FLOAT*)pData); pData += sizeof(FLOAT);
        m_fFontBottomPadding = *((FLOAT*)pData); pData += sizeof(FLOAT);
        m_fFontYAdvance      = *((FLOAT*)pData); pData += sizeof(FLOAT);

        // Point to the translator string
        m_cMaxGlyph       = ((WORD*)pData)[0];   pData += sizeof(WORD);
        m_TranslatorTable = (SHORT*)pData;       pData += sizeof(WCHAR)*(m_cMaxGlyph+1);

        // Read the glyph attributes from the file
        m_dwNumGlyphs = ((DWORD*)pData)[0];  pData += sizeof(DWORD);
        m_Glyphs      = (GLYPH_ATTR*)pData;
    }
    else
    {
        tracenocrlf("Incorrect version number on font file!\n" );
        return E_FAIL;
    }

    // Create the vertex and pixel shaders for rendering the font
    if( FAILED( CreateFontShaders() ) )
    {
        tracenocrlf("Could not create font shaders!\n" );
        return E_FAIL;
    }

    // Initialize the window
    const D3DDISPLAYMODE& DisplayMode = gRenderDraw.getDisplayMode();
    m_state.m_rcWindow.x1 = 0;
    m_state.m_rcWindow.y1 = 0;
    m_state.m_rcWindow.x2 = DisplayMode.Width;
    m_state.m_rcWindow.y2 = DisplayMode.Height;
    
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Destroy()
// Desc: Destroy the font object
//--------------------------------------------------------------------------------------
VOID Font::Destroy()
{
   gRenderThread.blockUntilGPUIdle();

   m_pFontTexture       = NULL;
   m_dwNumGlyphs        = 0L;
   m_Glyphs             = NULL;
   m_cMaxGlyph          = 0;
   m_TranslatorTable    = NULL;
   m_dwNestedBeginCount = 0L;

   // Ask the worker thread to release these resources.
   gRenderDraw.releaseD3DResource(g_pFontVertexDecl, true);
   gRenderDraw.releaseD3DResource(g_pFontVertexShader, true);
   gRenderDraw.releaseD3DResource(g_pFontPixelShader, true);
        
   g_pFontVertexDecl = NULL;
   g_pFontVertexShader = NULL;
   g_pFontPixelShader = NULL;
}


//--------------------------------------------------------------------------------------
// Name: SetWindow()
// Desc: Sets the cursor position for drawing text
//--------------------------------------------------------------------------------------
VOID Font::SetWindow( D3DRECT rcWindow )
{
    SetWindow( rcWindow.x1, rcWindow.y1, rcWindow.x2, rcWindow.y2 );
}


//--------------------------------------------------------------------------------------
// Name: SetWindow()
// Desc: Sets the cursor position for drawing text
//--------------------------------------------------------------------------------------
VOID Font::SetWindow( LONG x1, LONG y1, LONG x2, LONG y2 )
{
    m_state.m_rcWindow.x1 = x1;
    m_state.m_rcWindow.y1 = y1;
    m_state.m_rcWindow.x2 = x2;
    m_state.m_rcWindow.y2 = y2;
}

//--------------------------------------------------------------------------------------
// Name: SetScaleFactors()
// Desc: Sets X and Y scale factor to make rendered text bigger or smaller.
//       Note that since text is pre-anti-aliased and therefore point-filtered,
//       any scale factors besides 1.0f will degrade the quality.
//--------------------------------------------------------------------------------------
VOID Font::SetScaleFactors( FLOAT fXScaleFactor, FLOAT fYScaleFactor )
{
    m_state.m_fXScaleFactor = fXScaleFactor;
    m_state.m_fYScaleFactor = fYScaleFactor;
}


//--------------------------------------------------------------------------------------
// Name: SetSlantFactor()
// Desc: Sets the slant factor for rendering slanted text.
//--------------------------------------------------------------------------------------
VOID Font::SetSlantFactor( FLOAT fSlantFactor )
{
    m_state.m_fSlantFactor = fSlantFactor;
}


//--------------------------------------------------------------------------------------
// Name: GetTextExtent()
// Desc: Get the dimensions of a text string
//--------------------------------------------------------------------------------------
VOID Font::GetTextExtent( const WCHAR* strText, FLOAT* pWidth, 
                              FLOAT* pHeight, BOOL bFirstLineOnly ) const
{
    BASSERT( strText != NULL );
    BASSERT( pWidth != NULL );
    BASSERT( pHeight != NULL );

    // Set default text extent in output parameters
    *pWidth   = 0.0f;
    *pHeight  = 0.0f;

    // Initialize counters that keep track of text extent
    FLOAT sx = 0.0f;
    FLOAT sy = m_fFontHeight;

    // Loop through each character and update text extent
    while( *strText )
    {
        WCHAR letter = *strText++;
    
        // Handle newline character
        if( letter == L'\n' )
        {
            if( bFirstLineOnly )
                break;
            sx  = 0.0f;
            sy += m_fFontYAdvance;
        }

        // Handle carriage return characters by ignoring them. This helps when
        // displaying text from a file.
        if( letter == L'\r' )
            continue;

        // Translate unprintable characters
        GLYPH_ATTR* pGlyph;
        if( letter > m_cMaxGlyph || m_TranslatorTable[letter] == 0 )
            pGlyph = &m_Glyphs[0];
        else
            pGlyph = &m_Glyphs[m_TranslatorTable[letter]];

        // Get text extent for this character's glyph
        sx += pGlyph->wOffset;
        sx += pGlyph->wAdvance;

        // Store text extent of string in output parameters
        if( sx > (*pWidth) )
            *pWidth = sx;
        if( sy > (*pHeight) )
            *pHeight = sy;
     }

    // Apply the scale factor to the result
    (*pWidth)  *= m_state.m_fXScaleFactor;
    (*pHeight) *= m_state.m_fYScaleFactor;
}


//--------------------------------------------------------------------------------------
// Name: GetTextWidth()
// Desc: Returns the width in pixels of a text string
//--------------------------------------------------------------------------------------
FLOAT Font::GetTextWidth( const WCHAR* strText ) const
{
    FLOAT fTextWidth  = 0.0f;
    FLOAT fTextHeight = 0.0f;
    GetTextExtent( strText, &fTextWidth, &fTextHeight );
    return fTextWidth;
}

//--------------------------------------------------------------------------------------
// Anonymous namespace
//--------------------------------------------------------------------------------------
namespace
{
   struct BFontRenderData 
   {
      Font*                m_pFont;
      FLOAT                m_fOriginX;
      FLOAT                m_fOriginY;
      DWORD                m_dwColor;
      const WCHAR*         m_StrText;
      DWORD                m_dwFlags;
      FLOAT                m_fMaxPixelWidth;
      Font::BDynamicState  m_State;
   };
   
} // anonymous namespace

//--------------------------------------------------------------------------------------
// Name: Font::WorkerFontBegin()
//--------------------------------------------------------------------------------------
void Font::WorkerFontBegin(void* pData)
{
   Font* pFont = (Font*)(pData);

   // Set render state
   BD3D::mpDev->SetTexture( 0, pFont->m_pFontTexture );

   BD3D::mpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
   BD3D::mpDev->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_SRCALPHA );
   BD3D::mpDev->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_INVSRCALPHA );

   BD3D::mpDev->SetRenderState( D3DRS_ALPHATESTENABLE,  TRUE );
   BD3D::mpDev->SetRenderState( D3DRS_ALPHAREF,         0x08 );
   BD3D::mpDev->SetRenderState( D3DRS_ALPHAFUNC,        D3DCMP_GREATEREQUAL );
   BD3D::mpDev->SetRenderState( D3DRS_FILLMODE,         D3DFILL_SOLID );
   BD3D::mpDev->SetRenderState( D3DRS_CULLMODE,         D3DCULL_CCW );
   BD3D::mpDev->SetRenderState( D3DRS_ZENABLE,          FALSE );
   BD3D::mpDev->SetRenderState( D3DRS_STENCILENABLE,    FALSE );
   BD3D::mpDev->SetRenderState( D3DRS_VIEWPORTENABLE,   FALSE );
   BD3D::mpDev->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
   BD3D::mpDev->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
   BD3D::mpDev->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
   BD3D::mpDev->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

   BD3D::mpDev->SetVertexDeclaration( g_pFontVertexDecl );
   BD3D::mpDev->SetVertexShader( g_pFontVertexShader );
   BD3D::mpDev->SetPixelShader( g_pFontPixelShader );

   // Set the texture scaling factor as a vertex shader constant
   D3DSURFACE_DESC TextureDesc;
   pFont->m_pFontTexture->GetLevelDesc( 0, &TextureDesc );
   FLOAT vTexScale[4];
   vTexScale[0] = 1.0f/TextureDesc.Width;
   vTexScale[1] = 1.0f/TextureDesc.Height;
   vTexScale[2] = 0.0f;
   vTexScale[3] = 0.0f;
   BD3D::mpDev->SetVertexShaderConstantF( 2, vTexScale, 1 );
}

//--------------------------------------------------------------------------------------
// Name: Font::WorkerFontEnd()
//--------------------------------------------------------------------------------------
void Font::WorkerFontEnd(void* pData)
{
   Font* pFont = (Font*)(pData);
   pFont;

   BD3D::mpDev->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
   BD3D::mpDev->SetRenderState( D3DRS_SRCBLEND,         D3DBLEND_ONE);
   BD3D::mpDev->SetRenderState( D3DRS_DESTBLEND,        D3DBLEND_ZERO);

   BD3D::mpDev->SetRenderState( D3DRS_ALPHATESTENABLE,  FALSE );
   BD3D::mpDev->SetRenderState( D3DRS_ALPHAREF,         0 );
   BD3D::mpDev->SetRenderState( D3DRS_ALPHAFUNC,        D3DCMP_ALWAYS );
   BD3D::mpDev->SetRenderState( D3DRS_ZENABLE,          TRUE );
   BD3D::mpDev->SetRenderState( D3DRS_VIEWPORTENABLE,   TRUE );

   BD3D::mpDev->SetTexture( 0, NULL );   
}

//--------------------------------------------------------------------------------------
// Name: Begin()
// Desc: Prepares the font vertex buffers for rendering.
//--------------------------------------------------------------------------------------
VOID Font::Begin()
{
   // Set state on the first call
   if( 0 == m_dwNestedBeginCount )
   {
      if (!gRenderThread.isSimThread())
         WorkerFontBegin(this);
      else
         gRenderThread.submitCallback(WorkerFontBegin, this);
   }      

   // Keep track of the nested begin/end calls.
   m_dwNestedBeginCount++;
}

//--------------------------------------------------------------------------------------
// Name: DrawText()
// Desc: Draws text as textured polygons
//--------------------------------------------------------------------------------------
VOID Font::DrawText(FLOAT fOriginX, FLOAT fOriginY, DWORD dwColor,
                    const WCHAR* strText, DWORD dwFlags,
                    FLOAT fMaxPixelWidth )
{
   if (!m_pFontTexture)
      return;
      
   if (!strText)
      return;
      
   const uint len = wcslen(strText);
   if (!len)
      return;
      
   Begin();
   
   if (!gRenderThread.isSimThread())
   {
      BFontRenderData renderData;
      renderData.m_pFont            = this;
      renderData.m_fOriginX         = fOriginX;
      renderData.m_fOriginY         = fOriginY;
      renderData.m_dwColor          = dwColor;
      renderData.m_StrText          = strText;
      renderData.m_dwFlags          = dwFlags;
      renderData.m_fMaxPixelWidth   = fMaxPixelWidth;
      renderData.m_State            = m_state;
      
      WorkerDrawText(&renderData);
   }
   else
   {
      const uint bufLen = (len + 1) * sizeof(WCHAR);
      WCHAR* pBuf = static_cast<WCHAR*>(gRenderThread.allocateFrameStorage(bufLen));
      memcpy(pBuf, strText, bufLen);
            
      BFontRenderData* pDst = reinterpret_cast<BFontRenderData *>(
         gRenderThread.submitCommandBegin(cRCCCommandCallbackWithData, reinterpret_cast<DWORD>(WorkerDrawTextCallback), sizeof(BFontRenderData), 4) );

      pDst->m_pFont            = this;
      pDst->m_fOriginX         = fOriginX;
      pDst->m_fOriginY         = fOriginY;
      pDst->m_dwColor          = dwColor;
      pDst->m_StrText          = pBuf;
      pDst->m_dwFlags          = dwFlags;
      pDst->m_fMaxPixelWidth   = fMaxPixelWidth;
      pDst->m_State            = m_state;
      
      gRenderThread.submitCommandEnd(sizeof(BFontRenderData));
   }      

   End();
}                    

//--------------------------------------------------------------------------------------
// Name: WorkerDrawTextCallback
// Desc: 
//--------------------------------------------------------------------------------------
VOID Font::WorkerDrawTextCallback(const void* pData)
{
   const BFontRenderData* pFontRenderData = static_cast<const BFontRenderData*>(pData);
   return pFontRenderData->m_pFont->WorkerDrawText(pData);
}

//--------------------------------------------------------------------------------------
// Name: WorkerDrawText
// Desc: 
//--------------------------------------------------------------------------------------
VOID Font::WorkerDrawText(const void* pData)
{
   PIXBeginNamedEvent( 0xFFFFFFFF, "DrawText" );
   
   const BFontRenderData* pFontRenderData = static_cast<const BFontRenderData*>(pData);
   const Font::BDynamicState& dynamicState = pFontRenderData->m_State;
   const WCHAR* strText = pFontRenderData->m_StrText;
      
   float fCursorX = floorf( pFontRenderData->m_fOriginX );
   float fCursorY = floorf( pFontRenderData->m_fOriginY );
   
   // Set the color as a vertex shader constant
   FLOAT vColor[4];
   const DWORD dwColor = pFontRenderData->m_dwColor;
   vColor[0] = ((dwColor&0x00ff0000)>>16L)/255.0f;
   vColor[1] = ((dwColor&0x0000ff00)>> 8L)/255.0f;
   vColor[2] = ((dwColor&0x000000ff)>> 0L)/255.0f;
   vColor[3] = ((dwColor&0xff000000)>>24L)/255.0f;
   BD3D::mpDev->SetVertexShaderConstantF( 1, vColor, 1 );

   FLOAT fOriginX = pFontRenderData->m_fOriginX;
   FLOAT fOriginY = pFontRenderData->m_fOriginY;

   // Set the starting screen position
   if( ( fOriginX < 0.0f ) || ( ( pFontRenderData->m_dwFlags & ATGFONT_RIGHT ) && ( fOriginX <= 0.0f ) ) )
   {
      fOriginX += (dynamicState.m_rcWindow.x2 - dynamicState.m_rcWindow.x1);
   }
   if( fOriginY < 0.0f )
   {
      fOriginY += (dynamicState.m_rcWindow.y2 - dynamicState.m_rcWindow.y1);
   }

   // Adjust for padding
   fOriginY -= m_fFontTopPadding;

   FLOAT fEllipsesPixelWidth = dynamicState.m_fXScaleFactor * 3.0f * (m_Glyphs[m_TranslatorTable[L'.']].wOffset + m_Glyphs[m_TranslatorTable[L'.']].wAdvance);

   DWORD dwFlags = pFontRenderData->m_dwFlags;
   if( dwFlags & ATGFONT_TRUNCATED )
   {
      // Check if we will really need to truncate the string
      if( pFontRenderData->m_fMaxPixelWidth <= 0.0f )
      {
         dwFlags &= (~ATGFONT_TRUNCATED);
      }
      else
      {
         FLOAT w, h;
         GetTextExtent( strText, &w, &h, TRUE );

         // If not, then clear the flag
         if( w <= pFontRenderData->m_fMaxPixelWidth )
            dwFlags &= (~ATGFONT_TRUNCATED);
      }
   }

   // If vertically centered, offset the starting fCursorY value
   if( pFontRenderData->m_dwFlags & ATGFONT_CENTER_Y )
   {
      FLOAT w, h;
      GetTextExtent( strText, &w, &h );
      fCursorY = floorf( fCursorY - h/2 );
   }

   // Add window offsets
   fOriginX += dynamicState.m_rcWindow.x1;
   fOriginY += dynamicState.m_rcWindow.y1;
   fCursorX += dynamicState.m_rcWindow.x1;
   fCursorY += dynamicState.m_rcWindow.y1;

   // Set a flag so we can determine initial justification effects
   BOOL bStartingNewLine = TRUE;

   DWORD prevMask = 0xFFFFFFFF;

   while( *strText )
   {
      // If starting text on a new line, determine justification effects
      if( bStartingNewLine )
      {
         if( dwFlags & (ATGFONT_RIGHT|ATGFONT_CENTER_X) )
         {
            // Get the extent of this line
            FLOAT w, h;
            GetTextExtent( strText, &w, &h, TRUE );

            // Offset this line's starting fCursorX value
            if( dwFlags & ATGFONT_RIGHT )
               fCursorX = floorf( fOriginX - w );
            if( dwFlags & ATGFONT_CENTER_X )
               fCursorX = floorf( fOriginX - w/2 );
         }
         bStartingNewLine = FALSE;
      }

      // Get the current letter in the string
      WCHAR letter = *strText++;

      // Handle the newline character
      if( letter == L'\n' )
      {
         fCursorX  = fOriginX;
         fCursorY += m_fFontYAdvance * dynamicState.m_fYScaleFactor;
         bStartingNewLine = TRUE;
         continue;
      }

      // Handle carriage return characters by ignoring them. This helps when
      // displaying text from a file.
      if( letter == L'\r' )
         continue;

      // Translate unprintable characters
      GLYPH_ATTR* pGlyph = &m_Glyphs[ (letter<=m_cMaxGlyph) ? m_TranslatorTable[letter] : 0 ];

      FLOAT fOffset  = dynamicState.m_fXScaleFactor * (FLOAT)pGlyph->wOffset;
      FLOAT fAdvance = dynamicState.m_fXScaleFactor * (FLOAT)pGlyph->wAdvance;
      FLOAT fWidth   = dynamicState.m_fXScaleFactor * (FLOAT)pGlyph->wWidth;
      FLOAT fHeight  = dynamicState.m_fYScaleFactor * m_fFontHeight;

      if( dwFlags & ATGFONT_TRUNCATED )
      {
         // Check if we will be exceeded the max allowed width
         if( fCursorX + fOffset + fWidth + fEllipsesPixelWidth + dynamicState.m_fSlantFactor > fOriginX + pFontRenderData->m_fMaxPixelWidth )
         {
            break;  // Break out of the loop and exit.
         }
      }

      if (pGlyph->wMask != prevMask)
      {
         prevMask = pGlyph->wMask;

         // Select the mask
         FLOAT pShaderConsts[8] = 
         { 
            pGlyph->wMask&0x0f00?1.0f:0.0f,  
            pGlyph->wMask&0x00f0?1.0f:0.0f,  
            pGlyph->wMask&0x000f?1.0f:0.0f,  
            pGlyph->wMask&0xf000?1.0f:0.0f,

            pGlyph->wMask==0x0000?1.0f:0.0f, 
            pGlyph->wMask==0x0000?1.0f:0.0f, 
            pGlyph->wMask==0x0000?1.0f:0.0f, 
            pGlyph->wMask==0x0000?1.0f:0.0f
         };
         BD3D::mpDev->SetPixelShaderConstantF( 0, pShaderConsts, 2 );
      }           

      // Setup the screen coordinates
      fCursorX  += fOffset;
      FLOAT left1  = fCursorX;
      FLOAT left2  = left1 + dynamicState.m_fSlantFactor;
      FLOAT right1 = left1 + fWidth;
      FLOAT right2 = left2 + fWidth;
      FLOAT top    = fCursorY;
      FLOAT bottom = top + fHeight;
      fCursorX  += fAdvance;

      FLOAT v[4][4] = 
      {
         { left2,  top,    pGlyph->tu1, pGlyph->tv1 },
         { right2, top,    pGlyph->tu2, pGlyph->tv1 },
         { right1, bottom, pGlyph->tu2, pGlyph->tv2 },
         { left1,  bottom, pGlyph->tu1, pGlyph->tv2 },
      };

      BD3D::mpDev->DrawVerticesUP( D3DPT_QUADLIST, 4, v, sizeof(v[0]) );
   }
   
   // Close off the user-defined event opened with PIXBeginNamedEvent.
   PIXEndNamedEvent();   
}

//--------------------------------------------------------------------------------------
// Name: End()
// Desc: Paired call that restores state set in the Begin() call.
//--------------------------------------------------------------------------------------
VOID Font::End()
{
   BDEBUG_ASSERT( m_dwNestedBeginCount > 0 );
   if( --m_dwNestedBeginCount > 0 )
     return;
     
   if (!gRenderThread.isSimThread())
      WorkerFontEnd(this);
   else
      gRenderThread.submitCallback(WorkerFontEnd, this);
}

//--------------------------------------------------------------------------------------
// Name: CreateTexture()
// Desc: Creates a texture and renders a text string into it.
//--------------------------------------------------------------------------------------
LPDIRECT3DTEXTURE9 Font::CreateTexture( const WCHAR* strText, D3DCOLOR dwBackgroundColor, 
                                            D3DCOLOR dwTextColor, D3DFORMAT d3dFormat )
{
   // rg [1/14/06] - Fancy render target stuff isn't easily done from the main thread, yet.
   // rg [7/11/06] - We can easily do this now, but we don't seem to need this functionality.
   return NULL;
   
#if 0
    // Make sure the format is tiled (otherwise the Resolve will fail)
    if( FALSE == XGIsTiledFormat( d3dFormat ) )
    {
        tracenocrlf("Format must be tiled!\n" );
        return NULL;
    }

    // Calculate texture dimensions
    FLOAT fTexWidth;
    FLOAT fTexHeight;
    GetTextExtent( strText, &fTexWidth, &fTexHeight );
    DWORD dwWidth  = (DWORD)fTexWidth;
    DWORD dwHeight = (DWORD)fTexHeight;

    // Create a render target
    LPDIRECT3DSURFACE9 pNewRenderTarget;
    if( FAILED( gRenderDraw.createRenderTarget( dwWidth, dwHeight, d3dFormat, D3DMULTISAMPLE_NONE,
                                                  0L, FALSE, &pNewRenderTarget, NULL ) ) )
    {
        tracenocrlf("Could not create a render target for the font texture!\n" );
        return NULL;
    }
    
    // Create the texture
    LPDIRECT3DTEXTURE9 pNewTexture;
    if( FAILED( BRenderDevice::getDevice()->CreateTexture( dwWidth, dwHeight, 1, 0L, d3dFormat, 
                                             D3DPOOL_DEFAULT, &pNewTexture, NULL ) ) )
    {
        tracenocrlf("Could not create a font texture!\n" );
        gRenderDraw.releaseD3DResource(pNewRenderTarget);
        return NULL;
    }
    
    // Get the current backbuffer and zbuffer
    LPDIRECT3DSURFACE9 pOldRenderTarget;
    BRenderDevice::getDevice()->GetRenderTarget( 0, &pOldRenderTarget );

    // Set the new texture as the render target
    BRenderDevice::getDevice()->SetRenderTarget( 0, pNewRenderTarget );
    BRenderDevice::getDevice()->Clear( 0L, NULL, D3DCLEAR_TARGET, dwBackgroundColor, 1.0f, 0L );

    // Render the text
    SetWindow( 0, 0, dwWidth, dwHeight );
    DrawText( 0, 0, dwTextColor, strText, 0L );

    // Resolve to the texture
    BRenderDevice::getDevice()->Resolve( 0L, NULL, pNewTexture,  NULL, 0, 0, NULL, 0, 0, NULL );

    // Restore the render target
    BRenderDevice::getDevice()->SetRenderTarget( 0, pOldRenderTarget );
    pOldRenderTarget->Release();
    pNewRenderTarget->Release();

    // Return the new texture
    return pNewTexture;
#endif    
}

} // namespace ATG
