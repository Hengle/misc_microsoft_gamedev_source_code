//==============================================================================
// xssyscallmodule.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================
#ifndef _XSSYSCALLMODULE_H_
#define _XSSYSCALLMODULE_H_

//==============================================================================
// Includes
#include "xssymboltable.h"
#include "xssyscallentry.h"
#include "string\stringtable.h"

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif
class BXSData;
class BXSMessenger;

//==============================================================================
// Const declarations


//==============================================================================
// BXSSyscallModule is a simple module that encapsulates the useful methods
// for dealing with BXSSyscallArray and adding syscalls to it from the XS macros.
// It expects to be used by both the compiler(s) and interpreter(s) (they are
// derived from this class).  It requires one pure virtual method to be defined
// by the child classes to allow the child classes to decide how to store valid
// variable types.
class BXSSyscallModule
{
   public:
      enum
      {
         cMaximumNumberSyscallParms=12
      };

      //Ctor/Dtor.
      BXSSyscallModule( BXSMessenger *messenger, BXSSymbolTable *symbolTable, bool caseSensitive,
         bool addSymbols, bool storeHelp );
      virtual ~BXSSyscallModule( void );

      //Messenger.
      BXSMessenger*              getMessenger( void ) const { return(mMessenger); }

      //Syscall types.
      void                       initializeSyscallTypes();
      long                       getSyscallType(const char* syscallTypeString) const;
      bool                       buildSyscallTypeString(const BXSSyscallEntry* entry, char* buffer, long bufferSize) const;
      long                       getNumberSyscallTypeStrings() const { return mSyscallTypeStrings.getTags().getNumber(); }
      const char*                getSyscallTypeString(long index) const { return mSyscallTypeStrings.getTagAtIndex(index); }

      //Syscalls.
      BXSSyscallEntryArray&      getSyscalls( void ) { return(mSyscalls); }
      long                       getNumberSyscalls( void ) const { return(mSyscalls.getNumber()); }
      const BXSSyscallEntry*     getSyscall( long syscallID ) const;
      bool                       addSyscall( const char *syscallName, void *address, BYTE returnType );
      bool                       addSyscallIntegerParameter( long defaultValue );
      bool                       addSyscallFloatParameter( float defaultValue );
      bool                       addSyscallBoolParameter( bool defaultValue );
      bool                       addSyscallStringParameter( const char *defaultValue );
      bool                       addSyscallVectorParameter( const BVector &defaultValue );
      bool                       setSyscallContext( bool v );
      //NOTE: addSyscallParameter doesn't do any sanity/error checking on values.
      bool                       addSyscallParameter( BYTE parmType, void *data );
      bool                       setSyscallHelp( const char *helpString );
      bool                       finishSyscallAdd( void );
      BXSSyscallEntry*           getNewSyscall( void ) { return(mNewSyscall); }
      const char*                getNewSyscallName( void ) const;

      //Clear syscall.
      void                       clearSyscall( void );

      //Syscall calls.
      bool                       callVoidSyscall( long syscallID, long *parms, long numberParms );
      bool                       callIntegerSyscall( long syscallID, long *parms, long numberParms, long *sRV, BXSData *data );
      bool                       callFloatSyscall( long syscallID, long *parms, long numberParms, float *sRV, BXSData *data );
      bool                       callBoolSyscall( long syscallID, long *parms, long numberParms, bool *sRV, BXSData *data );
      bool                       callStringSyscall( long syscallID, long *parms, long numberParms, BXSData *data );
      bool                       callVectorSyscall( long syscallID, long *parms, long numberParms, BVector *sRV, BXSData *data );

      //Variable methods.
      bool                       isValidVariableType( long type );

      //Symbol methods.
      long                       addSymbol( const char *symbol, BYTE type, long value );
      long                       getSymbolID( const char *symbol, BYTE type );

      //SyscallID lookup.
      long                       getSyscallID( const char *syscallName );
      //Syscall Name lookup.
      char*                      getSyscallName( long syscallID ) const;

      //Match methods.
      void                       matchSyscallName( const char *matchString, bool prefix, bool complete, BDynamicSimLongArray &results ) const;
      void                       matchSyscallHelp( const char *matchString, bool prefix, bool complete, BDynamicSimLongArray &results ) const;

      //Misc.
      bool                       getLoading( void ) const { return(mLoading); }
      void                       setLoading( bool v ) { mLoading=v; }
      void                       validateLoadSyscall( void );
      void                       errorMsg( const char *lpszFormat, ... );
      void                       infoMsg( const char *lpszFormat, ... );

      //Cleanup.
      void                       cleanUp( void );

      //Save and load.
      #ifdef _BANG
      bool                       save( BChunkWriter *chunkWriter );
      bool                       load( BChunkReader *chunkReader );
      #endif

   protected:

      //Variables.
      BXSMessenger*              mMessenger;
      BXSSyscallEntryArray       mSyscalls;
      BXSSymbolTable*            mSymbols;
      BXSSyscallEntry*           mNewSyscall;
      bool                       mCaseSensitive;
      bool                       mAddSymbols;
      bool                       mStoreHelp;
      static bool                mSyscallTypesInitialized;
      static BStringTable<long>  mSyscallTypeStrings;
      //Save and load stuff.
      bool                       mLoading;
      BXSSyscallEntry*           mLoadSyscall;
      long                       mLoadAddress;
      long                       mLoadCurrentParameter;
      bool                       mLoadSyscallFailed;

      //Static savegame stuff.
      static const DWORD         msSaveVersion;
};


//==============================================================================
#endif // _XSSYSCALLMODULE_H_
