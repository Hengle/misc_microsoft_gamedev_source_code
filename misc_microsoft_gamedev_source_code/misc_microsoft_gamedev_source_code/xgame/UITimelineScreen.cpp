//============================================================================
// UICampaignMoviePicker.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UITimelineScreen.h"

#include "gamedirectories.h"
#include "database.h"
#include "UIMenuItemControl.h"
#include "UITimelineEventControl.h"
#include "skullmanager.h"
#include "usermanager.h"

#include "soundmanager.h"
#include "ui.h"

int __cdecl timelineEventSort(const void* pItem1, const void* pItem2)
{
   // Sort by position on the timeline
/*
   BUITimelineEventControl* pControl1 = (BUITimelineEventControl*)(&pItem1);
   BUITimelineEventControl* pControl2 = (BUITimelineEventControl*)(&pItem2);
*/

   BUITimelineEventControl* pControl1 = *((BUITimelineEventControl**)pItem1);
   BUITimelineEventControl* pControl2 = *((BUITimelineEventControl**)pItem2);


   if (pControl1->getPosition() < pControl2->getPosition())
      return -1;
  
   if (pControl2->getPosition() < pControl1->getPosition())
      return 1;

   return 0;
}



//============================================================================
//============================================================================
BUITimelineScreen::BUITimelineScreen( void ):
mNumFlashPanels(3),
mUpdateCount(0)
{
   mTimelineControls.clear();

   mName.set("UITimelineScreen");
}

//============================================================================
//============================================================================
BUITimelineScreen::~BUITimelineScreen( void )
{
   BUIControl* c = NULL;
   for (int i=0; i<mTimelineControls.getNumber(); i++)
   {
      c = mTimelineControls[i];
      delete c;
      mTimelineControls[i]=NULL;
   }

   mTimelineControls.clear();

   gSoundManager.playCue( "stop_timeline_menu_bg_02" );
}

//============================================================================
//============================================================================
bool BUITimelineScreen::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   return true;
}

//============================================================================
// BUITimelineScreen::init
//============================================================================
bool BUITimelineScreen::init(BXMLNode dataNode)
{
   // Read the timeline
   mTimeline.loadEvents();


   BXMLNode controlsNode;
   if (dataNode.getChild( "UIListControl", &controlsNode ) )
      mEventList.init(this, "mEventList", -1, &controlsNode);
   else
   {
      BASSERTM(false, "BUITimelineScreen:: Unable to initialize mEventList control input handler.");
      mEventList.init(this, "mEventList");    // use default
   }

   mEventList.setAlignment(BUIListControl::eHorizontal);

   BXMLNode calloutNode;
   if( dataNode.getChild( "UITimelineCalloutControl", &calloutNode) )
   {
      mCallout.init(this, "mCallout", -1, &calloutNode);
   }
   else
   {
      mCallout.init(this, "mCallout");
   }

   mCallout.hide(true);

   mTextLeftCount.init(this, "mTextLeftCount");
   mTextRightCount.init(this, "mTextRightCount");

   mTextCenterCount.init(this, "mTextCenterCount");
   BUString temp; 
   temp.set(L"");
   mTextCenterCount.setText(temp);


   mTitle.init(this, "mTitle");
   mTitle.setText(gDatabase.getLocStringFromID(25244));


   mButtonBar.init(this, "mButtonBar");

   createEvents();

   // Add our menu items into the menu
   // initTimelineEvents(dataNode);

   return(BUIScreen::init(dataNode));
}

//============================================================================
//============================================================================
void BUITimelineScreen::moveToIndex(int index)
{

   BUITimelineEventControl* pControl = (BUITimelineEventControl*)mEventList.getControl(index);
   if (!pControl)
      return;

   GFxValue value;
   value.SetString(pControl->getControlPath().getPtr());

   invokeActionScript("moveToEvent", &value, 1);
}

//============================================================================
//============================================================================
void BUITimelineScreen::populate()
{
   // fixme - add code here
   moveToIndex(0);

   mEventList.setIndex(0);
   updateLeftRightCounters();
   displayButtons();

   gSoundManager.playCue( "Set_State_Timeline" );
   gSoundManager.playCue( "play_timeline_menu_bg_02" );
}


//============================================================================
//============================================================================
void BUITimelineScreen::enter()
{
   __super::enter();
}

//============================================================================
//============================================================================
void BUITimelineScreen::leave()
{
   __super::leave();
}

