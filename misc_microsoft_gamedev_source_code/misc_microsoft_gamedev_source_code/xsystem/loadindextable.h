//==============================================================================
// loadIndexTable.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef __LOADINDEXTABLE__
#define __LOADINDEXTABLE__

class BChunkReader;
class BChunkWriter;

typedef long (*LOAD_TABLE_INDEX_CALLBACK)(const BSimString& name);
typedef bool (*LOAD_TABLE_NAME_CALLBACK)(long realIndex, BSimString& name);

//============================================================================
// class BLoadIndexTable
//
// This class abstracts out saving and loading indices to objects that are
// referenced by name, such as textures and models.  Given a set of real
// indices, it will save out a list of object names (determined via callback)
// and return a relative index for each unique real index given.  These relative
// indices can then be saved for each object.  During load, the names are read
// and mapped to real indices (via callback).  Relative indices are read in per object, then
// converted to real indices via the table.  Here's an example:
//
// ----------------------------------------------------------------------------
// During save:
//
// BLoadIndexTable textureTable;
// for (eachTexture)
//    textureTable.addRealIndex(eachTexture.index)
//
// textureTable.save(chunkWriter, getTextureNameFromIndex /*callback*/);
//
// for (eachTexture)
// {
//    // save relative index
//    chunkWriter->writeLong(textureTable.getRelativeIndex(eachTexture.index));
// }
//
// ----------------------------------------------------------------------------
// During load:
//
// BLoadIndex Table textureTable;
// textureTable.load(chunkReader, getTextureIndexFromName /*callback*/);
//
// for (eachTexture)
// {
//    // read relative index
//    long relativeIndex;
//    chunkWriter->readLong(relativeIndex);
//    
//    eachTexture.index = textureTable.getReadIndex(relativeIndex);
// }
//
//============================================================================
class BLoadIndexTable
{
   public:

      BLoadIndexTable();
      virtual ~BLoadIndexTable();

      inline void             addRealIndex(long realIndex);

      inline long             getRelativeIndex(const BSimString& name) const;
      inline long             getRelativeIndex(long realIndex) const;
      inline long             getRealIndex(long relativeIndex) const;
      inline bool             getName(long relativeIndex, BSimString& name) const;

      // Load / save
      bool                    load(BChunkReader *chunkReader, LOAD_TABLE_INDEX_CALLBACK func);
      bool                    save(BChunkWriter *chunkWriter, LOAD_TABLE_NAME_CALLBACK func);

   protected:

      BDynamicSimArray<long>      mRealIndexList;
      BDynamicSimArray<BSimString>   mNameList;
};

//============================================================================
//============================================================================
// BLoadIndexTable inline functions

//==============================================================================
// BLoadIndexTable::addRealIndex
//==============================================================================
inline void BLoadIndexTable::addRealIndex(long realIndex)
{
   // Search table for index
   long index = 0;
   while (index < mRealIndexList.getNumber())
   {
      if (mRealIndexList[index] == realIndex)
         return;

      index++;
   }

   // If not found, add
   mRealIndexList.add(realIndex);
}

//==============================================================================
// BLoadIndexTable::getRelativeIndex
//==============================================================================
inline long BLoadIndexTable::getRelativeIndex(const BSimString& name) const
{
   long index = 0;
   while (index < mNameList.getNumber())
   {
      if (mNameList[index].compare(name) == 0)
         return index;

      index++;
   }

   return -1;
}

//==============================================================================
// BLoadIndexTable::getRelativeIndex
//==============================================================================
inline long BLoadIndexTable::getRelativeIndex(long realIndex) const
{
   long index = 0;
   while (index < mNameList.getNumber())
   {
      if (mRealIndexList[index] == realIndex)
         return index;

      index++;
   }

   return -1;
}

//==============================================================================
// BLoadIndexTable::getRealIndex
//==============================================================================
inline long BLoadIndexTable::getRealIndex(long relativeIndex) const
{
   if (relativeIndex>=0 && relativeIndex < mRealIndexList.getNumber())
   {
      return mRealIndexList[relativeIndex];
   }
   else
   {
      BFAIL("Invalid relative index in load index table");
      return -1;
   }
}

//==============================================================================
// BLoadIndexTable::getName
//==============================================================================
inline bool BLoadIndexTable::getName(long relativeIndex, BSimString& name) const
{
   if (relativeIndex>=0 && relativeIndex < mNameList.getNumber())
   {
      name = mNameList[relativeIndex];
      return true;
   }
   else
   {
      BFAIL("Invalid relative index in load index table");
      return false;
   }
}

#endif

//============================================================================
// eof: loadindextable.h
//============================================================================
