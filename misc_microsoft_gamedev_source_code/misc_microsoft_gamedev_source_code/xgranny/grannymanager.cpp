//============================================================================
// grannymanager.cpp
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "grannymanager.h"
#include "bitArray.h"
#include "consoleOutput.h"

// xsystem
#include "workdirsetup.h"

// xgranny
#include "grannyanimation.h"
#include "grannyinstance.h"
#include "grannyinstancerenderer.h"
#include "grannymodel.h"

//xgame
#include "..\xgame\syncmacros.h"

//============================================================================
// Globals
//============================================================================
BGrannyManager gGrannyManager;


//============================================================================
// Max bone counts for each cache bucket.
const long cRenderPrepareCacheBucketSizes[cNumRenderPrepareCacheBuckets] = {2, 4, 8, 16, 32, 64, BGrannyManager::cMaxBones /*=128*/};



//============================================================================
// Granny lib dependency
//============================================================================
#ifdef XBOX
   #ifdef BUILD_CHECKED
      #pragma comment(lib, "granny2.lib")
   #elif BUILD_DEBUG
      //#pragma comment(lib, "granny2D.lib")
      #pragma comment(lib, "granny2.lib")
   #elif defined(BUILD_PROFILE) && defined(CALLCAP)
      #pragma comment(lib, "granny2P.lib")
   #elif defined(LTCG) && defined(BUILD_FINAL)
      #pragma comment(lib, "granny2LTCG.lib")
   #else
      #pragma comment(lib, "granny2.lib")
   #endif
#endif   
   
//============================================================================
// grannyAllocate - Callback for granny allocations
//============================================================================
void* grannyAllocate(char const* file, granny_int32x line, granny_int32x alignment, granny_int32x size)
{
   // Adjust the size of the allocation to make sure we have enough
   // room to align the result.  This is based on the fact that rockall allocations
   // are aligned at >= allocation size.
   long actualSize = size;
   
   if (alignment > sizeof(DWORD))
   {
      // Allocate enough extra to allow Granny to align up the pointer. (Does it do this itself?)
      actualSize = size + alignment - 1;
   }

   // Allocate the memory
   return gRenderHeap.New(actualSize);
}

//============================================================================
// grannyDeallocate - Callback for granny deallocations
//============================================================================
void grannyDeallocate(char const * /*file*/, granny_int32x /*line*/, void *buff)
{
   // Deallocate.
   if(buff)
      gRenderHeap.Delete((void*)buff);
}

//============================================================================
// grannyCloseReader - Callback for granny I/O closing file readers
//============================================================================
void __cdecl grannyCloseReader(char const * /*sourceFileName*/, granny_int32x /*sourceLineNumber*/, granny_file_reader *readerInit)
{
   BGrannyReader *reader = (BGrannyReader *)readerInit;
   if(reader)
   {
//      GrannyUnlinkFileReader(&reader->mGrannyReader);
      delete reader;
   }
}

//============================================================================
// grannyReadAtMost - Callback for granny I/O reading data.
//============================================================================
granny_int32x __cdecl grannyReadAtMost(char const * /*sourceFileName*/, granny_int32x /*sourceLineNumber*/, granny_file_reader *readerInit, 
   granny_int32x filePosition, granny_int32x len, void *buffer)
{
   // Cast to our class.
   BGrannyReader *reader = (BGrannyReader *)readerInit;

   // Seek to right spot.
   reader->mFile.setOffset(filePosition);

   // Read the data.
   return(reader->mFile.readEx(buffer, len));
}

//============================================================================
// grannyOpenReader - Callback for granny I/O opening a file..
//============================================================================
granny_file_reader * __cdecl grannyOpenReader(char const *sourceFileName, granny_int32x sourceLineNumber, char const *fileNameToOpen)
{
   // Allocate a reader object.
   BGrannyReader *reader = new BGrannyReader;
   if(!reader)
      return(NULL);

   // Try to open the requested file.
   bool ok = reader->mFile.openReadOnly(cDirProduction, fileNameToOpen);

   // If the open failed, clean up and bail.
   if(!ok)
   {
      delete reader;
      return(NULL);
   }

   // Create the granny reader object and hook it to our BGrannyReader.
   GrannyInitializeFileReader(sourceFileName, sourceLineNumber, grannyCloseReader, grannyReadAtMost, &reader->mGrannyReader);

   // Hand back the result.
   return((granny_file_reader *)reader);
}

//============================================================================
// BGrannyManager::BGrannyManager
//============================================================================
BGrannyManager::BGrannyManager() :
   mInstancesLocked(false),
   mBoundingBoxesDirty(false),
   mStatFrameGetBoneCallCount(0),
   mStatFrameGetBoneCachedCount(0),
   mStatTotalGetBoneCallCount(0),
   mStatTotalGetBoneCachedCount(0),
   mSampleAnimCacheRefCount(0),
   mSampleAnimCacheIndex(0),
   mEnableSampleAnimCache(false)
{
   mBaseDirectory.empty();

   ZeroMemory(&mLocalPose, sizeof(uint) * cThreadIndexMax);
   ZeroMemory(&mBlendLocalPose, sizeof(uint) * cThreadIndexMax);
   ZeroMemory(&mWorldPose, sizeof(uint) * cThreadIndexMax);

}

