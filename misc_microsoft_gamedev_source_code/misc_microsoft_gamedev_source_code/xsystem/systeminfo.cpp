//============================================================================
//
//  systeminfo.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================

#include "xsystem.h"
#include "systeminfo.h"


//============================================================================
// BSystemInfo::BSystemInfo
//============================================================================
BSystemInfo::BSystemInfo(void)
{
#ifdef XBOX
   mIsWin98orME = false;
   mIsWin2k = false;
   // rg [6/13/05] - I'm setting Xenon to WinXP to avoid breaking code that assumes at least one OS flag is true. (?)
   mIsWinXP = true;
#else
   // Get OS version.
   memset(&mOSVersionInfo, 0, sizeof(mOSVersionInfo));
   mOSVersionInfo.dwOSVersionInfoSize = sizeof(mOSVersionInfo);
   if (!GetVersionEx(&mOSVersionInfo))
   {
      // Should this be fatal?
      BFAIL("Could not determine OS version");
   }

   // Cache off a bool about whether this is Win9x/ME.
   if ((mOSVersionInfo.dwMajorVersion == 4) && ((mOSVersionInfo.dwMinorVersion == 10) || (mOSVersionInfo.dwMinorVersion == 90)))
      mIsWin98orME = true;
   else
      mIsWin98orME= false;

   // Cache off a bool about whether this is Win2k
   if ((mOSVersionInfo.dwMajorVersion == 5) && (mOSVersionInfo.dwMinorVersion == 0))
      mIsWin2k = true;
   else
      mIsWin2k = false;

   // Cache off a bool about whether this is WinXP
   if ((mOSVersionInfo.dwMajorVersion == 5) && (mOSVersionInfo.dwMinorVersion == 1))
      mIsWinXP = true;
   else
      mIsWinXP = false;
#endif      
}


//============================================================================
// eof: systeminfo.cpp
//============================================================================
