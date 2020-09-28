//============================================================================
//
//  Terrainimpactdecal.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================

// xgame
#include "common.h"
#include "terrainimpactdecal.h"
#include "gamedirectories.h"
#include "renderControl.h"

// xgamerender
#include "D3DTextureManager.h"
#include "decalManager.h"

// terrain
#include "TerrainTexturing.h"
#include "TerrainLitDecals.h"

BTerrainImpactDecalManager gImpactDecalManager;
//============================================================================
// BTerrainImpactDecal::destroy
//============================================================================
void BTerrainImpactDecal::destroy()
{
   
}

//============================================================================
// BTerrainStaticDecal::destroy
//============================================================================
void BTerrainStaticDecal::destroy()
{

}


//============================================================================
// BTerrainImpactDecalManager::BTerrainImpactDecalManager
//============================================================================
BTerrainImpactDecalManager::BTerrainImpactDecalManager()
{

}
//============================================================================
// BTerrainImpactDecalManager::~BTerrainImpactDecalManager
//============================================================================
BTerrainImpactDecalManager::~BTerrainImpactDecalManager()
{

}

//============================================================================
// BTerrainImpactDecalManager::destroy
//============================================================================
void BTerrainImpactDecalManager::destroy()
{
   for(uint i=0;i<mDecals.size();i++)
      mDecals[i].destroy();
   mDecals.clear();

  // gDecalManager.destroyDecalsWithCatagoryIndex(cImpactDecalCatagoryIndex);

   
}
//============================================================================
// BTerrainImpactDecalManager::destroy
//============================================================================
void BTerrainImpactDecalManager::destroyStaticDecals()
{
   for(uint i=0;i<mStaticDecals.size();i++)
      mStaticDecals[i].destroy();
   mStaticDecals.clear();

}



//============================================================================
// BTerrainImpactDecalSet::getImpactDecal
//============================================================================
const BTerrainImpactDecal* BTerrainImpactDecalManager::getImpactDecal(uint index)
{
   BDEBUG_ASSERT(index>=0 && index< mDecals.size());

   return &mDecals[index];
}
//============================================================================
// BTerrainImpactDecalSet::getImpactDecal
//============================================================================
const BTerrainImpactDecal* BTerrainImpactDecalManager::getImpactDecal(BString decalName)
{
   for(uint i=0;i<mDecals.size();i++)
   {
      if(mDecals[i].mTexName == decalName)
         return &mDecals[i];
   }

   return NULL;
}
//============================================================================
// BTerrainImpactDecalSet::getImpactDecal
//============================================================================
const int BTerrainImpactDecalManager::getImpactDecalIndex(BString decalName)
{
   for(uint i=0;i<mDecals.size();i++)
   {
      if(mDecals[i].mTexName == decalName)
         return i;
   }
   return -1;
}
//============================================================================
// BTerrainImpactDecalManager::loadImpactDecalTexture
//============================================================================
uint BTerrainImpactDecalManager::loadImpactDecal(const char* pFilename)
{
   int index = getImpactDecalIndex(pFilename);
   if(index!=-1)
      return index;

 
   
   BTerrainImpactDecal impactDecal;
   impactDecal.mTexName = pFilename;

   impactDecal.mDiffuseTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_df", pFilename), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainImpact, false, cDefaultTextureWhite, true, false, "BTerrainImpact");
   impactDecal.mNormalTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_nm", pFilename), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainImpact, false, cDefaultTextureNormal, true, false, "BTerrainImpact");
   impactDecal.mOpacityTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_op", pFilename), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainImpact, false, cDefaultTextureWhite, true, false, "BTerrainImpact");
   impactDecal.mSpecularTexHandle = gD3DTextureManager.getOrCreateHandle(BFixedString256(cVarArg, "%s_sp", pFilename), BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainImpact, false, cDefaultTextureBlack, true, false, "BTerrainImpact");

   mDecals.push_back(impactDecal);

   return mDecals.size()-1;
}

