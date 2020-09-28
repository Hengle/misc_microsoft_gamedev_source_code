//==============================================================================
// object.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#pragma once

// Includes
#include "entity.h"
#include "boundingBox.h"
#include "player.h"
#include "protovisual.h"
#include "TerrainSimRep.h"
#include "objectanimationstate.h"
#include "math\generalVector.h"
#include "..\xphysics\physicslistener.h"
#include "meshEffectTextures.h"


//#define BOBJECT_DEBUG_VISIBLEMAP

// Forward declarations
class BVisual;
class BPhysicsObject;
class BTactic;
class BHardpoint;
class IDamageInfo;
class BDamage;
class BObjectCreateParms;
struct granny_model_instance;

typedef void (*BObjectOverrideRenderCallback)(BObject* pObject, BVisualRenderAttributes* pRenderAttributes, DWORD subUpdate, DWORD subUpdatesPerUpdate, float elapsedSubUpdateTime);

//==============================================================================
// Debugging
#ifndef BUILD_FINAL
extern BEntityID sDebugObjectTempID;
void dbgObjectInternal(const char *format, ...);
void dbgObjectInternalTempID(const char *format, ...);

#define debugObject dbgObjectInternal
#else
#define debugObject __noop
#endif

//==============================================================================
enum 
{
   cExplodeTypeNormal,
   cExplodeTypeMethane,
};

//==============================================================================
enum BAdditionalTextureRenderType
{
   cATAdditive,
   cATLerp,

   cATMax
};

struct BAdditionalTexture
{
public: 
   BAdditionalTexture();

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BAdditionalTextureRenderType      RenderType;
   BMeshEffectTextures::BTextureType Texture;
   BVec2                             TexUVOfs;
   float                             TexUVScale;
   float                             TexInten;
   float                             TexScrollSpeed;
   float                             TexTimeout;
   DWORD                             TexStartTime;

   bool                              ModulateOffset:1;
   bool                              ModulateIntensity:1;
   bool                              ShouldBeCopied:1;
   bool                              TexClamp:1;
   bool                              TexScrollLoop:1;
};
typedef BDynamicSimArray<BAdditionalTexture,4,BDynamicArrayNoConstructOptions> BAdditionalTextureArray;

//==============================================================================
// BObjectAttachment
//==============================================================================
class BObjectAttachment
{
public:
   BObjectAttachment()  { mAttachmentObjectID = cInvalidObjectID; mToBoneHandle = -1; mFromBoneHandle = -1; mIsUnitAttachment = false; mUseOffset = false;}
   BObjectAttachment(BEntityID attachmentObjectID, long toBoneHandle, long fromBoneHandle, bool isUnitAttachment)  { mOffset.makeIdentity(); mAttachmentObjectID = attachmentObjectID; mToBoneHandle = toBoneHandle; mFromBoneHandle = fromBoneHandle; mIsUnitAttachment = isUnitAttachment; mUseOffset = false; }
   ~BObjectAttachment() {};

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BMatrix     mOffset;
   BEntityID   mAttachmentObjectID;
   long        mToBoneHandle;
   long        mFromBoneHandle;
   bool        mIsUnitAttachment:1;
   bool        mUseOffset:1;
};

// BObjectAttachmentArray
typedef BDynamicSimArray<BObjectAttachment,4,BDynamicArrayNoConstructOptions> BObjectAttachmentArray;

//==============================================================================
// BHardpointState
//==============================================================================
class BHardpointState
{
public:
   BHardpointState( void ) : mOwnerAction(-1), mAllowAutoCentering(false), mAutoCenteringTimer(0.0f), mYawSoundActivationTimer(0.0f), mYawSound(false), mYawSoundPlaying(false), mPitchSound(false), mPitchSoundPlaying(false), mOppID(BUnitOpp::cInvalidID), mSecondaryTurretScanToken(false) { }
   ~BHardpointState( void ) {};

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   long  mOwnerAction;
   float mAutoCenteringTimer;
   float mYawSoundActivationTimer;
   float mPitchSoundActivationTimer;
   BUnitOppID mOppID;
   bool  mAllowAutoCentering : 1;
   bool  mYawSound : 1;
   bool  mYawSoundPlaying : 1;
   bool  mPitchSound : 1;
   bool  mPitchSoundPlaying : 1;
   bool  mSecondaryTurretScanToken : 1;
};

typedef BDynamicSimArray<BHardpointState, 4> BHardpointStateArray;

//==============================================================================
// BObject
//==============================================================================
class BObject : public BEntity
{
   public:     

                              BObject();
      virtual                 ~BObject();

      void                    updateGrannySync(bool doSyncChecks);
      virtual bool            update(float elapsedTime);
      virtual bool            updatePreAsync(float elapsedTime);
      virtual bool            updateAsync(float elapsedTime);      
      virtual void            render();

      void                    setID(BEntityID id) { mID=id; }
      BEntityID               getID() const { return mID; }

      virtual void            setProtoID(long protoID);
      virtual long            getProtoID() const { return(mProtoID); }
     
