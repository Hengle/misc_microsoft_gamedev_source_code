//============================================================================
// UIGameMenu.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "UIScreen.h"
#include "UIGamerTagControl.h"
#include "UIImageViewerControl.h"
#include "UITextFieldControl.h"
#include "UIListControl.h"
#include "UIButtonControl.h"
#include "xcore.h"
#include "UIButtonBarControl.h"
#include "UIGlobals.h"
#include "UIDifficultyDisplayControl.h"
#include "AsyncTaskManager.h"


class BUser;
class BPlayer;
class BUIOptionsMenu;
class BUISkullPicker;
class BUIControllerScreen;

class BUIGameMenu : public BUIScreen, public BUIGlobals::yornHandlerInterface, public BAsyncNotify
{
public:
   BUIGameMenu( void );
   ~BUIGameMenu( void );

   // BFlashScene
   virtual bool init( BXMLNode root );
   virtual void deinit( void );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail );
   virtual void enter( void );
   virtual void refreshScreen(void);
   virtual void update( float dt );
   virtual void render( void );
   virtual void leave( void );

   virtual bool getVisible( void );

   //yornHandlerInterface
   virtual void yornResult(uint result, DWORD userContext, int port);

   virtual void handleUIScreenResult( BUIScreen* pScreen, long result );

   virtual void notify( DWORD eventID, void* pTask );

   enum EResult { eResult_Pause, eResult_Resume, eResult_Resign, eResult_Restart, eResult_Options };

protected:

   void addMenuItem( int index, int id, const BUString& text);
   void populateMenu( void );

   void updateGameTime( bool force = false );
   void populateDifficulty();

   void playCampaign(int startingNode = -1, bool useSaveIfAvailable = false);

   BUser* mpUser;

   BUITextFieldControl mTitleLabel;
   BUITextFieldControl mGameTime;
   BUIDifficultyDisplayControl mDifficulty;

   BUITextFieldControl mPlayerListLabel;

   // Controller Labels
   BUITextFieldControl mLBLabel;
   BUITextFieldControl mRBLabel;
   BUITextFieldControl mLSLabel;
   BUITextFieldControl mALabel;
   BUITextFieldControl mBLabel;
   BUITextFieldControl mXLabel;
   BUITextFieldControl mYLabel;

   BUITextFieldControl mPausedLabel;

   BUIListControl mMenu;
   BUIButtonControl mItem0;
   BUIButtonControl mItem1;
   BUIButtonControl mItem2;
   BUIButtonControl mItem3;
   BUIButtonControl mItem4;
   BUIButtonControl mItem5;
   BUIButtonControl mItem6;


   static const int NUM_ITEMS = 7;
   BUIButtonControl* mItems[NUM_ITEMS];
   
   enum EMenuItemIDs { ePause, eResume, eResign, eSave, eLoad, eRestart, eOptions, eControllerScreen, eSkullPicker, eButtonHelp, eResourceBoost, eFogOfWar, eUnlockView, eSaveAndQuit, eDevSave, eDevLoad, eLoadCorrupt };

   BUIButtonBarControl mButtonBar;

   int mPauseItemIndex;
   bool mbPaused;
   bool mbSelectingDevice;

   BUIOptionsMenu* mpOptionsMenu;
   BUISkullPicker* mpSkullPicker;
   BUIControllerScreen* mpControllerScreen;

   int  mSaveGameNode;
   long mSeconds;
};
