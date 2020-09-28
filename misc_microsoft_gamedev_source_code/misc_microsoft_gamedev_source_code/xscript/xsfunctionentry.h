//==============================================================================
// xsfunctionentry.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSFUNCTIONENTRY_H_
#define _XSFUNCTIONENTRY_H_

//==============================================================================
// Includes
#include "xsvariableentry.h"

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
class BXSData;
#endif


//==============================================================================
class BXSFunctionEntry
{
   public:
      BXSFunctionEntry( void );
      ~BXSFunctionEntry( void );

      //ID.
      long                       getID( void ) const { return(mID); }
      void                       setID( long v ) { mID=v; }
      //Symbol.
      long                       getSymbolID( void ) const { return(mSymbolID); }
      void                       setSymbolID( long v ) { mSymbolID=v; }
      //File ID.
      long                       getFileID( void ) const { return(mFileID); }
      void                       setFileID( long v ) { mFileID=v; }
      //Line number.
      long                       getLineNumber( void ) const { return(mLineNumber); }
      void                       setLineNumber( long v ) { mLineNumber=v; }
      //Return Type.
      BYTE                       getReturnType( void ) const { return(mReturnType); }
      void                       setReturnType( BYTE v ) { mReturnType=v; }
      //Code Offset.
      long                       getCodeOffset( void ) const { return(mCodeOffset); }
      void                       setCodeOffset( long v ) { mCodeOffset=v; }
      //Parms.  NOTE: Parms are the first N variables, so you "get" them back out by calling
      //getVariable() with the appropriate parm index.  Once inside a block of code, parms
      //and vars are all the same to the code that's looking them up.
      BYTE                       getNumberParameters( void ) const { return(mNumberParameters); }
      long                       getParameterType( long index ) const;
      bool                       addParameter( BXSVariableEntry *ve );
      //Variables.
      long                       getNumberVariables( void ) const { return(mVariables.getNumber()); }
      BXSVariableEntry*          getVariable( long index ) const;
      bool                       addVariable( BXSVariableEntry *ve );
      //Mutable.
      bool                       getMutable( void ) const { return(mMutable); }
      void                       setMutable( bool v ) { mMutable=v; }

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      static void                setVersion( DWORD v ) { msLoadVersion=v; }
      bool                       save( BChunkWriter* chunkWriter, BXSData *data );
      bool                       load( BChunkReader* chunkReader, BXSData *data );
      #endif

      //Copy.
      bool                       copy( BXSFunctionEntry *v );

   protected:
      long                       mID;
      long                       mSymbolID;
      long                       mFileID;
      long                       mLineNumber;
      long                       mCodeOffset;
      BXSVariableEntryArray      mVariables;
      bool                       mMutable;
      BYTE                       mReturnType;
      BYTE                       mNumberParameters;

      static const DWORD      msSaveVersion;
      static DWORD            msLoadVersion;
};
typedef BDynamicSimArray<BXSFunctionEntry*> BXSFunctionEntryArray;


//==============================================================================
#endif // _XSFUNCTIONENTRY_H_

//==============================================================================
// eof: xsfunctionentry.h
//==============================================================================
