//============================================================================
//
//  TerrainFoliage.cpp
//  
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
// terrain 
#include "TerrainPCH.h"
#include "terrain.h"
#include "terrainFoliage.h"
#include "terrainrender.h"

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
#include "gamedirectories.h"

BTerrainFoliageManager gFoliageManager;


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

const float cFadeOutDist = 500.f;

#define FOLIAGE_EFFECT_FILENAME "terrain\\terrainFoliage.bin"

//============================================================================
// BTerrainFoliageManager::BTerrainFoliageManager
//============================================================================
BTerrainFoliageManager::BTerrainFoliageManager():
mFoliageSets(0),
mFoliageChunks(0),
mpEffectLoader(NULL),
mChunkVertexBuffer(0)
{
}
//============================================================================
// BTerrainFoliageManager::~BTerrainFoliageManager
//============================================================================
BTerrainFoliageManager::~BTerrainFoliageManager()
{

}
//============================================================================
// BTerrainFoliageManager::init
//============================================================================
bool BTerrainFoliageManager::init()
{
   ASSERT_MAIN_THREAD

   commandListenerInit();

   return true;
}
//============================================================================
// BTerrainFoliageManager::deinit
//============================================================================
bool  BTerrainFoliageManager::deinit()
{
   // Block for safety. 
   gRenderThread.blockUntilGPUIdle();

   commandListenerDeinit();


   return true;
}
//============================================================================
// BTerrainVisual::destroy
//============================================================================
void  BTerrainFoliageManager::destroy()
{
   gRenderThread.submitCommand(mCommandHandle,cTRM_Destroy);
}
//============================================================================
// BTerrainVisual::destroyInternal
//============================================================================
bool  BTerrainFoliageManager::destroyInternal()
{
   for(uint i=0;i<mFoliageSets.size();i++)
   {
      if(mFoliageSets[i])
      {
         delete mFoliageSets[i];
         mFoliageSets[i]=NULL;
      }
   }
   mFoliageSets.clear();
   for(uint i=0;i<mFoliageChunks.size();i++)
   {
      if(mFoliageChunks[i])
      {
         delete mFoliageChunks[i];
         mFoliageChunks[i] =NULL;
      }
   }
   mFoliageChunks.clear();

   return true;
}
//============================================================================
// BTerrainFoliageManager::processCommand
//============================================================================
void BTerrainFoliageManager::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
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
// BTerrainFoliageManager::frameBegin
//============================================================================
void BTerrainFoliageManager::frameBegin(void)
{
   ASSERT_RENDER_THREAD
   for(uint i=0;i<mFoliageSets.size();i++)
   {
      if(mFoliageSets[i]->needReload())
      {
         mFoliageSets[i]->reload();
      }
   }
};
//============================================================================
//  BTerrainFoliageManager::frameEnd
//============================================================================
void BTerrainFoliageManager::frameEnd(void)
{
   ASSERT_RENDER_THREAD
};
//============================================================================
// BTerrainFoliageManager::loadEffect
//============================================================================
void BTerrainFoliageManager::loadEffect(void)
{
   ASSERT_RENDER_THREAD
      if (!mpEffectLoader)
      {
         mpEffectLoader = ALIGNED_NEW(BFXLEffectFileLoader, gRenderHeap);
         const bool status = mpEffectLoader->init(gRender.getEffectCompilerDefaultDirID(), FOLIAGE_EFFECT_FILENAME, true, false, true);
         BVERIFY(status);
      }
}   
//============================================================================
// BTerrainFoliageManager::tickEffect
//============================================================================
void BTerrainFoliageManager::tickEffect(void)
{
   BDEBUG_ASSERT(mpEffectLoader);

   if (mpEffectLoader->tick(true))
   {
      mTerrainFoliageShader.attach(mpEffectLoader->getFXLEffect().getEffect());

      initEffectConstants();
   }

   // We now never allow the terrain to render without a valid effect.
   BVERIFY(mTerrainFoliageShader.getEffect());

   mTerrainFoliageShader.updateIntrinsicParams();
}
//============================================================================
// BTerrainVisual::initEffectConstants
//============================================================================
void BTerrainFoliageManager::initEffectConstants(void)
{  
   ASSERT_RENDER_THREAD

      BDEBUG_ASSERT(mTerrainFoliageShader.getEffect());

   // Any HLSL variables that are manually updated via SetPixelShaderConstantF(), etc. must be marked for manual register update here!

   //RENDERING

   mShaderFoliageAlbedo                   = mTerrainFoliageShader("gFoliageAlbedo");
   mShaderFoliageNormal                   = mTerrainFoliageShader("gFoliageNormal");
   mShaderFoliageSpecular                 = mTerrainFoliageShader("gFoliageSpecular");
   mShaderFoliageOpacity                  = mTerrainFoliageShader("gFoliageOpacity");
   mShaderFoliagePositions                = mTerrainFoliageShader("gFoliagePositions");
   mShaderFoliageNormals                  = mTerrainFoliageShader("gFoliageNormals");
   mShaderFoliageAnimTime                 = mTerrainFoliageShader("gAnimTime");
   mShaderFoliageBacksideShadowScalar     = mTerrainFoliageShader("gBacksideShadowScalar");

   //TERRRAIN
   mShaderTerrainDataValsHandle           = mTerrainFoliageShader("g_terrainVals");

   mPosComprMin                           = mTerrainFoliageShader("g_posCompMin");
   mPosComprRange                         = mTerrainFoliageShader("g_posCompRange");

   mShaderTerrainPositionTextureHandle    = mTerrainFoliageShader("vertSampler_pos");
   mShaderTerrainBasisTextureHandle       = mTerrainFoliageShader("vertSampler_basis");

   //BLACKMAP
   mBlackmapEnabled                        = mTerrainFoliageShader("gBlackmapEnabled");
   mBlackmapSampler                        = mTerrainFoliageShader("gBlackmapSampler");
   mBlackmapUnexploredSampler              = mTerrainFoliageShader("gBlackmapUnexploredSampler");
   mBlackmapParams0                        = mTerrainFoliageShader("gBlackmapParams0");
   mBlackmapParams1                        = mTerrainFoliageShader("gBlackmapParams1");
   mBlackmapParams2                        = mTerrainFoliageShader("gBlackmapParams2");

   //Lighting
   mLocalLightingEnabled                   = mTerrainFoliageShader("gLocalLightingEnabled");
   mLocalLightingEnabled.setRegisterUpdateMode(true);

   mLocalShadowingEnabled                  = mTerrainFoliageShader("gLocalShadowingEnabled");
   mLocalShadowingEnabled.setRegisterUpdateMode(true);

   mLightData                              = mTerrainFoliageShader("gLightData");
   mLightData.setRegisterUpdateMode(true);

   mTerrainFoliageShader("gVisControl0").setRegisterUpdateMode(true);
   mTerrainFoliageShader("gVisControl1").setRegisterUpdateMode(true);
   mTerrainFoliageShader("gVisControl2").setRegisterUpdateMode(true);
   mTerrainFoliageShader("gVisControl3").setRegisterUpdateMode(true);

   mTerrainFoliageShader("gNumLights").setRegisterUpdateMode(true);

   mTerrainFoliageShader("gExtendedLocalLightingEnabled").setRegisterUpdateMode(true);
   mTerrainFoliageShader("gNumExtendedLights").setRegisterUpdateMode(true);
   mTerrainFoliageShader("gExtendedLocalLightingParams").setRegisterUpdateMode(true);

   mCurrTechnique = mTerrainFoliageShader.getTechniqueFromIndex(0);
}
//============================================================================
// BTerrainFoliageManager::newSet
//============================================================================
void BTerrainFoliageManager::newSet(const char *ifilename)
{
   BTerrainFoliageSet *fs = new BTerrainFoliageSet();
   mFoliageSets.push_back(fs);
   mFoliageSets[mFoliageSets.size()-1]->loadSet(ifilename);
}

