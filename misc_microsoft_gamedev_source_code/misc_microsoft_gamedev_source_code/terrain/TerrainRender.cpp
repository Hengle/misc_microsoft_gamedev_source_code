//============================================================================
//
//  TerrainRender.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "TerrainPCH.h"
#include "TerrainRender.h"
#include "TerrainVisual.h"
#include "TerrainTexturing.h"
#include "TerrainQuadNode.h"
#include "TerrainMetric.h"
#include "TerrainDynamicAlpha.h"

// xcore
#include "consoleOutput.h"
#include "reloadManager.h"
#include "threading\workDistributor.h"

// xrender
#include "renderThread.h"
#include "bd3d.h"
#include "renderDraw.h"
#include "effect.h"

#include "renderEventClasses.h"
#include "asyncFileManager.h"
#include "vertexTypes.h"
#include "effectFileLoader.h"

#include "gpuHeap.h"
#include "DCBManager.h"

// xgameRender
#include "render.h"
#include "occlusion.h"
#include "tiledAA.h"

//xsystem
#include "timelineprofiler.h"
class BProfileSystem;

// shaders
#include "defConstRegs.inc"
#include "..\shaders\terrain\gpuTerrainshaderRegs.inc"
#include "..\shaders\shared\localLightingRegs.inc"
#if TERRAIN_SHADER_REGS_VER != 101
   #error Please update gpuTerrainShaderRegs.inc
#endif

#ifndef BUILD_FINAL
#define MIPVIS_TEXTURE_PATH "test\\sw_texel_01\\red1x1_02_df"
static BManagedTextureHandle gMipVisTextureHandle = cInvalidManagedTextureHandle;
#endif   

// Max # of chunk lights = Max # of local light shader constants / 8
const uint cMaxChunkLocalLights = (NUM_LOCAL_LIGHT_PSHADER_CONSTANTS >> 3);

const uint cLightAttribTexWidth     = 128;
const uint cLightAttribTexWidthLog2 = 7;
const uint cLightAttribTexHeight    = 128;

#define EFFECT_FILENAME "terrain\\gpuTerrainXbox.bin"

//--------------------------------------
BTerrainRender gTerrainRender;

//--------------------------------------

//============================================================================
// BTerrainRender::BTerrainRender
//============================================================================
BTerrainRender::BTerrainRender():
   mEventHandle(cInvalidEventReceiverHandle),
   mVertexBuffer(0),
   mPhase(cTRP_Invalid),
   mRendering(false),
   mpEffectLoader(NULL),
   mpLightAttribTexture(NULL),
   mNextLightAttribTexelIndex(0),
   mAODiffuseIntensity(0.0f),
   mThreadedNodeDCBCreationIssued(FALSE),
   mDCBRenderingEnabled(true),
   mDCBRenderingToggle(false),
   mDCBRenderingActive(false),
   mPredicatedRendering(false)
{
   Utils::ClearObj(mSavedRenderTargetSurf);
   Utils::ClearObj(mSavedDepthStencilSurf);
} 
//============================================================================
// BTerrainRender::BTerrainRender
//============================================================================
BTerrainRender::~BTerrainRender()
{
}
//============================================================================
// BTerrainRender::init
//============================================================================
bool BTerrainRender::init(void)
{
   ASSERT_MAIN_THREAD

   if (mEventHandle == cInvalidEventReceiverHandle)
      mEventHandle = gEventDispatcher.addClient(this, cThreadIndexRender);
         
   if (mCommandListenerHandle == cInvalidCommandListenerHandle)
      mCommandListenerHandle = gRenderThread.registerCommandListener(this);

   Utils::ClearObj(mTempHDRScales);
   Utils::ClearObj(mTempHDRScales);
   Utils::ClearObj(mTempLayerUVs);
   
   mBlackmapParams.clear();
   
   return true;
}
//============================================================================
// BTerrainRender::deinit
//============================================================================
bool BTerrainRender::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   // Block for safety. 
   gRenderThread.blockUntilGPUIdle();
   
   gReloadManager.deregisterClient(mEventHandle);

   if (mCommandListenerHandle != cInvalidCommandListenerHandle)
   {
      gRenderThread.freeCommandListener(mCommandListenerHandle);
      mCommandListenerHandle = cInvalidCommandListenerHandle;
   }

   if (mEventHandle != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientDeferred(mEventHandle, true);
      mEventHandle = cInvalidEventReceiverHandle;
   }
   

   mVertexBuffer = 0;
 
   mPhase = cTRP_Invalid;
   mRendering = false;



   return true;
}

//============================================================================
// BTerrainRender::allocateTempResources
//============================================================================
void BTerrainRender::allocateTempResources(void)
{
   tickEffect();

   BDEBUG_ASSERT(!mpLightAttribTexture);
   mpLightAttribTexture = gRenderDraw.createDynamicTexture(cLightAttribTexWidth, cLightAttribTexHeight, D3DFMT_LIN_A32B32G32R32F);
   BVERIFY(mpLightAttribTexture);
   
   mExtendedLocalLightSampler = mpLightAttribTexture;
   
   mNextLightAttribTexelIndex = 0;
}

//============================================================================
// BTerrainRender::releaseTempResources
//============================================================================
void BTerrainRender::releaseTempResources(void)
{
   BDEBUG_ASSERT(mpLightAttribTexture);
   mpLightAttribTexture = NULL;
   
   mExtendedLocalLightSampler = NULL;
}

//============================================================================
// BTerrainRender::computeLightAttribTexelPtr
//============================================================================
XMFLOAT4* BTerrainRender::computeLightAttribTexelPtr(XMFLOAT4* pDstTexture, uint texelIndex)
{
   const uint x = texelIndex & (cLightAttribTexWidth - 1);
   const uint y = texelIndex >> cLightAttribTexWidthLog2;
   const uint dstBlockOfs = x + (y << cLightAttribTexWidthLog2);
   XMFLOAT4* pDst = pDstTexture + dstBlockOfs;
   return pDst;
}   

//============================================================================
// BTerrainRender::fillLightAttribTexture
//============================================================================
uint BTerrainRender::fillLightAttribTexture(const BSceneLightManager::BActiveLightIndexArray& lights, uint firstIndex, uint& outNumLights)
{
   // Fills the light attribute texture with the "extended" light constants that influence this chunk. We only send the first
   // four out of eight constants per light (no shadowing).
   const uint cTexelsPerLight = 4;
   const uint cTexelsPerLightLog2 = 2;
   
   const uint totalLightAttribTexels = cLightAttribTexWidth * cLightAttribTexHeight;
   const uint numLightAttribTexelsRemaining = totalLightAttribTexels - mNextLightAttribTexelIndex;
   const uint numLights = Math::Min(numLightAttribTexelsRemaining >> cTexelsPerLightLog2, lights.getSize() - firstIndex);
   outNumLights = numLights;
   if (!numLights)
      return 0;
      
   const uint lightAttribTexelIndex = mNextLightAttribTexelIndex;
   mNextLightAttribTexelIndex += numLights * cTexelsPerLight;
   
   XMFLOAT4* pDstTexture = (XMFLOAT4*)gRenderDraw.getResourceAddress(mpLightAttribTexture, false);   
   XMFLOAT4* pDstBegin = computeLightAttribTexelPtr(pDstTexture, lightAttribTexelIndex);
   XMFLOAT4* pDst = pDstBegin;
   
   const XMFLOAT4* pSrc = gVisibleLightManager.getVisibleLightTexels(true);
   
   for (uint i = 0; i < numLights; i++)
   {
      const BVisibleLightManager::BVisibleLightIndex visibleLightIndex = lights[firstIndex + i];
                     
      for (uint j = 0; j < cTexelsPerLight; j++)
         *pDst++ = pSrc[visibleLightIndex * 8 + j];
   }
      
   BD3D::mpDev->InvalidateGpuCache(pDstBegin, numLights * cTexelsPerLight * sizeof(XMFLOAT4), 0);
         
   return lightAttribTexelIndex;
}

