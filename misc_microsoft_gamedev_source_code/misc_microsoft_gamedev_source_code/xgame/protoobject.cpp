//==============================================================================
// protoobject.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "protoobject.h"

// xgame
#include "database.h"
#include "configsgame.h"
#include "object.h"
#include "physicsinfomanager.h"
#include "placementrules.h"
#include "recordgame.h"
#include "squad.h"
#include "tactic.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "gamedirectories.h"
#include "visualmanager.h"
#include "world.h"
#include "worldsoundmanager.h"
#include "terrainimpactdecal.h"

#include "particlegateway.h"
#include "archivemanager.h"

// xsound
#include "soundmanager.h"

// xsystem
#include "config.h"
#include "xmlreader.h"
#include "xmlwriter.h"

// xvisual
#include "xvisual.h"

GFIMPLEMENTVERSION(BProtoObject, 3);
enum
{
   cSaveMarkerProtoObject=10000,
};

extern char* gAnimTypeNames[]; // from protovisual.cpp

//==============================================================================
// BHardpointStatic::BHardpointStatic 
//==============================================================================
BHardpointStatic::BHardpointStatic() 
{
   mYawAttachmentHandle=-1;
   mYawLeftMaxAngle=-cPi;
   mYawRightMaxAngle=cPi;
   mPitchAttachmentHandle=-1;   
   mPitchMaxAngle=cPi;
   mPitchMinAngle=-cPi;
   mMaxCombinedAngle=cPi;

   mStartYawSoundCue = cInvalidCueIndex;
   mStopYawSoundCue = cInvalidCueIndex;
   mStartPitchSoundCue = cInvalidCueIndex;
   mStopPitchSoundCue = cInvalidCueIndex;
   mYawAttachmentTransform.makeIdentity();
   mPitchAttachmentTransform.makeIdentity();     

   //-- Flags   
   mFlagHardPitchLimits=false;
   mFlagAutoCenter=true;
   mFlagSingleBoneIK=false;
   mFlagCombined=false;
   mFlagRelativeToUnit=false;
   mFlagUseYawAndPitchAsTolerance=false;               
   mFlagHardPitchLimits=false;
   mFlagInfiniteRateWhenHasTarget=true;
}

//==============================================================================
// BHardpointStatic::~BHardpointStatic
//==============================================================================
BHardpointStatic::~BHardpointStatic() 
{
}

//==============================================================================
// BHardpoint::~BHardpoint
//==============================================================================
BHardpoint::BHardpoint() 
{    
   mYawRotationRate = cPiOver12;
   mPitchRotationRate = cPiOver12;
   mFlagOwnStaticData = false;
   mpStaticData = NULL;
}

//==============================================================================
// BHardpoint::~BHardpoint
//==============================================================================
BHardpoint::~BHardpoint() 
{
}


//==============================================================================
// BHardpoint::load
//==============================================================================
bool BHardpoint::load(BXMLNode root)
{
   bool result = true;

   if (!root)
      return (false);

   BASSERT(mpStaticData == NULL);
   mpStaticData = new BHardpointStatic();
   if (!mpStaticData)
      return (false);
   mFlagOwnStaticData = true;

   //BSimString szName;
   if (!root.getAttribValueAsString("name", mpStaticData->mName))
      result = false;

   root.getAttribValueAsString("yawattachment", mpStaticData->mYawAttachmentName);
   //-- look these up in the protovisual
   mpStaticData->mYawAttachmentHandle = gVisualManager.getAttachmentType(mpStaticData->mYawAttachmentName);
 
   root.getAttribValueAsString("pitchattachment", mpStaticData->mPitchAttachmentName);
   //-- look these up in the protovisual
   mpStaticData->mPitchAttachmentHandle = gVisualManager.getAttachmentType(mpStaticData->mPitchAttachmentName);

//   if (mPitchAttachmentHandle == -1 || mYawAttachmentHandle == -1)
//      return (false);
  
   bool autocenter = true;
   root.getAttribValueAsBool("autocenter", autocenter);
   mpStaticData->mFlagAutoCenter = autocenter;

   bool singleBoneIK = false;
   root.getAttribValueAsBool("singleboneik", singleBoneIK);
   mpStaticData->mFlagSingleBoneIK = singleBoneIK;

   bool combined = false;
   root.getAttribValueAsBool("combined", combined);
   mpStaticData->mFlagCombined = combined;

   bool hardPitchLimits = false;
   root.getAttribValueAsBool("hardpitchlimits", hardPitchLimits);
   mpStaticData->mFlagHardPitchLimits = hardPitchLimits;

   bool relativeToUnit = false;
   root.getAttribValueAsBool("relativetounit", relativeToUnit);
   mpStaticData->mFlagRelativeToUnit = relativeToUnit;

   bool useYawAndPitchAsTolerance = false;
   root.getAttribValueAsBool("useYawAndPitchAsTolerance", useYawAndPitchAsTolerance);
   mpStaticData->mFlagUseYawAndPitchAsTolerance = useYawAndPitchAsTolerance;

   bool infiniteRateWhenHasTarget = false;
   root.getAttribValueAsBool("infiniteRateWhenHasTarget", infiniteRateWhenHasTarget);
   mpStaticData->mFlagInfiniteRateWhenHasTarget = infiniteRateWhenHasTarget;

   mYawRotationRate = cPiOver12;
   mPitchRotationRate = cPiOver12;
   root.getAttribValueAsAngle("yawrate", mYawRotationRate);
   root.getAttribValueAsAngle("pitchrate", mPitchRotationRate);
   
   float tempYawMaxAngle;
   if(root.getAttribValueAsAngle("yawMaxAngle", tempYawMaxAngle))
   {
      mpStaticData->mYawLeftMaxAngle =  -tempYawMaxAngle;
      mpStaticData->mYawRightMaxAngle = tempYawMaxAngle;
   }

   root.getAttribValueAsAngle("YawLeftMaxAngle", mpStaticData->mYawLeftMaxAngle);
   root.getAttribValueAsAngle("YawRightMaxAngle", mpStaticData->mYawRightMaxAngle);

   root.getAttribValueAsAngle("pitchMaxAngle", mpStaticData->mPitchMaxAngle);
   mpStaticData->mPitchMinAngle = -(mpStaticData->mPitchMaxAngle);
   root.getAttribValueAsAngle("pitchMinAngle", mpStaticData->mPitchMinAngle);

   BSimString soundCueName;
   if (root.getAttribValueAsString("StartYawSound", soundCueName))
      mpStaticData->mStartYawSoundCue = gSoundManager.getCueIndex(soundCueName);
   if (root.getAttribValueAsString("StopYawSound", soundCueName))
      mpStaticData->mStopYawSoundCue = gSoundManager.getCueIndex(soundCueName);
   if (root.getAttribValueAsString("StartPitchSound", soundCueName))
      mpStaticData->mStartPitchSoundCue = gSoundManager.getCueIndex(soundCueName);
   if (root.getAttribValueAsString("StopPitchSound", soundCueName))
      mpStaticData->mStopPitchSoundCue = gSoundManager.getCueIndex(soundCueName);

   // SLB: Force combined flag if pitch and yaw attachments are the same
   if (mpStaticData->mYawAttachmentHandle == mpStaticData->mPitchAttachmentHandle)
   {
      if(mpStaticData->mFlagUseYawAndPitchAsTolerance == false) //-- If we're not rotating this thing anyway, then we dont need to do this.
         mpStaticData->mFlagCombined = true;      
   }

   if (mpStaticData->mFlagCombined)
   {
      mpStaticData->mMaxCombinedAngle = mpStaticData->mYawRightMaxAngle;

      BASSERT((mpStaticData->mPitchAttachmentHandle == -1) || (mpStaticData->mYawAttachmentHandle == mpStaticData->mPitchAttachmentHandle));
   }
   else
   {
      BASSERT((mpStaticData->mPitchAttachmentHandle == -1) || (mpStaticData->mYawAttachmentHandle != mpStaticData->mPitchAttachmentHandle) || mpStaticData->mFlagUseYawAndPitchAsTolerance);
   }

   if(!result && mpStaticData)
   {
      delete mpStaticData;
      mpStaticData = NULL;
   }


   return (result);
}


//==============================================================================
// BProtoObjectStatic::BProtoObjectStatic
//==============================================================================
BProtoObjectStatic::BProtoObjectStatic() :
   mName(),
   mMovementType(0),
   mObstructionRadiusX(0.0f),
   mObstructionRadiusY(0.0f),
   mObstructionRadiusZ(0.0f),
   mParkingMinX(0.0f),
   mParkingMaxX(0.0f),
   mParkingMinZ(0.0f),
   mParkingMaxZ(0.0f),
   mTerrainHeightTolerance(10.0f),
   mPhysicsInfoID(-1),
   mPhysicsReplacementInfoID(-1),
   mPickRadius(0.0f),
   mPickOffset(0.0f),
   mPickPriority(cPickPriorityNone),
   mSelectType(cSelectTypeNone),
   mGotoType(cGotoTypeNone),
   mSelectedRadiusX(0.0f),
   mSelectedRadiusZ(0.0f),
   mObjectClass(cObjectClassObject),
   mProtoCorpseDeathVisualIndex(-1),
   mTrainerType(0),
   mProtoUIVisualIndex(-1),
   mMiniMapColor(cColorWhite),
   mGatherLinkObjectType(-1),
   mGatherLinkTarget(-1),
   mGatherLinkRadius(0.0f),
   mGathererLimit(-1),
   mBlockMovementObject(-1),
   mAbilityCommand(-1),
   mProtoPowerID(-1),
   mIcon(),
   mMiniMapIcon(),
   mMiniMapIconSize(1.0f),
   mExistSoundBoneName(),
   mFlightLevel(10.0f),
   mLifespan(0),
   mAIAssetValueAdjust(0.0f),
   mCombatValue(0.0f),
   mResourceAmount(0.0f),
   mPlacementRule(-1),
   mDeathFadeTime(cDefaultDeathFadeTime),
   mDeathFadeDelayTime(cDefaultDeathFadeDelayTime),
   mTrainAnimType(-1),
   mRallyPointType(-1),
   mMaxProjectileHeight(0.0f),
   mAutoLockDownType(cAutoLockNone),
   mHPBarID(-1),
   mHPBarSizeX(0.0f),
   mHPBarSizeY(0.0f),
   mHPBarOffset(0.0f, 0.0f, 0.0f),
   mGroundIKTiltFactor(0.0f),
   mGroundIKTiltBoneName(),
   mBeamHead(-1),
   mBeamTail(-1),
   mLevel(0),
   mLevelUpEffect(-1),
   mRecoveringEffect(-1),
   mAddResourceID(-1),
   mAddResourceAmount(0.0f),
   mDeathReplacement(-1),
   mSurfaceType(0),
   mStatsNameIndex(-1),
   mRolloverTextIndex(-1),
   mEnemyRolloverTextIndex(-1),
   mPrereqTextIndex(-1),
   mRoleTextIndex(-1),
   mShieldDirection(-1),
   mDamageType(cInvalidDamageTypeID),
   mSecondaryDamageTypeMode(-1),
   mExitFromDirection( cExitFromFront ),
   mUnitSelectionIconID(-1),
   mSelectionDecalID(-1),
   mpImpactDecalHandle(0),
   mAutoTrainOnBuiltID(-1),
   mSocketID(-1),
   mSocketPlayerScope(cPlayerScopePlayer),
   mProtoSquadID(-1),
   mRateID(-1),
   mBuildStatsProtoID(-1),
   mSubSelectSort(INT_MAX),
   mAttackGradeDPS(0.0f),
   mRamDodgeFactor(0.0f),
   mImpactSoundSetIndex(-1),
   mpHoveringRumbleData(NULL),
   mClumpSelectionDecalID(BFlashProperties::eInvalidFlashPropertyHandle),
   mClumpSelectionDecalMaskID(BFlashProperties::eInvalidFlashPropertyHandle),
   mStaticDecalID(BFlashProperties::eInvalidFlashPropertyHandle),
   mVisualDisplayPriority(cVisualDisplayPriorityNormal),
   mChildObjectDamageTakenScalar(0.0f),
   mGarrisonSquadMode(-1),
   mTrueLOSHeight(3.0f),
   mNumStasisFieldsToStop(1),
   mNumConversions(0),
   mReverseSpeed(-1.0f),
   mTurnRate(0.0f),
   mRepairPoints(0.0f),
   mCostEscalation(1.0f),
   mTrackingDelay(0),
   mStartingVelocity(0.0f),
   mAcceleration(0.0f),
   mFuel(0.0f),
   mPerturbanceChance(0.0f),
   mPerturbanceVelocity(0.0f),
   mPerturbanceMinTime(0.0f),
   mPerturbanceMaxTime(0.0f),
   mActiveScanChance(0.0f),
   mActiveScanRadiusScale(0.0f),
   mInitialPerturbanceVelocity(0.0f),
   mInitialPerturbanceMinTime(0.0f),
   mInitialPerturbanceMaxTime(0.0f),
   mMaxFlameEffects(-1),
   mpCaptureCosts(NULL),
   mGarrisonTime(0.0f),
   mBuildRotation(0.0f),
   mBuildOffset(cOriginVector),
   mAutoParkingLotObject(cInvalidProtoObjectID),
   mAutoParkingLotRotation(0.0f),
   mAutoParkingLotOffset(cOriginVector),
   mShieldType(-1),
   mBuildingStrengthID(-1),
   mRevealRadius(0.0f),
   // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
   mTargetBeam(cInvalidObjectID),
   mKillBeam(cInvalidObjectID),
   mMaxPopCount(0),
   mID(-1)
{
   mAbstractTypes.setNumber(gDatabase.getNumberAbstractObjectTypes());
   mAbstractTypes.clear();
   for (uint i = 0; i < cNumDamageFrom; i++)
   {
      mDamageTypes[0][i] = -1;
      mDamageTypes[1][i] = -1;
   }
   for(long i=0; i<BSquadAI::cNumberModes; i++)
      mSquadModeAnimType[i]=-1;
   for(long i=0; i<2; i++)
      mTerrainFlatten[i].zero();
   for(long i=0; i<4; i++)
      mGaiaRolloverTextIndex[i]=-1;

   mContains.clear();

   // init static flags
   mFlagRotateObstruction = true;
   mFlagPlayerOwnsObstruction = false;
   mFlagCollidable = true;
   mFlagOrientUnitWithGround = false;
   mFlagSelectedRect = false;
   mFlagBuildingCommands = false;
   mFlagBuild = false;
   mFlagManualBuild = false;
   mFlagGatherLinkSelf = false;
   mFlagHasLifespan = false;
   mFlagNonMobile = false;
   mFlagFlying = false;
   mFlagTrackPlacement = false;
   mFlagDieAtZeroResources = false;
   mFlagUnlimitedResources = false;
   mFlagHasHPBar = false;
   mFlagBlockLOS = false;
   mFlagBlockMovement = false;
   mFlagAutoRepair = false;
   mFlagInvulnerable = false;
   mFlagHasShield = false;
   mFlagFullShield = false;
   mFlagIsAffectedByGravity = false;
   mFlagHighArc = false;
   mFlagHasAmmo = false;
   mFlagCapturable = false;
   mFlagUngarrisonToGaia = false;
   mFlagPassiveGarrisoned = false;
   mFlagShowRange = false;
   mFlagTracking = false;
   mFlagDamageGarrisoned = false;
   mFlagKillGarrisoned = false;
   mFlagUIDecal = false;
   mFlagKBAware = false;
   mFlagIsExternalShield = false;
   mFlagKBCreatesBase = false;
   mFlagDestructible = false;
   mFlagVisibleForOwnerOnly = false;
   mFlagVisibleForTeamOnly = false;
   mFlagRocketOnDeath = false;
   mFlagIsBeam = false;
   mFlagDontAttackWhileMoving = false;
   mFlagFadeOnDeath = false;
   mFlagNoCull = false;
   mFlagHasTrackMask = false;
   mFlagPerturbOnce = false;
   mFlagTargetsFootOfUnit = false;
   mFlagStartAtMaxAmmo = false;
   mFlagInvulnerableWhenGaia = false;
   mFlagUpdate = false;
   mFlagNoActionOverrideMove = false;
   mFlagForceAnimRate = false;
   mFlagScaleBuildAnimRate = false;
   mFlagAlwaysVisibleOnMinimap = false;
   mFlagObscurable = false;
   mFlagNoRender = false;
   mFlagRepairable = false;
   mFlagFilterOrient = true;
   mFlagWalkToTurn = false;
   mFlagAirMovement = false;
   mFlagAutoSocket = false;
   mFlagNoBuildUnderAttack = false;
   mFlagDamageLinkedSocketsFirst = false;
   mFlagForceCreateObstruction = false;
   mFlagGatherLinkCircularSockets = false;
   mFlagDontAutoAttackMe = false;
   mFlagAlwaysAttackReviveUnits = false;
   mFlagSingleSocketBuilding = false;
   mFlagCommandableByAnyPlayer = false;
   mFlagExplodeOnTimer = false;
   mFlagExpireOnTimer = false;
   mFlagIsSticky = false;
   mFlagIsFlameEffect = false;
   mFlagIsNeedler = false;
   mFlagLinearCostEscalation = false;
   mFlagInstantTrainWithRecharge = false;
   mFlagHasPivotingEngines = false;
   mFlagDamagedDeathReplacement = false;
   mFlagShatterDeathReplacement = false;
   mFlagCanRotate = true;
   mFlagUseBuildingAction = false;
   mFlagLockdownMenu = false;
   mFlagKillChildObjectsOnDeath = false;
   mFlagSelfParkingLot = false;
   mFlagChildForDamageTakenScalar = false;
   mFlagDieLast = false;
   mFlagSingleStick = false;
   mFlagForceUpdateContainedUnits = false;
   mFlagFlattenTerrain = false;
   mFlagRegularAttacksMeleeOnly = false;
   mFlagAbilityAttacksMeleeOnly = false;
   mFlagMustOwnToSelect = false;
   mFlagShowRescuedCount = false;
   mFlagNoCorpse = false;
   mFlagNoRenderForOwner = false;
   mFlagAutoExplorationGroup = false;
   mFlagTriggersBattleMusicWhenAttacked = false;
   mFlagIsProjectileObstructable = false;
   mFlagProjectileTumbles = false;
   mFlagOneSquadContainment = false;
   mFlagIsTeleporter = false;
   mFlagNotSelectableWhenChildObject = false;
   mFlagIgnoreSquadAI = false;
   mFlagCanSetAsRallyPoint = false;
   mFlagSecondaryBuildingQueue = false;
   mFlagSelfDamage = false;
   mFlagPermanentSocket = false;
   mFlagHideOnImpact=false;
   mFlagRandomMoveAnimStart=true;
   mFlagObstructsAir=false;
   mFlagPhysicsDetonateOnDeath=false;
   mFlagSelectionDontConformToTerrain=false;
   mFlagTurnInPlace=false;
   mFlagSyncAnimRateToPhysics=false;
   mFlagIKTransitionToIdle=false;
   mFlagAppearsBelowDecals=false;
   mFlagUseRelaxedSpeedGroup=false;
   mFlagCarryNoRenderToChildren=false;
   mFlagUseBuildRotation=false;
   mFlagUseAutoParkingLot=false;
   mFlagKillOnDetach=false;
   mFlagCheckPos=false;
   mFlagCheckLOSAgainstBase=false;
   mFlagTrainerApplyFormation = false;
   mFlagAllowStickyCam=true;
}

