//==============================================================================
// protoobject.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "cost.h"
#include "database.h"
#include "pop.h"
#include "squadAI.h"
#include "xmlreader.h"
#include "hitzone.h"
#include "flashscene.h"

// Forward declarations
class BObject;
class BPlayer;
class BXMLWriter;
class BTactic;
class BHitZone;
class BTerrainImpactDecalHandle;
class BRumbleEvent;

// Constants
const float cMaxVelocityMultiplier=1.5f;
const float cDefaultDeathFadeTime = 1.0f; // In seconds
const float cDefaultDeathFadeDelayTime = 0.0f; // In seconds

//==============================================================================
// BProtoObjectCommand
//==============================================================================
class BProtoObjectCommand
{
   public:
      enum
      {
         cTypeResearch,
         cTypeTrainUnit,
         cTypeBuild,
         cTypeTrainSquad,
         cTypeUnloadUnits,
         cTypeReinforce,
         cTypeChangeMode,
         cTypeAbility,
         cTypeKill,
         cTypeCancelKill,
         cTypeTribute,
         cTypeCustomCommand,
         cTypePower,
         cTypeBuildOther,
         cTypeTrainLock,
         cTypeTrainUnlock,
         cTypeRallyPoint,
         cTypeClearRallyPoint,
         cTypeDestroyBase,
         cTypeCancelDestroyBase,
         cTypeReverseHotDrop,

         cNumberTypes
      };

      void                    set(long val) { mData=val; }
      void                    set(long type, long id, long position) { mData=((type<<24)|(position<<16)|id); }
      long                    getType() const { return((long)(mData>>24)); }
      long                    getID() const { return((long)(mData&0X0000FFFF)); }
      long                    getPosition() const { return((long)((mData&0X00FF0000)>>16)); }

      operator long() { return (long)mData; }
      operator DWORD() { return mData; }

      DWORD                   mData;
};

//==============================================================================
// BProtoObjectTrainLimit
//==============================================================================
class BProtoObjectTrainLimit
{
   public:
      int16                   mID;
      uint8                   mCount;
      uint8                   mBucket:7;
      bool                    mSquad:1;
};
typedef BSmallDynamicSimArray<BProtoObjectTrainLimit> BProtoObjectTrainLimitArray;

//==============================================================================
// BProtoObjectChildObject
//==============================================================================
class BProtoObjectChildObject
{
   public:
      enum 
      {
         cTypeObject,
         cTypeParkingLot,
         cTypeSocket,
         cTypeRally,
         cTypeOneTimeSpawnSquad,
         cTypeUnit,
         cTypeFoundation,
      };
      BProtoObjectChildObject() : mType(cTypeObject), mUserCiv(-1), mID(-1), mOffsetX(0.0f), mOffsetZ(0.0f), mRotation(0.0f) {}

      const BSimString&       getAttachBoneName() const  { return mAttachBoneName; }
      BSimString              mAttachBoneName;
      int8                    mType;
      int8                    mUserCiv;
      int16                   mID;
      float                   mOffsetX;
      float                   mOffsetZ;
      float                   mRotation;
};
typedef BSmallDynamicSimArray<BProtoObjectChildObject> BProtoObjectChildObjectArray;

//==============================================================================
// BProtoObjectLevel
//==============================================================================
class BProtoObjectLevel
{
   public:
      BProtoObjectLevel() :
         mXP(0.0f),
         mDamage(1.0f),
         mVelocity(1.0f),
         mAccuracy(1.0f),
         mWorkRate(1.0f),
         mWeaponRange(1.0f),
         mDamageTaken(1.0f),
         mLevel(0)
      {
      }

      BProtoObjectLevel(const BProtoObjectLevel& source) { *this=source; }

      BProtoObjectLevel& operator=(const BProtoObjectLevel& source)
      {
         if(this==&source)
            return *this;
         mXP=source.mXP;
         mDamage=source.mDamage;
         mVelocity=source.mVelocity;
         mAccuracy=source.mAccuracy;
         mWorkRate=source.mWorkRate;
         mWeaponRange=source.mWeaponRange;
         mDamageTaken=source.mDamageTaken;
         mLevel=source.mLevel;
         return *this;
      }

      BHalfFloat              mXP;
      BHalfFloat              mDamage;
      BHalfFloat              mVelocity;
      BHalfFloat              mAccuracy;
      BHalfFloat              mWorkRate;
      BHalfFloat              mWeaponRange;
      BHalfFloat              mDamageTaken;
      uint8                   mLevel;
};
typedef BSmallDynamicSimArray<BProtoObjectLevel> BProtoObjectLevelArray;

//==============================================================================
// BSingleBoneIKNode
//==============================================================================
class BSingleBoneIKNode
{
public:

   const BSimString& getBoneName() const  { return mBoneName; }

   bool        load(BXMLNode root);

protected:

   BSimString  mBoneName;

};

//==============================================================================
// BGroundIKNode
//==============================================================================
class BGroundIKNode
{
   public:

      const BSimString& getBoneName() const  { return mBoneName; }

      float       getIKRange() const      { return mIKRange; }

      uint8       getLinkCount() const    { return mLinkCount; }

      bool        getFlagOnLeft() const   { return mFlagOnLeft; }
      bool        getFlagOnRight() const  { return mFlagOnRight; }
      bool        getFlagInFront() const  { return mFlagInFront; }
      bool        getFlagInBack() const   { return mFlagInBack; }

      bool        load(BXMLNode root);

   protected:

      void        setFlagOnLeft(bool v)   { mFlagOnLeft = v; }
      void        setFlagOnRight(bool v)  { mFlagOnRight = v; }
      void        setFlagInFront(bool v)  { mFlagInFront = v; }
      void        setFlagInBack(bool v)   { mFlagInBack = v; }

      BSimString  mBoneName;
      float       mIKRange;
      uint8       mLinkCount;
      bool        mFlagOnLeft:1;
      bool        mFlagOnRight:1;
      bool        mFlagInFront:1;
      bool        mFlagInBack:1;

};

//==============================================================================
// BSweetSpotIKNode
//==============================================================================
class BSweetSpotIKNode
{
public:

   const BSimString& getBoneName() const  { return mBoneName; }

   uint8       getLinkCount() const    { return mLinkCount; }

   bool        load(BXMLNode root);

protected:

   BSimString  mBoneName;
   uint8       mLinkCount;

};

//==============================================================================
// BHardpointStatic
//==============================================================================
class BHardpointStatic
{
public:
   BHardpointStatic();
   ~BHardpointStatic();

   BSimString     mName;
   BSimString     mYawAttachmentName;
   BSimString     mPitchAttachmentName;
   long           mYawAttachmentHandle;
   float          mYawLeftMaxAngle;
   float          mYawRightMaxAngle;
   long           mPitchAttachmentHandle;   
   float          mPitchMaxAngle;
   float          mPitchMinAngle;
   float          mMaxCombinedAngle;

   BCueIndex      mStartYawSoundCue;
   BCueIndex      mStopYawSoundCue;
   BCueIndex      mStartPitchSoundCue;
   BCueIndex      mStopPitchSoundCue;

   BMatrix        mYawAttachmentTransform;
   BMatrix        mPitchAttachmentTransform;

   // Flags
   bool mFlagHardPitchLimits:1;
   bool mFlagAutoCenter:1;
   bool mFlagSingleBoneIK:1;
   bool mFlagCombined:1;
   bool mFlagRelativeToUnit:1;
   bool mFlagUseYawAndPitchAsTolerance:1;          
   bool mFlagInfiniteRateWhenHasTarget:1;
};

//==============================================================================
// BHardpoint
//==============================================================================
class BHardpoint
{
public:
   BHardpoint();
   ~BHardpoint();

   //-- Get Methods
   BSimString     getName() const { return mpStaticData->mName; }
   BSimString     getYawAttachmentName() const { return mpStaticData->mYawAttachmentName; }
   BSimString     getPitchAttachmentName() const { return mpStaticData->mPitchAttachmentName; }
   long           getYawAttachmentHandle() const { return mpStaticData->mYawAttachmentHandle; }
   float          getYawRotationRate() const { return mYawRotationRate; }
   float          getYawLeftMaxAngle() const { return mpStaticData->mYawLeftMaxAngle; }
   float          getYawRightMaxAngle() const { return mpStaticData->mYawRightMaxAngle; }
   float          getMaxCombinedAngle() const { return mpStaticData->mMaxCombinedAngle; }
   long           getPitchAttachmentHandle() const { return mpStaticData->mPitchAttachmentHandle; }
   float          getPitchRotationRate() const { return mPitchRotationRate; }
   float          getPitchMaxAngle() const { return mpStaticData->mPitchMaxAngle; }
   float          getPitchMinAngle() const { return mpStaticData->mPitchMinAngle; }

