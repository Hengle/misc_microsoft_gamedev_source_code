//============================================================================
// uimanager.cpp
// 
// Copyright (c) 2006 Ensemble Studios
//============================================================================

#include "common.h"
#include "uimanager.h"

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
#include "gamesettings.h"
#include "UICampaignPostGameScreen.h"
#include "UISkirmishPostgameScreen.h"
#include "UIGameMenu.h"
#include "archiveManager.h"
#include "modemanager.h"
#include "modegame.h"
#include "LiveSystem.h"
#include "visiblemap.h"
#include "skullmanager.h"
#include "campaignmanager.h"
#include "timermanager.h"
#include "transitionManager.h"

GFIMPLEMENTVERSION(BUIManager, 4);

BUIManager* gUIManager = NULL;

//============================================================================
//============================================================================
BUIManager::BUIManager():   
   mGameType(-1),
   mpUIHints(NULL),
   mpUIWidgets(NULL),
   mpUICallouts(NULL),
   mpMinimap(NULL),
   mpInfoDialog(NULL),
   mbFirstUpdate(true),
   mbMinimapVisible(true),
   mbReticleVisible(true),
   mbEnableDecals(true),
   mbGamePaused(false),
   mbUnpauseOnHideGameMenu(false),
   mpDecal(NULL),
   mCivID(-1),
   mpCurrentScreen(NULL),
   mpCurrentUser(NULL),
   mScenarioResult(-1)
{   
   for( int i = 0; i < cMaxUIScreens; ++i )
      mScreens[i] = NULL;
}

//============================================================================
//============================================================================
BUIManager::~BUIManager()
{
}

//============================================================================
//============================================================================
bool BUIManager::init()
{
   SCOPEDSAMPLE(BUIManager_init)
   deinit();   
 
   mpUIHints = new BUIHints();
   const BFlashUIDataNode& hintsNode = gUIGame.getFlashUIHints();
   BFATAL_ASSERTM(mpUIHints->init(BStrConv::toA(hintsNode.mFlashFile), NULL), "UIManager::init() -- Flash UI Hints Init Failed!");

   // initialize the widgets
   mpUIWidgets = new BUIWidgets();
   const BFlashUIDataNode& widgetsNode = gUIGame.getFlashUIWidgets();
   BFATAL_ASSERTM(mpUIWidgets->init(BStrConv::toA(widgetsNode.mFlashFile), BStrConv::toA(widgetsNode.mDataFile)), "UIManager::init() -- Flash Widgets UI Init Failed!");

   // initialize the widgets
   mpUICallouts = new BUICallouts();
   const BFlashUIDataNode& calloutsNode = gUIGame.getFlashUICallouts();
   BFATAL_ASSERTM(mpUICallouts->init(BStrConv::toA(calloutsNode.mFlashFile), BStrConv::toA(calloutsNode.mDataFile)), "UIManager::init() -- Flash Callouts UI Init Failed!");

   // Init Non-Game UI Screens
//-- FIXING PREFIX BUG ID 1604
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   if(pSettings)
      pSettings->getLong(BGameSettings::cGameType, mGameType);

   if (  mGameType != BGameSettings::cGameTypeSkirmish )
   {
      // CAMPAIGN OBJECTIVES
      mScreens[cObjectivesScreen] = new BUICampaignObjectives();
      mScreens[cObjectivesScreen]->init("art\\ui\\flash\\hud\\hud_cpn_objective\\hud_cpn_objectiveScreen.gfx", "art\\ui\\flash\\hud\\hud_cpn_objective\\hud_cpn_objectiveScreenData.xml");

      // CAMPAIGN POST-GAME
      mScreens[cPostGameStatsScreen] = new BUICampaignPostGameScreen();
      mScreens[cPostGameStatsScreen]->init("art\\ui\\flash\\postgame\\CampaignPostgame\\CampaignPostgameScreen.gfx", "art\\ui\\flash\\postgame\\CampaignPostgame\\CampaignPostgameScreenData.xml");

      // CAMPAIGN INFO-DIALOG
      mScreens[cCampaignInfoDialog] = new BUIInfoDialog();
      mScreens[cCampaignInfoDialog]->init("art\\ui\\flash\\controls\\InfoDialog\\UIInfoDialog.gfx", "art\\ui\\flash\\controls\\InfoDialog\\UIInfoDialog.xml");
                                          
   }
   else if (mGameType==BGameSettings::cGameTypeSkirmish)
   {
      // SKIRMISH OBJECTIVES
      mScreens[cObjectivesScreen] = new BUISkirmishObjectives();
      mScreens[cObjectivesScreen]->init("art\\ui\\flash\\hud\\hud_skirm_objective\\hud_skirm_objectiveScreen.gfx", "art\\ui\\flash\\hud\\hud_skirm_objective\\hud_skirm_objectiveScreenData.xml");

      // SKIRMISH POST-GAME
      mScreens[cPostGameStatsScreen]=new BUISkirmishPostGameScreen();
      mScreens[cPostGameStatsScreen]->init("art\\ui\\flash\\postgame\\skirmishPostgame\\SkirmishPostgameScreen.gfx", "art\\ui\\flash\\postgame\\SkirmishPostgame\\SkirmishPostgameScreenData.xml");
   }

   // GAME MENU
   mScreens[cGameMenu] = new BUIGameMenu();
   mScreens[cGameMenu]->initScreen( "art\\ui\\flash\\hud\\pause\\UIPauseScreen.gfx", cFlashAssetCategoryInGame, "art\\ui\\flash\\hud\\pause\\UIPauseScreen.xml" );
      
   // END GAME
   mScreens[cEndGameScreen] = new BUIEndGame();
   mScreens[cEndGameScreen]->initScreen( "art\\ui\\flash\\postgame\\endgame\\UIEndGame.gfx", cFlashAssetCategoryInGame, "art\\ui\\flash\\postgame\\endgame\\UIEndGame.xml" );

   for( int i = 0; i < cMaxUIScreens; ++i )
   {
      if( mScreens[i] )
         mScreens[i]->setHandler( this );
   }
   return true;
}

