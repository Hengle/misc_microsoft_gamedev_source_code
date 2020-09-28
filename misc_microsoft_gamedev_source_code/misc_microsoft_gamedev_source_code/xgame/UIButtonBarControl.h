//============================================================================
// UIButtonBarControl.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "UIControl.h"

class BUIButtonBarControl : public BUIControl
{
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIButtonBarControl );

public:
   enum
   {
      cFlashButtonOff,
      cFlashButtonY,
      cFlashButtonA,
      cFlashButtonB,
      cFlashButtonX,
      cFlashButtonBack,
      cFlashButtonStart,
      cFlashButtonLeftTrigger,
      cFlashButtonRightTrigger,
      cFlashButtonLRTrigger,
      cFlashButtonLeftButton,
      cFlashButtonRightButton,
      cFlashButtonLRButton,
      cFlashButtonLStick,
      cFlashButtonRStick,
      cFlashButtonLRDPad,
      cFlashButtonUDDPad,
      
      cFlashButtonDpad,
      cFlashButtonlrLStick,
      cFlashButtonudLStick,
      cFlashButtonlrRStick,
      cFlashButtonudRStick,

      cFlashButtonStateCount,
   };

   BUIButtonBarControl();
   ~BUIButtonBarControl();

   void setButtonStates( uint state0 = cFlashButtonOff, 
                         uint state1 = cFlashButtonOff, 
                         uint state2 = cFlashButtonOff, 
                         uint state3 = cFlashButtonOff, 
                         uint state4 = cFlashButtonOff
                         );
   
   void setButtonState( uint index, uint state );

   void setButtonTexts( const BUString& text0 = L"",
                        const BUString& text1 = L"",
                        const BUString& text2 = L"",
                        const BUString& text3 = L"",
                        const BUString& text4 = L""
                        );

   void setButtonText( uint index, const BUString& text );

protected:
};
