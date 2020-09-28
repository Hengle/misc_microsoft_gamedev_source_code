//============================================================================
//
//  File: TerrainRibbon.cpp
//
//  Copyright (c) 2008 Ensemble Studios
//
/*

TODO :
+ early out point rejection on the sim thread so we're not spamming the render work distributer with information

+ 'pause' updating if sim is paused

+ Handle tight curves on the warthog better (if angle between this node, and last node is within talus, (but not yet above min dist) insert the node..)
+ Handle 'backing up' more gracefully (if the direction from the last point to the next point is negative, then stop the strip, and make a new one)

*/
//============================================================================

// xterrain
#include "terrainpch.h"
#include "terrain.h"
#include "TerrainRibbon.h"
#include "TerrainHeightField.h"

// xrender
#include "vertexTypes.h"
#include "fixedFuncShaders.h"

// xgamerender
#include "tiledAA.h"

BTerrainRibbonManager gTerrainRibbonManager;

// shaders
#include "defConstRegs.inc"
#include "..\shaders\terrain\gpuTerrainshaderRegs.inc"
#include "..\shaders\shared\localLightingRegs.inc"
#if TERRAIN_SHADER_REGS_VER != 101
#error Please update gpuTerrainShaderRegs.inc
#endif

//============================================================================
// BTerrainRibbon::BTerrainRibbon
//============================================================================
BTerrainRibbon::BTerrainRibbon():
mStopped(false),
mDiffuseTexHandle(cInvalidManagedTextureHandle),
mNormalTexHandle(cInvalidManagedTextureHandle),
mOpacityTexHandle(cInvalidManagedTextureHandle),
mSpecularTexHandle(cInvalidManagedTextureHandle)
{

}

//============================================================================
// BTerrainRibbon::~BTerrainRibbon
//============================================================================
BTerrainRibbon::~BTerrainRibbon()
{ 
   destroy();
}

//============================================================================
// BTerrainRibbon::init
//============================================================================
void BTerrainRibbon::init(BRibbonCreateParams parms, BTerrainRibbonManager* pOwnerManager)
{
   parms.copyTo(mParams);
   
   mStopped=false;

   mDiffuseTexHandle = pOwnerManager->getManagedTextureHandle(BFixedString256(cVarArg, "%s_df", parms.mTextureName.getPtr()));
   mNormalTexHandle = pOwnerManager->getManagedTextureHandle(BFixedString256(cVarArg, "%s_nm", parms.mTextureName.getPtr()));
   mOpacityTexHandle = pOwnerManager->getManagedTextureHandle(BFixedString256(cVarArg, "%s_op", parms.mTextureName.getPtr()));
   mSpecularTexHandle = pOwnerManager->getManagedTextureHandle(BFixedString256(cVarArg, "%s_sp", parms.mTextureName.getPtr()));
   
   mAnchorNodes.reserve(mParams.mMaxNodes);

   //CLM this is for non-archive builds here.
  /* if(!gArchiveManager.getArchivesEnabled())
   {
      if(mDiffuseTexHandle == cInvalidManagedTextureHandle)
         mDiffuseTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_df", parms.mTextureName.getPtr()), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureWhite, true, false, "BTerrainRibbon");
      if(mNormalTexHandle == cInvalidManagedTextureHandle)
         mNormalTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_nm", parms.mTextureName.getPtr()), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureNormal, true, false, "BTerrainRibbon");
      if(mOpacityTexHandle == cInvalidManagedTextureHandle)
         mOpacityTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_op", parms.mTextureName.getPtr()), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureWhite, true, false, "BTerrainRibbon");
      if(mSpecularTexHandle == cInvalidManagedTextureHandle)
         mSpecularTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_sp", parms.mTextureName.getPtr()), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureBlack, true, false, "BTerrainRibbon");
   }   */
}


//============================================================================
// BTerrainRibbon::deinit
//============================================================================
void BTerrainRibbon::deinit()
{
   mStopped=true;
}

//============================================================================
// BTerrainRibbon::Destroy
//============================================================================
void BTerrainRibbon::destroy()
{
   mAnchorNodes.clear();
   mParams.clear();

   mStopped = true;
   
   //the texture manager owns these handles, so we just null them here.
   mDiffuseTexHandle = cInvalidManagedTextureHandle;
   mNormalTexHandle = cInvalidManagedTextureHandle;
   mOpacityTexHandle = cInvalidManagedTextureHandle;
   mSpecularTexHandle = cInvalidManagedTextureHandle;
}

