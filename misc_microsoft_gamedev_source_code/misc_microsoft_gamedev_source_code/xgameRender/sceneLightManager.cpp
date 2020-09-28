//============================================================================
//
// File: sceneLightManager.cpp
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "sceneLightManager.h"
#include "renderDraw.h"
#include "volumeIntersection.h"
#include "effectIntrinsicManager.h"
#include "grannyInstanceRenderer.h"
#include "math\VMXIntersection.h"
#include "effectIntrinsicManager.h"
#include "lightEffectManager.h"
#include "debugPrimitives.h"
#include "primDraw2D.h"
#include "inputsystem.h"
#include "keyboard.h"
#include "reloadManager.h"

#include "tonemapManager.h"
#include "D3DTextureManager.h"
#include "particleSystemManager.h"

//terrain
#include "TerrainTexturing.h"
#include "TerrainRender.h"

//============================================================================
// Globals
//============================================================================
BSceneLightManager gSimSceneLightManager;
BSceneLightManager gRenderSceneLightManager;

//============================================================================
// BDirLightParams::clear
//============================================================================
void BDirLightParams::clear(void)
{
   mDir = XMVectorSet(-.5f, -1.0f, -.5f, 0.0f);
   mDir = XMVector4Normalize(mDir);
   
   mColor = XMVectorReplicate(1.0f);
   mShadowDarkness = 0.0f;
   mEnabled = true;
   mShadows = true;
}

//============================================================================
// BSHLightParams::clear
//============================================================================
void BSHLightParams::clear(void)
{
   mSHCoeffs[0].clear();
   mSHCoeffs[1].clear();
   mSHCoeffs[2].clear();
}

//============================================================================
// BSHLightParams::set
//============================================================================
void BSHLightParams::set(const BSpectralSHCoefficients& shCoeffs)
{
   mSHCoeffs[0] = shCoeffs[0];
   mSHCoeffs[1] = shCoeffs[1];
   mSHCoeffs[2] = shCoeffs[2];
}

//============================================================================
// BSHLightParams::addHemiLight
//============================================================================
void BSHLightParams::addHemiLight(const BHemiLightParams& h)
{
   BVecN<9> rCoeffs;
   BVecN<9> gCoeffs;
   BVecN<9> bCoeffs;
   
   D3DXSHEvalHemisphereLight(3, 
      reinterpret_cast<const D3DXVECTOR3*>(&h.mAxis), 
      *reinterpret_cast<const D3DXCOLOR*>(&h.mTopColor), 
      *reinterpret_cast<const D3DXCOLOR*>(&h.mBottomColor), 
      rCoeffs.getPtr(), gCoeffs.getPtr(), bCoeffs.getPtr());

   mSHCoeffs[0] += rCoeffs;
   mSHCoeffs[1] += gCoeffs;
   mSHCoeffs[2] += bCoeffs;
}

//============================================================================
// BSHLightParams::scale
//============================================================================
void BSHLightParams::scale(float s)
{
   mSHCoeffs[0] *= s;
   mSHCoeffs[1] *= s;
   mSHCoeffs[2] *= s;
}

//============================================================================
// BSHLightParams::load
//============================================================================
bool BSHLightParams::load(long dirID, const char* pFilename)
{
   BXMLReader reader;

   // rg [3/12/07] - For now, don't use XMB for FLS files because the game writes them. Ugh.
#ifdef BUILD_FINAL
   const bool useBinary = true;
#else
   const bool useBinary = false;
#endif      

   if (!reader.load(dirID, pFilename, useBinary ? 0 : XML_READER_IGNORE_BINARY))
   {
      clear();
      return false;
   }
      
   BXMLNode root(reader.getRootNode());
   
   return load(root);
}

//============================================================================
// BSHLightParams::load
//============================================================================
bool BSHLightParams::load(BXMLNode root)
{
   clear();

   const int nodeCount = root.getNumberChildren();
   for(int i = 0; i < nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString nodeName(node.getName()); 

      BString nodeVal;
      node.getText(nodeVal);

      int c = -1;
      if (nodeName == "R") 
         c = 0;
      else if (nodeName == "G") 
         c = 1;
      else if (nodeName == "B")
         c = 2;

      if (c != -1)
      {
         if(sscanf_s(nodeVal.getPtr(), "%f, %f, %f, %f, %f, %f, %f, %f, %f", &mSHCoeffs[c][0], &mSHCoeffs[c][1], &mSHCoeffs[c][2], &mSHCoeffs[c][3], &mSHCoeffs[c][4], &mSHCoeffs[c][5], &mSHCoeffs[c][6], &mSHCoeffs[c][7], &mSHCoeffs[c][8]) != 9)
         {
            clear();
            return false;
         }
      }
   }

   return true;
}

//============================================================================
// BSHLightParams::save
//============================================================================
bool BSHLightParams::save(long dirID, const char* pFilename) const
{
   BFile file;
   if (!file.openWriteable(dirID, pFilename, BFILE_OPEN_ENABLE_BUFFERING))
      return false;
      
   file.fprintf("<SHLightParams>\n");      
   for (uint i = 0; i < 3; i++)
   {
      int c = 0;
      switch (i)
      {
         case 0: c = 'R'; break;
         case 1: c = 'G'; break;
         case 2: c = 'B'; break;
      }
      
      file.fprintf("  <%c>%f, %f, %f, %f, %f, %f, %f, %f, %f</%c>\n", 
         c,
         mSHCoeffs[i][0], mSHCoeffs[i][1], mSHCoeffs[i][2], mSHCoeffs[i][3], 
         mSHCoeffs[i][4], mSHCoeffs[i][5], mSHCoeffs[i][6], mSHCoeffs[i][7], 
         mSHCoeffs[i][8], 
         c);
   }         
   
   file.fprintf("</SHLightParams>\n");      
   
   return file.close();
}

//============================================================================
// shIndex
//============================================================================
static int shIndex(int l, int m)
{
   //debugRangeCheckIncl(l, 2);
   //debugRangeCheckIncl(m, -l, l);
   //return debugRangeCheck(l * (l + 1) + m + 1 - 1, 9);
   return l * (l + 1) + m + 1 - 1;
}

//============================================================================
// BSHLightShaderConstants::clear
//============================================================================
void BSHLightShaderConstants::clear(void)
{
   mAr = XMVectorZero();
   mAg = XMVectorZero();
   mAb = XMVectorZero();
   mBr = XMVectorZero();
   mBg = XMVectorZero();
   mBb = XMVectorZero();
   mC = XMVectorZero();
}

//============================================================================
// BSHLightShaderConstants::set
//============================================================================
void BSHLightShaderConstants::set(const BSHLightParams& p)
{
   const float n0 = 1.0f/(2.0f*sqrt(Math::fPi));
   const float n1 = sqrt(3.0f)/(2.0f*sqrt(Math::fPi));
   const float n2 = sqrt(15.0f)/(2.0f*sqrt(Math::fPi));
//   const float n3 = sqrt(5.0f)/(4.0f*sqrt(Math::fPi));
   const float n4 = sqrt(5.0f)/(4.0f*sqrt(Math::fPi));

//   const float h0 = 1.0f; // unused, but present for clarity (actually Pi/Pi)
   const float h1 = 2.0f/3.0f;
   const float h2 = 1.0f/4.0f;

   // I'm multiplying by Pi here because in our renderer we don't multiply the diffuse contribution by 1/Pi, we scale the specular contribution by Pi instead.
   const float c0 = n0*Math::fPi;
   const float c1 = h1*n1*Math::fPi;
   const float c2 = h2*n2*Math::fPi;
   const float c3 = h2*n4*Math::fPi;
   const float c4 = c2*.5f*Math::fPi;

   mAr = XMVectorSet(
      -c1*p.mSHCoeffs[0][shIndex(1,1)],
      -c1*p.mSHCoeffs[0][shIndex(1,-1)],
      c1*p.mSHCoeffs[0][shIndex(1,0)],
      c0*p.mSHCoeffs[0][shIndex(0,0)] - c3*p.mSHCoeffs[0][shIndex(2,0)] );

   mAg = XMVectorSet(
      -c1*p.mSHCoeffs[1][shIndex(1,1)],
      -c1*p.mSHCoeffs[1][shIndex(1,-1)],
      c1*p.mSHCoeffs[1][shIndex(1,0)],
      c0*p.mSHCoeffs[1][shIndex(0,0)] - c3*p.mSHCoeffs[1][shIndex(2,0)] );

   mAb = XMVectorSet(
      -c1*p.mSHCoeffs[2][shIndex(1,1)],
      -c1*p.mSHCoeffs[2][shIndex(1,-1)],
      c1*p.mSHCoeffs[2][shIndex(1,0)],
      c0*p.mSHCoeffs[2][shIndex(0,0)] - c3*p.mSHCoeffs[2][shIndex(2,0)] );

   mBr = XMVectorSet(
      c2*p.mSHCoeffs[0][shIndex(2,-2)],
      -c2*p.mSHCoeffs[0][shIndex(2,-1)],
      3.0f*c3*p.mSHCoeffs[0][shIndex(2,0)],
      -c2*p.mSHCoeffs[0][shIndex(2,1)] ); 

   mBg = XMVectorSet(
      c2*p.mSHCoeffs[1][shIndex(2,-2)],
      -c2*p.mSHCoeffs[1][shIndex(2,-1)],
      3.0f*c3*p.mSHCoeffs[1][shIndex(2,0)],
      -c2*p.mSHCoeffs[1][shIndex(2,1)] );          

   mBb = XMVectorSet(
      c2*p.mSHCoeffs[2][shIndex(2,-2)],
      -c2*p.mSHCoeffs[2][shIndex(2,-1)],
      3.0f*c3*p.mSHCoeffs[2][shIndex(2,0)],
      -c2*p.mSHCoeffs[2][shIndex(2,1)] );

   mC = XMVectorSet(
      c4*p.mSHCoeffs[0][shIndex(2,2)],
      c4*p.mSHCoeffs[1][shIndex(2,2)],
      c4*p.mSHCoeffs[2][shIndex(2,2)],
      0.0f);
}

//============================================================================
// BBaseLocalLightParams::clear
//============================================================================
void BBaseLocalLightParams::clear(void)
{
   mPosRadius = XMVectorZero();
  (XMVECTOR&)mAtSpotOuter = XMVectorZero();
}

//============================================================================
// BBaseLocalLightParams::enforceLimits
//============================================================================
void BBaseLocalLightParams::enforceLimits(void)
{
   if (getRadius() < .00125f)
      setRadius(.00125f);
   else if (getRadius() > cMaxLocalLightRadius)
      setRadius(cMaxLocalLightRadius);
      
   float spotInner = getSpotInner();
   float spotOuter = getSpotOuter();
   
   spotInner = Math::Clamp(spotInner, 0.0f, Math::fDegToRad(160.0f));
   spotOuter = Math::Clamp(spotOuter, 0.0f, Math::fDegToRad(160.0f));

   if ((spotInner != 0.0f) || (spotOuter != 0.0f))
   {
      spotOuter = Math::Max(spotOuter, Math::fDegToRad(.025f));
      
      if (spotInner > spotOuter - Math::fDegToRad(.0125f))
         spotInner = spotOuter - Math::fDegToRad(.0125f);

       setSpotInner(spotInner);
       setSpotOuter(spotOuter);
   }      
}  

