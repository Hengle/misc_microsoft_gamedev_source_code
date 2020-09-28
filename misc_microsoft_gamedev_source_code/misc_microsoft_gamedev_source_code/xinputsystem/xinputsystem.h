//==============================================================================
// xinputsystem.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#ifndef _XINPUTSYSTEM_H_
#define _XINPUTSYSTEM_H_

// Dependant files
#include "xsystem.h"

// Local files
#include "inputsystem.h"
#include "gamepad.h"

// XInputSystemInfo
class XInputSystemInfo
{
   public:
      XInputSystemInfo() :
         mWindowHandle(NULL),
         mRegisterConsoleFuncs(NULL),
         mpEventHandler(NULL),
         mRootContext()
      {
      }

      HWND  mWindowHandle;
      REGISTER_CONSOLE_FUNCS_CALLBACK  mRegisterConsoleFuncs;
      IInputEventHandler*  mpEventHandler;
      BSimString mRootContext;
};

// Functions
bool XInputSystemCreate(XInputSystemInfo* info);
void XInputSystemRelease();

#endif
