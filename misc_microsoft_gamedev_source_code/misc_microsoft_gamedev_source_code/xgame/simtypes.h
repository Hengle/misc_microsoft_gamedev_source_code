//==============================================================================
// simtypes.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

#include "maximumsupportedplayers.h"
#include "gamefilemacros.h"

// Stop using long everywhere.
// Use the appropriate types instead.
// Note: These are still longs for now.  :)
typedef long BPlayerID;
typedef long BTeamID;
typedef long BProtoTechID;
typedef long BProtoObjectID;
typedef long BObjectTypeID;
typedef long BProtoSquadID;
typedef long BProtoPowerID;
typedef int BLeaderID;
typedef uint BTriggerScriptType;
typedef uint BTriggerScriptID;
typedef uint BTriggerGroupID;
typedef uint BTriggerVarID;
typedef uint BTriggerVarSigID;
typedef uint BTriggerVarType;
typedef long BCompareType;
typedef long BOperatorType;
typedef int BDamageTypeID;
typedef long BDesignLineID;
typedef long BPlayerState;
typedef long BAbilityID;
typedef int BAbilityType;
typedef int BAbilityTargetType;
typedef uint BPowerLevel;
typedef long BResourceID;
typedef long BRefCountType;

// Invalid values.
__declspec(selectany) extern const BPlayerID cInvalidPlayerID = -1;
__declspec(selectany) extern const BTeamID cInvalidTeamID = -1;
__declspec(selectany) extern const BProtoTechID cInvalidTechID = -1;
__declspec(selectany) extern const BProtoObjectID cInvalidProtoObjectID = -1;
__declspec(selectany) extern const BObjectTypeID cInvalidObjectTypeID = -1;
__declspec(selectany) extern const BProtoSquadID cInvalidProtoSquadID = -1;
__declspec(selectany) extern const BProtoPowerID cInvalidProtoPowerID = -1;
__declspec(selectany) extern const BLeaderID cInvalidLeaderID = -1;
__declspec(selectany) extern const BDamageTypeID cInvalidDamageTypeID = -1;
__declspec(selectany) extern const BAbilityID cInvalidAbilityID = -1;
__declspec(selectany) extern const BAbilityType cInvalidAbilityType = -1;
__declspec(selectany) extern const BAbilityTargetType cInvalidAbilityTargetType = -1;
__declspec(selectany) extern const BResourceID cInvalidResourceID = -1;



//==============================================================================
// class BInt32
// Template class which handles the conversion between the storage value (int32)
// and the ConversionType.  Does compile time asserts to make sure the MinValue
// and MaxValue are within range of the storage type.  Does run time asserts to
// make sure when the storage value is set, it's not out of range.
//==============================================================================
template<typename ConversionType, int MinValue = INT32_MIN, int MaxValue = INT32_MAX>
class BInt32
{
public:
   BInt32() { BCOMPILETIMEASSERT(MinValue >= INT32_MIN); BCOMPILETIMEASSERT(MaxValue <= INT32_MAX); }
   BInt32(ConversionType val) { BCOMPILETIMEASSERT(MinValue >= INT32_MIN); BCOMPILETIMEASSERT(MaxValue <= INT32_MAX); BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<int32>(val); }
   operator ConversionType() const { return static_cast<ConversionType>(mValue); }
   ConversionType asConversionType() const { return static_cast<ConversionType>(mValue); }
   BInt32& operator= (ConversionType val) { BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<int32>(val); return (*this); }
   BInt32& operator+= (ConversionType val) { ConversionType newVal = mValue + val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<int32>(newVal); return (*this); }
   BInt32& operator-= (ConversionType val) { ConversionType newVal = mValue - val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<int32>(newVal); return (*this); }
private:
   int32 mValue;
};


