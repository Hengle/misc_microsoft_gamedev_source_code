//=============================================================================
//
//  loadindextable.cpp
//
//  Copyright (c) 2004, Ensemble Studios
//
//=============================================================================

#include "xsystem.h"
#include "loadindextable.h"
#include "chunker.h"
#include "chunkreader.h"
#include "chunkwriter.h"

//==============================================================================
// BLoadIndexTable::BLoadIndexTable
//==============================================================================
BLoadIndexTable::BLoadIndexTable()
{
}


//==============================================================================
// BLoadIndexTable::~BLoadIndexTable
//==============================================================================
BLoadIndexTable::~BLoadIndexTable()
{
}


//==============================================================================
// BLoadIndexTable::load
//==============================================================================
bool BLoadIndexTable::load(BChunkReader *chunkReader, LOAD_TABLE_INDEX_CALLBACK func)
{
   // Check reader validity.
   if(!chunkReader)
   {
      BASSERT(0);
      return(false);
   }

   // Read tag.
   long result = chunkReader->readExpectedTag(BCHUNKTAG("LT"));
   if(!result)
      return(false);

   // Version.
   DWORD version;
   result = chunkReader->readDWORD(&version);
   if(!result)
      return(false);

   // Type count.
   long numNames;
   result=chunkReader->readLong(&numNames);
   if(!result)
      return(false);

   // Allocate names
   mNameList.setNumber(numNames);

   long i;
   BSimString tempName;
   for(i = 0; i < numNames; i++)
   {
      // Name.
      result=chunkReader->readBSimString(tempName);
      if(!result)
         return(false);

      mNameList[i] = tempName;
   }

   // Validate our reading of the chunk.
   result = chunkReader->validateChunkRead(BCHUNKTAG("LT"));
   if(!result)
   {
      {setBlogError(4595); blogerror("BLoadIndexTable::load -- did not read chunk properly!");}
      return(false);
   }

   // Populate index list
   mRealIndexList.setNumber(mNameList.getNumber());
   for (i = 0; i < mNameList.getNumber(); i++)
   {
      mRealIndexList[i] = func(mNameList[i]);
   }

   return(true);
}


//==============================================================================
// BLoadIndexTable::save
//==============================================================================
bool BLoadIndexTable::save(BChunkWriter *chunkWriter, LOAD_TABLE_NAME_CALLBACK func)
{
   // Current version.
   static const DWORD cLoadIndexTableSaveVersion=1;

   if(!chunkWriter)
      return(false);

   // Populate names list
   BSimString tempName;
   mNameList.setNumber(mRealIndexList.getNumber());
   long i;
   for (i = 0; i < mRealIndexList.getNumber(); i++)
   {
      func(mRealIndexList[i], tempName);
      mNameList[i].set(tempName);
   }

    // Write tag.
   long mainHandle;
   long result = chunkWriter->writeTagPostSized(BCHUNKTAG("LT"), mainHandle);
   if(!result)
      return(false);

   // Version.
   result = chunkWriter->writeDWORD(cLoadIndexTableSaveVersion);
   if(!result)
      return(false);

  // Name count.
   result=chunkWriter->writeLong(mNameList.getNumber());
   if(!result)
      return(false);

   // Write out names
   for (i = 0; i < mNameList.getNumber(); i++)
   {
      // Name.
      result=chunkWriter->writeBString(mNameList[i]);
      if(!result)
         return(false);
   }

   // Finish chunk.   
   result = chunkWriter->writeSize(mainHandle);
   if(!result)
      return(false);

   return(true);
}
