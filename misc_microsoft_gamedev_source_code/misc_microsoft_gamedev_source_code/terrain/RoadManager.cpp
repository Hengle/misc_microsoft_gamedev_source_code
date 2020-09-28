//============================================================================
//
//  RoadManager.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
// terrain 
#include "TerrainPCH.h"
#include "terrain.h"
#include "RoadManager.h"

// xcore
#include "reloadManager.h"
#include "consoleOutput.h"

//xrender
#include "renderThread.h"
#include "D3DTextureManager.h"
#include "renderDraw.h"
#include "renderThread.h"
#include "asyncFileManager.h"
#include "gpuHeap.h"
#include "effectFileLoader.h"
#include "D3DTextureLoader.h"

// xgameRender
#include "render.h"

// xgame
#include "vertexTypes.h"

BTerrainRoadManager  gRoadManager;

// shaders
#include "defConstRegs.inc"
#include "..\shaders\terrain\gpuTerrainshaderRegs.inc"
#include "..\shaders\shared\localLightingRegs.inc"
#if TERRAIN_SHADER_REGS_VER != 101
#error Please update gpuTerrainShaderRegs.inc
#endif

// Max # of chunk lights = Max # of local light shader constants / 8
const uint cMaxChunkLocalLights = (NUM_LOCAL_LIGHT_PSHADER_CONSTANTS >> 3);

const uint cLightAttribTexWidth     = 128;
const uint cLightAttribTexWidthLog2 = 7;
const uint cLightAttribTexHeight    = 128;


#define ROAD_EFFECT_FILENAME "terrain\\terrainRoads.bin"

