//============================================================================
// UIMainMenuScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIMainMenuScreen.h"

#include "UICampaignMissionPicker.h"
#include "UICampaignMoviePicker.h"
#include "UIMenuItemControl.h"
#include "UIDifficultyDisplayControl.h"

#include "usermanager.h"
#include "user.h"
#include "modemanager.h"
#include "modemenu.h"
#include "database.h"
#include "gamesettings.h"
#include "vincehelper.h"
#include "game.h"
#include "configsgame.h"
#include "gamedirectories.h"
#include "campaignprogress.h"
#include "campaignmanager.h"

//============================================================================
//============================================================================
BUIMainMenuScreen::BUIMainMenuScreen( void ) :
   mCurrentMenu(cMenuMain)
{
}

//============================================================================
//============================================================================
BUIMainMenuScreen::~BUIMainMenuScreen( void )
{

   clearMenu();

   // gBinkInterface.stopAllVideos();
   // mBackgroundVideoHandle = cInvalidVideoHandle;

}

//============================================================================
//============================================================================
bool BUIMainMenuScreen::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   return true;
}

//============================================================================
//============================================================================
bool BUIMainMenuScreen::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   // loadBackgroundMovie(dataNode);

   // initialize all the components for our screen
   //--- PRIMARY MENU
   mMainMenu.init( this, "", cMainMenuControlIDMainMenu, NULL );
   mMainMenu.setWrap(true);

   //--- SECONDARY MENU
   // --------- SECONDARY FLYOUT MENU
   BXMLNode controlsNode;
   if (dataNode.getChild( "UIListControlCustom", &controlsNode ) )
      mSecondaryMenu.init( this, "mSecondaryMenuList", cMainMenuControlIDSecondaryMenu, &controlsNode );
   else
   {
      BASSERTM(false, "UIMPSetupScreen:: Unable to initialize mSecondaryMenuList control input handler.");
      mSecondaryMenu.init( this, "mSecondaryMenuList", cMainMenuControlIDSecondaryMenu, NULL );
   }
   mSecondaryMenu.setIndex(-1);
   mSecondaryMenu.hide();
   mSecondaryMenu.setWrap(true);

/*
   BSimString controlPath;
   BUString text;
   text.set(L" ");
   text.set(L" ");
   for (int i=0; i<cMaxMainMenuSecondaryMenuItems; i++)
   {
      controlPath.format("mSecondaryMenuList.mMenu%d", i);

      BUIMenuItemControl *pMenuItem = new BUIMenuItemControl();
      pMenuItem->init( &mSecondaryMenu, controlPath.getPtr(), -1);
      pMenuItem->setText(text);

      mSecondaryMenu.addControl(pMenuItem);
      mSecondaryMenuItems.add(pMenuItem);
   }

*/
   initMenuItems();

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   displayButtons();

   // updateDifficultyText();

   return true;
}


//============================================================================
//============================================================================
bool BUIMainMenuScreen::displayButtons()
{
   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   long strID0=23437;
   long strID1=0;

   // change the string IDs and button faces as needed

   // A - accept
   // B - back to main menu
   BUString s2;
   BUString s3;
   BUString s4;

   b0 = BUIButtonBarControl::cFlashButtonA;

   if ( (mCurrentMenu != cMenuMain) || mSecondaryMenu.isShown() )
   {
      b1=BUIButtonBarControl::cFlashButtonB;
      strID1=23440;
   }

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(gDatabase.getLocStringFromID(strID0), gDatabase.getLocStringFromID(strID1), s2, s3, s4);

   return true;
}


//============================================================================
//============================================================================
bool BUIMainMenuScreen::updateSecondaryMenuItem(const WCHAR* menuText, int index, int controlID, bool enable)
{

/*
   BSimString controlPath;
   controlPath.format("mSecondaryMenuList.mMenu%d", index);
*/
   BUString text;
   text.set(menuText);

   BUIControl* pControl = mSecondaryMenu.getControl(index);
   if (!pControl)
      return false;

   BUIMenuItemControl *pMenuItem = reinterpret_cast<BUIMenuItemControl*>(pControl);
   pMenuItem->setControlID(controlID);
   pMenuItem->setText(text);

   // set up the control
   if (enable)
   {
      pMenuItem->enable();
      pMenuItem->show();
   }
   else
   {
      pMenuItem->disable();
      pMenuItem->hide();
   }

   return true;
}