//============================================================================
//============================================================================
bool BUIManager::initPlayerSpecific( bool bForceMinimapVisible /*=false*/ )
{
   BUser* pUser = gUserManager.getPrimaryUser();
   BDEBUG_ASSERT(pUser);
//-- FIXING PREFIX BUG ID 1605
   const BPlayer* pPlayer = pUser->getPlayer();
//--
   BDEBUG_ASSERT(pPlayer);
   int civID = pPlayer->getCivID();
   mCivID = civID;

   if(gConfig.isDefined(cConfigFlashGameUI))          
   {     
      if (!initDecals(civID))
         return false;
      
      mpMinimap = new BFlashMinimap();
      const BFlashUIDataNode& minimapNode = gUIGame.getFlashUIMinimap(civID);
      if (!mpMinimap->init(BStrConv::toA(minimapNode.mFlashFile), BStrConv::toA(minimapNode.mDataFile)))
         return false;

      mpMinimap->setFullZoomOut( !pUser->getOption_MiniMapZoomEnabled() );
      setMinimapMapTexture(gScenario.getMinimapTextureHandle());
      setMinimapSkirtMirroring(true);
   }
   
   mFlags.setAll(0);
   
   bool bMinimapVisible = bForceMinimapVisible || (mGameType != BGameSettings::cGameTypeCampaign);

   if( !bMinimapVisible && (mGameType == BGameSettings::cGameTypeCampaign) )
   {
      BCampaign* pCampaign = gCampaignManager.getCampaign( 0 );
      BASSERT( pCampaign );

      BGameSettings* pSettings = gDatabase.getGameSettings();
      BASSERT( pSettings );
      
      BString mapName;
      pSettings->getString( BGameSettings::cMapName, mapName );

      BCampaignNode* pNode = pCampaign->getNode( mapName.getPtr() );
      BASSERT( pNode );

      if( pNode )
         bMinimapVisible = pNode->getFlag( BCampaignNode::cNoInitialWow );
   }
   
   //if (!gConfig.isDefined(cConfigFlashGameUI))
   //   gMiniMap.setVisible(true);

   if (mpMinimap)
      mpMinimap->setVisible( bMinimapVisible );      

   setResourcePanelVisible( bMinimapVisible );

   return true;
}

//============================================================================
//============================================================================
bool BUIManager::initAfterLoad()
{
   if(gConfig.isDefined(cConfigFlashGameUI))          
   {     
      if (!mpMinimap->initAfterLoad())
         return false;
   }

   return true;
}

//============================================================================
//============================================================================
bool BUIManager::initDecals(int civID)
{
   const BFlashUIDataNode& decalNode = gUIGame.getFlashUIDecals(civID);

   mpDecal = new BFlashDecal();
   if (!mpDecal->init(NULL /*BStrConv::toA(decalNode.mFlashFile)*/, BStrConv::toA(decalNode.mDataFile)))
      return false;
   
   return true;
}

//============================================================================
//============================================================================
void BUIManager::deinitDecals()
{
   if (mpDecal)
   {
      mpDecal->deinit();
      delete mpDecal;
   }
   mpDecal=NULL;
}

//============================================================================
//============================================================================
void BUIManager::workerRenderDecals()
{
   ASSERT_RENDER_THREAD

   if (mpDecal)
      mpDecal->workerRender();
}

//============================================================================
//============================================================================
void BUIManager::workerRenderMinimapIconResources()
{
   ASSERT_RENDER_THREAD

   if (mpMinimap)
      mpMinimap->workerRenderIconAnimations();
}

//============================================================================
//============================================================================
void BUIManager::deinit()
{   
   if (mpUIHints)
   {
      mpUIHints->deinit();
      delete mpUIHints;
   }
   mpUIHints = NULL;

   if (mpUIWidgets)
   {
      mpUIWidgets->deinit();
      delete mpUIWidgets;
   }
   mpUIWidgets = NULL;

   if (mpUICallouts)
   {
      mpUICallouts->deinit();
      delete mpUICallouts;
   }
   mpUICallouts = NULL;
   
   // De-init Non-Game UI Screens
   for( int i = 0; i < cMaxUIScreens; ++i )
   {
      if( mScreens[i] )
      {
         mScreens[i]->deinit();
         delete mScreens[i];
         mScreens[i] = NULL;
      }
   }

   deinitPlayerSpecific();
   
   mbFirstUpdate = true;

   //-- sanity check for ui contexts
   for (uint i=0; i < mUserContexts.getSize(); i++)
   {
      BDEBUG_ASSERT(mUserContexts[i]==NULL);
      if (mUserContexts[i])
      {
         gConsoleOutput.warning("WARNING:  User UI Context %d was not properly released from the user", i);
         
         delete mUserContexts[i];
         mUserContexts[i] = NULL;
         mUserContexts.removeIndex(i);
         i--;
      }
   }      
}

//============================================================================
//============================================================================
void BUIManager::deinitPlayerSpecific()
{
   if (mpMinimap)
   {
      mpMinimap->deinit();
      delete(mpMinimap);
   }
   mpMinimap=NULL;

   deinitDecals();
}

//============================================================================
//============================================================================
BUIContext* BUIManager::getOrCreateContext(BUser* pUser)
{
   if (!pUser)
      return NULL;

   BUIContext* pNewContext = new BUIContext();
   if (!pNewContext)
      return NULL;

   if (!pNewContext->init(pUser))
   {
      delete pNewContext;
      return NULL;
   }

   mUserContexts.add(pNewContext);
   return pNewContext;
}

//============================================================================
//============================================================================
void BUIManager::releaseContext(BUIContext* pContext)
{
   if (pContext == NULL)
      return;

   for (uint i = 0; i < mUserContexts.getSize(); ++i)
   {
      if (mUserContexts[i] && (pContext == mUserContexts[i]))
      {
         mUserContexts[i]->deinit();
         delete mUserContexts[i];
         mUserContexts[i] = NULL;
         mUserContexts.removeIndex(i);
         break;
      }
   }
}

//============================================================================
//============================================================================
void BUIManager::update(float elapsedTime)
{
#ifndef BUILD_FINAL   
   gFlashGateway.setEnableBatching(gConfig.isDefined(cConfigFlashEnableBatching));
#endif
   
   if (mpUIWidgets)
      mpUIWidgets->update(elapsedTime);

   if (mpUICallouts)
      mpUICallouts->update(elapsedTime);

   if (mpMinimap && mbMinimapVisible)
      mpMinimap->update(elapsedTime);   

   bool bPaused = gModeManager.getModeGame()->getPaused();

   if( mpCurrentScreen && !gWorld->getCinematicManager()->isPlayingCinematic() && gWorld->getTransitionManager()->getTransitionComplete() )
      mpCurrentScreen->update(elapsedTime);
   else if( bPaused != mbGamePaused )
   {
      mbGamePaused = bPaused;
      if( bPaused )
      {
         #ifndef BUILD_FINAL
         if(!gConfig.isDefined( "noGameMenu" ))
         #endif
            showPauseDialog();
      }
      else
      {
         hidePauseDialog();
      }
   }
}

//============================================================================
//============================================================================
void BUIManager::handlePlayerSwitch()
{   
// Halwes - 9/26/2008 - Switching the player for now is used in the advanced tutorial
//#ifndef BUILD_FINAL
   BUser* pUser = gUserManager.getPrimaryUser();
   BDEBUG_ASSERT(pUser);
//-- FIXING PREFIX BUG ID 1606
   const BPlayer* pPlayer = pUser->getPlayer();
//--
   BDEBUG_ASSERT(pPlayer);
   int civID = pPlayer->getCivID();
   if (mCivID != civID)
   {      
      deinitPlayerSpecific();
      initPlayerSpecific( true );       
   }
//#endif
}

//============================================================================
//============================================================================
void BUIManager::updateRenderThread(double gameTime)
{
   ASSERT_THREAD(cThreadIndexSim);

   if (mpDecal && mbEnableDecals)
      mpDecal->updateRenderThread();
}

