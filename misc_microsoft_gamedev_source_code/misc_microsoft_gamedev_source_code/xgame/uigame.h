//==============================================================================
// uigame.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "ui.h"
#include "visual.h"

class BUser;

//==============================================================================
// BMinimapIcon
//==============================================================================
class BMinimapIcon
{
   public:

      float       mU;
      float       mV;
      float       mSize;
      BSimString  mName;
};

//==============================================================================
// BUIGamePlayerStat
//==============================================================================
class BUIGamePlayerStat
{
   public:
      enum
      {
         cTypePop,
         cTypeResource,
      };

      long              mType;
      long              mID;
      long              mID2;
      long              mLeaderID;
      BUIRect           mIconPosition;
      BUIRect           mValuePosition;
      BUIRect           mIconPositionSV1;
      BUIRect           mValuePositionSV1;
      BUIRect           mIconPositionSV2;
      BUIRect           mValuePositionSV2;
      BUIRect           mIconPositionSH1;
      BUIRect           mValuePositionSH1;
      BUIRect           mIconPositionSH2;
      BUIRect           mValuePositionSH2;
};

typedef BDynamicSimArray<BUIGamePlayerStat> BUIGamePlayerStatArray;

//==============================================================================
// BUIGameUnitStat
//==============================================================================
class BUIGameUnitStat
{
   public:
      enum
      {
         cTypeAttackRating,
         cTypeDefenseRating,
         cTypeAttackGrade,
      };

      BUIGameUnitStat() : mStatType(-1), mStatData(-1), mDisplayNameIndex(-1) {}
      BUIGameUnitStat(const BUIGameUnitStat& source) { *this=source; }
      BUIGameUnitStat& operator=(const BUIGameUnitStat& source)
      {
         if(this==&source)
            return *this;
         mStatType=source.mStatType;
         mStatData=source.mStatData;
         mDisplayNameIndex=source.mDisplayNameIndex;
         mName = source.mName;
         return *this;
      }

      BSimString        mName;
      long              mStatType;
      long              mStatData;
      long              mDisplayNameIndex;      
};

typedef BDynamicSimArray<BUIGameUnitStat> BUIGameUnitStatArray;

//==============================================================================
// BUIGameTribute
//==============================================================================
class BUIGameTribute
{
   public:
      int               mResourceID;
      int               mItemStringIndex;
      int               mDetailStringIndex;
      int               mReceivedStringIndex;
      int               mItemLoc[3];
};

typedef BDynamicSimArray<BUIGameTribute> BUIGameTributeArray;

//==============================================================================
// class BFlashUIDataNode
//==============================================================================
class BFlashUIDataNode
{
   public:
      BFlashUIDataNode() {};
     ~BFlashUIDataNode() {};

      int        mType;
      BSimString mFlashFile;
      BSimString mDataFile;

      BSimString mVSplitScreenFlashFile;
      BSimString mVSplitScreenDataFile;
};

//==============================================================================
// BUIGame
//==============================================================================
class BUIGame
{
   public:
      enum
      {
         cReticleNone,
         cReticleNormal,
         cReticleAttack,
         cReticleGarrison,
         cReticleHitch,
         cReticleCapture,
         cReticleRepair,
         cReticlePowerValid,
         cReticlePowerInvalid,
         cReticleBase,
         cReticleTransportFull,

         cFirstGatherReticle,
         cReticleGatherResource0=cFirstGatherReticle,
         cReticleGatherResource1,
         cReticleGatherResource2,
         cReticleGatherResource3,
         cReticleGatherResource4,
         cReticleGatherResource5,
         cReticleGatherResource6,
         cReticleGatherResource8,
         cReticleGatherResource9,
         cReticleGatherResource10,
         cLastGatherReticle=cReticleGatherResource10,

         cReticleCount,
         cGatherReticleCount=cLastGatherReticle-cFirstGatherReticle+1,
      };

      enum
      {
         cGameCommandRallyPoint,
         cGameCommandSelectPower,
         cGameCommandRepair,
         cGameCommandCount
      };

      enum
      {
         eFlashUITypeUNSC,
         eFlashUITypeCovenant,
         eFlashUITypeTotal,
      };

