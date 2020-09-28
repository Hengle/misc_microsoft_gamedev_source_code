//============================================================================
//
// File: sceneLightManager.h
//  
// Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "containers\bitArray2D.h"
#include "containers\staticArray.h"
#include "containers\linkedArray.h"
#include "math\generalVector.h"

#include "gridCuller.h"
#include "broadPhase2D.h"
#include "renderThread.h"
#include "xmlreader.h"
#include "ATGFont.h"

#include "tonemapManager.h"
#include "math\VMXUtils.h"

const uint cMaxExpectedLocalLights = 1024;
const float cMaxLocalLightRadius = 96.0f;

typedef uint64 BLocalLightGUID;

struct BHemiLightParams;

//============================================================================
// struct BDirLightParams
//============================================================================
struct BDirLightParams
{
   XMVECTOR mDir;
   XMVECTOR mColor;
   float    mShadowDarkness;
   bool     mEnabled : 1;
   bool     mShadows : 1;
         
   void clear(void);
};

//============================================================================
// struct BFogParams
//============================================================================
struct BFogParams
{
   BVec3 mZColor;
   float mZStart;
   float mZDensity;
   
   BVec3 mPlanarColor;
   float mPlanarStart;
   float mPlanarDensity;
   
   void clear(void) 
   { 
      Utils::ClearObj(*this);
      
      mZStart = 99999.0f;
      mZDensity = 0.0f;
   }
};

typedef BVecN<9> BSpectralSHCoefficients[3];

//============================================================================
// struct BSHLightParams
//============================================================================
class BSHLightParams
{
public:
   BSpectralSHCoefficients mSHCoeffs;
   
   void clear(void);
   
   // shCoeffs are the SH coefficients of a light probe.
   // Don't premultiply the SH coefficients by the cosine lobe! This function does
   // that, as well as divides by Pi, implicitly.
   void set(const BSpectralSHCoefficients& shCoeffs);
   
   void addHemiLight(const BHemiLightParams& h);
   
   void scale(float s);
   
   bool load(long dirID, const char* pFilename);
   bool load(BXMLNode root);
   
   bool save(long dirID, const char* pFilename) const; 
};

//============================================================================
// struct BSHLightShaderConstants
//============================================================================
class BSHLightShaderConstants
{
public:
   XMVECTOR mAr;
   XMVECTOR mAg; 
   XMVECTOR mAb;
   XMVECTOR mBr;
   XMVECTOR mBg;
   XMVECTOR mBb;
   XMVECTOR mC;
   
   BSHLightShaderConstants() { }
   
   BSHLightShaderConstants(const BSHLightParams& p) { set(p); }

   void clear(void);
   
   void set(const BSHLightParams& p);
};

//============================================================================
// struct BHemiLightParams
//============================================================================
struct BHemiLightParams
{
   XMVECTOR mAxis;
   XMVECTOR mTopColor;
   XMVECTOR mBottomColor;
   
   void clear(void);
};


//============================================================================
// enum eLocalLightType
//============================================================================
enum eLocalLightType
{
   cInvalidLightType = -1,

   cLTOmni,
   cLTSpot,

   cNumLightTypes
};

typedef uint BLocalLightHandle;
const BLocalLightHandle cInvalidLocalLightHandle = (BLocalLightHandle)0;

//============================================================================
// class BBaseLocalLightParams
//============================================================================
class BBaseLocalLightParams 
{
   friend class BSceneLightManager;

public:
   void clear(void);
   
   void enforceLimits(void);
   
   // pos in xyz, radius in w
   XMVECTOR getPosRadius(void) const { return mPosRadius; }
   void setPosRadius(XMVECTOR posRadius) { mPosRadius = posRadius; }
   
   XMVECTOR getPos(void) const { return __vrlimi(mPosRadius, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0); }
         
   float getRadius(void) const { return mPosRadius.w; }
   void setRadius(float radius) { mPosRadius.w = Math::Min<float>(cMaxLocalLightRadius, radius); }
   
   XMVECTOR getAtSpotOuter(void) const { return XMLoadHalf4(&mAtSpotOuter); }
   void setAtSpotOuter(XMVECTOR atSpotOuter) { XMStoreHalf4(&mAtSpotOuter, atSpotOuter); }
   
   float getSpotOuter(void) const { return XMConvertHalfToFloat(mAtSpotOuter.w); }
   void setSpotOuter(float spotOuter) { mAtSpotOuter.w = XMConvertFloatToHalf(spotOuter); }
   