//============================================================================
//============================================================================
void BUIManager::renderCampaignObjectives()
{
   if (mScreens[cObjectivesScreen]==NULL)
      return;
   mScreens[cObjectivesScreen]->render();
}

//============================================================================
//============================================================================
void BUIManager::refreshUI()
{
   if( mpCurrentScreen )
      mpCurrentScreen->refreshScreen();
}


//============================================================================
//============================================================================
void BUIManager::renderSkirmishObjectives()
{
   if (mScreens[cObjectivesScreen]==NULL)
      return;

   mScreens[cObjectivesScreen]->render();
}

//============================================================================
//============================================================================
void BUIManager::releaseGPUHeapResources()
{
   releaseDecalGPUHeapTextures();
   releaseMinimapGPUHeapTextures();
}

//============================================================================
//============================================================================
void BUIManager::renderUser()
{
   SCOPEDSAMPLE(BUIManager_RenderUser);

   static bool bWireframe = false;
   gRenderDraw.setRenderState(D3DRS_FILLMODE, getFlag(eFlagWireframe) || bWireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
   gRenderDraw.setDepthStencilSurface(BD3D::mpDevDepthStencil);

   //-- render all necessary flash components first
   if( !isNonGameUIVisible() )
   {
      if (mpUICallouts)
         mpUICallouts->render();

      if (mpUIHints)
         mpUIHints->render();      
   }

   gRender2DPrimitiveUtility.update(BRender2DPrimitiveUtility::eBR2DUUpdatePie, mPiePrimitives.getSize(), mPiePrimitives.getPtr());
   gRender2DPrimitiveUtility.update(BRender2DPrimitiveUtility::eBR2DUUpdateSprite, mSpritePrimitives.getSize(), mSpritePrimitives.getPtr());
   gRender2DPrimitiveUtility.render();
}

//============================================================================
//============================================================================
void BUIManager::renderMinimap()
{
   SCOPEDSAMPLE(BUIManager_RenderMinimap);
   if (mpMinimap && mbMinimapVisible)
      mpMinimap->render();
}

//============================================================================
//============================================================================
void BUIManager::renderWidgets()
{
   SCOPEDSAMPLE(BUIManager_RenderWidgets);
   if (mpUIWidgets)
      mpUIWidgets->render();      
}

//============================================================================
//============================================================================
void BUIManager::render2DPrimitives()
{
   gRender2DPrimitiveUtility.render();
}

//============================================================================
//============================================================================
void BUIManager::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{

}

//============================================================================
//============================================================================
void BUIManager::setWidgetsVisible(bool bVisible)
{
   if (mpUIWidgets)
      mpUIWidgets->setWidgetsVisible(bVisible);
}

//============================================================================
//============================================================================
bool BUIManager::isWidgetsVisible() const 
{
   if (!mpUIWidgets)
      return false;

   return mpUIWidgets->isWidgetsVisible();
}

//============================================================================
//============================================================================
void BUIManager::setCalloutsVisible(bool bVisible)
{
   if (!mpUICallouts)
      return;

   return mpUICallouts->setCalloutsVisible(bVisible);
}

//============================================================================
//============================================================================
bool BUIManager::isCalloutsVisible() const 
{
   if (!mpUICallouts)
      return false;

   return mpUICallouts->isCalloutsVisible();
}

//-- Hints
//============================================================================
//============================================================================
void BUIManager::displayHint(BHintMessage* pHint)
{
   if (!mpUIHints)
      return;

   mpUIHints->displayHint(pHint);
}

//============================================================================
//============================================================================
void BUIManager::hideHint()
{
   if (!mpUIHints)
      return;

   mpUIHints->hideHint();
}

//============================================================================
//============================================================================
void BUIManager::setHintsVisible(bool bVisible)
{
   if (!mpUIHints)
      return;

   return mpUIHints->setHintsVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIManager::setResourcePanelVisible(bool bVisible)
{
   gUserManager.getPrimaryUser()->getUIContext()->setResourcePanelVisible(bVisible);

   if (gGame.isSplitScreen())
      gUserManager.getSecondaryUser()->getUIContext()->setResourcePanelVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIManager::setReticleVisible(bool bVisible)
{
   gUserManager.getPrimaryUser()->getUIContext()->setReticleVisible(bVisible);

   if (gGame.isSplitScreen())
      gUserManager.getSecondaryUser()->getUIContext()->setReticleVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIManager::setDPadPanelVisible(bool bVisible)
{
   gUserManager.getPrimaryUser()->getUIContext()->setDPadPanelVisible(bVisible);

   if (gGame.isSplitScreen())
      gUserManager.getSecondaryUser()->getUIContext()->setDPadPanelVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIManager::setButtonPanelVisible(bool bVisible)
{
   gUserManager.getPrimaryUser()->getUIContext()->setButtonPanelVisible(bVisible);

   if (gGame.isSplitScreen())
      gUserManager.getSecondaryUser()->getUIContext()->setButtonPanelVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIManager::setGameTimeVisible(bool bVisible)
{
   gUserManager.getPrimaryUser()->getUIContext()->setGameTimeVisible(bVisible);

   if (gGame.isSplitScreen())
      gUserManager.getSecondaryUser()->getUIContext()->setGameTimeVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIManager::setScoresVisible(bool bVisible)
{
   gUserManager.getPrimaryUser()->getUIContext()->setScoresVisible(bVisible);

   if (gGame.isSplitScreen())
      gUserManager.getSecondaryUser()->getUIContext()->setScoresVisible(bVisible);
}


//============================================================================
//============================================================================
BBinkVideoHandle BUIManager::displayChat(BChatMessage* pChat, BBinkVideoStatus* pStatusCallback)
{
   BBinkVideoHandle handle = cInvalidVideoHandle;
   if (mpUIWidgets)
   {
      BUITalkingHeadControl& thc = mpUIWidgets->getTalkingHead();

      if (gWorld->isPlayingCinematic())
         thc.setShowBackground(true);
      else
         thc.setShowBackground(false);

      bool showChatText=true;
      // check to see if subtitles are turned on.
      if (gUserManager.getPrimaryUser())
         showChatText=gUserManager.getPrimaryUser()->getOption_ChatTextEnabled();

      if (showChatText || pChat->getForceSubtitles())
      {
         //mpUIWidgets->showTalkingHeadText(pChat->getChatString());
         thc.showTalkingHeadText(pChat->getChatString());
      }

      // show the talking head
      int speaker = pChat->getSpeakerID();   // speaker ID is actually the talking head id now.
      
      BCueIndex soundIndex = gSoundManager.getCueIndex(pChat->getSoundString());

      handle = gWorld->playTalkingHead(speaker, soundIndex, pStatusCallback);      // if there is none, this should fail gracefully
     
      //-- Only play the chat if there is no video
      if (handle == cInvalidVideoHandle) 
      {
         BCueHandle handle = gSoundManager.playCue(pChat->getSoundString(), cInvalidWwiseObjectID, pChat->getQueueSound());
         if(handle == cInvalidCueHandle)
            pChat->setCueIndex(cInvalidCueIndex); //-- The chat failed, mark the sound as complete.
      }
      
      if( handle != cInvalidVideoHandle )
         thc.showTalkingHead(handle); 
   }
   return handle;
}

//============================================================================
//============================================================================
void BUIManager::setChatVisible(bool bVisible)
{
   if (mpUIWidgets && !bVisible )
   {
      BUITalkingHeadControl& thc = mpUIWidgets->getTalkingHead();
      
      if( thc.getVideoHandle() != cInvalidVideoHandle )
         thc.hideTalkingHead();

      // Don't try to be so clever. Just turn the damn thing off.
      thc.hideTalkingHeadText();
   }
}

//============================================================================
//============================================================================
void BUIManager::showIngameObjective(bool bShow, long objectiveID)
{
   if (!mpUIWidgets)
      return;

   BUITalkingHeadControl& thc = mpUIWidgets->getTalkingHead();

   if (bShow)
   {
      // show the text
      thc.showObjectiveText(objectiveID);
   }
   else
   {
      hideIngameObjectives( true );
   }
}

//============================================================================
//============================================================================
void BUIManager::refreshObjective(long objectiveID)
{
   if (!mpUIWidgets)
      return;

   BUITalkingHeadControl& thc = mpUIWidgets->getTalkingHead();

   thc.showObjectiveText(objectiveID);
}

//============================================================================
//============================================================================
void BUIManager::hideIngameObjectives(bool easeOut)
{
   if (!mpUIWidgets)
      return;

   BUITalkingHeadControl& thc = mpUIWidgets->getTalkingHead();
   mpUIWidgets->getObjectiveTracker().trackObjective( gWorld->getObjectiveManager()->getObjective( gWorld->getObjectiveManager()->getIndexFromID(thc.getObjectiveID()) ) );
   thc.hideObjectiveText(easeOut);
}

//============================================================================
//============================================================================
void BUIManager::setupCinematicUI(bool playCinematic)
{
//    BUser* pPrimaryUser = gUserManager.getPrimaryUser();
//    BUIContext* pPrimaryUIContext = pPrimaryUser->getUIContext();
// 
//    BUser* pSecondaryUser = gUserManager.getSecondaryUser();
//    BUIContext* pSecondaryUIContext = (gGame.isSplitScreen() ? pSecondaryUser->getUIContext() : NULL);

   if (playCinematic)
   {
      hideInGameUI();
      /*
      if (gConfig.isDefined(cConfigWowRecord) || (gModeManager.getModeType() == BModeManager::cModeCinematic))
      {
         setChatVisible(false);
         setWidgetsVisible(false);
      }
      setHintsVisible(false);   
      setMinimapVisible(false);
      setScoresVisible(false);
      setReticleVisible(false);
      setResourcePanelVisible(false);
      setDPadPanelVisible(false);      
      setButtonPanelVisible(false);
      setGameTimeVisible(false);

      pPrimaryUIContext->setUnitStatsVisible(false);
      if (gGame.isSplitScreen())
         pSecondaryUIContext->setUnitStatsVisible(false);
      */
   }
   else if( !isNonGameUIVisible() )
   {
      restoreInGameUI();
      /*
      if (gConfig.isDefined(cConfigWowRecord) || (gModeManager.getModeType() == BModeManager::cModeCinematic))
      {
         setChatVisible(true);
         setWidgetsVisible(true);
      }
      setHintsVisible(true);   

      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemMinimap))
         setMinimapVisible(true);
      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemScore))
         pPrimaryUIContext->setScoresVisible(true);
      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemReticle))
         pPrimaryUIContext->setReticleVisible(true);
      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemResources))
         pPrimaryUIContext->setResourcePanelVisible(true);
      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemDpadHelp))
         pPrimaryUIContext->setDPadPanelVisible(true);
      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemButtonHelp))
         pPrimaryUIContext->setButtonPanelVisible(false);
      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemTime))
         pPrimaryUIContext->setGameTimeVisible(true);
      if (pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemUnitStats))
         pPrimaryUIContext->setUnitStatsVisible(true);

      if (gGame.isSplitScreen())
      {
         if (pSecondaryUser->getHUDItemEnabled(BUser::cHUDItemScore))
            pSecondaryUIContext->setScoresVisible(true);
         if (pSecondaryUser->getHUDItemEnabled(BUser::cHUDItemReticle))
            pSecondaryUIContext->setReticleVisible(true);
         if (pSecondaryUser->getHUDItemEnabled(BUser::cHUDItemResources))
            pSecondaryUIContext->setResourcePanelVisible(true);
         if (pSecondaryUser->getHUDItemEnabled(BUser::cHUDItemDpadHelp))
            pSecondaryUIContext->setDPadPanelVisible(true);
         if (pSecondaryUser->getHUDItemEnabled(BUser::cHUDItemButtonHelp))
            pSecondaryUIContext->setButtonPanelVisible(false);
         if (pSecondaryUser->getHUDItemEnabled(BUser::cHUDItemTime))
            pSecondaryUIContext->setGameTimeVisible(true);
         if (pSecondaryUser->getHUDItemEnabled(BUser::cHUDItemUnitStats))
            pSecondaryUIContext->setUnitStatsVisible(true);
      }
      */
   }
}

//============================================================================
//============================================================================
// void BUIManager::setPaused(bool paused, PlayerID playerID)
// {
// #ifndef BUILD_FINAL
//    // skip displaying the pause screen or pause dialog when we define StartPauseButton
//    if (gConfig.isDefined(cConfigStartPauseButton))
//       return;
// #endif
//    if (mScreens[cGameMenu] == NULL)
//       return;
// 
//    //mScreens[cGameMenu]->setPaused(paused);
// 
//    mbGamePaused = paused;
// 
//    showPauseDialog();
// }

//============================================================================
//============================================================================
int BUIManager::getDecalFlashMovieIndex(int decalID) const
{
   if (!mpDecal)
      return -1;

   return mpDecal->getMovieInstanceIndex(decalID);
}

//============================================================================
//============================================================================
BManagedTextureHandle BUIManager::getDecalTextureHandle(BFlashPropertyHandle handle) const
{
   if (!mpDecal)
      return cInvalidManagedTextureHandle;

   return mpDecal->getDecalTextureHandle(handle);
}

//============================================================================
//============================================================================
void BUIManager::getDecalTextureUV(int decalType, XMHALF4& uv, XMHALF4& uv2)
{
   if (mpDecal)
      mpDecal->getDecalUV(decalType, uv, uv2);
}

//============================================================================
//============================================================================
bool BUIManager::getDecalSize(int protoID, float& sizeX, float& sizeY, float& sizeZ)
{
   if (mpDecal)
      return mpDecal->getDecalSize(protoID, sizeX, sizeY, sizeZ);

   return false;
}

//============================================================================
//============================================================================
int BUIManager::getDecalProtoIconIndex(int protoID)
{
   if (mpDecal)
      return mpDecal->getProtoIconIndex(protoID);

   return -1;
}

//============================================================================
//============================================================================
const BFlashProtoIcon* BUIManager::getDecalProtoIcon(int index)
{
   if (mpDecal)
      return mpDecal->getProtoIcon(index);

   return NULL;
}

//============================================================================
//============================================================================
void BUIManager::releaseDecalGPUHeapTextures()
{
   if (mpDecal)
      mpDecal->relaseGPUHeapTextures();
}

//============================================================================
//============================================================================
void BUIManager::releaseMinimapGPUHeapTextures()
{
   if (mpMinimap)
      mpMinimap->releaseGPUHeapTextures();
}

//============================================================================
//============================================================================
void BUIManager::setMinimapVisible(bool bVisible)
{
   if (bVisible == mbMinimapVisible)
      return;

   //Don't turn on if we are using a skull to disable the minimap
   if (bVisible && gCollectiblesManager.isMinimapHidden())
      return; 

   mbMinimapVisible=bVisible;

   //if (!gConfig.isDefined(cConfigFlashGameUI))
   //   gMiniMap.setVisible(bVisible);

   if (!mpMinimap)
      return;

   mpMinimap->setVisible(bVisible);
}

//============================================================================
//============================================================================
void BUIManager::setMinimapRotationOffset(float degrees)
{
   if (mpMinimap)
      mpMinimap->setRotationOffset(degrees);
}

//============================================================================
//============================================================================
bool BUIManager::getMinimapVisible()
{
   return mbMinimapVisible;
}

//============================================================================
//============================================================================
bool BUIManager::getMinimapFullZoomOut() const
{
   if (!mpMinimap)
      return false;

   return mpMinimap->getFullZoomOut();
}

//============================================================================
//============================================================================
void BUIManager::setMinimapFullZoomOut(bool bFullZoomOut)
{
   if (!mpMinimap)
      return;
   mpMinimap->setFullZoomOut(bFullZoomOut);
}

//============================================================================
//============================================================================
void BUIManager::setMinimapMapTexture(BManagedTextureHandle mapHandle)
{
   if (!mpMinimap)
      return;
   mpMinimap->setMapTexture(mapHandle);
}

//============================================================================
//============================================================================
void BUIManager::setMinimapSkirtMirroring(bool value)
{
   if (!mpMinimap)
      return;
   mpMinimap->setMapSkirtMirroring(value);
}

//============================================================================
//============================================================================
float BUIManager::getMinimapMapViewCenterX() const
{
   if (!mpMinimap)
      return 0.0f;
   return mpMinimap->getMapViewCenterX();
}

//============================================================================
//============================================================================
float BUIManager::getMinimapMapViewCenterY() const
{
   if (!mpMinimap)
      return 0.0f;
   return mpMinimap->getMapViewCenterY();
}

//============================================================================
//============================================================================
float BUIManager::getMinimapMapViewWidth() const
{
   if (!mpMinimap)
      return 0.0f;
   return mpMinimap->getMapViewWidth();
}

//============================================================================
//============================================================================
float BUIManager::getMinimapMapViewHeight() const
{
   if (!mpMinimap)
      return 0.0f;
   return mpMinimap->getMapViewHeight();
}

//============================================================================
//============================================================================
void BUIManager::resetMinimap()
{
   if (mpMinimap)
   {
      mpMinimap->reset();
   }

   gVisibleMap.resetVisibilityTexture();
}

//============================================================================
//============================================================================
void BUIManager::commitMinimap()
{
   SCOPEDSAMPLE(CommitMinimap);

   if (!mpMinimap)
      return;

   mpMinimap->commit();
}

//============================================================================
//============================================================================
void BUIManager::addMinimapIcon(BObject* pObject)
{
   if (!mpMinimap)
      return;

   mpMinimap->addItem(BFlashMinimap::cMinimapItemTypeObject, pObject);
}

//============================================================================
//============================================================================
void BUIManager::addMinimapIcon(BUnit* pUnit)
{
   if (!mpMinimap)
      return;

   mpMinimap->addItem(BFlashMinimap::cMinimapItemTypeUnit, pUnit);
}

//============================================================================
//============================================================================
void BUIManager::addMinimapIcon(BSquad* pSquad)
{
   if (!mpMinimap)
      return;

   mpMinimap->addItem(BFlashMinimap::cMinimapItemTypeSquad, pSquad);
}

//============================================================================
//============================================================================
void BUIManager::addMinimapFlare(BVector pos, int playerID, int flareType)
{
   if (!mpMinimap)
      return;

   mpMinimap->addFlare(pos, playerID, flareType);
}

//============================================================================
//============================================================================
void BUIManager::addMinimapAlert(BVector pos)
{
   if (!mpMinimap)
      return;

   mpMinimap->addAlert(pos);
   //mpHUD->addAlert(pos);
}

//============================================================================
//============================================================================
void BUIManager::generateMinimapVisibility()
{
   if (!mpMinimap)
      return;

   mpMinimap->generateVisibility();
}

//============================================================================
//============================================================================
void BUIManager::revealMinimap(const BVector& position, float los)
{
   if (!mpMinimap)
     return;

   mpMinimap->addReveal(position, los);
}

//============================================================================
//============================================================================
void BUIManager::revealMinimap(const BVector& position, float width, float height)
{
   if (!mpMinimap)
      return;

   mpMinimap->addReveal(position, width, height);
}

//============================================================================
//============================================================================
void BUIManager::blockMinimap(const BVector& position, float los)
{
   if (!mpMinimap)
      return;

   mpMinimap->addBlock(position, los);
}

//============================================================================
//============================================================================
void BUIManager::blockMinimap(const BVector& position, float width, float height)
{
   if (!mpMinimap)
      return;

   mpMinimap->addBlock(position, width, height);
}

//============================================================================
//============================================================================
bool BUIManager::save(BStream* pStream, int saveType)
{
   GFWRITECLASS(pStream, saveType, gTimerManager);
   GFWRITECLASSPTR(pStream, saveType, mpUICallouts);
   GFWRITECLASSPTR(pStream, saveType, mpUIWidgets);

   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   BUIContext* pPrimaryUIContext = pPrimaryUser->getUIContext();

   bool widgetsVisible = (mpUIWidgets && mpUIWidgets->isWidgetsVisible());
   bool talkingHeadShown = getWidgetUI()->getTalkingHead().isShown();
   bool objectiveTrackerShown = getWidgetUI()->getObjectiveTracker().isShown();
   bool timerShown = getWidgetUI()->getTimerShown();
   bool objectiveWidgetsShown = getWidgetUI()->getObjectiveWidgetsShown();
   bool hintsVisible = (mpUIHints && mpUIHints->isHintsVisible());
   bool minimapVisible = mbMinimapVisible;
   bool unitStatsVisible = pPrimaryUIContext->getUnitStatsVisible();
   bool reticleVisible = pPrimaryUIContext->getReticleVisible();
   bool resourcePanelVisible = pPrimaryUIContext->getResourcePanelVisible();
   bool dpadPanelVisible = pPrimaryUIContext->isDPadPanelVisible();
   bool gameTimeVisible = pPrimaryUIContext->getGameTimeVisible();

   if (isNonGameUIVisible())
   {
      widgetsVisible = true;
      talkingHeadShown = true;
      objectiveTrackerShown = true;
      timerShown = true;
      objectiveWidgetsShown = true;
      hintsVisible = true;
      minimapVisible = pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemMinimap);
      unitStatsVisible = pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemUnitStats);
      reticleVisible = pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemReticle);
      resourcePanelVisible = pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemResources);
      dpadPanelVisible = pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemDpadHelp);
      gameTimeVisible = pPrimaryUser->getHUDItemEnabled(BUser::cHUDItemTime);
   }

   GFWRITEVAR(pStream, bool, widgetsVisible);
   GFWRITEVAR(pStream, bool, talkingHeadShown);
   GFWRITEVAR(pStream, bool, objectiveTrackerShown);
   GFWRITEVAR(pStream, bool, timerShown);
   GFWRITEVAR(pStream, bool, objectiveWidgetsShown);
   GFWRITEVAR(pStream, bool, hintsVisible);
   GFWRITEVAR(pStream, bool, minimapVisible);
   GFWRITEVAR(pStream, bool, unitStatsVisible);
   GFWRITEVAR(pStream, bool, reticleVisible);
   GFWRITEVAR(pStream, bool, resourcePanelVisible);
   GFWRITEVAR(pStream, bool, dpadPanelVisible);
   GFWRITEVAR(pStream, bool, gameTimeVisible);

   float minimapRotationOffset = (mpMinimap ? mpMinimap->getRotationOffset() : 0.0f);
   GFWRITEVAR(pStream, float, minimapRotationOffset);

   return true;
}

//============================================================================
//============================================================================
bool BUIManager::load(BStream* pStream, int saveType)
{
   if (mGameFileVersion >= 2)
      GFREADCLASS(pStream, saveType, gTimerManager)

   GFREADCLASSPTR(pStream, saveType, mpUICallouts);

   if (mGameFileVersion >= 2)
      GFREADCLASSPTR(pStream, saveType, mpUIWidgets)

   if (mGameFileVersion >= 3)
   {
      bool widgetsVisible;
      bool talkingHeadShown;
      bool objectiveTrackerShown;
      bool timerShown;
      bool objectiveWidgetsShown;
      bool hintsVisible;
      bool minimapVisible;
      bool unitStatsVisible;
      bool reticleVisible;
      bool resourcePanelVisible;
      bool dpadPanelVisible;
      bool gameTimeVisible;

      GFREADVAR(pStream, bool, widgetsVisible);
      GFREADVAR(pStream, bool, talkingHeadShown);
      GFREADVAR(pStream, bool, objectiveTrackerShown);
      GFREADVAR(pStream, bool, timerShown);
      GFREADVAR(pStream, bool, objectiveWidgetsShown);
      GFREADVAR(pStream, bool, hintsVisible);
      GFREADVAR(pStream, bool, minimapVisible);
      GFREADVAR(pStream, bool, unitStatsVisible);
      GFREADVAR(pStream, bool, reticleVisible);
      GFREADVAR(pStream, bool, resourcePanelVisible);
      GFREADVAR(pStream, bool, dpadPanelVisible);
      GFREADVAR(pStream, bool, gameTimeVisible);

      BUser* pPrimaryUser = gUserManager.getPrimaryUser();
      BUIContext* pPrimaryUIContext = pPrimaryUser->getUIContext();

      setWidgetsVisible(widgetsVisible);

      if (talkingHeadShown)
         getWidgetUI()->getTalkingHead().show();
      else
         getWidgetUI()->getTalkingHead().hide();

      if (objectiveTrackerShown)
         getWidgetUI()->getObjectiveTracker().show();
      else
         getWidgetUI()->getObjectiveTracker().hide();

//       if (timerShown)
//          getWidgetUI()->showTimer();
//       else
//          getWidgetUI()->hideTimer();

      if (objectiveWidgetsShown)
         getWidgetUI()->showObjectiveWidgets();
      else
         getWidgetUI()->hideObjectiveWidgets();

      setHintsVisible(hintsVisible);

      setMinimapVisible(minimapVisible);

      pPrimaryUIContext->setUnitStatsVisible(unitStatsVisible);
      pPrimaryUIContext->setReticleVisible(reticleVisible);
      pPrimaryUIContext->setResourcePanelVisible(resourcePanelVisible);
      pPrimaryUIContext->setDPadPanelVisible(dpadPanelVisible);
      pPrimaryUIContext->setGameTimeVisible(gameTimeVisible);
   }

   if (mGameFileVersion >= 4)
   {
      float minimapRotationOffset;
      GFREADVAR(pStream, float, minimapRotationOffset);
      if (minimapRotationOffset != 0.0f && mpMinimap)
         mpMinimap->setRotationOffset(minimapRotationOffset * cDegreesPerRadian);
   }

   return true;
}

//==============================================================================
// template<class T> void BUIManager::updatePrimitives(T& list)
//==============================================================================
template<class T> void BUIManager::updatePrimitives(T& list, double curTime)
{
   if (list.empty())
      return;
      
   int n = list.getSize();
   
   for (int i = 0; i < n; i++)
   {
      //-- is this primitive dead?
      if (curTime >= list[i].mTimeout)
      {
         list[i] = list.back();
         list.popBack();
         n--;
         i--;
      }
   }
}

//==============================================================================
// template<class T> void BUIManager::clearPrimitives(T& list, int category);
//==============================================================================
template<class T> void BUIManager::clearPrimitives(T& list, int category)
{
   if (category == -1)
   {    
      list.resize(0);
      return;
   }

   for (uint i = 0; i < list.getSize();i++)
   {      
      //-- is this primitive dead?
      if ((list[i].mCategory == category))
      {
         list[i] = list.back();
         list.popBack();
         i--;
      }
   }
}

//==============================================================================
// BDebugPrimitives::createTimeout
//==============================================================================
float BUIManager::createTimeout(float durationInSeconds)
{
   if (durationInSeconds <= 0.0f)
      return -1.0f;

   // rg [11/21/06] - This is converting absolute time from double to float!
   float timeOut = static_cast<float>(gWorld->getGametimeFloat() + durationInSeconds);
   return timeOut;
}

//==============================================================================
//==============================================================================
void BUIManager::clear2DPrimitives()
{
   clearPrimitives(mPiePrimitives, -1);
   clearPrimitives(mSpritePrimitives, -1);
}

//==============================================================================
//==============================================================================
void BUIManager::addPieProgress(const BMatrix& matrix, BManagedTextureHandle texture, DWORD color, float value, float scaleX, float scaleY, float offsetX, float offsetY, bool fill, bool clockwise, int layer, float timeout)
{
   B2DPrimitivePie* pPrim = mPiePrimitives.pushBackNoConstruction(1);
   pPrim->clear();
   pPrim->mTextureHandle = texture;
   pPrim->mMatrix = matrix;
   pPrim->mColor  = color;
   pPrim->mCW = clockwise;
   pPrim->mOffsetX = offsetX;
   pPrim->mOffsetY = offsetY;
   pPrim->mScaleX = scaleX;
   pPrim->mScaleY = scaleY;
   pPrim->mStart = 0.0f;
   pPrim->mEnd   = value;
   pPrim->mFill  = fill;
   pPrim->mTimeout = createTimeout(timeout);
   pPrim->mBlendMode = BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend;
   pPrim->mCategory  = 0;
   pPrim->mLayer = layer;

   return;
}

//==============================================================================
//==============================================================================
void BUIManager::addSprite(const BMatrix& matrix, BManagedTextureHandle texture, DWORD color, BRender2DPrimitiveUtility::eRender2DBlendMode blendmode, float scaleX, float scaleY, float offsetX, float offsetY, int layer, float timeout)
{
   B2DPrimitiveSprite* pPrim = mSpritePrimitives.pushBackNoConstruction(1);
   pPrim->clear();
   pPrim->mTextureHandle = texture;
   pPrim->mMatrix = matrix;
   pPrim->mColor  = color;
   pPrim->mOffsetX = offsetX;
   pPrim->mOffsetY = offsetY;
   pPrim->mScaleX = scaleX;
   pPrim->mScaleY = scaleY;
   pPrim->mTimeout = createTimeout(timeout);
   pPrim->mBlendMode = blendmode;
   pPrim->mCategory  = 0;
   pPrim->mLayer = layer;
   return;
}

//==============================================================================
//==============================================================================
void BUIManager::yornResult(uint result, DWORD userContext, int port)
{
   // currently we only have one use for this dialog and that's for Unpause
   // if we ever add more uses, we need to define and specify values in the userContext
   // to distinguish the callbacks
//-- FIXING PREFIX BUG ID 1607
   const BUser* pUser = gUserManager.getUserByPort(port);
//--

   gModeManager.getModeGame()->setPaused(false, true, (pUser ? pUser->getPlayerID() : -1));
}

//==============================================================================
//==============================================================================
void BUIManager::showPauseDialog()
{
   if (mScreens[cGameMenu] == NULL)
      return;

   if (mScreens[cGameMenu]->getVisible())
      return;

   // if the screen is not visible, then we want to display a dialog
   // provided we're not running in single human player mode
   if (mbUnpauseOnHideGameMenu)
      return;

   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   if (pUIGlobals)
   {
      if (mbGamePaused)
      {
//-- FIXING PREFIX BUG ID 1608
         const BPlayer* pPlayer = gWorld->getPlayer(gModeManager.getModeGame()->getPausedPlayerID());
//--
         if (pPlayer)
         {
            BUString message;
            message.locFormat(gDatabase.getLocStringFromID(24690), pPlayer->getLocalisedDisplayName().getPtr());
            pUIGlobals->showYornBoxSmall(this, message, BUIGlobals::cDialogButtonsOK, 0, gDatabase.getLocStringFromID(24692));
         }
         else
            pUIGlobals->showYornBoxSmall(this, gDatabase.getLocStringFromID(24691), BUIGlobals::cDialogButtonsOK, 0, gDatabase.getLocStringFromID(24692));

         BUser* pUser = gUserManager.getPrimaryUser();
         if (pUser->getFlagModifierAction())
            pUser->uiModifierAction(false);
         if (pUser->getFlagModifierSpeed())
            pUser->uiModifierSpeed(false);
      }
      else
      {
         pUIGlobals->hideYorn();
      }
   }
}

//==============================================================================
//==============================================================================
void BUIManager::hidePauseDialog()
{
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   if (pUIGlobals)
      pUIGlobals->hideYorn();
}
//============================================================================
//============================================================================
void BUIManager::handleUIScreenResult( BUIScreen* pScreen, long result )
{
   EScreen eScreen = getScreenEnum( pScreen );

   switch( eScreen )
   {
      case cGameMenu: 
         handleGameMenuResult( result ); 
      break;

      case cEndGameScreen: 
         handleEndGameScreenResult( result ); 
      break;

      case cPostGameStatsScreen:
         handlePostGameStatsScreenResult( result );
      break;

      case cObjectivesScreen:
      case cCampaignInfoDialog:
         mpCurrentUser->handleUIManagerResult( cResult_Resume );
      break;
   }
}

//============================================================================
//============================================================================
void BUIManager::handleGameMenuResult( long result )
{
   switch( result )
   {
      case BUIGameMenu::eResult_Resume:   
         mpCurrentUser->handleUIManagerResult( cResult_Resume );
      break;

      case BUIGameMenu::eResult_Resign:
         mpCurrentUser->handleUIManagerResult( cResult_Resign );
      break;

      case BUIGameMenu::eResult_Restart:
         mpCurrentUser->handleUIManagerResult( cResult_Restart );
      break;
   }
}

//============================================================================
//============================================================================
void BUIManager::handleEndGameScreenResult( long result )
{
   switch( result )
   {
      case BUIEndGame::eResult_Quit:
         mpCurrentUser->handleUIManagerResult( cResult_Exit);
      break;

      case BUIEndGame::eResult_Stats:
      {
         showNonGameUI( cPostGameStatsScreen, mpCurrentUser);
         
         //-- If this is the current user change the music state now
         if(gUserManager.getPrimaryUser() == mpCurrentUser)
         {
            if (!gConfig.isDefined(cConfigNoMusic))
            {
               if(mpCurrentUser->getPlayerState() == BPlayer::cPlayerStateWon)
                  gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicGameWon);
               else
                  gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicGameLost);
            }
         }
      }
      break;
   }
}

