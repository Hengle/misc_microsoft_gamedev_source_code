
//==============================================================================
// projectile.h
//
// Look! Up in the sky. It's a bird.  It's a plane...
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once


// Includes
#include "object.h"
#include "gamefilemacros.h"


// Forward Declarations
class BUnit;
class BProtoAction;
class IDamageInfo;
class BObjectCreateParms;
class BPerturbanceData;
class BStickData;


//==============================================================================
// BProjectile
//==============================================================================
class BProjectile : public BObject
{
public:

   enum
   {
      cStateFlying = 0,
      cStatePhysicsTumbling,
      cStateSticked,
      cStateRest,
   };


   BProjectile() : mpPerturbanceData(NULL), mpStickData(NULL) {};
   virtual                 ~BProjectile();

   //-- overrides
   virtual void            init(void);
   virtual void            destroy(void);
   virtual void            kill(bool bKillImmediately);
  
   virtual bool            initFromProtoObject(const BProtoObject* pProto, BObjectCreateParms& parms);

   virtual bool            update(float elapsedTime);  

   virtual void            notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

   inline BEntityID        getTargetObjectID() const { return mTargetObjectID; }
   inline void             setTargetObjectID(BEntityID id) { mTargetObjectID = id; }
   const BVector           getTargetLocation() const  { return mTargetLocation; }
   void                    setTargetLocation(const BVector location) { mTargetLocation = location; }

   void                    setDamage( float dmg )    { mDamage = dmg;  }
   float                   getDamage( void ) const   { return mDamage; }

   void                    setGravity( float g )    { mGravity = g;  }
   float                   getGravity( void ) const { return mGravity; }

   void                    setTargetOffset(BVector targetOffset)     { mTargetOffset = targetOffset; }
   BVector                 getTargetOffset(void) const               { return mTargetOffset; }

   void                    setDamageInfo(IDamageInfo *damageInfo){mpDamageInfo = damageInfo;}

   const BProtoAction*     getProtoAction() const { return mpProtoAction; }
   void                    setProtoAction(const BProtoAction* pProtoAction) { mpProtoAction = pProtoAction; }

   inline void             setHitZoneIndex( long hz ){ mHitZoneIndex = hz; }
   inline long             getHitZoneIndex(){ return( mHitZoneIndex ); }

   inline void             setAttachmentHandle( long handle ) {mAttachmentHandle = handle; }
   long                    getAttachmentHandle() { return mAttachmentHandle; }
   inline void             setLaunchPointHandle( long handle ) {mLaunchPointHandle = handle; }
   long                    getLaunchPointHandle() { return mLaunchPointHandle; }
   inline void             setBoneHandle( long handle ) {mBoneHandle = handle; }
   long                    getBoneHandle() { return mBoneHandle; }
         
   bool                    stickToTarget(BVector startPos, BUnit* pUnit);
   void                    deflect();

   void                    setOwningPowerID(BPowerID id) { mOwningPowerID = id; }

   BEntityID               getBeamHeadID() { return (mBeamHeadID); }
   BEntityID               getBeamTailID() { return (mBeamTailID); }

