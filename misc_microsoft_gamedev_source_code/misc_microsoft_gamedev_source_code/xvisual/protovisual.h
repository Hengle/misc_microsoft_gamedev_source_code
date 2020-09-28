//==============================================================================
// protovisual.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "math\vector.h"
#include "math\matrix.h"
#include "math\generalVector.h"
#include "visualrenderattributes.h"
#include "threading\eventDispatcher.h"

// Forward declarations
class BProtoVisualLogicNode;
class BProtoVisualModel;
class BVisual;
class BVisualItem;
class BFloatProgression;

extern char *gAnimTypeNames[];

// Asset types
enum
{
   cVisualAssetGrannyModel,
   cVisualAssetGrannyAnim,
   cVisualAssetParticleSystem,
   cVisualAssetLight,
   cVisualAssetTerrainEffect,
   cVisualAssetMaxCount
};

// ATTENTION:  If you add to this enumeration you need to update the list in TriggerPropAnimType class in the editor code.
// Animation types
enum
{
   cAnimTypeIdle = 0,
   cAnimTypeWalk,
   cAnimTypeJog,
   cAnimTypeRun,
   cAnimTypeRangedAttack,   
   cAnimTypeAttack,
   cAnimTypeLimber,
   cAnimTypeUnlimber,
   cAnimTypeDeath,
   cAnimTypeWork,
   cAnimTypeResearch,
   cAnimTypeTrain,
   cAnimTypeBored,
   cAnimTypePelicanGarrison,
   cAnimTypePelicanGarrison2,
   cAnimTypePelicanGarrison3,
   cAnimTypePelicanGarrison4,
   cAnimTypePelicanGarrison5,
   cAnimTypePelicanGarrison6,
   cAnimTypePelicanGarrison7,
   cAnimTypePelicanGarrison8,
   cAnimTypePelicanGarrison9,
   cAnimTypePelicanGarrison10,
   cAnimTypePelicanUngarrison,
   cAnimTypePelicanUngarrison2,
   cAnimTypePelicanUngarrison3,
   cAnimTypePelicanUngarrison4,
   cAnimTypePelicanUngarrison5,
   cAnimTypePelicanUngarrison6,
   cAnimTypePelicanUngarrison7,
   cAnimTypePelicanUngarrison8,
   cAnimTypePelicanUngarrison9,
   cAnimTypePelicanUngarrison10,
   // 7/7/06 mrh - temporary anim type enums for transport power.  Until we figure out how the anim system will work.
   cAnimTypeIncoming,
   cAnimTypeLanding,
   cAnimTypeTakeoff,
   cAnimTypeOutgoing,
   cAnimTypeHoverIdle,
   cAnimTypeShadePlasmaAttack,
   cAnimTypeClamshell,
   cAnimTypeClamshellRocket,
   cAnimTypeCinematic,
   cAnimTypeEvadeLeft,
   cAnimTypeEvadeRight,
   cAnimTypeEvadeFront,
   cAnimTypeEvadeBack,
   cAnimTypeBlock,
   cAnimTypeCheer,
   cAnimTypeRetreat,
   cAnimTypeHandSignalGo,
   cAnimTypeHandSignalStop,
   cAnimTypeReload,
   cAnimTypeCombatAction,
   cAnimTypeFlail,
   cAnimTypeStop,
   cAnimTypeTurnAround,
   cAnimTypeTurnLeft,
   cAnimTypeTurnRight,
   cAnimTypeTurnRight45Forward,
   cAnimTypeTurnRight45Back,
   cAnimTypeTurnLeft45Back,
   cAnimTypeTurnLeft45Forward,
   cAnimTypeTurnWalk,
   cAnimTypeTurnJog,
   cAnimTypeTurnRun,
   cAnimTypeRepair,
   cAnimTypeFloodDeathJog,
   cAnimTypeFloodDeath,
   cAnimTypeIdleWalk,
   cAnimTypeIdleJog,
   cAnimTypeIdleRun,
   cAnimTypeWalkIdle,
   cAnimTypeJogIdle,
   cAnimTypeRunIdle,
   cAnimTypeHitch,
   cAnimTypeUnhitch, 
   cAnimTypeSprint,
   cAnimTypeRecover,
   cAnimTypeHeal,
   cAnimArtilleryAttack,
   cAnimTypeLockdownArtilleryAttack,
   cAnimTypeStasis,
   cAnimTypeRoar,
   cAnimTypeCower,
   cAnimTypeKamikaze,
   cAnimTypeStunned,
   cAnimTypeDetonated,
   cAnimTypeShatterDeath,
   cAnimTypeRageLeap,
   cAnimTypeHotDropDown,
   cAnimTypePowerIdle,

