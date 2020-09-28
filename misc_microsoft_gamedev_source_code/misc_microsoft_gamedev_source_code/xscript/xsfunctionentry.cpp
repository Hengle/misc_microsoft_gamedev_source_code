//==============================================================================
// xsfunctionentry.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsfunctionentry.h"
#include "xsdefines.h"
#ifdef _BANG
#include "chunker.h"
#endif


//=============================================================================
// BXSFunctionEntry::msSaveVersion
//=============================================================================
const DWORD BXSFunctionEntry::msSaveVersion=2;
//=============================================================================
// BXSFunctionEntry::msLoadVersion
//=============================================================================
DWORD BXSFunctionEntry::msLoadVersion=0xFFFF;


//==============================================================================
// BXSFunctionEntry::BXSFunctionEntry
//==============================================================================
BXSFunctionEntry::BXSFunctionEntry(void) :
   mID(-1),
   mSymbolID(-1),
   mFileID(-1),
   mLineNumber(-1),
   mReturnType(BXSVariableEntry::cInvalidVariable),
   mCodeOffset(-1),
   mNumberParameters(0),
   //mVariables doesn't need any ctor args.
   mMutable(false)
   {
   }

//==============================================================================
// BXSFunctionEntry::~BXSFunctionEntry
//==============================================================================
BXSFunctionEntry::~BXSFunctionEntry(void)
{
   //Go through and delete any variables.
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      delete mVariables[i];
      mVariables[i]=NULL;
   }
}

//==============================================================================
// BXSFunctionEntry::getParameterType
//==============================================================================
long BXSFunctionEntry::getParameterType(long index) const
{
   if ((index < 0) || (index >= mNumberParameters))
      return(BXSVariableEntry::cInvalidVariable);
   return(mVariables[index]->getType());
}

//==============================================================================
// BXSFunctionEntry::addParameter
//==============================================================================
bool BXSFunctionEntry::addParameter(BXSVariableEntry *ve)
{
   //If the entry is NULL or we've already add some variables, fail.
   if ((ve == NULL) || (mNumberParameters != mVariables.getNumber()) || (mNumberParameters >= 254))
      return(false);
   //Add it.
   if (mVariables.add(ve) == -1)
      return(false);
   mNumberParameters++;
   return(true);
}

//==============================================================================
// BXSFunctionEntry::getVariable
//==============================================================================
BXSVariableEntry* BXSFunctionEntry::getVariable(long index) const
{
   if ((index < 0) || (index >= mVariables.getNumber()) )
      return(NULL);
   return(mVariables[index]);
}

//==============================================================================
// BXSFunctionEntry::addVariable
//==============================================================================
bool BXSFunctionEntry::addVariable(BXSVariableEntry *ve)
{
   //If the entry is NULL, fail.
   if (ve == NULL)
      return(false);
   //Add it.
   if (mVariables.add(ve) == -1)
      return(false);
   return(true);
}