//============================================================================
//============================================================================
void BUIManager::handlePostGameStatsScreenResult( long result )
{
   switch( result )
   {
      case BUISkirmishPostGameScreen::eResult_Back:
         showNonGameUI( cEndGameScreen, mpCurrentUser );
      break;

      case BUICampaignPostGameScreen::eResult_Quit:
         mpCurrentUser->handleUIManagerResult( cResult_Exit );
      break;

      case BUICampaignPostGameScreen::eResult_Continue:
         mpCurrentUser->handleUIManagerResult( cResult_Continue );
      break;

      case BUICampaignPostGameScreen::eResult_Replay:
         mpCurrentUser->handleUIManagerResult( cResult_Restart );
      break;

      case BUICampaignPostGameScreen::eResult_GoToAdvancedTutorial:
         mpCurrentUser->handleUIManagerResult( cResult_GoToAdvancedTutorial );
      break;
   }
}

//============================================================================
//============================================================================
bool BUIManager::isNonGameUIVisible( void )
{
   return mpCurrentScreen != NULL;
}

//============================================================================
//============================================================================
bool BUIManager::showNonGameUI( BUIManager::EScreen eScreen, BUser* pUser /* = NULL*/ )
{
   if( !pUser )
      pUser = gUserManager.getPrimaryUser();

   if( mpCurrentUser && pUser != mpCurrentUser )
      return false;

   if( mpCurrentScreen && getScreenEnum( mpCurrentScreen ) == eScreen )
      return false;

   if( !hideNonGameUI( false ) )
      hideInGameUI();

   if( eScreen >= 0 && eScreen < cMaxUIScreens )
   {
      mpCurrentUser = pUser;
      mpCurrentScreen = mScreens[eScreen];
      mpCurrentScreen->enter();
      mpCurrentScreen->setVisible( true );
      return true;
   }
   return false;
}

