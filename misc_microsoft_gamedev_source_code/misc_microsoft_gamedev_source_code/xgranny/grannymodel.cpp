//============================================================================
// grannymodel.cpp
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "grannymodel.h"

// xcore
#include "stream\byteStream.h"
#include "resource\ecfFileData.h"
#include "consoleOutput.h"
#include "timer.h"
#include "reloadManager.h"

// xrender
#include "renderEventClasses.h"

// xgeom
#include "ugxGeom.h"

// xsystem
#include "workdirsetup.h"
#include "bfileStream.h"

// xgranny
#include "grannymanager.h"

//============================================================================
// BGrannyModel::BGrannyModel
//============================================================================
BGrannyModel::BGrannyModel() :
   BEventReceiver(),
   mpGrannyFileInfo(NULL),
   mUGXGeomHandle(cInvalidEventReceiverHandle),
   mUGXGeomStatus(cUGXGeomStatusInvalid),
   mpMeshBindings(NULL),
   mUGXGeomRenderFlags(cRFDefaultModelFlags),
   mNumBones(0),
   mManagerIndex(-1),
   mLoadFailed(false),
   mAllocationSize(0)
{
   Utils::ClearObj(mpTrackMasks);
         
   eventReceiverInit();
}

//============================================================================
// BGrannyModel::~BGrannyModel
//============================================================================
BGrannyModel::~BGrannyModel()
{
   deinit();
   
   eventReceiverDeinit();
}

//============================================================================
// BGrannyModel::freeGrannyData
//============================================================================
void BGrannyModel::freeGrannyData(granny_file_info* pGrannyFileInfo, granny_mesh_binding** pMeshBindings, granny_track_mask* pTrackMasks[])
{
   if (pTrackMasks)
   {
      for(long i = 0; i < cNumTrackMasks; i++)
         GrannyFreeTrackMask(pTrackMasks[i]);
   }

   if (pMeshBindings)
   {
      if (pGrannyFileInfo)
      {
//-- FIXING PREFIX BUG ID 7690
         const granny_model* pModel = pGrannyFileInfo->Models[0];
//--
         if (pModel)
         {
            for(long i = 0; i < pModel->MeshBindingCount; i++)
               GrannyFreeMeshBinding(pMeshBindings[i]);
         }
      }         
                  
      delete[] pMeshBindings;
   }
   
   if (pGrannyFileInfo)
   {
      bool success = gRenderHeap.Delete(pGrannyFileInfo);
      success;
      BDEBUG_ASSERT(success);
   }
}

//============================================================================
// BGrannyModel::deinit
//============================================================================
void BGrannyModel::deinit(void)
{
   if (cInvalidEventReceiverHandle != mUGXGeomHandle)
   {
      gUGXGeomManager.remove(mUGXGeomHandle);  
      mUGXGeomHandle = cInvalidEventReceiverHandle;
   }     
         
   mUGXGeomStatus = cUGXGeomStatusInvalid;
   mUGXGeomRenderFlags = cRFDefaultModelFlags;
   mUGXGeomRenderInfo.clear();
               
   freeGrannyData(mpGrannyFileInfo, mpMeshBindings, mpTrackMasks);
   mpGrannyFileInfo = NULL;   
   
   Utils::ClearObj(mpTrackMasks);
   mpMeshBindings = NULL;

   mNumBones = 0;
   
   mAllocationSize = 0;
   
   mLoadFailed = false;
}

