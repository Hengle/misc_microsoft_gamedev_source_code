//============================================================================
// uiResourcePanel.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#pragma once
#include "visual.h"

//============================================================================
//============================================================================
class BUIUnitPanel
{
public:
   BUIUnitPanel();
   ~BUIUnitPanel();

   enum BUIUnitPanelBoneType
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