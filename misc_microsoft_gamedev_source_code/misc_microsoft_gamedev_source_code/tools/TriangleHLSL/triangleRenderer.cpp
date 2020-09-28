// File: triangleRenderer.cpp
#include "xcore.h"

#include <xtl.h>
#include <xboxmath.h>
#include <xgraphics.h>

// xcore
#include "xcore.h"
#include "Timer.h"

#include "triangleRenderer.h"

#include "renderDraw.h"

#include "fixedFuncShaders.h"

#include "renderThread.h"

const CHAR* g_strVertexShaderProgram = 
" float4x4 matWVP : register(c0);              "  
"                                              "  
" struct VS_IN                                 "  
" {                                            " 
"     float4 ObjPos   : POSITION;              "  // Object space position 
"     float4 Color    : COLOR;                 "  // Vertex color                 
"     float2 UV       : TEXCOORD0;             "
" };                                           " 
"                                              " 
" struct VS_OUT                                " 
" {                                            " 
"     float4 ProjPos  : POSITION;              "  // Projected space position 
"     float4 Color    : COLOR;                 " 
"     float2 UV       : TEXCOORD0;             "
" };                                           "  
"                                              "  
" VS_OUT main( VS_IN In )                      "  
" {                                            "  
"     VS_OUT Out;                              "  
"     Out.ProjPos = mul( matWVP, In.ObjPos );  "  // Transform vertex into
"     Out.Color = In.Color;                    "  // Projected space and 
"     Out.UV = In.UV;                          " 
"     return Out;                              "  // Transfer color
" }                                            ";

const CHAR* g_strPixelShaderProgram = 
" struct PS_IN                                 "
" {                                            "
"     float4 Color : COLOR;                    "  // Interpolated color from                      
"     float2 UV : TEXCOORD0;                   "
" };                                           "  // the vertex shader
" sampler gSampler0 : register(s0);            "  
" float4 main( PS_IN In ) : COLOR              "  
" {                                            "  
"     return tex2D(gSampler0, In.UV);          "  // Output color
" }                                            "; 

struct COLORVERTEX
{
   FLOAT       Position[3];
   DWORD       Color;
   FLOAT       UV[2];
};

BTriangleRenderer::BTriangleRenderer() :
   mpPixelShader(NULL),
   mpVertexShader(NULL),
   mpVertexDecl(NULL),
   mTexture(cInvalidTextureHandle)
{
}

BTriangleRenderer::~BTriangleRenderer()
{
}

