//==============================================================================
// HPBar.h
//
// The HP bar renderer.
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "containers\staticArray.h"
#include "threading\eventDispatcher.h"
#include "renderThread.h"
#include "effect.h"
#include "xmlreader.h"

//==============================================================================
// Forward definitions
//==============================================================================
class BUnit;
class BEntity;
class BSquad;

//==============================================================================
// Defines
//==============================================================================
#define BHPBAR_ARRAY_SIZE        1000

//==============================================================================
// BProtoHPBar
//==============================================================================
class BProtoHPBar
{
public:

   BProtoHPBar() { clear(); }
   ~BProtoHPBar() { }

   bool load(BXMLNode node, BXMLReader* pReader);
   
   void clear(void)
   {
      mName.empty();
      mDiffuseTexture.empty();
      mMaskTexture.empty();
      Utils::ClearObj(mHPUV);
      Utils::ClearObj(mShieldUV);
      Utils::ClearObj(mAmmoUV);
      mDiffuseIndex = -1;
      mMaskIndex = -1;
      mSizeX = 0;
      mSizeY = 0;
   }

   XMVECTOR       mHPUV;
   XMVECTOR       mShieldUV;
   XMVECTOR       mAmmoUV;
   
   BSimString     mName;
   BSimString     mDiffuseTexture;
   BSimString     mMaskTexture;
   
   float          mSizeX;
   float          mSizeY;   
   int            mDiffuseIndex;
   int            mMaskIndex;      
};

//==============================================================================
// BProtoPieProgress
//==============================================================================
class BProtoPieProgress 
{
   public: 
      BProtoPieProgress() { clear(); }
     ~BProtoPieProgress() {}

     bool load(BXMLNode node, BXMLReader* pReader);
   
   void clear(void)
   {
      mName.empty();
      mBackgroundTexture.empty();
      mMaskTexture.empty();      
      mMaskTextureHandle = cInvalidManagedTextureHandle;
      mBackgroundTextureHandle = cInvalidManagedTextureHandle;
      mSizeX = 1.0f;
      mSizeY = 1.0f;
      mOffsetX = 0.0f;
      mOffsetY = 0.0f;
      mBackgroundSizeX = 1.0f;
      mBackgroundSizeY = 1.0f;
      mBackgroundOffsetX = 0.0f;
      mBackgroundOffsetY = 0.0f;
   }
   
   BSimString     mName;
   BSimString     mBackgroundTexture;
   BSimString     mMaskTexture;   
   float          mSizeX;
   float          mSizeY;
   float          mOffsetX;
   float          mOffsetY;
   float          mBackgroundSizeX;
   float          mBackgroundSizeY;
   float          mBackgroundOffsetX;
   float          mBackgroundOffsetY;
   BManagedTextureHandle mBackgroundTextureHandle;
   BManagedTextureHandle mMaskTextureHandle;
   bool           mFill;
   bool           mClockwise;
};

//==============================================================================
// BProtoAnchor
//==============================================================================
class BProtoAnchor
{
   public:
      BProtoAnchor() {clear();}
     ~BProtoAnchor() {};

      bool load(BXMLNode node, BXMLReader* pReader);
      void clear()
      {
         mOffsetX = 0.0f;
         mOffsetY = 0.0f;
      }

      BProtoAnchor& operator=(const BProtoAnchor& rhs)
      {
         mOffsetX = rhs.mOffsetX;
         mOffsetY = rhs.mOffsetY;
         return *this;
      }

      float mOffsetX;
      float mOffsetY;
};

//==============================================================================
// BProtoBobbleHead
//==============================================================================
class BProtoBobbleHead 
{
   public: 
      BProtoBobbleHead() { clear(); }
     ~BProtoBobbleHead() {}

     bool load(BXMLNode node, BXMLReader* pReader);
   
   void clear(void)
   {
      mName.empty();
      mBackgroundTexture.empty();
      mVetTexture.empty();      
      mPortraitTexture.empty();
      mVetAnchors.clear();
      
      mBackgroundTextureHandle = cInvalidManagedTextureHandle;
      mVetTextureHandle = cInvalidManagedTextureHandle;
      mPortraitTextureHandle = cInvalidManagedTextureHandle;

      mPortraitSizeX = 1.0f;
      mPortraitSizeY = 1.0f;
      mPortraitOffsetX = 0.0f;
      mPortraitOffsetY = 0.0f;

      mBackgroundSizeX = 1.0f;
      mBackgroundSizeY = 1.0f;
      mBackgroundOffsetX = 0.0f;
      mBackgroundOffsetY = 0.0f;

      mVetRadius = 1.0f;
      mVetAngleRange = 90.0f;
      mVetOffsetX = 0.0f;
      mVetOffsetY = 0.0f;
      mVetScaleX = 1.0f;
      mVetScaleY = 1.0f;
      mVetSkewX = 1.0f;
      mVetSkewY = 1.0f;
   }
   
