//==============================================================================
// worldVisibility.cpp
//
// The world visibility renderer.
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "xgameRender.h"
#include "worldVisibility.h"
#include "renderDraw.h"
#include "render.h"
#include "consoleOutput.h"
#include "tiledAA.h"
#include "blurHelper.h"
#include "configsgameRender.h"
#include "terrainRender.h"
#include "ugxGeomRender.h"

#define SHADER_FILENAME "worldVisibility\\worldVisibility.bin"
#define UNEXPLORED_TEXTURE_FILENAME "system\\fogOfWar\\unexplored.ddx"   

//==============================================================================
// BWorldVisibility::BWorldVisibility
//==============================================================================
BWorldVisibility::BWorldVisibility() :
   mBPTVertexDecl(NULL),
   mpEffectLoader(NULL),
   mWorldSize(1.0f),
   mDataDirID(cDirBase),
   mpUnexploredTexture(NULL),
   mVisibilityTextureDim(512)
{
   setDefaultConfig();
}

//==============================================================================
// BWorldVisibility::~BWorldVisibility
//==============================================================================
BWorldVisibility::~BWorldVisibility()
{
   deinit();
}

//==============================================================================
// BWorldVisibility::init
//==============================================================================
void BWorldVisibility::init(long dataDirID, uint visibilityTextureDim)
{
   ASSERT_MAIN_THREAD

   if (mCommandHandle != cInvalidCommandListenerHandle)
      return;
   
   mVisibilityTextureDim = visibilityTextureDim;
   
   commandListenerInit();
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit();
#endif
   mDataDirID = dataDirID;
   
   loadConfig();
}

//==============================================================================
// BWorldVisibility::setDefaultConfig
//==============================================================================
void BWorldVisibility::setDefaultConfig(void)
{
   mExploredBrightness = .4f;
   mUnexploredBrightness = .045f;
   mEdgeOfWorldTransition = 19.0f;
   mUnexploredMaskUVMultiplier = .02f;
}

//==============================================================================
// BWorldVisibility::loadConfig
//==============================================================================
bool BWorldVisibility::loadConfig(void)
{
   const char* pFilename = "fogOfWar.xml";
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.registerClient(mDataDirID, pFilename, BReloadManager::cFlagSynchronous, getEventHandle());
#endif
   BXMLReader xmlReader;
   if (!xmlReader.load(mDataDirID, pFilename, XML_READER_LOAD_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.output(cMsgError, "BWorldVisibility::loadConfig: Unable to load config file fogOfWar.xml!");
      return false;
   }

   setDefaultConfig();

   BXMLNode rootNode(xmlReader.getRootNode());   
               
   rootNode.getChildValue("exploredBrightness", mExploredBrightness);
   rootNode.getChildValue("unexploredBrightness", mUnexploredBrightness);
   rootNode.getChildValue("edgeOfWorldTransition", mEdgeOfWorldTransition);
   rootNode.getChildValue("unexploredMaskUVMultiplier", mUnexploredMaskUVMultiplier);

   gConsoleOutput.output(cMsgResource, "BWorldVisibility::loadConfig: Successfully loaded config file");
   
   return true;   
}

//==============================================================================
// BWorldVisibility::deinit
//==============================================================================
void BWorldVisibility::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   if (mCommandHandle == cInvalidCommandListenerHandle)
      return;

   if (gRenderThread.getHasD3DOwnership())
      gRenderThread.blockUntilGPUIdle();         
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit();
#endif
   commandListenerDeinit();
}

//==============================================================================
// BWorldVisibility::render
//==============================================================================
void BWorldVisibility::updateVisibility(bool enabled, uint viewportIndex, const BVec4& playableBounds, IDirect3DTexture9* pVisibilityTexture)
{
   ASSERT_MAIN_THREAD
   
   BDEBUG_ASSERT(viewportIndex < cMaxViewports);
   BDEBUG_ASSERT(pVisibilityTexture);
         
   BGenerateVisibilityPacket* pPacket = reinterpret_cast<BGenerateVisibilityPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandFilterVisibility, sizeof(BGenerateVisibilityPacket)));

   pPacket->mpVisibilityTexture = pVisibilityTexture; 
   pPacket->mViewportIndex = viewportIndex;
   pPacket->mWorldSize = mWorldSize;
   pPacket->mEnabled = enabled;
   pPacket->mExploredBrightness = mExploredBrightness;
   pPacket->mUnexploredBrightness = mUnexploredBrightness;
   pPacket->mEdgeOfWorldTransition = mEdgeOfWorldTransition;
   pPacket->mPlayableBounds = playableBounds;
   pPacket->mUnexploredMaskUVMultiplier = mUnexploredMaskUVMultiplier;
            
   gRenderThread.submitCommandEnd(sizeof(BGenerateVisibilityPacket));
}

