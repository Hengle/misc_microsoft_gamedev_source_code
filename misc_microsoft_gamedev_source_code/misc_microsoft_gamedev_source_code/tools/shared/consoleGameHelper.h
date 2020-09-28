// File: consoleGameHelper.h
#pragma once

#include "atgConsole.h"

// Directory IDs
extern __declspec(selectany) long cDirData      = -1;
extern __declspec(selectany) long cDirArt       = -1;
extern __declspec(selectany) long cDirScenario  = -1;
extern __declspec(selectany) long cDirFonts     = -1;

class BConsoleGameHelper
{
public:
   // Init
   static bool                   setup(bool useXFS = true, bool xfsCopy = false);
   static bool                   deinit(void); 

   static bool                   setupGameDirectories(void);   
   static void                   setNoAutoRender(bool val);
   
   // Input
   static DWORD                  getButtons(void);
   static DWORD                  waitForButtonPress(DWORD buttons = XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B);
   
   // Console
   static bool                   consoleInit(void);
   static void                   consoleDeinit(void);
   static void                   consoleRender(void);
   static void                   consoleClear(void);
   static BOOL                   getConsoleValid(void) { return mConsole.GetValid(); }
   static IDirect3DDevice9*      getD3DDevice(void) { return mConsole.GetDevice(); }
   static ATG::Console&          getConsole(void) { return mConsole; }
     
private:   
   static long                   setupGameDir(const BSimString& dirName);
   static void                   emptyThreadQueues(void);
   
   static void                   consoleOutputFunc(void* data, BConsoleMessageCategory category, const char* pMessage);
   
   static ATG::Console           mConsole;
};


