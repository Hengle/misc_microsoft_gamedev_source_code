//==============================================================================
// techeffect.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

#include "xmlreader.h"
#include "simtypes.h"

// Forward declarations
class BPlayer;
class BProtoObject;
class BProtoSquad;
class BTactic;
class BTechTree;

//==============================================================================
// BTechEffectTarget
//==============================================================================
class BTechEffectTarget
{
   public:
      long mTargetType;
      long mTargetID;      
};

//==============================================================================
// BTechEffect
//==============================================================================
class BTechEffect
{
   public:
      enum
      {
         cEffectData,
         cEffectTransformUnit,
         cEffectTransformProtoUnit,
         cEffectTransformProtoSquad,
         cEffectBuild,
         cEffectAge,
         cEffectPower,
         cEffectTech,
         cEffectAbility,
         cEffectSharedLOS,
         cEffectAttachSquad,
      };

      enum
      {
         cDataEnable,
         cDataHitpoints,
         cDataShieldpoints,
         cDataAmmoMax,
         cDataLOS,
         cDataMaximumVelocity,
         cDataMaximumRange,
         cDataResearchPoints,
         cDataResourceTrickleRate,
         cDataMaximumResourceTrickleRate,
         cDataRateAmount,
         cDataRateMultiplier,
         cDataResource,
         cDataProjectile,
         cDataDamage,
         cDataMinRange,
         cDataAOERadius,
         cDataAOEPrimaryTargetFactor,
         cDataAOEDistanceFactor,
         cDataAOEDamageFactor,   
         cDataAccuracy,
         cDataMovingAccuracy,
         cDataMaxDeviation,
         cDataMovingMaxDeviation,
         cDataAccuracyDistanceFactor,
         cDataAccuracyDeviationFactor,
         cDataMaxVelocityLead,
         cDataWorkRate,
         cDataBuildPoints,
         cDataCost,
         cDataAutoCloak,
         cDataCloakMove,
         cDataCloakAttack,
         cDataActionEnable,
         cDataCommandEnable,
         cDataBountyResource,
         cDataTributeCost,
         cDataShieldRegenRate,
         cDataShieldRegenDelay,
         cDataDamageModifier,
         cDataPopCap,
         cDataPopMax,
         cDataUnitTrainLimit,
         cDataSquadTrainLimit,
         cDataRepairCost,
         cDataRepairTime,
         cDataPowerRechargeTime,
         cDataPowerUseLimit,
         cDataLevel,
         cDataBounty,
         cDataMaxContained,
         cDataMaxDamagePerRam,
         cDataReflectDamageFactor,
         cDataAirBurstSpan,
         cDataAbilityDisabled,
         cDataDOTrate,
         cDataDOTduration,
         cDataImpactEffect,
         cDataAmmoRegenRate,
         cDataCommandSelectable,
         cDataDisplayNameID,
         cDataIcon,
         cDataAltIcon,
         cDataStasis,
         cDataTurretYawRate,
         cDataTurretPitchRate,
         cDataPowerLevel,
         cDataBoardTime,
         cDataAbilityRecoverTime,
         cDataTechLevel,
         cDataTechHPBar,
         cDataWeaponPhysicsMultiplier,
         cDataDeathSpawn,
      };

      enum
      {
         cRelativityAbsolute,
         cRelativityBasePercent,
         cRelativityPercent,
         cRelativityAssign,
         cRelativityBasePercentAssign,
      };

      enum
      {
         cTargetProtoUnit,
         cTargetProtoSquad,
         cTargetUnit,
         cTargetTech,
         cTargetTechAll,
         cTargetPlayer,
      };

                              BTechEffect();
                              ~BTechEffect();

      bool                    load(BXMLNode  root);

      void                    apply(BTechTree* pTechTree, long unitID, bool unapply, bool noCost);

      void                    applyProtoObjectEffect(BPlayer* pPlayer, BProtoObject* pCurrentProtoObject, BProtoObject* pBaseProtoObject, float amount, long dataSubType, long relativity, bool allActions, BSimString actionName, long resource, long objectType, bool unapply = false);
      void                    applyProtoSquadEffect(BPlayer* pPlayer, BProtoSquad* pCurrentProtoSquad, BProtoSquad* pBaseProtoSquad, float amount, long dataSubType, long relativity, bool allActions, BSimString actionName, long resource, long objectType, bool unapply = false);

   protected:
      void                    applyProtoObjectEffect(BPlayer* pPlayer, BProtoObject* pCurrentProtoObject, BProtoObject* pBaseProtoObject, bool unapply);
      void                    applyProtoSquadEffect(BPlayer* pPlayer, BProtoSquad* pCurrentProtoSquad, BProtoSquad* pBaseProtoSquad, bool unapply);
      void                    applyProtoActionEffect(BTactic* pCurrentTactic, BTactic* pBaseTactic, long actionID, bool unapply);
      void                    applyWeaponEffect(BPlayer* pPlayer, int protoObjectID, BTactic* pCurrentTactic, BTactic* pBaseTactic, long weaponID, bool unapply);
      float                   calcAmount(float baseVal, float currentVal, bool invert);
      void                    doProtoSquadTransform(BPlayer* pPlayer, BProtoSquadID fromProtoID, BProtoSquadID toProtoID);
      void                    getTargets(BXMLNode &root);

      long                    mEffectType;
      long                    mEffectValue;
      bool                    mAllActions;
      BSimString              mActionName;
      long                    mDataType;
      long                    mResource;
      long                    mObjectType;
      float                   mAmount;
      long                    mRelativity;      
      BDynamicSimArray<BTechEffectTarget>  mTargets;

};