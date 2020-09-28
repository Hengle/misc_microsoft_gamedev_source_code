//============================================================================
// UITimelineCalloutControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "xcore.h"

#include "UIControl.h"
#include "UIImageViewerControl.h"
#include "UIMiniTimelineEventControl.h"

class BUITimelineEventControl : public BUIControl
{
public:
   enum Events
   {
      eTimelineEventControlEvent = UITimelineEventControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UITimelineEventControl );

   enum
   {
      cTimelineEventStateOff,
      cTimelineEventStateOn,
      cTimelineEventStateNew,
   };

   BUITimelineEventControl( void );
   virtual ~BUITimelineEventControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   bool handleUIControlEvent( BUIControlEvent& event );

   void setImage(const char* imageURL);

   const BString getImage() const { return mImageURL; }

   const float getPosition() const { return mTimelinePosition; }
   void setPosition(float position) { mTimelinePosition=position; }

   // index 
   const int getEventIndex() const { return mEventIndex; }
   void setEventIndex(int eventIndex) { mEventIndex=eventIndex; }

   void initMiniEvent(const char * path);
   BUIMiniTimelineEventControl& getMiniEvent() { return mMiniEvent; }

   void setState(int state);

protected:
   float       mTimelinePosition;
   int         mEventIndex;
   int         mState;

   BString                 mKeyframe;
   BString                 mImageURL;
   // Labels
   BUIImageViewerControl   mImage;

   BUIMiniTimelineEventControl   mMiniEvent;
};