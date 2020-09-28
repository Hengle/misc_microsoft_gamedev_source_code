//============================================================================
// BUIServiceRecordScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIServiceRecordScreen.h"
#include "UIServiceRecordCampaignPanelControl.h"
#include "UIServiceRecordSkirmishPanelControl.h"
#include "UITabControl.h"
#include "database.h"
#include "UILabelControl.h"
#include "user.h"
#include "player.h"

//============================================================================
//============================================================================
BUIServiceRecordScreen::BUIServiceRecordScreen( void ) :
   mpPanelCampaign(NULL),
   mpPanelSkirmish(NULL)
{
}

//============================================================================
//============================================================================
BUIServiceRecordScreen::~BUIServiceRecordScreen( void )
{
   for( int i = 0; i < (int)mControls.size(); ++i )
   {
      delete mControls[i];
   }
}

//============================================================================
//============================================================================
bool BUIServiceRecordScreen::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   return true;
}

//============================================================================
//============================================================================
bool BUIServiceRecordScreen::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   // initialize all the components for our screen
   mTitle.init(this, "mTitle", -1, NULL);

   mTabControl.init(this, "");      // no visual, so leave the path empty.

   // add the tabs
   mpPanelCampaign = new BUIServiceRecordCampaignPanelControl();
   mpPanelCampaign->init(this, "mCampaignPanel", -1, &dataNode);
   addTab("mTab0", mpPanelCampaign, gDatabase.getLocStringFromID(24929));

   mpPanelSkirmish = new BUIServiceRecordSkirmishPanelControl();
   mpPanelSkirmish->init(this, "mSkirmishPanel", -1, &dataNode);
   addTab("mTab1", mpPanelSkirmish, gDatabase.getLocStringFromID(24930));

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);
   displayButtons();

   mGamertag.init(this, "mGamertag", -1, NULL);

   return true;
}

//============================================================================
//============================================================================
void BUIServiceRecordScreen::populateFromUser(BUser* pUser)
{
   if (!pUser)
      return;

   mTabControl.setActiveTab(0);
   displayButtons();
   mTitle.setText(gDatabase.getLocStringFromID(25289)); // Service Record

   if(mpPanelSkirmish)
      mpPanelSkirmish->populateFromUser(pUser);

   if(mpPanelCampaign)
      mpPanelCampaign->populateFromUser(pUser);

   mGamertag.populateFromUser(pUser);
}

//============================================================================
//============================================================================
bool BUIServiceRecordScreen::addTab(const char * labelName, BUIPanelControl *panel, const WCHAR* labelText)
{
   BUILabelControl* label = NULL;

   label = new BUILabelControl();
   label->init(this, labelName);
   BUString labelTextString;
   labelTextString.set(labelText);
   label->setText(labelTextString);
   mControls.add(panel);
   mControls.add(label);
   mTabControl.addTab(label, panel);

   return true;
}


//============================================================================
//============================================================================
bool BUIServiceRecordScreen::displayButtons()
{
   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   // change the string IDs and button faces as needed
   // B - Quit
   BUString s0;
   BUString s1;
   BUString s2;
   BUString s3;
   BUString s4;
   
   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(23469));

/*
   b4=BUIButtonBarControl::cFlashButtonudRStick;
   s4.set(gDatabase.getLocStringFromID(24803)); // SCROLL
*/

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}

//============================================================================
//============================================================================
bool BUIServiceRecordScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {      
      gUI.playCancelSound();
      if( mpHandler )
         mpHandler->handleUIScreenResult( this, 0 );

      //this->setVisible(false);
      if (mpParent)
         mpParent->setVisible(true);

      return true;
   }
   return false;
}


//============================================================================
//============================================================================
bool BUIServiceRecordScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = mTabControl.handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   switch (mTabControl.getActiveTab())
   {
      case cServiceRecordTabCampaign:
         if (mpPanelCampaign)
            handled = mpPanelCampaign->handleInput(port, event, controlType, detail);
         break;
      case cServiceRecordTabSkirmish:
         if (mpPanelSkirmish)
            handled = mpPanelSkirmish->handleInput(port, event, controlType, detail);
         break;
   }

   if (handled)
      return handled;

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   return handled;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUIServiceRecordScreen::handleUIControlEvent( BUIControlEvent& event )
{
   return true;
}