//============================================================================
// BGrannyManager::~BGrannyManager
//============================================================================
BGrannyManager::~BGrannyManager()
{
   // Clear pose cache.
   clearRenderPreparePoseCache();
}


//============================================================================
//============================================================================
void BGrannyManager::clearRenderPreparePoseCache()
{
   for(long i=0; i<cNumRenderPrepareCacheBuckets; i++)
   {
      // If you're clearing the cache, the assumption is that there should be no outstanding allocations
      // that have not yet been freed.
      BASSERT(mRenderPreparePoseCache[i].mUsedCount == 0);
      
      // Free each cached pose.
      long count = mRenderPreparePoseCache[i].mPoses.getNumber();
      for(long j=0; j<count; j++)
         GrannyFreeLocalPose(mRenderPreparePoseCache[i].mPoses[j]);
      
      // Cache is empty.
      mRenderPreparePoseCache[i].mPoses.setNumber(0);
      mRenderPreparePoseCache[i].mUsedCount = 0;
   }
}



void GrannyLogCallback(
                       granny_log_message_type Type,
                       granny_log_message_origin Origin,
                       char const *Error,
                       void *UserData)
{
   Type;
   Origin;
   Error;
   UserData;
   if (Error)
   {
      gConsoleOutput.warning("(Granny) %s\n", Error);
   }
}

//============================================================================
// BGrannyManager::init
//============================================================================
bool BGrannyManager::init(const BCHAR_T* pDirName)
{
   // Check for matching versions.
   if(!GrannyVersionsMatch)
   {
      BFAIL("Granny SDK version does not match DLL version.");
      return(false);
   }

   mBaseDirectory=pDirName;

   GrannySetAllocator(grannyAllocate, grannyDeallocate);

   // Set our own I/O routine so we can read out of archives, etc.
   GrannySetDefaultFileReaderOpenCallback(grannyOpenReader);

#ifndef BUILD_FINAL   
   granny_log_callback Callback;
   Callback.Function = GrannyLogCallback;
   Callback.UserData = 0;
   GrannySetLogCallback(&Callback);
#endif   

   // Generate some scratch space local and world poses.
   for(uint threadIndex = 0; threadIndex < cThreadIndexMax; threadIndex++)
   {
      mLocalPose[threadIndex] = GrannyNewLocalPose(cMaxBones);
      if(!mLocalPose[threadIndex])
      {
         BFAIL("Could not allocate granny local pose.");
         return(false);
      }

      mBlendLocalPose[threadIndex] = GrannyNewLocalPose(cMaxBones);
      if(!mBlendLocalPose[threadIndex])
      {
         BFAIL("Could not allocate granny blend local pose.");
         return(false);
      }

      mWorldPose[threadIndex] = GrannyNewWorldPose(cMaxBones);
      if(!mWorldPose[threadIndex])
      {
         BFAIL("Could not allocate granny world pose.");
         return(false);
      }

      for(uint attachmentLevel = 0; attachmentLevel < cAttachmentDepthMax; attachmentLevel++)
      {
         mLocalSparsePoseCache[threadIndex][attachmentLevel].mLocalPose = GrannyNewLocalPose(cMaxBones);
         mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArray = new granny_int32x[BGrannyManager::cMaxBones];
         mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArrayReverse = new granny_int32x[BGrannyManager::cMaxBones];

         if(!mLocalSparsePoseCache[threadIndex][attachmentLevel].mLocalPose ||
            !mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArray ||
            !mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArrayReverse)
         {
            BFAIL("Could not allocate sparse pose data.");
            return(false);
         }
      }
   }



   // Instance renderer
   gGrannyInstanceRenderer.init();

   // Success.
   return(true);
}