//============================================================================
// BTerrainRibbon::update
//============================================================================
void BTerrainRibbon::update()
{
   ASSERT_RENDER_THREAD

   mMinBB.set(INT_MAX,-300.0f,INT_MAX);
   mMaxBB.set(INT_MIN,300.0f,INT_MIN);

   for(uint i = 0 ; i < mAnchorNodes.getSize(); i++)
   {
      BAnchorNode* pPtr = &mAnchorNodes[i];

      if(pPtr->mFullAlphaTime>0)
         pPtr->mFullAlphaTime--;
      else
         pPtr->mLife --;

      if(pPtr->mPositionX < mMinBB.x)   mMinBB.x = pPtr->mPositionX;
      if(pPtr->mPositionZ < mMinBB.z)   mMinBB.z = pPtr->mPositionZ;

      if(pPtr->mPositionX > mMaxBB.x)   mMaxBB.x = pPtr->mPositionX;
      if(pPtr->mPositionZ > mMaxBB.z)   mMaxBB.z = pPtr->mPositionZ;


      if(pPtr->mLife < 0)
      {
         mAnchorNodes.removeIndex(i);
         i--;
      }
   }
}

//============================================================================
// BTerrainRibbon::render
//============================================================================
void BTerrainRibbon::render()
{
   ASSERT_RENDER_THREAD

   const uint nodeCount = mAnchorNodes.getSize();
   if(nodeCount <= 1 )
      return;

   const uint numVerts = nodeCount * 2;
   const uint vertSize = sizeof(BPNTIVertex);

   BPNTIVertex* pTVB=(BPNTIVertex*)gRenderDraw.lockDynamicVB(numVerts, vertSize);
   BPNTIVertex* v = pTVB;

   BVector2 lastATang;
   BVector2 lastBTang;
  
   unsigned int vIndex = 0;
   
   const float rcpFadeAlphaFrameCount = 1.0f / (float)mParams.mNodeAmountFramesFading;
   for(unsigned int i = 0; i < nodeCount; i++, vIndex+=2)
   {
      calcTangentPoints(i, lastATang, lastBTang);

      const float lf =(float) mAnchorNodes[i].mLife;
      const float fatime = (float) mAnchorNodes[i].mFullAlphaTime;

      float lifeAlpha = 1.0f;
      if(fatime <= 0)
       lifeAlpha = lf  * rcpFadeAlphaFrameCount;
      

      v[vIndex].pos.x = lastBTang.x;
      v[vIndex].pos.y = 0;
      v[vIndex].pos.z = lastBTang.y;
      v[vIndex].intensity = lifeAlpha;
      v[vIndex].tu = 0;
      v[vIndex].tv = (float)i;

      v[vIndex + 1].pos.x = lastATang.x;
      v[vIndex + 1].pos.y = 0;
      v[vIndex + 1].pos.z = lastATang.y;
      v[vIndex + 1].intensity = lifeAlpha;
      v[vIndex + 1].tu = 1;
      v[vIndex + 1].tv = (float)i;
   }

//-- FIXING PREFIX BUG ID 6997
   const D3DVertexBuffer* pVB = gRenderDraw.getDynamicVB();
//--

   gRenderDraw.unlockDynamicVB();
  
   gD3DTextureManager.setManagedTextureByHandle(mDiffuseTexHandle == cInvalidManagedTextureHandle?
      gD3DTextureManager.getDefaultTextureHandle(cDefaultTextureWhite):mDiffuseTexHandle,0);

   gD3DTextureManager.setManagedTextureByHandle(mNormalTexHandle == cInvalidManagedTextureHandle?
      gD3DTextureManager.getDefaultTextureHandle(cDefaultTextureNormal):mNormalTexHandle,1);

   gD3DTextureManager.setManagedTextureByHandle(mOpacityTexHandle == cInvalidManagedTextureHandle?
      gD3DTextureManager.getDefaultTextureHandle(cDefaultTextureWhite):mOpacityTexHandle,2);

   gD3DTextureManager.setManagedTextureByHandle(mSpecularTexHandle == cInvalidManagedTextureHandle?
      gD3DTextureManager.getDefaultTextureHandle(cDefaultTextureBlack):mSpecularTexHandle,3);
      

   setupLocalLighting(BVec3(mMinBB.x,mMinBB.y,mMinBB.z),BVec3(mMaxBB.x,mMaxBB.y,mMaxBB.z));

   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);


