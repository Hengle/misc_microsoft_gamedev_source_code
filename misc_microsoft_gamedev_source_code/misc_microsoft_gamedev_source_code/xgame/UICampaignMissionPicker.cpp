//============================================================================
// UICampaignMissionPicker.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UICampaignMissionPicker.h"

#include "UIMenuItemControl.h"

#include "campaignmanager.h"
#include "campaignprogress.h"
#include "UIListControl.h"
#include "database.h"
#include "skullmanager.h"
#include "user.h"
#include "LiveSystem.h" //for presence updates - eric

//============================================================================
//============================================================================
BUICampaignMissionPicker::BUICampaignMissionPicker( void ) : 
mCurrentMission(0),
mInitialized(false)
{
}

//============================================================================
//============================================================================
BUICampaignMissionPicker::~BUICampaignMissionPicker( void )
{
}

//============================================================================
//============================================================================
bool BUICampaignMissionPicker::init( const char* filename, const char* datafile )
{
   return BUIScreen::init(filename, datafile);
}


//============================================================================
//============================================================================
bool BUICampaignMissionPicker::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   // initialize all the components for our screen
   mMissionMenu.init(this, "mMissionMenu", 0, NULL);
   for( int i = 0; i < cMAX_MENU_ITEMS; ++i )
   {
      BSimString controlPath;
      controlPath.format("mMissionMenu.mMenuItem%d", i);
      mMissionMenuItems[i].init( this, controlPath.getPtr(), cMissionPickerMissionMenu );
      mMissionMenuItems[i].hide();
   }
   mMissionMenu.setWrap(true);

   populateMissionMenu();

   mHelpText.init(this, "mHelpText", 0, NULL);

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   mDifficultyText.init(this, "mDifficultyText", 0, NULL);
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   if (pCampaign)
      mDifficultyText.update( pCampaign->getCurrentDifficulty() );

   mScoreDisplay.init(this, "mScoreLabel", 0, NULL);
   mMedalDisplay.init(this, "mMedalDispay", 0, NULL);

   //setScoreText("FOO");

   mTitleLabel.init(this, "mTitleLabel");
   updateTitle();

   mDescriptionLabel.init(this, "mDescriptionLabel");
   mDescriptionLabel.setText(gDatabase.getLocStringFromID(25082));

   initImageViewer();

   populateScreen();
   displayButtons();

   mInitialized = true;

   return true;
}

//============================================================================
//============================================================================
void BUICampaignMissionPicker::updateTitle()
{
   BUser* pUser = gUserManager.getSecondaryUser();
   if (pUser && pUser->getFlagUserActive())
   {
      BUString title;
      BUser* pUser = gUserManager.getPrimaryUser();
      title.locFormat(L"%s (%S)", gDatabase.getLocStringFromID(24822).getPtr(), pUser->getName().getPtr());
      mTitleLabel.setText(title);
   }
   else
   {
      mTitleLabel.setText(gDatabase.getLocStringFromID(24822));
   }
}

//============================================================================
//============================================================================
BCampaignNode* BUICampaignMissionPicker::getCampaignNode()
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   if ( (mCurrentMission<0 ) || (mCurrentMission>=pCampaign->getNumberNodes()) )
      mCurrentMission=0;

   for (int i=mCurrentMission; i<pCampaign->getNumberNodes(); i++)
   {
      BCampaignNode* pNode = pCampaign->getNode(i);

      // [10/13/2008 xemu] skip cinematic and hidden nodes 
      if (pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible))
         continue;

      mCurrentMission=i;

      return pNode;
   }

   return NULL;
}

//============================================================================
//============================================================================
void BUICampaignMissionPicker::populateScreen()
{
   BCampaignNode* pNode = getCampaignNode();
   BASSERT(pNode);
   if (!pNode)
      return;

   //BUString text;

   // Image viewer
   mImageViewer.clearImages();

   mImageViewer.addImage(pNode->getImageIntro());
   //mImageViewer.addImage(pNode->getImageMinimap());
   mImageViewer.addImage(pNode->getImageEndgame());
   mImageViewer.start();

   // Scrolling text box
   bool missionCompleted = false;
   BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
   if (pProgress != NULL)
      missionCompleted = pProgress->isScenarioCompleted(pNode->getID(), -1, -1);
   if (missionCompleted)
      mHelpText.setText(pNode->getCompletedText());
   else
      mHelpText.setText(pNode->getIntroText());
   
   //Set presence - eric
   gLiveSystem->setPresenceContext(PROPERTY_MISSIONINDEX, pNode->getLeaderboardLevelIndex(), true);

   updateScoreDisplay();
   updateRScrollButton();
}

//============================================================================
// BUICampaignMissionPicker::updateRScrollButton
//============================================================================
void BUICampaignMissionPicker::updateRScrollButton()
{
   GFxValue value;
   value.SetStringW(gDatabase.getLocStringFromID(24803).getPtr());
   invokeActionScript( "updateRScrollButton", &value, 1);
}

