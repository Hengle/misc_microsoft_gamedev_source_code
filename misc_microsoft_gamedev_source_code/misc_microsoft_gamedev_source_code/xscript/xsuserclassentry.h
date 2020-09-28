//==============================================================================
// xsuserclassentry.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _XSUSERCLASSENTRY_H_
#define _XSUSERCLASSENTRY_H_

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
class BXSUserClassEntry
{
   public:

      BXSUserClassEntry( void );
      ~BXSUserClassEntry( void );

      //ID.
      long                       getID( void ) const { return(mID); }
      void                       setID( long v ) { mID=v; }
      //Symbol ID.
      long                       getSymbolID( void ) const { return(mSymbolID); }
      void                       setSymbolID( long v ) { mSymbolID=v; }

      //Variables.
      long                       getNumberVariables( void ) const { return(mVariables.getNumber()); }
      BXSVariableEntry*          getVariable( long index ) const;
      bool                       addVariable( BXSVariableEntry *ve );

      //Init-related methods.
      long                       getDataLength( void ) const;
      bool                       init( BYTE *data );

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      bool                       save( BChunkWriter* chunkWriter, BXSData *data );
      bool                       load( BChunkReader* chunkReader, BXSData *data );
      #endif

      //Copy.
      bool                       copy( BXSUserClassEntry *v );

   protected:
      long                       mID;
      long                       mSymbolID;
      BXSVariableEntryArray      mVariables;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicSimArray<BXSUserClassEntry*> BXSUserClassEntryArray;


//==============================================================================
#endif