//==============================================================================
// BWorldVisibility::processCommand
//==============================================================================
void BWorldVisibility::processCommand(const BRenderCommandHeader &header, const uchar *pData)
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      case cMMCommandFilterVisibility:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BGenerateVisibilityPacket));
         workerUpdateVisibility(reinterpret_cast<const BGenerateVisibilityPacket *>(pData));
         break;
      }
   }
}

//==============================================================================
// BWorldVisibility::frameBegin
//==============================================================================
void BWorldVisibility::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// BWorldVisibility::frameEnd
//==============================================================================
void BWorldVisibility::frameEnd(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// BWorldVisibility::initDeviceData
//==============================================================================
void BWorldVisibility::initDeviceData()
{
   ASSERT_RENDER_THREAD
   
   mpUnexploredTexture = gD3DTextureManager.getOrCreate(UNEXPLORED_TEXTURE_FILENAME, BFILE_OPEN_DISCARD_ON_CLOSE, 1, true, cDefaultTextureWhite, true, false, "BWorldVisibility");

   BDEBUG_ASSERT(!mpEffectLoader);
   mpEffectLoader = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
   const bool status = mpEffectLoader->init(gRender.getEffectCompilerDefaultDirID(), SHADER_FILENAME, true, false, true);
   BVERIFY(status);

   // Create vertex decl
   const D3DVERTEXELEMENT9 PT_vertexElements[] =
   {
      // rg [6/13/06] - Can't use D3DDECLTYPE_FLOAT16_2 for position because it doesn't have enough precision.
      { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
      { 0, 8, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
      D3DDECL_END()
   };
   BDEBUG_ASSERT(!mBPTVertexDecl);
   BD3D::mpDev->CreateVertexDeclaration(PT_vertexElements, &mBPTVertexDecl);
      
   for (uint i = 0; i < cMaxViewports; i++)
   {
      HRESULT hres = gRenderDraw.createTexture(mVisibilityTextureDim, mVisibilityTextureDim, 1, 0, D3DFMT_A8, 0, &mpPrevVisibilityTexture[i], NULL);
      BVERIFY(SUCCEEDED(hres));
   
      D3DLOCKED_RECT lockedRect;
      mpPrevVisibilityTexture[i]->LockRect(0, &lockedRect, NULL, 0);

      memset(lockedRect.pBits, 0, lockedRect.Pitch * mVisibilityTextureDim);

      mpPrevVisibilityTexture[i]->UnlockRect(0);   
   }      
}

//==============================================================================
// Process deinitDeviceData
//==============================================================================
void BWorldVisibility::deinitDeviceData()
{
   ASSERT_RENDER_THREAD
      
   gTerrainRender.clearBlackmapParams();
   BUGXGeomRender::clearBlackmapParams();
   
   if (mpUnexploredTexture)
   {
      // ajl 3/6/08 changed from delete to release to fix crash on exit
      //delete mpUnexploredTexture;
      mpUnexploredTexture->release();
      mpUnexploredTexture = NULL;
   }
               
   for (uint i= 0; i < cMaxViewports; i++)
   {
      if (mpPrevVisibilityTexture[i])
      {
         mpPrevVisibilityTexture[i]->Release();
         mpPrevVisibilityTexture[i] = NULL;  
      }
   }      
         
   if (mpEffectLoader)
   {
      ALIGNED_DELETE(mpEffectLoader, gRenderHeap);
      mpEffectLoader = NULL;
   }
   
   if (mBPTVertexDecl)
   {
      mBPTVertexDecl->Release();
      mBPTVertexDecl = NULL;
   }
}

//==============================================================================
// BWorldVisibility::tickEffect
//==============================================================================
void BWorldVisibility::tickEffect(void)
{
   ASSERT_RENDER_THREAD
   
   BDEBUG_ASSERT(mpEffectLoader);
   if (mpEffectLoader->tick())
   {
      if (mpEffectLoader->isEffectValid())
      {
         BFXLEffect& effect = mpEffectLoader->getFXLEffect();
         
         // Get techniques and params
         mVisibilityTextureParam          = effect("visibilityTexture");
         mPrevVisibilityTextureParam      = effect("prevVisibilityTexture");
         mBlurVisibilityXTechnique        = effect.getTechnique("blurVisibilityX");
         mBlurVisibilityYTechnique        = effect.getTechnique("blurVisibilityY");
         mBlurVisibilityOffsetsParam      = effect("blurVisibilityOffsets");
         mBlurVisibilityWeightsParam      = effect("blurVisibilityWeights");
      }
      else
      {
         mVisibilityTextureParam.clear();
         mPrevVisibilityTextureParam.clear();
         mBlurVisibilityXTechnique.clear();
         mBlurVisibilityYTechnique.clear();
         mBlurVisibilityOffsetsParam.clear();
         mBlurVisibilityWeightsParam.clear();
      }         
   }      
}

//==============================================================================
// BWorldVisibility::workerDrawQuad
//==============================================================================
void BWorldVisibility::workerDrawQuad(uint width, uint height)
{
   BPTVertex* pVB;
   BD3D::mpDev->BeginVertices(D3DPT_RECTLIST, 3, sizeof(BPTVertex), (void **) &pVB);

   const float ofs = -.5f;
   pVB->position = XMFLOAT2(ofs, ofs);
   pVB->uv = XMHALF2(0.0f, 0.0f);
   pVB++;
   pVB->position = XMFLOAT2((float)width + ofs, ofs);
   pVB->uv = XMHALF2(1.0f, 0.0f);
   pVB++;
   pVB->position = XMFLOAT2(ofs, (float)height + ofs);
   pVB->uv = XMHALF2(0.0f, 1.0f);

   BD3D::mpDev->EndVertices();
}

//==============================================================================
// BWorldVisibility::workerFilterVisibility
//==============================================================================
void BWorldVisibility::workerFilterVisibility(const BGenerateVisibilityPacket* pVisibilityData)
{
   if (!mBPTVertexDecl || !pVisibilityData->mpVisibilityTexture)
      return;
      
#ifdef BUILD_DEBUG
   XGTEXTURE_DESC texDesc;
   XGGetTextureDesc(pVisibilityData->mpVisibilityTexture, 0, &texDesc);
   BDEBUG_ASSERT((texDesc.Width == mVisibilityTextureDim) && (texDesc.Height == mVisibilityTextureDim));
#endif   
   
   D3DSURFACE_PARAMETERS surfParams;
   Utils::ClearObj(surfParams);
   IDirect3DSurface9 visibilitySurface;
   Utils::ClearObj(visibilitySurface);
   UINT hierZSize;
   XGSetSurfaceHeader(mVisibilityTextureDim, mVisibilityTextureDim, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, &surfParams, &visibilitySurface, &hierZSize);
   visibilitySurface.Identifier = 0;
   visibilitySurface.ReadFence = 0;
   visibilitySurface.Fence = 0x3f7fffff;
   XGOffsetSurfaceAddress(&visibilitySurface, 0, 0);

   BD3D::mpDev->SetRenderTarget(0, &visibilitySurface);
   BD3D::mpDev->SetDepthStencilSurface(NULL);

   // Set params
   BD3D::mpDev->SetVertexDeclaration(mBPTVertexDecl);
   
   const uint cMaxBlurSamples = 4;
   BVec4 scales[cMaxBlurSamples], hSamples[cMaxBlurSamples], vSamples[cMaxBlurSamples];
   Utils::ClearObj(scales);
   Utils::ClearObj(hSamples);
   Utils::ClearObj(vSamples);
   
   BBlurHelper blurHelper;
   static float sigma = 6.0f;
   const uint sampleCount = blurHelper.computeBilinearBloomSamples(
      sigma, 1.0f, scales, hSamples, vSamples, cMaxBlurSamples, mVisibilityTextureDim, mVisibilityTextureDim); 
   sampleCount;
   BDEBUG_ASSERT((sampleCount >= 1) && (sampleCount <= cMaxBlurSamples));
   
   const BVec4 weights(scales[0][0], scales[1][0], scales[2][0], scales[3][0]);

   mVisibilityTextureParam = pVisibilityData->mpVisibilityTexture; 
   mPrevVisibilityTextureParam = mpPrevVisibilityTexture[pVisibilityData->mViewportIndex];
   mBlurVisibilityWeightsParam = weights;
   
   // Horizontal
   // This places the previous visibility factor into EDRAM RGB, and the horizontally filtered current factor into A.
   {
      SCOPEDSAMPLE(FilterVisiblityH);
      
      mBlurVisibilityXTechnique.beginRestoreDefaultState();
      mBlurVisibilityXTechnique.beginPass(0);
      mBlurVisibilityOffsetsParam.setArray(hSamples, cMaxBlurSamples);
      mBlurVisibilityXTechnique.commit();

      workerDrawQuad(mVisibilityTextureDim, mVisibilityTextureDim);
      
      mBlurVisibilityXTechnique.endPass();
      mBlurVisibilityXTechnique.end();
      
      BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS, NULL, mpPrevVisibilityTexture[pVisibilityData->mViewportIndex], NULL, 0, 0, NULL, 1.0f, 0, NULL);
   }      
     
   // Vertical
   {
      SCOPEDSAMPLE(FilterVisiblityV);
      
      mBlurVisibilityYTechnique.beginRestoreDefaultState();
      mBlurVisibilityYTechnique.beginPass(0);
      mBlurVisibilityOffsetsParam.setArray(vSamples, cMaxBlurSamples);
      mBlurVisibilityYTechnique.commit();

      workerDrawQuad(mVisibilityTextureDim, mVisibilityTextureDim);

      mBlurVisibilityYTechnique.endPass();
      mBlurVisibilityYTechnique.end();      
      
      // This will resolve the red channel to the alpha texture (i.e. we trick the GPU into thinking it's an D3DFMT_R8 texture, instead of A8.)
      IDirect3DTexture9 dstTex;
      memcpy(&dstTex, mpPrevVisibilityTexture[pVisibilityData->mViewportIndex], sizeof(dstTex));
      dstTex.Format.SwizzleX = GPUSWIZZLE_X;
      dstTex.Format.SwizzleY = GPUSWIZZLE_0;
      dstTex.Format.SwizzleZ = GPUSWIZZLE_0;
      dstTex.Format.SwizzleW = GPUSWIZZLE_1;
         
      BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS, NULL, &dstTex, NULL, 0, 0, NULL, 1.0f, 0, NULL);
   }      

   // Must reset the render target because the current surface is on the stack and will go out of scope here.      
   BD3D::mpDev->SetRenderTarget(0, BD3D::mpDevBackBuffer);      
}      

//==============================================================================
// BWorldVisibility::workerUpdateVisibility
//==============================================================================
void BWorldVisibility::workerUpdateVisibility(const BGenerateVisibilityPacket* pVisibilityData)
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLE(WorldVisibilityRender);
   
   tickEffect();
   
   if ((!pVisibilityData->mEnabled) || (!mpEffectLoader->isEffectValid()) || (!pVisibilityData->mpVisibilityTexture))
   {
      gTerrainRender.clearBlackmapParams();
      BUGXGeomRender::clearBlackmapParams();
      return;
   }
   
   BFXLEffect& effect = mpEffectLoader->getFXLEffect();
   
   effect.updateIntrinsicParams();
      
   workerFilterVisibility(pVisibilityData);

   gRenderDraw.unsetTextures();

   const BVec3 backgroundColor(gRenderSceneLightManager.getBackgroundColor() * gRenderSceneLightManager.getBackgroundIntensity());
   
   BBlackmapParams blackmapParams;
   blackmapParams.mpTexture = mpPrevVisibilityTexture[pVisibilityData->mViewportIndex];
   blackmapParams.mParams[0].set(backgroundColor[0], backgroundColor[1], backgroundColor[2], pVisibilityData->mExploredBrightness);
            
   float lx = pVisibilityData->mPlayableBounds[0] + mEdgeOfWorldTransition;
   float lz = pVisibilityData->mPlayableBounds[1] + mEdgeOfWorldTransition;
   float hx = pVisibilityData->mPlayableBounds[2] - mEdgeOfWorldTransition;
   float hz = pVisibilityData->mPlayableBounds[3] - mEdgeOfWorldTransition;
   
   blackmapParams.mParams[1].set(pVisibilityData->mUnexploredBrightness, lx, lz, pVisibilityData->mUnexploredMaskUVMultiplier);
   blackmapParams.mParams[2].set(1.0f / pVisibilityData->mWorldSize, hx, hz, 1.0f / mEdgeOfWorldTransition);
   blackmapParams.mpUnexploredTexture = mpUnexploredTexture->getD3DTexture().getTexture();
            
   gTerrainRender.setBlackmapParams(blackmapParams);
   BUGXGeomRender::setBlackmapParams(blackmapParams);
}

#ifdef ENABLE_RELOAD_MANAGER
bool BWorldVisibility::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassReloadNotify:
      {
         loadConfig();
         gConsoleOutput.printf("BWorldVisibility::loadConfig: Successfully loaded config file");
         break;
      }
   }
   
   return false;
}
#endif