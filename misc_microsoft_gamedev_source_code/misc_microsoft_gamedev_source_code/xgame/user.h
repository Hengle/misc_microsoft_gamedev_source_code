//==============================================================================
// user.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "ui.h"
#include "uicirclemenu.h"
#include "uilist.h"
#include "uihelp.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "math\generalVector.h"
#include "inputinterface.h"            
#include "volumeCuller.h"
#include "userminigame.h"
#include "camera.h"
#include "UIGlobals.h"
#include "uicontext.h"
#include "powertypes.h"
#include "UIManager.h"
#include "gamefilemacros.h"

#define USER_MESSAGES_MAX           20
#define USER_MESSAGES_DEFAULT_POINT 24.0f

#define ENABLE_CAMERA_BOUNDARY_LINES

// Forward declarations
class BCamera;
class BHintEngine;
class BPlayer;
class BSelectionManager;
class BTeam;
class BObject;
class BProtoObject;
class BVisual;
class BSquad;
class BUnit;
class BUserAchievementList;
class BUserProfile;
class BProtoObjectCommand;
class BUIPostGame;
class BUICampaignPostGame;
class BGeneralEventSubscriber;
class BObjectiveArrow;
class BPowerUser;
class BRenderViewParams;
class BProtoAction;

typedef uint BLocalLightHandle;


//==============================================================================
// BFlashUserMessage
//==============================================================================
class BFlashUserMessage
{
   public:

   BFlashUserMessage(long stringID, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame=false);
   BFlashUserMessage(const BUString& message, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame=false);
   ~BFlashUserMessage() {}

   void updateTime(float elapsedTime);
   bool hasExpired() const { if(mbNeverExpire) return false; return (mTimeToDisplay <= 0.0f); }
   void expire() { mTimeToDisplay = 0.0f; }

   bool getIsNew() const { return (mbIsNew); }
   bool getQueueSound() const { return (mbQueueSound); }

   void setIsNew(bool v) { mbIsNew = v; }

   // chat string
   const BUString&      getMessage() const { return mDisplayMessage; }
//   int                  getMessageStringID() const { return mStringID; }

   // chat audio
   bool                 hasSound() const { return (mSound.length() > 0); }
   const BSimString&    getSoundString() const { return mSound; }

   long                 getCueIndex() const { return mCueIndex; }
   void                 setCueIndex(long cueIndex) { mCueIndex=cueIndex; }

   bool                 existPastEndGame() const { return mbExistPastEndGame; }

protected:
   BSimString           mSound;
   BUString             mDisplayMessage;
//   long                 mStringID;
   float                mTimeToDisplay;
   long                 mCueIndex;
   long                 mStringIDIndex;

   bool                 mbIsNew        : 1;
   bool                 mbQueueSound   : 1;
   bool                 mbNeverExpire  : 1;
   bool                 mbExistPastEndGame : 1;
};


//==============================================================================
// BUserMessages
//==============================================================================
struct BUserMessages
{
   typedef enum UserMessagePlayer
   {
      cPlayer1 = 1 << 0,
      cPlayer2 = 1 << 1,
      cPlayer3 = 1 << 2,
      cPlayer4 = 1 << 3,
      cPlayer5 = 1 << 4,
      cPlayer6 = 1 << 5
   };

   BSimUString text;
   long        players;
   float       xPos;
   float       yPos;
   long        justify;
   float       scale;
   float       alpha;
   BColor      color;
   bool        enabled;

   inline void Init( void )
   {
      text    = L"";
      players = 0;
      xPos    = 0.0f;
      yPos    = 0.0f;
      justify = 0;
      scale   = 1.0f;
      alpha   = 1.0f;
      color   = cColorWhite;
      enabled = false;
   }

   inline void addPlayer( long playerID ){ players |= ( 1 << ( playerID - 1 ) ); }

   inline void addPlayer( UserMessagePlayer playerFlag ){ players |= playerFlag; }

   inline bool hasPlayer( long playerID ) const { return( ( players & ( 1 << ( playerID - 1 ) ) ) ? true : false ); }

   inline bool hasPlayer( UserMessagePlayer playerFlag ) const { return( ( players & playerFlag ) ? true : false ); }
};

//==============================================================================
// BUserUIButtonRequest
//==============================================================================
class BUserUIButtonRequest
{
   public:
      BTriggerScriptID           mTriggerScriptID;
      BTriggerVarID              mTriggerVarID;
      long                       mControlType;
      bool                       mSpeedModifier:1;
      bool                       mIgnoreSpeedModifier:1;
      bool                       mActionModifier:1;
      bool                       mIgnoreActionModifier:1;
      bool                       mOverrideGameInput:1;
      bool                       mOnRelease:1;
      bool                       mContinuous:1;
};

//==============================================================================
// BUserGameStateMessage
//==============================================================================
class BUserGameStateMessage
{
   public:
      int                        mType;
      BPlayerID                  mPlayerID;
      int                        mData;
      int                        mData2;
};



class BCameraBoundaryLineSegment
{
   public:
      BVector  mPoint0;
      BVector  mPoint1;
};

class BCameraBoundaryLine
{
   public:

      enum BCameraBoundaryType
      {
         cBoundaryHover=0,
         cBoundaryCamera,
      };

      BCameraBoundaryLine() { mType = cBoundaryHover; }

      BCameraBoundaryType        mType;
      BDynamicSimArray<BVector>  mPoints;
};

class BEntityIDFloatPair
{
public:
   BEntityID mEntityID;
   float mFloat;
};

//==============================================================================
// BUser
//==============================================================================
class BUser: public BUIGlobals::yornHandlerInterface
{
   public:     

      enum BExitMethod
      {
         cExitMethodQuit=0,
         cExitMethodRestart,
         cExitMethodContinue,
         cExitMethodGoToAdvancedTutorial,
      };

      enum
      { 
         cHoverTypeNone, 
         cHoverTypeSelect, 
         cHoverTypeGather, 
         cHoverTypeBuild,
         cHoverTypeCapture,
         cHoverTypeRepair,
         cHoverTypeGarrison, 
         cHoverTypeHitch,
         cHoverTypeEnemy, 
         cHoverTypeAbility,
         cHoverTypeMinRange,
         cHoverTypeRepairOther,
      };

      enum
      {
         cHoverResourceNone,
         cHoverResourceSupplies,
         cHoverResourceFavor,
         cHoverResourceOrganics,
         cHoverResourceRelics,
      };

      enum
      {
         cUserModeNormal = 0,
         cUserModeCircleSelecting,
         cUserModeInputUILocation,
         cUserModeRallyPoint,
         cUserModeBuildingRallyPoint,
         cUserModeBuildLocation,
         cUserModeCommandMenu,
         cUserModePowerMenu,      
         cUserModeAbility,
         cUserModeInputUIUnit,
         cUserModeInputUISquad,
         cUserModeCinematic,
         cUserModeFollow,
         cUserModeInputUISquadList,
         cUserModeInputUIPlaceSquadList,
         cUserModeInputUILocationMinigame,
         cUserModePower,
      };

      enum
      {
         cSubModeNone,
         cSubModeSelectPower,
         cSubModeUnpack,
         cSubModeTargetSelect,
      };

      enum
      {
         cGameStateMessageSupportPowerAvailable,
         cGameStateMessageResigned,
         cGameStateMessageDefeated,
         //cGameStateMessageDisconnected,
         cGameStateMessageWon,
         cGameStatePlaybackDone,
         cGameStateMessageTributeReceived,
      };

      enum
      {
         cMaxSelectedUnitIcons=8,
      };

      typedef enum USER_MESSAGE_JUSTIFY
      {
         cUserMessageJustify_Left = 0,
         cUserMessageJustify_Center,
         cUserMessageJustify_Right,
         cUserMessageJustify_NULL = -1,
      };

      enum
      {
         cInputUIModeFlagPlayerFilter,
         cInputUIModeFlagTeamFilter,
         cInputUIModeFlagObjectTypeFilter,
         cInputUIModeFlagProtoObjectFilter,
         cInputUIModeFlagProtoSquadFilter,
         cInputUIModeFlagRelationFilter,
         cInputUIModeFlagEntityFilterSet,
         cInputUIModeFlagSquadList,
      };

      enum
      {
         cMaxGroups=8,
      };

      enum
      {
         cHUDItemMinimap,
         cHUDItemResources,
         cHUDItemTime,
         cHUDItemPowerStatus,
         cHUDItemUnits,
         cHUDItemDpadHelp,
         cHUDItemButtonHelp,
         cHUDItemReticle,
         cHUDItemScore,
         cHUDItemUnitStats,
         cHUDItemCircleMenuExtraInfo,
         cNumberHUDItems
      };

      BUser();
      ~BUser();

