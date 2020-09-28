//============================================================================
// File: particlegateway.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#pragma once
#include "visualinstance.h"
#include "ParticleSystemManager.h"

class BParticleEffect;

//============================================================================
// class BParticleInstance
//============================================================================
class BParticleInstance : public IVisualInstance
{
public:
   BParticleInstance();
   ~BParticleInstance();

   //-- visual instance interface 
   virtual void deinit();
   virtual void update(float elapsedTime, bool synced = false);      
   virtual void updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix);
   virtual void setSecondaryMatrix(BMatrix matrix);
   virtual void setTintColor(DWORD color);
   virtual void setVisibility(bool bState);   
   virtual void setNearLayer(bool bState);
   virtual void render(const BVisualRenderAttributes* pRenderAttributes);
   virtual void computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners);
   virtual long getBoneHandle(const char* pBoneName);
   virtual bool getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix=NULL, BBoundingBox* pBox=NULL, const BMatrix* pOffsetMatrix=NULL, bool applyIK=true);
   virtual bool setNumIKNodes(long numNodes)  { numNodes; return false; }
   virtual void setIKNode(long node, BVector targetPos)   { node; targetPos; }
   virtual void setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType)  { node; boneHandle; linkCount; targetPos; nodeType; }
   virtual BIKNode* getIKNodeFromTypeBoneHandle(long type, long boneHandle) { return NULL; }
   virtual BIKNode* getIKNodeFromIndex(long node) { return NULL; }
   virtual void lockIKNodeToGround(long boneHandle, bool lock, float start, float end) { boneHandle; lock; start; end; }
   virtual long getIKNodeBoneHandle(long node) const  { node; return -1; }
   virtual bool getIKNodeAnchor(long node, BVector &anchorPos, float& lockStartTime, float& lockEndTime, bool& lockComplete) const { node; anchorPos; lockStartTime; lockEndTime; lockComplete; return false; }
   virtual void setIKNodeLockComplete(long node, bool lockComplete) { node; lockComplete; }
   virtual void setIKNodeSweetSpot(long boneHandle, BVector sweetSpotPos, float start, float sweetSpot, float end) { boneHandle; sweetSpotPos; start; sweetSpot; end; }
   virtual bool getIKNodeSweetSpot(long node, BVector &sweetSpotPos, float &start, float &sweetSpot, float &end) { node; sweetSpotPos; start; sweetSpot; end; return false; }
   virtual bool isIKNodeActive(long node) const { return false; }
   virtual void setIKNodeActive(long node, bool active) { node; active; }
   virtual void setIKNodeSingleBone(long node, BVector position, BQuaternion orientation) { node; position; orientation; }
   virtual bool getIKNodeSingleBone(long node, BVector &position, BQuaternion &orientation) { node; position; orientation; return false; }

   int mInstanceSlotIndex;
};

//============================================================================
// class BParticleGateway
//============================================================================
class BParticleGateway : public BRenderCommandListener, public BEventReceiver
{
public:
   BParticleGateway();
   ~BParticleGateway();
   
   void init(void);
   void deinit(void);
   void update();
      
   void getOrCreateData(const char* pName, BParticleEffectDataHandle& dataHandle);
   
   //BParticleInstance* createInstance(BParticleEffectDataHandle dataHandle, BMatrix matrix);   
   BParticleInstance* createInstance(const BParticleCreateParams& params);
   void createAutoReleaseInstance(const BParticleCreateParams& params);
   void updateInstanceMatrix(const BParticleInstance* pInstance, const BMatrix* pMatrix, const BMatrix* pLocalMatrix);
   void setSecondaryInstanceMatrix(const BParticleInstance* pInstance, BMatrix* pMatrix);
   void setInstanceFlag(const BParticleInstance* pInstance, int flag, bool bState);
   void releaseInstance(BParticleInstance* pInstance, bool bKillImmediately, bool bAutoRelease = false);
   void setTintColor(BParticleInstance* pInstance, DWORD color);
      
   void releaseAllInstances(void);
   void releaseAllDataSlots(void);

   void initMemoryPools();
   void deinitMemoryPools();
   void initTextureSystem();
   void deinitTextureSystem();

   void pauseUpdate(bool pause);
   void enableBBoxRendering(bool enable);
   void enableDistanceFade(bool enable);
   void enableMagnetRendering(bool enable);
   void enableCulling(bool enable);
   void setTimeSpeed(float speed);
   void setDistanceFade(float startDistance, float endDistance);
   void setForceEmitterKillNow(bool killNow);
   

