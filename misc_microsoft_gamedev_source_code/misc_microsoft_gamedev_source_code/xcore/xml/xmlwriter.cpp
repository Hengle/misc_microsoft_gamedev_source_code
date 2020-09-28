#if 0
//==============================================================================
// xmlwriter.cpp
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

// Includes
#include "xcore.h"
#include "math\vector.h"
#include "containers\utsimplearray.h"
#include "xmlwriter.h"

// Constants
const long cDataBufferSize=1024;
const long cDataBufferCutoff=768;

// Statics

//==============================================================================
// BXMLWriter::BXMLWriter
//==============================================================================
BXMLWriter::BXMLWriter() :
   mpFile(NULL),
   mDataBuffer(NULL),
   mDataIndex(0),
   mLevel(0),
   mItemList(),
   mCurrentItem(NULL),
   mError(false)
{
}

//==============================================================================
// BXMLWriter::~BXMLWriter
//==============================================================================
BXMLWriter::~BXMLWriter()
{
   close();
   if(mDataBuffer)
   {
      delete mDataBuffer;
      mDataBuffer=NULL;
   }
}

//==============================================================================
// BXMLWriter::close
//==============================================================================
void BXMLWriter::close()
{
   if(!getFlag(cFlagAttachedFile))
   {
      if(mpFile)
      {
         mpFile->close();
         delete mpFile;
      }
   }

   mpFile=NULL;

   mError=false;
}

//==============================================================================
// BXMLWriter::setError
//==============================================================================
bool BXMLWriter::setError()
{
   if(!mError)
   {
      mError=true;
      BASSERT(0);
   }
   return false;
}

//==============================================================================
// BXMLWriter::create
//==============================================================================
bool BXMLWriter::create(BIXMLFile* pFile, long dirID, const BString& filename)
{
   close();
   
   if(!pFile->openWriteable(dirID, filename))
      return setError();

   setFlag(cFlagAttachedFile, false);

   mpFile=pFile;

   return prepForWrite();
}

//==============================================================================
// BXMLWriter::prepForWrite
//==============================================================================
bool BXMLWriter::prepForWrite()
{
   if(!mDataBuffer)
   {
      mDataBuffer=new char[cDataBufferSize];
      if(!mDataBuffer)
      {
         close();
         return setError();
      }
   }

   mDataIndex=0;
   mLevel=0;
   mCurrentItem=NULL;

   return true;
}

//==============================================================================
// BXMLWriter::attach
//==============================================================================
bool BXMLWriter::attach(BIXMLFile& file)
{
   close();

   mpFile=&file;
   setFlag(cFlagAttachedFile, true);

   return prepForWrite();
}

//==============================================================================
// BXMLWriter::addItem
//==============================================================================
bool BXMLWriter::addItem(const char* itemName)
{
   if(!startItem(itemName))
      return false;
   return endItem();
}

//==============================================================================
// BXMLWriter::addItem
//==============================================================================
bool BXMLWriter::addItem(const char* itemName, const char* itemVal)
{
   if(!startItem(itemName, itemVal))
      return false;
   return endItem();
}

//==============================================================================
// BXMLWriter::addItem
//==============================================================================
bool BXMLWriter::addItem(const char* itemName, float itemVal, long decimalPlaces)
{
   if(!startItem(itemName, itemVal, decimalPlaces))
      return false;
   return endItem();
}

//==============================================================================
// BXMLWriter::addItem
//==============================================================================
bool BXMLWriter::addItem(const char* itemName, long itemVal)
{
   if(!startItem(itemName, itemVal))
      return false;
   return endItem();
}

//==============================================================================
// BXMLWriter::addItem
//==============================================================================
bool BXMLWriter::addItem(const char* itemName, DWORD itemVal)
{
   if(!startItem(itemName, itemVal))
      return false;
   return endItem();
}