   BCueIndex      getStartYawSoundCue() const { return mpStaticData->mStartYawSoundCue; }
   BCueIndex      getStopYawSoundCue() const { return mpStaticData->mStopYawSoundCue; }
   BCueIndex      getStartPitchSoundCue() const { return mpStaticData->mStartPitchSoundCue; }
   BCueIndex      getStopPitchSoundCue() const { return mpStaticData->mStopPitchSoundCue; }

   BMatrix        getYawAttachmentTransform() const { return mpStaticData->mYawAttachmentTransform; }
   BMatrix        getPitchAttachmentTransform() const { return mpStaticData->mPitchAttachmentTransform; }

   //-- Set Methods
   void           setYawRotationRate(float yawRateInRadians) { mYawRotationRate = yawRateInRadians; }
   void           setPitchRotationRate(float pitchRateInRadians) {mPitchRotationRate = pitchRateInRadians; }

   //-- Flags
   bool           getFlagAutoCenter() const { return(mpStaticData->mFlagAutoCenter); }
   void           setFlagAutoCenter(bool v) { mpStaticData->mFlagAutoCenter=v; }
   bool           getFlagSingleBoneIK() const { return(mpStaticData->mFlagSingleBoneIK); }
   void           setFlagSingleBoneIK(bool v) { mpStaticData->mFlagSingleBoneIK=v; }
   bool           getFlagCombined() const { return(mpStaticData->mFlagCombined); }
   void           setFlagCombined(bool v) { mpStaticData->mFlagCombined=v; }
   bool           getFlagRelativeToUnit() const { return(mpStaticData->mFlagRelativeToUnit); }
   void           setFlagRelativeToUnit(bool v) { mpStaticData->mFlagRelativeToUnit=v; }
   bool           getFlagUseYawAndPitchAsTolerance() const { return(mpStaticData->mFlagUseYawAndPitchAsTolerance); }
   void           setFlagUseYawAndPitchAsTolerance(bool v) { mpStaticData->mFlagUseYawAndPitchAsTolerance=v; }
   bool           getFlagHardPitchLimits() const { return(mpStaticData->mFlagHardPitchLimits); }
   void           setFlagHardPitchLimits(bool v) { mpStaticData->mFlagHardPitchLimits=v; }
   bool           getFlagInfiniteRateWhenHasTarget() const { return(mpStaticData->mFlagInfiniteRateWhenHasTarget); }
   void           setFlagInfiniteRateWhenHasTarget(bool v) { mpStaticData->mFlagInfiniteRateWhenHasTarget=v; }

   friend class BProtoObject;
protected:
   bool           load(BXMLNode root);

   //-- Per Player Data, because it can be changed through techs, etc.
   float          mYawRotationRate;
   float          mPitchRotationRate;

   //-- Flags
   bool           mFlagOwnStaticData:1;

   BHardpointStatic* mpStaticData;
};

//==============================================================================
// BProtoSound
//==============================================================================
class BProtoSound
{
   public:
      BProtoSound() : mSoundCue(cInvalidCueIndex), mExtendedSoundCue(cInvalidCueIndex), mSquadID(-1), mSoundType(cObjectSoundNone), mCastingUnitOnly(false), mWorldID(cWorldNone), mActionID(-1) {}
      BCueIndex            mSoundCue;
      BCueIndex            mExtendedSoundCue;
      union
      {
         BSoundType           mSoundType;
         BSquadSoundType      mSquadSoundType;
      };
      long                 mSquadID; //-- Specific target (such as chatter played when attacking a certain squad)
      uint8                mWorldID; //-- Only play on a specific world
      int8                 mActionID;
      bool                 mCastingUnitOnly:1;
};
typedef BSmallDynamicSimArray<BProtoSound> BProtoSoundArray;
typedef std::pair<BProtoSound*, BSimString> BSoundStringPair;
static BDynamicSimArray<BSoundStringPair> tempActionNameFixup;

 //==============================================================================
 // BTerrainFlattenRegion
 //==============================================================================
 class BTerrainFlattenRegion
 {
 public:
    void          zero() { mMinX=0.0f; mMaxX=0.0f; mMinZ=0.0f; mMaxZ=0.0f; };

    float         mMinX;
    float         mMaxX;
    float         mMinZ;
    float         mMaxZ;
 };

 //==============================================================================
 //==============================================================================
 class BUniqueTechStatus
 {
   public:
      BUniqueTechStatus() : mTechID(-1), mTechStatus(-1) {}
      BUniqueTechStatus(long techID, long status) : mTechID(techID), mTechStatus(status) {}

      long        mTechID;
      long        mTechStatus;
 };
 typedef BSmallDynamicSimArray<BUniqueTechStatus> BUniqueTechStatusArray;

//==============================================================================
//==============================================================================
class BProtoObjectSoundData 
{
   public:
      BProtoObjectSoundData() : mUsedSoundTypes(cObjectSoundNone)
      {
      }

      BProtoSoundArray        mSounds;
      uint32                  mUsedSoundTypes;
};

//==============================================================================
// BProtoObjectStatic
//==============================================================================
class BProtoObjectStatic
{
   public:
      enum
      {
         cDamageFromFront,
         cDamageFromFrontRight,
         cDamageFromBackRight,
         cDamageFromBack,
         cDamageFromBackLeft,
         cDamageFromFrontLeft,

         cNumDamageFrom
      };

      enum
      {
         cExitFromFront = 0,
         cExitFromFrontRight,
         cExitFromFrontLeft,
         cExitFromRight,
         cExitFromLeft,                  
         cExitFromBack,         

         cNumExitFrom
      };

                              BProtoObjectStatic();
                              ~BProtoObjectStatic();

      BSimString              mName;
      BBitArray               mAbstractTypes;
      BTerrainFlattenRegion   mTerrainFlatten[2];
      long                    mMovementType;
      float                   mObstructionRadiusX;
      float                   mObstructionRadiusY;
      float                   mObstructionRadiusZ;
      float                   mParkingMinX;
      float                   mParkingMaxX;
      float                   mParkingMinZ;
      float                   mParkingMaxZ;
      float                   mTerrainHeightTolerance;
      long                    mPhysicsInfoID;
      long                    mPhysicsReplacementInfoID;
      float                   mPickRadius;
      float                   mPickOffset;
      long                    mPickPriority;
      long                    mSelectType;
      long                    mGotoType;
      float                   mSelectedRadiusX;
      float                   mSelectedRadiusZ;
      long                    mObjectClass;
      int                     mTrainerType;
      long                    mProtoCorpseDeathVisualIndex;
      long                    mProtoUIVisualIndex;
      BColor                  mMiniMapColor;
      long                    mGatherLinkObjectType;
      long                    mGatherLinkTarget;
      float                   mGatherLinkRadius;
      long                    mGathererLimit;
      long                    mBlockMovementObject;
      float                   mFlightLevel;
      DWORD                   mLifespan;
      float                   mAIAssetValueAdjust;
      float                   mCombatValue;
      float                   mResourceAmount;
      long                    mPlacementRule;
      float                   mDeathFadeTime;
      float                   mDeathFadeDelayTime;
      long                    mTrainAnimType;
      long                    mSquadModeAnimType[BSquadAI::cNumberModes];
      long                    mRallyPointType;
      float                   mMaxProjectileHeight;
      long                    mDeathReplacement;
      byte                    mSurfaceType;
      long                    mAutoLockDownType;
      long                    mHPBarID;
      float                   mHPBarSizeX;
      float                   mHPBarSizeY;
      BVector                 mHPBarOffset;
      float                   mGroundIKTiltFactor;
      BSimString              mGroundIKTiltBoneName;
      long                    mBeamHead;
      long                    mBeamTail;
      int                     mLevel;
      int                     mLevelUpEffect;
      int                     mRecoveringEffect;
      int                     mAddResourceID;
      float                   mAddResourceAmount;
      int                     mVisualDisplayPriority;
      float                   mChildObjectDamageTakenScalar;
      int                     mGarrisonSquadMode;
      float                   mTrueLOSHeight;
      int                     mNumConversions;
      int                     mNumStasisFieldsToStop;      
      int                     mBuildingStrengthID;

      BPopArray               mPops;
      BPopArray               mPopCapAdditions;

      BSmallDynamicSimArray<BProtoObjectCommand>      mCommands;
      BBitArray                                       mCommandAutoCloseMenu;
      BObjectTypeIDArray                              mContains;
      BSmallDynamicSimArray<long>                     mCostEscalationObjects;      
      BSmallDynamicSimArray<BHardpoint>               mHardpoints;
      BSmallDynamicSimArray<BGroundIKNode>            mGroundIKNodes;
      BSmallDynamicSimArray<BSweetSpotIKNode>         mSweetSpotIKNodes;
      BSmallDynamicSimArray<BSingleBoneIKNode>        mSingleBoneIKNodes;
      BSmallDynamicSimArray<BSimString>               mAbilityTriggerScripts;
      BProtoObjectLevelArray                          mLevels;
      BProtoObjectChildObjectArray                    mChildObjects;
      BHitZoneArray                                   mHitZoneList;