//============================================================================
//============================================================================
bool BUIManager::hideNonGameUI( bool bRestoreInGameUI /* = true*/ )
{
   if( mpCurrentScreen )
   {
      mpCurrentUser = NULL;
      mpCurrentScreen->leave();
      mpCurrentScreen->setVisible( false );
      mpCurrentScreen = NULL;
      
      if( bRestoreInGameUI )
         restoreInGameUI();

      return true;
   }
   
   return false;
}

//============================================================================
//============================================================================
void BUIManager::updateNonGameUI( float elapsedTime )
{
   if( mpCurrentScreen )
      mpCurrentScreen->update( elapsedTime );
}

//============================================================================
//============================================================================
bool BUIManager::handleNonGameUIInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return mpCurrentScreen && mpCurrentScreen->handleInput( port, event, controlType, detail );
}

//============================================================================
//============================================================================
void BUIManager::renderNonGameUI( void )
{
   if( mpCurrentScreen )
      mpCurrentScreen->render();
}

//============================================================================
//============================================================================
BUser* BUIManager::getCurrentUser( void )
{
   return mpCurrentUser;
}

//============================================================================
//============================================================================
BUIManager::EScreen BUIManager::getCurrentScreenEnum( void )
{
   return getScreenEnum(mpCurrentScreen);
}