//============================================================================
// BGrannyModel::load
//============================================================================
bool BGrannyModel::load(BECFFileData* pECFFileData, const BSimString& filename, bool reloading)
{
   mLoadFailed = true;
   
   if (pECFFileData->getHeaderID() != BUGXGeom::cECFFileID)
   {
      gConsoleOutput.output(cMsgError, "BGrannyModel::load: Invalid UGX file: %s", filename.getPtr());
      return false;
   }
   
   int grxChunkIndex = pECFFileData->findChunkByID((uint64)BUGXGeom::cECFGRXChunkID);
   if (cInvalidIndex == grxChunkIndex)
   {
      gConsoleOutput.output(cMsgError, "BGrannyModel::load: Could not find GRX chunk: %s", filename.getPtr());
      return false;
   }
   
   granny_file_info* pGrannyFileInfo = static_cast<granny_file_info*>(pECFFileData->getChunkDataPtr(grxChunkIndex));
   BDEBUG_ASSERT(pGrannyFileInfo);
   
   bool success = GrannyRebasePointers(GrannyFileInfoType, pGrannyFileInfo, (int)pGrannyFileInfo, true);
   if (!success)
   {
      gConsoleOutput.output(cMsgError, "BGrannyModel::load: GrannyRebasePointers() failed: %s", filename.getPtr());
      return false;
   }

   // Make sure we have a model.
   if ( (pGrannyFileInfo->ModelCount != 1) || 
        (!pGrannyFileInfo->Models[0]) ||
        (!pGrannyFileInfo->FromFileName) || 
#if defined(BUILD_DEBUG) && !defined(BUILD_CHECKED)
        IsBadReadPtr(pGrannyFileInfo->FromFileName, 7) || 
#endif        
        (_stricmp(pGrannyFileInfo->FromFileName, "gr2ugx") != 0) )
   {
      gConsoleOutput.output(cMsgError, "BGrannyModel::load: GRX data is invalid: %s", filename.getPtr());
      return false;
   }
      
   // Create mesh binding to bone items
   granny_model* pModel = pGrannyFileInfo->Models[0];
//-- FIXING PREFIX BUG ID 7693
   const granny_skeleton* pSkeleton = pModel->Skeleton;
//--

   granny_mesh_binding** pMeshBindings = NULL;
   if (pModel->MeshBindingCount > 0)
   {
      pMeshBindings = new granny_mesh_binding*[pModel->MeshBindingCount];

      for(long i=0; i < pModel->MeshBindingCount; i++)
      {
//-- FIXING PREFIX BUG ID 7691
         const granny_mesh* pMesh = pModel->MeshBindings[i].Mesh;
//--
         pMeshBindings[i] = GrannyNewMeshBinding(pMesh, pSkeleton, pSkeleton);
      }
   }

   mNumBones = pSkeleton ? static_cast<uchar>(pSkeleton->BoneCount) : 0;

   granny_track_mask* pTrackMasks[cNumTrackMasks];
   // Create upper body track mask
   pTrackMasks[0] = GrannyNewTrackMask(0.0f, mNumBones);
   GrannyExtractTrackMask(pTrackMasks[0], mNumBones, pSkeleton, "TrackMaskUpperBody", 0.0f, true);
   // Make lower body track mask the inverse of the upper body one
   pTrackMasks[1] = GrannyCopyTrackMask(pTrackMasks[0]);
   GrannyInvertTrackMask(pTrackMasks[1]);
     
   if (reloading)
   {
      granny_file_info* pOldGrannyFileInfo = mpGrannyFileInfo;
      mpGrannyFileInfo = pGrannyFileInfo;

      granny_mesh_binding** pOldMeshBindings = mpMeshBindings;
      mpMeshBindings = pMeshBindings;

      granny_track_mask* pOldTrackMasks[cNumTrackMasks];
      for (long i = 0; i < cNumTrackMasks; i++)
      {
         pOldTrackMasks[i] = mpTrackMasks[i];
         mpTrackMasks[i] = pTrackMasks[i];
      }

      gGrannyManager.reInitInstances(mManagerIndex);

      freeGrannyData(pOldGrannyFileInfo, pOldMeshBindings, pOldTrackMasks);

      gGrannyManager.validateInstances();
   }
   else
   {
      freeGrannyData(mpGrannyFileInfo, mpMeshBindings, mpTrackMasks);

      for (long i = 0; i < cNumTrackMasks; i++)
         mpTrackMasks[i] = pTrackMasks[i];
         
      mpMeshBindings = pMeshBindings;
      mpGrannyFileInfo = pGrannyFileInfo;
   }
   
   mUGXGeomRenderFlags = cRFDefaultModelFlags;
      
   BDEBUG_ASSERT(&gRenderHeap == pECFFileData->getChunkDataHeap(grxChunkIndex));
   pECFFileData->acquireChunkDataOwnership(grxChunkIndex);
   
   if (mUGXGeomHandle != cInvalidEventReceiverHandle)
   {
      gUGXGeomManager.remove(mUGXGeomHandle);
      mUGXGeomHandle = cInvalidEventReceiverHandle;
   }

   mUGXGeomHandle = gUGXGeomManager.addGeom(pECFFileData, mEventHandle, mUGXGeomRenderFlags);
   
   gConsoleOutput.output(cMsgResource, "Loaded UGX: %s", filename.getPtr());
   
   mLoadFailed = false;
   
   return true;
}

