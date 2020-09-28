//============================================================================
// flashminimaprenderer.cpp
// renderer for the flash minimap
// Copyright (C) 2007 Ensemble Studios
//============================================================================

#include "xgameRender.h"
#include "render.h"
#include "flashminimaprenderer.h"
#include "renderThread.h"
#include "reloadManager.h"
#include "asyncFileManager.h"
#include "consoleOutput.h"
#include "color.h"
#include "renderToTextureXbox.h"
#include "worldVisibility.h"
#include "flashgateway.h"

BFlashMinimapRenderer gFlashMinimapRenderer;

void copyVisibilityTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData);
void floodFillTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData);
void fillCircleStencilTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData);

const float cMinimapVisibilityTextureSize = 512.0f;

//============================================================================
//============================================================================
BFlashMinimapRenderer::BFlashMinimapRenderer():
   mpIconVertexDecl(NULL),
   mpGenerateVisVertexDecl(NULL),
   mpGenerateVisSquareVertexDecl(NULL),
   mbInit(false),
   mbInitialized(false),
   mVisibilityColor(1,1,1,1),
   mInvalidMapTextureHandle(cInvalidManagedTextureHandle),
   mpVisibilityTexture(NULL),
   mpStencilTexture(NULL)
{
}

//============================================================================
//============================================================================
BFlashMinimapRenderer::~BFlashMinimapRenderer()
{
}

//============================================================================
// BFlashMinimapRenderer::init
//============================================================================
void BFlashMinimapRenderer::init(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   eventReceiverInit(cThreadIndexRender);
   commandListenerInit();   
   reloadInit();
   loadEffect();
   initVertexDeclarations();
   
   gRenderThread.blockUntilWorkerIdle();
   
   mbInit = true;
}

//============================================================================
// BFlashMinimapRenderer::deInit
//============================================================================
void BFlashMinimapRenderer::deInit(void)
{
   if (!mbInit)
      return;

   gRenderThread.blockUntilGPUIdle();
   
   reloadDeinit();
   commandListenerDeinit();
   eventReceiverDeinit();
   
   gRenderThread.blockUntilWorkerIdle();

   mbInit = false;
}

//============================================================================
// BFLashMinimapRenderer:submitIconRender
//============================================================================
void BFlashMinimapRenderer::submitIconRender(XMMATRIX matrix, BManagedTextureHandle textureHandle, int flashmovieIndex, int numIcons, void* pData)
{
   ASSERT_MAIN_THREAD

   if (numIcons < 1)
      return;

   if (!pData)
      return;

   BMinimapDrawIconPacket*packet = reinterpret_cast<BMinimapDrawIconPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cFMMCommandDrawIcons, sizeof(BMinimapDrawIconPacket), 16));
   packet->mMatrix = matrix;
   packet->mpData  = pData;
   packet->numIcons= numIcons;
   packet->mTextureHandle = textureHandle;
   packet->mMaskTextureHandle = cInvalidManagedTextureHandle;
   packet->mBackgroundTextureHandle = cInvalidManagedTextureHandle;
   packet->mRotationAngle = 0.0f;
   packet->mColor.c = cDWORDWhite;
   packet->mFlashMovieIndex = flashmovieIndex;
   packet->mSamplingMode = D3DTADDRESS_MIRROR;
   gRenderThread.submitCommandEnd(sizeof(BMinimapDrawIconPacket), 16);
}

//============================================================================
// BFLashMinimapRenderer:submitResetVisibility
//============================================================================
void BFlashMinimapRenderer::submitResetVisibility(DWORD maxX, DWORD maxZ, bool* pVisibilityData)
{
   ASSERT_MAIN_THREAD;

   BASSERT(pVisibilityData);

   BMinimapResetVisibilityPacket* pPacket = reinterpret_cast<BMinimapResetVisibilityPacket*>(gRenderThread.submitCommandBegin(mCommandHandle, cFMMCommandReset, sizeof(BMinimapResetVisibilityPacket), 16));
   pPacket->mMaxX = maxX;
   pPacket->mMaxZ = maxZ;
   pPacket->mfMaxX = float(maxX);
   pPacket->mfMaxZ = float(maxZ);
   pPacket->pVisibilityData = pVisibilityData;
   gRenderThread.submitCommandEnd(sizeof(BMinimapResetVisibilityPacket), 16);
}