//==============================================================================
// class BUInt32
// Template class which handles the conversion between the storage value (uint32)
// and the ConversionType.  Does compile time asserts to make sure the MinValue
// and MaxValue are within range of the storage type.  Does run time asserts to
// make sure when the storage value is set, it's not out of range.
//==============================================================================
template<typename ConversionType, uint MinValue = UINT32_MIN, uint MaxValue = UINT32_MAX>
class BUInt32
{
public:
   BUInt32() { BCOMPILETIMEASSERT(MinValue >= UINT32_MIN); BCOMPILETIMEASSERT(MaxValue <= UINT32_MAX); }
   BUInt32(ConversionType val) { BCOMPILETIMEASSERT(MinValue >= UINT32_MIN); BCOMPILETIMEASSERT(MaxValue <= UINT32_MAX); BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<uint32>(val); }
   operator ConversionType() const { return static_cast<ConversionType>(mValue); }
   ConversionType asConversionType() const { return static_cast<ConversionType>(mValue); }
   BUInt32& operator= (ConversionType val) { BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<uint32>(val); return (*this); }
   BUInt32& operator+= (ConversionType val) { ConversionType newVal = mValue + val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<uint32>(newVal); return (*this); }
   BUInt32& operator-= (ConversionType val) { ConversionType newVal = mValue - val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<uint32>(newVal); return (*this); }
private:
   uint32 mValue;
};


//==============================================================================
// class BInt16
// Template class which handles the conversion between the storage value (int16)
// and the ConversionType.  Does compile time asserts to make sure the MinValue
// and MaxValue are within range of the storage type.  Does run time asserts to
// make sure when the storage value is set, it's not out of range.
//==============================================================================
template<typename ConversionType, int MinValue = INT16_MIN, int MaxValue = INT16_MAX>
class BInt16
{
public:
   BInt16() { BCOMPILETIMEASSERT(MinValue >= INT16_MIN); BCOMPILETIMEASSERT(MaxValue <= INT16_MAX); }
   BInt16(ConversionType val) { BCOMPILETIMEASSERT(MinValue >= INT16_MIN); BCOMPILETIMEASSERT(MaxValue <= INT16_MAX); BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<int16>(val); }
   operator ConversionType() const { return static_cast<ConversionType>(mValue); }
   ConversionType asConversionType() const { return static_cast<ConversionType>(mValue); }
   BInt16& operator= (ConversionType val) { BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<int16>(val); return (*this); }
   BInt16& operator+= (ConversionType val) { ConversionType newVal = mValue + val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<int16>(newVal); return (*this); }
   BInt16& operator-= (ConversionType val) { ConversionType newVal = mValue - val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<int16>(newVal); return (*this); }
private:
   int16 mValue;
};


//==============================================================================
// class BUInt16
// Template class which handles the conversion between the storage value (uint16)
// and the ConversionType.  Does compile time asserts to make sure the MinValue
// and MaxValue are within range of the storage type.  Does run time asserts to
// make sure when the storage value is set, it's not out of range.
//==============================================================================
template<typename ConversionType, uint MinValue = UINT16_MIN, uint MaxValue = UINT16_MAX>
class BUInt16
{
public:
   BUInt16() { BCOMPILETIMEASSERT(MinValue >= UINT16_MIN); BCOMPILETIMEASSERT(MaxValue <= UINT16_MAX); }
   BUInt16(ConversionType val) { BCOMPILETIMEASSERT(MinValue >= UINT16_MIN); BCOMPILETIMEASSERT(MaxValue <= UINT16_MAX); BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<uint16>(val); }
   operator ConversionType() const { return static_cast<ConversionType>(mValue); }
   ConversionType asConversionType() const { return static_cast<ConversionType>(mValue); }
   BUInt16& operator= (ConversionType val) { BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<uint16>(val); return (*this); }
   BUInt16& operator+= (ConversionType val) { ConversionType newVal = mValue + val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<uint16>(newVal); return (*this); }
   BUInt16& operator-= (ConversionType val) { ConversionType newVal = mValue - val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<uint16>(newVal); return (*this); }
private:
   uint16 mValue;
};