   XMVECTOR getRightSpotInner(void) const { return XMLoadHalf4(&mRightSpotInner); }
   void setRightSpotInner(XMVECTOR rightSpotInner) { XMStoreHalf4(&mRightSpotInner, rightSpotInner); }
   
   float getSpotInner(void) const { return XMConvertHalfToFloat(mRightSpotInner.w); }
   void setSpotInner(float spotInner) { mRightSpotInner.w = XMConvertFloatToHalf(spotInner); }
   
   bool isSpotLight(void) const { return mAtSpotOuter.w != 0; }

private:                  
   XMVECTOR mPosRadius; 
   XMHALF4 mAtSpotOuter;
   XMHALF4 mRightSpotInner;
};

const uint cInvalidLocalLightID = 0;

//============================================================================
// class BLocalLightParams
//============================================================================
#pragma warning(push)
#pragma warning(disable:4324)
__declspec(align(16))
class BLocalLightParams 
{
   friend class BSceneLightManager;
      
public:
   BLocalLightParams();
   
   void clear(void);
   
   void enforceLimits(void);
         
   eLocalLightType getType(void) const { return mType; }
   void setType(eLocalLightType type) { mType = type; }
      
   // xyz = linear light color*intensity
   void setColor(XMVECTOR color) { XMStoreHalf4(&mColor, color); }
   XMVECTOR getColor(void) const { return XMLoadHalf4(&mColor); }
   
   // [0,.999]
   void setFarAttenStart(float farAttenStart) { mOmniFarAttenStart = XMConvertFloatToHalf(farAttenStart); }
   float getFarAttenStart(void) const { return XMConvertHalfToFloat(mOmniFarAttenStart); }
   
   // [.01,5000], may be > than radius
   void setDecayDist(float decayDist) { mOmniDecayDist = XMConvertFloatToHalf(decayDist); }
   float getDecayDist(void) const { return XMConvertHalfToFloat(mOmniDecayDist); }
         
   // relative light priority, [-128,127]   
   void setPriority(int priority) { mPriority = (char)priority; }
   int getPriority(void) const { return mPriority; }
   
   void setSpecular(bool specular) { mSpecular = specular; }
   bool getSpecular(void) const { return mSpecular; }
   
   void setFogged(bool fogged) { mFogged = fogged; }
   bool getFogged(void) const { return mFogged; }
   
   void setFoggedShadows(bool foggedShadows) { mFoggedShadows = foggedShadows; }
   bool getFoggedShadows(void) const { return mFoggedShadows; }
         
   // [0,1] - 0=darkest
   void setShadowDarkness(float shadowDarkness) { mShadowDarkness = XMConvertFloatToHalf(shadowDarkness); }
   float getShadowDarkness(void) const { return XMConvertHalfToFloat(mShadowDarkness); }
   
   float getShadowIndex(void) const { return XMConvertHalfToFloat(mShadowIndex); }
   void setShadowIndex(float shadowIndex) { mShadowIndex = XMConvertFloatToHalf(shadowIndex); }
   void setInvalidShadowIndex(void) { setShadowIndex(-1.0f); }
   bool isValidShadowIndex(void) const { return mShadowIndex != 0xBC00; } // 0xBC00 is -1.0 in float16
         
   XMVECTOR getShadowMatrixCol(uint column) const { BDEBUG_ASSERT(column < 3); return mShadowCol[column]; }
   void setShadowMatrixCol(uint column, XMVECTOR v) { BDEBUG_ASSERT(column < 3); mShadowCol[column] = v; }
   
   // Unique 32-bit light ID's - all values valid except for cInvalidLocalLightID.
   DWORD getLightID(void) const { return mLightID; }
   void setLightID(DWORD lightID) { mLightID = lightID; }
   
   // The shadow UV bounds index corresponds to a table in the pixel shader of possible shadow map UV bounds.
   void setShadowUVBoundsIndex(uint uvBoundsIndex) { mShadowUVBoundsIndex = (char)uvBoundsIndex; }
   uint getShadowUVBoundsIndex(void) const { return mShadowUVBoundsIndex; }
   
   BLocalLightGUID getLightGUID(void) const { return static_cast<uint64>(mHandle) | (static_cast<uint64>(mLightID) << 32U); }
   static BLocalLightHandle getLocalLightHandleFromGUID(BLocalLightGUID lightGUID) { return static_cast<BLocalLightHandle>(static_cast<uint>(lightGUID)); }
   