//============================================================================
//============================================================================
BUIManager::EScreen BUIManager::getScreenEnum( BUIScreen* pScreen )
{
   EScreen eScreen = cMaxUIScreens;
   for( int i = 0; i < cMaxUIScreens; ++i )
   {
      if( mScreens[i] && mScreens[i] == pScreen )
      {
         eScreen = (EScreen)i;
         break;
      }
   }
   return eScreen;
}

//============================================================================
//============================================================================
void BUIManager::hideInGameUI( void )
{
   if( getWidgetUI() )
   {
      getWidgetUI()->getTalkingHead().hide();
      getWidgetUI()->getObjectiveTracker().hide();
      getWidgetUI()->hideTimer();
      getWidgetUI()->hideObjectiveWidgets();
      //getWidgetUI()->setUserMessageVisible(false, true);
      setHintsVisible(false);
   }
   
   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   
   if( !pPrimaryUser )
      return;

   pPrimaryUser->clearCameraState();

   BUIContext* pPrimaryUIContext = pPrimaryUser->getUIContext();

   if( !pPrimaryUIContext )
      return;

   setMinimapVisible(false);
   pPrimaryUIContext->setScoresVisible(false);
   pPrimaryUIContext->setUnitStatsVisible(false);
   pPrimaryUIContext->setReticleVisible(false);
   pPrimaryUIContext->setResourcePanelVisible(false);
   pPrimaryUIContext->setDPadPanelVisible(false);  
   pPrimaryUIContext->setGameTimeVisible(false);

   if( gGame.isSplitScreen() )
   {
      BUser* pSecondaryUser = gUserManager.getSecondaryUser();
      if( !pSecondaryUser )
         return;

      pSecondaryUser->clearCameraState();

      BUIContext* pSecondaryUIContext = pSecondaryUser->getUIContext();

      if( !pSecondaryUIContext )
         return;

      setMinimapVisible(false);
      pSecondaryUIContext->setScoresVisible(false);
      pSecondaryUIContext->setUnitStatsVisible(false);
      pSecondaryUIContext->setReticleVisible(false);
      pSecondaryUIContext->setResourcePanelVisible(false);
      pSecondaryUIContext->setDPadPanelVisible(false);      
      pSecondaryUIContext->setGameTimeVisible(false);
   }
}