//============================================================================
// BFLashMinimapRenderer:submitMapRender
//============================================================================
void BFlashMinimapRenderer::submitMapRender(XMMATRIX matrix, BManagedTextureHandle textureHandle, BManagedTextureHandle maskTextureHandle, BManagedTextureHandle backgroundTextureHandle, float rotationAngle, XMCOLOR fogColor, float mapFogScalar, float mapSkirtFogScalar, void* pData, DWORD samplingMode)
{
   ASSERT_MAIN_THREAD

   if (!pData)
      return;

   BMinimapDrawIconPacket*packet = reinterpret_cast<BMinimapDrawIconPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cFMMCommandDrawMap, sizeof(BMinimapDrawIconPacket), 16));
   packet->mMatrix = matrix;
   packet->mpData  = pData;
   packet->numIcons= 1;
   packet->mTextureHandle = textureHandle;
   packet->mMaskTextureHandle = maskTextureHandle;
   packet->mBackgroundTextureHandle = backgroundTextureHandle;
   packet->mRotationAngle = rotationAngle;
   packet->mColor.c = fogColor;
   packet->mMapFogScalar = mapFogScalar;
   packet->mMapSkirtFogScalar = mapSkirtFogScalar;
   packet->mFlashMovieIndex = -1;
   packet->mSamplingMode = samplingMode;
   gRenderThread.submitCommandEnd(sizeof(BMinimapDrawIconPacket), 16);
}

//============================================================================
// BFLashMinimapRenderer:submitMapRender
//============================================================================
void BFlashMinimapRenderer::submitGenerateVisibility(XMMATRIX matrix, BManagedTextureHandle visMaskTexture, int numFog, void* pFogFrameStorage, int numVisibility, void* pVisibilityFrameStorage, int numBlocker, void* pBlockerFrameStorage)
{
   ASSERT_MAIN_THREAD

   BMinimapGenerateVisibilityPacket* packet = reinterpret_cast<BMinimapGenerateVisibilityPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cFMMCommandGenerateVisibility, sizeof(BMinimapGenerateVisibilityPacket), 16));

   packet->mMatrix = matrix;
   packet->mNumFog = numFog;
   packet->mNumVisibility = numVisibility;
   packet->mNumBlockers = numBlocker;
   packet->mpFogData = pFogFrameStorage;
   packet->mpVisibilityData = pVisibilityFrameStorage;
   packet->mpBlockerData = pBlockerFrameStorage;
   packet->mMaskTextureHandle = visMaskTexture;

   gRenderThread.submitCommandEnd(sizeof(BMinimapGenerateVisibilityPacket), 16);   
}

//==============================================================================
// BFlashMinimapRenderer::reloadInit
//==============================================================================
void BFlashMinimapRenderer::reloadInit(void)
{
   BReloadManager::BPathArray paths;
   BString effectFilename;
   eFileManagerError result =gFileManager.getDirListEntry(effectFilename, gRender.getEffectCompilerDefaultDirID());
   BVERIFY(cFME_SUCCESS == result);

   strPathAddBackSlash(effectFilename);
   effectFilename += "flashminimap\\flashminimap*.bin";
   paths.pushBack(effectFilename);

   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);
}

//==============================================================================
// BFlashMinimapRenderer::reloadDeinit
//==============================================================================
void BFlashMinimapRenderer::reloadDeinit(void)
{
   gReloadManager.deregisterClient(mEventHandle);
}

//==============================================================================
//==============================================================================
void BFlashMinimapRenderer::loadEffect(void)
{   
   BFixedString256 filename;
   filename.format("flashminimap\\flashminimap.bin");

   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

   pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
   pPacket->setFilename(filename);
   pPacket->setReceiverHandle(mEventHandle);
   pPacket->setPrivateData0(0); //-- effect slot;
   pPacket->setSynchronousReply(true);
   pPacket->setDiscardOnClose(true);
   gAsyncFileManager.submitRequest(pPacket);
}

//==============================================================================
// BFlashMinimapRenderer::receiveEvent
//==============================================================================
bool BFlashMinimapRenderer::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
   switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
         {
            ASSERT_RENDER_THREAD
            initEffect(event);
            break;
         }

      case cEventClassClientRemove:
         {
            ASSERT_RENDER_THREAD
            killEffect();
            break;
         }

      case eEventClassReloadEffects:
         {
            loadEffect();
            break;
         }
   }
   return false;
}

