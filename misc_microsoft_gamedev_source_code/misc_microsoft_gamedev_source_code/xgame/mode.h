//==============================================================================
// mode.h
//
// Copyright (c) 2002-2007, Ensemble Studios
//==============================================================================
#pragma once

#include "bitvector.h"

//==============================================================================
// BMode
//==============================================================================
class BMode
{
   public:
      /*
      // Go in reverse order for flag values (from 32 down) so they don't
      // conflict with derived class flags.
      enum
      {
         cFlagMode,
      };
      */

                        BMode(long modeType);
      virtual           ~BMode();

      virtual bool      setup();
      virtual void      shutdown();

      virtual void      preEnter(BMode* lastMode);
      virtual void      enter(BMode* lastMode);
      virtual void      leave(BMode* newMode);
      virtual void      postLeave(BMode* newMode);

      virtual void      renderBegin();
      virtual void      render();
      virtual void      renderEnd();
      virtual void      update();
      virtual void      frameStart();
      virtual void      frameEnd();
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      long              getModeType() const { return mModeType; }

      long              getType() { return mModeType; }

      bool              getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void              setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

      void              renderSafeAreas();

   protected:

      UTBitVector<32>   mFlags;
      long              mModeType;

};