//============================================================================
// BTerrainRender::beginRender
//============================================================================
void BTerrainRender::beginRender(eTRenderPhase renderPhase)
{
   // rg [12/2/07] - Cannot render normally while concurrently rendering with DCB's, because we only have a single effect pool.
   BDEBUG_ASSERT(!mDCBRenderingActive);

   mIntrinsicPool = gEffectIntrinsicManager.getRenderEffectIntrinsicPool();
   
   tickEffect();
   
   beginRenderInternal(renderPhase);
}

//============================================================================
// BTerrainRender::beginRenderInternal
//============================================================================
void BTerrainRender::beginRenderInternal(eTRenderPhase renderPhase)
{
   mTerrainGPUShader.updateIntrinsicParams();
   
   BDEBUG_ASSERT(!mRendering);
   mRendering = true;        
            
   mpUsingDevice->SetStreamSource( 0, mVertexBuffer, 0, sizeof(float)*3);
   
   // Dummy vertex decl
   mpUsingDevice->SetVertexDeclaration(BTLVertex::msVertexDecl);
   
   if (renderPhase != cTRP_ShadowGen)
   {
      mpUsingDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
      
      if (!mDCBRenderingActive)
         BD3D::mpDev->SetShaderGPRAllocation(0, 32, 96);
   }
   else
   {
      if (!mDCBRenderingActive)
         BD3D::mpDev->SetShaderGPRAllocation(0, 96, 32);
   }
 
   float rcpVal = 1.0f / gTerrainVisual.getNumXVerts();
   mShaderRcpMapSizeHandle=rcpVal;

   // Our position texture
   mPosComprRange = gTerrainVisual.mVisualData->mPosCompRange;
   mPosComprMin = gTerrainVisual.mVisualData->mPosCompMin;

   mShaderPositionTextureHandle = gTerrainVisual.getPosTex();
   mShaderNormalTextureHandle = gTerrainVisual.getNormTex();
   if (gTerrainVisual.getLightTex())
      mShaderLightingTextureHandle = gTerrainVisual.getLightTex();
   else
      mShaderLightingTextureHandle =  gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);

   mShaderAOTextureHandle = gTerrainVisual.getAOTex();
   mShaderAlphaTextureHandle = gTerrainVisual.getAlphaTex();
   
   mShaderDynamicAlphaTextureHandle        = gTerrainDynamicAlpha.getDynamicAlphaTexture();
   mShaderDynamicAlphaTextureWidth         = (float)gTerrainDynamicAlpha.getDynamicAlphaTextureWidth();
   
   if (!mBlackmapParams.mpTexture)
      mBlackmapEnabled = false;
   else
   {
      mBlackmapEnabled = true;
      mBlackmapSampler = mBlackmapParams.mpTexture;
      mBlackmapUnexploredSampler = mBlackmapParams.mpUnexploredTexture;
      BCOMPILETIMEASSERT(sizeof(mBlackmapParams.mParams)/sizeof(mBlackmapParams.mParams[0]) == 3);
      mBlackmapParams0 = mBlackmapParams.mParams[0];
      mBlackmapParams1 = mBlackmapParams.mParams[1];
      mBlackmapParams2 = mBlackmapParams.mParams[2];
   }
   
   if (!gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture))
   {
      mEnableLightBuffering = false;
      mLightBufferColorSampler = NULL;
      mLightBufferVectorSampler = NULL;
   }
   else
   {
      mEnableLightBuffering = true;
      mWorldToLightBuf = gVisibleLightManager.getWorldToLightBuffer();
      mLightBufferColorSampler = gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferColorTexture);
      mLightBufferVectorSampler = gVisibleLightManager.getLightBuffer(BVisibleLightManager::cLightBufferVectorTexture);
   }
   
   if (renderPhase != cTRP_ShadowGen)
      mpUsingDevice->GpuOwnPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);
      
   setRenderPhase(renderPhase);
   
   mCurrTechnique.begin(0);
}
//============================================================================
// BTerrainRender::endRender
//============================================================================
void BTerrainRender::endRender()
{
   if (!mRendering)
   {
      // Effect compilation is still pending, nothing to do.
      return;
   }
         
   if (mPhase != cTRP_ShadowGen)
   {
      mpUsingDevice->GpuDisownPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);
      
      mpUsingDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   }
      
   mCurrTechnique.end();
   
   mpUsingDevice->SetIndices(0);
   mpUsingDevice->SetPixelShader(NULL);
   mpUsingDevice->SetVertexShader(NULL);
    
   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
   {
      mpUsingDevice->SetTexture(i, NULL);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_SEPARATEZFILTERENABLE, FALSE);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
      mpUsingDevice->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1);
   }

   if (!mDCBRenderingActive)
      BD3D::mpDev->SetShaderGPRAllocation(0, 64, 64);
      
   mpUsingDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
   mpUsingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
   mpUsingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   mpUsingDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   
   mBlackmapSampler = NULL;
   mBlackmapUnexploredSampler = NULL;
   
   mRendering = false;
}
//============================================================================
// BTerrainRender::setVisControlRegs
//============================================================================
void BTerrainRender::setVisControlRegs()
{
   // All HLSL variables set manually here MUST be marked for manual register update in initEffectConstants!

   #ifndef BUILD_FINAL         
   switch (BTerrain::getVisMode())
   {
      case cVMDisabled:
      {
         break;
      }
      default:
      {
         const uint cNumControlValues = 16;
         float control[cNumControlValues];
         Utils::ClearObj(control);

         // rg [12/14/07] - This limits the max # of control values to 16, which isn't quite enough, but whatever.
         if ((BTerrain::getVisMode() - cVMAlbedo) < cNumControlValues)
            control[BTerrain::getVisMode() - cVMAlbedo] = 1.0f;

         mpUsingDevice->SetPixelShaderConstantF(VIS_CONTROL_0_REG, control, 4);                     

         break;
      }
   }
#endif  
}

