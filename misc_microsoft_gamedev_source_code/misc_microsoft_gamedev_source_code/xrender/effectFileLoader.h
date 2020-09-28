//============================================================================
//
// File: effectFileLoader.h
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "effect.h"
#include "asyncFileManager.h"
#include "asyncFileLoader.h"
#include "reloadManager.h"

//============================================================================
// class BFXLEffectFileLoader
// This class can only be constructed/destructed on the render thread!
//============================================================================
class BFXLEffectFileLoader 
{
public:
   BFXLEffectFileLoader();
   ~BFXLEffectFileLoader();
   
   // init() kicks off an async file request for the effect. 
   // The effect won't be usable immediately, unless waitUntilLoaded is true.
   // If waitUntilLoaded is true, this method will spin until the file requests succeeds or fails. This will be very slow, so don't do it!
   bool init(long dirID, const char* pFilename, bool validateIntrinsics = true, bool waitUntilLoaded = false, bool discardOnClose = false, BFXLEffectIntrinsicPool* pIntrinsicPool = NULL);
   
   void deinit(void);
   
   long getDirID(void) const { return mDirID; }
   const BString& getFilename(void) const { return mFilename; }
   
   // The load count is incremented every time the effect file is reloaded.
   DWORD getLoadCount(void) const { return mLoadCount; }
   
   BFXLEffect& getFXLEffect(void) { return mEffect; }
   bool isEffectValid(void) const { return (mEffect.getEffect() != NULL) && (mStatus == cStatusValid); }
   
   enum eStatus
   {
      cStatusInvalid,
      cStatusLoadPending,
      cStatusValid,
      cStatusLoadFailed
   };
   
   eStatus getStatus(void) const { return mStatus; }
            
   bool reload(void);
   
   // tick() must be called before you use the effect.
   // Returns true if the effect has been created/changed. In this case, you must recreate your effect parameters (check isEffectValid() first!).
   // The effect is guaranteed to be valid if tick() returns true.
   bool tick(bool waitUntilLoaded = false);
   
   // Waits until the effect file is ready to be created by calling tick().
   // Triggers a reload if reloadIfLoadFailed if the previous load request failed.
   // Returns true if the file has loaded.
   // If this method returns true, you must call tick() to create the effect!
   // If the status was already valid this method simply returns true.
   bool waitUntilReadyOrFailed(bool reloadIfLoadFailed = true);
   
   bool getDirtyFlag(void) const { return mDirtyFlag; }
   void clearDirtyFlag(void) { mDirtyFlag = false; }
      
private:
   BFXLEffect                 mEffect;
   BAsyncFileLoader           mFileLoader;
#ifdef ENABLE_RELOAD_MANAGER
   BFileWatcher               mFileWatcher;
#endif
   BFXLEffectIntrinsicPool*   mpIntrinsicPool;
   
   long                       mDirID;
   BString                    mFilename;
   
   DWORD                      mLoadCount;
               
   eStatus                    mStatus;
   bool                       mValidateIntrinsics;
   bool                       mDirtyFlag;
         
   bool              createEffect(void);
};
