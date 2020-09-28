//==============================================================================
//
// gpuDXTPack.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//
//==============================================================================
#include "xgameRender.h"
#include "gpuDXTPack.h"

// xgameRender
#include "render.h"
#include "vertexTypes.h"

// xrender
#include "renderDraw.h"

// xcore
#include "consoleOutput.h"

//==============================================================================
// Constants
//==============================================================================
const uint cGPUDXTPackMaxDim = 1024;

//==============================================================================
// Defines
//==============================================================================
#define FXL_EFFECT_FILENAME "dxtPack\\dxtPack.bin"

//==============================================================================
// Enums
//==============================================================================
enum eTechniques
{
   cTechniqueDXT1Pack,
   cTechniqueDXT5Pack,
   cTechniqueDXT5HPack,
   cTechniqueDXNPack,
   cTechniqueDXT1DSPack,
   cTechniqueDXT5DSPack,
   cTechniqueDXT5HDSPack,
   cTechniqueDXNDSPack,
   
   cNumTechniques
};

//==============================================================================
// BGPUDXTPack::BGPUDXTPack
//==============================================================================
BGPUDXTPack::BGPUDXTPack() :
   mMaxDim(0),
   mpVertexBuffers(NULL)
{
   ASSERT_RENDER_THREAD
   
   init(cGPUDXTPackMaxDim);
}

//==============================================================================
// BGPUDXTPack::~BGPUDXTPack
//==============================================================================
BGPUDXTPack::~BGPUDXTPack()
{
   ASSERT_RENDER_THREAD
   
   deinit();
}

//==============================================================================
// BGPUDXTPack::init
//==============================================================================
void BGPUDXTPack::init(uint maxDim)
{
   ASSERT_RENDER_THREAD
   
   BDEBUG_ASSERT(maxDim >= 32 && Math::IsPow2(maxDim));
   
   deinit();
   
   mMaxDim = maxDim;

#ifdef ENABLE_RELOAD_MANAGER
   mFileWatcher.clear();
   mFileWatcher.add(gRender.getEffectCompilerDefaultDirID(), FXL_EFFECT_FILENAME);
#endif

   bool success = mAsyncFileLoader.load(gRender.getEffectCompilerDefaultDirID(), FXL_EFFECT_FILENAME, false, true);
   BVERIFY(success);
   
   initVertexBuffers();
}

//==============================================================================
// BGPUDXTPack::initVertexBuffers
//==============================================================================
void BGPUDXTPack::initVertexBuffers(void)
{
   uint totalBytesToAlloc = 0;
   
   uint curDim = mMaxDim;
   do
   {
      totalBytesToAlloc += (curDim >> 2) * (curDim >> 2) * sizeof(DWORD);
      curDim >>= 1;             
   } while (curDim >= 32);
   
   mpVertexBuffers = XPhysicalAlloc(totalBytesToAlloc, MAXULONG_PTR, 0, PAGE_WRITECOMBINE | PAGE_READWRITE);

   DWORD* pDst = static_cast<DWORD*>(mpVertexBuffers);
   
   curDim = mMaxDim;
   uint VBIndex = 0;
   do
   {
      DWORD* pStartDst = pDst;
         
      DWORD maxDXT1Index = 0;
      DWORD maxDXT5Index = 0;
      
      const uint numCells = curDim >> 2;
      
      uint dxt1Pitch = numCells;
      uint dxt5Pitch = numCells;
      if (curDim <= 64)
      {
         dxt1Pitch = 32;
         dxt5Pitch = 32;
      }
      for (uint y = 0; y < numCells; y++)
      {
         for (uint x = 0; x < numCells; x++)
         {
            const uint dxt1BlockOfs = XGAddress2DTiledOffset(x, y, dxt1Pitch, 8);
            const uint dxt5BlockOfs = XGAddress2DTiledOffset(x, y, dxt5Pitch, 16);
            
            BDEBUG_ASSERT((dxt1BlockOfs <= USHRT_MAX) && (dxt5BlockOfs <= USHRT_MAX));
            
            *pDst++ = dxt1BlockOfs | (dxt5BlockOfs << 16);            
            
            maxDXT1Index = Math::Max<DWORD>(maxDXT1Index, dxt1BlockOfs);
            maxDXT5Index = Math::Max<DWORD>(maxDXT5Index, dxt5BlockOfs);
         }
      }  
      
      mMaxDXT1BlockIndex[VBIndex] = (WORD)maxDXT1Index;
      mMaxDXT5BlockIndex[VBIndex] = (WORD)maxDXT5Index;
      
      const DWORD totalVBSize = (pDst - pStartDst) * sizeof(DWORD);
      
      BDEBUG_ASSERT(VBIndex < cNumTileLookupVB);
      XGSetVertexBufferHeader(totalVBSize, 0, 0, 0, &mTileLookupVB[VBIndex]);
      XGOffsetResourceAddress(&mTileLookupVB[VBIndex], pStartDst);
      ++VBIndex;
      
      curDim >>= 1;             
   } while (curDim >= 32);
   
   BDEBUG_ASSERT((pDst - (DWORD*)mpVertexBuffers) * sizeof(DWORD) <= totalBytesToAlloc);
}

