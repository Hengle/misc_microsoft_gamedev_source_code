//============================================================================
// settings.cpp
//
// Copyright (c) 2002-2006 Ensemble Studios
//============================================================================

// INCLUDES
#include "xsystem.h"
#include "settings.h"
#include "chunkreader.h"
#include "chunkwriter.h"
#include "dataentry.h"
#include "dataentry.h"
#include "file.h"

// Constants
static const DWORD cSettingsVersion=0;

//============================================================================
// BSettings::BSettings
//============================================================================
BSettings::BSettings(const BSettingDef* settings, DWORD numSettings) : 
   BDataSet(numSettings)
{
   if (!settings || !numSettings)
   {
      BFATAL_FAIL("");
   }
   else
   {
      for (DWORD idx=0; idx<numSettings; idx++)
      {
         // make sure they are in order
         BASSERT(settings[idx].mEnum == (long)idx);

         // setup flags
         BDataSet::setFlag(idx, settings[idx].mFlags);
         
         // set type
         BDataSet::setDataType(idx, settings[idx].mType);

         // store the lookup
         mNameToEnum.add(settings[idx].mName, settings[idx].mEnum);
      }
   }
}

//============================================================================
// BSettings::BSettings
//============================================================================
BSettings::BSettings(const BSettings* settings, DWORD numSettings) : 
   BDataSet(numSettings)
{
   if (!settings || !numSettings)
   {
      BFATAL_FAIL("");
   }
   else
   {
      for (DWORD idx=0; idx<mNumEntries; idx++)
         mEntries[idx] = settings->mEntries[idx];

      BSimString tag;
      DWORD   val;
      BStringTableIter iter;

      mNameToEnum.clearAll();

      settings->mNameToEnum.iterStart(&iter);
      while(settings->mNameToEnum.iterNext(&iter, &val, &tag))
         mNameToEnum.add(tag.getPtr(), val);
   }
}

