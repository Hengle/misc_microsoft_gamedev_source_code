//============================================================================
//
//  DataEntry.h
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================


#pragma once 

#ifndef DATAENTRY_H
#define DATAENTRY_H

//#include "bstring.h"

//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
//class BSimString;
class BSerialObject;
class BSerialWriter;
class BSerialReader;

//----------------------------------------------------------------------------
//  Macros
//----------------------------------------------------------------------------

// set alignment to 1 byte alignment
#pragma pack(1)

//----------------------------------------------------------------------------
//  Class BVariableHeader
//----------------------------------------------------------------------------
class BDataEntryHeader
{
public:
   WORD  mCurSize;
   WORD  mMaxSize;
};

//----------------------------------------------------------------------------
//  Class BDataEntry
//----------------------------------------------------------------------------
class BDataEntry
{
public:
   enum
   {
      cTypeNone,
      cTypeFloat,
      cTypeLong,
      cTypeShort,
      cTypeByte,
      cTypeBool,
      cTypeMaxFixed = cTypeBool,

      // Only a pointer to memory, this will not be deleted
      cTypePointer,

      cTypeInt64,

      // NOTE: every type after this point allocates memory
      cTypeVariable,
      cTypeVector,
      cTypeString,         // this is similar to type variable, but more specific

      cTypeMaxVariable
   };

   enum
   {
      cMaskType = 0x0F,
      cMaskFlags = 0xF0
   };

   BDataEntry();
   ~BDataEntry();

   BDataEntry& operator=(const BDataEntry& entry);

   bool  isSetFlag(BYTE flag) const { return(mInfo&(flag&cMaskFlags)?true:false); }
   void  setFlag(BYTE flag)         { mInfo|=(flag&cMaskFlags); }
   void  clearFlag(BYTE flag)       { mInfo&=~(flag&cMaskFlags); }
   BYTE  getFlags(void) const       { return((BYTE)(mInfo&cMaskFlags)); }
   BYTE  getType(void) const        { return((BYTE)(mInfo&cMaskType)); }
   void  setType(BYTE type);

   bool  setInt64  (int64 data);
   bool  setUInt64 (uint64 data);
   bool  setLong   (long  data);
   bool  setDWORD  (DWORD data);
   bool  setFloat  (float data);
   bool  setShort  (short data);
   bool  setWORD   (WORD  data);
   bool  setBool   (bool  data);
   bool  setBOOL   (BOOL  data);
   bool  setChar   (char  data);
   bool  setBYTE   (BYTE  data);
   bool  setPointer(void* ptr);
   bool  setVector (const BVector&  data);
   bool  setData   (const void* data, WORD size);
   bool  setString (const BCHAR_T* string);
   bool  setString (const BSimString &string);

   bool   getInt64 (int64 &data) const;
   bool   getUInt64(uint64 &data) const;
   bool   getLong  (long  &data) const;
   bool   getDWORD (DWORD &data) const;
   bool   getFloat (float &data) const;
   bool   getShort (short &data) const;
   bool   getWORD  (WORD  &data) const;
   bool   getBool  (bool  &data) const;
   bool   getBOOL  (BOOL  &data) const;
   bool   getChar  (char  &data) const;
   bool   getBYTE  (BYTE  &data) const;
   bool   getPointer(void** ptr) const;
   bool   getVector(BVector &data) const;
   bool   getData  (void* data, WORD size) const;
   bool   getString(BCHAR_T* string, WORD length) const;
   //bool   getString(BSimString &string) const;
   //template<typename StringType>
   //bool   getString(StringType& string) const;
   template<typename StringType>
   bool getString(StringType& string) const
   {
      if (getType()!=cTypeString)
         return (false); 

      if (!mData.d4)
         return (false);

      string.set((BCHAR_T*)mData.d4);
      return (true);
   }

   long getSize(void) const;

//   virtual HRESULT serialize(BSerialWriter&);
//   virtual HRESULT deserialize(BSerialReader&);
//   virtual long    serializedSize(void);

protected:
   void  releaseVariableData();
   bool  allocateVariableData(WORD size);

   union
   {
      BYTE    *d4;
      __int64 d8;
   } mData;

   // rg [9/4/08] - Moved this down so d4/d8 are aligned, and added padding;
   // dpm [9/5/08] - Removed the padding and changed mInfo to a uint instead
   uint mInfo;
};

// reset to default packing mode
#pragma pack()

#endif   // DATA_ENTRY