//============================================================================
//============================================================================
bool BUIMainMenuScreen::updateMenuItem(const WCHAR* menuText, int index, int controlID, bool enable)
{

/*
   BSimString controlPath;
   controlPath.format("mMenuItem%d", index);
*/
   BUString text;
   text.set(menuText);

   BUIControl* pControl = mMainMenu.getControl(index);
   if (!pControl)
      return false;

   BUIMenuItemControl *pMenuItem = reinterpret_cast<BUIMenuItemControl*>(pControl);
   pMenuItem->setControlID(controlID);
   pMenuItem->setText(text);

   // set up the control
   if (enable)
   {
      pMenuItem->enable();
      pMenuItem->show();
   }
   else
   {
      pMenuItem->disable();
      pMenuItem->hide();
   }

   return true;
}

//============================================================================
//============================================================================
bool BUIMainMenuScreen::addMenuItem(const WCHAR* menuText, int index, int controlID)
{

   BSimString controlPath;
   controlPath.format("mMenuItem%d", index);
   BUString text;
   text.set(menuText);

   BUIMenuItemControl *pMenuItem = new BUIMenuItemControl();
   pMenuItem->init( this, controlPath.getPtr(), controlID);
   pMenuItem->setText(text);

   mMainMenu.addControl(pMenuItem);
   mMenuItems.add(pMenuItem);

   return true;
}


//============================================================================
//============================================================================
bool BUIMainMenuScreen::addSecondaryMenuItem(const WCHAR* menuText, int index, int controlID)
{
   BSimString controlPath;
   controlPath.format("mSecondaryMenuList.mMenu%d", index);
   BUString text;
   text.set(menuText);

   BUIMenuItemControl *pMenuItem = new BUIMenuItemControl();
   pMenuItem->init( this, controlPath.getPtr(), controlID);
   pMenuItem->setText(text);

   mSecondaryMenu.addControl(pMenuItem);
   mSecondaryMenuItems.add(pMenuItem);

   return true;
}

//============================================================================
//============================================================================
bool BUIMainMenuScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {
      gUI.playCancelSound();
      if (mCurrentMenu != cMenuMain)
         populateMainMenu(0);

      return true;
   }

   return false;
}


//============================================================================
//============================================================================
bool BUIMainMenuScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{

   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   BUser* pSecondarUser = gUserManager.getSecondaryUser();

   if (pPrimaryUser->getPort() != port)
   {
      // if the input is not the primary, or the secondary, then eat the input.
      if (pSecondarUser && !pSecondarUser->getFlagUserActive())
         return true;      // eat it.

      if (pSecondarUser && pSecondarUser->getPort() != port)
         return true;      // eat it.
   }

   bool handled = false;

   if (mSecondaryMenu.isShown())
   {
      handled = mSecondaryMenu.handleInput(port, event, controlType, detail);
      if (handled)
         return true;
   }

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   handled = mMainMenu.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   return handled;
}


// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUIMainMenuScreen::handleUIControlEvent( BUIControlEvent& event )
{

   // fixme - get the events from the other main menu.

   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   bool handled = false;
   switch ( control->getControlTypeID() )
   {
      //---------------- Handle the list events --------------------------
      case UIListControlID:
         switch (control->getControlID())
         {
         case cMainMenuControlIDSecondaryMenu:
            {
               if (event.getString()=="cancel")
               {
                  gUI.playCancelSound();
                  mSecondaryMenu.hide();
                  displayButtons();
                  return true;
               }
            }
         }
         break;
      //---------------- Handle the menu item events --------------------------
      case UIMenuItemControlID:

         if (event.getID() != BUIControl::eStringEvent)
            break;
         if (event.getString() != "Accept")
            break;

         gUI.playConfirmSound();

         //BUIMenuItemControl* mi = (BUIMenuItemControl*)control;
         switch (control->getControlID())
         {
            case cCommandBasicTutorial:
               {
                  //mSecondaryMenu.hide();
                  handled = true;
                  gModeManager.getModeMenu()->playTutorial( BModeMenu::cTutorialBasic, BGameSettings::cGameStartContextBasicTutorialFromMainMenu );
               }
               break;
            case cCommandAdvancedTutorial:
               {
                  //mSecondaryMenu.hide();
                  handled = true;
                  gModeManager.getModeMenu()->playTutorial( BModeMenu::cTutorialAdvanced, BGameSettings::cGameStartContextAdvancedTutorialFromMainMenu );
               }               
               break;
            case cCommandCampaign:
               // start the campaign mode
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoCampaign );
               handled=true;
               break;
            case cCommandSkirmish:
               if (!gConfig.isDefined("NewSkirmish"))
                  gModeManager.getModeMenu()->setNextState( BModeMenu::cStateSkirmish );
               else
                  gModeManager.getModeMenu()->setNextState( BModeMenu::cStateSkirmishSetup );
               handled=true;
               break;
            case cCommandMultiplayer:
               // show the multiplayer sub menu
               populateMultiplayerMenu(0);
               handled=true;
               break;
            case cCommandTimeline:
               if (gConfig.isDefined("ShowTimeline"))
                  gModeManager.getModeMenu()->setNextState( BModeMenu::cStateTimeline );
               handled=true;
               break;
            case cCommandOptions:
               // show the options screen
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateOptions );
               handled=true;
               break;
            case cCommandOptionsExtras:
               // show the extras menu
               //gModeManager.getModeMenu()->setNextState( BModeMenu::cStateExtras );
               populateExtrasMenu(0);
               handled=true;
               break;
            case cCommandDownloads:
               handled=true;
               break;
            case cCommandDemoNoMovie:
            {
               BCampaign *pCampaign = gCampaignManager.getCampaign(0);
               pCampaign->setPlayContinuous(true);
               pCampaign->setCurrentNodeID(4);
               pCampaign->launchGame(true);
               gConfig.define("SkipCampaignVideo");
               gModeManager.getModeMenu()->setNextState(BModeMenu::cStateGotoGame);
               handled=true;
               break;
            }
            case cCommandTutorial:
               populateTutorialMenu();
               positionMovieClip(control->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
               mSecondaryMenu.show();
               displayButtons();
               handled=true;
               break;
            case cCommandDemoMovie:
            {
               BCampaign *pCampaign = gCampaignManager.getCampaign(0);
               BASSERT(pCampaign);
               pCampaign->setPlayContinuous(true);
               pCampaign->setCurrentNodeID(4);
               pCampaign->launchGame(true);
               gConfig.remove("SkipCampaignVideo");
               gModeManager.getModeMenu()->setNextState(BModeMenu::cStateGotoGame);
               handled=true;
               break;
            }
            case cCommandDemoUNSCSkirmish:
            case cCommandDemoCovenantSkirmish:
            {
               gDatabase.resetGameSettings();
               BGameSettings* pSettings=gDatabase.getGameSettings();

               BSimString gameID;
               MVince_CreateGameID(gameID);

               BUser* pUser=gUserManager.getPrimaryUser();
               pUser->setFlagUserActive(true);

               BSimString scenarioName = "Skirmish\\Design\\Chasms";
               gConfig.get("DemoMap", scenarioName);

               pSettings->setLong(BGameSettings::cPlayerCount, 2);
               pSettings->setString(BGameSettings::cMapName, scenarioName);
               pSettings->setLong(BGameSettings::cMapIndex, -1);
               pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
               pSettings->setString(BGameSettings::cGameID, gameID);
               pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
               pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
               pSettings->setLong(BGameSettings::cGameMode, 0);
               pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
               pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
               pSettings->setLong(BGameSettings::cPlayer1Team, 1);
               pSettings->setLong(BGameSettings::cPlayer2Team, 2);
               pSettings->setString(BGameSettings::cPlayer2Name, BSimString("AI Player"));

               if (control->getControlID() == cCommandDemoUNSCSkirmish)
               {
                  pSettings->setLong(BGameSettings::cPlayer1Civ, 1);
                  pSettings->setLong(BGameSettings::cPlayer1Leader, 1);

                  pSettings->setLong(BGameSettings::cPlayer2Civ, 2);
                  pSettings->setLong(BGameSettings::cPlayer2Leader, 6);
               }
               else
               {
                  pSettings->setLong(BGameSettings::cPlayer1Civ, 2);
                  pSettings->setLong(BGameSettings::cPlayer1Leader, 6);

                  pSettings->setLong(BGameSettings::cPlayer2Civ, 1);
                  pSettings->setLong(BGameSettings::cPlayer2Leader, 1);
               }

               gModeManager.getModeMenu()->setNextState(BModeMenu::cStateGotoGame);
               break;
            }
            case cCommandSystemLink:
               // start party room in system link mode.
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoSystemLink );
               handled=true;
               break;
            case cCommandXboxLive:
               // start the party room in live mode
               //gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoMultiplayer );
               gModeManager.getModeMenu()->setNextStateWithLiveCheck( BModeMenu::cStateGotoMultiplayer, true );
               handled=true;
               break;
            case cCommandLeaderboards:
               // show the leader board screen
               // gModeManager.getModeMenu()->setNextState( BModeMenu::cStateLeaderboards );
               gModeManager.getModeMenu()->setNextStateWithLiveCheck( BModeMenu::cStateLeaderboards, false);
               handled=true;
               break;
            case cCommandServiceRecord:
               // show the service record screen
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateServiceRecord );
               //gModeManager.getModeMenu()->setNextStateWithLiveCheck( BModeMenu::cStateServiceRecord, false);
               handled=true;
               break;