//==============================================================================
// BFlashMinimapRenderer::initEffect
//==============================================================================
void BFlashMinimapRenderer::initEffect(const BEvent& event)
{
   ASSERT_RENDER_THREAD
   BAsyncFileManager::BRequestPacket* pPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);
   BDEBUG_ASSERT(pPacket!=NULL);

   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BFlashMinimapRenderer::initEffect: Async load of file %s failed", pPacket->getFilename().c_str());
   }
   else
   {
      mEffect.clear();
      HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, pPacket->getData());
      if (FAILED(hres))
      {
         gConsoleOutput.output(cMsgError, "BFlashMinimapRenderer::initEffect: Effect creation of file %s failed", pPacket->getFilename().c_str());
      }
      else
      {
         trace("BFlashMinimapRenderer::initEffect: Effect creation of file %s succeeded", pPacket->getFilename().c_str());
      }

      BDEBUG_ASSERT(mEffect.getNumTechniques() == 2);

      // Get techniques and params
      mTechnique      = mEffect.getTechnique("iconTechnique");
      mVisTechnique   = mEffect.getTechnique("visibilityTechnique");
      mTransformParam = mEffect("gTransform");
      mTextureParam   = mEffect("diffuseSampler");
      mMaskTextureParam = mEffect("maskSampler");
      mBackgroundTextureParam = mEffect("backgroundSampler");
      mRotationAngle  = mEffect("gRotationAngle");
      mVisTextureParam = mEffect("visTexture");
      mVisPointTextureParam = mEffect("visPointTexture");
      mColorParam = mEffect("gColor");
      mFogColorParam = mEffect("gMinimapFogColor");
      mFogScalar = mEffect("gFogScalar");

      BDEBUG_ASSERT(mTechnique.getValid());
      BDEBUG_ASSERT(mVisTechnique.getValid());
      BDEBUG_ASSERT(mTransformParam.getValid());
      BDEBUG_ASSERT(mTextureParam.getValid());
      BDEBUG_ASSERT(mMaskTextureParam.getValid());
      BDEBUG_ASSERT(mBackgroundTextureParam.getValid());
      BDEBUG_ASSERT(mRotationAngle.getValid());
      BDEBUG_ASSERT(mVisTextureParam.getValid());
      BDEBUG_ASSERT(mVisPointTextureParam.getValid());
      BDEBUG_ASSERT(mColorParam.getValid());
      BDEBUG_ASSERT(mFogScalar.getValid());

      mbInitialized = true;
   }
}

//==============================================================================
//==============================================================================
void BFlashMinimapRenderer::killEffect()
{
   if (mpIconVertexDecl)
   {
      mpIconVertexDecl->Release();
      mpIconVertexDecl = NULL;
   }

   if (mpGenerateVisVertexDecl)
   {
      mpGenerateVisVertexDecl->Release();
      mpGenerateVisVertexDecl = NULL;
   }

   if (mpGenerateVisSquareVertexDecl)
   {
      mpGenerateVisSquareVertexDecl->Release();
      mpGenerateVisSquareVertexDecl = NULL;
   }

   mTechnique.clear();
   mVisTechnique.clear();
   mEffect.clear();   

   mbInitialized = false;
}

//==============================================================================
//==============================================================================
void BFlashMinimapRenderer::initVertexDeclarations(void)
{
   const D3DVERTEXELEMENT9 BMinimapIconVertex_Elements[] =
   {  
      { 0, 0,   D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
      { 0, 8,   D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 12,  D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE,     0 },      
      { 0, 16,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BMinimapIconVertex_Elements, &mpIconVertexDecl);

   const D3DVERTEXELEMENT9 BMinimapGenerateVisbilityVertex_Elements[] =
   {        
      { 0, 0,   D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 4,   D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_PSIZE,     0 },      
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BMinimapGenerateVisbilityVertex_Elements, &mpGenerateVisVertexDecl);

   const D3DVERTEXELEMENT9 BMinimapGenerateVisbilitySquareVertex_Elements[] =
   {  
      { 0, 0,   D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 4,   D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BMinimapGenerateVisbilitySquareVertex_Elements, &mpGenerateVisSquareVertexDecl);


}

//==============================================================================
// WORKER THREAD FUNCTIONS
//==============================================================================
void BFlashMinimapRenderer::initDeviceData(void)
{
   ASSERT_RENDER_THREAD   

   // create visibility texture
   HRESULT hres = gRenderDraw.createTexture((uint) cMinimapVisibilityTextureSize, uint(cMinimapVisibilityTextureSize), 1, 0, D3DFMT_A8, D3DPOOL_DEFAULT, &mpVisibilityTexture, NULL);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BFlashMinimapRenderer::initDeviceData: createTexture() failed");
   }

   D3DXFillTexture(mpVisibilityTexture, floodFillTexture, &mVisibilityColor);
   
   mVisibilityRenderTarget.set((uint)cMinimapVisibilityTextureSize, (uint)cMinimapVisibilityTextureSize, D3DFMT_A8R8G8B8);
   bool result = mVisibilityRenderTarget.createDeviceObjects(0);
   if (!result)
   {
      BFATAL_FAIL("BFLashMinimapRenderer::initDevicData: createRenderTarget() failed");
   }

   // Create Stencil Texure
   hres = gRenderDraw.createTexture(128, 128, 1, 0, D3DFMT_A8, D3DPOOL_DEFAULT, &mpStencilTexture, NULL);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BMiniMap::initDeviceData: createTexture() failed");
   }
   D3DXFillTexture(mpStencilTexture, fillCircleStencilTexture, &mVisibilityColor);

   mInvalidMapTextureHandle = gD3DTextureManager.getOrCreateHandle("ui\\flash\\minimaps\\newmap", BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "FlashMinimapRenderer");
}

//==============================================================================
//==============================================================================
void copyVisibilityTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData)
{
   BFlashMinimapRenderer::BMinimapResetVisibilityPacket *pPacket = reinterpret_cast<BFlashMinimapRenderer::BMinimapResetVisibilityPacket*>(pData);

   float u = pTexCoord->x;
   float v = 1.0f - pTexCoord->y;
   float x = u * pPacket->mfMaxX;
   float z = v * pPacket->mfMaxZ;
   DWORD offset = (DWORD(z) * pPacket->mMaxX) + DWORD(x);

   float alpha = pPacket->pVisibilityData[offset] ? 0.5f : 1.0f;

   *pOut = D3DXVECTOR4(alpha, alpha, alpha, alpha);
}

