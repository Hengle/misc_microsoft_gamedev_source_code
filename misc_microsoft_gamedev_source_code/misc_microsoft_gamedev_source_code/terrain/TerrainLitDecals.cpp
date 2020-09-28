//============================================================================
//
//  File: LitDecalManager.cpp
//
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================
#include "terrainpch.h"
#include "xgameRender.h"
#include "terrainlitdecals.h"
#include "terrain.h"

//============================================================================
// Globals
//============================================================================
BLitDecalManager gLitDecalManager;

static const float cDefaultZBias = -.00001f;//-0.00175000005f;


// shaders
#include "defConstRegs.inc"
#include "..\shaders\terrain\gpuTerrainshaderRegs.inc"
#include "..\shaders\shared\localLightingRegs.inc"
#if TERRAIN_SHADER_REGS_VER != 101
#error Please update gpuTerrainShaderRegs.inc
#endif

//============================================================================
// BLitDecalManager::BLitDecalManager
//============================================================================
BLitDecalManager::BLitDecalManager() :
mRenderBegunDrawing(false),
mNonConformZBias(cDefaultZBias),
mConformZBias(cDefaultZBias)
{
   ASSERT_THREAD(cThreadIndexSim);
}

//============================================================================
// BLitDecalManager::~BLitDecalManager
//============================================================================
BLitDecalManager::~BLitDecalManager()
{
   ASSERT_THREAD(cThreadIndexSim);
}

//============================================================================
// BLitDecalManager::init
//============================================================================
void BLitDecalManager::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);

   commandListenerInit();

   mNonConformZBias = cDefaultZBias;
   mConformZBias = cDefaultZBias;
}

//============================================================================
// BLitDecalManager::deinit
//============================================================================
void BLitDecalManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);  

   commandListenerDeinit();
}

//============================================================================
// BLitDecalManager::destroy
//============================================================================
void BLitDecalManager::destroy(void)
{
   gRenderThread.submitCommand(mCommandHandle,cRCDestroy);
}

//============================================================================
// BLitDecalManager::createLitDecal
//============================================================================
void BLitDecalManager::createLitDecal(const BLitDecalAttribs &data)
{
   ASSERT_THREAD(cThreadIndexSim);

   BLitDecalAttribs *packet = reinterpret_cast<BLitDecalAttribs *>(gRenderThread.submitCommandBegin(mCommandHandle, cRCCreateDecal, sizeof(BLitDecalAttribs)));
   memcpy(packet,  &data,sizeof(BLitDecalAttribs));
   gRenderThread.submitCommandEnd(sizeof(BLitDecalAttribs));

}

//============================================================================
// BLitDecalManager::destroyLitDecal
//============================================================================
void BLitDecalManager::destroyLitDecal(BLitDecalHandle handle)
{
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(handle != cInvalidLitDecalHandle);

  

   //remove from hash map
//-- FIXING PREFIX BUG ID 6904
   const BLitDecalAttribs* atrribs = getLitDecal(handle);
//--
   if(atrribs == NULL)
      return;

   const BVec3 pos(atrribs->getPos());

   BLitDecalKey key;  
   initKey(pos, key);

   BLitDecalGridHashMap::iterator it(mHashGrid.find(key));

   if (it == mHashGrid.end())
   {  
      BASSERT(!"The impact effect moved since it was created");
      return;
   }

   BLitDecalHashBucket* pValue = (BLitDecalHashBucket*) &it->second;

   
   int objIndex = pValue->mValues.find(handle);
   if (objIndex != cInvalidIndex)
      pValue->mValues.eraseUnordered(objIndex);

   //free the bucket if there's nothing there..
   if (pValue->mValues.isEmpty())
   {
         mHashGrid.erase(it);         
   }


    mRenderLitDecalAttribs.freeIndex(handle);

}

