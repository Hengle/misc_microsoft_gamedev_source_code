//==============================================================================
// cameraEffectManager.cpp
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "cameraEffectManager.h"
#include "reloadManager.h"
#include "gamedirectories.h"

// Constants
#define CAMERA_EFFECT_FILENAME "cameraEffects.xml"

// Globals
BCameraEffectManager gCameraEffectManager;

//============================================================================
//============================================================================
BCameraEffectManager::BCameraEffectManager()
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit(cThreadIndexSim, true);
#endif
}

//============================================================================
//============================================================================
BCameraEffectManager::~BCameraEffectManager()
{
   mCameraEffects.clear();
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit(true);
#endif
}

//============================================================================
//============================================================================
void BCameraEffectManager::init()
{
   load();
}

//============================================================================
//============================================================================
int BCameraEffectManager::findCameraEffect(const BCHAR_T* pName)
{
   // Find
   for (int i = 0; i < mCameraEffects.getNumber(); i++)
   {
      if (mCameraEffects[i].mName.compare(pName) == 0)
         return i;
   }

   return -1;
}

//============================================================================
//============================================================================
int BCameraEffectManager::createCameraEffect(const BCHAR_T* pName)
{
   BCameraEffectData newCamEffect;
   newCamEffect.mName.set(pName);
   int index = mCameraEffects.add(newCamEffect);
   return index;
}

//============================================================================
//============================================================================
int BCameraEffectManager::getOrCreateCameraEffect(const BCHAR_T* pName)
{
   // Check param.
   if(!pName)
   {
      BFAIL("NULL pFileName.");
      return(-1);
   }

   // Look for existing.
   long index = findCameraEffect(pName);

   // None existing, must create.
   if (index < 0)
   {
      index = createCameraEffect(pName);
   }

   // Give back result.
   return index;
}

//============================================================================
//============================================================================
bool BCameraEffectManager::load()
{
#ifdef ENABLE_RELOAD_MANAGER
   // Deregister so it's not registered twice
   gReloadManager.deregisterClient(mEventHandle);
#endif

   // Register
   BSimString fileNameWithExtension(CAMERA_EFFECT_FILENAME);

#ifdef ENABLE_RELOAD_MANAGER
   BReloadManager::BPathArray paths;
   BString fullpath;
   gFileManager.constructQualifiedPath(cDirData, fileNameWithExtension, fullpath);
   paths.pushBack(fullpath);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle);
#endif

   // Read data
   BXMLReader reader;
   if(!reader.load(cDirData, fileNameWithExtension))
   {
      gConsoleOutput.output(cMsgError, "BCameraEffectManager ERROR: %s didn't load.", fileNameWithExtension);
      return false;
   }

   BXMLNode rootNode(reader.getRootNode());
   if (rootNode.getNumberChildren() <= 0)
   {
      gConsoleOutput.output(cMsgError, "BCameraEffectManager ERROR: %s has no root node.", fileNameWithExtension);
      return false;
   }

   // Load each camera effect in the file
   for (int i = 0; i < rootNode.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(rootNode.getChild(i));
      if(!child)
         continue;

      if (child.getName().compare("CameraEffect") == 0)
      {
         // Use getOrCreate to get existing effect if name matches
         BSimString tempStr;
         child.getChildValue("Name", &tempStr);
         int index = getOrCreateCameraEffect(tempStr);

         // Load
         mCameraEffects[index].load(child);

         // Hack - All effects loaded from camEffects.xml are considered mode effects
         mCameraEffects[index].mModeCameraEffect = true;
      }
   }

   return true;
}

//============================================================================
//============================================================================
void BCameraEffectManager::reload()
{
   // Don't reset the array of effects as indices will change
   load();
}

//============================================================================
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BCameraEffectManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassReloadNotify:
      {
         gConsoleOutput.status("Reloading cameraEffects.xml");
         reload();
         break;
      }
   }
   
   return false;      
}
#endif