      enum
      {
         eFlashUIReticle,
         eFlashUIHUD,
         eFlashUIMinimap,
         eFlashUIDecals,
         eFlashUIHints,
         eFlashUICallouts,
         eFlashUIWidgets,
         eFlashUITotal,
      };

      // These enumerations are mirrored in the editor code class TriggerPropFlareType in TriggerSystemTypes.cs, so if you add one here
      // please add one there as well.
      enum
      {
         cFlareLook,
         cFlareHelp,
         cFlareMeet,
         cFlareAttack,
      };

      

      enum
      {
         cFlashableItemMinimap,

         cFlashableItemCircleMenuSlot0,
         cFlashableItemCircleMenuSlot1,
         cFlashableItemCircleMenuSlot2,
         cFlashableItemCircleMenuSlot3,
         cFlashableItemCircleMenuSlot4,
         cFlashableItemCircleMenuSlot5,
         cFlashableItemCircleMenuSlot6,
         cFlashableItemCircleMenuSlot7,

         cFlashableItemCircleMenuPop,
         cFlashableItemCircleMenuPower,
         cFlashableItemCircleMenuSupply,

         cFlashableItemDpad,
         cFlashableItemDpadUp,
         cFlashableItemDpadDown,
         cFlashableItemDpadLeft,
         cFlashableItemDpadRight,

         cFlashableItemResourcePanel,
         cFlashableItemResourcePanelPop,
         cFlashableItemResourcePanelPower,
         cFlashableItemResourcePanelSupply,

         cFlashableItemCount,
      };

                        BUIGame();
                        ~BUIGame();

      bool              init();
      void              deinit();

      long              getMinimapNumFlares() const { return mMinimapNumFlares; }
      DWORD             getMinimapFlareDuration() const { return mMinimapFlareDuration; }
      float             getMinimapFlareStartSize() const { return mMinimapFlareStartSize; }
      float             getMinimapFlareSmallSize() const { return mMinimapFlareSmallSize; }
      float             getMinimapFlareBigSize() const { return mMinimapFlareBigSize; }
      float             getMinimapFlareStartSpeed() const { return mMinimapFlareStartSpeed; }
      float             getMinimapFlareGrowSpeed() const { return mMinimapFlareGrowSpeed; }
      float             getMinimapFlareShrinkSpeed() const { return mMinimapFlareShrinkSpeed; }
      float             getMinimapAlertSize() const { return mMinimapAlertSize; }
      long              getMinimapNumActiveAlerts() const { return mMinimapNumActiveAlerts; }
      long              getMinimapNumQueuedAlerts() const { return mMinimapNumQueuedAlerts; }
      DWORD             getMinimapAlertCooldown() const { return mMinimapAlertCooldown; }
      BUIRect*          getMinimapPosition() { return &mMinimapPosition; }
      BVector           getMinimapBackgroundColor() const { return mMinimapBackgroundColor; }
      bool              getMinimapBackgroundColorActive() const { return mMinimapBackgroundColorActive; }
      BVector           getMinimapVisibilityColor() const { return mMinimapVisibilityColor; }
      float             getMinimapBorderMin() const { return mMinimapBorderMin; }
      float             getMinimapBorderMax() const { return mMinimapBorderMax; }
      bool              getMinimapBorderActive() const { return mMinimapBorderActive; }
      BManagedTextureHandle    getMinimapBorderTextureHandle() const { return mMinimapBorderTextureHandle; }
      BManagedTextureHandle    getMinimapIconTextureHandle() const { return mMinimapIconTextureHandle; }
      BManagedTextureHandle    getMinimapOverlayTextureHandle() const { return mMinimapOverlayTextureHandle; }
      BManagedTextureHandle    getMinimapRadarTextureHandle() const { return mMinimapRadarTextureHandle; }
      float             getMinimapOpacity() const { return mMinimapOpacity; }
      long              getMinimapIconIndex(const BSimString &name) const;
      BMinimapIcon      getMinimapIcon(long index) const { return mMinimapIcon[index]; }
      BMinimapIcon      getMinimapIcon(const BSimString &name) const { return getMinimapIcon(getMinimapIconIndex(name)); }

      BUIRect*          getHelpPosition() { return &mHelpPosition; }
      BHandle           getHelpFont() { return mHelpFont; }
      
