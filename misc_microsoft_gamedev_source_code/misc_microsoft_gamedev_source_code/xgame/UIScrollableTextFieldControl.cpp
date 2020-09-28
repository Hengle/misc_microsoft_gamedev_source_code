//============================================================================
// UIScrollableTextFieldControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIScrollableTextFieldControl.h"

//============================================================================
//============================================================================
BUIScrollableTextFieldControl::BUIScrollableTextFieldControl( void ) : 
   mBounce(false)
{
   mControlType.set("UIScrollableTextFieldControl");
   mText.set("");
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUIScrollableTextFieldControl::~BUIScrollableTextFieldControl( void )
{
}

//============================================================================
//============================================================================
bool BUIScrollableTextFieldControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="scrollUp")
   {
      if (event == cInputEventCommandStart)
         startScrollDown();      // reversed to be consistent with the rest of the UI
      else if (event == cInputEventCommandStop)
         stopScroll();

      return true;
   }
   else if (command=="scrollDown")
   {
      if (event == cInputEventCommandStart)
         startScrollUp();        // reversed to be consistent with the rest of the UI
      else if (event == cInputEventCommandStop)
         stopScroll();
      return true;
   }
   else
      fireUIControlEvent(command.getPtr());

   return false;
}

//============================================================================
//============================================================================
void BUIScrollableTextFieldControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}


//============================================================================
//============================================================================
void BUIScrollableTextFieldControl::startScrollUp()
{
   invokeActionScript("startScrollUp");
}

//============================================================================
//============================================================================
void BUIScrollableTextFieldControl::startScrollDown()
{
   invokeActionScript("startScrollDown");
}

//============================================================================
//============================================================================
void BUIScrollableTextFieldControl::stopScroll()
{
   invokeActionScript("stopScroll");
}

//============================================================================
//============================================================================
void BUIScrollableTextFieldControl::setBounce(bool bBounce)
{
   mBounce = bBounce;

   GFxValue value;
   value.SetBoolean(mBounce);

   invokeActionScript("startScrollUp", &value, 1);
}