//============================================================================
// BTerrainRender::setupTexturing
//============================================================================
inline int BTerrainRender::setupTexturing(BTerrainTexturingRenderData *pTexturingData,bool isSmallNode)
{
   int passNum =cRenderPassAN; 
   mNumLayers=0;

   //CLM [09.04.07] This is now needed due to dynamic alpha from buildings going underground...
  // if(pTexturingData->mLayerContainer.mAlphaPassNeeded)
   {
       if (mPhase == cTRP_ShadowGen)
       {
          mpUsingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
          mpUsingDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
          mpUsingDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
          mpUsingDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
          mpUsingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

          static bool dithered = true;
          mpUsingDevice->SetRenderState(D3DRS_ALPHATOMASKOFFSETS, dithered ? D3DALPHATOMASK_DITHERED : D3DALPHATOMASK_SOLID);

          mpUsingDevice->SetRenderState(D3DRS_ALPHAREF,128);
       }
       else
       {
          //mpUsingDevice->SetRenderState(D3DRS_ALPHATOMASKOFFSETS, D3DALPHATOMASK_DITHERED);
          mpUsingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
          mpUsingDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
          mpUsingDevice->SetRenderState(D3DRS_ALPHAREF, 0x000000AA); 

          mpUsingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
          mpUsingDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
          mpUsingDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
          mpUsingDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
          //     mpUsingDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, true);
       }
   }
  /* else
   {
      mpUsingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
      mpUsingDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
      mpUsingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      
   }*/
      

   if (mPhase == cTRP_ShadowGen || !gTerrainTexturing.getOKToTexture() )
   {
      if ((pTexturingData) && (pTexturingData->mLayerContainer.mAlphaPassNeeded))
         return passNum+1;

      return passNum;
   }

   if(isSmallNode || !pTexturingData)
   {
      passNum =cRenderPassSmallChunk; 
      mShaderDistFromCameraScalar = 1;

#ifndef BUILD_FINAL         
      if(BTerrain::getTextureMode()== cTMAllWhite)
         mShaderUniqueAlbedoTexHandle=gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);
      else if(BTerrain::getTextureMode()== cTMMipVis)
      {
         BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(gMipVisTextureHandle);

         mShaderUniqueAlbedoTexHandle=pTexture->getD3DTexture().getBaseTexture();
      }
      else 
#endif
      mShaderUniqueAlbedoTexHandle= gTerrainTexturing.getLargeUniqueAlbedoTexture() ? 
                                                       gTerrainTexturing.getLargeUniqueAlbedoTexture():
                                                       gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);
   }
   else// if (mPhase != cTRP_ShadowGen && gTerrainTexturing.getOKToTexture() )
   {

      BASSERT(pTexturingData->mCachedUniqueTexture);
		
      //albedo
#ifndef BUILD_FINAL         
      if(BTerrain::getTextureMode()== cTMAllWhite)
         mShaderUniqueAlbedoTexHandle=gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);
      else if(BTerrain::getTextureMode()== cTMMipVis)
      {
         BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(gMipVisTextureHandle);

        mShaderUniqueAlbedoTexHandle=pTexture->getD3DTexture().getBaseTexture();
      }
      else 
#endif
      mShaderUniqueAlbedoTexHandle=pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeAlbedo];

      //normal
#ifndef BUILD_FINAL      
      if (BTerrain::getTextureMode()== cTMAllFlat)
         mShaderUniqueNormalTexHandle= gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureNormal);
      else
#endif      
      mShaderUniqueNormalTexHandle= pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeNormal]?
                                                  pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeNormal]:
                                                  gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureNormal);

      //spec
      if(pTexturingData->mLayerContainer.mSpecPassNeeded 
         && gTerrainTexturing.getActiveTextureArray(cTextureTypeSpecular))
      {
         passNum=cRenderPassANS;
         mShaderUniqueSpecularTexHandle= pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeSpecular];

         mShaderSpecPowerHandle                 = gTerrainTexturing.mSpecExponentPower;
         mShaderSpecOnlyDir_IntHandle           = gTerrainTexturing.mSpecOnlyDir_Dir_Int;
         mShaderSpecOnlyCol_ShadHandle          = gTerrainTexturing.mSpecOnlyDir_Col_Shad;
      }
      else
      {
         mShaderUniqueSpecularTexHandle= gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);
      }

      mShaderBumpPowerHandle                 = gTerrainTexturing.mBumpPower;
      mShaderAODiffuseIntensityHandle        = mAODiffuseIntensity;
                                                  

      mShaderEnvMapTexHandle                 = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);
      if((pTexturingData->mLayerContainer.mEnvMaskPassNeeded
         && gTerrainTexturing.getEnvMapTexture()->getBaseTexture()
         && gTerrainTexturing.getActiveTextureArray(cTextureTypeEnvMask))
         #ifndef BUILD_FINAL
         || BTerrain::getVisMode() ==cVMEnvMask
         || BTerrain::getVisMode() ==cVMEnv
         #endif
         )
      {
         mShaderUniqueEnvMaskTexHandle= pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeEnvMask] && pTexturingData->mLayerContainer.mEnvMaskPassNeeded?
                                                   pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeEnvMask]:
                                                gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);

          if(gTerrainTexturing.getEnvMapTexture()->getBaseTexture())
          {
             mShaderEnvMapTexHandle                 = gTerrainTexturing.getEnvMapTexture()->getBaseTexture();
             mShaderEnvMapHDRHandle                 = gTerrainTexturing.getEnvMapHDRScale();
          }
          else
          {
             mShaderEnvMapTexHandle                 = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);
             mShaderEnvMapHDRHandle                 = 0;
          }
           

            passNum = cRenderPassANSE;
      }


      //CLM hack for SELF map fadeout to small cache
      //this should be refactored in a MUCH cleaner form...
      if (gTerrainTexturing.getLODEnabled())
      {
         float distFromCamera = pTexturingData->mCachedUniqueTexture->mpOwnerQuadNode->mCacheInfo.mDistanceFromCamera;
         distFromCamera =  1-(distFromCamera / gTextureLODDistance[cNumMainCacheLevels-1]);

         mShaderDistFromCameraScalar = distFromCamera;
      }   

      if(pTexturingData->mLayerContainer.mSelfPassNeeded
         && gTerrainTexturing.getActiveTextureArray(cTextureTypeSelf)
         #ifndef BUILD_FINAL
         || BTerrain::getVisMode() ==cVMSelf
         #endif
         )
      {

         mShaderUniqueSelfTexHandle =pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeSelf] && pTexturingData->mLayerContainer.mSelfPassNeeded?
                                                            pTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeSelf]:
                                                            //put in a different texture based upon if we're compressed or not
                                                            (pTexturingData->mCachedUniqueTexture->mWidth > cTexSize360Cutoff)? 
                                                                  gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite):
                                                                  gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);


                  

         passNum=cRenderPassANSR;

         mShaderUniqueSelfTexPacked = pTexturingData->mCachedUniqueTexture->mWidth > cTexSize360Cutoff;

         if(pTexturingData->mLayerContainer.mEnvMaskPassNeeded)
            passNum=cRenderPassFull;
      }

    
      

#ifndef BUILD_FINAL         
      switch (BTerrain::getVisMode())
      {
      case cVMDisabled:
         {
            break;
         }
      default:
         {
            passNum=cRenderPassVis;
           //GOOD STUFF IS DONE IN   setVisControlRegs()
            break;
         }
      }