//==============================================================================
// BGPUDXTPack::deinit
//==============================================================================
void BGPUDXTPack::deinit(void)
{
   ASSERT_RENDER_THREAD

   mAsyncFileLoader.clear();

#ifdef ENABLE_RELOAD_MANAGER
   mFileWatcher.clear();
#endif

   if (mpVertexBuffers)
   {
      XPhysicalFree(mpVertexBuffers);
      mpVertexBuffers = NULL;
   }
   
   Utils::ClearObj(mTileLookupVB);
}

//==============================================================================
// BGPUDXTPack::renderQuad
//==============================================================================
void BGPUDXTPack::renderQuad(int x, int y, int width, int height, float ofsX, float ofsY, bool grid, float uMin, float uMax, float vMin, float vMax)
{
   if (width < 64)
      grid = false;
      
   const DWORD g_dwQuadGridSizeX  = 4; // 512 / 4 = 128
   const DWORD g_dwQuadGridSizeY  = 1; // 512 / 1 = 512
   //const DWORD g_dwNumQuadsInGrid = g_dwQuadGridSizeX * g_dwQuadGridSizeY;
   
   XMMATRIX matrix = XMMatrixIdentity();
   BD3D::mpDev->SetVertexShaderConstantF(0, reinterpret_cast<float*>(&matrix), 4);

   // Dummy vertex decl
   BD3D::mpDev->SetVertexDeclaration(BTLVertex::msVertexDecl);

   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);

   const uint numVerts = grid ? 3 * (g_dwQuadGridSizeX * g_dwQuadGridSizeY) : 3;
   
   struct BQuadVert
   {
      XMFLOAT2 pos;
      XMFLOAT2 uv;
   };
   
   BQuadVert* pVB = static_cast<BQuadVert*>(gRenderDraw.lockDynamicVB(numVerts, sizeof(BQuadVert)));

   BD3D::mpDev->SetVertexFetchConstant(0, gRenderDraw.getDynamicVB(), 0);

   if (grid)
   {
      XMFLOAT4* v = reinterpret_cast<XMFLOAT4*>(pVB);

      FLOAT fGridDimX = 1.0f / (FLOAT)g_dwQuadGridSizeX;
      FLOAT fGridDimY = 1.0f / (FLOAT)g_dwQuadGridSizeY;
      FLOAT fGridDimU = 1.0f / (FLOAT)g_dwQuadGridSizeX;
      FLOAT fGridDimV = 1.0f / (FLOAT)g_dwQuadGridSizeY;
      FLOAT T  = 0.0f;
      FLOAT V0 = 0.0f;

      for( DWORD iy=0; iy<g_dwQuadGridSizeY; iy++ )
      {
         FLOAT L  = 0.0f;
         FLOAT U0 = 0.0f;
         for( DWORD ix=0; ix<g_dwQuadGridSizeX; ix++ )
         {
            FLOAT R = L + fGridDimX;
            FLOAT B = T + fGridDimY;
            FLOAT U1 = U0 + fGridDimU;
            FLOAT V1 = V0 + fGridDimV;

            *v++ = XMFLOAT4( x + ofsX + L * width, y + ofsY + T * height, Math::Lerp(uMin, uMax, U0), Math::Lerp(vMin, vMax, V0) ); // x, y, tu, tv
            *v++ = XMFLOAT4( x + ofsX + R * width, y + ofsY + T * height, Math::Lerp(uMin, uMax, U1), Math::Lerp(vMin, vMax, V0) ); // x, y, tu, tv
            *v++ = XMFLOAT4( x + ofsX + L * width, y + ofsY + B * height, Math::Lerp(uMin, uMax, U0), Math::Lerp(vMin, vMax, V1) ); // x, y, tu, tv

            L  += fGridDimX;
            U0 += fGridDimU;
         }

         T  += fGridDimY;
         V0 += fGridDimV;
      }
   }
   else
   {
      pVB->pos = XMFLOAT2(x + ofsX, y + ofsY);
      pVB->uv = XMFLOAT2(uMin, vMin);
      pVB++;

      pVB->pos = XMFLOAT2(x + width + ofsX, y + ofsY);
      pVB->uv = XMFLOAT2(uMax, vMin);
      pVB++;

      pVB->pos = XMFLOAT2(x + ofsX, y + height + ofsY);
      pVB->uv = XMFLOAT2(uMin, vMax);
   }      

   gRenderDraw.unlockDynamicVB();

   BD3D::mpDev->DrawVertices(D3DPT_RECTLIST, 0, numVerts);
}


