//==============================================================================
// aitypes.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// Temporary hack to short-circuit combat result predictions, until we know how combat will shake out.
//#define aiCombatHack

// xgame
#include "SimTarget.h"
#include "simtypes.h"
#include "gamefilemacros.h"


// Forward declarations
class BAI;
class BAIGroup;
class BKBBase;
class BKBPlayer;
class BKBSquad;

// Typedef types
typedef uint BAIMissionType;
typedef uint BAIMissionState;
typedef uint BAIMissionTargetType;
typedef uint BBidState;
typedef uint BBidType;
typedef uint BBidPriority;
typedef uint BAIGroupRole;
typedef uint BMissionConditionType;
typedef uint BAIGroupState;
typedef uint BAISquadAnalysisComponent;
typedef uint BAIGroupTaskType;
typedef uint BAITeleporterZoneType;


namespace AIDebugType
{
   enum
   {
      cNone = 0,
      cAIActiveMission,
      cAIHoverMission,
      cAIMissionTargets,
      cKBSquad,
      cKBBase,

      cMin = cNone,
      cMax = cKBBase,
   };
}

//==============================================================================
//==============================================================================
namespace AIAbility
{
   enum
   {
      cUnscMarineRockets,
      cUnscWolverineBarrage,
      cUnscBarrage,
      cUnscHornetSpecial,
      cUnscScorpionSpecial,
      cCovGruntGrenades,

      //cUnscRam,
      //cUnscFlashBang,

      cNumAIAbilities,
   };
}


//==============================================================================
//==============================================================================
namespace MissionState
{
   enum
   {
      cInvalid = 0,  // Invalid.  This is an error state.
      cSuccess,      // Mission Success.  This is an end state.
      cFailure,      // Mission Failure.  This is an end state.
      cCreate,       // Initialization state.
      cWorking,      // Doing whatever the mission is supposed to do (mission specific)
      cWithdraw,     // Temporarily stop working for some reason, but mission has not ended.
      cRetreat,      // Permanently stop working and run away before failing.
      
      // Update these if you add more states.
      cMin = cInvalid,
      cMax = cRetreat,
   };
}


//==============================================================================
//==============================================================================
namespace AIGroupRole
{
   enum
   {
      cInvalid = 0,  // Error
      cNormal,       // Attack what makes sense
      cVIP,          // Gets protected by other units
      cBodyguard,    // Protects VIP as priority over normal behavior
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cBodyguard,
   };
}


//==============================================================================
//==============================================================================
namespace AIGroupState
{
   enum
   {
      cInvalid = 0,        // Error!
      cMoving,             // The group is on its way to some destination (like the battle)
      cWaiting,            // Not doing anything at the moment (except auto-engage)
      cWithdraw,           // Temporarily avoiding danger
      cEngaged,            // Actively targeting / engaged
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cEngaged,
   };
}


//==============================================================================
//==============================================================================
namespace MissionConditionType
{
   enum
   {
      cInvalid = 0,
      cSquadCount,
      cAreaSecured,

      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cAreaSecured,
   };
}


//==============================================================================
//==============================================================================
namespace MissionType
{
   enum
   {
      cInvalid = 0,
      cAttack,
      cDefend,
      cScout,
      cClaim,
      cPower,
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cPower,
   };
}


//==============================================================================
//==============================================================================
namespace AITeleporterZoneType
{
   enum
   {
      cInvalid = 0,
      cAABB,
      cSphere,
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cSphere,
   };
}


//==============================================================================
//==============================================================================
namespace MissionTargetType
{
   enum
   {
      cInvalid = 0,
      cArea,
      cKBBase,
      cCaptureNode,
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cCaptureNode,
   };
}


//==============================================================================
//==============================================================================
namespace BidState
{
   enum
   {
      cInvalid    = 0,  // This bid is invalid.
      cInactive,        // Not currently buying anything, but staying in queue to preserve last purchase time
      cWaiting,         // Wants to buy something, but not yet approved
      cApproved,        // Approved, may buy at any time
      // Update these if you add more types.
      cMin = cInactive,
      cMax = cApproved,
   };
}