//============================================================================
// BLitDecalManager::getLitDecal
//============================================================================
BLitDecalAttribs* BLitDecalManager::getLitDecal(BLitDecalHandle handle)
{
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(handle != cInvalidLitDecalHandle);

   if(!mRenderLitDecalAttribs.isItemAllocated(handle))
      return NULL;

   return static_cast<BLitDecalAttribs*>(mRenderLitDecalAttribs.getItem(handle));
}
//============================================================================
// BLitDecalManager::destroyAllLitDecals
//============================================================================
void BLitDecalManager::destroyAllLitDecals()
{
   ASSERT_THREAD(cThreadIndexRender);
   mRenderLitDecalAttribs.freeAll();

   BLitDecalGridHashMap::const_iterator it = mHashGrid.begin();
   while(it != mHashGrid.end())
   {
      BLitDecalHashBucket* pValue = ((BLitDecalHashBucket*)&it->second);
      pValue->mValues.clear();
      it++;
   }
   mHashGrid.clear();

  
}


//============================================================================
// BLitDecalManager::updateRenderThread
//============================================================================
void BLitDecalManager::updateRenderThread(double gameTime)
{
   ASSERT_THREAD(cThreadIndexSim);

   SCOPEDSAMPLE(LitDecalManagerUpdateRenderThread);
   float gt = (float)gameTime;
   gRenderThread.submitCommand(mCommandHandle,cRCUpdate,sizeof(float),&gt);
}


//============================================================================
// BLitDecalManager::renderTickLitDecal
//============================================================================
bool BLitDecalManager::renderTickLitDecal(BLitDecalHandle handle, BLitDecalAttribs& attribs)
{  
 
   if(attribs.mHoldingUntilFade)
   {
      if(mRenderGameTime - attribs.getFadeStartTime() > attribs.getHoldUntilFadeTotalTime())
      {
         attribs.mHoldingUntilFade = false;
         attribs.setFadeStartTime(mRenderGameTime);
         attribs.setAlpha(255);

      }
      return true;
   }


  
   float t = Math::Clamp<float>(float((mRenderGameTime - attribs.getFadeStartTime()) / attribs.getFadeTotalTime()), 0.0f, 1.0f);
   

   if (attribs.getFadeDirection() < 0)
      t = 1.0f - t;

   uint alpha = Math::FloatToIntRound(t * 255.0f);

   if ( ((attribs.getFadeDirection() < 0) && (alpha == 0)) ||
      ((attribs.getFadeDirection() > 0) && (alpha == 255)) )
      return false;
      

 //  uint origAlpha = attribs.getAlpha();
   //if (origAlpha != 255)
   //   alpha = (alpha * origAlpha + 128) / 255;

   //alpha = 
   attribs.setAlpha(alpha);

   return true;
}



//============================================================================
// BLitDecalManager::BLitDecalAttribAlphaKeySorter::operator()
//============================================================================
bool BLitDecalManager::BLitDecalAttribAlphaKeySorter::operator() (uint i, uint j) const
{ 
   const BLitDecalAttribs& lhs = *static_cast<BLitDecalAttribs*>(mLitDecalManager.mRenderLitDecalAttribs.getItem(i));
   const BLitDecalAttribs& rhs = *static_cast<BLitDecalAttribs*>(mLitDecalManager.mRenderLitDecalAttribs.getItem(j));

   const BVec4& camPos = gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4();

   const BVec3& lPos = lhs.getPos();
   const BVec3& rPos = rhs.getPos();

   float lDist2 = Math::Sqr(lPos[0] - camPos[0]) + Math::Sqr(lPos[1] - camPos[1]) + Math::Sqr(lPos[2] - camPos[2]);
   float rDist2 = Math::Sqr(rPos[0] - camPos[0]) + Math::Sqr(rPos[1] - camPos[1]) + Math::Sqr(rPos[2] - camPos[2]);

   return lDist2 > rDist2;
}

