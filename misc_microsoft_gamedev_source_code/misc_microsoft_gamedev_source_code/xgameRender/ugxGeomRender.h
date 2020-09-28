//============================================================================
// File: ugxGeomRender.h
//============================================================================
#pragma once

#include "threading\eventDispatcher.h"
#include "resource\ecfFileData.h"
#include "renderCommand.h"
#include "ugxGeomData.h"
#include "effect.h"
#include "D3DTextureManager.h"
#include "ugxGeomRenderTypes.h"
#include "asyncFileManager.h"
#include "ugxGeomSectionRenderer.h"
#include "worldVisibility.h"

struct granny_file;

class BUGXGeomRenderSection;

//============================================================================
// class BUGXGeomRender
// This class is only usable from the render thread.
//============================================================================
class BUGXGeomRender : public BEventReceiverInterface
{
public:
   BUGXGeomRender(BEventReceiverHandle ownerHandle = cInvalidEventReceiverHandle, BEventReceiverHandle managerHandle = cInvalidEventReceiverHandle);
   ~BUGXGeomRender();   
      
   BEventReceiverHandle getEventHandle(void) const;
   void setEventHandle(BEventReceiverHandle handle);
   
   BEventReceiverHandle getOwnerEventHandle(void) const;
   void setOwnerEventHandle(BEventReceiverHandle handle);
   
   BEventReceiverHandle getManagerEventHandle(void) const;
   void setManagerEventHandle(BEventReceiverHandle handle);
    
   eUGXGeomStatus getStatus(void);
   
   static void globalRenderBegin(double gameTime, void* pGPUFrameStorageBones, uint GPUFrameStorageBonesSize, eUGXGeomPass pass, BManagedTextureHandle globalEnvMap);
            
   void renderBegin(BUGXGeomRenderCommonInstanceData* pCommonData);
                  
   void render(const BUGXGeomRenderMeshMask& meshMask, const BUGXGeomRenderPerInstanceData* pInstances, uint numInstances);
         
   void renderEnd(void);   
      
   static void globalRenderEnd(eUGXGeomPass pass);
   
   eUGXGeomRenderFlags getRenderFlags(void) const { return mRenderFlags; }
   uint getNumBones(void) const { return mNumBones; }
         
   uint getLayerFlags(void) const { return mLayerFlags; }
      
   const BUGXGeomData& getGeomData(void) const { return mGeomData; }
   
   IUGXGeomSectionRendererManager* getSectionRendererManager() const { return mpSectionRendererManager; }
   IUGXGeomSectionRendererArray*   getSectionRendererArray() const { return mpSectionRendererArray; }
   
   //this will return false if the object is not large
   bool isLargeObjectSectionVisible(const XMMATRIX& worldMatrix, const BVolumeCuller& pCuller);

   // Static methods
   
   static void setVisMode(eUGXGeomVisMode visMode) { gVisMode = visMode; }
   static eUGXGeomVisMode getVisMode(void) { return gVisMode; }

   static void setTextureMode(eUGXGeomTextureMode textureMode) { gTextureMode = textureMode; }
   static eUGXGeomTextureMode getTextureMode(void) { return gTextureMode; }

   static void clearTotalDraws(void) { gTotalDraws = 0; }
   static uint getTotalDraws(void) { return gTotalDraws; }
   
   static void setBlackmapParams(BBlackmapParams& params) { gBlackmapParams = params; }
   static void clearBlackmapParams(void) { gBlackmapParams.clear(); }

#ifndef BUILD_FINAL   
   struct BStats 
   {
      BStats() { clear(); }
      
      void clear(void) { Utils::ClearObj(*this); }
      
      uint mTotalGlobalBegins;
      uint mTotalInstanceRenders;
      uint mTotalLargeRenders;
      uint mTotalDraws;
      uint mTotalInstances;
      uint mTotalInstanceSectionsRendered;
      uint mTotalLargeNodeCullTests;
      uint mTotalLargeNodeCullPasses;
      uint mTotalLargeSectionsRendered;
   };
   static BStats& getStats(void) { return gStats; }
#endif   
   