//============================================================================
//============================================================================
void BUIManager::restoreInGameUI( void )
{
   if( getWidgetUI() )
   {
      setWidgetsVisible(true);
      getWidgetUI()->getTalkingHead().show();
      getWidgetUI()->getObjectiveTracker().show();
      getWidgetUI()->showTimer();
      getWidgetUI()->showObjectiveWidgets();
      //getWidgetUI()->setUserMessageVisible(true, true);
      setHintsVisible(true);
   }

   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   
   if( !pPrimaryUser )
      return;
   
   BUIContext* pPrimaryUIContext = pPrimaryUser->getUIContext();

   if( !pPrimaryUIContext )
      return;

   setMinimapVisible( pPrimaryUser->getHUDItemEnabled( BUser::cHUDItemMinimap ) );
   pPrimaryUIContext->setScoresVisible( pPrimaryUser->getHUDItemEnabled( BUser::cHUDItemScore ) );
   pPrimaryUIContext->setUnitStatsVisible( pPrimaryUser->getHUDItemEnabled( BUser::cHUDItemUnitStats ) );
   pPrimaryUIContext->setReticleVisible( pPrimaryUser->getHUDItemEnabled( BUser::cHUDItemReticle ) );
   pPrimaryUIContext->setResourcePanelVisible( pPrimaryUser->getHUDItemEnabled( BUser::cHUDItemResources ) );
   pPrimaryUIContext->setDPadPanelVisible( pPrimaryUser->getHUDItemEnabled( BUser::cHUDItemDpadHelp ) );   
   pPrimaryUIContext->setGameTimeVisible( pPrimaryUser->getHUDItemEnabled( BUser::cHUDItemTime ) );

   if( gGame.isSplitScreen() )
   {
      BUser* pSecondaryUser = gUserManager.getSecondaryUser();

      if( !pSecondaryUser )
         return;

      BUIContext* pSecondaryUIContext = pSecondaryUser->getUIContext();

      if( !pSecondaryUIContext )
         return;

      setMinimapVisible( pSecondaryUser->getHUDItemEnabled( BUser::cHUDItemMinimap ) );
      pSecondaryUIContext->setScoresVisible( pSecondaryUser->getHUDItemEnabled( BUser::cHUDItemScore ) );
      pSecondaryUIContext->setUnitStatsVisible( pSecondaryUser->getHUDItemEnabled( BUser::cHUDItemUnitStats ) );
      pSecondaryUIContext->setReticleVisible( pSecondaryUser->getHUDItemEnabled( BUser::cHUDItemReticle ) );
      pSecondaryUIContext->setResourcePanelVisible( pSecondaryUser->getHUDItemEnabled( BUser::cHUDItemResources ) );
      pSecondaryUIContext->setDPadPanelVisible( pSecondaryUser->getHUDItemEnabled( BUser::cHUDItemDpadHelp ) );      
      pSecondaryUIContext->setGameTimeVisible( pSecondaryUser->getHUDItemEnabled( BUser::cHUDItemTime ) );
   }
}

//============================================================================
//============================================================================
BUIScreen* BUIManager::getNonGameUIScreen( EScreen eScreen )
{
   BUIScreen* pScreen = NULL;
   
   if( eScreen >= 0 && eScreen < cMaxUIScreens )
      pScreen = mScreens[eScreen];

   return pScreen;
}