      bool                    setup(long port);
      void                    reset();
      void                    resetUIContext();
      void                    gameInit(long playerID, long coopPlayerID, BVector lookAtPos, bool leaveCamera, bool defaultCamera, float cameraYaw, float cameraPitch, float cameraZoom);
      void                    gameRelease();
      void                    update(float elapsedTime);
      void                    renderVisualInstances();
      void                    updateViewportUI();
      void                    preRenderViewportUI();            
      void                    renderViewportUI();   
      void                    renderFrameUI();
      bool                    handleInput(long port, long event, long controlType, BInputEventDetail& detail);
      void                    changeMode(long newMode);

     
      void                    setDeviceRemoved(bool isRemoved);
      const bool              isDeviceRemoved() const { return mDeviceRemoved; } 

      XCONTENTDEVICEID        getDefaultDevice() const { return mDeviceID; }
      void                    setDefaultDevice(XCONTENTDEVICEID deviceID);
      XUID                    getXuid() const { return mXuid; }
      const BSimString&       getName() const { return mName; }
      void                    setName(BSimString newName) {mName.set(newName);};
      long                    getPort() const { return mPort; }
      void                    setPort(long port);
      void                    updateSigninStatus();
      void                    updateXuid();
      BOOL                    isSignedIn() const { return (mSignInState!=eXUserSigninState_NotSignedIn); }
      XUSER_SIGNIN_STATE      getSigninState() const { return mSignInState; }
      bool                    isLiveEnabled() const { return mLiveEnabled; }
      bool                    hasLiveStateChanged() const { return mLiveStateChanged; }
      void                    resetLiveStateChanged() { mLiveStateChanged = false; }
      void                    setLiveStateChanged() { mLiveStateChanged = true; }
      BOOL                    isSignedIntoLive() const;
      BOOL                    checkPrivilege(XPRIVILEGE_TYPE priv) const;
      BOOL                    isFriend(XUID xuid) const;
      BUserAchievementList*   getAchievementList();
      const BUserAchievementList*   getAchievementList() const;
      BUserProfile*           getProfile() { return mpProfile; }
      const BUserProfile*     getProfile() const { return mpProfile; }
      void                    setProfile(BUserProfile* pProfile);

      BCamera*                getCamera() { return getFlagFreeCamera() ? mpFreeCamera : mpCamera; }
      const BCamera*          getCamera() const { return getFlagFreeCamera() ? mpFreeCamera : mpCamera; }
      
      const BVector&          getHoverPoint() const { return mHoverPoint; }
      const float&            getCameraHoverPointOffsetHeight() const { return mCameraHoverPointOffsetHeight; }
      void                    setCameraHoverPointOffsetHeight(float cameraHoverPointOffsetHeight) { mCameraHoverPointOffsetHeight = cameraHoverPointOffsetHeight; }
      const BVector&          getCameraHoverPoint() const { return mCameraHoverPoint; }
      void                    setCameraHoverPoint(BVector cameraHoverPoint) { mCameraHoverPoint = cameraHoverPoint; }
      BEntityID               getHoverObject() const { return mHoverObject; }
      long                    getHoverType() const { return mHoverType; }
      long                    getHoverResource() const { return mHoverResource; }
      long                    getHoverHitZoneIndex() const { return( mHoverHitZoneIndex ); }

      long                    getPlayerID() const { return mPlayerID; }
      long                    getCoopPlayerID() const { return mCoopPlayerID; }
      BPlayer*                getPlayer();
      const BPlayer*          getPlayer() const;
      BPlayer*                getCoopPlayer();
      const BPlayer*          getCoopPlayer() const;
      BTeamID                 getTeamID() const { return mTeamID; }
      long                    getUserMode() const { return mUserMode; }
      long                    getSubMode() const { return mSubMode; }
      BEntityID               getCommandObject() const { return mCommandObject; }
      BSelectionManager*      getSelectionManager() { return mSelectionManager; }
      const BSelectionManager*  getSelectionManager() const { return mSelectionManager; }

      BUICircleMenu &         getCircleMenu() { return mCircleMenu; }      
      const BUICircleMenu &   getCircleMenu() const { return mCircleMenu; }      

      int                     getLastUserMode() const { return (mLastUserMode); }

      float                   getCameraZoomMin() const { return mCameraZoomMin; }
      float                   getCameraZoomMax() const { return mCameraZoomMax; }
      float                   getCameraPitchMin() const { return mCameraPitchMin; }
      float                   getCameraPitchMax() const { return mCameraPitchMax; }
      float                   getDefaultPitch() const { return mCameraDefaultPitch; }
      float                   getDefaultYaw() const { return mCameraDefaultYaw; }
      float                   getDefaultZoom() const { return mCameraDefaultZoom; }
      bool                    getDefaultCamera() const { return mFlagDefaultCamera; }

      void setUIModeRestoreCameraZoomMin(float v) { mUIModeRestoreCameraZoomMin = v; mFlagUIModeRestoreCameraZoomMin = true; }
      void setUIModeRestoreCameraZoomMax(float v) { mUIModeRestoreCameraZoomMax = v; mFlagUIModeRestoreCameraZoomMax = true; }
      void setUIModeRestoreCameraZoom(float v) { mUIModeRestoreCameraZoom = v; mFlagUIModeRestoreCameraZoom = true; }
      void setUIModeRestoreCameraPitchMin(float v) { mUIModeRestoreCameraPitchMin = v; mFlagUIModeRestoreCameraPitchMin = true; }
      void setUIModeRestoreCameraPitchMax(float v) { mUIModeRestoreCameraPitchMax = v; mFlagUIModeRestoreCameraPitchMax = true; }
      void setUIModeRestoreCameraPitch(float v) { mUIModeRestoreCameraPitch = v; mFlagUIModeRestoreCameraPitch = true; }
      void setUIModeRestoreCameraYaw(float v) { mUIModeRestoreCameraYaw = v; mFlagUIModeRestoreCameraYaw = true; }
      void setUIModeRestoreCameraEnableUserScroll(bool v) { mFlagRestoreCameraEnableUserScroll = v; }
      void setUIModeRestoreCameraEnableUserYaw(bool v) { mFlagRestoreCameraEnableUserYaw = v; }
      void setUIModeRestoreCameraEnableUserZoom(bool v) { mFlagRestoreCameraEnableUserZoom = v; }
      void setUIModeRestoreCameraEnableAutoZoomInstant(bool v) { mFlagRestoreCameraEnableAutoZoomInstant = v; }
      void setUIModeRestoreCameraEnableAutoZoom(bool v) { mFlagRestoreCameraEnableAutoZoom = v; }

      void restoreUIModeCameraLimitValues();
      void clearUIModeCameraLimitValues();

      BUIContext*             getUIContext() const { return mpUIContext; };

      void                    setCommandObject(BEntityID id) { mCommandObject=id; }

      // Follow
      BEntityID               getFollowEntity() const { return (mFollowEntity); }
      void                    setFollowEntity(BEntityID entity) { mFollowEntity = entity; }

      bool                    isUserLocked(void) const;
      bool                    isUserLockedByTriggerScript(BTriggerScriptID triggerScriptID) const;
      void                    lockUser(BTriggerScriptID triggerScriptID = cInvalidTriggerScriptID);
      void                    unlockUser(void);

      void                    circleSelect(bool on, bool clearSelections = true);
      void                    circleSelectResize(float x, float y);
      void                    resetCircleSelectionCycle();
      void                    clearAllSelections();

      bool                    doWorkAtCurrentLocation(long abilityID, long squadMode, bool noTarget, bool targetUnit, BEntityIDArray* pSquads);
      bool                    doAttackMove();
      void                    doSquadMode();
      bool                    doManualBuild();
      bool                    doUnpack();
      bool                    doAbility();
      void                    destroyAtCursor();
      void                    destroyUnitAtCursor();
      bool                    doTribute(BProtoObjectCommand& command);
      bool                    doReverseHotDrop(BProtoObjectCommand& command);
      void                    doRepair();

      void                    resetCameraDefaults();
      void                    resetCameraSettings(bool onlyZoomAndPitch=false);
      void                    applyCameraSettings(bool keepLookAtPos);
      float                   getCameraPitch() const { return mCameraPitch; }
      float                   getCameraYaw() const { return mCameraYaw; }
      float                   getCameraZoom() const { return mCameraZoom; }
      float                   getCameraFOV() const { return mCameraFOV; }
      void                    setCameraPitch(float val) { mCameraPitch=val; }
      void                    setCameraYaw(float val) { mCameraYaw=val; }
      void                    setCameraZoom(float val) { mCameraZoom=val; }
      void                    setCameraFOV(float val) { mCameraFOV=val; }
      void                    updatePowerCameraLimits();
      void                    updateModeCameraEffect(long oldMode, long newMode);

