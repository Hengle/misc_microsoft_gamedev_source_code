//=============================================================================
// stringtoidmap.cpp
//
// Copyright (c) 1999, 2000, 2001 Ensemble Studios
//
// Mapping table
//=============================================================================

#include "xsystem.h"
#include "stringtoidmap.h"
#include "chunker.h"



//=============================================================================
// BStringToIDMap::msSaveVersion
//=============================================================================
const DWORD BStringToIDMap::msSaveVersion=0;


//=============================================================================
// BStringToIDMap::BStringToIDMap
//=============================================================================
BStringToIDMap::BStringToIDMap(void) :
   mIDCount(0),
   mNewIDs(NULL),
   mbReflection(false)
{
} // BStringToIDMap::BStringToIDMap
 

//=============================================================================
// BStringToIDMap::~BStringToIDMap
//=============================================================================
BStringToIDMap::~BStringToIDMap(void)
{
   if(mNewIDs)
      delete []mNewIDs;
} // BStringToIDMap::~BStringToIDMap




//=============================================================================
// BStringToIDMap::allocateIDs
//=============================================================================
void BStringToIDMap::allocateIDs(long num)
{
   if(num < 0)
   {
      BFAIL("num < 0");
      return;
   }

   if(mNewIDs)
      delete []mNewIDs;

   mNewIDs = new long[num];
   if(!mNewIDs)
   {
      BASSERT(0);
      mIDCount = 0;
      return;
   }

   mIDCount = num;

   memset(mNewIDs, 0, num*sizeof(long));

} 


//=============================================================================
// BStringToIDMap::add
//=============================================================================
void BStringToIDMap::add(long id, long newID)
{
   if(id<0 || id>=mIDCount)
   {
      BASSERT(0);
      return;
   }

   mNewIDs[id] = newID;

} // BStringToIDMap::add

//=============================================================================
// BStringToIDMap::save
//=============================================================================
bool BStringToIDMap::save(BChunkWriter *chunkWriter)
{
   // Check param validity.
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4107); blogerror("BStringToIDMap::save -- invalid params!");}
      return(false);
   }

   // Write tag.
   long mainHandle;
   long result = chunkWriter->writeTagPostSized(BCHUNKTAG("TM"), mainHandle);
   if(!result)
   {
      {setBlogError(4108); blogerror("BStringToIDMap::save -- error writing tag.");}
      return(false);
   }

   // Version.
   result = chunkWriter->writeDWORD(msSaveVersion);
   if(!result)
   {
      {setBlogError(4109); blogerror("BStringToIDMap::save -- error writing version.");}
      return(false);
   }

   // Count.
   result=chunkWriter->writeLong(getCurrentIDCount());
   if(!result)
   {
      {setBlogError(4110); blogerror("BStringToIDMap::save -- failed to write count");}
      return(false);
   }

   // Names.
   for(long i=0; i<getCurrentIDCount(); i++)
   {
      
      // Get the name.
      const char *name = getStringFromID(i);

      // Save the name.
      if(name)
         result=chunkWriter->writeCharArray(strlen(name)+1, name);
      else
         result=chunkWriter->writeCharArray(0, NULL);
      if(!result)
      {
         {setBlogError(4111); blogerror("BStringToIDMap::save -- failed to save name.");}
         return(false);
      }
   }

   // Finish chunk.   
   result = chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4112); blogerror("BStringToIDMap::save -- failed to write chunk size!");}
      return(false);
   }

   return(true);
}


//=============================================================================
// BStringToIDMap::load
//=============================================================================
bool BStringToIDMap::load(BChunkReader *chunkReader)
{
   // Check reader validity.
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4113); blogerror("BStringToIDMap::load -- invalid params!");}
      return(false);
   }

   // Read tag.
   long result = chunkReader->readExpectedTag(BCHUNKTAG("TM"));
   if(!result)
   {
      {setBlogError(4114); blogerror("BStringToIDMap::load -- error reading tag.");}
      return(false);
   }

   // Version.
   DWORD version;
   result = chunkReader->readDWORD(&version);
   if(!result)
   {
      {setBlogError(4115); blogerror("BStringToIDMap::load -- error reading version.");}
      return(false);
   }

   // Count.
   long numIDs;
   result=chunkReader->readLong(&numIDs);
   if(!result)
   {  
      {setBlogError(4116); blogerror("BStringToIDMap::load -- failed to read count.");}
      return(false);
   }
   allocateIDs(numIDs);

   char tempString[1024];
   for(long i=0; i<numIDs; i++)
   {
      long len, count;

      // Peek length.
      result=chunkReader->peekArrayLength(&len);
      if(!result || len>1024)
      {
         BASSERT(0);
         {setBlogError(4117); blogerror("BStringToIDMap::load -- invalid name length");}
         return(false);
      }

      result=chunkReader->readCharArray(&count, tempString, len);
      if(len>0)
      {
         if(!result || count!=len)
         {
            {setBlogError(4118); blogerror("BStringToIDMap::load -- failed to read name");}
            return(false);
         }

         // Get new ID.
         long dwID = getIDFromString(tempString);

         // Add to table.
         add(i, dwID);
      }
      else
         add(i, -1);
   }

   // Validate our reading of the chunk.
   result = chunkReader->validateChunkRead(BCHUNKTAG("TM"));
   if(!result)
   {
      {setBlogError(4119); blogerror("BStringToIDMap::load -- did not read chunk properly!");}
      return(false);
   }

   // turn off reflect mode if it is set
   setReflection(false);
   return(true);
}


//=============================================================================
// BStringToIDMap::translate
//=============================================================================
bool BStringToIDMap::translate(long oldID, long &newID)
{

   // we allow -1 to be saved and restored as a valid id, but do
   // not allow the id to be greater than the array max (duh!)
   if (mbReflection || oldID < 0)
   {
      newID = oldID;
      return (true);
   }

   // if you are asserting here it is because the  id cannot possibly be
   // mapped into the new database
   if(oldID>=mIDCount)
   {
      BASSERT(0);
      return (false);
   }

   newID = mNewIDs[oldID];
   return (true);
}


//=============================================================================
// eof: stringtoidmap.cpp
//=============================================================================
