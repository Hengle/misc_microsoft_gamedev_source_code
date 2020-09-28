//============================================================================
// uicontext.cpp
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#include "common.h"
#include "uicontext.h"

#include "BD3D.h"
#include "render.h"
#include "renderDraw.h"

#include "ugxGeomInstanceManager.h"
#include "grannyInstanceRenderer.h"
#include "visual.h"
#include "visualmanager.h"
#include "flashhud.h"

#include "ui.h"
#include "uigame.h"
#include "usermanager.h"
#include "config.h"
#include "configsgame.h"
#include "minimap.h"
#include "user.h"
#include "player.h"
#include "scenario.h"
#include "game.h"
#include "configsgamerender.h"
#include "econfigenum.h"
#include "camera.h"
#include "world.h"
#include "flashminimap.h"
#include "uimanager.h"
#include "archiveManager.h"

//============================================================================
//============================================================================
BUIContext::BUIContext():   
   mpReticle(NULL),
   mpHUD(NULL),
   mbFirstUpdate(true),
   mbReticleVisible(true)
{   
}

//============================================================================
//============================================================================
BUIContext::~BUIContext()
{

}

//============================================================================
//============================================================================
bool BUIContext::init(BUser* pUser)
{
   SCOPEDSAMPLE(BUIContext_INIT)
   if (!pUser)
      return false;

   if(!gConfig.isDefined(cConfigFlashGameUI))
      return true;

   deinit();   
   mpParentUser = pUser;

//-- FIXING PREFIX BUG ID 1533
   const BPlayer* pPlayer = mpParentUser->getPlayer();
//--
   if (!pPlayer)
      return false;

   int civID = pPlayer->getCivID();   
   mCivID = civID;
   
   if (!initReticle(civID, gGame.isSplitScreen()))
      return false;
   
   if (!initHUD(civID, gGame.isSplitScreen()))
      return false;
      
   return true;
}

//============================================================================
//============================================================================
bool BUIContext::initReticle(int civID, bool bSplitScreen)
{
   mpReticle = new BFlashReticle();

   const BFlashUIDataNode& reticleNode = gUIGame.getFlashUIReticle(civID);
   BSimString flashFile = reticleNode.mFlashFile;
   BSimString dataFile = reticleNode.mDataFile;
   if (bSplitScreen)
   {
      if (!reticleNode.mVSplitScreenFlashFile.isEmpty())
         flashFile = reticleNode.mVSplitScreenFlashFile;

      if (!reticleNode.mVSplitScreenDataFile.isEmpty())
         dataFile = reticleNode.mVSplitScreenDataFile;
   }

   if (!mpReticle->init(BStrConv::toA(flashFile), BStrConv::toA(dataFile)))
   {
      mpReticle->deinit();
      delete mpReticle;
      mpReticle = NULL;
      return false;
   }

   return true;
}

//============================================================================
//============================================================================
bool BUIContext::initHUD(int civID, bool bSplitScreen)
{
   mpHUD = new BFlashHUD();

   const BFlashUIDataNode& hudNode = gUIGame.getFlashUIHUD(civID);
   BSimString flashFile = hudNode.mFlashFile;
   BSimString dataFile = hudNode.mDataFile;
   if (bSplitScreen)
   {
       if (!hudNode.mVSplitScreenFlashFile.isEmpty())
         flashFile = hudNode.mVSplitScreenFlashFile;

      if (!hudNode.mVSplitScreenDataFile.isEmpty())
         dataFile = hudNode.mVSplitScreenDataFile;
   }

   if (!mpHUD->init(BStrConv::toA(flashFile), BStrConv::toA(dataFile)))
   {
      mpHUD->deinit();
      delete mpHUD;
      mpHUD = NULL;
      return false;
   }

   mpHUD->setUser(mpParentUser);

   if (bSplitScreen)
   {
      long x = 0;
      long y = 0;
      long width = 640;
      long height = 720;
      mpHUD->setDimension(x, y, width, height);
   } 

   return true;
}