      BManagedTextureHandle    getReticleTexture(long reticleType) { return mReticleTextures[reticleType]; }
      const char*       getReticleFlashKeyframe(long reticleType) { return mReticleFlashKeyframes[reticleType].c_str(); }
      long              getReticleSize() const;

      BManagedTextureHandle    getResourceHudIcon(long resourceID) { if(resourceID<0 || resourceID>=mNumberResources) return cInvalidManagedTextureHandle; else return mpResourceHudIcons[resourceID]; }
      BManagedTextureHandle    getResourceTextIcon(long resourceID) { if(resourceID<0 || resourceID>=mNumberResources) return cInvalidManagedTextureHandle; else return mpResourceTextIcons[resourceID]; }
      BManagedTextureHandle    getResourceFloatyIcon(long resourceID) { if(resourceID<0 || resourceID>=mNumberResources) return cInvalidManagedTextureHandle; else return mpResourceFloatyIcons[resourceID]; }
      long              getResourceTextVisual(long resourceID) { if(resourceID<0 || resourceID>=mNumberResources) return -1; else return mpResourceTextVisuals[resourceID]; }
      long              getResourceTextVisualBucketSize(long resourceID) { if(resourceID<0 || resourceID>=mNumberResources) return -1; else return mpResourceTextVisualBucketSize[resourceID]; }
      int               getResourceFlashUICostPanelFrame(long resourceID) const { if(resourceID<0 || resourceID>=mNumberResources) return -1; else return mpResourceFlashUICostPanelFrame[resourceID]; }

      BManagedTextureHandle    getPopHudIcon(long popID) { if(popID<0 || popID>=mNumberPops) return cInvalidManagedTextureHandle; else return mpPopHudIcons[popID]; }
      BManagedTextureHandle    getPopTextIcon(long popID) { if(popID<0 || popID>=mNumberPops) return cInvalidManagedTextureHandle; else return mpPopTextIcons[popID]; }
      int               getPopFlashUICostPanelFrame(long popID) const { if(popID<0 || popID>=mNumberPops) return -1; else return mpPopFlashUICostPanelFrame[popID]; }

      long              getFloatyWidth() const { return mFloatyWidth; }
      long              getFloatyHeight() const { return mFloatyHeight; }

      BHandle           getPlayerStatFont() { return mPlayerStatFont; }
      long              getNumberPlayerStats(long civID) const { if(civID<0 || civID>=mNumberPlayerStatLists) return 0; else return mpPlayerStatLists[civID].getNumber(); }
      const BUIGamePlayerStat* getPlayerStat(long civID, long index) const { if(civID<0 || civID>=mNumberPlayerStatLists || index<0 || index>=mpPlayerStatLists[civID].getNumber()) return NULL; else return &(mpPlayerStatLists[civID][index]); }

      long              getNumberUnitStats() const { return mUnitStats.getNumber(); }
      const BUIGameUnitStat*  getUnitStat(long index) const { if (index<0 || index>=mUnitStats.getNumber()) return NULL; else return &(mUnitStats[index]); }
      long              getUnitStatIndex(const char* name);

      uint              getNumberTributes() const { return mTributes.getSize(); }
      const BUIGameTribute*   getTribute(uint index) const { if (index>=mTributes.getSize()) return NULL; else return &(mTributes[index]); }

      float             getCircleMenuFadeTime() const { return mCircleMenuFadeTime; }
      bool              getCircleMenuDisplayOnUp() const { return mCircleMenuDisplayOnUp; }
      long              getCircleMenuCount() const { return mCircleMenuCount; }
      long              getCircleMenuWidth(BUser* pUser);
      float             getCircleMenuItemRadius(BUser* pUser);
      long              getCircleMenuItemWidth(BUser* pUser);
      BHandle           getCircleMenuButtonFont() { return mCircleMenuButtonFont; }
      BHandle           getCircleMenuHelpTextFont() { return mCircleMenuHelpTextFont; }
      long              getCircleMenuHelpTextIconWidth() const { return mCircleMenuHelpTextIconWidth; }
      long              getCircleMenuHelpTextIconHeight() const { return mCircleMenuHelpTextIconHeight; }
      long              getNumberCircleMenuBackgrounds() const { return mNumberCircleMenuBackgrounds; }
      BManagedTextureHandle    getCircleMenuBackground(long civID) { if(civID<0 || civID>=mNumberCircleMenuBackgrounds) return cInvalidManagedTextureHandle; else return mpCircleMenuBackgrounds[civID]; }
      
