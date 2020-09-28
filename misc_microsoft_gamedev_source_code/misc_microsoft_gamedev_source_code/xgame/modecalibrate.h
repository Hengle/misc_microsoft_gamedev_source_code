//==============================================================================
// modecalibrate.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mode.h"
#include "ui.h"
#include "uilist.h"

//==============================================================================
// BModeCalibrate
//==============================================================================
class BModeCalibrate : public BMode
{
public:
   enum
   {
      cStateMain,
      cStateExit,
   };

   BModeCalibrate(long modeType);
   virtual           ~BModeCalibrate();

   virtual bool      setup();
   virtual void      shutdown();

   virtual void      preEnter(BMode* lastMode);
   virtual void      enter(BMode* lastMode);
   virtual void      leave(BMode* newMode);

   virtual void      renderBegin();
   virtual void      render();
   virtual void      renderEnd();
   virtual void      update();
   virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

   void              setNextState(long state) { mNextState=state; }

protected:
   long              mState;
   long              mNextState;
   
   BUIList           mList;
   long              mLastMainItem;
   
   BManagedTextureHandle    mBackgroundHandle;
   
   void              saveGamma(void);
};