      inline void             getSimXZ(long &x, long &z) const;

      //////////////////////////////////////////////////////////////////////////
      // SLB: Don't call these directly. Use getSimXZ instead.
      long                    getSimX() const { return mSimX; }
      long                    getSimZ() const { return mSimZ; }
      //////////////////////////////////////////////////////////////////////////

      float                   getLOS() const;// { getProtoObject()->getLOS_DO_NOT_CALL_DIRECTLY()}
      long                    getSimLOS() const;// {}      

      virtual void            onRelease();
      virtual void            init();

      void                    clearFlags(void);

      virtual bool            initFromProtoObject(const BProtoObject* pProto, BObjectCreateParms& parms);

      //-- visuals
      bool                    setVisual(long protoVisualIndex, int displayPriority, int64 userData = -1);
      bool                    setVisual(const BVisual* pSource);
      bool                    setVisualPtr(BVisual* pVisual);
      BVisual*                getVisual() const { return mpVisual; }
      BVector                 getVisualCenter() const       { return mBoundingBox.getCenter(); }
      float                   getVisualRadius() const       { return mRadius; }
      const BBoundingBox*     getVisualBoundingBox() const  { return &mBoundingBox; }
      void                    computeVisual();
      void                    computeAnimation();
      void                    computeDopple();
      void                    setHighlightIntensity(float value) { mHighlightIntensity = value; }
      float                   getHighlightIntensity() const { return mHighlightIntensity; }
      virtual void            getWorldMatrix(BMatrix& matrix) const;      
      BVector                 getInterpolatedPosition() const;
      BVector                 getInterpolatedUp() const;
      BVector                 getInterpolatedRight() const;
      BVector                 getInterpolatedForward() const;
      void                    setInterpolationMatrices(const BMatrix &matrix);
      void                    resetSubUpdating();
      DWORD                   getSubUpdateNumber() const { return mSubUpdateNumber; }

      //-- sim bounding box
      BVector                 getSimCenter() const       { return mSimBoundingBox.getCenter(); }
      const BBoundingBox*     getSimBoundingBox() const  { return &mSimBoundingBox; }
      BVector                 getCenterOffset() const    { return mCenterOffset; }
      void                    setCenterOffset(const BVector& offset);
      void                    clearCenterOffset();
      void                    updateSimBoundingBox();

      //-- animation
      long                    getAnimationType(long animationTrack) const;
      void                    setAnimationRate(float rate) { if (Math::fAbs(rate - mAnimationRate) > cFloatCompareEpsilon) { mAnimationRate = rate; mAnimationState.setDirty(); } }
      float                   getAnimationRate() const     { return mAnimationRate; }
      float                   getAnimationDuration(long animationTrack);
      DWORD                   getAnimationDurationDWORD(long animationTrack);
      void                    setAnimationEnabled(bool flag, bool includeVisualItem = false);
      void                    lockAnimation(DWORD lockDuration, bool updateSquadPositionOnAnimationUnlock);
      void                    unlockAnimation();
      bool                    isAnimationLocked() const     { return (getFlagAnimationLocked() || getFlagAnimationDisabled()); }

      virtual long            getIdleAnim() const           { return cAnimTypeIdle; }
      virtual long            getWalkAnim() const           { return cAnimTypeWalk; }
      virtual long            getJogAnim() const            { return cAnimTypeJog; }
      virtual long            getRunAnim() const            { return cAnimTypeRun; }
      virtual long            getDeathAnim() const          { return cAnimTypeDeath; }

      void                    overrideExitAction(const BProtoVisualAnimExitAction* pOverrideExitAction) { mAnimationState.overrideExitAction(pOverrideExitAction); }
      void                    setAnimationState(long state, long animType=-1, bool applyInstantly=false, bool reset=false, long forceAnimID=-1, bool lock=false) { mAnimationState.setState(state, animType, applyInstantly, reset, forceAnimID, lock); }
      long                    getAnimationState( void ) const                    { return mAnimationState.getState(); }
      void                    clearAnimationState( void )                        { mAnimationState.clear(); }
      bool                    isAnimationApplied() const                         { return (mAnimationState.getAnimType() == getAnimationType(0)); }
      void                    startChain(long fromAnimType, long toAnimType);

      virtual void            settle( void );     

      virtual bool            isVisible(BTeamID teamID) const;
      virtual bool            isExplored(BTeamID teamID) const;
      bool                    isVisibleOnScreen() const;
      virtual bool            isDoppled(BTeamID teamID) const           { return (mDoppleBits.isSet(teamID) || hasDoppleObject(teamID)); }

      virtual bool            isVisibleOrDoppled(BTeamID teamID) const  { return (isVisible(teamID) || isDoppled(teamID)); }
      //DCP/SL: Change this to match the way bit vector works (we think).
      bool                    hasDoppleObject(BTeamID teamID) const     { return mDoppleBits.isSet(teamID + 16); }
      void                    setDoppleObject(BTeamID teamID)           { mDoppleBits.set(teamID + 16); }
      void                    clearDoppleObject(BTeamID teamID)         { mDoppleBits.unset(teamID + 16); }

