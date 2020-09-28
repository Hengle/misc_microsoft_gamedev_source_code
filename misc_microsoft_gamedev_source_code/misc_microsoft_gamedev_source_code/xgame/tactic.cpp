//==============================================================================
// tactic.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================


#include "common.h"
#include "tactic.h"
#include "xmlreader.h"
#include "database.h"
#include "gamedirectories.h"
#include "unit.h"
#include "protoobject.h"
#include "protosquad.h"
#include "actionmanager.h"
#include "action.h"
#include "visualmanager.h"
#include "weapontype.h"
#include "world.h"
#include "protoimpacteffect.h"
#include "configsgame.h"
#include "ui.h"
#include "gamepad.h"
#include "usermanager.h"
#include "user.h"
#include "selectionmanager.h"
#include "ability.h"
#include "grannyanimation.h"
#include "grannymanager.h"
#include "camera.h"
#include "particlegateway.h"
#include "soundmanager.h"
#include "archiveManager.h"
#include "unitactionrevive.h"
#include "unitactionchargedrangedattack.h"

GFIMPLEMENTVERSION(BTactic, 2);
GFIMPLEMENTVERSION(BProtoAction, 2);
GFIMPLEMENTVERSION(BWeapon, 3);
enum
{
   cSaveMarkerTactic=10000,
   cSaveMarkerProtoAction,
   cSaveMarkerWeapon,
};

// -- BWeapon

//==============================================================================
// BWeaponStatic::BWeaponStatic
//==============================================================================
BWeaponStatic::BWeaponStatic() :
   mName(),
   mImpactEffectSize(-1),
   mImpactCameraShakeDuration(0.0f),
   mImpactCameraShakeStrength(0.0f),
   mWeaponType(-1),
   mHardpointID(-1),
   mAlternateHardpointID(-1),
   mDamageRatingOverride(),
   mVisualAmmo(0),
   mpImpactRumble(NULL),
   mpImpactCameraEffect(NULL),
   mTargetsFootOfUnit(false),   
   mKeepDPSRamp(false),
   mCausePhysicsExplosion(false),
   mPostAttackCooldownMin(0.0f),
   mPostAttackCooldownMax(0.0f),
   mPreAttackCooldownMin(0.0f),
   mPreAttackCooldownMax(0.0f),
   mAttackRate(0.0f),

   mStartDPSPercent(100.0f),
   mFinalDPSPercent(100.0f),
   mDPSRampTime(0.0f),
   mPhysicsLaunchAngleMin(0.0f),
   mPhysicsLaunchAngleMax(0.0f),
   mPhysicsForceMin(0.0f),
   mPhysicsForceMax(0.0f),
   mPhysicsForceMaxAngle(0.0f),
   mBeamSearchRadius(20.0f),
   mBeamActivationSpeed(100.0f),
   mBeamTrackingSpeed(0.15f),
   mBeamTurnRate(90.0f),
   mBeamWaitTime(5.0f),
   mDazeDuration(0.0f),
   mDazeMovementModifier(0.0f),
   mDazeTargetTypeID(-1),
   mBounceRange(0.0f),
   mShakeScalarNotLocal(1.0f),
   mSmartTargetType(-1),
   mNumBounces(0),
   mMaxPullRange(0.0f),
   mStasisHealToDrainRatio(0.0f),
   mThrowOffsetAngle(0.0f),
   mThrowVelocity(0.0f),
   mDOTEffectSmall(-1),
   mDOTEffectMedium(-1),
   mDOTEffectLarge(-1),
   mReapplyTime(0.0f),
   mApplyTime(0.0f),
   mPhysicsExplosionParticleName(),
   mPhysicsExplosionParticleHandle(-1),
   mPhysicsExplosionVictimType(-1),
   mFlagPullUnits(false),
   mFlagAirBurst(false),
   mFlagShieldDrain(false)
{
   mTriggerScript = "";
}

//==============================================================================
// BWeaponStatic::~BWeaponStatic
//==============================================================================
BWeaponStatic::~BWeaponStatic()
{
   if (mpImpactRumble)
   {
      delete mpImpactRumble;
      mpImpactRumble = NULL;
   }
   if (mpImpactCameraEffect)
   {
      delete mpImpactCameraEffect;
      mpImpactCameraEffect = NULL;
   }
}

//==============================================================================
// BWeapon::BWeapon
//==============================================================================
BWeapon::BWeapon() :
   mDamagePerSecond(0.0f),
   mDOTrate(0.0f),
   mDOTduration(0.0f),
   mMaxRange(cDefaultMaxRange),
   mAOERadius(0.0f),
   mAOEPrimaryTargetFactor(0.0f),
   mAOEDistanceFactor(0.0f),
   mAOEDamageFactor(0.0f),   
   mMinRange(0.0f),
   mAccuracy(1.0f),
   mMovingAccuracy(1.0f),
   mMaxDeviation(0.0f),
   mMovingMaxDeviation(0.0f),
   mAccuracyDistanceFactor(0.5f),
   mAccuracyDeviationFactor(0.5f),
   mMaxVelocityLead(0.0f),
   mMaxDamagePerRam(0.0f),
   mReflectDamageFactor(0.0f),
   mProjectileObjectID(-1),
   mImpactEffectProtoID(-1),
   mAirBurstSpan(-1.0f),
   mpStaticData(NULL)
{
   // Init Flags
   mFlagOwnStaticData=false;
   mFlagFriendlyFire=false;
   mFlagHeightBonusDamage=false;
   mFlagUsesAmmo=false;
   mFlagThrowUnits=false;
   mFlagThrowAliveUnits=false;
   mFlagThrowDamageParts=false;
   mFlagPhysicsLaunchAxial=false;
   mFlagUsesBeam=false;  
   mFlagFlailThrownUnits=false;
   mFlagDodgeable=false;
   mFlagDeflectable=false;
   mFlagSmallArmsDeflectable=false;
   mFlagUseDPSasDPA=false;
   mFlagDoShockwaveAction=false;
   mFlagUseGroupRange=false;
   mFlagCarriedObjectAsProjectileVisual=false;
   mFlagStasis=false;
   mFlagStasisDrain=false;
   mFlagStasisBomb=false;
   mFlagKnockback=false;
   mFlagTentacle=false;
   mFlagDaze=false;
   mFlagAOEDaze=false;
   mFlagAOELinearDamage=false;
   mFlagAOEIgnoresYAxis=false;
   mFlagOverridesRevive=false;
}

//==============================================================================
// BWeapon::BWeapon
//==============================================================================
BWeapon::BWeapon(const BWeapon* pBase)
{
   // Dynamic data
   mDamagePerSecond=pBase->mDamagePerSecond;
   mDOTrate=pBase->mDOTrate;
   mDOTduration=pBase->mDOTduration;
   mMaxRange=pBase->mMaxRange;
   mAOERadius=pBase->mAOERadius;
   mAOEPrimaryTargetFactor=pBase->mAOEPrimaryTargetFactor;
   mAOEDistanceFactor=pBase->mAOEDistanceFactor;
   mAOEDamageFactor=pBase->mAOEDamageFactor;
   mMinRange=pBase->mMinRange;
   mAccuracy=pBase->mAccuracy;
   mMovingAccuracy=pBase->mMovingAccuracy;
   mMaxDeviation=pBase->mMaxDeviation;
   mMovingMaxDeviation=pBase->mMovingMaxDeviation;
   mAccuracyDistanceFactor=pBase->mAccuracyDistanceFactor;
   mAccuracyDeviationFactor=pBase->mAccuracyDeviationFactor;
   mMaxVelocityLead=pBase->mMaxVelocityLead;
   mMaxDamagePerRam=pBase->mMaxDamagePerRam;
   mReflectDamageFactor=pBase->mReflectDamageFactor;
   mProjectileObjectID=pBase->mProjectileObjectID;
   mImpactEffectProtoID=pBase->mImpactEffectProtoID;
   mAirBurstSpan=pBase->mAirBurstSpan;

   // Flags
   mFlagOwnStaticData=pBase->mFlagOwnStaticData;
   mFlagFriendlyFire=pBase->mFlagFriendlyFire;
   mFlagHeightBonusDamage=pBase->mFlagHeightBonusDamage;
   mFlagUsesAmmo=pBase->mFlagUsesAmmo;
   mFlagThrowUnits=pBase->mFlagThrowUnits;
   mFlagThrowAliveUnits=pBase->mFlagThrowAliveUnits;
   mFlagThrowDamageParts=pBase->mFlagThrowDamageParts;
   mFlagPhysicsLaunchAxial=pBase->mFlagPhysicsLaunchAxial;
   mFlagUsesBeam=pBase->mFlagUsesBeam;
   mFlagFlailThrownUnits=pBase->mFlagFlailThrownUnits;
   mFlagDodgeable=pBase->mFlagDodgeable;
   mFlagDeflectable=pBase->mFlagDeflectable;
   mFlagSmallArmsDeflectable=pBase->mFlagSmallArmsDeflectable;
   mFlagUseDPSasDPA=pBase->mFlagUseDPSasDPA;
   mFlagUseGroupRange=pBase->mFlagUseGroupRange;
   mFlagDoShockwaveAction=pBase->mFlagDoShockwaveAction;   
   mFlagCarriedObjectAsProjectileVisual=pBase->mFlagCarriedObjectAsProjectileVisual;
   mFlagStasis=pBase->mFlagStasis;
   mFlagStasisDrain=pBase->mFlagStasisDrain;
   mFlagStasisBomb=pBase->mFlagStasisBomb;
   mFlagKnockback=pBase->mFlagKnockback;
   mFlagTentacle=pBase->mFlagTentacle;
   mFlagDaze=pBase->mFlagDaze;
   mFlagAOEDaze=pBase->mFlagAOEDaze;
   mFlagAOELinearDamage=pBase->mFlagAOELinearDamage;
   mFlagAOEIgnoresYAxis=pBase->mFlagAOEIgnoresYAxis;
   mFlagOverridesRevive=pBase->mFlagOverridesRevive;

   // Static data
   mpStaticData=pBase->mpStaticData;
   mFlagOwnStaticData = false;
}


//==============================================================================
// BWeapon::~BWeapon
//==============================================================================
BWeapon::~BWeapon()
{
   if(mpStaticData)
   {
      if(getFlagOwnStaticData())
      {
         delete mpStaticData;
         mFlagOwnStaticData = false;
      }
      mpStaticData=NULL;
   }
}

//==============================================================================
// BWeapon::loadWeapon
//==============================================================================
bool BWeapon::loadWeapon(BXMLNode root, const BProtoObject *pObj)
{
   mpStaticData=new BWeaponStatic();
   if(!mpStaticData)
      return false;
   mFlagOwnStaticData = true;

   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString& name=node.getName();

      BSimString tempStr;
      
      if(name == "Name")
         node.getText(mpStaticData->mName);
      else if(name == "DamagePerSecond") //-- If one shot item then this is simply the damage to be dealt
         node.getTextAsFloat(mDamagePerSecond);
      else if(name == "DPSRamp")
      {
         node.getAttribValueAsFloat("startDPSPercent", mpStaticData->mStartDPSPercent);
         node.getAttribValueAsFloat("finalDPSPercent", mpStaticData->mFinalDPSPercent);
         node.getAttribValueAsFloat("rampTime", mpStaticData->mDPSRampTime);
         bool shieldDrain = false;
         node.getAttribValueAsBool("shieldDrain", shieldDrain);
         mpStaticData->mFlagShieldDrain = shieldDrain;

         BASSERT(mpStaticData->mStartDPSPercent >= 0.0f);
         BASSERT(mpStaticData->mFinalDPSPercent >= 0.0f);
         BASSERT(mpStaticData->mDPSRampTime != 0.0f);
      }
      else if(name == "DOTrate")
         node.getTextAsFloat(mDOTrate);
      else if(name == "DOTduration")
         node.getTextAsFloat(mDOTduration);
      else if(name == "DOTEffect")
      {
         mpStaticData->mDOTEffectSmall = gDatabase.getProtoObject(node.getTextPtr(tempStr));
         mpStaticData->mDOTEffectMedium = gDatabase.getProtoObject(node.getTextPtr(tempStr));
         mpStaticData->mDOTEffectLarge = gDatabase.getProtoObject(node.getTextPtr(tempStr));

         BXMLAttribute attrib;
         BSimString effectStr;

         if (node.getAttribute("small", &attrib))
         {
            node.getAttribValueAsString("small", effectStr);
            mpStaticData->mDOTEffectSmall = gDatabase.getProtoObject(effectStr.getPtr());
         }
         if (node.getAttribute("medium", &attrib))
         {
            node.getAttribValueAsString("medium", effectStr);
            mpStaticData->mDOTEffectMedium = gDatabase.getProtoObject(effectStr.getPtr());
         }
         if (node.getAttribute("large", &attrib))
         {
            node.getAttribValueAsString("large", effectStr);
            mpStaticData->mDOTEffectLarge = gDatabase.getProtoObject(effectStr.getPtr());
         }
      }
      else if (name == "Reapply")
         node.getTextAsFloat(mpStaticData->mReapplyTime);
      else if (name == "Apply")
         node.getTextAsFloat(mpStaticData->mApplyTime);
      else if(name == "PostAttackCooldownMin")
         node.getTextAsFloat(mpStaticData->mPostAttackCooldownMin);
      else if(name == "PostAttackCooldownMax")
         node.getTextAsFloat(mpStaticData->mPostAttackCooldownMax);
      else if(name == "PreAttackCooldownMin")
         node.getTextAsFloat(mpStaticData->mPreAttackCooldownMin);
      else if(name == "PreAttackCooldownMax")
         node.getTextAsFloat(mpStaticData->mPreAttackCooldownMax);
      else if(name == "AttackRate")
         node.getTextAsFloat(mpStaticData->mAttackRate);
      else if(name == "Projectile")
      {
         mProjectileObjectID = gDatabase.getProtoObject(node.getTextPtr(tempStr));
         if(mProjectileObjectID == -1)
            gConsoleOutput.output(cMsgError, "Invalid projectile protoObject specified: %s, in BWeapon::loadWeapon.", node.getTextPtr(tempStr));
      }
      else if(name == "ImpactEffect")
      {
         BString impactEffectName = node.getText(tempStr);
         impactEffectName.toLower();
         mImpactEffectProtoID = gDatabase.getProtoImpactEffectIndex(impactEffectName.getPtr());

         BSimString szName;
         node.getAttribValueAsString("size", szName);
         mpStaticData->mImpactEffectSize = gDatabase.getImpactEffectSize(szName.getPtr());

         bool shockwave = false;
         if (node.getAttribValueAsBool("doShockwaveAction", shockwave))
            mFlagDoShockwaveAction = shockwave;
      }
      else if(name == "ImpactCameraShake")
      {
         node.getAttribValueAsFloat("Duration", mpStaticData->mImpactCameraShakeDuration);
         node.getAttribValueAsFloat("Strength", mpStaticData->mImpactCameraShakeStrength);
      }
      else if(name == "ImpactRumble")
      {
         mpStaticData->mpImpactRumble = new BRumbleEvent();
         if (mpStaticData->mpImpactRumble)
         {
            BSimString szName;
            if (node.getAttribValueAsString("LeftRumbleType", szName))
               mpStaticData->mpImpactRumble->mLeftRumbleType=(int8)BGamepad::getRumbleType(szName);
            if (node.getAttribValueAsString("RightRumbleType", szName))
               mpStaticData->mpImpactRumble->mRightRumbleType=(int8)BGamepad::getRumbleType(szName);
            node.getAttribValueAsHalfFloat("Duration", mpStaticData->mpImpactRumble->mDuration);
            node.getAttribValueAsHalfFloat("LeftStrength", mpStaticData->mpImpactRumble->mLeftStrength);
            node.getAttribValueAsHalfFloat("RightStrength", mpStaticData->mpImpactRumble->mRightStrength);
         }
      }
      else if (name == "ImpactCameraEffect")
      {
         mpStaticData->mpImpactCameraEffect = new BCameraEffectData();
         if (mpStaticData->mpImpactCameraEffect)
            mpStaticData->mpImpactCameraEffect->load(node);
      }
      else if(name == "WeaponType") //-- Determines what damage modifier to apply based on our target
      {
         mpStaticData->mWeaponType = gDatabase.getWeaponTypeIDByName(node.getTextPtr(tempStr));
      }
      else if(name == "VisualAmmo")
      {
         int visualAmmo=0;
         if (node.getTextAsInt(visualAmmo))
            mpStaticData->mVisualAmmo=static_cast<uint>(visualAmmo);
      }
      else if(name == "AOERadius") //-- Size of splash damage radius
      {
         node.getTextAsFloat(mAOERadius);
      }      
      else if(name == "AOEPrimaryTargetFactor") //-- Determines what percent of total damage is dealt to our primary target
      {
         node.getTextAsFloat(mAOEPrimaryTargetFactor);
      }
      //-- AOEDistanceFactor and AOEDamageFactor factor allow the designer to specify
      //-- a point along the AOERadius and specify the amount of damage that should be
      //-- dealt at that point. The damage is distrusted based on this curve and then
      //-- normalized if the damage exceeds the maximum damage. Both values are percentages
      //-- from 0.0 to 1.0
      else if(name == "AOEDistanceFactor")
      {
         node.getTextAsFloat(mAOEDistanceFactor);
      }
      else if(name == "AOEDamageFactor")
      {
         node.getTextAsFloat(mAOEDamageFactor);
      }
      else if(name == "AOELinearDamage")
      {
         mFlagAOELinearDamage = true;
      }
      else if(name == "PhysicsLaunchAngleMin")
      {
         node.getTextAsFloat(mpStaticData->mPhysicsLaunchAngleMin);
      }
      else if(name == "PhysicsLaunchAngleMax")
      {
         node.getTextAsFloat(mpStaticData->mPhysicsLaunchAngleMax);
      }
      else if(name == "PhysicsLaunchAxial")
      {
         mFlagPhysicsLaunchAxial = true;
      }
      else if(name == "PhysicsForceMin")
      {
         node.getTextAsFloat(mpStaticData->mPhysicsForceMin);
      }
      else if(name == "PhysicsForceMax")
      {
         node.getTextAsFloat(mpStaticData->mPhysicsForceMax);
      }
      else if(name == "PhysicsForceMaxAngle")
      {
         node.getTextAsFloat(mpStaticData->mPhysicsForceMaxAngle);
      }
      else if(name == "ThrowUnits")
      {
         mFlagThrowUnits = true;
      }
      else if(name == "ThrowAliveUnits")
      {
         mFlagThrowAliveUnits = true;
      }    
      else if(name == "ThrowDamageParts")
      {
         mFlagThrowDamageParts = true;
      }
      else if(name == "FlailThrownUnits")
      {
         mFlagFlailThrownUnits = true;
      }
      else if(name == "Dodgeable")
      {
         mFlagDodgeable = true;
      }
      else if(name == "Deflectable")
      {
         mFlagDeflectable = true;
      }
      else if(name == "SmallArmsDeflectable")
      {
         mFlagSmallArmsDeflectable = true;
      }
      else if(name == "OverridesRevive")
      {
         mFlagOverridesRevive = true;
      }
      else if(name == "PullUnits")
      {
         mpStaticData->mFlagPullUnits = true;
      }
      else if (name == "UseDPSasDPA")
      {
         mFlagUseDPSasDPA = true;
      }
      else if (name == "UseGroupRange")
      {
         mFlagUseGroupRange = true;
      }
      else if(name == "CarriedObjectAsProjectileVisual")
      {
         mFlagCarriedObjectAsProjectileVisual = true;
      }
      else if (name=="Hardpoint")
      {
         mpStaticData->mHardpointID = pObj->findHardpoint(node.getTextPtr(tempStr));
      }
      else if (name=="AlternateHardpoint")
      {
         mpStaticData->mAlternateHardpointID = pObj->findHardpoint(node.getTextPtr(tempStr));
      }
      else if (name=="TriggerScript")
      {
         mpStaticData->mTriggerScript = node.getTextPtr(tempStr);
      }
      else if(name == "AllowFriendlyFire")
      {         
         mFlagFriendlyFire = true;
      }
      else if(name == "EnableHeightBonusDamage")
      {
         mFlagHeightBonusDamage = true;
      }      
      else if(name == "MinRange")
      {
         node.getTextAsFloat(mMinRange);
      }
      else if(name == "MaxRange")
      {
         float temp = 0.0f;
         node.getTextAsFloat(temp);                     
         if (temp > mMaxRange) // Never let mMaxRange be less than default value
            mMaxRange = temp;
      }    
      else if(name == "UsesAmmo")
      {
         mFlagUsesAmmo = true;
      }
      else if(name == "Beam")
      {
         node.getAttribValueAsFloat("activationSpeed", mpStaticData->mBeamActivationSpeed);
         node.getAttribValueAsFloat("trackingSpeed", mpStaticData->mBeamTrackingSpeed);
         node.getAttribValueAsFloat("turnRate", mpStaticData->mBeamTurnRate);
         node.getAttribValueAsFloat("waitTime", mpStaticData->mBeamWaitTime);
         node.getAttribValueAsFloat("searchRadius", mpStaticData->mBeamSearchRadius);
         mFlagUsesBeam = true;
      }
      else if(name == "MaxDamagePerRam")
      {
         node.getTextAsFloat(mMaxDamagePerRam);
      }
      else if(name == "ReflectDamageFactor")
      {
         node.getTextAsFloat(mReflectDamageFactor);
      }
      else if(name == "Accuracy")
      {
         node.getTextAsFloat(mAccuracy);
      }
      else if(name == "MovingAccuracy")
      {
         node.getTextAsFloat(mMovingAccuracy);
      }
      else if(name == "MaxDeviation")
      {
         node.getTextAsFloat(mMaxDeviation);
      }
      else if(name == "MovingMaxDeviation")
      {
         node.getTextAsFloat(mMovingMaxDeviation);
      }
      else if(name == "AccuracyDistanceFactor")
      {
         node.getTextAsFloat(mAccuracyDistanceFactor);
      }
      else if(name == "AccuracyDeviationFactor")
      {
         node.getTextAsFloat(mAccuracyDeviationFactor);
      }
      else if(name == "MaxVelocityLead")
      {
         node.getTextAsFloat(mMaxVelocityLead);
      }
      else if(name == "AirBurstSpan")
      {
         node.getTextAsFloat(mAirBurstSpan);
      }
      else if(name == "DamageRatingOverride")
      {
         if(node.getAttribValueAsString("type", tempStr))
         {
            BWeaponDamageTypeRatingOverride rating;
            rating.mDamageType=gDatabase.getDamageType(tempStr);
            if(rating.mDamageType!=-1)
            {
               rating.mDamageRating=1.0f;
               node.getTextAsFloat(rating.mDamageRating);

               rating.mHalfKillCutoffFactor = -1.0f;
               node.getAttribValueAsFloat("halfKillCutoffFactor", rating.mHalfKillCutoffFactor);

               mpStaticData->mDamageRatingOverride.add(rating);
            }
         }
      }
      else if(name == "TargetPriority")
      {  
         BSimString str; 
         node.getAttribValueAsString("type", str);
         long objectType = gDatabase.getObjectType(str);
         if(objectType == -1)
            gConsoleOutput.output(cMsgError, "Invalid object type specified: Line %d of %s", root.getReader()->getLineNumber(), root.getReader()->getFilename().getPtr());
         else
         {
            BWeaponStatic::BTargetPriority& targetPri =  mpStaticData->mTargetPriorities.grow();
            targetPri.mProtoID = objectType;
            bool result = node.getTextAsFloat(targetPri.mPriorityAdjustment);
            if(!result)
               targetPri.mPriorityAdjustment = 0.0f;
         }    
      } 
      else if(name == "TargetsFootOfUnit")
      {
         mpStaticData->mTargetsFootOfUnit = true;
      }
      else if(name == "KeepDPSRamp")
      {
         mpStaticData->mKeepDPSRamp = true;
      }
      else if (name == "CausePhysicsExplosion")
      {
         mpStaticData->mCausePhysicsExplosion = true;

         /*
         BString particleName;
         node.getAttribValueAsString("particle", particleName);
         BParticleEffectDataHandle particleDataHandle;
         gParticleGateway.getOrCreateData(particleName, particleDataHandle);
         mpStaticData->mPhysicsExplosionParticle = (int)particleDataHandle;
         */
         node.getAttribValueAsString("particle", mpStaticData->mPhysicsExplosionParticleName);

         BString victimTypeName;
         node.getAttribValueAsString("victimType", victimTypeName);
         mpStaticData->mPhysicsExplosionVictimType = gDatabase.getObjectType(victimTypeName);
      }
      else if(name == "Stasis")
      {
         bool tempBool = false;
         node.getAttribValueAsBool("SmartTargeting", tempBool);
         if (tempBool)
         {
            mFlagStasis = true;
            mpStaticData->mSmartTargetType = BSquad::cSmartTargetStasis;
         }
      }
      else if(name == "StasisDrain")
      {
         mFlagStasisDrain = true;
      }
      else if(name == "StasisBomb")
      {
         mFlagStasisBomb = true;
      }
      else if(name == "StasisHealToDrainRatio")
      {
         node.getTextAsFloat(mpStaticData->mStasisHealToDrainRatio);
      }
      else if(name == "ThrowOffsetAngle")
      {
         node.getTextAsAngle(mpStaticData->mThrowOffsetAngle);
      }
      else if(name == "ThrowVelocity")
      {
         node.getTextAsFloat(mpStaticData->mThrowVelocity);
      }
      else if(name == "ApplyKnockback")
      {
         mFlagKnockback = true;
      }
      else if (name == "Tentacle")
      {
         mFlagTentacle = true;
      }
      else if(name == "Daze")
      {
         mFlagDaze = true;
      
         node.getTextAsFloat(mpStaticData->mDazeDuration);

         mpStaticData->mDazeDuration *= 1000.0f;

         BSimString szName;
         node.getAttribValueAsString("TargetType", szName); 
         mpStaticData->mDazeTargetTypeID = gDatabase.getObjectType(szName.getPtr());

         mpStaticData->mDazeMovementModifier = 0;
         node.getAttribValueAsFloat("MovementModifier", mpStaticData->mDazeMovementModifier);

         bool tempBool = true;
         node.getAttribValueAsBool("AOEDaze", tempBool);
         mFlagAOEDaze = tempBool;

         tempBool = false;
         node.getAttribValueAsBool("SmartTargeting", tempBool);
         if (tempBool)
            mpStaticData->mSmartTargetType = BSquad::cSmartTargetDaze;
      }
      else if(name == "AOEIgnoresYAxis")
      {
         mFlagAOEIgnoresYAxis = true;
      }
      else if(name == "Bounces")
      {
         node.getTextAsInt8(mpStaticData->mNumBounces);
      }
      else if(name =="BounceRange")
      {
         node.getTextAsFloat(mpStaticData->mBounceRange);
      }
      else if (name=="CameraRubleShakeScalarNotLocal")
      {
         node.getTextAsFloat(mpStaticData->mShakeScalarNotLocal);
      }
      else if (name == "AirBurst")
      {
         mpStaticData->mFlagAirBurst = true;
      }
      else if(name == "MaxPullRange")
      {
         float temp = 0.0f;
         node.getTextAsFloat(temp);                     
         if (temp > mpStaticData->mMaxPullRange) // Never let mMaxPullRange be less than default value
            mpStaticData->mMaxPullRange = temp;
      }    
   }
   return true;
}



