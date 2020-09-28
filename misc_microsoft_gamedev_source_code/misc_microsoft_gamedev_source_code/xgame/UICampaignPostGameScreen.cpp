//============================================================================
// UICampaignPostGameScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UICampaignPostGameScreen.h"

#include "UITabControl.h"
#include "UIPanelControl.h"
#include "UILabelControl.h"
#include "UIcpgSummaryPanelControl.h"
#include "UICpgGradePanelControl.h"
#include "campaignmanager.h"
#include "player.h"
#include "world.h"
#include "database.h"
#include "configsgame.h"
#include "uimanager.h"
#include "user.h"
#include "game.h"
#include "gamesettings.h"
#include "LiveSystem.h"

//============================================================================
//============================================================================
BUICampaignPostGameScreen::BUICampaignPostGameScreen( void ) :
   mpPanelSummary(NULL),
   mpPanelGrade(NULL),
   mAButtonIcon(BUIButtonBarControl::cFlashButtonA),
   mAButtonResult(eResult_Continue),
   mStartContext(0),
   mSelectedXUID(INVALID_XUID)
{
}

//============================================================================
//============================================================================
BUICampaignPostGameScreen::~BUICampaignPostGameScreen( void )
{
   for( int i = 0; i < (int)mControls.size(); ++i )
   {
      delete mControls[i];
   }
   mpPanelSummary=NULL;
   mpPanelGrade=NULL;
}

//============================================================================
//============================================================================
bool BUICampaignPostGameScreen::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   BGameSettings* pGameSettings = gDatabase.getGameSettings();
   BASSERT( pGameSettings );

   mStartContext = 0;
   pGameSettings->getLong( BGameSettings::cGameStartContext, mStartContext );

   switch( mStartContext )
   {
      case BGameSettings::cGameStartContextBasicTutorialFromMainMenu:
      case BGameSettings::cGameStartContextBasicTutorialFromSPCMenu:
         mAButtonText = gDatabase.getLocStringFromID( 25428 );
         mAButtonResult = eResult_GoToAdvancedTutorial;
         break;

      case BGameSettings::cGameStartContextBasicTutorialFromNewSPC:
         mAButtonText = gDatabase.getLocStringFromID( 25065 );
         mAButtonResult = eResult_Continue;
         break;

      case BGameSettings::cGameStartContextAdvancedTutorialFromMainMenu:
      case BGameSettings::cGameStartContextAdvancedTutorialFromSPCMenu:
         mAButtonIcon = BUIButtonBarControl::cFlashButtonOff;
         mAButtonText.empty();
         mAButtonResult = eResult_None;
         break;

      case BGameSettings::cGameStartContextPartyCampaign:
         mAButtonIcon = BUIButtonBarControl::cFlashButtonA;
         mAButtonText = gDatabase.getLocStringFromID( 25962 );
         mAButtonResult = eResult_Continue;
         break;

      default:
         mAButtonIcon = BUIButtonBarControl::cFlashButtonA;
         mAButtonText = gDatabase.getLocStringFromID( 25065 );
         mAButtonResult = eResult_Continue;
         break;
   }

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignPostGameScreen::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   // initialize all the components for our screen
   mTabControl.init(this, "");      // no visual, so leave the path empty.

   mTabInitDone=false;

   // create the tab panels here, but don't add them to the tab until we are "entered" for the first time.
   mpPanelSummary = new BUICpgSummaryPanelControl();
   mpPanelSummary->init(this, "mTabSummary", -1, &dataNode);

   mpPanelGrade = new BUICpgGradePanelControl();
   mpPanelGrade->init(this, "mTabGrades", -1, &dataNode);

   BXMLNode controlsNode;
   if (dataNode.getChild( "UIScrollableCallout", &controlsNode ) )
      mCallout.init(this, "mCallout", -1, &controlsNode);
   else
   {
      BASSERTM(false, "UISkirmishPostgameScreen:: Unable to initialize mCallout control input handler.");
      mCallout.init(this, "mCallout");    // use default
   }
   mCallout.hide(true);

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   // Text Fields
   mTitle.init(this, "mTitle");
   mGameTime.init(this, "mGameTime");
   mDifficulty.init(this, "mDifficulty");

   return true;
}