//============================================================================
// BTerrainRoadManager::BTerrainRoadManager
//============================================================================
BTerrainRoadManager::BTerrainRoadManager():
   mEventHandle(cInvalidEventReceiverHandle),
   mRoads(0),
   mpEffectLoader(NULL),
   mpLightAttribTexture(NULL),
   mNextLightAttribTexelIndex(0)
{
}
//============================================================================
// BTerrainRoadManager::~BTerrainRoadManager
//============================================================================
BTerrainRoadManager::~BTerrainRoadManager()
{
   
}
//============================================================================
// BTerrainVisual::init
//============================================================================
bool           BTerrainRoadManager::init()
{
   ASSERT_MAIN_THREAD

      commandListenerInit();

   return true;
}
//============================================================================
// BTerrainVisual::deinit
//============================================================================
bool  BTerrainRoadManager::deinit()
{
   // Block for safety. 
   gRenderThread.blockUntilGPUIdle();

   commandListenerDeinit();

   gReloadManager.deregisterClient(mEventHandle);

   if (mEventHandle != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientDeferred(mEventHandle, true);
      mEventHandle = cInvalidEventReceiverHandle;
   }

   return true;
}
//============================================================================
// BTerrainVisual::destroy
//============================================================================
void  BTerrainRoadManager::destroy()
{
   gRenderThread.submitCommand(mCommandHandle,cTRM_Destroy);
}
//============================================================================
// BTerrainVisual::destroyInternal
//============================================================================
bool  BTerrainRoadManager::destroyInternal()
{
   for(uint i=0;i<mRoads.size();i++)
   {
      if(mRoads[i])
      {
         delete mRoads[i];
         mRoads[i]=NULL;
      }
   }
   mRoads.clear();
   
   return true;
}
//============================================================================
// BTerrainRoadManager::processCommand
//============================================================================
void BTerrainRoadManager::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      case cTRM_Destroy:
         {
            destroyInternal();
            break;
         }
   }
};  
//============================================================================
// BTerrainRoadManager::frameBegin
//============================================================================
void BTerrainRoadManager::frameBegin(void)
{
   ASSERT_RENDER_THREAD
};
//============================================================================
//  BTerrainRoadManager::frameEnd
//============================================================================
void BTerrainRoadManager::frameEnd(void)
{
   ASSERT_RENDER_THREAD
};
//============================================================================
// BTerrainRoadManager::loadEffect
//============================================================================
void BTerrainRoadManager::loadEffect(void)
{
     ASSERT_RENDER_THREAD
   if (!mpEffectLoader)
   {
      mpEffectLoader = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
      const bool status = mpEffectLoader->init(gRender.getEffectCompilerDefaultDirID(), ROAD_EFFECT_FILENAME, true, false, true);
      BVERIFY(status);
   }
}   
//============================================================================
// BTerrainRoadManager::tickEffect
//============================================================================
void BTerrainRoadManager::tickEffect(void)
{
   BDEBUG_ASSERT(mpEffectLoader);

   if (mpEffectLoader->tick(true))
   {
      mTerrainRoadShader.attach(mpEffectLoader->getFXLEffect().getEffect());

      initEffectConstants();
   }

   // We now never allow the terrain to render without a valid effect.
   BVERIFY(mTerrainRoadShader.getEffect());

   mTerrainRoadShader.updateIntrinsicParams();
}
//============================================================================
// BTerrainVisual::initEffectConstants
//============================================================================
void BTerrainRoadManager::initEffectConstants(void)
{  
   ASSERT_RENDER_THREAD

   BDEBUG_ASSERT(mTerrainRoadShader.getEffect());

   // Any HLSL variables that are manually updated via SetPixelShaderConstantF(), etc. must be marked for manual register update here!

   //RENDERING
   mShaderDataValsHandle                   = mTerrainRoadShader("g_terrainVals");

   mPosComprMin                            = mTerrainRoadShader("g_posCompMin");
   mPosComprRange                          = mTerrainRoadShader("g_posCompRange");

   mShaderPositionTextureHandle            = mTerrainRoadShader("vertSampler_pos");
   mShaderNormalTextureHandle              = mTerrainRoadShader("vertSampler_basis");

   mShaderRoadTextureHandle[cTextureTypeAlbedo] =  mTerrainRoadShader("gRoadAlbedo");
   mShaderRoadTextureHandle[cTextureTypeNormal] =  mTerrainRoadShader("gRoadNormal");
   mShaderRoadTextureHandle[cTextureTypeSpecular] =  mTerrainRoadShader("gRoadSpecular");

   //Lighting
   mLocalLightingEnabled                   = mTerrainRoadShader("gLocalLightingEnabled");
   mLocalLightingEnabled.setRegisterUpdateMode(true);

   mLocalShadowingEnabled                  = mTerrainRoadShader("gLocalShadowingEnabled");
   mLocalShadowingEnabled.setRegisterUpdateMode(true);

   mLightData                              = mTerrainRoadShader("gLightData");
   mLightData.setRegisterUpdateMode(true);

   mTerrainRoadShader("gVisControl0").setRegisterUpdateMode(true);
   mTerrainRoadShader("gVisControl1").setRegisterUpdateMode(true);
   mTerrainRoadShader("gVisControl2").setRegisterUpdateMode(true);
   mTerrainRoadShader("gVisControl3").setRegisterUpdateMode(true);

   mTerrainRoadShader("gNumLights").setRegisterUpdateMode(true);

   mTerrainRoadShader("gExtendedLocalLightingEnabled").setRegisterUpdateMode(true);
   mTerrainRoadShader("gNumExtendedLights").setRegisterUpdateMode(true);
   mTerrainRoadShader("gExtendedLocalLightingParams").setRegisterUpdateMode(true);

   mCurrTechnique = mTerrainRoadShader.getTechniqueFromIndex(0);
}
//============================================================================
// BTerrainVisual::receiveEvent
//============================================================================
bool BTerrainRoadManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD

      return false;   
}
//============================================================================
// BTerrainRoadManager::initDeviceData
//============================================================================
void BTerrainRoadManager::initDeviceData(void)
{
   ASSERT_RENDER_THREAD

   loadEffect();
}
//============================================================================
// BTerrainRoadManager::deinitDeviceData
//============================================================================
void BTerrainRoadManager::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
      if (mpEffectLoader)
      {
         ALIGNED_DELETE(mpEffectLoader, gRenderHeap);
         mpEffectLoader = NULL;
      }
       mTerrainRoadShader.clear();
}
//============================================================================
// BTerrainRoadManager::render
//============================================================================
void BTerrainRoadManager::render(const BTerrainQuadNode *terrainQuadGrid, int tileIndex)
{
   SCOPEDSAMPLE(BTerrainRenderRoads);
   ASSERT_RENDER_THREAD
   tickEffect();

   if(!mRoads.size())
      return;

   //Set our terrain shader data
   mPosComprRange                         = gTerrainVisual.getPosRange();
   mPosComprMin                           = gTerrainVisual.getPosMin();
   mShaderPositionTextureHandle           = gTerrainVisual.getPosTex();
   mShaderNormalTextureHandle             = gTerrainVisual.getNormTex();
   mShaderDataValsHandle                  = D3DXVECTOR4(  1.0f / (float)gTerrainVisual.getNumXVerts() , (float)gTerrainVisual.getTileScale(), 0, 0);

   BD3D::mpDev->SetVertexDeclaration(BPTVertex::msVertexDecl);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);   //CLM speed this up later by ensuring CW only winding out of the editor splitter


   BD3D::mpDev->GpuOwnPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

   mCurrTechnique.begin(0);
   
   for(uint i=0;i<mRoads.size();i++)
   {
      int passIndex = setupTexturing(i);

      mCurrTechnique.beginPass(passIndex);
      mCurrTechnique.commitU();

      
     
       
      //render our intersections
      for(uint q=0;q<mRoads[i]->mRoadChunks.size();q++)
      {
         if(terrainQuadGrid[mRoads[i]->mRoadChunks[q]->mQNParentIndex].mVisibleInThisTile[tileIndex]) //are we visible?
         {
            setupLighting(terrainQuadGrid[mRoads[i]->mRoadChunks[q]->mQNParentIndex].getDesc());   //CLM speed this up later by merging this into terrainRender, and using the chunks' already
                                                                                                         //         calculated lighting data.
            

            BD3D::mpDev->SetStreamSource(0,mRoads[i]->mRoadChunks[q]->mSegmentVB,0,sizeof(BPTVertex));
            BD3D::mpDev->DrawPrimitive(D3DPT_TRIANGLELIST , 0, mRoads[i]->mRoadChunks[q]->mNumPrims);
            
         }
      }
      mCurrTechnique.endPass();  
   }
   

 
    

   mCurrTechnique.end();
   BD3D::mpDev->SetIndices(0);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
   BD3D::mpDev->GpuDisownPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

}

