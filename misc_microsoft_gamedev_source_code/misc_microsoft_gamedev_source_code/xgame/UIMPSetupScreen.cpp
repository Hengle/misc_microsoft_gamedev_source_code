//============================================================================
// UIMPSetupScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIMPSetupScreen.h"

#include "UIMenuItemControl.h"

#include "game.h"
#include "database.h"
#include "gamemode.h"

#include "configsgame.h"
#include "econfigenum.h"

#include "campaignmanager.h"

#include "user.h"
#include "usermanager.h"
#include "userprofilemanager.h"

#include "leaders.h"

#include "liveSession.h"
#include "gamerPicManager.h"

//==============================================================================
// BUIMPSetupScreen::BUIMPSetupScreen
//==============================================================================
BUIMPSetupScreen::BUIMPSetupScreen() : 
mGridControl(3, 2),
mPartyRoomView(cPRViewPlayerList),
mpMode(NULL),
mLastBumperKeyInputTime(0),
mLastViewChangeTime(0),
mpGamesList(NULL),
mLastPopUpdateTime(0),
mLastHelp2Clear(0),
mLastGamerPicUpdate(0)
{
   mLastMapImage.set("");
}

//==============================================================================
// BUIMPSetupScreen::~BUIMPSetupScreen
//==============================================================================
BUIMPSetupScreen::~BUIMPSetupScreen()
{
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




}