      long                    mAbilityCommand;
      long                    mProtoPowerID;
      long                    mStatsNameIndex;
      long                    mRolloverTextIndex;
      long                    mGaiaRolloverTextIndex[4];
      long                    mEnemyRolloverTextIndex;
      long                    mPrereqTextIndex;
      long                    mRoleTextIndex;
      BSimString              mIcon;
      BSimString              mMiniMapIcon;
      float                   mMiniMapIconSize;
      BSimString              mExistSoundBoneName;
      BSimString              mExtendedSoundBankName;

      long                    mShieldDirection;
      BDamageTypeIDSmall      mDamageTypes[2][cNumDamageFrom]; // Damage types for both available modes
      BDamageTypeIDSmall      mDamageType;
      int8                    mSecondaryDamageTypeMode;

      long                    mExitFromDirection;
      int                     mUnitSelectionIconID;
      int                     mSelectionDecalID;
      BFlashPropertyHandle    mClumpSelectionDecalID;
      BFlashPropertyHandle    mClumpSelectionDecalMaskID;
      BFlashPropertyHandle    mStaticDecalID;

      BTerrainImpactDecalHandle* mpImpactDecalHandle;

      int                     mAutoTrainOnBuiltID;
      int                     mSocketID;
      int                     mSocketPlayerScope;
      int                     mProtoSquadID;
      int                     mRateID;
      int                     mBuildStatsProtoID;
      int                     mSubSelectSort;
      float                   mAttackGradeDPS;
      float                   mRamDodgeFactor;

      int8                    mImpactSoundSetIndex;

      BRumbleEvent*           mpHoveringRumbleData;

      float                   mReverseSpeed;
      float                   mTurnRate;
      float                   mRepairPoints;
      float                   mCostEscalation;
      DWORD                   mTrackingDelay;
      float                   mStartingVelocity;
      float                   mAcceleration;
      float                   mFuel;
      float                   mPerturbanceChance;
      float                   mPerturbanceVelocity;
      float                   mPerturbanceMinTime;
      float                   mPerturbanceMaxTime;
      float                   mActiveScanChance;
      float                   mActiveScanRadiusScale;
      float                   mInitialPerturbanceVelocity;
      float                   mInitialPerturbanceMinTime;
      float                   mInitialPerturbanceMaxTime;
      int                     mMaxFlameEffects;
      BCost*                  mpCaptureCosts;
      float                   mGarrisonTime;
      float                   mBuildRotation;
      BVector                 mBuildOffset;
      BProtoObjectID          mAutoParkingLotObject;
      float                   mAutoParkingLotRotation;
      BVector                 mAutoParkingLotOffset;
      float                   mRevealRadius;

      long                    mShieldType;

      // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
      BProtoObjectID          mTargetBeam;
      BProtoObjectID          mKillBeam;

      long                    mMaxPopCount;

      BProtoObjectID          mID;

      // Static flags (only set at database load time and not per player)
      bool                    mFlagRotateObstruction:1;
      bool                    mFlagPlayerOwnsObstruction:1;
      bool                    mFlagCollidable:1;
      bool                    mFlagOrientUnitWithGround:1;
      bool                    mFlagSelectedRect:1;
      bool                    mFlagBuildingCommands:1;
      bool                    mFlagBuild:1;
      bool                    mFlagManualBuild:1;
      bool                    mFlagGatherLinkSelf:1;
      bool                    mFlagHasLifespan:1;
      bool                    mFlagNonMobile:1;
      bool                    mFlagFlying:1;
      bool                    mFlagTrackPlacement:1;
      bool                    mFlagDieAtZeroResources:1;
      bool                    mFlagUnlimitedResources:1;
      bool                    mFlagHasHPBar:1;
      bool                    mFlagBlockLOS:1;
      bool                    mFlagBlockMovement:1;
      bool                    mFlagAutoRepair:1;
      bool                    mFlagInvulnerable:1;
      bool                    mFlagHasShield:1;
      bool                    mFlagFullShield:1;
      bool                    mFlagIsAffectedByGravity:1;
      bool                    mFlagHighArc:1;
      bool                    mFlagHasAmmo:1;
      bool                    mFlagCapturable:1;
      bool                    mFlagUngarrisonToGaia:1;
      bool                    mFlagPassiveGarrisoned:1;
      bool                    mFlagShowRange:1;
      bool                    mFlagTracking:1;
      bool                    mFlagDamageGarrisoned:1;
      bool                    mFlagKillGarrisoned:1;
      bool                    mFlagUIDecal:1;      
      bool                    mFlagKBAware:1;
      bool                    mFlagIsExternalShield:1;
      bool                    mFlagKBCreatesBase:1;
      bool                    mFlagDestructible:1;
      bool                    mFlagVisibleForOwnerOnly:1;
      bool                    mFlagVisibleForTeamOnly:1;
      bool                    mFlagRocketOnDeath:1;
      bool                    mFlagIsBeam:1;
      bool                    mFlagDontAttackWhileMoving:1;
      bool                    mFlagFadeOnDeath:1;
      bool                    mFlagNoCull:1;
      bool                    mFlagHasTrackMask:1;
      bool                    mFlagPerturbOnce:1;
      bool                    mFlagTargetsFootOfUnit:1;
      bool                    mFlagStartAtMaxAmmo:1;
      bool                    mFlagInvulnerableWhenGaia:1;
      bool                    mFlagUpdate:1;
      bool                    mFlagNoActionOverrideMove:1;
      bool                    mFlagForceAnimRate:1;
      bool                    mFlagAlwaysVisibleOnMinimap:1;
      bool                    mFlagObscurable:1;
      bool                    mFlagNoRender:1;
      bool                    mFlagRepairable:1;
      bool                    mFlagFilterOrient:1;
      bool                    mFlagScaleBuildAnimRate:1;
      bool                    mFlagWalkToTurn:1;
      bool                    mFlagAirMovement:1;
      bool                    mFlagAutoSocket:1;
      bool                    mFlagNoBuildUnderAttack:1;
      bool                    mFlagDamageLinkedSocketsFirst:1;
      bool                    mFlagForceCreateObstruction:1;
      bool                    mFlagGatherLinkCircularSockets:1;
      bool                    mFlagDontAutoAttackMe:1;
      bool                    mFlagAlwaysAttackReviveUnits:1;
      bool                    mFlagSingleSocketBuilding:1;
      bool                    mFlagCommandableByAnyPlayer:1;
      bool                    mFlagExplodeOnTimer:1;
      bool                    mFlagExpireOnTimer:1;
      bool                    mFlagIsSticky:1;
      bool                    mFlagIsNeedler:1;
      bool                    mFlagLinearCostEscalation:1;
      bool                    mFlagInstantTrainWithRecharge:1;
      bool                    mFlagHasPivotingEngines:1;
      bool                    mFlagDamagedDeathReplacement:1;
      bool                    mFlagShatterDeathReplacement:1;
      bool                    mFlagCanRotate:1;
      bool                    mFlagUseBuildingAction:1;
      bool                    mFlagLockdownMenu:1;
      bool                    mFlagKillChildObjectsOnDeath:1;
      bool                    mFlagSelfParkingLot:1;
      bool                    mFlagChildForDamageTakenScalar:1;
      bool                    mFlagDieLast:1;
      bool                    mFlagSingleStick:1;
      bool                    mFlagIsFlameEffect:1;
      bool                    mFlagForceUpdateContainedUnits:1;
      bool                    mFlagFlattenTerrain:1;
      bool                    mFlagRegularAttacksMeleeOnly:1;
      bool                    mFlagAbilityAttacksMeleeOnly:1;
      bool                    mFlagMustOwnToSelect:1;
      bool                    mFlagShowRescuedCount:1;
      bool                    mFlagNoCorpse:1;
      bool                    mFlagNoRenderForOwner:1;
      bool                    mFlagTriggersBattleMusicWhenAttacked:1;
      bool                    mFlagAutoExplorationGroup:1;
      bool                    mFlagIsProjectileObstructable:1;
      bool                    mFlagProjectileTumbles:1;
      bool                    mFlagOneSquadContainment:1;
      bool                    mFlagIsTeleporter:1;
      bool                    mFlagNotSelectableWhenChildObject:1;
      bool                    mFlagIgnoreSquadAI:1;
      bool                    mFlagCanSetAsRallyPoint:1;
      bool                    mFlagSecondaryBuildingQueue:1;
      bool                    mFlagSelfDamage:1;
      bool                    mFlagPermanentSocket:1;
      bool                    mFlagHideOnImpact:1;
      bool                    mFlagRandomMoveAnimStart:1;
      bool                    mFlagObstructsAir:1;
      bool                    mFlagPhysicsDetonateOnDeath:1;
      bool                    mFlagSelectionDontConformToTerrain:1;
      bool                    mFlagTurnInPlace:1;
      bool                    mFlagSyncAnimRateToPhysics:1;
      bool                    mFlagIKTransitionToIdle:1;
      bool                    mFlagAppearsBelowDecals:1;
      bool                    mFlagUseRelaxedSpeedGroup:1;           // Means allow us to be grouped with units +- 2 m/s of our speed, as opposed to the typical 1 m/s
      bool                    mFlagCarryNoRenderToChildren:1;
      bool                    mFlagUseBuildRotation:1;
      bool                    mFlagUseAutoParkingLot:1;
      bool                    mFlagKillOnDetach:1;
      bool                    mFlagCheckPos:1;
      bool                    mFlagCheckLOSAgainstBase:1;
      bool                    mFlagTrainerApplyFormation:1;
      bool                    mFlagAllowStickyCam:1;
};

