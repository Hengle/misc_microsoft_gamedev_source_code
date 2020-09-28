//==============================================================================
// symboltable.cpp
//
// Copyright (c) 1999, Ensemble Studios
//
// OPTIMIZATION: right now a string is added each time a symbol is added - even
// if the string is already in the table.  It might be worth optimizing this if 
// there will be alot of overloading. 
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "dynamicspace.h"
#include "stringspace.h"
#include "symboltable.h"
#include "chunker.h"

//==============================================================================
// Defines
const long cAllocCount = 16;        // we allocate a group of symbols together
                                    // so they are allocated near each other.

struct SaveHeader
{
   long tag;
   long checksum;
   long count;
};

struct SaveData
{
   long kind;
   long nameOffset;
};

#define MYASSERT(e)  BASSERT(e)

//==============================================================================
// BSymbolTable::BSymbolTable
//==============================================================================
BSymbolTable::BSymbolTable(void) :
   mTag(0),
   mEntrySize(0),
   mTableSize(0),
   mSymbolCount(0),
   mStringSpace(NULL),
   mFreeSymbols(NULL),
   mHashTable(NULL)
{
} // BSymbolTable::BSymbolTable


//==============================================================================
// BSymbolTable::~BSymbolTable
//
// note: the string space was not allocated by this object, so it should not
// be deleted by this object.
//==============================================================================
BSymbolTable::~BSymbolTable(void)
{
   BSymbolEntry * pEntry;
   BSymbolEntry * next;

   // move all allocated symbols to the free list
   clear();

   // clear, but don't delete the string space - we didn't alloc it
   mStringSpace = NULL;

   // delete the symbols on the free list
   pEntry = mFreeSymbols;
   while (pEntry)
   {
      // save the next ptr
      next = pEntry->mNext;

      // delete it
      delete pEntry;

      // advance to the next
      pEntry = next;
   }
   mFreeSymbols = NULL;

   // now delete the hash table.
   if (mHashTable)
   {
      delete mHashTable;
      mHashTable = NULL;
   }
} // BSymbolTable::~BSymbolTable


//==============================================================================
// BSymbolTable::init
//==============================================================================
bool BSymbolTable::init(
   long tag, 
   long entrySize, 
   long tableSize,
   BStringSpace * pStringSpace)
{
   // Sanity.
   if(entrySize <= 0 || tableSize <= 0)
   {
      BFAIL("bad params to BSymbolTable::init");
      return(false);
   }
   
   bool bResult = false;
   MYASSERT(mTag == 0);      // should only call this once
   MYASSERT(tag != 0);

   mHashTable = (BSymbolEntry **) new BSymbolEntry * [tableSize];
   if (mHashTable)
   {
      memset((void *) mHashTable, 0, sizeof(new BSymbolEntry *) * tableSize);

      mTag = tag;
      mEntrySize = entrySize;
      mTableSize = tableSize;
      mStringSpace = pStringSpace;

      bResult = true;
   }

   return bResult;
} // BSymbolTable::init


//==============================================================================
// BSymbolTable::lookupAnyKind - lookup matching any kind
//==============================================================================
BSymbolEntry * BSymbolTable::lookup(
   const char * name)
{
   BSymbolEntry * pEntry;

   pEntry = getHashList(name);
   while (pEntry) 
   {
      if (strcmp(pEntry->mNamePtr,name)==0)
         break;
      pEntry = pEntry->mNext;
   }

   return pEntry;
} // BSymbolTable::lookupAnyKind


//==============================================================================
// BSymbolTable::getHashList 
//==============================================================================
BSymbolEntry * BSymbolTable::getHashList(
   const char * name) const
{
   BSymbolEntry * pEntry = NULL;
   long hash;

   if (mHashTable)
   {
      // get the list
      hash = BSymbolTable::hash(name);
      MYASSERT(hash < mTableSize);
      pEntry = mHashTable[hash];
   }

   return pEntry;
} // BSymbolTable::getHashList


