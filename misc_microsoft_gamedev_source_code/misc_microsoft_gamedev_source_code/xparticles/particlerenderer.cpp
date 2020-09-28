//============================================================================
// particlerenderer.h
// Ensemble Studios (C) 2006
//============================================================================

#include "xparticlescommon.h"
#include "particlevertextypes.h"
#include "particlerenderer.h"
#include "asyncFileManager.h"
#include "reloadManager.h"
#include "render.h"
#include "renderthread.h"
#include "vertexTypes.h"
#include "particleemitter.h"
#include "particletexturemanager.h"
#include "consoleOutput.h"
#include "primDraw2D.h"
#include "debugprimitives.h"
#include "math\VMXUtils.h"
#include "visibleLightManager.h"
#include "ParticleSystemManager.h"
#include "..\terrain\TerrainHeightField.h"
#include "..\xgameRender\\tiledAA.h"
#include "deviceStateDumper.h"

BParticleRenderer gParticleRenderer;

//#define RENDER_CULLING_BBOXES

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleRenderer::BParticleRenderer() :
   mFlags()
{
   setFlag(eFlagInitialized, false);
   setFlag(eFlagRenderBBoxes, false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleRenderer::~BParticleRenderer()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleRenderer::init()
{
   ASSERT_MAIN_THREAD
   
   if (getFlag(eFlagInitialized))
      return;
      
   BDEBUG_ASSERT(gRenderThread.getInitialized());
      
   setFlag(eFlagInitialized, true);

   commandListenerInit();
   eventReceiverInit(cThreadIndexRender);
   
   reloadInit();

   loadEffect();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleRenderer::deInit()
{
   ASSERT_MAIN_THREAD
   
   if (!getFlag(eFlagInitialized))
      return;
      
   reloadDeinit();

   commandListenerDeinit();
   eventReceiverDeinit();
   
   setFlag(eFlagInitialized, false);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleRenderer::reloadInit(void)
{
   BReloadManager::BPathArray paths;
   BString effectFilename;
   eFileManagerError result = gFileManager.getDirListEntry(effectFilename, gRender.getEffectCompilerDefaultDirID());
   BVERIFY(cFME_SUCCESS == result);
   strPathAddBackSlash(effectFilename);
   effectFilename += "particles\\particle*.bin";
   paths.pushBack(effectFilename);
   
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleRenderer::reloadDeinit(void)
{
   gReloadManager.deregisterClient(mEventHandle);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleRenderer::loadEffect()
{
   ASSERT_MAIN_OR_WORKER_THREAD

   //-- loads all of the effects up
   for (uint i = 0; i < eRenderCommandTotal; i++)
   {  
      BFixedString256 filename;
      filename.format("particles\\particle%i.bin", i);

      BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

      pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
      pPacket->setFilename(filename);
      pPacket->setReceiverHandle(mEventHandle);
      pPacket->setPrivateData0(i); //-- effect slot;
      pPacket->setSynchronousReply(true);
      pPacket->setDiscardOnClose(true);

      gAsyncFileManager.submitRequest(pPacket);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleRenderer::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
   switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
      {
         initEffect(event);
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

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleRenderer::initEffect(const BEvent& event)
{
   ASSERT_RENDER_THREAD
   BAsyncFileManager::BRequestPacket* pPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);
   BDEBUG_ASSERT(pPacket!=NULL);

   uint effectID = pPacket->getPrivateData0();

   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BParticleRenderer::initEffect: Async load of file %s failed", pPacket->getFilename().c_str());
   }
   else
   {
      // rg [5/28/06] - This will randomly fail if the effect is ever left set on the D3D device! 
      // As of today, it appears 
      mEffects[effectID].clear();
      HRESULT hres = mEffects[effectID].createFromCompiledData(BD3D::mpDev, pPacket->getData());
      if (FAILED(hres))
      {
         gConsoleOutput.output(cMsgError, "BParticleRenderer::initEffect: Effect creation of file %s failed", pPacket->getFilename().c_str());
      }
      else
      {
         trace("BParticleRenderer::initEffect: Effect creation of file %s succeeded", pPacket->getFilename().c_str());
      }
      
      BDEBUG_ASSERT(mEffects[effectID].getNumTechniques() == 4);

      // Get techniques and params
      mTechnique[effectID] = mEffects[effectID].getTechnique("particleDefault");
      mMultiTextureTechnique[effectID] = mEffects[effectID].getTechnique("particleMultiTexture");
      mSoftParticleTechnique[effectID] = mEffects[effectID].getTechnique("softParticleDefault");
      mSoftParticleMultiTextureTechnique[effectID] = mEffects[effectID].getTechnique("softParticleMultiTexture");

      mDiffuseTextureParam[effectID] = mEffects[effectID]("gDiffuseArraySampler");
      mDiffuse2TextureParam[effectID] = mEffects[effectID]("gDiffuseArraySampler2");
      mDiffuse3TextureParam[effectID] = mEffects[effectID]("gDiffuseArraySampler3");
      mIntensityTextureParam[effectID] = mEffects[effectID]("gIntensityArraySampler");
      mColorProgressionTextureParam[effectID] = mEffects[effectID]("gColorProgressionSampler");
      mScaleProgressionTextureParam[effectID] = mEffects[effectID]("gScaleProgressionSampler");
      mIntensityProgressionTextureParam[effectID] = mEffects[effectID]("gIntensityProgressionSampler");
      mRandomTextureParam[effectID] = mEffects[effectID]("gRandomTextureSampler");
      mTextureAnimationData[effectID] = mEffects[effectID]("gTextureAnimationData");
      mTextureAnimationToggle[effectID] = mEffects[effectID]("gTextureAnimationToggle");
      mIntensityMaskToggle[effectID] = mEffects[effectID]("gIntensityMaskToggle");
      mTimeData[effectID] = mEffects[effectID]("gTimeData");
      mEmitterMatrix[effectID] = mEffects[effectID]("gEmitterMatrix");
      mLastUpdateEmitterMatrix[effectID] = mEffects[effectID]("gLastUpdateEmitterMatrix");
      mEmitterAttraction[effectID] = mEffects[effectID]("gEmitterAttraction");
      mEmitterOpacity[effectID] = mEffects[effectID]("gEmitterOpacity");
      mProgressionTextureV[effectID] = mEffects[effectID]("gProgressionTextureV");
      mMultiTextureEnable3Layers[effectID] = mEffects[effectID]("gMultiTextureEnable3Layers");
      mMultiTextureBlendMultiplier[effectID] = mEffects[effectID]("gMultiTextureBlendMultiplier");      
      mTextureStageCounts[effectID] = mEffects[effectID]("gTextureStageCounts");    
      mUVVelocity0[effectID] = mEffects[effectID]("gUVVelocity0");
      mUVVelocity1[effectID] = mEffects[effectID]("gUVVelocity1");

      mUVRandomOffsetSelector0[effectID] = mEffects[effectID]("gUVRandomOffsetSelector0");
      mUVRandomOffsetSelector1[effectID] = mEffects[effectID]("gUVRandomOffsetSelector1");

      mVertex1Color[effectID] = mEffects[effectID]("gVertex1Color");
      mVertex2Color[effectID] = mEffects[effectID]("gVertex2Color");
      mVertex3Color[effectID] = mEffects[effectID]("gVertex3Color");
      mVertex4Color[effectID] = mEffects[effectID]("gVertex4Color");
      mTintColor[effectID] = mEffects[effectID]("gTintColor");
      mBeamForward[effectID]  = mEffects[effectID]("gBeamForward");
      mBeamProgressionAlphaSelector[effectID] = mEffects[effectID]("gBeamProgressionAlphaSelector");

      mPalletteToggle[effectID] = mEffects[effectID]("gPalletteColorToggle");
      mPalletteColorCount[effectID] = mEffects[effectID]("gPalletteColorCount");      
      
      mLightBufferSampler[effectID] = mEffects[effectID]("gLightBufferSampler");     
      mLightBufferingEnabled[effectID] = mEffects[effectID]("gLightBufferingEnabled");
      mLightBufferIntensityScale[effectID] = mEffects[effectID]("gLightBufferIntensityScale");
      mWorldToLightBufCols[effectID] = mEffects[effectID]("gWorldToLightBufCols");

      mHeightfieldSampler[effectID] = mEffects[effectID]("gHeightfieldSampler");
      mWorldToHeightfieldParam[effectID] = mEffects[effectID]("gWorldToHeightfield");
      mHeightfieldYScaleOfsParam[effectID] = mEffects[effectID]("gHeightfieldYScaleOfs");
      mTerrainDecalYOffsetParam[effectID] = mEffects[effectID]("gTerrainDecalYOffset");

      mSoftParticleParams[effectID] = mEffects[effectID]("gSoftParams");
      mSoftParticleParams2[effectID] = mEffects[effectID]("gSoftParams2");
      mDepthTextureSampler[effectID] = mEffects[effectID]("gDepthTextureSampler");

      BDEBUG_ASSERT(mDiffuseTextureParam[effectID].getValid());
      BDEBUG_ASSERT(mDiffuse2TextureParam[effectID].getValid());
      BDEBUG_ASSERT(mDiffuse3TextureParam[effectID].getValid());
      BDEBUG_ASSERT(mIntensityTextureParam[effectID].getValid());
      BDEBUG_ASSERT(mColorProgressionTextureParam[effectID].getValid());
      BDEBUG_ASSERT(mScaleProgressionTextureParam[effectID].getValid());
      BDEBUG_ASSERT(mIntensityProgressionTextureParam[effectID].getValid());
      BDEBUG_ASSERT(mRandomTextureParam[effectID].getValid());
      BDEBUG_ASSERT(mTextureAnimationData[effectID].getValid());
      BDEBUG_ASSERT(mTextureAnimationToggle[effectID].getValid());
      BDEBUG_ASSERT(mIntensityMaskToggle[effectID].getValid());
      BDEBUG_ASSERT(mTimeData[effectID].getValid());
      BDEBUG_ASSERT(mEmitterMatrix[effectID].getValid());
      BDEBUG_ASSERT(mLastUpdateEmitterMatrix[effectID].getValid());
      BDEBUG_ASSERT(mEmitterAttraction[effectID].getValid());
      BDEBUG_ASSERT(mEmitterOpacity[effectID].getValid());
      BDEBUG_ASSERT(mProgressionTextureV[effectID].getValid());
      BDEBUG_ASSERT(mPalletteToggle[effectID].getValid());
      BDEBUG_ASSERT(mPalletteColorCount[effectID].getValid());
      BDEBUG_ASSERT(mMultiTextureEnable3Layers[effectID].getValid());
      BDEBUG_ASSERT(mMultiTextureBlendMultiplier[effectID].getValid());   
      BDEBUG_ASSERT(mTextureStageCounts[effectID].getValid());      
      BDEBUG_ASSERT(mUVVelocity0[effectID].getValid());
      BDEBUG_ASSERT(mUVVelocity1[effectID].getValid());
      BDEBUG_ASSERT(mVertex1Color[effectID].getValid());
      BDEBUG_ASSERT(mVertex2Color[effectID].getValid());
      BDEBUG_ASSERT(mVertex3Color[effectID].getValid());
      BDEBUG_ASSERT(mVertex4Color[effectID].getValid());
      BDEBUG_ASSERT(mTintColor[effectID].getValid());
      BDEBUG_ASSERT(mBeamForward[effectID].getValid());
      BDEBUG_ASSERT(mBeamProgressionAlphaSelector[effectID].getValid());
   }
}

//==============================================================================
// WORKER THREAD FUNCTIONS
//==============================================================================
void BParticleRenderer::initDeviceData(void)
{
   ASSERT_RENDER_THREAD
      //-- do any setup here we need

   initParticleVertexDeclarations();
}

//==============================================================================
//==============================================================================
void BParticleRenderer::frameBegin(void)
{
   ASSERT_RENDER_THREAD

      //gRenderDraw.setRenderState(D3DRS_FILLMODE, renderGetFlag(cFlagDebugRenderWireframe) ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
      //-- prep for rendering
}
//==============================================================================
// Called from the worker thread to process commands.
//==============================================================================
void BParticleRenderer::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
}

//==============================================================================
// Called from the worker thread to process commands.
//==============================================================================
void BParticleRenderer::render(int effectIndex, const uchar* pData)
{
   ASSERT_RENDER_THREAD 
   
   //-- the render command type maps 1:1 to the effect we need to use
   //-- each FXLite effect assumes to have only 1 technique with multiple passes      

   const BParticleEmitterRenderCommand* pCommand = reinterpret_cast<const BParticleEmitterRenderCommand*>(pData);
   BDEBUG_ASSERT(pCommand);
   pCommand = Utils::AlignUp(pCommand, 16);

   int techniquePass = computeTechniquePass(pCommand);   
   BASSERT(techniquePass != -1);

   if (techniquePass == -1)
      return;

   switch (effectIndex)
   {      
      case eRenderBillboard:
      case eRenderUpfacing:
      case eRenderAxial:
      case eRenderVelocityAligned:      
      {            
         //-- we use instancing so multiply the vert count passed in by 4 because we fetch from
         //-- the vb in the vertex shader
         int indexCount  = pCommand->mIndexCount;
         int vertexCount = indexCount > 0 ? pCommand->mVertexCount : pCommand->mVertexCount * 4;         
         render(pCommand, effectIndex, techniquePass, vertexCount, indexCount, BPVertexPT::msVertexDecl, sizeof(BPVertexPT));
         break;
      }
      case eRenderTerrainPatch:
      {         
         int indexCount  = pCommand->mIndexCount;
         int vertexCount = pCommand->mVertexCount;
         render(pCommand, effectIndex, techniquePass, vertexCount, indexCount, BPVertexPT::msVertexDecl, sizeof(BPVertexPT));
         break;
      }
      case eRenderTrail:
      case eRenderTrailStretch:
      case eRenderTrailCrossPass1:
      case eRenderTrailCrossPass2:
      case eRenderTrailCrossStretchPass1:
      case eRenderTrailCrossStretchPass2:
      {
         int vertexCount = (pCommand->mVertexCount - 1) * 8;
         int indexCount = 0;
         render(pCommand, effectIndex, techniquePass, vertexCount, indexCount, BPTrailVertexPT::msVertexDecl, sizeof(BPTrailVertexPT));
         break;
      }      
      case eRenderBeam:
      case eRenderBeamVertical:
      case eRenderBeamHorizontal:
      {
         int vertexCount = (pCommand->mVertexCount - 1) * 4;
         int indexCount = 0;
         render(pCommand, effectIndex, techniquePass, vertexCount, indexCount, BPBeamVertexPT::msVertexDecl, sizeof(BPBeamVertexPT));
         break;
      }
   }

   if (getFlag(eFlagRenderBBoxes))
      renderAABB(pCommand->mAABBMins, pCommand->mAABBMaxs);
}

//==============================================================================
// Called from worker thread.
//==============================================================================
void BParticleRenderer::frameEnd(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// deinit will be called from the worker thread before the RCL is freed, 
// but always before the D3D device is release.
//==============================================================================
void BParticleRenderer::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
   
   for (uint i = 0; i < eRenderCommandTotal; i++)
      mEffects[i].clear();
}

//==============================================================================
//==============================================================================
void BParticleRenderer::render(const BParticleEmitterRenderCommand* RESTRICT pData, int effectIndex, int techniquePassIndex, int vertCount, int indexCount, D3DVertexDeclaration* pVertexDecl, int vertSize)
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLE(BParticleRenderer_render)
   BDEBUG_ASSERT(pData!=NULL);

   static bool gRenderP = true;
   if (!gRenderP)
      return;

   if (vertCount == 0)
      return;
   
   LPDIRECT3DBASETEXTURE9 pTexture = NULL;
   if (pData->mTextureHandle != -1)
   {
      pTexture = gPSTextureManager.getTexture(pData->mTextureHandle);   
   }

   if (pTexture == NULL)
      return;
      
   if (!mEffects[effectIndex].getEffect())
      return;
      
   //-- declare Vertex type
   BD3D::mpDev->SetVertexDeclaration(pVertexDecl);
   //-- tell D3D which stream of verts to use
   BD3D::mpDev->SetStreamSource(0, reinterpret_cast<IDirect3DVertexBuffer9*>(pData->mpVB), pData->mStartOffsetVB, vertSize);

   if (pData->mpIB && indexCount > 0)
      BD3D::mpDev->SetIndices(reinterpret_cast<IDirect3DIndexBuffer9*>(pData->mpIB));

   //-- Apply FXLite and blast out verts
   mEffects[effectIndex].updateIntrinsicParams();

   mDiffuseTextureParam[effectIndex]    = pTexture; //pTexture->getD3DTexture().getNormalTexture();

   LPDIRECT3DBASETEXTURE9 pIntensityTexture = NULL;
   mIntensityTextureParam[effectIndex] = NULL;
   if (pData->mIntensityTextureHandle != -1)
   {
      pIntensityTexture = gPSTextureManager.getTexture(pData->mIntensityTextureHandle);   
      if (pIntensityTexture)
         mIntensityTextureParam[effectIndex]  = pIntensityTexture;
   }

   LPDIRECT3DBASETEXTURE9 pDiffuseTexture2 = NULL;
   mDiffuse2TextureParam[effectIndex] = NULL;
   if (pData->mTextureHandle2 != -1)
   {
      pDiffuseTexture2 = gPSTextureManager.getTexture(pData->mTextureHandle2);   
      if (pDiffuseTexture2)
         mDiffuse2TextureParam[effectIndex]  = pDiffuseTexture2;
   }
      
   LPDIRECT3DBASETEXTURE9 pDiffuseTexture3 = NULL;
   mDiffuse3TextureParam[effectIndex] = NULL;
   if (pData->mTextureHandle3 != -1)
   {
      pDiffuseTexture3 = gPSTextureManager.getTexture(pData->mTextureHandle3);   
      if (pDiffuseTexture3)
         mDiffuse3TextureParam[effectIndex]  = pDiffuseTexture3;
   }

   mColorProgressionTextureParam[effectIndex]= gPSTextureManager.getColorProgressionTexture();
   mScaleProgressionTextureParam[effectIndex]= gPSTextureManager.getScaleProgressionTexture();
   mIntensityProgressionTextureParam[effectIndex]= gPSTextureManager.getIntensityProgressionTexture();

   mRandomTextureParam[effectIndex]     = gPSTextureManager.getRandomTexture();
   mTextureAnimationData[effectIndex]   = pData->mTextureAnimationData;
   mTextureAnimationToggle[effectIndex] = pData->mTextureAnimationToggle;
   mIntensityMaskToggle[effectIndex]    = pData->mIntensityMaskToggle;
   mTimeData[effectIndex]               = pData->mTimeData;
   mEmitterMatrix[effectIndex]          = pData->mEmitterMatrix;
   mLastUpdateEmitterMatrix[effectIndex]= pData->mLastUpdateEmitterMatrix;
   mEmitterAttraction[effectIndex]      = pData->mEmitterAttraction;
   mEmitterOpacity[effectIndex]         = pData->mOpacity;
   mProgressionTextureV[effectIndex]    = pData->mProgressionTextureV;
   mTextureStageCounts[effectIndex]     = pData->mTextureStageCounts;
   mUVVelocity0[effectIndex]            = pData->mUVVelocity0;
   mUVVelocity1[effectIndex]            = pData->mUVVelocity1;
   mUVRandomOffsetSelector0[effectIndex] = pData->mUVRandomOffsetSelector0;
   mUVRandomOffsetSelector1[effectIndex] = pData->mUVRandomOffsetSelector1;

   mBeamForward[effectIndex] = pData->mBeamForward;
   mVertex1Color[effectIndex] = pData->mVertex1Color;
   mVertex2Color[effectIndex] = pData->mVertex2Color;
   mVertex3Color[effectIndex] = pData->mVertex3Color;
   mVertex4Color[effectIndex] = pData->mVertex4Color;
   mTintColor[effectIndex] = pData->mTintColor;

   mBeamProgressionAlphaSelector[effectIndex] = pData->mBeamProgressionAlphaSelector;
      
   if (effectIndex == eRenderTerrainPatch)
   {
      mHeightfieldSampler[effectIndex] = gTerrainHeightField.getHeightFieldTex();
      const BTerrainHeightField::BHeightFieldAttributes& attribs = gTerrainHeightField.getHeightFieldAttributes();

      mWorldToHeightfieldParam[effectIndex] = attribs.mWorldToNormZ;
      const float yLowRange = 10.0f;
      const float yHighRange = 10.0f;
      mHeightfieldYScaleOfsParam[effectIndex] = BVec4((float)attribs.mWorldRangeY, (float)attribs.mWorldMinY, yLowRange, yHighRange);
      mTerrainDecalYOffsetParam[effectIndex] = BVec4(pData->mTerrainDecalYOffset, 0,0,0);
   }
   else
   {
      mHeightfieldSampler[effectIndex] = NULL;
   }

   //-- Pallette Color
   mPalletteToggle[effectIndex] = pData->mPalletteToggle;
   mPalletteColorCount[effectIndex] = (int) pData->mNumPalletteColors;   
   
   const bool lightBufferingEnabled = pData->mLightBuffering && (gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture) != NULL);
   mLightBufferingEnabled[effectIndex] = lightBufferingEnabled;
   if (lightBufferingEnabled)
   {
      mLightBufferSampler[effectIndex] = gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture);
      
      // Compute overall intensity scale
      const float lightBufferIntensityScale = 
         gPSManager.getLightBufferIntensityScale() * gVisibleLightManager.getLightBufferIntensityScale() * pData->mLightBufferIntensityScale;
      mLightBufferIntensityScale[effectIndex] = lightBufferIntensityScale;
      
      const XMMATRIX& worldToLightBuffer = gVisibleLightManager.getWorldToLightBuffer();
      
      BVec4 cols[3];
      cols[0].set(worldToLightBuffer.m[0][0], worldToLightBuffer.m[1][0], worldToLightBuffer.m[2][0], worldToLightBuffer.m[3][0]);
      cols[1].set(worldToLightBuffer.m[0][1], worldToLightBuffer.m[1][1], worldToLightBuffer.m[2][1], worldToLightBuffer.m[3][1]);
      cols[2].set(worldToLightBuffer.m[0][2], worldToLightBuffer.m[1][2], worldToLightBuffer.m[2][2], worldToLightBuffer.m[3][2]);
      
      mWorldToLightBufCols[effectIndex].setArray(cols, 3);
   }
   else
   {
      mLightBufferSampler[effectIndex] = NULL;
   }

   //-- Soft Particle Support
   if (pData->mSoftParticleToggle)
   {
      mDepthTextureSampler[effectIndex] = gTiledAAManager.getDepthTexture();   
      mSoftParticleParams[effectIndex] = pData->mSoftParticleParams;
      mSoftParticleParams2[effectIndex] = pData->mSoftParticleParams2;
   }
   else
   {
      mDepthTextureSampler[effectIndex] = NULL;
   }
         
   mMultiTextureEnable3Layers[effectIndex] = pData->mMultiTexture3LayersToggle;
   mMultiTextureBlendMultiplier[effectIndex] = pData->mMultiTextureBlendMultiplier;      

   BFXLEffectTechnique* pTechnique = &mTechnique[effectIndex];   
   if (pData->mMultiTextureToggle)
   { 
      pTechnique = &mMultiTextureTechnique[effectIndex];
   }

   if (pData->mSoftParticleToggle)
   {
      if (pData->mMultiTextureToggle)
         pTechnique = &mSoftParticleMultiTextureTechnique[effectIndex];
      else
         pTechnique = &mSoftParticleTechnique[effectIndex];
   }
   
   /*
   static bool bDumpStates = false;
   if (bDumpStates)
   {      
      BD3DDeviceStateDumper::dumpState(BD3DDeviceStateDumper::cDumpAllStates);
   }
   */

   pTechnique->beginRestoreDefaultState();
      // sets up appropriate render states for the selected technique pass      
      pTechnique->beginPass(techniquePassIndex);
               
         setRenderStatesForPass((eRenderBlendMode)pData->mBlendMode);

         if (effectIndex == eRenderTerrainPatch)
         {
            float tessLevel = pData->mTerrainDecalTesselation;
            BD3D::mpDev->SetRenderState(D3DRS_MAXTESSELLATIONLEVEL, CAST(DWORD, tessLevel));   
            float minTessLevel = 1.0f;
            BD3D::mpDev->SetRenderState(D3DRS_MINTESSELLATIONLEVEL, CAST(DWORD, minTessLevel));   
            BD3D::mpDev->SetRenderState(D3DRS_TESSELLATIONMODE, D3DTM_CONTINUOUS);

            pTechnique->commit();
            //-- set the palette colors
            BD3D::mpDev->SetVertexShaderConstantF(0, (const float*)&pData->mPalletteColor, cMaxPalletteColors);
            if ((pData->mpIB!=NULL) && (indexCount > 0))
            {
               int startIndex = pData->mStartOffsetIB >> 1;
               BD3D::mpDev->DrawIndexedTessellatedPrimitive(D3DTPT_QUADPATCH, 0, startIndex, indexCount);
            }
            else
               BD3D::mpDev->DrawTessellatedPrimitive(D3DTPT_QUADPATCH, 0, vertCount);
         }
         else
         {
            pTechnique->commit();
            //-- set the palette colors
            BD3D::mpDev->SetVertexShaderConstantF(0, (const float*)&pData->mPalletteColor, cMaxPalletteColors);
            if ((pData->mpIB!=NULL) && (indexCount > 0))
            {
               int startIndex = pData->mStartOffsetIB >> 1;
               BD3D::mpDev->DrawIndexedVertices(D3DPT_QUADLIST, 0, startIndex, indexCount);
            }
            else
               BD3D::mpDev->DrawVertices(D3DPT_QUADLIST, 0, vertCount);
         }

      pTechnique->endPass();
   pTechnique->end();

   BD3D::mpDev->SetVertexDeclaration(NULL);
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetIndices(NULL);   
      
   float cDefaultTessLevel = 1.0f;
   BD3D::mpDev->SetRenderState(D3DRS_MAXTESSELLATIONLEVEL, CAST(DWORD, cDefaultTessLevel));         
   BD3D::mpDev->SetRenderState(D3DRS_MINTESSELLATIONLEVEL, CAST(DWORD, cDefaultTessLevel));   
   BD3D::mpDev->SetRenderState(D3DRS_TESSELLATIONMODE, D3DTM_DISCRETE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_ONE);
   BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_ZERO);
   BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE); 

   for (int k = 0; k < 8; ++k)
   {
      BD3D::mpDev->SetSamplerState(k, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      BD3D::mpDev->SetSamplerState(k, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      BD3D::mpDev->SetSamplerState(k, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
      BD3D::mpDev->SetSamplerState(k, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      BD3D::mpDev->SetSamplerState(k, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      BD3D::mpDev->SetSamplerState(k, D3DSAMP_MIPFILTER, D3DTEXF_POINT);      
   }
   
   mDiffuseTextureParam[effectIndex] = NULL; 
   mDiffuse2TextureParam[effectIndex] = NULL;
   mDiffuse3TextureParam[effectIndex] = NULL;
   mIntensityTextureParam[effectIndex] = NULL;
   mColorProgressionTextureParam[effectIndex] = NULL;
   mScaleProgressionTextureParam[effectIndex] = NULL;
   mIntensityProgressionTextureParam[effectIndex]=NULL;
   mRandomTextureParam[effectIndex] = NULL;
   mHeightfieldSampler[effectIndex] = NULL;
   mDepthTextureSampler[effectIndex] = NULL;
   mLightBufferSampler[effectIndex] = NULL;

   /*
   if (bDumpStates)
   {      
      BD3DDeviceStateDumper::dumpState(BD3DDeviceStateDumper::cDumpAllStates);      
      bDumpStates = false;
   }
   */
}

//==============================================================================
//==============================================================================
void BParticleRenderer::renderAABB(XMVECTOR min, XMVECTOR max)
{
   DWORD color = 0xFF0000FF; 
   BVector c0 = min;
   BVector c1 = __vrlimi(min, max, VRLIMI_CONST(0,0,1,0), 0);
   BVector c2 = __vrlimi(min, max, VRLIMI_CONST(0,1,0,0), 0);
   BVector c3 = __vrlimi(min, max, VRLIMI_CONST(0,1,1,0), 0);
   BVector c4 = __vrlimi(min, max, VRLIMI_CONST(1,0,0,0), 0);
   BVector c5 = __vrlimi(min, max, VRLIMI_CONST(1,0,1,0), 0);
   BVector c6 = __vrlimi(min, max, VRLIMI_CONST(1,1,0,0), 0);
   BVector c7 = max;

   
   BPrimDraw2D::drawLine(c0, c1, color, color);
   BPrimDraw2D::drawLine(c2, c3, color, color);
   BPrimDraw2D::drawLine(c2, c0, color, color);
   BPrimDraw2D::drawLine(c3, c1, color, color);

   BPrimDraw2D::drawLine(c6, c7, color, color);
   BPrimDraw2D::drawLine(c4, c5, color, color);
   BPrimDraw2D::drawLine(c5, c7, color, color);
   BPrimDraw2D::drawLine(c6, c4, color, color);

   BPrimDraw2D::drawLine(c2, c6, color, color);
   BPrimDraw2D::drawLine(c3, c7, color, color);
   BPrimDraw2D::drawLine(c1, c5, color, color);
   BPrimDraw2D::drawLine(c0, c4, color, color);
}

//==============================================================================
//==============================================================================
void BParticleRenderer::setRenderStatesForPass(eRenderBlendMode blendMode)
{
   switch (blendMode)
   {
      case eRenderBlendMode_Alphablend:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_SRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_INVSRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
            break;
         }         
      case eRenderBlendMode_Additive:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
            break;
         }

      case eRenderBlendMode_Subtractive:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_SRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_REVSUBTRACT);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);            
            break;
         }

      case eRenderBlendMode_Distortion:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
            BD3D::mpDev->SetRenderState(D3DRS_ZENABLE,                  TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ZFUNC,                    D3DCMP_LESSEQUAL);            
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);                                                
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);          
            break;
         }

      case eRenderBlendMode_PremultiplyAlpha:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE,             FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,         TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE,          FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,                 D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,                D3DBLEND_INVSRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,                  D3DBLENDOP_ADD);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,                 0);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC,                D3DCMP_GREATER);
            BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,                 D3DCULL_NONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);            
            break;
         }

      default:
         BFAIL(0);
         break;
   }
}