//==============================================================================
// BProtoObject
//==============================================================================
class BProtoObject
{
   public:   
                              BProtoObject(BProtoObjectID id);
                              BProtoObject(const BProtoObject* pBase);
                              ~BProtoObject();

      void                    transform(const BProtoObject* pBase);

      bool                    load(BXMLNode  root);      
      void                    postload();
      
      void                    loadAllAssets();
      void                    unloadAllAssets();

      // Dynamic data access (changeable per player during a game)
      long                    getProtoVisualIndex() const { return mProtoVisualIndex; }      
      float                   getDesiredVelocity() const { return mDesiredVelocity; }
      float                   getMaxVelocity() const { return mMaxVelocity; }
      float                   getHitpoints() const { return mHitpoints; }
      float                   getMass() const { return mHitpoints; }
      float                   getShieldpoints() const { return mShieldpoints; }
      float                   getMaxAmmo() const { return mAmmoMax; }
      float                   getAmmoRegenRate() const { return mAmmoRegenRate; }
      float                   getBounty() const { return mBounty; }
      bool                    getCommandAvailable(uint index) const { return !mCommandDisabled.isSet(index); }
      bool                    getCommandSelectable(uint index) const { return (!mCommandSelectable.isSet(index)); }
      bool                    getHasAttackRatings() const;
      float                   getAttackRatingDPS(BDamageTypeID damageType) const;
      float                   getAttackRatingDPS() const;
      float                   getAttackRating(BDamageTypeID damageType) const;
      float                   getAttackRating() const;
      float                   getDefenseRating() const;
      float                   getStrength() const;
      uint                    getAttackGrade(BDamageTypeID damageType) const;
      float                   getProtoLOS() const { return mLOS; } // SLB: ***** WARNING! This LOS value is in world space. Do not use this data with the visible map or the sim rep. *****
      long                    getProtoSimLOS() const { return mSimLOS; } // SLB: ***** WARNING! This LOS value is in sim tile space. Only use this data with the visible map or the sim rep. *****
      float                   getBuildPoints() const { return mBuildPoints; }
      const BCost*            getCost() const { return &mCost; }
      void                    getCost(const BPlayer* pPlayer, BCost* pCost, int countAdjustment) const;
      BTactic*                getTactic() const { return mpTactic; }
      float                   getRateAmount() const { return mRateAmount; }
      int                     getMaxContained() const { return mMaxContained; }
      long                    getTrainLimit(long id, bool squad, uint8* pBucketOut) const;
      long                    getDisplayNameIndex() const { return mDisplayNameIndex; }
      int                     getCircleMenuIconID() const { return mCircleMenuIconID; }

      void                    setVelocity(float val) { mDesiredVelocity=val; mMaxVelocity=val*cMaxVelocityMultiplier; }
      void                    setHitpoints(float val) { mHitpoints=val; }
      void                    setShieldpoints(float val) { mShieldpoints=val; }
      void                    setMaxAmmo(float val) { mAmmoMax=val; }
      void                    setAmmoRegenRate(float val) { mAmmoRegenRate=val; }
      void                    setBounty(float val) { mBounty=val; }
      void                    setProtoLOS(float val) { mLOS=val; }
      void                    setProtoSimLOS(long val) { mSimLOS=val; }
      void                    setBuildPoints(float val) { mBuildPoints=val; }
      void                    setCost(BCost* pVal) { mCost=*pVal; }
      void                    setRateAmount(float val) { mRateAmount=val; }
      void                    setMaxContained(int val) { mMaxContained=val; }
      void                    setTrueLOSHeight(float val) { mpStaticData->mTrueLOSHeight=val; }
      void                    setContains(const BObjectTypeIDArray& val) { mpStaticData->mContains = val; }
      void                    setCommandAvailable(uint index, bool val) { if(val) mCommandDisabled.unset(index); else mCommandDisabled.set(index); }
      void                    setCommandSelectable(uint index, bool val) { if(val) mCommandSelectable.unset(index); else mCommandSelectable.set(index); }      
      void                    setTrainLimit(long id, bool squad, long count);
      void                    setDisplayNameIndex(long val) { mDisplayNameIndex=val; }
      void                    setCircleMenuIconID(int id) { mCircleMenuIconID = id; }
      long                    getDeathSpawnSquad() const { return mDeathSpawnSquad; }
      void                    setDeathSpawnSquad(long v) { mDeathSpawnSquad = v; }


      // Dynamic flags (changeable per player during a game)
      bool                    getFlagOwnStaticData() const { return(mFlagOwnStaticData); }
      void                    setFlagOwnStaticData(bool v) { mFlagOwnStaticData=v; }
      bool                    getFlagOwnSoundData() const { return(mFlagOwnSoundData); }
      void                    setFlagOwnSoundData(bool v) { mFlagOwnSoundData=v; }
      bool                    getFlagAvailable() const { return(mFlagAvailable); }
      void                    setFlagAvailable(bool v) { mFlagAvailable=v; }
      bool                    getFlagForbid() const { return(mFlagForbid); }
      void                    setFlagForbid(bool v) { mFlagForbid=v; }
      bool                    getFlagAbilityDisabled() const { return mFlagAbilityDisabled; }
      void                    setFlagAbilityDisabled(bool v) { mFlagAbilityDisabled=v; }
      bool                    getFlagAutoCloak() const { return(mFlagAutoCloak); }
      void                    setFlagAutoCloak(bool v) { mFlagAutoCloak=v; }
      bool                    getFlagCloakMove() const { return(mFlagCloakMove); }
      void                    setFlagCloakMove(bool v) { mFlagCloakMove=v; }
      bool                    getFlagCloakAttack() const { return(mFlagCloakAttack); }
      void                    setFlagCloakAttack(bool v) { mFlagCloakAttack=v; }
      bool                    getFlagUniqueInstance() const { return(mFlagUniqueInstance); }
      void                    setFlagUniqueInstance(bool v) { mFlagUniqueInstance=v; }
      bool                    getFlagPlayerOwned() const { return mFlagPlayerOwned; }
      void                    setFlagPlayerOwned(bool v) { mFlagPlayerOwned=v; }

      //-- Non-dynamic flags, but here instead of in BProtoObjectStatic because they can
      // be used by objects which share common static data (this was done to save memory).
      bool                    getFlagForceToGaiaPlayer() const { return(mFlagForceToGaiaPlayer); }
      bool                    getFlagNoTieToGround() const { return(mFlagNoTieToGround); }
      bool                    getFlagGrayMapDopples() const { return(mFlagGrayMapDopples); }
      bool                    getFlagDopples() const { return(mFlagDopples); }
      bool                    getFlagVisibleToAll() const { return (mFlagVisibleToAll); }
      bool                    getFlagSoundBehindFOW() const { return (mFlagSoundBehindFOW); }
      bool                    getFlagNeutral() const { return(mFlagNeutral); }
      bool                    getFlagNoGrayMapDoppledInCampaign() const { return (mFlagNoGrayMapDoppledInCampaign); }

