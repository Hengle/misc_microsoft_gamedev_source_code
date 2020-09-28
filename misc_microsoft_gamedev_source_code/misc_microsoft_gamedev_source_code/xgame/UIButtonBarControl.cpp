//============================================================================
// UIButtonBar2.cpp
// Ensemble Studios (c) 2007
//============================================================================

#include "common.h"
#include "UIButtonBarControl.h"


//==============================================================================
//==============================================================================
BUIButtonBarControl::BUIButtonBarControl()
{
}

//==============================================================================
//==============================================================================
BUIButtonBarControl::~BUIButtonBarControl()
{

}


//==============================================================================
// BUIButtonBarControl::setButtonTexts
//==============================================================================
void BUIButtonBarControl::setButtonTexts(const BUString& text0,
                                         const BUString& text1,
                                         const BUString& text2,
                                         const BUString& text3,
                                         const BUString& text4
                                         )
{
   GFxValue values[5];

   values[0].SetStringW(text0.getPtr());
   values[1].SetStringW(text1.getPtr());
   values[2].SetStringW(text2.getPtr());
   values[3].SetStringW(text3.getPtr());
   values[4].SetStringW(text4.getPtr());

   invokeActionScript("setButtonTexts", values, 5);
}

//==============================================================================
// BUIButtonBarControl::setButtonText
//==============================================================================
void BUIButtonBarControl::setButtonText( uint index, const BUString& text )
{
   GFxValue values[2];
   values[0].SetNumber( index + 1 );
   values[1].SetStringW( text );
   invokeActionScript( "setButtonText", values, 2 );
}

//==============================================================================
// BUIPartyRoom::setButtonStates
//==============================================================================
void BUIButtonBarControl::setButtonStates(uint state0, uint state1, uint state2, uint state3, uint state4)
{
   GFxValue values[5];
   values[0].SetNumber(state0);
   values[1].SetNumber(state1);
   values[2].SetNumber(state2);
   values[3].SetNumber(state3);
   values[4].SetNumber(state4);

   invokeActionScript("setButtonStates", values, 5);
}

//==============================================================================
// BUIButtonBarControl::setButtonState
//==============================================================================
void BUIButtonBarControl::setButtonState( uint index, uint state )
{
   // only 5 buttons
   if (index > 4)
      return;

   GFxValue values[2];
   values[0].SetNumber( index + 1 );
   values[1].SetNumber( state );
   invokeActionScript( "setButtonState", values, 2 );
}