   // ATTENTION:  If you add to this enumeration you need to update the list in TriggerPropAnimType class in the editor code.
   // Animation types
   cAnimTypeMaxCount
};

// Tag types
enum
{
   cAnimEventAttack,
   cAnimEventSound,
   cAnimEventParticles,
   cAnimEventTerrainEffect,
   cAnimEventCameraShake,
   cAnimEventLoop,
   cAnimEventLight,
   cAnimEventGroundIK,
   cAnimEventAttachTarget,
   cAnimEventSweetSpot,
   cAnimEventAlphaTerrain,
   cAnimEventEnd,
   cAnimEventChain,
   cAnimEventRumble,
   cAnimEventBuildingDecal,
   cAnimEventUVOffset,
   cAnimEventKillAndThrow,
   cAnimEventPhysicsImpulse,
   cAnimEventMaxCount
};

// Exit action types
enum
{
   cAnimExitActionLoop,
   cAnimExitActionFreeze,
   cAnimExitActionTransition,
   cAnimExitActionMaxCount
};

// Logic types
enum
{
   cVisualLogicVariation,
   cVisualLogicBuildingCompletion,
   cVisualLogicTech,
   cVisualLogicSquadMode,
   cVisualLogicImpactSize,
   cVisualLogicDestruction,
   cVisualLogicMaxCount
};

// Point types
enum
{
   cVisualPointImpact,
   cVisualPointLaunch,
   cVisualPointHitpointBar,
   cVisualPointReflect,
   cVisualPointCover,
   cVisualPointCarry,
   cVisualPointPickup,
   cVisualPointBoard,
   cVisualPointBobble,
   cVisualPointMaxCount
};

//==============================================================================
// BProtoVisualTag
//==============================================================================
class BProtoVisualTag
{
   public:
      enum { cFlagCheckVisible, cFlagDiregardOrient, cFlagLockToGround, cFlagAttach, cFlagCheckFOW, cFlagCheckSelected };

                                                BProtoVisualTag();
                                                BProtoVisualTag(const BProtoVisualTag& source) { *this=source; }
                                                BProtoVisualTag& operator=(const BProtoVisualTag& source);

      bool                                      getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void                                      setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      int8                                      mEventType;
      float                                     mPosition;
      union 
      {
         float                                  mValue0;
         int                                    mValueInt0;
      };
      union
      {
         float                                  mValue1;
         int                                    mValueInt1;
      };
      union
      {
         float                                  mValue2;
         int                                    mValueInt2;
      };
      BSimString                                mName;
      BSimString                                mToBoneName;      // used for attack, particle, sound and light tags
      long                                      mToBoneHandle;
      int                                       mToBoneModelIndex;
      int                                       mData0;
      float                                     mLifespan;
      bool                                      mBoolValue0;

      UTBitVector<8>                            mFlags;
};

typedef BSmallDynamicSimArray<BProtoVisualTag> BProtoVisualTagArray;
typedef BSmallDynamicSimArray<BProtoVisualTag*> BProtoVisualTagPtrArray;

//==============================================================================
// BProtoVisualAsset
//==============================================================================
class BProtoVisualAsset
{
   public:
                                                BProtoVisualAsset();
                                                BProtoVisualAsset(const BProtoVisualAsset& source) { *this=source; }
                                                BProtoVisualAsset& operator=(const BProtoVisualAsset& source);