//==============================================================================
//==============================================================================
namespace BidType
{
   enum
   {
      cInvalid   = 0,  // This doesn't want to do anything yet.
      cNone,
      cSquad,          // This is for training a squad.
      cTech,           // This is for researching a tech.
      cBuilding,       // This is for constructing a building.
      cPower,          // This is for casting a power.
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cPower,
   };
}


//==============================================================================
//==============================================================================
namespace AITopicType
{
   enum
   {
      cInvalid = 0,
      cScriptModule,
      cMission,
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cMission,
   };
}


//==============================================================================
//==============================================================================
namespace AISquadAnalysisComponent
{
   enum
   {
      cInvalid = 0,
      cCVLight,
      cCVLightArmored,
      cCVMedium,
      cCVMediumAir,
      cCVHeavy,
      cCVBuilding,
      cCVTotal,
      cHPLight,
      cHPLightArmored,
      cHPMedium,
      cHPMediumAir,
      cHPHeavy,
      cHPBuilding,
      cHPTotal,
      cSPLight,
      cSPLightArmored,
      cSPMedium,
      cSPMediumAir,
      cSPHeavy,
      cSPBuilding,
      cSPTotal,
      cDPSLight,
      cDPSLightArmored,
      cDPSMedium,
      cDPSMediumAir,
      cDPSHeavy,
      cDPSBuilding,
      cDPSTotal,
      cCVPercentLight,
      cCVPercentLightArmored,
      cCVPercentMedium,
      cCVPercentMediumAir,
      cCVPercentHeavy,
      cCVPercentBuilding,
      cHPPercentLight,
      cHPPercentLightArmored,
      cHPPercentMedium,
      cHPPercentMediumAir,
      cHPPercentHeavy,
      cHPPercentBuilding,
      cCVStarsLight,
      cCVStarsLightArmored,
      cCVStarsMedium,
      cCVStarsMediumAir,
      cCVStarsHeavy,
      cCVStarsBuilding,
      cCVStarsTotal,
      cNormalizedStarsLight,
      cNormalizedStarsLightArmored,
      cNormalizedStarsMedium,
      cNormalizedStarsMediumAir,
      cNormalizedStarsHeavy,
      cNormalizedStarsBuilding,
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cNormalizedStarsBuilding
   };
}


//==============================================================================
//==============================================================================
namespace AIGroupTaskType
{
   enum
   {
      cInvalid,
      cWait,
      cMove,
      cAttack,
      cHijack,
      cCloak,
      cGather,
      cGarrison,
      cRepairOther,
      cDetonate,
      // Update the Min/Max if you add any.
      cMin = cInvalid,
      cMax = cRepairOther,
   };
}




//==============================================================================
//==============================================================================
class BAISquadAnalysis
{
public:

   BAISquadAnalysis() { reset(); }

   void reset();
   void add(const BAISquadAnalysis& a);
   float getComponent(BAISquadAnalysisComponent c) const;
   float getNormalizedStars(BDamageTypeID damageTypeID) const;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   // The makeup of the squad list:  CombatValue * PercentHP = CombatValueHitPoints = "CVHP"
   float mCVLight;
   float mCVLightArmored;
   float mCVMedium;
   float mCVMediumAir;
   float mCVHeavy;
   float mCVBuilding;
   float mCVTotal;

   // Raw HP
   float mHPLight;
   float mHPLightArmored;
   float mHPMedium;
   float mHPMediumAir;
   float mHPHeavy;
   float mHPBuilding;
   float mHPTotal;

   // Raw SP
   float mSPLight;
   float mSPLightArmored;
   float mSPMedium;
   float mSPMediumAir;
   float mSPHeavy;
   float mSPBuilding;
   float mSPTotal;

   // Raw DPS
   float mDPSLight;
   float mDPSLightArmored;
   float mDPSMedium;
   float mDPSMediumAir;
   float mDPSHeavy;
   float mDPSBuilding;
   float mDPSTotal;

