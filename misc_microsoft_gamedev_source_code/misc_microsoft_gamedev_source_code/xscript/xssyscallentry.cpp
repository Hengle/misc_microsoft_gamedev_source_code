//==============================================================================
// xssyscallentry.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xssyscallentry.h"
#include "xsdefines.h"
#ifdef _BANG
#include "chunker.h"
#endif


//=============================================================================
// BXSSyscallEntry::msSaveVersion
//=============================================================================
const DWORD BXSSyscallEntry::msSaveVersion=2;
//=============================================================================
// BXSSyscallEntry::msLoadVersion
//=============================================================================
DWORD BXSSyscallEntry::msLoadVersion=0xFFFF;

//==============================================================================
// BXSSyscallEntry::BXSSyscallEntry
//==============================================================================
BXSSyscallEntry::BXSSyscallEntry(void) :
   mID(-1),
   mSymbolID(-1),
   mSyscallType(-1),
   mReturnType(BXSVariableEntry::cInvalidVariable),
   mAddress(-1),
   mContext(false),
   mHelp(NULL)
   //mParameters doesn't need any ctor args.
   {
   }

//==============================================================================
// BXSSyscallEntry::~BXSSyscallEntry
//==============================================================================
BXSSyscallEntry::~BXSSyscallEntry(void)
{
   //Go through and delete any parms.
   for (long i=0; i < mParameters.getNumber(); i++)
   {
      delete mParameters[i];
      mParameters[i]=NULL;
   }
   BDELETE(mHelp);
}

//==============================================================================
// BXSSyscallEntry::setHelp
//==============================================================================
bool BXSSyscallEntry::setHelp(const char *helpString)
{
   //Bomb check.
   if (helpString == NULL)
      return(false);

   //Cleanup.
   BDELETE(mHelp);

   //Allocate a copy.
   long length = strlen(helpString) + 1;
   mHelp=new char[length];
   if (mHelp == NULL)
      return(false);
   //Copy.
   StringCchCopyA(mHelp, length, helpString);

   return(true);
}

//==============================================================================
// BXSSyscallEntry::getParameter
//==============================================================================
BXSVariableEntry* BXSSyscallEntry::getParameter(long index) const
{
   if ((index < 0) || (index >= mParameters.getNumber()) )
      return(NULL);
   return(mParameters[index]);
}

//==============================================================================
// BXSSyscallEntry::getParameterType
//==============================================================================
long BXSSyscallEntry::getParameterType(long index) const
{
   if ((index < 0) || (index >= mParameters.getNumber()) )
      return(-1);
   return(mParameters[index]->getType());
}

//==============================================================================
// BXSSyscallEntry::addParameter
//==============================================================================
bool BXSSyscallEntry::addParameter(BXSVariableEntry *ve)
{
   //If the entry is NULL, fail.
   if (ve == NULL)
      return(false);
   //Add it.
   if (mParameters.add(ve) == -1)
      return(false);
   return(true);
}

#ifdef _BANG
//=============================================================================
// BXSSyscallEntry::writeVersion
//=============================================================================
bool BXSSyscallEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4439); blogerror("BXSSyscallEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("ya"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4440); blogerror("BXSSyscallEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSyscallEntry::readVersion
//=============================================================================
bool BXSSyscallEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4441); blogerror("BXSSyscallEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("ya"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4442); blogerror("BXSSyscallEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSyscallEntry::save
//=============================================================================
bool BXSSyscallEntry::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("yb"), mainHandle);
   if(!result)
   {
      {setBlogError(4443); blogerror("BXSSyscallEntry::save -- error writing tag.");}
      return(false);
   }

   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //Symbol ID.
   CHUNKWRITESAFE(chunkWriter, Long, mSymbolID);
   //Return Type.
   CHUNKWRITESAFE(chunkWriter, BYTE, mReturnType);
   //Syscall Type
   CHUNKWRITESAFE(chunkWriter, Long, mSyscallType);
   //DO NOT SAVE mAddress.
   //Context.
   CHUNKWRITESAFE(chunkWriter, Bool, mContext);
   //TODO (maybe): Help.  If we do end up saving help, change what BXSSyscallModule::setSyscallHelp(...) does.
   //Parameters.
   CHUNKWRITESAFE(chunkWriter, Long, mParameters.getNumber());
   for (long i=0; i < mParameters.getNumber(); i++)
   {
      if (mParameters[i]->save(chunkWriter, NULL) == false)
      {
         {setBlogError(4444); blogerror("BXSSyscallEntry::save -- failed to save mParameters[%d].", i);}
         return(false);
      }
   }

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4445); blogerror("BXSSyscallEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSSyscallEntry::load
//=============================================================================
bool BXSSyscallEntry::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("yb"));
   if(!result)
   {
      {setBlogError(4446); blogerror("BXSSyscallEntry::load -- error reading tag.");}
      return(false);
   }

   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //Symbol ID.
   CHUNKREADSAFE(chunkReader, Long, mSymbolID);
   //Return type.
   if (msLoadVersion >= 1)
      CHUNKREADSAFE(chunkReader, BYTE, mReturnType);
   else
   {
      long temp;
      CHUNKREADSAFE(chunkReader, Long, temp);
      mReturnType=(BYTE)temp;
   }
   //Syscall Type.
   if (msLoadVersion >= 2)
      CHUNKREADSAFE(chunkReader, Long, mSyscallType);
   else
      mSyscallType=-1;
   //DO NOT SAVE mAddress.
   //Context.
   CHUNKREADSAFE(chunkReader, Bool, mContext);
   //TODO (maybe): Help.  If we do end up saving help, change what BXSSyscallModule::setSyscallHelp(...) does.
   //Parameters.
   long numberParameters=0;
   CHUNKREADSAFE(chunkReader, Long, numberParameters);
   if (mParameters.setNumber(numberParameters) == false)
   {
      {setBlogError(4447); blogerror("BXSSyscallEntry::load -- failed to allocate %d mParameters.", numberParameters);}
      return(false);
   }
   for (long i=0; i < mParameters.getNumber(); i++)
   {
      mParameters[i]=new BXSVariableEntry;
      if (mParameters[i] == NULL)
      {
         {setBlogError(4448); blogerror("BXSSyscallEntry::load -- failed to allocate mParameters[%d].", i);}
         return(false);
      }
      if (mParameters[i]->load(chunkReader, NULL) == false)
      {
         {setBlogError(4449); blogerror("BXSSyscallEntry::load -- failed to load mParameters[%d].", i);}
         return(false);
      }
   }

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("yb"));
   if(!result)
   {
      {setBlogError(4450); blogerror("BXSSyscallEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif
