//==============================================================================
// xssource.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _XSSOURCE_H_
#define _XSSOURCE_H_

//==============================================================================
// Includes
#include "xsdefines.h"
#include "xsfileentry.h"

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif
class BXSMessenger;


//==============================================================================
class BXSSource
{
   public:
      enum
      {
         cDefaultCodeSize=256
      };

      //Ctors/Dtor.
      BXSSource( bool caseSensitive );
      virtual ~BXSSource( void );

      //Case sensitive.
      bool                       getCaseSensitive( void ) const { return(mSymbols.getCaseSensitive()); }

      //File lookups.
      long                       getNumberFiles( void ) const { return(mFiles.getNumber()); }
      BXSFileEntry*              allocateFileEntry( void );
      BXSFileEntry*              getFileEntry( long id ) const;

      //Symbol lookups.
      BXSSymbolTable&            getSymbols( void ) { return(mSymbols); }
      BXSSymbolTable*            getSymbolTable( void ) { return(&mSymbols); }
      const char*                getSymbol( long id ) const;
      //Code.
      SBYTE*                     getCode( void ) const { return(mCode); }
      long                       getCodeSize( void ) const { return(mCodeSize); }
      long                       getMaxCodeSize( void ) const { return(mMaxCodeSize); }
      bool                       addCode( SBYTE *code, long size );
      bool                       overwriteCode( long position, SBYTE *code, long size );

      //Save and load.
      #ifdef _BANG
      bool                       save( BChunkWriter *chunkWriter );
      bool                       load( BChunkReader *chunkReader );
      #endif

   protected:

      //Code.
      SBYTE*                     mCode;
      long                       mCodeSize;
      long                       mMaxCodeSize;
      //Files.
      BXSFileEntryArray          mFiles;
      //Symbols.
      BXSSymbolTable             mSymbols;

      //Static savegame stuff.
      static const DWORD         msSaveVersion;
};

//==============================================================================
#endif
