//============================================================================
// UIMPSetupScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UISkirmishSetupScreen.h"

#include "UIMenuItemControl.h"

#include "database.h"
#include "gamemode.h"
#include "modemanager.h"
#include "modemenu.h"

#include "configsgame.h"
#include "econfigenum.h"

#include "campaignmanager.h"
#include "humanPlayerAITrackingData.h"

#include "user.h"
#include "usermanager.h"

#include "leaders.h"
#include "uiMenu.h"
#include "vincehelper.h"
#include "gamesettings.h"

#include "game.h"
#include "UIGlobals.h"

#include "liveSystem.h"
#include "GamerPicManager.h"

#define AUTO_FILL_PLAYERS
#define SKIP_ITEM_FIX

//==============================================================================
// BUISkirmishSetupScreen::BUISkirmishSetupScreen
//==============================================================================
BUISkirmishSetupScreen::BUISkirmishSetupScreen() : 
mGridControl(3, 2),
mLastBumperKeyInputTime(0),
mLastViewChangeTime(0),
mpGamesList(NULL),
mDoRefresh(false),
mLastGamerPicUpdate(0)
{
   mLastMapImage.set("");
}

//==============================================================================
// BUISkirmishSetupScreen::~BUISkirmishSetupScreen
//==============================================================================
BUISkirmishSetupScreen::~BUISkirmishSetupScreen()
{
   gUserManager.removeListener(this);

   // clean up the class
   if (mpGamesList)
   {
      delete mpGamesList;
      mpGamesList=NULL;
   }

   BUIMenuItemControl* c = NULL;
   for (int i=0; i<mMenuItems.getNumber(); i++)
   {
      c = mMenuItems[i];
      delete c;
      mMenuItems[i]=NULL;
   }
   mMenuItems.clear();


   for (int i=0; i<mPlayerMenuItems.getNumber(); i++)
   {
      c = mPlayerMenuItems[i];
      delete c;
      mPlayerMenuItems[i]=NULL;
   }
   mPlayerMenuItems.clear();



   for (int i=0; i<mSecondaryMenuItems.getNumber(); i++)
   {
      c = mSecondaryMenuItems[i];
      delete c;
      mSecondaryMenuItems[i]=NULL;
   }
   mSecondaryMenuItems.clear();

   if (mpLeaderPicker)
   {
      mpLeaderPicker->deinit();
      delete mpLeaderPicker;
      mpLeaderPicker=NULL;
   }

}