//============================================================================
// BLocalLightParams::BLocalLightParams
//============================================================================
BLocalLightParams::BLocalLightParams()
{
   clear();
}

//============================================================================
// BLocalLightParams::clear
//============================================================================
void BLocalLightParams::clear(void)
{
   XMStoreHalf4(&mColor, XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));

   mHandle = cInvalidLocalLightHandle;   
   
   mLightID = cInvalidLocalLightID;
   
   setShadowUVBoundsIndex(0);
   
   setFarAttenStart(.5f);
   setDecayDist(300.0f);
   
   mShadowDarkness = 0;
   setInvalidShadowIndex();
         
   setShadowMatrixCol(0, XMVectorZero());
   setShadowMatrixCol(1, XMVectorZero());
   setShadowMatrixCol(2, XMVectorZero());
   
   setPriority(0);
   
   setSpecular(false);
   setFogged(false);
   setFoggedShadows(false);
                     
   mType = cInvalidLightType;
   
   mLightBuffered = false;
}

//============================================================================
// BLocalLightParams::enforceLimits
//============================================================================
void BLocalLightParams::enforceLimits(void)
{
   float farAttenStart = getFarAttenStart();

   if (farAttenStart > .999f)
      setFarAttenStart(.999f);
   else if (farAttenStart < 0.0f)
      setFarAttenStart(0.0f);

   float decayDist = getDecayDist();
   if (decayDist < 0.01f)
      setDecayDist(0.01f);
}

//============================================================================
// BHemiLightParams::clear
//============================================================================
void BHemiLightParams::clear(void)
{
   mAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
   mTopColor = XMVectorSet(.25f, .25f, .25f, 0.0f);
   mBottomColor = XMVectorSet(.075f, .075f, .075f, 0.0f);
}

//============================================================================
// BSceneLightManager::BSceneLightManager
//============================================================================
BSceneLightManager::BSceneLightManager() :
   mInitialized(false),
   mLightsLocked(false),
   mNextLightID(1),
   mCurLightCat(cLCTerrain),
   mGlobalEnvMap(cInvalidManagedTextureHandle),
   mSHFillLightIntensity(1.0f),
   mBackgroundColor(.25*106.0f/255.0f, .25*107.0f/255.0f, .25*135.0f/255.0f),
   mBackgroundIntensity(1.0f),
   mFogEnabled(true)
{
#ifndef BUILD_FINAL  
   mDebugDisplay = false;
#endif  
}

//============================================================================
// BSceneLightManager::~BSceneLightManager
//============================================================================
BSceneLightManager::~BSceneLightManager()
{
}

//============================================================================
// BSceneLightManager::init
//============================================================================
void BSceneLightManager::init(void)
{
   if (mInitialized)
      return;
      
   commandListenerInit();      
      
   mLightGrid.init(cBroadPhaseMaxWorldCoord, cBroadPhaseCellDim);
         
   mLightsLocked = false;
   
   mNextLightID = 1;
   mCurLightCat = cLCTerrain;
   mGlobalEnvMap = cInvalidManagedTextureHandle;
   mBackgroundColor.set(.25*106.0f/255.0f, .25*107.0f/255.0f, .25*135.0f/255.0f);
   mBackgroundIntensity = 1.0f;
   mSHFillLightIntensity = 1.0f;

   clear();
   
   mInitialized = true;

   clearLightSets();
}

//============================================================================
// BSceneLightManager::deinit
//============================================================================
void BSceneLightManager::deinit(void)
{
   if (!mInitialized)
      return;
      
   commandListenerDeinit();            
      
   mInitialized = false;

   mLightGrid.deinit();
   mBaseLocalLightParams.clear();
   mLocalLightParams.clear();
   mLocalLightEnabled.clear();
   mLocalLightEnabledFrame.clear();
   mActiveLightShadows.clear();
   mActiveLightIndices.clear();
   mFreeLightHandles.clear();

   clearLightSets();
}

//============================================================================
// BSceneLightManager::clear
//============================================================================
void BSceneLightManager::clear(void)
{
   for (uint i = 0; i < cNumLightCategories; i++)
   {
      mDirLights[i].clear();
      mSHFillLights[i].clear();
      mHemiFillLights[i].clear();
   }
   
   mBaseLocalLightParams.reserve(cMaxExpectedLocalLights);
   mBaseLocalLightParams.resize(0);

   mLocalLightParams.reserve(cMaxExpectedLocalLights);
   mLocalLightParams.resize(0);

   mLocalLightEnabled.resize(cMaxExpectedLocalLights, 1);
   mLocalLightEnabled.setAll(0);
   
   mLocalLightEnabledFrame.resize(cMaxExpectedLocalLights, 1);
   mLocalLightEnabledFrame.setAll((uint)cInvalidLocalLightEnabledFrame);
   
   mActiveLightShadows.resize(cMaxExpectedLocalLights, 1);
   mActiveLightShadows.setAll(0);
               
   mActiveLightIndices.resize(cMaxExpectedLocalLights);
   mActiveLightIndices.setAll(USHRT_MAX);
   
   mFreeLightHandles.resize(cMaxExpectedLocalLights);
   for (uint i = 0; i < cMaxExpectedLocalLights; i++)
      mFreeLightHandles[i] = (ushort)(cMaxExpectedLocalLights - 1 - i);
      
   mLightGrid.clear();    

   clearLightSets();
}

//============================================================================
// BSceneLightManager::resetLightParams
//============================================================================
void BSceneLightManager::resetLightParams(eLightCategory lightIndex)
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   
   mDirLights[lightIndex].clear();
   mHemiFillLights[lightIndex].clear();
   mSHFillLights[lightIndex].clear();
}

//============================================================================
// BSceneLightManager::getDirLight
//============================================================================
const BDirLightParams& BSceneLightManager::getDirLight(eLightCategory lightIndex) const 
{ 
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   return mDirLights[lightIndex]; 
}

//============================================================================
// BSceneLightManager::setDirLight
//============================================================================
void BSceneLightManager::setDirLight(eLightCategory lightIndex, const BDirLightParams& params) 
{ 
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   mDirLights[lightIndex] = params; 
}

//============================================================================
// BSceneLightManager::getFogParams
//============================================================================
const BFogParams& BSceneLightManager::getFogParams(eLightCategory lightIndex) const
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   return mFogParams[lightIndex];
}

//============================================================================
// BSceneLightManager::setFogParams
//============================================================================
void BSceneLightManager::setFogParams(eLightCategory lightIndex, const BFogParams& params) 
{ 
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   mFogParams[lightIndex] = params; 
}

//============================================================================
// BSceneLightManager::getSHFillLight
//============================================================================
const BSHLightParams& BSceneLightManager::getSHFillLight(eLightCategory lightIndex) const 
{ 
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   return mSHFillLights[lightIndex]; 
}

//============================================================================
// BSceneLightManager::setSHFillLight
//============================================================================
void BSceneLightManager::setSHFillLight(eLightCategory lightIndex, const BSHLightParams& shParams) 
{ 
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   mSHFillLights[lightIndex] = shParams; 
}

//============================================================================
// BSceneLightManager::resetSHFillLights
//============================================================================
void BSceneLightManager::resetSHFillLights(void)
{
   for (int i = cLCFirstCategory; i < cNumLightCategories; i++)
      mSHFillLights[i].clear();
}

//============================================================================
// BSceneLightManager::getHemiFillLight
//============================================================================
const BHemiLightParams& BSceneLightManager::getHemiFillLight(eLightCategory lightIndex) const 
{ 
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   return mHemiFillLights[lightIndex]; 
}

//============================================================================
// BSceneLightManager::setHemiFillLight
//============================================================================
void BSceneLightManager::setHemiFillLight(eLightCategory lightIndex, const BHemiLightParams& shParams) 
{ 
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((lightIndex >= 0) && (lightIndex < cNumLightCategories)); 
   mHemiFillLights[lightIndex] = shParams; 
}

//============================================================================
// BSceneLightManager::createLocalLight
//============================================================================
BLocalLightHandle BSceneLightManager::createLocalLight(void)
{
   BDEBUG_ASSERT(mInitialized);
   if (mFreeLightHandles.empty())
      return cInvalidLocalLightHandle;

   const uint slot = mFreeLightHandles.back();
   mFreeLightHandles.popBack();
   
   const uint numActiveLights = mBaseLocalLightParams.size();
   mActiveLightIndices[slot] = (ushort)numActiveLights;
   
   mBaseLocalLightParams.resize(numActiveLights + 1);
   mLocalLightParams.resize(numActiveLights + 1);

   const BLocalLightHandle handle = slot + 1;
   
   mLocalLightParams[numActiveLights].clear();
   mBaseLocalLightParams[numActiveLights].clear();
   
   mLocalLightParams[numActiveLights].mHandle = handle;
   mLocalLightParams[numActiveLights].mLightID = mNextLightID;
   if (++mNextLightID == cInvalidLocalLightID)
      ++mNextLightID;
      
   mLocalLightEnabled.set(numActiveLights, FALSE);
   mLocalLightEnabledFrame[numActiveLights] = (uint)cInvalidLocalLightEnabledFrame;
   mActiveLightShadows.set(numActiveLights, FALSE);
   
   check();
      
   return handle;
}