//============================================================================
//============================================================================
void BUIContext::deinit()
{
   if (mpReticle)
   {
      mpReticle->deinit();
      delete(mpReticle);
   }
   mpReticle = NULL;

   if (mpHUD)
   {
      mpHUD->deinit();
      delete(mpHUD);
   }
   mpHUD = NULL;
   mpParentUser = NULL;

   mbFirstUpdate = true;
}

//============================================================================
//============================================================================
void BUIContext::update(float elapsedTime)
{
#ifndef BUILD_FINAL   
   gFlashGateway.setEnableBatching(gConfig.isDefined(cConfigFlashEnableBatching));
#endif

   if (mpReticle)
      mpReticle->update(elapsedTime);

   if (mpHUD)
      mpHUD->update(elapsedTime);
}

//============================================================================
//============================================================================
void BUIContext::handlePlayerSwitch()
{
// Halwes - 9/26/2008 - Switching the player for now is used in the advanced tutorial
//#ifndef BUILD_FINAL
   //-- this codes handling of switching between civs when you switch between players   
//-- FIXING PREFIX BUG ID 1534
   const BPlayer* pPlayer = mpParentUser->getPlayer();
//--
   BDEBUG_ASSERT(pPlayer);
   int civID = pPlayer->getCivID();
   if (mCivID != civID)
   {      
      //-- cache off the temp user because deinit clobbers the pointer
      BUser* pTempUser = mpParentUser;
      deinit();
      init(pTempUser);         
   }
//#endif
}

//============================================================================
//============================================================================
void BUIContext::renderReticle()
{   
   if (mpReticle)
      mpReticle->render();
}

//============================================================================
//============================================================================
void BUIContext::renderFlash()
{
   SCOPEDSAMPLE(BUIManager_RenderFlash);
   
   if (mpHUD)
      mpHUD->render();       
}

//============================================================================
//============================================================================
void BUIContext::renderBegin()
{
   SCOPEDSAMPLE(BUIManager_RenderBegin);

   //static bool bWireframe = false;
   //gRenderDraw.setRenderState(D3DRS_FILLMODE, getFlag(eFlagWireframe) || bWireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
   //gRenderDraw.setDepthStencilSurface(BD3D::mpDevDepthStencil);

   //-- render all necessary flash components first
   renderFlash();   
}

//============================================================================
//============================================================================
void BUIContext::render()
{
   SCOPEDSAMPLE(BUIManager_Render);      
}

//============================================================================
//============================================================================
void BUIContext::renderEnd()
{
   SCOPEDSAMPLE(BUIManager_RenderEnd);      
}

//============================================================================
//============================================================================
void BUIContext::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{

}

//============================================================================
//============================================================================
void BUIContext::setHUDLabelText(int labelID, const BUString& text)
{
   if (mpHUD)
      mpHUD->setControlText(labelID, text);
}

//============================================================================
//============================================================================
void BUIContext::setHUDLabelText(int labelID, const WCHAR* pText)
{
   if (mpHUD)
      mpHUD->setControlText(labelID, pText);
}

//============================================================================
//============================================================================
void BUIContext::setHUDLabelTextVisible(int labelID, bool bVisible)
{
   if (mpHUD)
      mpHUD->setControlVisible(labelID, bVisible);
}

//============================================================================
//============================================================================
void BUIContext::setAllHUDLabelTextVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setAllControlsVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIContext::refreshHUD()
{
   if (mpHUD)
      mpHUD->refresh();
}

//============================================================================
//============================================================================
void BUIContext::setUnitStats(const BUString& name, float stat1, float stat2, float stat3, float stat4, float stat5)
{
   if (mpHUD)
      mpHUD->setUnitStats(name, stat1, stat2, stat3, stat4, stat5);
}

//============================================================================
//============================================================================
void BUIContext::setUnitStatsVisible(bool bON)
{
   if (mpHUD)
   {
      mpHUD->setUnitStatsVisible(bON);
   }
}