//============================================================================
//============================================================================
void BUICampaignPostGameScreen::populateMissionName()
{
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if( pSettings )
   {
      long gameType = -1;
      pSettings->getLong( BGameSettings::cGameType, gameType );

      BSimString mapName;
      pSettings->getString( BGameSettings::cMapName, mapName );

      if ( (gameType == BGameSettings::cGameTypeSkirmish ) ||
         (gameType == BGameSettings::cGameTypeScenario ) )
      {
         const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapName.getPtr());
         if (pMap!=NULL)
         {
            mTitle.setText( pMap->getMapName() );
         }
      }
      else
      {
         BCampaign* pCampaign = gCampaignManager.getCampaign(0);
         if (pCampaign)
         {
            BCampaignNode* pNode = pCampaign->getNode(mapName.getPtr());
            if (pNode)
            {
               mTitle.setText( gDatabase.getLocStringFromID(pNode->getDisplayNameStringID()) );
            }
         }
      }
   }



}

//============================================================================
//============================================================================
void BUICampaignPostGameScreen::populateTimeAndDifficulty()
{
   BUString time;
   DWORD gameTime=gWorld->getGametime();
   formatTime(gameTime, time);

   mGameTime.setText(time);

   BPlayer* pPlayer = gWorld->getPlayer(1);        // any way to make this safer?
   if (pPlayer && pPlayer->isHuman())
   {
      BDifficultyType playerDifficulty = pPlayer->getDifficultyType();
      mDifficulty.update(playerDifficulty);
   }
}

//============================================================================
//============================================================================
void BUICampaignPostGameScreen::populate()
{
   if( gConfig.isDefined( cConfigAutoScenarioLoad ) )
   {
      displayButtons();
      return;
   }

   populateMissionName();
   populateTimeAndDifficulty();

   if (mpPanelSummary)
      mpPanelSummary->populate();

   if (mpPanelGrade && mpPanelGrade->isShown())
      mpPanelGrade->populate();

   mTabControl.setActiveTab(0);

   if( mTabControl.getActivePane() )
      mTabControl.getActivePane()->show();
   
   displayButtons();
}

//============================================================================
//============================================================================
// bool BUICampaignPostGameScreen::addTab(const char * labelName, BUIPanelControl *panel, const WCHAR* labelText)
bool BUICampaignPostGameScreen::addTab(const char * labelName, BUIPanelControl *panel, const WCHAR* labelText, bool visible)
{
   BUILabelControl* label = NULL;

   label = new BUILabelControl();
   label->init(this, labelName);
   BUString labelTextString;
   labelTextString.set(labelText);
   label->setText(labelTextString);

   if (visible)
   {
      mTabControl.addTab(label, panel);
   }
   else
   {
      label->hide();
      panel->hide();
   }

   mControls.add(panel);
   mControls.add(label);

   return true;
}


