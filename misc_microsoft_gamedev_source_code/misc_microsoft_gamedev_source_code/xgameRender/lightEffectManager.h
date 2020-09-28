//==============================================================================
// File: lightEffectManager.h
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================
#pragma once
#include "lightEffect.h"
#include "visualInstance.h"

//============================================================================
// class BLightEffectVisualInstance
//============================================================================
class BLightEffectVisualInstance : public IVisualInstance
{
public:
   BLightEffectVisualInstance();
   ~BLightEffectVisualInstance();

   //-- visual instance interface 
   virtual void deinit();
   virtual void update(float elapsedTime, bool synced = false);      
   virtual void updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix);
   virtual void setSecondaryMatrix(BMatrix matrix) {};
   virtual void setVisibility(bool bState);
   virtual void setNearLayer(bool bState) {};
   virtual void setTintColor(DWORD color) {};
   virtual void render(const BVisualRenderAttributes* pRenderAttributes);
   virtual void computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners);
   virtual long getBoneHandle(const char* pBoneName);
   virtual bool getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix=NULL, BBoundingBox* pBox=NULL, const BMatrix* pOffsetMatrix=NULL, bool applyIK=true);
   virtual bool setNumIKNodes(long numNodes)  { numNodes; return false; }
   virtual void setIKNode(long node, BVector targetPos)   { node; targetPos; }
   virtual void setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType) { node; boneHandle; linkCount; targetPos; nodeType; }
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

   float mElapsedTime;
   BLightEffectInstance mInstance;
};

//============================================================================
// class BTimedLightInstance
//============================================================================
class BTimedLightInstance
{
   public:
      BTimedLightInstance() : mInstance(NULL), mEndTime(0) {}
      ~BTimedLightInstance() {}

      BMatrix                     mWorldMtx;
      BLightEffectVisualInstance* mInstance;
      DWORD                       mEndTime;
};

//============================================================================
// class BLightEffectManager
//============================================================================
class BLightEffectManager
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
public:
   BLightEffectManager();
   ~BLightEffectManager();
   
   void init(long dirID = 0);
   void deinit(void);
   void clear(void);

   void update(DWORD gameTime, float elapsedTime);
   
   void setIntensityScale(float scale) { mIntensityScale = scale; }
   float getIntensityScale(void) { return mIntensityScale; }
   
   void getOrCreateData(const char* pName, long& dataHandle);
   
   BLightEffectVisualInstance* createInstance(int dataHandle, BMatrix matrix);   
   void createTimedInstance(int dataHandle, BMatrix matrix, DWORD endTime);
   BCameraEffectInstance* createCameraInstance(int dataHandle, BMatrix matrix);  
   void releaseInstance(BLightEffectVisualInstance* pInstance);
   void releaseCameraInstance(BCameraEffectInstance* pInstance);

   void releaseAllInstances(void);
   
   uint getNumLightEffects(void) const { return mEffects.getSize(); }
   uint getNumLightEffectInstances(void) const { return mInstances.getSize(); }
   uint getNumCameraEffects(void) const { return mCameraInstances.getSize(); }
   const BLightEffectData* getLightEffect(long dataHandle) const;
   
private:
   long mDirID;
   BDynamicArray<BLightEffectData*>           mEffects;

   BDynamicArray<BLightEffectVisualInstance*> mInstances;
   BDynamicArray<BCameraEffectInstance*>      mCameraInstances;

   BDynamicArray<BTimedLightInstance>         mTimedInstances;
   
   float                                      mIntensityScale;
   
   void reloadEffect(int dataHandle);
#ifdef ENABLE_RELOAD_MANAGER
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
};

extern BLightEffectManager gLightEffectManager;
