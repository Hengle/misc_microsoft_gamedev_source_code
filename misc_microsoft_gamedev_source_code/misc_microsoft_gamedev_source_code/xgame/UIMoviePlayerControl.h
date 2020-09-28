//============================================================================
// UIListControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "UIListControl.h"
#include "xcore.h"

class BXMLReader;
class BUIButtonControl;

class BUIMoviePlayerControl : public BUIListControl
{
public:
   enum Events
   {
      eMoviePlayerControlEvent = UIMoviePlayerControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIMoviePlayerControl );

public:
   enum
   {
      eButtonPlayPause,
      eButtonSkipBack,
      eButtonRewind,
      eButtonStop,
      eButtonFastForward,
      eButtonSkipForward,
      eButtonSubtitles,
      eButtonInfo,

      eButtonCount
   };

   BUIMoviePlayerControl( void );
   virtual ~BUIMoviePlayerControl( void );

   //----- Override of init
   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData = NULL );


   //----- IInputControlEventHandler 
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

   // IUIControlEventHandler
   bool handleUIControlEvent( BUIControlEvent& event );

protected:
   bool addButtons();
   bool addButton(const WCHAR* buttonText, const char* buttonImage, int index, int controlID);

   BDynamicArray<BUIButtonControl*> mControlButtons;

   bool  mPaused;
};