//==============================================================================
//==============================================================================
void floodFillTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData)
{
   *pOut = *((D3DXVECTOR4 *) pData);
}

//==============================================================================
//==============================================================================
void fillCircleStencilTexture(D3DXVECTOR4 *pOut, CONST D3DXVECTOR2 *pTexCoord, CONST D3DXVECTOR2 *pTexelSize, LPVOID pData)
{
   const D3DXVECTOR2 center(0.5f, 0.5f);
   const D3DXVECTOR2 delta = *pTexCoord - center;
   const float distanceSqr = delta.x * delta.x + delta.y * delta.y;
   const float maxDistanceSqr = 0.5f * 0.5f;

   if (distanceSqr < maxDistanceSqr)
   {
      *pOut = *((D3DXVECTOR4 *) pData);
   }
   else
   {
      *pOut = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
   }
}

//==============================================================================
//==============================================================================
void BFlashMinimapRenderer::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}
//==============================================================================
// Called from the worker thread to process commands.
//==============================================================================
void BFlashMinimapRenderer::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD

   switch(header.mType)
   {
      case cFMMCommandDrawIcons:      
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BMinimapDrawIconPacket));
         const BMinimapDrawIconPacket* pPacket = reinterpret_cast<const BMinimapDrawIconPacket*>(Utils::AlignUp(pData, 16));
         drawIconsInternal(pPacket, eTechniquePassIcons);
         break;
      }
      case cFMMCommandDrawMap:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BMinimapDrawIconPacket));
         const BMinimapDrawIconPacket* pPacket = reinterpret_cast<const BMinimapDrawIconPacket*>(Utils::AlignUp(pData, 16));
         drawIconsInternal(pPacket, eTechniquePassMap);
         break;
      }
      case cFMMCommandGenerateVisibility:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BMinimapGenerateVisibilityPacket));
         const BMinimapGenerateVisibilityPacket* pPacket = reinterpret_cast<const BMinimapGenerateVisibilityPacket*>(Utils::AlignUp(pData, 16));
         generateVisibilityInternal(pPacket);
         break;
      }
      case cFMMCommandReset:
      {
         if (mpVisibilityTexture)
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BMinimapResetVisibilityPacket));
            BMinimapResetVisibilityPacket* pPacket = const_cast<BMinimapResetVisibilityPacket*>(reinterpret_cast<const BMinimapResetVisibilityPacket*>(Utils::AlignUp(pData, 16)));
            D3DXFillTexture(mpVisibilityTexture, copyVisibilityTexture, pPacket);
         }
         break;
      }
   };
}

//==============================================================================
// Called from worker thread.
//==============================================================================
void BFlashMinimapRenderer::frameEnd(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// deinit will be called from the worker thread before the RCL is freed, 
// but always before the D3D device is release.
//==============================================================================
void BFlashMinimapRenderer::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD

   if (mpVisibilityTexture)
   {      
      mpVisibilityTexture->Release();
      mpVisibilityTexture = NULL;
   }

   if (mpStencilTexture)
   {
      mpStencilTexture->Release();
      mpStencilTexture = NULL;
   }

   mVisibilityRenderTarget.destroyDeviceObjects();   

   if (mInvalidMapTextureHandle!=cInvalidManagedTextureHandle)
   {
      gD3DTextureManager.unloadManagedTextureByHandle(mInvalidMapTextureHandle);
      mInvalidMapTextureHandle = cInvalidManagedTextureHandle;
   }
}