/*
            case cCommandStorageDevice:
               gUserManager.showDeviceSelector(gUserManager.getPrimaryUser(), NULL, 0, gSaveGame.MAX_SIZE_IN_BYTES, true);
               handled=true;
               break;
*/
            case cCommandExit:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateExit);
               // exit the game
               handled=true;
               break;
            case cCommandOther:
               //gModeManager.getModeMenu()->setNextState( BModeMenu::cStateOther);
               // show the other menu
               populateOtherMenu(0);
               handled=true;
               break;
            case cCommandSaveGame:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateSaveGame );
               handled=true;
               break;
            case cCommandRecordGame:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateRecordGame );
               handled=true;
               break;
            case cCommandCredits:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateCreditsMovie );
               handled=true;
               break;
            case cCommandScenario:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateScenario );
               handled=true;
               break;
            case cCommandCalibrate:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoCalibrate );
               handled=true;
               break;
            case cCommandFlash:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateFlash );
               handled=true;
               break;
            case cCommandCinematic:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateCinematic );
               handled=true;
               break;
            case cCommandModelView:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateModelView );
               handled=true;
               break;
            case cCommandStartCampaign:
               gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(25580), BUIGlobals::cDialogButtonsOKCancel, cCommandStartCampaign, gDatabase.getLocStringFromID(25578), gDatabase.getLocStringFromID(25579) );
               handled = true;
               break;
            case cCommandContinueCampaign:
               gModeManager.getModeMenu()->setNextState( BModeMenu::cStateContinueCampaign );
               handled = true;
               break;
         }
         break;
   }
   return handled;

}


//==============================================================================
//==============================================================================
void BUIMainMenuScreen::render()
{
   BUIScreen::render();
}

//==============================================================================
//==============================================================================
void BUIMainMenuScreen::clearMenu()
{
   mMainMenu.clearControls();

   BUIMenuItemControl* c = NULL;
   for (int i=0; i<mMenuItems.getNumber(); i++)
   {
      c = mMenuItems[i];
      delete c;
      mMenuItems[i]=NULL;
   }
   mMenuItems.clear();

   for (int i=0; i<mSecondaryMenuItems.getNumber(); i++)
   {
      c = mSecondaryMenuItems[i];
      delete c;
      mSecondaryMenuItems[i]=NULL;
   }
   mSecondaryMenuItems.clear();

}

//==============================================================================
//==============================================================================
void BUIMainMenuScreen::populateMultiplayerMenu(int selectedItem)
{

   gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateSkirmishMenu);

   mCurrentMenu = cMenuMultiplayer;

   long n=0;
   updateMenuItem(gDatabase.getLocStringFromID(24924), n++, cCommandXboxLive, true);
   
   updateMenuItem(gDatabase.getLocStringFromID(24925), n++, cCommandLeaderboards, true);
   
   updateMenuItem(gDatabase.getLocStringFromID(24926), n++, cCommandSystemLink, true);
   updateMenuItem(gDatabase.getLocStringFromID(25289), n++, cCommandServiceRecord, true);
   // updateMenuItem(gDatabase.getLocStringFromID(24928), n++, cCommandDownloads, true);

   disableMenuItems(n);

   if ( (selectedItem<0) || (selectedItem>=n))
      selectedItem=0;

   mMainMenu.setIndex(selectedItem);

   displayButtons();
}