//==============================================================================
// BGPUDXTPack::pack
//==============================================================================
void BGPUDXTPack::dxtPack(
   IDirect3DTexture9* pSrcTex, 
   IDirect3DTexture9* pDstTex, 
   uint width, uint height, 
   BGPUDXTPack::ePackFormat packFormat, 
   const XGTEXTURE_DESC& srcTexDesc, const XGTEXTURE_DESC& dstTexDesc, 
   uint srcLevel, uint dstLevel)
{
   const bool doDownSample = srcTexDesc.Width > dstTexDesc.Width;
   const bool doubleDXTBlocks = packFormat > cPackDXT1;
   
   const uint quarterWidth = width >> 2;
   const uint quarterHeight = height >> 2;
   const uint numVertices = quarterWidth * quarterHeight;
   const uint rowPitchInBlocks = (width <= 64) ? 32 : quarterWidth;
   const uint bytesPerBlock = doubleDXTBlocks ? 16 : 8;
   bytesPerBlock;
      
   BDEBUG_ASSERT((dstTexDesc.BytesPerBlock == bytesPerBlock) && (dstTexDesc.WidthInBlocks == quarterWidth) && (dstTexDesc.HeightInBlocks == quarterHeight));
   const BOOL tiledFlag = (0 != (dstTexDesc.Format & D3DFORMAT_TILED_MASK));
   
   BDEBUG_ASSERT(dstTexDesc.RowPitch == rowPitchInBlocks * bytesPerBlock);

#if _XDK_VER >= 6274   
   BD3D::mpDev->BeginExport(0, pDstTex, D3DBEGINEXPORT_VERTEXSHADER);
#else
   BD3D::mpDev->BeginExport(0, pDstTex);
#endif   
   
   uint techniqueIndex = doDownSample ? cTechniqueDXT1DSPack : cTechniqueDXT1Pack;
   switch (packFormat)
   { 
      case cPackDXT5:  techniqueIndex = doDownSample ? cTechniqueDXT5DSPack  : cTechniqueDXT5Pack;    break;
      case cPackDXT5H: techniqueIndex = doDownSample ? cTechniqueDXT5HDSPack : cTechniqueDXT5HPack;   break;
      case cPackDXN:   techniqueIndex = doDownSample ? cTechniqueDXNDSPack   : cTechniqueDXNPack;     break;
   }
   
   BFXLEffectTechnique technique = mEffect.getTechniqueFromIndex(techniqueIndex);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(16, pSrcTex);
            
   const uint cFirstSampler = 16;
   const uint cLastSampler = 16;
   for (uint i = cFirstSampler; i <= cLastSampler; i++)
   {
      BD3D::mpDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      BD3D::mpDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      BD3D::mpDev->SetSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

      if (doDownSample)
      {
         BD3D::mpDev->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
         BD3D::mpDev->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
         BD3D::mpDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
      }
      else
      {
         BD3D::mpDev->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_POINT);
         BD3D::mpDev->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
         BD3D::mpDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
      }
      
   }
   
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   
   uint maxExportIndex;
   
   BD3D::mpDev->SetVertexShaderConstantB(0, &tiledFlag, 1);
   
   if (tiledFlag)
   {
      BDEBUG_ASSERT((width <= mMaxDim) && (height <= mMaxDim));
      
      uint curDim = mMaxDim;
      uint VBIndex = 0;
      while (curDim != width)
      {
         VBIndex++;
         BDEBUG_ASSERT(VBIndex < cNumTileLookupVB);
         curDim >>= 1;
      }
      
      // Dummy vertex decl
      BD3D::mpDev->SetVertexDeclaration(BTLVertex::msVertexDecl);
                  
      BD3D::mpDev->SetVertexFetchConstant(0, &mTileLookupVB[VBIndex], 0);
      
      maxExportIndex = doubleDXTBlocks ? (mMaxDXT5BlockIndex[VBIndex] * 2 + 1) : mMaxDXT1BlockIndex[VBIndex];
   }
   else
   {
      maxExportIndex = rowPitchInBlocks * quarterHeight * (doubleDXTBlocks ? 2 : 1);
   }  

   BYTE* pTexData;
   
   if (!dstLevel)
   {
      pTexData = reinterpret_cast<BYTE*>((DWORD)pDstTex->Format.BaseAddress << 12U);
   }
   else
   {
      const UINT mipLevelOffset = XGGetMipLevelOffset(pDstTex, 0, dstLevel);
      BDEBUG_ASSERT(pDstTex->Format.MipAddress);
      pTexData = reinterpret_cast<BYTE*>(((DWORD)pDstTex->Format.MipAddress << 12U) + mipLevelOffset);
   }

   GPU_MEMEXPORT_STREAM_CONSTANT streamConstant;
   GPU_SET_MEMEXPORT_STREAM_CONSTANT(&streamConstant,
      (pTexData),                                     // pointer to the data
      // + 1 because this is actually the max # of vertices, not the max index!
      maxExportIndex + 1,                             // max index = # of vertices * stride
      SURFACESWAP_LOW_BLUE,                           // whether to output ABGR or ARGB           
      GPUSURFACENUMBER_UINTEGER,                      // data type 
      GPUCOLORFORMAT_16_16_16_16,                     // data format
      GPUENDIAN128_8IN16);                            // endian swap
   
   BD3D::mpDev->SetVertexShaderConstantF(0, streamConstant.c, 1);
         
   BVec4 constants[2];
   constants[0] = BVec4(1.0f / (float)quarterWidth, (float)quarterWidth, .5f / (float)quarterWidth, (float)rowPitchInBlocks);
   constants[1].set((float)srcLevel);

   BD3D::mpDev->SetVertexShaderConstantF(1, constants[0].getPtr(), 2);
         
   BD3D::mpDev->DrawPrimitive(D3DPT_POINTLIST, 0, numVertices);
            
   technique.endPass();
   technique.end();
   
   for (uint i = cFirstSampler; i <= cLastSampler; i++)
      BD3D::mpDev->SetTexture(i, NULL);
   
   BD3D::mpDev->EndExport(0, pDstTex, 0);
}