      // Static data access (only set at database load time and not per player)
      BProtoObjectID          getID() const { return mID; }
      void                    setID(BProtoObjectID id) { mID = id; }
      BProtoObjectID          getBaseType() const { return mBaseType; }
      void                    setBaseType(BProtoObjectID id) { mBaseType = id; }
      BProtoObjectID          getStaticID() { return mpStaticData->mID; }
      bool                    isType(BObjectTypeID type) const;
      long                    getMovementType() const { return mpStaticData->mMovementType; }
      float                   getReverseSpeed() const { return mpStaticData->mReverseSpeed; }
      float                   getTurnRate() const { return mpStaticData->mTurnRate; }
      float                   getRepairPoints() const { return mpStaticData->mRepairPoints; }
      float                   getCostEscalation() const { return mpStaticData->mCostEscalation; }
      DWORD                   getTrackingDelay() const { return mpStaticData->mTrackingDelay; }
      float                   getStartingVelocity() const { return mpStaticData->mStartingVelocity; }
      float                   getAcceleration() const { return mpStaticData->mAcceleration; }
      float                   getFuel() const { return mpStaticData->mFuel; }
      float                   getPerturbanceChance() const { return mpStaticData->mPerturbanceChance; }
      float                   getPerturbanceVelocity() const { return mpStaticData->mPerturbanceVelocity; }
      float                   getPerturbanceMinTime() const { return mpStaticData->mPerturbanceMinTime; }
      float                   getPerturbanceMaxTime() const { return mpStaticData->mPerturbanceMaxTime; }      
      float                   getActiveScanChance() const { return (mpStaticData->mActiveScanChance); }
      float                   getActiveScanRadius() const { return (mpStaticData->mActiveScanRadiusScale); }
      float                   getInitialPerturbanceVelocity() const { return (mpStaticData->mInitialPerturbanceVelocity); }
      float                   getInitialPerturbanceMinTime() const { return (mpStaticData->mInitialPerturbanceMinTime); }
      float                   getInitialPerturbanceMaxTime() const { return (mpStaticData->mInitialPerturbanceMaxTime); }
      int                     getMaxFlameEffects() const { return mpStaticData->mMaxFlameEffects; }
      const BCost*            getCaptureCost(const BPlayer* pPlayer) const;
      float                   getObstructionRadius() const { return(max(mpStaticData->mObstructionRadiusX, mpStaticData->mObstructionRadiusZ)); }
      float                   getObstructionRadiusX() const { return mpStaticData->mObstructionRadiusX; }
      void                    setObstructionRadiusX(float v) { mpStaticData->mObstructionRadiusX=v; }
      float                   getObstructionRadiusY() const { return mpStaticData->mObstructionRadiusY; }
      void                    setObstructionRadiusY(float v) { mpStaticData->mObstructionRadiusY=v; }
      float                   getObstructionRadiusZ() const { return mpStaticData->mObstructionRadiusZ; }
      void                    setObstructionRadiusZ(float v) { mpStaticData->mObstructionRadiusZ=v; }
      float                   getParkingMinX() const { return mpStaticData->mParkingMinX; }
      float                   getParkingMaxX() const { return mpStaticData->mParkingMaxX; }
      float                   getParkingMinZ() const { return mpStaticData->mParkingMinZ; }
      float                   getParkingMaxZ() const { return mpStaticData->mParkingMaxZ; }
      BTerrainFlattenRegion   getFlattenRegion(int index) const { return mpStaticData->mTerrainFlatten[index]; }
      float                   getTerrainHeightTolerance() const { return mpStaticData->mTerrainHeightTolerance; }
      float                   getPickRadius() const { return mpStaticData->mPickRadius; }
      void                    setPickRadius(float v) { mpStaticData->mPickRadius=v; }
      float                   getPickOffset() const { return mpStaticData->mPickOffset; }
      void                    setPickOffset(float v) { mpStaticData->mPickOffset=v; }
      long                    getPickPriority() const { return mpStaticData->mPickPriority; }
      long                    getSelectType() const { return mpStaticData->mSelectType; }
      long                    getGotoType() const { return mpStaticData->mGotoType; }
      float                   getSelectedRadiusX() const { return mpStaticData->mSelectedRadiusX; }
      void                    setSelectedRadiusX(float v) { mpStaticData->mSelectedRadiusX=v; }
      float                   getSelectedRadiusZ() const { return mpStaticData->mSelectedRadiusZ; }
      void                    setSelectedRadiusZ(float v) { mpStaticData->mSelectedRadiusZ=v; }
      long                    getObjectClass() const { return mpStaticData->mObjectClass; }
      int                     getTrainerType() const { return mpStaticData->mTrainerType; }
      long                    getProtoCorpseDeathVisualIndex() const { return mpStaticData->mProtoCorpseDeathVisualIndex; }
      long                    getProtoUIVisualIndex() const { return mpStaticData->mProtoUIVisualIndex; }
      BCueIndex               getSound(BSoundType soundType, long squadID=-1, bool extendedBankSound=false, long abilityID=-1) const;
      const BColor&           getMinimapColor() const { return mpStaticData->mMiniMapColor; }
      long                    getGatherLinkObjectType() const { return mpStaticData->mGatherLinkObjectType; }
      long                    getGatherLinkTarget() const { return mpStaticData->mGatherLinkTarget; }
      float                   getGatherLinkRadius() const { return mpStaticData->mGatherLinkRadius; }
      long                    getGathererLimit() const { return mpStaticData->mGathererLimit; }
      long                    getBlockMovementObject() const { return mpStaticData->mBlockMovementObject; }
      long                    getNumberPops() const { return mpStaticData->mPops.getNumber(); }
      BPop                    getPop(long index) const { return mpStaticData->mPops[index]; }
      BPopArray*              getPops() const { return &(mpStaticData->mPops); }
      long                    getNumberPopCapAdditions() const { return mpStaticData->mPopCapAdditions.getNumber(); }
      BPop                    getPopCapAddition(long index) const { return mpStaticData->mPopCapAdditions[index]; }
      uint                    getNumberCommands() const { return mpStaticData->mCommands.getSize(); }
      BProtoObjectCommand     getCommand(uint index) const { return mpStaticData->mCommands[index]; }
      bool                    getCommandAutoCloseMenu(uint index) const { return ((long)index >= mpStaticData->mCommandAutoCloseMenu.getNumber() ? false : mpStaticData->mCommandAutoCloseMenu.isBitSet(index)!=0); }
      bool                    getCommandAutoCloseMenu(int type, int id) const;
      long                    getNumberCostEscalationObjects() const { return mpStaticData->mCostEscalationObjects.getNumber(); }
      long                    getCostEscalationObject(long index) const { return mpStaticData->mCostEscalationObjects[index]; }
      long                    findHardpoint(const char* szName) const;
      long                    getAbilityCommand() const { return mpStaticData->mAbilityCommand; }
      long                    getProtoPowerID() const { return mpStaticData->mProtoPowerID; }
      long                    getDBID() const { return mDBID; }
      const BSimString&       getName() const { return mpStaticData->mName; }
      void                    getDisplayName(BUString& string) const;
      void                    getStatsName(BUString& string) const;
      void                    getRolloverText(BUString& string) const;
      void                    getGaiaRolloverText(int civID, BUString& string) const;
      void                    getEnemyRolloverText(BUString& string) const;
      void                    getPrereqText(BUString& string) const;
      void                    getRoleText(BUString& string) const;
      long                    getStatsTextIndex() const { return mpStaticData->mStatsNameIndex; }
      long                    getRolloverTextIndex() const { return mpStaticData->mRolloverTextIndex; }
      long                    getGaiaRolloverTextIndex(int civID) const { return (civID>=0 && civID<=3 ? mpStaticData->mGaiaRolloverTextIndex[civID] : -1); }
      long                    getEnemyRolloverTextIndex() const { return mpStaticData->mEnemyRolloverTextIndex; }
      long                    getPrereqTextIndex() const { return mpStaticData->mPrereqTextIndex; }
      long                    getRoleTextIndex() const { return mpStaticData->mRoleTextIndex; }
      const BBitArray&        getAbstractTypes() const { return mpStaticData->mAbstractTypes; }
      const BSimString&       getIcon() const { return mpStaticData->mIcon; }
      const BSimString&       getMiniMapIcon() const { return mpStaticData->mMiniMapIcon; }
      float                   getMiniMapIconSize() const { return mpStaticData->mMiniMapIconSize; }
      long                    getPhysicsInfoID() const { return mpStaticData->mPhysicsInfoID; }
      long                    getPhysicsReplacementInfoID() const { return mpStaticData->mPhysicsReplacementInfoID; }
      const BSimString&       getExistSoundBoneName() const { return mpStaticData->mExistSoundBoneName; }
      DWORD                   getLifespan() const { return(mpStaticData->mLifespan); }
      float                   getAIAssetValueAdjust() const { return mpStaticData->mAIAssetValueAdjust; }
      float                   getCombatValue() const { return mpStaticData->mCombatValue; }
      float                   getResourceAmount() const { return mpStaticData->mResourceAmount; }
      float                   getFlightLevel() const { return mpStaticData->mFlightLevel; }
      long                    getPlacementRuleIndex() const { return mpStaticData->mPlacementRule; }
      float                   getDeathFadeTime() const { return mpStaticData->mDeathFadeTime; }
      float                   getDeathFadeDelayTime() const { return (mpStaticData->mDeathFadeDelayTime); }
      DWORD                   getDeathFadeDelayTimeDWORD() const { return ((DWORD)(mpStaticData->mDeathFadeDelayTime * 1000.0f)); }
      long                    getTrainAnimType() const { return mpStaticData->mTrainAnimType; }
      long                    getSquadModeAnimType(long mode) const { return mpStaticData->mSquadModeAnimType[mode]; }
      long                    getRallyPointType() const { return mpStaticData->mRallyPointType; }
      float                   getMaxProjectileHeight() const { return mpStaticData->mMaxProjectileHeight; }
      float                   getGroundIKTiltFactor() const { return mpStaticData->mGroundIKTiltFactor; }
      const BSimString&       getGroundIKTiltBoneName() const { return mpStaticData->mGroundIKTiltBoneName; }
      BDamageTypeID           getDamageType(BVector damageDirection, BVector unitForward, BVector unitRight, bool testShield, bool testArmor, int mode) const;
      BDamageTypeID           getDamageType() const { return mpStaticData->mDamageType; }
      int                     getSecondaryDamageTypeMode() const { return mpStaticData->mSecondaryDamageTypeMode; }
      long                    getNumberHardpoints() const { return mHardpoints.getNumber(); }
      const BHardpoint*       getHardpoint(long index) const { if (index < 0 || index >= mHardpoints.getNumber()) return NULL; return &mHardpoints[index]; }
      BHardpoint*             getHardpointMutable(long index) { if (index < 0 || index >= mHardpoints.getNumber()) return NULL; return &mHardpoints[index]; }
      long                    getNumberGroundIKNodes() const { return mpStaticData->mGroundIKNodes.getNumber(); }
      const BGroundIKNode*    getGroundIKNode(long index)const  { if (index < 0 || index >= mpStaticData->mGroundIKNodes.getNumber()) return NULL; return &mpStaticData->mGroundIKNodes[index]; }
      long                    getNumberSweetSpotIKNodes() const { return mpStaticData->mSweetSpotIKNodes.getNumber(); }
      const BSweetSpotIKNode* getSweetSpotIKNode(long index)const  { if (index < 0 || index >= mpStaticData->mSweetSpotIKNodes.getNumber()) return NULL; return &mpStaticData->mSweetSpotIKNodes[index]; }
      long                    getNumberSingleBoneIKNodes() const { return mpStaticData->mSingleBoneIKNodes.getNumber(); }
      const BSingleBoneIKNode* getSingleBoneIKNode(long index)const  { if (index < 0 || index >= mpStaticData->mSingleBoneIKNodes.getNumber()) return NULL; return &mpStaticData->mSingleBoneIKNodes[index]; }
      uint                    getNumAbilityTriggerScripts() const { return mpStaticData->mAbilityTriggerScripts.size(); }
      const BSimString&       getAbilityTriggerScript(uint index) const { return mpStaticData->mAbilityTriggerScripts[index]; }
      int                     getNumberLevels() const { return mpStaticData->mLevels.getNumber(); }
      const BProtoObjectLevel* getLevel(int index) const  { if (index < 0 || index >= mpStaticData->mLevels.getNumber()) return NULL; return &mpStaticData->mLevels[index]; }
      int                     getAddResourceID() const { return mpStaticData->mAddResourceID; }
      float                   getAddResourceAmount() const { return mpStaticData->mAddResourceAmount; }
      const long              getExitFromDirection() const { return(mpStaticData->mExitFromDirection); }
      long                    getDeathReplacement() const { return mpStaticData->mDeathReplacement; }
      byte                    getSurfaceType() const { return mpStaticData->mSurfaceType; }
      long                    getAutoLockDownType() const { return mpStaticData->mAutoLockDownType; }
      void                    setImpactSoundSet(int8 impactSoundSetIndex)     { mpStaticData->mImpactSoundSetIndex = impactSoundSetIndex; }
      int8                    getImpactSoundSet() const { return mpStaticData->mImpactSoundSetIndex; }
      bool                    getImpactSoundCue(byte surfaceType, BImpactSoundInfo& soundInfo) const;
      int                     getHPBarID() const { return mpStaticData->mHPBarID; }
      float                   getHPBarSizeX() const { return mpStaticData->mHPBarSizeX; }
      float                   getHPBarSizeY() const { return mpStaticData->mHPBarSizeY; }
      const BVector&          getHPBarOffset() const { return mpStaticData->mHPBarOffset; }
      int                     getUnitSelectionIconID() const { return mpStaticData->mUnitSelectionIconID; }
      void                    setUnitSelectionIconID(int id) const { mpStaticData->mUnitSelectionIconID = id; }
      int                     getSelectionDecalID() const { return mpStaticData->mSelectionDecalID; }
      void                    setSelectionDecalID(int id) const { mpStaticData->mSelectionDecalID = id; }
      BFlashPropertyHandle    getClumpSelectionDecalID() const { return mpStaticData->mClumpSelectionDecalID; }
      void                    setClumpSelectionDecalID(BFlashPropertyHandle id) const { mpStaticData->mClumpSelectionDecalID = id; }
      BFlashPropertyHandle    getClumpSelectionDecalMaskID() const { return mpStaticData->mClumpSelectionDecalMaskID; }
      void                    setClumpSelectionDecalMaskID(BFlashPropertyHandle id) const { mpStaticData->mClumpSelectionDecalMaskID = id; }
      BFlashPropertyHandle    getStaticDecalID() const { return mpStaticData->mStaticDecalID; }
      void                    setStaticDecalID(BFlashPropertyHandle id) const { mpStaticData->mStaticDecalID = id; }
      const BSimString&       getExtendedSoundBankName() const { return mpStaticData->mExtendedSoundBankName; }
      long                    getBeamHead() const { return mpStaticData->mBeamHead; }
      long                    getBeamTail() const { return mpStaticData->mBeamTail; }
      int                     getLevel() const { return mpStaticData->mLevel; }
      int                     getLevelUpEffect() const { return mpStaticData->mLevelUpEffect; }
      int                     getRecoveringEffect() const { return mpStaticData->mRecoveringEffect; }
      const BTerrainImpactDecalHandle* getImpactDecal() const {return mpStaticData->mpImpactDecalHandle;}
      int                     getAutoTrainOnBuiltID() const { return mpStaticData->mAutoTrainOnBuiltID; }
      int                     getSocketID() const { return mpStaticData->mSocketID; }
      int                     getSocketPlayerScope() const { return mpStaticData->mSocketPlayerScope; }
      uint                    getNumberChildObjects() const { return mpStaticData->mChildObjects.getSize(); };
      int                     getChildObjectType(uint index) const { if (index < mpStaticData->mChildObjects.getSize()) return mpStaticData->mChildObjects[index].mType; else return -1; }
      int                     getChildObjectUserCiv(uint index) const { if (index < mpStaticData->mChildObjects.getSize()) return mpStaticData->mChildObjects[index].mUserCiv; else return -1; }
      int                     getChildObjectID(uint index) const { if (index < mpStaticData->mChildObjects.getSize()) return mpStaticData->mChildObjects[index].mID; else return -1; }
      BVector                 getChildObjectOffset(uint index) const { if (index < mpStaticData->mChildObjects.getSize()) return BVector(mpStaticData->mChildObjects[index].mOffsetX,0.0f,mpStaticData->mChildObjects[index].mOffsetZ); else return cOriginVector; }
      float                   getChildObjectRotation(uint index) const { if (index < mpStaticData->mChildObjects.getSize()) return mpStaticData->mChildObjects[index].mRotation; else return 0.0f; }
      const BSimString&       getChildObjectAttachBoneName(uint index) const { if (index < mpStaticData->mChildObjects.getSize()) return mpStaticData->mChildObjects[index].mAttachBoneName; else return BSimString::getEmptyString(); }

