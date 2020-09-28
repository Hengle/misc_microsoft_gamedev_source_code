//============================================================================
// settings.h
//
// Copyright (c) 2002-2006 Ensemble Studios
//============================================================================

#ifndef SETTINGS_H
#define SETTINGS_H

// Includes
#include "dataset.h"
#include "string/stringtable.h"
#include "xmlreader.h"

// Externs
class BFile;
class BChunkReader;
class BChunkWriter;

// Typedefs
class BSettingDef
{
   public:
      long     mEnum;
      char     mName[64];
      BYTE     mType;
      BYTE     mFlags;
};

//============================================================================
// BSettings
//============================================================================
class BSettings : public BDataSet
{
   public:
                     BSettings(const BSettingDef* settings, DWORD numSettings);
                     BSettings(const BSettings* settings, DWORD numSettings);

      virtual        ~BSettings() {}

      BSettings&     operator=(const BSettings& settings);

      bool  setInt64N   (const char* name, int64 data);
      bool  setLongN    (const char* name, long  data);
      bool  setDWORDN   (const char* name, DWORD data);
      bool  setFloatN   (const char* name, float data);
      bool  setShortN   (const char* name, short data);
      bool  setWORDN    (const char* name, WORD  data);
      bool  setBoolN    (const char* name, bool  data);
      bool  setBOOLN    (const char* name, BOOL  data);
      bool  setCharN    (const char* name, char  data);
      bool  setBYTEN    (const char* name, BYTE  data);
      bool  setPointerN (const char* name, void* data);
      bool  setVectorN  (const char* name, const BVector&  data);
      bool  setDataN    (const char* name, const void* data, WORD size);
      bool  setStringN  (const char* name, const char* data);
      bool  setStringN  (const char* name, const BSimString &data);

      bool  getInt64N   (const char* name, int64 &data) const;
      bool  getLongN    (const char* name, long  &data) const;
      bool  getDWORDN   (const char* name, DWORD &data) const;
      bool  getFloatN   (const char* name, float &data) const;
      bool  getShortN   (const char* name, short &data) const;
      bool  getWORDN    (const char* name, WORD  &data) const;
      bool  getBoolN    (const char* name, bool  &data) const;
      bool  getBOOLN    (const char* name, BOOL  &data) const;
      bool  getCharN    (const char* name, char  &data) const;
      bool  getBYTEN    (const char* name, BYTE  &data) const;
      bool  getPointerN (const char* name, void** data) const;
      bool  getVectorN  (const char* name, BVector &data) const;
      bool  getDataN    (const char* name, void* data, WORD size) const;
      bool  getStringN  (const char* name, char* data, WORD size);
      bool  getStringN  (const char* name, BSimString& data);

      BYTE  getDataTypeN(const char* name) const;

      bool  isSetFlagN  (const char* name, BYTE flags);
      void  setFlagN    (const char* name, BYTE flags);
      void  clearFlagN  (const char* name, BYTE flags);

      bool  loadFromFile(long dirID, const BCHAR_T* fileName);
      bool  loadFromNode(const BXMLNode node);
      bool  writeToFile (long dirID, const BCHAR_T* fileName);
      bool  writeToFile (BFile* pFile);
      bool  load        (BChunkReader* reader);
      bool  save        (BChunkWriter* writer);

      // Lets me translate the name to the enum type;
      DWORD  nameToIndex(const char* name) const;

   protected:
      enum
      {
         cNameHashError = 0xffffffff
      };

      BSettings();
      BStringTable<DWORD, false, 256, hash, BSimString>  mNameToEnum;
};

#endif
