//============================================================================
//
//  File: gpuDXTUnpacker.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"

#if 0
#include "gpuDXTUnpacker.h"
#include "resource\ecfUtils.h"
#include "BD3D.h"

//==============================================================================
// Defines
//==============================================================================
#define FXL_EFFECT_FILENAME "shaders\\dxtPack\\dxtqUnpack.bin"

//==============================================================================
// Enums
//==============================================================================
enum eTechniques
{
   cTechniqueDXT1QUnpack,
   cTechniqueDXT5QUnpack,
   cTechniqueDXNQUnpack,
   
   cNumTechniques
};

//============================================================================
// BDXTQTextureData::BDXTQTextureData
//============================================================================
BDXTQTextureData::BDXTQTextureData() :
   mpCachedMemory(NULL),
   mpPhysicalMemory(NULL),
   mpTex(NULL),
   mpColorCodebookTex(NULL),
   mpAlphaCodebookTex(NULL),
   mpColorIndicesVB(NULL),
   mpAlphaIndicesVB(NULL)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
}

//============================================================================
// BDXTQTextureData::~BDXTQTextureData
//============================================================================
BDXTQTextureData::~BDXTQTextureData()
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   deinit();
}

//============================================================================
// BDXTQTextureData::init
//============================================================================
bool BDXTQTextureData::init(void* pDDXFileData, uint DDXFileDataLen)
{
   deinit();
   
   BECFFileReader ddxFile(BConstDataBuffer(static_cast<const BYTE*>(pDDXFileData), DDXFileDataLen));
   
   if (!ddxFile.checkHeader(true))
      return false;
      
   if ((ddxFile.getID() != cDDX_ECF_FILE_ID) || (ddxFile.getNumChunks() < 2))
      return false;
      
   uint ddxHeaderLen;
   const BDDXHeader* pDDXHeader = reinterpret_cast<const BDDXHeader*>(ddxFile.getChunkDataByID(cDDX_ECF_HEADER_CHUNK_ID, ddxHeaderLen));
   if ((!pDDXHeader) || (ddxHeaderLen != sizeof(BDDXHeader)))
      return false;
     
   mDDXHeader = *pDDXHeader;
   if ((mDDXHeader.mHeaderMagic != BDDXHeader::cDDXHeaderMagic) || (mDDXHeader.mMinRequiredVersion > BDDXHeader::cDDXVersion))
      return false;
      
   if (!getDDXDataFormatIsDXTQ(pDDXHeader->mDataFormat))
      return false;      
      
   uint dxtqDataLen;
   const void* pDXTQData = ddxFile.getChunkDataByID(cDDX_ECF_MIP0_CHUNK_ID, dxtqDataLen);
   if ((!pDXTQData) || (!dxtqDataLen))
      return false;
      
   BECFFileReader dxtqFile(BConstDataBuffer(static_cast<const BYTE*>(pDXTQData), dxtqDataLen));
   if (!dxtqFile.checkHeader(true))
      return false;

   if ((dxtqFile.getID() != cDXTQID) || (dxtqFile.getNumChunks() != 3))
      return false;
       
   uint dxtqHeaderLen;       
   const BDXTQHeader* pDXTQHeader = reinterpret_cast<const BDXTQHeader*>(dxtqFile.getChunkDataByIndex(0, dxtqHeaderLen));
   if ((!pDXTQHeader) || (dxtqHeaderLen != sizeof(BDXTQHeader)))
      return false;
      
   mDXTQHeader = *pDXTQHeader;
   if (mDXTQHeader.mVersion != BDXTQHeader::cVersion)
      return false;
   if ( (Utils::ExtractLowByteFromWORD((WORD)mDXTQHeader.mD3DStructSize) != sizeof(IDirect3DTexture9)) ||
        (Utils::ExtractHighByteFromWORD((WORD)mDXTQHeader.mD3DStructSize) != sizeof(IDirect3DVertexBuffer9)) )
      return false;        
   
   const void* pCachedMem = dxtqFile.getChunkDataByIndex(1);
   const void* pPhysMem = dxtqFile.getChunkDataByIndex(2);
      
   const uint cachedMemSize = dxtqFile.getChunkDataLenByIndex(1);
   const uint physMemSize = dxtqFile.getChunkDataLenByIndex(2);
   if ((!pCachedMem) || (!pPhysMem) || (!cachedMemSize) || (!physMemSize) || (cachedMemSize > 65536) || (physMemSize > 64*1024*1024))
      return false;
   
   mpCachedMemory = gRenderHeap.New(cachedMemSize);
   Utils::FastMemCpy(mpCachedMemory, pCachedMem, cachedMemSize);
   
   mpPhysicalMemory = XPhysicalAlloc(physMemSize, MAXULONG_PTR, 4096, PAGE_READWRITE);
   Utils::FastMemCpy(mpPhysicalMemory, pPhysMem, physMemSize);
   Utils::FlushCacheLines(mpPhysicalMemory, physMemSize);
   
   mpTex = (IDirect3DTexture9*)(mDXTQHeader.mD3DTexOfs + (BYTE*)mpCachedMemory);
      
   XGTEXTURE_DESC texDesc;
   XGGetTextureDesc(mpTex, 0, &texDesc);
   switch (texDesc.Format)
   {
      case D3DFMT_DXT1:
      case D3DFMT_DXT5:
      case D3DFMT_DXN:
      {
         break;
      }
      default:
      {
         deinit();
         return false;
      }
   }
   
   if (mDXTQHeader.mColorCodebookSize)   
   {
      mpColorCodebookTex = (IDirect3DTexture9*)(mDXTQHeader.mColorCodebookTexOfs + (BYTE*)mpCachedMemory);
      XGOffsetResourceAddress(mpColorCodebookTex, mpPhysicalMemory);
                  
      mpColorIndicesVB = (IDirect3DVertexBuffer9*)(mDXTQHeader.mColorIndicesVBOfs + (BYTE*)mpCachedMemory);
      XGOffsetResourceAddress(mpColorIndicesVB, mpPhysicalMemory);
   }
         
   if (mDXTQHeader.mAlphaCodebookSize)
   {
      mpAlphaCodebookTex = (IDirect3DTexture9*)(mDXTQHeader.mAlphaCodebookTexOfs + (BYTE*)mpCachedMemory);
      XGOffsetResourceAddress(mpAlphaCodebookTex, mpPhysicalMemory);
      
      mpAlphaIndicesVB = (IDirect3DVertexBuffer9*)(mDXTQHeader.mAlphaIndicesVBOfs + (BYTE*)mpCachedMemory);
      XGOffsetResourceAddress(mpAlphaIndicesVB, mpPhysicalMemory);
   }
      
   return true;
}

