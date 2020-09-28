//==============================================================================
// xsvariableentry.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsvariableentry.h"
#include "xsdefines.h"
#include "xsuserclassentry.h"
#ifdef _BANG
#include "chunker.h"
#include "xsdata.h"
#endif

//==============================================================================
// Defines
//#define DEBUGSAVEANDLOAD

//=============================================================================
// BXSVariableEntry::msSaveVersion
//=============================================================================
const DWORD BXSVariableEntry::msSaveVersion=2;
//=============================================================================
// BXSVariableEntry::msLoadVersion
//=============================================================================
DWORD BXSVariableEntry::msLoadVersion=0xFFFF;

//==============================================================================
// BXSVariableEntry::BXSVariableEntry
//==============================================================================
BXSVariableEntry::BXSVariableEntry(void) :
   mSymbolID(-1),
   mData(NULL),
   mType(cInvalidVariable),
   mModifiers(0)
   {
   }

//==============================================================================
// BXSVariableEntry::~BXSVariableEntry
//==============================================================================
BXSVariableEntry::~BXSVariableEntry(void)
{
   if (mData != NULL)
   {
      delete [] mData;
      mData=NULL;
   }
}

//==============================================================================
// BXSVariableEntry::setConst
//==============================================================================
void BXSVariableEntry::setConst(bool v)
{
   if (v)
      mModifiers=(BYTE)(mModifiers|cConst);
   else
      mModifiers=(BYTE)(mModifiers&~cConst);
}

//==============================================================================
// BXSVariableEntry::setExtern
//==============================================================================
void BXSVariableEntry::setExtern(bool v)
{
   if (v)
      mModifiers=(BYTE)(mModifiers|cExtern);
   else
      mModifiers=(BYTE)(mModifiers&~cExtern);
}

//==============================================================================
// BXSVariableEntry::setStatic
//==============================================================================
void BXSVariableEntry::setStatic(bool v)
{
   if (v)
      mModifiers=(BYTE)(mModifiers|cStatic);
   else
      mModifiers=(BYTE)(mModifiers&~cStatic);
}

//==============================================================================
// BXSVariableEntry::setExport
//==============================================================================
void BXSVariableEntry::setExport(bool v)
{
   if (v)
      mModifiers=(BYTE)(mModifiers|cExport);
   else
      mModifiers=(BYTE)(mModifiers&~cExport);
}

//==============================================================================
// BXSVariableEntry::setTemp
//==============================================================================
void BXSVariableEntry::setTemp(bool v)
{
   if (v)
      mModifiers=(BYTE)(mModifiers|cTemp);
   else
      mModifiers=(BYTE)(mModifiers&~cTemp);
}

//==============================================================================
// BXSVariableEntry::getDataLength
//==============================================================================
long BXSVariableEntry::getDataLength(void) const
{
   switch (mType)
   {
      case BXSVariableEntry::cIntegerVariable:
         return(cIntegerDataSize);
      case BXSVariableEntry::cFloatVariable:
         return(cFloatDataSize);
      case BXSVariableEntry::cBoolVariable:
         return(cBoolDataSize);
      case BXSVariableEntry::cStringVariable:
         return(getStringLength());
      case BXSVariableEntry::cVectorVariable:
         return(cVectorDataSize);
      case BXSVariableEntry::cUserClassVariable:
         return(getUserClassLength());
   }

   return(-1);
}

//==============================================================================
// BXSVariableEntry::getData
//==============================================================================
BYTE* BXSVariableEntry::getData(void) const
{
   if ((mType != cStringVariable) && (mType != cUserClassVariable))
      return(mData);
   return( (mData+sizeof(long)) );
}

//==============================================================================
// BXSVariableEntry::setData
//==============================================================================
bool BXSVariableEntry::setData(void *d)
{
   return(setAll(mSymbolID, mType, d));
}

//==============================================================================
// BXSVariableEntry::setAll
//==============================================================================
bool BXSVariableEntry::setAll(long s, long t, void *d)
{
   mSymbolID=s;
   mType=t;

   //If we have a string, set that.
   if (mType == cStringVariable)
      return(setString(d));
   //If we have a user class, set that.
   if (mType == cUserClassVariable)
      return(setUserClass(d));

   long length=getDataLength();
   if (length <= 0)
      return(false);

   if (mData == NULL)
   {
#if defined(XBOX)
      if (mType == cVectorVariable)
         mData=new _DECLSPEC_ALIGN_16_ BYTE[length];
      else
         mData=new BYTE[length];
#else
      mData=new BYTE[length];
#endif
      if (mData == NULL)
         return(false);
   }

   if (d == NULL)
      memset(mData, 0, length);
   else
      memcpy(mData, d, length);

   return(true);
}