//============================================================================
// BSceneLightManager::freeLocalLight
//============================================================================
void BSceneLightManager::freeLocalLight(BLocalLightHandle handle)
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT(handle != cInvalidLocalLightHandle);
   BDEBUG_ASSERT(!getLightsLocked());
   
   if (handle == cInvalidLocalLightHandle)
      return;
      
   check();

   const uint handleSlot = handle - 1;
   BDEBUG_ASSERT(handleSlot < cMaxExpectedLocalLights);
   
   const uint activeLightIndex = mActiveLightIndices[handleSlot];
   BDEBUG_ASSERT(activeLightIndex != USHRT_MAX);
   
   if (mLocalLightEnabled.get(activeLightIndex))
      mLightGrid.removeObject(mLightGrid.getGridRect(mBaseLocalLightParams[activeLightIndex].mPosRadius), activeLightIndex);
   
   mLocalLightEnabledFrame[activeLightIndex] = (uint)cInvalidLocalLightEnabledFrame;
   
   mActiveLightIndices[handleSlot] = USHRT_MAX;
   mFreeLightHandles.pushBack((ushort)handleSlot);
      
   BLocalLightParams& localLightParams = mLocalLightParams[activeLightIndex];
   BBaseLocalLightParams& baseLocalLightParams = mBaseLocalLightParams[activeLightIndex];
   
   BDEBUG_ASSERT(localLightParams.mHandle == handle);
   
   const uint numActiveLights = mBaseLocalLightParams.size();
   if (activeLightIndex != (numActiveLights - 1))
   {
      baseLocalLightParams = mBaseLocalLightParams[numActiveLights - 1];
      localLightParams = mLocalLightParams[numActiveLights - 1];
                  
      const uint movingHandleSlot = localLightParams.mHandle - 1;
      
      if (mLocalLightEnabled.get(numActiveLights - 1))
      {
         const BLightGrid::BGridRect gridRect = mLightGrid.getGridRect(baseLocalLightParams.mPosRadius);

         mLightGrid.removeObject(gridRect, numActiveLights - 1);
         mLightGrid.addObject(gridRect, activeLightIndex);
      }
      
      BDEBUG_ASSERT(movingHandleSlot < cMaxExpectedLocalLights);
      mActiveLightIndices[movingHandleSlot] = (ushort)activeLightIndex;
      
      mLocalLightEnabled.set(activeLightIndex, mLocalLightEnabled.get(numActiveLights - 1));
      mLocalLightEnabledFrame[activeLightIndex] = mLocalLightEnabledFrame[numActiveLights - 1];
      mActiveLightShadows.set(activeLightIndex, mActiveLightShadows.get(numActiveLights - 1));
   }
   else
   {
      baseLocalLightParams.clear();
      localLightParams.clear();
      
      mLocalLightEnabled.set(activeLightIndex, FALSE);
      mLocalLightEnabledFrame[activeLightIndex] = (uint)cInvalidLocalLightEnabledFrame;
      mActiveLightShadows.set(activeLightIndex, FALSE);
   }
   
   mBaseLocalLightParams.popBack();
   mLocalLightParams.popBack();
   
   check();
}

//============================================================================
// BSceneLightManager::freeAllLocalLights
//============================================================================
void BSceneLightManager::freeAllLocalLights(void)
{
   BDEBUG_ASSERT(mInitialized);
   
   mLocalLightParams.resize(0);
   mBaseLocalLightParams.resize(0);

   mLocalLightEnabled.setAll(0);
   mLocalLightEnabledFrame.setAll((uint)cInvalidLocalLightEnabledFrame);

   mActiveLightIndices.setAll(USHRT_MAX);

   mFreeLightHandles.resize(cMaxExpectedLocalLights);
   for (uint i = 0; i < cMaxExpectedLocalLights; i++)
      mFreeLightHandles[i] = (ushort)(cMaxExpectedLocalLights - 1 - i);   
      
   mLightGrid.clear();      
   
   check();
}

//============================================================================
// BSceneLightManager::getSlotFromHandle
//============================================================================
uint BSceneLightManager::getSlotFromHandle(BLocalLightHandle handle) const
{
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((handle != cInvalidLocalLightHandle) && (handle >= 1) && (handle <= cMaxExpectedLocalLights));
   const uint slot = mActiveLightIndices[handle - 1];
   BDEBUG_ASSERT(slot != USHRT_MAX && mLocalLightParams[slot].mHandle == handle);
   return slot;   
}

//============================================================================
// BSceneLightManager::enforceLimits
//============================================================================
void BSceneLightManager::enforceLimits(BLocalLightHandle handle)
{
   BDEBUG_ASSERT(mInitialized);
   
   check();
   
   const bool curEnabled = getLocalLightEnabled(handle);
   if (curEnabled)
      setLocalLightEnabled(handle, false);
   
   const uint slot = getSlotFromHandle(handle);
         
   if (mLocalLightParams[slot].getType() == cLTOmni)
   {
      setLocalLightSpotInner(handle, 0.0f);
      setLocalLightSpotOuter(handle, 0.0f);
   }
   else
   {
      if (getLocalLightSpotOuter(handle) < Math::fDegToRad(1.0f))
         setLocalLightSpotOuter(handle, Math::fDegToRad(1.0f));
         
      if (getLocalLightSpotInner(handle) < Math::fDegToRad(.5f))
         setLocalLightSpotInner(handle, Math::fDegToRad(.5f));
   }
   
   mBaseLocalLightParams[slot].enforceLimits();
   mLocalLightParams[slot].enforceLimits();
   
   if (curEnabled)
      setLocalLightEnabled(handle, true);
   
   check();
}

//============================================================================
// BSceneLightManager::getLocalLightPosRadius
//============================================================================
XMVECTOR BSceneLightManager::getLocalLightPosRadius(BLocalLightHandle handle) const
{
   const uint slot = getSlotFromHandle(handle);
   return mBaseLocalLightParams[slot].mPosRadius;
}

//============================================================================
// BSceneLightManager::setLocalLightPosRadius
//============================================================================
void BSceneLightManager::setLocalLightPosRadius(BLocalLightHandle handle, XMVECTOR posRadius)
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());
   
   check();
   
   BDEBUG_ASSERT(posRadius.w >= 0.0f);
   
   if (posRadius.w > cMaxLocalLightRadius)
      posRadius.w = cMaxLocalLightRadius;
   
   BLightGrid::BGridRect oldGridRect = mLightGrid.getGridRect(mBaseLocalLightParams[slot].mPosRadius);
   BLightGrid::BGridRect newGridRect = mLightGrid.getGridRect(posRadius);
   
   mBaseLocalLightParams[slot].mPosRadius = posRadius;
         
   if (mLocalLightEnabled.get(slot))
   {
      mLightGrid.removeObject(oldGridRect, slot);
      mLightGrid.addObject(newGridRect, slot);
   }
   
   check();
}

//============================================================================
// BSceneLightManager::setLocalLightPos
//============================================================================
void BSceneLightManager::setLocalLightPos(BLocalLightHandle handle, const XMFLOAT3* RESTRICT pPos)
{
   XMVECTOR posRadius = XMLoadFloat3(pPos);
   posRadius = XMVectorInsert(posRadius, getLocalLightPosRadius(handle), 0,  0, 0, 0, 1);
   setLocalLightPosRadius(handle, posRadius);
}

//============================================================================
// BSceneLightManager::setLocalLightRadius
//============================================================================
void BSceneLightManager::setLocalLightRadius(BLocalLightHandle handle, float radius)
{
   XMVECTOR posRadius = getLocalLightPosRadius(handle);
   posRadius = XMVectorInsert(posRadius, XMVectorReplicate(radius), 0,  0, 0, 0, 1);
   setLocalLightPosRadius(handle, posRadius);
}

//============================================================================
// BSceneLightManager::getLocalLightRadius
//============================================================================
float BSceneLightManager::getLocalLightRadius(BLocalLightHandle handle) const
{
   return getLocalLightPosRadius(handle).w;
}

//============================================================================
// BSceneLightManager::getLocalLightAtSpotOuter
//============================================================================
XMVECTOR BSceneLightManager::getLocalLightAtSpotOuter(BLocalLightHandle handle) const
{
   const uint slot = getSlotFromHandle(handle);
   
   return XMLoadHalf4(&mBaseLocalLightParams[slot].mAtSpotOuter); 
}

//============================================================================
// BSceneLightManager::setLocalLightAtSpotOuter
//============================================================================
void BSceneLightManager::setLocalLightAtSpotOuter(BLocalLightHandle handle, XMVECTOR atSpotOuter) 
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());
   
   XMStoreHalf4(&mBaseLocalLightParams[slot].mAtSpotOuter, atSpotOuter);
}

//============================================================================
// BSceneLightManager::getLocalLightRightSpotInner
//============================================================================
XMVECTOR BSceneLightManager::getLocalLightRightSpotInner(BLocalLightHandle handle) const
{
   const uint slot = getSlotFromHandle(handle);

   return XMLoadHalf4(&mBaseLocalLightParams[slot].mRightSpotInner); 
}

//============================================================================
// BSceneLightManager::setLocalLightRightSpotInner
//============================================================================
void BSceneLightManager::setLocalLightRightSpotInner(BLocalLightHandle handle, XMVECTOR rightSpotInner) 
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());

   XMStoreHalf4(&mBaseLocalLightParams[slot].mRightSpotInner, rightSpotInner);
}

//============================================================================
// BSceneLightManager::setLocalLightAt
//============================================================================
void BSceneLightManager::setLocalLightAt(BLocalLightHandle handle, const XMFLOAT3* RESTRICT pAt)
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());
   
   mBaseLocalLightParams[slot].mAtSpotOuter.x = XMConvertFloatToHalf(pAt->x);
   mBaseLocalLightParams[slot].mAtSpotOuter.y = XMConvertFloatToHalf(pAt->y);
   mBaseLocalLightParams[slot].mAtSpotOuter.z = XMConvertFloatToHalf(pAt->z);
}

//============================================================================
// BSceneLightManager::setLocalLightRight
//============================================================================
void BSceneLightManager::setLocalLightRight(BLocalLightHandle handle, const XMFLOAT3* RESTRICT pRight)
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());

   mBaseLocalLightParams[slot].mRightSpotInner.x = XMConvertFloatToHalf(pRight->x);
   mBaseLocalLightParams[slot].mRightSpotInner.y = XMConvertFloatToHalf(pRight->y);
   mBaseLocalLightParams[slot].mRightSpotInner.z = XMConvertFloatToHalf(pRight->z);
}

//============================================================================
// BSceneLightManager::getLocalLightSpotOuter
//============================================================================
float BSceneLightManager::getLocalLightSpotOuter(BLocalLightHandle handle) const
{
   const uint slot = getSlotFromHandle(handle);
   
   return XMConvertHalfToFloat(mBaseLocalLightParams[slot].mAtSpotOuter.w);
}

//============================================================================
// BSceneLightManager::setLocalLightSpotOuter
//============================================================================
void BSceneLightManager::setLocalLightSpotOuter(BLocalLightHandle handle, float spotOuter)
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());
   
   mBaseLocalLightParams[slot].mAtSpotOuter.w = XMConvertFloatToHalf(spotOuter);
}

//============================================================================
// BSceneLightManager::getLocalLightSpotInner
//============================================================================
float BSceneLightManager::getLocalLightSpotInner(BLocalLightHandle handle) const
{
   const uint slot = getSlotFromHandle(handle);

   return XMConvertHalfToFloat(mBaseLocalLightParams[slot].mRightSpotInner.w);
}

//============================================================================
// BSceneLightManager::setLocalLightSpotInner
//============================================================================
void BSceneLightManager::setLocalLightSpotInner(BLocalLightHandle handle, float spotInner)
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());

   mBaseLocalLightParams[slot].mRightSpotInner.w = XMConvertFloatToHalf(spotInner);
}

//============================================================================
// BSceneLightManager::getActiveLightIndex
//============================================================================
uint BSceneLightManager::getActiveLightIndex(BLocalLightHandle handle) const
{
   return getSlotFromHandle(handle);
}

//============================================================================
// BSceneLightManager::getLocalLightParams
//============================================================================
const BLocalLightParams& BSceneLightManager::getLocalLightParams(BLocalLightHandle handle) const
{
   const uint slot = getSlotFromHandle(handle);
   return mLocalLightParams[slot];
}

