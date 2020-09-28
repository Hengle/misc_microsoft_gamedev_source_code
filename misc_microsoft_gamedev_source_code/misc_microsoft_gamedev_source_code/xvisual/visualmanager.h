//==============================================================================
// visualmanager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "string/stringTable.h"
#include "visualrenderattributes.h"

// Forward declarations
class BProtoVisual;
class BProtoVisualLogicNode;
class BProtoVisualTag;
class BVisual;
class BVisualItem;
struct BProtoVisualAnimExitAction;
class BGrannyInstance;

class BDamageTemplate;

//==============================================================================
// IProtoVisualHandler
//==============================================================================
class IProtoVisualHandler
{
   public:
      virtual bool            getVisualLogicValue(long logicType, const char* pName, DWORD& valDword, float& valFloat) const=0;
      virtual void            handleProtoVisualLoaded(BProtoVisual* pProtoVisual)=0;
};

//============================================================================
//  Class BDamageTemplateInterface
//============================================================================
class BDamageTemplateInterface
{
   public:
      virtual long  getOrCreateDamageTemplate(const BCHAR_T* pFileName, long modelindex)=0;
};


//==============================================================================
// IVisualHandler
//==============================================================================
class IVisualHandler
{
   public:
      virtual bool            handleAnimEvent(long attachmentHandle, long animType, long eventType, int64 userData, BProtoVisualTag* pTag, BVisual* pVisual)=0;
      virtual bool            handleSetAnimSync(int64 userData, long animationTrack, long animType, bool applyInstantly, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, const BProtoVisualAnimExitAction* pOverrideExitAction, BVisual* pVisual)=0;
      virtual long            handleVisualLogic(BProtoVisualLogicNode* pLogicNode, long randomTag, int64 userData, BProtoVisual* pProtoVisual)=0;
      virtual long            handleVisualPoint(long pointType, const BSimString& dataName)=0;
      virtual long            getRandomValue(long randomTag, long minVal, long maxVal)=0;
};


//==============================================================================
//==============================================================================
class BVisualRenderEntry
{
   public:
      BVisualItem             *mItem;
      BVisualRenderAttributes mVisualRenderAttributes;
      BMatrix                 mWorldMatrix;
};


//==============================================================================
//==============================================================================
class BGrannyPoseWorkEntry
{
   public:
      BDynamicArray<BGrannyInstance*> mInstances;
};


//==============================================================================
// BVisualManager
//==============================================================================
class BVisualManager
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
   public:
                              BVisualManager();
                              ~BVisualManager();

      bool                    init(long dirID, const BCHAR_T* pDirName);
      void                    deinit();

      void                    gameInit();
            
      long                    getDirID() const { return mDirID; }
            
      bool                    getVisualLogicValue(long logicType, const char* pName, DWORD& valDword, float& valFloat) const;

      // Polls to check if any managers are reporting that one or more model instances are "dirty" due to a reload.
      // If so all proto visual bounding boxes are recomputed.
      void                    update(void);

      // The name should not have any extension!
      long                    getOrCreateProtoVisual(const BCHAR_T* pName, bool loadFile);
      long                    findProtoVisual(const BCHAR_T* pName);
      long                    createProtoVisual(const BCHAR_T* pName);
      BProtoVisual*           getProtoVisual(long index, bool ensureLoaded);
      long                    getNumProtoVisuals() const { return mProtoVisualList.getNumber(); }

      BVisual*                createVisual(long protoIndex, bool synced, int64 userData, DWORD tintColor, const BMatrix& worldMatrix, int displayPriority);
      BVisual*                createVisual(const BVisual* pSource, bool synced, int64 userData, DWORD tintColor, const BMatrix& worldMatrix);
      void                    releaseVisual(BVisual* pVisual);
      long                    getNumVisuals() const;

      void                    addVisualHandler(IVisualHandler* handler) { mVisualHandlers.add(handler); }
      void                    removeVisualHandler(IVisualHandler* handler) { mVisualHandlers.remove(handler); }

      void                    addProtoVisualHandler(IProtoVisualHandler* handler) { mProtoVisualHandlers.add(handler); }
      void                    removeProtoVisualHandler(IProtoVisualHandler* handler) { mProtoVisualHandlers.remove(handler); }

      bool                    handleAnimEvent(long attachmentHandle, long animType, BVisual* pVisual, BProtoVisualTag* pTag);
      void                    handleSetAnimSync(BVisual* pVisual, long animationTrack, long animType, bool applyInstantly, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, const BProtoVisualAnimExitAction* pOverrideExitAction);
      long                    handleVisualLogic(BProtoVisualLogicNode* pLogicNode, long randomTag, int64 userData, BProtoVisual* pProtoVisual);
      long                    handleVisualPoint(long pointType, const BSimString& data);
      long                    getRandomValue(long randomTag, long minVal, long maxVal);
      void                    handleProtoVisualLoaded(BProtoVisual* pProtoVisual);

      int                     getNumAnimTypes() const { return mAnimTypeTable.getTags().getNumber(); }
      long                    getAnimType(const BSimString& name);
      long                    getAnimType(const BSimString& name, bool &found);
      const char*             getAnimName(long type) const;

      int                     getNumAttachmentTypes() const { return mAttachmentTypeTable.getTags().getNumber(); }
      long                    getAttachmentType(const BSimString& name);
      const char*             getAttachmentName(long type) const;


      void                                registerDamageTemplateHandler(BDamageTemplateInterface *pInterface)  { mpDamageTemplateInterface = pInterface; }
      BDamageTemplateInterface            *getDamageTemplateInterface( void ) const { return mpDamageTemplateInterface; }

