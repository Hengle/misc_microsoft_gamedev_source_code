//============================================================================
//
//  File: decalManager.h
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

class BDecalManager;

//============================================================================
// class BDecalAttribs
// This class must be bitwise copyable!
//============================================================================
class BDecalAttribs
{
   friend class BDecalManager;
   
public:
   enum eBlendMode
   {
      cBlendOpaque = 0,
      cBlendOver = 1,
      cBlendAdditive = 2,
      cBlendStencil = 3,
   };
   
   BDecalAttribs() : mUseCount(0) 
   {
      clear();
   }
         
   void clear(void)
   {
      mTextureHandle = cInvalidManagedTextureHandle;
      mStencilTextureHandle = cInvalidManagedTextureHandle;
      mFlashMovieIndex = -1;      
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
   }
   
   // The texture handle MUST be owned by the render thread!
   BManagedTextureHandle getTextureHandle(void) const { return mTextureHandle; }
   void setTextureHandle(BManagedTextureHandle textureHandle) { mTextureHandle = textureHandle; }

   BManagedTextureHandle getStencilTextureHandle(void) const { return mStencilTextureHandle; }
   void setStencilTextureHandle(BManagedTextureHandle textureHandle) { mStencilTextureHandle = textureHandle; }
   
   int getFlashMovieIndex(void) const { return mFlashMovieIndex; }
   void setFlashMovieIndex(int index) { mFlashMovieIndex = index; }
   
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
               
   bool keyCompare(const BDecalAttribs& rhs) const
   {
      const BDecalAttribs& lhs = *this;
#define COMPARE(v) if (lhs.##v < rhs.##v) return true; else if (lhs.##v > rhs.v##) return false;          
      COMPARE(mCategoryIndex);
      COMPARE(mBlendMode);
      COMPARE(mTextureHandle);
      COMPARE(mStencilTextureHandle);
      COMPARE(mConformToTerrain)
      COMPARE(mTessLevel);
#undef COMPARE
      return false;
   }
            
private:   
   BManagedTextureHandle mTextureHandle;   
   BManagedTextureHandle mStencilTextureHandle;
   
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
   
   DWORD mColor;
   BHalfFloat mIntensity;
      
   int  mFlashMovieIndex;
   uint mUseCount;
                           
   uchar mCategoryIndex;
   uchar mTessLevel;
   uchar mBlendMode;
         
   bool mConformToTerrain : 1;
   bool mEnabled : 1;
   bool mEnabledForOneFrame : 1;
            
   void setDirty(void) { }
   
   void updateTessLevel(void)
   {
      BHalfFloat maxSize = Math::Max<ushort>(mSizeX.getBits(), mSizeZ.getBits());
      float fMaxSize = (float)maxSize;
      mTessLevel = (uchar)Math::Clamp<int>(Math::FloatToIntRound(fMaxSize), 1, 15);
   }
};

typedef int BDecalHandle;
const int cInvalidDecalHandle = -1;

#pragma warning(push)
#pragma warning(disable:4324)
//============================================================================
// class BDecalManager
//============================================================================
class BDecalManager : public BRenderCommandListener, public BEventReceiver
{
   friend class BDecalAttribs;
   
public:
   // Sim thread
   BDecalManager();
   ~BDecalManager();
   
   void init(void);
   void deinit(void);
   
   // Returns cInvalidDecalHandle if allocation failed.
   BDecalHandle createDecal(void);
   void destroyDecal(BDecalHandle handle);
   BDecalAttribs* getDecal(BDecalHandle handle);
   
   void destroyAllDecals();
   void destroyDecalsWithCatagoryIndex(uint catIndex);

   enum eFadeType
   {
      cFTNormal,
      cFTDestroyWhenDone,
      cFTDisableWhenDone,
   };
   
   void fadeDecal(BDecalHandle handle, double curGameTime, float fadeOutTime, int fadeDirection, eFadeType fadeType);
   void holdUntilFadeDecal(BDecalHandle handle, double curGameTime, float holdUntilFadeTime, float fadeOutTime, int fadeDirection, eFadeType fadeType);
   
   void updateRenderThread(double gameTime);   
   
   // Render thread
   void renderDrawDecalsBegin(void);
   void renderDrawDecalsBeginTile(uint tileIndex);
   
   enum eRenderFilter
   {
      cRFAll,
      cRFConformToTerrain,
      cRFNonConformToTerrain
   };
   
   void renderSetZBiases(float nonconform, float conform) { mNonConformZBias = nonconform; mConformZBias = conform; }
   