//============================================================================
// BSceneLightManager::getLocalLightParams
//============================================================================
BLocalLightParams& BSceneLightManager::getLocalLightParams(BLocalLightHandle handle)
{
   const uint slot = getSlotFromHandle(handle);
   return mLocalLightParams[slot];
}

//============================================================================
// BSceneLightManager::setLocalLightEnabled
//============================================================================
void BSceneLightManager::setLocalLightEnabled(BLocalLightHandle handle, bool enabled)
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());
   
   check();
   
   const bool curEnabled = (mLocalLightEnabled.get(slot) != FALSE);
   if (curEnabled == enabled)
      return;
      
   mLocalLightEnabled.set(slot, enabled);
      
   if (!enabled)
   {
      mLightGrid.removeObject(mLightGrid.getGridRect(mBaseLocalLightParams[slot].mPosRadius), slot);  
   }
   else
   {
      if (gRenderThread.isSimThread())
         mLocalLightEnabledFrame[slot] = gRenderThread.getCurMainFrameNoBarrier();
      else
         mLocalLightEnabledFrame[slot] = gRenderThread.getCurWorkerFrameNoBarrier();
         
      const BLocalLightParams& params = getLocalLightParams(handle);
      
      if (params.getType() == cLTOmni)
      {
         setLocalLightSpotInner(handle, 0.0f);
         setLocalLightSpotOuter(handle, 0.0f);
      }
   
      mLightGrid.addObject(mLightGrid.getGridRect(mBaseLocalLightParams[slot].mPosRadius), slot);
   }
   
   check();
}

//============================================================================
// BSceneLightManager::getLocalLightEnabled
//============================================================================
bool BSceneLightManager::getLocalLightEnabled(BLocalLightHandle handle) const
{
   const uint slot = getSlotFromHandle(handle);
   return mLocalLightEnabled.get(slot) != 0;
}

//============================================================================
// BSceneLightManager::setLocalLightShadows
//============================================================================
void BSceneLightManager::setLocalLightShadows(BLocalLightHandle handle, bool enable)
{
   const uint slot = getSlotFromHandle(handle);
   BDEBUG_ASSERT(!getLightsLocked());
   
   mActiveLightShadows.set(slot, enable);
   
   if (!enable)
      mLocalLightParams[slot].setInvalidShadowIndex();
}

//============================================================================
// BSceneLightManager::getLocalLightShadows
//============================================================================
bool BSceneLightManager::getLocalLightShadows(BLocalLightHandle handle)
{
   const uint slot = getSlotFromHandle(handle);
   return mActiveLightShadows.get(slot) != FALSE;
}

//============================================================================
// BSceneLightManager::getActiveLocalLightEnabledFrame
//============================================================================
uint BSceneLightManager::getActiveLocalLightEnabledFrame(BActiveLightIndex activeLightIndex)
{
   return mLocalLightEnabledFrame[activeLightIndex];
}

//============================================================================
// BSceneLightManager::getActiveLightShadows
//============================================================================
bool BSceneLightManager::getActiveLightShadows(BActiveLightIndex activeLightIndex) const
{
   return mActiveLightShadows.get(activeLightIndex) != FALSE;
}

//============================================================================
// BSceneLightManager::findLights
//============================================================================
void BSceneLightManager::findLights(BActiveLightIndexArray& activeLightIndices, BGridRect gridRect, XMVECTOR min, XMVECTOR max, bool refine)
{
   BDEBUG_ASSERT(mInitialized);
         
   mLightGrid.getAllObjects(&activeLightIndices, gridRect);
   
   if (refine)
   {
      XMVECTOR boundingSphere = XMVectorMultiply(XMVectorAdd(min, max), XMVectorReplicate(.5f));
      boundingSphere = __vrlimi(boundingSphere, XMVector3LengthEst(XMVectorSubtract(max, boundingSphere)), VRLIMI_CONST(0, 0, 0, 1), 0);
                  
      uint dstIndex = 0;
      
      for (uint i = 0; i < activeLightIndices.size(); i++)
      {
         const uint activeLightIndex = activeLightIndices[i];
         const BBaseLocalLightParams& baseParams = mBaseLocalLightParams[activeLightIndex];
         
         if (!BVMXIntersection::sphereVsAABB(baseParams.mPosRadius, min, max))
            continue;
            
         if (baseParams.isSpotLight())
         {
            XMVECTOR atSpotOuter = baseParams.getAtSpotOuter();
            const bool touching = BVMXIntersection::coneSphere(baseParams.mPosRadius, atSpotOuter, .5f * atSpotOuter.w, boundingSphere);
            if (!touching)
               continue;
         }
         
         activeLightIndices[dstIndex] = (WORD)activeLightIndex;
         dstIndex++;
      }   
      
      activeLightIndices.resize(dstIndex);
   }
}

//============================================================================
// Helper func
//============================================================================
namespace
{
   BVector srgb8ToLinear(BVector c)
   {
      float* p = reinterpret_cast<float*>(&c);
      for (uint i = 0; i < 3; i++)
         p[i] = powf(Math::Clamp(p[i] * 1.0f/255.0f, 0.0f, 1.0f), 2.2f);
      return c;         
   }
}


//============================================================================
// BRawLightSettings::lerp
//============================================================================
BRawLightSettings BRawLightSettings::lerp(BRawLightSettings a, BRawLightSettings b, float fract)
{
   BRawLightSettings output;

   output.mSunShadows = b.mSunShadows;
   output.mSunUnitsEnabled = b.mSunUnitsEnabled;
   output.mSunTerrainEnabled = b.mSunTerrainEnabled;


   output.mSunInclination = Math::Lerp<float,float>(a.mSunInclination, b.mSunInclination, fract);
   output.mSunRotation = Math::Lerp<float,float>(a.mSunRotation, b.mSunRotation, fract);
   
   output.mSunUnitColor = Math::Lerp<BVector,float>(a.mSunUnitColor, b.mSunUnitColor, fract);
   output.mSunTerrainColor = Math::Lerp<BVector,float>(a.mSunTerrainColor, b.mSunTerrainColor, fract);
   output.mSunParticleColor = Math::Lerp<BVector,float>(a.mSunParticleColor, b.mSunParticleColor, fract);

   output.mSunUnitIntensity = Math::Lerp<float,float>(a.mSunUnitIntensity, b.mSunUnitIntensity, fract);
   output.mSunTerrainIntensity = Math::Lerp<float,float>(a.mSunTerrainIntensity, b.mSunTerrainIntensity, fract);
   output.mSunParticleIntensity = Math::Lerp<float,float>(a.mSunParticleIntensity, b.mSunParticleIntensity, fract);

   output.mSunUnitShadowDarkness = Math::Lerp<float,float>(a.mSunUnitShadowDarkness, b.mSunUnitShadowDarkness, fract);
   output.mSunTerrainShadowDarkness = Math::Lerp<float,float>(a.mSunTerrainShadowDarkness, b.mSunTerrainShadowDarkness, fract);

   //mSunShadows = true;
   //mSunUnitsEnabled = true;
   //mSunTerrainEnabled = true;
   

   output.mHemiInclination = Math::Lerp<float,float>(a.mHemiInclination, b.mHemiInclination, fract);
   output.mHemiRotation = Math::Lerp<float,float>(a.mHemiRotation, b.mHemiRotation, fract);

   output.mHemiUnitTopColor = Math::Lerp<BVector,float>(a.mHemiUnitTopColor, b.mHemiUnitTopColor, fract);
   output.mHemiUnitBottomColor = Math::Lerp<BVector,float>(a.mHemiUnitBottomColor, b.mHemiUnitBottomColor, fract);
   output.mHemiTerrainTopColor = Math::Lerp<BVector,float>(a.mHemiTerrainTopColor, b.mHemiTerrainTopColor, fract);
   output.mHemiTerrainBottomColor = Math::Lerp<BVector,float>(a.mHemiTerrainBottomColor, b.mHemiTerrainBottomColor, fract);
   output.mHemiUnitIntensity = Math::Lerp<float,float>(a.mHemiUnitIntensity, b.mHemiUnitIntensity, fract);
   output.mHemiTerrainIntensity = Math::Lerp<float,float>(a.mHemiTerrainIntensity, b.mHemiTerrainIntensity, fract);
   
   //mToneMapParams;
   //output.mToneMapParams.clear();
   //mToneMapParams.

   output.mToneMapParams.mMiddleGrey = Math::Lerp<float,float>(a.mToneMapParams.mMiddleGrey, b.mToneMapParams.mMiddleGrey, fract);
   output.mToneMapParams.mBrightMaskThresh = Math::Lerp<float,float>(a.mToneMapParams.mBrightMaskThresh, b.mToneMapParams.mBrightMaskThresh, fract);
   
   output.mToneMapParams.mBloomIntensity = Math::Lerp<float,float>(a.mToneMapParams.mBloomIntensity, b.mToneMapParams.mBloomIntensity, fract);
   output.mToneMapParams.mBloomSigma = Math::Lerp<float,float>(a.mToneMapParams.mBloomSigma, b.mToneMapParams.mBloomSigma, fract);
   
   output.mToneMapParams.mAdaptationRate = Math::Lerp<float,float>(a.mToneMapParams.mAdaptationRate, b.mToneMapParams.mAdaptationRate, fract);
   output.mToneMapParams.mLogAveMin = Math::Lerp<float,float>(a.mToneMapParams.mLogAveMin, b.mToneMapParams.mLogAveMin, fract);  
   output.mToneMapParams.mLogAveMax = Math::Lerp<float,float>(a.mToneMapParams.mLogAveMax, b.mToneMapParams.mLogAveMax, fract);

   output.mToneMapParams.mWhitePointMin = Math::Lerp<float,float>(a.mToneMapParams.mWhitePointMin, b.mToneMapParams.mWhitePointMin, fract);
   output.mToneMapParams.mWhitePointMax = Math::Lerp<float,float>(a.mToneMapParams.mWhitePointMax, b.mToneMapParams.mWhitePointMax, fract);

   output.mToneMapParams.mDOFFarBlurPlaneDist = Math::Lerp<float,float>(a.mToneMapParams.mDOFFarBlurPlaneDist, b.mToneMapParams.mDOFFarBlurPlaneDist, fract);
   output.mToneMapParams.mDOFFocalPlaneDist = Math::Lerp<float,float>(a.mToneMapParams.mDOFFocalPlaneDist, b.mToneMapParams.mDOFFocalPlaneDist, fract);
   output.mToneMapParams.mDOFNearBlurPlaneDist = Math::Lerp<float,float>(a.mToneMapParams.mDOFNearBlurPlaneDist, b.mToneMapParams.mDOFNearBlurPlaneDist, fract);
   output.mToneMapParams.mDOFMaxBlurriness = Math::Lerp<float,float>(a.mToneMapParams.mDOFMaxBlurriness, b.mToneMapParams.mDOFMaxBlurriness, fract);
   
   output.mToneMapParams.mDOFEnabled = b.mToneMapParams.mDOFEnabled;
   output.mToneMapParams.mQuarterResBlooms = b.mToneMapParams.mQuarterResBlooms; 
   
   output.mZFogColor = Math::Lerp<BVector,float>(a.mZFogColor, b.mZFogColor, fract);
   output.mZFogIntensity = Math::Lerp<float,float>(a.mZFogIntensity, b.mZFogIntensity, fract);
   output.mZFogStart = Math::Lerp<float,float>(a.mZFogStart, b.mZFogStart, fract);
   output.mZFogDensity = Math::Lerp<float,float>(a.mZFogDensity, b.mZFogDensity, fract);

   output.mPlanarFogColor = Math::Lerp<BVector,float>(a.mPlanarFogColor, b.mPlanarFogColor, fract);
   output.mPlanarFogIntensity = Math::Lerp<float,float>(a.mPlanarFogIntensity, b.mPlanarFogIntensity, fract);
   output.mPlanarFogStart = Math::Lerp<float,float>(a.mPlanarFogStart, b.mPlanarFogStart, fract);
   output.mPlanarFogDensity = Math::Lerp<float,float>(a.mPlanarFogDensity, b.mPlanarFogDensity, fract);

   
   output.mTerrainSpecPower = Math::Lerp<float,float>(a.mTerrainSpecPower, b.mTerrainSpecPower, fract);
   output.mTerrainBumpPower = Math::Lerp<float,float>(a.mTerrainBumpPower, b.mTerrainBumpPower, fract);
   output.mTerrainAODiffuseIntensity = Math::Lerp<float,float>(a.mTerrainAODiffuseIntensity, b.mTerrainAODiffuseIntensity, fract);

   output.mTerrainSpecOnlyColor = Math::Lerp<BVector,float>(a.mTerrainSpecOnlyColor, b.mTerrainSpecOnlyColor, fract);
   output.mTerrainSpecOnlyPower = Math::Lerp<float,float>(a.mTerrainSpecOnlyPower, b.mTerrainSpecOnlyPower, fract);
   output.mTerrainSpecOnlyShadowAttn = Math::Lerp<float,float>(a.mTerrainSpecOnlyShadowAttn, b.mTerrainSpecOnlyShadowAttn, fract);
   output.mTerrainSpecOnlySunInclination = Math::Lerp<float,float>(a.mTerrainSpecOnlySunInclination, b.mTerrainSpecOnlySunInclination, fract);
   output.mTerrainSpecOnlySunRotation = Math::Lerp<float,float>(a.mTerrainSpecOnlySunRotation, b.mTerrainSpecOnlySunRotation, fract);

   
   //mGlobalEnvMapName = "\\environment\\reflection\\defaultGlobalEnvMap";
   output.mShFillLightIntensity = Math::Lerp<float,float>(a.mShFillLightIntensity, b.mShFillLightIntensity, fract);
   output.mBackgroundColor = Math::Lerp<BVec3,float>(a.mBackgroundColor, b.mBackgroundColor, fract);
   output.mBackgroundIntensity = Math::Lerp<float>(a.mBackgroundIntensity, b.mBackgroundIntensity, fract);

   output.mDofNearRange = Math::Lerp<float,float>(a.mDofNearRange, b.mDofNearRange, fract);
   output.mDofFarRange = Math::Lerp<float,float>(a.mDofFarRange, b.mDofFarRange, fract);


   for(int i=0;i<3;i++)
   {
      for(int j=0;j<9;j++)
      {
         output.mSHFillLights.mSHCoeffs[i][j] = Math::Lerp<float,float>(a.mSHFillLights.mSHCoeffs[i][j], b.mSHFillLights.mSHCoeffs[i][j], fract);
      }
   }
   
   output.mLGTIntensityScale = Math::Lerp<float, float>(a.mLGTIntensityScale, b.mLGTIntensityScale, fract);
   output.mLGTParticleIntensityScale = Math::Lerp<float, float>(a.mLGTParticleIntensityScale, b.mLGTParticleIntensityScale, fract);
      
   return output;
}