      virtual float           getDesiredVelocity() const;
      virtual float           getMaxVelocity() const;
      virtual float           getReverseSpeed() const;
      virtual float           getAcceleration() const;
      virtual void            kill(bool bKillImmediately);
      virtual void            destroy();

      virtual void            notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

      void                    setLifespan(DWORD lifespan, bool useDeathFadeDelay = false);
      void                    enableLifespan(bool flag);

      void                    enableAlphaFade(bool flag, float duration);
      bool                    isFullyFaded( void ) { return (mCurrentAlphaTime >= mAlphaFadeDuration); }
      float                   getAlphaFadeDuration() const { return mAlphaFadeDuration; }
      float                   getCurrentAlphaTime() const { return mCurrentAlphaTime; }

      void                    generateFloatingTextForResourceGather(long resourceID);

      void                    markLOSOn();
      void                    markLOSOff();
      void                    markLOSUpdate();
      void                    revealOverTime();
      void                    setRevealOverTimePercent(float percent);

      void                    setLOSScalar(float losScalar);
      void                    adjustLOSScalar(float adjust);
      void                    clearLOSScalar(void);

      void                    createCorpseObstruction();

      BEntityID               addAttachment(long protoObjectID, long toBoneHandle = -1, long fromBoneHandle = -1, bool isUnitAttachment = false);
      void                    addAttachment(BObject* pObject, long toBoneHandle = -1, long fromBoneHandle = -1, bool isUnitAttachment = false, bool useOffset = false);
      void                    removeAttachment(BEntityID attachmentID);
      void                    removeAllAttachments();
      void                    removeAttachmentsOfType(long protoObjectID);
      bool                    findAttachment(BEntityID attachmentID) const;
      BObject*                getAttachedToObject() const;
      BObjectAttachment*      getAttachedToObjectAttachment() const;
      bool                    isObjectAttached(BObject* pObj) const;
      bool                    isAttachedToObject(BObject* pObj) const;

      BTactic*                getTactic( void ) const;

      long                    getNumberHardpoints( void ) const;
      const BHardpoint        *getHardpoint( long index ) const;
      bool                    validateHardpoint( long index ) const;
      bool                    getHardpointYawLocation( long index, BVector &vec, BMatrix& matrix, BMatrix* transformedMatrix = NULL ) const;
      bool                    getHardpointPitchLocation( long index, BVector &vec, BMatrix& matrix, BMatrix* transformedMatrix = NULL ) const;
      bool                    getHardpointYawTransform( long index, BMatrix &mat) const;
      bool                    getHardpointPitchTransform( long index, BMatrix &mat) const;      
      bool                    getHardpointYawTargetLocation(long index, BVector& wsTargetLocation) const;

      bool                    canOrientHardpointToWorldPos(long index, BVector wsTargetPos) const;
      bool                    canYawHardpointToWorldPos(long index, BVector wsTargetPos, const BMatrix *pCachedYawHardpointMatrix = NULL) const;
      bool                    canPitchHardpointToWorldPos(long index, BVector wsTargetPos) const;

      bool                    capTargetOrient(const BHardpoint* pHP, BVector &tsTargetDir) const;
      bool                    capTargetYaw(const BHardpoint* pHP, BVector &tsTargetDir) const;
      bool                    capTargetPitch(const BHardpoint* pHP, BVector &tsTargetDir) const;

      bool                    isHardpointOrientedToWorldPos(long index, BVector wsTargetPos, float tolerance, const BMatrix *pCachedYawHardpointMatrix = NULL) const;

      bool                    orientHardpointToWorldPos(long index, BVector wsTargetPos, float time, BVector *pwsActualTargetPos = NULL, const BMatrix *pCachedYawHardpointMatrix = NULL);
      bool                    yawHardpointToWorldPos(long index, BVector wsTargetPos, float time, BVector *pwsActualTargetPos = NULL, const BMatrix *pCachedYawHardpointMatrix = NULL);
      bool                    pitchHardpointToWorldPos(long index, BVector wsTargetPos, float time, BVector *pwsActualTargetPos = NULL);
      void                    pitchHardpointToGoalAngle(long index, float goalAngle, float time);
      bool                    autoCenterHardpoint(int index, float time);
      bool                    updateHardpoints( float elapsed );

      // Interface so hardpoint code can access unit opp stuff in BUnit.
      virtual uint8           getOppPriority(BUnitOppID oppID) const { return 0; }

      bool                    grabHardpoint(long actionID, long index, BUnitOppID oppID);
      bool                    hardpointHasAction(long actionID, long index);
      bool                    canGrabHardpoint(long actionID, long index, BUnitOppID oppID);
      long                    getHardpointController(long index) const;
      bool                    releaseHardpoint( long actionID, long index);
      void                    clearHardpoint(long index);

