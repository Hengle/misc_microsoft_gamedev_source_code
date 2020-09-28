//==============================================================================
// gamepad.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#pragma once

// Includes
#ifndef XBOX
   #define DIRECTINPUT_VERSION 0x800
   #include <dinput.h>
   #include <XInput.h>
#endif

#include "inputcontrolenum.h"
#include "string\stringTable.h"

// Forward declarations
class BGamepadMap;
class BInputData;
class BXMLNode;
class IInputEventHandler;


//==============================================================================
// BGamepadRumble
//==============================================================================
class BGamepadRumble
{
   public:
                           BGamepadRumble();

      int                  mID;
      int                  mLeftRumbleType;
      float                mLeftStrength;
      int                  mRightRumbleType;
      float                mRightStrength;
      DWORD                mStartTime;
      float                mDuration;
      int16                mPatternIndex;
      uint8                mPatternSequence;
      uint8                mPatternAction;
      bool                 mLoop:1;
      bool                 mPlayed:1;
};
typedef BSmallDynamicSimArray<BGamepadRumble>         BGamepadRumbleSequence;
typedef BSmallDynamicSimArray<BGamepadRumbleSequence> BGamepadRumblePattern;

//==============================================================================
// BGamepad
//==============================================================================
class BGamepad
{
   public:
                           BGamepad();
                           ~BGamepad();

      enum
      {
         cRumbleTypeNone,
         cRumbleTypeFixed,
         cRumbleTypeSineWave,
         cRumbleTypeIntervalBurst,
         cRumbleTypeRandomNoise,
         cRumbleTypeIncline,
         cRumbleTypeDecline,
         cRumbleTypeBumpLRL,
         cNumberRumbleTypes
      };

      enum
      {
         cRumbleMotorBoth,
         cRumbleMotorLeft,
         cRumbleMotorRight,
      };

      bool                 setup(long port, BGamepadMap* map);

#ifndef XBOX
      bool                 createDevice(REFGUID guid, BGamepadMap* map);
#endif

      void                 resetDevice();
      void                 close();
      void                 update(BInputData* pInputData, IInputEventHandler* pInputEventHandler, bool doUpdateRumble=true);
      
      bool                 isActive() const { return mActive; }
      bool                 isControlActive(long control) const;
      float                getControlValue(long control) const;
      bool                 wasActivatedThisUpdate(long control);

      float                getStickLX() const { return mStickLX; }
      float                getStickLY() const { return mStickLY; }
      float                getStickRX() const { return mStickRX; }
      float                getStickRY() const { return mStickRY; }
      float                getDpadX() const { return mDpadX; }
      float                getDpadY() const { return mDpadY; }

#ifndef XBOX
      const DIJOYSTATE&    getInputState() const { return mInputState; }
#endif
      const XINPUT_STATE&  getInputState2() const { return mInputState2; }

      static int           getRumbleType(const char* pName);
      static int           getRumbleMotor(const char* pName);
      static int           getRumblePattern(const char* pName) { long index=0; if (mRumblePatternTable.find(pName, &index)) return index; else return -1; }
      static const char*   getRumbleTypeName(int rumbleType);
      static const char*   getRumbleMotorName(int rumbleMotor);
      static const char*   getRumblePatternName(int patternType);
      int                  playRumblePattern(int patternIndex, bool loop);
      int                  playRumble(int leftRumbleType, float leftStrength, int rightRumbleType, float rightStrength, float duration, bool loop);
      void                 stopRumble(int rumbleID);
      void                 resetRumble();
      static void          loadRumblePattern(BXMLNode& rootNode);

      void                 enableRumble( bool v = true ) { mRumbleEnabled = v; }
      void                 disableRumble() { enableRumble( false ); }

   protected:
      void                 updateRumble();
      float                getRumblePatternValue(long type, float strength, float scale, int rumbleMotor);

      long                 mPort;

      BGamepadMap*         mMap;

      bool                 mActive;

      bool                 mRumbleEnabled;

#ifndef XBOX
      IDirectInputDevice8* mDevice;
      DIJOYSTATE           mInputState;
#endif
      XINPUT_STATE         mInputState2;

      float                mStickLX;
      float                mStickLY;
      float                mStickRX;
      float                mStickRY;
      float                mDpadX;
      float                mDpadY;

      int                                    mNextRumbleID;
      BSmallDynamicSimArray<BGamepadRumble>  mRumbleList;
      float                                  mLeftRumbleSpeed;
      float                                  mRightRumbleSpeed;

      static BSmallDynamicSimArray<BGamepadRumblePattern>   mRumblePatternList;
      static BStringTableLong                               mRumblePatternTable;

      bool                 mControlActive[cGamepadControlCount];
      bool                 mLastControlActive[cGamepadControlCount];
      float                mControlValue[cGamepadControlCount];

};
