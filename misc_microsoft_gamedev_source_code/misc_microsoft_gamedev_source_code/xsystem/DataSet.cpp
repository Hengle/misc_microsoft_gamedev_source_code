//============================================================================
//
//  DataSet.cpp
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "dataentry.h"
#include "dataset.h"

//============================================================================
//  Class Methods
//============================================================================

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BDataSet::BDataSet(DWORD setSize)
{
   if (setSize == 0)
   {
      BFATAL_FAIL("BDataSet::BDataSet(DWORD setSize) -- don't specify 0 fool!");
   }
   else
   {
      mEntries = new BDataEntry[setSize];
      mNumEntries = setSize;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BDataSet::BDataSet(const BDataSet& set)
{
   *this = set;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BDataSet::~BDataSet()
{
   delete[] mEntries;
   mEntries = 0;
   mNumEntries = 0;

   mListeners.empty();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BDataSet& BDataSet::operator=(const BDataSet& set)
{
   delete[] mEntries;
   mEntries = NULL;
   mNumEntries = set.mNumEntries;
   if (mNumEntries)
   {
      mEntries = new BDataEntry[mNumEntries];

      for (DWORD idx=0; idx<mNumEntries; idx++)
      {
         mEntries[idx] = set.mEntries[idx];
         dataChanged(idx);
      }
   }
   return(*this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::addDataListener(BDataListener* listener)
{
   mListeners.addUnique(listener);
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::removeDataListener(BDataListener* listener)
{
   mListeners.removeItem(listener);
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setInt64(DWORD index, int64 data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setInt64(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setUInt64(DWORD index, uint64 data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setUInt64(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setLong(DWORD index, long data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setLong(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setDWORD(DWORD index, DWORD data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setDWORD(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setFloat(DWORD index, float data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setFloat(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setShort(DWORD index, short data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setShort(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setWORD(DWORD index, WORD  data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setWORD(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setBool(DWORD index, bool  data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setBool(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setBOOL(DWORD index, BOOL  data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setBOOL(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setChar(DWORD index, char  data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setChar(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setBYTE(DWORD index, BYTE  data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setBYTE(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setPointer(DWORD index, void* data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setPointer(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setVector(DWORD index, const BVector&  data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setVector(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setData(DWORD index, const void* data, WORD size)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setData(data, size))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setString(DWORD index, const BCHAR_T* data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setString(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setString(DWORD index, const BSimString& data)
{
   if (index >= mNumEntries)
      return(false);

   if (mEntries[index].setString(data))
   {
      dataChanged(index);
      return(true);
   }
   return(false);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getInt64(DWORD index, int64 &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getInt64(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getUInt64(DWORD index, uint64 &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getUInt64(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getLong(DWORD index, long  &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getLong(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getDWORD(DWORD index, DWORD &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getDWORD(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getFloat(DWORD index, float &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getFloat(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getShort(DWORD index, short &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getShort(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getWORD(DWORD index, WORD  &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getWORD(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getBool(DWORD index, bool  &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getBool(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getBOOL(DWORD index, BOOL  &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getBOOL(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getChar(DWORD index, char  &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getChar(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getBYTE(DWORD index, BYTE  &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getBYTE(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getPointer(DWORD index, void** data) const
{
   if (!data)
      return(false);

   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getPointer(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getVector(DWORD index, BVector &data) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getVector(data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getData(DWORD index, void* data, WORD size) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getData(data, size));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::getString(DWORD index, BCHAR_T* data, WORD size) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getString(data, size));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//bool BDataSet::getString(DWORD index, BSimString &data) const
//{
//   if (index >= mNumEntries)
//      return(false);
//
//   return(mEntries[index].getString(data));
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BDataSet::getDataSize(DWORD index) const
{
   if (index >= mNumEntries)
      return(-1);

   return(mEntries[index].getSize());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE BDataSet::getDataType(DWORD index) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].getType());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::setDataType(DWORD index, BYTE type)
{
   if (index >= mNumEntries)
      return(false);

   mEntries[index].setType(type);
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataSet::isSetFlag(DWORD index, BYTE flags) const
{
   if (index >= mNumEntries)
      return(false);

   return(mEntries[index].isSetFlag(flags));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BDataSet::setFlag(DWORD index, BYTE flags)
{
   if (index >= mNumEntries)
      return;

   mEntries[index].setFlag(flags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BDataSet::clearFlag(DWORD index, BYTE flags)
{
   if (index >= mNumEntries)
      return;

   mEntries[index].clearFlag(flags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE BDataSet::getFlags(DWORD index) const
{
   if (index >= mNumEntries)
      return(0);

   return(mEntries[index].getFlags());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BDataSet::dataChanged(DWORD index)
{
   for (DWORD idx=0; idx<mListeners.getSize(); idx++)
   {
      if (mListeners[idx])
         mListeners[idx]->OnDataChanged(this, index, getFlags(index));
   }
}