//============================================================================
//============================================================================
bool BUIMPSetupScreen::init(BXMLNode dataNode)
{
   mpMode = (BModePartyRoom2*)gModeManager.getMode(BModeManager::cModePartyRoom2);

   BUIScreen::init(dataNode);

   // initialize all the slot transition animations
   mSlotKeyFrames.setNumber(cPRSlotKF_TransitionCount);

   mSlotKeyFrames[cPRSlotKF_Empty].set("on");
   mSlotKeyFrames[cPRSlotKF_EaseIn].set("ease_in");      // entry transition
   mSlotKeyFrames[cPRSlotKF_Normal].set("normal");       // State
   mSlotKeyFrames[cPRSlotKF_Center].set("center");       // state
   mSlotKeyFrames[cPRSlotKF_N2C].set("N2C");             // Normal to Center
   mSlotKeyFrames[cPRSlotKF_C2N].set("C2N");             // Center to Normal

   // reset all the slot states to a default state
   resetSlotStates();

   // Lan Browser
   if (mpGamesList==NULL)
   {
      mpGamesList=new BUIMenu();
      mpGamesList->init(getMovie(), "art\\ui\\flash\\pregame\\partyroom\\GameListMenuInput.xml");
      mpGamesList->setMovieClip("mLanGameListContainer");
      mpGamesList->setEventHandler(this);
      mpGamesList->setUseSelectButton(false);
      mpGamesList->clearItemFocus();
      mpGamesList->hide();
   }

   mGamesListTitle.init(this, "mLanGameListContainer.mTitle");
   mGamesListTitle.setText(gDatabase.getLocStringFromID(25290));

   // --------- PRIMARY MENU
   mImageViewer.init(this, "mImageViewer", 0, NULL);
   mImageViewer.setAutoTransition(true);
   mImageViewer.setViewDuration(9000.0f);
   mImageViewer.setImageSize(700, 350);

   mImageViewer.start();


   // --------- PRIMARY MENU
   mMainMenu.init( this, "", cControlIDMainMenu, NULL );
   mMainMenu.setIndex(-1);

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

   mPlayerListTitle.init(this, "mPlayerMenuList.mDescriptionLabel");
   mPlayerListTitle.setText(gDatabase.getLocStringFromID(25663));

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

   mTeamAlphaLabel.init(this, "alphaField");
   mTeamBravoLabel.init(this, "bravoField");

   mHelpText2.init(this, "mHelpText2");
   mSecondaryTitle.init(this, "mSecondaryTitle");

   mTeamAlphaLabel.setText(gDatabase.getLocStringFromID(25296));
   mTeamBravoLabel.setText(gDatabase.getLocStringFromID(25297));

   // --------- BUTTON BAR
   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   mLastBumperKeyInputTime = 0;

   return true;
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::cancelSubMenuIfUp()
{
   if (mPartyRoomView != BUIMPSetupScreen::cPRViewSubMenu)
      return;

   setView(BUIMPSetupScreen::cPRViewMenu);
   mSecondaryMenu.hide();
   updateHelp();
   displayButtons();
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::reset()
{
   if (mpGamesList)
      mpGamesList->hide();

   mSecondaryMenu.hide();
   mPlayerMenu.hide();
}

//==============================================================================
//==============================================================================
bool BUIMPSetupScreen::handleUIControlEvent( BUIControlEvent& event )
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
                     setView(BUIMPSetupScreen::cPRViewMenu);
                     mSecondaryMenu.hide();
                     updateHelp();
                     displayButtons();
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

                  setView(BUIMPSetupScreen::cPRViewMenu);
                  mSecondaryMenu.hide();
                  updateHelp();
                  displayButtons();
                  return true;

               }
               break;

            case cControlIDPlayerMenu:
               {
                  if (event.getString()=="cancel")
                  {
                     gUI.playCancelSound();
                     setView(BUIMPSetupScreen::cPRViewPlayerList);      
                     mPlayerMenu.hide();
                     updateHelp();
                     displayButtons();
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

                  uint8 slot = (int8)menuItemControl->getData();
                  BPartySessionPartyMember* pMember = mpMode->getPlayerBySlot(slot);

                  switch (c->getControlID())
                  {
                     case cPlayerMenuCommandChangeSettings:
                        {
                           // push back to the player list view (this is so an accept, cancel gets fixed up properly in re. views)
                           setView(BUIMPSetupScreen::cPRViewPlayerList);      
                           mPlayerMenu.hide();
                           if (pMember)
                              mpMode->editPlayer(pMember);     // edit the player'

                           updateHelp();
                           displayButtons();
                        }
                        break;
                     case cPlayerMenuCommandViewGamerCard:
                        {
                           setView(BUIMPSetupScreen::cPRViewPlayerList);      
                           mPlayerMenu.hide();
                           if (pMember)
                              XShowGamerCardUI(gUserManager.getPrimaryUser()->getPort(), pMember->mXuid);
                           updateHelp();
                           displayButtons();
                        }
                        break;
                     case cPlayerMenuCommandAddAI:
                        {
                           if (!pMember)
                              mpMode->addAIPlayer(slot);
                           setView(BUIMPSetupScreen::cPRViewPlayerList);      
                           mPlayerMenu.hide();
                           updateHelp();
                           displayButtons();
                        }
                        break;
                     case cPlayerMenuCommandKick:
                     case cPlayerMenuCommandKickAI:
                        {
                           mpMode->kickPlayer(pMember);
                           setView(BUIMPSetupScreen::cPRViewPlayerList);      
                           mPlayerMenu.hide();
                           updateHelp();
                           displayButtons();
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
                     setView(BUIMPSetupScreen::cPRViewPlayerList);
                     
                     // change the focus to slot 0
                     setPlayerCurrentSlotFocus(0);
                     refreshPlayerSlots();
                     mMainMenu.setIndex(-1);                     
                     
                     resetPlayerInput();
                     updateHelp();
                     displayButtons();
                     return true;
                  }

                  // see if it was one of the children

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

                  if (!mpMode->isOkToChangeGameSettingsInUI())
                  {
                     gUI.playCancelSound();
                     setInfoText(gDatabase.getLocStringFromID(25463));
                     return true;
                  }

                  switch (c->getControlID())
                  {
                     // all menus
                     case cMenuCommandSetLobby:
                        populateLobbyMenu(mpMode->getHostSettings(), mpMode->getUseLanMode());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;
               
                     case cMenuCommandLivePartyType:
                        populatePartyType(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;


                     // custom menu

                     case cMenuCommandSetMap:
                        populateRandomMapMenu(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;
                     case cMenuCommandSetPlayerCount:
                        populatePlayerCountMenu(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;
                     case cMenuCommandSetTeamType:
                        populateTeamMenu(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;
                     case cMenuCommandSetGameMode:
                        populateGameModes(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;
                     case cMenuCommandSetAIDifficulty:
                        populateDifficultyMenu(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;

                        // matchmaking
                     case cMenuCommandSetMatchmakingHopper:
                        populateMatchmakingHopperMenu(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;

                     // Campaign
                     case cMenuCommandSetCampaignMission:
                        populateCampaignMissionPicker(mpMode->getHostSettings());
                        setView(BUIMPSetupScreen::cPRViewSubMenu); 
                        positionMovieClip(c->getControlPath().getPtr(), mSecondaryMenu.getControlPath().getPtr());
                        mSecondaryMenu.show();
                        updateHelp();
                        displayButtons();
                        break;
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
                     // move back up to the menu
                     if (!isLocalPlayerHost())
                        return false;

                     setView(BUIMPSetupScreen::cPRViewMenu);
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
                     updateHelp();
                     displayButtons();
                  }
                  return true;
                  break;

               case BUIGridControl::eSelectionChanged:
                  int row = -1;
                  int col = -1; 
                  if (!mGridControl.getSelectedCell(row, col))
                     break;

                  int slot = (3*col)+row;
                  setPlayerCurrentSlotFocus((int8)slot);
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
bool BUIMPSetupScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
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
   if (command=="inviteFriends")                
   {
      if (isLocalPlayerReady())
         return false;

      if (mpMode->getUseLanMode())
         return false;

      if (!haveRoomToInvitePlayers())
         return false;

      BUser* user = gUserManager.getPrimaryUser();
      if (!user)
         return false;

      if (!user->checkPrivilege(XPRIVILEGE_COMMUNICATIONS))
         return false;

      XShowGameInviteUI( user->getPort(), NULL, 0, NULL);
      handled=true;
   }
   // -------------------- EVENT - incrementLeader --------------------
   else if (command=="incrementLeader")                
   {
/*
      // removed for the time being


      if (mpMode->getPartyRoomMode()==BModePartyRoom2::cPartyRoomModeCampaign)
         return true;   // eat it

      if (event == cInputEventCommandRepeat)
      {
         mpMode->editLocalPlayer();
         handled=true;
      }
      else
      {
         mpMode->incrementLeader();
         handled=true;
      }
*/
   }
   // -------------------- EVENT - decrementLeader --------------------
   else if (command=="decrementLeader")                
   {
/*
      removed for the time being
      if (mpMode->getPartyRoomMode()==BModePartyRoom2::cPartyRoomModeCampaign)
         return true;   // eat it

      if (event == cInputEventCommandRepeat)
      {
         mpMode->editLocalPlayer();
         handled=true;
      }
      else
      {
         mpMode->decrementLeader();
         handled=true;
      }
*/
   }   
   // -------------------- EVENT - changeTeamLeft --------------------
   else if (command=="changeTeamLeft")                
   {
      //No team switching for the E3 demo
      if (timeGetTime()-100>mLastBumperKeyInputTime)
      {
         mLastBumperKeyInputTime = timeGetTime();
         mpMode->changeTeamLeft();
      }
      handled = true;
   }
   // -------------------- EVENT - changeTeamRight --------------------
   else if (command=="changeTeamRight")                
   {
      //No team switching for the E3 demo
      if (timeGetTime()-100>mLastBumperKeyInputTime)
      {
         mLastBumperKeyInputTime = timeGetTime();
         mpMode->changeTeamRight();
      }
      handled = true;
   }
   // -------------------- EVENT - kick --------------------
   else if (command == "kick")                
   {
      mpMode->kickPlayer();
      handled = true;
   }
   // -------------------- EVENT - ready --------------------
   else if (command == "ready")                
   {
      //if (isLocalPlayerReady())
      //   return false;
      const BPartySessionPartyMember* pPartyMember = getLocalMember();
      if (!pPartyMember || 
         (pPartyMember->mSettings.mConnectionState != cBPartySessionMemberConnectionStateConnected))
      {
         return false;
      }
      gUI.playConfirmSound();
      mSecondaryMenu.hide();
      mpMode->onReady();
      handled = true;
   }
   // -------------------- EVENT - cancel --------------------
   else if (command == "cancel")                
   {
      if (isLocalPlayerReady())
      {
         mpMode->onUnready();
      }
      else
      {
         gUI.playCancelSound();
         mpMode->exitParty();
      }

      gUI.playCancelSound();
      handled = true;
   }
   // -------------------- EVENT - playerOptions --------------------
   else if (command == "playerOptions")                
   {
      if (isLocalPlayerReady())
         return false;

      // either a) nothing, b) edit my options, c) bring up gamertag window
      BPartySessionPartyMember* pFocusMember = mpMode->getPlayerBySlot(getPlayerCurrentSlotFocus());
      BPartySessionPartyMember* pLocalMember = getLocalMember();

      if (gConfig.isDefined(cConfigPlayerMenu))
      {
         int8 slot = getPlayerCurrentSlotFocus();
         if (slot==-1)
         {
            //No focus - so don't show menu - just eat the command
            return true;
         }
         
         if (pFocusMember && (pFocusMember->mXuid == pLocalMember->mXuid) && (!mpMode->isOkToShowLocalPlayerOptionsInUI()))
         {
            //It is focused on my local member but I'm not in a state where I can let them change the leader - so eat this command
            return true;
         }  

         if (populatePlayerMenu(pFocusMember, slot))
         {
            gUI.playConfirmSound();
            setView(BUIMPSetupScreen::cPRViewPlayerMenu);
            updateHelp();
            displayButtons();
            mPlayerMenu.show();
         }
      }
      else 
      {         
         if (pFocusMember)
         {
            // edit or gamer tag
            if (pLocalMember && (pFocusMember->mXuid == pLocalMember->mXuid))
            {                
               // it's me, I can edit me
               mpMode->editPlayer(pLocalMember);
            }
            else
            {
               // it's somebody else, I can check out their gamer tag
               XShowGamerCardUI(gUserManager.getPrimaryUser()->getPort(), pFocusMember->mXuid);
            }
         }
      }
   }

   return handled;
}


//==============================================================================
// BUIMPSetupScreen::handleInput
//==============================================================================
bool BUIMPSetupScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   BUser* pPrimaryUser = gUserManager.getPrimaryUser();

   if (pPrimaryUser->getPort() != port)
   {
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
      case BUIMPSetupScreen::cPRViewPlayerEdit:
         // This isn't handled here.
         return false;
         break;
      case BUIMPSetupScreen::cPRViewMenu:
         // let the screen catch it's globals first
         handled = mMainMenu.handleInput(port, event, controlType, detail);
         break;
      case BUIMPSetupScreen::cPRViewSubMenu:
         handled = mSecondaryMenu.handleInput(port, event, controlType, detail);
         return handled;      // the screen doesn't get to see this input
         break;
      case BUIMPSetupScreen::cPRViewPlayerMenu:
         handled = mPlayerMenu.handleInput(port, event, controlType, detail);
         return handled;      // the screen doesn't get to see this input
         break;
      case BUIMPSetupScreen::cPRViewPlayerList:
         handled = mGridControl.handleInput(port, event, controlType, detail);
         if (handled)
            return handled;
         break;
      case BUIMPSetupScreen::cPRViewLanGameList:
         //Only let the main screen handler function when we don't have a party session running (so you can cancel to get out)
         handled = mpGamesList->handleInput(port, event, controlType, detail);
         return handled;      // the screen doesn't get to see this input
         break;
   }

   if (!handled)
      handled = __super::handleInput(port, event, controlType, detail);

   if (handled)
      return handled;

   return false;
}



//==============================================================================
// BUIMPSetupScreen::setImage
//==============================================================================
void BUIMPSetupScreen::setImage(const char * imageURL)
{
   if (mLastMapImage.compare(imageURL) == 0)
      return;

   mLastMapImage.set(imageURL);

   mImageViewer.clearImages();
   mImageViewer.addImage(imageURL);
   mImageViewer.start();
}

//==============================================================================
// BUIMPSetupScreen::setTitle
//==============================================================================
void BUIMPSetupScreen::setTitle(const BUString& pTitle)
{
   GFxValue values[1];
   values[0].SetStringW(pTitle.getPtr());
   mpMovie->invokeActionScript("setTitle", values, 1);
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::positionMovieClip(const char* firstMC, const char* secondMC )
{
   GFxValue values[2];
   values[0].SetString(firstMC);
   values[1].SetString(secondMC);
   mpMovie->invokeActionScript("positionMovieClip", values, 2);
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::update( float dt )
{
   // note, since this is pregame, dt is 0;

   DWORD currentTime = timeGetTime();

   // 3 seconds passed?
   if ( !mpMode->getUseLanMode() && ( (mLastPopUpdateTime+3000) < currentTime ) )
   {
      updatePopulation();
      mLastPopUpdateTime=currentTime;
   }

   if ( (mHelpText2.getText().length() > 0) && ( (mLastHelp2Clear + 3000) < currentTime) )
   {
      BUString clear;
      clear.set(L"");
      mHelpText2.setText(clear);
   }

   if (mLastGamerPicUpdate < gGamerPicManager.getLastUpdate())
   {
      mLastGamerPicUpdate=timeGetTime();
      int slot = 0;
      if (getPartySession() != NULL)
      {
         for (slot = 0; slot < cPlayerSlotCount; slot++)
         {
            BPartySessionPartyMember* pMember = mpMode->getPlayerBySlot(slot);
            if (pMember != NULL)
            {
               const BGamerPic* gamerPic = gGamerPicManager.getGamerPic(pMember->mXuid);
               if (gamerPic != NULL)
               {
                  BUIGamerTagLongControl& playerControl = mPlayerControls[slot].getGamerTag();
                  BUString temp;
                  temp.format(L"img://gamerPic:%I64x", pMember->mXuid);
                  playerControl.setGamerPic(temp);
               }
            }
         }
      }
   }

}


//==============================================================================
//==============================================================================
void BUIMPSetupScreen::updatePopulation()
{
   BUString popString;

   if (!gLiveSystem->getHopperList() ||
       (gLiveSystem->getHopperList()->getStaticData()->mLowPopulationLimit==0))
   {
      popString = "";
   }
   else
   {
      popString.locFormat(gDatabase.getLocStringFromID(25576), gLiveSystem->getHopperList()->getDynamicData()->mGlobalUserCount);
   }

   mSecondaryTitle.setText(popString);
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::setInfoText(const BUString& infoText) 
{ 
   mHelpText2.setText(infoText); 
   mLastHelp2Clear = timeGetTime();    // start our timer
}


//==============================================================================
// BUIMPSetupScreen::setHelpText
//==============================================================================
void BUIMPSetupScreen::setHelpText(const BUString& helpText)
{
   GFxValue values[1];
   values[0].SetStringW(helpText.getPtr());
   mpMovie->invokeActionScript("setHelpText", values, 1);
}


//==============================================================================
void BUIMPSetupScreen::setPlayerCurrentSlotFocus(int8 newSlot)
{
   // break the slot into a row, column
   int col = (int)(newSlot/3);
   int row = newSlot%3;

   // set the focus on the new player slot
   mGridControl.setSelectedCell(row,col);
}

//==============================================================================
void BUIMPSetupScreen::setPlayerNavigationEnabled(bool value)
{
   mGridControl.setAllowNavigation(value);
}

//==============================================================================
void BUIMPSetupScreen::setPlayerVoiceStates(int slot0, int slot1, int slot2, int slot3, int slot4, int slot5)
{
   BUIGamerTagLongControl& c0 = mPlayerControls[0].getGamerTag();
   c0.setSpeakerState(slot0);
   BUIGamerTagLongControl& c1 = mPlayerControls[1].getGamerTag();
   c1.setSpeakerState(slot1);
   BUIGamerTagLongControl& c2 = mPlayerControls[2].getGamerTag();
   c2.setSpeakerState(slot2);
   BUIGamerTagLongControl& c3 = mPlayerControls[3].getGamerTag();
   c3.setSpeakerState(slot3);
   BUIGamerTagLongControl& c4 = mPlayerControls[4].getGamerTag();
   c4.setSpeakerState(slot4);
   BUIGamerTagLongControl& c5 = mPlayerControls[5].getGamerTag();
   c5.setSpeakerState(slot5);
}

//==============================================================================
BUIPlayer* BUIMPSetupScreen::getPlayer(uint8 slot)
{
   return &mPlayers[slot];
}

//==============================================================================
void BUIMPSetupScreen::clearPlayerSlot(uint8 slot)
{
   mPlayers[slot].mSlotType=BUIPlayer::cEmpty;
}

//==============================================================================
void BUIMPSetupScreen::initializePlayerSlots()
{
   // fixme - do I still need this?
}

//==============================================================================
void BUIMPSetupScreen::setPlayerSlotActive(uint8 slot, bool active)
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
int8 BUIMPSetupScreen::getPlayerCurrentSlotFocus()
{
   int row=-1;
   int col=-1;
   if (mGridControl.getSelectedCell(row, col))
   {
      int slot = (3*col)+row;
      return (int8)slot;
   }
   return -1;
}

//==============================================================================
void BUIMPSetupScreen::resetPlayerInput()
{
   // fixme 
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::updatePlayerSlot(uint8 slot)
{
   BUIPlayer& player = mPlayers[slot];
   if (!player.mActive)
      return;

   if (player.mSlotType==BUIPlayer::cMatchmaking)
   {
      // Note: we matchmake with human players only, so using the mGamerTag without checking for AI type is OK.
      setPlayerSlotDataMatchmaking(slot, player.mGamerTag, player.mReady);
   }
   else
   {
      // display the player
      setPlayerSlotData(slot);
   }
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::setPlayerSlotData(uint8 slot)
{
//-- FIXING PREFIX BUG ID 3652
   const BUIPlayer& player = mPlayers[slot];
//--

   // bool isLocal = (player.mSlotType==BUIPlayer::cLocalHuman);

   BUIGamerTagLongControl& playerControl = mPlayerControls[slot].getGamerTag();

   // The player can be a human or an AI, display special string if it's an AI.
   if (player.mSlotType == BUIPlayer::cAI)
      playerControl.setGamerTag(gDatabase.getLocStringFromID(25940));
   else
      playerControl.setGamerTag(player.mGamerTag);


   playerControl.setPing(player.mPing);

   BProfileRank rank(player.mRank);
   playerControl.setRank(rank.mRank);

   BPartySessionPartyMember* pMember = mpMode->getPlayerBySlot(slot);

   if (pMember)
   {
      BUString temp;
      temp.format(L"img://gamerPic:%I64x", pMember->mXuid);
      playerControl.setGamerPic(temp);
   }

   playerControl.setPort(player.mControllerPort);

   if (mpMode->getPartyRoomMode() != BModePartyRoom2::cPartyRoomModeCampaign)
   {
      // leader info
      const BLeader* pLeader=gDatabase.getLeader(player.mLeader);   //-- FIXING PREFIX BUG ID 3653
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
   }
   else
   {
      // this is a render hack for the campaign party mode
      BString tempCivURL;
      BUString tempLeaderString;

      tempCivURL.set("img://art\\ui\\flash\\shared\\textures\\leaders\\LeaderPict_random_unsc.ddx");
      tempLeaderString.set(gDatabase.getLocStringFromID(25947));     // "Campaign"

      playerControl.setLeaderPic(tempCivURL);
      playerControl.setTitleText(tempLeaderString);
   }

   playerControl.setHost(player.mHost);
   playerControl.setReady(player.mReady);
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::setPlayerSlotDataMatchmaking(uint8 slot, BUString& gamerTag, bool ready)
{
   // Note: the gamerTag is guaranteed to be a user gamertag and therefore OK because they are limited to ASCII.
   BUIGameSetupPlayerControl& player = mPlayerControls[slot];
   player.showSpinner(!ready);   

   BUIGamerTagLongControl& playerControl = mPlayerControls[slot].getGamerTag();
   playerControl.setGamerTag(gamerTag);
   playerControl.show(true);

   BUIPlayer& uiplayer = mPlayers[slot];
   uiplayer.mSlot = slot;
   uiplayer.mSlotType=BUIPlayer::cMatchmaking;
   setPlayerSlotActive(slot, true);
}

//==============================================================================
// BUIPlayerList::resetSlotStates
//==============================================================================
void BUIMPSetupScreen::resetSlotStates()
{
   for (uint i=0; i<cPlayerSlotCount; i++)
   {
      BUIGameSetupPlayerControl& player = mPlayerControls[i];
      player.showSpinner(false);
      clearPlayerSlotData((uint8)i);
      mSlotStates[i]=cPRSlotStateEmpty;
   }
}

//==============================================================================
// BUIPlayerList::setSlotState
//==============================================================================
void BUIMPSetupScreen::setSlotState(uint8 slot, uint8 slotState)
{
   mSlotStates[slot]=slotState;
}

//==============================================================================
// BUIPlayerList::getSlotTransitionState
//==============================================================================
uint8 BUIMPSetupScreen::getSlotTransitionState(uint8 slot, bool inCenter)
{
   uint8 currentState = mSlotStates[slot];
   /*
                     Empty      Normal    Center   
   --------------+---------------------------------------
   Empty         |   Empty      ease_in   CENTER   
   Normal        |   NORMAL     NORMAL    N2NC           
   Center        |   CENTER     C2N       CENTER   
   --------------+---------------------------------------

   */
   //                      [Current State]      [Next State]
   uint8 stateTransitions[cPRSlotStateCount][cPRSlotStateCount]=
   {
      // Empty                   Normal            NormalCenter            
      {  cPRSlotKF_Empty,        cPRSlotKF_EaseIn, cPRSlotKF_Center  },
      {  cPRSlotKF_Normal,       cPRSlotKF_Normal, cPRSlotKF_N2C     },
      {  cPRSlotKF_Center,       cPRSlotKF_C2N,    cPRSlotKF_Center  },
   };

   // determine our next state
   uint8 nextState = cPRSlotStateNormal;
   if (inCenter)
      nextState = cPRSlotStateCenter;

   if (currentState == nextState)
      return 0;

   // save it
   setSlotState(slot, nextState);

   // lookup the transition that we should play
   uint8 transition = stateTransitions[currentState][nextState];

   return transition;
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::refreshPlayerSlots()
{
   mLastGamerPicUpdate=timeGetTime();

   for (uint8 slot=0; slot<cPlayerSlotCount; slot++)
   {
//-- FIXING PREFIX BUG ID 3655
      const BUIPlayer& player = mPlayers[slot];
//--
      if (!player.mActive)
         continue;

      BUIGameSetupPlayerControl& playerControl=mPlayerControls[slot];

      bool inCenter=false;
      if (player.mSlotType==BUIPlayer::cEmpty)
      {
         // clear the player
         clearPlayerSlotData(slot);
      }
      else
      {
         updatePlayerSlot(slot);
         inCenter = player.mInCenter;
      }

      uint8 slotTransitionState = getSlotTransitionState(slot, inCenter);

      if (slotTransitionState != 0)
      {
//-- FIXING PREFIX BUG ID 3654
         const BString& keyFrame = mSlotKeyFrames[slotTransitionState];
//--
         playerControl.playTransition(keyFrame.getPtr());
      }
   }
}

//==============================================================================
// BUIPlayerList::clearPlayerSlotData
//==============================================================================
void BUIMPSetupScreen::clearPlayerSlotData(uint8 slot)
{
   BUIGamerTagLongControl& player = mPlayerControls[slot].getGamerTag();
   player.clear();
   player.setGamerTag(gDatabase.getLocStringFromID(25937));
}



//==============================================================================
//==============================================================================
void BUIMPSetupScreen::populateGamesList()
{
   BMPSession* pMPSession = gLiveSystem->getMPSession();
   if (!pMPSession)
      return;

   BDynamicSimArray<BLanGameInfo>* pLanList = pMPSession->getLanGamesList();
   if (pLanList == NULL)
      return;

   uint lanGameCount = pLanList->getSize();
   int currentFocus = mpGamesList->getFocus();
   mpGamesList->setMenuStripCount(1);
   BUIMenuStrip* pMenuStrip = mpGamesList->getMenuStrip(0);

   // Add the menu items in
   pMenuStrip->setNumberItems(lanGameCount);
   pMenuStrip->setName("LanGames");

   for (uint i=0; i < lanGameCount; i++)
   {
      const BLanGameInfo& lanInfo = pLanList->at(i);

      // Draw the info out.
      // gFontManager.drawText(fontHandle, sx, gamesY, pLanGame->getName());

      BUIMenuItem* pItem=NULL;

      // ----  Item: A Game
      pItem = pMenuStrip->getItem(i);
      pItem->mID = (int8)i;
      pItem->mName="game";

      // if we want to output other information such as the map name, game type, difficulty, etc...
      //const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(lanGame.getMapIndex());

      BUString info;
      if (lanInfo.getBadCRC())
         info.locFormat(gDatabase.getLocStringFromID(25282), lanInfo.getInfo().getPtr());
      else if (lanInfo.getLocked())
         info.locFormat(gDatabase.getLocStringFromID(25283), lanInfo.getInfo().getPtr());
      else
         info.locFormat(gDatabase.getLocStringFromID(25284), lanInfo.getInfo().getPtr(), lanInfo.getFilledSlots(), lanInfo.getMaxSlots());

      pItem->mText = info;
      pItem->mCommandID=i;
      pItem->mCommand="game";
      pItem->mHelpText.set(L"");
   }

   mpGamesList->setActiveStrip(0);

   if (lanGameCount>0)
   {
      if ( (currentFocus>=0) && (currentFocus < static_cast<int>(lanGameCount)))
         mpGamesList->setFocus(currentFocus);
      else
         mpGamesList->setFocus(0);
   }

}


//==============================================================================
//==============================================================================
bool BUIMPSetupScreen::menuEvent(const BSimString& command, BUIMenu* pMenu)
{
   if (pMenu->getMovieClip() == getLanGamesList()->getMovieClip())
   {
      // handle game list events
      if (command == "accept")
      {
         // join the game
//-- FIXING PREFIX BUG ID 3657
         const BUIMenuItem* pItem = pMenu->getCurrentMenuItem();
//--
         if (pItem)
         {
            BMPSession* pMPSession = gLiveSystem->getMPSession();
            if (!pMPSession)
               return false;

            const BLanGameInfo* pLanInfo = pMPSession->getLanGame(static_cast<uint>(pItem->mCommandID));
            if (pLanInfo == NULL)
               return false;

            BUIGlobals* pUIGlobals = gGame.getUIGlobals();
            BASSERT(pUIGlobals);

            if (pLanInfo->getBadCRC())
            {
               if (pUIGlobals)
                  pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25250), BUIGlobals::cDialogButtonsOK);
               return false;
            }
            else if (pLanInfo->getLocked())
            {
               if (pUIGlobals)
                  pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25285), BUIGlobals::cDialogButtonsOK);
               return false;
            }
            else if (pLanInfo->getFilledSlots() == pLanInfo->getMaxSlots())
            {
               if (pUIGlobals)
                  pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25249), BUIGlobals::cDialogButtonsOK);
               return false;
            }

            int index = pItem->mCommandID;
            if (!mpMode->joinInitLan((uint)index))
               return false;

            getLanGamesList()->hide();
         }
      }
      else if (command == "cancel")
      {
         mpMode->exitParty();
      }
      else if (command == "host")
      {
         mpMode->hostInit();
         getLanGamesList()->hide();
      }
      else if (command == "refresh")
      {
         populateGamesList();
         updateHelp();
         displayButtons();
      }
   }
   return true;
}


//==============================================================================
// BModePartyRoom2::populatePlayerMenu();
//==============================================================================
bool BUIMPSetupScreen::populatePlayerMenu(BPartySessionPartyMember* pMember, int8 slot)
{
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   if (!pMember)
   {
      // we can't do anything here.
      if (!isLocalPlayerHost())
      {
         // sound?
         return false;
      }

      //Host can't add AI if not in custom game mode
      if (mpMode->getHostSettings()->mPartyRoomMode != BModePartyRoom2::cPartyRoomModeCustom)
      {
         return false;
      }

      // no person in the slot, if Host, then I can add somebody
      menuText.set( gDatabase.getLocStringFromID(25311) );  // add ai player
      helpText.set(L"");
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
      pMenuItem->setControlID(BUIMPSetupScreen::cPlayerMenuCommandAddAI);     // command id
      pMenuItem->setText(menuText);
      pMenuItem->setHelpText(helpText);
      pMenuItem->setData(slot);
      pMenuItem->enable();
      pMenuItem->show();
   }
   else if (pMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberAI)
   {
      if (!isLocalPlayerHost())
      {
         // sound?
         return false;
      }

      // AI in the slot, if I'm the host, I can work on him.
      menuText.set( gDatabase.getLocStringFromID(25312) );  // drop player
      helpText.set(L"");
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
      pMenuItem->setControlID(BUIMPSetupScreen::cPlayerMenuCommandKickAI);     // command id
      pMenuItem->setText(menuText);
      pMenuItem->setHelpText(helpText);
      pMenuItem->setData(slot);
      pMenuItem->enable();
      pMenuItem->show();

      menuText.set( gDatabase.getLocStringFromID(25313) );  // change settings
      helpText.set(L"");
      pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
      pMenuItem->setControlID(BUIMPSetupScreen::cPlayerMenuCommandChangeSettings);     // command id
      pMenuItem->setText(menuText);
      pMenuItem->setHelpText(helpText);
      pMenuItem->setData(slot);
      pMenuItem->enable();
      pMenuItem->show();
   }
   else if (isLocalPlayer(pMember))
   {
      bool haveItem = false;
      if (mpMode->getPartyRoomMode() != BModePartyRoom2::cPartyRoomModeCampaign)
      {
         // me in the slot, I can always change my settings
         menuText.set( gDatabase.getLocStringFromID(25313) );  // change settings
         helpText.set(L"");
         pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
         pMenuItem->setControlID(BUIMPSetupScreen::cPlayerMenuCommandChangeSettings);     // command id
         pMenuItem->setText(menuText);
         pMenuItem->setHelpText(helpText);
         pMenuItem->setData(slot);
         pMenuItem->enable();
         pMenuItem->show();
         haveItem=true;
      }

      // Always view my gamer card
      if (!mpMode->getUseLanMode())
      {
         menuText.set( gDatabase.getLocStringFromID(25316) );  // view gamer card
         helpText.set(L"");
         pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
         pMenuItem->setControlID(BUIMPSetupScreen::cPlayerMenuCommandViewGamerCard);     // command id
         pMenuItem->setText(menuText);
         pMenuItem->setHelpText(helpText);
         pMenuItem->setData(slot);
         pMenuItem->enable();
         pMenuItem->show();
         haveItem=true;
      }

      if (!haveItem)
         return false;
   }
   else
   {
      bool haveItem = false;
      // other human in the slot, can view his gamer card
      if (!mpMode->getUseLanMode())
      {
         menuText.set( gDatabase.getLocStringFromID(25316) );  // view gamer card
         helpText.set(L"");
         pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
         pMenuItem->setControlID(BUIMPSetupScreen::cPlayerMenuCommandViewGamerCard);     // command id
         pMenuItem->setText(menuText);
         pMenuItem->setHelpText(helpText);
         pMenuItem->setData(slot);
         pMenuItem->enable();
         pMenuItem->show();
         haveItem=true;
      }

      if (isLocalPlayerHost())
      {
         // if I'm the host, I can also kick this player.
         menuText.set( gDatabase.getLocStringFromID(25312) );  // drop player
         helpText.set(L"");
         pMenuItem=(BUIMenuItemControl*)mPlayerMenu.getControl(index++);
         pMenuItem->setControlID(BUIMPSetupScreen::cPlayerMenuCommandKick);     // command id
         pMenuItem->setText(menuText);
         pMenuItem->setHelpText(helpText);
         pMenuItem->setData(slot);
         pMenuItem->enable();
         pMenuItem->show();
         haveItem=true;
      }

      if (!haveItem)
         return false;

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

   return true;
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::addMenuItem(long index, BUIMenuStrip* pMenuStrip, const WCHAR* text, int commandID, int8 id)
{
   BUIMenuItem* pItem=NULL;

   pItem = pMenuStrip->getItem(index);
   if(pItem)
   {
      pItem->mID = id;                       // Parent Menu ID (or special command)
      pItem->mName="menu";
      pItem->mText.set(text);
      pItem->mCommandID=commandID;           // Next state for mode menu
      pItem->mCommand="menu";
   }
}


//==============================================================================
// BModePartyRoom2::isLocalPlayer
// FIXME-COOP - calls made to isLocalPlayer need to be aware that you could
//              have two local players
//==============================================================================
BOOL BUIMPSetupScreen::isLocalPlayer(BPartySessionPartyMember* pPartyMember) const
{
   BASSERT(pPartyMember);

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   if (!pParty || !pParty->getSession())
   {
      return FALSE;
   }

   //the host is the local player for AIs
   if (pPartyMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI)
   {
      return (isLocalPlayerHost());
   }

   // is this the local player?
   return (pParty->getSession()->isLocalClientID(pPartyMember->mClientID));
}

//==============================================================================
// helper method
//==============================================================================
BPartySession* BUIMPSetupScreen::getPartySession() const
{
   BMPSession* pMPSession = gLiveSystem->getMPSession();
   if (!pMPSession)
   {
      return NULL;
   }
   BPartySession* pPartySession = pMPSession->getPartySession();

   return pPartySession;
}

//==============================================================================
// BModePartyRoom2::isLocalPlayerHost
//==============================================================================
BOOL BUIMPSetupScreen::isLocalPlayerHost() const
{
//-- FIXING PREFIX BUG ID 3659
   const BPartySession* pParty = getPartySession();
//--
   BASSERT(pParty);
   if (pParty == NULL)
      return FALSE;

   // only a host can do this.
   return pParty->isHosting();
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::populatePartyType(const BPartySessionHostSettings* pHostSettings)
{
   int menuFocus=0;

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

/*
   <String _locID='25419' category='UI' scenario='MPSetupScreen'>OPEN PARTY</String>
      <String _locID='25420' category='UI' scenario='MPSetupScreen'>FRIENDS ONLY</String>
      <String _locID='25421' category='UI' scenario='MPSetupScreen'>INVITE ONLY</String>
*/

   // Item: OPEN PARTY
   menuText=gDatabase.getLocStringFromID(25419);   // OPEN PARTY
   if (pHostSettings->mLiveMode == cBLiveSessionHostingModeOpen)
      menuFocus = index;
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandLivePartyType);        // The command 
   pMenuItem->setText(menuText);
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setData(cBLiveSessionHostingModeOpen);             
   pMenuItem->enable();
   pMenuItem->show();

   // Item: FRIENDS ONLY
   menuText=gDatabase.getLocStringFromID(25420);   // FRIENDS ONLY
   if (pHostSettings->mLiveMode == cBLiveSessionHostingModeFriendsOnly)
      menuFocus = index;
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandLivePartyType);        // The command 
   pMenuItem->setText(menuText);
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setData(cBLiveSessionHostingModeFriendsOnly);
   pMenuItem->enable();
   pMenuItem->show();

   // Item: INVITE ONLY
   menuText=gDatabase.getLocStringFromID(25421);   // OPEN PARTY
   if (pHostSettings->mLiveMode == cBLiveSessionHostingModeInviteOnly)
      menuFocus = index;
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandLivePartyType);        // The command 
   pMenuItem->setText(menuText);
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setData(cBLiveSessionHostingModeInviteOnly);
   pMenuItem->enable();
   pMenuItem->show();

   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }

   mSecondaryMenu.setIndex(menuFocus);
}


//==============================================================================
//==============================================================================
void BUIMPSetupScreen::populateLobbyMenu(const BPartySessionHostSettings* pHostSettings, bool lanMode)
{
   int menuFocus=0;

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   if (!lanMode)
   {
      if (pHostSettings->mPartyRoomMode == BModePartyRoom2::cPartyRoomModeMM)
         menuFocus = index;

      // ----  Item: Lobby 
      menuText=gDatabase.getLocStringFromID(23457);   // matchmaking
      pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
      pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetLobby);     // The command 
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->setData(BModePartyRoom2::cPartyRoomModeMM);               // The data
      pMenuItem->enable();
      pMenuItem->show();
   }

   // Item: custom lobby  
   if (pHostSettings->mPartyRoomMode == BModePartyRoom2::cPartyRoomModeCustom)
      menuFocus = index;
   menuText=gDatabase.getLocStringFromID(23456);   // "Custom";
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetLobby);        // The command 
   pMenuItem->setText(menuText);
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setData(BModePartyRoom2::cPartyRoomModeCustom);              // The data
   pMenuItem->enable();
   pMenuItem->show();

   // Item3: campaign lobby
   if (pHostSettings->mPartyRoomMode == BModePartyRoom2::cPartyRoomModeCampaign)
      menuFocus = index;

   menuText=gDatabase.getLocStringFromID(23458);   // Campaign
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetLobby);        // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(BModePartyRoom2::cPartyRoomModeCampaign);            // The data
   pMenuItem->enable();
   pMenuItem->show();

   // hide the rest
   for (int i=index; i<cMaxSecondaryMenuItems; i++)
   {
      BUIControl* c=mSecondaryMenu.getControl(i);
      c->disable();
      c->hide();
   }

   mSecondaryMenu.setIndex(menuFocus);
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::populateRandomMapMenu(const BPartySessionHostSettings* pHostSettings)
{

   // get the current map so we can set the proper focus after we show
   uint8 currentMap = pHostSettings->mMapIndex;

   int validMapCount=0;
   for (int i=0; i<gDatabase.getScenarioList().getMapCount(); i++)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(i);
      BASSERT(pMap);
      if (!isValidMapType(pMap->getType()))
         continue;

      if (!isValidMap((uint8)i, pHostSettings->mNumPlayers))
         continue;

      validMapCount++;
   }


//----------------------
   // Now actually populate the menu
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;
   int itemNumber=0;
   int itemWithFocus=0;
   for (int i=0; i<gDatabase.getScenarioList().getMapCount(); i++)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(i);
      BASSERT(pMap);
      if (!isValidMapType(pMap->getType()))
         continue;

      if (!isValidMap((uint8)i, pHostSettings->mNumPlayers))
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
      pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetMap);       // The command 
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
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
void BUIMPSetupScreen::populateDifficultyMenu(const BPartySessionHostSettings* pHostSettings)
{

   int index = 0;
   int focus = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   if (pHostSettings->mDifficulty==DifficultyType::cEasy)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cEasy));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cEasy);               // The data
   pMenuItem->enable();
   pMenuItem->show();


   if (pHostSettings->mDifficulty==DifficultyType::cNormal)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cNormal));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cNormal);               // The data
   pMenuItem->enable();
   pMenuItem->show();


   if (pHostSettings->mDifficulty==DifficultyType::cHard)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cHard));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cHard);               // The data
   pMenuItem->enable();
   pMenuItem->show();


   if (pHostSettings->mDifficulty==DifficultyType::cLegendary)
      focus = index;
   menuText.set(gDatabase.getDifficultyStringByType(DifficultyType::cLegendary));
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetAIDifficulty);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(DifficultyType::cLegendary);               // The data
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
void BUIMPSetupScreen::populateGameModes(const BPartySessionHostSettings* pHostSettings)
{

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;
   int count = gDatabase.getNumberGameModes();

   for (int i=0; i<count; i++)
   {
//-- FIXING PREFIX BUG ID 3660
      const BGameMode* gameMode = gDatabase.getGameModeByID(i);
//--
      if (!gameMode)
         continue;

      menuText=gameMode->getDisplayName();
      pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
      pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetGameMode);     // The command 
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
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

   int currentGameMode = pHostSettings->mGameMode;
   if (currentGameMode<count)
      mSecondaryMenu.setIndex(currentGameMode);
   else
      mSecondaryMenu.setIndex(0);
}