//==============================================================================
//==============================================================================
void BUIMainMenuScreen::populateExtrasMenu(int selectedItem)
{

   mCurrentMenu = cMenuExtras;


   long n=0;

   updateMenuItem(gDatabase.getLocStringFromID(24935), n++, cCommandOptions, true);
   updateMenuItem(gDatabase.getLocStringFromID(24936), n++, cCommandCredits, true);
   updateMenuItem(gDatabase.getLocStringFromID(25289), n++, cCommandServiceRecord, true);
   //updateMenuItem(gDatabase.getLocStringFromID(25959), n++, cCommandStorageDevice, true);


   disableMenuItems(n);

   if ( (selectedItem<0) || (selectedItem>=n))
      selectedItem=0;

   mMainMenu.setIndex(selectedItem);
   displayButtons();
}


//==============================================================================
//==============================================================================
void BUIMainMenuScreen::populateOtherMenu(int selectedItem)
{
   mCurrentMenu = cMenuOther;

   long n=0;
   // Note: These are not shipping and don't need to be loc'd
   updateMenuItem(L"SCENARIO",         n++, cCommandScenario, true);
   updateMenuItem(L"SAVED GAME",       n++, cCommandSaveGame, true);
   updateMenuItem(L"RECORDED GAME",    n++, cCommandRecordGame, true);
   updateMenuItem(L"CALIBRATE",        n++, cCommandCalibrate, true);
   updateMenuItem(L"FLASH VIEWER",     n++, cCommandFlash, true);
   updateMenuItem(L"CINEMATIC",        n++, cCommandCinematic, true);
   updateMenuItem(L"MODEL VIEWER",     n++, cCommandModelView, true);
   updateMenuItem(L"EXIT",             n++, cCommandExit, true);

   disableMenuItems(n);

   if ( (selectedItem<0) || (selectedItem>=n))
      selectedItem=0;

   mMainMenu.setIndex(selectedItem);

   displayButtons();
}

//==============================================================================
//==============================================================================
void BUIMainMenuScreen::initMenuItems()
{
   for (int i=0; i<cMaxMenuItems; i++)
      addMenuItem(L"", i, -1);

   for (int i=0; i<cMaxMainMenuSecondaryMenuItems; i++)
      addSecondaryMenuItem(L"", i, -1);

}



//==============================================================================
//==============================================================================
void BUIMainMenuScreen::disableMenuItems(int startngIndex)
{
   long n = startngIndex;
   while (n < cMaxMenuItems)
   {
      updateMenuItem(L"", n++, 0, false);
   }
}

//==============================================================================
//==============================================================================
void BUIMainMenuScreen::disableSecondaryMenuItems(int startngIndex)
{
   long n = startngIndex;
   while (n < cMaxMainMenuSecondaryMenuItems)
   {
      updateSecondaryMenuItem(L"", n++, 0, false);
   }
}


//==============================================================================
//==============================================================================
void BUIMainMenuScreen::populateTutorialMenu()
{
   long n = 0;

   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25427), n++, cCommandBasicTutorial, true);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25428), n++, cCommandAdvancedTutorial, true);

/*
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(24929), n++, cCommandCampaign, true);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(24930), n++, cCommandSkirmish, true);
*/

   disableSecondaryMenuItems(n);

   mSecondaryMenu.setIndex(0);

   displayButtons();
}