   typedef BStaticArray<BProtoAnchor, 16>  AnchorArray;
   AnchorArray    mVetAnchors;

   BSimString     mName;
   BSimString     mBackgroundTexture;
   BSimString     mVetTexture;
   BSimString     mPortraitTexture;

   float          mPortraitSizeX;
   float          mPortraitSizeY;
   float          mPortraitOffsetX;
   float          mPortraitOffsetY;

   float          mBackgroundSizeX;
   float          mBackgroundSizeY;
   float          mBackgroundOffsetX;
   float          mBackgroundOffsetY;

   float          mVetRadius;
   float          mVetAngleRange;
   float          mVetOffsetX;
   float          mVetOffsetY;
   float          mVetScaleX;
   float          mVetScaleY;
   float          mVetSkewX;
   float          mVetSkewY;

   BManagedTextureHandle mBackgroundTextureHandle;
   BManagedTextureHandle mVetTextureHandle;
   BManagedTextureHandle mPortraitTextureHandle;   
};


class BProtoVeterancyBar
{
   public: 
      BProtoVeterancyBar() { clear(); }
     ~BProtoVeterancyBar() {}

     bool load(BXMLNode node, BXMLReader* pReader);
   
   void clear(void)
   {
      mName.empty();
      mTexture.empty();
      mTextureHandle = cInvalidManagedTextureHandle;
      mSizeX = 0;
      mSizeY = 0;
      mOffsetX = 0.0f;
      mOffsetY = 0.0f;
   }
   
   BSimString     mName;
   BSimString     mTexture;
   float          mSizeX;
   float          mSizeY;
   float          mOffsetX;
   float          mOffsetY;
   BManagedTextureHandle mTextureHandle;
};

//==============================================================================
// BProtoBuildingStrength
//==============================================================================
class BProtoBuildingStrength
{
   public: 
      BProtoBuildingStrength() { clear(); }
     ~BProtoBuildingStrength() {}

     bool load(BXMLNode node, BXMLReader* pReader);
   
   void clear(void)
   {
      mName.empty();
      mTextureOn.empty();
      mTextureOff.empty();      
      mTextureOffHandle=cInvalidManagedTextureHandle;
      mTextureOnHandle=cInvalidManagedTextureHandle;
      mAnchors.clear();
      mSizeX = 1.0f;
      mSizeY = 1.0f;
      mOffsetX = 0.0f;
      mOffsetY = 0.0f;
      mPadding = 1.0f;
   }

   typedef BStaticArray<BProtoAnchor, 16>  AnchorArray;
   AnchorArray    mAnchors;
   BSimString     mName;
   BSimString     mTextureOn;
   BSimString     mTextureOff;
   BManagedTextureHandle mTextureOnHandle;
   BManagedTextureHandle mTextureOffHandle;

   float          mSizeX;
   float          mSizeY;
   float          mOffsetX;
   float          mOffsetY;
   float          mPadding;
};

//==============================================================================
//==============================================================================
class BProtoHPBarColorStage
{
   public:
      BProtoHPBarColorStage() { clear(); };
     ~BProtoHPBarColorStage() {};

      bool load(BXMLNode node, BXMLReader* pReader);
      
      void clear(void) 
      {
         mColor = 0;
         mPct = 0;
         mIntensity = 0;
      }

      DWORD mColor;
      float mPct;
      float mIntensity;
};

//==============================================================================
//==============================================================================
class BProtoHPBarColorStages
{
   public: 
      BProtoHPBarColorStages() {};
     ~BProtoHPBarColorStages() {};

     bool load(BXMLNode node, BXMLReader* pReader);
     const BProtoHPBarColorStage* getHPStage(float pct);
     const BProtoHPBarColorStage* getAmmoStage(float pct);
     const BProtoHPBarColorStage* getShieldStage(float pct);

     BSimString mName;
     BDynamicSimArray<BProtoHPBarColorStage> mHP;
     BDynamicSimArray<BProtoHPBarColorStage> mShield;
     BDynamicSimArray<BProtoHPBarColorStage> mAmmo;
};

//==============================================================================
// Class definition
//==============================================================================
class BHPBar : public BEventReceiver, BRenderCommandListener
{

public:

   // Constructor/Destructor
   BHPBar();
   ~BHPBar();

   // Initializer/Deinitializer
   void init(void);
   void deinit(void);
   
   void initScenario(void);
   void deinitScenario(void);