//============================================================================
// BTerrainRoad::setupLighting
//============================================================================
inline void BTerrainRoadManager::setupLighting(const BTerrainQuadNodeDesc& desc)
{
   //BDEBUG_ASSERT(mPhase != cTRP_ShadowGen);

   BOOL boolValueFALSE = FALSE;
   BOOL boolValueTRUE = TRUE;

   const XMVECTOR boundsMin = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&desc.m_min));
   const XMVECTOR boundsMax = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&desc.m_max));

   BSceneLightManager::BGridRect gridRect = gRenderSceneLightManager.getGridRect(boundsMin, boundsMax);

   mCurVisibleLightIndices.resize(0);
   gVisibleLightManager.findLights(mCurVisibleLightIndices, gridRect, boundsMin, boundsMax, true, true);

   if (mCurVisibleLightIndices.isEmpty())
   {
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, &boolValueFALSE, 1);
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);

      return;  
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

   BD3D::mpDev->SetPixelShaderConstantB(ENABLE_LOCAL_LIGHTS_REG, numNormalLights ? &boolValueTRUE : &boolValueFALSE, 1);

   const XMFLOAT4* pTexels = gVisibleLightManager.getVisibleLightTexels();

   for (uint i = 0; i < numNormalLights; i++)
   {
      BD3D::mpDev->GpuLoadPixelShaderConstantF4Pointer(i * 8, pTexels + 8 * mCurVisibleLightIndices[i], 8);
   }

   const int numLightsInt4[4] = { numNormalLights, 0, 8, 0 };
   BD3D::mpDev->SetPixelShaderConstantI(NUM_LOCAL_LIGHTS_REG, numLightsInt4, 1);

   BOOL boolValue = numShadowedLights > 0;
   BD3D::mpDev->SetPixelShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, &boolValue, 1);

   if (!numExtendedLights)      
   {
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
   }
   else
   {
      uint actualNumExtendedLights;
      uint texelIndex = fillLightAttribTexture(mCurVisibleLightIndices, numNormalLights, actualNumExtendedLights);

      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, actualNumExtendedLights ? &boolValueTRUE : &boolValueFALSE, 1);

      if (actualNumExtendedLights)
      {
         const int numExtendedLightsInt4[4] = { actualNumExtendedLights, 0, 1, 0 };
         BD3D::mpDev->SetPixelShaderConstantI(NUM_EXTENDED_LOCAL_LIGHTS_REG, numExtendedLightsInt4, 1);

         BVec4 extendedParams((float)texelIndex, 0.0f, 0.0f, 0.0f);
         BD3D::mpDev->SetPixelShaderConstantF(EXTENDED_LOCAL_LIGHTING_PARAMS_REG, extendedParams.getPtr(), 1);
      }      
   }      
}
//============================================================================
// BTerrainRoadManager::allocateTempResources
//============================================================================
void BTerrainRoadManager::allocateTempResources(void)
{
   BDEBUG_ASSERT(!mpLightAttribTexture);
   mpLightAttribTexture = gRenderDraw.createDynamicTexture(cLightAttribTexWidth, cLightAttribTexHeight, D3DFMT_LIN_A32B32G32R32F);
   BVERIFY(mpLightAttribTexture);

   mNextLightAttribTexelIndex = 0;
}