   BLocalLightHandle getLocalLightHandle(void) const { return mHandle; }
   
   bool getLightBuffered(void) const { return mLightBuffered; }
   void setLightBuffered(bool lightBuffered) { mLightBuffered = lightBuffered; }
                  
private:
   XMVECTOR          mShadowCol[3];
   
   XMHALF4           mColor;
      
   BLocalLightHandle mHandle;
   
   DWORD             mLightID;
      
   HALF              mOmniFarAttenStart;
   HALF              mOmniDecayDist;
   
   HALF              mShadowDarkness;
   HALF              mShadowIndex;
            
   char              mPriority;
   char              mShadowUVBoundsIndex;

   bool              mSpecular       : 1;
   bool              mFogged         : 1;
   bool              mFoggedShadows  : 1;
   bool              mLightBuffered  : 1;

   eLocalLightType   mType;
};
#pragma warning(pop)

//============================================================================
// enum eLightCategory
//============================================================================
enum eLightCategory
{
   cLCFirstCategory = 0,
   
   cLCTerrain = 0,
   cLCUnits = 1,
   cLCUI = 2,

   cNumLightCategories
};



//============================================================================
// struct BRawLightSettings
//============================================================================
class BRawLightSettings
{
public:
   uint mId;

   float mSunInclination;
   float mSunRotation;
   
   BVector mSunUnitColor;
   BVector mSunTerrainColor;
   BVector mSunParticleColor;
   float mSunUnitIntensity;
   float mSunTerrainIntensity;
   float mSunParticleIntensity;
   float mSunUnitShadowDarkness;
   float mSunTerrainShadowDarkness;
   bool mSunShadows;
   bool mSunUnitsEnabled;
   bool mSunTerrainEnabled;
   
   float mHemiInclination;
   float mHemiRotation;
   BVector mHemiUnitTopColor;
   BVector mHemiUnitBottomColor;
   BVector mHemiTerrainTopColor;
   BVector mHemiTerrainBottomColor;
   float mHemiUnitIntensity;
   float mHemiTerrainIntensity;
   
   BToneMapParams mToneMapParams;
   
   BVector mZFogColor;
   float mZFogIntensity;
   float mZFogStart;
   float mZFogDensity;
   
   BVector mPlanarFogColor;
   float mPlanarFogIntensity;
   float mPlanarFogStart;
   float mPlanarFogDensity;

   float mTerrainSpecPower;
   float mTerrainBumpPower;
   float mTerrainAODiffuseIntensity;
   BVector mTerrainSpecOnlyColor;
   float mTerrainSpecOnlyPower;
   float mTerrainSpecOnlyShadowAttn;
   float mTerrainSpecOnlySunInclination;
   float mTerrainSpecOnlySunRotation;

   
   BString mGlobalEnvMapName;

   float mShFillLightIntensity;
   BVec3 mBackgroundColor;
   float mBackgroundIntensity;
   
   float mDofNearRange;
   float mDofFarRange;

   BString mFilename;

   BSHLightParams mSHFillLights;
   
   float mLGTIntensityScale;
   float mLGTParticleIntensityScale;

   void reload();
   bool loadLightSet(BString filename, BXMLNode& root);
   static BRawLightSettings lerp(BRawLightSettings a, BRawLightSettings b, float amount);
};

//============================================================================
// Class BSceneLightManager
//============================================================================
#pragma warning(push)
#pragma warning(disable:4324)
__declspec(align(128))
class BSceneLightManager : public BRenderCommandListener
{
public:
   BSceneLightManager();
   ~BSceneLightManager();
      
   void init(void);
   void deinit(void);
   
   bool loadLightSet(BXMLNode root, bool uiSetOnly = false);
   bool loadLightSet(BXMLNode glsRoot, BXMLNode flsRoot, bool uiSetOnly = false);
   bool setLightSet(const BRawLightSettings& rawSettings, bool uiSetOnly = false, bool updateTextures = true);

   void resetLightSet(void);
   
   uint updateRenderThreadState(BSceneLightManager* pDstManager);
      
   void updateLightIntrinsics(eLightCategory cat);
   eLightCategory getCurLightCategory(void) const { return mCurLightCat; }

   void resetLightParams(eLightCategory lightIndex);      
   
   const BDirLightParams& getDirLight(eLightCategory lightIndex) const;
   void setDirLight(eLightCategory lightIndex, const BDirLightParams& params);
   