//============================================================================
// BGrannyManager::deinit
//============================================================================
void BGrannyManager::deinit()
{
   gGrannyInstanceRenderer.deinit();

   // Clean up models.
   for(long i=0; i<mModels.getNumber(); i++)
      delete mModels[i];
   
   // Clean up anims.
   for(long i=0; i<mAnimations.getNumber(); i++)
      delete mAnimations[i];
   
   // Clean up anims.
   for(long i=0; i<mInstances.getNumber(); i++)
      delete mInstances[i];

   for(uint threadIndex = 0; threadIndex < cThreadIndexMax; threadIndex++)
   {
      if(mLocalPose[threadIndex])
      {
         GrannyFreeLocalPose(mLocalPose[threadIndex]);
         mLocalPose[threadIndex]=NULL;
      }

      if(mBlendLocalPose[threadIndex])
      {
         GrannyFreeLocalPose(mBlendLocalPose[threadIndex]);
         mBlendLocalPose[threadIndex]=NULL;
      }

      if(mWorldPose)
      {
         GrannyFreeWorldPose(mWorldPose[threadIndex]);
         mWorldPose[threadIndex]=NULL;
      }

      for(uint attachmentLevel = 0; attachmentLevel < cAttachmentDepthMax; attachmentLevel++)
      {
         if(mLocalSparsePoseCache[threadIndex][attachmentLevel].mLocalPose)
         {
            GrannyFreeLocalPose(mLocalSparsePoseCache[threadIndex][attachmentLevel].mLocalPose);
            mLocalSparsePoseCache[threadIndex][attachmentLevel].mLocalPose = NULL;
         }

         if(mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArray)
         {
            delete(mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArray);
            mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArray = NULL;
         }

         if(mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArrayReverse)
         {
            delete(mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArrayReverse);
            mLocalSparsePoseCache[threadIndex][attachmentLevel].mSparseBoneArrayReverse = NULL;
         }
      }
   }

   mBaseDirectory.empty();

   GrannyFlushAllUnusedAnimationBindings();
   
   mInstancesLocked = false;

   // Clear pose cache.
   clearRenderPreparePoseCache();
}

/*
void ReplaceControl(granny_control* Control, granny_model_instance* NewInstance)
{
    if (Control)
    {
        // Get the model_control_binding from this control
        granny_model_control_binding* Binding = GrannyControlModelsBegin(Control);

        // We need to be sure that there is only one model for this control (i.e, this
        // isn't a multi-instance control), otherwise, the following code will fail.
        if (GrannyControlModelsEnd(Control) != GrannyControlModelsNext(Binding))
        {
            // Handle the error somehow
            assert(false);
        }

        granny_animation_binding* AnimBinding = GrannyGetAnimationBindingFromControlBinding(Binding);
        if (AnimBinding == NULL)
        {
            // This is a controlled_local_pose or an spu controlled animation.  No good.
            assert(false);
        }

        granny_model_instance* OldModelInstance =
            GrannyGetModelInstanceFromBinding(Binding);

        granny_int32x TrackGroupIndex;
        if (!GrannyFindTrackGroupForModel(AnimBinding->ID.Animation,
                                          GrannyGetSourceModel(NewInstance)->Name,
                                          &TrackGroupIndex))
        {
            // No track group for the new model, handle the error
            assert(false);
        }

        granny_controlled_animation_builder* Builder =
            GrannyBeginControlledAnimation(0.0f, AnimBinding->ID.Animation);
        GrannySetTrackGroupTarget(Builder, TrackGroupIndex, NewInstance);

        // Set the track and model masks.  Note that the model mask may not work on a new
        // model, since the bone orders may have change
        {
            granny_track_mask const* ModelMask =
                GrannyGetControlTrackGroupModelMask(Control, OldModelInstance);
            if (ModelMask)
            {
                GrannySetTrackGroupModelMask(Builder, TrackGroupIndex,
                                             const_cast<granny_track_mask*>(ModelMask));
            }

            granny_track_mask const* TrackMask
                = GrannyGetControlTrackGroupTrackMask(Control, AnimBinding->ID.Animation, TrackGroupIndex);
            if (TrackMask)
            {
                GrannySetTrackGroupTrackMask(Builder, TrackGroupIndex,
                                             const_cast<granny_track_mask*>(TrackMask));
            }
        }

        // Technically, since the animation is aimed at the "old" model, we should use the
        // retargeter to bind it to the new one.  Commented out, since it's likely that
        // this isn't necessary in this particular case
        //
        //GrannySetTrackGroupBasisTransform(Builder, TrackGroupIndex,
        //                                  GrannyGetSourceModel(OldModelInstance),
        //                                  GrannyGetSourceModel(NewModelInstance));

        // Almost never anything but "*", but let's be thorough...
        GrannySetTrackMatchRule(Builder, TrackGroupIndex,
                                AnimBinding->ID.TrackPattern,
                                AnimBinding->ID.BonePattern);

        granny_control* NewControl = GrannyEndControlledAnimation(Builder);
        if (NewControl)
        {
            GrannySetControlActive(NewControl,    GrannyControlIsActive(Control));
            GrannySetControlClock(NewControl,     GrannyGetControlClock(Control));
            GrannySetControlLoopCount(NewControl, GrannyGetControlLoopCount(Control));
            GrannySetControlWeight(NewControl,    GrannyGetControlWeight(Control));
            GrannySetControlSpeed(NewControl,     GrannyGetControlSpeed(Control));

            bool BackLoop, ForwardLoop;
            GrannyGetControlLoopState(Control, &BackLoop, &ForwardLoop);
            assert(BackLoop == ForwardLoop); // won't work otherwise...
            GrannySetControlForceClampedLooping(NewControl, !BackLoop);

            if (GrannyGetControlCompletionCheckFlag(Control))
            {
                GrannyCompleteControlAt(NewControl, GrannyGetControlCompletionClock(Control));
            }

            // !!!A VERY BAD IDEA TO LEAVE THIS IN!!!
            GrannyFreeControl(Control);
       }
    }
}
*/

