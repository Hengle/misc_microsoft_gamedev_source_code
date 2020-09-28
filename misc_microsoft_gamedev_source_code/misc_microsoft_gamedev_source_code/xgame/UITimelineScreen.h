//============================================================================
// UITimelineScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIListControl.h"
#include "UIScrollableTextFieldControl.h"
//#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "binkInterface.h"
#include "UITimelineCalloutControl.h"
#include "Timeline.h"
#include "UITimelineEventControl.h"

class BUITimelineScreen : public BUIScreen
{
   enum
   {
      cMoviePlayerMenuItemControl,
   };

public:
   BUITimelineScreen( void );
   virtual ~BUITimelineScreen( void );

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   virtual void update( float dt );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   void enter( void );

   void populate();

   void leave( void );

protected:
   void updateLeftRightCounters();
   void setEventData();

   void displayButtons();
   bool initTimelineEvents(BXMLNode dataNode);
   bool createEvents();
   bool createTimelineEvent(int panel, int index, const BTimelineEvent* pEvent);
   void createFlashTimelineEvent(int panel, int index, float localPostion, float globalPosition);
   void moveToIndex(int index);

   int                           mUpdateCount;

   // Controls
   BUIListControl                mEventList;

   BUITextFieldControl           mTitle;
   BUITextFieldControl           mTextCenterCount;
   BUITextFieldControl           mTextLeftCount;
   BUITextFieldControl           mTextRightCount;
   BUITimelineCalloutControl     mCallout;
   BUIButtonBarControl           mButtonBar;

   // Menu Items
   BDynamicArray<BUITimelineEventControl*>    mTimelineControls;

   BTimeline                     mTimeline;
   int                           mNumFlashPanels;
};