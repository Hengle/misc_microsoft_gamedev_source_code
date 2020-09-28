//============================================================================
// uiminimappanel.cpp
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#include "common.h"
#include "uiminimappanel.h"
#include "uimanager.h"
#include "visual.h"
#include "visualmanager.h"
#include "uigame.h"

class BMiniMapBoneLookup
{
public:
   int      mEnum;
   char     mName[64];
};
#define BONEENUM(type, str) {BUIMinimapPanel::type, str},
BMiniMapBoneLookup gMinimapBoneLookup[BUIMinimapPanel::eBoneTotal]=
{
   BONEENUM(eBoneMain,         "bone_main")
};

//============================================================================
//============================================================================
BUIMinimapPanel::BUIMinimapPanel():
   mpModel(NULL)
{

}

//============================================================================
//============================================================================
BUIMinimapPanel::~BUIMinimapPanel()
{

}

//============================================================================
//============================================================================
void BUIMinimapPanel::init()
{
   mpModel = gUIGame.get3DUIPanel(BUIGame::e3DUIMinimapPanel);

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
void BUIMinimapPanel::deinit()
{
}

//============================================================================
//============================================================================
void BUIMinimapPanel::update(float elapsedTime)
{
   BDEBUG_ASSERT(mpModel!=NULL);
   mpModel->update(elapsedTime);
}

//============================================================================
//============================================================================
void BUIMinimapPanel::renderFlash()
{
}

//============================================================================
//============================================================================
void BUIMinimapPanel::render()
{
   BDEBUG_ASSERT(mpModel!=NULL);

   BMatrix matrix;
   matrix.makeIdentity();
   gUIManager->renderModel(mpModel, matrix, cInvalidTextureHandle);
}