//============================================================================
//============================================================================
bool BUIContext::getUnitStatsVisible() const
{
   if (mpHUD)
      return mpHUD->getUnitStatsVisible();
   else
      return false;
}

//============================================================================
//============================================================================
void BUIContext::setScoresVisible(bool bON)
{
/*
   - if we redo the player/score UI in flash, we need to turn this on
   if (mpHUD)
      mpHUD->setScoresVisible(bON);
*/
}

//============================================================================
//============================================================================
void BUIContext::setPlayerScore(int playerID, const BUString& text)
{
   if (mpHUD)
      mpHUD->setPlayerScore(playerID, text);
}

//============================================================================
//============================================================================
void BUIContext::setPlayerColor(int playerID, int color)
{
   if (mpHUD)
      mpHUD->setPlayerColor(playerID, color);
}

//============================================================================
//============================================================================
void BUIContext::setButtonState(int controlID, int keyFrameID)
{
   if (mpHUD)
      mpHUD->setButtonState(controlID, keyFrameID);
}

//============================================================================
//============================================================================
void BUIContext::setDPadButtonIcon(int controlID, int iconID, int frameID)
{
   if (mpHUD)
      mpHUD->setDPadButtonIcon(controlID, iconID, frameID);
}

//============================================================================
//============================================================================
int BUIContext::getInputFunctionIconID(int inputFunctionID)
{
   if (mpHUD)
      return mpHUD->getInputFunctionIconID(inputFunctionID);

   return -1;
}

//============================================================================
//============================================================================
bool BUIContext::isOverlayVisible() const
{
   if (!mpHUD)
      return false;

   return mpHUD->isOverlayVisible();
}

//============================================================================
//============================================================================
void BUIContext::setOverlayVisible(bool bON)
{
   if (!mpHUD)
      return;

   mpHUD->setOverlayVisible(bON);
}

//============================================================================
//============================================================================
void BUIContext::displayChat(BChatMessage* pChat)
{
   if (!mpHUD)
      return;

   mpHUD->displayChat(pChat);
}

//============================================================================
//============================================================================
void BUIContext::setChatVisible(bool bVisible)
{
   if (!mpHUD)
      return;

   mpHUD->setChatVisible(false);
}

//============================================================================
//============================================================================
void BUIContext::playTechNotification(int techID)
{
   if (!mpHUD)
      return;

   mpHUD->setTechNotification(techID, true);
}

//============================================================================
//============================================================================
bool BUIContext::isPowerOverlayVisible() const
{
   if (!mpHUD)
      return false;

   return mpHUD->isPowerOverlayVisible();
}

//============================================================================
//============================================================================
void BUIContext::setPowerOverlayVisible(int powerID, bool bON)
{
   if (!mpHUD)
      return;

   mpHUD->setPowerOverlayVisible(powerID, bON);
   
   // ajl 12/11/07 - requested to not hide minimap while casting power since it makes it harder to target
   //setMinimapVisible(!bON);
   
   bool bOnOff = bON ? false : mpParentUser->getHUDItemEnabled(BUser::cHUDItemDpadHelp);   
   setDPadPanelVisible(bOnOff);

   bOnOff = bON ? false : mpParentUser->getHUDItemEnabled(BUser::cHUDItemButtonHelp);   
   setButtonPanelVisible(bOnOff);

   bOnOff = bON ? false : mpParentUser->getHUDItemEnabled(BUser::cHUDItemResources);   
   setResourcePanelVisible(bOnOff);

   bOnOff = bON ? false : mpParentUser->getHUDItemEnabled(BUser::cHUDItemTime);   
   setGameTimeVisible(bOnOff);
}

//============================================================================
//============================================================================
void BUIContext::setPowerOverlayText(int powerID, const BUString& text)
{
   if (!mpHUD)
      return;

   mpHUD->setPowerOverlayText(powerID, text);
}


//============================================================================
//============================================================================
void BUIContext::setReticleVisible(bool bVisible)
{
   if (!mpReticle)
      return;

   mpReticle->setVisible(bVisible);
}