//============================================================================
// BLitDecalManager::renderUpdate
//============================================================================
void BLitDecalManager::renderUpdate(float gameTime)
{
   ASSERT_THREAD(cThreadIndexRender);

   SCOPEDSAMPLE(LitDecalManagerRenderUpdate);

   mRenderGameTime = gameTime;

   mRenderValidAlphaLitDecals.resize(0);

   for (uint i = 0; i < mRenderLitDecalAttribs.getHighwaterMark(); i++)
   {
      if (!mRenderLitDecalAttribs.isItemAllocated(i))
         continue;

      BLitDecalAttribs& LitDecalAttribs = *static_cast<BLitDecalAttribs*>(mRenderLitDecalAttribs.getItem(i));

      if (!LitDecalAttribs.getEnabled())
         continue;


      if(!renderTickLitDecal(static_cast<BLitDecalHandle>(i), LitDecalAttribs))
      {
         destroyLitDecal(i);
       //  i--;
      }
      else
      {
         if (LitDecalAttribs.getBlendMode() == BLitDecalAttribs::cBlendOver)
            mRenderValidAlphaLitDecals.pushBack(static_cast<ushort>(i));
      }
   }

   
   if (mRenderValidAlphaLitDecals.size())
      std::sort(mRenderValidAlphaLitDecals.begin(), mRenderValidAlphaLitDecals.end(), BLitDecalAttribAlphaKeySorter(*this));
}

//============================================================================
// BLitDecalManager::initDeviceData
//============================================================================
void BLitDecalManager::initDeviceData(void)
{
}

//============================================================================
// BLitDecalManager::frameBegin
//============================================================================
void BLitDecalManager::frameBegin(void)
{
}

//============================================================================
// BLitDecalManager::processCommand
//============================================================================
void BLitDecalManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
   case cRCDestroy:
      {
         destroyAllLitDecals();
         break;
      }
   case cRCUpdate:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(float));
         renderUpdate(*reinterpret_cast<const float*>(pData));
         break;
      }
   case cRCCreateDecal:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BLitDecalAttribs));
         renderCreateDecal(*reinterpret_cast<const BLitDecalAttribs*>(pData));
         break;
      }
   }
}

//============================================================================
// BLitDecalManager::frameEnd
//============================================================================
void BLitDecalManager::frameEnd(void)
{
}

//============================================================================
// BLitDecalManager::deinitDeviceData
//============================================================================
void BLitDecalManager::deinitDeviceData(void)
{
}

//============================================================================
// BLitDecalManager::setupLighting
//============================================================================
inline int BLitDecalManager::setupLocalLighting(BVec3 min, BVec3 max)
{
   BOOL boolValueFALSE = FALSE;
   BOOL boolValueTRUE = TRUE;

   const XMVECTOR boundsMin = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&min));
   const XMVECTOR boundsMax = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&max));

   BSceneLightManager::BGridRect gridRect = gRenderSceneLightManager.getGridRect(boundsMin, boundsMax);

   mCurVisibleLightIndices.resize(0);
   gVisibleLightManager.findLights(mCurVisibleLightIndices, gridRect, boundsMin, boundsMax, true, true);

   if (mCurVisibleLightIndices.isEmpty())
   {
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, &boolValueFALSE, 1);
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);

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

   const uint cMaxChunkLocalLights = 4;
   const uint totalLights = mCurVisibleLightIndices.getSize();
   const uint numNormalLights = Math::Min<uint>(totalLights, cMaxChunkLocalLights);
  // const uint cMaxExtendedLights = 250;
 //  const uint numExtendedLights = Math::Min<uint>(totalLights - numNormalLights, cMaxExtendedLights);

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

   //if (!numExtendedLights)      
   {
      BD3D::mpDev->SetPixelShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
   }
 /*  else
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
   }    */
   return totalLights;
}