//==============================================================================
// BProtoObjectStatic::~BProtoObjectStatic
//==============================================================================
BProtoObjectStatic::~BProtoObjectStatic()
{
   if (mpCaptureCosts != NULL)
   {
      delete []mpCaptureCosts;
      mpCaptureCosts=NULL;
   }
}

//==============================================================================
// BSingleBoneIKNode::load
//==============================================================================
bool BSingleBoneIKNode::load(BXMLNode root)
{
   if (!root)
      return false;

   // Load bone name
   if (!root.getTextAsString(mBoneName))
      return false;

   return true;
}

//==============================================================================
// BGroundIKNode::load
//==============================================================================
bool BGroundIKNode::load(BXMLNode root)
{
   if (!root)
      return false;

   // Load bone name
   if (!root.getTextAsString(mBoneName))
      return false;

   // Load IK range
   if (!root.getAttribValueAsFloat("ikRange", mIKRange))
      return false;
   BASSERT((mIKRange >= 0.0f));

   // Load link count
   long tempLong;
   if (!root.getAttribValueAsLong("linkCount", tempLong))
      return false;
   BASSERT((tempLong >= UINT8_MIN) && (tempLong <= UINT8_MAX));
   mLinkCount = static_cast<uint8>(tempLong);

   // Load x axis positioning
   float tempFloat = 0.0f;
   root.getAttribValueAsFloat("x", tempFloat);
   mFlagOnLeft = (tempFloat <= -1.0f);
   mFlagOnRight = (tempFloat >= 1.0f);

   // Load z axis positioning
   tempFloat = 0.0f;
   root.getAttribValueAsFloat("z", tempFloat);
   mFlagInFront = (tempFloat >= 1.0f);
   mFlagInBack = (tempFloat <= -1.0f);

   return true;
}

//==============================================================================
// BSweetSpotIKNode::load
//==============================================================================
bool BSweetSpotIKNode::load(BXMLNode root)
{
   if (!root)
      return false;

   // Load bone name
   if (!root.getTextAsString(mBoneName))
      return false;

   // Load link count
   long tempLong;
   if (!root.getAttribValueAsLong("linkCount", tempLong))
      return false;
   BASSERT((tempLong >= UINT8_MIN) && (tempLong <= UINT8_MAX));
   mLinkCount = static_cast<uint8>(tempLong);

   return true;
}

//==============================================================================
// BProtoObject::BProtoObject
//==============================================================================
BProtoObject::BProtoObject(long id) :
   mID(id),
   mBaseType(id),
   mDBID(-1),
   mProtoVisualIndex(-1),
   mDesiredVelocity(0.0f),
   mCommandDisabled(),
   mCommandSelectable(),
   mMaxVelocity(0.0f),
   mShieldpoints(0.0f),
   mHitpoints(0.0f),
   mLOS(0.0f),
   mSimLOS(0),
   mBuildPoints(0.0f),
   mBounty(0.0f),
   mCost(),
   mAmmoMax(0.0f),
   mAmmoRegenRate(0.0f),
   mpTactic(NULL),
   mpStaticData(NULL),
   mpSoundData(NULL),
   mRateAmount(0.0f),
   mMaxContained(0),
   mDisplayNameIndex(-1),
   mCircleMenuIconID(-1),
   mpUniqueTechStatusArray(NULL),
   mDeathSpawnSquad(-1)
{
   //-- setup our flags
   clearFlags();
}

//==============================================================================
// BProtoObject::BProtoObject
//==============================================================================
 BProtoObject::BProtoObject(const BProtoObject* pBase)
{
   // Dynamic data
   mID=pBase->mID;
   mBaseType=pBase->mBaseType;
   mDBID=pBase->mDBID;
   copyFlags(pBase);
   mProtoVisualIndex=pBase->mProtoVisualIndex;
   mDesiredVelocity=pBase->mDesiredVelocity;
   mCommandDisabled=pBase->mCommandDisabled;
   mCommandSelectable = pBase->mCommandSelectable;
   mMaxVelocity=pBase->mMaxVelocity;
   mShieldpoints=pBase->mShieldpoints;
   mHitpoints=pBase->mHitpoints;
   mLOS=pBase->mLOS;
   mSimLOS=pBase->mSimLOS;
   mBuildPoints=pBase->mBuildPoints;
   mBounty=pBase->mBounty;
   mCost=pBase->mCost;
   mAmmoMax=pBase->mAmmoMax;
   mAmmoRegenRate=pBase->mAmmoRegenRate; 
   mTrainLimits = pBase->mTrainLimits;
   mRateAmount = pBase->mRateAmount;
   mMaxContained = pBase->mMaxContained;
   mDisplayNameIndex = pBase->mDisplayNameIndex;
   mCircleMenuIconID = pBase->mCircleMenuIconID;
   mDeathSpawnSquad = pBase->mDeathSpawnSquad;
   mHardpoints = pBase->mHardpoints;

   if (pBase->mpUniqueTechStatusArray)
      mpUniqueTechStatusArray = new BUniqueTechStatusArray(*(pBase->mpUniqueTechStatusArray));
   else
      mpUniqueTechStatusArray = NULL;

   if(pBase->mpTactic)
      mpTactic=new BTactic(pBase->mpTactic, mID);
   else
      mpTactic=NULL;

   // Static data
   mpStaticData = pBase->mpStaticData;
   mFlagOwnStaticData = false;

   mpSoundData = pBase->mpSoundData;
   mFlagOwnSoundData = false;

   mFlagPlayerOwned = false;
}

//==============================================================================
// BProtoObject::~BProtoObject
//==============================================================================
BProtoObject::~BProtoObject()
{
   if (mpTactic != NULL)
   {
      delete mpTactic;
      mpTactic=NULL;
   }

   if (mpStaticData)
   {
      if (mFlagOwnStaticData)
      {
         delete mpStaticData;
         mFlagOwnStaticData=false;
      }
      mpStaticData=NULL;
   }

   if (mpSoundData)
   {
      if (mFlagOwnSoundData)
      {
         delete mpSoundData;
         mFlagOwnSoundData=false;
      }
      mpSoundData=NULL;
   }

   if (mpUniqueTechStatusArray)
   {
      delete mpUniqueTechStatusArray;
      mpUniqueTechStatusArray = NULL;
   }
}

//==============================================================================
// BProtoObject::transform
//==============================================================================
void BProtoObject::transform(const BProtoObject* pBase)
{
   if (mpTactic != NULL)
   {
      gWorld->addTransformedTactic(mpTactic);
      mpTactic=NULL;
   }

   if (mpStaticData)
   {
      if (mFlagOwnStaticData)
      {
         delete mpStaticData;
         mFlagOwnStaticData=false;
      }
      mpStaticData=NULL;
   }

   if (mpSoundData)
   {
      if (mFlagOwnSoundData)
      {
         delete mpSoundData;
         mFlagOwnSoundData=false;
      }
      mpSoundData=NULL;
   }

   // Dynamic data

   // Don't copy mID/mBaseType/mDBID... Leave it set to the original value.

   copyFlags(pBase);
   mProtoVisualIndex=pBase->mProtoVisualIndex;
   mDesiredVelocity=pBase->mDesiredVelocity;
   mCommandDisabled=pBase->mCommandDisabled;
   mCommandSelectable = pBase->mCommandSelectable;
   mMaxVelocity=pBase->mMaxVelocity;
   mHitpoints=pBase->mHitpoints;
   mShieldpoints=pBase->mShieldpoints;
   mLOS=pBase->mLOS;
   mSimLOS=pBase->mSimLOS;
   mBuildPoints=pBase->mBuildPoints;
   mBounty=pBase->mBounty;
   mCost=pBase->mCost;
   mAmmoMax=pBase->mAmmoMax;
   mAmmoRegenRate=pBase->mAmmoRegenRate;
   mDisplayNameIndex=pBase->mDisplayNameIndex;
   mCircleMenuIconID=pBase->mCircleMenuIconID;
   mDeathSpawnSquad=pBase->mDeathSpawnSquad;
   mTrainLimits = pBase->mTrainLimits;
   mHardpoints = pBase->mHardpoints;

   if(pBase->mpTactic)
      mpTactic=new BTactic(pBase->mpTactic, mID);
   else
      mpTactic=NULL;

   // Static data
   mpStaticData = pBase->mpStaticData;
   mFlagOwnStaticData = false;

   mpSoundData = pBase->mpSoundData;
   mFlagOwnSoundData = false;
}

//==============================================================================
// BProtoObject::postload
//==============================================================================
void BProtoObject::postload()
{
   if (mpTactic)
      mpTactic->postloadTactic(this);
}

