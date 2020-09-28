//============================================================================
// grannyanimation.cpp
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "grannyanimation.h"
#include "grannymanager.h"

// xcore
#include "consoleOutput.h"
#include "resource\ecfFileData.h"

// xsystem
#include "reloadManager.h"
#include "workdirsetup.h"
#include "bfileStream.h"

// xvisual
#include "visualmanager.h"

// xgeom
#include "uaxDefs.h"

//xgame
#include "..\xgame\syncmacros.h"

namespace
{
   void cleanFilename(const char *name, BSimString& cleanedName)
   {
      cleanedName.set(name);
      cleanedName.standardizePath();

      long index = cleanedName.findRight(B("\\art\\"));
      if(index != -1)
         cleanedName.crop(index + 5, cleanedName.length());

      strPathRemoveExtension(cleanedName);
   }   
}

//============================================================================
// BGrannyAnimation::BGrannyAnimation
//============================================================================
BGrannyAnimation::BGrannyAnimation() :
   mpGrannyFileInfo(NULL),
   mMotionExtractionMode(cNoExtraction),
   mLoadFailed(false),
   mHasModelData(false),
   mAllocationSize(0)
{
   mTotalMotionExtraction.zero();
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit(cThreadIndexSim, true);
#endif
}

//============================================================================
// BGrannyAnimation::~BGrannyAnimation
//============================================================================
BGrannyAnimation::~BGrannyAnimation()
{
   deinit();
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit(true);
#endif
}

//============================================================================
// BGrannyAnimation::deinit
//============================================================================
void BGrannyAnimation::deinit()
{
   mTotalMotionExtraction.zero();
   if(mpGrannyFileInfo)
   {
      bool success = gRenderHeap.Delete(mpGrannyFileInfo);
      success;
      BDEBUG_ASSERT(success);
      
      mpGrannyFileInfo = NULL;
      
      mAllocationSize = 0;
   }
}

//============================================================================
// BGrannyAnimation::init
//============================================================================
bool BGrannyAnimation::init(const BSimString& basePath, const BCHAR_T* pFileName, bool synced)
{  
   mLoadFailed = true;

   // Cleanse out any existing info.
   deinit();

   // Check param.
   if (!pFileName)
      return(false);

   // Remember filename.
   cleanFilename(pFileName, mFilename);
   mBasePath = basePath;

   return load(synced);
}

//============================================================================
// BGrannyAnimation::load
//============================================================================
bool BGrannyAnimation::load(bool synced)
{
   mLoadFailed = true;
   
   deinit();

#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
#endif

   BSimString fullPath;
   fullPath.format("%s%s", mBasePath.getPtr(), mFilename.getPtr());
   if (strPathHasExtension(fullPath, "gr2"))
      strPathRemoveExtension(fullPath);

   strPathAddExtension(fullPath, UAX_ANIM_EXTENSION);

#ifdef ENABLE_RELOAD_MANAGER
   BReloadManager::BPathArray paths;
   paths.pushBack(fullPath);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle);
#endif

#ifndef XBOX
   // Reset the floating point state because Granny does some floating point calculations
   // with the data that will cause OOS errors if the state is wrong.
   _control87( _CW_DEFAULT, 0xfffff );
