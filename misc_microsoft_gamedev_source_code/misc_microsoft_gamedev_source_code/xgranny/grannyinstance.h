//============================================================================
// grannyinstance.h
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================

#ifndef _GRANNYINSTANCE_H_
#define _GRANNYINSTANCE_H_

// Includes
#include <granny.h>
#include "grannyanimation.h"
#include "bitarray.h"
#include "boundingbox.h"
#include "visualinstance.h"
#include "gamefilemacros.h"

// Forward declarations
class BGrannyAnimation;
class BGrannyInstanceRenderer;
class BGrannyModel;
class BGrannySampleAnimCache;
class BVisualRenderAttributes;
class BGrannyInstance;
struct granny_model_instance;

//-- This can't be any larger than GrannyMaximumUserData which is defined as (1 << 2) in Granny.
enum
{
	GrannyControlUserData_AnimIndex=0,
	GrannyControlUserData_IsBlendControl,
	GrannyControlUserData_EasingOut,
	GrannyControlUserData_AnimationTrack
};

typedef void (*BGrannyInstance_updateSyncCallback)(granny_model_instance* modelInstance, bool doSyncChecks);
typedef bool (*BGrannyInstance_meshRenderMaskResetCallback)(BGrannyInstance* pGrannyInstance, const BBitArray& previousMask);

//==============================================================================
// BIKNode
//==============================================================================
class BIKNode
{
public:
   enum
   {
      cIKNodeTypeNotSet = 0,
      cIKNodeTypeGround,
      cIKNodeTypeSweetSpot, 
      cIKNodeTypeSingleBone,

      cNumIKNodeTypes,
   };
   
                              BIKNode();
   
   bool                       save(BStream* pStream, int saveType) const;
   bool                       load(BStream* pStream, int saveType);
   
   BVector                    getAnchorPos() const {return(mAnchorPos);}
   BVector                    getTargetPos() const {return(mTargetPos);}
   BVector                    getOldAnchorPos() const {return(mOldAnchorPos);}
   BVector                    getOldTargetPos() const {return(mOldTargetPos);}
  
   void                       setAnchorPos(BVector pos);
   void                       setTargetPos(BVector pos);
   void                       setOldAnchorPos(BVector pos);
   void                       setOldTargetPos(BVector pos);
   
   BVector                    getInterpolatedAnchorPosAsOrientation(float interpolation) const;  // jce [11/14/2008] -- the anchor pos when used for single bone "IK" is used as an orientation.
   BVector                    getInterpolatedTargetPos(float interpolation) const;
   

   float    mStart;
   float    mSweetSpot;
   float    mEnd;
   long     mBoneHandle;
   uint8    mLinkCount;

   uint8    mNodeType;

   bool     mIsActive:1;
   bool     mHasAnchor:1;
   bool     mLockComplete:1;
   bool     mHeightOnlyLock:1;
   bool     mIdleTransitioning:1;
   bool     mIdleTransitionLockStarted:1;
   bool     mAnchorPosUpdated;
   bool     mTargetPosUpdated;

protected:   
   BVector  mAnchorPos;
   BVector  mTargetPos;

   BVector  mOldAnchorPos;
   BVector  mOldTargetPos;
};

class BClockOverride
{
   public:
      BClockOverride() : 
         mOldAnimPos(0.0f),
         mNewAnimPos(0.0f),
         mActionIndex(-1),
         mMovementIndex(-1),
         mElapsed(0.0f),
         mAnimDuration(0.0f),
         mOldClock(0.0f)
      {
      }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      float mOldAnimPos;      
      float mNewAnimPos;      
      long  mActionIndex;
      long  mMovementIndex;
      float mElapsed;
      float mAnimDuration;
      float mOldClock;
};

struct BGrannySaveControlState
{
   granny_control* Control;
   granny_real32   LocalClock;
   granny_real32   CurrentClock;
   granny_real32   dTLocalClockPending;
   granny_int32    LoopIndex;
};