//==============================================================================
// BProtoObject::load
//==============================================================================
bool BProtoObject::load(BXMLNode root)
{
   int nodeCount = root.getNumberChildren();

   // See if this is an object with no custom static data. If so, then use a shared static data to save memory.
   bool useShared = false;
   if (!gConfig.isDefined(cConfigNoProtoObjectOptimization))
   {
      if (nodeCount < 12)
      {
         useShared = true;
         for (int i = 0; i < nodeCount; i++)
         {
            BXMLNode node(root.getChild(i));
            const BPackedString name(node.getName());
            if (name == "ObjectClass")
            {
               BSimString tempStr;
               BPackedString nodeText(node.getTextPtr(tempStr));
               if (nodeText == "Object")
                  continue;
            }
            else if (name == "Visual" || name == "Sound")
               continue;
            else if (name == "Flag")
            {
               BSimString tempStr;
               BPackedString nodeText(node.getTextPtr(tempStr));
               if (nodeText == "ForceToGaiaPlayer" || nodeText == "NoTieToGround" || nodeText == "GrayMapDoppled" || 
                   nodeText == "Doppled" || nodeText == "VisibleToAll" || nodeText == "SoundBehindFOW" || nodeText == "Neutral")
                  continue;
            }
            useShared=false;
            break;
         }
      }
   }

   if (useShared)
      mpStaticData = gDatabase.getSharedProtoObjectStaticData();
   else
   {
      mpStaticData = new BProtoObjectStatic();
      if (!mpStaticData)
         return (false);
      mFlagOwnStaticData=true;
      mpStaticData->mID = mID;

      BSimString string;
      if (root.getAttribValue("name", &string))
         mpStaticData->mName.set(string);   
   }

   root.getAttribValueAsLong("dbid", mDBID);

   for (int i = 0; i < nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      BSimString tempStr;
      BPackedString nodeText(node.getTextPtr(tempStr));
      
      if (name == "MovementType")
      {
         mpStaticData->mMovementType = (WORD)gDatabase.getMovementType(nodeText);
         if (mpStaticData->mMovementType==cMovementTypeAir)
            mpStaticData->mFlagFlying = true;
      }
      else if (name == "Hardpoint")
      {         
         BHardpoint& hardpoint = mHardpoints.grow();
         bool result = hardpoint.load(node);            
         if(!result)
         {            
            BASSERT(result);
            mHardpoints.removeIndex(mHardpoints.getNumber()-1);
         }         
      }
      else if (name == "SingleBoneIK")
      {
         BSingleBoneIKNode singleBoneIKNode;
         if (singleBoneIKNode.load(node))
            mpStaticData->mSingleBoneIKNodes.add(singleBoneIKNode);
      }
      else if (name == "GroundIK")
      {
         BGroundIKNode groundIKNode;
         if (groundIKNode.load(node))
            mpStaticData->mGroundIKNodes.add(groundIKNode);
      }
      else if (name == "SweetSpotIK")
      {
         BSweetSpotIKNode sweetSpotIKNode;
         if (sweetSpotIKNode.load(node))
            mpStaticData->mSweetSpotIKNodes.add(sweetSpotIKNode);
      }
      else if (name == "ObstructionRadiusX")
         node.getTextAsFloat(mpStaticData->mObstructionRadiusX);
      else if (name == "ObstructionRadiusY")
         node.getTextAsFloat(mpStaticData->mObstructionRadiusY);
      else if (name == "ObstructionRadiusZ")
         node.getTextAsFloat(mpStaticData->mObstructionRadiusZ);
      // Automatic terrain flattening for building placement
      else if (name == "FlattenMinX0")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[0].mMinX);
      else if (name == "FlattenMaxX0")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[0].mMaxX);
      else if (name == "FlattenMinZ0")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[0].mMinZ);
      else if (name == "FlattenMaxZ0")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[0].mMaxZ);
      else if (name == "FlattenMinX1")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[1].mMinX);
      else if (name == "FlattenMaxX1")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[1].mMaxX);
      else if (name == "FlattenMinZ1")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[1].mMinZ);
      else if (name == "FlattenMaxZ1")
         node.getTextAsFloat(mpStaticData->mTerrainFlatten[1].mMaxZ);
      // Parking lot position for building placement
      else if (name == "ParkingMinX")
         node.getTextAsFloat(mpStaticData->mParkingMinX);
      else if (name == "ParkingMaxX")
         node.getTextAsFloat(mpStaticData->mParkingMaxX);
      else if (name == "ParkingMinZ")
         node.getTextAsFloat(mpStaticData->mParkingMinZ);
      else if (name == "ParkingMaxZ")
         node.getTextAsFloat(mpStaticData->mParkingMaxZ);
      else if (name == "TerrainHeightTolerance")
         node.getTextAsFloat(mpStaticData->mTerrainHeightTolerance);

      else if (name == "PhysicsInfo")
         mpStaticData->mPhysicsInfoID = gPhysicsInfoManager.getOrCreate(nodeText, true);
      else if (name == "PhysicsReplacementInfo")
         mpStaticData->mPhysicsReplacementInfoID = gPhysicsInfoManager.getOrCreate(nodeText, true);
      else if(name == "Velocity")
      {
         node.getTextAsFloat(mDesiredVelocity);
         mMaxVelocity = mDesiredVelocity * cMaxVelocityMultiplier;
         //NOTE: If you change this auto calc'ed acceleration, then you need to verify that
         //the fixup calcs in BProjectile are still correct.
         mpStaticData->mAcceleration=mDesiredVelocity*2.0f;
      }
      else if(name == "MaxVelocity")
         node.getTextAsFloat(mMaxVelocity);
      else if(name == "ReverseSpeed")
         node.getTextAsFloat(mpStaticData->mReverseSpeed);
      else if (name == "Acceleration")
         node.getTextAsFloat(mpStaticData->mAcceleration);
      else if (name == "TrackingDelay")
      {
         float trackingDelay;
         node.getTextAsFloat(trackingDelay);
         trackingDelay *= 1000.0f;
         mpStaticData->mTrackingDelay = (DWORD)trackingDelay;
      }
      else if (name == "StartingVelocity")
         node.getTextAsFloat(mpStaticData->mStartingVelocity);
      else if (name == "Fuel")
         node.getTextAsFloat(mpStaticData->mFuel);
      else if (name == "PerturbanceChance")
         node.getTextAsFloat(mpStaticData->mPerturbanceChance);
      else if (name == "PerturbanceVelocity")
         node.getTextAsFloat(mpStaticData->mPerturbanceVelocity);
      else if (name == "PerturbanceMinTime")
         node.getTextAsFloat(mpStaticData->mPerturbanceMinTime);
      else if (name == "PerturbanceMaxTime")
         node.getTextAsFloat(mpStaticData->mPerturbanceMaxTime);
      else if (name == "PerturbInitialVelocity")
      {       
         mpStaticData->mFlagPerturbOnce = true;
         node.getTextAsFloat(mpStaticData->mInitialPerturbanceVelocity);
         node.getAttribValueAsFloat("minTime", mpStaticData->mInitialPerturbanceMinTime);
         node.getAttribValueAsFloat("maxTime", mpStaticData->mInitialPerturbanceMaxTime);
      }
      else if (name == "ActiveScanChance")
      {
         node.getTextAsFloat(mpStaticData->mActiveScanChance);
         node.getAttribValueAsFloat("radiusScale", mpStaticData->mActiveScanRadiusScale);
      }
      else if (name == "TurnRate")
         node.getTextAsFloat(mpStaticData->mTurnRate);
      else if (name == "Hitpoints")
         node.getTextAsFloat(mHitpoints);
      else if (name == "Shieldpoints")
         node.getTextAsFloat(mShieldpoints);
      else if (name == "LOS")
         node.getTextAsFloat(mLOS);
      else if (name == "PickRadius")
         node.getTextAsFloat(mpStaticData->mPickRadius);
      else if (name == "PickOffset")
         node.getTextAsFloat(mpStaticData->mPickOffset);
      else if (name == "PickPriority")
         mpStaticData->mPickPriority = gDatabase.getPickPriority(nodeText);
      else if (name == "SelectType")
         mpStaticData->mSelectType = gDatabase.getSelectType(nodeText);
      else if (name == "GotoType")
         mpStaticData->mGotoType = gDatabase.getGotoType(nodeText);
      else if (name == "SelectedRadiusX")
         node.getTextAsFloat(mpStaticData->mSelectedRadiusX);
      else if (name == "SelectedRadiusZ")
         node.getTextAsFloat(mpStaticData->mSelectedRadiusZ);
      else if (name == "BuildPoints" && mBuildPoints == 0.0f)
         node.getTextAsFloat(mBuildPoints);
      else if (name == "RepairPoints")
         node.getTextAsFloat(mpStaticData->mRepairPoints);
      else if (name == "ObjectClass")
         mpStaticData->mObjectClass=gDatabase.getObjectClass(nodeText);
      else if (name == "TrainerType")
      {
         node.getTextAsInt(mpStaticData->mTrainerType);
         bool applyFormation = false;
         if (node.getAttribValueAsBool("ApplyFormation", applyFormation))
            mpStaticData->mFlagTrainerApplyFormation = applyFormation;
      }
      else if (name == "AutoLockDown")
         mpStaticData->mAutoLockDownType=gDatabase.getAutoLockDownType(nodeText);
      else if (name == "Cost")
         mCost.load(node);
      else if (name == "CostEscalation")
         node.getTextAsFloat(mpStaticData->mCostEscalation);
      else if (name == "CostEscalationObject")
      {
         long id = gDatabase.getProtoObject(nodeText);
         if (id != -1)
            mpStaticData->mCostEscalationObjects.add(id);
      }
      else if (name == "CaptureCost")
      {
         BCost cost;
         cost.load(node);
         if (cost.getTotal() != 0.0f)
         {
            int numCivs = gDatabase.getNumberCivs();
            if (!mpStaticData->mpCaptureCosts)
               mpStaticData->mpCaptureCosts=new BCost[numCivs];
            int civID=-1;
            BSimString civName;
            if (node.getAttribValue("Civ", &civName))
               civID = gDatabase.getCivID(civName);
            if (civID==-1)
            {
               for (int j=0; j<numCivs; j++)
                  mpStaticData->mpCaptureCosts[j].add(&cost);
            }
            else
               mpStaticData->mpCaptureCosts[civID].add(&cost);
         }
      }
      else if (name == "Bounty")
         node.getTextAsFloat(mBounty);
      else if (name == "AIAssetValueAdjust")
         node.getTextAsFloat(mpStaticData->mAIAssetValueAdjust);
      else if (name == "CombatValue")
         node.getTextAsFloat(mpStaticData->mCombatValue);
      else if (name == "ResourceAmount")
         node.getTextAsFloat(mpStaticData->mResourceAmount);
      else if (name == "PlacementRules")
         mpStaticData->mPlacementRule = gDatabase.getPlacementRules(nodeText);
      else if (name == "DeathFadeTime")
         node.getTextAsFloat(mpStaticData->mDeathFadeTime);
      else if (name == "DeathFadeDelayTime")
         node.getTextAsFloat(mpStaticData->mDeathFadeDelayTime);
      else if (name == "TrainAnim")
         mpStaticData->mTrainAnimType = gVisualManager.getAnimType(BSimString(nodeText));
      else if (name == "SquadModeAnim")
      {
         BSimString modeName;
         if (node.getAttribValue("Mode", &modeName))
         {
            int mode = gDatabase.getSquadMode(modeName);
            if (mode != -1)
               mpStaticData->mSquadModeAnimType[mode]=gVisualManager.getAnimType(BSimString(nodeText));
         }
      }
      else if (name == "RallyPoint")
      {
         if (node.compareText("Military") == 0)
            mpStaticData->mRallyPointType = cRallyPointMilitary;
         else if (node.compareText("Civilian") == 0)
            mpStaticData->mRallyPointType = cRallyPointCivilian;
      }
      else if (name == "MaxProjectileHeight")
         node.getTextAsFloat(mpStaticData->mMaxProjectileHeight);
      else if (name == "GroundIKTilt")
      {
         node.getAttribValueAsFloat("factor", mpStaticData->mGroundIKTiltFactor);
         node.getTextAsString(mpStaticData->mGroundIKTiltBoneName);
      }
      else if (name == "DeathReplacement")
         mpStaticData->mDeathReplacement = gDatabase.getProtoObject(nodeText);
      else if (name == "DeathSpawnSquad")
      {
         mDeathSpawnSquad = gDatabase.getProtoSquad(nodeText);
         if (node.getAttribute("CheckPos"))
            mpStaticData->mFlagCheckPos = true;
         node.getAttribValueAsLong("MaxPopCount", mpStaticData->mMaxPopCount);
      }
      else if (name == "SurfaceType")
         mpStaticData->mSurfaceType = gDatabase.getSurfaceType(nodeText);
      else if (name == "DisplayNameID")
      {
         long id;
         if (node.getTextAsLong(id))
            mDisplayNameIndex = gDatabase.getLocStringIndex(id);
      }
      else if (name == "RolloverTextID")
      {
         long id;
         if (node.getTextAsLong(id))
           mpStaticData->mRolloverTextIndex = gDatabase.getLocStringIndex(id);
      }
      else if (name == "StatsNameID")
      {
         long id;
         if (node.getTextAsLong(id))
            mpStaticData->mStatsNameIndex = gDatabase.getLocStringIndex(id);
      }
      else if (name == "GaiaRolloverTextID")
      {
         long id;
         if (node.getTextAsLong(id))
         {
            int stringIndex = gDatabase.getLocStringIndex(id);
            BSimString civName;
            if (node.getAttribValueAsString("civ", civName))
            {
               int civID=gDatabase.getCivID(civName);
               if (civID >=0 && civID<=3)
                  mpStaticData->mGaiaRolloverTextIndex[civID] = stringIndex;
            }
            else
            {
               for (int k=0; k<4; k++)
                  mpStaticData->mGaiaRolloverTextIndex[k] = stringIndex;
            }
         }
      }
      else if (name == "EnemyRolloverTextID")
      {
         long id;
         if (node.getTextAsLong(id))
            mpStaticData->mEnemyRolloverTextIndex = gDatabase.getLocStringIndex(id);
      }
      else if (name == "PrereqTextID")
      {
         long id;
         if (node.getTextAsLong(id))
            mpStaticData->mPrereqTextIndex = gDatabase.getLocStringIndex(id);
      }
      else if (name == "RoleTextID")
      {
         long id;
         if (node.getTextAsLong(id))
            mpStaticData->mRoleTextIndex = gDatabase.getLocStringIndex(id);
      }
      else if (name == "Visual")
         mProtoVisualIndex = gVisualManager.getOrCreateProtoVisual(nodeText, false);               
      else if (name == "CorpseDeath")
         mpStaticData->mProtoCorpseDeathVisualIndex = gVisualManager.getOrCreateProtoVisual(nodeText, false);
      else if (name == "AbilityCommand")
         mpStaticData->mAbilityCommand = gDatabase.getAbilityIDFromName(nodeText);
      else if (name == "Power")
         mpStaticData->mProtoPowerID = gDatabase.getProtoPowerIDByName(nodeText);
      else if (name == "Ability")
         mpStaticData->mAbilityTriggerScripts.add(BSimString(nodeText));
      else if (name == "Veterancy")
      {
         BProtoObjectLevel level;
         if (node.getAttribValueAsUInt8("Level", level.mLevel) && level.mLevel > 0)
         {
            node.getAttribValueAsHalfFloat("XP", level.mXP);
            node.getAttribValueAsHalfFloat("Damage", level.mDamage);
            node.getAttribValueAsHalfFloat("Velocity", level.mVelocity);
            node.getAttribValueAsHalfFloat("Accuracy", level.mAccuracy);
            node.getAttribValueAsHalfFloat("WorkRate", level.mWorkRate);
            node.getAttribValueAsHalfFloat("WeaponRange", level.mWeaponRange);
            node.getAttribValueAsHalfFloat("DamageTaken", level.mDamageTaken);
            mpStaticData->mLevels.add(level);
         }
      }
      else if (name == "AddResource")
      {
         mpStaticData->mAddResourceID = gDatabase.getResource(nodeText);
         node.getAttribValueAsFloat("Amount", mpStaticData->mAddResourceAmount);
      }
      else if (name == "ExistSound")
         node.getAttribValueAsString("bone", mpStaticData->mExistSoundBoneName);
      else if (name == "GathererLimit")
         node.getTextAsLong(mpStaticData->mGathererLimit);
      else if (name == "BlockMovementObject")
         mpStaticData->mBlockMovementObject = gDatabase.getProtoObject(nodeText);
      else if (name == "Lifespan")
      {
         float lifespanFloat = 0.0f;
         if (node.getTextAsFloat(lifespanFloat) && lifespanFloat != 0.0f)
         {
            mpStaticData->mLifespan = (DWORD)(1000.0f * lifespanFloat);
            mpStaticData->mFlagHasLifespan = true;
         }
      }
      else if (name == "AmmoMax")
         node.getTextAsFloat(mAmmoMax);
      else if (name == "AmmoRegenRate")
         node.getTextAsFloat(mAmmoRegenRate);
      else if (name == "NumConversions")
         node.getTextAsInt(mpStaticData->mNumConversions);
      else if (name == "NumStasisFieldsToStop")
         node.getTextAsInt(mpStaticData->mNumStasisFieldsToStop);

      
      //DCP 09/14/07: Arbitraily break this uber else if up to let it compile:)
      if (name == "Flag")
      {                  
         if (nodeText == "DontRotateObstruction")
            mpStaticData->mFlagRotateObstruction = false;
         else if (nodeText == "PlayerOwnsObstruction")
            mpStaticData->mFlagPlayerOwnsObstruction = true;
         else if (nodeText == "NonCollideable")
            mpStaticData->mFlagCollidable = false;
         else if (nodeText == "OrientUnitWithGround")
            mpStaticData->mFlagOrientUnitWithGround = true;
         else if (nodeText == "DoNotFilterOrient")
            mpStaticData->mFlagFilterOrient = false;
         else if (nodeText == "NoTieToGround")
            mFlagNoTieToGround = true;
         else if (nodeText == "SelectedRect")
            mpStaticData->mFlagSelectedRect = true;
         else if (nodeText == "Build")
            mpStaticData->mFlagBuild = true;
         else if (nodeText == "ManualBuild")
            mpStaticData->mFlagManualBuild = true;
         else if (nodeText == "Doppled")
            mFlagDopples = true;
         else if (nodeText == "GrayMapDoppled")
            mFlagGrayMapDopples = true;
         else if (nodeText == "NoGrayMapDoppledInCampaign")
            mFlagNoGrayMapDoppledInCampaign = true;
         else if (nodeText == "ForceToGaiaPlayer")
            mFlagForceToGaiaPlayer = true;
         else if (nodeText == "HasHPBar")
            mpStaticData->mFlagHasHPBar = true;
         if (nodeText == "AutoCloak")
            mFlagAutoCloak = true;
         else if (nodeText == "MoveWhileCloaked")
            mFlagCloakMove = true;
         else if (nodeText == "AttackWhileCloaked")
            mFlagCloakAttack = true;
         else if (nodeText == "Invulnerable")
            mpStaticData->mFlagInvulnerable = true;
         else if (nodeText == "Immoveable")
            mpStaticData->mFlagNonMobile = true;
         else if (nodeText == "NonRotatable")
            mpStaticData->mFlagCanRotate = false;
         else if (nodeText == "DieAtZeroResources")
            mpStaticData->mFlagDieAtZeroResources = true;
         else if (nodeText == "UnlimitedResources")
            mpStaticData->mFlagUnlimitedResources = true;
         else if (nodeText == "BlockLOS")
            mpStaticData->mFlagBlockLOS = true;
         else if (nodeText == "BlockMovement")
            mpStaticData->mFlagBlockMovement = true;
         else if (nodeText == "AutoRepair")
            mpStaticData->mFlagAutoRepair = true;
         else if (nodeText == "IsAffectedByGravity")
            mpStaticData->mFlagIsAffectedByGravity = true;
         else if (nodeText == "HighArc")
            mpStaticData->mFlagHighArc = true;
         else if (nodeText == "Capturable")
            mpStaticData->mFlagCapturable = true;
         else if (nodeText == "UngarrisonToGaia")
            mpStaticData->mFlagUngarrisonToGaia = true;
         else if (nodeText == "PassiveGarrisoned")
            mpStaticData->mFlagPassiveGarrisoned = true;
         else if (nodeText == "ShowRange")
            mpStaticData->mFlagShowRange = true;
         else if (nodeText == "Tracking")
            mpStaticData->mFlagTracking = true;
         else if (nodeText == "DamageGarrisoned")
            mpStaticData->mFlagDamageGarrisoned = true;
         else if (nodeText == "KillGarrisoned")
            mpStaticData->mFlagKillGarrisoned = true;
         else if (nodeText == "Neutral")
            mFlagNeutral = true;
         else if (nodeText == "UIDecal")
            mpStaticData->mFlagUIDecal = true;
         else if (nodeText == "KBAware")
            mpStaticData->mFlagKBAware = true;
         else if (nodeText == "ExternalShield")
            mpStaticData->mFlagIsExternalShield = true;
         else if (nodeText == "KBCreatesBase")
            mpStaticData->mFlagKBCreatesBase = true;
         else if (nodeText == "Destructible")
            mpStaticData->mFlagDestructible = true;
         else if (nodeText == "VisibleForOwnerOnly")
            mpStaticData->mFlagVisibleForOwnerOnly = true;
         else if (nodeText == "VisibleForTeamOnly")
            mpStaticData->mFlagVisibleForTeamOnly = true;
         else if (nodeText == "RocketOnDeath")
            mpStaticData->mFlagRocketOnDeath = true;
         else if (nodeText == "VisibleToAll")
            mFlagVisibleToAll = true;
         else if (nodeText == "SoundBehindFOW")
            mFlagSoundBehindFOW = true;
		   else if (nodeText == "Beam")
            mpStaticData->mFlagIsBeam = true;
         else if (nodeText == "DontAttackWhileMoving")
            mpStaticData->mFlagDontAttackWhileMoving = true;
         else if (nodeText == "FadeOnDeath")
            mpStaticData->mFlagFadeOnDeath = true;
         else if (nodeText == "NoCull")
            mpStaticData->mFlagNoCull = true;
         else if (nodeText == "HasTrackMask")
            mpStaticData->mFlagHasTrackMask = true;
         else if (nodeText == "TargetsFootOfUnit")
            mpStaticData->mFlagTargetsFootOfUnit = true;
         else if (nodeText == "StartAtMaxAmmo")
            mpStaticData->mFlagStartAtMaxAmmo = true;
         else if (nodeText == "InvulnerableWhenGaia")
            mpStaticData->mFlagInvulnerableWhenGaia = true;
         else if (nodeText == "Update")
            mpStaticData->mFlagUpdate = true;
         else if (nodeText == "NoActionOverrideMove")
            mpStaticData->mFlagNoActionOverrideMove = true;
         else if (nodeText == "ForceAnimRate")
            mpStaticData->mFlagForceAnimRate = true;
         else if (nodeText == "ScaleBuildAnimRate")
            mpStaticData->mFlagScaleBuildAnimRate = true;
         else if (nodeText == "AlwaysVisibleOnMinimap")
            mpStaticData->mFlagAlwaysVisibleOnMinimap = true;
         if (nodeText == "Obscurable")
            mpStaticData->mFlagObscurable = true;
         else if (nodeText == "NoRender")
            mpStaticData->mFlagNoRender = true;
         else if (nodeText == "Repairable")
            mpStaticData->mFlagRepairable = true;
         else if (nodeText == "WalkToTurn")
            mpStaticData->mFlagWalkToTurn = true;
         else if (nodeText == "AirMovement")
            mpStaticData->mFlagAirMovement = true;
         else if (nodeText == "NoBuildUnderAttack")
            mpStaticData->mFlagNoBuildUnderAttack = true;
         else if (nodeText == "DamageLinkedSocketsFirst")
            mpStaticData->mFlagDamageLinkedSocketsFirst = true;
         else if (nodeText == "ForceCreateObstruction")
            mpStaticData->mFlagForceCreateObstruction = true;
         else if (nodeText == "DontAutoAttackMe")
            mpStaticData->mFlagDontAutoAttackMe = true;
         else if (nodeText == "AlwaysAttackReviveUnits")
            mpStaticData->mFlagAlwaysAttackReviveUnits = true;
         else if (nodeText == "SingleSocketBuilding")
            mpStaticData->mFlagSingleSocketBuilding = true;
         else if (nodeText == "CommandableByAnyPlayer")
            mpStaticData->mFlagCommandableByAnyPlayer = true;
         else if (nodeText == "ExplodeOnTimer")
            mpStaticData->mFlagExplodeOnTimer = true;
         else if (nodeText == "ExpireOnTimer")
            mpStaticData->mFlagExpireOnTimer = true;
         else if (nodeText == "IsSticky")
            mpStaticData->mFlagIsSticky = true;
         else if (nodeText == "IsFlameEffect")
            mpStaticData->mFlagIsFlameEffect = true;
         else if (nodeText == "IsNeedler")
            mpStaticData->mFlagIsNeedler = true;
         else if (nodeText == "LinearCostEscalation")
            mpStaticData->mFlagLinearCostEscalation = true;
         else if (nodeText == "InstantTrainWithRecharge")
            mpStaticData->mFlagInstantTrainWithRecharge = true;
         else if (nodeText == "HasPivotingEngines")
            mpStaticData->mFlagHasPivotingEngines = true;
         else if (nodeText == "DamagedDeathReplacement")
            mpStaticData->mFlagDamagedDeathReplacement = true;
         else if (nodeText == "ShatterDeathReplacement")
            mpStaticData->mFlagShatterDeathReplacement = true;
         else if (nodeText == "UseBuildingAction")
            mpStaticData->mFlagUseBuildingAction = true;
         else if (nodeText == "LockdownMenu")
            mpStaticData->mFlagLockdownMenu = true;
         else if (nodeText == "AbilityDisabled")
            mFlagAbilityDisabled = true;
         else if (nodeText == "KillChildObjectsOnDeath")
            mpStaticData->mFlagKillChildObjectsOnDeath = true;
         else if (nodeText == "SelfParkingLot")
            mpStaticData->mFlagSelfParkingLot = true;
         else if (nodeText == "ChildForDamageTakenScalar")
            mpStaticData->mFlagChildForDamageTakenScalar = true;
         else if (nodeText == "DieLast")
            mpStaticData->mFlagDieLast = true;
         else if (nodeText == "SingleStick")
            mpStaticData->mFlagSingleStick = true;
         else if (nodeText == "ForceUpdateContainedUnits")
            mpStaticData->mFlagForceUpdateContainedUnits = true;
         else if (nodeText == "FlattenTerrain")
            mpStaticData->mFlagFlattenTerrain = true;
         else if (nodeText == "RegularAttacksMeleeOnly")
            mpStaticData->mFlagRegularAttacksMeleeOnly = true;
         else if (nodeText == "AbilityAttacksMeleeOnly")
            mpStaticData->mFlagAbilityAttacksMeleeOnly = true;
         else if (nodeText == "MustOwnToSelect")
            mpStaticData->mFlagMustOwnToSelect = true;
         else if (nodeText == "ShowRescuedCount")
            mpStaticData->mFlagShowRescuedCount = true;
         else if (nodeText == "NoCorpse")
            mpStaticData->mFlagNoCorpse = true;
         else if (nodeText == "NoRenderForOwner")
            mpStaticData->mFlagNoRenderForOwner = true;
         else if (nodeText == "AutoExplorationGroup")
            mpStaticData->mFlagAutoExplorationGroup = true;
		   else if (nodeText == "TriggersBattleMusicWhenAttacked")
            mpStaticData->mFlagTriggersBattleMusicWhenAttacked = true;
         else if (nodeText == "ProjectileObstructable")
            mpStaticData->mFlagIsProjectileObstructable = true;
         else if (nodeText == "ProjectileTumbles")
            mpStaticData->mFlagProjectileTumbles = true;
         else if (nodeText == "OneSquadContainment")
            mpStaticData->mFlagOneSquadContainment=true;
         else if (nodeText == "Teleporter")
            mpStaticData->mFlagIsTeleporter=true;
         else if (nodeText == "NotSelectableWhenChildObject")
            mpStaticData->mFlagNotSelectableWhenChildObject = true;
         else if (nodeText == "IgnoreSquadAI")
            mpStaticData->mFlagIgnoreSquadAI = true;
         else if (nodeText == "CanSetAsRallyPoint")
            mpStaticData->mFlagCanSetAsRallyPoint = true;
         else if (nodeText == "SecondaryBuildingQueue")
            mpStaticData->mFlagSecondaryBuildingQueue = true;
         else if (nodeText == "SelfDamage")
            mpStaticData->mFlagSelfDamage = true;
         else if (nodeText == "PermanentSocket")
            mpStaticData->mFlagPermanentSocket = true;
         else if (nodeText == "HideOnImpact")
            mpStaticData->mFlagHideOnImpact = true;
         else if (nodeText == "NoRandomMoveAnimStart")
            mpStaticData->mFlagRandomMoveAnimStart = false;
         else if (nodeText == "ObstructsAir")
            mpStaticData->mFlagObstructsAir = true;
         else if (nodeText == "PhysicsDetonateOnDeath")
            mpStaticData->mFlagPhysicsDetonateOnDeath = true;
         else if (nodeText == "SelectionDontConformToTerrain")
            mpStaticData->mFlagSelectionDontConformToTerrain = true;
         else if (nodeText == "TurnInPlace")
            mpStaticData->mFlagTurnInPlace = true;
         else if (nodeText == "SyncAnimRateToPhysics")
            mpStaticData->mFlagSyncAnimRateToPhysics = true;
         else if (nodeText == "IKTransitionToIdle")
            mpStaticData->mFlagIKTransitionToIdle = true;
         else if (nodeText == "AppearsBelowDecals")
            mpStaticData->mFlagAppearsBelowDecals = true;
         else if (nodeText == "UseRelaxedSpeedGroup")
            mpStaticData->mFlagUseRelaxedSpeedGroup = true;
         else if (nodeText == "CarryNoRenderToChildren")
            mpStaticData->mFlagCarryNoRenderToChildren = true;
         else if (nodeText == "UseBuildRotation")
            mpStaticData->mFlagUseBuildRotation = true;
         else if (nodeText == "UseAutoParkingLot")
            mpStaticData->mFlagUseAutoParkingLot = true;
         else if (nodeText == "KillOnDetach")
            mpStaticData->mFlagKillOnDetach = true;
         else if (nodeText == "CheckLOSAgainstBase")
            mpStaticData->mFlagCheckLOSAgainstBase = true;
         else if (nodeText == "NoStickyCam")
            mpStaticData->mFlagAllowStickyCam = false;
      }
      else if (name == "ObjectType")
      {
         long type = gDatabase.getObjectType(nodeText);
         long abstractType = type - gDatabase.getNumberBaseObjectTypes();
         if (abstractType >= 0 && abstractType < mpStaticData->mAbstractTypes.getNumber())
            mpStaticData->mAbstractTypes.setBit(abstractType);
      }
      else if (name == "DamageType")
         loadDamageType(node);
      else if (name == "Sound")
      {         
         BCueIndex cueIndex = gSoundManager.getCueIndex(nodeText.getPtr());         
         
         if(cueIndex == cInvalidCueIndex)
         {
            BSimString str;
            str.format("Objects.xml: Cannot find sound cue: %s", nodeText.getPtr());
            gConsole.output(cMsgWarning, str.getPtr());
         }

         if (cueIndex != cInvalidCueIndex)
         {
            BSimString typeName;
            if (node.getAttribValue("Type", &typeName))
            {
               BSoundType soundType = cObjectSoundNone;
               if (typeName == "Create")
                  soundType = cObjectSoundCreate;
               else if (typeName == "Death")
                  soundType = cObjectSoundDeath;
               else if (typeName == "Select")
                  soundType = cObjectSoundSelect;
               else if (typeName == "SelectDowned")
                  soundType = cObjectSoundSelectDowned;
               else if (typeName == "Exist")
                  soundType = cObjectSoundExist;
               else if (typeName == "StopExist")
                  soundType = cObjectSoundStopExist;
               else if (typeName == "Work")
                  soundType = cObjectSoundAckWork;
               else if (typeName == "Attack")
                  soundType = cObjectSoundAckAttack;
               else if (typeName == "Pain")
                  soundType = cObjectSoundPain;           
               else if (typeName == "StartMove")
                  soundType = cObjectSoundStartMove;                          
               else if (typeName == "StopMove")
                  soundType = cObjectSoundStopMove;           
               else if (typeName == "StepDown")
                  soundType = cObjectSoundStepDown;           
               else if (typeName == "StepUp")
                  soundType = cObjectSoundStepUp;           
               else if (typeName == "SkidOn")
                  soundType = cObjectSoundSkidOn;   
               else if (typeName == "SkidOff")
                  soundType = cObjectSoundSkidOff;   
               else if (typeName == "Jump")
                  soundType = cObjectSoundJump;   
               else if (typeName == "Land")
                  soundType = cObjectSoundLand;   
               else if (typeName == "LandHard")
                  soundType = cObjectSoundLandHard;   
               else if (typeName == "RocketStart")
                  soundType = cObjectSoundRocketStart;   
               else if (typeName == "RocketEnd")
                  soundType = cObjectSoundRocketEnd;
               else if (typeName == "ImpactDeath")
                  soundType = cObjectSoundImpactDeath;
               else if (typeName == "PieceThrownOff")
                  soundType = cObjectSoundPieceThrownOff;               
               else if (typeName == "CaptureComplete")
                  soundType = cObjectSoundCaptureComplete;
               else if (typeName == "CorpseDeath")
                  soundType = cObjectSoundCorpseDeath;
               else if (typeName == "Cloak")
                  soundType = cObjectSoundCloak;
               else if (typeName == "UnCloak")
                  soundType = cObjectSoundUncloak;
               else if (typeName == "Ability")
                  soundType = cObjectSoundAckAbility;
               else if (typeName == "AbilityJacked")
                  soundType = cObjectSoundAckAbilityJacked;
               else if (typeName == "ShieldLow")
                  soundType = cObjectSoundShieldLow;
               else if (typeName == "ShieldDepleted")
                  soundType = cObjectSoundShieldDepleted;
               else if (typeName == "ShieldRegen")
                  soundType = cObjectSoundShieldRegen;

               if (soundType != cObjectSoundNone)
               {
                  if (!mpSoundData)
                  {
                     mpSoundData = new BProtoObjectSoundData();
                     if (!mpSoundData)
                        return (false);
                     mFlagOwnSoundData=true;
                  }

                  //-- Add the sound to the list
                  BProtoSound& sound = mpSoundData->mSounds.grow();

                  //-- Load the cue Index
                  sound.mSoundCue = cueIndex;
                  sound.mSoundType = soundType;

                  //-- Load the extended cue index if it exists
                  BString extendedEventName = nodeText;
                  extendedEventName.append(L"_xtnd");
                  sound.mExtendedSoundCue = gSoundManager.getCueIndex(extendedEventName);
                  

                  //-- Load squad specific info
                  BSimString squadType;
                  if(node.getAttribValue("Squad", &squadType))
                  {
                     long squadID = gDatabase.getProtoSquad(squadType);
                     if(squadID != -1)
                        sound.mSquadID = squadID;
                  }

                  //-- Load abilityID
                  BSimString actionStr;
                  if(node.getAttribValue("Action", &actionStr))
                  {

                     BSoundStringPair pairToAdd;
                     pairToAdd.first = &sound;
                     pairToAdd.second = actionStr;
                     tempActionNameFixup.add(pairToAdd);                  
                  }
                  
                  mpSoundData->mUsedSoundTypes = mpSoundData->mUsedSoundTypes | soundType;
                                    
                  //mpSoundData->mSounds.add(sound);
               }
            }
         }
      }
      else if (name == "ImpactDecal")
      {
         mpStaticData->mpImpactDecalHandle = new BTerrainImpactDecalHandle();
         BSimString typeName;
         if (!node.getAttribValueAsFloat("sizeX", mpStaticData->mpImpactDecalHandle->mSizeX))
            mpStaticData->mpImpactDecalHandle->mSizeX=2.0f;
         if (!node.getAttribValueAsFloat("sizeZ", mpStaticData->mpImpactDecalHandle->mSizeZ))
            mpStaticData->mpImpactDecalHandle->mSizeZ=2.0f;
         if (!node.getAttribValueAsFloat("timeFullyOpaque", mpStaticData->mpImpactDecalHandle->mTimeFullyOpaque))
            mpStaticData->mpImpactDecalHandle->mTimeFullyOpaque=5.0f;
         if (!node.getAttribValueAsFloat("fadeOutTime", mpStaticData->mpImpactDecalHandle->mFadeOutTime))
            mpStaticData->mpImpactDecalHandle->mFadeOutTime=10.0f;
         if (!node.getAttribValueAsString("orientation", typeName))
         {
            mpStaticData->mpImpactDecalHandle->mOrientation = false;
         }
         else
         {
            if(typeName=="random")mpStaticData->mpImpactDecalHandle->mOrientation = false;
            else if(typeName=="forward")mpStaticData->mpImpactDecalHandle->mOrientation = true;
         }
         
         mpStaticData->mpImpactDecalHandle->mImpactTextureName = nodeText;

//         gImpactDecalManager.loadImpactDecal(nodeText);
      }
      else if (name == "ExtendedSoundBank")
      {
         mpStaticData->mExtendedSoundBankName = nodeText;
      }
      else if (name == "PortraitIcon")
         mpStaticData->mIcon = nodeText;
      else if (name == "MinimapIcon")
      {
         mpStaticData->mMiniMapIcon = nodeText;
         node.getAttribValueAsFloat("size", mpStaticData->mMiniMapIconSize);
      }
      else if (name == "MinimapColor")
      {
         float r = 0;
         float g = 0;
         float b = 0;
         node.getAttribValueAsFloat("red", r);
         node.getAttribValueAsFloat("green", g);
         node.getAttribValueAsFloat("blue", b);
         mpStaticData->mMiniMapColor.set(r,g,b);
      } 
      else if (name == "Command")
      {
         BSimString typeName;
         if (node.getAttribValue("Type", &typeName))
         {
            long type = gDatabase.getProtoObjectCommandType(typeName);
            if (type != -1)
            {
               long id = -1;

               switch (type)
               {
                  case BProtoObjectCommand::cTypeTrainSquad:
                     id = gDatabase.getProtoSquad(nodeText);
                     mpStaticData->mFlagBuildingCommands = true; // Only buildings with Train or Research commands should set this
                     break;
               
                  case BProtoObjectCommand::cTypeTrainUnit:
                     id = gDatabase.getProtoObject(nodeText);
                     mpStaticData->mFlagBuildingCommands = true; // Only buildings with Train or Research commands should set this
                     break;

                  case BProtoObjectCommand::cTypeBuild:
                     id = gDatabase.getProtoObject(nodeText);
                     break;

                  case BProtoObjectCommand::cTypeResearch:
                     id = gDatabase.getProtoTech(nodeText);
                     mpStaticData->mFlagBuildingCommands = true; // Only buildings with Train or Research commands should set this
                     break;

                  case BProtoObjectCommand::cTypeUnloadUnits:
                     id = 0;
                     break;

                  case BProtoObjectCommand::cTypeReinforce:
                     id = 0;
                     break;

                  case BProtoObjectCommand::cTypeAbility:
                     id = gDatabase.getAbilityIDFromName(nodeText);
                     break;

                  case BProtoObjectCommand::cTypeChangeMode:
                     id = gDatabase.getSquadMode(nodeText);
                     break;

                  case BProtoObjectCommand::cTypeKill:
                  case BProtoObjectCommand::cTypeCancelKill:
                  case BProtoObjectCommand::cTypeDestroyBase:
                  case BProtoObjectCommand::cTypeCancelDestroyBase:
                  case BProtoObjectCommand::cTypeReverseHotDrop:
                     id = 0;
                     break;

                  case BProtoObjectCommand::cTypeTribute:
                     id = 0;
                     break;

                  case BProtoObjectCommand::cTypePower:
                     id = gDatabase.getProtoPowerIDByName(nodeText);
                     break;

                  case BProtoObjectCommand::cTypeBuildOther:
                     id = gDatabase.getProtoObject(nodeText);
                     mpStaticData->mFlagBuildingCommands = true; // Only buildings with Train or Research commands should set this
                     break;

                  case BProtoObjectCommand::cTypeTrainLock:
                  case BProtoObjectCommand::cTypeTrainUnlock:
                     id = 0;
                     break;

                  case BProtoObjectCommand::cTypeRallyPoint:
                  case BProtoObjectCommand::cTypeClearRallyPoint:
                     id = 0;
                     break;
               }

               long position = 0;
               node.getAttribValueAsLong("Position", position);
               BProtoObjectCommand command;
               command.set(type, id, position);
               mpStaticData->mCommands.add(command);

               bool autoClose = false;
               node.getAttribValueAsBool("AutoClose", autoClose);
               if (autoClose)
               {
                  int commandCount = mpStaticData->mCommands.getNumber();
                  mpStaticData->mCommandAutoCloseMenu.setNumber(commandCount);
                  mpStaticData->mCommandAutoCloseMenu.setBit(commandCount-1);
               }
            }
         }
      }
      else if (name == "TrainLimit")
      {
         BSimString typeName;
         if (node.getAttribValue("Type", &typeName))
         {
            BProtoObjectTrainLimit trainLimit;
            trainLimit.mID = -1;
            if (typeName == "Unit")
            {
               trainLimit.mSquad = false;
               trainLimit.mID = (int16)gDatabase.getProtoObject(nodeText);
            }
            else if (typeName == "Squad")
            {
               trainLimit.mSquad = true;
               trainLimit.mID = (int16)gDatabase.getProtoSquad(nodeText);
            }
            if (trainLimit.mID != -1)
            {
               trainLimit.mCount = 0;
               node.getAttribValueAsUInt8("Count", trainLimit.mCount);
               uint8 bucket =0;
               node.getAttribValueAsUInt8("Bucket", bucket);
               trainLimit.mBucket = bucket;
               mTrainLimits.add(trainLimit);
            }
         }
      }
      else if (name == "GatherLink")
      {
         mpStaticData->mGatherLinkObjectType = gDatabase.getObjectType(nodeText);
         node.getAttribValueAsFloat("Radius", mpStaticData->mGatherLinkRadius);
         BSimString targetName;
         if (node.getAttribValue("Target", &targetName))
            mpStaticData->mGatherLinkTarget = gDatabase.getObjectType(targetName.getPtr());
         bool val = false;
         if (node.getAttribValueAsBool("Self", val))
            mpStaticData->mFlagGatherLinkSelf = val;
      }
      else if (name == "ChildObjects")
      {
         for (int j=0; j<node.getNumberChildren(); j++)
         {
            BXMLNode childNode(node.getChild(j));
            const BPackedString childName(childNode.getName());
            if (childName == "Object")
            {
               BProtoObjectChildObject childObject;
               BPackedString childNodeText(childNode.getTextPtr(tempStr));
               childObject.mID = (int16)gDatabase.getProtoObject(childNodeText);
               if (childObject.mID != -1)
               {
                  BSimString typeName;
                  BSimString boneName;
                  BSimString civName;
                  if (childNode.getAttribValue("Type", &typeName))
                  {
                     if (typeName == "ParkingLot")
                        childObject.mType = BProtoObjectChildObject::cTypeParkingLot;
                     else if (typeName == "Socket")
                        childObject.mType = BProtoObjectChildObject::cTypeSocket;
                     else if (typeName == "Rally")
                        childObject.mType = BProtoObjectChildObject::cTypeRally;
                     else if (typeName == "OneTimeSpawnSquad")
                     {
                        // For this type the object ID is the proto squad ID rather than proto object
                        childObject.mType = BProtoObjectChildObject::cTypeOneTimeSpawnSquad;
                        childObject.mID = (int16)gDatabase.getProtoSquad(childNodeText);
                     }
                     else if (typeName == "Unit")
                        childObject.mType = BProtoObjectChildObject::cTypeUnit;
                     else if (typeName == "Foundation")
                        childObject.mType = BProtoObjectChildObject::cTypeFoundation;
                  }
                  if (childNode.getAttribValue("AttachBone", &boneName))
                  {
                     childObject.mAttachBoneName = boneName;
                  }

                  BVector offset;
                  if (childNode.getAttribValueAsVector("Offset", offset))
                  {
                     childObject.mOffsetX=offset.x;
                     childObject.mOffsetZ=offset.z;
                  }

                  childNode.getAttribValueAsAngle("Rotation", childObject.mRotation);

                  if (childNode.getAttribValue("UserCiv", &civName))
                     childObject.mUserCiv = (int8)gDatabase.getCivID(civName);

                  mpStaticData->mChildObjects.add(childObject);
               }
            }
         }
      }
      else if (name == "Pop")
      {
         BSimString typeName;
         if (node.getAttribValue("type", &typeName))
         {
            long popID;
            popID = gDatabase.getPop(typeName);
            if (popID != -1)
            {
               float popCount = 0.0f;
               if (node.getTextAsFloat(popCount))
               {
                  BPop pop;
                  pop.mID = (short)popID;
                  pop.mCount = popCount;
                  mpStaticData->mPops.add(pop);
               }
            }
         }
      }
      else if (name == "PopCapAddition")
      {
         BSimString typeName;
         if (node.getAttribValue("type", &typeName))
         {
            long popID;
            popID = gDatabase.getPop(typeName);
            if (popID != -1)
            {
               float popCount = 0.0f;
               if (node.getTextAsFloat(popCount))
               {
                  BPop pop;
                  pop.mID = (short)popID;
                  pop.mCount = popCount;
                  mpStaticData->mPopCapAdditions.add(pop);
               }
            }
         }
      }
      else if (name == "Tactics")
      {         
         //-- pre load the tactic... then load it when everybody is done
         if (!nodeText.isEmpty())
         {
            mpTactic = new BTactic();
            if (mpTactic)
            {
               mpTactic->setProtoObjectID(mID);
               mpTactic->preloadTactic(nodeText);
            }
         }
      }
      else if (name == "FlightLevel")
      {
         node.getTextAsFloat(mpStaticData->mFlightLevel);
      }
      else if (name == "ExitFromDirection")
      {
         node.getTextAsLong(mpStaticData->mExitFromDirection);
      }
      else if (name == "HPBar")
      {
         mpStaticData->mHPBarID = gDatabase.getProtoHPBarID(nodeText);

         mpStaticData->mHPBarSizeX = 0.0f;
         mpStaticData->mHPBarSizeY = 0.0f;
         
         node.getAttribValueAsFloat("sizeX", mpStaticData->mHPBarSizeX);
         node.getAttribValueAsFloat("sizeY", mpStaticData->mHPBarSizeY);

         mpStaticData->mHPBarOffset = XMVectorZero();
         node.getAttribValueAsVector("offset", mpStaticData->mHPBarOffset);            
      }
      else if (name == "HitZone")
      {
         BHitZone hz;
         hz.setAttachmentName(BSimString(nodeText));

         float data = -1.0f;
         node.getAttribValueAsFloat("Hitpoints", data);
         hz.setHitpoints(data);

         data = -1.0f;
         node.getAttribValueAsFloat("Shieldpoints", data);
         hz.setShieldpoints(data);

         bool flag = false;
         node.getAttribValueAsBool("Active", flag);
         hz.setActive(flag);

         flag = false;
         node.getAttribValueAsBool("HasShields", flag);
         hz.setHasShields(flag);

         mpStaticData->mHitZoneList.uniqueAdd(hz);
      }
      else if (name == "BeamHead")
      {
         long objectType = gDatabase.getObjectType(nodeText);
         if (objectType != -1)
            mpStaticData->mBeamHead= objectType;
      }
      else if (name == "BeamTail")
      {
         long objectType = gDatabase.getObjectType(nodeText);
         if (objectType != -1)
            mpStaticData->mBeamTail= objectType;
      }
      else if (name == "Level")
         node.getTextAsInt(mpStaticData->mLevel);
      else if (name == "LevelUpEffect")
         mpStaticData->mLevelUpEffect = gDatabase.getProtoObject(nodeText);
      else if (name == "RecoveringEffect")
         mpStaticData->mRecoveringEffect = gDatabase.getProtoObject(nodeText);
      else if (name == "AutoTrainOnBuilt")
         mpStaticData->mAutoTrainOnBuiltID = gDatabase.getProtoSquad(nodeText);
      else if (name == "Socket")
      {
         mpStaticData->mSocketID = gDatabase.getObjectType(nodeText);
         mpStaticData->mSocketPlayerScope=cPlayerScopePlayer;
         BSimString scopeText;
         if (node.getAttribValueAsString("player", scopeText))
         {
            int scope=gDatabase.getPlayerScope(scopeText);
            if (scope!=-1)
               mpStaticData->mSocketPlayerScope=scope;
         }
         bool autoSocket=false;
         node.getAttribValueAsBool("AutoSocket", autoSocket);
         mpStaticData->mFlagAutoSocket=autoSocket;
      }
      else if (name == "Rate")
      {
         BSimString rateText;
         if (node.getAttribValueAsString("rate", rateText))
         {
            mpStaticData->mRateID = gDatabase.getRate(rateText);
            node.getTextAsFloat(mRateAmount);
         }
      }
      else if (name == "MaxContained")
         node.getTextAsInt(mMaxContained);
      else if (name == "MaxFlameEffects")
         node.getTextAsInt(mpStaticData->mMaxFlameEffects);
      else if (name == "Contain")
      {
         mpStaticData->mContains.uniqueAdd(gDatabase.getObjectType(nodeText));
      }
      else if (name == "GarrisonSquadMode")
      {
         mpStaticData->mGarrisonSquadMode = gDatabase.getSquadMode(nodeText);
      }
      else if (name == "BuildStatsObject")
         mpStaticData->mBuildStatsProtoID = gDatabase.getProtoObject(nodeText);
      else if (name == "SubSelectSort")
         node.getTextAsInt(mpStaticData->mSubSelectSort);
      else if (name == "AttackGradeDPS")
         node.getTextAsFloat(mpStaticData->mAttackGradeDPS);
      else if (name == "RamDodgeFactor")
         node.getTextAsFloat(mpStaticData->mRamDodgeFactor);
      else if (name == "HoveringRumble")
      {
         mpStaticData->mpHoveringRumbleData = new BRumbleEvent();
         if (mpStaticData->mpHoveringRumbleData)
         {
            BSimString szName;
            if (node.getAttribValueAsString("LeftRumbleType", szName))
               mpStaticData->mpHoveringRumbleData->mLeftRumbleType=(int8)BGamepad::getRumbleType(szName);
            if (node.getAttribValueAsString("RightRumbleType", szName))
               mpStaticData->mpHoveringRumbleData->mRightRumbleType=(int8)BGamepad::getRumbleType(szName);
            node.getAttribValueAsHalfFloat("Duration", mpStaticData->mpHoveringRumbleData->mDuration);
            node.getAttribValueAsHalfFloat("LeftStrength", mpStaticData->mpHoveringRumbleData->mLeftStrength);
            node.getAttribValueAsHalfFloat("RightStrength", mpStaticData->mpHoveringRumbleData->mRightStrength);
         }
      }
      else if (name == "VisualDisplayPriority")
      {
         mpStaticData->mVisualDisplayPriority = gDatabase.getVisualDisplayPriority(nodeText);
      }
      else if (name == "ChildObjectDamageTakenScalar")
         node.getTextAsFloat(mpStaticData->mChildObjectDamageTakenScalar);
      else if (name == "TrueLOSHeight")
         node.getTextAsFloat(mpStaticData->mTrueLOSHeight);
      else if (name == "GarrisonTime")
         node.getTextAsFloat(mpStaticData->mGarrisonTime);
      else if (name == "BuildRotation")
         node.getTextAsFloat(mpStaticData->mBuildRotation);
      else if (name == "BuildOffset")
         node.getTextAsVector(mpStaticData->mBuildOffset);
      else if (name == "AutoParkingLot")
      {
         mpStaticData->mAutoParkingLotObject = gDatabase.getProtoObject(nodeText);
         node.getAttribValueAsFloat("Rotation", mpStaticData->mAutoParkingLotRotation);
         node.getAttribValueAsVector("Offset", mpStaticData->mAutoParkingLotOffset);
      }
      else if (name =="BuildingStrengthDisplay")
      {
         mpStaticData->mBuildingStrengthID = gDatabase.getProtoBuildingStrengthID(node.getTextPtr(tempStr));         
      }
      else if (name == "ShieldType")
      {
         long objectType = gDatabase.getObjectType(nodeText);
         if (objectType != -1)
            mpStaticData->mShieldType= objectType;
      }
      else if (name == "RevealRadius")
      {
         node.getTextAsFloat(mpStaticData->mRevealRadius);
      }
      // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
      else if (name == "TargetBeam")
      {
         mpStaticData->mTargetBeam = gDatabase.getProtoObject(nodeText);
      }
      else if (name == "KillBeam")
      {
         mpStaticData->mKillBeam = gDatabase.getProtoObject(nodeText);
      }
   }

   // Default some values
   if (mpStaticData->mSelectedRadiusX == 0.0f)
      mpStaticData->mSelectedRadiusX = mpStaticData->mObstructionRadiusX;
   if (mpStaticData->mSelectedRadiusZ == 0.0f)
      mpStaticData->mSelectedRadiusZ = mpStaticData->mObstructionRadiusZ;
   if (mpStaticData->mObstructionRadiusY == 0.0f)
      mpStaticData->mObstructionRadiusY = getObstructionRadius();
  
   //-- finish loading the tactic
   if (mpTactic)
   {
      if (!mpTactic->loadTactic(this))
      {
         delete mpTactic;
         mpTactic = NULL;
      }
   }

   uint count = tempActionNameFixup.getSize();
   for(uint i=0; i < count; i++)
   {
      //-- get the Action ID
      BSimString actionStr = tempActionNameFixup[i].second;  
      long actionID = getTactic()->getProtoActionID(actionStr);      
      BASSERT(actionID < INT8_MAX);
      tempActionNameFixup[i].first->mActionID =  static_cast<int8>(actionID);
   }
   tempActionNameFixup.clear();

   
   if (!gArchiveManager.getArchivesEnabled())
   {
      loadAllAssets();
   }

   return (true);
}


