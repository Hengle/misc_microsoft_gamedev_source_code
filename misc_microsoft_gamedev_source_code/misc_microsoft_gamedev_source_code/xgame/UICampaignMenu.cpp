//============================================================================
// UICampaignMenu.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UICampaignMenu.h"

#include "UICampaignMissionPicker.h"
#include "UICampaignMoviePicker.h"
#include "UIMenuItemControl.h"
#include "UIDifficultyDisplayControl.h"

#include "usermanager.h"
#include "user.h"
#include "modemanager.h"
#include "ModeCampaign2.h"
#include "database.h"
#include "gamesettings.h"
#include "vincehelper.h"
#include "game.h"
#include "configsgame.h"
#include "gamedirectories.h"
#include "campaignprogress.h"
#include "BackgroundMovies.h"

#include "modemenu.h"

#define LOC( id ) gDatabase.getLocStringFromID( id )

enum
{
   cDialogNewCampaignTutorial,
};

//============================================================================
//============================================================================
BUICampaignMenu::BUICampaignMenu( void ) : 
   mpMissionPicker(NULL),
   mpMoviePicker(NULL),
   mFoundSaveGame(false),
   mBackgroundVideoHandle(cInvalidVideoHandle)
{
}

//============================================================================
//============================================================================
BUICampaignMenu::~BUICampaignMenu( void )
{
   if (mpMoviePicker)
   {
      delete mpMoviePicker;
      mpMoviePicker=NULL;
   }

   if (mpMissionPicker)
   {
      delete mpMissionPicker;
      mpMissionPicker=NULL;
   }

   gBinkInterface.stopAllVideos(false);
   mBackgroundVideoHandle = cInvalidVideoHandle;

   gUserManager.removeListener(this);
}

//============================================================================
//============================================================================
bool BUICampaignMenu::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignMenu::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   mBackgroundMovies.loadEvents();

   loadBackgroundMovie();

   // Menu
   mCampaignMenu.init( this, "", cCampaignControlIDMainMenu, NULL );
   for( int i = 0; i < cMAX_MENU_ITEMS; ++i )
   {
      BSimString controlPath;
      controlPath.format("mMenuItem%d", i);
      mMenuItems[i].init( this, controlPath.getPtr() );
      mMenuItems[i].hide();
   }

   mCampaignMenu.setWrap(true);

   populateMenu();

   //--- SECONDARY MENU
   BXMLNode controlsNode;
   if (dataNode.getChild( "UIListControlCustom", &controlsNode ) )
      mSecondaryMenu.init( this, "mSecondaryMenuList", cCampaignControlIDSecondaryMenu, &controlsNode );
   else
   {
      BASSERTM(false, "UIMPSetupScreen:: Unable to initialize mSecondaryMenuList control input handler.");
      mSecondaryMenu.init( this, "mSecondaryMenuList", cCampaignControlIDSecondaryMenu, NULL );
   }
   mSecondaryMenu.setIndex(-1);
   mSecondaryMenu.hide();
   mSecondaryMenu.setWrap(true);

   initMenuItems();

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   mDifficultyText.init(this, "mDifficultyText", 0, NULL);

   mTitleLabel.init(this, "mTitleLabel");
   mTitleLabel.setText(gDatabase.getLocStringFromID(24929));
   mPlayerListLabel.init(this, "mPlayerListLabel");
   mPlayerListLabel.setText(gDatabase.getLocStringFromID(23447));

   setupGamerTagControls();
   updateGamerTagControls();

   // Initialize our mission picker
   if (mpMissionPicker==NULL)
   {
      mpMissionPicker=new BUICampaignMissionPicker();
      mpMissionPicker->init("art\\ui\\flash\\pregame\\CampaignMissionPicker\\UICampaignMissionPickerScreen.gfx", "art\\ui\\flash\\pregame\\CampaignMissionPicker\\UICampaignMissionPickerScreenData.xml");
      mpMissionPicker->setVisible(false);
      mpMissionPicker->setParent(this);
   }

   // Initialize our movie picker
   if (mpMoviePicker==NULL)
   {
      mpMoviePicker=new BUICampaignMoviePicker();
      mpMoviePicker->init("art\\ui\\flash\\pregame\\CampaignMoviePicker\\UICampaignMoviePickerScreen.gfx", "art\\ui\\flash\\pregame\\CampaignMoviePicker\\UICampaignMoviePickerScreenData.xml");
      mpMoviePicker->setVisible(false);
      mpMoviePicker->setParent(this);
   }
   displayButtons();

   updateDifficultyText();

   gUserManager.addListener(this);

   return true;
}