//============================================================================
// class BGrannyInstance
//============================================================================
class BGrannyInstance : 
   public IVisualInstance
{
   friend class BGrannyInstanceRenderer;
   
   public:
                              BGrannyInstance();
                             ~BGrannyInstance();

      bool                    init(long modelIndex, const BVisualModelUVOffsets* pUVOffsets);

      // IVisualInstance
      virtual void            deinit();
      virtual void            update(float elapsedTime, bool synced = false);      
      void                    updateGrannySync(bool doSyncChecks);
      virtual void            updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix){};
      virtual void            setSecondaryMatrix(BMatrix matrix){};
      virtual void            setVisibility(bool bState) {};
      virtual void            setNearLayer(bool bState) {};      
      virtual void            setTintColor(DWORD color) {};
      virtual void            render(const BVisualRenderAttributes* pRenderAttributes);      
      virtual void            computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners);
      virtual long            getBoneHandle(const char* pBoneName);
      virtual bool            getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix=NULL, BBoundingBox* pBox=NULL, const BMatrix* pWorldMatrix=NULL, bool applyIK=true);
      virtual bool            getBoneForRender(long boneHandle, BMatrix &matrix);  // jce [11/3/2008] -- this will only work properly for a short-interval during rendering (i.e. not for general use)
      virtual bool            setNumIKNodes(long numNodes)  { return mIKNodes.setNumber(numNodes); }
      virtual void            setIKNode(long node, BVector targetPos);
      virtual void            setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType);
      virtual BIKNode*        getIKNodeFromTypeBoneHandle(long type, long boneHandle);
      virtual BIKNode*        getIKNodeFromIndex(long node);
      virtual void            lockIKNodeToGround(long boneHandle, bool lock, float start, float end);
      virtual void            setIKNodeLockComplete(long node, bool lockComplete);
      virtual long            getIKNodeBoneHandle(long node) const;
      virtual bool            getIKNodeAnchor(long node, BVector &anchorPos, float& lockStartTime, float& lockEndTime, bool& lockComplete) const;
      virtual void            setIKNodeSweetSpot(long boneHandle, BVector sweetSpotPos, float start, float sweetSpot, float end);
      virtual bool            getIKNodeSweetSpot(long node, BVector &sweetSpotPos, float &start, float &sweetSpot, float &end);
      virtual bool            isIKNodeActive(long node) const;
      virtual void            setIKNodeActive(long node, bool active);
      virtual void            setIKNodeSingleBone(long node, BVector position, BQuaternion orientation);
      virtual bool            getIKNodeSingleBone(long node, BVector &position, BQuaternion &orientation);

      void                    preRender(float interpolation);
      void                    updateInterpolatedClockValues();      
      void                    resetInterpolatedClockValues();      

      // These functions should not be called directly.  Go through the Granny manager since it is the
      // real owner of the Granny instances.
      long                    getRefCount() const { return mRefCount; }
      void                    incRef()            { mRefCount++; }
      void                    decRef()            { mRefCount--; }

      // Animation control.
      bool                    playAnimation(long animationTrack, long animIndex, float playWeight, float currentTime, long loopCount=0, float transitionTime=0.25f, float timeIntoAnimation=0.0f);
		bool                    blendAnimation(long animationTrack, long animIndex, float playWeight, float currentTime, long loopCount=0, float transitionTimeIn=0.25f, float transitionTimeOut=0.25f, float timeIntoAnimation=0.0f);
      long                    stopAnimations(long animationTrack, float stopOverSeconds);

      void                    setAnimationRate(long animationTrack, float animationRate);

      void                    freeGrannyControls();

      void                    setClock(float seconds);
      void                    recenterClock(float delta);
      float                   getAnimClock() const { return mAnimClock; }
      float                   getGrannyControlClock();
      bool                    isInterpolating() { return mIsInterpolating; }
      void                    setInterpolatedClocks(granny_model_instance *modelInstance, BGrannySaveControlState* pSaveStates, uint maxSaveStates, uint& numSaveStates);
      void                    unsetInterpolatedClocks(granny_model_instance *modelInstance, const BGrannySaveControlState* pSaveStates, uint numSaveStates);
      bool                    isClockSet(){ return mClockSet; }
            
      // Gets a matrix that represents the motion that occurred from (current time - elapsedTime)
      // to the current time.  In other words, this looks back in time at the motion that occurred
      // up to the current time.  The clock must have already been set to the current time before
      // calling this function.
      // The extracted motion will be added to the data already in the passed in matrix so it can
      // be the identity matrix or a matrix that contains accumulated position and orientation data.
      void                    getExtractedMotion(float elapsedTime, BMatrix &matrix) const;

      BVector                 getAverageExtractedMotion() const;

      granny_world_pose*      computeWorldPose(D3DXMATRIX* pWorldMatrix = NULL, bool useIK = false);
      granny_matrix_4x4*      getCompositeBuffer(D3DXMATRIX* pWorldMatrix = NULL);

      // Low-level accessors.
      long                    getModelIndex() const {return(mModelIndex);}
      granny_model_instance*  getModelInstance() {return(mModelInstance);}
      long                    getGMIndex() const {return(mGrannyManagerIndex);}
      void                    setGMIndex(long index) {BDEBUG_ASSERT((index >= SHRT_MIN) && (index <= SHRT_MAX)); mGrannyManagerIndex=(short)index;}

      // Render mask
      const BBitArray&        getMeshRenderMask() const { return mMeshRenderMask; }
      void                    setMeshRenderMask(const BBitArray& mask);
      void                    clearMeshRenderMask() { mMeshRenderMask.clear(); mMeshRenderMaskAllSet = false; }
      void                    setMeshVisible(uint meshIndex, bool flag);
      bool                    getMeshRenderMaskAllSet() const { return mMeshRenderMaskAllSet; }
      void                    setMeshRenderMaskToUndamageState();
      void                    setMeshRenderMaskToDamageNoHitsState();
      
      const BVisualModelUVOffsets&  getUVOffsets() const { return mUVOffsets; }
      void                    setUVOffsets(const BVisualModelUVOffsets &uvOffsets) { mUVOffsets = uvOffsets; }
      
      long                    getMeshCount() const;
      long                    computeVertexCount() const;
      long                    computeTriangleCount() const;
      
      const BCHAR_T*          getFilename() const;
      
      bool                    hasMotionExtraction() const      { return mHasMotionExtraction; }

      long                    getNumIKNodes() const            { return mIKNodes.getNumber(); }
      const BIKNode&          getIKNode(long node) const       { return mIKNodes.get(node); }
      void                    setClockOverride(float animPos, float animDuration, long actionIndex, long movementIndex, float elapsed);
      void                    clearClockOverride();

      bool                    raySegmentIntersects(const BVector &originWorldSpace, const BVector &vectorWorldSpace, bool segment, const BMatrix &worldMatrix, float *distanceSqr, float &intersectDistanceSqr, long &intersectBoneHandle, BVector &intersectBoneSpacePos, BVector &intersectBoneSpaceDir);

      static void             setUpdateSyncCallback(BGrannyInstance_updateSyncCallback newCallBack) { sUpdateSyncCallback = newCallBack; }
      static void	            clearUpdateSyncCallback(void) { sUpdateSyncCallback = defaultUpdateSyncCallback; }

      void                    setMeshRenderMaskResetCallback(BGrannyInstance_meshRenderMaskResetCallback newCallBack) { mpRenderMaskResetCallback = newCallBack; }
      void                    clearMeshRenderMaskResetCallback() { mpRenderMaskResetCallback = NULL; }

      void                    clearSampleAnimCache();
      
      // jce [10/31/2008] -- Functions related to pre-posing granny models before interpolated rendering to set up attachments correctly.
      void                    renderPrepare();
      void                    renderPrepareSampleAnims();
      void                    cleanupRenderPrepare();
      const granny_local_pose* getRenderPrepareLocalPose() const {return(mRenderPrepareLocalPose);}
      

      GFDECLAREVERSION
      bool                    save(BStream* pStream, int saveType) const;
      bool                    load(BStream* pStream, int saveType);

   protected:
      static BGrannyInstance_updateSyncCallback sUpdateSyncCallback;
      static void             defaultUpdateSyncCallback(granny_model_instance* modelInstance, bool doSyncChecks);
      float                   getInterpolatedClock();

      granny_model_instance*  mModelInstance;
      short                   mModelIndex;
      short                   mRefCount;

      BSmallDynamicArray<BIKNode> mIKNodes;

      BGrannyInstance_meshRenderMaskResetCallback mpRenderMaskResetCallback;

      float                   mAnimClock;
      float                   mOldClock;
      float                   mNewClock;
      bool                    mIsInterpolating;
      float                   mInterpolation; 

// ajl 10/10/08 - Temp ifdef out because code using it will cause OOS in MP games.
//#if 0
      // used if we have a clock override
      BClockOverride*         mpClockOverride;
//#endif

      BBitArray               mMeshRenderMask;
      
      float                   mActiveAnimTime;
      //float                   mActiveAnimDuration;
      //granny_transform_track* mpAttachmentTransformTrack;
      
      BVisualModelUVOffsets   mUVOffsets;

      BGrannySampleAnimCache* mpSampleAnimCache;
      
      granny_local_pose*      mRenderPrepareLocalPose;

      short                   mGrannyManagerIndex;
      uint16                  mStateChange;
                  
      bool                    mHasMotionExtraction : 1;
      bool                    mMeshRenderMaskAllSet : 1;      
      bool                    mClockSet : 1;
      
      void                    verifyBoneCount(long boneCount);

};

#endif
