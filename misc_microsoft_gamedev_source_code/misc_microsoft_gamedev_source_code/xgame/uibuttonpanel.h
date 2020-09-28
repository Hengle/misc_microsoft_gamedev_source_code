//============================================================================
// uibuttonpanel.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#pragma once
#include "flashbuttonpanel.h"
#include "visual.h"

//============================================================================
//============================================================================
class BUIButtonPanel
{
public:
   BUIButtonPanel();
   ~BUIButtonPanel();

   enum BUIButtonPanelBoneType
   {
      eBoneMain,
      eBoneTotal
   };

   enum BUIResourceModels
   {
      eResourceModelTotal
   };

   void init(BVisual* pVisual, const char* flashFile);
   void deinit();

   void setLabelText(int labelID, const char* text);
   void setLabelVisible(int labelID, bool bVisible);
   void setAllLabelsVisible(bool bVisible);
   void refresh();
   void update(float elapsedTime);
   void renderFlash();
   void render();
   void renderDebug(int x, int y, int width, int height);

private:
   BDynamicArray<int>      mBoneHandle;
   BVisual*                mpModel;
   BFlashButtonPanel*      mpButtonPanel;
};