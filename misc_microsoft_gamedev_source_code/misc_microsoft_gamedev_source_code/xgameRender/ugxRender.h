#if 0
// File: ugxRender.h
#pragma once

#include "threading\eventDispatcher.h"
#include "renderCommand.h"
#include "ugxRenderBase.h"
#include "effect.h"
#include "textureManager.h"

struct granny_file_info;

enum eUGXGeomStatus
{
   cUGXGeomStatusInvalid,
   cUGXGeomStatusPending,
   cUGXGeomStatusReady,
   cUGXGeomStatusFailed,

   cUGXGeomStatusMax,
};

typedef uint64 BUGXGeomHandle;
enum { cInvalidUGXGeomHandle = cInvalidEventReceiverHandle };

class BUGXGeom : public BEventReceiverInterface
{
public:
   BUGXGeom(BEventReceiverHandle ownerHandle);
   ~BUGXGeom();   
      
   BEventReceiverHandle getEventHandle(void) const;
   void setEventHandle(BEventReceiverHandle handle);
   
   BEventReceiverHandle getOwnerEventHandle(void) const;
   void setOwnerEventHandle(BEventReceiverHandle handle);
    
   eUGXGeomStatus getStatus(void);
   
   void render(const BMatrix44& matrix, const BVec4& color);  
      
private:
   BUGXGeomRenderBase mGeomRenderBase;
   BEventReceiverHandle mEventHandle;
   BEventReceiverHandle mOwnerEventHandle;
   eUGXGeomStatus mStatus;

   BFXLEffect mEffect;
   BThreadTextureManager::BHandle mTextureHandle;
   uint mNumEffectsRemaining;
   uint mNumTexturesRemaining;

   void changeStatus(eUGXGeomStatus newStatus);
   
   void checkIfReady(void);
   
   bool init(granny_file_info* pFileInfo, long effectDirID);
         
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

class BUGXGeomManager : public BEventReceiverInterface, BRenderCommandListenerInterface
{
   friend class BUGXGeom;

   struct BRenderInstanceData
   {
      BUGXGeom* mpGeom;
      BMatrix44 mMatrix;
      BVec4 mColor;
   };

   enum eRenderCommandTypes
   {
      cRCDeinit,
      cRCRender,
   };
   
   BUGXGeomManager(const BUGXGeomManager&);
   BUGXGeomManager& operator= (const BUGXGeomManager&);

public:
   BUGXGeomManager();
   ~BUGXGeomManager();
   
   // All public methods callable from the main thread only.
   void init(void);
   void deinit(void);
   
   void setEffectDirID(long dirID);
   
   // pFileInfo must remain stable until you receive an event!
   BEventReceiverHandle addGeom(granny_file_info* pFileInfo, BEventReceiverHandle ownerHandle);
   
   void removeGeom(BEventReceiverHandle handle);
   
   void renderGeom(BEventReceiverHandle handle, const BMatrix44& matrix, const BVec4& color);
   
   // Returns the status of the model, as recorded by the manager in the main thread.
   // This value depends on the update status events received so far, it is not an actual 
   // snapshot of the geom's status.
   eUGXGeomStatus getLastRecordedStatus(BEventReceiverHandle handle);
   
   uint getTotalModels(void) const;
   uint getModelStats(eUGXGeomStatus status) const;
   
   void waitForAllPending(void);
      
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

      BUGXGeom* mpGeom;
      eUGXGeomStatus mStatus;
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
   
   BEventReceiverHandle getEventHandle(void) const { return mEventHandle; }

   uint allocSlot(void);
   void freeSlot(uint i);
   void updateGeomStatus(BEventReceiverHandle handle, eUGXGeomStatus newStatus);
   
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

extern BUGXGeomManager gUGXGeomManager;
#endif