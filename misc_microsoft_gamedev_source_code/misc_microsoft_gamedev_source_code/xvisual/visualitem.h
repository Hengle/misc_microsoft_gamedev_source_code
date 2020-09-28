//==============================================================================
// visualitem.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "protovisual.h"
#include "poolable.h"
#include "visualrenderattributes.h"
#include "Quaternion.h"
#include "gamefilemacros.h"

// Forward declarations
class BBoundingBox;
class IVisualInstance;
class BFloatProgression;
class BGrannyAnimation;
class BGrannyInstance;
class BIKNode;

enum
{
   cActionAnimationTrack,
   cMovementAnimationTrack,

   cNumAnimationTracks
};

typedef void (*BVisualItem_updateSyncCallback)(BVisualItem* pVisualItem, bool fullSync, bool doSyncChecks);

struct granny_skeleton;
struct granny_local_pose;

//==============================================================================
// BSparsePoseHelper
//==============================================================================
class BSparsePoseHelper
{
   public:
      BSparsePoseHelper() : mpSkeleton(NULL), mpLocalPose(NULL), mpSparseBoneArray(NULL), mpSparseBoneArrayReverse(NULL), bIsValid(false)  {};
     ~BSparsePoseHelper() {};

      void set(const granny_skeleton* pSkeleton, granny_local_pose* pLocalPose, void* pSparseBoneArray, void* pSparseBoneArrayReverse);
      bool getBone(long boneHandle, BMatrix *result) const;
      bool isValid() const { return bIsValid; }

      const granny_skeleton*  mpSkeleton;
      granny_local_pose*      mpLocalPose;
      void*                   mpSparseBoneArray;
      void*                   mpSparseBoneArrayReverse;
      bool                    bIsValid;
};

//==============================================================================
// BVisualAsset
//==============================================================================
class BVisualAsset
{
   public:
      BVisualAsset() : mType(-1), mIndex(-1) {};
     ~BVisualAsset() {};
      long                       mType;
      long                       mIndex;
};

//==============================================================================
// BVisualAnimationData
//==============================================================================
class BVisualAnimationData
{

public:
   BVisualAnimationData()     {};
   ~BVisualAnimationData()    {};

   void     init();
   void     init(const BVisualAnimationData &pSource);
   void     deinit();

   void     update(long animationTrack, BVisualItem* pVisualItem, float elapsedTime, BVisual* pVisual, long attachmentHandle, bool sendLoopEvent, bool &chain, float &timeIntoChainedAnimation, DWORD tintColor, const BMatrix& worldMatrix);

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType, BProtoVisual* pProtoVisual) const;
   virtual bool load(BStream* pStream, int saveType, BProtoVisual* pProtoVisual);

   long                       mAnimType;
   BVisualAsset               mAnimAsset;

   BProtoVisualTagArray*      mpTags;
   BProtoVisualPointArray*    mpPoints;
   BFloatProgression*         mpOpacityProgression;

   float                      mPosition;
   float                      mDuration;

   long                       mExitAction;
   long                       mTweenToAnimation;
   float                      mTweenTime;

   float                      mAnimationRate;

   BProtoVisualTagArray*      mpTweenOutTags;
   float                      mTweenOutPosition;
   float                      mTweenOutTimeLeft;
   float                      mTweenOutDuration;
   long                       mTweenOutAnimAssetType;

   bool                       mIsClone:1;
   bool                       mIsLocked:1;
   bool                       mIsDirty:1;
   bool                       mUsedByThisTrack:1;
};

//==============================================================================
// BVisualItem
//==============================================================================
class BVisualItem : public IPoolable
{
   public:
      enum
      {
         // Visual item flags
         cFlagSynced,
         cFlagSendNoAnimEvent,
         cFlagVisible,
         cFlagAnimationDisabled,

         // Attachment flags
         cFlagUsed,
         cFlagUser,
         cFlagActive,
         cFlagUseTransform,
         cFlagUseLifespan,
         cFlagJustCreated,
         cFlagSyncAnims,
         cFlagDisregardOrient,

         // Halwes - 2/2/2007 - This is temporary to draw targeted hit zone visuals with different tint colors.
         // Hit zone flags
         cFlagHitZone,

         // [10-8-08 CJS] Adding this to allow us to force particles to be immediately removed
         cFlagImmediateRemove,
         cFlagTransformUpdated,

         cFlagPostLoadCreateInstance,
      }; 

                                 BVisualItem();
      virtual                    ~BVisualItem();

      virtual bool               init(BProtoVisual* pProtoVisual, int64 userData, bool synced, DWORD tintColor, const BMatrix& worldMatrix, int displayPriority);
      virtual bool               clone(BProtoVisual* pProtoVisual, int64 userData, const BVisualItem* pSource, bool synced, bool bDisregardAttachments, DWORD tintColor, const BMatrix& worldMatrix);
      virtual void               deinit();