bool BTriangleRenderer::init(void)
{
   if (mpVertexDecl)
      return true;
      
   // Buffers to hold compiled shaders and possible error messages
   ID3DXBuffer* pShaderCode = NULL;
   ID3DXBuffer* pErrorMsg = NULL;

   // Compile vertex shader.
   HRESULT hr = D3DXCompileShader( g_strVertexShaderProgram, (UINT)strlen( g_strVertexShaderProgram ),
      NULL, NULL, "main", "vs_2_0", 0,
      &pShaderCode, &pErrorMsg, NULL );
   if( FAILED(hr) )
   {
      BFAIL( pErrorMsg ? (CHAR*)pErrorMsg->GetBufferPointer() : "" );
   }

   // Create pixel shader.
   gRenderDraw.createVertexShader( (DWORD*)pShaderCode->GetBufferPointer(), &mpVertexShader );

   // Shader code is no longer required.
   pShaderCode->Release();
   pShaderCode = NULL;

   // Compile pixel shader.
   hr = D3DXCompileShader( g_strPixelShaderProgram, (UINT)strlen( g_strPixelShaderProgram ),
      NULL, NULL, "main", "ps_2_0", 0,
      &pShaderCode, &pErrorMsg, NULL );
   if( FAILED(hr) )
   {
      BFAIL( pErrorMsg ? (CHAR*)pErrorMsg->GetBufferPointer() : "" );
   }

   // Create pixel shader.
   gRenderDraw.createPixelShader( (DWORD*)pShaderCode->GetBufferPointer(),
      &mpPixelShader );

   // Shader code no longer required.
   pShaderCode->Release();
   pShaderCode = NULL;  

   // Define the vertex elements.
   D3DVERTEXELEMENT9 VertexElements[] =
   {
      { 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
      { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
      { 0, 16, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
      D3DDECL_END()
   };

   gRenderDraw.createVertexDeclaration( VertexElements, &mpVertexDecl );
   
   mTexture = gTextureManager.getOrCreate("blah", cTRTDynamic, cInvalidEventReceiverHandle, cDefaultTextureNormal, cThreadIndexSim);

   BDynamicTexture* pTexture = gTextureManager.getDynamic(mTexture);
   //pTexture->set(64, 64, D3DFMT_LE_A8R8G8B8, false, 4);
   pTexture->set(64, 64, D3DFMT_LIN_A8R8G8B8, true, 4);   
   pTexture->load(0);
   
   return true;
}

bool BTriangleRenderer::deinit(void)
{
   if (mpVertexShader)
   {
      gRenderDraw.releaseD3DResource(mpVertexShader);
      mpVertexShader = NULL;
   }
   
   if (mpPixelShader)
   {
      gRenderDraw.releaseD3DResource(mpPixelShader);
      mpPixelShader = NULL;
   }
   
   if (mpVertexDecl)
   {
      gRenderDraw.releaseD3DResource(mpVertexDecl);
      mpVertexDecl = NULL;
   }
   
   if (mTexture != cInvalidTextureHandle)
   {
      gTextureManager.unload(mTexture);
      mTexture = cInvalidTextureHandle;
   }
   
   return true;
}

void BTriangleRenderer::render(const void* pData)
{
   // World matrix (identity in this sample)
   XMMATRIX matWorld = XMMatrixIdentity();
   XMMATRIX matView  = XMLoadFloat4x4((const XMFLOAT4X4*)&gRenderDraw.getMainActiveMatrixTracker().getMatrix(cWorldToView, false));
   XMMATRIX matProj = XMLoadFloat4x4((const XMFLOAT4X4*)&gRenderDraw.getMainActiveMatrixTracker().getMatrix(cViewToProj, false));

   // World*view*projection
   XMMATRIX matWVP = matWorld * matView * matProj;
   XMMATRIX matWVPTransposed = XMMatrixTranspose(matWVP);

   const float* pPos = reinterpret_cast<const float*>(pData);

   COLORVERTEX Vertices[3] =
   {
      { 0.0f+pPos[0], 0.0f+pPos[1], 0.0f+pPos[2], 0x00FF0000, 0.0, 0.0f },
      {  0.0f+pPos[0], 10.0f+pPos[1], 0.0f+pPos[2], 0x0000FF00, 0.0f, 1.0f },
      {  10.0f+pPos[0], 10.0f+pPos[1], 0.0f+pPos[2], 0x000000FF, 1.0f, 1.0f }
   };

   //gRenderDraw.setVertexShader( mpVertexShader );
   //gRenderDraw.setPixelShader( mpPixelShader );
   
   //gRenderDraw.setVertexShader(gFixedFuncShaders.getVertexShader(cPosDiffuseTex1VS));
   //gRenderDraw.setPixelShader(gFixedFuncShaders.getPixelShader(cDiffuseTex1PS));
   
   gFixedFuncShaders.set(cPosDiffuseTex1VS, cDiffuseTex1PS);
      
   gRenderDraw.setVertexShaderConstantF( 0, (FLOAT*)&matWVPTransposed, 4 );

   gRenderDraw.setVertexDeclaration( mpVertexDecl );

   static int r = 0;
   r += 1;
   r &= 0xFF;
   
#if 0   
   const uint width = 64, height = 64;
   D3DFORMAT format = D3DFMT_A8R8G8B8;// & ~D3DDECLTYPE_ENDIAN_MASK;
         
   IDirect3DTexture9* pTexture = gRenderDraw.createDynamicTexture(width, height, format);
         
   D3DLOCKED_RECT rect;
   pTexture->LockRect(0, &rect, NULL, 0);
   
   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         DWORD* pPixel = (DWORD*)((uchar*)rect.pBits + sizeof(DWORD) * XGAddress2DTiledOffset(x, y, rect.Pitch >> 2, sizeof(DWORD)));
         uchar g, b;

         g = b  = ((x^y) & 1) ? 255 : 0;
         *pPixel = D3DCOLOR_ARGB(255, r, g, b);
      }
   }
   
   pTexture->UnlockRect(0);
#endif

   static int frame;
   static int p;
   //if ((frame % 50) == 0)
   {
      p++;
      
      DWORD buf[64*64];
      
      BDynamicTexture* pTexture = (BDynamicTexture*)gTextureManager.get(mTexture);
      for (uint y = 0; y < pTexture->getHeight(); y++)
      {
         for (uint x = 0; x < pTexture->getWidth(); x++)
         {
            uchar g, b;

            g = b = ((x^y^p) & 1) ? 255 : 0;
            buf[x+64*y] = D3DCOLOR_ARGB(255, r, g, b);
         }
      }
     
      bool succeeded = pTexture->update(buf, pTexture->getWidth()*sizeof(DWORD), false);
      if (!succeeded)
      {
         BTimer timer;
         timer.start();
         pTexture->update(buf, pTexture->getWidth()*sizeof(DWORD), true);
         timer.stop();
         trace("Blocked for %1.3f", timer.getElapsedSeconds() * 1000.0f);
      }
   }      
   
   frame++;      
      
   gRenderDraw.setTextureByHandle(0, mTexture);

   //gRenderThread.dumpState();
   //gRenderThread.breakPoint();
   
   gRenderDraw.drawVerticesUP( D3DPT_TRIANGLELIST, 3, Vertices, sizeof(COLORVERTEX) );
   
   gRenderDraw.setTexture(0, NULL);
}