#endif  
   }

   return passNum;

}
//============================================================================
// BTerrainRender::setupLighting
//============================================================================
inline int BTerrainRender::setupLocalLighting(const BTerrainQuadNodeDesc& desc)
{
   BDEBUG_ASSERT(mPhase != cTRP_ShadowGen);
         
   BOOL boolValueFALSE = FALSE;
   BOOL boolValueTRUE = TRUE;
         
   const XMVECTOR boundsMin = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&desc.m_min));
   const XMVECTOR boundsMax = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&desc.m_max));
         
   BSceneLightManager::BGridRect gridRect = gRenderSceneLightManager.getGridRect(boundsMin, boundsMax);
   
   mCurVisibleLightIndices.resize(0);
   gVisibleLightManager.findLights(mCurVisibleLightIndices, gridRect, boundsMin, boundsMax, true, true);
      
   if (mCurVisibleLightIndices.isEmpty())
   {
      mpUsingDevice->SetPixelShaderConstantB(ENABLE_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
      mpUsingDevice->SetPixelShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, &boolValueFALSE, 1);
      mpUsingDevice->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
      
      return 0;  
   }
   
   const uint cMaxLights = 256;
   if (mCurVisibleLightIndices.getSize() > cMaxLights)
      mCurVisibleLightIndices.resize(cMaxLights);
   
   uint numShadowedLights = 0;
   uint numUnshadowedLights = 0;
   ushort unshadowedLightIndices[cMaxLights];
   
   // Reorder the indices so the shadowed lights appear first.
   for (uint i = 0; i < mCurVisibleLightIndices.getSize(); i++)
   {
      const BVisibleLightManager::BVisibleLightIndex visibleLightIndex = mCurVisibleLightIndices[i];
      if (gVisibleLightManager.getVisibleLightShadows(visibleLightIndex))
      {
         mCurVisibleLightIndices[numShadowedLights] = (WORD)visibleLightIndex;
         numShadowedLights++;
      }
      else
      {
         unshadowedLightIndices[numUnshadowedLights] = static_cast<ushort>(visibleLightIndex);
         numUnshadowedLights++;
      }
   }
         
   if (numShadowedLights)
   {
      for (uint i = 0; i < numUnshadowedLights; i++)
         mCurVisibleLightIndices[numShadowedLights + i] = unshadowedLightIndices[i];
   }         
            
   const uint totalLights = mCurVisibleLightIndices.getSize();
   const uint numNormalLights = Math::Min<uint>(totalLights, cMaxChunkLocalLights);
   const uint cMaxExtendedLights = 250;
   const uint numExtendedLights = Math::Min<uint>(totalLights - numNormalLights, cMaxExtendedLights);
      
   mpUsingDevice->SetPixelShaderConstantB(ENABLE_LOCAL_LIGHTS_REG, numNormalLights ? &boolValueTRUE : &boolValueFALSE, 1);
      
   const XMFLOAT4* pTexels = gVisibleLightManager.getVisibleLightTexels();

   for (uint i = 0; i < numNormalLights; i++)
   {
      mpUsingDevice->GpuLoadPixelShaderConstantF4Pointer(i * 8, pTexels + 8 * mCurVisibleLightIndices[i], 8);
   }
   
   const int numLightsInt4[4] = { numNormalLights, 0, 8, 0 };
   mpUsingDevice->SetPixelShaderConstantI(NUM_LOCAL_LIGHTS_REG, numLightsInt4, 1);
   
   BOOL boolValue = numShadowedLights > 0;
   mpUsingDevice->SetPixelShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, &boolValue, 1);
      
   if (!numExtendedLights)      
   {
      mpUsingDevice->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
   }
   else
   {
      uint actualNumExtendedLights;
      uint texelIndex = fillLightAttribTexture(mCurVisibleLightIndices, numNormalLights, actualNumExtendedLights);
            
      mpUsingDevice->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, actualNumExtendedLights ? &boolValueTRUE : &boolValueFALSE, 1);
      
      if (actualNumExtendedLights)
      {
         const int numExtendedLightsInt4[4] = { actualNumExtendedLights, 0, 1, 0 };
         mpUsingDevice->SetPixelShaderConstantI(NUM_EXTENDED_LOCAL_LIGHTS_REG, numExtendedLightsInt4, 1);
         
         BVec4 extendedParams((float)texelIndex, 0.0f, 0.0f, 0.0f);
         mpUsingDevice->SetPixelShaderConstantF(EXTENDED_LOCAL_LIGHTING_PARAMS_REG, extendedParams.getPtr(), 1);
      }      
   }    
   return totalLights;
}
//============================================================================
// BTerrainRender::render3DPacketTesselated
//============================================================================

inline void BTerrainRender::render3DPacketTesselated(BTerrain3DSimpleVisualPacket *visDat, const BTerrainQuadNodeDesc& desc,int techniquePassNum, int quadrant)
{

   SCOPEDSAMPLE(BTerrainRender_render3DPacketTesselated)
   float maxTessLevel = 15.0f;

   //SET SPECIFIC VERTEX CONSTANTS :
   mShaderDataValsHandle = D3DXVECTOR4(  1.0f / (float)gTerrainVisual.getNumXVerts() , (float)gTerrainVisual.getTileScale(), (float)desc.mMinXVert, (float)desc.mMinZVert);



   if (quadrant < 0)
   {     
      D3DXMATRIX identMat;
      D3DXMatrixIdentity(&identMat);
      mShaderQuadrantMatrixHandle = identMat;

      if (mPhase != cTRP_ShadowGen)
         mpUsingDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
   }
   else
   {
      maxTessLevel = 1.0f;

      if (mPhase != cTRP_ShadowGen)
         mpUsingDevice->SetRenderState(D3DRS_CULLMODE, (quadrant & 1) ? D3DCULL_CCW : D3DCULL_CW);

      mShaderQuadrantMatrixHandle = gTerrain.mQuadrantMatrices[quadrant];
   }

   mShaderPatchOffsetHandle = D3DXVECTOR4(0,0,4.0f,0.25f);



   mpUsingDevice->SetRenderState( D3DRS_MAXTESSELLATIONLEVEL, *((DWORD*)&maxTessLevel));
   mpUsingDevice->SetRenderState(D3DRS_TESSELLATIONMODE, D3DTM_PEREDGE);
   mpUsingDevice->SetIndices(gTerrainVisual.getPatchEdgeDynamicIB());


   int minXChunk = desc.mMinXVert >> 6;
   int minZChunk = desc.mMinZVert >> 6;

   int minXPatch = minXChunk << 4;

   int patchPitch = gTerrain.getNumXChunks() << 4;
   int startIndex = (minXPatch + patchPitch * minZChunk);
   startIndex <<= 2;

   startIndex = desc.mTessPatchStartIndex;


   int tPassNum = techniquePassNum;

   int numLights =0;
   if (mPhase != cTRP_ShadowGen)
   {
      setVisControlRegs();
      
      //SCOPEDSAMPLE(BTerrainRender_render3DPacketTesselated_1x1)
      numLights = setupLocalLighting(desc);
      if(numLights< cMaxChunkLocalLights)
      {
         if(numLights==0)
            if(tPassNum < cRenderPassAN_NoLocalLighting)
               tPassNum+=cRenderPassNumTexturePass;

         mCurrTechnique.beginPass(tPassNum);
         mCurrTechnique.commitU();
         mpUsingDevice->DrawIndexedTessellatedPrimitive(D3DTPT_QUADPATCH, 0, startIndex, 16);
         mCurrTechnique.endPass();    
      }
      else if (numLights >= cMaxChunkLocalLights)
      {
      //   SCOPEDSAMPLE(BTerrainRender_render3DPacketTesselated_2x2)
         //if we've got more than cMaxChunkLocalLights local lights, split the chunk into 4 smaller chunks..

         //get local lighting for each of our 4 chunks
         D3DXVECTOR3 halfdiff = (desc.m_max - desc.m_min)*0.5f;

         BTerrainQuadNodeDesc subDesc[4];
         subDesc[0]= desc;
         subDesc[0].m_max -= halfdiff;

         subDesc[1] = desc;
         subDesc[1].m_min.x += halfdiff.x;
         subDesc[1].m_max.z -= halfdiff.z;

         subDesc[2] = desc;
         subDesc[2].m_min.z += halfdiff.z;
         subDesc[2].m_max.x -= halfdiff.x;

         subDesc[3] = desc;
         subDesc[3].m_min.x += halfdiff.x;
         subDesc[3].m_min.z += halfdiff.z;

         //CLM Patches are in raster order!
         //that means this offset won't work. 

         //IE we'll be rendering strips instead of blocks...
         const int patchAddIndex[]=     {0,16,32,48};
         const float xPatchAdd[] =      {0,32,0,32};
         const float yPatchAdd[] =      {0,0,32,32};



         for(int k=0;k<4;k++)
         {
            int tPassNum = techniquePassNum;
            numLights = setupLocalLighting(subDesc[k]);
            if(numLights==0)
               if(tPassNum < cRenderPassAN_NoLocalLighting)
                  tPassNum+=cRenderPassNumTexturePass;

            mCurrTechnique.beginPass(tPassNum);

            int patchStartIndex = startIndex + patchAddIndex[k];
            mShaderPatchOffsetHandle = D3DXVECTOR4(xPatchAdd[k],yPatchAdd[k],4.0f,0.25f);
            mCurrTechnique.commitU();
            mpUsingDevice->DrawIndexedTessellatedPrimitive(D3DTPT_QUADPATCH, 0, patchStartIndex, 4);

            mCurrTechnique.endPass();   

         }
      }
   }
   else
   {
    //  SCOPEDSAMPLE(BTerrainRender_render3DPacketTesselated_shadow)

      {
      //   SCOPEDSAMPLE(BTerrainRender_tess_beginPass)
         mCurrTechnique.beginPass(tPassNum);
      }

      {
       //  SCOPEDSAMPLE(BTerrainRender_tess_commitU)
         mCurrTechnique.commitU();
      }

      {
       //  SCOPEDSAMPLE(BTerrainRender_tess_dip)
         mpUsingDevice->DrawIndexedTessellatedPrimitive(D3DTPT_QUADPATCH, 0, startIndex, 16);
      }

      mCurrTechnique.endPass();    
   }


   mpUsingDevice->SetIndices(NULL);
}
//============================================================================
// BTerrainRender::render
//============================================================================
void BTerrainRender::render(const BTerrainQuadNode& quadNode, const BTerrainRenderPacket* handle)
{
//   ASSERT_RENDER_THREAD


   SCOPEDSAMPLE(BTerrainRender_render)


   if (!mRendering)
   {
      // Effect compilation is still pending, nothing to do.
      return;
   }

   if (mDCBRenderingActive && mPredicatedRendering)
      setDCBPredicationData(&quadNode);

   bool smallNode = (cUniqueTextureWidth >> quadNode.mCacheInfo.mCurrLODLevel) <= cTexSize360Cutoff;
   int passNum = handle->mTexturingData ? setupTexturing(handle->mTexturingData,smallNode) : 0;

   BDEBUG_ASSERT(mCurrTechnique.getNumPasses() >= (uint)passNum);

   BTerrainQuadNodeDesc desc = quadNode.getDesc();
   
   render3DPacketTesselated(handle->mVisualData,desc,passNum, -1);

}

