//============================================================================
// flashhud.h
//
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"
#include "cost.h"
#include "pop.h"

class BUser;

const int cMaxStatItems = 3;
const int cMaxUnitStatItems = 5;

// Forward declarations
class BChatMessage;

class BFlashIconLookup
{
   public:
      BFlashIconLookup(): mProtoID(-1), mIndex(-1) {};
     ~BFlashIconLookup(){};

      int mProtoID;
      int mIndex;
};

//==============================================================================
// BFlashCircleMenuItemStatData
//==============================================================================
template<uint MaxStatCount> 
class BFlashCircleMenuItemStatData
{
   public:
      enum { eMaxStatCount = MaxStatCount };
      
      BFlashCircleMenuItemStatData() 
      {
         clear();
      };
                  
      void clear()
      {
         memset(mStatID, -1, sizeof(mStatID));
         memset(mStatType, -1, sizeof(mStatType));
         memset(mStatIndex, -1, sizeof(mStatIndex));
         memset(mStatFrameID, -1, sizeof(mStatFrameID));
         memset(mStatAmount, -1, sizeof(mStatAmount));
         mCount=0;
      }
      
      BFlashCircleMenuItemStatData(const BFlashCircleMenuItemStatData& s) { *this=s; }
      
      BFlashCircleMenuItemStatData& operator=(const BFlashCircleMenuItemStatData& s)
      {
         if (this==&s)
            return *this;

         mCount = s.mCount;
         memcpy(mStatID, s.mStatID, sizeof(s.mStatID));
         memcpy(mStatType, s.mStatType, sizeof(s.mStatType));
         memcpy(mStatIndex, s.mStatIndex, sizeof(s.mStatIndex));
         memcpy(mStatFrameID, s.mStatFrameID, sizeof(s.mStatFrameID));
         memcpy(mStatAmount, s.mStatAmount, sizeof(s.mStatAmount));

         return *this;
      }

      int mCount;
      int mStatID[eMaxStatCount];
      int mStatType[eMaxStatCount];
      int mStatIndex[eMaxStatCount];
      int mStatFrameID[eMaxStatCount];
      float mStatAmount[eMaxStatCount];
};

//==============================================================================
// BFlashCircleMenuItem
//==============================================================================
class BFlashCircleMenuItem
{
public:
   BFlashCircleMenuItem() : mOrder(-1), mID(-1), mCost(), mPops(), mTrainCount(-1), mTrainLimit(-1), mTrainPercent(0.0f), mTechPrereqID(-1), mInfoText(), mInfoDetail(), mInfoPercentage(), mItemType(-1), mOwnerProtoID(-1), mIconID(-1), mStatData(), mUnavailable(false), mUnitStatProtoID(-1), mFree(false) {}
   BFlashCircleMenuItem(const BFlashCircleMenuItem& source) { *this=source; }
   BFlashCircleMenuItem& operator=(const BFlashCircleMenuItem& source)
   {
      if(this==&source)
         return *this;
      mOrder=source.mOrder;
      mID=source.mID;
      mCost=source.mCost;
      mPops=source.mPops;
      mTrainCount=source.mTrainCount;
      mTrainLimit=source.mTrainLimit;
      mTechPrereqID=source.mTechPrereqID;
      mInfoText=source.mInfoText;
      mInfoText2=source.mInfoText2;
      mInfoDetail=source.mInfoDetail;
      mInfoPercentage=source.mInfoPercentage;
      mItemType=source.mItemType;      
      mStatData=source.mStatData;
      mUnitStatData=source.mUnitStatData;
      mOwnerProtoID=source.mOwnerProtoID;
      mIconID=source.mIconID;
      mUnavailable=source.mUnavailable;
      mTrainPercent=source.mTrainPercent;
      mUnitStatProtoID=source.mUnitStatProtoID;
      mFree=source.mFree;
      
      return *this;
   }

   long                    mOrder;
   long                    mID;
   BCost                   mCost;
   BPopArray               mPops;
   long                    mTrainCount;
   long                    mTrainLimit;
   float                   mTrainPercent;
   long                    mTechPrereqID;
   BUString                mInfoText;
   BUString                mInfoText2;
   BUString                mInfoDetail;
   BUString                mInfoPercentage;
   int                     mItemType;   
   int                     mOwnerProtoID;
   int                     mIconID;
   long                    mUnitStatProtoID;
   BFlashCircleMenuItemStatData<cMaxStatItems> mStatData;
   BFlashCircleMenuItemStatData<cMaxUnitStatItems> mUnitStatData;
   bool                    mUnavailable;
   bool                    mFree;
   
};