   //--Flags
   bool                    getFlagIsAffectedByGravity() const { return(mFlagIsAffectedByGravity); }
   void                    setFlagIsAffectedByGravity(bool v) { mFlagIsAffectedByGravity=v; }
   bool                    getFlagTrackingDelay() const { return(mFlagTrackingDelay); }
   void                    setFlagTrackingDelay(bool v) { mFlagTrackingDelay=v; }
   bool                    getFlagTracking() const { return(mFlagTracking); }
   void                    setFlagTracking(bool v) { mFlagTracking=v; }
   bool                    getFlagPerturbed() const { return(mFlagPerturbed); }
   void                    setFlagPerturbed(bool v) { mFlagPerturbed=v; }
   bool                    getFlagTumbling() const { return(mFlagTumbling); }
   void                    setFlagTumbling(bool v) { mFlagTumbling=v; }
   bool                    getFlagDoLogging() const { return(mFlagDoLogging); }
   void                    setFlagDoLogging(bool v) { mFlagDoLogging=v; }
   bool                    getFlagBeam() const { return mFlagBeam; }
   void                    setFlagBeam(bool v) { mFlagBeam=v; }
   bool                    getFlagBeamCollideWithUnits() const { return mFlagBeamCollideWithUnits; }
   void                    setFlagBeamCollideWithUnits(bool v) { mFlagBeamCollideWithUnits=v; }
   bool                    getFlagBeamCollideWithTerrain() const { return mFlagBeamCollideWithTerrain; }
   void                    setFlagBeamCollideWithTerrain(bool v) { mFlagBeamCollideWithTerrain=v; }
   bool                    getFlagCollidesWithAllUnits() const { return mFlagCollidesWithAllUnits; }
   void                    setFlagCollidesWithAllUnits(bool v) { mFlagCollidesWithAllUnits=v; }
   bool                    getFlagClearedLauncher() const { return (mFlagClearedLauncher); }
   void                    setFlagClearedLauncher(bool v) { mFlagClearedLauncher = v;}
   bool                    getFlagInterceptDist() const { return (mFlagInterceptDist); }
   void                    setFlagInterceptDist(bool v) { mFlagInterceptDist = v; }
   bool                    getFlagExplodeOnTimer() const { return(mFlagExplodeOnTimer); }
   void                    setFlagExplodeOnTimer(bool v) { mFlagExplodeOnTimer=v; }
   bool                    getFlagExpireOnTimer() const { return(mFlagExpireOnTimer); }
   void                    setFlagExpireOnTimer(bool v) { mFlagExpireOnTimer=v; }
   bool                    getFlagIsSticky() const { return(mFlagIsSticky); }
   void                    setFlagIsSticky(bool v) { mFlagIsSticky=v; }
   bool                    getFlagDetonate() const { return(mFlagDetonate); }
   void                    setFlagDetonate(bool v) { mFlagDetonate=v; }
   bool                    getFlagIsNeedler() const { return(mFlagIsNeedler); }
   void                    setFlagIsNeedler(bool v) { mFlagIsNeedler=v; }
   bool                    getFlagTargetAlreadyDazed() const { return(mFlagTargetAlreadyDazed); }
   void                    setFlagTargetAlreadyDazed(bool v) { mFlagTargetAlreadyDazed=v; }
   bool                    getFlagCheckedForDeflect() const { return(mFlagCheckedForDeflect); }
   void                    setFlagCheckedForDeflect(bool v) { mFlagCheckedForDeflect=v; }
   bool                    getFlagSelfDamage() const { return(mFlagSelfDamage); }
   void                    setFlagSelfDamage(bool v) { mFlagSelfDamage=v; }
   bool                    getFlagAirBurst() const { return(mFlagAirBurst); }
   void                    setFlagAirBurst(bool v) { mFlagAirBurst=v; }
   bool                    getFlagHideOnImpact() const { return(mFlagHideOnImpact); }
   void                    setFlagHideOnImpact(bool v) { mFlagHideOnImpact=v; }
   bool                    getFlagStaticPosition() const { return(mFlagStaticPosition); }
   void                    setFlagStaticPosition(bool v) { mFlagStaticPosition=v; }
   bool                    getFlagFromLeaderPower() const { return(mFlagFromLeaderPower); }
   void                    setFlagFromLeaderPower(bool v) { mFlagFromLeaderPower=v; }
   
   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);
   virtual bool postLoad(int saveType);