//============================================================================
// BTerrainRender::compositeNodeInstances
//============================================================================
void BTerrainRender::renderNodeInstances(const BTerrainQuadNodePtrArray* pVisibleNodeInstances)
{
   //ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(BTerrainRender_renderNodeInstances);

   beginRenderInternal(cTRP_Full);
   
   //render chunks who's textures are still in the cache
   for(unsigned int i=0;i<(*pVisibleNodeInstances).size();i++)
   {
      const BTerrainQuadNode* node = (*pVisibleNodeInstances)[i];
      BASSERT(node);
      if (!node)
         continue;

      BDEBUG_ASSERT(!node->mSkirtInfo.mIsSkirtChunk);

      //CLM [04.25.07] Skip fully alpha'd chunks
      if ((node->mRenderPacket->mTexturingData) && (node->mRenderPacket->mTexturingData->mLayerContainer.mIsFullyOpaque))
         continue;

      if(cUniqueTextureWidth >> node->mCacheInfo.mCurrLODLevel <= cTexSize360Cutoff)
      {
         render(*node,node->mRenderPacket);
      }
      else
      {
         BASSERT((node->mRenderPacket->mTexturingData) && (node->mRenderPacket->mTexturingData->mCachedUniqueTexture));

         BASSERT(gTerrainTexturing.isTextureInCache(node->mRenderPacket->mTexturingData->mCachedUniqueTexture));

         render(*node,node->mRenderPacket);
      }
   }
   
   endRender();
}