class BCircleMenuSlotState
{   
   public:
      BCircleMenuSlotState() : mType(-1), mFrame(-1), mOwnerID(-1), mIconID(-1), mBuildPanelFrameID(-1), mVisible(false), mTagged(false), mIsTraining(false) {};
      ~BCircleMenuSlotState() {};

      void clear()
      {
         mType=-1;
         mFrame=-1;
         mOwnerID=-1;
         mIconID=-1;
         mBuildPanelFrameID=-1;
         mVisible=false;
         mTagged=false;
         mIsTraining=false;
      };

      int  mType;      
      int  mFrame;
      int  mOwnerID;
      int  mIconID;
      int  mBuildPanelFrameID;
      bool mVisible;
      bool mTagged;
      bool mIsTraining;
};

class BUnitSlotState
{   
   public:
      BUnitSlotState() : mType(-1), mFrame(-1), mOwnerID(-1), mIconID(-1), mAbilityIconID(-1), mCount(-1), mAbilityRechargePct(0), mVetTechLevel(0), mVisible(false), mTagged(false), mAbilityStateID(-1), mDirty(false), mColor(cDWORDWhite), mShowGamerTag(false) {};
     ~BUnitSlotState() {};

      void clear()
      {
         mType=-1;
         mFrame=-1;
         mOwnerID=-1;
         mIconID=-1;
         mAbilityIconID=-1;
         mCount= -1;
         mVetTechLevel = 0;
         mVisible=false;
         mTagged=false;
         mCountStr.empty();
         mDetailStr.empty();
         mNameStr.empty();
         mRoleStr.empty();
         mVetTechLevelStr.empty();
         mGamerTagStr.empty();
         mAbilityStateID=-1;
         mAbilityRechargePct=0;
         mColor = cDWORDWhite;
         mShowGamerTag = false;
         mDirty=false;
      };

      BUString mCountStr;
      BUString mDetailStr;
      BUString mNameStr; 
      BUString mRoleStr; 
      BUString mGamerTagStr;
      BUString mVetTechLevelStr;

      int  mType;      
      int  mFrame;
      int  mOwnerID;
      int  mIconID;
      int  mAbilityIconID;
      int  mAbilityStateID;
      int  mAbilityRechargePct;
      int  mCount;
      int  mVetTechLevel;
      DWORD mColor;
      bool mVisible;
      bool mTagged;      
      bool mShowGamerTag;
      bool mDirty;
};


class BButtonState
{
   public:
      BButtonState() : mID (-1), mFrame(-1), mDirty(false) {};
     ~BButtonState() {};
      int  mID;
      int  mFrame;
      bool mDirty : 1;
};

class BInfoDataElement
{
   public:
      BUString mText;
      DWORD    mColor;   
};

class BCostPanelStatState
{
   public:
      BCostPanelStatState() : mFrame(-1), mCanAfford(true) {};
     ~BCostPanelStatState(){};

      void clear()
      {
         mFrame=-1;
         mCanAfford=true;
      };

      int  mFrame;
      bool mCanAfford;
};

//============================================================================
// class BFlashHUD
//============================================================================
class BFlashHUD : public BFlashScene
{
public:
   BFlashHUD();
   virtual ~BFlashHUD();

   enum 
   {
      cFlagInitialized = 0,      
      cFlagTotal
   };

   enum BUIMenuType 
   {
      eMenuNormal,
      eMenuGod,
      eMenuTotal,
   };

   enum BHUDControls
   {
      eControlLabelButtonX = 0,
      eControlLabelButtonY,
      eControlLabelButtonA,
      eControlLabelButtonB,
      eControlLabelDPadUp,
      eControlLabelDPadDown,
      eControlLabelDPadLeft,
      eControlLabelDPadRight,