//============================================================================
// BLitDecalManager::renderCreateDecal
//============================================================================
void BLitDecalManager::renderCreateDecal(const BLitDecalAttribs& data)
{
   //check to see if we're already maxed out in this 'voxel' in space
   BLitDecalKey key;
   initKey(data.getPos(), key);


   BLitDecalGridHashMap::const_iterator it = mHashGrid.find(key);
   BLitDecalHashBucket* pValue = NULL;
   if (it != mHashGrid.end())
   {
      pValue = (BLitDecalHashBucket*) &it->second;

      if(pValue->mValues.getNumber() >= eMaxNumPerVoxel)
         return;
   }


   //-- if we got this far then we can create an impact.

   int index = mRenderLitDecalAttribs.allocIndex(false);
   if (index < 0)
      return;

   BLitDecalAttribs* pLitDecal = getLitDecal(index);

   if(pLitDecal == NULL)
      return;

   memcpy(pLitDecal,&data,sizeof(BLitDecalAttribs));

   //add this to our hashmap
   if(it!= mHashGrid.end())
   {
      //-- its in the grid and we have one of the same type already then just add the object to that list.
      BDEBUG_ASSERT(pValue != NULL);
      pValue->mValues.add(index);
      
   }
   else
   {
      //we have to add this voxel bucket
      BLitDecalHashBucket newValue;
      newValue.mValues.add(index);
      mHashGrid.insert(key, newValue);      
   }
   

}

//============================================================================
// BLitDecalManager::renderDrawLitDecalsBegin
//============================================================================
void BLitDecalManager::renderDrawLitDecalsBegin(void)
{
}

//============================================================================
// BLitDecalManager::renderDrawLitDecalsEnd
//============================================================================
void BLitDecalManager::renderDrawLitDecalsEnd(void)
{
}

//============================================================================
// BLitDecalManager::renderDrawLitDecalsBeginTile
//============================================================================
void BLitDecalManager::renderDrawLitDecalsBeginTile(uint tileIndex)
{
   BDEBUG_ASSERT(!mRenderBegunDrawing);
   mRenderBegunDrawing = true;
   tileIndex;

   //		depthBias	-0.00175000005	float
   //   static float depthBias = -0.000150000007;
   //   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));

   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 1);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKOFFSETS, D3DALPHATOMASK_DITHERED);

   gRenderDraw.unsetTextures();

   for (uint i = 0; i < 4; i++)
   {
#define SET_SS(s, v) BD3D::mpDev->SetSamplerState(i, s, v); 
      SET_SS(D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP)
      SET_SS(D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP)
      SET_SS(D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP)
      SET_SS(D3DSAMP_MAGFILTER, D3DTEXF_LINEAR)
      SET_SS(D3DSAMP_MINFILTER, D3DTEXF_LINEAR)
      SET_SS(D3DSAMP_MIPFILTER, D3DTEXF_POINT)
      SET_SS(D3DSAMP_MIPMAPLODBIAS, 0)
      SET_SS(D3DSAMP_MAXMIPLEVEL, 0)
      SET_SS(D3DSAMP_MAXANISOTROPY, 1)
      SET_SS(D3DSAMP_MAGFILTERZ, D3DTEXF_POINT)
      SET_SS(D3DSAMP_MINFILTERZ, D3DTEXF_POINT)
      SET_SS(D3DSAMP_SEPARATEZFILTERENABLE, FALSE)
#undef SET_SS   
   }
}

//============================================================================
// BLitDecalManager::renderDrawLitDecalsEndTile
//============================================================================
void BLitDecalManager::renderDrawLitDecalsEndTile(uint tileIndex)
{
   BDEBUG_ASSERT(mRenderBegunDrawing);
   tileIndex;

   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
   BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 0);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);

   mRenderBegunDrawing = false;
}