//============================================================================
//============================================================================
bool BUIContext::getReticleVisible() const
{
   if (!mpReticle)
      return false;

   return mpReticle->getVisible();
}

//============================================================================
//============================================================================
void BUIContext::setReticleMode(int mode, uint goodAgainstRating, const BCost* pCost)
{
   if (!mpReticle)
      return;

   mpReticle->setMode(mode, goodAgainstRating, pCost);
}

//============================================================================
//============================================================================
void BUIContext::setReticleHelp(int button, int frameID)
{
   if (!gConfig.isDefined(cConfigShowReticuleHelp))
      return;

   if (!mpReticle)
      return;

   mpReticle->setReticleHelp(button, frameID);
}

//============================================================================
//============================================================================
const BReticleHelpButtonState* BUIContext::getReticleButtonState(int buttonID)
{
   if (!gConfig.isDefined(cConfigShowReticuleHelp))
      return NULL;

   if (!mpReticle)
      return NULL;

   return mpReticle->getButtonState(buttonID);
}

//============================================================================
//============================================================================
void BUIContext::updateReticleButtons()
{
   if (!gConfig.isDefined(cConfigShowReticuleHelp))
      return;

   if (!mpReticle)
      return;

   mpReticle->updateButtons();
}

//============================================================================
//============================================================================
void BUIContext::setReticlePosition(int x, int y, int width, int height)
{
   if (!mpReticle)
      return;

   mpReticle->setDimension(x,y,width,height);
}

//============================================================================
//============================================================================
void BUIContext::setUnitSelectionVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setUnitSelectionVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIContext::setUnitCardVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setUnitCardVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIContext::setUnitSelection(int slotID, int frameID, BEntityID squadID, int protoSquadID, int protoObjID, bool bTagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color)
{
   if (mpHUD)
      mpHUD->setUnitSelection(slotID, frameID, squadID, protoSquadID, protoObjID, bTagged, abilityFrameID, count, detailStr, nameStr, roleStr, bShowGamerTag, gamerTagStr, color);
}

//============================================================================
//============================================================================
void BUIContext::updateUnitSelectionDisplay(int subSelectIndex, bool bDisplayArrows)
{
   if (mpHUD)
      mpHUD->updateUnitSelectionDisplay(subSelectIndex, bDisplayArrows);
}

//============================================================================
//============================================================================
void BUIContext::setUnitCard(int slotID, int frameID, int playerID, BEntityID squadID, int protoSquadID, int protoObjID, int abilityID, bool bTagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color)
{
   if (mpHUD)
      mpHUD->setUnitCard(slotID, frameID, playerID, squadID, protoSquadID, protoObjID, abilityID, bTagged, abilityFrameID, count, detailStr, nameStr, roleStr, bShowGamerTag, gamerTagStr, color);
}

//============================================================================
//============================================================================
void BUIContext::updateUnitCardDisplay()
{
   if (mpHUD)
      mpHUD->updateUnitCardDisplay();
}

//============================================================================
//============================================================================
void BUIContext::setGameTimeVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setGameTimeVisible(bVisible);
}

//============================================================================
//============================================================================
bool BUIContext::getGameTimeVisible() const
{
   if (mpHUD)
      return mpHUD->getGameTimeVisible();
   else
      return false;
}

//============================================================================
//============================================================================
void BUIContext::setGameTime(const BUString& timeStr)
{
   if (mpHUD)
      mpHUD->setGameTime(timeStr);
}

//==============================================================================
//==============================================================================
void BUIContext::setResourcePanelVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setResourcePanelVisible(bVisible);
}

//==============================================================================
//==============================================================================
bool BUIContext::getResourcePanelVisible() const
{
   if (mpHUD)
      return mpHUD->getResourcePanelVisible();
   else
      return false;
}

//==============================================================================
//==============================================================================
void BUIContext::setPowerPanelVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setPowerPanelVisible(bVisible);
}

//==============================================================================
//==============================================================================
void BUIContext::setDPadPanelVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setDPadPanelVisible(bVisible);
}