      eControlLabelUnitSelectionStart,
      eControlLabelUnitSelectionSlot0 = eControlLabelUnitSelectionStart,
      eControlLabelUnitSelectionSlot1,
      eControlLabelUnitSelectionSlot2,
      eControlLabelUnitSelectionSlot3,
      eControlLabelUnitSelectionSlot4,
      eControlLabelUnitSelectionSlot5,
      eControlLabelUnitSelectionSlot6,
      eControlLabelUnitSelectionSlot7,
      eControlLabelUnitSelectionEnd = eControlLabelUnitSelectionSlot7,

      eControlLabelGameTime,
      eControlLabelButtonShoulderRight,
      eControlLabelButtonShoulderLeft,

      eControlLabelPlayerScoreBegin,
      eControlLabelPlayer1Score = eControlLabelPlayerScoreBegin,
      eControlLabelPlayer2Score,
      eControlLabelPlayer3Score,
      eControlLabelPlayer4Score,
      eControlLabelPlayer5Score,
      eControlLabelPlayer6Score,
      eControlLabelPlayerScoreEnd = eControlLabelPlayer6Score,

      eControlLabelTotal,

      eControlInfoPanelSlot1Start=eControlLabelTotal,
      eControlInfoPanelSlot1Text=eControlInfoPanelSlot1Start,
      eControlInfoPanelSlot1DetailText1,
      eControlInfoPanelSlot1DetailText2,
      eControlInfoPanelSlot1DetailText3,
      eControlInfoPanelSlot1DetailText4,
      eControlInfoPanelSlot1DetailText5,
      eControlInfoPanelSlot1DetailText6,
           
      eControlInfoPanelSlot2Start,
      eControlInfoPanelSlot2Text = eControlInfoPanelSlot2Start,
      eControlInfoPanelSlot2DetailText1,
      eControlInfoPanelSlot2DetailText2,
      eControlInfoPanelSlot2DetailText3,
      eControlInfoPanelSlot2DetailText4,
      eControlInfoPanelSlot2DetailText5,
      eControlInfoPanelSlot2DetailText6,

      eControlInfoPanelSlot3Start,
      eControlInfoPanelSlot3Text = eControlInfoPanelSlot3Start,
      eControlInfoPanelSlot3DetailText1,
      eControlInfoPanelSlot3DetailText2,
      eControlInfoPanelSlot3DetailText3,
      eControlInfoPanelSlot3DetailText4,
      eControlInfoPanelSlot3DetailText5,
      eControlInfoPanelSlot3DetailText6,
      
      eControlInfoPanelSlot4Start,
      eControlInfoPanelSlot4Text = eControlInfoPanelSlot4Start,
      eControlInfoPanelSlot4DetailText1,
      eControlInfoPanelSlot4DetailText2,
      eControlInfoPanelSlot4DetailText3,
      eControlInfoPanelSlot4DetailText4,
      eControlInfoPanelSlot4DetailText5,
      eControlInfoPanelSlot4DetailText6,

      eControlInfoPanelSlot5Start,
      eControlInfoPanelSlot5Text = eControlInfoPanelSlot5Start,
      eControlInfoPanelSlot5DetailText1,
      eControlInfoPanelSlot5DetailText2,
      eControlInfoPanelSlot5DetailText3,
      eControlInfoPanelSlot5DetailText4,
      eControlInfoPanelSlot5DetailText5,
      eControlInfoPanelSlot5DetailText6,

      eControlInfoPanelSlot6Start,
      eControlInfoPanelSlot6Text = eControlInfoPanelSlot6Start,
      eControlInfoPanelSlot6DetailText1,
      eControlInfoPanelSlot6DetailText2,
      eControlInfoPanelSlot6DetailText3,
      eControlInfoPanelSlot6DetailText4,
      eControlInfoPanelSlot6DetailText5,
      eControlInfoPanelSlot6DetailText6,
      
      eControlInfoPanelSlot7Start,
      eControlInfoPanelSlot7Text = eControlInfoPanelSlot7Start,
      eControlInfoPanelSlot7DetailText1,
      eControlInfoPanelSlot7DetailText2,
      eControlInfoPanelSlot7DetailText3,
      eControlInfoPanelSlot7DetailText4,
      eControlInfoPanelSlot7DetailText5,
      eControlInfoPanelSlot7DetailText6,

      eControlInfoPanelSlot8Start,
      eControlInfoPanelSlot8Text = eControlInfoPanelSlot8Start,
      eControlInfoPanelSlot8DetailText1,
      eControlInfoPanelSlot8DetailText2,
      eControlInfoPanelSlot8DetailText3,
      eControlInfoPanelSlot8DetailText4,
      eControlInfoPanelSlot8DetailText5,
      eControlInfoPanelSlot8DetailText6,