//============================================================================
// BRawLightSettings::reload
//============================================================================
void BRawLightSettings::reload()
{
   BXMLReader reader;
   if(!reader.load(0, mFilename.getPtr()))
   {
      return;
   }
   int id = mId;
   BXMLNode rootNode(reader.getRootNode());
   loadLightSet(mFilename, rootNode);
   mId = id;
}

//============================================================================
// BRawLightSettings::loadLightSet
//============================================================================
bool BRawLightSettings::loadLightSet(BString filename, BXMLNode& root)
{
   mFilename = filename;

   mId = 0;
   mSunInclination = 0.0f;
   mSunRotation = 0.0f;
   
   mSunUnitColor.set(255.0f, 255.0f, 255.0f);
   mSunTerrainColor.set(255.0f, 255.0f, 255.0f);
   mSunParticleColor.set(255.0f, 255.0f, 255.0f);

   mSunUnitIntensity = 1.0f;
   mSunTerrainIntensity = 1.0f;
   mSunParticleIntensity = 1.0f;

   mSunUnitShadowDarkness = 0.0f;
   mSunTerrainShadowDarkness = 0.0f;
   mSunShadows = true;
   mSunUnitsEnabled = true;
   mSunTerrainEnabled = true;
   
   mHemiInclination = 0.0f;
   mHemiRotation = 0.0f;
   mHemiUnitTopColor.set(0.0f, 0.0f, 0.0f);
   mHemiUnitBottomColor.set(0.0f, 0.0f, 0.0f);
   mHemiTerrainTopColor.set(0.0f, 0.0f, 0.0f);
   mHemiTerrainBottomColor.set(0.0f, 0.0f, 0.0f);
   mHemiUnitIntensity = 1.0f;
   mHemiTerrainIntensity = 1.0f;
   
   mToneMapParams;
   mToneMapParams.clear();
   
   mZFogColor.set(0.0f, 0.0f, 0.0f);
   mZFogIntensity = 1.0f;
   mZFogStart = 99999.0f;
   mZFogDensity = 0.0f;
   
   mPlanarFogColor.set(0.0f, 0.0f, 0.0f);
   mPlanarFogIntensity = 1.0f;
   mPlanarFogStart = 99999.0f;
   mPlanarFogDensity = 0.0f;

   mTerrainSpecPower = 25.0f;
   mTerrainBumpPower = 1.0f;
   mTerrainAODiffuseIntensity = 0.0f;
   mTerrainSpecOnlyColor.set(255,255,255);
   mTerrainSpecOnlyPower = 100.0f;
   mTerrainSpecOnlyShadowAttn = 1.0f;
   mTerrainSpecOnlySunInclination = 0.0f;
   mTerrainSpecOnlySunRotation = 0.0f;

   
   mGlobalEnvMapName = "\\environment\\reflection\\defaultGlobalEnvMap";

   mShFillLightIntensity = 1.0f;
   mBackgroundColor.set(.25*106.0f/255.0f, .25*107.0f/255.0f, .25*135.0f/255.0f);
   mBackgroundIntensity = 1.0f;
   

   mDofNearRange = 100.0f;
   mDofFarRange = 100.0f;
   
   mLGTIntensityScale = 1.3f;
   mLGTParticleIntensityScale = 1.4f;   
       
   const int nodeCount = root.getNumberChildren();
   for(int i = 0; i < nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString nodeName(node.getName());
      
      if (nodeName=="LGTIntensityScale")
      {
         node.getTextAsFloat(mLGTIntensityScale);
         mLGTIntensityScale = Math::Clamp(mLGTIntensityScale, .0125f, 16.0f);
      }
      else if (nodeName=="LGTParticleIntensityScale")
      {
         node.getTextAsFloat(mLGTParticleIntensityScale);
         mLGTParticleIntensityScale = Math::Clamp(mLGTParticleIntensityScale, .0125f, 16.0f);
      }      
      else if (nodeName=="middleGrey")
      {
         node.getTextAsFloat(mToneMapParams.mMiddleGrey);
      }
      else if (nodeName=="logAveMin")
      {
         node.getTextAsFloat(mToneMapParams.mLogAveMin);      
      }
      else if (nodeName=="logAveMax")
      {
         node.getTextAsFloat(mToneMapParams.mLogAveMax);      
      }
      else if (nodeName=="whitePointMin")
      {
         node.getTextAsFloat(mToneMapParams.mWhitePointMin);      
      }
      else if (nodeName=="whitePointMax")
      {
         node.getTextAsFloat(mToneMapParams.mWhitePointMax);      
      }
      else if (nodeName=="brightMaskThresh")
      {
         node.getTextAsFloat(mToneMapParams.mBrightMaskThresh);
      }
      else if (nodeName=="bloomIntensity")
      {
         node.getTextAsFloat(mToneMapParams.mBloomIntensity);
      }
      else if (nodeName=="bloomSigma")
      {
         node.getTextAsFloat(mToneMapParams.mBloomSigma);
      }
      else if (nodeName=="adaptationRate")
      {
         node.getTextAsFloat(mToneMapParams.mAdaptationRate);
         mToneMapParams.mAdaptationRate = Math::Max(mToneMapParams.mAdaptationRate, .001f);
      }
      else if (nodeName=="sunInclination")
      {
         node.getTextAsFloat(mSunInclination);
      }
      else if (nodeName=="sunRotation")
      {
         node.getTextAsFloat(mSunRotation);
      }
      else if (nodeName=="sunUnitColor")
      {
         node.getTextAsVector(mSunUnitColor);
      }
      else if (nodeName=="sunUnitIntensity")
      {
         node.getTextAsFloat(mSunUnitIntensity);
      }
      else if (nodeName=="sunUnitShadowDarkness")
      {
         node.getTextAsFloat(mSunUnitShadowDarkness);
      }
      else if (nodeName=="setTerrainColor")
      {
         node.getTextAsVector(mSunTerrainColor);
      }
      else if (nodeName=="sunTerrainIntensity")
      {
         node.getTextAsFloat(mSunTerrainIntensity);
      }
      else if (nodeName=="SunParticleColor")
      {
         node.getTextAsVector(mSunParticleColor);
      }
      else if (nodeName=="SunParticleIntensity")
      {
         node.getTextAsFloat(mSunParticleIntensity);
      }
      else if (nodeName=="sunTerrainShadowDarkness")
      {
         node.getTextAsFloat(mSunTerrainShadowDarkness);
      }
      else if (nodeName=="sunShadows")
      {
         node.getTextAsBool(mSunShadows);
      }
      else if (nodeName=="sunUnitsEnabled")
      {
         node.getTextAsBool(mSunUnitsEnabled);
      }
      else if (nodeName=="sunTerrainEnabled")
      {
         node.getTextAsBool(mSunTerrainEnabled);
      }
      else if (nodeName=="hemiInclination")
      {
         node.getTextAsFloat(mHemiInclination);
      }
      else if (nodeName=="hemiRotation")
      {
         node.getTextAsFloat(mHemiRotation);
      }
      else if (nodeName=="hemiUnitTopColor")
      {
         node.getTextAsVector(mHemiUnitTopColor);
      }
      else if (nodeName=="hemiUnitBottomColor")
      {
         node.getTextAsVector(mHemiUnitBottomColor);
      }
      else if (nodeName=="hemiTerrainTopColor")
      {
         node.getTextAsVector(mHemiTerrainTopColor);
      }
      else if (nodeName=="hemiTerrainBottomColor")
      {
         node.getTextAsVector(mHemiTerrainBottomColor);
      }
      else if (nodeName=="hemiUnitIntensity")
      {
         node.getTextAsFloat(mHemiUnitIntensity);
      }
      else if (nodeName=="hemiTerrainIntensity")
      {
         node.getTextAsFloat(mHemiTerrainIntensity);
      }
      else if (nodeName=="ZFogColor")
      {
         node.getTextAsVector(mZFogColor);  
      }
      else if (nodeName=="ZFogIntensity")
      {
         node.getTextAsFloat(mZFogIntensity);  
      }
      else if (nodeName=="ZFogStart")
      {
         node.getTextAsFloat(mZFogStart);
      }
      else if (nodeName=="ZFogDensity")
      {
         node.getTextAsFloat(mZFogDensity);
      }
      else if (nodeName=="PlanarFogColor")
      {
         node.getTextAsVector(mPlanarFogColor);  
      }
      else if (nodeName=="PlanarFogIntensity")
      {
         node.getTextAsFloat(mPlanarFogIntensity);  
      }
      else if (nodeName=="PlanarFogStart")
      {
         node.getTextAsFloat(mPlanarFogStart);
      }
      else if (nodeName=="PlanarFogDensity")
      {
         node.getTextAsFloat(mPlanarFogDensity);
      }
      else if (nodeName=="environmentMap")
      {
         if (!node.isTextEmpty())
         {
            node.getText(mGlobalEnvMapName);
            strPathRemoveExtension(mGlobalEnvMapName);
         }
      }
      else if (nodeName=="SHFillIntensity")
      {
         node.getTextAsFloat(mShFillLightIntensity);
         mShFillLightIntensity = Math::Clamp(mShFillLightIntensity, 0.0f, 30.0f);
      }
      else if (nodeName=="backgroundColor")
      {
         BVector color;
         node.getTextAsVector(color);
         //color = srgb8ToLinear(color);
         color.x = pow(color.x/255.0f, 2.2f);
         color.y = pow(color.y/255.0f, 2.2f);
         color.z = pow(color.z/255.0f, 2.2f);
         
         mBackgroundColor.set(color.x, color.y, color.z);
      }
      else if (nodeName=="backgroundIntensity")
      {
         node.getTextAsFloat(mBackgroundIntensity);
         mBackgroundIntensity = Math::Clamp(mBackgroundIntensity, 0.0f, 128.0f);
      }
      else if (nodeName=="terrainSpecularPower")
      {
         node.getTextAsFloat(mTerrainSpecPower);
      }
      else if (nodeName=="terrainBumpStrength")
      {
         node.getTextAsFloat(mTerrainBumpPower);
      }
      else if (nodeName=="TerrainAODiffuseIntensity")
      {
         node.getTextAsFloat(mTerrainAODiffuseIntensity);
         mTerrainAODiffuseIntensity = Math::Clamp(mTerrainAODiffuseIntensity, 0.0f, 1.0f);
      }
      else if (nodeName=="TerrainSpecOnlyColor")
      {
         node.getTextAsVector(mTerrainSpecOnlyColor);
      }
      else if (nodeName=="TerrainSpecOnlyPower")
      {
         node.getTextAsFloat(mTerrainSpecOnlyPower);
      }
      else if (nodeName=="TerrainSpecOnlyShadowDarkness")
      {
         node.getTextAsFloat(mTerrainSpecOnlyShadowAttn);
      }
      else if (nodeName=="TerrainSpecOnlySunInclination")
      {
         node.getTextAsFloat(mTerrainSpecOnlySunInclination);
      }
      else if (nodeName=="TerrainSpecOnlySunRotation")
      {
         node.getTextAsFloat(mTerrainSpecOnlySunRotation);
      }
      else if (nodeName == "dofEnabled")
      {
         bool dofEnabled = false;
         node.getTextAsBool(dofEnabled);
         mToneMapParams.mDOFEnabled = dofEnabled;
      }
      else if (nodeName == "dofNearRange")
      {
         node.getTextAsFloat(mDofNearRange);
      }
      else if (nodeName == "dofFocalPlaneDist")
      {
         node.getTextAsFloat(mToneMapParams.mDOFFocalPlaneDist);
      }
      else if (nodeName == "dofFarRange")
      {
         node.getTextAsFloat(mDofFarRange);
      }
      else if (nodeName == "dofMaxBlurriness")
      {
         node.getTextAsFloat(mToneMapParams.mDOFMaxBlurriness);
      }
   }

   mSHFillLights.clear();

   return true;
}

