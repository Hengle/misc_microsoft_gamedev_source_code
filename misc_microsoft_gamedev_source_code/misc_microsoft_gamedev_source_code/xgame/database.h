//==============================================================================
// database.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "action.h"
#include "playercolor.h"
#include "D3DTextureManager.h"
#include "cost.h"
#include "visualmanager.h"
#include "simtypes.h"
#include "damagetype.h"
#include "containers\staticarray.h"
#include "ScenarioList.h"
#include "simtypes.h"
#include "tips.h"
#include "burningEffectLimits.h"
#include "..\xsound\xsound.h"
#include "powertypes.h"
#include "containers\hashTable.h"

#ifndef BUILD_FINAL
   #include "reloadManager.h"
#endif

// Forward declarations
class BAbility;
class BCiv;
class BDatabase;
class BFileWatcher;
class BLeader;
class BGameSettings;
class BProtoObject;
class BPlacementRules;
class BProtoPower;
class BProtoTech;
class BProtoSquad;
class BWeaponType;
class BGameMode;
class BProtoHPBar;
class BProtoHPBarColorStages;
class BProtoImpactEffect;
class BProtoVeterancyBar;
class BProtoPieProgress;
class BProtoBobbleHead;
class BProtoBuildingStrength;
class BProtoObjectStatic;

// Global variable for the one BDatabase object
extern BDatabase gDatabase;


// Impact effect sizes
enum
{
   cImpactEffectSizeSmall,
   cImpactEffectSizeMedium,
   cImpactEffectSizeLarge,

   cNumImpactEffectSizes
};

enum
{
   cReticleAttackNoEffectAgainst,
   cReticleAttackWeakAgainst,
   cReticleAttackFairAgainst,
   cReticleAttackGoodAgainst,
   cReticleAttackExtremeAgainst,

   cNumReticleAttackGrades
};

// Damage directions
enum
{
   cDamageDirectionFull,
   cDamageDirectionFrontHalf,
   cDamageDirectionBackHalf,
   cDamageDirectionFront,
   cDamageDirectionBack,
   cDamageDirectionLeft,
   cDamageDirectionRight,

   cNumDamageDirections
};

// Object class types
enum
{
   cObjectClassObject,
   cObjectClassSquad,
   cObjectClassBuilding,
   cObjectClassUnit,
   cObjectClassProjectile,
};

// Movement types
enum 
{
   cMovementTypeNone     = 0x0000,
   cMovementTypeLand     = 0x0001,
   cMovementTypeFlood    = 0x0002,
   cMovementTypeScarab   = 0x0004,
   cMovementTypeAir      = 0x0008,
   cMovementTypeNonSolid = 0x0010,
   cMovementTypeHover    = 0x0020,
};

// Pick priority types
enum
{
   cPickPriorityNone,
   cPickPriorityBuilding,
   cPickPriorityResource,
   cPickPriorityUnit,
   cPickPriorityRally,
};

// Select types
enum
{
   cSelectTypeNone,
   cSelectTypeUnit,
   cSelectTypeCommand,
   cSelectTypeTarget,
   cSelectTypeSingleUnit,
   cSelectTypeSingleType,
};

// Goto types
enum
{
   cGotoTypeNone,
   cGotoTypeBase,
   cGotoTypeMilitary,
   cGotoTypeInfantry,
   cGotoTypeVehicle,
   cGotoTypeAir,
   cGotoTypeCivilian,
   cGotoTypeScout,
   cGotoTypeNode,
   cGotoTypeAlert,
   cGotoTypeArmy,

   cGotoTypeHero,
};

// Rally point types
enum
{
   cRallyPointMilitary,
   cRallyPointCivilian,
};

// Relations
enum
{
   cRelationTypeAny,
   cRelationTypeSelf,
   cRelationTypeAlly,
   cRelationTypeEnemy,
   cRelationTypeNeutral,
};

//-- Squad Sound State
typedef enum
{
   cSquadSoundStateIdle = 0,
   cSquadSoundStateMove,
   cSquadSoundStateMoveAttack,
   cSquadSoundStateAttack,
   cSquadSoundStateMax,
   cSquadSoundStateInvalid,
} BSquadSoundState;

// Object sounds
// Note: I'm using this both as a bitmask and unique identifier. This way I can quickly know
//       if an item is filled out our not and early out.
enum BSoundType
{
   // UI Sound Events
   cObjectSoundNone        =     0x00000000,
   cObjectSoundCreate      =     0x00000001,
   cObjectSoundDeath       =     0x00000002,
   cObjectSoundSelect      =     0x00000004,
   cObjectSoundAckWork     =     0x00000008,
   cObjectSoundAckAttack   =     0x00000010,
   cObjectSoundCaptureComplete = 0x00000020,
   cObjectSoundAckAbility      = 0x00000040,
   cObjectSoundAckAbilityJacked= 0x00000080,


   // World Sound Events   
   cObjectSoundStopExist   =     0x00000100,
   cObjectSoundStepDown    =     0x00000200,
   cObjectSoundStepUp      =     0x00000400,
   cObjectSoundSkidOn      =     0x00000800,
   cObjectSoundSkidOff     =     0x00001000,
   cObjectSoundRocketStart =     0x00002000,
   cObjectSoundRocketEnd   =     0x00004000,
   cObjectSoundStartMove   =     0x00008000, //-- Ramp up sound leading to moving sound
   cObjectSoundCorpseDeath =     0x00010000,
   cObjectSoundStopMove    =     0x00020000, //-- The ramp down sound
   cObjectSoundJump        =     0x00040000,
   cObjectSoundLand        =     0x00080000,
   cObjectSoundImpactDeath =     0x00100000,
   cObjectSoundPieceThrownOff =  0x00200000,
   cObjectSoundLandHard    =     0x00400000,

   cObjectSoundSelectDowned =    0x00800000,
   //cObjectSoundShieldRegen =     0x01000000,

   cObjectSoundPain        =     0x02000000,
   cObjectSoundCloak       =     0x04000000,
   cObjectSoundUncloak     =     0x08000000,
   cObjectSoundExist       =     0x10000000,

   // Shield sounds
   cObjectSoundShieldLow          = 0x20000000,
   cObjectSoundShieldDepleted     = 0x40000000,
   cObjectSoundShieldRegen        = 0x80000000
};

enum BSquadSoundType
{
   // Squad UI Sounds   
   cSquadSoundNone               = 0x00000000,
   cSquadSoundExist              = 0x00000001,
   cSquadSoundStopExist          = 0x00000002,
   cSquadSoundChatterMove        = 0x00000004,      
   cSquadSoundChatterAttack      = 0x00000008,
   cSquadSoundChatterMoveAttack  = 0x00000010,
   cSquadSoundChatterIdle        = 0x00000020,
   cSquadSoundChatterAllyKilled  = 0x00000040,
   cSquadSoundChatterKilledEnemy = 0x00000080,
   cSquadSoundChatterCheer       = 0x00000100,   
   cSquadSoundChatterLevelUp     = 0x00000200,

   // Reaction Sounds
   cSquadSoundChatterReactBirth         = 0x00000400,
   cSquadSoundChatterReactDeath         = 0x00000800,
   cSquadSoundChatterReactJoinBattle    = 0x00001000,
   cSquadSoundChatterReactPowCarpet     = 0x00002000,
   cSquadSoundChatterReactPowOrbital    = 0x00004000,
   cSquadSoundChatterReactPowCleansing  = 0x00008000,
   cSquadSoundChatterReactPowCryo       = 0x00010000,
   cSquadSoundChatterReactPowRage       = 0x00020000,
   cSquadSoundChatterReactPowWave       = 0x00040000,
   cSquadSoundChatterReactPowDisruption = 0x00080000,
   cSquadSoundChatterReactFatalityUNSC  = 0x00100000,
   cSquadSoundChatterReactFatalityCOV   = 0x00200000,
   cSquadSoundChatterReactJacking       = 0x00400000,
   cSquadSoundChatterReactCommandeer    = 0x00800000,
   cSquadSoundChatterReactHotDrop       = 0x01000000,

   // Movement Sounds
   cSquadSoundStartMove          = 0x02000000,
   cSquadSoundStopMove           = 0x04000000,

   // Misc sounds
   cSquadSoundKamikaze           = 0x08000000,

   // Special Brute Jump Sounds
   cSquadSoundStartJump          = 0x10000000,
   cSquadSoundStopJump           = 0x20000000,
};

// Ability Types
enum
{
   cAbilityWork,
   cAbilityChangeMode,
   cAbilityUnload,
   cAbilityUnpack,
   cAbilityCommandMenu,
   cAbilityPower,
};

enum
{
   cPlayerScopeAny=0,
   cPlayerScopePlayer,
   cPlayerScopeTeam,
   cPlayerScopeEnemy,
   cPlayerScopeGaia,
};