//==============================================================================
// BWeapon::postloadWeapon
//==============================================================================
void BWeapon::postloadWeapon()
{
   // Load needed pfx files
   //

   if(!mpStaticData->mPhysicsExplosionParticleName.isEmpty())
   {
      gParticleGateway.getOrCreateData(mpStaticData->mPhysicsExplosionParticleName, mpStaticData->mPhysicsExplosionParticleHandle);
   }
}

//==============================================================================
//==============================================================================
bool BWeapon::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, float, mDamagePerSecond);
   GFWRITEVAR(pStream, float, mDOTrate);
   GFWRITEVAR(pStream, float, mDOTduration);
   GFWRITEVAR(pStream, float, mMaxRange);
   GFWRITEVAR(pStream, float, mMinRange);
   GFWRITEVAR(pStream, float, mAOERadius);
   GFWRITEVAR(pStream, float, mAOEPrimaryTargetFactor);
   GFWRITEVAR(pStream, float, mAOEDistanceFactor);
   GFWRITEVAR(pStream, float, mAOEDamageFactor);   
   GFWRITEVAR(pStream, float, mAccuracy);
   GFWRITEVAR(pStream, float, mMovingAccuracy);
   GFWRITEVAR(pStream, float, mMaxDeviation);
   GFWRITEVAR(pStream, float, mMovingMaxDeviation);
   GFWRITEVAR(pStream, float, mAccuracyDistanceFactor);
   GFWRITEVAR(pStream, float, mAccuracyDeviationFactor);
   GFWRITEVAR(pStream, float, mMaxVelocityLead);
   GFWRITEVAR(pStream, float, mMaxDamagePerRam);
   GFWRITEVAR(pStream, float, mReflectDamageFactor);
   GFWRITEVAR(pStream, float, mAirBurstSpan);
   GFWRITEVAR(pStream, long, mProjectileObjectID);
   GFWRITEVAR(pStream, long, mImpactEffectProtoID);

   GFWRITEMARKER(pStream, cSaveMarkerWeapon);
   return true;
}

//==============================================================================
//==============================================================================
bool BWeapon::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, float, mDamagePerSecond);
   GFREADVAR(pStream, float, mDOTrate);
   GFREADVAR(pStream, float, mDOTduration);
   GFREADVAR(pStream, float, mMaxRange);
   GFREADVAR(pStream, float, mMinRange);
   GFREADVAR(pStream, float, mAOERadius);
   GFREADVAR(pStream, float, mAOEPrimaryTargetFactor);
   GFREADVAR(pStream, float, mAOEDistanceFactor);
   GFREADVAR(pStream, float, mAOEDamageFactor);   
   GFREADVAR(pStream, float, mAccuracy);
   GFREADVAR(pStream, float, mMovingAccuracy);
   GFREADVAR(pStream, float, mMaxDeviation);
   GFREADVAR(pStream, float, mMovingMaxDeviation);
   GFREADVAR(pStream, float, mAccuracyDistanceFactor);
   GFREADVAR(pStream, float, mAccuracyDeviationFactor);
   GFREADVAR(pStream, float, mMaxVelocityLead);
   GFREADVAR(pStream, float, mMaxDamagePerRam);
   GFREADVAR(pStream, float, mReflectDamageFactor);
   GFREADVAR(pStream, float, mAirBurstSpan);
   GFREADVAR(pStream, long, mProjectileObjectID);
   GFREADVAR(pStream, long, mImpactEffectProtoID);

   GFREADMARKER(pStream, cSaveMarkerWeapon);

   gSaveGame.remapProtoObjectID(mProjectileObjectID);

   if (mGameFileVersion >= 3)
      gSaveGame.remapImpactEffectID(mImpactEffectProtoID);
   else
   {
       if (mImpactEffectProtoID >= gDatabase.getNumberImpactEffects())
          mImpactEffectProtoID = -1;
   }

   return true;
}

// -- BProtoAction

//==============================================================================
// BProtoActionStatic::BProtoActionStatic
//==============================================================================
BProtoActionStatic::BProtoActionStatic() : 
   mID(-1),
   mActionType(BAction::cActionTypeInvalid),
   mPersistentActionType(BAction::cActionTypeInvalid),
   mWeaponID(-1),
   mLinkedActionID(-1),
   mReloadAnimType(-1),
   mWorkRange(cDefaultMaxRange),
   mDodgeChanceMin(0.0f),
   mDodgeChanceMax(0.0f),
   mDodgeMaxAngle(cPi),
   mDodgeCooldown(0),
   mDodgePhysicsImpulse(0.0f),
   mDeflectChanceMin(0.0f),
   mDeflectChanceMax(0.0f),
   mDeflectMaxAngle(cPi),
   mDeflectCooldown(0),
   mDeflectMaxDmg(0.0f),
   mDeflectTimeout(0),
   mStrafingMaxDistance(0.0f),
   mStrafingTrackingSpeed(-1.0f),
   mStrafingJitter(-1.0f),
   mStartAnimType(-1),
   mEndAnimType(-1),
   mProjectileSpread(0.0f),
   mStopAttackingWhenAmmoDepleted(false),
   mDontLoopAttackAnim(false),
   mDontCheckOrientTolerance(false),
   mKillSelfOnAttack(false),
   mMeleeRange(false),
   mInstantAttack(false),
   mInfection(false),
   mCanOrientOwner(true),
   mExposedActionIndex(-1),
   mSlaveAttackActionID(-1),
   mBaseDPSWeaponID(-1),
   mMainAttack(false),
   mStationary(false),
   mStrafing(false),
   mHoverAltitudeOffset(cDefaultHoverAltitudeOffset),
   mMaxTgtDepressionAngle(cDefaultMaxTgtDepressionAngle),
   mMaxPitch(cDefaultMaxPitch),
   mMaxRoll(cDefaultMaxRoll),
   mDamageBuffFactor(1.0f),
   mDamageTakenBuffFactor(1.0f),
   mDamageBuffsByCombatValue(false),
   mJoinType(BUnitActionJoin::cJoinTypeFollow),
   mMergeType(BUnitActionJoin::cMergeTypeGround),
   mJoinRevertDamagePct(0.0f),
   mJoinVeterancyOverride(false),
   mJoinBoardAnimType(-1),
   mJoinLevels(0),
   mJoinMaxTeleportDist(0.0f),
   mStartAnimNoInterrupt(false),
   mAnimNoInterrupt(false),
   mEndAnimNoInterrupt(false),
   mEndAnimSoundCue(cInvalidCueIndex),
   mMinIdleDuration(0),
   mBeam(false),
   mBeamCollideWithUnits(false),
   mBeamCollideWithTerrain(false),
   mTargetAir(false),
   mAttackWaitTimerEnabled(true),
   mFindBetterAction(false),
   mPersistBetweenOpps(false),
   mShockwaveAction(false),   
   mAllowReinforce(false),
   mOverrun(false),
   mClearTacticState(false),
   mShockwaveDuration(0),
   mAutoRepairIdleTime(0),
   mAutoRepairSearchDistance(0),
   mAutoRepairThreshold(1.0f),
   mSquadMode(-1),
   mNewSquadMode(-1),
   mNewTacticState(-1),
   mDuration(0),
   mDurationSpread(0),
   mAttackRunDelayMin(0.0f),
   mAttackRunDelayMax(0.0f),
   mDetonateWhenInRange(false),
   mPhysicsDetonationThreshold(0),
   mVelocityScalar(1.0f),
   mParticleEffectName(),
   mParticleEffectId(-1),
   mProtoObject(-1),
   mBone(""),
   mCount(0),
   mReviveDelay(0.0f),
   mHibernateDelay(0.0f),
   mReviveRate(0.0f),
   mMaxNumUnitsPerformAction(-1),
   mDetonateThrowHorizontalMax(1.0f),
   mDetonateThrowVerticalMax(1.0f),
   mDamageCharge(0.0f),
   mSupportAnimType(-1),
   mLowDetailAnimType(-1),
   mLowDetailAnimThreshold(cMaximumLong),
   mDetonateFromPhysics(false),
   mMeleeAttack(false),
   mHealTarget(false),
   mFlagTargetOfTarget(false),
   mFlagUseTeleporter(false),
   mFlagWaitForDeflectCooldown(false),
   mFlagMultiDeflect(false),
   mFlagWaitForDodgeCooldown(false),
   mFlagDetonateOnDeath(false),
   mFlagPickupObject(false),
   mFlagPickupObjectOnKill(false),
   mFlagAlertWhenComplete(false),
   mFlagAutoJoin(false),
   mChargeOnTaken(false),
   mChargeOnDealt(false),
   mChargable(false),
   mFlagSmallArms(false),
   mFlagDoShakeOnAttackTag(false),
   mFlagHideSpawnUntilRelease(false),
   mFlagAvoidOnly(false),
   mDontAutoRestart(false)
{
}

//==============================================================================
// BProtoAction::BProtoAction
//==============================================================================
BProtoAction::BProtoAction() : 
   mWorkRate(0.0f),
   mWorkRateVariance(0.0f),
   mpTactic(NULL),
   mpStaticData(NULL),
   mAnimType(-1),
   mDamagePerAttack(0.0f),
   mMaxNumAttacksPerAnim(0),
   mStrafingTurnRate(-1.0f),
   mJoinBoardTime(0.0f),
   mIndex(-1)
{
   // Flags
   mFlagOwnStaticData=false;
   mFlagDisabled=false;
   mFlagNoAutoTarget=false;
   mFlagDieOnBuilt=false;
   mFlagTeamShare=false;
   mFlagRequiresLockdown=false;
}

//==============================================================================
// BProtoAction::BProtoAction
//==============================================================================
BProtoAction::BProtoAction(BTactic* pTactic, const BProtoAction* pBase)
{
   // Dynamic data
   mWorkRate=pBase->mWorkRate;
   mWorkRateVariance=pBase->mWorkRateVariance;
   mpTactic=pTactic;
   mAnimType=pBase->mAnimType;
   mDamagePerAttack=pBase->mDamagePerAttack;
   mMaxNumAttacksPerAnim=pBase->mMaxNumAttacksPerAnim;
   mStrafingTurnRate=pBase->mStrafingTurnRate;
   mJoinBoardTime=pBase->mJoinBoardTime;
   mIndex=pBase->mIndex;
   
   // Flags
   mFlagOwnStaticData=pBase->mFlagOwnStaticData;
   mFlagDisabled=pBase->mFlagDisabled;
   mFlagNoAutoTarget=pBase->mFlagNoAutoTarget;
   mFlagDieOnBuilt=pBase->mFlagDieOnBuilt;
   mFlagTeamShare=pBase->mFlagTeamShare;
   mFlagRequiresLockdown=pBase->mFlagRequiresLockdown;   

   // Static data
   mpStaticData=pBase->mpStaticData;
   mFlagOwnStaticData = false;
}

//==============================================================================
// BProtoAction::~BProtoAction
//==============================================================================
BProtoAction::~BProtoAction()
{
   if(mpStaticData)
   {
      if(getFlagOwnStaticData())
      {
         delete mpStaticData;
         mFlagOwnStaticData = false;
      }
      mpStaticData=NULL;
   }
}

