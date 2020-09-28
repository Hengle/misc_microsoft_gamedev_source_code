//============================================================================
// grannymodel.h
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================
#pragma once

// Includes
#include <granny.h>
#include "render.h"
#include "effect.h"
#include "ugxGeomManager.h"
#include "bitarray.h"

#define cNumTrackMasks 2

//============================================================================
// Indices of the sections of the Granny model file.
//============================================================================
enum BGrannyModelSectionIndices
{
   cGrannyModelDefaultSection  = 0,
   cGrannyModelVertexSection   = 1,
   cGrannyModelMaterialSection = 2
};

//============================================================================
// Bone handles
//============================================================================
enum
{
   cGrannyBoneHandleBoneMask    = 0x0000FFFF,
   cGrannyBoneHandleMeshMask    = 0x00FF0000,
   cGrannyBoneHandleBindingMask = 0xFF000000,
};
#define BONEFROMGRANNYBONEHANDLE(n)             ((n)&cGrannyBoneHandleBoneMask)
#define MESHFROMGRANNYBONEHANDLE(n)             (((n)&cGrannyBoneHandleMeshMask)>>16==255?-1:((n)&cGrannyBoneHandleMeshMask)>>16)
#define BINDINGFROMGRANNYBONEHANDLE(n)          (((n)&cGrannyBoneHandleBindingMask)>>24) 
#define GRANNYBONEHANDLE(bone, mesh, binding)   (((binding)<<24)|((mesh)<<16)|bone)

//============================================================================
// BGrannyModel
//============================================================================
class BGrannyModel : public BEventReceiver
{
   public:
                                 BGrannyModel();
                                 ~BGrannyModel();

      bool                       init(const char* pBasePath, const char* pFileName, bool reloading = false);
            
      void                       deinit(void);
                              
      bool                       isLoaded(void) const { return mpGrannyFileInfo != NULL; }
      bool                       loadFailed(void) const { return mLoadFailed; }
            
      const BSimString&          getFilename(void) const { return mFilename; }
      void                       setFilename(const BCHAR_T* pFileName);

      granny_track_mask*         getTrackMask(long trackMask)  { BDEBUG_ASSERT(trackMask < cNumTrackMasks); return mpTrackMasks[trackMask]; }

      granny_file_info*          getGrannyFileInfo(void) { return mpGrannyFileInfo; }
      granny_mesh_binding**      getMeshBindings(void);

      long                       getBoneHandle(const char* pBoneName);
      void                       getBoneHandles(BSmallDynamicSimArray<long> *pBoneHandles);
                  
      uint                       getNumBones(void) const { return mNumBones; }
      uint                       getNumMeshes(void) const;

      bool                       getMeshName(long index, BSimString &name) const;
      long                       getMeshIndex(BSimString &name) const;
      bool                       getMeshBounds(long meshIndex, BVector &min, BVector &max);
      const char*                getMeshBone(long meshIndex);

      void                       setManagerIndex(int index) { mManagerIndex = index; }
      int                        getManagerIndex(void) const { return mManagerIndex; }
      
      eUGXGeomRenderFlags        getRenderFlags(void) const { return mUGXGeomRenderFlags; }
      void                       setRenderFlags(eUGXGeomRenderFlags newFlags) { mUGXGeomRenderFlags = newFlags; }
      
      BEventReceiverHandle       getEventHandle(void) const { return mEventHandle; }
      BEventReceiverHandle       getUGXGeomHandle(void) const { return mUGXGeomHandle; }
      eUGXGeomStatus             getUGXGeomStatus(void) const { return mUGXGeomStatus; }
      const BUGXGeomRenderInfo&  getUGXGeomRenderInfo(void) const { return mUGXGeomRenderInfo; }
      
      // Undamage render mask
      const BBitArray&           getUndamageRenderMask() const { return mUndamageRenderMask; }
      const BBitArray&           getDamageNoHitsRenderMask() const { return mDamageNoHitsRenderMask; }
      
      uint                       getAllocationSize() const { return mAllocationSize; }
                                    
   protected:
      virtual bool               receiveEvent(const BEvent& event, BThreadIndex threadIndex);
      
      BEventReceiverHandle       mUGXGeomHandle;
      eUGXGeomStatus             mUGXGeomStatus;
      BUGXGeomRenderInfo         mUGXGeomRenderInfo;
      
      granny_mesh_binding**      mpMeshBindings;
      granny_track_mask*         mpTrackMasks[cNumTrackMasks];
      
      BSimString                 mBasePath;
		BSimString                 mFilename;
		int                        mManagerIndex;
		
		granny_file_info*          mpGrannyFileInfo;
		
		uint                       mAllocationSize;
		      				
		eUGXGeomRenderFlags        mUGXGeomRenderFlags;
		uchar                      mNumBones;

      BBitArray                  mUndamageRenderMask;
      BBitArray                  mDamageNoHitsRenderMask;
		
		bool                       mLoadFailed : 1;
		
      bool                       load(BECFFileData* pECFFileData, const BSimString& filename, bool reloading);
      bool                       load(bool reloading);
				
		void                       freeGrannyData(granny_file_info* pGrannyFileInfo, granny_mesh_binding** pMeshBindings, granny_track_mask* pTrackMasks[]);

      void                       computeRenderMasks();
};