protected:

   virtual void            updateVisibleLists(void);

   float                   doDamage(bool &dead, BEntityID unitHit, BEntityIDArray* pKilledUnits = NULL, BEntityIDArray* pDamagedUnits = NULL);
   void                    createImpactEffect(void);
   void                    adjustTargetPosition();
   void                    getBeamLaunchPosition(BVector* position, BMatrix* pMatrix) const;
   void                    launchTriggerScript(BEntityID unitHitID, BVector hitLocation) const;

   XMFINLINE BEntityID     getStickedToUnitID() const;

   //-- Beam Functions
   BEntityID               initBeamFromProtoObject(long protoObjectID, BVector pos);
   void                    updateBeamMatrices();        
   bool                    updateBeam(float elapsedTime);        

   void                    initTumbleVector();

   void                    attemptFlares();

   bool                    fastCheckHitTerrain(const BVector& startPt, const BVector& endPt, BVector& intersectionPt, bool onlyHeightApproximation = false);

   // Important: Order all member variables by largest to smallest! Anything that requires 16-byte alignment should appear first.
   // Use "bool mBlah : 1;" instead of "bool bBlah;"
   // Use chars and shorts instead of longs or ints when possible.
   
   BVector                 mTumblingVector;              // 16 bytes
   BVector                 mTargetOffset;                // 16 bytes
   BVector                 mInitialProjectilePosition;   // 16 bytes
   BVector                 mInitialTargetPosition;       // 16 bytes
   
   BVector                 mTargetLocation;              // 16 bytes
   BEntityID               mTargetObjectID;              // 4 bytes
   
   BEntityID               mBeamHeadID;                  // 4 bytes
   BEntityID               mBeamTailID;                  // 4 bytes

   BEntityID               mDeflectingUnitID;            // 4 bytes
   BEntityID               mUnitHitID;                   // 4 bytes

   float                   mDamage;                      // 4 bytes
   IDamageInfo*            mpDamageInfo;                 // 4 bytes
   const BProtoAction*     mpProtoAction;                // 4 bytes
   float                   mGravity;                     // 4 bytes
   float                   mStartingVelocity;            // 4 bytes
   float                   mFuel;                        // 4 bytes
   float                   mAcceleration;                // 4 bytes
   DWORD                   mTrackingStartTime;           // 4 bytes
   float                   mTurnRate;                    // 4 bytes
   BPerturbanceData*       mpPerturbanceData;            // 4 bytes
   BStickData*             mpStickData;                  // 4 bytes
   float                   mActiveScanChance;            // 4 bytes
   float                   mActiveScanRadiusScale;       // 4 bytes
   float                   mFollowGroundHeight;          // 4 bytes
   long                    mHitZoneIndex;                // 4 bytes

   // Used for beams right now but we may want to use these for real projectiles because
   // their positions are not in sync with the animation of their owner
   long                    mLaunchPointHandle;           // 4 bytes
   long                    mAttachmentHandle;            // 4 bytes
   long                    mBoneHandle;                  // 4 bytes

   long                    mMotionState;                 // 4 bytes

   BPowerID                mOwningPowerID;               // 2 bytes

   byte                    mSurfaceImpactType;           // 1 byte
   byte                    mBeamDecalCreateStall;        // 1 byte

   float                   mApplicationTimer;            // 4 bytes
   float                   mReapplicationTimer;          // 4 bytes

   //--Flags   
   bool                    mFlagIsAffectedByGravity:1;   // 1 byte   (1/8)
   bool                    mFlagTrackingDelay:1;         //          (2/8)
   bool                    mFlagTracking:1;              //          (3/8)
   bool                    mFlagPerturbed:1;             //          (4/8)
   bool                    mFlagTumbling:1;              //          (5/8)
   bool                    mFlagDoLogging:1;             //          (6/8)
   bool                    mFlagBeam:1;                  //          (7/8)
   bool                    mPerturbOnce:1;               //          (8/8)   

   bool                    mFlagClearedLauncher:1;       // 1 byte   (1/8)
   bool                    mFlagInterceptDist:1;         //          (2/8)
   bool                    mFlagTestFuel:1;              //          (3/8)
   bool                    mFlagCollidesWithAllUnits:1;  //          (4/8)
   bool                    mFlagExplodeOnTimer:1;        //          (5/8)
   bool                    mFlagExpireOnTimer:1;         //          (6/8)
   bool                    mFlagIsSticky:1;              //          (7/8)
   bool                    mFlagDetonate:1;              //          (8/8)

   bool                    mFlagIsNeedler:1;             // 1 byte   (1/8)
   bool                    mFlagSingleStick:1;           //          (2/8)
   bool                    mFlagAddedStickRef:1;         //          (3/8)
   bool                    mFlagTargetAlreadyDazed:1;    //          (4/8)
   bool                    mFlagCheckedForDeflect:1;     //          (5/8)
   bool                    mFlagDeflected:1;             //          (6/8)
   bool                    mFlagSelfDamage:1;            //          (7/8)
   bool                    mFlagAirBurst:1;              //          (8/8)

   bool                    mFlagHideOnImpact:1;          // 1 byte   (1/8)
   bool                    mFlagStaticPosition:1;        //          (2/8)
   bool                    mFlagFromLeaderPower:1;       //          (3/8)
   bool                    mFlagIngoreTargetCollisions:1;//          (4/8)
   bool                    mFlagBeamCollideWithUnits:1;  //          (5/8)
   bool                    mFlagBeamCollideWithTerrain:1;//          (6/8)
};