//============================================================================
//    fixme - we need to get the auto updating of the user control working
//          for when users sign in or sign out
//============================================================================
void BUICampaignMenu::setupGamerTagControls()
{

   // initialize the gamertag controls
   mGamerTagPrimary.init(this, "mGamerTagPrimary", 0, NULL);
   mGamerTagSecondary.init(this, "mGamerTagSecondary", 0, NULL);

   mGamerTagPrimary.hide();
   mGamerTagSecondary.hide();

}

//============================================================================
//============================================================================
void BUICampaignMenu::updateGamerTagControls()
{
/*
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser && pUser->getFlagUserActive())
   {
      mGamerTagPrimary.populateFromUser(pUser);
      mGamerTagPrimary.show();
   }
   else
      mGamerTagPrimary.hide();

   if (gGame.isSplitScreenAvailable())
   {
      pUser = gUserManager.getSecondaryUser();
      if (pUser && pUser->getFlagUserActive())
      {
         mGamerTagSecondary.populateFromUser(pUser);
         mGamerTagSecondary.show();
      }
      else
      {
         if (gConfig.isDefined(cConfigEnableAttractMode))
         {
            // mGamerTagSecondary.clear();
            BUString temp;
            temp.set(L"Press A to join.");
            mGamerTagSecondary.setGamerTag(temp);

            mGamerTagSecondary.show();
         }
         else 
            mGamerTagSecondary.hide();
      }
   }
   else 
      mGamerTagSecondary.hide();
*/
}


//============================================================================
//============================================================================
void BUICampaignMenu::userStatusChanged()
{
   updateGamerTagControls();
}


//============================================================================
//============================================================================
bool BUICampaignMenu::displayButtons()
{
   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   // change the string IDs and button faces as needed

   // A - accept
   // B - back to main menu
   BUString s0;
   BUString s1;
   BUString s2;
   BUString s3;
   BUString s4;

   b0 = BUIButtonBarControl::cFlashButtonA;
   s0.set(gDatabase.getLocStringFromID(23437)); // accept

   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(23440)); // back

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignMenu::addMenuItem(const WCHAR* menuText, int index, int controlID, int data)
{
   BASSERT( index < cMAX_MENU_ITEMS );
   BUIMenuItemControl& rMenuItem = mMenuItems[index];
   rMenuItem.show(true);
   rMenuItem.setText(menuText);
   rMenuItem.setControlID(controlID);
   rMenuItem.setData(data);
   mCampaignMenu.addControl(&rMenuItem);
   
   return true;
}

//============================================================================
//============================================================================
bool BUICampaignMenu::addSecondaryMenuItem(const WCHAR* menuText, int index, int controlID)
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
bool BUICampaignMenu::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {
      gUI.playCancelSound();

      // [7/31/2008 xemu] added support for backing out of flyout submenu
      if (mSecondaryMenu.isShown())
      {
         mSecondaryMenu.hide();
         return true;
      }

      // fixme - confirmation dialog?

      BModeCampaign2* pMode = getCampaignMode();
      BASSERT(pMode);
      if (!pMode)
         return false;

      pMode->setNextState(BModeCampaign2::cStateGotoMainMenu);

      return true;
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BUICampaignMenu::checkSecondaryUser(long port, long event, long controlType, BInputEventDetail& detail)
{
   BUser* pSecondaryUser = gUserManager.getSecondaryUser();

   // when will we not have a secondary user?
   if (!pSecondaryUser)
      return false;

   // if we already have a secondary user, then scoot out.
   if (pSecondaryUser->getFlagUserActive())
      return false;

   if ( (event==cInputEventControlStart) && (controlType == cButtonA))
   {
      switch (controlType)
      {
      case cButtonA:
         {
            // let's hook this guy up.
            gUserManager.setUserPort(BUserManager::cSecondaryUser, port);
            pSecondaryUser->setFlagUserActive(true);

            updateGamerTagControls();

            // we eat the input
            return true;
         }
      }
   }

   return false;

}

//============================================================================
//============================================================================
bool BUICampaignMenu::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   BUser* pSecondaryUser = gUserManager.getSecondaryUser();

   if (gGame.isSplitScreenAvailable())
   {
      if (getVisible())
      {
         // check for secondary user controller
         if (pPrimaryUser->getPort() != port)
         {
            if (checkSecondaryUser(port, event, controlType, detail))
               return true;
         }
      }
   }

   if (pPrimaryUser->getPort() != port)
   {
      // if the input is not the primary, or the secondary, then eat the input.
      if (pSecondaryUser && !pSecondaryUser->getFlagUserActive())
         return true;      // eat it.

      if (pSecondaryUser && pSecondaryUser->getPort() != port)
         return true;      // eat it.
   }


   bool handled = false;

   // if a screen is up, let it handle the events. We don't want to look at them.
   if (mpMissionPicker->getVisible())
      return mpMissionPicker->handleInput(port, event, controlType, detail);

   if (mpMoviePicker->getVisible())
      return mpMoviePicker->handleInput(port, event, controlType, detail);

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   if (mSecondaryMenu.isShown())
   {
      handled = mSecondaryMenu.handleInput(port, event, controlType, detail);
      if (handled)
         return true;
   }
   else
   {
      handled = mCampaignMenu.handleInput(port, event, controlType, detail);
      if (handled)
         return true;
   }

   return handled;
}


// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICampaignMenu::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   BModeCampaign2* pMode = getCampaignMode();
   BASSERT(pMode);
   if (!pMode)
      return false;

   bool handled = false;

   switch ( control->getControlTypeID() )
   {
      case UIMenuItemControlID:

         if (event.getID() != BUIControl::eStringEvent)
            break;
         if (event.getString() != "Accept")
            break;

         gUI.playConfirmSound();

         //BUIMenuItemControl* mi = (BUIMenuItemControl*)control;
         switch (control->getControlID())
         {
            case cControlContinue:
               playCampaign();
               handled=true;
               break;
            case cControlLoadGame:
               {
                  if ( checkSaveDevice() )
                  {
                     showLoadGameFlyout();
                     positionMovieClip(control->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                  }
                  handled=true;
               }
               break;
            case cControlLoadGameStart:
               {
                  if (gSaveGame.getCampaignSaveExists(&mSavegameSettings))
                  {
                     BCampaign * pCampaign = gCampaignManager.getCampaign(0);
                     BASSERT(pCampaign);
                     if (pCampaign)
                     {
                        BString mapName;
                        mSavegameSettings.getString(BGameSettings::cMapName, mapName);
                        if (mapName == "NewCampaign")
                           playCampaign(0);
                        else
                        {
                           BCampaignNode* pNode = pCampaign->getNode(mapName);
                           if (pNode)
                           {
                              gSaveGame.setLoadDeviceRemove(false);
                              playCampaign(pNode->getID(), true);
                           }
                        }
                     }
                  }
                  handled=true;
               }
               break;
            case cControlLoadChangeDevice:
               {
                  gGame.getUIGlobals()->showWaitDialog(gDatabase.getLocStringFromID(25989));
                  BModeCampaign2* pModeCampaign = (BModeCampaign2*)gModeManager.getMode( BModeManager::cModeCampaign2 );
                  if (!gUserManager.showDeviceSelector( gUserManager.getPrimaryUser(), pModeCampaign, 0, 0, true ))
                     gGame.getUIGlobals()->setWaitDialogVisible(false);
                  mSecondaryMenu.hide();
               }
               break;

            case cControlNewCampaign:
               gGame.getUIGlobals()->showYornBoxSmall( this, LOC(25580), BUIGlobals::cDialogButtonsOKCancel, cDialogNewCampaignTutorial, LOC(25578), LOC(25579) );
               handled=true;
               break;

            case cControlTutorial:
               showTutorialFlyout();
               positionMovieClip(control->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
               handled = true;
               break;

            case cControlCampaignTutorial:
               {
                  //mSecondaryMenu.hide();
                  handled = true;
                  
                  BModeCampaign2* pModeCampaign = (BModeCampaign2*)gModeManager.getMode( BModeManager::cModeCampaign2 );
                  pModeCampaign->setNextState( BModeCampaign2::cStateGotoMainMenu );

                  BModeMenu* pModeMenu = gModeManager.getModeMenu();
                  BASSERT( pModeMenu );
                  if( pModeMenu )
                     pModeMenu->playTutorial( BModeMenu::cTutorialBasic, BGameSettings::cGameStartContextBasicTutorialFromSPCMenu );
               }
               break;

            case cControlSkirmishTutorial:
               {
                  //mSecondaryMenu.hide();
                  handled = true;
                  
                  BModeCampaign2* pModeCampaign = (BModeCampaign2*)gModeManager.getMode( BModeManager::cModeCampaign2 );
                  pModeCampaign->setNextState( BModeCampaign2::cStateGotoMainMenu );

                  
                  BModeMenu* pModeMenu = gModeManager.getModeMenu();
                  BASSERT( pModeMenu );
                  if( pModeMenu )
                     pModeMenu->playTutorial( BModeMenu::cTutorialAdvanced, BGameSettings::cGameStartContextAdvancedTutorialFromSPCMenu );
               }
               break;

            case cControlMissionSelector:
               // fixme - this is just a test, we need to put actual code here.
               mpMissionPicker->setVisible(true);
               mpMissionPicker->updateTitle();
               mpMissionPicker->updateScoreDisplay();
               mpMissionPicker->updateRScrollButton();
               this->setVisible(false);
               handled = true;
               break;

            case cControlCoop:
               showCoopFlyout();
               positionMovieClip(control->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
               handled = true;
               break;

            case cControlCoopLive:
               pMode->setNextStateWithLiveCheck( BModeCampaign2::cStateStartLiveCampaign, true );
               handled=true;
               break;

            case cControlCoopSysLink:
               pMode->setNextState(BModeCampaign2::cStateStartLanCampaign);
               handled=true;
               break;

            case cControlMovieSelector:
               // fixme - this is just a test, we need to put actual code here.
               mpMoviePicker->setVisible(true);
               mpMoviePicker->enterScreen();
               mpMoviePicker->updateRScrollButton();
               this->setVisible(false);
               handled = true;
               break;      
            case cControlChangeDifficulty:
               showDifficultyFlyout();
               positionMovieClip(control->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
               handled = true;
               break;
               
               // [7/31/2008 xemu] flyout difficulty submenu
            case cControlSetDifficultyEasy:
               setDifficulty(0);
               handled = true;
               break;
            case cControlSetDifficultyNormal:
               setDifficulty(1);
               handled = true;
               break;
            case cControlSetDifficultyHeroic:
               setDifficulty(2);
               handled = true;
               break;
            case cControlSetDifficultyLegendary:
               setDifficulty(3);
               handled = true;
               break;
         }
         break;
   }
   return handled;




}


//==============================================================================
//==============================================================================
void BUICampaignMenu::render()
{
   BUIScreen::render();

   if (mpMissionPicker)
      mpMissionPicker->render();

   if (mpMoviePicker)
      mpMoviePicker->render();
}

//==============================================================================
//==============================================================================
BModeCampaign2* BUICampaignMenu::getCampaignMode()
{
   BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);

   return pMode;
}

//==============================================================================
// We should really just centralize this method.
//==============================================================================
void BUICampaignMenu::playCampaign(int startingNode /* = -1*/, bool useSaveIfAvailable /* = -1 */)
{
   BUser* pUser=gUserManager.getPrimaryUser();
   BUserProfile* pProfile = pUser->getProfile();

   // error checking
   if (!pUser || !pProfile)
   {
      // gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoCampaign );
      return;
   }

   BModeCampaign2* pMode = getCampaignMode();
   BASSERT(pMode);
   if (!pMode)
      return;

   // Start the Campaign in Single Player Mode
   // determine the last campaign mode the player was in
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (!pCampaign)
      return;

   pCampaign->setPlayContinuous(true);

   if( startingNode != -1 )
      pCampaign->setCurrentNodeID(startingNode);
/*
// SRL 10/29/08 - Change the way save games work, this parm is passed in now as needed.
   else
      pCampaign->setUseSaveIfAvailable(true);
*/

   if( pCampaign->getCurrentNodeID() == -1 )
      pCampaign->setCurrentNodeID(0);
   
   // [7/21/2008 xemu] do the actual launching into game mode 
   pCampaign->launchGame(true, useSaveIfAvailable);
}

//==============================================================================
// BUICampaignMenu::setDifficulty
//==============================================================================
void BUICampaignMenu::setDifficulty(long newDiff)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   BASSERT(pUser);
   if( pUser )
      pUser->setOption_DefaultCampaignDifficulty( (uint8)newDiff );
   
   // [7/24/2008 xemu] update the text
   updateDifficultyText();

   // [7/31/2008 xemu] close the flyout now that we've selected
   mSecondaryMenu.hide();
}

//==============================================================================
// BUICampaignMenu::updateDifficultyText
//==============================================================================
void BUICampaignMenu::updateDifficultyText()
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (!pCampaign)
      return;

   mDifficultyText.update( pCampaign->getCurrentDifficulty() );
   if ((mpMissionPicker != NULL) && (mpMissionPicker->getDifficultyDisplay() != NULL))
      mpMissionPicker->getDifficultyDisplay()->update( pCampaign->getCurrentDifficulty() );
}