//============================================================================
//============================================================================
bool BUISkirmishSetupScreen::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   // reset all the slot states to a default state
   resetSlotStates();

   mpLeaderPicker=NULL;

   if (mpLeaderPicker == NULL)
   {
      mpLeaderPicker = new BUILeaderPicker();
      mpLeaderPicker->init("art\\ui\\flash\\pregame\\leaderPicker\\LeaderPicker.gfx", "art\\ui\\flash\\pregame\\leaderPicker\\LeaderPickerData.xml");
      mpLeaderPicker->setEventHandler(this);
   }

   setInitialValues();

   // initialize all the slot transition animations
   mSlotKeyFrames.setNumber(cPRSlotKF_TransitionCount);

   mSlotKeyFrames[cPRSlotKF_Empty].set("on");
   mSlotKeyFrames[cPRSlotKF_EaseIn].set("ease_in");      // entry transition
   mSlotKeyFrames[cPRSlotKF_Normal].set("normal");       // State
   mSlotKeyFrames[cPRSlotKF_Center].set("center");       // state
   mSlotKeyFrames[cPRSlotKF_N2C].set("N2C");             // Normal to Center
   mSlotKeyFrames[cPRSlotKF_C2N].set("C2N");             // Center to Normal

   // Lan Browser
   if (mpGamesList==NULL)
   {
      mpGamesList=new BUIMenu();
      mpGamesList->init(getMovie(), "art\\ui\\flash\\pregame\\partyroom\\GameListMenuInput.xml");
      mpGamesList->setMovieClip("mLanGameListContainer");
      //mpGamesList->setEventHandler(this);
      mpGamesList->setUseSelectButton(false);
      mpGamesList->clearItemFocus();
      mpGamesList->hide();
   }

   // --------- PRIMARY MENU
   mImageViewer.init(this, "mImageViewer", 0, NULL);
   mImageViewer.setAutoTransition(true);
   mImageViewer.setViewDuration(9000.0f);
   mImageViewer.setImageSize(700, 350);

   mImageViewer.start();

   mHelpText.init(this, "mHelpText");


   // --------- PRIMARY MENU
   mMainMenu.init( this, "", cControlIDMainMenu, NULL );
   //mMainMenu.setIndex(-1);

   BSimString controlPath;
   BUString text;
   text.set(L" ");
   for (int i=0; i<cMaxMainMenuItems; i++)
   {
      controlPath.format("mMainMenu%d", i);

      BUIMenuItemControl *pMenuItem = new BUIMenuItemControl();
      pMenuItem->init( &mMainMenu, controlPath.getPtr(), -1);
      pMenuItem->setText(text);

      mMainMenu.addControl(pMenuItem);
      mMenuItems.add(pMenuItem);
   }
   mMainMenu.setIndex(0);

   // --------- SECONDARY FLYOUT MENU
   BXMLNode controlsNode;
   if (dataNode.getChild( "UIListControlCustom", &controlsNode ) )
      mSecondaryMenu.init( this, "mSecondaryMenuList", cControlIDSecondaryMenu, &controlsNode );
   else
   {
      BASSERTM(false, "UIMPSetupScreen:: Unable to initialize mSecondaryMenuList control input handler.");
      mSecondaryMenu.init( this, "mSecondaryMenuList", cControlIDSecondaryMenu, NULL );
   }
   mSecondaryMenu.setIndex(-1);
   mSecondaryMenu.hide();
   mSecondaryMenu.setWrap(true);

   text.set(L" ");
   for (int i=0; i<cMaxSecondaryMenuItems; i++)
   {
      controlPath.format("mSecondaryMenuList.mMenu%d", i);

      BUIMenuItemControl *pMenuItem = new BUIMenuItemControl();
      pMenuItem->init( &mSecondaryMenu, controlPath.getPtr(), -1);
      pMenuItem->setText(text);

      mSecondaryMenu.addControl(pMenuItem);
      mSecondaryMenuItems.add(pMenuItem);
   }

   // --------- PLAYERS POPUP MENU
   if (dataNode.getChild( "UIListControlCustom", &controlsNode ) )
      mPlayerMenu.init( this, "mPlayerMenuList", cControlIDPlayerMenu, &controlsNode );
   else
   {
      BASSERTM(false, "UIMPSetupScreen:: Unable to initialize mPlayerMenuList control input handler.");
      mPlayerMenu.init( this, "mPlayerMenuList", cControlIDPlayerMenu, NULL );
   }
   mPlayerMenu.setIndex(-1);
   mPlayerMenu.hide();

   mPlayerMenu.setWrap(true);

   text.set(L" ");
   for (int i=0; i<cMaxPlayerMenuItems; i++)
   {
      controlPath.format("mPlayerMenuList.mMenu%d", i);

      BUIMenuItemControl *pMenuItem = new BUIMenuItemControl();
      pMenuItem->init( &mPlayerMenu, controlPath.getPtr(), -1);
      pMenuItem->setText(text);

      mPlayerMenu.addControl(pMenuItem);
      mPlayerMenuItems.add(pMenuItem);
   }


   // --------- PLAYERS AND GRID CONTROL
   // Players
   BString temp;
   for (int i=0; i<cPlayerSlotCount; i++)
   {
      temp.format("mPlayer%d", i);
      mPlayerControls[i].init(this, temp.getPtr());
   }

   // now initialize the handler for the player list
   mGridControl.init(this, "", -1, NULL);
   int index=0;
   for (int col=0; col<2; col++)
   {
      for (int row=0; row<3; row++)
      {
         mGridControl.setControl(&(mPlayerControls[index]), row, col);
         index++;
      }
   }

   mGridControl.setSelectedCell(-1, -1);

   // --------- BUTTON BAR
   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   mLastBumperKeyInputTime = 0;

   mTitle.init(this, "mTitle");

   text.set(L"");
   mHelpText.setText(text);

   mTeamAlphaLabel.init(this, "alphaField");
   mTeamBravoLabel.init(this, "bravoField");

   mTeamAlphaLabel.setText(gDatabase.getLocStringFromID(25296));
   mTeamBravoLabel.setText(gDatabase.getLocStringFromID(25297));

   setPlayerSlotsVisible(mNumPlayers, true);

   initHostPlayer();

   populate();

   gUserManager.addListener(this);

   return true;
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::initHostPlayer()
{
   BUser* user=gUserManager.getPrimaryUser();

   setPlayer(user, 1, 1, 0, 1);


   if (gGame.isSplitScreenAvailable())
   {
      user=gUserManager.getSecondaryUser();
      if (user->getFlagUserActive())
      {
         int slot = getFirstOpenSlot(true);
         if (isValidSlot(slot))
            setPlayer(user, 1, 1, slot, 2);
      }
   }
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::populate()
{
   // populate the title, etc
   setTitle(gDatabase.getLocStringFromID(25032));

   // populate the menu
   populateMenu();

   // populate the map information.
   updateMap();
   
   // populate the player list
   updatePlayerList();

   updateHelp();
   displayButtons();
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::updatePlayerList()
{
   // fixme - add stuff here.
   refreshPlayerSlots();
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::updateMap()
{
   const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mMapIndex);
   setImage(pMap->getMapKeyFrame().getPtr());
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::populateMenu()
{
   // fixme - if we handle more than one, then make that decision here.
   populateCustomModeMenu();
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   BString temp;
   switch ( control->getControlTypeID() )
   {
      case UIListControlID:
         switch (control->getControlID())
         {
            case cControlIDSecondaryMenu:
               {
                  if (event.getString()=="cancel")
                  {
                     gUI.playCancelSound();
                     setView(BUISkirmishSetupScreen::cPRViewMenu);
                     mSecondaryMenu.hide();
                     return true;
                  }

                  // Extract the wrapped event
                  const BUIControlEvent* childEvent = event.getChildEvent();
                  if (!childEvent)
                     return true;

                  if (childEvent->getString()!="accept")
                     return true;

                  gUI.playConfirmSound();

                  BUIControl * c = childEvent->getControl();
                  if (!c)
                     return true;        // can't do anything with this.

                  if (c->getControlTypeID() != UIMenuItemControlID)
                     return true;

                  BUIMenuItemControl* menuItemControl = (BUIMenuItemControl*)c;

                  getSettingsChangeFromSecondaryMenuItem(menuItemControl);

                  setView(BUISkirmishSetupScreen::cPRViewMenu);
                  mSecondaryMenu.hide();
                  return true;
               }
               break;

            case cControlIDPlayerMenu:
               {
                  if (event.getString()=="cancel")
                  {
                     gUI.playCancelSound();
                     setView(BUISkirmishSetupScreen::cPRViewPlayerList);      
                     mPlayerMenu.hide();
                     return true;
                  }

                  // Extract the wrapped event
                  const BUIControlEvent* childEvent = event.getChildEvent();
                  if (!childEvent)
                     return true;

                  if (childEvent->getString()!="accept")
                     return true;

                  BUIControl * c = childEvent->getControl();
                  if (!c)
                     return true;        // can't do anything with this.

                  if (c->getControlTypeID() != UIMenuItemControlID)
                     return true;

                  gUI.playConfirmSound();

                  // BUIMenuItemControl* menuItemControl = (BUIMenuItemControl*)c;

                  // uint8 slot = (int8)menuItemControl->getData();
                  switch (c->getControlID())
                  {
                     case cPlayerMenuCommandChangeSettings:
                        {
                           // push back to the player list view (this is so an accept, cancel gets fixed up properly in re. views)
                           setView(BUISkirmishSetupScreen::cPRViewPlayerList);      
                           mPlayerMenu.hide();
                           editPlayer(getPlayerCurrentSlotFocus());
                           // fixme - edit player
                        }
                        break;
                     case cPlayerMenuCommandViewGamerCard:
                        {
                           setView(BUISkirmishSetupScreen::cPRViewPlayerList);      
                           mPlayerMenu.hide();
                           // XShowGamerCardUI(gUserManager.getPrimaryUser()->getPort(), pMember->mXuid);
                        }
                        break;
                     case cPlayerMenuCommandAddAI:
                        {
                           // fixme - add an AI player
                           addAI(getPlayerCurrentSlotFocus());
                           setView(BUISkirmishSetupScreen::cPRViewPlayerList);      
                           refreshPlayerSlots();
                           mPlayerMenu.hide();
                        }
                        break;
                     case cPlayerMenuCommandKick:
                     case cPlayerMenuCommandKickAI:
                        {
                           // fixme - kick player (for AI, as simple as turning the slot to empty)
                           kickPlayer(getPlayerCurrentSlotFocus());
                           setView(BUISkirmishSetupScreen::cPRViewPlayerList);      
                           mPlayerMenu.hide();
                        }
                        break;
                  }

               }
               break;

            case cControlIDMainMenu:
               {
                  //Added because if you went to the player area and back up, that help text never updated - eric
                  updateHelp();

                  if (event.getID() == BUIListControl::eStopEnd)
                  {
                     setView(BUISkirmishSetupScreen::cPRViewPlayerList);

                     #ifdef SKIP_ITEM_FIX
                        // Need to tell the grid control to ignore the next down input event otherwise an item will get skipped 
                        // due to how the contexes are separate between controls and the repeat time won't be set correctly.
                        mGridControl.setIgnoreNextDown(true);
                     #endif
                     
                     // change the focus to slot 0
                     setPlayerCurrentSlotFocus(0);
                     refreshPlayerSlots();
                     mMainMenu.setIndex(-1);                     
                     
                     // resetPlayerInput();
                     return true;
                  }

                  // see if it was one of the children

                  // Extract the wrapped event
                  const BUIControlEvent* childEvent = event.getChildEvent();
                  if (!childEvent)
                     return true;

                  if (childEvent->getString()!="accept")
                     return true;

                  BUIControl * c = childEvent->getControl();
                  if (!c)
                     return true;        // can't do anything with this.

                  if (c->getControlTypeID() != UIMenuItemControlID)
                     return true;

                  gUI.playConfirmSound();

                  switch (c->getControlID())
                  {
                     // call menus
                     case cMenuCommandSetLobby:
/*
                        populateLobbyMenu();
                        setView(BUISkirmishSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
*/
                        break;
                     // custom menu

                     case cMenuCommandSetMap:
                        populateRandomMapMenu();
                        setView(BUISkirmishSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        break;
                     case cMenuCommandSetPlayerCount:
                        populatePlayerCountMenu();
                        setView(BUISkirmishSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        break;
                     case cMenuCommandSetTeamType:
                        populateTeamMenu();
                        setView(BUISkirmishSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        break;
                     case cMenuCommandSetGameMode:
                        populateGameModes();
                        setView(BUISkirmishSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        break;
                     case cMenuCommandSetAIDifficulty:
                        populateDifficultyMenu();
                        setView(BUISkirmishSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        break;

                     // Campaign
/*
                     case cMenuCommandSetCampaignMission:
                        populateCampaignMissionPicker();
                        setView(BUISkirmishSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        break;
*/
                  }
               }
               break;
         }
         break;
      case UIGridControlID:
         {
            switch (event.getID())
            {
               case BUIGridControl::eStopTop:
                  {
                     setView(BUISkirmishSetupScreen::cPRViewMenu);

                     #ifdef SKIP_ITEM_FIX
                        // Need to tell the list control to ignore the next prev input event otherwise an item will get skipped 
                        // due to how the contexes are separate between controls and the repeat time won't be set correctly.
                        mMainMenu.setIgnoreNextPrev(true);
                     #endif

                     updateHelp();
                     displayButtons();
                     setPlayerCurrentSlotFocus(-1);
                     refreshPlayerSlots();

                     for (int i=mMainMenu.getControlCount()-1; i>=0; i--)
                     {
                        BUIControl * c = mMainMenu.getControl(i);
                        if (c && c->isShown() && c->isEnabled())
                        {
                           mMainMenu.setIndex(i);
                           break;
                        }
                     }
                  }
                  return true;
                  break;

               case BUIGridControl::eSelectionChanged:
                  int row = -1;
                  int col = -1; 
                  if (!mGridControl.getSelectedCell(row, col))
                     break;

                  int slot = (3*col)+row;
                  setPlayerCurrentSlotFocus(slot);
                  refreshPlayerSlots();            // fixme - necessary?
                  updateHelp();
                  displayButtons();
                  break;
            }
         }      
      break;
   }

   return false;
}

//============================================================================
//============================================================================
bool BUISkirmishSetupScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
/*

<Control name="ButtonA"             command="playerOptions" event="start" sound="" />
<Control name="ButtonB"             command="cancel" event="start" sound="" />
<Control name="ButtonX"             command="ready" event="start" sound="" />
<Control name="ButtonY"             command="inviteFriends" event="start" sound="" />
<Control name="ButtonBack"          command="kick" event="start" sound="" />

<Control name="TriggerLeft"         command="decrementLeader" event="stop" repeat="" delay="0.5" rate="1" sound="" />
<Control name="TriggerRight"        command="incrementLeader" event="stop" repeat="" delay="0.5" rate="1" sound="" />
<Control name="ButtonShoulderLeft"  command="changeTeamLeft" event="start" sound=""/>
<Control name="ButtonShoulderRight" command="changeTeamRight" event="start" sound=""/>

   if (command=="cancel")
   else if (command=="playerOptions")
   else if (command=="ready")
   else if (command=="inviteFriends")
   else if (command=="kick")
   else if (command=="decrementLeader")
   else if (command=="incrementLeader")
   else if (command=="changeTeamLeft")
   else if (command=="changeTeamRight")
*/



   BString temp;
   bool handled = false;
   // -------------------- EVENT - inviteFriends ---------------------------
   
   // -------------------- EVENT - incrementLeader --------------------
   if (command=="incrementLeader")                
   {
/*
      removed for the time being because of confusion.
      if (event == cInputEventCommandRepeat)
      {
         editPlayer(getPlayerSlotByPort(port));
         handled=true;
      }
      else
      {
         incrementLeader(getPlayerSlotByPort(port));
         handled=true;
      }
*/
   }
   // -------------------- EVENT - decrementLeader --------------------
   else if (command=="decrementLeader")                
   {
/*
      removed for the time being because of confusion.
      if (event == cInputEventCommandRepeat)
      {
         editPlayer(getPlayerSlotByPort(port));
         handled=true;
      }
      else
      {
         decrementLeader(getPlayerSlotByPort(port));
         handled=true;
      }
*/
   }   
   // -------------------- EVENT - changeTeamLeft --------------------
   else if (command=="changeTeamLeft")                
   {
      if (timeGetTime()-100>mLastBumperKeyInputTime)
      {
         mLastBumperKeyInputTime = timeGetTime();
         //mpMode->changeTeamLeft();
         // fixme - change team
      }
      handled = true;
   }
   // -------------------- EVENT - changeTeamRight --------------------
   else if (command=="changeTeamRight")                
   {
      if (timeGetTime()-100>mLastBumperKeyInputTime)
      {
         mLastBumperKeyInputTime = timeGetTime();
         //mpMode->changeTeamRight();
         // fixme - change team
      }
      handled = true;
   }
   // -------------------- EVENT - kick --------------------
   else if (command == "kick")                
   {
      //mpMode->kickPlayer();
      // fixme - kick player
      handled = true;
   }
   // -------------------- EVENT - ready --------------------
   else if (command == "ready")                
   {
      if (!canStartGame(true))
      {
         BUIGlobals* puiGlobals = gGame.getUIGlobals();
         puiGlobals->showYornBox(this, gDatabase.getLocStringFromID(25219), BUIGlobals::cDialogButtonsOK);

         return false;
      }

      gUI.playConfirmSound();

      mSecondaryMenu.hide();
      startGame();

      handled = true;
   }
   // -------------------- EVENT - cancel --------------------
   else if (command == "cancel")                
   {
      gUI.playCancelSound();
      // quit the screen.
      // hack for now to get it to work
      if( mpHandler )
      {
         mpHandler->handleUIScreenResult( this, 0 );
      }
      handled = true;
   }
   // -------------------- EVENT - playerOptions --------------------
   else if (command == "playerOptions")                
   {
      gUI.playConfirmSound();
      #ifdef AUTO_FILL_PLAYERS
         editPlayer(getPlayerCurrentSlotFocus());
      #else
         setView(BUISkirmishSetupScreen::cPRViewPlayerMenu);
         populatePlayerMenu(getPlayerCurrentSlotFocus());
         mPlayerMenu.show();
      #endif
   }
   return handled;
}

//==============================================================================
// BUISkirmishSetupScreen::handleInput
//==============================================================================
int BUISkirmishSetupScreen::getFirstOpenSlot(bool bTeam)
{
   if (bTeam)
   {
      int count=mNumPlayers/2;

      for (int i=0; i<cPRNumPlayersSlotsPerTeam; i++)
      {
         if (i>=count)
            break;

         // check the user slots
         if (isSlotEmpty(i))
            return i;
         if (isSlotEmpty(i+3))
            return i+3;
      }
   }
   else
   {
      // just do a straight show of the slots
      for (int i=0; i<cPRNumPlayerSlots; i++)
      {
         if (i>=mNumPlayers)
            break;

         if (isSlotEmpty(i))
            return i;
      }
   }

   return -1;
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::checkSecondaryUser(long port, long event, long controlType, BInputEventDetail& detail)
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
               // do we have a slot open?
               int slot = getFirstOpenSlot(true);
               if (slot<0)
               {
                  // fixme - maybe pop a note up here?
                  return false;
               }

               // let's hook this guy up.
               gUserManager.setUserPort(BUserManager::cSecondaryUser, port);
               pSecondaryUser->setFlagUserActive(true);

               setPlayer(pSecondaryUser, 1, 1, slot, 2);

               refreshPlayerSlots();

               // we eat the input
               return true;
            }
         case cButtonB:
         case cButtonX:
         case cButtonY:
            {
               // fixme - throw a message up there.
               break;
            }

      }
   }


   return false;

}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   BUser* pSecondarUser = gUserManager.getSecondaryUser();

   if (gGame.isSplitScreenAvailable())
   {
      // check for secondary user controller
      if (mPartyRoomView != cPRViewPlayerEdit)
      {
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
      if (pSecondarUser && !pSecondarUser->getFlagUserActive())
         return true;      // eat it.

      if (pSecondarUser && pSecondarUser->getPort() != port)
         return true;      // eat it.
   }

   //Dirty E3 hack to stop the double up/down tap when you switch between modes - eric
   if (((controlType==cStickLeftUp) ||
        (controlType==cStickLeftDown) ||
        (controlType==cDpadUp) ||
        (controlType==cDpadDown)) && 
      (timeGetTime()-mLastViewChangeTime < 200))
   {
      return true;
   }

   switch(mPartyRoomView)
   {
      case BUISkirmishSetupScreen::cPRViewPlayerEdit:
         if (mpLeaderPicker)
            handled = mpLeaderPicker->handleInput(port, (BInputEventType)event, (BInputControlType)controlType, detail);
         return handled;
         break;
      case BUISkirmishSetupScreen::cPRViewMenu:
         // let the screen catch it's globals first
         handled = mMainMenu.handleInput(port, event, controlType, detail);
         break;
      case BUISkirmishSetupScreen::cPRViewSubMenu:
         handled = mSecondaryMenu.handleInput(port, event, controlType, detail);
         break;
      case BUISkirmishSetupScreen::cPRViewPlayerMenu:
         handled = mPlayerMenu.handleInput(port, event, controlType, detail);
         break;
      case BUISkirmishSetupScreen::cPRViewPlayerList:
         handled = mGridControl.handleInput(port, event, controlType, detail);
         if (handled)
            return handled;
         break;
   }

   if (!handled)
      handled = __super::handleInput(port, event, controlType, detail);

   if (handled)
      return handled;

   return false;
}



//==============================================================================
// BUISkirmishSetupScreen::setImage
//==============================================================================
void BUISkirmishSetupScreen::setImage(const char * imageURL)
{
   if (mLastMapImage.compare(imageURL) == 0)
      return;

   mLastMapImage.set(imageURL);

   mImageViewer.clearImages();
   mImageViewer.addImage(imageURL);
   mImageViewer.start();
}

//==============================================================================
// BUISkirmishSetupScreen::setTitle
//==============================================================================
void BUISkirmishSetupScreen::setTitle(const BUString& title)
{
   mTitle.setText(title);
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::positionMovieClip(const char* firstMC, const char* secondMC )
{
   GFxValue values[2];
   values[0].SetString(firstMC);
   values[1].SetString(secondMC);
   mpMovie->invokeActionScript("positionMovieClip", values, 2);
}


//==============================================================================
// BUISkirmishSetupScreen::setHelpText
//==============================================================================
void BUISkirmishSetupScreen::setHelpText(const BUString& helpText)
{
   mHelpText.setText(helpText);
}


//==============================================================================
void BUISkirmishSetupScreen::setPlayerCurrentSlotFocus(int newSlot)
{
   // break the slot into a row, column
   int col = (int)(newSlot/3);
   int row = newSlot%3;

   // set the focus on the new player slot
   mGridControl.setSelectedCell(row,col);
}

/*
//==============================================================================
void BUISkirmishSetupScreen::setPlayerNavigationEnabled(bool value)
{
   mGridControl.setAllowNavigation(value);
}
*/

//==============================================================================
BUIPlayer* BUISkirmishSetupScreen::getPlayer(int slot)
{
   if ( (slot<0) || (slot>=cPlayerSlotCount) )
      return NULL;

   return &mPlayers[slot];
}

//==============================================================================
void BUISkirmishSetupScreen::setPlayerSlotActive(int slot, bool active)
{
   // update its state
   mPlayers[slot].mActive=active;

   // turn it on or off
   //setPlayerSlotVisible(slot, active);

   BUIGameSetupPlayerControl& player = mPlayerControls[slot];
   if (active)
      player.show();
   else
      player.hide();
}

//==============================================================================
//==============================================================================
int BUISkirmishSetupScreen::getPlayerSlotByPort(int port)
{
   int slot = -1;
   for (int i=0; i<cPlayerSlotCount; i++)
   {
      if (mPlayers[i].mSlotType == BUIPlayer::cEmpty)
         continue;

      if (mPlayers[i].mControllerPort != port)
         continue;

      slot = i;
      break;
   }

   return slot;
}

//==============================================================================
int BUISkirmishSetupScreen::getPlayerCurrentSlotFocus()
{
   int row=-1;
   int col=-1;
   if (mGridControl.getSelectedCell(row, col))
   {
      int slot = (3*col)+row;
      return slot;
   }
   return -1;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::setPlayerSlotData(int slot)
{
   BUIPlayer& player = mPlayers[slot];

   BUIGamerTagLongControl& playerControl = mPlayerControls[slot].getGamerTag();

   playerControl.setGamerTag(player.mGamerTag);
   playerControl.setPing(player.mPing);

   // [10/21/2008 xemu] specify the gamer picture lookup by XUID 
   playerControl.setGamerPic(player.mGamerPic);

   playerControl.setRank(player.mRank);
   playerControl.setPort(player.mControllerPort);

   // leader info
   BLeader* pLeader=gDatabase.getLeader(player.mLeader);
   if (pLeader)
   {
      playerControl.setLeaderPic(pLeader->getUIBackgroundImage());
      playerControl.setTitleText(gDatabase.getLocStringFromIndex(pLeader->mNameIndex));
   }
   else
   {
      // fixme - this needs to be data driven.
      BString tempCivURL;
      BUString tempLeaderString;
      if (player.mCiv==-1)
      {
         // full random
         tempCivURL.set("img://art\\ui\\flash\\shared\\textures\\leaders\\leader_pict_random.ddx");
         tempLeaderString.set(gDatabase.getLocStringFromID(23722));     // random
      }
      else if (player.mCiv==gDatabase.getCivID("UNSC"))
      {
         // random UNSC
         tempCivURL.set("img://art\\ui\\flash\\shared\\textures\\leaders\\LeaderPict_random_unsc.ddx");
         tempLeaderString.set(gDatabase.getLocStringFromID(25343));     // random unsc
      }
      else if (player.mCiv==gDatabase.getCivID("Covenant"))
      {
         // Random covenant
         tempCivURL.set("img://art\\ui\\flash\\shared\\textures\\leaders\\LeaderPict_random_cov.ddx");
         tempLeaderString.set(gDatabase.getLocStringFromID(25344));     // random covenant
      }
      playerControl.setLeaderPic(tempCivURL);
      playerControl.setTitleText(tempLeaderString);


   }

   playerControl.setHost(player.mHost);
   playerControl.setReady(player.mReady);
}


//==============================================================================
// BModePartyRoom2::populatePlayerMenu();
//==============================================================================
void BUISkirmishSetupScreen::populatePlayerMenu(int slot)
{
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   if ( (slot<0) || (slot>=cPlayerSlotCount) )
   {
      BASSERT(0);
      return;
   }

   // Grab the slot player
   BUIPlayer& player = mPlayers[slot];
   if (!player.mActive)
      return;


   if (player.mSlotType == BUIPlayer::cEmpty)
   {
      // no person in the slot
      menuText.set(gDatabase.getLocStringFromID(25311)); 
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cPlayerMenuCommandAddAI);     // command id
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->setData(slot);
      pMenuItem->enable();
      pMenuItem->show();
   }
   else if (player.mSlotType == BUIPlayer::cAI)
   {
      // AI in the slot
      menuText.set(gDatabase.getLocStringFromID(25312)); 
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cPlayerMenuCommandKickAI);     // command id
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->setData(slot);
      pMenuItem->enable();
      pMenuItem->show();

      menuText.set(gDatabase.getLocStringFromID(25313)); 
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cPlayerMenuCommandChangeSettings);     // command id
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->setData(slot);
      pMenuItem->enable();
      pMenuItem->show();
   }
   else // human is in the slot
   {
      // if not primary user, then we can drop this guy
      if (isUserType(slot, BUserManager::cSecondaryUser))
      {
         menuText.set(gDatabase.getLocStringFromID(25312)); 
         pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
         pMenuItem->setControlID(BUISkirmishSetupScreen::cPlayerMenuCommandKick);     // command id
         helpText.set(L"");
         pMenuItem->setHelpText(helpText);
         pMenuItem->setText(menuText);
         pMenuItem->setData(slot);
         pMenuItem->enable();
         pMenuItem->show();
      }

      // me in the slot
      menuText.set(gDatabase.getLocStringFromID(25313)); 
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cPlayerMenuCommandChangeSettings);     // command id
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->setData(slot);
      pMenuItem->enable();
      pMenuItem->show();
   }

   // clear out all the rest of menu items:
   for (int i=index; i<cMaxPlayerMenuItems; i++)
   {
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(i);
      pMenuItem->setData(slot);
      pMenuItem->disable();
      pMenuItem->hide();
   }

   mPlayerMenu.setIndex(0);
}

//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::isUserType(int slot, int userType)
{
   if (!isValidSlot(slot))
      return false;

   BUser* pSlotUser = gUserManager.getUserByPort(mPlayers[slot].mControllerPort);
   if (!pSlotUser)
      return false;

   BUser* pUser = gUserManager.getUser(userType);
   if (pUser != pSlotUser)
      return false;

   return true;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::populateLobbyMenu()
{
/*
   int menuFocus=0;

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;

   // Item: custom lobby  
   if (pHostSettings->mPartyRoomMode == BModePartyRoom2::cPartyRoomModeCustom)
      menuFocus = index;
   menuText=gDatabase.getLocStringFromID(23456);   // "Custom";
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetLobby);        // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(BModePartyRoom2::cPartyRoomModeCustom);              // The data
   pMenuItem->enable();
   pMenuItem->show();

   if (!gConfig.isDefined(cConfigDemo))
   {
      // Item3: campaign lobby
      if (pHostSettings->mPartyRoomMode == BModePartyRoom2::cPartyRoomModeCampaign)
         menuFocus = index;

      menuText=gDatabase.getLocStringFromID(23458);   // Campaign
      pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetLobby);        // The command 
      pMenuItem->setText(menuText);
      pMenuItem->setData(BModePartyRoom2::cPartyRoomModeCampaign);            // The data
      pMenuItem->enable();
      pMenuItem->show();
   }


   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }

   mSecondaryMenu.setIndex(menuFocus);
*/

}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::populateRandomMapMenu()
{
   // get the current map so we can set the proper focus after we show
   int currentMap = mMapIndex;

   int validMapCount=0;
   for (int i=0; i<gDatabase.getScenarioList().getMapCount(); i++)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(i);
      BASSERT(pMap);
      if (!isValidMapType(pMap->getType()))
         continue;

      if (!isValidMap(i, mNumPlayers))
         continue;

      validMapCount++;
   }


//----------------------
   // Now actually populate the menu
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   int itemNumber=0;
   int itemWithFocus=0;
   for (int i=0; i<gDatabase.getScenarioList().getMapCount(); i++)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(i);
      BASSERT(pMap);
      if (!isValidMapType(pMap->getType()))
         continue;

      if (!isValidMap(i, mNumPlayers))
         continue;

      // Set up the item.
      if (currentMap == i)
         itemWithFocus = itemNumber;


      if (pMap->getMapName().length() > 0)
         menuText.set(pMap->getMapName().getPtr());
      else
      {
         BSimString mapFilename;
         strPathGetFilename(pMap->getFilename(), mapFilename);

         // use the filename itself
         menuText.set(mapFilename.getPtr());
      }

      if (itemNumber>=mSecondaryMenu.getControlCount())
         break;

      pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(itemNumber);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetMap);       // The command 
      pMenuItem->setText(menuText);
      pMenuItem->setData(i);                                               // index to the map in the scenario list
      pMenuItem->enable();
      pMenuItem->show();

      // prep for the next number
      itemNumber++;

      if (itemNumber == validMapCount)
         break;
   }

   mSecondaryMenu.setIndex(itemWithFocus);

   // hide the rest
   for (int i=itemNumber; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::populateDifficultyMenu()
{
   int index = 0;
   int focus = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;

   if (mDifficulty==DifficultyType::cEasy)
      focus = index;

   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cEasy));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cEasy);               // The data
   pMenuItem->enable();
   pMenuItem->show();


   if (mDifficulty==DifficultyType::cNormal)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cNormal));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cNormal);               // The data
   pMenuItem->enable();
   pMenuItem->show();


   if (mDifficulty==DifficultyType::cHard)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cHard));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cHard);               // The data
   pMenuItem->enable();
   pMenuItem->show();


   if (mDifficulty==DifficultyType::cLegendary)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cLegendary));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cLegendary);               // The data
   pMenuItem->enable();
   pMenuItem->show();

