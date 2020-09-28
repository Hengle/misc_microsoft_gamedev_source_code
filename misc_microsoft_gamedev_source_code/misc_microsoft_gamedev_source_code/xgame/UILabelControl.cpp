//============================================================================
// UILabelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UILabelControl.h"

//============================================================================
//============================================================================
BUILabelControl::BUILabelControl( void )
{
   mControlType.set("UILabelControl");
   mText.set("");
}

//============================================================================
//============================================================================
BUILabelControl::~BUILabelControl( void )
{
   mText.set("");
}


//============================================================================
//============================================================================
void BUILabelControl::setText(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   invokeActionScript( "setText", &value, 1);
}