//============================================================================
// BUICampaignMissionPicker::updateScoreDisplay
//============================================================================
void BUICampaignMissionPicker::updateScoreDisplay()
{
   BCampaignNode* pNode = getCampaignNode();
   BASSERT(pNode);
   if (!pNode)
      return;

   BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
   BASSERT(pProgress);
   if (pProgress == NULL)
      return;
      
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   // [8/15/2008 xemu] compose a medal icon string from current difficulty and medal level earned
   int mode = -1;
   int difficulty = pCampaign->getCurrentDifficulty();
   int medal1 = pProgress->getScenarioMedal(pNode->getID(), 0, difficulty);
   int medal2 = pProgress->getScenarioMedal(pNode->getID(), 1, difficulty);
   int medal = -1;
   // [10/15/2008 xemu] because of the wacky way we do medals, 1 is the best, 0 is the worst, 4 is the next-to-worst
   if (medal1 == 0)
      medal1 = 5;
   if (medal2 == 0)
      medal2 = 5;
   // [10/15/2008 xemu] with that out of the way, whichever medal is lower is the one we should use
   if (medal1 <= medal2)
   {
      medal = medal1;
      mode = 0;
   }
   else
   {
      medal = medal2;
      mode = 1;
   }

   BUString scoreString;
   uint score = pProgress->getScenarioScore(pNode->getID(), mode, pCampaign->getCurrentDifficulty());  //fixme 0 = solo, 1 = coop

   if (score == 0)
      setScoreText("");
   else
   {
      BUString iconName("");
      switch (pCampaign->getCurrentDifficulty())
      {
         case 0: iconName.set(L"$$EASY$$"); break;
         case 1: iconName.set(L"$$NORMAL$$"); break;
         case 2: iconName.set(L"$$HEROIC$$"); break;
         case 3: iconName.set(L"$$LEGENDARY$$"); break;
      }
      scoreString.locFormat(L"%s: %d %s",gDatabase.getLocStringFromID(24951).getPtr(), score, iconName.getPtr());
      setScoreText(scoreString);
   }


   BUString medalDisplayText;
   medalDisplayText.set("");
   if ((medal > 0) && (medal < 5))
   {
      BUString diffText;
      switch (difficulty)
      {
         case 0: diffText.set("EAS"); break;
         case 1: diffText.set("NRM"); break;
         case 2: diffText.set("HER"); break;
         case 3: diffText.set("LEG"); break;
         default:
            BASSERT(0);
      }
      BUString medalText;
      switch (medal)
      {
         case 1: medalText.set("GLD"); break;
         case 2: medalText.set("SIL"); break;
         case 3: medalText.set("BRZ"); break;
         case 4: medalText.set("TIN"); break; 
         default:
            BASSERT(0);
      }
      medalDisplayText.locFormat(L"$$%s-%s$$", diffText.getPtr(), medalText.getPtr());
   }

   setMedalText(medalDisplayText);

   BUString skullText;
   BUser * const user = gUserManager.getPrimaryUser();
   int scenarioIndex = pProgress->scenarioNodeIDToProgressIndex(pNode->getID());
   bool hasSkull = gCollectiblesManager.hasSkullBeenCollected(user, scenarioIndex);
   if (hasSkull)
      skullText.format(L"$$SKULL-%d$$",scenarioIndex+1);
   else
      skullText.set("");
   setSkullText(skullText);

   BUString blackboxText;
   bool hasBox = gCollectiblesManager.hasBlackBoxBeenCollected(user, scenarioIndex);
   if (hasBox)
      blackboxText.set("$$BLACKBOX$$");
   else
      blackboxText.set("");
   setBlackBoxText(blackboxText);

   BUString coopText;
   uint coopScore = pProgress->getScenarioScore(pNode->getID(), 1, pCampaign->getCurrentDifficulty());  
   if (coopScore > 0)
      coopText.set("$$COOP$$");
   else
      coopText.set("");
   setCoopText(coopText);

   BUString parTimeText;
   bool beatParTime = pProgress->getScenarioParMet(pNode->getID(), -1, pCampaign->getCurrentDifficulty());
   if (beatParTime)
      parTimeText.set("$$PARTIME$$");
   else
      parTimeText.set("");
   setParTimeText(parTimeText);

   //setMedalText("$$LEG-BRZ$$");
}

//============================================================================
//============================================================================
void BUICampaignMissionPicker::initImageViewer()
{
   mImageViewer.init(this, "mImageViewer", 0, NULL);

   mImageViewer.setAutoTransition(true);
   mImageViewer.setViewDuration(5000.0f);
   mImageViewer.setImageSize(700, 350);

   mImageViewer.start();
   
}