//==============================================================================
// BProtoObject::loadAllAssets
//==============================================================================
void BProtoObject::loadAllAssets()
{
   //if(getFlag(cFlagAreAllAssetsLoaded))
   //   return;

   if(mpStaticData->mpImpactDecalHandle)
   {
      gImpactDecalManager.loadImpactDecal(mpStaticData->mpImpactDecalHandle->mImpactTextureName);
   }

   //setFlag(cFlagAreAllAssetsLoaded, true);
}


//==============================================================================
// BProtoObject::unloadAllAssets
//==============================================================================
void BProtoObject::unloadAllAssets()
{
   //if(!getFlag(cFlagAreAllAssetsLoaded))
   //   return;
}


//==============================================================================
// BProtoObject::getImpactSoundCue
//==============================================================================
bool BProtoObject::getImpactSoundCue(byte surfaceType, BImpactSoundInfo& soundInfo) const
{
   const BImpactSoundInfo* pSoundInfo = gDatabase.getImpactSound(getImpactSoundSet(), surfaceType);
   if (pSoundInfo)
   {
      soundInfo = *pSoundInfo;
      return true;
   }
   else
   {
      return false;
   }
}

//==============================================================================
// BProtoObject::isType
//==============================================================================
bool BProtoObject::isType(BObjectTypeID type) const
{
   if(type<0)
      return false;

   //Check for a base type match.
   if (type < gDatabase.getNumberBaseObjectTypes())
   {
      if (type == mBaseType)
         return (true);
      else
         return (false);
   }

   // Check POID
   if(type==mID)
      return true;

   if (static_cast<uint>(type) >= static_cast<uint>(gDatabase.getNumberObjectTypes()))
      return false;

   long typeOffset = gDatabase.getNumberBaseObjectTypes();
   if(type<typeOffset)
      return false;

   long abstractType=type-typeOffset;
   if(mpStaticData->mAbstractTypes.isBitSet(abstractType)>(DWORD)0)
      return true;

   return false;
}