//==============================================================================
// BXSVariableEntry::setString
//==============================================================================
bool BXSVariableEntry::setString(void *d)
{
   //Get our length.
   char *string=(char*)d;
   long length=0;
   if (string != NULL)
      length=strlen(string);

   //DCP TODO: Just nuke the old data for now.
   if (mData != NULL)
   {
      delete [] mData;
      mData=NULL;
   }

   //Allocate new data.
   if (mData == NULL)
   {
      mData=new BYTE[length+1+sizeof(long)];
      if (mData == NULL)
         return(false);
   }

   //Copy in the size.
   memcpy(mData, &length, sizeof(long));
   //Copy in the string.
   if (length > 0)
   {
      if (d == NULL)
         memset(mData+sizeof(long), 0, length);
      else
         memcpy(mData+sizeof(long), d, length);
   }
   //Terminate.
   mData[length+sizeof(long)]='\0';

   return(true);
}

//==============================================================================
// BXSVariableEntry::getStringLength
//==============================================================================
long BXSVariableEntry::getStringLength(void) const
{
   if (mData == NULL)
      return(0);
   //Fetch the length out of the first 4 bytes.
   long length=*(long*)mData;
   //Add in the terminator.
   length++;
   return(length);
}

//=============================================================================
// BXSVariableEntry::initUserClass
//=============================================================================
bool BXSVariableEntry::initUserClass(BXSUserClassEntry *uce)
{
   //Bomb check.
   if (uce == NULL)
      return(false);
   //For now (at least), we expect our data to be NULL.
   BASSERT(mData == NULL);
   if (mData != NULL)
   {
      delete [] mData;
      mData=NULL;
   }
   //Blow our symbol out as well.
   mSymbolID=-1;
   //Set our type.
   mType=cUserClassVariable;

   //Allocate new data size: the size of the UC, plus 4 bytes for storing that
   //size here for faster lookup later.
   long length=uce->getDataLength();
   mData=new BYTE[length+sizeof(long)];
   if (mData == NULL)
      return(false);

   //Copy in the length.
   memcpy(mData, &length, sizeof(long));
   //Copy in the data.
   if (uce->init(mData+sizeof(long)) == false)
      return(false);

   //Done.
   return(true);
}

//==============================================================================
// BXSVariableEntry::setUserClass
//==============================================================================
bool BXSVariableEntry::setUserClass(void *d)
{
   //If we're not already allocated, we fail.
   if (mData == NULL)
      return(false);

   //Get our length.
   long length=*(long*)mData;

   //Else, we assume we were allocated to the same size (as user classes don't
   //change size).  If the data is NULL, 0 us out.
   if (d == NULL)
      memset(mData+sizeof(long), 0, length);
   else
      memcpy(mData+sizeof(long), d, length);

   return(true);
}

//==============================================================================
// BXSVariableEntry::getUserClassLength
//==============================================================================
long BXSVariableEntry::getUserClassLength(void) const
{
   if (mData == NULL)
      return(0);
   //Fetch the length out of the first 4 bytes.
   long length=*(long*)mData;
   return(length);
}

