//==============================================================================
// xsfilentry.cpp
//
// Copyright (c) 2001-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsfileentry.h"
#include "xsdefines.h"
#ifdef _BANG
#include "chunker.h"
#endif


//=============================================================================
// BXSFileEntry::msSaveVersion
//=============================================================================
const DWORD BXSFileEntry::msSaveVersion=0;
//=============================================================================
// BXSFileEntry::msLoadVersion
//=============================================================================
DWORD BXSFileEntry::msLoadVersion=0xFFFF;

//==============================================================================
// BXSFileEntry::BXSFileEntry
//==============================================================================
BXSFileEntry::BXSFileEntry(void) :
   mID(-1),
   mFilename(NULL),
   mFilenameLength(0),
   mSource(NULL),
   mSourceLength(0)
   {
   }

//==============================================================================
// BXSFileEntry::~BXSFileEntry
//==============================================================================
BXSFileEntry::~BXSFileEntry(void)
{
   if (mFilename != NULL)
   {
      delete [] mFilename;
      mFilename=NULL;
   }
   mFilenameLength=0;
   if (mSource != NULL)
   {
      delete [] mSource;
      mSource=NULL;
   }
   mSourceLength=0;
}

//==============================================================================
// BXSFileEntry::setFilename
//==============================================================================
bool BXSFileEntry::setFilename(const char *filename)
{
   //Bomb check.
   if (filename == NULL)
      return(false);
   //Clean out old name (if any).
   BDELETE(mFilename);
   mFilenameLength=0;

   //Allocate new name.
   mFilenameLength=strlen(filename);
   if (mFilenameLength == 0)
      return(false);
   mFilename=new char[mFilenameLength+1];
   if (mFilename == NULL)
   {
      mFilenameLength=0;
      return(false);
   }
   StringCchCopyA(mFilename, mFilenameLength+1, filename);

   return(true);
}

//==============================================================================
// BXSFileEntry::getSourceLine
//==============================================================================
const char* BXSFileEntry::getSourceLine(long lineNumber) const
{
   if ((lineNumber < 0) || (lineNumber >= mSourceLines.getNumber()) )
      return(NULL);
   return(&(mSource[mSourceLines[lineNumber]]) );
}

//==============================================================================
// BXSFileEntry::setSource
//==============================================================================
bool BXSFileEntry::setSource(const char *source)
{
   //Bomb check.
   if (source == NULL)
      return(false);
   //Clean out old source (if any).
   BDELETE(mSource);
   mSourceLength=0;

   //Allocate new source.
   mSourceLength=strlen(source);
   if (mSourceLength == 0)
      return(false);
   mSource=new char[mSourceLength+1];
   if (mSource == NULL)
   {
      mSourceLength=0;
      return(false);
   }
   StringCchCopyA(mSource, mSourceLength+1, source);
   mSource[mSourceLength]='\0';

   //If we have some valid source, parse it up into lines.
   if (mSourceLength > 0)
   {
      //Go through and calculate the source line starts.
      mSourceLines.setNumber(1);
      mSourceLines[0]=0;
      for (long i=0; i < mSourceLength; i++)
      {
         //Create a new line if we have one.
         if (mSource[i] == '\n')
         {
            if (mSourceLines.add(i+1) == -1)
            {
               BASSERT(0);
               return(false);
            }
            //Replace the newline char with a term char (to create the line).
            mSource[i]='\0';
            //If the previous char is a linefeed, replace it, too.
            if ((i >= 1) && (mSource[i-1] == '\r'))
               mSource[i-1]='\0';
         }
      }
   }

   return(true);
}

#ifdef _BANG
//=============================================================================
// BXSFileEntry::writeVersion
//=============================================================================
bool BXSFileEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4335); blogerror("BXSFileEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xg"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4336); blogerror("BXSFileEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSFileEntry::readVersion
//=============================================================================
bool BXSFileEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4337); blogerror("BXSFileEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xg"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4338); blogerror("BXSFileEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSFileEntry::save
//=============================================================================
bool BXSFileEntry::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xh"), mainHandle);
   if(!result)
   {
      {setBlogError(4339); blogerror("BXSFileEntry::save -- error writing tag.");}
      return(false);
   }

   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //Filename length.
   CHUNKWRITESAFE(chunkWriter, Long, mFilenameLength);
   //Filename.
   if (mFilenameLength > 0)
      CHUNKWRITETAGGEDARRAYSAFE(chunkWriter, Char, BCHUNKTAG("xi"), mFilenameLength+1, mFilename);
   //Source length.
   CHUNKWRITESAFE(chunkWriter, Long, mSourceLength);
   //Source.
   if (mSourceLength > 0)
      CHUNKWRITETAGGEDARRAYSAFE(chunkWriter, Char, BCHUNKTAG("xj"), mSourceLength+1, mSource);
   //Source lines.
   CHUNKWRITESAFE(chunkWriter, Long, mSourceLines.getNumber());
   CHUNKWRITEARRAYSAFE(chunkWriter, Long, mSourceLines.getNumber(), (long*)mSourceLines.getPtr());

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4340); blogerror("BXSFileEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSFileEntry::load
//=============================================================================
bool BXSFileEntry::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xh"));
   if(!result)
   {
      {setBlogError(4341); blogerror("BXSFileEntry::load -- error reading tag.");}
      return(false);
   }

   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //Filename length.
   CHUNKREADSAFE(chunkReader, Long, mFilenameLength);
   //Filename.
   if (mFilenameLength > 0)
   {
      mFilename=new char[mFilenameLength+1];
      if (mFilename == NULL)
      {
         {setBlogError(4342); blogerror("BXSFileEntry::load -- failed to allocate mFilename.");}
         return(false);
      }
      long readCount=0;
      CHUNKREADTAGGEDARRAYSAFE(chunkReader, Char, BCHUNKTAG("xi"), &readCount, mFilename, mFilenameLength+1);
   }
   //Source length.
   CHUNKREADSAFE(chunkReader, Long, mSourceLength);
   //Source.
   if (mSourceLength > 0)
   {
      mSource=new char[mSourceLength+1];
      if (mSource == NULL)
      {
         {setBlogError(4343); blogerror("BXSFileEntry::load -- failed to allocate mSource.");}
         return(false);
      }
      long readCount=0;
      CHUNKREADTAGGEDARRAYSAFE(chunkReader, Char, BCHUNKTAG("xj"), &readCount, mSource, mSourceLength+1);
   }
   //Source lines.
   long numberSourceLines=0;
   CHUNKREADSAFE(chunkReader, Long, numberSourceLines);
   if (mSourceLines.setNumber(numberSourceLines) == false)
   {
      {setBlogError(4344); blogerror("BXSFileEntry::load -- failed to allocate mSourceLines.");}
      return(false);
   }
   long readCount=0;
   CHUNKREADARRAYSAFE(chunkReader, Long, &readCount, (long*)mSourceLines.getPtr(), numberSourceLines);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xh"));
   if(!result)
   {
      {setBlogError(4345); blogerror("BXSFileEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif
