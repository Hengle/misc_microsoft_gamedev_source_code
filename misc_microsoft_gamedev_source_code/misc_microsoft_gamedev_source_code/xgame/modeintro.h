//==============================================================================
// modeintro.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mode.h"

//==============================================================================
// BModeIntro
//==============================================================================
class BModeIntro : public BMode
{
   public:
      enum
      {
         cFlagModeUnused,
      };

                        BModeIntro(long modeType);
      virtual           ~BModeIntro();

      virtual bool      setup();

      virtual void      preEnter(BMode* lastMode);
      virtual void      enter(BMode* lastMode);
      virtual void      leave(BMode* newMode);

      virtual void      renderBegin();
      virtual void      render();
      virtual void      renderEnd();
      virtual void      update();
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

   protected:
      float             mX;
      float             mY;

};