//============================================================================
// BTerrainImpactDecalManager::getImpactDecal
//============================================================================
void BTerrainImpactDecalManager::createImpactDecal(BVector pos,const BTerrainImpactDecalHandle* mpImpactHandle,BVector forward/*=cZAxisVector*/)
{
   if(!mpImpactHandle)
      return;


   BLitDecalAttribs pDecalAttribs;

   pDecalAttribs.setEnabled(true);
   pDecalAttribs.setIntensity(3.0f);
   pDecalAttribs.setConformToTerrain(true);    
   pDecalAttribs.setRenderOneFrame(false);
   pDecalAttribs.setBlendMode(BLitDecalAttribs::cBlendOver);

   //RANDOMIZE THESE
   BVector right = cXAxisVector;   
   if(!mpImpactHandle->mOrientation)
   {
      BVec3 dir = BVec3(getRandRangeFloat(cUnsyncedRand,-1,1),
                        0,
                        getRandRangeFloat(cUnsyncedRand,-1,1));
      dir.normalize();
      pDecalAttribs.setForward(dir);      //RANDOM ROTATION
   }
   else
   {
      pDecalAttribs.setForward(BVec3(forward.x,0,forward.z));      
   }

  
   pDecalAttribs.setCategoryIndex(cImpactDecalCatagoryIndex);

   
   pDecalAttribs.setSizeX(mpImpactHandle->mSizeX);
   pDecalAttribs.setSizeZ(mpImpactHandle->mSizeZ);
   pDecalAttribs.setYOffset(0.25f);      

   pDecalAttribs.setPos(BVec3(pos.x, pos.y, pos.z));

   //fade data
   pDecalAttribs.setFadeStartTime((float)gRenderControl.getSimGameTime());
   pDecalAttribs.setFadeTotalTime(mpImpactHandle->mFadeOutTime);
   pDecalAttribs.setFadeDirection(-1);
   pDecalAttribs.setHoldUntilFadeTotalTime(mpImpactHandle->mTimeFullyOpaque);
   pDecalAttribs.setFadeMode(BLitDecalAttribs::cFTDestroyWhenDone);
   

   const BTerrainImpactDecal* dcl = getImpactDecal(mpImpactHandle->mImpactTextureName);
   // rg [6/20/07] - dcl is still someetimes NULL!!
   if (!dcl)
      dcl = getImpactDecal(loadImpactDecal(mpImpactHandle->mImpactTextureName));

   if (dcl)
   {
      pDecalAttribs.setDiffuseTextureHandle(dcl->mDiffuseTexHandle);
      pDecalAttribs.setNormalTextureHandle(dcl->mNormalTexHandle);
      pDecalAttribs.setOpacityTextureHandle(dcl->mOpacityTexHandle);
      pDecalAttribs.setSpecularTextureHandle(dcl->mSpecularTexHandle);
   }
   else
      gConsoleOutput.error("BTerrainImpactDecalManager::createImpactDecal: BTerrainImpactDecal %s not preloaded properly!\n", mpImpactHandle->mImpactTextureName.getPtr());

   gLitDecalManager.createLitDecal(pDecalAttribs);

   //gLitDecalManager.holdUntilFadeDecal(dh,gRenderControl.getSimGameTime(),mpImpactHandle->mTimeFullyOpaque,mpImpactHandle->mFadeOutTime,-1,BDecalManager::cFTDestroyWhenDone);
}

//============================================================================
// BTerrainImpactDecalManager::loadStaticDecalTexture
//============================================================================
uint BTerrainImpactDecalManager::loadStaticDecalTexture(const char* pFilename)
{
   int index = getStaticDecalIndex(pFilename);
   if(index!=-1)
      return index;


   BTerrainStaticDecal staticDecal;
   staticDecal.mTexName = pFilename;
   staticDecal.mTexHandle = gD3DTextureManager.getOrCreateHandle(pFilename, BFILE_OPEN_NORMAL, BD3DTextureManager::cTerrainImpact, false, cDefaultTextureWhite, true, false, "BTerrainImpact");

   mStaticDecals.push_back(staticDecal);

   return mStaticDecals.size()-1;
}
//============================================================================
// BTerrainImpactDecalSet::getStaticDecalIndex
//============================================================================
const int BTerrainImpactDecalManager::getStaticDecalIndex(BString decalName)
{
   for(uint i=0;i<mStaticDecals.size();i++)
   {
      if(mStaticDecals[i].mTexName == decalName)
         return i;
   }
   return -1;
}
//============================================================================
// BTerrainImpactDecalManager::getStaticDecal
//============================================================================
void BTerrainImpactDecalManager::createStaticBuildingDecal(BVector pos,const BTerrainStaticDecalHandle* mpStaticHandle,int Civ,BVector forward/*=cZAxisVector*/)
{ 
   if(!mpStaticHandle)
      return;


   int alphaIdx = getStaticDecalIndex(mpStaticHandle->mImpactTextureName);
   if(alphaIdx==-1)
   {
      alphaIdx = loadStaticDecalTexture(mpStaticHandle->mImpactTextureName);

      if(alphaIdx==-1)
         return;
   }

   int terrainSplatTextureIndex= Civ==1?gTerrainTexturing.getBuildingSplatIndexCOVN():gTerrainTexturing.getBuildingSplatIndexUNSC();

   
   BVector zUp(-1,0,0);

   forward.y = 0;
   forward.normalize();

   float rotation =    zUp.angleBetweenVector(forward);

   //If z is negative, we have to shift the rotation so that it's in the proper visual space.
   if(forward.z < 0)
   {
      rotation = (Math::fPi - rotation) + Math::fPi;
   }


   //posX/Z must be normalized
   float xVal = pos.x/ gTerrain.getMax().x;
   float zVal = pos.z/ gTerrain.getMax().z;

   gTerrainTexturing.addSplatDecal(xVal, zVal, terrainSplatTextureIndex, rotation, mpStaticHandle->mSizeX, mpStaticHandle->mSizeZ, mStaticDecals[alphaIdx].mTexHandle);
}
