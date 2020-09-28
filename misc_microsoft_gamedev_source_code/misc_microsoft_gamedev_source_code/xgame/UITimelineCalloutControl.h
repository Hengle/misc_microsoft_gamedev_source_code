//============================================================================
// UITimelineCalloutControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "xcore.h"

#include "UIControl.h"
//#include "UIImageViewerControl.h"
#include "UITextFieldControl.h"


class BPlayer;

class BUITimelineCalloutControl : public BUIControl
{
public:
   enum Events
   {
      eTimelineCalloutControlEvent = UITimelineCalloutControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UITimelineCalloutControl );

   BUITimelineCalloutControl( void );
   virtual ~BUITimelineCalloutControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   bool handleUIControlEvent( BUIControlEvent& event );

   void setTitle(const BUString& title);
   void setDescription(const BUString& description);
//   void setImage(const char* imageURL);

protected:

   void startScrollUp();
   void startScrollDown();
   void stopScroll();

   BUString                mDescription;

   // Labels
   BUITextFieldControl     mTitle;
//   BUIImageViewerControl   mImage;
};