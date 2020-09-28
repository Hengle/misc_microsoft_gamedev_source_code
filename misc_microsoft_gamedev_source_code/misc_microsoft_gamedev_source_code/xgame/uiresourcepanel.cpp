//============================================================================
// uiResourcePanel.h
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#include "common.h"
#include "uiresourcepanel.h"
#include "uimanager.h"
#include "flashResourcePanel.h"
#include "visual.h"
#include "visualmanager.h"

#include "uigame.h"
#include "usermanager.h"
#include "user.h"
#include "player.h"

class BBoneLookup
{
   public:
      int      mEnum;
      char     mName[64];
};
#define BONEENUM(type, str) {BUIResourcePanel::type, str},
BBoneLookup gBoneLookup[BUIResourcePanel::eBoneTotal]=
{
   BONEENUM(eBoneMain,         "bone_main")
   BONEENUM(eBoneTopPanel,     "bone_resource_panel")
   BONEENUM(eBoneUNSCPop,      "bone_unsc_pop")
   BONEENUM(eBoneUNSCPower,    "bone_unsc_power")
   BONEENUM(eBoneUNSCSupplies, "bone_unsc_supplies")
   BONEENUM(eBoneCOVPop,       "bone_cov_pop")
   BONEENUM(eBoneCOVRelics,    "bone_cov_relics")
   BONEENUM(eBoneCOVFavor,     "bone_cov_favor")
   BONEENUM(eBoneCOVOrganics,  "bone_cov_organics")
};

class BResourceModel
{
   public:
      long mBoneIndex;
};

BResourceModel gResourceModels[BUIGame::e3DUIResourceModelTotal]=
{
   {BUIResourcePanel::eBoneUNSCPop},
   {BUIResourcePanel::eBoneUNSCPower},
   {BUIResourcePanel::eBoneUNSCSupplies},
   {BUIResourcePanel::eBoneCOVPop},
   {BUIResourcePanel::eBoneCOVRelics},
   {BUIResourcePanel::eBoneCOVFavor},
   {BUIResourcePanel::eBoneCOVOrganics},
};


//============================================================================
//============================================================================
BUIResourcePanel::BUIResourcePanel():
  mpResourcePanel(NULL),
  mpModel(NULL)
{

}

//============================================================================
//============================================================================
BUIResourcePanel::~BUIResourcePanel()
{

}

//============================================================================
//============================================================================
void BUIResourcePanel::init()
{
   mpResourcePanel = new BFlashResourcePanel();
   mpResourcePanel->init(gUIGame.get3DUIFlashMovie(BUIGame::e3DUIResourcePanel));
   mpModel = gUIGame.get3DUIPanel(BUIGame::e3DUIResourcePanel);
      
   mBoneHandle.resize(eBoneTotal);
   int handle = -1;
   for (int i = 0; i < eBoneTotal; i++)
   {
      handle = mpModel->getBoneHandle(gBoneLookup[i].mName);
      mBoneHandle[i] = handle;
   }

   mResourceModels.resize(BUIGame::e3DUIResourceModelTotal);
   for (int j = 0; j < BUIGame::e3DUIResourceModelTotal; ++j)
   {
      mResourceModels[j] = gUIGame.get3DUIResourceModel(j);
   }   
}

//============================================================================
//============================================================================
void BUIResourcePanel::deinit()
{   
   mpModel = NULL;

   for (int j = 0; j < BUIGame::e3DUIResourceModelTotal; j++)
      mResourceModels[j] = NULL;
   mResourceModels.resize(0);

   if (mpResourcePanel)
   {
      mpResourcePanel->deinit();
      delete mpResourcePanel;
   }
   mpResourcePanel = NULL;
}

//============================================================================
//============================================================================
void BUIResourcePanel::update(float elapsedTime)
{
   BDEBUG_ASSERT(mpResourcePanel!=NULL);
   mpResourcePanel->update();

   BDEBUG_ASSERT(mpModel!=NULL);
   mpModel->update(elapsedTime);

   BVector pos;
   BMatrix matrix;
   for (int j = 0; j < BUIGame::e3DUIResourceModelTotal; j++)
   {
      BDEBUG_ASSERT(mResourceModels[j]!=NULL);
      mResourceModels[j]->update(elapsedTime);

      mpModel->getBone(mBoneHandle[gResourceModels[j].mBoneIndex], &pos, &matrix);
      mResourceModels[j]->updateWorldMatrix(matrix, NULL);
   }
}

