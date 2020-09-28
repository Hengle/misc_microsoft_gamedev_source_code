//============================================================================
//
//  TerrainRibbon.h
//  
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma  once

// xcore
#include "math\generalVector.h"
#include "containers\dynamicArray.h"
#include "containers\hashTable.h"
#include "threading\eventDispatcher.h"

// xrender
#include "renderThread.h"

// xgame render
#include "sceneLightManager.h"

typedef uint BTerrainRibbonHandle;
const uint cInvalidBTerrainRibbonHandle = UINT_MAX;
class BTerrainRibbonManager;

//============================================================================
// BRibbonCreateParams
//============================================================================
class BRibbonCreateParams
{
public:
   BRibbonCreateParams():
      mTextureName(""),
         mMaxNodes(30),
         mMinDist(2),

         mRibbonWidth(2.5),

         mNodeAmountFramesFullAlpha(30),
         mNodeAmountFramesFading(30),

         mMyHandle(cInvalidBTerrainRibbonHandle)
      {

      }

      BRibbonCreateParams(const BRibbonCreateParams& rVal)
      {
         mMyHandle = rVal.mMyHandle;
         mMaxNodes = rVal.mMaxNodes;
         mMinDist = rVal.mMinDist;
         mRibbonWidth = rVal.mRibbonWidth;
         mTextureName = rVal.mTextureName;

         mNodeAmountFramesFullAlpha = rVal.mNodeAmountFramesFullAlpha;
         mNodeAmountFramesFading = rVal.mNodeAmountFramesFading;
      }

      ~BRibbonCreateParams()
      {
         mMyHandle = (cInvalidBTerrainRibbonHandle);
      }

      BString  mTextureName;
      BTerrainRibbonHandle mMyHandle;
      uint mMaxNodes;
      float mMinDist;
      float mRibbonWidth;

      int mNodeAmountFramesFullAlpha;
      int mNodeAmountFramesFading;

      void copyTo(BRibbonCreateParams& parms)
      {
         parms.mMyHandle = mMyHandle;
         parms.mMaxNodes = mMaxNodes;
         parms.mMinDist = mMinDist;
         parms.mRibbonWidth = mRibbonWidth;
         parms.mTextureName = mTextureName;

         parms.mNodeAmountFramesFullAlpha = mNodeAmountFramesFullAlpha;
         parms.mNodeAmountFramesFading = mNodeAmountFramesFading;
      }

      bool equalTo(BRibbonCreateParams& parms)
      {
         bool eql = true;
         eql &= parms.mMyHandle == mMyHandle;
         eql &= parms.mMaxNodes == mMaxNodes;
         eql &= parms.mMinDist == mMinDist;
         eql &= parms.mRibbonWidth == mRibbonWidth;
         eql &= parms.mTextureName == mTextureName;

         eql &= parms.mNodeAmountFramesFullAlpha == mNodeAmountFramesFullAlpha;
         eql &= parms.mNodeAmountFramesFading == mNodeAmountFramesFading;

         return eql;
      }

      void clear()
      {
         mMyHandle = cInvalidBTerrainRibbonHandle;
         mTextureName = "";
      }
};


//============================================================================
// BTerrainRibbon
//============================================================================
class BTerrainRibbon
{
public:

   BTerrainRibbon();
   ~BTerrainRibbon();

   
   void init(BRibbonCreateParams parms, BTerrainRibbonManager* pOwnerManager);
   void deinit();

   void destroy();

   void update();
   void render();

   bool addPoint(float x, float z, float dirX, float dirZ);

   void stopRibbon(){mStopped = true;}

   bool isStopped() { return mStopped==TRUE;}

   BTerrainRibbonHandle getHandle() { return mParams.mMyHandle;};

   uint getNumAnchors();

   BRibbonCreateParams*  getParams() { return &mParams;};

   class BAnchorNode
   {
   public:
      int mLife;
      int mFullAlphaTime;

      float mPositionX;
      float mPositionZ;
      float mDirectionX;
      float mDirectionZ;
   };

   BAnchorNode* getLastAnchorNode();
   

   BVector getMinBB(){return mMinBB;}
   BVector getMaxBB(){return mMaxBB;}


   bool shouldSplitRibbonForNextPoint(float x, float z);
  
private:

   void calcTangentPoints(uint anchorIndex, BVector2& tangA, BVector2 &tangB);
   float vec2Len(BVector2 vec);
   void vec2Normalize(BVector2& vec);
   float vec2Facing(BVector2 vecA, BVector2 vecB);

   int setupLocalLighting(BVec3 min, BVec3 max);

   BRibbonCreateParams mParams;

   BVector mMinBB;
   BVector mMaxBB;

   BManagedTextureHandle mDiffuseTexHandle;
   BManagedTextureHandle mNormalTexHandle;
   BManagedTextureHandle mOpacityTexHandle;
   BManagedTextureHandle mSpecularTexHandle;
  

   BStaticArray<BAnchorNode> mAnchorNodes;

   BSceneLightManager::BActiveLightIndexArray mCurVisibleLightIndices;

   BOOL  mStopped;
};

//============================================================================
// BTerrainRibbonManager
//============================================================================
class BTerrainRibbonManager : public BRenderCommandListener
{
public:
   BTerrainRibbonManager();
   ~BTerrainRibbonManager();

   void init();
   void deinit();

   void loadAssets(BRibbonCreateParams parms);

   void destroy();

   void update();
   void render(int tileIndex);
   



   BTerrainRibbonHandle createRibbon(BRibbonCreateParams params);
   void addPointToRibbon( BTerrainRibbonHandle handle, float x, float z, float dirX, float dirZ);
   void stopRibbon(BTerrainRibbonHandle handle);
   
   void removeRibbon(BTerrainRibbonHandle handle);
   void clearRibbons();



   BManagedTextureHandle   getManagedTextureHandle(const BFixedString256& textureName);
   
private:

   static const uint cMaxNumRibbons = 60;

   BStaticArray<BTerrainRibbon, cMaxNumRibbons, false, true> mRibbons;
   uint mRibbonHandleCounter;

   class BAddPointData
   {
   public:
      // DO NOT add any member variables to this class with constructors/destructors!
      BTerrainRibbonHandle handle;
      float x;
      float z;
      float dirX;
      float dirZ;
   };
   BTerrainRibbon* getRibbon(BTerrainRibbonHandle handle);
   void addPointToRibbonInternal(const BAddPointData* dat);
   void createRibbonInternal(const BRibbonCreateParams *params);

   enum eCommandEnums
   {
      cTRMDestroy=0,
      cTRMClear,
      cTRMAddRibbon,
      cTRMAddPointToRibbon,
      cTRMStopRibbon,
      cTRMUpdate

   };
   void processCommand(const BRenderCommandHeader& header, const uchar* pData);

   //typedef std::pair<BFixedString256, BManagedTextureHandle> ManagedRibbonTexturePair;
   BHashMap<BFixedString256, BManagedTextureHandle> mDecalHandles;

   //BDynamicArray<ManagedRibbonTexturePair> mDecalHandles;
};


extern BTerrainRibbonManager gTerrainRibbonManager;


