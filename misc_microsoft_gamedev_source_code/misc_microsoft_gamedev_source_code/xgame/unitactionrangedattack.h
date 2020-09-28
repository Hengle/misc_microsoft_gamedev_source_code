//==============================================================================
// unitactionrangedattack.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "objectmanager.h"

class BObject;
class BUnit;
class BUnitActionSecondaryTurretAttack;
class BWeapon;
class BVisualItem;

//==============================================================================
//==============================================================================
class BUnitActionRangeAttackScanResult
{
   public:
      //Constructors and Destructor.
      BUnitActionRangeAttackScanResult( void ) {}
      BUnitActionRangeAttackScanResult( BUnit* pTarget, BVector position, float priority, float dot ) : mpTarget(pTarget), mPosition(position), mPriority(priority), mDotProduct(dot) {}
      ~BUnitActionRangeAttackScanResult( void ) {}

      //Target
      BUnit*                     getTarget( void ) const { return(mpTarget); }
      void                       setTarget( BUnit* v ) { mpTarget=v; }
      //Target Position
      BVector                    getTargetPosition( void ) const { return(mPosition); }
      void                       setTargetPosition( BVector v ) { mPosition=v; }
      //Priority
      float                      getPriority( void ) const { return(mPriority); }
      void                       setPriority( float v ) { mPriority=v; }
      //DotProduct
      float                      getDotProduct( void ) const { return(mDotProduct); }
      void                       setDotProduct( float v ) { mDotProduct=v; }

   protected:

      BUnit*                     mpTarget;
      BVector                    mPosition;
      float                      mPriority;
      float                      mDotProduct;
};