//============================================================================
//============================================================================
void BGrannyManager::gameInit()
{
   mStatFrameGetBoneCallCount=0;
   mStatFrameGetBoneCachedCount=0;
   mStatTotalGetBoneCallCount=0;
   mStatTotalGetBoneCachedCount=0;
}

//============================================================================
// BGrannyManager::reInitInstances
//============================================================================
void BGrannyManager::reInitInstances(long modelIndex)
{
   if(modelIndex < 0)
      return;

   for(long i=0; i<mInstances.getNumber(); i++)
   {
      BGrannyInstance* pInst = mInstances[i];
      if(pInst == NULL)
         continue;

      //-- reinit if there is a match.
      if(pInst->getModelIndex() == modelIndex)
      {
         pInst->freeGrannyControls();
         pInst->init(modelIndex, &pInst->getUVOffsets());
      }
   }

   mBoundingBoxesDirty = true;   
//   gVisualManager.recomputeProtoVisualBoundingBoxes();
}

//============================================================================
// BGrannyManager::validateInstances
//============================================================================
void BGrannyManager::validateInstances()
{
   long i;

   // Make sure the instances are pointing to the right models after a model reload.
   // There was a case where the granny_model pointer referenced by the BGrannyInstance
   // mModelInstance handle didn't match up with the granny_model pointer stored by BGrannyModel,
   // resulting in a crash.
   for (i = 0; i < mInstances.getNumber(); i++)
   {
      BGrannyInstance* pInst = mInstances[i];
      if (pInst == NULL)
         continue;

      granny_model *instancesGrannyModel = NULL;
      if (pInst->getModelInstance() != NULL)
         instancesGrannyModel = GrannyGetSourceModel(pInst->getModelInstance());

      granny_model *modelsGrannyModel = NULL;
      BGrannyModel *model = getModel(pInst->getModelIndex());
      if (model != NULL)
      {
         if ((model->getGrannyFileInfo() != NULL) && (model->getGrannyFileInfo()->ModelCount > 0) &&
             (model->getGrannyFileInfo()->Models != NULL))
         {
            modelsGrannyModel = model->getGrannyFileInfo()->Models[0];
         }
      }

      // Models differ so granny instance wasn't updated properly
      if (instancesGrannyModel != modelsGrannyModel)
      {
         BSimString msg;
         if (model != NULL)
            msg.format(B("Granny instance has different source model data (%s).  A model reload probably went bad."), model->getFilename().getPtr());
         else
            msg.format(B("Granny instance has different source model data.  A model reload probably went bad."));
         BASSERTM(0, msg.getPtr());

         // Can reinit the granny instance here if we want to
      }

      // Try to compute the world pose since that's where crashes usually occur
      pInst->computeWorldPose(NULL);
   }
}

//============================================================================
// BGrannyManager::rebindAnimation
//
// This function is called when an animation has been reloaded and since
// the animation may be currently playing, it needs to be fixed up.  It will
// loop through all model instances and check if any granny control is playing
// the given animation.  When it finds one, it release it and replays the
// anim.
//============================================================================
void BGrannyManager::rebindAnimation(int animIndex)
{
   for(long i=0; i<mInstances.getNumber(); i++)
   {
      BGrannyInstance* pInst = mInstances[i];
      if(pInst == NULL)
         continue;

      granny_model_instance *pGrannyModelInstance = pInst->getModelInstance();

      if(pGrannyModelInstance == NULL)
         return;

      // Iterate over controls
      for(granny_model_control_binding *Binding = GrannyModelControlsBegin(pGrannyModelInstance);
         Binding != GrannyModelControlsEnd(pGrannyModelInstance);
         Binding = GrannyModelControlsNext(Binding))
      {
         granny_control *pControl = GrannyGetControlFromBinding(Binding);

         void** ppUserData = GrannyGetControlUserDataArray(pControl);
         if(ppUserData[GrannyControlUserData_AnimIndex] == (void*)animIndex)
         {
            // Get info from control
            int loopCount = GrannyGetControlLoopCount(pControl);
            float playWeight = GrannyGetControlWeight(pControl);
            float timeIntoAnimation = GrannyGetControlRawLocalClock(pControl);

            // Free control
            // Don't need to free this here since pInst->playAnimation will do the trick.  SAT
            //GrannyFreeControl(pControl);

            // rg [3/1/07] - The params where messed up here
            pInst->playAnimation((long)ppUserData[GrannyControlUserData_AnimationTrack], animIndex, playWeight, 0.0f, loopCount, 0.0f, timeIntoAnimation);
         }
      }
   }
}