//==============================================================================
//==============================================================================
void BFlashMinimapRenderer::drawIconsInternal(const BMinimapDrawIconPacket* pPacket, int techniquePass)
{
   ASSERT_RENDER_THREAD
   if (!pPacket)
      return;

   if (!mbInitialized)
      return;

   IDirect3DTexture9* pD3DTexture = NULL;
   if (pPacket->mFlashMovieIndex != -1)
   {
      pD3DTexture = gFlashGateway.getTexture(pPacket->mFlashMovieIndex);
   }
   else
   {
      BManagedTextureHandle mapTextureHandle = pPacket->mTextureHandle;
      if (!gD3DTextureManager.isValidManagedTextureHandle(mapTextureHandle))
      {
         mapTextureHandle = mInvalidMapTextureHandle;
         if (!gD3DTextureManager.isValidManagedTextureHandle(mapTextureHandle))
            return;
      }

      BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(mapTextureHandle);
      if (!pTexture)
         return;

      if (pTexture->getStatus() != BD3DTextureManager::BManagedTexture::cStatusLoaded)
      {
         //-- load the invalid map texture up if we didn't get a valid texture
         pTexture = gD3DTextureManager.getManagedTextureByHandle(mInvalidMapTextureHandle);
         if (!pTexture || pTexture->getStatus() != BD3DTextureManager::BManagedTexture::cStatusLoaded)      
            return;
      }
     
      pD3DTexture = pTexture->getD3DTexture().getTexture();
   }

   if (!pD3DTexture)
      return;
   
   // fill out verts
   BMinimapIconVertex* pVB = static_cast<BMinimapIconVertex*>(gRenderDraw.lockDynamicVB(pPacket->numIcons, sizeof(BMinimapIconVertex)));
   BFlashMinimapItem* pItems = static_cast<BFlashMinimapItem*>(pPacket->mpData);
   for (int i = 0; i < pPacket->numIcons; i++)
   {
      pVB->mPosition = XMHALF2(pItems->mPosition.x, pItems->mPosition.z);
      pVB->mSize     = pItems->mSize;
      pVB->mUV       = pItems->mUV;
      pVB->mColor.c  = pItems->mColor;

      pVB++;
      pItems++;
   }   
   gRenderDraw.unlockDynamicVB();

   gRenderDraw.setDefaultRenderStates();
   gRenderDraw.setDefaultSamplerStates();

   // set streams
   gRenderDraw.setVertexDeclaration(mpIconVertexDecl);
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(BMinimapIconVertex));   
   
   mEffect.updateIntrinsicParams();
            
   mTransformParam = pPacket->mMatrix;
   mTextureParam   = pD3DTexture;

   if (pPacket->mMaskTextureHandle != cInvalidManagedTextureHandle)
   {
      BD3DTextureManager::BManagedTexture* pMaskTexture = gD3DTextureManager.getManagedTextureByHandle(pPacket->mMaskTextureHandle);
      if (!pMaskTexture || pMaskTexture->getStatus() != BD3DTextureManager::BManagedTexture::cStatusLoaded)
         return;

      mMaskTextureParam = pMaskTexture->getD3DTexture().getTexture();
   }

   if (pPacket->mBackgroundTextureHandle != cInvalidManagedTextureHandle)
   {
      BD3DTextureManager::BManagedTexture* pBackground = gD3DTextureManager.getManagedTextureByHandle(pPacket->mBackgroundTextureHandle);
      if (!pBackground || pBackground->getStatus() != BD3DTextureManager::BManagedTexture::cStatusLoaded)
         return;

      mBackgroundTextureParam = pBackground->getD3DTexture().getTexture();
   }

   mVisTextureParam = mpVisibilityTexture;

   mRotationAngle  = pPacket->mRotationAngle;
   mFogColorParam  = XMLoadColor(&pPacket->mColor);
   mFogScalar = XMVectorSet(pPacket->mMapFogScalar, pPacket->mMapSkirtFogScalar, 0, 0);
   
   gRenderDraw.setSamplerState(0, D3DSAMP_ADDRESSU, pPacket->mSamplingMode);
   gRenderDraw.setSamplerState(0, D3DSAMP_ADDRESSV, pPacket->mSamplingMode);
   gRenderDraw.setSamplerState(0, D3DSAMP_ADDRESSW, pPacket->mSamplingMode);

   gRenderDraw.setSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   gRenderDraw.setSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   
   // draw   
   mTechnique.beginRestoreDefaultState();
      mTechnique.beginPass(techniquePass);                  
         mTechnique.commit();         
         BD3D::mpDev->DrawVertices(D3DPT_POINTLIST, 0, pPacket->numIcons);
      mTechnique.endPass();
   mTechnique.end();

   // reset renderer
   gRenderDraw.clearStreamSource(0);
   gRenderDraw.setVertexDeclaration(NULL);
}