   const BFogParams& getFogParams(eLightCategory lightIndex) const;
   void setFogParams(eLightCategory lightIndex, const BFogParams& params);
      
   const BSHLightParams& getSHFillLight(eLightCategory lightIndex) const;
   void setSHFillLight(eLightCategory lightIndex, const BSHLightParams& shParams);
   
   // Resets all SH fill lights to zeros.
   void resetSHFillLights(void);
         
   const BHemiLightParams& getHemiFillLight(eLightCategory lightIndex) const;
   void setHemiFillLight(eLightCategory lightIndex, const BHemiLightParams& shParams);
      
   BLocalLightHandle createLocalLight(void);
   void freeLocalLight(BLocalLightHandle handle);
   
   void freeAllLocalLights(void);
   
   void enforceLimits(BLocalLightHandle handle);
   
   typedef uint BActiveLightIndex;
   BActiveLightIndex getActiveLightIndex(BLocalLightHandle handle) const;
      
   const BLocalLightParams& getLocalLightParams(BLocalLightHandle handle) const;
         BLocalLightParams& getLocalLightParams(BLocalLightHandle handle);
         
   XMVECTOR getLocalLightPosRadius(BLocalLightHandle handle) const;
   float getLocalLightRadius(BLocalLightHandle handle) const;
   void setLocalLightPosRadius(BLocalLightHandle handle, XMVECTOR posRadius);
   void setLocalLightPos(BLocalLightHandle handle, const XMFLOAT3* RESTRICT pPos);
   void setLocalLightRadius(BLocalLightHandle handle, float radius);
   
   XMVECTOR getLocalLightAtSpotOuter(BLocalLightHandle handle) const;
   void setLocalLightAtSpotOuter(BLocalLightHandle handle, XMVECTOR atSpotOuter);
   
   XMVECTOR getLocalLightRightSpotInner(BLocalLightHandle handle) const;
   void setLocalLightRightSpotInner(BLocalLightHandle handle, XMVECTOR rightSpotInner);
   
   void setLocalLightAt(BLocalLightHandle handle, const XMFLOAT3* RESTRICT pAt);
   void setLocalLightRight(BLocalLightHandle handle, const XMFLOAT3* RESTRICT pAt);
         
   float getLocalLightSpotOuter(BLocalLightHandle handle) const;
   void setLocalLightSpotOuter(BLocalLightHandle handle, float spotOuter);
         
   float getLocalLightSpotInner(BLocalLightHandle handle) const;
   void setLocalLightSpotInner(BLocalLightHandle handle, float spotInner);
                              
   void setLocalLightEnabled(BLocalLightHandle handle, bool enable);
   bool getLocalLightEnabled(BLocalLightHandle handle) const;
         
   void setLocalLightShadows(BLocalLightHandle handle, bool enable);
   bool getLocalLightShadows(BLocalLightHandle handle);
         
   typedef BDynamicArray<BBaseLocalLightParams, 16> BBaseLocalLightParamsArray;
   const BBaseLocalLightParamsArray& getBaseLocalLightParams(void) const { return mBaseLocalLightParams; }
   
   typedef BDynamicArray<BLocalLightParams, 64> BLocalLightParamsArray;
   const BLocalLightParamsArray& getLocalLightParams(void) const { return mLocalLightParams; }
   
   uint getNumActiveLocalLights(void) const { return mBaseLocalLightParams.size(); }
   const BBaseLocalLightParams& getActiveLocalLightBaseParams(BActiveLightIndex activeLightIndex) const { return mBaseLocalLightParams[activeLightIndex]; }
   const BLocalLightParams& getActiveLocalLightParams(BActiveLightIndex activeLightIndex) const { return mLocalLightParams[activeLightIndex]; }
         BLocalLightParams& getActiveLocalLightParams(BActiveLightIndex activeLightIndex)       { return mLocalLightParams[activeLightIndex]; }
      
   enum { cInvalidLocalLightEnabledFrame = 0xFFFFFFFF };
   // Returns cInvalidLocalLightEnabledFrame, or the last frame the light was enabled (as measured by gRenderThread.getCurMainFrameNoBarrier()).
   uint getActiveLocalLightEnabledFrame(BActiveLightIndex activeLightIndex);
   
   bool getActiveLightShadows(BActiveLightIndex activeLightIndex) const;
   
