//============================================================================
//
//  systeminfo.h
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================


#pragma once

#ifndef _SYSTEMINFO_H_
#define _SYSTEMINFO_H_

//============================================================================
// class BSystemInfo
//============================================================================
class BSystemInfo
{
   public:
                              BSystemInfo(void);

      // These functions get info about the current OS.  You should be careful about how you use these because
      // it's easy to make logic bugs on new OSes that ship after the code was written.  They should really
      // just be used to enable workarounds for known problems in existing OSes.
      bool                    isWin98orME(void) const {return(mIsWin98orME);}
      bool                    isWin2k(void) const {return(mIsWin2k);}
      bool                    isWinXP(void) const {return(mIsWinXP);}
#ifdef XBOX
      // rg [6/10/05] - Got to return something.
      DWORD                   getOSMajorVersion(void) const {return(99);}
      DWORD                   getOSMinorVersion(void) const {return(0);}
#else      
      DWORD                   getOSMajorVersion(void) const {return(mOSVersionInfo.dwMajorVersion);}
      DWORD                   getOSMinorVersion(void) const {return(mOSVersionInfo.dwMinorVersion);}
#endif      

   protected:
      bool                    mIsWin98orME;
      bool                    mIsWin2k;
      bool                    mIsWinXP;
#ifndef XBOX      
      OSVERSIONINFO           mOSVersionInfo;
#endif      
};

// The actual storage for this is declared in freakyWeirdStartupStatics.cpp to ensure proper
// ordering of the construction/destruction of this.
extern BSystemInfo gSystemInfo;

#endif


//============================================================================
// eof: systeminfo.h
//============================================================================