//==============================================================================
// BUICampaignMenu::loadBackgroundMovie
//==============================================================================
void BUICampaignMenu::loadBackgroundMovie()
{
   BSimString movie;

   BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
   long unlockCount = 0;
   if (pProgress != NULL)
      unlockCount = pProgress->getUnlockedMissionCount();

   // [7/31/2008 xemu] cycle through all the possible background movies, noting ones we qualify for and then when we finish, the last qualifier is the winner 
   for (int i=0; i<mBackgroundMovies.getNumber(); i++)
   {
      const BBackgroundMovieData* pData = mBackgroundMovies.getData(i);
      if (pData)
      {
         // look for the node that matches our current node
         if (pData->mUnlocksAt <= unlockCount)
            movie.set(pData->mMovieName.getPtr());
      }
   }

   if (!movie.isEmpty())
   {
      BBinkInterface::BLoadParams lp;
      lp.mFilename.set(movie.getPtr());
      lp.mCaptionDirID = cDirData;
      //lp.mpStatusCallback = this;
      lp.mLoopVideo = true;
      lp.mFullScreen = true;
      mBackgroundVideoHandle = gBinkInterface.loadActiveVideo(lp);//gBinkInterface.loadActiveVideoFullScreen(movie.getPtr(), cDirData, "", "", this, NULL, true);
      BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
      if (pMode)
         pMode->setBackgroundVideoHandle(mBackgroundVideoHandle);
   }
}