//==============================================================================
//==============================================================================
void BFlashMinimapRenderer::generateVisibilityInternal(const BMinimapGenerateVisibilityPacket* pPacket)
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(FlashMiniMapGenerateVisibility);

   if (!pPacket)
      return;

   if (!mbInitialized)
      return;

   // fill out verts

   // fog vb 
   IDirect3DVertexBuffer9*    pFogCircleVB = NULL;
   IDirect3DVertexBuffer9*    pFogSquareVB = NULL;

   int fogCircleCount = 0;
   int fogSquareCount = 0;

   BMinimapGenerateVisVertex* pVB;
   if (pPacket->mNumFog > 0)
   {
//-- FIXING PREFIX BUG ID 6526
      const BFlashMinimapVisibilityItem* pFogItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpFogData);
//--
      for (int j = 0; j < pPacket->mNumFog; ++j)
      {
         if (pFogItems->mSize.y > 0.0f)
            fogSquareCount++;
         else
            fogCircleCount++;
         pFogItems++;
      }

      if (fogCircleCount > 0)
      {
         pFogCircleVB = gRenderDraw.createDynamicVB(fogCircleCount * sizeof(BMinimapGenerateVisVertex));
         BVERIFY(pFogCircleVB);
         pFogCircleVB->Lock(0, 0, (void**) &pVB, 0);
         pFogItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpFogData);
         for (int i = 0; i < pPacket->mNumFog; i++)
         {
            if (pFogItems->mSize.y == 0.0f)
            {
               //trace("fogPos[%d]: x=%4.3f z=%4.3f", i, pFogItems->mPosition.x, pFogItems->mPosition.z);
               pVB->mPosition = XMHALF2(pFogItems->mPosition.x, pFogItems->mPosition.z);
               pVB->mSize     = pFogItems->mSize;
               pVB++;
            }
            pFogItems++;
         }   
         pFogCircleVB->Unlock();         
      }

      if (fogSquareCount > 0)
      {         
         pFogSquareVB = gRenderDraw.createDynamicVB(fogSquareCount * sizeof(BMinimapGenerateVisVertex));
         BVERIFY(pFogSquareVB);

         BMinimapGenerateVisVertex* pVB = NULL;
         pFogSquareVB->Lock(0, 0, (void**) &pVB, 0);                 
         pFogItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpFogData);         
         for (int i = 0; i < pPacket->mNumFog; i++)
         {
            if (pFogItems->mSize.y > 0.0f)
            {
               //trace("fogPos[%d]: x=%4.3f z=%4.3f", i, pFogItems->mPosition.x, pFogItems->mPosition.z);
               pVB->mPosition = XMHALF2(pFogItems->mPosition.x, pFogItems->mPosition.z);
               pVB->mSize     = pFogItems->mSize;
               pVB++;
            }
            pFogItems++;
         }   

         pFogSquareVB->Unlock();         
      }
   }

   // vis vb
   IDirect3DVertexBuffer9* pVisCircleVB = NULL;
   IDirect3DVertexBuffer9* pVisSquareVB = NULL;

   int visCircleCount = 0;
   int visSquareCount = 0;
   if (pPacket->mNumVisibility > 0)
   {
//-- FIXING PREFIX BUG ID 6527
      const BFlashMinimapVisibilityItem* pVisItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpVisibilityData);