   // The percentage of our makeup
   float mCVPercentLight;
   float mCVPercentLightArmored;
   float mCVPercentMedium;
   float mCVPercentMediumAir;
   float mCVPercentHeavy;
   float mCVPercentBuilding;

   float mHPPercentLight;
   float mHPPercentLightArmored;
   float mHPPercentMedium;
   float mHPPercentMediumAir;
   float mHPPercentHeavy;
   float mHPPercentBuilding;

   // Note:  Stars = AttackGradeRatio.
   // The amount of PAIN we bring: CombatValue * PercentHP * "Stars" = CVHPStars = CombatValueHitPoints * AttackGradeRatio.
   float mCVStarsLight;
   float mCVStarsLightArmored;
   float mCVStarsMedium;
   float mCVStarsMediumAir;
   float mCVStarsHeavy;
   float mCVStarsBuilding;
   float mCVStarsTotal;

   // The normalized PAIN by category.
   float mNormalizedStarsLight;
   float mNormalizedStarsLightArmored;
   float mNormalizedStarsMedium;
   float mNormalizedStarsMediumAir;
   float mNormalizedStarsHeavy;
   float mNormalizedStarsBuilding;

#ifdef aiCombatHack
   // Hack to make combat results non-zero until we figure out how combat will finally work.
   uint mNumSquads;
#endif
};