      void                    setUILocationCheckObstruction(bool v) { mUILocationCheckObstruction = v; }
      void                    setUILocationLOSCenterOnly(bool v) { mUILocationLOSCenterOnly = v; }
      void                    setUILocationLOSType(long v) { mUILocationLOSType = v; }
      void                    setUILocationPlacementRuleIndex(long v) { mUILocationPlacementRuleIndex = v; }
      void                    setUILocationCheckMoving(bool v) { mUILocationCheckMoving = v; }
      void                    setUILocationPlacementSuggestion(bool v) { mUILocationPlacementSuggestion = v; }
      void                    setUIPowerRadius(float v) { mUIPowerRadius = v; }
      void                    setUIProtoPowerID(int v) { mUIProtoPowerID = v; }
      int                     getUIProtoPowerID() const { return mUIProtoPowerID; }
      void                    setTriggerScriptID(BTriggerScriptID v) { mTriggerScriptID = v; }
      void                    setTriggerVarID(BTriggerVarID v) { mTriggerVarID = v; }
      void                    setBuildProtoID(long protoObjectID, long protoSquadID = cInvalidProtoSquadID, bool useSquadObs = false) { mBuildProtoID = protoObjectID; mBuildProtoSquadID = protoSquadID; mBuildUseSquadObstruction = useSquadObs; }
      BTriggerScriptID        getTriggerScriptID() const { return mTriggerScriptID; }
      BTriggerVarID           getTriggerVarID() const { return mTriggerVarID; }
      void                    setAbilityID(long v) { mAbilityID = v; }
      long                    getAbilityID(void) const { return(mAbilityID); }
      void                    setMinigameTimeFactor(float v) { mMinigameTimeFactor = v; }

      void                    addTriggerUIButtonRequest(long scriptID, BTriggerVarID varID, long controlType, bool speedModifier, bool ignoreSpeedModifier, bool actionModifier, bool ignoreActionModifier, bool overrideGameInput, bool onRelease, bool continuous);

      void                    switchPlayer(long playerID);
      void                    handlePlayerSwitchForUI();

      void                    notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

      enum { cFindCrowdMilitary, cFindCrowdIdleVillager, };
      bool                    findCrowd(long findType, bool noLookAt, bool checkOnly, long* pGotoTypeOut);
      void                    resetFindCrowd();

      bool                    gotoItem(int gotoType, bool checkOnly, BEntityID* pReturnID = NULL);

      static void             setUserMessage( long index, BPlayerIDArray playerList, BSimUString* text, float xPos, float yPos, USER_MESSAGE_JUSTIFY justify, float point, float alpha, BColor color, bool enabled  );

      int                     getGroupIndex() const { return mGroupIndex; }
      bool                    isGroupAssigned(long groupID) const { return (mGroups[groupID].getNumber()>0); }
      bool                    isAnyGroupAssigned() const;

      // Objective Arrow
      void                     setObjectiveArrow(int index, BVector origin, BVector target, bool visible, bool useTarget, bool forceTargetVisible = false);
      void                     updateObjectiveArrows();

      void                    applyScreenBlur(bool active);


      // UI input mode (trigger related) filter stuff...
      bool                    testUnitAgainstInputUIModeFilters(const BUnit* pUnit);
      bool                    testSquadAgainstInputUIModeFilters(const BSquad* pSquad);
      BEntityIDArray          testSquadListAgainstInputUIModeEntityFilterSet(const BEntityIDArray& squadList);
      bool                    useInputUIModePlayerFilter() const { return (mInputUIModeFlags.isSet(BUser::cInputUIModeFlagPlayerFilter) != 0); }
      bool                    useInputUIModeTeamFilter() const { return (mInputUIModeFlags.isSet(BUser::cInputUIModeFlagTeamFilter) != 0); }
      bool                    useInputUIModeObjectTypeFilter() const { return (mInputUIModeFlags.isSet(BUser::cInputUIModeFlagObjectTypeFilter) != 0); }
      bool                    useInputUIModeProtoObjectFilter() const { return (mInputUIModeFlags.isSet(BUser::cInputUIModeFlagProtoObjectFilter) != 0); }
      bool                    useInputUIModeProtoSquadFilter() const { return (mInputUIModeFlags.isSet(BUser::cInputUIModeFlagProtoSquadFilter) != 0); }
      bool                    useInputUIModeRelationFilter() const { return (mInputUIModeFlags.isSet(BUser::cInputUIModeFlagRelationFilter) != 0); }
      bool                    useInputUIModeEntityFilterSet() const { return (mInputUIModeFlags.isSet(BUser::cInputUIModeFlagEntityFilterSet) != 0); }
      void                    setInputUIModePlayerFilter(const BPlayerIDArray& playerFilter) { if (playerFilter.getNumber() > 0){ mInputUIModeFlags.set(BUser::cInputUIModeFlagPlayerFilter); mInputUIModePlayerFilter = playerFilter; } }
      void                    setInputUIModeTeamFilter(const BTeamIDArray& teamFilter) { if (teamFilter.getNumber() > 0){ mInputUIModeFlags.set(BUser::cInputUIModeFlagTeamFilter); mInputUIModeTeamFilter = teamFilter; } }
      void                    setInputUIModeObjectTypeFilter(const BObjectTypeIDArray& objectTypeFilter){ if (objectTypeFilter.getNumber() > 0){ mInputUIModeFlags.set(BUser::cInputUIModeFlagObjectTypeFilter); mInputUIModeObjectTypeFilter = objectTypeFilter; } }
      void                    setInputUIModeProtoObjectFilter(const BProtoObjectIDArray& protoObjectFilter){ if (protoObjectFilter.getNumber() > 0){ mInputUIModeFlags.set(BUser::cInputUIModeFlagProtoObjectFilter); mInputUIModeProtoObjectFilter = protoObjectFilter; } }
      void                    setInputUIModeProtoSquadFilter(const BProtoSquadIDArray& protoSquadFilter){ if (protoSquadFilter.getNumber() > 0){ mInputUIModeFlags.set(BUser::cInputUIModeFlagProtoSquadFilter); mInputUIModeProtoSquadFilter = protoSquadFilter; } }
      void                    setInputUIModeRelationFilter(const BRelationTypeArray& relationFilter){ if (relationFilter.getNumber() > 0){ mInputUIModeFlags.set(BUser::cInputUIModeFlagRelationFilter); mInputUIModeRelationFilter = relationFilter; } }
      void                    setInputUIModeEntityFilterSet(BEntityFilterSet* entityFilterSet) { if (entityFilterSet && !entityFilterSet->isEmpty()) { mInputUIModeFlags.set(BUser::cInputUIModeFlagEntityFilterSet); mInputUIModeEntityFilterSet = entityFilterSet; } }
      void                    appendInputUIModePlayerFilter(BPlayerID playerID){ mInputUIModeFlags.set(BUser::cInputUIModeFlagPlayerFilter); mInputUIModePlayerFilter.uniqueAdd(playerID); }
      void                    appendInputUIModeTeamFilter(BTeamID teamID){ mInputUIModeFlags.set(BUser::cInputUIModeFlagTeamFilter); mInputUIModeTeamFilter.uniqueAdd(teamID); }
      void                    appendInputUIModeObjectTypeFilter(BObjectTypeID objectTypeID){ mInputUIModeFlags.set(BUser::cInputUIModeFlagObjectTypeFilter); mInputUIModeObjectTypeFilter.uniqueAdd(objectTypeID); }
      void                    appendInputUIModeProtoObjectFilter(BProtoObjectID protoObjectID){ mInputUIModeFlags.set(BUser::cInputUIModeFlagProtoObjectFilter); mInputUIModeProtoObjectFilter.uniqueAdd(protoObjectID); }
      void                    appendInputUIModeProtoSquadFilter(BProtoSquadID protoSquadID){ mInputUIModeFlags.set(BUser::cInputUIModeFlagProtoSquadFilter); mInputUIModeProtoSquadFilter.uniqueAdd(protoSquadID); }
      void                    appendInputUIModeRelationFilter(BRelationType relationType){ mInputUIModeFlags.set(BUser::cInputUIModeFlagRelationFilter); mInputUIModeRelationFilter.uniqueAdd(relationType); }
      void                    resetInputUIModePlayerFilter(){ mInputUIModeFlags.unset(BUser::cInputUIModeFlagPlayerFilter); mInputUIModePlayerFilter.clear(); }
      void                    resetInputUIModeTeamFilter(){ mInputUIModeFlags.unset(BUser::cInputUIModeFlagTeamFilter); mInputUIModeTeamFilter.clear(); }
      void                    resetInputUIModeObjectTypeFilter(){ mInputUIModeFlags.unset(BUser::cInputUIModeFlagObjectTypeFilter); mInputUIModeObjectTypeFilter.clear(); }
      void                    resetInputUIModeProtoObjectFilter(){ mInputUIModeFlags.unset(BUser::cInputUIModeFlagProtoObjectFilter); mInputUIModeProtoObjectFilter.clear(); }
      void                    resetInputUIModeProtoSquadFilter(){ mInputUIModeFlags.unset(BUser::cInputUIModeFlagProtoSquadFilter); mInputUIModeProtoSquadFilter.clear(); }
      void                    resetInputUIModeRelationFilter(){ mInputUIModeFlags.unset(BUser::cInputUIModeFlagRelationFilter); mInputUIModeRelationFilter.clear(); }
      void                    resetInputUIModeEntityFilterSet() { mInputUIModeFlags.unset(BUser::cInputUIModeFlagEntityFilterSet); mInputUIModeEntityFilterSet=NULL; }
      void                    resetAllInputUIModeData(){ resetInputUIModePlayerFilter(); resetInputUIModeTeamFilter(); resetInputUIModeObjectTypeFilter(); resetInputUIModeProtoObjectFilter(); resetInputUIModeProtoSquadFilter(); resetInputUIModeRelationFilter(); resetInputUIModeEntityFilterSet(); }

