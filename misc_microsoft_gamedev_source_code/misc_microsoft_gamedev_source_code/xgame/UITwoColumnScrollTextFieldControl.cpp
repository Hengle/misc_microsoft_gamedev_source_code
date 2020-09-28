//============================================================================
// UITwoColumnScrollTextFieldControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UITwoColumnScrollTextFieldControl.h"

//============================================================================
//============================================================================
BUITwoColumnScrollTextFieldControl::BUITwoColumnScrollTextFieldControl( void ) : 
   mBounce(false)
{
   mControlType.set("UITwoColumnScrollTextFieldControl");
   mTextColumn0.set("");
   mTextColumn1.set("");
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUITwoColumnScrollTextFieldControl::~BUITwoColumnScrollTextFieldControl( void )
{
}

//============================================================================
//============================================================================
bool BUITwoColumnScrollTextFieldControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="scrollUp")
   {
      if (event == cInputEventCommandStart)
         startScrollUp();
      else if (event == cInputEventCommandStop)
         stopScroll();

      return true;
   }
   else if (command=="scrollDown")
   {
      if (event == cInputEventCommandStart)
         startScrollDown();
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
void BUITwoColumnScrollTextFieldControl::setText(const BUString& textColumn0, const BUString& textColumn1)
{
   mTextColumn0 = textColumn0;
   mTextColumn1 = textColumn1;

   GFxValue value[2];
   value[0].SetStringW(textColumn0.getPtr());
   value[1].SetStringW(textColumn1.getPtr());
   invokeActionScript( "setText", value, 2);
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::appendTextColumn0(const BUString& textColumn)
{
   mTextColumn0.append(textColumn);

   GFxValue value;
   value.SetStringW(textColumn.getPtr());
   invokeActionScript( "appendTextColumn0", &value, 1);
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::appendTextColumn1(const BUString& textColumn)
{
   mTextColumn1.append(textColumn);

   GFxValue value;
   value.SetStringW(textColumn.getPtr());   
   invokeActionScript( "appendTextColumn1", &value, 1);
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::updateTextBegin()
{
   invokeActionScript( "clearTextBuffers", NULL, 0);
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::updateTextEnd()
{
   invokeActionScript( "init", NULL, 0);
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::startScrollUp()
{
   invokeActionScript("startScrollUp");
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::startScrollDown()
{
   invokeActionScript("startScrollDown");
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::stopScroll()
{
   invokeActionScript("stopScroll");
}

//============================================================================
//============================================================================
void BUITwoColumnScrollTextFieldControl::setBounce(bool bBounce)
{
   mBounce = bBounce;

   GFxValue value;
   value.SetBoolean(mBounce);

   invokeActionScript("startScrollUp", &value, 1);
}
