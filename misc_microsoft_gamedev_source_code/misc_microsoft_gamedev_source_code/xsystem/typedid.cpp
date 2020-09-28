//==============================================================================
// typedid.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

//==============================================================================
//Includes
#include "xsystem.h"
#include "typedid.h"
#include "chunker.h"


// @TITAN -- is this safe to do?  Need a better way of specifying the defaults, maybe mo
//const BTypedID cTypedIDNone(BEntity::cNone, -1);
const BTypedID cTypedIDNone(-1, -1);

//=============================================================================
// BTypedID::msSaveVersion
//=============================================================================
const DWORD BTypedID::msSaveVersion=0;
//=============================================================================
// BTypedID::msLoadVersion
//=============================================================================
DWORD BTypedID::msLoadVersion=0xFFFF;


//=============================================================================
// BTypedID::writeVersion
//=============================================================================
bool BTypedID::writeVersion(BChunkWriter* chunkWriter)
{
   if (!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4120); blogerror("BTypedID::writeVersion -- bad chunkWriter.");}
      return(false);
   }

   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("TV"), msSaveVersion);
   if (!result)
   {
      {setBlogError(4121); blogerror("BTypedID::writeVersion -- failed to write version.");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BTypedID::readVersion
//=============================================================================
bool BTypedID::readVersion(BChunkReader* chunkReader)
{
   if (!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4122); blogerror("BTypedID::readVersion -- bad chunkReader.");}
      return(false);
   }

   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("TV"), &msLoadVersion);
   if (!result)
   {
      {setBlogError(4123); blogerror("BTypedID::readVersion -- failed to read version.");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BTypedID::save
//=============================================================================
bool BTypedID::save(BChunkWriter* chunkWriter, bool scenario)
{
   scenario;
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   long result;
   CHUNKWRITESAFE(chunkWriter, Long, mType);
   CHUNKWRITESAFE(chunkWriter, Long, mID);

   return(true);
}

//=============================================================================
// BTypedID::load
//=============================================================================
bool BTypedID::load(BChunkReader* chunkReader, bool scenario)
{
   scenario;
   //Check reader validity.
   if (!chunkReader)
   {
      BASSERT(0);
      return(false);
   }

   long result;
   CHUNKREADSAFE(chunkReader, Long, mType);
   CHUNKREADSAFE(chunkReader, Long, mID);

   return(true);
}


//==============================================================================
// BTypedID::loadUnitIDsIntoArray
//==============================================================================
void BTypedID::loadUnitIDsIntoArray(long filterType, const BTypedID *ids, long count, BDynamicSimLongArray &array)
{
   array.setNumber(0);
   for(long i=0; i<count; i++)
   {
      if(ids[i].getType()==filterType)
         array.add(ids[i].getID());
   }
}

//==============================================================================
// BTypedID::loadUnitIDsIntoArray
//==============================================================================
void BTypedID::loadUnitIDsIntoArray(long filterType, const BTypedID *ids, long count, BTypedIDArray &array)
{
   array.setNumber(0);
   for(long i=0; i<count; i++)
   {
      if(ids[i].getType()==filterType)
         array.add(ids[i]);
   }
}


//==============================================================================
// eof: typedid.h
//==============================================================================