//==============================================================================
//==============================================================================
bool BUIContext::isDPadPanelVisible() const
{
   if (mpHUD)
      return mpHUD->getDPadPanelVisible();

   return false;
}

//==============================================================================
//==============================================================================
void BUIContext::setButtonPanelVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setButtonPanelVisible(bVisible);
}

//==============================================================================
//==============================================================================
bool BUIContext::isButtonPanelVisible() const
{
   if (mpHUD)
      return mpHUD->getButtonPanelVisible();

   return false;
}

//==============================================================================
//==============================================================================
void BUIContext::showCircleMenu(int type, const WCHAR* pText)
{
   setScoresVisible(false);
   setReticleVisible(false);

   resetCircleMenuPointingAt();
   setCircleMenuBaseText(pText);

   setCircleMenuVisible(true, type);
   setButtonPanelVisible(true);
}

//==============================================================================
//==============================================================================
void BUIContext::hideCircleMenu()
{  
   setButtonPanelVisible(false);
   setCircleMenuVisible(false, -1);
   
   if (mpParentUser->getHUDItemEnabled(BUser::cHUDItemScore))
      setScoresVisible(true);

   if (mpParentUser->getHUDItemEnabled(BUser::cHUDItemReticle))
      setReticleVisible(true);
}

//============================================================================
//============================================================================
void BUIContext::resetCircleMenuPointingAt()
{
   if (mpHUD)
      mpHUD->resetCircleMenuPointingAt();
}

//============================================================================
//============================================================================
void BUIContext::setCircleMenuVisible(bool bVisible, int type)
{
   if (mpHUD)
      mpHUD->setCircleMenuVisible(bVisible, type);
}

//============================================================================
//============================================================================
void BUIContext::setCircleMenuBaseText(const WCHAR* text)
{
   if (mpHUD)
      mpHUD->setCircleMenuBaseText(text);
}

//============================================================================
//============================================================================
void BUIContext::setCircleMenuBaseText2(const WCHAR* text)
{
   if (mpHUD)
      mpHUD->setCircleMenuBaseText2(text);
}

//============================================================================
//============================================================================
void BUIContext::setCircleMenuBaseTextDetail(const WCHAR* text)
{
   if (mpHUD)
      mpHUD->setCircleMenuBaseTextDetail(text);
}

//============================================================================
//============================================================================
void BUIContext::clearCircleMenuItems()
{
   if (mpHUD)
      mpHUD->clearCircleMenuItems();
}

//============================================================================
//============================================================================
void BUIContext::clearCircleMenuDisplay()
{
   if (mpHUD)
      mpHUD->clearCircleMenuDisplay(true);
}

//============================================================================
//============================================================================
void BUIContext::clearInfoPanelStats()
{
   if (mpHUD)
      mpHUD->clearInfoPanelStats();
}

//============================================================================
//============================================================================
void BUIContext::clearInfoPanelDescription()
{
   if (mpHUD)
      mpHUD->clearInfoPanelDescription();
}

//============================================================================
//============================================================================
long BUIContext::getCircleMenuItemIndex(long id) const
{
   if (mpHUD)
      return mpHUD->getCircleMenuItemIndex(id);

   return -1;
}

//============================================================================
//============================================================================
long BUIContext::addCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerID, int unitStatProtoID)
{
   if (mpHUD)
      return mpHUD->addCircleMenuItem(order,id,pCost,pPops, trainCount, trainLimit, trainPercent, techPrereqID, unavail, label, infoText, infoText2, infoDetail, itemType, iconID, ownerID, unitStatProtoID);

   return -1;
}

//============================================================================
//============================================================================
bool BUIContext::editCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerID, int unitStatProtoID)
{
   if (mpHUD)
      return mpHUD->editCircleMenuItem(order,id,pCost,pPops, trainCount, trainLimit, trainPercent, techPrereqID, unavail, label, infoText, infoText2, infoDetail, itemType, iconID, ownerID, unitStatProtoID);
   else
      return false;
}