//============================================================================
// BLitDecalManager::renderDrawSortedLitDecals
//============================================================================
void BLitDecalManager::renderDrawSortedLitDecals(const BLitDecalHashBucket*  hashBucket, BVec3 minBounds, BVec3 maxBounds, uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming, bool categoryEarlyOut, int pass)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mRenderBegunDrawing);
   tileIndex;

   BManagedTextureHandle curDiffuseTextureHandle = cInvalidManagedTextureHandle;
   BManagedTextureHandle curNormalTextureHandle = cInvalidManagedTextureHandle;
   BManagedTextureHandle curOpacityTextureHandle = cInvalidManagedTextureHandle;
   BManagedTextureHandle curSpecularTextureHandle = cInvalidManagedTextureHandle;
   bool curConformToTerrain = false;
   int curTessLevel = -1;
   int firstLitDecalIndex = -1;
   int lastLitDecalIndex = -1;
   int curBlendMode = -1;

   //copy our list of instances over, and sort them here..
   BDynamicRenderArray<ushort> sortedIndices;
   for(uint indIdx = 0;indIdx <hashBucket->mValues.getSize();indIdx++)
      sortedIndices.add((ushort)hashBucket->mValues[indIdx]);

   if (sortedIndices.getSize())
      std::sort(sortedIndices.begin(), sortedIndices.end(), BLitDecalAttribAlphaKeySorter(*this));


   //setup lighting here..
   int numLights = setupLocalLighting(minBounds,maxBounds);

   numLights;

   //traverse our list and batch render common types
   for (uint sortedLitDecalIndex = 0; sortedLitDecalIndex < sortedIndices.getSize(); sortedLitDecalIndex++)
   {
      const uint LitDecalIndex = sortedIndices[sortedLitDecalIndex];

      BDEBUG_ASSERT(mRenderLitDecalAttribs.isItemAllocated(LitDecalIndex));

      const BLitDecalAttribs& LitDecalAttribs = *static_cast<const BLitDecalAttribs*>(mRenderLitDecalAttribs.getItem(LitDecalIndex));

      BDEBUG_ASSERT(LitDecalAttribs.getEnabled());

      if (filter != cRFAll)
      {
         const bool conformToTerrain = LitDecalAttribs.getConformToTerrain();
         if (filter == cRFConformToTerrain)
         {
            if (!conformToTerrain)
               continue;
         }
         else if (filter == cRFNonConformToTerrain)
         {
            if (conformToTerrain)
               continue;
         }
      }

      if (categoryIndex != -1)
      {
         if ((categoryEarlyOut) && ((int)LitDecalAttribs.getCategoryIndex() > (int)categoryIndex))
            break;
         else if ((int)LitDecalAttribs.getCategoryIndex() != (int)categoryIndex)
            continue;
      }

      if ( (curBlendMode            != LitDecalAttribs.getBlendMode())         ||
         (curDiffuseTextureHandle   != LitDecalAttribs.getDiffuseTextureHandle())     ||
         (curNormalTextureHandle != LitDecalAttribs.getNormalTextureHandle()) ||
         (curOpacityTextureHandle != LitDecalAttribs.getOpacityTextureHandle()) ||
         (curSpecularTextureHandle != LitDecalAttribs.getSpecularTextureHandle()) ||
         
         (curConformToTerrain     != LitDecalAttribs.getConformToTerrain())  ||
         (curTessLevel            != (int)LitDecalAttribs.getTessLevel()) )
      {
         if (firstLitDecalIndex != -1)
         {
            
               renderDrawLitDecalRange(sortedIndices, firstLitDecalIndex, lastLitDecalIndex, stencilTestWhenConforming);
         }

         curBlendMode         = LitDecalAttribs.getBlendMode();
         curDiffuseTextureHandle     = LitDecalAttribs.getDiffuseTextureHandle();
         curNormalTextureHandle     = LitDecalAttribs.getNormalTextureHandle();
         curOpacityTextureHandle     = LitDecalAttribs.getOpacityTextureHandle();
         curSpecularTextureHandle     = LitDecalAttribs.getSpecularTextureHandle();
         curConformToTerrain  = LitDecalAttribs.getConformToTerrain();
         curTessLevel         = LitDecalAttribs.getTessLevel();

         firstLitDecalIndex = sortedLitDecalIndex;   
         lastLitDecalIndex = sortedLitDecalIndex;
      }  
      else 
      {
         BDEBUG_ASSERT(firstLitDecalIndex != -1);
         lastLitDecalIndex = sortedLitDecalIndex;
      }
   }

   if (firstLitDecalIndex != -1)
   {
     
         renderDrawLitDecalRange(sortedIndices, firstLitDecalIndex, lastLitDecalIndex, stencilTestWhenConforming);
   }

   sortedIndices.resize(0);
}