//============================================================================
// BDXTQTextureData::deinit
//============================================================================
void BDXTQTextureData::deinit(void)
{
   if (mpCachedMemory)
   {
      gRenderHeap.Delete(mpCachedMemory);
      mpCachedMemory = NULL;
   }
   
   if (mpPhysicalMemory)
   {
      XPhysicalFree(mpPhysicalMemory);
      mpPhysicalMemory = NULL;
   }
   
   mpTex = NULL;
   mpColorCodebookTex = NULL;
   mpAlphaCodebookTex = NULL;
   mpColorIndicesVB = NULL;
   mpAlphaIndicesVB = NULL;
}

//============================================================================
// BDXTQTextureData::offsetResourceAddresses
//============================================================================
void BDXTQTextureData::initTex(void* pPhysMemory, bool hasMip0)
{
   if (!mpTex)
      return;
    
   if (!hasMip0)
   {
      mpTex->Format.MinMipLevel = 1;
      mpTex->Format.BaseAddress = 0;
      mpTex->Format.MipAddress = (DWORD)pPhysMemory >> 12;
   }
   else
   {
      mpTex->Format.MinMipLevel = 0;
      mpTex->Format.BaseAddress = (DWORD)pPhysMemory >> 12;
      mpTex->Format.MipAddress = ((DWORD)pPhysMemory + mDXTQHeader.mBaseSize) >> 12;
   }
}

//============================================================================
// BGPUDXTUnpacker::BGPUDXTUnpacker
//============================================================================
BGPUDXTUnpacker::BGPUDXTUnpacker() :
   mpDummyDecl(NULL)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   init();
}

//============================================================================
// BGPUDXTUnpacker::~BGPUDXTUnpacker
//============================================================================
BGPUDXTUnpacker::~BGPUDXTUnpacker()
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   deinit();
}

