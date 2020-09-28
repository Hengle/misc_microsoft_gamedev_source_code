//==============================================================================
// visualinstance.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "visualrenderattributes.h"
#include "Quaternion.h"

// Forward declarations
class BBoundingBox;
class BIKNode;

//==============================================================================
// IVisualInstance
//==============================================================================
class IVisualInstance
{
   public:
      // rg [9/11/06] - Is deinit() ever called?
      virtual void               deinit()=0;
      
      virtual void               update(float elapsedTime, bool synced = false)=0;      
      virtual void               updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix)=0;
      virtual void               setSecondaryMatrix(BMatrix matrix) = 0;
      virtual void               setVisibility(bool bState) = 0;
      virtual void               setNearLayer(bool bState) = 0;
      virtual void               setTintColor(DWORD color) = 0;
      
      // rg [3/5/06] - render() shouldn't actually render anything to the backbuffer, it should queue up all render calls
      // for later flushing, like BGrannyInstanceRenderer.
      virtual void               render(const BVisualRenderAttributes* pRenderAttributes)=0;            
      virtual void               computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners)=0;
      virtual long               getBoneHandle(const char* pBoneName)=0;
      virtual bool               getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix=NULL, BBoundingBox* pBox=NULL, const BMatrix* pOffsetMatrix=NULL, bool applyIK=true)=0;
      virtual bool               getBoneForRender(long boneHandle, BMatrix &matrix) {boneHandle; matrix; return(false);}  // jce [11/3/2008] -- this will only work properly for a short-interval during rendering (i.e. not for general use)

      virtual bool               setNumIKNodes(long numNodes)=0;
      virtual void               setIKNode(long node, BVector targetPos)=0;
      virtual void               setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType)=0;
      virtual BIKNode*           getIKNodeFromTypeBoneHandle(long type, long boneHandle)=0;
      virtual BIKNode*           getIKNodeFromIndex(long node)=0;
      virtual void               lockIKNodeToGround(long boneHandle, bool lock, float start, float end)=0;
      virtual long               getIKNodeBoneHandle(long node) const=0;
      virtual bool               getIKNodeAnchor(long node, BVector &anchorPos, float& lockStartTime, float& lockEndTime, bool& lockComplete) const=0;
      virtual void               setIKNodeLockComplete(long node, bool lockComplete)=0;
      virtual void               setIKNodeSweetSpot(long boneHandle, BVector sweetSpotPos, float start, float sweetSpot, float end)=0;
      virtual bool               getIKNodeSweetSpot(long node, BVector &sweetSpotPos, float &start, float &sweetSpot, float &end)=0;
      virtual bool               isIKNodeActive(long node) const=0;
      virtual void               setIKNodeActive(long node, bool active)=0;
      virtual void               setIKNodeSingleBone(long node, BVector position, BQuaternion orientation)=0;
      virtual bool               getIKNodeSingleBone(long node, BVector &position, BQuaternion &orientation)=0;

protected:

};

