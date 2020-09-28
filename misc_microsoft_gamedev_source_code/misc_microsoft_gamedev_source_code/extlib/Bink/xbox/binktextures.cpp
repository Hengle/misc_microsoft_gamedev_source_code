#include "xgameRender.h"

#include <xtl.h>

#include <xgraphics.h>
#include "binktextures.h"

#include "PS_YCrCbAToRGBA.hlsl.h"
#include "PS_YCrCbToRGBNoPixelAlpha.hlsl.h"
#include "VS_PositionAndTexCoordPassThrough.hlsl.h"

//
// pointers to our local vertex and pixel shader
//

static D3DPixelShader  * YCrCbToRGBNoPixelAlpha = 0;
static D3DPixelShader  * YCrCbAToRGBA = 0;
static D3DVertexShader * PositionAndTexCoordPassThrough = 0;


//
// structure and definition for uploading our texture verts
//

#define POS_TC_VERTEX_FVF ( D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2( 0 ) )
typedef struct POS_TC_VERTEX 
{
  F32 sx, sy, sz;  // Screen coordinates
  F32 tu, tv;      // Texture coordinates 
} POS_TC_VERTEX;



//############################################################################
//##                                                                        ##
//## Free the shaders that we use.                                          ##
//##                                                                        ##
//############################################################################

void Free_Bink_shaders( void )
{
  if ( YCrCbToRGBNoPixelAlpha )
  {
    YCrCbToRGBNoPixelAlpha->Release();
    YCrCbToRGBNoPixelAlpha = 0;
  }
  
  if ( YCrCbAToRGBA )
  {
    YCrCbAToRGBA->Release();
    YCrCbAToRGBA = 0;
  }
  
  if ( PositionAndTexCoordPassThrough )
  {
    PositionAndTexCoordPassThrough->Release();
    PositionAndTexCoordPassThrough = 0;
  }

}


//############################################################################
//##                                                                        ##
//## Create the three shaders that we use.                                  ##
//##                                                                        ##
//############################################################################

S32 Create_Bink_shaders( LPDIRECT3DDEVICE9 d3d_device )
{
  HRESULT hr;
  
  //
  // create a pixel shader that goes from YcRcB to RGB (without alpha)
  //
  
  if ( YCrCbToRGBNoPixelAlpha == 0 )
  {
    hr = d3d_device->CreatePixelShader( g_xps_PS_YCrCbToRGBNoPixelAlpha, &YCrCbToRGBNoPixelAlpha );

    if ( FAILED( hr ) )
    {
      Free_Bink_shaders( );
      return( 0 );
    }
  }
    
  //
  // create a pixel shader that goes from YcRcB to RGB with an alpha plane
  //
  
  if ( YCrCbAToRGBA == 0 )
  {
    hr = d3d_device->CreatePixelShader( g_xps_PS_YCrCbAToRGBA, &YCrCbAToRGBA );

    if ( FAILED( hr ) )
    {
      Free_Bink_shaders( );
      return( 0 );
    }
  }
    
  //
  // create a vertex shader that just passes the vertices straight through
  //
  
  if ( PositionAndTexCoordPassThrough == 0 )
  {
    hr = d3d_device->CreateVertexShader( g_xvs_VS_PositionAndTexCoordPassThrough, &PositionAndTexCoordPassThrough );

    if ( FAILED( hr ) )
    {
      Free_Bink_shaders( );
      return( 0 );
    }
  }

  return( 1 );
}


//############################################################################
//##                                                                        ##
//## Free the textures that we allocated.                                   ##
//##                                                                        ##
//############################################################################

void Free_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                         BINKTEXTURESET * set_textures )
{
  int i;
  BINKFRAMETEXTURES * bt;
    
  // free the texture memory and then the textures directly
  bt = set_textures->textures;
  for( i = 0 ; i < set_textures->bink_buffers.TotalFrames ; i++ )
  {
    if ( bt->Ytexture )
    {
      XPhysicalFree( set_textures->bink_buffers.Frames[ i ].YPlane.Buffer );
      delete bt->Ytexture;
    }
    if ( bt->cRtexture )
    {
      XPhysicalFree( set_textures->bink_buffers.Frames[ i ].cRPlane.Buffer );
      delete bt->cRtexture;
    }
    if ( bt->cBtexture )
    {
      XPhysicalFree( set_textures->bink_buffers.Frames[ i ].cBPlane.Buffer );
      delete bt->cBtexture;
    }
    if ( bt->Atexture )
    {
      XPhysicalFree( set_textures->bink_buffers.Frames[ i ].APlane.Buffer );
      delete bt->Atexture;
    }
    ++bt;
  }
}


//############################################################################
//##                                                                        ##
//## Create a texture while allocating the memory ourselves.                ##
//##                                                                        ##
//############################################################################

