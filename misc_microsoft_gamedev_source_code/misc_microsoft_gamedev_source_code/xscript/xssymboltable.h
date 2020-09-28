//==============================================================================
// xssymboltable.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSSYMBOLTABLE_H_
#define _XSSYMBOLTABLE_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BXSSymbolEntry;
class BXSSymbolTableStats;

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif


//==============================================================================
class BXSSymbolEntry
{
   public:
      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      static void                setVersion( DWORD v ) { msLoadVersion=v; }
      bool                       save( BChunkWriter* chunkWriter );
      bool                       load( BChunkReader* chunkReader );
      #endif

      //Variables.
      char*                      mSymbol;
      long                       mValue;
      long                       mID;
      BXSSymbolEntry*            mNext;
      BYTE                       mType;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicArray<BXSSymbolEntry*> BXSSymbolEntryPointerArray;


//==============================================================================
class BXSSymbolTableStats
{
   public:
      BXSSymbolTableStats( void ) :
         mSize(0),
         mLists(0),
         mLongest(0),
         mAverage(0),
         mCount(0)
         { }

      long                       mSize;
      long                       mLists;
      long                       mLongest;
      long                       mAverage;
      long                       mCount;
};


//==============================================================================
class BXSSymbolTable
{
   public:
      //Ctor/Dtor.
      BXSSymbolTable( void );
      ~BXSSymbolTable( void );

      //Case sensitivity.
      bool                       getCaseSensitive( void ) const { return(mCaseSensitive); }

      //Initialize.
      bool                       initialize( long hashTableSize, bool caseSensitive, long invalidValue, BYTE invalidType );

      //Symbol methods.
      long                       getNumberSymbols( void ) const { return(mNumberSymbols); }
      //Symbol.
      const char*                getSymbolByValue( long value ) const;
      const char*                getSymbolByID( long id ) const;
      bool                       isDefined( const char *symbol );
      //Value.
      long                       getValueBySymbol( const char *symbol ) const;
      long                       getValueByID( long id ) const;
      //Type.
      BYTE                       getTypeBySymbol( const char *symbol ) const;
      BYTE                       getTypeByID( long id ) const;
      //ID.
      long                       getIDBySymbol( const char *symbol ) const;
      long                       getIDBySymbolAndType( const char *symbol, BYTE type ) const;
      //Symbol Entry.
      const BXSSymbolEntry*      getEntryBySymbol( const char *symbol ) const;
      const BXSSymbolEntry*      getEntryByID( long id ) const;

      //Symbol Addition.
      long                       addSymbol( const char *symbol, BYTE type, long value );
      long                       addSymbol( const char *symbol, BYTE type, long value, long id );
      bool                       setSymbolValue( long symbolID, long value );


      //Hash list lookups.  Do we need these anymore?
      long                       getHashTableSize( void ) const { return mHashTableSize; };
      BXSSymbolEntry*            getHashList( const char *symbol ) const;
      BXSSymbolEntry*            getHashList( long hashIndex ) const;

      //Stats.
      void                       getStats( BXSSymbolTableStats &stats );
      //Dumps.
      void                       dumpTable( void(*output)(const char*) ) const;
      void                       dumpTable() const;
      void                       dumpFreeTable(void(*output)(const char *)) const;
      void                       dumpFreeTable() const;
      //Misc.
      long                       getInvalidValue( void ) const { return(mInvalidValue); }
      void                       setInvalidValue( long v ) { mInvalidValue=v; }
      BYTE                       getInvalidType( void ) const { return(mInvalidType); }
      void                       setInvalidType( BYTE v ) { mInvalidType=v; }
      void                       clear( void );

      //Cleanup.
      void                       cleanUp( void );

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      static void                setVersion( DWORD v ) { msLoadVersion=v; }
      bool                       save( BChunkWriter* chunkWriter );
      bool                       load( BChunkReader* chunkReader );
      #endif

   protected:
      //Misc.
      void                       dumpEntry( void(*output)(const char*), BXSSymbolEntry *entry, long id ) const;
      void                       dumpEntry(BXSSymbolEntry *entry, long id ) const;
      long                       getHashListLength( long hashIndex);
      long                       getHashIndex( const char *symbol ) const;
      BXSSymbolEntry*            addSymbolInternal( const char *symbol, BYTE type, long value, long id, long hashIndex );
      BXSSymbolEntry*            getEntryByIDNonConst( long symbolID );

      //Variables.
      long                       mHashTableSize;
      long                       mNumberSymbols;
      BXSSymbolEntry*            mFreeSymbols;
      BXSSymbolEntry**           mHashTable;
      long                       mInvalidValue;
      BXSSymbolEntryPointerArray mSymbolsByID;
      bool                       mCaseSensitive;
      BYTE                       mInvalidType;

      static long                mNumberToAllocateAtOnce;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};


//==============================================================================
#endif // _SYMBOLTABLE_H_
