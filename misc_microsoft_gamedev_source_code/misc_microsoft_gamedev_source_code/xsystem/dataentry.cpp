//============================================================================
//
//  DataEntry.cpp
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "math\vector.h"
#include "dataentry.h"
//#include "strhelper.h"

//============================================================================
//  PRIVATE MACROS
//============================================================================
#define HEADERFROMDATA(ptr) ((BDataEntryHeader*)((ptr) - sizeof(BDataEntryHeader)))

//============================================================================
//  Class Methods
//============================================================================

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BDataEntry::BDataEntry()
: mInfo(0)
{
   mData.d8 = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BDataEntry::~BDataEntry()
{ 
   if (getType() >= cTypeVariable) 
      releaseVariableData();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BDataEntry& BDataEntry::operator=(const BDataEntry& entry)
{
   if (getType() >= cTypeVariable)
      releaseVariableData();

   if (entry.getType() >= cTypeVariable)
   {
      long size = entry.getSize();
      if (size > 0)
      {
         allocateVariableData((WORD)size);
         XMemCpy(mData.d4, entry.mData.d4, size);
      }
   }
   else
      mData = entry.mData;

   mInfo = entry.mInfo;

   return(*this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BDataEntry::setType(BYTE type)         
{ 
   if (getType() == type)
      return;

   if (getType() >= cTypeVariable)
   {
      // clean up memory
      releaseVariableData();
   }
   mData.d8 = 0;
   mInfo=(BYTE)((mInfo&cMaskFlags)|(type&cMaskType)); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setInt64(int64 data)
{ 
   if (getType()!=cTypeInt64)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setUInt64(uint64 data)
{
   if (getType()!=cTypeInt64)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setLong(long  data)
{ 
   if (getType()!=cTypeLong)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setDWORD(DWORD data)
{ 
   if (getType()!=cTypeLong)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setFloat(float data)
{ 
   if (getType()!=cTypeFloat)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setShort(short data)
{ 
   if (getType()!=cTypeShort)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setWORD(WORD  data)
{ 
   if (getType()!=cTypeShort)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setBool(bool  data)
{ 
   if (getType()!=cTypeBool)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setBOOL(BOOL  data)
{ 
   if (getType()!=cTypeBool)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setChar(char  data)
{ 
   if (getType()!=cTypeByte)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setBYTE(BYTE  data)
{ 
   if (getType()!=cTypeByte)
      return(false); 

   XMemCpy(&mData, &data, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setPointer(void* ptr)
{
   if (getType()!=cTypePointer)
      return(false);

   mData.d4 = (BYTE*)ptr;
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setVector(const BVector&  data)
{
   if (getType()!=cTypeVector)
      return(false); 

   WORD size = sizeof(BVector);
   if (!allocateVariableData(size))
      return(false);

   XMemCpy(mData.d4, &data, size);
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setData(const void* data, WORD size)
{
   if (getType() < cTypeVariable)
   {
      XMemCpy(&mData, data, min(size, sizeof(mData)));
      return(true);
   }

   if (!allocateVariableData(size))
      return(false);

   XMemCpy(mData.d4, data, size);
   return(true);
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setString(const BCHAR_T* string)
{
   if (getType()!=cTypeString)
      return(false); 

   if (!string)
      return(false);

   DWORD length = strLength(string);
   WORD size = (WORD)((length+1)*sizeof(BCHAR_T));

   if (!allocateVariableData(size))
      return(false);

   XMemCpy(mData.d4, string, size);
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::setString(const BSimString &string)
{
   if (getType()!=cTypeString)
      return(false); 

   WORD size = (WORD)((string.length()+1)*sizeof(BCHAR_T));

   if (!allocateVariableData(size))
      return(false);

   XMemCpy(mData.d4, string.getPtr(), size);
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getInt64(int64 &data) const
{
   if (getType()!=cTypeInt64)
      return(false);
   XMemCpy(&data, &mData, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getUInt64(uint64 &data) const
{
   if (getType()!=cTypeInt64)
      return(false);
   XMemCpy(&data, &mData, sizeof(data));
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getLong(long  &data) const
{ 
   if (getType()!=cTypeLong)
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getDWORD(DWORD &data) const 
{ 
   if (getType()!=cTypeLong)  
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getFloat(float &data) const 
{ 
   if (getType()!=cTypeFloat) 
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getShort(short &data) const 
{ 
   if (getType()!=cTypeShort) 
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getWORD(WORD  &data) const 
{ 
   if (getType()!=cTypeShort) 
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getBool(bool  &data) const 
{ 
   if (getType()!=cTypeBool)  
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getBOOL(BOOL  &data) const 
{ 
   if (getType()!=cTypeBool)  
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getChar(char  &data) const 
{ 
   if (getType()!=cTypeByte)  
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getBYTE(BYTE  &data) const 
{ 
   if (getType()!=cTypeByte)  
      return(false); 
   XMemCpy(&data, &mData, sizeof(data));
   return(true); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getPointer(void** ptr) const
{
   if (!ptr)
      return(false);

   if (getType()!=cTypePointer)
      return(false);

   *ptr = mData.d4;
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getVector(BVector &data) const
{ 
   if (getType()!=cTypeVector)
      return(false); 

   XMemCpy(&data, &mData, sizeof(data)); 
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getData(void* data, WORD size) const
{
   if (getType() < cTypeVariable)
   {
      XMemCpy(data, &mData, min(size, sizeof(mData)));
      return(true);
   }

   if (!mData.d4)
      return(false);

//-- FIXING PREFIX BUG ID 567
   const BDataEntryHeader* pHeader = HEADERFROMDATA(mData.d4);
//--
   if (size < pHeader->mCurSize)
      return(false);
   XMemCpy(data, mData.d4, min(size, pHeader->mCurSize));
   return(true);
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::getString(BCHAR_T* string, WORD length) const
{
   if (getType()!=cTypeString)
      return(false); 

   if (!mData.d4)
      return(false);

//-- FIXING PREFIX BUG ID 568
   const BDataEntryHeader* pHeader = HEADERFROMDATA(mData.d4);
//--
   if (length < (pHeader->mCurSize/sizeof(BCHAR_T)))
      return(false);

   XMemCpy(string, mData.d4, min(length*sizeof(BCHAR_T), pHeader->mCurSize));   
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//bool BDataEntry::getString(BSimString &string) const
//{
//   if (getType()!=cTypeString)
//      return(false); 
//
//   if (!mData.d4)
//      return(false);
//
//   string.set((BCHAR_T*)mData.d4);
//   return(true);
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BDataEntry::getSize(void) const
{
   if (getType() < cTypeInt64)
      return (sizeof(mData.d4));
   else if (getType() < cTypeVariable)
      return (sizeof(mData));

   if (!mData.d4)
      return(0);

//-- FIXING PREFIX BUG ID 569
   const BDataEntryHeader* pHeader = HEADERFROMDATA(mData.d4);
//--
   return(pHeader->mCurSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BDataEntry::releaseVariableData()
{
   if (!mData.d4)
      return;
   BDataEntryHeader* pHeader = HEADERFROMDATA(mData.d4);
   delete [] ((BYTE*)pHeader);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BDataEntry::allocateVariableData(WORD size)
{
   BDataEntryHeader* pHeader;
   if (mData.d4)
   {
      pHeader = HEADERFROMDATA(mData.d4);
      if (pHeader->mMaxSize >= size)
      {
         pHeader->mCurSize = size;
         return(true);
      }

      delete [] ((BYTE*)pHeader);
      mData.d8 = 0;
   }

   mData.d4 = new BYTE[size + sizeof(BDataEntryHeader)];
   if (!mData.d4)
      return(false);

   mData.d4 += sizeof(BDataEntryHeader);
   pHeader = HEADERFROMDATA(mData.d4);
   pHeader->mMaxSize = pHeader->mCurSize = size;

   return(true);
}

/*
HRESULT BDataEntry::serialize(BSerialWriter& writer)
{
   HRESULT hr = S_OK;

   if (getType() < cTypeVariable)
   {
      writer.writeData(&mData, sizeof(mData));
      return S_FALSE;
   }

   if (!mData)
      return S_FALSE;

   BDataEntryHeader* pHeader = HEADERFROMDATA(mData);
   //if (size < )
//      return S_FALSE;

   if (FAILED(hr = writer.writeData(mData, pHeader->mCurSize)))
      return hr;

   return hr;
}

HRESULT BDataEntry::deserialize(BSerialReader& reader)
{
   WORD size = (WORD)reader.size();

   if (getType() < cTypeVariable)
   {
      if (size > sizeof(mData))
         return(false);

      reader.readData(&mData, size);
      return(true);
   }

   if (!allocateVariableData(size))
      return(false);

   BDataEntryHeader* pHeader = HEADERFROMDATA(mData);
   reader.readData(mData, size);
   pHeader->mCurSize = size;

   return(true);
}

long BDataEntry::serializedSize(void)
{
	return this->getSize();
}
*/
