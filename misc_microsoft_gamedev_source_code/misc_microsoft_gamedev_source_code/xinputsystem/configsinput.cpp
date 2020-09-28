//==============================================================================
// configsinput.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "configsinput.h"

//==============================================================================
// Defines
//==============================================================================
DEFINE_CONFIG(cConfigControllerConfig);
DEFINE_CONFIG(cConfigControlMap);
DEFINE_CONFIG(cConfigControlSet);
DEFINE_CONFIG(cConfigGamepadRumble);
DEFINE_CONFIG(cConfigGamepadDoubleClickTime);
DEFINE_CONFIG(cConfigGamepadHoldTime);
DEFINE_CONFIG(cConfigActionWithDirectionHoldTime);
DEFINE_CONFIG(cConfigConsoleSpew);
DEFINE_CONFIG(cConfigConsoleFileEntire);
DEFINE_CONFIG(cConfigXSWarn);
DEFINE_CONFIG(cConfigXSInfo);
DEFINE_CONFIG(cConfigThreadInput);
DEFINE_CONFIG(cConfigConsoleStringLimit);
DEFINE_CONFIG(cConfigMinimalConsoleTraffic);
DEFINE_CONFIG(cConfigChatPadRemapping);
DEFINE_CONFIG(cConfigUserGamepadRumble);

//==============================================================================
// BConfigsInput::registerConfigs
//==============================================================================
static bool registerInputConfigs(bool)
{
   DECLARE_CONFIG(cConfigControllerConfig,            "ControllerConfig",            "", 0, NULL);
   DECLARE_CONFIG(cConfigControlMap,                  "ControlMap",                  "", 0, NULL);
   DECLARE_CONFIG(cConfigControlSet,                  "ControlSet",                  "", 0, NULL);
   DECLARE_CONFIG(cConfigGamepadRumble,               "GamepadRumble",               "", 0, NULL);
   DECLARE_CONFIG(cConfigGamepadDoubleClickTime,      "GamepadDoubleClickTime",      "", 0, NULL);
   DECLARE_CONFIG(cConfigGamepadHoldTime,             "GamepadHoldTime",             "", 0, NULL);   
   DECLARE_CONFIG(cConfigActionWithDirectionHoldTime, "ActionWithDirectionHoldTime", "", 0, NULL);
   DECLARE_CONFIG(cConfigConsoleSpew,                 "ConsoleSpew",                 "", 0, NULL);
   DECLARE_CONFIG(cConfigConsoleFileEntire,           "ConsoleFileEntire",           "", 0, NULL);
   DECLARE_CONFIG(cConfigXSWarn,                      "XSWarn",                      "", 0, NULL);
   DECLARE_CONFIG(cConfigXSInfo,                      "XSInfo",                      "", 0, NULL);
   DECLARE_CONFIG(cConfigThreadInput,                 "ThreadInput",                 "", 0, NULL);
   DECLARE_CONFIG(cConfigConsoleStringLimit,          "ConsoleStringLimit",          "", 0, NULL);
   DECLARE_CONFIG(cConfigMinimalConsoleTraffic,       "MinimalConsoleTraffic",       "", 0, NULL);
   DECLARE_CONFIG(cConfigChatPadRemapping,            "ChatPadRemapping",            "", 0, NULL);
   DECLARE_CONFIG(cConfigUserGamepadRumble,           "UserGamepadRumble",           "", 0, NULL);
   return true;
}

// This causes xcore to call registerInputConfigs() after it initializes.
#pragma data_seg(".ENS$XIU") 
BXCoreInitFuncPtr gpRegisterInputConfigs[] = { registerInputConfigs };
#pragma data_seg() 
