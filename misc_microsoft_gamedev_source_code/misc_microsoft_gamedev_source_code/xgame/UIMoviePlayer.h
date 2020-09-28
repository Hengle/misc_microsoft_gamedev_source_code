//============================================================================
// UIMoviePlayer.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIButtonControl.h"
#include "UITextFieldControl.h"

class BMovieHandler
{
public:
   virtual void play() = 0;
   virtual void pause() = 0;
   virtual void stop() = 0;
   virtual void skipBack() = 0;
   virtual void rewind() = 0;
   virtual void fastForward() = 0;
   virtual void skipForward() = 0;

   virtual bool isPaused() = 0;
};

class BUIMoviePlayer : public BUIScreen
{
public:
   BUIMoviePlayer( void );
   virtual ~BUIMoviePlayer( void );

   virtual bool init( const char* filename, const char* datafile );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   virtual void update( float dt );

   virtual void setVisible( bool visible );

   virtual void checkAutoHide();

   virtual void setCaptionData(bool enable, const BUString& text);
   void checkCaptionText();
   void clearCaptionText();


   // BBinkVideoStatus methods
   void setMovieHandler(BMovieHandler* pHandler) { mpMovieHandler = pHandler; }

   void doHide();
   void doShow();

protected:

   void setupButtons();


   //BUIMoviePlayerControl   mMovieController;

   BUIButtonControl  mButton1;
   BUIButtonControl  mButton2;
   BUIButtonControl  mButton3;

   BMovieHandler*    mpMovieHandler;

   BUITextFieldControl mCaptionTextField;

   BUString          mCaptionText;
   DWORD             mAutoHideTime;
   bool              mShowing;
};