//==============================================================================
// BProtoObject::getTrainLimit
//==============================================================================
long BProtoObject::getTrainLimit(long id, bool squad, uint8* pBucketOut) const
{
   if (pBucketOut)
      *pBucketOut=0;

   long numLimits = mTrainLimits.getNumber();
   for (long i = 0; i < numLimits; i++)
   {
      const BProtoObjectTrainLimit& limit = mTrainLimits[i];
      if (limit.mID == id && limit.mSquad == squad)
      {
         if (pBucketOut)
            *pBucketOut=limit.mBucket;
         return limit.mCount;
      }
   }

   return -1;
}

//==============================================================================
// BProtoObject::setTrainLimit
//==============================================================================
void BProtoObject::setTrainLimit(long id, bool squad, long count)
{
   long numLimits = mTrainLimits.getNumber();
   for (long i = 0; i < numLimits; i++)
   {
      BProtoObjectTrainLimit& limit = mTrainLimits[i];
      if (limit.mID == id && limit.mSquad == squad)
      {
         limit.mCount = (uint8) count;
         return;
      }
   }
}

//==============================================================================
// BProtoObject::findHardpoint
//==============================================================================
long BProtoObject::findHardpoint(const char* szName) const
{
   long count = mHardpoints.getNumber();
   for (long i=0; i < count; i++)
   {
      if (mHardpoints[i].mpStaticData->mName.compare(szName)==0)
      {
         return i;
      }
   }

   return -1;
}