      // UI input mode (trigger related) squad placement stuff...
      void                    setInputUIModeSquadList(const BEntityIDArray& squadList) { mInputUIModeSquadList = squadList; }      
      const BEntityIDArray&   getInputUIModeSquadList() const { return (mInputUIModeSquadList); }

      bool                    getFlagUserActive() const { return mFlagUserActive; }
      void                    setFlagUserActive(bool v) { mFlagUserActive=v; }
      bool                    getFlagUpdateHoverPoint() const { return(mFlagUpdateHoverPoint); }
      void                    setFlagUpdateHoverPoint(bool v) { mFlagUpdateHoverPoint=v; }
      bool                    getFlagHaveHoverPoint() const { return(mFlagHaveHoverPoint); }
      void                    setFlagHaveHoverPoint(bool v) { mFlagHaveHoverPoint=v; }
      bool                    getFlagHoverPointOverTerrain() const { return(mFlagHoverPointOverTerrain); }
      void                    setFlagHoverPointOverTerrain(bool v) { mFlagHoverPointOverTerrain=v; }
      bool                    getFlagCircleSelectPoints() const { return(mFlagCircleSelectPoints); }
      void                    setFlagCircleSelectPoints(bool v) { mFlagCircleSelectPoints=v; }
      bool                    getFlagModifierSpeed() const { return(mFlagModifierSpeed); }
      void                    setFlagModifierSpeed(bool v) { mFlagModifierSpeed=v; }
      bool                    getFlagModifierAction() const { return(mFlagModifierAction); }
      void                    setFlagModifierAction(bool v) { mFlagModifierAction=v; }
      bool                    getFlagCircleSelecting() const { return(mFlagCircleSelecting); }
      void                    setFlagCircleSelecting(bool v) { mFlagCircleSelecting=v; }
      bool                    getFlagCircleSelectReset() const { return(mFlagCircleSelectReset); }
      void                    setFlagCircleSelectReset(bool v) { mFlagCircleSelectReset=v; }
      bool                    getFlagCircleSelectSelected() const { return mFlagCircleSelectSelected; }
      void                    setFlagCircleSelectSelected(bool v) { mFlagCircleSelectSelected=v; }
      bool                    getFlagCircleSelectGrow() const { return(mFlagCircleSelectGrow); }
      void                    setFlagCircleSelectGrow(bool v) { mFlagCircleSelectGrow=v; }
      bool                    getFlagCircleSelectFullyGrown() const { return(mFlagCircleSelectFullyGrown); }
      void                    setFlagCircleSelectFullyGrown(bool v) { mFlagCircleSelectFullyGrown=v; }
      bool                    getFlagCircleSelectShrink() const { return(mFlagCircleSelectShrink); }
      void                    setFlagCircleSelectShrink(bool v) { mFlagCircleSelectShrink=v; }
      bool                    getFlagCircleSelectFullyShrunk() const { return(mFlagCircleSelectFullyShrunk); }
      void                    setFlagCircleSelectFullyShrunk(bool v) { mFlagCircleSelectFullyShrunk=v; }
      bool                    getFlagCircleSelectDoNotification() const { return(mFlagCircleSelectDoNotification); }
      void                    setFlagCircleSelectDoNotification(bool v) { mFlagCircleSelectDoNotification=v; }
      bool                    getFlagCircleSelectVinceEvent() const { return mFlagCircleSelectVinceEvent; }
      void                    setFlagCircleSelectVinceEvent(bool v) { mFlagCircleSelectVinceEvent=v; }
      bool                    getFlagDelayScrolling() const { return(mFlagDelayScrolling); }
      void                    setFlagDelayScrolling(bool v) { mFlagDelayScrolling=v; }
      bool                    getFlagDelayScrollingStart() const { return(mFlagDelayScrollingStart); }
      void                    setFlagDelayScrollingStart(bool v) { mFlagDelayScrollingStart=v; }
      bool                    getFlagGameActive() const { return(mFlagGameActive); }
      void                    setFlagGameActive(bool v) { mFlagGameActive=v; }
      bool                    getFlagGameDoExit() const { return(mFlagGameDoExit); }
      void                    setFlagGameDoExit(bool v) { mFlagGameDoExit=v; }
      bool                    getFlagCommandMenuRefresh() const { return(mFlagCommandMenuRefresh); }
      void                    setFlagCommandMenuRefresh(bool v) { mFlagCommandMenuRefresh=v; }
      bool                    getFlagCommandMenuRefreshTrainProgressOnly() const { return(mFlagCommandMenuRefreshTrainProgressOnly); }
      void                    setFlagCommandMenuRefreshTrainProgressOnly(bool v) { mFlagCommandMenuRefreshTrainProgressOnly=v; }
   
      bool                    getFlagPowerMenuRefreshTrainProgressOnly() const { return(mFlagPowerMenuRefreshTrainProgressOnly); }
      void                    setFlagPowerMenuRefreshTrainProgressOnly(bool v) { mFlagPowerMenuRefreshTrainProgressOnly=v; }

      bool                    getFlagPowerMenuRefresh() const { return(mFlagPowerMenuRefresh); }
      void                    setFlagPowerMenuRefresh(bool v) { mFlagPowerMenuRefresh=v; }
      bool                    getFlagExitModeOnModifierRelease() const { return(mFlagExitModeOnModifierRelease); }
      void                    setFlagExitModeOnModifierRelease(bool v) { mFlagExitModeOnModifierRelease=v; }
      bool                    getFlagNoCameraLimits() const { return(mFlagNoCameraLimits); }
      void                    setFlagNoCameraLimits(bool v) { mFlagNoCameraLimits=v; }
      bool                    getFlagFreeCamera() const { return(mFlagFreeCamera); }
      void                    setFlagFreeCamera(bool v) { mFlagFreeCamera=v; }
      bool                    getFlagUILocationValid() const { return(mFlagUILocationValid); }
      void                    setFlagUILocationValid(bool v) { mFlagUILocationValid=v; }
      bool                    getFlagGameStateMessage() const { return(mFlagGameStateMessage); }
      void                    setFlagGameStateMessage(bool v) { mFlagGameStateMessage=v; }
      bool                    getFlagHoverLight() const { return(mFlagHoverLight); }
      void                    setFlagHoverLight(bool v) { mFlagHoverLight=v; }
      bool                    getFlagUIUnitValid() const { return(mFlagUIUnitValid); }
      void                    setFlagUIUnitValid(bool v) { mFlagUIUnitValid=v; }
      bool                    getFlagUISquadValid() const { return(mFlagUISquadValid); }
      void                    setFlagUISquadValid(bool v) { mFlagUISquadValid=v; }
//       bool                    getFlagInvertTilt() const { return(mFlagInvertTilt); }
//       void                    setFlagInvertTilt(bool v) { mFlagInvertTilt=v; }
//       bool                    getFlagInvertPan() const { return(mFlagInvertPan); }
//       void                    setFlagInvertPan(bool v) { mFlagInvertPan=v; }
      bool                    getFlagUserLocked() const { return(mFlagUserLocked); }
      void                    setFlagUserLocked(bool v) { mFlagUserLocked=v; }  
      bool                    getFlagPowerMenuOverride() const { return (mFlagPowerMenuOverride); }
      void                    setFlagPowerMenuOverride(bool v) { mFlagPowerMenuOverride = v; }
      void                    setFlagPowerMenuEnable(bool v) { mFlagPowerMenuEnable = v; }
      bool                    getFlagPowerMenuEnable() const { return (mFlagPowerMenuEnable); }
      bool                    getFlagCameraScrollEnabled() const { return (mFlagCameraScrollEnabled); }
      void                    setFlagCameraScrollEnabled(bool v) { mFlagCameraScrollEnabled=v; }
      bool                    getFlagCameraYawEnabled() const { return (mFlagCameraYawEnabled); }
      void                    setFlagCameraYawEnabled(bool v) { mFlagCameraYawEnabled=v; }
      bool                    getFlagCameraZoomEnabled() const { return (mFlagCameraZoomEnabled); }      
      void                    setFlagCameraZoomEnabled(bool v) { mFlagCameraZoomEnabled=v; }
      bool                    getFlagCameraAutoZoomInstantEnabled() const { return (mFlagCameraAutoZoomInstantEnabled); }
      void                    setFlagCameraAutoZoomInstantEnabled(bool v) { mFlagCameraAutoZoomInstantEnabled = v; }
      bool                    getFlagCameraAutoZoomEnabled() const { return (mFlagCameraAutoZoomEnabled); }
      void                    setFlagCameraAutoZoomEnabled(bool v) { mFlagCameraAutoZoomEnabled = v; }
//       bool                    getFlagUserCameraRotation() const { return (mFlagUserCameraRotation); }
//       void                    setFlagUserCameraRotation(bool v) { mFlagUserCameraRotation=v; }
//       bool                    getFlagContextHelp() const { return (mFlagContextHelp); }
//       void                    setFlagContextHelp(bool v) { mFlagContextHelp = v; }
//       bool                    getFlagSubtitles() const { return (mFlagSubtitles); }
//       void                    setFlagSubtitles(bool v) { mFlagSubtitles = v; }
//       bool                    getFlagFriendOrFoe() const { return (mFlagFriendOrFoe); }
//       void                    setFlagFriendOrFoe(bool v) { mFlagFriendOrFoe = v; }
      void                    setFlagUIPowerMinigame(bool v) { mFlagUIPowerMinigame = v; }
      void                    setFlagUIRestartMinigame(bool v) { mFlagUIRestartMinigame = v; }
      bool                    getFlagTeleportCamera() const { return(mFlagCameraTeleport); }
      void                    setFlagTeleportCamera(bool v) { mFlagCameraTeleport=v; }
      bool                    getFlagIgnoreDpad() const { return (mFlagIgnoreDpad); }
      void                    setFlagIgnoreDpad(bool v) { mFlagIgnoreDpad = v; }
      bool                    getFlagClearCircleMenuDisplay() const { return(mFlagClearCircleMenuDisplay); }
      void                    setFlagClearCircleMenuDisplay(bool v) { mFlagClearCircleMenuDisplay=v; }


//============================================================================
// OPTIONS
public:
   bool mShowButtonHelp; // Remove after E3
// This macro simply declares get and set methods for the options.
#define DECLARE_OPTION( varType, optionName )\
   varType getOption_##optionName( void ) const;\
   void setOption_##optionName( varType v );

