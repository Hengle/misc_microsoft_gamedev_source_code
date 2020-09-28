//==============================================================================
// user.cpp
//
// Copyright (c) Ensemble Studios, 2005-2007
//==============================================================================

// Includes
#include "common.h"
#include "XeCR.h"
#include "user.h"
#include "math\generalMatrix.h"

// terrain
#include "TerrainSimRep.h"

// xgame
#include "ai.h"
#include "ability.h"
#include "civ.h"
#include "commandmanager.h"
#include "commands.h"
#include "commandtypes.h"
#include "configsgame.h"
#include "database.h"
#include "Formation2.h"
#include "game.h"
#include "generaleventmanager.h"
#include "modemanager.h"
#include "modegame.h"
#include "modemenu.h"
#include "object.h"
#include "objectmanager.h"
#include "protosquad.h"
#include "protoobject.h"
#include "protopower.h"
#include "prototech.h"
#include "recordgame.h"
#include "scenario.h"
#include "selectionmanager.h"
#include "tactic.h"
#include "techtree.h"
#include "triggermanager.h"
#include "triggervar.h"
#include "ui.h"
#include "uigame.h"
#include "unitactionbuilding.h"
#include "unitactionmoveair.h"
#include "squadactioncarpetbomb.h"
#include "unitactionrevive.h"
#include "SimOrderManager.h"
#include "world.h"
#include "minimap.h"
#include "alert.h"
#include "squadai.h"
#include "userachievements.h"
#include "achievementmanager.h"
#include "team.h"
#include "userprofilemanager.h"
#include "humanPlayerAITrackingData.h"
#include "campaignmanager.h"
#include "gamesettings.h"
#include "unitquery.h"
#include "UnitActionCollisionAttack.h"
#include "ChatManager.h"
//#include "uipostgame.h"
//#include "uicampaignpostgame.h"
#include "squadactiontransport.h"
#include "squadplotter.h"
#include "Formation2.h"
#include "usermanager.h"
#include "HPBar.h"
#include "GamerPicManager.h"
#include "cameraEffectManager.h"
#include "UIGlobals.h"
#include "UIButtonBar2.h"
#include "econfigenum.h"
#include "uiwatermark.h"
#include "power.h"
#include "powercleansing.h"
#include "powerorbital.h"
#include "powercarpetbombing.h"
#include "powercryo.h"
#include "powerwave.h"
#include "powerrage.h"
#include "powerdisruption.h"
#include "powerrepair.h"
#include "powerodst.h"
#include "powertransport.h"
#include "scoremanager.h"
#include "UIGameMenu.h"
#include "simhelper.h"

// xgamerender
#include "camera.h"
#include "debugprimitives.h"
#include "FontSystem2.h"
#include "render.h"
#include "dirShadowManager.h"
#include "sceneLightManager.h"
#include "configsgamerender.h"
#include "primDraw2D.h"

// xparticles
#include "particlegateway.h"

// xinputsystem
#include "configsinput.h"
#include "keyboard.h"

// xsystem
#include "config.h"
#include "mathutil.h"
#include "utilities.h"
#include "notification.h"
#include "image.h"

//xLive - so that it can query if a mp game is active
#include "liveSystem.h"

// xvisual
#include "visual.h"
#include "visualmanager.h"

// xcore
#include "containers\dynamicArray.h"

// xvince
#include "vincehelper.h"

#include "uimanager.h"
#include "decalManager.h"

// Hit Zones
#include "hitzone.h"
#include "math\collision.h"
#include "Quaternion.h"

#include "renderControl.h"
#include "uiticker.h"

#include "gammaramp.h"
#include "archiveManager.h"

#include "transitionManager.h"

#include "ModeCampaign2.h"

// Statics
BUser::BoolOptionMap BUser::mBoolOptionMap;
BUser::Uint8OptionMap BUser::mUint8OptionMap;

GFIMPLEMENTVERSION(BUser, 5);

// Defines
//#define DEBUG_SIMREP_RAYCAST
//#define ENABLE_CAMERA_OUT_OF_BOUNDS_POTENTIAL_FIX

#if defined( _VINCE_ )
   #define VINCE_TIME_DELAY 5000 // 5 seconds
#endif

// Constants
static const DWORD cColorSelectionCircle  = cDWORDWhite;
static const DWORD cColorSelected         = cDWORDWhite;
static const DWORD cColorReticleNone      = cDWORDWhite;
static const DWORD cColorReticleSelect    = cDWORDYellow;
static const DWORD cColorReticleWork      = cDWORDGreen;
static const DWORD cColorReticleEnemy     = cDWORDRed;
static const DWORD cColorHoverNone        = cDWORDLightGrey;
static const DWORD cColorHoverSelect      = cDWORDWhite;
static const DWORD cColorHoverWork        = cDWORDWhite;
static const DWORD cColorHoverEnemy       = cDWORDWhite;

static const float cGameStateMessageTimeout = 3.0f;

static const uint cDefaultSelectionSpeed = 1;
static const uint cDefaultMusicVolume = 1;
static const uint cDefaultSoundFXVolume= 1;
static const uint cDefaultVoiceVolume = 1;
static const uint cDefaultUserGamma = 100;

static const float cConvertScrollAdjMinFloat = 0.5f;
static const float cConvertScrollAdjMaxFloat = 3.0f;
static const float cConvertScrollAdjNegStepFloat = 0.1f;
static const float cConvertScrollAdjPosStepFloat = 0.2f;
static const float cDefaultScrollSpeedAdjFloat = 1.0f;

static const uint8 cSliderZoomStep = 5;

enum 
{
   cUserYornBoxResignConfirmation = 0,  
   cUserYornBoxRestartConfirmation,  
   cUserYornBoxSelfDestructBuilding,
   cUserYornBoxOOSNotification,
   cUserYornBoxDisconnectNotification,
   cUserYornBoxResetOptionsNotification,
   cUserYornBoxSaveError,
};

BUserMessages BUser::mUserMessages[USER_MESSAGES_MAX];


const float cCameraLineLimitEpsilon = 0.025f;

bool xzSegmentToSegmentIntersect(BVector lnA0, BVector lnA1, BVector lnB0, BVector lnB1, BVector &intersectionPoint);
void xzClosestsPointInLine(BVector point, BVector lnB0, BVector lnB1, BVector &closestPoint);



//==============================================================================
//==============================================================================
BFlashUserMessage::BFlashUserMessage(long stringID, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame) : 
   mbQueueSound(bQueueSound),
   mTimeToDisplay(duration),
   mbIsNew(true),
   mCueIndex(cInvalidCueIndex),
   mbNeverExpire(false),
   mbExistPastEndGame(existPastEndGame)
{
   mSound = sound;

   if (sound.length() > 0)
      mCueIndex = gSoundManager.getCueIndex(sound.getPtr());

   // look up the string index
   mDisplayMessage.set(gDatabase.getLocStringFromID(stringID).getPtr());

   if(duration == 0)
   {
      mbNeverExpire = true;
   }
}

//==============================================================================
//==============================================================================
BFlashUserMessage::BFlashUserMessage(const BUString& message, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame) : 
   mbQueueSound(bQueueSound),
   mTimeToDisplay(duration),
   mbIsNew(true),
   mCueIndex(cInvalidCueIndex),
   mbNeverExpire(false),
   mbExistPastEndGame(existPastEndGame)
{
   mSound = sound;

   if (sound.length() > 0)
      mCueIndex = gSoundManager.getCueIndex(sound.getPtr());

   // look up the string index
   mDisplayMessage.set(message.getPtr());

   if(duration == 0)
   {
      mbNeverExpire = true;
   }
}

//==============================================================================
// BFlashUserMessage::updateTime
//==============================================================================
void BFlashUserMessage::updateTime(float elapsedTime)
{
   mTimeToDisplay-=elapsedTime;
}

//==============================================================================
// BFlashUserMessage::getMessage
//==============================================================================
/*
const BUString& BFlashUserMessage::getMessage() const
{
   return gDatabase.getLocStringFromIndex(mStringIDIndex);
}
*/


#ifdef DEBUG_SIMREP_RAYCAST
   static bool debugState=true;
   static bool debugDrawn=false;
   static BVector debugLoc(6.27100611, 20.8357029, 107.061211);
   static BVector debugDir(-0.0518120490, -0.601832926, 0.796939492);
#endif

   
//==============================================================================
// BUser::BUser
//==============================================================================
BUser::BUser() :
   mShowButtonHelp(true), // Remove after E3
   // General
   mSignInState(eXUserSigninState_NotSignedIn),
   mSystemUIShowing(FALSE),
   mPort(-1),
   mPlayerID(-1),
   mCoopPlayerID(-1),
   mTeamID(-1),
   mUserMode(cUserModeNormal),
   mLastUserMode(cUserModeNormal),
   mSubMode(cSubModeSelectPower),
   mSelectionManager(NULL),
   mPlayerState(BPlayer::cPlayerStatePlaying),
   mXuid(0),
   mDeviceID(XCONTENTDEVICE_ANY),
   mDeviceRemoved(false),
   //mpAchievementList(NULL),
   mpProfile(NULL),

   // Hover
   mHoverPoint(cOriginVector),
   mHoverObject(cInvalidObjectID),
   mHoverType(cHoverTypeNone),
   mHoverTypeAttackRating(cReticleAttackNoEffectAgainst),
   mHoverResource(cHoverResourceNone),
   mHoverHitZoneIndex( -1 ),

   mLastSelectionChangeCounter(0),

   // Sticky reticle hover
   mStickyHoverCameraPos(cOriginVector),
   mStickyHoverObject(cInvalidObjectID),
   mStickyHoverPoint(cOriginVector),
   mStickyHoverType(cHoverTypeNone),
   mStickyHoverTypeAttackRating(cReticleAttackNoEffectAgainst),
   mStickyHoverResource(cHoverResourceNone),
   mStickyHoverHitZoneIndex( -1 ),
   mStickyHoverObjectPos(cOriginVector),
   mStickyReticleJumpDist(0.0f),
//   mStickyReticleSensitivity(0.0f),

   // Follow
   mFollowEntity(cInvalidObjectID),

   // Camera settings
   mpCamera(NULL),
   mCameraPitch(0.0f),
   mCameraYaw(0.0f),
   mCameraZoom(0.0f),
   mCameraFOV(0.0f),
   mCameraAdjustZoom(0.0f),
   mCameraAdjustYaw(0.0f),
   mCameraAdjustFOV(0.0f),
   mCameraAutoZoomOutTime(0.0f),
   mCameraAutoZoomInTime(0.0f),
   mCameraHoverPointOffsetHeight(0.0f),
   mCameraHoverPoint(cOriginVector),
   mLastCameraLoc(cOriginVector),
   mLastCameraHoverPoint(cOriginVector),
   
   mpFreeCamera(NULL),
   mFreeCameraMovement(0.0f),
   mFreeCameraRotation(0.0f),
   mFreeCameraLModifier(false),
   mFreeCameraRModifier(false),
   mFreeCameraReset(false),
   mFreeCameraCurLoc(0.0f),
   mFreeCameraCurRot(0.0f),

   // Reticle
   mReticleOffset(0.0f),

   // Scrolling
   mScrollTime(0),
   mScrollX(0.0f),
   mScrollY(0.0f),
   mScrollRate(0.0f),
   mScrollPercent(0.0f),
   mScrollXDelay(0.0f),
   mScrollYDelay(0.0f),
   mScrollDelayTime(0.0f),
   mScrollDirection(cOriginVector),
   mScrollSpeedSampleIndex(0),
   mScrollSpeedSampleCount(0),
   mScrollSpeedAverage(0.0f),
   mScrollStartHoverObject(cInvalidObjectID),
   mScrollStartPos(cOriginVector),
   mScrollAngle(-1.0f),
   mScrollTotalTime(0.0f),
   mScrollTotalDist(0.0f),
   mScrollPrevTotalTime(0.0f),
   mScrollPrevTotalDist(0.0f),
   mScrollPrevDirection(cOriginVector),

   mLookAtPosStaleness(0.0f),

   // Help UI
   mHelpUI(),

   // Selected unit icons
   mSelectedUnitDisplayCount(0),

   // Circle select mode.
   mCircleSelectSize(0.0f),
   mCircleSelectRate(0.0f),
   mCircleSelectBaseSize(0.0f),
   mCircleSelectBaseMaxSize(0.0f),
   mCircleSelectMaxSize(0.0f),
   mCircleSelectOpacityFadeRate(0.0f),
   mCircleSelectOpacity(0.0f),
   //mCircleSelectPoints[] initialized below.
   mCircleSelectLastPoint(cInvalidVector),
   mCircleSelectLastSelected(cInvalidObjectID),
   mCircleSelectDecal(-1),
   mHoverDecal(-1),
   mPowerDecal(-1),

   // Groups
   //mGroups[] initialized below.
   mGroupIndex(-1),

   // Menu modes
   mCircleMenu(),
   mMenuTextures() ,
   // Input UI trigger stuff
   mUILocationCheckObstruction(false),
   mUILocationLOSCenterOnly(false),
   mUILocationCheckMoving(false),
   mUILocationPlacementSuggestion(false),
   mUILocationLOSType(-1),
   mUILocationPlacementRuleIndex(-1),
   mUIPowerRadius(0.0f),
   mUIProtoPowerID(-1),
   mTriggerScriptID(cInvalidTriggerScriptID),
   mTriggerVarID(cInvalidTriggerVarID),
   mUILockOwner(cInvalidTriggerScriptID),
   mInputUIModeEntityFilterSet(NULL),
   mMinigameTimeFactor(1.0f),
   mCameraEffectTransitionOutIndex(-1),
   mClearRestoreCamValuesOnEffectCompletion(false),
   mUIModeRestoreCameraZoomMin(0.0f),
   mUIModeRestoreCameraZoomMax(0.0f),
   mUIModeRestoreCameraZoom(0.0f),
   mUIModeRestoreCameraPitchMin(0.0f),
   mUIModeRestoreCameraPitchMax(0.0f),
   mUIModeRestoreCameraPitch(0.0f),
   mUIModeRestoreCameraYaw(0.0f),

   // Building and unit commands
   mCommandObject(cInvalidObjectID),
   mCommandObjectPlayerID(cInvalidPlayerID),
   mBuildProtoID(-1),
   mBuildProtoSquadID(-1),
   mBuildBaseYaw(0.0f),
   mBuildUseSquadObstruction(false),
   mpHoverVisual(NULL),

   // Ability
   mAbilityID(-1),

   // Local select
   mLocalSelectTime(0.0f),

   mFlareTime(0.0f),
   mFlarePos(cOriginVector),

   // Target selection
   mTargetSelectObject(cInvalidObjectID),
   mTargetSelectSquad(cInvalidObjectID),
   mTargetSelectDoppleForObject(cInvalidObjectID),
   mTargetSelectPos(cOriginVector),

   mCrowdIndex(-1),
   mCrowdPos(cOriginVector),

   // Goto base
   mGotoItemID(cInvalidObjectID),
   mGotoItemTime(0.0f),
   mGotoType(-1),
   mGotoPosition(cOriginVector),

   // interp
   mInterpHoverPoint(cOriginVector),
   mInterpZoom(0.0f),
   mInterpPitch(0.0f),
   mInterpYaw(0.0f),
   mInterpTimeLeft(0.0f),

   // Goto selected
   mGotoSelectedIndex(-1),

   mBaseNumberDisplayTime(0.0f),
   mBaseNumberToDisplay(0),

   mSaveGameFile(),

   // Game state messages
   mGameStateMessageList(),
   mGameStateMessageTimer(0.0f),
   mGameStateMessageType(-1),
   mGameStateMessageText(),

   // Support power selection
   mSupportPowerIndex(0),
   mSupportPowerIconLocation(-1),

   // Power related stuff.
   mNextPowerUserRefCount(0),
   mpPowerUser(NULL),

   // Dev only - Unit pick, selection, and obstruction modification
   mPickChangeXPlus(false),
   mPickChangeXMinus(false),
   mPickChangeYPlus(false),
   mPickChangeYMinus(false),

   mSelectionChangeXPlus(false),
   mSelectionChangeXMinus(false),
   mSelectionChangeZPlus(false),
   mSelectionChangeZMinus(false),

   mObstructionChangeXPlus(false),
   mObstructionChangeXMinus(false),
   mObstructionChangeYPlus(false),
   mObstructionChangeYMinus(false),
   mObstructionChangeZPlus(false),
   mObstructionChangeZMinus(false),

   mWeaponRangeDisplayEnabled(false),

   mShowEntityIDType(0),

   // Hover light
   mHoverLightHandle(cInvalidLocalLightHandle),
   mHoverLightEnabled(false),

//   mConfigIndex( 0 ),   
   mFlagRepairUnavail(false),

   mRumbleHoveringObject(cInvalidObjectID),
   mRumbleHoveringID(-1),

   mTimerFrequency(0),
   mTimerFrequencyFloat(0.0),

   mpUIContext(NULL),

   mSceneVolumeCuller(),

   #if defined( _VINCE_ )
      mVinceTimeZoom( 0 ),
      mVinceTimeTilt( 0 ),
   #endif
   mFriends(),
   mLiveEnabled(false),
   mLiveStateChanged(false)
{
   mDoubleClickSquads.clear();

   // Flags
   mFlagUserActive=false;
   mFlagUpdateHoverPoint=false;
   mFlagHaveHoverPoint=false;
   mFlagHaveLastHoverPoint=false;
   mFlagHoverPointOverTerrain=false;
   mFlagCircleSelectPoints=false;
   mFlagModifierSpeed=false;
   mFlagModifierAction=false;
   mFlagCircleSelecting=false;
   mFlagCircleSelectReset=false;
   mFlagCircleSelectSelected=false;
   mFlagCircleSelectGrow=false;
   mFlagCircleSelectFullyGrown=false;
   mFlagCircleSelectShrink=false;
   mFlagCircleSelectFullyShrunk=false;
   mFlagCircleSelectDoNotification=false;
   mFlagCircleSelectVinceEvent=false;
   mFlagDelayScrolling=false;
   mFlagDelayScrollingStart=false;
   mFlagGameActive=false;
   mFlagCommandMenuRefresh=false;
   mFlagPowerMenuRefresh=false;
   mFlagExitModeOnModifierRelease=false;
   mFlagNoCameraLimits=false;
   mFlagFreeCamera=false;
   mFlagUILocationValid=false;
   mFlagGameStateMessage=false;
   mFlagHoverLight=false;
   mFlagUIUnitValid=false;
   mFlagUISquadValid=false;
//    mFlagInvertTilt=false;
//    mFlagInvertPan=false;
   mFlagUserLocked=false;   
   mFlagDeselectOnMenuClose=false;
   mFlagOverrideUnitPanelDisplay=false;
   mFlagRestoreUnitPanelOnModeChange=false;
   mFlagRepairUnavail=false;
   mFlagScrollStartSaved=false;
   mFlagPowerMenuOverride = false;
   mFlagPowerMenuEnable = true;
//   mFlagStickyReticle = false;
   mFlagStickyReticleJumpCamera = false;
//   mFlagStickyReticleFollow = false;
   mFlagStickyReticleDoJump=false;
   mFlagStickyReticleDoFollow=false;
   mFlagTargetSelecting=false;
   mFlagCommandMenuRefreshTrainProgressOnly=false;
   mFlagPowerMenuRefreshTrainProgressOnly=false;
   mFlagCameraScrollEnabled = true;
   mFlagCameraYawEnabled = true;
   mFlagCameraZoomEnabled = true;
   mFlagCameraAutoZoomInstantEnabled = gConfig.isDefined(cConfigCameraAutoZoomInstant);
   mFlagCameraAutoZoomEnabled = true;
//   mFlagUserCameraRotation = true;
   mFlagDelayLocalMilitarySound = false;
   mFlagDelayFlare = false;
   mFlagUIPowerMinigame = false;
   mFlagUIRestartMinigame = true;
   mFlagUIModeRestoreCameraZoomMin = false;
   mFlagUIModeRestoreCameraZoomMax = false;
   mFlagUIModeRestoreCameraZoom = false;
   mFlagUIModeRestoreCameraPitchMin = false;
   mFlagUIModeRestoreCameraPitchMax = false;
   mFlagUIModeRestoreCameraPitch = false;
   mFlagUIModeRestoreCameraYaw = false;
   mFlagSkipNextSelect = false;
//    mFlagContextHelp = true;
//    mFlagSubtitles = false;
//    mFlagFriendOrFoe = false;
   mFlagRestoreCameraEnableUserScroll = true;
   mFlagRestoreCameraEnableUserYaw = true;
   mFlagRestoreCameraEnableUserZoom = true;
   mFlagRestoreCameraEnableAutoZoomInstant = gConfig.isDefined(cConfigCameraAutoZoomInstant);
   mFlagRestoreCameraEnableAutoZoom = true;
   mFlagCameraTeleport = true;
   mFlagIgnoreDpad = false;
   mFlagClearCircleMenuDisplay = false;

   mSkipNextSelectTime = 0.0f;

   // Placement suggestion
   mPlacementSuggestion = XMVectorReplicate(-1.0f);
   mPlacementSocketID = cInvalidObjectID;

   // Initialize our mCircleSelectPoints
   for (long i=0; i<cNumCircleSelectPoints; i++)
      mCircleSelectPoints[i] = cInvalidVector;

   // Initialize user messages
   for( long i = 0; i < USER_MESSAGES_MAX; i++ )
   {
      mUserMessages[i].Init();
   }

   // Initialize controller key times
   memset( mControllerKeyTimes, 0, sizeof( BControllerKeyTimes ) * cNumDC );

   // Input UI trigger stuff
   mInputUIModeFlags.zero();

   // Objective Arrow
   mObjectiveArrowList.clear();
   uint8 maxIndex = gDatabase.getObjectiveArrowMaxIndex();
   for (uint8 i = 0; i < maxIndex; i++)
   {
      BObjectiveArrow* pObjectiveArrow = NULL;
      mObjectiveArrowList.add(pObjectiveArrow);
   }

   readPrivileges();

   mHelpUI.init(this);

   for (uint i=0; i<cNumberHUDItems; i++)
      mHUDItemEnabled[i]=true;


#ifdef ENABLE_CAMERA_BOUNDARY_LINES

   clearCameraBoudnaryLines();

   /*
   BCameraBoundaryLine mapBoundarylimitLine;
   mapBoundarylimitLine.mPoints.add(BVector(100.0f, 0.0f, 100.0f));
   mapBoundarylimitLine.mPoints.add(BVector(100.0f, 0.0f, 200.0f));
   mapBoundarylimitLine.mPoints.add(BVector(200.0f, 0.0f, 200.0f));

   mCameraBoundaryLines.add(mapBoundarylimitLine);
   */

#endif

}

//==============================================================================
// BUser::~BUser
//==============================================================================
BUser::~BUser()
{
   reset();
   //if (mpAchievementList)
   //{
   //   delete mpAchievementList;
   //   mpAchievementList=NULL;
   //}

   delete mpProfile;
   mpProfile = NULL;
}

//==============================================================================
// BUser::setup
//
// This is the main setup, and should not setup anything sim-related.
//==============================================================================
bool BUser::setup(long port)
{
   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);
   mTimerFrequency=freq.QuadPart;
   mTimerFrequencyFloat=(double)mTimerFrequency;

   setPort(port);

   // Camera
   mpCamera=new BCamera();
   mpCamera->setZoomLimitOn(false);
   mpCamera->setPitchLimitOn(false);

   // Free camera   
   mpFreeCamera = new BCamera();
   mpFreeCamera->setZoomLimitOn(false);
   mpFreeCamera->setPitchLimitOn(false);

   setProfile( new BUserProfile(NULL) );

   resetCameraDefaults();
   resetCameraSettings();

   // Selection manager
   mSelectionManager=new BSelectionManager();
   if(!mSelectionManager)
   {
      BASSERT(0);
      return false;
   }

   mSelectionManager->attachUser(this);

   mLastSelectionChangeCounter=0;

   // Circle select
   mCircleSelectBaseSize=0.0f;
   mCircleSelectBaseMaxSize=0.0f;
   gConfig.get(cConfigCircleSelectSize, &mCircleSelectBaseSize);
   gConfig.get(cConfigCircleSelectMaxSize, &mCircleSelectBaseMaxSize);
   mCircleSelectSize=mCircleSelectBaseSize;
   mCircleSelectOpacity = 0.0f;
   mCircleSelectMaxSize=mCircleSelectBaseMaxSize;

   mCircleSelectDecal = gDecalManager.createDecal();
   mHoverDecal = gDecalManager.createDecal();
   mPowerDecal = gDecalManager.createDecal();
     
   if (gConfig.isDefined(cConfigDefaultOptions))
      resetUserOptionsToDefault();

   //TODO: Load up saved options
   calcCameraDefaultPitch();

   for(long i=0; i<cMaxGroups; i++)
      mGroups[i].empty();

   return true;
}

//==============================================================================
// BUser::clearHoverLight
//==============================================================================
void BUser::clearHoverLight(void)
{
   if (mHoverLightHandle != cInvalidLocalLightHandle)
   {
      gSimSceneLightManager.freeLocalLight(mHoverLightHandle);
      mHoverLightHandle = cInvalidLocalLightHandle;
   }  
}

//==============================================================================
// BUser::updateHoverLight
//==============================================================================
void BUser::updateHoverLight(bool enabled)
{
   static bool deleteLight;
   if ((deleteLight) && (mHoverLightHandle != cInvalidLocalLightHandle))
   {
      gSimSceneLightManager.freeLocalLight(mHoverLightHandle);
      mHoverLightHandle = cInvalidLocalLightHandle;
   }
   // rg [9/7/06] - This is mostly intended for material debugging. 
   if (mHoverLightHandle == cInvalidLocalLightHandle)
   {
      static bool spotLight = false;
      mHoverLightHandle = gSimSceneLightManager.createLocalLight();

      static float radius = 90.0f;
      gSimSceneLightManager.setLocalLightRadius(mHoverLightHandle, radius);
      static bool shadowed = true;
      gSimSceneLightManager.setLocalLightShadows(mHoverLightHandle, shadowed);
      BVec3 at(0,0,1);
      BVec3 right(1,0,0);
      gSimSceneLightManager.setLocalLightRight(mHoverLightHandle, (XMFLOAT3*)&right);
      gSimSceneLightManager.setLocalLightAt(mHoverLightHandle, (XMFLOAT3*)&at);
      gSimSceneLightManager.setLocalLightSpotInner(mHoverLightHandle, Math::fDegToRad(70.0f));
      gSimSceneLightManager.setLocalLightSpotOuter(mHoverLightHandle, Math::fDegToRad(90.0f));      
            
      BLocalLightParams& lightParams = gSimSceneLightManager.getLocalLightParams(mHoverLightHandle);
      //lightParams.setColor(XMVectorSet(3.0f, 2.8f, 3.0f, 0.0f));
      static float r = 5.0f;
      static float g = 5.0f;
      static float b = 5.0f;
      lightParams.setColor(XMVectorSet(r, g, b, 0.0f));
      lightParams.setDecayDist(250.0f);
      lightParams.setFarAttenStart(.25f);
      lightParams.setType(spotLight ? cLTSpot : cLTOmni);
      lightParams.setSpecular(true);
      static bool lightBuffered = false;
      lightParams.setLightBuffered(lightBuffered);

      gSimSceneLightManager.enforceLimits(mHoverLightHandle);
   }  
   
   XMFLOAT3 pos(*(XMFLOAT3*)&mHoverPoint.x);
   static float yOfs = 10.0f;
   pos.y += yOfs;
   gSimSceneLightManager.setLocalLightPos(mHoverLightHandle, &pos);
   
   gSimSceneLightManager.setLocalLightEnabled(mHoverLightHandle, enabled && mHoverLightEnabled);
}

//==============================================================================
// BUser::reset
//==============================================================================
void BUser::reset()
{
   mGameStateMessageList.clear();
   setFlagGameStateMessage(false);

   long count=mMenuTextures.getNumber();
   for(long i=0; i<count; i++)
      gUI.unloadTexture(mMenuTextures[i]);
   mMenuTextures.clear();

   // Reset user messages
   for( long i = 0; i < USER_MESSAGES_MAX; i++ )
   {
      mUserMessages[i].Init();
   }

   if(mpCamera)
   {
      delete mpCamera;
      mpCamera=NULL;
   }
   
   if (mpFreeCamera)
   {
      delete mpFreeCamera;
      mpFreeCamera=NULL;
   }

   SAFE_DELETE(mSelectionManager);
   
   clearHoverLight();

   if (mCircleSelectDecal!=-1)
      gDecalManager.destroyDecal(mCircleSelectDecal);
   mCircleSelectDecal=-1;

   if (mHoverDecal!=-1)
      gDecalManager.destroyDecal(mHoverDecal);
   mHoverDecal=-1;

   if (mPowerDecal != -1)
      gDecalManager.destroyDecal(mPowerDecal);
   mPowerDecal = -1;

   for ( int i = mFlashUserMessages.getNumber()-1; i >= 0; --i )
      delete mFlashUserMessages[i];

   mFlashUserMessages.clear();
}

//==============================================================================
// BUser::resetUIContext
//==============================================================================
void BUser::resetUIContext()
{
   if (mpUIContext)
      gUIManager->releaseContext(mpUIContext);
   mpUIContext=NULL;
}

//==============================================================================
// BUser::gameInit
//==============================================================================
void BUser::gameInit(long playerID, long coopPlayerID, BVector lookAtPos, bool leaveCamera, bool defaultCamera, float cameraYaw, float cameraPitch, float cameraZoom)
{   
   mUserMode=cUserModeNormal;
   mLastUserMode = cUserModeNormal;
   mSubMode=cSubModeNone;
   mPlayerID=playerID;
   mCoopPlayerID=coopPlayerID;
   unlockUser();

   // Set up the player to it knows about the user. (is this safe?)
   BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
   BASSERT(pPlayer);
   pPlayer->setUser(this);   

   mPlayerState=pPlayer->getPlayerState();

   mTeamID=pPlayer->getTeamID();
   if(!leaveCamera)
   {
      mHoverPoint=lookAtPos;
      mCameraHoverPointOffsetHeight = 0.0f;
      mCameraHoverPoint=mHoverPoint;
      mLastCameraHoverPoint = mCameraHoverPoint;
   }
   mLookAtPosStaleness = 0;

   setFlagHaveHoverPoint(true);
   mFlagHaveLastHoverPoint=false;
   setFlagHoverPointOverTerrain(true);
   setFlagUpdateHoverPoint(true);
   setFlagCircleSelectPoints(false);
   setFlagModifierSpeed(false);
   setFlagModifierAction(false);
   setFlagCircleSelectReset(false);
   setFlagCircleSelectSelected(false);
   setFlagCircleSelectGrow(false);
   setFlagCircleSelectFullyGrown(false);
   setFlagCircleSelectShrink(false);
   setFlagCircleSelectFullyShrunk(false);
   setFlagCircleSelectDoNotification(false);
   setFlagCircleSelectVinceEvent(false);
   setFlagDelayScrolling(false);
   setFlagDelayScrollingStart(false);
   setFlagGameStateMessage(false);
   setFlagCameraZoomEnabled(true);
   mHoverLightEnabled = false;
   mFlagDeselectOnMenuClose=false;
   mFlagOverrideUnitPanelDisplay=false;
   mFlagRestoreUnitPanelOnModeChange=false;
   mFlagRepairUnavail=false;

   //-- initialize the ui context after the player has been setup for this user
   BDEBUG_ASSERT(mpUIContext==NULL);
   mpUIContext = gUIManager->getOrCreateContext(this);
   
   // Objective Arrow
   uint numArrows = mObjectiveArrowList.getSize();
   for (uint i = 0; i < numArrows; i++)
   {
      BObjectiveArrow* pObjectiveArrow = mObjectiveArrowList[i];
      if (pObjectiveArrow)
      {
         delete (pObjectiveArrow);
         mObjectiveArrowList[i] = NULL;
      }
   }

   mGameStateMessageList.clear();

   resetCameraDefaults();
   mFlagDefaultCamera = defaultCamera;
   if (!defaultCamera)
   {
      float zoomRange = mCameraZoomMax - mCameraZoomMin;
      float pitchRange = mCameraPitchMax - mCameraPitchMin;
      if (zoomRange == 0.0f || pitchRange == 0.0f)
      {
         mCameraZoomMin = cameraZoom;
         mCameraZoomMax = cameraZoom;
         mCameraPitchMin = cameraPitch;
         mCameraPitchMax = cameraPitch;
      }
      else
      {
         if (cameraZoom < mCameraZoomMin)
            mCameraZoomMin = cameraZoom;
         else if (cameraZoom > mCameraZoomMax)
            mCameraZoomMax = cameraZoom;

         if (cameraPitch < mCameraPitchMin)
            mCameraPitchMin = cameraPitch;
         else if (cameraPitch > mCameraPitchMax)
            cameraPitch = mCameraPitchMax;

         zoomRange = mCameraZoomMax - mCameraZoomMin;
         pitchRange = mCameraPitchMax - mCameraPitchMin;

         float zoomPct = (cameraZoom - mCameraZoomMin) / zoomRange;
         float pitchPct = (cameraPitch - mCameraPitchMin) / pitchRange;

         if (zoomPct != pitchPct)
            mCameraPitchMax = ((cameraPitch - mCameraPitchMin) / zoomPct) + mCameraPitchMin;
      }

      mCameraDefaultPitch=cameraPitch;
      mCameraDefaultYaw=cameraYaw;
      mCameraDefaultZoom=cameraZoom;      
   }

   if (!leaveCamera)
   {
      setFlagNoCameraLimits(false);
      
      setFlagFreeCamera(false);
      mCameraAutoZoomInTime=0.0f;
      mCameraAutoZoomInTime=0.0f;
      mFreeCameraMovement.clear();
      mFreeCameraRotation.clear();
      mFreeCameraLModifier=false;
      mFreeCameraRModifier=false;
      mFreeCameraReset=false;
      mFreeCameraCurLoc.clear();
      mFreeCameraCurRot.clear();
      
      resetCameraSettings();

      if(!defaultCamera)
      {
         mCameraYaw=cameraYaw;
         mCameraPitch=cameraPitch;
         mCameraZoom=cameraZoom;
         applyCameraSettings(true);
      }
      else
      {
         // Use _default_ default zoom for initial zoom in campaign
//-- FIXING PREFIX BUG ID 5662
         const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
         BASSERT(pSettings);
         long gameType = -1;
         bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
         if( result && (gameType != BGameSettings::cGameTypeSkirmish) )
         {
            float defaultZoom = 85.0f;
            gConfig.get( cConfigCameraZoom, &defaultZoom );
            mCameraZoom = defaultZoom;
            applyCameraSettings(true);
         }
      }

      clearHoverLight();

      mLastCameraLoc = mpCamera->getCameraLoc();
   }      

   resetCircleSelectionCycle();
   resetGotoSelected();
   resetGotoCameraData();
   resetFindCrowd();

   mGroupIndex=-1;
   mAbilityID=-1;

   

   // Initialize user messages
   for( long i = 0; i < USER_MESSAGES_MAX; i++ )
   {
      mUserMessages[i].Init();
   }

   //DCP 04/13/07:  Adding a lucky config on the luckiest of days.
   if (!gConfig.isDefined(cConfigCameraLimits))
      setFlagNoCameraLimits(true);

   mInputUIButtonRequests.clear();

   mScrollTime=0;
   mScrollX=0.0f;
   mScrollY=0.0f;
   mScrollRate=0.0f;
   mScrollPercent=0.0f;
   mScrollXDelay=0.0f;
   mScrollYDelay=0.0f;
   mScrollDelayTime=0.0f;
   mScrollDirection=cOriginVector;
   mScrollSpeedSampleIndex=0;
   mScrollSpeedSampleCount=0;
   mScrollSpeedAverage=0.0f;
   mScrollStartHoverObject=cInvalidObjectID;
   mScrollStartPos=cOriginVector;
   mScrollAngle=-1.0f;
   mFlagScrollStartSaved=false;
   mFlagPowerMenuOverride = false;
   mFlagPowerMenuEnable = true;
   mFlagStickyReticleDoJump=false;
   mFlagStickyReticleDoFollow=false;
   mFlagTargetSelecting=false;
   mScrollTotalTime=0.0f;
   mScrollTotalDist=0.0f;
   mScrollPrevTotalTime=0.0f;
   mScrollPrevTotalDist=0.0f;
   mScrollPrevDirection=cOriginVector;

   mRumbleHoveringObject=cInvalidObjectID;
   mRumbleHoveringID=-1;

   mFlagCameraScrollEnabled = true;
   mFlagCameraYawEnabled = true;
   mFlagCameraZoomEnabled = true;
   mFlagCameraAutoZoomInstantEnabled = gConfig.isDefined(cConfigCameraAutoZoomInstant);
   mFlagCameraAutoZoomEnabled = true;
   mFlagDelayLocalMilitarySound = false;
   mFlagDelayFlare = false;
   mFlagUIPowerMinigame = false;
   mFlagUIRestartMinigame = true;
   mMinigameTimeFactor = 1.0f;
   mFlagSkipNextSelect = false;
   mSkipNextSelectTime = 0.0f;
   mCameraEffectTransitionOutIndex = -1;
   mClearRestoreCamValuesOnEffectCompletion = false;
   mFlagIgnoreDpad = false;

   mLocalSelectTime=0.0f;

   mFlareTime=0.0f;
   mFlarePos=cOriginVector;

   mTargetSelectObject=cInvalidObjectID;
   mTargetSelectSquad=cInvalidObjectID;
   mTargetSelectDoppleForObject=cInvalidObjectID;

   gInputSystem.enterContext("World");
   gInputSystem.enterContext("Game");
   gInputSystem.enterContext(getUserModeName(cUserModeNormal));

   for (uint i=0; i<cNumberHUDItems; i++)
      mHUDItemEnabled[i]=true;   

   // The initial state of the Minimap and Resource panel is
   // determined by the UIManager and whether or not this is
   // a skirmish or campaign game.
   //setHUDItemEnabled(BUser::cHUDItemMinimap, true);
   //setHUDItemEnabled(BUser::cHUDItemResources, true);
   
   // Set the initial enabled state of the HUD items
#ifndef BUILD_FINAL
   setHUDItemEnabled(BUser::cHUDItemTime, gConfig.isDefined(cConfigShowGameTime));
#else
   // force this to off in final builds
   setHUDItemEnabled(BUser::cHUDItemTime, false);
#endif
   setHUDItemEnabled(BUser::cHUDItemPowerStatus, true);
   setHUDItemEnabled(BUser::cHUDItemUnits, true);
   setHUDItemEnabled(BUser::cHUDItemDpadHelp, false);
   setHUDItemEnabled(BUser::cHUDItemButtonHelp, false);
   setHUDItemEnabled(BUser::cHUDItemReticle, true);
   setHUDItemEnabled(BUser::cHUDItemScore, false);
   setHUDItemEnabled(BUser::cHUDItemUnitStats, false);
   setHUDItemEnabled(BUser::cHUDItemUnitStats, true);

   // Help UI
   mHelpUI.refresh( (long)getOption_ControlScheme() );

   /*
   if( gConfig.isDefined(cConfigDemo) )
       setOption_ShowButtonHelp(getOption_ShowButtonHelp());
   */

   for ( int i = mFlashUserMessages.getNumber()-1; i >= 0; --i )
      delete mFlashUserMessages[i];

   mFlashUserMessages.clear();

   // Save game file name
   mSaveGameFile.empty();
}

//==============================================================================
// BUser::gameRelease
//==============================================================================
void BUser::gameRelease()
{
   gInputSystem.leaveContext(getUserModeName(cUserModeNormal));
   gInputSystem.leaveContext("Game");
   gInputSystem.leaveContext("World");

   mUserMode=cUserModeNormal;
   mLastUserMode = cUserModeNormal;
   mSubMode=cSubModeNone;
   mPlayerID=-1;
   mCoopPlayerID=-1;
   mTeamID=-1;
   clearAllSelections();
   clearHoverLight();   
   mInputUIButtonRequests.clear();
   deletePowerUser();

   if (mRumbleHoveringID != -1)
   {
      gUI.stopRumble(mRumbleHoveringID);
      mRumbleHoveringID=-1;
   }
   mRumbleHoveringObject=cInvalidObjectID;

   resetUIContext();   

   mCommandedSquads.clear();
}

//==============================================================================
// BUser::update
//==============================================================================
void BUser::update(float elapsedTime)
{
   if(!getFlagGameActive())
      return;

   if (getFlagGameDoExit())
   {
      // reset the flag
      setFlagGameDoExit(false);

      switch (mRequestedExitMethod)
      {
         case cExitMethodRestart:
         {
#ifndef BUILD_FINAL
            if (gFinalBuild)
            {
               if (DmRebootEx(DMBOOT_TITLE, "e:\\demo\\default.xex", "e:\\demo", "") != XBDM_NOERR)
                  DmReboot(DMBOOT_TITLE);
            }
#endif               

            const BGameSettings* pSettings = gDatabase.getGameSettings();
            BASSERT(pSettings);
            long gameType = -1;
            pSettings->getLong(BGameSettings::cGameType, gameType);
            if (gameType == BGameSettings::cGameTypeCampaign)
            {
               // SRL, 11/6/08 - If we are replaying the campaign scenario, we need to set the current 
               //    campaign node back to this node because at the end of the game we incremented it so that
               //    it would catch a profile write.
               BCampaign * pCampaign = gCampaignManager.getCampaign(0);
               BASSERT(pCampaign);
               BString mapName;
               pSettings->getString( BGameSettings::cMapName, mapName );
               long nodeID = pCampaign->getNodeIDByFilename(mapName);
               pCampaign->setCurrentNodeID(nodeID);
            }


            //gModeManager.setMode( BModeManager::cModeMenu );
            //gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoGame );

            BModeGame* pModeGame = gModeManager.getModeGame();
            pModeGame->leave(pModeGame);
            pModeGame->enter(pModeGame);
         }
         break;
         case cExitMethodQuit:
         {
            //-- FIXING PREFIX BUG ID 5897
            const BGameSettings* pSettings = gDatabase.getGameSettings();
            //--
            BASSERT(pSettings);
            long gameType = -1;
            pSettings->getLong(BGameSettings::cGameType, gameType);
            if (gameType != BGameSettings::cGameTypeSkirmish)
            {
               BCampaign * pCampaign = gCampaignManager.getCampaign(0);
               BASSERT(pCampaign);
               pCampaign->setPlayContinuous(false);
            }
            exitGameMode(cExitMethodQuit);
         }
         break;

         case cExitMethodContinue:
            exitGameMode(cExitMethodContinue);
            break;

         case cExitMethodGoToAdvancedTutorial:
            exitGameMode(cExitMethodGoToAdvancedTutorial);
            break;

         default:
            exitGameMode(cExitMethodQuit);
            break;
      }
      return;
   }

   // PAUSE HANDLING
   // if we're not running a multiplayer game,
   // and the game isn't already paused,
   // then check for the blade notification or
   // controller disconnect
   if (!gLiveSystem->isMultiplayerGameActive() && 
       !gModeManager.getModeGame()->getPaused() )
   {
      bool bShouldPause = false;
      // only do this while in-game in a non-MP game.s
      BOOL uiShowing = gNotification.isSystemUIShowing();
      if (mSystemUIShowing != uiShowing)
      {
         mSystemUIShowing = !mSystemUIShowing;
         bShouldPause = !!mSystemUIShowing;
      }
      else if (gNotification.getInputDeviceChangedNotification())
      {
         // check the state of the controller for mPort
         XINPUT_STATE state;
         if (mPort != -1 && XInputGetState(mPort, &state) == ERROR_DEVICE_NOT_CONNECTED)
         {
            bShouldPause = true;
         }
      }

      if (bShouldPause)
      {
         if (getUserMode() != cUserModeCinematic && !gUIManager->isNonGameUIVisible() && gWorld->getTransitionManager()->getTransitionComplete() )
            gUIManager->showNonGameUI( BUIManager::cGameMenu, this );
         else if (gUIManager->getScenarioResult() == -1) // don't pause if we're at the end of a scenario.  Bad things happen. :(
            gModeManager.getModeGame()->setPaused( true );
      }
   }
   
   // detect profile changes

   if(elapsedTime>0.1f)
      elapsedTime=0.1f;

   updateObjectiveMessages(elapsedTime);   

   updateUIHints(elapsedTime);

   updateFlashUserMessages(elapsedTime);

   if( gModeManager.getModeGame() && !gModeManager.getModeGame()->getPaused() )
      updateChatMessages(elapsedTime);


   updateGameStateMessages(elapsedTime);

   updateDebug(elapsedTime);

   updateUIContext(elapsedTime);

   updateCommandSquads(elapsedTime);

   if(!gConfig.isDefined(cConfigNoHelpUI))
      mHelpUI.update( (long)getOption_ControlScheme() );

//   refreshScores();

   // Update rumble
   if (mHoverObject != mRumbleHoveringObject)
   {
      if (mRumbleHoveringID != -1)
      {
         gUI.stopRumble(mRumbleHoveringID);
         mRumbleHoveringID=-1;
      }
      mRumbleHoveringObject = mHoverObject;
      if (mRumbleHoveringObject != cInvalidObjectID)
      {
//-- FIXING PREFIX BUG ID 5663
         const BObject* pObject = gWorld->getObject(mRumbleHoveringObject);
//--
         if (pObject)
         {
            const BRumbleEvent* pRumbleData = pObject->getProtoObject()->getHoveringRumbleData();
            if (pRumbleData)
               mRumbleHoveringID = gUI.playRumbleEvent(BRumbleEvent::cTypeUnitHovering, pRumbleData, true);
         }
      }
   }

   updateCircleSelectShrink(elapsedTime);

   updateTargetSelect();

//   gUIManager->updateNonGameUI(elapsedTime);

   bool shouldClampCamera = true;

   switch(mUserMode)
   {
      case cUserModeNormal:
      {
         updateHoverPoint();
         updateCamera(elapsedTime);
         break;
      }

      case cUserModeCircleSelecting:
         updateHoverPoint();
         updateCamera(elapsedTime);
         updateCircleSelect(elapsedTime);
         break;

      case cUserModeInputUILocation:
      case cUserModeInputUIUnit:
      case cUserModeInputUISquad:
      case cUserModeInputUISquadList:
      case cUserModeInputUIPlaceSquadList:
         updateHoverPoint();
         updateCamera(elapsedTime);
         break;

      case cUserModeInputUILocationMinigame:
         updateHoverPoint();
         updateCamera(elapsedTime);
         updateMinigame();
         break;

      case cUserModeBuildLocation:
      case cUserModeRallyPoint:
      case cUserModeBuildingRallyPoint:
         updateHoverPoint();
         updateCamera(elapsedTime);
         updateHoverVisual(elapsedTime);
         break;

      case cUserModeAbility:
         updateHoverPoint();
         updateCamera(elapsedTime);
         break;

      case cUserModeCommandMenu:
      {
         BObject* pObject=gWorld->getObject(mCommandObject);
         if (pObject)
         {
            // If we had a socket base object selected and now it has a child socket object attached,
            // unselect the base and select the child. We only want socket child objects selectable
            // whenever the base is filled in.
//-- FIXING PREFIX BUG ID 5664
            const BEntityRef* pRef=pObject->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug);
//--
            if (pRef)
            {
               BUnit* pChild=gWorld->getUnit(pRef->mID);
               if (pChild)
               {
                  pObject=pChild;
                  mCommandObject=pChild->getID();
                  mSelectionManager->clearSelections();
                  mSelectionManager->selectUnit(mCommandObject);
                  mFlagCommandMenuRefresh=true;
               }
            }
         }

         if (pObject && mCommandObject.getType() == BEntity::cClassTypeDopple)
         {
            // if the hover object was a dopple, then get the actual object
            pObject=gWorld->getObject(pObject->getParentID());
         }

         // Keep the command object player ID up to date
         if (pObject && pObject->isAlive() && mCommandObjectPlayerID==0 && pObject->getPlayerID()!=0)
            mCommandObjectPlayerID=pObject->getPlayerID();

         // Leave the command menu mode if the object died or was converted to another player
         if (!pObject || !pObject->isAlive() || pObject->getPlayerID()!=mCommandObjectPlayerID || !pObject->isVisible(mTeamID))
         {
            changeMode(cUserModeNormal);
            if (pObject && pObject->isAlive())
               mSelectionManager->selectUnit(pObject->getID());
         }
         else
         {
            refreshCommandMenu();         
            mCircleMenu.update();         
         }
         break;
      }

      case cUserModePowerMenu:
         if(mSubMode==cSubModeNone && getFlagPowerMenuRefresh())
         {
            setFlagPowerMenuRefreshTrainProgressOnly(true);
            refreshPowerMenu();         
         }
         mCircleMenu.update();         
         break;



      case cUserModeCinematic:
         if (getFlagFreeCamera())
         {
            updateFreeCamera(elapsedTime);
         }
         shouldClampCamera = false;
         break;

      case cUserModeFollow:
      {
//-- FIXING PREFIX BUG ID 5665
         const BEntity* pFollowEntity = gWorld->getEntity(mFollowEntity);
//--
         BVector pos = mpCamera->getCameraLoc();
         if (pFollowEntity)
         {
            BVector entityPos;
            long classType = pFollowEntity->getClassType();
            if (classType == BEntity::cClassTypeSquad)
               entityPos = ((BSquad*)pFollowEntity)->getInterpolatedPosition();
            else if (classType == BEntity::cClassTypeUnit || classType == BEntity::cClassTypeObject || classType == BEntity::cClassTypeProjectile)
               entityPos = ((BObject*)pFollowEntity)->getInterpolatedPosition();
            else
               entityPos = pFollowEntity->getPosition();
            pos = entityPos - (mpCamera->getCameraDir() * mCameraZoom);
            mpCamera->setCameraLoc(pos);
            tieToCameraRep();
            setFlagUpdateHoverPoint(true);
         }          
         updateHoverPoint();
         updateCamera(elapsedTime);
         break;
      }

      case cUserModePower:
      {
         if(mpPowerUser && mpPowerUser->getFlagDestroy())
            deletePowerUser();

         if(!mpPowerUser)
         {
            resetUserMode();
            break;
         }

         mpPowerUser->update(elapsedTime);
         updateCamera(elapsedTime);

         shouldClampCamera = mpPowerUser->shouldClampCamera();
         break;
      }
   }

   shakeCamera();

   // Update the current camera effect.  If anything changed, update the user camera pos / orient values
   bool camUpdated = mpCamera->updateCameraEffect(gWorld->getSubGametime(), mCameraHoverPoint);
   if (camUpdated)
   {
      float pitch = 0.0f;
      if (mpCamera->getXZPitch(pitch))
         mCameraPitch = -pitch * cDegreesPerRadian;
      mCameraZoom = mpCamera->getCameraLoc().distance(mCameraHoverPoint);
      mCameraYaw = mpCamera->getXZYaw() * cDegreesPerRadian;
   }
   else
   {
      if (mClearRestoreCamValuesOnEffectCompletion)
      {
         mUIModeRestoreCameraZoom = 0.0f;
         mUIModeRestoreCameraYaw = 0.0f;
         mUIModeRestoreCameraPitch = 0.0f;
         mFlagUIModeRestoreCameraZoom = false;
         mFlagUIModeRestoreCameraYaw = false;
         mFlagUIModeRestoreCameraPitch = false;
         mClearRestoreCamValuesOnEffectCompletion = false;
      }
   }

   updateSelectedUnitIcons();

   if (getPlayer()->isHuman() && !gModeManager.getModeGame()->getPaused() && mUserMode != cUserModeCinematic && !gWorld->getFlagGameOver())
   {
      // [7/1/2008 xemu] modified to always run through the command pipeline in both MP and SP 
      if (gWorld->getGametimeFloat() >= mLookAtPosStaleness)
      {
         BGameCommand* pCommand = (BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
         if(pCommand)
         {
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setSenders(1, &mPlayerID);
            pCommand->setRecipientType(BCommand::cGame);
            pCommand->setType(BGameCommand::cTypeLookAtPosBroadcast);
            BVector pos(mHoverPoint.x, 0.0f, mHoverPoint.z);
            pCommand->setPosition(pos);
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
         }
         mLookAtPosStaleness = (float)(gWorld->getGametimeFloat() + 0.2);
      }
   }   

   gGeneralEventManager.flushQueue(mPlayerID, elapsedTime);

   // Handle flare delay
   if (mFlagDelayFlare)
   {
      if (mFlareTime == 0.0f)
      {
         mFlareTime = 0.25f;
         gConfig.get(cConfigGamepadDoubleClickTime, &mFlareTime);
      }
      else
      {
         mFlareTime-=elapsedTime;
         if (mFlareTime <= 0.0f)
         {
            mFlagDelayFlare=false;
            mFlareTime=0.0f;
            sendFlare(-1, mFlarePos);
         }
      }
   }

   // Handle playing delayed local military sound confirmation when both local and all military are on the same button (local is single click and all is double click)
   if (mFlagDelayLocalMilitarySound)
   {
      if (mLocalSelectTime == 0.0f)
      {
         mLocalSelectTime=0.25f;
         gConfig.get(cConfigGamepadDoubleClickTime, &mLocalSelectTime);
      }
      else
      {
         mLocalSelectTime-=elapsedTime;
         if (mLocalSelectTime <= 0.0f)
         {
            mFlagDelayLocalMilitarySound=false;
            mLocalSelectTime=0.0f;
            if (mSelectionManager->getNumberSelectedSquads() > 0)
               gUIGame.playSound(BSoundManager::cSoundLocalMilitary, getPlayer()->getCivID(), true);
            else
               gUI.playCantDoSound();
         }
      }
   }

   if(shouldClampCamera)
      clampCamera();

   // Store last camera position
   //saveLastCameraLoc();

   // Update objective arrows' positions
   updateObjectiveArrows();

   //Display the Base number when you jump to it
   if(mBaseNumberDisplayTime > 0.0f)
      mBaseNumberDisplayTime -= elapsedTime;

}

//==============================================================================
// BUser::updateGameStateMessages
//==============================================================================
void BUser::updateGameStateMessages(float elapsedTime)
{
   bool nextMsg=false;
   if(!getFlagGameStateMessage())
   {
      if(mGameStateMessageList.getNumber()>0)
         nextMsg=true;
   }
   else
   {
      mGameStateMessageTimer+=elapsedTime;
      if(mGameStateMessageTimer>=cGameStateMessageTimeout)
      {
         setFlagGameStateMessage(false);
         if(mGameStateMessageList.getNumber()>0)
            nextMsg=true;
      }
      else if (mGameStateMessageType==cGameStateMessageTributeReceived && mGameStateMessageList.getSize()>0)
      {
//-- FIXING PREFIX BUG ID 5666
         const BUserGameStateMessage& msg=mGameStateMessageList[0];
//--
         if (msg.mType==mGameStateMessageType)
            nextMsg=true;
      }
   }
   if(nextMsg)
   {
      mGameStateMessageText.empty();
      BUserGameStateMessage msg=mGameStateMessageList[0];
      switch(msg.mType)
      {
         case cGameStateMessageSupportPowerAvailable:
         {
            if(msg.mPlayerID==mPlayerID)
            {
               gUIGame.playSound(BSoundManager::cSoundSupportPowerAvailable);
               mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(22000));//Support Power Unlocked
            }
            break;
         }
         case cGameStateMessageResigned:
         {
//-- FIXING PREFIX BUG ID 5667
            const BPlayer* pPlayer=gWorld->getPlayer(msg.mPlayerID);
//--
            if(pPlayer)
            {
               gUIGame.playSound(BSoundManager::cSoundPlayerResigned);
               mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(22001), pPlayer->getLocalisedDisplayName().getPtr());//%S Resigned
            }
            break;
         }
         case cGameStateMessageDefeated:
         {
            if(msg.mPlayerID==mPlayerID)
            {
               gUIGame.playSound(BSoundManager::cSoundStopAllExceptMusic);

               const BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
               if(pPlayer && pPlayer->getTeam() && (pPlayer->getTeam()->getNumberPlayers() > 1) && gWorld->getFlagGameOver())
                  gUIGame.playSound(BSoundManager::cSoundVOGLossTeam);
               else
                  gUIGame.playSound(BSoundManager::cSoundVOGLoss);

               mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(22002));//Defeated
            }
            else
            {
//-- FIXING PREFIX BUG ID 5668
               const BPlayer* pPlayer=gWorld->getPlayer(msg.mPlayerID);
//--
               if(pPlayer)
               {
                  gUIGame.playSound(BSoundManager::cSoundPlayerDefeated);
                  mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(22003), pPlayer->getLocalisedDisplayName().getPtr());//%S Defeated
               }
            }
            break;
         }
//         case cGameStateMessageDisconnected:
//         {
//            if(msg.mPlayerID==mPlayerID)
//            {
//               gUIGame.playSound(BSoundManager::cSoundStopAllExceptMusic);
//               gUIGame.playSound(BSoundManager::cSoundPlayerResigned);
//               mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(24155));//Disconnected
//            }
//            else
//            {
////-- FIXING PREFIX BUG ID 5669
//               const BPlayer* pPlayer=gWorld->getPlayer(msg.mPlayerID);
////--
//               if(pPlayer)
//               {
//                  gUIGame.playSound(BSoundManager::cSoundPlayerResigned);
//                  mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(24156), pPlayer->getLocalisedDisplayName().getPtr());//%S Disconnected
//               }
//            }
//            break;
//         }
         case cGameStateMessageWon:
         {
            if(msg.mPlayerID==mPlayerID)
            {
               gUIGame.playSound(BSoundManager::cSoundStopAllExceptMusic);

               const BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
               if(pPlayer && pPlayer->getTeam() && (pPlayer->getTeam()->getNumberPlayers() > 1))
                  gUIGame.playSound(BSoundManager::cSoundVOGWinTeam);
               else
                  gUIGame.playSound(BSoundManager::cSoundVOGWin);

               mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(22004));//Victory!
            }
            break;
         }
         case cGameStatePlaybackDone:
         {
            gUIGame.playSound(BSoundManager::cSoundPlaybackDone);
            mGameStateMessageText.locFormat(gDatabase.getLocStringFromID(22005));//Playback Complete
            break;
         }
         case cGameStateMessageTributeReceived:
         {
            if(msg.mPlayerID==mPlayerID)
            {
//-- FIXING PREFIX BUG ID 5670
               const BPlayer* pFromPlayer=gWorld->getPlayer(msg.mData);
//--
               if (pFromPlayer)
               {
                  for (uint i=0; i<gUIGame.getNumberTributes(); i++)
                  {
                     const BUIGameTribute* pTribute=gUIGame.getTribute(i);
                     if (pTribute->mResourceID==msg.mData2)
                     {
                        gUIGame.playSound(BSoundManager::cSoundTributeReceived);
                        mGameStateMessageText.locFormat(gDatabase.getLocStringFromIndex(pTribute->mReceivedStringIndex), pFromPlayer->getLocalisedDisplayName().getPtr());
                        break;
                     }
                  }
               }
            }
            break;
         }
      }
      mGameStateMessageList.removeIndex(0, true);
      if(!mGameStateMessageText.isEmpty())
      {
         setFlagGameStateMessage(true);
         mGameStateMessageTimer=0.0f;
         mGameStateMessageType=msg.mType;
      }
   }
}

//==============================================================================
// BUser::updateDebug
//==============================================================================
void BUser::updateDebug(float elapsedTime)
{
#ifdef DEBUG_SIMREP_RAYCAST
   if(debugState)
   {
      if(!debugDrawn)
      {
         debugDir.normalize();
         gpDebugPrimitives->clear();
         gpDebugPrimitives->addDebugLine(debugLoc, debugLoc+(debugDir*100.0f), cDWORDYellow, cDWORDYellow, BDebugPrimitives::cCategoryNone, 100.0f);
         debugDrawn=true;
      }
      BVector point=cOriginVector;
      if(gTerrainSimRep.rayIntersects(debugLoc, debugDir, point))
      {
         static int debugCount=0;
         debugCount++;
      }
   }
#endif

   // These are used in a couple places here...
   BUnit *pHoverUnit = gWorld->getUnit(mHoverObject);
   BProtoObject *pHoverProtoObject = pHoverUnit ? const_cast<BProtoObject*>(pHoverUnit->getProtoObject()) : NULL;

   // Handle modifying pick settings when in debug selection picking mode
   if(gConfig.isDefined(cConfigDebugSelectionPicking) && !gLiveSystem->isMultiplayerGameActive() && mHoverObject!=cInvalidObjectID)
   {
      float pickChangeX=(mPickChangeXPlus && mPickChangeXMinus ? 0.0f : mPickChangeXPlus ? 1.0f : mPickChangeXMinus ? -1.0f : 0.0f);
      float pickChangeY=(mPickChangeYPlus && mPickChangeYMinus ? 0.0f : mPickChangeYPlus ? 1.0f : mPickChangeYMinus ? -1.0f : 0.0f);
      if(pickChangeX!=0.0f || pickChangeY!=0.0f)
      {
         const float cPickChangeRate=1.0f;
         if(pHoverProtoObject)
         {
            if(pHoverProtoObject->getPickRadius()==0.0f)
            {
               if (pHoverUnit)
                  pHoverProtoObject->setPickRadius(pHoverUnit->getVisualRadius());
            }
            if(pickChangeY!=0.0f)
               pHoverProtoObject->setPickOffset(pHoverProtoObject->getPickOffset()+(cPickChangeRate*elapsedTime*pickChangeY));
            if(pickChangeX!=0.0f)
            {
               pHoverProtoObject->setPickRadius(pHoverProtoObject->getPickRadius()+(cPickChangeRate*elapsedTime*pickChangeX));
               if(pHoverProtoObject->getPickRadius()<0.5f)
                  pHoverProtoObject->setPickRadius(0.5f);
            }
         }
      }

      float selectionChangeX=(mSelectionChangeXPlus && mSelectionChangeXMinus ? 0.0f : mSelectionChangeXPlus ? 1.0f : mSelectionChangeXMinus ? -1.0f : 0.0f);
      float selectionChangeZ=(mSelectionChangeZPlus && mSelectionChangeZMinus ? 0.0f : mSelectionChangeZPlus ? 1.0f : mSelectionChangeZMinus ? -1.0f : 0.0f);
      if(selectionChangeX!=0.0f || selectionChangeZ!=0.0f)
      {
         const float cSelectionChangeRate=1.0f;
         if(pHoverProtoObject)
         {
            if(pHoverProtoObject->getPickRadius()==0.0f)
            {
               if (pHoverUnit)
                  pHoverProtoObject->setPickRadius(pHoverUnit->getVisualRadius());
            }
            if(selectionChangeX!=0.0f)
            {
               pHoverProtoObject->setSelectedRadiusX(pHoverProtoObject->getSelectedRadiusX()+(cSelectionChangeRate*elapsedTime*selectionChangeX));
               if(pHoverProtoObject->getSelectedRadiusX()<0.1f)
                  pHoverProtoObject->setSelectedRadiusX(0.1f);
            }
            if(selectionChangeZ!=0.0f)
            {
               pHoverProtoObject->setSelectedRadiusZ(pHoverProtoObject->getSelectedRadiusZ()+(cSelectionChangeRate*elapsedTime*selectionChangeZ));
               if(pHoverProtoObject->getSelectedRadiusZ()<0.1f)
                  pHoverProtoObject->setSelectedRadiusZ(0.1f);
            }
         }
      }

      float obstructionChangeX=(mObstructionChangeXPlus && mObstructionChangeXMinus ? 0.0f : mObstructionChangeXPlus ? 1.0f : mObstructionChangeXMinus ? -1.0f : 0.0f);
      float obstructionChangeY=(mObstructionChangeYPlus && mObstructionChangeYMinus ? 0.0f : mObstructionChangeYPlus ? 1.0f : mObstructionChangeYMinus ? -1.0f : 0.0f);
      float obstructionChangeZ=(mObstructionChangeZPlus && mObstructionChangeZMinus ? 0.0f : mObstructionChangeZPlus ? 1.0f : mObstructionChangeZMinus ? -1.0f : 0.0f);
      if(obstructionChangeX!=0.0f || obstructionChangeY!=0.0f || obstructionChangeZ!=0.0f)
      {
         const float cSelectionChangeRate=1.0f;
         if(pHoverUnit && pHoverProtoObject)
         {
            if(obstructionChangeX!=0.0f)
            {
               pHoverProtoObject->setObstructionRadiusX(pHoverProtoObject->getObstructionRadiusX()+(cSelectionChangeRate*elapsedTime*obstructionChangeX));
               if(pHoverProtoObject->getObstructionRadiusX()<0.1f)
                  pHoverProtoObject->setObstructionRadiusX(0.1f);
               pHoverUnit->setObstructionRadiusX(pHoverProtoObject->getObstructionRadiusX());
            }
            if(obstructionChangeY!=0.0f)
            {
               pHoverProtoObject->setObstructionRadiusY(pHoverProtoObject->getObstructionRadiusY()+(cSelectionChangeRate*elapsedTime*obstructionChangeY));
               if(pHoverProtoObject->getObstructionRadiusY()<0.1f)
                  pHoverProtoObject->setObstructionRadiusY(0.1f);
               pHoverUnit->setObstructionRadiusY(pHoverProtoObject->getObstructionRadiusY());
            }
            if(obstructionChangeZ!=0.0f)
            {
               pHoverProtoObject->setObstructionRadiusZ(pHoverProtoObject->getObstructionRadiusZ()+(cSelectionChangeRate*elapsedTime*obstructionChangeZ));
               if(pHoverProtoObject->getObstructionRadiusZ()<0.1f)
                  pHoverProtoObject->setObstructionRadiusZ(0.1f);
               pHoverUnit->setObstructionRadiusZ(pHoverProtoObject->getObstructionRadiusZ());
            }
            pHoverUnit->updateSimBoundingBox();
         }
      }
   }
}

//==============================================================================
// BUser::updateScrolling
//==============================================================================
void BUser::updateScrolling(float elapsedTime)
{
   // The elapsed time can get really big if the game is paused while the user was holding down on the left stick.
   if (elapsedTime > 0.1f)
      elapsedTime = 0.1f;

   bool stickyReticle=false;
   if (getOption_StickyCrosshairSensitivity() > 0 && !mFlagTargetSelecting)
   {
      if (mUserMode==cUserModeNormal || mUserMode==cUserModeInputUISquad || mUserMode==cUserModeInputUIUnit )
      {
         stickyReticle=true;
      }
   }

   if(getFlagDelayScrolling())
   {
      if(mScrollX==0.0f && mScrollY==0.0f)
         setFlagDelayScrolling(false);
      else
      {
         if(getFlagDelayScrollingStart())
         {
            gConfig.get(cConfigDelayScrollingTime, &mScrollDelayTime);
            setFlagDelayScrollingStart(false);
         }
         else
         {
            if (elapsedTime > 0.1f)
               mScrollDelayTime-=0.1f;
            else
               mScrollDelayTime-=elapsedTime;
            if(mScrollDelayTime<=0.0f)
               setFlagDelayScrolling(false);
            else
            {
               float releaseAngle=0.0f;
               gConfig.get(cConfigDelayScrollingAngle, &releaseAngle);
               releaseAngle*=cRadiansPerDegree;

               float delayAngle=xyAngle(mScrollXDelay, mScrollYDelay);
               float currentAngle=xyAngle(mScrollX, mScrollY);

               if(fabs(currentAngle-delayAngle)>=releaseAngle)
                  setFlagDelayScrolling(false);
            }
         }
      }
   }

   if(mScrollX==0.0f && mScrollY==0.0f)
   {
      mScrollRate=0.0f;
      gConfig.get(cConfigScrollRateBase, &mScrollRate);
   }
   else
   {
      if ((mScrollRate<1.0f) && !getFlagDelayScrolling())
      {
         float grow=0.0f;
         gConfig.get(cConfigScrollRateGrow, &grow);
         mScrollRate+=grow*elapsedTime;
         if(mScrollRate>1.0f)
            mScrollRate=1.0f;
      }
   }

   if (!getFlagDelayScrolling() && (mScrollX != 0.0f || mScrollY != 0.0f))
   {
      mStickyHoverObject = cInvalidObjectID;
      mFlagStickyReticleDoJump = false;
      mFlagStickyReticleDoFollow = false;

      if (stickyReticle)
      {
         // Save off the hover object that was current at the start of scrolling
         if (!mFlagScrollStartSaved)
         {
            mScrollStartHoverObject=mHoverObject;
            mScrollStartPos=mpCamera->getCameraLoc();
            mFlagScrollStartSaved=true;
            mScrollAngle=-1.0f;
            mScrollPrevTotalTime=0.0f;
            mScrollPrevTotalDist=0.0f;
            mScrollPrevDirection=cOriginVector;
            mScrollTotalTime=0.0f;
            mScrollTotalDist=0.0f;
            gpDebugPrimitives->clear(BDebugPrimitives::cCategoryControls);
         }
      }

      float x=mScrollX;
      float y=mScrollY;

      float h=(float)sqrt(x*x+y*y);

      float scrollMaxSpeed = 0.0f;
      float scrollRampPoint = 0.0f;
      float scrollRampSpeed = 0.0f;
      float scrollExp = 0.0f;
      float scrollUserSpeedAdjustment = cDefaultScrollSpeedAdjFloat;

      if(getFlagModifierSpeed())
         gConfig.get(cConfigScrollFastSpeed, &scrollMaxSpeed);
      else
         gConfig.get(cConfigScrollMaxSpeed, &scrollMaxSpeed);

      scrollUserSpeedAdjustment = scrollSpeedAdjByteToFloat(getOption_ScrollSpeed());
      scrollMaxSpeed *= scrollUserSpeedAdjustment;

      gConfig.get(cConfigScrollRampPoint, &scrollRampPoint);
      gConfig.get(cConfigScrollRampSpeed, &scrollRampSpeed);
      gConfig.get(cConfigScrollExp, &scrollExp);
      
      BDEBUG_ASSERT((scrollMaxSpeed > 0.0f) && (scrollRampPoint > 0.0f) && (scrollRampSpeed > 0.0f));
            
      float speedPct=1.0f;

      float h2=h/0.8f;

      if(h2>=1.0f)
         speedPct=1.0f;
      else if(h2<=scrollRampPoint)
      {
         speedPct=h2/scrollRampPoint*scrollRampSpeed;
      }
      else
      {
         float val=((h2-scrollRampPoint)/(1.0f-scrollRampPoint))*scrollExp;
         float val2=(val*val)/(scrollExp*scrollExp);
         speedPct=(val2*(1.0f-scrollRampSpeed))+scrollRampSpeed;
      }

      mScrollPercent=speedPct;

      speedPct*=mScrollRate;

      float d=speedPct*scrollMaxSpeed*elapsedTime;

      BVector pos1=mpCamera->getCameraLoc();

      float xm=(x/h)*d;
      if(xm!=0.0f)
      {
         //mpCamera->moveRight(xm);
         BVector right = mpCamera->getCameraRight();
         right.y = 0.0f;
         right.normalize();
         right.scale(xm);
         mCameraHoverPoint += right;
      }

      float ym=(y/h)*d;
      if(ym!=0.0f)
      {
         //mpCamera->moveWorldForward(-ym);
         BVector forward = mpCamera->getCameraRight();
         forward.y = 0.0f;
         forward.normalize();
         forward.rotateXZ(cPiOver2);
         forward.scale(ym);
         mCameraHoverPoint += forward;
      }

      if((xm!=0.0f) || (ym!=0.0f))
      {
         // Plant postion in camera height field
         computeClosestCameraHoverPointVertical(mCameraHoverPoint, mCameraHoverPoint);
      }

      setFlagUpdateHoverPoint(true);

      clampCamera();

      BVector pos2=mpCamera->getCameraLoc();

      if (mFlagTargetSelecting && gConfig.isDefined(cConfigExitTargetSelectOnScroll))
         autoExitSubMode();

      if (stickyReticle)
      {
         // Save off the scroll direction
         BVector dir=pos2-pos1;
         float dist = dir.length();
         if (dist > cFloatCompareEpsilon)
         {
            BVector prevDir=mScrollDirection;
            mScrollDirection=dir;
            mScrollDirection.normalize();

            // If the angle change is too great, reset the scroll start data and the total scroll time & distance.
            float newAngle = mScrollDirection.getAngleAroundY();
            if (mScrollAngle != -1.0f)
            {
               float angleTolerance=0.0f;
               gConfig.get(cConfigStickyReticleJumpToAngleTolerance, &angleTolerance);
               if (angleTolerance != 0.0f)
               {
                  angleTolerance = Math::fDegToRad(angleTolerance);
                  float angleDiff = Math::fAbs(newAngle - mScrollAngle);
                  if (angleDiff > cPi)
                     angleDiff = Math::fAbs(angleDiff - cTwoPi);
                  if (angleDiff > angleTolerance)
                  {
                     setFlagUpdateHoverPoint(true);
                     updateHoverPoint(1.0f, 1.0f, true);
                     if (mScrollStartHoverObject!=mHoverObject)
                        mScrollStartHoverObject=cInvalidObjectID;
                     mScrollStartPos=mpCamera->getCameraLoc();
                     mScrollPrevTotalTime=mScrollTotalTime;
                     mScrollPrevTotalDist=mScrollTotalDist;
                     mScrollPrevDirection=prevDir;
                     mScrollTotalTime=0.0f;
                     mScrollTotalDist=0.0f;
                     gpDebugPrimitives->clear(BDebugPrimitives::cCategoryControls);
                  }
               }
            }
            mScrollAngle=newAngle;
         }
         else
            mScrollDirection=cOriginVector;

         // Update the amount of time scrolling and the distance
         mScrollTotalTime+=elapsedTime;
         mScrollTotalDist+=dist;

         // Update the average scroll speed
         float speedInterval=0.0f;
         gConfig.get(cConfigStickyReticleAverageSpeedInterval, &speedInterval);
         if (speedInterval > 0.0f)
         {
            mScrollSpeedSampleDistances[mScrollSpeedSampleIndex]=dist;
            mScrollSpeedSampleElapsedTimes[mScrollSpeedSampleIndex]=elapsedTime;
            if (mScrollSpeedSampleCount < cMaxScrollSpeedSamples)
            {
               mScrollSpeedSampleCount++;
               if (mScrollSpeedSampleIndex < cMaxScrollSpeedSamples-1)
                  mScrollSpeedSampleIndex++;
               else
                  mScrollSpeedSampleIndex=0;
            }
            else
            {
               if (mScrollSpeedSampleIndex < cMaxScrollSpeedSamples-1)
                  mScrollSpeedSampleIndex++;
               else
                  mScrollSpeedSampleIndex=0;
            }
            float totalDist=0.0f;
            float totalElapsed=0.0f;
            for (uint i=0; i<mScrollSpeedSampleCount; i++)
            {
               totalDist+=mScrollSpeedSampleDistances[i];
               totalElapsed+=mScrollSpeedSampleElapsedTimes[i];
            }
            if (totalElapsed>0.0f)
               mScrollSpeedAverage=totalDist/totalElapsed;
            else
               mScrollSpeedAverage=0.0f;
         }
         else
         {
            if (elapsedTime>0.0f)
               mScrollSpeedAverage=dist/elapsedTime;
            else
               mScrollSpeedAverage=0.0f;
         }
      }

      //gConsole.output(cChannelUI, "US:: x=%f, y=%f, mScrollRate=%f, h=%f, h2=%f, speedPct=%f, maxSpeed=%f.", x, y, mScrollRate, h, h2, speedPct, scrollMaxSpeed);
   }
   else if (mScrollPercent != 0.0f)
   {
      mScrollPercent=0.0f;

      if (stickyReticle)
      {
         if (mScrollDirection != cOriginVector)
         {
            // Sticky reticle - Jump to unit
            // If the pointer stops before or after a unit along the movement vector, pop the camera to that unit. 
            bool doStickyJump=true;

            // Make sure we have a jump step
            float jumpStep=0.0f;
            gConfig.get(cConfigStickyReticleJumpStep, &jumpStep);
            if (jumpStep == 0.0f)
               doStickyJump = false;

            // If the scroll time is too short, the user probably just let go of the stick, so use the prev
            // scroll time and direction instead.
            float minTime=0.0f;
            gConfig.get(cConfigStickyReticleJumpMinTime, &minTime);
            if (mScrollTotalTime < minTime && mScrollPrevTotalTime > 0.0f)
            {
               mScrollTotalTime = mScrollPrevTotalTime;
               mScrollTotalDist = mScrollPrevTotalDist;
               mScrollDirection = mScrollPrevDirection;
            }

            // Don't stick if scrolling fast long enough.
            float maxSpeed=0.0f;
            gConfig.get(cConfigStickyReticleJumpMaxSpeed, &maxSpeed);
            if (maxSpeed != 0.0f && mScrollSpeedAverage > maxSpeed)
            {
               float maxTime=0.0f;
               gConfig.get(cConfigStickyReticleJumpMaxTime, &maxTime);
               if (mScrollTotalTime > maxTime)
                  doStickyJump=false;
            }

            // Don't stick if scrolling too slow
            float minSpeed=0.0f;
            gConfig.get(cConfigStickyReticleJumpMinSpeed, &minSpeed);
            if (mScrollSpeedAverage < minSpeed)
               doStickyJump=false;

            if (doStickyJump)
            {
               if (mHoverObject!=cInvalidObjectID && mHoverObject!=mScrollStartHoverObject)
               {
                  // User is pointing at an object already
                  BEntity* pEntity = gWorld->getEntity(mHoverObject);
                  if (pEntity)
                  {
//-- FIXING PREFIX BUG ID 5680
                     const BUnit* pUnit = pEntity->getUnit();
//--
                     if (getOption_CameraFollowEnabled() && mFlagHaveHoverPoint && (!pUnit || (pUnit && pUnit->getFlagAllowStickyCam())))
                     {
                        mStickyHoverObject = mHoverObject;
                        mStickyHoverPoint = mHoverPoint;
                        if (pUnit)
                           mStickyHoverObjectPos = pUnit->getInterpolatedPosition();
                        else
                           mStickyHoverObjectPos = pEntity->getPosition();
                        mFlagStickyReticleDoFollow=true;
                        mStickyReticleJumpDist = pEntity->getObstructionRadius();
                        if (pUnit)
                        {
//-- FIXING PREFIX BUG ID 5678
                           const BSquad* pSquad = pUnit->getParentSquad();
//--
                           if (pSquad)
                              mStickyReticleJumpDist = pSquad->getObstructionRadius();
                        }
                     }

                     if (gConfig.isDefined(cConfigDebugStickyReticle))
                     {
                        if (pUnit)
                           gpDebugPrimitives->addDebugSphere(pUnit->getVisualCenter(), pUnit->getVisualRadius(), cDWORDRed, BDebugPrimitives::cCategoryControls, 100.0f);
                        else
                           gpDebugPrimitives->addDebugSphere(pEntity->getPosition(), 4.0f, cDWORDRed, BDebugPrimitives::cCategoryControls, 100.0f);
                     }
                  }
               }
               else
               {
                  // Look for an object to jump to
                  BVector saveCameraPos=mpCamera->getCameraLoc();

                  BEntityID saveHoverObject=mHoverObject;
                  BEntityID startHoverObject=mScrollStartHoverObject;
                  BUnit* pStartHoverUnit=gWorld->getUnit(startHoverObject);
//-- FIXING PREFIX BUG ID 5681
                  const BSquad* pStartHoverSquad=(pStartHoverUnit?pStartHoverUnit->getParentSquad():NULL);
//--

                  BEntityID jumpFwdHoverObject=cInvalidObjectID, jumpBackHoverObject=cInvalidObjectID;
                  bool jumpFwdHaveHoverPoint=false, jumpBackHaveHoverPoint=false;
                  BVector jumpFwdHoverPoint=cOriginVector, jumpBackHoverPoint=cOriginVector;
                  int jumpFwdHoverType=0, jumpBackHoverType=0;
                  uint jumpFwdHoverTypeAttackRating=0, jumpBackHoverTypeAttackRating=0;
                  long jumpFwdHoverResource=-1, jumpBackHoverResource=-1;
                  long jumpFwdHoverHitZoneIndex=-1, jumpBackHoverHitZoneIndex=-1;
                  int jumpFwdHoverPriority=-1, jumpBackHoverPriority=-1;
                  bool jumpFwdIsStartObject=false, jumpBackIsStartObject=false;
                  float jumpFwdDist=0.0f, jumpBackDist=0.0f;
                  float jumpFwdAmount=0.0f, jumpBackAmount=0.0f;

                  //BVector jumpToCameraPos=saveCameraPos;

                  if (gConfig.isDefined(cConfigDebugStickyReticle))
                  {
                     if (pStartHoverUnit)
                        gpDebugPrimitives->addDebugSphere(pStartHoverUnit->getVisualCenter(), pStartHoverUnit->getVisualRadius(), cDWORDOrange, BDebugPrimitives::cCategoryControls, 100.0f);
                  }

                  // Look for objects to select both forward and backward along the scroll vector.
                  // If nothing is found, then search using a larger reticle.
                  bool extendedSearch = false;
                  for (uint j=0; j<4; j++)
                  {
                     if (j==2)
                     {
                        if (jumpFwdHoverObject!=cInvalidObjectID || jumpBackHoverObject!=cInvalidObjectID)
                           break;
                        extendedSearch = true;
                     }

                     bool fwd=(j==0 || j==2);

                     float jumpMinSpeed=0.0f, jumpMaxSpeed=0.0f, jumpMinDist=0.0f, jumpMaxDist=0.0f;
                     gConfig.get(fwd ? cConfigStickyReticleJumpForwardMinSpeed : cConfigStickyReticleJumpBackMinSpeed, &jumpMinSpeed);
                     gConfig.get(fwd ? cConfigStickyReticleJumpForwardMaxSpeed : cConfigStickyReticleJumpBackMaxSpeed, &jumpMaxSpeed);
                     gConfig.get(fwd ? cConfigStickyReticleJumpForwardMinDist : cConfigStickyReticleJumpBackMinDist, &jumpMinDist);
                     gConfig.get(fwd ? cConfigStickyReticleJumpForwardMaxDist : cConfigStickyReticleJumpBackMaxDist, &jumpMaxDist);
                     
                     float stickyReticleSensitivity = getOption_StickyCrosshairSensitivity() / 100.0f;
                     jumpMinDist*=stickyReticleSensitivity;
                     jumpMaxDist*=stickyReticleSensitivity;

                     float searchScale=1.0f;
                     if (j >= 2)
                     {
                        float minSearchScale=1.0f;
                        float maxSearchScale=1.0f;
                        gConfig.get(cConfigStickyReticleJumpMinSearchScale, &minSearchScale);
                        gConfig.get(cConfigStickyReticleJumpMaxSearchScale, &maxSearchScale);
                        minSearchScale*=stickyReticleSensitivity;
                        maxSearchScale*=stickyReticleSensitivity;
                        if (minSearchScale<1.0f)
                           minSearchScale=1.0f;
                        if (maxSearchScale<1.0f)
                           maxSearchScale=1.0f;
                        if (minSearchScale <= 1.0f && maxSearchScale <= 1.0f)
                           break;
                        if (mScrollSpeedAverage<jumpMinSpeed)
                           searchScale=minSearchScale;
                        else if (mScrollSpeedAverage>jumpMaxSpeed)
                           searchScale=maxSearchScale;
                        else
                        {
                           float speedRange=jumpMaxSpeed-jumpMinSpeed;
                           float pct=(mScrollSpeedAverage-jumpMinSpeed)/speedRange;
                           float scaleRange=maxSearchScale-minSearchScale;
                           searchScale=minSearchScale+(pct*scaleRange);
                        }
                     }

                     float jumpDist=0.0f;
                     if (mScrollSpeedAverage<jumpMinSpeed)
                        jumpDist=jumpMinDist;
                     else if (mScrollSpeedAverage>jumpMaxSpeed)
                        jumpDist=jumpMaxDist;
                     else
                     {
                        float speedRange=jumpMaxSpeed-jumpMinSpeed;
                        float pct=(mScrollSpeedAverage-jumpMinSpeed)/speedRange;
                        float distRange=jumpMaxDist-jumpMinDist;
                        jumpDist=jumpMinDist+(pct*distRange);
                     }
                     if (!fwd && jumpDist>mScrollTotalDist-jumpStep)
                     {
                        // Don't jump back past our starting point.
                        jumpDist=mScrollTotalDist-jumpStep;
                     }
                     if (jumpDist>0.0f)
                     {
                        float jumpAmount=0.0f;
                        BVector jumpDir = mScrollDirection * (fwd ? 1.0f : -1.0f);

                        DWORD color=(fwd ? cDWORDGreen : cDWORDBlue);
                        if (gConfig.isDefined(cConfigDebugStickyReticle))
                        {
                           BVector ip1, ip2;
                           if (gTerrainSimRep.rayIntersects(saveCameraPos, mpCamera->getCameraDir(), ip1) && gTerrainSimRep.rayIntersects(saveCameraPos+(jumpDir*jumpDist), mpCamera->getCameraDir(), ip2))
                              gTerrainSimRep.addDebugLineOverTerrain(ip1, ip2, color, color, 0.5f, BDebugPrimitives::cCategoryControls, 100.0f);
                        }

                        bool done=false;
                        while (!done)
                        {
                           jumpAmount+=jumpStep;
                           if (jumpAmount>=jumpDist)
                           {
                              jumpAmount=jumpDist;
                              done=true;
                           }
                           BVector newCameraPos=saveCameraPos+(jumpAmount*jumpDir);
                           mpCamera->setCameraLoc(newCameraPos);
                           setFlagUpdateHoverPoint(true);
                           updateHoverPoint(searchScale, 1.0f, true);
                           if (mHoverObject!=cInvalidObjectID)
                           {
                              BEntity* pEntity = gWorld->getEntity(mHoverObject);
                              BUnit* pUnit = pEntity->getUnit();
                              if (pEntity)
                              {
//-- FIXING PREFIX BUG ID 5679
                                 const BSquad* pSquad=(pUnit ? pUnit->getParentSquad() : pEntity->getSquad());
//--
                                 bool isStartObject = (mHoverObject==startHoverObject || pSquad==pStartHoverSquad);
                                 if (isStartObject)
                                    continue;
                                 if (fwd)
                                 {
                                    int priority=getHoverObjectPriority(mHoverObject, mHoverType);;
                                    if (jumpFwdHoverObject==cInvalidObjectID || priority<jumpFwdHoverPriority || (!isStartObject && jumpFwdIsStartObject))
                                    {
                                       jumpFwdHoverObject=mHoverObject;
                                       jumpFwdHaveHoverPoint=mFlagHaveHoverPoint;
                                       jumpFwdHoverPoint=mHoverPoint;
                                       jumpFwdHoverType=mHoverType;
                                       jumpFwdHoverTypeAttackRating=mHoverTypeAttackRating;
                                       jumpFwdHoverResource=mHoverResource;
                                       jumpFwdHoverHitZoneIndex=mHoverHitZoneIndex;
                                       jumpFwdDist=jumpDist;
                                       jumpFwdAmount=jumpAmount;
                                       jumpFwdHoverPriority=priority;
                                       jumpFwdIsStartObject=isStartObject;
                                    }
                                 }
                                 else
                                 {
                                    int priority=getHoverObjectPriority(mHoverObject, mHoverType);
                                    if (jumpBackHoverObject==cInvalidObjectID || priority<jumpBackHoverPriority || (!isStartObject && jumpBackIsStartObject))
                                    {
                                       jumpBackHoverObject=mHoverObject;
                                       jumpBackHaveHoverPoint=mFlagHaveHoverPoint;
                                       jumpBackHoverPoint=mHoverPoint;
                                       jumpBackHoverType=mHoverType;
                                       jumpBackHoverTypeAttackRating=mHoverTypeAttackRating;
                                       jumpBackHoverResource=mHoverResource;
                                       jumpBackHoverHitZoneIndex=mHoverHitZoneIndex;
                                       jumpBackDist=jumpDist;
                                       jumpBackAmount=jumpAmount;
                                       jumpBackHoverPriority=priority;
                                       jumpBackIsStartObject=isStartObject;
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }

                  if (jumpFwdHoverObject!=cInvalidObjectID || jumpBackHoverObject!=cInvalidObjectID)
                  {
                     BEntityID jumpToObject=cInvalidObjectID;
                     int jumpToPriority=-1;
                     if (jumpFwdHoverObject==cInvalidObjectID)
                     {
                        jumpToObject=jumpBackHoverObject;
                        jumpToPriority=jumpBackHoverPriority;
                     }
                     else if (jumpBackHoverObject==cInvalidObjectID)
                     {
                        jumpToObject=jumpFwdHoverObject;
                        jumpToPriority=jumpFwdHoverPriority;
                     }
                     else
                     {
                        if (jumpFwdHoverPriority < jumpBackHoverPriority)
                        {
                           jumpToObject=jumpFwdHoverObject;
                           jumpToPriority=jumpFwdHoverPriority;
                        }
                        else if (jumpBackHoverPriority < jumpFwdHoverPriority)
                        {
                           jumpToObject=jumpBackHoverObject;
                           jumpToPriority=jumpBackHoverPriority;
                        }
                        else
                        {
                           if (jumpFwdAmount < jumpBackAmount)
                           {
                              jumpToObject=jumpFwdHoverObject;
                              jumpToPriority=jumpFwdHoverPriority;
                           }
                           else
                           {
                              jumpToObject=jumpBackHoverObject;
                              jumpToPriority=jumpBackHoverPriority;
                           }
                        }
                     }

                     if (jumpToObject!=cInvalidObjectID)
                     {
                        BEntity* pEntity =gWorld->getEntity(jumpToObject);
                        if (pEntity)
                        {
                           BUnit* pUnit = pEntity->getUnit();

                           if (pUnit && pUnit->getFlagAllowStickyCam())
                           {
                              if (extendedSearch)
                                 mStickyHoverCameraPos = pEntity->getPosition() - (mpCamera->getCameraDir() * mCameraZoom);
                              else
                                 mStickyHoverCameraPos = (jumpToObject==jumpFwdHoverObject ? saveCameraPos+(jumpFwdAmount*mScrollDirection) : saveCameraPos+(-jumpBackAmount*mScrollDirection));
                              mStickyHoverObject = jumpToObject;
                              mStickyHoverObjectPos = pUnit->getInterpolatedPosition();
                              bool haveHoverPoint=false;
                              if (jumpToObject==jumpFwdHoverObject)
                              {
                                 haveHoverPoint=jumpFwdHaveHoverPoint;
                                 mStickyHoverPoint=jumpFwdHoverPoint;
                                 mStickyHoverType=jumpFwdHoverType;
                                 mStickyHoverTypeAttackRating=jumpFwdHoverTypeAttackRating;
                                 mStickyHoverResource=jumpFwdHoverResource;
                                 mStickyHoverHitZoneIndex=jumpFwdHoverHitZoneIndex;
                                 mStickyReticleJumpDist=jumpFwdDist;
                              }
                              else
                              {
                                 haveHoverPoint=jumpBackHaveHoverPoint;
                                 mStickyHoverPoint=jumpBackHoverPoint;
                                 mStickyHoverType=jumpBackHoverType;
                                 mStickyHoverTypeAttackRating=jumpBackHoverTypeAttackRating;
                                 mStickyHoverResource=jumpBackHoverResource;
                                 mStickyHoverHitZoneIndex=jumpBackHoverHitZoneIndex;
                                 mStickyReticleJumpDist=jumpBackDist;
                              }
                              if (mFlagStickyReticleJumpCamera)
                              {
                                 float jumpSpeed=0.0f;
                                 gConfig.get(cConfigStickyReticleJumpCameraSpeed, &jumpSpeed);
                                 if (jumpSpeed != 0.0f)
                                    mFlagStickyReticleDoJump=true;
                              }
                              if (getOption_CameraFollowEnabled() && haveHoverPoint)
                                 mFlagStickyReticleDoFollow=true;
                              if (gConfig.isDefined(cConfigDebugStickyReticle))
                              {
                                 if (pUnit)
                                    gpDebugPrimitives->addDebugSphere(pUnit->getVisualCenter(), pUnit->getVisualRadius()*1.25f, cDWORDWhite, BDebugPrimitives::cCategoryControls, 100.0f);
                                 else
                                    gpDebugPrimitives->addDebugSphere(pEntity->getPosition(), 5.0f, cDWORDWhite, BDebugPrimitives::cCategoryControls, 100.0f);
                              }
                           }
                        }
                     }
                  }

                  //mpCamera->setCameraLoc(jumpToCameraPos);
                  mpCamera->setCameraLoc(saveCameraPos);
                  setFlagUpdateHoverPoint(true);
                  updateHoverPoint(1.0f, 1.0f, true);

                  if (mHoverObject != cInvalidObjectID && mHoverObject != saveHoverObject)
                     gUI.playRumbleEvent(BRumbleEvent::cTypeUnitHover);
               }
            }
         }

         mScrollSpeedSampleIndex=0;
         mScrollSpeedSampleCount=0;
         mScrollSpeedAverage=0.0f;

         mScrollStartHoverObject=cInvalidObjectID;
         mFlagScrollStartSaved=false;

         mScrollTotalTime=0.0f;
      }
   }
}

//==============================================================================
// BUser::getHoverObjectPriority
//==============================================================================
BUserAchievementList* BUser::getAchievementList()
{ 
   if( mpProfile )
      return mpProfile->getAchievementList();

   return NULL;
}

const BUserAchievementList* BUser::getAchievementList() const
{ 
   if( mpProfile )
      return mpProfile->getAchievementList();

   return NULL;
}

//==============================================================================
// BUser::getHoverObjectPriority
//==============================================================================
int BUser::getHoverObjectPriority(BEntityID hoverObject, int hoverType) const
{
   if (hoverType==cHoverTypeEnemy)
      return 10;
   else if (hoverType>=cHoverTypeGather && hoverType<=cHoverTypeHitch)
      return 20;
   else if (hoverType==cHoverTypeAbility)
      return 30;
   else if (hoverType==cHoverTypeSelect)
      return 40;
   else
      return 50;
}

//==============================================================================
// BUser::shakeCamera
//==============================================================================
void BUser::shakeCamera(void)
{
   mpCamera->checkCameraShake();
}

//==============================================================================
// BUser::clampCamera
//==============================================================================
bool BUser::clampCamera(void)
{
   // Compute new camera position
   //
   BVector pos=mCameraHoverPoint-(mpCamera->getCameraDir()*mCameraZoom);
   if (!(gRecordGame.isPlaying() && gRecordGame.isViewLocked()))
      mpCamera->setCameraLoc(pos);

   setFlagUpdateHoverPoint(true);
   updateHoverPoint();

   if(getFlagNoCameraLimits() || (gRecordGame.isPlaying() && gRecordGame.isViewLocked()))
   {
      // Save last camera params
      mLastCameraLoc = mpCamera->getCameraLoc();
      mLastCameraHoverPoint = mCameraHoverPoint;
      mFlagHaveLastHoverPoint = mFlagHaveHoverPoint;
      return false;
   }

#ifndef ENABLE_CAMERA_BOUNDARY_LINES

   if (!mFlagHaveHoverPoint)
   {
      mpCamera->setCameraLoc(mLastCameraLoc);
      setFlagUpdateHoverPoint(true);
      saveLastCameraLoc();
      return true;
   }
   else
   {
      float buffer = 8.0f;
      float worldMinX = gWorld->getSimBoundsMinX() + buffer;
      float worldMinZ = gWorld->getSimBoundsMinZ() + buffer;
      float worldMaxX = gWorld->getSimBoundsMaxX() - buffer;
      float worldMaxZ = gWorld->getSimBoundsMaxZ() - buffer;
      if (mCameraHoverPoint.x < worldMinX || mCameraHoverPoint.z < worldMinZ || mCameraHoverPoint.x > worldMaxX || mCameraHoverPoint.z > worldMaxZ)
      {
         BVector newHoverPos = mCameraHoverPoint;
         if (newHoverPos.x < worldMinX)
            newHoverPos.x = worldMinX;
         else if (newHoverPos.x > worldMaxX)
            newHoverPos.x = worldMaxX;
         if (newHoverPos.z < worldMinZ)
            newHoverPos.z = worldMinZ;
         else if (newHoverPos.z > worldMaxZ)
            newHoverPos.z = worldMaxZ;
         BVector diff = newHoverPos - mLastCameraHoverPoint;
         BVector newCameraLoc = mLastCameraLoc + diff;
         mpCamera->setCameraLoc(newCameraLoc);
         setFlagUpdateHoverPoint(true);
         updateHoverPoint();
         tieToCameraRep();
         saveLastCameraLoc();
         return true;
      }

   }
   saveLastCameraLoc();
   return false;

#else

   bool bClampped = false;

   // Adjust first line to be the same as the map playable bounds, this can
   // change at anytime in the sim.
   //
   float buffer = 8.0f;
   float worldMinX = gWorld->getSimBoundsMinX() + buffer;
   float worldMinZ = gWorld->getSimBoundsMinZ() + buffer;
   float worldMaxX = gWorld->getSimBoundsMaxX() - buffer;
   float worldMaxZ = gWorld->getSimBoundsMaxZ() - buffer;

   // Allow the hover point to re-enter if it ever finds itself out of bounds
   bool lastHoverPointOutOfBounds = ((mLastCameraHoverPoint.x < worldMinX) || (mLastCameraHoverPoint.x > worldMaxX) || (mLastCameraHoverPoint.z < worldMinZ) || (mLastCameraHoverPoint.z > worldMaxZ));
   if (lastHoverPointOutOfBounds)
   {
      // Clamp to world instead, or whichever is bigger
      float worldSize = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale();
      worldMinX = Math::Min(worldMinX, buffer);
      worldMinZ = Math::Min(worldMinZ, buffer);
      worldMaxX = Math::Max(worldMaxX, worldSize - buffer);
      worldMaxZ = Math::Max(worldMaxZ, worldSize - buffer);

      // If for some reason we're off world, then skip bounds clamping altogether
      lastHoverPointOutOfBounds = ((mLastCameraHoverPoint.x < worldMinX) || (mLastCameraHoverPoint.x > worldMaxX) || (mLastCameraHoverPoint.z < worldMinZ) || (mLastCameraHoverPoint.z > worldMaxZ));
      if (lastHoverPointOutOfBounds)
      {
         BFAIL("BUser::clampCamera - The camera is off the map!");
         worldMinX = worldMinZ = -cMaximumFloat;
         worldMaxX = worldMaxZ = cMaximumFloat;
      }
   }

   BCameraBoundaryLine *borderLine = &mCameraBoundaryLines[0];
   BASSERT(borderLine->mPoints.getNumber() == 5);

   borderLine->mPoints.setAt(0, BVector(worldMinX, 0.0f, worldMinZ));
   borderLine->mPoints.setAt(1, BVector(worldMinX, 0.0f, worldMaxZ));
   borderLine->mPoints.setAt(2, BVector(worldMaxX, 0.0f, worldMaxZ));
   borderLine->mPoints.setAt(3, BVector(worldMaxX, 0.0f, worldMinZ));
   borderLine->mPoints.setAt(4, BVector(worldMinX, 0.0f, worldMinZ));

   if(!getFlagTeleportCamera())
   {
      // Keep the camera hover point and the camera position constrained to their corresponding bounds,
      // it's important to pass in both last and current values for the camera hover point and the
      // camera pos, since the camera my have not only translated but also rotated.
      //
      BVector newHoverPointPos;
      BVector newCameraPos;
      bool bNoSolution;

      if(resolvePositionConstraints2D(mLastCameraLoc, mLastCameraHoverPoint, mpCamera->getCameraLoc(), mCameraHoverPoint, newCameraPos, newHoverPointPos, bNoSolution))
      {
         if(!bNoSolution)
         {
            // Adjust y values of the new returned positions since the constraint was done in 2D
            //

            // Plant postion in camera height field
            computeClosestCameraHoverPointVertical(newHoverPointPos, newHoverPointPos);

            BVector diff = mpCamera->getCameraLoc() - mCameraHoverPoint;
            newCameraPos.y = newHoverPointPos.y + diff.y;

            mCameraHoverPoint = newHoverPointPos;
            mpCamera->setCameraLoc(newCameraPos);
         }
         else
         {
            mCameraHoverPoint = mLastCameraHoverPoint;

            BVector oldCamDir = mLastCameraHoverPoint - mLastCameraLoc;
            oldCamDir.normalize();

            mpCamera->setCameraLoc(mLastCameraLoc);
            mpCamera->setCameraDir(oldCamDir);
            mpCamera->setCameraUp(cYAxisVector);
            mpCamera->calcCameraRight();

            BVector up = mpCamera->getCameraDir().cross(mpCamera->getCameraRight());
            up.normalize();
            mpCamera->setCameraUp(up);

            mCameraZoom = mLastCameraHoverPoint.distance(mLastCameraLoc);
         }

         // Compute new hover point
         //
         setFlagUpdateHoverPoint(true);
         updateHoverPoint();

         // Set clamp bool
         bClampped = true;
      }
   }
   else
   {
      // We have teleported the hover point, make sure that the translation between the previous and
      // current camera positions and camera hover points do not exit the bounds.  It is allow for 
      // these offset vectors to cross an even number of boundary lines, meaning that the position has
      // exited the bounds but then reenter, ending a valid position.
      //

      // Reset teleport flag
      setFlagTeleportCamera(false);

      BVector newHoverPointPos;
      BVector newCameraPos;
      bool bNoSolution;

      if(resolvePositionTeleportConstraints2D(mLastCameraLoc, mLastCameraHoverPoint, mpCamera->getCameraLoc(), mCameraHoverPoint, newCameraPos, newHoverPointPos, bNoSolution))
      {
         if(!bNoSolution)
         {
            // Adjust y values of the new returned positions since the constraint was done in 2D
            //

            // Plant postion in camera height field
            computeClosestCameraHoverPointVertical(newHoverPointPos, newHoverPointPos);

            BVector diff = mpCamera->getCameraLoc() - mCameraHoverPoint;
            newCameraPos.y = newHoverPointPos.y + diff.y;

            mCameraHoverPoint = newHoverPointPos;
            mpCamera->setCameraLoc(newCameraPos);
         }
         else
         {
            mCameraHoverPoint = mLastCameraHoverPoint;

            BVector oldCamDir = mLastCameraHoverPoint - mLastCameraLoc;
            oldCamDir.normalize();

            mpCamera->setCameraLoc(mLastCameraLoc);
            mpCamera->setCameraDir(oldCamDir);
            mpCamera->setCameraUp(cYAxisVector);
            mpCamera->calcCameraRight();

            BVector up = mpCamera->getCameraDir().cross(mpCamera->getCameraRight());
            up.normalize();
            mpCamera->setCameraUp(up);

            mCameraZoom = mLastCameraHoverPoint.distance(mLastCameraLoc);
         }

         // Compute new hover point
         //
         setFlagUpdateHoverPoint(true);
         updateHoverPoint();

         // Set clamp bool
         bClampped = true;
      }
   }

   // Save last camera params
   mLastCameraLoc = mpCamera->getCameraLoc();
   mLastCameraHoverPoint = mCameraHoverPoint;
   mFlagHaveLastHoverPoint = mFlagHaveHoverPoint;

   return bClampped;

#endif
}

//==============================================================================
//==============================================================================
void BUser::updateMinigame()
{
   if (!mFlagUIPowerMinigame)
      return;

   // Update
   BMinigame::eState state = mMinigame.update(this);

   // Minigame complete
   if (state == BMinigame::cComplete)
   {
      BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand( mPlayerID, cCommandTrigger );
      if( !c )
      {
         return;
      }
    
      // Set up the command.
      c->setSenders( 1, &mPlayerID );
      c->setSenderType( BCommand::cPlayer );
      c->setRecipientType( BCommand::cPlayer );
      c->setType( BTriggerCommand::cTypeBroadcastInputUILocationMinigameResult );
      // Register the correct trigger system and variable this stuff goes into.
      c->setTriggerScriptID( mTriggerScriptID );
      c->setTriggerVarID( mTriggerVarID );
      // Set the data that will be poked in.
      c->setInputResult( BTriggerVarUILocation::cUILocationResultOK );
      c->setInputLocation( mHoverPoint );
      c->setInputQuality(mMinigame.getResult());

      // Ok rock on.
      gWorld->getCommandManager()->addCommandToExecute( c );
      changeMode( cUserModeNormal );         
   }
}

//==============================================================================
// BUser::updateFreeCamera
//==============================================================================
void BUser::updateFreeCamera(float elapsedTime)
{
   if (mFreeCameraReset)
   {
      mFreeCameraReset = false;
      
      mFreeCameraCurRot.clear();
   }

   BMatrix44 x(BMatrix44::makeRotate(0, mFreeCameraCurRot[0]));
   BMatrix44 y(BMatrix44::makeRotate(1, mFreeCameraCurRot[1]));
   BMatrix44 z(BMatrix44::makeRotate(2, mFreeCameraCurRot[2]));
   BMatrix44 t(BMatrix44::makeTranslate(BVec4(-mFreeCameraCurLoc, 1.0f)));
      
   BMatrix44 worldToView(t * y * x * z);
   
   mpFreeCamera->setCameraDir(BVector(worldToView.getColumn(2)[0], worldToView.getColumn(2)[1], worldToView.getColumn(2)[2]));
   mpFreeCamera->setCameraUp(BVector(worldToView.getColumn(1)[0], worldToView.getColumn(1)[1], worldToView.getColumn(1)[2]));
   mpFreeCamera->calcCameraRight();
   
   mpFreeCamera->setCameraLoc(BVector(mFreeCameraCurLoc[0], mFreeCameraCurLoc[1], mFreeCameraCurLoc[2]));
         
   mFreeCameraCurRot += mFreeCameraRotation * cRadiansPerDegree * elapsedTime;
      
   BMatrix44 viewToWorld(worldToView);
   viewToWorld.invert();
   
   BVec4 worldMovement(BVec4(mFreeCameraMovement, 0.0f) * viewToWorld);
   
   mFreeCameraCurLoc += worldMovement * elapsedTime;
}

//==============================================================================
// BUser::updateCamera
//==============================================================================
void BUser::updateCamera(float elapsedTime)
{
   if (gRecordGame.isPlaying() && gRecordGame.isViewLocked())
      return;

#ifndef BUILD_FINAL
   if (gModeManager.getModeGame()->getFlag(BModeGame::cFlagRandomCameraTest))
   {
      float maxT = gTerrainSimRep.getNumXDataTiles()*gTerrainSimRep.getDataTileScale();
      
      mpCamera->setCameraLoc(BVector(Math::fRand(0.0f, maxT), Math::fRand(0.0f, 250.0f), Math::fRand(0.0f, maxT)));
   }
#endif   
   if (getFlagFreeCamera())
   {
      updateFreeCamera(elapsedTime);
      return;
   }

   if (mInterpTimeLeft > 0.0f)
   {
      updateInterpCamera(elapsedTime);
      return;
   }

   if (mGotoItemTime > 0.0f)
   {
      updateGotoCamera(elapsedTime);
      return;
   }

   // Sticky reticle
   if (getOption_StickyCrosshairSensitivity() > 0)
   {
      if (mUserMode==cUserModeNormal || mUserMode==cUserModeInputUISquad || mUserMode==cUserModeInputUIUnit)
      {
         // See if the stick hover object is no longer visible or is out of bounds
//-- FIXING PREFIX BUG ID 5687
         const BEntity* pEntity = gWorld->getEntity(mStickyHoverObject);
//--
         if (mStickyHoverObject!=cInvalidObjectID)
         {
            if (!pEntity || !pEntity->isVisible(mTeamID) || pEntity->isOutsidePlayableBounds(true) || !pEntity->isAlive())
            {
               resetStickyReticle();
               pEntity=NULL;
            }
            
            // If the allow sticky cam flag has been unset, clear out the sticky hover object
            if (pEntity)
            {
               const BUnit* pUnit = pEntity->getUnit();
               if (pUnit && !pUnit->getFlagAllowStickyCam())
               {
                  resetStickyReticle();
                  pEntity=NULL;
               }
            }
         }

         if (!mFlagStickyReticleDoFollow && !mFlagStickyReticleDoJump && pEntity)
         {
            // See if the sticky hover object is too far from it's original position
            BVector objectPos;
            const BObject *obj = pEntity->getObject();
            if (obj)
               objectPos = obj->getInterpolatedPosition();
            else
               objectPos = pEntity->getPosition();

            BVector posDiff = objectPos - mStickyHoverObjectPos;
            float diffLen = posDiff.length();
            if (diffLen > mStickyReticleJumpDist)
               resetStickyReticle();
         }
         
         if (mFlagStickyReticleDoFollow && pEntity)
         {
            // Follow the unit we stopped on
            BVector objectPos;
            const BObject *obj = pEntity->getObject();
            if (obj)
               objectPos = obj->getInterpolatedPosition();
            else
               objectPos = pEntity->getPosition();

            BVector posDiff = objectPos - mStickyHoverObjectPos;
            float diffLen = posDiff.length();
            if (diffLen != 0.0f)
            {
               mStickyHoverObjectPos = objectPos;
               /*
               mStickyHoverPoint += posDiff;
               float camRepHeight = 0.0f;
               gTerrainSimRep.getCameraHeightRaycast(mStickyHoverPoint, camRepHeight, true);
               BVector intersectionPt = cInvalidVector;
               BVector testDir = mpCamera->getCameraDir();
               if (camRepHeight > mStickyHoverPoint.y)
               {
                  testDir = -testDir;
               }
               gTerrainSimRep.rayIntersectsCamera(mStickyHoverPoint, testDir, intersectionPt);
               */

               BVector intersectionPt;
               computeClosestCameraHoverPointInDirection(objectPos, mpCamera->getCameraDir(), intersectionPt);
               mStickyHoverPoint = intersectionPt;


               mStickyHoverCameraPos = mStickyHoverPoint - (mpCamera->getCameraDir() * mCameraZoom);
               if (!mFlagStickyReticleDoJump)
               {
                  mCameraHoverPoint = mStickyHoverPoint;
                  mpCamera->setCameraLoc(mStickyHoverCameraPos);                  

                  setFlagUpdateHoverPoint(true);
                  //bool retval=updateHoverPoint();
                  updateHoverPoint(1.0f, 1.0f, true);
                  /*
                  if (mFlagHaveHoverPoint)
                  {
                     BVector pos=mCameraHoverPoint-(mpCamera->getCameraDir()*mCameraZoom);
                     mpCamera->setCameraLoc(pos);
                  }
                  */
                  if (clampCamera())
                     mFlagStickyReticleDoFollow=false;
               }
            }
         }

         if (mFlagStickyReticleDoJump)
         {
            // Jump to unit
            BVector pos=mpCamera->getCameraLoc();
            BVector vec=mStickyHoverCameraPos-pos;
            float dist=vec.length();
            float jumpSpeed=0.0f;
            gConfig.get(cConfigStickyReticleJumpCameraSpeed, &jumpSpeed);
            float jumpDist = jumpSpeed * elapsedTime;
            if (jumpSpeed == 0.0f || dist <= jumpDist)
            {
               mpCamera->setCameraLoc(mStickyHoverCameraPos);
               mFlagStickyReticleDoJump=false;
            }
            else
            {
               vec.normalize();
               pos += vec * jumpDist;
               mpCamera->setCameraLoc(pos);
            }
            setFlagUpdateHoverPoint(true);
            bool retval=updateHoverPoint(1.0f, 1.0f, true);
            /*
            if (mFlagHaveHoverPoint)
            {
               BVector pos=mCameraHoverPoint-(mpCamera->getCameraDir()*mCameraZoom);
               mpCamera->setCameraLoc(pos);
            }
            */
            if (clampCamera())
               mFlagStickyReticleDoJump=false;
            if (retval && mHoverObject==mStickyHoverObject)
            {
               mStickyHoverCameraPos=mpCamera->getCameraLoc();
               mFlagStickyReticleDoJump=false;
            }
         }
      }
   }

   if(mCameraAdjustZoom!=0.0f)
   {
      bool skipAdjustment=false;
      if(gConfig.isDefined(cConfigDebugAttachmentRotation))
      {
         BUnit* pUnit=gWorld->getUnit(mHoverObject);
         if(pUnit)
         {
            BVisual* pVisual=pUnit->getVisual();
            if(pVisual)
            {
               long handle=pVisual->getAttachmentHandle("Cannon");
               if(handle!=-1)
               {
                  BMatrix matrix;
                  if(pVisual->getAttachmentTransform(handle, matrix))
                  {
                     matrix.multRotateX(mCameraAdjustZoom*elapsedTime*100.0f*cRadiansPerDegree);
                     pVisual->setAttachmentTransform(handle, matrix);
                     skipAdjustment=true;
                  }
               }
            }
         }
      }
      if(!skipAdjustment)
      {    
         // Manual Tilt
         //BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
         //if (getFlagNoCameraLimits() && getFlagModifierAction() == pInputInterface->usesActionModifier(BInputInterface::cInputTilt))
         if (getFlagNoCameraLimits() && gInputSystem.getGamepad(mPort).isControlActive(cTriggerRight))
         {
            #if defined( _VINCE_ )
               DWORD gameTime = gWorld->getGametime();
               if( mVinceTimeTilt <= gameTime )
               {
                  MVinceEventAsync_ControlUsed( this, "tilt" );
                  mVinceTimeTilt = gameTime + VINCE_TIME_DELAY;
               }
            #endif
            gGeneralEventManager.eventTrigger(BEventDefinitions::cControlTilt, mPlayerID);

            float pitchRate=60.0f;
            gConfig.get(cConfigCameraPitchRate, &pitchRate);
            float dist=mCameraAdjustZoom*elapsedTime*pitchRate;
            if(getFlagModifierSpeed())
            {
               float pitchFastFactor=1.0f;
               gConfig.get(cConfigCameraPitchFastFactor, &pitchFastFactor);
               dist*=pitchFastFactor;
            }
            if(getOption_YInverted())
            {
               dist = -dist;
            }
            if(!getFlagNoCameraLimits())
            {
               float newDist=mCameraPitch+dist;
               if(newDist<mCameraPitchMin)
                  dist=mCameraPitchMin-mCameraPitch;
               else if(newDist>mCameraPitchMax)
                  dist=mCameraPitchMax-mCameraPitch;
            }
            if(getFlagHaveHoverPoint())
            {
               mpCamera->pitchWorldAbout(dist*cRadiansPerDegree, mCameraHoverPoint);
               mCameraPitch+=dist;
            }
            else
            {
               mpCamera->pitch(dist*cRadiansPerDegree);
               mCameraPitch+=dist;
            }
            if(mCameraPitch>=360.0f)
               mCameraPitch-=360.0f;
            else if(mCameraPitch<0.0f)
               mCameraPitch+=360.0f;
            setFlagUpdateHoverPoint(true);
         }
         // Zoom and Auto-Tilt
         else
         {
            #if defined( _VINCE_ )
               DWORD gameTime = gWorld->getGametime();
               if( mVinceTimeZoom <= gameTime )
               {
                  MVinceEventAsync_ControlUsed( this, "zoom" );
                  mVinceTimeZoom = gameTime + VINCE_TIME_DELAY;
               }
            #endif
            gGeneralEventManager.eventTrigger(BEventDefinitions::cControlZoom, mPlayerID);

            float zoomRate=80.0f;
            gConfig.get(cConfigCameraZoomRate, &zoomRate);

            float dist=mCameraAdjustZoom*elapsedTime*zoomRate;
            if(getFlagModifierSpeed())
            {
               float zoomFastFactor=2.0f;
               gConfig.get(cConfigCameraZoomFastFactor, &zoomFastFactor);
               dist*=zoomFastFactor;
            }
            if(getOption_YInverted())
            {
               dist = -dist;
            }
            if(!getFlagNoCameraLimits())
            {
               float newDist=mCameraZoom-dist;
               if(newDist<mCameraZoomMin)
                  dist=mCameraZoom-mCameraZoomMin;
               else if(newDist>mCameraZoomMax)
                  dist=-(mCameraZoomMax-mCameraZoom);
            }

            if(dist!=0.0f)
            {
               mpCamera->moveForward(dist);
               mCameraZoom-=dist;
               setFlagUpdateHoverPoint(true);

               // Auto-Tilt
               if (!getFlagNoCameraLimits() && mCameraZoomMin != mCameraZoomMax && mCameraPitchMin != mCameraPitchMax)
               {
                  float zoomRange = mCameraZoomMax - mCameraZoomMin;
                  float zoomPct = dist / zoomRange;

                  float pitchRange = mCameraPitchMax - mCameraPitchMin;
                  dist = -(zoomPct * pitchRange);

                  float newDist=mCameraPitch+dist;
                  if(newDist<mCameraPitchMin)
                     dist=mCameraPitchMin-mCameraPitch;
                  else if(newDist>mCameraPitchMax)
                     dist=mCameraPitchMax-mCameraPitch;

                  if(getFlagHaveHoverPoint())
                  {
                     mpCamera->pitchWorldAbout(dist*cRadiansPerDegree, mCameraHoverPoint);
                     mCameraPitch+=dist;
                  }
                  else
                  {
                     mpCamera->pitch(dist*cRadiansPerDegree);
                     mCameraPitch+=dist;
                  }

                  if(mCameraPitch>=360.0f)
                     mCameraPitch-=360.0f;
                  else if(mCameraPitch<0.0f)
                     mCameraPitch+=360.0f;
               }
            }
         }
      }
   }

   if (mCameraAdjustYaw!=0.0f)
   {
      bool skipAdjustment=false;
      if(gConfig.isDefined(cConfigDebugAttachmentRotation))
      {
         BUnit* pUnit=gWorld->getUnit(mHoverObject);
         if(pUnit)
         {
            BVisual* pVisual=pUnit->getVisual();
            if(pVisual)
            {
               long handle=pVisual->getAttachmentHandle("Turret");
               if(handle!=-1)
               {
                  BMatrix matrix;
                  if(pVisual->getAttachmentTransform(handle, matrix))
                  {
                     matrix.multRotateY(mCameraAdjustYaw*elapsedTime*100.0f*cRadiansPerDegree);
                     pVisual->setAttachmentTransform(handle, matrix);
                     skipAdjustment=true;
                  }
               }
            }
         }
      }
      if(!skipAdjustment)
      {
         float rotateRate=90.0f;
         gConfig.get(cConfigCameraRotateRate, &rotateRate);
         float dist=mCameraAdjustYaw*elapsedTime*rotateRate;
         if(getFlagModifierSpeed())
         {
            float rotateFastFactor=2.0f;
            gConfig.get(cConfigCameraRotateFastFactor, &rotateFastFactor);
            dist*=rotateFastFactor;
         }
         if( getOption_XInverted() )
         {
            dist = -dist;
         }
         if(getFlagHaveHoverPoint())
         {
            mpCamera->yawWorldAbout(dist*cRadiansPerDegree, mCameraHoverPoint);      
            mCameraYaw+=dist;
         }
         else
         {
            mpCamera->yawWorld(dist*cRadiansPerDegree);
            mCameraYaw+=dist;
         }
         if(mCameraYaw>=360.0f)
            mCameraYaw-=360.0f;
         else if(mCameraYaw<0.0f)
            mCameraYaw+=360.0f;
         setFlagUpdateHoverPoint(true);

         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlRotate, mPlayerID);         
      }
   }      

   if(mCameraAdjustFOV!=0.0f)
   {
      float fovRate=5.0f;
      gConfig.get(cConfigCameraFOVRate, &fovRate);
      float dist=mCameraAdjustFOV*elapsedTime*fovRate;
      mCameraFOV+=dist;
      mpCamera->setFOV(mCameraFOV*cRadiansPerDegree);
   }

   if(!getFlagNoCameraLimits())
   {
      if (mFlagCameraAutoZoomInstantEnabled)
      {
         float dist = mpCamera->getCameraLoc().distance(mCameraHoverPoint);
         float diff = dist - mCameraZoom;
         if (diff != 0.0f)
            mpCamera->moveForward(diff);
      }
      else if (mFlagCameraAutoZoomEnabled)
      {
         // Auto adjust zoom based on distance to terrain only while scrolling or pitching
         mCameraAutoZoomOutTime+=elapsedTime;
         mCameraAutoZoomInTime+=elapsedTime;

         float autoZoomPercent=mScrollPercent;
         if(mScrollPercent==0.0f)
         {
            BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
            if( ( mCameraAdjustZoom != 0.0f ) && ( getFlagModifierAction() == pInputInterface->usesActionModifier( BInputInterface::cInputTilt ) ) ) // pitch
            {
               autoZoomPercent = fabs( mCameraAdjustZoom );
            }
         }

         if (autoZoomPercent==0.0f)
         {
            if (mFlagStickyReticleDoJump)
            {
               float jumpSpeed=0.0f, scrollMaxSpeed=0.0f, scrollUserSpeedAdjustment = cDefaultScrollSpeedAdjFloat;
               gConfig.get(cConfigStickyReticleJumpCameraSpeed, &jumpSpeed);
               gConfig.get(cConfigScrollMaxSpeed, &scrollMaxSpeed);
               scrollUserSpeedAdjustment = scrollSpeedAdjByteToFloat(getOption_ScrollSpeed());
               scrollMaxSpeed *= scrollUserSpeedAdjustment;
               if (jumpSpeed != 0.0f && scrollMaxSpeed != 0.0f)
                  autoZoomPercent = jumpSpeed / scrollMaxSpeed;
            }
            else if (mFlagStickyReticleDoFollow)
            {
               BEntity* pEntity=gWorld->getEntity(mStickyHoverObject);
               if (pEntity)
               {
                  float unitSpeed=pEntity->getDesiredVelocity();
                  float scrollMaxSpeed=0.0f, scrollUserSpeedAdjustment = cDefaultScrollSpeedAdjFloat;
                  gConfig.get(cConfigScrollMaxSpeed, &scrollMaxSpeed);
                  scrollUserSpeedAdjustment = scrollSpeedAdjByteToFloat(getOption_ScrollSpeed());
                  scrollMaxSpeed *= scrollUserSpeedAdjustment;
                  if (unitSpeed != 0.0f && scrollMaxSpeed != 0.0f)
                  {
                     autoZoomPercent = unitSpeed / scrollMaxSpeed;
                     if (autoZoomPercent > 1.0f)
                        autoZoomPercent = 1.0f;
                  }
               }
            }
            else if (mUserMode==cUserModeFollow)
            {
//-- FIXING PREFIX BUG ID 5688
               const BEntity* pFollowEntity = gWorld->getEntity(mFollowEntity);
//--
               if (pFollowEntity)
               {
                  float unitSpeed=pFollowEntity->getDesiredVelocity();
                  float scrollMaxSpeed=0.0f, scrollUserSpeedAdjustment = cDefaultScrollSpeedAdjFloat;
                  gConfig.get(cConfigScrollMaxSpeed, &scrollMaxSpeed);
                  scrollUserSpeedAdjustment = scrollSpeedAdjByteToFloat(getOption_ScrollSpeed());
                  if (unitSpeed != 0.0f && scrollMaxSpeed != 0.0f)
                  {
                     autoZoomPercent = unitSpeed / scrollMaxSpeed;
                     if (autoZoomPercent > 1.0f)
                        autoZoomPercent = 1.0f;
                  }
               }
            }
         }

         if(autoZoomPercent>0.0f && getFlagHaveHoverPoint())
         {
            float dist=mpCamera->getCameraLoc().distance(mCameraHoverPoint);
            float diff=dist-mCameraZoom;
            if(diff>0.01f)
            {
               float delay=0.01f;
               gConfig.get(cConfigCameraAutoZoomInDelay, &delay);
               if(mCameraAutoZoomOutTime>delay)
               {
                  float zoomRate=40.0f;
                  gConfig.get(cConfigCameraAutoZoomInRate, &zoomRate);
                  float zoom=elapsedTime*zoomRate*autoZoomPercent;
                  if(getFlagModifierSpeed())
                  {
                     float zoomFastFactor=2.0f;
                     gConfig.get(cConfigCameraAutoZoomInFastFactor, &zoomFastFactor);
                     zoom*=zoomFastFactor;
                  }
                  if(zoom>diff)
                     zoom=diff;
                  mpCamera->moveForward(zoom);
                  mCameraAutoZoomInTime=0.0f;
               }
            }
            else if(diff<-0.01f)
            {
               float delay=0.01f;
               gConfig.get(cConfigCameraAutoZoomOutDelay, &delay);
               if(mCameraAutoZoomInTime>delay)
               {
                  float zoomRate=40.0f;
                  gConfig.get(cConfigCameraAutoZoomOutRate, &zoomRate);
                  float zoom=elapsedTime*zoomRate*autoZoomPercent;
                  if(getFlagModifierSpeed())
                  {
                     float zoomFastFactor=2.0f;
                     gConfig.get(cConfigCameraAutoZoomOutFastFactor, &zoomFastFactor);
                     zoom*=zoomFastFactor;
                  }
                  zoom*=-1.0f;
                  if(zoom<diff)
                     zoom=diff;
                  mpCamera->moveForward(zoom);
                  mCameraAutoZoomOutTime=0.0f;
               }
            }
         }
      }
   }

   gGeneralEventManager.eventTrigger(BEventDefinitions::cCameraLookingAt, mPlayerID, mpCamera);

}

//=============================================================================
// BUser::updateHoverPoint
//
// Uses the camera position matrix (mpCamera) to compute a new hover point 
// and new hover params (mHoverPoint, mHoverObject, mHoverType, etc.).  This
// function does not update the camera hover point (mCameraHoverPoint).  The 
// order of operation is the following:
//
//       mCameraHoverPoint  ->  mpCamera(loc)  ->  mHoverPoint
//
// The camera hover point drives the camera position which in turn drives
// the hover point.
//
// Returns true if found hover object without using sticky hover object
//=============================================================================
bool BUser::updateHoverPoint(float xScale, float yScale, bool noRumble)
{
   // Update the hover point
   bool lastHaveHoverPoint = mFlagHaveHoverPoint;
   bool lastHoverPointOverTerrain = mFlagHoverPointOverTerrain;
   BVector lastHoverPoint = mHoverPoint;
   if(getFlagUpdateHoverPoint())
   {
      setFlagUpdateHoverPoint(false);
      setFlagHaveHoverPoint(false);
      setFlagHoverPointOverTerrain(false);
      BVector intersectionPt;
      BVector dir(mpCamera->getCameraDir());
      bool hit=gTerrainSimRep.rayIntersects(mpCamera->getCameraLoc(), dir, intersectionPt);
      if (hit)
      {
         if (intersectionPt.x >= gWorld->getSimBoundsMinX() && intersectionPt.z >= gWorld->getSimBoundsMinZ() && 
             intersectionPt.x <  gWorld->getSimBoundsMaxX() && intersectionPt.z <  gWorld->getSimBoundsMaxZ())
         {
            setFlagHoverPointOverTerrain(true);
         }
      }
      else
      {
         BVector planePoint(0.0f, (lastHaveHoverPoint ? lastHoverPoint.y : 0.0f), 0.0f);
         hit=raySegmentIntersectionPlane(planePoint, cYAxisVector, mpCamera->getCameraLoc(), dir, false, intersectionPt, 0.01f);
      }
      if(hit)
      {
         mHoverPoint.set(intersectionPt.x, intersectionPt.y, intersectionPt.z);
         setFlagHaveHoverPoint(true);
         updateHoverLight(true);
      }
      else
         updateHoverLight(false);

      if (gConfig.isDefined(cConfigFlashGameUI))
      {  
         BDecalAttribs* pDecalAttribs = gDecalManager.getDecal(mCircleSelectDecal);
         if (pDecalAttribs)
            pDecalAttribs->setPos(BVec3(mHoverPoint.x, mHoverPoint.y, mHoverPoint.z));

         pDecalAttribs = gDecalManager.getDecal(mHoverDecal);
         if (pDecalAttribs)
            pDecalAttribs->setPos(BVec3(mHoverPoint.x, mHoverPoint.y, mHoverPoint.z));            

         pDecalAttribs = gDecalManager.getDecal(mPowerDecal);
         if (pDecalAttribs)
            pDecalAttribs->setPos(BVec3(mHoverPoint.x, mHoverPoint.y, mHoverPoint.z));            
      }
   }

   // Override hover point stuff while target selecting
   if (mFlagTargetSelecting)
   {
      mHoverPoint = lastHoverPoint;
      mFlagHaveHoverPoint = lastHaveHoverPoint;
      mFlagHoverPointOverTerrain = lastHoverPointOverTerrain;
      return (mTargetSelectObject != cInvalidObjectID);
   }

   // Do some processing whenever the set of selected units/squads changes.
   if (mLastSelectionChangeCounter != mSelectionManager->getSelectionChangeCounter())
   {
      updateSelectionChangeForHover();
      mLastSelectionChangeCounter = mSelectionManager->getSelectionChangeCounter();
   }

   BEntityID saveHoverObject = mHoverObject;

   mHoverObject = cInvalidObjectID;
   mHoverType = cHoverTypeNone;
   mHoverTypeAttackRating = cReticleAttackNoEffectAgainst;
   mHoverResource = cHoverResourceNone;
   mHoverHitZoneIndex = -1;

   setFlagCircleSelectPoints(false);

   if (getFlagHaveHoverPoint() && (mUserMode != cUserModeInputUISquadList) && (mUserMode != cUserModeInputUIPlaceSquadList))
   {
      if((mUserMode == cUserModeCircleSelecting && getFlagCircleSelectGrow()) || getFlagCircleSelectShrink())
      {
         float radius = mCircleSelectSize;
         float radiansPerSegment = cTwoPi / cNumCircleSelectPoints;
         float angularOffset = 0.0f;
         BVector dir;
         for (long segment = 0; segment < cNumCircleSelectPoints; ++segment)
         {
            BVector& pt=mCircleSelectPoints[segment];
            float sinAngle = (float)sin(angularOffset);
            float cosAngle = (float)cos(angularOffset);
            pt.set(sinAngle * radius + mHoverPoint.x, mHoverPoint.y, cosAngle * radius + mHoverPoint.z);
            dir.set(pt.x, 1000.0f, pt.z);
            if(gTerrainSimRep.getHeightRaycast(dir, pt.y, true))
               pt.y+=0.1f;
            angularOffset += radiansPerSegment;
         }
         setFlagCircleSelectPoints(true);
      }
      //if(!(mUserMode == cUserModeCircleSelecting && getFlagCircleSelectGrow()))
      {
         float clickSize=20.0f;
         gConfig.get(cConfigCircleSelectClickSize, &clickSize);
         bool targetMode=(mSelectionManager->getNumberSelectedUnits()>0 || BDisplayStats::isDisplaying());
         mSelectionManager->populateSelectionsRect(mpCamera->getCameraLoc(), mHoverPoint, clickSize*0.5f*xScale, clickSize*0.5f*yScale, false, targetMode);
         long possibleCount = mSelectionManager->getPossibleSelectionCount();

         BEntityID possibleHoverObject = cInvalidObjectID;
         int possibleHoverType = cHoverTypeNone;

         BEntityIDArray selectedSquads;
         mSelectionManager->getSubSelectedSquads(selectedSquads);
         long numSelectedSquads = selectedSquads.getNumber();
         if (possibleCount > 0)
         {
            for (long selSquadIdx=0; selSquadIdx<numSelectedSquads; selSquadIdx++)
            {
               mHoverObject = cInvalidObjectID;
               mHoverType = cHoverTypeNone;
               BSquad* pSelectedSquad = gWorld->getSquad(selectedSquads[selSquadIdx]);
               if(pSelectedSquad)
               {
                  for(long i=0; i<possibleCount; i++)
                  {
                     BEntityID possibleID=mSelectionManager->getPossibleSelection(i);
                     if (updateHoverType(pSelectedSquad, possibleID))
                     {
                        if (mHoverType != cHoverTypeNone)
                        {
                           mHoverObject = possibleID;
                           if (mHoverType == cHoverTypeEnemy)
                              break;
                           if (possibleHoverObject == cInvalidObjectID)
                           {
                              possibleHoverObject = mHoverObject;
                              possibleHoverType = mHoverType;
                           }
                        }
                     }
                  }
               }
               if (mHoverType == cHoverTypeEnemy)
                  break;
            }

            if (mHoverType == cHoverTypeNone && possibleHoverObject != cInvalidObjectID)
            {
               mHoverObject = possibleHoverObject;
               mHoverType = possibleHoverType;
            }

            // If no work on unit found, then pick the first object in the list.
            if(mHoverObject==cInvalidObjectID)
            {
               BObject* pObject=gWorld->getObject(mSelectionManager->getPossibleSelection(0));
               if(pObject)
               {
                  int selectType = pObject->getSelectType(getTeamID());
                  if ((selectType != cSelectTypeNone) && (selectType != cSelectTypeTarget) && (!pObject->getProtoObject()->getFlagMustOwnToSelect() || (!pObject->getDopple() && (pObject->getPlayerID() == mPlayerID || pObject->getPlayerID() == mCoopPlayerID))))
                  {
                     mHoverObject = pObject->getID();
                     mHoverType = cHoverTypeSelect;       
                  }
                  #ifndef BUILD_FINAL
                  else
                  {
                     if(BDisplayStats::getMode()==BDisplayStats::cDSMSim && BDisplayStats::getType(BDisplayStats::cDSMSim)==BDisplayStats::cDisplayStatObject)
                     {
                        mHoverObject=pObject->getID();
                        mHoverType=cHoverTypeSelect;
                     }
                  }
                  #endif
               }
            }
         }

         // Handle ability
         mSelectionManager->updateSubSelectAbilities(mHoverObject, mHoverPoint);
      }
   }
   else if (getFlagHaveHoverPoint() && (mUserMode == cUserModeInputUISquadList))
   {
      mSelectionManager->clearSelections();            
      BEntityIDArray results(0, 100);       
      BUnitQuery query(mHoverPoint, mUIPowerRadius, true);
      gWorld->getSquadsInArea(&query, &results);                        
      BEntityIDArray selectedSquadList;
      selectedSquadList.clear();
      if (results.getSize() > 0)
         selectedSquadList = testSquadListAgainstInputUIModeEntityFilterSet(results);
      if (selectedSquadList.getSize() > 0)
         mSelectionManager->selectSquads(selectedSquadList);      
   }
   else if (getFlagHaveHoverPoint() && (mUserMode == cUserModeInputUIPlaceSquadList))
   {
      // Squads already provided by UI placement mode so clear selection list
      mSelectionManager->clearSelections();
   }

   // Are we hovering over an enemy's hit zone?
   if (mHoverObject != cInvalidObjectID && mHoverType == cHoverTypeEnemy)
      updateHoverHitZone();

   // Sticky reticle - Use computed sticky hover object if set
   if (getOption_StickyCrosshairSensitivity() > 0 && (mUserMode != cUserModeInputUISquadList) && (mUserMode != cUserModeInputUIPlaceSquadList) && (mStickyHoverObject != cInvalidObjectID))
   {
      if (mHoverObject == cInvalidObjectID)
      {
         mHoverObject = mStickyHoverObject;
         mHoverType = mStickyHoverType;
         mHoverTypeAttackRating = mStickyHoverTypeAttackRating;
         mHoverResource = mStickyHoverResource;
         mHoverHitZoneIndex = mStickyHoverHitZoneIndex;
         mSelectionManager->updateSubSelectAbilities(mHoverObject, mHoverPoint);
         return false;
      }
   }

   if (!noRumble && mHoverObject != cInvalidObjectID && mHoverObject != saveHoverObject)
      gUI.playRumbleEvent(BRumbleEvent::cTypeUnitHover);

   return (mHoverObject != cInvalidObjectID);
}

//=============================================================================
// BUser::updateHoverType
//=============================================================================
bool BUser::updateHoverType(BSquad* pSelectedSquad, BEntityID hoverObject)
{
   bool minRange = false;
//-- FIXING PREFIX BUG ID 5690
   const BProtoAction* pAction = pSelectedSquad->getProtoActionForTarget(hoverObject, mHoverPoint, mAbilityID, false, &minRange);
//--
   if (!pAction)
   {
      // SLB: Look for any ability that can be done on this target
      if (mAbilityID == -1)
      {
         long abilityID = -1;
         bool targetsGround=false;
         pAction = pSelectedSquad->getProtoActionForTarget(hoverObject, mHoverPoint, mAbilityID, false, &minRange, true, &abilityID, &targetsGround);
         if (pAction && !targetsGround)
         {
            if (abilityID == gDatabase.getAIDCommand())
            {
               const BProtoObject* pProtoObject = pSelectedSquad->getProtoObject();
               if (pProtoObject)
                  abilityID = pProtoObject->getAbilityCommand();
            }
            BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
            if (pAbility && !pAbility->getNoAbilityReticle())
            {
               mHoverType = cHoverTypeAbility;
               return true;
            }
         }
      }

      if (minRange)
      {
         mHoverType = cHoverTypeMinRange;
         return true;
      }
      return false;
   }

   switch (pAction->getActionType())
   {
      case BAction::cActionTypeUnitRangedAttack:
         mHoverType = cHoverTypeEnemy;
         mHoverTypeAttackRating = getGoodAgainst(pSelectedSquad, hoverObject);
         return true;

      case BAction::cActionTypeUnitGather:
         mHoverType = cHoverTypeGather;
         mHoverResource = pAction->getResourceType();
         return true;

      case BAction::cActionTypeUnitCapture:
         mHoverType = cHoverTypeCapture;
         return true;

      case BAction::cActionTypeUnitGarrison:
      case BAction::cActionTypeUnitJoin:
         mHoverType = cHoverTypeGarrison;
         return true;

      case BAction::cActionTypeUnitHitch:
         mHoverType = cHoverTypeHitch;
         return true;

      case BAction::cActionTypeSquadRepairOther:
         mHoverType = cHoverTypeRepair;
         return true;
   }

   return false;
}

//=============================================================================
// BUser::updateHoverHitZone
//=============================================================================
void BUser::updateHoverHitZone()
{
   // Get unit associated with hover object
   BUnit* pUnit = gWorld->getUnit( mHoverObject );

   if( pUnit )
   {
      pUnit->clearTargetedHitZone();
//-- FIXING PREFIX BUG ID 5691
      const BVisual* pVisual = pUnit->getVisual();
//--
      if( pVisual && pUnit->getHitZoneList() )
      {
         // Iterate through unit's hit zones
         BHitZoneArray* pHitZoneList = pUnit->getHitZoneList();
         long           hitZoneCount = pHitZoneList->getNumber();
         float          closest      = -1.0f;
         for( long i = 0; i < hitZoneCount; i++ )
         {     
            BBoundingBox bbox;
            if( pHitZoneList->get( i ).getActive() && pUnit->getHitZoneOBB( i, bbox ) )
            {                                      
               // Define ray
               BVector  camPos   = mpCamera->getCameraLoc();
               BVector  camDir   = mpCamera->getCameraDir();
               float    dist     = 0.0f;
               bool     collided = bbox.raySegmentIntersects( camPos, camDir, false, NULL, dist );
               if( collided && ( ( closest == -1.0f ) || ( closest > dist ) ) )                     
               {
                  if( mHoverHitZoneIndex != -1 )
                  {
                     // Halwes - 2/2/2007 - This is temporary to draw targeted hit zone visuals with different tint colors.   
                     //pHitZoneList->get( mHoverHitZoneIndex ).setTargeted( false );
                     pUnit->setTargetedHitZone( mHoverHitZoneIndex, false );
                  }

                  closest            = dist;
                  mHoverHitZoneIndex = i;
               }
            }
         }

         if( mHoverHitZoneIndex != -1 )
         {
            // Halwes - 2/2/2007 - This is temporary to draw targeted hit zone visuals with different tint colors.   
            //pHitZoneList->get( mHoverHitZoneIndex ).setTargeted( true );
            pUnit->setTargetedHitZone( mHoverHitZoneIndex, true );
            //printf( "****** YOU ARE LOOKING AT THE %s ******%d\n", pHitZoneList->get( mHoverHitZoneIndex ).getAttachmentName().asNative(), gBlah );
         }
      }
   }
}

//=============================================================================
// BUser::updateSelectionChangeForHover
//=============================================================================
void BUser::updateSelectionChangeForHover()
{
   // Update saved off sticky reticle hover data
   if (mStickyHoverObject != cInvalidObjectID)
   {
      mStickyHoverType = cHoverTypeNone;
      mStickyHoverTypeAttackRating = cReticleAttackNoEffectAgainst;
      mStickyHoverResource = cHoverResourceNone;

      BEntityIDArray selectedSquads;
      mSelectionManager->getSubSelectedSquads(selectedSquads);

      int numSelectedSquads = selectedSquads.getNumber();
      for (int i=0; i<numSelectedSquads; i++)
      {
         BSquad* pSelectedSquad = gWorld->getSquad(selectedSquads[i]);
         if (pSelectedSquad)
         {
            if (updateHoverType(pSelectedSquad, mStickyHoverObject))
               break;
         }
      }

      mStickyHoverType = mHoverType;
      mStickyHoverTypeAttackRating = mHoverTypeAttackRating;
      mStickyHoverResource = mHoverResource;

      if (mStickyHoverType == cHoverTypeNone)
      {
//-- FIXING PREFIX BUG ID 5692
         const BObject* pObject=gWorld->getObject(mStickyHoverObject);
//--
         if (pObject)
         {
            int selectType = pObject->getSelectType(getTeamID());
            if ((selectType != cSelectTypeNone) && ((selectType != cSelectTypeTarget) || BDisplayStats::isDisplaying()))
               mStickyHoverType = cHoverTypeSelect;
         }
      }
   }
}

//=============================================================================
// BUser::getGoodAgainst - this function is going to get rewritten.
//=============================================================================
uint BUser::getGoodAgainst(BSquad * pSquad, BEntityID targetID)
{
   if (!pSquad)
      return cReticleAttackNoEffectAgainst;  // this shouldn't happen, but just in case.

   BDamageTypeID damageType = cInvalidDamageTypeID;
   if (targetID.getType() == BEntity::cClassTypeSquad)
   {
//-- FIXING PREFIX BUG ID 5693
      const BSquad* pTargetSquad = gWorld->getSquad(targetID);
//--
      if (pTargetSquad)
         damageType = pTargetSquad->getProtoSquad()->getDamageType();
   }
   else if (targetID.getType() == BEntity::cClassTypeUnit)
   {
//-- FIXING PREFIX BUG ID 5694
      const BUnit* pTargetUnit = gWorld->getUnit(targetID);
//--
      if (pTargetUnit)
         damageType = pTargetUnit->getProtoObject()->getDamageType();
   }
   if (damageType == cInvalidDamageTypeID)
      return cReticleAttackNoEffectAgainst;

   uint statValue = pSquad->getAttackGrade(damageType);

   // extreme?
   if (statValue >= gDatabase.getGoodAgainstAttackGrade(cReticleAttackExtremeAgainst))   // extreme?
      return cReticleAttackExtremeAgainst;
   else if (statValue >= gDatabase.getGoodAgainstAttackGrade(cReticleAttackGoodAgainst)) // good?
      return cReticleAttackGoodAgainst;
   else if (statValue >= gDatabase.getGoodAgainstAttackGrade(cReticleAttackFairAgainst)) // fair?
      return cReticleAttackFairAgainst;
   else if (statValue >= gDatabase.getGoodAgainstAttackGrade(cReticleAttackWeakAgainst)) // weak?
      return cReticleAttackWeakAgainst;

   // default - no effect
   return cReticleAttackNoEffectAgainst;
}


//=============================================================================
// BUser::updateHoverVisual
//=============================================================================
void BUser::updateHoverVisual(float elapsedTime)
{
   if(mpHoverVisual)
   {
      BMatrix mat;
      mat.makeIdentity();

      DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextUI);

      mpHoverVisual->update(elapsedTime, playerColor, mat, gWorld->getSubUpdate());
      if(getFlagHaveHoverPoint())
      {
         BMatrix worldMatrix;
         worldMatrix.makeTranslate(mHoverPoint);
         mpHoverVisual->updateWorldMatrix(worldMatrix, NULL);
      }
   }
}

//=============================================================================
// BUser::updateCircleSelect
//=============================================================================
void BUser::updateCircleSelect(float elapsedTime)
{
   float secs=elapsedTime;
   if(secs>0.1f)
      secs=0.1f;

   if (mFlagSkipNextSelect)
   {
      mSkipNextSelectTime -= elapsedTime;
      if (mSkipNextSelectTime <= 0.0f)
         mFlagSkipNextSelect = false;
   }

   if(!getFlagCircleSelectGrow())
   {
      if (!mFlagSkipNextSelect)
      {
         setFlagCircleSelectGrow(true);
         gConfig.get(cConfigCircleSelectRate, &mCircleSelectRate);
         if(mCircleSelectRate==0.0f)
         {
            mCircleSelectSize=mCircleSelectMaxSize;
            setFlagCircleSelectFullyGrown(true);
         }
      }
   }
   else if(!getFlagCircleSelectFullyGrown())
   {
      float rateAccel=0.0f;
      float maxRate=0.0f;

      gConfig.get(cConfigCircleSelectRateAccel, &rateAccel);
      gConfig.get(cConfigCircleSelectMaxRate, &maxRate);

      float fadeAccel= 0.0f;
      gConfig.get(cConfigCircleSelectFadeInAccel, &fadeAccel);
      float rate;
      float opacityRate;

      if(mCircleSelectRate<maxRate)
      {
         float rateAdd = rateAccel * secs;
         
         float newRate = mCircleSelectRate + rateAdd;
         if (newRate > maxRate)
            newRate = maxRate;

         rate = mCircleSelectRate + (rateAdd * 0.5f);

         mCircleSelectRate = newRate;


         float opacityRateAdd = fadeAccel * secs;
         float newOpacityRate = Math::ClampHigh(mCircleSelectOpacityFadeRate + opacityRateAdd, 1.0f);
         opacityRate = mCircleSelectOpacityFadeRate + (opacityRateAdd * 0.5f);
         mCircleSelectOpacityFadeRate = newOpacityRate;
      }
      else
      {
         rate = mCircleSelectRate;
         opacityRate = mCircleSelectOpacityFadeRate;
      }

      mCircleSelectSize+=rate*secs;
         mCircleSelectOpacity+=opacityRate*secs;
         mCircleSelectOpacity= Math::ClampHigh(mCircleSelectOpacity, 1.0f);
      if(mCircleSelectSize>=mCircleSelectMaxSize)
      {
         mCircleSelectSize=mCircleSelectMaxSize;
         setFlagCircleSelectFullyGrown(true);
         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlCircleSelectFullyGrown, mPlayerID);
      }
   }

   if(getFlagHaveHoverPoint() && getFlagCircleSelectGrow())
   {
      // Get our previous selections
      //const BEntityIDArray &lastSelected = mSelectionManager->getSelectedUnits();

      // Get our possible new selections.
      if(!getFlagCircleSelectPoints())
      {
         float clickSize=20.0f;
         gConfig.get(cConfigCircleSelectClickSize, &clickSize);
         mSelectionManager->populateSelectionsRect(mpCamera->getCameraLoc(), mHoverPoint, clickSize*0.5f, clickSize*0.5f, false, false);
      }
      else
      {
         mSelectionManager->populateSelectionsCircle(mpCamera->getCameraLoc(), mCircleSelectPoints, cNumCircleSelectPoints, 0.0f, false);
      }

      bool usePrevSelection = (!getFlagCircleSelectReset() || getFlagModifierAction());
      int baseSelectType=-1;
      int baseProtoID=-1;
      if (mSelectionManager->getNumberSelectedUnits() > 0 && usePrevSelection)
      {
         const BEntityIDArray& units=mSelectionManager->getSelectedUnits();
         for (uint i=0; i<units.getSize(); i++)
         {
//-- FIXING PREFIX BUG ID 5695
            const BUnit* pUnit=gWorld->getUnit(units[i]);
//--
            if (pUnit)
            {
               baseProtoID=pUnit->getProtoID();
               baseSelectType=pUnit->getSelectType(getTeamID());
               break;
            }
         }
      }

      // Go through all the possible selections and qualify or disqualify them.      
      long userPlayerID = getPlayerID();
      long numPossibleSelections = mSelectionManager->getPossibleSelectionCount();
      long selectionsMade = 0;
      for (long i=0; i<numPossibleSelections; i++)
      {
         BEntityID possibleSelectionUnitID = mSelectionManager->getPossibleSelection(i);
         if (possibleSelectionUnitID.getType() != BEntity::cClassTypeUnit)
            continue;

         // Already selected.
         if (usePrevSelection && mSelectionManager->isUnitSelected(possibleSelectionUnitID))
            continue;

         // Not a unit or select type not type unit.
         BUnit *pUnit = gWorld->getUnit(possibleSelectionUnitID);
         if (!pUnit)
            continue;

         // Can't circle select another player's stuff.
         if (pUnit->getPlayerID() != userPlayerID)
            continue;

         int selectType = pUnit->getSelectType(getTeamID());
         if(baseSelectType!=-1 && selectType!=baseSelectType)
            continue;

         int protoID = pUnit->getProtoID();
         if (selectType==cSelectTypeSingleType && baseProtoID!=-1 && protoID!=baseProtoID)
            continue;

         if (selectType != cSelectTypeUnit && selectType != cSelectTypeSingleType)
            continue;

         // Clear the selections if necessary.
         if(getFlagCircleSelectReset())
         {
            if (!getFlagModifierAction())
	            mSelectionManager->clearSelections();
            setFlagCircleSelectReset(false);
         }

         bool selected = mSelectionManager->selectUnit(possibleSelectionUnitID);
         if (selected)
         {
            setFlagCircleSelectSelected(true);

            selectionsMade++;

            playUnitSelectSound(possibleSelectionUnitID);
            gUI.playRumbleEvent(BRumbleEvent::cTypePaintSelect);

            if (selectType==cSelectTypeSingleUnit)
               break;

            if (baseSelectType==-1)
               baseSelectType=selectType;

            if (baseProtoID==-1)
               baseProtoID=protoID;
         }
      }
      if(selectionsMade > 0)
      {
         mFlagCircleSelectDoNotification=true;
      }
   }

   if (mFlagCircleSelectDoNotification)
   {
      float minSizeForEvent = 0.0f;
      gConfig.get(cConfigCircleSelectEventSize, &minSizeForEvent);
      if (mCircleSelectSize >= minSizeForEvent)
      {
         if (!mFlagCircleSelectVinceEvent)
         {
            MVinceEventAsync_ControlUsed( this, "circle_multi_select" );
            mFlagCircleSelectVinceEvent = true;
         }
         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlCircleMultiSelect, mPlayerID);
         mFlagCircleSelectDoNotification=false;
      }
   }
}

//=============================================================================
// BUser::updateCircleSelectShrink
//=============================================================================
void BUser::updateCircleSelectShrink(float elapsedTime)
{      
   float secs=Math::ClampHigh(elapsedTime, 0.1f);
   
   if(getFlagCircleSelectShrink() && !getFlagCircleSelectFullyShrunk())
   {
      float rateAccel=0.0f;
      float maxRate=0.0f;

      gConfig.get(cConfigCircleSelectRateAccel, &rateAccel);
      gConfig.get(cConfigCircleSelectMaxRate, &maxRate);

      float fadeAccel= 0.0f;
      gConfig.get(cConfigCircleSelectFadeInAccel, &fadeAccel);
      float rate;
      float opacityRate;

      if(mCircleSelectRate<maxRate)
      {
         float rateAdd = rateAccel * secs;
         
         float newRate = mCircleSelectRate + rateAdd;
         if (newRate > maxRate)
            newRate = maxRate;

         rate = mCircleSelectRate + (rateAdd * 0.5f);

         mCircleSelectRate = newRate;


         float opacityRateAdd = fadeAccel * secs;
         float newOpacityRate = Math::ClampHigh(mCircleSelectOpacityFadeRate + opacityRateAdd, 1.0f);
         opacityRate = mCircleSelectOpacityFadeRate + (opacityRateAdd * 0.5f);
         mCircleSelectOpacityFadeRate = newOpacityRate;
      }
      else
      {
         rate = mCircleSelectRate;
         opacityRate = mCircleSelectOpacityFadeRate;
      }
      
      
      float shrinkMultiplier = 1.5f;
      mCircleSelectSize    -= shrinkMultiplier * rate * secs;
      mCircleSelectOpacity -= opacityRate* secs;
      mCircleSelectOpacity= Math::ClampLow(mCircleSelectOpacity, 0.0f);

      if(mCircleSelectSize<=mCircleSelectBaseSize)
      {
         mCircleSelectSize=mCircleSelectBaseSize;
         setFlagCircleSelectFullyShrunk(true);  
         setFlagCircleSelectShrink(false);
      }
   }
}

//==============================================================================
// BUser::updateSelectedUnitIcons
//==============================================================================
void BUser::updateSelectedUnitIcons()
{
   if(mSelectionManager->getNumberSelectedSquads()==0 || mFlagOverrideUnitPanelDisplay)
   {
      if(mSelectedUnitDisplayCount!=0)
         mSelectedUnitDisplayCount=0;

      if (gConfig.isDefined(cConfigFlashGameUI))
      {
         if (getHUDItemEnabled(BUser::cHUDItemUnits))
         {
            mpUIContext->setUnitSelectionVisible(false);
            mpUIContext->setUnitCardVisible(false);
         }
         if (getHUDItemEnabled(BUser::cHUDItemUnitStats))
         {
            mpUIContext->setUnitStatsVisible(false);
         }
      }
      return;
   }

   // Build a list of selected squads and units not in squads
   mSelectedUnitDisplayCount=0;

   struct BUnitIconData
   {
      int16 mSquad;
      BEntityID mSquadID;
      int   mProtoID;
      uint  mCount;
      int   mSubselectGroupID;
   };

   int dataIndexToDisplay = 0;
   BSmallDynamicSimArray<BUnitIconData> dataList;

   int subSelectGroup = mSelectionManager->getSubSelectGroupHandle();

   uint count=mSelectionManager->getNumSubSelectGroups();
   for (uint i=0; i<count; i++)
   {
      const BEntityIDArray& group=mSelectionManager->getSubSelectGroup(i);
      if (group.getNumber() == 0)
         continue;
//-- FIXING PREFIX BUG ID 5697
      const BSquad* pSquad=gWorld->getSquad(group[0]);
//--
      if (!pSquad)
         continue;
      BUnitIconData data;
      if (subSelectGroup == (int)i)
         dataIndexToDisplay = i;
      data.mSquadID = pSquad->getID();
      data.mProtoID = pSquad->getProtoSquadID();
      if (data.mProtoID == -1)
      {
         data.mProtoID = pSquad->getProtoObjectID();
         data.mSquad = 0;
      }
      else
         data.mSquad = 1;
      data.mCount=group.getSize();
      data.mSubselectGroupID = i;
      dataList.add(data);
   }

   if(dataList.getNumber()==0)
      return;

   // Update and render the icons
   long sx, sy, iconSize, spacing;
   if (gGame.isSplitScreen())
   {
      if (gGame.isVerticalSplit())
      {
         iconSize=48;
         spacing=64;
         sy=gUI.mlSafeY2-iconSize-20;
         if (this == gUserManager.getPrimaryUser())
            sx=gUI.mlSafeX1;
         else
            sx=gUI.mlCenterX+20;
      }
      else
      {
         iconSize=48;
         spacing=64;
         sx=gUI.mlCenterX-((spacing*8)/2);
         if (this == gUserManager.getPrimaryUser())
            sy=gUI.mlCenterY-iconSize-20;
         else
            sy=gUI.mlSafeY2-iconSize-20;
      }
   }
   else
   {
      iconSize=48;
      spacing=80;
      sx=gUI.mlCenterX-((spacing*8)/2);
      sy=gUI.mlSafeY2-iconSize-20;
   }

   gFontManager.setFont(gUIGame.getPlayerStatFont());
   long fontHgt=(long)(gFontManager.getLineHeight()+0.5f);
   BUString label;
   count=dataList.getSize();
   for(uint i=0; i<count && i<cMaxSelectedUnitIcons; i++)
   {
      BUnitIconData& data=dataList[i];

      long x=sx+(i*spacing);
      long y=sy;

      BManagedTextureHandle textureHandle=cInvalidManagedTextureHandle;
      if(data.mSquad)
         textureHandle=gUIGame.getSquadIcon(data.mProtoID, getPlayerID());
      else
         textureHandle=gUIGame.getObjectIcon(data.mProtoID, getPlayerID());
      mSelectedUnitIcons[i].setTexture(textureHandle, true);
      mSelectedUnitIcons[i].setPosition(x, y);
      mSelectedUnitIcons[i].setSize(iconSize, iconSize);
      //if ((int)i == subSelectGroup)
      //   mSelectedUnitIcons[i].setColor(cDWORDGold);
      //else
         mSelectedUnitIcons[i].setColor(cDWORDWhite);
      mSelectedUnitIcons[i].refresh();

      label.format(L"%u", data.mCount);
      mSelectedUnitText[i].setText(label);
      mSelectedUnitText[i].setFont(gUIGame.getPlayerStatFont());
      mSelectedUnitText[i].setTextJust(BFontManager2::cJustifyLeft);
      //mSelectedUnitText[i].setPosition(x+cIconSize, y+(cIconSize/2)-(fontHgt/2));
      long textLen=(long)(gFontManager.getLineLength(label, label.length())+0.5f);
      //mSelectedUnitText[i].setPosition(x+cIconSize, y+cIconSize-fontHgt);
      mSelectedUnitText[i].setPosition(x+iconSize/2, y+iconSize);
      mSelectedUnitText[i].setSize(fontHgt, textLen);
      mSelectedUnitText[i].refresh();

      mSelectedUnitDisplayCount++;
   }

   if (gConfig.isDefined(cConfigFlashGameUI))
   {      
      BUString detail;
      BUString name;
      BUString role;
      BUString playerName;
      int protoID;
      int objID;
      int frameID;
      

      if (getHUDItemEnabled(BUser::cHUDItemUnits))
      {
         mpUIContext->setUnitSelectionVisible(true);
         mpUIContext->setUnitCardVisible(true);
      }

      int subSelectIndex = mSelectionManager->getSubSelectGroupHandle();
      int uiSubSelectIndex = -1;
      int dataCount = dataList.getNumber();
      int startIndex = mSelectionManager->getSubSelectGroupUIStartIndex();
      int endIndex   = mSelectionManager->getSubSelectGroupUIEndIndex();      
      
      detail.empty();
      name.empty();
      role.empty();
      playerName.empty();
      DWORD playerColor = cDWORDWhite;
      int uiSlotIndex = 0;
      for (int i = startIndex; i<=endIndex; i++)
      {  
         if (i < dataCount)
         {          
//-- FIXING PREFIX BUG ID 5701
            const BSquad* pSquad = gWorld->getSquad(dataList[i].mSquadID);
//--

            if (dataList[i].mSquad)
            {
               const BProtoSquad* pProtoSquad = (pSquad ? pSquad->getProtoSquad() : gDatabase.getGenericProtoSquad(dataList[i].mProtoID));
               if (pProtoSquad)
               {
                  pProtoSquad->getRoleText(role);
                  pProtoSquad->getDisplayName(name);

                  // Hack: If this is a hibernating unit, we need to prepend "Dormant" to the name
                  // ...and change the role text to "Waiting to Regenerate"
                  BUnit* pLeaderUnit = pSquad->getLeaderUnit();
                  if (pLeaderUnit && pLeaderUnit->isHibernating())
                  {
                     name.prepend(L" ");
                     name.prepend(gDatabase.getLocStringFromID(25723)); // "Dormant"
                     role.set(gDatabase.getLocStringFromID(25701)); // "Waiting to Regenerate"
                  }
               }
            }
            else
            {
               const BProtoObject* pProtoObject = NULL;
               if (pSquad)
                  pProtoObject = pSquad->getProtoObject();
               if (!pProtoObject)
               {
//-- FIXING PREFIX BUG ID 5699
                  const BPlayer* pPlayer = getPlayer();
//--
                  pProtoObject = pPlayer ? pPlayer->getProtoObject(dataList[i].mProtoID) : gDatabase.getGenericProtoObject(dataList[i].mProtoID);
               }
               if (pProtoObject)
               {
                  pProtoObject->getRoleText(role);
                  pProtoObject->getDisplayName(name);
               }
            }                        
            
            
            bool bShowGamerTag = false;
            int abilityID = -1;
            int playerID = -1;
            if (pSquad)
            {
//-- FIXING PREFIX BUG ID 5700
               const BPlayer* pPlayer = gWorld->getPlayer(pSquad->getPlayerID());
//--
               if (pPlayer)
               {
                  BUString tempName = pPlayer->getLocalisedDisplayName();
                  if(tempName.isEmpty() == false)
                  {
                     playerName = tempName;
                  }
                  else
                  {
                     playerName.set(BStrConv::toB(pPlayer->getName()));
                  }
                  playerID = pPlayer->getID();
               }               

               int squadPlayerID = pSquad->getPlayerID();
               if (squadPlayerID > 0 && squadPlayerID != gUserManager.getPrimaryUser()->getPlayerID())
                  bShowGamerTag = true;

               playerColor = gWorld->getPlayerColor(pSquad->getColorPlayerID(), BWorld::cPlayerColorContextSelection);
            }

            int abilityFrameID = BFlashHUD::eHUDKeyframeOff;
            bool bShowAbility = false;
            if (dataList[i].mSubselectGroupID != -1)
            {               
               //-- BTK - Bug Fix [PHX-8381]
               //-- I changed this to compute a selection ability for the current subselect group every time this executes.
               //-- The cached values in the selection manager do not get updated frequently enough.  If this 
               //-- turns out to be too slow we need to find a clever way of updating the cached values more often
               //-- without breaking a bunch of stuff.
               BSelectionAbility ability;
               const BEntityIDArray& subselectEntities = mSelectionManager->getSubSelectGroup(dataList[i].mSubselectGroupID);
               BSimHelper::calculateSelectionAbility(subselectEntities, getPlayerID(), ability);
               BSimHelper::updateSelectionAbility(subselectEntities, mHoverObject, mHoverPoint, ability);                              
               if (ability.mAbilityID != -1)
               {
                  bShowAbility = true;
                  abilityID = ability.mAbilityID;
                  if (ability.mRecovering || !ability.mPlayer)
                     abilityFrameID = BFlashHUD::eHUDKeyframePending;
                  else
                     abilityFrameID = BFlashHUD::eHUDKeyframeOn;
               }
            }

            BEntityID squadID = dataList[i].mSquadID;
            protoID = dataList[i].mSquad ? dataList[i].mProtoID : -1;
            objID   = !dataList[i].mSquad ? dataList[i].mProtoID : -1;
            
            //-- unselect the unit
            if (dataList[i].mCount == 0)
            {
               squadID = cInvalidObjectID;
               protoID = -1;
               objID = -1;            
            }            

            if ( (dataCount == 1) && (dataList[i].mCount == 1))
               frameID = bShowAbility ? BFlashHUD::eHUDKeyframeCardSkirmish : BFlashHUD::eHUDKeyframeOver;
            else if (dataCount == 1)
               frameID = BFlashHUD::eHUDKeyframeOver;
            else 
               frameID = (i == subSelectIndex) ? BFlashHUD::eHUDKeyframeOver : BFlashHUD::eHUDKeyframeOn;
                        
            bool tagged = (i == subSelectIndex);

            if (i == subSelectIndex)
               uiSubSelectIndex = uiSlotIndex;            
            
            if ( (dataCount == 1) && (dataList[i].mCount == 1))
            {
               mpUIContext->setUnitCard(uiSlotIndex, frameID, playerID, squadID, protoID, objID, abilityID, tagged, abilityFrameID, dataList[i].mCount, detail, name, role, bShowGamerTag, playerName, playerColor);
               mpUIContext->setUnitSelection(uiSlotIndex, BFlashHUD::eHUDKeyframeOff, cInvalidObjectID, -1, -1, false, BFlashHUD::eHUDKeyframeOff, -1, detail, name, role, false, playerName, cDWORDWhite);
            }
            else
            {
               if (dataCount==1)
                  tagged=true;
               mpUIContext->setUnitCard(uiSlotIndex, BFlashHUD::eHUDKeyframeOff, -1, cInvalidObjectID, -1, -1, -1, false, BFlashHUD::eHUDKeyframeOff, -1, detail, name, role, false, playerName, cDWORDWhite);
               mpUIContext->setUnitSelection(uiSlotIndex, frameID, squadID, protoID, objID, tagged, abilityFrameID, dataList[i].mCount, detail, name, role, bShowGamerTag, playerName, playerColor);
            }
         }
         else
         {
            mpUIContext->setUnitSelection(uiSlotIndex, BFlashHUD::eHUDKeyframeOff, cInvalidObjectID, -1, -1, false, BFlashHUD::eHUDKeyframeOff, -1, detail, name, role, false, playerName, cDWORDWhite);
         }

         uiSlotIndex++;
      }

      mpUIContext->updateUnitCardDisplay();
      if ( (dataCount == 1) && (dataList[0].mCount > 1))
         uiSubSelectIndex=0;

      bool displayArrows = (dataCount > 8) && (uiSubSelectIndex != -1);
      mpUIContext->updateUnitSelectionDisplay(uiSubSelectIndex, displayArrows);

      if (dataList.getNumber() > dataIndexToDisplay)
      {
         // figure out which unit to display stats on.
//-- FIXING PREFIX BUG ID 5702
         const BSquad* pSquad = gWorld->getSquad(dataList[dataIndexToDisplay].mSquadID);
//--
         if (pSquad)
         {
            // Show the unit stat info here.
            if (getHUDItemEnabled(BUser::cHUDItemUnitStats))
               mpUIContext->setUnitStatsVisible(true);

            const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();

            BUString objectName;
            pProtoSquad->getDisplayName(objectName);

            uint attackGrades[4];
            memset(attackGrades, 0, sizeof(attackGrades));
            uint index=0;
            long statCount=gUIGame.getNumberUnitStats();
            for(long j=0; j<statCount; j++)
            {
               const BUIGameUnitStat* pUnitStat=gUIGame.getUnitStat(j);
               if (pUnitStat->mStatType == BUIGameUnitStat::cTypeAttackGrade)
               {
                  attackGrades[index] = pProtoSquad->getAttackGrade((int8)pUnitStat->mStatData);
                  index++;
                  if (index == 4)
                     break;
               }
            }

            mpUIContext->setUnitStats(objectName, (float)attackGrades[0], (float)attackGrades[1], (float)attackGrades[2], (float)attackGrades[3], 0.0f);
         }
      }
   }
}


//==============================================================================
// BUser::isUserLocked
//==============================================================================
bool BUser::isUserLocked(void) const
{
   return(getFlagUserLocked());
}


//==============================================================================
// BUser::isUserLockedByTriggerScript
//==============================================================================
bool BUser::isUserLockedByTriggerScript(BTriggerScriptID triggerScriptID) const
{
   // Any of these three things failing means it's not locked by the trigger script specified.
   if (getFlagUserLocked() && mUILockOwner == triggerScriptID && gTriggerManager.getTriggerScript(triggerScriptID))
      return(true);
   else
      return(false);
}


//==============================================================================
// BUser::lockUser
//==============================================================================
void BUser::lockUser(BTriggerScriptID triggerScriptID)
{
   setFlagUserLocked(true);
   mUILockOwner = gTriggerManager.getTriggerScript(triggerScriptID) ? triggerScriptID : cInvalidTriggerScriptID;
}


//==============================================================================
// BUser::unlockUser
//==============================================================================
void BUser::unlockUser(void)
{
   setFlagUserLocked(false);
   mUILockOwner = cInvalidTriggerScriptID;
}

//==============================================================================
// BUser::circleSelect
//==============================================================================
void BUser::circleSelect(bool on, bool clearSelections)
{
   if(on)
   {
      resetGotoSelected();

      setFlagCircleSelectGrow(false);
      setFlagCircleSelectFullyGrown(false);
      setFlagCircleSelectSelected(false);
      setFlagCircleSelectShrink(false);
      setFlagCircleSelectFullyShrunk(false);
      setFlagCircleSelectDoNotification(false);
      setFlagCircleSelectVinceEvent(false);
     
      //mCircleSelectSize=mCircleSelectBaseSize;
      mCircleSelectRate=0.0f;      
      mCircleSelectOpacityFadeRate=0.0f;

      if(getFlagModifierAction() || !clearSelections)
         setFlagCircleSelectReset(false);
      else
         setFlagCircleSelectReset(true);

      changeMode(cUserModeCircleSelecting);
   }
   else
   {
      // Don't do select here if the user has been holding the multi select button too long
      bool allowSelect = true;
      float minSizeForEvent = 0.0f;
      gConfig.get(cConfigCircleSelectEventSize, &minSizeForEvent);
      if (mCircleSelectSize >= minSizeForEvent)
         allowSelect = false;

      if(mSelectionManager->getNumberSelectedUnits() == 0)
      {
         setFlagCircleSelectGrow(false);
         setFlagCircleSelectShrink(true);
         updateHoverPoint(1.0f, 1.0f, true);
      }

      bool doCommandMenu=false;
      bool doRallyPoint=false;
      bool doSingleSelect=false;
      bool isPlayer=false;
      bool isCommandableByAnyPlayer=false;
      bool mustOwnToSelect=false;
//-- FIXING PREFIX BUG ID 5696
      const BUnit* pUnit = NULL;
//--

      BEntityID selectableObject = mHoverObject;
      if (selectableObject == cInvalidObjectID || mHoverType != cHoverTypeSelect)
      {
         selectableObject = cInvalidObjectID;
         float clickSize=20.0f;
         gConfig.get(cConfigCircleSelectClickSize, &clickSize);
         mSelectionManager->populateSelectionsRect(mpCamera->getCameraLoc(), mHoverPoint, clickSize*0.5f, clickSize*0.5f, false, false);
//-- FIXING PREFIX BUG ID 5703
         const BObject* pObject=gWorld->getObject(mSelectionManager->getPossibleSelection(0));
//--
         if(pObject)
         {
            int selectType = pObject->getSelectType(getTeamID());
            if ((selectType != cSelectTypeNone) && (selectType != cSelectTypeTarget) && (!pObject->getProtoObject()->getFlagMustOwnToSelect() || (!pObject->getDopple() && (pObject->getPlayerID() == mPlayerID || pObject->getPlayerID() == mCoopPlayerID))))
               selectableObject = pObject->getID();
         }
      }

      if (allowSelect)
      {
         if (selectableObject!=cInvalidObjectID)
         {
            if (selectableObject.getType() == BEntity::cClassTypeDopple)
            {
               BObject * pObject = gWorld->getObject(selectableObject);
               if (pObject)
                  pUnit=gWorld->getUnit(pObject->getParentID());
            }
            else
            {
               pUnit=gWorld->getUnit(selectableObject);
            }
         }

         if (pUnit)
         {
            isPlayer = (pUnit->getPlayerID() == mPlayerID || pUnit->getPlayerID() == mCoopPlayerID);    // see if this is the current player's unit
            if (!isPlayer && gWorld->getFlagCoop())
            {
               // Jira PHX-12729 - Host player needs to be able to access command menu for client player's locked down elephant.
               if (pUnit->getPlayer()->isHuman() && gWorld->getPlayer(mPlayerID)->getTeamID() == pUnit->getTeamID())
                  isPlayer = true;
            }
         }

         if (pUnit)
         {
            isCommandableByAnyPlayer = pUnit->getProtoObject()->getFlagCommandableByAnyPlayer();
            mustOwnToSelect = pUnit->getProtoObject()->getFlagMustOwnToSelect();
         }

         if((mSelectionManager->getNumberSelectedUnits()==0 || getFlagCircleSelectReset() || !getFlagCircleSelectSelected()) && pUnit)
         {
            if(pUnit->getSelectType(getTeamID())==cSelectTypeCommand)
            {
               if(gConfig.isDefined(cConfigBuildingMenuOnSelect))
               {
                  // Pop-up the command menu
                  doCommandMenu=true;
                  if(getFlagCircleSelectReset())
                  {
                     mSelectionManager->clearSelections();
                     setFlagCircleSelectReset(false);
                  }
               }
            }
            else if(pUnit->getSelectType(getTeamID())==cSelectTypeSingleUnit)
            {
               int rallyPointProtoID=getPlayer()->getCiv()->getRallyPointObjectID();
               if (pUnit->getProtoID()==rallyPointProtoID && pUnit->getPlayerID()==mPlayerID)
               {
                  // Let the user move the rally point
                  doRallyPoint=true;
                  if(getFlagCircleSelectReset())
                  {
                     mSelectionManager->clearSelections();
                     setFlagCircleSelectReset(false);
                  }
               }
               else
               {
                  doSingleSelect=true;
                  if(getFlagCircleSelectReset())
                  {
                     mSelectionManager->clearSelections();
                     setFlagCircleSelectReset(false);
                  }
               }
            }
         }

         // Cycle between units when pointing at more than one unit
         if(!doCommandMenu && !doSingleSelect && !doRallyPoint && selectableObject!=cInvalidObjectID && canSelectUnit(selectableObject, getFlagCircleSelectReset()) && !mFlagSkipNextSelect)
         {
            BEntityID selectID=selectableObject;
            
            if(mCircleSelectLastSelected!=cInvalidObjectID && mHoverPoint.almostEqual(mCircleSelectLastPoint))
            {
               long count=mSelectionManager->getPossibleSelectionCount();
               for(long i=0; i<count; i++)
               {
                  if(mSelectionManager->getPossibleSelection(i)==mCircleSelectLastSelected)
                  {
//-- FIXING PREFIX BUG ID 5705
                     const BUnit* pLastUnit=gWorld->getUnit(mCircleSelectLastSelected);
//--
                     if(pLastUnit)
                     {
                        BEntityID lastParentID=pLastUnit->getParentID();
                        long selectIndex=i;
                        for(;;)
                        {
                           selectIndex++;
                           if(selectIndex>=count)
                              selectIndex=0;
                           if(selectIndex==i)
                              break;
                           BEntityID checkID=mSelectionManager->getPossibleSelection(selectIndex);
                           if(!canSelectUnit(checkID, getFlagCircleSelectReset()))
                              continue;
//-- FIXING PREFIX BUG ID 5704
                           const BUnit* pCheckUnit=gWorld->getUnit(checkID);
//--
                           if(pCheckUnit && pCheckUnit->getParentID()!=lastParentID)
                           {
                              selectID=checkID;
                              break;
                           }
                        }
                     }
                     break;
                  }
               }
            }

            if(selectID!=cInvalidObjectID)
            {
               if(getFlagCircleSelectReset())
               {
                  mSelectionManager->clearSelections();
                  setFlagCircleSelectReset(false);
               }

               bool okayToSelect=true;
               if (mSelectionManager->getNumberSelectedUnits() > 0)
               {
//-- FIXING PREFIX BUG ID 5706
                  const BUnit* pUnit=gWorld->getUnit(selectID);
//--
                  if (pUnit)
                  {
                     int selectType = pUnit->getSelectType(getTeamID());
                     int protoID = pUnit->getProtoID();

                     const BEntityIDArray& units=mSelectionManager->getSelectedUnits();
                     for (uint i=0; i<units.getSize(); i++)
                     {
                        pUnit=gWorld->getUnit(units[i]);
                        if (pUnit)
                        {
                           int baseProtoID=pUnit->getProtoID();
                           int baseSelectType=pUnit->getSelectType(getTeamID());

                           if (selectType!=baseSelectType)
                              okayToSelect=false;

                           if (selectType==cSelectTypeSingleType && protoID!=baseProtoID)
                              okayToSelect=false;

                           break;
                        }
                     }
                  }
               }

               bool alreadySelected = mSelectionManager->isUnitSelected(selectID); //-- DJB - Added check to fix selecting a unit here and also in updateCircleSelect()
               if (okayToSelect && !alreadySelected)
               {
                  mSelectionManager->selectUnit(selectID);
                  mCircleSelectLastSelected=selectID;
                  mCircleSelectLastPoint=mHoverPoint;
                  const BUnit *pLastSelected = gWorld->getUnit(mCircleSelectLastSelected);
                  if (pLastSelected && pLastSelected->getPlayerID() == mPlayerID)
                  {
                     playUnitSelectSound(mCircleSelectLastSelected);
                     gUI.playRumbleEvent(BRumbleEvent::cTypePaintSelect);
                  }
                  MVinceEventAsync_ControlUsed( this, "circle_select" );
                  gGeneralEventManager.eventTrigger(BEventDefinitions::cControlCircleSelect, mPlayerID);

               }
            }
         }
      }

      setFlagCircleSelectGrow(false);
      setFlagCircleSelectFullyGrown(false);
      
      setFlagCircleSelectShrink(true);
      setFlagCircleSelectFullyShrunk(false);

      setFlagCircleSelectReset(false);
      setFlagCircleSelectSelected(false);
      
      changeMode(cUserModeNormal);
      mCircleSelectRate=0.0f;
      mCircleSelectOpacityFadeRate=0.0f;
      mFlagSkipNextSelect=false;

      if(doCommandMenu)
      {
         if (!mustOwnToSelect || isPlayer || isCommandableByAnyPlayer)
         {
            mSelectionManager->clearSelections();
            mSelectionManager->selectUnit(pUnit->getID());
            gUI.playRumbleEvent(BRumbleEvent::cTypePaintSelect);
            if ((isPlayer || isCommandableByAnyPlayer) && pUnit->isVisible(getPlayer()->getTeamID()))
            {
               mFlagDeselectOnMenuClose=true;
               showCommandMenu(selectableObject);
            }
         }
         return;
      }

      if (doSingleSelect)
      {
         if (!mustOwnToSelect || isPlayer || isCommandableByAnyPlayer)
         {
            mSelectionManager->clearSelections();
            mSelectionManager->selectUnit(pUnit->getID());
            gUI.playRumbleEvent(BRumbleEvent::cTypePaintSelect);
         }
         return;
      }

      if (doRallyPoint)
      {
         setHoverVisual(getPlayer()->getCiv()->getRallyPointObjectID());
         changeMode(cUserModeRallyPoint);
         return;
      }
   }   
}

//==============================================================================
// BUser::resetCircleSelectionCycle
//==============================================================================
void BUser::resetCircleSelectionCycle()
{
   mCircleSelectLastSelected=cInvalidObjectID;
   mCircleSelectLastPoint=cInvalidVector;
}



//==============================================================================
// BUser::drawObjectSelectedShape
//==============================================================================
void BUser::drawObjectSelectedShape(const BObject* pObject, DWORD color, bool useObstructionRadius, bool drawRanges)
{
   BASSERT(pObject);
   if (!pObject->isVisible(mTeamID))
      return;

   const BProtoObject* pProtoObject=pObject->getProtoObject();
   BVector position=pObject->getPosition();
   BVector forward=pObject->getForward();
   BVector right=pObject->getRight();
   BVector up=pObject->getUp();
   BMatrix matrix;

   // Draw selection circle or rectangle
   if(pProtoObject->getFlagFlying())
   {
      float thickness = 0.02f; // Use smaller thickness for flying units since they are closer to the camera
      position-=up*0.5f; // lower the position so the circle will be under the object since the art is not offset correctly
      matrix.makeOrient(forward, up, right);
      matrix.setTranslation(position);
      if(pProtoObject->getFlagSelectedRect() || useObstructionRadius)
      {
         float rx=(useObstructionRadius ? pProtoObject->getObstructionRadiusX() : pProtoObject->getSelectedRadiusX());
         float rz=(useObstructionRadius ? pProtoObject->getObstructionRadiusZ() : pProtoObject->getSelectedRadiusZ());
         BVector p1, p2, p3, p4, p5, p6, p7, p8;

         float thicknessAdjustedRX = rx+thickness;
         float thicknessAdjustedRZ = rz+thickness;
         matrix.transformVectorAsPoint(BVector(-thicknessAdjustedRX,0,-thicknessAdjustedRZ), p5);
         matrix.transformVectorAsPoint(BVector(thicknessAdjustedRX,0,-thicknessAdjustedRZ), p6);
         matrix.transformVectorAsPoint(BVector(thicknessAdjustedRX,0,thicknessAdjustedRZ), p7);
         matrix.transformVectorAsPoint(BVector(-thicknessAdjustedRX,0,thicknessAdjustedRZ), p8);
         matrix.transformVectorAsPoint(BVector(-rx,0.0f,-rz), p1);
         matrix.transformVectorAsPoint(BVector(rx,0.0f,-rz), p2);
         matrix.transformVectorAsPoint(BVector(rx,0.0f,rz), p3);
         matrix.transformVectorAsPoint(BVector(-rx,0.0f,rz), p4);

         gpDebugPrimitives->addDebugThickLine(p5, p6, thickness, color, color);
         gpDebugPrimitives->addDebugThickLine(p2, p3, thickness, color, color);
         gpDebugPrimitives->addDebugThickLine(p7, p8, thickness, color, color);
         gpDebugPrimitives->addDebugThickLine(p4, p1, thickness, color, color);
      }
      else
      {
         gpDebugPrimitives->addDebugThickCircle(matrix, pProtoObject->getSelectedRadiusX(), thickness, color);
      }
   }
   else
   {
      matrix.makeOrient(forward, up, right);
      matrix.setTranslation(position+(cYAxisVector*0.1f));
      if(pProtoObject->getFlagSelectedRect() || useObstructionRadius)
      {
         float rx=(useObstructionRadius ? pProtoObject->getObstructionRadiusX() : pProtoObject->getSelectedRadiusX());
         float rz=(useObstructionRadius ? pProtoObject->getObstructionRadiusZ() : pProtoObject->getSelectedRadiusZ());
         BVector p1, p2, p3, p4, p5, p6, p7, p8;
         matrix.transformVectorAsPoint(BVector(-rx,0,-rz), p1);
         matrix.transformVectorAsPoint(BVector(rx,0,-rz), p2);
         matrix.transformVectorAsPoint(BVector(rx,0,rz), p3);
         matrix.transformVectorAsPoint(BVector(-rx,0,rz), p4);

         float thickness = 0.1f;
         float thicknessAdjustedRX = rx+thickness;
         float thicknessAdjustedRZ = rz+thickness;
         matrix.transformVectorAsPoint(BVector(-thicknessAdjustedRX,0,-thicknessAdjustedRZ), p5);
         matrix.transformVectorAsPoint(BVector(thicknessAdjustedRX,0,-thicknessAdjustedRZ), p6);
         matrix.transformVectorAsPoint(BVector(thicknessAdjustedRX,0,thicknessAdjustedRZ), p7);
         matrix.transformVectorAsPoint(BVector(-thicknessAdjustedRX,0,thicknessAdjustedRZ), p8);
         
         gTerrainSimRep.addDebugThickLineOverTerrain(p5, p6, thickness, color, color, 0.1f);
         gTerrainSimRep.addDebugThickLineOverTerrain(p2, p3, thickness, color, color, 0.1f);
         gTerrainSimRep.addDebugThickLineOverTerrain(p7, p8, thickness, color, color, 0.1f);
         gTerrainSimRep.addDebugThickLineOverTerrain(p4, p1, thickness, color, color, 0.1f);
      }
      else
      {
         if (!gConfig.isDefined(cConfigFlashGameUI))
         {
            const long cNumObjectSelectPoints=24;
            BVector points[cNumObjectSelectPoints];

            float radius = pProtoObject->getSelectedRadiusX();
            float radiansPerSegment = cTwoPi / cNumObjectSelectPoints;
            float angularOffset = 0.0f;
            for (long segment = 0; segment < cNumObjectSelectPoints; ++segment)
            {
               BVector& pt=points[segment];
               float sinAngle = (float)sin(angularOffset);
               float cosAngle = (float)cos(angularOffset);
               pt.set(sinAngle * radius + position.x, position.y, cosAngle * radius + position.z);
               angularOffset += radiansPerSegment;
            }

            float thickness = 0.1f;
            for(long i=0; i<cNumObjectSelectPoints; i++)
               gTerrainSimRep.addDebugThickLineOverTerrain(points[i], (i==cNumObjectSelectPoints-1?points[0]:points[i+1]), thickness, color, color, 0.1f);
         }         
      }
   }
   
   // Draw weapon range circles
//   if(drawRanges && pProtoObject->getFlagShowRange())
   const BUnit* pUnit = pObject->getUnit();
   if(mWeaponRangeDisplayEnabled && pUnit)
   {
      long squadMode=BSquadAI::cModeNormal;
//-- FIXING PREFIX BUG ID 5711
      const BSquad* pSquad=gWorld->getSquad(pObject->getParentID());
//--
      if(pSquad)
         squadMode=pSquad->getSquadMode();
      BTactic* pTactic=pObject->getTactic();
      if(pTactic)
      {
         for(long i=0; i<pTactic->getNumberTargetRules(); i++)
         {
            const BTargetRule* pRule=pTactic->getTargetRule(i);
//-- FIXING PREFIX BUG ID 5710
            const BProtoAction* pAction=pTactic->getProtoAction(pRule->mActionID);
//--
            if(pAction)
            {
               long actionType=pAction->getActionType();
               if(actionType==BAction::cActionTypeUnitRangedAttack)
               {
                  if(pRule->mSquadMode==squadMode || squadMode==BSquadAI::cModeNormal)
                  {
                     float range=pAction->getMaxRange(pUnit);
                     DWORD color=(pRule->mSquadMode==squadMode ? cDWORDGreen : cDWORDBlue);
                     if(pProtoObject->getFlagFlying())
                     {
                        float thickness = 0.04f; // Use smaller thickness for flying units since they are closer to the camera
                        gpDebugPrimitives->addDebugThickCircle(matrix, range, thickness, color);
                     }
                     else
                     {
                        const long cNumObjectSelectPoints=48;
                        BVector points[cNumObjectSelectPoints];
                        float radiansPerSegment = cTwoPi / cNumObjectSelectPoints;
                        float angularOffset = 0.0f;
                        for (long segment = 0; segment < cNumObjectSelectPoints; ++segment)
                        {
                           BVector& pt=points[segment];
                           float sinAngle = (float)sin(angularOffset);
                           float cosAngle = (float)cos(angularOffset);
                           pt.set(sinAngle * range + position.x, position.y, cosAngle * range + position.z);
                           angularOffset += radiansPerSegment;
                        }
                        float thickness = 0.2f;
                        for(long i=0; i<cNumObjectSelectPoints; i++)
                           gTerrainSimRep.addDebugThickLineOverTerrain(points[i], (i==cNumObjectSelectPoints-1?points[0]:points[i+1]), thickness, color, color, 0.1f);
                     }
                  }
               }
            }
         }
      }
      // Now draw LOS
      float range = pProtoObject->getProtoLOS();
      const long cNumObjectSelectPoints=48;
      BVector points[cNumObjectSelectPoints];
      float radiansPerSegment = cTwoPi / cNumObjectSelectPoints;
      float angularOffset = 0.0f;
      for (long segment = 0; segment < cNumObjectSelectPoints; ++segment)
      {
         BVector& pt=points[segment];
         float sinAngle = (float)sin(angularOffset);
         float cosAngle = (float)cos(angularOffset);
         pt.set(sinAngle * range + position.x, position.y, cosAngle * range + position.z);
         angularOffset += radiansPerSegment;
      }
      float thickness = 0.2f;
      DWORD color=cDWORDYellow;
      for(long i=0; i<cNumObjectSelectPoints; i++)
         gTerrainSimRep.addDebugThickLineOverTerrain(points[i], (i==cNumObjectSelectPoints-1?points[0]:points[i+1]), thickness, color, color, 0.1f);

   }
}

//==============================================================================
// BUser::renderVisualInstances
//==============================================================================
void BUser::renderVisualInstances()
{
   if (!getFlagGameActive() || gWorld->isPlayingCinematic())
      return;

   // The renderer wants us to render any granny instances
   // so they can be queued up by the instance manager.
   if(mpHoverVisual && getFlagHaveHoverPoint())
   {
      BMatrix matrix;
      matrix.makeTranslate(mHoverPoint);
      gRender.setWorldMatrix(matrix);

      BVisualRenderAttributes renderAttributes;

      BVector min=mpHoverVisual->getMinCorner();
      BVector max=mpHoverVisual->getMaxCorner();
      renderAttributes.setBounds(min+mHoverPoint, max+mHoverPoint);

      renderAttributes.mTintColor = 0xFF000000;
      renderAttributes.mPixelXFormColor = gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextObjects);

      mpHoverVisual->render(&renderAttributes);
   }
}

//==============================================================================
// BUser::updateViewportUI
//==============================================================================
DWORD cCommandFlashTime = 500;
void BUser::updateViewportUI()
{
   SCOPEDSAMPLE(BUser_updateViewportUI);
   
   if (!getFlagGameActive() || gWorld->isPlayingCinematic())
      return;
      
   mSceneVolumeCuller = gRenderDraw.getMainSceneVolumeCuller();

   renderPlayableBorder();

   renderCameraBoundaryLines();

   bool debugSelectionPicking=gConfig.isDefined(cConfigDebugSelectionPicking);
   long simDebugValue=0;
   #ifndef BUILD_FINAL
   gConfig.get(cConfigRenderSimDebug, &simDebugValue);
   #endif;

   if(!debugSelectionPicking)
   {
      if (simDebugValue == 0)
      {
         // Object selection circles
         if (!gConfig.isDefined(cConfigFlashGameUI) || mWeaponRangeDisplayEnabled)
         {
            int subSelectGroupHandle=(mSelectionManager->getNumSubSelectGroups()>1 ? mSelectionManager->getSubSelectGroupHandle() : -1);
            const BEntityIDArray &ugIDs = mSelectionManager->getSelectedSquads();
            bool firstUnit=true;
            long count=mSelectionManager->getNumberSelectedUnits();
            for(long i=0; i<count; i++)
            {
               const BObject* pObject=gWorld->getObjectManager()->getUnit(mSelectionManager->getSelected(i));
               if(!pObject)
                  continue;
               bool drawRanges=false;
               if(ugIDs.getNumber()==1 && firstUnit)
               {
                  drawRanges=true;
                  firstUnit=false;
               }
               DWORD color=gWorld->getPlayerColor(pObject->getColorPlayerID(), BWorld::cPlayerColorContextSelection);
               if (subSelectGroupHandle!=-1)
               {
                  BEntityID squadID=pObject->getParentID();
                  if(subSelectGroupHandle!=-1 && squadID!=cInvalidObjectID && !mSelectionManager->isSubSelected(squadID))
                  {
                     float alpha=0.6f;
                     gConfig.get(cConfigSubSelectDecalAlpha, &alpha);
                     BColor temp(color);
                     color=packRGBA(temp, alpha);
                  }
               }
               drawObjectSelectedShape(pObject, color, false, drawRanges);
            }
         }

         if (simDebugValue == 0)
         {
            int subSelectGroupHandle=(mSelectionManager->getNumSubSelectGroups()>1 ? mSelectionManager->getSubSelectGroupHandle() : -1);

            //-- Render flash if turned on
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- only draw selection decals if we are not in the command menus
               if ((mUserMode != cUserModeCommandMenu))
               {
	               const BEntityIDArray &ugIDs = mSelectionManager->getSelectedSquads();
	               BSquad* pSquad = NULL;
	               for(long j=0; j<ugIDs.getNumber(); j++)
	               {
	                  BEntityID squadID=ugIDs[j];
	                  pSquad = gWorld->getSquad(squadID);
	                  if (pSquad && pSquad->isVisible(mTeamID))
	                  {
                        DWORD color=gWorld->getPlayerColor(pSquad->getColorPlayerID(), BWorld::cPlayerColorContextSelection);
	                     if (subSelectGroupHandle!=-1 && !mSelectionManager->isSubSelected(squadID))
	                     {
	                        float alpha=0.6f;
	                        gConfig.get(cConfigSubSelectDecalAlpha, &alpha);
	                        BColor temp(color);
	                        color=packRGBA(temp, alpha);
	                     }
	                     float intensity=6.0f;
	                     gConfig.get(cConfigBaseDecalIntensity, &intensity);
	                     pSquad->drawSquadSelection(color, false, intensity, false, false);
	                  }
	               }

                  if (gConfig.isDefined(cConfigShowCommands))
                     renderCommandSquads();
               }
            }
         }
      }
   }
   else
   {
//-- FIXING PREFIX BUG ID 5713
      const BObject* pObject=gWorld->getObject(mHoverObject);
//--
      if(pObject)
      {
         //drawObjectSelectedShape(pObject, cDWORDOrange, true, false);
         pObject->getSimBoundingBox()->draw(cDWORDOrange);
      }
   }

   if(getFlagHaveHoverPoint())
   {
      if(!debugSelectionPicking)
      {
         // Circle select area        
         long debugCircleSelect = 0;
         gConfig.get(cConfigDebugCircleSelect, &debugCircleSelect);
         if(getFlagCircleSelectPoints())
         {
            if (!gConfig.isDefined(cConfigFlashGameUI) || debugCircleSelect != 0)
            {
               float thickness = 0.1f;
               for(long i=0; i<cNumCircleSelectPoints; i++)
                  gpDebugPrimitives->addDebugThickLine(mCircleSelectPoints[i], (i==cNumCircleSelectPoints-1?mCircleSelectPoints[0]:mCircleSelectPoints[i+1]), thickness, cColorSelectionCircle, cColorSelectionCircle);
            }                        
         }  

         if(getFlagCircleSelectPoints() || getFlagCircleSelectShrink() || getFlagCircleSelectGrow())
         {
            if (gConfig.isDefined(cConfigFlashGameUI) && debugCircleSelect != 2)
            {
               float sizeFactor = 1.0f;
               float intensity = 1.0f;
               float offset = 0.0f;
               gConfig.get(cConfigCircleSelectSizeFactor, &sizeFactor);
               gConfig.get(cConfigCircleSelectIntensity, &intensity);
               gConfig.get(cConfigCircleSelectOffset, &offset);
               
               BColor decalColor(cColorSelectionCircle);
               DWORD  color = packRGBA(decalColor, mCircleSelectOpacity);
               drawSelectionCircleDecal(mHoverPoint, color, mCircleSelectSize*sizeFactor, intensity, offset);
            }
         }
      }

      // Hover object highlight
      if (mHoverObject != cInvalidObjectID && mUserMode!=cUserModeBuildLocation)
      {
         BObject* pObject=gWorld->getObject(mHoverObject);
         if(pObject)
         {
            DWORD hoverColor=cColorHoverNone;
            hoverColor=gWorld->getPlayerColor(pObject->getColorPlayerID(), BWorld::cPlayerColorContextSelection);
            bool drawn=false;
//-- FIXING PREFIX BUG ID 5707
            const BUnit* pUnit=gWorld->getUnit(mHoverObject);
//--
            if (!pUnit)
            {
//-- FIXING PREFIX BUG ID 5714
               const BDopple* pDopple = gWorld->getDopple(mHoverObject);
//--
               if (pDopple)
                  pUnit = gWorld->getUnit(pDopple->getParentID());
            }
            if(pUnit)
            {
               BSquad* pSquad=gWorld->getSquad(pUnit->getParentID());
               if(pSquad)
               {
                  drawn=true;

                  if (mSelectionManager->isSubSelected(pSquad->getID()))
                  {
                     float alpha=0.6f;
                     gConfig.get(cConfigSubSelectDecalAlpha, &alpha);
                     BColor temp(hoverColor);
                     hoverColor=packRGBA(temp, alpha);
                  }

                  //If the menu is not up, set the highlighted squad to pulsing for selection clarity
                  if (!gConfig.isDefined(cConfigNoSelectionHighlight) && 
                      mUserMode!=cUserModeCommandMenu && mUserMode!=cUserModeAbility && mUserMode!=cUserModePowerMenu &&
                      mUserMode != cUserModeInputUILocation && mUserMode != cUserModeInputUILocationMinigame)
                  {
                     for( uint i=0; i<pSquad->getNumberChildren(); i++ )
                     {
                        BEntityID childID = pSquad->getChild(i);
                        BObject *pChildObject = gWorld->getObject( childID );
                        if (pChildObject)
                        {
                           pChildObject->setSelectionPulseTime(0.001f);  //time should be short since we want it to stop as soon as we move away
                        }
                     }
                  }


                  bool isRepairing = pSquad->getFlagIsRepairing();
                  bool isConstructing = false;//!pUnit->getFlagBuilt();
                  bool isCapturing = pUnit->getFlagBeingCaptured();
                  BUnit* pLeaderUnit = pSquad->getLeaderUnit();
                  bool isHibernating = (pLeaderUnit) ? pLeaderUnit->isHibernating() : NULL;

                  bool displayHitpointBar = !isRepairing && !isConstructing && !isCapturing && !isHibernating;

                  if (gConfig.isDefined(cConfigFlashGameUI))
                  {
                     //-- only draw the hover decal if we are not in one of the menus
                     if (mUserMode!=cUserModeCommandMenu && mUserMode!=cUserModeAbility && mUserMode!=cUserModePowerMenu)
                     {
                        bool draw=true;
                        if (gConfig.isDefined(cConfigNoEnemySelectionDecals))
                        {
                           BPlayerID playerID=pSquad->getPlayerID();
                           if (playerID!=mPlayerID && playerID!=mCoopPlayerID)
                              draw=false;
                        }
                        if (draw)
                        {
	                        float intensity=18.0f;
	                        gConfig.get(cConfigSelectionDecalIntensity, &intensity);
                           if (pSquad->isVisible(mTeamID))
	                           pSquad->drawSquadSelection(hoverColor, true, intensity, true, false);
                        }
                     }

                     int unitCount = pSquad->getNumberChildren();
                     
                     const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
                     if (pProtoSquad)
                        pSquad->setFlagForceDisplayHP(displayHitpointBar);
                     else
                     {
                        for (int i = 0; i < unitCount; i++)
                        {
                           BUnit* pChild=gWorld->getObjectManager()->getUnit(pSquad->getChild(i));
                           if(!pChild)
                              continue;
                           pChild->setFlagForceDisplayHP(displayHitpointBar);
                        }
                     }                                         
                  }
                  else 
                  {
                     const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
                     if (pProtoSquad)
                        pSquad->setFlagForceDisplayHP(displayHitpointBar);

                     else
                     {
                        for(uint i=0; i<pSquad->getNumberChildren(); i++)
                        {
                           BUnit* pChild=gWorld->getObjectManager()->getUnit(pSquad->getChild(i));
                           if(!pChild)
                              continue;
                           drawObjectSelectedShape(pChild, hoverColor, false, false);
                           pChild->setFlagForceDisplayHP(displayHitpointBar);
                        }
                     }
                  }
               }
            }
            if(!drawn)
            {
               drawObjectSelectedShape(pObject, hoverColor, false, false);
               BUnit* pUnit = pObject->getUnit();
               if (pUnit)
                  pUnit->setFlagForceDisplayHP(true);
            }
         }
      }
   }

   if(mUserMode==cUserModeBuildLocation)
   {
      if(getFlagHaveHoverPoint())
      {
         BVector forward = cZAxisVector;
         if (mCameraYaw != mBuildBaseYaw)
         {
            float angle = Math::fDegToRad(mCameraYaw - mBuildBaseYaw);
            forward.rotateXZ(angle);
            forward.normalize();
         }

         const DWORD flags = BWorld::cCPRender | BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain | BWorld::cCPSetPlacementSuggestion;
         BPlayerID playerID=(mCoopPlayerID==cInvalidPlayerID ? mPlayerID : mCoopPlayerID);
         const BUnit* pBuilder=gWorld->getUnit(mCommandObject);
         mPlacementSuggestion  = XMVectorReplicate(-1.0f);
         gWorld->checkPlacement(mBuildProtoID, playerID, mHoverPoint, mPlacementSuggestion, forward, BWorld::cCPLOSDontCare, flags, 1, pBuilder, &mPlacementSocketID);

//-- FIXING PREFIX BUG ID 5717
         const BPlayer* pPlayer = getPlayer();
//--
         const BProtoObject* pBuildProto = pPlayer ? pPlayer->getProtoObject(mBuildProtoID) : gDatabase.getGenericProtoObject(mBuildProtoID);
         if (pBuildProto && pBuildProto->getNumberChildObjects() > 0)
         {
            BVector right;
            right.assignCrossProduct(cYAxisVector, forward);
            right.normalize();

            BMatrix mat;
            mat.makeOrient(forward, cYAxisVector, right);

            BSmallDynamicSimArray<BChildObjectData> childObjects;
            if (gWorld->getChildObjects(playerID, mBuildProtoID, mPlacementSuggestion, forward, childObjects))
            {
               int count=childObjects.getNumber();
               for (int i=0; i<count; i++)
               {
//-- FIXING PREFIX BUG ID 5716
                  const BChildObjectData& childData = childObjects[i];
//--
                  const BProtoObject* pSocketProto = pPlayer ? pPlayer->getProtoObject(childData.mObjectType) : gDatabase.getGenericProtoObject(childData.mObjectType);
                  if (!pSocketProto)
                     continue;
                  float sizeX = pSocketProto->getObstructionRadiusX();
                  float sizeZ = pSocketProto->getObstructionRadiusZ();
                  if (sizeX < 2.0f)
                     sizeX = 2.0f;
                  if (sizeZ < 2.0f)
                     sizeZ = 2.0f;

                  float thickness=0.1f;

                  mat.setTranslation(childData.mPosition);

                  BVector p1, p2, p3, p4;
                  mat.transformVectorAsPoint(BVector(-sizeX, 0.0f, -sizeZ), p1);
                  mat.transformVectorAsPoint(BVector( sizeX, 0.0f, -sizeZ), p2);
                  mat.transformVectorAsPoint(BVector( sizeX, 0.0f,  sizeZ), p3);
                  mat.transformVectorAsPoint(BVector(-sizeX, 0.0f,  sizeZ), p4);

                 gTerrainSimRep.addDebugThickSquareOverTerrain(p1, p2, p3, p4, thickness, cDWORDGreen, 0.01f);

                 if (i == 0)
                 {
                    float sizeX2 = sizeX * 0.5f;
                    float sizeZ2 = sizeZ * 0.5f;
                    mat.transformVectorAsPoint(BVector(-sizeX2, 0.0f, -sizeZ2), p1);
                    mat.transformVectorAsPoint(BVector( sizeX2, 0.0f, -sizeZ2), p2);
                    mat.transformVectorAsPoint(BVector( sizeX2, 0.0f,  sizeZ2), p3);
                    mat.transformVectorAsPoint(BVector(-sizeX2, 0.0f,  sizeZ2), p4);

                    gTerrainSimRep.addDebugThickSquareOverTerrain(p1, p2, p3, p4, thickness, cDWORDGreen, 0.01f);
                 }
               }
            }
         }
      }
   }
   else if (mUserMode == cUserModeAbility)
   {
      // donothing.
   }
   else if (mUserMode == cUserModePower)
   {
      if (mpPowerUser)
         mpPowerUser->updateUI();
   }
   else if ((mUserMode == cUserModeInputUILocation) || (mUserMode == cUserModeInputUILocationMinigame))
   {
      DWORD flags = 0;
      if (mUILocationCheckObstruction)
      {
         flags = BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain;
         if (mUILocationCheckMoving)
            flags &= ~BWorld::cCPIgnoreMoving;
      }

      if (mUILocationLOSCenterOnly)
      {
         flags |= BWorld::cCPLOSCenterOnly;
      }

      if (mBuildProtoID == -1)
      {
         flags |= BWorld::cCPIgnorePlacementRules;
      }

      if (mUILocationPlacementRuleIndex != -1)
      {
         flags &= ~BWorld::cCPIgnorePlacementRules;
      }

      long searchScale = 1;
      if (mUILocationPlacementSuggestion)
      {
         flags |= BWorld::cCPSetPlacementSuggestion;

         // Calc search scale based on power radius
         // MPB [3/14/2008] - This search scale really needs to take the obstruction size into account
         // because it is mulitpled by an obs size based tile count inside checkPlacement.  Better yet,
         // checkPlacement should probably just calc searchSize differently
         if (mUIPowerRadius > 0.0f)
            searchScale = Math::Max(1, Math::FloatToIntTrunc(mUIPowerRadius * gTerrainSimRep.getReciprocalDataTileScale()));
      }

      if (getFlagHaveHoverPoint())
      {
         mPlacementSuggestion = XMVectorReplicate(-1.0f);
         bool result = gWorld->checkPlacement(mBuildProtoID,  mPlayerID, mHoverPoint, mPlacementSuggestion, cZAxisVector, mUILocationLOSType, flags, searchScale, NULL, NULL, mUILocationPlacementRuleIndex, mBuildProtoSquadID, mBuildUseSquadObstruction);
         setFlagUILocationValid(result);
      }

      // Power specific rendering
//-- FIXING PREFIX BUG ID 5719
      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mUIProtoPowerID);
//--
      if (pProtoPower)
      {
         if (!gConfig.isDefined(cConfigNoSelectionHighlight) && pProtoPower->getFlagShowTargetHighlight() && pProtoPower->getTargetObjectType() != -1 && mUIPowerRadius > 0.0f && getFlagHaveHoverPoint())
         {
            // Highlight potential targets
            BEntityIDArray workingList(0,50);
            BUnitQuery query(mHoverPoint, mUIPowerRadius, true);
            query.addObjectTypeFilter(pProtoPower->getTargetObjectType());
            query.setFlagIgnoreDead(true);
            query.setRelation(getPlayerID(), pProtoPower->getTargetRelation());
            gWorld->getSquadsInArea(&query, &workingList);

            long numInWorkingList = workingList.getNumber();
            for (long i=0; i<numInWorkingList; i++)
            {
               BEntityID squadID = workingList[i];
               BSquad *pSquad = gWorld->getSquad(squadID);
               if (pSquad)
               {
                  for (uint j = 0; j < pSquad->getNumberChildren(); j++)
                  {
                     BObject* pObject = gWorld->getObject(pSquad->getChild(j));
                     if (pObject)
                     {
                        pObject->setSelectionPulseTime(0.001f, 2.0f);  //time should be short since we want it to stop as soon as we move away
                     }
                  }
               }
            }
         }
      }

      // Minigame rendering
      if (mUserMode == cUserModeInputUILocationMinigame)
         mMinigame.render();
   }
   else if (mUserMode == cUserModeInputUIUnit)
   {
      BUnit *pUnit = gWorld->getUnit(mHoverObject);
      bool unitIsValid = testUnitAgainstInputUIModeFilters(pUnit);
      setFlagUIUnitValid(unitIsValid);
   }
   else if (mUserMode == cUserModeInputUISquad)
   {
      BUnit *pUnit = gWorld->getUnit(mHoverObject);
      BSquad *pSquad = (pUnit) ? pUnit->getParentSquad() : NULL;
      bool squadIsValid = testSquadAgainstInputUIModeFilters(pSquad);
      setFlagUISquadValid(squadIsValid);
   }
   else if (mUserMode == cUserModeInputUISquadList)
   {      
      if (getFlagHaveHoverPoint())
      {         
         bool valid = (mSelectionManager->getNumberSelectedSquads() > 0);
         if (mUIPowerRadius > 0.0f)
         {
            BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mPowerDecal);
            if (pDecalAttributes)
               pDecalAttributes->setColor(valid ? cDWORDGreen : cDWORDRed);
         }
         setFlagUISquadValid(valid);

//-- FIXING PREFIX BUG ID 5722
         const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mUIProtoPowerID);
//--
         if (pProtoPower && pProtoPower->getFlagShowTransportArrows())
         {
            BMatrix matrix;
            BVector right = mpCamera->getCameraRight();
            BVector forward;
            forward.assignCrossProduct(right, cYAxisVector);
            forward.normalize();
            matrix.makeOrient(cYAxisVector, forward, right);
            static float cArrowOffset = 10.0f;
            matrix.multTranslate(mHoverPoint.x, mHoverPoint.y + cArrowOffset, mHoverPoint.z);
            gpDebugPrimitives->addDebugArrow(matrix, BVector(2.0f, 3.0f, 2.0f), valid ? cDWORDGreen : cDWORDRed);
         }
      }
   }
   else if (mUserMode == cUserModeInputUIPlaceSquadList)
   {
      bool allValid = false;
      uint numSquads = mInputUIModeSquadList.getSize();
      if (numSquads > 0 && getFlagHaveHoverPoint() && getFlagHoverPointOverTerrain())
      {  
         // Get the proto object with the largest radius.
         int useProtoObjectID = -1;
         float obstructionRadius = 0.0f;
         for (uint i = 0; i < numSquads; i++)
         {
//-- FIXING PREFIX BUG ID 5723
            const BSquad* pSquad = gWorld->getSquad(mInputUIModeSquadList[i]);
//--
            if (pSquad)
            {
               const BProtoObject* pProtoObject = pSquad->getProtoObject();
               if (pProtoObject && pProtoObject->getObstructionRadius() > obstructionRadius)
               {
                  useProtoObjectID = pProtoObject->getID();
                  obstructionRadius = pProtoObject->getObstructionRadius();
               }
            }
         }

         // Validate hover point
         //DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain | BWorld::cCPIgnoreNonSolid | BWorld::cCPIgnorePlacementRules;
         //BVector placementSuggestion = cInvalidVector;
         //if (gWorld->checkPlacement(useProtoObjectID, mPlayerID, mHoverPoint, placementSuggestion, cZAxisVector, mUILocationLOSType, flags))
         {
            // Plot squad locations      
            BDynamicSimVectorArray waypoints;
            waypoints.add(mHoverPoint);
            gSquadPlotter.plotSquads(mInputUIModeSquadList, mPlayerID, mHoverObject, waypoints, mHoverPoint, -1, BSquadPlotter::cSPFlagForceLandMovementType | BSquadPlotter::cSPFlagNoSync);
            const BDynamicSimArray<BSquadPlotterResult>& dropOffPlotterResults = gSquadPlotter.getResults();      
            uint numDropOffPlotterResults = dropOffPlotterResults.getSize();
            
            // Check and show squad locations
            if (numDropOffPlotterResults == numSquads)
            {
               allValid = true;
               for (uint i = 0; i < numSquads; i++)
               {
//-- FIXING PREFIX BUG ID 5725
                  const BSquad* pSquad = gWorld->getSquad(mInputUIModeSquadList[i]);
//--
                  if (pSquad)
                  {          
                     BVector dropOffPlotPosition = dropOffPlotterResults[i].getDesiredPosition();

                     // Check squad location
                     DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain;
                     int protoObjectID = pSquad->getProtoObjectID();               
                     if (protoObjectID == -1)
                        flags |= (BWorld::cCPIgnorePlacementRules | BWorld::cCPIgnoreNonSolid);
                     else
                     {
//-- FIXING PREFIX BUG ID 5724
                        const BProtoObject* pProtoObject = getPlayer()->getProtoObject(protoObjectID);
//--
                        if (pProtoObject && !pProtoObject->isType(gDatabase.getOTIDBuilding()))
                           flags |= BWorld::cCPIgnoreNonSolid;
                     }
                     BVector placementSuggestion = cInvalidVector;
                     bool validPlacement = gWorld->checkPlacement(protoObjectID, mPlayerID, dropOffPlotPosition, placementSuggestion, cZAxisVector, mUILocationLOSType, flags);
                     if (!validPlacement)
                        allValid = false;

                     /*
                     // Show squad locations
                     const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
                     if (pProtoSquad)
                     {
                        int decalID = pProtoSquad->getSelectionDecalID();
                        if (decalID != -1 && gUIManager)
                        {
                           int flashMovieIndex = gUIManager->getDecalFlashMovieIndex(decalID);
                           if (flashMovieIndex != -1)
                           {
                              BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(pSquad->getSelectionDecal());
                              if (pDecalAttributes)
                              {
                                 BPlayer* pPlayer = pSquad->getPlayer();
                                 BCiv* pCiv = pPlayer ? pPlayer->getCiv() : NULL;
                                 const float expansionRadius = pCiv ? pCiv->getHullExpansionRadius() : 0.0f;
                                 BVector forward = cZAxisVector;
                                 BVector right = cXAxisVector;   
                                 float sizeX = pSquad->getObstructionRadiusX() + expansionRadius;
                                 float yOffset = 0.0f;
                                 float sizeZ = pSquad->getObstructionRadiusZ() + expansionRadius;
                                 float intensity = 3.0f;

                                 pDecalAttributes->setForward(BVec3(forward.x,forward.y,forward.z));      
                                 pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
                                 pDecalAttributes->setEnabled(true);
                                 pDecalAttributes->setIntensity(intensity);
                                 pDecalAttributes->setRenderOneFrame(true);   
                                 pDecalAttributes->setColor(validPlacement ? cDWORDGreen : cDWORDRed);   
                                 pDecalAttributes->setSizeX(sizeX);
                                 pDecalAttributes->setSizeZ(sizeZ);
                                 pDecalAttributes->setYOffset(yOffset);         
                                 pDecalAttributes->setFlashMovieIndex(flashMovieIndex);
                                 pDecalAttributes->setConformToTerrain(true);      
                                 pDecalAttributes->setPos(BVec3(dropOffPlotPosition.x, dropOffPlotPosition.y, dropOffPlotPosition.z));
                              }
                           }
                        }
                     }
                     */
                  }
               }
            }
         }
      }

      setFlagUILocationValid(allValid);

      if (mUIPowerRadius != 0.0f)
      {
         float thickness = 0.1f;
         gTerrainSimRep.addDebugThickCircleOverTerrain(mHoverPoint, mUIPowerRadius, thickness, (allValid ? cDWORDGreen : cDWORDRed), 0.1f);
      }

//-- FIXING PREFIX BUG ID 5726
      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mUIProtoPowerID);
//--
      if (pProtoPower && pProtoPower->getFlagShowTransportArrows())
      {
         BMatrix matrix;
         BVector right = -mpCamera->getCameraRight();
         BVector forward;
         forward.assignCrossProduct(right, -cYAxisVector);
         forward.normalize();
         matrix.makeOrient(-cYAxisVector, forward, right);
         static float cArrowOffset = 10.0f;
         matrix.multTranslate(mHoverPoint.x, mHoverPoint.y + cArrowOffset, mHoverPoint.z);
         gpDebugPrimitives->addDebugArrow(matrix, BVector(2.0f, 3.0f, 2.0f), allValid ? cDWORDGreen : cDWORDRed);
      }
   }

   if(debugSelectionPicking)
   {
      if(!getFlagCircleSelectPoints())
      {
         float clickSize=20.0f;
         gConfig.get(cConfigCircleSelectClickSize, &clickSize);
         mSelectionManager->populateSelectionsRect(mpCamera->getCameraLoc(), mHoverPoint, clickSize*0.5f, clickSize*0.5f, true, true);
         if(mSelectionManager->getPossibleSelectionCount()>0)
         {
            BEntityID id=mSelectionManager->getPossibleSelection(0);
            if(id!=mHoverObject)
            {
               mHoverObject=id;
               mHoverType=cHoverTypeSelect;       
            }
         }
      }
      else
      {
         mSelectionManager->populateSelectionsCircle(mpCamera->getCameraLoc(), mCircleSelectPoints, cNumCircleSelectPoints, 0.0f, true);
      }
   }
}

//==============================================================================
// BUser::preRenderViewportUI
//==============================================================================
void BUser::preRenderViewportUI()
{
   SCOPEDSAMPLE(BUser_preRenderViewportUI);
   
   if(!getFlagGameActive())
      return;

   bool primaryUserRender = (!gGame.isSplitScreen() || this == gUserManager.getPrimaryUser());

   if (primaryUserRender)
      renderEntityIDs();

}

//==============================================================================
// BUser::renderFrameUI
//==============================================================================
void BUser::renderFrameUI()
{
   if(!getFlagGameActive())
      return;

   if (mpUIContext)
   {
      mpUIContext->renderBegin();
      mpUIContext->render();
      mpUIContext->renderEnd();
   }   
}

//==============================================================================
// BUser::renderUI
//==============================================================================
void BUser::renderViewportUI()
{
   SCOPEDSAMPLE(BUser_renderUI);
   
   if(!getFlagGameActive())
      return;
   
#ifndef BUILD_FINAL
   bool showingDebugStats = BDisplayStats::isDisplaying();
   if (!showingDebugStats)
      renderPlayerStats();
#endif

   renderSelectedUnitIcons();

   bool primaryUserRender = (!gGame.isSplitScreen() || this == gUserManager.getPrimaryUser());

   if (primaryUserRender)
   {
      if(!gConfig.isDefined(cConfigNoHelpUI))
         mHelpUI.render(0, 0);

      // - This is going away.
      // renderPlayers();
#ifndef BUILD_FINAL
      gAchievementDispayManager.render();
#endif
      renderGameStateMessages();
      renderUserMessages();
   }

#ifndef BUILD_FINAL
   if (primaryUserRender)
   {
      // AJL 9/28/07 - Temp code to indicate minimap is jammed
      if (getPlayer()->getFlagMinimapBlocked())
      {
// LOC - This doesn't draw in final
         gFontManager.drawText(gFontManager.getFontDenmark14(), 130.0f, 130.0f, "JAMMED!");
      }
   }
#endif

   static bool bDisplayOldUI = true;

   
      
   switch(mUserMode)
   {
      case cUserModeNormal:
      case cUserModeFollow:
      {
         long reticleType=BUIGame::cReticleNormal;
         uint attackRating=0;
         BCost cost;

         if (mSelectionManager->getNumberSelectedUnits()>0)
         {
            const BEntityIDArray& units=mSelectionManager->getSelectedUnits();
            for (uint i=0; i<units.getSize(); i++)
            {
//-- FIXING PREFIX BUG ID 5720
               const BUnit* pUnit=gWorld->getUnit(units[i]);
//--
               if (pUnit)
               {
                  if (pUnit->getPlayerID()==mPlayerID)
                  {
                     int powerID = pUnit->getProtoObject()->getProtoPowerID();
                     if (powerID != -1)
                     {
//-- FIXING PREFIX BUG ID 5730
                        const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(powerID);
//--
                        if (pProtoPower && pProtoPower->getFlagUnitPower())
                        {
                           reticleType = BUIGame::cReticlePowerInvalid;

                           if (mFlagHoverPointOverTerrain)
                           {
//-- FIXING PREFIX BUG ID 5729
                              const BSquad* pSquad = pUnit->getParentSquad();
//--
                              if (pSquad)
                              {
                                 if (pUnit->getPlayer()->canUsePower(powerID, pSquad->getID()))
                                 {
                                    bool unitAvail = true;
                                    int actionType = pProtoPower->getActionType();
                                    if (actionType == BAction::cActionTypeSquadCarpetBomb)
                                    {
//-- FIXING PREFIX BUG ID 5728
                                       const BUnitActionMoveAir* pAirMoveAction = reinterpret_cast<const BUnitActionMoveAir*>(pUnit->getActionByTypeConst(BAction::cActionTypeUnitMoveAir));
//--
                                       if (pAirMoveAction && pAirMoveAction->getState()!=BAction::cStateNone)
                                          unitAvail= false;
                                    }
                                    if (unitAvail)
                                       reticleType = BUIGame::cReticlePowerValid;
                                 }
                              }
                           }
                        }
                     }
                  }
                  break;
               }
            }
         }

         if (reticleType == BUIGame::cReticleNormal)
         {
            if (!mFlagHoverPointOverTerrain)
            {
               // ajl 8/13/07 fixme - need custom reticle type for this
               reticleType=BUIGame::cReticlePowerInvalid;
            }
            else
            {
               switch(mHoverType)
               {
                  case cHoverTypeEnemy: 
                     {
                        reticleType=BUIGame::cReticleAttack; 
                        attackRating=mHoverTypeAttackRating;

                        // BEK -- Fixes bug PHX-8162
                        // if the "enemy" is my own unit because I have an enemy unit selected and I am now hovering over my own unit
                        // show the normal cursor
                        BUser* pPrimaryUser = gUserManager.getPrimaryUser();
                        if (pPrimaryUser && (mPlayerID == pPrimaryUser->getPlayerID()))
                        {
                           BUnit* pCurHoverObject = gWorld->getUnit(mHoverObject);
                           // PAC -- Fixes bug PHX-16767
                           // changing the check from PlayerID to TeamID, so that it works for teammates as well.
                           // [11-12-08 CJS] Changing this to check if the unit is being jacked.  If so, don't change the cursor
                           if (pCurHoverObject && (pCurHoverObject->getTeamID() == mTeamID) && !pCurHoverObject->getFlagBeingBoarded())
                              reticleType = BUIGame::cReticleNormal;
                        }
                     }
                     break;
                  case cHoverTypeMinRange:
                     reticleType=BUIGame::cReticlePowerInvalid;
                     break;
                  case cHoverTypeGather: 
                     if(mHoverResource!=-1 && mHoverResource<BUIGame::cGatherReticleCount)
                        reticleType=BUIGame::cFirstGatherReticle+mHoverResource; 
                     break;
                  case cHoverTypeBuild: 
                     reticleType=BUIGame::cReticleCapture;
                     break;
                  case cHoverTypeCapture: 
                  {
                     reticleType=BUIGame::cReticleCapture;
//-- FIXING PREFIX BUG ID 5733
                     const BUnit* pUnit = gWorld->getUnit(mHoverObject);
//--
                     if (pUnit)
                     {
                        bool costPaid = false;
//-- FIXING PREFIX BUG ID 5731
                        const BSquad* pSquad = pUnit->getParentSquad();
//--
                        if (pSquad)
                        {
                           const BEntityRef* pRef = pSquad->getFirstEntityRefByID(BEntityRef::cTypeCapturingPlayer, BEntityID(mPlayerID));
                           if (pRef && pRef->mData2 == 1)
                              costPaid = true;
                        }
                        if (!costPaid)
                        {
                           const BCost* pCost = pUnit->getProtoObject()->getCaptureCost(getPlayer());
                           if (pCost)
                           {
                              cost = *pCost;
                              if (!getPlayer()->checkCost(pCost))
                                 reticleType = BUIGame::cReticlePowerInvalid; // AJL FIXME - NEED CUSTOM "CAN'T AFFORD CAPTURE" RETICLE FOR THIS
                           }
                        }
                     }
                     break;
                  }
                  case cHoverTypeRepair: 
                     reticleType=BUIGame::cReticleRepair;
                     break;
                  case cHoverTypeGarrison: 
                     {
                        reticleType = BUIGame::cReticleTransportFull;
//-- FIXING PREFIX BUG ID 5734
                        const BUnit* pUnit = gWorld->getUnit(mHoverObject);
//--
                        if (pUnit)
                        {
                           BEntityIDArray ugIDs;
                           mSelectionManager->getSubSelectedSquads(ugIDs);
//-- FIXING PREFIX BUG ID 5732
                           const BSquad* pSquad = NULL;
//--
                           for (int i = 0; i < ugIDs.getNumber(); i++)
                           {
                              pSquad = gWorld->getSquad(ugIDs[i]);
                              if (pSquad && pUnit->canContain(pSquad))
                              {
                                 reticleType = BUIGame::cReticleGarrison;
                                 break;
                              }
                           }
                        }
                        break;
                     }
                  case cHoverTypeHitch:
                     reticleType = BUIGame::cReticleHitch;
                     break;
                  case cHoverTypeAbility:
                     reticleType = BUIGame::cReticlePowerValid;
                     break;
                  case cHoverTypeSelect:
                     if (gConfig.isDefined(cConfigBuildingMenuOnSelect))
                     {
                        BUnit* pUnit = gWorld->getUnit(mHoverObject);
                        if (pUnit && pUnit->getSelectType(getTeamID())==cSelectTypeCommand)
                        {
                           bool isPlayer = (pUnit->getPlayerID() == mPlayerID || pUnit->getPlayerID() == mCoopPlayerID);
                           bool isCommandableByAnyPlayer = pUnit->getProtoObject()->getFlagCommandableByAnyPlayer();
                           if (isPlayer || isCommandableByAnyPlayer)
                              reticleType = BUIGame::cReticleBase;
                        }
                     }
                     break;
               }               
            }
         }

         renderReticle(reticleType, attackRating, &cost);
         break;
      }

      case cUserModeCircleSelecting:
         if(!getFlagCircleSelectGrow())
            renderReticle(BUIGame::cReticleNormal);
         break;

      case cUserModeInputUILocation:
      case cUserModeInputUILocationMinigame:
      case cUserModeInputUIPlaceSquadList:
         {
            if (getFlagUILocationValid() && mFlagHoverPointOverTerrain)
            {
               renderReticle(BUIGame::cReticlePowerValid);
               //if (mUIPowerRadius > 0.0f)
                //  gTerrainSimRep.addDebugCircleOverTerrain(mHoverPoint, mUIPowerRadius, cDWORDGreen, 0.1f);
            }
            else
            {
               renderReticle(BUIGame::cReticlePowerInvalid);
               //if (mUIPowerRadius > 0.0f)
                 // gTerrainSimRep.addDebugCircleOverTerrain(mHoverPoint, mUIPowerRadius, cDWORDRed, 0.1f);
            }
            break;
         }

      case cUserModeInputUIUnit:
         {
            if (getFlagUIUnitValid())
               renderReticle(BUIGame::cReticlePowerValid);
            else
               renderReticle(BUIGame::cReticlePowerInvalid);
            break;
         }

      case cUserModeInputUISquad:
         {
            if (getFlagUISquadValid())
               renderReticle(BUIGame::cReticlePowerValid);
            else
               renderReticle(BUIGame::cReticlePowerInvalid);
            break;
         }

      case cUserModeInputUISquadList:
         {
            if (getFlagUISquadValid())
               renderReticle(BUIGame::cReticlePowerValid);
            else
               renderReticle(BUIGame::cReticlePowerInvalid);
            break;
         }

      case cUserModeRallyPoint:
      case cUserModeBuildingRallyPoint:
      case cUserModeBuildLocation:
         if (mFlagHoverPointOverTerrain)
            renderReticle(BUIGame::cReticleNormal);
         else
            renderReticle(BUIGame::cReticlePowerInvalid);
         break;

      case cUserModeAbility:
      {
         bool valid=true;
//-- FIXING PREFIX BUG ID 5735
         const BAbility* pAbility=gDatabase.getAbilityFromID(mAbilityID);
//--
         if(pAbility && pAbility->getTargetType()==BAbility::cTargetUnit)
         {
            if(mHoverType==cHoverTypeNone || mHoverType==cHoverTypeSelect)
               valid=false;
         }
         else
         {
            if(!getFlagHoverPointOverTerrain())
               valid=false;
         }
         if(valid)
            renderReticle(BUIGame::cReticlePowerValid);
         else
            renderReticle(BUIGame::cReticlePowerInvalid);
         break;
      }

      case cUserModePower:
         {
            if (mpPowerUser)
               mpPowerUser->renderUI();
            break;
         }

      case cUserModeCommandMenu:        
      case cUserModePowerMenu:
      {         
         if (!gConfig.isDefined(cConfigFlashGameUI))
            mCircleMenu.render(0, 0);
         else
         {
#ifndef BUILD_FINAL
            renderDebugMenuInfo();
#endif
         }
         break;
      }

      case cUserModeCinematic:
         break;
   }
}

#ifndef BUILD_FINAL
//==============================================================================
// BUser::renderDebugMenuInfo
//==============================================================================
void BUser::renderDebugMenuInfo()
{
   if (!gConfig.isDefined(cConfigCircleDebugData))
      return;

   // Is a menu item selected?
   long currentItemIndex = mCircleMenu.getCurrentItemIndex();
   if(currentItemIndex == -1)
      return;

   BSimString debugText;
   debugText.set("");

   switch (mUserMode)
   {
      case cUserModeCommandMenu:        
      {
         // Get the info from the menu item.
         long currentItemID=mCircleMenu.getCurrentItemID();
         BProtoObjectCommand command;
         command.set(currentItemID);
         long type=command.getType();
         long id=command.getID();

         if (currentItemID==gDatabase.getPPIDRallyPoint())
         {
            debugText = "GlobalRallyPoint";
         }
         else if(type==BProtoObjectCommand::cTypeResearch)
         {
//-- FIXING PREFIX BUG ID 5736
            const BProtoTech* pProtoTech=gWorld->getPlayer(mPlayerID)->getProtoTech(id);
//--
            if (pProtoTech)
               debugText = pProtoTech->getName();
         }
         else if(type==BProtoObjectCommand::cTypeTrainUnit)
         {
            debugText.set(gDatabase.getProtoObjectName(id));
         }
         else if(type==BProtoObjectCommand::cTypeTrainSquad)
         {
            debugText.set(gDatabase.getProtoSquadName(id));
         }
         else if(type==BProtoObjectCommand::cTypeBuild)
         {
            debugText.set(gDatabase.getProtoObjectName(id));
         }
         else if(type==BProtoObjectCommand::cTypeCustomCommand)
         {
//-- FIXING PREFIX BUG ID 5737
            const BCustomCommand* pCustomCommand = gWorld->getCustomCommand(id);
//--
            if (pCustomCommand)
               debugText.set(gDatabase.getLocStringFromIndex(pCustomCommand->mNameStringIndex).getPtr());
         }
         else if(type==BProtoObjectCommand::cTypePower)
         {
//-- FIXING PREFIX BUG ID 5738
            const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(id);
//--
            if (pProtoPower)
               debugText.set(pProtoPower->getName());
         }
         else if(type==BProtoObjectCommand::cTypeBuildOther)
         {
            debugText.set(gDatabase.getProtoObjectName(id));
         }
         else if(type==BProtoObjectCommand::cTypeUnloadUnits)
            debugText="Unload";
         else if(type==BProtoObjectCommand::cTypeTrainLock)
            debugText="TrainLock";
         else if(type==BProtoObjectCommand::cTypeTrainUnlock)
            debugText="TrainUnlock";
         else if(type==BProtoObjectCommand::cTypeRallyPoint)
            debugText="RallyPoint";
         else if(type==BProtoObjectCommand::cTypeClearRallyPoint)
            debugText="ClearRallyPoint";
         break;
      }

      case cUserModePowerMenu:
      {
         long currentItemIndex = mCircleMenu.getCurrentItemIndex();
         if(currentItemIndex != -1)
         {
            long currentItemID=mCircleMenu.getCurrentItemID();
            if (mSubMode==cSubModeSelectPower)
            {
               const BLeaderSupportPower* pSupportPower=getPlayer()->getSupportPower(mSupportPowerIndex);
               if(pSupportPower)
               {
                  int protoPowerID = pSupportPower->mPowers[currentItemID];
//-- FIXING PREFIX BUG ID 5739
                  const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
                  if (pProtoPower)
                     debugText = pProtoPower->getName();
               }
            }
            else if(currentItemID==gDatabase.getPPIDRallyPoint())
            {
               debugText = "GlobalRallyPoint";
            }
            else if(currentItemID==gDatabase.getPPIDRepair())
            {
               debugText = "Repair";
            }
            else if(currentItemID<0)
            {
               // Select Support Power
               uint supportPowerIndex=(-currentItemID)-1;
               if (supportPowerIndex < getPlayer()->getLeader()->getSupportPowers().getSize())
                  debugText = "SelectSupportPower";
            }
            else
            {
               // Power Command
               if(currentItemIndex != -1)
               {
//-- FIXING PREFIX BUG ID 5741
                  const BPowerEntry* pEntry = getPlayer()->findPowerEntry(mCircleMenu.getCurrentItemID());
//--
                  if (pEntry)
                  {
//-- FIXING PREFIX BUG ID 5740
                     const BProtoPower* pPP = gDatabase.getProtoPowerByID(pEntry->mProtoPowerID);
//--
                     if (pPP)
                        debugText = pPP->getName();
                  }
               }
            }
         }
         break;
      }
   }

   // Display the menu item details
   BHandle fontHandle=gFontManager.getFontDenmark14();
   gFontManager.setFont( fontHandle );
   float x = 70.0f;
   float y = 400.0f;
   //float rowSpacing = 26.0f;
// LOC - This doesn't draw in final
   gFontManager.drawText(fontHandle, x, y, debugText.getPtr());
}
#endif

//==============================================================================
// BUser::renderReticle
//==============================================================================
void BUser::renderReticle(long reticleType, uint goodAgainstRating, const BCost* pCost)
{
   //bool primaryUserRender = (!gGame.isSplitScreen() || this == gUserManager.getPrimaryUser());

   long size=gUIGame.getReticleSize();   
   if (!gConfig.isDefined(cConfigFlashGameUI))// || !primaryUserRender)
      size=32; // ajl 8/27/07 - override reticle size for dev UI
   long x, y;
   gUIGame.getViewCenter(this, x, y);
   long x1=x-(size/2);
   long y1=y-(size/2);

   if (pCost && pCost->getTotal() != 0.0f)
   {
      BHandle fontHandle=gUIGame.getPlayerStatFont();
      gFontManager.setFont(fontHandle);
      float lineHeight = gFontManager.getLineHeight();
      float x=(float)(x1+size);
      float y=(float)(y1+(size/2)-(lineHeight/2));
      BSimString text;

//-- FIXING PREFIX BUG ID 5745
      const BPlayer* pPlayer = getPlayer();
//--
      int civID = pPlayer->getCivID();
      int leaderID = pPlayer->getLeaderID();
      long statCount=gUIGame.getNumberPlayerStats(civID);
      for(long i=0; i<statCount; i++)
      {
         const BUIGamePlayerStat* pStat=gUIGame.getPlayerStat(civID, i);
         if(pStat->mLeaderID!=-1 && pStat->mLeaderID!=leaderID)
            continue;
         if(pStat->mType==BUIGamePlayerStat::cTypeResource)
         {
            float amount = pCost->get(pStat->mID);
            if (amount > 0.0f)
            {
               gUI.renderTexture(gUIGame.getResourceHudIcon(pStat->mID), (long)x, (long)y, (long)(x+lineHeight), (long)(y+lineHeight), true, cDWORDWhite);
               x+=lineHeight+4.0f;
               text.locFormat("%.f", amount);
               bool costOkay = (pPlayer->getResource(pStat->mID) >= amount);
// LOC - fixme - does this draw?
               gFontManager.drawText(fontHandle, x, y, text.getPtr(), (costOkay ? cDWORDWhite : cDWORDRed));
               x+=gFontManager.getLineLength(text, text.length())+10.0f;
            }
         }
      }
   }

   if (gConfig.isDefined(cConfigFlashGameUI))
   {  
      setReticleMode(reticleType, goodAgainstRating, pCost);
      mpUIContext->renderReticle();
      return;
   }

   BManagedTextureHandle handle=gUIGame.getReticleTexture(reticleType);      
   gUI.renderTexture(handle, x1, y1, x1+size-1, y1+size-1, true);
}

//==============================================================================
// BUser::setReticleMode
//==============================================================================
void BUser::setReticleMode(long reticleType, uint goodAgainstRating, const BCost* pCost)
{
   if (!gConfig.isDefined(cConfigFlashGameUI))
      return;

   long size=gUIGame.getReticleSize();
   long halfSize = size/2;

   long x, y;
   if (mFlagTargetSelecting)
   {
      const BRenderViewParams& view = gRender.getViewParams();
      view.calculateWorldToScreen(mTargetSelectPos, x, y);
      if (x < halfSize)
         x = halfSize;
      else if (x + halfSize > view.getViewportWidth())
         x = view.getViewportWidth() - halfSize;
      if (y < halfSize)
         y = halfSize;
      else if (y + halfSize > view.getViewportHeight())
         y = view.getViewportHeight() - halfSize;
   }
   else
      gUIGame.getViewCenter(this, x, y);
   
   long x1=x-(size/2);
   long y1=y-(size/2);
   
   mpUIContext->setReticlePosition(x1,y1,size-1,size-1);
   mpUIContext->setReticleMode(reticleType, goodAgainstRating, pCost);
}

//==============================================================================
// BUser::renderPlayerStats
// only in debug
//==============================================================================
void BUser::renderPlayerStats()
{
   if (!getHUDItemEnabled(BUser::cHUDItemResources) || gUIManager->isNonGameUIVisible())
      return;

   BFixedString<128> t;

//-- FIXING PREFIX BUG ID 5746
   const BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
//--
   long civID=pPlayer->getCivID();
   long leaderID=pPlayer->getLeaderID();
   
   // Leader power charges
   if (pPlayer->getHaveLeaderPower())
   {
      float x=(gConfig.isDefined(cConfigFlashGameUI) ? 910.0f : 930.0f);
      float y=(gConfig.isDefined(cConfigFlashGameUI) ? 35.0f : 56.0f);
      uint numCharges=pPlayer->getLeaderPowerChargeCount();
      float percentToNextCharge=pPlayer->getLeaderPowerChargePercent();

      t.format("C: %u/+%d", numCharges, (int)pPlayer->getRate(gDatabase.getLeaderPowerChargeRateID()));
      BHandle fontHandle=(gConfig.isDefined(cConfigFlashGameUI) ? gFontManager.getFontDenmark16() : gUIGame.getPlayerStatFont());
      gFontManager.setFont(fontHandle);
// LOC - This doesn't draw in final
      gFontManager.drawText(fontHandle, x, y, t);
      y+=gFontManager.getLineHeight();

      t.format("(%.f%%)", percentToNextCharge);
      fontHandle=gFontManager.getFontCourier10();
      gFontManager.setFont(fontHandle);
// LOC - This doesn't draw in final
      gFontManager.drawText(fontHandle, x, y, t);
   }

   if (gConfig.isDefined(cConfigFlashGameUI))
      return;

   BHandle fontHandle=gUIGame.getPlayerStatFont();
   gFontManager.setFont(fontHandle);

   long statCount=gUIGame.getNumberPlayerStats(civID);
   for(long i=0; i<statCount; i++)
   {
      const BUIGamePlayerStat* pStat=gUIGame.getPlayerStat(civID, i);

      if(pStat->mLeaderID!=-1 && pStat->mLeaderID!=leaderID)
         continue;

      BUIRect iconRect=pStat->mIconPosition;
      BUIRect valRect=pStat->mValuePosition;
      if (gGame.isSplitScreen())
      {
         if (gGame.isVerticalSplit())
         {
            if (this==gUserManager.getPrimaryUser())
            {
               iconRect=pStat->mIconPositionSV1;
               valRect=pStat->mValuePositionSV1;
            }
            else
            {
               iconRect=pStat->mIconPositionSV2;
               valRect=pStat->mValuePositionSV2;
            }
         }
         else
         {
            if (this==gUserManager.getPrimaryUser())
            {
               iconRect=pStat->mIconPositionSH1;
               valRect=pStat->mValuePositionSH1;
            }
            else
            {
               iconRect=pStat->mIconPositionSH2;
               valRect=pStat->mValuePositionSH2;
            }
         }
      }

      if(pStat->mType==BUIGamePlayerStat::cTypeResource)
      {
         gUI.renderTexture(gUIGame.getResourceHudIcon(pStat->mID), iconRect.mX1, iconRect.mY1, iconRect.mX2, iconRect.mY2, true, cDWORDWhite);
         if (pStat->mID2==-1)
            t.format("%d", (long)pPlayer->getResource(pStat->mID));
         else
            t.format("%d/+%d", (long)pPlayer->getResource(pStat->mID), (long)pPlayer->getRate(pStat->mID2));
// LOC - This doesn't draw in final
         gFontManager.drawText(fontHandle, (float)valRect.mX1, (float)valRect.mY1, t);
      }
      else
      {
         gUI.renderTexture(gUIGame.getPopHudIcon(pStat->mID), iconRect.mX1, iconRect.mY1, iconRect.mX2, iconRect.mY2, true, cDWORDWhite);
         t.format("%.0f/%.0f", pPlayer->getPopCount(pStat->mID)+pPlayer->getPopFuture(pStat->mID), min(pPlayer->getPopCap(pStat->mID), pPlayer->getPopMax(pStat->mID)));
// LOC - This doesn't draw in final
         gFontManager.drawText(fontHandle, (float)valRect.mX1, (float)valRect.mY1, t);
      }
   }
}


//==============================================================================
// BUser::renderGameTime
//==============================================================================
void BUser::renderGameTime()
{
   BHandle fontHandle=gUIGame.getPlayerStatFont();
   gFontManager.setFont(fontHandle);
   //float fh=gFontManager.getLineHeight();

   DWORD s=gWorld->getGametime()/1000;
   DWORD m=s/60;
   s-=m*60;
   DWORD h=m/60;
   m-=h*60;

   BFixedString<128> t;
   float fw;
   if(h>0)
   {
      t.format("%02d:%02d:%02d", h, m, s);
      fw=gFontManager.getLineLength("00:00:00", 8);
   }
   else
   {
      t.format("%02d:%02d", m, s);
      fw=gFontManager.getLineLength("00:00", 5);
   }

   if (gConfig.isDefined(cConfigFlashGameUI))
   {      
      mpUIContext->setGameTime(BStrConv::toA(t));
      return;
   }

#ifndef BUILD_FINAL
// LOC - This doesn't draw in final
   gFontManager.drawText(fontHandle, gUI.mfSafeX2-fw, gUI.mfSafeY1, t);
#endif
}

//==============================================================================
// BUser::renderEntityIDs
// fixme - check this function for drawing text - it's not loc friendly.
//==============================================================================
void BUser::renderEntityIDs()
{
   if( gUIManager->isNonGameUIVisible() )
      return;

   BHandle fontHandle=gFontManager.getFontCourier10();
   gFontManager.setFont(fontHandle);
   const BRenderViewParams& view=gRender.getViewParams();
   BVector objectPos;
   BVector idPos;
   BString sID;
   BEntityHandle handle=cInvalidObjectID;
#ifndef BUILD_FINAL
   if(mShowEntityIDType!=0)
   {
      for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
      {
         if(pUnit->isGarrisoned())
            continue;
         const BBoundingBox* pBoundingBox=pUnit->getVisualBoundingBox();
         if(!isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
            continue;
         objectPos = pUnit->getSimCenter();
         objectPos.y += pUnit->getProtoObject()->getObstructionRadiusY() * 2.0f;
         view.calculateWorldToScreen(objectPos, idPos.x, idPos.y);
         sID.format("%s", pUnit->getID().getDebugString().getPtr());
// LOC - This doesn't draw in final
         gFontManager.drawText(fontHandle, idPos.x, idPos.y, sID.getPtr());
      }
   }
#endif

   //DCP 07/25/07: Hack to show node progress.
   handle=cInvalidObjectID;
   BString text1, text2;
   for (BUnit* pUnit=gWorld->getNextUnit(handle); pUnit != NULL; pUnit=gWorld->getNextUnit(handle))
   {
      if (!pUnit->isVisible(mTeamID))
         continue;

      const BBoundingBox* pBoundingBox=pUnit->getVisualBoundingBox();
      if (!isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
         continue;

      if (pUnit->getProtoObject()->getFlagCapturable())
      {
         if (pUnit->getCapturePlayerID() == cInvalidPlayerID)
         {
            continue;
         }
         else
         {
            if ((pUnit->getCapturePercent() < cFloatCompareEpsilon) ||
               (pUnit->getCapturePercent() >= 1.0f))
               continue;
//-- FIXING PREFIX BUG ID 5747
            const BPlayer* pPlayer=gWorld->getPlayer(pUnit->getCapturePlayerID());
//--
            if (!pPlayer)
               continue;
            //text1.format("%s: %d%%", pPlayer->getName().getPtr(), (long)(pUnit->getCapturePercent()*100.0f));
            //text2.empty();
            BSquad* pSquad = pUnit->getParentSquad();
            if (pSquad)
               gHPBar.displayProgress(pSquad, pUnit->getCapturePercent());
            else
               gHPBar.displayProgress(pUnit, pUnit->getCapturePercent());
         }
      }
      else
         continue;
   }

   /*
   //AJL 09/12/07: Hack to show repair progress, construction progress, veterancy level, and bowling info
   bool showLevel = gConfig.isDefined(cConfigVeterancy) && gScenario.getFlagAllowVeterancy();
   bool showBowling = gConfig.isDefined(cConfigBowling);
   handle=cInvalidObjectID;
   for (BSquad* pSquad=gWorld->getNextSquad(handle); pSquad != NULL; pSquad=gWorld->getNextSquad(handle))
   {
      BUnit* pUnit=pSquad->getLeaderUnit();
      if (!pUnit)
         continue;
      if (!pUnit->isVisible(mTeamID))
         continue;
      bool isRepairing = pSquad->getFlagIsRepairing();
      bool isConstructing = false;//!pUnit->getFlagBuilt();
      bool isBowling = (showBowling && pSquad->getSquadMode() == BSquadAI::cModeHitAndRun);
      bool hasLevel = (showLevel && pSquad->getVetLevel() > 0);
      if (!isRepairing && !isConstructing && !isBowling && !hasLevel)
         continue;
      const BBoundingBox* pBoundingBox=pUnit->getVisualBoundingBox();
      if (!isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
         continue;
      gHPBar.getSquadHPBarPosition(pSquad, objectPos);
      view.calculateWorldToScreen(objectPos, idPos.x, idPos.y);
      idPos.y -= 30.0f;
      DWORD color = gWorld->getPlayerColor(pSquad->getColorPlayerID(), BWorld::cPlayerColorContextSelection);
      if (isConstructing || isRepairing)
      {
         if (isConstructing)
         {
            BUString name;
            pUnit->getProtoObject()->getDisplayName(name);
            sID.format("%S %d%%", name.getPtr(), (long)(pUnit->getBuildPercent()*100.0f));
            gHPBar.displayProgress(pSquad, pUnit->getBuildPercent());
         }
         else if (isRepairing)
         {
            sID.format("REPAIRING %d%%", (long)(pSquad->getRepairPercent()*100.0f));
            gHPBar.displayProgress(pSquad, pSquad->getRepairPercent());
         }
         float wid = gFontManager.getLineLength(sID, sID.length());

// fixme - does this ever draw?
         gFontManager.drawText(fontHandle, idPos.x-(wid*0.5f), idPos.y, sID.getPtr(), color);
      }

      if (isBowling)
      {
         sID = "";
//-- FIXING PREFIX BUG ID 5748
         const BUnitActionCollisionAttack* pCollisionAction = static_cast<BUnitActionCollisionAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitCollisionAttack));
//--
         if (pCollisionAction)
         {
            float currentRamJuice = pUnit->getAmmunition();//pCollisionAction->getCurrentRamJuice();
            float maxRamJuice = pUnit->getAmmoMax();//pCollisionAction->getProtoAction()->getBowlMaxRamJuice();
            float percentRamJuice = 0.0f;
            if (maxRamJuice > 0.0f)
               percentRamJuice = 100.0f * (currentRamJuice / maxRamJuice);
            BASSERT(percentRamJuice >= 0.0f && percentRamJuice <= 100.0f);
            float minRamJuiceToDamage = 0.0f;//pCollisionAction->getProtoAction()->getBowlMinRamJuiceToDamage();
            DWORD ramJuiceColor = cDWORDRed;
            if (currentRamJuice >= minRamJuiceToDamage)
            {
               ramJuiceColor = cDWORDGreen;
               sID.format("BOWL!!: %d(%d%%)", static_cast<int>(currentRamJuice), static_cast<int>(percentRamJuice));
            }
            else
            {
               ramJuiceColor = cDWORDRed;
               sID.format("JUICE: %d(%d%%)", static_cast<int>(currentRamJuice), static_cast<int>(percentRamJuice));
            }
            float wid = gFontManager.getLineLength(sID, sID.length());
// fixme - does this ever draw
            gFontManager.drawText(fontHandle, idPos.x-(wid*0.5f), idPos.y+10.0f, sID.getPtr(), ramJuiceColor);
         }
      }
   }
   */

   //XXXHalwes - 7/27/2007 - Hack to show number of civies in Scn04 escape rockets
/*   handle = cInvalidObjectID;
   for (BUnit* pUnit = gWorld->getNextUnit(handle); pUnit != NULL; pUnit = gWorld->getNextUnit(handle))
   {
      if (pUnit->getFlagHasGarrisoned())
      {
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (pProtoObject && pProtoObject->getFlagShowRescuedCount())
         {
            BEntityIDArray garrisonedUnits = pUnit->getGarrisonedUnits();
            uint numGarrisonedUnits = garrisonedUnits.getSize();
            if (numGarrisonedUnits == 0)
               continue;

            const BBoundingBox* pBoundingBox = pUnit->getVisualBoundingBox();
            if (!isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
               continue;

            objectPos = pUnit->getSimCenter();
            objectPos.y += pUnit->getProtoObject()->getObstructionRadiusY() * 2.0f;
            view.calculateWorldToScreen(objectPos, idPos.x, idPos.y);
            
            sID.format("Rescued: %d", numGarrisonedUnits);
// fixme - does this ever draw?
            gFontManager.drawText(fontHandle, idPos.x, idPos.y, sID.getPtr());
         }
      }
   }
   */

   //MWC - Hack to display the number of the base the player just jumped to.
   if(mBaseNumberDisplayTime > 0.0f)
   {
      sID.format("%d", mBaseNumberToDisplay);

      BHandle fontHandle=gFontManager.getFontDenmark24();
      gFontManager.setFont(fontHandle);
// fixme - does this ever draw
      gFontManager.drawText(fontHandle, gUI.mfCenterX, gUI.mfCenterY - 20, sID.getPtr(), cDWORDWhite, BFontManager2::cJustifyCenter);
   }
}


//==============================================================================
// BUser::renderSelectedUnitIcons
//==============================================================================
void BUser::renderSelectedUnitIcons()
{
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigFlashGameUI))
      return;

   BHandle fontHandle=gUIGame.getPlayerStatFont();
   gFontManager.setFont(fontHandle);

   int subSelectIndex = mSelectionManager->getSubSelectGroupHandle();

   for(long i=0; i<mSelectedUnitDisplayCount; i++)
   {
      if (i == subSelectIndex)
         mSelectedUnitIcons[i].setColor(cDWORDGold);
      else
         mSelectedUnitIcons[i].setColor(cDWORDCyan);
      mSelectedUnitIcons[i].render(0, 0);
      mSelectedUnitText[i].render(0, 0);
   }

   for (int i=0; i<BSelectionManager::cMaxSubSelectGroups; i++)
   {
      if (mSelectionManager->getSubSelectTag(i))
      {
         long x=mSelectedUnitIcons[i].getX()+20;
         long y=mSelectedUnitIcons[i].getY()+40;
// LOC - this doesn't draw in final.
         gFontManager.drawText(fontHandle, (float)x, (float)y, "^^", cDWORDGold);
      }
   }
#endif
}


//==============================================================================
// BUser::renderPlayableBorder
//==============================================================================
void BUser::renderPlayableBorder()
{
   if (gConfig.isDefined(cConfigNoWorldBorder))
      return;
   float thickness = 0.5f;
   DWORD color=cDWORDRed;
   float offset=0.5f;
   float line = 10.0f;
   float worldMinX = gWorld->getSimBoundsMinX();
   float worldMinZ = gWorld->getSimBoundsMinZ();
   float worldMaxX = gWorld->getSimBoundsMaxX();
   float worldMaxZ = gWorld->getSimBoundsMaxZ();
   float xSize=worldMaxX-worldMinX;
   float zSize=worldMaxZ-worldMinZ;
   int xCount = (int)(xSize / line)-1;
   int zCount = (int)(zSize / line)-1;
   BVector p1, p2;
   // Line 1
   p1.x=worldMinX;
   p2.x=worldMinX;
   p1.z=worldMinZ-thickness;
   p2.z=p1.z+line;
   for (int i=0; i<zCount; i++)
   {
      gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
      p1.z=p2.z;
      p2.z+=line;
   }
   p2.z=worldMaxZ+thickness;
   gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
   // Line 2
   p1.x=worldMaxX;
   p2.x=worldMaxX;
   p1.z=worldMinZ-thickness;
   p2.z=p1.z+line;
   for (int i=0; i<zCount; i++)
   {
      gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
      p1.z=p2.z;
      p2.z+=line;
   }
   p2.z=worldMaxZ+thickness;
   gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
   // Line 3
   p1.x=worldMinX-thickness;
   p2.x=p1.x+line;
   p1.z=worldMinZ;
   p2.z=worldMinZ;
   for (int i=0; i<xCount; i++)
   {
      gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
      p1.x=p2.x;
      p2.x+=line;
   }
   p2.x=worldMaxX+thickness;
   gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
   // Line 4
   p1.x=worldMinX-thickness;
   p2.x=p1.x+line;
   p1.z=worldMaxZ;
   p2.z=worldMaxZ;
   for (int i=0; i<xCount; i++)
   {
      gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
      p1.x=p2.x;
      p2.x+=line;
   }
   p2.x=worldMaxX+thickness;
   gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, thickness, color, color, offset);
}

//==============================================================================
// BUser::renderCameraBoundaryLines
//==============================================================================
void BUser::renderCameraBoundaryLines()
{
   if (!gConfig.isDefined(cConfigRenderCameraBoundaryLines))
      return;

   // Render lines
   //
   long numLines = mCameraBoundaryLines.getNumber();
   for(long i = 0; i < numLines; i++)
   {
      const BCameraBoundaryLine *line = &mCameraBoundaryLines[i];

      DWORD color = cDWORDRed;

      if(line->mType == BCameraBoundaryLine::cBoundaryHover)
      {
         color = cDWORDYellow;
      }
      else
      {
         color = cDWORDRed;
      }


      long numPoints = line->mPoints.getNumber() - 1;
      for(long i = 0; i < numPoints; i++)
      {
         //gTerrainSimRep.addDebugLineOverCameraRep(line->mPoints[i], line->mPoints[i + 1], cDWORDRed, cDWORDRed, 0.25f);

         BVector p1 = line->mPoints[i];
         BVector p2 = line->mPoints[i + 1];
         BVector dir = p2 - p1;
         dir.normalize();

         const long tesselationDist = 4;

         float dist = p1.xzDistance(p2);
         long numSegs = long(dist / tesselationDist) + 1;
         float distPerSeg = dist / numSegs;

         for(long j = 0; j < numSegs; j++)
         {
            BVector seg1 = p1 + (dir * float(distPerSeg * j));
            BVector seg2 = seg1 + (dir * distPerSeg);

            gTerrainSimRep.addDebugLineOverCameraRep(seg1, seg2, color, color, 0.25f);
         }

      }
   }

   // Render both hover points.  The camera hover point, which is planted on the camera sim rep.  And the 
   // regular hover point, planted on the sim rep.
   //
   BMatrix mat;
   mat.makeIdentity();
   mat.setTranslation(mHoverPoint);
   gpDebugPrimitives->addDebugCircle(mat, 3.0f, 0xFFFFFFFF);

   mat.makeIdentity();
   mat.setTranslation(mCameraHoverPoint);
   gpDebugPrimitives->addDebugCircle(mat, 3.0f, 0xFF00FFFF);
}

//==============================================================================
// BUser::renderGameStateMessages
//==============================================================================
void BUser::renderGameStateMessages()
{
#ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigRenderGameStateMessages))
      return;

   if(getFlagGameStateMessage())
   {
      BHandle fontHandle=gFontManager.getFontDenmark24();
      gFontManager.setFont(fontHandle);
// LOC - this doeasn't draw in final
      gFontManager.drawText(fontHandle, gUI.mfCenterX, gUI.mfSafeY2-gFontManager.getLineHeight()-11, mGameStateMessageText, cDWORDWhite, BFontManager2::cJustifyCenter);
   }
#endif
}

//==============================================================================
// BUser::renderUserMessages
//==============================================================================
void BUser::renderUserMessages()
{
#ifndef BUILD_FINAL
   BHandle fontHandle = gFontManager.getFontDenmark24();
   gFontManager.setFont( fontHandle );
   for( long i = 0; i < USER_MESSAGES_MAX; i++ )
   {      
      if( mUserMessages[i].enabled && mUserMessages[i].hasPlayer( mPlayerID ) )
      {
         float relativeX1 = gUI.mfSafeX1 + mUserMessages[i].xPos;
         float relativeY1 = gUI.mfSafeY1 + mUserMessages[i].yPos;
// LOC - this doesn't draw in final
         gFontManager.drawText(fontHandle, relativeX1, relativeY1, mUserMessages[i].text.getPtr(), mUserMessages[i].color.asDWORD(), mUserMessages[i].justify);
      }
   }
#endif
}

//==============================================================================
// BUser::updateFlashUserMessages
// Maybe this should go into the hint manager?
//==============================================================================
void BUser::updateFlashUserMessages(float elapsedTime)
{
   BFlashUserMessage* pMessage = getFlashUserMessage();
   if (!pMessage)
      return;

   BUIWidgets* pWidgets=gUIManager->getWidgetUI();
   if (!pWidgets)
      return;

   pMessage->updateTime(elapsedTime);

   if (pMessage->hasExpired())
   {
      removeFlashUserMessage(pMessage);
      pWidgets->setUserMessageVisible(false);
      return;
   }

   // if it's not new, we are done
   if (!pMessage->getIsNew())
      return;

   pMessage->setIsNew(false);

   if (pMessage->hasSound())
      gSoundManager.playCue(pMessage->getSoundString(), cInvalidWwiseObjectID, pMessage->getQueueSound());

   pWidgets->displayUserMessage(pMessage->getMessage());
}

//==============================================================================
//==============================================================================
BFlashUserMessage* BUser::getFlashUserMessage()
{
   if (mFlashUserMessages.getNumber() == 0)
      return NULL;

   return mFlashUserMessages[0];
}

//==============================================================================
//==============================================================================
void BUser::removeFlashUserMessage(BFlashUserMessage* message)
{
   if (message == NULL)
      return;

   for (uint i=0; i < mFlashUserMessages.getSize(); ++i)
   {
      if (mFlashUserMessages[i] == message)
      {
         delete message;
         mFlashUserMessages.removeIndex(i);
         break;
      }
   }

   // for right now, just remove from the front of the queue.
   //int index = 0;

   //// is the index
   //if (index >= mFlashUserMessages.getNumber())
   //   return;

   //BFlashUserMessage* pMessage = mFlashUserMessages[index];
   //delete pMessage;
   //mFlashUserMessages[index]=NULL;
   //mFlashUserMessages.removeIndex(index);

   if (mFlashUserMessages.getSize() > 0 && gUIManager->getWidgetUI())
      (gUIManager->getWidgetUI())->setUserMessageVisible(true, true);
}

//==============================================================================
//==============================================================================
void BUser::addFlashUserMessage(const BUString& message, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame)
{
   // grab the chat that is currently at the head of the queue and expire it.
   BFlashUserMessage* pCurrentMessage = getFlashUserMessage();

   // If we are not currently playing a sound with this text, then let's expire it 
   //    so the next sound can go through.
   if (pCurrentMessage && (pCurrentMessage->getCueIndex() == cInvalidCueIndex) && (!pCurrentMessage->existPastEndGame() || pCurrentMessage->hasExpired()))
   {
      pCurrentMessage->expire();
      removeFlashUserMessage(pCurrentMessage);
   }

   BFlashUserMessage* pMessage = new BFlashUserMessage(message, sound, bQueueSound, duration, existPastEndGame);
   mFlashUserMessages.add(pMessage);

   if (existPastEndGame && gUIManager->getWidgetUI())
      (gUIManager->getWidgetUI())->setUserMessageVisible(true, true);
}


//==============================================================================
//==============================================================================
void BUser::addFlashUserMessage(long stringID, const BSimString& sound, bool bQueueSound, float duration, bool existPastEndGame)
{
   // grab the chat that is currently at the head of the queue and expire it.
   BFlashUserMessage* pCurrentMessage = getFlashUserMessage();

   // If we are not currently playing a sound with this text, then let's expire it 
   //    so the next sound can go through.
   if (pCurrentMessage && (pCurrentMessage->getCueIndex() == cInvalidCueIndex) && (!pCurrentMessage->existPastEndGame() || pCurrentMessage->hasExpired()))
   {
      pCurrentMessage->expire();
      removeFlashUserMessage(pCurrentMessage);
   }

   BFlashUserMessage* pMessage = new BFlashUserMessage(stringID, sound, bQueueSound, duration, existPastEndGame);
   mFlashUserMessages.add(pMessage);

   if (existPastEndGame && gUIManager->getWidgetUI())
      (gUIManager->getWidgetUI())->setUserMessageVisible(true, true);
}



//==============================================================================
// BUser::updateUIHints
// Maybe this should go into the hint manager?
//==============================================================================
void BUser::updateUIHints(float elapsedTime)
{
   BHintManager* pHintManager = gWorld->getHintManager();
   if (!pHintManager)
      return;

   BHintMessage* pHint = pHintManager->getHint();
   if (!pHint)
      return;

   pHint->updateTime(elapsedTime);

   if (pHint->hasExpired())
   {
      pHintManager->removeHint(pHint);
      gUIManager->hideHint();
      return;
   }

   // if it's not new, we are done
   if (!pHint->getIsNew())
      return;

   pHint->setIsNew(false);

   gUIManager->displayHint(pHint);
}


//==============================================================================
// BUser::updateChatMessages
// maybe this should go into the chatmanager?
//==============================================================================
void BUser::updateChatMessages(float elapsedTime)
{
   BChatManager* pChatManager = gWorld->getChatManager();
   if (!pChatManager)
      return;

   BChatMessage* pChat = pChatManager->getChat();
   if (!pChat)
      return;

   pChat->updateTime(elapsedTime);

   bool bTextVisible = pChat->getForceSubtitles() || getOption_ChatTextEnabled();
   bool bVideoDone   = pChat->getVideoHandle() == cInvalidVideoHandle;
   bool bAudioDone   = pChat->getCueIndex() == cInvalidCueIndex;
   bool bExpired     = !bTextVisible || pChat->getAutoExpire() || pChat->hasExpired() || pChatManager->getNumChats() > 1;

#ifndef BUILD_FINAL
   if( gConfig.isDefined( "fastChats" ) )
   {
      bVideoDone = bAudioDone = bExpired = true;
   }
#endif

   if (bVideoDone && bAudioDone && bExpired)
   {      
      if (pChat->getVideoHandle() != cInvalidVideoHandle)
      {
         gBinkInterface.stopVideo(pChat->getVideoHandle(), true);
      }

      gGeneralEventManager.eventTrigger(BEventDefinitions::cChatCompleted, mPlayerID);            

      // The message has expired
      pChatManager->removeChat(pChat);

      if (gConfig.isDefined(cConfigShowFlashChat))
      {
         // turn the chat off
         //mpUIContext->setChatVisible(false);
         gUIManager->setChatVisible(false);
      }
      return;
   }

   // don't display this if we don't have the config.
   if (!gConfig.isDefined(cConfigObjectivesDisplay))
      return;

   if (pChat->getIsNew())
   {    
      pChat->setIsNew(false);

      // For right now, if we are not in the right mode, we will fire off the audio chat
      //    and skip the display of the text chat

      // Only show the chat dialog in the right 
      bool canShow = true;
      switch (mUserMode)
      {
         case cUserModeNormal:
         case cUserModeCircleSelecting:
         case cUserModeInputUILocation:
         case cUserModeInputUILocationMinigame:
         case cUserModeRallyPoint:
         case cUserModeBuildingRallyPoint:
         case cUserModeBuildLocation:
         case cUserModeInputUISquadList:
         case cUserModeInputUIPlaceSquadList:
         case cUserModeCommandMenu:
         case cUserModePowerMenu:    
            break;
         case cUserModeAbility:
         case cUserModeInputUIUnit:
         case cUserModeInputUISquad:
            break;
         case cUserModeCinematic:
            break;
         case cUserModeFollow:
            break;
      }

      if (!pChatManager->getChatsEnabled() || gConfig.isDefined(cConfigDisableAllChats))
         canShow = false;

      // display the chat
      if (gConfig.isDefined(cConfigShowFlashChat))
      {
         if (canShow)
         {
            //mpUIContext->displayChat(pChat);
            BBinkVideoHandle videoHandle = gUIManager->displayChat(pChat, pChatManager);
            pChat->setVideoHandle(videoHandle);           
         }
      }

      MVinceEventAsync_ChatFired(this, pChat->getChatStringID());

      gGeneralEventManager.eventTrigger(BEventDefinitions::cChatShown, mPlayerID);

   }
}



//==============================================================================
// BUser::updateObjectiveMessages
//==============================================================================
void BUser::updateObjectiveMessages(float elapsedTime)
{
   BObjectiveManager* pObjManager = gWorld->getObjectiveManager();
   if (!pObjManager)
      return;

   BObjectiveMessage* pMessage = pObjManager->getMessageNotification();
   if (!pMessage)
      return;

   pMessage->updateTime(elapsedTime);
   if (pMessage->hasExpired())
   {
      // The message has expired
      pObjManager->removeObjectiveNotification(pMessage->getObjectiveID());

      gUIManager->showIngameObjective(false);
      return;
   }

   // don't display this if we don't have the config.
   if (!gConfig.isDefined(cConfigObjectivesDisplay))
      return;

   if (pMessage->isNew())
   {
      pMessage->setIsNew(false);

//-- FIXING PREFIX BUG ID 5754
      const BObjective* pObjective = pObjManager->getObjective( pObjManager->getIndexFromID(pMessage->getObjectiveID()));
//--
      if (!pObjective)
         return;

      gUIManager->showIngameObjective(true, pMessage->getObjectiveID());
   }
   else if (pMessage->shouldRefresh())
   {
      pMessage->setShouldRefresh(false);

//-- FIXING PREFIX BUG ID 5755
      const BObjective* pObjective = pObjManager->getObjective( pObjManager->getIndexFromID(pMessage->getObjectiveID()));
//--
      if (!pObjective)
         return;

      gUIManager->refreshObjective(pMessage->getObjectiveID());
   }
}

//==============================================================================
// BUser::udpateUIContext()
//==============================================================================
void BUser::updateUIContext(float elapsedTime)
{
   if (mpUIContext)
      mpUIContext->update(elapsedTime);
}

//==============================================================================
// BUser::doWorkAtCurrentLocation
//==============================================================================
bool BUser::doWorkAtCurrentLocation(long abilityID, long squadMode, bool noTarget, bool targetUnit, BEntityIDArray* pSquads)
{
   //Clear the double click squads here.
   mDoubleClickSquads.setNumber(0);

   if (!mFlagHoverPointOverTerrain && mHoverObject==cInvalidObjectID)
      return (false);

   const BPlayer* pPlayer = getPlayer();

   // MS 10/15/2008: PHX-13886, don't do work commands on transport units. This hack is sponsored by DP.
   // VT 10/23/2008: PHX-15976, since this check was added to disallow garrisoning in friendly units, 
   // allow work commands to go through if the unit targetted is an enemy
   if(pPlayer && !noTarget && mHoverObject.getType() == BEntity::cClassTypeUnit)
   {
      BUnit* pTestUnit = gWorld->getUnit(mHoverObject);
      if(pTestUnit && pTestUnit->getParent() && !pPlayer->isEnemy(pTestUnit->getPlayerID()))
      {
         BSquad* pParentSquad = reinterpret_cast<BSquad*>(pTestUnit->getParent());
         if(pParentSquad && pParentSquad->getProtoObject() && pParentSquad->getProtoObject()->isType(gDatabase.getOTIDTransporter()))
            return false;
      }
   }

   BEntityIDArray ugIDs;
   if (pSquads)
      ugIDs = *pSquads;
   else
      mSelectionManager->getSubSelectedSquads(ugIDs);
   if (ugIDs.getNumber() == 0)
      return (false);

   bool playAbilitySound = false;
   BProtoAction* pProtoActionSound = NULL;
   bool commanded=false;
   bool bHoverObjectSet=false;
   for (long j = ugIDs.getNumber() - 1; j >= 0; j--)
   {
      bool valid = false;
//-- FIXING PREFIX BUG ID 5759
      const BSquad* pSquad = gWorld->getSquad(ugIDs[j]);
//--
      if (pSquad && pSquad->getNumberChildren() > 0)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
         if (pUnit && pUnit->isAlive() && !pUnit->getFlagDown() && pUnit->getFlagBuilt() && !pUnit->getIgnoreUserInput() && pUnit->getPlayerID()==mPlayerID)
         {
           valid = true;

            // Don't allow issuing orders to units that are repairing.
            if (pSquad->getFlagIsRepairing())
               valid = false;

            // Allow users to kick off unit powers (such as air strike and carpet bomb) directly through selecting and commanding the unit.
            int protoPowerID = pUnit->getProtoObject()->getProtoPowerID();
            if (protoPowerID != -1)
            {
//-- FIXING PREFIX BUG ID 5758
               const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
               if (pProtoPower && pProtoPower->getFlagUnitPower())
               {
                  if (pUnit->getPlayer()->canUsePower(protoPowerID, pSquad->getID()))
                  {
                     bool unitAvail = true;
                     int actionType = pProtoPower->getActionType();
                     if (actionType == BAction::cActionTypeSquadCarpetBomb)
                     {
//-- FIXING PREFIX BUG ID 5756
                        const BUnitActionMoveAir* pAirMoveAction = reinterpret_cast<BUnitActionMoveAir*>(pUnit->getActionByType(BAction::cActionTypeUnitMoveAir));
//--
                        if (pAirMoveAction && pAirMoveAction->getState()!=BAction::cStateNone)
                           unitAvail= false;
                     }
                     if (unitAvail)
                     {
                        BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
                        if(pCommand)
                        {
                           pCommand->setSenderType(BCommand::cPlayer);
                           pCommand->setSenders(1, &mPlayerID);
                           pCommand->setRecipientType(BCommand::cGame);
                           pCommand->setType(BGameCommand::cTypeUnitPower);
                           pCommand->setPosition(mHoverPoint);
                           pCommand->setData(pUnit->getID().asLong());
                           gWorld->getCommandManager()->addCommandToExecute(pCommand);
                           if (!commanded)
                           {
                              //-- Look up the selected unit
                              long squadID=cInvalidProtoID;
                              BUnit *pHoverUnit = gWorld->getUnit(mHoverObject);
                              if(pHoverUnit && pHoverUnit->getParentSquad())
                                 squadID = pHoverUnit->getParentSquad()->getProtoID();
                              
                              BUnit* pUnitToPlaySound = gWorld->getUnit(pSquad->getChild(pSquad->getNumberChildren()-1));
                              if(pUnitToPlaySound)
                                 pUnitToPlaySound->playUISound(cObjectSoundAckAttack, false, squadID);

                              gUI.playRumbleEvent(BRumbleEvent::cTypeConfirmCommand);
                              if (abilityID != -1)
                              {
                                 MVinceEventAsync_AbilityUsed(this, abilityID);
                              }
                              else
                              {
                                 MVinceEventAsync_ControlUsed(this, "do_work_at_current_location");
                              }
                              commanded=true;
                           }
                        }
                     }
                  }
                  valid = false; // have this unit removed from the list of units to command
               }
            }
         }
      }
      if (!valid)
      {
         ugIDs.removeIndex(j);
      }
      else
      {
         //-- Is this squad doing an ability? If so, we're use it's ability ack sound
         if (!playAbilitySound)
         {
            playAbilitySound =  canPlayAbilitySound(abilityID, pSquad, &pProtoActionSound);
         }
      }
   }
   if (ugIDs.getNumber()==0)
   {
      return (commanded);
   }

   //-- Send out the work command
   BWorkCommand* c = (BWorkCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandWork);
   if (!c)
   {
      return (commanded);
   }
   c->setSenders(1, &mPlayerID);
   c->setSenderType(BCommand::cPlayer);
   c->setRecipientType(BCommand::cArmy);
   if (!c->setRecipients(ugIDs.getNumber(), ugIDs.getPtr()))
   {
      delete c;
      return (commanded);
   }
   c->setFlag(BCommand::cFlagAlternate, getFlagModifierAction());   
                                                                                                                                                                                                                 
   c->setAbilityID(abilityID);
   c->setSquadMode(squadMode);

   if (noTarget)
   {
      c->setUnitID(cInvalidObjectID);
   }
   else if ((mHoverType != cHoverTypeSelect && mHoverType != cHoverTypeNone) || targetUnit)
   {
      c->setUnitID(mHoverObject);
      bHoverObjectSet = true;
   }

   BVector pos = mHoverPoint;

   // For change mode abilities, set the location to the location of the squad if hovering over a selected unit.
   bool noAbilityWaypointIndicator=false;
   if (abilityID != -1 && mHoverObject != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 5760
      const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--
      if (pAbility && pAbility->getType()==cAbilityChangeMode)
      {
         bool found=false;
         for (uint i=0; i<ugIDs.getSize() && !found; i++)
         {
//-- FIXING PREFIX BUG ID 5762
            const BSquad* pSquad = gWorld->getSquad(ugIDs[i]);
//--
            if (pSquad)
            {
               for (uint j=0; j<pSquad->getNumberChildren(); j++)
               {
                  if (pSquad->getChild(j) == mHoverObject)
                  {
                     found=true;
                     pos = pSquad->getPosition();
                     noAbilityWaypointIndicator=true;
                     break;
                  }
               }
            }
         }
      }
   }

   c->setWaypoints(&pos, 1);
   c->setHitZoneIndex(mHoverHitZoneIndex);

   gWorld->getCommandManager()->addCommandToExecute(c);

   //Save off the squads we just ordered here in case we want to do something
   //with them on a double click.  At this point, we're not worrying about this
   //being set "wrong" since the double click can only go off in a small window
   //of time.  If it turns out that we're getting wacky stuff, we'll have to go
   //through and make sure this is reset properly by all of the other inputs.  If
   //someone changes this, double check that the doAttackMove() method doesn't
   //also need to be changed.
   mDoubleClickSquads = ugIDs;

   //-- maybe add a waypoint indicator
   if (((mHoverType == cHoverTypeNone || mHoverType == cHoverTypeSelect) && squadMode == -1) || (abilityID != -1 && !noAbilityWaypointIndicator && c->getUnitID()==cInvalidObjectID))
   {
      BObjectCreateParms waypointParms;
      waypointParms.mProtoObjectID = getPlayer()->getCiv()->getCommandAckObjectID();
      if (waypointParms.mProtoObjectID != -1)
      {
         waypointParms.mPlayerID = mPlayerID;
         waypointParms.mPosition = mHoverPoint;
               
         BObject* pObject = gWorld->createObject(waypointParms);
         if (pObject)
         {
            pObject->setFlagVisibility(true);
            pObject->setFlagLOS(false);
            pObject->setFlagDopples(false);
            pObject->setLifespan(1000);
         }
      }      
   }

   //-- play a sound
//    if(squadIndexWhoDoesAbility != -1)
//       pSquad=gWorld->getSquad(ugIDs[squadIndexWhoDoesAbility]);
//    else
//       pSquad=gWorld->getSquad(ugIDs[0]);

   // Get lists of squad types and counts
   BProtoSquadIDArray squadTypeList;
   BInt32Array squadCountList;
   bool foundCampaignHero = false;
   bool foundHero = false;
   int bestIndex = cInvalidIndex;
   int bestCount = 0;   
   for (long i = 0; i < ugIDs.getNumber(); i++)
   {
      const BSquad* pSquad = gWorld->getSquad(ugIDs[i]);
      if (pSquad)
      {
         // Filter out squads that can't play ability sounds if we want to play an ability sound
         if (playAbilitySound && !canPlayAbilitySound(abilityID, pSquad))
            continue;

         // Is this a campaign hero?
         const BUnit* pUnit = pSquad->getLeaderUnit();
         bool isCampaignHero = (pUnit && (pUnit->isType(gDatabase.getOTIDCampaignHero())));

         // Filter out non campaign heroes after we find our first campaign hero
         if (foundCampaignHero && !isCampaignHero)
            continue;

         // Reset best if this is our first campaign hero
         if (!foundCampaignHero && isCampaignHero)
         {
            foundCampaignHero = true;
            bestIndex = cInvalidIndex;
            bestCount = 0;
         }

         // Only care about heroes if we didn't find a campaign hero yet
         if (!foundCampaignHero)
         {
            // Is this a hero?
            bool isHero = (pUnit && (pUnit->isType(gDatabase.getOTIDHero()) || pSquad->getFlagSpartanContainer()));

            // Filter out non heroes after we find our first hero
            if (foundHero && !isHero)
               continue;

            // Reset best if this is our first hero
            if (!foundHero && isHero)
            {
               foundHero = true;
               bestIndex = cInvalidIndex;
               bestCount = 0;
            }
         }

         int index;
         int count;
         BProtoSquadID protoSquad = pSquad->getProtoSquadID();

         // Do we already have that type?
         int existingIndex = squadTypeList.find(protoSquad);

         // No. Add it to the list and set its count to 1
         if (existingIndex == cInvalidIndex)
         {
            squadTypeList.add(protoSquad);
            squadCountList.add(1);

            index = squadTypeList.getNumber() - 1;
            count = 1;
         }
         // Yes. Increment the count.
         else
         {
            count = squadCountList[existingIndex];
            squadCountList[existingIndex] = ++count;

            index = existingIndex;
         }

         // We have a new winner
         if (count > bestCount)
         {
            bestCount = count;
            bestIndex = index;
         }
      }
   }

   if (bestIndex != cInvalidIndex)
   {
      // Find a squad with the winning squad type
      BProtoSquadID bestSquadType = squadTypeList[bestIndex];
      const BSquad* pBestSquad = NULL;
      for (long i = 0; i < ugIDs.getNumber(); i++)
      {
         const BSquad* pSquad = gWorld->getSquad(ugIDs[i]);
         if (pSquad && (pSquad->getProtoSquadID() == bestSquadType))
         {
            pBestSquad = pSquad;
            break;
         }
      }

      // Play the acknowledgement
      if (pBestSquad)
      {
         const BUnit* pUnit = gWorld->getUnit(pBestSquad->getChild(pBestSquad->getNumberChildren()-1)); //-- Use the last unit in the squad
         if (pUnit)
         {     
            //-- Look up the selected unit protoObject
            long squadProtoID = cInvalidProtoID;
            if(!noTarget && ((mHoverType==cHoverTypeEnemy) || (mHoverType==cHoverTypeAbility))) 
            {            
               const BUnit *pHoverUnit = gWorld->getUnit(mHoverObject);
               if(pHoverUnit && pHoverUnit->getParentSquad())
                  squadProtoID = pHoverUnit->getParentSquad()->getProtoID();           
            }

            bool soundPlayed = false;
            if (playAbilitySound)
            {
               int actionID = -1;
               if(pProtoActionSound)
                  actionID = pProtoActionSound->getID();
               soundPlayed = pUnit->playUISound(cObjectSoundAckAbility, false, squadProtoID, actionID);
            }
            if (!soundPlayed && (squadProtoID != cInvalidProtoID) && (mHoverType==cHoverTypeEnemy))
            {
               soundPlayed = pUnit->playUISound(cObjectSoundAckAttack, false, squadProtoID);
            }
            if (!soundPlayed)
            {
               soundPlayed = pUnit->playUISound(cObjectSoundAckWork, false, squadProtoID);
            }
         }
      }
   }

   //Flash the object that is being worked on (attacked, etc) if available
   if ((mHoverType != cHoverTypeSelect && mHoverType != cHoverTypeNone) || targetUnit)
   {
      //-- Look up the selected unit
      BUnit *pHoverUnit = gWorld->getUnit(mHoverObject);
      if(pHoverUnit && pHoverUnit->getParentSquad())
      {
         BSquad *pSquad = pHoverUnit->getParentSquad();
         if( pSquad )
         {
            for( uint i=0; i<pSquad->getNumberChildren(); i++ )
            {
               BEntityID childID = pSquad->getChild(i);
               BObject *pChildObject = gWorld->getObject( childID );
               if (pChildObject)
               {
                  const float speed = -4.0f;
                  const float duration = 2.0f;
                  float scale = 1.0f;
                  float minY;
                  if (pChildObject->getVisualBoundingBox())
                  {
                     BVector minCorner, maxCorner;
                     pChildObject->getVisualBoundingBox()->computeWorldCorners(minCorner, maxCorner);
                     float yExtent = maxCorner.y - minCorner.y;
                     if (yExtent > cFloatCompareEpsilon || yExtent < -cFloatCompareEpsilon)
                        scale = -1.0f / yExtent;
                     minY = minCorner.y;
                  }
                  else
                     minY = pChildObject->getPosition().y;
                  float heightOffset = 2.0f - (minY * scale);
                  
                  pChildObject->setOverrideTintColor(cDWORDWhite);
                  pChildObject->setTargettingSelection(true, scale, heightOffset, heightOffset, duration, speed, true);
               }
            }
         }
      }
   }

   if (!commanded)
   {
      gUI.playRumbleEvent(BRumbleEvent::cTypeConfirmCommand);
      if (abilityID != -1)
      {
         MVinceEventAsync_AbilityUsed(this, abilityID);
      }
      else
      {
         MVinceEventAsync_ControlUsed(this, "do_work_at_current_location");
      }

      //All work commands
      if(mHoverType == cHoverTypeEnemy)
      {
         gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandAttack, mPlayerID, &ugIDs, cInvalidObjectID, NULL, mHoverObject); 
      }
      if(bHoverObjectSet == false)
      {
         gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandMove, mPlayerID, &ugIDs, cInvalidObjectID, NULL, cInvalidObjectID); 
         
         gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandDoWork, mPlayerID, &ugIDs, cInvalidObjectID, NULL, cInvalidObjectID); 
      }
      else
      {
         gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandDoWork, mPlayerID, &ugIDs, cInvalidObjectID, NULL, mHoverObject); 
      }


   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BUser::doAttackMove()
{
   //NOTE: This is taking some liberties by assuming that the squads that were ordered
   //on the original work are still valid to go tack the 'attack move' flag onto.  That
   //may have to change later, but Angelo and I can't think of any major issues with
   //it right now.

   if (!mFlagHoverPointOverTerrain && mHoverObject==cInvalidObjectID)
      return (false);
   if (mDoubleClickSquads.getSize() == 0)
      return (false);

   //Create the "Make the last thing you were told to do an Attack Move" command.
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
   if (!pCommand)
      return (false);
   pCommand->setSenderType(BCommand::cPlayer);
   pCommand->setSenders(1, &mPlayerID);
   pCommand->setRecipientType(BCommand::cGame);
   pCommand->setType(BGameCommand::cTypeAttackMove);
   if (!pCommand->setRecipients(mDoubleClickSquads.getNumber(), mDoubleClickSquads.getPtr()))
   {
      delete pCommand;
      return (false);
   }

   //Execute it (this really should check return code).
   gWorld->getCommandManager()->addCommandToExecute(pCommand);
   return (true);
}

//==============================================================================
// BUser::doManualBuild
//==============================================================================
bool BUser::doManualBuild()
{
   BEntityIDArray ugIDs;
   mSelectionManager->getSubSelectedSquads(ugIDs);
   if (ugIDs.getNumber() == 0)
      return (false);

   for (long j = ugIDs.getNumber() - 1; j >= 0; j--)
   {
      bool valid = false;
//-- FIXING PREFIX BUG ID 5763
      const BSquad* pSquad = gWorld->getSquad(ugIDs[j]);
//--
      if (pSquad && pSquad->getNumberChildren() > 0)
      {
//-- FIXING PREFIX BUG ID 5765
         const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
//--
         if (pUnit && !pUnit->getIgnoreUserInput())
         {
            const BProtoObject* pProtoObject = pUnit->getProtoObject();
            if (mAbilityID != -1)
            {
               if (pProtoObject->getAbilityCommand() == mAbilityID)
                  valid = true;
            }
            else
            {
               uint commandCount = pProtoObject->getNumberCommands();
               for (uint i=0; i<commandCount; i++)
               {
                  BProtoObjectCommand command = pProtoObject->getCommand(i);
                  long id = command.getID();
                  long type = command.getType();
                  if (type == BProtoObjectCommand::cTypeBuild && id == mBuildProtoID)
                  {
                     valid = true;
                     break;
                  }
               }
            }
         }
      }
      if (!valid)
         ugIDs.removeIndex(j);
   }
   if (ugIDs.getNumber() == 0)
      return (false);

   BWorkCommand* c = (BWorkCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandWork);
   if (!c)
   {
      return (false);
   }
   c->setSenders(1, &mPlayerID);
   c->setSenderType(BCommand::cPlayer);
   c->setRecipientType(BCommand::cArmy);
   if (!c->setRecipients(ugIDs.getNumber(), ugIDs.getPtr()))
   {
      delete c;
      return (false);
   }
   c->setFlag(BCommand::cFlagAlternate, getFlagModifierAction());   
   c->setBuildProtoID(mBuildProtoID);
   c->setWaypoints(&mPlacementSuggestion, 1);
   gWorld->getCommandManager()->addCommandToExecute(c);

   //-- play a sound
//-- FIXING PREFIX BUG ID 5766
   const BSquad* pSquad = gWorld->getSquad(ugIDs[0]);
//--
   if (pSquad && pSquad->getNumberChildren() > 0)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(pSquad->getNumberChildren()-1));
      if (pUnit)
      {         
         pUnit->playUISound(cObjectSoundAckWork);
      }
   }

   return (true);
}

//==============================================================================
// BUser::doUnpack
//==============================================================================
bool BUser::doUnpack()
{
   if (mAbilityID == -1)
      return (false);

   BEntityIDArray ugIDs;
   mSelectionManager->getSubSelectedSquads(ugIDs);
   if (ugIDs.getNumber() == 0)
      return (false);

   BEntityID closestSquad = cInvalidObjectID;
   float closestDist = 0.0f;
   for (long j = ugIDs.getNumber() - 1; j >= 0; j--)
   {
//-- FIXING PREFIX BUG ID 5767
      const BSquad* pSquad = gWorld->getSquad(ugIDs[j]);
//--
      if (pSquad && pSquad->getNumberChildren() > 0)
      {
//-- FIXING PREFIX BUG ID 5768
         const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
//--
         if (pUnit && !pUnit->getIgnoreUserInput())
         {
            const BProtoObject* pProtoObject = pUnit->getProtoObject();
            if (pProtoObject->getAbilityCommand() == mAbilityID)
            {
               float dist = pSquad->getPosition().xzDistanceSqr(mPlacementSuggestion);
               if (closestSquad == cInvalidObjectID || dist < closestDist)
               {
                  closestSquad = pSquad->getID();
                  closestDist = dist;
               }
            }
         }
      }
   }
   if (closestSquad == cInvalidObjectID)
      return (false);

   BWorkCommand* c = (BWorkCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandWork);
   if (!c)
   {
      return (false);
   }
   c->setSenders(1, &mPlayerID);
   c->setSenderType(BCommand::cPlayer);
   c->setRecipientType(BCommand::cArmy);
   if (!c->setRecipients(1, &closestSquad))
   {
      delete c;
      return (false);
   }
   c->setFlag(BCommand::cFlagAlternate, getFlagModifierAction());   
   //c->setBuildProtoID(mBuildProtoID);
   c->setAbilityID(mAbilityID);
   c->setWaypoints(&mPlacementSuggestion, 1);

   float angle = (mCameraYaw == mBuildBaseYaw ? 0.0f : mCameraYaw - mBuildBaseYaw);
   c->setAngle(angle);

   gWorld->getCommandManager()->addCommandToExecute(c);

   //-- play a sound
//-- FIXING PREFIX BUG ID 5769
   const BSquad* pSquad = gWorld->getSquad(closestSquad);
//--
   if (pSquad && pSquad->getNumberChildren() > 0)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(pSquad->getNumberChildren()-1));
      if (pUnit)
         pUnit->playUISound(cObjectSoundAckWork);
   }

   return (true);
}

//==============================================================================
// BUser::doAbility
//==============================================================================
bool BUser::doAbility()
{
   bool retval=false;

   // Break squads up into 2 groups.  The first supports hetero ability commands and the second
   // does not.  If there is only one group that doesn't support hetero commands then it can
   // do the ability.
   const int cMaxGroups = 2;
   BEntityIDArray groups[cMaxGroups];
   int groupHandle = mSelectionManager->getSubSelectGroupHandle();
   uint start = (groupHandle == -1 ? 0 : (uint)groupHandle);
   uint count = (groupHandle == -1 ? mSelectionManager->getNumSubSelectGroups() : (uint)groupHandle + 1);   
   int numGroupsToCommand = 0;
   for (uint i=start; i<count; i++)
   {
      BSelectionAbility ability = mSelectionManager->getSubSelectAbility(i);
//-- FIXING PREFIX BUG ID 5773
      const BAbility* pAbility = gDatabase.getAbilityFromID(ability.mAbilityID);
//--

      const BEntityIDArray& squads = mSelectionManager->getSubSelectGroup(i);
      int squadCount = squads.getNumber();

      bool supportHeteroCommand = true;

      // Process ability.  Squads that don't have an ability will still get added to the list of squads
      // to command below.  They won't do an ability but will instead do their default action, which is
      // what we want now.
      if (ability.mValid && ability.mPlayer && !ability.mRecovering && pAbility)
      {
         supportHeteroCommand = pAbility->getCanHeteroCommand();

         if (pAbility->getType() == cAbilityPower)
         {
            // power ability, so invoke here.
            if ((mpPowerUser == NULL) && (supportHeteroCommand || ((count - start) == 1)))
            {
               for (int j=0; j<squadCount; j++)
               {
//-- FIXING PREFIX BUG ID 5770
                  const BSquad* pSquad = gWorld->getSquad(squads[j]);
//--
                  if (pSquad && !pSquad->getIgnoreUserInput())
                  {
                     BProtoPowerID protoPowerID = gDatabase.getProtoPowerIDByName(pAbility->getName());
                     BPowerLevel powerLevel = 0;
                     BPlayer* pPlayer = getPlayer();

#ifndef BUILD_FINAL
                     BSimString abilityOverride;
                     if (gConfig.get(cConfigOverrideLeaderPower, abilityOverride))
                     {
                        BProtoPowerID overridePowerId = gDatabase.getProtoPowerIDByName(abilityOverride);
                        if (overridePowerId != cInvalidProtoPowerID)
                        {
                           protoPowerID = overridePowerId;
                        
                           // make sure the player has this power
                           pPlayer->addPowerEntry(protoPowerID, squads[j], 1, -1);
                        }
                     }
#endif

                     if (pPlayer)
                        powerLevel = pPlayer->getPowerLevel(protoPowerID);

                     mpPowerUser = createPowerUser(protoPowerID, powerLevel, squads[j]);
                     return (mpPowerUser != NULL);
                  }
               }
            }
         }
         else
         {
            // Special ability handling for old ability types that can't be mixed in with the new ones.
            // If one of these exist then just handle the ability now and return;
            switch (pAbility->getType())
            {
               case cAbilityUnpack:
               {
                  mBuildProtoID = pAbility->getObject(0);
                  if (mBuildProtoID != -1)
                  {
                     gUI.playClickSound();
                     mAbilityID = pAbility->getID();
                     mCommandObject = cInvalidObjectID;
                     mBuildBaseYaw = mCameraYaw;
                     changeMode(cUserModeBuildLocation);
                     if (pAbility->getType()==cAbilityUnpack)
                        mSubMode=cSubModeUnpack;
                     gGeneralEventManager.eventTrigger(BEventDefinitions::cCommandUnpack, mPlayerID); 
                  }
                  return true;
               }

               case cAbilityCommandMenu:
               {
                  BEntityIDArray ugIDs = mSelectionManager->getSelectedUnits();
                  for (long i = 0; i < ugIDs.getNumber(); i++)
                  {
//-- FIXING PREFIX BUG ID 5772
                     const BUnit* pUnit = gWorld->getUnit(ugIDs[i]);
//--
                     if (pUnit && !pUnit->getIgnoreUserInput() && (pUnit->getProtoObject()->getAbilityCommand() == ability.mAbilityID))
                     {
                        mFlagDeselectOnMenuClose=false;
                        showCommandMenu(pUnit->getID());
                        break;
                     }
                  }
                  return true;
               }

               // Halwes - 4/23/2008 - This was explicitly put in to fix SPC scn06 Rhino transport puzzle based on the "manual" transporting design, and may
               //                      become irrelevant when the transporting design changes...again.
               case cAbilityUnload:
               {
                  BEntityIDArray unloadSquads = squads;
                  return (doWorkAtCurrentLocation(ability.mAbilityID, -1, false, true, &unloadSquads));
               }
            }
         }
      }

      numGroupsToCommand++;
      for (int j=0; j<squadCount; j++)
      {
         BSquad* pSquad = gWorld->getSquad(squads[j]);
         if (pSquad && !pSquad->getIgnoreUserInput())
         {
            if (supportHeteroCommand)
               groups[0].add(squads[j]);
            else
               groups[1].add(squads[j]);
         }
      }
   }

   // Issue a doWork for all the squads that can't do an ability on the target or have a non-script based ability.
   if (groups[0].getNumber() > 0)
   {
      // This may command squads that can't actually do an ability but this should be ok since the squads will
      // decide later whether they can really do one.
      retval = doWorkAtCurrentLocation(gDatabase.getAIDCommand(), -1, false, true, &(groups[0]));
   }

   // These squads don't support hetero ability commands so only send them the ability command if there is only one group
   if (groups[1].getNumber() > 0)
   {
      long abilityID = -1;
      if (numGroupsToCommand == 1)
         abilityID = gDatabase.getAIDCommand();

      // This may command squads that can't actually do an ability but this should be ok since the squads will
      // decide later whether they can really do one.
      retval = doWorkAtCurrentLocation(abilityID, -1, false, true, &(groups[1]));
   }

   return retval;
}

//==============================================================================
// BUser::clearAllSelections
//==============================================================================
void BUser::clearAllSelections( void )
{
   mSelectionManager->clearSelections();
   resetCircleSelectionCycle();

   MVinceEventAsync_ControlUsed( this, "clear_all_selections" );

   gGeneralEventManager.eventTrigger(BEventDefinitions::cControlClearAllSelections, mPlayerID);

}

//==============================================================================
// BUser::toggleFreeCamera
//==============================================================================
void BUser::toggleFreeCamera(void)
{
   setFlagFreeCamera(!getFlagFreeCamera());
   
   if (getFlagFreeCamera())
   {
      mFreeCameraCurLoc.set(mpCamera->getCameraLoc().x, mpCamera->getCameraLoc().y, mpCamera->getCameraLoc().z);
      mpFreeCamera->setFOV(mpCamera->getFOV());
   }
}

//==============================================================================
// BUser::handleInput
//==============================================================================
bool BUser::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   port;

   if(!getFlagGameActive())
      return false;   

   // if the user mode is in a waiting for players state, then bail, ignoring all input
   if (gModeManager.getModeGame() && gModeManager.getModeGame()->getFlagWaitingOnPlayers())
      return true;

   // Non-Game UI gets first-pass at input handling before the User (except in a Wow moment)
   if( mUserMode != cUserModeCinematic && gUIManager->isNonGameUIVisible() && gUIManager->handleNonGameUIInput( port, (BInputEventType)event, (BInputControlType)controlType, detail ) )
      return true;

   if (mFlagIgnoreDpad && ((controlType == cDpadUp) || (controlType == cDpadDown) || (controlType == cDpadLeft) || (controlType == cDpadRight)))
      return (true);

//    if ((mUserMode != cUserModePostGame) && 
//        (mUserMode != cUserModePostGameStats) && 
//        (mUserMode != cUserModeCampaignPostGame) )
   if( gUIManager && !gUIManager->isNonGameUIVisible() )
   {
      // Handle trigger UI Button requests
      bool buttonProcessed=false;
      for (int i=0; i<mInputUIButtonRequests.getNumber(); i++)
      {
//-- FIXING PREFIX BUG ID 5775
         const BUserUIButtonRequest& req=mInputUIButtonRequests[i];
//--
         if (controlType==req.mControlType && 
             (req.mIgnoreActionModifier || req.mActionModifier==mFlagModifierAction) &&
             (req.mIgnoreSpeedModifier || req.mSpeedModifier==mFlagModifierSpeed) &&
             (req.mContinuous || (!req.mOnRelease && event==cInputEventControlStart) || (req.mOnRelease && event==cInputEventControlStop)))
         {
            if (req.mOverrideGameInput)
               buttonProcessed=true;
            BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
            if (c)
            {
               c->setSenders(1, &mPlayerID);
               c->setSenderType(BCommand::cPlayer);
               c->setRecipientType(BCommand::cPlayer);
               c->setType(BTriggerCommand::cTypeBroadcastInputUIButtonResult);
               c->setTriggerScriptID(req.mTriggerScriptID);
               c->setTriggerVarID(req.mTriggerVarID);
               c->setInputResult(BTriggerVarUIButton::cUIButtonResultPressed);
               c->setInputLocation(BVector(detail.mX, detail.mY, detail.mAnalog));
               c->setInputModifiers(mFlagModifierAction, mFlagModifierSpeed);
               gWorld->getCommandManager()->addCommandToExecute(c);
            }
            mInputUIButtonRequests.removeIndex(i);
            i--;
         }
      }
      if (buttonProcessed)
         return true;
   }

   bool start = (event == cInputEventControlStart);
   bool stop = (event == cInputEventControlStop);
   bool repeat = (event == cInputEventControlRepeat);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());

   if (pInputInterface->isFunctionControl(BInputInterface::cInputActionModifier, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if ((start || stop) && gInputSystem.isContextActive("Game"))
         uiModifierAction(start);
   }

   if (pInputInterface->isFunctionControl( BInputInterface::cInputSpeedModifier, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if ((start || stop) && gInputSystem.isContextActive("Game"))
         uiModifierSpeed(start);
   }

   if (pInputInterface->isFunctionControl(BInputInterface::cInputStart, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (start && gInputSystem.isContextActive("Game"))
      {
#ifndef BUILD_FINAL
         if( gConfig.isDefined( "noGameMenu" ) && !gLiveSystem->isMultiplayerGameActive() )
         {
            // jce [8/29/2008] -- Adding a check for left-trigger so you can still bring up the menu with "noGameMenu" if needed.
            // Probably a dumb way to do this since I don't know what I'm doing but it seems to work.
            BGamepad& gamepad=gInputSystem.getGamepad(port);
            if(gamepad.isControlActive(cTriggerLeft))
               uiMenu();
            else
               gModeManager.getModeGame()->setPaused( !gModeManager.getModeGame()->getPaused() );
            return true;
         }
#endif
         if( mUserMode != cUserModeCinematic && gWorld->getTransitionManager()->getTransitionComplete() )
            uiMenu();
         else
            gModeManager.getModeGame()->setPaused( !gModeManager.getModeGame()->getPaused() );
         return true;
      }
   }

   //if( !gConfig.isDefined( cConfigDemo ) )
   {
      if (pInputInterface->isFunctionControl(BInputInterface::cInputBack, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
      {
         if (start && gInputSystem.isContextActive("Game"))
         {
            if( mUserMode != cUserModeCinematic && gWorld->getTransitionManager()->getTransitionComplete() )
               uiObjectives();
            else
               gModeManager.getModeGame()->setPaused( !gModeManager.getModeGame()->getPaused() );
            return true;
         }
      }
   }

   if (pInputInterface->isFunctionControl(BInputInterface::cInputFlare, controlType, getFlagModifierAction(), detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      if (start && gInputSystem.isContextActive("Normal"))
      {
         float x = 0.0f;
         float y = 0.0f;
         if (controlType == cButtonThumbLeft)
         {
            x = gInputSystem.getGamepad(port).getStickLX();
            y = gInputSystem.getGamepad(port).getStickLY();
         }
         else if (controlType == cButtonThumbRight)
         {
            x = gInputSystem.getGamepad(port).getStickRX();
            y = gInputSystem.getGamepad(port).getStickRY();
         }
         uiFlare(x, y);
         return true;
      }
   }

#ifndef BUILD_FINAL
   // Debug keys
   bool altKey=gInputSystem.getKeyboard()->isKeyActive(cKeyAlt);
   bool ctrlKey=gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl);
   bool shiftKey=gInputSystem.getKeyboard()->isKeyActive(cKeyShift);

   if(event==cInputEventControlStart)
   {

      if (controlType == cKeyA)
      {
         if (altKey && !ctrlKey && !shiftKey)
            gGame.incAIDebugType();
         else if (altKey && !ctrlKey && shiftKey)
            gGame.decAIDebugType();
         gConsoleOutput.status("AIDebugType set to %d", gGame.getAIDebugType());
         switch (gGame.getAIDebugType())
         {
            case AIDebugType::cNone: gConsole.outputStatusText(2000, 0xFFFFFFFF, cMsgDebug, "AIDebugType set to %d (None)", AIDebugType::cNone);
               break;
            case AIDebugType::cAIActiveMission: gConsole.outputStatusText(2000, 0xFFFFFFFF, cMsgDebug, "AIDebugType set to %d (AIActiveMission)", AIDebugType::cAIActiveMission);
               break;
            case AIDebugType::cAIHoverMission: gConsole.outputStatusText(2000, 0xFFFFFFFF, cMsgDebug, "AIDebugType set to %d (AIHoverMission)", AIDebugType::cAIHoverMission);
               break;
            case AIDebugType::cAIMissionTargets: gConsole.outputStatusText(2000, 0xFFFFFFFF, cMsgDebug, "AIDebugType set to %d (AIMissionTargets)", AIDebugType::cAIMissionTargets);
               break;
            case AIDebugType::cKBSquad: gConsole.outputStatusText(2000, 0xFFFFFFFF, cMsgDebug, "AIDebugType set to %d (KBSquad)", AIDebugType::cKBSquad);
               break;
            case AIDebugType::cKBBase: gConsole.outputStatusText(2000, 0xFFFFFFFF, cMsgDebug, "AIDebugType set to %d (KBBase)", AIDebugType::cKBBase);
               break;
            // Add more here.
         }
      }

      #ifndef BUILD_FINAL
      if (controlType == cKeyD && altKey && !ctrlKey && !shiftKey)
      {
         // Add resource for the leader power
//-- FIXING PREFIX BUG ID 5776
         const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
//--
         if (pPlayer)
         {
            BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
            if(pCommand)
            {
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setSenders(1, &mPlayerID);
               pCommand->setRecipientType(BCommand::cGame);
               pCommand->setType(BGameCommand::cTypeToggleAI);
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
               gConsoleOutput.status("Toggling AIs, KBs, AI Scripts.");
            }
         }
      }
      #endif

      if(altKey)
      {
         if (controlType==cKeyQ)
         {
            gGame.toggleRenderPathingQuad();
            gConsoleOutput.status("Pathing Quad render mode set to %d", gGame.getRenderPathingQuad());
            switch (gGame.getRenderPathingQuad())
            {
               case 0: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 0 (Off)."); break;
               case 1: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 1 (Conn. Info for Bucket 0)."); break;
               case 2: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 2 (Conn. Info for Bucket 1)."); break;
               case 3: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 3 (Conn. Info for Bucker 2)."); break;
               case 4: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 4 (Conn. for Bucket 0, Level 0)."); break;
               case 5: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 5 (Conn. for Bucket 0, Level 1)."); break;
               case 6: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 6 (Conn. for Bucket 0, Level 2)."); break;
               case 7: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderPathingQuad set to 7 (Conn. for Bucket 0, Level 3)."); break;
            }
         }
         else if (controlType==cKeyO)
         {
            gGame.incrementObstructionRenderMode();

            gConsoleOutput.status("Obstruction render mode set to %d", gGame.getObstructionRenderMode());
            switch (gGame.getObstructionRenderMode())
            {
               case 0: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "ObstructionRenderMode set to 0 (Off)."); break;
               case 1: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "ObstructionRenderMode set to 1 (Units)."); break;
               case 2: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "ObstructionRenderMode set to 2 (Squads)."); break;
               case 3: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "ObstructionRenderMode set to 3 (Terrain)."); break;
               case 4: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "ObstructionRenderMode set to 4 (Units & Squads)."); break;
            }
         }
         if (controlType==cKeyP)
         {
            gGame.incrementShowPaths();
            switch (gGame.getShowPaths())
            {
               case 0: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderShowPaths set to 0 (Off)."); break;
               case 1: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderShowPaths set to 1 (User Specified Path)."); break;
               case 2: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderShowPaths set to 2 (Low Level Path)."); break;
               case 3: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderShowPaths set to 3 (High Level Path)."); break;
            }
         }
      }

      if(shiftKey && !altKey && !ctrlKey)
      {
         // Add resource
         long civID=gWorld->getPlayer(mPlayerID)->getCivID();
         long statCount=gUIGame.getNumberPlayerStats(civID);         
         if(controlType>=cKeyF1 && controlType<=cKeyF9)
         {
            long resourceStat=controlType-cKeyF1;
            long counter=0;
            for(long i=0; i<statCount; i++)
            {
               const BUIGamePlayerStat* pStat=gUIGame.getPlayerStat(civID, i);
               if(pStat->mType==BUIGamePlayerStat::cTypeResource)
               {
                  if(counter==resourceStat)
                  {
                     BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
                     if(pCommand)
                     {
                        pCommand->setSenderType(BCommand::cPlayer);
                        pCommand->setSenders(1, &mPlayerID);
                        pCommand->setRecipientType(BCommand::cGame);
                        pCommand->setType(BGameCommand::cTypeAddResources);
                        pCommand->setData(pStat->mID);
                        gWorld->getCommandManager()->addCommandToExecute(pCommand);
                        gConsoleOutput.status("Adding resources to PlayerID %u", mPlayerID);
                     }
                     return true;
                  }
                  counter++;
               }
            }
            if(counter==resourceStat)
            {
               // Add resource for the leader power
//-- FIXING PREFIX BUG ID 5777
               const BPlayer* pPlayer=gWorld->getPlayer(mPlayerID);
//--
               if (pPlayer->getHaveLeaderPower())
               {
                  BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
                  if(pCommand)
                  {
                     pCommand->setSenderType(BCommand::cPlayer);
                     pCommand->setSenders(1, &mPlayerID);
                     pCommand->setRecipientType(BCommand::cGame);
                     pCommand->setType(BGameCommand::cTypeAddResources);
                     pCommand->setData(gDatabase.getLeaderPowerChargeResourceID());
                     gWorld->getCommandManager()->addCommandToExecute(pCommand);
                     gConsoleOutput.status("Adding resources to PlayerID %u", mPlayerID);
                  }
               }
               return true;
            }
         }

         if (controlType==cKeyQ)
         {
            gGame.toggleRenderLRPTreeType();
            switch (gGame.getRenderLRPTreeType())
            {
               case 0: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderLRPTreeType set to 0 (Land)."); break;
               case 1: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderLRPTreeType set to 1 (Scarab)."); break;
               case 2: gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "RenderLRPTreeType set to 2 (Flood)."); break;
            }
         }
      }

      if(ctrlKey && !altKey && !shiftKey)
      {
         if(!gLiveSystem->isMultiplayerGameActive())
         {
            // Switch player
            long playerCount=gWorld->getNumberPlayers();
            if(controlType==cKeyF10 || (controlType>=cKeyF1 && controlType<cKeyF1+playerCount))
            {
               long playerID=(controlType==cKeyF10 ? 0 : controlType-cKeyF1+1);
               BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
               if(pCommand)
               {
                  pCommand->setSenderType(BCommand::cPlayer);
                  pCommand->setSenders(1, &mPlayerID);
                  pCommand->setRecipientType(BCommand::cGame);
                  pCommand->setType(BGameCommand::cTypeSwitchPlayer);
                  pCommand->setData(playerID);
                  gWorld->getCommandManager()->addCommandToExecute(pCommand);
               }
               switchPlayer(playerID);
               gConsoleOutput.status("Switching to PlayerID %u", playerID);
               return true;
            }
         }

         // Quick build
         if(controlType==cKeyQ)
         {
            BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
            if(pCommand)
            {
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setSenders(1, &mPlayerID);
               pCommand->setRecipientType(BCommand::cGame);
               pCommand->setType(BGameCommand::cTypeQuickBuild);
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
               gConsoleOutput.status("Toggled QuickBuild");
            }
            return true;
         }

         // Camera
         if(controlType==cKeyL)
         {
            setFlagNoCameraLimits(!getFlagNoCameraLimits());
            clearCameraState();
            mScrollX=0.0f;
            mScrollY=0.0f;
            setFlagDelayScrolling(false);
            setFlagDelayScrollingStart(false);
            if(!getFlagNoCameraLimits())
            {
               applyCameraSettings(true);
            }
            gConsoleOutput.status("Camera limits %s", getFlagNoCameraLimits() ? "Disabled" : "Enabled");
            return true;
         }
         
         // Hover light 
         if (controlType==cKeyH)
         {
            mHoverLightEnabled = !mHoverLightEnabled;
            
            updateHoverLight(mHoverLightEnabled && getFlagHaveHoverPoint());
            
            gConsoleOutput.status("Hover light %s", mHoverLightEnabled ? "Enabled" : "Disabled");
         }

         // Show Weapon Ranges 
         if (controlType==cKeyZ)
         {
            mWeaponRangeDisplayEnabled = !mWeaponRangeDisplayEnabled;

            gConsoleOutput.status("Weapon Range Display %s", mWeaponRangeDisplayEnabled ? "Enabled" : "Disabled");
         }

         // Entity ID's
         if (controlType==cKeyI)
         {
            if (mShowEntityIDType==1)
               mShowEntityIDType=0;
            else
               mShowEntityIDType++;
         }
      }


      if(altKey && !ctrlKey && !shiftKey)
      {
         // Add pop
         if(controlType==cKeyF1)
         {
            long popStat=controlType-cKeyF1;
            BSimString popName = gDatabase.getPopName(popStat);
            if(popName.isEmpty() == false)
            {
               BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
               if(pCommand)
               {
                  pCommand->setSenderType(BCommand::cPlayer);
                  pCommand->setSenders(1, &mPlayerID);
                  pCommand->setRecipientType(BCommand::cGame);
                  pCommand->setType(BGameCommand::cTypeAddPopulation);
                  pCommand->setData(popStat);
                  gWorld->getCommandManager()->addCommandToExecute(pCommand);
                  gConsoleOutput.status("Adding pop to PlayerID %u", mPlayerID);
               }
               return true;
            }
         }
      }
      
      // Fog of war
      if(controlType==cKeyF6)
      {
         BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
         if(pCommand)
         {
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setSenders(1, &mPlayerID);
            pCommand->setRecipientType(BCommand::cGame);
            pCommand->setType(BGameCommand::cTypeFogOfWar);
            pCommand->setData((gConfig.isDefined(cConfigNoFogMask)?1:0));
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
         }
         if (!gLiveSystem->isMultiplayerGameActive())
         {
            gConfig.toggleDefine(cConfigNoFogMask);
            
            gConsoleOutput.status("Fog of war %s", gConfig.isDefined(cConfigNoFogMask) ? "Disabled" : "Enabled");
         }
         return true;
      }
      
      // Free camera toggle
      if (controlType==cKeyF7)
      {
         toggleFreeCamera();
         
         gConsoleOutput.status("Free camera toggled");
         
         return true;
      }

      // Deleting units
      if(controlType==cKeyDelete)
      {
         destroyAtCursor();
         return true;
      }

      // Game speed modifying
      if (controlType == cKeyG)
      {
         float gameSpeed = 1.0f;
         gConfig.get(cConfigGameSpeed, &gameSpeed);
         
         // Slow down
         if (ctrlKey)
            gameSpeed *= 0.5f;
         else
            gameSpeed *= 2.0f;
         
         const float cMaximumGameSpeed = 16.0f;
         const float cMinimumGameSpeed = 1.0f/32.0f;
         
         if (gameSpeed > cMaximumGameSpeed)
            gameSpeed = cMinimumGameSpeed;
         else if (gameSpeed < cMinimumGameSpeed) 
            gameSpeed = cMaximumGameSpeed;

         BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
         if(pCommand)
         {
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setSenders(1, &mPlayerID);
            pCommand->setRecipientType(BCommand::cGame);
            pCommand->setType(BGameCommand::cTypeGameSpeed);
            pCommand->setDataFloat(gameSpeed);
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
            gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "Gamespeed = %f", gameSpeed);
         }
      }

      // Frame rate modifying
      if (controlType == cKeyI)
      {
         // Slow down
         if (shiftKey && ctrlKey)
         {
            float targetFPS = 30.0f;
            gConfig.get(cConfigFPSLockRate, &targetFPS);
            targetFPS -= 0.5f;
            if (targetFPS < 0.5f)
               targetFPS = 0.5f;
            gConfig.set(cConfigFPSLockRate, targetFPS);            

            // Output targetFPS change to screen
            gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "FPS Lock Rate = %f", targetFPS);
         }
         // Speed up
         else if (shiftKey)
         {
            float targetFPS = 30.0f;
            gConfig.get(cConfigFPSLockRate, &targetFPS);
            targetFPS += 0.5f;
            if (targetFPS > 60.0f)
               targetFPS = 60.0f;
            gConfig.set(cConfigFPSLockRate, targetFPS);
            gParticleGateway.setTimeSpeed(targetFPS);

            // Output targetFPS change to screen
            gConsole.outputStatusText(2000, 0xffffffff, cMsgDebug, "FPS Lock Rate = %f", targetFPS);
         }
      }
   }

   // Unit pick settings modification
   if(gConfig.isDefined(cConfigDebugSelectionPicking) && !gLiveSystem->isMultiplayerGameActive() )
   {
      switch(controlType)
      {
         case cKeyDown:
            if(event==cInputEventControlStart)
            {
               if(ctrlKey)
                  mSelectionChangeZMinus=true;
               else if(altKey)
                  mObstructionChangeZMinus=true;
               else
                  mPickChangeYMinus=true;
            }
            else if(event==cInputEventControlStop)
            {
               mSelectionChangeZMinus=false;
               mObstructionChangeZMinus=false;
               mPickChangeYMinus=false;
            }
            return true;

         case cKeyUp:
            if(event==cInputEventControlStart)
            {
               if(ctrlKey)
                  mSelectionChangeZPlus=true;
               else if(altKey)
                  mObstructionChangeZPlus=true;
               else
                  mPickChangeYPlus=true;
            }
            else if(event==cInputEventControlStop)
            {
               mSelectionChangeZPlus=false;
               mObstructionChangeZPlus=false;
               mPickChangeYPlus=false;
            }
            return true;

         case cKeyRight:
            if(event==cInputEventControlStart)
            {
               if(ctrlKey)
                  mSelectionChangeXMinus=true;
               else if(altKey)
                  mObstructionChangeXMinus=true;
               else
                  mPickChangeXMinus=true;
            }
            else if(event==cInputEventControlStop)
            {
               mSelectionChangeXMinus=false;
               mObstructionChangeXMinus=false;
               mPickChangeXMinus=false;
            }
            return true;

         case cKeyLeft:
            if(event==cInputEventControlStart)
            {
               if(ctrlKey)
                  mSelectionChangeXPlus=true;
               else if(altKey)
                  mObstructionChangeXPlus=true;
               else
                  mPickChangeXPlus=true;
            }
            else if(event==cInputEventControlStop)
            {
               mSelectionChangeXPlus=false;
               mObstructionChangeXPlus=false;
               mPickChangeXPlus=false;
            }
            return true;

         case cKeyPageDown:
            if(event==cInputEventControlStart)
            {
               if(altKey)
                  mObstructionChangeYMinus=true;
            }
            else if(event==cInputEventControlStop)
            {
               mObstructionChangeYMinus=false;
            }
            return true;

         case cKeyPageUp:
            if(event==cInputEventControlStart)
            {
               if(altKey)
                  mObstructionChangeYPlus=true;
            }
            else if(event==cInputEventControlStop)
            {
               mObstructionChangeYPlus=false;
            }
            return true;
      }
   }
#endif      

   if(cUserModeCommandMenu == mUserMode)
   {
      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandIsMenuOpen, mPlayerID);          
   }
   else
   {
      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommanndIsMenuNotOpen, mPlayerID);          
   }

   if(cUserModePowerMenu == mUserMode)
   {
      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerIsMenuOpen, mPlayerID);          
   }
   else
   {
      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerIsMenuNotOpen, mPlayerID);          
   }


   // Individual mode input handling
   switch(mUserMode)
   {
      case cUserModeNormal:
         if(handleInputNormal(port, event, controlType, detail))
            return true;
         break;

      case cUserModeCircleSelecting:
         if(handleInputCircleSelect(port, event, controlType, detail))
            return true;
         break;

      case cUserModeInputUILocation:
         if(handleInputUILocation(port, event, controlType, detail))
            return(true);
         break;

      case cUserModeInputUILocationMinigame:
         if(handleInputUILocationMinigame(port, event, controlType, detail))
            return(true);
         break;

      case cUserModeInputUIUnit:
         if (handleInputUIUnit(port, event, controlType, detail))
            return(true);
         break;

      case cUserModeInputUISquad:
         if (handleInputUISquad(port, event, controlType, detail))
            return(true);
         break;

      case cUserModeInputUISquadList:
         if (handleInputUISquadList(port, event, controlType, detail))
            return (true);
         break;

      case cUserModeInputUIPlaceSquadList:
         if (handleInputUIPlaceSquadList(port, event, controlType, detail))
            return (true);
         break;

      case cUserModeRallyPoint:
         if(handleInputRallyPoint(port, event, controlType, detail))
            return true;
         break;

      case cUserModeBuildingRallyPoint:
         if(handleInputBuildingRallyPoint(port, event, controlType, detail))
            return true;
         break;

      case cUserModeBuildLocation:
         if(handleInputBuildLocation(port, event, controlType, detail))
            return true;
         break;

      case cUserModeAbility:
         if (handleInputAbility(port, event, controlType, detail))
            return(true);
         break;

      case cUserModeCommandMenu:
         if(handleInputCommandMenu(port, event, controlType, detail))
            return true;
         break;

      case cUserModePowerMenu:
         if(handleInputPowerMenu(port, event, controlType, detail))
            return true;
         break;

      case cUserModeCinematic:
      {
         if (getFlagFreeCamera())
         {
            if(handleInputFreeCamera(port, event, controlType, detail))
               return(true);
         }
         /*
         if (start && controlType == cButtonB)
         {
            gWorld->getCinematicManager()->cancelPlayback();
            return true;
         }
         */
         break;
      }

      case cUserModeFollow:
         // No inputs allowed
         return (true);
         break;

      case cUserModePower:
      {
         if (mpPowerUser)
         {
            bool rval = false;
            rval = mpPowerUser->handleInput(port, event, controlType, detail);
            
            if(mpPowerUser->getFlagDestroy())
            {
               deletePowerUser();
               resetUserMode();
               return true;
            }

            if(getFlagUpdateHoverPoint())
               updateHoverPoint(1.0f, 1.0f, true);

            return rval;
         }
         // May want to handle what happens if we're in this mode but have no mpPowerUser ptr...
         break;
      }
   }

   // Halwes - 9/17/2007 - This is a catch all, and may be overkill
   // jce [10/1/2008] -- Disabling this.  Best I can gather from talking to those involved, this was to
   // fix an issue in a scenario that doesn't work the same way any more.  The problem with this check is
   // that if you have any guys that accidentally get out of bounds, they become non-selectable and stuck out there.
   //mSelectionManager->fixupPlayableBoundsSelections();

   return false;
}

//==============================================================================
// BUser::handleInputFreeCamera
//==============================================================================
bool BUser::handleInputFreeCamera(long port, long event, long controlType, BInputEventDetail& detail)
{
   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);

   if(start || repeat || stop)
   {
      switch(controlType)
      {
         case cTriggerLeft:
         {
            mFreeCameraLModifier = stop ? false : true;
            return true;
         }
         case cTriggerRight:
         {
            mFreeCameraRModifier = stop ? false : true;
            return true;
         }
         // Move forward/backward, left/right
         case cStickLeft:
         {
            if(stop)
               mFreeCameraMovement.clear();
            else
            {
               static float cMoveDefMultiplier = 35.0f;
               static float cMoveExtraMultiplier = 3.0f;
               
               if (mFreeCameraLModifier)
                  mFreeCameraMovement.set(detail.mX, -detail.mY, 0.0f);
               else
                  mFreeCameraMovement.set(detail.mX, 0.0f, -detail.mY);
               
               mFreeCameraMovement *= cMoveDefMultiplier;
               
               if (mFreeCameraRModifier)
                  mFreeCameraMovement *= cMoveExtraMultiplier;
            }
            return true;
         }
         // Yaw/pitch
         case cStickRight:
         {
            static float cRadPerSec = 50.0f;
            static float cRotMultiplier = 2.0f;
            if(stop)
               mFreeCameraRotation.clear();
            else
               mFreeCameraRotation.set(detail.mY * cRadPerSec, detail.mX * cRadPerSec, 0.0f);
            
            if (mFreeCameraRModifier)
               mFreeCameraRotation *= cRotMultiplier;
            return true;
         }            
         case cKeyHome:
         {
            if (!stop)
               mFreeCameraReset = true;
            return true;
         }
      }
   }
   return false;
}

//==============================================================================
// BUser::handleInputScrollingAndCamera
//==============================================================================
bool BUser::handleInputScrollingAndCamera(long port, long event, long controlType, BInputEventDetail& detail)
{
   port;
   
   if (getFlagFreeCamera())
   {
      return (handleInputFreeCamera(port, event, controlType, detail));
   }

   bool start = (event == cInputEventControlStart);
   bool repeat = (event == cInputEventControlRepeat);
   bool stop = (event == cInputEventControlStop);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());
   bool modifyAction = getFlagModifierAction();

   if (start || repeat || stop)
   {
      // Translation
      if (pInputInterface->isFunctionControl( BInputInterface::cInputTranslation, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ))
      {
         bool cameraScrollEnabled = mFlagCameraScrollEnabled;
#if !defined(BUILD_FINAL)
         cameraScrollEnabled |= getFlagNoCameraLimits();
#endif
         if (stop || !cameraScrollEnabled)
         {
            mScrollX = 0.0f;
            mScrollY = 0.0f;
         }
         else
         {
            mScrollX = detail.mX;
            mScrollY = detail.mY;
            gGeneralEventManager.eventTrigger(BEventDefinitions::cControlPan, mPlayerID); 
         }
         int64 time = detail.mTime;
         float calcElapsed = 0.0f;
         if (mScrollTime > 0)
            calcElapsed = (float)((time - mScrollTime)/mTimerFrequencyFloat);
         if (mScrollX == 0.0f && mScrollY == 0.0f)
            mScrollTime = 0;
         else
            mScrollTime = time;
         updateScrolling(calcElapsed);
         //updateHoverPoint();
         //mLastCameraLoc = mpCamera->getCameraLoc();
         return (true);
      }
      // Pan or tilt or zoom
      else if (pInputInterface->isFunctionControl( BInputInterface::cInputPan, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail)  ||
               pInputInterface->isFunctionControl( BInputInterface::cInputTilt, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) ||
               pInputInterface->isFunctionControl( BInputInterface::cInputZoom, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
      {
         if (stop)
         {
            mCameraAdjustZoom = 0.0f;
            mCameraAdjustYaw  = 0.0f;
         }
         else
         {
            const float cRotationDeadzone = 0.25f;
            bool yawEnabled = (mFlagCameraYawEnabled && getOption_CameraRotationEnabled());
#if !defined(BUILD_FINAL)
            yawEnabled |= getFlagNoCameraLimits();
#endif
            if (yawEnabled && (detail.mX < -cRotationDeadzone))
            {
               mCameraAdjustYaw = (detail.mX + cRotationDeadzone) / (1.0f - cRotationDeadzone);
            }
            else if (yawEnabled && (detail.mX > cRotationDeadzone))
            {
               mCameraAdjustYaw = (detail.mX - cRotationDeadzone) / (1.0f - cRotationDeadzone);
            }
            else
            {
               mCameraAdjustYaw = 0.0f;              
            }

            const float cZoomDeadZone = 0.1f;
            bool zoomEnabled = mFlagCameraZoomEnabled;
#if !defined(BUILD_FINAL)
            zoomEnabled |= getFlagNoCameraLimits();
#endif
            if (zoomEnabled && (detail.mY < -cZoomDeadZone))
            {
               mCameraAdjustZoom = -(detail.mY + cZoomDeadZone) / (1.0f - cZoomDeadZone);
            }
            else if (zoomEnabled && (detail.mY > cZoomDeadZone))
            {
               mCameraAdjustZoom = -(detail.mY - cZoomDeadZone) / (1.0f - cZoomDeadZone);
            }
            else
            {
               mCameraAdjustZoom = 0.0f;
            }
         }
         return (true);
      }
      // Minimap Zoom
      else if( pInputInterface->isFunctionControl( BInputInterface::cInputMapZoom, controlType, modifyAction, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail ) )
      {
         gUI.playClickSound();
         gUIManager->setMinimapFullZoomOut(!gUIManager->getMinimapFullZoomOut());
         return true;
      }

      // Arrows, pageup/down, home, end do camera control unless the console is displayed
      if (!gConfig.isDefined(cConfigConsoleRenderEnable))
      {
         switch(controlType)
         {
            case cKeyRight:
               if(stop)
                  mScrollX-=1.0f;
               else if(start)
                  mScrollX+=1.0f;
               return true;

            case cKeyLeft:
               if(stop)
                  mScrollX+=1.0f;
               else if(start)
                  mScrollX-=1.0f;
               return true;

            case cKeyUp:
               if(stop)
                  mScrollY+=1.0f;
               else if(start)
                  mScrollY-=1.0f;
               return true;

            case cKeyDown:
               if(stop)
                  mScrollY-=1.0f;
               else if(start)
                  mScrollY+=1.0f;
               return true;

            case cKeyPageUp:
               if(stop)
                  mCameraAdjustZoom-=1.0f;
               else if(start)
                  mCameraAdjustZoom+=1.0f;
               return true;

            case cKeyPageDown:
               if(stop)
                  mCameraAdjustZoom+=1.0f;
               else if(start)
                  mCameraAdjustZoom-=1.0f;
               return true;

            case cKeyHome:
               if(stop)
                  mCameraAdjustFOV-=1.0f;
               else if(start)
                  mCameraAdjustFOV+=1.0f;
               return true;

            case cKeyEnd:
               if(stop)
                  mCameraAdjustFOV+=1.0f;
               else if(start)
                  mCameraAdjustFOV-=1.0f;
               return true;
         }
      }

      // Reset camera
      if (pInputInterface->isFunctionControl(BInputInterface::cInputResetCamera, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
      {
         if(start && (mUserMode != cUserModePower && mUserMode != cUserModeInputUILocation && mUserMode != cUserModeInputUILocationMinigame))
         {
            if( !gUIManager->isNonGameUIVisible() )
            {
               MVinceEventAsync_ControlUsed( this, "reset_camera_settings" );
               gGeneralEventManager.eventTrigger(BEventDefinitions::cControlResetCameraSettings, mPlayerID);
               resetCameraSettings(false);
            }
         }
         return (true);
      }
   }

   return false;
}

//==============================================================================
// BUser::handleInputNormal
//==============================================================================
bool BUser::handleInputNormal(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return (true);

   // Don't allow any stuff in locked mode.
   if (isUserLocked())
      return (false);

   bool start = (event == cInputEventControlStart);
   bool stop = (event == cInputEventControlStop);
   bool repeat = (event == cInputEventControlRepeat);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());
   bool actionModify = getFlagModifierAction();

   float multiSelectTime = 0.25f;
   gConfig.get(cConfigCircleSelectDelayTime, &multiSelectTime);

   float assignGroupTime = 0.25f;
   gConfig.get(cConfigGroupCreateTime, &assignGroupTime);

   float gotoGroupTime = 0.25f;
   gConfig.get(cConfigGroupGotoTime, &gotoGroupTime);

   float actionWithDirection = 0.25f;
   gConfig.get(cConfigActionWithDirectionHoldTime, &actionWithDirection);
   
   if( gUIManager && !gUIManager->isNonGameUIVisible() )
   {
      // Double Click Select
      if (pInputInterface->isFunctionControl(BInputInterface::cInputDoubleClickSelect, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         doubleClickSelect();    
         mFlagSkipNextSelect = true;
         mSkipNextSelectTime = 0.0f;
         gConfig.get(cConfigGamepadDoubleClickTime, &mSkipNextSelectTime);
         return (true);
      }
      // Select
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, true, -1.0f, -1.0f, &detail))
      {
         if (mSubMode == cSubModeTargetSelect)
            uiSelectTarget();
         else
         {
            if (mFlagSkipNextSelect)
               mFlagSkipNextSelect = false;
            else
            {
               setFlagCircleSelectGrow(false);
               setFlagCircleSelectFullyGrown(false);
               setFlagCircleSelectSelected(false);
               setFlagCircleSelectDoNotification(false);
               setFlagCircleSelectVinceEvent(false);

               mCircleSelectSize = mCircleSelectBaseSize;
               mCircleSelectRate = 0.0f;
               mCircleSelectOpacityFadeRate = 0.0f;

               if (actionModify)
                  setFlagCircleSelectReset(false);
               else
                  setFlagCircleSelectReset(true);

               circleSelect(false);
            }
         }
         return (true);
      }
      // Multi Select - ignore modifier action flag since this is handled in circleSelect function, also set click to happen on a stop
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputMultiSelect, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, multiSelectTime, multiSelectTime, &detail))
      {
         if (getFlagCircleSelectShrink())
            circleSelect(true, false);
         else
            circleSelect(true);

         return (true);
      }
      // Clear
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputClear, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiCancel();
         return true;
      }
      // Do work - ignore modifier action flag since this is handled in doWorkAtCurrentLocation function, also set click to happen on a stop
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputDoWork, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, true, -1.0f, -1.0f, &detail))
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
            gUI.playCantDoSound();
         else
         {
            if (!doWorkAtCurrentLocation(-1, -1, false, false, NULL))
               gUI.playCantDoSound();
         }
         return (true);
      }
      // Do work with queue
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputDoWorkQueue, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
            gUI.playCantDoSound();
         else
         {
            bool saveModifier = mFlagModifierAction;
            mFlagModifierAction = true;
            if (!doWorkAtCurrentLocation(-1, -1, false, false, NULL))
               gUI.playCantDoSound();
            mFlagModifierAction = saveModifier;
         }
         return (true);
      }
      // Attack Move.
      else if (gConfig.isDefined(cConfigHumanAttackMove) &&
         (pInputInterface->isFunctionControl(BInputInterface::cInputAttackMove, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, true, -1.0f, -1.0f, &detail)) )
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
            gUI.playCantDoSound();
         if (!doAttackMove())
            gUI.playCantDoSound();
         return (true);
      }
      // Ability
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputAbility, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, true, -1.0f, -1.0f, &detail))
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
            gUI.playCantDoSound();
         else
         {
            if (!doAbility())
               gUI.playCantDoSound();
         }
         return (true);
      }
      // Ability with queue
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputAbilityQueue, controlType, false, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
            gUI.playCantDoSound();
         else
         {
            bool saveModifier = mFlagModifierAction;
            mFlagModifierAction = true;
            if (!doAbility())
               gUI.playCantDoSound();
            mFlagModifierAction = saveModifier;
         }
         return (true);
      }
      // Powers
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputPowers, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail ))
      {
         if (!getPlayer()->getCiv()->getPowerFromHero())
         {
            showPowerMenu();
            return (true);
         }
      }
      // Assign group 1 or group 2 or group 3 or group 4
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputAssignGroup1, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, assignGroupTime, assignGroupTime, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputAssignGroup2, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, assignGroupTime, assignGroupTime, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputAssignGroup3, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, assignGroupTime, assignGroupTime, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputAssignGroup4, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, assignGroupTime, assignGroupTime, &detail))
      {
         assignGroup(controlType, &detail);
         return (true);
      }
      // Goto group 1 or group 2 or group 3 or group 4
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoGroup1, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, gotoGroupTime, gotoGroupTime, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputGotoGroup2, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, gotoGroupTime, gotoGroupTime, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputGotoGroup3, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, gotoGroupTime, gotoGroupTime, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputGotoGroup4, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, gotoGroupTime, gotoGroupTime, &detail))
      {
         gotoGroup(controlType, &detail);
         return (true);
      }
      // Select group 1 or group 2 or group 3 or group 4
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputSelectGroup1, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputSelectGroup2, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputSelectGroup3, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail) ||
         pInputInterface->isFunctionControl(BInputInterface::cInputSelectGroup4, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         selectGroup(controlType, &detail);
         return (true);
      }
      // Group add
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGroupAdd, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         groupAdd();
         return (true);
      }
      // Group next
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGroupNext, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         resetGotoSelected();
         groupNext();
         return (true);
      }
      // Group prev
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGroupPrev, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         resetGotoSelected();
         groupPrev();
         return (true);
      }
      // Group goto
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGroupGoto, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         gotoSelected();
         return (true);
      }
      // Global select
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGlobalSelect, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(true,false,true,false,false,false);
         return (true);
      }
      // Screen select
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputScreenSelect, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(false,false,true,false,false,false);
         return (true);
      }
      // New global and screen select stuff
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGlobalSelectPrev, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(true,true,false,true,false,false);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputGlobalSelectNext, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(true,false,false,false,false,false);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputScreenSelectPrev, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(false,true,false,true,false,false);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputScreenSelectNext, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(false,false,false,true,false,false);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputScreenCyclePrev, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(false,true,false,false,true,false);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputScreenCycleNext, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiSelectAll(false,false,false,false,true,false);
         return (true);
      }
      // Sub selection
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputSubSelectPrev, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiCycleGroup(true, true);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputSubSelectNext, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiCycleGroup(false, true);
         return (true);
      }
      // Target cycling
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputTargetPrev, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiCycleTarget(true, true);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputTargetNext, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         uiCycleTarget(false, true);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputFlareLook, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         sendFlare(BUIGame::cFlareLook, mHoverPoint);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputFlareHelp, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         sendFlare(BUIGame::cFlareHelp, mHoverPoint);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputFlareMeet, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         sendFlare(BUIGame::cFlareMeet, mHoverPoint);
         return (true);
      }
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputFlareAttack, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         sendFlare(BUIGame::cFlareAttack, mHoverPoint);
         return (true);
      }
      // Set Rally
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputSetRally, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
      {
         // Reset delayed flare since flare and rally are on same button
         mFlagDelayFlare = false;

         // Check for special entities that the rally point can be tasked on
         BEntityID rallyPointEntityID = cInvalidObjectID;
         if (mHoverObject != cInvalidObjectID)
         {
//-- FIXING PREFIX BUG ID 5780
            const BEntity* pHoverObj = gWorld->getEntity(mHoverObject);
//--
            if (pHoverObj && pHoverObj->getProtoObject() && pHoverObj->getProtoObject()->getFlagCanSetAsRallyPoint())
               rallyPointEntityID = mHoverObject;
         }

         long obstructionQuadTrees = 
            BObstructionManager::cIsNewTypeCollidableNonMovableUnit | // Unit that can't move                                                       
            BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement

         // Test exit position
         BOOL bObstructed = gObsManager.testObstructions( mHoverPoint, 1.0f, 1.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, mPlayerID );

         if(bObstructed && (rallyPointEntityID == cInvalidObjectID))
         {
            gUI.playCantDoSound();
            return true;
         }

         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return true;
         }

         if (getFlagHoverPointOverTerrain())
         {
            gUI.playClickSound();
            BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
            if(pCommand)
            {
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setSenders(1, &mPlayerID);
               pCommand->setRecipientType(BCommand::cGame);
               pCommand->setType(BGameCommand::cTypeSetGlobalRallyPoint);
               pCommand->setPosition(mHoverPoint);
               pCommand->setData(rallyPointEntityID.asLong());
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
               MVinceEventAsync_ControlUsed( this, "set_global_rally_point2" );

               gGeneralEventManager.eventTrigger(BEventDefinitions::cControlSetRallyPoint, mPlayerID);
            }
         }
         else
            gUI.playCantDoSound();
         return (true);
      }
   }

   // Handle goto actions
   if (handleInputGoto(port, event, controlType, detail))
      return (true);

   return (false);
}

//==============================================================================
// BUser::handleInputGoto
//==============================================================================
bool BUser::handleInputGoto(long port, long event, long controlType, BInputEventDetail& detail)
{
   bool start = (event == cInputEventControlStart);
   bool stop = (event == cInputEventControlStop);
   bool repeat = (event == cInputEventControlRepeat);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());
   bool actionModify = getFlagModifierAction();

   // Goto base
   if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoBase, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      if (gotoItem(cGotoTypeBase, true))
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
         }
         gotoItem(cGotoTypeBase, false);
      }
      return (true);
   }
   // Goto alert
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoAlert, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      #ifndef BUILD_FINAL
      // jce [9/12/2008] -- If paused, and noGameMenu is turned on, then we are probably wanting the right dpad to single-step and
      // not jump our camera around.
      if(gConfig.isDefined("noGameMenu") && gModeManager.getModeGame() && gModeManager.getModeGame()->getPaused())
         return(true);
      #endif
      if (getPlayer()->getAlertManager()->peekGotoAlert())
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
         }
         gotoAlert();
      }
      return (true);
   }
   // Goto scout
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoScout, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      if (gotoItem(cGotoTypeScout, true))
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
         }
         gotoItem(cGotoTypeScout, false);
      }
      return (true);
   }
   // Goto army
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoArmy, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      if (findCrowd(cFindCrowdMilitary, false, true, NULL))
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
         }
         findCrowd(cFindCrowdMilitary, false, false, NULL);
      }
      return (true);
   }
   // Goto node
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoNode, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      if (gotoItem(cGotoTypeNode, true))
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
         }
         gotoItem(cGotoTypeNode, false);
      }
      return (true);
   }
   // Goto hero
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoHero, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      if (gotoItem(cGotoTypeHero, true))
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
         }
         gotoItem(cGotoTypeHero, false);
      }
      return (true);
   }
   // Goto rally point
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoRally, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
//-- FIXING PREFIX BUG ID 5781
      const BPlayer* pPlayer=getPlayer();
//--
      if (pPlayer->haveRallyPoint())
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
         }
         gUI.playClickSound();
         BVector pos=pPlayer->getRallyPoint()-(mpCamera->getCameraDir()*mCameraZoom);
         mpCamera->setCameraLoc(pos);
         tieToCameraRep();
         setFlagUpdateHoverPoint(true);
         //mSelectionManager->clearSelections();
         if( !gUIManager->isNonGameUIVisible() )
         {
            MVinceEventAsync_ControlUsed( this, "goto_rally" );
            gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoRally, mPlayerID);
         }
      }
      else
         gUI.playCantDoSound();
      return (true);
   }
   // Goto selected
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputGotoSelected, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail))
   {
      if (mSelectionManager->getNumberSelectedUnits() > 0)
      {
         if( !gUIManager->isNonGameUIVisible() )
         {
            autoExitSubMode();
            changeMode(cUserModeNormal);
            gotoSelected();
         }
      }
      return (true);
   }
   // Goto hero for power
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputPowers, controlType, actionModify, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail ))
   {
      if (getPlayer()->getCiv()->getPowerFromHero())
      {
         if (gotoItem(cGotoTypeHero, true))
         {
            if( !gUIManager->isNonGameUIVisible() )
            {
               autoExitSubMode();
               changeMode(cUserModeNormal);
            }
            gotoItem(cGotoTypeHero, false);
         }
         return (true);
      }
   }

   return (false);
}

//==============================================================================
// BUser::handleInputCircleSelect
//==============================================================================
bool BUser::handleInputCircleSelect(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return true;

   bool stop=(event==cInputEventControlStop);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
   // Test if multi select is finished
   if( pInputInterface->isFunctionControl( BInputInterface::cInputMultiSelect, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( stop )
         circleSelect( false );
      
      return( true );      
   }
   

   return false;
}

//==============================================================================
// BUser::handleInputUILocation
//==============================================================================
bool BUser::handleInputUILocation(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return(true);

   bool start = (event == cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
   bool             modifyAction    = getFlagModifierAction();

   // Selection or do work
   if( pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) || 
       pInputInterface->isFunctionControl( BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( start )
      {
         if( mPlayerState != BPlayer::cPlayerStatePlaying )
         {
            gUI.playCantDoSound();
            return( true );
         }

         if( !getFlagHoverPointOverTerrain() )
         {
            gUI.playCantDoSound();
            return( true );
         }

         DWORD flags = 0;
         if (mUILocationCheckObstruction)
         {
            flags = BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain;
            if (mUILocationCheckMoving)
               flags &= ~BWorld::cCPIgnoreMoving;
         }

         if (mUILocationLOSCenterOnly)
         {
            flags |= BWorld::cCPLOSCenterOnly;
         }

         if (mBuildProtoID == -1)
         {
            flags |= BWorld::cCPIgnorePlacementRules;
         }

         if (mUILocationPlacementRuleIndex != -1)
         {
            flags &= ~BWorld::cCPIgnorePlacementRules;
         }

         long searchScale = 1;
         if (mUILocationPlacementSuggestion)
         {
            flags |= BWorld::cCPSetPlacementSuggestion;

            // Calc search scale based on power radius
            // MPB [3/14/2008] - This search scale really needs to take the obstruction size into account
            // because it is mulitpled by an obs size based tile count inside checkPlacement.  Better yet,
            // checkPlacement should probably just calc searchSize differently
            if (mUIPowerRadius > 0.0f)
               searchScale = Math::Max(1, Math::FloatToIntTrunc(mUIPowerRadius * gTerrainSimRep.getReciprocalDataTileScale()));
         }

         if (!gWorld->checkPlacement(mBuildProtoID, mPlayerID, mHoverPoint, mPlacementSuggestion, cZAxisVector, mUILocationLOSType, flags, searchScale, NULL, NULL, mUILocationPlacementRuleIndex, mBuildProtoSquadID, mBuildUseSquadObstruction))
         {
            gUI.playCantDoSound();
            return (true);
         }
         gUI.playClickSound();

         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand( mPlayerID, cCommandTrigger );
         if( !c )
         {
            return( true );
         }
       
         // Set up the command.
         c->setSenders( 1, &mPlayerID );
         c->setSenderType( BCommand::cPlayer );
         c->setRecipientType( BCommand::cPlayer );
         c->setType( BTriggerCommand::cTypeBroadcastInputUILocationResult );
         // Register the correct trigger system and variable this stuff goes into.
         c->setTriggerScriptID( mTriggerScriptID );
         c->setTriggerVarID( mTriggerVarID );
         // Set the data that will be poked in.
         c->setInputResult( BTriggerVarUILocation::cUILocationResultOK );
         if (mUILocationPlacementSuggestion)
            c->setInputLocation( mPlacementSuggestion );
         else
            c->setInputLocation( mHoverPoint );
         // Ok rock on.
         gWorld->getCommandManager()->addCommandToExecute( c );
         changeMode( cUserModeNormal );         
      }
      return( true );
   }
   // Clear
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( start )
      {
         gUI.playClickSound();
         resetUserMode();
      }
      return( true );
   }

   return( false );
}

//==============================================================================
// BUser::handleInputUILocationMinigame
//==============================================================================
bool BUser::handleInputUILocationMinigame(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Camera scrolling
   if (mMinigame.isScrolling())
   {
      if(handleInputScrollingAndCamera(port, event, controlType, detail))
         return(true);
   }

   bool start = (event == cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
   bool             modifyAction    = getFlagModifierAction();

   // Minigame input
   if (mMinigame.handleInput(this, port, event, controlType, detail))
      return true;

   // Standard handling like UILocation
   if( pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) || 
       pInputInterface->isFunctionControl( BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( start )
      {
         if( mPlayerState != BPlayer::cPlayerStatePlaying )
         {
            gUI.playCantDoSound();
            return( true );
         }

         if( !getFlagHoverPointOverTerrain() )
         {
            gUI.playCantDoSound();
            return( true );
         }

         DWORD flags = 0;
         if (mUILocationCheckObstruction)
         {
            flags = BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain;
            if (mUILocationCheckMoving)
               flags &= ~BWorld::cCPIgnoreMoving;
         }

         if (mUILocationLOSCenterOnly)
         {
            flags |= BWorld::cCPLOSCenterOnly;
         }

         if (mBuildProtoID == -1)
         {
            flags |= BWorld::cCPIgnorePlacementRules;
         }

         if (mUILocationPlacementRuleIndex != -1)
         {
            flags &= ~BWorld::cCPIgnorePlacementRules;
         }

         long searchScale = 1;
         if (mUILocationPlacementSuggestion)
         {
            flags |= BWorld::cCPSetPlacementSuggestion;

            // Calc search scale based on power radius
            // MPB [3/14/2008] - This search scale really needs to take the obstruction size into account
            // because it is mulitpled by an obs size based tile count inside checkPlacement.  Better yet,
            // checkPlacement should probably just calc searchSize differently
            if (mUIPowerRadius > 0.0f)
               searchScale = Math::Max(1, Math::FloatToIntTrunc(mUIPowerRadius * gTerrainSimRep.getReciprocalDataTileScale()));
         }

         if (!gWorld->checkPlacement(mBuildProtoID, mPlayerID, mHoverPoint, mPlacementSuggestion, cZAxisVector, mUILocationLOSType, flags, searchScale, NULL, NULL, mUILocationPlacementRuleIndex, mBuildProtoSquadID, mBuildUseSquadObstruction))
         {
            gUI.playCantDoSound();
            return (true);
         }
         gUI.playClickSound();

         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand( mPlayerID, cCommandTrigger );
         if( !c )
         {
            return( true );
         }
       
         // Set up the command.
         c->setSenders( 1, &mPlayerID );
         c->setSenderType( BCommand::cPlayer );
         c->setRecipientType( BCommand::cPlayer );
         c->setType( BTriggerCommand::cTypeBroadcastInputUILocationMinigameResult );
         // Register the correct trigger system and variable this stuff goes into.
         c->setTriggerScriptID( mTriggerScriptID );
         c->setTriggerVarID( mTriggerVarID );
         // Set the data that will be poked in.
         c->setInputResult( BTriggerVarUILocation::cUILocationResultOK );
         if (mUILocationPlacementSuggestion)
            c->setInputLocation( mPlacementSuggestion );
         else
            c->setInputLocation( mHoverPoint );
         c->setInputQuality(1.0f);
         // Ok rock on.
         gWorld->getCommandManager()->addCommandToExecute( c );
         changeMode( cUserModeNormal );         
      }
      return( true );
   }
   // Clear
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if( start )
      {
         gUI.playClickSound();
         mMinigame.stop();
         resetUserMode();
      }
      return( true );
   }

   return( false );
}

//==============================================================================
// BUser::handleInputUIUnit
//==============================================================================
bool BUser::handleInputUIUnit(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return true;

   bool start=(event==cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
   bool             modifyAction    = getFlagModifierAction();

   // Selection or do work
   if( pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) || 
       pInputInterface->isFunctionControl( BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if(start)
      {
         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return true;
         }

         BUnit *pUnit = gWorld->getUnit(mHoverObject);
         if (!testUnitAgainstInputUIModeFilters(pUnit))
         {
            gUI.playCantDoSound();
            return(true);
         }

         gUI.playClickSound();

         BTriggerCommand *c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (!c)
            return(true);
         // Set up the command.
         c->setSenders(1, &mPlayerID);
         c->setSenderType(BCommand::cPlayer);
         c->setRecipientType(BCommand::cPlayer);
         c->setType(BTriggerCommand::cTypeBroadcastInputUIUnitResult);
         // Register the correct trigger system and variable this stuff goes into.
         c->setTriggerScriptID(mTriggerScriptID);
         c->setTriggerVarID(mTriggerVarID);
         // Set the data that will be poked in.
         c->setInputResult(BTriggerVarUIUnit::cUIUnitResultOK);
         c->setInputUnit(pUnit->getID());
         // Ok rock on.
         gWorld->getCommandManager()->addCommandToExecute(c);
         changeMode(cUserModeNormal);
      }
      return true;
   }
   // Clear
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if(start)
      {
         gUI.playClickSound();
         resetUserMode();
      }
      return true;
   }

   return false;
}


//==============================================================================
// BUser::handleInputUISquad
//==============================================================================
bool BUser::handleInputUISquad(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return true;

   bool start=(event==cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
   bool             modifyAction    = getFlagModifierAction();

   // Selection or do work
   if( pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) || 
       pInputInterface->isFunctionControl( BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if(start)
      {
         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return true;
         }

         BUnit *pUnit = gWorld->getUnit(mHoverObject);
         BSquad *pSquad = (pUnit) ? pUnit->getParentSquad() : NULL;
         if (!testSquadAgainstInputUIModeFilters(pSquad))
         {
            gUI.playCantDoSound();
            return(true);
         }

         // Check dynamic cost
         if (mUIProtoPowerID != -1)
         {
            BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mUIProtoPowerID);
            if (pProtoPower)
            {
//-- FIXING PREFIX BUG ID 5783
               const BCost* pCost = pProtoPower->getCost(pSquad->getProtoObject());
//--
               if (!getPlayer()->checkCost(pCost))
               {
                  gUI.playCantDoSound();
                  return(true);
               }
            }
         }

         gUI.playClickSound();

         BTriggerCommand *c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (!c)
            return(true);
         // Set up the command.
         c->setSenders(1, &mPlayerID);
         c->setSenderType(BCommand::cPlayer);
         c->setRecipientType(BCommand::cPlayer);
         c->setType(BTriggerCommand::cTypeBroadcastInputUISquadResult);
         // Register the correct trigger system and variable this stuff goes into.
         c->setTriggerScriptID(mTriggerScriptID);
         c->setTriggerVarID(mTriggerVarID);
         // Set the data that will be poked in.
         c->setInputResult(BTriggerVarUISquad::cUISquadResultOK);
         c->setInputSquad(pSquad->getID());
         // Ok rock on.
         gWorld->getCommandManager()->addCommandToExecute(c);
         changeMode(cUserModeNormal);
      }
      return true;
   }
   // Clear
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail )  )
   {
      if(start)
      {
         gUI.playClickSound();
         resetUserMode();
      }
      return true;
   }

   return false;
}

//==============================================================================
// Handle controller inputs when in input UI squad list mode
//==============================================================================
bool BUser::handleInputUISquadList(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (handleInputScrollingAndCamera(port, event, controlType, detail))
      return (true);

   bool start = (event == cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());
   bool modifyAction = getFlagModifierAction();   

   // Selection or do work
   if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) || 
       pInputInterface->isFunctionControl(BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (start)
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return (true);
         }

         if (mSelectionManager->getNumberSelectedSquads() == 0)
         {
            gUI.playCantDoSound();
            return (true);
         }

         gUI.playClickSound();
         
         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (!c)
            return (true);
         // Set up the command.
         c->setSenders(1, &mPlayerID);
         c->setSenderType(BCommand::cPlayer);
         c->setRecipientType(BCommand::cPlayer);
         c->setType(BTriggerCommand::cTypeBroadcastInputUISquadListResult);
         // Register the correct trigger system and variable this stuff goes into.
         c->setTriggerScriptID(mTriggerScriptID);
         c->setTriggerVarID(mTriggerVarID);
         // Set the data that will be poked in.
         c->setInputResult(BTriggerVarUISquad::cUISquadResultOK);
         //c->setInputSquadList(squadList);
         c->setInputSquadList(mSelectionManager->getSelectedSquads());         
         // Ok rock on.
         gWorld->getCommandManager()->addCommandToExecute(c);
         clearAllSelections();
         changeMode(cUserModeNormal);
      }
      return (true);
   }
   // Clear
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail )  )
   {
      if (start)
      {
         gUI.playClickSound();
         resetUserMode();
      }
      return (true);
   }

   return (false);
}

//==============================================================================
// Handle the inputs when placing squads
//==============================================================================
bool BUser::handleInputUIPlaceSquadList(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return(true);

   bool start = (event==cInputEventControlStart);
   bool stop = (event == cInputEventControlStop);
   bool repeat = (event == cInputEventControlRepeat);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());
   bool modifyAction = getFlagModifierAction();

   // Selection or do work
   if (pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) || 
       pInputInterface->isFunctionControl( BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (start)
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return (true);
         }

         if (!getFlagUILocationValid())
         {
            gUI.playCantDoSound();
            return (true);
         }
         
         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (!c)
         {
            return (true);
         }

         // Set up the command.
         c->setSenders(1, &mPlayerID);
         c->setSenderType(BCommand::cPlayer);
         c->setRecipientType(BCommand::cPlayer);
         c->setType(BTriggerCommand::cTypeBroadcastInputUIPlaceSquadListResult);
         // Register the correct trigger system and variable this stuff goes into.
         c->setTriggerScriptID(mTriggerScriptID);
         c->setTriggerVarID(mTriggerVarID);
         // Set the data that will be poked in.
         c->setInputResult(BTriggerVarUILocation::cUILocationResultOK);
         c->setInputLocation(mHoverPoint);

         // Ok rock on.
         gUI.playClickSound();
         gWorld->getCommandManager()->addCommandToExecute(c);
         changeMode(cUserModeNormal);         
      }
      return (true);
   }
   // Clear
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) ||
            pInputInterface->isFunctionControl(BInputInterface::cInputScreenSelect, controlType, modifyAction, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail ) ||
            pInputInterface->isFunctionControl(BInputInterface::cInputGlobalSelect, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) )
   {
      if (start)
      {
         gUI.playClickSound();

         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (!c)
         {
            return (true);
         }

         // Set up the command.
         c->setSenders(1, &mPlayerID);
         c->setSenderType(BCommand::cPlayer);
         c->setRecipientType(BCommand::cPlayer);
         c->setType(BTriggerCommand::cTypeBroadcastInputUIPlaceSquadListResult);
         // Register the correct trigger system and variable this stuff goes into.
         c->setTriggerScriptID(mTriggerScriptID);
         c->setTriggerVarID(mTriggerVarID);
         // Set the data that will be poked in.
         c->setInputResult(BTriggerVarUILocation::cUILocationResultCancel);
         c->setInputLocation(cInvalidVector);
         c->setInputSquadList(mInputUIModeSquadList);

         // No input location because we canceled.
         // OK rock on.
         gWorld->getCommandManager()->addCommandToExecute(c);
         changeMode(cUserModeNormal);         
      }
      return (true);
   }

   return (false);
}

//==============================================================================
// BUser::handleInputRallyPoint
//==============================================================================
bool BUser::handleInputRallyPoint(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return true;

   bool start=(event==cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );

   // Selection or do work
   if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) || 
       pInputInterface->isFunctionControl(BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if(start)
      {
         // Check for special entities that the rally point can be tasked on
         BEntityID rallyPointEntityID = cInvalidObjectID;
         if (mHoverObject != cInvalidObjectID)
         {
//-- FIXING PREFIX BUG ID 5785
            const BEntity* pHoverObj = gWorld->getEntity(mHoverObject);
//--
            if (pHoverObj && pHoverObj->getProtoObject() && pHoverObj->getProtoObject()->getFlagCanSetAsRallyPoint())
               rallyPointEntityID = mHoverObject;
         }
/*
         // 9/22/08 BSR: Commented out this block to prevent players from deducing where their opponents are building through black map
         long obstructionQuadTrees = 
            BObstructionManager::cIsNewTypeCollidableNonMovableUnit | // Unit that can't move                                                       
            BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement

         // Test exit position
         BOOL bObstructed = gObsManager.testObstructions( mHoverPoint, 1.0f, 1.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, mPlayerID );

         if(bObstructed && (rallyPointEntityID == cInvalidObjectID))
*/
         if((mHoverObject != cInvalidObjectID) && (rallyPointEntityID == cInvalidObjectID))
         {
            gUI.playCantDoSound();
            return false;
         }

         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            resetUserMode();
            gUI.playCantDoSound();
            return true;
         }

         if (getFlagHoverPointOverTerrain())
         {
            gUI.playClickSound();
            BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
            if(pCommand)
            {
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setSenders(1, &mPlayerID);
               pCommand->setRecipientType(BCommand::cGame);
               pCommand->setType(BGameCommand::cTypeSetGlobalRallyPoint);
               pCommand->setPosition(mHoverPoint);
               pCommand->setData(rallyPointEntityID.asLong());
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
               MVinceEventAsync_ControlUsed( this, "set_global_rally_point" );

               gGeneralEventManager.eventTrigger(BEventDefinitions::cControlSetRallyPoint, mPlayerID);

            }
            releaseHoverVisual();
            changeMode(cUserModeNormal);
         }
         else
            gUI.playCantDoSound();
      }
      return true;
   }
   // Clear
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if(start)
         resetUserMode();
      return true;
   }

   return false;
}

//==============================================================================
// BUser::handleInputBuildingRallyPoint
//==============================================================================
bool BUser::handleInputBuildingRallyPoint(long port, long event, long controlType, BInputEventDetail& detail)
{
   if(handleInputScrollingAndCamera(port, event, controlType, detail))
      return true;

   bool start=(event==cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );

   // Selection or do work
   if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) || 
      pInputInterface->isFunctionControl(BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if(start)
      {
         // Check for special entities that the rally point can be tasked on
         BEntityID rallyPointEntityID = cInvalidObjectID;
         if (mHoverObject != cInvalidObjectID)
         {
//-- FIXING PREFIX BUG ID 5786
            const BEntity* pHoverObj = gWorld->getEntity(mHoverObject);
//--
            if (pHoverObj && pHoverObj->getProtoObject() && pHoverObj->getProtoObject()->getFlagCanSetAsRallyPoint())
               rallyPointEntityID = mHoverObject;
         }

         long obstructionQuadTrees = 
            BObstructionManager::cIsNewTypeCollidableNonMovableUnit | // Unit that can't move                                                       
            BObstructionManager::cIsNewTypeBlockLandUnits;            // Terrain that blocks any combo of movement that includes land-based movement

         // Test exit position
         BOOL bObstructed = gObsManager.testObstructions( mHoverPoint, 1.0f, 1.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, mPlayerID );

         if(bObstructed && (rallyPointEntityID == cInvalidObjectID))
         {
            gUI.playCantDoSound();
            return false;
         }

         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            resetUserMode();
            gUI.playCantDoSound();
            return true;
         }

         if (getFlagHoverPointOverTerrain())
         {
            gUI.playClickSound();
            BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
            if(pCommand)
            {
               pCommand->setSenderType(BCommand::cPlayer);
               pCommand->setSenders(1, &mPlayerID);
               pCommand->setRecipientType(BCommand::cGame);
               pCommand->setType(BGameCommand::cTypeSetBuildingRallyPoint);
               pCommand->setData(mCommandObject.asLong());
               pCommand->setPosition(mHoverPoint);
               pCommand->setData2(rallyPointEntityID.asLong());
               gWorld->getCommandManager()->addCommandToExecute(pCommand);
               MVinceEventAsync_ControlUsed( this, "set_base_rally_point" );
            }
            releaseHoverVisual();
            changeMode(cUserModeNormal);
         }
         else
            gUI.playCantDoSound();
      }
      return true;
   }
   // Clear
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
   {
      if(start)
         resetUserMode();
      return true;
   }

   return false;
}

//==============================================================================
// BUser::handleInputBuildLocation
//==============================================================================
bool BUser::handleInputBuildLocation(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (handleInputScrollingAndCamera(port, event, controlType, detail))
   {
      return (true);
   }

   bool start = (event == cInputEventControlStart);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());
   bool modifyAction = getFlagModifierAction();

   if (mSubMode!=cSubModeUnpack)
   {
      // If Modifier Action is active, make sure we can still build this
      if (getFlagModifierAction())
      {
         // See if this action is still available to use again
         const BUnit* pBuilder=gWorld->getUnit(mCommandObject);
         if(pBuilder)
         {
            const BProtoObject* pBuilderProtoObject = pBuilder->getProtoObject();
            if(pBuilderProtoObject)
            {
               uint commandCount = pBuilderProtoObject->getNumberCommands();

               for(uint i=0; i<commandCount; i++)
               {
                  BProtoObjectCommand command=pBuilderProtoObject->getCommand(i);
                  if (command.getType() ==  BProtoObjectCommand::cTypeBuild &&
                     command.getID() == mBuildProtoID &&
                     !pBuilderProtoObject->getCommandAvailable(i))
                  {
                     releaseHoverVisual();
                     changeMode(cUserModeNormal);
                  }
               }
            }
         }
      }
   }

   // Selection or do work - ignore modifier action as this is handled in doManualBuild function
   if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) || 
       pInputInterface->isFunctionControl(BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (start)
      {
         if (mPlayerState != BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return (true);
         }

         // Calc forward vector
         BVector forward=cZAxisVector;
         if (mSubMode==cSubModeUnpack)
         {
            float angle = (mCameraYaw == mBuildBaseYaw ? 0.0f : mCameraYaw - mBuildBaseYaw);
            if (angle != 0.0f)
            {
               forward.rotateXZ(Math::fDegToRad(angle));
               forward.normalize();
            }
         }

         // check placement
         const DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPSetPlacementSuggestion | BWorld::cCPPushOffTerrain;
         BPlayerID playerID=(mCoopPlayerID==cInvalidPlayerID ? mPlayerID : mCoopPlayerID);
         const BUnit* pBuilder=gWorld->getUnit(mCommandObject);
         if (!(getFlagHoverPointOverTerrain() && gWorld->checkPlacement(mBuildProtoID,  playerID, mHoverPoint, mPlacementSuggestion, forward, BWorld::cCPLOSDontCare, flags, 1, pBuilder, &mPlacementSocketID)))
         {
            gUI.playCantDoSound();
            return (true);
         }

         if (mSubMode==cSubModeUnpack)
         {
            // unpack
            if (!doUnpack())
            {
               gUI.playCantDoSound();
               return (false);
            }
            mAbilityID = -1;
            releaseHoverVisual();
            changeMode(cUserModeNormal);
         }
         else
         {
            // build
//-- FIXING PREFIX BUG ID 5787
            const BProtoObject* pProto=getPlayer()->getProtoObject(mBuildProtoID);
//--
            if (pProto)
            {
               if (pProto->getFlagManualBuild())
               {
                  if (!doManualBuild())
                  {
                     gUI.playCantDoSound();
                     return (false);
                  }
               }
               else
               {
                  // click
                  gUI.playClickSound();

                  BBuildingCommand* pCommand = (BBuildingCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandBuilding);
                  if (pCommand)
                  {
                     pCommand->setSenderType(BCommand::cPlayer);
                     pCommand->setSenders(1, &mPlayerID);
                     pCommand->setRecipientType(BCommand::cUnit);
                     pCommand->setRecipients(1, &mCommandObject);
                     pCommand->setType(BProtoObjectCommand::cTypeBuild);
                     pCommand->setTargetID(mBuildProtoID);
                     pCommand->setTargetPosition(mPlacementSuggestion);
                     pCommand->setSocketID(mPlacementSocketID);    
                     gWorld->getCommandManager()->addCommandToExecute(pCommand);
                     // Halwes - 10/26/2006 - Desired?
                     //MVinceEventAsync_ControlUsed( this, "build_location" );
                  }
               }
            }

            if (mAbilityID != -1)
            {
               mAbilityID = -1;
               releaseHoverVisual();
               changeMode(cUserModeNormal);
            }
            else
            {
               if (getFlagModifierAction())
                  setFlagExitModeOnModifierRelease(true);
               else
               {
                  releaseHoverVisual();
                  changeMode(cUserModeNormal);
               }
            }
         }
      }
      return (true);
   }
   // Clear
   else if (pInputInterface->isFunctionControl(BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
   {
      if (start)
         resetUserMode();
      return (true);
   }

   return (false);
}

//==============================================================================
// BUser::handleInputAbility
//==============================================================================
bool BUser::handleInputAbility(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (handleInputScrollingAndCamera(port, event, controlType, detail))
   {
      return (true);
   }

   if (event == cInputEventControlStart)
   {
      BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get(getOption_ControlScheme());
      bool modifyAction = getFlagModifierAction();

      // Selection or do work
      if (pInputInterface->isFunctionControl(BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail) || 
          pInputInterface->isFunctionControl(BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
      {
         bool valid = true;
//-- FIXING PREFIX BUG ID 5774
         const BAbility* pAbility = gDatabase.getAbilityFromID(mAbilityID);
//--
         if(pAbility && pAbility->getTargetType()==BAbility::cTargetUnit)
         {
            if(mHoverType == cHoverTypeNone || mHoverType == cHoverTypeSelect)
            {
               valid = false;
            }
         }
         else
         {
            if (!getFlagHoverPointOverTerrain())
            {
               valid = false;
            }
         }

         if (valid)
         {            
            if (doWorkAtCurrentLocation(mAbilityID, -1, false, false, NULL))
            {
               gUI.playClickSound();
            }
            else
            {
               gUI.playCantDoSound();
               return (false);
            }
            setAbilityID(-1);
            changeMode(cUserModeNormal);
         }
         else
         {
            gUI.playCantDoSound();
         }
         return (true);
      }
      // Clear
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
      {
         setAbilityID(-1);
         changeMode(cUserModeNormal);
         return (true);
      }
   }

   return (false);
}

//==============================================================================
// BUser::handleInputCommandMenu
//==============================================================================
bool BUser::handleInputCommandMenu(long port, long event, long controlType, BInputEventDetail& detail)
{
   // Make sure the command menu is up to date
   refreshCommandMenu();

   // Just in case...
   if (mUserMode != cUserModeCommandMenu)
      return false;

   if (controlType == cStickLeft)
   {
      mScrollXDelay = detail.mX;
      mScrollYDelay = detail.mY;

      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuNavCommand, mPlayerID); 
   }

   bool bSuccessFlash = false;   
   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      bSuccessFlash = mpUIContext->handleCircleMenuInput(port, (BInputEventType) event, (BInputControlType) controlType, detail);
   }

   bool bSuccess2D = mCircleMenu.handleInput(port, event, controlType, detail);      

   //if (bSuccess2D || bSuccess3D)
   if (bSuccess2D)
      return true;

   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);

   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
   bool             modifyAction    = getFlagModifierAction();

   int focusIndex = mCircleMenu.getCurrentItemOrder();
   
   if(stop)
   {
      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocusN, mPlayerID, mCommandObject, -1);          
   }

   if (start || repeat)
   {      

      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocusN, mPlayerID, mCommandObject, focusIndex);          


      switch (focusIndex)
      {
         case 0:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus0, mPlayerID);          
            break;

         case 1:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus1, mPlayerID); 
            break;

         case 2:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus2, mPlayerID); 
            break;

         case 3:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus3, mPlayerID); 
            break;

         case 4:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus4, mPlayerID); 
            break;

         case 5:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus5, mPlayerID); 
            break;

         case 6:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus6, mPlayerID); 
            break;

         case 7:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandHasFocus7, mPlayerID); 
            break;
      }   
   }

   if(start)
   {
      // Selection
      if( pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
      {
         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return true;
         }

         long currentItemIndex = mCircleMenu.getCurrentItemIndex();
         if(currentItemIndex != -1)
         {
            if(mCircleMenu.getItemUnavailable(currentItemIndex))
            {
               BPlayer* pPlayer = getPlayer();
//-- FIXING PREFIX BUG ID 5800
               const BCost* pCost = mCircleMenu.getCurrentItemCost();
//--
//-- FIXING PREFIX BUG ID 5801
               const BPopArray* pPops = mCircleMenu.getCurrentItemPops();
//--
               if (pPlayer && (!pPlayer->checkCost(mCircleMenu.getCurrentItemCost()) || !pPlayer->checkPops(pPops)))
               {
                  BSimHelper::playSPCCostErrorSound(mPlayerID, pCost, pPops);
               }
               else
               {
                  gUI.playCantDoSound();
               }

               return (true);
            }

            long currentItemID = mCircleMenu.getCurrentItemID();
            BProtoObjectCommand command;
            command.set(currentItemID);
            long type=command.getType();
            long id=command.getID();

            if (currentItemID==gDatabase.getPPIDRallyPoint())
            {
               // Rally Point
               long id=getPlayer()->getCiv()->getRallyPointObjectID();
               if(id!=-1)
                  setHoverVisual(id);
               changeMode(cUserModeRallyPoint);
               doScrollDelay();
               return true;
            }
            else if (type==BProtoObjectCommand::cTypeTribute)
            {
               doTribute(command);
               return true;
            }
            else if (type==BProtoObjectCommand::cTypeReverseHotDrop)
            {
               doReverseHotDrop(command);
               return true;
            }
            else if(type==BProtoObjectCommand::cTypeResearch)
            {
               if(gWorld->getPlayer(mPlayerID)->getTechTree()->getTechStatus(id, mCommandObject.asLong())!=BTechTree::cStatusAvailable)
               {
                  gUI.playCantDoSound();
                  return true;
               }
//-- FIXING PREFIX BUG ID 5782
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if(pUnit && pUnit->getResearchCount(mPlayerID, id)>0)
               {
                  gUI.playCantDoSound();
                  return true;
               }
            }
            else if(type==BProtoObjectCommand::cTypeTrainUnit)
            {
//-- FIXING PREFIX BUG ID 5790
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if(pUnit && pUnit->getTrainLimit(mPlayerID, id, false)==0)
               {
                  gUI.playCantDoSound();
                  return true;
               }
            }
            else if(type==BProtoObjectCommand::cTypeTrainSquad)
            {
//-- FIXING PREFIX BUG ID 5791
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if(pUnit && pUnit->getTrainLimit(mPlayerID, id, true)==0)
               {
                  gUI.playCantDoSound();
                  return true;
               }
            }
            else if(type==BProtoObjectCommand::cTypeBuild)
            {
//-- FIXING PREFIX BUG ID 5792
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if(pUnit && pUnit->getTrainLimit(mPlayerID, id, false)==0)
               {
                  gUI.playCantDoSound();
                  return true;
               }
            }
            else if(type==BProtoObjectCommand::cTypeBuildOther)
            {
//-- FIXING PREFIX BUG ID 5793
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if (pUnit)
               {
                  const BUnitActionBuilding* pBuildingAction=(const BUnitActionBuilding*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding);
                  if (pBuildingAction)
                  {
                     long trainCount=0;
                     long trainLimit=-1;
                     pBuildingAction->getTrainCounts(mPlayerID, id, &trainCount, &trainLimit, false, true);
                     //long queueCount=pBuildingAction->getBuildOtherCount(mPlayerID, id);
                     //if (queueCount > 1)
                     //   trainCount+=(queueCount-1);
                     if (trainLimit != -1 && trainCount >= trainLimit)
                     {
                        gUI.playCantDoSound();
                        return true;
                     }
                  }

                  if (!gConfig.isDefined(cConfigBuildingQueue))
                  {
                     if (!pUnit->isNewBuildingConstructionAllowed(NULL, NULL, pUnit->getProtoObject()->getFlagSecondaryBuildingQueue()))
                     {
                        gUI.playCantDoSound();
                        return true;
                     }
                  }
               }
            }
            else if(type == BProtoObjectCommand::cTypeUnloadUnits)
            {
//-- FIXING PREFIX BUG ID 5794
               const BUnit* pUnit = gWorld->getUnit(mCommandObject);
//--
               if (pUnit && !pUnit->getFlagHasGarrisoned() && !pUnit->getFlagHasAttached())
               {
                  gUI.playCantDoSound();
                  return true;
               }
            }
            else if(type==BProtoObjectCommand::cTypeCustomCommand)
            {
//-- FIXING PREFIX BUG ID 5796
               const BCustomCommand* pCustomCommand = gWorld->getCustomCommand(id);
//--
               if (pCustomCommand)
               {
                  if (pCustomCommand->mLimit != 0 && pCustomCommand->mQueuedCount + pCustomCommand->mFinishedCount >= pCustomCommand->mLimit)
                  {
                     gUI.playCantDoSound();
                     return true;
                  }
                  if (pCustomCommand->mQueuedCount>0 && (!pCustomCommand->mFlagAllowMultiple || !pCustomCommand->mFlagPersistent))
                  {
                     gUI.playCantDoSound();
                     return true;
                  }
               }
            }
            else if(type==BProtoObjectCommand::cTypePower)
            {
               BEntityID squadID=cInvalidObjectID;
//-- FIXING PREFIX BUG ID 5795
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if (pUnit)
               {
//-- FIXING PREFIX BUG ID 5784
                  const BSquad* pSquad=pUnit->getParentSquad();
//--
                  if (pSquad)
                     squadID=pSquad->getID();
               }
               if (!invokePower(id, squadID))
                  gUI.playCantDoSound();
               return true;
            }

            BCost* pCost = mCircleMenu.getCurrentItemCost();
            BPopArray* pPops = mCircleMenu.getCurrentItemPops();
            if(!gWorld->getPlayer(mPlayerID)->checkCost(pCost) || !gWorld->getPlayer(mPlayerID)->checkPops(pPops))
            {
               BSimHelper::playSPCCostErrorSound(mPlayerID, pCost, pPops);
               return (true);
            }

            gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCommandClickmenuN, mPlayerID, mCommandObject, focusIndex);          


            gUI.playClickSound();

            if(type==BProtoObjectCommand::cTypeBuild)
            {
               mBuildProtoID=id;
               mBuildBaseYaw = mCameraYaw;
               //setHoverVisual(id);
               changeMode(cUserModeBuildLocation);
               doScrollDelay();
            }
            else if (type == BProtoObjectCommand::cTypeRallyPoint)
            {
               setHoverVisual(getPlayer()->getCiv()->getLocalRallyPointObjectID());
               changeMode(cUserModeBuildingRallyPoint);
               doScrollDelay();
            }
            else if ((type == BProtoObjectCommand::cTypeKill) && gWorld->getUnit(mCommandObject) && gWorld->getUnit(mCommandObject)->getFlagBuilt())
            {
               // show the yorn box to verify.
               BUIGlobals* puiGlobals = gGame.getUIGlobals();
               if (puiGlobals)
               {
                  changeMode(cUserModeNormal);
                  doScrollDelay();

                  BASSERT ((id & 0xFFFF0000)== 0);  //make sure this fits in 16 bits
                  DWORD value = (cUserYornBoxSelfDestructBuilding << 16) | id;
                  puiGlobals->showYornBoxSmall(this, gDatabase.getLocStringFromID(24052), BUIGlobals::cDialogButtonsOKCancel, value);
               }
               return true;
            }
            else if ((type == BProtoObjectCommand::cTypeDestroyBase) && gWorld->getUnit(mCommandObject) && gWorld->getUnit(mCommandObject)->getFlagBuilt())
            {
               // show the yorn box to verify.
               BUIGlobals* puiGlobals = gGame.getUIGlobals();
               if (puiGlobals)
               {
                  changeMode(cUserModeNormal);
                  doScrollDelay();

                  BASSERT ((id & 0xFFFF0000)== 0);  //make sure this fits in 16 bits
                  DWORD value = (cUserYornBoxSelfDestructBuilding << 16) | id;
                  puiGlobals->showYornBoxSmall(this, gDatabase.getLocStringFromID(24051), BUIGlobals::cDialogButtonsOKCancel, value);
               }
               return true;
            }
            else
            {
               BBuildingCommand* pCommand=(BBuildingCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandBuilding);
               if(pCommand)
               {
                  pCommand->setSenderType(BCommand::cPlayer);
                  pCommand->setSenders(1, &mPlayerID);
                  pCommand->setRecipientType(BCommand::cUnit);
                  pCommand->setRecipients(1, &mCommandObject);
                  pCommand->setType(type);
                  pCommand->setTargetID(id);
                  pCommand->setCount(1);
                  gWorld->getCommandManager()->addCommandToExecute(pCommand);

//-- FIXING PREFIX BUG ID 5799
                  const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
                  if (pUnit && pUnit->getProtoObject()->getCommandAutoCloseMenu(type, id))
                  {
                     changeMode(cUserModeNormal);
                     doScrollDelay();
                     return true;
                  }

                  if(type==BProtoObjectCommand::cTypeCustomCommand)
                  {
//-- FIXING PREFIX BUG ID 5798
                     const BCustomCommand* pCustomCommand = gWorld->getCustomCommand(id);
//--
                     if (pCustomCommand && pCustomCommand->mFlagCloseMenu)
                     {
                        changeMode(cUserModeNormal);
                        doScrollDelay();
                        return true;
                     }
                  }
               }
            }
         }
         return true;
      }
      // Do work
      else if( pInputInterface->isFunctionControl( BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
      {
         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return true;
         }

         long currentItem = mCircleMenu.getCurrentItemIndex();
         if(currentItem != -1)
         {
            gUI.playClickSound();

            BProtoObjectCommand command;
            command.set(mCircleMenu.getCurrentItemID());
            long type=command.getType();
            long id=command.getID();

            bool sendCommand=false;
            if(type==BProtoObjectCommand::cTypeTrainUnit || type==BProtoObjectCommand::cTypeTrainSquad)
            {
//-- FIXING PREFIX BUG ID 5802
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if(pUnit && pUnit->getTrainCount(mPlayerID, id)>0)
                  sendCommand=true;
            }
            else if(type==BProtoObjectCommand::cTypeResearch)
            {
               if(gWorld->getPlayer(mPlayerID)->getTechTree()->getTechStatus(id, mCommandObject.asLong())==BTechTree::cStatusResearching)
                  sendCommand=true;
            }
            else if(type==BProtoObjectCommand::cTypeCustomCommand)
            {
//-- FIXING PREFIX BUG ID 5803
               const BCustomCommand* pCustomCommand = gWorld->getCustomCommand(id);
//--
               if (pCustomCommand && pCustomCommand->mQueuedCount > 0 && pCustomCommand->mFlagAllowCancel)
                  sendCommand=true;
            }
            else if(type==BProtoObjectCommand::cTypeBuildOther)
            {
//-- FIXING PREFIX BUG ID 5804
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if (pUnit)
               {
                  const BUnitActionBuilding* pBuildingAction=(const BUnitActionBuilding*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding);
                  if (pBuildingAction)
                  {
                     long queueCount=pBuildingAction->getBuildOtherCount(mPlayerID, id);
                     if (queueCount > 0)
                        sendCommand=true;
                  }
               }
            }
            else if (type == BProtoObjectCommand::cTypeRallyPoint)
            {
//-- FIXING PREFIX BUG ID 5805
               const BUnit* pUnit=gWorld->getUnit(mCommandObject);
//--
               if (pUnit && pUnit->haveRallyPoint(mPlayerID))
               {
                  sendCommand=true;
                  type = BProtoObjectCommand::cTypeClearRallyPoint;
                  resetUserMode();
                  doScrollDelay();
               }
            }
            if(sendCommand)
            {
               BBuildingCommand* pCommand=(BBuildingCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandBuilding);
               if(pCommand)
               {
                  pCommand->setSenderType(BCommand::cPlayer);
                  pCommand->setSenders(1, &mPlayerID);
                  pCommand->setRecipientType(BCommand::cUnit);
                  pCommand->setRecipients(1, &mCommandObject);
                  pCommand->setType(type);
                  pCommand->setTargetID(id);
                  pCommand->setCount(-1);
                  gWorld->getCommandManager()->addCommandToExecute(pCommand);
                  return true;
               }
            }

            gUI.playCantDoSound();
         }
         return true;
      }
      else if(pInputInterface->isFunctionControl( BInputInterface::cInputDisplayExtraInfo, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
      {         
         //Disabled as per PHX-14572 -- Leave here in case we want it back
         //setHUDItemEnabled(BUser::cHUDItemCircleMenuExtraInfo, !getHUDItemEnabled(BUser::cHUDItemCircleMenuExtraInfo));
         //gUI.playClickSound();
         return true;
      }      
      // Clear
      else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
      {
         gUI.playClickSound();
         changeMode(cUserModeNormal);

         gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuCloseCommand, mPlayerID); 

         return true;
      }
      // Powers
      else if (pInputInterface->isFunctionControl(BInputInterface::cInputPowers, controlType, modifyAction, detail.mTime, mControllerKeyTimes, start, repeat, stop, false, -1.0f, -1.0f, &detail ))
      {
         if (!getPlayer()->getCiv()->getPowerFromHero())
         {
            changeMode(cUserModeNormal);
            showPowerMenu();
            return (true);
         }
      }
   }

   // Handle goto actions
   if (handleInputGoto(port, event, controlType, detail))
      return (true);

   return false;
}

//==============================================================================
// BUser::handleInputPowerMenu
//==============================================================================
bool BUser::handleInputPowerMenu(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (controlType == cStickLeft)
   {
      mScrollXDelay = detail.mX;
      mScrollYDelay = detail.mY;
   }

   BPlayer* pPlayer = getPlayer();
   BDEBUG_ASSERT(pPlayer!=NULL);
   bool bSuccessFlash = false;
   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      bSuccessFlash = mpUIContext->handleCircleMenuInput(port, (BInputEventType) event, (BInputControlType) controlType, detail);
   }

   bool success2D = mCircleMenu.handleInput(port, event, controlType, detail);   
   if (success2D)// || success3D)
      return true;

   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);
   int focusIndex = mCircleMenu.getCurrentItemOrder();
   if (start || repeat)
   {      
      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerHasFocusN, mPlayerID, cInvalidObjectID, focusIndex);          
   }
   if(stop)
   {
      gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerHasFocusN, mPlayerID, cInvalidObjectID, -1);          
   }

   if(event==cInputEventControlStart)
   {
      BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
      bool             modifyAction    = getFlagModifierAction();

      // Selection
      if( pInputInterface->isFunctionControl( BInputInterface::cInputSelection, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
      {
         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return true;
         }

         long currentItemIndex = mCircleMenu.getCurrentItemIndex();
         if(currentItemIndex != -1)
         {
            long currentItemID=mCircleMenu.getCurrentItemID();
            if (mSubMode==cSubModeSelectPower)
            {
               const BLeaderSupportPower* pSupportPower=pPlayer->getSupportPower(mSupportPowerIndex);
               if (pSupportPower)
               {
                  int protoPowerID = pSupportPower->mPowers[currentItemID];
                  BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
                  BCost* pCost = pProtoPower->getCost();
                  const BPopArray* pPops = pProtoPower->getPop();
                  bool passCost = pPlayer->checkCost(pCost);
                  bool passPop = pPlayer->checkPops(pPops);
                  if (pProtoPower && passCost && passPop)
                  {
                     gUI.playClickSound();

                     gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerClickmenuN, mPlayerID, cInvalidObjectID, focusIndex);          


                     BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
                     if(pCommand)
                     {
                        pCommand->setSenderType(BCommand::cPlayer);
                        pCommand->setSenders(1, &mPlayerID);
                        pCommand->setRecipientType(BCommand::cGame);
                        pCommand->setType(BGameCommand::cTypeSelectPower);
                        BVector data;
                        data.x=(float)mSupportPowerIndex;         // power index
                        data.y=(float)currentItemID;              // sub power index
                        data.z=(float)mSupportPowerIconLocation;  // icon location
                        pCommand->setPosition(data);
                        gWorld->getCommandManager()->addCommandToExecute(pCommand);
                     }
                     changeMode(cUserModeNormal);
                     doScrollDelay();
                  }
                  else if (!passCost || !passPop)
                  {                     
                     BSimHelper::playSPCCostErrorSound(mPlayerID, pCost, (BPopArray*)pPops);
                  }
               }
               else
                  gUI.playCantDoSound();
            }
            else if(currentItemID==gDatabase.getPPIDRallyPoint())
            {
               // Rally Point
               long id=getPlayer()->getCiv()->getRallyPointObjectID();
               if(id!=-1)
                  setHoverVisual(id);
               changeMode(cUserModeRallyPoint);
               doScrollDelay();
            }
            else if(currentItemID==gDatabase.getPPIDRepair())
            {
               // Repair
               if (mCircleMenu.getItemUnavailable(mCircleMenu.getItemIndex(currentItemID)))
               {
                  BPlayer* pPlayer = getPlayer();
                  BCost* pCost = mCircleMenu.getCurrentItemCost();
                  BPopArray* pPops = mCircleMenu.getCurrentItemPops();
                  if (pPlayer && (!pPlayer->checkCost(pCost) || !pPlayer->checkPops(pPops)))
                  {
                     BSimHelper::playSPCCostErrorSound(mPlayerID, pCost, pPops);
                  }
                  else
                  {
                     gUI.playCantDoSound();
                  }
               }
               else
               {
                  doRepair();
                  gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerClickmenuN, mPlayerID, cInvalidObjectID, focusIndex);          
               }
            }
            else if(currentItemID<0)
            {
               // Select Support Power
               uint supportPowerIndex=(-currentItemID)-1;
               if (supportPowerIndex < pPlayer->getLeader()->getSupportPowers().getSize())
               {
                  showSelectPowerMenu(supportPowerIndex, mCircleMenu.getCurrentItemOrder());
                  gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerClickmenuN, mPlayerID, cInvalidObjectID, focusIndex);          
               }
               else
                  gUI.playCantDoSound();
            }
            else
            {
               // Power Command
               long currentItemIndex = mCircleMenu.getCurrentItemIndex();
               if(currentItemIndex != -1)
               {
                  BCost* pCost = mCircleMenu.getCurrentItemCost();
                  if(mCircleMenu.getItemUnavailable(currentItemIndex))
                  {                                          
                     BPopArray* pPops = mCircleMenu.getCurrentItemPops();
                     if (!pPlayer->checkCost(pCost) || !pPlayer->checkPops(pPops))
                     {
                        BSimHelper::playSPCCostErrorSound(mPlayerID, pCost, pPops);
                     }
                     else
                     {
                        gUI.playCantDoSound();
                     }

                     return (true);
                  }  
                  else if (!pPlayer->checkCost(pCost))
                  {
                     BSimHelper::playSPCCostErrorSound(mPlayerID, pCost, NULL);
                     return (true);
                  }

                  if (invokePower(mCircleMenu.getCurrentItemID(), cInvalidObjectID))
                  {
                     gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuPowerClickmenuN, mPlayerID, cInvalidObjectID, focusIndex);          

                     clearAllSelections();
                     doScrollDelay();
                  }
                  else
                  {
                     gUI.playCantDoSound();
                  }

               }
            }
         }
         return true;
      }
      // Do work
      else if( pInputInterface->isFunctionControl( BInputInterface::cInputDoWork, controlType, false, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
      {
         if(mPlayerState!=BPlayer::cPlayerStatePlaying)
         {
            gUI.playCantDoSound();
            return true;
         }

         long currentItemIndex = mCircleMenu.getCurrentItemIndex();
         if(currentItemIndex != -1)
         {
            if(mCircleMenu.getCurrentItemID()==gDatabase.getPPIDRallyPoint() && pPlayer->haveRallyPoint())
            {
               // Clear Rally Point
               gUI.playClickSound();
               BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
               if(pCommand)
               {
                  pCommand->setSenderType(BCommand::cPlayer);
                  pCommand->setSenders(1, &mPlayerID);
                  pCommand->setRecipientType(BCommand::cGame);
                  pCommand->setType(BGameCommand::cTypeClearGlobalRallyPoint);
                  pCommand->setPosition(mHoverPoint);
                  gWorld->getCommandManager()->addCommandToExecute(pCommand);
                  resetUserMode();
                  doScrollDelay();
               }
            }
         }
         return true;
      }
      // Clear
      else if( pInputInterface->isFunctionControl( BInputInterface::cInputClear, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
      {
         if (mSubMode==cSubModeSelectPower)
            showPowerMenu();
         else
         {
            gUI.playClickSound();
            changeMode(cUserModeNormal);
         }
         gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuClosePower, mPlayerID); 
         
         return true;
      }
      // Powers
      else if( pInputInterface->isFunctionControl( BInputInterface::cInputPowers, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail ) )
      {
         //gUI.playClickSound();
         //changeMode(cUserModeNormal);
         return true;
      }
      else if(pInputInterface->isFunctionControl( BInputInterface::cInputDisplayExtraInfo, controlType, modifyAction, 0, NULL, false, false, false, false, -1.0f, -1.0f, &detail))
      {         
         //Disabled as per PHX-14572 -- Leave here in case we want it back
         //setHUDItemEnabled(BUser::cHUDItemCircleMenuExtraInfo, !getHUDItemEnabled(BUser::cHUDItemCircleMenuExtraInfo));
         //gUI.playClickSound();         
         return true;
      }      
   }

   // Handle goto actions
   if (handleInputGoto(port, event, controlType, detail))
      return (true);

   return false;
}

//==============================================================================
// BUser::processAutoDifficulty
//==============================================================================
void BUser::processAutoDifficulty()
{
   // If auto difficulty is selected, see if this game qualifies for changing the 
   // difficulty setting in the user's profile for the next game.
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if (!pSettings)
      return;
   long gameType = -1;
   if (!pSettings->getLong(BGameSettings::cGameType, gameType))
      return;
   if (gameType != BGameSettings::cGameTypeSkirmish)
      return;
   long player1DifficultyType = -1;
   if (!pSettings->getLong(BGameSettings::cPlayer1DifficultyType, player1DifficultyType))
      return;
   if (player1DifficultyType != DifficultyType::cAutomatic)
      return;

   BPlayer* pMyPlayer = this->getPlayer();
   BASSERT(pMyPlayer);

   /*
   BUserProfile* pProfile = NULL;
   if ( isSignedIn() )
      pProfile = getProfile();
   if (!pProfile)
      return;
   */
   if (!pMyPlayer->getTrackingData())
      return;


   int autoDiffSetting = pMyPlayer->getTrackingData()->getAutoDifficultyLevel();
   int numGamesPlayed = pMyPlayer->getTrackingData()->getAutoDifficultyNumGamesPlayed();
   DWORD endTime = gWorld->getGametime();
   if (endTime <= 0)
      return;

   BTeamID usersTeam = pMyPlayer->getTeamID();

   bool iWon = pMyPlayer->getPlayerState() == BPlayer::cPlayerStateWon;

   long numPlayers = gWorld->getNumberPlayers();
   long numOnTeam1 = 0;
   long humansOnTeam1 = 0;
   long numOnTeam2 = 0;
   long humansOnTeam2 = 0;
   bool gameQualifies = true;
   float team1supplies = 0.0f;
   float team2supplies = 0.0f;

   for (long player = 1; player < numPlayers; player++)
   {
//-- FIXING PREFIX BUG ID 5808
      const BPlayer* pPlayer = gWorld->getPlayer(player);
//--
      BASSERT(pPlayer);
      BTeamID team = pPlayer->getTeamID();
      switch(pPlayer->getPlayerType())
      {
      case BPlayer::cPlayerTypeNPC:
         {
            break;
         }
      case BPlayer::cPlayerTypeComputerAI:
         {
            if (team == 1)
            {
               numOnTeam1++;
               team1supplies += pPlayer->getTotalResource(0);
            }
            else if (team == 2)
            {
               numOnTeam2++;
               team2supplies += pPlayer->getTotalResource(0);
            }

            break;
         }
      case BPlayer::cPlayerTypeHuman:
         {            
            if (team == 1)
            {
               numOnTeam1++;
               humansOnTeam1++;
               team1supplies += pPlayer->getTotalResource(0);
            }
            else if (team == 2)
            {
               numOnTeam2++;
               humansOnTeam2++;
               team2supplies += pPlayer->getTotalResource(0);
            }
            break;
         }
      }
   }

   // Check eligibility
   if (numOnTeam1 != numOnTeam2)
      gameQualifies = false;

   if ( (humansOnTeam1 > 0) && (humansOnTeam2 > 0) )   //  Must be humans on just one team
      gameQualifies = false;

   if ( (humansOnTeam1 + humansOnTeam2) != 1 )  // 11/6/08:  Reject all multiplayer games.
      gameQualifies = false;

   // Game type already checked before call

   if (!gameQualifies)
      return;

   if (team1supplies <= 0.0f)
      team1supplies = 1.0f;

   if (team2supplies <= 0.0f)
      team2supplies = 1.0f;

   float suppliesRatio = team1supplies / team2supplies;
   if (usersTeam == 2)
      suppliesRatio = team2supplies / team1supplies;   // 1.00 = a tie, 

   if (iWon && (suppliesRatio < 1.01))
      suppliesRatio = 1.01f;
   if (!iWon && (suppliesRatio > 0.99))
      suppliesRatio = 0.99f;

   // Now we need to combine suppliesRatio (how big was the win) with game time, to get a unified measure.  
   // For example, a win with suppliesRatio 1.0 in 10 minutes is bigger than a win with supplies ratio 2.0 at 40 minutes.
   // Consider that in the example above, the winning team could have kept racking up resources, going to 2.0 by 20 minutes, etc.

   float gameLength =  endTime / 1200000.0f;   // Fraction of normal 20 minute game, big number = slow game.
   if (gameLength < 0.3f)
      gameLength = 0.3f;
   

   float performanceRatio = 1.0f + ( (suppliesRatio - 1.0f) / gameLength);  
   int adjustment = 0;

   if (performanceRatio > 1.55)
      adjustment = 30;   // Big win, get tougher
   else if (performanceRatio > 1.30)
      adjustment = 20;
   else if (performanceRatio > 1.15)
      adjustment = 10;
   else if (performanceRatio < 0.55)
      adjustment = -30;  // Big loss, get easier
   else if (performanceRatio < 0.80)
      adjustment = -20;
   else if (performanceRatio < 0.95)
      adjustment = -10;

   float amplitude = 1.0f;   // Scale of adjustment, 1.0 if first game (up to 30 diff points), .15 at maturity (max 6 diff points)


   if (numGamesPlayed <= 0)
      amplitude = 1.0f;
   else if (numGamesPlayed == 1)
      amplitude = 0.7f;
   else if (numGamesPlayed < 10)
      amplitude = 0.76f - (numGamesPlayed * 0.06f); // 2 -> .64, 9 -> .22
   else
      amplitude = 0.17f;

   adjustment = (int)(amplitude * adjustment);
   if (iWon && (adjustment < 1))
      adjustment = 1;   // Always increment after a win.

   // Make sure adjustment is less than half the distance to zero or 200.
   const int autoDiffMin = 0;
   const int autoDiffMax = 200;
   int minAdjustment = (autoDiffMin - autoDiffSetting) / 2;
   int maxAdjustment = (autoDiffMax - autoDiffSetting) / 2;
   if (adjustment < minAdjustment)
      adjustment = minAdjustment;
   if (adjustment > maxAdjustment)
      adjustment = maxAdjustment;

   int newDiff = autoDiffSetting + adjustment;
   if (newDiff > autoDiffMax)
      newDiff = autoDiffMax;
   if (newDiff < autoDiffMin)
      newDiff = autoDiffMin;

   pMyPlayer->getTrackingData()->setAutoDifficultyLevel(newDiff);
   if (numGamesPlayed < 100)
      pMyPlayer->getTrackingData()->setAutoDifficultyNumGamesPlayed(numGamesPlayed + 1);
   else
      pMyPlayer->getTrackingData()->setAutoDifficultyNumGamesPlayed(numGamesPlayed);

   //10/30/2008 Removing call to writeProfile as this calls XUserWriteProfileSettings which 
   //we are only allowed to call once per session or every 5 minutes per TCR #136
   //This function was being called here at the end of every skirmish. Now
   //only BPlayer::processGameEnd will call writeProfile at the end of a skirmish.
   //gUserProfileManager.writeProfile(this);

}

//==============================================================================
// BUser::processGameOver
//==============================================================================
void BUser::processGameOver()
{
   // Reset rumble whenever game is over to prevent rumble staying on forever.
   gInputSystem.getGamepad(gUserManager.getPrimaryUser()->getPort()).resetRumble();

//-- FIXING PREFIX BUG ID 5809
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   BASSERT(pSettings);

   long gameType = -1;
   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   if (!result)
      return;

   // did we just complete a campaign scenario
   if (gameType != BGameSettings::cGameTypeSkirmish)
   {
      if (!gConfig.isDefined(cConfigShowCampaignPostGame))
      {
         gUIManager->showNonGameUI(BUIManager::cEndGameScreen, this);
      }
      else
      {
         gUIManager->showNonGameUI(BUIManager::cPostGameStatsScreen, this);
      }
      changeMode(cUserModeNormal);
      return;
   }

   if (gameType == BGameSettings::cGameTypeSkirmish)
   {
      // Not campaign. If skirmish, check for automatic difficulty updates
      //processAutoDifficulty();

      // Reveal the map (only visually)
      gWorld->setFogOfWar(false, false, true);
   }

   clearAllSelections();

   /*
   if( gConfig.isDefined( cConfigDemo ) )
   {
      gUIManager->getWidgetUI()->showButtonHelp( false );
   }
   */

}


//==============================================================================
// BUser::exitGameMode
//
// - this is a contextual exit depending on how the game was started.
// - right now everything exits to the main menu exit if you are not playing
//    the campaign. If in the campaign, then you exit to the campaign mode.
//
//    BTW. If you have suggestions on a better way to do this I'm all ears. :)
//
//    The method is 0-quit, 1-restart, 2-continue
//==============================================================================
void BUser::exitGameMode(BExitMethod method)
{
//-- FIXING PREFIX BUG ID 5811
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   BASSERT(pSettings);

   long gameType = -1;

   bool result = pSettings->getLong(BGameSettings::cGameType, gameType);
   if (!result || gConfig.isDefined(cConfigDemo))
   {
      gModeManager.setMode( BModeManager::cModeMenu );
      return;
   }

   if (gameType == BGameSettings::cGameTypeScenario )
   {
      switch( method )
      {
         case cExitMethodGoToAdvancedTutorial:
         {
            long startContext = BGameSettings::cGameStartContextNone;
            pSettings->getLong( BGameSettings::cGameStartContext, startContext );

            switch( startContext )
            {
               case BGameSettings::cGameStartContextBasicTutorialFromMainMenu:
                  startContext = BGameSettings::cGameStartContextAdvancedTutorialFromMainMenu;
                  break;
               case BGameSettings::cGameStartContextBasicTutorialFromSPCMenu:
                  startContext = BGameSettings::cGameStartContextAdvancedTutorialFromSPCMenu;
                  break;

               default:
                  BASSERTM( false, "UNEXPECTED START CONTEXT WHEN GOING TO ADVANCED TUTORIAL!" );
                  break;
            }

            gModeManager.getModeMenu()->playTutorial( BModeMenu::cTutorialAdvanced, startContext );
            gModeManager.setMode( BModeManager::cModeMenu );
         }
         break;

         case cExitMethodQuit:
         {
            long startContext = BGameSettings::cGameStartContextNone;
            pSettings->getLong( BGameSettings::cGameStartContext, startContext );
          
            switch( startContext )
            {
               case BGameSettings::cGameStartContextBasicTutorialFromSPCMenu:
               case BGameSettings::cGameStartContextBasicTutorialFromNewSPC:
               case BGameSettings::cGameStartContextAdvancedTutorialFromSPCMenu:
                  gModeManager.setMode( BModeManager::cModeCampaign2 );
               break;

               default:
                  gModeManager.setMode( BModeManager::cModeMenu );
               break;
            }
         }
         break;

         case cExitMethodContinue:
         {
            BModeCampaign2* pCampaignMode = (BModeCampaign2*)gModeManager.getMode( BModeManager::cModeCampaign2 );
            BASSERT( pCampaignMode );
            if ( pCampaignMode )
            {
               BCampaign* pCampaign = gCampaignManager.getCampaign( 0 );
               BASSERT( pCampaign );
               pCampaign->setCurrentNodeID(0);
               pCampaign->setPlayContinuous( true );
               pCampaignMode->setNextState( BModeCampaign2::cStateStartSPCampaign );
               gModeManager.setMode( BModeManager::cModeCampaign2 );
               return;
            }
         }
         // No break here so that if there's no campaign mode (as if), we just go to the menu.

         default:
            gModeManager.setMode( BModeManager::cModeMenu );
         break;
      }

      return;
   }

   if (gameType == BGameSettings::cGameTypeCampaign)
   {  
      BCampaign * pCampaign = gCampaignManager.getCampaign(0);
      BASSERT(pCampaign);

      // Save the mode we just played in.
      long networkType=0;
      if (pSettings->getLong(BGameSettings::cNetworkType, networkType) )
      {
         uint8 lastPlayedMode=BUserProfile::eLastCampaignModeLocal;
         switch (networkType)
         {

         case BGameSettings::cNetworkTypeLocal:
            lastPlayedMode = BUserProfile::eLastCampaignModeLocal;
            break;
         case BGameSettings::cNetworkTypeLan:
            lastPlayedMode = BUserProfile::eLastCampaignModeLan;
            break;
         case BGameSettings::cNetworkTypeLive:
            lastPlayedMode = BUserProfile::eLastCampaignModeLive;
            break;
         }
         pCampaign->setLastPlayedMode((uint8) lastPlayedMode);
      }

      if (mPlayerState == BPlayer::cPlayerStateWon)
         pCampaign->incrementSessionID();

      if (method==cExitMethodQuit || method==cExitMethodGoToAdvancedTutorial)
      {
         // We want to stop if we are quitting the campaign.
         pCampaign->setPlayContinuous(false);
      }

      if ((method==cExitMethodQuit) || (method == cExitMethodContinue))
      {
         // Save the campaign progress
         if (mPlayerState == BPlayer::cPlayerStateWon)
         {
            // if we've won, we need to go to the next node
            // SRL 11/6/08, don't increment here. This is done earlier right at game end.
            // pCampaign->incrementCurrentNode();

            BCampaignNode* pNode = pCampaign->getNode(pCampaign->getCurrentNodeID());
            if (pNode && pNode->getFlag(BCampaignNode::cEndOfCampaign))
            {
               // ajl 10/8/08 - A session ID of UINT16_MAX means the player completed the last mission of the campaign and
               // hasn't played another mission since. This is used to let the UI know not to show the Continue option until
               // the player plays another mission.
               pCampaign->setSessionID(UINT16_MAX);
            }
         }
      }
      else
      {
         // method == cExitMethodReplay
      }

      // pCampaign->setLastMissionType();
      pCampaign->saveToCurrentProfile();
      // exit to the campaign or the party
      if (gConfig.isDefined(cConfigRejoinParty))
      {
         BCampaignNode* pNode = pCampaign->getNode(pCampaign->getCurrentNodeID());
         if (gLiveSystem->isPartySessionActive())
         {
            if (mPlayerState == BPlayer::cPlayerStateWon)
            {
               if (pNode && pNode->getFlag(BCampaignNode::cEndOfCampaign))
               {
                  // ajl 10/8/08 - A session ID of UINT16_MAX means the player completed the last mission of the campaign and
                  // hasn't played another mission since. This is used to let the UI know not to show the Continue option until
                  // the player plays another mission.
                  pCampaign->setPlayContinuous(true); // this makes sure the cinematics play
                  pCampaign->setSessionID(UINT16_MAX);
                  pCampaign->saveToCurrentProfile();

                  gLiveSystem->setPartySystemForReenter(true);
                  gLiveSystem->getMPSession()->belayNextGameDisconnectEvent();


                  gModeManager.setMode( BModeManager::cModeCampaign2 );
                  return;
               }
            }

            gLiveSystem->setPartySystemForReenter(true);
            gModeManager.setMode( BModeManager::cModePartyRoom2 ); 
            return;
         }
         else
         {
            gLiveSystem->setPartySystemForReenter(false);
            gLiveSystem->disposeMPSession();
            gLiveSystem->disposeVoice();
         }
      }

      BCampaignNode* pNode = pCampaign->getNode(pCampaign->getCurrentNodeID());
      if (!pCampaign->getPlayContinuous() || (pNode && pNode->getFlag(BCampaignNode::cEndOfCampaign)))
         gModeManager.setMode( BModeManager::cModeCampaign2 );
      else
      {
         gLiveSystem->setPresenceContext(PROPERTY_GAMETIMEMINUTES, 0, true);
         gLiveSystem->setPresenceContext(PROPERTY_GAMESCORE, 0, true);
         gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_CAMPAIGNSOLOPLAY);

         pCampaign->setGameSettings(true, false);

         BModeGame* pModeGame = gModeManager.getModeGame();
         pModeGame->leave(pModeGame);
         pModeGame->enter(pModeGame);
      }

      return;
   }

   //After a MP game - you go back to the party screen if the party is still valid - eric
   if (gConfig.isDefined(cConfigRejoinParty))
   {
      if (gLiveSystem->isPartySessionActive())
      {
         gLiveSystem->setPartySystemForReenter(true);
         gModeManager.setMode( BModeManager::cModePartyRoom2 ); 
         return;
      }
      else
      {
         gLiveSystem->setPartySystemForReenter(false);
         gLiveSystem->disposeMPSession();
         gLiveSystem->disposeVoice();
      }
   }


   // default return to place for now.
   gModeManager.setMode( BModeManager::cModeMenu );
   return;

}

//=============================================================================
// BUser::changeMode
//=============================================================================
void BUser::changeMode(long newMode)
{
   setFlagExitModeOnModifierRelease(false);

   long oldMode = mUserMode;
   if (oldMode == newMode)
   {
      autoExitSubMode();
      return;
   }

   if (newMode == cUserModeCinematic)
   {
      gUIManager->hideInGameUI();
   }
   else if (oldMode == cUserModeCinematic && !gUIManager->isNonGameUIVisible() )
   {
      gUIManager->restoreInGameUI();
   }

   // Setup minigame stuff before we do other camera checks
   if (newMode == cUserModeInputUILocationMinigame)
   {
      if (mFlagUIPowerMinigame)
      {
//-- FIXING PREFIX BUG ID 5815
         const BProtoPower* pPP = gDatabase.getProtoPowerByID(mUIProtoPowerID);
//--
         if (pPP)
            mMinigame.start(this, pPP->getMinigameType(), mMinigameTimeFactor, !mFlagUIRestartMinigame);
      }
      else
         mMinigame.reset();
   }

   if (oldMode == cUserModeInputUILocationMinigame)
   {
      mMinigame.stop();
   }

   if(oldMode == cUserModePower)
   {
      // MS 5/2/2008: do any user-level processing here if we're exiting the power user mode
      mScrollTime = 0;
      mScrollX = 0.0f;
      mScrollY = 0.0f;

      cancelPower();
   }


   // Update the camera effect for transitioning between modes
   updateModeCameraEffect(oldMode, newMode);

   if (isScrollingMode(oldMode) != isScrollingMode(newMode))
   {
      mScrollTime = 0;
      mScrollX = 0.0f;
      mScrollY = 0.0f;
   }
   
   if ((oldMode==cUserModeNormal || oldMode==cUserModeInputUISquad || oldMode==cUserModeInputUIUnit) && newMode!=cUserModeCircleSelecting)
      resetStickyReticle();

   if (oldMode == cUserModeRallyPoint)
   {
      getPlayer()->setRallyPointVisible(true);
      // Make building specific rally points visible
      BEntityHandle handle=cInvalidObjectID;
      for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
      {
         if(pUnit->getPlayerID()==getPlayerID() && pUnit->haveRallyPoint(getPlayerID()))
            pUnit->setRallyPointVisible(true, getPlayerID());
      }
   }
   else if (newMode == cUserModeRallyPoint)
   {
      getPlayer()->setRallyPointVisible(false);
      // Make building specific rally points invisible
      BEntityHandle handle=cInvalidObjectID;
      for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
      {
         if(pUnit->getPlayerID()==getPlayerID() && pUnit->haveRallyPoint(getPlayerID()))
            pUnit->setRallyPointVisible(false, getPlayerID());
      }
   }

   if (oldMode == cUserModeBuildingRallyPoint)
   {
      // Make that building's rally point visible
      BUnit* pUnit = gWorld->getUnit(mCommandObject);
      if (pUnit)
         pUnit->setRallyPointVisible(true, getPlayerID());
   }
   else if (newMode == cUserModeBuildingRallyPoint)
   {
      // Make that building's rally point invisible
      BUnit* pUnit = gWorld->getUnit(mCommandObject);
      if (pUnit)
         pUnit->setRallyPointVisible(false, getPlayerID());
   }

   if (oldMode == cUserModeCommandMenu && mFlagDeselectOnMenuClose)
      mSelectionManager->clearSelections();

   if(oldMode==cUserModeNormal)
      clearCameraState();
   else if(oldMode==cUserModeBuildLocation)
   {
      mPlacementSuggestion = XMVectorReplicate(-1.0f);
      mPlacementSocketID = cInvalidObjectID;
   }

   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      //-- update flash
      if (oldMode == cUserModeCommandMenu || oldMode == cUserModePowerMenu)
      {
         mpUIContext->hideCircleMenu();
      }
   }

   if (oldMode == cUserModeInputUISquadList)
   {
      BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mPowerDecal);
      if (pDecalAttributes)
         pDecalAttributes->setEnabled(false);
   }

   // AJL 2/26/08 - This clear is causing problems so taking it out.
   // Clear controller key times on user mode change
   //memset( mControllerKeyTimes, 0, sizeof( BControllerKeyTimes ) * cNumDC );

   // If follow mode or input ui squad list clear selections
   if ((newMode == cUserModeFollow) || (newMode == cUserModeInputUISquadList) || (newMode == cUserModeInputUIPlaceSquadList))
   {
      clearAllSelections();

      if (newMode == cUserModeInputUISquadList)
      {
         BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(mUIProtoPowerID);
         BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mPowerDecal);
         if (pProtoPower && pDecalAttributes)
         {
            BManagedTextureHandle textureBase;
            if (pProtoPower->getDataTexture(0, "Base", textureBase))
            {
               pDecalAttributes->setTextureHandle(textureBase);
               pDecalAttributes->setEnabled(true);
               pDecalAttributes->setForward(BVec3(cXAxisVector.x, cXAxisVector.y, cXAxisVector.z));
               pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
               pDecalAttributes->setIntensity(1.0f);
               pDecalAttributes->setRenderOneFrame(false);
               pDecalAttributes->setColor(packRGBA(cColorGreen, 1.0f));
               pDecalAttributes->setSizeX(mUIPowerRadius);
               pDecalAttributes->setSizeZ(mUIPowerRadius);
               pDecalAttributes->setYOffset(0.0f);
               pDecalAttributes->setFlashMovieIndex(-1);
               pDecalAttributes->setConformToTerrain(true);
            }
         }
      }
   }

   // Restore the unit panel
   if (mFlagRestoreUnitPanelOnModeChange)
   {
      mFlagRestoreUnitPanelOnModeChange=false;
      mFlagOverrideUnitPanelDisplay=false;
      updateSelectedUnitIcons();
   }

   // Set the mode.
   mUserMode = newMode;   
   mLastUserMode = oldMode;

   autoExitSubMode();

   // Input context switching
   gInputSystem.leaveContext(getUserModeName(oldMode));
   gInputSystem.enterContext(getUserModeName(newMode));
}

//=============================================================================
//=============================================================================
void BUser::setDeviceRemoved(bool isRemoved)
{
   mDeviceRemoved = isRemoved;
}

//=============================================================================
// 
//=============================================================================
void BUser::setDefaultDevice(XCONTENTDEVICEID deviceID)
{
   setDeviceRemoved(false);               // even if this is XCONTENTDEVICE_ANY
   mDeviceID = deviceID;
}

//=============================================================================
// This is called when a live signin/signout event triggers and we want to make
//  sure that the sign in state is correct.  It is a catchall for the problematic
//  series where the user is signed in, but not connected to Live - eric
//=============================================================================
void BUser::updateSigninStatus()
{
   if (mPort < 0 || mPort > XUSER_MAX_COUNT)
   {
      mSignInState = eXUserSigninState_NotSignedIn;
      mLiveEnabled = false;
      mLiveStateChanged = true;
      return;
   }

   XUSER_SIGNIN_STATE prevState = mSignInState;

   mSignInState = eXUserSigninState_NotSignedIn;
   mLiveEnabled = false;

   //mSignInState = XUserGetSigninState(mPort);

   XUSER_SIGNIN_INFO onlineSigninInfo;
   XUSER_SIGNIN_INFO offlineSigninInfo;

   if (XUserGetSigninInfo(mPort, XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY, &onlineSigninInfo) == ERROR_SUCCESS)
   {
      mLiveEnabled = (onlineSigninInfo.dwInfoFlags & XUSER_INFO_FLAG_LIVE_ENABLED);

      mSignInState = onlineSigninInfo.UserSigninState;
   }

   if (XUserGetSigninInfo(mPort, XUSER_GET_SIGNIN_INFO_OFFLINE_XUID_ONLY, &offlineSigninInfo) == ERROR_SUCCESS)
   {
      mLiveEnabled = (offlineSigninInfo.dwInfoFlags & XUSER_INFO_FLAG_LIVE_ENABLED);

      mSignInState = offlineSigninInfo.UserSigninState;
   }

   mLiveStateChanged = (mLiveStateChanged || (mSignInState != prevState));

   readPrivileges();
}

//=============================================================================
// BUser::setPort
//=============================================================================
void BUser::setPort(long port)
{
   mPort = port;

   mName.empty();
   mXuid = 0;

   if (mPort >=0 && mPort < XUSER_MAX_COUNT)
   {
      updateSigninStatus();
      //mSignInState=XUserGetSigninState(mPort);

      CHAR gamerTag[XUSER_NAME_SIZE];
      if (isSignedIn() && XUserGetName(mPort, gamerTag, XUSER_NAME_SIZE) == ERROR_SUCCESS)
      {
         gamerTag[XUSER_NAME_SIZE - 1] = '\0';
         mName = gamerTag;
      }
      else
      {
         //Generate a fake user name and XUID
         //mName = "Player"; // XXX loc default player name string
         BUString temp;
         temp.locFormat(gDatabase.getLocStringFromID(25567).getPtr(), 1);
         mName.set(temp.getPtr());
         uint64 tempXuid = 0;
         XNetRandom((BYTE*)&tempXuid, sizeof(XUID));
         uint64 fakeBits = 0xFFFFFFFFFFFF0000;
         mXuid = (tempXuid & fakeBits) | 0xBEEF;
      }

      if (isSignedIn())
         gUserProfileManager.readProfile(this);
      else
         setProfile(new BUserProfile());

      updateXuid();

      readPrivileges();

      initializeAchievements();

      //if (mSignInState == eXUserSigninState_SignedInToLive && mPrivMultiplayer)
      //   gLSPManager.updateServiceRecord(mXuid);

      // [10/21/2008 xemu] convert the gamer picture texture into something we can actually use in the UI 
      gGamerPicManager.readGamerPic(mXuid, mPort, TRUE);

      //queryFriends();
   }
}

//=============================================================================
// Will only update the xuid if we're uninitialized or if our sign-in state
//    has changed.
//=============================================================================
void BUser::updateXuid()
{
   if (mPort >=0 && mPort < XUSER_MAX_COUNT)
   {
      XUSER_SIGNIN_INFO onlineSigninInfo;
      XUSER_SIGNIN_INFO offlineSigninInfo;

      if (mXuid == 0 ||
          (XUserGetSigninInfo(mPort, 0x00000001, &onlineSigninInfo) == ERROR_SUCCESS &&
           XUserGetSigninInfo(mPort, 0x00000002, &offlineSigninInfo) == ERROR_SUCCESS &&
           (mXuid == onlineSigninInfo.xuid || mXuid == offlineSigninInfo.xuid)))
      {
         XUserGetXUID(mPort, &mXuid);
      }
   }
}

//=============================================================================
// BUser::setProfile
//=============================================================================
void BUser::setProfile(BUserProfile* pProfile) 
{
   if (mpProfile)
      delete mpProfile;

   mpProfile = pProfile; 

   long currentNode = 0;
   
   if( mpProfile )
   {
      if( mpProfile->getTitle1VersionChanged() )
      {
         resetAllOptions();
         gUserProfileManager.writeProfile(this);

         BUIGlobals* puiGlobals = gGame.getUIGlobals();
         if (puiGlobals)
         {
            DWORD value = (cUserYornBoxResetOptionsNotification << 16);
            // Profile has changed.Your options have been reset.
            puiGlobals->showYornBoxSmall(this, gDatabase.getLocStringFromID(25352), BUIGlobals::cDialogButtonsOK, value);
         }
      }

      validateUintOptions();
      if( gUserManager.getPrimaryUser() == this )
         applyAllOptions();
      currentNode = mpProfile->mCampaignProgress.getCurrentCampaignNode();
   }

   // initialize campaign progress
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   // reset things first
   pCampaign->resetCampaign();

   // now let's get this campaign seeded.
   pCampaign->setCurrentNodeID(currentNode);
}

//=============================================================================
// BUser::initializeAchievements
//=============================================================================
void BUser::initializeAchievements()
{
   if (mpProfile != NULL)
      mpProfile->initializeAchievements();

   //if (mpAchievementList != NULL)
   //   delete mpAchievementList;

   //note: if this is ever added back, initialize and clean up this variable. 
   //mpAchievementList = new BUserAchievementList();
   //mpAchievementList->initialize();
}

//=============================================================================
// BUser::checkPrivilege
//=============================================================================
BOOL BUser::checkPrivilege(XPRIVILEGE_TYPE priv) const
{
   switch (priv)
   {
      case XPRIVILEGE_MULTIPLAYER_SESSIONS:
         return mPrivMultiplayer;
      case XPRIVILEGE_COMMUNICATIONS:
         return mPrivCommunications;
      case XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY:
         return mPrivCommunicationsFriends;
      case XPRIVILEGE_PROFILE_VIEWING:
         return mPrivProfileViewing;
      case XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY:
         return mPrivProfileViewingFriends;
      case XPRIVILEGE_USER_CREATED_CONTENT:
         return mPrivUserContent;
      case XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY:
         return mPrivUserContentFriends;
      case XPRIVILEGE_PURCHASE_CONTENT:
         return mPrivPurchaseContent;
      case XPRIVILEGE_PRESENCE:
         return mPrivPresence;
      case XPRIVILEGE_PRESENCE_FRIENDS_ONLY:
         return mPrivPresenceFriends;
   }
   return FALSE;
}

//==============================================================================
// BUser::isFriend
//==============================================================================
BOOL BUser::isFriend(XUID xuid) const
{
   if (mPort < 0 || mPort >= XUSER_MAX_COUNT)
      return FALSE;

   if (!isSignedIn())      // should this really check isSignedIntoLive()?
      return FALSE;

   BOOL result = FALSE;

   return (XUserAreUsersFriends(mPort, &xuid, 1, &result, NULL) == ERROR_SUCCESS) && result;
}

//=============================================================================
// BUser::clearCameraState
//=============================================================================
void BUser::clearCameraState()
{
   mCameraAdjustZoom=0.0f;
   mCameraAdjustYaw=0.0f;
   mCameraAdjustFOV=0.0f;
   mCameraAutoZoomOutTime=0.0f;
   mCameraAutoZoomInTime=0.0f;

   mPickChangeXMinus=false;
   mPickChangeXPlus=false;
   mPickChangeYMinus=false;
   mPickChangeYPlus=false;

   mSelectionChangeXPlus=false;
   mSelectionChangeXMinus=false;
   mSelectionChangeZPlus=false;
   mSelectionChangeZMinus=false;

   mObstructionChangeXPlus=false;
   mObstructionChangeXMinus=false;
   mObstructionChangeYPlus=false;
   mObstructionChangeYMinus=false;
   mObstructionChangeZPlus=false;
   mObstructionChangeZMinus=false;
}

//=============================================================================
// BUser::getPlayer
//=============================================================================
BPlayer* BUser::getPlayer()
{
   return(gWorld->getPlayer(mPlayerID));
}

const BPlayer* BUser::getPlayer() const
{
   return(gWorld->getPlayer(mPlayerID));
}

//=============================================================================
// BUser::getCoopPlayer
//=============================================================================
BPlayer* BUser::getCoopPlayer()
{
   return(gWorld->getPlayer(mCoopPlayerID));
}

const BPlayer * BUser::getCoopPlayer() const
{
   return(gWorld->getPlayer(mCoopPlayerID));
}

//==============================================================================
// BUser::resetCameraDefaults
//==============================================================================
void BUser::resetCameraDefaults()
{
   mCameraDefaultPitch=38.0f;
   mCameraDefaultYaw=45.0f;
   mCameraDefaultZoom=getOption_DefaultZoomLevel();
   mCameraZoomMin=20.0f;
   mCameraZoomMax=150.0f;
   mCameraPitchMin=30.0f;
   mCameraPitchMax=60.0f;
   gConfig.get(cConfigCameraPitch, &mCameraDefaultPitch);
   gConfig.get(cConfigCameraYaw, &mCameraDefaultYaw);
   gConfig.get(cConfigCameraZoomMin, &mCameraZoomMin);
   gConfig.get(cConfigCameraZoomMax, &mCameraZoomMax);
   gConfig.get(cConfigCameraPitchMin, &mCameraPitchMin);
   gConfig.get(cConfigCameraPitchMax, &mCameraPitchMax);

   calcCameraDefaultPitch();
}

//==============================================================================
// BUser::resetCameraSettings
//==============================================================================
/* float BUser::getDefaultZoom() const
{
   return getOption_DefaultZoomLevel();
}*/

//==============================================================================
// BUser::resetCameraSettings
//==============================================================================
void BUser::resetCameraSettings(bool onlyZoomAndPitch)
{
   if (mFlagCameraZoomEnabled)
   {
      mCameraPitch = mCameraDefaultPitch;
      mCameraZoom = mCameraDefaultZoom; //getOption_DefaultZoomLevel();
   }
   else
   {
      BVector currentDir = mpCamera->getCameraDir();
      if (Math::fAbs(currentDir.y) > cFloatCompareEpsilon)
      {         
         BVector dir = cZAxisVector;
         currentDir.x = 0.0f;
         currentDir.normalize();
         float angle = dir.angleBetweenVector(currentDir);
         angle = (currentDir.y > 0.0f) ? -angle : angle;
         mCameraPitch = Math::fRadToDeg(angle);
      }
      else
      {
         mCameraPitch = 0.0f;
      }

      if (mCameraPitch >= 360.0f)
      {
         mCameraPitch -= 360.0f;
      }
      else if (mCameraPitch < 0.0f)
      {
         mCameraPitch += 360.0f;
      }

      if (getFlagHaveHoverPoint())
      {
         mCameraZoom = mCameraHoverPoint.distance(mpCamera->getCameraLoc());
      }
      else
      {
         BVector intersectionPt;
         if (gTerrainSimRep.rayIntersectsCamera(mpCamera->getCameraLoc(), mpCamera->getCameraDir(), intersectionPt))
         {
            mCameraZoom = intersectionPt.distance(mpCamera->getCameraLoc());
         }
         else
         {
            mCameraZoom = mCameraDefaultZoom; //getOption_DefaultZoomLevel();
         }
      }
   }

   if (mFlagCameraYawEnabled && !onlyZoomAndPitch)
   {
      mCameraYaw = mCameraDefaultYaw;
   }
   else
   {
      mCameraYaw = mpCamera->getXZYaw() * cDegreesPerRadian;
   }

   if (mCameraYaw >= 360.0f)
   {
      mCameraYaw -= 360.0f;
   }
   else if (mCameraYaw < 0.0f)
   {
      mCameraYaw += 360.0f;
   }

   mCameraFOV = 40.0f;
   gConfig.get(cConfigCameraFOV, &mCameraFOV);

   if (gGame.isSplitScreen())
   {
      if (!gGame.isVerticalSplit())
         mCameraFOV *= 0.5f;
   }

   mReticleOffset = 0.0f;
   gConfig.get(cConfigReticleOffset, &mReticleOffset);
   
   applyCameraSettings(true);
}


//==============================================================================
//==============================================================================
void BUser::restoreUIModeCameraLimitValues()
{
   if (mFlagUIModeRestoreCameraZoomMin)
   {
      mCameraZoomMin = mUIModeRestoreCameraZoomMin;
   }
   if (mFlagUIModeRestoreCameraZoomMax)
   {
      mCameraZoomMax = mUIModeRestoreCameraZoomMax;
   }
   if (mFlagUIModeRestoreCameraPitchMin)
   {
      mCameraPitchMin = mUIModeRestoreCameraPitchMin;
   }
   if (mFlagUIModeRestoreCameraPitchMax)
   {
      mCameraPitchMax = mUIModeRestoreCameraPitchMax;
   }

   mFlagCameraScrollEnabled = mFlagRestoreCameraEnableUserScroll;
   mFlagCameraYawEnabled = mFlagRestoreCameraEnableUserYaw;
   mFlagCameraZoomEnabled = mFlagRestoreCameraEnableUserZoom;
   mFlagCameraAutoZoomInstantEnabled = mFlagRestoreCameraEnableAutoZoomInstant;
   mFlagCameraAutoZoomEnabled = mFlagRestoreCameraEnableAutoZoom;

   clearUIModeCameraLimitValues();
}


//==============================================================================
//==============================================================================
void BUser::clearUIModeCameraLimitValues()
{
   mUIModeRestoreCameraZoomMin = 0.0f;
   mUIModeRestoreCameraZoomMax = 0.0f;
   mUIModeRestoreCameraPitchMin = 0.0f;
   mUIModeRestoreCameraPitchMax = 0.0f;

   mFlagUIModeRestoreCameraZoomMin = false;
   mFlagUIModeRestoreCameraZoomMax = false;
   mFlagUIModeRestoreCameraPitchMin = false;
   mFlagUIModeRestoreCameraPitchMax = false;

   mFlagRestoreCameraEnableUserScroll = true;
   mFlagRestoreCameraEnableUserYaw = true;
   mFlagRestoreCameraEnableUserZoom = true;
   mFlagRestoreCameraEnableAutoZoomInstant = gConfig.isDefined(cConfigCameraAutoZoomInstant);
   mFlagRestoreCameraEnableAutoZoom = true;
}


//==============================================================================
// BUser::applyCameraSettings
//==============================================================================
void BUser::applyCameraSettings(bool keepLookAtPos)
{
   BVector dir;
   dir=BVector(1.0f, 0.0f, 1.0f);
   dir.normalize();
   dir=cZAxisVector;
   dir.rotateXZ(mCameraYaw*cRadiansPerDegree);
   dir.normalize();
   mpCamera->setCameraDir(dir);
   mpCamera->setCameraUp(cYAxisVector);
   mpCamera->calcCameraRight();

   mpCamera->pitch(mCameraPitch*cRadiansPerDegree);

   if(keepLookAtPos && getFlagHaveHoverPoint())
   {
      BVector pos=mCameraHoverPoint-(mpCamera->getCameraDir()*mCameraZoom);
      mpCamera->setCameraLoc(pos);
   }
   else
      mpCamera->zoomTo(-mCameraZoom);

   mpCamera->setFOV(mCameraFOV*cRadiansPerDegree);
   //gRender.getViewParams().setFOV(mpCamera->getFOV());

   setFlagUpdateHoverPoint(false);
   setFlagTeleportCamera(true);
   saveLastCameraLoc();
}

//==============================================================================
//==============================================================================
void BUser::updatePowerCameraLimits()
{
//-- FIXING PREFIX BUG ID 5816
   const BProtoPower* pPP = gDatabase.getProtoPowerByID(mUIProtoPowerID);
//--
   if (pPP)
   {
      if (pPP->getFlagCameraZoomMin())
      {
         setUIModeRestoreCameraZoomMin(mCameraZoomMin);
         mCameraZoomMin = pPP->getCameraZoomMin();
      }
      if (pPP->getFlagCameraZoomMax())
      {
         setUIModeRestoreCameraZoomMax(mCameraZoomMax);
         mCameraZoomMax = pPP->getCameraZoomMax();
      }
      if (pPP->getFlagCameraPitchMin())
      {
         setUIModeRestoreCameraPitchMin(mCameraPitchMin);
         mCameraPitchMin = pPP->getCameraPitchMin();
      }
      if (pPP->getFlagCameraPitchMax())
      {
         setUIModeRestoreCameraPitchMax(mCameraPitchMax);
         mCameraPitchMax = pPP->getCameraPitchMax();
      }

      setUIModeRestoreCameraEnableUserScroll(mFlagCameraScrollEnabled);
      mFlagCameraScrollEnabled = pPP->getFlagCameraEnableUserScroll();
      setUIModeRestoreCameraEnableUserYaw(mFlagCameraYawEnabled);
      mFlagCameraYawEnabled = pPP->getFlagCameraEnableUserYaw();
      setUIModeRestoreCameraEnableUserZoom(mFlagCameraZoomEnabled);
      mFlagCameraZoomEnabled = pPP->getFlagCameraEnableUserZoom();
      setUIModeRestoreCameraEnableAutoZoomInstant(mFlagCameraAutoZoomInstantEnabled);
      mFlagCameraAutoZoomInstantEnabled = pPP->getFlagCameraEnableAutoZoomInstant();
      setUIModeRestoreCameraEnableAutoZoom(mFlagCameraAutoZoomEnabled);
      mFlagCameraAutoZoomEnabled = pPP->getFlagCameraEnableAutoZoom();
   }
}

//==============================================================================
//==============================================================================
void BUser::updateModeCameraEffect(long oldMode, long newMode)
{
   // Setup camera limit stuff from powers
   // TODO - put these limits in the same system with the transition effect
   if ((newMode == cUserModePower) || (newMode == cUserModeInputUILocation) || ((newMode == cUserModeInputUILocationMinigame) && mFlagUIPowerMinigame))
   {
//-- FIXING PREFIX BUG ID 5817
      const BProtoPower* pPP = gDatabase.getProtoPowerByID(mUIProtoPowerID);
//--
      if (pPP)
         updatePowerCameraLimits();
   }
   else if ((oldMode == cUserModePower) || (oldMode == cUserModeInputUILocation) || (oldMode == cUserModeInputUILocationMinigame))
      restoreUIModeCameraLimitValues();


   // Transition to camera effects
   bool transitionTo = false;
   int camEffectIndex = -1;

   if (newMode == cUserModeCommandMenu || newMode == cUserModePowerMenu || newMode == cUserModePower || newMode == cUserModeInputUILocation || newMode == cUserModeInputUILocationMinigame)
   {
      // Default to cached transition out effect
      camEffectIndex = mCameraEffectTransitionOutIndex;

      // Reset transition out index if we're going transitioning in to a new mode
      mCameraEffectTransitionOutIndex = -1;

      // MPB TODO - cache the various camera effect indices so we don't have to do a string search
      if (newMode == cUserModeCommandMenu)
      {
         camEffectIndex = gCameraEffectManager.getOrCreateCameraEffect("CircleMenu");
         mCameraEffectTransitionOutIndex = gCameraEffectManager.getOrCreateCameraEffect("CircleMenuToNormal");
      }
      else if (newMode == cUserModePowerMenu)
      {
         camEffectIndex = gCameraEffectManager.getOrCreateCameraEffect("PowerMenu");
         mCameraEffectTransitionOutIndex = gCameraEffectManager.getOrCreateCameraEffect("PowerMenuToNormal");
      }
      else if ((newMode == cUserModePower) || (newMode == cUserModeInputUILocation) || ((newMode == cUserModeInputUILocationMinigame) && mFlagUIPowerMinigame))
      {
//-- FIXING PREFIX BUG ID 5818
         const BProtoPower* pPP = gDatabase.getProtoPowerByID(mUIProtoPowerID);
//--
         if (pPP)
         {
            // Only set the effect if one exists
            if (pPP->getCameraEffectIn() != -1)
               camEffectIndex = pPP->getCameraEffectIn();
            mCameraEffectTransitionOutIndex = pPP->getCameraEffectOut();
         }
      }
      transitionTo = true;
   }
   // Transition from camera effects
   else if (oldMode == cUserModeCommandMenu || oldMode == cUserModePowerMenu || oldMode == cUserModePower || oldMode == cUserModeInputUILocation || oldMode == cUserModeInputUILocationMinigame)
   {
      // MPB TODO - cache the various camera effect indices so we don't have to do a string search
      if (oldMode == cUserModeCommandMenu)
         camEffectIndex = gCameraEffectManager.getOrCreateCameraEffect("CircleMenuToNormal");
      else if (oldMode == cUserModePowerMenu)
         camEffectIndex = gCameraEffectManager.getOrCreateCameraEffect("PowerMenuToNormal");
      else if (oldMode == cUserModePower || oldMode == cUserModeInputUILocation || oldMode == cUserModeInputUILocationMinigame)
         camEffectIndex = mCameraEffectTransitionOutIndex;

      // Reset transition out index
      mCameraEffectTransitionOutIndex = -1;
   }

   // Setup and apply camera effect
   mClearRestoreCamValuesOnEffectCompletion = false;
   const BCameraEffectData* pProtoCamEffect = gCameraEffectManager.getCameraEffect(camEffectIndex);
   if (pProtoCamEffect)
   {
      // Copy proto effect's data
      mModeCameraEffect = *gCameraEffectManager.getCameraEffect(camEffectIndex);

      // Set starting values to current cam values so we don't pop      
      uint viewportIndex = 0;
      if (gGame.isSplitScreen())
      {
         viewportIndex = 1;
         if (this == gUserManager.getSecondaryUser())
            viewportIndex = 0;
      }

      mModeCameraEffect.setInitialValuesToCurrent(mCameraZoom, mCameraYaw, mCameraPitch, viewportIndex);

      // If transitioning into a mode, save off the current camera pos / orient values
      // for restoring later.
      if (transitionTo)
      {
         // Set starting values to current cam values
         if (!mFlagUIModeRestoreCameraZoom)
            setUIModeRestoreCameraZoom(mCameraZoom);
         if (!mFlagUIModeRestoreCameraYaw)
            setUIModeRestoreCameraYaw(mCameraYaw);
         if (!mFlagUIModeRestoreCameraPitch)
            setUIModeRestoreCameraPitch(mCameraPitch);
      }
      // If transitioning out of a mode, set the camera pos / orient restore values as
      // the final values in the effect
      else
      {
         // Set end values to restore
         int numKeys = mModeCameraEffect.mZoomTable.getNumKeys();
         if (numKeys > 1)
            mModeCameraEffect.mZoomTable.setValue(numKeys - 1, mUIModeRestoreCameraZoom);
         numKeys = mModeCameraEffect.mYawTable.getNumKeys();
         if (numKeys > 1)
            mModeCameraEffect.mYawTable.setValue(numKeys - 1, mUIModeRestoreCameraYaw);
         numKeys = mModeCameraEffect.mPitchTable.getNumKeys();
         if (numKeys > 1)
            mModeCameraEffect.mPitchTable.setValue(numKeys - 1, mUIModeRestoreCameraPitch);

         // Clear out restore values
         mClearRestoreCamValuesOnEffectCompletion = true;
      }

      // Begin camera effect
      BCamera* pCamera = getCamera();
      if (pCamera)
         pCamera->beginCameraEffect(gWorld->getSubGametime(), &mModeCameraEffect, NULL, viewportIndex);
   }
}

//==============================================================================
//==============================================================================
void BUser::applyScreenBlur(bool active)
{
   int cameraEffectIdx = -1;
   BCamera* pCamera = gUserManager.getPrimaryUser()->getCamera();

   if (active)
      cameraEffectIdx = gCameraEffectManager.getOrCreateCameraEffect("ScreenBlur");
   else
      cameraEffectIdx = gCameraEffectManager.getOrCreateCameraEffect("ScreenBlurToNormal");

   const BCameraEffectData* pProtoCamEffect = gCameraEffectManager.getCameraEffect(cameraEffectIdx);

   if ( pProtoCamEffect)
   {
      mModeCameraEffect = *gCameraEffectManager.getCameraEffect(cameraEffectIdx);
      mModeCameraEffect.setInitialValuesToCurrent(getCameraZoom(), getCameraYaw(), getCameraPitch(), 0);
      pCamera->beginCameraEffect(gWorld->getSubGametime(), &mModeCameraEffect, NULL, 0);
   }
}

//==============================================================================
// BUser::showCommandMenu
//==============================================================================
bool BUser::showCommandMenu(BEntityID useObjectID)
{
   if(useObjectID!=cInvalidObjectID)
      mCommandObject=useObjectID;
   else
   {
      if(mSelectionManager->getNumberSelectedUnits()==0)
         return false;
      mCommandObject=mSelectionManager->getSelected(0);
   }

//-- FIXING PREFIX BUG ID 5819
   const BObject* pCommandObject=gWorld->getObject(mCommandObject);
//--
   if (!pCommandObject)
      return false;
   mCommandObjectPlayerID=pCommandObject->getPlayerID();

   long civID=gWorld->getPlayer(mPlayerID)->getCivID();

   long circleWidth=gUIGame.getCircleMenuWidth(this);

   mCircleMenu.resetPointingAt();
   mCircleMenu.setCircleWidth(circleWidth);
   mCircleMenu.autoPosition(this);
   mCircleMenu.setCircleCount(gUIGame.getCircleMenuCount());
   mCircleMenu.setItemRadius(gUIGame.getCircleMenuItemRadius(this));
   mCircleMenu.setItemWidth(gUIGame.getCircleMenuItemWidth(this));
   mCircleMenu.setTexture(gUIGame.getCircleMenuBackground(civID), true);
   mCircleMenu.setFont(gUIGame.getCircleMenuHelpTextFont());
   mCircleMenu.setColors(cDWORDCyan, cDWORDYellow, cDWORDRed, cDWORDOrange);
   mCircleMenu.setPlayer(mPlayerID);

   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      mpUIContext->showCircleMenu(BFlashHUD::eMenuNormal, L"");
   }

   setFlagCommandMenuRefresh(true);
   if(!refreshCommandMenu())
      return false;

   gUI.playClickSound();
   
   changeMode(cUserModeCommandMenu);

   mScrollXDelay=0.0f;
   mScrollYDelay=0.0f;

   mFlagOverrideUnitPanelDisplay=true;
   mFlagRestoreUnitPanelOnModeChange=true;
   updateSelectedUnitIcons();

   MVinceEventAsync_ControlUsed( this, "show_command_menu" );

   gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuShowCommand, mPlayerID, useObjectID); 

   return true;
}

//==============================================================================
// BUser::refreshCommandMenu
//==============================================================================
bool BUser::refreshCommandMenu()
{
   if( !getFlagCommandMenuRefresh() && !getFlagCommandMenuRefreshTrainProgressOnly() )
      return false;

   bool bUpdateTrainProgressOnly = getFlagCommandMenuRefreshTrainProgressOnly() && !getFlagCommandMenuRefresh();

   setFlagCommandMenuRefresh(false);
   setFlagCommandMenuRefreshTrainProgressOnly(false);

   bool bCostChanged = false;

   mCircleMenu.setBaseText(L"");
   mCircleMenu.setBaseTextDetail(L"");

   BUnit* pUnit=gWorld->getUnit(mCommandObject);
   if (!pUnit)
   {
      const BObject* pObject=gWorld->getObject(mCommandObject);
      if (pObject && mCommandObject.getType() == BEntity::cClassTypeDopple)
      {
         pUnit=gWorld->getUnit(pObject->getParentID());
      }
   }

   if(!pUnit)
   {
      mCircleMenu.clearItems();
      if(gConfig.isDefined(cConfigFlashGameUI))
      {
         mpUIContext->clearCircleMenuItems();
         mpUIContext->setCircleMenuBaseText(L"");
         mpUIContext->setCircleMenuBaseText2(L"");
         mpUIContext->setCircleMenuBaseTextDetail(L"");
      }
      return false;
   }

   BPlayer* pUserPlayer = getPlayer();
   const BProtoObject* pUnitProtoObject=pUnit->getProtoObject();
   bool isCommandableByAnyPlayer = pUnitProtoObject->getFlagCommandableByAnyPlayer();

   float baseRebuildTimer = 0.0f;
   if (isCommandableByAnyPlayer)
   {
      if (pUnit && pUnit->isType(gDatabase.getOTIDSettlement()) && pUnit->getAssociatedBase() == cInvalidObjectID)
      {
         const BUnitActionBuilding* pBuildingAction=(const BUnitActionBuilding*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding);
         if (pBuildingAction && pBuildingAction->getRebuildTimer() > 0.0f)
            baseRebuildTimer = pBuildingAction->getRebuildTimer();
      }
   }

   BUString name, detail, stats, infoPercentage, label, roleText;

   bool userCommandable=false;
   BPlayerID unitPlayerID=pUnit->getPlayerID();
   if (unitPlayerID == mPlayerID)
      userCommandable=true;
   else if (gWorld->getFlagCoop() && mCoopPlayerID==unitPlayerID)
      userCommandable=true;
   else if (isCommandableByAnyPlayer)
      userCommandable=true;

   if (!userCommandable && gWorld->getFlagCoop())
   {
      // Jira PHX-12729 - Host player needs to be able to access command menu for client player's locked down elephant.
      if (pUnit->getPlayer()->isHuman() && gWorld->getPlayer(mPlayerID)->getTeamID() == pUnit->getTeamID())
         userCommandable = true;
   }

   // FIXING PHX-18728 :
   // getProtoObject can return NULL.  I don't know why.  I don't know when.  If it happens, we don't
   // want to crash.  Closing the circle menu is the "graceful" exit. -PChapman
   const BProtoObject* pUserProtoObject=pUserPlayer->getProtoObject(pUnitProtoObject->getID());
   if( !pUserProtoObject )
   {
      changeMode(cUserModeNormal);
      return false;
   }

   uint commandCount=pUserProtoObject->getNumberCommands();
   //float timeRemaining=0.0f;

   if (!userCommandable)
   {
      mCircleMenu.clearItems();
      
      pUnitProtoObject->getDisplayName(name);
      BUString text;
      if(pUnit->getProtoObject()->getFlagCapturable())
      {
//-- FIXING PREFIX BUG ID 5821
         const BPlayer* pCapturePlayer=gWorld->getPlayer(pUnit->getCapturePlayerID());
//--
         text.locFormat(L"%s (%.0f%%)\n%s", name.getPtr(), pUnit->getCapturePercent()*100.0f, (pCapturePlayer ? pCapturePlayer->getLocalisedDisplayName().getPtr() : L""));
      }
      else if(pUnit->getPlayerID()!=0)
         text.locFormat(L"%s", name.getPtr());
      else
         text.locFormat(L"%s\n%s", name.getPtr(), pUnit->getPlayer()->getLocalisedDisplayName().getPtr());
      mCircleMenu.setBaseText(text);

      detail.empty();
      roleText.empty();
      if (pUnit->getPlayerID()==0)
      {
         int civID=pUserPlayer->getCivID();
         if (pUnitProtoObject->getGaiaRolloverTextIndex(civID)!=-1)
            pUnitProtoObject->getGaiaRolloverText(civID, detail);
      }
      else
      {
         if (pUnitProtoObject->getEnemyRolloverTextIndex()!=-1)
            pUnitProtoObject->getEnemyRolloverText(detail);
      }
      mCircleMenu.setBaseTextDetail(detail);

      if(gConfig.isDefined(cConfigFlashGameUI))
      {
         mpUIContext->clearCircleMenuItems();         
         mpUIContext->setCircleMenuBaseText(text);
         mpUIContext->setCircleMenuBaseText2(roleText);
         mpUIContext->setCircleMenuBaseTextDetail(detail);
      }
   }
#if 0  //Now that we have a verification prompt we don't need the time out.  Leaving commented out in case minds change.
   else if (pUnit->isSelfDestructing(timeRemaining))
   {
      mCircleMenu.clearItems();      
      pUnitProtoObject->getDisplayName(name);
      BUString format;
      if (pUnit->getAssociatedBase() == pUnit->getID())
         format = gDatabase.getLocStringFromID(373);
      else
         format = gDatabase.getLocStringFromID(22503);
      BUString text;
      text.format(format, timeRemaining);
      mCircleMenu.setBaseText(text);
      detail.empty();
      roleText.empty();
      if(gConfig.isDefined(cConfigFlashGameUI))
      {       
         gUIManager->clearCircleMenuItems();         
         gUIManager->setCircleMenuBaseText(text);
         gUIManager->setCircleMenuBaseText2(roleText);
         gUIManager->setCircleMenuBaseTextDetail(detail);
      }

      // Add a Cancel Kill menu option if this building has a Kill option
      for(uint i=0; i<commandCount; i++)
      {
         BProtoObjectCommand command=pUserProtoObject->getCommand(i);
         long type=command.getType();
         long position=command.getPosition();
         if(!pUserProtoObject->getCommandAvailable(i))
            continue;
         if (type == BProtoObjectCommand::cTypeKill)
         {
            type=BProtoObjectCommand::cTypeCancelKill;
            command.set(type, 0, position);
            BUString format = gDatabase.getLocStringFromID(22502);
            BUString text;
            text.format(format, timeRemaining);
            detail.empty();
            label.empty();
            mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, false, label, text, detail, gUIGame.getObjectCommandIcon(type));
            if(gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;                     
               gUIManager->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, false, label, text, roleText, detail, itemType, -1, type, -1);
            }
         }
         if (type == BProtoObjectCommand::cTypeDestroyBase)
         {
            type=BProtoObjectCommand::cTypeCancelDestroyBase;
            command.set(type, 0, position);
            BUString format = gDatabase.getLocStringFromID(372);
            BUString text;
            text.format(format, timeRemaining);
            detail.empty();
            label.empty();
            mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, false, label, text, detail, gUIGame.getObjectCommandIcon(type));
            if(gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;                     
               gUIManager->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, false, label, text, roleText, detail, itemType, -1, type, -1);
            }
         }
      }
   }
#endif
   else if (!pUnit->getFlagBuilt())
   {
      mCircleMenu.clearItems();      

      pUnitProtoObject->getDisplayName(name);
      pUnitProtoObject->getRoleText(roleText);

      BUString text;
      text.locFormat(L"%s (%.0f%%)", name.getPtr(), pUnit->getBuildPercent()*100.0f);
      mCircleMenu.setBaseText(text);

      detail.empty();
      mCircleMenu.setBaseTextDetail(detail);

      if(gConfig.isDefined(cConfigFlashGameUI))
      {       
         mpUIContext->clearCircleMenuItems();         
         mpUIContext->setCircleMenuBaseText(text);
         mpUIContext->setCircleMenuBaseText2(roleText);
         mpUIContext->setCircleMenuBaseTextDetail(detail);
      }

      // Add a Kill menu option if this building has a Kill option
      for(uint i=0; i<commandCount; i++)
      {
         BProtoObjectCommand command=pUserProtoObject->getCommand(i);
         long type=command.getType();
         long position=command.getPosition();
         if(!pUserProtoObject->getCommandAvailable(i))
            continue;
         if (type == BProtoObjectCommand::cTypeKill || type == BProtoObjectCommand::cTypeDestroyBase)
         {                       
            name=gDatabase.getLocStringFromID(23573);
            roleText = text;
            detail=gDatabase.getLocStringFromID(23574);
            mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, false, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if(gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;                     
               mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, false, label, name, roleText, detail, itemType, -1, type, -1);
            }
         }
      }
   }
   else if (isCommandableByAnyPlayer && baseRebuildTimer != 0.0f)
   {
      mCircleMenu.clearItems();      

      pUnitProtoObject->getDisplayName(name);
      pUnitProtoObject->getRoleText(roleText);

      BUString format = gDatabase.getLocStringFromID(374);
      BUString text;
      text.locFormat(format, baseRebuildTimer);
      mCircleMenu.setBaseText(text);

      detail.empty();
      mCircleMenu.setBaseTextDetail(detail);

      if(gConfig.isDefined(cConfigFlashGameUI))
      {       
         mpUIContext->clearCircleMenuItems();         
         mpUIContext->setCircleMenuBaseText(text);
         mpUIContext->setCircleMenuBaseText2(roleText);
         mpUIContext->setCircleMenuBaseTextDetail(detail);
      }
   }
   else
   {
      //const long cMaxItems=8;
      //bool itemUpdated[cMaxItems];
      //memset(itemUpdated, 0, sizeof(itemUpdated));
      //long itemCount=mCircleMenu.getNumberItems();
      mCircleMenu.clearItems();

      if (mFlagClearCircleMenuDisplay)
      {
         mpUIContext->clearCircleMenuDisplay();
         mFlagClearCircleMenuDisplay = false;
      }


      BPlayerID uniqueTechPlayerID=cInvalidPlayerID;
      long uniqueTechID=-1;
      float uniqueTechPercentComplete=0.0f;
      bool hasUniqueTech=false;
      const BUnitActionBuilding* pBuildingAction=(const BUnitActionBuilding*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding);
      if (pBuildingAction)
         hasUniqueTech=pBuildingAction->getUniqueTechInfo(uniqueTechPlayerID, uniqueTechID, uniqueTechPercentComplete);

      pUserProtoObject->getDisplayName(name);
      label.locFormat(L"%s", name.getPtr());
      mCircleMenu.setBaseText(label);
      pUserProtoObject->getRoleText(roleText);

      detail.empty();      
      if (pUserProtoObject->getRolloverTextIndex()!=-1)
      {
         pUserProtoObject->getRolloverText(detail);
         mCircleMenu.setBaseTextDetail(detail);
      }

      if(gConfig.isDefined(cConfigFlashGameUI) && !bUpdateTrainProgressOnly)
      {
         mpUIContext->clearCircleMenuItems();   
         mpUIContext->setCircleMenuBaseText(label);

         if (pBuildingAction && pBuildingAction->hasDescriptionOverride())
            mpUIContext->setCircleMenuBaseText2(pBuildingAction->getDescriptionOverride());
         else
            mpUIContext->setCircleMenuBaseText2(roleText);

         mpUIContext->setCircleMenuBaseTextDetail(detail);
      }

      // First add in custom command
      BSmallDynamicSimArray<BCustomCommand>& customCommands=gWorld->getCustomCommands();
      if (customCommands.getSize() > 0)
      {
         BPlayerID commandPlayerID = mPlayerID;
         if (gWorld->getFlagCoop() && mCoopPlayerID!=-1)
            commandPlayerID = mCoopPlayerID;

         for (uint i=0; i<customCommands.getSize(); i++)
         {
//-- FIXING PREFIX BUG ID 5822
            const BCustomCommand& customCommand=customCommands[i];
//--
            if (customCommand.mUnitID!=mCommandObject)
               continue;

            long existingIndex=mCircleMenu.getItemIndexByOrder(customCommand.mPosition);
            if(existingIndex!=-1)
               continue;

            long trainCount=-1;
            long trainLimit=-1;
            if (customCommand.mLimit!=0)
            {
               trainCount=customCommand.mQueuedCount+customCommand.mFinishedCount;
               trainLimit=customCommand.mLimit;
            }

            float trainPercent = 0.0f;
            if (pBuildingAction)
               trainPercent = pBuildingAction->getCustomCommandPercent(commandPlayerID , customCommand.mID);

            label.empty();
            roleText.empty();
            if (customCommand.mQueuedCount > 0)
            {
               if (customCommand.mFlagShowLimit)
               {
                  if (trainPercent>0.0f)
                     label.locFormat(L"%d (%.0f%%)", customCommand.mQueuedCount, trainPercent*100.0f);
                  else
                     label.locFormat(L"%d", customCommand.mQueuedCount);
               }
               else
               {
                  if (trainPercent>0.0f)
                     label.locFormat(L"%.0f%%", trainPercent*100.0f);
               }
            }

            name=gDatabase.getLocStringFromIndex(customCommand.mNameStringIndex);
            detail=gDatabase.getLocStringFromIndex(customCommand.mInfoStringIndex);

            int type = BProtoObjectCommand::cTypeCustomCommand;
            BProtoObjectCommand command;
            command.set(type, customCommand.mID, customCommand.mPosition);
            mCircleMenu.addItem(customCommand.mPosition, command, &customCommand.mCost, NULL, trainCount, trainLimit, -1, customCommand.mFlagUnavailable, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if(gConfig.isDefined(cConfigFlashGameUI))
            {
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc; 

               int customCommandIconID = customCommand.mPrimaryUserIconID;
               if (gGame.isSplitScreen() && gUserManager.getSecondaryUser() == this)
                  customCommandIconID = customCommand.mSecondaryUserIconID;

               if (bUpdateTrainProgressOnly)
                  mpUIContext->editCircleMenuItem(customCommand.mPosition, command, &customCommand.mCost, NULL, trainCount, trainLimit, trainPercent, -1, customCommand.mFlagUnavailable, label, name, roleText, detail, itemType, customCommandIconID, type, -1);
               else
                  mpUIContext->addCircleMenuItem(customCommand.mPosition, command, &customCommand.mCost, NULL, trainCount, trainLimit, trainPercent, -1, customCommand.mFlagUnavailable, label, name, roleText, detail, itemType, customCommandIconID, type, -1);
            }
         }
      }

      //bool checkedBuildOther = false;
      //bool buildOtherAllowed = true;
      BEntityID inProgressBuilding = cInvalidObjectID;

      for (uint i = 0; i < commandCount; i++)
      {
         BProtoObjectCommand command = pUserProtoObject->getCommand(i);
         long id = command.getID();
         long type = command.getType();
         long position = command.getPosition();

         // Grey out commands that are not available
         if (!pUserProtoObject->getCommandAvailable(i))
            continue;

         // Don't allow more than one item to take up the same position.
         // But give available items priority over unavailable ones.
         long existingIndex = mCircleMenu.getItemIndexByOrder(position);
         if (existingIndex!=-1)
         {
            if (!mCircleMenu.getItemUnavailable(existingIndex))
               continue;
            BProtoObjectCommand tempCommand;
            tempCommand.set(mCircleMenu.getItemID(existingIndex));
            if (tempCommand.getType() == BProtoObjectCommand::cTypeCustomCommand)
               continue;
         }

         label.empty();
         if ((type == BProtoObjectCommand::cTypeTrainUnit) || (type == BProtoObjectCommand::cTypeBuild) || (type == BProtoObjectCommand::cTypeBuildOther))
         {
            BProtoObject* pTrainProtoObject = pUserPlayer->getProtoObject(id);
            if (pTrainProtoObject && !pTrainProtoObject->getFlagForbid())
            {
               BCost cost;
               pTrainProtoObject->getCost(pUserPlayer, &cost, 0);
               bool unavail = (!pTrainProtoObject->getFlagAvailable() || !pUserPlayer->checkAvail(&cost));

               // if it's 'available', do one more check if it's a special commandable object
               if (!unavail && pUnitProtoObject->getFlagCommandableByAnyPlayer())
               {
                  // the user must have LOS to this object to be able to command it.
                  unavail = !pUnit->isVisible(pUserPlayer->getTeamID());

                  if (!unavail)
                  {
                     // make sure there are no buildings that were built from the base left by other people
                     uint numEntityRefs = pUnit->getNumberEntityRefs();
                     for (uint r = 0; r < numEntityRefs; r++)
                     {
//-- FIXING PREFIX BUG ID 5823
                        const BEntityRef* pSocketBaseRef = pUnit->getEntityRefByIndex(r);
//--
                        if (!pSocketBaseRef)
                           continue;
                        if (pSocketBaseRef->mType != BEntityRef::cTypeAssociatedBuilding)
                           continue;

//-- FIXING PREFIX BUG ID 5824
                        const BUnit* pSocketBaseUnit = gWorld->getUnit(pSocketBaseRef->mID);
//--
                        if (!pSocketBaseUnit)
                           continue;

                        // check the playerID
                        if (pSocketBaseUnit->getPlayerID() != pUserPlayer->getID())
                        {
                           unavail = true;
                           break;
                        }
                     }
                  }
               }

               if (unavail && !gConfig.isDefined(cConfigShowUnavailIcons))
                  continue;

               // See if this item is recharging
               bool recharging = false;
               if (!unavail)
               {
                  float rechargeTime = 0.0f;
                  if (pBuildingAction && pBuildingAction->getRecharging(false, id, &rechargeTime))
                  {
                     recharging = true;
                     label.locFormat(gDatabase.getLocStringFromID(25980),rechargeTime);
                     unavail = true;
                  }
               }

               if (existingIndex != -1)
               {
                  if (unavail)
                     continue;
                  else
                     mCircleMenu.removeItem(existingIndex);
               }
               long trainCount = -1;
               long trainLimit = -1;
               float trainPercent = 0.0f;
               if (pBuildingAction && !recharging)
               {
                  if (type == BProtoObjectCommand::cTypeTrainUnit)
                  {
                     pBuildingAction->getTrainCounts(mPlayerID, id, &trainCount, &trainLimit, false, false);
                     long queueCount = pBuildingAction->getTrainCount(mPlayerID, id);
                     trainPercent = pBuildingAction->getTrainPercent(mPlayerID, id);
                     if (queueCount > 0)                     
                        label.locFormat(L"%d", queueCount);

                     if (pBuildingAction->isQueuedItem(mPlayerID, id))
                        trainPercent = -1.0f;
                  }
                  else if (type == BProtoObjectCommand::cTypeBuildOther)
                  {
                     long queueCount = 0;
                     pBuildingAction->getTrainCounts(mPlayerID, id, &trainCount, &trainLimit, false, true);
                     queueCount = pBuildingAction->getBuildOtherCount(mPlayerID, id);
                     //if (queueCount > 1)
                     //   trainCount+=(queueCount-1);
                     trainPercent = pBuildingAction->getBuildOtherPercent(mPlayerID, id);
                     if (queueCount > 0)
                        label.locFormat(L"%d", queueCount);

                     if (pBuildingAction->isQueuedItem(mPlayerID, id))
                        trainPercent = -1.0f;
                  }
                  else if (type == BProtoObjectCommand::cTypeBuild)
                     pBuildingAction->getTrainCounts(mPlayerID, id, &trainCount, &trainLimit, false, false);
               }
               if (trainLimit == -1)
               {
                  // AJL 8/3/06 - Somewhat of a hack... Look for a pop that's not ID 0 and set the limit to it (used to show cov_bldg_icon_01 limit)
                  for (long i = 0; i < pTrainProtoObject->getNumberPops(); i++)
                  {
                     long popID = pTrainProtoObject->getPop(i).mID;
                     if (popID != 0)
                     {
                        trainCount = (long)pUserPlayer->getPopCount(popID);
                        trainLimit = (long)pUserPlayer->getPopCap(popID);

                        // [10/27/2008 xemu] show as unavailable in the UI if unable to build 
                        if (trainCount >= trainLimit)
                           unavail = true;
                     }
                  }
               }

               bool recovering = false;
               if (!unavail)
               {
                  // See if building is recovering from being destroyed
                  if (pUnit->getFlagDeathReplacementHealing())
                  {
                     unavail = true;
                     recovering = true;
                  }
               }

               pTrainProtoObject->getDisplayName(name);
               pTrainProtoObject->getRoleText(roleText);

               if (recovering)
                  detail = gDatabase.getLocStringFromID(230);               
               else
                  pTrainProtoObject->getRolloverText(detail);
//-- FIXING PREFIX BUG ID 5825
               const BProtoObject* pStatsProtoObject = pTrainProtoObject;
//--
               if (pTrainProtoObject->getBuildStatsProtoID() != -1)
               {
                  pStatsProtoObject = pUserPlayer->getProtoObject(pTrainProtoObject->getBuildStatsProtoID());
                  if (pStatsProtoObject == NULL)
                     pStatsProtoObject = pTrainProtoObject;
               }

//-- FIXING PREFIX BUG ID 5826
               const BProtoObject* pBaseStatsProtoObject = gDatabase.getGenericProtoObject(pStatsProtoObject->getBaseType());
//--
               if (!gConfig.isDefined(cConfigFlashGameUI))
               {
                  long statCount = gUIGame.getNumberUnitStats();
                  for (long j = 0; j < statCount; j++)
                  {
                     const BUIGameUnitStat* pUnitStat = gUIGame.getUnitStat(j);
                     stats.empty();
                     switch (pUnitStat->mStatType)
                     {
                        case BUIGameUnitStat::cTypeAttackRating:
                        {
                           if (!pStatsProtoObject->getHasAttackRatings())
                              continue;
                           float statValue = pStatsProtoObject->getAttackRating((int8)pUnitStat->mStatData);
                           if ((statValue > 0.0f) && (statValue < 1.0f))
                              statValue = 1.0f;
                           stats.locFormat(L"%s: %.f", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                           break;
                        }

                        case BUIGameUnitStat::cTypeDefenseRating:
                        {
                           float statValue = pStatsProtoObject->getDefenseRating();
                           if ((statValue > 0.0f) && (statValue < 1.0f))
                              statValue = 1.0f;
                           stats.locFormat(L"%s: %.f", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                           break;
                        }

                        case BUIGameUnitStat::cTypeAttackGrade:
                        {
                           if (!pStatsProtoObject->getHasAttackRatings())
                              continue;
                           uint statValue = pBaseStatsProtoObject->getAttackGrade((int8)pUnitStat->mStatData);
                           stats.locFormat(L"%s: %u", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                           break;
                        }
                     }
                     if (!detail.isEmpty())
                        detail.append(L"\\n");
                     detail.append(stats);
                  }
               }

               // Set command selectable override
               if (!unavail && !pUserProtoObject->getCommandSelectable(i))
               {
                  unavail = true;
               }

               mCircleMenu.addItem(position, command, &cost, pTrainProtoObject->getPops(), trainCount, trainLimit, -1, unavail, label, name, detail, gUIGame.getObjectIcon(id, getPlayerID()));
               if (gConfig.isDefined(cConfigFlashGameUI))
               {
                  int itemType = BFlashHUD::eCircleMenuItemTypeUnit;
                  if (pTrainProtoObject->getObjectClass() == cObjectClassBuilding)
                     itemType = BFlashHUD::eCircleMenuItemTypeBuilding;

                  if (bUpdateTrainProgressOnly)
                  {
                     if (mpUIContext->editCircleMenuItem(position, command, &cost, pTrainProtoObject->getPops(), trainCount, trainLimit, trainPercent, -1, unavail, label, name, roleText, detail, itemType, pTrainProtoObject->getCircleMenuIconID(), pTrainProtoObject->getID(), pStatsProtoObject->getID()))
                        bCostChanged = true;
                  }
                  else
                     mpUIContext->addCircleMenuItem(position, command, &cost, pTrainProtoObject->getPops(), trainCount, trainLimit, trainPercent, -1, unavail, label, name, roleText, detail, itemType, pTrainProtoObject->getCircleMenuIconID(), pTrainProtoObject->getID(), pStatsProtoObject->getID());
               }
            }
         }
         else if (type == BProtoObjectCommand::cTypeTrainSquad)
         {
//-- FIXING PREFIX BUG ID 5828
            const BProtoSquad* pTrainProtoSquad = pUserPlayer->getProtoSquad(id);
//--
            if (pTrainProtoSquad && !pTrainProtoSquad->getFlagForbid())
            {
               bool unavail = (!pTrainProtoSquad->getFlagAvailable() || !pUserPlayer->checkAvail(pTrainProtoSquad->getCost()));

               // if it's 'available', do one more check if it's a special commandable object
               if (!unavail && pUnitProtoObject->getFlagCommandableByAnyPlayer())
               {
                  // the user must have LOS to this object to be able to command it.
                  unavail = !pUnit->isVisible(pUserPlayer->getTeamID());
               }

               if (unavail && !gConfig.isDefined(cConfigShowUnavailIcons))
                  continue;
               if (existingIndex != -1)
               {
                  if (unavail)
                     continue;
                  else
                     mCircleMenu.removeItem(existingIndex);
               }

               // See if this item is recharging
               bool recharging = false;
               if (!unavail)
               {
                  float rechargeTime = 0.0f;
                  if (pBuildingAction && pBuildingAction->getRecharging(true, id, &rechargeTime))
                  {
                     recharging = true;
                     label.locFormat(gDatabase.getLocStringFromID(25980), rechargeTime);
                     unavail = true;
                  }
               }

               long trainCount = -1;
               long trainLimit = -1;
               float trainPercent = 0.0f;
               if ((type == BProtoObjectCommand::cTypeTrainSquad) && pBuildingAction && !recharging)
               {
                  pBuildingAction->getTrainCounts(mPlayerID, id, &trainCount, &trainLimit, true, false);
                  long queueCount = pBuildingAction->getTrainCount(mPlayerID, id);
                  trainPercent = pBuildingAction->getTrainPercent(mPlayerID, id) * 100.0f;
                  if (queueCount > 0)
                     label.locFormat(L"%d", queueCount);

                  if (pBuildingAction->isQueuedItem(mPlayerID, id))
                     trainPercent = -1.0f;                     
               }
               if (trainLimit == -1)
               {
                  // AJL 8/3/06 - Somewhat of a hack... Look for a pop that's not ID 0 and set the limit to it (used to show vulture pop limit)
                  BPopArray pops;
                  pTrainProtoSquad->getPops(pops);
                  if (pops.getNumber() > 0)
                  {
                     for (long i = 0; i < pops.getNumber(); i++)
                     {
                        long popID = pops[i].mID;
                        if (popID != 0)
                        {
                           trainCount = (long)pUserPlayer->getPopCount(popID);
                           trainLimit = (long)pUserPlayer->getPopCap(popID);

                           // VAT 06/30/2008 little bit more of a hack, but set to unavailable if we're at our limit
                           if (trainCount >= trainLimit)
                              unavail = true;
                        }
                     }
                  }
               }

               bool recovering = false;
               if (!unavail)
               {
                  // See if building is recovering from being destroyed
                  if (pUnit->getFlagDeathReplacementHealing())
                  {
                     unavail = true;
                     recovering = true;
                  }
               }

               BPopArray pops;
               pTrainProtoSquad->getPops(pops);
               pTrainProtoSquad->getDisplayName(name);
               pTrainProtoSquad->getRoleText(roleText);               

               if (recovering)
                  detail = gDatabase.getLocStringFromID(230);               
               else
                  pTrainProtoSquad->getRolloverText(detail);
//-- FIXING PREFIX BUG ID 5827
               const BProtoSquad* pBaseTrainProtoSquad = gDatabase.getGenericProtoSquad(pTrainProtoSquad->getBaseType());
//--
               long statCount = gUIGame.getNumberUnitStats();
               if (!gConfig.isDefined(cConfigFlashGameUI))
               {
                  for (long j = 0; j < statCount; j++)
                  {
                     const BUIGameUnitStat* pUnitStat = gUIGame.getUnitStat(j);
                     stats.empty();
                     switch (pUnitStat->mStatType)
                     {
                        case BUIGameUnitStat::cTypeAttackRating:
                        {
                           if (!pTrainProtoSquad->getHasAttackRatings())
                              continue;
                           float statValue = pTrainProtoSquad->getAttackRating((int8)pUnitStat->mStatData);
                           if ((statValue > 0.0f) && (statValue < 1.0f))
                              statValue = 1.0f;
                           stats.locFormat(L"%s: %.f", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                           break;
                        }

                        case BUIGameUnitStat::cTypeDefenseRating:
                        {
                           float statValue = pTrainProtoSquad->getDefenseRating();
                           if ((statValue > 0.0f) && (statValue < 1.0f))
                              statValue = 1.0f;
                           stats.locFormat(L"%s: %.f", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                           break;
                        }

                        case BUIGameUnitStat::cTypeAttackGrade:
                        {
                           if (!pTrainProtoSquad->getHasAttackRatings())
                              continue;
                           uint statValue = pBaseTrainProtoSquad->getAttackGrade((int8)pUnitStat->mStatData);
                           stats.format(L"%s: %u", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                           break;
                        }
                     }
                     if (!detail.isEmpty())
                        detail.append(L"\\n");
                     detail.append(stats);
                  }
               }

               // Set command selectable override
               if (!unavail && !pUserProtoObject->getCommandSelectable(i))
               {
                  unavail = true;
               }

               mCircleMenu.addItem(position, command, pTrainProtoSquad->getCost(), &pops, trainCount, trainLimit, -1, unavail, label, name, detail, gUIGame.getSquadIcon(id, getPlayerID()));
               if (gConfig.isDefined(cConfigFlashGameUI))
               {
                  int itemType = BFlashHUD::eCircleMenuItemTypeUnit;      

                  if (bUpdateTrainProgressOnly)
                     mpUIContext->editCircleMenuItem(position, command, pTrainProtoSquad->getCost(), &pops, trainCount, trainLimit, trainPercent, -1, unavail, label, name, roleText, detail, itemType, pTrainProtoSquad->getCircleMenuIconID(), pTrainProtoSquad->getID(), pTrainProtoSquad->getID());
                  else
                     mpUIContext->addCircleMenuItem(position, command, pTrainProtoSquad->getCost(), &pops, trainCount, trainLimit, trainPercent, -1, unavail, label, name, roleText, detail, itemType, pTrainProtoSquad->getCircleMenuIconID(), pTrainProtoSquad->getID(), pTrainProtoSquad->getID());
               }
            }
         }
         else if (type == BProtoObjectCommand::cTypeResearch)
         {
//-- FIXING PREFIX BUG ID 5831
            const BProtoTech* pResearchProtoTech = pUserPlayer->getProtoTech(id);
//--
            if (pResearchProtoTech && !pResearchProtoTech->getFlagForbid())
            {
               const BUnit* pTechUnit = pUnit;
               BPlayer* pTechPlayer = pUserPlayer;
               pResearchProtoTech = pTechPlayer->getProtoTech(id);
               long techStatus = pTechPlayer->getTechTree()->getTechStatus(id, pTechUnit->getID().asLong());
               if ((techStatus == BTechTree::cStatusActive) || (techStatus == BTechTree::cStatusDisabled) || (techStatus == BTechTree::cStatusUnobtainable))
                  continue;
               bool unavail = ((techStatus == BTechTree::cStatusObtainable) || !pUserPlayer->checkAvail(pResearchProtoTech->getCost()));
               if (techStatus == BTechTree::cStatusResearching || techStatus == BTechTree::cStatusCoopResearching)
               {
                  if (pTechPlayer->getTechTree()->getResearchBuilding(id, pTechUnit->getID().asLong()) != mCommandObject.asLong())
                     continue;
               }
               if (unavail && !gConfig.isDefined(cConfigShowUnavailIcons))
                  continue;
               if (hasUniqueTech && pResearchProtoTech->getFlagUnique() && (id != uniqueTechID))
                  continue;
               if (existingIndex != -1)
               {
                  if (unavail)
                     continue;
                  else
                     mCircleMenu.removeItem(existingIndex);
               }

               bool recovering = false;
               if (!unavail)
               {
                  // See if building is recovering from being destroyed
                  if (pUnit->getFlagDeathReplacementHealing())
                  {
                     unavail = true;
                     recovering = true;
                  }
               }

               float trainPercent = pTechPlayer->getTechTree()->getResearchPercent(id, pTechUnit->getID().asLong())*100.0f;  

               //FIXME - Add info text to proto techs
               pResearchProtoTech->getDisplayName(name);
               roleText.empty();

               if (pBuildingAction)
               {
                  if (pBuildingAction->isQueuedItem(mPlayerID, id))
                  {
                     trainPercent = -1.0f;
                     roleText = gDatabase.getLocStringFromID(25330);
                  }
               }
                             
               if (recovering)
                  detail = gDatabase.getLocStringFromID(230);
               else
                  pResearchProtoTech->getRolloverText(detail);
               if (hasUniqueTech && (id == uniqueTechID) && (mPlayerID != uniqueTechPlayerID))
               {
                  unavail = true;
                  detail = gDatabase.getLocStringFromID(354);
               }

               if ((pResearchProtoTech->getStatsProtoID() != -1) && (!gConfig.isDefined(cConfigFlashGameUI)))
               {
//-- FIXING PREFIX BUG ID 5830
                  const BProtoObject* pStatsProtoObject = pUserPlayer->getProtoObject(pResearchProtoTech->getStatsProtoID());
//--
                  if (pStatsProtoObject)
                  {
//-- FIXING PREFIX BUG ID 5829
                     const BProtoObject* pBaseStatsProtoObject = gDatabase.getGenericProtoObject(pStatsProtoObject->getBaseType());
//--
                     long statCount = gUIGame.getNumberUnitStats();
                     for (long j = 0; j < statCount; j++)
                     {
                        const BUIGameUnitStat* pUnitStat=gUIGame.getUnitStat(j);
                        stats.empty();
                        switch(pUnitStat->mStatType)
                        {
                           case BUIGameUnitStat::cTypeAttackRating:
                           {
                              if (!pStatsProtoObject->getHasAttackRatings())
                                 continue;
                              float statValue = pStatsProtoObject->getAttackRating((int8)pUnitStat->mStatData);
                              if ((statValue > 0.0f) && (statValue < 1.0f))
                                 statValue = 1.0f;
                              stats.locFormat(L"%s: %.f", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                              break;
                           }

                           case BUIGameUnitStat::cTypeDefenseRating:
                           {
                              float statValue = pStatsProtoObject->getDefenseRating();
                              if ((statValue > 0.0f) && (statValue < 1.0f))
                                 statValue = 1.0f;
                              stats.locFormat(L"%s: %.f", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                              break;
                           }

                           case BUIGameUnitStat::cTypeAttackGrade:
                           {
                              if (!pStatsProtoObject->getHasAttackRatings())
                                 continue;
                              uint statValue = pBaseStatsProtoObject->getAttackGrade((int8)pUnitStat->mStatData);
                              stats.locFormat(L"%s: %u", gDatabase.getLocStringFromIndex(pUnitStat->mDisplayNameIndex).getPtr(), statValue);
                              break;
                           }
                        }
                        if (!detail.isEmpty())
                           detail.append(L"\\n");
                        detail.append(stats);
                     }
                  }
               }

               // Set command selectable override
               if (!unavail && !pUserProtoObject->getCommandSelectable(i))
               {
                  unavail = true;
               }

               mCircleMenu.addItem(position, command, pResearchProtoTech->getCost(), NULL, -1, -1, -1, unavail, label, name, detail, gUIGame.getTechIcon(id));
               if (gConfig.isDefined(cConfigFlashGameUI))
               {
                  int itemType = BFlashHUD::eCircleMenuItemTypeTech;    
                  if (bUpdateTrainProgressOnly)
                     mpUIContext->editCircleMenuItem(position, command, pResearchProtoTech->getCost(), NULL, -1, -1, trainPercent, -1, unavail, label, name, roleText, detail, itemType, pResearchProtoTech->getCircleMenuIconID(), pResearchProtoTech->getID(), pResearchProtoTech->getStatsProtoID());
                  else
                     mpUIContext->addCircleMenuItem(position, command, pResearchProtoTech->getCost(), NULL, -1, -1, trainPercent, -1, unavail, label, name, roleText, detail, itemType, pResearchProtoTech->getCircleMenuIconID(), pResearchProtoTech->getID(), pResearchProtoTech->getStatsProtoID());
               }
            }
         }
         else if (type == BProtoObjectCommand::cTypeUnloadUnits)
         {
            bool unavail = (!pUserProtoObject->getCommandSelectable(i) || !pUnit->getFlagHasGarrisoned() && !pUnit->getFlagHasAttached());
            if (!unavail)
            {
               BEntityIDArray containedUnits = pUnit->getGarrisonedUnits(getPlayerID());
               unavail = (containedUnits.getSize() == 0);
            }
            name = gDatabase.getLocStringFromID(358);
            detail = gDatabase.getLocStringFromID(359);
            roleText.empty();
            mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, unavail, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;        
               if (bUpdateTrainProgressOnly)
                  mpUIContext->editCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               else
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
            }
         }
         else if (type == BProtoObjectCommand::cTypeTrainLock)
         {
            bool unavail = false;
//-- FIXING PREFIX BUG ID 5834
            const BSquad* pSquad = pUnit->getParentSquad();
//--
            if (pSquad && pSquad->getFlagChangingMode())
               unavail = true;
            if (pUnit->getTrainLock(pUserPlayer->getID()))
            {
               type = BProtoObjectCommand::cTypeTrainUnlock;
               command.set(type, id, position);
               name = gDatabase.getLocStringFromID(362);
               detail = gDatabase.getLocStringFromID(363);
            }
            else
            {
               name = gDatabase.getLocStringFromID(360);
               detail = gDatabase.getLocStringFromID(361);
            }
            roleText.empty();

            // Set command selectable override
            if (!unavail && !pUserProtoObject->getCommandSelectable(i))
            {
               unavail = true;
            }

            mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, unavail, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;        
               if (bUpdateTrainProgressOnly)
                  mpUIContext->editCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               else
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
            }

            // Update base circle menu text
            pUserProtoObject->getDisplayName(name);
            pUserProtoObject->getRoleText(roleText);

            if (pUnit->getTrainLock(mPlayerID))
            {
               BEntityIDArray containedSquads;
               for (uint i=0; i<pUnit->getNumberEntityRefs(); i++)
               {
//-- FIXING PREFIX BUG ID 5833
                  const BEntityRef* pRef=pUnit->getEntityRefByIndex(i);
//--
                  if (pRef->mType == BEntityRef::cTypeContainUnit)
                  {
//-- FIXING PREFIX BUG ID 5832
                     const BUnit* pContainedUnit = gWorld->getUnit(pRef->mID);
//--
                     if (pContainedUnit && pContainedUnit->getPlayerID()==mPlayerID)
                     {
                        BEntityID parentSquadID = pContainedUnit->getParentID();
                        if (parentSquadID != cInvalidObjectID)
                           containedSquads.uniqueAdd(parentSquadID);
                     }
                  }
               }

               label = name;
               detail=gDatabase.getLocStringFromID(365);
               roleText.locFormat(detail.getPtr(), containedSquads.getNumber());
            }
            else
            {
               label = name;
               roleText=gDatabase.getLocStringFromID(364);
            }
            mCircleMenu.setBaseText(label);            
            if(gConfig.isDefined(cConfigFlashGameUI) && !bUpdateTrainProgressOnly)
            {
               mpUIContext->setCircleMenuBaseText(label);
               mpUIContext->setCircleMenuBaseText2(roleText);
            }
         }
         else if (type == BProtoObjectCommand::cTypeChangeMode)
         {
            bool unavail = !pUserProtoObject->getCommandSelectable(i);
            long squadMode = id;
//-- FIXING PREFIX BUG ID 5835
            const BSquad* pSquad = pUnit->getParentSquad();
//--
            if (pSquad && (pSquad->getCurrentOrChangingMode() == id))
               squadMode = BSquadAI::cModeNormal;
            command.set(type, squadMode, position);
            name = gDatabase.getLocStringFromID(366);
            detail = gDatabase.getLocStringFromID(367);
            roleText.empty();
            mCircleMenu.addItem(position, command, NULL, NULL, -1,-1,-1, unavail, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;
               if (bUpdateTrainProgressOnly)
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1,-1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               else
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1,-1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
            }
         }
         else if (type == BProtoObjectCommand::cTypeAbility)
         {
//-- FIXING PREFIX BUG ID 5788
            const BAbility* pAbility = gDatabase.getAbilityFromID(id);
//--
            if (pAbility && pUserPlayer->canUseAbility(id))
            {
               bool unavail = !pUserProtoObject->getCommandSelectable(i);
               pAbility->getDisplayName(name);
               pAbility->getRolloverText(detail);
               roleText.empty();
               mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, unavail, label, name, detail, gUIGame.getReticleTexture(BUIGame::cReticleGarrison));
               if(gConfig.isDefined(cConfigFlashGameUI))
               {
                  int itemType = BFlashHUD::eCircleMenuItemTypeMisc;
                  if (bUpdateTrainProgressOnly)
                     mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
                  else
                     mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               }
            }
         }
         else if (type == BProtoObjectCommand::cTypeKill)
         {
            bool unavail = !pUserProtoObject->getCommandSelectable(i);
            name = gDatabase.getLocStringFromID(22500);
            detail = gDatabase.getLocStringFromID(22501);
            roleText.empty();
            mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, unavail, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;
               if (bUpdateTrainProgressOnly)
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               else
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
            }
         }
         else if (type==BProtoObjectCommand::cTypeDestroyBase)
         {
            bool unavail = !pUserProtoObject->getCommandSelectable(i);
            name = gDatabase.getLocStringFromID(370);
            detail = gDatabase.getLocStringFromID(371);
            roleText.empty();
            mCircleMenu.addItem(position, command, NULL, NULL, -1, -1, -1, unavail, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;
               if (bUpdateTrainProgressOnly)
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               else
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
            }
         }
         else if (type == BProtoObjectCommand::cTypeTribute)
         {
            addTributeCommands();
         }
         else if (type == BProtoObjectCommand::cTypeReverseHotDrop)
         {
            bool unavail = !gotoItem(cGotoTypeHero, true);

            // Make sure we can hotdrop the leader
            if (!unavail)
            {
               // Already garrisoning, so we can't hotdrop
               BUnit* pLeader = gWorld->getUnit(pUserPlayer->getLeaderUnit());
               if (pLeader)
               {
//-- FIXING PREFIX BUG ID 5797
                  const BSquad* pSquad = pLeader->getParentSquad();
//--
                  if (pSquad && pSquad->getActionByTypeConst(BAction::cActionTypeSquadGarrison))
                     unavail = true;
               }

               // Hot drop pad is disabled, so we can't hotdrop
               if (pUnit && pUnit->getFlagBlockContain())
                  unavail = true;
            }
            name = gDatabase.getLocStringFromID(24939);
            detail = gDatabase.getLocStringFromID(24940);
            roleText.empty();
            BASSERT(pUserPlayer->getLeader());
            const BCost* pReverseHotDropCost = pUserPlayer->getLeader()->getReverseHotDropCost();
            mCircleMenu.addItem(position, command, pReverseHotDropCost, NULL, -1, -1, -1, unavail, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               //-- fix me make misc icons data driven through game ui xml
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;
               if (bUpdateTrainProgressOnly)
                  mpUIContext->addCircleMenuItem(position, command, pReverseHotDropCost, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               else
                  mpUIContext->addCircleMenuItem(position, command, pReverseHotDropCost, NULL, -1, -1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
            }
         }
         else if (type == BProtoObjectCommand::cTypePower)
         {
            BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(id);
            if (pProtoPower)
            {
               BPowerEntry* pPowerEntry = pUserPlayer->findPowerEntry(id);
               if (pPowerEntry)
               {
                  int useLimit = pUserPlayer->getPowerUseLimit(id);
                  int usedCount=0;
                  long availableCount=0;
                  long maxCount=0;
                  float maxRechargePercent=0.0f;
                  bool recharging=false;
                  for(long j=0; j<pPowerEntry->mItems.getNumber(); j++)
                  {
//-- FIXING PREFIX BUG ID 5837
                     const BPowerEntryItem& item=pPowerEntry->mItems[j];
//--
                     availableCount+=item.mUsesRemaining;
                     if (useLimit != 0)
                     {
                        maxCount+=useLimit;
                        usedCount+=item.mTimesUsed;
                     }
                     else
                     {
                        if(item.mUsesRemaining>0)
                           maxCount+=item.mUsesRemaining;
                        if (!pProtoPower->getFlagUnitPower() && !pProtoPower->getFlagMultiRechargePower() && !pProtoPower->getFlagInfiniteUses())
                           maxCount+=item.mTimesUsed;
                        else if(item.mRecharging)
                           maxCount++;
                     }
                     if(item.mRecharging)
                     {
                        DWORD rechargeTime = pUserPlayer->getPowerRechargeTime(id);
                        if(rechargeTime>0)
                        {
                           recharging=true;
                           float timeLeft;
                           if (gWorld->getGametime()>item.mNextGrantTime)
                              timeLeft=0.0f;
                           else
                              timeLeft=(float)(item.mNextGrantTime-gWorld->getGametime());
                           float rechargeDuration=(float)(rechargeTime);
                           float rechargePercent=1.0f-(timeLeft/rechargeDuration);
                           if(rechargePercent>maxRechargePercent)
                              maxRechargePercent=rechargePercent;
                        }
                     }
                  }

                  bool unavail = false;
                  bool blocked = false;

                  label.empty();

                  long techPrereqID=-1;
                  if(pProtoPower->getNumberTechPrereqs()>0)
                  {
                     long techID=pProtoPower->getTechPrereq(0);
                     if(pUserPlayer->getTechTree()->getTechStatus(techID, -1)!=BTechTree::cStatusActive)
                     {
                        techPrereqID=techID;
                        availableCount=0;
                        maxCount=0;
                        useLimit=0;
                        usedCount=0;
                        unavail = true;
                     }
                  }
                  if(recharging && ((useLimit==0 && availableCount<1) || (useLimit!=0 && usedCount<useLimit)))
                     label.locFormat(L"%.0f%%", maxRechargePercent*100.0f);
                  else if((pProtoPower->getFlagUnitPower() || pProtoPower->getFlagMultiRechargePower()) && maxCount>0)
                     label.locFormat(L"%d", availableCount);
                  if(useLimit!=0)
                     availableCount=useLimit-usedCount;
                  if(!pProtoPower->getFlagShowLimit() && ((!pProtoPower->getFlagUnitPower() && !pProtoPower->getFlagMultiRechargePower()) || maxCount==0))
                  {
                     availableCount=-1;
                     maxCount=-1;
                  }
                  if ((pProtoPower->getFlagUnitPower() || pProtoPower->getFlagMultiRechargePower()) && maxCount<=0)
                     unavail=true;
                  if (!unavail && pProtoPower->getFlagLeaderPower() && pUserPlayer->getFlagLeaderPowersBlocked())
                  {
                     unavail=true;
                     blocked=true;
                  }

                  DWORD availableTime = pUserPlayer->getPowerAvailableTime(pPowerEntry->mProtoPowerID);
                  if (!unavail && gWorld->getGametime() < availableTime)
                  {
                     unavail=true;
                     blocked=true;
                  }

                  pProtoPower->getDisplayName(name);
                  if(unavail && !blocked)
                     pProtoPower->getPrereqText(detail);
                  else if (blocked)
                  {
                     BUString str1, str2;
                     str1=gDatabase.getLocStringFromID(357);
                     pProtoPower->getRolloverText(str2);
                     detail.locFormat(L"%s %s", str1.getPtr(), str2.getPtr());
                  }
                  else
                     pProtoPower->getRolloverText(detail);

                  roleText.empty();

                  // Set command selectable override
                  if (!unavail && !pUserProtoObject->getCommandSelectable(i))
                  {
                     unavail = true;
                  }

                  int itemType = BFlashHUD::eCircleMenuItemTypePower;
                  if (pPowerEntry->mProtoPowerID == gDatabase.getPPIDRallyPoint())
                  {
                     (long&) command = pPowerEntry->mProtoPowerID;
                     itemType = BFlashHUD::eCircleMenuItemTypeMisc;
                     id = BUIGame::cGameCommandRallyPoint;
                  }


                  mCircleMenu.addItem(position, command, pProtoPower->getCost(), pProtoPower->getPop(), availableCount, maxCount, techPrereqID, unavail, label, name, detail, pProtoPower->getIconTextureName());
                  if(gConfig.isDefined(cConfigFlashGameUI))
                  {
                     if (bUpdateTrainProgressOnly)
                        mpUIContext->addCircleMenuItem(position, command, pProtoPower->getCost(), pProtoPower->getPop(), availableCount, maxCount, 0.0f, techPrereqID, unavail, label, name, roleText, detail, itemType, pProtoPower->getCircleMenuIconID(), id, -1);
                     else
                        mpUIContext->addCircleMenuItem(position, command, pProtoPower->getCost(), pProtoPower->getPop(), availableCount, maxCount, 0.0f, techPrereqID, unavail, label, name, roleText, detail, itemType, pProtoPower->getCircleMenuIconID(), id, -1);
                  }
               }
            }
         }
         else if (type == BProtoObjectCommand::cTypeRallyPoint)
         {
            bool unavail = !pUserProtoObject->getCommandSelectable(i);
            name = gDatabase.getLocStringFromID(23958);
            detail = gDatabase.getLocStringFromID(23959);
            roleText.empty();
            mCircleMenu.addItem(position, command, NULL, NULL, -1,-1,-1, unavail, label, name, detail, gUIGame.getObjectCommandIcon(type));
            if (gConfig.isDefined(cConfigFlashGameUI))
            {
               int itemType = BFlashHUD::eCircleMenuItemTypeMisc;
               if (bUpdateTrainProgressOnly)
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1,-1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
               else
                  mpUIContext->addCircleMenuItem(position, command, NULL, NULL, -1,-1, 0.0f, -1, unavail, label, name, roleText, detail, itemType, -1, type, -1);
            }
         }
      }

      if(mCircleMenu.getNumberItems()==0)
      {
         pUnitProtoObject->getDisplayName(name);
         BUString text;
         text.locFormat(L"%s", name.getPtr());
         mCircleMenu.setBaseText(text);
      }

      if(gConfig.isDefined(cConfigFlashGameUI))
      {
         if (mpUIContext->getNumberCircleMenuItems() == 0)
         {
            pUnitProtoObject->getDisplayName(name);
            BUString text;
            text.locFormat(L"%s", name.getPtr());

            detail.empty();
            roleText.empty();
            if (pUnitProtoObject->getRolloverTextIndex()!=-1)
               pUnitProtoObject->getRolloverText(detail);

            mpUIContext->setCircleMenuBaseText(text);
            mpUIContext->setCircleMenuBaseText2(roleText);
            mpUIContext->setCircleMenuBaseTextDetail(detail);
         }
      }
   }

   mCircleMenu.refresh();
   if(gConfig.isDefined(cConfigFlashGameUI))
   {      
      mpUIContext->refreshCircleMenu(bUpdateTrainProgressOnly && !bCostChanged);
   }

   return true;
}

//==============================================================================
// BUser::showPowerMenu
//==============================================================================
void BUser::showPowerMenu()
{
   if( gUIManager->isNonGameUIVisible() || !mFlagPowerMenuEnable)
      return;

   gUI.playClickSound();

   long civID=gWorld->getPlayer(mPlayerID)->getCivID();

   long circleWidth=gUIGame.getCircleMenuWidth(this);

   mCircleMenu.resetPointingAt();
   mCircleMenu.setCircleWidth(circleWidth);
   mCircleMenu.autoPosition(this);
   mCircleMenu.setCircleCount(gUIGame.getCircleMenuCount());
   mCircleMenu.setItemRadius(gUIGame.getCircleMenuItemRadius(this));
   mCircleMenu.setItemWidth(gUIGame.getCircleMenuItemWidth(this));
   mCircleMenu.setTexture(gUIGame.getCircleMenuBackground(civID), true);
   mCircleMenu.setFont(gUIGame.getCircleMenuHelpTextFont());
   mCircleMenu.setBaseText(gDatabase.getLocStringFromID(25353).getPtr());  // Leader Menu
   mCircleMenu.setBaseTextDetail(L"");
   mCircleMenu.setColors(cDWORDCyan, cDWORDYellow, cDWORDRed, cDWORDOrange);
   mCircleMenu.setPlayer(mPlayerID);

   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      BUString baseText;
      BUString baseText2;
      baseText = gDatabase.getLocStringFromID(118);
      baseText2.empty();
      mpUIContext->showCircleMenu(BFlashHUD::eMenuGod, baseText.getPtr());
      mpUIContext->setCircleMenuBaseText(baseText.getPtr());
      mpUIContext->setCircleMenuBaseText2(baseText2);
   }

   setFlagPowerMenuRefreshTrainProgressOnly(false);
   refreshPowerMenu();

   changeMode(cUserModePowerMenu);

   mFlagOverrideUnitPanelDisplay=true;
   mFlagRestoreUnitPanelOnModeChange=true;
   updateSelectedUnitIcons();

   mScrollXDelay=0.0f;
   mScrollYDelay=0.0f;

   MVinceEventAsync_ControlUsed( this, "show_power_menu" );

   gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuShowPower, mPlayerID);
}

//==============================================================================
// BUser::refreshPowerMenu
//==============================================================================
void BUser::refreshPowerMenu()
{
   setFlagPowerMenuRefresh(false);

   bool bUpdateRechargeOnly = getFlagPowerMenuRefreshTrainProgressOnly();
   setFlagPowerMenuRefreshTrainProgressOnly(false);

   mCircleMenu.clearItems();
   if(gConfig.isDefined(cConfigFlashGameUI))
   {
      if (!bUpdateRechargeOnly)
         mpUIContext->clearCircleMenuItems();
   }

   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
   {
      mCircleMenu.refresh();
      if(gConfig.isDefined(cConfigFlashGameUI))
         mpUIContext->refreshCircleMenu(bUpdateRechargeOnly);
      return;
   }

   BUString name, detail, label, roleText;

   // If we are not overriding the power menu
   if (!mFlagPowerMenuOverride)
   {
      // Support powers
      uint numSupportPowers = pPlayer->getNumberSupportPowers();
      for (uint i=0; i<numSupportPowers; i++)
      {
         if (pPlayer->getSupportPowerStatus(i) != BPlayer::cSupportPowerStatusAvailable)
            continue;
         const BLeaderSupportPower* pSupportPower = pPlayer->getSupportPower(i);
         name=gDatabase.getLocStringFromID(352);
         detail=gDatabase.getLocStringFromID(353);
         roleText.empty();
         long itemID=(-1 - i);
         mCircleMenu.addItem(pSupportPower->mIconLocation, itemID, NULL, NULL, -1, -1, -1, false, L"", name, detail, gUIGame.getGameCommandIcon(BUIGame::cGameCommandSelectPower));
         if(gConfig.isDefined(cConfigFlashGameUI))
         {
            int itemType = BFlashHUD::eCircleMenuItemTypeMisc;                                     
            mpUIContext->addCircleMenuItem(pSupportPower->mIconLocation, itemID, NULL, NULL, -1, -1, 0.0f, -1, false, L"", name, roleText, detail, itemType, -1, BUIGame::cGameCommandSelectPower, -1);
         }
      }
   }

   // Other powers
   long numPowerEntries = pPlayer->getNumPowerEntries();
   for (long i=0; i<min(numPowerEntries, 8); i++)
   {
      BPowerEntry *pPowerEntry = pPlayer->getPowerEntryAtIndex(i);
      if (!pPowerEntry)
         continue;

      BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(pPowerEntry->mProtoPowerID);
      if (!pProtoPower)
         continue;

      // [5/8/2008 xemu] don't display unit-specific powers here (megaturret)
      if (!pProtoPower->getFlagShowInPowerMenu())
         continue;

      int useLimit = pPlayer->getPowerUseLimit(pPowerEntry->mProtoPowerID);
      int usedCount=0;
      long availableCount=0;
      long maxCount=0;
      float maxRechargePercent=0.0f;
      bool recharging=false;
      for(long j=0; j<pPowerEntry->mItems.getNumber(); j++)
      {
//-- FIXING PREFIX BUG ID 5838
         const BPowerEntryItem& item=pPowerEntry->mItems[j];
//--
         availableCount+=item.mUsesRemaining;
         if (useLimit != 0)
         {
            maxCount+=useLimit;
            usedCount+=item.mTimesUsed;
         }
         else
         {
            if (pProtoPower->getFlagSequentialRecharge())
               maxCount += item.mChargeCap;
            else
            {
               if(item.mUsesRemaining>0)
                  maxCount+=item.mUsesRemaining;

               if(item.mRecharging)
                  maxCount++;
               // MPB 2/19/08 - This doesn't really make much sense, but I assume it was here for a reason.  Just commenting
               // out for now.
               //else if (!pProtoPower->getFlagUnitPower() && !pProtoPower->getFlagMultiRechargePower() && !pProtoPower->getFlagInfiniteUses())
               //   maxCount+=item.mTimesUsed;
            }
         }
         if(item.mRecharging)
         {
            DWORD rechargeTime = pPlayer->getPowerRechargeTime(pPowerEntry->mProtoPowerID);
            if(rechargeTime>0)
            {
               recharging=true;
               float timeLeft;
               if (gWorld->getGametime()>item.mNextGrantTime)
                  timeLeft=0.0f;
               else
                  timeLeft=(float)(item.mNextGrantTime-gWorld->getGametime());
               float rechargeDuration=(float)(rechargeTime);
               float rechargePercent=1.0f-(timeLeft/rechargeDuration);
               if(rechargePercent>maxRechargePercent)
                  maxRechargePercent=rechargePercent;
            }
         }
      }

      long iconLoc=pPowerEntry->mIconLocation;
      if(iconLoc==-1)
      {
         for(long k=0; k<pProtoPower->getNumberIconLocations(); k++)
         {
            long loc=pProtoPower->getIconLocation(k);
            bool used=false;
            for(long j=0; j<mCircleMenu.getNumberItems(); j++)
            {
               long order=mCircleMenu.getItemOrder(j);
               if(order==loc)
               {
                  used=true;
                  break;
               }
            }
            if(!used)
            {
               iconLoc=loc;
               break;
            }
         }
      }

      if(iconLoc!=-1)
      {
         bool unavail = false;
         bool blocked = false;
         bool showPreqreqText = false;

         label.empty();
         roleText.empty();

         long techPrereqID=-1;
         if ((pProtoPower->getNumberTechPrereqs() > 0) && !pPowerEntry->mFlagIgnoreTechPrereqs)
         {
            long techID=pProtoPower->getTechPrereq(0);
            if(pPlayer->getTechTree()->getTechStatus(techID, -1)!=BTechTree::cStatusActive)
            {
               techPrereqID=techID;
               availableCount=0;
               maxCount=0;
               useLimit=0;
               usedCount=0;
               unavail = true;
               showPreqreqText = true;
            }
         }
/*
         if(recharging && ((useLimit==0 && availableCount<1) || (useLimit!=0 && usedCount<useLimit) || pProtoPower->getFlagSequentialRecharge()))
            label.format(L"%.0f%%", maxRechargePercent*100.0f);
*/
         else if((pProtoPower->getFlagUnitPower() || pProtoPower->getFlagMultiRechargePower()) && maxCount>0)
            label.locFormat(L"%d", availableCount);
         if(useLimit!=0)
            availableCount=useLimit-usedCount;
         if(!pProtoPower->getFlagShowLimit() && ((!pProtoPower->getFlagUnitPower() && !pProtoPower->getFlagMultiRechargePower()) || maxCount==0))
         {
            availableCount=-1;
            maxCount=-1;
         }
         if ((pProtoPower->getFlagUnitPower() || pProtoPower->getFlagMultiRechargePower()) && maxCount<=0)
            unavail=true;

         if (!pPlayer->hasAvailablePowerEntryUses(pPowerEntry->mProtoPowerID))
            unavail=true;

         int iconItemType = BFlashHUD::eCircleMenuItemTypePower;
         int iconOwnerID = pPowerEntry->mProtoPowerID;
         BCost cost;
         if (pPowerEntry->mFlagIgnoreCost)
         {
            cost.zero();
         }
         else if (pPowerEntry->mProtoPowerID == gDatabase.getPPIDRepair())
         {
            unavail=!canRepair(&cost);
            mFlagRepairUnavail=unavail;
            iconItemType = BFlashHUD::eCircleMenuItemTypeMisc;
            iconOwnerID = BUIGame::cGameCommandRepair;
         }
         else if (pPowerEntry->mProtoPowerID == gDatabase.getPPIDRallyPoint())
         {
            iconItemType = BFlashHUD::eCircleMenuItemTypeMisc;
            iconOwnerID = BUIGame::cGameCommandRallyPoint;
         }
         else
         {
            const BCost* pCost = pProtoPower->getCost();
            if (pCost)
               cost = *pCost;
         }

         for (int i=0; i<cost.getNumberResources(); i++)
         {
            if (cost.get(i) > 0.0f && !gDatabase.getResourceDeductable(i))
            {
               if (pPlayer->getResource(i) < cost.get(i))
               {
                  unavail = true;
                  break;
               }
            }
         }

         if (!unavail && pProtoPower->getFlagLeaderPower() && pPlayer->getFlagLeaderPowersBlocked())
         {
            unavail=true;
            blocked=true;
         }

         DWORD availableTime = pPlayer->getPowerAvailableTime(pPowerEntry->mProtoPowerID);
         if (!unavail && gWorld->getGametime() < availableTime)
         {
            unavail=true;
            blocked=true;
         }

         pProtoPower->getDisplayName(name);
         if(unavail && !blocked && showPreqreqText)
            pProtoPower->getPrereqText(detail);
         else if (blocked)
         {
            BUString str1, str2;
            str1=gDatabase.getLocStringFromID(357);
            pProtoPower->getRolloverText(str2);
            detail.locFormat(L"%s %s", str1.getPtr(), str2.getPtr());
         }
         else
            pProtoPower->getRolloverText(detail);

         mCircleMenu.addItem(iconLoc, pPowerEntry->mProtoPowerID, &cost, pProtoPower->getPop(), availableCount, maxCount, techPrereqID, unavail, label, name, detail, pProtoPower->getIconTextureName());
         if(gConfig.isDefined(cConfigFlashGameUI))
         {
            if(bUpdateRechargeOnly && recharging && ((useLimit==0 && availableCount<1) || (useLimit!=0 && usedCount<useLimit) || pProtoPower->getFlagSequentialRecharge()))
            {
               float rechargePercent = maxRechargePercent*100.0f;
               mpUIContext->editCircleMenuItem(iconLoc, pPowerEntry->mProtoPowerID, &cost, pProtoPower->getPop(), -1, -1, rechargePercent, techPrereqID, unavail, label, name, roleText, detail, iconItemType, pProtoPower->getCircleMenuIconID(), iconOwnerID, -1);
            }
            else
               mpUIContext->addCircleMenuItem(iconLoc, pPowerEntry->mProtoPowerID, &cost, pProtoPower->getPop(), -1, -1, 0.0f, techPrereqID, unavail, label, name, roleText, detail, iconItemType, pProtoPower->getCircleMenuIconID(), iconOwnerID, -1);
         }
      }
   }

   if(gConfig.isDefined(cConfigFlashGameUI))
      mpUIContext->refreshCircleMenu(bUpdateRechargeOnly);

   mCircleMenu.refresh();
}

//==============================================================================
//==============================================================================
void BUser::refreshScores()
{
   if (!gConfig.isDefined(cConfigFlashGameUI))
      return;

   if (!getHUDItemEnabled(BUser::cHUDItemScore))
      return;

   BUString text;

   // Gaia is team 0, so start at team 1
   for (int i = 1; i < gWorld->getNumberTeams(); ++i)
   {
//-- FIXING PREFIX BUG ID 5840
      const BTeam* pTeam = gWorld->getTeam(i);
      if (pTeam == NULL)
         continue;
//--
      for (int j = 0; j < pTeam->getNumberPlayers(); j++)
      {
         int playerID = pTeam->getPlayerID(j);

//-- FIXING PREFIX BUG ID 5839
         const BPlayer* pPlayer = gWorld->getPlayer(playerID);
         if (pPlayer==NULL)
            continue;
//--

         if (pPlayer->isNPC())
            continue;
   
         //text.format("%s  %s [%d]", BStrConv::toA(pPlayer->getName()), BStrConv::toA(gDatabase.getLocStringFromID(186)), pPlayer->getStrength());
         text.locFormat(L"%1!s!  [%2!d!]", pPlayer->getLocalisedDisplayName().getPtr(), pPlayer->getStrength());
         
         mpUIContext->setPlayerScore(playerID, text);
         mpUIContext->setPlayerColor(playerID, (int)pPlayer->getColorIndex());
      }      
   }
}

//==============================================================================
// BUser::showSelectPowerMenu
//==============================================================================
void BUser::showSelectPowerMenu(uint supportPowerIndex, int supportPowerIconLocation)
{
   gUI.playClickSound();
   mSupportPowerIndex=supportPowerIndex;
   mSupportPowerIconLocation=supportPowerIconLocation;

   long civID=gWorld->getPlayer(mPlayerID)->getCivID();

   long circleWidth=gUIGame.getCircleMenuWidth(this);

   mCircleMenu.resetPointingAt();
   mCircleMenu.setCircleWidth(circleWidth);
   mCircleMenu.autoPosition(this);
   mCircleMenu.setCircleCount(gUIGame.getCircleMenuCount());
   mCircleMenu.setItemRadius(gUIGame.getCircleMenuItemRadius(this));
   mCircleMenu.setItemWidth(gUIGame.getCircleMenuItemWidth(this));
   mCircleMenu.setTexture(gUIGame.getCircleMenuBackground(civID), true);
   mCircleMenu.setFont(gUIGame.getCircleMenuHelpTextFont());
   mCircleMenu.setBaseText(gDatabase.getLocStringFromID(25354).getPtr()); // Select Power
   mCircleMenu.setBaseTextDetail(L"");
   mCircleMenu.setColors(cDWORDCyan, cDWORDYellow, cDWORDRed, cDWORDOrange);
   mCircleMenu.setPlayer(mPlayerID);

   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      mpUIContext->showCircleMenu(BFlashHUD::eMenuGod, gDatabase.getLocStringFromID(25354).getPtr()); //   SELECT POWER
   }

   mCircleMenu.clearItems();
   if(gConfig.isDefined(cConfigFlashGameUI))                  
      mpUIContext->clearCircleMenuItems();

//-- FIXING PREFIX BUG ID 5841
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
//--

   BUString name, detail, label, roleText;

   const BLeaderSupportPower* pSupportPower = pPlayer->getSupportPower(supportPowerIndex);

   long iconLoc=0;

   for (uint i=0; i<pSupportPower->mPowers.getSize(); i++)
   {
      long powerID=pSupportPower->mPowers[i];
      BProtoPower *pProtoPower = gDatabase.getProtoPowerByID(powerID);
      if (!pProtoPower)
         continue;

      label.empty();
      roleText.empty();
      pProtoPower->getDisplayName(name);
      pProtoPower->getChooseText(detail);
      mCircleMenu.addItem(iconLoc, i, pProtoPower->getCost(), pProtoPower->getPop(), -1, -1, -1, false, label, name, detail, pProtoPower->getIconTextureName());
      if(gConfig.isDefined(cConfigFlashGameUI))
      {
         int itemType = BFlashHUD::eCircleMenuItemTypePower;                                     
         mpUIContext->addCircleMenuItem(iconLoc, i, pProtoPower->getCost(), pProtoPower->getPop(), -1, -1, 0.0f, -1, false, label, name, roleText, detail, itemType, pProtoPower->getCircleMenuIconID(), powerID, -1);
      }

      switch (iconLoc)
      {
         case 0: iconLoc=2; break;
         case 2: iconLoc=6; break;
         case 6: iconLoc=4; break;
         case 4: iconLoc=1; break;
         case 1: iconLoc=7; break;
         case 7: iconLoc=3; break;
         case 3: iconLoc=5; break;
      }
   }

   if(gConfig.isDefined(cConfigFlashGameUI))
      mpUIContext->refreshCircleMenu(false);

   mCircleMenu.refresh();

   changeMode(cUserModePowerMenu);
   mSubMode=cSubModeSelectPower;

   MVinceEventAsync_ControlUsed( this, "show_select_power_menu" );

   gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuShowSelectPower, mPlayerID);

}

//==============================================================================
// BUser::addTriggerUIButtonRequest
//==============================================================================
void BUser::addTriggerUIButtonRequest(long scriptID, BTriggerVarID varID, long controlType, bool speedModifier, bool ignoreSpeedModifier, bool actionModifier, bool ignoreActionModifier, bool overrideGameInput, bool onRelease, bool continuous)
{
   BUserUIButtonRequest req;
   req.mTriggerScriptID=scriptID;
   req.mTriggerVarID=varID;
   req.mControlType=controlType;
   req.mSpeedModifier=speedModifier;
   req.mIgnoreSpeedModifier=ignoreSpeedModifier;
   req.mActionModifier=actionModifier;
   req.mIgnoreActionModifier=ignoreActionModifier;
   req.mOverrideGameInput=overrideGameInput;
   req.mOnRelease=onRelease;
   req.mContinuous=continuous;
   mInputUIButtonRequests.add(req);
}

//==============================================================================
// BUser::addTributeCommands
//==============================================================================
void BUser::addTributeCommands()
{
   BPlayer* pPlayer=getPlayer();
//-- FIXING PREFIX BUG ID 5844
   const BTeam* pTeam=pPlayer->getTeam();
//--

   BUString format, detail, label, text, roleText;

   long gameType = -1;
   gDatabase.getGameSettings()->getLong(BGameSettings::cGameType, gameType);

   if (gameType == BGameSettings::cGameTypeCampaign && gConfig.isDefined(cConfigCoopSharedResources))
      return;

   int count=0;

   int playerCount=pTeam->getNumberPlayers();
   for (int i=0; i<playerCount; i++)
   {
      BPlayerID tributePlayerID=pTeam->getPlayerID(i);
      if (tributePlayerID==mPlayerID)
         continue;

      if (gameType != BGameSettings::cGameTypeSkirmish)
      {
//-- FIXING PREFIX BUG ID 5842
         const BPlayer* pTributePlayer=gWorld->getPlayer(tributePlayerID);
//--
         if (pTributePlayer && !pTributePlayer->isHuman())
            continue;
      }

      for (uint j=0; j<gUIGame.getNumberTributes(); j++)
      {
         const BUIGameTribute* pTribute=gUIGame.getTribute(j);
         int position=pTribute->mItemLoc[count];
         float tributeAmount=gDatabase.getTributeAmount();
//-- FIXING PREFIX BUG ID 5843
         const BPlayer* pTributePlayer=gWorld->getPlayer(tributePlayerID);
//--
         // Move this to the role text area
/*
         format = gDatabase.getLocStringFromIndex(pTribute->mItemStringIndex);
         roleText.locFormat(format, pTributePlayer->getLocalisedDisplayName().getPtr());
*/
         roleText.set(pTributePlayer->getLocalisedDisplayName().getPtr());

         // hard code this to "SUPPLIES"
         text.set(gDatabase.getLocStringFromID(25592).getPtr());

         format = gDatabase.getLocStringFromIndex(pTribute->mDetailStringIndex);
         detail.locFormat(format, tributeAmount, pTributePlayer->getLocalisedDisplayName().getPtr());

         int type=BProtoObjectCommand::cTypeTribute;
         //int data=i<<16|pTribute->mResourceID;
         BProtoObjectCommand command;
         command.set(type, pTribute->mResourceID, i);

         BCost cost;
         cost.add(pTribute->mResourceID, tributeAmount + (tributeAmount * pPlayer->getTributeCost()));

         bool unavail = (pTributePlayer->getPlayerState() != BPlayer::cPlayerStatePlaying);

         mCircleMenu.addItem(position, command, &cost, NULL, -1, -1, -1, unavail, label, text, detail, gUIGame.getObjectCommandIcon(type));
         if(gConfig.isDefined(cConfigFlashGameUI))
         {
            //-- fix me make misc icons data driven through game ui xml
            int itemType = BFlashHUD::eCircleMenuItemTypeMisc;                     
            mpUIContext->addCircleMenuItem(position, command, &cost, NULL, -1, -1, 0.0f, -1, unavail, label, text, roleText, detail, itemType, -1, type, -1);
         }
      }

      count++;
      if (count>1)
         break;
   }
}

//==============================================================================
// BUser::showGameMenu
//==============================================================================
void BUser::showGameMenu()
{
   gUIManager->showNonGameUI( BUIManager::cGameMenu, this );
}

//==============================================================================
// BUser::showObjectiveMenu
//==============================================================================
void BUser::showObjectiveMenu()
{
   if (mUserMode != cUserModeFollow)
      resetUserMode();

   gUIManager->showNonGameUI( BUIManager::cObjectivesScreen, this );
   
   mFlagOverrideUnitPanelDisplay=true;
   mFlagRestoreUnitPanelOnModeChange=true;
   updateSelectedUnitIcons();

   MVinceEventAsync_ControlUsed( this, "objectives" );

   gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuShowObjectives, mPlayerID);

}

//==============================================================================
// BUser::setHoverVisual
//==============================================================================
void BUser::setHoverVisual(long protoObjectID)
{
   releaseHoverVisual();

//-- FIXING PREFIX BUG ID 5845
   const BProtoObject* pProtoObject=getPlayer()->getProtoObject(protoObjectID);
//--
   if(pProtoObject)
   {
      long visualIndex=pProtoObject->getProtoVisualIndex();
      if(visualIndex!=-1)
      {
         BMatrix identMat; identMat.makeIdentity();

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextUI);

         mpHoverVisual=gVisualManager.createVisual(visualIndex, false, cInvalidObjectID.asLong(), playerColor, identMat, cVisualDisplayPriorityNormal);
         if(mpHoverVisual)
         {
            mpHoverVisual->setAnimation(cActionAnimationTrack, cAnimTypeIdle, true, playerColor, identMat);
            if(getFlagHaveHoverPoint())
            {
               BMatrix worldMatrix;
               worldMatrix.makeTranslate(mHoverPoint);
               mpHoverVisual->updateWorldMatrix(worldMatrix, NULL);
            }
         }
      }
   }
}

//==============================================================================
// BUser::releaseHoverVisual
//==============================================================================
void BUser::releaseHoverVisual()
{
   if(mpHoverVisual)
   {
      gVisualManager.releaseVisual(mpHoverVisual);
      mpHoverVisual=NULL;
   }
}

//==============================================================================
// BUser::destroyAtCursor
//==============================================================================
void BUser::destroyAtCursor()
{
#ifndef BUILD_FINAL
   BUnit *pUnit = gWorld->getUnit(mHoverObject);
   if (pUnit)
   {
      BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
      if(pCommand)
      {
         pCommand->setSenderType(BCommand::cPlayer);
         pCommand->setSenders(1, &mPlayerID);
         pCommand->setRecipientType(BCommand::cGame);
         pCommand->setType(BGameCommand::cTypeDestroySquad);
         pCommand->setData(mHoverObject.asLong());
         gWorld->getCommandManager()->addCommandToExecute(pCommand);
      }
   }
#endif
}

//==============================================================================
// BUser::destroyUnitAtCursor
//==============================================================================
void BUser::destroyUnitAtCursor()
{
#ifndef BUILD_FINAL
   BUnit *pUnit = gWorld->getUnit(mHoverObject);
   if (pUnit)
   {
      BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
      if(pCommand)
      {
         pCommand->setSenderType(BCommand::cPlayer);
         pCommand->setSenders(1, &mPlayerID);
         pCommand->setRecipientType(BCommand::cGame);
         pCommand->setType(BGameCommand::cTypeDestroyUnit);
         pCommand->setData(mHoverObject.asLong());
         gWorld->getCommandManager()->addCommandToExecute(pCommand);
      }
   }
#endif
}

//==============================================================================
// BUser::doubleClickSelect
//==============================================================================
void BUser::doubleClickSelect()
{
   BEntityID selectableObject = mHoverObject;
   if (selectableObject == cInvalidObjectID || mHoverType != cHoverTypeSelect)
   {
      selectableObject = cInvalidObjectID;
      float clickSize=20.0f;
      gConfig.get(cConfigCircleSelectClickSize, &clickSize);
      mSelectionManager->populateSelectionsRect(mpCamera->getCameraLoc(), mHoverPoint, clickSize*0.5f, clickSize*0.5f, false, false);
      BObject* pObject=gWorld->getObject(mSelectionManager->getPossibleSelection(0));
      if(pObject)
      {
         int selectType = pObject->getSelectType(getTeamID());
         if ((selectType != cSelectTypeNone) && (selectType != cSelectTypeTarget) && (!pObject->getProtoObject()->getFlagMustOwnToSelect() || (!pObject->getDopple() && (pObject->getPlayerID() == mPlayerID || pObject->getPlayerID() == mCoopPlayerID))))
            selectableObject = pObject->getID();
      }
   }

   if (!selectableObject.isValid())
      return;

   if (!canSelectUnit(selectableObject, !getFlagModifierAction()))
      return;

   const BObject *pHoverObject = gWorld->getObject(selectableObject);
   if (!pHoverObject)
      return;

   if (pHoverObject->getPlayerID() != mPlayerID)
      return;

   const BProtoObject* pHoverPO = pHoverObject->getProtoObject();
   if (!pHoverPO)
      return;
   //long hoverProtoID = pHoverPO->getBaseType();
   int hoverSort = pHoverPO->getSubSelectSort();

   resetCircleSelectionCycle();
   resetFindCrowd();

   if (!getFlagModifierAction())
      mSelectionManager->clearSelections();

   if (pHoverObject->getSelectType(getTeamID())==cSelectTypeSingleUnit)
   {
      if (mSelectionManager->getNumberSelectedUnits()>0)
         return;
   }
   else if (pHoverObject->getSelectType(getTeamID())==cSelectTypeSingleType)
   {
      if (mSelectionManager->getNumberSelectedUnits()>0)
      {
         const BEntityIDArray& units=mSelectionManager->getSelectedUnits();
         for (uint i=0; i<units.getSize(); i++)
         {
//-- FIXING PREFIX BUG ID 5846
            const BUnit* pUnit=gWorld->getUnit(units[i]);
//--
            if (pUnit)
            {
               const BProtoObject* pUnitPO = pUnit->getProtoObject();
               //if (pUnitPO && pUnitPO->getBaseType()!=hoverProtoID)
               if (pUnitPO && pUnitPO->getSubSelectSort()!=hoverSort)
                  return;
            }
         }
      }
   }

   int baseSelectType=-1;
   //int baseProtoID=-1;
   int baseSort=-1;
   if (mSelectionManager->getNumberSelectedUnits() > 0)
   {
      const BEntityIDArray& units=mSelectionManager->getSelectedUnits();
      for (uint i=0; i<units.getSize(); i++)
      {
//-- FIXING PREFIX BUG ID 5847
         const BUnit* pUnit=gWorld->getUnit(units[i]);
//--
         if (pUnit)
         {
            const BProtoObject* pUnitPO = pUnit->getProtoObject();
            //baseProtoID=pUnitPO ? pUnitPO->getBaseType() : pUnit->getProtoID();
            baseSort=pUnitPO ? pUnitPO->getSubSelectSort() : -1;
            baseSelectType=pUnit->getSelectType(getTeamID());
            break;
         }
      }
   }

   BEntityHandle handle=cInvalidObjectID;
//-- FIXING PREFIX BUG ID 5848
   for(const BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
//--
   {
      if(pUnit->getPlayerID()!=mPlayerID)
         continue;

      const BProtoObject* pUnitPO = pUnit->getProtoObject();
      //int protoID = pUnitPO ? pUnitPO->getBaseType() : pUnit->getProtoID();
      //if(protoID!=hoverProtoID)
      //   continue;
      int unitSort = pUnitPO ? pUnitPO->getSubSelectSort() : -1;
      if (unitSort != hoverSort)
         continue;

      int selectType = pUnit->getSelectType(getTeamID());
      if(baseSelectType!=-1 && selectType!=baseSelectType)
         continue;

      //if (selectType==cSelectTypeSingleType && baseProtoID!=-1 && protoID!=baseProtoID)
      if (selectType==cSelectTypeSingleType && baseSort!=-1 && unitSort!=baseSort)
         continue;

      if(!canSelectUnit(pUnit->getID(), false))
         continue;

      if (pUnit->isOutsidePlayableBounds())
         continue;

      // ajl 4/28/07 - use the previous scene's volume culler to determine if the unit was rendered the last frame.
      const BBoundingBox* pBoundingBox=pUnit->getVisualBoundingBox();
      if(!isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
         continue;

      if (!Math::IsValidFloat(pUnit->getPosition().x))
         continue;

      if(mSelectionManager->selectUnit(pUnit->getID()))
      {
         playUnitSelectSound(pUnit->getID());

         if (selectType==cSelectTypeSingleUnit)
            break;

         if (baseSelectType==-1)
            baseSelectType=selectType;

         //if (baseProtoID==-1)
         //   baseProtoID=protoID;
         if (baseSort==-1)
            baseSort=unitSort;
      }
   }

   MVinceEventAsync_ControlUsed( this, "double_click_select" );

   gGeneralEventManager.eventTrigger(BEventDefinitions::cControlDoubleClickSelect, mPlayerID);

}

//==============================================================================
// BUser::findCrowd
//==============================================================================
bool BUser::findCrowd(long findType, bool noLookAt, bool checkOnly, long* pGotoTypeOut)
{
   if(!checkOnly)
   {
      resetCircleSelectionCycle();
      resetGotoSelected();
      resetStickyReticle();
   }

   if(pGotoTypeOut)
      *pGotoTypeOut=-1;

   BDynamicSimArray<BSquad*> squadList;

   //FIXME AJL 6/7/06 - Might be better to store a list of squads per player instead of having 
   // to check all squads in the world
   BEntityHandle handle=cInvalidObjectID;
   for(BSquad* pSquad=gWorld->getNextSquad(handle); pSquad!=NULL; pSquad=gWorld->getNextSquad(handle))
   {
      if(pSquad->getPlayerID()!=mPlayerID)
         continue;

      if(!pSquad->isAlive())
         continue;

      const BProtoObject* pProtoObject=pSquad->getProtoObject();
      if (!pProtoObject)
         continue;
      // ajl 3/10/08 - don't use dynamic select type so that things like locked down elephants will be included here
      //if(pSquad->getSelectType(getTeamID())!=cSelectTypeUnit)
      if (pProtoObject->getSelectType()!=cSelectTypeUnit)
         continue;

      long gotoType=pSquad->getGotoType();

      if(findType==cFindCrowdMilitary)
      {
         if(gotoType!=cGotoTypeInfantry && gotoType!=cGotoTypeVehicle && gotoType!=cGotoTypeAir && gotoType!=cGotoTypeHero)
            continue;
      }
      else if(findType==cFindCrowdIdleVillager)
      {
         if(gotoType!=cGotoTypeCivilian && gotoType!=cGotoTypeScout)
            continue;
      }
      else
         continue;

      if (!Math::IsValidFloat(pSquad->getPosition().x))
         continue;

      if (pSquad->getNumberChildren() == 1)
      {
         const BUnit* pUnit = pSquad->getLeaderUnit();
         if (!pUnit || !pUnit->isAlive() || pUnit->getFlagDown())
            continue;
      }

      if(pGotoTypeOut && *pGotoTypeOut==-1)
         *pGotoTypeOut=gotoType;

      squadList.add(pSquad);
   }

   long squadCount = squadList.getNumber();
   if(squadCount==0)
      return false;

   if(checkOnly)
      return true;

   float clusterNeighborDist=5.0f;
   gConfig.get(cConfigCrowdNeighborDistance, &clusterNeighborDist);
   float clusterNeighborDistSqr=clusterNeighborDist*clusterNeighborDist;

   typedef BDynamicSimArray<BSquad*> BSquadPtrArray;
   BDynamicSimArray<BSquadPtrArray*> clusterList;

   while(squadCount>0)
   {
      BSquad* pSquad=squadList[0];

      BSquadPtrArray* cluster=new BSquadPtrArray;
      if(!cluster)
         break;

      long clusterIndex=clusterList.add(cluster);
      if(clusterIndex==-1)
         break;

      if(cluster->add(pSquad)==-1)
         break;
      squadCount--;
      squadList[0]=squadList[squadCount];

      long gotoType=pSquad->getGotoType();

      for(long i=0; i<squadCount; i++)
      {
         BSquad* squad1=squadList[i];

         if(findType==cFindCrowdIdleVillager && squad1->getGotoType()!=gotoType)
            continue;

         const BVector& pos1=squad1->getPosition();
    
         long clusterSquadCount=cluster->getNumber();
         for(long j=0; j<clusterSquadCount; j++)
         {
//-- FIXING PREFIX BUG ID 5850
            const BSquad* squad2=cluster->get(j);
//--
            const BVector& pos2=squad2->getPosition();

            float distSqr=pos1.distanceSqr(pos2);
            if(distSqr<=clusterNeighborDistSqr)
            {
               if(cluster->add(squad1)==-1)
                  break;
               squadCount--;
               squadList[i]=squadList[squadCount];
               i--;
               break;
            }
         }
      }
   }

   if( !gUIManager->isNonGameUIVisible() )
      mSelectionManager->clearSelections();

   long clusterCount=clusterList.getNumber();
   if(clusterCount==0)
   {
      mCrowdIndex=-1;
      return false;
   }

   BDynamicSimArray<BVector> clusterPosList;
   if(clusterPosList.setNumber(clusterCount))
   {
      for(long i=0; i<clusterCount; i++)
      {
         BSquadPtrArray* cluster=clusterList[i];

//-- FIXING PREFIX BUG ID 5852
         const BSquad* bestSquad=NULL;
//--
         float bestDistSqr=0.0f;

         long clusterSquadCount=cluster->getNumber();
         for(long j=0; j<clusterSquadCount; j++)
         {
            BSquad* squad1=cluster->get(j);
            const BVector& pos1=squad1->getPosition();
            float totalDistSqr=0.0f;
            for(long k=0; k<clusterSquadCount; k++)
            {
               if(k==j)
                  continue;
//-- FIXING PREFIX BUG ID 5851
               const BSquad* squad2=cluster->get(k);
//--
               const BVector& pos2=squad2->getPosition();
               float distSqr=pos1.distanceSqr(pos2);
               totalDistSqr+=distSqr;
            }
            if(!bestSquad || totalDistSqr<bestDistSqr)
            {
               bestSquad=squad1;
               bestDistSqr=totalDistSqr;
            }
         }
         if(bestSquad)                                    
            clusterPosList[i]=bestSquad->getPosition();
         else
            clusterPosList[i]=cOriginVector;
      }

      for(long i=0; i<clusterCount; i++)
      {
         BSquadPtrArray* cluster=clusterList[i];
         long clusterSquadCount=cluster->getNumber();
         const BVector& clusterPos=clusterPosList[i];

         for(long j=0; j<clusterSquadCount; j++)
         {
            BSquad* pSquad=cluster->get(j);
            const BVector& pos=pSquad->getPosition();

            long gotoType=pSquad->getGotoType();

            long closestIndex=i;
            float closestDistSqr=pos.distanceSqr(clusterPos);

            for(long k=0; k<clusterCount; k++)
            {
               if(k==i)
                  continue;

               if(findType==cFindCrowdIdleVillager)
               {
                  BSquadPtrArray* cluster2=clusterList[k];
//-- FIXING PREFIX BUG ID 5853
                  const BSquad* squad2=cluster2->get(0);
//--
                  if(squad2 && squad2->getGotoType()!=gotoType)
                     continue;
               }

               float distSqr=pos.distanceSqr(clusterPosList[k]);
               if(distSqr<closestDistSqr)
               {
                  closestIndex=k;
                  closestDistSqr=distSqr;
               }
            }

            if(closestIndex!=i)
            {
               BSquadPtrArray* newCluster=clusterList[closestIndex];
               newCluster->add(pSquad);

               clusterSquadCount--;
               BSquad* moveSquad=cluster->get(clusterSquadCount);
               cluster->setAt(j, moveSquad);
               cluster->setNumber(clusterSquadCount);
               j--;
            }
         }
      }

      if(mCrowdIndex==-1)
      {
         if(getFlagHaveHoverPoint())
            mCrowdPos=mHoverPoint;
      }

      // Calculate fighting states of the clusters.
      BSmallDynamicSimArray<bool> fightingList;
      fightingList.setNumber(clusterCount);
      DWORD gameTime=gWorld->getGametime();
      for (int i=0; i<clusterCount; i++)
      {
         fightingList[i]=false;
//-- FIXING PREFIX BUG ID 5856
         const BSquadPtrArray* pCluster=clusterList[i];
//--
         uint squadCount=pCluster->getSize();
         for (uint j=0; j<squadCount; j++)
         {
            BSquad* pSquad=pCluster->get(j);
            if (gameTime-pSquad->getLastAttackedTime() < 5000 || gameTime-pSquad->getLastDamagedTime() < 5000)
            {
               fightingList[i]=true;
               break;
            }
         }
      }
      
      // Sort the clusters by size (clusters with the most squads first). Prioritize fighting squads highest.
      BDynamicSimLongArray sortedClusterList;
      for(long i=0; i<clusterCount; i++)
      {
         BSquadPtrArray* pCluster=clusterList[i];
         if(pCluster->getNumber()==0)
            continue;
         bool added=false;
         for(long j=0; j<sortedClusterList.getNumber(); j++)
         {
            long index2=sortedClusterList[j];
//-- FIXING PREFIX BUG ID 5855
            const BSquadPtrArray* pCluster2=clusterList[index2];
//--
            uint size1=pCluster->getSize();
            uint size2=pCluster2->getSize();
            if (fightingList[i])
               size1+=1000;
            if(size1>size2)
            {
               sortedClusterList.insertAtIndex(i, j);
               added=true;
               break;
            }
         }
         if(!added)
            sortedClusterList.add(i);
      }

      if(sortedClusterList.getNumber()==0)
      {
         mCrowdIndex=-1;
         return false;
      }

      mCrowdIndex++;
      if(mCrowdIndex>=sortedClusterList.getNumber())
         mCrowdIndex=0;

      long clusterIndex=sortedClusterList[mCrowdIndex];

      BVector avgPos(cOriginVector);
      BSquadPtrArray* cluster=clusterList[clusterIndex];
      long clusterSquadCount=cluster->getNumber();
      bool selectSoundPlayed = false;
      for(long j=0; j<clusterSquadCount; j++)
      {
         BSquad* squad1=cluster->get(j);
         uint unitCount=squad1->getNumberChildren();
         if( !gUIManager->isNonGameUIVisible() )
         {
            bool selected=(!squad1->getFlagGarrisoned() && !squad1->getFlagAttached() && squad1->getSelectType(getTeamID())==cSelectTypeUnit ? mSelectionManager->selectSquad(squad1->getID()) : true);
            if(selected && !selectSoundPlayed)
            {
               for (uint k=0; k<unitCount; k++)
               {
//-- FIXING PREFIX BUG ID 5849
                  const BUnit* pUnit=gWorld->getUnit(squad1->getChild(k));
//--
                  if (pUnit && !pUnit->isAttached() && !pUnit->isGarrisoned() && !pUnit->isHitched())
                  {
                     if (playUnitSelectSound(pUnit->getID(), true))
                     {
                        selectSoundPlayed = true;
                        break;
                     }
                  }
               }
            }
         }
         if (unitCount == 1 && squad1->getLeaderUnit())
            avgPos+=squad1->getLeaderUnit()->getInterpolatedPosition();
         else
            avgPos+=squad1->getInterpolatedPosition();
      }
      avgPos/=(float)clusterSquadCount;

      if(!noLookAt)
      {
         setGotoPosition(avgPos, cGotoTypeArmy);
         setFlagUpdateHoverPoint(true);

         // Sticky reticle follow
         if (getOption_CameraFollowEnabled())
         {
//-- FIXING PREFIX BUG ID 5863
            const BUnit* pClosestUnit=NULL;
//--
            float closestDist=cMaximumFloat;
            for(long j=0; j<clusterSquadCount; j++)
            {
               BSquad* pSquad=cluster->get(j);
               uint unitCount=pSquad->getNumberChildren();
               for (uint k=0; k<unitCount; k++)
               {
//-- FIXING PREFIX BUG ID 5857
                  const BUnit* pUnit=gWorld->getUnit(pSquad->getChild(k));
//--
                  if (pUnit)
                  {
                     const BUnit* pUseUnit = NULL;
                     if (pUnit->isAttached())
                     {
//-- FIXING PREFIX BUG ID 5859
                        const BEntityRef* pRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeAttachedToObject);
//--
                        if (pRef)
                           pUseUnit = gWorld->getUnit(pRef->mID);
                     }
                     else if (pUnit->isGarrisoned())
                     {
//-- FIXING PREFIX BUG ID 5860
                        const BEntityRef* pRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeContainingUnit);
//--
                        if (pRef)
                           pUseUnit = gWorld->getUnit(pRef->mID);
                     }
                     else if (pUnit->isHitched())
                     {
//-- FIXING PREFIX BUG ID 5861
                        const BEntityRef* pRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeHitchedToUnit);
//--
                        if (pRef)
                           pUseUnit = gWorld->getUnit(pRef->mID);
                     }
                     else
                        pUseUnit = pUnit;
                     if (pUseUnit)
                     {
                        float dist=avgPos.distanceSqr(pUseUnit->getPosition());
                        if (dist < closestDist)
                        {
                           pClosestUnit=pUseUnit;
                           closestDist=dist;
                        }
                     }
                  }
               }
            }
            if (pClosestUnit && pClosestUnit->getFlagAllowStickyCam())
            {
               mStickyHoverObject = pClosestUnit->getID();
               mStickyHoverPoint = avgPos;
               mStickyHoverObjectPos = pClosestUnit->getPosition();
               mFlagStickyReticleDoFollow=true;
               mGotoItemID = pClosestUnit->getID();
            }
         }
      }
   }

   for(long i=0; i<clusterCount; i++)
   {
      BSquadPtrArray* cluster=clusterList[i];
      delete cluster;
   }

   if( !gUIManager->isNonGameUIVisible() )
   {
#if defined( _VINCE_ )
      switch( findType )
      {
         case cFindCrowdMilitary:
            MVinceEventAsync_ControlUsed( this, "find_crowd_military" );
            break;

         case cFindCrowdIdleVillager:
            MVinceEventAsync_ControlUsed( this, "find_crowd_villager" );
            break;
      }
#endif
      switch( findType )
      {
         case cFindCrowdMilitary:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cControlFindCrowdMilitary, mPlayerID);
            break;

         case cFindCrowdIdleVillager:
            gGeneralEventManager.eventTrigger(BEventDefinitions::cControlFindCrowdVillager, mPlayerID);
            break;
      }
   }
   else
      gUI.playClickSound();

   return true;
}

//==============================================================================
// BUser::resetFindCrowd
//==============================================================================
void BUser::resetFindCrowd()
{
   mCrowdIndex=-1;
   mCrowdPos=cOriginVector;
}

//==============================================================================
// BUser::gotoAlert
//==============================================================================
void BUser::gotoAlert()
{
   resetGotoSelected();
   resetStickyReticle();

   const BAlert *pAlert = getPlayer()->getAlertManager()->getGotoAlert();
   if (pAlert)
   {
      BVector alertPosition = pAlert->getLocation();

      // Plant postion in camera height field
      computeClosestCameraHoverPointVertical(alertPosition, alertPosition);

      setGotoPosition(alertPosition, cGotoTypeAlert);
      gSoundManager.playCue( "play_ui_game_menu_select" );
   }
}

//==============================================================================
// BUser::gotoSelected
//==============================================================================
void BUser::gotoSelected()
{
   resetFindCrowd();
   resetCircleSelectionCycle();
   resetStickyReticle();

   if (mSelectionManager->getSubSelectGroupHandle() != -1)
   {
      BEntityIDArray squadList;
      mSelectionManager->getSubSelectedSquads(squadList);
      long numSquads=squadList.getNumber();
      if (numSquads==0)
      {
         resetGotoSelected();
         return;
      }

      gUI.playClickSound();

      mGotoSelectedIndex++;
      if(mGotoSelectedIndex>=numSquads)
         mGotoSelectedIndex=0;

//-- FIXING PREFIX BUG ID 5865
      const BSquad* pSquad = gWorld->getSquad(squadList[mGotoSelectedIndex]);
//--
      if (pSquad)
      {
         setFlagTeleportCamera(true);

         BVector pos=pSquad->getPosition()-(mpCamera->getCameraDir()*mCameraZoom);
         mpCamera->setCameraLoc(pos);
         tieToCameraRep();
         setFlagUpdateHoverPoint(true);
      }
   }
   else
   {
      long numSquads=mSelectionManager->getNumberSelectedSquads();
      if (numSquads==0)
      {
         resetGotoSelected();
         return;
      }

      gUI.playClickSound();

      mGotoSelectedIndex++;
      if(mGotoSelectedIndex>=numSquads)
         mGotoSelectedIndex=0;

//-- FIXING PREFIX BUG ID 5866
      const BSquad* pSquad = gWorld->getSquad(mSelectionManager->getSelectedSquad(mGotoSelectedIndex));
//--
      if (pSquad)
      {
         setFlagTeleportCamera(true);

         BVector pos=pSquad->getPosition()-(mpCamera->getCameraDir()*mCameraZoom);
         mpCamera->setCameraLoc(pos);
         tieToCameraRep();
         setFlagUpdateHoverPoint(true);
      }
   }

   MVinceEventAsync_ControlUsed( this, "goto_selected" );

   gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoAlert, mPlayerID);

}

//==============================================================================
// BUser::resetGotoSelected
//==============================================================================
void BUser::resetGotoSelected()
{
   mGotoSelectedIndex=-1;
}

//==============================================================================
// BUser::sendFlare
//==============================================================================
void BUser::sendFlare(int flareType, BVector pos)
{
   autoExitSubMode();

   if(mPlayerState!=BPlayer::cPlayerStatePlaying || !mFlagHoverPointOverTerrain)
   {
      gUI.playCantDoSound();
      return;
   }

   gUI.playClickSound();
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &mPlayerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeFlare);
      pCommand->setPosition(pos);
      pCommand->setData(flareType);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
      switch(flareType)
      {
         case BUIGame::cFlareLook: 
            MVinceEventAsync_ControlUsed( this, "flare" ); 
            gGeneralEventManager.eventTrigger(BEventDefinitions::cFlare, mPlayerID);
            break;
         case BUIGame::cFlareHelp: 
            MVinceEventAsync_ControlUsed( this, "flare_help" ); 
            gGeneralEventManager.eventTrigger(BEventDefinitions::cFlareHelp, mPlayerID);
            break;
         case BUIGame::cFlareMeet: 
            MVinceEventAsync_ControlUsed( this, "flare_meet" ); 
            gGeneralEventManager.eventTrigger(BEventDefinitions::cFlareMeet, mPlayerID);
            break;
         case BUIGame::cFlareAttack: 
            MVinceEventAsync_ControlUsed( this, "flare_attack" ); 
            gGeneralEventManager.eventTrigger(BEventDefinitions::cFlareAttack, mPlayerID);
            break;
      }

      gGeneralEventManager.eventTrigger(BEventDefinitions::cFlare, mPlayerID);

   }
}

//==============================================================================
// BUser::gotoItem
//==============================================================================
bool BUser::gotoItem(int gotoType, bool checkOnly, BEntityID* pReturnID)
{
//-- FIXING PREFIX BUG ID 5869
   const BPlayer* pPlayer=getPlayer();
//--

   if(!checkOnly)
   {
      resetFindCrowd();
      resetCircleSelectionCycle();
      resetStickyReticle();

      if(mGotoItemID!=cInvalidObjectID && !gWorld->getUnit(mGotoItemID))
         resetGotoCameraData();
   }

   float gotoBaseDistance=70.0f;
   gConfig.get(cConfigGotoBaseDistance, &gotoBaseDistance);

//-- FIXING PREFIX BUG ID 5870
   const BUnit* pPickedUnit=NULL;
//--
   bool pickNextUnit=false;

   if (gotoType==cGotoTypeBase)
   {
      if (mCoopPlayerID!=-1)
         pPlayer=getCoopPlayer();

      BSmallDynamicSimArray<uint> validBases;
      validBases.clear();
      int closestBase = -1;
      float closestDistance = -1.0f;

      int currentGotoBase = -1;
      uint count=pPlayer->getNumberGotoBases();
      for (uint i=0; i<count; i++)
      {
//-- FIXING PREFIX BUG ID 5867
         const BUnit* pUnit=gWorld->getUnit(pPlayer->getGotoBase(i));
//--
         if (!pUnit)
            continue;

         if(!pUnit->isAlive() || pUnit->getFlagDown())
            continue;

         if (!Math::IsValidFloat(pUnit->getPosition().x))
            continue;

         // Is unit selectable?
         if (pUnit->getSelectType(getTeamID()) == cSelectTypeNone)
            continue;

         if (pReturnID)
            *pReturnID = pUnit->getID();

         if(checkOnly)
            return true;

         if (mGotoItemID == pUnit->getID())
            currentGotoBase = i;

         validBases.add(i);
         if (getFlagHoverPointOverTerrain())
         {
            float distance = mHoverPoint.distanceEstimate(pUnit->getPosition());
            if (closestBase == -1 || (distance < closestDistance))
            {
               closestBase = i;
               closestDistance = distance;
            }
         }

#if 0 //old system
         if(pickNextUnit || mGotoItemID==cInvalidObjectID)
         {
            pPickedUnit=pUnit;
            break;
         }

         if(pPickedUnit==NULL)
            pPickedUnit=pUnit;

         if(pUnit->getID()==mGotoItemID)
            pickNextUnit=true;
#endif
      }
      if (validBases.getNumber() > 0)
      {
         //If we aren't close to a base or we are at the last base, choose the first valid base;
         int chooseValidBase = 0;
         
         // if we're on our way to a goto base, go to the base after that one
         int currValidBase = -1;
         if (currentGotoBase != -1)
            currValidBase = validBases.find(currentGotoBase);
         else if (closestDistance <= gotoBaseDistance)
            currValidBase = validBases.find(closestBase);

         // We're at or on our way to a base, so find the next one in the list
         if  ((currValidBase >= 0) && (currValidBase < (validBases.getNumber()-1)))  
            chooseValidBase = currValidBase + 1;

         BUnit* pUnit=gWorld->getUnit(pPlayer->getGotoBase(validBases.get(chooseValidBase)));
         BASSERT(pUnit); //we checked this before it went into the array
         pPickedUnit = pUnit;
         mBaseNumberDisplayTime = 1.0f;
         mBaseNumberToDisplay = pUnit->getBaseNumber();
      }
   }
   else
   {
//-- FIXING PREFIX BUG ID 5868
      const BSquad* pGotoSquad=NULL;
//--
      if (mGotoItemID != cInvalidObjectID)
      {
         BUnit* pUnit=gWorld->getUnit(mGotoItemID);
         if (pUnit)
            pGotoSquad=pUnit->getParentSquad();
      }

      //FXIME AJL 6/7/06 - Would be nice to have a list of units stored per player or even a list of
      // base buildings so that we don't have to iterate over all the units in the world
      BEntityHandle handle=cInvalidObjectID;
      for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
      {
         BPlayerID unitPlayerID=pUnit->getPlayerID();
         if(unitPlayerID!=mPlayerID && unitPlayerID!=mCoopPlayerID)
            continue;

         int unitGotoType=pUnit->getGotoType();
         if(unitGotoType!=gotoType)
            continue;

         if(!pUnit->isAlive() || pUnit->getFlagDown())
            continue;

         if (!Math::IsValidFloat(pUnit->getPosition().x))
            continue;

         // Is unit selectable?
         if (pUnit->getSelectType(getTeamID()) == cSelectTypeNone)
            continue;

         if (pReturnID)
            *pReturnID = pUnit->getID();

         if(checkOnly)
            return true;

         if (mGotoItemID==cInvalidObjectID)
         {
            pPickedUnit=pUnit;
            break;
         }

         if (pickNextUnit)
         {
            if (pGotoSquad==NULL || pUnit->getParentSquad()!=pGotoSquad)
            {
               pPickedUnit=pUnit;
               break;
            }
         }

         if(pPickedUnit==NULL)
            pPickedUnit=pUnit;

         if(pUnit->getID()==mGotoItemID || (pGotoSquad && pUnit->getParentSquad()==pGotoSquad))
            pickNextUnit=true;
      }
   }

   if(checkOnly)
      return false;

   if(pPickedUnit == NULL)
      return false;

   if (pReturnID)
      *pReturnID = pPickedUnit->getID();

   setGotoEntity(pPickedUnit->getID(), gotoType);

   // goto type specific code to run if we're not checking only
   if( !gUIManager->isNonGameUIVisible() )
   {
      BASSERTM(pPlayer, "Attempt to make a goto selection with a null player!");
      if (gotoType == cGotoTypeBase)
         gUIGame.playSound(BSoundManager::cSoundHotkeyBase, pPlayer->getCivID(), true);
      else if (gotoType == cGotoTypeNode)
         gUIGame.playSound(BSoundManager::cSoundHotkeyNode, pPlayer->getCivID(), true);
   }

   return(pPickedUnit!=NULL);
}

//==============================================================================
// BUser::setInterpData
//==============================================================================
void BUser::setInterpData(float time, const BVector& hoverPoint, const float* yaw, const float* pitch, const float* zoom)
{
   BASSERTM(time > 0.0f, "Set interp data must be called with a positive time value!");

   mInterpTimeLeft = time;
   mInterpHoverPoint = hoverPoint; 
   mInterpYaw = (yaw) ? *yaw : mCameraYaw;
   mInterpPitch = (pitch) ? *pitch : mCameraPitch;
   mInterpZoom = (zoom) ? *zoom : mCameraZoom;

   float yawDiff = (mInterpYaw - mCameraYaw);
   if (yawDiff > 180.0f)
      mInterpYaw -= 360.0f;
   else if (yawDiff < -180.0f)
      mInterpYaw += 360.0f;
}

//==============================================================================
// BUser::updateInterpCamera
//==============================================================================
void BUser::updateInterpCamera(float elapsedTime)
{
   if (elapsedTime >= mInterpTimeLeft)
   {
      mInterpTimeLeft = 0.0f;
      mHoverPoint = mInterpHoverPoint;
      mCameraYaw = mInterpYaw;
      mCameraPitch = mInterpPitch;
      mCameraZoom = mInterpZoom;
   }
   else
   {
      // this sucks, but for some reason the quaternion <-> yaw was returning strange results
      float targetYaw = mInterpYaw;
      float yawDiff = (mInterpYaw - mCameraYaw);
      if (yawDiff > 180.0f)
         targetYaw = mInterpYaw - 360.0f;
      else if (yawDiff < -180.0f)
         targetYaw = mInterpYaw + 360.0f;

      mHoverPoint += (mInterpHoverPoint - mHoverPoint) / mInterpTimeLeft * elapsedTime;
      mCameraYaw += (targetYaw - mCameraYaw) / mInterpTimeLeft * elapsedTime;
      mCameraPitch += (mInterpPitch - mCameraPitch) / mInterpTimeLeft * elapsedTime;
      mCameraZoom += (mInterpZoom - mCameraZoom) / mInterpTimeLeft * elapsedTime;
      mInterpTimeLeft -= elapsedTime;
   }

   mLastCameraHoverPoint = mCameraHoverPoint;
   mCameraHoverPoint = mHoverPoint;

   applyCameraSettings(true);
}

//==============================================================================
// BUser::updateGotoCamera
//==============================================================================
void BUser::updateGotoCamera(float elapsedTime)
{
   BVector desiredPosition;
//-- FIXING PREFIX BUG ID 5871
   const BUnit* pPickedUnit = gWorld->getUnit(mGotoItemID);
//--
   if (pPickedUnit)
      desiredPosition = pPickedUnit->getInterpolatedPosition();
   else if (mGotoType == cGotoTypeArmy || mGotoType == cGotoTypeAlert)
      desiredPosition = mGotoPosition;
   else
   {
      resetGotoCameraData();
      return;
   }

   // Goto item timeout
   mGotoItemTime-=elapsedTime;
   if(mGotoItemTime<=0.0f)
      mGotoItemTime=0.0f;

   BVector newLookAtPosition = desiredPosition;
   bool arrivedAtGotoPoint = true;
   if (mGotoItemTime > 0.0f)
   {
      float slideTime = 1.0f;
      gConfig.get(cConfigGotoSlideTime, &slideTime);
      float slideAwaySpeed = 10.0f;
      gConfig.get(cConfigGotoSlideAwaySpeed, &slideAwaySpeed);
      float slideTowardsDistance = 10.0f;
      gConfig.get(cConfigGotoSlideTowardsDistance, &slideTowardsDistance);

      float rampScale = 1.0f;
      float slideTowardsTime = 0.0f;
      bool hasSlideTowards = (slideTowardsDistance > cFloatCompareEpsilon);
      bool hasSlideAway = (slideAwaySpeed > cFloatCompareEpsilon);

      if (hasSlideTowards && hasSlideAway)
      {
         // ramp from 0.0 to 1.0 back to 0.0, slide towards at half time
         slideTowardsTime = slideTime * 0.5f;
         rampScale = slideTowardsTime - fabs(mGotoItemTime - slideTowardsTime);
         rampScale /= slideTowardsTime;
      }
      else if (hasSlideTowards && !hasSlideAway)
      {
         // ramp from 1.0 to 0.0, only slide towards
         slideTowardsTime = slideTime;
         rampScale = mGotoItemTime / slideTime;
      }
      else if (!hasSlideTowards && hasSlideAway)
      {
         // ramp from 0.0 to 1.0, never slide towards
         slideTowardsTime = 0.0f;
         rampScale = (slideTime - mGotoItemTime) / slideTime;
      }

      //BVector cameraLocWithoutZoom = mpCamera->getCameraLoc() + (mpCamera->getCameraDir() * mCameraZoom);
      BVector cameraLocWithoutZoom = mCameraHoverPoint;
      BVector directionToTarget = desiredPosition - cameraLocWithoutZoom;
      float currentDistanceToTarget = directionToTarget.length();
      if (directionToTarget.safeNormalize())
      {
         arrivedAtGotoPoint = false;

         if (mGotoItemTime >= slideTowardsTime)
         {
            // push with an average velocity using the ramp in the first half of the interp (away from current position)
            float velocityScale = rampScale * slideAwaySpeed;
            newLookAtPosition = cameraLocWithoutZoom + (directionToTarget * velocityScale * elapsedTime);
         }
         else 
         {
            // in the second half, don't use velocity, use a distance that scales down to ensure we end at the right place
            // ramp scale^2 gives a better curve
            float distanceScale = (slideTowardsDistance * rampScale * rampScale);
            if (currentDistanceToTarget > distanceScale)
               newLookAtPosition = desiredPosition - (directionToTarget * distanceScale);
            else
               newLookAtPosition = cameraLocWithoutZoom;
         }
      }
   }

   // Now that we have the position that the camera needs to look at, compute the corresponding 
   // camera hover point position, by intersecting a segment in the direction of the camera against 
   // the camera height field.
   //

   BVector closestCameraHoverPoint;
   computeClosestCameraHoverPointInDirection(newLookAtPosition, mpCamera->getCameraDir(), closestCameraHoverPoint);

   mCameraHoverPoint = closestCameraHoverPoint;

   // Now compute the new camera position
   //
   BVector pos = mCameraHoverPoint - (mpCamera->getCameraDir() * mCameraZoom);
   mpCamera->setCameraLoc(pos);


   setFlagTeleportCamera(true);

   //BVector pos = newLookAtPosition - (mpCamera->getCameraDir() * mCameraZoom);
   //mpCamera->setCameraLoc(pos);

   clampCamera();
   tieToCameraRep();

   if (!arrivedAtGotoPoint)
      return;

   // if we got to the object, do all the other stuff
   setFlagUpdateHoverPoint(true);

   // Sticky reticle follow
   if (getOption_CameraFollowEnabled() && pPickedUnit)
   {
      mStickyHoverObject = pPickedUnit->getID();
      mStickyHoverPoint = pPickedUnit->getInterpolatedPosition();
      mStickyHoverObjectPos = pPickedUnit->getInterpolatedPosition();
      mFlagStickyReticleDoFollow=true;
   }
   if( !gUIManager->isNonGameUIVisible() )
   {
      // don't do any selection update for bases or alerts
      if (mGotoType != cGotoTypeBase && mGotoType != cGotoTypeAlert)
      {
         if(mGotoType==cGotoTypeNode)
         {
            mSelectionManager->clearSelections();
            if(!gConfig.isDefined(cConfigBuildingMenuOnSelect))
               mSelectionManager->selectUnit(mGotoItemID);
         }
         else if (mGotoType != cGotoTypeArmy)
         {
            // we go in here if it's not army selection, because in army selection
            // the selection was already correctly set before the interp
            mSelectionManager->clearSelections();
            if (canSelectUnit(mGotoItemID, true))
            {
               mSelectionManager->selectUnit(mGotoItemID);
               playUnitSelectSound(mGotoItemID);
            }
         }
      }
   }
   else
      gUI.playClickSound();

   switch (mGotoType)
   {
   case cGotoTypeBase:
      if( !gUIManager->isNonGameUIVisible() )
      {
         MVinceEventAsync_ControlUsed( this, "goto_base" );
         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoBase, mPlayerID);
      }
      break;

   case cGotoTypeScout:
      if( !gUIManager->isNonGameUIVisible() )
      {
         MVinceEventAsync_ControlUsed( this, "goto_scout" );
         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoScout, mPlayerID);
      }
      break;

   case cGotoTypeNode:
      if( !gUIManager->isNonGameUIVisible() )
      {
         MVinceEventAsync_ControlUsed( this, "goto_node" );
         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoNode, mPlayerID);
      }
      break;

   case cGotoTypeHero:
      if( !gUIManager->isNonGameUIVisible() )
      {
         MVinceEventAsync_ControlUsed( this, "goto_hero" );
         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoHero, mPlayerID);
      }
      break;

   case cGotoTypeAlert:
      if( !gUIManager->isNonGameUIVisible() )
      {
         MVinceEventAsync_ControlUsed( this, "goto_alert" );
         gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoAlert, mPlayerID);
      }
   }

   resetGotoCameraData();
}

//==============================================================================
// BUser::setGotoPosition
//==============================================================================
void BUser::setGotoPosition(const BVector& position, int gotoType)
{
   if (position == mGotoPosition && mGotoType == gotoType)
      return;

   mGotoItemID = cInvalidObjectID;
   mGotoPosition = position;
   mGotoItemTime = 0.0f;
   gConfig.get(cConfigGotoSlideTime, &mGotoItemTime);
   mGotoType = gotoType;

   // safeguard against bad config screwing things up 
   if (mGotoItemTime < cFloatCompareEpsilon)
      mGotoItemTime = 0.01f;
}

//==============================================================================
// BUser::setGotoEntity
//==============================================================================
void BUser::setGotoEntity(const BEntityID& entity, int gotoType)
{
   if (mGotoItemID == entity && mGotoType == gotoType)
      return;

   mGotoItemID = entity;
   mGotoPosition = cOriginVector;
   mGotoItemTime = 0.0f;
   gConfig.get(cConfigGotoSlideTime, &mGotoItemTime);
   mGotoType = gotoType;

   // safeguard against bad config screwing things up 
   if (mGotoItemTime < cFloatCompareEpsilon)
      mGotoItemTime = 0.01f;
}

//==============================================================================
// BUser::resetGotoCameraData
//==============================================================================
void BUser::resetGotoCameraData()
{
   mGotoItemID=cInvalidObjectID;
   mGotoPosition=cOriginVector;
   mGotoType = -1;
   mGotoItemTime = 0.0f;
}

//==============================================================================
// BUser::setUserMessage
//==============================================================================
void BUser::setUserMessage( long index, BPlayerIDArray playerList, BSimUString* text, float xPos, float yPos, USER_MESSAGE_JUSTIFY justify, float point, float alpha, BColor color, bool enabled  )
{
   BASSERTM( ( index >= 0 ) && ( index < USER_MESSAGES_MAX ), "Index not within valid range!" );

   if (text->compare(L"NULL") != 0)
   {
      mUserMessages[index].text = *text;
   }

   uint playerCount = playerList.getSize();
   if (playerCount > 0)
   {
      // Reset player flags
      mUserMessages[index].players = 0;
   }
   for (uint i = 0; i < playerCount; i++)
   {
      mUserMessages[index].addPlayer(playerList[i]);
   }

   if( xPos >= 0.0f )
   {
      mUserMessages[index].xPos = xPos;
   }

   if( yPos >= 0.0f )
   {
      mUserMessages[index].yPos = yPos;
   }

   if( point >= 0.0f )
   {
      mUserMessages[index].scale = point / USER_MESSAGES_DEFAULT_POINT;
   }

   if( alpha >= 0.0f )
   {
      mUserMessages[index].alpha = alpha;      
   }

   if( color.r >= 0.0f )
   {
      mUserMessages[index].color = color;
   }

   mUserMessages[index].enabled = enabled;

   switch( justify )
   {      
      case cUserMessageJustify_Left:
         mUserMessages[index].justify = BFontManager2::cJustifyLeft;
         break;

      case cUserMessageJustify_Center:
         mUserMessages[index].justify = BFontManager2::cJustifyCenter;
         break;

      case cUserMessageJustify_Right:
         mUserMessages[index].justify = BFontManager2::cJustifyRight;
         break;
   }
}

//==============================================================================
// BUser::isAnyGroupAssigned
//==============================================================================
bool BUser::isAnyGroupAssigned() const
{
   for (uint i=0; i<cMaxGroups; i++)
   {
      if (mGroups[i].getNumber()>0)
         return true;
   }
   return false;
}

//===============================================
// Test a unit against the input UI mode filters
//===============================================
bool BUser::testUnitAgainstInputUIModeFilters(const BUnit* pUnit)
{
   // Check for a bad unit.
   if (!pUnit)
      return (false);

   // Check that the unit belongs to one of the allowed players.
   if (useInputUIModePlayerFilter())
   {
      if (mInputUIModePlayerFilter.find(pUnit->getPlayerID()) == cInvalidIndex)
         return (false);
   }

   // Check that the unit belongs to one of the allowed teams.
   if (useInputUIModeTeamFilter())
   {
      if (mInputUIModeTeamFilter.find(pUnit->getTeamID()) == cInvalidIndex)
         return (false);
   }

   // Check that the unit is one of the allowed object types.
   if (useInputUIModeObjectTypeFilter())
   {
      bool passesObjectTypeFilter = false;
      uint numObjectTypeFilters = (uint)mInputUIModeObjectTypeFilter.getNumber();
      for (uint i = 0; i < numObjectTypeFilters; i++)
      {
         if (pUnit->isType(mInputUIModeObjectTypeFilter[i]))
         {
            passesObjectTypeFilter = true;
            break;
         }
      }
      if (!passesObjectTypeFilter)
         return (false);
   }

   // Check that the unit is one of the allowed proto objects.
   if (useInputUIModeProtoObjectFilter())
   {
      bool passesProtoObjectFilter = false;
      uint numProtoObjectFilters = (uint)mInputUIModeProtoObjectFilter.getNumber();
      for (uint i = 0; i < numProtoObjectFilters; i++)
      {
         if (pUnit->isType(mInputUIModeProtoObjectFilter[i]))
         {
            passesProtoObjectFilter = true;
            break;
         }
      }
      if (!passesProtoObjectFilter)
         return (false);
   }

   // Check that unit matches one of the allowed relationships
   if (useInputUIModeRelationFilter())
   {
      bool passesRelationFilter = false;
      uint numRelationFilters = (uint)mInputUIModeRelationFilter.getNumber();
      for (uint i = 0; i < numRelationFilters; i++)
      {
         if (gWorld->getTeamRelationType(getTeamID(), pUnit->getTeamID()) == mInputUIModeRelationFilter[i])
         {
            passesRelationFilter = true;
            break;
         }
      }
      if (!passesRelationFilter)
      {
         return (false);
      }
   }

   // Guess it passes.
   return (true);
}

//==============================================================================
// Test a squad against the input UI mode filters
//==============================================================================
bool BUser::testSquadAgainstInputUIModeFilters(const BSquad* pSquad)
{
   // Check for a bad squad.
   if (!pSquad)
      return (false);

   // Check that the squad belongs to one of the allowed players.
   if (useInputUIModePlayerFilter())
   {
      if (mInputUIModePlayerFilter.find(pSquad->getPlayerID()) == cInvalidIndex)
         return (false);
   }

   // Check that the squad belongs to one of the allowed teams.
   if (useInputUIModeTeamFilter())
   {
      if (mInputUIModeTeamFilter.find(pSquad->getTeamID()) == cInvalidIndex)
         return (false);
   }

   // Check that the squad is one of the allowed proto squads.
   if (useInputUIModeProtoSquadFilter())
   {
      if (mInputUIModeProtoSquadFilter.find(pSquad->getProtoSquadID()) == cInvalidIndex)
         return (false);
   }

   // Check the squads child units against the filters.
   uint numSquadChildren = (uint)pSquad->getNumberChildren();
   for (uint i = 0; i < numSquadChildren; i++)
   {      
//-- FIXING PREFIX BUG ID 5872
      const BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(i));
//--
      if (!testUnitAgainstInputUIModeFilters(pChildUnit))
         return (false);
   }

   // Check that squad matches one of the allowed relationships
   if (useInputUIModeRelationFilter())
   {
      bool passesRelationFilter = false;
      uint numRelationFilters = (uint)mInputUIModeRelationFilter.getNumber();
      for (uint i = 0; i < numRelationFilters; i++)
      {
         if (gWorld->getTeamRelationType(getTeamID(), pSquad->getTeamID()) == mInputUIModeRelationFilter[i])
         {
            passesRelationFilter = true;
            break;
         }
      }
      if (!passesRelationFilter)
      {
         return (false);
      }
   }

   // Guess it passes.
   return (true);
}

//==============================================================================
// Test a squad list against the input UI mode entity filter set
//==============================================================================
BEntityIDArray BUser::testSquadListAgainstInputUIModeEntityFilterSet(const BEntityIDArray& squadList)
{
   BEntityIDArray passedSquads;
   BEntityIDArray failedSquads;
   passedSquads.clear();
   failedSquads.clear();
   if (mInputUIModeEntityFilterSet)
   {
      mInputUIModeEntityFilterSet->filterSquads(squadList, &passedSquads, &failedSquads, NULL);
   }
   else
   {
      passedSquads = squadList;
   }

   return (passedSquads);
}

//==============================================================================
// BUser::playUnitSelectSound
//==============================================================================
bool BUser::playUnitSelectSound(BEntityID id, bool suppressBankLoad)
{
   //-- Run through the units of the squad backwards to find a suitable unit to play the sound
   BUnit* pUnit=gWorld->getUnit(id);
   if(!pUnit)
      return false;

   BSquad* pSquad = pUnit->getParentSquad();
   if(!pSquad)
      return false;

   int numUnits = pSquad->getNumberChildren();
   for(int i=numUnits-1; i >= 0; i--)
   {
      BUnit* pTestUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pTestUnit && pTestUnit->isAlive() && !pTestUnit->isHibernating())
      {      
         if(!pTestUnit->getFlagDown())
         {
            pTestUnit->playUISound(cObjectSoundSelect, suppressBankLoad);         
         }
         else
         {
            pTestUnit->playUISound(cObjectSoundSelectDowned, suppressBankLoad);         
         }     
         gUI.playRumbleEvent(BRumbleEvent::cTypeUnitSelect);
         return true;
      }      
   }
   return false;
}

//==============================================================================
// BUser::playSelectedSounds
//==============================================================================
void BUser::playSelectedSounds()
{
   int numSquads=mSelectionManager->getNumberSelectedSquads();
   for (int i=0; i<numSquads; i++)
   {
      BEntityID squadID=mSelectionManager->getSelectedSquad(i);
//-- FIXING PREFIX BUG ID 5873
      const BSquad* pSquad=gWorld->getSquad(squadID);
//--
      if (pSquad)
      {
         playUnitSelectSound(pSquad->getLeader(), true);         
      }
   }
}

//==============================================================================
// BUser::notify
//==============================================================================
void BUser::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   switch(eventType)
   {
      // test for achievements
      case BEntity::cEventKilled:   // fixme - the sim needs needs to have a killed by player event.
         break;

      case BEntity::cEventBuilt:
      {
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeBuild, static_cast<long>(data), false);

         if (!isSignedIn())
            break;

         AccumulatorActionType actionType = eActionBuild;
         if (eventType == BEntity::cEventKilled)
            actionType = eActionKill;

         long dbid = -1;
         BPlayer* pPlayer = NULL;

         BEntity* pEntity = gWorld->getEntity(senderID);
         if (pEntity == NULL)
            break;
         switch (pEntity->getClassType())
         {
            case BEntity::cClassTypeSquad:
               {
//-- FIXING PREFIX BUG ID 5874
                  const BSquad* pSquad = reinterpret_cast<BSquad*>(pEntity);
//--
                  pPlayer = gWorld->getPlayer( pSquad->getPlayerID() );
                  const BProtoSquad* pProto = pSquad->getProtoSquad();
                  if (pProto != NULL)
                     dbid = pProto->getDBID();
               }
               break;
            case BEntity::cClassTypeUnit:
               {
                  BUnit* pUnit=reinterpret_cast<BUnit*>(pEntity);
                  pPlayer = pUnit->getPlayer();
                  const BProtoObject* pProto = pUnit->getProtoObject();
                  if (pProto)
                     dbid = pProto->getDBID();
               }
               break;
         }
         
         if (pPlayer->getUser() != this)
            break;

         if (dbid <= 0)
            break;

         BGameSettings* pSettings = gDatabase.getGameSettings();
         BASSERT(pSettings);
         gAchievementManager.updateAccumulator(this, dbid, actionType, pSettings, 1);

         break;
      }

      case BEntity::cEventSupportPowerAvailable:
      {
         long playerID=data;
         //uint supportPowerIndex=data2;
         addGameStateMessage(BUser::cGameStateMessageSupportPowerAvailable, playerID, 0, 0);
         setFlagPowerMenuRefreshTrainProgressOnly(true);
         refreshPowerMenu(playerID, -1);
         break;
      }

      case BEntity::cEventGameOver:
      {
         processGameOver();
         break;
      }

      case BEntity::cEventPlayerResigned:
      {
         long playerID=data;
         addGameStateMessage(BUser::cGameStateMessageResigned, playerID, 0, 0);
         if(playerID==mPlayerID)
         {
            mPlayerState=BPlayer::cPlayerStateResigned;
            handleEndGame();
         }
         else
         {
            const BPlayer* pPlayer=gWorld->getPlayer(playerID);
            //--
            if(pPlayer && (pPlayer->isHuman() || pPlayer->isComputerAI()))
            {
               gUIGame.playSound(BSoundManager::cSoundPlayerResigned);
               BUString message;
               if (pPlayer->getNetState() == BPlayer::cPlayerStateDisconnected)
                  message.locFormat(gDatabase.getLocStringFromID(24156), pPlayer->getLocalisedDisplayName().getPtr());//%S Disconnected
               else
                  message.locFormat(gDatabase.getLocStringFromID(22001), pPlayer->getLocalisedDisplayName().getPtr());//%S Resigned
               addFlashUserMessage(message, "", true, 2.0f, true);
            }
         }
         break;
      }

      case BEntity::cEventPlayerDefeated:
      {
         long playerID=data;
         //addGameStateMessage(BUser::cGameStateMessageDefeated, playerID, 0, 0);
         if(playerID==mPlayerID)
         {
            mPlayerState=BPlayer::cPlayerStateDefeated;
            handleEndGame();
         }
         else
         {
            //-- FIXING PREFIX BUG ID 5668
            const BPlayer* pPlayer=gWorld->getPlayer(playerID);
            //--
            if(pPlayer && (pPlayer->isHuman() || pPlayer->isComputerAI()))
            {
               gUIGame.playSound(BSoundManager::cSoundPlayerDefeated);
               BUString message;
               message.locFormat(gDatabase.getLocStringFromID(22003), pPlayer->getLocalisedDisplayName().getPtr());//%S Defeated
               addFlashUserMessage(message, "", true, 2.0f, true);
            }
         }
         break;
      }

      //case BEntity::cEventPlayerDisconnected:
      //{
      //   long playerID=data;
      //   addGameStateMessage(BUser::cGameStateMessageDisconnected, playerID, 0, 0);
      //   if(playerID==mPlayerID)
      //   {
      //      mPlayerState=BPlayer::cPlayerStateDisconnected;
      //      handleEndGame();
      //   }
      //   else
      //   {
      //      //-- FIXING PREFIX BUG ID 5669
      //      const BPlayer* pPlayer=gWorld->getPlayer(playerID);
      //      //--
      //      if(pPlayer)
      //      {
      //         gUIGame.playSound(BSoundManager::cSoundPlayerResigned);
      //         BUString message;
      //         message.locFormat(gDatabase.getLocStringFromID(24156), pPlayer->getLocalisedDisplayName().getPtr());//%S Disconnected
      //         addFlashUserMessage(message, "", true, 2.0f, true);
      //      }
      //   }
      //   break;
      //}

      case BEntity::cEventPlayerWon:
      {
         long playerID=data;
         if(playerID==mPlayerID)
         {
            addGameStateMessage(BUser::cGameStateMessageWon, playerID, 0, 0);
            mPlayerState=BPlayer::cPlayerStateWon;
            handleEndGame();
         }
         break;
      }

      case BEntity::cEventPlaybackDone:
         addGameStateMessage(BUser::cGameStatePlaybackDone, 0, 0, 0);
         break;

      case BEntity::cEventTrainSquadQueued:
      case BEntity::cEventTrainSquadPercent:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeTrainSquad, static_cast<long>(data), (eventType == BEntity::cEventTrainSquadPercent) ? true : false);
         break;

      case BEntity::cEventTrainQueued:
      case BEntity::cEventTrainPercent:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeTrainUnit, static_cast<long>(data), (eventType == BEntity::cEventTrainPercent) ? true : false);
         break;

      case BEntity::cEventTechQueued:
      case BEntity::cEventTechPercent:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeResearch, static_cast<long>(data), (eventType == BEntity::cEventTechPercent) ? true : false);
         break;

      case BEntity::cEventTechResearched:
         setFlagClearCircleMenuDisplay(true);
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeResearch, static_cast<long>(data), (eventType == BEntity::cEventTechPercent) ? true : false);
         break;

      case BEntity::cEventUnloaded:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeUnloadUnits, 0, false);
         break;

      case BEntity::cEventFullyHealed:
      case BEntity::cEventCapturePercent:
      case BEntity::cEventBuildPercent:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeBuild, static_cast<long>(data), true);
         break;

      case BEntity::cEventDeathReplacement:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeBuild, static_cast<long>(data), false);
         break;

      case BEntity::cEventPowerPercent:
         setFlagPowerMenuRefreshTrainProgressOnly(true);
         refreshPowerMenu(static_cast<long>(data2), static_cast<long>(data));
         // [5/8/2008 xemu] needed to refresh command menu as well since it is possible for power to be on the command menu (megaturret) 
         //refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeBuild, static_cast<long>(data), false);
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeBuild, static_cast<long>(data), true);
         break;

      case BEntity::cEventSelfDestructTime:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeCancelKill, 0, false);
         break;

      case BEntity::cEventTribute:
      {
         BPlayerID sendingPlayerID=data;
         BPlayerID receivingPlayerID=data2>>16;
         int resourceID=data2&0x0000FFFF;
         // addGameStateMessage(BUser::cGameStateMessageTributeReceived, receivingPlayerID, sendingPlayerID, resourceID);

         if(receivingPlayerID==mPlayerID)       // did this user receive the tribute?
         {
            const BPlayer* pFromPlayer=gWorld->getPlayer(sendingPlayerID); // who we received the $$ from
            if (pFromPlayer)
            {
               for (uint i=0; i<gUIGame.getNumberTributes(); i++)
               {
                  const BUIGameTribute* pTribute=gUIGame.getTribute(i);
                  if (pTribute->mResourceID==resourceID)
                  {
                     gUIGame.playSound(BSoundManager::cSoundTributeReceived);

                     BUString message;
                     message.locFormat(gDatabase.getLocStringFromIndex(pTribute->mReceivedStringIndex), pFromPlayer->getLocalisedDisplayName().getPtr());
                     addFlashUserMessage(message, "", true, 2.0f);
                     break;
                  }
               }
            }
         }

         break;
      }

      case BEntity::cEventCustomCommandQueued:
      case BEntity::cEventCustomCommandPercent:
      case BEntity::cEventCustomCommandResearched:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeCustomCommand, static_cast<long>(data), (eventType == BEntity::cEventCustomCommandPercent) ? true : false);
         break;

      case BEntity::cEventRepair:
      {
         if (mUserMode==BUser::cUserModePowerMenu && !mFlagRepairUnavail && mSelectionManager->isSquadSelected(senderID))
         {
            const BSquad* pSquad = gWorld->getSquad(senderID);
            if (pSquad)
            {
               long playerID = pSquad->getPlayerID();
               if (playerID == mPlayerID)
                  setFlagPowerMenuRefreshTrainProgressOnly(true);
                  refreshPowerMenu(playerID, gDatabase.getPPIDRepair());
            }
         }
         break;
      }
      case BEntity::cEventBuildingResource:
      {
         //int resourceID=data;
         BPlayerID playerID=data2;
         if (playerID == mPlayerID || playerID == mCoopPlayerID)
            refreshCommandMenu(playerID, -1, -1, -1, false);
         break;
      }
      case BEntity::cEventTrainLock:
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeTrainLock, -1, false);
         break;
      case BEntity::cEventSquadModeChanaged:
      {
         if (mSelectionManager->isUnitSelected(senderID))
            mSelectionManager->recomputeSubSelection();
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeTrainLock, -1, false);
         break;
      }
      case BEntity::cEventContainedUnit:
         mSelectionManager->unselectUnit(senderID);
         refreshCommandMenu(-1, senderID.asLong(), BProtoObjectCommand::cTypeTrainLock, -1, false);
         break;
      case BEntity::cEventRebuildTimer:
         refreshCommandMenu(-1, senderID.asLong(), -1, -1, false);
         break;
   }
}

//==============================================================================
// BUser::refreshCommandMenu
//==============================================================================
void BUser::refreshCommandMenu(long playerID, long unitID, long itemType, long itemID, bool bProgressUpdateOnly)
{
   if(getUserMode()==BUser::cUserModeCommandMenu && !getFlagCommandMenuRefresh())
   {
      if (playerID == -1 && unitID != -1)
      {
         const BEntity *pEntity = gWorld->getEntity(unitID);
         playerID = pEntity ? pEntity->getPlayerID() : -1;
      }

      long coopPlayerID = -1;
      if (gWorld->getFlagCoop() && gWorld->getPlayer(playerID))
         coopPlayerID=gWorld->getPlayer(playerID)->getCoopID();

      bool doRefresh = false;
      if ((playerID == mPlayerID) || (playerID == mCoopPlayerID) || (gWorld->getFlagCoop() && (coopPlayerID == mPlayerID)))
         doRefresh=true;
      else
      {
         const BUnit* pUnit=gWorld->getUnit(unitID);
         if (pUnit && pUnit->getProtoObject()->getFlagCommandableByAnyPlayer())
            doRefresh=true;
      }
      if (!doRefresh)
         return;

      bool update=false;
      if(unitID==-1)
         update=true;
      else if(unitID==getCommandObject().asLong())
         update=true;
      else if(itemType==BProtoObjectCommand::cTypeTrainUnit)
      {
         const BUnit* pUnit=gWorld->getUnit(unitID);
         if (pUnit && pUnit->isType(gDatabase.getOTIDBuildingSocket()))
            update=true;
      }
      else if(itemType==BProtoObjectCommand::cTypeResearch)
      {
//-- FIXING PREFIX BUG ID 5879
         const BProtoTech* pProtoTech=getPlayer()->getProtoTech(itemID);
//--
         if(pProtoTech && !pProtoTech->getFlagUnique())
            update=true;
      }
      if(update)
      {
         if( bProgressUpdateOnly )
            setFlagCommandMenuRefreshTrainProgressOnly(true);
         else
            setFlagCommandMenuRefresh(true);
      }
   }
}

//==============================================================================
// BUser::refreshPowerMenu
//==============================================================================
void BUser::refreshPowerMenu(long playerID, long protoPowerID)
{
   if(getUserMode()==BUser::cUserModePowerMenu && !getFlagPowerMenuRefresh())
      setFlagPowerMenuRefresh(true);
}

//==============================================================================
// BUser::addGameStateMessage
//==============================================================================
void BUser::addGameStateMessage(int messageType, BPlayerID playerID, int data, int data2)
{
   // Bail if a campaign game
   long gameType = 0;
   bool valid = gDatabase.getGameSettings()->getLong(BGameSettings::cGameType, gameType);
   if (valid && (gameType != BGameSettings::cGameTypeSkirmish))
   {
      return;
   }

   // Have a game over message immediately override any other messages in the queue
   if(messageType==cGameStateMessageWon)
   {
      mGameStateMessageList.clear();
      mGameStateMessageTimer=0.0f;
      setFlagGameStateMessage(false);
   }

   BUserGameStateMessage msg;
   msg.mType=messageType;
   msg.mPlayerID=playerID;
   msg.mData=data;
   msg.mData2=data2;
   mGameStateMessageList.add(msg);
}

//==============================================================================
// BUser::switchPlayer
//==============================================================================
void BUser::switchPlayer(long playerID)
{
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;

   mPlayerID = playerID;
   mCoopPlayerID = pPlayer->getCoopID();
   mTeamID = pPlayer->getTeamID();
   mPlayerState = pPlayer->getPlayerState();
   pPlayer->setUser(this);
   pPlayer->getAlertManager()->clearAlerts();

   //if (!gConfig.isDefined(cConfigFlashGameUI))
   //   gMiniMap.reset();
   //else
   gUIManager->resetMinimap();

   handlePlayerSwitchForUI();
}

//==============================================================================
// BUser::handlePlayerSwitch
//==============================================================================
void BUser::handlePlayerSwitchForUI()
{
// Halwes - 9/26/2008 - Switching the player for now is used in the advanced tutorial
//#ifndef BUILD_FINAL
   if (gArchiveManager.getArchivesEnabled())
   {
      gArchiveManager.reloadRootArchive();
      gArchiveManager.beginInGameUIPrefetch();
      gArchiveManager.beginInGameUILoad();
   }

   if (mpUIContext)
      mpUIContext->handlePlayerSwitch();

   if (gUIManager)
      gUIManager->handlePlayerSwitch();

   if (gArchiveManager.getArchivesEnabled())
   {
      gArchiveManager.endInGameUILoad();
      gArchiveManager.unloadRootArchive();
   }
//#endif 
}

//==============================================================================
// BUser::getGroupID
//==============================================================================
long BUser::getGroupID( long controlType, BInputEventDetail* pDetail )
{
   long             result          = -1;
   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
   if( pInputInterface->isFunctionControl( BInputInterface::cInputAssignGroup1, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputAssignGroup1 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
       pInputInterface->isFunctionControl( BInputInterface::cInputSelectGroup1, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputSelectGroup1 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
       pInputInterface->isFunctionControl( BInputInterface::cInputGotoGroup1, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputGotoGroup1 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) )
   {
      result = 0;
   }
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputAssignGroup2, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputAssignGroup2 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
            pInputInterface->isFunctionControl( BInputInterface::cInputSelectGroup2, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputSelectGroup2 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
            pInputInterface->isFunctionControl( BInputInterface::cInputGotoGroup2, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputGotoGroup2), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) )
   {
      result = 1;
   }
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputAssignGroup3, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputAssignGroup3 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
            pInputInterface->isFunctionControl( BInputInterface::cInputSelectGroup3, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputSelectGroup3 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
            pInputInterface->isFunctionControl( BInputInterface::cInputGotoGroup3, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputGotoGroup3 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) )
   {
      result = 2;
   }
   else if( pInputInterface->isFunctionControl( BInputInterface::cInputAssignGroup4, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputAssignGroup4 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
            pInputInterface->isFunctionControl( BInputInterface::cInputSelectGroup4, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputSelectGroup4 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) ||
            pInputInterface->isFunctionControl( BInputInterface::cInputGotoGroup4, controlType, pInputInterface->usesActionModifier( BInputInterface::cInputGotoGroup4 ), 0, NULL, false, false, false, false, -1.0f, -1.0f, pDetail ) )
   {
      result = 3;
   }

   return( result );
}

//==============================================================================
// BUser::selectGroup
//==============================================================================
void BUser::selectGroup( long controlType, BInputEventDetail* pDetail )
{
   long groupID = getGroupID( controlType, pDetail );
   if (groupID < 0 || groupID >= cMaxGroups)
      return;

   if( mGroups[groupID].getNumber() > 0 )
   {
      mSelectionManager->clearSelections();
      for( long i = 0; i < mGroups[groupID].getNumber(); i++ )
      {
//-- FIXING PREFIX BUG ID 5881
         const BSquad* pSquad = gWorld->getSquad( mGroups[groupID][i] );
//--
         if( pSquad )
         {
            for( uint j = 0; j < pSquad->getNumberChildren(); j++ )
            {
               BEntityID unitID = pSquad->getChild(j);
//-- FIXING PREFIX BUG ID 5880
               const BUnit* pUnit = gWorld->getUnit(unitID);
//--
               if (pUnit && !pUnit->isOutsidePlayableBounds())
               {
                  mSelectionManager->selectUnit(unitID);
                  //if(selected) //-- Swapping unit select sound for group sound. May change back later
                  //playUnitSelectSound(unitID);
               }
            }
         }
      }
      //Play the select group sound
      playSelectGroupSound( groupID );
   }

   #if defined( _VINCE_ )
      BSimString group;
      group.format( "select_group_%d", groupID + 1 );
      MVinceEventAsync_ControlUsed( this, group );
   #endif
   
   gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGroupSelect, mPlayerID);

}

//==============================================================================
// BUser::selectGroup
//==============================================================================
void BUser::gotoGroup( long controlType, BInputEventDetail* pDetail )
{
   long groupID = getGroupID( controlType, pDetail );
   if (groupID < 0 || groupID >= cMaxGroups)
      return;

   BVector center = cOriginVector;
   long    count  = 0;
   for( long i = 0; i < mGroups[groupID].getNumber(); i++ )
   {
//-- FIXING PREFIX BUG ID 5883
      const BSquad* pSquad = gWorld->getSquad( mGroups[groupID][i] );
//--
      if( pSquad )
      {
         for( uint j = 0; j < pSquad->getNumberChildren(); j++ )
         {
//-- FIXING PREFIX BUG ID 5882
            const BUnit* pUnit = gWorld->getUnit( pSquad->getChild( j ) );
//--
            if( pUnit )
            {
               center += pUnit->getPosition();
               count++;
            }
         }
      }
   }

   if( count > 0 )
   {
      resetStickyReticle();
      center /= (float)count;
      gTerrainSimRep.getCameraHeightRaycast( center, center.y, true );
      BVector pos = center - ( mpCamera->getCameraDir() * mCameraZoom );
      mpCamera->setCameraLoc( pos );
      setFlagUpdateHoverPoint(true );
      saveLastCameraLoc();
   }

   #if defined( _VINCE_ )
      BSimString group;
      group.format( "goto_group_%d", groupID + 1 );
      MVinceEventAsync_ControlUsed( this, group );
   #endif

   gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGroupGoto, mPlayerID);

}

//==============================================================================
// BUser::assignGroup
//==============================================================================
void BUser::assignGroup( long controlType, BInputEventDetail* pDetail )
{
   long groupID = getGroupID( controlType, pDetail );
   if (groupID < 0 || groupID >= cMaxGroups)
      return;

   mSelectionManager->getSubSelectedSquads(mGroups[groupID]);

   #if defined( _VINCE_ )
      BSimString group;
      group.format( "create_group_%d", groupID + 1 );
      MVinceEventAsync_ControlUsed( this, group );
   #endif

   if( mGroups[groupID].getNumber() > 0 )
   {
      playCreateGroupSound( groupID );
   }

   gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGroupAssign, mPlayerID);

}

//==============================================================================
// BUser::groupAdd
//==============================================================================
void BUser::groupAdd()
{
   uint count=mSelectionManager->getNumberSelectedSquads();
   if (mGroupIndex==-1 || count==0)
   {
      gUI.playCantDoSound();
      return;
   }

   gUI.playClickSound();

   for (uint i=0; i<count; i++)
   {
      BEntityID squadID=mSelectionManager->getSelectedSquad(i);
//-- FIXING PREFIX BUG ID 5884
      const BSquad* pSquad=gWorld->getSquad(squadID);
//--
      if (pSquad)
      {
         uint unitCount=pSquad->getNumberChildren();
         for (uint j=0; j<unitCount; j++)
         {
            if (mSelectionManager->selectUnit(pSquad->getChild(j)))
            {
               // Add to current group
               mGroups[mGroupIndex].uniqueAdd(squadID);

               // Remove from other groups
               for (int k=0; k<cMaxGroups; k++)
               {
                  if (k!=mGroupIndex)
                     mGroups[k].remove(squadID);
               }
               break;
            }
         }
      }
   }

   #ifdef _VINCE_
      MVinceEventAsync_ControlUsed(this, "group_add");
   #endif

   gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGroupAddTo, mPlayerID);

}

//==============================================================================
// BUser::groupNext
//==============================================================================
void BUser::groupNext()
{
   gUI.playClickSound();
   mGroupIndex++;
   if(mGroupIndex>=cMaxGroups)
      mGroupIndex=0;
   mSelectionManager->clearSelections();
   mSelectionManager->selectSquads(mGroups[mGroupIndex]);
   playSelectedSounds();
}

//==============================================================================
// BUser::groupPrev
//==============================================================================
void BUser::groupPrev()
{
   gUI.playClickSound();
   mGroupIndex--;
   if(mGroupIndex<0)
      mGroupIndex=cMaxGroups-1;
   mSelectionManager->clearSelections();
   mSelectionManager->selectSquads(mGroups[mGroupIndex]);
   playSelectedSounds();
}

//==============================================================================
// BUser::autoExitSubMode
//==============================================================================
void BUser::autoExitSubMode()
{
   if (mSubMode == cSubModeTargetSelect)
   {
      mTargetSelectObject = cInvalidObjectID;
      mTargetSelectSquad = cInvalidObjectID;
      mFlagTargetSelecting = false;
      setFlagUpdateHoverPoint(true);
      gInputSystem.leaveContext("TargetSelect");
   }
   mSubMode=cSubModeNone;
}

//==============================================================================
// BUser::resetDoubleClick
//==============================================================================
void BUser::resetDoubleClick(long controlType)
{
   long index = getDoubleClickFromControlType(controlType);
   if (index != -1)
   {
      mControllerKeyTimes[index].mDCTime = 0;
      mControllerKeyTimes[index].mHTime = 0;
   }
}

//==============================================================================
// BUser::playCreateGroupSound
//==============================================================================
void BUser::playCreateGroupSound(long groupID)
{
   switch(groupID)
   {
      case 0:
         gUIGame.playSound(BSoundManager::cSoundGroupCreate1, -1, true);
         break;
      case 1:
         gUIGame.playSound(BSoundManager::cSoundGroupCreate2, -1, true);
         break;
      case 2:
         gUIGame.playSound(BSoundManager::cSoundGroupCreate3, -1, true);
         break;
      case 3:
         gUIGame.playSound(BSoundManager::cSoundGroupCreate4, -1, true);
         break;
   }
}

//==============================================================================
// BUser::playSelectGroupSound
//==============================================================================
void BUser::playSelectGroupSound(long groupID)
{
   switch(groupID)
   {
      case 0:
         gUIGame.playSound(BSoundManager::cSoundGroupSelect1, -1, true);
            break;
      case 1:
         gUIGame.playSound(BSoundManager::cSoundGroupSelect2, -1, true);
            break;
      case 2:
         gUIGame.playSound(BSoundManager::cSoundGroupSelect3, -1, true);
            break;
      case 3:
         gUIGame.playSound(BSoundManager::cSoundGroupSelect4, -1, true);
            break;
   }
}

//==============================================================================
// BUser::drawSelectionCircleDecal
//==============================================================================
void BUser::drawSelectionCircleDecal(BVector pos, DWORD color, float size, float intensity, float offset)
{
   if (!gUIManager)
      return;

   if (mCircleSelectDecal == -1)
      return;

   int decalID = gUIGame.getCircleSelectDecalHandle();
   if (decalID == -1)
      return;

   int flashMovieIndex = gUIManager->getDecalFlashMovieIndex(decalID);
   if (flashMovieIndex == -1)
      return;

   //trace("FlashMovieIndex: %u", flashMovieIndex);
   
   BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mCircleSelectDecal);
   if (!pDecalAttributes)
      return;
   
   BVector forward = cZAxisVector;
   BVector right = cXAxisVector;   
   
   // multiple children use the squads position   
   pDecalAttributes->setForward(BVec3(forward.x,forward.y,forward.z));      
   pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
   pDecalAttributes->setEnabled(true);
   pDecalAttributes->setIntensity(intensity);
   pDecalAttributes->setRenderOneFrame(true);   
   pDecalAttributes->setColor(color);
   
   size *= 2.0f;
   pDecalAttributes->setSizeX(size);
   pDecalAttributes->setSizeZ(size);

   pDecalAttributes->setYOffset(offset);
   
   pDecalAttributes->setFlashMovieIndex(flashMovieIndex);
   pDecalAttributes->setConformToTerrain(true);      
   pDecalAttributes->setPos(BVec3(pos.x, pos.y, pos.z));
}

//==============================================================================
// BUser::drawHoverDecal
//==============================================================================
void BUser::drawHoverDecal(BVector pos, DWORD color, float size, float intensity, float offset)
{
   if (mHoverDecal == -1)
      return;

   BManagedTextureHandle texture =  gUIGame.getHoverDecalTextureHandle();

   BDecalAttribs* pDecalAttributes = gDecalManager.getDecal(mHoverDecal);
   if (!pDecalAttributes)
      return;
   
   BVector forward = cZAxisVector;
   BVector right = cXAxisVector;   
   
   // multiple children use the squads position   
   pDecalAttributes->setForward(BVec3(forward.x,forward.y,forward.z));      
   pDecalAttributes->setBlendMode(BDecalAttribs::cBlendOver);
   pDecalAttributes->setEnabled(true);
   pDecalAttributes->setIntensity(intensity);
   pDecalAttributes->setRenderOneFrame(true);   
   pDecalAttributes->setColor(color);
   
   size *= 2.0f;
   pDecalAttributes->setSizeX(size);
   pDecalAttributes->setSizeZ(size);

   pDecalAttributes->setYOffset(offset);
   
   pDecalAttributes->setTextureHandle(texture);
   pDecalAttributes->setConformToTerrain(true);      
   pDecalAttributes->setPos(BVec3(pos.x, pos.y, pos.z));
}

//==============================================================================
// BUser::readPrivilege
//==============================================================================
BOOL BUser::readPrivilege(XPRIVILEGE_TYPE priv) const
{
   if (mPort < 0 || mPort >= XUSER_MAX_COUNT)
      return FALSE;

   BOOL result;

   return ( XUserCheckPrivilege(mPort, priv, &result) == ERROR_SUCCESS ) && result;
}

//==============================================================================
// BUser::readPrivileges
//==============================================================================
void BUser::readPrivileges()
{
   mPrivMultiplayer           = readPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS);
   mPrivCommunications        = readPrivilege(XPRIVILEGE_COMMUNICATIONS);
   mPrivCommunicationsFriends = readPrivilege(XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY);
   mPrivProfileViewing        = readPrivilege(XPRIVILEGE_PROFILE_VIEWING);
   mPrivProfileViewingFriends = readPrivilege(XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY);
   mPrivUserContent           = readPrivilege(XPRIVILEGE_USER_CREATED_CONTENT);
   mPrivUserContentFriends    = readPrivilege(XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY);
   mPrivPurchaseContent       = readPrivilege(XPRIVILEGE_PURCHASE_CONTENT);
   mPrivPresence              = readPrivilege(XPRIVILEGE_PRESENCE);
   mPrivPresenceFriends       = readPrivilege(XPRIVILEGE_PRESENCE_FRIENDS_ONLY);
}

#ifndef BUILD_FINAL
//==============================================================================
// BUser::multiplayerCheatNotification
//==============================================================================
void BUser::multiplayerCheatNotification(long playerID, const char* pCheatType)
{
   if(gLiveSystem->isMultiplayerGameActive())
   {
      gUIGame.playSound(BSoundManager::cSoundFlare);
      mGameStateMessageText.locFormat(L"%S used %S cheat", gWorld->getPlayer(playerID)->getName().getPtr(), pCheatType);
      setFlagGameStateMessage(true);
      mGameStateMessageTimer=0.0f;
   }
}
#endif

//==============================================================================
// BUser::canSelectUnit
//==============================================================================
bool BUser::canSelectUnit(BEntityID unitID, bool ignoreCurrentSelection) const
{
   const BUnit* pUnit=gWorld->getUnit(unitID);
   if (!pUnit)
      return false;

   if (!pUnit->isAlive() || pUnit->getFlagDown())
      return false;

   if(pUnit->isGarrisoned() || pUnit->isAttached() || pUnit->isHitched())
      return false;

   int selectType = pUnit->getSelectType(getTeamID());
   if (selectType!=cSelectTypeUnit && selectType!=cSelectTypeCommand && selectType!=cSelectTypeSingleUnit && selectType!=cSelectTypeSingleType)
      return false;

   if (gConfig.isDefined(cConfigBuildingMenuOnSelect))
   {
      if (selectType==cSelectTypeCommand)
         return false;
   }

   if (!ignoreCurrentSelection && mSelectionManager->getNumberSelectedUnits()>0)
   {
      const BEntityIDArray& selList=mSelectionManager->getSelectedUnits();
      const BUnit* pSelectedUnit = gWorld->getUnit(selList[0]);
      if (pSelectedUnit && pSelectedUnit->getPlayerID()!=pUnit->getPlayerID())
         return false;
   }

   return true;
}

//==============================================================================
// Set an objective arrows parameters
//==============================================================================
void BUser::setObjectiveArrow(int index, BVector origin, BVector target, bool visible, bool useTarget, bool forceTargetVisible /*= false*/)
{
   if ((index < 0) || (index >= gDatabase.getObjectiveArrowMaxIndex()))   
   {
      BASSERTM(false, "Index out of range!");
      return;
   }

   BObjectiveArrow* pObjectiveArrow = mObjectiveArrowList[index];
   if (pObjectiveArrow)
   {
      if (mPlayerID != pObjectiveArrow->getPlayerID())
      {
         pObjectiveArrow->changeOwner(mPlayerID);
      }
      pObjectiveArrow->setOrigin(origin);
      pObjectiveArrow->setTarget(target);
      pObjectiveArrow->setFlagVisible(visible);
      pObjectiveArrow->setFlagUseTarget(useTarget);
      pObjectiveArrow->setFlagForceTargetVisible(forceTargetVisible);
   }
   else
   {
      pObjectiveArrow = new BObjectiveArrow(mPlayerID, origin, target, visible, useTarget, forceTargetVisible);      
      BASSERT(pObjectiveArrow);
      mObjectiveArrowList[index] = pObjectiveArrow;
   }   
}

//==============================================================================
// Update the objective arrows
//==============================================================================
void BUser::updateObjectiveArrows()
{
   uint numObjectiveArrows = mObjectiveArrowList.getSize();
   for (uint i = 0; i < numObjectiveArrows; i++)
   {
      BObjectiveArrow* pPObjectiveArrow = mObjectiveArrowList[i];
      if (pPObjectiveArrow)
      {
         pPObjectiveArrow->update(this);
      }
   }
}


//============================================================================
// OPTIONS
//============================================================================
// [PChapman - 6/18/08]
// The DEFINE_OPTION macro is a bit silly.  I had to implement a bunch of simple
// "get" and "set" methods that assert the existence of the profile and then 
// pass the call along.  For the "set" functions, I also needed to, optionally,
// be able to do something else.  So, the DEFINE_OPTION macro fully implements
// the "get" method and then _mostly_ implements the "set" method.  The closing
// '}' is omitted, hence the trailing braces after each DEFINE_MACRO line.  So,
// if I need to do something extra, I can do it on a new line and then close the
// function with a '}'.
// 
// Silly, yes, but it saves about 200 lines of code in this file which is already,
// at the time of writing, 19,247 lines long.

#define DEFINE_OPTION( varType, optionName )\
varType BUser::getOption_##optionName( void ) const\
{\
   BASSERT( mpProfile );\
   return mpProfile->getOption_##optionName();\
}\
\
void BUser::setOption_##optionName( varType v )\
{\
   BASSERT( mpProfile );\
   mpProfile->setOption_##optionName( v );\

DEFINE_OPTION(bool,  AIAdvisorEnabled )}
DEFINE_OPTION(uint8, DefaultAISettings )}
DEFINE_OPTION(uint8, DefaultCampaignDifficulty)
   BCampaign* pCampaign = gCampaignManager.getCampaign(0);
   if( pCampaign )
   {
      pCampaign->setCurrentDifficulty( v );
   }
}

DEFINE_OPTION(uint8, ControlScheme )}

DEFINE_OPTION(bool,  RumbleEnabled )
   gInputSystem.getGamepad( getPort() ).enableRumble( v );
}

DEFINE_OPTION(bool,  CameraRotationEnabled )
   if( !v )
   {
      mCameraYaw = mCameraDefaultYaw;
      applyCameraSettings(true);
   }
}

DEFINE_OPTION(bool,  XInverted )}
DEFINE_OPTION(bool,  YInverted )}
DEFINE_OPTION(uint8, DefaultZoomLevel )}
DEFINE_OPTION(uint8, StickyCrosshairSensitivity )}
DEFINE_OPTION(uint8, ScrollSpeed )}
DEFINE_OPTION(bool,  CameraFollowEnabled )}
DEFINE_OPTION(bool,  HintsEnabled )}
DEFINE_OPTION(uint8, SelectionSpeed )}

DEFINE_OPTION(uint8, MusicVolume )
   v = uint8(v / 10.0f * 255);
   gSoundManager.setVolumeMusic(v);
}
DEFINE_OPTION(uint8, SFXVolume )
   // scale the bink volume to max out at the default
   uint8 defaultVol = 10;
   getOptionDefaultByName( "SFXVolume", defaultVol );
   float binkVolPct = (Math::Min(defaultVol, v) / ((float)defaultVol));

   v = uint8(v / 10.0f * 255);
   gSoundManager.setVolumeSFX(v);

   gBinkInterface.setVolume(uint(binkVolPct * BBinkInterface::cMAXIMUM_VOLUME));
}

DEFINE_OPTION(uint8, VoiceVolume )
   v = uint8(v / 10.0f * 255);
   gSoundManager.setVolumeVoice(v);
}
DEFINE_OPTION(bool,  SubtitlesEnabled )}
DEFINE_OPTION(bool,  ChatTextEnabled )}

DEFINE_OPTION(uint8, Brightness )
   uint8 minVal, maxVal;
   getOptionRangeByName( "Brightness", minVal, maxVal );
   v = minVal + maxVal - v;
   gGammaRamp.set( v / 100.0f, gGammaRamp.getContrast() );  
}

DEFINE_OPTION(bool,  FriendOrFoeColorsEnabled )}

DEFINE_OPTION(bool,  MiniMapZoomEnabled )
   if( gUIManager )
      gUIManager->setMinimapFullZoomOut( !v );
}

//============================================================================
// BEGIN E3 Options - Remove after E3
//============================================================================
bool BUser::getOption_ShowButtonHelp( void ) const { return mShowButtonHelp; }

void BUser::setOption_ShowButtonHelp( bool v )
{
   /*
   if( gConfig.isDefined( cConfigDemo ) )
   {
      mShowButtonHelp = v;
      if( gUIManager )
         gUIManager->getWidgetUI()->showButtonHelp( mShowButtonHelp );
   }
   */
}

bool BUser::getOption_ResourceBoost( void ) const { return false; }

void BUser::setOption_ResourceBoost( bool v )
{
   if( gWorld && v )
   {
      long civID=gWorld->getPlayer(mPlayerID)->getCivID();
      const BUIGamePlayerStat* pStat=gUIGame.getPlayerStat(civID, 2);

      BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
      if(pCommand)
      {
         pCommand->setSenderType(BCommand::cPlayer);
         pCommand->setSenders(1, &mPlayerID);
         pCommand->setRecipientType(BCommand::cGame);
         pCommand->setType(BGameCommand::cTypeAddResources);
         pCommand->setData(pStat->mID);
         gWorld->getCommandManager()->addCommandToExecute(pCommand);
         //gConsoleOutput.status("Adding resources to PlayerID %u", mPlayerID);
      }
   }
}

bool BUser::getOption_FogOfWarEnabled( void ) const
{
   return !gConfig.isDefined(cConfigNoFogMask);
}

void BUser::setOption_FogOfWarEnabled( bool v )
{
   if( gConfig.isDefined(cConfigNoFogMask) != v )
      return;

   if( gWorld )
   {
      BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
      if(pCommand)
      {
         pCommand->setSenderType(BCommand::cPlayer);
         pCommand->setSenders(1, &mPlayerID);
         pCommand->setRecipientType(BCommand::cGame);
         pCommand->setType(BGameCommand::cTypeFogOfWar);
         pCommand->setData(v?1:0);
         gWorld->getCommandManager()->addCommandToExecute(pCommand);
      }
   }
   //if (!gLiveSystem->isMultiplayerGameActive())
   {
      if( !v )
         gConfig.define(cConfigNoFogMask);
      else
         gConfig.remove(cConfigNoFogMask);

      //gConsoleOutput.status("Fog of war %s", gConfig.isDefined(cConfigNoFogMask) ? "Disabled" : "Enabled");
   }
}

//============================================================================
// END E3 Options - Remove after E3
//============================================================================

//============================================================================
// BUser::getOptionByName
//============================================================================
bool BUser::getOptionByName( const char* optionName, uint8& optionValue ) const
{
   Uint8OptionMap::const_iterator it = mUint8OptionMap.find( optionName );

   if( it != mUint8OptionMap.end() )
   {
      optionValue = (this->*(it->second.get))();
      return true;
   }

   return false;
}

//============================================================================
// BUser::setOptionByName
//============================================================================
bool BUser::setOptionByName( const char* optionName, uint8 optionValue )
{
   Uint8OptionMap::const_iterator it = mUint8OptionMap.find( optionName );

   if( it != mUint8OptionMap.end() )
   {
      (this->*(it->second.set))( optionValue );
      return true;
   }

   return false;
}

//============================================================================
// BUser::getOptionByName
//============================================================================
bool BUser::getOptionByName( const char* optionName, bool& optionValue ) const
{
   BoolOptionMap::const_iterator it = mBoolOptionMap.find( optionName );

   if( it != mBoolOptionMap.end() )
   {
      optionValue = (this->*(it->second.get))();
      return true;
   }

   return false;
}

//============================================================================
// BUser::setOptionByName
//============================================================================
bool BUser::setOptionByName( const char* optionName, bool optionValue )
{
   BoolOptionMap::const_iterator it = mBoolOptionMap.find( optionName );

   if( it != mBoolOptionMap.end() )
   {
      (this->*(it->second.set))( optionValue );
      return true;
   }

   return false;
}

//============================================================================
// BUser::getOptionRangeByName
//============================================================================
bool BUser::getOptionRangeByName( const char* optionName, uint8& optionMin, uint8& optionMax )
{
   Uint8OptionMap::const_iterator it = mUint8OptionMap.find( optionName );

   if( it != mUint8OptionMap.end() )
   {
      optionMin = it->second.minValue;
      optionMax = it->second.maxValue;
      return true;
   }

   return false;
}

//============================================================================
// BUser::getOptionDefaultByName
//============================================================================
bool BUser::getOptionDefaultByName( const char* optionName, uint8& optionDef )
{
   Uint8OptionMap::const_iterator it = mUint8OptionMap.find( optionName );

   if( it != mUint8OptionMap.end() )
   {
      optionDef = it->second.defaultValue;
      return true;
   }

   return false;
}

//============================================================================
// BUser::resetOptionByName
//============================================================================
bool BUser::resetOptionByName( const char* optionName )
{
   BoolOptionMap::const_iterator itBool = mBoolOptionMap.find( optionName );

   if( itBool != mBoolOptionMap.end() )
   {
      if( stricmp( optionName, "rumbleEnabled" ) )
         (this->*(itBool->second.set))( itBool->second.defaultValue );
      else
      {
         XUSER_PROFILE_SETTING* rumbleSetting = getProfile()->getProfileSetting(Setting_Controller_Vibration);
         if( rumbleSetting )
            (this->*(itBool->second.set))( !!rumbleSetting->data.nData );
         else
            (this->*(itBool->second.set))( itBool->second.defaultValue );
      }
      return true;
   }
   else
   {
      Uint8OptionMap::const_iterator itUint8 = mUint8OptionMap.find( optionName );

      if( itUint8 != mUint8OptionMap.end() )
      {
         (this->*(itUint8->second.set))( itUint8->second.defaultValue );
         return true;
      }
   }

   return false;
}

//============================================================================
// BUser::resetAllOptions
//============================================================================
void BUser::resetAllOptions( void )
{
   for( BoolOptionMap::const_iterator itBool = mBoolOptionMap.begin(); itBool != mBoolOptionMap.end(); ++itBool )
   {
      (this->*(itBool->second.set))( itBool->second.defaultValue );
   }

   XUSER_PROFILE_SETTING* rumbleSetting = getProfile()->getProfileSetting(Setting_Controller_Vibration);
   if( rumbleSetting )
      setOption_RumbleEnabled( !!rumbleSetting->data.nData );

   for( Uint8OptionMap::const_iterator itUint8 = mUint8OptionMap.begin(); itUint8 != mUint8OptionMap.end(); ++itUint8 )
   {
      (this->*(itUint8->second.set))( itUint8->second.defaultValue );
   }
}

//============================================================================
// BUser::applyAllOptions
//============================================================================
void BUser::applyAllOptions( void )
{
   for( BoolOptionMap::const_iterator itBool = mBoolOptionMap.begin(); itBool != mBoolOptionMap.end(); ++itBool )
   {
      (this->*(itBool->second.set))( (this->*(itBool->second.get))() );
   }

   for( Uint8OptionMap::const_iterator itUint8 = mUint8OptionMap.begin(); itUint8 != mUint8OptionMap.end(); ++itUint8 )
   {
      (this->*(itUint8->second.set))( (this->*(itUint8->second.get))() );
   }
}

//============================================================================
// BUser::validateUintOptions
//============================================================================
void BUser::validateUintOptions( void )
{
   for( Uint8OptionMap::const_iterator itUint8 = mUint8OptionMap.begin(); itUint8 != mUint8OptionMap.end(); ++itUint8 )
   {
      uint8 curVal = (this->*(itUint8->second.get))();
      uint8 minVal = min( itUint8->second.minValue, itUint8->second.maxValue );
      uint8 maxVal = max( itUint8->second.minValue, itUint8->second.maxValue );
      if( curVal < minVal || curVal > maxVal )
      {
//          BString msg;
//          msg.format( "OPTION VALIDATION FAILED!  %s == %d which is outside the valid range ( %d - %d ). Let PChapman know.", itUint8->first.getPtr(), curVal, minVal, maxVal ); 
//          BASSERTM( false, msg.getPtr() );
         (this->*(itUint8->second.set))( itUint8->second.defaultValue );
      }
   }
}


//============================================================================
// BUser::initOptionMaps
//============================================================================
// [PChapman - 6/18/08]
// This static function is called ONCE to initialize the static option maps
// in BUser. The INIT_OPTION macro declares an instance of one of the Option
// structs, and initializes it with the appropriate get/set methods and a 
// default value.  It then adds it to the appropriate map based on the type of
// the option.
void BUser::initOptionMaps( void )
{
#define INIT_BOOL_OPTION( name, def )\
   SBoolOption s##name;\
   s##name.get = &BUser::getOption_##name;\
   s##name.set = &BUser::setOption_##name;\
   s##name.defaultValue = def;\
   mBoolOptionMap[#name] = s##name;

#define INIT_UINT_OPTION( name, min, def, max )\
   SUint8Option s##name;\
   s##name.get = &BUser::getOption_##name;\
   s##name.set = &BUser::setOption_##name;\
   s##name.minValue = min;\
   s##name.defaultValue = def;\
   s##name.maxValue = max;\
   mUint8OptionMap[#name] = s##name;
   //BASSERT( min <= def && def <= max );\
//   mUint8OptionMap[#name] = s##name;
      
   // Even though this function should only be called once, clearing the
   // maps here makes it safe to call multiple times.  Don't, though.
   mBoolOptionMap.clear();
   mUint8OptionMap.clear();

   INIT_BOOL_OPTION( AIAdvisorEnabled, true );
   INIT_UINT_OPTION( DefaultAISettings, 0, DifficultyType::cAutomatic, 5 );
   INIT_UINT_OPTION( DefaultCampaignDifficulty, 0, DifficultyType::cNormal, 5 );
   long controlScheme = BUserProfile::eControlScheme1;
   BSimString controllerConfigName;
   if (gConfig.get(cConfigControllerConfig, controllerConfigName))
   {
      long index = gInputSystem.getControllerConfigIndex(controllerConfigName);
      if (index != -1)
         controlScheme = (uint8)index;
   }

   INIT_UINT_OPTION( ControlScheme, 0, (uint8)controlScheme, (uint8)gInputSystem.getInputInterfaces()->getNumber() );

   INIT_BOOL_OPTION( RumbleEnabled, true );
   INIT_BOOL_OPTION( CameraRotationEnabled, true );
   INIT_BOOL_OPTION( XInverted, false );
   INIT_BOOL_OPTION( YInverted, false );

   long defaultZoom = 85;
   gConfig.get( cConfigCameraZoom, &defaultZoom );
   INIT_UINT_OPTION( DefaultZoomLevel, 60, (uint8)defaultZoom, 115 );

   float stickyReticleSensitivity = 0.0f;
   gConfig.get(cConfigStickyReticleSensitivity, &stickyReticleSensitivity);
   INIT_UINT_OPTION( StickyCrosshairSensitivity, 0, (uint8)(stickyReticleSensitivity * 100), 166 );

   INIT_UINT_OPTION( ScrollSpeed, 0, 10, 15 );

   INIT_BOOL_OPTION( CameraFollowEnabled, gConfig.isDefined(cConfigStickyReticleFollow) );

   INIT_BOOL_OPTION( HintsEnabled, true );
   INIT_UINT_OPTION( SelectionSpeed, 1, 1, 2 );

   INIT_UINT_OPTION( MusicVolume, 0, 8, 10);
   INIT_UINT_OPTION( SFXVolume, 0, 8, 10);
   INIT_UINT_OPTION( VoiceVolume, 0, 8, 10);

   INIT_BOOL_OPTION( SubtitlesEnabled, gConfig.isDefined(cConfigSubTitles) );
   INIT_BOOL_OPTION( ChatTextEnabled, gConfig.isDefined(cConfigSubTitles) );

   INIT_UINT_OPTION( Brightness, 75, 130, 150 );
   INIT_BOOL_OPTION( FriendOrFoeColorsEnabled, false );
   INIT_BOOL_OPTION( MiniMapZoomEnabled, false );

   //============================================================================
   // BEGIN E3 Options - Remove after E3
   //============================================================================
   INIT_BOOL_OPTION( ShowButtonHelp, true );
   INIT_BOOL_OPTION( ResourceBoost, false );
   INIT_BOOL_OPTION( FogOfWarEnabled, true );
   //============================================================================
   // END E3 Options - Remove after E3
   //============================================================================

#undef INIT_BOOL_OPTION
#undef INIT_UINT_OPTION
}
//============================================================================
//============================================================================


//==============================================================================
// BUser::invokePower
//==============================================================================
bool BUser::invokePower(int protoPowerID, BEntityID squadID, bool noCost /*= false*/, bool overridePowerLevel /*= false*/, BPowerLevel overrideLevel /*= 0*/)
{
   BPlayer* pPlayer = getPlayer();
//-- FIXING PREFIX BUG ID 5888
   const BPowerEntry* pEntry = pPlayer->findPowerEntry(protoPowerID);
//--
   if (!pEntry)
      return false;

//-- FIXING PREFIX BUG ID 5889
   const BProtoPower* pPP = gDatabase.getProtoPowerByID(protoPowerID);
//--
   if (!pPP)
      return false;

   if (!noCost && !pPlayer->canUsePower(protoPowerID))
      return false;

   BPowerLevel powerLevel = overrideLevel ? overrideLevel : pPlayer->getPowerLevel(protoPowerID);

   if (pPP->getPowerType() == PowerType::cInvalid)
   {
      BPowerCommand *c = (BPowerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandPower);
      if (!c)
         return false;
      c->setSenders(1, &mPlayerID);
      c->setSenderType(BCommand::cPlayer);
      c->setRecipientType(BCommand::cPlayer);
      c->setType(BPowerCommand::cTypeInvokePower);
      c->setProtoPowerID(protoPowerID);
      c->setPowerLevel(powerLevel);
      c->setSquadID(squadID);
      c->setAbilitySquads(mSelectionManager->getSelectedSquads());
      gWorld->getCommandManager()->addCommandToExecute(c);
      setUIProtoPowerID(protoPowerID);
      setUIPowerRadius(pPP->getUIRadius());
      changeMode(cUserModeInputUILocation);
   }
   else
   {
      BASSERTM(!mpPowerUser, "Trying to invoke a power (create a BPowerUser) when BUser already has one!  Error!");
      if (!mpPowerUser)
         mpPowerUser = createPowerUser(protoPowerID, powerLevel, squadID, noCost);
   }

   return (true);
}


//==============================================================================
// BUser::invokePower2
//==============================================================================
bool BUser::invokePower2(BProtoPowerID protoPowerID)
{
   BPlayer* pPlayer = getPlayer();
//-- FIXING PREFIX BUG ID 5890
   const BPowerEntry* pEntry = pPlayer->findPowerEntry(protoPowerID);
//--
   if (!pEntry)
      return false;

//-- FIXING PREFIX BUG ID 5891
   const BProtoPower* pPP = gDatabase.getProtoPowerByID(pEntry->mProtoPowerID);
//--
   if (!pPP)
      return false;

   if (!pPlayer->canUsePower(pEntry->mProtoPowerID))
      return false;

   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);

   BPowerCommand *c = (BPowerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandPower);
   if (!c)
      return false;
   c->setSenders(1, &mPlayerID);
   c->setSenderType(BCommand::cPlayer);
   c->setRecipientType(BCommand::cPlayer);
   c->setType(BPowerCommand::cTypeInvokePower);
   c->setProtoPowerID(protoPowerID);
   c->setPowerLevel(powerLevel);
   //c->setSquadID(squadID);
   c->setAbilitySquads(mSelectionManager->getSelectedSquads());
   gWorld->getCommandManager()->addCommandToExecute(c);
   setUIProtoPowerID(protoPowerID);
   setUIPowerRadius(pPP->getUIRadius());
   changeMode(cUserModeNormal);

   return true;
}

//==============================================================================
// BUser::doTribute
//==============================================================================
bool BUser::doTribute(BProtoObjectCommand& command)
{
   BPlayer* pPlayer = getPlayer();
//-- FIXING PREFIX BUG ID 5892
   const BTeam* pTeam=pPlayer->getTeam();
//--
   int playerIndex=command.getPosition();
   int resourceID=command.getID();
   if (pPlayer->getResource(resourceID)<gDatabase.getTributeAmount())
   {
      gUI.playCantDoSound();
      return false;
   }
   gUI.playClickSound();
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &mPlayerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeTribute);
      pCommand->setData(pTeam->getPlayerID(playerIndex));
      pCommand->setData2(resourceID);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BUser::doReverseHotDrop(BProtoObjectCommand& command)
{
   // Check that leader and hot drop pad are both available
   BUnit* pHotDropUnit = gWorld->getUnit(mCommandObject);
   BEntityID leaderUnitID = cInvalidObjectID;
   bool avail = gotoItem(cGotoTypeHero, true, &leaderUnitID);
   BPlayer* pPlayer = getPlayer();
   BASSERT(pPlayer && pPlayer->getLeader());
   if (!pHotDropUnit || !pHotDropUnit->isAlive() || pHotDropUnit->getFlagDown() || !avail || 
       !pPlayer->checkCost(pPlayer->getLeader()->getReverseHotDropCost()) ||
       gFatalityManager.getFatality(leaderUnitID))
   {
      gUI.playCantDoSound();
      return false;
   }

   gUI.playClickSound();
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &mPlayerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeReverseHotDrop);
      pCommand->setData(mCommandObject);
      pCommand->setData2(leaderUnitID);
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
   return true;
}

//==============================================================================
// BUser::doRepair
//==============================================================================
void BUser::doRepair()
{
   BEntityIDArray ugIDs;
   mSelectionManager->getSubSelectedSquads(ugIDs);

   BCost totalCost;
   BEntityIDArray squads;
   int squadCount=ugIDs.getNumber();
   if (squadCount>0)
   {
      BCost squadCost;
      for (int i=0; i<squadCount; i++)
      {
         const BSquad* pSquad=gWorld->getSquad(ugIDs[i]);
         if (pSquad && pSquad->getPlayerID()==mPlayerID && pSquad->getProtoSquad()->getFlagRepairable() && !pSquad->getFlagIsRepairing())
         {
            float temp;
            if (pSquad->getRepairCost(squadCost, temp))
            {
               squads.add(pSquad->getID());
               totalCost.add(&squadCost);
            }
         }
      }
   }
   bool passedCost = getPlayer()->checkCost(&totalCost);
   if ((squads.getSize() == 0) || !passedCost)
   {
      if (!passedCost)
      {
         BSimHelper::playSPCCostErrorSound(mPlayerID, &totalCost, NULL);
      }
      else
      {
         gUI.playCantDoSound();
      }
      return;
   }
   gUI.playClickSound();
   BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandGame);
   if(pCommand)
   {
      pCommand->setSenderType(BCommand::cPlayer);
      pCommand->setSenders(1, &mPlayerID);
      pCommand->setRecipientType(BCommand::cGame);
      pCommand->setType(BGameCommand::cTypeRepair);
      if (!pCommand->setRecipients(squads.getNumber(), squads.getPtr()))
      {
         delete pCommand;
         return;
      }
      gWorld->getCommandManager()->addCommandToExecute(pCommand);
   }
}

//==============================================================================
// BUser::canRepair
//==============================================================================
bool BUser::canRepair(BCost* pCost) const
{
   bool retval=false;

   BEntityIDArray ugIDs;
   mSelectionManager->getSubSelectedSquads(ugIDs);

   int squadCount=ugIDs.getNumber();
   if (squadCount>0)
   {
      BCost squadCost;
      for (int i=0; i<squadCount; i++)
      {
         const BSquad* pSquad=gWorld->getSquad(ugIDs[i]);
         if (pSquad && pSquad->getProtoSquad()->getFlagRepairable() && !pSquad->getFlagIsRepairing())
         {
            if (pCost)
            {
               if  (pSquad->getPlayerID()!=mPlayerID)
               {
                  retval=false;
                  break;
               }
               float temp;
               if (pSquad->getRepairCost(squadCost, temp))
               {
                  pCost->add(&squadCost);
                  retval=true;
               }
            }
            else
            {
               if (pSquad->getPlayerID()==mPlayerID)
               {
                  retval=true;
                  break;
               }
            }
         }
      }
   }
   return retval;
}

//==============================================================================
// BUser::resetStickyReticle
//==============================================================================
void BUser::resetStickyReticle()
{
   if (mStickyHoverObject != cInvalidObjectID)
      setFlagUpdateHoverPoint(true);
   mStickyHoverObject = cInvalidObjectID;
   mFlagStickyReticleDoFollow = false;
   mFlagStickyReticleDoJump = false;
}

//==============================================================================
// BUser::handleEndGame
//==============================================================================
void BUser::handleEndGame()
{
   cancelPower();

   // Cancel any confirmation dialogs that might be up.
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   if( pUIGlobals )
      pUIGlobals->cancel();

   // Reset the user mode
   resetUserMode();

   // [8/22/2008 xemu] make sure we reset the camera to match
   setFlagCameraZoomEnabled(true);
   resetCameraDefaults();
   resetCameraSettings(false);
  
   // Reset rumble whenever game ends to prevent rumble staying on forever.
   gInputSystem.getGamepad(gUserManager.getPrimaryUser()->getPort()).resetRumble();

   // Remove any remaining Flash User Messages, since they are now irrelevant
   for ( int i = mFlashUserMessages.getNumber()-1; i >= 0; --i )
   {
      if (!mFlashUserMessages[i]->existPastEndGame())
      {
         mFlashUserMessages[i]->expire();
      }
   }
   //mFlashUserMessages.clear();

   long gameType = -1;

   //-- FIXING PREFIX BUG ID 5893
   const BGameSettings* pSettings = gDatabase.getGameSettings();
   //--
   BASSERT(pSettings);
   pSettings->getLong(BGameSettings::cGameType, gameType);

   if (gameType == BGameSettings::cGameTypeSkirmish)
      gUIManager->showNonGameUI(BUIManager::cEndGameScreen, this);

   if (mFlashUserMessages.getSize() > 0 && gUIManager->getWidgetUI())
      (gUIManager->getWidgetUI())->setUserMessageVisible(true, true);
}

//==============================================================================
// BUser::resetUserMode
//==============================================================================
void BUser::resetUserMode()
{
   switch (mUserMode)
   {
      case cUserModeCircleSelecting:
         circleSelect(false);
         break;      // this causes a reset of the user mode.

      case cUserModeNormal:
         return;

      case cUserModeCommandMenu:
      case cUserModePowerMenu:    
      case cUserModeCinematic:
      case cUserModeFollow:
         break;

      case cUserModeRallyPoint:
      case cUserModeBuildingRallyPoint:
      case cUserModeBuildLocation:
      case cUserModeAbility:
      {
         mAbilityID = -1;
         releaseHoverVisual();
         break;
      }
      case cUserModeInputUILocation:
      {
         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (c)
         {
            // Set up the command.
            c->setSenders( 1, &mPlayerID );
            c->setSenderType( BCommand::cPlayer );
            c->setRecipientType( BCommand::cPlayer );
            c->setType( BTriggerCommand::cTypeBroadcastInputUILocationResult );
            // Register the correct trigger system and variable this stuff goes into.
            c->setTriggerScriptID( mTriggerScriptID );
            c->setTriggerVarID( mTriggerVarID );
            // Set the data that will be poked in.
            c->setInputResult( BTriggerVarUILocation::cUILocationResultCancel );
            // No input location because we cancelled.
            // Ok rock on.
            gWorld->getCommandManager()->addCommandToExecute( c );
         }
         break;
      }
      case cUserModeInputUILocationMinigame:
      {
         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (c)
         {
            // Set up the command.
            c->setSenders( 1, &mPlayerID );
            c->setSenderType( BCommand::cPlayer );
            c->setRecipientType( BCommand::cPlayer );
            c->setType( BTriggerCommand::cTypeBroadcastInputUILocationMinigameResult );
            // Register the correct trigger system and variable this stuff goes into.
            c->setTriggerScriptID( mTriggerScriptID );
            c->setTriggerVarID( mTriggerVarID );
            // Set the data that will be poked in.
            c->setInputResult( BTriggerVarUILocation::cUILocationResultCancel );
            // No input location because we cancelled.
            // Ok rock on.
            gWorld->getCommandManager()->addCommandToExecute( c );
         }
         break;
      }
      case cUserModeInputUIUnit:
      {
         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (c)
         {
            // Set up the command.
            c->setSenders(1, &mPlayerID);
            c->setSenderType(BCommand::cPlayer);
            c->setRecipientType(BCommand::cPlayer);
            c->setType(BTriggerCommand::cTypeBroadcastInputUIUnitResult);
            // Register the correct trigger system and variable this stuff goes into.
            c->setTriggerScriptID(mTriggerScriptID);
            c->setTriggerVarID(mTriggerVarID);
            // Set the data that will be poked in.
            c->setInputResult(BTriggerVarUIUnit::cUIUnitResultCancel);
            // Ok rock on.
            gWorld->getCommandManager()->addCommandToExecute(c);
         }
         break;
      }
      case cUserModeInputUISquad:
      {
         BTriggerCommand *c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (c)
         {
            // Set up the command.
            c->setSenders(1, &mPlayerID);
            c->setSenderType(BCommand::cPlayer);
            c->setRecipientType(BCommand::cPlayer);
            c->setType(BTriggerCommand::cTypeBroadcastInputUISquadResult);
            // Register the correct trigger system and variable this stuff goes into.
            c->setTriggerScriptID(mTriggerScriptID);
            c->setTriggerVarID(mTriggerVarID);
            // Set the data that will be poked in.
            c->setInputResult(BTriggerVarUISquad::cUISquadResultCancel);
            // Ok rock on.
            gWorld->getCommandManager()->addCommandToExecute(c);
         }
         break;
      }
      case cUserModeInputUISquadList:
      {
         BTriggerCommand* c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandTrigger);
         if (c)
         {
            // Set up the command.
            c->setSenders(1, &mPlayerID);
            c->setSenderType(BCommand::cPlayer);
            c->setRecipientType(BCommand::cPlayer);
            c->setType(BTriggerCommand::cTypeBroadcastInputUISquadListResult);
            // Register the correct trigger system and variable this stuff goes into.
            c->setTriggerScriptID(mTriggerScriptID);
            c->setTriggerVarID(mTriggerVarID);
            // Set the data that will be poked in.
            c->setInputResult(BTriggerVarUISquad::cUISquadResultCancel);
            // Ok rock on.
            gWorld->getCommandManager()->addCommandToExecute(c);
            clearAllSelections();
         }
         break;
      }
   }

   changeMode(cUserModeNormal);
}

//==============================================================================
// BUser::resign
//    For multiplayer, you just send a message saying that you have resigned.
//
//    For single player, everybody on your team gets a defeated state and everybody
//       on the other team gets a won state. We have to do this manually because
//       of the AIs. We don't want them to continue playing if we quit.
//==============================================================================
void BUser::resign()
{
   if (gLiveSystem->isMultiplayerGameActive())
   {
      mPlayerState = BPlayer::cPlayerStateResigned;
      getPlayer()->sendResign();
      return;
   }

   // for single player, do it manually
   BPlayer* player = getPlayer();
   if (!player)
      return;

//-- FIXING PREFIX BUG ID 5894
   const BTeam* team = player->getTeam();
//--
   if (!team)
      return;

   //-- resign the player
   player->setPlayerState(BPlayer::cPlayerStateResigned);

   //-- resign everyone else on the team if only AI Players are left
   bool bOnlyAIPlayersLeftOnTeam = true;
//-- FIXING PREFIX BUG ID 5895
   const BPlayer* pCurPlayer = NULL;
//--
   for (int j = 0; j < team->getNumberPlayers(); ++j)
   {      
      pCurPlayer = gWorld->getPlayer(team->getPlayerID(j));

      if (!pCurPlayer)
         continue;

      if (pCurPlayer == player)
         continue;

      if (pCurPlayer->isHuman())
      {
         bOnlyAIPlayersLeftOnTeam = false;
         break;
      }
   }

   if (bOnlyAIPlayersLeftOnTeam)
   {
      for (long i=1; i<gWorld->getNumberPlayers(); i++)
      {
         player = gWorld->getPlayer(i);
         if (!player)
            continue;

         if (player->getPlayerState() != BPlayer::cPlayerStatePlaying)
            continue;

         if (player->getTeamID() == team->getID())
         {
            // you lose
            player->setPlayerState(BPlayer::cPlayerStateDefeated);
         }
         else
         {
            // they win
            player->setPlayerState(BPlayer::cPlayerStateWon);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BUser::isScrollingMode(int mode) const
{
   if (mode==cUserModeNormal || 
       mode==cUserModeCircleSelecting || 
       mode==cUserModeInputUILocation || 
       mode==cUserModeInputUIUnit || 
       mode==cUserModeInputUISquad || 
       mode==cUserModeInputUISquadList || 
       mode==cUserModeInputUIPlaceSquadList || 
       mode==cUserModeRallyPoint || 
       mode==cUserModeBuildingRallyPoint ||
       mode==cUserModeBuildLocation || 
       mode==cUserModeAbility)
      return true;

   // Check minigame mode
   if (mode == cUserModeInputUILocationMinigame)
   {
      if (mFlagUIPowerMinigame)
         return mMinigame.isScrolling();
      else
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BUser::doScrollDelay()
{
   mFlagDelayScrolling=true;
   mFlagDelayScrollingStart=true;
   mScrollDelayTime=0.0f;
}

//==============================================================================
//==============================================================================
bool BUser::isSphereVisible(XMVECTOR center, float radius) const
{
   return mSceneVolumeCuller.isSphereVisible(center, radius);
}

//==============================================================================
//==============================================================================
float BUser::scrollSpeedAdjByteToFloat( uint8 u )
{
   int i = (int)u - 5;
   if (i >= 0)
      return (cDefaultScrollSpeedAdjFloat + cConvertScrollAdjPosStepFloat*((float)i));
   else
      return (cDefaultScrollSpeedAdjFloat + cConvertScrollAdjNegStepFloat*((float)i));
 //  return (i / 15.0f) * 2.5f;
}

//==============================================================================
//==============================================================================
uint8 BUser::scrollSpeedAdjFloatToByte( float f )
{
   int i;
   if (f >= cDefaultScrollSpeedAdjFloat)
      i = (int)((f - cDefaultScrollSpeedAdjFloat) / cConvertScrollAdjPosStepFloat);
   else
      i = (int)((f - cDefaultScrollSpeedAdjFloat) / cConvertScrollAdjNegStepFloat);
   
   return (uint8)(i + 5);

//   return (uint8)(((f - 0.5f) / 2.5f) * 15);
}


//==============================================================================
//==============================================================================
void BUser::resetUserOptionsToDefault()
{
   //game options

   //control options
   setOption_ScrollSpeed(scrollSpeedAdjFloatToByte(cDefaultScrollSpeedAdjFloat));
   setOption_ControlScheme( 0 );
   BSimString controllerConfigName;
   if (gConfig.get(cConfigControllerConfig, controllerConfigName))
   {
      long index = gInputSystem.getControllerConfigIndex(controllerConfigName);
      if (index != -1)
         setOption_ControlScheme( (uint8)index );
   }

   setOption_RumbleEnabled( gConfig.isDefined(cConfigGamepadRumble) );

   setOption_CameraRotationEnabled( true );


   setOption_YInverted(gConfig.isDefined(cConfigCameraPitchInvert));

   setOption_XInverted(false);

   setOption_DefaultZoomLevel( 45 );
   calcCameraDefaultPitch();

   
   float stickyReticleSensitivity = 0.0f;
   gConfig.get(cConfigStickyReticleSensitivity, &stickyReticleSensitivity);
   setOption_StickyCrosshairSensitivity( (uint8)stickyReticleSensitivity );

   mFlagStickyReticleJumpCamera = gConfig.isDefined(cConfigStickyReticleJumpCamera);
   
   setOption_CameraFollowEnabled( gConfig.isDefined(cConfigStickyReticleFollow) );

   setOption_AIAdvisorEnabled( true );
   
   setOption_SelectionSpeed( (uint8)cDefaultSelectionSpeed );
   
   //Music options
   setOption_MusicVolume( (uint8)cDefaultMusicVolume );
   setOption_SFXVolume( (uint8)cDefaultSoundFXVolume );
   setOption_VoiceVolume( (uint8)cDefaultVoiceVolume );
   setOption_SubtitlesEnabled( gConfig.isDefined(cConfigSubTitles) );
   setOption_ChatTextEnabled( gConfig.isDefined(cConfigSubTitles) );

   //graphics options
   setOption_Brightness( (uint8)cDefaultUserGamma );
   setOption_FriendOrFoeColorsEnabled( gConfig.isDefined(cConfigFriendOrFoe) );
   setOption_MiniMapZoomEnabled( true );

   //misc options

}

//==============================================================================
//==============================================================================
void BUser::yornResult(uint result, DWORD userContext, int port)
{
   long id = userContext & 0x0000FFFF;
   long yornType = userContext >> 16;

   switch (yornType)
   {
      // --------------------------------------------
      case cUserYornBoxOOSNotification:
         if (!gWorld->getFlagGameOver())
         {
            bool flag = gWorld->getFlagSkipGameOverCheck();
            gWorld->setFlagSkipGameOverCheck(true);

            // end ourselves first
            BPlayer* player = getPlayer();
            if (player && player->getPlayerState() == BPlayer::cPlayerStatePlaying)
               player->setPlayerState(BPlayer::cPlayerStateResigned);

            // end the game - everybody loses.
            for (long i=1; i<gWorld->getNumberPlayers(); i++)
            {
               player = gWorld->getPlayer(i);
               if (!player)
                  continue;

               if (!player->isHuman() && !player->isComputerAI())
                  continue;

               // everbody else
               if (player->getPlayerState() == BPlayer::cPlayerStatePlaying)
                  player->setPlayerState(BPlayer::cPlayerStateResigned);
            }
            
            gWorld->setFlagSkipGameOverCheck(flag);

            // do this now that 
            gWorld->checkForGameOver();
         }
         break;

      // --------------------------------------------
      case cUserYornBoxDisconnectNotification:
         {
            break;
         }

      // --------------------------------------------
      case cUserYornBoxResignConfirmation:
      {
         switch(result)
         {
            case BUIGlobals::cDialogResultOK:
            {
               resign();
               break;
            }
            case BUIGlobals::cDialogResultCancel:
               break;
         }
         break;
      }


      // --------------------------------------------
      case cUserYornBoxRestartConfirmation:
      {
         switch(result)
         {
            case BUIGlobals::cDialogResultOK:
            {
   #ifndef BUILD_FINAL                              
   // rg [6/28/07] - E3 demo hackage
               if (gFinalBuild)
               {
                  if (DmRebootEx(DMBOOT_TITLE, "e:\\demo\\default.xex", "e:\\demo", "") != XBDM_NOERR)
                     DmReboot(DMBOOT_TITLE);
               }
   #endif               
               
               gModeManager.setMode( BModeManager::cModeMenu );
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoGame );
               break;
            }
            case BUIGlobals::cDialogResultCancel:
               break;
         }
         break;
      }


      // --------------------------------------------
      case cUserYornBoxSelfDestructBuilding:
      {
         switch(result)
         {
            case BUIGlobals::cDialogResultOK:
            {
               BBuildingCommand* pCommand=(BBuildingCommand*)gWorld->getCommandManager()->createCommand(mPlayerID, cCommandBuilding);
               if(pCommand)
               {
                  long type = BProtoObjectCommand::cTypeKill;

                  pCommand->setSenderType(BCommand::cPlayer);
                  pCommand->setSenders(1, &mPlayerID);
                  pCommand->setRecipientType(BCommand::cUnit);
                  pCommand->setRecipients(1, &mCommandObject);
                  pCommand->setType(type);
                  pCommand->setTargetID(id);
                  pCommand->setCount(1);
                  gWorld->getCommandManager()->addCommandToExecute(pCommand);

                  changeMode(cUserModeNormal);
                  doScrollDelay();
                  break;
               }
               break;
            }
            case BUIGlobals::cDialogResultCancel:
            {
               showCommandMenu(mHoverObject);
               break;
         }
      }
   }
   }
}

//==============================================================================
//==============================================================================
void BUser::handleUIManagerResult( BUIManager::EResult eResult )
{
   gUIManager->hideNonGameUI();

   switch( eResult )
   {
      case BUIManager::cResult_Resume:
         break;

      case BUIManager::cResult_Resign:
         resign();
         break;

      case BUIManager::cResult_Restart:
         mRequestedExitMethod = cExitMethodRestart;
         setFlagGameDoExit(true);
/*
#ifndef BUILD_FINAL
         if (gFinalBuild)
         {
            if (DmRebootEx(DMBOOT_TITLE, "e:\\demo\\default.xex", "e:\\demo", "") != XBDM_NOERR)
               DmReboot(DMBOOT_TITLE);
         }
#endif               
         gModeManager.setMode( BModeManager::cModeMenu );
         gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoGame );
*/
         break;

      case BUIManager::cResult_Exit:
         mRequestedExitMethod = cExitMethodQuit;
         setFlagGameDoExit(true);
         break;

      case BUIManager::cResult_Continue:
         mRequestedExitMethod = cExitMethodContinue;
         setFlagGameDoExit(true);
         break;

      case BUIManager::cResult_GoToAdvancedTutorial:
         mRequestedExitMethod = cExitMethodGoToAdvancedTutorial;
         setFlagGameDoExit(true);
      break;
   }
}

//==============================================================================
// BUser::tieToCameraRep()
//
// Changes the camera position (mpCamera->mCameraLoc) so that it is a given
// distance away from the camera height field
//==============================================================================
void BUser::tieToCameraRep()
{
   if (gTerrainSimRep.cameraHeightsLoaded())
   {
      BVector intersectionPt;
      if (gTerrainSimRep.rayIntersectsCamera(mpCamera->getCameraLoc(), mpCamera->getCameraDir(), intersectionPt))
      {
         BVector pos=intersectionPt-(mpCamera->getCameraDir()*mCameraZoom);
         mpCamera->setCameraLoc(pos);
         setFlagUpdateHoverPoint(true);
      }
   }
   saveLastCameraLoc();
}


//==============================================================================
//==============================================================================
void BUser::endGameOOS()
{
   // show the yorn box to verify.
   BUIGlobals* puiGlobals = gGame.getUIGlobals();
   if (puiGlobals)
   {
      DWORD value = (cUserYornBoxOOSNotification << 16);
      puiGlobals->showYornBoxSmall(this, gDatabase.getLocStringFromID(24057), BUIGlobals::cDialogButtonsOK, value);
   }
}

//==============================================================================
// 
//==============================================================================
void BUser::endGameDisconnect()
{
   if (!gWorld)
      return;

   // check to see if the game has ended, and if not end it now
   if (!gWorld->getFlagGameOver())
   {
      bool flag = gWorld->getFlagSkipGameOverCheck();
      gWorld->setFlagSkipGameOverCheck(true);

      // mark my team as having won
      BPlayer* pPlayer = getPlayer();
      if (pPlayer && pPlayer->getPlayerState() == BPlayer::cPlayerStatePlaying)
      {
         // set us to disconnected and then won
         pPlayer->setNetState(BPlayer::cPlayerNetStateDisconnected);

         long gameType = -1;
         const BGameSettings* pSettings = gDatabase.getGameSettings();
         if(pSettings)
            pSettings->getLong(BGameSettings::cGameType, gameType);

         if (gameType == BGameSettings::cGameTypeSkirmish)
            pPlayer->setPlayerState(BPlayer::cPlayerStateWon);
         else
            pPlayer->setPlayerState(BPlayer::cPlayerStateDisconnected);
      }

      for (long i=1; i < gWorld->getNumberPlayers(); i++)
      {
         pPlayer = gWorld->getPlayer(i);
         if (!pPlayer)
            continue;

         if (!pPlayer->isHuman() && !pPlayer->isComputerAI())
            continue;

         // setting a player to disconnected will resign them and set their net state to disconnected
         if (pPlayer->getPlayerState() == BPlayer::cPlayerStatePlaying)
            pPlayer->setPlayerState(BPlayer::cPlayerStateDisconnected);
      }

      gWorld->setFlagSkipGameOverCheck(flag);

      // do this now that 
      gWorld->checkForGameOver();

      // show the yorn box to verify.
      BUIGlobals* puiGlobals = gGame.getUIGlobals();
      if (puiGlobals)
      {
         DWORD value = (cUserYornBoxDisconnectNotification << 16);
         puiGlobals->showYornBoxSmall(this, gDatabase.getLocStringFromID(24116), BUIGlobals::cDialogButtonsOK, value);
      }
   }
}

//==============================================================================
// check live and GOLD account.
//==============================================================================
BOOL BUser::isSignedIntoLive() const 
{ 
   if (mSignInState != eXUserSigninState_SignedInToLive)
      return false;

   // removing the privilege check.
   // if we want to explicitly test for a Gold account we should do so where needed and not overload this method.
   //return checkPrivilege(XPRIVILEGE_MULTIPLAYER_SESSIONS);
   return TRUE;
}

//==============================================================================
//==============================================================================
bool BUser::isPointingAt(BEntityID entityID)
{
   if (!mFlagHaveHoverPoint)
      return false;

//-- FIXING PREFIX BUG ID 5898
   const BSquad* pSquad = (entityID.getType() == BEntity::cClassTypeSquad ? gWorld->getSquad(entityID) : NULL);
//--
   uint numChildren = (pSquad ? pSquad->getNumberChildren() : 0);

   float clickSize=20.0f;
   gConfig.get(cConfigCircleSelectClickSize, &clickSize);

   mSelectionManager->populateSelectionsRect(mpCamera->getCameraLoc(), mHoverPoint, clickSize*0.5f, clickSize*0.5f, false, true);

   long possibleCount = mSelectionManager->getPossibleSelectionCount();
   for(long i=0; i<possibleCount; i++)
   {
      BEntityID possibleID = mSelectionManager->getPossibleSelection(i);
      if (possibleID == entityID)
         return true;
      if (pSquad)
      {
         for (uint j=0; j<numChildren; j++)
         {
            if (possibleID == pSquad->getChild(j))
               return true;
         }
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
void BUser::saveLastCameraLoc()
{
//   mLastCameraLoc = mpCamera->getCameraLoc();

   if (mFlagUpdateHoverPoint)
      updateHoverPoint();

//   mLastCameraHoverPoint = mCameraHoverPoint;
//   mFlagHaveLastHoverPoint = mFlagHaveHoverPoint;
}

//==============================================================================
//==============================================================================
void BUser::calcCameraDefaultPitch()
{
   if (mCameraZoomMin != mCameraZoomMax && mCameraPitchMin != mCameraPitchMax)
   {
      float zoomRange = mCameraZoomMax - mCameraZoomMin;
      float zoomPct = (mCameraDefaultZoom - mCameraZoomMin) / zoomRange; //(getOption_DefaultZoomLevel() - mCameraZoomMin) / zoomRange;
      float pitchRange = mCameraPitchMax - mCameraPitchMin;
      mCameraDefaultPitch = mCameraPitchMin + (zoomPct * pitchRange);
   }
}

//==============================================================================
//==============================================================================
const char* BUser::getUserModeName(int userMode)
{
   switch (userMode)
   {
      case BUser::cUserModeNormal:                 return "Normal";
      case BUser::cUserModeCircleSelecting:        return "CircleSelecting";
      case BUser::cUserModeInputUILocation:        return "InputUILocation";
      case BUser::cUserModeRallyPoint:             return "RallyPoint";
      case BUser::cUserModeBuildLocation:          return "BuildLocation";
      case BUser::cUserModeCommandMenu:            return "CommandMenu";
      case BUser::cUserModePowerMenu:              return "PowerMenu";
//      case BUser::cUserModeGameMenu:               return "GameMenu";
      case BUser::cUserModeAbility:                return "Ability";
      case BUser::cUserModeInputUIUnit:            return "InputUIUnit";
      case BUser::cUserModeInputUISquad:           return "InputUISquad";
//      case BUser::cUserModeObjectiveMenu:          return "ObjectiveMenu";
      case BUser::cUserModeCinematic:              return "Cinematic";
      case BUser::cUserModeFollow:                 return "Follow";
//      case BUser::cUserModeCampaignEndMenu:        return "CampaignEndMenu";
      case BUser::cUserModeInputUILocationMinigame:return "InputUILocationMinigame";
      case BUser::cUserModeInputUISquadList:       return "InputUISquadList";
//      case BUser::cUserModePostGame:               return "PostGame";
//      case BUser::cUserModePostGameStats:          return "PostGameStats";
      case BUser::cUserModePower:                  return "Power";
   }
   return "Unknown";
}

//==============================================================================
//==============================================================================
bool BUser::canTarget(const BObject* pObject, const BRenderViewParams& view) const
{
   if (!pObject)
      return false;

   bool isDopple = (pObject->getDopple() != NULL);
   if (!isDopple && !pObject->isAlive())
      return false;

   const BUnit* pUnit = pObject->getUnit();
   if (pUnit && pUnit->getFlagDown())
      return (false);

   if (pObject->isOutsidePlayableBounds())
      return false;

   if (pObject->isGarrisoned() || pObject->isAttached() || pObject->isHitched())
      return false;

   if (pObject->getSelectType(getTeamID()) == cSelectTypeNone)
      return false;

   if (pObject->getFlagNoRender())
      return false;

   const BProtoObject* pProtoObject = pObject->getProtoObject();
   if ((!isDopple && pObject->getFlagInvulnerable()) || (isDopple && pProtoObject->getFlagInvulnerable()))
      return false;

   if (!pObject->isVisible(mTeamID) && (!pObject->isDoppled(mTeamID) || pObject->hasDoppleObject(mTeamID)))
      return false;

   //const BBoundingBox* pBoundingBox=pObject->getVisualBoundingBox();
   //if (!isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
   //   return false;
   float r=pProtoObject->getPickRadius();
   if(r==0.0f)
      r=pObject->getVisualRadius();
   float offset=pProtoObject->getPickOffset();
   BVector uc = (offset==0.0f ? pObject->getVisualCenter() : pObject->getPosition()+(pObject->getUp()*offset));
   BVector up;
   view.calculateWorldToScreen(uc, up.x, up.y);
   if(up.x<0.0f || up.y<0.0f || up.x>(float)view.getViewportWidth() || up.y>(float)view.getViewportHeight())
      return false;

   if (!Math::IsValidFloat(pObject->getPosition().x))
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BUser::calcTargetSelectPos()
{
   if (mTargetSelectSquad != cInvalidObjectID)
   {
      BSquad* pSquad = gWorld->getSquad(mTargetSelectSquad);
      if (pSquad)
      {
         mTargetSelectPos = pSquad->getAveragePosition();
         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (pProtoObject)
            mTargetSelectPos.y += pProtoObject->getObstructionRadiusY();
         return;
      }
   }
//-- FIXING PREFIX BUG ID 5901
   const BObject* pObject = gWorld->getObject(mTargetSelectObject);
//--
   if (pObject)
      mTargetSelectPos = pObject->getSimCenter();
}

//==============================================================================
//==============================================================================
void BUser::updateTargetSelect()
{
   if (mFlagTargetSelecting)
   {
//-- FIXING PREFIX BUG ID 5902
      const BSquad* pTargetSquad = gWorld->getSquad(mTargetSelectSquad);
//--
      if (!pTargetSquad && mTargetSelectSquad != cInvalidObjectID)
      {
         uiCycleTarget(false, false);
         return;
      }

      BRenderViewParams view(gRender.getViewParams());
      BMatrix mat;
      mpCamera->getViewMatrix(mat);
      view.setViewMatrix(mat);

      if (pTargetSquad)
         mTargetSelectObject = pTargetSquad->getLeader();

      BObject* pTargetObject = gWorld->getObject(mTargetSelectObject);
      if (!pTargetObject || !canTarget(pTargetObject, view))
      {
         if (mTargetSelectDoppleForObject != cInvalidObjectID)
         {
            pTargetObject = gWorld->getObject(mTargetSelectObject);
            if (pTargetObject && canTarget(pTargetObject, view))
            {
               mTargetSelectObject = mTargetSelectDoppleForObject;
               mTargetSelectDoppleForObject = cInvalidObjectID;
               calcTargetSelectPos();
               return;
            }
         }
         uiCycleTarget(false, false);
      }
      else
         calcTargetSelectPos();
   }
}

//==============================================================================
//==============================================================================
void BUser::uiSelectAll(bool global, bool reverse, bool autoGlobal, bool autoCycle, bool selectAndCycle, bool noSound)
{
   if (autoGlobal && mFlagDelayLocalMilitarySound)
      global = true;
   else 
   {
      if (selectAndCycle)
      {
         mSelectionManager->saveSubSelect();
         uiSelectAll(global, reverse, false, false, false, true);
         mSelectionManager->restoreSubSelect();
         if (mSelectionManager->getNumSubSelectGroups() > 1)
         {
            resetGotoSelected();
            gUI.playClickSound();
            if (reverse)
               mSelectionManager->subSelectPrev();
            else
               mSelectionManager->subSelectNext();
         }
         return;
      }
      else if (autoCycle && mSelectionManager->getNumSubSelectGroups() > 1)
      {
         uiCycleGroup(reverse, false);
         return;
      }
   }

   autoExitSubMode();
   resetCircleSelectionCycle();
   resetFindCrowd();
   mSelectionManager->clearSelections();

   if (global)
   {
      mFlagDelayLocalMilitarySound=false;
      mLocalSelectTime=0.0f;
   }

   BEntityHandle handle=cInvalidObjectID;
//-- FIXING PREFIX BUG ID 5900
   for (const BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
//--
   {
      if (pUnit->getPlayerID()!=mPlayerID)
         continue;

      if (!pUnit->isAlive() || pUnit->getFlagDown())
         continue;

      // jce [10/1/2008] -- Disabling this.  Best I can gather from talking to those involved, this was to
      // fix an issue in a scenario that doesn't work the same way any more.  The problem with this check is
      // that if you have any guys that accidentally get out of bounds, they become non-selectable and stuck out there.
      /*
      if (pUnit->isOutsidePlayableBounds())
         continue;
      */

      if (pUnit->isGarrisoned() || pUnit->isAttached() || pUnit->isHitched())
         continue;

      if (gConfig.isDefined(cConfigSelectAllIgnoreWorking))
      {
         if(pUnit->isWorking())
            continue;
      }

      if (pUnit->getSelectType(getTeamID()) != cSelectTypeUnit)
         continue;

      long gotoType=pUnit->getGotoType();
      if (gotoType!=cGotoTypeInfantry && gotoType!=cGotoTypeVehicle && gotoType!=cGotoTypeAir && gotoType!=cGotoTypeHero)
         continue;

      if (!global)
      {
         const BBoundingBox* pBoundingBox=pUnit->getVisualBoundingBox();
         if (!isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius()))
            continue;
      }

      if (!Math::IsValidFloat(pUnit->getPosition().x))
         continue;

      mSelectionManager->selectUnit(pUnit->getID());
   }

   if (!noSound && mSelectionManager->getNumberSelectedUnits() > 0)
   {
      if (global)
         gUIGame.playSound(BSoundManager::cSoundGlobalMilitary, getPlayer()->getCivID(), true);
      else
      {
         BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( getOption_ControlScheme() );
         long controlType1=-1, controlType2=-1;
         bool modifyFlag1=false, modifyFlag2=false;
         bool doubleClick1=false, doubleClick2=false;
         bool hold1=false, hold2=false;
         pInputInterface->getFunctionControl(BInputInterface::cInputGlobalSelect, controlType1, modifyFlag1, doubleClick1, hold1);
         pInputInterface->getFunctionControl(BInputInterface::cInputScreenSelect, controlType2, modifyFlag2, doubleClick2, hold2);
         if (controlType1==controlType2 && modifyFlag1==modifyFlag2)
         {
            mFlagDelayLocalMilitarySound=true;
            mLocalSelectTime=0.0f;
         }
         else
            gUIGame.playSound(BSoundManager::cSoundLocalMilitary, getPlayer()->getCivID(), true);
      }
   }

   if (global)
   {
      #ifdef _VINCE_
         MVinceEventAsync_ControlUsed(this, "global_select");
      #endif
      gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGlobalSelect, mPlayerID);
   }
   else
   {
      #ifdef _VINCE_
         MVinceEventAsync_ControlUsed(this, "screen_select_military");
      #endif
      gGeneralEventManager.eventTrigger(BEventDefinitions::cControlScreenSelectMilitary, mPlayerID);
   }
}

//==============================================================================
//==============================================================================
void BUser::uiCycleGroup(bool reverse, bool wrap)
{
   if (mSelectionManager->getNumSubSelectGroups() > 1)
   {
      resetGotoSelected();
      gUI.playClickSound();
      if (reverse)
      {
         if (!wrap && mSelectionManager->getSubSelectGroupHandle() == 0)
            //mSelectionManager->clearCurrentSubSelect();
            clearAllSelections();               
         else
            mSelectionManager->subSelectPrev();
      }
      else
      {
         if (!wrap && mSelectionManager->getSubSelectGroupHandle() == ((int)mSelectionManager->getNumSubSelectGroups()) - 1)
            //mSelectionManager->clearCurrentSubSelect();
            clearAllSelections();               
         else
            mSelectionManager->subSelectNext();
      }
   }
   #ifdef _VINCE_
      MVinceEventAsync_ControlUsed(this, "sub_select");
   #endif
}


//==============================================================================
//==============================================================================
struct BUserTarget
{
   BObject* mpObject;
   long     mX, mY;
};

int userTargetSort(const void* pItem1, const void* pItem2)
{
   // Sort left to right, top to bottom, then by ID
   BUserTarget* pTarget1 = (BUserTarget*)pItem1;
   BUserTarget* pTarget2 = (BUserTarget*)pItem2;
   if (pTarget1->mX < pTarget2->mX)
      return -1;
   if (pTarget1->mX > pTarget2->mX)
      return 1;
   if (pTarget1->mY < pTarget2->mY)
      return -1;
   if (pTarget1->mY > pTarget2->mY)
      return 1;
   int id1 = pTarget1->mpObject->getID().asLong();
   int id2 = pTarget2->mpObject->getID().asLong();
   if (id1 < id2)
      return -1;
   if (id1 > id2)
      return 1;
   return 0;
}

void BUser::uiCycleTarget(bool reverse, bool userInitiated)
{
   if (mSelectionManager->getNumSubSelectGroups() == 0)
   {
      if (mFlagTargetSelecting)
         autoExitSubMode();
   }
   else
   {
      // Build a list of on-screen targets
//-- FIXING PREFIX BUG ID 5905
      const BPlayer* pPlayer = getPlayer();
//--
      BUserTarget target;
      target.mX = -1;
      target.mY = -1;
      BSmallDynamicSimArray<BUserTarget> targets;

      BRenderViewParams view(gRender.getViewParams());
      BMatrix mat;
      mpCamera->getViewMatrix(mat);
      view.setViewMatrix(mat);

      BEntityHandle handle=cInvalidObjectID;
      for (BSquad* pSquad=gWorld->getNextSquad(handle); pSquad!=NULL; pSquad=gWorld->getNextSquad(handle))
      {
         if (pSquad->getPlayerID()==mPlayerID)
            continue;
         if (!pPlayer->isEnemy(pSquad->getPlayerID()))
            continue;
         BUnit* pUnit = pSquad->getLeaderUnit();
         if (!pUnit)
            continue;
         if (!canTarget(pUnit, view))
            continue;
         if (pUnit->hasDoppleObject(mTeamID))
            continue;
         target.mpObject = pUnit;
         targets.add(target);
      }

      handle=cInvalidObjectID;
      for (BDopple* pDopple=gWorld->getNextDopple(handle); pDopple!=NULL; pDopple=gWorld->getNextDopple(handle))
      {
         if(pDopple->getForTeamID()!=mTeamID)
            continue;
         if (!pPlayer->isEnemy(pDopple->getPlayerID()))
            continue;
         if (!canTarget(pDopple, view))
            continue;
         target.mpObject = pDopple;
         targets.add(target);
      }

      int targetCount=targets.getNumber();
      if (targetCount == 0)
      {
         if (mFlagTargetSelecting)
            autoExitSubMode();
      }
      else
      {
         // Compute screen position for all targets
         const BRenderViewParams& view = gRender.getViewParams();
         for (int i=0; i<targetCount; i++)
         {
            BUserTarget& target = targets[i];
            view.calculateWorldToScreen(target.mpObject->getSimCenter(), target.mX, target.mY);
         }

         // Sort the targets
         targets.sort(userTargetSort);

         // Pick the next target
         int targetIndex = -1;
         if (mFlagTargetSelecting)
         {
            long lastX, lastY;
            view.calculateWorldToScreen(mTargetSelectPos, lastX, lastY);

            if (reverse)
            {
               for (int i=targetCount-1; i>=0; i--)
               {
//-- FIXING PREFIX BUG ID 5904
                  const BUserTarget& target = targets[i];
//--
                  if (target.mX < lastX)
                  {
                     targetIndex = i;
                     break;
                  }
                  else if (target.mX == lastX)
                  {
                     if (target.mY < lastY)
                     {
                        targetIndex = i;
                        break;
                     }
                     else if (target.mY == lastY)
                     {
                        int id1 = target.mpObject->getID().asLong();
                        int id2 = mTargetSelectObject.asLong();
                        if (id1 < id2)
                        {
                           targetIndex = i;
                           break;
                        }
                     }
                  }
               }
            }
            else
            {
               for (int i=0; i<targetCount; i++)
               {
                  BUserTarget& target = targets[i];
                  if (target.mX > lastX)
                  {
                     targetIndex = i;
                     break;
                  }
                  else if (target.mX == lastX)
                  {
                     if (target.mY > lastY)
                     {
                        targetIndex = i;
                        break;
                     }
                     else if (target.mY == lastY)
                     {
                        int id1 = target.mpObject->getID().asLong();
                        int id2 = mTargetSelectObject.asLong();
                        if (id1 > id2)
                        {
                           targetIndex = i;
                           break;
                        }
                     }
                  }
               }
            }
         }
         else
         {
            if (reverse)
               targetIndex = targetCount - 1;
            else
               targetIndex = 0;
         }

         if (targetIndex == -1)
            autoExitSubMode();
         else
         {
            BObject* pTargetObject = targets[targetIndex].mpObject;
            mTargetSelectObject = pTargetObject->getID();
            mTargetSelectSquad = (pTargetObject->getUnit() ? pTargetObject->getParentID() : cInvalidObjectID);
            mTargetSelectDoppleForObject = (pTargetObject->getDopple() ? pTargetObject->getParentID() : cInvalidObjectID);
            calcTargetSelectPos();
            mFlagTargetSelecting = true;
            mSubMode = cSubModeTargetSelect;
            gInputSystem.enterContext("TargetSelect");

            resetStickyReticle();

            mHoverObject = mTargetSelectObject;
            mHoverType = cHoverTypeNone;

            BVector point;
            if (gTerrainSimRep.rayIntersects(pTargetObject->getSimCenter(), mpCamera->getCameraDir(), point))
            {
               if (point.x >= gWorld->getSimBoundsMinX() && point.z >= gWorld->getSimBoundsMinZ() && 
                  point.x <  gWorld->getSimBoundsMaxX() && point.z <  gWorld->getSimBoundsMaxZ())
               {
                  mHoverPoint = point;
                  mFlagHaveHoverPoint = true;
                  mFlagHoverPointOverTerrain = true;
               }
            }

            BEntityIDArray selectedSquads;
            mSelectionManager->getSubSelectedSquads(selectedSquads);
            uint squadCount = selectedSquads.getSize();
            for (uint i=0; i<squadCount; i++)
            {
               BSquad* pSelectedSquad = gWorld->getSquad(selectedSquads[i]);
               if (pSelectedSquad)
               {
                  if (updateHoverType(pSelectedSquad, mHoverObject))
                  {
                     if (mHoverType != cHoverTypeNone)
                        break;
                  }
               }
            }
            if (mHoverType == cHoverTypeNone)
            {
               int selectType = pTargetObject->getSelectType(getTeamID());
               if ((selectType != cSelectTypeNone) && (selectType != cSelectTypeTarget) && (!pTargetObject->getProtoObject()->getFlagMustOwnToSelect() || (!pTargetObject->getDopple() && (pTargetObject->getPlayerID() == mPlayerID || pTargetObject->getPlayerID() == mCoopPlayerID))))
                  mHoverType = cHoverTypeSelect;       
            }
            mSelectionManager->updateSubSelectAbilities(mHoverObject, mHoverPoint);
         }
      }
   }

   if (userInitiated)
   {
      gUI.playRolloverSound();

      #ifdef _VINCE_
         MVinceEventAsync_ControlUsed(this, "target_select");
      #endif

      //gGeneralEventManager.eventTrigger(BEventDefinitions::cControlTargetSelect, mPlayerID);
   }
}

//==============================================================================
//==============================================================================
void BUser::uiSelectTarget()
{
   if (mFlagTargetSelecting)
   {
      gUI.playClickSound();

      if (mTargetSelectObject != cInvalidObjectID && mFlagHaveHoverPoint)
      {
         BVector targetPos = mHoverPoint;
         BVector cameraPos = targetPos - (mpCamera->getCameraDir() * mCameraZoom);
         mpCamera->setCameraLoc(cameraPos);
         tieToCameraRep();
         mSelectionManager->clearSelections();
//-- FIXING PREFIX BUG ID 5908
         const BUnit* pUnit = gWorld->getUnit(mTargetSelectObject);
//--
         if (pUnit && pUnit->isAlive() && !pUnit->getFlagDown() && pUnit->getSelectType(getTeamID()) != cSelectTypeNone && pUnit->getSelectType(getTeamID()) != cSelectTypeTarget)
            mSelectionManager->selectUnit(mTargetSelectObject);
      }

      autoExitSubMode();

      #ifdef _VINCE_
         MVinceEventAsync_ControlUsed( this, "goto_target_selection" );
      #endif

      //gGeneralEventManager.eventTrigger(BEventDefinitions::cControlGotoTargetSelection, mPlayerID);
   }
}

//==============================================================================
//==============================================================================
void BUser::uiCancel()
{
   if (mSubMode!=cSubModeNone)
   {
      gUI.playClickSound();
      autoExitSubMode();
      return;
   }

   if (gConfig.isDefined(cConfigExitSubSelectOnCancel) && mSelectionManager->getSubSelectGroupHandle() != -1)
   {
      gUI.playClickSound();
      mSelectionManager->clearCurrentSubSelect();
   }
   else
      clearAllSelections();               
}

//==============================================================================
//==============================================================================
void BUser::uiModifierAction(bool on)
{
   if (on)
   {
      setFlagModifierAction(true);
      MVinceEventAsync_ControlUsed(this, "modifier_action");
      gGeneralEventManager.eventTrigger(BEventDefinitions::cControlModifierAction, mPlayerID);

   }
   else
   {
      setFlagModifierAction(false);
      if (getFlagExitModeOnModifierRelease())
      {
         if (mUserMode==cUserModeBuildLocation)
            releaseHoverVisual();
         changeMode(cUserModeNormal);
      }
   }
}

//==============================================================================
//==============================================================================
void BUser::uiModifierSpeed(bool on)
{
   if (on)
   {
      setFlagModifierSpeed(true);
      MVinceEventAsync_ControlUsed(this, "modifier_speed");
      gGeneralEventManager.eventTrigger(BEventDefinitions::cControlModifierSpeed, mPlayerID);

   }
   else
      setFlagModifierSpeed(false);
}

//==============================================================================
//==============================================================================
void BUser::uiFlare(float x, float y)
{
   if(fabs(x)>0.1f || fabs(y)>0.1f)
      return;
   mFlagDelayFlare = true;
   mFlareTime = 0.0f;
   mFlarePos = mHoverPoint;
   //sendFlare(-1, mHoverPoint);
}

//==============================================================================
//==============================================================================
void BUser::uiMenu()
{
   if( !gUIManager->isNonGameUIVisible() )
   {
      if (mUserMode == cUserModeCircleSelecting)
         circleSelect(false);

      if (getFlagModifierAction())
         uiModifierAction(false);
      if (getFlagModifierSpeed())
         uiModifierSpeed(false);

      gUIManager->showNonGameUI( BUIManager::cGameMenu, this );
   }

/*
   if (mUserMode == cUserModePostGame)
      return;

//    #ifndef BUILD_FINAL
//       if (gConfig.isDefined(cConfigStartPauseButton) && !mFlagModifierAction)
//       {
//          bool paused=gModeManager.getModeGame()->getPaused();
//          gModeManager.getModeGame()->setPaused(!paused);
//          return;
//       }
//    #endif

   if (mUserMode != cUserModeGameMenu )
   {
      showGameMenu();
   }
*/
}

//==============================================================================
//==============================================================================
void BUser::uiObjectives()
{
   if( !gUIManager->isNonGameUIVisible() )
   {
      if (mUserMode == cUserModeCircleSelecting)
         circleSelect(false);

      gUIManager->showNonGameUI( BUIManager::cObjectivesScreen, this );
   }

   gGeneralEventManager.eventTrigger(BEventDefinitions::cMenuShowObjectives, mPlayerID);
}


//==============================================================================
//==============================================================================
void BUser::commandSquad(BEntityID squadID)
{
   // Don't if the squad is not valid.
   if (!gWorld->getSquad(squadID))
      return;

   // If we already have an entry for this squad, just update that one.
   uint highWaterMark = mCommandedSquads.getHighWaterMark();
   for (uint i=0; i<highWaterMark; i++)
   {
      if (mCommandedSquads.isInUse(i) && (mCommandedSquads[i].mEntityID == squadID))
      {
         mCommandedSquads[i].mFloat = 1.0f;
         return;
      }
   }

   // Create a new entry and update it.
   BEntityIDFloatPair* pData = mCommandedSquads.acquire(true);
   pData->mEntityID = squadID;
   pData->mFloat = 1.0f;
}


//==============================================================================
//==============================================================================
void BUser::updateCommandSquads(float elapsedTime)
{
   uint highWaterMark = mCommandedSquads.getHighWaterMark();
   for (uint i=0; i<highWaterMark; i++)
   {
      if (mCommandedSquads.isInUse(i))
      {
         if (!gWorld->getSquad(mCommandedSquads[i].mEntityID) || mCommandedSquads[i].mFloat <= elapsedTime)
            mCommandedSquads.release(i);
         else
            mCommandedSquads[i].mFloat -= elapsedTime;
      }
   }
}


//==============================================================================
//==============================================================================
void BUser::renderCommandSquads()
{
   uint hwm = mCommandedSquads.getHighWaterMark();
   for (uint i=0; i<hwm; i++)
   {
      if (!mCommandedSquads.isInUse(i))
         continue;
      BSquad* pSquad = gWorld->getSquad(mCommandedSquads[i].mEntityID);
      if (!pSquad)
         continue;
      if (!pSquad->isVisible(mTeamID))
         continue;

      float alphaUnclamped = mCommandedSquads[i].mFloat * 2.0f;
      float alphaClamped = static_cast<float>(Math::fSelectClamp(alphaUnclamped, 0.0f, 1.0f));
      BColor temp(gWorld->getPlayerColor(pSquad->getColorPlayerID(), BWorld::cPlayerColorContextSelection));
      DWORD color = packRGBA(temp, alphaClamped);

      float intensity=6.0f;
      gConfig.get(cConfigBaseDecalIntensity, &intensity);
      pSquad->drawSquadSelection(color, true, intensity, false, false);
   }
}


//==============================================================================
// BUser::setHUDItemEnabled
//==============================================================================
void BUser::setHUDItemEnabled(int hudItem, bool onoff)
{
   mHUDItemEnabled[hudItem]=onoff;
   if (!mpUIContext)
   {
      BFAIL("mpUIContext is invalid.");
      return;
   }

   switch (hudItem)
   {
      case cHUDItemMinimap:
         gUIManager->setMinimapVisible(onoff);
         break;

      case cHUDItemResources:
         mpUIContext->setResourcePanelVisible(onoff);
         break;

      case cHUDItemTime:
         mpUIContext->setGameTimeVisible(onoff);
         break;

      case cHUDItemPowerStatus:
         mpUIContext->setPowerPanelVisible(onoff);
         break;

      case cHUDItemUnits:
         {
            mpUIContext->setUnitSelectionVisible(onoff);
            mpUIContext->setUnitCardVisible(onoff);
         }
         break;

      case cHUDItemDpadHelp:
         mpUIContext->setDPadPanelVisible(onoff);
         break;

      case cHUDItemButtonHelp:
         mpUIContext->setButtonPanelVisible(onoff);
         break;

      case cHUDItemReticle:
         mpUIContext->setReticleVisible(onoff);
         break;

      case cHUDItemScore:
         mpUIContext->setScoresVisible(onoff);
         break;

      case cHUDItemUnitStats:
         mpUIContext->setUnitStatsVisible(onoff);
         break;

      case cHUDItemCircleMenuExtraInfo:
         mpUIContext->setCircleMenuExtraInfoVisible(onoff);
         break;
   }
}


//==============================================================================
//==============================================================================
BPowerUser* BUser::createPowerUser(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BEntityID ownerSquadID, bool noCost /*= false*/)
{
   BPowerUser* pNewPowerUser = NULL;

   // Attempt to create the pNewPowerUser...
//-- FIXING PREFIX BUG ID 5909
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
   if (pProtoPower)
   {
      // Must be in normal mode to invoke a power user.
      setUIProtoPowerID(protoPowerID);
      setUIPowerRadius(pProtoPower->getUIRadius());
      changeMode(cUserModeNormal);

      BPowerType powerType = pProtoPower->getPowerType();
      pNewPowerUser = allocatePowerUser(powerType);

      // If we were successful, set the ID, and send the command across the wire so the sim-component of the power can be created for all...
      if (pNewPowerUser)
      {
         // Set our ID.
         BPowerUserID newPowerUserID(mPlayerID, powerType, mNextPowerUserRefCount++);
         pNewPowerUser->setID(newPowerUserID);

         // Set our type.
         pNewPowerUser->setType(powerType);

         // Link to us.
         if (!pNewPowerUser->init(protoPowerID, powerLevel, this, ownerSquadID, getHoverPoint(), noCost))
         {
            gUI.playCantDoSound();
            HEAP_DELETE(pNewPowerUser, gSimHeap);
            return NULL;
         }
      }
   }

   return (pNewPowerUser);
}

//==============================================================================
//==============================================================================
BPowerUser* BUser::allocatePowerUser(BPowerType powerType)
{
   BPowerUser* pNewPowerUser = NULL;
   switch (powerType)
   {
      case PowerType::cCleansing:
         pNewPowerUser = HEAP_NEW(BPowerUserCleansing, gSimHeap);
         break;
      case PowerType::cOrbital:
         pNewPowerUser = HEAP_NEW(BPowerUserOrbital, gSimHeap);
         break;
      case PowerType::cCarpetBombing:
         pNewPowerUser = HEAP_NEW(BPowerUserCarpetBombing, gSimHeap);
         break;
      case PowerType::cCryo:
         pNewPowerUser = HEAP_NEW(BPowerUserCryo, gSimHeap);
         break;
      case PowerType::cWave:
         pNewPowerUser = HEAP_NEW(BPowerUserWave, gSimHeap);
         break;
      case PowerType::cRage:
         pNewPowerUser = HEAP_NEW(BPowerUserRage, gSimHeap);
         break;
      case PowerType::cDisruption:
         pNewPowerUser = HEAP_NEW(BPowerUserDisruption, gSimHeap);
         break;
      case PowerType::cRepair:
         pNewPowerUser = HEAP_NEW(BPowerUserRepair, gSimHeap);
         break;
      case PowerType::cTransport:
         pNewPowerUser = HEAP_NEW(BPowerUserTransport, gSimHeap);
         break;
      case PowerType::cODST:
         pNewPowerUser = HEAP_NEW(BPowerUserODST, gSimHeap);
         break;
         // Add more here:
      default:
         BFAIL("BUser::allocatePowerUser() - Unsupported PowerType specified.");
         break;
   }
   return (pNewPowerUser);
}

//==============================================================================
//==============================================================================
void BUser::deletePowerUser()
{
   if (!mpPowerUser)
      return;

   HEAP_DELETE(mpPowerUser, gSimHeap);
   mpPowerUser = NULL;
}


//==============================================================================
//==============================================================================
void BUser::clearCameraBoudnaryLines()
{
   mCameraBoundaryLines.clear();
}


//==============================================================================
//==============================================================================
void BUser::addCameraBoundaryLine(BCameraBoundaryLine line)
{
   mCameraBoundaryLines.add(line);
}


#define MAX_CONTRAINTS_RESOLVE_ITERATIONS    5

//==============================================================================
// BUser::resolvePositionConstraints2D
//
// Given the last camera and hover point positions and the (current) desired camera and hover point positions, check
// if these positions have cross any of there corresponding boundaries.  If they have, then the function will return
// true, along with new valid positions.
//==============================================================================
bool BUser::resolvePositionConstraints2D(BVector lastCameraPos, BVector lastHoverPointPos, BVector curCameraPos, BVector curHoverPointPos, BVector &newCameraPos, BVector &newHoverPointPos, bool &bNoSolution)
{
   // Zero out all y components
   lastCameraPos.y = lastHoverPointPos.y = curCameraPos.y = curHoverPointPos.y = 0.0f;

   // Computer camera direction and zoom distance
   BVector cameraDir = curHoverPointPos - curCameraPos;
   cameraDir.normalize();
   float zoomDist = curCameraPos.distance(curHoverPointPos);


   bool bPositionsChanged = false;


   BVector tempCameraPos = curCameraPos;
   BVector tempHoverPointPos = curHoverPointPos;

   // Constrain camera pos movement
   if(constrainMovementToBoundaryLines2D(lastCameraPos, tempCameraPos, tempCameraPos, true, BCameraBoundaryLine::cBoundaryCamera, false))
   {
      // Compute new hover point
      tempHoverPointPos = tempCameraPos + (cameraDir * zoomDist);

      bPositionsChanged = true;
   }



   // Loop until constrains are all met
   int count = 0;

   bool bContinueProcessing = false;
   do
   {
      if(count % 2)
      {   
         // Constrain camera pos movement
         if(constrainMovementToBoundaryLines2D(lastCameraPos, tempCameraPos, tempCameraPos, true, BCameraBoundaryLine::cBoundaryCamera, false))
         {
            bContinueProcessing = true;
            bPositionsChanged = true;

            // Compute new hover point
            tempHoverPointPos = tempCameraPos + (cameraDir * zoomDist);
         }         
         else
         {
            bContinueProcessing = false;
         }
      }
      else
      {
         // Constrain hover point pos movement
         if(constrainMovementToBoundaryLines2D(lastHoverPointPos, tempHoverPointPos, tempHoverPointPos, true, BCameraBoundaryLine::cBoundaryHover, false))
         {
            bContinueProcessing = true;
            bPositionsChanged = true;

            // Compute new camera pos
            tempCameraPos = tempHoverPointPos - (cameraDir * zoomDist);
         }         
         else
         {
            bContinueProcessing = false;
         }
      }

      count++;
   }
   while(bContinueProcessing && (count < MAX_CONTRAINTS_RESOLVE_ITERATIONS));



   //BASSERTM(count < MAX_CONTRAINTS_RESOLVE_ITERATIONS, "BUser::resolvePositionConstraints: Unable to find a camera position that meets all constraints");
   if(count >= MAX_CONTRAINTS_RESOLVE_ITERATIONS)
   {
      // Could not find a better position, prohibit movement
      bNoSolution = true;
      return true;
   }
   else if(bPositionsChanged)
   {
      // Use newly computed position
      newCameraPos = tempCameraPos;
      newHoverPointPos = tempHoverPointPos;
      bNoSolution = false;
      return true;
   }
   else
   {
      return false;
   }
}


//==============================================================================
// BUser::resolvePositionTeleportConstraints2D
//
// Given the last camera and hover point positions and the (current) desired camera and hover point positions, check
// if these positions have cross any of there corresponding boundaries.  If they have, then the function will return
// true, along with new valid positions.
//==============================================================================
bool BUser::resolvePositionTeleportConstraints2D(BVector lastCameraPos, BVector lastHoverPointPos, BVector curCameraPos, BVector curHoverPointPos, BVector &newCameraPos, BVector &newHoverPointPos, bool &bNoSolution)
{
   // Zero out all y components
   lastCameraPos.y = lastHoverPointPos.y = curCameraPos.y = curHoverPointPos.y = 0.0f;

   // Computer camera direction and zoom distance
   BVector cameraDir = curHoverPointPos - curCameraPos;
   cameraDir.normalize();
   float zoomDist = curCameraPos.distance(curHoverPointPos);


   BVector tempCameraPos = curCameraPos;
   BVector tempHoverPointPos = curHoverPointPos;


   // Constrain HoverPoint translation from the previous position to the new teleported position.
   // Allow crossing of boundary lines as long as number of crossing is even, meaning that it has
   // gone from a valid position into another valid position.
   //
   if(constrainMovementToBoundaryLines2D(lastHoverPointPos, curHoverPointPos, tempHoverPointPos, false, BCameraBoundaryLine::cBoundaryHover, true))
   {
      // Prohibit movement if we can't get to desired position exactly
      bNoSolution = true;
      return true;
   }       

   lastHoverPointPos = curHoverPointPos;

   // Do the same with the translation between the camera positions
   //
   if(constrainMovementToBoundaryLines2D(lastCameraPos, curCameraPos, tempCameraPos, false, BCameraBoundaryLine::cBoundaryCamera, true))
   {
      lastCameraPos = tempCameraPos;

      // Compute new hover point
      tempHoverPointPos = tempCameraPos + (cameraDir * zoomDist);


      // Loop until constrains are all met
      //
      int count = 0;
      bool bContinueProcessing = false;
      do
      {
         if(count % 2)
         {   
            // Ensure segment from hover point pos to camera pos isn't intersected
            if(constrainMovementToBoundaryLines2D(lastCameraPos, tempCameraPos, tempCameraPos, true, BCameraBoundaryLine::cBoundaryCamera, false))
            {
               lastCameraPos = tempCameraPos;

               // Compute new hover point
               tempHoverPointPos = tempCameraPos + (cameraDir * zoomDist);
               bContinueProcessing = true;
            }         
            else
            {
               bContinueProcessing = false;
            }
         }
         else
         {
            // Constrain hover point pos movement
            if(constrainMovementToBoundaryLines2D(lastHoverPointPos, tempHoverPointPos, tempHoverPointPos, true, BCameraBoundaryLine::cBoundaryHover, false))
            {
               lastHoverPointPos = tempHoverPointPos;

               // Compute new camera pos
               tempCameraPos = tempHoverPointPos - (cameraDir * zoomDist);
               bContinueProcessing = true;
            }         
            else
            {
               bContinueProcessing = false;
            }
         }

         count++;
      }
      while(bContinueProcessing && (count < MAX_CONTRAINTS_RESOLVE_ITERATIONS));



      //BASSERTM(count < MAX_CONTRAINTS_RESOLVE_ITERATIONS, "BUser::resolvePositionConstraints: Unable to find a camera position that meets all constraints");
      if(count >= MAX_CONTRAINTS_RESOLVE_ITERATIONS)
      {
         // Could not find a better position, prohibit movement
         bNoSolution = true;
      }
      else
      {
         // Use newly computed position
         newCameraPos = tempCameraPos;
         newHoverPointPos = tempHoverPointPos;
         bNoSolution = false;
      }
      return true;
   }

   return false;
}


//==============================================================================
// BUser::constrainMovementToBoundaryLines2D
//
// Given two positions, a previous position and a current (desired) position, and assuming that the
// previous positions is within the valid bounds, check if the movement intersects the boundary lines.  
// If it does, then compute a new extrapolated position that does not violate the constraints.
//==============================================================================
bool BUser::constrainMovementToBoundaryLines2D(BVector lastPosition, BVector curPosition, BVector &newPosition, bool bExtrapolate, BCameraBoundaryLine::BCameraBoundaryType type, bool bAllowEvenCrossing)
{
   BVector offsetPoint0, offsetPoint1;

   offsetPoint0 = lastPosition;
   offsetPoint1 = curPosition;

   offsetPoint0.y = 0.0f;
   offsetPoint1.y = 0.0f;


#ifdef ENABLE_CAMERA_OUT_OF_BOUNDS_POTENTIAL_FIX
   // Extend offsetPoint1 by the cCameraLineLimitEpsilon distance
   BVector offsetVec = offsetPoint1 - offsetPoint0;
   offsetVec.safeNormalize();
   offsetPoint1 += offsetVec * cCameraLineLimitEpsilon;
#endif


   // First Pass -
   // ============
   // Find the closest intersection to all line segments
   //

   BVector firstIntersectionPoint(0.0f, 0.0f, 0.0f);
   long firstIntersectionLine = -1;
   long firstIntersectionSegment = -1;

   bool bIntersected = false;
   if(bAllowEvenCrossing)
   {
      bIntersected = intersectSegmentToBoundaryLinesUnevenOnly2D(offsetPoint0, offsetPoint1, type, firstIntersectionPoint, firstIntersectionLine, firstIntersectionSegment);
   }
   else
   {
      bIntersected = intersectSegmentToBoundaryLines2D(offsetPoint0, offsetPoint1, type, -1, -1, firstIntersectionPoint, firstIntersectionLine, firstIntersectionSegment);
   }
      

   // If nothing is intersected then there nothing to do here, early out
   if(!bIntersected)
      return false;

   // Offset intersection point by some epsilon away from the segment it collided with, this is important
   // or else the new position will be on the line and next frame it my cross over it.

   /*
   BVector camDir = offsetPoint1 - offsetPoint0;
   camDir.normalize();

   BVector solution = firstIntersectionPoint + (camDir * (-cCameraLineLimitEpsilon));
   */

   BVector solution;
   computeValidOffsetPositionFromLine(offsetPoint0, firstIntersectionPoint, firstIntersectionLine, firstIntersectionSegment, type, solution);

   if(bExtrapolate)
   {
      // Since we have intersected compute the extrapolated position along the intersected segment
      BVector extrapolatedPos;

      const BCameraBoundaryLine *line;
      BVector line0Point0, line0Point1;

      line = &mCameraBoundaryLines[firstIntersectionLine];
      line0Point0 = line->mPoints[firstIntersectionSegment];
      line0Point1 = line->mPoints[firstIntersectionSegment + 1];


      offsetPoint0 = solution;

      BVector lineVec = line0Point1 - line0Point0;
#ifdef ENABLE_CAMERA_OUT_OF_BOUNDS_POTENTIAL_FIX
      BVector offsetVec = curPosition - firstIntersectionPoint;
#else
      BVector offsetVec = offsetPoint1 - offsetPoint0;
#endif

      lineVec.normalize();
      float mag = offsetVec.dot(lineVec);

      extrapolatedPos = (lineVec * mag) + offsetPoint0;



      // Second Pass -   
      // ============
      // Now check if the new segment from the intersection position to the extrapolated position intersects
      // with any other segment.  Find the closets instersection here and this will be our final answer.
      //

      offsetPoint0 = solution;
      offsetPoint1 = extrapolatedPos;

#ifdef ENABLE_CAMERA_OUT_OF_BOUNDS_POTENTIAL_FIX
      // Extend offsetPoint1 by the cCameraLineLimitEpsilon distance
      BVector offsetVec2 = offsetPoint1 - offsetPoint0;
      offsetVec2.safeNormalize();
      offsetPoint1 += offsetVec2 * cCameraLineLimitEpsilon;
#endif

      BVector secondIntersectionPoint(0.0f, 0.0f, 0.0f);
      long secondIntersectionLine = -1;
      long secondIntersectionSegment = -1;

      bIntersected = intersectSegmentToBoundaryLines2D(offsetPoint0, offsetPoint1, type, firstIntersectionLine, firstIntersectionSegment, secondIntersectionPoint, secondIntersectionLine, secondIntersectionSegment);

      // If nothing is intersected then there nothing to do here, early out
      if(!bIntersected)
      {
#ifdef ENABLE_CAMERA_OUT_OF_BOUNDS_POTENTIAL_FIX
         solution = extrapolatedPos;
#else
         solution = offsetPoint1;
#endif
      }
      else
      {
         // Offset intersection point by some epsilon away from the segment it collided with
         /*
         BVector camDir = offsetPoint1 - offsetPoint0;
         camDir.normalize();

         solution = secondIntersectionPoint + (camDir * (-cCameraLineLimitEpsilon));
         */

         BVector point0 = offsetPoint0;
         BVector closestPointInLine;

         xzClosestsPointInLine(point0, line0Point0, line0Point1, closestPointInLine);

         BVector offsetVec0 = point0 - closestPointInLine;
         if(offsetVec0.safeNormalize())
         {
            offsetVec0.scale(cCameraLineLimitEpsilon);
         }
         else
         {
            BASSERTM(false, "BUser::computeValidOffsetPositionFromLine - How did we get here?");
         }


         BVector line1Point0, line1Point1;

         line = &mCameraBoundaryLines[secondIntersectionLine];
         line1Point0 = line->mPoints[secondIntersectionSegment];
         line1Point1 = line->mPoints[secondIntersectionSegment + 1];

         xzClosestsPointInLine(point0, line1Point0, line1Point1, closestPointInLine);   

         BVector offsetVec1 = point0 - closestPointInLine;
         if(offsetVec1.safeNormalize())
         {
            offsetVec1.scale(cCameraLineLimitEpsilon);
         }
         else
         {
#ifdef ENABLE_CAMERA_OUT_OF_BOUNDS_POTENTIAL_FIX
            // Do not move
            newPosition = lastPosition;
            newPosition.y = 0.0f;

            return true;
#else
            BASSERTM(false, "BUser::computeValidOffsetPositionFromLine - How did we get here?");
#endif
         }

         // Offset lines
         line0Point0 += offsetVec0;
         line0Point1 += offsetVec0;
         line1Point0 += offsetVec1;
         line1Point1 += offsetVec1;

         BVector thirdIntersectionPoint(0.0f, 0.0f, 0.0f);
         if(xzSegmentToSegmentIntersect(line0Point0, line0Point1, line1Point0, line1Point1, thirdIntersectionPoint))
         {
            solution = thirdIntersectionPoint;
         }      
         else
         {
            computeValidOffsetPositionFromLine(offsetPoint0, secondIntersectionPoint, secondIntersectionLine, secondIntersectionSegment, type, solution);
         }
      }
   }

   newPosition = solution;
   newPosition.y = 0.0f;

   return true;
}


//==============================================================================
//==============================================================================
bool BUser::intersectSegmentToBoundaryLines2D(BVector point0, BVector point1, BCameraBoundaryLine::BCameraBoundaryType type, long ignorLineIndex, long ignoreLineSegmentIndex, BVector &intersectionPoint, long &intersectionLineIndex, long &intersectionLineSegmentIndex)
{
   point0.y = 0.0f;
   point1.y = 0.0f;


   // Find the closest intersection to all line segments
   BVector closestIntersectionPoint(0.0f, 0.0f, 0.0f);
   float closestIntersectionDistSqr = cMaximumFloat;
   long closestInterectionLine = -1;
   long closestInterectionSegment = -1;


   long numLines = mCameraBoundaryLines.getNumber();
   for(long i = 0; i < numLines; i++)
   {
      const BCameraBoundaryLine *line = &mCameraBoundaryLines[i];

      if(type != line->mType)
         continue;

      long numSegments = line->mPoints.getNumber() - 1;
      for(long j = 0; j < numSegments; j++)
      {
         if((ignorLineIndex == i) && (ignoreLineSegmentIndex == j))
            continue;

         BVector lineSeg0 = line->mPoints[j];
         BVector lineSeg1 = line->mPoints[j + 1];

         BVector intersectionPointTemp;

         if(xzSegmentToSegmentIntersect(point0, point1, lineSeg0, lineSeg1, intersectionPointTemp))
         {
            // Compute square distance
            float distSqr = point0.xzDistanceSqr(intersectionPointTemp);
   
            if(distSqr < closestIntersectionDistSqr)
            {
               closestIntersectionPoint = intersectionPointTemp;
               closestIntersectionDistSqr = distSqr;
               closestInterectionLine = i;
               closestInterectionSegment = j;
            }
         }      
      }
   }

   // If nothing is intersected then there nothing to do here, early out
   if(closestIntersectionDistSqr == cMaximumFloat)
      return false;


   intersectionPoint = closestIntersectionPoint;
   intersectionLineIndex = closestInterectionLine;
   intersectionLineSegmentIndex = closestInterectionSegment;

   return true;
}


//==============================================================================
//==============================================================================
bool BUser::intersectSegmentToBoundaryLinesUnevenOnly2D(BVector point0, BVector point1, BCameraBoundaryLine::BCameraBoundaryType type, BVector &intersectionPoint, long &intersectionLineIndex, long &intersectionLineSegmentIndex)
{
   point0.y = 0.0f;
   point1.y = 0.0f;


   // Find the closest intersection to all line segments
   BVector closestIntersectionPoint(0.0f, 0.0f, 0.0f);
   float closestIntersectionDistSqr = cMaximumFloat;
   long closestInterectionLine = -1;
   long closestInterectionSegment = -1;


   long numLines = mCameraBoundaryLines.getNumber();
   for(long i = 0; i < numLines; i++)
   {
      const BCameraBoundaryLine *line = &mCameraBoundaryLines[i];

      if(type != line->mType)
         continue;

      // For each line find the number of segment instersections keeping track of the farthest intersection.  If the number of instersections
      // is even then nothing is done, since we have moved from one valid location to another.  If it's uneven then we have move to an invalid
      // location, update our closest intersection.
      //
      BVector farthestIntersectionPoint(0.0f, 0.0f, 0.0f);
      float farthestIntersectionDistSqr = 0.0f;
      long farthestInterectionSegment = -1;

      long numIntesections = 0;
      long numSegments = line->mPoints.getNumber() - 1;
      for(long j = 0; j < numSegments; j++)
      {
         BVector lineSeg0 = line->mPoints[j];
         BVector lineSeg1 = line->mPoints[j + 1];

         BVector intersectionPointTemp;

         if(xzSegmentToSegmentIntersect(point0, point1, lineSeg0, lineSeg1, intersectionPointTemp))
         {
            numIntesections++;

            // Compute square distance
            float distSqr = point0.xzDistanceSqr(intersectionPointTemp);
   
            if(distSqr > farthestIntersectionDistSqr)
            {
               farthestIntersectionPoint = intersectionPointTemp;
               farthestIntersectionDistSqr = distSqr;
               farthestInterectionSegment = j;
            }
         }      
      }

      if(numIntesections % 2)
      {
         if(farthestIntersectionDistSqr < closestIntersectionDistSqr)
         {
            closestIntersectionPoint = farthestIntersectionPoint;
            closestIntersectionDistSqr = farthestIntersectionDistSqr;
            closestInterectionLine = i;
            closestInterectionSegment = farthestInterectionSegment;
         }
      }
   }

   // If nothing is intersected then there nothing to do here, early out
   if(closestIntersectionDistSqr == cMaximumFloat)
      return false;


   intersectionPoint = closestIntersectionPoint;
   intersectionLineIndex = closestInterectionLine;
   intersectionLineSegmentIndex = closestInterectionSegment;

   return true;
}


//==============================================================================
//==============================================================================
void BUser::computeValidOffsetPositionFromLine(BVector point0, BVector intersectionPoint, long intersectedLine, long intersectedSegment, BCameraBoundaryLine::BCameraBoundaryType type, BVector &offsetPoint)
{
   // Given an instersection point and a line segment where we have intersected, compute a new position that
   // is offseted away from the interesected line by a given episilon.  To ensure that this new position does
   // not cross any boundary lines, this function will intersect the offseted vector with all other lines.  If
   // an intersection is found here, it means that these lines intersect  The result then is the intersection
   // of both lines offseted inwards.
   //
   BVector closestPointInLine;
   const BCameraBoundaryLine *line;

   BVector line0Point0, line0Point1;

   line = &mCameraBoundaryLines[intersectedLine];
   line0Point0 = line->mPoints[intersectedSegment];
   line0Point1 = line->mPoints[intersectedSegment + 1];

   xzClosestsPointInLine(point0, line0Point0, line0Point1, closestPointInLine);

   BVector offsetVec0 = point0 - closestPointInLine;
   if(offsetVec0.safeNormalize())
   {
      offsetVec0.scale(cCameraLineLimitEpsilon);

      offsetPoint = intersectionPoint + offsetVec0;
   }
   else
   {
      BASSERTM(false, "BUser::computeValidOffsetPositionFromLine - How did we get here?");
   }

   // Test to see if the segement from the instersectionPoint to the offsetPoint collides with any other lines
   //
   BVector intersectionPoint1;
   if(intersectSegmentToBoundaryLines2D(intersectionPoint, offsetPoint, type, intersectedLine, intersectedSegment, intersectionPoint1, intersectedLine, intersectedSegment))
   {
      // Since we have collided it means that these two lines intersect.  Offset both lines towards the intersection 
      // and intersect them.  Use the intersection as the resulting offset point.
      BVector line1Point0, line1Point1;

      line = &mCameraBoundaryLines[intersectedLine];
      line1Point0 = line->mPoints[intersectedSegment];
      line1Point1 = line->mPoints[intersectedSegment + 1];

      xzClosestsPointInLine(point0, line1Point0, line1Point1, closestPointInLine);   

      BVector offsetVec1 = point0 - closestPointInLine;
      if(offsetVec1.safeNormalize())
      {
         offsetVec1.scale(cCameraLineLimitEpsilon);
      }
      else
      {
         BASSERTM(false, "BUser::computeValidOffsetPositionFromLine - How did we get here?");
      }

      // Offset lines
      line0Point0 += offsetVec0;
      line0Point1 += offsetVec0;
      line1Point0 += offsetVec1;
      line1Point1 += offsetVec1;


      if(xzSegmentToSegmentIntersect(line0Point0, line0Point1, line1Point0, line1Point1, intersectionPoint))
      {
         offsetPoint = intersectionPoint;
      }      
      else
      {
         BASSERTM(false, "BUser::computeValidOffsetPositionFromLine - How did we get here?");
      }
   }
}

//==============================================================================
// BUser::computeClosestCameraHoverPointVertical
//
// Given a XZ position compute the closest point planted in the camera height field.
//==============================================================================
void BUser::computeClosestCameraHoverPointVertical(BVector position, BVector &newCameraHoverPoint)
{
   BVector intersectionPt(position);
   gTerrainSimRep.getCameraHeightRaycast(intersectionPt, intersectionPt.y, true);
   intersectionPt.y += mCameraHoverPointOffsetHeight;

   newCameraHoverPoint = intersectionPt;
}

//==============================================================================
// BUser::computeClosestCameraHoverPointInDirection
//
// Given a position and direction, compute the closest point planted in the camera
// height field.
//==============================================================================
void BUser::computeClosestCameraHoverPointInDirection(BVector lookAtPosition, BVector lookAtDirection, BVector &newCameraHoverPoint)
{
   BVector intersectionPt;
   if (gTerrainSimRep.rayIntersectsCamera(lookAtPosition, lookAtDirection, intersectionPt))
   {
      newCameraHoverPoint.set(intersectionPt.x, intersectionPt.y, intersectionPt.z);
      newCameraHoverPoint.y += mCameraHoverPointOffsetHeight;
   }
   else
   {
      // try opposite direction now
      if (gTerrainSimRep.rayIntersectsCamera(lookAtPosition, lookAtDirection * -1.0, intersectionPt))
      {
         newCameraHoverPoint.set(intersectionPt.x, intersectionPt.y, intersectionPt.z);
         newCameraHoverPoint.y += mCameraHoverPointOffsetHeight;
      }
      else
      {
         // Collide against ZX plane at 0 height
         float dirFactor = 1.0f;
         if(lookAtPosition.y < 0)
            dirFactor = -1.0f;
         else
            dirFactor = 1.0f;


         if (raySegmentIntersectionPlane(cOriginVector, cYAxisVector, lookAtPosition, lookAtDirection * dirFactor, false, intersectionPt, 0.01f))
         {
            newCameraHoverPoint.set(intersectionPt.x, intersectionPt.y, intersectionPt.z);
            newCameraHoverPoint.y += mCameraHoverPointOffsetHeight;
         }
         else
         {
            BASSERT(false);
         }
      }
   }
}


//==============================================================================
//==============================================================================
bool BUser::canPlayAbilitySound(long abilityID, const BSquad* pSquad, BProtoAction** returnAction)
{
   if(returnAction != NULL)
      *returnAction = NULL;

   if(!pSquad)
      return false;

   if(abilityID == -1)
      return false;

   if(!pSquad->getProtoObject())
      return false;

  //-- Can we play the ability sound?  
//-- FIXING PREFIX BUG ID 5836
   const BAbility* pAbility = gDatabase.getAbilityFromID(pSquad->getProtoObject()->getAbilityCommand());
//--
   if(pAbility)
   {
      int targetType = pAbility->getTargetType();
      if((targetType == BAbility::cTargetUnit) && (mHoverType != cHoverTypeEnemy) && (mHoverType != cHoverTypeAbility))
      {
         return false;
      }
      else if(pSquad->getRecoverType() == cRecoverAbility && pSquad->getRecoverTime() > 0) //-- Make sure that the dude's timer isn't running
      {                     
         return false;
      }
      else if(pSquad->getProtoObject()->getFlagAbilityDisabled())
      {
         return false;
      }

      // Only return true if we can actually do the ability
      BAbilityID actionAbilityID;
      BProtoAction* pProtoAction = pSquad->getProtoActionForTarget(mHoverObject, mHoverPoint, abilityID, false, NULL, false, &actionAbilityID);
      if(pProtoAction && (actionAbilityID == abilityID))
      {
         //Hack - if we are a cobra that is already locked down, do not play the ability ack because that says "locking down".  
         if (pSquad->getProtoSquadID() == gDatabase.getPSIDCobra() && (pSquad->getSquadMode() == BSquadAI::cModeLockdown))
            return false;

         //More Hacks! - if we are a vampire, only play the ability ack if we are hovered over an enemy flying unit.  This doesn't catch all of the cases - 
         //you can target the ground or other units near flying enemy units and the vampires will search for a target.
         if (pSquad->getProtoSquadID() == gDatabase.getPSIDVampire())
         {
            if(mHoverType != cHoverTypeEnemy) 
               return false;

            BUnit* pUnit = gWorld->getUnit(getHoverObject());
            if( !pUnit || !pUnit->getFlagFlying() )
               return false;
         }

         if(returnAction != NULL)
            *returnAction = pProtoAction;
         return true;
      }
      else
      {
         //No protoaction based on the target, but what if this ability doesn't require a target (banshee boost).  If it only requires a location, play the ability sound.
         if( (pAbility->getTargetType() == BAbility::cTargetUnitOrLocation) || (pAbility->getTargetType() == BAbility::cTargetLocation) )
            return true;
         else
            return false;
      }
   }
   return false;
}


//============================================================================
//============================================================================
void BUser::cancelPower()
{
   if( mpPowerUser )
   {
      // I have to call cancelPower twice here because the Transport power may
      // be in a state where a single cancel won't shut it down.
      mpPowerUser->cancelPower();
      mpPowerUser->cancelPower();
      deletePowerUser();
   }
}


//==============================================================================
//==============================================================================
// Helper functions for restricting the camera to arbitrary boundary lines
//


//==============================================================================
//==============================================================================
bool xzSegmentToSegmentIntersect(BVector lnA0, BVector lnA1, BVector lnB0, BVector lnB1, BVector &intersectionPoint)
{
   /*
   * Notes:
   * The denominators for the equations for ua and ub are the same.
   * If the denominator for the equations for ua and ub is 0 then the two lines are parallel.
   * If the denominator and numerator for the equations for ua and ub are 0 then the two lines are coincident.
   * The equations apply to lines, if the intersection of line segments is required then it is only necessary to test 
   * if ua and ub lie between 0 and 1. 
   * Whichever one lies within that range then the corresponding line segment contains the intersection point. 
   * If both lie within the range of 0 to 1 then the intersection point is within both line segments. 
   */

   float denom = ((lnB1.z - lnB0.z) * (lnA1.x - lnA0.x)) -
      ((lnB1.x - lnB0.x) * (lnA1.z - lnA0.z));

   float nume_a = ((lnB1.x - lnB0.x) * (lnA0.z - lnB0.z)) -
      ((lnB1.z - lnB0.z) * (lnA0.x - lnB0.x));

   float nume_b = ((lnA1.x - lnA0.x) * (lnA0.z - lnB0.z)) -
      ((lnA1.z - lnA0.z) * (lnA0.x - lnB0.x));

   if(denom == 0.0f)
   {
      if(nume_a == 0.0f && nume_b == 0.0f)
      {
         // lines are coicident
         intersectionPoint.x = lnA0.x;
         intersectionPoint.y = 0.0f;
         intersectionPoint.z = lnA0.z;
         intersectionPoint.w = 0.0f;
         return true;
      }
      return false;
   }

   float ua = nume_a / denom;
   float ub = nume_b / denom;

   if(ua >= 0.0f && ua <= 1.0f && ub >= 0.0f && ub <= 1.0f)
   {
      // Get the intersection point.
      intersectionPoint.x = lnA0.x + ua * (lnA1.x - lnA0.x);
      intersectionPoint.y = 0.0f;
      intersectionPoint.z = lnA0.z + ua * (lnA1.z - lnA0.z);
      intersectionPoint.w = 0.0f;

      return true;
   }

   return false;
}


//==============================================================================
//==============================================================================
void xzClosestsPointInLine(BVector point, BVector lnB0, BVector lnB1, BVector &closestPoint)
{
   // Find closets point in line segment
   //
   BVector closestPointInSegment;

   BVector segmentDir = lnB1 - lnB0;
   segmentDir.normalize();

   float d = point.dot(segmentDir) - lnB0.dot(segmentDir);

   closestPoint = lnB0 + (d * segmentDir);
}


//==============================================================================
//==============================================================================
void BUser::updateFriends()
{
   HANDLE enumerateHandle;
   DWORD  bufferSize;
   DWORD result = XFriendsCreateEnumerator(getPort(), 0, MAX_FRIENDS, &bufferSize, &enumerateHandle);
   if (result != ERROR_SUCCESS )
   {
      //list was empty or some other issue
      return;
   }

   XONLINE_FRIEND * friends = 0;
   DWORD itemCount = bufferSize / sizeof(XONLINE_FRIEND);
   friends = new XONLINE_FRIEND[itemCount];
   result = XEnumerate(enumerateHandle, friends, bufferSize, &itemCount, NULL);

   int friendsOnline = 0;
   int friendsOnlinePlayingHaloWars = 0;

   mFriends.clear();
   
   if( result == ERROR_SUCCESS )
   {
      unsigned int i = 0;
      for(i = 0; i < itemCount; ++i)
      {
         mFriends.add(friends[i]);
         if(friends[i].dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE && 
            // JAR : Fix PHX-14545 : "Ticker shows pending Friend Requests as actual friends"
            // XDK docs indicate that these flags will be cleared once a pending 
            // friends request has been accepted. Exclude pending friends from
            // the display/count.
            !(friends[i].dwFriendState & XONLINE_FRIENDSTATE_FLAG_SENTREQUEST || 
               friends[i].dwFriendState & XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST
              )
            )
         {
            ++friendsOnline;
            if(friends[i].dwTitleID == TITLEID_HALO_WARS)
            {
               if(friendsOnlinePlayingHaloWars < cMaxGamerTagTickers)
               {
                  BUString friendOnline;
                  friendOnline.locFormat(gDatabase.getLocStringFromID(25990).getPtr(), friends[i].szGamertag);
                  //friendOnline.format("%s is online and playing Halo Wars!", friends[i].szGamertag);
                  BUITicker::addString(friendOnline, 2, -1, friends[i].xuid);
               }
               else
               {
                  // This will remove the gamertag string from the ticker if it already exists
                  BUITicker::addString( L"", 2, 0, friends[i].xuid);
               }

               ++friendsOnlinePlayingHaloWars;
            }
            else
            {
               // This will remove the gamertag string from the ticker if it already exists
               BUITicker::addString( L"", 2, 0, friends[i].xuid);
            }
         }
         else
         {
            // This will remove the gamertag string from the ticker if it already exists
            BUITicker::addString( L"", 2, 0, friends[i].xuid);
         }
      }
   }

   BUString testingOnline = "";
   BUString testingPlayingHaloWars = "";
   testingOnline.locFormat(gDatabase.getLocStringFromID(25245), friendsOnline);
   testingPlayingHaloWars.locFormat(gDatabase.getLocStringFromID(25246), friendsOnlinePlayingHaloWars);
   BUITicker::addString(testingOnline, 2, -1, BUITicker::eFriendsOnline);
   BUITicker::addString(testingPlayingHaloWars, 2, -1, BUITicker::eFriendsPlayingHW);

   CloseHandle( enumerateHandle );
   delete[] friends;
}

//==============================================================================
//==============================================================================
const BDynamicArray<XONLINE_FRIEND> & BUser::getFriends() const
{
   return mFriends;
}

//==============================================================================
//==============================================================================
bool BUser::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, mUserMode);
   GFWRITEVAR(pStream, long, mSubMode);

   // VAT: 11/11/08: to avoid potential dangerous side effects of serializing these
   // variables this late in the game, we only write these if we're in the power user mode 
   if (mUserMode == cUserModePower)
   {
      GFWRITEVAR(pStream, float, mUIPowerRadius);
      GFWRITEVAR(pStream, int, mUIProtoPowerID);
      GFWRITEVAR(pStream, float, mUIModeRestoreCameraZoomMin);
      GFWRITEVAR(pStream, float, mUIModeRestoreCameraZoomMax);
      GFWRITEVAR(pStream, float, mUIModeRestoreCameraZoom);
      GFWRITEVAR(pStream, float, mUIModeRestoreCameraPitchMin);
      GFWRITEVAR(pStream, float, mUIModeRestoreCameraPitchMax);
      GFWRITEVAR(pStream, float, mUIModeRestoreCameraPitch);
      GFWRITEVAR(pStream, float, mUIModeRestoreCameraYaw);
      GFWRITEBITBOOL(pStream, mFlagUIModeRestoreCameraZoomMin);
      GFWRITEBITBOOL(pStream, mFlagUIModeRestoreCameraZoomMax);
      GFWRITEBITBOOL(pStream, mFlagUIModeRestoreCameraZoom);
      GFWRITEBITBOOL(pStream, mFlagUIModeRestoreCameraPitchMin);
      GFWRITEBITBOOL(pStream, mFlagUIModeRestoreCameraPitchMax);
      GFWRITEBITBOOL(pStream, mFlagUIModeRestoreCameraPitch);
      GFWRITEBITBOOL(pStream, mFlagUIModeRestoreCameraYaw);
      GFWRITEBITBOOL(pStream, mFlagCameraScrollEnabled);
      GFWRITEBITBOOL(pStream, mFlagCameraYawEnabled);
      GFWRITEBITBOOL(pStream, mFlagCameraZoomEnabled);
      GFWRITEBITBOOL(pStream, mFlagCameraAutoZoomInstantEnabled);
      GFWRITEBITBOOL(pStream, mFlagCameraAutoZoomEnabled);
      GFWRITEBITBOOL(pStream, mFlagRestoreCameraEnableUserScroll);
      GFWRITEBITBOOL(pStream, mFlagRestoreCameraEnableUserYaw);
      GFWRITEBITBOOL(pStream, mFlagRestoreCameraEnableUserZoom);      
      GFWRITEBITBOOL(pStream, mFlagRestoreCameraEnableAutoZoomInstant);
      GFWRITEBITBOOL(pStream, mFlagRestoreCameraEnableAutoZoom);

      BPowerType powerType = (mpPowerUser ? mpPowerUser->getType() : PowerType::cInvalid);
      GFWRITEVAR(pStream, BPowerType, powerType);
      if (powerType != PowerType::cInvalid)
         GFWRITECLASSPTR(pStream, saveType, mpPowerUser);
   }

   const BEntityIDArray& selectionList = mSelectionManager->getSelectedSquads();
   GFWRITEARRAY(pStream, BEntityID, selectionList, uint16, 1000);

   GFWRITEVAR(pStream, float, mCameraZoomMin);
   GFWRITEVAR(pStream, float, mCameraZoomMax);
   GFWRITEVAR(pStream, float, mCameraPitchMin);
   GFWRITEVAR(pStream, float, mCameraPitchMax);
   GFWRITEVAR(pStream, float, mCameraPitch);
   GFWRITEVAR(pStream, float, mCameraYaw);
   GFWRITEVAR(pStream, float, mCameraZoom);
   GFWRITEVAR(pStream, float, mCameraFOV);
   GFWRITEVECTOR(pStream, mHoverPoint);
   GFWRITEVECTOR(pStream, mCameraHoverPoint);
   GFWRITEVAR(pStream, float, mCameraHoverPointOffsetHeight);
   GFWRITEVECTOR(pStream, mLastCameraLoc);
   GFWRITEVECTOR(pStream, mLastCameraHoverPoint);
   GFWRITEBITBOOL(pStream, mFlagHaveHoverPoint);
   GFWRITEBITBOOL(pStream, mFlagHoverPointOverTerrain);

   GFWRITEPTR(pStream, sizeof(bool)*cNumberHUDItems, mHUDItemEnabled);

   uint numArrows = mObjectiveArrowList.getSize();
   GFWRITEVAR(pStream, uint, numArrows);
   for (uint i = 0; i < numArrows; i++)
   {
      BObjectiveArrow* pObjectiveArrow = mObjectiveArrowList[i];
      bool haveArrow = (pObjectiveArrow != NULL);
      GFWRITEVAR(pStream, bool, haveArrow);
      if (haveArrow)
      {
         GFWRITECLASSPTR(pStream, saveType, pObjectiveArrow);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUser::load(BStream* pStream, int saveType)
{
   if (mGameFileVersion >= 4)
   {
      long userMode, subMode;
      GFREADVAR(pStream, long, userMode);
      GFREADVAR(pStream, long, subMode);

      // VAT: 11/11/08: to avoid potential dangerous side effects of serializing these
      // variables this late in the game, we only read these in if we're the in the power
      // user mode. This matches the previous behavior of defaulting to the normal user mode
      // in the case that we saved in a state that wasn't cUserModePower
      if (userMode == cUserModePower)
      {
         mUserMode = userMode;
         mSubMode = subMode;

         GFREADVAR(pStream, float, mUIPowerRadius);
         GFREADVAR(pStream, int, mUIProtoPowerID);
         GFREADVAR(pStream, float, mUIModeRestoreCameraZoomMin);
         GFREADVAR(pStream, float, mUIModeRestoreCameraZoomMax);
         GFREADVAR(pStream, float, mUIModeRestoreCameraZoom);
         GFREADVAR(pStream, float, mUIModeRestoreCameraPitchMin);
         GFREADVAR(pStream, float, mUIModeRestoreCameraPitchMax);
         GFREADVAR(pStream, float, mUIModeRestoreCameraPitch);
         GFREADVAR(pStream, float, mUIModeRestoreCameraYaw);
         GFREADBITBOOL(pStream, mFlagUIModeRestoreCameraZoomMin);
         GFREADBITBOOL(pStream, mFlagUIModeRestoreCameraZoomMax);
         GFREADBITBOOL(pStream, mFlagUIModeRestoreCameraZoom);
         GFREADBITBOOL(pStream, mFlagUIModeRestoreCameraPitchMin);
         GFREADBITBOOL(pStream, mFlagUIModeRestoreCameraPitchMax);
         GFREADBITBOOL(pStream, mFlagUIModeRestoreCameraPitch);
         GFREADBITBOOL(pStream, mFlagUIModeRestoreCameraYaw);
         GFREADBITBOOL(pStream, mFlagCameraScrollEnabled);
         GFREADBITBOOL(pStream, mFlagCameraYawEnabled);
         GFREADBITBOOL(pStream, mFlagCameraZoomEnabled);
         GFREADBITBOOL(pStream, mFlagCameraAutoZoomInstantEnabled);
         GFREADBITBOOL(pStream, mFlagCameraAutoZoomEnabled);
         GFREADBITBOOL(pStream, mFlagRestoreCameraEnableUserScroll);
         GFREADBITBOOL(pStream, mFlagRestoreCameraEnableUserYaw);
         GFREADBITBOOL(pStream, mFlagRestoreCameraEnableUserZoom);      
         GFREADBITBOOL(pStream, mFlagRestoreCameraEnableAutoZoomInstant);
         GFREADBITBOOL(pStream, mFlagRestoreCameraEnableAutoZoom);

         BPowerType powerType;
         GFREADVAR(pStream, BPowerType, powerType);
         if (powerType != PowerType::cInvalid)
         {
            mpPowerUser = allocatePowerUser(powerType);
            GFREADCLASSPTR(pStream, saveType, mpPowerUser);
         }
      }
   }

   BSmallDynamicSimArray<BEntityID> selectionList;
   GFREADARRAY(pStream, BEntityID, selectionList, uint16, 1000);
   mSelectionManager->clearSelections();
   int count = selectionList.getNumber();
   for (int i=0; i<count; i++)
   {
      BSquad* pSquad = gWorld->getSquad(selectionList[i]);
      if (pSquad)
      {
         BUnit* pUnit = pSquad->getLeaderUnit();
         if (pUnit && pUnit->getSelectType(getTeamID())!=cSelectTypeCommand)
            mSelectionManager->selectSquad(selectionList[i]);
      }
   }

   if (mGameFileVersion >= 2)
   {
      GFREADVAR(pStream, float, mCameraZoomMin);
      GFREADVAR(pStream, float, mCameraZoomMax);
      GFREADVAR(pStream, float, mCameraPitchMin);
      GFREADVAR(pStream, float, mCameraPitchMax);
      GFREADVAR(pStream, float, mCameraPitch);
      GFREADVAR(pStream, float, mCameraYaw);
      GFREADVAR(pStream, float, mCameraZoom);
      GFREADVAR(pStream, float, mCameraFOV);
      GFREADVECTOR(pStream, mHoverPoint);
      GFREADVECTOR(pStream, mCameraHoverPoint);
      GFREADVAR(pStream, float, mCameraHoverPointOffsetHeight);
      GFREADVECTOR(pStream, mLastCameraLoc);
      GFREADVECTOR(pStream, mLastCameraHoverPoint);
      GFREADBITBOOL(pStream, mFlagHaveHoverPoint);
      GFREADBITBOOL(pStream, mFlagHoverPointOverTerrain);
      applyCameraSettings(true);
   }

   if (mGameFileVersion >= 3)
      GFREADPTR(pStream, sizeof(bool)*cNumberHUDItems, mHUDItemEnabled)


   if (mGameFileVersion >= 5)
   {
      uint numArrows;
      GFREADVAR(pStream, uint, numArrows);
      for (uint i = 0; i < numArrows; i++)
      {
         bool haveArrow;
         GFREADVAR(pStream, bool, haveArrow);
         if (haveArrow)
         {
            if (i < mObjectiveArrowList.getSize())
            {
               BObjectiveArrow* pObjectiveArrow = mObjectiveArrowList[i];
               if (!pObjectiveArrow)
               {
                  pObjectiveArrow = new BObjectiveArrow();
                  if (!pObjectiveArrow)
                     return false;
                  mObjectiveArrowList[i] = pObjectiveArrow;
               }
               GFREADCLASSPTR(pStream, saveType, pObjectiveArrow);
            }
            else
            {
               GFREADTEMPCLASS(pStream, saveType, BObjectiveArrow);
            }
         }
      }
   }

   clampCamera();
   mFlagUpdateHoverPoint = true;

   // setup the camera out efect if we have an active power
   // this is safe to do, since the cam effects and proto powers have already
   // been loaded
   if (mUserMode == cUserModePower)
   {
      const BProtoPower* pPP = gDatabase.getProtoPowerByID(mUIProtoPowerID);
      if (pPP)
         mCameraEffectTransitionOutIndex = pPP->getCameraEffectOut();
   }

   // post load the power user if we have one
   if (mpPowerUser)
      mpPowerUser->postLoad(saveType);

   return true;
}
