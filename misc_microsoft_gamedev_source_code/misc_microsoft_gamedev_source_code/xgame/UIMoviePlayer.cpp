//============================================================================
// UIMoviePlayer.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIMoviePlayer.h"

#include "UIMenuItemControl.h"
#include "campaignmanager.h"
#include "configsgame.h"
#include "gamedirectories.h"
#include "UIButtonControl.h"
#include "ModeCampaign2.h"
#include "modemanager.h"
#include "database.h"

#include "game.h"

DWORD cAutoHideTime = 2000;

//============================================================================
//============================================================================
BUIMoviePlayer::BUIMoviePlayer( void ) : 
   mpMovieHandler(NULL),
   mShowing(false),
   mCaptionText(L"")
{
}

//============================================================================
//============================================================================
BUIMoviePlayer::~BUIMoviePlayer( void )
{
}

//============================================================================
//============================================================================
void BUIMoviePlayer::update( float dt )
{
   // [8/14/2008 xemu] support for auto-hiding UI controls after a time with no input 
   checkAutoHide();

   // update our caption text
   checkCaptionText();
}

//============================================================================
//============================================================================
void BUIMoviePlayer::checkCaptionText()
{
   if (mCaptionText != mCaptionTextField.getText())
      mCaptionTextField.setText(mCaptionText);
}

//============================================================================
//============================================================================
void BUIMoviePlayer::clearCaptionText()
{
   mCaptionText.set(L"");
   checkCaptionText();
}



//============================================================================
//============================================================================
bool BUIMoviePlayer::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   mButton1.init(this, "mButton1");
   mButton2.init(this, "mButton2");
   mButton3.init(this, "mButton3");

   mCaptionTextField.init(this, "mCaptionText");

   setupButtons();

   return true;
}

//============================================================================
//============================================================================
void BUIMoviePlayer::setupButtons()
{
   bool isPaused = false;
   BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
   if (pMode)
      isPaused = pMode->isPaused();

   GFxValue args[6];
   BUString text1;
   BUString art1;
   BUString text2;
   BUString art2;
   BUString text3;
   BUString art3;

   if (isPaused)
      text1.set(gDatabase.getLocStringFromID(25064));
   else
      text1.set(gDatabase.getLocStringFromID(25056));
   art1.set("Abtn");

   text2.set(gDatabase.getLocStringFromID(23469));
   art2.set("Xbtn");

   if ((pMode != NULL) && (pMode->getPlayAllIndex() != -1))
   {
      text3.set(gDatabase.getLocStringFromID(25153));
      art3.set("Bbtn");
   }
   else
   {
      text3.set("");
      art3.set("");
   }

   args[0].SetStringW( art1 );
   args[1].SetStringW( text1 );
   args[2].SetStringW( art2 );
   args[3].SetStringW( text2 );
   args[4].SetStringW( art3 );
   args[5].SetStringW( text3 );
   invokeActionScript( "setupButtons", args, 6 );

   //doHide();

}

//============================================================================
//============================================================================
void BUIMoviePlayer::setVisible( bool visible )
{
   BUIScreen::setVisible(visible);

   // [8/14/2008 xemu] we never want these up fresh, so always hide in this case
   if (visible)
      setupButtons();
   else
      doHide();
}


//============================================================================
//============================================================================
void BUIMoviePlayer::doHide()
{
   invokeActionScript( "onHide" );
   mAutoHideTime = 0;
   mShowing = false;
}

//============================================================================
//============================================================================
void BUIMoviePlayer::doShow()
{
   invokeActionScript( "onShow" );

   mAutoHideTime = gGame.getTotalTime() + cAutoHideTime;
   mShowing = true;
}

//============================================================================
//============================================================================
bool BUIMoviePlayer::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
   BASSERT(pMode);
   if (pMode == NULL)
      return(false);

   if (command=="show")
   {
      doShow();
      return true;
   }
   else if (command=="hide")
   {
      doHide();
      return true;
   }
   else if (command == "pause")
   {
      pMode->togglePause();
   }
   else if (command == "showOrExit")
   {
      if (!mShowing)
      {
         doShow();
      }
      else
      {
         pMode->stop();
      }
   }
   else if (command == "showOrPause")
   {
      if (!mShowing)
      {
         doShow();
      }
      else
      {
         pMode->togglePause();
      }
   }
   else if (command == "showOrAdvance")
   {
      if (!mShowing || (pMode->getPlayAllIndex() == -1))
      {
         doShow();
      }
      else
      {
         pMode->advanceMovie();
      }
   }

   // [8/14/2008 xemu] always show if we are paused
   if (pMode->isPaused())
      doShow();

   setupButtons();

   return false;
}


//============================================================================
//============================================================================
bool BUIMoviePlayer::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   // the only input handling here is currently done by the player control.
   //if (mMovieController.isShown())
      //handled = mMovieController.handleInput(port, event, controlType, detail);
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
bool BUIMoviePlayer::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   BASSERT(control);
   if (!control)
      return false;        // can't do anything with this.

   /*
   if (event.getID() == BUIControl::eChildControlEvent)
   {
      const BUIControlEvent* childEvent = event.getChildEvent();
      BASSERT(childEvent);
      if (!childEvent)
         return false;

      BUIControl *childControl = childEvent->getControl();
      BASSERT(childControl);
      if (!childControl)
         return false;

      if (childEvent->getID() == BUIButtonControl::ePress)
      {
         BASSERT(mpMovieHandler);
         if (!mpMovieHandler)
            return false;
         switch (childControl->getControlID())
         {
            case BUIMoviePlayerControl::eButtonPlayPause:
               if (mpMovieHandler->isPaused())
                  mpMovieHandler->play();
               else
                  mpMovieHandler->pause();
               break;
            case BUIMoviePlayerControl::eButtonSkipBack:
               mpMovieHandler->skipBack();
               break;
            case BUIMoviePlayerControl::eButtonRewind:
               mpMovieHandler->rewind();
               break;
            case BUIMoviePlayerControl::eButtonStop:
               mpMovieHandler->stop();
               break;
            case BUIMoviePlayerControl::eButtonFastForward:
               mpMovieHandler->fastForward();
               break;
            case BUIMoviePlayerControl::eButtonSkipForward:
               mpMovieHandler->skipForward();
               break;
            case BUIMoviePlayerControl::eButtonSubtitles:
               // fixme - support this?
               break;
            case BUIMoviePlayerControl::eButtonInfo:
               // fixme - support this?
               break;
         }

         // do play pause stuff
      }
   }
   */

   return true;;
}

//==============================================================================
//==============================================================================
void BUIMoviePlayer::checkAutoHide()
{
   BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
   DWORD curTime = gGame.getTotalTime();
   // [8/14/2008 xemu] never time out while paused 
   if ((mAutoHideTime > 0) && (curTime > mAutoHideTime) && (pMode != NULL) && (!pMode->isPaused()))
   {
      doHide();
   }
}

//==============================================================================
//==============================================================================
void BUIMoviePlayer::setCaptionData(bool enable, const BUString& text)
{
   mCaptionText.set(text);
}


