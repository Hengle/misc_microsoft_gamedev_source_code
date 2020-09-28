//============================================================================
// UIMenuItemControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIMenuItemControl.h"

//============================================================================
//============================================================================
BUIMenuItemControl::BUIMenuItemControl( void ) :
   mData(-1)
{
   mControlType.set("UIMenuItemControl");
   mText.set(L"");
   mHelpText.set(L"");
}

//============================================================================
//============================================================================
BUIMenuItemControl::~BUIMenuItemControl( void )
{
}

//============================================================================
//============================================================================
bool BUIMenuItemControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   fireUIControlEvent( command.getPtr() );
   return true;
}

//============================================================================
//============================================================================
void BUIMenuItemControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}