#ifndef BUILD_FINAL
   BTerrainHeightField::eRenderPassIndex passNum = BTerrain::getVisMode()==cVMDisabled? BTerrainHeightField::eLitRibbon : BTerrainHeightField::eVisRibbon;
#else
   BTerrainHeightField::eRenderPassIndex passNum = BTerrainHeightField::eLitRibbon;
#endif   


   gTerrainHeightField.renderRibbon(pVB, vertSize, numVerts, passNum);


   gRenderDraw.clearStreamSource(0);


}



//============================================================================
// BTerrainRibbon::render
//============================================================================
int BTerrainRibbon::setupLocalLighting(BVec3 min, BVec3 max)
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
// BTerrainRibbon::getNumAnchors
//============================================================================
uint BTerrainRibbon::getNumAnchors()
{
   //this should be threadsafe since it's a simple read.
   //Although depending on your container, it could cause a problem...
   return mAnchorNodes.getSize();
}


//============================================================================
// BTerrainRibbon::getLastAnchorNode
//============================================================================
BTerrainRibbon::BAnchorNode* BTerrainRibbon::getLastAnchorNode()
{
   ASSERT_RENDER_THREAD

   const unsigned int nodeSize = mAnchorNodes.getSize();
   if(nodeSize ==0)
      return NULL;

   BASSERT((nodeSize-1) < mParams.mMaxNodes);

   return &mAnchorNodes[nodeSize-1];
}

//============================================================================
// BTerrainRibbon::addPoint
//============================================================================
bool BTerrainRibbon::addPoint(float x, float z, float dirX, float dirZ)
{
   ASSERT_RENDER_THREAD

   const uint nodeCount = mAnchorNodes.getSize();
   if(nodeCount >= mParams.mMaxNodes || isStopped())
      return false;

   

   //ensure we're 'far' enough away..
   if(nodeCount >0)
   {
//-- FIXING PREFIX BUG ID 7000
      const BAnchorNode* pAn = &mAnchorNodes[mAnchorNodes.getSize()-1];
//--
      BVector2 td(x,z);
      BVector2 tp(pAn->mPositionX,pAn->mPositionZ);
      BVector2 diff = td - tp;
      float dist = vec2Len(diff);
      if(dist < mParams.mMinDist)
         return false;
   }

  

   BAnchorNode an;
   mAnchorNodes.add(an);
   BAnchorNode* pAn = &mAnchorNodes[mAnchorNodes.getSize()-1];
   pAn->mPositionX = x;
   pAn->mPositionZ = z;
   pAn->mDirectionX= dirX;
   pAn->mDirectionZ= dirZ;
   pAn->mLife = mParams.mNodeAmountFramesFading;
   pAn->mFullAlphaTime = mParams.mNodeAmountFramesFullAlpha;


   return true;
}

//============================================================================
// BTerrainRibbon::calcTangentPoints
//============================================================================
void BTerrainRibbon::calcTangentPoints(uint anchorIndex, BVector2& tangA, BVector2 &tangB)
{
   ASSERT_RENDER_THREAD

//-- FIXING PREFIX BUG ID 7003
   const BAnchorNode* pCurrNode = &mAnchorNodes[anchorIndex];
//--

   float DirectionX = mAnchorNodes[anchorIndex].mDirectionX;
   float DirectionZ = mAnchorNodes[anchorIndex].mDirectionZ;

   if(anchorIndex > 0)
   {
      
//-- FIXING PREFIX BUG ID 7001
      const BAnchorNode* pEndNode = &mAnchorNodes[anchorIndex - 1];
//--
      //we set the direction back to the anchor node here so that new strips can use it later on.
      mAnchorNodes[anchorIndex].mDirectionX = DirectionX = pCurrNode->mPositionX - pEndNode->mPositionX;
      mAnchorNodes[anchorIndex].mDirectionZ = DirectionZ = pCurrNode->mPositionZ - pEndNode->mPositionZ;
   }
   else
   {
      if(anchorIndex +1 < mAnchorNodes.getSize())
      {
//-- FIXING PREFIX BUG ID 7002
         const BAnchorNode* pEndNode = &mAnchorNodes[anchorIndex +1];
//--
         //we set the direction back to the anchor node here so that new strips can use it later on.
         mAnchorNodes[anchorIndex].mDirectionX = DirectionX = -(pCurrNode->mPositionX - pEndNode->mPositionX);
         mAnchorNodes[anchorIndex].mDirectionZ = DirectionZ = -(pCurrNode->mPositionZ - pEndNode->mPositionZ);
      }
      else
      {
         //  BAnchorNode* pEndNode = &mAnchorNodes[anchorIndex + 1];
         //   DirectionX = pEndNode->mPositionX - pCurrNode->mPositionX;
         //   DirectionZ = pEndNode->mPositionZ - pCurrNode->mPositionZ;
      }    
   }
   
   


   BVector2 tangVec(-DirectionZ, DirectionX);
   vec2Normalize(tangVec);
   
   
   
   tangA.x = pCurrNode->mPositionX;
   tangA.y = pCurrNode->mPositionZ;
   tangA += tangVec * mParams.mRibbonWidth;
   
   tangB.x = pCurrNode->mPositionX;
   tangB.y = pCurrNode->mPositionZ;
   tangB += tangVec * -mParams.mRibbonWidth;
}