//==============================================================================
// BProtoObject::getCost
//==============================================================================
void BProtoObject::getCost(const BPlayer* pPlayer, BCost* pCost, int countAdjustment) const
{
   *pCost=mCost;
   if((!mpStaticData->mFlagLinearCostEscalation && mpStaticData->mCostEscalation!=1.0f) || (mpStaticData->mFlagLinearCostEscalation && mpStaticData->mCostEscalation != 0.0f))
   {
//-- FIXING PREFIX BUG ID 4031
      const BPlayer* pCoopPlayer=NULL;
//--
      if (gWorld->getFlagCoop() && pPlayer->getCoopID()!=cInvalidPlayerID && mpStaticData->mObjectClass == cObjectClassBuilding)
         pCoopPlayer=gWorld->getPlayer(pPlayer->getCoopID());
      int existingCount = countAdjustment;
      uint escalationCount = mpStaticData->mCostEscalationObjects.getSize();
      if(escalationCount > 0)
      {
         for(uint i=0; i<escalationCount; i++)
         {
            BProtoObjectID protoObjectID = mpStaticData->mCostEscalationObjects[i];
            existingCount += (pPlayer->getNumUnitsOfType(protoObjectID) + pPlayer->getFutureUnitCount(protoObjectID));
            if (pCoopPlayer)
               existingCount += (pCoopPlayer->getNumUnitsOfType(protoObjectID) + pCoopPlayer->getFutureUnitCount(protoObjectID));
         }
      }
      else
      {
         existingCount += (pPlayer->getNumUnitsOfType(mID) + pPlayer->getFutureUnitCount(mID));
         if (pCoopPlayer)
            existingCount += (pCoopPlayer->getNumUnitsOfType(mID) + pCoopPlayer->getFutureUnitCount(mID));
      }

      if (mpStaticData->mFlagLinearCostEscalation)
      {
         if (existingCount > 0)
         {
            float additionalCost = mpStaticData->mCostEscalation * existingCount;
            for(int i=0; i<pCost->getNumberResources(); i++)
            {
               float v=pCost->get(i);
               if (v!=0.0f)
                  pCost->set(i, v+additionalCost);
            }
         }
      }
      else
      {
         if (existingCount > 0)
         {
            for(int i=0; i<existingCount; i++)
               (*pCost)*=mpStaticData->mCostEscalation;

            // round up to the 10s so 128 would = 130
            for(int i=0; i<pCost->getNumberResources(); i++)
            {
               float a=pCost->get(i);
               if (a!=0.0f)
               {
                  a=a*0.1f;
                  int b=(int)(a+0.5f);
                  float c=b*10.0f;
                  pCost->set(i, c);
               }
            }
         }
      }
   }
}