//==============================================================================
// BModePartyRoom2::
//==============================================================================
void BUIMPSetupScreen::populatePlayerCountMenu(const BPartySessionHostSettings* pHostSettings)
{
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   menuText=gDatabase.getLocStringFromID(23452);   // 1v1
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetPlayerCount);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(2);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   menuText=gDatabase.getLocStringFromID(23453);   // 2v2
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetPlayerCount);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(4);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   menuText=gDatabase.getLocStringFromID(23454);   // 3v3
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetPlayerCount);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
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
   switch (pHostSettings->mNumPlayers)
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
void BUIMPSetupScreen::populateTeamMenu(const BPartySessionHostSettings* pHostSettings)
{
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   menuText=gDatabase.getLocStringFromID(23497);   // Alpha vs Bravo
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetTeamType);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->setData(cCustomTeamTypeTeams);               // The data
   pMenuItem->enable();
   pMenuItem->show();

   menuText=gDatabase.getLocStringFromID(23496);   // Random
   pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetTeamType);     // The command 
   helpText.set(L"");
   pMenuItem->setHelpText(helpText);
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

}

//==============================================================================
//==============================================================================
bool BUIMPSetupScreen::isValidMapType(long mapType)
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
bool BUIMPSetupScreen::isValidMap(uint8 mapIndex, uint8 numPlayers, bool matchPlayersExactly)
{
   const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapIndex);
   BASSERT(pMap);

   if (gConfig.isDefined(cConfigDemo))
   {
      BSimString tutorialMap;
      gConfig.get(cConfigTutorialMap, tutorialMap);
      tutorialMap+=".scn";
      if (pMap->getFilename() == tutorialMap)
         return false;
   }

   uint8 mapMaxPlayers=(uint8)pMap->getMaxPlayers();

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
void BUIMPSetupScreen::populateCampaignModeMenu(const BPartySessionHostSettings* pHostSettings)
{
   int itemWithFocus = mMainMenu.getSelectedIndex();

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   // ----  Item: Lobby 
   menuText.locFormat(gDatabase.getLocStringFromID(25315).getPtr(), gDatabase.getLocStringFromID(23458).getPtr() );
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetLobby);     // command id
   helpText.set(gDatabase.getLocStringFromID(25547).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();

   // if not in lan mode, add this in
   if (!mpMode->getUseLanMode())
   {
      // ---- Item : Party Type
      switch(pHostSettings->mLiveMode)
      {
      case cBLiveSessionHostingModeOpen:
         menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25419).getPtr() );
         break;
      case cBLiveSessionHostingModeFriendsOnly:
         menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25420).getPtr() );
         break;
      case cBLiveSessionHostingModeInviteOnly:
         menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25421).getPtr() );
         break;
      }
      pMenuItem=mMenuItems[index++];
      pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandLivePartyType);     // command id
      helpText.set(gDatabase.getLocStringFromID(25548).getPtr());
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->enable();
      pMenuItem->show();

   }


   // ---- Item: Mission Picker
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   int currentMission = pHostSettings->mMapIndex;
   BCampaignNode* pNode = pCampaign->getNode(currentMission);
   if (pNode)
      menuText.locFormat(gDatabase.getLocStringFromID(25317).getPtr(), pNode->getDisplayName().getPtr());
   else
      menuText.set(gDatabase.getLocStringFromID(25318));

   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetCampaignMission);     // command id
   helpText.set(gDatabase.getLocStringFromID(25553).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();

   // ---- Item: AI Difficulty
   switch (pHostSettings->mDifficulty)
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
      menuText.set(gDatabase.getLocStringFromID(25310).getPtr());
      break;
   }
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetAIDifficulty);     // command id
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
// BUIMPSetupScreen::populateMatchmakingModeMenu
//==============================================================================
void BUIMPSetupScreen::populateMatchmakingModeMenu(const BPartySessionHostSettings* pHostSettings)
{
   int itemWithFocus = mMainMenu.getSelectedIndex();

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   // ----  Item: Lobby 
   menuText.locFormat(gDatabase.getLocStringFromID(25315).getPtr(), gDatabase.getLocStringFromID(23457).getPtr() );
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetLobby);     // command id
   helpText.set(gDatabase.getLocStringFromID(25547).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();

   // if not in lan mode, add this in
   if (!mpMode->getUseLanMode())
   {
      // ---- Item : Party Type
      switch(pHostSettings->mLiveMode)
      {
      case cBLiveSessionHostingModeOpen:
         menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25419).getPtr() );
         break;
      case cBLiveSessionHostingModeFriendsOnly:
         menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25420).getPtr() );
         break;
      case cBLiveSessionHostingModeInviteOnly:
         menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25421).getPtr() );
         break;
      }
      pMenuItem=mMenuItems[index++];
      pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandLivePartyType);     // command id
      helpText.set(gDatabase.getLocStringFromID(25548).getPtr());
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->enable();
      pMenuItem->show();

   }


   // ---- Item: Hopper
   BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
