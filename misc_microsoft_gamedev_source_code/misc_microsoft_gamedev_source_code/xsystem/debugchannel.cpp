//==============================================================================
// debugchannel.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"

#ifdef XBOX
   #include <xbdm.h>
#endif // XBOX
#include "debugchannel.h"
#include "containers\queue.h"
#include "string\fixedString.h"

// One and Only debug channel
BDebugChannel gDebugChannel;

// Static Initializers
BCriticalSection BDebugChannel::mCritical;
BDebugChannel::BStringQueue BDebugChannel::mRemoteBuff;

// Command prefix for things sent across the dbg channel
static const CHAR g_strDebugConsoleCommandPrefix[] = "XCMD";



//--------------------------------------------------------------------------------------
// Temporary replacement for CRT string funcs, since
// we can't call CRT functions on the debug monitor
// thread right now.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Name: dbgtolower
// Desc: Returns lowercase of char
//--------------------------------------------------------------------------------------
inline CHAR dbgtolower( CHAR ch )
{
    if( ch >= 'A' && ch <= 'Z' )
        return ch - ( 'A' - 'a' );
    else
        return ch;
}


//--------------------------------------------------------------------------------------
// Name: dbgstrnicmp
// Desc: Critical section safe string compare.
//       Returns zero if the strings match.
//--------------------------------------------------------------------------------------
static INT dbgstrnicmp( const CHAR* str1, const CHAR* str2, int n )
{
    while( n > 0 )
    {
        if( dbgtolower( *str1 ) != dbgtolower( *str2 ) )
            return *str1 - *str2;
        --n;
        ++str1;
        ++str2;
    }

    return 0;
}


#if 0
//--------------------------------------------------------------------------------------
// Name: dbgstrcpy
// Desc: Critical section safe string copy
//--------------------------------------------------------------------------------------
static VOID dbgstrcpy( CHAR* strDest, const CHAR* strSrc )
{
    while( ( *strDest++ = *strSrc++ ) != 0 );
}
#endif



//==============================================================================
// BDebugChannel::formatMessage
//==============================================================================
void BDebugChannel::formatMessage(const char* strFormat, ... )
{
   // moving the XBOX ifdef up higher because this method does nothing
   // without the DmSendNotificationString call.
#if defined(XBOX) && !defined(BUILD_FINAL)
   BFixedString<MAXRCMDLENGTH> strBuffer;

   strBuffer.format("%s!", g_strDebugConsoleCommandPrefix);

   // Format arguments
   va_list arglist;
   va_start( arglist, strFormat );
   strBuffer.formatAppendArgs(strFormat, arglist);
   va_end( arglist );

   // Send it out the string
   DmSendNotificationString( strBuffer );
#endif // XBOX
}

#ifdef XBOX               
//--------------------------------------------------------------------------------------
// Name: DebugConsoleCmdProcessor
// Desc: Command notification proc that is called by the Xbox debug monitor to
//       have us process a command.  What we'll actually attempt to do is tell
//       it to make calls to us on a separate thread, so that we can just block
//       until we're able to process a command.
//
// Note: Do NOT include newlines in the response string! To do so will confuse
//       the internal WinSock networking code used by the debug monitor API.
//
// Note: It is not possible to set breakpoints in this function with VisualC++.
//       This function is called in the context of a debug system thread that will
//       not stop on breakpoints.
// 
// Note: This function should do as little work as possible because there are
//       many XTL functions that cannot be safely called from this thread. The safest
//       thing to do is to copy the command to a global buffer - protected by a
//       critical section - and let the main thread process the command.
//       Even some of the CRT string handling functions cannot be safely called from
//       this function, which is why dbgstrcpy and dbgstrnicmp exist.
//--------------------------------------------------------------------------------------
HRESULT __stdcall BDebugChannel::DebugConsoleCmdProcessor( const CHAR* strCommand,
                                                   CHAR* strResponse, DWORD dwResponseLen,
                                                   PDM_CMDCONT pdmcc )
{
    trace("Command received.");
    
    // Skip over the command prefix and the exclamation mark
    strCommand += strlen(g_strDebugConsoleCommandPrefix) + 1;

    // Check if this is the initial connect signal
    if( dbgstrnicmp( strCommand, "__connect__", 11 ) == 0 )
    {
        // If so, respond that we're connected
#ifndef BUILD_FINAL
        lstrcpynA( strResponse, "Connected.", dwResponseLen );
#endif//BUILD_FINAL
        return XBDM_NOERR;
    }
    
    if( dbgstrnicmp( strCommand, "__ping__", 8 ) == 0 )
    {
#ifndef BUILD_FINAL
       lstrcpynA( strResponse, "XCMD!pong", dwResponseLen );
       DmSendNotificationString( "pong" );
#endif
       return XBDM_NOERR;
    }

   mCritical.lock();
   
   // rg [6/23/06] - According to MS it may be dangerous to use dynamic memory allocation in the global server thread. So far it seems to work fine..
   const bool success = mRemoteBuff.pushFront(BString(strCommand));
            
   mCritical.unlock();
   
   if (!success)
   {
      trace("BDebugChannel::DebugConsoleCmdProcessor: FIFO full!");
   }
   
    return XBDM_NOERR;
}


//==============================================================================
// BDebugChannel::init
//==============================================================================
void BDebugChannel::init( void)
{
// Initialize ourselves when we're first called.
    static BOOL s_bInitialized = FALSE;
    if( !s_bInitialized )
    {
        // Register our command handler with the debug monitor
        // The prefix is what uniquely identifies our command handler. Any
        // commands that are sent to the console with that prefix and a '!'
        // character (i.e.; XCMD!data data data) will be sent to
        // our callback.
#if defined(XBOX) && !defined(BUILD_FINAL)
        HRESULT hr = DmRegisterCommandProcessor( g_strDebugConsoleCommandPrefix, 
                                                 &BDebugChannel::DebugConsoleCmdProcessor );
        if( FAILED(hr) )
            return;
#endif
         mRemoteBuff.resize(256);

        s_bInitialized = TRUE;
    }
}

//==============================================================================
// BDebugChannel::pumpCommands
//==============================================================================
void BDebugChannel::pumpCommands( void )
{
   BString command;
   
   for ( ; ; )
   {
      mCritical.lock();
   
      const bool execute = mRemoteBuff.popBack(command);
         
      mCritical.unlock();
      
      if (!execute)
         break;
   
      //--now process the command
      if (mpConsole)
      {
         mpConsole->execute(command.getPtr(), NULL);
      }
   }      
}


//==============================================================================
// BDebugChannel::registerConsole
//==============================================================================
void BDebugChannel::registerConsole(IConsoleInterface *pInterface)
{
   if (!pInterface)
      return;
   
   pInterface->registerRemoteOutputTarget(this);
   mpConsole = pInterface;
}

//==============================================================================
// BDebugChannel::releaseConsole
//==============================================================================
void BDebugChannel::releaseConsole( void )
{
   if (!mpConsole)
      return;
      
   mpConsole->releaseRemoteOutputTarget();
   mpConsole = NULL;
}
#endif // XBOX