//============================================================================
// uiResourcePanel.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#pragma once

#include "flashResourcePanel.h"
#include "visual.h"


//============================================================================
//============================================================================
class BUIResourcePanel
{
   public:
      BUIResourcePanel();
      ~BUIResourcePanel();

      enum BUIResourcePanelBoneType
      {
         eBoneMain,
         eBoneTopPanel,
         eBoneUNSCPop,
         eBoneUNSCPower,
         eBoneUNSCSupplies,
         eBoneCOVPop,
         eBoneCOVRelics,
         eBoneCOVFavor,
         eBoneCOVOrganics,
         eBoneTotal
      };      

      void init();
      void deinit();

      void update(float elapsedTime);
      void renderFlash();
      void render();

   private:
      BDynamicArray<int>      mBoneHandle;
      BFlashResourcePanel*    mpResourcePanel;
      BVisual*                mpModel;
      BDynamicArray<BVisual*> mResourceModels;
};