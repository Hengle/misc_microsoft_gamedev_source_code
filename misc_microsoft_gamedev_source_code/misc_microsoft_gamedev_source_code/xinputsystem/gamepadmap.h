//==============================================================================
// gamepadmap.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================
#pragma once

#include "xmlreader.h"

// Includes
#ifndef XBOX
   #define DIRECTINPUT_VERSION 0x800
   #include <dinput.h>
   #include <XInput.h>
#endif

#include "inputcontrolenum.h"

//==============================================================================
// BGamepadMapItem
//==============================================================================
class BGamepadMapItem
{
   public:
                              BGamepadMapItem();

      bool                    setup(BXMLNode node);

      float                   translate(XINPUT_STATE& state);
#ifndef XBOX
      float                   translate(DIJOYSTATE& state);
#endif

   protected:
      long                    mBaseControlType;
      long                    mControlIndex;
      long                    mMinVal;
      long                    mMaxVal;
      long                    mPovDir;

};

//==============================================================================
// BGamepadMap
//==============================================================================
class BGamepadMap
{
   public:
                              BGamepadMap();

      const BSimString&          getName() const { return mName; }
      bool                    setup(BXMLNode node);

      float                   translate(long control, XINPUT_STATE& state);

#ifndef XBOX
      float                   translate(long control, DIJOYSTATE& state);
#endif

   protected:
      BSimString                 mName;
      BGamepadMapItem         mItems[cControlCount];
};