//============================================================================
// BTerrainRibbon::vec2Len
//============================================================================
float BTerrainRibbon::vec2Len(BVector2 vec)
{
   return Math::fSqrt(vec.x * vec.x + vec.y * vec.y);
}

//============================================================================
// BTerrainRibbon::vec2Normalize
//============================================================================
void BTerrainRibbon::vec2Normalize(BVector2& vec)
{
   float len = vec2Len(vec);
   if(len == 0)
      return;
   vec.x /= len;
   vec.y /= len;
}





//============================================================================
// BTerrainRibbon::vec2Facing
//============================================================================
float BTerrainRibbon::vec2Facing(BVector2 vecA, BVector2 vecB)
{
   vec2Normalize(vecA);
   vec2Normalize(vecB);

   return (vecA.x * vecB.x + vecA.y * vecB.y + vecA.z * vecB.z);
}

//============================================================================
// BTerrainRibbon::shouldSplitRibbonForNextPoint
//============================================================================
bool BTerrainRibbon::shouldSplitRibbonForNextPoint(float x, float z)
{
   const int anchorIndex = mAnchorNodes.getSize()-1;
   if(anchorIndex <= 0)
      return false;

//-- FIXING PREFIX BUG ID 7006
   const BAnchorNode* pAn = &mAnchorNodes[anchorIndex];
//--
   BVector2 td(x,z);
   BVector2 tp(pAn->mPositionX,pAn->mPositionZ);
   BVector2 diff = td - tp;
   vec2Normalize(diff);

   BVector2 nodeDir(mAnchorNodes[anchorIndex].mDirectionX,mAnchorNodes[anchorIndex].mDirectionZ);
   vec2Normalize(nodeDir);

   float dotRes = (diff.x * nodeDir.x + diff.y * nodeDir.y + diff.z * nodeDir.z);
  
   //split if the direction of the two points are facing away from each other
   const float cTalusAngle = 0.5f;

   bool facing = dotRes > cTalusAngle;
   if(!facing)
      return true;

   return false;
}
//============================================================================
// BTerrainRibbon::BTerrainRibbonManager
//============================================================================
BTerrainRibbonManager::BTerrainRibbonManager():
mRibbonHandleCounter(0)
{

}

//============================================================================
// BTerrainRibbon::~BTerrainRibbonManager
//============================================================================
BTerrainRibbonManager::~BTerrainRibbonManager()
{

}
 

//============================================================================
// BTerrainRibbon::init
//============================================================================
void BTerrainRibbonManager::init()
{ 
   ASSERT_THREAD(cThreadIndexSim);
 
   commandListenerInit();

   mRibbons.reserve(cMaxNumRibbons);
}

//============================================================================
// BTerrainRibbon::deinit
//============================================================================
void BTerrainRibbonManager::deinit()
{
   ASSERT_THREAD(cThreadIndexSim);

   commandListenerDeinit();
}

//============================================================================
// BTerrainRibbon::deinit
//============================================================================
void BTerrainRibbonManager::destroy()
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
     clearRibbons();

     //free all of our decal handles
     BHashMap<BFixedString256, BManagedTextureHandle>::iterator iter;
     for(int i = 0 ;i< mDecalHandles.getSize(); i++)
         gD3DTextureManager.releaseManagedTextureByHandle(mDecalHandles.get(i).second);
     
      mDecalHandles.clear();
   }
   else
   {
      gRenderThread.submitCommand(mCommandHandle, cTRMDestroy);
   }

}