//============================================================================
// BGrannyManager::unloadAll
//============================================================================
bool BGrannyManager::unloadAll()
{
   // SLB: We don't want to rely on other systems to remove all instances before this function gets called. The assert should never be triggered.
   BASSERT(mInstances.isEmpty());

   if (!mInstances.isEmpty())
      return false;

   for (uint i = 0; i < mModels.getSize(); i++)
   {
      BGrannyModel* pGrannyModel = mModels[i];
      if (!pGrannyModel)
         continue;

      delete pGrannyModel;
   }
   
   for (uint i = 0; i < mAnimations.getSize(); i++)
   {
      BGrannyAnimation* pGrannyAnim = mAnimations[i];
      if (!pGrannyAnim)
         continue;
         
      delete pGrannyAnim;
   }

   // SLB: Clear the arrays
   mInstances.clear();
   mModels.clear();
   mModelNameTable.clearAll();
   mAnimations.clear();
   mAnimationNameTable.clearAll();
   
   return true;
}

//============================================================================
// BGrannyManager::getNumLoadedModels
//============================================================================
long BGrannyManager::getNumLoadedModels() const
{
   long totalLoaded = 0;
   for (uint i = 0; i < mModels.getSize(); i++)
   {
      if ((mModels[i]) && (mModels[i]->isLoaded()))
         totalLoaded++;
   }
   return totalLoaded;
}

//============================================================================
// BGrannyManager::findModel
//============================================================================
long BGrannyManager::findModel(const BCHAR_T* pFileName)
{
   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);

   // Look up in name table.
   short index=-1;
   bool found=mModelNameTable.find(standardizedFilename, &index);

   // If we found a match, give back the index.
   if(found)
      return(index);

   // No match, give back -1.
   return(-1);
}

//============================================================================
// BGrannyManager::createModel
//============================================================================
long BGrannyManager::createModel(const BCHAR_T* pFileName, bool loadFile)
{
   // Allocate model.
   BGrannyModel* pModel = new BGrannyModel;
   if(!pModel)
   {
      BFAIL("Could not allocate granny model.");
      return(-1);
   }
   
   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);

   // Load the file
   if (loadFile)
   {
      // Initialize it.
      bool ok=pModel->init(mBaseDirectory, standardizedFilename);

      // If init failed (file not found, file corrupt, etc), clean up and bail.
      if(!ok)
      {
         delete pModel;
         return(-1);
      }
   }
   // Set the file name so the file can be loaded later
   else
   {
      pModel->setFilename(standardizedFilename);
   }
   
   long index;
   for (index = 0; index < mModels.getNumber(); index++)
      if (!mModels[index])
         break;
         
   if (index == mModels.getNumber())
   {
      // Add to array.
      long index = mModels.getNumber();
         
      if(!mModels.setNumber(index + 1))
      {
         BFAIL("Could not allocate space in model array.");
         return(-1);
      }
   }
         
   mModels[index] = pModel;
   
   pModel->setManagerIndex(index);

   // Add to hash table.
   mModelNameTable.add(standardizedFilename, static_cast<short>(index));

   // Hand back index.
   return(index);
}

//============================================================================
// BGrannyManager::getOrCreateModel
//============================================================================
long BGrannyManager::getOrCreateModel(const BCHAR_T* pFileName, bool loadFile)
{
   //SCOPEDSAMPLE(BGrannyManager_getOrCreateModel)

   BDEBUG_ASSERT(pFileName);
   
   // Look for existing.
   long index = findModel(pFileName);

   // None existing, must create.
   if (index < 0)
   {
      index = createModel(pFileName, loadFile);
   }
   else if (loadFile)
   {
      BGrannyModel* pModel = getModel(index, false);
      if ((pModel) && (!pModel->isLoaded()))
         pModel->init(mBaseDirectory, pFileName);
   }

   // Give back result.
   return (index);
}

//============================================================================
// BGrannyManager::getModel
//============================================================================
BGrannyModel* BGrannyManager::getModel(long index, bool ensureLoaded)
{
   //SCOPEDSAMPLE(BGrannyManager_getModel)
   // Check for bogus index.
   if(index<0 || index>=mModels.getNumber())
      return(NULL);

   BGrannyModel* pModel = mModels[index];

   if (ensureLoaded && pModel)
   {
      if (!pModel->isLoaded() && !pModel->loadFailed())
         pModel->init(mBaseDirectory, pModel->getFilename().getPtr());

      // don't return unloaded models if ensureLoaded was set
      if (!pModel->isLoaded())
         pModel = NULL;
   }

   return pModel;
}

//============================================================================
// BGrannyManager::getNumLoadedAnimations
//============================================================================
long BGrannyManager::getNumLoadedAnimations() const
{
   long totalLoaded = 0;
   for (uint i = 0; i < mAnimations.getSize(); i++)
   {
      if ((mAnimations[i]) && (mAnimations[i]->isLoaded()))
         totalLoaded++;
   }
   return totalLoaded;
}