//==============================================================================
// BProtoAction::loadProtoAction
//==============================================================================
bool BProtoAction::loadProtoAction(BXMLNode root, long id, BTactic *pData)
{
   mpStaticData=new BProtoActionStatic();
   if(!mpStaticData)
      return false;
   mFlagOwnStaticData = true;

   mpStaticData->mID = id;

   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node=root.getChild(i);
      const BPackedString& name=node.getName();

      BSimString tempStr;
      if(name == "Name")
         node.getText(mpStaticData->mName);
      else if(name == "ActionType")
      {
         bool melee = false;
         mpStaticData->mActionType = gDatabase.getActionTypeByName(node.getTextPtr(tempStr), &melee);
         if(mpStaticData->mActionType == BAction::cActionTypeInvalid)
         {
            gConsoleOutput.output(cMsgError, "Invalid action type '%s' specified: Line %d of %s", node.getTextPtr(tempStr), root.getReader()->getLineNumber(), root.getReader()->getFilename().getPtr());
            return false;
         }
         mpStaticData->mMeleeAttack = melee;
      }
      else if (name == "ProjectileSpread")
      {
         float spread = 0.0f;
         node.getTextAsFloat(spread);
         mpStaticData->mProjectileSpread = spread;
      }
      else if (name == "StopAttackingWhenAmmoDepleted")
      {
         mpStaticData->mStopAttackingWhenAmmoDepleted = true;
      }
      else if (name == "DontLoopAttackAnim")
      {
         mpStaticData->mDontLoopAttackAnim = true;
      }
      else if (name == "DontAutoRestart")
      {
         mpStaticData->mDontAutoRestart = true;
      }
      else if (name == "DontCheckOrientTolerance")
      {
         mpStaticData->mDontCheckOrientTolerance = true;
      }
      else if (name == "KillSelfOnAttack")
         mpStaticData->mKillSelfOnAttack=true;
      else if (name == "MeleeRange")
         mpStaticData->mMeleeRange=true;
      else if (name == "InstantAttack")
         mpStaticData->mInstantAttack=true;
      else if (name == "Infection")
         mpStaticData->mInfection=true;
      else if(name == "CanOrientOwner")
      {
         if(node.compareText("false") == 0)
         {
            mpStaticData->mCanOrientOwner=false;
         }
      }
      else if (name == "MainAttack")
      {
         mpStaticData->mMainAttack=true;
      }	  
      else if (name == "Stationary")
      {
         mpStaticData->mStationary=true;
      }	  
      else if(name == "SquadType")
      {
         mpStaticData->mSquadType = gDatabase.getProtoSquad(node.getTextPtr(tempStr));
      }
      else if(name == "Weapon")
      {
         if(pData)
         {
            mpStaticData->mWeaponID = pData->getWeaponID(node.getTextPtr(tempStr));
            if(pData->getNumberWeapons()==0 && mpStaticData->mWeaponID!=-1)
               mpStaticData->mWeaponID=-1;
         }
      }      
      else if(name == "LinkedAction")
      {
         if(pData)
         {
            mpStaticData->mLinkedActionID = pData->getProtoActionID(node.getTextPtr(tempStr));
         }
      }      
      else if(name == "Shockwave")
      {
         mpStaticData->mShockwaveAction = true;
         float duration = 0.0f;
         if (node.getAttribValueAsFloat("duration", duration))
            mpStaticData->mShockwaveDuration = (DWORD) (duration * 1000.0f);
      }
      else if(name == "SquadMode")
      {
         mpStaticData->mSquadMode = gDatabase.getSquadMode(node.getTextPtr(tempStr));
      }
      else if(name == "NewSquadMode")
      {
         mpStaticData->mNewSquadMode = gDatabase.getSquadMode(node.getTextPtr(tempStr));
      }
      else if(name == "NewTacticState")
      {
         if (pData)
            mpStaticData->mNewTacticState = pData->getTacticStateID(node.getTextPtr(tempStr));
      }
      else if(name == "Anim")
      {         
         bool noInterrupt = false;
         mAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));
         node.getAttribValueAsBool("noInterrupt", noInterrupt);
         mpStaticData->mAnimNoInterrupt = noInterrupt;
      }
      else if(name == "AnimLowDetail")
      {         
         mpStaticData->mLowDetailAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));         
         long actionCountThreshold = cMaximumLong;
         node.getAttribValueAsLong("actionCountThreshold", actionCountThreshold);
         mpStaticData->mLowDetailAnimThreshold = actionCountThreshold;
      }
      else if(name == "SupportAnim")
      {
         mpStaticData->mSupportAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));         
      }
      else if(name == "ReloadAnim")
      {
         mpStaticData->mReloadAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));
      }
      else if(name == "WorkRate")
      {
         node.getTextAsFloat(mWorkRate);
      }
      else if(name == "WorkRateVariance")
      {
         node.getTextAsFloat(mWorkRateVariance);
      }
      else if(name == "WorkRange")
      {
         node.getTextAsFloat(mpStaticData->mWorkRange);
      }
      else if(name == "DodgeChanceMax")
      {
         node.getTextAsFloat(mpStaticData->mDodgeChanceMax);
      }
      else if(name == "DodgeChanceMin")
      {
         node.getTextAsFloat(mpStaticData->mDodgeChanceMin);
      }
      else if(name == "DodgeMaxAngle")
      {
         // Convert to radians
         float degrees = 180.0f;
         node.getTextAsFloat(degrees);
         mpStaticData->mDodgeMaxAngle = Math::Clamp(degrees * cRadiansPerDegree, 0.0f, cPi);
      }
      else if(name == "DodgeCooldown")
      {
         float duration = 0.0f;
         node.getTextAsFloat(duration);
         if (duration >= 0.0f)
            mpStaticData->mDodgeCooldown = static_cast<DWORD>(duration * 1000);
      }
      else if(name == "DodgePhysicsImpulse")
      {
         node.getTextAsFloat(mpStaticData->mDodgePhysicsImpulse);
      }
      else if(name == "WaitForDeflectCooldown")
      {
         mpStaticData->mFlagWaitForDeflectCooldown = true;
      }
      else if (name == "MultiDeflect")
      {
         mpStaticData->mFlagMultiDeflect = true;
      }
      else if (name == "SmallArms")
      {
         mpStaticData->mFlagSmallArms = true;
      }
      else if (name == "DoShakeOnAttackTag")
      {
         mpStaticData->mFlagDoShakeOnAttackTag = true;
      }
      else if (name == "HideSpawnUntilRelease")
      {
         mpStaticData->mFlagHideSpawnUntilRelease = true;
      }
      else if (name == "AvoidOnly")
      {
         mpStaticData->mFlagAvoidOnly = true;
      }
      else if(name == "DeflectChanceMax")
      {
         node.getTextAsFloat(mpStaticData->mDeflectChanceMax);
      }
      else if(name == "DeflectChanceMin")
      {
         node.getTextAsFloat(mpStaticData->mDeflectChanceMin);
      }
      else if(name == "DeflectMaxAngle")
      {
         // Convert to radians
         float degrees = 180.0f;
         node.getTextAsFloat(degrees);
         mpStaticData->mDeflectMaxAngle = Math::Clamp(degrees * cRadiansPerDegree, 0.0f, cPi);
      }
      else if(name == "DeflectCooldown")
      {
         float duration = 0.0f;
         node.getTextAsFloat(duration);
         if (duration >= 0.0f)
            mpStaticData->mDeflectCooldown = static_cast<DWORD>(duration * 1000);
      }
      else if(name == "DeflectMaxDmg")
      {
         node.getTextAsFloat(mpStaticData->mDeflectMaxDmg);
      }
      else if(name == "DeflectTimeout")
      {
         node.getTextAsFloat(mpStaticData->mDeflectTimeout);
      }
      else if(name == "WaitForDodgeCooldown")
      {
         mpStaticData->mFlagWaitForDodgeCooldown = true;
      }
      else if(name == "Strafing")
      {
         node.getAttribValueAsFloat("maxDistance", mpStaticData->mStrafingMaxDistance);
         node.getAttribValueAsAngle("turnRate", mStrafingTurnRate);
         node.getAttribValueAsFloat("trackingSpeed", mpStaticData->mStrafingTrackingSpeed);
         node.getAttribValueAsFloat("jitter", mpStaticData->mStrafingJitter);
         mpStaticData->mStrafing = true;

         BASSERT(mpStaticData->mStrafingMaxDistance > 0.0f);
      }
      else if (name == "HoverAltitudeOffset")
      {
         node.getTextAsFloat(mpStaticData->mHoverAltitudeOffset);
      }
      else if (name == "MaxTgtDepressionAngle")
      {
         node.getTextAsFloat(mpStaticData->mMaxTgtDepressionAngle);
      }
      else if (name == "MaxPitch")
      {
         node.getTextAsFloat(mpStaticData->mMaxPitch);
      }
      else if (name == "MaxRoll")
      {
         node.getTextAsFloat(mpStaticData->mMaxRoll);
      }
      else if (name == "DamageModifiers")
      {
         node.getAttribValueAsFloat("damage", mpStaticData->mDamageBuffFactor);
         node.getAttribValueAsFloat("damageTaken", mpStaticData->mDamageTakenBuffFactor);
         bool temp = false;
         if (node.getAttribValueAsBool("byCombatValue", temp))
            mpStaticData->mDamageBuffsByCombatValue = temp;
      }
      else if (name == "JoinType")
      {
         node.getText(tempStr);
         if (tempStr == "Follow")
            mpStaticData->mJoinType = BUnitActionJoin::cJoinTypeFollow;
         else if (tempStr == "Merge")
            mpStaticData->mJoinType = BUnitActionJoin::cJoinTypeMerge;
         else if (tempStr == "Board")
            mpStaticData->mJoinType = BUnitActionJoin::cJoinTypeBoard;
         else if (tempStr == "FollowAttack")
            mpStaticData->mJoinType = BUnitActionJoin::cJoinTypeFollowAttack;

         // Other join data
         node.getAttribValueAsFloat("revertDamagePct", mpStaticData->mJoinRevertDamagePct);
         bool temp = false;
         if (node.getAttribValueAsBool("veterancyOverride", temp))
            mpStaticData->mJoinVeterancyOverride = temp;
         node.getAttribValueAsFloat("boardTime", mJoinBoardTime);
         node.getAttribValueAsString("boardAnim", tempStr);
         node.getAttribValueAsFloat("unjoinMaxDist", mpStaticData->mJoinMaxTeleportDist);
         mpStaticData->mJoinBoardAnimType = gVisualManager.getAnimType(tempStr);
         node.getAttribValueAsInt("levels", mpStaticData->mJoinLevels);
      }
      else if (name == "ParticleEffect")
      {
         /*
         node.getText(tempStr);
         gParticleGateway.getOrCreateData(tempStr, mpStaticData->mParticleEffectId);
         */
         node.getText(mpStaticData->mParticleEffectName);
      }
      else if (name == "MergeType")
      {
         node.getText(tempStr);
         if (tempStr == "Air")
            mpStaticData->mMergeType = BUnitActionJoin::cMergeTypeAir;
         if (tempStr == "Ground")
            mpStaticData->mMergeType = BUnitActionJoin::cMergeTypeGround;
      }
      else if(name == "StartAnim")
      {
         bool noInterrupt = false;
         mpStaticData->mStartAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));
         node.getAttribValueAsBool("noInterrupt", noInterrupt);
         mpStaticData->mStartAnimNoInterrupt = noInterrupt;
      }
      else if(name == "EndAnim")
      {
         bool noInterrupt = false;
         mpStaticData->mEndAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));
         node.getAttribValueAsBool("noInterrupt", noInterrupt);
         mpStaticData->mEndAnimNoInterrupt = noInterrupt;

         node.getAttribValueAsString("SoundCue", tempStr);
         mpStaticData->mEndAnimSoundCue = gSoundManager.getCueIndex(tempStr);
      }
      else if (name == "MinIdleDuration")
      {
         float minIdleDuration;
         node.getTextAsFloat(minIdleDuration);
         BASSERT(minIdleDuration >= 0.0f);
         mpStaticData->mMinIdleDuration = static_cast<DWORD>(1000.0f * minIdleDuration);
      }
      else if(name == "ClearTacticState")
      {
         mpStaticData->mClearTacticState = true;
      }
      else if(name == "Beam")
      {
         mpStaticData->mBeam = true;
         bool collide = false;
         node.getAttribValueAsBool("collideWithUnits", collide);
         mpStaticData->mBeamCollideWithUnits = collide;
         collide = false;
         node.getAttribValueAsBool("collideWithTerrain", collide);
         mpStaticData->mBeamCollideWithTerrain = collide;
      }
      else if(name == "TargetAir")
      {
         mpStaticData->mTargetAir = true;
      }
      else if(name == "Resource")
      {
         mpStaticData->mResourceType = gDatabase.getResource(node.getTextPtr(tempStr));
      }
      else if(name == "Default")
      {
         if(node.compareText("true") == 0)
         {
            if(pData)
               pData->setDefaultActionID(id);
         }            
      }
      else if (name == "NoAutoTarget")
      {
         mFlagNoAutoTarget = true;
      }
      else if (name == "DieOnBuilt")
      {
         mFlagDieOnBuilt = true;
      }
      else if (name == "TeamShare")
      {
         mFlagTeamShare = true;
      }
      else if (name == "ExposedAction")
      {
         int index = -1;
         if (node.getTextAsInt(index))
         {
            mpStaticData->mExposedActionIndex = index;
         }
         else
         {
            mpStaticData->mExposedActionIndex = -1;
         }
      }
      else if (name == "TrampleAttack")
         pData->setTrampleActionID(id);
      else if (name == "SlaveAttackAction")
      {
         mpStaticData->mSlaveAttackActionID = pData->getProtoActionID(node.getTextPtr(tempStr));
      }
      else if (name == "BaseDPSWeapon")
      {
         mpStaticData->mBaseDPSWeaponID = pData->getWeaponID(node.getTextPtr(tempStr));
      }
      else if (name == "DisableAttackWaitTimer")
      {
         mpStaticData->mAttackWaitTimerEnabled = false;
      }	  
      else if (name == "FindBetterAction")
      {
         mpStaticData->mFindBetterAction = true;
      }	  
      else if (name == "PersistBetweenOpps")
      {
         mpStaticData->mPersistBetweenOpps = true;
      }
      else if (name == "AllowReinforce")
      {
         mpStaticData->mAllowReinforce = true;
      }
      else if (name == "Overrun")
      {
         mpStaticData->mOverrun = true;
      }
      else if (name == "PersistentActionType")
      {
         mpStaticData->mPersistentActionType = gDatabase.getActionTypeByName(node.getTextPtr(tempStr));
         if(mpStaticData->mActionType == BAction::cActionTypeInvalid)
         {
            gConsoleOutput.output(cMsgError, "Invalid action type '%s' specified: Line %d of %s", node.getTextPtr(tempStr), root.getReader()->getLineNumber(), root.getReader()->getFilename().getPtr());
            return false;
         }

         //-- Add this action to the persistent list
         pData->addPersistenAction(id);         
      }
      else if (name == "Fatality")
      {
         BProtoObjectID targetProtoObjectID = gDatabase.getProtoObject(node.getTextPtr(tempStr));

         node.getAttribValueAsString("attacker", tempStr);
         long attackerAnimType = gVisualManager.getAnimType(tempStr);

         node.getAttribValueAsString("target", tempStr);
         long targetAnimType = gVisualManager.getAnimType(tempStr);

         DWORD cooldownTime = 0;
         float cooldownTimeFloat = 0.0f;
         if (node.getAttribValueAsFloat("cooldown", cooldownTimeFloat))
         {
            cooldownTime = static_cast<DWORD>(cooldownTimeFloat * 1000.0f);
         }

         float offsetToleranceOverride = gDatabase.getFatalityPositionOffsetTolerance();
         node.getAttribValueAsFloat("offsetTolerance", offsetToleranceOverride);
         
         float orientationOffsetOverride = gDatabase.getFatalityOrientationOffsetTolerance();
         if (node.getAttribValueAsFloat("orientationOffset", orientationOffsetOverride))
         {
            orientationOffsetOverride = XMConvertToRadians(orientationOffsetOverride);
         }

         bool toBoneRelative = false;
         node.getAttribValueAsBool("toBoneRelative", toBoneRelative);

         bool transitionBeforeAnimating = false;
         node.getAttribValueAsBool("transitionBeforeAnimating", transitionBeforeAnimating);

         bool aircraftTransition = false;
         float airTransitionHeight = 5.0f;
         float airTransitionApexPercent = 0.67f;
         node.getAttribValueAsBool("air", aircraftTransition);
         if (aircraftTransition)
         {
            node.getAttribValueAsFloat("airHeight", airTransitionHeight);
            node.getAttribValueAsFloat("airApexPercent", airTransitionApexPercent);
         }

         DWORD transitionDelay = 0;
         float transitionDelayFloat = 0.0f;
         if (node.getAttribValueAsFloat("transitionDelay", transitionDelayFloat))
            transitionDelay = static_cast<DWORD>(transitionDelayFloat * 1000.0f);

         DWORD transitionTime = 0;
         float transitionTimeFloat = 0.0f;
         if (node.getAttribValueAsFloat("transitionTime", transitionTimeFloat))
            transitionTime = static_cast<DWORD>(transitionTimeFloat * 1000.0f);

         bool noMoveFlag = false;
         node.getAttribValueAsBool("noMove", noMoveFlag);

         bool lastUnitOnly = false;
         node.getAttribValueAsBool("lastUnitOnly", lastUnitOnly);

         bool attackerMoveFlag = false;
         bool animateTarget = true;
         bool killTarget = true;
         bool boarding = false;
         long attackerEndAnimType = -1;
         long targetEndAnimType = -1;
         node.getAttribValueAsBool("boarding", boarding);
         if (boarding)
         {
            attackerMoveFlag = true;
            animateTarget = false;
            killTarget = false;

            node.getAttribValueAsString("attackerEnd", tempStr);
            attackerEndAnimType = gVisualManager.getAnimType(tempStr);
            if (attackerEndAnimType == cAnimTypeIdle) // If this came out as cAnimTypeIdle it is invalid
               attackerEndAnimType = -1;

            node.getAttribValueAsString("targetEnd", tempStr);
            targetEndAnimType = gVisualManager.getAnimType(tempStr);
            if (targetEndAnimType == cAnimTypeIdle) // If this came out as cAnimTypeIdle it is invalid
               targetEndAnimType = -1;
         }

         if ((targetProtoObjectID != cInvalidProtoID) && (attackerAnimType != -1) && (targetAnimType != -1))
         {
            // Add the new fatality to the list
            BFatality fatality;
            fatality.mTargetProtoObjectID = targetProtoObjectID;
            fatality.mAttackerAnimType = attackerAnimType;
            fatality.mAttackerEndAnimType = attackerEndAnimType;
            fatality.mTargetEndAnimType = targetEndAnimType;
            fatality.mTargetAnimType = targetAnimType;
            fatality.mCooldownTime = cooldownTime;
            fatality.mOffsetToleranceOverride = offsetToleranceOverride;
            fatality.mOrientationOffsetOverride = orientationOffsetOverride;
            fatality.mAirTransitionApexPercent = airTransitionApexPercent;
            fatality.mAirTransitionHeight = airTransitionHeight;
            if (noMoveFlag)
               fatality.mMoveType = cFatalityNoMove;
            else if (attackerMoveFlag)
               fatality.mMoveType = cFatalityAttackerPosOrient;
            else
               fatality.mMoveType = cFatalityAttackerPosTargetOrient;
            fatality.mAnimateTarget = animateTarget;
            fatality.mKillTarget = killTarget;
            fatality.mBoarding = boarding;
            fatality.mToBoneRelative = toBoneRelative;
            fatality.mTransitionBeforeAnimating = transitionBeforeAnimating;
            fatality.mAircraftTransition = aircraftTransition;
            fatality.mTransitionTimeDWORD = transitionTime;
            fatality.mTransitionDelayDWORD = transitionDelay;
            fatality.mpProtoAction = this;
            fatality.mIndex = (uint8)mpStaticData->mFatalityArray.size();
            fatality.mLastUnitOnly = lastUnitOnly;
            mpStaticData->mFatalityArray.add(fatality);
         }
      }
      else if (name == "StartDisabled")
      {
         mFlagDisabled = true;
      }
      else if(name == "RequiresLockdown")
      {
         mFlagRequiresLockdown = true;
      }
      else if (name == "Duration")
      {
         node.getTextAsFloat(mpStaticData->mDuration);
         node.getAttribValueAsFloat("DurationSpread", mpStaticData->mDurationSpread);
      }
      else if (name == "DetonateThrow")
      {
         node.getAttribValueAsFloat("HorizontalMax", mpStaticData->mDetonateThrowHorizontalMax);
         node.getAttribValueAsFloat("VerticalMax", mpStaticData->mDetonateThrowVerticalMax);
      }
      else if (name == "DetonateFromPhysics")
      {
         mpStaticData->mDetonateFromPhysics = true;
         node.getAttribValueAsFloat("PhysicsDetonationThreshold", mpStaticData->mPhysicsDetonationThreshold);
      }
      else if (name == "DetonateWhenInRange")
      {
         mpStaticData->mDetonateWhenInRange = true;
      }
      else if (name == "AutoRepair")
      {
         float fval1, fval2;
         DWORD dwval;
         node.getAttribValueAsDWORD("AutoRepairIdleTime", dwval);
         mpStaticData->mAutoRepairIdleTime = dwval;
         node.getAttribValueAsFloat("AutoRepairThreshold", fval1);
         mpStaticData->mAutoRepairThreshold = fval1;
         node.getAttribValueAsFloat("AutoRepairSearchDistance", fval2);
         mpStaticData->mAutoRepairSearchDistance = fval2;
      }
      else if (name == "InvalidTarget")
      {
         const char* pString = node.getTextPtr(tempStr);
         BProtoObjectID poid = gDatabase.getProtoObject(pString);
         mpStaticData->mInvalidTargetsArray.add(poid);
      }
      else if (name == "HealTarget")
      {
         mpStaticData->mHealTarget = true;
      }
      else if (name == "TargetOfTarget")
      {
         mpStaticData->mFlagTargetOfTarget = true;
      }
      else if (name == "UseTeleporter")
      {
         mpStaticData->mFlagUseTeleporter = true;
      }
      else if (name == "VelocityScalar")
      {
         node.getTextAsFloat(mpStaticData->mVelocityScalar);
      }
      else if (name == "DetonateOnDeath")
      {
         mpStaticData->mFlagDetonateOnDeath = true;
      }
      else if (name == "PickupObject")
      {
         mpStaticData->mFlagPickupObject = true;
      }
      else if (name == "PickupObjectOnKill")
      {
         mpStaticData->mFlagPickupObjectOnKill = true;
      }
      else if (name == "AttackRunDelayMin")
      {
         float delay = 0.0f;
         node.getTextAsFloat(delay);
         mpStaticData->mAttackRunDelayMin = delay;
      }
      else if (name == "AttackRunDelayMax")
      {
         float delay = 0.0f;
         node.getTextAsFloat(delay);
         mpStaticData->mAttackRunDelayMax = delay;
      }
      else if (name == "AlertWhenComplete")
      {
         mpStaticData->mFlagAlertWhenComplete = true;
      }
      else if (name == "AutoJoin")
      {
         mpStaticData->mFlagAutoJoin = true;
      }
      else if(name == "ReviveDelay")
      {
         node.getTextAsFloat(mpStaticData->mReviveDelay);
      }
      else if(name == "HibernateReviveDelay")
      {
         node.getTextAsFloat(mpStaticData->mHibernateDelay);
      }
      else if(name == "ReviveRate")
      {
         node.getTextAsFloat(mpStaticData->mReviveRate);
      }
      else if(name == "ProtoObject")
      {
         if (node.getAttribute("Squad"))
            mpStaticData->mProtoObject = gDatabase.getProtoSquad(node.getTextPtr(tempStr));
         else
            mpStaticData->mProtoObject = gDatabase.getProtoObject(node.getTextPtr(tempStr));

         node.getAttribValueAsString("bone", mpStaticData->mBone);
      }
      else if (name == "Count")
      {
         node.getTextAsLong(mpStaticData->mCount);
      }
      else if(name == "MaxNumUnitsPerformAction")
      {         
         node.getTextAsInt(mpStaticData->mMaxNumUnitsPerformAction);
         BASSERT(mpStaticData->mMaxNumUnitsPerformAction >= 1);
      }
      else if (name=="DamageCharge")
      {
         node.getTextAsFloat(mpStaticData->mDamageCharge);
      }
      else if(name == "ChargeOnTaken")
         mpStaticData->mChargeOnTaken = true;
      else if(name == "ChargeOnDealt")
         mpStaticData->mChargeOnDealt = true;
      else if(name == "Chargable")
         mpStaticData->mChargable = true;
   }

   mpTactic = pData;

   return true;
}