//============================================================================
//============================================================================
bool BUICampaignMissionPicker::populateMissionMenu()
{
   mMissionMenu.clearControls();

   // Iterate over the campaign nodes
   BCampaignProgress *pCampaignProgress = BCampaignProgress::getCampaignProgress();
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   int menuIndex = 0;
   //int selectedIndex = 0;

   for( int nodeIndex = 0; nodeIndex < pCampaign->getNumberNodes() && menuIndex < cMAX_MENU_ITEMS; nodeIndex++ )
   {
      BCampaignNode* pNode = pCampaign->getNode(nodeIndex);

      if( pNode->getFlag(BCampaignNode::cCinematic) || !pNode->getFlag(BCampaignNode::cVisible) )
         continue;

      BUIMenuItemControl& rMenuItem = mMissionMenuItems[menuIndex++];
      rMenuItem.show();

      if( pCampaignProgress && pCampaignProgress->isScenarioUnlocked( pNode->getID() ) )
      {
         rMenuItem.setText(pNode->getDisplayName());
         rMenuItem.setControlID(nodeIndex);

         mMissionMenu.addControl(&rMenuItem);

         //if( nodeIndex == mCurrentMission )
            //selectedIndex = menuIndex - 1;
      }
      else
      {
         rMenuItem.setText( L"$$LOCKED$$" );
      }
   }

   mMissionMenu.setIndex( 0 ); // selectedIndex );

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignMissionPicker::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   handled = mMissionMenu.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   handled = mHelpText.handleInput(port, event, controlType, detail);

   return handled;
}

//============================================================================
//============================================================================
bool BUICampaignMissionPicker::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {
      /*
      if (mSecondaryMenu.isShown())
      {
         mSecondaryMenu.hide();
         return true;
      }
      */

      gUI.playCancelSound();

      this->setVisible(false);
      if (mpParent)
         mpParent->setVisible(true);
      return true;
   }
   return false;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICampaignMissionPicker::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.
   bool handled = false;

   if (control->getControlID() == cMissionPickerMissionMenu)
   {
      BUIListControl *listControl = (BUIListControl*)control;
      if (event.getID() == BUIListControl::eItemSelected)
      {
         BUIMenuItemControl *mi = (BUIMenuItemControl *)listControl->getSelectedControl();
         mCurrentMission = mi->getControlID();
         populateScreen();
      }
   }
   else if (control->getControlTypeID() == UIMenuItemControlID)
   {
      if ((event.getID() == BUIControl::eStringEvent) && (event.getString() == "accept"))
      {
         gUI.playConfirmSound();

         // [7/18/2008 xemu] actually launch mission! 
         BCampaign * pCampaign = gCampaignManager.getCampaign(0);
         BASSERT(pCampaign);

         // [7/21/2008 xemu] @TODO - figure out continuous mode? 
         pCampaign->setPlayContinuous(true);
         pCampaign->setCurrentNodeID(control->getControlID());
         pCampaign->launchGame(true);
      }
   }
   return handled;
}

//============================================================================
//============================================================================
bool BUICampaignMissionPicker::displayButtons()
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
   s0.set(gDatabase.getLocStringFromID(24801)); // PLAY

   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(24802)); // BACK


   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}

//============================================================================
// BUICampaignMissionPicker::setScoreText
//============================================================================
void BUICampaignMissionPicker::setScoreText(const BUString &text)
{
   GFxValue value;
   value.SetStringW(text.getPtr());

   invokeActionScript( "setScoreText", &value, 1);
}

//============================================================================
// BUICampaignMissionPicker::setMedalText
//============================================================================
void BUICampaignMissionPicker::setMedalText(const BUString &text)
{
   GFxValue value;
   value.SetStringW(text.getPtr());

   invokeActionScript( "setMedalText", &value, 1);
}

//============================================================================
// BUICampaignMissionPicker::setSkullText
//============================================================================
void BUICampaignMissionPicker::setSkullText(const BUString &text)
{
   GFxValue value;
   value.SetStringW(text.getPtr());

   invokeActionScript( "setSkullText", &value, 1);
}

//============================================================================
// BUICampaignMissionPicker::setBlackBoxText
//============================================================================
void BUICampaignMissionPicker::setBlackBoxText(const BUString &text)
{
   GFxValue value;
   value.SetStringW(text.getPtr());

   invokeActionScript( "setBlackBoxText", &value, 1);
}

//============================================================================
// BUICampaignMissionPicker::setParTimeText
//============================================================================
void BUICampaignMissionPicker::setParTimeText(const BUString &text)
{
   GFxValue value;
   value.SetStringW(text.getPtr());

   invokeActionScript( "setParTimeText", &value, 1);
}

//============================================================================
// BUICampaignMissionPicker::setCoopText
//============================================================================
void BUICampaignMissionPicker::setCoopText(const BUString &text)
{
   GFxValue value;
   value.SetStringW(text.getPtr());

   invokeActionScript( "setCoopText", &value, 1);
}
