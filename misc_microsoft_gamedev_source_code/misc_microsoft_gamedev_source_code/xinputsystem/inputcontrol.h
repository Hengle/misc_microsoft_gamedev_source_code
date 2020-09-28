//==============================================================================
// inputcontrol.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "inputsystem.h"
#include "bitarray.h"
#include "xmlreader.h"

// Forward declarations
class BInputEventDetail;
class IInputControlEventHandler;

//==============================================================================
// BInputControl
//==============================================================================
class BInputControl
{
   public:
      enum
      {
         cFlagRepeat,
         cFlagOverrideX,
         cFlagOverrideY,
         cFlagOverrideAnalog,
         cFlagSwapXY,
         cFlagOneWay,
         cFlagDigital,
         cFlagNoStartEvent,
         cFlagNoStopEvent,
         cFlagDoubleEvent,
         cFlagCount
      };

      enum
      {
         cMaxModifiers=4
      };

                                       BInputControl();
                                       ~BInputControl();

      bool                             setup(BXMLNode node, IInputControlEventHandler* handler);
      void                             reset();

      bool                             handleInput(long port, long event, long controlType, const BInputEventDetail& detail);
      const BSimString&                getCommand() const { return mInputCommand; }
      const BSimString&                getSoundEvent() const { return mSoundEvent; }

   protected:
      bool                             checkControl(long port, long controlType);
      void                             eventStart(long port, const BInputEventDetail& detail);
      void                             eventRepeat(long port, const BInputEventDetail& detail);
      void                             eventStop(long port, const BInputEventDetail& detail);
      void                             fixupDetail(const BInputEventDetail& in, BInputEventDetail& out);

      bool                             getFlag(long n) const { return(mFlags.isBitSet(n) > 0); }
      void                             setFlag(long n, bool v) { if (v) mFlags.setBit(n); else mFlags.clearBit(n); }


      IInputControlEventHandler*       mpInputControlEventHandler;
      long                             mControlType;
      BSimString                       mInputCommand;
      BSimString                       mSoundEvent;
      long                             mModifierCount;
      long                             mModifiers[cMaxModifiers];
      DWORD                            mRepeatDelay;
      DWORD                            mRepeatRate;
      float                            mX;
      float                            mY;
      float                            mAnalog;
      DWORD                            mRepeatTime[BInputSystem::cMaxPorts];
      float                            mThreshold;
      float                            mDigitalX[BInputSystem::cMaxPorts];
      float                            mDigitalY[BInputSystem::cMaxPorts];
      DWORD                            mStartTime[BInputSystem::cMaxPorts];
      BBitArray                        mFlags;

};
