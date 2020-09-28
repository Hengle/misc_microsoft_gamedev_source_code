//============================================================================
// UIMoviePlayer.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UICreditsMoviePlayer.h"

#include "UIMenuItemControl.h"
#include "campaignmanager.h"
#include "configsgame.h"
#include "gamedirectories.h"
#include "UIButtonControl.h"
#include "modemanager.h"
#include "database.h"
#include "modemenu.h"

#include "game.h"

DWORD cCreditsAutoHideTime = 2000;

//============================================================================
//============================================================================
BUICreditsMoviePlayer::BUICreditsMoviePlayer( void ) : 
   mShowing(false),
   mCaptionText(L"")
{
   mName.set("UICreditsMoviePlayer");
}

//============================================================================
//============================================================================
BUICreditsMoviePlayer::~BUICreditsMoviePlayer( void )
{
}

//============================================================================
//============================================================================
bool BUICreditsMoviePlayer::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   mCaptionTextField.init(this, "mCaptionText");

   mButton1.init(this, "mButton1");
   mButton2.init(this, "mButton2");
   mButton3.init(this, "mButton3");

   setupButtons();

   return true;
}

//============================================================================
//============================================================================
void BUICreditsMoviePlayer::setupButtons()
{
   bool isPaused = false;
   BModeMenu* pMode = (BModeMenu*)gModeManager.getMode(BModeManager::cModeMenu);
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

   text3.set("");
   art3.set("");

   args[0].SetStringW( art1 );
   args[1].SetStringW( text1 );
   args[2].SetStringW( art2 );
   args[3].SetStringW( text2 );
   args[4].SetStringW( art3 );
   args[5].SetStringW( text3 );
   invokeActionScript( "setupButtons", args, 6 );

   // doHide();

}

//============================================================================
//============================================================================
void BUICreditsMoviePlayer::setVisible( bool visible )
{
   BUIScreen::setVisible(visible);

   // [8/14/2008 xemu] we never want these up fresh, so always hide in this case
   if (visible)
   {
      doHide();
   }
   else
      doHide();

   setupButtons();

}


//============================================================================
//============================================================================
void BUICreditsMoviePlayer::doHide()
{
   invokeActionScript( "onHide" );
   mAutoHideTime = 0;
   mShowing = false;
}

//============================================================================
//============================================================================
void BUICreditsMoviePlayer::doShow()
{
   invokeActionScript( "onShow" );

   mAutoHideTime = gGame.getTotalTime() + cCreditsAutoHideTime;
   mShowing = true;
}

//============================================================================
//============================================================================
void BUICreditsMoviePlayer::stop()
{
   if( mpHandler )
   {
      mpHandler->handleUIScreenResult( this, 0 );
   }
}

//============================================================================
//============================================================================
bool BUICreditsMoviePlayer::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   BModeMenu* pMode = (BModeMenu*)gModeManager.getMode(BModeManager::cModeMenu);
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
         stop();
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


   // [8/14/2008 xemu] always show if we are paused
   if (pMode->isPaused())
      doShow();

   setupButtons();

   return false;
}


//============================================================================
//============================================================================
bool BUICreditsMoviePlayer::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

   return handled;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICreditsMoviePlayer::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   BASSERT(control);
   if (!control)
      return false;        // can't do anything with this.

   return true;;
}

//==============================================================================
//==============================================================================
void BUICreditsMoviePlayer::checkAutoHide()
{
   BModeMenu* pMode = (BModeMenu*)gModeManager.getMode(BModeManager::cModeMenu);
   DWORD curTime = gGame.getTotalTime();

   // [8/14/2008 xemu] never time out while paused 

   if ((mAutoHideTime > 0) && (curTime > mAutoHideTime) && (pMode != NULL) && (!pMode->isPaused()))
   {
      doHide();
   }
}

//==============================================================================
//==============================================================================
void BUICreditsMoviePlayer::update( float dt )
{
   checkAutoHide();

   // update our caption text
   checkCaptionText();
}

//============================================================================
//============================================================================
void BUICreditsMoviePlayer::checkCaptionText()
{
   if (mCaptionText != mCaptionTextField.getText())
      mCaptionTextField.setText(mCaptionText);
}

//============================================================================
//============================================================================
void BUICreditsMoviePlayer::clearCaptionText()
{
   mCaptionText.set(L"");
   checkCaptionText();
}

//==============================================================================
//==============================================================================
void BUICreditsMoviePlayer::setCaptionData(bool enable, const BUString& text)
{
   mCaptionText.set(text);
}