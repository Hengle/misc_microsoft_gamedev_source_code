//============================================================================
// UILabelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIGridCellControl.h"

//============================================================================
//============================================================================
BUIGridCellControl::BUIGridCellControl( void ) : 
   mHasDetail(false)
{
   mControlType.set("UIGridCellControl");
   mText.set("");
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUIGridCellControl::~BUIGridCellControl( void )
{
   mText.set("");
}


//============================================================================
//============================================================================
void BUIGridCellControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}

//============================================================================
//============================================================================
bool BUIGridCellControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="click")
      onPress();

   fireUIControlEvent(ePress);
   return true;
}

//============================================================================
//============================================================================
void BUIGridCellControl::onPress()
{
   invokeActionScript("onPress");
}

//============================================================================
//============================================================================
void BUIGridCellControl::setDetailIndicator(bool enable)
{
   if (mHasDetail == enable)
      return;

   mHasDetail = enable;

   GFxValue value;
   value.SetBoolean(mHasDetail);

   invokeActionScript( "enableDetailIndicator", &value, 1);
}