      eControlResearchPosTextStart,
      eControlResearchPos1Text = eControlResearchPosTextStart,
      eControlResearchPos2Text,
      eControlResearchPos3Text,
      eControlResearchPos4Text,
      eControlResearchPos5Text,
      eControlResearchPos6Text,
      eControlResearchPos7Text,
      eControlResearchPos8Text,
      eControlResearchPosTextEnd = eControlResearchPos8Text,
           
      eControlInfoPanelStatStart,
      eControlInfoPanelStatText1 = eControlInfoPanelStatStart,
      eControlInfoPanelStatText2,
      eControlInfoPanelStatText3,
      eControlInfoPanelStatText4,
      eControlInfoPanelStatText5,
      eControlInfoPanelStatEnd = eControlInfoPanelStatText5,
      eControlInfoPanelStatHeader,
      eControlInfoPanelDescriptionHeader,
      eControlInfoPanelDescriptionText,

      eControlCostPanelTitleBar,
      eControlCostPanelTitleBar2,
      eControlCostPanelStat1Label,
      eControlCostPanelStat1Icon,
      eControlCostPanelStat2Label,
      eControlCostPanelStat2Icon, 
      eControlCostPanelStat3Label,
      eControlCostPanelStat3Icon, 
      eControlBaseTextLabel,
      eControlBaseText2Label,

      eControlResourcePanelResource1,
      eControlResourcePanelResource2,
      eControlResourcePanelResource3,     

      eControlButtonBegin,
      eControlButtonX = eControlButtonBegin,
      eControlButtonY,
      eControlButtonA,
      eControlButtonB,
      eControlButtonLB,
      eControlButtonRB,
      eControlDPad,
      eControlButtonEnd = eControlDPad,

      eControlDPadButtonBegin,
      eControlDPadUp = eControlDPadButtonBegin,
      eControlDPadRight,
      eControlDPadDown,
      eControlDPadLeft,
      eControlDPadButtonEnd = eControlDPadLeft,

      eControlResourcePanelResource1SplitLeft,
      eControlResourcePanelResource2SplitLeft,
      eControlResourcePanelResource3SplitLeft,

      eControlResourcePanelResource1SplitRight,
      eControlResourcePanelResource2SplitRight,
      eControlResourcePanelResource3SplitRight,
      
      eControlTotal,

      eControlButtonCount = eControlButtonEnd - eControlButtonBegin + 1,
      eControlDPadButtonCount = eControlDPadButtonEnd - eControlDPadButtonBegin + 1,
   };

   enum BHUDASFunctions
   {
      eHUDASFunctionSetCircleMenuUnitSlot = 0,
      eHUDASFunctionSetCircleMenuBuildingSlot,
      eHUDASFunctionSetCircleMenuPowerSlot,
      eHUDASFunctionSetCircleMenuTechSlot,
      eHUDASFunctionSetCircleMenuMiscSlot,
      eHUDASFunctionSetCircleMenuAbilitySlot,
      eHUDASFunctionSetSelectionPad,
      eHUDASFunctionSetBuildDataSlot,
      eHUDASFunctionSetUnitSelection,
      eHUDASFunctionSetInfoPanelSlot,
      eHUDASFunctionSetOverlayVisible,
      eHUDASFunctionInitResourcePanel,
      eHUDASFunctionSetUnitSelectionVisible,
      eHUDASFunctionSetGameTimeVisible,
      eHUDASFunctionSetDPadVisible,
      eHUDASFunctionSetCircleMenuVisible,
      eHUDASFunctionSetCircleMenuMode,
      eHUDASFunctionSetButtonState,
      eHUDASFunctionSetBuildPanelSlot,
      eHUDASFunctionSetScoreVisible,      
      eHUDASFunctionSetButtonPanelVisible,
      eHUDASFunctionSetPowerOverlayVisible,
      eHUDASFunctionSetUnitStatsVisible,
      eHUDASFunctionSetUnitStatsData,
      eHUDASFunctionSetInfoPanelStatData,
      eHUDASFunctionSetTechNotification,
      eHUDASFunctionSetChatDisplayVisible,
      eHUDASFunctionSetChatData,
      eHUDASFunctionSetControlDimension,
      eHUDASFunctionSetControlXY,
      eHUDASFunctionSetControlWidthHeight,
      eHUDASFunctionSetControlOffsetXY,
      eHUDASFunctionSetControlScaleXY,
      eHUDASFunctionSetCircleMenuExtraInfoVisible,
      eHUDASFunctionSetCircleMenuSlotIcon,
      eHUDASFunctionSetDPadButtonIcon,
      eHUDASFunctionSetInfoPanelScrollBoxFrame,
      eHUDASFunctionSetCostPanelStatIcon,
      eHUDASFunctionInitResourcePanelSplitLeft,
      eHUDASFunctionInitResourcePanelSplitRight,
      eHUDASFunctionStartInfoPanelScrollUp,
      eHUDASFunctionStartInfoPanelScrollDown,
      eHUDASFunctionStopInfoPanelScroll,
      eHUDASFunctionSetInfoPanelDescription,
      eHUDASFunctionShowInfoPanelRightStick,