      void                    setProtoSquadID(int id) { mpStaticData->mProtoSquadID=id; }
      int                     getProtoSquadID() const { return mpStaticData->mProtoSquadID; }
      int                     getRateID() const { return mpStaticData->mRateID; }
      int                     getBuildStatsProtoID() const { return mpStaticData->mBuildStatsProtoID; }
      int                     getSubSelectSort() const { return mpStaticData->mSubSelectSort; }
      float                   getAttackGradeDPS() const { return mpStaticData->mAttackGradeDPS; }
      float                   getRamDodgeFactor() const { return mpStaticData->mRamDodgeFactor; }
      const BRumbleEvent*     getHoveringRumbleData() const { return mpStaticData->mpHoveringRumbleData; }
      int                     getVisualDisplayPriority() const { return mpStaticData->mVisualDisplayPriority; };
      float                   getChildObjectDamageTakenScalar() const { return mpStaticData->mChildObjectDamageTakenScalar; }
      BHitZoneArray*          getHitZoneList(){ return &(mpStaticData->mHitZoneList); }
      int                     getGarrisonSquadMode() const { return (mpStaticData->mGarrisonSquadMode); }
      float                   getTrueLOSHeight() const { return mpStaticData->mTrueLOSHeight; }
      int                     getNumConversions() const { return mpStaticData->mNumConversions;; }
      int                     getNumStasisFieldsToStop() const { return mpStaticData->mNumStasisFieldsToStop; }
      const BObjectTypeIDArray& getContains() const { return (mpStaticData->mContains); }
      float                   getGarrisonTime() const { return mpStaticData->mGarrisonTime; }
      float                   getBuildRotation() const { return mpStaticData->mBuildRotation; }
      BVector                 getBuildOffset() const { return mpStaticData->mBuildOffset; }
      BProtoObjectID          getAutoParkingLotObject() const { return mpStaticData->mAutoParkingLotObject; }
      float                   getAutoParkingLotRotation() const { return mpStaticData->mAutoParkingLotRotation; }
      BVector                 getAutoParkingLotOffset() const { return mpStaticData->mAutoParkingLotOffset; }
      int                     getBuildingStrengthID() const { return mpStaticData->mBuildingStrengthID; };
      long                    getShieldType() const { return mpStaticData->mShieldType; }
      float                   getRevealRadius() const { return mpStaticData->mRevealRadius; }
      long                    getMaxPopCount() const { return mpStaticData->mMaxPopCount; }