/*
   if (mDifficulty==DifficultyType::cCustom)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cCustom));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cCustom);               // The data
   pMenuItem->enable();
   pMenuItem->show();
*/


   if (mDifficulty==DifficultyType::cAutomatic)
      focus = index;

   BHumanPlayerAITrackingData trackingData;
 
   int autoDiffLevel = 0;
   BUString autoText;
   autoText.empty();

   if (trackingData.loadValuesFromMemoryBlock(gUserManager.getPrimaryUser()->getProfile()->getAITrackingDataMemoryPointer()))
   {
      autoDiffLevel = trackingData.getAutoDifficultyLevel();
      autoText.locFormat(gDatabase.getLocStringFromID(25976).getPtr(), gDatabase.getDifficultyStringByType(DifficultyType::cAutomatic).getPtr(), autoDiffLevel);
   }
   else
   {
      autoText.set(gDatabase.getDifficultyStringByType(DifficultyType::cAutomatic));
   }
   menuText.set(autoText);

   //menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cAutomatic));

   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cAutomatic);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   mSecondaryMenu.setIndex(focus);

   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }
}

//==============================================================================
// BModePartyRoom2::
//==============================================================================
void BUISkirmishSetupScreen::populateGameModes()
{
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   int count = gDatabase.getNumberGameModes();

   for (int i=0; i<count; i++)
   {
      BGameMode* gameMode = gDatabase.getGameModeByID(i);
      if (!gameMode)
         continue;

      menuText=gameMode->getDisplayName();
      pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetGameMode);     // The command 
      pMenuItem->setText(menuText);
      pMenuItem->setData(i);               // The data
      pMenuItem->enable();
      pMenuItem->show();
   }

   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }

   int currentGameMode = mGameMode;
   if (currentGameMode<count)
      mSecondaryMenu.setIndex(currentGameMode);
   else
      mSecondaryMenu.setIndex(0);

}