//============================================================================
// BTerrainRender::renderSkirt
//============================================================================
void BTerrainRender::renderSkirt(const BTerrainQuadNode::skirtNodeInfo& skirtInfo)
{
   SCOPEDSAMPLE(BTerrainRender_renderSkirt)

   if (!mRendering)
   {
      // Effect compilation is still pending, nothing to do.
      return;
   }

   BTerrainQuadNode& quadNode = *skirtInfo.mpOwnerNode;
   BTerrainRenderPacket* handle = quadNode.mRenderPacket;
   int quadrant = skirtInfo.mQuadrant;

   if (mDCBRenderingActive && mPredicatedRendering)
      setDCBPredicationData(&quadNode);

   

   int passNum =cRenderPassSkirtBatched; 
   mNumLayers=0;
   mpUsingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
   mpUsingDevice->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);

   BTerrainQuadNodeDesc desc = quadNode.getDesc();


   bool isChunkOnEdge = skirtInfo.mSkirtBatchSize==1;
   //TEXTURING//////////////
   if (mPhase == cTRP_ShadowGen || !gTerrainTexturing.getOKToTexture() )
   {
      passNum = cRenderPassANS;
   }
   else
   {
     
      mpUsingDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
      mpUsingDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
      mpUsingDevice->SetRenderState(D3DRS_ALPHAREF, 0x000000AA); 

      mpUsingDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      mpUsingDevice->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
      mpUsingDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
      mpUsingDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);

      if(isChunkOnEdge)
      {
         mShaderDistFromCameraScalar = 1;

         if(mBlackmapParams.mpTexture==NULL)
         {
            if(handle->mTexturingData && handle->mTexturingData->mCachedUniqueTexture && mPhase != cTRP_Reflect)
            {
               passNum =cRenderPassSkirtHiTex;
               mShaderUniqueAlbedoTexHandle=handle->mTexturingData->mCachedUniqueTexture->mTextures[cTextureTypeAlbedo];
            }
            else
            {
               passNum =cRenderPassSkirtLowTex;
               mShaderUniqueAlbedoTexHandle= gTerrainTexturing.getLargeUniqueAlbedoTexture();   
            }
         }
         else
         {
            passNum =cRenderPassSkirtNonBatchedFog;
         }
      }
      else
      {

         mShaderDistFromCameraScalar = 1;

         if(mBlackmapParams.mpTexture!=NULL)
         {
            passNum =cRenderPassSkirtBatchedFog;
         }
         else if(gTerrainTexturing.getLargeUniqueAlbedoTexture())
         {
            passNum =cRenderPassSkirtBatched;

#ifndef BUILD_FINAL         
            if(BTerrain::getTextureMode()== cTMAllWhite)
               mShaderUniqueAlbedoTexHandle=gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);
            else if(BTerrain::getTextureMode()== cTMMipVis)
            {
               BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(gMipVisTextureHandle);

               mShaderUniqueAlbedoTexHandle=pTexture->getD3DTexture().getBaseTexture();
            }
            else 
#endif
               mShaderUniqueAlbedoTexHandle= gTerrainTexturing.getLargeUniqueAlbedoTexture();
         }
         else
         {
            return;
         }
      }


      if(quadrant>=0)
         mpUsingDevice->SetRenderState(D3DRS_CULLMODE, (quadrant & 1) ? D3DCULL_CCW : D3DCULL_CW);
      else
         mpUsingDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
      
   }

   //SKIRT MIRROR ////////////////////
   D3DXMATRIX ident; D3DXMatrixIdentity(&ident);
   mShaderQuadrantMatrixHandle = quadrant<0?ident :gTerrain.mQuadrantMatrices[quadrant];

   BDEBUG_ASSERT(mCurrTechnique.getNumPasses() >= (uint)passNum);

   

   //RENDERING //////////////////////
   const uint numBatchedXChunks = skirtInfo.mSkirtBatchSize;
   const uint numBatchedXPatches = numBatchedXChunks * 4;
   const uint numPatches = isChunkOnEdge?16:numBatchedXPatches * numBatchedXPatches;
   float maxTessLevel = isChunkOnEdge?15.0f:1.0f;


   //SET SPECIFIC VERTEX CONSTANTS :
   mShaderDataValsHandle = D3DXVECTOR4(  1.0f / (float)gTerrainVisual.getNumXVerts() , (float)gTerrainVisual.getTileScale(), (float)desc.mMinXVert, (float)desc.mMinZVert);
   mShadeNumBatchedXChunks = (float)numBatchedXChunks;
   mShaderPatchOffsetHandle = D3DXVECTOR4(0,0,4.0f,0.25f);

   
   mpUsingDevice->SetRenderState( D3DRS_MAXTESSELLATIONLEVEL, *((DWORD*)&maxTessLevel));
   mpUsingDevice->SetRenderState(D3DRS_TESSELLATIONMODE, D3DTM_PEREDGE);
   mpUsingDevice->SetIndices(gTerrainVisual.getPatchEdgeDynamicIB());

   mCurrTechnique.beginPass(passNum);
   
   mCurrTechnique.commitU();

   mpUsingDevice->DrawIndexedTessellatedPrimitive(D3DTPT_QUADPATCH, 0, desc.mTessPatchStartIndex, numPatches);//16);

   mCurrTechnique.endPass();    

   mpUsingDevice->SetIndices(NULL);
} 
//============================================================================
// BTerrainRender::renderSkirtNodeInstances
//============================================================================
void BTerrainRender::renderSkirtNodeInstances(const BTerrainQuadNodePtrArray* pVisibleNodeInstances)
{
   SCOPEDSAMPLE(BTerrainRender_renderSkirtNodeInstances);

   // Disable skirting if blackmap is enabled
//   if (mBlackmapParams.mpTexture)
  //    return;
      
   beginRenderInternal(cTRP_Full);
   
   // Render chunks who's textures are still in the cache.
   for (uint i = 0; i < pVisibleNodeInstances->size(); i++)
   {
      const BTerrainQuadNode* node = (*pVisibleNodeInstances)[i];
      BASSERT(node);
      if (!node)
         continue;

      BDEBUG_ASSERT(node->mSkirtInfo.mQuadrant >= 0);

      renderSkirt(node->mSkirtInfo);
   }
   
   endRender();
}
//============================================================================
// BTerrainRender::setRenderPhase
//============================================================================
void BTerrainRender::setRenderPhase(eTRenderPhase phase)
{
   //ASSERT_RENDER_THREAD
   
   if (phase == mPhase)
      return;
      
   mPhase = phase;
   
   BDEBUG_ASSERT(mTerrainGPUShader.getEffect() && (mTerrainGPUShader.getNumTechniques() >= (uint)mPhase));
   
   mCurrTechnique = mTerrainGPUShader.getTechniqueFromIndex((unsigned int)mPhase);
}

//============================================================================
// BTerrainRender::processCommand
//============================================================================
void BTerrainRender::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD
}
//============================================================================
// BTerrainRender::frameBegin
//============================================================================
void BTerrainRender::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}
//============================================================================
// BTerrainRender::frameEnd
//============================================================================
void BTerrainRender::frameEnd(void)
{
   ASSERT_RENDER_THREAD
   
   if (mDCBRenderingToggle)
   {
      mDCBRenderingToggle = false;
      mDCBRenderingEnabled = !mDCBRenderingEnabled;
   }
}
//============================================================================
// BTerrainRender::deinitDeviceData
//============================================================================
void BTerrainRender::deinitDeviceData()
{
   ASSERT_RENDER_THREAD
   
   if (mpEffectLoader)
   {
      ALIGNED_DELETE(mpEffectLoader, gRenderHeap);
      mpEffectLoader = NULL;
   }
      
   mTerrainGPUShader.clear();
      
   if (mVertexBuffer)
   {
      mVertexBuffer->Release();
      mVertexBuffer=NULL;
   }

   if (mpCommandBufferDevice)
   {
      mpCommandBufferDevice->Release();
      mpCommandBufferDevice = NULL;
   }
      
   mIntrinsicPool.clear();
      
   mPhase = cTRP_Invalid;
}

//============================================================================
// BTerrainRender::loadEffect
//============================================================================
void BTerrainRender::loadEffect(void)
{
   ASSERT_RENDER_THREAD
   
   if (!mpEffectLoader)
   {
      mpEffectLoader = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
      const bool status = mpEffectLoader->init(gRender.getEffectCompilerDefaultDirID(), EFFECT_FILENAME, true, false, true, &mIntrinsicPool);
      BVERIFY(status);
   }
}   
//============================================================================
// BTerrainRender::initDeviceData
//============================================================================
void BTerrainRender::initDeviceData()
{
   ASSERT_RENDER_THREAD
         
   loadEffect();

   mVertexMemSize = sizeof(D3DXVECTOR3) + sizeof(D3DXVECTOR4);
   
   //create a dummy VB that contains the indicies for this process.
   BD3D::mpDev->CreateVertexBuffer( 1, 0, 0 , 0, &mVertexBuffer, NULL );

   mPhase = cTRP_Invalid;

#ifndef BUILD_FINAL
   if (cInvalidManagedTextureHandle == gMipVisTextureHandle)
      gMipVisTextureHandle = gD3DTextureManager.getOrCreateHandle(MIPVIS_TEXTURE_PATH, BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainCommon, false, cDefaultTextureWhite, true, false, "BTerrainRender");
#endif 

   //For DCBs
   Direct3D_CreateDevice(0, D3DDEVTYPE_COMMAND_BUFFER, NULL, 0, NULL, &mpCommandBufferDevice);
   
   mpUsingDevice = BD3D::mpDev;
   
   mIntrinsicPool.create();
}

//============================================================================
// BTerrainRender::tickEffect
//============================================================================
void BTerrainRender::tickEffect(void)
{
   BDEBUG_ASSERT(mpEffectLoader);
   
   if (mpEffectLoader->tick(true))
   {
      mTerrainGPUShader.attach(mpEffectLoader->getFXLEffect().getEffect(), &mIntrinsicPool);

      initEffectConstants();
   }
      
   // We now never allow the terrain to render without a valid effect.
   BVERIFY(mTerrainGPUShader.getEffect());
}