//==============================================================================
//==============================================================================
int BParticleRenderer::computeTechniquePass(const BParticleEmitterRenderCommand* pCommand)
{   
   if (!pCommand)
      return -1;

   int techniquePass = -1;

   bool bLightBufferingEnabled = pCommand->mLightBuffering && (gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture) != NULL);

   if (pCommand->mMultiTextureToggle || pCommand->mMultiTexture3LayersToggle)
   {
      if (pCommand->mMultiTexture3LayersToggle)
      {
         if (pCommand->mSoftParticleToggle)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:       techniquePass = eRenderPassSoftMulti_Alphablend; break;
               case eRenderBlendMode_Additive:         techniquePass = eRenderPassSoftMulti_Additive; break;
               case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSoftMulti_Subtractive; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSoftMulti_Distortion; break;
               case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSoftMulti_PremultipliedAlpha; break;
               default: techniquePass = eRenderPassSoftMulti_Alphablend;
            }
         }
         else
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:       techniquePass = eRenderPassMulti_Alphablend; break;
               case eRenderBlendMode_Additive:         techniquePass = eRenderPassMulti_Additive; break;
               case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassMulti_Subtractive; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassMulti_Distortion; break;
               case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassMulti_PremultipliedAlpha; break;
               default: techniquePass = eRenderPassMulti_Alphablend;
            }
         }
      }
      else
      {
         if (pCommand->mSoftParticleToggle)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:       techniquePass = eRenderPassSoftMulti_Alphablend; break;
               case eRenderBlendMode_Additive:         techniquePass = eRenderPassSoftMulti_Additive; break;
               case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSoftMulti_Subtractive; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSoftMulti_Distortion; break;
               case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSoftMulti_PremultipliedAlpha; break;
               default: techniquePass = eRenderPassSoftMulti_Alphablend;
            }
         }
         else
         {
            if (bLightBufferingEnabled && pCommand->mIntensityMaskToggle)
            {
               switch (pCommand->mBlendMode)
               {
                  case eRenderBlendMode_Alphablend:  
                  case eRenderBlendMode_Subtractive:
                     {
                        techniquePass = eRenderPassMulti2_Alphablend_LightBuffer_IntensityMask; 
                        break;
                     }
                  case eRenderBlendMode_Additive:
                  case eRenderBlendMode_PremultiplyAlpha:
                     {
                        techniquePass = eRenderPassMulti2_Additive_LightBuffer_IntensityMask; 
                        break;
                     }
                  //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassMulti2_Subtractive_LightBuffer_IntensityMask; break;
                  case eRenderBlendMode_Distortion:       techniquePass = eRenderPassMulti2_Distortion; break;
                  //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassMulti2_PremultipliedAlpha_LightBuffer_IntensityMask; break;
                  default: techniquePass = eRenderPassMulti2_Alphablend_LightBuffer_IntensityMask;
               }
            }
            else if (bLightBufferingEnabled)
            {
               switch (pCommand->mBlendMode)
               {
                  case eRenderBlendMode_Alphablend:   
                  case eRenderBlendMode_Subtractive:
                     {
                        techniquePass = eRenderPassMulti2_Alphablend_LightBuffer; 
                        break;
                     }
                  case eRenderBlendMode_Additive:
                  case eRenderBlendMode_PremultiplyAlpha:
                     {
                        techniquePass = eRenderPassMulti2_Additive_LightBuffer; 
                        break;
                     }
                  //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassMulti2_Subtractive_LightBuffer; break;
                  case eRenderBlendMode_Distortion:       techniquePass = eRenderPassMulti2_Distortion; break;
                  //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassMulti2_PremultipliedAlpha_LightBuffer; break;
                  default: techniquePass = eRenderPassMulti2_Alphablend_LightBuffer;
               }
            }
            else if (pCommand->mIntensityMaskToggle)
            {
               switch (pCommand->mBlendMode)
               {
                  case eRenderBlendMode_Alphablend:
                  case eRenderBlendMode_Subtractive:
                     {
                        techniquePass = eRenderPassMulti2_Alphablend_IntensityMask; 
                        break;
                     }
                  case eRenderBlendMode_Additive:
                  case eRenderBlendMode_PremultiplyAlpha:
                     {
                        techniquePass = eRenderPassMulti2_Additive_IntensityMask; 
                        break;
                     }
                  //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassMulti2_Subtractive_IntensityMask; break;
                  case eRenderBlendMode_Distortion:       techniquePass = eRenderPassMulti2_Distortion; break;
                  //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassMulti2_PremultipliedAlpha_IntensityMask; break;
                  default: techniquePass = eRenderPassMulti2_Alphablend_IntensityMask;
               }
            }
            else
            {
               switch (pCommand->mBlendMode)
               {
                  case eRenderBlendMode_Subtractive:
                  case eRenderBlendMode_Alphablend:
                     {
                        techniquePass = eRenderPassMulti2_Alphablend; 
                        break;
                     }
                  
                  case eRenderBlendMode_Additive:
                  case eRenderBlendMode_PremultiplyAlpha:
                     {
                        techniquePass = eRenderPassMulti2_Additive; 
                        break;
                     }
                  //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassMulti2_Subtractive; break;
                  case eRenderBlendMode_Distortion:       techniquePass = eRenderPassMulti2_Distortion; break;
                  //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassMulti2_PremultipliedAlpha; break;
                  default: techniquePass = eRenderPassMulti2_Alphablend;
               }
            }
         }
      }
   }
   else
   {
      if (pCommand->mSoftParticleToggle)
      {
         if (bLightBufferingEnabled && pCommand->mIntensityMaskToggle)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:  
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSoftSingle_Alphablend_LightBuffer_IntensityMask; 
                     break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSoftSingle_Additive_LightBuffer_IntensityMask; 
                     break;
                  }
               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSoftSingle_Subtractive_LightBuffer_IntensityMask; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSoftSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSoftSingle_PremultipliedAlpha_LightBuffer_IntensityMask; break;
               default: techniquePass = eRenderPassSoftSingle_Alphablend_LightBuffer_IntensityMask;
            }
         }
         else if (bLightBufferingEnabled)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSoftSingle_Alphablend_LightBuffer; 
                     break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSoftSingle_Additive_LightBuffer; 
                     break;
                  }
               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSoftSingle_Subtractive_LightBuffer; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSoftSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSoftSingle_PremultipliedAlpha_LightBuffer; break;
               default: techniquePass = eRenderPassSingle_Alphablend_LightBuffer;
            }
         }
         else if (pCommand->mIntensityMaskToggle)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSoftSingle_Alphablend_IntensityMask; 
                     break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSoftSingle_Additive_IntensityMask; 
                     break;
                  }
               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSoftSingle_Subtractive_IntensityMask; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSoftSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSoftSingle_PremultipliedAlpha_IntensityMask; break;
               default: techniquePass = eRenderPassSoftSingle_Alphablend_IntensityMask;
            }
         }
         else
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:   
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSoftSingle_Alphablend; 
                     break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSoftSingle_Additive; 
                     break;
                  }
               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSoftSingle_Subtractive; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSoftSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSoftSingle_PremultipliedAlpha; break;
               default: techniquePass = eRenderPassSoftSingle_Alphablend;
            }
         }
      }
      else
      {
         if (bLightBufferingEnabled && pCommand->mIntensityMaskToggle)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSingle_Alphablend_LightBuffer_IntensityMask; 
                     break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSingle_Additive_LightBuffer_IntensityMask; 
                     break;
                  }

               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSingle_Subtractive_LightBuffer_IntensityMask; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSingle_PremultipliedAlpha_LightBuffer_IntensityMask; break;
               default: techniquePass = eRenderPassSingle_Alphablend_LightBuffer_IntensityMask;
            }
         }
         else if (bLightBufferingEnabled)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSingle_Alphablend_LightBuffer; 
                     break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSingle_Additive_LightBuffer; break;
                  }
               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSingle_Subtractive_LightBuffer; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSingle_PremultipliedAlpha_LightBuffer; break;
               default: techniquePass = eRenderPassSingle_Alphablend_LightBuffer;
            }
         }
         else if (pCommand->mIntensityMaskToggle)
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSingle_Alphablend_IntensityMask; 
                     break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSingle_Additive_IntensityMask; 
                     break;
                  }
               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSingle_Subtractive_IntensityMask; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSingle_PremultipliedAlpha_IntensityMask; break;
               default: techniquePass = eRenderPassSingle_Alphablend_IntensityMask;
            }
         }
         else
         {
            switch (pCommand->mBlendMode)
            {
               case eRenderBlendMode_Alphablend:
               case eRenderBlendMode_Subtractive:
                  {
                     techniquePass = eRenderPassSingle_Alphablend; break;
                  }
               case eRenderBlendMode_Additive:
               case eRenderBlendMode_PremultiplyAlpha:
                  {
                     techniquePass = eRenderPassSingle_Additive; 
                     break;
                  }
               //case eRenderBlendMode_Subtractive:      techniquePass = eRenderPassSingle_Subtractive; break;
               case eRenderBlendMode_Distortion:       techniquePass = eRenderPassSingle_Distortion; break;
               //case eRenderBlendMode_PremultiplyAlpha: techniquePass = eRenderPassSingle_PremultipliedAlpha; break;
               default: techniquePass = eRenderPassSingle_Alphablend;
            }
         }
      }
   }

   return techniquePass;
}
