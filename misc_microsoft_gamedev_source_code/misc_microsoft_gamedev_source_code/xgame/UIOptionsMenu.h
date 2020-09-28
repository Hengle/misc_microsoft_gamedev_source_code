//============================================================================
// UIOptionsMenu.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "xcore.h"
#include "UIScreen.h"
#include "UITabControl.h"
#include "UIMenuItemControl.h"
#include "UIOptionsMenuItemControl.h"
#include "UIImageViewerControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UIButtonBarControl.h"
#include "UIGlobals.h"
#include "UITextFieldControl.h"
#include "UIDifficultyDisplayControl.h"

class BUIOptionsMenu : public BUIScreen, public BUIGlobals::yornHandlerInterface
{
public:
   BUIOptionsMenu( void );
   virtual ~BUIOptionsMenu( void );

   // BFlashScene
   virtual bool init( BXMLNode root );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail );

   virtual void enter( void );
   virtual void update( float dt );

   virtual void setUser( BUser* pUser );

   //yornHandlerInterface
   virtual void yornResult(uint result, DWORD userContext, int port);

   virtual void setTab( int index );

protected:

   virtual void addOptionPane( const char* name );
   virtual void updateDescription( void );

   BUser* mpUser;
   BUITabControl mTabs;
   BDynamicArray<BString> mTabImages;

   enum EControlID { eTabControlID, eOptionListID };
   
   enum EYorNContext { eResetOneOption, eResetOnePane, eResetAllOptions, eResetHints };

   // Array of allocated controls
   BDynamicArray<BUIControl*> mControls;

   BUITextFieldControl mTitle;
   BUIImageViewerControl mDetailImage;
   BUITextFieldControl mDescriptionLabel;
   BUIScrollableTextFieldControl mDetailText;
   BUIButtonBarControl mButtonBar;
   BUIDifficultyDisplayControl mDifficulty;
   BUITextFieldControl mTimeLabel;
   long mSeconds;
};