//==============================================================================
// BModePartyRoom2::
//==============================================================================
void BUISkirmishSetupScreen::populatePlayerCountMenu()
{
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;

   menuText=gDatabase.getLocStringFromID(23452);   // 1v1
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetPlayerCount);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(2);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   menuText=gDatabase.getLocStringFromID(23453);   // 2v2
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetPlayerCount);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(4);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   menuText=gDatabase.getLocStringFromID(23454);   // 3v3
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetPlayerCount);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(6);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }

   int itemWithFocus = 0;
   switch (mNumPlayers)
   {
   case 2:
      itemWithFocus=0;
      break;
   case 4:
      itemWithFocus=1;
      break;
   case 6:
      itemWithFocus=2;
      break;
   }

   mSecondaryMenu.setIndex(itemWithFocus);

}

//==============================================================================
// BModePartyRoom2::
//==============================================================================
void BUISkirmishSetupScreen::populateTeamMenu()
{
/*
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;

   menuText=gDatabase.getLocStringFromID(23497);   // Alpha vs Bravo
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetTeamType);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(cCustomTeamTypeTeams);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   menuText=gDatabase.getLocStringFromID(23496);   // Random
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetTeamType);     // The command 
   pMenuItem->setText(menuText);
   pMenuItem->setData(cCustomTeamTypeRandom);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }

   int itemWithFocus=0;
   if (pHostSettings->mRandomTeam)
      itemWithFocus=1;

   mSecondaryMenu.setIndex(itemWithFocus);
*/

}

//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::isValidMapType(long mapType)
{
   bool isValid=false;

   switch (mapType)
   {
   case BScenarioMap::cScenarioTypeDevelopment:
      if (gConfig.isDefined(cConfigShowDevMaps))
         isValid=true;
      break;
   case BScenarioMap::cScenarioTypePlaytest:
      isValid=true;
      break;
   case BScenarioMap::cScenarioTypeFinal:
      isValid=true;
      break;
   }
   return isValid;
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::isValidMap(int mapIndex, int numPlayers, bool matchPlayersExactly)
{
   const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapIndex);
   BASSERT(pMap);

   int mapMaxPlayers=pMap->getMaxPlayers();

   if (matchPlayersExactly)
   {
      // do we have an exact fit for players to map?
      return (mapMaxPlayers == numPlayers);
   }

   // can the map handle that many players?
   if (mapMaxPlayers<numPlayers)
      return false;

   return true;
}



//==============================================================================
// BModePartyRoom2::populateCampaignModeMenu
//==============================================================================
void BUISkirmishSetupScreen::populateCampaignModeMenu()
{
/*

   int itemWithFocus = mMainMenu.getSelectedIndex();

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;

   // ----  Item: Lobby 
   menuText.format(L"LOBBY: %s", gDatabase.getLocStringFromID(23458).getPtr() );
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetLobby);     // command id
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();


   // ---- Item: Mission Picker
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   int currentMission = pHostSettings->mMapIndex;
   BCampaignNode* pNode = pCampaign->getNode(currentMission);
   if (pNode)
      menuText.format(L"MISSION: %s", pNode->getDisplayName().getPtr());
   else
      menuText.set(L"SELECT MISSION");

   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetCampaignMission);     // command id
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();

   // clear out all the rest of menu items:
   for (int i=index; i<cMaxMainMenuItems; i++)
   {
      pMenuItem=mMenuItems[i];
      pMenuItem->disable();
      pMenuItem->hide();
   }

   mMainMenu.setIndex(itemWithFocus);
*/

}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::setInitialValues()
{

   mPartyRoomView = BUISkirmishSetupScreen::cPRViewMenu;       // start out looking at the menu

   mNumPlayers=2;

   setRandSeed(cUIRand, GetTickCount());
   mNumPlayers=2*getRandRange(cUIRand, 1, 2);                    // random between 1v1 and 2v2

   mDifficulty=gUserManager.getPrimaryUser()->getOption_DefaultAISettings();//DifficultyType::cNormal;
   setDefaultMapIndex();

   mGameMode=0;

   mUseDefaultMap=true;
   mRandomTeam=false;

}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::setDefaultMapIndex()
{
   mMapIndex=0;

   int newMapIndex = 0;

   if (!getRandomValidMap(mNumPlayers, newMapIndex, true))
   {
      // we couldn't find a map, throw up a message and quit
      return;
   }
   mMapIndex = newMapIndex;
}