//============================================================================
// BTerrainFoliageManager::initDeviceData
//============================================================================
void BTerrainFoliageManager::initDeviceData(void)
{
   ASSERT_RENDER_THREAD

      loadEffect();

   
   
   //DUMB VB!
   BD3D::mpDev->CreateVertexBuffer(sizeof(BPVertex),0,0,0,&mChunkVertexBuffer,0);


  



   ////TEMP INITALIZATION!!!!
 //  newSet("foliage\\foliageset");
  // uint numTotalNodes = gTerrain.getNumXChunks()*gTerrain.getNumXChunks();



   ////POSITIONS
   //BD3D::mpDev->CreateLineTexture(10,1,0,D3DFMT_LIN_A32B32G32R32F,0,&mFoliageSets[0]->mPositionsTexture,0);
   //D3DLOCKED_RECT pLockedRect;
   //mFoliageSets[0]->mPositionsTexture->LockRect(0,&pLockedRect,0,0);
   //D3DXVECTOR4 *pVerts = (D3DXVECTOR4*)pLockedRect.pBits;
   //pVerts[0].x = -0.25f;      pVerts[0].y = 0.f;   pVerts[0].z = 0.f;   pVerts[0].w= 1.f;
   //pVerts[1].x =  0.25f;      pVerts[1].y = 0.f;   pVerts[1].z = 0.f;   pVerts[1].w= 1.f;
   //pVerts[2].x = -0.25f;      pVerts[2].y = 0.1f;   pVerts[2].z = 0.1f; pVerts[2].w= 1.f;
   //pVerts[3].x =  0.25f;      pVerts[3].y = 0.1f;   pVerts[3].z = 0.1f; pVerts[3].w= 1.f;
   //pVerts[4].x = -0.25f;      pVerts[4].y = 1.8f;   pVerts[4].z = 0.3f; pVerts[4].w= 1.f;
   //pVerts[5].x =  0.25f;      pVerts[5].y = 1.8f;   pVerts[5].z = 0.3f; pVerts[5].w= 1.f;
   //pVerts[6].x = -0.25f;      pVerts[6].y = 2.4f;   pVerts[6].z = 0.6f; pVerts[6].w= 1.f;
   //pVerts[7].x =  0.25f;      pVerts[7].y = 2.4f;   pVerts[7].z = 0.6f; pVerts[7].w= 1.f;
   //pVerts[8].x = -0.25f;      pVerts[8].y = 2.8f;   pVerts[8].z = 1.3f; pVerts[8].w= 1.f;
   //pVerts[9].x =  0.25f;      pVerts[9].y = 2.8f;   pVerts[9].z = 1.3f; pVerts[9].w= 1.f;

   //mFoliageSets[0]->mPositionsTexture->UnlockRect(0);


   ////NORMALS
   //BD3D::mpDev->CreateLineTexture(10,1,0,D3DFMT_LIN_A32B32G32R32F,0,&mFoliageSets[0]->mVertNormalsTexture,0);
   //mFoliageSets[0]->mVertNormalsTexture->LockRect(0,&pLockedRect,0,0);
   //pVerts = (D3DXVECTOR4*)pLockedRect.pBits;
   //pVerts[0].x = 0;      pVerts[0].y = 0.f;   pVerts[0].z = 1.f;    pVerts[0].w= 1.f;
   //pVerts[1].x = 0;      pVerts[1].y = 0.f;   pVerts[1].z = 1.f;    pVerts[1].w= 1.f;
   //pVerts[2].x = 0;      pVerts[2].y = 0.2f;   pVerts[2].z = 0.6f;   pVerts[2].w= 1.f;
   //pVerts[3].x = 0;      pVerts[3].y = 0.2f;   pVerts[3].z = 0.6f;   pVerts[3].w= 1.f;
   //pVerts[4].x = 0;      pVerts[4].y = 0.4f;   pVerts[4].z = 0.4f;   pVerts[4].w= 1.f;
   //pVerts[5].x = 0;      pVerts[5].y = 0.4f;   pVerts[5].z = 0.4f;   pVerts[5].w= 1.f;
   //pVerts[6].x = 0;      pVerts[6].y = 0.6f;   pVerts[6].z = 0.2f;   pVerts[6].w= 1.f;
   //pVerts[7].x = 0;      pVerts[7].y = 0.6f;   pVerts[7].z = 0.2f;   pVerts[7].w= 1.f;
   //pVerts[8].x = 0;      pVerts[8].y = 1.0f;   pVerts[8].z = 0.f;   pVerts[8].w= 1.f;
   //pVerts[9].x = 0;      pVerts[9].y = 1.0f;   pVerts[9].z = 0.f;   pVerts[9].w= 1.f;

   //mFoliageSets[0]->mVertNormalsTexture->UnlockRect(0);


}
//============================================================================
// BTerrainFoliageManager::deinitDeviceData
//============================================================================
void BTerrainFoliageManager::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
      if (mpEffectLoader)
      {
         ALIGNED_DELETE(mpEffectLoader, gRenderHeap);
         mpEffectLoader = NULL;
      }
      mTerrainFoliageShader.clear();
   if(mChunkVertexBuffer)
   {
      mChunkVertexBuffer->Release();
      mChunkVertexBuffer=NULL;
   }
}
//============================================================================
// BTerrainFoliageManager::render
//============================================================================
void BTerrainFoliageManager::renderCustom(eTRenderPhase renderPhase,const BTerrainQuadNodePtrArray* pNodes,const BTerrainQuadNode *terrainQuadGrid)
{
   SCOPEDSAMPLE(BFoliageRenderCustom);
   ASSERT_RENDER_THREAD
   BASSERT(pNodes);
   BASSERT(terrainQuadGrid);
      tickEffect();

   if(!mFoliageSets.size())
      return;

   //Set our terrain shader data
   mPosComprRange                         = gTerrainVisual.getPosRange();
   mPosComprMin                           = gTerrainVisual.getPosMin();
   mShaderTerrainPositionTextureHandle    = gTerrainVisual.getPosTex();
   mShaderTerrainBasisTextureHandle       = gTerrainVisual.getNormTex();
   mShaderFoliageAnimTime                 = mAnimTime;
   

   BD3D::mpDev->SetVertexDeclaration(BPVertex::msVertexDecl);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);   
   BD3D::mpDev->SetRenderState(D3DRS_PRIMITIVERESETENABLE,TRUE);  
   
  

   if(renderPhase == cTRP_ShadowGen)
   {
      BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKENABLE, FALSE);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

      static bool dithered = true;
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATOMASKOFFSETS, dithered ? D3DALPHATOMASK_DITHERED : D3DALPHATOMASK_SOLID);

      BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF,128);


      mCurrTechnique = mTerrainFoliageShader.getTechniqueFromIndex(1);   
   }
   else if(renderPhase == cTRP_Reflect)
   {
      BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
      BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
      BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 0x000000AA); 

      BD3D::mpDev->GpuOwnVertexShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);
      mCurrTechnique = mTerrainFoliageShader.getTechniqueFromIndex(0);   
   }

   mCurrTechnique.begin(0);
   mCurrTechnique.beginPass(0);
   for(uint qnI=0;qnI<mFoliageChunks.size();qnI++)
   {
      uint indx=mFoliageChunks[qnI]->mQNParentIndex;
      const BTerrainQuadNode *ptrToOwner = &terrainQuadGrid[indx];
      if(ptrToOwner==NULL) continue;
      for (uint i = 0; i < pNodes->size(); i++)
      {
         const BTerrainQuadNode *tPtr = (*pNodes)[i];
         if(tPtr ==NULL)continue;

         if(ptrToOwner == tPtr)
         {
            if(tPtr->mCacheInfo.mDistanceFromCamera > cFadeOutDist-100)
               continue;

            for(uint k=0;k<mFoliageChunks[qnI]->mNumSets;k++)
            {
               uint setIndex = mFoliageChunks[qnI]->mSetIndexes[k];

               if(setIndex >= mFoliageSets.getSize() || setIndex <0)
                  continue;

               setupTexturing(renderPhase,setIndex);

               //we ALWAYS set opacity. Regardless.
               //OPACITY
               if(mFoliageSets[setIndex]->mOpacityTexture.getBaseTexture())
                  mShaderFoliageOpacity = mFoliageSets[setIndex]->mOpacityTexture.getBaseTexture();
               else
                  mShaderFoliageOpacity = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);

               mShaderFoliagePositions = mFoliageSets[setIndex]->mPositionsTexture;
               mShaderFoliageNormals = mFoliageSets[setIndex]->mVertNormalsTexture;

               BTerrainQuadNodeDesc desc = terrainQuadGrid[indx].getDesc();
               mShaderTerrainDataValsHandle = D3DXVECTOR4(  1.0f / (float)gTerrainVisual.getNumXVerts() , (float)gTerrainVisual.getTileScale(), (float)desc.mMinXVert, (float)desc.mMinZVert);

               mShaderFoliageBacksideShadowScalar = mFoliageSets[setIndex]->mBacksideShadowScalar;

               mCurrTechnique.commitU();

               if(renderPhase != cTRP_ShadowGen)
                  setupLighting(desc);       //CLM speed this up later by merging this into terrainRender, and using the chunks' already calculated lighting data.

               BD3D::mpDev->SetStreamSource(0,mChunkVertexBuffer,0,sizeof(BPVertex));
               BD3D::mpDev->SetIndices(mFoliageChunks[qnI]->mSetIBs[k]);
               BD3D::mpDev->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP,0,0,0,0,mFoliageChunks[qnI]->mSetPolyCount[k]);


            }
         }
      }
   }

   mCurrTechnique.endPass(); 




   mCurrTechnique.end();
   BD3D::mpDev->SetIndices(0);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_PRIMITIVERESETENABLE,FALSE);  
   BD3D::mpDev->GpuDisownVertexShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 

}
//============================================================================
// BTerrainFoliageManager::render
//============================================================================
void BTerrainFoliageManager::render(const BTerrainQuadNode *terrainQuadGrid, int tileIndex)
{

   SCOPEDSAMPLE(BTerrainRenderFoliage);
   ASSERT_RENDER_THREAD
      tickEffect();

   if(!mFoliageSets.size())
     return;

   mCurrTechnique = mTerrainFoliageShader.getTechniqueFromIndex(0);   

   //Set our terrain shader data
   mPosComprRange                         = gTerrainVisual.getPosRange();
   mPosComprMin                           = gTerrainVisual.getPosMin();
   mShaderTerrainPositionTextureHandle    = gTerrainVisual.getPosTex();
   mShaderTerrainBasisTextureHandle       = gTerrainVisual.getNormTex();
   mShaderFoliageAnimTime                 = mAnimTime;

   if (!gTerrainRender.getBlackmapParams().mpTexture)
      mBlackmapEnabled = false;
   else
   {
      mBlackmapEnabled = true;
      mBlackmapSampler = gTerrainRender.getBlackmapParams().mpTexture;
      mBlackmapUnexploredSampler = gTerrainRender.getBlackmapParams().mpUnexploredTexture;
      BCOMPILETIMEASSERT(sizeof(gTerrainRender.getBlackmapParams().mParams)/sizeof(gTerrainRender.getBlackmapParams().mParams[0]) == 3);
      mBlackmapParams0 = gTerrainRender.getBlackmapParams().mParams[0];
      mBlackmapParams1 = gTerrainRender.getBlackmapParams().mParams[1];
      mBlackmapParams2 = gTerrainRender.getBlackmapParams().mParams[2];
   }


   BD3D::mpDev->SetVertexDeclaration(BPVertex::msVertexDecl);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);   
   BD3D::mpDev->SetRenderState(D3DRS_PRIMITIVERESETENABLE,TRUE);  
   BD3D::mpDev->GpuOwnVertexShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 0x000000AA); 

   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

   mCurrTechnique.begin(0);

   int passIndex=cRenderPassANS;