   enum { cBroadPhaseMaxWorldCoord = 1024, cBroadPhaseCellDim = 32, cBroadPhaseMaxCellsPerAxis = cBroadPhaseMaxWorldCoord / cBroadPhaseCellDim };
   typedef BBroadPhase2D<cMaxExpectedLocalLights, cBroadPhaseMaxCellsPerAxis> BLightGrid;   
   const BLightGrid& getLightGrid(void) const   { return mLightGrid; }
         BLightGrid& getLightGrid(void)         { return mLightGrid; }
   
   typedef BLightGrid::BGridRect BGridRect;
   BGridRect getGridRect(XMVECTOR min, XMVECTOR max) const { return mLightGrid.getGridRect(min, max); }
         
   // Returns active light indices by default, unless visibleLightIndices is set.
   typedef BStaticArray<WORD, cMaxExpectedLocalLights, false> BActiveLightIndexArray;
   void findLights(BActiveLightIndexArray& activeLightIndices, BGridRect gridRect, XMVECTOR min, XMVECTOR max, bool refine = true);
   
   void lockLights(void) { mLightsLocked = true; }
   void unlockLights(void) { mLightsLocked = false; }
   bool getLightsLocked(void) const { return mLightsLocked; }
   
   const BVec3& getBackgroundColor(void) const { return mBackgroundColor; }
   float getBackgroundIntensity(void) const { return mBackgroundIntensity; }
   
#ifndef BUILD_FINAL
   void debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed);
   void debugAddDebugPrims(void);
   void debugDraw(ATG::Font& font);
#endif   

   BManagedTextureHandle getGlobalEnvMap(void) const { return mGlobalEnvMap; }
   void setGlobalEnvMap(BManagedTextureHandle handle) { mGlobalEnvMap = handle; }
           
   void setAnimation(const BRawLightSettings& A, const BRawLightSettings& B,  DWORD startTime, DWORD duration);//setAnimation(BRawLightSettings A, BRawLightSettings B, float time);
   void updateAnimation(DWORD gametime);
   
   const BRawLightSettings& getCurrentLightSet(void) const { return mCurrentRawSettings; }
   void addLightSet(BRawLightSettings* pLightset);
   BRawLightSettings* getLightSet(uint id);
   void clearLightSets();
   void reloadOtherLightSets();
      
private:
   BLightGrid                    mLightGrid;
      
   BDirLightParams               mDirLights[cNumLightCategories];
   BFogParams                    mFogParams[cNumLightCategories];
   BSHLightParams                mSHFillLights[cNumLightCategories];
   BHemiLightParams              mHemiFillLights[cNumLightCategories];
      
   BBaseLocalLightParamsArray    mBaseLocalLightParams;
     
   BLocalLightParamsArray        mLocalLightParams;
         
   BBitArray2D<1>                mLocalLightEnabled;
   UIntVec                       mLocalLightEnabledFrame;
   
   BBitArray2D<1>                mActiveLightShadows;
         
   UShortVec                     mActiveLightIndices;
   
   UShortVec                     mFreeLightHandles;
   DWORD                         mNextLightID;
      
   BManagedTextureHandle         mGlobalEnvMap;
   float                         mSHFillLightIntensity;
   
   BVec3                         mBackgroundColor;
   float                         mBackgroundIntensity;
   
   BRawLightSettings             mAnimA;
   BRawLightSettings             mAnimB;
   BRawLightSettings             mCurrentRawSettings;
   DWORD                         mAnimDuration;
   DWORD                         mAnimEndTime;
   BDynamicArray<BRawLightSettings*> mOtherLightsets;
            
   bool                          mLightsLocked : 1;
   bool                          mInitialized : 1;
#ifndef BUILD_FINAL   
   bool                          mDebugDisplay : 1;
#endif
   bool                          mFogEnabled : 1;

   eLightCategory                mCurLightCat;

   void clear(void);
   uint getSlotFromHandle(BLocalLightHandle handle) const;
   
   enum 
   {
      cRenderCommandUpdateState
   };
   
   class BUpdateStateData
   {
   public:
      BUpdateStateData(uint len, const uchar* pData) : mLen(len), mpData(pData) { }

      uint mLen;
      const uchar* mpData;
   };
   
   void check(void);      
   void workerUpdateState(const BUpdateStateData* pStateData);
   
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};
#pragma warning(pop)

//============================================================================
// Externs
//============================================================================\

// Sim thread's scene light manager.
extern BSceneLightManager gSimSceneLightManager;

// Render thread's scene light manager. This object is a direct copy of the sim thread's manager.
extern BSceneLightManager gRenderSceneLightManager;