static S32 make_texture( U32 width, U32 height, D3DFORMAT format, 
                         LPDIRECT3DTEXTURE9 * out_texture, void ** out_ptr, U32 * out_pitch, U32 * out_size )
{
  DWORD size;
  IDirect3DTexture9 * texture;
  void * ptr;
  
  texture = new IDirect3DTexture9;
  if ( texture == 0 )
    return( 0 );

  size = XGSetTextureHeader( width, height, 1, D3DUSAGE_CPU_CACHED_MEMORY, format, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, texture, 0, 0 );
  
  size = ( ( size + GPU_TEXTURE_ALIGNMENT - 1 ) / GPU_TEXTURE_ALIGNMENT ) * GPU_TEXTURE_ALIGNMENT;
  
  //
  // Bink textures must be allocated in read-write cacheable memory!
  //
  
  ptr = XPhysicalAlloc( size, MAXULONG_PTR, GPU_TEXTURE_ALIGNMENT, PAGE_READWRITE );
  if ( ptr == 0 )
  {
    delete texture;
    return( 0 );
  }
  
  XGOffsetResourceAddress( texture, ptr );
  
  *out_pitch = width;
  *out_texture = texture;
  *out_ptr = ptr;
  *out_size = size;

  //
  // use lockrect to get the d3d pitch
  //
    
  D3DLOCKED_RECT lr;

   texture->LockRect( 0, &lr, 0, 0 );
  {
    *out_pitch = lr.Pitch;
     texture->UnlockRect( 0 );
  }
  
  return( 1 );
}


//############################################################################
//##                                                                        ##
//## Create 2 sets of textures for Bink to decompress into...               ##
//##                                                                        ##
//############################################################################

S32 Create_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                          BINKTEXTURESET * set_textures )
{
  int i;
  
  //
  // Create our textures
  //

  for( i = 0 ; i < set_textures->bink_buffers.TotalFrames ; i++ )
  {
    set_textures->textures[ i ].Ytexture = 0;
    set_textures->textures[ i ].cBtexture = 0;
    set_textures->textures[ i ].cRtexture = 0;
    set_textures->textures[ i ].Atexture = 0;

    if ( set_textures->bink_buffers.Frames[ i ].YPlane.Allocate )
    {
      // create Y plane
      if ( ! make_texture( set_textures->bink_buffers.YABufferWidth, 
                           set_textures->bink_buffers.YABufferHeight,
                           D3DFMT_LIN_A8,
                           &set_textures->textures[ i ].Ytexture, 
                           &set_textures->bink_buffers.Frames[ i ].YPlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].YPlane.BufferPitch, 
                           &set_textures->textures[ i ].Ysize ) )
        goto fail;
    }
                                              
    if ( set_textures->bink_buffers.Frames[ i ].cRPlane.Allocate )
    {
      // create cR plane
      if ( ! make_texture( set_textures->bink_buffers.cRcBBufferWidth, 
                           set_textures->bink_buffers.cRcBBufferHeight,
                           D3DFMT_LIN_A8,
                           &set_textures->textures[ i ].cRtexture, 
                           &set_textures->bink_buffers.Frames[ i ].cRPlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].cRPlane.BufferPitch, 
                           &set_textures->textures[ i ].cRsize ) )
        goto fail;
    }

    if ( set_textures->bink_buffers.Frames[ i ].cBPlane.Allocate )
    {
      // create cR plane
      if ( ! make_texture( set_textures->bink_buffers.cRcBBufferWidth, 
                           set_textures->bink_buffers.cRcBBufferHeight,
                           D3DFMT_LIN_A8,
                           &set_textures->textures[ i ].cBtexture, 
                           &set_textures->bink_buffers.Frames[ i ].cBPlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].cBPlane.BufferPitch, 
                           &set_textures->textures[ i ].cBsize ) )
        goto fail;
    }
    
    if ( set_textures->bink_buffers.Frames[ i ].APlane.Allocate )
    {
      // create alpha plane
      if ( ! make_texture( set_textures->bink_buffers.YABufferWidth, 
                           set_textures->bink_buffers.YABufferHeight,
                           D3DFMT_LIN_A8,
                           &set_textures->textures[ i ].Atexture, 
                           &set_textures->bink_buffers.Frames[ i ].APlane.Buffer, 
                           &set_textures->bink_buffers.Frames[ i ].APlane.BufferPitch, 
                           &set_textures->textures[ i ].Asize ) )
        goto fail;
    }  
  }
  
  return( 1 );
  
 fail:

  Free_Bink_textures( d3d_device, set_textures );
  return( 0 );
}


//############################################################################
//##                                                                        ##
//## Draw our textures onto the screen with our vertex and pixel shaders.   ##
//##                                                                        ##
//############################################################################