#ifndef BUILD_FINAL         
   switch (BTerrain::getVisMode())
   {
   case cVMDisabled:
      {
         break;
      }
   default:
      {
         passIndex=cRenderPassVis;
      }
   }
#endif

   mCurrTechnique.beginPass(passIndex);
   for(uint qnI=0;qnI<mFoliageChunks.size();qnI++)
   {
      uint indx=mFoliageChunks[qnI]->mQNParentIndex;
      if(terrainQuadGrid[indx].mVisibleInThisTile[tileIndex]) //are we visible?
      {
         if(terrainQuadGrid[indx].mCacheInfo.mDistanceFromCamera > cFadeOutDist)
            continue;

         for(uint k=0;k<mFoliageChunks[qnI]->mNumSets;k++)
         {
            uint setIndex = mFoliageChunks[qnI]->mSetIndexes[k];

            setupTexturing(cTRP_Full,setIndex);

            mShaderFoliagePositions = mFoliageSets[setIndex]->mPositionsTexture;
            mShaderFoliageNormals = mFoliageSets[setIndex]->mVertNormalsTexture;

            BTerrainQuadNodeDesc desc = terrainQuadGrid[indx].getDesc();
            mShaderTerrainDataValsHandle = D3DXVECTOR4(  1.0f / (float)gTerrainVisual.getNumXVerts() , (float)gTerrainVisual.getTileScale(), (float)desc.mMinXVert, (float)desc.mMinZVert);

            mCurrTechnique.commitU();

            setupLighting(desc);       //CLM speed this up later by merging this into terrainRender, and using the chunks' already calculated lighting data.

            BD3D::mpDev->SetStreamSource(0,mChunkVertexBuffer,0,sizeof(BPVertex));
            BD3D::mpDev->SetIndices(mFoliageChunks[qnI]->mSetIBs[k]);
            BD3D::mpDev->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP,0,0,0,0,mFoliageChunks[qnI]->mSetPolyCount[k]);

                   
         }
      }
   }

   mCurrTechnique.endPass(); 




   mCurrTechnique.end();
   BD3D::mpDev->SetIndices(0);
   BD3D::mpDev->SetPixelShader(NULL);
   BD3D::mpDev->SetVertexShader(NULL);
   BD3D::mpDev->SetRenderState(D3DRS_CULLMODE,D3DCULL_CCW);
   BD3D::mpDev->SetRenderState(D3DRS_PRIMITIVERESETENABLE,FALSE);  
   BD3D::mpDev->GpuDisownVertexShaderConstantF(0, NUM_LOCAL_LIGHT_PSHADER_CONSTANTS);

   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 


   mBlackmapSampler = NULL;
}
//============================================================================
// BTerrainFoliageManager::setupTexturing
//============================================================================
int BTerrainFoliageManager::setupTexturing(eTRenderPhase renderPhase,int foliageIndex)
{
   int passNum =cRenderPassANS; 
   
   if(foliageIndex >= (int)mFoliageSets.getSize() || foliageIndex < 0 )
      return passNum;

   if(renderPhase != cTRP_ShadowGen)
   {
      //ALBEDO
      if(mFoliageSets[foliageIndex]->mAlbedoTexture.getBaseTexture())
         mShaderFoliageAlbedo = mFoliageSets[foliageIndex]->mAlbedoTexture.getBaseTexture();
      else
         mShaderFoliageAlbedo = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureGreen);

      //NORMAL
      if(mFoliageSets[foliageIndex]->mNormalTexture.getBaseTexture())
         mShaderFoliageNormal = mFoliageSets[foliageIndex]->mNormalTexture.getBaseTexture();
      else
         mShaderFoliageNormal = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureNormal);

      //SPECULAR
      if(mFoliageSets[foliageIndex]->mSpecularTexture.getBaseTexture())
         mShaderFoliageSpecular = mFoliageSets[foliageIndex]->mSpecularTexture.getBaseTexture();
      else
         mShaderFoliageSpecular = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureBlack);


   }
   //we ALWAYS set opacity. Regardless.
   //OPACITY
   if(mFoliageSets[foliageIndex]->mOpacityTexture.getBaseTexture())
      mShaderFoliageOpacity = mFoliageSets[foliageIndex]->mOpacityTexture.getBaseTexture();
   else
      mShaderFoliageOpacity = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);

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
// BTerrainFoliageManager::setupLighting
//============================================================================
inline void BTerrainFoliageManager::setupLighting(const BTerrainQuadNodeDesc& desc)
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
      BD3D::mpDev->SetVertexShaderConstantB(ENABLE_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
      BD3D::mpDev->SetVertexShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, &boolValueFALSE, 1);
      BD3D::mpDev->SetVertexShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);

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

   BD3D::mpDev->SetVertexShaderConstantB(ENABLE_LOCAL_LIGHTS_REG, numNormalLights ? &boolValueTRUE : &boolValueFALSE, 1);

   const XMFLOAT4* pTexels = gVisibleLightManager.getVisibleLightTexels();

   for (uint i = 0; i < numNormalLights; i++)
   {
      BD3D::mpDev->GpuLoadVertexShaderConstantF4Pointer(i * 8, pTexels + 8 * mCurVisibleLightIndices[i], 8);
   }

   const int numLightsInt4[4] = { numNormalLights, 0, 8, 0 };
   BD3D::mpDev->SetVertexShaderConstantI(NUM_LOCAL_LIGHTS_REG, numLightsInt4, 1);

   BOOL boolValue = numShadowedLights > 0;
   BD3D::mpDev->SetVertexShaderConstantB(ENABLE_LOCAL_SHADOWING_REG, &boolValue, 1);

   if (!numExtendedLights)      
   {
      BD3D::mpDev->SetVertexShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, &boolValueFALSE, 1);
   }
   else
   {
      uint actualNumExtendedLights;
      uint texelIndex = fillLightAttribTexture(mCurVisibleLightIndices, numNormalLights, actualNumExtendedLights);

      BD3D::mpDev->SetVertexShaderConstantB(ENABLE_EXTENDED_LOCAL_LIGHTS_REG, actualNumExtendedLights ? &boolValueTRUE : &boolValueFALSE, 1);

      if (actualNumExtendedLights)
      {
         const int numExtendedLightsInt4[4] = { actualNumExtendedLights, 0, 1, 0 };
         BD3D::mpDev->SetVertexShaderConstantI(NUM_EXTENDED_LOCAL_LIGHTS_REG, numExtendedLightsInt4, 1);

         BVec4 extendedParams((float)texelIndex, 0.0f, 0.0f, 0.0f);
         BD3D::mpDev->SetVertexShaderConstantF(EXTENDED_LOCAL_LIGHTING_PARAMS_REG, extendedParams.getPtr(), 1);
      }      
   }      
}
//============================================================================
// BTerrainFoliageManager::allocateTempResources
//============================================================================
void BTerrainFoliageManager::allocateTempResources(void)
{
   BDEBUG_ASSERT(!mpLightAttribTexture);
   mpLightAttribTexture = gRenderDraw.createDynamicTexture(cLightAttribTexWidth, cLightAttribTexHeight, D3DFMT_LIN_A32B32G32R32F);
   BVERIFY(mpLightAttribTexture);

   mNextLightAttribTexelIndex = 0;
}