//============================================================================
// BSceneLightManager::loadLightSet
//============================================================================
bool BSceneLightManager::loadLightSet(BXMLNode glsRoot, BXMLNode flsRoot, bool uiSetOnly)
{
   BRawLightSettings rawSettings;
   rawSettings.loadLightSet("", glsRoot);
   
   if (flsRoot.getValid())
      rawSettings.mSHFillLights.load(flsRoot);
   
   return setLightSet(rawSettings, uiSetOnly);
}

//============================================================================
// BSceneLightManager::loadLightSet
//============================================================================
bool BSceneLightManager::loadLightSet(BXMLNode root, bool uiSetOnly)
{
   BRawLightSettings rawSettings;
   rawSettings.loadLightSet("", root);
   
   return setLightSet(rawSettings, uiSetOnly);
}

//============================================================================
// BSceneLightManager::setLightSet
//============================================================================
bool BSceneLightManager::setLightSet(const BRawLightSettings& rawSettings, bool uiSetOnly, bool updateTextures)
{
   if(uiSetOnly == false)
   {
      mCurrentRawSettings = rawSettings;
   }

   BVector sunUnitColor = srgb8ToLinear(rawSettings.mSunUnitColor);
   BVector sunTerrainColor = srgb8ToLinear(rawSettings.mSunTerrainColor);
   BVector sunParticleColor = srgb8ToLinear(rawSettings.mSunParticleColor);

   BVector hemiUnitTopColor = srgb8ToLinear(rawSettings.mHemiUnitTopColor);
   BVector hemiUnitBottomColor = srgb8ToLinear(rawSettings.mHemiUnitBottomColor);
   BVector hemiTerrainTopColor = srgb8ToLinear(rawSettings.mHemiTerrainTopColor);
   BVector hemiTerrainBottomColor = srgb8ToLinear(rawSettings.mHemiTerrainBottomColor);
   BVector zFogColor = srgb8ToLinear(rawSettings.mZFogColor);
   BVector planarFogColor = srgb8ToLinear(rawSettings.mPlanarFogColor);
   
   // Dir lighting
   BVector sunDir;
   BVector hemiDir;
   
   BMatrix mat;
   mat.makeRotateX(Math::fDegToRad(rawSettings.mSunInclination));
   mat.multRotateY(Math::fDegToRad(rawSettings.mSunRotation));   
   mat.transformVector(cYAxisVector, sunDir);
   sunDir = -sunDir;
   
   mat.makeRotateX(Math::fDegToRad(rawSettings.mHemiInclination));
   mat.multRotateY(Math::fDegToRad(rawSettings.mHemiRotation));   
   mat.transformVector(cYAxisVector, hemiDir);
      
   BDirLightParams unitDirLightParams;
   BDirLightParams terrainDirLightParams;
   
   unitDirLightParams.clear();
   terrainDirLightParams.clear();
   
   unitDirLightParams.mEnabled = rawSettings.mSunUnitsEnabled;
   terrainDirLightParams.mEnabled = rawSettings.mSunTerrainEnabled;
   
   unitDirLightParams.mShadows = rawSettings.mSunShadows;
   terrainDirLightParams.mShadows = rawSettings.mSunShadows;
         
   unitDirLightParams.mDir = XMVectorSet(sunDir.x, sunDir.y, sunDir.z, 0.0f);
   terrainDirLightParams.mDir = unitDirLightParams.mDir;
     
   unitDirLightParams.mColor = XMVectorSet(rawSettings.mSunUnitIntensity*sunUnitColor.x, rawSettings.mSunUnitIntensity*sunUnitColor.y, rawSettings.mSunUnitIntensity*sunUnitColor.z, 1.0f);
   terrainDirLightParams.mColor = XMVectorSet(rawSettings.mSunTerrainIntensity*sunTerrainColor.x, rawSettings.mSunTerrainIntensity*sunTerrainColor.y, rawSettings.mSunTerrainIntensity*sunTerrainColor.z, 1.0f);
   
   unitDirLightParams.mShadowDarkness = rawSettings.mSunUnitShadowDarkness;
   terrainDirLightParams.mShadowDarkness = rawSettings.mSunTerrainShadowDarkness;
   
   // Hemi
   BHemiLightParams unitHemiLightParams;
   BHemiLightParams terrainHemiLightParams;
   unitHemiLightParams.clear();
   terrainHemiLightParams.clear();
   
   unitHemiLightParams.mAxis = XMVectorSet(hemiDir.x, hemiDir.y, hemiDir.z, 0.0f);
   terrainHemiLightParams.mAxis = unitHemiLightParams.mAxis;
   
   unitHemiLightParams.mTopColor = XMVectorSet(rawSettings.mHemiUnitIntensity*hemiUnitTopColor.x, rawSettings.mHemiUnitIntensity*hemiUnitTopColor.y, rawSettings.mHemiUnitIntensity*hemiUnitTopColor.z, 1.0f);
   unitHemiLightParams.mBottomColor = XMVectorSet(rawSettings.mHemiUnitIntensity*hemiUnitBottomColor.x, rawSettings.mHemiUnitIntensity*hemiUnitBottomColor.y, rawSettings.mHemiUnitIntensity*hemiUnitBottomColor.z, 1.0f);
   
   terrainHemiLightParams.mTopColor = XMVectorSet(rawSettings.mHemiTerrainIntensity*hemiTerrainTopColor.x, rawSettings.mHemiTerrainIntensity*hemiTerrainTopColor.y, rawSettings.mHemiTerrainIntensity*hemiTerrainTopColor.z, 1.0f);
   terrainHemiLightParams.mBottomColor = XMVectorSet(rawSettings.mHemiTerrainIntensity*hemiTerrainBottomColor.x, rawSettings.mHemiTerrainIntensity*hemiTerrainBottomColor.y, rawSettings.mHemiTerrainIntensity*hemiTerrainBottomColor.z, 1.0f);
   
   // Fog
   BFogParams fogParams;
   fogParams.mZColor = BVec3(zFogColor.x * rawSettings.mZFogIntensity, zFogColor.y * rawSettings.mZFogIntensity, zFogColor.z * rawSettings.mZFogIntensity);
   fogParams.mZStart = rawSettings.mZFogStart;
   fogParams.mZDensity = rawSettings.mZFogDensity;   
   
   fogParams.mPlanarColor = BVec3(planarFogColor.x * rawSettings.mPlanarFogIntensity, planarFogColor.y * rawSettings.mPlanarFogIntensity, planarFogColor.z * rawSettings.mPlanarFogIntensity);
   fogParams.mPlanarStart = rawSettings.mPlanarFogStart;
   fogParams.mPlanarDensity = rawSettings.mPlanarFogDensity;
   
   // Set UI or terrain / unit / global light settings
   if (uiSetOnly)
   {
      setDirLight(cLCUI, unitDirLightParams);
      setHemiFillLight(cLCUI, unitHemiLightParams);
      setFogParams(cLCUI, fogParams);
   }
   else
   {
      mSHFillLightIntensity = rawSettings.mShFillLightIntensity;
      mBackgroundColor = rawSettings.mBackgroundColor;
      mBackgroundIntensity = rawSettings.mBackgroundIntensity;

      if(updateTextures)
      {
         if (mGlobalEnvMap != cInvalidManagedTextureHandle)
            gD3DTextureManager.unloadManagedTextureByHandle(mGlobalEnvMap);
         
         mGlobalEnvMap = gD3DTextureManager.getOrCreateHandle(rawSettings.mGlobalEnvMapName.getPtr(), BFILE_OPEN_NORMAL, BD3DTextureManager::cSystem, false, cDefaultTextureWhite, true, false, "SceneLightManager");
      }
      
      BToneMapParams toneMapParams = rawSettings.mToneMapParams;
      toneMapParams.mDOFNearBlurPlaneDist = rawSettings.mToneMapParams.mDOFFocalPlaneDist - rawSettings.mDofNearRange;
      toneMapParams.mDOFFarBlurPlaneDist = rawSettings.mToneMapParams.mDOFFocalPlaneDist + rawSettings.mDofFarRange;   
      toneMapParams.enforceLimits();
      gToneMapManager.setParams(toneMapParams, 0);
      gToneMapManager.setParams(toneMapParams, 1);

      setDirLight(cLCTerrain, terrainDirLightParams);
      setDirLight(cLCUnits, unitDirLightParams);

      setHemiFillLight(cLCTerrain, terrainHemiLightParams);
      setHemiFillLight(cLCUnits, unitHemiLightParams);

      setFogParams(cLCTerrain, fogParams);
      setFogParams(cLCUnits, fogParams);

      gTerrainTexturing.setSpecExponentPower(rawSettings.mTerrainSpecPower);
      gTerrainTexturing.setBumpPower(rawSettings.mTerrainBumpPower);

      gTerrainTexturing.setSpecOnlyDirPower(rawSettings.mTerrainSpecOnlyPower);
      BVector TerrainSpecOnlyColor = srgb8ToLinear(rawSettings.mTerrainSpecOnlyColor);
      gTerrainTexturing.setSpecOnlyDirColor(TerrainSpecOnlyColor.x,TerrainSpecOnlyColor.y,TerrainSpecOnlyColor.z);
      gTerrainTexturing.setSpecOnlyDirShadowAttn(rawSettings.mTerrainSpecOnlyShadowAttn);
      BVector specOnlySunDir;
      mat.makeRotateX(Math::fDegToRad(rawSettings.mTerrainSpecOnlySunInclination));
      mat.multRotateY(Math::fDegToRad(rawSettings.mTerrainSpecOnlySunRotation));   
      mat.transformVector(cYAxisVector, specOnlySunDir);
      gTerrainTexturing.setSpecOnlyDirDirection(specOnlySunDir.x,specOnlySunDir.y,specOnlySunDir.z);

      gTerrainRender.setAODiffuseIntensity(rawSettings.mTerrainAODiffuseIntensity);
      
      //FLS
      setSHFillLight(cLCTerrain, rawSettings.mSHFillLights);
      setSHFillLight(cLCUnits, rawSettings.mSHFillLights);

      gLightEffectManager.setIntensityScale(rawSettings.mLGTIntensityScale);
      gPSManager.setLightBufferIntensityScale(rawSettings.mLGTParticleIntensityScale);   

      XMVECTOR sunParticleIntensityV = XMVectorSplatX(XMLoadScalar(&rawSettings.mSunParticleIntensity));
      XMVECTOR finalSunParticleColor = XMVectorMultiply(sunParticleIntensityV, sunParticleColor);
      gPSManager.setSunColor(finalSunParticleColor);
   }

   return true;
}