//==============================================================================
// BProtoObject::getCaptureCost
//==============================================================================
const BCost* BProtoObject::getCaptureCost(const BPlayer* pPlayer) const
{
   if (!mpStaticData->mpCaptureCosts)
      return NULL;
   int civID = pPlayer->getCivID();
   return &(mpStaticData->mpCaptureCosts[civID]);   
}

//==============================================================================
// BProtoObject::loadDamageType
//==============================================================================
void BProtoObject::loadDamageType(BXMLNode  node)
{
   BSimString tempStr;
   BDamageTypeID damageType = gDatabase.getDamageType(node.getTextPtr(tempStr));
   BSimString directionName;
   node.getAttribValue("direction", &directionName);
   int direction = gDatabase.getDamageDirection(directionName.getPtr());

   int mode = 0;
   int squadMode = -1;
   BSimString modeName = "";
   node.getAttribValue("mode", &modeName);
   if ((modeName == "Normal") || (modeName == ""))
   {
      mode = 0;
      squadMode = BSquadAI::cModeNormal;
   }
   else
   {
      mode = 1;
      squadMode = gDatabase.getSquadMode(modeName);
   }   

   if (damageType == gDatabase.getDamageTypeShielded())
   {
      if (mpStaticData->mShieldDirection == -1)
      {
         if ((direction != cDamageDirectionFull) && (direction != cDamageDirectionFrontHalf))
         {
            BFAIL("BProtoObject::loadDamageType -- That shield configuration isn't supported.");
         }
         else
         {
            mpStaticData->mShieldDirection = direction;
            mpStaticData->mFlagHasShield = true;
            if (direction == cDamageDirectionFull)
               mpStaticData->mFlagFullShield = true;
         }
      }
      else
      {
         BFAIL("BProtoObject::loadDamageType -- Shield defined more than once.");
      }
   }
   else
   {
      switch (direction)
      {
      case cDamageDirectionFull:
         for (long i = 0; i < BProtoObjectStatic::cNumDamageFrom; i++)
            addDamageType(damageType, mode, i, true, squadMode);
         break;
      case cDamageDirectionFrontHalf:
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromFrontLeft, false, squadMode);
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromFront, false, squadMode);
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromFrontRight, false, squadMode);
         break;
      case cDamageDirectionBackHalf:
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromBackLeft, false, squadMode);
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromBack, false, squadMode);
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromBackRight, false, squadMode);
         break;
      case cDamageDirectionRight:
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromFrontRight, false, squadMode);
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromBackRight, false, squadMode);
         break;
      case cDamageDirectionLeft:
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromFrontLeft, false, squadMode);
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromBackLeft, false, squadMode);
         break;
      case cDamageDirectionFront:
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromFront, false, squadMode);
         break;
      case cDamageDirectionBack:
         addDamageType(damageType, mode, BProtoObjectStatic::cDamageFromBack, false, squadMode);
         break;
      }
   }
   
   if (mode == 0 && (direction == cDamageDirectionFull || mpStaticData->mDamageType == cInvalidDamageTypeID) && gDatabase.isBaseDamageType(damageType))
      mpStaticData->mDamageType = damageType;
}

//==============================================================================
// BProtoObject::addDamageType
//==============================================================================
void BProtoObject::addDamageType(BDamageTypeID damageType, int mode, int damageFrom, bool ignoreOverlap, int squadMode)
{
   if (mode!=BSquadAI::cModeNormal && mode!=-1)
   {
      if (mpStaticData->mSecondaryDamageTypeMode==-1)
         mpStaticData->mSecondaryDamageTypeMode = (int8)squadMode;
      else if (mpStaticData->mSecondaryDamageTypeMode != squadMode)
         BFAIL("BProtoObject::addDamageType -- Multiple secondary damage type mdoes not supported.");
   }
   if (mpStaticData->mDamageTypes[mode][damageFrom] == -1)
      mpStaticData->mDamageTypes[mode][damageFrom] = damageType;
   else if (!ignoreOverlap)
      BFAIL("BProtoObject::addDamageType -- Damage type direction overlap.");
}

//==============================================================================
// BProtoObject::getDamageType
//==============================================================================
BDamageTypeID BProtoObject::getDamageType(BVector damageDirection, BVector unitForward, BVector unitRight, bool testShield, bool testArmor, int mode) const
{
   // Check the shield
   if (testShield && (mpStaticData->mShieldDirection != -1))
   {
      if ((mpStaticData->mShieldDirection == cDamageDirectionFull) || (damageDirection.dot(unitForward) <= 0.0f))
         return (gDatabase.getDamageTypeShielded());
   }

   BDamageTypeID damageType = cInvalidDamageTypeID;

   if (testArmor)
   {
      // Figure out which direction the damage is coming from
      damageDirection.normalize();
      float cosTheta = damageDirection.dot(unitForward);
      int direction = -1;

      // Front half
      if (cosTheta <= 0.0f)
      {
         // Front
         if (cosTheta <= -0.70710678118654752440084436210485f)
            direction = BProtoObjectStatic::cDamageFromFront;
         else
         {
            // Front right
            if (damageDirection.dot(unitRight) <= 0.0f)
               direction = BProtoObjectStatic::cDamageFromFrontRight;
            // Front left
            else
               direction = BProtoObjectStatic::cDamageFromFrontLeft;
         }
      }
      // Back half
      else
      {
         // Back
         if (cosTheta >= 0.70710678118654752440084436210485f)
            direction = BProtoObjectStatic::cDamageFromBack;
         else
         {
            // Back right
            if (damageDirection.dot(unitRight) <= 0.0f)
               direction = BProtoObjectStatic::cDamageFromBackRight;
            // Back left
            else
               direction = BProtoObjectStatic::cDamageFromBackLeft;
         }
      }

      damageType = mpStaticData->mDamageTypes[mode][direction];

      // Pick damage type from default mode if no damage type is specified for this mode
      if ((mode != 0) && (damageType == cInvalidDamageTypeID))
         damageType = mpStaticData->mDamageTypes[0][direction];
   }

   return (damageType);
}

//==============================================================================
// BProtoObject::getDisplayName
//==============================================================================
void BProtoObject::getDisplayName(BUString& string) const
{
   if(mDisplayNameIndex==-1)
      string=mpStaticData->mName.getPtr(); // AJL FIXME - Need to return empty string or other value to indicate missing string
   else
      string=gDatabase.getLocStringFromIndex(mDisplayNameIndex);
}

//==============================================================================
// BProtoObject::getStatsName
//==============================================================================
void BProtoObject::getStatsName(BUString& string) const
{
   if (mpStaticData->mStatsNameIndex == -1)
      getDisplayName(string);
   else
      string=gDatabase.getLocStringFromIndex(mpStaticData->mStatsNameIndex);
}

//==============================================================================
// BProtoObject::getRolloverText
//==============================================================================
void BProtoObject::getRolloverText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mRolloverTextIndex);
}

//==============================================================================
// BProtoObject::getGaiaRolloverText
//==============================================================================
void BProtoObject::getGaiaRolloverText(int civID, BUString& string) const
{
   if (civID>=0 && civID<=3)
      string=gDatabase.getLocStringFromIndex(mpStaticData->mGaiaRolloverTextIndex[civID]);
   else
      string.empty();
}

//==============================================================================
// BProtoObject::getEnemyRolloverText
//==============================================================================
void BProtoObject::getEnemyRolloverText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mEnemyRolloverTextIndex);
}

//==============================================================================
// BProtoObject::getPrereqText
//==============================================================================
void BProtoObject::getPrereqText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mPrereqTextIndex);
}

//==============================================================================
// BProtoObject::getRoleText
//==============================================================================
void BProtoObject::getRoleText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mRoleTextIndex);
}

//==============================================================================
//==============================================================================
bool BProtoObject::getHasAttackRatings() const
{
   if (!mpTactic)
      return false;
   return mpTactic->getHasAttackRatings();
}

//==============================================================================
//==============================================================================
float BProtoObject::getAttackRatingDPS(BDamageTypeID damageType) const
{
   if (!mpTactic)
      return 0.0f;
   return mpTactic->getAttackRatingDPS(damageType);
}


//==============================================================================
//==============================================================================
float BProtoObject::getAttackRatingDPS() const
{
   if (!mpTactic)
      return (0.0f);
   return (mpTactic->getAttackRatingDPS());
}


//==============================================================================
//==============================================================================
float BProtoObject::getAttackRating(BDamageTypeID damageType) const
{
   if (!mpTactic)
      return 0.0f;
   return mpTactic->getAttackRating(damageType);
}


//==============================================================================
//==============================================================================
float BProtoObject::getAttackRating() const
{
   if (!mpTactic)
      return (0.0f);
   return (mpTactic->getAttackRating());
}


//==============================================================================
//==============================================================================
float BProtoObject::getDefenseRating() const
{
   return gDatabase.getDefenseRating(mHitpoints + mShieldpoints);
}

//==============================================================================
//==============================================================================
float BProtoObject::getStrength() const
{
   return (getAttackRating() * getDefenseRating());
}

//==============================================================================
//==============================================================================
uint BProtoObject::getAttackGrade(BDamageTypeID damageType) const
{
   return gDatabase.getAttackGrade(mpStaticData->mAttackGradeDPS, getAttackRatingDPS(damageType));
}

//==============================================================================
//==============================================================================
void BProtoObject::clearFlags(void)
{   
   mFlagOwnStaticData = false;
   mFlagOwnSoundData = false;
   mFlagAvailable = false;
   mFlagForbid = false;
   mFlagAbilityDisabled = false;
   mFlagAutoCloak = false;
   mFlagCloakMove = false;
   mFlagCloakAttack = false;
   mFlagUniqueInstance = false;
   mFlagPlayerOwned = false;

   mFlagNoTieToGround = false;
   mFlagDopples = false;
   mFlagForceToGaiaPlayer = false;
   mFlagGrayMapDopples = false;
   mFlagNeutral = false;
   mFlagVisibleToAll = false;
   mFlagSoundBehindFOW = false;
   mFlagNoGrayMapDoppledInCampaign = false;
}