enum
{
   cLifeScopeAny = 0,
   cLifeScopeAlive,
   cLifeScopeDead,
};

// Auto LockDown types
enum
{
   cAutoLockNone = 0,
   cAutoLockAndUnlock,
   cAutoLockOnly,
   cAutoUnlockOnly,
};

// Max player colors
enum
{
   cMaxPlayerColors = 16
};

enum BPlayerColorCategory
{
   cPlayerColorCategorySkirmish  = 0,
   cPlayerColorCategorySPC       = 1,
   
   cMaxPlayerColorCategories     = 2
};

enum
{
   cVisualDisplayPriorityNormal = 1,
   cVisualDisplayPriorityCombat,
   cVisualDisplayPriorityHigh,
};

enum
{
   cRecoverMove,
   cRecoverAttack,
   cRecoverAbility,
};

enum
{
   cWorldNone = 0,
   cWorldHarvest,
   cWorldArcadia,
   cWorldSWI,
   cWorldSWE
};


//==============================================================================
// BImpactSoundInfo
//==============================================================================
class BImpactSoundInfo
{
public:
   BImpactSoundInfo() : mSoundCue(cInvalidCueIndex), mCheckSoundRadius(true) {}
   BCueIndex mSoundCue;
   bool mCheckSoundRadius;
};

//==============================================================================
// BImpactSoundSet
//==============================================================================
class BImpactSoundSet
{
public:
   BImpactSoundSet() {}
   BSimString                                         mName;
   BSmallDynamicSimArray<BImpactSoundInfo>            mImpactSounds;
};

//==============================================================================
//==============================================================================
class BDatabaseObjectMap
{
   public:
      BDatabaseObjectMap() :
         mPOID1(cInvalidProtoObjectID),
         mPOID2(cInvalidProtoObjectID),
         mPSID2(cInvalidProtoSquadID)
         { };
         
      BProtoObjectID       getPOID1() const { return (mPOID1); }
      void                 setPOID1(BProtoObjectID v) { mPOID1=v; }

      BProtoObjectID       getPOID2() const { return (mPOID2); }
      void                 setPOID2(BProtoObjectID v) { mPOID2=v; }

      BProtoSquadID        getPSID2() const { return (mPSID2); }
      void                 setPSID2(BProtoSquadID v) { mPSID2=v; }
   protected:
      BProtoObjectID       mPOID1;
      BProtoObjectID       mPOID2;
      BProtoSquadID        mPSID2;
};


typedef BDynamicSimArray<long,4,BDynamicArrayNoConstructOptions> BTechUnitDependencyArray;
typedef BInt8<BDamageTypeID, -1, INT8_MAX-1> BDamageTypeIDSmall;

//==============================================================================
//==============================================================================
class BRumbleEvent
{
   public:
      enum
      {
         cTypeTrigger,
         cTypeImpact,
         cTypeCollision,
         cTypeAnimTag,
         cTypeFlare,
         cTypeAttack,
         cTypeTrainComplete,
         cTypeResearchComplete,
         cTypeBuildComplete,
         cTypeNewObjective,
         cTypeObjectiveComplete,
         cTypeUnitSelect,
         cTypePaintSelect,
         cTypeUnitHover,
         cTypeUnitHovering,
         cTypeConfirmCommand,
         cTypeCantDoCommand,
         cTypeUIRollover,
         cTypeRadialMenuItem,
         cNumEvents
      };

      BHalfFloat mLeftStrength;
      BHalfFloat mRightStrength;
      BHalfFloat mDuration;
      int8  mLeftRumbleType;
      int8  mRightRumbleType;
      int8  mPattern;
      bool  mEnabled:1;
      bool  mMultiple:1;
};

//==============================================================================
// BDatabase
//==============================================================================
class BDatabase : public IProtoVisualHandler
{
   public:    

                           BDatabase();
                           ~BDatabase();

      bool                 setup();
      void                 postloadProtoObjects();
      int                  run();
      void                 shutdown();

      void                 update();      

      byte                 getNumberSurfaceTypes() const { return mNumberSurfaceTypes; }
      byte                 getSurfaceType(const char* pName) const { byte index; if(mSurfaceTypes.find(pName, &index)) return index; else return 0; }
      const char*          getSurfaceTypeName(byte type) const;
      long                 getSurfaceTypeImpactEffect() const { return mSurfaceTypeImpactEffect; }

      long                 getNumberProtoObjects() const { return (long)mProtoObjects.size(); }
      BProtoObject*        getGenericProtoObject(BProtoObjectID protoObjectID) const;
      BProtoObjectID       getProtoObject(const char* pName) const { BProtoObjectID protoObjectID; if(mObjectTypes.find(pName, &protoObjectID) && isValidProtoObject(protoObjectID)) return (protoObjectID); else return (cInvalidProtoObjectID); }
      const char*          getProtoObjectName(BProtoObjectID protoObjectID) const { return getObjectTypeName(protoObjectID); }
      float                getAttackRevealerLOS() const { return(mAttackRevealerLOS); }
      DWORD                getAttackRevealerLifespan() const { return(mAttackRevealerLifespan); }
      float                getAttackedRevealerLOS() const { return(mAttackedRevealerLOS); }
      DWORD                getAttackedRevealerLifespan() const { return(mAttackedRevealerLifespan); }
      float                getMinimumRevealerSize() const { return(mMinimumRevealerSize); }
      DWORD                getBuildingSelfDestructTime() const { return(mBuildingSelfDestructTime); }
      float                getTributeAmount() const { return mTributeAmount; }
      float                getTributeCost() const { return mTributeCost; }
      float                getUnscSupplyPadBonus() const { return (mUnscSupplyPadBonus); }
      float                getUnscSupplyPadBreakEvenPoint() const { return (mUnscSupplyPadBreakEvenPoint); }
      float                calculateUnscSupplyPadModifier(float numSupplyPads) const;
      float                getCovSupplyPadBonus() const { return (mCovSupplyPadBonus); }
      float                getCovSupplyPadBreakEvenPoint() const { return (mCovSupplyPadBreakEvenPoint); }
      float                calculateCovSupplyPadModifier(float numSupplyPads) const;
      int                  getLeaderPowerChargeResourceID() const { return mLeaderPowerChargeResourceID; }
      int                  getLeaderPowerChargeRateID() const { return mLeaderPowerChargeRateID; }
      float                getDamageReceivedXPFactor() const { return mDamageReceivedXPFactor; }
      
      long                 getNumberObjectTypes() const { return mNumberObjectTypes; }
      long                 getNumberBaseObjectTypes() const { return mNumberBaseObjects; }
      long                 getNumberAbstractObjectTypes() const { return mNumberAbstractObjectTypes; }
      long                 getObjectType(const char* pName) { long index; if(mObjectTypes.find(pName, &index)) return index; else return -1; }
      const char*          getObjectTypeName(long type) const;

      // Cached proto object IDs
      BProtoObjectID       getPOIDRevealer() const { return (mPOIDRevealer); }
      BProtoObjectID       getPOIDBlocker() const { return (mPOIDBlocker); }
      BProtoObjectID       getPOIDUnitStart() const { return mPOIDUnitStart; }
      BProtoObjectID       getPOIDRallyStart() const { return mPOIDRallyStart; }
      BProtoObjectID       getPOIDAIEyeIcon() const { return (mPOIDAIEyeIcon); }
      BProtoObjectID       getPOIDObstruction() const { return (mPOIDObstruction); }
      BProtoObjectID       getPOIDPhysicsThrownObject() const { return (mPOIDPhysicsThrownObject); }
      BProtoObjectID       getPOIDScn07Scarab() const { return (mPOIDScn07Scarab); }
      BProtoObjectID       getPSIDSkirmishScarab() const { return (mPSIDSkirmishScarab); }
      BProtoObjectID       getPSIDCobra() const { return (mPSIDCobra); }
      BProtoObjectID       getPSIDVampire() const { return (mPSIDVampire); }
      BProtoObjectID       getPOIDLeaderPowerCaster() const { return mPOIDLeaderPowerCaster; }
      BProtoSquadID        getPSIDLeaderPowerCaster() const { return mPSIDLeaderPowerCaster; }

      // SPC Heroes
      BProtoObjectID       getPOIDForge() const { return (mPOIDForge); }
      BProtoObjectID       getPOIDForgeWarthog() const { return (mPOIDForgeWarthog); }
      BProtoObjectID       getPOIDAnders() const { return (mPOIDAnders); }
      BProtoObjectID       getPOIDSpartanRocket() const { return (mPOIDSpartanRocket); }
      BProtoObjectID       getPOIDSpartanMG() const { return (mPOIDSpartanMG); }
      BProtoObjectID       getPOIDSpartanSniper() const { return (mPOIDSpartanSniper); }