//============================================================================
// BGrannyManager::findAnimation
//============================================================================
long BGrannyManager::findAnimation(const BCHAR_T* pFileName)
{
   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);

   // Look up in name table.
   short index=-1;
   bool found=mAnimationNameTable.find(standardizedFilename, &index);

   // If we found a match, give back the index.
   if(found)
      return(index);

   // No match, give back -1.
   return(-1);
}

//============================================================================
// BGrannyManager::createAnimation
//============================================================================
long BGrannyManager::createAnimation(const BCHAR_T* pFileName, bool loadFile)
{
   // Allocate Animation.
   BGrannyAnimation* pAnim = new BGrannyAnimation;
   if(!pAnim)
   {
      BFAIL("Could not allocate granny Animation.");
      return(-1);
   }

   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);

   // Load the file
   if (loadFile)
   {
      // Initialize it.
      bool ok=pAnim->init(mBaseDirectory, standardizedFilename);

      // If init failed (file not found, file corrupt, etc), clean up and bail.
      if(!ok)
      {
         delete pAnim;
         return(-1);
      }
   }
   // Set the file name so the file can be loaded later
   else
   {
      pAnim->setFilename(standardizedFilename);
   }

   // Add to array.
   long index = mAnimations.getNumber();
   if(!mAnimations.setNumber(index + 1))
   {
      delete pAnim;
      BFAIL("Could not allocate space in Animation array.");
      return(-1);
   }
   mAnimations[index] = pAnim;

   // Add to hash table.
   mAnimationNameTable.add(standardizedFilename, static_cast<short>(index));

   // Hand back index.
   return(index);
}

//============================================================================
// BGrannyManager::getOrCreateAnimation
//============================================================================
long BGrannyManager::getOrCreateAnimation(const BCHAR_T* pFileName, bool loadFile, bool synced)
{
    #ifdef SYNC_Anim
      if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncAnimData("BGrannyManager::getOrCreateAnimation pFileName", pFileName);
         syncAnimData("BGrannyManager::getOrCreateAnimation loadFile", loadFile);
      }
   #endif

   //SCOPEDSAMPLE(BGrannyManager_getOrCreateAnimation)
   // Check param.
   if(!pFileName)
   {
      BFAIL("NULL pFileName.");
      return(-1);
   }

   // Look for existing.
   long index=findAnimation(pFileName);

   #ifdef SYNC_Anim
      if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncAnimData("BGrannyManager::getOrCreateAnimation index<0", index<0);
      }
   #endif

   // None existing, must create.
   if(index<0)
   {
      index=createAnimation(pFileName, loadFile);
   }
   else
   {
      if (loadFile)
      {
         BGrannyAnimation* pAnim = getAnimation(index, false);
         if ((pAnim != NULL) && (pAnim->isLoaded() == false) && (pAnim->loadFailed() == false))
            pAnim->init(mBaseDirectory, pFileName, synced);
      }
   }

   // Give back result.
   return(index);
}

//============================================================================
// BGrannyManager::getAnimation
//============================================================================
BGrannyAnimation* BGrannyManager::getAnimation(long index, bool ensureLoaded, bool synced)
{
   #ifdef SYNC_Anim
      if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncAnimData("BGrannyManager::getAnimation (index<0 || index>=mAnimations.getNumber())", (index<0 || index>=mAnimations.getNumber()));
      }
   #endif

   //SCOPEDSAMPLE(BGrannyManager_getAnimation)
   // Check for bogus index.
   if(index<0 || index>=mAnimations.getNumber())
      return(NULL);

   BGrannyAnimation* pAnim = mAnimations[index];
   if (ensureLoaded && pAnim)
   {
      if (!pAnim->isLoaded() && !pAnim->loadFailed())
         pAnim->init(mBaseDirectory, pAnim->getFilename().getPtr(), synced);

      // don't return unloaded animations if ensureLoaded was set
      if (!pAnim->isLoaded())
         pAnim = NULL;
   }

   return pAnim;
}

//============================================================================
// BGrannyManager::createInstance
//============================================================================
BGrannyInstance* BGrannyManager::createInstance()
{
   BGrannyInstance* pInst = new BGrannyInstance;
   long index = mInstances.add(pInst);
   pInst->setGMIndex(index);
   pInst->incRef();
   return(pInst);
}

//============================================================================
// BGrannyManager::incInstanceRef
//============================================================================
void BGrannyManager::incInstanceRef(BGrannyInstance* pInst)
{
   if (pInst)
      pInst->incRef();
}