void Draw_Bink_textures( LPDIRECT3DDEVICE9 d3d_device,
                         BINKTEXTURESET * set_textures,
                         U32 width,
                         U32 height,
                         F32 x_offset,
                         F32 y_offset,
                         F32 x_scale,
                         F32 y_scale,
                         IDirect3DTexture9* pMaskTexture)
{
  SCOPEDSAMPLEID(BinkMovie, 0xFFFF0000);

  POS_TC_VERTEX vertices[ 4 ];
  int i;

  //
  // Turn on texture filtering and texture clamping
  //

  for( i = 0 ; i < 5 ; i++ )
  {
    d3d_device->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    d3d_device->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    d3d_device->SetSamplerState( i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
  }

  //
  // Select the texture.
  //

  d3d_device->SetTexture( 0, set_textures->textures[ set_textures->bink_buffers.FrameNum ].Ytexture );
  d3d_device->SetTexture( 1, set_textures->textures[ set_textures->bink_buffers.FrameNum ].cRtexture );
  d3d_device->SetTexture( 2, set_textures->textures[ set_textures->bink_buffers.FrameNum ].cBtexture );
  
  //
  // turn off Z buffering, culling, and projection (since we are drawing orthographically)
  //
  
  d3d_device->SetRenderState( D3DRS_ZENABLE, FALSE );
  d3d_device->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
  d3d_device->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  d3d_device->SetRenderState( D3DRS_VIEWPORTENABLE, 0 );
 
  //
  // set the vertex format and turn on our vertex shader
  //
  
  d3d_device->SetFVF( POS_TC_VERTEX_FVF );
  d3d_device->SetVertexShader( PositionAndTexCoordPassThrough );

  
  //
  // are we using an alpha plane? if so, turn on the 4th texture and set our pixel shader
  //
  
  if ( set_textures->textures[ set_textures->bink_buffers.FrameNum ].Atexture )
  {
    //
    // set the alpha texture
    //
    
    d3d_device->SetTexture( 3, set_textures->textures[ set_textures->bink_buffers.FrameNum ].Atexture );

    //
    // turn on our pixel shader
    //
  
    d3d_device->SetPixelShader( YCrCbAToRGBA );

    goto do_alpha;
  }
  else
  {
    //
    // turn on our pixel shader
    //
  
    d3d_device->SetPixelShader( YCrCbToRGBNoPixelAlpha );
  }

  // set the external mask texture
  BOOL bUseMaskTexture = 0;
  if (pMaskTexture)
  {
     bUseMaskTexture = 1;
     d3d_device->SetTexture(4, pMaskTexture);
  }

   d3d_device->SetPixelShaderConstantB(0, &bUseMaskTexture, 1);

  //
  // are we completely opaque or somewhat transparent?
  //
  
  if (!bUseMaskTexture)
  {
    d3d_device->SetRenderState( D3DRS_ALPHABLENDENABLE, 0 );
  }
  else
  {
   do_alpha:
    d3d_device->SetRenderState( D3DRS_ALPHABLENDENABLE, 1 );
    d3d_device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    d3d_device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
  }
  
  //
  // Setup up the vertices.
  //

  vertices[ 0 ].sx = x_offset;
  vertices[ 0 ].sy = y_offset;
  vertices[ 0 ].sz = 0.0F;
  vertices[ 0 ].tu = 0.0f;
  vertices[ 0 ].tv = 0.0f;
  vertices[ 1 ] = vertices[ 0 ];
  vertices[ 1 ].sx = x_offset + ( ( (F32)(S32) width ) * x_scale );
  vertices[ 1 ].tu = 1.0f;
  vertices[ 2 ] = vertices[0];
  vertices[ 2 ].sy = y_offset + ( ( (F32)(S32) height ) * y_scale );
  vertices[ 2 ].tv = 1.0f;
  vertices[ 3 ] = vertices[ 1 ];
  vertices[ 3 ].sy = vertices[ 2 ].sy;
  vertices[ 3 ].tv = 1.0f;

  //
  // Draw the vertices.
  //

  d3d_device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( vertices[ 0 ] ) );

  //
  // clear the textures
  //
  
  d3d_device->SetTexture( 0, 0 );
  d3d_device->SetTexture( 1, 0 );
  d3d_device->SetTexture( 2, 0 );

  if ( set_textures->textures[ set_textures->bink_buffers.FrameNum ].Atexture )
  {
    d3d_device->SetTexture( 3, 0 );
  }

  if (pMaskTexture)
     d3d_device->SetTexture( 4, 0 );
}


// make sure the GPU is done with the textures that we are about to write info
RADDEFFUNC void Wait_for_Bink_textures( BINKTEXTURESET * set_textures )
{
  S32 next;
  
  next = set_textures->bink_buffers.FrameNum + 1;
  if ( ( next >= set_textures->bink_buffers.TotalFrames ) || ( set_textures->textures[ next ].Ytexture == 0 ) )
    next = 0;

  if ( set_textures->textures[ next ].Ytexture )
  {
    //
    // block until the texture is ready
    //

    set_textures->textures[ next ].Ytexture->BlockUntilNotBusy();
  }
}