      DECLARE_OPTION(bool, ShowButtonHelp );
      DECLARE_OPTION(bool,  AIAdvisorEnabled );
      DECLARE_OPTION(uint8, DefaultAISettings );
      DECLARE_OPTION(uint8, DefaultCampaignDifficulty);
      DECLARE_OPTION(uint8, ControlScheme );
      DECLARE_OPTION(bool,  RumbleEnabled );
      DECLARE_OPTION(bool,  CameraRotationEnabled );
      DECLARE_OPTION(bool,  XInverted );
      DECLARE_OPTION(bool,  YInverted );
      DECLARE_OPTION(uint8, DefaultZoomLevel );
      DECLARE_OPTION(uint8, StickyCrosshairSensitivity );
      DECLARE_OPTION(uint8, ScrollSpeed );
      DECLARE_OPTION(bool,  CameraFollowEnabled );
      DECLARE_OPTION(bool,  HintsEnabled );
      DECLARE_OPTION(uint8, SelectionSpeed );
      DECLARE_OPTION(uint8, MusicVolume );
      DECLARE_OPTION(uint8, SFXVolume );
      DECLARE_OPTION(uint8, VoiceVolume );
      DECLARE_OPTION(bool,  SubtitlesEnabled );
      DECLARE_OPTION(bool,  ChatTextEnabled );
      DECLARE_OPTION(uint8, Brightness );
      DECLARE_OPTION(bool,  FriendOrFoeColorsEnabled );
      DECLARE_OPTION(bool,  MiniMapZoomEnabled );

      // E3 Cheats
      DECLARE_OPTION(bool,  ResourceBoost );
      DECLARE_OPTION(bool,  FogOfWarEnabled )

      bool getOptionByName( const char* optionName, uint8& optionValue ) const;
      bool setOptionByName( const char* optionName, uint8 optionValue );

      bool getOptionByName( const char* optionName, bool& optionValue ) const;
      bool setOptionByName( const char* optionName, bool optionValue );

      bool getOptionRangeByName( const char* optionName, uint8& optionMin, uint8& optionMax );
      bool getOptionDefaultByName( const char* optionName, uint8& optionDef );

      bool resetOptionByName( const char* optionName );
      void resetAllOptions( void );
      void applyAllOptions( void );

      void validateUintOptions( void );

      static void initOptionMaps( void );

protected:
      // Option Map Stuff
      //----------------------------------------------------------------------------------
      // [PChapman - 6/18/08]
      // The Options Menu is data-driven and options need to be specified in XML. In order
      // to get/set options by name, I needed to create a map of this sort:
      // 
      //    { Key, Value } = { string Name, struct Option }
      // 
      // where the Option struct contains get/set functions (pointer-to-method, actually)
      // and a default value. Because the options can be either booleans or bytes, I need
      // 2 maps, one for each type.
      //----------------------------------------------------------------------------------
      struct SBoolOption
      {
         bool (BUser::*get)( void ) const;
         void (BUser::*set)( const bool optionValue );
         bool defaultValue;
      };
      typedef BHashMap<BString, SBoolOption> BoolOptionMap;
      static BoolOptionMap mBoolOptionMap;

      struct SUint8Option
      {
         uint8 (BUser::*get)( void ) const;
         void (BUser::*set)( const uint8 optionValue );
         uint8 minValue, defaultValue, maxValue;
      };
      typedef BHashMap<BString, SUint8Option> Uint8OptionMap;
      static Uint8OptionMap mUint8OptionMap;
//============================================================================

public:
      #ifndef BUILD_FINAL
      void                    multiplayerCheatNotification(long playerID, const char* pCheatType);
      #endif

      //bool                    invokePower(int protoPowerID, BEntityID squadID);
      bool                    invokePower(int protoPowerID, BEntityID squadID, bool noCost = false, bool overridePowerLevel = false, BPowerLevel overrideLevel = 0);

      bool                    invokePower2(BProtoPowerID protoPowerID); // new power system



      bool                    canRepair(BCost* pCost) const;

      void                    resetUserMode();

      bool                    isSphereVisible(XMVECTOR center, float radius) const;

//      long                    getConfigIndex() const { return mConfigIndex; }

      void                    updateSelectionChangeForHover();

      bool                    isPointingAt(BEntityID entityID);


      void                    setHUDItemEnabled(int hudItem, bool onoff);
      bool                    getHUDItemEnabled(int hudItem) const { return mHUDItemEnabled[hudItem]; }

      void                    setFlagOverrideUnitPanelDisplay(bool value) { mFlagOverrideUnitPanelDisplay = value; }
      bool                    getFlagOverrideUnitPanelDisplay() { return mFlagOverrideUnitPanelDisplay; }

      // yorn handler
      virtual void yornResult(uint result, DWORD userContext, int port);

      // UIManager Result Handler
      virtual void handleUIManagerResult( BUIManager::EResult eResult );

      void                    addFlashUserMessage(long stringID, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame=false);
      void                    addFlashUserMessage(const BUString& message, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame=false);

      void                    endGameOOS();
      void                    endGameDisconnect();

      static const char*      getUserModeName(int userMode);

      void                    uiSelectAll(bool global, bool reverse, bool autoGlobal, bool autoCycle, bool selectAndCycle, bool noSound);
      void                    uiCycleGroup(bool reverse, bool wrap);
      void                    uiCycleTarget(bool reverse, bool userInitiated);
      void                    uiSelectTarget();
      void                    uiCancel();
      void                    uiModifierAction(bool on);
      void                    uiModifierSpeed(bool on);
      void                    uiFlare(float x, float y);
      void                    uiMenu();
      void                    uiObjectives();

      void                    commandSquad(BEntityID squadID);
      void                    updateCommandSquads(float elapsedTime);
      void                    renderCommandSquads();


      void                    setSaveGame(BSimString& fileName) { mSaveGameFile=fileName; }
      const BSimString&       getSaveGameFile() const { return mSaveGameFile; }