//==============================================================================
int BUICampaignMenu::getNextPlayAllIndex()
{
   if (mpMoviePicker == NULL)
   {
      BASSERT(0);
      return -1;
   }
   return mpMoviePicker->getNextPlayAllIndex();
}

//==============================================================================
bool BUICampaignMenu::isMissionPickerVisible()
{
   if (!mpMissionPicker)
      return false;
   return (mpMissionPicker->getVisible());
}

//==============================================================================
void BUICampaignMenu::refreshMoviePicker()
{
   if (mpMoviePicker == NULL)
   {
      BASSERT(0);
      return;
   }
   mpMoviePicker->refresh();
}

//==============================================================================
void BUICampaignMenu::yornResult(uint result, DWORD userContext, int port)
{
   switch( userContext )
   {
      case cDialogNewCampaignTutorial:
      {
         if( result == BUIGlobals::cDialogResultOK )
         {
            BModeMenu* pModeMenu = gModeManager.getModeMenu();
            BASSERT( pModeMenu );
            if( pModeMenu )
            {
               gModeManager.setMode( BModeManager::cModeMenu );
               pModeMenu->playTutorial( BModeMenu::cTutorialBasic, BGameSettings::cGameStartContextBasicTutorialFromNewSPC );
            }
         }
         else
         {
            playCampaign(0);
         }
         break;
      }
   }
}

//============================================================================
//============================================================================
BCampaignNode* BUICampaignMenu::getDisplayNode()
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   BCampaignNode* pNode = pCampaign->getNode(pCampaign->getCurrentNodeID());
   if (pNode)
   {
      while (pNode->getFlag(BCampaignNode::cCinematic))
      {
         // get the next node.
         pNode = pCampaign->getNode(pNode->getID()+1);
         if (!pNode)
            break;
      }
   }

   return pNode;
}

