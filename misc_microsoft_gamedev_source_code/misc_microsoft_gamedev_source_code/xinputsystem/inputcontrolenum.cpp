//==============================================================================
// inputcontrolenum.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "inputcontrolenum.h"

// Base control names
const BCHAR_T* gBaseControlNames[cBaseControlCount]=
{
   B("X"),
   B("Y"),
   B("Z"),
   B("RX"),
   B("RY"),
   B("RZ"),
   B("POV"),
   B("Slider"),
   B("Button"),
};

// POV direction names
const BCHAR_T* gPovDirNames[cPovDirCount]=
{
   B("Up"),
   B("Right"),
   B("Down"),
   B("Left"),
};

// Control names
const BCHAR_T* gControlNames[] =
{
   B("StickLeftUp"),
   B("StickLeftDown"),
   B("StickLeftLeft"),
   B("StickLeftRight"),
   B("StickRightUp"),
   B("StickRightDown"),
   B("StickRightLeft"),
   B("StickRightRight"),
   B("DpadUp"),
   B("DpadDown"),
   B("DpadLeft"),
   B("DpadRight"),
   B("ButtonA"),
   B("ButtonB"),
   B("ButtonX"),
   B("ButtonY"),
   B("ButtonStart"),
   B("ButtonBack"),
   B("ButtonShoulderRight"),
   B("ButtonShoulderLeft"),
   B("ButtonThumbLeft"),
   B("ButtonThumbRight"),
   B("TriggerLeft"),
   B("TriggerRight"),

   B("StickLeft"),
   B("StickRight"),
   B("Dpad"),
   B("Triggers"),

   B("GamepadInsert"),
   B("GamepadRemove"),

   B("KeyBackSpace"),
   B("KeyTab"),
   B("KeyEnter"),
   B("KeyShift"),
   B("KeyCtrl"),
   B("KeyAlt"),
   B("KeyPause"),
   B("KeyEscape"),
   B("KeySpace"),
   B("KeyPageUp"),
   B("KeyPageDown"),
   B("KeyEnd"),
   B("KeyHome"),
   B("KeyLeft"),
   B("KeyUp"),
   B("KeyRight"),
   B("KeyDown"),
   B("KeyPrtSc"),
   B("KeyInsert"),
   B("KeyDelete"),
   B("Key0"),
   B("Key1"),
   B("Key2"),
   B("Key3"),
   B("Key4"),
   B("Key5"),
   B("Key6"),
   B("Key7"),
   B("Key8"),
   B("Key9"),
   B("KeyA"),
   B("KeyB"),
   B("KeyC"),
   B("KeyD"),
   B("KeyE"),
   B("KeyF"),
   B("KeyG"),
   B("KeyH"),
   B("KeyI"),
   B("KeyJ"),
   B("KeyK"),
   B("KeyL"),
   B("KeyM"),
   B("KeyN"),
   B("KeyO"),
   B("KeyP"),
   B("KeyQ"),
   B("KeyR"),
   B("KeyS"),
   B("KeyT"),
   B("KeyU"),
   B("KeyV"),
   B("KeyW"),
   B("KeyX"),
   B("KeyY"),
   B("KeyZ"),
   B("KeyAccent"),
   B("KeyMultiply"),
   B("KeyAdd"),
   B("KeySeparator"),
   B("KeySubtract"),
   B("KeyDecimal"),
   B("KeyDivide"),
   B("KeyF1"),
   B("KeyF2"),
   B("KeyF3"),
   B("KeyF4"),
   B("KeyF5"),
   B("KeyF6"),
   B("KeyF7"),
   B("KeyF8"),
   B("KeyF9"),
   B("KeyF10"),
   B("KeyF11"),
   B("KeyF12"),
   B("KeyF16"),
   B("KeyShiftLeft"),
   B("KeyShiftRight"),
   B("KeyCtrlLeft"),
   B("KeyCtrlRight"),
   B("KeyAltLeft"),
   B("KeyAltRight"),
   B("KeyGreenModifier"),
   B("KeyOrangeModifier")
};

//==============================================================================
// lookupBaseControl
//==============================================================================
long lookupBaseControl(const BCHAR_T* name)
{
   for(long i=0; i<cBaseControlCount; i++)
   {
      if(strCompare(gBaseControlNames[i], 100, name, 100)==0)
         return i;
   }
   return -1;
}

//==============================================================================
// lookupPovDir
//==============================================================================
long lookupPovDir(const BCHAR_T* name)
{
   for(long i=0; i<cPovDirCount; i++)
   {
      if(strCompare(gPovDirNames[i], 100, name, 100)==0)
         return i;
   }
   return -1;
}

//==============================================================================
// lookupControl
//==============================================================================
long lookupControl(const BCHAR_T *name)
{
   BCOMPILETIMEASSERT(sizeof(gControlNames)/sizeof(gControlNames[0]) == cControlCount);
   
   // Look through list for a match.
   for(long i=0; i<cControlCount; i++)
   {
      if(strCompare(gControlNames[i], 100, name, 100)==0)
         return(i);
   }

   // Didn't find a match, so return -1.
   return(-1);
}

//==============================================================================
// getDoubleClickFromControlType
//==============================================================================
long getDoubleClickFromControlType( long controlType )
{
   long result = -1;

   switch( controlType )
   {
      case cButtonShoulderLeft:
         result = cButtonShoulderLeftDC;
         break;

      case cButtonShoulderRight:
         result = cButtonShoulderRightDC;
         break;

      case cDpadUp:
         result = cDPadUpDC;
         break;

      case cDpadRight:
         result = cDPadRightDC;
         break;

      case cDpadDown:
         result = cDPadRightDC;
         break;

      case cDpadLeft:
         result = cDPadLeftDC;
         break;

      case cButtonY:
         result = cButtonYDC;
         break;

      case cButtonB:
         result = cButtonBDC;
         break;

      case cButtonA:
         result = cButtonADC;
         break;

      case cButtonX:
         result = cButtonXDC;
         break;

      case cButtonThumbLeft:
         result = cButtonThumbLeftDC;
         break;

      case cButtonThumbRight:
         result = cButtonThumbRightDC;
         break;

      case cTriggerLeft:
         result = cTriggerLeftDC;
         break;

      case cTriggerRight:
         result = cTriggerRightDC;
         break;
   }

   return( result );
}

//==============================================================================
// getControlTypeFromDoubleClick
//==============================================================================
long getControlTypeFromDoubleClick( long index )
{
   long result = -1;

   switch( index )
   {
      case cButtonShoulderLeftDC:
         result = cButtonShoulderLeft;
         break;

      case cButtonShoulderRightDC:
         result = cButtonShoulderRight;
         break;

      case cDPadUpDC:
         result = cDpadUp;
         break;

      case cDPadRightDC:
         result = cDpadRight;
         break;

      case cDPadDownDC:
         result = cDpadDown;
         break;

      case cDPadLeftDC:
         result = cDpadLeft;
         break;

      case cButtonYDC:
         result = cButtonY;
         break;

      case cButtonBDC:
         result = cButtonB;
         break;

      case cButtonADC:
         result = cButtonA;
         break;

      case cButtonXDC:
         result = cButtonX;
         break;

      case cButtonThumbLeftDC:
         result = cButtonThumbLeft;
         break;

      case cButtonThumbRightDC:
         result = cButtonThumbRight;
         break;

      case cTriggerLeftDC:
         result = cTriggerLeft;
         break;

      case cTriggerRightDC:
         result = cTriggerRight;
         break;
   }

   return( result );
}