//==============================================================================
// BSymbolTable::getThisList 
//==============================================================================
BSymbolEntry * BSymbolTable::getThisList(
   long index) const
{
   BSymbolEntry * pEntry = NULL;

   if (mHashTable)
   {
      // get the list
      MYASSERT(index < mTableSize);
      pEntry = mHashTable[index];
   }

   return pEntry;
} // BSymbolTable::getThisList


//==============================================================================
// BSymbolTable::clear
//==============================================================================
void BSymbolTable::clear(void)
{
   BSymbolEntry * pEntry;
   BSymbolEntry * next;
   long i;

   for (i = 0 ; i < mTableSize ; i++)
   { 
      pEntry = mHashTable[i];
      while (pEntry)
      {
         // save the next ptr
         next = pEntry->mNext;

         // put it on the free list
         pEntry->mNext = mFreeSymbols;
         mFreeSymbols = pEntry;

         // move to the next ptr
         pEntry = next;
      }
      mHashTable[i] = NULL;
   }
   mSymbolCount = 0;
} // BSymbolTable::clear


//==============================================================================
// BSymbolTable::add
//==============================================================================
BSymbolEntry * BSymbolTable::add(
   const char * name, 
   long kind)
{
   long nameOffset;
   long hash;
   bool moved = false;
   
   nameOffset = mStringSpace->add(&moved, name);
   MYASSERT(nameOffset >= 0);

   if (moved)
   {
      // new pointer is fine, but all the others need to be re-assigned
      reassignPointers();
   }

   hash = BSymbolTable::hash(name);
   MYASSERT(hash < mTableSize);

   return add(name, kind, hash, nameOffset);
} // BSymbolTable::add


//==============================================================================
// BSymbolTable::checksum
//==============================================================================
long BSymbolTable::checksum(void) const
{
   long           lResult;
   BSymbolEntry * pEntry;
   long i;
   long sum;

   lResult = ~mTag;
   for (i = 0 ; i < mTableSize ; i++)
   { 
      pEntry = mHashTable[i];

      if (pEntry)
      {
         // Since entries are always added to the beginning of the list, lists will
         // change when the table is saved and then restored.  For example, the
         // list A->B->C will be written as A,B,C then read back and inserted in 
         // the list in that order building the list C->B->A.  Thus the checksum
         // can't be dependent on the order of data in lists.
         sum = 0;
         while (pEntry)
         {
            sum += (pEntry->mKind * pEntry->mNameOffset) + i;
            sum += checksumExtra(pEntry); // add in the use specific checksum

            // move to the next ptr
            pEntry = pEntry->mNext;
         }

         // add it into the checksum
         lResult ^= sum;
      }
   }

   return lResult;
} // BSymbolTable::checksum

//==============================================================================
// BSymbolTable::save
//==============================================================================
bool BSymbolTable::save(
   FILE *pFile)
{
   bool             bResult = false;
   BSymbolEntry *   pEntry;
   SaveHeader       saveHeader;
   SaveData         saveData;
   int              written;
   int              i;

   // write out the header
   saveHeader.tag       = mTag;
   saveHeader.checksum  = checksum();
   saveHeader.count     = mSymbolCount;
   written = fwrite(&saveHeader, sizeof(saveHeader), 1, pFile);
   if (written != 1)
   {
      blog("failed saving SymbolTable(%x) save header", mTag);
      goto bail;
   }

   // now write out the records
   for (i = 0 ; i < mTableSize ; i++)
   { 
      pEntry = mHashTable[i];
      while (pEntry)
      {
         // save it
         saveData.kind = pEntry->mKind;
         saveData.nameOffset = pEntry->mNameOffset;
         written = fwrite(&saveData, sizeof(saveData), 1, pFile);
         if (written != 1)
         {
            {setBlogError(3939); blogerror("failed writing saveData for SymbolTable(%x)", mTag);}
            goto bail;
         }
         if (! saveExtra(pEntry, pFile))
         {
            {setBlogError(3940); blogerror("failed writing extra info for SymbolTable(%x)", mTag);}
            goto bail;
         }

         // move to the next ptr
         pEntry = pEntry->mNext;
      }
   }

   // all don
   bResult = true;

bail:
   return bResult;
} // BSymbolTable::save


