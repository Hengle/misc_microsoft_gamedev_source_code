//==============================================================================
//
// gpuDXTPack.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//
//==============================================================================
#pragma once

// xgameRender
#include "effect.h"

// xsystem
#include "asyncFileManager.h"
#include "asyncFileLoader.h"
#include "reloadManager.h"

// xcore
#include "utils\heapSingleton.h"

//==============================================================================
// class BGPUDXTPack
// This class is ONLY usable from the render thread, including the constructors!
//==============================================================================
class BGPUDXTPack : public BHeapSingleton<BGPUDXTPack, BRenderFixedHeapAllocator>
{
public:
   BGPUDXTPack();
   ~BGPUDXTPack();
         
   uint getMaxDim(void) const { return mMaxDim; }
   
   // This method changes render targets 0 and 1, the depth stencil surface, and enables D3DRS_HALFPIXELOFFSET.
   // Source texture's mip level must be >= the size of the destination mip level.
   // Supported destination formats:
   // D3DFMT_DXT1, D3DFMT_DXT5, D3DFMT_DXN, D3DFMT_LIN_DXT1, D3DFMT_LIN_DXT5, D3DFMT_LIN_DXN
   // Packed mip tails are unsupported.  
   void pack(IDirect3DTexture9* pSrcTex, IDirect3DTexture9* pDstTex, uint srcLevel = 0, uint dstLevel = 0, bool HDR = false);
      
private:
   BAsyncFileLoader mAsyncFileLoader;
#ifdef ENABLE_RELOAD_MANAGER
   BFileWatcher mFileWatcher;
#endif
   BFXLEffect mEffect;
   uint mMaxDim;
      
   // 1024, 512, 256, 128, 64, 32
   enum { cNumTileLookupVB = 6 };
   IDirect3DVertexBuffer9 mTileLookupVB[cNumTileLookupVB];
   WORD mMaxDXT1BlockIndex[cNumTileLookupVB];
   WORD mMaxDXT5BlockIndex[cNumTileLookupVB];
   void* mpVertexBuffers;
   
   enum ePackFormat
   {
      cPackDXT1,
      cPackDXT5,
      cPackDXT5H,
      cPackDXN,
      cPackDXT1DS,
   };
   
   static void renderQuad(int x, int y, int width, int height, float ofsX, float ofsY, bool grid, float uMin = 0.0f, float uMax = 0.0f, float vMin = 0.0f, float vMax = 0.0f);
   void init(uint maxDim);
   void deinit(void);
   void initVertexBuffers(void);
   void tickEffect(void);
   void packBegin(IDirect3DTexture9* pSrcTex, IDirect3DTexture9* pDstTex, uint srcLevel, uint dstLevel, ePackFormat& packFormat, XGTEXTURE_DESC& srcTexDesc, XGTEXTURE_DESC& dstTexDesc, bool HDR);
   void packEnd(void);
   void dxtPack(IDirect3DTexture9* pSrcTex, IDirect3DTexture9* pDstTex, uint width, uint height, ePackFormat packFormat, const XGTEXTURE_DESC& srcTexDesc, const XGTEXTURE_DESC& dstTexDesc, uint srcLevel, uint dstLevel);
};