//==============================================================================
//==============================================================================
class BUnitActionRangedAttack : public BAction
{
   public:
      BUnitActionRangedAttack() { }
      virtual ~BUnitActionRangedAttack() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      virtual void               setHitZoneIndex(long index) { mHitZoneIndex=index; }
      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);
      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;      

      bool                       hasEnoughAmmoForFullAttack();    

      void                       changeTarget(const BSimTarget* pNewTarget, BUnitOppID oppID, BSimOrder* pOrder);

      // Launch
      virtual const BVector      getLaunchLocation() const;

      virtual bool               attemptToRemove();

      void                       setFlagCompleteWhenDoneAttacking(bool v)  { mFlagCompleteWhenDoneAttacking = v; }

      //Add block pool
      DECLARE_FREELIST(BUnitActionRangedAttack, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

      bool                       canHitTarget();

   protected:
      bool                       canStrafeToTarget();
      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      void                       playAttackAnimation();
      void                       playIdleAnimation(bool force=false);      

      bool                       doAttack(long attachmentHandle, long boneHandle);
      void                       doMeleeAttack(long attachmentHandle, long boneHandle);
      void                       doProjectileAttack(long attachmentHandle, long boneHandle);
      void                       doProjectileAttack(BObjectCreateParms& parms, const BEntityID startID, const BEntityID targetID);
      void                       doBeamAttack(long attachmentHandle, long boneHandle);
      void                       doKnockbackAttack(long attachmentHandle, long boneHandle);

      bool                       validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const;
      bool                       validateHardpoint() const;
      BVector                    getOrientation() const;
      void                       updateTargetingLead();
      void                       cacheYawHardpointMatrix();
      bool                       updateHardpointOrientation(float elapsed);
      bool                       updateOrientation(float elasped, bool& turning);
      bool                       getHardpointPosition(BVector& pos) const;
      void                       getLaunchPosition(BUnit* pUnit, long attachmentHandle, long boneHandle, BVector* position, BVector* forward=NULL, BVector* right=NULL) const;
      void                       getTargetPosition(BSimTarget target, BVector& targetPosition, bool& validHitZone, BVector& targetOffset, bool targetGround=false);
      void                       updateVisualAmmo();
      bool                       checkForBetterProtoAction();
      void                       updateDamageBank(float elapsed);
      bool                       addAttackCooldown(const BProtoAction* pProtoAction, BUnit* pUnit);
      bool                       addPreAttackCooldown(const BProtoAction* pProtoAction, BUnit* pUnit);

      bool                       scanForTarget();
      void                       getTurretPositionForward(BVector &position, BVector &forward) const;
      BVector                    applyGravityOffset(const BUnit *pTarget, BVector targetPosition) const;
      bool                       canOrientToTargetPosition(BVector targetPosition) const;
      BVector                    getTargetStrafeSpot();

      void                       stopStrafing();
      void                       startStrafing();

      void                       updateBeam(float elapsed);
      void                       applyStasisEffect();

      static int                 entityDistanceSort(const void* pA, const void* pB);

      // pickup object functions
      bool                       canPickup(const BUnit* pUnit);
      bool                       pickupObject();
      bool                       tearAttachmentOffTarget();
      BVisualItem*               findNearestThrowableAttachment(BObject* pObject, const BVector& positionWorldSpace);
      bool                       tearPartOffTarget();
      bool                       findCarryObject();
      BObject*                   createCarryObject(const BObject* pSourceObject, BVisualItem* pSourceVisual, long& meshPickupBone, BVector& centerOfMassOffset);

      void                       changeBeamState( BProjectile *projectile, bool visible, bool setDamage=false, float damage = 0.0f);

      bool                       tryFatality();
      bool                       getSimTargetPosition(BVector &targetPosition, bool &isMoving);

      BMatrix                    mCachedYawHardpointMatrix;

      BSimTarget                 mTarget;
      BVector                    mTargetingLead;

      BVector                    mStrafingSpot;
      BVector                    mStrafingDirection;
      float                      mStrafingDirectionTimer;
      float                      mStrafingSpotStuckTimer;
      float                      mBeamDPSRampTimer;
      BEntityID                  mBeamProjectileID;

      BUnitOppID                 mOppID;
      BUnitOppID                 mMoveOppID;
      BUnitOppID                 mReloadOppID;
      long                       mLaunchPointHandle;
      long                       mAttachmentHandle;
      long                       mBoneHandle;
      long                       mHitZoneIndex;      
      float                      mTargetDPS;
      float                      mAddedBaseDPS;
      float                      mSelfHealAmount;
      
      int                        mSecondaryTurretAttackID;
      int                        mAltSecondaryTurretAttackID;

      BActionState               mFutureState;

      BVector                    mInitialTargetPos;
      BVector                    mFacingPos;

      // Halwes - 9/30/2008 - Hack for Scn07 Scarab custom beams.
      BEntityID                  mTargetBeamProjectileID;
      BEntityID                  mKillBeamProjectileID;
      BProtoObjectID             mTargetBeamProtoProjectile;
      BProtoObjectID             mKillBeamProtoProjectile;
      float                      mKillBeamDamage;
      
      DWORD                      mLastLOSValidationTime;

      bool                       mFlagAttacking:1;
      bool                       mFlagHasAttacked:1;
      bool                       mFlagMelee:1;
      bool                       mFlagProjectile:1;
      bool                       mFlagBeam:1;
      bool                       mFlagStrafing:1;
      bool                       mFlagStrafingDirection:1;
      bool                       mFlagStasisApplied:1;
      bool                       mFlagStasisDrainApplied:1;
      bool                       mFlagStasisBombApplied:1;
      bool                       mFlagKnockback:1;
      bool                       mMissedTentacleAttack:1;
      bool                       mFlagForceUnitToFace:1;
      bool                       mFlagYawHardpointMatrixCached:1;
      bool                       mFlagCompleteWhenDoneAttacking:1;

      // Temp sorting stuff
      static BDynamicSimArray<BUnitActionRangeAttackScanResult>   mTempScanResults;
      static BDynamicSimUIntArray                                 mTempScanOrder;
};
