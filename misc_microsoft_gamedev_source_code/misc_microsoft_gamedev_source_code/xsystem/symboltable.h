//==============================================================================
// symboltable.h
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BSymbolEntry;
class BSymbolTableStats;
class BChunkWriter;
class BChunkReader;

//==============================================================================
// Const declarations

//==============================================================================
class BSymbolTable
{
   public:
      // Constructors
      BSymbolTable();
      
      // Destructors
      ~BSymbolTable();

      // General Functions
      bool           init(long tag,                   // uniq ids this sym tab
                          long entrySize,             // sizeof an entry
                          long tableSize,             // hash table size
                          BStringSpace * pStringSpace);  

      // symbol functions
      BSymbolEntry * lookup(const char * name);
      BSymbolEntry * add(const char * name, long kind);

      BSymbolEntry * getHashList(const char * name) const;
      BSymbolEntry * getThisList(long index) const;

      long           getSymbolCount(void) const { return mSymbolCount; };
      long           getTableSize(void) const { return mTableSize; };

      void           clear(void);

      // game functions
      long           checksum(void) const;             
      bool           save(BChunkWriter *writer);
      bool           load(BChunkReader *reader, BStringSpace * pRestoredStringSpace);
      // FIXME: These functions below are left intact because I don't want to go rewrite all of XS's save/load
      // stuff right now (to use the chunker), but that should definitely be done at some point - pdb 7/31/00
      bool           save(FILE *pFile);
      bool           load(FILE *pFile, BStringSpace * pRestoredStringSpace);

      // support function
      void           getStats(BSymbolTableStats * pStats);
      void           dumpTable(void(*output)(const char *)) const;
      void           dumpEntry(void(*output)(const char *), BSymbolEntry * pEntry, long index) const;

   protected:
      // Functions
      virtual bool   saveExtra(BSymbolEntry * pEntry, BChunkWriter *writer) = 0;     
      virtual bool   loadExtra(BSymbolEntry * pEntry, BChunkReader *reader) = 0;
      // FIXME: These functions below are left intact because I don't want to go rewrite all of XS's save/load
      // stuff right now (to use the chunker), but that should definitely be done at some point - pdb 7/31/00
      virtual bool   saveExtra(BSymbolEntry * pEntry, FILE *file) = 0;     
      virtual bool   loadExtra(BSymbolEntry * pEntry, FILE *file) = 0;
      virtual long   checksumExtra(BSymbolEntry * pEntry) const = 0;
      virtual void   dumpExtra(void(*output)(const char *), BSymbolEntry * pEntry) const = 0; // should include the \n

      long           listLength(long index);

      long           hash(const char * name) const;
      BSymbolEntry * add(const char * name, long kind, long hash, long nameOffset);
      void           reassignPointers(void);

      // variables
      long              mTag;             // used in writing to save file
      long              mEntrySize;       // used in allocating new entries
      long              mTableSize;       // number of entries in the table
      long              mSymbolCount;     // number of symbols in the table
      BStringSpace *    mStringSpace;     // where the names are stored
      BSymbolEntry *    mFreeSymbols;     
      BSymbolEntry **   mHashTable;
};


//==============================================================================
class BSymbolEntry
{
public:
   // the following are re-created in a load
   BSymbolEntry * mNext;
   char *         mNamePtr;       
   // the following are saved
   long           mNameOffset;      
   long           mKind;
};

//==============================================================================
class BSymbolTableStats
{
public:
   long mSize;
   long mLists;
   long mLongest;
   long mAverage;
   long mCount;
};

//==============================================================================
#endif // _SYMBOLTABLE_H_

//==============================================================================
// eof: symboltable.h
//==============================================================================