//============================================================================
// BGrannyManager::releaseInstance
//============================================================================
void BGrannyManager::releaseInstance(BGrannyInstance* pInst)
{
   // The renderer locks the instances while it's using them, to avoid
   // having to inc/dec the refcounts of all visible instances every frame.
   BDEBUG_ASSERT(!mInstancesLocked);
   
   if(pInst == NULL)
      return;

   long index = pInst->getGMIndex();
   long endIndex = mInstances.getNumber()-1;

   if((index < 0) || (index > endIndex))
      return;

   pInst->decRef();

   // If nothing else is referencing this then delete it
   if (pInst->getRefCount() <= 0)
   {
      delete mInstances[index];
      mInstances[index] = NULL;
      if(index!=endIndex)
      {
         BGrannyInstance* pEndInst=mInstances[endIndex];
         mInstances[index]=pEndInst;
         if(pEndInst)
            pEndInst->setGMIndex(index);
      }
      mInstances.setNumber(endIndex);
   }
}

//============================================================================
// BGrannyManager::convertCoordinateSystem
//============================================================================
void BGrannyManager::convertCoordinateSystem(granny_file_info* pGrannyFileInfo, bool model, bool flipWinding)
{
   model;
   
   if (strcmp(pGrannyFileInfo->ArtToolInfo->FromArtToolName, POST_GRANNY_EXPORT_TOOL_NAME) == 0)
      return;
      
   // Transform from the art tool's coordinate system to the game's coordinate system.
   granny_real32 affine3[3];
   granny_real32 linear3x3[9];
   granny_real32 inverseLinear3x3[9];
   BVector forward =  cZAxisVector;
   BVector right   = -cXAxisVector;

   GrannyComputeBasisConversion(pGrannyFileInfo, pGrannyFileInfo->ArtToolInfo->UnitsPerMeter / 64.0f, 
      (float*)&cOriginVector, (float*)&right, (float*)&cYAxisVector, (float*)&forward,
      affine3, linear3x3, inverseLinear3x3);

   GrannyTransformFile(pGrannyFileInfo, affine3, linear3x3, inverseLinear3x3, 1e-5f, 1e-5f, (flipWinding ? GrannyReorderTriangleIndices : 0) | GrannyRenormalizeNormals);
}

//==============================================================================
//==============================================================================
BGrannySampleAnimCache* BGrannyManager::createSampleAnimCache()
{
   BGrannySampleAnimCache* pCache = BGrannySampleAnimCache::getInstance();
   if (!pCache)
      return NULL;

   if (!pCache->mLocalPose)
   {
      BGrannySampleAnimCache::releaseInstance(pCache);
      return NULL;
   }

   mSampleAnimCacheIndex++;
   mSampleAnimCacheRefCount++;
   mSampleAnimCacheCounter++;

   return pCache;
}

//==============================================================================
//==============================================================================
void BGrannyManager::releaseSampleAnimCache(BGrannySampleAnimCache* pCache)
{   
   if (!pCache)
      return;   

   BGrannySampleAnimCache::releaseInstance(pCache);

   mSampleAnimCacheRefCount--;
}

#ifndef BUILD_FINAL
//==============================================================================
// BGrannyManager::getModelAllocStats
//==============================================================================
void BGrannyManager::getModelAllocStats(BAssetAllocStatsArray& stats)
{
   for (uint i = 0; i < mModels.getSize(); i++)
   {
      if ((mModels[i]) && (mModels[i]->isLoaded()))
      {
         BAssetAllocStats* p = stats.enlarge(1);
         p->mFilename = mModels[i]->getFilename();
         p->mAllocationSize = mModels[i]->getAllocationSize();
      }
   }
}
   
//==============================================================================
// BGrannyManager::getAnimAllocStats
//==============================================================================
void BGrannyManager::getAnimAllocStats(BAssetAllocStatsArray& stats)
{
   for (uint i = 0; i < mAnimations.getSize(); i++)
   {
      if ((mAnimations[i]) && (mAnimations[i]->isLoaded()))
      {
         BAssetAllocStats* p = stats.enlarge(1);
         p->mFilename = mAnimations[i]->getFilename();
         p->mAllocationSize = mAnimations[i]->getAllocationSize();
      }
   }
}
#endif // BUILD_FINAL


//==============================================================================
//==============================================================================
void BGrannyManager::beginRenderPrepare()
{
   #ifndef BUILD_FINAL
   // There shouldn't be any instances in the list right now.
   BASSERT(mRenderPrepareInstances.getNumber() == 0);

   for(long i=0; i<cNumRenderPrepareCacheBuckets; i++)
   {
      // No one should still be holding on to a pose... they should have all been released at this point.
      BASSERT(mRenderPreparePoseCache[i].mUsedCount == 0);
   }
   #endif
}


