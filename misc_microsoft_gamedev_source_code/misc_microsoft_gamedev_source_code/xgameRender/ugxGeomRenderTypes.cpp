//==============================================================================
//
// File: ugxGeomRenderTypes.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#include "xgameRender.h"
#include "ugxGeomRenderTypes.h"

//==============================================================================
// Globals
//==============================================================================
IDirect3DDevice9* gpUGXD3DDev;

//==============================================================================
// gUGXGeomVisModeDesc
//==============================================================================
const char* gUGXGeomVisModeDesc[cVMNum] =
{
   "Disabled",
   
   "Albedo",
   "AO",
   "XForm",
   "Emissive XForm",
   "Self",
   
   "EnvMask",
   "Env",
   "Spec",
   "Opacity",
   
   "AmbientSum",
   "DiffuseSum",
   "SpecSum",
   "WorldNormal",
   
   "TangentNormal",
   "SpecColor",
   "SpecPower",
   "Highlight",
   "Modulate"
};

//==============================================================================
// BUGXGeomRenderCommonInstanceData::BUGXGeomRenderCommonInstanceData
//==============================================================================
BUGXGeomRenderCommonInstanceData::BUGXGeomRenderCommonInstanceData() : 
   mpVolumeCuller(NULL),
   mTintColor(0xFF000000),
   mpExtendedAttributes(NULL),
   mNumPixelLights(0),
   mPass(cUGXGeomPassMain),
   mLayerFlags(0),
   mTileFlags(0),
   mGlobalLighting(false),
   mLocalLighting(false),
   mDirLightShadows(false),
   mLocalLightShadows(false),
   mLocalReflection(false),
   mHighlightIntensity(1.0f),
   mSetCommandBufferRunPredication(false)
{
}

//==============================================================================
// BUGXGeomRenderCommonInstanceData::clear
//==============================================================================
void BUGXGeomRenderCommonInstanceData::clear(void)
{
   mpVolumeCuller = NULL;
   mTintColor = 0xFF000000;
   mpExtendedAttributes = NULL;
   mNumPixelLights = 0;
   mPass = cUGXGeomPassMain;
   mLayerFlags = cUGXGeomLayerOpaque;
   mTileFlags = 0;
   mGlobalLighting = false;
   mLocalLighting = false;
   mDirLightShadows = 0;
   mLocalLightShadows = false;
   mLocalReflection = false;
   mHighlightIntensity = 1.0f;
   mSetCommandBufferRunPredication = true;
   mSampleBlackmap = false;
}







