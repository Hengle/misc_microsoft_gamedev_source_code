//==============================================================================
// damagehelper.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once 

#include "simtypes.h"

//==============================================================================
//Forward Declarations
//==============================================================================
class BEntity;
class BUnit;
class BProtoAction;
class BObject;

class IDamageInfo
{
public:
   virtual float  getDamagePerSecond(void) const = 0;
   virtual long   getWeaponType(void) const = 0;
   virtual DWORD  getShockwaveDuration(void) const = 0;
   virtual float  getAOERadius(void) const = 0;
   virtual float  getAOEDistanceFactor(void) const = 0;
   virtual float  getAOEDamageFactor(void) const = 0;
   virtual bool   getFlagAOELinearDamage() const = 0;
   virtual bool   getFriendlyFire(void) const = 0;
   virtual float  getAOEPrimaryTargetFactor(void) const = 0;
   virtual float  getPhysicsLaunchAngleMin() const = 0;
   virtual float  getPhysicsLaunchAngleMax() const = 0;
   virtual bool   getPhysicsLaunchAxial(void) const = 0;
   virtual float  getPhysicsForceMin() const = 0;
   virtual float  getPhysicsForceMax() const = 0;
   virtual float  getPhysicsForceMaxAngle() const = 0;
   virtual bool   getThrowUnits(void) const = 0;   
   virtual bool   getThrowDamageParts(void) const = 0;   
   virtual bool   getFlailThrownUnits(void) const = 0;   
   virtual bool   getDodgeable(void) const = 0;   
   virtual bool   getDeflectable(void) const = 0;   
   virtual bool   getSmallArmsDeflectable(void) const = 0;   
   virtual float  getMaxRange(const BUnit* pUnit, bool bIncludeGroupRange = true, bool bIncludePullRange=true) const = 0;   
   virtual void   getTriggerScript(BSimString& scriptName) const = 0;
   virtual bool   getInfection() const = 0;
   virtual float  getDOTrate() const = 0;
   virtual float  getDOTduration() const = 0;
   virtual long   getDOTEffect(BUnit* pUnit) const = 0;
   virtual float  getReapplyTime() const = 0;
   virtual float  getApplyTime() const = 0;
   virtual bool   getHalfKillCutoffFactor(long damageType, float& halfKillCutoffFactor) const = 0;
   virtual bool   getFlagStasis() const = 0;
   virtual bool   getFlagDaze() const = 0;
   virtual bool   getFlagAOEDaze() const = 0;
   virtual bool   getFlagAOEIgnoresYAxis() const = 0;
   virtual bool   getCausePhysicsExplosion() const = 0;
   virtual int    getPhysicsExplosionParticle() const = 0;
   virtual BObjectTypeID getPhysicsExplosionVictimType() const = 0;
   virtual bool   getFlagAirBurst() const = 0;
};


//==============================================================================
//BDamageStorage
//==============================================================================
class BDamageStorage 
{
public:
   BDamageStorage( void )
   {
      mUnitID.invalidate();
      mDamage = 0.0f;
   }

   ~BDamageStorage( void ) {}

   BEntityID   mUnitID;
   float       mDamage;
};

//==============================================================================
//BDamageHelper
//==============================================================================
class BDamageHelper
{
public:

   BDamageHelper(){}
   ~BDamageHelper(){}

   enum
   {
      cMaxUnitsDamaged = 256
   };   

   static bool       doesDamageWithWeaponTypeKillUnit(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID targetID, IDamageInfo* pDamageInfo, float damageAmount, long weaponType, bool isDirectionalDamage, BVector direction, float distanceFactor, BVector physicsForceOrigin, BEntityID attackerID = cInvalidObjectID);
   static float      doDamageWithWeaponType(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID targetID, IDamageInfo* pDamageInfo, float damageAmount, long weaponType, bool isDirectionalDamage, BVector direction, float distanceFactor, BVector physicsForceOrigin, BEntityID projectileID =cInvalidObjectID, BEntityID attackerID = cInvalidObjectID, bool* unitKilled = NULL, float* excessDamageOut = NULL, long hitZoneIndex = -1, bool doLogging = false, float* actualDamageDealt = NULL);
   static float      doAreaEffectDamage(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID attackerID, IDamageInfo* pDamageInfo, BVector aoeGroundZero, BEntityID projectileID =cInvalidObjectID, BVector direction=cOriginVector, BEntityIDArray* pKilledUnits=NULL, BEntityID primaryTargetID = cInvalidObjectID, long hitZoneIndex = -1, bool doLogging = false, BEntityIDArray* pDamagedUnits=NULL);
   static float      doAreaEffectDamage(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID attackerID, IDamageInfo* pDamageInfo, float damageOverride, BVector aoeGroundZero, BEntityID projectileID =cInvalidObjectID, BVector direction=cOriginVector, BEntityIDArray* pKilledUnits=NULL, BEntityID primaryTargetID = cInvalidObjectID, long hitZoneIndex = -1, bool doLogging = false, BEntityIDArray* pDamagedUnits=NULL);
   static float      getDamageAmount(BEntityID sourceId, float baseDamage, BEntityID targetId, bool usesHeightBonusDamage, bool displayLog = true);

//#ifndef BUILD_FINAL
//   static float      updateAttackTimer(BUnit *pUnit, float rate, float timer);
//#endif

protected:
   static float      doAreaEffectDamage(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID attackerID, IDamageInfo* pDamageInfo, float damageAmount, long weaponType, BVector aoeGroundZero, BVector direction, float aoeRadius, float aoeDistanceRatio, float aoeDamageRatio, bool friendlyFire, BEntityIDArray* killedUnits, BEntityID projectileID = cInvalidObjectID, BEntityID primaryTargetID = cInvalidObjectID, float primaryTargetDmgRatio = 1.0f, long hitZoneIndex = -1, bool doLogging = false, BEntityIDArray* pDamagedUnits=NULL);
   static void       useDamageBank(BUnit* pUnit, float &damageAmount, bool adjustBank = true);
};


//==============================================================================
//==============================================================================
class BDamageAreaOverTimeInstance
{
   public:
      BDamageAreaOverTimeInstance();
      virtual ~BDamageAreaOverTimeInstance();

      void                       reset();
      bool                       init(DWORD curTime, IDamageInfo* pDamageInfo, BPlayerID attackerPlayerID, BTeamID attackerTeamID, /*BVector location,*/ BEntityID attackerObjectID);
      void                       setObjectID(BEntityID id);
      bool                       update(DWORD elapsedTime);

      // Free list stuff
      virtual void               onAcquire();
      virtual void               onRelease();
      DECLARE_FREELIST(BDamageAreaOverTimeInstance, 4);

      // Static funcs for managing active instances
      static void                updateAllActiveInstances();
      static void                removeAllInstances();

   protected:

      BEntityIDArray             mDamagedEntityIDs;
      IDamageInfo*               mpDamageInfo;
      BEntityID                  mAttackerObjectID;
      BPlayerID                  mAttackerPlayerID;
      BTeamID                    mAttackerTeamID;
      DWORD                      mTimer;
      DWORD                      mEndTime;
      bool                       mAttackerObjectInitialized : 1;
      bool                       mDurationInitialized : 1;

      static BSmallDynamicSimArray<int> mActiveIndices;
};

// Callback function to set impact effect object associated with damage over time
void setObjectForDamageAreaOverTimeInstance(const BObject* pObject, DWORD freeListIndex);