//==============================================================================
// BAIMissionTargetID
//==============================================================================
class BAIMissionTargetID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BAIMissionTargetID() : mID(static_cast<uint32>(BAIMissionTargetID::cInvalidID)) {}
   BAIMissionTargetID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAIMissionTargetID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BAIMissionTargetID::cRefCountMask) >> BAIMissionTargetID::cRefCountBitShift); }
   bool isValid() const { return (mID == BAIMissionTargetID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAIMissionTargetID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BAIMissionTargetID::cMaxIndex); mID = ((refCount << BAIMissionTargetID::cRefCountBitShift) & BAIMissionTargetID::cRefCountMask) | (index & BAIMissionTargetID::cIndexMask); }
   void bumpRefCount() { BASSERT(getIndex() < BAIMissionTargetID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BAIMissionTargetID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAIMissionTargetID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAIMissionTargetID(int32 val) { mID = static_cast<uint32>(val); }
   BAIMissionTargetID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};


//==============================================================================
// BAIMissionTargetWrapperID
//==============================================================================
class BAIMissionTargetWrapperID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BAIMissionTargetWrapperID() : mID(static_cast<uint32>(BAIMissionTargetWrapperID::cInvalidID)) {}
   BAIMissionTargetWrapperID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAIMissionTargetWrapperID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BAIMissionTargetWrapperID::cRefCountMask) >> BAIMissionTargetWrapperID::cRefCountBitShift); }
   bool isValid() const { return (mID == BAIMissionTargetWrapperID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAIMissionTargetWrapperID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BAIMissionTargetWrapperID::cMaxIndex); mID = ((refCount << BAIMissionTargetWrapperID::cRefCountBitShift) & BAIMissionTargetWrapperID::cRefCountMask) | (index & BAIMissionTargetWrapperID::cIndexMask); }
   void bumpRefCount() { BASSERT(getIndex() < BAIMissionTargetWrapperID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BAIMissionTargetWrapperID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAIMissionTargetWrapperID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAIMissionTargetWrapperID(int32 val) { mID = static_cast<uint32>(val); }
   BAIMissionTargetWrapperID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};



//==============================================================================
// BAIMissionID
//==============================================================================
class BAIMissionID
{
public:
   
   enum
   {
      cInvalidID        = UINT32_MAX,  // Invalid ID
      cMaxIndex         = 0x00000FFF,  // Max index allowed.
      cMaxType          = 0x0000000F,  // Max type allowed.
      cRefCountMask     = 0x7FFF0000,  // Mask for the refcount.
      cIndexMask        = 0x00000FFF,  // Mask for the index.
      cTypeMask         = 0x0000F000,  // Mask for the type.
      cTypeBitShift     = 12,          // # of bits to shift the type within the ID
      cRefCountBitShift = 16,          // # of bits to shift the refcount within the ID
   };

   BAIMissionID() : mID(static_cast<uint32>(BAIMissionID::cInvalidID)) {}
   BAIMissionID(uint refCount, BAIMissionType type, uint index) { set(refCount, type, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAIMissionID::cIndexMask); }
   BAIMissionType getType() const { return static_cast<uint>((mID & BAIMissionID::cTypeMask) >> BAIMissionID::cTypeBitShift); }
   uint getRefCount() const { return static_cast<uint>((mID & BAIMissionID::cRefCountMask) >> BAIMissionID::cRefCountBitShift); }
   bool isValid() const { return (mID == BAIMissionID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAIMissionID::cInvalidID); }
   void set(uint refCount, BAIMissionType type, uint index) { BASSERT(index < BAIMissionID::cMaxIndex); BASSERT(type < BAIMissionID::cMaxType); mID = ((refCount << BAIMissionID::cRefCountBitShift) & BAIMissionID::cRefCountMask) | ((type << BAIMissionID::cTypeBitShift) & BAIMissionID::cTypeMask) | (index & BAIMissionID::cIndexMask); }
   void bumpRefCount() { BASSERT(getIndex() < BAIMissionID::cMaxIndex); set(getRefCount()+1, getType(), getIndex()); }
   bool operator == (const BAIMissionID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAIMissionID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAIMissionID(int32 val) { mID = static_cast<uint32>(val); }
   BAIMissionID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};

//==============================================================================
// BAIGroupID
//==============================================================================
class BAIGroupID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BAIGroupID() : mID(static_cast<uint32>(BAIGroupID::cInvalidID)) {}
   BAIGroupID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAIGroupID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BAIGroupID::cRefCountMask) >> BAIGroupID::cRefCountBitShift); }
   bool isValid() const { return (mID == BAIGroupID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAIGroupID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BAIGroupID::cMaxIndex); mID = ((refCount << BAIGroupID::cRefCountBitShift) & BAIGroupID::cRefCountMask) | (index & BAIGroupID::cIndexMask); }
   void bumpRefCount() { BASSERT(getIndex() < BAIGroupID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BAIGroupID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAIGroupID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAIGroupID(int32 val) { mID = static_cast<uint32>(val); }
   BAIGroupID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};


//==============================================================================
// BAITeleporterZoneID
//==============================================================================
class BAITeleporterZoneID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX,  // Invalid ID
      cMaxIndex         = 0x00000FFF,  // Max index allowed.
      cMaxType          = 0x0000000F,  // Max type allowed.
      cRefCountMask     = 0x7FFF0000,  // Mask for the refcount.
      cIndexMask        = 0x00000FFF,  // Mask for the index.
      cTypeMask         = 0x0000F000,  // Mask for the type.
      cTypeBitShift     = 12,          // # of bits to shift the type within the ID
      cRefCountBitShift = 16,          // # of bits to shift the refcount within the ID
   };

   BAITeleporterZoneID() : mID(static_cast<uint32>(BAITeleporterZoneID::cInvalidID)) {}
   BAITeleporterZoneID(uint refCount, BAITeleporterZoneType type, uint index) { set(refCount, type, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAITeleporterZoneID::cIndexMask); }
   BAITeleporterZoneType getType() const { return static_cast<uint>((mID & BAITeleporterZoneID::cTypeMask) >> BAITeleporterZoneID::cTypeBitShift); }
   uint getRefCount() const { return static_cast<uint>((mID & BAITeleporterZoneID::cRefCountMask) >> BAITeleporterZoneID::cRefCountBitShift); }
   bool isValid() const { return (mID == BAITeleporterZoneID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAITeleporterZoneID::cInvalidID); }
   void set(uint refCount, BAITeleporterZoneType type, uint index) { BASSERT(index < BAITeleporterZoneID::cMaxIndex); BASSERT(type < BAITeleporterZoneID::cMaxType); mID = ((refCount << BAITeleporterZoneID::cRefCountBitShift) & BAITeleporterZoneID::cRefCountMask) | ((type << BAITeleporterZoneID::cTypeBitShift) & BAITeleporterZoneID::cTypeMask) | (index & BAITeleporterZoneID::cIndexMask); }
   void bumpRefCount() { BASSERT(getIndex() < BAITeleporterZoneID::cMaxIndex); set(getRefCount()+1, getType(), getIndex()); }
   bool operator == (const BAITeleporterZoneID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAITeleporterZoneID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAITeleporterZoneID(int32 val) { mID = static_cast<uint32>(val); }
   BAITeleporterZoneID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};