//==============================================================================
// BProtoAction::postloadProtoAction
//==============================================================================
void BProtoAction::postloadProtoAction(const BProtoObject* pObj)
{
   // If the fatality assets were already loaded, we just need to copy the
   // asset indices
   if (gFatalityManager.areFatalityAssetsLoaded())
   {
      long numFatalities = mpStaticData->mFatalityArray.getNumber();
      if (numFatalities > 0)
      {
         for (long i = 0; i < numFatalities; i++)
         {
            // Clear the array
            BFatality& fatality = mpStaticData->mFatalityArray.get(i);

            gFatalityManager.addFatalityAssets(fatality.mFatalityAssetArray, pObj->getID(), fatality.mTargetProtoObjectID, getIndex());
         }
      }

      return;
   }

   // Otherwise, compute the data below...

   // This is bad.  The tactic files are force loading protovisuals directly throught the BVisualManager::getProtoVisual call.  Changing 
   // the getProtoVisual calls to not force load when running with archives.  This is very bad on non-archive builds too since a lot of
   // assets get loaded that don't need to if they aren't in the map.  It needs to be rearchitected IMO.  SAT
   
   bool forceLoad = !gArchiveManager.getArchivesEnabled();

   long numFatalities = mpStaticData->mFatalityArray.getNumber();
   if (numFatalities > 0)
   {
      // Iterate through fatality entries and clear the array
      for (long i = 0; i < numFatalities; i++)
      {
         BFatality& fatality = mpStaticData->mFatalityArray.get(i);
         fatality.mFatalityAssetArray.resize(0);
      }

      // Get attacker's proto visual model
      long attackerProtoVisualID = pObj->getProtoVisualIndex();
      BProtoVisual* pAttackerProtoVisual = gVisualManager.getProtoVisual(attackerProtoVisualID, forceLoad);
      BASSERT(pAttackerProtoVisual);
      
      if(!pAttackerProtoVisual->areAllAssetsLoaded())
         return;

      BProtoVisualModel* pAttackerProtoVisualModel = pAttackerProtoVisual->mModels[0];
      BASSERT(pAttackerProtoVisualModel);

      // Iterate through fatality entries
      for (long i = 0; i < numFatalities; i++)
      {
         BFatality& fatality = mpStaticData->mFatalityArray.get(i);

         // Get target's proto visual model
//-- FIXING PREFIX BUG ID 4052
         const BProtoObject* pTargetProtoObject = gDatabase.getGenericProtoObject(fatality.mTargetProtoObjectID);
//--
         BASSERT(pTargetProtoObject);

         long targetProtoVisualID = pTargetProtoObject->getProtoVisualIndex();
         BProtoVisual* pTargetProtoVisual = gVisualManager.getProtoVisual(targetProtoVisualID, forceLoad);
         BASSERT(pTargetProtoVisual);

         if(!pTargetProtoVisual || !pTargetProtoVisual->areAllAssetsLoaded())
            continue;


         BASSERT(pTargetProtoVisual);
         BProtoVisualModel* pTargetProtoVisualModel = pTargetProtoVisual->mModels[0];
         BASSERT(pTargetProtoVisualModel);
         if (!pTargetProtoVisualModel)
            continue;


         // Get attacker's proto visual anim
         long numAnims = pAttackerProtoVisualModel->mAnims.getNumber();
         for (long anim = 0; anim < numAnims; anim++)
         {
            BProtoVisualAnim* pAttackerProtoVisualAnim = &pAttackerProtoVisualModel->mAnims.get(anim);
            BDEBUG_ASSERT(pAttackerProtoVisualAnim);
            if (pAttackerProtoVisualAnim->mAnimType == fatality.mAttackerAnimType)
            {
               // Get target's proto visual anim
               BProtoVisualAnim* pTargetProtoVisualAnim = NULL;
               long targetNumAnims = pTargetProtoVisualModel->mAnims.getNumber();
               for (long targetAnim = 0; targetAnim < targetNumAnims; targetAnim++)
               {
                  BProtoVisualAnim& targetProtoVisualAnim = pTargetProtoVisualModel->mAnims.get(targetAnim);
                  if (targetProtoVisualAnim.mAnimType == fatality.mTargetAnimType)
                  {
                     pTargetProtoVisualAnim = &targetProtoVisualAnim;
                     break;
                  }
               }
               if (!pTargetProtoVisualAnim)
               {
                  // Output warning message and continue
                  BSimString errorMsg;
                  errorMsg.format(B("Fatality target [%s] missing animation [%s]."), pTargetProtoObject->getName().getPtr(), gVisualManager.getAnimName(fatality.mTargetAnimType));
                  BASSERTM(0, errorMsg.getPtr());

                  continue;
               }

               // Iterate through attacker's assets
               long numAssets = pAttackerProtoVisualAnim->mAssets.getNumber();
               long numTargetAssets = pTargetProtoVisualAnim->mAssets.getNumber();
               if (fatality.mAnimateTarget && (numTargetAssets < numAssets))
               {
                  BSimString errorMsg;
                  errorMsg.format(B("Fatality attacker [%s] has more [%s] anims than target [%s] [%s]. <%d vs. %d>"),
                     pObj->getName().getPtr(),
                     gVisualManager.getAnimName(pAttackerProtoVisualAnim->mAnimType),
                     pTargetProtoObject->getName().getPtr(),
                     gVisualManager.getAnimName(pTargetProtoVisualAnim->mAnimType),
                     numAssets, numTargetAssets);
                  BASSERTM(0, errorMsg.getPtr());
               }

               for (long asset = 0; asset < numAssets; asset++)
               {
                  // Get attacker's granny animation
//-- FIXING PREFIX BUG ID 4048
                  const BProtoVisualAsset& attackerProtoVisualAsset = pAttackerProtoVisualAnim->mAssets.get(asset);
//--
//-- FIXING PREFIX BUG ID 4049
                  const BGrannyAnimation* pAttackerAnim = gGrannyManager.getAnimation(attackerProtoVisualAsset.mAssetIndex, true);
//--
                  if (!pAttackerAnim)
                  {
                     BSimString errorMsg;
                     errorMsg.format(B("Fatality attacker [%s] missing granny animation [%s]."), pObj->getName().getPtr(), attackerProtoVisualAsset.mAssetName.getPtr());
                     BASSERTM(0, errorMsg.getPtr());
                     continue;
                  }

                  // Get target's granny animation
                  long targetAsset = Math::Min(asset, numTargetAssets - 1);
//-- FIXING PREFIX BUG ID 4050
                  const BProtoVisualAsset& targetProtoVisualAsset = pTargetProtoVisualAnim->mAssets.get(targetAsset);
//--
//-- FIXING PREFIX BUG ID 4051
                  const BGrannyAnimation* pTargetAnim = gGrannyManager.getAnimation(targetProtoVisualAsset.mAssetIndex, true);
//--
                  if (!pTargetAnim)
                  {
                     BSimString errorMsg;
                     errorMsg.format(B("Fatality target [%s] missing granny animation [%s]."), pTargetProtoObject->getName().getPtr(), targetProtoVisualAsset.mAssetName.getPtr());
                     BASSERTM(0, errorMsg.getPtr());
                     continue;
                  }
                  
                  // Paranoia
                  BASSERT(attackerProtoVisualAsset.mWeight == targetProtoVisualAsset.mWeight);

                  // Calculate the target's offset relative to the attacker
                  granny_transform attackerAnimTransform = pAttackerAnim->getGrannyFileInfo()->Animations[0]->TrackGroups[0]->InitialPlacement;
                  BMatrix attackerAnimMatrix;
                  GrannyBuildCompositeTransform4x4(&attackerAnimTransform, (granny_real32*)&attackerAnimMatrix);
                  BMatrix attackerAnimMatrixInvert(attackerAnimMatrix);
                  attackerAnimMatrixInvert.invert();

                  granny_transform targetAnimTransform = pTargetAnim->getGrannyFileInfo()->Animations[0]->TrackGroups[0]->InitialPlacement;
                  BMatrix targetAnimMatrix;
                  GrannyBuildCompositeTransform4x4(&targetAnimTransform, (granny_real32*)&targetAnimMatrix);
                  BMatrix targetAnimMatrixInvert(targetAnimMatrix);
                  targetAnimMatrixInvert.invert();

                  // O = T * A^-1
                  BMatrix targetOffsetMatrix;
                  targetOffsetMatrix.mult(targetAnimMatrix, attackerAnimMatrixInvert);

                  // Calculate overall area
                  BVector targetCorner1;
                  targetOffsetMatrix.transformVectorAsPoint(targetProtoVisualAsset.mMinCorner, targetCorner1);
                  BVector targetCorner2;
                  targetOffsetMatrix.transformVectorAsPoint(targetProtoVisualAsset.mMaxCorner, targetCorner2);
                  BVector minCorner = XMVectorMin(attackerProtoVisualAsset.mMinCorner, XMVectorMin(targetCorner1, targetCorner2));
                  BVector maxCorner = XMVectorMax(attackerProtoVisualAsset.mMaxCorner, XMVectorMax(targetCorner1, targetCorner2));

                  // Add fatality asset
                  long fatalityAssetIndex = gFatalityManager.registerFatalityAsset(targetOffsetMatrix.getRow(3), targetOffsetMatrix, 
                     attackerProtoVisualAsset.mWeight, asset, targetAsset);
                  fatality.mFatalityAssetArray.add(fatalityAssetIndex);
               }

               break;
            }
         }
      }
   }


   // Load needed pfx files
   //



   if(!mpStaticData->mParticleEffectName.isEmpty())
   {
      gParticleGateway.getOrCreateData(mpStaticData->mParticleEffectName, mpStaticData->mParticleEffectId);
   }
}

//==============================================================================
// BProtoAction::getFatality
//==============================================================================
BFatality* BProtoAction::getFatality(const BUnit* pAttackerUnit, const BUnit* pTargetUnit, uint& outAssetIndex) const
{
   long numFatalities = mpStaticData->mFatalityArray.getNumber();
   if (numFatalities)
   {
      // Make sure our units aren't already doing a fatality
      if (pAttackerUnit->getFlagDoingFatality() || pTargetUnit->getFlagDoingFatality())
         return NULL;

      // Ask the manager if it's ok to run a fatality at this position
      BVector attackerPosition = pAttackerUnit->getPosition();
      BVector targetPosition = pTargetUnit->getPosition();
      BVector fatalityPosition;
      fatalityPosition.assignSum(attackerPosition, targetPosition);
      fatalityPosition.scale(0.5f);
      if (!gFatalityManager.isPositionValid(fatalityPosition))
         return NULL;

      // Iterate through fatalities
      BProtoObjectID targetProtoID = pTargetUnit->getProtoID();
      for (long fatalityIndex = 0; fatalityIndex < numFatalities; fatalityIndex++)
      {
         // Look for a target protoID match
         BFatality& fatality = mpStaticData->mFatalityArray.get(fatalityIndex);
         if (fatality.mTargetProtoObjectID == targetProtoID)
         {
            // If this fatality can only be used on the final unit in a squad, and the target
            // squad has more than 1 unit, skip it.
            BSquad* pTargetSquad = pTargetUnit->getParentSquad();
            if (fatality.mLastUnitOnly && pTargetSquad && (pTargetSquad->getNumberChildren() > 1))
               continue;

            BSmallDynamicSimArray<long> validAssets;
            long weightTotal = 0;

            // Calculate the target's offset relative to the attacker

            // For normal fatalities where the target orients to the attacker, get
            // the current attacker matrix for computing offsets.
            // For boarding fatalities where the attacker orients to the target, the
            // offset orientation only needs to consider the relative angle that the
            // attacker is positioned off of the target
            BMatrix attackerMatrix;
            if (fatality.mMoveType == cFatalityAttackerPosTargetOrient)
               pAttackerUnit->getWorldMatrix(attackerMatrix);
            else
            {
               BVector fwd, right;
               fwd = pTargetUnit->getPosition() - pAttackerUnit->getPosition();
               fwd.y = 0.0f;
               if (!fwd.safeNormalize())
                  fwd = pAttackerUnit->getForward();
               right = cYAxisVector.cross(fwd);

               attackerMatrix.makeOrient(fwd, cYAxisVector, right);
               attackerMatrix.setTranslation(pAttackerUnit->getPosition());
            }

            BMatrix attackerMatrixInvert(attackerMatrix);
            attackerMatrixInvert.invert();

            BMatrix targetMatrix;
            pTargetUnit->getWorldMatrix(targetMatrix);

            // Use boarding bone matrix for targetMatrix if the fatality is "toBoneRelative"
            if (fatality.mBoarding && fatality.mToBoneRelative)
            {
//-- FIXING PREFIX BUG ID 4053
               const BVisual* pTargetVisual = pTargetUnit->getVisual();
//--
               if (pTargetVisual)
               {
                  long boardPointHandle = pTargetVisual->getPointHandle(cActionAnimationTrack, cVisualPointBoard);
                  const BProtoVisualPoint* pPoint = pTargetVisual->getPointProto(cActionAnimationTrack, boardPointHandle);
                  long targetToBoneHandle = -1;
                  if (pPoint)
                     targetToBoneHandle = pPoint->mBoneHandle;

                  if (targetToBoneHandle != -1)
                  {
                     BVector tempPos;
                     BMatrix tempMtx;
                     pTargetVisual->getBone(targetToBoneHandle, &tempPos, &tempMtx, 0, &targetMatrix);
                     targetMatrix = tempMtx;
                  }
               }
            }

            // O = T * A^-1
            BMatrix offsetMatrix;
            offsetMatrix.mult(targetMatrix, attackerMatrixInvert);

            BVector positionOffset = offsetMatrix.getRow(3);
            BQuaternion orientationOffset(offsetMatrix);

            // Iterate through the assets
            long numAssets = fatality.mFatalityAssetArray.getNumber();
            validAssets.reserve(numAssets);
            for (long assetIndex = 0; assetIndex < numAssets; assetIndex++)
            {
               long weight = 0;
//-- FIXING PREFIX BUG ID 4054
               const BFatalityAsset* pAsset = gFatalityManager.getFatalityAsset(fatality.mFatalityAssetArray.get(assetIndex));
//--

               //==============================================================================
               // Test cooldown flag
               if (!fatality.mBoarding && pAsset->mFlagCooldown)
               {
                  validAssets.add(weight);
                  continue;
               }

               //==============================================================================
               // Test position offset vs. tolerance
               const float offsetDistance = pAsset->mTargetPositionOffset.xzDistance(positionOffset);
               if (offsetDistance > fatality.mOffsetToleranceOverride)
               {
                  validAssets.add(weight);
                  continue;
               }

               //==============================================================================
               // Test orientation offset vs. tolerance
               float angle;
               BVector axis;
               BQuaternion orientationOffsetOffset = pAsset->mTargetOrientationOffset * orientationOffset.inverse();
               orientationOffsetOffset.toAxisAngle(&axis, &angle);
               if (angle > cPi)
                  angle = cTwoPi - angle;
               if (angle > fatality.mOrientationOffsetOverride)
               {
                  validAssets.add(weight);
                  continue;
               }

               //==============================================================================
               // Test for obstructions
               // MPB [4/11/2008] - Removed the obstruction test as the fatalities are melee only
               // currently and being within
               /*
               // Setup collision data
               float radius = pAttackerUnit->getObstructionRadius();
               BVector pos1 = pAttackerUnit->getPosition();
               BVector pos2 = pTargetUnit->getPosition();

               // Set up ignore list - ignore everything in attacker or target squad
               static BEntityIDArray ignoreUnits;
               ignoreUnits.setNumber(0);
               BSquad* pAttackerSquad = pAttackerUnit->getParentSquad();
               if (pAttackerSquad)
               {
                  ignoreUnits.add(pAttackerSquad->getID());
                  ignoreUnits.append(pAttackerSquad->getChildList());
               }
               else
                  ignoreUnits.add(pAttackerUnit->getID());

               BSquad* pTargetSquad = pTargetUnit->getParentSquad();
               if (pTargetSquad)
               {
                  ignoreUnits.add(pTargetSquad->getID());
                  ignoreUnits.append(pTargetSquad->getChildList());
               }
               else
                  ignoreUnits.add(pTargetUnit->getID());

               long lObOptions=
                  BObstructionManager::cIsNewTypeBlockLandUnits |
                  BObstructionManager::cIsNewTypeAllCollidableUnits |
                  BObstructionManager::cIsNewTypeAllCollidableSquads;
               long lObNodeType = BObstructionManager::cObsNodeTypeAll;
               static BObstructionNodePtrArray collisionObs;
               collisionObs.setNumber(0);

               // Do the obstruction check
               BVector intersectionPoint;
               gObsManager.begin(BObstructionManager::cBeginEntity, radius, lObOptions, lObNodeType, 0, 0.0f, &ignoreUnits, false);
               gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, pos1, pos2, 
                  true, intersectionPoint, collisionObs);
               gObsManager.end();

               // Obstructions in the way
               if (collisionObs.getNumber() > 0)
               {
                  validAssets.add(weight);
                  continue;
               }
               */

               //==============================================================================
               // All tests passed - add the asset's weight
               weight = pAsset->mWeight;
               weightTotal += weight;
               validAssets.add(weight);
            }

            // If some assets meet the criteria, pick one of them
            if (weightTotal > 0)
            {
               long random = getRandRange(cSimRand, 1, weightTotal);
               long weightAccumulator = 0;

               for (long assetIndex = 0; assetIndex < numAssets; assetIndex++)
               {
                  weightAccumulator += validAssets.get(assetIndex);
                  if (weightAccumulator >= random)
                  {
                     outAssetIndex = assetIndex;
                     return &fatality;
                     //attackerAnimType = fatality.mAttackerAnimType;
                     //targetAnimType = fatality.mTargetAnimType;
                     //return gFatalityManager.getFatalityAsset(fatality.mFatalityAssetArray.get(assetIndex));
                  }
               }
            }

            return NULL;
         }
      }
   }

   return NULL;
}

//==============================================================================
//==============================================================================
BFatality* BProtoAction::getFatalityByIndex(uint index) const
{
   if (index >= mpStaticData->mFatalityArray.size())
      return NULL;
   BFatality& fatality = mpStaticData->mFatalityArray.get(index);
   return &fatality;
}

