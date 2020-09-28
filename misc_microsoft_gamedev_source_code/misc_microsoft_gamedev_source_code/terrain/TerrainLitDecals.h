//============================================================================
//
//  File: LitDecalManager.h
//
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================
#pragma once
#include "math\generalVector.h"
#include "containers\fixedBlockAllocator.h"
#include "threading\eventDispatcher.h"
#include "math\halfFloat.h"

#include "D3DTextureManager.h"
#include "renderThread.h"
#include "terrainHeightField.h"

class BLitDecalManager;

//============================================================================
// class BLitDecalAttribs
// This class must be bitwise copyable!
//============================================================================
class BLitDecalAttribs
{
   friend class BLitDecalManager;

public:
   enum eBlendMode
   {
      cBlendOpaque = 0,
      cBlendOver = 1,
      cBlendAdditive = 2
   };
   enum eFadeType
   {
      cFTNormal,
      cFTDestroyWhenDone,
      cFTDisableWhenDone,
   };

   BLitDecalAttribs() : mUseCount(0) 
   {
      clear();
   }

   void clear(void)
   {
      mDiffuseTextureHandle = cInvalidManagedTextureHandle;
      mNormalTextureHandle = cInvalidManagedTextureHandle;
      mSpecularTextureHandle = cInvalidManagedTextureHandle;
      mOpacityTextureHandle = cInvalidManagedTextureHandle;
      
      mPos.clear();
      mForwardX = 0.0f;
      mForwardY = 0.0f;
      mForwardZ = 1.0f;
      mSizeX = 1.0f;
      mSizeZ = 1.0f;
      mYOffset = 0.0f;
      mColor = 0xFFFFFFFF;
      mIntensity = 1.0f;

      mU = 0.0f;
      mV = 0.0f;
      mUWidth = 1.0f;
      mVHeight = 1.0f;

      mCategoryIndex = 0;
      mTessLevel = 15;
      mBlendMode = cBlendOpaque;

      mEnabled = true;
      mEnabledForOneFrame = false;      

      mConformToTerrain = true;


      mFadeStartTime = 0.0f;
      mFadeTotalTime = 0.0f;
      mFadeDirection = 0;
      mFadeType = cFTNormal;
      mHoldingUntilFade = true;
      mHoldUntilFadeDuration = 0.0f;
   }

   // The texture handle MUST be owned by the render thread!
   BManagedTextureHandle getDiffuseTextureHandle(void) const { return mDiffuseTextureHandle; }
   void setDiffuseTextureHandle(BManagedTextureHandle textureHandle) { mDiffuseTextureHandle = textureHandle; }

   BManagedTextureHandle getNormalTextureHandle(void) const { return mNormalTextureHandle; }
   void setNormalTextureHandle(BManagedTextureHandle textureHandle) { mNormalTextureHandle = textureHandle; }

   BManagedTextureHandle getSpecularTextureHandle(void) const { return mSpecularTextureHandle; }
   void setSpecularTextureHandle(BManagedTextureHandle textureHandle) { mSpecularTextureHandle = textureHandle; }


   BManagedTextureHandle getOpacityTextureHandle(void) const { return mOpacityTextureHandle; }
   void setOpacityTextureHandle(BManagedTextureHandle textureHandle) { mOpacityTextureHandle = textureHandle; }

   const BVec3& getPos(void) const { return mPos; }
   void setPos(const BVec3& pos) { mPos = pos; }

   float getU(void) const { return mU; }
   float getV(void) const { return mV; }
   float getUWidth(void) const { return mUWidth; }
   float getVHeight(void) const { return mVHeight; }
   void setUV(float u, float v, float width, float height) { mU = u; mV = v; mUWidth = width; mVHeight = height; }

   BVec3 getForward(void) const { return BVec3((float)mForwardX, (float)mForwardY, (float)mForwardZ); }
   void setForward(const BVec3& forward) { mForwardX = forward[0]; mForwardY = forward[1]; mForwardZ = forward[2]; } 

   float getSizeX(void) const { return mSizeX; }
   void setSizeX(float sizeX) { mSizeX = sizeX; updateTessLevel(); }

   float getSizeZ(void) const { return mSizeZ; }
   void setSizeZ(float sizeZ) { mSizeZ = sizeZ; updateTessLevel(); }

   float getYOffset(void) const { return mYOffset; }
   void setYOffset(float yOffset) { mYOffset = yOffset; }

   DWORD getColor(void) const { return mColor; }
   void setColor(DWORD c) { mColor = c; }

   uint getAlpha(void) const { return mColor >> 24; }
   void setAlpha(uint alpha) { mColor &= 0xFFFFFF; mColor |= (alpha << 24); }

   float getIntensity(void) const { return mIntensity; }
   void setIntensity(float intensity) { mIntensity = intensity; }