//============================================================================
//============================================================================
void BUICampaignMenu::populateMenu(void)
{
   mCampaignMenu.clearControls();
   
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   bool addedNewCampaign = false;
   long n = 0;
   if (!gConfig.isDefined(cConfigDemo2))
   {
      // ajl 10/8/08 - A session ID of UINT16_MAX means the player completed the last mission of the campaign and
      // hasn't played another mission since. Once the player does play another mission they are allowed to continue
      // again from that mission.
      if (pCampaign->getCurrentNodeID() != -1 && pCampaign->getSessionID() != UINT16_MAX)
      {
         BCampaignNode* pNode = getDisplayNode();
         if (pNode)
         {
            BUString temp;
            temp.locFormat(L"%s: %s", gDatabase.getLocStringFromID(25166).getPtr(), pNode->getDisplayName().getPtr());
            addMenuItem(temp, n++, cControlContinue);
         }
         else
            addMenuItem(gDatabase.getLocStringFromID(25166), n++, cControlContinue);
      }
      else
      {
         addMenuItem(gDatabase.getLocStringFromID(24820), n++, cControlNewCampaign);
         addedNewCampaign = true;
      }

      // ajl 11/18/08 - Delay calling saveGameExists until the showLoadGameFlyout is called. The delay fixes issues where
      // the save file name shown was out of sync due to the way the device selector works in the background and sometimes
      // the file isn't available at this point.
      //saveGameExists(); // get save game info
      if (gUserManager.getPrimaryUser()->isSignedIn())
         addMenuItem(gDatabase.getLocStringFromID(25039), n++, cControlLoadGame);
   }

   if (!addedNewCampaign)
      addMenuItem(gDatabase.getLocStringFromID(24820), n++, cControlNewCampaign);

   addMenuItem(gDatabase.getLocStringFromID(25426), n++, cControlTutorial);
   addMenuItem(gDatabase.getLocStringFromID(24821), n++, cControlChangeDifficulty);
   addMenuItem(gDatabase.getLocStringFromID(24822), n++, cControlMissionSelector);

   if (!gConfig.isDefined(cConfigDemo2))
   {
      addMenuItem(gDatabase.getLocStringFromID(24823), n++, cControlCoop);
      addMenuItem(gDatabase.getLocStringFromID(24824), n++, cControlMovieSelector);
   }
   
   while( n < cMAX_MENU_ITEMS )
      mMenuItems[n++].hide();

   mCampaignMenu.setIndex( 0 );
}

//============================================================================
//============================================================================
void BUICampaignMenu::showLoadGameFlyout(void)
{
   // ajl 11/18/08 - Always refresh the save game data before the load game flyout is shown.
   saveGameExists();

   long n = 0;

   BUString text;
   int nodeID = getSaveGameMenuName(text);
   updateSecondaryMenuItem(text.getPtr(), n++, cControlLoadGameStart, true, nodeID);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25987).getPtr(), n++, cControlLoadChangeDevice, true, nodeID);

   disableSecondaryMenuItems(n);

   mSecondaryMenu.setIndex(0);
   mSecondaryMenu.show();
   displayButtons();
}

//============================================================================
//============================================================================
void BUICampaignMenu::showDifficultyFlyout(void)
{
   long n = 0;

   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25422), n++, cControlSetDifficultyEasy, true);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25423), n++, cControlSetDifficultyNormal, true);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25424), n++, cControlSetDifficultyHeroic, true);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25425), n++, cControlSetDifficultyLegendary, true);

   disableSecondaryMenuItems(n);

   int index = 0;
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (pCampaign)
      index = pCampaign->getCurrentDifficulty();

   mSecondaryMenu.setIndex( index );

   mSecondaryMenu.show();
   displayButtons();
}

//============================================================================
//============================================================================
void BUICampaignMenu::showTutorialFlyout(void)
{
   long n = 0;
   
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25427), n++, cControlCampaignTutorial, true);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(25428), n++, cControlSkirmishTutorial, true);

   disableSecondaryMenuItems(n);

   mSecondaryMenu.setIndex( 0 );
   mSecondaryMenu.show();
   displayButtons();
}

//============================================================================
//============================================================================
void BUICampaignMenu::showCoopFlyout(void)
{
   long n = 0;
   
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(24924), n++, cControlCoopLive, true);
   updateSecondaryMenuItem(gDatabase.getLocStringFromID(24926), n++, cControlCoopSysLink, true);
   
   disableSecondaryMenuItems(n);

   mSecondaryMenu.setIndex( 0 );
   mSecondaryMenu.show();
   displayButtons();
}