//============================================================================
// BTerrainFoliageManager::releaseTempResources
//============================================================================
void BTerrainFoliageManager::releaseTempResources(void)
{
   BDEBUG_ASSERT(mpLightAttribTexture);
   mpLightAttribTexture = NULL;
}

//============================================================================
// BTerrainFoliageManager::computeLightAttribTexelPtr
//============================================================================
XMFLOAT4* BTerrainFoliageManager::computeLightAttribTexelPtr(XMFLOAT4* pDstTexture, uint texelIndex)
{
   const uint x = texelIndex & (cLightAttribTexWidth - 1);
   const uint y = texelIndex >> cLightAttribTexWidthLog2;
   const uint dstBlockOfs = x + (y << cLightAttribTexWidthLog2);
   XMFLOAT4* pDst = pDstTexture + dstBlockOfs;
   return pDst;
}   

//============================================================================
// BTerrainFoliageManager::fillLightAttribTexture
//============================================================================
uint BTerrainFoliageManager::fillLightAttribTexture(const BSceneLightManager::BActiveLightIndexArray& lights, uint firstIndex, uint& outNumLights)
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
// BTerrainFoliageManager::fillLightAttribTexture
//============================================================================
void BTerrainFoliageManager::setVisControlRegs()
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
// BTerrainFoliageSet::setTextures
//============================================================================
void BTerrainFoliageManager::update()
{
   mAnimTime+=0.05f;
}
//============================================================================
// BTerrainFoliageSet::setTextures
//============================================================================
void BTerrainFoliageSet::loadSet(const char *texNameRoot)
{
   ASSERT_THREAD(cThreadIndexRender);

   mFilename = BFixedString256(texNameRoot);
   deinit();
   init();

   loadTextures();
   loadPositionsXML();

   ///Setup our reload events
   BReloadManager::BPathArray paths;
   BString pathXML("art\\"); pathXML +=mFilename.c_str();pathXML += ".xml";
   BString pathDF("art\\"); pathDF +=mFilename.c_str();pathDF += "_df.ddx";
   BString pathNM("art\\"); pathNM +=mFilename.c_str();pathNM += "_nm.ddx";
   BString pathSP("art\\"); pathSP +=mFilename.c_str();pathSP += "_sp.ddx";
   BString pathOP("art\\"); pathOP +=mFilename.c_str();pathOP += "_op.ddx";
   paths.pushBack(pathXML);
   paths.pushBack(pathDF);
   paths.pushBack(pathNM);
   paths.pushBack(pathSP);
   paths.pushBack(pathOP);


   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, cFoliageSetReloadFileEvent, 0);

}
//============================================================================
// BTerrainFoliageSet::loadTextures
//============================================================================
void BTerrainFoliageSet::loadTextures()
{

   //DF
   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();
   BFixedString256 filename;
   filename = BFixedString256(cVarArg, "%s%s.ddx", mFilename.c_str(), "_df");
   pPacket->setFilename(filename);
   pPacket->setDirID(cDirArt);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainFoliageSet::setTexturesCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(0);  //ALBEDO
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);

   //NM
   pPacket = gAsyncFileManager.newRequestPacket();
   filename = BFixedString256(cVarArg, "%s%s.ddx", mFilename.c_str(), "_nm");
   pPacket->setFilename(filename);
   pPacket->setDirID(cDirArt);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainFoliageSet::setTexturesCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(1); //NORMAL
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);


   //SP
   pPacket = gAsyncFileManager.newRequestPacket();
   filename = BFixedString256(cVarArg, "%s%s.ddx", mFilename.c_str(), "_sp");
   pPacket->setFilename(filename);
   pPacket->setDirID(cDirArt);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainFoliageSet::setTexturesCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(2); //SPECULAR
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);

   //OP
   pPacket = gAsyncFileManager.newRequestPacket();
   filename = BFixedString256(cVarArg, "%s%s.ddx", mFilename.c_str(), "_op");
   pPacket->setFilename(filename);
   pPacket->setDirID(cDirArt);
   pPacket->setCallback(BEventDispatcher::BDataFunctor(this, &BTerrainFoliageSet::setTexturesCallback));
   pPacket->setCallbackThreadIndex(cThreadIndexRender);
   pPacket->setPrivateData0(3);  //OPACITY
   // pPacket->setPrivateData1(textureType | (mNextAsyncFileSetRequestID << 4));
   pPacket->setPriority(-3);
   gAsyncFileManager.submitRequest(pPacket);
}
//============================================================================
// BTerrainFoliageSet::setTexturesCallback
//============================================================================
void BTerrainFoliageSet::setTexturesCallback(void *pData)
{
   BAsyncFileManager::BRequestPacket* pPacket = static_cast<BAsyncFileManager::BRequestPacket*>(pData);

   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.resource("BTerrainFoliageSet::setTexturesCallback: File Not Found %s", pPacket->getFilename().c_str());

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }


 
   BD3DTextureLoader* mpTextureLoader = new BD3DTextureLoader();

   BD3DTextureLoader::BCreateParams textureLoadParams;
   textureLoadParams.mBigEndian = true;
   textureLoadParams.mTiled = true;
   textureLoadParams.mManager = "TerrainFoliage";
   textureLoadParams.mName = pPacket->getFilename();

   bool status;
   status = mpTextureLoader->createFromDDXFileInMemory(static_cast<const uchar*>(pPacket->getData()), pPacket->getDataLen(), textureLoadParams);

   if (!status)
   {
      gConsoleOutput.resource("BTerrainFoliageSet::setTexturesCallback: Error Reading %s", pPacket->getFilename().c_str());

      delete mpTextureLoader;
      mpTextureLoader = NULL;

      gAsyncFileManager.deleteRequestPacket(pPacket);
      return;
   }

   const uint textureIndex = pPacket->getPrivateData0();

   //sRGB space
   GPUTEXTURE_FETCH_CONSTANT& fc = mpTextureLoader->getD3DTexture().getTexture()->Format;
   fc.SignX = GPUSIGN_GAMMA;
   fc.SignY = GPUSIGN_GAMMA;
   fc.SignZ = GPUSIGN_GAMMA;

   switch (textureIndex)
   {
   case 0:
      if(mAlbedoTexture.getBaseTexture())
         mAlbedoTexture.release();
      mAlbedoTexture = mpTextureLoader->getD3DTexture();
      break;
   case 1:
      if(mNormalTexture.getBaseTexture())
         mNormalTexture.release();
      mNormalTexture = mpTextureLoader->getD3DTexture();
      break;
   case 2:
      if(mSpecularTexture.getBaseTexture())
         mSpecularTexture.release();
      mSpecularTexture = mpTextureLoader->getD3DTexture();
      break;
   case 3:
      if(mOpacityTexture.getBaseTexture())
         mOpacityTexture.release();
      mOpacityTexture = mpTextureLoader->getD3DTexture();
      break;
   default:
      break;
   };
  
   mpTextureLoader->releaseOwnership();

   delete mpTextureLoader;
   mpTextureLoader = NULL;

   gAsyncFileManager.deleteRequestPacket(pPacket);
}
//============================================================================
// BTerrainFoliageSet::loadPositionsXML
//============================================================================
void BTerrainFoliageSet::loadPositionsXML()
{
   BString pathXML(mFilename.c_str());
   pathXML += ".xml";

   BXMLReader reader;
   if (!reader.load(cDirArt, pathXML, XML_READER_LOAD_DISCARD_ON_CLOSE))
      return;//(false); 

   BXMLNode rootNode(reader.getRootNode());

   BASSERT(rootNode.getName() == "foliageset");
//xmlNode.getTextPtr(temp)
   DWORD numBlades=0;
   DWORD numVerts =0;

   rootNode.getAttribValueAsDWORD("typecount",numBlades);
   rootNode.getAttribValueAsDWORD("numVertsPerType",numVerts);
   rootNode.getAttribValueAsFloat("backsideShadowScalar",mBacksideShadowScalar);

   int totalVertCount = numVerts * numBlades;
   if(totalVertCount==0)
      return;

    BXMLNode setElementsNode;
    if(!rootNode.getChild("setElements",&setElementsNode))
       return;

   //load our sets into one texture
   if(mPositionsTexture)
      mPositionsTexture->Release();
   if(mVertNormalsTexture)
      mVertNormalsTexture->Release();


   BD3D::mpDev->CreateLineTexture(totalVertCount,1,0,D3DFMT_LIN_A32B32G32R32F,0,&mPositionsTexture,0);
   D3DLOCKED_RECT pLockedRect;
   mPositionsTexture->LockRect(0,&pLockedRect,0,0);
   D3DXVECTOR4 *pVerts = (D3DXVECTOR4*)pLockedRect.pBits;

   D3DLOCKED_RECT pLockedRect2;
   BD3D::mpDev->CreateLineTexture(totalVertCount,1,0,D3DFMT_LIN_A32B32G32R32F,0,&mVertNormalsTexture,0);
   mVertNormalsTexture->LockRect(0,&pLockedRect2,0,0);
   D3DXVECTOR4 *pNormals = (D3DXVECTOR4*)pLockedRect2.pBits;


   int currentVertCount=0;
   //load our XML file
   for (long i=0; i < setElementsNode.getNumberChildren(); i++)
   {
      BXMLNode node(setElementsNode.getChild(i));

      if (node.getName().compare("setElement") == 0)
      {
         for (long q=0; q < node.getNumberChildren(); q++)
         {
            BXMLNode enode(node.getChild(q));

            if (enode.getName().compare("elementVerts") == 0)
            {
               int numVerts = enode.getNumberChildren();
               if(currentVertCount + numVerts > totalVertCount)
                  return;

               for(int k=0;k<numVerts;k++)
               {
                  BXMLNode cnode(enode.getChild(k));
                  if (cnode.getName().compare("vert") == 0)
                  {
                     BVector pos;
                     BVector nrm;
                     BVector uv;
                     cnode.getAttribValueAsVector("pos",pos);
                     cnode.getAttribValueAsVector("norm",nrm);
                     cnode.getAttribValueAsVector("uv",uv);

                     pVerts[currentVertCount].x = pos.x;
                     pVerts[currentVertCount].y = pos.y;
                     pVerts[currentVertCount].z = pos.z;
                     pVerts[currentVertCount].w = uv.x;

                     pNormals[currentVertCount].x = nrm.x;
                     pNormals[currentVertCount].y = nrm.y;
                     pNormals[currentVertCount].z = nrm.z;
                     pNormals[currentVertCount].w = uv.y;

                     currentVertCount++;
                  }
               }
            }
         }
        
      }
   }

   mPositionsTexture->UnlockRect(0);
   mVertNormalsTexture->UnlockRect(0);
}