//==============================================================================
// BPerturbanceData
//==============================================================================
class BPerturbanceData
{
   public:
      BPerturbanceData() {};
      virtual ~BPerturbanceData() {};

      XMFINLINE void                   startPerturbance(bool perturbOnce, float velocityRatio);
      XMFINLINE bool                   updatePerturbance(float elapsedTime, BVector& perturbance);

      //IPoolable Methods
      virtual void                     onAcquire() { }
      virtual void                     onRelease() { }
      DECLARE_FREELIST(BPerturbanceData, 4);

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      // Data
      BVector                          mPerturbanceVector;           // 16 bytes
      float                            mPerturbanceChance;           // 4 bytes
      float                            mPerturbanceVelocity;         // 4 bytes
      float                            mPerturbanceMinTime;          // 4 bytes
      float                            mPerturbanceMaxTime;          // 4 bytes
      float                            mPerturbanceDuration;         // 4 bytes
      float                            mPerturbanceTimer;            // 4 bytes
      float                            mInitialPerturbanceVelocity;  // 4 bytes
      float                            mInitialPerturbanceMinTime;   // 4 bytes
      float                            mInitialPerturbanceMaxTime;   // 4 bytes
};

//==============================================================================
// BStickData
//==============================================================================
class BStickData
{
   public:
      BStickData() {};
      virtual ~BStickData() {};

      //IPoolable Methods
      virtual void                     onAcquire() { }
      virtual void                     onRelease() { }
      DECLARE_FREELIST(BStickData, 4);

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      // Data
      BVector                          mStickedModelSpacePosition;   // 16 bytes
      BVector                          mStickedModelSpaceDirection;  // 16 bytes
      BEntityID                        mStickedToUnitID;             // 4 bytes
      long                             mStickedToAttachmentHandle;   // 4 bytes
      long                             mStickedToBoneHandle;         // 4 bytes
};



//==============================================================================
// Inline functions
//==============================================================================

//==============================================================================
//==============================================================================
XMFINLINE BEntityID BProjectile::getStickedToUnitID() const
{
   return mpStickData ? mpStickData->mStickedToUnitID : cInvalidObjectID;
}

//==============================================================================
//==============================================================================
XMFINLINE void BPerturbanceData::startPerturbance(bool perturbOnce, float velocityRatio)
{
   mPerturbanceTimer = 0.0f;
   if (perturbOnce)
   {               
      mPerturbanceDuration = Math::Lerp(mInitialPerturbanceMinTime, mInitialPerturbanceMaxTime, getRandDistribution(cSimRand));
      mPerturbanceVector.set(getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f));               
      mPerturbanceVector.scale(mInitialPerturbanceVelocity / mPerturbanceVector.length());               
   }
   else
   {               
      mPerturbanceDuration = Math::Lerp(mPerturbanceMinTime, mPerturbanceMaxTime, getRandDistribution(cSimRand));
      mPerturbanceVector.set(getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f), getRandRangeFloat(cSimRand, -1.0f, 1.0f));
      mPerturbanceVector.scale((mPerturbanceVelocity * velocityRatio) / mPerturbanceVector.length());
   }
}

//==============================================================================
//==============================================================================
XMFINLINE bool BPerturbanceData::updatePerturbance(float elapsedTime, BVector& perturbance)
{
   bool done = false;
   mPerturbanceTimer += elapsedTime;
   if (mPerturbanceTimer >= mPerturbanceDuration)
   {
      mPerturbanceTimer = mPerturbanceDuration;
      done = true;
   }

   perturbance = mPerturbanceVector * XMScalarSin(XM_PI * mPerturbanceTimer / mPerturbanceDuration);

   return done;
}