//==============================================================================
// BSymbolTable::load
//==============================================================================
bool BSymbolTable::load(
   FILE * pFile,
   BStringSpace * pRestoredStringSpace)
{
   bool             bResult = false;
   BSymbolEntry *   pEntry;
   SaveHeader       saveHeader;
   SaveData         saveData;
   size_t           read;
   long             i;
   const char *     namePtr;

   MYASSERT(mTag != 0);
   
   // Don't want to use any old data
   clear();

   // Put in the new string space
   mStringSpace = pRestoredStringSpace;

   // read the header
   read = fread(&saveHeader, sizeof(saveHeader), 1, pFile);
   if (read != 1)
   {
      {setBlogError(3941); blogerror("failed loading SymbolTable(%x) save header - reading the header", mTag);}
      goto bail;
   }
   if (saveHeader.tag != mTag)
   {
      {setBlogError(3942); blogerror("failed loading SymbolTable(%x) save header - tag error", mTag);}
      goto bail;
   }
   
   // read the data
   for (i = 0; i < saveHeader.count ; i++)
   {
      read = fread(&saveData, sizeof(saveData), 1, pFile);
      if (read != 1)
      {
         blog("saveData");
         break;
      }
      namePtr = mStringSpace->getString(saveData.nameOffset);
      if (namePtr == NULL)
      {
         blog("namePtr");
         break;
      }
      pEntry = add(namePtr, saveData.kind, hash(namePtr), saveData.nameOffset);
      if (pEntry == NULL)
      {
         blog("add"); 
         break;
      }
      if (! loadExtra(pEntry, pFile))
      {
         blog("extra");
         break;
      }
   }
   if (i < saveHeader.count)
   {
      {setBlogError(3943); blogerror("failed loading SymbolTable(%x)", mTag);}
      goto bail;
   }

   // check the checksum
   if (saveHeader.checksum != checksum())
   {
      {setBlogError(3944); blogerror("failed loading SymbolTable(%x) - checksum on the new data", mTag);}
      goto bail;
   }

   bResult = true;

bail:
   return bResult;
} // BSymbolTable::load


          
//==============================================================================
// BSymbolTable::save
//==============================================================================
bool BSymbolTable::save(
   BChunkWriter *writer)
{
   bool             bResult = false;
   BSymbolEntry *   pEntry;   
   int              i;

   long pos;
   long result=writer->writeTagPostSized(BCHUNKTAG("ST"), pos);
   if (!result)
   {
      {setBlogError(3945); blogerror("BSymbolTable::save-- error writing tag");}
      return(false);
   }   

   // write out the header   
   CHUNKWRITESAFE(writer, Long, mTag);
   CHUNKWRITESAFE(writer, Long, checksum());
   CHUNKWRITESAFE(writer, Long, mSymbolCount);   
   
   // now write out the records
   for (i = 0 ; i < mTableSize ; i++)
   { 
      pEntry = mHashTable[i];
      while (pEntry)
      {
         // save it         
         CHUNKWRITESAFE(writer, Long, pEntry->mKind);
         CHUNKWRITESAFE(writer, Long, pEntry->mNameOffset);                  
         if (! saveExtra(pEntry, writer))
         {
            {setBlogError(3946); blogerror("failed writing extra info for SymbolTable(%x)", mTag);}
            goto bail;
         }

         // move to the next ptr
         pEntry = pEntry->mNext;
      }
   }

   
   //Finish chunk.   
   result=writer->writeSize(pos);
   if (!result)
   {
      {setBlogError(3947); blogerror("BSymbolTable::save - error with writeSize");}
      return(false);
   }   

   // all done
   bResult = true;

bail:
   return bResult;
} // BSymbolTable::save