//============================================================================
//============================================================================
void BUIResourcePanel::renderFlash()
{
   BDEBUG_ASSERT(mpResourcePanel!=NULL);
   mpResourcePanel->render();
}

//============================================================================
//============================================================================
void BUIResourcePanel::render()
{
   BDEBUG_ASSERT(mpModel!=NULL);

   BMatrix matrix;
   matrix.makeIdentity();
   gUIManager->renderModel(mpModel, matrix, mpResourcePanel->getRenderTargetTexture());

   BUser* pUser = gUserManager.getUser(0);
   BDEBUG_ASSERT(pUser!=NULL);

   BPlayer* pPlayer = pUser->getPlayer();
   BDEBUG_ASSERT(pPlayer!=NULL);
   long civID=pPlayer->getCivID();

   static bool bRenderResourceModels = true;
   BVector pos;
   if (bRenderResourceModels)
   {
      //-- civ ID == UNSC
      if (civID == 1)
      {
         mpModel->getBone(mBoneHandle[eBoneUNSCPop], &pos);
         matrix.setTranslation(pos);
         BDEBUG_ASSERT(mResourceModels[BUIGame::e3DUIResourceModelUNSCPop]!=NULL);
         gUIManager->renderModel(mResourceModels[BUIGame::e3DUIResourceModelUNSCPop], matrix, cInvalidTextureHandle);

         mpModel->getBone(mBoneHandle[eBoneUNSCPower], &pos);
         matrix.setTranslation(pos);
         BDEBUG_ASSERT(mResourceModels[BUIGame::e3DUIResourceModelUNSCPower]!=NULL);
         gUIManager->renderModel(mResourceModels[BUIGame::e3DUIResourceModelUNSCPower], matrix, cInvalidTextureHandle);

         mpModel->getBone(mBoneHandle[eBoneUNSCSupplies], &pos);
         matrix.setTranslation(pos);
         BDEBUG_ASSERT(mResourceModels[BUIGame::e3DUIResourceModelUNSCSupplies]!=NULL);
         gUIManager->renderModel(mResourceModels[BUIGame::e3DUIResourceModelUNSCSupplies], matrix, cInvalidTextureHandle);
      }
      else if (civID == 2)
      {
         mpModel->getBone(mBoneHandle[eBoneCOVPop], &pos);
         matrix.setTranslation(pos);
         BDEBUG_ASSERT(mResourceModels[BUIGame::e3DUIResourceModelCOVPop]!=NULL);
         gUIManager->renderModel(mResourceModels[BUIGame::e3DUIResourceModelCOVPop], matrix, cInvalidTextureHandle);

         mpModel->getBone(mBoneHandle[eBoneCOVRelics], &pos);
         matrix.setTranslation(pos);
         BDEBUG_ASSERT(mResourceModels[BUIGame::e3DUIResourceModelCOVRelics]!=NULL);
         gUIManager->renderModel(mResourceModels[BUIGame::e3DUIResourceModelCOVRelics], matrix, cInvalidTextureHandle);

         mpModel->getBone(mBoneHandle[eBoneCOVFavor], &pos);
         matrix.setTranslation(pos);
         BDEBUG_ASSERT(mResourceModels[BUIGame::e3DUIResourceModelCOVFavor]!=NULL);
         gUIManager->renderModel(mResourceModels[BUIGame::e3DUIResourceModelCOVFavor], matrix, cInvalidTextureHandle);

         mpModel->getBone(mBoneHandle[eBoneCOVOrganics], &pos);
         matrix.setTranslation(pos);
         BDEBUG_ASSERT(mResourceModels[BUIGame::e3DUIResourceModelCOVOrganics]!=NULL);
         gUIManager->renderModel(mResourceModels[BUIGame::e3DUIResourceModelCOVOrganics], matrix, cInvalidTextureHandle);
      }
   }
}