//============================================================================
// BGrannyModel::load
//============================================================================
bool BGrannyModel::load(bool reloading)
{
   BSimString fullPath;
   fullPath.format("%s%s", mBasePath.getPtr(), mFilename.getPtr());
   if (strPathHasExtension(fullPath, "gr2"))
      strPathRemoveExtension(fullPath);
      
   strPathAddExtension(fullPath, "ugx");
   
   if (!reloading)
   {
      gReloadManager.deregisterClient(mEventHandle);

      BReloadManager::BPathArray paths;
      paths.pushBack(fullPath);
      gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, cEventClassReloadNotify, 0);
   }
   else
   {
      gConsoleOutput.status("Reloading model: %s", fullPath.getPtr());
   }
   
   BFile file;
   if (!file.openReadOnly(cDirProduction, fullPath, BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.output(cMsgError, "BGrannyModel::load: Could not open file: %s", fullPath.getPtr());
      return false;
   }
   
   BFileSystemStream stream;
   stream.open(&file, fullPath, cSFReadable|cSFNoAutoClose);
            
   BECFFileData* pECFFileData = HEAP_NEW(BECFFileData, gRenderHeap);
   if (!pECFFileData->load(&stream, false, true, true))
   {
      HEAP_DELETE(pECFFileData, gRenderHeap);
      
      gConsoleOutput.output(cMsgError, "BGrannyModel::load: Could not load file: %s", fullPath.getPtr());
      return false;
   }
   
   stream.close();
   
   file.close();
      
   // This will be approximate, but good enough.      
   mAllocationSize = 
      Math::Max(0, pECFFileData->getChunkDataLenByID(BUGXGeom::cECFCachedDataChunkID)) + 
      Math::Max(0, pECFFileData->getChunkDataLenByID(BUGXGeom::cECFIBChunkID)) + 
      Math::Max(0, pECFFileData->getChunkDataLenByID(BUGXGeom::cECFVBChunkID)) + 
      Math::Max(0, pECFFileData->getChunkDataLenByID(BUGXGeom::cECFGRXChunkID)) + 
      Math::Max(0, pECFFileData->getChunkDataLenByID(BUGXGeom::cECFTreeChunkID));
         
   if (!load(pECFFileData, fullPath, reloading))
   {
      mAllocationSize = 0;
      
      HEAP_DELETE(pECFFileData, gRenderHeap);
      return false;
   }

   computeRenderMasks();
   
   return true;
}

//============================================================================
// BGrannyModel::init
//============================================================================
bool BGrannyModel::init(const char* pBasePath, const BCHAR_T* pFileName, bool reloading)
{
   // Cleanse out any existing info.
   deinit();
   
   mLoadFailed = true;
                  
   // Check param.
   if (!pFileName)
      return false;
                  
   // Remember filename.
   mFilename = pFileName;
   mFilename.toLower();
   mBasePath = pBasePath;
   mBasePath.toLower();

   return load(false);
}

//============================================================================
// BGrannyModel::setFilename
//============================================================================
void BGrannyModel::setFilename(const BCHAR_T* pFileName)
{
   mFilename = pFileName;
   mFilename.toLower();
}

//============================================================================
// BGrannyModel::getMeshBindings
//============================================================================
granny_mesh_binding** BGrannyModel::getMeshBindings(void) 
{ 
   return mpMeshBindings; 
}

//============================================================================
//============================================================================
void BGrannyModel::getBoneHandles(BSmallDynamicSimArray<long> *pBoneHandles)
{
   BASSERT(pBoneHandles);
   pBoneHandles->empty();

   if (!mpGrannyFileInfo)
      return;

   if (mpMeshBindings)
   {
      granny_model* pModel=mpGrannyFileInfo->Models[0];

      long meshCount=pModel->MeshBindingCount;
      for(long i=0; i < meshCount; i++)
      {
//-- FIXING PREFIX BUG ID 7694
         const granny_mesh* pMesh=pModel->MeshBindings[i].Mesh;
//--
//-- FIXING PREFIX BUG ID 7695
         const granny_mesh_binding* pMeshBinding=mpMeshBindings[i];
//--
         if(!pMeshBinding)
            continue;

         long bindingCount=pMesh->BoneBindingCount;
         for(long j=0; j<bindingCount; j++)
         {
            //granny_bone_binding& boneBinding=pMesh->BoneBindings[j];
            long boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];
            long boneHandle = GRANNYBONEHANDLE(boneIndex, i, j);
            pBoneHandles->add(boneHandle);
         }
      }
   }

   long skelCount = mpGrannyFileInfo->SkeletonCount;
   for (long s=0; s < skelCount; s++)
   {
//-- FIXING PREFIX BUG ID 7696
      const granny_skeleton* pSkeleton=mpGrannyFileInfo->Skeletons[s];
//--
      long boneCount=pSkeleton->BoneCount;
      for(long b=0; b<boneCount; b++)
      {
         long boneHandle = GRANNYBONEHANDLE(b, -1, -1);
         pBoneHandles->add(boneHandle);
      }
   }
}