      void                       setAnimationEnabled(bool flag);

      virtual void               update(float elapsedTime, BVisual* pVisual, long attachmentHandle, bool sendLoopEvent, DWORD tintColor, const BMatrix& matrix, DWORD subUpdate);
      void                       updateDone();
      void                       updatePreAsync(float elapsedTime, DWORD subUpdate, bool animationEnabled = true);
      void                       updateAsync(float elapsedTime, bool root, DWORD subUpdate);      
      void                       resetInterpolatedClockValues();
      void                       updateGrannySync(bool doSyncChecks);
      bool                       isWorldMatrixUpdateNeeded();
      void                       updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix);
      void                       updateSecondaryWorldMatrix(BMatrix secondaryMatrix);
      void                       updateVisibility(bool bVisible);      
      void                       updateNearLayer(bool bNearLayer);

      void                       renderPrepare();
      void                       render(BVisualRenderAttributes* pRenderAttributes);  // jce [11/3/2008] -- only should be called on the top item of a visualitem tree
      void                       internalRender(BVisualRenderAttributes* pRenderAttributes);  // jce [11/3/2008] -- this is not meant to be called directly.  It is called from visualamanager.

      void                       createInstance(const BMatrix& worldMatrix, DWORD tintColor);
      void                       releaseInstance(bool fullRelease);

      void                       resetCombinedBoundingBox();
      void                       computeCombinedBoundingBox();
      void                       computeCombinedBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool attachment);

      BVector                    getMinCorner() const { return mCombinedMinCorner; }
      BVector                    getMaxCorner() const { return mCombinedMaxCorner; }

      void                       updateState(long animationTrack, BProtoVisual* pProtoVisual, int64 userData, long animType, bool applyInstantly, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, DWORD tintColor, const BMatrix& worldMatrix, const BProtoVisualAnimExitAction* pOverrideExitAction, bool clone, bool noRecursion);
      void                       updateState(long animationTrack, long animType, bool synced, long modelAssetType, long modelAssetIndex, const BVisualModelUVOffsets& modelUVOffsets, long animAssetType, long animAssetIndex, BProtoVisualTagArray* pTags, BProtoVisualPointArray* pPoints, BFloatProgression* pOpacityProgression, BVector minCorner, BVector maxCorner, bool applyInstantly, float timeIntoAnimation, float tweenTime, long damageIndex, bool reset, BVisualItem* startOnThisAttachment, DWORD tintColor, const BMatrix& worldMatrix);
      void                       updateState(long animationTrack, BVisualItem* pState, bool synced, long animType, bool applyInstantly, float timeIntoAnimation, float tweenTime, long newExitAction, long newTweenToAnimation, float newTweenTime, bool reset, BVisualItem* startOnThisAttachment, bool clone, DWORD tintColor, const BMatrix& worldMatrix, bool noRecursion);

      virtual void               copyAnimationTrack(long fromTrack, long toTrack, BProtoVisual* pProtoVisual, int64 userData, bool applyInstantly, DWORD tintColor, const BMatrix& worldMatrix, BVisualItem* startOnThisAttachment = NULL, bool noRecursion = false);
      virtual void               setAnimation(long animationTrack, BProtoVisual* pProtoVisual, int64 userData, long animType, bool applyInstantly, float timeIntoAnimation, DWORD tintColor, const BMatrix& worldMatrix, long forceAnimID = -1, bool reset = false, BVisualItem* startOnThisAttachment = NULL, const BProtoVisualAnimExitAction* pOverrideExitAction = NULL);
      long                       getAnimationType(long animationTrack) const { return mAnimationTrack[animationTrack].mAnimType; }
      const BGrannyAnimation*    getAnimation(long animationTrack) const;
      float                      getAnimationDuration(long animationTrack) const;
      float                      getAnimationPosition(long animationTrack) const;
      float                      getAnimationClock(long animationTrack) const;
      float                      getAnimationOpacity() const;
      float                      getAnimationTweenTime(long animationTrack) const { return mAnimationTrack[animationTrack].mTweenTime; }
      void                       applyAnimation(long animationTrack, long animType, bool synced, bool applyInstantly, float timeIntoAnimation, float tweenTime, BVisualItem* startOnThisAttachment);
      BVisualAnimationData       getAnimationData(long animationTrack, BProtoVisual* pProtoVisual, int64 userData, long animType);
      int                        getAnimationExitAction(long animationTrack) const { return mAnimationTrack[animationTrack].mExitAction; }

      float                      getAnimationRate(long animationTrack) const { mAnimationTrack[animationTrack].mAnimationRate; }
      void                       setAnimationRate(long animationTrack, float animationRate);
      void                       setAttachmentAnimationRate(long animationTrack, float animationRate);

      long                       getBoneHandle(const char* pBoneName) const;
      bool                       getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix=NULL, BBoundingBox* pBox=NULL, const BMatrix* pOffsetMatrix=NULL, bool applyIK=true, const BVisualItem* pHasThisChild=NULL) const;

      long                       getNumBones() const;

      bool                       setNumIKNodes(long numNodes);
      void                       setIKNode(long node, BVector targetPos);
      void                       setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType);
      BIKNode*                   getIKNodeFromTypeBoneHandle(long type, long boneHandle);
      BIKNode*                   getIKNodeFromIndex(long node);
      void                       lockIKNodeToGround(long boneHandle, bool lock, float start, float end);
      long                       getIKNodeBoneHandle(long node) const;
      bool                       getIKNodeAnchor(long node, BVector &anchorPos, float& lockStartTime, float& lockEndTime, bool& lockComplete) const;
      void                       setIKNodeLockComplete(long node, bool lockComplete);
      void                       setIKNodeSweetSpot(long boneHandle, BVector sweetSpotPos, float start, float sweetSpot, float end);
      bool                       getIKNodeSweetSpot(long node, BVector &sweetSpotPos, float &start, float &sweetSpot, float &end);
      bool                       isIKNodeActive(long node) const;
      void                       setIKNodeActive(long node, bool active);
      void                       setIKNodeSingleBone(long node, BVector position, BQuaternion orientation);
      bool                       getIKNodeSingleBone(long node, BVector &position, BQuaternion &orientation);

      long                       getNumTagsOfType(long animationTrack, long animType) const;
      long                       getNumTags(long animationTrack) const;

      long                       getNumberPoints(long animationTrack, long pointType) const;
      long                       getPointHandle(long animationTrack, long pointType) const;
      long                       getNextPointHandle(long animationTrack, long pointHandle, long pointType) const;
      long                       getNextPointHandle(long animationTrack, long& item, long point, long pointType, long& itemCounter) const;
      long                       getRandomPoint(long animationTrack, long pointType) const;
      bool                       getPointPosition(long animationTrack, long pointHandle, BVector& position, BMatrix *pMatrixOut=NULL) const;
      long                       getPointData(long animationTrack, long pointHandle) const;
      const BProtoVisualPoint*   getPointProto(long animationTrack, long pointHandle) const;
      const BVisualItem*         getPointItem(long pointHandle, BMatrix* pMatrixOut=NULL) const;
      const BVisualItem*         getPointItem(long& item, long point, long& itemCounter, BMatrix* pMatrixOut=NULL) const;

      long                       getHPBarPoint(void) const;

      long                       getAttachmentHandle(const char* pAttachmentName);
      BVisualItem*               getAttachment(long attachmentHandle, BMatrix* pTransformedMatrixOut=NULL, BMatrix* pMatrixOut=NULL);
      const BVisualItem*         getAttachment(long attachmentHandle, BMatrix* pTransformedMatrixOut=NULL, BMatrix* pMatrixOut=NULL) const;
      BVisualItem*               getAttachmentByToBoneName(const char* pToBoneName) const;
      BVisualItem*               getAttachmentByData(long visualAssetType, long data0) const;
      long                       getAttachmentFromBoneHandle(long attachmentHandle);
      long                       getAttachmentToBoneHandle(long attachmentHandle);
      long                       getNumberAttachments() const { return mAttachments.size(); }
      BVisualItem*               getAttachmentByIndex(uint index) const { if (index >= mAttachments.size()) return NULL; else return mAttachments[index]; }

      void                       setTransform(BMatrix matrix);
      BMatrix                    getTransform() const {return(mTransform);}
      void                       clearTransform();
      void                       setAttachmentTransform(long attachmentHandle, BMatrix matrix);
      bool                       getAttachmentTransform(long attachmentHandle, BMatrix& matrix);
      void                       clearAttachmentTransform(long attachmentHandle);

      long                       getAttachmentAnimationType(long animationTrack, long attachmentHandle) const;
      float                      getAttachmentAnimationDuration(long animationTrack, long attachmentHandle) const;
      float                      getAttachmentAnimationPosition(long animationTrack, long attachmentHandle) const;
      float                      getAttachmentAnimationClock(long animationTrack, long attachmentHandle) const;

      void                       updateAttachmentTransforms(bool init=false);
      void                       updateTransform(IVisualInstance* parentInstance);
      void                       getTransform(IVisualInstance* parentInstance, BMatrix &result) const;
      void                       getRenderTransform(IVisualInstance* parentInstance, BMatrix &result) const;
      void                       updateTerrainEffects(const BMatrix& matrix, DWORD tintColor);

      bool                       processAttachmentEvent(long animationTrack, long animType, long eventType, BProtoVisualTag* pTag, float tweenTime, DWORD tintColor, const BMatrix& worldMatrix);
      int                        addAttachment(long visualAssetType, long animationTrack, long animType, float tweenTime, int data0, long toBoneHandle, float lifespan, bool bDisregardOrient, DWORD tintColor, const BMatrix& worldMatrix, const BMatrix *transformMatrix=NULL, bool isUserAttachment=false);

      long                       getDamageTemplateID() const;

      void                       setAnimationLock(long animationTrack, bool lock);
      bool                       getAnimationLock(long animationTrack) const           { return mAnimationTrack[animationTrack].mIsLocked; }

      bool                       getAnimationIsClone(long animationTrack) const       { return mAnimationTrack[animationTrack].mIsClone; }

      bool                       raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, const BMatrix &worldMatrix, float *distanceSqr, float &intersectDistanceSqr, long &intersectAttachmentHandle, long &intersectBoneHandle, BVector &intersectBoneSpacePos, BVector &intersectBoneSpaceDir);


      bool                       getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void                       setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

      void                       setDisplayPriority(int value) { mDisplayPriority = value; }
      int                        getDisplayPriority() const { return mDisplayPriority; }

      const BVisualModelUVOffsets&  getUVOffsets() const { return mModelUVOffsets; }
      void                       setUVOffsets(const BVisualModelUVOffsets &uvOffsets);

      void                       setGrannyMeshMask(bool visible);
      
      static void                setUpdateSyncCallback(BVisualItem_updateSyncCallback newCallBack) { sUpdateSyncCallback = newCallBack; }
      static void	               clearUpdateSyncCallback(void) { sUpdateSyncCallback = defaultUpdateSyncCallback; }

      virtual void               onAcquire();
      virtual void               onRelease();

      BGrannyInstance*           getGrannyInstance();      
                                 
      void                       clearAttachmentData();

      void                       removeAttachmentsOfAssetType(long assetType);

      void                       setTintColor(DWORD color);

      void                       validateAnimationTracks() const;

      DECLARE_FREELIST(BVisualItem, 10);

      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType, BProtoVisual* pProtoVisual) const;
      virtual bool load(BStream* pStream, int saveType, BProtoVisual* pProtoVisual, const BMatrix& worldMatrix, DWORD tintColor);
      virtual bool postLoad(int saveType, BProtoVisual* pProtoVisual, const BMatrix& worldMatrix, DWORD tintColor);

      static BVisualItem_updateSyncCallback sUpdateSyncCallback;
      static void                defaultUpdateSyncCallback(BVisualItem* pVisualItem, bool fullSync, bool doSyncChecks);

      // FIXME AJL 9/27/06 - Possibly separate out attachment data and only allocate if needed
      BMatrix                    mMatrix;      
      DWORD                      mSubUpdateNumber;
      DWORD                      mGrannySubUpdateNumber;

      // Visual item data
      IVisualInstance*           mpInstance;
      BVector                    mCombinedMinCorner;
      BVector                    mCombinedMaxCorner;
      BVector                    mMinCorner;
      BVector                    mMaxCorner;
      BVisualAsset               mModelAsset;
      BVisualModelUVOffsets      mModelUVOffsets;
      UTBitVector<32>            mFlags;
      BSmallDynamicSimArray<BVisualItem*>    mAttachments;

      // FIXME AJL 9/27/06 - Possibly separate out attachment data and only allocate if needed
      long                       mFromBoneHandle;
      long                       mToBoneHandle;        
      long                       mAttachmentHandle;
      long                       mAttachmentType;
      long                       mIndex;
      long                       mData;
      const BSimString*          mpName;
      float                      mLifespan;      

      BVisualAnimationData       mAnimationTrack[cNumAnimationTracks];

      long                       mModelVariationIndex;      // This is use when component variation is present, and the selected
                                                            // variation needs to be consistent for the lifetime of the unit.
      long                       mDamageTemplateIndex;
      int                        mDisplayPriority;

      bool                       mFlagObjectVisible;        // This bool replaces cFlagObjectVisible because flags are synced
   protected:

      void                       updateAsyncRecurse(float elapsedTime, DWORD subUpdate);      
      void                       updateTransformsRecurse(BSparsePoseHelper parentSP, long attachmentLevel = 0);
      void                       getTransform(BSparsePoseHelper parentSP, BSparsePoseHelper curSP, BMatrix &result) const;

      BVisualItem*               getNewInstance(bool isUserAttachment) const;

      BMatrix                    mTransform;
      BMatrix                    mOldTransform;

};