      BObject*                createPhysicsReplacement();
      virtual void            changeOwner(BPlayerID newPlayerID, BProtoObjectID newPOID = cInvalidProtoObjectID);
      virtual int             getSelectionDecalHandle() const { return -1; };
      virtual bool            isSelectable(BTeamID teamId) const;
      virtual int             getSelectType(BTeamID teamId) const;
      virtual int             getGotoType() const;

      void                    onVisibilityChanged(bool visible, BTeamID teamID);
      void                    onDoppleBitsChanged(bool dopple, BTeamID teamID);

      void                    setExplorationGroup(int16 newGroup);
      int16                   getExplorationGroup() const;

      void                    startExistSound(const BProtoObject* pProto=NULL);
      void                    stopExistSound(BEntityID parentID=cInvalidObjectID);

      bool                    teleport( BVector location, long searchScale = 1 );      

      virtual BPlayerID       getColorPlayerID() const { return (mColorPlayerID != cInvalidPlayerID) ? mColorPlayerID : mPlayerID; }
      void                    setColorPlayerID(BPlayerID newColor) { mColorPlayerID = newColor; }

      //virtual float           damage(BDamage &dmg);
      virtual long            computeObstructionType( void);

      // Icon
      void                    setIconColor(BColor color) { mIconColorSize.x = color.r; mIconColorSize.y = color.g; mIconColorSize.z = color.b; }
      BColor                  getIconColor() const { BColor color; color.r = mIconColorSize.x; color.g = mIconColorSize.y; color.b = mIconColorSize.z; return (color); }
      void                    setIconSize(float size) { mIconColorSize.w = size; }
      float                   getIconSize() const { return (mIconColorSize.w); }

      void                    explode(int explodeType);

      // Tint
      void                    setOverrideTintColor(DWORD color) { mOverrideTint = color; }
      DWORD                   getOverrideTintColor() const { return (mOverrideTint); }
      void                    setOverrideFlashDuration(DWORD duration);
      void                    setOverrideFlashInterval(DWORD interval);

      // UVOffset
      void                    setUV0Offset(float u, float v) { mUVOffsets[0].set(u, v); }
      void                    setUV1Offset(float u, float v) { mUVOffsets[1].set(u, v); }
      void                    setUV2Offset(float u, float v) { mUVOffsets[2].set(u, v); }
      
      static uint             getMaxUVOffsetIndex() { return cMaxUVOffsets; }
      void                    setUVOffset(uint index, const BVec2& ofs) { BDEBUG_ASSERT(index < cMaxUVOffsets); mUVOffsets[index] = ofs; }
      const BVec2&            getUVOffset(uint index) const { BDEBUG_ASSERT(index < cMaxUVOffsets); return mUVOffsets[index]; }

      // MultiframeTextureIndex
      void                    setMultiframeTextureIndex(uint index) { mMultiframeTextureIndex = index; }
      uint                    getMultiframeTextureIndex() const { return mMultiframeTextureIndex; }

      // Visual Variation Index
      int                     getVisualVariationIndex() const { return mVisualVariationIndex;}

      // Override render callback
      void                    setOverrideRenderCallback(BObjectOverrideRenderCallback cb) { mpOverrideRenderCB = cb; }

      BAdditionalTexture*     addAdditionalTexture(const BAdditionalTexture& texture);
      BAdditionalTexture*     getAdditionalTexture(BAdditionalTextureRenderType type);
      void                    removeAdditionalTexture(BAdditionalTextureRenderType type);
      void                    copyAdditionalTextures(const BObject* pSourceObject);

      void                    setTargettingSelection(bool on, float scale = 0.05f, float uOffset = 0.0f, float vOffset = 0.0f, float timeout = -1.0f, float speed = 1.0f, bool clamp = false, bool loop = false, float intensity = 5.0f, DWORD color = cDWORDWhite);

