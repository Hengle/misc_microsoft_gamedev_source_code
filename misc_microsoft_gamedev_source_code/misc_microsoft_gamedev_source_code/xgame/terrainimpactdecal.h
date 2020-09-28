//============================================================================
//
//  Terrainimpactdecal.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// xgamerender
#include "D3DTextureManager.h"
//==============================================================================
// class BTerrainImpactDecalHandle
//==============================================================================
class BTerrainImpactDecalHandle
{
public:
   BString  mImpactTextureName;
   float    mSizeX;
   float    mSizeZ;
   float    mTimeFullyOpaque;
   float    mFadeOutTime;
   bool     mOrientation;
};
//==============================================================================
// class BTerrainImpactDecal
//==============================================================================
class BTerrainImpactDecal
{
public:

   void destroy();

   BManagedTextureHandle mDiffuseTexHandle;
   BManagedTextureHandle mNormalTexHandle;
   BManagedTextureHandle mOpacityTexHandle;
   BManagedTextureHandle mSpecularTexHandle;

   BString mTexName;
};

//==============================================================================
// class BTerrainStaticDecalHandle
//==============================================================================
class BTerrainStaticDecalHandle
{
public:
   BString  mImpactTextureName;
   float    mSizeX;
   float    mSizeZ;
   
};
//==============================================================================
// class BTerrainStaticDecal
//==============================================================================
class BTerrainStaticDecal
{
public:

   void destroy();

   BManagedTextureHandle mTexHandle;

   BString mTexName;
};

static const int cImpactDecalCatagoryIndex = 77;
//==============================================================================
// class BTerrainImpactDecalManager
//==============================================================================
class BTerrainImpactDecalManager
{
public:
   BTerrainImpactDecalManager();
   ~BTerrainImpactDecalManager();

   void destroy();
   void destroyStaticDecals();
   
   uint loadImpactDecal(const char* pFilename);
   void createImpactDecal(BVector pos,const BTerrainImpactDecalHandle* mpImpactHandle,BVector forward=cZAxisVector);
   
   const BTerrainImpactDecal* getImpactDecal(uint index);
   const BTerrainImpactDecal* getImpactDecal(BString decalName);
   const int getImpactDecalIndex(BString decalName);


   uint loadStaticDecalTexture(const char* pFilename);
   const int getStaticDecalIndex(BString decalName);
   void createStaticBuildingDecal(BVector pos,const BTerrainStaticDecalHandle* mpStaticHandle,int Civ,BVector forward=cZAxisVector);

private:
   BDynamicArray<BTerrainImpactDecal> mDecals;
   BDynamicArray<BTerrainStaticDecal> mStaticDecals;

};

extern BTerrainImpactDecalManager gImpactDecalManager;