//============================================================================
//
//  File: ugxGeomManager.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "ugxGeomRender.h"
#include "containers\nameValueMap.h"
#include "effectIntrinsicManager.h"
#include "renderStateFilter.h"

typedef uint64 BUGXGeomHandle;
enum { cInvalidUGXGeomHandle = cInvalidEventReceiverHandle };

//============================================================================
// class BUGXGeomManager
//============================================================================
class BUGXGeomManager : public BEventReceiverInterface, BRenderCommandListenerInterface
{
   struct BRenderBeginData
   {
      BUGXGeomRender* mpGeomRender;
      BUGXGeomRenderCommonInstanceData mCommonData;
   };
      
   enum eRenderCommandTypes
   {
      cRCDeinit,
      cRCSetOptions
   };

   BUGXGeomManager(const BUGXGeomManager&);
   BUGXGeomManager& operator= (const BUGXGeomManager&);

public:
   BUGXGeomManager();
   ~BUGXGeomManager();

   // All public methods callable from the main thread only unless otherwise indicated.
   void init(long effectDirID);
   void deinit(void);

   void reset();

   // Main thread
   void setOptions(const BNameValueMap& nameValueMap);
   
   // Main or render threads.
   const BNameValueMap& getOptions(void) const;

   void setEffectDirID(long dirID);
   long getEffectDirID(void) { return mEffectDirID; }
      
   // pECFData must have been allocated from the render heap.
   // Takes ownership of the ECF data!
   BEventReceiverHandle addGeom(BECFFileData* pECFData, BEventReceiverHandle ownerHandle, eUGXGeomRenderFlags renderFlags);

   void remove(BEventReceiverHandle handle);
         
   // Returns the status of the model, as recorded by the manager in the main thread.
   // This value depends on the update status events received so far, it is not an actual 
   // snapshot of the geom's status.
   eUGXGeomStatus getLastRecordedStatus(BEventReceiverHandle handle);
      
   uint getTotalModels(void) const;
   uint getModelStats(eUGXGeomStatus status) const;

   void waitForAllPending(void);
   
   BEventReceiverHandle getEventHandle(void) const { return mEventHandle; }
         
   // Render thread only.
   BUGXGeomRender* getGeomRenderByHandle(BEventReceiverHandle handle) const;
   BUGXGeomRender* getGeomRenderByIndex(uint slotIndex) const;
         
   IDirect3DDevice9* getCommandBufferDevice(void) { return mpCommandBufferDevice; }
   
   const BFXLEffectIntrinsicPool& getIntrinsicPool(void) const { return mIntrinsicPool; }
         BFXLEffectIntrinsicPool& getIntrinsicPool(void)       { return mIntrinsicPool; }
                  
private:
   struct BUGXGeomSlot
   {
      BUGXGeomSlot() { clear(); }

      void clear(void)
      {
         mpGeom = NULL;
         mStatus = cUGXGeomStatusInvalid;
         mBeingDeleted = false;
      }

      BUGXGeomRender* mpGeom;
      eUGXGeomStatus mStatus;
      
      // rg - mBeingDeleted is a flag of questionable value.
      bool mBeingDeleted;
   };

   typedef BDynamicArray<BUGXGeomSlot> BUGXGeomSlotArray;
   // mGeomSlots is only accessible from the main thread!
   BUGXGeomSlotArray mGeomSlots;          

   long mEffectDirID;

   BEventReceiverHandle mEventHandle;
   BCommandListenerHandle mCommandListenerHandle;

   uint mTotalModels;
   uint mModelStats[cUGXGeomStatusMax];
   
   BNameValueMap mSimOptions;
   BNameValueMap mRenderOptions;
   
   IDirect3DDevice9* mpCommandBufferDevice;
   BFXLEffectIntrinsicPool mIntrinsicPool;
                        
   uint allocSlot(void);
   void freeSlot(uint i);
   void updateGeomStatus(BEventReceiverHandle handle, eUGXGeomStatus newStatus);

#if 0   
   struct BGlobalRenderBeginData
   {
      void* mpGPUFrameStorageBase;
      eUGXGeomPass mPass;
      eUGXGeomVisMode mVisMode;
   };
#endif   
   
   struct BGlobalRenderEndData
   {
      eUGXGeomPass mPass;
   };

   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

//============================================================================
// externs
//============================================================================
extern BUGXGeomManager     gUGXGeomManager;
extern BRenderStateFilter* gpUGXGeomRSFilter;