//============================================================================
// BSceneLightManager::resetLightSet
//============================================================================
void BSceneLightManager::resetLightSet(void)
{
   for (uint i = 0; i < cNumLightCategories; i++)
   {
      mDirLights[i].clear();
      mFogParams[i].clear();
      mSHFillLights[i].clear();
      mHemiFillLights[i].clear();
   }   
      
   gToneMapManager.setParams(BToneMapParams(), 0);
   gToneMapManager.setParams(BToneMapParams(), 1);
   
   mSHFillLightIntensity = 1.0f;
   mGlobalEnvMap =cInvalidManagedTextureHandle;

}

//============================================================================
// BSceneLightManager::updateLightIntrinsics
//============================================================================
void BSceneLightManager::updateLightIntrinsics(eLightCategory cat)
{
   BDEBUG_ASSERT((cat >= 0) && (cat < cNumLightCategories)); 
   
   mCurLightCat = cat;
   
   XMVECTOR dirToLight(-mDirLights[cat].mDir);
   
   const BOOL enabled = mDirLights[cat].mEnabled;
   
   gEffectIntrinsicManager.set(cIntrinsicDirLightEnabled,         &enabled, cIntrinsicTypeBool);
   gEffectIntrinsicManager.set(cIntrinsicDirLightVecToLightWorld, &dirToLight, cIntrinsicTypeFloat3);
   
   XMVECTOR dirLightColorShadowDarkness(mDirLights[cat].mColor);
   dirLightColorShadowDarkness.w = mDirLights[cat].mShadowDarkness;
   gEffectIntrinsicManager.set(cIntrinsicDirLightColor,           &dirLightColorShadowDarkness, cIntrinsicTypeFloat4);
      
   BSHLightParams shLight(mSHFillLights[cat]);
   shLight.scale(mSHFillLightIntensity);
   shLight.addHemiLight(mHemiFillLights[cat]);
      
   BSHLightShaderConstants shConstants(shLight);
   gEffectIntrinsicManager.set(cIntrinsicSHFillAr, &shConstants.mAr, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicSHFillAg, &shConstants.mAg, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicSHFillAb, &shConstants.mAb, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicSHFillBr, &shConstants.mBr, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicSHFillBg, &shConstants.mBg, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicSHFillBb, &shConstants.mBb, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicSHFillC, &shConstants.mC, cIntrinsicTypeFloat4);
   
   //static float3 gFogColor = float3(.3,.3,.6);
   //static float gFogDensity2 = (5.0f / 1000.0f) * (5.0f / 1000.0f);
   //static float gFogStart2 = 200.0f * 200.0f;  
   //static float3 gPlanarFogColor = float3(.5,.5,.6);
   //static float gPlanarFogDensity2 = (15.0f / 1000.0f) * (15.0f / 1000.0f);
   //static float gPlanarFogStart = -5.0; 
   
#ifdef BUILD_FINAL
   const BFogParams& fogParams = mFogParams[cat];
#else   
   BFogParams fogParams(mFogParams[cat]);
   if (!mFogEnabled)
      fogParams.clear();
#endif      
      
   BVec4 fogParamsVec(
      (fogParams.mZDensity / 1000.0f) * (fogParams.mZDensity / 1000.0f), 
      fogParams.mZStart * fogParams.mZStart,
      (fogParams.mPlanarDensity / 1000.0f) * (fogParams.mPlanarDensity / 1000.0f), 
      fogParams.mPlanarStart);
   
   gEffectIntrinsicManager.set(cIntrinsicFogParams, &fogParamsVec, cIntrinsicTypeFloat4);
   gEffectIntrinsicManager.set(cIntrinsicFogColor, &fogParams.mZColor, cIntrinsicTypeFloat3);
   gEffectIntrinsicManager.set(cIntrinsicPlanarFogColor, &fogParams.mPlanarColor, cIntrinsicTypeFloat3);
   
   const BOOL fogEnabled = (fogParams.mPlanarDensity > 0.0f);
   gEffectIntrinsicManager.set(cIntrinsicPlanarFogEnabled, &fogEnabled, cIntrinsicTypeBool);
}

#ifndef BUILD_FINAL
//============================================================================
// BSceneLightManager::debugHandleInput
//============================================================================
void BSceneLightManager::debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if (event == cInputEventControlStart)
   {
      switch (controlType)
      {
         case cKeyV:
         {
            if ((controlPressed) && (altPressed))
            {
            
            }
            else if ((controlPressed) || (altPressed))
            {
               if (altPressed)
               {
                  mFogEnabled = !mFogEnabled;
                  
                  gToneMapManager.setDOFEnabled(mFogEnabled);
                  
                  gConsoleOutput.status("Fog/DOF %s", mFogEnabled ? "Enabled" : "Disabled");
               }
               else
               {
                  mDebugDisplay = !mDebugDisplay;
                  gConsoleOutput.status("Local light display %s", mDebugDisplay ? "Enabled" : "Disabled");
               }
            }
            break;
         }  
         case cKeyE:
         {
            bool postEffectsEnabled = gToneMapManager.getPostEffectsEnabled();
            postEffectsEnabled = !postEffectsEnabled;
            gToneMapManager.setPostEffectsEnabled(postEffectsEnabled);
            gConsoleOutput.status("Post process effects %s", postEffectsEnabled ? "Enabled" : "Disabled");
         }
      }         
   }         
}

