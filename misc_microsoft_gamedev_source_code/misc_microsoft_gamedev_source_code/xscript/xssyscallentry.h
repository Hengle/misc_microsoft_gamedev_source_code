//==============================================================================
// xssyscallentry.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSSYSCALLENTRY_H_
#define _XSSYSCALLENTRY_H_

//==============================================================================
// Includes
#include "xsvariableentry.h"

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif


//==============================================================================
class BXSSyscallEntry
{
   public:
      BXSSyscallEntry( void );
      ~BXSSyscallEntry( void );

      //ID.
      long                       getID( void ) const { return(mID); }
      void                       setID( long v ) { mID=v; }
      //Symbol.
      long                       getSymbolID( void ) const { return(mSymbolID); }
      void                       setSymbolID( long v ) { mSymbolID=v; }
      //Return Type.
      BYTE                       getReturnType( void ) const { return(mReturnType); }
      void                       setReturnType( BYTE v ) { mReturnType=v; }
      //Address.
      long                       getAddress( void ) const { return(mAddress); }
      void                       setAddress( long v ) { mAddress=v; }
      //Context.
      bool                       getContext( void ) const { return(mContext); }
      void                       setContext( bool v ) { mContext=v; }
      //Help.
      const char*                getHelp( void ) const { return(mHelp); }
      bool                       setHelp( const char *helpString );
      //Parms.
      long                       getNumberParameters( void ) const { return(mParameters.getNumber()); }
      BXSVariableEntry*          getParameter( long index ) const;
      long                       getParameterType( long index ) const;
      bool                       addParameter( BXSVariableEntry *ve );
      //Syscall Type.
      long                       getSyscallType() const { return mSyscallType; }
      void                       setSyscallType(long v) { mSyscallType=v; }

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      static void                setVersion( DWORD v ) { msLoadVersion=v; }
      bool                       save( BChunkWriter* chunkWriter );
      bool                       load( BChunkReader* chunkReader );
      #endif

   protected:
      long                       mID;
      long                       mSymbolID;
      long                       mAddress;
      char*                      mHelp;
      BXSVariableEntryArray      mParameters;
      long                       mSyscallType;
      BYTE                       mReturnType;
      bool                       mContext;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicArray<BXSSyscallEntry*> BXSSyscallEntryArray;


//==============================================================================
#endif // _XSSYSCALLENTRY_H_
