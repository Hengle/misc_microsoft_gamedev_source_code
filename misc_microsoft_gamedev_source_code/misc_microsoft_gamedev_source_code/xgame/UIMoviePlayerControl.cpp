//============================================================================
// UIListControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIMoviePlayerControl.h"
#include "UIButtonControl.h"
#include "UIInputHandler.h"
#include "database.h"

//==============================================================================
//==============================================================================
BUIMoviePlayerControl::BUIMoviePlayerControl( void ) :
   mPaused(false)
{
   mWrap = false;
   mAlignment = eHorizontal;
   mSelectedIndex = 0;

   mControlType.set("UIMoviePlayerControl");
}

//==============================================================================
//==============================================================================
BUIMoviePlayerControl::~BUIMoviePlayerControl( void )
{

   BUIButtonControl* c = NULL;
   for (int i=0; i<mControlButtons.getNumber(); i++)
   {
      c = mControlButtons[i];
      delete c;
      mControlButtons[i]=NULL;
   }
   mControlButtons.clear();
}

//==============================================================================
//==============================================================================
bool BUIMoviePlayerControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData /*= NULL*/ )
{
   bool success = __super::init(parent, controlPath, controlID, initData);

   // our input handler needs to be horizontal
   mpInputHandler->enterContext("Main");

   // add all my buttons
   addButtons();

   setIndex(0);

   return success;
}


//============================================================================
//============================================================================
bool BUIMoviePlayerControl::addButtons()
{
   // addButton(L"PLAY",         "play",        0, eButtonPlayPause);
   addButton(gDatabase.getLocStringFromID(25056), "pause",        0, eButtonPlayPause); //L"PAUSE"
   mPaused=false;
   addButton(gDatabase.getLocStringFromID(25057), "skip_back",    1, eButtonSkipBack);   //L"SKIP BACK"   
   addButton(gDatabase.getLocStringFromID(25058), "rewind",       2, eButtonRewind);  //L"REWIND"      
   addButton(gDatabase.getLocStringFromID(25059), "stop",         3, eButtonStop);  //L"STOP"        
   addButton(gDatabase.getLocStringFromID(25060), "forward",      4, eButtonFastForward);   //L"FAST FORWARD"
   addButton(gDatabase.getLocStringFromID(25061), "skip_forward", 5, eButtonSkipForward);  //L"SKIP FORWARD"
   addButton(gDatabase.getLocStringFromID(25062), "subtitles",    6, eButtonSubtitles);  //L"SUBTITLES"   
   addButton(gDatabase.getLocStringFromID(25063), "info",         7, eButtonInfo);  //L"INFO"        

   return true;
}

//============================================================================
//============================================================================
bool BUIMoviePlayerControl::handleUIControlEvent( BUIControlEvent& event )
{
   if (event.getID() == BUIButtonControl::ePress)
   {
      BUIButtonControl* pButton = (BUIButtonControl*)event.getControl();
      BASSERT(pButton);
      if (!pButton)
         return false;        // can't do anything with this.

      switch (pButton->getControlID())
      {
         case BUIMoviePlayerControl::eButtonPlayPause:
            BUString text;
            if (!mPaused)
            {
               // we are going to pause, so turn on play button
               text.set(gDatabase.getLocStringFromID(25064));//L"PLAY");         // locme
               pButton->setText(text);
               pButton->setImage("play");
            }
            else
            {
               // we are going to play, so turn on pause
               text.set(gDatabase.getLocStringFromID(25056));//L"PAUSE");         // locme
               pButton->setText(text);
               pButton->setImage("pause");
            }
            // this is a hack to get the text to show up on the bottom of the control.
            pButton->unfocus();           
            pButton->focus();

            mPaused = !mPaused;           // toggle this.
            break;
      }
   }

   fireUIControlEvent( eChildControlEvent, &event);
   // This will handle the pressed buttons for the player. It needs to do something with those.
   return false;
}


//============================================================================
//============================================================================
bool BUIMoviePlayerControl::addButton(const WCHAR* buttonText, const char* buttonImage, int index, int controlID)
{

   BSimString controlPath;
   controlPath.format("%s.mButton%d", mControlPath.getPtr(), index);
   BUString text;
   text.set(buttonText);

   BUIButtonControl *pButton = new BUIButtonControl();
   pButton->init( this, controlPath.getPtr(), controlID);
   pButton->setText(text);
   pButton->setImage(buttonImage);

   addControl(pButton);
   mControlButtons.add(pButton);

   return true;
}




//==============================================================================
//==============================================================================
bool BUIMoviePlayerControl::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   if (command=="prev")
   {
      prev();
      return true;
      // fixme - bubble the event up too.
   }
   else if (command=="next")
   {
      next();
      // fixme - bubble the event up too.
      return true;
   }

   return false;
}


//==============================================================================
//==============================================================================
bool BUIMoviePlayerControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = BUIControl::handleInput(port, event, controlType, detail);

   // If we didn't handle it, then try the selected item.
   if (!handled)
   {
      if ( (mSelectedIndex >=0) && (mSelectedIndex <mControls.getNumber()) )
      {
         BUIControl * c = mControls[mSelectedIndex];
         if (c && c->isFocused() && c->isEnabled())
            handled = c->handleInput(port, event, controlType, detail);
      }
   }

   return handled;
}