//--
      for (int j = 0; j < pPacket->mNumVisibility; ++j)
      {
         if (pVisItems->mSize.y > 0.0f)
            visSquareCount++;
         else
            visCircleCount++;
         pVisItems++;
      }

      if (visCircleCount > 0)
      {         
         pVisCircleVB = gRenderDraw.createDynamicVB(visCircleCount* sizeof(BMinimapGenerateVisVertex));
         BVERIFY(pVisCircleVB);
         pVisCircleVB->Lock(0, 0, (void**) &pVB, 0);
         pVisItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpVisibilityData);
         for (int i = 0; i < pPacket->mNumVisibility; i++)
         {
            if (pVisItems->mSize.y == 0.0f)
            {
               //trace("visPos[%d]: x=%4.3f z=%4.3f", i, pVisItems->mPosition.x, pVisItems->mPosition.z);
               pVB->mPosition = XMHALF2(pVisItems->mPosition.x, pVisItems->mPosition.z);
               pVB->mSize     = pVisItems->mSize;
               pVB++;
            }
            pVisItems++;
         }   
         pVisCircleVB->Unlock();         
      }

      if (visSquareCount > 0)
      {
         pVisSquareVB = gRenderDraw.createDynamicVB(visSquareCount * sizeof(BMinimapGenerateVisVertex));
         BVERIFY(pVisSquareVB);
         pVisSquareVB->Lock(0, 0, (void**) &pVB, 0);
         pVisItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpVisibilityData);
         for (int k = 0; k < pPacket->mNumVisibility; k++)
         {
            if (pVisItems->mSize.y > 0.0f)
            {
               //trace("visPos[%d]: x=%4.3f z=%4.3f", i, pVisItems->mPosition.x, pVisItems->mPosition.z);
               pVB->mPosition = XMHALF2(pVisItems->mPosition.x, pVisItems->mPosition.z);
               pVB->mSize     = pVisItems->mSize;
               pVB++;
            }
            pVisItems++;
         }   
         pVisSquareVB->Unlock();
      }
   }

   // blocker VB   
   IDirect3DVertexBuffer9* pBlockerCircleVB = NULL;
   IDirect3DVertexBuffer9* pBlockerSquareVB = NULL;
   int blockerCircleCount = 0;
   int blockerSquareCount = 0;
   if (pPacket->mNumBlockers > 0)
   {
//-- FIXING PREFIX BUG ID 6528
      const BFlashMinimapVisibilityItem* pBlockerItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpBlockerData);
