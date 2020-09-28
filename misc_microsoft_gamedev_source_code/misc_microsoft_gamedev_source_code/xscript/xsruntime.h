//==============================================================================
// xsruntime.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSRUNTIME_H_
#define _XSRUNTIME_H_

//==============================================================================
// Includes.  We go ahead and include more than we strictly have to because this
// is supposed to wrap-up a lot of functionality, so anything that uses this will
// likely include xscompiler.h, etc.
#include "color.h"
#include "xsconfig.h"
#include "xsdata.h"
#include "xsdefines.h"
#include "xscompiler.h"
#include "xsinterpreter.h"
#include "xsmessenger.h"
#include "xssyscallmodule.h"

//==============================================================================
// Const declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif


//==============================================================================
class BXSRuntime
{
   public:
      BXSRuntime( const BSimString &name, long playerID, long instanceLimit );
      virtual ~BXSRuntime( void );

      //ID, PlayerID, and Name.
      long                       getID( void ) const { return(mID); }
      long                       getPlayerID( void ) const { return(mPlayerID); }
      const BSimString&             getName( void ) const { return(mName); }
      void                       setName( const BSimString &name ) { mName=name; }
      //Instance Limit.
      long                       getInstanceLimit( void ) const { return(mInstanceLimit); }
      //Case.
      bool                       getCaseSensitive( void ) const { return(mCaseSensitive); }
      void                       setCaseSensitive( bool v ) { mCaseSensitive=v; }

      //Setup.
      void                       setWarningsOn( bool v ) { mWarningsOn=v; }
      void                       setInfoMessagesOn( bool v ) { mInfoMessagesOn=v; }
      void                       setRunMessagesOn( bool v ) { mRunMessagesOn=v; }
      void                       setListInterpreter( bool v ) { mListInterpreter=v; }
      void                       setListFunctionEntry( bool v ) { mListFunctionEntry=v; }
      void                       setGenerateListing( bool v ) { mGenerateListing=v; }
      void                       setDebugTokenizer( bool v ) { mDebugTokenizer=v; }
      void                       setDebugCode( bool v ) { mDebugCode=v; }
      void                       setErrorFunction( STANDARDMSGFUN v ) { mErrorFunction=v; }
      void                       setWarningFunction( STANDARDMSGFUN v ) { mWarningFunction=v; }
      void                       setInfoMessageFunction( STANDARDMSGFUN v ) { mInfoMessageFunction=v; }
      void                       setRunMessageFunction( STANDARDMSGFUN v ) { mRunMessageFunction=v; }
      void                       setDebuggerMessageFunction( STANDARDMSGFUN v ) { mDebuggerMessageFunction=v; }
      void                       setAllMessageFunctions( STANDARDMSGFUN v ) { mErrorFunction=mWarningFunction=mInfoMessageFunction=mRunMessageFunction=mDebuggerMessageFunction=v; }

      //Initialization.  This should be called AFTER the config functions have
      //been added.
      bool                       initialize( bool defineMathVariables, long baseDirID, long baseUWDirID );
      //Configuration functions.
      bool                       addConfigFunction( BXSConfigFunction cf, bool apply );
      void                       clearConfigFunctions( void );

      //Access methods.
      BXSMessenger*              getMessenger( void ) const { return(mMessenger); }
      BXSCompiler*               getCompiler( void ) const { return(mCompiler); }
      BXSInterpreter*            getInterpreter( void ) const { return(mInterpreter); }
      BXSSource*                 getSource( void ) const { return(mSource); }
      //Datas.
      long                       getNumberDatas( void ) const { return(mDatas.getNumber()); }
      BXSData*                   getData( long dataID ) const;
      long                       allocateData( void );
      bool                       deleteData( long dataID );
      bool                       isReadyToExecute( long dataID ) const;

      //NOTE: ContextPlayerID may or may not be set during interp; it's the game's responsibility
      //to set that appropriately.  ContextRuntimeID should always automatically be set
      //(by this class); don't muck with it.
      //Context player.
      static long                getXSContextPlayerID( void ) { return(mXSContextPlayerID); }
      static void                setXSContextPlayerID( long v ) { mXSContextPlayerID=v; }
      //Context runtime.
      static long                getXSContextRuntimeID( void ) { return(mXSContextRuntimeID); }
      static void                setXSContextRuntimeID( long v ) { mXSContextRuntimeID=v; }

      //Function ID lookup.  BE CAREFUL USING THIS.  The function IDs will change with different
      //compiles, so they need to be fixed up, etc.
      long                       getFunctionID( const char *functionName ) const;

      //Module management.
      long                       getNumberModules( void ) const;
      const BSimString&             getModuleFilename( long index ) const;
      bool                       addModuleFilename( const BSimString &filename );       
      bool                       removeModuleFilename( const BSimString &filename );       

      //Compilation.  NOTE: You must call initialize from outside this class to clean
      //out symbols, etc.  This class cannot call that because there may have been external
      //variables added from outside.
      bool                       compileModules( bool deleteCompiler );
      bool                       compileFile( const BSimString &filename, const BSimString &qualifiedFilename,
                                    const BSimString &listingPrefix, bool deleteCompiler );