//==============================================================================
// BUISkirmishSetupScreen::populateCustomModeMenu
//==============================================================================
void BUISkirmishSetupScreen::populateCustomModeMenu()
{
   int itemWithFocus = mMainMenu.getSelectedIndex();

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   // ----  Item: Lobby 
/*

   menuText.format(L"LOBBY: %s", gDatabase.getLocStringFromID(23456).getPtr() );
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetLobby);     // command id
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();
*/


   const BUString& formatString = gDatabase.getLocStringFromID(25299);
   // ---- Item : Number of Players
   switch(mNumPlayers)
   {
      case 2:
         menuText.locFormat(formatString.getPtr(), gDatabase.getLocStringFromID(23452).getPtr());
         break;
      case 4:
         menuText.locFormat(formatString.getPtr(), gDatabase.getLocStringFromID(23453).getPtr());
         break;
      case 6:
         menuText.locFormat(formatString.getPtr(), gDatabase.getLocStringFromID(23454).getPtr());
         break;
   }
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetPlayerCount);     // command id
   pMenuItem->setText(menuText);
   helpText.set(gDatabase.getLocStringFromID(25549).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->enable();
   pMenuItem->show();


   // ---- Item: Map
   
   const BScenarioMap* pScenarioMap = gDatabase.getScenarioList().getMapInfo(mMapIndex);
   if (pScenarioMap->getMapName().length() > 0)
   {
      if (pScenarioMap->getMaxPlayers() == mNumPlayers)
         menuText.locFormat(gDatabase.getLocStringFromID(25301).getPtr(), pScenarioMap->getMapName().getPtr());
      else
         menuText.locFormat(gDatabase.getLocStringFromID(25302).getPtr(), pScenarioMap->getMapName().getPtr());
   }
   else
   {
      BSimString mapFilename;
      strPathGetFilename(pScenarioMap->getFilename(), mapFilename);

      BUString mapName;
      mapName.format(L"%S", mapFilename.getPtr());

      // use the filename itself
      if (pScenarioMap->getMaxPlayers() == mNumPlayers)
         menuText.locFormat(gDatabase.getLocStringFromID(25301).getPtr(), mapName.getPtr());
      else
         menuText.locFormat(gDatabase.getLocStringFromID(25302).getPtr(), mapName.getPtr());
   }
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetMap);     // command id
   helpText.set(gDatabase.getLocStringFromID(25550).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();


   // ---- Item: Game Mode
   BGameMode* pGameMode = gDatabase.getGameModeByID(mGameMode);
   if (pGameMode)
      menuText.locFormat(gDatabase.getLocStringFromID(25303).getPtr(), pGameMode->getDisplayName().getPtr() );
   else
      menuText.set(gDatabase.getLocStringFromID(25304).getPtr());
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetGameMode);     // command id
   helpText.set(gDatabase.getLocStringFromID(25551).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();

   // ---- Item: AI Difficulty
   switch (mDifficulty)
   {
      case DifficultyType::cEasy:
         menuText.set(gDatabase.getLocStringFromID(25305).getPtr());
         break;
      case DifficultyType::cNormal:
         menuText.set(gDatabase.getLocStringFromID(25306).getPtr());
         break;
      case DifficultyType::cHard:
         menuText.set(gDatabase.getLocStringFromID(25307).getPtr());
         break;
      case DifficultyType::cLegendary:
         menuText.set(gDatabase.getLocStringFromID(25308).getPtr());
         break;
      case DifficultyType::cCustom:
         menuText.set(gDatabase.getLocStringFromID(25309).getPtr());
         break;
      case DifficultyType::cAutomatic:
         {
            BHumanPlayerAITrackingData trackingData;

            int autoDiffLevel = 0;
            BUString autoText;
            autoText.empty();

            if (trackingData.loadValuesFromMemoryBlock(gUserManager.getPrimaryUser()->getProfile()->getAITrackingDataMemoryPointer()))
            {
               autoDiffLevel = trackingData.getAutoDifficultyLevel();
               autoText.locFormat(gDatabase.getLocStringFromID(25976).getPtr(), gDatabase.getLocStringFromID(25310).getPtr(), autoDiffLevel);
            }
            else
            {
               autoText.set(gDatabase.getLocStringFromID(25310).getPtr());
            }
            menuText.set(autoText);
         }
         break;
   }
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty);     // command id
   helpText.set(gDatabase.getLocStringFromID(25552).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();

   // clear out all the rest of menu items:
   for (int i=index; i<cMaxMainMenuItems; i++)
   {
      pMenuItem=mMenuItems[i];
      pMenuItem->disable();
      pMenuItem->hide();
   }

   mMainMenu.setIndex(itemWithFocus);
}



//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::populateCampaignMissionPicker()
{
/*

   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   // how many menu items do we have?
   int count = pCampaign->getNumberNodes();

   // see how many we want to show in the picker.
   int numItems=0;
   for (int i=0; i<count; i++)
   {
      BCampaignNode* pNode = pCampaign->getNode(i);
      BASSERT(pNode);

      // for right now, skip cinematics
      if ( pNode->getFlag(BCampaignNode::cCinematic) )
         continue;

      numItems++;
   }

   int currentMission = pHostSettings->mMapIndex;


//----------------------------------------------------------------------------------------------
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;

   int itemWithFocus=0;
   //int itemNumber=0;
   for (int i=0; i<count; i++)
   {
      BCampaignNode* pNode = pCampaign->getNode(i);
      BASSERT(pNode);

      // for right now, skip cinematics
      if ( pNode->getFlag(BCampaignNode::cCinematic) )
         continue;

      if (currentMission == i)
         itemWithFocus = index;

      menuText=pNode->getDisplayName();
      pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index);
      pMenuItem->setControlID(BUISkirmishSetupScreen::cMenuCommandSetCampaignMission);     // The command 
      pMenuItem->setText(menuText);
      pMenuItem->setData(i);               // The data
      pMenuItem->enable();
      pMenuItem->show();

      if (index == numItems)
         break;

      index++;
   }

   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }



   mSecondaryMenu.setIndex(itemWithFocus);
*/
}

//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::getRandomValidMap(int numPlayers, int& mapIndex, bool matchPlayersExactly)
{
   int type = -1;
   switch (numPlayers)
   {
      case 2:
         type = BScenarioList::cPlayerMapType1v1;
         break;
      case 4:
         type = BScenarioList::cPlayerMapType2v2;
         break;
      case 6:
         type = BScenarioList::cPlayerMapType3v3;
         break;
   }

   BASSERT(type>=0);
   if (type<0)
      return false;

   int count = gDatabase.getScenarioList().getPlayerNumberMapCount(type);
   setRandSeed(cUIRand, GetTickCount());
   long startIndex=getRandRange(cUIRand, 0, count-1);

   bool done = false;
   int index = startIndex;
   while(!done)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getPlayerNumberMapInfo(type, index);
      BASSERT(pMap);


      if ( isValidMapType(pMap->getType()) && isValidMap(pMap->getID(), numPlayers, matchPlayersExactly) )
      {
         mapIndex=pMap->getID();
         return true;
      }

      // increment our index
      index++;

      if (index>=count)
         index=0;

      if (index == startIndex)
         done=true; // we've wrapped and haven't found anything.
   }

   return false;
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::getFirstValidMap(int numPlayers, int& mapIndex, bool matchPlayersExactly)
{
   for (int i=0; i<gDatabase.getScenarioList().getMapCount(); i++)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(i);
      BASSERT(pMap);
      if (!isValidMapType(pMap->getType()))
         continue;

      if (!isValidMap(i, numPlayers, matchPlayersExactly))
         continue;

      mapIndex=i;
      return true;
   }

   return false;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::getSettingsChangeFromSecondaryMenuItem(BUIMenuItemControl * pMenuItemControl)
{
   bool update = false;
   switch (pMenuItemControl->getControlID())
   {
/*
   case BUISkirmishSetupScreen::cMenuCommandSetLobby:
      {
         long newPartyRoomMode = pMenuItemControl->getData();
         mpMode->onAcceptLobby(newPartyRoomMode);
      }
      break;
*/

   case BUISkirmishSetupScreen::cMenuCommandSetCampaignMission:
      if (mMapIndex != pMenuItemControl->getData())
      {
         mMapIndex = pMenuItemControl->getData();
         update=true;
      }

      break;
   case BUISkirmishSetupScreen::cMenuCommandSetMap:
      // set the map
      if (mMapIndex != pMenuItemControl->getData())
      {
         setUseDefaultMap(false);
         int newMapIndex = pMenuItemControl->getData();
         if (updateMap(newMapIndex))
            update=true;
      }
      break;
   case BUISkirmishSetupScreen::cMenuCommandSetPlayerCount:
      if (mNumPlayers != pMenuItemControl->getData())
      {
         int newMapIndex = mMapIndex;
         int newNumPlayers = mNumPlayers;
         bool matchPlayersExactly = getUseDefaultMap();

         const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mMapIndex);
         // is our current map a valid type and valid for the num players?
         if ( !pMap ||
            !isValidMapType(pMap->getType()) ||
            !isValidMap(mMapIndex, pMenuItemControl->getData(), matchPlayersExactly) )
         {
            // for whatever reason we need a new map, let's go back to using the default.
            setUseDefaultMap(true);

            // find a new map
            int newMapIndexTemp = 0;
            if (!getFirstValidMap(pMenuItemControl->getData(), newMapIndexTemp, true))
            {
               // we couldn't find a map, throw up a message and quit
               return;
            }
            newMapIndex = newMapIndexTemp;
         }

         newNumPlayers = pMenuItemControl->getData();

         updateMapAndPlayers(newMapIndex, newNumPlayers);
         // update=true; handled in the updateMapAndPlayers function
      }
      break;
   case BUISkirmishSetupScreen::cMenuCommandSetGameMode:
      {
         if (mGameMode != pMenuItemControl->getData())
         {
            BGameMode* gameMode = gDatabase.getGameModeByID(pMenuItemControl->getData());
            if (gameMode)
            {
               mGameMode = pMenuItemControl->getData();
               // mpMode->setGameOptions(gameOptionSettings);
               update=true;
            }
         }
      }
      break;
   case BUISkirmishSetupScreen::cMenuCommandSetAIDifficulty:
      {
         if (mDifficulty != pMenuItemControl->getData())
         {
            mDifficulty = pMenuItemControl->getData();
            // mpMode->setGameOptions(gameOptionSettings);
            update=true;
         }
      }
      break;
/*
   case BUISkirmishSetupScreen::cMenuCommandSetTeamType:
      if (mRandomTeam != (pMenuItemControl->getData() == BUISkirmishSetupScreen::cCustomTeamTypeRandom))
      {
         mRandomTeam = !mRandomTeam;
         // mpMode->setGameOptions(gameOptionSettings);
         update = true;
      }
      break;
*/
   }

   if (update)
   {
      // only send out an update if something changed.
      // pParty->changeHostSettings(gameOptionSettings);
      // fixme - update ui
      populateMenu();
   }
}