//============================================================================
// BSettings::operator=
//
// Assignment operator, assigns the settings regardless of the version differences.
// We still need this because of initialization (from the default profile settings).
//============================================================================
BSettings& BSettings::operator=(const BSettings& settings)
{
   BDataSet::operator=(settings);

   BSimString tag;
   DWORD   val;
   BStringTableIter iter;

   mNameToEnum.clearAll();

   settings.mNameToEnum.iterStart(&iter);
   while(settings.mNameToEnum.iterNext(&iter, &val, &tag))
      mNameToEnum.add(tag.getPtr(), val);

   return(*this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setInt64N(const char* name, int64 data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setInt64(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setLongN(const char* name, long  data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setLong(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setDWORDN(const char* name, DWORD data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setDWORD(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setFloatN(const char* name, float data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setFloat(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setShortN(const char* name, short data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setShort(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setWORDN(const char* name, WORD  data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setWORD(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setBoolN(const char* name, bool  data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setBool(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setBOOLN(const char* name, BOOL  data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setBOOL(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setCharN(const char* name, char  data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setChar(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setBYTEN(const char* name, BYTE  data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setBYTE(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setPointerN(const char* name, void* data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setPointer(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setVectorN(const char* name, const BVector&  data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setVector(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setDataN(const char* name, const void* data, WORD size)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setData(index, data, size));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setStringN(const char* name, const char* data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setString(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::setStringN(const char* name, const BSimString &data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::setString(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getInt64N(const char* name, int64 &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getInt64(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getLongN(const char* name, long  &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getLong(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getDWORDN(const char* name, DWORD &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getDWORD(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getFloatN(const char* name, float &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getFloat(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getShortN(const char* name, short &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getShort(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getWORDN(const char* name, WORD  &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getWORD(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getBoolN(const char* name, bool  &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getBool(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getBOOLN(const char* name, BOOL  &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getBOOL(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getCharN(const char* name, char  &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getChar(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getBYTEN(const char* name, BYTE  &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getBYTE(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getPointerN(const char* name, void** data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getPointer(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getVectorN(const char* name, BVector &data) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getVector(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getDataN(const char* name, void* data, WORD size) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getData(index, data, size));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getStringN(const char* name, char* data, WORD size)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getString(index, data, size));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::getStringN(const char* name, BSimString &data)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::getString(index, data));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE BSettings::getDataTypeN(const char* name) const
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(BDataEntry::cTypeNone);
   return(BDataSet::getDataType(index));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::isSetFlagN(const char* name, BYTE flags)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return(false);
   return(BDataSet::isSetFlag(index, flags));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BSettings::setFlagN(const char* name, BYTE flags)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return;
   BDataSet::setFlag(index, flags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BSettings::clearFlagN(const char* name, BYTE flags)
{
   DWORD index = nameToIndex(name);
   if (index == cNameHashError)
      return;
   BDataSet::clearFlag(index, flags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD BSettings::nameToIndex(const char* name) const
{
   DWORD value=0;
   if (mNameToEnum.find(name, &value))
      return((long)value);

   //BFAIL("BSettings::nameToIndex -- setting not found.");
   return((DWORD)cNameHashError);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::loadFromFile(long dirID, const BCHAR_T* fileName)
{
   BXMLReader reader;

   bool ok = reader.load(dirID, fileName);
   if (!ok)
      return(false);

   return(loadFromNode(reader.getRootNode()));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::loadFromNode(BXMLNode node)
{
   // The format for a settings file is:
   // <Settings>
   //    <Setting Name=""></Setting>
   //    <Setting Name=""></Setting>
   // </Settings>
   if (node.getName().compare(B("Settings"))!=0)
      return(false);
   
   long settingsVersion = 0;
   if (!node.getAttribValueAsLong("Version", settingsVersion))
   {
      BFAIL("Settings node contains no version attribute.");
      return(false);
   }

   BSimString name;
   BXMLNode child;
   DWORD index=0;
   BYTE type;

   long count = node.getNumberChildren();
   for (long idx=0; idx<count; idx++)
   {
      child = node.getChild(idx);
      
      if (child.getName().compare(B("Setting"))!=0)
         continue;

      if (!child.getAttribValue("Name", &name))
         continue;
      
      if (!mNameToEnum.find(name.getPtr(), &index))
         continue;
         
      type = getDataType(index);
      
      BSimString childText;
      child.getText(childText);
      
      bool result = false;
      switch (type)
      {
         case BDataEntry::cTypeBool:
         {
            result = setBool(index, childText.compare(B("true"))==0);
            break;
         }

         case BDataEntry::cTypeByte:
         {
            result = setBYTE(index, (BYTE)childText.asLong());
            break;
         }

         case BDataEntry::cTypeFloat:
         {
            result = setFloat(index, childText.asFloat());
            break;
         }

         case BDataEntry::cTypeLong:
         {
            result = setLong(index, childText.asLong());
            break;
         }

         case BDataEntry::cTypeShort:
         {
            result = setShort(index, (short)childText.asLong());
            break;
         }

         case BDataEntry::cTypeVector:
         {
            const char* pText = childText.getPtr();
            float x, y, z;

            long ret = sscanf_s(pText, "%f, %f, %f", &x, &y, &z);
            if (ret != 3)
               break;

            result = setVector(index, BVector(x,y,z));
            break;
         }

         case BDataEntry::cTypeVariable:
         {
            result = setData(index, childText.getPtr(), (WORD)((childText.length()+1)*sizeof(char)));
            break;
         }
         
         case BDataEntry::cTypeString:
         {
            result = setString(index, childText.getPtr());
            break;
         }
         
         case BDataEntry::cTypePointer:
         {
            BFAIL("BSettings::loadFromNode -- cannot initialize type pointer from a file.");
            break;
         }

         default:
            break;            
      }

      if (!result)
      {
         BSimString msg;
         msg.format(B("BSettings::loadFromNode -- Failed to set value for setting: %s"), name.getPtr());
         BFAIL(msg.getPtr());
      }
   }
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::writeToFile(long dirID, const BCHAR_T* fileName)
{
   BFile file;
   if(!file.openWriteable(dirID, fileName, BFILE_OPEN_ENABLE_BUFFERING))
      return false;
   bool retval=writeToFile(&file);
   file.close();
   return retval;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::writeToFile(BFile* pFile)
{
   if (!pFile)
      return(false);

   // The format for a settings file is:
   // <Settings Version="n">
   //    <Setting Name=""></Setting>
   //    <Setting Name=""></Setting>
   // </Settings>

   BSimString tag;
   DWORD     index, type;
   BStringTableIter iter;
   

   pFile->fprintf(B("<Settings Version=\"%d\">\n"), cSettingsVersion);

   mNameToEnum.iterStart(&iter);
   while(mNameToEnum.iterNext(&iter, &index, &tag))
   {
      pFile->fprintf(B("\t<Setting Name=\"%s\">"), tag.getPtr());

      type = getDataType(index);
      switch (type)
      {
         case BDataEntry::cTypeBool:
         {
            bool val;

            if (getBool(index, val))
            {
               if (val)
                  pFile->fprintf(B("true"));
               else
                  pFile->fprintf(B("false"));
            }
            break;
         }

         case BDataEntry::cTypeByte:
         {
            BYTE val;
            if (getBYTE(index, val))
               pFile->fprintf(B("%d"), val);
            break;
         }

         case BDataEntry::cTypeFloat:
         {
            float val;
            if (getFloat(index, val))
               pFile->fprintf(B("%f"), val);
            break;
         }

         case BDataEntry::cTypeLong:
         {
            long val;
            if (getLong(index, val))
               pFile->fprintf(B("%d"), val);
            break;
         }

         case BDataEntry::cTypeShort:
         {
            short val;
            if (getShort(index, val))
               pFile->fprintf(B("%d"), val);
            break;
         }

         case BDataEntry::cTypeVector:
         {
            BVector val;
            if (getVector(index, val))
               pFile->fprintf(B("%f, %f, %f"), val.x, val.y, val.z);
            break;
         }

         case BDataEntry::cTypeVariable:
         {
            long size = getDataSize(index);
            if (size<=0)
               break;

            BYTE* data = new BYTE[size];
            pFile->write(data, size);
            break;
         }

         case BDataEntry::cTypeString:
         {
            char data[MAX_PATH];
            if (getString(index, data, MAX_PATH))
               pFile->fprintf(B("%s"), BStrConv::toB(data));
            break;
         }
         
         case BDataEntry::cTypePointer:
         default:
            break;            
      }
      pFile->fprintf(B("</Setting>\n"));
   }

   pFile->fprintf(B("</Settings>\n"));

   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::load(BChunkReader* reader)
{
   if (!reader)
      return false;

   long result=reader->readExpectedTag(BCHUNKTAG("ST"));
   if (!result)
   {
      blog("BSettings::load -- error reading tag.");
      return false;
   }

   DWORD version;
   if (!reader->readDWORD(&version))
   {
      blog("BSettings::load -- version missing.");
      return false;
   }

   DWORD count=0;
   if (!reader->readDWORD(&count))
   {
      blog("BSettings::load -- missing number of entries.");
      return false;
   }

   if (count > getNumberEntries())
   {
      blog("BSettings::load -- number of entries saved does not match the current number of entries.");
      return false;
   }

   BSimString tag;
   DWORD index, type;

   for (DWORD idx=0; idx<count; idx++)
   {
      if (!reader->readBSimString(&tag))
      {
         blog("BSettings::load -- failed to load tag.");       
         return false;
      }

      if (!mNameToEnum.find(tag.getPtr(), &index))
      {
         blog("BSettings::load -- invalid setting name specified.");
         return false;
      }

      if (!reader->readDWORD(&type))
      {
         blog("BSettings::load -- failed to load entry type.");
         return false;
      }

      if (type != (DWORD)getDataType(index))
      {
         blog("BSettings::load -- Entry types don't match.");
         return false;
      }

      // read in the value
      switch (type)
      {
         case BDataEntry::cTypeBool:
         {
            bool val;
            if (!reader->readBool(&val))
               return false;

            if (!setBool(index, val))
               return false;

            break;
         }

         case BDataEntry::cTypeByte:
         {
            BYTE val;
            if (!reader->readBYTE(&val))
               return false;

            if (!setBYTE(index, val))
               return false;
            break;
         }

         case BDataEntry::cTypeFloat:
         {
            float val;
            if (!reader->readFloat(&val))
               return false;

            if (!setFloat(index, val))
               return false;
            break;
         }

         case BDataEntry::cTypeLong:
         {
            long val;
            if (!reader->readLong(&val))
               return false;

            if (!setLong(index, val))
               return false;
            break;
         }

         case BDataEntry::cTypeInt64:
         {
            int64 val;
            if (!reader->readInt64(&val))
               return false;

            if (!setInt64(index, val))
               return false;
            break;
         }

         case BDataEntry::cTypeShort:
         {
            short val;
            if (!reader->readShort(&val))
               return false;

            if (!setShort(index, val))
               return false;
            break;
         }

         case BDataEntry::cTypeVector:
         {
            BVector val;
            if (!reader->readVector(&val))
               return false;

            if (!setVector(index, val))
               return false;
            break;
         }

         case BDataEntry::cTypeVariable:
         {
            long size = 0;
            if (!reader->peekArrayLength(&size))
               return false;

            BYTE* data = new BYTE[size];
            if (!reader->readBYTEArray(&size, data, size))
               return false;

            if (!setData(index, data, (WORD)size))
               return false;
            break;
         }

         case BDataEntry::cTypeString:
         {
            BSimString val;
            if (!reader->readBSimString(&val))
               return false;

            if (!setString(index, val))
               return false;
            break;
         }

         case BDataEntry::cTypePointer:
         default:
         break;            
      }
   }

   //Validate our reading of the chunk.
   result=reader->validateChunkRead(BCHUNKTAG("ST"));
   if (result == false)
   {
      blog("BSettings::load -- did not read chunk properly!");
      return false;
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BSettings::save(BChunkWriter* writer)
{
   if (!writer)
      return false;

   BSimString tag;
   DWORD     index, type;
   BStringTableIter iter;

   long mainHandle;
   long result=writer->writeTagPostSized(BCHUNKTAG("ST"), mainHandle);
   if (!result)
   {
      blog("BSettings::save -- error writing tag.");
      return false;
   }

   if (!writer->writeDWORD(cSettingsVersion))
   {
      blog("BSettings::save -- failed to write version.");
      return false;
   }
   
   if (!writer->writeDWORD(getNumberEntries()))
   {
      blog("BSettings::save -- failed to write number of entries.");
      return false;
   }

   mNameToEnum.iterStart(&iter);
   while(mNameToEnum.iterNext(&iter, &index, &tag))
   {
      if (!writer->writeBString(tag))
      {
         blog("BSettings::save -- failed to write settings name.");
         return false;
      }

      type = getDataType(index);
      if (!writer->writeDWORD(type))
      {
         blog("BSettings::save -- failed to write type.");
         return false;
      }

      switch (type)
      {
         case BDataEntry::cTypeBool:
         {
            bool val;

            if (getBool(index, val))
            {
               if (!writer->writeBool(val))
                  return false;
            }
            break;
         }

         case BDataEntry::cTypeByte:
         {
            BYTE val;
            if (getBYTE(index, val))
               if (!writer->writeBYTE(val))
                  return false;
            break;
         }

         case BDataEntry::cTypeFloat:
         {
            float val;
            if (getFloat(index, val))
               if (!writer->writeFloat(val))
                  return false;
            break;
         }

         case BDataEntry::cTypeLong:
         {
            long val;
            if (getLong(index, val))
               if (!writer->writeLong(val))
                  return false;
            break;
         }

         case BDataEntry::cTypeInt64:
         {
            int64 val;
            if (getInt64(index, val))
               if (!writer->writeInt64(val))
                  return false;
            break;
         }

         case BDataEntry::cTypeShort:
         {
            short val;
            if (getShort(index, val))
               if (!writer->writeShort(val))
                  return false;
            break;
         }

         case BDataEntry::cTypeVector:
         {
            BVector val;
            if (getVector(index, val))
               if (!writer->writeVector(val))
                  return false;
            break;
         }

         case BDataEntry::cTypeVariable:
         {
            long size = getDataSize(index);
            if (size<=0)
               break;

            BYTE* data = new BYTE[size];
            if (!writer->writeBYTEArray(size, data))
               return false;            
            break;
         }

         case BDataEntry::cTypeString:
         {
            char data[MAX_PATH];
            data[0] = 0;
            getString(index, data, MAX_PATH);

            BSimString val(data);
            if (!writer->writeBString(val))
               return false;

            break;
         }

         case BDataEntry::cTypePointer:
         default:
         break;            
      }
   }

   result=writer->writeSize(mainHandle);
   if (!result)
   {
      blog("BSettings::save -- failed to write chunk size!");
      return false;
   }
   return true;
}