//-- FIXING PREFIX BUG ID 3661
   const BMatchMakingHopper* pHopper = pHopperList->findHopperByHopperIndex(pHostSettings->mHopperIndex);
//--
   menuText.locFormat(gDatabase.getLocStringFromID(25319).getPtr(), gDatabase.getLocStringFromID(pHopper->mLocStringID).getPtr());
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetMatchmakingHopper);     // command id
   helpText.set(gDatabase.getLocStringFromID(25554).getPtr());
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
// BUIMPSetupScreen::populateCustomModeMenu
//==============================================================================
void BUIMPSetupScreen::populateCustomModeMenu(const BPartySessionHostSettings* pHostSettings)
{
   int itemWithFocus = mMainMenu.getSelectedIndex();

   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   // ----  Item: Lobby 
   menuText.locFormat(gDatabase.getLocStringFromID(25315).getPtr(), gDatabase.getLocStringFromID(23456).getPtr() );
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetLobby);     // command id
   helpText.set(gDatabase.getLocStringFromID(25547).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();


   // if not in lan mode, add this in
   if (!mpMode->getUseLanMode())
   {
      // ---- Item : Party Type
      switch(pHostSettings->mLiveMode)
      {
         case cBLiveSessionHostingModeOpen:
            menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25419).getPtr() );
            break;
         case cBLiveSessionHostingModeFriendsOnly:
            menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25420).getPtr() );
            break;
         case cBLiveSessionHostingModeInviteOnly:
            menuText.locFormat(gDatabase.getLocStringFromID(25429).getPtr(), gDatabase.getLocStringFromID(25421).getPtr() );
            break;
      }
      pMenuItem=mMenuItems[index++];
      pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandLivePartyType);     // command id
      helpText.set(gDatabase.getLocStringFromID(25548).getPtr());
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      pMenuItem->enable();
      pMenuItem->show();

   }

   // ---- Item : Number of Players
   switch(pHostSettings->mNumPlayers)
   {
      case 2:
         menuText.locFormat(gDatabase.getLocStringFromID(25299).getPtr(), gDatabase.getLocStringFromID(23452).getPtr());
         break;
      case 4:
         menuText.locFormat(gDatabase.getLocStringFromID(25299).getPtr(), gDatabase.getLocStringFromID(23453).getPtr());
         break;
      case 6:
         menuText.locFormat(gDatabase.getLocStringFromID(25299).getPtr(), gDatabase.getLocStringFromID(23454).getPtr());
         break;
   }
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetPlayerCount);     // command id
   helpText.set(gDatabase.getLocStringFromID(25549).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();


   // ---- Item: Map
   const BScenarioMap* pScenarioMap = gDatabase.getScenarioList().getMapInfo(pHostSettings->mMapIndex);
   if (pScenarioMap->getMapName().length() > 0)
   {
      if (pScenarioMap->getMaxPlayers() == pHostSettings->mNumPlayers)
         menuText.locFormat(gDatabase.getLocStringFromID(25301).getPtr(), pScenarioMap->getMapName().getPtr());
      else
         menuText.locFormat(gDatabase.getLocStringFromID(25302).getPtr(), pScenarioMap->getMapName().getPtr());
   }
   else
   {
      BSimString mapFilename;
      strPathGetFilename(pScenarioMap->getFilename(), mapFilename);

      BUString mapName;
      mapName.locFormat(L"%S", mapFilename.getPtr());

      // use the filename itself
      if (pScenarioMap->getMaxPlayers() == pHostSettings->mNumPlayers)
         menuText.locFormat(gDatabase.getLocStringFromID(25301).getPtr(), mapName.getPtr());
      else
         menuText.locFormat(gDatabase.getLocStringFromID(25302).getPtr(), mapName.getPtr());
   }
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetMap);     // command id
   helpText.set(gDatabase.getLocStringFromID(25550).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();


   // ---- Item: Game Mode
//-- FIXING PREFIX BUG ID 3662
   const BGameMode* pGameMode = gDatabase.getGameModeByID(pHostSettings->mGameMode);
//--
   if (pGameMode)
      menuText.locFormat(gDatabase.getLocStringFromID(25303).getPtr(), pGameMode->getDisplayName().getPtr() );
   else
      menuText.set(gDatabase.getLocStringFromID(25304).getPtr());
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetGameMode);     // command id
   helpText.set(gDatabase.getLocStringFromID(25551).getPtr());
   pMenuItem->setHelpText(helpText);
   pMenuItem->setText(menuText);
   pMenuItem->enable();
   pMenuItem->show();

   // ---- Item: AI Difficulty
   switch (pHostSettings->mDifficulty)
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
         menuText.set(gDatabase.getLocStringFromID(25310).getPtr());
         break;
   }
   pMenuItem=mMenuItems[index++];
   pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetAIDifficulty);     // command id
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
void BUIMPSetupScreen::populateMatchmakingHopperMenu(const BPartySessionHostSettings* pHostSettings)
{
   // how many menu items do we have?
   BMatchMakingHopperList* pHopperList = gLiveSystem->getHopperList();
   // get a count of all the valid hoppers
   int numItems=0;
   for (int i=0; i<pHopperList->getHopperCount(); i++)
   {
      // set up the text
//-- FIXING PREFIX BUG ID 3663
      const BMatchMakingHopper* pHopper = pHopperList->findHopperByID(i);
//--
      if (!pHopper ||
         (pHopper->mTeamCode==BMatchMakingHopper::cBMatchMakingHopperTeamCodeCustomTeams))         
         continue;
      numItems++;
   }


//----------------------------------------------------------------------------------------------------------
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   int focusStripIndex = 0;
   int index=0;

   for (int i=0; i<pHopperList->getHopperCount(); i++)
   {
//-- FIXING PREFIX BUG ID 3664
      const BMatchMakingHopper* pHopper = pHopperList->findHopperByID(i);
//--
      if (!pHopper ||
         (pHopper->mTeamCode==BMatchMakingHopper::cBMatchMakingHopperTeamCodeCustomTeams))      
         continue;

      //Is this the one I should set focus on after we are loaded up?
      if (pHostSettings->mHopperIndex == pHopper->mListIndex)
         focusStripIndex = index;

      menuText.set(pHopper->getLocString(true));
      pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index++);
      pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetMatchmakingHopper);     // The command 
      helpText.set(L"");
      pMenuItem->setHelpText(helpText);
      pMenuItem->setText(menuText);
      //pMenuItem->setData(i);               // The data
      pMenuItem->setData(pHopper->mListIndex);
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

   mSecondaryMenu.setIndex(focusStripIndex);

}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::populateCampaignMissionPicker(const BPartySessionHostSettings* pHostSettings)
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   // how many menu items do we have?
   int count = pCampaign->getNumberNodes();

   // see how many we want to show in the picker.
   int numItems=0;
   for (int i=0; i<count; i++)
   {
//-- FIXING PREFIX BUG ID 3665
      const BCampaignNode* pNode = pCampaign->getNode(i);
//--
      BASSERT(pNode);

      // for right now, skip cinematics
      if ( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
         continue;

      numItems++;
   }

   int currentMission = pHostSettings->mMapIndex;


//----------------------------------------------------------------------------------------------
   int index = 0;
   BUIMenuItemControl* pMenuItem=NULL;
   BUString menuText;
   BUString helpText;

   int itemWithFocus=0;
   //int itemNumber=0;
   for (int i=0; i<count; i++)
   {
      BCampaignNode* pNode = pCampaign->getNode(i);
      BASSERT(pNode);

      // for right now, skip cinematics
      if ( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
         continue;

      if (currentMission == i)
         itemWithFocus = index;

      // fixme - check to see if the node is unlocked or locked.
      bool unlocked = mpMode->isScenarioUnlocked(pNode);

      if (unlocked)
      {
         // unlocked
         pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index);
         pMenuItem->enable();
         menuText=pNode->getDisplayName();
         pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetCampaignMission);     // The command 
         helpText.set(L"");
         pMenuItem->setHelpText(helpText);
         pMenuItem->setText(menuText);
         pMenuItem->setData(i);               // The data
         pMenuItem->show();
      }
      else
      {
         // locked
         // disable the menu item,
         // show "Locked" in the text
         pMenuItem=(BUIMenuItemControl*)mSecondaryMenu.getControl(index);
         pMenuItem->disable();
         //menuText.set(L"Locked");
         pMenuItem->setControlID(BUIMPSetupScreen::cMenuCommandSetCampaignMission);     // The command 
         helpText.set(L"");
         pMenuItem->setHelpText(helpText);
         pMenuItem->setText(gDatabase.getLocStringFromID(25893));
         pMenuItem->setData(i);               // The data
         pMenuItem->show();
      }

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
}