//==============================================================================
// class BInt8
// Template class which handles the conversion between the storage value (int8)
// and the ConversionType.  Does compile time asserts to make sure the MinValue
// and MaxValue are within range of the storage type.  Does run time asserts to
// make sure when the storage value is set, it's not out of range.
//==============================================================================
template<typename ConversionType, int MinValue = INT8_MIN, int MaxValue = INT8_MAX>
class BInt8
{
public:
   BInt8() { BCOMPILETIMEASSERT(MinValue >= INT8_MIN); BCOMPILETIMEASSERT(MaxValue <= INT8_MAX); }
   BInt8(ConversionType val) { BCOMPILETIMEASSERT(MinValue >= INT8_MIN); BCOMPILETIMEASSERT(MaxValue <= INT8_MAX); BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<int8>(val); }
   operator ConversionType() const { return static_cast<ConversionType>(mValue); }
   ConversionType asConversionType() const { return static_cast<ConversionType>(mValue); }
   BInt8& operator= (ConversionType val) { BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<int8>(val); return (*this); }
   BInt8& operator+= (ConversionType val) { ConversionType newVal = mValue + val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<int8>(newVal); return (*this); }
   BInt8& operator-= (ConversionType val) { ConversionType newVal = mValue - val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<int8>(newVal); return (*this); }
private:
   int8 mValue;
};


//==============================================================================
// class BUInt8
// Template class which handles the conversion between the storage value (uint8)
// and the ConversionType.  Does compile time asserts to make sure the MinValue
// and MaxValue are within range of the storage type.  Does run time asserts to
// make sure when the storage value is set, it's not out of range.
//==============================================================================
template<typename ConversionType, uint MinValue = UINT8_MIN, uint MaxValue = UINT8_MAX>
class BUInt8
{
public:
   BUInt8() { BCOMPILETIMEASSERT(MinValue >= UINT8_MIN); BCOMPILETIMEASSERT(MaxValue <= UINT8_MAX); }
   BUInt8(ConversionType val) { BCOMPILETIMEASSERT(MinValue >= UINT8_MIN); BCOMPILETIMEASSERT(MaxValue <= UINT8_MAX); BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<uint8>(val); }
   operator ConversionType() const { return static_cast<ConversionType>(mValue); }
   ConversionType asConversionType() const { return static_cast<ConversionType>(mValue); }
   BUInt8& operator= (ConversionType val) { BASSERT(val >= MinValue && val <= MaxValue); mValue = static_cast<uint8>(val); return (*this); }
   BUInt8& operator+= (ConversionType val) { ConversionType newVal = mValue + val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<uint8>(newVal); return (*this); }
   BUInt8& operator-= (ConversionType val) { ConversionType newVal = mValue - val; BASSERT(newVal >= MinValue && newVal <= MaxValue); mValue = static_cast<uint8>(newVal); return (*this); }
private:
   uint8 mValue;
};


// Examples of using these storage classes
typedef BInt8<BPlayerID, cInvalidPlayerID, (cMaximumSupportedPlayers-1)> BPlayerIDSmall; 
typedef BInt8<BTeamID, cInvalidTeamID, (cMaximumSupportedTeams-1)> BTeamIDSmall;
typedef BInt8<BProtoTechID, cInvalidTechID, INT8_MAX> BProtoTechIDSmall;
typedef BInt16<BProtoObjectID, cInvalidProtoObjectID, INT16_MAX> BProtoObjectIDSmall;
typedef BInt16<BObjectTypeID, cInvalidObjectTypeID, INT16_MAX> BObjectTypeIDSmall;
typedef BInt8<BProtoSquadID, cInvalidProtoSquadID, INT8_MAX> BProtoSquadIDSmall;
typedef BInt8<BProtoPowerID, cInvalidProtoPowerID, INT8_MAX> BProtoPowerIDSmall;

// Hacky overbound one.
__declspec(selectany) extern const long cInvalidProtoID = -1;

// I have converted these types over... :)
typedef uint8 BRelationType;