      //Interpretation.  Interpret functions that take functionIDs instead of functionNames
      //are obviously faster.  However, bad things will happen if you foobar the functionID
      //lookup, so be careful calling those.
      bool                       interpretCode( long dataID, const char *callingString=NULL );
      bool                       interpretFunction( long dataID, const char *functionName, const char *callingString=NULL );
      bool                       interpretRules( long dataID, DWORD currentTime, DWORD timeLimit, const char *callingString=NULL );
      void                       resetRuleTime( long dataID, DWORD currentTime );
      bool                       interpretTrigger( long dataID, DWORD currentTime, const char *callingString=NULL );
      bool                       interpretHandler( long dataID, long functionID, long parameter, const char *callingString=NULL );
      bool                       interpretHandler( long dataID, const char *functionName, long parameter, const char *callingString=NULL );
      bool                       interpretBreakpoint( long dataID );
      //Calling string.  A simple string that can be set by the code that calls one
      //of the interpretXXX methods.  If set, it will be stored throughout execution
      //and then cleared out when done.  Used for making the debugger nicer to look at.
      const char*                getCallingString( long dataID ) const;
      void                       setCallingString( long dataID, const char *text );

      //Events.  The idea here is that the code that wants to immediately call interpHandler with
      //a given parm will instead use this facility to queue up the events.  This way, that code
      //can still work with the breakpoint system.
      bool                       addEvent( long dataID, long functionID, long parameter );
      bool                       addEvent( long dataID, const char *functionName, long parameter );
      bool                       interpretEvents( long dataID, const char *callingString=NULL );

      //Breakpoints.
      bool                       inBreakpoint( long dataID ) const;
      bool                       setBreakpoint( long dataID, const BSimString &filename, long lineNumber, bool on );
      bool                       setBreakpoint( long dataID, long fileID, long lineNumber, bool on );
      bool                       setFunctionStartBreakpoint( long dataID, long functionID, bool on );
      // This sets up a function return breakpoint that pops when we get back to the current callstack level.
      bool                       setStepOverBreakpoint( long dataID );
      //Current time.
      DWORD                      getCurrentTime( long dataID ) const;
      void                       setCurrentTime( long dataID, DWORD v );

      //Save and load.
      #ifdef _BANG
      bool                       save( BChunkWriter *chunkWriter, bool scenario );
      bool                       load( BChunkReader *chunkReader, bool scenario );
      #endif

      //Output.
      bool                       getOutputChanged( long dataID ) const;
      void                       clearOutputChanged( long dataID );
      void                       addOutput( long dataID, const BColor &color, const char *v, ... );
      const BDynamicSimArray<BSimString>& getOutput( long dataID ) const;
      const BDynamicSimArray<BColor>& getOutputColors( long dataID ) const;
      long                       getOutputLineNumber( long dataID ) const;
      void                       setOutputLineNumber( long dataID, long v ) const;
      //Default Output.
      static void                defaultOutputMessage( const char *text );

      //Instance support.
      static long                getNumberXSRuntimes( void ) { return(mXSRuntimes.getNumber()); }
      static BXSRuntime*         getXSRuntimeByID( long id );
      static BXSRuntime*         getXSRuntimeByIndex( long index );
      static BXSRuntime*         getXSRuntime( const BSimString &name );
      
   protected:
      //Misc.
      bool                       addCommonMathVariables( void );
      //Cleanup.
      void                       cleanUpAll( void );
      void                       cleanUpData( void );


      //Variables.
      long                       mID;
      long                       mPlayerID;
      BSimString                    mName;

      //High level classes.
      BXSMessenger*              mMessenger;
      BXSCompiler*               mCompiler;
      BXSInterpreter*            mInterpreter;
      BXSSyscallModule*          mSyscalls;
      BXSSource*                 mSource;
      BDynamicSimLongArray           mConfigFunctions;
      //Data.
      BXSDataArray               mDatas;

      //Setup options.
      long                       mInstanceLimit;
      bool                       mCaseSensitive;
      bool                       mWarningsOn;
      bool                       mInfoMessagesOn;
      bool                       mRunMessagesOn;
      bool                       mListInterpreter;
      bool                       mListFunctionEntry;
      bool                       mGenerateListing;
      bool                       mDebugTokenizer;
      bool                       mDebugCode;
      STANDARDMSGFUN             mErrorFunction;
      STANDARDMSGFUN             mWarningFunction;
      STANDARDMSGFUN             mInfoMessageFunction;
      STANDARDMSGFUN             mRunMessageFunction;
      STANDARDMSGFUN             mDebuggerMessageFunction;

      //Directories.
      long                       mBaseDirectoryID;
      long                       mBaseUWDirectoryID;

      //Context player ID.
      static long                mXSContextPlayerID;
      //Context runtime ID.
      static long                mXSContextRuntimeID;
      //Instance stuff.  Static so that it serves as an 'under-the-covers' manager
      //type of thing.
      static long                mNextRuntimeID;
      static BDynamicSimArray<BXSRuntime*> mXSRuntimes;

      //Static savegame stuff.
      static const DWORD         msSaveVersion;
};


//==============================================================================
#endif // _XSRUNTIME_H_