//--
      for (int j = 0; j < pPacket->mNumBlockers; ++j)
      {
         if (pBlockerItems->mSize.y > 0.0f)
            blockerSquareCount++;
         else
            blockerCircleCount++;
         pBlockerItems++;
      }

      if (blockerCircleCount > 0)
      {
         pBlockerCircleVB = gRenderDraw.createDynamicVB(blockerCircleCount * sizeof(BMinimapGenerateVisVertex));
         BVERIFY(pBlockerCircleVB);
         pBlockerCircleVB->Lock(0, 0, (void**) &pVB, 0);         
         pBlockerItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpBlockerData);
         for (int i = 0; i < pPacket->mNumBlockers; i++)
         {
            if (pBlockerItems->mSize.y == 0)
            {
               pVB->mPosition = XMHALF2(pBlockerItems->mPosition.x, pBlockerItems->mPosition.z);
               pVB->mSize     = pBlockerItems->mSize;
               pVB++;
            }            
            pBlockerItems++;
         }   
         pBlockerCircleVB->Unlock();         
      }

      if (blockerSquareCount > 0)
      {
         pBlockerSquareVB = gRenderDraw.createDynamicVB(blockerSquareCount * sizeof(BMinimapGenerateVisVertex));
         BVERIFY(pBlockerSquareVB);
         pBlockerSquareVB->Lock(0, 0, (void**) &pVB, 0);
         pBlockerItems = static_cast<BFlashMinimapVisibilityItem*>(pPacket->mpBlockerData);
         int sanityCheckBlockerCount = 0;
         for (int i = 0; i < pPacket->mNumBlockers; i++)
         {
            if (pBlockerItems->mSize.y > 0.0f)
            {
               pVB->mPosition = XMHALF2(pBlockerItems->mPosition.x, pBlockerItems->mPosition.z);
               pVB->mSize     = pBlockerItems->mSize;
               pVB++;

               sanityCheckBlockerCount++;
            }            
            pBlockerItems++;
         }   

         BVERIFY(sanityCheckBlockerCount == blockerSquareCount);
         pBlockerSquareVB->Unlock();
      }
   }

   BMatrix identityMatrix;
   identityMatrix.makeIdentity();
     
   // set streams
   gRenderDraw.setVertexDeclaration(mpGenerateVisVertexDecl);
   
   BMinimapGenerateVisVertex* pVB2;
   mVisibilityRenderTarget.begin(NULL);
   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET,0, 1.0f, 0);
      mVisTechnique.beginRestoreDefaultState();

         static bool bDoPass0 = true;
         if (bDoPass0)
         {
            mVisTechnique.beginPass(0);
               mVisTextureParam = mpVisibilityTexture;
               mTransformParam  = identityMatrix;
               mColorParam      = D3DXVECTOR4(1,1,1,1);
               mVisTechnique.commit();            
               BD3D::mpDev->BeginVertices(D3DPT_POINTLIST, 1, sizeof(BMinimapGenerateVisVertex), (void **) &pVB2);
                  pVB2->mPosition = XMHALF2((cMinimapVisibilityTextureSize-1.0f) * 0.5f, (cMinimapVisibilityTextureSize-1.0f) * 0.5f);
                  pVB2->mSize     = XMHALF2(cMinimapVisibilityTextureSize, cMinimapVisibilityTextureSize);
               BD3D::mpDev->EndVertices();
            mVisTechnique.endPass();
         }

         static bool bDoPass1 = true;
         static bool bRenderFog = true;
         static bool bRenderVis = true;
         static float fogAlphaValue = 0.5f;
         if (bDoPass1)
         {
            mVisTechnique.beginPass(1);

            mVisPointTextureParam = mpStencilTexture;            
            mTransformParam = pPacket->mMatrix;
            
            if (fogCircleCount > 0)
            {
               if (pPacket->mNumFog && bRenderFog)
               {
                  mColorParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, fogAlphaValue);       
                  BD3D::mpDev->SetStreamSource(0, pFogCircleVB, 0, sizeof(BMinimapGenerateVisVertex));
                  mVisTechnique.commit();
                  BD3D::mpDev->DrawVertices(D3DPT_POINTLIST, 0, fogCircleCount);
               }
            }

            if (visCircleCount > 0)
            {
               if (pPacket->mNumVisibility && bRenderVis)
               {
                  mColorParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f / 255.0f);
                  BD3D::mpDev->SetStreamSource(0, pVisCircleVB, 0, sizeof(BMinimapGenerateVisVertex));
                  mVisTechnique.commit();
                  BD3D::mpDev->DrawVertices(D3DPT_POINTLIST, 0, visCircleCount);
               }
            }

            mVisTechnique.endPass();

            if (visSquareCount > 0 || fogSquareCount > 0)
            {
               mVisTechnique.beginPass(3);
               if (pPacket->mNumFog && bRenderFog && fogSquareCount)
               {
                  mColorParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, fogAlphaValue);            

                  BD3D::mpDev->SetStreamSource(0, pFogSquareVB, 0, sizeof(BMinimapGenerateVisVertex));
                  mVisTechnique.commit();
                  BD3D::mpDev->DrawVertices(D3DPT_POINTLIST, 0, fogSquareCount);
               }

               if (pPacket->mNumVisibility && bRenderVis && visSquareCount)
               {
                  mColorParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f / 255.0f);
                  BD3D::mpDev->SetStreamSource(0, pVisSquareVB, 0, sizeof(BMinimapGenerateVisVertex));
                  mVisTechnique.commit();
                  BD3D::mpDev->DrawVertices(D3DPT_POINTLIST, 0, visSquareCount);                  
               }
               mVisTechnique.endPass();
            }
         }
         
         static bool bDoBlockers = true;
         if (pPacket->mNumBlockers && bDoBlockers)
         {
            // blockers block visibility completely (0.0f visible - 1.0f not visible)
            float blockerAlpha = 1.0f;
            if (blockerCircleCount)
            {
               mVisTechnique.beginPass(2);               
               mColorParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, blockerAlpha);
               BD3D::mpDev->SetStreamSource(0, pBlockerCircleVB, 0, sizeof(BMinimapGenerateVisVertex));
               mVisTechnique.commit();
               BD3D::mpDev->DrawVertices(D3DPT_POINTLIST, 0, blockerCircleCount);
               mVisTechnique.endPass();
            }

            if (blockerSquareCount)
            {               
               gRenderDraw.setVertexDeclaration(mpGenerateVisSquareVertexDecl);
               mVisTechnique.beginPass(5);
               mColorParam = D3DXVECTOR4(1.0f, 1.0f, 1.0f, blockerAlpha);
               BD3D::mpDev->SetStreamSource(0, pBlockerSquareVB, 0, sizeof(BMinimapGenerateVisVertex));
               mVisTechnique.commit();
               BD3D::mpDev->DrawVertices(D3DPT_QUADLIST, 0, blockerSquareCount*4);
               mVisTechnique.endPass();
            }
         }

      mVisTechnique.end();
   mVisibilityRenderTarget.resolve(mpVisibilityTexture);
   mVisibilityRenderTarget.end();  

   BD3D::mpDev->SetVertexDeclaration(NULL);  
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   
   // rg [1/5/08] - Force a kick-off to keep the GPU busy during the beginning of the frame.
   BD3D::mpDev->InsertFence();
}