//==============================================================================
// BAIGroupTaskID
//==============================================================================
class BAIGroupTaskID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BAIGroupTaskID() : mID(static_cast<uint32>(BAIGroupTaskID::cInvalidID)) {}
   BAIGroupTaskID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAIGroupTaskID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BAIGroupTaskID::cRefCountMask) >> BAIGroupTaskID::cRefCountBitShift); }
   bool isValid() const { return (mID == BAIGroupTaskID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAIGroupTaskID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BAIGroupTaskID::cMaxIndex); mID = ((refCount << BAIGroupTaskID::cRefCountBitShift) & BAIGroupTaskID::cRefCountMask) | (index & BAIGroupTaskID::cIndexMask); }
   void bumpRefCount() { BASSERT(getIndex() < BAIGroupTaskID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BAIGroupTaskID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAIGroupTaskID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAIGroupTaskID(int32 val) { mID = static_cast<uint32>(val); }
   BAIGroupTaskID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};


//==============================================================================
// BAITopicID
//==============================================================================
class BAITopicID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BAITopicID() : mID(static_cast<uint32>(BAITopicID::cInvalidID)) {}
   BAITopicID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAITopicID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BAITopicID::cRefCountMask) >> BAITopicID::cRefCountBitShift); }
   bool isValid() const { return (mID == BAITopicID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAITopicID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BAITopicID::cMaxIndex); mID = ((refCount << BAITopicID::cRefCountBitShift) & BAITopicID::cRefCountMask) | (index & BAITopicID::cIndexMask);}
   void bumpRefCount() { BASSERT(getIndex() < BAITopicID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BAITopicID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAITopicID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAITopicID(int32 val) { mID = static_cast<uint32>(val); }
   BAITopicID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};


//==============================================================================
// class BKBSquadID
// Encapsulates the index and refcount of the BKBSquad entry in a BKB.
// Note:  Is not unique across multiple BKB's.  Two BKB's can have the same ID.
//==============================================================================
class BKBSquadID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BKBSquadID() : mID(static_cast<uint32>(BKBSquadID::cInvalidID)) {}
   BKBSquadID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BKBSquadID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BKBSquadID::cRefCountMask) >> BKBSquadID::cRefCountBitShift); }
   bool isValid() const { return (mID == BKBSquadID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BKBSquadID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BKBSquadID::cMaxIndex); mID = ((refCount << BKBSquadID::cRefCountBitShift) & BKBSquadID::cRefCountMask) | (index & BKBSquadID::cIndexMask);}
   void bumpRefCount() { BASSERT(getIndex() < BKBSquadID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BKBSquadID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BKBSquadID rhs) const { return (mID != rhs.mID); }
   operator int32() const { return (static_cast<int32>(mID)); }
   BKBSquadID(int32 val) { mID = static_cast<uint32>(val); }

protected:
   uint32 mID;
};


//==============================================================================
// class BKBBaseID
// Encapsulates the index and refcount of the BKBBase entry in a BKB.
// Note:  Is not unique across multiple BKB's.  Two BKB's can have the same ID.
//==============================================================================
class BKBBaseID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BKBBaseID() : mID(static_cast<uint32>(BKBBaseID::cInvalidID)) {}
   BKBBaseID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BKBBaseID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BKBBaseID::cRefCountMask) >> BKBBaseID::cRefCountBitShift); }
   bool isValid() const { return (mID == BKBBaseID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BKBBaseID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BKBBaseID::cMaxIndex); mID = ((refCount << BKBBaseID::cRefCountBitShift) & BKBBaseID::cRefCountMask) | (index & BKBBaseID::cIndexMask);}
   void bumpRefCount() { BASSERT(getIndex() < BKBBaseID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BKBBaseID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BKBBaseID rhs) const { return (mID != rhs.mID); }
   operator int32() const { return (static_cast<int32>(mID)); }
   BKBBaseID(int32 val) { mID = static_cast<uint32>(val); }

protected:
   uint32 mID;
};