//==============================================================================
//==============================================================================
void BProtoObject::copyFlags(const BProtoObject* pBase)
{   
   mFlagAvailable = pBase->mFlagAvailable;
   mFlagForbid = pBase->mFlagForbid;
   mFlagAbilityDisabled = pBase->mFlagAbilityDisabled;
   mFlagAutoCloak = pBase->mFlagAutoCloak;
   mFlagCloakMove = pBase->mFlagCloakMove;
   mFlagCloakAttack = pBase->mFlagCloakAttack;
   mFlagUniqueInstance = pBase->mFlagUniqueInstance;

   mFlagNoTieToGround = pBase->mFlagNoTieToGround;
   mFlagDopples = pBase->mFlagDopples;
   mFlagForceToGaiaPlayer = pBase->mFlagForceToGaiaPlayer;
   mFlagGrayMapDopples = pBase->mFlagGrayMapDopples;
   mFlagNeutral = pBase->mFlagNeutral;
   mFlagVisibleToAll = pBase->mFlagVisibleToAll;
   mFlagSoundBehindFOW = pBase->mFlagSoundBehindFOW;
   mFlagNoGrayMapDoppledInCampaign = pBase->mFlagNoGrayMapDoppledInCampaign;
}

//==============================================================================
// BProtoObject::playUISound
//==============================================================================
bool BProtoObject::playUISound(BSoundType soundType, bool suppressBankLoad, BVector position, long squadID, long actionID) const
{
   BCueIndex cueIndex = cInvalidCueIndex;
   if (gWorld->getWorldSoundManager()->getExtendedSoundBankLoaded(getExtendedSoundBankName()))
   {
      cueIndex = getSound(soundType, squadID, true, actionID); //-- try the extended cue index
      if(cueIndex == cInvalidCueIndex)
         cueIndex = getSound(soundType, squadID, false, actionID); //-- fallback to the regular index
   }
   else
   {         
      cueIndex = getSound(soundType, squadID, false, actionID);
      if(suppressBankLoad == false)
         gWorld->getWorldSoundManager()->loadExtendedSoundBank(getExtendedSoundBankName());
   }

   if(gConfig.isDefined(cConfigUIWorldSounds))
   {
      gWorld->getWorldSoundManager()->addSound(position, cueIndex, false, cInvalidCueIndex, false, false);
   }
   else
      gSoundManager.playCue(cueIndex);

   if (gRecordGame.isRecording())
      gRecordGame.recordUIUnitSound(mID, soundType, suppressBankLoad, position);

   // Let the caller know whether we were successful in finding a cue index
   return (cueIndex != cInvalidCueIndex);
}

//==============================================================================
// BProtoObject::getCommandAutoCloseMenu
//==============================================================================
bool BProtoObject::getCommandAutoCloseMenu(int type, int id) const
{
   for (uint i=0; i<getNumberCommands(); i++)
   {
      BProtoObjectCommand command=getCommand(i);
      if (type==command.getType() && id==command.getID())
         return getCommandAutoCloseMenu(i);
   }
   return false;
}

//==============================================================================
// BProtoObject::getSound
//==============================================================================
BCueIndex BProtoObject::getSound(BSoundType soundType, long squadID, bool extendedBankSound, long abilityID) const 
{
   if (!mpSoundData)
      return cInvalidCueIndex;

   if((mpSoundData->mUsedSoundTypes & soundType) == 0x00000000)
      return cInvalidCueIndex;

   BCueIndex defaultCueIndex = cInvalidCueIndex;

   uint numSounds = mpSoundData->mSounds.getSize();
   for(uint i=0; i < numSounds; i++)
   {
      if(mpSoundData->mSounds[i].mSoundType == soundType)
      {
         if(mpSoundData->mSounds[i].mSquadID == squadID && abilityID == mpSoundData->mSounds[i].mActionID)                  
         {            
            //-- Perfect match
            if(extendedBankSound)
               return mpSoundData->mSounds[i].mExtendedSoundCue;
            else
               return mpSoundData->mSounds[i].mSoundCue;
         }                  

         //-- Check squadID
         bool isDefault = true;
         if(mpSoundData->mSounds[i].mSquadID != cInvalidProtoID && mpSoundData->mSounds[i].mSquadID != squadID)
            isDefault = false;

         //-- Check abilityID
         if(mpSoundData->mSounds[i].mActionID != -1 && mpSoundData->mSounds[i].mActionID != abilityID)
            isDefault = false;

         if(isDefault)
         {
            if(extendedBankSound)
               defaultCueIndex = mpSoundData->mSounds[i].mExtendedSoundCue;
            else
               defaultCueIndex = mpSoundData->mSounds[i].mSoundCue;
         }
      }
   }

   BDEBUG_ASSERT(defaultCueIndex != cInvalidCueIndex);

   return defaultCueIndex;
}

//==============================================================================
//==============================================================================
void BProtoObject::addUniqueTechStatus(long techID, long status)
{
   if (!mpUniqueTechStatusArray)
      mpUniqueTechStatusArray = new BUniqueTechStatusArray();

   // Search for matching entry to overwrite
   for (int i = 0; i < mpUniqueTechStatusArray->getNumber(); i++)
   {
      BUniqueTechStatus& temp = (*mpUniqueTechStatusArray)[i];
      if (temp.mTechID == techID)
      {
         temp.mTechStatus = status;
         return;
      }
   }

   // If no matching techID, add new entry
   mpUniqueTechStatusArray->add(BUniqueTechStatus(techID, status));
}

//==============================================================================
//==============================================================================
bool BProtoObject::getUniqueTechStatus(long techID, long& status) const
{
   if (!mpUniqueTechStatusArray)
      return false;
   BASSERT(getFlagUniqueInstance());

   // Search for matching techID in array and return its status
   for (int i = 0; i < mpUniqueTechStatusArray->getNumber(); i++)
   {
//-- FIXING PREFIX BUG ID 4035
      const BUniqueTechStatus& temp = (*mpUniqueTechStatusArray)[i];
//--
      if (temp.mTechID == techID)
      {
         status = temp.mTechStatus;
         return true;
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BProtoObject::save(BStream* pStream, int saveType) const
{
   // mpStaticData;
   GFWRITEVAR(pStream, BProtoObjectID, mpStaticData->mID);

   //GFWRITEVAR(pStream, long, mDBID);

   GFWRITEARRAY(pStream, BProtoObjectTrainLimit, mTrainLimits, uint8, 100);

   // mHardPoints
   uint numHardpoints = mHardpoints.size();
   GFWRITEVAL(pStream, uint8, numHardpoints);
   GFVERIFYCOUNT(numHardpoints, 20);
   for (uint i=0; i<numHardpoints; i++)
   {
      const BHardpoint& hp = mHardpoints[i];
      GFWRITEVAL(pStream, float, hp.getYawRotationRate());
      GFWRITEVAL(pStream, float, hp.getPitchRotationRate());
   }

   GFWRITECLASS(pStream, saveType, mCost);
   GFWRITEVAR(pStream, long, mProtoVisualIndex);
   GFWRITEVAR(pStream, float, mDesiredVelocity);
   GFWRITEVAR(pStream, float, mMaxVelocity);
   GFWRITEVAR(pStream, float, mHitpoints);
   GFWRITEVAR(pStream, float, mShieldpoints);
   GFWRITEVAR(pStream, float, mLOS);
   GFWRITEVAR(pStream, long, mSimLOS);
   GFWRITEVAR(pStream, float, mBuildPoints);
   GFWRITEVAR(pStream, float, mBounty);

   bool haveTactic = (mpTactic != NULL);
   GFWRITEVAR(pStream, bool, haveTactic);
   if (haveTactic && !mpTactic->save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, float, mAmmoMax);
   GFWRITEVAR(pStream, float, mAmmoRegenRate);
   GFWRITEVAR(pStream, float, mRateAmount);
   GFWRITEVAR(pStream, int, mMaxContained);
   GFWRITEVAR(pStream, long, mDisplayNameIndex);
   GFWRITEVAR(pStream, int, mCircleMenuIconID);
   GFWRITEVAR(pStream, long, mDeathSpawnSquad);
   GFWRITEUTBITVECTOR(pStream, mCommandDisabled, uint8, 32);
   GFWRITEUTBITVECTOR(pStream, mCommandSelectable, uint8, 32);
   //bool mFlagOwnStaticData:1;
   //bool mFlagOwnSoundData:1;
   GFWRITEBITBOOL(pStream, mFlagAvailable);
   GFWRITEBITBOOL(pStream, mFlagForbid);
   GFWRITEBITBOOL(pStream, mFlagAbilityDisabled);
   GFWRITEBITBOOL(pStream, mFlagAutoCloak);
   GFWRITEBITBOOL(pStream, mFlagCloakMove);
   GFWRITEBITBOOL(pStream, mFlagCloakAttack);
   GFWRITEBITBOOL(pStream, mFlagUniqueInstance);
   //bool mFlagPlayerOwned:1;
   //bool mFlagForceToGaiaPlayer:1;
   //bool mFlagNoTieToGround:1;
   //bool mFlagGrayMapDopples:1;
   //bool mFlagDopples:1;
   //bool mFlagVisibleToAll:1;
   //bool mFlagSoundBehindFOW:1;
   //bool mFlagNeutral:1;
   //bool mFlagNoGrayMapDoppledInCampaign:1;

   GFWRITEMARKER(pStream, cSaveMarkerProtoObject);
   return true;
}

//==============================================================================
//==============================================================================
bool BProtoObject::load(BStream* pStream, int saveType, BPlayer* pPlayer)
{
   BProtoObjectID staticProtoID = mID;

   if (mGameFileVersion >= 3)
   {
      // mpStaticData;
      GFREADVAR(pStream, BProtoObjectID, staticProtoID);
      gSaveGame.remapProtoObjectID(staticProtoID);
      if (staticProtoID != -1 && staticProtoID != mID)
      {
         const BProtoObject* pFromProtoObject = pPlayer->getProtoObject(staticProtoID);
         mpStaticData = pFromProtoObject->mpStaticData;
         mFlagOwnStaticData = false;

         if (mpTactic)
         {
            gWorld->addTransformedTactic(mpTactic);
            mpTactic=NULL;
         }
         const BTactic* pFromTactic = pFromProtoObject->getTactic();
         if (pFromTactic)
            mpTactic = new BTactic(pFromTactic, mID);

         mHardpoints = pFromProtoObject->mHardpoints;
      }
   }

   //GFREADVAR(pStream, long, mDBID);

   GFREADARRAY(pStream, BProtoObjectTrainLimit, mTrainLimits, uint8, 100);
   uint count = mTrainLimits.size();
   for (uint i=0; i<count; i++)
   {
      if (mTrainLimits[i].mSquad)
         gSaveGame.remapProtoSquadID(mTrainLimits[i].mID);
      else
         gSaveGame.remapProtoObjectID(mTrainLimits[i].mID);
   }

   if (mGameFileVersion >= 3)
   {
      // mHardPoints
      uint numHardpoints = mHardpoints.size();
      uint numSavedHardpoints;
      GFREADVAL(pStream, uint8, uint, numSavedHardpoints);
      GFVERIFYCOUNT(numSavedHardpoints, 20);
      for (uint i=0; i<numSavedHardpoints; i++)
      {
         float yawRate, pitchRate;
         GFREADVAR(pStream, float, yawRate);
         GFREADVAR(pStream, float, pitchRate);
         if (i < numHardpoints)
         {
            BHardpoint& hp = mHardpoints[i];
            hp.setYawRotationRate(yawRate);
            hp.setPitchRotationRate(pitchRate);
         }
      }
   }

   GFREADCLASS(pStream, saveType, mCost);
   if (mGameFileVersion >= 3)
   {
      GFREADVAR(pStream, long, mProtoVisualIndex);
      gSaveGame.remapProtoVisualID(mProtoVisualIndex);
   }
   GFREADVAR(pStream, float, mDesiredVelocity);
   GFREADVAR(pStream, float, mMaxVelocity);
   GFREADVAR(pStream, float, mHitpoints);
   GFREADVAR(pStream, float, mShieldpoints);
   GFREADVAR(pStream, float, mLOS);
   GFREADVAR(pStream, long, mSimLOS);
   GFREADVAR(pStream, float, mBuildPoints);
   GFREADVAR(pStream, float, mBounty);

   bool haveTactic = (mpTactic != NULL);
   GFREADVAR(pStream, bool, haveTactic);
   if (haveTactic)
   {
      if (!mpTactic)
      {
         mpTactic = new BTactic();
         if (!mpTactic)
            return false;
      }
      if (!mpTactic->load(pStream, saveType, pPlayer, staticProtoID))
         return false;
   }
   else
   {
      if (mpTactic)
      {
         delete mpTactic;
         mpTactic = NULL;
      }
   }

   GFREADVAR(pStream, float, mAmmoMax);
   GFREADVAR(pStream, float, mAmmoRegenRate);
   GFREADVAR(pStream, float, mRateAmount);
   GFREADVAR(pStream, int, mMaxContained);
   GFREADVAR(pStream, long, mDisplayNameIndex);
   GFREADVAR(pStream, int, mCircleMenuIconID);
   if (BProtoObject::mGameFileVersion >= 2)
   {
      GFREADVAR(pStream, long, mDeathSpawnSquad);
   }
   GFREADUTBITVECTOR(pStream, mCommandDisabled, uint8, 32);
   GFREADUTBITVECTOR(pStream, mCommandSelectable, uint8, 32);
   //bool mFlagOwnStaticData:1;
   //bool mFlagOwnSoundData:1;
   GFREADBITBOOL(pStream, mFlagAvailable);
   GFREADBITBOOL(pStream, mFlagForbid);
   GFREADBITBOOL(pStream, mFlagAbilityDisabled);
   GFREADBITBOOL(pStream, mFlagAutoCloak);
   GFREADBITBOOL(pStream, mFlagCloakMove);
   GFREADBITBOOL(pStream, mFlagCloakAttack);
   GFREADBITBOOL(pStream, mFlagUniqueInstance);
   //bool mFlagPlayerOwned:1;
   //bool mFlagForceToGaiaPlayer:1;
   //bool mFlagNoTieToGround:1;
   //bool mFlagGrayMapDopples:1;
   //bool mFlagDopples:1;
   //bool mFlagVisibleToAll:1;
   //bool mFlagSoundBehindFOW:1;
   //bool mFlagNeutral:1;

   GFREADMARKER(pStream, cSaveMarkerProtoObject);
   return true;
}
