//============================================================================
// UICampaignMissionPicker.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UISkullPicker.h"
#include "UICheckedMenuItemControl.h"
#include "UIListControl.h"
#include "database.h"
#include "skullmanager.h"
#include "usermanager.h"
#include "user.h"
#include "soundmanager.h"

//============================================================================
//============================================================================
BUISkullPicker::BUISkullPicker( void ) : 
   mCurrentSkull(0),
   mInitialized(false)
{
}

//============================================================================
//============================================================================
BUISkullPicker::~BUISkullPicker( void )
{
   BUICheckedMenuItemControl* pControl = NULL;
   for (int i=0; i<mSkullControls.getNumber(); i++)
   {
      pControl = mSkullControls[i];
      delete pControl;
      mSkullControls[i]=NULL;
   }
   mSkullControls.clear();

   mSkullMapping.clear();
}

//============================================================================
//============================================================================
bool BUISkullPicker::init( const char* filename, const char* datafile )
{
   return BUIScreen::init(filename, datafile);
}

//============================================================================
//============================================================================
bool BUISkullPicker::init(BXMLNode dataNode)
{
   BUIScreen::init(dataNode);

   mTitle.init(this, "mTitle", -1, NULL);
   mDescriptionLabel.init(this, "mDescriptionLabel", -1, NULL);
   mReminderLabel.init(this, "mReminderLabel", -1, NULL);
   mScoreLabel.init(this, "mScoreLabel", -1, NULL);

   // initialize all the components for our screen
   mSkullList.init(this, "mSkullMenu", 0, NULL);
   mSkullList.setWrap(true);

   addSkulls();   

   mHelpText.init(this, "mHelpText", 0, NULL);
   
   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   initImageViewer();

   updateSelection(gUserManager.getPrimaryUser());
   displayButtons();

   showSkullMenu();
   mInitialized = true;
   return true;
}

//============================================================================
//============================================================================
void BUISkullPicker::initImageViewer()
{
   mImageViewer.init(this, "mImageViewer", 0, NULL);

   mImageViewer.setAutoTransition(true);
   mImageViewer.setViewDuration(5000.0f);   

   mImageViewer.start();   
}

//============================================================================
//============================================================================
void BUISkullPicker::updateSelection(BUser* pUser)
{  
   mTitle.setText(gDatabase.getLocStringFromID(24988));

   bool bCollected = false;
   if (pUser)
      bCollected = gCollectiblesManager.hasSkullBeenCollected(pUser, mCurrentSkull);  
   bool isActivated = gCollectiblesManager.getSkullActivationUI(mCurrentSkull);
   
   mDescriptionLabel.setText(bCollected ? gDatabase.getLocStringFromID(25021) : gDatabase.getLocStringFromID(25022));
   mReminderLabel.setText(gDatabase.getLocStringFromID(25860));

   float scoreMod = gCollectiblesManager.getScoreModifierUI();
   if (scoreMod == 1.0f)
      mScoreLabel.setText("");
   else
   {
      BUString scoreText;
      BUString txt = gDatabase.getLocStringFromID(25861);
      scoreText.locFormat(gDatabase.getLocStringFromID(25991), txt.getPtr(), scoreMod);
      mScoreLabel.setText(scoreText);
   }

   mImageViewer.clearImages();
   const BProtoSkull* pProtoSkull = gCollectiblesManager.getSkullDefinition(mCurrentSkull);
   if (pProtoSkull)
   {      
      mHelpText.setText(gDatabase.getLocStringFromID(pProtoSkull->mDescriptionID));

      BString temp;
      temp.format("img://%s", pProtoSkull->mDisplayImageLocked.getPtr());

      if (bCollected)      
      {
         if (isActivated)
            temp.format("img://%s", pProtoSkull->mDisplayImageOn.getPtr());
         else
            temp.format("img://%s", pProtoSkull->mDisplayImageOff.getPtr());
      }
      mImageViewer.addImage(temp);
   }   
   mImageViewer.start();   
}

//============================================================================
//============================================================================
void BUISkullPicker::initMenuItem(int index)
{
   BSimString controlPath;
   controlPath.format("mSkullMenu.mMenuItem%d", index);
   
   BUICheckedMenuItemControl* pMenuItem = new BUICheckedMenuItemControl();
   pMenuItem->init( this, controlPath.getPtr(), cSkullPickerList);

   if (index >= mSkullMapping.getNumber())
   {
      pMenuItem->hide();
      mSkullList.addControl(pMenuItem);
      mSkullControls.add(pMenuItem);
      return;
   }

   // do we have the skull yet?
   int skullIndex = mSkullMapping[index];
   bool bSkullLocked = gCollectiblesManager.hasSkullBeenCollected(gUserManager.getPrimaryUser(), skullIndex);
   bool isActivated  = gCollectiblesManager.getSkullActivationUI(skullIndex);

   const BProtoSkull* pProtoSkull = gCollectiblesManager.getSkullDefinition(skullIndex);
   if (!pProtoSkull)
   {
      pMenuItem->hide();
      mSkullList.addControl(pMenuItem);
      mSkullControls.add(pMenuItem);
      return;
   }

   //int nextIndex = mSkullList.getControlCount();

   int displayStringID = bSkullLocked ? pProtoSkull->mDisplayNameID : 25022;
   pMenuItem->setText(gDatabase.getLocStringFromID(displayStringID));
   pMenuItem->setData(skullIndex);
   pMenuItem->check(isActivated);

   mSkullList.addControl(pMenuItem);
   mSkullControls.add(pMenuItem);
}

