//============================================================================
// UIMiniTimelineCalloutControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "xcore.h"

#include "UIControl.h"
#include "UIImageViewerControl.h"

class BUIMiniTimelineEventControl : public BUIControl
{
public:
   enum Events
   {
      eMiniTimelineEventControlEvent = UIMiniTimelineEventControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIMiniTimelineEventControl );

   enum 
   {
      cMiniTimelineStateOff,
      cMiniTimelineStateUnlocked,
      cMiniTimelineStateNew,
   };

   BUIMiniTimelineEventControl( void );
   virtual ~BUIMiniTimelineEventControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   void setState(int state);

protected:

   int   mState;
   BString mKeyframe;
};