#ifdef _BANG
//=============================================================================
// BXSVariableEntry::writeVersion
//=============================================================================
bool BXSVariableEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4270); blogerror("BXSVariableEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xe"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4271); blogerror("BXSVariableEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSVariableEntry::readVersion
//=============================================================================
bool BXSVariableEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4272); blogerror("BXSVariableEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xe"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4273); blogerror("BXSVariableEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSVariableEntry::save
//=============================================================================
bool BXSVariableEntry::save(BChunkWriter *chunkWriter, BXSData *data)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xf"), mainHandle);
   if(!result)
   {
      {setBlogError(4274); blogerror("BXSVariableEntry::save -- error writing tag.");}
      return(false);
   }

   //Symbol ID.
   CHUNKWRITESAFE(chunkWriter, Long, mSymbolID);
   //Type.
   CHUNKWRITESAFE(chunkWriter, Long, mType);
   //Modifiers.
   CHUNKWRITESAFE(chunkWriter, BYTE, mModifiers);
   //Data length.
   long dataLength=getDataLength();
   CHUNKWRITESAFE(chunkWriter, Long, dataLength);
   //Data.
   if ((mType == cStringVariable) || (mType == cUserClassVariable))
      CHUNKWRITEARRAYSAFE(chunkWriter, BYTE, dataLength, (BYTE*)(mData+sizeof(long)) );
   else
      CHUNKWRITEARRAYSAFE(chunkWriter, BYTE, dataLength, (BYTE*)mData);

   //Hack test thing.
   data;
   /*#ifdef DEBUGSAVEANDLOAD
   if ((getStatic() == true) && (data != NULL))
   {
      const char *symbol=data->getSymbol(mSymbolID);
      char buffer[256];
      switch (mType)
      {
         case BXSVariableEntry::cIntegerVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableSave: INT, %4d, v=%d (%s, SID=%d).",
               dataLength, *(long*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cFloatVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableSave: FLT, %4d, v=%f (%s, SID=%d).",
               dataLength, *(float*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cBoolVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableSave: BOO, %4d, v=%d (%s, SID=%d).",
               dataLength, *(bool*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cStringVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableSave: STR, %4d, v=%s (%s, SID=%d).",
               dataLength, (char*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cVectorVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableSave: VEC, %4d, v=(%f, %f, %f) (%s, SID=%d).",
               dataLength, ((BVector*)mData)->x, ((BVector*)mData)->y, ((BVector*)mData)->z, symbol, mSymbolID);
            break;
         case BXSVariableEntry::cUserClassVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableSave: UCL, %4d, (%s, SID=%d).",
               dataLength, symbol, mSymbolID);
            break;
         default:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableSave: INVALID, v=??? (%s, SID=%d).", symbol, mSymbolID);
            break;
      }
      blog(buffer);
   }
   #endif*/

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4275); blogerror("BXSVariableEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSVariableEntry::load
//=============================================================================
bool BXSVariableEntry::load(BChunkReader *chunkReader, BXSData *data)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xf"));
   if(!result)
   {
      {setBlogError(4276); blogerror("BXSVariableEntry::load -- error reading tag.");}
      return(false);
   }

   //Symbol ID.
   CHUNKREADSAFE(chunkReader, Long, mSymbolID);
   //Type.
   CHUNKREADSAFE(chunkReader, Long, mType);
   //Modifiers.
   CHUNKREADSAFE(chunkReader, BYTE, mModifiers);
   //Data length.
   long dataLength=0;
   CHUNKREADSAFE(chunkReader, Long, dataLength);
   long allocateLength=dataLength;
   //Add sizeof(long) to the string and user class allocate length.
   if ((mType == cStringVariable) || (mType == cUserClassVariable))
      allocateLength+=sizeof(long);
   
   //Data.
#if defined(XBOX)
   if (mType == cVectorVariable)
      mData=new _DECLSPEC_ALIGN_16_ BYTE[allocateLength];
   else
      mData=new BYTE[allocateLength];
#else
   mData=new BYTE[allocateLength];
#endif

   if (mData == NULL)
   {
      {setBlogError(4277); blogerror("BXSVariableEntry::load -- failed to allocate mData.");}
      return(false);
   }
   long count=0;
   //Read the data (remembering to offset it to allow for the length storage).
   if ((mType == cStringVariable) || (mType == cUserClassVariable))
      CHUNKREADARRAYSAFE(chunkReader, BYTE, &count, (BYTE*)(mData+sizeof(long)), dataLength);
   else
      CHUNKREADARRAYSAFE(chunkReader, BYTE, &count, (BYTE*)mData, dataLength);

   //Do the wacky fixup for the string terminator.
   if (mType == cStringVariable)
      dataLength--;
   //Stuff in the data length now for strings and UCs.
   if ((mType == cStringVariable) || (mType == cUserClassVariable))
      memcpy(mData, &dataLength, sizeof(long));

   //Hack test thing.
   data;
   /*#ifdef DEBUGSAVEANDLOAD
   if ((getStatic() == true) && (data != NULL))
   {
      const char *symbol=data->getSymbol(mSymbolID);
      char buffer[256];
      switch (mType)
      {
         case BXSVariableEntry::cIntegerVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableLoad: INT, %4d, v=%d (%s, SID=%d), mSymbolID.",
               dataLength, *(long*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cFloatVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableLoad: FLT, %4d, v=%f (%s, SID=%d), mSymbolID.",
               dataLength, *(float*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cBoolVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableLoad: BOO, %4d, v=%d (%s, SID=%d), mSymbolID.",
               dataLength, *(bool*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cStringVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableLoad: STR, %4d, v=%s (%s, SID=%d), mSymbolID.",
               dataLength, (char*)(mData), symbol, mSymbolID);
            break;
         case BXSVariableEntry::cVectorVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableLoad: VEC, %4d, v=(%f, %f, %f) (%s, SID=%d), mSymbolID.",
               dataLength, ((BVector*)mData)->x, ((BVector*)mData)->y, ((BVector*)mData)->z, symbol, mSymbolID);
            break;
         case BXSVariableEntry::cUserClassVariable:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableLoad: UCL, %4d, (%s, SID=%d), mSymbolID.",
               dataLength, symbol, mSymbolID);
            break;
         default:
            bsnprintf(buffer, sizeof(buffer), "StaticVariableLoad: INVALID, v=??? (%s, SID=%d).", symbol, mSymbolID);
            break;
      }
      blog(buffer);
   }
   #endif*/

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xf"));
   if(!result)
   {
      {setBlogError(4278); blogerror("BXSVariableEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif

//=============================================================================
// BXSVariableEntry::copy
//=============================================================================
bool BXSVariableEntry::copy(BXSVariableEntry *v)
{
   if (v == NULL)
      return(false);

   mSymbolID=v->mSymbolID;
   mType=v->mType;
   mModifiers=v->mModifiers;

   //Data.
   long allocateLength=v->getDataLength();
   if ((mType == cStringVariable) || (mType == cUserClassVariable))
      allocateLength+=sizeof(long);
#if defined(XBOX)
   else if (mType == cVectorVariable)
      mData=new _DECLSPEC_ALIGN_16_ BYTE[allocateLength];
#endif
   else
      mData=new BYTE[allocateLength];
   if (mData == NULL)
      return(false);
   memcpy(mData, v->mData, allocateLength);

   return(true);
}