//============================================================================
// BUISkullPicker::addSkulls
//============================================================================
bool BUISkullPicker::addSkulls()
{      
   gCollectiblesManager.unlockHiddenSkulls();

   int i;
   int skullCount = 0;
   // [10/8/2008 xemu] go through the skull manager, find all the non-hidden skulls, and create a mapping between menu indices and skull indices 
   for (i=0; i < gCollectiblesManager.getNumberSkulls(true); i++)
   {
      if (!gCollectiblesManager.isSkullHidden(i))
      {
         skullCount++;
         mSkullMapping.resize(skullCount);
         mSkullMapping[skullCount-1] = i;
      }
   }

   // [10/8/2008 xemu] now add the actual menu items in accordance 
   for (i=0; i < cMaxMenuItems; i++)
      initMenuItem(i);
   
   return true;
}

//============================================================================
//============================================================================
bool BUISkullPicker::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   handled = mSkullList.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   // Return true here because this screen can come up in-game and we don't want
   // the game to handle any input.
   return true;
}

//============================================================================
// BUISkullPicker::executeInputEvent
//============================================================================
bool BUISkullPicker::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {      
      this->setVisible(false);

      if (mpParent)
         mpParent->setVisible(true);

      gCollectiblesManager.sendSkullActivationCommands(gUserManager.getPrimaryUser()->getPlayerID());

      gUI.playCancelSound();

      return true;
   }
   return false;
}

// IUIControlEventHandler
//==============================================================================
// BUISkullPicker::handleUIControlEvent
//==============================================================================
bool BUISkullPicker::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.
   bool handled = false;

   if (control->getControlID() == cSkullPickerList)
   {
      BUIListControl *listControl = (BUIListControl*)control;
      if (event.getID() == BUIListControl::eItemSelected)
      {
         BUICheckedMenuItemControl *mi = (BUICheckedMenuItemControl *)listControl->getSelectedControl();
         mCurrentSkull = mi->getData();        
         updateSelection(gUserManager.getPrimaryUser());
      }
      else if (control->getControlTypeID() == UICheckedMenuItemControlID)
      {
         if ((event.getID() == BUIControl::eStringEvent) && (event.getString() == "accept"))
         {
            BUICheckedMenuItemControl* pControl = reinterpret_cast<BUICheckedMenuItemControl*>(control);
            
            int skullIndex = pControl->getData();
            bool bCollected = gCollectiblesManager.hasSkullBeenCollected(gUserManager.getPrimaryUser(), skullIndex);
            bool success = false;
            if (bCollected)
            {
               // [10/9/2008 xemu] don't allow un-checking an already active skull
               if (gCollectiblesManager.canToggleSkull(skullIndex))
               {
                  pControl->toggleChecked();
                  gCollectiblesManager.setSkullActivationUI(skullIndex, pControl->isChecked(), gUserManager.getPrimaryUser()->getPlayerID());
                  updateSelection(gUserManager.getPrimaryUser());
                  success = true;
               }
            }

            if (!success)
            {
               gSoundManager.playCue( "play_ui_cant_do_that" );
            }
         }
      }
   }
   return handled;
}

//============================================================================
//============================================================================
bool BUISkullPicker::displayButtons()
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
   s0.set(gDatabase.getLocStringFromID(24064)); // PLAY

   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(24802)); // BACK

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}

//============================================================================
//============================================================================
void BUISkullPicker::showSkullMenu()
{
   disableSound();
   BUICheckedMenuItemControl* pMenuItem = NULL;  
   int menuIndex = 0;
   int selectedIndex = 0;
   for (int i = 0; i < mSkullMapping.getNumber(); ++i)
   {
      int skullIndex = mSkullMapping[i];
      const BProtoSkull* pProtoSkull = gCollectiblesManager.getSkullDefinition(skullIndex);
      if (!pProtoSkull)
         continue;
     
      pMenuItem = mSkullControls[menuIndex];
      if (!pMenuItem)
         continue;

      pMenuItem->setText(gDatabase.getLocStringFromID(pProtoSkull->mDisplayNameID));
      pMenuItem->setData(skullIndex);
      pMenuItem->check(gCollectiblesManager.getSkullActivationUI(skullIndex));
      if (skullIndex == mCurrentSkull)
         selectedIndex=menuIndex;
      menuIndex++;      
   }

   mSkullList.setIndex(selectedIndex);
   enableSound();
}

//============================================================================
// BUISkullPicker::enter
//============================================================================
void BUISkullPicker::enter( void )
{
   BUIScreen::enter();

   showSkullMenu();
}