//==============================================================================
// BProtoAction::getImpactRumble
//==============================================================================
bool BProtoAction::getImpactRumble() const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon && pWeapon->mpStaticData->mpImpactRumble)
      return true;
   return false;
}

//==============================================================================
// BProtoAction::doImpactRumbleAndCameraShake
//==============================================================================
void BProtoAction::doImpactRumbleAndCameraShake(int eventType, BVector location, bool onlyIfSelected, BEntityID unitID) const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   const BWeaponStatic* pWeaponStatic = (pWeapon) ? pWeapon->mpStaticData : NULL;

   if (pWeaponStatic)
   {
      const BEntity* pEntity = gWorld->getEntity(unitID);
      float scalar = (pEntity && pEntity->getPlayerID() != gUserManager.getPrimaryUser()->getPlayerID()) ? pWeaponStatic->mShakeScalarNotLocal : 1.0f;

      // Rumble
      if (pWeaponStatic->mpImpactRumble)
      {
         // ajl fixme 12/4/07 - add support for secondary user in split screen
         if (!onlyIfSelected || gUserManager.getPrimaryUser()->getSelectionManager()->isUnitSelected(unitID))
         {
            if (gWorld->isSphereVisible(location, 1.0f))
            {
               gUI.playRumbleEvent(eventType, pWeaponStatic->mpImpactRumble->mLeftRumbleType, pWeaponStatic->mpImpactRumble->mLeftStrength * scalar, pWeaponStatic->mpImpactRumble->mRightRumbleType, pWeaponStatic->mpImpactRumble->mRightStrength * scalar, pWeaponStatic->mpImpactRumble->mDuration, false);
               gUI.playRumbleEvent(eventType, pWeaponStatic->mpImpactRumble->mLeftRumbleType, pWeaponStatic->mpImpactRumble->mLeftStrength * scalar, pWeaponStatic->mpImpactRumble->mRightRumbleType, pWeaponStatic->mpImpactRumble->mRightStrength * scalar, pWeaponStatic->mpImpactRumble->mDuration, false);
            }
         }
      }

      gUI.doCameraShake(location, pWeaponStatic->mImpactCameraShakeStrength * scalar, pWeaponStatic->mImpactCameraShakeDuration, onlyIfSelected, unitID);

      // sucks, but we can't scale the camera effect trivially, so let's just turn it off if this is not 1.0
      if (scalar >= (1.0f - cFloatCompareEpsilon))
         gUI.doCameraEffect(location, pWeaponStatic->mpImpactCameraEffect);
   }
}

//==============================================================================
// BProtoAction::getProjectileID
//==============================================================================
long BProtoAction::getProjectileID() const
{
   if(mpTactic->getNumberWeapons()==0 && mpStaticData->mWeaponID!=-1)
      return -1;
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mProjectileObjectID;
   else
      return -1;
}

//==============================================================================
// BProtoAction::getDamagePerSecond
//==============================================================================
float BProtoAction::getDamagePerSecond() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mDamagePerSecond;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getDamagePerAttack
//==============================================================================
float BProtoAction::getDamagePerAttack() const
{
   return mDamagePerAttack;
}

//==============================================================================
// BProtoAction::getMaxNumAttacksPerAnim
//==============================================================================
long BProtoAction::getMaxNumAttacksPerAnim() const
{
   return mMaxNumAttacksPerAnim;
}

//==============================================================================
// BProtoAction::getAccuracy
//==============================================================================
float BProtoAction::getAccuracy() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAccuracy;
   else
      return 1.0f;
}

//==============================================================================
// BProtoAction::getMovingAccuracy
//==============================================================================
float BProtoAction::getMovingAccuracy() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mMovingAccuracy;
   else
      return 1.0f;
}

//==============================================================================
// BProtoAction::getMaxDeviation
//==============================================================================
float BProtoAction::getMaxDeviation() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mMaxDeviation;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getMovingMaxDeviation
//==============================================================================
float BProtoAction::getMovingMaxDeviation() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mMovingMaxDeviation;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getAccuracyDistanceFactor
//==============================================================================
float BProtoAction::getAccuracyDistanceFactor() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAccuracyDistanceFactor;
   else
      return 0.5f;
}

//==============================================================================
// BProtoAction::getAccuracyDeviationFactor
//==============================================================================
float BProtoAction::getAccuracyDeviationFactor() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAccuracyDeviationFactor;
   else
      return 0.5f;
}

//==============================================================================
// BProtoAction::getAirBurstSpan
//==============================================================================
float BProtoAction::getAirBurstSpan() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAirBurstSpan;
   else
      return -1.0f;
}

//==============================================================================
// BProtoAction::getMaxVelocityLead
//==============================================================================
float BProtoAction::getMaxVelocityLead() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mMaxVelocityLead;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getWeaponType
//==============================================================================
long BProtoAction::getWeaponType() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mWeaponType;
   else      
      return -1;
}

//==============================================================================
//==============================================================================
uint BProtoAction::getVisualAmmo() const
{
   const BWeapon* pWeapon=mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return (pWeapon->mpStaticData->mVisualAmmo);
   return (0);
}

//==============================================================================
// BProtoAction::getDamageRatingOverride
//==============================================================================
bool BProtoAction::getDamageRatingOverride(long damageType, float& rating) const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
   {
      uint count=pWeapon->mpStaticData->mDamageRatingOverride.getSize();
      for (uint i=0; i<count; i++)
      {
//-- FIXING PREFIX BUG ID 4056
         const BWeaponDamageTypeRatingOverride& item=pWeapon->mpStaticData->mDamageRatingOverride[i];
//--
         if (item.mDamageType==damageType)
         {
            rating=item.mDamageRating;
            return true;
         }
      }
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BProtoAction::getHalfKillCutoffFactor(long damageType, float& halfKillCutoffFactor) const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
   {
      uint count=pWeapon->mpStaticData->mDamageRatingOverride.getSize();
      for (uint i=0; i<count; i++)
      {
//-- FIXING PREFIX BUG ID 4057
         const BWeaponDamageTypeRatingOverride& item=pWeapon->mpStaticData->mDamageRatingOverride[i];
//--
         if (item.mDamageType==damageType)
         {
            if (item.mHalfKillCutoffFactor >= 0.0f)
            {
               halfKillCutoffFactor = item.mHalfKillCutoffFactor;
               return true;
            }
         }
      }
   }
   halfKillCutoffFactor = -1.0f;
   return false;
}

//==============================================================================
// BProtoAction::getAOERadius
//==============================================================================
float BProtoAction::getAOERadius() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAOERadius;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getAOEPrimaryTargetFactor
//==============================================================================
float BProtoAction::getAOEPrimaryTargetFactor() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAOEPrimaryTargetFactor;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getAOEDistanceFactor
//==============================================================================
float BProtoAction::getAOEDistanceFactor() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAOEDistanceFactor;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getAOEDamageFactor
//==============================================================================
float BProtoAction::getAOEDamageFactor() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mAOEDamageFactor;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getFlagAOELinearDamage
//==============================================================================
bool BProtoAction::getFlagAOELinearDamage() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return (pWeapon->mFlagAOELinearDamage);
   else
      return false;
}

//==============================================================================
// BProtoAction::getFlagAOEIgnoresYAxis
//==============================================================================
bool BProtoAction::getFlagAOEIgnoresYAxis() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return (pWeapon->mFlagAOEIgnoresYAxis);
   else
      return false;
}

//==============================================================================
// BProtoAction::getDOTrate
//==============================================================================
float BProtoAction::getDOTrate() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mDOTrate;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getDOTduration
//==============================================================================
float BProtoAction::getDOTduration() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mDOTduration;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getDOTEffect
//==============================================================================
long BProtoAction::getDOTEffect(BUnit* pUnit) const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
   {
      float obs = pUnit->getObstructionRadius();
      if (obs <= gDatabase.getDotSizeSmall())
         return pWeapon->mpStaticData->mDOTEffectSmall;
      else if (obs <= gDatabase.getDotSizeMedium())
         return pWeapon->mpStaticData->mDOTEffectMedium;
      else
         return pWeapon->mpStaticData->mDOTEffectLarge;
   }
   else
      return -1;
}

//==============================================================================
// BProtoAction::getReapplyTime
//==============================================================================
float BProtoAction::getReapplyTime() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mReapplyTime;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getApplyTime
//==============================================================================
float BProtoAction::getApplyTime() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mApplyTime;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getStasis
//==============================================================================
bool BProtoAction::getFlagStasis() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mFlagStasis;
   else
      return false;
}

//==============================================================================
// BProtoAction::getStasisDrain
//==============================================================================
bool BProtoAction::getFlagStasisDrain() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mFlagStasisDrain;
   else
      return false;
}

//==============================================================================
// BProtoAction::getStasisBomb
//==============================================================================
bool BProtoAction::getFlagStasisBomb() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mFlagStasisBomb;
   else
      return false;
}

//==============================================================================
// BProtoAction::getKnockback
//==============================================================================
bool BProtoAction::getFlagKnockback() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mFlagKnockback;
   else
      return false;
}

//==============================================================================
// BProtoAction::getTentacle
//==============================================================================
bool BProtoAction::getFlagTentacle() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mFlagTentacle;
   else
      return false;
}

//==============================================================================
// BProtoAction::getDaze
//==============================================================================
bool BProtoAction::getFlagDaze() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagDaze();
   return false;
}

//==============================================================================
// BProtoAction::getFlagAOEDaze
//==============================================================================
bool BProtoAction::getFlagAOEDaze() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagAOEDaze();
   return false;
}

//==============================================================================
// BProtoAction::getDazeDuration
//==============================================================================
float BProtoAction::getDazeDuration() const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return pWeapon->getDazeDuration();

   return 0.0f;
}

//==============================================================================
// BProtoAction::getDazeTargetTypeID
//==============================================================================
long BProtoAction::getDazeTargetTypeID() const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return pWeapon->getDazeTargetTypeID();

   return -1;
}

//==============================================================================
// BProtoAction::getDazeMovementModifier
//==============================================================================
float BProtoAction::getDazeMovementModifier() const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return pWeapon->getDazeMovementModifier();

   return 0.0f;
}

//==============================================================================
// BProtoAction::getPhysicsLaunchAngleMin
//==============================================================================
float BProtoAction::getPhysicsLaunchAngleMin() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPhysicsLaunchAngleMin;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getPhysicsLaunchAngleMax
//==============================================================================
float BProtoAction::getPhysicsLaunchAngleMax() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPhysicsLaunchAngleMax;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getPhysicsLaunchAxial
//==============================================================================
bool BProtoAction::getPhysicsLaunchAxial() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagPhysicsLaunchAxial();
   else
      return false;
}

//==============================================================================
// BProtoAction::getPhysicsForceMin
//==============================================================================
float BProtoAction::getPhysicsForceMin() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPhysicsForceMin;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getPhysicsForceMax
//==============================================================================
float BProtoAction::getPhysicsForceMax() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPhysicsForceMax;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getPhysicsForceMaxAngle
//==============================================================================
float BProtoAction::getPhysicsForceMaxAngle() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPhysicsForceMaxAngle;
   else
      return 0.0f;  
}

//==============================================================================
// BProtoAction::getThrowUnits
//==============================================================================
bool BProtoAction::getThrowUnits() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagThrowUnits();
   else
      return false;
}

//==============================================================================
// BProtoAction::getThrowAliveUnits
//==============================================================================
bool BProtoAction::getThrowAliveUnits() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mFlagThrowAliveUnits;
   else
      return false;
}

//==============================================================================
// BProtoAction::getThrowDamageParts
//==============================================================================
bool BProtoAction::getThrowDamageParts() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagThrowDamageParts();
   else
      return false;
}

//==============================================================================
// BProtoAction::getFlailThrownUnits
//==============================================================================
bool BProtoAction::getFlailThrownUnits() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagFlailThrownUnits();
   else
      return false;
}

//==============================================================================
// BProtoAction::getDodgeable
//==============================================================================
bool BProtoAction::getDodgeable() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagDodgeable();
   else
      return false;
}

//==============================================================================
// BProtoAction::getDeflectable
//==============================================================================
bool BProtoAction::getDeflectable() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagDeflectable();
   else
      return false;
}

//==============================================================================
// BProtoAction::getSmallArmsDeflectable
//==============================================================================
bool BProtoAction::getSmallArmsDeflectable() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagSmallArmsDeflectable();
   else
      return false;
}

//==============================================================================
// BProtoAction::getOverridesRevive
//==============================================================================
bool BProtoAction::getOverridesRevive() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagOverridesRevive();
   return false;
}

//==============================================================================
// BProtoAction::getPullUnits
//==============================================================================
bool BProtoAction::getPullUnits() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagPullUnits();
   return false;
}

//==============================================================================
// BProtoAction::getUseDPSasDPA
//==============================================================================
bool BProtoAction::getUseDPSasDPA() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagUseDPSasDPA();
   else
      return false;
}

//==============================================================================
// BProtoAction::getCarriedObjectAsProjectileVisual
//==============================================================================
bool BProtoAction::getCarriedObjectAsProjectileVisual() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagCarriedObjectAsProjectileVisual();
   else
      return false;
}

//==============================================================================
// BProtoAction::getFlagAirBurst
//==============================================================================
bool BProtoAction::getFlagAirBurst() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return (pWeapon->getFlagAirBurst());
   else
      return false;
}

//==============================================================================
// BProtoAction::setMaxRange
//==============================================================================
void BProtoAction::setMaxRange(float val)
{
   BWeapon *pWeapon = const_cast<BWeapon*>(mpTactic->getWeapon(mpStaticData->mWeaponID));
   if(pWeapon)
      pWeapon->mMaxRange=val;
}


//==============================================================================
// BProtoAction::setDamagePerAttack
//==============================================================================
void BProtoAction::setDamagePerAttack(float val)
{
   mDamagePerAttack = val;   
}

//==============================================================================
// BProtoAction::setMaxNumAttacksPerAnim
//==============================================================================
void BProtoAction::setMaxNumAttacksPerAnim(long val)
{
   mMaxNumAttacksPerAnim = max(mMaxNumAttacksPerAnim, val);      
}

//==============================================================================
// BProtoAction::getImpactEffectVisIndex
//==============================================================================
long BProtoAction::getImpactEffectProtoID() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mImpactEffectProtoID;
   else
      return -1;
}

//==============================================================================
// BProtoAction::getImpactEffectSize
//==============================================================================
long BProtoAction::getImpactEffectSize() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mImpactEffectSize;
   else
      return -1;
}

//==============================================================================
// BProtoAction::getImpactCameraShakeDuration
//==============================================================================
float BProtoAction::getImpactCameraShakeDuration() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mImpactCameraShakeDuration;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getImpactCameraShakeStrength
//==============================================================================
float BProtoAction::getImpactCameraShakeStrength() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mImpactCameraShakeStrength;
   else
      return 0.0f;
}

//==============================================================================
//==============================================================================
bool BProtoAction::getDoShockwaveAction() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagDoShockwaveAction();
   else
      return false;
}

//==============================================================================
// BProtoAction::getFriendlyFire
//==============================================================================
bool BProtoAction::getFriendlyFire() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagFriendlyFire();
   else
      return false;
}

//==============================================================================
// BProtoAction::usesHeightBonusDamage
//==============================================================================
bool BProtoAction::usesHeightBonusDamage() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagHeightBonusDamage();
   else
      return false;
}

//==============================================================================
// BProtoAction::usesHeightBonusDamage
//==============================================================================
bool BProtoAction::usesAmmo() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagUsesAmmo();
   else
      return false;
}

//==============================================================================
// BProtoAction::usesAmmo
//==============================================================================
bool BProtoAction::usesBeam() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->getFlagUsesBeam();
   else
      return false;
}

//==============================================================================
// BProtoAction::getHardpointID
//==============================================================================
long BProtoAction::getHardpointID() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mHardpointID;
   else
      return -1;
}

//==============================================================================
// BProtoAction::getAlternateHardpointID
//==============================================================================
long BProtoAction::getAlternateHardpointID() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mAlternateHardpointID;
   else
      return -1;
}

//==============================================================================
// BProtoAction::getTriggerScript
//==============================================================================
void BProtoAction::getTriggerScript(BSimString& scriptName) const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      scriptName = pWeapon->mpStaticData->mTriggerScript;
   else
      scriptName = "";
}

//==============================================================================
// Get the work rate taking into account the source unit's work rate scalar
//==============================================================================
float BProtoAction::getWorkRate(BEntityID sourceUnit /*= cInvalidObjectID*/) const
{
   if (sourceUnit != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 4058
      const BUnit* pUnit = gWorld->getUnit(sourceUnit);
//--
      if (pUnit)
      {
         return (mWorkRate * pUnit->getWorkRateScalar());
      }
   }

   return (mWorkRate);
}

//===========================================================================================================
// Get max range taking into account a valid weapon, garrison multiplier, and the unit't weapon range scalar
//===========================================================================================================
float BProtoAction::getMaxRange(const BUnit* pUnit, bool bIncludeGroupRange, bool bIncludePullRange) const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);

   if (pWeapon)
   {
      float range = pWeapon->mMaxRange;

      if (pUnit)
      {
         // If we have a charging weapon and it's charge, take pull range into account
         if (bIncludePullRange)
         {
            BUnitActionChargedRangedAttack* pAction = (BUnitActionChargedRangedAttack*)pUnit->getActionByTypeConst(BAction::cActionTypeUnitChargedRangedAttack);
            if (pAction && pAction->isCharged())
               range = Math::Max(range, pWeapon->getMaxPullRange());
         }

         if (bIncludeGroupRange && pWeapon->getFlagUseGroupRange())
            range += pUnit->getGroupDiagSpan();
         range *= pUnit->getWeaponRangeScalar();
      }

      return (range);
   }
   else
   {
      return (mpStaticData->mWorkRange);
   }
}

//==============================================================================
// BProtoAction::getMinRange
//==============================================================================
float BProtoAction::getMinRange() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mMinRange;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::isWeaponAffectedByGravity
//==============================================================================
bool BProtoAction::isWeaponAffectedByGravity(const BPlayer* pPlayer) const
{
   const BProtoObject*  pProto = pPlayer ? pPlayer->getProtoObject(getProjectileID()) : gDatabase.getGenericProtoObject(getProjectileID());
   if (pProto)
      return pProto->getFlagIsAffectedByGravity();

   return false;
}

//==============================================================================
// BProtoAction::getTargetPriority
//==============================================================================
float BProtoAction::getTargetPriority(const BProtoObject* pTargetProto) const
{
   float priorityAdjustment = 0.0f;
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
   {
      uint numTargetPriorities = pWeapon->mpStaticData->mTargetPriorities.getSize();
      for(uint i = 0; i < numTargetPriorities; i++)
      {
//-- FIXING PREFIX BUG ID 4059
         const BWeaponStatic::BTargetPriority& targetPri = pWeapon->mpStaticData->mTargetPriorities.get(i);
//--
         //-- Check to see if the objectType specified is a protoID
         if(targetPri.mProtoID == pTargetProto->getID())
         {
            priorityAdjustment += targetPri.mPriorityAdjustment;
         }
         else
         {
            if(pTargetProto && pTargetProto->isType(targetPri.mProtoID))
            {
               priorityAdjustment += targetPri.mPriorityAdjustment;
            }
         }                         
      }           
   }   

   //-- No matches? Then there is no adjustment.
   return priorityAdjustment;
}

//==============================================================================
// BProtoAction::getBeamSearchRadius
//==============================================================================
float BProtoAction::getBeamSearchRadius() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mBeamSearchRadius;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getBeamActivationSpeed
//==============================================================================
float BProtoAction::getBeamActivationSpeed() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mBeamActivationSpeed;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getBeamTrackingSpeed
//==============================================================================
float BProtoAction::getBeamTrackingSpeed() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mBeamTrackingSpeed;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getBeamTurnRate
//==============================================================================
float BProtoAction::getBeamTurnRate() const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return (pWeapon->mpStaticData->mBeamTurnRate);
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getBeamWaitTime
//==============================================================================
float BProtoAction::getBeamWaitTime() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mBeamWaitTime;
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getTargetsFootOfUnit
//==============================================================================
bool  BProtoAction::getTargetsFootOfUnit() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mTargetsFootOfUnit;
   else
      return false;
}

//==============================================================================
// BProtoAction::getTargetsFootOfUnit
//==============================================================================
bool BProtoAction::keepDPSRamp() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mKeepDPSRamp;
   else
      return false;
}

