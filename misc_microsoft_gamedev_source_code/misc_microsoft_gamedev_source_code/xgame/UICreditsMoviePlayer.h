//============================================================================
// UIMoviePlayer.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIButtonControl.h"
#include "UITextFieldControl.h"

class BUICreditsMoviePlayer : public BUIScreen
{
public:
   BUICreditsMoviePlayer( void );
   virtual ~BUICreditsMoviePlayer( void );

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

   void doHide();
   void doShow();

protected:

   void stop();
   void setupButtons();

   BUIButtonControl mButton1;
   BUIButtonControl mButton2;
   BUIButtonControl mButton3;

   BUITextFieldControl mCaptionTextField;

   BUString          mCaptionText;

   DWORD mAutoHideTime;
   bool  mShowing;
};