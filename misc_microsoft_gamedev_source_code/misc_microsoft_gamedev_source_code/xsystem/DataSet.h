//============================================================================
//
//  DataSet.h
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================


#pragma once 

#ifndef DATASET_H
#define DATASET_H


//----------------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------------
#include "array.h"
#include "dataentry.h"

//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
//class BDataEntry;
class BDataSet;

//----------------------------------------------------------------------------
//  Typedefs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  Class BDataSet
//----------------------------------------------------------------------------
class BDataSet
{
public:
   // defining the flags at this level just seems wrong. someone really should move them.
   enum
   {
      cFlagMultiplayer = 0x10,
      cFlagUpdateFromESO = 0x20,
      cFlagModifyIngame = 0x40,           // this option can be modified during a game.
      cFlagGamelist = 0x80                
   };
   
   class BDataListener
   {
   public:
      virtual void OnDataChanged(const BDataSet* set, DWORD index, BYTE flags) = 0;      
   };
   
   BDataSet(DWORD setSize);
   BDataSet(const BDataSet& set);
   virtual ~BDataSet();
   
   BDataSet& operator=(const BDataSet& set);
   
   bool  addDataListener(BDataListener* listener);
   bool  removeDataListener(BDataListener* listener);   

   bool  setInt64    (DWORD index, int64 data);
   bool  setUInt64   (DWORD index, uint64 data);
   bool  setLong     (DWORD index, long  data);
   bool  setDWORD    (DWORD index, DWORD data);
   bool  setFloat    (DWORD index, float data);
   bool  setShort    (DWORD index, short data);
   bool  setWORD     (DWORD index, WORD  data);
   bool  setBool     (DWORD index, bool  data);
   bool  setBOOL     (DWORD index, BOOL  data);
   bool  setChar     (DWORD index, char  data);
   bool  setBYTE     (DWORD index, BYTE  data);
   bool  setPointer  (DWORD index, void* data);
   bool  setVector   (DWORD index, const BVector&  data);
   bool  setData     (DWORD index, const void* data, WORD size);
   bool  setString   (DWORD index, const BCHAR_T* data);
   bool  setString   (DWORD index, const BSimString& data);

   bool  getInt64    (DWORD index, int64 &data) const;
   bool  getUInt64   (DWORD index, uint64 &data) const;
   bool  getLong     (DWORD index, long  &data) const;
   bool  getDWORD    (DWORD index, DWORD &data) const;
   bool  getFloat    (DWORD index, float &data) const;
   bool  getShort    (DWORD index, short &data) const;
   bool  getWORD     (DWORD index, WORD  &data) const;
   bool  getBool     (DWORD index, bool  &data) const;
   bool  getBOOL     (DWORD index, BOOL  &data) const;
   bool  getChar     (DWORD index, char  &data) const;
   bool  getBYTE     (DWORD index, BYTE  &data) const;
   bool  getPointer  (DWORD index, void** data) const;
   bool  getVector   (DWORD index, BVector &data) const;
   bool  getData     (DWORD index, void* data, WORD size) const;
   bool  getString   (DWORD index, BCHAR_T* data, WORD size) const;
   //bool  getString   (DWORD index, BSimString &data) const;
   //template<typename StringType>
   //bool  getString   (DWORD index, StringType& data) const;
   template<typename StringType>
   bool getString(DWORD index, StringType& data) const
   {
      if (index >= mNumEntries)
         return (false);

      return (mEntries[index].getString(data));
   }

   long  getDataSize (DWORD index) const;
   BYTE  getDataType (DWORD index) const;
   bool  setDataType (DWORD index, BYTE type);
   DWORD getNumberEntries(void) const { return(mNumEntries); }

   bool  isSetFlag   (DWORD index, BYTE flags) const;
   void  setFlag     (DWORD index, BYTE flags);
   void  clearFlag   (DWORD index, BYTE flags);

protected:
   BDataSet();

   BYTE  getFlags    (DWORD index) const;
   void  dataChanged (DWORD index);

   BDataEntry*             mEntries;
   DWORD                   mNumEntries;
   BArray<BDataListener*>  mListeners;
};

#endif   // DATASET_H