   static void tickDebugBoundingBoxes(bool boundingBoxesEnabled);
      
private:
   // Member variables
   IUGXGeomSectionRendererManager*                 mpSectionRendererManager;
   IUGXGeomSectionRendererArray*                   mpSectionRendererArray;
   
   BAABBTree                                       mAABBTree;
         
   uint                                            mNumBones;
   eUGXGeomRenderFlags                             mRenderFlags;
   
   BUGXGeomData                                    mGeomData;
         
   BEventReceiverHandle                            mEventHandle;
   BEventReceiverHandle                            mOwnerEventHandle;
   BEventReceiverHandle                            mManagerEventHandle;
         
   eUGXGeomStatus                                  mStatus;         
         
   uchar                                           mLayerFlags;
   bool                                            mRenderBegun : 1;
      
   // Statics
   static eUGXGeomVisMode                          gVisMode;
   static BBlackmapParams                          gBlackmapParams;
   
   static BUGXGeomRenderCommonInstanceData*        gpRenderCommonData;   
   
   static IDirect3DVertexBuffer9                   gBoneVB;
   static XMVECTOR                                 gInstanceControlRegs[4];
   static uint                                     gMaxInstancesPerDraw;
   static uint                                     gMaxInstancesPerDrawShift;
   static uint                                     gLightRegsPerInstance;
   static eUGXGeomPass                             gCurGlobalPass;
   static IUGXGeomSectionRendererManager*          gpCurSectionRendererManager;
   static double                                   gGameTime;
   static BManagedTextureHandle                    gGlobalEnvMap;
   static eUGXGeomTextureMode                      gTextureMode;
   static uint                                     gTotalDraws;
   static void*                                    gpGPUFrameStorageBones;
   static uint                                     gGPUFrameStorageBonesSize;

#ifndef BUILD_FINAL   
   static BDynamicRenderArray<AABB>                gDebugBoundingBoxes;
   static bool                                     gDebugBoundingBoxesEnabled;
   static BStats                                   gStats;
#endif   
                     
   void                 changeStatus(eUGXGeomStatus newStatus);
   BUGXGeomRenderInfo*  createRenderInfo(void);
   bool                 init(BECFFileData* pECFFileData, eUGXGeomRenderFlags renderFlags);
   
   XMMATRIX             getBoneMatrix(uint instanceBoneVertexIndex, int boneIndex);
   void                 renderLargeModel(const BUGXGeomRenderPerInstanceData& instance);
   void                 renderWithInstancing(BUGXGeomRenderMeshMask meshMask, const BUGXGeomRenderPerInstanceData* pInstances, uint numInstances);
   
   struct BSectionToRender
   {
      BSectionToRender() { }
      
      BSectionToRender(uint sectionIndex, const AABB& worldBounds, uint tileFlags) : 
         mSectionIndex(static_cast<uint16>(sectionIndex)), 
         mWorldBounds(worldBounds),
         mTileFlags(static_cast<uchar>(tileFlags))
      {
      }

      AABB     mWorldBounds;
      uint16   mSectionIndex;
      uchar    mTileFlags;
            
      bool operator< (const BSectionToRender& rhs) const { return mSectionIndex < rhs.mSectionIndex; }
   };
   typedef BStaticArray<BSectionToRender, 512> BSectionToRenderArray;
   
   bool                 shouldRenderSection(const IUGXGeomSectionRenderer& section);
   void                 cullSections(const BUGXGeomRenderPerInstanceData& instance, BSectionToRenderArray& sectionsToRender);
   void                 createSectionIndices(BStaticArray<uint16, 128>& sectionIndices, const BUGXGeomRenderMeshMask& meshMask);
   
   static void getLight(XMVECTOR* pDst, const XMVECTOR* pSrcTexels, int lightIndex);
   static void setLightGroup(uint dstReg, const short* pVisibleLightIndices);
         
   virtual bool         receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};

