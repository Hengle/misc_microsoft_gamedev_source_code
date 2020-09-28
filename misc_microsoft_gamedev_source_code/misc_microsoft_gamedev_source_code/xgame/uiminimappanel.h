//============================================================================
// uiResourcePanel.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#pragma once
#include "visual.h"

//============================================================================
//============================================================================
class BUIMinimapPanel
{
   public:
      BUIMinimapPanel();
      ~BUIMinimapPanel();

      enum BUIMinimapPanelBoneType
      {
         eBoneMain,
         eBoneTotal
      };

      enum BUIResourceModels
      {
         eResourceModelTotal
      };

      void init();
      void deinit();

      void update(float elapsedTime);
      void renderFlash();
      void render();

   private:
      BDynamicArray<int>      mBoneHandle;
      BVisual*                mpModel;
};