      //-- Flags      
      virtual void            setFlagMoving(bool v) { if (v != mFlagMoving) { mFlagMoving=v; mAnimationState.setDirty(); } }
      bool                    getFlagVisibility() const { return(mFlagVisibility); }
      void                    setFlagVisibility(bool v) { mFlagVisibility=v; }
      bool                    getFlagLOS() const { return(mFlagLOS); }
      void                    setFlagLOS(bool v) { mFlagLOS=v; }      
      bool                    getFlagHasLifespan() const { return(mFlagHasLifespan); }
      void                    setFlagHasLifespan(bool v) { mFlagHasLifespan=v; }
      bool                    getFlagDopples() const { return(mFlagDopples); }
      void                    setFlagDopples(bool v) { mFlagDopples=v; }
      bool                    getFlagIsFading() const { return(mFlagIsFading); }
      void                    setFlagIsFading(bool v) { mFlagIsFading=v; }
      bool                    getFlagAnimationDisabled() const { return(mFlagAnimationDisabled); }
      bool                    getFlagIsRevealer() const { return(mFlagIsRevealer); }      
      void                    setFlagIsRevealer(bool v) { mFlagIsRevealer=v; }
      void                    setFlagDontInterpolate(bool v) { mFlagDontInterpolate=v; }
      bool                    getFlagBlockLOS() const { return(mFlagBlockLOS); }
      void                    setFlagBlockLOS(bool v) { mFlagBlockLOS=v; }
      bool                    getFlagGrayMapDopples() const { return(mFlagGrayMapDopples); }
      void                    setFlagGrayMapDopples(bool v) { mFlagGrayMapDopples=v; }
      bool                    getFlagLOSMarked() const { return(mFlagLOSMarked); }
      void                    setFlagLOSMarked(bool v) { mFlagLOSMarked=v; }
      bool                    getFlagUseLOSScalar() const { return(mFlagUseLOSScalar); }
      void                    setFlagUseLOSScalar(bool v) { mFlagUseLOSScalar=v; }
      bool                    getFlagLOSDirty() const { return(mFlagLOSDirty); }
      void                    setFlagLOSDirty(bool v) { mFlagLOSDirty=v; }
      bool                    getFlagAnimationLocked() const { return(mFlagAnimationLocked); }
      void                    setFlagAnimationLocked(bool v) { mFlagAnimationLocked=v; }
      bool                    getFlagDontLockMovementAnimation() const { return(mFlagDontLockMovementAnimation); }
      void                    setFlagDontLockMovementAnimation(bool v) { mFlagDontLockMovementAnimation=v; }
      bool                    getFlagRemainVisible() const { return(mFlagRemainVisible); }
      void                    setFlagRemainVisible(bool v) { mFlagRemainVisible=v; }
      bool                    getFlagUpdateSquadPositionOnAnimationUnlock() const { return(mFlagUpdateSquadPositionOnAnimationUnlock); }
      void                    setFlagUpdateSquadPositionOnAnimationUnlock(bool v) { mFlagUpdateSquadPositionOnAnimationUnlock=v; }
      bool                    getFlagExistSoundPlaying() const { return(mFlagExistSoundPlaying); }
      void                    setFlagExistSoundPlaying(bool v) { mFlagExistSoundPlaying=v; }
      bool                    getFlagNoUpdate() const { return(mFlagNoUpdate); }
      void                    setFlagNoUpdate(bool v);
      bool                    getFlagSensorLockTagged() const { return(mFlagSensorLockTagged); }
      void                    setFlagSensorLockTagged(bool v) { mFlagSensorLockTagged=v; }
      bool                    getFlagNoReveal() const { return(mFlagNoReveal); }
      void                    setFlagNoReveal(bool v) { mFlagNoReveal=v; }
      bool                    getFlagBeingCaptured() const { return(mFlagBeingCaptured); }
      void                    setFlagBeingCaptured(bool v) { mFlagBeingCaptured=v; }
      bool                    getFlagBuilt() const { return(mFlagBuilt); }
      void                    setFlagBuilt(bool v);
      bool                    getFlagInvulnerable() const { return(mFlagInvulnerable); }
      void                    setFlagInvulnerable(bool v) { mFlagInvulnerable=v; }
      bool                    getFlagVisibleForOwnerOnly() const { return(mFlagVisibleForOwnerOnly); }
      void                    setFlagVisibleForOwnerOnly(bool v) { mFlagVisibleForOwnerOnly=v; }
      bool                    getFlagVisibleToAll() const { return (mFlagVisibleToAll); }
      void                    setFlagVisibleToAll(bool v) { mFlagVisibleToAll = v; }      
      bool                    getFlagNearLayer() const { return (mFlagNearLayer); }
      void                    setFlagNearLayer(bool v);
      bool                    getFlagLODFading() const { return (mFlagLODFading); }
      void                    setFlagLODFading(bool v) { mFlagLODFading = v; }
      bool                    getFlagIKDisabled() const { return mFlagIKDisabled; }
      void                    setFlagIKDisabled(bool v) { mFlagIKDisabled = v; }
      bool                    getFlagOccluded() const { return mFlagOccluded; }
      void                    setFlagOccluded(bool v);
      void                    setFlagFadeOnDeath(bool v) { mFlagFadeOnDeath = v; }
      bool                    getFlagFadeOnDeath() const { return mFlagFadeOnDeath; }
      bool                    getFlagHasTrackMask() const { return mFlagHasTrackMask; }
      void                    setFlagHasTrackMask(bool v) { mFlagHasTrackMask = v; }
      bool                    getFlagObscurable() const { return mFlagObscurable; }
      void                    setFlagObscurable(bool v) { mFlagObscurable=v; }
      bool                    getFlagNoRender() const { return mFlagNoRender; }
      void                    setFlagNoRender(bool v);
      bool                    getFlagTurning() const { return mFlagTurning; }
      void                    setFlagTurning(bool v) { if (mFlagTurning != v) { mFlagTurning=v; mAnimationState.setDirty(); } }
      bool                    getFlagSkipMotionExtraction() const { return mFlagSkipMotionExtraction; }
      void                    setFlagSkipMotionExtraction(bool v) { mFlagSkipMotionExtraction=v; }
      bool                    getFlagOverrideTint() const { return (mFlagOverrideTint); }
      void                    setFlagOverrideTint(bool v) { mFlagOverrideTint = v; }
      bool                    getFlagMotionCollisionChecked() const { return (mFlagMotionCollisionChecked); }
      void                    setFlagMotionCollisionChecked(bool v) { mFlagMotionCollisionChecked = v; }
      bool                    getFlagIsImpactEffect() const { return (mFlagIsImpactEffect); }
      void                    setFlagIsImpactEffect(bool v) { mFlagIsImpactEffect = v; }
      bool                    getFlagDebugRenderAreaAttackRange() const { return (mFlagDebugRenderAreaAttackRange); }
      void                    setFlagDebugRenderAreaAttackRange(bool v) { mFlagDebugRenderAreaAttackRange = v; }
      bool                    getFlagVisibleForTeamOnly() const { return (mFlagVisibleForTeamOnly); }
      void                    setFlagVisibleForTeamOnly(bool v) { mFlagVisibleForTeamOnly = v; }
      bool                    getFlagDontAutoAttackMe() const { return (mFlagDontAutoAttackMe); }
      void                    setFlagDontAutoAttackMe(bool v) { mFlagDontAutoAttackMe = v; }
      bool                    getFlagAlwaysAttackReviveUnits() const { return (mFlagAlwaysAttackReviveUnits); }
      void                    setFlagAlwaysAttackReviveUnits(bool v) { mFlagAlwaysAttackReviveUnits = v; }
      bool                    getFlagNoRenderForOwner() const { return(mFlagNoRenderForOwner); }
      void                    setFlagNoRenderForOwner(bool v) { mFlagNoRenderForOwner=v; }
      bool                    getFlagNoRenderDuringCinematic() const { return(mFlagNoRenderDuringCinematic); }
      void                    setFlagNoRenderDuringCinematic(bool v) { mFlagNoRenderDuringCinematic=v; }
      bool                    getFlagNotDoppleFriendly() const { return(mFlagNotDoppleFriendly); }
      void                    setFlagNotDoppleFriendly(bool v) { mFlagNotDoppleFriendly=v; }
      bool                    getFlagForceVisibilityUpdateNextFrame() const { return (mFlagForceVisibilityUpdateNextFrame); }
      void                    setFlagForceVisibilityUpdateNextFrame(bool v) { mFlagForceVisibilityUpdateNextFrame = v; }
      bool                    getFlagTurningRight() const { return mFlagTurningRight; }
      void                    setFlagTurningRight(bool v) { mFlagTurningRight=v; }
      void                    setFlagIsUnderCinematicControl(bool v) { mFlagIsUnderCinematicControl = v; }
      bool                    getFlagIsUnderCinematicControl() const { return mFlagIsUnderCinematicControl; }
      void                    setFlagNoWorldUpdate(bool v) { mFlagNoWorldUpdate = v; }
      bool                    getFlagNoWorldUpdate() const { return (mFlagNoWorldUpdate); }

