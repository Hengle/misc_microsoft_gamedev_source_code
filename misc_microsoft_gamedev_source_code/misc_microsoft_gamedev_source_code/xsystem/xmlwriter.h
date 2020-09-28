//==============================================================================
// xmlwriter.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#pragma once 

// Forward declarations.

//==============================================================================
// BXMLWriterItem
//==============================================================================
class BXMLWriterItem
{
   public:
      BSimString  mName;
      BSimString  mVal;
      bool     mSubItems;
};

//==============================================================================
// BXMLWriter
//==============================================================================
class BXMLWriter
{
   public:
      enum
      {
         cFlagUnused=0x01,
         cFlagAttachedFile=0x02
      };

                              BXMLWriter();
                              ~BXMLWriter();

      bool                    create(long dirID, const BSimString& filename);
      bool                    attach(BFile& file);

      bool                    addItem(const char* itemName);
      bool                    addItem(const char* itemName, const char* itemVal);
      bool                    addItem(const char* itemName, float itemVal, long decimalPlaces=4);
      bool                    addItem(const char* itemName, long itemVal);
      bool                    addItem(const char* itemName, DWORD itemVal);
      bool                    addItem(const char* itemName, bool itemVal);
      bool                    addItem(const char* itemName, const BVector& itemVal, long decimalPlaces=4);

      bool                    startItem(const char* itemName);
      bool                    startItem(const char* itemName, const char* itemVal);
      bool                    startItem(const char* itemName, float itemVal, long decimalPlaces=4);
      bool                    startItem(const char* itemName, long itemVal);
      bool                    startItem(const char* itemName, DWORD itemVal);
      bool                    startItem(const char* itemName, bool itemVal);
      bool                    startItem(const char* itemName, const BVector& itemVal, long decimalPlaces=4);

      bool                    addAttribute(const char* attrName);
      bool                    addAttribute(const char* attrName, const char* attrVal);
      bool                    addAttribute(const char* attrName, float attrVal, long decimalPlaces=4);
      bool                    addAttribute(const char* attrName, long attrVal);
      bool                    addAttribute(const char* attrName, DWORD attrVal);
      bool                    addAttribute(const char* attrName, bool attrVal);
      bool                    addAttribute(const char* attrName, const BVector& attrVal, long decimalPlaces=4);

      bool                    endItem();

      void                    close();

      void                    setFlag(DWORD flag, bool val) { if(val) mFlags|=flag; else mFlags&=~flag; }
      bool                    getFlag(DWORD flag) const { return ((mFlags&flag)!=0); }

   protected:
      bool                    prepForWrite();
      bool                    prepForChildItems();

      bool                    setError();

      BFile*                  mFile;
      char*                   mDataBuffer;
      long                    mDataIndex;
      long                    mLevel;
      BDynamicSimArray<BXMLWriterItem>  mItemList;
      BXMLWriterItem*         mCurrentItem;
      bool                    mError;
      DWORD                   mFlags;
      
};