//============================================================================
// BTerrainRender::initEffectConstants
//============================================================================
void BTerrainRender::initEffectConstants(void)
{
   ASSERT_RENDER_THREAD
   
   BDEBUG_ASSERT(!mRendering && mTerrainGPUShader.getEffect());
   
   // Any HLSL variables that are manually updated via SetPixelShaderConstantF(), etc. must be marked for manual register update here!
               
   //RENDERING
   mShaderDataValsHandle                   = mTerrainGPUShader("g_terrainVals");
   mShaderDataVals2Handle                  = mTerrainGPUShader("g_terrainVals2");
 
   mPosComprMin                            = mTerrainGPUShader("g_posCompMin");
   mPosComprRange                          = mTerrainGPUShader("g_posCompRange");
   
   mShaderPositionTextureHandle            = mTerrainGPUShader("vertSampler_pos");
   mShaderNormalTextureHandle              = mTerrainGPUShader("vertSampler_basis");
   mShaderAOTextureHandle                  = mTerrainGPUShader("vertSampler_ao");
   mShaderAlphaTextureHandle               = mTerrainGPUShader("vertSampler_alpha");
   mShaderLightingTextureHandle            = mTerrainGPUShader("vertSampler_light");

   mShaderDynamicAlphaTextureHandle        = mTerrainGPUShader("dynamicAlphaSampler");
   mShaderDynamicAlphaTextureWidth         = mTerrainGPUShader("g_dynamicAlphaTexWidth");

  //TEXTURING
   mShaderRcpMapSizeHandle                 = mTerrainGPUShader("g_rcpMapSize");

     
   mShaderUniqueAlbedoTexHandle				 = mTerrainGPUShader("UniqueAlbedoSampler");
   mShaderUniqueNormalTexHandle				 = mTerrainGPUShader("UniqueNormalSampler");
   mShaderUniqueSpecularTexHandle   		 = mTerrainGPUShader("UniqueSpecularSampler");
   mShaderUniqueEnvMaskTexHandle   		   = mTerrainGPUShader("UniqueEnvMaskSampler");
   mShaderUniqueSelfTexHandle   		      = mTerrainGPUShader("UniqueSelfSampler");
   mShaderUniqueSelfTexPacked             = mTerrainGPUShader("gPackedSelfTexture");

   mShaderEnvMapTexHandle                  = mTerrainGPUShader("EnvSampler");
   mShaderEnvMapHDRHandle                  = mTerrainGPUShader("gEnvMapHDR");
   
   mShaderSpecPowerHandle                 = mTerrainGPUShader("gSpecPower");
   mShaderBumpPowerHandle                 = mTerrainGPUShader("gBumpPower");

   mShaderSpecOnlyDir_IntHandle           = mTerrainGPUShader("gSpecOnlyDir_Dir_Inten");
   mShaderSpecOnlyCol_ShadHandle          = mTerrainGPUShader("gSpecOnlyDir_Col_ShadScl");

   mShaderAODiffuseIntensityHandle        = mTerrainGPUShader("gAODiffuseIntensity");

   //lighting & shadows
   mLocalLightingEnabled                   = mTerrainGPUShader("gLocalLightingEnabled");
   mLocalLightingEnabled.setRegisterUpdateMode(true);
   
   mLocalShadowingEnabled                  = mTerrainGPUShader("gLocalShadowingEnabled");
   mLocalShadowingEnabled.setRegisterUpdateMode(true);
   

   mLightData                              = mTerrainGPUShader("gLightData");
   mLightData.setRegisterUpdateMode(true);

   mExtendedLocalLightSampler              = mTerrainGPUShader("ExtendedLocalLightSampler");
   
   mBlackmapEnabled                        = mTerrainGPUShader("gBlackmapEnabled");
   mBlackmapSampler                        = mTerrainGPUShader("gBlackmapSampler");
   mBlackmapUnexploredSampler              = mTerrainGPUShader("gBlackmapUnexploredSampler");
   mBlackmapParams0                        = mTerrainGPUShader("gBlackmapParams0");
   mBlackmapParams1                        = mTerrainGPUShader("gBlackmapParams1");
   mBlackmapParams2                        = mTerrainGPUShader("gBlackmapParams2");
   mEnableLightBuffering                   = mTerrainGPUShader("gEnableLightBuffering");
   mWorldToLightBuf                        = mTerrainGPUShader("gWorldToLightBuf");
   mLightBufferColorSampler                = mTerrainGPUShader("LightBufferColorSampler");
   mLightBufferVectorSampler               = mTerrainGPUShader("LightBufferVectorSampler");
   
   //misc
   mShaderDistFromCameraScalar             = mTerrainGPUShader("gSelfMapFadeoutScalar");
   
   mShaderWVPMat                          = mTerrainGPUShader("gWorldToProj");
   mShaderQuadrantMatrixHandle            = mTerrainGPUShader("g_quadrantMatrix");
   mShadeNumBatchedXChunks                = mTerrainGPUShader("gNumBatchedXChunks");
   
   mShaderPatchOffsetHandle               = mTerrainGPUShader("gPatchOffsetNumbers");
 
   

   //LIGHTING
   

  
   
   mTerrainGPUShader("gVisControl0").setRegisterUpdateMode(true);
   mTerrainGPUShader("gVisControl1").setRegisterUpdateMode(true);
   mTerrainGPUShader("gVisControl2").setRegisterUpdateMode(true);
   mTerrainGPUShader("gVisControl3").setRegisterUpdateMode(true);

   mTerrainGPUShader("gNumLights").setRegisterUpdateMode(true);

   mTerrainGPUShader("gExtendedLocalLightingEnabled").setRegisterUpdateMode(true);
   mTerrainGPUShader("gNumExtendedLights").setRegisterUpdateMode(true);
   mTerrainGPUShader("gExtendedLocalLightingParams").setRegisterUpdateMode(true);
   
   mCurrTechnique = mTerrainGPUShader.getTechniqueFromIndex(0);
}

//============================================================================
// BTerrainRender::receiveEvent
//============================================================================
bool BTerrainRender::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
   
   return false;   
}

//============================================================================
// BTerrainRender::setDCBPredicationData
//============================================================================
void BTerrainRender::setDCBPredicationData(const BTerrainQuadNode* pNode)
{
   uint numTiles = gTiledAAManager.getNumTiles();

   DWORD pLogic = 0;
   DWORD bitMask = 1;
   for(uint i = 0; i < numTiles; i++, bitMask <<= 1)
   {
      if (!pNode->mVisibleInThisTile[i])
         continue;

      pLogic |= bitMask;
   }

   mpUsingDevice->SetCommandBufferPredication(0, pLogic);
}