      //Used for pulsing the color of the unit when it is being selected
      void                    setSelectionPulseTime( float length, float pulseSpeed = 1.0f ) { mSelectionPulseTime = length; mSelectionPulseSpeed = pulseSpeed; }
      void                    setSelectionFlashTime( float length ) { mSelectionFlashTime = length; }

      bool                    hasAnimation(long animType) const;

      long                    getMoveAnimType(void) const;
      bool                    isMoveAnimType(long animType) const;
      bool                    isTurnAnimType(long animType) const;

      virtual BPlayerID       getCapturePlayerID() const { return (cInvalidPlayerID); }
      bool                    isCapturable(BPlayerID playerID, BEntityID squadID) const;

      void                    updateBoundingBox();

      bool                    getNearestCollision(BVector startPosition, BVector endPosition, bool collideWithTerrain, bool collideWithStaticObjects, bool collideWithMovingObjects, BVector &intersection) const;

      static void             updateGrannyInstanceSyncCallback(granny_model_instance* modelInstance, bool doSyncCheck);
      static void             updateVisualItemSyncCallback(BVisualItem* pVisualItem, bool fullSync, bool doSyncCheck);
      
      void                    makeVisible(BTeamID teamID);

      void                    setTeamSelectionMask(BTeamID teamID, bool selectableByTeam);
      void                    fillTeamSelectionMask(bool selectableByAll);