      void                    clearFlags();
      void                    copyFlags(const BProtoObject* pBase);

      void                    addUniqueTechStatus(long techID, long status);
      bool                    getUniqueTechStatus(long techID, long& status) const;

      // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
      BProtoObjectID          getTargetBeam() const { return (mpStaticData->mTargetBeam); }
      BProtoObjectID          getKillBeam() const { return (mpStaticData->mKillBeam); }

      // Static flags (only set at database load time and not per player)
      bool                    getFlagRotateObstruction() const { return(mpStaticData->mFlagRotateObstruction); }
      bool                    getFlagPlayerOwnsObstruction() const { return(mpStaticData->mFlagPlayerOwnsObstruction); }
      bool                    getFlagCollidable() const { return(mpStaticData->mFlagCollidable); }
      bool                    getFlagOrientUnitWithGround() const { return(mpStaticData->mFlagOrientUnitWithGround); }
      bool                    getFlagFilterOrient() const { return(mpStaticData->mFlagFilterOrient); }
      bool                    getFlagSelectedRect() const { return(mpStaticData->mFlagSelectedRect); }
      bool                    getFlagBuildingCommands() const { return(mpStaticData->mFlagBuildingCommands); }
      bool                    getFlagBuild() const { return(mpStaticData->mFlagBuild); }
      bool                    getFlagManualBuild() const { return(mpStaticData->mFlagManualBuild); }
      bool                    getFlagGatherLinkSelf() const { return(mpStaticData->mFlagGatherLinkSelf); }
      bool                    getFlagGatherLinkCircularSockets() const { return(mpStaticData->mFlagGatherLinkCircularSockets); }
      bool                    getFlagHasLifespan() const { return(mpStaticData->mFlagHasLifespan); }
      bool                    getFlagNonMobile() const { return(mpStaticData->mFlagNonMobile); }
      bool                    getFlagFlying() const { return(mpStaticData->mFlagFlying); }
      bool                    getFlagTrackPlacement() const { return(mpStaticData->mFlagTrackPlacement); }
      void                    setFlagTrackPlacement(bool v) { mpStaticData->mFlagTrackPlacement=v; }
      bool                    getFlagDieAtZeroResources() const { return(mpStaticData->mFlagDieAtZeroResources); }
      bool                    getFlagUnlimitedResources() const { return(mpStaticData->mFlagUnlimitedResources); }
      bool                    getFlagHasHPBar() const { return(mpStaticData->mFlagHasHPBar); }
      bool                    getFlagBlockLOS() const { return(mpStaticData->mFlagBlockLOS); }
      bool                    getFlagBlockMovement() const { return(mpStaticData->mFlagBlockMovement); }
      bool                    getFlagAutoRepair() const { return(mpStaticData->mFlagAutoRepair); }
      bool                    getFlagInvulnerable() const { return(mpStaticData->mFlagInvulnerable); }
      bool                    getFlagInvulnerableWhenGaia() const { return(mpStaticData->mFlagInvulnerableWhenGaia); }
      bool                    getFlagHasShield() const { return(mpStaticData->mFlagHasShield); }
      bool                    getFlagFullShield() const { return(mpStaticData->mFlagFullShield); }
      bool                    getFlagIsAffectedByGravity() const { return(mpStaticData->mFlagIsAffectedByGravity); }
      bool                    getFlagHighArc() const { return(mpStaticData->mFlagHighArc); }
      bool                    getFlagHasAmmo() const { return(mpStaticData->mFlagHasAmmo); }
      bool                    getFlagCapturable() const { return(mpStaticData->mFlagCapturable); }
      bool                    getFlagUngarrisonToGaia() const { return(mpStaticData->mFlagUngarrisonToGaia); }
      bool                    getFlagPassiveGarrisoned() const { return(mpStaticData->mFlagPassiveGarrisoned); }
      bool                    getFlagShowRange() const { return(mpStaticData->mFlagShowRange); }
      bool                    getFlagTracking() const { return(mpStaticData->mFlagTracking); }
      bool                    getFlagDamageGarrisoned() const { return(mpStaticData->mFlagDamageGarrisoned); }
      bool                    getFlagKillGarrisoned() const { return(mpStaticData->mFlagKillGarrisoned); }
      bool                    getFlagUIDecal() const { return(mpStaticData->mFlagUIDecal); }
      bool                    getFlagKBAware() const { return(mpStaticData->mFlagKBAware); }
      bool                    getFlagIsExternalShield() const { return(mpStaticData->mFlagIsExternalShield); }
      bool                    getFlagKBCreatesBase() const { return(mpStaticData->mFlagKBCreatesBase); }
      bool                    getFlagDestructible() const { return(mpStaticData->mFlagDestructible); }
      bool                    getFlagVisibleForOwnerOnly() const { return(mpStaticData->mFlagVisibleForOwnerOnly); }
      bool                    getFlagVisibleForTeamOnly() const { return (mpStaticData->mFlagVisibleForTeamOnly); }
      bool                    getFlagRocketOnDeath() const { return(mpStaticData->mFlagRocketOnDeath); }
      bool                    getFlagIsBeam() const { return(mpStaticData->mFlagIsBeam); }
      bool                    getFlagDontAttackWhileMoving() const { return mpStaticData->mFlagDontAttackWhileMoving; }
      bool                    getFlagFadeOnDeath() const { return(mpStaticData->mFlagFadeOnDeath); }
      bool                    getFlagNoCull() const { return(mpStaticData->mFlagNoCull); }
      bool                    getFlagHasTrackMask() const { return(mpStaticData->mFlagHasTrackMask); }
      bool                    getFlagPertubOnce() const { return (mpStaticData->mFlagPerturbOnce); }
      bool                    getFlagTargetsFootOfUnit() const { return(mpStaticData->mFlagTargetsFootOfUnit); }
      bool                    getFlagStartAtMaxAmmo() const { return(mpStaticData->mFlagStartAtMaxAmmo); }
      bool                    getFlagUpdate() const { return(mpStaticData->mFlagUpdate); }
      bool                    getFlagNoActionOverrideMove() const { return(mpStaticData->mFlagNoActionOverrideMove); }
      bool                    getFlagForceAnimRate() const { return(mpStaticData->mFlagForceAnimRate); }
      bool                    getFlagScaleBuildAnimRate() const { return(mpStaticData->mFlagScaleBuildAnimRate); }
      bool                    getFlagWalkToTurn() const { return(mpStaticData->mFlagWalkToTurn); }
      bool                    getFlagAlwaysVisibleOnMinimap() const { return(mpStaticData->mFlagAlwaysVisibleOnMinimap); }
      bool                    getFlagObscurable() const { return mpStaticData->mFlagObscurable; }
      bool                    getFlagNoRender() const { return mpStaticData->mFlagNoRender; }
      bool                    getFlagRepairable() const { return(mpStaticData->mFlagRepairable); }
      bool                    getFlagAirMovement() const { return mpStaticData->mFlagAirMovement; }
      bool                    getFlagAutoSocket() const { return mpStaticData->mFlagAutoSocket; }
      bool                    getFlagNoBuildUnderAttack() const { return mpStaticData->mFlagNoBuildUnderAttack; }
      bool                    getFlagDamageLinkedSocketsFirst() const { return mpStaticData->mFlagDamageLinkedSocketsFirst; }
      bool                    getFlagForceCreateObstruction() const { return mpStaticData->mFlagForceCreateObstruction; }
      bool                    getFlagDontAutoAttackMe() const { return mpStaticData->mFlagDontAutoAttackMe; }
      bool                    getFlagAlwaysAttackReviveUnits() const { return mpStaticData->mFlagAlwaysAttackReviveUnits; }
      bool                    getFlagSingleSocketBuilding() const { return mpStaticData->mFlagSingleSocketBuilding; }
      bool                    getFlagCommandableByAnyPlayer() const { return mpStaticData->mFlagCommandableByAnyPlayer; }
      bool                    getFlagExplodeOnTimer() const { return(mpStaticData->mFlagExplodeOnTimer); }
      bool                    getFlagExpireOnTimer() const { return(mpStaticData->mFlagExpireOnTimer); }
      bool                    getFlagIsSticky() const { return(mpStaticData->mFlagIsSticky); }
      bool                    getFlagIsFlameEffect() const { return(mpStaticData->mFlagIsFlameEffect); }
      bool                    getFlagIsNeedler() const { return(mpStaticData->mFlagIsNeedler); }
      bool                    getFlagLinearCostEscalation() const { return mpStaticData->mFlagLinearCostEscalation; }
      bool                    getFlagInstantTrainWithRecharge() const { return mpStaticData->mFlagInstantTrainWithRecharge; }
      bool                    getFlagHasPivotingEngines() const { return(mpStaticData->mFlagHasPivotingEngines); }
      bool                    getFlagDamagedDeathReplacement() const { return mpStaticData->mFlagDamagedDeathReplacement; }
      bool                    getFlagShatterDeathReplacement() const { return mpStaticData->mFlagShatterDeathReplacement; }
      bool                    getFlagCanRotate() const { return mpStaticData->mFlagCanRotate; }
      bool                    getFlagUseBuildingAction() const { return mpStaticData->mFlagUseBuildingAction; }
      bool                    getFlagLockdownMenu() const { return mpStaticData->mFlagLockdownMenu; }
      bool                    getFlagKillChildObjectsOnDeath() const { return mpStaticData->mFlagKillChildObjectsOnDeath; }
      bool                    getFlagSelfParkingLot() const { return mpStaticData->mFlagSelfParkingLot; }
      bool                    getFlagChildForDamageTakenScalar() const { return mpStaticData->mFlagChildForDamageTakenScalar; }
      bool                    getFlagDieLast() const { return (mpStaticData->mFlagDieLast); }
      bool                    getFlagSingleStick() const { return(mpStaticData->mFlagSingleStick); }
      bool                    getFlagForceUpdateContainedUnits() const { return (mpStaticData->mFlagForceUpdateContainedUnits); }
      bool                    getFlagFlattenTerrain() const { return (mpStaticData->mFlagFlattenTerrain); }
      bool                    getFlagRegularAttacksMeleeOnly() const { return mpStaticData->mFlagRegularAttacksMeleeOnly; }
      bool                    getFlagAbilityAttacksMeleeOnly() const { return mpStaticData->mFlagAbilityAttacksMeleeOnly; }
      bool                    getFlagMustOwnToSelect() const { return mpStaticData->mFlagMustOwnToSelect; }
      bool                    getFlagShowRescuedCount() const { return mpStaticData->mFlagShowRescuedCount; }
      bool                    getFlagNoCorpse() const { return mpStaticData->mFlagNoCorpse; }
      bool                    getFlagNoRenderForOwner() const { return(mpStaticData->mFlagNoRenderForOwner); }
      bool                    getFlagAutoExplorationGroup() const { return(mpStaticData->mFlagAutoExplorationGroup); }
      bool                    getFlagTriggersBattleMusicWhenAttacked() const { return mpStaticData->mFlagTriggersBattleMusicWhenAttacked; }
      bool                    getFlagIsProjectileObstructable() const { return(mpStaticData->mFlagIsProjectileObstructable); }
      bool                    getFlagProjectileTumbles() const { return(mpStaticData->mFlagProjectileTumbles); }
      bool                    getFlagOneSquadContainment() const { return(mpStaticData->mFlagOneSquadContainment); }
      bool                    getFlagIsTeleporter() const { return(mpStaticData->mFlagIsTeleporter); }
      bool                    getFlagNotSelectableWhenChildObject() const { return(mpStaticData->mFlagNotSelectableWhenChildObject); }
      bool                    getFlagIgnoreSquadAI() const { return (mpStaticData->mFlagIgnoreSquadAI); }
      bool                    getFlagCanSetAsRallyPoint() const { return (mpStaticData->mFlagCanSetAsRallyPoint); }
      bool                    getFlagSecondaryBuildingQueue() const { return (mpStaticData->mFlagSecondaryBuildingQueue); }
      bool                    getFlagSelfDamage() const { return(mpStaticData->mFlagSelfDamage); }
      bool                    getFlagPermanentSocket() const { return mpStaticData->mFlagPermanentSocket; }
      bool                    getFlagHideOnImpact() const { return mpStaticData->mFlagHideOnImpact; }
      bool                    getFlagRandomMoveAnimStart() const { return mpStaticData->mFlagRandomMoveAnimStart; }
      bool                    getFlagObstructsAir() const { return mpStaticData->mFlagObstructsAir; }
      bool                    getFlagPhysicsDetonateOnDeath() const { return mpStaticData->mFlagPhysicsDetonateOnDeath; }
      bool                    getFlagSelectionDontConformToTerrain() const { return mpStaticData->mFlagSelectionDontConformToTerrain; }
      bool                    getFlagTurnInPlace() const { return mpStaticData->mFlagTurnInPlace; }
      bool                    getFlagSyncAnimRateToPhysics() const { return mpStaticData->mFlagSyncAnimRateToPhysics; }
      bool                    getFlagIKTransitionToIdle() const { return mpStaticData->mFlagIKTransitionToIdle; }
      bool                    getFlagAppearsBelowDecals() const { return mpStaticData->mFlagAppearsBelowDecals; }
      bool                    getFlagUsedRelaxedSpeedGroup() const { return mpStaticData->mFlagUseRelaxedSpeedGroup; }
      bool                    getFlagCarryNoRenderToChildren() const { return mpStaticData->mFlagCarryNoRenderToChildren; }
      bool                    getFlagUseBuildRotation() const { return mpStaticData->mFlagUseBuildRotation; }
      bool                    getFlagUseAutoParkingLot() const { return mpStaticData->mFlagUseAutoParkingLot; }
      bool                    getFlagKillOnDetach() const { return mpStaticData->mFlagKillOnDetach; }
      bool                    getFlagCheckPos() const { return mpStaticData->mFlagCheckPos; }
      bool                    getFlagCheckLOSAgainstBase() const { return mpStaticData->mFlagCheckLOSAgainstBase; }
      bool                    getFlagTrainerApplyFormation() const { return mpStaticData->mFlagTrainerApplyFormation; }
      bool                    getFlagAllowStickCam() const { return mpStaticData->mFlagAllowStickyCam; }