      // Cached object type IDs
      BObjectTypeID        getOTIDBuilding() const { return mOTIDBuilding; }
      BObjectTypeID        getOTIDBuildingSocket() const { return (mOTIDBuildingSocket); }
      BObjectTypeID        getOTIDTurretSocket() const { return (mOTIDTurretSocket); }
      BObjectTypeID        getOTIDTurretBuilding() const { return (mOTIDTurretBuilding); }      
      BObjectTypeID        getOTIDSettlement() const { return (mOTIDSettlement); }
      BObjectTypeID        getOTIDBase() const { return (mOTIDBase); }
      BObjectTypeID        getOTIDIcon() const { return (mOTIDIcon); }
      BObjectTypeID        getOTIDGatherable() const { return (mOTIDGatherable); }
      BObjectTypeID        getOTIDInfantry() const { return (mOTIDInfantry); }
      BObjectTypeID        getOTIDTransporter() const { return (mOTIDTransporter); }
      BObjectTypeID        getOTIDTransportable() const { return (mOTIDTransportable); }
      BObjectTypeID        getOTIDFlood() const { return (mOTIDFlood); }
      BObjectTypeID        getOTIDCover() const { return (mOTIDCover); }
      BObjectTypeID        getOTIDLOSObstructable() const { return (mOTIDLOSObstructable); }
      BObjectTypeID        getOTIDObjectiveArrow() const { return (mOTIDObjectiveArrow); }
      BObjectTypeID        getOTIDCampaignHero() const { return (mOTIDCampaignHero); }
      BObjectTypeID        getOTIDHero() const { return (mOTIDHero); }
      BObjectTypeID        getOTIDHeroDeath() const { return(mOTIDHeroDeath); }
      BObjectTypeID        getOTIDHotDropPickup() const { return (mOTIDHotDropPickup); }
      BObjectTypeID        getOTIDTeleportPickup() const { return (mOTIDTeleportPickup); }
      BObjectTypeID        getOTIDTeleportDropoff() const { return(mOTIDTeleportDropoff); }      
      BObjectTypeID        getOTIDGroundVehicle() const { return(mOTIDGroundVehicle); }      
      BObjectTypeID        getOTIDGarrison() const { return (mOTIDGarrison); }
      BObjectTypeID        getOTIDUnscSupplyPad() const { return (mOTIDUnscSupplyPad); }
      BObjectTypeID        getOTIDCovSupplyPad() const { return (mOTIDCovSupplyPad); }
      BObjectTypeID        getOTIDWallShield() const { return(mOTIDWallShield); }
      BObjectTypeID        getOTIDBaseShield() const { return(mOTIDBaseShield); }
      BObjectTypeID        getOTIDLeader() const { return (mOTIDLeader); }
      BObjectTypeID        getOTIDCovenant() const { return (mOTIDCovenant); }
      BObjectTypeID        getOTIDUnsc() const { return (mOTIDUnsc); }
      BObjectTypeID        getOTIDBlackBox() const { return (mOTIDBlackBox); }
      BObjectTypeID        getOTIDSkull() const { return (mOTIDSkull); }
      BObjectTypeID        getOTIDBirthOnTop() const { return (mOTIDBirthOnTop); }
      BObjectTypeID        getOTIDCanCryo() const { return (mOTIDCanCryo); }
      BObjectTypeID        getOTIDHook() const { return (mOTIDHook); }

      // Cached effect IDs
      BObjectTypeID        getOTIDStun() const { return (mOTIDStun); }
      BObjectTypeID        getOTIDEmp() const { return (mOTIDEmp); }
      BObjectTypeID        getOTIDHotdropPadBeam() const { return (mHotdropPadBeamPOID); }

      // Cached proto tech IDs
      BProtoTechID         getPTIDCovenantShieldUpgrade() const { return(mPTIDShieldUpgrade); }
      BProtoTechID         getPTIDCovenantShieldDowngrade() const { return(mPTIDShieldDowngrade); }

      // Cached power ids
      BProtoPowerID        getPPIDRepair() const { return mPPIDRepair; }
      BProtoPowerID        getPPIDRallyPoint() const { return mPPIDRallyPoint; }
      BProtoPowerID        getPPIDHookRepair() const { return mPPIDHookRepair; }
      BProtoPowerID        getPPIDUnscOdstDrop() const { return (mPPIDUnscOdstDrop); }

      // DOT effects
      float                getDotSizeSmall() const { return mSmallDotSize; }
      float                getDotSizeMedium() const { return mMediumDotSize; }

      bool                 isValidObjectType(BObjectTypeID objectTypeID) const { return (objectTypeID >= 0 && objectTypeID < getNumberObjectTypes()); }
      bool                 isValidProtoSquad(BProtoSquadID protoSquadID) const { return (protoSquadID >= 0 && protoSquadID < getNumberProtoSquads()); }
      bool                 isValidAbstractType(BObjectTypeID objectTypeID) const { return (objectTypeID >= getNumberBaseObjectTypes() && objectTypeID < getNumberObjectTypes()); }
      bool                 isValidProtoObject(BObjectTypeID objectTypeID) const { return (objectTypeID >= 0 && objectTypeID < getNumberBaseObjectTypes()); }

      long                 getProtoObjectDataType(BSimString subTypeName) const;
      long                 getProtoObjectDataRelativity(BSimString relativityName) const;
      long                 getProtoObjectCommandType(const char* pName) const;
      const char*          getProtoObjectCommandName(long type) const;

      BProtoSquadID        getProtoShieldType(BProtoSquadID squadType);

      BDamageTypeID        getDamageType(const char* pName) const;
      const char*          getDamageTypeName(BDamageTypeID type) const;
      BDamageTypeID        getDamageTypeLight() const { return (mDamageTypeLight); }
      BDamageTypeID        getDamageTypeLightInCover() const { return (mDamageTypeLightInCover); }
      BDamageTypeID        getDamageTypeLightArmored() const { return (mDamageTypeLightArmored); }
      BDamageTypeID        getDamageTypeLightArmoredInCover() const { return (mDamageTypeLightArmoredInCover); }
      BDamageTypeID        getDamageTypeMedium() const { return (mDamageTypeMedium); }
      BDamageTypeID        getDamageTypeMediumAir() const { return (mDamageTypeMediumAir); }
      BDamageTypeID        getDamageTypeHeavy() const { return (mDamageTypeHeavy); }
      BDamageTypeID        getDamageTypeBuilding() const { return (mDamageTypeBuilding); }
      BDamageTypeID        getDamageTypeShielded() const { return (mDamageTypeShielded); }
      BDamageTypeID        getDamageTypeSmallAnimal() const { return (mDamageTypeSmallAnimal); }
      BDamageTypeID        getDamageTypeBigAnimal() const { return (mDamageTypeBigAnimal); }
      bool                 isBaseDamageType(BDamageTypeID type) const;
      int                  getNumberDamageTypes() const { return mDamageTypes.getNumber(); }
      BProtoObjectID       getDamageTypeExemplar(BDamageTypeID type);

      uint                 getNumberAttackRatingDamageTypes() const { return mAttackRatingDamageTypes.getSize(); }
      BDamageTypeID        getAttackRatingDamageType(uint index) const { return (index < mAttackRatingDamageTypes.getSize() ? mAttackRatingDamageTypes[index] : -1); }
      bool                 getAttackRatingIndex(BDamageTypeID damageType, uint& indexOut) const;
      float                getAttackRating(float dps);
      float                getDefenseRating(float hp);
      float                getAttackRatingMultiplier() const { return mAttackRatingMultiplier; }
      float                getDefenseRatingMultiplier() const { return mDefenseRatingMultiplier; }
      uint                 getGoodAgainstMinAttackGrade() const { return mGoodAgainstMinAttackGrade; }
      uint                 getGoodAgainstAttackGrade(long type);
      uint                 getAttackGrade(float baseDPS, float attackDPS);
      float                getAttackGradeRatio(float baseDPS, float attackDPS);

      long                 getAutoLockDownType(const char* pName) const;
      const char*          getAutoLockDownTypeName(long type) const;

      int                  getDamageDirection(const char* pName) const;
      const char*          getDamageDirectionName(int type) const;

      long                 getObjectClass(const char* pName) const;
      const char*          getObjectClassName(long type) const;

      long                 getMovementType(const char* pName) const;
      const char*          getMovementTypeName(long type) const;

      long                 getPickPriority(const char* pName) const;
      const char*          getPickPriorityName(long type) const;

      long                 getSelectType(const char* pName) const;
      const char*          getSelectTypeName(long type) const;

      long                 getGotoType(const char* pName) const;
      const char*          getGotoTypeName(long type) const;

      BRelationType        getRelationType(const char* pName) const;
      const char*          getRelationTypeName(BRelationType type) const;

      int                  getSquadMode(const char* pName) const;
      const char*          getSquadModeName(int type) const;

      long                 getImpactEffectSize(const char* pName) const;
      const char*          getImpactEffectSizeName(long type) const;