//============================================================================
// BGrannyModel::getBoneHandle
//============================================================================
long BGrannyModel::getBoneHandle(const char* pBoneName)
{
   if (!mpGrannyFileInfo)
      return -1;
      
   if (mpMeshBindings)
   {
      granny_model* pModel=mpGrannyFileInfo->Models[0];

      long meshCount=pModel->MeshBindingCount;
      for(long i=0; i < meshCount; i++)
      {
         granny_mesh* pMesh=pModel->MeshBindings[i].Mesh;
//-- FIXING PREFIX BUG ID 7698
         const granny_mesh_binding* pMeshBinding=mpMeshBindings[i];
//--
         if(!pMeshBinding)
            continue;

         long bindingCount=pMesh->BoneBindingCount;
         for(long j=0; j<bindingCount; j++)
         {
//-- FIXING PREFIX BUG ID 7697
            const granny_bone_binding& boneBinding=pMesh->BoneBindings[j];
//--
            if(stricmp(boneBinding.BoneName, pBoneName)==0)
            {
               long boneIndex = GrannyGetMeshBindingToBoneIndices(pMeshBinding)[j];
               long boneHandle = GRANNYBONEHANDLE(boneIndex, i, j);
               return boneHandle;
            }
         }
      }
   }

   long skelCount = mpGrannyFileInfo->SkeletonCount;
   for (long s=0; s < skelCount; s++)
   {
//-- FIXING PREFIX BUG ID 7699
      const granny_skeleton* pSkeleton=mpGrannyFileInfo->Skeletons[s];
//--
      long boneCount=pSkeleton->BoneCount;
      for(long b=0; b<boneCount; b++)
      {
         if(stricmp(pSkeleton->Bones[b].Name, pBoneName)==0)
         {
            long boneHandle = GRANNYBONEHANDLE(b, -1, -1);
            return boneHandle;
         }
      }
   }

   return -1;
}

//============================================================================
// BGrannyModel::getNumMeshes
//============================================================================
uint BGrannyModel::getNumMeshes(void) const
{
   // Have file info?
   if(!mpGrannyFileInfo)
      return(0);

   // Have a model?
   if(mpGrannyFileInfo->ModelCount < 1)
      return(0);

   // Get the number of meshes -- we only use the first model.
   granny_model *model = mpGrannyFileInfo->Models[0];

   // Model null for some reason?
   if(!model)
      return(0);

   // Make sure mesh binding array valid
   if (model->MeshBindings == NULL)
      return 0;

   // Here's the count.
   return(model->MeshBindingCount);
}


//============================================================================
// BGrannyModel::getMeshName
//============================================================================
bool  BGrannyModel::getMeshName(long index, BSimString &name) const
{
   // Have file info?
   if(!mpGrannyFileInfo)
      return(false);

   // Have a model?
   if(mpGrannyFileInfo->ModelCount < 1)
      return(false);

   // Get the number of meshes -- we only use the first model.
   granny_model *model = mpGrannyFileInfo->Models[0];

   // Model null for some reason?
   if(!model)
      return(false);

   // Here's the count.
   if (index < 0 || index >= model->MeshBindingCount)
      return (false);

   name.set(model->MeshBindings[index].Mesh->Name);
   return (true);
}

//============================================================================
// BGrannyModel::getMeshIndex
//============================================================================
long BGrannyModel::getMeshIndex(BSimString &name) const
{
   // Have file info?
   if(!mpGrannyFileInfo)
      return -1;

   // Have a model?
   if(mpGrannyFileInfo->ModelCount < 1)
      return -1;

   // Get the number of meshes -- we only use the first model.
   granny_model *model = mpGrannyFileInfo->Models[0];

   // Model null for some reason?
   if(!model)
      return -1;

   for (int i = 0; i < model->MeshBindingCount; i++)
   {
      if (name.compare(model->MeshBindings[i].Mesh->Name) == 0)
         return i;
   }

   return -1;
}