//==============================================================================
// BProtoAction::getCausePhysicsExplosion
//==============================================================================
bool BProtoAction::getCausePhysicsExplosion() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mCausePhysicsExplosion;
   else
      return false;
}

//==============================================================================
// BProtoAction::getPhysicsExplosionParticle
//==============================================================================
int BProtoAction::getPhysicsExplosionParticle() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPhysicsExplosionParticleHandle;
   else
      return -1;

}

//==============================================================================
// BProtoAction::getPhysicsExplosionParticle
//==============================================================================
BObjectTypeID BProtoAction::getPhysicsExplosionVictimType() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return pWeapon->mpStaticData->mPhysicsExplosionVictimType;
   else
      return -1;
}

//==============================================================================
// BProtoAction::getPreAttackCooldownMin
//==============================================================================
float BProtoAction::getPreAttackCooldownMin() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPreAttackCooldownMin;
   else
      return false;
}

//==============================================================================
// BProtoAction::getPreAttackCooldownMax
//==============================================================================
float BProtoAction::getPreAttackCooldownMax() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPreAttackCooldownMax;
   else
      return false;
}

//==============================================================================
// BProtoAction::getPostAttackCooldownMin
//==============================================================================
float BProtoAction::getPostAttackCooldownMin() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPostAttackCooldownMin;
   else
      return false;
}

//==============================================================================
// BProtoAction::getPostAttackCooldownMax
//==============================================================================
float BProtoAction::getPostAttackCooldownMax() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mPostAttackCooldownMax;
   else
      return false;
}

//==============================================================================
// BProtoAction::rollPostAttackCooldown
//==============================================================================
float BProtoAction::rollPostAttackCooldown() const
{
   float maxVariation = getPostAttackCooldownMax();
   if(maxVariation > 0.0f)
      return getRandRangeFloat(cSimRand, getPostAttackCooldownMin(), maxVariation);
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::rollPreAttackCooldown
//==============================================================================
float BProtoAction::rollPreAttackCooldown() const
{
   float maxVariation = getPreAttackCooldownMax();
   if(maxVariation > 0.0f)
      return getRandRangeFloat(cSimRand, getPreAttackCooldownMin(), maxVariation);
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getAttackRate
//==============================================================================
float BProtoAction::getAttackRate() const
{
   const BWeapon *pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if(pWeapon)
      return pWeapon->mpStaticData->mAttackRate;
   else
      return false;
}

//==============================================================================
//==============================================================================
float BProtoAction::getMaxDamagePerRam() const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return (pWeapon->mMaxDamagePerRam);
   else
      return 0.0f;
}

//==============================================================================
//==============================================================================
float BProtoAction::getReflectDamageFactor() const
{
   const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
   if (pWeapon)
      return (pWeapon->mReflectDamageFactor);
   else
      return 0.0f;
}

//==============================================================================
// BProtoAction::getAnimType
//==============================================================================
long BProtoAction::getAnimType() const
{
   return mAnimType;
}

//==============================================================================
// BProtoAction::getAddedBaseDPS
//==============================================================================
float BProtoAction::getAddedBaseDPS() const
{
   if(mpTactic)
   {      
      const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mBaseDPSWeaponID);
      if(pWeapon)
         return pWeapon->mDamagePerSecond;
   }

   return 0.0f;
}

//==============================================================================
// BProtoAction::getThrowOffsetAngle
//==============================================================================
float BProtoAction::getThrowOffsetAngle() const
{
   if(mpTactic)
   {      
      const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
      if(pWeapon)
         return pWeapon->mpStaticData->mThrowOffsetAngle;
   }
   return 0.0f;
}

//==============================================================================
// BProtoAction::getThrowOffsetAngle
//==============================================================================
float BProtoAction::getThrowVelocity() const
{
   if(mpTactic)
   {      
      const BWeapon* pWeapon = mpTactic->getWeapon(mpStaticData->mWeaponID);
      if(pWeapon)
         return pWeapon->mpStaticData->mThrowVelocity;
   }
   return 0.0f;
}

//==============================================================================
//==============================================================================
bool BProtoAction::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, float, mWorkRate);
   GFWRITEVAR(pStream, float, mWorkRateVariance);
   GFWRITEVAR(pStream, long, mAnimType);
   GFWRITEVAR(pStream, float, mDamagePerAttack);
   GFWRITEVAR(pStream, long, mMaxNumAttacksPerAnim);
   GFWRITEVAR(pStream, float, mStrafingTurnRate);
   GFWRITEVAR(pStream, float, mJoinBoardTime);
   GFWRITEBITBOOL(pStream, mFlagDisabled);
   GFWRITEMARKER(pStream, cSaveMarkerProtoAction);
   return true;
}

//==============================================================================
//==============================================================================
bool BProtoAction::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, float, mWorkRate);
   GFREADVAR(pStream, float, mWorkRateVariance);
   GFREADVAR(pStream, long, mAnimType);
   GFREADVAR(pStream, float, mDamagePerAttack);
   if (mGameFileVersion >= 2)
      GFREADVAR(pStream, long, mMaxNumAttacksPerAnim);
   GFREADVAR(pStream, float, mStrafingTurnRate);
   GFREADVAR(pStream, float, mJoinBoardTime);
   GFREADBITBOOL(pStream, mFlagDisabled);

   GFREADMARKER(pStream, cSaveMarkerProtoAction);

   gSaveGame.remapAnimType(mAnimType);
   return true;
}