      long                 getNumberCivs() const { return (long)mCivs.size(); }
      BCiv*                getCiv(long id) const { if(id<0 || id>=(long)mCivs.size()) return NULL; else return mCivs[id]; }
      long                 getCivID(const char* pName) const;

      uint                 getNumberBurningEffectLimits() const;
      int                  getBurningEffectLimitByProtoObject(const BProtoObject* proto) const;
      int                  getDefaultBurningEffectLimit() const { return (mDefaultBurningEffectLimit); }

      long                 getNumberResources() const { return mResources.numTags(); }
      long                 getResource(const char* pName) const { long index; if(mResources.find(pName, &index)) return index; else return -1; }
      const char*          getResourceName(long resourceID) const;
      bool                 getResourceDeductable(long resourceID) const;

      int                  getNumberRates() { return mRates.numTags(); }
      int                  getRate(const char* pName) { long index; if(mRates.find(pName, &index)) return index; else return -1; }
      const char*          getRateName(int rateID);

      long                 getNumberPops() { return mPops.numTags(); }
      long                 getPop(const char* pName) { long index; if(mPops.find(pName, &index)) return index; else return -1; }
      const char*          getPopName(long type) const;

      long                 getNumberRefCountTypes() { return mRefCountTypes.numTags(); }
      BRefCountType        getRefCountType(const char* pName) { long index; if(mRefCountTypes.find(pName, &index)) return index; else return -1; }
      const char*          getRefCountTypeName(BRefCountType type) const;

      BPlayerState         getPlayerState(const char* pName) { long index; if (mPlayerStates.find(pName, &index)) return (index); else return (-1); }
      const char*          getPlayerStateName(BPlayerState type) const;

      int                  getHUDItem(const char* pName) const;
      const char*          getHUDItemName(int type) const;

      int                  getFlashableUIItem(const char* pName) const;

      long                 getUnitFlag(const char* pName) const;
      const char*          getUnitFlagName(long type) const;
      long                 getSquadFlag(const char* pName) const;
      const char*          getSquadFlagName(long type) const;

      long                 getNumberImpactEffects() { return mProtoImpactEffects.getSize(); }
      int                  getProtoImpactEffectIndex(const char* pName);
      const BProtoImpactEffect* getProtoImpactEffectFromIndex(int index);

      int                  getVisualDisplayPriority(const char* pName) const;

      long                 getNumberLocStrings() { return mLocStrings.getNumber(); }
      long                 getLocStringIndex(long id) const;
      const BUString&      getLocStringFromIndex(long index) const;
      const BUString&      getLocStringFromID(long id) const;
      BUString&            decodeLocString( BUString& locString ) const;

      BAbility*            getAbilityFromID(long id);
      long                 getAbilityIDFromName(const char* pName);
      const char*          getAbilityName(long id);
      int                  getSquadAbilityID(const BSquad* pSquad, int abilityID) const;
      long                 getAbilityType(const char* pName);
      long                 getAbilityTypeFromID(long id);
      int                  getNumberAbilities() const { return mAbilities.getNumber(); }

      int                  getRecoverType(const char* pName);

      BAIMissionType             getMissionType(const char* pName) const;
      const char*                getMissionTypeName(BAIMissionType missionType) const;
      BAIGroupTaskType           getAIGroupTaskType(const char* pName) const;
      const char*                getAIGroupTaskTypeName(BAIGroupTaskType aiGroupTaskType) const;
      BAIGroupState              getAIGroupState(const char* pName) const;
      const char*                getAIGroupStateName(BAIGroupState aiGroupState) const;
      BAIMissionState            getMissionState(const char* pName) const;
      const char*                getMissionStateName(BAIMissionState missionState) const;
      BAIMissionTargetType       getMissionTargetType(const char* pName) const;
      const char*                getMissionTargetTypeName(BAIMissionTargetType missionTargetType) const;
      BBidType                   getBidType(const char* pName) const;
      const char*                getBidTypeName(BBidType bidType) const;
      BBidState                  getBidState(const char* pName) const;
      const char*                getBidStateName(BBidState bidState) const;
      BMissionConditionType      getMissionConditionType(const char* pName) const;
      const char*                getMissionConditionTypeName(BMissionConditionType missionConditionType) const;
      BAISquadAnalysisComponent  getAISquadAnalysisComponent(const char* pName) const;
      const char*                getAISquadAnalysisComponentName(BAISquadAnalysisComponent aiSquadAnalysisComponent) const;
      BPowerType                 getPowerType(const char* pName) const;
      const char*                getPowerTypeName(BPowerType powerType) const;

      BAbilityID                 getAIDCommand() const { return (mAIDCommand); }
      BAbilityID                 getAIDUngarrison() const { return (mAIDUngarrison); }
      BAbilityID                 getAIDUnhitch() const { return (mAIDUnhitch); }
      BAbilityID                 getAIDUnscRam() const { return (mAIDUnscRam); }
      BAbilityID                 getAIDUnscMarineRockets() const { return (mAIDUnscMarineRockets); }
      BAbilityID                 getAIDUnscWolverineBarrage() const { return (mAIDUnscWolverineBarrage); }
      BAbilityID                 getAIDUnscBarrage() const { return (mAIDUnscBarrage); }
      BAbilityID                 getAIDUnscFlashBang() const { return (mAIDUnscFlashBang); }
      BAbilityID                 getAIDUnscHornetSpecial() const { return (mAIDUnscHornetSpecial); }
      BAbilityID                 getAIDUnscScorpionSpecial() const { return (mAIDUnscScorpionSpecial); }
      BAbilityID                 getAIDCovGruntGrenade() const { return (mAIDCovGruntGrenade); }
      BAbilityID                 getAIDUnscLockdown() const { return (mAIDUnscLockdown); }
      BAbilityID                 getAIDUnscCyclopsThrow() const { return (mAIDUnscCyclopsThrow); }
      BAbilityID                 getAIDUnscSpartanTakeOver() const { return (mAIDUnscSpartanTakeOver); }
      BAbilityID                 getAIDUnscGremlinSpecial() const { return (mAIDUnscGremlinSpecial); }
      BAbilityID                 getAIDCovCloak() const { return (mAIDCovCloak); }
      BAbilityID                 getAIDCovArbCloak() const { return (mAIDCovArbCloak); }
      BAbilityID                 getAIDCovLeaderGlassing() const { return (mAIDCovLeaderGlassing); }
      BAbilityID                 getAIDCovGhostRam() const { return (mAIDCovGhostRam); }
      BAbilityID                 getAIDCovChopperRunOver() const { return (mAIDCovChopperRunOver); }
      BAbilityID                 getAIDCovGruntSuicideExplode() const { return (mAIDCovGruntSuicideExplode); }
      BAbilityID                 getAIDCovStasis() const { return (mAIDCovStasis); }
      BAbilityID                 getAIDCovLocustOverburn() const { return (mAIDCovLocustOverburn); }
      BAbilityID                 getAIDCovJumppack() const { return (mAIDCovJumppack); }
      BAbilityID                 getAIDCovWraithSpecial() const { return (mAIDCovWraithSpecial); }
      BAbilityID                 getAIDUnscUnload() const { return (mAIDUnscUnload); }
      BAbilityID                 getAIDUnscCobraLockdown() const { return (mAIDUnscCobraLockdown); }

      BResourceID                getRIDSupplies() const { return (mRIDSupplies); }
      BResourceID                getRIDPower() const { return (mRIDPower); }
      BResourceID                getRIDLeaderPowerCharge() const { return (mRIDLeaderPowerCharge); }

      BRefCountType              getRCTRegen() const { return (mRCTRegen); }

      BGameSettings*       getBaseGameSettings() const { return mBaseGameSettings; }
      BGameSettings*       getGameSettings() const { return mCurrentGameSettings; }
      void                 resetGameSettings();
      //Added this as a way to set a particular player's settings to their default values - ewb
      void                 resetPlayer( long playerID );
      long                 getGameSettingsNetworkType() const;

      const BScenarioList& getScenarioList() const { return mScenarioList; }
      BTips&               getTips() { return mTips; }

      long                 getNumberProtoPowers() const { return mProtoPowers.getNumber(); }
      BProtoPower*         getProtoPowerByID(long id);
      long                 getProtoPowerIDByName(const char*  pProtoPowerName);

      int                  getNumberWeaponTypes() const { return mWeaponTypes.getNumber(); }
      BWeaponType*         getWeaponTypeByID(long id) const;
      long                 getWeaponTypeIDByName(const char*  pWeaponTypeName);
      float                getDamageModifier(long weaponTypeID, BEntityID targetID, BVector damageDirection, float &shieldedDamageModifier);

      int                  getNumberGameModes() const { return mGameModes.getNumber(); }
      BGameMode*       getGameModeByID(int id) const;
      long                 getGameModeIDByName(const char*  pGameModeName);