      eHUDASFunctionTotal,
   };

   enum BHUDKeyframes
   {
      eHUDKeyframeOff = 0,
      eHUDKeyframeOn,
      eHUDKeyframePending,
      eHUDKeyframeOver,
      eHUDKeyframeBuild,
      eHUDKeyframeEaseIn,
      eHUDKeyframeEaseOut,
      eHUDKeyframeGod,
      eHUDKeyframeNormal,
      eHUDKeyframeResource,
      eHUDKeyframeActive,
      eHUDKeyframeAlert,
      eHUDKeyframeCardSkirmish,
      eHUDKeyframeCardCampaign,
      eHUDKeyframeLocked,
      eHUDKeyframeQueued,
      eHUDKeyframeTotal,
   };
   
   enum BUICircleMenuItemType
   {
      eCircleMenuItemTypeUnit = 0,
      eCircleMenuItemTypeBuilding,
      eCircleMenuItemTypePower,
      eCircleMenuItemTypeTech,
      eCircleMenuItemTypeMisc,
      eCircleMenuItemTypeAbility,
      eCircleMenuItemTypeMode,
      eCircleMenuItemTypeTotal,
   };

   enum BUIHUDStringType
   {
      eHUDStringCombatGrade = 0,
      eHUDStringTotal,
   };

   enum
   {
      cMaxInfoLines=10,      
      cMaxDetailLines=6,
      cMaxUnitStatsData=5,
      cMaxPlayerSlots=6,
      cMaxAttackSlots=8,
      cMaxCostPanelStats=3,
   };

   static BUString sLocalTempUString;
   bool init(const char* filename, const char* dataFile);
   void deinit();
   void enter();
   void leave();
   BFlashMovieInstance*  getMovie() { return mpMovie; }
   void update(float elapsedTime);
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void setDimension(int x, int y, int width, int height);

   BManagedTextureHandle getRenderTargetTexture();

   void setUser(BUser* pUser) { mpUser = pUser; }
   BUser* getUser() const { return mpUser; }

   void setControlText(int labelID, const BUString& text);
   void setControlText(int labelID, const WCHAR* pText);
   void setControlVisible(int labelID, bool bVisible);
   void setAllControlsVisible(bool bVisible);

   // Player Score (strength)
   void setScoresVisible(bool bON);
   bool getScoresVisible() const { return mScoresVisible; }
   void setPlayerScore(int playerID, const BUString& text);
   void setPlayerColor(int playerID, int color);

   // Chat display
   void displayChat(BChatMessage* pChat);
   void setChatVisible(bool bVisible);

   // Unit Stats
   void setUnitStatsVisible(bool bON);
   void setUnitStats(const BUString& name, float stat1, float stat2, float stat3, float stat4, float stat5);
   bool getUnitStatsVisible() const { return mUnitStatsVisible; }
   
   void setButtonState(int controlID, int keyFrameID);
   void setDPadButtonIcon(int controlID, int iconID, int frameID);
   int  getInputFunctionIconID(int inputFunctionID);

   bool isOverlayVisible() const { return mOverlayVisible; }
   void setOverlayVisible(bool bON);

   bool isPowerOverlayVisible() const { return mPowerOverlayVisible; }
   void setPowerOverlayVisible(int powerID, bool bON);