#ifdef _BANG
//=============================================================================
// BXSFunctionEntry::writeVersion
//=============================================================================
bool BXSFunctionEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4346); blogerror("BXSFunctionEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xk"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4347); blogerror("BXSFunctionEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSFunctionEntry::readVersion
//=============================================================================
bool BXSFunctionEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4348); blogerror("BXSFunctionEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xk"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4349); blogerror("BXSFunctionEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSFunctionEntry::save
//=============================================================================
bool BXSFunctionEntry::save(BChunkWriter *chunkWriter, BXSData *data)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xm"), mainHandle);
   if(!result)
   {
      {setBlogError(4350); blogerror("BXSFunctionEntry::save -- error writing tag.");}
      return(false);
   }

   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //Symbol ID.
   CHUNKWRITESAFE(chunkWriter, Long, mSymbolID);
   //File ID.
   CHUNKWRITESAFE(chunkWriter, Long, mFileID);
   //Line number.
   CHUNKWRITESAFE(chunkWriter, Long, mLineNumber);
   //Return type.
   CHUNKWRITESAFE(chunkWriter, BYTE, mReturnType);
   //Code offset.
   CHUNKWRITESAFE(chunkWriter, Long, mCodeOffset);
   //Number parameters.
   CHUNKWRITESAFE(chunkWriter, BYTE, mNumberParameters);
   //Variables.
   CHUNKWRITESAFE(chunkWriter, Long, mVariables.getNumber());
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      if (mVariables[i]->save(chunkWriter, data) == false)
      {
         {setBlogError(4351); blogerror("BXSFunctionEntry::save -- failed to save mVariables[%d].", i);}
         return(false);
      }
   }
   //Mutable.
   CHUNKWRITESAFE(chunkWriter, Bool, mMutable);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4352); blogerror("BXSFunctionEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSFunctionEntry::load
//=============================================================================
bool BXSFunctionEntry::load(BChunkReader *chunkReader, BXSData *data)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xm"));
   if(!result)
   {
      {setBlogError(4353); blogerror("BXSFunctionEntry::load -- error reading tag.");}
      return(false);
   }

   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //Symbol ID.
   CHUNKREADSAFE(chunkReader, Long, mSymbolID);
   //File ID.
   CHUNKREADSAFE(chunkReader, Long, mFileID);
   //Line number.
   CHUNKREADSAFE(chunkReader, Long, mLineNumber);
   //Return type.
   if (msLoadVersion >= 2)
      CHUNKREADSAFE(chunkReader, BYTE, mReturnType);
   else
   {
      long temp;
      CHUNKREADSAFE(chunkReader, Long, temp);
      mReturnType=(BYTE)temp;
   }
   //Code offset.
   CHUNKREADSAFE(chunkReader, Long, mCodeOffset);
   //Number parameters.
   if (msLoadVersion >= 2)
      CHUNKREADSAFE(chunkReader, BYTE, mNumberParameters);
   else
   {
      long temp;
      CHUNKREADSAFE(chunkReader, Long, temp);
      mNumberParameters=(BYTE)temp;
   }
   //Variables.
   long numberVariables=0;
   CHUNKREADSAFE(chunkReader, Long, numberVariables);
   if (mVariables.setNumber(numberVariables) == false)
   {
      {setBlogError(4354); blogerror("BXSFunctionEntry::load -- failed to allocate %d mVariables.", numberVariables);}
      return(false);
   }
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      mVariables[i]=new BXSVariableEntry;
      if (mVariables[i] == NULL)
      {
         {setBlogError(4355); blogerror("BXSFunctionEntry::load -- failed to allocate mVariables[%d].", i);}
         return(false);
      }
      if (mVariables[i]->load(chunkReader, data) == false)
      {
         {setBlogError(4356); blogerror("BXSFunctionEntry::load -- failed to load mVariables[%d].", i);}
         return(false);
      }
   }
   //Mutable.
   if (msLoadVersion >= 1)
      CHUNKREADSAFE(chunkReader, Bool, mMutable);
   else
      mMutable=false;

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xm"));
   if(!result)
   {
      {setBlogError(4357); blogerror("BXSFunctionEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif

//=============================================================================
// BXSFunctionEntry::copy
//=============================================================================
bool BXSFunctionEntry::copy(BXSFunctionEntry *v)
{
   if (v == NULL)
      return(false);

   mID=v->mID;
   mSymbolID=v->mSymbolID;
   mFileID=v->mFileID;
   mLineNumber=v->mLineNumber;
   mCodeOffset=v->mCodeOffset;
   if (mVariables.setNumber(v->mVariables.getNumber()) == false)
      return(false);
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      mVariables[i]=new BXSVariableEntry;
      if (mVariables[i] == NULL)
         return(false);
      if (mVariables[i]->copy(v->mVariables[i]) == false)
         return(false);
   }
   mMutable=v->mMutable;
   mReturnType=v->mReturnType;
   mNumberParameters=v->mNumberParameters;

   return(true);
}
