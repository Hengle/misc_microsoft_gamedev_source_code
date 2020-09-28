//============================================================================
//
//  TerrainTexturingCache.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
//terrain
#include "TerrainPCH.h"
#include "TerrainTexturing.h"
#include "TerrainTexturingCache.h"
#include "Terrain.h"
#include "TerrainRender.h"
#include "TerrainMetric.h"

//xcore
#include "reloadManager.h"
#include "consoleOutput.h"
#include "math\vmxIntersection.h"
#include "mathutil.h"
#include "file\win32file.h"

//xgameRender
#include "tiledAA.h"
#include "primDraw2D.h"
#include "render.h"

// shaders
#include "defConstRegs.inc"
#include "..\shaders\terrain\gpuTerrainshaderRegs.inc"
#if TERRAIN_SHADER_COMPOS_REGS_VER != 101
#error Please update gpuTerrainShaderRegs.inc
#endif

#pragma warning(disable:4702) // unreachable code warning


//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void BTerrainTexturing::composeCompositeTexture(BTerrainCachedCompositeTexture *input,BTerrainTextureLayerContainer &layerInput)
{
   BASSERT(input);
   
   compositeTexturesGPU(layerInput,input);
}
//-------------------------------------------------------------------------
inline void BTerrainTexturing::composeSplatLayers(int channelIndex,int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output)
{
   SCOPEDSAMPLE(BTerrainTexturing_composeSplatLayers);

   if(mActiveTextures[channelIndex].mD3DTextureArrayHandle.getBaseTexture())             mShaderTargetSamplerHandle = mActiveTextures[channelIndex].mD3DTextureArrayHandle.getBaseTexture();
   else                                                                                   mShaderTargetSamplerHandle = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);


   if(layerInput.getNumSplatLayers()>1)
   {
      mShaderAlphaSamplerHandle = layerInput.mSplatData.mAlphaLayerTexture;

      mCurrTechnique.beginPass( channelIndex );   //will choose small cache albedo creation
      mCurrTechnique.commitU();

      BD3D::mpDev->SetVertexShaderConstantF(LAYER_DATA,(float*)layerInput.mSplatData.mLayerData,layerInput.mSplatData.getNumLayers());

      if ((gTerrainChannelParams[channelIndex].mIsHDR) && (gTerrainTexturing.mActiveTextures[channelIndex].mHDRScales))
      {
         int numAlignedActiveTex = gTerrainTexturing.getNumActiveTextures();//((gTerrainTexturing.getNumActiveTextures()-1)>>2)+1;
         BD3D::mpDev->SetPixelShaderConstantF(HDR_DATA_CURRENT,(float*)gTerrainTexturing.mActiveTextures[channelIndex].mHDRScales,numAlignedActiveTex);
      }


      BD3D::mpDev->DrawVertices(D3DPT_RECTLIST,0,mNumVertsInQuad * layerInput.mSplatData.mNumLayers);//mNumVertsInQuad*cNumBlendsDoneAtOnce);//


      mCurrTechnique.endPass();
   }
   else 
   {
      mShaderAlphaSamplerHandle = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);
      mCurrTechnique.beginPass( channelIndex );   //will choose small cache albedo creation
      mCurrTechnique.commitU();

      D3DXVECTOR4 vecDat(0,layerInput.mSplatData.mLayerData[0].y,layerInput.mSplatData.mLayerData[0].z,layerInput.mSplatData.mLayerData[0].w);
      BD3D::mpDev->SetVertexShaderConstantF(LAYER_DATA,(float*)vecDat,1);

      if((gTerrainChannelParams[channelIndex].mIsHDR) && (gTerrainTexturing.mActiveTextures[channelIndex].mHDRScales))
      {
         int hdrIndex = Math::FloatToIntTrunc(layerInput.mSplatData.mLayerData[0].x);

         BD3D::mpDev->SetPixelShaderConstantF(HDR_DATA_CURRENT,gTerrainTexturing.mActiveTextures[channelIndex].mHDRScales[hdrIndex],1);
      }


      BD3D::mpDev->DrawVertices(D3DPT_RECTLIST,0,mNumVertsInQuad * 1);//mNumVertsInQuad*cNumBlendsDoneAtOnce);//


      mCurrTechnique.endPass();
   }
}
//-------------------------------------------------------------------------
inline void BTerrainTexturing::composeDecalLayers(int channelIndex,int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output)
{
   SCOPEDSAMPLE(BTerrainTexturing_composeDecalLayers);

   if(layerInput.getNumDecalLayers()>0)
   {
      mCurrTechnique.beginPass( channelIndex + cTextureTypeMax);

      mShaderAlphaSamplerHandle = layerInput.mDecalData.mAlphaLayerTexture;

      int numDecals = layerInput.getNumDecalLayers();
      float rcpNumAlignedDecals = layerInput.mDecalData.mNumAlignedLayers==1?0:1.0f/layerInput.mDecalData.mNumAlignedLayers;
      //CLM WE DON'T NEED TO PROCESS ALL THE EMPTY LAYERS
      //WE SHOULD WRITE THIS TO FILE!
      //for now, just do one draw call per each decal
      for(int i=0;i<numDecals;i++)
      {

         BTerrainActiveDecalInstanceInfo dcli = mActiveDecalInstanceInfo[layerInput.mDecalData.mActiveDecalIndexes[i]];
         BTerrainActiveDecalHolder        dcl = mActiveDecals[dcli.mActiveDecalIndex];

         if(!(dcl.mTextureTypesUsed & (1<<channelIndex)))
            continue;

         mShaderTargetDecalSamplerHandle = dcl.mTextures[channelIndex].getTexture();
         mShaderTargetDecalOpactiySamplerHandle = dcl.mOpactityTexture.getTexture();

         mCurrTechnique.commitU();


         D3DXVECTOR4 decalDat;
         decalDat.x = dcli.mRotation;
         decalDat.w = gTerrainChannelParams[channelIndex].mIsHDR?dcl.mSelfHDRScale:1;

         //compute our U and V offset
         BTerrainQuadNodeDesc desc = output->mpOwnerQuadNode->getDesc();
         float vertsToHighResPixelSpaceRatio = (float)(cUniqueTextureWidth / BTerrainQuadNode::getMaxNodeWidth());
         float oV = (dcli.mTileCenterX - (desc.mMinXVert * vertsToHighResPixelSpaceRatio));
         decalDat.y = oV? oV / cUniqueTextureWidth : 0;
         oV = (dcli.mTileCenterY - (desc.mMinZVert * vertsToHighResPixelSpaceRatio));
         decalDat.z = oV? oV  / cUniqueTextureHeight : 0;


         BD3D::mpDev->SetPixelShaderConstantF(DECAL_PROPERTIES,decalDat,1);

         D3DXVECTOR4 uvHDRDat;
         uvHDRDat.x = dcli.mUScale;
         uvHDRDat.y = dcli.mVScale;
         uvHDRDat.z = (float)(rcpNumAlignedDecals * (i / 4));
         uvHDRDat.w = (float) (i % 4);

         BD3D::mpDev->SetPixelShaderConstantF(DECAL_UVSCALE,uvHDRDat,1);


         BD3D::mpDev->DrawVertices(D3DPT_RECTLIST,0,mNumVertsInQuad * 1);

      }

      mCurrTechnique.endPass();

   }
}
//-------------------------------------------------------------------------
inline void BTerrainTexturing::composeTextureSetDecalLayers(int channelIndex,int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output)
{
   SCOPEDSAMPLE(BTerrainTexturing_composeTextureSetDecalLayers);

   int numDecals = layerInput.getNumTextureSetDecalLayers();

   if(numDecals>0)
   {
      mCurrTechnique.beginPass( channelIndex + (cTextureTypeMax*2));

      if(mActiveTextures[channelIndex].mD3DTextureArrayHandle.getBaseTexture())             
         mShaderTargetSamplerHandle = mActiveTextures[channelIndex].mD3DTextureArrayHandle.getBaseTexture();
      else                                                                                 
         mShaderTargetSamplerHandle = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);


      mShaderTargetDecalOpactiySamplerHandle = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);

      
      

      //for now, just do one draw call per each decal
      for(int i=0;i<numDecals;i++)
      {

         int splatTexIndex = layerInput.mTextureSetDecalData.mTextureSetDecalInstances[i].mActiveTextureIndex;
         if(splatTexIndex >=gTerrainTexturing.getNumActiveTextures())
            splatTexIndex=0;

         BTerrainActiveTextureSetDecalInstanceInfo dcli = layerInput.mTextureSetDecalData.mTextureSetDecalInstances[i];

         BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(layerInput.mTextureSetDecalData.mTextureSetDecalInstances[i].mpExternalAlphaTextureToUse);

         if(pTexture==NULL)
            mShaderTargetDecalOpactiySamplerHandle = gD3DTextureManager.getDefaultD3DTexture(cDefaultTextureWhite);
         else
            mShaderTargetDecalOpactiySamplerHandle = pTexture->getD3DTexture().getTexture();


         mCurrTechnique.commitU();

         //grab our layer data for this splat texture
         D3DXVECTOR4 layDat(0,(float)mActiveTexInfo[splatTexIndex].mUScale,(float)mActiveTexInfo[splatTexIndex].mVScale,0);
         BD3D::mpDev->SetPixelShaderConstantF(LAYER_DATA,(float*)&layDat,1);


         D3DXVECTOR4 decalDat;
         decalDat.x = dcli.mRotation;
         decalDat.w = 1;//gTerrainChannelParams[channelIndex].mIsHDR?dcl.mSelfHDRScale:1;

         //compute our U and V offset
         BTerrainQuadNodeDesc desc = output->mpOwnerQuadNode->getDesc();
         float vertsToHighResPixelSpaceRatio = (float)(cUniqueTextureWidth / BTerrainQuadNode::getMaxNodeWidth());
         float oV = (dcli.mTileCenterX - (desc.mMinXVert * vertsToHighResPixelSpaceRatio));
         decalDat.y = oV? oV / cUniqueTextureWidth : 0;
         oV = (dcli.mTileCenterY - (desc.mMinZVert * vertsToHighResPixelSpaceRatio));
         decalDat.z = oV? oV  / cUniqueTextureHeight : 0;

         BD3D::mpDev->SetPixelShaderConstantF(DECAL_PROPERTIES,decalDat,1);


         D3DXVECTOR4 uvHDRDat;
         uvHDRDat.x = dcli.mUScale;
         uvHDRDat.y = dcli.mVScale;
         uvHDRDat.z = splatTexIndex==0?0:(float)(splatTexIndex / ((float)gTerrainTexturing.getNumActiveTextures()-1));
         uvHDRDat.w = (float) (i % 4);

         BD3D::mpDev->SetPixelShaderConstantF(DECAL_UVSCALE,uvHDRDat,1);


         BD3D::mpDev->DrawVertices(D3DPT_RECTLIST,0,mNumVertsInQuad * 1);

      }

      mCurrTechnique.endPass();

   }
}
//-------------------------------------------------------------------------
inline void BTerrainTexturing::doComposeTextures(int channelIndex,int mipIndex,int resolveLevel,int levelWidth,BTerrainCompositeSurface *surf,bool doCompression,BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output)
{
   SCOPEDSAMPLE(BTerrainTexturing_doComposeTextures);

   D3DVIEWPORT9 viewport;
   viewport.X=0;
   viewport.Y=0;
   viewport.Width=output->mWidth>>mipIndex;
   viewport.Height=output->mHeight>>mipIndex;
   viewport.MinZ=0;
   viewport.MaxZ=1;
   BD3D::mpDev->SetViewport(&viewport);

   composeSplatLayers(channelIndex,mipIndex,resolveLevel,levelWidth,surf,doCompression,layerInput, output);
   composeDecalLayers(channelIndex,mipIndex,resolveLevel,levelWidth,surf,doCompression,layerInput, output);
   composeTextureSetDecalLayers(channelIndex,mipIndex,resolveLevel,levelWidth,surf,doCompression,layerInput, output);




   //DO OUR RESOLVES
   if(gTerrainChannelParams[channelIndex].mIsHDR)
   {
      BTerrainMetrics::addResolve(levelWidth * levelWidth);
      D3DRECT pSourceRect;
      pSourceRect.x1=0;
      pSourceRect.y1=0;
      pSourceRect.x2 = levelWidth;
      pSourceRect.y2 = levelWidth;

      BD3D::mpDev->Resolve( D3DRESOLVE_RENDERTARGET0, &pSourceRect, 
         doCompression?surf->mTempResolveTarget:output->mTextures[channelIndex], 
         NULL,//&pointOffsets[0], 
         doCompression?resolveLevel:mipIndex,
         0, NULL, 0, 0, NULL );
   }
   else
   {
      BD3D::mpDev->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, 
         doCompression?surf->mTempResolveTarget:output->mTextures[channelIndex], 
         NULL, 
         doCompression?resolveLevel:mipIndex,
         0, NULL, 0, 0, NULL );
      BTerrainMetrics::addResolve(levelWidth * levelWidth);
   }
}
//-------------------------------------------------------------------------
void BTerrainTexturing::compositeTexturesGPU(BTerrainTextureLayerContainer &layerInput, BTerrainCachedCompositeTexture *output)
{   
   ASSERT_THREAD(cThreadIndexRender);
   SCOPEDSAMPLE(BTerrainTexturing_compositeTexturesGPU);

   bool isSmallCache = output->mWidth <=cTexSize360Cutoff;

   int numLayers = layerInput.mSplatData.getNumLayers();
   //  int numMips = isSmallCache?cNumSmallTextureNumMips:cNumMainTextureNumMips;

   int rcpNumFileMips = (int)(mActiveTextures[0].mD3DTextureArrayHandle.getBaseTexture() ? (1.0f /  mActiveTextures[0].mD3DTextureArrayHandle.getBaseTexture()->GetLevelCount()) : 1.0f);

   //determine our level
   int level=0;
   for(int i=0;i<cNumMainTextureNumMips;i++)
   {
      if(output->mWidth == cUniqueTextureWidth >> i)
      {
         level=i;
         break;
      }
   }


   mShaderNumLayers = (1.0f / (float)numLayers);
   mShaderRcpNumAlignedLayers = (1.0f/ (float)layerInput.mSplatData.mNumAlignedLayers);

   int numIndexes=(layerInput.mSplatData.mNumLayers >> 2) + 1;
   numIndexes;


   //compute our data
   for(int i=0;i<cNumCachedChannels;i++)
   {      

      BOOL doCompression=false;
      if(isSmallCache)
      {
         if(!gTerrainChannelParams[i].mSmallCacheFormat)
            continue;
         doCompression = XGIsCompressedFormat(gTerrainChannelParams[i].mSmallCacheFormat);
      }
      else
      {
         doCompression = XGIsCompressedFormat(gTerrainChannelParams[i].mMainCacheFormat);
      }


      //early out for specality channels
      if(i == cTextureTypeSelf && !layerInput.mSelfPassNeeded ||
         i == cTextureTypeEnvMask && !layerInput.mEnvMaskPassNeeded)
         continue;

      if(mActiveTextures[i].mD3DTextureArrayHandle.getBaseTexture()==NULL)
         continue;

      BASSERT(output->mTextures[i]);

      //   for(int k=0;k<numMips;k++)
      int k=0;
      {
         //compute our level into the temp texture 
         level = level >> k;

         int levelWidth = output->mWidth>>k;

         //SET SHIT UP AGAIN
         BTerrainCompositeSurface *surf = getRenderSurface(gTerrainChannelParams[i].mIsHDR);
         BDEBUG_ASSERT(surf->mRenderSurface && surf->mTempResolveTarget);

         BD3D::mpDev->SetRenderTarget( 0, surf->mRenderSurface );
         BD3D::mpDev->SetDepthStencilSurface(NULL);

         //BD3D::mpDev->SetShaderGPRAllocation(0, 32, 96);
         BD3D::mpDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
         BD3D::mpDev->SetVertexDeclaration( m_pQuadVertexDecl );
         BD3D::mpDev->SetStreamSource(0,m_pQuadVertexBuffer,0,sizeof(GPU_QUAD_VERTEX));
         BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, false);
         BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, false);
         BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE,true);
         BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
         BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

         mShaderExplicitTargetMipLevel=level * rcpNumFileMips;

         mCurrTechnique.begin(0 );

         doComposeTextures(i,k,level,levelWidth,surf,doCompression!=0,layerInput,output);

         mCurrTechnique.end();


         if(doCompression)
         {
            //now compress our resolved data to the cache
            BGPUDXTPack::getInstance().pack(surf->mTempResolveTarget,output->mTextures[i],level,k,gTerrainChannelParams[i].mIsHDR);               
            mGPUPackCount++;
            mGPUPackPixels += levelWidth * levelWidth;

            BGPUDXTPack::getInstance().pack(surf->mTempResolveTarget,output->mTextures[i],level,k+1,gTerrainChannelParams[i].mIsHDR);               
            mGPUPackCount++;
            mGPUPackPixels += (levelWidth>>1) * (levelWidth>>1);
         } 
      } 

   }

}