      // New power stuff.
      uint                    getNextPowerUserRefCount() const { return (mNextPowerUserRefCount); }
      BPowerUser*             getPowerUser() const { return (mpPowerUser); }
      BPowerUser*             createPowerUser(BProtoPowerID protoPowerID, BPowerLevel powerLevel,  BEntityID ownerSquadID, bool noCost = false);
      BPowerUser*             allocatePowerUser(BPowerType powerType);
      void                    deletePowerUser();
      void                    updateFriends();
      const BDynamicArray<XONLINE_FRIEND> & getFriends() const;
      // Make the PowerUser derived classes friend classes.
      // Usually 'icky' but in this case these are extensions of BUser so it's ok.
      friend class BPowerUserCleansing;
      friend class BPowerUserOrbital;
      friend class BPowerUserCarpetBombing;
      friend class BPowerUserCryo;
      friend class BPowerUserRage;
      friend class BPowerUserWave;
      friend class BPowerUserDisruption;
      friend class BPowerUserRepair;
      friend class BPowerUserTransport;
      friend class BPowerUserODST;


      void  clearCameraBoudnaryLines();
      void  addCameraBoundaryLine(BCameraBoundaryLine line);

      void                    setInterpData(float time, const BVector& position, const float* yaw = NULL, const float* pitch = NULL, const float* zoom = NULL);
      bool                    isInterpolatingCamera() const { return mInterpTimeLeft > cFloatCompareEpsilon; }

      BPlayerState getPlayerState() const { return mPlayerState; }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      void                    clearCameraState();

      void                    computeClosestCameraHoverPointVertical(BVector position, BVector &newCameraHoverPoint);
      void                    computeClosestCameraHoverPointInDirection(BVector lookAtPosition, BVector lookAtDirection, BVector &newCameraHoverPoint);
      void                    processAutoDifficulty();      // If auto difficulty selected, update parms if appropriate

   protected:
      uint                    getGoodAgainst(BSquad * pSelectedSquad, BEntityID possibleID);

      void                    exitGameMode(BExitMethod method);
      void                    processGameOver();

      // user flash messages
      void                    updateFlashUserMessages(float elapsedTime);
      BFlashUserMessage*      getFlashUserMessage();
      void                    removeFlashUserMessage(BFlashUserMessage* pMessage);


      void                    updateChatMessages(float elapsedTime);
      void                    updateUIHints(float elapsedTime);
      void                    updateObjectiveMessages(float elapsedTime);
      void                    updateGameStateMessages(float elapsedTime);
      void                    updateDebug(float elapsedTime);
      void                    updateScrolling(float elapsedTime);
      void                    toggleFreeCamera(void);
      void                    updateFreeCamera(float elapsedTime);
      void                    updateGotoCamera(float elapsedTime);
      void                    updateInterpCamera(float elapsedTime);
      void                    updateCamera(float elapsedTime);
      bool                    updateHoverPoint(float xScale=1.0f, float yScale=1.0f, bool noRumble=false);
      void                    updateHoverVisual(float elapsedTime);
      void                    updateCircleSelect(float elapsedTime);
      void                    updateCircleSelectShrink(float elapsedTime);
      void                    updateUIContext(float elapsedTime);

      void                    updateSelectedUnitIcons();
      bool                    clampCamera();
      void                    shakeCamera();
      void                    updateMinigame();
      
      bool                    updateHoverType(BSquad* pSelectedSquad, BEntityID hoverObject);
      void                    updateHoverHitZone();