      long                 getNumberProtoTechs() const { return (long)mProtoTechs.size(); }
      BProtoTech*          getProtoTech(long id) const { if(id<0 || id>=(long)mProtoTechs.size()) return NULL; else return mProtoTechs[id]; }
      long                 getProtoTech(const char* pName) const { long index=-1; mTechTable.find(pName, &index); return index; }
      const char*          getProtoTechName(long id) const;
      void                 addTechUnitDependecy(long techID, long unitID);
      BTechUnitDependencyArray*  getTechUnitDependencies(long unitID);

      long                 getTechStatusFromName(const char* pName) const;

      long                 getNumberProtoSquads() const { return (long)mProtoSquads.size(); }
      BProtoSquad*         getGenericProtoSquad(long id) const;
      long                 getProtoSquad(const char* pName) const { long index; if(mProtoSquadTable.find(pName, &index)) return index; else return -1; }
      const char*          getProtoSquadName(long type) const;
      int                  getNumBaseProtoSquads() const { return mNumBaseProtoSquads; }

      long                 getNumberProtoHPBars() const { return (long) mProtoHPBars.size(); }
      BProtoHPBar*         getProtoHPBar(long id) const { if(id<0 || id>=(long)mProtoHPBars.size()) return NULL; else return mProtoHPBars[id]; }
      long                 getProtoHPBarID(const char* pName) const;
      long                 getProtoHPColorStageID(const char* pName) const;
      BProtoHPBarColorStages* getProtoHPColorStage(long id) const { if(id<0 || id>=(long)mProtoHPBarColorStages.size()) return NULL; else return mProtoHPBarColorStages[id]; }
                  
      long                 getProtoVeterancyBarID(const char* pName) const;
      long                 getNumberProtoVeterancyBars() const { return (long) mProtoVeterancyBars.size(); }
      BProtoVeterancyBar*  getProtoVeterancyBar(long id) const { if(id<0 || id>=(long)mProtoVeterancyBars.size()) return NULL; else return mProtoVeterancyBars[id]; }

      long                 getProtoPieProgressBarID(const char* pName) const;
      long                 getNumberProtoPieProgressBars() const { return (long) mProtoPieProgressBars.size(); }
      BProtoPieProgress*   getProtoPieProgressBar(long id) const { if(id<0 || id>=(long)mProtoPieProgressBars.size()) return NULL; else return mProtoPieProgressBars[id]; }

      long                 getProtoBobbleHeadID(const char* pName) const;
      long                 getNumberProtoBobbleHeads() const { return (long) mProtoBobbleHeads.size(); }
      BProtoBobbleHead*    getProtoBobbleHead(long id) const { if(id<0 || id>=(long)mProtoBobbleHeads.size()) return NULL; else return mProtoBobbleHeads[id]; }

      long                 getProtoBuildingStrengthID( const char* pName) const;
      long                 getNumberProtoBuildingStrength() const { return (long) mProtoBuildingStrength.size(); }
      BProtoBuildingStrength* getProtoBuildingStrength(long id) const { if(id<0 || id>=(long)mProtoBuildingStrength.size()) return NULL; else return mProtoBuildingStrength[id]; }
                  
      BActionType          getActionTypeByName(const char *pName, bool* pMelee = NULL) const; //Maps a name to an action type enum for tactics

      const BPlayerColor&  getPlayerColor(BPlayerColorCategory category, long index);
      const BPlayerColor&  getFOFSelfColor(BPlayerColorCategory category) const { BDEBUG_ASSERT(category < cMaxPlayerColorCategories); return mFOFSelfColor[category]; }
      const BPlayerColor&  getFOFAllyColor(BPlayerColorCategory category) const { BDEBUG_ASSERT(category < cMaxPlayerColorCategories); return mFOFAllyColor[category]; }
      const BPlayerColor&  getFOFNeutralColor(BPlayerColorCategory category) const { BDEBUG_ASSERT(category < cMaxPlayerColorCategories); return mFOFNeutralColor[category]; }
      const BPlayerColor&  getFOFEnemyColor(BPlayerColorCategory category) const { BDEBUG_ASSERT(category < cMaxPlayerColorCategories); return mFOFEnemyColor[category]; }

      int                  getCivPlayerColor(int civ, int player);

#ifndef BUILD_FINAL
      // This is incremented after the player colors are reloaded.
      uint                 getPlayerColorLoadCount() const { return mPlayerColorLoadCount; }
#endif

      long                 getPlacementRules(const char* pName);
      BPlacementRules*     getPlacementRules(long index);

      long                 getChatSpeakerFromName(const char* pName) const;
      long                 getExposedActionFromName(const char* pName) const;
      uint                 getDataScalarFromName(const char* pName) const;
      long                 getActionStatusFromName(const char* pName) const;
      int                  getFlareTypeFromName(const char* pName) const;
      long                 getAnimTypeFromName(const char* pName) const;


      // Leaders
      bool                 setupLeaders(void);
      void                 removeAllLeaders(void);
      long                 getNumberLeaders(void) const { return(mLeaders.getNumber()); }
      BLeader*             getLeader(long index);
      long                 getLeaderID(const char* pName) const;
      void                 loadLeaderIcons();
      void                 unloadLeaderIcons();

      //Infection map.
      bool                 getInfectedForm(BProtoObjectID basePOID, BProtoObjectID& infectedPOID, BProtoSquadID& infectedPSID) const;

      float                getGarrisonDamageMultiplier(void) const { return mGarrisonDamageMultiplier; }
      float                getConstructionDamageMultiplier() const { return mConstructionDamageMultiplier; }
      float                getCaptureDecayRate() const { return mCaptureDecayRate; }
      float                getSquadLeashLength(void) const { return mSquadLeashLength; }
      float                getSquadAggroLength(void) const { return mSquadAggroLength; }
      float                getUnitLeashLength(void) const { return mUnitLeashLength; }
      DWORD                getCloakingDelay(void) const { return mCloakingDelay; }
      DWORD                getReCloakingDelay(void) const { return mReCloakingDelay; }
      DWORD                getCloakDetectFrequency(void) const { return mCloakDetectFrequency; }
      DWORD                getShieldRegenDelay(void) const { return mShieldRegenDelay; }
      DWORD                getShieldRegenTime(void) const { return mShieldRegenTime; }
      float                getHeightBonusDamage(void) const { return mHeightBonusDamage;}
      float                getShieldRegenRate(void) const { return mShieldRegenRate; }
      DWORD                getShieldBarColor(void) const { return mShieldBarColor; }
      DWORD                getAmmoBarColor(void) const { return mAmmoBarColor; }
      long                 getMaxNumCorpses(void) const { return mMaxNumCorpses; }
      float                getInfantryCorpseDecayTime(void) const { return mInfantryCorpseDecayTime; }
      float                getCorpseSinkingSpacing(void) const { return mCorpseSinkingSpacing; }
      long                 getMaxCorpseDisposalCount(void) const { return mMaxCorpseDisposalCount; }
      float                getProjectileGravity(void) const { return mProjectileGravity; }
      float                getProjectileTumbleRate(void) const { return mProjectileTumbleRate; }
      float                getTrackInterceptDistance(void) const { return mTrackInterceptDistance; }
      float                getStationaryTargetAttackToleranceAngle(void) const { return mStationaryTargetAttackToleranceAngle; }
      float                getMovingTargetAttackToleranceAngle(void) const { return mMovingTargetAttackToleranceAngle; }
      float                getMovingTargetTrackingAttackToleranceAngle(void) const { return mMovingTargetTrackingAttackToleranceAngle; }
      float                getMovingTargetRangeMultiplier(void) const { return mMovingTargetRangeMultiplier; }
      float                getOpportunityBeingAttackedPriBonus(void) const { return mOpportunityBeingAttackedPriBonus; }
      float                getOpportunityDistPriFactor(void) const { return mOpportunityDistPriFactor; }
      float                getChanceToRocket(void) const { return mChanceToRocket; }
      float                getMaxDamageBankPctAdjust(void) const { return mMaxDamageBankPctAdjust; }
      float                getDamageBankTimer(void) const { return mDamageBankTimer; }
      float                getAirStrikeLoiterTime(void) const { return mAirStrikeLoiterTime; }
      uint32               getMaxSquadPathingCallsPerFrame(void) const { return mMaxSquadPathingCallsPerFrame; }
      uint32               getMaxPlatoonPathingCallsPerFrame(void) const { return mMaxPlatoonPathingCallsPerFrame; }
      float                getFatalityTransitionScale() const { return mFatalityTransitionScale; }
      float                getFatalityMaxTransitionTime() const { return mFatalityMaxTransitionTime; }
      float                getFatalityPositionOffsetTolerance() const { return mFatalityPositionOffsetTolerance; }
      float                getFatalityOrientationOffsetTolerance() const { return mFatalityOrientationOffsetTolerance; }
      float                getFatalityExclusionRange() const { return mFatalityExclusionRange; }
      float                getGameOverDelay() const { return mGameOverDelay; }
      int                  getSkirmishEmptyBaseObject() const { return mSkirmishEmptyBaseObject; }
      float                getRecyleRefundRate() const { return mRecyleRefundRate; }
      float                getBaseRebuildTimer() const { return mBaseRebuildTimer; }
      
