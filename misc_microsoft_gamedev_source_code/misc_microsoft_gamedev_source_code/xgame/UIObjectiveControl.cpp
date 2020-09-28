//============================================================================
// UIObjectiveControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIObjectiveControl.h"

//============================================================================
//============================================================================
BUIObjectiveControl::BUIObjectiveControl( void )
{
   mControlType.set("UIMenuItemControl");
   mText.set("");
}

//============================================================================
//============================================================================
BUIObjectiveControl::~BUIObjectiveControl( void )
{
   mText.set("");
}

//============================================================================
//============================================================================
bool BUIObjectiveControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   fireUIControlEvent(command.getPtr());
   return true;
}

//============================================================================
//============================================================================
void BUIObjectiveControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}


//============================================================================
//============================================================================
void BUIObjectiveControl::setType(int type)
{
   mType = type;

   GFxValue value;
   value.SetNumber(mType);

   invokeActionScript( "setType", &value, 1);
}

//============================================================================
//============================================================================
void BUIObjectiveControl::setStatus(int status)
{
   mStatus = status;

   GFxValue value;
   value.SetNumber(mStatus);

   invokeActionScript( "setStatus", &value, 1);
}