//==============================================================================
// BGPUDXTPack::tickEffect
//==============================================================================
void BGPUDXTPack::tickEffect(void)
{
#ifdef ENABLE_RELOAD_MANAGER
   if (mFileWatcher.getAreAnyDirty())
   {
      bool success = mAsyncFileLoader.load(gRender.getEffectCompilerDefaultDirID(), FXL_EFFECT_FILENAME);
      BVERIFY(success);

      mEffect.clear();
   }
#endif

   if (!mEffect.getEffect())
   {
      bool success = mAsyncFileLoader.waitUntilReady();
      BVERIFY(success);

      if (!mAsyncFileLoader.getSucceeded())
      {  
         BFATAL_FAIL("Unable to load precompiled shader!");
      }

      HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, mAsyncFileLoader.getData());
      if (FAILED(hres))
      {  
         BFATAL_FAIL("Effect creation failed!");
      }

#ifdef ENABLE_RELOAD_MANAGER
      mAsyncFileLoader.clear();
#else
      mAsyncFileLoader.clear(true);
#endif
   }

   BDEBUG_ASSERT(mEffect.getEffect());

   mEffect.updateIntrinsicParams();
}

//==============================================================================
// BGPUDXTPack::packBegin
//==============================================================================
void BGPUDXTPack::packBegin(
   IDirect3DTexture9* pSrcTex, 
   IDirect3DTexture9* pDstTex, 
   uint srcLevel, uint dstLevel, 
   BGPUDXTPack::ePackFormat& packFormat, 
   XGTEXTURE_DESC& srcTexDesc, 
   XGTEXTURE_DESC& dstTexDesc,
   bool  HDR)
{
   XGGetTextureDesc(pSrcTex, srcLevel, &srcTexDesc);
   XGGetTextureDesc(pDstTex, dstLevel, &dstTexDesc);
   
   const uint width = dstTexDesc.Width;
   const uint height = dstTexDesc.Height;
   width;
   height;

   BDEBUG_ASSERT((width >= 32) && (height >= 32) && ((width & 3) == 0) && ((height & 3) == 0));
   BDEBUG_ASSERT((srcTexDesc.Width >= width) && (srcTexDesc.Height >= height));
   BDEBUG_ASSERT(0 == (dstTexDesc.Flags & XGTDESC_PACKED));

   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, TRUE);
   BD3D::mpDev->SetDepthStencilSurface(NULL);
   
   DWORD fmt = (dstTexDesc.Format & ~D3DFORMAT_TILED_MASK) & ~(D3DFORMAT_SIGNX_MASK | D3DFORMAT_SIGNY_MASK | D3DFORMAT_SIGNZ_MASK);
  
   if (fmt == D3DFMT_LIN_DXT1)
      packFormat = cPackDXT1;
   else if (fmt == D3DFMT_LIN_DXT5 && HDR)
      packFormat = cPackDXT5H;
   else if (fmt == D3DFMT_LIN_DXT5)
      packFormat = cPackDXT5;
   else if (fmt == D3DFMT_LIN_DXN)
      packFormat = cPackDXN;
   else
   {
      BDEBUG_ASSERTM(0, "Unsupported format");  
   }
}

//==============================================================================
// BGPUDXTPack::packEnd
//==============================================================================
void BGPUDXTPack::packEnd(void)
{
}

//==============================================================================
// BGPUDXTPack::pack
//==============================================================================
void BGPUDXTPack::pack(IDirect3DTexture9* pSrcTex, IDirect3DTexture9* pDstTex, uint srcLevel, uint dstLevel,bool HDR)
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(pSrcTex && pDstTex && srcLevel <= 13 && dstLevel <= 13);
   
   tickEffect();
               
   ePackFormat packFormat;
   XGTEXTURE_DESC srcTexDesc;
   XGTEXTURE_DESC dstTexDesc;
   packBegin(pSrcTex, pDstTex, srcLevel, dstLevel, packFormat, srcTexDesc, dstTexDesc,HDR);
               
   dxtPack(pSrcTex, pDstTex, dstTexDesc.Width, dstTexDesc.Height, packFormat, srcTexDesc, dstTexDesc, srcLevel, dstLevel);
   
   packEnd();
}