      //-- UI Sound Methods      
      bool                    playUISound(BSoundType soundType, bool suppressBankLoad, BVector position, long squadID, long actionID=-1) const;

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType, BPlayer* pPlayer);

   protected:
      void                    loadDamageType(BXMLNode node);
      void                    addDamageType(BDamageTypeID damageType, int mode, int damageFrom, bool ignoreOverlap, int squadMode);      


      // Static data
      BProtoObjectStatic*     mpStaticData;                    // 4 bytes
      BProtoObjectSoundData*  mpSoundData;                     // 4  bytes

      // Dynamic data (changeable per player during the game)
      BProtoObjectTrainLimitArray   mTrainLimits;              // 12 bytes      
      BSmallDynamicSimArray<BHardpoint> mHardpoints;           // 12 bytes;
      BCost                   mCost;                           // 8 bytes
      BProtoObjectID          mID;                             // 4 bytes
      BProtoObjectID          mBaseType;                       // 4 bytes
      long                    mDBID;
      long                    mProtoVisualIndex;               // 4 bytes
      float                   mDesiredVelocity;                // 4 bytes
      float                   mMaxVelocity;                    // 4 bytes
      float                   mHitpoints;                      // 4 bytes
      float                   mShieldpoints;                   // 4 bytes
      float                   mLOS;                            // 4 bytes
      long                    mSimLOS;                         // 4 bytes
      float                   mBuildPoints;                    // 4 bytes
      float                   mBounty;                         // 4 bytes
      BTactic*                mpTactic;                        // 4 bytes
      float                   mAmmoMax;                        // 4 bytes
      float                   mAmmoRegenRate;                  // 4 bytes
      float                   mRateAmount;                     // 4 bytes
      int                     mMaxContained;                   // 4 bytes
      long                    mDisplayNameIndex;               // 4 bytes
      int                     mCircleMenuIconID;               // 4 bytes
      UTBitVector<32>         mCommandDisabled;                // 4 bytes
      UTBitVector<32>         mCommandSelectable;              // 4 bytes

      BUniqueTechStatusArray* mpUniqueTechStatusArray;         // 4 bytes
      long                    mDeathSpawnSquad;                // 4 bytes


      //-- Dynamic Flags (changeable per player during the game)
      bool                    mFlagOwnStaticData:1;            // 1 byte
      bool                    mFlagOwnSoundData:1;             //
      bool                    mFlagAvailable:1;                //
      bool                    mFlagForbid:1;                   //
      bool                    mFlagAbilityDisabled:1;          //
      bool                    mFlagAutoCloak:1;                //
      bool                    mFlagCloakMove:1;                //
      bool                    mFlagCloakAttack:1;              //
      bool                    mFlagUniqueInstance:1;           // 1 byte
      bool                    mFlagPlayerOwned:1;              //

      //-- Non-dynamic flags, but here instead of in BProtoObjectStatic because they can
      // be used by objects which share common static data (this was done to save memory).
      bool                    mFlagForceToGaiaPlayer:1;          //
      bool                    mFlagNoTieToGround:1;              //
      bool                    mFlagGrayMapDopples:1;             //
      bool                    mFlagDopples:1;                    //
      bool                    mFlagVisibleToAll:1;               //
      bool                    mFlagSoundBehindFOW:1;             //
      bool                    mFlagNeutral:1;                    // 1 byte
      bool                    mFlagNoGrayMapDoppledInCampaign:1; // 
};
