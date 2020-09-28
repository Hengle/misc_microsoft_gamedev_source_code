//============================================================================
// UIControllerScreen.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "xcore.h"
#include "UIScreen.h"
#include "UITextFieldControl.h"
#include "UIButtonControl.h"
#include "UIButtonBarControl.h"
#include "UIDifficultyDisplayControl.h"

class BUIControllerScreen : public BUIScreen
{
public:
   BUIControllerScreen( void );
   ~BUIControllerScreen( void );

   // BFlashScene
   virtual bool init( BXMLNode root );
   virtual void deinit( void );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail );
   virtual void enter( void );
   virtual void update( float dt );
   virtual void render( void );
   virtual void leave( void );

   virtual bool getVisible( void );

   // virtual void handleUIScreenResult( BUIScreen* pScreen, long result );

protected:

   void addMenuItem( int index, int id, const BUString& text );
   void populateMenu( void );
   void updateGameTime( bool force = false );
   void populateDifficulty();

   BUITextFieldControl mTitle;
   BUITextFieldControl mGameTime;
   BUIDifficultyDisplayControl mDifficulty;

   // Controller Labels
   BUITextFieldControl mLeftButton;
   BUITextFieldControl mRightButton;
   BUITextFieldControl mLeftStick;
   BUITextFieldControl mRightStick;

   BUITextFieldControl mLeftTrigger;
   BUITextFieldControl mRightTrigger;

   BUITextFieldControl mBackButton;
   BUITextFieldControl mStartButton;

   BUITextFieldControl mDPadUp;
   BUITextFieldControl mDPadDown;
   BUITextFieldControl mDPadLeft;
   BUITextFieldControl mDPadRight;

   BUITextFieldControl mAButton;
   BUITextFieldControl mBButton;
   BUITextFieldControl mXButton;
   BUITextFieldControl mYButton;

   BUIButtonBarControl mButtonBar;

   long mSeconds;
};