#endif   

   BFile file;
   if (!file.openReadOnly(cDirProduction, fullPath, BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      #ifdef SYNC_Anim
         if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncAnimData("BGrannyAnimation::load: Could not open file:", fullPath.getPtr());
         }
      #endif
      gConsoleOutput.output(cMsgError, "BGrannyAnimation::load: Could not open file: %s", fullPath.getPtr());
      return false;
   }

   BFileSystemStream stream;
   stream.open(&file, fullPath, cSFReadable|cSFNoAutoClose);
   
   BECFFileData* pECFFileData = HEAP_NEW(BECFFileData, gSimHeap);
   if (!pECFFileData->load(&stream, false, true))
   {
      HEAP_DELETE(pECFFileData, gSimHeap);

      #ifdef SYNC_Anim
         if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncAnimData("BGrannyAnimation::load: Could not read ECF file:", fullPath.getPtr());
         }
      #endif
      gConsoleOutput.output(cMsgError, "BGrannyAnimation::load: Could not read ECF file: %s", fullPath.getPtr());
      return false;
   }

   stream.close();

   file.close();
   
   if (pECFFileData->getHeaderID() != BUAX::cECFFileID)
   {
      HEAP_DELETE(pECFFileData, gSimHeap);
      pECFFileData = NULL;

      #ifdef SYNC_Anim
         if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncAnimData("BGrannyAnimation::load: Invalid UAX file:", fullPath.getPtr());
         }
      #endif

      gConsoleOutput.output(cMsgError, "BGrannyAnimation::load: Invalid UAX file: %s", fullPath.getPtr());
      return false;
   }
   
   int uaxChunkIndex = pECFFileData->findChunkByID((uint64)BUAX::cECFUAXChunkID);
   if (cInvalidIndex == uaxChunkIndex)
   {
      HEAP_DELETE(pECFFileData, gSimHeap);
      pECFFileData = NULL;

      #ifdef SYNC_Anim
         if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncAnimData("BGrannyAnimation::load: Could not find UAX chunk:", fullPath.getPtr());
         }
      #endif
      
      gConsoleOutput.output(cMsgError, "BGrannyAnimation::load: Could not find UAX chunk: %s", fullPath.getPtr());
      return false;
   }
   
   const uint uaxChunkDataLen = pECFFileData->getChunkDataLen(uaxChunkIndex);
         
   if (uaxChunkDataLen < sizeof(granny_file_info))
   {
      HEAP_DELETE(pECFFileData, gSimHeap);
      pECFFileData = NULL;

      #ifdef SYNC_Anim
         if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncAnimData("BGrannyAnimation::load: UAX chunk is invalid:", fullPath.getPtr());
         }
      #endif

      gConsoleOutput.output(cMsgError, "BGrannyAnimation::load: UAX chunk is invalid: %s", fullPath.getPtr());
      return false;
   }
               
   granny_file_info* pGrannyFileInfo = static_cast<granny_file_info*>(pECFFileData->getChunkDataPtr(uaxChunkIndex));
   
   bool success = GrannyRebasePointers(GrannyFileInfoType, pGrannyFileInfo, (int)pGrannyFileInfo, true);
   if (!success)
   {
      HEAP_DELETE(pECFFileData, gSimHeap);
      pECFFileData = NULL;

      #ifdef SYNC_Anim
         if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncAnimData("BGrannyAnimation::load: GrannyRebasePointers failed", fullPath.getPtr());
         }
      #endif
      
      gConsoleOutput.output(cMsgError, "BGrannyAnimation::load: GrannyRebasePointers failed %s", fullPath.getPtr());
      return(false);         
   }
   
   if ( 
       (pGrannyFileInfo->AnimationCount < 1) ||
       (!pGrannyFileInfo->FromFileName) || 
#if defined(BUILD_DEBUG) && !defined(BUILD_CHECKED)
      IsBadReadPtr(pGrannyFileInfo->FromFileName, 7) || 
#endif        
      (_stricmp(pGrannyFileInfo->FromFileName, "gr2ugx") != 0) )
   {
      HEAP_DELETE(pECFFileData, gSimHeap);
      pECFFileData = NULL;

      #ifdef SYNC_Anim
         if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
         {
            syncAnimData("BGrannyAnimation::load: UAX Granny data is invalid:", fullPath.getPtr());
         }
      #endif
      
      gConsoleOutput.output(cMsgError, "BGrannyAnimation::load: UAX Granny data is invalid: %s", fullPath.getPtr());
      return(false);
   }
   
   BDEBUG_ASSERT(&gRenderHeap == pECFFileData->getChunkDataHeap(uaxChunkIndex));
   pECFFileData->acquireChunkDataOwnership(uaxChunkIndex);
   
   HEAP_DELETE(pECFFileData, gSimHeap);
   pECFFileData = NULL;
     
   BDEBUG_ASSERT(!mpGrannyFileInfo);
   mpGrannyFileInfo = pGrannyFileInfo;   
   pGrannyFileInfo = NULL;
   
   long trackGroupFlag = mpGrannyFileInfo->TrackGroupCount ? mpGrannyFileInfo->TrackGroups[0]->Flags : 0;
   if (trackGroupFlag & GrannyAccumulationIsVDA)
      setMotionExtractionMode(cVariableExtraction);
   else if (trackGroupFlag & GrannyAccumulationExtracted)
      setMotionExtractionMode(cConstantExtraction);
   else
      setMotionExtractionMode(cNoExtraction);
   
   // Determine whether this has model data and should do skeleton mapping.
   // rg [3/13/07] - Huh? This can never happen according to VD.
   mHasModelData = (mpGrannyFileInfo->ModelCount>0 && mpGrannyFileInfo->Models && mpGrannyFileInfo->Models[0]);

   // Success.
   mLoadFailed = false;
   
   gConsoleOutput.output(cMsgResource, "Loaded UAX: %s", fullPath.getPtr());
   
   mAllocationSize = uaxChunkDataLen;
   
   return(true);
}

//============================================================================
// BGrannyAnimation::reload
//============================================================================
bool BGrannyAnimation::reload(void)
{
   if (mFilename.isEmpty())
      return false;
   
   return load();  
}

//============================================================================
// BGrannyAnimation::getDuration
//============================================================================
float BGrannyAnimation::getDuration() const
{
   // If no file, we can't do anything.
   if(!mpGrannyFileInfo || mpGrannyFileInfo->AnimationCount<1)
   {
      BFAIL("Can't get animation duration: granny animation not initialized.");
      return(0.0f);
   }

   return(mpGrannyFileInfo->Animations[0]->Duration);
}

//============================================================================
// BGrannyAnimation::receiveEvent
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BGrannyAnimation::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassReloadNotify)
   {
      gConsoleOutput.status("Reloading animation: %s", mFilename.getPtr());
      
      // Reload animation
      reload();

      // Fix bindings if the animation is playing
      int animIndex = gGrannyManager.findAnimation(mFilename);
      gGrannyManager.rebindAnimation(animIndex);
   }

   return false;
}
#endif