//==============================================================================
// entityID.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "hash/hash.h"

// Masks for the components that make up the BEntityID
__declspec(selectany) extern const long cObjectTypeMask = 0xF0000000; // Object/Unit/Squad/Dopple/Projectile/etc.
__declspec(selectany) extern const long cObjectCountMask = 0x0FFF0000; // Refcount
__declspec(selectany) extern const long cObjectIndexMask = 0x0000FFFF; // Index


// Amount to bitshift the components that make up the BEntityID
__declspec(selectany) extern const long cObjectTypeBitShift = 28;
__declspec(selectany) extern const long cObjectCountBitShift = 16;


//==============================================================================
// class BEntityID
// The BEntityID class is used to compose ID's for all the entities in the game.
// There are 3 components:
// 1.)  ObjectType - Is the entity a unit, squad, dopple, projectile, etc...
// 2.)  ObjectCount - The refcount of this particular object.
// 3.)  ObjectIndex - The actual position in the array of the object.
//==============================================================================
class BEntityID
{
   public:
      
      // Constructors
      BEntityID() : mID(-1) {}
      explicit BEntityID(long id) { mID = id; }
      BEntityID(uint type, uint count, uint index) { mID = ((type << cObjectTypeBitShift) | (count << cObjectCountBitShift) | index); }    

      // Set functionality for the entire BEntityID
      void set(uint type, uint count, uint index) { mID = ((type << cObjectTypeBitShift) | (count << cObjectCountBitShift) | index); }
      void set(long id) { mID = id; }

      // Get component functionality.
      uint getIndex() const { return static_cast<uint>(mID & cObjectIndexMask); }
      uint getUseCount() const { return static_cast<uint>((mID & cObjectCountMask) >> cObjectCountBitShift); }
      uint getType() const { return static_cast<uint>((mID & cObjectTypeMask) >> cObjectTypeBitShift); }
      
      // Checking validity, invalidating and returning the BEntityID as a long.
      long asLong() const { return (mID); }
      operator long() const { return (mID); }
      bool isValid() const { return (mID == 0xFFFFFFFF ? false : true); }
      void invalidate() { mID = 0xFFFFFFFF; }

      // Operator overloads
      bool operator == (const BEntityID rhs) const { return (mID == rhs.mID); }
      bool operator != (const BEntityID rhs) const { return (mID != rhs.mID); }

      //-- getBumpUseCount
      // mrh 8/22/06 - Note: this returns the bumped usecount handle, it doesn't modify anything.
      BEntityID getBumpUseCount(void)
      {
         uint index = getIndex();
         uint count = getUseCount();
         uint type = getType();
         count++;
         return BEntityID(type, count, index);
      }

#ifndef BUILD_FINAL
      //========================================================================
      // NOTE:
      // Helpful DEBUGGING ONLY function.  Therefore, it is using hard-coded numbers
      // in order to not include various headers in this file which is included everywhere.
      //========================================================================
      BFixedString32 getDebugStringLong()
      {
         BFixedString32 debugString;
         uint entityType = getType();
         uint entityIndex = getIndex();
         uint useCount = getUseCount();
         if (entityType == 0) //BEntity::cClassTypeObject
            debugString.format("O:%08d:%08d", useCount, entityIndex);
         else if (entityType == 1) //BEntity::cClassTypeUnit
            debugString.format("U:%08d:%08d", useCount, entityIndex);
         else if (entityType == 2) //BEntity::cClassTypeSquad
            debugString.format("S:%08d:%08d", useCount, entityIndex);
         else if (entityType == 3) //BEntity::cClassTypeDopple
            debugString.format("D:%08d:%08d", useCount, entityIndex);
         else if (entityType == 4) //BEntity::cClassTypeProjectile
            debugString.format("R:%08d:%08d", useCount, entityIndex);
         else if (entityType == 5) //BEntity::cClassTypePlatoon
            debugString.format("P:%08d:%08d", useCount, entityIndex);
         else if (entityType == 6) //BEntity::cClassTypeArmy
            debugString.format("A:%08d:%08d", useCount, entityIndex);
         else
            debugString.set("InvalidEntity");
         return(debugString);
      }
      BFixedString16 getDebugString()
      {
         BFixedString16 debugString;
         uint entityType = getType();
         if (entityType == 0) //BEntity::cClassTypeObject
            debugString.format("O:%d", mID);
         else if (entityType == 1) //BEntity::cClassTypeUnit
            debugString.format("U:%d", mID);
         else if (entityType == 2) //BEntity::cClassTypeSquad
            debugString.format("S:%d", mID);
         else if (entityType == 3) //BEntity::cClassTypeDopple
            debugString.format("D:%d", mID);
         else if (entityType == 4) //BEntity::cClassTypeProjectile
            debugString.format("R:%d", mID);
         else if (entityType == 5) //BEntity::cClassTypePlatoon
            debugString.format("P:%d", mID);
         else if (entityType == 6) //BEntity::cClassTypeArmy
            debugString.format("A:%d", mID);
         else
            debugString.set("InvalidEntity");
            
         /*uint entityIndex = getIndex();
         uint useCount = getUseCount();
         if (entityType == 0) //BEntity::cClassTypeObject
            debugString.format("O:%d:%d", useCount, entityIndex);
         else if (entityType == 1) //BEntity::cClassTypeUnit
            debugString.format("U:%d:%d", useCount, entityIndex);
         else if (entityType == 2) //BEntity::cClassTypeSquad
            debugString.format("S:%d:%d", useCount, entityIndex);
         else if (entityType == 3) //BEntity::cClassTypeDopple
            debugString.format("D:%d:%d", useCount, entityIndex);
         else if (entityType == 4) //BEntity::cClassTypeProjectile
            debugString.format("R:%d:%d", useCount, entityIndex);
         else if (entityType == 5) //BEntity::cClassTypePlatoon
            debugString.format("P:%d:%d", useCount, entityIndex);
         else if (entityType == 6) //BEntity::cClassTypeArmy
            debugString.format("A:%d:%d", useCount, entityIndex);*/
         return(debugString);
      }
#endif

   protected:
      long mID;
};

__declspec(selectany) extern const BEntityID cInvalidObjectID = BEntityID(-1);

typedef BSmallDynamicSimArray<BEntityID> BEntityIDArray;
typedef BSmallDynamicSimArray<BEntityIDArray> BEntityIDArrayArray;
typedef BEntityID BEntityHandle;

template <> struct BHasher<BEntityID> { size_t operator() (const BEntityID& key) const { long l = key.asLong(); return hashFast(&l, sizeof(long)); } };
template <> struct BHasher<const BEntityID> { size_t operator() (const BEntityID& key) const { long l = key.asLong(); return hashFast(&l, sizeof(long)); } };