//============================================================================
// BLitDecalManager::renderDrawAlphaLitDecals
//============================================================================
void BLitDecalManager::renderDrawAlphaLitDecals(uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming)
{
   ASSERT_THREAD(cThreadIndexRender);
   BDEBUG_ASSERT(mRenderBegunDrawing);

   SCOPEDSAMPLE(DrawAlphaLitDecals);


   BLitDecalGridHashMap::iterator it(mHashGrid.begin());

   if(it==mHashGrid.end())
      return;

   
   BD3D::mpDev->GpuOwnPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

   do
   {      
      const BLitDecalHashBucket* pValue = (BLitDecalHashBucket*) &it->second;

      const BLitDecalKey* pKey = (BLitDecalKey*) &it->first;

      BVec3 minBounds(  (float)(pKey->x * eVoxelXDim),  
                        -300.0f,
                        (float)(pKey->z * eVoxelZDim));

      BVec3 maxBounds(  (float)((pKey->x+1)* eVoxelXDim),
                        300.0f,
                        (float)((pKey->z+1)* eVoxelZDim));

      renderDrawSortedLitDecals(pValue,minBounds,maxBounds, tileIndex, categoryIndex, filter, stencilTestWhenConforming, false, -1);

      it++;
   }while(it!=mHashGrid.end());

   BD3D::mpDev->GpuDisownPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);



}
//============================================================================
// BLitDecalManager::renderDrawLitDecalRange
//============================================================================
void BLitDecalManager::renderDrawLitDecalRange(const BDynamicRenderArray<ushort>& sortedIndices, int firstSortedLitDecalIndex, int lastSortedLitDecalIndex, bool stencilTestWhenConforming)
{
   const uint numLitDecals = lastSortedLitDecalIndex - firstSortedLitDecalIndex + 1;

   const uint firstLitDecalIndex = sortedIndices[firstSortedLitDecalIndex];
   const BLitDecalAttribs& firstLitDecalAttribs = *static_cast<const BLitDecalAttribs*>(mRenderLitDecalAttribs.getItem(firstLitDecalIndex));

   const BLitDecalAttribs::eBlendMode blendMode = firstLitDecalAttribs.getBlendMode();

   gTerrainHeightField.setLightBufferingParams();
   
#ifndef BUILD_FINAL
   if(BTerrain::getVisMode() !=cVMDisabled )
   {
      BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
      BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
      BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
      setVisControlRegs();
   }
   else
#endif   
   {
      gTerrainHeightField.setBlackmapShaderParms(gTerrainRender.getBlackmapParams());
            
      switch (blendMode)
      {  
         case BLitDecalAttribs::cBlendOver:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);

            static BOOL zWriteEnable = FALSE;
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);
            break;
         }
         case BLitDecalAttribs::cBlendAdditive:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            break;
         }
         default:
         {
            BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
            BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
            BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
            BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, TRUE);
            BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            break;
         }
      }
   }


   // if we have an index to a flash movie but did not get a texture then bail
  
   {
      const BManagedTextureHandle diffuseTextureHandle = firstLitDecalAttribs.getDiffuseTextureHandle();
      const BManagedTextureHandle normalTextureHandle = firstLitDecalAttribs.getNormalTextureHandle();
      const BManagedTextureHandle opacityTextureHandle = firstLitDecalAttribs.getOpacityTextureHandle();
      const BManagedTextureHandle specularTextureHandle = firstLitDecalAttribs.getSpecularTextureHandle();
      gRenderDraw.setTextureByHandle(0, diffuseTextureHandle, cDefaultTextureBlack);
      gRenderDraw.setTextureByHandle(1, normalTextureHandle, cDefaultTextureNormal);
      gRenderDraw.setTextureByHandle(2, opacityTextureHandle, cDefaultTextureBlack);
      gRenderDraw.setTextureByHandle(3, specularTextureHandle, cDefaultTextureBlack);
   }

   const bool conformToTerrain = firstLitDecalAttribs.getConformToTerrain();
   const int tessLevel = firstLitDecalAttribs.getTessLevel();

   if (stencilTestWhenConforming)
   {
      BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, conformToTerrain ? TRUE : FALSE);
   }

   float depthBias = conformToTerrain ? mConformZBias : mNonConformZBias;;
   BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, depthBias));      

   mRenderPatchInstances.resize(numLitDecals);

   for (uint i = 0; i < numLitDecals; i++)
   {
      const uint LitDecalIndex = sortedIndices[firstSortedLitDecalIndex + i];
      const BLitDecalAttribs& LitDecalAttribs = *static_cast<const BLitDecalAttribs*>(mRenderLitDecalAttribs.getItem(LitDecalIndex));
      //const BLitDecalState& LitDecalState = mRenderLitDecalState[LitDecalIndex];

      BTerrainHeightField::BPatchInstance& patch = mRenderPatchInstances[i];

      patch.mWorldPos = LitDecalAttribs.getPos();
      BVec3 forward(LitDecalAttribs.getForward());

      BVec3 right(forward % BVec3(0.0f, 1.0f, 0.0f));

      forward *= LitDecalAttribs.getSizeZ();
      right *= LitDecalAttribs.getSizeX();

      patch.mForwardX = XMConvertFloatToHalf(forward[0]);
      patch.mForwardY = XMConvertFloatToHalf(forward[1]);
      patch.mForwardZ = XMConvertFloatToHalf(forward[2]);

      patch.mRightX = XMConvertFloatToHalf(right[0]);
      patch.mRightY = XMConvertFloatToHalf(right[1]);
      patch.mRightZ = XMConvertFloatToHalf(right[2]);
      patch.mYOffset = LitDecalAttribs.mYOffset.getBits();
      patch.mIntensity = LitDecalAttribs.mIntensity.getBits();

      patch.mU = LitDecalAttribs.mU.getBits();
      patch.mV = LitDecalAttribs.mV.getBits();
      patch.mWidth = LitDecalAttribs.mUWidth.getBits();
      patch.mHeight = LitDecalAttribs.mVHeight.getBits();

      patch.mColor = LitDecalAttribs.mColor;
   }

   const float yLowRange = 10.0f;
   const float yHighRange = 10.0f;

