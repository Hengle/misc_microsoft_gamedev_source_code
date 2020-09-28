//==============================================================================
// powermanager.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once


#include "powertypes.h"


// Forward declarations.
class BPower;
class BPowerUser;
class BSquad;
class BProtoPower;
class BPowerDisruption;

//==============================================================================
// class BPowerManager
// Global power manager that hangs off world.
// Handles sim updating of power related logic.
//==============================================================================
class BPowerManager
{
public:
   BPowerManager();
   ~BPowerManager();

   BPower* createNewPower(long protoPowerID);
   BPower* getPowerByID(BPowerID powerID);
   BPower* getPowerByUserID(BPowerUserID powerUserID);
   void cancelPowerUser(BPowerUserID powerUserID);
   const BPower* getPowerByID(BPowerID powerID) const;
   uint getNumPowers() const { return (mPowers.getSize()); }
   const BPower* getPowerByIndex(uint powerIndex) const;
   void update(DWORD currentGameTime, float lastUpdateLength);

   bool isValidPowerLocation(const BVector& location, BPowerDisruption** pOutDisruptPower = NULL);
   bool isValidPowerLocation(const BVector& location, const BPower* pSourcePower, BPowerDisruption** pOutDisruptPower = NULL);
   bool isValidPowerLocation(const BVector& location, const BPowerUser* pSourcePowerUser, BPowerDisruption** pOutDisruptPower = NULL);

   bool isValidPowerLineSegment(const BVector& start, const BVector& end, BPowerDisruption** pOutDisruptPower = NULL);
   bool isValidPowerLineSegment(const BVector& start, const BVector& end, const BPower* pSourcePower, BPowerDisruption** pOutDisruptPower = NULL);
   bool isValidPowerLineSegment(const BVector& start, const BVector& end, const BPowerUser* pSourcePowerUser, BPowerDisruption** pOutDisruptPower = NULL);

   bool isValidPowerCircle(const BVector& start, float radius, BPowerDisruption** pOutDisruptPower = NULL);
   bool isValidPowerCircle(const BVector& start, float radius, const BPower* pSourcePower, BPowerDisruption** pOutDisruptPower = NULL);
   bool isValidPowerCircle(const BVector& start, float radius, const BPowerUser* pSourcePowerUser, BPowerDisruption** pOutDisruptPower = NULL);

   bool isSquadPowerValid(BPlayerID playerId, const BSquad& squad, const BProtoPower& protoPower, const BVector& targetLocation);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   BPower* allocatePower(BPowerType powerType);

   BPowerPtrArray mPowers;    // All the live powers currently in the sim.
   BPowerID mNextPowerID;     // This could wrap in our lifetime if we reduce the size of a BPowerID.
};