//============================================================================
// Fixme - we should do the following here:
// 1) Limit to only the unlocked ones (or new ones)
// 2) not count the ones on the screen.
//============================================================================
void BUITimelineScreen::updateLeftRightCounters()
{
   int leftCount=mEventList.getSelectedIndex();
   int rightCount=mEventList.getControlCount()-leftCount-1;

   BUString temp;

   temp.locFormat(L"%d", leftCount);
   mTextLeftCount.setText(temp);
   temp.locFormat(L"%d", rightCount);
   mTextRightCount.setText(temp);

}

//============================================================================
//============================================================================
bool BUITimelineScreen::createEvents()
{
   for (int i=0; i<mTimeline.getNumberEvents(); i++)
   {
      BTimelineEvent* pEvent = mTimeline.getEvent(i);
      if (!pEvent)
         continue;
   
      // calculate which panel it needs to be on.
      float panelFraction = 100.0f/(float)mNumFlashPanels;
      float currentFraction = panelFraction;
      int panel = 0;
      for (int p=0; p<mNumFlashPanels; p++)
      {
         // check to see if the event should be attached to this panel.
         if (pEvent->getPosition() <= currentFraction)
         {
            panel = p;
            break;
         }
         currentFraction+=panelFraction;
      }

      createTimelineEvent(panel, i, pEvent);
   }

   // set all the events as "Not New"
   gCollectiblesManager.markAllTimeLineEventsSeen(gUserManager.getPrimaryUser());

   // sort these by position
   mTimelineControls.sort(timelineEventSort);

   BUITimelineEventControl* pControl=NULL;
   for (int i=0; i<mTimelineControls.getNumber(); i++)
   {
      pControl = (BUITimelineEventControl*)mTimelineControls[i];
      if (!pControl)
         continue;

      // add it to the list control in the proper proper order
      mEventList.addControl(pControl);
   }

   return true;
}

//============================================================================
//============================================================================
bool BUITimelineScreen::createTimelineEvent(int panel, int index, const BTimelineEvent* pEvent)
{
   BString controlName;
   controlName.format("mTimelinePanel%d.m%d", panel, index);

   float panelFraction = 100.0f/(float)mNumFlashPanels;
   float localPosition=pEvent->getPosition()-(panel*panelFraction);

   // call flash to create this object for us.
   createFlashTimelineEvent(panel, index, localPosition, pEvent->getPosition());

   // init the control
   BUITimelineEventControl* pControl = new BUITimelineEventControl();
   pControl->init(this, controlName.getPtr());
   if (pEvent->getStatus() == BTimelineEvent::cStatusLocked)
      pControl->setImage(mTimeline.getLockedImage().getPtr());
   else 
      pControl->setImage(pEvent->getUnlockedImage().getPtr());

   BString miniControlName;
   miniControlName.format("mMiniTimeline.m%d", index);

   pControl->initMiniEvent(miniControlName.getPtr());

   if (pEvent->isNew())
   {
      pControl->setState(BUITimelineEventControl::cTimelineEventStateNew);
      pControl->getMiniEvent().setState(BUIMiniTimelineEventControl::cMiniTimelineStateNew);
   }
   else if (pEvent->getStatus() == BTimelineEvent::cStatusUnlocked)
   {
      pControl->getMiniEvent().setState(BUIMiniTimelineEventControl::cMiniTimelineStateUnlocked);
      pControl->setState(BUITimelineEventControl::cTimelineEventStateOn);
   }

   /* else leave it in the default state */


   pControl->setPosition(pEvent->getPosition());
   pControl->setEventIndex(index);

   mTimelineControls.add(pControl);

   return true;
}

//============================================================================
//============================================================================
void BUITimelineScreen::createFlashTimelineEvent(int panel, int index, float localPosition, float globalPosition)
{
   GFxValue values[4];

   values[0].SetNumber(panel);
   values[1].SetNumber(index);
   values[2].SetNumber(localPosition);
   values[3].SetNumber(globalPosition);

   invokeActionScript("createFlashTimelineEvent", values, 4);
}


//============================================================================
//============================================================================
bool BUITimelineScreen::initTimelineEvents(BXMLNode dataNode)
{
/*
      mEventList.clearControls();
      mTimelineControls.clear();
   
      // fixme - turn this into a data driven mechanism.
   
      BXMLNode child;
      if( dataNode.getChild( "TimelineControls", &child ) )
      {
         // walk this and get the name of each control.
         for (int i=0; i<child.getNumberChildren(); i++)
         {
            BXMLNode controlNode = child.getChild(i);
   
            BString path;
            path.empty();
            if (!controlNode.getAttribValueAsString("path", path))
            {
               BASSERTM(0, "did not find path attribute");
               continue;
            }
   
            // Create the control and add it to the list
            BUIMenuItemControl* pControl = new BUIMenuItemControl();       // hack - replace with a UITimelineEventControl
            pControl->init(this, path.getPtr());
            mTimelineControls.add(pControl);
            mEventList.addControl(pControl);
         }
      }
*/
   
   return true;
}