      static void             getInterpolatedMatrix(const BMatrix &matrix1, const BMatrix &matrix2, BMatrix &interpolatedMatrix, DWORD forSubUpdate, bool interpolateScale);
      static float            getInterpolation(DWORD forSubUpdate);
            
      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);
      virtual bool postLoad(int saveType);

      void                    updateVisualVisibility();
      bool                    getVisualIsVisible() const;

      void                    setFlagSecondaryTurretScanToken(long hardpointIndex, bool v);
      bool                    getFlagSecondaryTurretScanToken(long hardpointIndex);


   protected:
      void                    updatePhysicsVisuals(float elapsedTime);
      void                    updateIK(bool firstUpdate);
      void                    updateMotionExtraction(float elapsedTime);
      void                    updateVisual(float elapsedTime);
      void                    updateVisualWorldMatrix();
      void                    updateLifespan();
      void                    updateAlpha();
      void                    updateAttachments(float elapsedTime, bool moving);
      void                    updateDetonate();
      void                    unselect(BTeamID teamID);
      void                    assertVisibility() const;              

      bool                    createDopple(BTeamID teamID);

      void                    loadTacticAnimInfo(const BProtoObject* pProto);

      void                    renderVisual();

      void                    playYawTurningSound(long hardpointIndex);
      void                    stopYawTurningSound(long hardpointIndex, bool immediate = false);
      void                    playPitchTurningSound(long hardpointIndex);
      void                    stopPitchTurningSound(long hardpointIndex, bool immediate = false);
      void                    stopAllHardpointSounds();

      //////////////////////////////////////////////////////////////////////////
      // SLB: This function is only meant to be called by markLOSOn and markLOSUpdate. Please don't call it or visibility will break.
      inline void             updateSimPosition(void);
      //////////////////////////////////////////////////////////////////////////

      virtual void            updateVisibleLists(void);
      DWORD                   getNewVisibility(void) const;
      void                    updateVisibility(DWORD newVisibility);
      DWORD                   getEnemyVisibility() const;
      DWORD                   getEnemySpyVisibility() const;

      void                    makeInvisible(BTeamID teamID);
      void                    makeSoftDopple(BTeamID teamID);
      void                    makeHardDopple(BTeamID teamID);
      
      void                    initIK();
      bool                    ikIdleTransitionComplete() const;

      bool                    canOrientHardpointToWorldPos(const BHardpoint *pHP, BVector tsTargetPos) const;
      bool                    canYawHardpointToWorldPos(const BHardpoint *pHP, BVector tsTargetPos) const;
      bool                    canPitchHardpointToWorldPos(const BHardpoint *pHP, BVector tsTargetPos) const;

#ifndef BUILD_FINAL
      void debugDrawHardpoints();