//==============================================================================
// BSymbolTable::load
//==============================================================================
bool BSymbolTable::load(
   BChunkReader *reader,
   BStringSpace * pRestoredStringSpace)
{
   bool             bResult = false;
   BSymbolEntry *   pEntry;   
   long tag, csum, count, kind, nameOffset;   
   long             i;
   const char *     namePtr;

   MYASSERT(mTag != 0);
   
   // Don't want to use any old data
   clear();

   // Put in the new string space
   mStringSpace = pRestoredStringSpace;

   // read the header
   long result=reader->readExpectedTag(BCHUNKTAG("ST"));
   if (!result)
   {
      {setBlogError(3948); blogerror("BSymbolTable::load -- error reading tag");}
      return(false);
   }   

   // read in the header   
   CHUNKREADSAFE(reader, Long, tag);
   CHUNKREADSAFE(reader, Long, csum);
   CHUNKREADSAFE(reader, Long, count);   

   if (tag != mTag)
   {
      {setBlogError(3949); blogerror("failed loading SymbolTable(%x) save header - tag error", mTag);}
      goto bail;
   }
   
   // now write out the records
   for (i = 0 ; i < count; i++)
   {       
      // load it         
      CHUNKREADSAFE(reader, Long, kind);
      CHUNKREADSAFE(reader, Long, nameOffset);                  

      namePtr = mStringSpace->getString(nameOffset);
      if (namePtr == NULL)
      {
         blog("namePtr");
         break;
      }
      pEntry = add(namePtr, kind, hash(namePtr), nameOffset);
      if (pEntry == NULL)
      {
         blog("add"); 
         break;
      }
      if (! loadExtra(pEntry, reader))
      {
         blog("extra");
         break;
      }      
   }

   //Validate our reading of the chunk.
   result=reader->validateChunkRead(BCHUNKTAG("ST"));
   if (!result)
   {
      {setBlogError(3950); blogerror("BSymbolTable::load didn't validate chunk correctly");}
      return(false);
   }

   if (i < count)
   {
      {setBlogError(3951); blogerror("failed loading SymbolTable(%x)", mTag);}
      goto bail;
   }

   // check the checksum
   if (csum!= checksum())
   {
      {setBlogError(3952); blogerror("failed loading SymbolTable(%x) - checksum on the new data", mTag);}
      goto bail;
   }

   bResult = true;

bail:
   return bResult;
} // BSymbolTable::load


//==============================================================================
// BSymbolTable::getStats
//==============================================================================
void BSymbolTable::getStats(
   BSymbolTableStats * pStats)
{
   long numListsInUse;
   long hashEntries; 
   long longest;
   long totalEntries;
   long i;
   long avg;
      
   numListsInUse = 0;
   longest = 0;
   totalEntries = 0;
   MYASSERT(mHashTable);

   for (i = 0; i < mTableSize; i++)
   {
      hashEntries = listLength(i);
      if (hashEntries)
      {
         numListsInUse++;
         totalEntries += hashEntries;
         if (hashEntries > longest) 
            longest = hashEntries;
      }
   }

   if (numListsInUse)
   {
      avg = totalEntries/numListsInUse;
   }
   else
   {
      avg = 0;
   }

   pStats->mAverage = avg;
   pStats->mCount = totalEntries;
   pStats->mLists = numListsInUse;
   pStats->mLongest = longest;
   pStats->mSize = mTableSize;
} // BSymbolTable::getStats


//==============================================================================
// BSymbolTable::dumpTable
//==============================================================================
void BSymbolTable::dumpTable(void(*output)(const char *)) const
{
   BSymbolEntry * pEntry;
   long i;
   char buffer132[132];
   int length;

   length = bsnprintf(buffer132, sizeof(buffer132), "BSymbolTable::dumpTable table(%x)\n", mTag);
   MYASSERT(length < 132);
   (output)(buffer132);

   for (i = 0 ; i < mTableSize ; i++)
   { 
      pEntry = mHashTable[i];
      while (pEntry)
      {
         dumpEntry(output, pEntry, i);
         pEntry = pEntry->mNext;
      }
   }

} // BSymbolTable::dumpTable