//============================================================================
//============================================================================
bool BUITimelineScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {
      if (mCallout.isShown())
      {
         mCallout.hide();
         return true;
      }

      gUI.playCancelSound();
      gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicSetStateMainTheme);

      gSoundManager.playCue( "stop_timeline_menu_bg_02" );

      if( mpHandler )
         mpHandler->handleUIScreenResult( this, 0 );
      return true;
   }
   return false;
}


//============================================================================
//============================================================================
bool BUITimelineScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   handled = mEventList.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   if (mCallout.isShown())
      handled = mCallout.handleInput(port, event, controlType, detail);

   return handled;
}



//============================================================================
//============================================================================
void BUITimelineScreen::displayButtons()
{
   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   // A - accept
   // B - back to main menu
   BUString s0;
   BUString s1;
   BUString s2;
   BUString s3;
   BUString s4;

   b0 = BUIButtonBarControl::cFlashButtonA;
   s0.set(gDatabase.getLocStringFromID(23437));

   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(23440));

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUITimelineScreen::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.
   //bool handled = false;

   if (control->getControlTypeID() == UITimelineEventControlID)
   {
      if ((event.getID() == BUIControl::eStringEvent) && (event.getString() == "accept"))
      {
         // fill in the callout
         if (mCallout.isShown())
         {
            mCallout.hide();
            gSoundManager.playCue( "play_ui_menu_open_and_close" );
         }
         else
         {
            // get the selected index
            // int index = mEventList.getSelectedIndex();
            BUITimelineEventControl* pControl = (BUITimelineEventControl*)mEventList.getSelectedControl();
            if (!pControl)
               return false;

            // get the timeline event
            BTimelineEvent* tlEvent = mTimeline.getEvent(pControl->getEventIndex());
            if (!tlEvent)
            {
               BASSERT(0);
               return true;
            }

            // if unlocked show it.
            if (tlEvent->getStatus() != BTimelineEvent::cStatusUnlocked)
            {
               gSoundManager.playCue( "play_ui_menu_back_button" );
               return true;
            }

            // populate the event and show it.
            mCallout.setTitle(tlEvent->getTitle());
            mCallout.setDescription(tlEvent->getDetailText());
            // mCallout.setImage(tlEvent->getDetailImage().getPtr());
            mCallout.show();
            gSoundManager.playCue( "play_ui_menu_open_and_close" );
         }
      }
   }
   else if (control->getControlTypeID() == UIListControlID)
   {
      // if (event.getID() == BUIListControl::eItemSelected)
      if (event.getID() == BUIListControl::eSelectionChanged)
      {
         BUIListControl *listControl = (BUIListControl*)control;
         BUIMenuItemControl *mi = (BUIMenuItemControl *)listControl->getSelectedControl();

         GFxValue value;
         value.SetString(mi->getControlPath().getPtr());

         invokeActionScript("scrollToEvent", &value, 1);

         updateLeftRightCounters();

         setEventData();

         // fixme - hide control
         mCallout.hide();
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
void BUITimelineScreen::setEventData()
{
   BUITimelineEventControl* pControl = (BUITimelineEventControl*)mEventList.getSelectedControl();
   if (!pControl)
      return;

   // get the timeline event
   BTimelineEvent* tlEvent = mTimeline.getEvent(pControl->getEventIndex());
   if (!tlEvent)
   {
      BASSERT(0);
      return;
   }

   mTextCenterCount.setText(tlEvent->getDateString());
}



//==============================================================================
//==============================================================================
void BUITimelineScreen::update( float dt )
{
   if (mUpdateCount>0)
      return;           // don't check anymore.

   mUpdateCount++;

   if (mUpdateCount==1)
   {
      // set all the images as needed.
      for (int i=0; i<mEventList.getControlCount(); i++)
      {
         BUITimelineEventControl* pControl = (BUITimelineEventControl*)mEventList.getControl(i);
         if (!pControl)
            continue;

         pControl->setImage( pControl->getImage().getPtr() );
      }
   }
}

