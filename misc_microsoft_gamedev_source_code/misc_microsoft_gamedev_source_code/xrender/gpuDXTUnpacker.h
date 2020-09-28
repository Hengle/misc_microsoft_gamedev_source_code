//============================================================================
//
//  File: gpuDXTUnpacker.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#if 0
#include "effectFileLoader.h"
#include "reloadManager.h"
#include "ddxDef.h"
#include "dxtqPack.h"
#include "utils\heapSingleton.h"

//============================================================================
// class BDXTQTextureData
//============================================================================
class BDXTQTextureData
{
public:
   BDXTQTextureData();
   ~BDXTQTextureData();
   
   bool init(void* pDDXFileData, uint DDXFileDataLen);
   void deinit(void);
   
   const BDDXHeader& getDDXHeader(void) const { return mDDXHeader; }
   const BDXTQHeader& getDXTQHeader(void) const { return mDXTQHeader; }
   
   IDirect3DTexture9* getTex(void) const { return mpTex; }
   void initTex(void* pPhysMemory, bool hasMip0);
   
   IDirect3DTexture9* getColorCodebookTex(void) const { return mpColorCodebookTex; }
   IDirect3DTexture9* getAlphaCodebookTex(void) const { return mpAlphaCodebookTex; }
   IDirect3DVertexBuffer9* getColorIndicesVB(void) const { return mpColorIndicesVB; }
   IDirect3DVertexBuffer9* getAlphaIndicesVB(void) const { return mpAlphaIndicesVB; }
   
   uint getMip0PhysMemRequired(void) const { return mDXTQHeader.mBaseSize; }
   uint getMipChainPhysMemRequired(void) const { return mDXTQHeader.mMipSize; }
   uint getTotalPhysMemRequired(void) const { return mDXTQHeader.mBaseSize + mDXTQHeader.mMipSize; }
               
private:   
   BDDXHeader mDDXHeader;
   BDXTQHeader mDXTQHeader;
   
   void* mpCachedMemory;
   void* mpPhysicalMemory;
             
   IDirect3DTexture9* mpTex;
   IDirect3DTexture9* mpColorCodebookTex;
   IDirect3DTexture9* mpAlphaCodebookTex;
   IDirect3DVertexBuffer9* mpColorIndicesVB;
   IDirect3DVertexBuffer9* mpAlphaIndicesVB;
};

//============================================================================
// class BGPUDXTUnpacker
//============================================================================
class BGPUDXTUnpacker : public BHeapSingleton<BGPUDXTUnpacker, BRenderFixedHeapAllocator>
{
public:
   BGPUDXTUnpacker();
   ~BGPUDXTUnpacker();

   void init(void);
   void deinit(void);

   bool unpack(BDXTQTextureData& DXTQTexData, void* pPhysMem, uint physMemSize, bool unpackMip0, bool unpackMipChain);

private:
   BFXLEffectFileLoader mEffectFile;
   
   IDirect3DVertexDeclaration9* mpDummyDecl;
};

#endif