//============================================================================
// BTerrainRibbon::update
//============================================================================
void BTerrainRibbonManager::update()
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   BDynamicArray<BTerrainRibbonHandle> handlesToRemove;
   if(threadID == cThreadIndexRender)
   {
      for(uint i = 0; i <mRibbons.getSize(); i ++)
      {
         mRibbons[i].update();
         if(mRibbons[i].isStopped())
         {
            if(mRibbons[i].getNumAnchors() ==0)
            {
               handlesToRemove.add(mRibbons[i].getHandle());
            }
         }
      }


      for(uint i = 0 ; i < handlesToRemove.getSize(); i++)
         removeRibbon(handlesToRemove[i]);
      
      handlesToRemove.clear();
   }
   else
   {
      gRenderThread.submitCommand(mCommandHandle, cTRMUpdate);
   }
}

//============================================================================
// BTerrainRibbon::render
//============================================================================
void BTerrainRibbonManager::render(int tileIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   tileIndex;

   BD3D::mpDev->GpuOwnPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

   gTerrainHeightField.setLightBufferingParams();
   gTerrainHeightField.setBlackmapShaderParms(gTerrainRender.getBlackmapParams());

#ifndef BUILD_FINAL
   if(BTerrain::getVisMode() !=cVMDisabled )
   {
      BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
      BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
      BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

      //set vis control regs
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
   }
   else
#endif   
   {
      BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
   }

   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    //  BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    //  BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 0);
    //  BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);


      gRenderDraw.unsetTextures();

      for (uint i = 0; i < 4; i++)
      {
#define SET_SS(s, v) BD3D::mpDev->SetSamplerState(i, s, v); 
         SET_SS(D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP)
            SET_SS(D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP)
            SET_SS(D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP)
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

      float cDefaultZBias = -.00001f;//;
      BD3D::mpDev->SetRenderState(D3DRS_DEPTHBIAS, CAST(DWORD, cDefaultZBias));      


      BVector minBB;
      BVector maxBB;
      {
            for(uint i = 0; i <mRibbons.getSize(); i ++)
            {
               BTerrainRibbon* rb = &mRibbons[i];
               if(rb == NULL)
                  continue;
               minBB = rb->getMinBB();
               maxBB = rb->getMaxBB();

               bool isVisibleToFrustum = gTiledAAManager.getTileVolumeCuller(tileIndex).isAABBVisible(minBB, maxBB);
               if(isVisibleToFrustum)
                  rb->render();
            }
      }


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
      BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
      BD3D::mpDev->GpuDisownPixelShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);
}

//============================================================================
// BTerrainRibbon::clearRibbons
//============================================================================
void BTerrainRibbonManager::clearRibbons()
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      for(uint i = 0; i <mRibbons.getSize(); i ++)
      {
         mRibbons[i].destroy();
      }
      mRibbons.clear();
      mRibbonHandleCounter=0;
   }
   else
   {
      gRenderThread.submitCommand(mCommandHandle, cTRMClear);
   }
}

//============================================================================
// BTerrainRibbonManager::getRibbon
//============================================================================
void BTerrainRibbonManager::removeRibbon(BTerrainRibbonHandle handle)
{
   for(uint i = 0; i <mRibbons.getSize(); i ++)
   {
      if(mRibbons[i].getHandle() == handle)
      {
         mRibbons[i].destroy();
         mRibbons.removeIndex(i);
         return;
      }
   }
}

//============================================================================
// BTerrainRibbonManager::getRibbon
//============================================================================
BTerrainRibbon* BTerrainRibbonManager::getRibbon(BTerrainRibbonHandle handle)
{
   if(gEventDispatcher.getThreadIndex()!=cThreadIndexRender)
      return NULL;

   for(uint i = 0; i <mRibbons.getSize(); i ++)
      if(mRibbons[i].getHandle() == handle)
         return &mRibbons[i];

   return NULL;
}

