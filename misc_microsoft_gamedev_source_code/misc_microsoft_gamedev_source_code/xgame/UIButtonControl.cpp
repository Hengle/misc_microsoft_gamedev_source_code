//============================================================================
// UIMenuItemControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIButtonControl.h"

//============================================================================
//============================================================================
BUIButtonControl::BUIButtonControl( void )
{
   mControlType.set("UIButtonControl");
   mText.set("");
   mImage.set("");
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUIButtonControl::~BUIButtonControl( void )
{
}

//============================================================================
//============================================================================
bool BUIButtonControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="click")
      onPress();

   fireUIControlEvent(ePress);
   return true;
}

//============================================================================
//============================================================================
void BUIButtonControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}

//============================================================================
//============================================================================
void BUIButtonControl::setImage(const char* pImage)
{
   mImage.set(pImage);

   GFxValue value;
   value.SetString(mImage.getPtr());

   invokeActionScript( "setImage", &value, 1);
}

//============================================================================
//============================================================================
void BUIButtonControl::onPress()
{
   invokeActionScript("onPress");
}