//==============================================================================
//==============================================================================
void BUICampaignMenu::disableSecondaryMenuItems(int startngIndex)
{
   long n = startngIndex;
   while (n < cMAX_FLYOUT_ITEMS)
   {
      updateSecondaryMenuItem(L"", n++, 0, false);
   }
}

//============================================================================
//============================================================================
bool BUICampaignMenu::updateSecondaryMenuItem(const WCHAR* menuText, int index, int controlID, bool enable, int data)
{
   BUString text;
   text.set(menuText);

   BUIControl* pControl = mSecondaryMenu.getControl(index);
   if (!pControl)
      return false;

   BUIMenuItemControl *pMenuItem = reinterpret_cast<BUIMenuItemControl*>(pControl);
   pMenuItem->setControlID(controlID);
   pMenuItem->setData(data);
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

//==============================================================================
//==============================================================================
void BUICampaignMenu::initMenuItems()
{
   for (int i=0; i<cMAX_FLYOUT_ITEMS; i++)
      addSecondaryMenuItem(L"", i, -1);
}

//==============================================================================
//==============================================================================
void BUICampaignMenu::positionMovieClip(const char* firstMC, const char* secondMC )
{
   GFxValue values[2];
   values[0].SetString(firstMC);
   values[1].SetString(secondMC);
   mpMovie->invokeActionScript("positionMovieClip", values, 2);
}


//==============================================================================
//==============================================================================
bool BUICampaignMenu::saveGameExists()
{
   mFoundSaveGame=false;

   bool result = gSaveGame.getCampaignSaveExists(NULL, &mSavegameName);

   mFoundSaveGame=result;

   return result;
}

//==============================================================================
//==============================================================================
int BUICampaignMenu::getSaveGameMenuName(BUString& name)
{
   // prefill
   name.set(gDatabase.getLocStringFromID(25981));

   if (!mFoundSaveGame)
      return -1;

   name = mSavegameName;
   return -1;
}


//============================================================================
//============================================================================
void BUICampaignMenu::updateSaveGameData()
{
   // requery the save game data
   saveGameExists();

   // pull down the save game menu if it's up.
   BUIMenuItemControl * pMI = (BUIMenuItemControl*)mCampaignMenu.getSelectedControl();
   if ( pMI && mSecondaryMenu.isShown() && (pMI->getControlID() == cControlLoadGame) )
      mSecondaryMenu.hide();

}


//============================================================================
// returns true if the current selected device is valid, otherwise, pops a yorn box and goes on.
// 
//============================================================================
bool BUICampaignMenu::checkSaveDevice()
{
   XCONTENTDEVICEID deviceID = gUserManager.getPrimaryUser()->getDefaultDevice();
   if( deviceID != XCONTENTDEVICE_ANY )
      return true;

   // the device ID is not valid, but we are not going to check unless they removed the device. 
   // They may have chosen to enter this screen without selecting a device or they may have no device on the box.
   if (!gUserManager.getPrimaryUser()->isDeviceRemoved())
      return true;

   /* ajl 11/18/08 - This was causing undesired behaviour when the user had two storage devices with save files and
      removed the default one where a message would display stating that progress would not be saved, but it actually
      would be saved because this was auto-selecting the other storage device. So commenting this out so that the
      user will have to choose the new device from the device selector instead.

   // see if we can auto detect the save storage device.
   deviceID = gSaveGame.autoSelectStorageDevice();
   if (deviceID != XCONTENTDEVICE_ANY)
   {
      gUserManager.getPrimaryUser()->setDefaultDevice(deviceID);
      // ajl 11/18/08 - Delay calling saveGameExists until the showLoadGameFlyout is called. The delay fixes issues where
      // the save file name shown was out of sync due to the way the device selector works in the background and sometimes
      // the file isn't available at this point.
      //saveGameExists();
      return true;
   }
   */

   // They have no device because they pulled it and we couldn't auto select a device, so we are going to prompt them.
   gGame.getUIGlobals()->showWaitDialog(gDatabase.getLocStringFromID(25989));
   BModeCampaign2* pModeCampaign = (BModeCampaign2*)gModeManager.getMode( BModeManager::cModeCampaign2 );
   if (!gUserManager.showDeviceSelector( gUserManager.getPrimaryUser(), pModeCampaign, 0, 0, true ))
      gGame.getUIGlobals()->setWaitDialogVisible(false);
   mSecondaryMenu.hide();

   return false;
}