//============================================================================
//============================================================================
void BUIContext::removeCircleMenuItem(long index)
{
   if (mpHUD)
      mpHUD->removeCircleMenuItem(index);
}

//============================================================================
//============================================================================
long BUIContext::getNumberCircleMenuItems() const
{
   if (mpHUD)
      return mpHUD->getNumberCircleMenuItems();

   return 0;
}

//============================================================================
//============================================================================
void BUIContext::refreshCircleMenu(bool bRefreshTrainProgressOnly)
{
   if (mpHUD)
      mpHUD->refreshCircleMenu(bRefreshTrainProgressOnly);
}

//============================================================================
//============================================================================
bool BUIContext::handleCircleMenuInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   if (mpHUD)
      return mpHUD->handleInput(port, event, controlType, detail);

   return false;
}

//============================================================================
//============================================================================
int BUIContext::lookupIconID(const char* pIconType, const char* pIconName)
{
   if (mpHUD)
      return mpHUD->lookupIconID(pIconType, pIconName);
   return -1;
}

//============================================================================
//============================================================================
int BUIContext::getProtoIconCount() const
{
   if (mpHUD)
      return mpHUD->getProtoIconCount();
   return 0;
}

//============================================================================
//============================================================================
const BFlashProtoIcon* BUIContext::getProtoIcon(int index)
{
   if (mpHUD)
      return mpHUD->getProtoIcon(index);
   return 0;
}

//============================================================================
//============================================================================
void BUIContext::setCircleMenuExtraInfoVisible(bool bVisible)
{
   if (mpHUD)
      mpHUD->setCircleMenuExtraInfoVisible(bVisible);
}

//============================================================================
//============================================================================
bool BUIContext::getCircleMenuExtraInfoVisible() const
{
   if (mpHUD)
      return mpHUD->getCircleMenuExtraInfoVisible();

   return false;
}  

//============================================================================
//============================================================================
void BUIContext::flashUIElement(int element, bool flashOn)
{
   switch (element)
   {
      case BUIGame::cFlashableItemMinimap:
         {
            BFlashMinimap* pMinimap = gUIManager->getMinimap();
            if (pMinimap)
               pMinimap->setMapFlash(flashOn);
         }
         break;
      case BUIGame::cFlashableItemCircleMenuSlot0:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(1, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSlot1:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(2, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSlot2:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(3, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSlot3:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(4, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSlot4:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(5, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSlot5:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(6, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSlot6:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(7, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSlot7:
         if (mpHUD)
            mpHUD->flashCircleMenuSlot(8, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuPop:
         if (mpHUD)
            mpHUD->flashCircleMenuCenterPanel(1, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuPower:
         if (mpHUD)
            mpHUD->flashCircleMenuCenterPanel(2, flashOn);
         break;
      case BUIGame::cFlashableItemCircleMenuSupply:
         if (mpHUD)
            mpHUD->flashCircleMenuCenterPanel(3, flashOn);
         break;
      case BUIGame::cFlashableItemDpad:
         break;
      case BUIGame::cFlashableItemDpadUp:
         break;
      case BUIGame::cFlashableItemDpadDown:
         break;
      case BUIGame::cFlashableItemDpadLeft:
         break;
      case BUIGame::cFlashableItemDpadRight:
         break;
      case BUIGame::cFlashableItemResourcePanel:
         if (mpHUD)
            mpHUD->flashResourcePanel(flashOn);
         break;
      case BUIGame::cFlashableItemResourcePanelPop:
         if (mpHUD)
            mpHUD->flashResourcePanelSlot(1, flashOn);
         break;
      case BUIGame::cFlashableItemResourcePanelPower:
         if (mpHUD)
            mpHUD->flashResourcePanelSlot(2, flashOn);
         break;
      case BUIGame::cFlashableItemResourcePanelSupply:
         if (mpHUD)
            mpHUD->flashResourcePanelSlot(3, flashOn);
         break;
   }
}