//==============================================================================
//==============================================================================
void BUIMainMenuScreen::populateMainMenu(int selectedItem)
{
   gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateMainTheme);

   mCurrentMenu = cMenuMain;

   long n = 0;
   if( gConfig.isDefined(cConfigDemo) )
   {
      updateMenuItem(L"DEMO", n++, cCommandDemoMovie, true);
      updateMenuItem(L"EXIT", n++, cCommandExit, true);
      disableMenuItems(n);
      mMainMenu.setIndex(0);
      displayButtons();
      return;
   }
   else if (gConfig.isDefined(cConfigDemo2))
   {
      updateMenuItem(L"CAMPAIGN", n++, cCommandCampaign, true);
      updateMenuItem(L"UNSC SKIRMISH", n++, cCommandDemoUNSCSkirmish, true);
      updateMenuItem(L"COVENANT SKIRMISH", n++, cCommandDemoCovenantSkirmish, true);
      updateMenuItem(L"TUTORIAL", n++, cCommandTutorial, true);
      updateMenuItem(L"EXIT", n++, cCommandExit, true);
      disableMenuItems(n);
      mMainMenu.setIndex(0);
      displayButtons();
      return;
   }

   updateMenuItem(gDatabase.getLocStringFromID(24929), n++, cCommandCampaign, true);
   updateMenuItem(gDatabase.getLocStringFromID(24930), n++, cCommandSkirmish, true);
   updateMenuItem(gDatabase.getLocStringFromID(25426), n++, cCommandTutorial, true);
   updateMenuItem(gDatabase.getLocStringFromID(24932), n++, cCommandMultiplayer, true);
   updateMenuItem(gDatabase.getLocStringFromID(24931), n++, cCommandTimeline, true);
   updateMenuItem(gDatabase.getLocStringFromID(24933), n++, cCommandOptionsExtras, true);

   if (gConfig.isDefined(cConfigShowDebugMenu))
   {
      updateMenuItem(L"DEVELOPER", n++, cCommandOther, true);
   }

   disableMenuItems(n);

   if ( (selectedItem<0) || (selectedItem>=n))
      selectedItem=0;
   mMainMenu.setIndex(selectedItem);

   displayButtons();
}

//==============================================================================
//==============================================================================
void BUIMainMenuScreen::yornResult(uint result, DWORD userContext, int port)
{
   switch( userContext )
   {
      case cCommandStartCampaign:
         {
            long startNodeID;
            if( result == BUIGlobals::cDialogResultOK )
               startNodeID = 2;
            else
               startNodeID = 3;

            BUser* pUser=gUserManager.getPrimaryUser();
            BUserProfile* pProfile = pUser->getProfile();

            // error checking
            if (!pUser || !pProfile)
            {
               // gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoCampaign );
               return;
            }

            // Start the Campaign in Single Player Mode
            // determine the last campaign mode the player was in
            BCampaign * pCampaign = gCampaignManager.getCampaign(0);
            BASSERT(pCampaign);
            if (!pCampaign)
               return;

            pCampaign->setPlayContinuous(true);

            pCampaign->setCurrentNodeID(startNodeID);

            if( pCampaign->getCurrentNodeID() == -1 )
               pCampaign->setCurrentNodeID(0);

            gModeManager.getModeMenu()->setNextState(BModeMenu::cStateContinueCampaign);
         }
         break;
   }
}


//==============================================================================
//==============================================================================
void BUIMainMenuScreen::playCampaign(int startingNode /* = -1*/)
{
   BUser* pUser=gUserManager.getPrimaryUser();
   BUserProfile* pProfile = pUser->getProfile();

   // error checking
   if (!pUser || !pProfile)
   {
      return;
   }

/*
   BModeCampaign2* pMode = getCampaignMode();
   BASSERT(pMode);
   if (!pMode)
      return;
*/

   // Start the Campaign in Single Player Mode
   // determine the last campaign mode the player was in
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (!pCampaign)
      return;

   pCampaign->setPlayContinuous(true);

   if( startingNode != -1 )
      pCampaign->setCurrentNodeID(startingNode);

   if( pCampaign->getCurrentNodeID() == -1 )
      pCampaign->setCurrentNodeID(0);

   // [7/21/2008 xemu] do the actual launching into game mode 
   pCampaign->launchGame(true);

   gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoGame );

}

//==============================================================================
//==============================================================================
void BUIMainMenuScreen::positionMovieClip(const char* firstMC, const char* secondMC )
{
   GFxValue values[2];
   values[0].SetString(firstMC);
   values[1].SetString(secondMC);
   mpMovie->invokeActionScript("positionMovieClip", values, 2);
}