   void setPowerOverlayText(int powerID, const BUString& text);
   
   void refresh();

   void setUnitSelectionVisible(bool bVisible);
   void setUnitSelection(int slotID, int frameID, BEntityID squadID, int protoSquadID, int protoObjID, bool tagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color);   
   bool getUnitSelectionVisible() const { return mUnitSelectionVisible; }
   void updateUnitSelectionDisplay(int subSelectIndex, bool bDisplayArrows);

   void setUnitCardVisible(bool bVisible);
   void setUnitCard(int slotID, int frameID, int playerID, BEntityID squadID, int protoSquadID, int protoObjID, int abilityID, bool tagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color);   
   bool getUnitCardVisible() const { return mUnitCardVisible; }
   void updateUnitCardDisplay();

   void setGameTimeVisible(bool bVisible);
   void setGameTime(const BUString& timeStr);
   bool getGameTimeVisible() const { return mGameTimeVisible; }

   void setResourcePanelVisible(bool bVisible);
   bool getResourcePanelVisible() const { return mResourcePanelVisible; }

   void setPowerPanelVisible(bool bVisible);
   bool getPowerPanelVisible() const { return mPowerPanelVisible; }

   void setDPadPanelVisible(bool bVisible);
   bool getDPadPanelVisible() const { return mDPadVisible; }

   void setButtonPanelVisible(bool bVisible);
   bool getButtonPanelVisible() const { return mButtonPanelVisible; }

   void setTechNotification(int techID, bool bVisible);   

   //-- Circle Menu Interface
   void resetCircleMenuPointingAt() { mCurrentItem=-1; mPointingAtX=0.0f; mPointingAtY=0.0f; }
   void setCircleMenuVisible(bool bVisible, int type);
   bool getCircleMenuVisible() const { return mCircleMenuVisible; }
   void clearCircleMenuItems();
   void clearInfoPanelStats();
   void clearInfoPanelDescription();
   long getCircleMenuItemIndex(long id) const;
   long addCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerProtoID, int unitStatProtoID);
   bool editCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerProtoID, int unitStatProtoID);
   void removeCircleMenuItem(long index);   
   long getNumberCircleMenuItems() const { return mItems.getNumber(); }
   void refreshCircleMenu(bool bRefreshTrainProgressOnly);
   void setCircleMenuBaseText(const WCHAR* text) { mBaseText = text; }      
   void setCircleMenuBaseText2(const WCHAR* text) { mBaseText2 = text; }
   void setCircleMenuBaseTextDetail(const WCHAR* text) { mBaseTextDetail = text; }      
   int  lookupIconID(const char* pIconType, const char* pIconName);
   void setCircleMenuExtraInfoVisible(bool bVisible);
   bool getCircleMenuExtraInfoVisible() const;
   void clearCircleMenuDisplay(bool force);   

   // show an attack 4 on the UI
   void addAlert(BVector pos);

   void flashCircleMenuSlot(int slot, bool flashOn);
   void flashCircleMenuCenterPanel(int slot, bool flashOn);
   void flashResourcePanelSlot(int slot, bool flashOn);
   void flashResourcePanel(bool flashOn);