//============================================================================
// BTerrainRibbonManager::getRibbon
//============================================================================
BTerrainRibbonHandle BTerrainRibbonManager::createRibbon(BRibbonCreateParams params)
{
   const uint numRibbons = mRibbons.getSize();
   if(numRibbons + 1 > cMaxNumRibbons)
      return cInvalidBTerrainRibbonHandle; 

   const uint handle = mRibbonHandleCounter++;

   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      
      params.mMyHandle = handle;
      createRibbonInternal(&params);
   }
   else
   {
      BRibbonCreateParams *lp = new(gRenderHeap) BRibbonCreateParams;
      
      lp->mTextureName = params.mTextureName;
      lp->mMaxNodes = params.mMaxNodes;
      lp->mMinDist = params.mMinDist;
      lp->mNodeAmountFramesFading = params.mNodeAmountFramesFading;
      lp->mNodeAmountFramesFullAlpha = params.mNodeAmountFramesFullAlpha;
      lp->mRibbonWidth = params.mRibbonWidth;
      lp->mMyHandle = handle;
      
      gRenderThread.submitCommand(mCommandHandle, cTRMAddRibbon, sizeof(BRibbonCreateParams*), &lp);
   }

   return handle;
}

//============================================================================
// BTerrainRibbonManager::addPointToRibbon
//============================================================================
void BTerrainRibbonManager::createRibbonInternal(const BRibbonCreateParams *params)
{
   const uint numRibbons = mRibbons.getSize();
   if(numRibbons + 1 > cMaxNumRibbons)
      return; 

   BTerrainRibbon rib;
   mRibbons.add(rib);

   BASSERT(numRibbons != mRibbons.getSize() && "Adding a ribbon element failed");

   mRibbons[numRibbons].init(*params, this);
}

//============================================================================
// BTerrainRibbonManager::addPointToRibbon
//============================================================================
void BTerrainRibbonManager::addPointToRibbon( BTerrainRibbonHandle handle, float x, float z, float dirX, float dirZ)
{
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      BAddPointData lp;
      lp.handle = handle;
      lp.x = x;
      lp.z = z;
      lp.dirX = dirX;
      lp.dirZ = dirZ;
      
      addPointToRibbonInternal(&lp);
   }
   else
   {

      BAddPointData *lp = reinterpret_cast<BAddPointData *>(gRenderThread.submitCommandBegin(mCommandHandle, cTRMAddPointToRibbon, sizeof(BAddPointData)));
      lp->handle = handle;
      lp->x = x;
      lp->z = z;
      lp->dirX = dirX;
      lp->dirZ = dirZ;
      gRenderThread.submitCommandEnd(sizeof(BAddPointData));
   }

}

//============================================================================
// BTerrainRibbonManager::addPointToRibbon
//============================================================================
void BTerrainRibbonManager::addPointToRibbonInternal(const BAddPointData* dat)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BTerrainRibbon* rib = const_cast<BTerrainRibbon*>(getRibbon(dat->handle));
   if(rib == NULL)
      return;

   const uint numAnchors = rib->getNumAnchors();
   const uint maxNodes = rib->getParams()->mMaxNodes;
   BRibbonCreateParams tParms;
   rib->getParams()->copyTo(tParms);
   //As a sanity check, if our ribbon is over the max count, create a NEW ribbon instead
   if(numAnchors >= (maxNodes-1))
   {

      const BTerrainRibbon::BAnchorNode* node = rib->getLastAnchorNode();
      BASSERT(node != NULL);

      rib->stopRibbon();
       
      //In order to spawn this new ribbon, but still have our parent not be updated
      //create the new ribbon, and change the hash pointer internally...
      BTerrainRibbonHandle newHandle = createRibbon(tParms);      
      if(newHandle == cInvalidBTerrainRibbonHandle)
         return;

      BTerrainRibbon* newRib = getRibbon(newHandle);


      newRib->addPoint(node->mPositionX,node->mPositionZ, node->mDirectionX, node->mDirectionZ);

      newRib->getParams()->mMyHandle = dat->handle;
      rib->getParams()->mMyHandle = newHandle;

   }
   //Split the ribbon if we're turning some weird angle that can cause visual problems
   else if (rib->shouldSplitRibbonForNextPoint(dat->x,dat->z))
   {
      //CLM should this be above the above line??
      rib->stopRibbon();

      //In order to spawn this new ribbon, but still have our parent not be updated
      //create the new ribbon, and change the hash pointer internally...
      BTerrainRibbonHandle newHandle = createRibbon(tParms);      
      if(newHandle == cInvalidBTerrainRibbonHandle)
         return;

      BTerrainRibbon* newRib = getRibbon(newHandle);

      newRib->addPoint(dat->x,dat->z,dat->dirX,dat->dirZ);

    

      newRib->getParams()->mMyHandle = dat->handle;
      rib->getParams()->mMyHandle = newHandle;

      
   }
   else
   {
      rib->addPoint(dat->x,dat->z,dat->dirX,dat->dirZ);
   }
}