      bool                    handleInputFreeCamera(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputScrollingAndCamera(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputNormal(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputGoto(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputCircleSelect(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputUILocation(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputUILocationMinigame(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputPower(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputUIUnit(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputUISquad(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputUISquadList(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputUIPlaceSquadList(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputRallyPoint(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputBuildingRallyPoint(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputBuildLocation(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputAbility(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputSquadMode(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputCommandMenu(long port, long event, long controlType, BInputEventDetail& detail);
      bool                    handleInputPowerMenu(long port, long event, long controlType, BInputEventDetail& detail);
      
      void                    renderReticle(long reticleType, uint goodAgainstRating = 0, const BCost* pCost=NULL);
      void                    setReticleMode(long reticleType, uint goodAgainstRating = 0, const BCost* pCost=NULL);

      void                    renderPlayerStats();
      void                    renderGameTime();
      void                    renderSelectedUnitIcons();
      void                    renderEntityIDs();
      void                    renderGameStateMessages();
      void                    drawObjectSelectedShape(const BObject* pObject, DWORD color, bool useObstructionRadius, bool drawRanges);      
      void                    drawSelectionCircleDecal(BVector pos, DWORD color, float size, float intensity, float offset);
      void                    drawHoverDecal(BVector pos, DWORD color, float size, float intensity, float offset);
      void                    renderUserMessages();
      void                    renderPlayableBorder();
      void                    renderCameraBoundaryLines();


      #ifndef BUILD_FINAL
      void                    renderDebugMenuInfo();
      #endif

      void                    showGameMenu();
      bool                    showCommandMenu(BEntityID useObjectID);
      void                    showPowerMenu();
      void                    showSelectPowerMenu(uint supportPowerIndex, int supportPowerIconLocation);
      void                    showObjectiveMenu();

      bool                    refreshCommandMenu();
      void                    refreshPowerMenu();
      void                    refreshScores();

      void                    addTributeCommands();

      void                    setHoverVisual(long protoObjectID);
      void                    releaseHoverVisual();

      bool                    playUnitSelectSound(BEntityID id, bool suppressBankLoad=false);
      void                    playSelectedSounds();

      void                    doubleClickSelect();

      void                    setGotoEntity(const BEntityID& entityId, int gotoType);
      void                    setGotoPosition(const BVector& position, int gotoType);
      void                    resetGotoCameraData();
      void                    resetGotoSelected();

      void                    gotoAlert();
      void                    gotoSelected();

      void                    sendFlare(int flareType, BVector pos);

      void                    refreshCommandMenu(long playerID, long unitID, long itemType, long itemID, bool bProgressUpdateOnly);
      void                    refreshPowerMenu(long playerID, long protoPowerID);

      void                    addGameStateMessage(int messageType, BPlayerID playerID, int data, int data2);

      void                    clearHoverLight(void);
      void                    updateHoverLight(bool enabled);
      
      long                    getGroupID( long controlType, BInputEventDetail* pDetail );
      void                    selectGroup( long controlType, BInputEventDetail* pDetail );
      void                    gotoGroup( long controlType, BInputEventDetail* pDetail );
      void                    assignGroup( long controlType, BInputEventDetail* pDetail );

      void                    groupAdd();
      void                    groupNext();
      void                    groupPrev();

      void                    autoExitSubMode();
      void                    resetDoubleClick(long controlType);

      void                    playCreateGroupSound(long groupID);
      void                    playSelectGroupSound(long groupID);

      BOOL                    readPrivilege(XPRIVILEGE_TYPE priv) const;
      void                    readPrivileges();

      void                    initializeAchievements();

      void                    initializeHintEngine();

      bool                    canSelectUnit(BEntityID unitID, bool ignoreCurrentSelection) const;

      int                     getHoverObjectPriority(BEntityID hoverObject, int hoverType) const;
      void                    resetStickyReticle();

      void                    handleEndGame();
      void                    resign();

      bool                    isScrollingMode(int mode) const;
      void                    doScrollDelay();

      static float            scrollSpeedAdjByteToFloat( uint8 i );
      static uint8            scrollSpeedAdjFloatToByte( float f );

      void                    resetUserOptionsToDefault();

      void                    tieToCameraRep();
      void                    saveLastCameraLoc();
      void                    calcCameraDefaultPitch();

      bool                    canTarget(const BObject* pObject, const BRenderViewParams& view) const;
      void                    calcTargetSelectPos();
      void                    updateTargetSelect();

      bool                    resolvePositionConstraints2D(BVector lastCameraPos, BVector lastHoverPointPos, BVector curCameraPos, BVector curHoverPointPos, BVector &newCameraPos, BVector &newHoverPointPos, bool &unresolved);
      bool                    resolvePositionTeleportConstraints2D(BVector lastCameraPos, BVector lastHoverPointPos, BVector curCameraPos, BVector curHoverPointPos, BVector &newCameraPos, BVector &newHoverPointPos, bool &unresolved);
      bool                    constrainMovementToBoundaryLines2D(BVector lastPosition, BVector curPosition, BVector &newPosition, bool bExtrapolate, BCameraBoundaryLine::BCameraBoundaryType type, bool bAllowEvenCrossing);
      bool                    intersectSegmentToBoundaryLines2D(BVector point0, BVector point1, BCameraBoundaryLine::BCameraBoundaryType type, long ignorLineIndex, long ignoreLineSegmentIndex, BVector &intersectionPoint, long &intersectionLineIndex, long &intersectionLineSegmentIndex);
      bool                    intersectSegmentToBoundaryLinesUnevenOnly2D(BVector point0, BVector point1, BCameraBoundaryLine::BCameraBoundaryType type, BVector &intersectionPoint, long &intersectionLineIndex, long &intersectionLineSegmentIndex);
      void                    computeValidOffsetPositionFromLine(BVector point0, BVector intersectionPoint, long intersectedLine, long intersectedSegment, BCameraBoundaryLine::BCameraBoundaryType type, BVector &offsetPoint);


      //BProtoAction*           canPlayAbilitySound(long abilityID, const BSquad* pSquad);
      bool                    canPlayAbilitySound(long abilityID, const BSquad* pSquad, BProtoAction** returnAction = NULL);

      void                    cancelPower();

      BMinigame                  mMinigame;
      float                      mMinigameTimeFactor;
      int                        mCameraEffectTransitionOutIndex;
      BCameraEffectData          mModeCameraEffect;
      bool                       mClearRestoreCamValuesOnEffectCompletion;

      // General
      BOOL                       mSystemUIShowing;
      // Privileges
      BOOL                       mPrivMultiplayer;
      BOOL                       mPrivCommunications;
      BOOL                       mPrivCommunicationsFriends;
      BOOL                       mPrivProfileViewing;
      BOOL                       mPrivProfileViewingFriends;
      BOOL                       mPrivUserContent;
      BOOL                       mPrivUserContentFriends;
      BOOL                       mPrivPurchaseContent;
      BOOL                       mPrivPresence;
      BOOL                       mPrivPresenceFriends;
      // More General      
      long                       mPort;
      long                       mPlayerID;
      long                       mCoopPlayerID;
      BTeamID                    mTeamID;
      long                       mUserMode;
      int                        mLastUserMode;
      long                       mSubMode;
      BSelectionManager*         mSelectionManager;
      BPlayerState               mPlayerState;
      XUSER_SIGNIN_STATE         mSignInState;
      XUID                       mXuid;
      XCONTENTDEVICEID           mDeviceID;
      BSimString                 mName;
      //BUserAchievementList*      mpAchievementList; //moved to userprofile
      BUserProfile*              mpProfile;

      // Hover
      BVector                    mHoverPoint;
      BEntityID                  mHoverObject;
      long                       mHoverType;
      uint                       mHoverTypeAttackRating;
      long                       mHoverResource;
      long                       mHoverHitZoneIndex;
      int                        mHoverDecal;
      int                        mPowerDecal;

      uint                       mLastSelectionChangeCounter;

      // Sticky reticle
      BVector                    mStickyHoverCameraPos;
      BEntityID                  mStickyHoverObject;
      BVector                    mStickyHoverPoint;
      long                       mStickyHoverType;
      uint                       mStickyHoverTypeAttackRating;
      long                       mStickyHoverResource;
      long                       mStickyHoverHitZoneIndex;
      BVector                    mStickyHoverObjectPos;           // Position of the unit we are hovering on
      float                      mStickyReticleJumpDist;
//      float                      mStickyReticleSensitivity;

      // Follow
      BEntityID                  mFollowEntity;

      // Placement suggestion
      BVector                    mPlacementSuggestion;
      BEntityID                  mPlacementSocketID;

      // Camera
      BCamera*                   mpCamera;
      float                      mCameraDefaultPitch;
      float                      mCameraDefaultYaw;
      float                      mCameraDefaultZoom;
      float                      mCameraZoomMin;
      float                      mCameraZoomMax;
      float                      mCameraPitchMin;
      float                      mCameraPitchMax;
      float                      mCameraPitch;
      float                      mCameraYaw;
      float                      mCameraZoom;
      float                      mCameraFOV;
      float                      mCameraAdjustZoom;
      float                      mCameraAdjustYaw;
      float                      mCameraAdjustFOV;
      float                      mCameraAutoZoomOutTime;
      float                      mCameraAutoZoomInTime;
      float                      mCameraHoverPointOffsetHeight;    // Height offset between the camera height field and the hoverpoint
      BVector                    mCameraHoverPoint;
      BVector                    mLastCameraLoc;
      BVector                    mLastCameraHoverPoint;

      BDynamicSimArray<BCameraBoundaryLine>  mCameraBoundaryLines;
      
      // Alternate Free Camera 
      // rg [1/1/07] - The "free" camera is used by artists. It is not intended to ever be used during gameplay.
      BCamera*                   mpFreeCamera;
      BVec3                      mFreeCameraMovement;
      BVec3                      mFreeCameraRotation;
      bool                       mFreeCameraLModifier : 1;
      bool                       mFreeCameraRModifier : 1;
      bool                       mFreeCameraReset : 1;
      BVec3                      mFreeCameraCurLoc;
      BVec3                      mFreeCameraCurRot;

      // UI Context
      BUIContext*                mpUIContext;

      // Reticle
      float                      mReticleOffset;

      // Scrolling
      int64                      mScrollTime;
      float                      mScrollX;
      float                      mScrollY;
      float                      mScrollRate;
      float                      mScrollPercent;
      float                      mScrollXDelay;
      float                      mScrollYDelay;
      float                      mScrollDelayTime;
      BVector                    mScrollDirection;
      enum { cMaxScrollSpeedSamples=30 };
      float                      mScrollSpeedSampleDistances[cMaxScrollSpeedSamples];
      float                      mScrollSpeedSampleElapsedTimes[cMaxScrollSpeedSamples];
      uint                       mScrollSpeedSampleIndex;
      uint                       mScrollSpeedSampleCount;
      float                      mScrollSpeedAverage;
      BEntityID                  mScrollStartHoverObject;
      BVector                    mScrollStartPos;
      float                      mScrollAngle;
      float                      mScrollTotalTime;
      float                      mScrollTotalDist;
      float                      mScrollPrevTotalTime;
      float                      mScrollPrevTotalDist;
      BVector                    mScrollPrevDirection;

      float                      mLookAtPosStaleness;

      // Help UI
      BUIHelp                    mHelpUI;

      BFreeList<BEntityIDFloatPair, 4, true> mCommandedSquads;

      // Selected unit icons
      BUIElement                 mSelectedUnitIcons[cMaxSelectedUnitIcons];
      BUIElement                 mSelectedUnitText[cMaxSelectedUnitIcons];
      long                       mSelectedUnitDisplayCount;

      // Circle select mode.
      enum { cNumCircleSelectPoints=48 }; //32
      float                      mCircleSelectSize;
      float                      mCircleSelectRate;
      float                      mCircleSelectBaseSize;
      float                      mCircleSelectBaseMaxSize;
      float                      mCircleSelectMaxSize;
      BVector                    mCircleSelectPoints[cNumCircleSelectPoints];
      BVector                    mCircleSelectLastPoint;
      BEntityID                  mCircleSelectLastSelected;
      int                        mCircleSelectDecal; 
      float                      mCircleSelectOpacityFadeRate;
      float                      mCircleSelectOpacity;



      // Groups
      BEntityIDArray             mGroups[cMaxGroups];
      int                        mGroupIndex;

      // Controller Key Timers
      BControllerKeyTimes        mControllerKeyTimes[cNumDC];

      // Menu modes
      BUICircleMenu                 mCircleMenu;      
      BDynamicSimArray<BManagedTextureHandle>  mMenuTextures;
      
      bool                       mUILocationCheckObstruction:1;
      bool                       mUILocationLOSCenterOnly:1;
      bool                       mUILocationCheckMoving:1;
      bool                       mUILocationPlacementSuggestion:1;
      long                       mUILocationLOSType;
      long                       mUILocationPlacementRuleIndex;
      float                      mUIPowerRadius;
      int                        mUIProtoPowerID;
      BTriggerScriptID           mTriggerScriptID;
      BTriggerVarID              mTriggerVarID;
      BTriggerScriptID           mUILockOwner;
      BBitVector8                mInputUIModeFlags;
      BPlayerIDArray             mInputUIModePlayerFilter;
      BTeamIDArray               mInputUIModeTeamFilter;
      BObjectTypeIDArray         mInputUIModeObjectTypeFilter;
      BProtoObjectIDArray        mInputUIModeProtoObjectFilter;
      BProtoSquadIDArray         mInputUIModeProtoSquadFilter;
      BRelationTypeArray         mInputUIModeRelationFilter;
      BEntityFilterSet*          mInputUIModeEntityFilterSet;
      BEntityIDArray             mInputUIModeSquadList;
      BSmallDynamicSimArray<BUserUIButtonRequest> mInputUIButtonRequests;
      float                      mUIModeRestoreCameraZoomMin;
      float                      mUIModeRestoreCameraZoomMax;
      float                      mUIModeRestoreCameraZoom;
      float                      mUIModeRestoreCameraPitchMin;
      float                      mUIModeRestoreCameraPitchMax;
      float                      mUIModeRestoreCameraPitch;
      float                      mUIModeRestoreCameraYaw;

      // Building and unit commands
      BEntityID                  mCommandObject;
      BPlayerID                  mCommandObjectPlayerID;
      long                       mBuildProtoID;
      long                       mBuildProtoSquadID;
      float                      mBuildBaseYaw;
      bool                       mBuildUseSquadObstruction;

      // Ability.
      long                       mAbilityID;

      // Hover visual
      BVisual*                   mpHoverVisual; 

      // Local select
      float                      mLocalSelectTime;

      float                      mFlareTime;
      BVector                    mFlarePos;

      // Target selection
      BEntityID                  mTargetSelectObject;
      BEntityID                  mTargetSelectSquad;
      BEntityID                  mTargetSelectDoppleForObject;
      BVector                    mTargetSelectPos;

      // Crowd select and goto
      long                       mCrowdIndex;
      BVector                    mCrowdPos;

      // Goto item (base, node, hero, scout)
      BEntityID                  mGotoItemID;
      BVector                    mGotoPosition;
      float                      mGotoItemTime;
      int                        mGotoType;

      // Interp
      BVector                    mInterpHoverPoint;
      float                      mInterpZoom;
      float                      mInterpPitch;
      float                      mInterpYaw;
      float                      mInterpTimeLeft;

      // Goto selected
      long                       mGotoSelectedIndex;

      //Double Click Squads (used for attack move for now).
      BEntityIDArray             mDoubleClickSquads;      

      // Game state messages
      BSmallDynamicSimArray<BUserGameStateMessage>  mGameStateMessageList;
      float                         mGameStateMessageTimer;
      int                           mGameStateMessageType;
      BUString                      mGameStateMessageText;

      // Support power selection
      uint                       mSupportPowerIndex;
      int                        mSupportPowerIconLocation;

      // Power related stuff.
      uint mNextPowerUserRefCount; // The next power user ID (just for uniqueness / safety) - might not be necessary... (revisit this.)
      BPowerUser* mpPowerUser;     // Pointer to the power user we use for ASYNC controlling of power stuff.

      // Dev only - Unit pick, selection, and obstruction modification
      bool                       mPickChangeXPlus;
      bool                       mPickChangeXMinus;
      bool                       mPickChangeYPlus;
      bool                       mPickChangeYMinus;

      bool                       mSelectionChangeXPlus;
      bool                       mSelectionChangeXMinus;
      bool                       mSelectionChangeZPlus;
      bool                       mSelectionChangeZMinus;

      bool                       mObstructionChangeXPlus;
      bool                       mObstructionChangeXMinus;
      bool                       mObstructionChangeYPlus;
      bool                       mObstructionChangeYMinus;
      bool                       mObstructionChangeZPlus;
      bool                       mObstructionChangeZMinus;
  
      uint                       mShowEntityIDType;

      // Hover light - temporary test until Billy implements light effects.
      BLocalLightHandle          mHoverLightHandle;
      bool                       mHoverLightEnabled;

      // Dev only - Show range rings for selected unit weapons / LOS
      bool                       mWeaponRangeDisplayEnabled;

      static BUserMessages       mUserMessages[USER_MESSAGES_MAX];

      BDynamicSimArray<BFlashUserMessage*>   mFlashUserMessages;

      // Objective Arrow
      BDynamicSimArray<BObjectiveArrow*> mObjectiveArrowList;

//      long                       mConfigIndex;

      BEntityID                  mRumbleHoveringObject;
      int                        mRumbleHoveringID;

      int64                      mTimerFrequency;
      double                     mTimerFrequencyFloat;

      BVolumeCuller              mSceneVolumeCuller;


      bool                       mHUDItemEnabled[cNumberHUDItems];

      float                      mSkipNextSelectTime;
      DWORD                      mFlareTimer;

      //display the number of the base when the user jumps to it
      float                      mBaseNumberDisplayTime;
      int                        mBaseNumberToDisplay;

      BExitMethod                mRequestedExitMethod;

      BSimString                 mSaveGameFile;

      #if defined( _VINCE_ )
         DWORD mVinceTimeZoom;
         DWORD mVinceTimeTilt;
      #endif

      BDynamicArray<XONLINE_FRIEND> mFriends;

      bool                       mLiveEnabled:1;
      bool                       mLiveStateChanged:1;
      bool                       mDeviceRemoved:1;

      // Flags
      bool                       mFlagUserActive:1;
      bool                       mFlagUpdateHoverPoint:1;
      bool                       mFlagHaveHoverPoint:1;
      bool                       mFlagHaveLastHoverPoint:1;
      bool                       mFlagHoverPointOverTerrain:1;
      bool                       mFlagCircleSelectPoints:1;
      bool                       mFlagModifierSpeed:1;
      bool                       mFlagModifierAction:1;
      bool                       mFlagCircleSelecting:1;
      bool                       mFlagCircleSelectReset:1;
      bool                       mFlagCircleSelectSelected:1;
      bool                       mFlagCircleSelectGrow:1;
      bool                       mFlagCircleSelectFullyGrown:1;
      bool                       mFlagCircleSelectShrink:1;
      bool                       mFlagCircleSelectFullyShrunk:1;
      bool                       mFlagCircleSelectDoNotification:1;
      bool                       mFlagCircleSelectVinceEvent:1;
      bool                       mFlagDelayScrolling:1;
      bool                       mFlagDelayScrollingStart:1;
      bool                       mFlagGameActive:1;
      bool                       mFlagCommandMenuRefresh:1;
      bool                       mFlagPowerMenuRefresh:1;
      bool                       mFlagExitModeOnModifierRelease:1;
      bool                       mFlagNoCameraLimits:1;
      bool                       mFlagFreeCamera:1;
      bool                       mFlagUILocationValid:1;
      bool                       mFlagGameStateMessage:1;
      bool                       mFlagHoverLight:1;
      bool                       mFlagUIUnitValid:1;
      bool                       mFlagUISquadValid:1;
//      bool                       mFlagInvertTilt:1;
//      bool                       mFlagInvertPan:1;
      bool                       mFlagUserLocked:1;
      bool                       mFlagDeselectOnMenuClose:1;
      bool                       mFlagOverrideUnitPanelDisplay:1;
      bool                       mFlagRestoreUnitPanelOnModeChange:1;
      bool                       mFlagRepairUnavail:1;
      bool                       mFlagScrollStartSaved:1;
      bool                       mFlagPowerMenuOverride:1;
      bool                       mFlagPowerMenuEnable:1;
//      bool                       mFlagStickyReticle:1;
      bool                       mFlagStickyReticleJumpCamera:1;
//      bool                       mFlagStickyReticleFollow:1;
      bool                       mFlagStickyReticleDoJump:1;
      bool                       mFlagStickyReticleDoFollow:1;
      bool                       mFlagTargetSelecting:1;
      bool                       mFlagCommandMenuRefreshTrainProgressOnly:1;
      bool                       mFlagPowerMenuRefreshTrainProgressOnly:1;
      bool                       mFlagCameraScrollEnabled:1;
      bool                       mFlagCameraYawEnabled:1;
      bool                       mFlagCameraZoomEnabled:1;
      bool                       mFlagCameraAutoZoomInstantEnabled:1;
      bool                       mFlagCameraAutoZoomEnabled:1;
//      bool                       mFlagUserCameraRotation:1;
      bool                       mFlagDelayLocalMilitarySound:1;
      bool                       mFlagDelayFlare:1;
      bool                       mFlagUIPowerMinigame:1;
      bool                       mFlagUIRestartMinigame:1;
      bool                       mFlagUIModeRestoreCameraZoomMin:1;
      bool                       mFlagUIModeRestoreCameraZoomMax:1;
      bool                       mFlagUIModeRestoreCameraZoom:1;
      bool                       mFlagUIModeRestoreCameraPitchMin:1;
      bool                       mFlagUIModeRestoreCameraPitchMax:1;
      bool                       mFlagUIModeRestoreCameraPitch:1;
      bool                       mFlagUIModeRestoreCameraYaw:1;
      bool                       mFlagSkipNextSelect:1;
//      bool                       mFlagContextHelp:1;
//      bool                       mFlagSubtitles:1;
//      bool                       mFlagFriendOrFoe:1;
      bool                       mFlagRestoreCameraEnableUserScroll:1;
      bool                       mFlagRestoreCameraEnableUserYaw:1;
      bool                       mFlagRestoreCameraEnableUserZoom:1;      
      bool                       mFlagRestoreCameraEnableAutoZoomInstant:1;
      bool                       mFlagRestoreCameraEnableAutoZoom:1;
      bool                       mFlagCameraTeleport:1;
      bool                       mFlagIgnoreDpad:1;
      bool                       mFlagGameDoExit:1;
      bool                       mFlagDefaultCamera:1;
      bool                       mFlagClearCircleMenuDisplay:1;
};