private:

   enum
   {
      eUnitSelectionSlotCount = 8,
   };

   void initLookups();
   void initStringIDs();
   void initButtonPanel();
   void initResourcePanel();
   void updateLabels();  
   void updateResourcePanel();
   void updateInfoPanel();
   void setText(int labelID, const BUString& text);
   void setText(int labelID, const WCHAR* pText);
   void updatePopupHelp(float elapsedTime);

   //-- Attack Alert functions
   void initAttackAngles();
   void updateAlerts();

   void updateTechNotification();

   //-- Circle Menu functions
   void updatePointingAt(float x, float y, bool playSound);
   long calcCircleIndex(float x, float y, long indexCount, float offset);
   void updateCircleMenuDisplay();

   void setSlotFrame(int itemType, int slotID, int frameID, int iconID, int ownerID, bool bForceToFrame);
   void setSlotIcon(int itemType, int slotID, int frameID, int iconID, int ownerID, bool bForceToFrame);

   void hideSlot(int slotID);
   void setCircleMenuIcon(const char* method, int slotID, int frame);   
   void setCircleMenuIcon(const char* method, int slotID, const char* iconName, int frameID);
   void setCircleMenuSelectionPad(int slotID, int frameID);   
   void setInfoPanelSlot(int slotID, int selectorFrameID);
   void setInfoPanelSlotData();
   void setInfoPanelDescription();
   void setInfoPanelStats();   
   void setCenterDisplayData();
   void setCenterDisplayTextVisible(bool bVisible);
   void updateResearchSlotData();
   void hideAllInfoPanels();   
   void updateItemStatData(BFlashCircleMenuItem& item);
   void updateItemUnitStatData(BFlashCircleMenuItem& item);
   void updateCostTextColors();
   void clearSelection();
   void clearCostPanelState();

   void setCircleMenuSlotIcon(int slotID, int frameID, int iconID);

   void startInfoPanelScroll(bool scrollUp);
   void stopInfoPanelScroll();

   //-- Circle Menu Data
   BDynamicSimArray<BFlashCircleMenuItem> mItems;
   int                                    mCurrentItem;
   int                                    mCurrentOrder;
   int                                    mCircleCount;
   float                                  mPointingAtX;
   float                                  mPointingAtY;
   float                                  mCurrentItemTimer;

   //-- Info Elements   
   BUString                   mBaseText;  
   BUString                   mBaseText2;  
   BUString                   mBaseTextDetail;  

   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;
   BDynamicArray<BUString>    mText;
   
   BDynamicSimArray<BFlashIconLookup> mPowerIconLookup;
   BDynamicSimArray<BFlashIconLookup> mAbilityIconLookup;
   BDynamicSimArray<BFlashIconLookup> mTechIconLookup;
   BDynamicSimArray<BFlashIconLookup> mUnitIconLookup;
   BDynamicSimArray<BFlashIconLookup> mBuildingIconLookup;
   BDynamicSimArray<BFlashIconLookup> mMiscIconLookup;
   BDynamicSimArray<BFlashIconLookup> mStatIconLookup;
   BDynamicSimArray<BFlashIconLookup> mNotificationIconLookup;
   BDynamicSimArray<BFlashIconLookup> mUnitSelectionIconLookup;
   BDynamicSimArray<BFlashIconLookup> mInputFunctionIconLookup;

   BUser*                     mpUser;

   UTBitVector<eControlTotal> mEnableStates;
   UTBitVector<8>             mInfoPanelStates;  
   BUnitSlotState             mUnitSlotID[eUnitSelectionSlotCount];
   BUnitSlotState             mUnitCard;
   int                        mPlayerColors[cMaxPlayerSlots];

   BCircleMenuSlotState       mCircleMenuState[8];   

   BButtonState               mButtonState[eControlButtonCount];
   BButtonState               mDPadIconState[eControlDPadButtonCount];
   BCostPanelStatState        mCostPanelState[cMaxCostPanelStats];

   BUString                   mUnitStatName;

   BFixedString128            mPlaceholderIcon;

   //-- cached string ID
   int                        mHUDStringID[eHUDStringTotal];
   float                      mUnitStatsData[cMaxUnitStatsData];
   int                        mCurInfoPanelSlot;

   //-- tech notification
   double                     mTechNotificationEndTimer;
   int                        mTechNotificationID;

   //-- power overlay
   int                        mPowerOverlayID;

   // Attack Alert members
   int                        mAngleNormalizer;
   bool                       mAttackSlotsStates[cMaxAttackSlots];
   bool                       mAttackSlotsStatesCache[cMaxAttackSlots];
   int                        mAttackSlotsAngleMin[cMaxAttackSlots];
   int                        mAttackSlotsAngleMax[cMaxAttackSlots];

   long                       mLastDisplayedTrainCount;

   bool                       mOverlayVisible:1;
   bool                       mUnitSelectionVisible:1;
   bool                       mUnitCardVisible:1;
   bool                       mCircleMenuVisible:1;
   bool                       mGameTimeVisible:1;
   bool                       mResourcePanelVisible:1;
   bool                       mPowerPanelVisible:1;
   bool                       mDPadVisible:1;
   bool                       mScoresVisible:1;
   bool                       mUnitStatsVisible:1;
   bool                       mButtonPanelVisible:1;
   bool                       mPowerOverlayVisible:1;
   bool                       mChatDisplayVisible:1;
   bool                       mCircleMenuExtraInfoVisible:1;
   bool                       mInfoTextNeedsUpdate:1;

};