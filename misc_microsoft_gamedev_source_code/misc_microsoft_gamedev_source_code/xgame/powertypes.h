//==============================================================================
// powertypes.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

// Forward declarations
class BPower;

// Typedefs
typedef int BPowerID;
typedef uint BPowerType;
typedef uint BPowerInputType;
typedef BSmallDynamicSimArray<BPower*> BPowerPtrArray;

#include "simtypes.h"

//==============================================================================
// class BPowerInput
// Helpful container class for encapsulating the different types of inputs a
// power might require (positions, entities, etc...)
//==============================================================================
class BPowerInput
{
public:
   BVector mVector;        // This could be a position, or a vector (depending on mType)
   BEntityID mEntityID;    // This could be a squad or a unit (depending on mType)
   BPowerInputType mType;  // What type are we?
};


// Power type enumeration.
namespace PowerType
{
   enum
   {
      cInvalid = 0,
      cCleansing,
      cOrbital,
      cCarpetBombing,
      cCryo,
      cRage,
      cWave,
      cDisruption,
      cTransport,
      cODST,
      cRepair,
      // Add additional power types here and update the cMin/cMax

      cMin = cInvalid,
      cMax = cRepair,
   };
}

// Power input type enumeration.
namespace PowerInputType
{
   enum
   {
      cInvalid = 0,
      cUserOK,
      cUserCancel,
      cPosition,
      cDirection,
      cSquad,
      cUnit,
      // Add additional power input types here and update the cMin/cMax

      cMin = cInvalid,
      cMax = cUnit,
   };
}


//==============================================================================
// BPowerUserID
//==============================================================================
class BPowerUserID
{
public:

   enum
   {
      cInvalidID           = UINT32_MAX, // Invalid ID
      cPlayerMask          = 0xF0000000, // Mask for the player
      cPowerTypeMask       = 0x0FF00000, // Mask for the refcount.
      cRefCountMask        = 0x000FFFFF, // Mask for the index.
      cPlayerBitShift      = 28,         // # of bits to shift the player within the ID
      cPowerTypeBitShift   = 20,         // # of bits to shift the power type within the ID
      cRefCountBitShift    = 0,          // # of bits to shift the refcount within the ID
   };

   BPowerUserID() : mID(static_cast<uint32>(BPowerUserID::cInvalidID)) {}
   BPowerUserID(uint32 val) { mID = static_cast<uint32>(val); }
   BPowerUserID(BPlayerID playerID, BPowerType powerType, uint refCount) { set(playerID, powerType, refCount); }
  
   BPlayerID getPlayerID() const { return static_cast<BPlayerID>((mID & BPowerUserID::cPlayerMask) >> BPowerUserID::cPlayerBitShift); }
   BPowerType getPowerType() const { return static_cast<BPowerType>((mID & BPowerUserID::cPowerTypeMask) >> BPowerUserID::cPowerTypeBitShift); }
   uint getRefCount() const { return static_cast<uint>((mID & BPowerUserID::cRefCountMask) >> BPowerUserID::cRefCountBitShift); }

   bool isValid() const { return (mID == BPowerUserID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BPowerUserID::cInvalidID); }
   void set(BPlayerID playerID, BPowerType powerType, uint refCount) { mID = ((playerID << BPowerUserID::cPlayerBitShift) & BPowerUserID::cPlayerMask) | ((powerType << BPowerUserID::cPowerTypeBitShift) & BPowerUserID::cPowerTypeMask) | ((refCount << BPowerUserID::cRefCountBitShift) & BPowerUserID::cRefCountMask); }
   bool operator == (const BPowerUserID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BPowerUserID rhs) const { return (mID != rhs.mID); }
   operator uint32() const { return (static_cast<int32>(mID)); }

   void setID(uint32 val) { mID = val; }
   uint32 getID() const { return mID; }

protected:
   uint32 mID;
};


__declspec(selectany) extern const BPowerUserID cInvalidPowerUserID = BPowerUserID();
__declspec(selectany) extern const BPowerID cInvalidPowerID = -1;