//==============================================================================
//==============================================================================
bool BUIMPSetupScreen::isLocalPlayerReady()
{
//-- FIXING PREFIX BUG ID 3666
   const BPartySessionPartyMember* pPartyMember = getLocalMember();
//--
   if (!pPartyMember)
      return false;

   // am I ready
   return (pPartyMember->mSettings.mConnectionState == cBPartySessionMemberConnectionStateReadyToStart);
}

//==============================================================================
// BModePartyRoom2::getLocalMember
//==============================================================================
BPartySessionPartyMember* BUIMPSetupScreen::getMemberByXUID(XUID xuid)
{
   BPartySession* pParty = getPartySession();
   //It is possible during startup for this to be called and there be no party session
   //BASSERT(pParty);
   if (!pParty || !pParty->getSession())
   {
      return NULL;
   }

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      if (pPartyMember->mXuid != xuid)
         continue;

      return pPartyMember;
   }

   return NULL;
}

//==============================================================================
// BModePartyRoom2::getLocalMember
// FIXME-COOP - we could have two local players
//==============================================================================
BPartySessionPartyMember* BUIMPSetupScreen::getLocalMember()
{
   BPartySession* pParty = getPartySession();
   //It is possible during startup for this to be called and there be no party session
   //BASSERT(pParty);
   if (!pParty || !pParty->getSession())
   {
      return NULL;
   }

   uint partyCount = pParty->getPartyMaxSize();
   for (uint i=0; i<partyCount; i++)
   {
      BPartySessionPartyMember* pPartyMember = pParty->getPartyMember(i);
      if (!pPartyMember)
         continue;

      // is this me?
      if (!pParty->getSession()->isLocalClientID(pPartyMember->mClientID))
         continue;

      return pPartyMember;
   }

   return NULL;
}