//============================================================================
// BTerrainRoadManager::releaseTempResources
//============================================================================
void BTerrainRoadManager::releaseTempResources(void)
{
   BDEBUG_ASSERT(mpLightAttribTexture);
   mpLightAttribTexture = NULL;
}

//============================================================================
// BTerrainRoadManager::computeLightAttribTexelPtr
//============================================================================
XMFLOAT4* BTerrainRoadManager::computeLightAttribTexelPtr(XMFLOAT4* pDstTexture, uint texelIndex)
{
   const uint x = texelIndex & (cLightAttribTexWidth - 1);
   const uint y = texelIndex >> cLightAttribTexWidthLog2;
   const uint dstBlockOfs = x + (y << cLightAttribTexWidthLog2);
   XMFLOAT4* pDst = pDstTexture + dstBlockOfs;
   return pDst;
}   

//============================================================================
// BTerrainRoadManager::fillLightAttribTexture
//============================================================================
uint BTerrainRoadManager::fillLightAttribTexture(const BSceneLightManager::BActiveLightIndexArray& lights, uint firstIndex, uint& outNumLights)
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
// BTerrainRoadManager::fillLightAttribTexture
//============================================================================
void BTerrainRoadManager::setVisControlRegs()
{
#ifndef BUILD_FINAL         
   switch (BTerrain::getVisMode())
   {
   case cVMDisabled:
      {
         break;
      }
   default:
      {
         float control[16];
         Utils::ClearObj(control);

         BDEBUG_ASSERT((BTerrain::getVisMode() - cVMAlbedo) < cVMNum);
         control[BTerrain::getVisMode() - cVMAlbedo] = 1.0f;

         BD3D::mpDev->SetPixelShaderConstantF(VIS_CONTROL_0_REG, control, 4);                     

         break;
      }
   }
#endif  
}
//============================================================================
// BTerrainRoadManager::setupTexturing
//============================================================================
int BTerrainRoadManager::setupTexturing(int roadIndex)
{
    int passNum =cRenderPassANS; 
   //set our textures for this road
   mShaderRoadTextureHandle[cTextureTypeAlbedo] = mRoads[roadIndex]->mTexture[cTextureTypeAlbedo].getBaseTexture();

   if(mRoads[roadIndex]->mTexture[cTextureTypeNormal].getBaseTexture())
      mShaderRoadTextureHandle[cTextureTypeNormal] = mRoads[roadIndex]->mTexture[cTextureTypeNormal].getBaseTexture();
   else
      mShaderRoadTextureHandle[cTextureTypeNormal] = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureNormal);


   if(mRoads[roadIndex]->mTexture[cTextureTypeSpecular].getBaseTexture())
      mShaderRoadTextureHandle[cTextureTypeSpecular] = mRoads[roadIndex]->mTexture[cTextureTypeSpecular].getBaseTexture();
   else
      mShaderRoadTextureHandle[cTextureTypeSpecular] = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);


 /*  if(mRoads[roadIndex]->mTexture[cTextureTypeSelf].getBaseTexture()
#ifndef BUILD_FINAL
      || BTerrain::getVisMode() ==cVMSelf
#endif
      )
   {
      mShaderRoadTextureHandle[cTextureTypeSelf] = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);


      passNum=cRenderPassANSR;

      
      if(mRoads[roadIndex]->mTexture[cTextureTypeEnvMask].getBaseTexture())
         passNum=cRenderPassFull;
   }*/


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
         setVisControlRegs(); 
         //GOOD STUFF IS DONE IN   setVisControlRegs()
         break;
      }
   }