      float                getOverrunMinVel() const { return mOverrunMinVel; }
      float                getOverrunJumpForce() const { return mOverrunJumpForce; }
      float                getOverrunDistance() const { return mOverrunDistance; }

      float                getCoopResourceSplitRate() const { return mCoopResourceSplitRate; }

      const BUString&      getDifficultyStringByType(int difficultyType);

      float                getDifficulty(int type);
      float                getDifficultyEasy() const { return (mDifficultyEasy); }
      float                getDifficultyNormal() const { return (mDifficultyNormal); }
      float                getDifficultyHard() const { return (mDifficultyHard); }
      float                getDifficultyLegendary() const { return (mDifficultyLegendary); }
      float                getDifficultyDefault() const { return (mDifficultyDefault); }
      float                getDifficultySPCAIDefault() const { return (mDifficultySPCAIDefault); }

      // Hero stuff
      float                getHeroDownedLOS() const { return (mHeroDownedLOS); }
      float                getHeroHPRegenTime() const { return (mHeroHPRegenTime); }
      float                getHeroRevivalDistance() const { return (mHeroRevivalDistance); }
      float                getHeroPercentHPRevivalThreshhold() const { return (mHeroPercentHPRevivalThreshhold); }
      float                getHeroMaxDeadTransportDist() const { return (mHeroMaxDeadTransportDist); }

      // Transport stuff
      float                getTransportClearRadiusScale() const { return (mTransportClearRadiusScale); }
      float                getTransportMaxSearchRadiusScale() const { return (mTransportMaxSearchRadiusScale); }
      uint                 getTransportMaxSearchLocations() const { return (mTransportMaxSearchLocations); }
      DWORD                getTransportBlockTime() const { return (mTransportBlockTime); }
      DWORD                getTransportLoadBlockTime() const { return (mTransportLoadBlockTime); }
      uint                 getTransportMaxBlockAttempts() const { return (mTransportMaxBlockAttempts); }
      float                getTransportIncomingHeight() const { return mTransportIncomingHeight; }
      float                getTransportIncomingOffset() const { return mTransportIncomingOffset; }
      float                getTransportOutgoingHeight() const { return mTransportOutgoingHeight; }
      float                getTransportOutgoingOffset() const { return mTransportOutgoingOffset; }
      float                getTransportPickupHeight() const { return mTransportPickupHeight; }
      float                getTransportDropoffHeight() const { return mTransportDropoffHeight; }
      uint                 getTransportMax() const { return (mTransportMax); }

      // Hitch Stuff
      float                getHitchOffset() const { return (mHitchOffset); }

      // Objective Arrow
      BProtoObjectID getPOIDObjectiveArrow() const { return (mPOIDObjectiveArrow); }
      BProtoObjectID getPOIDObjectiveLocArrow() const { return (mPOIDObjectiveLocArrow); }
      //BProtoObjectID getPOIDObjectiveGroundFX() const { return (mPOIDObjectiveGroundFX); }
      float getObjectiveArrowRadialOffset() const { return (mObjectiveArrowRadialOffset); }
      float getObjectiveArrowSwitchOffset() const { return (mObjectiveArrowSwitchOffset); }
      float getObjectiveArrowYOffset() const { return (mObjectiveArrowYOffset); }
      uint8 getObjectiveArrowMaxIndex() const { return (mObjectiveArrowMaxIndex); }

      bool                 setupLocStrings(bool reload);     

      // IProtoVisualHandler
      virtual bool         getVisualLogicValue(long logicType, const char* pName, DWORD& valDword, float& valFloat) const;
      virtual void         handleProtoVisualLoaded(BProtoVisual* pProtoVisual);

      uint                 decomposeObjectType(BObjectTypeID objectTypeID, BProtoObjectIDArray** protoObjectIDs);

      int                  getPlayerScope(const char* pName) const;
      int                  getLifeScope(const char* pName) const;
      bool                 checkPlayerScope(const BUnit* pUnit, BPlayerID playerID, int playerScope) const;
      bool                 checkLifeScope(const BUnit* pUnit, int lifeScope) const;

      const BRumbleEvent*  getRumbleEvent(int eventType) const;
      bool                 isRumbleEventEnabled(int eventType) const;
      int                  getRumbleEventID(int eventType);
      void                 setRumbleEventID(int eventType, int rumbleID);

      uint8                getWorldID(BSimString worldName) const;

#ifndef BUILD_FINAL
      BCueIndex            getSoundCueIndex(const char* pName); 
#endif

#if !defined(BUILD_FINAL) && defined(ENABLE_RELOAD_MANAGER)
      void                 setupDataReloading();
#endif

      // Impact sounds
      long                    getNumberImpactSoundSets() const                              { return mImpactSoundSets.getNumber(); }
      const BImpactSoundInfo* getImpactSound(int8 soundSetIndex, uint8 surfaceType) const;
      int8                    getImpactSoundSetIndex(const char* pName) const;

      // Ambient Life stuff
      DWORD                getALMaxWanderFrequency() const { return mALMaxWanderFrequency; }
      DWORD                getALPredatorCheckFrequency() const { return mALPredatorCheckFrequency; }
      DWORD                getALPreyCheckFrequency() const { return mALPreyCheckFrequency; }
      float                getALOppCheckRadius() const { return mALOppCheckRadius; }
      float                getALFleeDistance() const { return mALFleeDistance; }
      float                getALFleeMovementModifier() const { return mALFleeMovementModifier; }
      float                getALMinWanderDistance() const { return mALMinWanderDistance; }
      float                getALMaxWanderDistance() const { return mALMaxWanderDistance; }
      float                getALSpawnerCheckFrequency() const { return mALSpawnerCheckFrequency; }

      // Cryo stuff
      float                getTimeFrozenToThaw() const { return mTimeFrozenToThaw; }
      float                getTimeFreezingToThaw() const { return mTimeFreezingToThaw; }
      float                getDefaultCryoPoints() const { return mDefaultCryoPoints; }
      float                getDefaultThawSpeed() const { return mDefaultThawSpeed; }
      float                getFreezingSpeedModifier() const { return mFreezingSpeedModifier; }
      float                getFreezingDamageModifier() const { return mFreezingDamageModifier; }
      float                getFrozenDamageModifier() const { return mFrozenDamageModifier; }
      BProtoObjectID       getSmallSnowMoundId() const { return (mSmallSnowMoundPOID); }
      BProtoObjectID       getMediumSnowMoundId() const { return (mMediumSnowMoundPOID); }
      BProtoObjectID       getLargeSnowMoundId() const { return (mLargeSnowMoundPOID); }

      BProtoObjectStatic*  getSharedProtoObjectStaticData() { return mpSharedProtoObjectStaticData; }

      void                 resolveProtoPowerTextures();

      void                 discardDatabaseFiles();

      const BXMLReader*    getPreloadXmlFile(long dirID, const char* pFilename) const;

   protected:
      bool                 setupGameData();
      bool                 setupDependentData();
      bool                 setupPlacementRules();
      bool                 setupPlacementTracking();
      bool                 setupCivs();
      bool                 preloadVisFiles();
      bool                 preloadTfxFiles();
      bool                 preloadPhysicsFiles();
      bool                 setupProtoObjects();
      void                 setupDecomposedObjectTypes();
      bool                 setupProtoSquads();
      void                 setupProtoPowers();
      void                 setupAbilities();
      bool                 setupProtoTechs();
      bool                 setupPlayerColors();
      bool                 setupWeaponTypes();
      bool                 setupImpactEffects();
      bool                 setupImpactSounds();
      bool                 setupRumble();
      bool                 setupHPBars();
      bool                 setupDBIDs();
      bool                 setupGameModes();
      void                 setupDamageTypeExemplars();
      void                 setupTacticAttackRatings();
      void                 setupPreloadXmlFiles();


      BStringTable<byte>   mSurfaceTypes;
      byte                 mNumberSurfaceTypes;
      long                 mSurfaceTypeImpactEffect;

      int               mCorpseDeathEffect;

      BStringTableLong  mObjectTypes;
      long              mNumberBaseObjects;
      long              mNumberObjectTypes;
      long              mNumberAbstractObjectTypes;
      uint              mGoodAgainstGrades[cNumReticleAttackGrades];

      // Array of decomposed abstract types.
      BSmallDynamicSimArray<BProtoObjectIDArray> mDecomposedObjectTypes;

