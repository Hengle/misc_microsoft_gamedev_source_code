//==============================================================================
// symboltable.cpp
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xssymboltable.h"
#include "xsdefines.h"
#ifdef _BANG
#include "chunker.h"
#endif


//=============================================================================
// BXSSymbolEntry::msSaveVersion
//=============================================================================
const DWORD BXSSymbolEntry::msSaveVersion=1;
//=============================================================================
// BXSSymbolEntry::msLoadVersion
//=============================================================================
DWORD BXSSymbolEntry::msLoadVersion=0xFFFF;

#ifdef _BANG
//=============================================================================
// BXSSymbolEntry::writeVersion
//=============================================================================
bool BXSSymbolEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4416); blogerror("BXSSymbolEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xq"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4417); blogerror("BXSSymbolEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSymbolEntry::readVersion
//=============================================================================
bool BXSSymbolEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4418); blogerror("BXSSymbolEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xq"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4419); blogerror("BXSSymbolEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSymbolEntry::save
//=============================================================================
bool BXSSymbolEntry::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xr"), mainHandle);
   if(!result)
   {
      {setBlogError(4420); blogerror("BXSSymbolEntry::save -- error writing tag.");}
      return(false);
   }

   //Symbol.
   long symbolLength=0;
   if (mSymbol != NULL)
      symbolLength=strlen(mSymbol);
   CHUNKWRITESAFE(chunkWriter, Long, symbolLength);
   if (symbolLength > 0)
      CHUNKWRITETAGGEDARRAYSAFE(chunkWriter, Char, BCHUNKTAG("xs"), symbolLength+1, mSymbol);
   //Type.
   CHUNKWRITESAFE(chunkWriter, BYTE, mType);
   //Value.
   CHUNKWRITESAFE(chunkWriter, Long, mValue);
   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //DO NOT SAVE mNext.

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4421); blogerror("BXSSymbolEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSymbolEntry::load
//=============================================================================
bool BXSSymbolEntry::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xr"));
   if(!result)
   {
      {setBlogError(4422); blogerror("BXSSymbolEntry::load -- error reading tag.");}
      return(false);
   }

   //Symbol.
   long symbolLength=0;
   CHUNKREADSAFE(chunkReader, Long, symbolLength);
   if (symbolLength > 0)
   {
      mSymbol=new char[symbolLength+1];
      if (mSymbol == NULL)
      {
         {setBlogError(4423); blogerror("BXSSymbolEntry::load -- failed to allocate mSymbol.");}
         return(false);
      }
      long readCount=0;
      CHUNKREADTAGGEDARRAYSAFE(chunkReader, Char, BCHUNKTAG("xs"), &readCount, mSymbol, symbolLength+1);
   }
   else
      mSymbol=NULL;
   //Type.
   if (msLoadVersion >= 1)
      CHUNKREADSAFE(chunkReader, BYTE, mType);
   else
   {
      long temp;
      CHUNKREADSAFE(chunkReader, Long, temp);
      mType=(BYTE)temp;
   }
   //Value.
   CHUNKREADSAFE(chunkReader, Long, mValue);
   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //DO NOT SAVE mNext.

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xr"));
   if(!result)
   {
      {setBlogError(4424); blogerror("BXSSymbolEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif



//=============================================================================
// BXSSymbolTable::msSaveVersion
//=============================================================================
const DWORD BXSSymbolTable::msSaveVersion=1;
//=============================================================================
// BXSSymbolTable::msLoadVersion
//=============================================================================
DWORD BXSSymbolTable::msLoadVersion=0xFFFF;
//=============================================================================
// BXSSymbolTable::mNumberToAllocateAtOnce
//=============================================================================
long BXSSymbolTable::mNumberToAllocateAtOnce=16;


//==============================================================================
// BXSSymbolTable::BXSSymbolTable
//==============================================================================
BXSSymbolTable::BXSSymbolTable(void) :
   mHashTableSize(0),
   mNumberSymbols(0),
   mFreeSymbols(NULL),
   mHashTable(NULL),
   mCaseSensitive(true),
   mInvalidValue(-1),
   mInvalidType(BXSBinary::cInvalidSymbol)
   //mSymbolsByID doesn't need any ctor args.
{
}

//==============================================================================
// BXSSymbolTable::~BXSSymbolTable
//==============================================================================
BXSSymbolTable::~BXSSymbolTable(void)
{
   cleanUp();
}

//==============================================================================
// BXSSymbolTable::initialize
//==============================================================================
bool BXSSymbolTable::initialize(long hashTableSize, bool caseSensitive, long invalidValue, BYTE invalidType)
{
   //If we're already initialized, skip.  It's the caller's responsibility to 
   //do this properly.
   if (mHashTable != NULL)
      return(true);
   BASSERT(hashTableSize > 0);

   mHashTable=(BXSSymbolEntry**)new BXSSymbolEntry*[hashTableSize];
   if (mHashTable == NULL)
      return(false);

   memset((void*)mHashTable, 0, sizeof(new BXSSymbolEntry*)*hashTableSize);
   mHashTableSize=hashTableSize;
   mCaseSensitive=caseSensitive;
   mInvalidValue=invalidValue;
   mInvalidType=invalidType;
   return(true);
}

//==============================================================================
// BXSSymbolTable::getSymbolByValue
//==============================================================================
const char* BXSSymbolTable::getSymbolByValue(long value) const
{
   //Simple O(N) traversal (for now).
   for (long i=0; i < mHashTableSize; i++)
   { 
      BXSSymbolEntry *entry=mHashTable[i];
      while (entry != NULL)
      {
         if (entry->mValue == value)
            return(entry->mSymbol);
         entry=entry->mNext;
      }
   }
   return(NULL);
}

//==============================================================================
// BXSSymbolTable::getSymbolByID
//==============================================================================
const char* BXSSymbolTable::getSymbolByID(long id) const
{
   if ((id < 0) || (id >= mSymbolsByID.getNumber()) )
      return(NULL);
   return(mSymbolsByID[id]->mSymbol);
}

//==============================================================================
// BXSSymbolTable::isDefined
//==============================================================================
bool BXSSymbolTable::isDefined(const char *symbol)
{
   long v=getValueBySymbol(symbol);
   if (v == mInvalidValue)
      return(false);
   return(true);
}

//==============================================================================
// BXSSymbolTable::getValueBySymbol
//==============================================================================
long BXSSymbolTable::getValueBySymbol(const char *symbol) const
{
   if (symbol == NULL)
      return(mInvalidValue);
   //Grab the hash list and then just rip through it.
   BXSSymbolEntry *entry=getHashList(symbol);
   while (entry)
   {
      if (entry->mSymbol != NULL)
      {
         if ((mCaseSensitive == true) && (strcmp(entry->mSymbol, symbol) == 0))
            return(entry->mValue);
         if ((mCaseSensitive == false) && (stricmp(entry->mSymbol, symbol) == 0))
            return(entry->mValue);
      }
      entry=entry->mNext;
   }
   return(mInvalidValue);
}

//==============================================================================
// BXSSymbolTable::getValueByID
//==============================================================================
long BXSSymbolTable::getValueByID(long id) const
{
   if ((id < 0) || (id >= mSymbolsByID.getNumber()) )
      return(mInvalidValue);
   return(mSymbolsByID[id]->mValue);
}

//==============================================================================
// BXSSymbolTable::getTypeBySymbol
//==============================================================================
BYTE BXSSymbolTable::getTypeBySymbol(const char *symbol) const
{
   if (symbol == NULL)
      return(mInvalidType);
   //Grab the hash list and then just rip through it.
   BXSSymbolEntry *entry=getHashList(symbol);
   while (entry)
   {
      if (entry->mSymbol != NULL)
      {
         if ((mCaseSensitive == true) && (strcmp(entry->mSymbol, symbol) == 0))
            return(entry->mType);
         if ((mCaseSensitive == false) && (stricmp(entry->mSymbol, symbol) == 0))
            return(entry->mType);
      }
      entry=entry->mNext;
   }
   return(mInvalidType);
}

//==============================================================================
// BXSSymbolTable::getTypeByID
//==============================================================================
BYTE BXSSymbolTable::getTypeByID(long id) const
{
   if ((id < 0) || (id >= mSymbolsByID.getNumber()) )
      return(mInvalidType);
   return(mSymbolsByID[id]->mType);
}

//==============================================================================
// BXSSymbolTable::getIDBySymbol
//==============================================================================
long BXSSymbolTable::getIDBySymbol(const char *symbol) const
{
   if (symbol == NULL)
      return(-1);
   //Grab the hash list and then just rip through it.
   BXSSymbolEntry *entry=getHashList(symbol);
   while (entry)
   {
      if (entry->mSymbol != NULL)
      {
         if ((mCaseSensitive == true) && (strcmp(entry->mSymbol, symbol) == 0))
            return(entry->mID);
         if ((mCaseSensitive == false) && (stricmp(entry->mSymbol, symbol) == 0))
            return(entry->mID);
      }
      entry=entry->mNext;
   }
   return(-1);
}

//==============================================================================
// BXSSymbolTable::getIDBySymbolAndType
//==============================================================================
long BXSSymbolTable::getIDBySymbolAndType(const char *symbol, BYTE type) const
{
   if (symbol == NULL)
      return(-1);
   //Grab the hash list and then just rip through it.
   BXSSymbolEntry *entry=getHashList(symbol);
   while (entry)
   {
      if (entry->mType != type)
      {
         entry=entry->mNext;
         continue;
      }
      if (entry->mSymbol != NULL)
      {
         if ((mCaseSensitive == true) && (strcmp(entry->mSymbol, symbol) == 0))
            return(entry->mID);
         if ((mCaseSensitive == false) && (stricmp(entry->mSymbol, symbol) == 0))
            return(entry->mID);
      }
      entry=entry->mNext;
   }
   return(-1);
}

//==============================================================================
// BXSSymbolTable::getEntryBySymbol
//==============================================================================
const BXSSymbolEntry* BXSSymbolTable::getEntryBySymbol(const char *symbol) const
{
   if (symbol == NULL)
      return(NULL);
   //Grab the hash list and then just rip through it.
   BXSSymbolEntry *entry=getHashList(symbol);
   while (entry)
   {
      if (entry->mSymbol != NULL)
      {
         if ((mCaseSensitive == true) && (strcmp(entry->mSymbol, symbol) == 0))
            return(entry);
         if ((mCaseSensitive == false) && (stricmp(entry->mSymbol, symbol) == 0))
            return(entry);
      }
      entry=entry->mNext;
   }
   return(NULL);
}

//==============================================================================
// BXSSymbolTable::getEntryByID
//==============================================================================
const BXSSymbolEntry* BXSSymbolTable::getEntryByID(long id) const
{
   if ((id < 0) || (id >= mSymbolsByID.getNumber()) )
      return(NULL);
   return(mSymbolsByID[id]);
}

//==============================================================================
// BXSSymbolTable::addSymbol
//==============================================================================
long BXSSymbolTable::addSymbol(const char *symbol, BYTE type, long value)
{
   long hashIndex=getHashIndex(symbol);
   if ((hashIndex < 0) || (hashIndex >= mHashTableSize))
      return(-1);
//-- FIXING PREFIX BUG ID 8001
   const BXSSymbolEntry* e=addSymbolInternal(symbol, type, value, mNumberSymbols, hashIndex);
//--
   if (e == NULL)
      return(-1);
   return(e->mID);
}

//==============================================================================
// BXSSymbolTable::addSymbol
//==============================================================================
long BXSSymbolTable::addSymbol(const char *symbol, BYTE type, long value, long id)
{
   long hashIndex=getHashIndex(symbol);
   if ((hashIndex < 0) || (hashIndex >= mHashTableSize))
      return(-1);
//-- FIXING PREFIX BUG ID 8002
   const BXSSymbolEntry* e=addSymbolInternal(symbol, type, value, id, hashIndex);
//--
   if (e == NULL)
      return(-1);
   return(e->mID);
}

//==============================================================================
// BXSSymbolTable::setSymbolValue
//==============================================================================
bool BXSSymbolTable::setSymbolValue(long symbolID, long value)
{
   BXSSymbolEntry *e=getEntryByIDNonConst(symbolID);
   if (e == NULL)
      return(false);
   e->mValue=value;
   return(true);
}

//==============================================================================
// BXSSymbolTable::getHashList 
//==============================================================================
BXSSymbolEntry* BXSSymbolTable::getHashList(const char *symbol) const
{
   if (mHashTable == NULL)
      return(NULL);

   //Get the hash list index.
   long hashIndex=getHashIndex(symbol);
   //Double check:)
   if ((hashIndex < 0) || (hashIndex >= mHashTableSize))
      return(NULL);
   BXSSymbolEntry *entry=mHashTable[hashIndex];
   return(entry);
}

//==============================================================================
// BXSSymbolTable::getHashList 
//==============================================================================
BXSSymbolEntry * BXSSymbolTable::getHashList(long hashIndex) const
{
   if (mHashTable == NULL)
      return(NULL);
   //Bomb check.
   if ((hashIndex < 0) || (hashIndex >= mHashTableSize))
      return(NULL);
   BXSSymbolEntry *entry=mHashTable[hashIndex];
   return(entry);
}

//==============================================================================
// BXSSymbolTable::getStats
//==============================================================================
void BXSSymbolTable::getStats(BXSSymbolTableStats &stats)
{
   if (mHashTable == NULL)
      return;
   stats.mLists=0;
   stats.mCount=0;
   stats.mLongest=0;
   stats.mSize=mHashTableSize;
   stats.mAverage=0;

   for (long i=0; i < mHashTableSize; i++)
   {
      long hashEntries=getHashListLength(i);
      if (hashEntries > 0)
      {
         stats.mLists++;
         stats.mCount+=hashEntries;
         if (hashEntries > stats.mLongest)
            stats.mLongest=hashEntries;
      }
   }

   if (stats.mLists > 0)
      stats.mAverage=stats.mCount/stats.mLists;
}

//==============================================================================
// BXSSymbolTable::dumpTable
//==============================================================================
void BXSSymbolTable::dumpTable(void(*output)(const char *)) const
{
   char buffer132[132];
   bsnprintf(buffer132, sizeof(buffer132), "BXSSymbolTable::dumpTable:\n");
   (output)(buffer132);
   for (long i=0; i < mHashTableSize; i++)
   { 
      BXSSymbolEntry *entry=mHashTable[i];
      while (entry != NULL)
      {
         dumpEntry(output, entry, i);
         entry=entry->mNext;
      }
   }
}
//==============================================================================
// BXSSymbolTable::dumpTable
//==============================================================================
void BXSSymbolTable::dumpTable() const
{
   char buffer132[132];
   bsnprintf(buffer132, sizeof(buffer132), "BXSSymbolTable::dumpTable:\n");
   blogtrace(buffer132);
   long j = 0;
   for (long i=0; i < mHashTableSize; i++)
   { 
      BXSSymbolEntry *entry=mHashTable[i];
      while (entry != NULL)
      {
         dumpEntry(entry, j);
         entry=entry->mNext;
         j++;
      }
   }
}

//==============================================================================
// BXSSymbolTable::dumpFreeTable
//==============================================================================
void BXSSymbolTable::dumpFreeTable() const
{
   char buffer132[132];
   bsnprintf(buffer132, sizeof(buffer132), "BXSSymbolTable::dumpTable:\n");
   blogtrace(buffer132);

   BXSSymbolEntry *entry=mFreeSymbols;
   long i = 0;
   while (entry != NULL)
   {
      dumpEntry(entry, i);
      i++;
      entry=entry->mNext;
   }

}
//==============================================================================
// BXSSymbolTable::clear
//==============================================================================
void BXSSymbolTable::clear(void)
{
   if (mHashTable != NULL)
   {
      //Clear the hash table.
      for (long i=0; i < mHashTableSize; i++)
      { 
         BXSSymbolEntry *entry=mHashTable[i];
         while (entry != NULL)
         {
            BXSSymbolEntry *next=entry->mNext;
            //Delete the symbol (since we realloc it on the add).
            if (entry->mSymbol != NULL)
            {
               delete entry->mSymbol;
               entry->mSymbol=NULL;
            }
            //Put it on the free list.
            entry->mNext=mFreeSymbols;
            mFreeSymbols=entry;
            //Move to the next ptr.
            entry=next;
         }

         mHashTable[i]=NULL;
      }

      delete [] mHashTable;
      mHashTable=NULL;
   }
   mNumberSymbols=0;

   //Nuke anything in the ID lookup table.
   mSymbolsByID.setNumber(0);
}

//==============================================================================
// BXSSymbolTable::cleanUp
//==============================================================================
void BXSSymbolTable::cleanUp(void)
{
   //Move all of the symbols to the free symbol list.
   clear();

   //Delete the symbols on the free list.
   BXSSymbolEntry *entry=mFreeSymbols;
   long n = 0;
   //blogtrace("Cleaning up free symbol table...");
   while (entry != NULL)
   {
      BXSSymbolEntry *next=entry->mNext;
      entry->mNext = NULL;
      if (entry->mSymbol != NULL)
      {
         delete entry->mSymbol;
         entry->mSymbol = NULL;
      }
      //dumpEntry(entry, n);
      delete entry;
      entry=next;
      n++;
   }
   mFreeSymbols=NULL;

   //Delete the hash table.
   BDELETE(mHashTable);
}

#ifdef _BANG
//=============================================================================
// BXSSymbolTable::writeVersion
//=============================================================================
bool BXSSymbolTable::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4425); blogerror("BXSSymbolTable::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xo"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4426); blogerror("BXSSymbolTable::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSymbolTable::readVersion
//=============================================================================
bool BXSSymbolTable::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4427); blogerror("BXSSymbolTable::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xo"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4428); blogerror("BXSSymbolTable::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSymbolTable::save
//=============================================================================
bool BXSSymbolTable::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xp"), mainHandle);
   if(!result)
   {
      {setBlogError(4429); blogerror("BXSSymbolTable::save -- error writing tag.");}
      return(false);
   }

   //Hash table size.
   CHUNKWRITESAFE(chunkWriter, Long, mHashTableSize);
   //Number symbols.
   CHUNKWRITESAFE(chunkWriter, Long, mNumberSymbols);
   //DO NOT SAVE mFreeSymbols.
   //Case sensitive.
   CHUNKWRITESAFE(chunkWriter, Bool, mCaseSensitive);
   //Invalid value.
   CHUNKWRITESAFE(chunkWriter, Long, mInvalidValue);
   //Invalid type.
   CHUNKWRITESAFE(chunkWriter, BYTE, mInvalidType);
   //Hash table.
   for (long i=0; i < mHashTableSize; i++)
   { 
      //Count the number of entries in this row.
      long entryCount=0;
      BXSSymbolEntry *entry=mHashTable[i];
      while (entry != NULL)
      {
         entryCount++;
         entry=entry->mNext;
      }
      //Save out the count.
      CHUNKWRITESAFE(chunkWriter, Long, entryCount);
      //Save out each entry now.
      entry=mHashTable[i];
      while (entry != NULL)
      {
         if (entry->save(chunkWriter) == false)
         {
            {setBlogError(4430); blogerror("BXSSymbolTable::save -- failed to save a symbol entry.");}
            return(false);
         }
         entry=entry->mNext;
      }
   }
   //DO NOT SAVE mSymbolsByID.  They will be rebuilt as the entries are read-in.

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4431); blogerror("BXSSymbolTable::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSymbolTable::load
//=============================================================================
bool BXSSymbolTable::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
    //Cleanup.
   cleanUp();

  //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xp"));
   if(!result)
   {
      {setBlogError(4432); blogerror("BXSSymbolTable::load -- error reading tag.");}
      return(false);
   }

   //Hash table size.
   CHUNKREADSAFE(chunkReader, Long, mHashTableSize);
   //Number symbols.
   CHUNKREADSAFE(chunkReader, Long, mNumberSymbols);
   //Setup space for mSymbolsByID.
   if (mSymbolsByID.setNumber(mNumberSymbols) == false)
   {
      {setBlogError(4433); blogerror("BXSSymbolTable::load -- failed to allocate %d mSymbolsByID.", mNumberSymbols);}
      return(false);
   }
   for (long i=0; i < mSymbolsByID.getNumber(); i++)
      mSymbolsByID[i]=NULL;
   //DO NOT SAVE mFreeSymbols.
   //Case sensitive.
   CHUNKREADSAFE(chunkReader, Bool, mCaseSensitive);
   //Invalid value.
   CHUNKREADSAFE(chunkReader, Long, mInvalidValue);
   //Invalid type.
   if (msLoadVersion >= 1)
      CHUNKREADSAFE(chunkReader, BYTE, mInvalidType);
   else
   {
      long temp;
      CHUNKREADSAFE(chunkReader, Long, temp);
      mInvalidType=(BYTE)temp;
   }
   //Init to allocate the hash table.
   if (initialize(mHashTableSize, mCaseSensitive, mInvalidValue, mInvalidType) == false)
   {
      {setBlogError(4434); blogerror("BXSSymbolTable::load -- failed to initialize mHashTable.");}
      return(false);
   }
   //Hash table.
   for (long i=0; i < mHashTableSize; i++)
   { 
      //Get the count of the number of entries in this row.
      long entryCount=0;
      CHUNKREADSAFE(chunkReader, Long, entryCount);
      for (long j=0; j < entryCount; j++)
      {
         //Allocate an entry.
         BXSSymbolEntry *entry=new BXSSymbolEntry;
         if (entry == NULL)
         {
            {setBlogError(4435); blogerror("BXSSymbolTable::load -- failed to allocatge a symbol entry.");}
            return(false);
         }
         //Load it.
         if (entry->load(chunkReader) == false)
         {
            {setBlogError(4436); blogerror("BXSSymbolTable::load -- failed to load a symbol entry.");}
            return(false);
         }
         //Add it.
         entry->mNext=mHashTable[i];
         mHashTable[i]=entry;

         //Link this entry into it's ID spot.
         if ((entry->mID < 0) || (entry->mID >= mSymbolsByID.getNumber()) || (mSymbolsByID[entry->mID] != NULL))
         {
            BASSERT(0);
            {setBlogError(4437); blogerror("BXSSymbolTable::load -- failed to insert entry into mSymbolsByID.");}
            return(false);
         }
         mSymbolsByID[entry->mID]=entry;
      }
   }

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xp"));
   if(!result)
   {
      {setBlogError(4438); blogerror("BXSSymbolTable::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif

//==============================================================================
// BXSSymbolTable::dumpEntry
//==============================================================================
void BXSSymbolTable::dumpEntry(void(*output)(const char *), BXSSymbolEntry *entry, long id) const
{
   //note: the linefeed is put out in dumpExtra
   char buffer132[132];
   if (entry->mSymbol != NULL)
      bsnprintf(buffer132, sizeof(buffer132), "\t[%3d] %18s value=%d.", id, entry->mSymbol, entry->mValue);
   else
      bsnprintf(buffer132, sizeof(buffer132), "\t[%3d] (NULL SYMBOL) value=%d.", id, entry->mValue);
   (output)(buffer132);
}

//==============================================================================
// BXSSymbolTable::dumpEntry
//==============================================================================
void BXSSymbolTable::dumpEntry(BXSSymbolEntry *entry, long id) const
{
   //note: the linefeed is put out in dumpExtra
   char buffer132[132];
   if (entry->mSymbol != NULL)
      bsnprintf(buffer132, sizeof(buffer132), "\t[%3d] %x, %18s value=%d.", id, entry, entry->mSymbol, entry->mValue);
   else
      bsnprintf(buffer132, sizeof(buffer132), "\t[%3d] %x, (NULL SYMBOL) value=%d.", id, entry, entry->mValue);
   blogtrace(buffer132);

}

//==============================================================================
// BXSSymbolTable::getHashListLength
//==============================================================================
long BXSSymbolTable::getHashListLength(long hashIndex)
{
   if ((mHashTable == NULL) || (hashIndex < 0) || (hashIndex >= mHashTableSize))
      return(0);

   long rVal=0;
   BXSSymbolEntry *entry=mHashTable[hashIndex];
   while (entry)
   {
      rVal++;
      entry=entry->mNext;
   }

   return(rVal);
}

//==============================================================================
// BXSSymbolTable::getHashIndex
//==============================================================================
long BXSSymbolTable::getHashIndex(const char *symbol) const
{
   //DCP 07/20/00:  This could be slow, but I'm leaving Doug's code in place for now.
   //If we don't have a symbol, put it on the first list.
   if (symbol == NULL)
      return(0);

   long result=0;
   long count=strlen(symbol);

   //Convert to uppercase if we're case insensitive.
   unsigned char *pTemp=(unsigned char *)symbol;
   char uppercasesymbol[256];
   if (mCaseSensitive == false)
   {
      long i;
      for (i=0; (i < count) && (i < 256); i++)
         uppercasesymbol[i]=(char)toupper(symbol[i]);
      uppercasesymbol[i]='\0';
      pTemp=(unsigned char *)uppercasesymbol;
   }

   for ( ; count >= 4; pTemp += 4, count -= 4)
   {
      long temp=*(long *)(pTemp);
      #ifdef _BIGENDIAN_
      OutputDebugString("Swap the bytes to little endian first\n");
      #endif
      result^=temp;
   }
   for( ; count > 0 ; pTemp++, count-- )   // get the remaining bytes
   {
      unsigned char uc=*pTemp;
      result+=(long)uc;
   }

   //Finally, mod the result into the right size and return it.
   result%=mHashTableSize;
   return(result);
}

//==============================================================================
// BXSSymbolTable::addSymbol
//==============================================================================
BXSSymbolEntry* BXSSymbolTable::addSymbolInternal(const char *symbol, BYTE type, long value,
   long id, long hashIndex)
{
   if ((mHashTable == NULL) || (hashIndex < 0) || (hashIndex >= mHashTableSize) || (symbol == NULL))
      return(NULL);
   if (id < mNumberSymbols)
   {
      BASSERT(0);
      return(NULL);
   }

   //Allocate some free symbols if there aren't any.
   if (mFreeSymbols == NULL) 
   {
      for (long i=0 ; i < mNumberToAllocateAtOnce; i++)
      {
         //Allocate an entry.
         BXSSymbolEntry *newEntry=new BXSSymbolEntry;
         if (newEntry == NULL)
            break;
         //Init.
         newEntry->mSymbol=NULL;
         newEntry->mType=BXSBinary::cInvalidSymbol;
         newEntry->mValue=-1;
         newEntry->mID=-1;
         //Add it to the free list.
         newEntry->mNext=mFreeSymbols;
         mFreeSymbols=newEntry;
      }
   }
   if (mFreeSymbols == NULL)
      return(NULL);

   //Make sure we can inc the ID table.
   if (mSymbolsByID.setNumber(mNumberSymbols+1) == false)
      return(NULL);

   //Get one of the free symbols.
   BXSSymbolEntry *entry=mFreeSymbols;
   mFreeSymbols=mFreeSymbols->mNext;

   //Fill it in.
   long symbolLength=strlen(symbol);
   entry->mSymbol=new char[symbolLength+1];
   if (entry->mSymbol == NULL)
   {
      BASSERT(0);
      //Reset the ID table.
      mSymbolsByID.setNumber(mNumberSymbols);
      //Put it back on the free list (even though we're prolly f'ed).
      entry->mNext=mFreeSymbols;
      mFreeSymbols=entry;
      return(NULL);
   }
   StringCchCopyA(entry->mSymbol, symbolLength+1, symbol);
   entry->mValue=value;
   entry->mType=type;
   entry->mID=id;

   //Hook it in to its hash list.
   entry->mNext=mHashTable[hashIndex];
   mHashTable[hashIndex]=entry;

   //Hook it in to the ID table.
   mSymbolsByID[mNumberSymbols]=entry;

   //Done.
   mNumberSymbols++;
   return(entry);
}

//==============================================================================
// BXSSymbolTable::getEntryByIDNonConst
//==============================================================================
BXSSymbolEntry* BXSSymbolTable::getEntryByIDNonConst(long symbolID)
{
   if ((symbolID < 0) || (symbolID >= mSymbolsByID.getNumber()) )
      return(NULL);
   return(mSymbolsByID[symbolID]);
}

