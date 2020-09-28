//==============================================================================
// cameraEffectManager.h
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

#pragma once

#include "camera.h"

class BCameraEffectManager
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
   public:
      BCameraEffectManager();
      virtual ~BCameraEffectManager();

      void                       init();

      int                        findCameraEffect(const BCHAR_T* pName);
      int                        createCameraEffect(const BCHAR_T* pName);
      int                        getOrCreateCameraEffect(const BCHAR_T* pName);
      const BCameraEffectData*   getCameraEffect(int index) const { if (index < 0 || index > mCameraEffects.getNumber()) return NULL; else return &mCameraEffects[index]; }

   protected:

      bool                       load();
      void                       reload();
#ifdef ENABLE_RELOAD_MANAGER
      bool                       receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
      BSmallDynamicSimArray<BCameraEffectData> mCameraEffects;
};

extern BCameraEffectManager gCameraEffectManager;
