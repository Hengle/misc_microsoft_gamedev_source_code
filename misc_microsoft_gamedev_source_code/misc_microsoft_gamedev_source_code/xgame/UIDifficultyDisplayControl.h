//============================================================================
// UIDifficultyDisplayControl.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "UIControl.h"

class BUIDifficultyDisplayControl : public BUIControl
{
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIDifficultyDisplayControl );

public:
   enum BUIDifficultyIcons
   {
      cFlashButtonOff,
      cFlashButtonEasy,
      cFlashButtonNormal,
      cFlashButtonHeroic,
      cFlashButtonLegendary,

      cFlashButtonStateCount,
   };

   BUIDifficultyDisplayControl();
   ~BUIDifficultyDisplayControl();

   void setState( uint s = cFlashButtonOff);
   void setText( const BUString& t = L"");

   void update(long difficulty);

protected:

   long  mCurrentDifficulty;
};