      BStringTableLong  mProtoSquadTable;
      BStringTableLong  mResources;
      BStringTableLong  mRates;
      BStringTableLong  mPops;
      BStringTableLong  mRefCountTypes;
      BStringTableLong  mPlayerStates;
      typedef BHashMap<BSimString, int, BHasher<BSimString>, BEqualTo<BSimString>, true, BSimFixedHeapAllocator> BImpactEffectHashMap;      
      BImpactEffectHashMap mImpactEffectHashMap;
      BSmallDynamicSimArray<BProtoImpactEffect> mProtoImpactEffects;

      typedef BHashMap<BSimString, int, BHasher<BSimString>, BEqualTo<BSimString>, true, BSimFixedHeapAllocator> BEffectPriorityHashMap;      
      BEffectPriorityHashMap mEffectPriorityHashMap;

      BStringTableLong  mLocStringTable;
      BDynamicSimArray<BUString>  mLocStrings;

      BScenarioList     mScenarioList; // scenario string IDs and types
      BTips             mTips;

      BSmallDynamicSimArray<BAbility*>                   mAbilities;
      BSmallDynamicSimArray<BDamageType>                 mDamageTypes;
      BSmallDynamicSimArray<BProtoObjectID>              mDamageTypeExemplars;
      BSmallDynamicSimArray<BDamageTypeIDSmall>          mAttackRatingDamageTypes;
      BSmallDynamicSimArray<BCiv*>                       mCivs;
      BSmallDynamicSimArray<BProtoObject*>               mProtoObjects;
      BSmallDynamicSimArray<BProtoPower*>                mProtoPowers;
      BSmallDynamicSimArray<BProtoSquad*>                mProtoSquads;
      BSmallDynamicSimArray<BProtoTech*>                 mProtoTechs;
      BSmallDynamicSimArray<BPlacementRules*>            mPlacementRules;
      BSmallDynamicSimArray<BProtoHPBar*>                mProtoHPBars;
      BSmallDynamicSimArray<BProtoHPBarColorStages*>     mProtoHPBarColorStages;
      BSmallDynamicSimArray<BLeader*>                    mLeaders;
      BSmallDynamicSimArray<BBurningEffectLimit>         mBurningEffectLimits;
      BSmallDynamicSimArray<BDatabaseObjectMap>          mInfectionMap;
      BSmallDynamicSimArray<bool>                        mResourceDeductable;
      BSmallDynamicSimArray<BGameMode*>                  mGameModes;
      BSmallDynamicSimArray<BImpactSoundSet>             mImpactSoundSets;
      BSmallDynamicSimArray<BProtoVeterancyBar*>         mProtoVeterancyBars;
      BSmallDynamicSimArray<BProtoPieProgress*>          mProtoPieProgressBars;
      BSmallDynamicSimArray<BProtoBobbleHead*>           mProtoBobbleHeads;
      BSmallDynamicSimArray<BProtoBuildingStrength*>     mProtoBuildingStrength;

      BStringTableLong           mTechTable;
      long                       mNumberTechUnitDependecies;
      BTechUnitDependencyArray** mpTechUnitDependecies;

      BDynamicArray<BWeaponType*,4,BDynamicArraySimHeapAllocator,BDynamicArrayNoConstructOptions> mWeaponTypes;

      BSmallDynamicSimArray<BRumbleEvent> mRumbleEvents;

      BGameSettings*       mBaseGameSettings;
      BGameSettings*       mCurrentGameSettings;

      BPlayerColor         mPlayerColors[cMaxPlayerColorCategories][cMaxPlayerColors];
      BPlayerColor         mFOFSelfColor[cMaxPlayerColorCategories];
      BPlayerColor         mFOFAllyColor[cMaxPlayerColorCategories];
      BPlayerColor         mFOFNeutralColor[cMaxPlayerColorCategories];
      BPlayerColor         mFOFEnemyColor[cMaxPlayerColorCategories];
      int                  mCivPlayerColors[cMaximumSupportedCivs][cMaximumSupportedPlayers];
      BUString             mStringNotFoundString;
      int                  mRumbleEventIDs[BRumbleEvent::cNumEvents];

      // Cached proto object ids
      BProtoObjectID       mPOIDRevealer;
      BProtoObjectID       mPOIDBlocker;
      BProtoObjectID       mPOIDUnitStart;
      BProtoObjectID       mPOIDRallyStart;
      BProtoObjectID       mPOIDAIEyeIcon;
      BProtoObjectID       mPOIDObstruction;
      BProtoObjectID       mPOIDPhysicsThrownObject;
      BProtoObjectID       mPOIDScn07Scarab;
      BProtoObjectID       mPSIDSkirmishScarab;
      BProtoObjectID       mPSIDCobra;
      BProtoObjectID       mPSIDVampire;
      BProtoObjectID       mPOIDLeaderPowerCaster;
      BProtoSquadID        mPSIDLeaderPowerCaster;

      // SPC Heroes
      BProtoObjectID       mPOIDForge;
      BProtoObjectID       mPOIDForgeWarthog;
      BProtoObjectID       mPOIDAnders;
      BProtoObjectID       mPOIDSpartanRocket;
      BProtoObjectID       mPOIDSpartanMG;
      BProtoObjectID       mPOIDSpartanSniper;

      // Objective arrow
      BProtoObjectID       mPOIDObjectiveArrow;
      BProtoObjectID       mPOIDObjectiveLocArrow;
      //BProtoObjectID       mPOIDObjectiveGroundFX;
      float                mObjectiveArrowRadialOffset;
      float                mObjectiveArrowSwitchOffset;
      float                mObjectiveArrowYOffset;
      uint8                mObjectiveArrowMaxIndex;

      // Cached object type ids
      BObjectTypeID        mOTIDBuilding;
      BObjectTypeID        mOTIDBuildingSocket;
      BObjectTypeID        mOTIDTurretSocket;
      BObjectTypeID        mOTIDTurretBuilding;
      BObjectTypeID        mOTIDSettlement;
      BObjectTypeID        mOTIDBase;
      BObjectTypeID        mOTIDIcon;
      BObjectTypeID        mOTIDGatherable;
      BObjectTypeID        mOTIDInfantry;
      BObjectTypeID        mOTIDTransporter;
      BObjectTypeID        mOTIDTransportable;
      BObjectTypeID        mOTIDFlood;
      BObjectTypeID        mOTIDCover;
      BObjectTypeID        mOTIDLOSObstructable;
      BObjectTypeID        mOTIDObjectiveArrow;
      BObjectTypeID        mOTIDCampaignHero;
      BObjectTypeID        mOTIDHero;
      BObjectTypeID        mOTIDHeroDeath;
      BObjectTypeID        mOTIDHotDropPickup;
      BObjectTypeID        mOTIDTeleportPickup;
      BObjectTypeID        mOTIDTeleportDropoff;
      BObjectTypeID        mOTIDGroundVehicle;
      BObjectTypeID        mOTIDGarrison;
      BObjectTypeID        mOTIDUnscSupplyPad;
      BObjectTypeID        mOTIDCovSupplyPad;
      BObjectTypeID        mOTIDStun;
      BObjectTypeID        mOTIDEmp;
      BObjectTypeID        mOTIDWallShield;
      BObjectTypeID        mOTIDBaseShield;
      BObjectTypeID        mOTIDLeader;
      BObjectTypeID        mOTIDCovenant;
      BObjectTypeID        mOTIDUnsc;
      BObjectTypeID        mOTIDBlackBox;
      BObjectTypeID        mOTIDSkull;
      BObjectTypeID        mOTIDBirthOnTop;
      BObjectTypeID        mOTIDCanCryo;
      BObjectTypeID        mOTIDHook;

      // Cached power ids
      BProtoPowerID        mPPIDRepair;
      BProtoPowerID        mPPIDRallyPoint;
      BProtoPowerID        mPPIDHookRepair;
      BProtoPowerID        mPPIDUnscOdstDrop;

      // Cached proto tech ids
      BProtoTechID         mPTIDShieldUpgrade;
      BProtoTechID         mPTIDShieldDowngrade;