//==============================================================================
// BSymbolTable::dumpEntry
//==============================================================================
void BSymbolTable::dumpEntry(
   void(*output)(const char *),
   BSymbolEntry * pEntry,
   long index) const
{
   char buffer132[132];
   int length;

   // note: the linefeed is put out in dumpExtra
   length = bsnprintf(buffer132, sizeof(buffer132), "\t[%3d] %18s nameOffset=%3d kind=%d", index, pEntry->mNamePtr, pEntry->mNameOffset, pEntry->mKind);
   MYASSERT(length < 132);
   (output)(buffer132);
   dumpExtra(output, pEntry);
} // BSymbolTable::dumpEntry

//==============================================================================
// BSymbolTable::listLength
//==============================================================================
long BSymbolTable::listLength(long index)
{
   long           lResult = 0;
   BSymbolEntry * pEntry;

   pEntry = mHashTable[index];
   while (pEntry)
   {
      lResult++;
      pEntry = pEntry->mNext;
   }

   return lResult;
} // BSymbolTable::listLength


//==============================================================================
// BSymbolTable::hash
//
// TBD: optimize?
//==============================================================================
long BSymbolTable::hash(
   const char * name) const
{
   if (mTableSize <= 0)
      return(-1);

   long              lResult = 0;
   long              count = strlen(name);
   long              lTemp;
   unsigned char *   pTemp;
   unsigned char     uc;

   for (pTemp = (unsigned char *) name ; count >= 4; pTemp += 4, count -= 4)
   {
      lTemp = *(long *)(pTemp);
      #ifdef _BIGENDIAN_
         OutputDebugString("Swap the bytes to little endian first\n");
      #endif
      lResult ^= lTemp;
   }
   for( ; count > 0 ; pTemp++, count-- )   // get the remaining bytes
   {
      uc = *pTemp;
      lResult += (long) uc;
   }
   lResult %= mTableSize;
   MYASSERT(lResult < mTableSize);

   return lResult;
} // BSymbolTable::hash


//==============================================================================
// BSymbolTable::add
//==============================================================================
BSymbolEntry * BSymbolTable::add(
   const char * name, 
   long kind,
   long hash,
   long nameOffset)
{
   name;

   BSymbolEntry * pResult = NULL;
   BSymbolEntry * pEntry;

   MYASSERT(mHashTable);
   MYASSERT(hash == BSymbolTable::hash(name));

   // Allocate some free symbols if there aren't any
   if (mFreeSymbols == NULL) 
   {
      for (long i = 0 ; i < cAllocCount; i++)   // allocate them together for locality
      {
         // allocate an entry
         MYASSERT(mEntrySize >= sizeof(BSymbolEntry));
         pEntry = (BSymbolEntry *) new unsigned char [mEntrySize];
         if (pEntry == NULL)
            break;
         // add it to the free list
         pEntry->mNext = mFreeSymbols;
         mFreeSymbols = pEntry;
      }
   }

   if (mFreeSymbols)
   {
      // Get one of the free symbols
      pEntry = mFreeSymbols;
      mFreeSymbols = mFreeSymbols->mNext;

      // check the hash value
      MYASSERT(hash < mTableSize);

      // Fill it in
      pEntry->mNameOffset = nameOffset;
      pEntry->mNamePtr = mStringSpace->getString(nameOffset);
      pEntry->mKind = kind;

      // Hook it in
      pEntry->mNext = mHashTable[hash];
      mHashTable[hash] = pEntry;

      // Done
      mSymbolCount++;
      pResult = pEntry;
   }
   return pResult;
} // BSymbolTable::add


//==============================================================================
// BSymbolTable::reassignPointers
//==============================================================================
void BSymbolTable::reassignPointers(void)
{
   BSymbolEntry * pEntry;
   long i;

   for (i = 0 ; i < mTableSize ; i++)
   { 
      pEntry = mHashTable[i];
      while (pEntry)
      {
         pEntry->mNamePtr = mStringSpace->getString(pEntry->mNameOffset);
         pEntry = pEntry->mNext;
      }
   }
} // BSymbolTable::reassignPointers



//==============================================================================
// eof: symboltable.cpp
//==============================================================================