   uint getCategoryIndex(void) const { return mCategoryIndex; }
   void setCategoryIndex(uint category) { mCategoryIndex = (uchar)category; }

   bool getEnabled(void) const { return mEnabled; }
   void setEnabled(bool enabled) { mEnabled = enabled; }

   void setRenderOneFrame(bool bRenderOneFrame) { mEnabledForOneFrame = bRenderOneFrame; }
   bool getRenderOneFrame(void) const { return mEnabledForOneFrame; }

   uint getUseCount(void) const { return mUseCount; }
   void setUseCount(uint useCount) { mUseCount = useCount; }
   void incUseCount(void) { mUseCount++; }

   uint getTessLevel(void) const { return mTessLevel; }

   bool getConformToTerrain(void) const { return mConformToTerrain; }
   void setConformToTerrain(bool conform) { mConformToTerrain = conform; }

   eBlendMode getBlendMode(void) const { return static_cast<eBlendMode>(mBlendMode); }
   void setBlendMode(eBlendMode blendMode) { mBlendMode = static_cast<uchar>(blendMode); }


   float getFadeStartTime(void) const { return mFadeStartTime; }
   void setFadeStartTime(float time) { mFadeStartTime = time; }

   float getFadeTotalTime(void) const { return mFadeTotalTime; }
   void setFadeTotalTime(float time) { mFadeTotalTime = time; }

   float getHoldUntilFadeTotalTime(void) const { return mHoldUntilFadeDuration; }
   void setHoldUntilFadeTotalTime(float time) { mHoldUntilFadeDuration = time; }

   eFadeType getFadeMode(void) const { return static_cast<eFadeType>(mFadeType); }
   void setFadeMode(eFadeType fadeMode) { mFadeType = static_cast<uchar>(fadeMode); }

   char getFadeDirection(void) const { return mFadeDirection; }
   void setFadeDirection(char dir) { mFadeDirection = dir; }



   bool keyCompare(const BLitDecalAttribs& rhs) const
   {
      const BLitDecalAttribs& lhs = *this;
#define COMPARE(v) if (lhs.##v < rhs.##v) return true; else if (lhs.##v > rhs.v##) return false;          
      COMPARE(mCategoryIndex);
      COMPARE(mBlendMode);
      COMPARE(mDiffuseTextureHandle);
      COMPARE(mNormalTextureHandle);
      COMPARE(mOpacityTextureHandle);
      COMPARE(mSpecularTextureHandle);
      COMPARE(mConformToTerrain)
         COMPARE(mTessLevel);
#undef COMPARE
      return false;
   }

private:   
   BManagedTextureHandle mDiffuseTextureHandle;   
   BManagedTextureHandle mNormalTextureHandle;   
   BManagedTextureHandle mSpecularTextureHandle;   
   BManagedTextureHandle mOpacityTextureHandle;   
   
   

   BVec3 mPos;   
   BHalfFloat mForwardX;
   BHalfFloat mForwardY;
   BHalfFloat mForwardZ;
   BHalfFloat mSizeX;
   BHalfFloat mSizeZ;
   BHalfFloat mYOffset;
   BHalfFloat mU;
   BHalfFloat mV;
   BHalfFloat mUWidth;
   BHalfFloat mVHeight;
   BHalfFloat mHoldUntilFadeDuration;
   BHalfFloat mFadeStartTime;
   BHalfFloat mFadeTotalTime;

   DWORD mColor;
   BHalfFloat mIntensity;

   uint mUseCount;

   uchar mCategoryIndex;
   uchar mTessLevel;
   uchar mBlendMode;
   uchar mFadeType;
   char  mFadeDirection;

   bool mConformToTerrain : 1;
   bool mEnabled : 1;
   bool mEnabledForOneFrame : 1;
   bool mHoldingUntilFade : 1;


   

   

  

   

  
  


   void setDirty(void) { }

   void updateTessLevel(void)
   {
      BHalfFloat maxSize = Math::Max<ushort>(mSizeX.getBits(), mSizeZ.getBits());
      float fMaxSize = (float)maxSize;
      mTessLevel = (uchar)Math::Clamp<int>(Math::FloatToIntRound(fMaxSize), 1, 15);
   }
};

class BLitDecalKey : public BBitHashable<BLitDecalKey>
{
public:
   BLitDecalKey() { }

   BLitDecalKey(int xx, int yy, int zz) { set (xx, yy, zz); }

   void set(int xx, int yy, int zz) { x = static_cast<int16>(xx); y = static_cast<int16>(yy); z = static_cast<int16>(zz); dummy = 0; }

   bool operator==(const BLitDecalKey& rhs) const { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }

   int16 x;
   int16 y;
   int16 z;
   int16 dummy; // not use/for alignment
};


typedef int BLitDecalHandle;
const int cInvalidLitDecalHandle = -1;

#pragma warning(push)
#pragma warning(disable:4324)
//============================================================================
// class BLitDecalManager
//============================================================================
class BLitDecalManager : public BRenderCommandListener
{
   friend class BLitDecalAttribs;

public:
   enum 
   {
      eVoxelXDim = 16,
      eVoxelYDim = 40,
      eVoxelZDim = 16,

      eMaxNumPerVoxel = 4
   };

   // Sim thread
   BLitDecalManager();
   ~BLitDecalManager();

   void init(void);
   void deinit(void);

   void destroy(void);

   // Returns cInvalidLitDecalHandle if allocation failed.
   void createLitDecal(const BLitDecalAttribs &data);
   void updateRenderThread(double gameTime);   


   // Render thread
   void renderDrawLitDecalsBegin(void);
   void renderDrawLitDecalsBeginTile(uint tileIndex);

   enum eRenderFilter
   {
      cRFAll,
      cRFConformToTerrain,
      cRFNonConformToTerrain
   };

   void renderSetZBiases(float nonconform, float conform) { mNonConformZBias = nonconform; mConformZBias = conform; }

   void renderDrawAlphaLitDecals(uint tileIndex, int categoryIndex = -1, eRenderFilter filter = cRFAll, bool stencilTestWhenConforming = false);

   void renderDrawLitDecalsEndTile(uint tileIndex);
   void renderDrawLitDecalsEnd(void);



private:
   enum { cMaxLitDecals = 2048 };
   typedef BFixedBlockAllocator<sizeof(BLitDecalAttribs), cMaxLitDecals> BLitDecalAllocator;

   __declspec(align(16)) BLitDecalAllocator mRenderLitDecalAttribs;



  
   float mRenderGameTime; 

   float mNonConformZBias;
   float mConformZBias;

   BDynamicRenderArray<ushort> mRenderValidAlphaLitDecals;


   BDynamicRenderArray<BTerrainHeightField::BPatchInstance> mRenderPatchInstances;

   BSceneLightManager::BActiveLightIndexArray mCurVisibleLightIndices;

   bool mRenderBegunDrawing : 1;

   enum 
   {
      cECDoneFading = cEventClassFirstUser,
   };

   enum 
   {
      cRCDestroy=0,
      cRCUpdate,
      cRCBeginFade,
      cRCCreateDecal,
   };



   class BLitDecalHashBucket
   {
      public:
         BLitDecalHashBucket() {};
        ~BLitDecalHashBucket() { clear(); };
         void clear() { mValues.clear(); };

         BLitDecalHashBucket& operator=(const BLitDecalHashBucket& rhs)
         {
            if (this == &rhs)
               return *this;

            mValues = rhs.mValues;
            return *this;
         }

         BSmallDynamicRenderArray<BLitDecalHandle> mValues;
   };

   typedef BHashMap<BLitDecalKey, BLitDecalHashBucket, BHasher<BLitDecalKey>, BEqualTo<BLitDecalKey>, true, BRenderFixedHeapAllocator> BLitDecalGridHashMap;      
   BLitDecalGridHashMap mHashGrid;
   void initKey(const BVec3& pos, BLitDecalKey& key) const;

   //============================================================================
   // struct BLitDecalAttribAlphaKeySorter
   //============================================================================
   struct BLitDecalAttribAlphaKeySorter
   {
      BLitDecalManager& mLitDecalManager;
      BLitDecalAttribAlphaKeySorter(BLitDecalManager& LitDecalManager) : mLitDecalManager(LitDecalManager) { }

      bool operator() (uint i, uint j) const;
   };    

   void renderUpdate(float pUpdateData);
   bool renderTickLitDecal(BLitDecalHandle handle, BLitDecalAttribs& attribs);
   void renderDrawSortedLitDecals(const BLitDecalHashBucket*  hashBucket, BVec3 minBounds, BVec3 maxBounds, uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming, bool categoryEarlyOut, int pass);
   void renderDrawLitDecalRange(const BDynamicRenderArray<ushort>& sortedIndices, int firstSortedLitDecalIndex, int lastSortedLitDecalIndex, bool stencilTestWhenConforming);
   void renderCreateDecal(const BLitDecalAttribs& data);

   void destroyLitDecal(BLitDecalHandle handle);
   BLitDecalAttribs* getLitDecal(BLitDecalHandle handle);
   void destroyAllLitDecals();

   void setVisControlRegs();


   int setupLocalLighting(BVec3 min, BVec3 max);

   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};
#pragma warning(pop)

//============================================================================
// Globals
//============================================================================
extern BLitDecalManager gLitDecalManager;