//==============================================================================
// fixme - if we drop players we will need to pop up a dialog?
//==============================================================================
bool BUISkirmishSetupScreen::updateMap(int newMapIndex)
{
   bool update=false;
   if (newMapIndex!=mMapIndex)
   {
      mMapIndex=newMapIndex;
      // show a new map
      updateMap();
      update=true;
   }

   return update;
}

//==============================================================================
// fixme - if we drop players we will need to pop up a dialog?
//==============================================================================
bool BUISkirmishSetupScreen::updateMapAndPlayers(int newMapIndex, int newNumPlayers)
{
   bool update = false;

   if (newNumPlayers!=mNumPlayers)
   {
      mNumPlayers=newNumPlayers;

      int numVisible=mNumPlayers;
      bool team=true;
      if (team)
      {
         int count=numVisible/2;
         // just do a straight show of the slots
         for (int i=0; i<cPRNumPlayersSlotsPerTeam; i++)
         {
            if (i<count)
               continue;
            kickPlayer(i);
            kickPlayer(i+3);
         }
      }
      else
      {
         // just do a straight show of the slots
         for (int i=0; i<cPRNumPlayerSlots; i++)
         {
            if (i<numVisible)
               continue;

            setPlayerSlotActive(i, (i<numVisible));
         }
      }

      setPlayerSlotsVisible(mNumPlayers, true);
      refreshPlayerSlots();
      update=true;
   }

   if (updateMap(newMapIndex))
      update=true;

   if (update)
      populateMenu();

   return update;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::clearPlayerSlotData(int slot, const BUString& text)
{
   BUIGamerTagLongControl& player = mPlayerControls[slot].getGamerTag();
   player.clear();

   // fixme - make this a BUString
   player.setGamerTag(text);
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::refreshPlayerSlots()
{
   // this updates the player slots so set the last time we got gamer pics here.
   mLastGamerPicUpdate=timeGetTime();

   bool isSecondaryUserActive=false;
   if (gUserManager.getSecondaryUser()->getFlagUserActive())
      isSecondaryUserActive=true;

   for (int slot=0; slot<cPlayerSlotCount; slot++)
   {
      BUIPlayer& player = mPlayers[slot];
      if (!player.mActive)
         continue;

      BUIGameSetupPlayerControl& playerControl=mPlayerControls[slot];

      bool inCenter=false;
      if (player.mSlotType==BUIPlayer::cEmpty)
      {
         // clear the player
         clearPlayerSlotData(slot, gDatabase.getLocStringFromID(25358));
         #ifdef AUTO_FILL_PLAYERS
            int count=mNumPlayers/2;
            if (slot<count || (slot >= 3 && slot-3<count))
            {
               addAI(slot);
               updatePlayerSlot(slot);
               inCenter = player.mInCenter;
            }
         #endif
      }
      else
      {
         updatePlayerSlot(slot);
         inCenter = player.mInCenter;
      }

      int slotTransitionState = getSlotTransitionState(slot, inCenter);

      if (slotTransitionState != 0)
      {
         BString& keyFrame = mSlotKeyFrames[slotTransitionState];
         playerControl.playTransition(keyFrame.getPtr());
      }
   }
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::updatePlayerSlot(int slot)
{
   BUIPlayer& player = mPlayers[slot];
   if (!player.mActive)
      return;

   // display the player
   setPlayerSlotData(slot);
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::resetSlotStates()
{
   for (uint i=0; i<cPlayerSlotCount; i++)
      mSlotStates[i]=cPRSlotStateEmpty;
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::setSlotState(int slot, int slotState)
{
   mSlotStates[slot]=slotState;
}

//==============================================================================
//==============================================================================
int BUISkirmishSetupScreen::getSlotTransitionState(int slot, bool inCenter)
{
   int currentState = mSlotStates[slot];
   /*
   Empty      Normal    Center   
   --------------+---------------------------------------
   Empty         |   Empty      ease_in   CENTER   
   Normal        |   NORMAL     NORMAL    N2NC           
   Center        |   CENTER     C2N       CENTER   
   --------------+---------------------------------------

   */
   //                      [Current State]      [Next State]
   int stateTransitions[cPRSlotStateCount][cPRSlotStateCount]=
   {
      // Empty                   Normal            NormalCenter            
      {  cPRSlotKF_Empty,        cPRSlotKF_EaseIn, cPRSlotKF_Center  },
      {  cPRSlotKF_Normal,       cPRSlotKF_Normal, cPRSlotKF_N2C     },
      {  cPRSlotKF_Center,       cPRSlotKF_C2N,    cPRSlotKF_Center  },
   };

   // determine our next state
   int nextState = cPRSlotStateNormal;
   if (inCenter)
      nextState = cPRSlotStateCenter;

   if (currentState == nextState)
      return 0;

   // save it
   setSlotState(slot, nextState);

   // lookup the transition that we should play
   int transition = stateTransitions[currentState][nextState];

   return transition;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::setPlayer(BUser* user, int leader, int civ, int slot, int team)
{
   if ( (slot<0) || (slot>=cPlayerSlotCount) )
   {
      BASSERT(0);
      return;
   }

   BUIPlayer& player = mPlayers[slot];

   // bool  mActive;             // active - available for player,
   // int  mSlot;               // the visible slot this player is mapped to.
   // int  mSlotType;           // See enum above
   player.mSlotType=BUIPlayer::cLocalHuman;

   // BSimString mGamerTag;
   BUString temp;
   temp.locFormat(L"%S", user->getName().getPtr());
   player.mGamerTag=temp;

   // BSimString mGamerPic;      
   player.mGamerPic.format("img://gamerPic:%I64x", user->getXuid() );

   // int8  mLeader;
   player.mLeader=(int8)leader;
   player.mCiv=(int8)civ;


   // long  mLeaderStringIDIndex; // 
   BLeader* pLeader=gDatabase.getLeader(leader);
   if(pLeader)
   {
      player.mLeaderStringIDIndex=pLeader->mNameIndex;
   }
   else
   {
      // random UNSC or Covenant?
      if (gDatabase.getCivID("UNSC") == civ)
      {
         // random unsc
         player.mLeaderStringIDIndex=gDatabase.getLocStringIndex(23718);
      }
      else if (gDatabase.getCivID("Covenant") == civ)
      {
         // random covenant
         player.mLeaderStringIDIndex=gDatabase.getLocStringIndex(23720);
      }
      else
      {
         // random random
         player.mLeaderStringIDIndex=gDatabase.getLocStringIndex(23722);
      }
   }

   // int8  mCiv;
   player.mCiv=(int8)civ;

   // uint8 mVoice;              // voice state
   // uint8 mPing;               // QOS indicator
   // int8  mTeam;               // team the player is on
   player.mTeam=(int8)team;

   const BProfileRank& rank = user->getProfile()->getRank();
   player.mRank=rank.mRank;
   // int8  mRank;               // online rank?
   // int8  mControllerPort;     // controller that this player is tied to.
   player.mControllerPort=(int8)user->getPort();
   // int8  mID;
   // bool  mInCenter:1;         // indicates that this player is in the center
   player.mInCenter=false;
   // bool  mHost:1;             // host / not host
   if (gUserManager.getPrimaryUser()->getPort() == user->getPort())
      player.mHost=true;
   else
      player.mHost=false;

   // bool  mReady:1;            // player is ready
   player.mReady=true;

   // bool  mArrowLeft:1;        
   player.mArrowLeft=false;

   // bool  mArrowRight:1;       
   player.mArrowRight=false;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::setPlayerSlotsVisible(int numVisible, bool team)
{
   if (team)
   {
      int count=numVisible/2;
      // just do a straight show of the slots
      for (int i=0; i<cPRNumPlayersSlotsPerTeam; i++)
      {
         setPlayerSlotActive((i), (i<count));
         setPlayerSlotActive((i+3), (i<count));
      }
   }
   else
   {
      // just do a straight show of the slots
      for (int i=0; i<cPRNumPlayerSlots; i++)
      {
         setPlayerSlotActive((i), (i<numVisible));
      }
   }
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::kickPlayer(int slot)
{
   if (!isValidSlot(slot))
   {
      BASSERT(0);
      return;
   }

   BUIPlayer& player = mPlayers[slot];
   if (player.mSlotType==BUIPlayer::cAI)
   {
      player.mSlotType=BUIPlayer::cEmpty;
   }
   else if (player.mSlotType == BUIPlayer::cLocalHuman)
   {
      // if not secondary user, then skip
      if (isUserType(slot, BUserManager::cSecondaryUser))
      {
         // drop the secondary user
         gUserManager.dropSecondaryUser();

         // clean up the slot
         player.mSlotType=BUIPlayer::cEmpty;
      }
   }

   refreshPlayerSlots();
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::addAI(int slot)
{
   if ( (slot<0) || (slot>=cPlayerSlotCount))
   {
      BASSERT(0);
      return;
   }

   BUIPlayer& player = mPlayers[slot];
   if (player.mSlotType!=BUIPlayer::cEmpty)
   {
      BASSERT(0);
      return;
   }

   // bool  mActive;             // active - available for player,
   // int8  mSlot;               // the visible slot this player is mapped to.
   // int8  mSlotType;           // See enum above
   player.mSlotType=BUIPlayer::cAI;

   // BSimString mGamerTag;
   // loc fixme
//   player.mGamerTag="AI Player - need cool names";
   player.mGamerTag=gDatabase.getLocStringFromID(25940);

   // BSimString mGamerPic;      
   //player.mGamerPic.format("img://gamerPic:%I64x", user->getXuid() );

   #ifdef AUTO_FILL_PLAYERS
      int leaderNum;
      if (slot<cPRNumPlayersSlotsPerTeam)
      {
         player.mCiv=1;
         leaderNum=1+slot;
      }
      else
      {
         player.mCiv=2;
         leaderNum=1+(slot-cPRNumPlayersSlotsPerTeam);
      }
      int leaderCount = gDatabase.getNumberLeaders();
      int leaderCounter = 1;
      for (int i=0; i<leaderCount; i++)
      {
         BLeader* pLeader = gDatabase.getLeader(i);
         if (pLeader->mLeaderCivID == player.mCiv)
         {
            player.mLeader = (int8)i;
            if (leaderNum == leaderCounter)
               break;
            leaderCounter++;
         }
      }
   #else
	   // int8  mLeader;
	   player.mLeader=1;

	   // int8  mCiv;
	   player.mCiv=1;
   #endif

   // long  mLeaderStringIDIndex; // 
   BLeader* pLeader=gDatabase.getLeader(player.mLeader);
   if(pLeader)
   {
      player.mLeaderStringIDIndex=pLeader->mNameIndex;
   }
   else
   {
      // random UNSC or Covenant?
      if (gDatabase.getCivID("UNSC") == player.mCiv)
      {
         // random unsc
         player.mLeaderStringIDIndex=gDatabase.getLocStringIndex(23718);
      }
      else if (gDatabase.getCivID("Covenant") == player.mCiv)
      {
         // random covenant
         player.mLeaderStringIDIndex=gDatabase.getLocStringIndex(23720);
      }
      else
      {
         // random random
         player.mLeaderStringIDIndex=gDatabase.getLocStringIndex(23722);
      }
   }

   // uint8 mVoice;              // voice state
   // uint8 mPing;               // QOS indicator
   // int8  mTeam;               // team the player is on
   int team=1;
   if (slot>=cPRNumPlayersSlotsPerTeam)
      team=2;
   player.mTeam=(int8)team;

   player.mRank=0;
   // int8  mControllerPort;     // controller that this player is tied to.
   player.mControllerPort=-1;
   // int8  mID;
   // bool  mInCenter:1;         // indicates that this player is in the center
   player.mInCenter=false;
   // bool  mHost:1;             // host / not host
   player.mHost=false;

   // bool  mReady:1;            // player is ready
   player.mReady=true;

   // bool  mArrowLeft:1;        
   player.mArrowLeft=false;

   // bool  mArrowRight:1;       
   player.mArrowRight=false;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::displayButtons()
{
   uint btn1=BUIButtonBarControl::cFlashButtonOff;
   long strID1=0;

   uint btn2=BUIButtonBarControl::cFlashButtonOff;
   long strID2=0;

   uint btn3=BUIButtonBarControl::cFlashButtonOff;
   long strID3=0;

   uint btn4=BUIButtonBarControl::cFlashButtonOff;
   long strID4=0;

   uint btn5=BUIButtonBarControl::cFlashButtonOff;
   long strID5=0;


   switch (mPartyRoomView)
   {
   case BUISkirmishSetupScreen::cPRViewMenu:
      btn1=BUIButtonBarControl::cFlashButtonA;
      strID1=23789;   // Edit
      //strID1=23439;        // Ready Up

      btn2=BUIButtonBarControl::cFlashButtonB;
      strID2=23851;   // Leave Party

      btn4=BUIButtonBarControl::cFlashButtonStart;
      strID4=23644;        // Ready Up

/*
      btn5=BUIButtonBarControl::cFlashButtonLRButton;
      strID5=23852;        // Switch Teams
*/

      break;
   case BUISkirmishSetupScreen::cPRViewPlayerMenu:
      btn1=BUIButtonBarControl::cFlashButtonA;
      strID1=23437;   // Accept
      btn2=BUIButtonBarControl::cFlashButtonB;
      strID2=23438;   // Cancel
      break;
   case BUISkirmishSetupScreen::cPRViewSubMenu:
      btn1=BUIButtonBarControl::cFlashButtonA;
      strID1=23437;   // Accept
      btn2=BUIButtonBarControl::cFlashButtonB;
      strID2=23438;   // Cancel
      break;
   case BUISkirmishSetupScreen::cPRViewPlayerList:
      // edit or gamer tag
      btn1=BUIButtonBarControl::cFlashButtonA;
      // it's me, I can edit me
      strID1=23446;   // Settings

      btn2=BUIButtonBarControl::cFlashButtonB;
      strID2=23851;   // Leave party

      /*
      btn3=BUIButtonBarControl::cFlashButtonX;
      strID3=23439;   // Ready Up
      */
      btn4=BUIButtonBarControl::cFlashButtonStart;
      strID4=23644;        // Ready Up


/*
      btn5=BUIButtonBarControl::cFlashButtonLRButton;
      strID5=23852;        // Switch Teams
*/

      break;
   case BUISkirmishSetupScreen::cPRViewPlayerEdit:
      btn1=BUIButtonBarControl::cFlashButtonA;
      strID1=23437;   // Accept
      btn2=BUIButtonBarControl::cFlashButtonB;
      strID2=23440;   // Back
      break;
   }

   mButtonBar.setButtonStates(btn1, btn2, btn3, btn4, btn5);
   mButtonBar.setButtonTexts(  gDatabase.getLocStringFromID(strID1),
      gDatabase.getLocStringFromID(strID2),
      gDatabase.getLocStringFromID(strID3),
      gDatabase.getLocStringFromID(strID4),
      gDatabase.getLocStringFromID(strID5) );
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::render( void )
{
   __super::render();

   if (mpLeaderPicker)
      mpLeaderPicker->render();
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::update( float dt )
{
   if (mLastGamerPicUpdate < gGamerPicManager.getLastUpdate())
      mDoRefresh=true;

   if (mDoRefresh)
   {
      refreshPlayerSlots();
      updateHelp();
      displayButtons();

      mDoRefresh=false;
   }


   gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_HWGAMEMODE, mGameMode);
   switch(mNumPlayers)
   {
      case (2) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_1V1);break;
      case (4) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_2V2);break;
      case (6) : gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_GAMESIZE, CONTEXT_PRE_CON_GAMESIZE_PRE_CON_GAMESIZE_3V3);break;
   }
   gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_SKIRMLOCALLOBBY);
   gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_DIFFICULTY, mDifficulty);
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::editPlayer(int slot)
{
   if ( (slot<0) || (slot>=cPRNumPlayerSlots) )
      return;

   BUIPlayer& player = mPlayers[slot];
   if (player.mSlotType == BUIPlayer::cEmpty)
      return;

   mPartyRoomViewPrevious=mPartyRoomView;
   mPartyRoomView=BUISkirmishSetupScreen::cPRViewPlayerEdit;

   if (mpLeaderPicker)
   {
      mEditSlot=slot;
      mpLeaderPicker->setCurrentLeader(player.mCiv, player.mLeader);
      mpLeaderPicker->show();
      this->setVisible(false);
   }
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::leaderPickerEvent(const BSimString& command)
{
   if (command == "accept")
   {
      if (mpLeaderPicker)
      {
         // grab the new leader info here.
         gUI.playConfirmSound();
         mpLeaderPicker->hide();
         acceptPlayerChanges();
      }
   }
   else if (command == "cancel")
   {
      if (mpLeaderPicker)
      {
         mpLeaderPicker->hide();
         cancelPlayerChanges();
      }
   }
   else if (command == "close")
   {
      this->setVisible(true);
      mPartyRoomView=mPartyRoomViewPrevious;
      mDoRefresh=true;
   }
   else 
   {
      return false;
   }

   return true;
}


//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::acceptPlayerChanges()
{
   //mPartyRoomView=mPartyRoomViewPrevious;
   // push the new civ/leader settings into my settings and send out
   int slot = mEditSlot;
   if ( (slot<0) || (slot>=cPRNumPlayerSlots) )
      return;

   BUIPlayer& player = mPlayers[slot];

   if (mpLeaderPicker)
   {
      const BLeaderPickerData* pLeader = mpLeaderPicker->getCurrentLeader();

      if (pLeader)
      {
         player.mCiv = pLeader->mCiv;
         player.mLeader = pLeader->mLeader;
      }
   }
/*
   refreshPlayerSlots();
   updateHelp();
   displayButtons();
*/
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::cancelPlayerChanges()
{
   // mPartyRoomViewPrevious needs to set before calling this method.
   // mPartyRoomView=mPartyRoomViewPrevious;
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::decrementLeader(int slot)
{
   if ( (slot<0) || (slot>=cPRNumPlayerSlots) )
      return false;

   BUIPlayer& player = mPlayers[slot];

   if (mpLeaderPicker)
   {
      mpLeaderPicker->setPreviousLeader(player.mCiv, player.mLeader);
      const BLeaderPickerData* pLeader = mpLeaderPicker->getCurrentLeader();

      if (pLeader)
      {
         player.mCiv = pLeader->mCiv;
         player.mLeader = pLeader->mLeader;
      }
   }

   refreshPlayerSlots();
   return true;
}

//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::incrementLeader(int slot)
{
   if ( (slot<0) || (slot>=cPRNumPlayerSlots) )
      return false;

   BUIPlayer& player = mPlayers[slot];

   if (mpLeaderPicker)
   {
      mpLeaderPicker->setNextLeader(player.mCiv, player.mLeader);
      const BLeaderPickerData* pLeader = mpLeaderPicker->getCurrentLeader();

      if (pLeader)
      {
         player.mCiv = pLeader->mCiv;
         player.mLeader = pLeader->mLeader;
      }
   }

   refreshPlayerSlots();
   return true;
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::canStartGame(bool bTeam)
{
   // basically, we just need to make sure that each slot is not empty.
   if (bTeam)
   {
      int count=mNumPlayers/2;

      for (int i=0; i<cPRNumPlayersSlotsPerTeam; i++)
      {
         if (i>=count)
            break;

         // check the user slots
         if (isSlotEmpty(i))
            return false;
         if (isSlotEmpty(i+3))
            return false;
      }
   }
   else
   {
      // just do a straight show of the slots
      for (int i=0; i<cPRNumPlayerSlots; i++)
      {
         if (i>=mNumPlayers)
            break;

         if (isSlotEmpty(i))
            return false;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::isSlotEmpty(int slot)
{
   if ( (slot<0) || (slot>=cPRNumPlayerSlots) )
      return true;

   BUIPlayer& player = mPlayers[slot];
   
   return (player.mSlotType == BUIPlayer::cEmpty);
}

//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::startGame()
{
/*
   BUser* pUser = gUserManager.getPrimaryUser();
   BUser* pUser2 = gUserManager.getSecondaryUser();
*/

   //gDatabase.resetGameSettings();
   gDatabase.resetGameSettings();
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(pSettings)
   {
      BSimString gameID;
      MVince_CreateGameID(gameID);

      pSettings->setLong(BGameSettings::cPlayerCount, mNumPlayers);

      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mMapIndex);
      BASSERT(pMap);
      BSimString tempFileName = pMap->getFilename();
      tempFileName.crop(0,tempFileName.length()-5);
      pSettings->setString(BGameSettings::cMapName, tempFileName);
      pSettings->setLong(BGameSettings::cMapIndex, -1);
      pSettings->setBool(BGameSettings::cRecordGame, gConfig.isDefined(cConfigRecordGames));
      pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
      pSettings->setString(BGameSettings::cGameID, gameID);
      pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
      pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeSkirmish);
      pSettings->setLong(BGameSettings::cGameMode, mGameMode);
      pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

      // set up all the players now.
      int slot=1;
      for (int i=0; i<cPlayerSlotCount; i++)
      {
         BUIPlayer& player = mPlayers[i];

         if (player.mSlotType==BUIPlayer::cEmpty)
            continue;

         // Common settings for human and AI
         pSettings->setLong( PSINDEX(slot, BGameSettings::cPlayerTeam), player.mTeam);
         pSettings->setLong( PSINDEX(slot, BGameSettings::cPlayerCiv), player.mCiv);
         pSettings->setLong( PSINDEX(slot, BGameSettings::cPlayerLeader), player.mLeader);

         float difficultyValue=0.0f;
         getDifficultyValueForType( mDifficulty, difficultyValue);
         pSettings->setLong( PSINDEX(slot, BGameSettings::cPlayerDifficultyType), mDifficulty);
         pSettings->setFloat( PSINDEX(slot, BGameSettings::cPlayerDifficulty), difficultyValue);


         if (player.mSlotType==BUIPlayer::cLocalHuman)
         {
            BUser* pUser = gUserManager.getUserByPort(player.mControllerPort);
            if (!pUser)
            {
               BASSERT(0);
               continue;
            }
            
            #ifndef LTCG
            if (gConfig.isDefined(cConfigPlayer1AI))
            {
               pSettings->setLong( PSINDEX(slot, BGameSettings::cPlayerType), BGameSettings::cPlayerComputer);
               BString temp;
               temp.format("%S", player.mGamerTag.getPtr());
               pSettings->setString( PSINDEX(slot, BGameSettings::cPlayerName), temp.getPtr());
            }
            else
            #endif
            {
               uint16 rank = 0;
               if (pUser->getProfile())
                  rank = pUser->getProfile()->getRank().mValue;
               pSettings->setWORD( PSINDEX(slot, BGameSettings::cPlayerRank), rank);
               pSettings->setLong( PSINDEX(slot, BGameSettings::cPlayerType), BGameSettings::cPlayerHuman);
               pSettings->setString( PSINDEX(slot, BGameSettings::cPlayerName), pUser->getName());
               pSettings->setUInt64( PSINDEX(slot, BGameSettings::cPlayerXUID), pUser->getXuid());
            }
         }
         else if (player.mSlotType==BUIPlayer::cAI)
         {
            pSettings->setLong( PSINDEX(slot, BGameSettings::cPlayerType), BGameSettings::cPlayerComputer);

            // Fixes Crash Bug - PHX-18389             
            // We don't really need the ansi name for the ai players in the settings as their display names 
            // get superceded by a display string from the string table.
            BString temp("AI Player");            
            pSettings->setWORD( PSINDEX(slot, BGameSettings::cPlayerRank), 0);
            pSettings->setString( PSINDEX(slot, BGameSettings::cPlayerName), temp.getPtr());
            //pSettings->setUInt64( PSINDEX(slot, BGameSettings::cPlayerXUID, pUser->getXuid());
         }
         else
         {
            BASSERT(0);
            continue;
         }

         slot++;
      }
   }

   gLiveSystem->setPresenceContext(PROPERTY_GAMESCORE, 0, true);
   gLiveSystem->setPresenceContext(PROPERTY_GAMETIMEMINUTES, 0, true);
   gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_NETWORK, CONTEXT_PRE_CON_NETWORK_LOCAL);
   gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_PLAYINGSKIRM);

   gModeManager.getModeMenu()->setNextState( BModeMenu::cStateGotoGame );
   return true;
}


//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::getDifficultyValueForType( long type, float& value )
{
   switch( type )
   {
   case DifficultyType::cAutomatic:
      {
         BHumanPlayerAITrackingData trackingData;
         if( trackingData.loadValuesFromMemoryBlock( gUserManager.getPrimaryUser()->getProfile()->getAITrackingDataMemoryPointer() ) )
            value = trackingData.getAutoDifficultyLevel() / 100.0f;
         else
            value = gDatabase.getDifficultyDefault();
         return true;
      }

   case DifficultyType::cEasy:
      value = gDatabase.getDifficultyEasy();
      return true;

   case DifficultyType::cNormal:
      value = gDatabase.getDifficultyNormal();
      return true;

   case DifficultyType::cHard:
      value = gDatabase.getDifficultyHard();
      return true;

   case DifficultyType::cLegendary:
      value = gDatabase.getDifficultyLegendary();
      return true;

   case DifficultyType::cCustom:
      return false;

   default:
      BASSERTM(false, "Invalid difficulty type specified.  Setting Default Difficulty Value.");
      value = gDatabase.getDifficultyDefault();
      return false;
   }
}

//==============================================================================
//==============================================================================
bool BUISkirmishSetupScreen::isValidSlot(int slot)
{
   if ( (slot<0) || (slot>cPlayerSlotCount))
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BUISkirmishSetupScreen::refreshPlayer(BUser* user, int slot)
{
   if (!isValidSlot(slot))
      return;

   BUIPlayer& player = mPlayers[slot];

   // only add a subset of the stuff. The rest should be valid.
   BUString temp;
   temp.locFormat(L"%S", user->getName().getPtr());
   player.mGamerTag=temp;
   player.mGamerPic.format("img://gamerPic:%I64x", user->getXuid() );
}


//==============================================================================
// BUserNotificationListener interface method, this is called when the user
//    status has changed on either the primary or secondary user.
//==============================================================================
void BUISkirmishSetupScreen::userStatusChanged()
{
   // We really only care about the secondary user. If the primary user changes, we are leaving and
   // going back to the main menu.
   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   BUser* pUser = gUserManager.getSecondaryUser();
   if (!pUser || !pUser->getFlagUserActive())
   {
      // remove the secondary user from our slots (if we find one)
      for (int i=0; i<cPlayerSlotCount; i++)
      {
         BUIPlayer& player = mPlayers[i];
         if (player.mSlotType == BUIPlayer::cLocalHuman)
         {
            if (player.mControllerPort == pPrimaryUser->getPort())
               continue;
            
            // drop the slot
            player.mSlotType = BUIPlayer::cEmpty;
         }
      }

      refreshPlayerSlots();
      return;
   }

   // find our secondary user and update him.
   int slot = getPlayerSlotByPort(pUser->getPort());
   if (!isValidSlot(slot))
   {
      // we need to add the secondary user if we didn't find him.
      slot = getFirstOpenSlot(true);
      if (!isValidSlot(slot))
         return;

      setPlayer(pUser, 1, 1, slot, 2);       // find defaults
   }
   else
   {
      // refresh the slot with the gamer tag data.
      refreshPlayer(pUser, slot);
   }

   // refresh our slots.
   refreshPlayerSlots();
}


//==============================================================================
void BUISkirmishSetupScreen::yornResult(uint result, DWORD userContext, int port)
{
   return;
}

//==============================================================================
// BModePartyRoom2::updateMenuHelp
//==============================================================================
void BUISkirmishSetupScreen::updateMenuHelp()
{
   switch (mPartyRoomView)
   {
   case cPRViewMenu:
      {
         BUIMenuItemControl* c = (BUIMenuItemControl*)mMainMenu.getSelectedControl();
         if (c)
         {
            setHelpText(c->getHelpText());
         }
      }
      break;

   case cPRViewSubMenu:
      {
         BUIMenuItemControl* c = (BUIMenuItemControl*)mMainMenu.getSelectedControl();
         if (c)
         {
            setHelpText(c->getHelpText());
         }
      }
      break;
   }
}



//==============================================================================
// BModePartyRoom2::updatePlayListHelp
//==============================================================================
void BUISkirmishSetupScreen::updatePlayerListHelp()
{
   if (mPartyRoomView != cPRViewPlayerList)
      return;

   if (getPlayerCurrentSlotFocus() >= 0)
   {
      BUIPlayer* player = getPlayer(getPlayerCurrentSlotFocus());
      if (!player)
         return;

      if (player->mSlotType == BUIPlayer::cEmpty)
      {
         // empty slot.
         setHelpText(gDatabase.getLocStringFromID(25555));
      }
      else
      {
         // "Set up your player slot
         setHelpText(gDatabase.getLocStringFromID(23825));
      }
   }
}

//==============================================================================
// BModePartyRoom2::updateHelp
//==============================================================================
void BUISkirmishSetupScreen::updateHelp()
{
   updateMenuHelp();
   updatePlayerListHelp();
}


