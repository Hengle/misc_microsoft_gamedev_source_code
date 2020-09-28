//==============================================================================
// xssource.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xssource.h"
#ifdef _BANG
#include "chunker.h"
#endif


//==============================================================================
// BXSSource Static stuff
//==============================================================================
const DWORD BXSSource::msSaveVersion=0;

//==============================================================================
// BXSSource::BXSSource
//==============================================================================
BXSSource::BXSSource(bool caseSensitive) :
   mCode(NULL),
   mCodeSize(0),
   mMaxCodeSize(0)
   //mFiles doesn't need any ctor args.
   //mSymbols doesn't need any ctor args.
{
   mSymbols.initialize(113, caseSensitive, -1, BXSBinary::cInvalidSymbol);
}

//==============================================================================
// BXSSource::~BXSSource
//==============================================================================
BXSSource::~BXSSource(void)
{
   for (long i=0; i < mFiles.getNumber(); i++)
   {
      delete mFiles[i];
      mFiles[i]=NULL;
   }

   //Clean up the code.
   if (mCode != NULL)
   {
      BDELETE(mCode);
      mCodeSize=0;
      mMaxCodeSize=0;
   }
}

//==============================================================================
// BXSSource::allocateFileEntry
//==============================================================================
BXSFileEntry* BXSSource::allocateFileEntry(void)
{
   //Allocate.
   BXSFileEntry *newFE=new BXSFileEntry();
   if (newFE == NULL)
      return(NULL);
   //Store.
   if (mFiles.add(newFE) < 0)
      return(NULL);
   //Set the ID.
   newFE->setID(mFiles.getNumber()-1);
   return(newFE);
}

//==============================================================================
// BXSSource::getFileEntry
//==============================================================================
BXSFileEntry* BXSSource::getFileEntry(long id) const
{
   if ((id < 0) || (id >= mFiles.getNumber()) )
      return(NULL);
   return(mFiles[id]);
}

//==============================================================================
// BXSSource::getSymbol
//==============================================================================
const char* BXSSource::getSymbol(long id) const
{
   return(mSymbols.getSymbolByID(id));
}

//==============================================================================
// BXSSource::addCode
//==============================================================================
bool BXSSource::addCode(SBYTE *code, long size)
{
   //See if we have enough space just to add the code.
   long newSize=mCodeSize+size;
   BASSERT(newSize >= mCodeSize);
   BASSERT(newSize >= size);
   BASSERT(size > 0);
   if (newSize > mMaxCodeSize)
   {
      //Allocate space for more code.  Ensure it's big enough and doesn't start off
      //really small.
      long newCodeSize=mMaxCodeSize*2;
      if (newCodeSize <= 0)
         newCodeSize=cDefaultCodeSize;
      if (newCodeSize < newSize)
         newCodeSize=newSize*2;
      SBYTE *newCode=new SBYTE[newCodeSize];
      if (newCode == NULL)
         return(false);
      //Copy the old code.
      memcpy(newCode, mCode, mCodeSize);
      //Nuke the old code.
      delete [] mCode;
      //Save the new max and reset the pointer.
      mMaxCodeSize=newCodeSize;
      mCode=newCode;
   }

   //Copy the new code in.
   memcpy(mCode+mCodeSize, code, size);
   mCodeSize+=size;
   return(true);
}

//==============================================================================
// BXSSource::overwriteCode
//==============================================================================
bool BXSSource::overwriteCode(long position, SBYTE *code, long size)
{
   //Sanity checks.
   BASSERT(position+size >= position);
   BASSERT(position+size >= size);
   BASSERT(size > 0);
   if ((position < 0) || (position+size > mCodeSize) || (mCode == NULL))
      return(false);

   memcpy(mCode+position, code, size);
   return(true);
}

#ifdef _BANG
//=============================================================================
// BXSSource::save
//=============================================================================
bool BXSSource::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("Xs"), mainHandle);
   if (result == false)
   {
      {setBlogError(4405); blogerror("BXSSource::save -- error writing tag.");}
      return(false);
   }

   //Version.
   CHUNKWRITESAFE(chunkWriter, DWORD, msSaveVersion);

   //Files.
   CHUNKWRITESAFE(chunkWriter, Long, mFiles.getNumber());
   for (long i=0; i < mFiles.getNumber(); i++)
   {
      if (mFiles[i]->save(chunkWriter) == false)
      {
         {setBlogError(4406); blogerror("BXSSource::save -- failed to save mFiles[%d].", i);}
         return(false);
      }
   }
   //Symbols.
   if (mSymbols.save(chunkWriter) == false)
   {
      {setBlogError(4407); blogerror("BXSSource::save -- failed to save mSymbols.");}
      return(false);
   }
   //Code.
   CHUNKWRITESAFE(chunkWriter, Long, mCodeSize);
   CHUNKWRITEARRAYSAFE(chunkWriter, SignedChar, mCodeSize, mCode);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if (result == false)
   {
      {setBlogError(4408); blogerror("BXSSource::save -- failed to write chunk size!");}
      return(false);
   }

   return(true);
}

//=============================================================================
// BXSSource::load
//=============================================================================
bool BXSSource::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("Xs"));
   if (result == false)
   {
      {setBlogError(4409); blogerror("BXSSource::load -- error reading tag.");}
      return(false);
   }

   //Version.
   DWORD version=(DWORD)0;
   CHUNKREADSAFE(chunkReader, DWORD, version);
   //Don't load old format versions.
   //if (version < X)
   //{
   //   blog("BXSRuntime::load -- old format is incompatible, sorry.");
   //   return(false);
   //}

   //Files.
   long numberFiles=0;
   CHUNKREADSAFE(chunkReader, Long, numberFiles);
   if (mFiles.setNumber(numberFiles) == false)
   {
      {setBlogError(4410); blogerror("BXSSource::load -- failed to allocate %d mFiles.", numberFiles);}
      return(false);
   }
   for (long i=0; i < mFiles.getNumber(); i++)
   {
      mFiles[i]=new BXSFileEntry;
      if (mFiles[i] == NULL)
      {
         {setBlogError(4411); blogerror("BXSSource::load -- failed to allocate mFiles[%d].", i);}
         return(false);
      }
      if (mFiles[i]->load(chunkReader) == false)
      {
         {setBlogError(4412); blogerror("BXSSource::load -- failed to load mFiles[%d].", i);}
         return(false);
      }
   }
   //Symbols.
   if (mSymbols.load(chunkReader) == false)
   {
      {setBlogError(4413); blogerror("BXSSource::load -- failed to load mSymbols.");}
      return(false);
   }
   //Code.
   CHUNKREADSAFE(chunkReader, Long, mCodeSize);
   mCode=new SBYTE[mCodeSize];
   if (mCode == NULL)
   {
      {setBlogError(4414); blogerror("BXSSource::load -- failed to allocate mCode.");}
      return(false);
   }
   long readCount=0;
   CHUNKREADARRAYSAFE(chunkReader, SignedChar, &readCount, mCode, mCodeSize);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("Xs"));
   if (result == false)
   {
      {setBlogError(4415); blogerror("BXSSource::load -- did not read chunk properly!");}
      return(false);
   }

   return(true);
}
#endif