//==============================================================================
// BXMLWriter::addItem
//==============================================================================
bool BXMLWriter::addItem(const char* itemName, bool itemVal)
{
   if(!startItem(itemName, itemVal))
      return false;
   return endItem();
}

//==============================================================================
// BXMLWriter::addItem
//==============================================================================
bool BXMLWriter::addItem(const char* itemName, const BVector& itemVal, long decimalPlaces)
{
   if(!startItem(itemName, itemVal, decimalPlaces))
      return false;
   return endItem();
}


//==============================================================================
// BXMLWriter::startItem
//==============================================================================
bool BXMLWriter::startItem(const char* itemName)
{
   const char* itemVal=NULL;
   return startItem(itemName, itemVal);
}

//==============================================================================
// BXMLWriter::startItem
//==============================================================================
bool BXMLWriter::startItem(const char* itemName, const char* itemVal)
{
   if(mError)
      return false;

   if(mCurrentItem)
   {
      if(!prepForChildItems())
         return false;
   }

   if(!itemName)
      return setError();

   mDataIndex=mLevel*3;

   for(long i=0; i<mDataIndex; i++)
      mDataBuffer[i]=' ';

   long len=sprintf(mDataBuffer+mDataIndex, "<%s", itemName);
   if(len==-1)
      return setError();

   mDataIndex+=len;

   mLevel++;

   BXMLWriterItem tempItem;
   tempItem.mName.set(itemName);
   tempItem.mVal.set(itemVal);
   tempItem.mSubItems=false;
   if(mItemList.add(tempItem)!=mLevel-1)
      return setError();

   mCurrentItem=&(mItemList[mLevel-1]);

   return true;
}