//============================================================================
// BSceneLightManager::debugAddDebugPrims
//============================================================================
void BSceneLightManager::debugAddDebugPrims(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   if (!mDebugDisplay)
      return;

   for (uint activeLightIndex = 0; activeLightIndex < mBaseLocalLightParams.size(); activeLightIndex++)
   {
      if (!mLocalLightEnabled.get(activeLightIndex))
         continue;
         
      const BBaseLocalLightParams& baseParams = mBaseLocalLightParams[activeLightIndex];
      const BLocalLightParams& params = mLocalLightParams[activeLightIndex];

      BFixedString128 text;
      if (params.getType() == cLTOmni)
      {
         gpDebugPrimitives->addDebugSphere(baseParams.getPosRadius(), baseParams.getRadius(), 0xFFFFFFFF);
      }
      else
      {
         XMVECTOR boxMin, boxMax;
         XMMATRIX boxToWorld;

         BVMXIntersection::calculateCappedConeOBB(boxMin, boxMax, boxToWorld, baseParams.getPosRadius(), baseParams.getAtSpotOuter(), baseParams.getSpotOuter() * .5f, baseParams.getRadius());

         XMVECTOR boxCenter = (boxMin + boxMax) * XMVectorReplicate(.5f);
         XMVECTOR boxExtents = boxMax - boxCenter;

         XMMATRIX matrix;
         matrix.r[0] = boxToWorld.r[0];
         matrix.r[1] = boxToWorld.r[1];
         matrix.r[2] = boxToWorld.r[2];
         matrix.r[3] = XMVectorInsert(XMVector3Transform(boxCenter, boxToWorld), XMVectorSplatOne(), 0,  0, 0, 0, 1);

         gpDebugPrimitives->addDebugBox(matrix, boxExtents, 0xFFFF80A0);            
      }
      
      XMVECTOR at = baseParams.getAtSpotOuter();
      XMVECTOR right = baseParams.getRightSpotInner();
      XMVECTOR up = XMVector3Cross(at, right);
      
      gpDebugPrimitives->addDebugAxis(baseParams.getPosRadius(), right, up, at, baseParams.getRadius());

      text.format("%s P:(%3.2f %3.2f %3.2f) R:%2.1f S:%i",
         (params.getType() == cLTOmni) ? "Omni" : "Spot", 
         baseParams.getPosRadius().x, baseParams.getPosRadius().y, baseParams.getPosRadius().z, 
         baseParams.getRadius(),
         mActiveLightShadows.get(activeLightIndex));

      gpDebugPrimitives->addDebugText(text.c_str(), baseParams.getPosRadius(), .4f, (params.getType() == cLTOmni) ? 0xFFFFFF00 : 0xFF00FFFF);
      
      text.format("C:(%3.2f %3.2f %3.2f) In:%2.1f Out:%2.1f\nFAtten:%3.2f Decay:%3.1f Spec: %i LB: %i",
         params.getColor().x, params.getColor().y, params.getColor().z, 
         Math::fRadToDeg(baseParams.getSpotInner()), 
         Math::fRadToDeg(baseParams.getSpotOuter()), 
         params.getFarAttenStart(),
         params.getDecayDist(),
         params.getSpecular(),
         params.getLightBuffered());
      gpDebugPrimitives->addDebugText(text.c_str(), baseParams.getPosRadius() + BVector(0,-1.5,0), .4f, (params.getType() == cLTOmni) ? 0xFFFFFF00 : 0xFF00FFFF);
   }
}

//============================================================================
// BSceneLightManager::debugDraw
//============================================================================
void BSceneLightManager::debugDraw(ATG::Font& font)
{
}
#endif

//============================================================================
// BSceneLightManager::initDeviceData
//============================================================================
void BSceneLightManager::initDeviceData(void)
{
}

//============================================================================
// BSceneLightManager::frameBegin
//============================================================================
void BSceneLightManager::frameBegin(void)
{
}

//============================================================================
// BSceneLightManager::workerUpdateState
//============================================================================
void BSceneLightManager::workerUpdateState(const BUpdateStateData* pStateData)
{
   const void* p = pStateData->mpData;

   p = Utils::readObj(p, mLightGrid);

   p = Utils::readObj(p, mDirLights);
   p = Utils::readObj(p, mFogParams);
   p = Utils::readObj(p, mSHFillLights);
   p = Utils::readObj(p, mHemiFillLights);

   p = BDynamicArraySerializer::deserialize(mBaseLocalLightParams, p);
   p = BDynamicArraySerializer::deserialize(mLocalLightParams, p);
   p = mLocalLightEnabled.deserialize(p);
   p = BDynamicArraySerializer::deserialize(mLocalLightEnabledFrame, p);
   p = mActiveLightShadows.deserialize(p);
   p = BDynamicArraySerializer::deserialize(mActiveLightIndices, p);
   p = Utils::readObj(p, mGlobalEnvMap);
   p = Utils::readObj(p, mSHFillLightIntensity);
   p = Utils::readObj(p, mBackgroundColor);
   p = Utils::readObj(p, mBackgroundIntensity);

   BDEBUG_ASSERT( (uint)((const uchar*)p - pStateData->mpData) == pStateData->mLen);
   
   mFreeLightHandles.resize(0);
   mNextLightID = 0;
   
   check();
}

//============================================================================
// BSceneLightManager::processCommand
//============================================================================
void BSceneLightManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cRenderCommandUpdateState:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BUpdateStateData));
         workerUpdateState(reinterpret_cast<const BUpdateStateData*>(pData));
         break;
      }
   }
}

//============================================================================
// BSceneLightManager::frameEnd
//============================================================================
void BSceneLightManager::frameEnd(void)
{
}

//============================================================================
// BSceneLightManager::deinitDeviceData
//============================================================================
void BSceneLightManager::deinitDeviceData(void)
{
}

//============================================================================
// BSceneLightManager::check
//============================================================================
void BSceneLightManager::check(void)
{
#if 0
#ifdef BUILD_DEBUG
   BDEBUG_ASSERT(mBaseLocalLightParams.getSize() == mLocalLightParams.getSize());
   
   UShortVec results;   
   
   for (uint x = 0; x < mLightGrid.getGridDimension(); x++)
   {
      for (uint z = 0; z < mLightGrid.getGridDimension(); z++)
      {
         BLightGrid::BGridRect gridRect;
         ((uint*)&gridRect)[0] = x;
         ((uint*)&gridRect)[1] = z; 
         ((uint*)&gridRect)[2] = x;
         ((uint*)&gridRect)[3] = z; 
         
         results.resize(0);               
         mLightGrid.getAllObjects(&results, gridRect);
         
         for (uint i = 0; i < results.getSize(); i++)
         {
            BDEBUG_ASSERT(results[i] < getNumActiveLocalLights());
            
            BDEBUG_ASSERT(mLocalLightEnabled.get(results[i]));
            
            BDEBUG_ASSERT(mLocalLightEnabledFrame[results[i]] != (uint)cInvalidLocalLightEnabledFrame);
            
            const BBaseLocalLightParams& baseParams = getActiveLocalLightBaseParams(results[i]);
            
            BLightGrid::BGridRect lightRect = mLightGrid.getGridRect(baseParams.getPosRadius());
            
            const uint* p = (uint*)&lightRect;
            BDEBUG_ASSERT((x >= p[0]) && (z >= p[1]) && (x <= p[2]) && (z <= p[3]));
         }
      }
   }
#endif   
#endif
}

//============================================================================
// BSceneLightManager::updateRenderThreadState
//============================================================================
uint BSceneLightManager::updateRenderThreadState(BSceneLightManager* pDstManager)
{
   ASSERT_THREAD(cThreadIndexSim);

   check();

   const uint totalSize = 
      sizeof(mLightGrid) + 
      (sizeof(mDirLights) + sizeof(mFogParams) + sizeof(mSHFillLights) + sizeof(mHemiFillLights)) +
      BDynamicArraySerializer::getSize(mBaseLocalLightParams) + 
      BDynamicArraySerializer::getSize(mLocalLightParams) + 
      mLocalLightEnabled.getSerializeSize() +
      BDynamicArraySerializer::getSize(mLocalLightEnabledFrame) +
      mActiveLightShadows.getSerializeSize() +
      BDynamicArraySerializer::getSize(mActiveLightIndices) +
      sizeof(mGlobalEnvMap) +
      sizeof(mSHFillLightIntensity) +
      sizeof(mBackgroundColor) +
      sizeof(mBackgroundIntensity);
      
   uchar* pDst = static_cast<uchar*>(gRenderThread.allocateFrameStorage(totalSize));
   
   void* p = pDst;
   
   p = Utils::writeObj(p, mLightGrid);
   
   p = Utils::writeObj(p, mDirLights);
   p = Utils::writeObj(p, mFogParams);
   p = Utils::writeObj(p, mSHFillLights);
   p = Utils::writeObj(p, mHemiFillLights);
   
   p = BDynamicArraySerializer::serialize(mBaseLocalLightParams, p);
   p = BDynamicArraySerializer::serialize(mLocalLightParams, p);
   p = mLocalLightEnabled.serialize(p);
   p = BDynamicArraySerializer::serialize(mLocalLightEnabledFrame, p);
   p = mActiveLightShadows.serialize(p);
   p = BDynamicArraySerializer::serialize(mActiveLightIndices, p);
   p = Utils::writeObj(p, mGlobalEnvMap);
   p = Utils::writeObj(p, mSHFillLightIntensity);
   p = Utils::writeObj(p, mBackgroundColor);
   p = Utils::writeObj(p, mBackgroundIntensity);
   
   BDEBUG_ASSERT( (uint)((uchar*)p - pDst) == totalSize);
   
   gRenderThread.submitCommand(*pDstManager, cRenderCommandUpdateState, BUpdateStateData(totalSize, pDst));
   
   return totalSize;      
}


//============================================================================
// BSceneLightManager::setAnimation
//============================================================================
void BSceneLightManager::setAnimation(const BRawLightSettings& A, const BRawLightSettings& B, DWORD startTime, DWORD duration)
{
   mAnimA = A;
   mAnimB = B;
   mAnimDuration = duration;
   mAnimEndTime = startTime + duration;

}

//============================================================================
// BSceneLightManager::updateAnimation
//============================================================================
void BSceneLightManager::updateAnimation(DWORD gametime)
{
   if (gametime < mAnimEndTime)
   {
      float fract = 1 - ((mAnimEndTime - gametime) / (float)mAnimDuration);      
      setLightSet(BRawLightSettings::lerp(mAnimA, mAnimB, fract), false, false);
   }
}

//============================================================================
// BSceneLightManager::updateAnimation
//============================================================================
void BSceneLightManager::addLightSet(BRawLightSettings* pLightset)
{ 
   mOtherLightsets.add(pLightset); 
}

//============================================================================
// BSceneLightManager::getLightset
//============================================================================
BRawLightSettings* BSceneLightManager::getLightSet(uint id)
{
   for (uint i=0; i< mOtherLightsets.size(); i++)
   {
      if(mOtherLightsets[i]->mId == id)
      {
         return mOtherLightsets[i];
      }
   }

   return NULL;
}


//============================================================================
// BSceneLightManager::clearLightsets
//============================================================================
void BSceneLightManager::clearLightSets()
{   
   mAnimEndTime = 0;
   for (uint i=0; i< mOtherLightsets.size(); i++)
   {
      if(mOtherLightsets[i] != NULL)
      {         
         delete mOtherLightsets[i];
         mOtherLightsets[i] = NULL;
      }
   }  

   mOtherLightsets.clear(); 
}


//============================================================================
// BSceneLightManager::reloadOtherLightsets
//============================================================================
void BSceneLightManager::reloadOtherLightSets()
{
   for (uint i=0; i< mOtherLightsets.size(); i++)
   {
      mOtherLightsets[i]->reload();     
   }   
}