#endif  

   return passNum;
}

//============================================================================
// BTerrainTexturing::setEnvMapTexture
//============================================================================
void BTerrainRoad::setRoadTexture(int dirID, const char *ifilename)
{
   ASSERT_THREAD(cThreadIndexRender);


   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();
   BFixedString256 filename;
   filename = BFixedString256(cVarArg, "%s%s.ddx", ifilename, "_df");
   pPacket->setFilename(filename);
   pPacket->setDirID(dirID);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainRoad::loadRoadTextureCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(cTextureTypeAlbedo);
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);

   //NM
   pPacket = gAsyncFileManager.newRequestPacket();
   filename = BFixedString256(cVarArg, "%s%s.ddx", ifilename, "_nm");
   pPacket->setFilename(filename);
   pPacket->setDirID(dirID);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainRoad::loadRoadTextureCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(cTextureTypeNormal);
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);

   //Sp
   pPacket = gAsyncFileManager.newRequestPacket();
   filename = BFixedString256(cVarArg, "%s%s.ddx", ifilename, "_sp");
   pPacket->setFilename(filename);
   pPacket->setDirID(dirID);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainRoad::loadRoadTextureCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(cTextureTypeSpecular);
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);

   //RM
   pPacket = gAsyncFileManager.newRequestPacket();
   filename = BFixedString256(cVarArg, "%s%s.ddx", ifilename, "_rm");
   pPacket->setFilename(filename);
   pPacket->setDirID(dirID);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainRoad::loadRoadTextureCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(cTextureTypeEnvMask);
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);

   //EM
   pPacket = gAsyncFileManager.newRequestPacket();
   filename = BFixedString256(cVarArg, "%s%s.ddx",ifilename, "_em");
   pPacket->setFilename(filename);
   pPacket->setDirID(dirID);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainRoad::loadRoadTextureCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(cTextureTypeSelf);
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);



}
//============================================================================
// BTerrainRoad::loadRoadTextureCallback
//============================================================================
void BTerrainRoad::loadRoadTextureCallback(void* pData)
{
   BAsyncFileManager::BRequestPacket* pPacket = static_cast<BAsyncFileManager::BRequestPacket*>(pData);

   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.resource("BTerrainRoad::loadRoadTextureCallback: File Not Found %s", pPacket->getFilename().c_str());

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   BD3DTextureLoader* mpTextureLoader = new BD3DTextureLoader();

   BD3DTextureLoader::BCreateParams textureLoadParams;
   textureLoadParams.mBigEndian = true;
   textureLoadParams.mTiled = true;
   textureLoadParams.mManager = "TerrainRoads";
   textureLoadParams.mName = pPacket->getFilename();

   bool status;
   status = mpTextureLoader->createFromDDXFileInMemory(static_cast<const uchar*>(pPacket->getData()), pPacket->getDataLen(), textureLoadParams);

   if (!status)
   {
      gConsoleOutput.resource("BTerrainRoad::loadRoadTextureCallback: Error Reading %s", pPacket->getFilename().c_str());

      delete mpTextureLoader;
      mpTextureLoader = NULL;

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   const uint textureIndex = pPacket->getPrivateData0();

   if(mTexture[textureIndex].getBaseTexture())
      mTexture[textureIndex].release();
   mTexture[textureIndex] = mpTextureLoader->getD3DTexture();
   
   mpTextureLoader->releaseOwnership();

   delete mpTextureLoader;
   mpTextureLoader = NULL;


   gAsyncFileManager.deleteRequestPacket(pPacket);

}
