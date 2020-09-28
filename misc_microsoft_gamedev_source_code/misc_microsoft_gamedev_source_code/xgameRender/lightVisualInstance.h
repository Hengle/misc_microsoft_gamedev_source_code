//==============================================================================
// lightVisualInstance.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "visualinstance.h"
#include "sceneLightManager.h"

// Forward declarations
class BLightVisualData;

//==============================================================================
// BLightVisualInstance
//==============================================================================
class BLightVisualInstance : public IVisualInstance
{
   public:
                                 BLightVisualInstance(); 
                                ~BLightVisualInstance();

      bool                       init(BLightVisualData* pData, const BMatrix* pTransform);
      virtual void               deinit();
      virtual void               update(float elapsedTime, bool synced = false);
      virtual void               updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalTransform); 
      virtual void               setSecondaryMatrix(BMatrix matrix) {};
      virtual void               setTintColor(DWORD color) {};
      virtual void               setVisibility(bool bState);
      virtual void               setNearLayer(bool bState) {};
      
      virtual void               render(const BVisualRenderAttributes* pRenderAttributes) { pRenderAttributes; }
      
      virtual void               computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners) { pMinCorner; pMaxCorner; initCorners; }
      virtual long               getBoneHandle(const char* pBoneName) { pBoneName; return -1; }
      virtual bool               getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix=NULL, BBoundingBox* pBox=NULL, const BMatrix* pOffsetMatrix=NULL, bool applyIK=true) {return false;}

      virtual bool               setNumIKNodes(long numNodes)  { numNodes; return false; }
      virtual void               setIKNode(long node, BVector targetPos) { node; targetPos; }
      virtual void               setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType)   { node; boneHandle; linkCount; targetPos; nodeType; }
      virtual BIKNode*           getIKNodeFromTypeBoneHandle(long type, long boneHandle) { return NULL; }
      virtual BIKNode*           getIKNodeFromIndex(long node) { return NULL; }
      virtual void               lockIKNodeToGround(long boneHandle, bool lock, float start, float end) { boneHandle; lock; start; end; }
      virtual long               getIKNodeBoneHandle(long node) const  { node; return -1; }
      virtual bool               getIKNodeAnchor(long node, BVector &anchorPos, float& lockStartTime, float& lockEndTime, bool& lockComplete) const { node; anchorPos; lockStartTime; lockEndTime; lockComplete; return false; }
      virtual void               setIKNodeLockComplete(long node, bool lockComplete) { node; lockComplete; }
      virtual void               setIKNodeSweetSpot(long boneHandle, BVector sweetSpotPos, float start, float sweetSpot, float end) { boneHandle; sweetSpotPos; start; sweetSpot; end; }
      virtual bool               getIKNodeSweetSpot(long node, BVector &sweetSpotPos, float &start, float &sweetSpot, float &end) { node; sweetSpotPos; start; sweetSpot; end; return false; }
      virtual bool               isIKNodeActive(long node) const { return false; }
      virtual void               setIKNodeActive(long node, bool active) { node; active; }
      virtual void               setIKNodeSingleBone(long node, BVector position, BQuaternion orientation) { node; position; orientation; }
      virtual bool               getIKNodeSingleBone(long node, BVector &position, BQuaternion &orientation) { node; position; orientation; return false; }

   protected:
      BLocalLightHandle          mLightHandle;
      BLightVisualData*          mpData;
};
