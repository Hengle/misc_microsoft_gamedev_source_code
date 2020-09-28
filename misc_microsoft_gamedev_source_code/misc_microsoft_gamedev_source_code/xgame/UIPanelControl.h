//============================================================================
// UIPanelControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

class BUIPanelControl : public BUIControl
{
   enum Events
   {
      ePanelControlEvent = UIPanelControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIPanelControl );

public:
   BUIPanelControl( void );
   virtual ~BUIPanelControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

protected:
};