//============================================================================
// UIScrollableCalloutControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIScrollableCalloutControl.h"

//============================================================================
//============================================================================
BUIScrollableCalloutControl::BUIScrollableCalloutControl( void ) : 
   mBounce(false)
{
   mControlType.set("UIScrollableCalloutControl");
   mText.set("");
}

//============================================================================
//============================================================================
BUIScrollableCalloutControl::~BUIScrollableCalloutControl( void )
{
}

//============================================================================
//============================================================================
bool BUIScrollableCalloutControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
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
   else if (command=="cancel")
      fireUIControlEvent( eCancel );

   return false;
}

//============================================================================
//============================================================================
void BUIScrollableCalloutControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}

//============================================================================
//============================================================================
void BUIScrollableCalloutControl::setTitle(const BUString& text)
{
   mTitle = text;

   GFxValue value;
   value.SetStringW(mTitle.getPtr());

   invokeActionScript( "setTitle", &value, 1);
}


//============================================================================
//============================================================================
void BUIScrollableCalloutControl::startScrollUp()
{
   invokeActionScript("startScrollUp");
}

//============================================================================
//============================================================================
void BUIScrollableCalloutControl::startScrollDown()
{
   invokeActionScript("startScrollDown");
}

//============================================================================
//============================================================================
void BUIScrollableCalloutControl::stopScroll()
{
   invokeActionScript("stopScroll");
}

//============================================================================
//============================================================================
void BUIScrollableCalloutControl::setBounce(bool bBounce)
{
   mBounce = bBounce;

   GFxValue value;
   value.SetBoolean(mBounce);

   invokeActionScript("startScrollUp", &value, 1);
}