//============================================================================
// BGrannyModel::getMeshBounds
//============================================================================
bool BGrannyModel::getMeshBounds(long meshIndex, BVector &min, BVector &max)
{
   // Have file info?
   if(!mpGrannyFileInfo)
      return(false);

   // Have a model?
   if(mpGrannyFileInfo->ModelCount < 1)
      return(false);

   // Get the number of meshes -- we only use the first model.
   granny_model *model = mpGrannyFileInfo->Models[0];

   // Model null for some reason?
   if(!model)
      return(false);

   long meshcount = model->MeshBindingCount;
   if (meshIndex <0 || meshIndex >= meshcount)
      return (false);

   if (model->MeshBindings[meshIndex].Mesh->BoneBindingCount < 1)
      return (false);

   //-- WMJ [8/23/2004] We assume that bone 0 is the root bone that influences the entire mesh
   max.set( model->MeshBindings[meshIndex].Mesh->BoneBindings[0].OBBMax[0],
            model->MeshBindings[meshIndex].Mesh->BoneBindings[0].OBBMax[1],
            model->MeshBindings[meshIndex].Mesh->BoneBindings[0].OBBMax[2]);

   min.set( model->MeshBindings[meshIndex].Mesh->BoneBindings[0].OBBMin[0],
            model->MeshBindings[meshIndex].Mesh->BoneBindings[0].OBBMin[1],
            model->MeshBindings[meshIndex].Mesh->BoneBindings[0].OBBMin[2]);

   return(true);
}


//============================================================================
// BGrannyModel::getMeshBone
//============================================================================
const char* BGrannyModel::getMeshBone(long meshIndex)
{
   // Have file info?
   if(!mpGrannyFileInfo)
      return(NULL);

   // Have a model?
   if(mpGrannyFileInfo->ModelCount < 1)
      return(NULL);

   // Get the number of meshes -- we only use the first model.
   granny_model *model = mpGrannyFileInfo->Models[0];

   // Model null for some reason?
   if(!model)
      return(NULL);

   long meshcount = model->MeshBindingCount;
   if (meshIndex <0 || meshIndex >= meshcount)
      return (NULL);

   if (model->MeshBindings[meshIndex].Mesh->BoneBindingCount < 1)
      return (NULL);

   return(model->MeshBindings[meshIndex].Mesh->BoneBindings[0].BoneName);
}


//============================================================================
// BGrannyModel::computeRenderMasks
//============================================================================
void BGrannyModel::computeRenderMasks()
{
   if (mpMeshBindings)
   {
      granny_model* pModel=mpGrannyFileInfo->Models[0];

      // Compute the rendermasks
      mDamageNoHitsRenderMask.setNumber(pModel->MeshBindingCount, true);
      mDamageNoHitsRenderMask.setAll();

      mUndamageRenderMask.setNumber(pModel->MeshBindingCount, true);
      mUndamageRenderMask.setAll();

      bool bHasUndamageParts = false;

      long meshCount=pModel->MeshBindingCount;
      for(long i=0; i < meshCount; i++)
      {
//-- FIXING PREFIX BUG ID 7704
         const granny_mesh* pMesh=pModel->MeshBindings[i].Mesh;
//--
         
         bool damageFlag = strstr(pMesh->Name, "damage") ? true : false;
         bool intactFlag = strstr(pMesh->Name, "intact") ? true : false;

         if (damageFlag || intactFlag)
         {
            mDamageNoHitsRenderMask.clearBit(i);
         }

         if (!intactFlag)
         {
            mUndamageRenderMask.clearBit(i);
         }
         else
         {
            bHasUndamageParts = true;
         }

         // Turn off optional meshes by default
         bool optionalMesh = strstr(pMesh->Name, "optional") ? true : false;
         if (optionalMesh)
         {
            mDamageNoHitsRenderMask.clearBit(i);
            mUndamageRenderMask.clearBit(i);
         }

         // Spartan driver mesh is off by default
         bool spartanDriver = (strCompare(pMesh->Name, strLength(pMesh->Name), "spartanDriver", 13) == 0);
         if (spartanDriver)
         {
            mDamageNoHitsRenderMask.clearBit(i);
            mUndamageRenderMask.clearBit(i);
         }
      }

      if(!bHasUndamageParts)
      {
         mUndamageRenderMask = mDamageNoHitsRenderMask;
      }
   }
   else
   {
      mUndamageRenderMask.setNumber(0, true);
      mDamageNoHitsRenderMask.setNumber(0, true);
   }
}

//============================================================================
// BGrannyModel::receiveEvent
//============================================================================
bool BGrannyModel::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cRenderEventClassUGXGeomRenderInfo:
      {
         BDEBUG_ASSERT(event.mpPayload);
         if (event.mFromHandle == mUGXGeomHandle)
         {
            mUGXGeomRenderInfo = *(BUGXGeomRenderInfo*)event.mpPayload;
         }
         break;
      }
      case cRenderEventClassUGXGeomStatusChanged:
      {
         if (event.mFromHandle == mUGXGeomHandle)
            mUGXGeomStatus = static_cast<eUGXGeomStatus>(event.mPrivateData);
         break;
      }
      case cEventClassReloadNotify:
      {
         load(true);
         
         break;
      }
   }
   
   return false;
}
