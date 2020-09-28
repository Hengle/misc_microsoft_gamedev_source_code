//============================================================================
// UIPanelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIPanelControl.h"

//============================================================================
//============================================================================
BUIPanelControl::BUIPanelControl( void )
{
   mControlType.set("UIPanelControl");
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUIPanelControl::~BUIPanelControl( void )
{
}

//============================================================================
//============================================================================
bool BUIPanelControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   fireUIControlEvent( command.getPtr() );
   return true;
}