//==============================================================================
//==============================================================================
granny_local_pose* BGrannyManager::getRenderPreparePose(BGrannyInstance *instance)
{
   // Sanity check.
   if(!instance || !instance->getModelInstance())
   {
      BFAIL("Trying to prepare rendering on a null instance");
      return(NULL);
   }
   
   // Remember we've got to clean up this instance's cached data when we call endRenderPrepare
   mRenderPrepareInstances.add(instance);

   // Get the skeleten from the granny instance   
   const granny_skeleton* pSkeleton=GrannyGetSourceSkeleton(instance->getModelInstance());
   if(!pSkeleton)
   {
      BFAIL("No skeleton");
      return(NULL);
   }
   
   // Get the bone count we need.
   long numBones = pSkeleton->BoneCount;

   // Find the bucket we need.
   for(long bucketIndex = 0; bucketIndex<cNumRenderPrepareCacheBuckets; bucketIndex++)
   {
      // If we fit in this bucket, break out now.
      if(numBones <= cRenderPrepareCacheBucketSizes[bucketIndex])
         break;
   }
   
   // Sanity.
   if(bucketIndex >= cNumRenderPrepareCacheBuckets)
   {
      BFAIL("Did not find pose cache bucket");
      return(NULL);
   }
      
   // How many poses currently in cache?
   long numPoses = mRenderPreparePoseCache[bucketIndex].mPoses.getNumber();
   
   // Do we have one allocated that we can give out?
   granny_local_pose *newPose;
   if(numPoses > 0)
   {
      // Get the pose from the cache.
      long lastIndex = numPoses-1;
      newPose = mRenderPreparePoseCache[bucketIndex].mPoses[lastIndex];
      
      // One less free pose in the cache.
      mRenderPreparePoseCache[bucketIndex].mPoses.setNumber(lastIndex);
   }
   else
   {
      // There wasn't room in the cache, so allocate a new entry.
      newPose = GrannyNewLocalPose(cRenderPrepareCacheBucketSizes[bucketIndex]);
   }

   // Keep track of how poses we've given out so that we can test if everyone is releasing them properly.
   mRenderPreparePoseCache[bucketIndex].mUsedCount++;

   // Hand it back
   return(newPose);
}


//==============================================================================
//==============================================================================
void BGrannyManager::releaseRenderPreparePose(granny_local_pose *pose, long numBones)
{
   // Don't tell us to release NULL!
   if(!pose)
   {
      BFAIL("Attempting to release null pose");
      return;
   }
   
   // Find the bucket we need.
   for(long bucketIndex = 0; bucketIndex<cNumRenderPrepareCacheBuckets; bucketIndex++)
   {
      // If we fit in this bucket, break out now.
      if(numBones <= cRenderPrepareCacheBucketSizes[bucketIndex])
         break;
   }

   // Sanity.
   if(bucketIndex >= cNumRenderPrepareCacheBuckets)
   {
      BFAIL("Did not find pose cache bucket");
      GrannyFreeLocalPose(pose);
      return;
   }

   // Put it back in the cache free list.
   mRenderPreparePoseCache[bucketIndex].mPoses.add(pose);
   
   // One less pose is currently handed out.
   mRenderPreparePoseCache[bucketIndex].mUsedCount--;
}


//==============================================================================
//==============================================================================
void BGrannyManager::endRenderPrepare()
{
   // Go through all the instances that were rendered and clean up their local pose we stored.
   for(long i=mRenderPrepareInstances.getNumber()-1; i>=0; i--)
   {
      // Get instance.
      BGrannyInstance *instance = mRenderPrepareInstances[i];
      
      // We check these for null-ness on the way in so they should never be any nulls in here.
      BASSERT(instance);
      
      // Let it know to clean up.
      instance->cleanupRenderPrepare();
   }
   
   // List is now empty.
   mRenderPrepareInstances.setNumber(0);
   
   // No one should still be holding on to a pose... they should have all been released at this point.
   #ifndef BUILD_FINAL
   for(long i=0; i<cNumRenderPrepareCacheBuckets; i++)
   {
      BASSERT(mRenderPreparePoseCache[i].mUsedCount == 0);
   }
   #endif
}




//==============================================================================
//==============================================================================
IMPLEMENT_THREADSAFE_FREELIST(BGrannySampleAnimCache, 10, &gSimHeap);

//==============================================================================
//==============================================================================
BGrannySampleAnimCache::BGrannySampleAnimCache() :
   mLocalPose(NULL),
   mMaxBoneIndex(0),
   mStateChange(0),
   mLocalPoseSet(false),
   mUseIK(false)
{
}

//==============================================================================
//==============================================================================
BGrannySampleAnimCache::~BGrannySampleAnimCache()
{
   if (mLocalPose)
   {
      GrannyFreeLocalPose(mLocalPose);
      mLocalPose = NULL;
   }
}

//==============================================================================
//==============================================================================
void BGrannySampleAnimCache::onAcquire()
{
   if (!mLocalPose)
      mLocalPose = GrannyNewLocalPose(BGrannyManager::cMaxBones);
   mMaxBoneIndex = 0;
   mStateChange = 0;
   mLocalPoseSet = false;
   mUseIK = false;
}

//==============================================================================
//==============================================================================
void BGrannySampleAnimCache::onRelease()
{
}

