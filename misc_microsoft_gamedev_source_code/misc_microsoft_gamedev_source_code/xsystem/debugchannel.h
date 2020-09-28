//==============================================================================
// debugchannel.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#ifndef _DEBUG_CHANNEL
#define _DEBUG_CHANNEL

#define MAXRCMDLENGTH       256                 // Size of the remote cmd buffer

#ifdef XBOX               
#include <xbdm.h>
#endif // XBOX

#include "containers\queue.h"

//==============================================================================
// IRemoteOutputTarget
//==============================================================================
class IRemoteOutputTarget
{
public:
    virtual void formatMessage(const char* szFormat, ... )=0;   
};


//==============================================================================
// IConsoleInterface
//==============================================================================
class IConsoleInterface
{
public:

   enum
   {
      cErrOK,
      cErrGetBinaryFailed,
      cErrCouldNotCompile,
      cErrNothingToCompile,
      cErrFile,
   };
      
   virtual long execute(const char* szCommand, const char*szUnused)=0;
   virtual bool registerRemoteOutputTarget(IRemoteOutputTarget *pTarget)=0;
   virtual bool releaseRemoteOutputTarget( void )=0;
};


class BDebugChannel : public IRemoteOutputTarget
{
public:

   BDebugChannel() : mpConsole(NULL)         {};
   ~BDebugChannel()                          {};
   
   void                       init( void );
   void                       pumpCommands( void );
   void                       formatMessage(const char* szFormat, ... ); 
        
   void        registerConsole(IConsoleInterface *pConsole);
   void        releaseConsole( void );
   
protected:

   static BCriticalSection  mCritical;
   typedef BQueue<BString> BStringQueue;
   static BStringQueue mRemoteBuff;
   IConsoleInterface *mpConsole;
      
#ifdef XBOX               
   static HRESULT __stdcall   DebugConsoleCmdProcessor( const CHAR* strCommand,
      CHAR* strResponse, DWORD dwResponseLen,
      PDM_CMDCONT pdmcc );
#endif // XBOX
};


extern BDebugChannel gDebugChannel;

#endif