//==============================================================================
// BBidID
//==============================================================================
class BBidID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cRefCountMask     = 0x7FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
   };

   BBidID() : mID(static_cast<uint32>(BBidID::cInvalidID)) {}
   BBidID(uint refCount, uint index) { set(refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BBidID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BBidID::cRefCountMask) >> BBidID::cRefCountBitShift); }
   bool isValid() const { return (mID == BBidID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BBidID::cInvalidID); }
   void set(uint refCount, uint index) { BASSERT(index < BBidID::cMaxIndex); mID = ((refCount << BBidID::cRefCountBitShift) & BBidID::cRefCountMask) | (index & BBidID::cIndexMask);}
   void bumpRefCount() { BASSERT(getIndex() < BBidID::cMaxIndex); set(getRefCount()+1, getIndex()); }
   bool operator == (const BBidID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BBidID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BBidID(int32 val) { mID = static_cast<uint32>(val); }
   BBidID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};


// Some typedefs.
typedef BSmallDynamicSimArray<BKBSquadID> BKBSquadIDArray;
typedef BSmallDynamicSimArray<BKBBaseID> BKBBaseIDArray;
typedef BSmallDynamicSimArray<BKBSquad*> BKBSquadArray;
typedef BSmallDynamicSimArray<BKBSquadArray> BKBSquadArrayArray;
typedef BSmallDynamicSimArray<BKBPlayer*> BKBPlayerArray;
typedef BSmallDynamicSimArray<BKBBase*> BKBBaseArray;
typedef BSmallDynamicSimArray<BAIMissionTargetID> BAIMissionTargetIDArray;
typedef BSmallDynamicSimArray<BAIMissionTargetWrapperID> BAIMissionTargetWrapperIDArray;
typedef BSmallDynamicSimArray<BAIMissionID> BAIMissionIDArray;
typedef BSmallDynamicSimArray<BAITopicID> BAITopicIDArray;
typedef BSmallDynamicSimArray<BBidID> BBidIDArray;
typedef BSmallDynamicSimArray<BAIGroupID> BAIGroupIDArray;
typedef BSmallDynamicSimArray<BAITeleporterZoneID> BAITeleporterZoneIDArray;
typedef BSmallDynamicSimArray<BAIGroupTaskID> BAIGroupTaskIDArray;

// Globals
__declspec(selectany) extern const BAIMissionTargetID cInvalidAIMissionTargetID = BAIMissionTargetID();
__declspec(selectany) extern const BAIMissionTargetWrapperID cInvalidAIMissionTargetWrapperID = BAIMissionTargetWrapperID();
__declspec(selectany) extern const BAIMissionID cInvalidAIMissionID = BAIMissionID();
__declspec(selectany) extern const BAITopicID cInvalidAITopicID = BAITopicID();
//__declspec(selectany) extern const BAIDifficultySettingID cInvalidAIDifficultySettingID = BAIDifficultySettingID();
__declspec(selectany) extern const BKBSquadID cInvalidKBSquadID = BKBSquadID();
__declspec(selectany) extern const BKBBaseID cInvalidKBBaseID = BKBBaseID();
__declspec(selectany) extern const BBidID cInvalidBidID = BBidID();
__declspec(selectany) extern const BAIGroupID cInvalidAIGroupID = BAIGroupID();
__declspec(selectany) extern const BAITeleporterZoneID cInvalidAITeleporterZoneID = BAITeleporterZoneID();
__declspec(selectany) extern const BAIGroupTaskID cInvalidAIGroupTaskID = BAIGroupTaskID();