//============================================================================
// BTerrainRender::beginDCBRender
//============================================================================
void BTerrainRender::beginDCBRender(const BTerrainQuadNodePtrArray* pVisibleNodeInstances, const BTerrainQuadNodePtrArray* pVisibleSkirtNodeInstances, bool deferToThreads)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(pVisibleNodeInstances && pVisibleSkirtNodeInstances);
      
   SCOPEDSAMPLE(BTerrainRender_beginDCBRender);
         
   tickEffect();
      
   mIntrinsicPool.getTable() = gEffectIntrinsicManager.getRenderIntrinsicTable();
      
   if (!mDCBRenderingEnabled)
   {
      // Set all of our intrinsics to the device.
      mIntrinsicPool.updateAll();
      
      renderNodeInstances(pVisibleNodeInstances);
      renderSkirtNodeInstances(pVisibleSkirtNodeInstances);
      return;
   }

   IDirect3DSurface9* pRenderTarget;
   BD3D::mpDev->GetRenderTarget(0, &pRenderTarget);
   memcpy(&mSavedRenderTargetSurf, pRenderTarget, sizeof(IDirect3DSurface9));
   pRenderTarget->Release();

   IDirect3DSurface9* pDepthStencilSurf;
   BD3D::mpDev->GetDepthStencilSurface(&pDepthStencilSurf);
   memcpy(&mSavedDepthStencilSurf, pDepthStencilSurf, sizeof(IDirect3DSurface9));
   pDepthStencilSurf->Release();
   
   BD3D::mpDev->GetViewport(&mSavedViewPort);
      
   mDCBRenderingActive = true;

   mpUsingDevice = mpCommandBufferDevice;

   mTerrainGPUShader.getEffect()->ChangeDevice(mpUsingDevice);
   
   mIntrinsicPool.setDevice(mpUsingDevice);
   
   // Copy the state of the render thread's intrinsic effect intrinsic pool to our private pool.
   if (deferToThreads)
   {
      mpCommandBufferDevice->ReleaseThreadOwnership();
            
      mCalcNodeDCBsCallbackData.mNodeList = *pVisibleNodeInstances;
      mCalcNodeDCBsCallbackData.mSkirtNodeList = *pVisibleSkirtNodeInstances;
      BD3D::mpDev->GetRenderState(D3DRS_FILLMODE, &mCalcNodeDCBsCallbackData.mFillMode);
      BD3D::mpDev->GetRenderState(D3DRS_COLORWRITEENABLE, &mCalcNodeDCBsCallbackData.mColorWriteEnable);
      BD3D::mpDev->GetRenderState(D3DRS_HALFPIXELOFFSET, &mCalcNodeDCBsCallbackData.mHalfPixelOffset);
      
      BDEBUG_ASSERT(!mpCommandBuffer);
      mpCommandBuffer = gDCBManager.acquire();
      BVERIFY(mpCommandBuffer);
                
      gWorkDistributor.flush();
      gWorkDistributor.queue(beginDCBRenderCallback, NULL, 0);
      gWorkDistributor.flush();

      mThreadedNodeDCBCreationIssued = TRUE;
   }
   else
   {
      mIntrinsicPool.updateAll();
      
      beginDCBRenderInternal(pVisibleNodeInstances, pVisibleSkirtNodeInstances);

      mTerrainGPUShader.getEffect()->ChangeDevice(BD3D::mpDev);
      
      mIntrinsicPool.setDevice(BD3D::mpDev);

      mpUsingDevice = BD3D::mpDev;
   }
}

//============================================================================
// BTerrainRender::calcSkirtNodeDCBsCallback
//============================================================================
void BTerrainRender::beginDCBRenderCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{ 
   gTerrainRender.mpCommandBufferDevice->AcquireThreadOwnership();
   
   gTerrainRender.beginDCBRenderInternal(&gTerrainRender.mCalcNodeDCBsCallbackData.mNodeList, &gTerrainRender.mCalcNodeDCBsCallbackData.mSkirtNodeList);
   
   gTerrainRender.mpCommandBufferDevice->ReleaseThreadOwnership();
   
   gTerrainRender.mNodeDCBCreationEvent.set();
}

//============================================================================
// BTerrainRender::calcSkirtNodeDCBsCallback
//============================================================================
void BTerrainRender::beginDCBRenderInternal(const BTerrainQuadNodePtrArray* pVisibleNodeInstances, const BTerrainQuadNodePtrArray* pVisibleSkirtNodeInstances)
{
   SCOPEDSAMPLE(BTerrainRender_beginDCBRenderInternal);

   BASSERT(mDCBRenderingEnabled);
      
   // MPB [12/5/2008] - Added the RECORD_ALL_STATE flag because there can be state set after
   // the final draw call and this needs to be recorded and played back to avoid leaving
   // things in a bad way.
   DWORD cbFlags = D3DBEGINCB_RECORD_ALL_SET_STATE;
   mpUsingDevice->BeginCommandBuffer(mpCommandBuffer, cbFlags, NULL, NULL, 0, 0);
   
   mpUsingDevice->FlushHiZStencil(D3DFHZS_ASYNCHRONOUS);

   mpUsingDevice->SetRenderTarget(0, &mSavedRenderTargetSurf);
   mpUsingDevice->SetDepthStencilSurface(&mSavedDepthStencilSurf);
   
   mpUsingDevice->SetViewport(&mSavedViewPort);

   mIntrinsicPool.updateAll();
   
   mpUsingDevice->SetRenderState(D3DRS_FILLMODE, mCalcNodeDCBsCallbackData.mFillMode);
   mpUsingDevice->SetRenderState(D3DRS_COLORWRITEENABLE, mCalcNodeDCBsCallbackData.mColorWriteEnable);
   mpUsingDevice->SetRenderState(D3DRS_HALFPIXELOFFSET, mCalcNodeDCBsCallbackData.mHalfPixelOffset);
   
   renderNodeInstances(pVisibleNodeInstances);
   renderSkirtNodeInstances(pVisibleSkirtNodeInstances);
   
   mpUsingDevice->SetVertexShader(NULL);
   mpUsingDevice->SetPixelShader(NULL);
   mpUsingDevice->SetIndices(NULL);

   for (uint i = 0; i < D3DMAXSTREAMS; i++)
      mpUsingDevice->SetStreamSource(i, NULL, 0, 0);

   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
      mpUsingDevice->SetTexture(i, NULL);

   mpUsingDevice->SetRenderTarget(0, NULL);
   mpUsingDevice->SetDepthStencilSurface(NULL);
   
   mpUsingDevice->FlushHiZStencil(D3DFHZS_SYNCHRONOUS);
   
   mpUsingDevice->EndCommandBuffer();
}
//============================================================================
// BTerrainRender::joinDCBWork
//============================================================================
void BTerrainRender::joinDCBWork()
{
   SCOPEDSAMPLE(BTerrainRender_endDCBRender)

   if (!mDCBRenderingActive)
      return;

   if (mDCBRenderingEnabled)
   {
      if (mThreadedNodeDCBCreationIssued)
      {
         BD3D::mpDev->InsertFence();
         
         gWorkDistributor.waitSingle(mNodeDCBCreationEvent);

         mpCommandBufferDevice->AcquireThreadOwnership();

         mTerrainGPUShader.getEffect()->ChangeDevice(BD3D::mpDev);

         mIntrinsicPool.setDevice(BD3D::mpDev);

         mpUsingDevice = BD3D::mpDev;

         mThreadedNodeDCBCreationIssued = FALSE;
      }
   }
}
//============================================================================
// BTerrainRender::flushDCBs
//============================================================================
void BTerrainRender::flushDCBs(uint tileIndex)
{
   // This assumes we're rendering the visual pass.
   BD3D::mpDev->SetShaderGPRAllocation(0, 26, 102);
   
   gDCBManager.run(mpCommandBuffer, mPredicatedRendering ? (1 << tileIndex) : 0);
   
   BD3D::mpDev->SetShaderGPRAllocation(0, 64, 64);
   
   BD3D::mpDev->InsertFence();
}
//============================================================================
// BTerrainRender::endDCBRender
//============================================================================
void BTerrainRender::endDCBRender()
{
   if (!mDCBRenderingActive)
      return;
      
   SCOPEDSAMPLE(BTerrainRender_endDCBRender)
      
   if (mDCBRenderingEnabled)
   {
      joinDCBWork();
      flushDCBs(0);
                  
      gDCBManager.release(mpCommandBuffer);
      mpCommandBuffer = NULL;
   }
         
   mDCBRenderingActive = false;
}