      BVector                                   mMinCorner;
      BVector                                   mMaxCorner;
      long                                      mAssetType;
      BSimString                                mAssetName;
      long                                      mAssetIndex;
      BSimString                                mDamageAssetName;
      long                                      mDamageAssetIndex;
      long                                      mWeight;
      BProtoVisualTagArray                      mTags;
      BFloatProgression*                        mpOpacityProgression;
      
      BVisualModelUVOffsets                     mModelUVOffsets;
            
      bool                                      mLoadFailed;
};

//==============================================================================
// BProtoVisualAttachment
//==============================================================================
class BProtoVisualAttachment
{
   public:
                                                BProtoVisualAttachment();
                                                BProtoVisualAttachment(const BProtoVisualAttachment& source) { *this=source; }
                                                BProtoVisualAttachment& operator=(const BProtoVisualAttachment& source);

      BSimString                                mAttachmentName;
      long                                      mAttachmentHandle;
      long                                      mAttachmentType;
      long                                      mAttachmentIndex;
      BSimString                                mFromBoneName;
      long                                      mFromBoneHandle;
      BSimString                                mToBoneName;
      long                                      mToBoneHandle;
      bool                                      mFromBoneFailed;
      bool                                      mToBoneFailed;
      bool                                      mSyncAnims;
      bool                                      mDisregardOrient;
};

//==============================================================================
// BProtoVisualPoint
//==============================================================================
class BProtoVisualPoint
{
   public:
                                                BProtoVisualPoint();
                                                BProtoVisualPoint(const BProtoVisualPoint& source) { *this=source; }
                                                BProtoVisualPoint& operator=(const BProtoVisualPoint& source);

      long                                      mPointType;
      BSimString                                mPointDataName;
      long                                      mPointDataValue;
      BSimString                                mBoneName;
      long                                      mBoneHandle;
};

typedef BSmallDynamicSimArray<BProtoVisualPoint> BProtoVisualPointArray;

//==============================================================================
// BProtoVisualLogicValue
//==============================================================================
class BProtoVisualLogicValue
{
   public:
                                                BProtoVisualLogicValue();
                                                BProtoVisualLogicValue(const BProtoVisualLogicValue& source) { *this=source; }
                                                BProtoVisualLogicValue& operator=(const BProtoVisualLogicValue& source);

      BSimString                                mValueName;
      DWORD                                     mValueDWORD;
      float                                     mValueFloat;
      BProtoVisualModel*                        mpModel;
};

//==============================================================================
// BProtoVisualLogicNode
//==============================================================================
class BProtoVisualLogicNode
{
   public:
                                                BProtoVisualLogicNode();
                                                BProtoVisualLogicNode(const BProtoVisualLogicNode& source) { *this=source; }
                                                BProtoVisualLogicNode& operator=(const BProtoVisualLogicNode& source);

      long                                      mLogicType;
      BSmallDynamicSimArray<BProtoVisualLogicValue>  mLogicValues;
};

//==============================================================================
// BProtoVisualAnimExitAction
//==============================================================================
struct BProtoVisualAnimExitAction
{
   long                                            mExitAction;
   long                                            mTweenToAnimation;
   float                                           mTweenTime;
};

//==============================================================================
// BProtoVisualOptionalMesh
//==============================================================================
class BProtoVisualOptionalMesh
{
   public:
                                                BProtoVisualOptionalMesh();
                                                BProtoVisualOptionalMesh(const BProtoVisualOptionalMesh& source) { *this=source; }
                                                BProtoVisualOptionalMesh& operator=(const BProtoVisualOptionalMesh& source);

      BSimString                                mMeshName;
      long                                      mMeshIndex;
};

//==============================================================================
// BProtoVisualAnim
//==============================================================================
class BProtoVisualAnim
{
   public:
                                                      BProtoVisualAnim();
                                                      BProtoVisualAnim(const BProtoVisualAnim& source) { *this=source; }
                                                      BProtoVisualAnim& operator=(const BProtoVisualAnim& source);

      long                                            mAnimType;
      BProtoVisualAnimExitAction                      mExitAction;
      BSmallDynamicSimArray<BProtoVisualAsset>        mAssets;
      BSmallDynamicSimArray<BProtoVisualAttachment>   mAttachmentLinks;
      BSmallDynamicSimArray<BProtoVisualOptionalMesh> mOptionalMeshes;
};