//==============================================================================
//==============================================================================
bool BProtoAction::savePtr(BStream* pStream, const BProtoAction* pProtoAction)
{
   bool protoAction = (pProtoAction != NULL);
   GFWRITEVAR(pStream, bool, protoAction);
   if (pProtoAction)
   {
      const BTactic* pTactic = pProtoAction->getTactic();
      GFWRITEVAL(pStream, int8, (pTactic ? pTactic->getPlayerID() : cInvalidPlayerID));
      GFWRITEVAL(pStream, BProtoObjectID, (pTactic ? pTactic->getProtoObjectID() : cInvalidProtoObjectID));
      GFWRITEVAL(pStream, int8, pProtoAction->getIndex());
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BProtoAction::loadPtr(BStream* pStream, BProtoAction** ppProtoAction)
{
   (*ppProtoAction) = NULL;
   bool protoAction;
   GFREADVAR(pStream, bool, protoAction);
   if (protoAction)
   {
      BPlayerID playerID;
      BProtoObjectID protoObjectID;
      int index;
      GFREADVAL(pStream, int8, BPlayerID, playerID);
      if (playerID != cInvalidPlayerID)
         GFVERIFYCOUNT(playerID, cMaximumSupportedPlayers);
      GFREADVAR(pStream, BProtoObjectID, protoObjectID);
      gSaveGame.remapProtoObjectID(protoObjectID);
      GFREADVAL(pStream, int8, int, index);
      GFVERIFYCOUNT(index, 100);
      BPlayer* pPlayer = gWorld->getPlayer(playerID);
      BProtoObject* pProtoObject = (pPlayer ? pPlayer->getProtoObject(protoObjectID) : gDatabase.getGenericProtoObject(protoObjectID));
      if (pProtoObject)
      {
         BTactic* pTactic = pProtoObject->getTactic();
         if (pTactic)
            (*ppProtoAction) = pTactic->getProtoAction(index);
      }
   }
   return true;
}

//==============================================================================
// BTargetRule
//==============================================================================
BTargetRule::BTargetRule() : 
   mActionID(-1),
   mAbilityID(-1),
   mRelation(cRelationTypeEnemy), 
   mSquadMode(-1),
   mHasShieldDamageType(false), 
   mTargetStateUnbuilt(false),
   mTargetStateDamaged(false),
   mTargetStateCapturable(false),
   mTargetsGround(false),
   mContainsUnits(false),
   mGaiaOwned(false),
   mAutoTargetSquadMode(false),
   mOptionalAbility(false),
   mMergeSquads(false),
   mMeleeAttacker(false)
{
}

//==============================================================================
// BTacticState
//==============================================================================
BTacticState::BTacticState() : 
   mName(""),
   mIdleAnim(-1),
   mWalkAnim(-1),
   mJogAnim(-1),
   mRunAnim(-1),
   mDeathAnim(-1)
{
}

//==============================================================================
// BTactic::BTactic
//==============================================================================
BTactic::BTactic()
{
   for(long i=0; i<BSquadAI::cNumberModes; i++)
      mOverallRange[i]=0.0f;

   mpStaticData = new BTacticStatic();

   mProtoObjectID = cInvalidProtoObjectID;
   mPlayerID = (int8)cInvalidPlayerID;

   // Init Flags
   mFlagOwnStaticData=true;
   mFlagCanAttack=false;
   mFlagCanGather=false;
   mFlagCanGatherSupplies=false;
   mFlagCanBuild=false;
   mFlagCanRepair=false;
   mFlagCanAutoRepair=false;
   mFlagLoaded=false;
   mAnimInfoLoaded=false;

}

//==============================================================================
// BTactic::BTactic
//==============================================================================
BTactic::BTactic(const BTactic* pBase, BProtoObjectID protoObjectID)
{
   // Dynamic data
   
   mProtoObjectID = protoObjectID;
   mPlayerID = pBase->mPlayerID;

   // Flags
   mFlagOwnStaticData=pBase->mFlagOwnStaticData;
   mFlagCanAttack=pBase->mFlagCanAttack;
   mFlagCanGather=pBase->mFlagCanGather;
   mFlagCanGatherSupplies=pBase->mFlagCanGatherSupplies;
   mFlagCanBuild=pBase->mFlagCanBuild;
   mFlagCanAutoRepair=pBase->mFlagCanAutoRepair;
   mFlagCanRepair=pBase->mFlagCanRepair;
   mFlagLoaded=pBase->mFlagLoaded;
   mAnimInfoLoaded=pBase->mAnimInfoLoaded;

   for(long i=0; i<BSquadAI::cNumberModes; i++)
      mOverallRange[i]=pBase->mOverallRange[i];

   mAttackRatingDPS.clear();
   uint numAttackRatings = pBase->mAttackRatingDPS.getSize();
   if(numAttackRatings>0)
   {
      mAttackRatingDPS.resize(numAttackRatings);
      for(uint i=0; i<numAttackRatings; i++)
         mAttackRatingDPS[i]=pBase->mAttackRatingDPS[i];
   }

   for(long i=0; i<pBase->mWeapons.getNumber(); i++)
   {
      BWeapon* pWeapon=new BWeapon(pBase->mWeapons[i]);
      if(pWeapon)
         mWeapons.add(pWeapon);
   }

   for(long i=0; i<pBase->mProtoActions.getNumber(); i++)
   {
      BProtoAction* pProtoAction=new BProtoAction(this, pBase->mProtoActions[i]);
      if(pProtoAction)
      {
         pProtoAction->setIndex(mProtoActions.getNumber());
         mProtoActions.add(pProtoAction);
      }
   }

   mpStaticData=pBase->mpStaticData;

   mFlagOwnStaticData = false;
}

//==============================================================================
// BTactic::BTactic
//==============================================================================
BTactic::~BTactic()
{
   //-- Cleanup Weapons
   for(long i = 0; i < mWeapons.getNumber(); i++)
   {
      delete mWeapons[i];
   }
   mWeapons.clear();

   //-- Cleanup ProtoActions
   for(long i = 0; i < mProtoActions.getNumber(); i++)
   {
      delete mProtoActions[i];
   }
   mProtoActions.clear();

   if(mpStaticData)
   {
      if(getFlagOwnStaticData())
      {
         delete mpStaticData;
         mFlagOwnStaticData = false;
      }
      mpStaticData=NULL;
   }

   mAttackRatingDPS.clear();

}


//==============================================================================
// BTactic::loadTactic
//==============================================================================
bool BTactic::loadTactic(const BProtoObject *pObj)
{
   mFlagLoaded = true;

   BXMLReader reader;
   if(!reader.load(cDirTactics, mpStaticData->mName))
      return false;
   BXMLNode root=reader.getRootNode();

   bool result = true;
   BDynamicArray<BStateLoadHelper> stateLoadHelpers;

   //Note: Each load is dependent on the next. Order matters
   result &= loadWeapons(root, pObj);
   // WMJ TODO - pass the proto
   result &= loadStates(root, stateLoadHelpers);
   result &= loadProtoActions(root);
   result &= loadTacticRules(root);
   // load up the actions in the states
   result &= loadStateActions(stateLoadHelpers);

   if(result)
   {
      updateOverallRanges();
      //updateAttackRatings(NULL);  //moved to setupTacticAttackRatings
   }

   return result;
}

//==============================================================================
// BTactic::postloadTactic
//==============================================================================
void BTactic::postloadTactic(const BProtoObject *pObj)
{
   long numActions = mProtoActions.getNumber();
   for (long i = 0; i < numActions; i++)
   {
      mProtoActions[i]->postloadProtoAction(pObj);
   }

   long numWeapons = mWeapons.getNumber();
   for (long i = 0; i < numWeapons; i++)
   {
      mWeapons[i]->postloadWeapon();
   }
}

//==============================================================================
// BTactic::setupTacticAttackRatings
//==============================================================================
void BTactic::setupTacticAttackRatings()
{
   updateAttackRatings(NULL);
}

//==============================================================================
// BTactic::loadWeapons
//==============================================================================
bool BTactic::loadWeapons(BXMLNode root, const BProtoObject *pObj)
{
   if(root == NULL)
      return false;

   //-- Run through the file, find and load all the specified weapons
   long numNodes = root.getNumberChildren();
   for(long i = 0; i < numNodes; i++)
   {
      BXMLNode node = root.getChild(i);
      if(node.getName() == "Weapon")
      {
         BWeapon* pWeapon = new BWeapon();
         pWeapon->loadWeapon(node, pObj);
         mWeapons.add(pWeapon);
         mFlagCanAttack = true;
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BTactic::loadStates(BXMLNode root, BDynamicArray<BStateLoadHelper>& loadHelpers)
{
   if (!root)
      return (false);

   long numNodes=root.getNumberChildren();
   for (long i=0; i < numNodes; i++)
   {
      BXMLNode node=root.getChild(i);
      if (node.getName() == "State")
      {
         BStateLoadHelper& loadHelper = loadHelpers.grow();
         loadTacticState(node, loadHelper);
      }
   }

   return (true);
}

//==============================================================================
// BTactic::loadProtoActions
//==============================================================================
bool BTactic::loadProtoActions(BXMLNode root)
{
   if(root == NULL)
      return false;

   //-- Run through the file, find and load all the protoActions
   long numNodes = root.getNumberChildren();
   for(long i = 0; i < numNodes; i++)
   {
      BXMLNode node = root.getChild(i);
      if(node.getName() == "Action")
      {
         BProtoAction* pAction = new BProtoAction();
         if(pAction->loadProtoAction(node, mProtoActions.getNumber(), this))
         {
            pAction->setIndex(mProtoActions.getNumber());
            mProtoActions.add(pAction);
            if(pAction->getActionType()==BAction::cActionTypeUnitGather)
            {
               mFlagCanGather = true;
               if (pAction->getResourceType() == gDatabase.getRIDSupplies())
                  mFlagCanGatherSupplies = true;
            }
            
            if (pAction->getAutoRepairIdleTime() > 0)
            {
               mFlagCanAutoRepair = true;
            }
         }
         else
            delete pAction;
      }
   }
   
   return true;
}

//==============================================================================
// BTactic::loadTacticRules
//==============================================================================
bool BTactic::loadTacticRules(BXMLNode root)
{
   if (root == NULL)
      return (false);

   //-- Run through the file, find and load the Tactic Rules
   //-- Note: There should only be one!
   BXMLNode node;
   long numNodes = root.getNumberChildren();
   for(long i = 0; i < numNodes; i++)
   {
      node = root.getChild(i);
      if (node.getName() == "Tactic")
         break;
      else
         node.setInvalid();
   }

   if (!node.getValid())
      return (false);

   //-- Load the target rules and persistent actions
   long nodeCount = node.getNumberChildren();
   for (long i = 0; i < nodeCount; i++)
   {
      BXMLNode child(node.getChild(i));
      const BPackedString& name = child.getName();

      BSimString tempStr;
      if (name == "TargetRule")
      {
         loadTargetRule(child);
      }
      else if (name == "PersistentAction")
      {
         long actionID = getProtoActionID(child.getTextPtr(tempStr));
         if (actionID != -1)
            mpStaticData->mPersistentActions.add(actionID);
      }
      else if (name == "PersistentSquadAction")
      {
         long actionID = getProtoActionID(child.getTextPtr(tempStr));
         if (actionID != -1)
            mpStaticData->mPersistentSquadActions.add(actionID);
      }
   }

   return (true);
}

//==============================================================================
// BTactic::loadTargetRule
//==============================================================================
bool BTactic::loadTargetRule(BXMLNode root)
{
   if (root == NULL)
      return (false);

   BTargetRule& rule = mpStaticData->mTargetRules.grow();

   long nodeCount = root.getNumberChildren();
   for (long i = 0; i < nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString& name = node.getName();

      BSimString tempStr;
      if (name == "Relation")
         rule.mRelation = gDatabase.getRelationType(node.getTextPtr(tempStr));
      else if (name == "SquadMode" || name == "AutoTargetSquadMode")
      {
         rule.mSquadMode = gDatabase.getSquadMode(node.getTextPtr(tempStr));
         if (name == "AutoTargetSquadMode")
            rule.mAutoTargetSquadMode = true;
      }
      else if (name == "TargetsGround")
         rule.mTargetsGround = true;
      else if (name == "ContainsUnits")
         rule.mContainsUnits = true;
      else if (name == "DamageType")
      {
         long damageType = gDatabase.getDamageType(node.getTextPtr(tempStr));
         if (damageType == -1)
            gConsoleOutput.output(cMsgError, "Invalid damage type specified: Line %d of %s", root.getReader()->getLineNumber(), root.getReader()->getFilename().getPtr());
         else
         {
            if (damageType == gDatabase.getDamageTypeShielded())
               rule.mHasShieldDamageType = true;
            else
               rule.mDamageTypes.add(damageType);
         }
      }
      else if (name == "TargetType")
      {
         long objectType = gDatabase.getObjectType(node.getTextPtr(tempStr));
         if (objectType == -1)
            gConsoleOutput.output(cMsgError, "Invalid object type specified: Line %d of %s", root.getReader()->getLineNumber(), root.getReader()->getFilename().getPtr());
         else
            rule.mObjectTypes.add(objectType);
      }
      else if (name == "Action")
      {
         rule.mActionID = getProtoActionID(node.getTextPtr(tempStr));            
         if (rule.mActionID == -1)
            gConsoleOutput.output(cMsgError, "Invalid action '%s' referenced by target rule: Line %d of %s", node.getTextPtr(tempStr), root.getReader()->getLineNumber(), root.getReader()->getFilename().getPtr());
      }
      else if (name == "TargetState")
      {
         const BSimString& nodeText = node.getText(tempStr);
         if (nodeText == "Unbuilt")
            rule.mTargetStateUnbuilt = true;
         else if (nodeText == "Damaged")
            rule.mTargetStateDamaged = true;
         else if (nodeText == "Capturable")
            rule.mTargetStateCapturable = true;
      }
      else if (name == "Ability" || name == "OptionalAbility")
      {
         const BSimString& nodeText = node.getText(tempStr);
         rule.mAbilityID = gDatabase.getAbilityIDFromName(nodeText);

         // Delete rule if bad ability
         if (rule.mAbilityID == -1)
         {
            mpStaticData->mTargetRules.removeIndex(mpStaticData->mTargetRules.getNumber()-1);
            return false;
         }

         if (name == "OptionalAbility")
            rule.mOptionalAbility = true;
      }
      else if (name == "GaiaOwned")
      {
         rule.mGaiaOwned = true;
      }
      else if (name == "MergeSquads")
      {
         rule.mMergeSquads = true;
      }
      else if (name == "MeleeAttacker")
      {
         rule.mMeleeAttacker = true;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BTactic::loadTacticState(BXMLNode root, BStateLoadHelper& loadHelper)
{
   if (root == NULL)
      return (false);

   BTacticState& state=mpStaticData->mStates.grow();
   loadHelper.state = &state;

   long nodeCount=root.getNumberChildren();
   for (long i=0; i < nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString& name=node.getName();

      BSimString tempStr;
      if (name == "Name")
         state.mName.set(node.getTextPtr(tempStr));
      else if (name == "IdleAnim")
         state.mIdleAnim=gVisualManager.getAnimType(node.getTextPtr(tempStr));
      else if (name == "WalkAnim")
         state.mWalkAnim=gVisualManager.getAnimType(node.getTextPtr(tempStr));
      else if (name == "JogAnim")
         state.mJogAnim=gVisualManager.getAnimType(node.getTextPtr(tempStr));
      else if (name == "RunAnim")
         state.mRunAnim=gVisualManager.getAnimType(node.getTextPtr(tempStr));
      else if (name == "DeathAnim")
         state.mDeathAnim=gVisualManager.getAnimType(node.getTextPtr(tempStr));
      else if (name == "Action")
         loadHelper.actions.add(node.getTextPtr(tempStr));
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BTactic::loadStateActions(const BDynamicArray<BStateLoadHelper>& loadHelpers)
{
   long size = loadHelpers.getSize();
   for (long i = 0; i < size; ++i)
   {
      const BStateLoadHelper& loadHelper = loadHelpers.get(i);
      if (!loadHelper.state)
         continue;

      long actionSize = loadHelper.actions.getSize();
      for (int ai = 0; ai < actionSize; ++ai)
      {
         long paid=getProtoActionID(loadHelper.actions.get(ai));
         if (paid >= 0)
            loadHelper.state->mActions.uniqueAdd(paid);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BTactic::can(uint state, BActionType type) const
{
   //Get the state, okay if NULL (for E3).
   const BTacticState* pState=getState(state);

   for (uint i=0; i < mProtoActions.getSize(); i++)
   {
      //If state is valid, must have this action.
      if (pState && !pState->mActions.contains(i))
         continue;
   
      if (mProtoActions[i]->getActionType() == type)
         return (true);
   }
   return (false);
}

//==============================================================================
// BTactic::getTargetRule
//==============================================================================
const BTargetRule* BTactic::getTargetRule(long index)
{
   if(index < 0 || index >= mpStaticData->mTargetRules.getNumber() )
      return NULL;

   return &(mpStaticData->mTargetRules[index]);
}

//==============================================================================
// Get the ability ID for this action if one exists
//==============================================================================
int BTactic::getAbilityID(int actionID)
{
   if (actionID == -1)
   {
      return (-1);
   }

   int numTargetRules = getNumberTargetRules();
   for (int i = 0; i < numTargetRules; i++)
   {
      const BTargetRule* pTargetRule = getTargetRule(i);
      if (pTargetRule && (pTargetRule->mActionID == actionID) && (pTargetRule->mAbilityID != -1))
      {
         return (pTargetRule->mAbilityID);
      }
   }

   return (-1);
}

//==============================================================================
// Returns the matching exposed action
//==============================================================================
BProtoAction* BTactic::getExposedAction(int exposedIndex, int& actionID)
{
   actionID = -1;
   int numActions = getNumberProtoActions();
   for (int i = 0; i < numActions; i++)
   {
      BProtoAction* pProtoAction = getProtoAction(i);
      if (pProtoAction && (pProtoAction->getExposedActionIndex() == exposedIndex))
      {
         //Halwes 4/26/2007 - Is this just 'i'?
         actionID = getProtoActionID(pProtoAction->getName());         
         return (pProtoAction);
      }
   }

   return (NULL);
}

//==============================================================================
// BTactic::getWeaponID
//==============================================================================
long BTactic::getWeaponID(const BSimString &weaponName)
{
   long numWeapons = mWeapons.getNumber();
   for(long i = 0; i < numWeapons; i++)
   {
      if(mWeapons.get(i)->mpStaticData->mName == weaponName)
         return i;
   }
   return -1;
}

//==============================================================================
// BTactic::getTacticStateID
//==============================================================================
int BTactic::getTacticStateID(const BSimString &tacticStateName)
{
   BASSERT(mpStaticData);
   long numStates = mpStaticData->mStates.getNumber();
   for(long i = 0; i < numStates; i++)
   {
      if(mpStaticData->mStates.get(i).mName == tacticStateName)
         return i;
   }
   return -1;
}

//==============================================================================
// BTactic::getProtoActionID
//==============================================================================
long BTactic::getProtoActionID(const BSimString &protoActionName)
{
   long numActions = mProtoActions.getNumber();
   for(long i = 0; i < numActions; i++)
   {
      if(mProtoActions.get(i)->getName() == protoActionName)
         return i;
   }
   return -1;
}

//==============================================================================
// BTactic::getProtoAction
//==============================================================================
BProtoAction* BTactic::getProtoAction(uint state, const BObject* pTarget, const BVector targetLoc,
   const BPlayer* pPlayer, BVector sourcePosition, BEntityID sourceUnit, long abilityID,
   bool autoTarget, long actionType, bool checkTimers, bool checkOrientation, bool* pInsideMinRange, bool noDiscardAbilities, long* pRuleAbilityID,
   bool* pRuleTargetsGround, bool ignoreAlreadyJoined)
{
   if (!getFlagLoaded())
   {
      BFAIL("Tactic not loaded!");

      if (pRuleAbilityID)
      {
         *pRuleAbilityID = cInvalidAbilityID;
      }

      return NULL;
   }

   //Grab the source (okay to be NULL).
   BUnit* pSourceUnit=gWorld->getUnit(sourceUnit);
//-- FIXING PREFIX BUG ID 4068
   const BSquad* pSourceSquad=NULL;
//--
   if (pSourceUnit)
      pSourceSquad=pSourceUnit->getParentSquad();
   long sourceSquadMode=-1;
   bool sourceSquadAbilityRecovering=false;
   BEntityID sourceSquad = cInvalidObjectID;
   if (pSourceSquad)
   {
      sourceSquadMode=pSourceSquad->getCurrentOrChangingMode();
      sourceSquadAbilityRecovering=(pSourceSquad->getRecoverType() == cRecoverAbility);
      sourceSquad=pSourceSquad->getID();
   }
   // See if abilities are enabled for the source unit
   if (pSourceUnit && pSourceUnit->getProtoObject() && pSourceUnit->getProtoObject()->getFlagAbilityDisabled() && pSourceUnit->getProtoObject()->getAbilityCommand() == abilityID)
      sourceSquadAbilityRecovering = true;

   //Get the state.  Okay if NULL.
   const BTacticState* pState=getState(state);

   // No actions, so use actions from the default state
   if (pState && pState->mActions.size() == 0)
      pState = NULL;

   // Fall back to non-ability proto action if there isn't an ability one.
   BProtoAction* pFallbackProtoAction = NULL;

   // Iterate through all the target rules in this tactic to try to find the right action to do based on the info provided.
   long numTargetRules = mpStaticData->mTargetRules.getNumber();
   for (long ruleIdx=0; ruleIdx<numTargetRules; ruleIdx++)
   {
      // Quick bail if no valid action.
      BTargetRule &rule = mpStaticData->mTargetRules[ruleIdx];
      if (rule.mActionID == -1)
         continue;

      //-- If there is still a cooldown for this weapon then bail.
      if(checkTimers)
      {
         long weaponID = mProtoActions[rule.mActionID]->getWeaponID();
         if(pSourceUnit && weaponID >= 0)
         {
            if(pSourceUnit->getAttackWaitTimer(weaponID) > 0.2f)
               continue;

            if(pSourceUnit->getPreAttackWaitTimer(weaponID) > 0.2f)
               continue;
         }      
      }

      //Bail if there is a specific type that doesn't match this action.
      if ((actionType != -1) && (actionType != mProtoActions[rule.mActionID]->getActionType()))
         continue;
      // Bail if the action is disabled
      if(mProtoActions[rule.mActionID]->getFlagDisabled())
         continue;
      //Bail if the action isn't in the state.
      if (pState && !pState->mActions.contains(rule.mActionID))
         continue;
      // Bail if this isn't allowed for auto-target.
      if (autoTarget && mProtoActions[rule.mActionID]->getFlagNoAutoTarget())
         continue;

      //---------------------------------------------------------
      // Ability check #1
      //---------------------------------------------------------
      // Halwes - 9/22/2008 - Don't filter the ungarrison ability out.
      if ((rule.mAbilityID != -1) && !rule.mOptionalAbility && (rule.mAbilityID != gDatabase.getAIDUngarrison()))
      {
         // Not looking for abilities so reject them
         if (!noDiscardAbilities && (abilityID == -1))
            continue;

         // Reject all abilities if recovering
         if (sourceSquadAbilityRecovering)
            continue;

         // Reject if ability disabled
         if (pSourceUnit && pSourceUnit->getProtoObject() && pSourceUnit->getProtoObject()->getFlagAbilityDisabled())
            continue;

         // Make sure ability IDs match
         if (!noDiscardAbilities && (rule.mAbilityID != abilityID))
            continue;
      }

      //---------------------------------------------------------
      // Merge squads check.
      //---------------------------------------------------------
      if (rule.mMergeSquads)
      {
         if (!pSourceSquad || !pTarget)
            continue;
//-- FIXING PREFIX BUG ID 4062
         const BSquad* pTargetSquad = NULL;
//--
         if (pTarget->isClassType(BEntity::cClassTypeUnit))
         {
            const BUnit* pTargetUnit = pTarget->getUnit();
            if (pTargetUnit)
               pTargetSquad = pTargetUnit->getParentSquad();
         }
         else if (pTarget->isClassType(BEntity::cClassTypeSquad))
            pTargetSquad = const_cast<BObject*>(pTarget)->getSquad();

         if (!pTargetSquad)
            continue;
         const BProtoSquad* pSourceProtoSquad = pSourceSquad->getProtoSquad();
         if (!pSourceProtoSquad)
            continue;
         BProtoSquadID tempPSID;
         if (!pSourceProtoSquad->canMerge(pTargetSquad->getProtoSquadID(), tempPSID))
            continue;
      }

      //---------------------------------------------------------
      // Squad mode check.  Happens regardless of target type
      //---------------------------------------------------------
      if (rule.mSquadMode != -1 && !(rule.mAutoTargetSquadMode && !autoTarget))
      {
         if(sourceSquadMode != rule.mSquadMode)
            continue;
      }

      //---------------------------------------------------------
      // Contains unit check
      //---------------------------------------------------------
      if (rule.mContainsUnits)
      {
         if (!pSourceUnit)
            continue;
         if (pSourceUnit->getFlagHasGarrisoned() && pSourceUnit->getFlagHasAttached())
            continue;
      }

      //---------------------------------------------------------
      // Lockdown checks
      //---------------------------------------------------------
      bool minRangeTargetCheck = false;
      if(sourceSquadMode == BSquadAI::cModeLockdown && pSourceSquad && (pTarget || rule.mTargetsGround))
      {
         // ajl 4/20/08 - Jira 5650 X/Y button change
         bool bCanAutoUnlock = (!autoTarget || (pSourceUnit && pSourceUnit->canAutoUnlock()));
         //bool bCanAutoUnlock = (pSourceUnit ? pSourceUnit->canAutoUnlock() : false);
         if (!bCanAutoUnlock && abilityID != -1)
         {
            int squadAbilityID = gDatabase.getSquadAbilityID(pSourceSquad, abilityID);
//-- FIXING PREFIX BUG ID 4063
            const BAbility* pAbility = gDatabase.getAbilityFromID(squadAbilityID);
//--
            if (pAbility && pAbility->getType() == cAbilityChangeMode && pAbility->getSquadMode() == BSquadAI::cModeLockdown)
               bCanAutoUnlock = true;
         }
         if (!bCanAutoUnlock)
         {
            // Range
            float range = mProtoActions[rule.mActionID]->getMaxRange(pSourceUnit);
            float minRange=mProtoActions[rule.mActionID]->getMinRange();

            if (pSourceSquad)
            {
               // mrh 3/18/08 - Min range is now defined as edge of attacker to center point of target.  Ummm... ok.
               float distance = 0.0f;
               float minRangeDistance = 0.0f;

               if (pTarget)
               {
                  // If the target object is a valid unit then we need to calculate the range based on the unit's parent squad
                  const BUnit* pUnit = pTarget->getUnit();
                  if (pUnit)
                  {
//-- FIXING PREFIX BUG ID 4064
                     const BSquad* pSquad = pUnit->getParentSquad();
//--
                     if (!pSquad)
                        continue;

                     distance = pSourceSquad->calculateXZDistance(pSquad);
                     minRangeDistance = pSourceSquad->calculateXZDistance(pSquad->getPosition());
                  }
                  //Else, just use pTarget.
                  else
                  {
                     distance = pSourceSquad->calculateXZDistance(pTarget);
                     minRangeDistance = pSourceSquad->calculateXZDistance(pTarget->getPosition());
                  }
               }
               else
               {
                  distance = pSourceSquad->calculateXZDistance(targetLoc);
                  minRangeDistance = distance;
               }

               //Over range == fail.
               if (distance > range)
                  continue;
               //Under min range == fail.
               if ((minRange > cFloatCompareEpsilon) && (minRangeDistance < minRange))
               {
                  if (pInsideMinRange)
                     minRangeTargetCheck = true;
                  else
                     continue;
               }
            }
         }
      }

      //Check ammo if we need to.
      if (mProtoActions[rule.mActionID]->usesAmmo() && (mProtoActions[rule.mActionID]->getMaxNumAttacksPerAnim() > 0))
      {
         float ammoAmount=mProtoActions[rule.mActionID]->getMaxNumAttacksPerAnim()*mProtoActions[rule.mActionID]->getDamagePerAttack();
         if (pSourceUnit->getAmmunition() < ammoAmount)
            continue;
      }

      //E3 unit to unit range check for stationary actions
      /*if (pTarget && mProtoActions[rule.mActionID]->getStationary())
      {
         // SLB: This code sucks, but I'm tired and I'm in a hurry so give me a break
         const BSquad* pTargetSquad = pTarget->isClassType(BEntity::cClassTypeSquad) ? reinterpret_cast<const BSquad*>(pTarget) : NULL;
         const BObject* pTargetObject = pTargetSquad ? pTargetSquad->getLeaderUnit() : pTarget;
         float targetDist = pSourceUnit->calculateXZDistance(pTargetObject);
         if (targetDist > mProtoActions[rule.mActionID]->getMaxRange(pSourceUnit))
            continue;
      }*/

      if (rule.mTargetsGround)
      {
         //----------------------------------------------
         // Target ground checks
         //----------------------------------------------
         // BSR 5/8/08 - The following travesty is to circumvent the fact that we cannot elegantly tell the system that a "TargetsGround"
         // ability would be smashing right now if only you would pretend that there is not an untargetable unit under the cursor. Forgive me, I was raised by wolves.
//         if (pTarget)
         if (pTarget && (autoTarget || !mProtoActions[rule.mActionID]->targetsAir()))
            continue;
      }
      else
      {
         //----------------------------------------------
         // Target unit checks
         //----------------------------------------------
         if (!pTarget)
            continue;

         // Cast to target unit
//-- FIXING PREFIX BUG ID 4067
         const BUnit* pTargetUnit = (BUnit*)pTarget->getUnit();
//--

         // Don't target hibernating stuff
         if (pTargetUnit && pTargetUnit->isHibernating())
            continue;

         if (rule.mGaiaOwned)
         {
            if (pTarget->getPlayerID() != cGaiaPlayer)
               continue;
         }
         else
         {
            //-- Validate the relation to the target player
            if (rule.mRelation != cRelationTypeAny)
            {
               const BPlayer *pTargetPlayer = pTarget->getPlayer();
               switch(rule.mRelation)
               {
               case cRelationTypeSelf:
                  if(pTargetPlayer != pPlayer)
                     continue;
                  break;

               case cRelationTypeAlly:
                  if(!pPlayer->isAlly(pTargetPlayer))
                     continue;
                  break;

               case cRelationTypeEnemy:
                  {
                     // If the target does NOT contain any enemy units                     
                     if (!pTargetUnit || !pTargetUnit->hasGarrisonedEnemies(pPlayer->getID()))
                     {
                        if (pTarget->getProtoObject()->getFlagNeutral())
                           continue;

                        if (!pPlayer->isEnemy(pTargetPlayer) && !pTargetPlayer->isGaia())
                           continue;

                        // SLB: Filter out gaia units that aren't a threat
                        if (pTargetPlayer->getID() == cGaiaPlayer)
                        {
                           BTactic *pTargetTactic = pTarget->getTactic();
                           if (!pTargetTactic || !pTargetTactic->canAttack())
                              continue;
                        }
                     }
                  }
                  break;

               case cRelationTypeNeutral:
                  if (!pTarget->getProtoObject()->getFlagNeutral())
                     continue;
                  if (!pPlayer->isEnemy(pTargetPlayer))
                     continue;
                  break;
               }
            }
         }

         //-- Don't allow melee to attack cover objects
         if (mProtoActions[rule.mActionID]->isMeleeAttack())
         {
            if (pTarget->getProtoObject()->isType(gDatabase.getOTIDCover()))
               continue;
         }

         //-- Target Built State
         if (rule.mTargetStateUnbuilt)
         {
            if (pTarget->getFlagBuilt() || !pTarget->getProtoObject()->getFlagManualBuild())
               continue;
         }

         //-- Target Damaged State
         if (rule.mTargetStateDamaged)
         {
            if (!pTarget->isDamaged())
               continue;
         }

         //-- Target Capturable State
         if (rule.mTargetStateCapturable)
         {
            if (!pTarget->isCapturable(pPlayer->getID(), sourceSquad))
               continue;
         }

         //-- Is target invulnerable?
         if (mProtoActions[rule.mActionID]->getActionType() == BAction::cActionTypeUnitRangedAttack)
         {
            // If we are attacking an invulnerable unit we need to make sure it does NOT have garrisoned enemies
            if (pTarget->getFlagInvulnerable() && (!pTargetUnit || !pTargetUnit->hasGarrisonedEnemies(pPlayer->getID())))
               continue;
         }

         //-- If if the action's weapon cannot orient the unit, and it can't currently hit it, then we fail.
         if (checkOrientation && mProtoActions[rule.mActionID]->canOrientOwner() == false)
         {
            //-- Make sure that this action's weapon can hit the target in its current orientation
            if(pSourceUnit->canOrientHardpointToWorldPos(mProtoActions[rule.mActionID]->getHardpointID(), pTarget->getPosition()) == false)
               continue;
         }

         // Is target melee attacking the source unit?
         if (rule.mMeleeAttacker)
         {
            if (!pTargetUnit)
               continue;
            // Make sure being attacked by target unit
            if (!pSourceUnit->isBeingAttackedByUnit(pTargetUnit->getID()))
               continue;
            // And the attack is melee attack
//-- FIXING PREFIX BUG ID 4066
            const BAction* pTargetsAttackAction = pTargetUnit->getActionByTypeConst(BAction::cActionTypeUnitRangedAttack);
//--
            if (!pTargetsAttackAction)
               continue;
            const BProtoAction* pPA = pTargetsAttackAction->getProtoAction();
            if (!pPA)
               continue;
            if (!pPA->isMeleeAttack())
               continue;
         }

         //----------------------------------------------
         // Check for target type match based on any of the following: shield damage type, regular damage type, or object type
         //----------------------------------------------
         //--Don't check for damage except against enemies
         bool isEnemy = pPlayer->isEnemy(pTarget->getPlayerID());

         long numDamageTypes = rule.mDamageTypes.getNumber();
         long numObjectTypes = rule.mObjectTypes.getNumber();
         if (isEnemy && rule.mHasShieldDamageType || numDamageTypes>0 || numObjectTypes>0)
         {
            bool anyTargetTypeMatch = false;

            //-- If our actionID has the shielded damage type and the target passes the shield test then return this actionID
            if (rule.mHasShieldDamageType)
            {
               const BUnit* pTargetUnit = const_cast<BObject*>(pTarget)->getUnit();
               if (pTargetUnit)
               {
                  // Shielded
                  if (pTargetUnit->getFlagHasShield())
                  {
                     // Has shield points
                     if (pTargetUnit->getShieldpoints() > 0.0f)
                     {
                        if (pTargetUnit->getProtoObject()->getDamageType(XMVectorSubtract(pTargetUnit->getPosition(), sourcePosition), pTargetUnit->getForward(), pTargetUnit->getRight(), true, false, pTargetUnit->getDamageTypeMode()) == gDatabase.getDamageTypeShielded())
                           anyTargetTypeMatch = true;
                     }
                  }
               }
            }

            //-- If our target is any of the specified damage types for this actionID then return it
            if (isEnemy && !anyTargetTypeMatch && numDamageTypes > 0)
            {
               const BUnit* pTargetUnit = const_cast<BObject*>(pTarget)->getUnit();
               if (pTargetUnit)
               {
                  int damageType = pTarget->getProtoObject()->getDamageType(XMVectorSubtract(pTarget->getPosition(), sourcePosition), pTarget->getForward(), pTarget->getRight(), false, true, pTargetUnit->getDamageTypeMode());
                  for(long typeIdx = 0; typeIdx < numDamageTypes; typeIdx++)
                  {
                     if(damageType == rule.mDamageTypes[typeIdx])
                     {
                        anyTargetTypeMatch = true;
                        break;
                     }
                  }
               }
            }

            //-- If our target is any of the specified types for this actionID then return it
            if (!anyTargetTypeMatch && numObjectTypes > 0)
            {
               for(long typeIdx = 0; typeIdx < numObjectTypes; typeIdx++)
               {
                  const BProtoObject*  pTargetProto = pTarget->getProtoObject();
                  if(pTargetProto && pTargetProto->isType(rule.mObjectTypes[typeIdx]))
                  {
                     if (mProtoActions[rule.mActionID]->getActionType() == BAction::cActionTypeUnitGarrison)
                     {
                        if (!pTarget->getFlagBuilt())
                           break;
                     }

                     anyTargetTypeMatch=true;
                     break;
                  }
               }
            }

            if(!anyTargetTypeMatch)
               continue;
         }
      }

      
      // If it's a jump attack rule, make sure we're not already in our weapon's distance
      if (mProtoActions[rule.mActionID]->getActionType() == BAction::cActionTypeUnitJumpAttack)
      {
         float range = mProtoActions[rule.mActionID]->getMaxRange(pSourceUnit);

         // If the target object is a valid unit then we need to calculate the range based on the unit's parent squad.  Otherwise, we just bail
         const BUnit* pUnit = pTarget->getUnit();
         if (pUnit)
         {
            BSquad* pSquad = pUnit->getParentSquad();
            if (!pSquad)
               continue;

            float distance = pSourceSquad->calculateXZDistance(pSquad);
            
            if (distance <= range)
            {
               continue;
            }
         }
      }

      // Don't garrison in a target that contains enemy, is full, or rejects our unit type
      BActionType actionType = mProtoActions[rule.mActionID]->getActionType();
      if ((actionType == BAction::cActionTypeUnitGarrison) || (actionType == BAction::cActionTypeSquadGarrison))
      {
         BUnit* pTargetUnit = (BUnit*)pTarget->getUnit();
         if (pTargetUnit && pTargetUnit->hasGarrisonedEnemies(pPlayer->getID()))
            continue;

         if (pTargetUnit && pSourceSquad && !pTargetUnit->canContain(pSourceSquad))
            continue;

         if (pTargetUnit && pSourceUnit && !pTargetUnit->canContain(pSourceUnit))
            continue;

         // [9/4/2008 xemu] also don't garrison in an object that we can't see yet due to blackmap
         // [9/8/2008 xemu] disable this check to put back old behavior if design changes it's mind. 
         if (pTargetUnit && pSourceUnit && !pTargetUnit->isExplored(pSourceUnit->getTeamID()))
            continue;
      }

      if (minRangeTargetCheck)
      {
         if (pInsideMinRange)
            *pInsideMinRange = true;
         continue;
      }

      // Don't board a vehicle that has already been boarded
      if (actionType == BAction::cActionTypeUnitJoin && !ignoreAlreadyJoined)
      {
         if (mProtoActions[rule.mActionID]->getJoinType() == BUnitActionJoin::cJoinTypeBoard)
         {
            BUnit* pTargetUnit = (BUnit*)pTarget->getUnit();
            // This isn't the most robust check, but boarding will either have a unit on it or in it.
            // If we need to board stuff that has other attachments or contained units, this check needs to change
            if (pTargetUnit && (pTargetUnit->getFlagHasGarrisoned() || pTargetUnit->getFlagHasAttached()))
               continue;
         }
      }

      //---------------------------------------------------------
      // Ability check #2.
      // If not looking for abilities, return the proto action immediately.
      // If looking for abilities, return ability proto actions immediately.
      // Save non-ability actions as a fallback in case an ability action isn't found.
      //---------------------------------------------------------
      if (abilityID == -1)
      {
         if (pRuleAbilityID)
         {
            *pRuleAbilityID = rule.mAbilityID;
         }

         if (pRuleTargetsGround)
            *pRuleTargetsGround = rule.mTargetsGround;

         return mProtoActions[rule.mActionID];
      }
      else
      {
         if ((rule.mAbilityID != -1) && !rule.mOptionalAbility)
         {
            if (pRuleAbilityID)
            {
               *pRuleAbilityID = rule.mAbilityID;
            }

            if (pRuleTargetsGround)
               *pRuleTargetsGround = rule.mTargetsGround;

            return mProtoActions[rule.mActionID];
         }
         else if (pFallbackProtoAction == NULL)
         {
            if (pRuleAbilityID)
            {
               *pRuleAbilityID = rule.mAbilityID;
            }

            if (pRuleTargetsGround)
               *pRuleTargetsGround = rule.mTargetsGround;

            pFallbackProtoAction = mProtoActions[rule.mActionID];
         }
      }
   }

   // Return the fallback proto action if one was found
   if (pFallbackProtoAction)
      return pFallbackProtoAction;

   //-- Didn't find one? Was there a default action specified?
   if(mpStaticData->mDefaultActionID >= 0 && mpStaticData->mDefaultActionID < mProtoActions.getNumber())
   {
      if (pRuleAbilityID)
      {
         *pRuleAbilityID = cInvalidAbilityID;
      }

      if (pRuleTargetsGround)
         *pRuleTargetsGround = false;

      return mProtoActions[mpStaticData->mDefaultActionID];
   }

   //-- Couldn't find an action for the target
   if (pRuleAbilityID)
   {
      *pRuleAbilityID = cInvalidAbilityID;
   }

   if (pRuleTargetsGround)
      *pRuleTargetsGround = false;

   return NULL;
}


//==============================================================================
// BTactic::getProtoAction
//==============================================================================
BProtoAction* BTactic::getProtoAction(long id)
{
   if(id<0 || id>=mProtoActions.getNumber())
      return NULL;
   else
      return mProtoActions[id];
}

//==============================================================================
// BTactic::updateOverallRanges
//==============================================================================
void BTactic::updateOverallRanges()
{
   for(long i=0; i<BSquadAI::cNumberModes; i++)
      mOverallRange[i]=0.0f;
   long numTargetRules = mpStaticData->mTargetRules.getNumber();
   for (long ruleIdx=0; ruleIdx<numTargetRules; ruleIdx++)
   {
      BTargetRule &rule = mpStaticData->mTargetRules[ruleIdx];
      if (rule.mActionID == -1)
         continue;
      float actionMaxRange=mProtoActions[rule.mActionID]->getMaxRange(NULL);
      for(long i=0; i<BSquadAI::cNumberModes; i++)
      {
         if(rule.mSquadMode==-1 || rule.mSquadMode==i)
         {
            if(actionMaxRange>mOverallRange[i])
               mOverallRange[i]=actionMaxRange;
         }
      }
   }
}

//==============================================================================
// BTactic::getOverallRange
//==============================================================================
float BTactic::getOverallRange(BEntityID sourceUnit, int squadMode) const
{
   if (squadMode < 0 || squadMode >= BSquadAI::cNumberModes)
      return 0.0f; 
   float range = mOverallRange[squadMode];
   if (sourceUnit != cInvalidObjectID)
   {
//-- FIXING PREFIX BUG ID 4072
      const BUnit* pUnit = gWorld->getUnit(sourceUnit);
//--
      if (pUnit)
         range *= pUnit->getWeaponRangeScalar();
   }

   return (range);
}

//==============================================================================
// BTactic::updateAttackRatings
//==============================================================================
void BTactic::updateAttackRatings(BPlayer* pPlayer)
{
   if (!canAttack())
      return;

   uint numDamageTypes = gDatabase.getNumberAttackRatingDamageTypes();
   mAttackRatingDPS.resize(numDamageTypes);
   for (uint i=0; i<numDamageTypes; i++)
      mAttackRatingDPS[i]=0.0f;

   uint numProtoActions = mProtoActions.getSize();
   for (uint i=0; i<numProtoActions; i++)
   {
//-- FIXING PREFIX BUG ID 4074
      const BProtoAction* pProtoAction = mProtoActions[i];
//--
      long actionType = pProtoAction->getActionType();
      if ((actionType == BAction::cActionTypeUnitRangedAttack) || (actionType == BAction::cActionTypeUnitSecondaryTurretAttack) || (actionType == BAction::cActionTypeUnitSlaveTurretAttack))
      {
//-- FIXING PREFIX BUG ID 4073
         const BWeaponType* pWeaponType = (pPlayer ? pPlayer->getWeaponTypeByID(pProtoAction->getWeaponType()) : gDatabase.getWeaponTypeByID(pProtoAction->getWeaponType()));
//--
         if (pWeaponType)
         {
            float dps = pProtoAction->getDamagePerSecond();

            bool checkRules = false;
            bool checkPriorities = false;
            long abiltyID = -1;
            bool isLockDownRule = false;
            bool isCoverRule = false;
            bool isCarryingObject = false;

            //for debug
            //const BString actionName = pProtoAction->getName();

            //Find ability
            long numTargetRules = getNumberTargetRules();
            for(int j=0; j< numTargetRules; j++)
            {
               const BTargetRule* pRule = getTargetRule(j);
               if(pRule->mActionID == pProtoAction->getID())
               {
                  abiltyID = pRule->mAbilityID;  

                  if(pRule->mSquadMode == BSquadAI::cModeLockdown)
                  {
                     isLockDownRule = true;
                  }
                  if(pRule->mSquadMode == BSquadAI::cModeCover)
                  {
                     isCoverRule = true;
                  }    
                  if(pRule->mSquadMode == BSquadAI::cModeCarryingObject)
                  {
                     isCarryingObject = true;
                  }
                  break;
               }

            }

            //Is this a slow recharging abilty
            if(abiltyID != -1 || pProtoAction->getUseDPSasDPA())
            {
               //todo find real recharge rate from abilty
               //todo2 add coeficient for how much lumped damage changes things for us
               dps = dps / 25.0f;
            }

            if(pProtoAction->getFlagDisabled())
            {
               continue;
            }
            //if(pProtoAction->canFindBetterAction())
            //{
            //   //There is one situation (wolverine) where we may want to discout the damage here.
            //   continue;
            //}
            if(isLockDownRule || isCoverRule || isCarryingObject)
            {
               continue;   //todo find a way to account for these bonuses..?
                           // how about a <1 multiplier for each one
                           // or a custom multiplier?
            }

            if(actionType == BAction::cActionTypeUnitRangedAttack)
            {
               checkRules = true;
            }
            if(actionType == BAction::cActionTypeUnitSecondaryTurretAttack)
            {
               checkRules = true;
            }
            if(actionType == BAction::cActionTypeUnitSlaveTurretAttack)
            {
               checkPriorities = true;
            }

            for (uint j=0; j<numDamageTypes; j++)
            {
               long damageType = gDatabase.getAttackRatingDamageType(j);

               //rule checking
               BProtoObjectID exemplar = gDatabase.getDamageTypeExemplar(damageType);
               bool allowedByRules = true;
               bool allowedByPriorities = true;
               const BProtoObject* pExemplar = NULL;
               if(exemplar != cInvalidProtoObjectID)
               {               
                  pExemplar = gDatabase.getGenericProtoObject(exemplar);
               }

               if(checkRules && pExemplar)
               {                  
                  allowedByRules = canAttackTargetType(pExemplar, pProtoAction);
               }              
               if(checkPriorities && pExemplar)
               {
                  if( pProtoAction->getTargetPriority(pExemplar) <= 0)
                     allowedByPriorities = false;
               }

               //if not allowed, then no dps
               if(allowedByRules == false || allowedByPriorities == false )
               {
                  continue; //No dps added.
               }

               float ratingMod = pWeaponType->getDamageRating(damageType);
                              
               //RatingMod disabled.  We could use it for the ai later on though
               //pProtoAction->getDamageRatingOverride(damageType, ratingMod);
              
               float dpsMod = dps * pWeaponType->getDamagePercentage(damageType) * ratingMod;
               mAttackRatingDPS[j] += dpsMod;
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
float BTactic::getAttackRatingDPS(BDamageTypeID damageType) const
{
   if (mAttackRatingDPS.getSize() > 0)
   {
      uint index;
      if (gDatabase.getAttackRatingIndex(damageType, index))
         return mAttackRatingDPS[index];
   }
   return 0.0f;
}


//==============================================================================
//==============================================================================
float BTactic::getAttackRatingDPS() const
{
   uint numAttackRatingDPS = mAttackRatingDPS.getSize();
   if (numAttackRatingDPS == 0)
      return (0.0f);
   float totalAttackRatingDPS = 0.0f;
   for (uint i=0; i<numAttackRatingDPS; i++)
      totalAttackRatingDPS += mAttackRatingDPS[i];
   return (totalAttackRatingDPS / static_cast<float>(numAttackRatingDPS));
}


//==============================================================================
// BTactic::getAttackRating
//==============================================================================
float BTactic::getAttackRating(BDamageTypeID damageType) const
{
   if (mAttackRatingDPS.getSize() > 0)
   {
      uint index;
      if (gDatabase.getAttackRatingIndex(damageType, index))
         return gDatabase.getAttackRating(mAttackRatingDPS[index]);
   }
   return 0.0f;
}


//==============================================================================
//==============================================================================
float BTactic::getAttackRating() const
{
   uint numAttackRatingDPS = mAttackRatingDPS.getSize();
   if (numAttackRatingDPS == 0)
      return (0.0f);
   float totalAttackRatingDPS = 0.0f;
   for (uint i=0; i<numAttackRatingDPS; i++)
      totalAttackRatingDPS += mAttackRatingDPS[i];
   return (totalAttackRatingDPS / static_cast<float>(numAttackRatingDPS));
}


//==============================================================================
// BTactic::getTrampleAction
//==============================================================================
BProtoAction* BTactic::getTrampleAction()
{
   if(mpStaticData->mTrampleActionID != -1)
   {
      return getProtoAction(mpStaticData->mTrampleActionID);
   }
   else
      return NULL;
}

//==============================================================================
// BTactic::canAttackTarget
//==============================================================================
bool  BTactic::canAttackTarget(BEntityID targetID, const BProtoAction* pProtoAction)
{
   //-- Run through our target rules, find the rule which applies to the given protoaction, 
   //-- see if that rule allows you to attack the specified targetID
//-- FIXING PREFIX BUG ID 4075
   const BUnit* pUnit = gWorld->getUnit(targetID);
//--
   if(!pUnit)
      return false;

   if (pUnit->getFlagInvulnerable())
      return false;

   long numTargetRules = getNumberTargetRules();
   for(long ruleID=0; ruleID < numTargetRules; ruleID++)
   {
      const BTargetRule* pRule = getTargetRule(ruleID);
      if(!pRule)
         continue;

      const BProtoAction* pRuleProtoAction = getProtoAction(pRule->mActionID);
      if(pRuleProtoAction == pProtoAction)
      {
         //-- We found a target rule referring to our protoaction.

         //-- If there are no object types specified in our rule, then it can attack anything
         if(pRule->mObjectTypes.getNumber() == 0)
            return true;
         //-- Otherwise, we need to see if we're allowed to attack the specified targetID
         for(long i=0; i < pRule->mObjectTypes.getNumber(); i++)
         {         
            const BProtoObject* pTargetProto = pUnit->getProtoObject();
            if(pTargetProto && pTargetProto->isType(pRule->mObjectTypes[i]))
            {
               return true;
            }
         }
         return false;
      }
   }
   return false;
}


//==============================================================================
// BTactic::canAttackGenericTypeTarget
// this is for the ai
//==============================================================================
bool  BTactic::canAttackTargetType(const BProtoObject* pTargetProto, const BProtoAction* pProtoAction)
{

   long numTargetRules = getNumberTargetRules();
   for(long ruleID=0; ruleID < numTargetRules; ruleID++)
   {
      const BTargetRule* pRule = getTargetRule(ruleID);
      if(!pRule)
         continue;

      const BProtoAction* pRuleProtoAction = getProtoAction(pRule->mActionID);
      if(pRuleProtoAction == pProtoAction)
      {
         //-- We found a target rule referring to our protoaction.

         //-- If there are no object types specified in our rule, then it can attack anything
         if(pRule->mObjectTypes.getNumber() == 0)
            return true;
         //-- Otherwise, we need to see if we're allowed to attack the specified targetID
         for(long i=0; i < pRule->mObjectTypes.getNumber(); i++)
         {         
            if(pTargetProto && pTargetProto->isType(pRule->mObjectTypes[i]))
            {
               return true;
            }
         }
         return false;
      }
   }
   return false;
}


//==============================================================================
//==============================================================================
bool BTactic::hasEnabledAttackAction(bool allowSecondary) const
{
   for (uint i=0; i < mProtoActions.getSize(); i++)
   {
      if (mProtoActions[i]->getActionType() == BAction::cActionTypeUnitRangedAttack &&
          !mProtoActions[i]->getFlagDisabled())
      {
         if (!mProtoActions[i]->getMainAttack() && !allowSecondary)
            continue;

         return (true);
      }
   }
   return (false);
}


//==============================================================================
//==============================================================================
BProtoAction* BTactic::getShockwaveAction() const
{
   for (int i = 0; i < mProtoActions.getNumber(); i++)
   {
      BProtoAction* pAction = mProtoActions[i];
      if (pAction && pAction->getFlagShockwaveAction())
         return pAction;
   }
   return NULL;
}

//==============================================================================
//==============================================================================
bool BTactic::save(BStream* pStream, int saveType) const
{
   uint8 weaponCount = (uint8)mWeapons.size();
   GFWRITEVAR(pStream, uint8, weaponCount);
   GFVERIFYCOUNT(weaponCount, 200);

   for (uint8 i=0; i<weaponCount; i++)
   {
      if (!mWeapons[i]->save(pStream, saveType))
         return false;
   }

   uint8 protoActionCount = (uint8)mProtoActions.size();
   GFWRITEVAR(pStream, uint8, protoActionCount);
   GFVERIFYCOUNT(protoActionCount, 200);
   for (uint8 i=0; i<protoActionCount; i++)
   {
      if (!mProtoActions[i]->save(pStream, saveType))
         return false;
   }

   //BSmallDynamicSimArray<float>        mAttackRatingDPS;
   //float                               mOverallRange[BSquadAI::cNumberModes];      

   //bool mFlagOwnStaticData:1;
   //bool mFlagCanAttack:1;
   //bool mFlagCanGather:1;
   //bool mFlagCanBuild:1;
   //bool mFlagCanRepair:1;
   //bool mFlagCanAutoRepair:1;
   //bool mFlagLoaded:1;      

   GFWRITEBITBOOL(pStream, mAnimInfoLoaded);

   GFWRITEMARKER(pStream, cSaveMarkerTactic);
   return true;
}

//==============================================================================
//==============================================================================
bool BTactic::load(BStream* pStream, int saveType, BPlayer* pPlayer, BProtoObjectID protoObjectID)
{
   int weaponCount;
   GFREADVAL(pStream, uint8, int, weaponCount);
   GFVERIFYCOUNT(weaponCount, 200);

   BWeapon tempWeapon;
   for (int i=0; i<weaponCount; i++)
   {
      int weaponID = gSaveGame.getWeaponID(protoObjectID, i);
      if (weaponID != -1)
      {
         if (!mWeapons[weaponID]->load(pStream, saveType))
            return false;
      }
      else
      {
         if (!tempWeapon.load(pStream, saveType))
            return false;
      }
   }

   int protoActionCount;
   GFREADVAL(pStream, uint8, int, protoActionCount);
   GFVERIFYCOUNT(protoActionCount, 200);
   BProtoAction tempProtoAction;
   for (int i=0; i<protoActionCount; i++)
   {
      int protoActionID = gSaveGame.getProtoActionID(protoObjectID, i);
      if (protoActionID != -1)
      {
         if (!mProtoActions[protoActionID]->load(pStream, saveType))
            return false;
      }
      else
      {
         if (!tempProtoAction.load(pStream, saveType))
            return false;
      }
   }

   if (mGameFileVersion >= 2)
      GFREADBITBOOL(pStream, mAnimInfoLoaded);

   GFREADMARKER(pStream, cSaveMarkerTactic);

   updateOverallRanges();
   updateAttackRatings(pPlayer);
   return true;
}