//============================================================================
// BTerrainRibbonManager::stopRibbon
//============================================================================
void BTerrainRibbonManager::stopRibbon(BTerrainRibbonHandle handle)
{
   
   const uint threadID = gEventDispatcher.getThreadIndex();

   if(threadID == cThreadIndexRender)
   {
      BTerrainRibbon* rib = const_cast<BTerrainRibbon*>(getRibbon(handle));
      if(rib == NULL)
         return;
      rib->stopRibbon();
   }
   else
   {
      gRenderThread.submitCommand(mCommandHandle, cTRMStopRibbon,handle);
   }

}

//============================================================================
// BTerrainRibbonManager::loadAssets
//============================================================================
void BTerrainRibbonManager::loadAssets(BRibbonCreateParams parms)
{
   if(parms.mTextureName.isEmpty())
      return; 

   // CLM [09.08.08] this forces the texture manager to load these textures during archive loading
   // CLM [11.04.08] check the cache first so we're not churning through extra memory.
   BFixedString256 diffuseName(cVarArg, "%s_df", parms.mTextureName.getPtr());
   BFixedString256 normalName(cVarArg, "%s_nm", parms.mTextureName.getPtr());
   BFixedString256 opacityName(cVarArg, "%s_op", parms.mTextureName.getPtr());
   BFixedString256 specularName(cVarArg, "%s_sp", parms.mTextureName.getPtr());

   
   if(getManagedTextureHandle(diffuseName)==cInvalidManagedTextureHandle)
      mDecalHandles.insert(diffuseName, gD3DTextureManager.getOrCreateHandle(diffuseName, BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureWhite, true, false, "BTerrainRibbon"));

   if(getManagedTextureHandle(normalName)==cInvalidManagedTextureHandle)
      mDecalHandles.insert(normalName, gD3DTextureManager.getOrCreateHandle(normalName, BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureNormal, true, false, "BTerrainRibbon"));

   if(getManagedTextureHandle(opacityName)==cInvalidManagedTextureHandle)
      mDecalHandles.insert(opacityName, gD3DTextureManager.getOrCreateHandle(opacityName, BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureWhite, true, false, "BTerrainRibbon"));
   
   if(getManagedTextureHandle(specularName)==cInvalidManagedTextureHandle)
      mDecalHandles.insert(opacityName, gD3DTextureManager.getOrCreateHandle(specularName, BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainRibbon, false, cDefaultTextureBlack, true, false, "BTerrainRibbon"));
}

//============================================================================
// BTerrainRibbonManager::getManagedTextureHandle
//============================================================================
BManagedTextureHandle   BTerrainRibbonManager::getManagedTextureHandle(const BFixedString256& textureName)
{
   BHashMap<BFixedString256, BManagedTextureHandle>::iterator iter = mDecalHandles.find(textureName);
   if (iter != mDecalHandles.end())
      return  iter->second;

   return cInvalidManagedTextureHandle;
}

//============================================================================
// BTerrainRibbonManager::processCommand
//============================================================================
void BTerrainRibbonManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
   case cTRMDestroy:
      {
         destroy();
         break;
      }
   case cTRMClear:
      {
         clearRibbons();
         break;
      }
   case cTRMAddRibbon:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRibbonCreateParams*));
         BRibbonCreateParams* bDat = *(BRibbonCreateParams**)(pData);
         createRibbonInternal(bDat);
         HEAP_DELETE(bDat, gRenderHeap);
         break;
      }
   case cTRMUpdate:
      {
         update();
         break;
      }
   case cTRMAddPointToRibbon:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BAddPointData));
         const BAddPointData* bDat = reinterpret_cast<const BAddPointData*>(pData);
         addPointToRibbonInternal(bDat);
         break;
      }
   case cTRMStopRibbon:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         stopRibbon(*reinterpret_cast<const BTerrainRibbonHandle*>(pData));
         break;
      }
   }
}