//==============================================================================
// BModePartyRoom2::updateMenuHelp
//==============================================================================
void BUIMPSetupScreen::updateHelp()
{
   updateMenuHelp();
   updatePlayerListHelp();
}


//==============================================================================
// BModePartyRoom2::updateMenuHelp
//==============================================================================
void BUIMPSetupScreen::updateMenuHelp()
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
void BUIMPSetupScreen::updatePlayerListHelp()
{
   if (mPartyRoomView != cPRViewPlayerList)
      return;


   BUString emptyString;
   emptyString.set(L"");

   BOOL playerIsHost=isLocalPlayerHost();

   if (getPlayerCurrentSlotFocus() >= 0)
   {
      BPartySessionPartyMember* pFocusMember = mpMode->getPlayerBySlot(getPlayerCurrentSlotFocus());

      //Custom handling of the text for E3 - eric
      if (!pFocusMember)
      {
         // empty slot.
         if (mpMode->getUseLanMode())
         {
            // LAN
            switch (mpMode->getPartyRoomMode())
            {
               case BModePartyRoom2::cPartyRoomModeCampaign:
                  setHelpText(gDatabase.getLocStringFromID(25931));  // wait for others to join. (host and client)
                  break;
               case BModePartyRoom2::cPartyRoomModeCustom:
                  if (playerIsHost)
                     setHelpText(gDatabase.getLocStringFromID(25557));  // host - wait or press A to add ai
                  else
                     setHelpText(gDatabase.getLocStringFromID(25931));  // client - wait for others to join.
                  break;
            }
         }
         else
         {
            // LIVE
            switch (mpMode->getPartyRoomMode())
            {
               case BModePartyRoom2::cPartyRoomModeMM:
                  // fallthrough intentional.
               case BModePartyRoom2::cPartyRoomModeCampaign:
                  if (playerIsHost)
                     setHelpText(gDatabase.getLocStringFromID(24112));     // host - Press $$Y$$ to invite friends to play
                  else
                     setHelpText(emptyString);  // client - nothing
                  break;
               case BModePartyRoom2::cPartyRoomModeCustom:
                  if (playerIsHost)
                     setHelpText(gDatabase.getLocStringFromID(25932));  // host - press Y to invite friends or A to add ai player
                  else
                     setHelpText(gDatabase.getLocStringFromID(24112));  // client - Press $$Y$$ to invite friends to play
                  break;
            }
         }
      }
      else
      {
         if (isLocalPlayer(pFocusMember))
         {
            // is player ready?
            if (isLocalPlayerReady())
            {
               BPartySession* pPartySession = getPartySession();

               //Don't display this text if we are in matchmaking mode - it is confusing  
               //if (mGameOptionSettings.mPartyRoomMode != cPartyRoomModeMM)
               if (!mpMode->isMatchmakingMode())
               {
                  // "Ready and waiting for game to start"
                  if (pPartySession && pPartySession->isPartyFull())
                     setHelpText(gDatabase.getLocStringFromID(24111));  // Ready and waiting for game to start...
                  else
                     setHelpText(gDatabase.getLocStringFromID(25569));  // Waiting for others to join to start the game.
               }
               else
               {
                  if (pPartySession && pPartySession->isPartyFull())
                     setHelpText(gDatabase.getLocStringFromID(25341));  // Waiting to start matchmaking.
                  else
                     setHelpText(gDatabase.getLocStringFromID(25569));  // Waiting for others to join to start the game.
               }
            }
            else
            {
               // It's me and I am NOT ready
               if (mpMode->getUseLanMode())
               {
                  // LAN
                  switch (mpMode->getPartyRoomMode())
                  {
                  case BModePartyRoom2::cPartyRoomModeCampaign:
                     setHelpText(emptyString);                             // can't change anything about me in campaign on lan (host or not)
                     break;
                  case BModePartyRoom2::cPartyRoomModeCustom:
                     setHelpText(gDatabase.getLocStringFromID(25572));     // (client and host) - Press A for player options.
                     break;
                  }
               }
               else
               {
                  // LIVE
                  // All Modes:
                  setHelpText(gDatabase.getLocStringFromID(25572));     // Press A for player options.
               }
            }
         }
         else
         {
            // "other player"
            if (mpMode->getUseLanMode())
            {
               // LAN
               switch (mpMode->getPartyRoomMode())
               {
                  case BModePartyRoom2::cPartyRoomModeCampaign:
                     if (playerIsHost)
                        setHelpText(gDatabase.getLocStringFromID(25572));     // (host) - Press A for player options.
                     else
                        setHelpText(emptyString);                             // can't change anything about me in campaign on lan (host or not)
                     break;
                  case BModePartyRoom2::cPartyRoomModeCustom:
                     if (playerIsHost)
                        setHelpText(gDatabase.getLocStringFromID(25572));     // (host) - Press A for player options.
                     else
                        setHelpText(emptyString);                             // can't change anything about me in campaign on lan (host or not)
                     break;
               }
            }
            else
            {
               // LIVE
               switch (mpMode->getPartyRoomMode())
               {
                  case BModePartyRoom2::cPartyRoomModeCustom:
                     if (playerIsHost)
                        setHelpText(gDatabase.getLocStringFromID(25572));     // (host) - Press A for player options.
                     else
                     {
                        // if player is human, player options, else nothing
                        if (pFocusMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI)
                           setHelpText(emptyString);
                        else
                           setHelpText(gDatabase.getLocStringFromID(25572));     // (host) - Press A for player options.
                     }

                     break;
                  case BModePartyRoom2::cPartyRoomModeCampaign:
                     setHelpText(gDatabase.getLocStringFromID(25572));     // (host) - Press A for player options.
                     break;
                  case BModePartyRoom2::cPartyRoomModeMM:
                     setHelpText(gDatabase.getLocStringFromID(25572));     // (host) - Press A for player options.
                     break;
               }
            }
         }
      }
   }

}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::displayButtons()
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

   BUser* user = gUserManager.getPrimaryUser();

   // fixme - set up the string ids here, fixme - we need to cache the IDs too for perf
   switch (mPartyRoomView)
   {
      case BUIMPSetupScreen::cPRViewLanGameList:
         // qualify this item
         if (getLanGamesList()->getCurrentMenuItem()!=NULL)
         {
            btn1=BUIButtonBarControl::cFlashButtonA;
            strID1=24169;   // Join
         }

         btn2=BUIButtonBarControl::cFlashButtonB;
         strID2=23851;   // Leave Party

         btn3=BUIButtonBarControl::cFlashButtonX;
         strID3=24168;        // Host
         break;

      case BUIMPSetupScreen::cPRViewMenu:
         btn1=BUIButtonBarControl::cFlashButtonA;
         strID1=23789;   // Edit
         //strID1=23439;        // Ready Up

         btn2=BUIButtonBarControl::cFlashButtonB;
         strID2=23851;   // Leave Party
         
         if (!mpMode->getUseLanMode() && 
             haveRoomToInvitePlayers() &&
             user &&
             user->checkPrivilege(XPRIVILEGE_COMMUNICATIONS))
         {
            btn3=BUIButtonBarControl::cFlashButtonY;
            strID3=23512;        // Invite Friends
         }

/*
         btn3=BUIButtonBarControl::cFlashButtonX;
         strID3=23439;        // Ready Up
*/
         btn4=BUIButtonBarControl::cFlashButtonStart;
         strID4=23439;        // Ready Up

         if (mpMode->getPartyRoomMode()==BModePartyRoom2::cPartyRoomModeCustom)
         {
            btn5=BUIButtonBarControl::cFlashButtonLRButton;
            strID5=23852;        // Switch Teams
         }

/*
         if (mpMode->getPartyRoomMode()!=BModePartyRoom2::cPartyRoomModeCampaign)
         {
            btn7=BUIButtonBarControl::cFlashButtonLRTrigger;
            strID7=23853;        // Switch Leader
         }
*/

         break;
      case BUIMPSetupScreen::cPRViewPlayerMenu:
         btn1=BUIButtonBarControl::cFlashButtonA;
         strID1=23437;   // Accept
         btn2=BUIButtonBarControl::cFlashButtonB;
         strID2=23438;   // Cancel
         break;
      case BUIMPSetupScreen::cPRViewSubMenu:
         btn1=BUIButtonBarControl::cFlashButtonA;
         strID1=23437;   // Accept
         btn2=BUIButtonBarControl::cFlashButtonB;
         strID2=23438;   // Cancel
         break;
      case BUIMPSetupScreen::cPRViewPlayerList:
         if (!isLocalPlayerReady())
         {
            // Decide if I should 1) hide the button, 2) show the player options label, 3) show the gamer tag option
//-- FIXING PREFIX BUG ID 3667
            const BPartySessionPartyMember* pFocusMember = mpMode->getPlayerBySlot(getPlayerCurrentSlotFocus());
//--
//-- FIXING PREFIX BUG ID 3668
            const BPartySessionPartyMember* pLocalMember = getLocalMember();
//--

            if (pFocusMember)
            {
               // edit or gamer tag
               btn1=BUIButtonBarControl::cFlashButtonA;
               if (pLocalMember && (pFocusMember->mXuid == pLocalMember->mXuid))
               {
                  // it's me, I can edit me
                  if (mpMode->getPartyRoomMode()==BModePartyRoom2::cPartyRoomModeCampaign)
                  {
                     if (!mpMode->getUseLanMode())
                     {
                        strID1=25573;   // OPTIONS
                     }
                     else
                     {
                        // campaign/lan, it's me, can't do anything
                        btn1=BUIButtonBarControl::cFlashButtonOff;
                        strID1=0;
                     }
                  }
                  else if (mpMode->isOkToShowLocalPlayerOptionsInUI())
                  {
                     strID1=23446;   // Settings
                  }
                  else
                  {
                     //Show nothing
                     btn1=BUIButtonBarControl::cFlashButtonOff;
                     strID1=0;
                  }
               }
               else 
               {
                  // IT'S NOT ME

                  // if this is an AI, then settings, else, gamercard
                  if (pFocusMember->mSettings.getPartyMemberType() == cBPartySessionPartyMemberAI)
                  {
                     // it's and AI, I can edit them, I can edit me
                     if (isLocalPlayerHost())
                     {
                        strID1=23446;   // Settings
                     }
                     else
                     {
                        // if I'm not host, I can't do anything here.
                        btn1=BUIButtonBarControl::cFlashButtonOff;
                        strID1=0;
                     }
                  }
                  else
                  {
                     if (!mpMode->getUseLanMode())
                     {
                        // it's somebody else, I can check out their gamer tag
                        strID1=25573;   // "Options"
                     }
                     else
                     {
                        if (isLocalPlayerHost())
                        {
                           // as a host, I can always drop them.
                           strID1=25573;   // "Options"
                        }
                        else
                        {
                           // turn off - campaign/lan
                           btn1=BUIButtonBarControl::cFlashButtonOff;
                           strID1=0;
                        }
                     }
                  }

                  if (isLocalPlayerHost())
                  {
                     btn3=BUIButtonBarControl::cFlashButtonBack;
                     strID3=23442;  // Kick
                  }
               }
            }
            else
            {
               if (mpMode->getPartyRoomMode()==BModePartyRoom2::cPartyRoomModeCustom && isLocalPlayerHost())
               {
                  // open slot
                  btn1=BUIButtonBarControl::cFlashButtonA;
                  strID1=23446;   // Edit (add ai)
               }
            }

            btn2=BUIButtonBarControl::cFlashButtonB;
            strID2=23851;   // Leave party

            if (!mpMode->getUseLanMode() && haveRoomToInvitePlayers())
            {
               btn3=BUIButtonBarControl::cFlashButtonY;
               strID3=23512;        // Invite Friends
            }

            btn4=BUIButtonBarControl::cFlashButtonStart;
            strID4=23439;        // Ready Up

            if (mpMode->getPartyRoomMode()==BModePartyRoom2::cPartyRoomModeCustom)
            {
               btn5=BUIButtonBarControl::cFlashButtonLRButton;
               strID5=23852;        // Switch Teams
            }

/*
            btn7=BUIButtonBarControl::cFlashButtonLRTrigger;
            strID7=23853;        // Switch Leader
*/
         }
         else
         {

            if (mpMode->mpSessionEvent_queryIsInMatchmakingMode() && gLiveSystem->getMPSession()->isMatchmakingRunning())
            {
               // CANCEL
               btn1=BUIButtonBarControl::cFlashButtonB;
               strID1=23438;   // CANCEL
            }
            else
            {
               // Unready
               btn1=BUIButtonBarControl::cFlashButtonB;
               strID1=23498;   // Unready
            }
         }
         break;
      case BUIMPSetupScreen::cPRViewPlayerEdit:
         btn1=BUIButtonBarControl::cFlashButtonA;
         strID1=23437;   // Accept
         btn2=BUIButtonBarControl::cFlashButtonB;
         strID2=23440;   // Back
         break;
   }

      getButtonBar().setButtonStates(btn1, btn2, btn3, btn4, btn5);
      getButtonBar().setButtonTexts(  gDatabase.getLocStringFromID(strID1),
         gDatabase.getLocStringFromID(strID2),
         gDatabase.getLocStringFromID(strID3),
         gDatabase.getLocStringFromID(strID4),
         gDatabase.getLocStringFromID(strID5) );
}