#ifdef ENABLE_RELOAD_MANAGER
      virtual bool            receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif

      bool                    unloadAll();
      
      // jce [11/3/2008] -- New cache functionality to allow deformation to work correctly in interpolated case.  To minimize code change, the render
      // will queue things to here and then it will process them all.  If we could have gone back in time, it would have been better to restructure everything.
      void                    beginRenderPrepare();
      void                    queueVisualItemForRender(BVisualItem *item, BVisualRenderAttributes* renderAttributes);
      void                    queueGrannyInstanceForSampling(BGrannyInstance *instance);  // jce [11/3/2008] -- This is meant to be called by BVisualItem::renderPrepare
      void                    processVisualRenderQueue();
      void                    endRenderPrepare();
      static void             grannyPoseCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);

#ifndef BUILD_FINAL
      void                    ensureUnloaded() const;
      long                    getDamageTemplateCount() const;
#endif

      void                    saveState();
      void                    restoreState();

      bool                    saveVisual(BStream* pStream, int saveType, const BVisual* pVisual) const;
      bool                    loadVisual(BStream* pStream, int saveType, BVisual** ppVisual, BMatrix& worldMatrix, DWORD tintColor);

   protected:
      long                                mDirID;
      BDynamicSimArray<BProtoVisual*>     mProtoVisualList;
      
      // rg [4/19/07] - I've changed mProtoVisualTable to not be case sensitive.      
      BStringTable<long, false, 1024>      mProtoVisualTable;
      
      BDynamicSimArray<IVisualHandler*>   mVisualHandlers;
      BDynamicSimArray<IProtoVisualHandler*> mProtoVisualHandlers;
      BStringTable<long, false, 1024, hash, BSimString> mAnimTypeTable;
      long                                mNextAnimType;
      BStringTable<long, false, 2048, hash, BSimString> mAttachmentTypeTable;
      long                                mNextAttachmentType;
      
      void                    recomputeProtoVisualBoundingBoxes(void);

      
      BDamageTemplateInterface      *mpDamageTemplateInterface;

      // jce [11/3/2008] -- New cache functionality to allow deformation to work correctly in interpolated case.  To minimize code change, the render
      // will queue things to here and then it will process them all.  If we could have gone back in time, it would have been better to restructure everything.
      BDynamicSimArray<BVisualRenderEntry>      mVisualRenderQueue;
      BDynamicSimArray<BGrannyInstance *>       mVisualRenderGrannyQueue;

      // jce [11/4/2008] -- Used for multi-threaded posing.
      BDynamicArray<BGrannyPoseWorkEntry, 32>   mGrannyPoseWorkEntries;
      BCountDownEvent                           mGrannyPoseRemainingBuckets;


      // SLB: This is made redundant by Sergio's changes.
//       long     mSavedState_ProtoVisualListCount;
//       long     mSavedState_AnimTypeCount;
//       long     mSavedState_AttachmentTypeCount;
};

// Global variable for the one BVisualManager object
extern BVisualManager gVisualManager;