   // Display a unit's HP
   void displayHP(BUnit &unit);
   void getHPBarPosition(const BEntity &entity, BVector &position, float &width, float &height);
   void displaySquadHP(BSquad* pSquad);
   void displayProgress(const BUnit* pUnit, float value);
   void displayProgress(BSquad* pSquad, float value);   
   void displayVeterancy(BSquad* pSquad, bool bCentered);
   void displayRechargeProgress(BSquad* pSquad, bool bCentered);
   void displayBobbleHead(BSquad* pSquad);
   void displayBuildingStrength(BSquad* pSquad);
   
   // helper functions
   void getHPBarStatus(BUnit& unit, float& hpPct, float& shieldPct, float& ammoPct);
   void getSquadBarStatus(BSquad* pSquad, float& hpPct, float& shieldPct, float& ammoPct);
   void getSquadHPBarPosition(BSquad* pSquad, BVector& pos);

   // Prep for next game update
   void prep(void);

   // Render
   void render(int viewportIndex, bool bSplitscreen);

protected:

   struct BPTCVertex
   {
      XMFLOAT4 position;
      XMHALF2  size;
      D3DCOLOR color;
   };

   struct BSquadHPVertex
   {
      XMFLOAT4 mPos;      
      XMHALF4  mHPDimension;
      XMHALF4  mShieldDimension;
      XMHALF4  mAmmoDimension;
      XMHALF4  mStatus;
      XMHALF2  mSize;      
      D3DCOLOR mColor;
      D3DCOLOR mHPColor;
      D3DCOLOR mShieldColor;
      D3DCOLOR mAmmoColor;

   };

   struct BPieProgress
   {

      D3DCOLOR mColor;
   };

   struct BRenderHPBarPacket
   {
      long                    numPrimitives;
      IDirect3DVertexBuffer9* HPBarVB;
      int                     mViewportIndex;
      bool                    mSplitScreen;
   };

   struct BRenderSquadBarPacket
   {
      long                     numPrimitives;
      IDirect3DVertexBuffer9*  mSquadBarVB;
      int                      mViewportIndex;
      bool                     mSplitScreen;
   };

   typedef BStaticArray<BPTCVertex, BHPBAR_ARRAY_SIZE, true, false>  HPBarArray;
   typedef BStaticArray<BSquadHPVertex, BHPBAR_ARRAY_SIZE, true, false>  SquadHPBarArray;

   enum 
   {
      cMMCommandRender = 0,
      cMMCommandRenderSquadBars,

      cMMCommandMax
   };

   enum 
   {
      eEventClassReloadEffects = cEventClassFirstUser
   };

   enum 
   {
      eColorStageIDDefault,
      eColorStageIDTotal,
   };
      
   bool loadTextures();
   bool loadVeterancyBarTextures();
   bool loadAbilityRecoveryBarTextures();
   bool loadBobbleHeadTextures();
   bool loadBuildingStrengthTextures();

   void unloadTextures();
   void unloadVeterancyBarTextures();
   void unloadAbilityRecoveryBarTextures();
   void unloadBobbleHeadTextures();
   void unloadBuildingStrengthTextures();

   //Squad Bar functions
   void getBarColors(int playerID, float hpPct, float shieldPct, float ammoPct, DWORD& hpColor, DWORD& shieldColor, DWORD& ammoColor);

   //BRenderCommandListener interface
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader &header, const uchar *pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);

   //BEventReceiver interface
   virtual bool receiveEvent(const BEvent &event, BThreadIndex threadIndex);

   // Worker thread functions
   void loadEffect(void);
   void setupEffect(const BEvent &event);
   void killEffect(void);
   void workerRender(const BRenderHPBarPacket *HPBarData);
   void workerRender(const BRenderSquadBarPacket* pPacket);   

   HPBarArray           mHPBarArray;
   SquadHPBarArray      mSquadBarArray;

   BFXLEffect           mEffect;
   BFXLEffectTechnique  mRenderHPBarTechnique;   
   BFXLEffectTechnique  mRenderSquadBarTechnique;
   



   BFXLEffectParam      mDiffuseTextureParam;
   BFXLEffectParam      mMaskTextureParam;
   BFXLEffectParam      mAmmoBarColor;
   BFXLEffectParam      mShieldBarColor;

   //-- Texture Arrays
   BD3DTexture          mDiffuseTexture; 
   BD3DTexture          mMaskTexture;
   
   int                  mColorStageID[eColorStageIDTotal];

   IDirect3DVertexDeclaration9   *mBPTCVertexDecl;
   IDirect3DVertexDeclaration9*   mSquadVertexDecl;   
};

extern BHPBar gHPBar;