      // Ability IDs
      BAbilityID           mAIDCommand;
      BAbilityID           mAIDUngarrison;
      BAbilityID           mAIDUnhitch;
      BAbilityID           mAIDUnscRam;
      BAbilityID           mAIDUnscMarineRockets;
      BAbilityID           mAIDUnscWolverineBarrage;
      BAbilityID           mAIDUnscBarrage;
      BAbilityID           mAIDUnscFlashBang;
      BAbilityID           mAIDUnscHornetSpecial;
      BAbilityID           mAIDUnscScorpionSpecial;
      BAbilityID           mAIDCovGruntGrenade;
      BAbilityID           mAIDUnscLockdown;
      BAbilityID           mAIDUnscCyclopsThrow;
      BAbilityID           mAIDUnscSpartanTakeOver;
      BAbilityID           mAIDUnscGremlinSpecial;
      BAbilityID           mAIDCovCloak;
      BAbilityID           mAIDCovArbCloak;
      BAbilityID           mAIDCovLeaderGlassing;
      BAbilityID           mAIDCovGhostRam;
      BAbilityID           mAIDCovChopperRunOver;
      BAbilityID           mAIDCovGruntSuicideExplode;
      BAbilityID           mAIDCovStasis;
      BAbilityID           mAIDCovLocustOverburn;
      BAbilityID           mAIDCovJumppack;
      BAbilityID           mAIDCovWraithSpecial;
      BAbilityID           mAIDUnscUnload;
      BAbilityID           mAIDUnscCobraLockdown;

      BResourceID          mRIDSupplies;
      BResourceID          mRIDPower;
      BResourceID          mRIDLeaderPowerCharge;

      BRefCountType        mRCTRegen;

      uint32               mMaxSquadPathingCallsPerFrame;
      uint32               mMaxPlatoonPathingCallsPerFrame;
      DWORD                mBuildingSelfDestructTime;
      float                mTributeAmount;
      float                mTributeCost;
      float                mUnscSupplyPadBonus;
      float                mUnscSupplyPadBreakEvenPoint;
      float                mCovSupplyPadBonus;
      float                mCovSupplyPadBreakEvenPoint;
      int                  mLeaderPowerChargeResourceID;
      int                  mLeaderPowerChargeRateID;
      float                mDamageReceivedXPFactor;
      float                mAttackRevealerLOS;
      DWORD                mAttackRevealerLifespan;
      float                mAttackedRevealerLOS;
      DWORD                mAttackedRevealerLifespan;
      float                mMinimumRevealerSize;
      float                mAttackRatingMultiplier;
      float                mDefenseRatingMultiplier;
      uint                 mGoodAgainstMinAttackGrade;
      float                mGarrisonDamageMultiplier;
      float                mConstructionDamageMultiplier;
      float                mCaptureDecayRate;
      float                mSquadLeashLength;
      float                mSquadAggroLength;
      float                mUnitLeashLength;
      DWORD                mCloakingDelay;
      DWORD                mReCloakingDelay;
      DWORD                mCloakDetectFrequency;
      DWORD                mShieldBarColor;
      DWORD                mAmmoBarColor;
      DWORD                mShieldRegenDelay;
      DWORD                mShieldRegenTime;
      float                mHeightBonusDamage;
      float                mShieldRegenRate;
      long                 mMaxNumCorpses;
      float                mInfantryCorpseDecayTime;
      float                mCorpseSinkingSpacing;
      long                 mMaxCorpseDisposalCount;
      float                mProjectileGravity;
      float                mProjectileTumbleRate;
      float                mTrackInterceptDistance;
      float                mStationaryTargetAttackToleranceAngle;
      float                mMovingTargetAttackToleranceAngle;
      float                mMovingTargetTrackingAttackToleranceAngle;
      float                mMovingTargetRangeMultiplier;
      float                mOpportunityDistPriFactor;
      float                mOpportunityBeingAttackedPriBonus;
      float                mChanceToRocket;
      float                mMaxDamageBankPctAdjust;
      float                mDamageBankTimer;
      float                mAirStrikeLoiterTime;
      int                  mDefaultBurningEffectLimit;
      BDamageTypeIDSmall   mDamageTypeLight;
      BDamageTypeIDSmall   mDamageTypeLightInCover;
      BDamageTypeIDSmall   mDamageTypeLightArmored;
      BDamageTypeIDSmall   mDamageTypeLightArmoredInCover;
      BDamageTypeIDSmall   mDamageTypeMedium;
      BDamageTypeIDSmall   mDamageTypeMediumAir;
      BDamageTypeIDSmall   mDamageTypeHeavy;
      BDamageTypeIDSmall   mDamageTypeBuilding;
      BDamageTypeIDSmall   mDamageTypeShielded;
      BDamageTypeIDSmall   mDamageTypeSmallAnimal;
      BDamageTypeIDSmall   mDamageTypeBigAnimal;
      float                mFatalityTransitionScale;
      float                mFatalityMaxTransitionTime;
      float                mFatalityPositionOffsetTolerance;
      float                mFatalityOrientationOffsetTolerance;
      float                mFatalityExclusionRange;
      float                mGameOverDelay;
      int                  mSkirmishEmptyBaseObject;
      float                mRecyleRefundRate;
      float                mBaseRebuildTimer;
      float                mOverrunMinVel;
      float                mOverrunJumpForce;
      float                mOverrunDistance;
      float                mCoopResourceSplitRate;

      float                mDifficultyEasy;
      float                mDifficultyNormal;
      float                mDifficultyHard;
      float                mDifficultyLegendary;
      float                mDifficultyDefault;
      float                mDifficultySPCAIDefault;

      // Hero stuff
      float                mHeroDownedLOS;
      float                mHeroHPRegenTime;
      float                mHeroRevivalDistance;
      float                mHeroPercentHPRevivalThreshhold;
      float                mHeroMaxDeadTransportDist;

      // Transport stuff
      float                mTransportClearRadiusScale;
      float                mTransportMaxSearchRadiusScale;
      uint                 mTransportMaxSearchLocations;
      DWORD                mTransportBlockTime;
      DWORD                mTransportLoadBlockTime;
      uint                 mTransportMaxBlockAttempts;
      float                mTransportIncomingHeight;
      float                mTransportIncomingOffset;
      float                mTransportOutgoingHeight;
      float                mTransportOutgoingOffset;
      float                mTransportPickupHeight;
      float                mTransportDropoffHeight;
      uint                 mTransportMax;

      // Hitch Stuff
      float                mHitchOffset;

      // Bubble shield types
      //typedef BSmallDynamicSimArray<std::pair<BProtoSquadID, BProtoSquadID>> ShieldPairCtnr;
      typedef BHashTable<BProtoSquadID, BProtoSquadID> ShieldPairCtnr;
      BProtoSquadID        mDefaultShieldID;
      ShieldPairCtnr       mProtoShieldIDs;

      // Ambient Life stuff
      DWORD                mALMaxWanderFrequency;
      DWORD                mALPredatorCheckFrequency;
      DWORD                mALPreyCheckFrequency;
      float                mALOppCheckRadius;
      float                mALFleeDistance;
      float                mALFleeMovementModifier;
      float                mALMinWanderDistance;
      float                mALMaxWanderDistance;
      float                mALSpawnerCheckFrequency;

      // Cryo stuff
      float                mTimeFrozenToThaw;
      float                mTimeFreezingToThaw;
      float                mDefaultCryoPoints;
      float                mDefaultThawSpeed;
      float                mFreezingSpeedModifier;
      float                mFreezingDamageModifier;
      float                mFrozenDamageModifier;
      BProtoObjectID       mSmallSnowMoundPOID;
      BProtoObjectID       mMediumSnowMoundPOID;
      BProtoObjectID       mLargeSnowMoundPOID;

      // Hot drop stuff
      BProtoObjectID       mHotdropGlowLargePOID;
      BProtoObjectID       mHotdropGlowSmallPOID;
      BProtoObjectID       mHotdropPadBeamPOID;

      // DOT stuff
      float                mSmallDotSize;
      float                mMediumDotSize;

      // Cached XML data
      BXMLReader*          mpGameDataReader;
      BXMLReader*          mpObjectsReader;
      BXMLReader*          mpSquadsReader;
      BXMLReader*          mpTechsReader;
      BXMLReader*          mpLeadersReader;
      BXMLReader*          mpCivsReader;
      BXMLReader*          mpPowersReader;

      BProtoObjectStatic*  mpSharedProtoObjectStaticData;

      int                  mNumBaseProtoSquads;

      BSmallDynamicSimArray<BXMLReader*> mPreloadXmlFiles;
      BStringTable<uint> mPreloadXmlFileTable;

      BSmallDynamicSimArray<BProtoVisual*> mLoadedProtoVisuals;
      bool                 mPostProcessProtoVisualLoads;

#if !defined(BUILD_FINAL) && defined(ENABLE_RELOAD_MANAGER)
      BFileWatcher*              mpFileWatcher;
      BFileWatcher::BPathHandle  mPathStringTable;
      BFileWatcher::BPathHandle  mPathPowers;
      BFileWatcher::BPathHandle  mPathAbilities;
      BFileWatcher::BPathHandle  mPathTrigerScripts;
      BFileWatcher::BPathHandle  mPathRumble;
      BFileWatcher::BPathHandle  mPlayerColorPathHandle;
      BFileWatcher::BPathHandle  mPowersHandle;
#endif

#ifndef BUILD_FINAL
      uint                       mPlayerColorLoadCount;
      BStringTable<BCueIndex>    mSoundCueTable;
#endif

};
