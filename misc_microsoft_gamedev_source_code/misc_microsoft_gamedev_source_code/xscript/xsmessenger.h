//==============================================================================
// xsmessenger.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes.
#ifndef _BXSMESSENGER_H_
#define _BXSMESSENGER_H_

//==============================================================================
// Forward Declarations.
typedef void (*STANDARDMSGFUN)(const char * msg);


//==============================================================================
class BXSMessenger
{
   public:
      BXSMessenger( void );
      ~BXSMessenger( void );

      //Function sets.
      void                       setErrorMsgFun( STANDARDMSGFUN v ) { mErrorMsgFun=v; }
      void                       setWarnMsgFun( STANDARDMSGFUN v ) { mWarnMsgFun=v; }
      void                       setInfoMsgFun( STANDARDMSGFUN v ) { mInfoMsgFun=v; }
      void                       setRunMsgFun( STANDARDMSGFUN v ) { mRunMsgFun=v; }
      void                       setDebuggerMsgFun( STANDARDMSGFUN v ) { mDebuggerMsgFun=v; }

      //Simple gets and sets.  Errors are always on.
      long                       getErrorCount( void ) { return(mErrorCount); }
      long                       getWarningCount( void ) { return(mWarningCount); }
      bool                       getWarn( void ) const { return(mWarn); }
      void                       setWarn( bool v ) { mWarn=v; }
      bool                       getInfo( void) const { return(mInfo); }
      void                       setInfo( bool v ) { mInfo=v; }
      bool                       getRun( void) const { return(mRun); }
      void                       setRun( bool v ) { mRun=v; }
      void                       resetCounters( void ) { mErrorCount=0; mWarningCount=0; }

      //More specific gets and sets.  These control things that are second level things underneath
      //stuff like runMsg, etc.
      bool                       getListInterpreter( void ) const { return(mListInterpreter); }
      void                       setListInterpreter( bool v ) { mListInterpreter=v; }
      bool                       getListFunctionEntry( void ) const { return(mListFunctionEntry); }
      void                       setListFunctionEntry( bool v ) { mListFunctionEntry=v; }
      bool                       getDebuggerInterpreter( void ) const { return(mDebuggerInterpreter); }
      void                       setDebuggerInterpreter( bool v ) { mDebuggerInterpreter=v; }
      
      //Error message methods.
      void                       errorMsg( const char *lpszFormat, ... );
      void                       sourceErrorMsg( const char * filename, long lineNumber, const char *lpszFormat, ... );

      //Warning message methods.
      void                       warningMsg( const char *lpszFormat, ... );
      void                       sourceWarningMsg( const char * filename, long lineNumber, const char *lpszFormat, ... );

      //Info message methods.
      void                       infoMsg(const char *lpszFormat, ...);
      void                       sourceInfoMsg( const char * filename, long lineNumber, const char *lpszFormat, ... );

      //Run message methods.
      void                       runMsg(const char *lpszFormat, ...);

      //Debugger message methods.
      void                       debuggerMsg(const char *lpszFormat, ...);

      //Simple message methods.
      void                       msg(const char *lpszFormat, ...);


   protected:
      STANDARDMSGFUN             mErrorMsgFun;
      STANDARDMSGFUN             mWarnMsgFun;
      STANDARDMSGFUN             mInfoMsgFun;
      STANDARDMSGFUN             mRunMsgFun;
      STANDARDMSGFUN             mDebuggerMsgFun;

      long                       mErrorCount;
      long                       mWarningCount;
      bool                       mWarn;
      bool                       mInfo;
      bool                       mRun;
      bool                       mListInterpreter;
      bool                       mListFunctionEntry;
      bool                       mDebuggerInterpreter;
};


//==============================================================================
#endif // _BXSMESSENGER_H_