      const BFlashUIDataNode& getFlashUICallouts() const { return mFlashUICallouts; }
      const BFlashUIDataNode& getFlashUIHints() const { return mFlashUIHints; }
      const BFlashUIDataNode& getFlashUIWidgets() const { return mFlashUIWidgets; }

      // const BFlashUIDataNode& getFlashUIObjectives(int flashUIType) const { debugRangeCheck(flashUIType, mFlashUIObjectives.getNumber()); return mFlashUIObjectives[flashUIType]; }
      const BFlashUIDataNode& getFlashUIReticle(int flashUIType) const { debugRangeCheck(flashUIType, mFlashUIReticles.getNumber()); return mFlashUIReticles[flashUIType]; }
      const BFlashUIDataNode& getFlashUIHUD(int flashUIType) const { debugRangeCheck(flashUIType, mFlashUIHUDs.getNumber()); return mFlashUIHUDs[flashUIType]; }
      const BFlashUIDataNode& getFlashUIMinimap(int flashUIType) const { debugRangeCheck(flashUIType, mFlashUIMinimaps.getNumber()); return mFlashUIMinimaps[flashUIType]; }
      const BFlashUIDataNode& getFlashUIDecals(int flashUIType) const { debugRangeCheck(flashUIType, mFlashUIDecals.getNumber()); return mFlashUIDecals[flashUIType]; }

      BManagedTextureHandle    getObjectIcon(long protoObjectID, long playerID);      
      BManagedTextureHandle    getTechIcon(long protoTechID);
      BManagedTextureHandle    getSquadIcon(long protoSquadID, long playerID);
      BManagedTextureHandle    getSquadModeIcon(long squadMode);
      BManagedTextureHandle    getObjectCommandIcon(long commandType);

      BManagedTextureHandle    getGameCommandIcon(long commandType);
      long              getGameCommandID(const char* pCommandName);

      int               getSquadFlashSelectionIconFrameID(long protoSquadID);
      int               getObjectFlashSelectionIconFrameID(long protoObjectID);

      void              playSound(uint soundType, long civID=-1, bool record=false);

      void              setCircleSelectDecalHandle(int handle) { mCircleSelectDecalHandle = handle; };
      int               getCircleSelectDecalHandle() { return mCircleSelectDecalHandle; };

      void              setDefaultDecalHandle(int handle) { mDefaultDecalHandle = handle; };
      int               getDefaultDecalHandle() { return mDefaultDecalHandle; };

      

      void              getViewCenter(BUser* pUser, long& x, long& y);

      BManagedTextureHandle    getHoverDecalTextureHandle() const { return mHoverDecalTexture; }

   protected:
      void              loadMinimap(BXMLNode root);
      void              loadHelp(BXMLNode root);
      void              loadReticles(BXMLNode root);
      void              loadResources(BXMLNode root);
      void              loadPops(BXMLNode root);
      void              loadFloaty(BXMLNode root);
      void              loadPlayerStats(BXMLNode root);
      void              loadCircleMenu(BXMLNode root);
      void              loadSquadModes(BXMLNode root);
      void              loadObjectCommands(BXMLNode root);            
      void              loadGameCommands(BXMLNode root);      
      void              loadUnitStats(BXMLNode node);
      void              loadTribute(BXMLNode node);
      void              loadHoverDecal(BXMLNode node);

      void              loadFlashUI(BXMLNode root);
      void              loadFlashUINode(int resourceType, const BXMLNode node);
      void              addFlashUIDataNode(int civID, int resourceType, const BFlashUIDataNode& data);

      typedef BDynamicSimArray<BMinimapIcon>  MinimapIconArray;

