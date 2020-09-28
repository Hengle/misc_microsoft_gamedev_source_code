//============================================================================
// uiunitpanel.cpp
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#include "common.h"
#include "uiunitpanel.h"
#include "uimanager.h"
#include "visual.h"
#include "visualmanager.h"
#include "uigame.h"

class BUnitBoneLookup
{
public:
   int      mEnum;
   char     mName[64];
};
#define BONEENUM(type, str) {BUIUnitPanel::type, str},
BUnitBoneLookup gMinimapBoneLookup[BUIUnitPanel::eBoneTotal]=
{
   BONEENUM(eBoneMain,         "bone_main")
};

//============================================================================
//============================================================================
BUIUnitPanel::BUIUnitPanel():
   mpModel(NULL)
{

}

//============================================================================
//============================================================================
BUIUnitPanel::~BUIUnitPanel()
{

}

//============================================================================
//============================================================================
void BUIUnitPanel::init()
{
   mpModel = gUIGame.get3DUIPanel(BUIGame::e3DUIUnitPanel);

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
void BUIUnitPanel::deinit()
{
}

//============================================================================
//============================================================================
void BUIUnitPanel::update(float elapsedTime)
{
   BDEBUG_ASSERT(mpModel!=NULL);
   mpModel->update(elapsedTime);
}

//============================================================================
//============================================================================
void BUIUnitPanel::renderFlash()
{
}

//============================================================================
//============================================================================
void BUIUnitPanel::render()
{
   BDEBUG_ASSERT(mpModel!=NULL);

   BMatrix matrix;
   matrix.makeIdentity();
   gUIManager->renderModel(mpModel, matrix, cInvalidTextureHandle);
}