//==============================================================================
// BXMLWriter::startItem
//==============================================================================
bool BXMLWriter::startItem(const char* itemName, float itemVal, long decimalPlaces)
{
   BString tempString, tempString2;
   tempString2.format(B("%%.%df"), decimalPlaces);
   tempString.format(tempString2.getPtr(), itemVal);
   return startItem(itemName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::startItem
//==============================================================================
bool BXMLWriter::startItem(const char* itemName, long itemVal)
{
   BString tempString, tempString2;
   tempString.format(B("%d"), itemVal);
   return startItem(itemName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::startItem
//==============================================================================
bool BXMLWriter::startItem(const char* itemName, DWORD itemVal)
{
   BString tempString, tempString2;
   tempString.format(B("%u"), itemVal);
   return startItem(itemName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::startItem
//==============================================================================
bool BXMLWriter::startItem(const char* itemName, bool itemVal)
{
   BString tempString, tempString2;
   tempString.format(B("%s"), itemVal ? B("true") : B("false"));
   return startItem(itemName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::startItem
//==============================================================================
bool BXMLWriter::startItem(const char* itemName, const BVector& itemVal, long decimalPlaces)
{
   BString tempString, tempString2;
   tempString2.format(B("%%.%df,%%.%df,%%.%df"), decimalPlaces, decimalPlaces, decimalPlaces);
   tempString.format(tempString2.getPtr(), itemVal.x, itemVal.y, itemVal.z);
   return startItem(itemName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::addAttribute
//==============================================================================
bool BXMLWriter::addAttribute(const char* attrName)
{
   const char* val=NULL;
   return addAttribute(attrName, val);
}

//==============================================================================
// BXMLWriter::addAttribute
//==============================================================================
bool BXMLWriter::addAttribute(const char* attrName, const char* attrVal)
{
   if(mError)
      return false;

   if(!mCurrentItem)
      return setError();

   if(mCurrentItem->mSubItems)
      return setError();

   if(!attrName)
      return setError();

   long len=sprintf(mDataBuffer+mDataIndex, " %s=\"%s\"", attrName, (attrVal && *attrVal ? attrVal: ""));
   if(len==-1)
      return setError();
   mDataIndex+=len;

   if(mDataIndex>=cDataBufferCutoff)
   {
      if(!mpFile->write(mDataBuffer, mDataIndex))
         return setError();
      mDataIndex=0;
   }

   return true;
}

//==============================================================================
// BXMLWriter::addAttribute
//==============================================================================
bool BXMLWriter::addAttribute(const char* attrName, float attrVal, long decimalPlaces)
{
   BString tempString, tempString2;
   tempString2.format(B("%%.%df"), decimalPlaces);
   tempString.format(tempString2.getPtr(), attrVal);
   return addAttribute(attrName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::addAttribute
//==============================================================================
bool BXMLWriter::addAttribute(const char* attrName, long attrVal)
{
   BString tempString, tempString2;
   tempString.format(B("%d"), attrVal);
   return addAttribute(attrName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::addAttribute
//==============================================================================
bool BXMLWriter::addAttribute(const char* attrName, DWORD attrVal)
{
   BString tempString, tempString2;
   tempString.format(B("%u"), attrVal);
   return addAttribute(attrName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::addAttribute
//==============================================================================
bool BXMLWriter::addAttribute(const char* attrName, bool attrVal)
{
   BString tempString, tempString2;
   tempString.format(B("%s"), attrVal ? B("true") : B("false"));
   return addAttribute(attrName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::addAttribute
//==============================================================================
bool BXMLWriter::addAttribute(const char* attrName, const BVector& attrVal, long decimalPlaces)
{
   BString tempString, tempString2;
   tempString2.format(B("%%.%df,%%.%df,%%.%df"), decimalPlaces, decimalPlaces, decimalPlaces);
   tempString.format(tempString2.getPtr(), attrVal.x, attrVal.y, attrVal.z);
   return addAttribute(attrName, tempString.asANSI());
}

//==============================================================================
// BXMLWriter::prepForChildItems
//==============================================================================
bool BXMLWriter::prepForChildItems()
{
   if(mError)
      return false;

   if(mCurrentItem->mSubItems)
      return true;

   long len=sprintf(mDataBuffer+mDataIndex, ">%s\r\n", mCurrentItem->mVal.isEmpty() ? "" : mCurrentItem->mVal.asANSI());
   if(len==-1)
      return setError();
   mDataIndex+=len;

   if(!mpFile->write(mDataBuffer, mDataIndex))
      return setError();

   mCurrentItem->mSubItems=true;

   return true;
}

//==============================================================================
// BXMLWriter::endItem
//==============================================================================
bool BXMLWriter::endItem()
{
   if(mError)
      return false;

   if(mLevel<1)
      return setError();

   mLevel--;

   if(mItemList.getNumber()<mLevel+1)
      return setError();

   mCurrentItem=&(mItemList[mLevel]);

   if(mCurrentItem->mSubItems)
   {
      mDataIndex=mLevel*3;

      for(long i=0; i<mDataIndex; i++)
         mDataBuffer[i]=' ';

      long len=sprintf(mDataBuffer+mDataIndex, "</%s>\r\n", mCurrentItem->mName.asANSI());
      if(len==-1)
         return setError();
      mDataIndex+=len;
   }
   else if(!mCurrentItem->mVal.isEmpty())
   {
      long len=sprintf(mDataBuffer+mDataIndex, ">%s</%s>\r\n", mCurrentItem->mVal.asANSI(), mCurrentItem->mName.asANSI());
      if(len==-1)
         return setError();
      mDataIndex+=len;
   }
   else
   {
      long len=sprintf(mDataBuffer+mDataIndex, "/>\r\n");
      if(len==-1)
         return setError();
      mDataIndex+=len;
   }

   if(!mpFile->write(mDataBuffer, mDataIndex))
      return setError();

   if(!mItemList.setNumber(mLevel))
      return setError();

   if(mLevel>0)
      mCurrentItem=&(mItemList[mLevel-1]);
   else
      mCurrentItem=NULL;

   return true;
}
#endif