      long              mMinimapNumFlares;
      DWORD             mMinimapFlareDuration;
      float             mMinimapFlareStartSize;
      float             mMinimapFlareSmallSize;
      float             mMinimapFlareBigSize;
      float             mMinimapFlareStartSpeed;
      float             mMinimapFlareGrowSpeed;
      float             mMinimapFlareShrinkSpeed;
      float             mMinimapAlertSize;
      long              mMinimapNumActiveAlerts;
      long              mMinimapNumQueuedAlerts;
      DWORD             mMinimapAlertCooldown;
      BUIRect           mMinimapPosition;
      BVector           mMinimapBackgroundColor;
      bool              mMinimapBackgroundColorActive;
      BVector           mMinimapVisibilityColor;
      float             mMinimapBorderMin;
      float             mMinimapBorderMax;
      bool              mMinimapBorderActive;
      BManagedTextureHandle    mMinimapBorderTextureHandle;
      BManagedTextureHandle    mMinimapIconTextureHandle;
      BManagedTextureHandle    mMinimapOverlayTextureHandle;
      BManagedTextureHandle    mMinimapRadarTextureHandle;
      
      long              mMinimapNumIcons;
      MinimapIconArray  mMinimapIcon;
      float             mMinimapOpacity;

      BUIRect           mHelpPosition;
      BHandle           mHelpFont;

      long              mReticleSize;
      long              mReticleSize4x3;
      BManagedTextureHandle    mReticleTextures[cReticleCount];
      BFixedString32    mReticleFlashKeyframes[cReticleCount];

      long              mNumberResources;
      BManagedTextureHandle*   mpResourceHudIcons;
      BManagedTextureHandle*   mpResourceTextIcons;
      BManagedTextureHandle*   mpResourceFloatyIcons;
      long*             mpResourceTextVisuals;
      long*             mpResourceTextVisualBucketSize;
      int*              mpResourceFlashUICostPanelFrame;

      long              mNumberPops;
      BManagedTextureHandle*   mpPopHudIcons;
      BManagedTextureHandle*   mpPopTextIcons;
      int*              mpPopFlashUICostPanelFrame;

      long              mFloatyWidth;
      long              mFloatyHeight;

      BHandle           mPlayerStatFont;
      long              mNumberPlayerStatLists;
      BUIGamePlayerStatArray* mpPlayerStatLists;
      
      BUIGameUnitStatArray mUnitStats;
      BUIGameTributeArray  mTributes;

      float             mCircleMenuFadeTime;
      bool              mCircleMenuDisplayOnUp;
      long              mCircleMenuCount;
      long              mCircleMenuWidth;
      float             mCircleMenuItemRadius;
      long              mCircleMenuItemWidth;
      long              mCircleMenuWidthSV;
      float             mCircleMenuItemRadiusSV;
      long              mCircleMenuItemWidthSV;
      long              mCircleMenuWidthSH;
      float             mCircleMenuItemRadiusSH;
      long              mCircleMenuItemWidthSH;
      BHandle           mCircleMenuButtonFont;
      BHandle           mCircleMenuHelpTextFont;
      long              mCircleMenuHelpTextIconWidth;
      long              mCircleMenuHelpTextIconHeight;
      long              mNumberCircleMenuBackgrounds;
      BManagedTextureHandle*   mpCircleMenuBackgrounds;

      int               mCircleSelectDecalHandle;
      int               mDefaultDecalHandle;
      BManagedTextureHandle    mHoverDecalTexture;

      //-- FlashUI
      
      BFlashUIDataNode mFlashUICallouts;
      BFlashUIDataNode mFlashUIHints;
      BFlashUIDataNode mFlashUIWidgets;

      // BDynamicSimArray<BFlashUIDataNode> mFlashUIObjectives;
      BDynamicSimArray<BFlashUIDataNode> mFlashUIReticles;
      BDynamicSimArray<BFlashUIDataNode> mFlashUIHUDs;
      BDynamicSimArray<BFlashUIDataNode> mFlashUIMinimaps;
      BDynamicSimArray<BFlashUIDataNode> mFlashUIDecals;      

      BManagedTextureHandle*   mpObjectIcons;      
      BManagedTextureHandle*   mpTechIcons;
      BManagedTextureHandle*   mpSquadIcons;
      BManagedTextureHandle*   mpSquadModeIcons;
      BManagedTextureHandle*   mpObjectCommandIcons;

      BManagedTextureHandle    mGameCommandIcons[cGameCommandCount];

      
};

extern BUIGame gUIGame;
