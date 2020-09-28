//==============================================================================
// xsuserclassentry.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsuserclassentry.h"
#ifdef _BANG
#include "chunker.h"
#include "xsdata.h"
#endif

//==============================================================================
// Defines
//#define DEBUGSAVEANDLOAD


//=============================================================================
// BXSUserClassEntry::msSaveVersion
//=============================================================================
const DWORD BXSUserClassEntry::msSaveVersion=0;
//=============================================================================
// BXSUserClassEntry::msLoadVersion
//=============================================================================
DWORD BXSUserClassEntry::msLoadVersion=0xFFFF;

//==============================================================================
// BXSUserClassEntry::BXSUserClassEntry
//==============================================================================
BXSUserClassEntry::BXSUserClassEntry(void) :
   mID(-1),
   mSymbolID(-1)
   //mVariables doesn't need any ctor args.
   {
   }

//==============================================================================
// BXSUserClassEntry::~BXSUserClassEntry
//==============================================================================
BXSUserClassEntry::~BXSUserClassEntry(void)
{
   //Go through and delete any variables.
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      delete mVariables[i];
      mVariables[i]=NULL;
   }
}

//==============================================================================
// BXSUserClassEntry::getVariable
//==============================================================================
BXSVariableEntry* BXSUserClassEntry::getVariable(long index) const
{
   if ((index < 0) || (index >= mVariables.getNumber()) )
      return(NULL);
   return(mVariables[index]);
}

//==============================================================================
// BXSUserClassEntry::addVariable
//==============================================================================
bool BXSUserClassEntry::addVariable(BXSVariableEntry *ve)
{
   //If the entry is NULL, fail.
   if (ve == NULL)
      return(false);
   //Add it.
   if (mVariables.add(ve) == -1)
      return(false);
   return(true);
}

//=============================================================================
// BXSUserClassEntry::getDataLength
//=============================================================================
long BXSUserClassEntry::getDataLength(void) const
{
   long rVal=0;
   for (long i=0; i < mVariables.getNumber(); i++)
      rVal+=mVariables[i]->getDataLength();
   return(rVal);
}

//=============================================================================
// BXSUserClassEntry::init
//=============================================================================
bool BXSUserClassEntry::init(BYTE *data)
{
   //We assume data is already appropriately sized here.  Thus, we blow and go.
   if (data == NULL)
      return(false);

   long index=0;
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      long tempDL=mVariables[i]->getDataLength();
      memcpy(data+index, mVariables[i]->getData(), tempDL);
      index+=tempDL;
   }
   //Done.
   return(true);
}

#ifdef _BANG
//=============================================================================
// BXSUserClassEntry::writeVersion
//=============================================================================
bool BXSUserClassEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4479); blogerror("BXSUserClassEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("uB"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4480); blogerror("BXSUserClassEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSUserClassEntry::readVersion
//=============================================================================
bool BXSUserClassEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4481); blogerror("BXSUserClassEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("uB"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4482); blogerror("BXSUserClassEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSUserClassEntry::save
//=============================================================================
bool BXSUserClassEntry::save(BChunkWriter *chunkWriter, BXSData *data)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("uC"), mainHandle);
   if(!result)
   {
      {setBlogError(4483); blogerror("BXSUserClassEntry::save -- error writing tag.");}
      return(false);
   }

   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //SymbolID.
   CHUNKWRITESAFE(chunkWriter, Long, mSymbolID);
   //Variables.
   CHUNKWRITESAFE(chunkWriter, Long, mVariables.getNumber());
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      if (mVariables[i]->save(chunkWriter, data) == false)
      {
         {setBlogError(4484); blogerror("BXSUserClassEntry::save -- failed to save mVariables[%d].", i);}
         return(false);
      }
   }

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4485); blogerror("BXSUserClassEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSUserClassEntry::load
//=============================================================================
bool BXSUserClassEntry::load(BChunkReader *chunkReader, BXSData *data)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("uC"));
   if(!result)
   {
      {setBlogError(4486); blogerror("BXSUserClassEntry::load -- error reading tag.");}
      return(false);
   }

   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //SymbolID.
   CHUNKREADSAFE(chunkReader, Long, mSymbolID);
   //Variables.
   long numberVariables=0;
   CHUNKREADSAFE(chunkReader, Long, numberVariables);
   if (mVariables.setNumber(numberVariables) == false)
   {
      {setBlogError(4487); blogerror("BXSUserClassEntry::load -- failed to allocate %d mVariables.", numberVariables);}
      return(false);
   }
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      mVariables[i]=new BXSVariableEntry;
      if (mVariables[i] == NULL)
      {
         {setBlogError(4488); blogerror("BXSUserClassEntry::load -- failed to allocate mVariables[%d].", i);}
         return(false);
      }
      if (mVariables[i]->load(chunkReader, data) == false)
      {
         {setBlogError(4489); blogerror("BXSUserClassEntry::load -- failed to load mVariables[%d].", i);}
         return(false);
      }
   }

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("uC"));
   if(!result)
   {
      {setBlogError(4490); blogerror("BXSUserClassEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif

//=============================================================================
// BXSUserClassEntry::copy
//=============================================================================
bool BXSUserClassEntry::copy(BXSUserClassEntry *v)
{
   if (v == NULL)
      return(false);

   mID=v->mID;
   mSymbolID=v->mSymbolID;
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

   return(true);
}
