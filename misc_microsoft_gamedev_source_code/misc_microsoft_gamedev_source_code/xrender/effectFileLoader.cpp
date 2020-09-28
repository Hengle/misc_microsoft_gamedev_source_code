//============================================================================
//
// File: effectFileLoader.cpp
//
// Copyright (c) 2005-2006, Ensemble Studios
//
// rg
//
//============================================================================
#include "xrender.h"
#include "effectFileLoader.h"
#include "BD3D.h"
#include "consoleOutput.h"

//============================================================================
// BFXLEffectFileLoader::BFXLEffectFileLoader
//============================================================================
BFXLEffectFileLoader::BFXLEffectFileLoader() :
   mStatus(cStatusInvalid),
   mLoadCount(1),
   mValidateIntrinsics(false),
   mDirtyFlag(false),
   mpIntrinsicPool(NULL)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
}

//============================================================================
// BFXLEffectFileLoader::~BFXLEffectFileLoader
//============================================================================
BFXLEffectFileLoader::~BFXLEffectFileLoader()
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   deinit();
}

//============================================================================
// BFXLEffectFileLoader::init
//============================================================================
bool BFXLEffectFileLoader::init(long dirID, const char* pFilename, bool validateIntrinsics, bool waitUntilLoaded, bool discardOnClose, BFXLEffectIntrinsicPool* pIntrinsicPool)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   deinit();
   
   mDirID = dirID;
   mFilename.set(pFilename);
   mValidateIntrinsics = validateIntrinsics;
   mpIntrinsicPool = pIntrinsicPool;

#ifdef ENABLE_RELOAD_MANAGER
   mFileWatcher.clear(false);
   mFileWatcher.add(mDirID, mFilename);
#endif

   const bool status = mFileLoader.load(mDirID, mFilename, false, discardOnClose);
   
   if (!status)
   {
      deinit();
      return false;
   }
   
   mStatus = cStatusLoadPending;
   
   if (waitUntilLoaded)
   {
      if (!waitUntilReadyOrFailed())
         return false;
      
      return createEffect();
   }
   
   return true;
}

//============================================================================
// BFXLEffectFileLoader::deinit
//============================================================================
void BFXLEffectFileLoader::deinit(void)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   mFileLoader.clear();
#ifdef ENABLE_RELOAD_MANAGER
   mFileWatcher.clear(false);
#endif
   mDirID = 0;
   mFilename.empty();
   mEffect.clear();
   
   mStatus = cStatusInvalid;
   mValidateIntrinsics = false;
   mDirtyFlag = false;
   mpIntrinsicPool = NULL;
}

//============================================================================
// BFXLEffectFileLoader::reload
//============================================================================
bool BFXLEffectFileLoader::reload(void)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   if (mStatus == cStatusInvalid)
      return false;
      
   gConsoleOutput.resource("BFXLEffectFileLoader::createEffect: Reloading effect file %s", mFilename.getPtr());      
   
   mFileLoader.load(mDirID, mFilename);
   
   return true;
}

//============================================================================
// BFXLEffectFileLoader::createEffect
// Returns true if the current effect has been created, false if unchanged.
//============================================================================
bool BFXLEffectFileLoader::createEffect(void)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   if (mStatus == cStatusInvalid)   
      return false;
      
   if ((!mFileLoader.getSucceeded()) || (!mFileLoader.getData()) || (!mFileLoader.getDataLen()))
   {
#ifdef ENABLE_RELOAD_MANAGER
      mFileLoader.clear();
#else
      mFileLoader.clear(true);
#endif
      
      if (mStatus == cStatusLoadPending)
      {
         gConsoleOutput.output(cMsgError, "BFXLEffectFileLoader::createEffect: Failed reading effect file %s", mFilename.getPtr());
      
         mStatus = cStatusLoadFailed;
      }
            
      return false;
   }
         
   BFXLEffect newEffect;
   const HRESULT hres = newEffect.createFromCompiledData(BD3D::mpDev, mFileLoader.getData(), mpIntrinsicPool, mValidateIntrinsics);

#ifdef ENABLE_RELOAD_MANAGER
   mFileLoader.clear();
#else
   mFileLoader.clear(true);
#endif
            
   if (FAILED(hres))
   {
      gConsoleOutput.output(cMsgError, "BFXLEffectFileLoader::createEffect: Failed creating effect file %s", mFilename.getPtr());
      
      if (mStatus == cStatusLoadPending)
      {
         mStatus = cStatusLoadFailed;
      }
         
      return false;
   }
   
   gConsoleOutput.resource("BFXLEffectFileLoader::createEffect: Successfully loaded effect file %s", mFilename.getPtr());
   
   mLoadCount++;

   mDirtyFlag = true;
   
   mEffect.attach(newEffect.getEffect());
   
   mStatus = cStatusValid;
            
   return true;   
}

//============================================================================
// BFXLEffectFileLoader::tick
// Returns true if the current effect has been changed or deleted.
//============================================================================
bool BFXLEffectFileLoader::tick(bool waitUntilLoaded)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   if (mStatus == cStatusInvalid)
      return false;
      
   if ((waitUntilLoaded) && (mStatus == cStatusLoadPending))
   {
      if (!waitUntilReadyOrFailed())
         return false;
   }
   
   if (mFileLoader.getReady())      
      return createEffect();

#ifdef ENABLE_RELOAD_MANAGER
   if (mFileWatcher.getAreAnyDirty())
   {
      // This requests the file to be loaded, but the existing effect will remain unchanged.
      reload();      
   }
#endif
   return false;
}

//============================================================================
// BFXLEffectFileLoader::waitUntilReadyOrFailed
//============================================================================
bool BFXLEffectFileLoader::waitUntilReadyOrFailed(bool reloadIfLoadFailed)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadIndex() == cThreadIndexRender);
   
   if (mStatus == cStatusInvalid)
      return false;
   
   if (mStatus == cStatusValid)   
      return true;
   else if (mStatus == cStatusLoadFailed)       
   {
      if (!reloadIfLoadFailed)
         return false;
         
      reload();
   }
   
   if (!mFileLoader.waitUntilReady())
      return false;

   return true;
}