typedef uint BActionID;
typedef BYTE BActionState;
typedef BYTE BActionType;
typedef uint8 BPathLevel;
typedef BYTE BFormationType;
typedef BYTE BSimOrderType;
typedef uint BUnitOppID;
typedef uint8 BUnitOppType;
typedef long BDifficultyType;
typedef uint8 BJumpOrderType;

                         
// Some helpful dynamic arrays of the above common sim types.
// Note: small dynamic arrays limited to 65535 entries.
typedef BSmallDynamicSimArray<BPlayerID> BPlayerIDArray;
typedef BSmallDynamicSimArray<BTeamID> BTeamIDArray;
typedef BSmallDynamicSimArray<BProtoTechID> BTechIDArray;
typedef BSmallDynamicSimArray<BProtoObjectID> BProtoObjectIDArray;
typedef BSmallDynamicSimArray<BProtoSquadID> BProtoSquadIDArray;
typedef BSmallDynamicSimArray<BObjectTypeID> BObjectTypeIDArray;
typedef BSmallDynamicSimArray<BRelationType> BRelationTypeArray;
typedef BSmallDynamicSimArray<BActionID> BActionIDArray;
typedef BSmallDynamicSimArray<int32> BInt32Array;
typedef BSmallDynamicSimArray<DWORD> BDWORDArray;
typedef BSmallDynamicSimArray<BDesignLineID> BDesignLineIDArray;
typedef BSmallDynamicSimArray<float> BFloatArray;

struct BPlayerSpecificEntityIDs
{
   BPlayerID mPlayerID;
   BEntityIDArray mEntityIDs;

   bool operator == (const BPlayerSpecificEntityIDs rhs) const { return ((mPlayerID == rhs.mPlayerID) && (mEntityIDs == rhs.mEntityIDs)); }
   bool operator != (const BPlayerSpecificEntityIDs rhs) const { return ((mPlayerID != rhs.mPlayerID) || (mEntityIDs != rhs.mEntityIDs)); }
};

typedef BSmallDynamicSimArray<BPlayerSpecificEntityIDs> BPlayerSpecificEntityIDsArray;

// Move action collision results.
typedef enum
{
   cSimCollisionNone,
   cSimCollisionTerrain,
   cSimCollisionEdgeOfMap,
   cSimCollisionUnit,
   cSimCollisionSquad,
   cNumberSimCollisionResults
} BSimCollisionResult;


//=============================================================================
// AlertType
//=============================================================================
namespace AlertType
{
   enum
   {
      cInvalid = 0,
      cAttack,
      cFlare,
      cBaseDestruction,
      cResearchComplere,
      cTrainingComplete,
      cTransportComplete,
      // Update these if you add more types.
      cMin = cInvalid,
      cMax = cTrainingComplete,
   };
}


//=============================================================================
//=============================================================================
namespace DifficultyType
{
   enum 
   {
      // Standard types.
      cEasy = 0,
      cNormal,
      cHard,
      cLegendary,

      cNumberStandardDifficultyTypes,
      // Non standard types.
      cCustom = cNumberStandardDifficultyTypes,
      cAutomatic,

      // misc.
      cNumberDifficultyTypes,
   };
}


//==============================================================================
// BSelectionAbility
//==============================================================================
//typedef BInt8<BAbilityID, cInvalidAbilityID, INT8_MAX> BAbilityIDSmall;
//typedef BInt8<BAbilityType, cInvalidAbilityType, INT8_MAX> BAbilityTypeSmall;
//typedef BInt8<BAbilityTargetType, cInvalidAbilityTargetType, INT8_MAX> BAbilityTargetTypeSmall;
class BSelectionAbility
{
public:
   BSelectionAbility() : mAbilityID(-1), mAbilityType(-1), mTargetType(-1), mReverse(false), mValid(false), mTargetUnit(false), mRecovering(false), mPlayer(false), mHideYSpecial(false) {}

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   //int8                       mAbilityID;
   //int8                       mAbilityType;
   //int8                       mTargetType;
   BAbilityID                 mAbilityID;
   BAbilityType               mAbilityType;
   BAbilityTargetType         mTargetType;
   bool                       mReverse    : 1;
   bool                       mValid      : 1;
   bool                       mTargetUnit : 1;
   bool                       mRecovering : 1;
   bool                       mPlayer     : 1;
   bool                       mHideYSpecial : 1;
};