#ifndef BUILD_FINAL
   BTerrainHeightField::eRenderPassIndex passNum = BTerrain::getVisMode()==cVMDisabled? BTerrainHeightField::eLitDecalPatch:BTerrainHeightField::eVisDecalPatch;
#else
   BTerrainHeightField::eRenderPassIndex passNum = BTerrainHeightField::eLitDecalPatch;
#endif   

   gTerrainHeightField.renderPatches(mRenderPatchInstances.getPtr(), numLitDecals, float(tessLevel), yLowRange, yHighRange, conformToTerrain, passNum);
}



//============================================================================
// BLitDecalManager::setVisControlRegs
//============================================================================
void BLitDecalManager::setVisControlRegs()
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

         BD3D::mpDev->SetPixelShaderConstantF(VIS_CONTROL_0_REG, control, 4);                     

         break;
      }
   }
#endif  
}









//============================================================================
// BLitDecalManager::initKey
//============================================================================
void BLitDecalManager::initKey(const BVec3& pos, BLitDecalKey& key) const
{
   key.set( 
      Math::FloatToIntTrunc( pos.getX() / eVoxelXDim), 
      Math::FloatToIntTrunc( pos.getY() / eVoxelYDim), 
      Math::FloatToIntTrunc( pos.getZ() / eVoxelZDim) );
}