//==============================================================================
// BProtoVisualModel
//==============================================================================
class BProtoVisualModel
{
   public:
                                                      BProtoVisualModel();
                                                      BProtoVisualModel(const BProtoVisualModel& source) { *this=source; }
                                                      BProtoVisualModel& operator=(const BProtoVisualModel& source);

      BSimString                                      mModelName;
      BProtoVisualLogicNode*                          mpLogicNode;
      BProtoVisualAsset                               mAsset;
      BSmallDynamicSimArray<BProtoVisualAttachment>   mAttachmentLinks;
      BSmallDynamicSimArray<BProtoVisualAnim>         mAnims;
      BSimString                                      mRefModelName;
      long                                            mRefModelIndex;
      BProtoVisualPointArray                          mPoints;
      bool                                            mPointsInitialized;
};

//==============================================================================
// BProtoVisual
//==============================================================================
class BProtoVisual
{
   public:
      enum
      {
         cFlagIsLoaded,
         cFlagAreAllAssetsLoaded,
         cFlagLoadFailed,
         cFlagHasAnimation,
         cFlagHasParticleSystem,
         cFlagHasLight,
         cFlagHasAttachment,
         cFlagHasTechLogic,
         cFlagHasDestructionLogic,
         cFlagHasSquadModeLogic,
         cFlagHasOptionalMesh,
      };

                                                BProtoVisual();
      virtual                                   ~BProtoVisual();

      bool                                      init(const BCHAR_T* pName, long id);
      void                                      deinit();
      
      bool                                      load();
      bool                                      reload();

      void                                      loadAllAssets();
      void                                      unloadAllAssets();

      void                                      calcState(long animationTrack, long animType, long randomTag, int64 userData, BVisualItem* pState, BProtoVisualAnim** ppAnimOut, BVisualItem* pVisualItem, long forceAnimID = -1);
      void                                      calcAttachmentAnimState(long animationTrack, long attachmentModelIndex, long animType, long randomTag, BVisualItem* pAttachmentState);

      bool                                      isLoaded() const   { return getFlag(cFlagIsLoaded); }
      bool                                      areAllAssetsLoaded() const   { return getFlag(cFlagAreAllAssetsLoaded); }
      bool                                      loadFailed() const { return getFlag(cFlagLoadFailed); }

      const BSimString&                         getName() const { return mName; }
      long                                      getID() const { return mID; }

      bool                                      getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void                                      setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }
      
      DWORD                                     getGeneration(void) const { return mGeneration; }      

      const BProtoVisualModel*                  getDefaultModel() const;
      long                                      getModelIndex(const BSimString& modelName) const;

      const BProtoVisualLogicNode*              getLogicNode() const { return (mpLogicNode); }

      BSmallDynamicSimArray<BProtoVisualModel*> mModels;
      
      void                                      recomputeBoundingBox();
      void                                      computeAttackInfo(long protoVisualModelIndex, long animType, float damagePerSecond, bool useDPSasDPA, float coolDownAverage, float& damagePerAttackOut, long& maxNumAttacksPerAnim);
      void                                      computeReloadInfo(long animType, float& reloadTime);
      bool                                      hasAttackTags(long protoVisualModelIndex, long animType);
      long                                      getNumAttackTags(long protoVisualModelIndex, long animType, long animAssetCount, long animAsset);
      void                                      getTechLogicIDs(BSmallDynamicSimArray<long>& techIDs);
      void                                      getBuildingConstructionTags(BProtoVisualTagPtrArray& tags, int targetTag, long animType=cAnimTypeIdle) const;

#ifndef BUILD_FINAL
      void                                      ensureUnloaded() const;
      long                                      getDamageTemplateCount() const;
