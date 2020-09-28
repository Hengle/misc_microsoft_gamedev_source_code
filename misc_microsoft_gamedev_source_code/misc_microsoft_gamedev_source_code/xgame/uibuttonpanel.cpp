//============================================================================
// uibuttonpanel.cpp
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#include "common.h"
#include "uibuttonpanel.h"
#include "uimanager.h"
#include "visual.h"
#include "visualmanager.h"
#include "uigame.h"
#include "flashbuttonpanel.h"

class BButtonBoneLookup
{
public:
   int      mEnum;
   char     mName[64];
};
#define BONEENUM(type, str) {BUIButtonPanel::type, str},
BButtonBoneLookup gMinimapBoneLookup[BUIButtonPanel::eBoneTotal]=
{
   BONEENUM(eBoneMain,         "bone_main")
};

//============================================================================
//============================================================================
BUIButtonPanel::BUIButtonPanel():
   mpModel(NULL),
   mpButtonPanel(NULL)
{

}

//============================================================================
//============================================================================
BUIButtonPanel::~BUIButtonPanel()
{

}

//============================================================================
//============================================================================
void BUIButtonPanel::init(BVisual* pVisual, const char* flashFile)
{
   BDEBUG_ASSERT(pVisual);
   BDEBUG_ASSERT(flashFile);
   mpButtonPanel = new BFlashButtonPanel();
   mpButtonPanel->init(flashFile);

   mpModel = pVisual;
   BDEBUG_ASSERT(mpModel);

   mBoneHandle.resize(eBoneTotal);
   int handle = -1;
   for (int i = 0; i < eBoneTotal; i++)
   {
      handle = mpModel->getBoneHandle(gMinimapBoneLookup[i].mName);
      mBoneHandle[i] = handle;
   }
}

//============================================================================
//============================================================================
void BUIButtonPanel::deinit()
{
   mpModel = NULL;

   if (mpButtonPanel)
   {
      mpButtonPanel->deinit();
      delete mpButtonPanel;
   }
   mpButtonPanel = NULL;
}

//============================================================================
//============================================================================
void BUIButtonPanel::setLabelText(int labelID, const char* text)
{
   BDEBUG_ASSERT(mpButtonPanel!=NULL);
   mpButtonPanel->setLabelText(labelID, text);
}

//============================================================================
//============================================================================
void BUIButtonPanel::setLabelVisible(int labelID, bool bVisible)
{
   BDEBUG_ASSERT(mpButtonPanel!=NULL);
   mpButtonPanel->setLabelVisible(labelID, bVisible);
}

//============================================================================
//============================================================================
void BUIButtonPanel::setAllLabelsVisible(bool bVisible)
{
   BDEBUG_ASSERT(mpButtonPanel!=NULL);
   mpButtonPanel->setAllLabelsVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIButtonPanel::refresh()
{
   BDEBUG_ASSERT(mpButtonPanel!=NULL);
   mpButtonPanel->refresh();
}

//============================================================================
//============================================================================
void BUIButtonPanel::update(float elapsedTime)
{
   BDEBUG_ASSERT(mpButtonPanel!=NULL);
   mpButtonPanel->update();

   BDEBUG_ASSERT(mpModel!=NULL);
   mpModel->update(elapsedTime);
}

//============================================================================
//============================================================================
void BUIButtonPanel::renderFlash()
{
   BDEBUG_ASSERT(mpButtonPanel!=NULL);
   mpButtonPanel->render();
}

//============================================================================
//============================================================================
void BUIButtonPanel::render()
{
   BDEBUG_ASSERT(mpModel!=NULL);

   BMatrix matrix;
   matrix.makeIdentity();
   gUIManager->renderModel(mpModel, matrix, mpButtonPanel->getRenderTargetTexture());
}

//============================================================================
//============================================================================
void BUIButtonPanel::renderDebug(int x, int y, int width, int height)
{
   gUI.renderTexture(mpButtonPanel->getRenderTargetTexture(), x, y, x+width, y+height);
}