//============================================================================
// BGPUDXTUnpacker::init
//============================================================================
void BGPUDXTUnpacker::init(void)
{
   deinit();
   
   D3DVERTEXELEMENT9 dummyVertexElements[] =
   {
      { 0,  0, D3DDECLTYPE_UDEC4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
      D3DDECL_END()
   };
   
   BD3D::mpDev->CreateVertexDeclaration(dummyVertexElements, &mpDummyDecl);
   
   mEffectFile.init(0, FXL_EFFECT_FILENAME);
}

//============================================================================
// BGPUDXTUnpacker::deinit
//============================================================================
void BGPUDXTUnpacker::deinit(void)
{
   mEffectFile.deinit();
   
   if (mpDummyDecl)
   {
      mpDummyDecl->Release();
      mpDummyDecl = NULL;
   }
}

//============================================================================
// BGPUDXTUnpacker::unpack
//============================================================================
bool BGPUDXTUnpacker::unpack(BDXTQTextureData& DXTQTexData, void* pPhysMemory, uint physMemSize, bool unpackMip0, bool unpackMipChain)
{
   if (!DXTQTexData.getTex())
      return false;
      
   mEffectFile.tick();
   if (!mEffectFile.isEffectValid())
      return false;
      
   BFXLEffect& effect = mEffectFile.getEffect();
   
   effect.updateIntrinsicParams();

   BDEBUG_ASSERT(unpackMip0 || unpackMipChain);
      
   uint physMemNeeded = unpackMip0 ? DXTQTexData.getMip0PhysMemRequired() : 0;
   physMemNeeded += unpackMipChain ? DXTQTexData.getMipChainPhysMemRequired() : 0;
   
   if (physMemSize < physMemNeeded)
      return false;

   bool hasColorBlocks = false;
   bool hasAlpha0Blocks = false;
   bool hasAlpha1Blocks = false;
   uint bytesPerBlock = 0;
   uint totalBlocksToUnpack = 0;
   uint techniqueIndex = cTechniqueDXT1QUnpack;

   switch (DXTQTexData.getDDXHeader().mDataFormat)
   {
      case cDDXDataFormatDXT1Q:  
      {
         hasColorBlocks = true; 
         bytesPerBlock = 8; 
         totalBlocksToUnpack = physMemSize >> 3; 
         techniqueIndex = cTechniqueDXT1QUnpack; break;
         break;
      }
      case cDDXDataFormatDXT5Q:  
      {
         hasColorBlocks = true; 
         hasAlpha0Blocks = true; 
         bytesPerBlock = 16; 
         totalBlocksToUnpack = physMemSize >> 4; 
         techniqueIndex = cTechniqueDXT5QUnpack; break;
         break;
      }
      case cDDXDataFormatDXT5HQ: 
      {
         hasColorBlocks = true; 
         hasAlpha0Blocks = true; 
         bytesPerBlock = 16; 
         totalBlocksToUnpack = physMemSize >> 4; 
         techniqueIndex = cTechniqueDXT5QUnpack; break;
         break; 
      }
      case cDDXDataFormatDXNQ:   
      {
         hasAlpha0Blocks = true; 
         hasAlpha1Blocks = true; 
         bytesPerBlock = 16; 
         totalBlocksToUnpack = physMemSize >> 4; 
         techniqueIndex = cTechniqueDXNQUnpack; break;
         break;
      }
      default:
         return false;
   }

   IDirect3DVertexBuffer9 unpackVB;
   XGSetVertexBufferHeader(physMemNeeded, 0, 0, 0, &unpackVB);
   XGOffsetResourceAddress(&unpackVB, pPhysMemory);

#if _XDK_VER >= 6274
   BD3D::mpDev->BeginExport(0, &unpackVB, D3DBEGINEXPORT_VERTEXSHADER);
#else
   BD3D::mpDev->BeginExport(0, &unpackVB);
#endif   
   
   BFXLEffectTechnique technique = effect.getTechniqueFromIndex(techniqueIndex);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();
            
   BD3D::mpDev->SetTexture(16, DXTQTexData.getColorCodebookTex());
   BD3D::mpDev->SetTexture(17, DXTQTexData.getAlphaCodebookTex());
   
   // Dummy vertex decl
   BD3D::mpDev->SetVertexDeclaration(mpDummyDecl);
   
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   BD3D::mpDev->SetStreamSource(1, NULL, 0, 0);

   if (DXTQTexData.getColorIndicesVB()) BD3D::mpDev->SetVertexFetchConstant(0, DXTQTexData.getColorIndicesVB(), 0);
   if (DXTQTexData.getAlphaIndicesVB()) BD3D::mpDev->SetVertexFetchConstant(1, DXTQTexData.getAlphaIndicesVB(), 0);
         
   GPU_MEMEXPORT_STREAM_CONSTANT streamConstant;
   GPU_SET_MEMEXPORT_STREAM_CONSTANT(&streamConstant,
      pPhysMemory,                                    // pointer to the data
      // + 1 because this is actually the max # of vertices, not the max index!
      totalBlocksToUnpack + 1,                        // max index = # of vertices * stride
      SURFACESWAP_LOW_BLUE,                           // whether to output ABGR or ARGB           
      GPUSURFACENUMBER_UINTEGER,                      // data type 
      GPUCOLORFORMAT_16_16_16_16,                     // data format
      GPUENDIAN128_8IN16);                            // endian swap

   BD3D::mpDev->SetVertexShaderConstantF(0, streamConstant.c, 1);   
   
   BVec4 unpackParams(
      (float)DXTQTexData.getDXTQHeader().mColorCodebookSize, 
      (float)DXTQTexData.getDXTQHeader().mColorSelectorCodebookSize, 
      (float)DXTQTexData.getDXTQHeader().mAlphaCodebookSize, 
      (float)DXTQTexData.getDXTQHeader().mAlphaSelectorCodebookSize);

   BD3D::mpDev->SetVertexShaderConstantF(1, unpackParams.getPtr(), 1);
     
   BD3D::mpDev->DrawPrimitive(D3DPT_POINTLIST, 0, totalBlocksToUnpack);

   technique.endPass();
   technique.end();
   
   BD3D::mpDev->SetTexture(0, NULL);
   BD3D::mpDev->SetTexture(1, NULL);
      
   BD3D::mpDev->EndExport(0, &unpackVB, 0);
   
   return true;
}

#endif