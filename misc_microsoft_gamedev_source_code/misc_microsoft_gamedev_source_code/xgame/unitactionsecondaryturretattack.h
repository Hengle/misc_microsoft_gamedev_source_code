//==============================================================================
// unitactionsecondaryturretattack.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;



//==============================================================================
//==============================================================================
class BUnitActionSecondaryTurretScanResult
{
   public:
      //Constructors and Destructor.
      BUnitActionSecondaryTurretScanResult( void ) {}
      BUnitActionSecondaryTurretScanResult( BUnit* pTarget, BVector position, float priority, float dot ) : mpTarget(pTarget), mPosition(position), mPriority(priority), mDotProduct(dot) {}
      ~BUnitActionSecondaryTurretScanResult( void ) {}

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
class BUnitActionSecondaryTurretAttack : public BAction
{
   public:
      BUnitActionSecondaryTurretAttack() { }
      virtual ~BUnitActionSecondaryTurretAttack() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);

      void                       setPreferredTarget(BSimTarget target) { mPreferredTarget = target; } 

      //Add block pool
      DECLARE_FREELIST(BUnitActionSecondaryTurretAttack, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const;
      bool                       validateTarget(BEntityID targetID) const;
      bool                       validateControllers() const;
      bool                       validateHardpoint() const;
      //bool                       validateRange() const;
      //bool                       validateTargetRange(BUnit* target) const;
      bool                       grabControllers();
      void                       releaseControllers();
      void                       scanForTarget();
      void                       trackTarget(float elapsed);
      void                       updateTargetingLead();
      bool                       updateTurretRotation(float elapsed);
      BVector                    applyGravityOffset(const BUnit *pTarget, BVector targetPosition, BVector hardpointPosition, const BMatrix& hardpointMatrix) const;
      bool                       canOrientToTargetPosition(BVector targetPosition) const;
      void                       getTurretPositionForward(BVector &position, BVector &forward) const;
      bool                       canHitTarget() const;
      void                       startAttacking();
      void                       stopAttacking();
      void                       doProjectileAttack(long attachmentHandle, long boneHandle);
      void                       getLaunchPosition(const BUnit* pUnit, long attachmentHandle, long boneHandle, BVector* position, BVector* forward, BVector* right) const;
      void                       getTargetPosition(const BUnit* pTarget, BVector& targetPosition, BVector& targetOffset, bool targetGround) const;
      float                      getDamageAmount(const BUnit* pUnit, const BUnit* pTarget) const;
      bool                       isBlocked() const;
      void                       pauseAltSecondary(bool pause);      
      bool                       shouldSwitchToPrimaryTarget();
      bool                       doesParentOrderAllowAttacking();

      BSimTarget                 mTarget;
      BSimTarget                 mPreferredTarget;
      BVector                    mTargetingLead;
      long                       mOtherSecondaryID;
      long                       mScanTimeLeft;
      DWORD                      mLastLOSValidationTime;

      bool                       mFirstUpdate:1;
      bool                       mIsAttacking:1;
      bool                       mOtherSecondaryToldToWait:1;
      bool                       mWaitingForLockdown:1;


      // Temp sorting stuff
      static BDynamicSimArray<BUnitActionSecondaryTurretScanResult>     mTempScanResults;
      static BDynamicSimUIntArray                                       mTempScanOrder;
};