//============================================================================
//============================================================================
bool BUICampaignPostGameScreen::displayButtons()
{

   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   BUString s0;
   BUString s1;
   BUString s2;
   BUString s3;
   BUString s4;

   if (mCallout.isShown())
   {
      // if the callout is up, then A is close
      b0 = BUIButtonBarControl::cFlashButtonB;
      s0.set(gDatabase.getLocStringFromID(25044)); // CLOSE
      mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
      mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);
      return true;
   }

   uint aButtonIcon = BUIButtonBarControl::cFlashButtonOff;
   BUString aButtonText;


   // In a multi-player campaign game, we can only continue;
   if (mStartContext == BGameSettings::cGameStartContextPartyCampaign)
   {
      if (mpPanelGrade && mpPanelGrade->isShown())
      {
         // A, X button
         mButtonBar.setButtonStates( mAButtonIcon, BUIButtonBarControl::cFlashButtonX);
         mButtonBar.setButtonTexts( mAButtonText, gDatabase.getLocStringFromID( 25896 ) );
      }
      else
      {
         mButtonBar.setButtonStates( mAButtonIcon);
         mButtonBar.setButtonTexts( mAButtonText);
      }

      if( mSelectedXUID != INVALID_XUID )
      {
         mButtonBar.setButtonState( 4, BUIButtonBarControl::cFlashButtonY );
         mButtonBar.setButtonText( 4, gDatabase.getLocStringFromID(23441) );
      }
      return true;
   }


   if ( gUIManager->getCurrentUser()->getPlayer()->getPlayerState() == BPlayer::cPlayerStateWon )
   {
      aButtonIcon = mAButtonIcon;
      aButtonText = mAButtonText;
   }

   if (mpPanelGrade && mpPanelGrade->isShown())
   {
      mButtonBar.setButtonStates( aButtonIcon, BUIButtonBarControl::cFlashButtonB, BUIButtonBarControl::cFlashButtonX, BUIButtonBarControl::cFlashButtonY );
      mButtonBar.setButtonTexts( aButtonText, gDatabase.getLocStringFromID( 25042 ), gDatabase.getLocStringFromID( 25896 ), gDatabase.getLocStringFromID( 25066 ) );
   }
   else
   {
      mButtonBar.setButtonStates( aButtonIcon, BUIButtonBarControl::cFlashButtonB, BUIButtonBarControl::cFlashButtonY );
      mButtonBar.setButtonTexts( aButtonText, gDatabase.getLocStringFromID( 25042 ), gDatabase.getLocStringFromID( 25066 ) );
   }

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignPostGameScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (mStartContext==BGameSettings::cGameStartContextPartyCampaign)
   {
      // in campaign, ignore all but continue and score details.
      if (command=="continue")
      {
         gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(25964), BUIGlobals::cDialogButtonsOKCancel, mAButtonResult );
         //mpHandler->handleUIScreenResult( this, mAButtonResult );
         return true;
      }
      else if (command=="scoreDetails")
      {
         if (mpPanelGrade && mpPanelGrade->isShown())
         {
            gUI.playConfirmSound();
            mCallout.setTitle(gDatabase.getLocStringFromID(25894));
            mCallout.setText(gDatabase.getLocStringFromID(25895));
            mCallout.show();
            displayButtons();
         }
         return true;
      }
      else if (command=="replay")
      {
         if( mSelectedXUID != INVALID_XUID )
         {
            XShowGamerCardUI( gUserManager.getPrimaryUser()->getPort(), mSelectedXUID );
            return true;
         }
      }

      return true;
   }


   if (command=="continue" && gUIManager->getCurrentUser()->getPlayer()->getPlayerState() == BPlayer::cPlayerStateWon && mAButtonResult != eResult_None )
   {
      if ( (mStartContext==BGameSettings::cGameStartContextBasicTutorialFromMainMenu) ||
           (mStartContext==BGameSettings::cGameStartContextBasicTutorialFromSPCMenu) )
      {
         gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(25964), BUIGlobals::cDialogButtonsOKCancel, mAButtonResult );
      }
      else
      {
         mpHandler->handleUIScreenResult( this, mAButtonResult );
      }
      return true;
   }
   else if (command=="quit")
   {
      gUI.playCancelSound();
      // gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(24048), BUIGlobals::cDialogButtonsOKCancel, eResult_Quit );
      mpHandler->handleUIScreenResult( this, eResult_Quit );
      return true;
   }
   else if (command=="replay")
   {
      gUI.playConfirmSound();
      // gGame.getUIGlobals()->showYornBoxSmall( this, gDatabase.getLocStringFromID(24049), BUIGlobals::cDialogButtonsOKCancel, eResult_Replay );
      mpHandler->handleUIScreenResult( this, eResult_Replay );
      return true;
   }
   else if (command=="scoreDetails")
   {
      if (mpPanelGrade && mpPanelGrade->isShown())
      {
         gUI.playConfirmSound();
         mCallout.setTitle(gDatabase.getLocStringFromID(25894));
         mCallout.setText(gDatabase.getLocStringFromID(25895));
         mCallout.show();
         displayButtons();
      }
      return true;
   }

   return false;
}


//============================================================================
//============================================================================
bool BUICampaignPostGameScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   if (!mTabInitDone)
      return true;      // eat the input until we are actually initialized.

   bool handled = false;

   if (!handled)
      handled = mTabControl.handleInput(port, event, controlType, detail);

   if (!handled && mCallout.isShown())
      handled = mCallout.handleInput(port, event, controlType, detail);

   if (!handled)
   {
      switch (mTabControl.getActiveTab())
      {
         // since the tabs can change order now, we are going to use their visibility. 
         // we still want to qualify the first 2 tabs, though
         case cCampaignPostgameTabSummary:
         case cCampaignPostgameTabGrades:
            if (mpPanelSummary && mpPanelSummary->isShown())
               handled = mpPanelSummary->handleInput(port, event, controlType, detail);
            else if (mpPanelGrade && mpPanelGrade->isShown())
               handled = mpPanelGrade->handleInput(port, event, controlType, detail);
         break;
      }
   }

   if (!handled)
      handled = BUIScreen::handleInput(port, event, controlType, detail);
   
   return true;
}


// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICampaignPostGameScreen::handleUIControlEvent( BUIControlEvent& event )
{

   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.

   bool handled = false;

   switch ( control->getControlTypeID() )
   {
      case UIScrollableCalloutControlID:
         if (event.getID() == BUIScrollableCalloutControl::eCancel)
         {
            if (!mCallout.isShown())
               break;
            gUI.playCancelSound();
            mCallout.hide();
            displayButtons();
         }
         break;

      case UITabControlID:
         if (event.getID() == BUITabControl::eTabChanged)
         {
            mCallout.hide();
            displayButtons();
         }
         break;

      case UIPanelControlID:
         // if (gLiveSystem->isMultiplayerLiveGameActive() && 
         if ( (gDatabase.getGameSettingsNetworkType() == BGameSettings::cNetworkTypeLive) &&
             event.getChildEvent() && 
             event.getChildEvent()->getControl()->getControlTypeID() == UIGamerTagControlID )
         {
            if (event.getChildEvent()->getID() == BUIGamerTagLongControl::eFocus)
            {
               mButtonBar.setButtonState( 4, BUIButtonBarControl::cFlashButtonY );
               mButtonBar.setButtonText( 4, gDatabase.getLocStringFromID(23441) );
               BUIGamerTagLongControl* pGamerTag = reinterpret_cast<BUIGamerTagLongControl*>(event.getChildEvent()->getControl());
               mSelectedXUID = pGamerTag->getXuid();
            }
            else if (event.getChildEvent()->getID() == BUIGamerTagLongControl::eUnfocus)
            {
               mButtonBar.setButtonState( 4, BUIButtonBarControl::cFlashButtonOff );
               mButtonBar.setButtonText( 4, L"" );
               mSelectedXUID = INVALID_XUID;
            }
         }
         break;
   }

   return handled;
}

//============================================================================
//============================================================================
void BUICampaignPostGameScreen::enter( void )
{
   bool isWinner = (gUIManager->getScenarioResult() == BPlayer::cPlayerStateWon);

   if (!mTabInitDone)
   {
      BString tabLabel;
      int tabNumber=0;

      long gameType = -1;
      BGameSettings* pSettings = gDatabase.getGameSettings();
      BASSERT( pSettings );
      if( pSettings )
         pSettings->getLong( BGameSettings::cGameType, gameType );

      if (isWinner && gameType != BGameSettings::cGameTypeScenario)
      {
         tabLabel.format("mTabLabel%d", tabNumber);
         addTab(tabLabel.getPtr(), mpPanelGrade, gDatabase.getLocStringFromID(25068), true ); //L"Awards");

         tabNumber++;
         tabLabel.format("mTabLabel%d", tabNumber);
         addTab(tabLabel.getPtr(),       mpPanelSummary,   gDatabase.getLocStringFromID(25067), true); //L"Summary");
      }
      else
      {
         // if you lose, make the summery the first tab and turn off the awards tab
         tabLabel.format("mTabLabel%d", tabNumber);
         addTab(tabLabel.getPtr(),       mpPanelSummary,   gDatabase.getLocStringFromID(25067), true); //L"Summary");

         tabNumber++;
         tabLabel.format("mTabLabel%d", tabNumber);
         addTab(tabLabel.getPtr(), mpPanelGrade, gDatabase.getLocStringFromID(25068), false); //L"Awards");
      }


      mTabInitDone=true;
   }

   populate();

   if( gUIManager->getScenarioResult() != BPlayer::cPlayerStateWon )
   {
      if (!gConfig.isDefined(cConfigNoMusic))
         gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateSPCLost);
   }
}

//============================================================================
//============================================================================
void BUICampaignPostGameScreen::yornResult(uint result, DWORD userContext, int port)
{
   if( result == BUIGlobals::cDialogResultOK )
   {
      mpHandler->handleUIScreenResult( this, userContext );
   }
}