   void renderDrawNormalDecals(uint tileIndex, int categoryIndex = -1, eRenderFilter filter = cRFAll, bool stencilTestWhenConforming = false);
   void renderDrawAlphaDecals(uint tileIndex, int categoryIndex = -1, eRenderFilter filter = cRFAll, bool stencilTestWhenConforming = false);
   void renderDrawStencilDecals(uint tileIndex, int categoryIndex = -1, eRenderFilter filter = cRFAll, bool stencilTestWhenConforming = false);
   
   void renderDrawDecalsEndTile(uint tileIndex);
   void renderDrawDecalsEnd(void);

private:
   enum { cMaxDecals = 2048 };
   typedef BFixedBlockAllocator<sizeof(BDecalAttribs), cMaxDecals> BDecalAllocator;
   
   __declspec(align(16)) BDecalAllocator mSimDecalAttribs;
   __declspec(align(16)) BDecalAllocator mRenderDecalAttribs;
                     
   struct BDecalState
   {
      BDecalState() : mUseCount(0xFFFFFFFF)
      {
         clear();
      }
      
      void clear(void)
      {
         mFadeStartTime = 0.0f;
         mFadeTotalTime = 0.0f;
         mFadeDirection = 0;
         mFading = false;
         mSentDoneFadingEvent = false;
         mFadeType = cFTNormal;
         mHoldingUntilFade = false;
         mHoldUntilFadeDuration = 0.0f;
      }
      
      
      
      float mHoldUntilFadeDuration;

      uint mUseCount;
      
      double mFadeStartTime;
      float mFadeTotalTime;
      
      char mFadeDirection;
      
      eFadeType mFadeType;
      
      bool mFading : 1;
      bool mSentDoneFadingEvent : 1;
      bool mHoldingUntilFade : 1;
   };
   
   BDynamicRenderArray<BDecalState> mRenderDecalState;   
         
   double mRenderGameTime; 
   
   float mNonConformZBias;
   float mConformZBias;
   
   BDynamicRenderArray<ushort> mRenderValidNormalDecals;
   BDynamicRenderArray<ushort> mRenderValidAlphaDecals;
   BDynamicRenderArray<ushort> mRenderValidStencilDecals;

               
   BDynamicRenderArray<BTerrainHeightField::BPatchInstance> mRenderPatchInstances;
   
   bool mRenderBegunDrawing : 1;
   
   enum 
   {
      cECDoneFading = cEventClassFirstUser,
   };
   
   enum 
   {
      cRCUpdate,
      
      cRCBeginFade,
   };
   
   //============================================================================
   // struct BBeginFadeDecalData
   //============================================================================
   struct BBeginFadeDecalData
   {
      BDecalHandle mHandle;
      uint mUseCount;
      double mStartTime;
      float mTotalTime;
      char mFadeDirection;
      eFadeType mFadeType;
      float mHoldTime;
   };
   
   //============================================================================
   // struct BUpdateData
   //============================================================================
   struct BUpdateData
   {
      BUpdateData(double gameTime, const uchar* pDecalAttribs) : mGameTime(gameTime), mpDecalAttribs(pDecalAttribs) { }
      
      double mGameTime;
      const uchar* mpDecalAttribs;
   };
   
   //============================================================================
   // struct BDecalAttribNormalKeySorter
   //============================================================================
   struct BDecalAttribNormalKeySorter
   {
      BDecalManager& mDecalManager;
      BDecalAttribNormalKeySorter(BDecalManager& decalManager) : mDecalManager(decalManager) { }

      bool operator() (uint i, uint j) const;
   };

   //============================================================================
   // struct BDecalAttribAlphaKeySorter
   //============================================================================
   struct BDecalAttribAlphaKeySorter
   {
      BDecalManager& mDecalManager;
      BDecalAttribAlphaKeySorter(BDecalManager& decalManager) : mDecalManager(decalManager) { }

      bool operator() (uint i, uint j) const;
   };    

   void updateSingleFrameDecals();
            
   void renderUpdate(const BUpdateData* pUpdateData);
   void renderBeginFade(const BBeginFadeDecalData& data);
   void renderTickDecal(BDecalHandle handle, BDecalAttribs& attribs, BDecalState& state);
   void renderDrawSortedDecals(BDynamicRenderArray<ushort>& sortedIndices, uint tileIndex, int categoryIndex, eRenderFilter filter, bool stencilTestWhenConforming, bool categoryEarlyOut, int pass);
   void renderDrawDecalRange(const BDynamicRenderArray<ushort>& sortedIndices, int firstSortedDecalIndex, int lastSortedDecalIndex, bool stencilTestWhenConforming);
   void renderDrawDecalRangeStencil(int pass, const BDynamicRenderArray<ushort>& sortedIndices, int firstSortedDecalIndex, int lastSortedDecalIndex, bool stencilTestWhenConforming);
            
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
           
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
extern BDecalManager gDecalManager;