//==============================================================================
//==============================================================================
void BUIMPSetupScreen::getSettingsChangeFromSecondaryMenuItem(BUIMenuItemControl * pMenuItemControl)
{

   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   // make a copy of the current game options for editing
   BPartySessionHostSettings gameOptionSettings = pParty->getCurrentHostSettings();

   /*
   [mCommandID]               [command]        [mID]
   cMenuCommandSetLobby       "selectLobby"    mID - new lobby type
   cPartyRoomModeCustom,
   cPartyRoomModeMM,
   cPartyRoomModeCampaign,

   cMenuCommandSetMap          "mapPicker"      mID - index into scenario list
   cMenuCommandSetPlayerCount  "playerCount"    mID - number of players for the game
   cMenuCommandSetTeamType     "selectTeamType" mID - team Type
   cCustomTeamTypeTeams=0,
   cCustomTeamTypeRandom,
   */
   bool update = false;
   switch (pMenuItemControl->getControlID())
   {
   case BUIMPSetupScreen::cMenuCommandSetLobby:
      {
         long newPartyRoomMode = pMenuItemControl->getData();
         mpMode->onAcceptLobby(newPartyRoomMode);
      }
      break;

   case BUIMPSetupScreen::cMenuCommandLivePartyType:
      {
         if (gameOptionSettings.mLiveMode != (uint8)pMenuItemControl->getData())
         {
            gameOptionSettings.mLiveMode = (uint8)pMenuItemControl->getData();
            mpMode->setGameOptions(gameOptionSettings);
            update=true;
         }
      }
      break;

   case BUIMPSetupScreen::cMenuCommandSetCampaignMission:
      if (gameOptionSettings.mMapIndex != pMenuItemControl->getData())
      {
         gameOptionSettings.mMapIndex = (uint8)pMenuItemControl->getData();
         mpMode->setGameOptions(gameOptionSettings);
         update = true;
      }

      break;
   case BUIMPSetupScreen::cMenuCommandSetMap:
      // set the map

      if (gameOptionSettings.mMapIndex != pMenuItemControl->getData())
      {
         mpMode->setUseDefaultMap(false);
         gameOptionSettings.mMapIndex = (uint8)pMenuItemControl->getData();
         mpMode->setGameOptions(gameOptionSettings);
         update = true;
      }
      break;
   case BUIMPSetupScreen::cMenuCommandSetPlayerCount:
      if (gameOptionSettings.mNumPlayers != pMenuItemControl->getData())
      {
         bool matchPlayersExactly = mpMode->getUseDefaultMap();

         const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(gameOptionSettings.mMapIndex);
         // is our current map a valid type and valid for the num players?
         if ( !pMap ||
            !isValidMapType(pMap->getType()) ||
            !isValidMap(gameOptionSettings.mMapIndex, (uint8)pMenuItemControl->getData(), matchPlayersExactly) )
         {
            // for whatever reason we need a new map, let's go back to using the default.
            mpMode->setUseDefaultMap(true);

            // find a new map
            uint8 newMapIndex = 0;
            if (!getFirstValidMap((uint8)pMenuItemControl->getData(), newMapIndex, true))
            {
               // we couldn't find a map, throw up a message and quit
               return;
            }
            gameOptionSettings.mMapIndex = newMapIndex;
         }

         gameOptionSettings.mNumPlayers = (uint8)pMenuItemControl->getData();
         mpMode->setGameOptions(gameOptionSettings);
         mpMode->onAcceptGameOptions();
      }
      break;
   case BUIMPSetupScreen::cMenuCommandSetGameMode:
      {
         if (gameOptionSettings.mGameMode != (uint8)pMenuItemControl->getData())
         {
//-- FIXING PREFIX BUG ID 3669
            const BGameMode* gameMode = gDatabase.getGameModeByID(pMenuItemControl->getData());
//--
            if (gameMode)
            {
               gameOptionSettings.mGameMode = (uint8)pMenuItemControl->getData();
               mpMode->setGameOptions(gameOptionSettings);
               update=true;
            }
         }
      }
      break;
   case BUIMPSetupScreen::cMenuCommandSetAIDifficulty:
      {
         if (gameOptionSettings.mDifficulty != (uint8)pMenuItemControl->getData())
         {
            gameOptionSettings.mDifficulty = (uint8)pMenuItemControl->getData();
            mpMode->setGameOptions(gameOptionSettings);
            update=true;
         }
      }
      break;
   case BUIMPSetupScreen::cMenuCommandSetTeamType:
      if (gameOptionSettings.mRandomTeam != (pMenuItemControl->getData() == BUIMPSetupScreen::cCustomTeamTypeRandom))
      {
         gameOptionSettings.mRandomTeam = !gameOptionSettings.mRandomTeam;
         mpMode->setGameOptions(gameOptionSettings);
         update = true;
      }
      break;

   case BUIMPSetupScreen::cMenuCommandSetMatchmakingHopper:
      if (gameOptionSettings.mHopperIndex != pMenuItemControl->getData())
      {
//-- FIXING PREFIX BUG ID 3670
         const BMatchMakingHopper* pHopper = gLiveSystem->getHopperList()->findHopperByHopperIndex(pMenuItemControl->getData());
//--
         gameOptionSettings.mHopperIndex = (uint8)pMenuItemControl->getData();
         gameOptionSettings.mNumPlayers = (pHopper->mTeamCode==BMatchMakingHopper::cBMatchMakingHopperTeamCodeNoTeams)?1:pHopper->mPlayersPerTeam;
         mpMode->setGameOptions(gameOptionSettings);
         mpMode->onAcceptGameOptions();
         // note: leave update == false because onAcceptGameOptions handles all that.
      }
      break;
   }

   if (update)
   {
      // only send out an update if something changed.
      pParty->changeHostSettings(gameOptionSettings);
   }
}

//==============================================================================
//==============================================================================
bool BUIMPSetupScreen::haveRoomToInvitePlayers()
{
   BPartySession* pParty = getPartySession();
   BASSERT(pParty);

   if ( pParty->getPartyCount() < pParty->getPartyMaxSize() )
      return true;

   return false;
}

//==============================================================================
//==============================================================================
bool BUIMPSetupScreen::getFirstValidMap(uint8 numPlayers, uint8& mapIndex, bool matchPlayersExactly)
{
   for (int i=0; i<gDatabase.getScenarioList().getMapCount(); i++)
   {
      const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(i);
      BASSERT(pMap);
      if (!isValidMapType(pMap->getType()))
         continue;

      if (!isValidMap((uint8)i, numPlayers, matchPlayersExactly))
         continue;

      mapIndex=(uint8)i;
      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BUIMPSetupScreen::setView(long view) 
{ 
   mPartyRoomView = view; 
   mLastViewChangeTime = timeGetTime(); 
}