   uint getTotalActiveInstances(void)     { ASSERT_MAIN_THREAD return mTotalActiveInstances; }
   uint getNumInstanceSlotsFree(void)     { ASSERT_MAIN_THREAD return mNumInstanceSlotsFree; }
   uint getNumInstanceHighWaterMark(void) { ASSERT_MAIN_THREAD return mInstanceSlotsHighWaterMark; }
   uint getNumDataSlotsInUse(void)        { ASSERT_MAIN_THREAD return mNumDataSlotsInUse; }

   long getDataHandleForInstance(const BParticleInstance* pInstance);

   const BSimString* getDataName(BParticleEffectDataHandle dataHandle) const;

private:
   enum eSlotStatus
   {
      cSSFree,
      
      cSSPending,
      cSSFailed,
      cSSValid,
                  
      cSSTotal
   };
   
   struct BDataSlot
   {
      // All member variables that don't start with "mRender" can generally only be manipulated from the sim thread!
      BSimString mName;
     
      // mRenderDataIndex can only be read/written from the render thread! 
      int mRenderDataIndex;
         
      eSlotStatus mStatus;
      bool mBeingDeleted : 1;
   };
   
   // Can't use a dynamic array because this array will be read/written by multiple threads.
   enum { cMaxDataSlots = 512 };
   BDataSlot mDataSlots[cMaxDataSlots];
   uint mNumDataSlotsInUse;
            
   struct BInstanceSlot
   {
      // All member variables that don't start with "mRender" can generally only be manipulated from the sim thread!
      union
      {
         BParticleEffectDataHandle mDataSlotIndex;
         int mNextFreeInstanceSlotIndex;
      };
      
      BParticleInstance* mpInstance;
      
      // mpRenderEffect can only be read/written from the render thread!
      BParticleEffect* mpRenderEffect;
            
      uchar mStatus;
      bool mBeingDeleted : 1;
   };
         
   // Can't use a dynamic array because this array will be read/written by multiple threads.
   enum { cMaxInstanceSlots = 8192 };
   BInstanceSlot mInstanceSlots[cMaxInstanceSlots];
   BDynamicSimArray<int> mAutoReleaseInstances;
   uint mInstanceSlotsHighWaterMark;
   int mFirstFreeInstanceSlotIndex;
   uint mNumInstanceSlotsFree;
   
   uint mTotalActiveInstances;
   
   bool mInitialized : 1;
    
   struct BInitInstanceSlotData
   {      
      BParticleCreateParams mParams;
      uint                  mInstanceSlotIndex;
      /*
      BMatrix mMatrix;
      uint    mInstanceSlotIndex;
      bool    mNearLayerEffect:1;
      */
   };

   struct BUpdateInstanceMatrixData
   {
      BMatrix mMatrix;
      BMatrix mLocalMatrix;
      uint mInstanceSlotIndex;
      bool mHasLocalMatrix;
   };

   struct BSetSecondaryInstanceMatrixData
   {
      BMatrix mMatrix;      
      uint mInstanceSlotIndex;      
   };

   struct BSetTintColorData
   {
      DWORD mColor;      
      uint  mInstanceSlotIndex;
   };

   struct BUpdateInstanceFlagData
   {
      int  mFlag;
      uint mInstanceSlotIndex;
      bool mbState;
   };

   struct BReleaseInstanceData
   {
      uint mInstanceSlotIndex;
      bool mbKillImmediately:1;
      bool mbAutoRelease:1;
   };

   struct BSetDistanceFadeData
   {
      float mStartDistance;
      float mEndDistance;
   };
   
   enum 
   {
      cRCInitDataSlot,
            
      cRCInitInstanceSlot,
      cRCUpdateInstanceMatrix,
      cRCDeleteInstanceSlot,
      cRCPauseUpdate,
      cRCEnableBBoxRendering,
      cRCEnableDistanceFade,
      cRCSetInstanceFlag,
      cRCSetTimeSpeed,
      cRCSetSecondaryMatrix,
      cRCSetDistanceFade,
      cRCEnableMagnetRendering,
      cRCDestroyAllParticleInstances,
      cRCSetTintColor,
      cRCInitMemoryPools,
      cRCDeinitMemoryPools,
      cRCEnableCulling,
      cRCDestroyAllParticleData,
      cRCInitTextureSystem,
      cRCDeinitTextureSystem
   };
   
   enum 
   {
      cECInitDataSlotReply = cEventClassFirstUser,
      cECInitInstanceSlotReply,
      cECDeleteInstanceSlotReply
   };
   
   void clear(void);
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};

//============================================================================
// externs
//============================================================================
extern BParticleGateway gParticleGateway;