#endif

      // Important: Order all member variables by largest to smallest! Anything that requires 16-byte alignment should appear first.
      // Use "bool mBlah : 1;" instead of "bool bBlah;"
      // Use chars and shorts instead of longs or ints when possible.
      
      //-- visuals
      BBoundingBox            mBoundingBox;                                   // 80 bytes
      BBoundingBox            mSimBoundingBox;                                // 80 bytes
      
      BMatrix                 mOldWorldMatrix;                                // 48 bytes
      BMatrix                 mNewWorldMatrix;                                // 48 bytes

      BVector                 mCenterOffset;                                  // 16 bytes
      BVector                 mIconColorSize;                                 // 16 bytes
            
      enum { cMaxUVOffsets = BVisualRenderAttributes::cMaxUVOffsets };
      BVec2                   mUVOffsets[cMaxUVOffsets];
      uint                    mMultiframeTextureIndex;

      int                     mVisualVariationIndex;                          // 4 bytes
      BVisual*                mpVisual;                                       // 4 bytes
      float                   mAnimationRate;                                 // 4 bytes
      float                   mRadius;                                        // 4 bytes
      float                   mMoveAnimationPosition;                         // 4 bytes
      float                   mHighlightIntensity;                            // 4 bytes
      DWORD                   mSubUpdateNumber;                               // 4 bytes
      
      // Visibility stuff.
      BBitVector              mPlayerVisibility;                              // 4 bytes
      BBitVector              mDoppleBits;                                    // 4 bytes

      long                    mSimX;                                          // 4 bytes
      long                    mSimZ;                                          // 4 bytes

      //TODO: Maybe this is going away:)
      float                   mLOSScalar;                                     // 4 bytes
      long                    mLastSimLOS;                                    // 4 bytes
      float                   mLOSRevealTime;                                 // 4 bytes
      //TODO: Maybe this is going away:) (maybe it isn't)

      BAdditionalTextureArray* mpAdditionalTextures;                           // 4 bytes
      BObjectAttachmentArray* mpObjectAttachments;                            // 4 bytes
      BHardpointStateArray    mHardpointState;                                // 12 bytes (should this be a small dynamic array?)

      BObjectAnimationState   mAnimationState;                                // 16 bytes
      DWORD                   mAnimationLockEnds;                             // 4 bytes

      #if defined(BOBJECT_DEBUG_VISIBLEMAP)
      long                    mLOSX;                                          // 4 bytes
      long                    mLOSZ;                                          // 4 bytes
      long                    mLOSTeamID;                                     // 4 bytes
      #endif

      long                    mProtoID;                                       // 4 bytes
      BPlayerID               mColorPlayerID;                                 // 4 bytes

      // Tint
      DWORD                   mOverrideTint;                                  // 4 bytes
      DWORD                   mOverrideFlashInterval;                         // 4 bytes
      DWORD                   mOverrideFlashIntervalTimer;                    // 4 bytes
      DWORD                   mOverrideFlashDuration;                         // 4 bytes
      

      //TODO: Everything in this block is going away/getting moved someplace else.
      DWORD                   mLifespanExpiration;                            // 4 bytes
      float                   mCurrentAlphaTime;                              // 4 bytes
      float                   mAlphaFadeDuration;                             // 4 bytes
      //TODO: Everything in this block is going away/getting moved someplace else.           
      
      //Used for pulsing the color of the unit when it is being selected
      float                   mSelectionPulseTime;                            // 4 bytes
      float                   mSelectionPulsePercent;                         // 4 bytes
      float                   mSelectionFlashTime;                            // 4 bytes
      float                   mSelectionPulseSpeed;                           // 4 bytes
      float                   mLastRealtime;                                  // 4 bytes      

      BObjectOverrideRenderCallback mpOverrideRenderCB;                       // 4 bytes

      // Object's ambient occlusion "tint" value, set in the editor.
      uchar                   mAOTintValue;                                   // 1 byte
      int16                   mExplorationGroup;                              // 2 bytes
      uchar                   mTeamSelectionMask;                             // 1 byte

      //-- Flags
      bool                    mFlagVisibility:1;                              // 1 byte   
      bool                    mFlagLOS:1;                                     //       
      bool                    mFlagHasLifespan:1;                             // 
      bool                    mFlagDopples:1;                                 // 
      bool                    mFlagIsFading:1;                                // 
      bool                    mFlagAnimationDisabled:1;                       // 
      bool                    mFlagIsRevealer:1;                              // 
      bool                    mFlagDontInterpolate:1;                         //

      bool                    mFlagBlockLOS:1;                                // 1 byte
      bool                    mFlagCloaked:1;                                 //
      bool                    mFlagCloakDetected:1;                           //
      bool                    mFlagGrayMapDopples:1;                          //
      bool                    mFlagLOSMarked:1;                               //
      bool                    mFlagUseLOSScalar:1;                            //
      bool                    mFlagLOSDirty:1;                                //
      bool                    mFlagAnimationLocked:1;                         //

      bool                    mFlagUpdateSquadPositionOnAnimationUnlock:1;    // 1 byte
      bool                    mFlagExistSoundPlaying:1; //-- Asynsc flag!!!   //
      bool                    mFlagNoUpdate:1;                                //
      bool                    mFlagSensorLockTagged:1;                        //
      bool                    mFlagNoReveal:1;                                //
      bool                    mFlagBuilt:1;                                   //
      bool                    mFlagBeingCaptured:1;                           //
      bool                    mFlagInvulnerable:1;                            //

      bool                    mFlagVisibleForOwnerOnly:1;                     // 1 byte
      bool                    mFlagVisibleToAll:1;                            //      
      bool                    mFlagNearLayer:1;                               //
      bool                    mFlagIKDisabled:1;                              //
      bool                    mFlagHasTrackMask:1;                            //
      bool                    mFlagLODFading:1;                               //
      bool                    mFlagOccluded:1;                                //
      bool                    mFlagFadeOnDeath:1;                             //

      bool                    mFlagObscurable:1;                              // 1 byte
      bool                    mFlagNoRender:1;                                //
      bool                    mFlagTurning:1;                                 //
      bool                    mFlagAppearsBelowDecals:1;                      //
      bool                    mFlagSkipMotionExtraction:1;                    //
      bool                    mFlagOverrideTint:1;                            //
      bool                    mFlagMotionCollisionChecked:1;                  //
      bool                    mFlagIsDopple:1;                                //

      bool                    mFlagIsImpactEffect:1;                          // 1 byte
      bool                    mFlagDebugRenderAreaAttackRange:1;              //
      bool                    mFlagDontLockMovementAnimation:1;               //
      bool                    mFlagRemainVisible:1;                           //
      bool                    mFlagVisibleForTeamOnly:1;                      //
      bool                    mFlagDontAutoAttackMe:1;                        //
      bool                    mFlagAlwaysAttackReviveUnits:1;                 //
      bool                    mFlagNoRenderForOwner:1;                        //

      bool                    mFlagNoRenderDuringCinematic:1;                 // 1 byte
      bool                    mFlagUseCenterOffset:1;                         //
      bool                    mFlagNotDoppleFriendly:1;                       //
      bool                    mFlagForceVisibilityUpdateNextFrame:1;          //
      bool                    mFlagTurningRight:1;                            //
      bool                    mFlagIsUnderCinematicControl:1;                 //
      bool                    mFlagNoWorldUpdate:1;                           //

};

//==============================================================================
// BObject::updateSimPosition
// SLB: This function is only meant to be called by markLOSOn and markLOSUpdate. Please don't call it or visibility will break.
//==============================================================================
inline void BObject::updateSimPosition(void)
{
   getSimXZ(mSimX, mSimZ);
}

inline void BObject::getSimXZ(long &x, long &z) const
{
   XMVECTOR simPosition = __vctsxs(XMVectorMultiply(mPosition, XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), 0);
   x = simPosition.u[0];
   z = simPosition.u[2];
}
