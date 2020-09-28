//============================================================================
// BUISkullPicker.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIListControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "UITextFieldControl.h"

class BUICheckedMenuItemControl;
class BUser;

//============================================================================
//============================================================================
class BUISkullPicker : public BUIScreen
{
   public:

      enum
      {
         cSkullPickerList,
         cSkullPickerControlCount
      };


      BUISkullPicker( void );
      virtual ~BUISkullPicker( void );

      bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
      virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
      virtual bool handleUIControlEvent( BUIControlEvent& event );

      virtual bool init( const char* filename, const char* datafile );
      virtual bool init(BXMLNode dataNode);
      virtual void enter( void );


   protected:

      enum
      {
         cMaxMenuItems = 20
      };

      void showSkullMenu();
      bool addSkulls();
      void initMenuItem(int index);
      void initImageViewer();


      void updateSelection(BUser* pUser);
      bool displayButtons();

      BUITextFieldControl           mTitle;
      BUITextFieldControl           mDescriptionLabel;
      BUITextFieldControl           mReminderLabel;
      BUITextFieldControl           mScoreLabel;
      BUIListControl                mSkullList;      
      BUIButtonBarControl           mButtonBar;
      BDynamicArray<BUICheckedMenuItemControl*> mSkullControls;

      BUIScrollableTextFieldControl mHelpText;
      BUIImageViewerControl         mImageViewer;

      BDynamicArray<int>            mSkullMapping;

      // [10/8/2008 xemu] note this is an actual skull index and NOT a menu index! 
      int                           mCurrentSkull;

      bool                          mInitialized;
};