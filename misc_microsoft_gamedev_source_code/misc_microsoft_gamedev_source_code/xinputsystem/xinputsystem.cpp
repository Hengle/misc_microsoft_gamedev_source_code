//==============================================================================
// xinputsystem.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "xinputsystem.h"
#include "configsinput.h"

// Globals
static long             gXInputRefCount=0;
static XInputSystemInfo gXInputSystemInfo;

//==============================================================================
// XInputSystemCreate
//==============================================================================
bool XInputSystemCreate(XInputSystemInfo* info)
{
   if(gXInputRefCount==0)
   {
      if(info)
         gXInputSystemInfo=*info;

      if (!gInputSystem.setup(gXInputSystemInfo.mWindowHandle, gXInputSystemInfo.mRegisterConsoleFuncs))
         return false;

      if(gXInputSystemInfo.mpEventHandler)
         gInputSystem.addEventHandler(gXInputSystemInfo.mpEventHandler);

      if(!gXInputSystemInfo.mRootContext.isEmpty())
         gInputSystem.enterContext(gXInputSystemInfo.mRootContext);
         
      if (gConfig.isDefined(cConfigMinimalConsoleTraffic))
      {
         gConsole.setChannelEnabled(cMsgResource, false);
         gConsole.setChannelEnabled(cMsgFileManager, false);
      }
   }

   gXInputRefCount++;

   return true;
}

//==============================================================================
// XInputSystemRelease
//==============================================================================
void XInputSystemRelease()
{
   if(gXInputRefCount==0)
   {
      BASSERT(0);
      return;
   }

   gXInputRefCount--;

   if(gXInputRefCount==0)
   {
      gInputSystem.removeEventHandler(gXInputSystemInfo.mpEventHandler);
      gInputSystem.shutdown();
   }
}