#endif

      bool                                      findPoints(const BProtoVisualPointArray* pPoints, long &modelIndex) const;
      bool                                      findTags(const BProtoVisualTagArray* pTags, long &modelIndex, long &animIndex, long& assetIndex) const;
      bool                                      findProgression(const BFloatProgression* pProgressions, long &modelIndex, long &animIndex, long& assetIndex) const;

      BProtoVisualPointArray*                   getPoints(long modelIndex) const;
      BProtoVisualTagArray*                     getTags(long modelIndex, long animIndex, long assetIndex) const;
      BFloatProgression*                        getProgression(long modelIndex, long animIndex, long assetIndex) const;

      // SLB: This lets the sim know when a protovisual has changed
      static bool                               mGenerationChanged;

   protected:
      bool                                      loadXML();

      bool                                      saveVIS();
      bool                                      loadVIS();

      void                                      postLoadModel(BProtoVisualModel* pModel);
      void                                      postLoadAttachmentLinks(BProtoVisualModel* pModel, BProtoVisualAttachment* pLink);
      void                                      postLoadLogicNode(BProtoVisualLogicNode* pLogicNode);
      void                                      calcBaseModel(BProtoVisualModel* pModel, long animType, long randomTag, int64 userData, BProtoVisualModel** ppCurrentModel);
      bool                                      calcModelState(long animationTrack, BProtoVisualModel* pModel, BProtoVisualAsset* pParentModelAsset, BProtoVisualAttachment* pAttachment, long animType, long randomTag, int64 userData, long animAssetIndex, BVisualItem* pModelState, BProtoVisualAnim** ppAnimOut, BVisualItem* pVisualItem, long forceAnimID = -1);
      BProtoVisualAsset*                        calcAnimAsset(BProtoVisualModel* pModel, BProtoVisualAsset* pModelAsset, long animType, long randomTag, long animAssetIndex, long* pAnimTypeOut, BProtoVisualAnim** pAnimOut, long* pAnimAssetIndexOut);
      bool                                      checkLogic(BProtoVisualLogicNode* pLogicNode, BProtoVisualLogicValue* pLogicValue);
      void                                      ensureAssetIsLoaded(BProtoVisualAsset* pAsset, bool synced=false);
      void                                      ensureAssetIsLoadedStub(BProtoVisualAsset* pAsset, bool synced=false);
      void                                      ensureAssetIsUnloaded(BProtoVisualAsset* pAsset, BSmallDynamicSimArray<BProtoVisualAttachment>* pAttachments);
      void                                      ensureTagIsLoaded(BProtoVisualTag* pTag);
      long                                      getBoneHandle(BProtoVisualAsset* pAsset, const char* pBoneName);
      long                                      pickRandomAnimAsset(BProtoVisualAnim* pAnim, long randomTag) const;
      void                                      getBoundingBox(BProtoVisualAsset* pModelAsset, BProtoVisualAsset* pAnimAsset, BVector* pMinCorner, BVector* pMaxCorner);
      
      void                                      computeBoundingBox(BProtoVisualModel* pModel);            
      void                                      deinitModel(BProtoVisualModel* pModel);
      void                                      deinitAsset(BProtoVisualAsset* pAsset);
      void                                      initPoints(BProtoVisualModel* pModel, BProtoVisualAsset* pAsset);
      void                                      computeBoundingBox();

      void                                      getTechLogicIDs(BProtoVisualLogicNode* pLogicNode, BSmallDynamicSimArray<long>& techIDs);
      bool                                      getBuildingConstructionTags(BProtoVisualTagPtrArray& tags, BProtoVisualLogicNode* pNode, int targetTag, long animType) const;

      void                                      loadAllLogicNodeAssetsRecurse(BProtoVisualLogicNode* pLogicNode);
      void                                      unloadAllLogicNodeAssetsRecurse(BProtoVisualLogicNode* pLogicNode);

      void                                      lookupAttachmentBoneHandles(BProtoVisualAsset& parentAsset, BSmallDynamicSimArray<BProtoVisualAttachment>& attachments);
      
      long                                      mID;
      BSimString                                mName;
      BProtoVisualLogicNode*                    mpLogicNode;
      int                                       mDefaultModelIndex;

      DWORD                                     mGeneration;
      UTBitVector<16>                           mFlags;      
};

