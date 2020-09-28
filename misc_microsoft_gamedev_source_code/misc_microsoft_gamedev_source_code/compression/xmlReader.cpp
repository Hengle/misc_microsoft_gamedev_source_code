//===================================================================================================================================================
//
//  File: xmlReader.cpp
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//===================================================================================================================================================
#include "compression.h"
#include "xmlReader.h"
#include "file\win32FileStream.h"
#include "resource\ecfUtils.h"
#include "stream\byteStream.h"
#include "string\convertToken.h"
#include "compressedStream.h"
#include "consoleOutput.h"

#define DISABLE_XMB_FILES 0
#define CHECK_RETURN_VALUES 0
#define MISSING_XML_WARNING 0

#ifdef BUILD_FINAL
   #undef DISABLE_XMB_FILES
   #define DISABLE_XMB_FILES 0
   
   #undef CHECK_RETURN_VALUES
   #define CHECK_RETURN_VALUES 0
#endif

static const uint cDefaultStringBufferSize = 8192U;
static const char* gpEmptyString = "";

IStreamFactory*  BXMLReader::mpStreamFactory;
bool BXMLReader::mXMBEnabled = true;

BXMLReader::BXMLReader() :
   mDirID(0),
   mpXMLDoc(NULL),
   mpXMXData(NULL),
   mFileType(cCFTInvalid),
   mValid(false),
   mXMXVariantData(NULL, 0)
{
   BCOMPILETIMEASSERT(sizeof(long) == sizeof(int));       
   BCOMPILETIMEASSERT(sizeof(DWORD) == sizeof(uint));       
}

BXMLReader::~BXMLReader()
{
   reset();
}

void BXMLReader::reset(void)
{
   mDirID = 0;
   mFilename.empty();
   
   if (mpXMLDoc)
   {
      delete mpXMLDoc;
      mpXMLDoc = NULL;
   }

   if (mpXMXData)
   {
      alignedFree(mpXMXData);
      mpXMXData = NULL;
   }
 
   mFileType = cCFTInvalid;  
   mValid = false;
   
   mStringBuffer.clear();
   
   mXMXVariantData.set(NULL, 0);
}

bool BXMLReader::load(long dirID, const char* pFilename, uint loadFlags, IStreamFactory* pStreamFactory)
{
   BDEBUG_ASSERT(pFilename);
   
   bool loadBinary     = (loadFlags & XML_READER_IGNORE_BINARY) == 0;
   bool discardOnClose = (loadFlags & XML_READER_LOAD_DISCARD_ON_CLOSE) != 0;
      
#if DISABLE_XMB_FILES
   loadBinary = false;
#endif

   if (!mXMBEnabled)
      loadBinary = false;

#if XML_READER_DEBUG_LOGGING
   debugLog("BXMLReader::load: %i %s", dirID, pFilename);
#endif

   reset();
   
   if (!pFilename)   
      return false;

   BWin32FileStreamFactory win32FileStreamFactory;
   if (!pStreamFactory)
   {
      pStreamFactory = mpStreamFactory;
      if (!pStreamFactory)
         pStreamFactory = &win32FileStreamFactory;
   }
   
   BFixedStringMaxPath filename(pFilename);
   filename.tolower();
   
   BFixedStringMaxPath xmbFilename(filename);
   xmbFilename += ".xmb";
   
   BStream* pStream = NULL;
   eXMLFileType fileType = cCFTInvalid;
   
   uint streamFlags = (eStreamFlags)(cSFReadable | cSFOpenExisting);
   if (discardOnClose)
      streamFlags |= cSFDiscardOnClose;
   
   bool useXMB = false;
   if (loadBinary)
   {
      uint64 xmbFileTime, xmlFileTime;
      const bool foundXMB = pStreamFactory->getFileTime(dirID, BString(xmbFilename), xmbFileTime);
      const bool foundXML = pStreamFactory->getFileTime(dirID, BString(filename), xmlFileTime);
      
      if ((foundXMB) && (foundXML))      
      {
         if (xmbFileTime >= xmlFileTime)
            useXMB = true;
         else
            gConsoleOutput.warning("BXMLReader::load:: XML file \"%s\" is newer than \"%s\"! Ignoring XMB file.", filename.getPtr(), xmbFilename.getPtr());
      }
      else if (foundXMB)
      {
         useXMB = true;

#if MISSING_XML_WARNING
         gConsoleOutput.warning("BXMLReader::load:: XML file \"%s\" is missing! Using XMB file \"%s\".", filename.getPtr(), xmbFilename.getPtr());
#endif
      }
   }
   
   if (useXMB) 
   {
      pStream = pStreamFactory->create(dirID, xmbFilename.getPtr(), (eStreamFlags)streamFlags);
      if (pStream)
      {
         fileType = cXFTXMB;
         filename = xmbFilename;
      }
   }
   
   if (!pStream)   
   {  
      pStream = pStreamFactory->create(dirID, filename.getPtr(), (eStreamFlags)streamFlags);
      if (!pStream)
      {
         gConsoleOutput.error("BXMLReader::load: Unable to open file: %s", filename.getPtr());
      
         return false;
      }
             
      if (filename.findRight(".xmb") == (int)(filename.length() - 4))
         fileType = cXFTXMB;
      else
         fileType = cXFTXML;
   }

   const bool success = load(pStream, fileType);
   
   delete pStream;
   
   if (success)
   {
      mDirID = dirID;
      mFilename = filename;
      mStringBuffer.resize(cDefaultStringBufferSize);
      mValid = true;
   }
         
   return success;
}

bool BXMLReader::loadXML(const BYTE* pData, uint dataLen)
{
   mpXMLDoc = new BXMLDocument;
   
   if (FAILED(mpXMLDoc->parse((const char*)pData, dataLen)))
   {
      delete mpXMLDoc;
      mpXMLDoc = NULL;
      
      return false;
   }
      
   return true;
}

bool BXMLReader::loadXMB(const BYTE* pData, uint dataLen)
{
   BECFFileReader fileReader(BConstDataBuffer(pData, dataLen));
   
   if (!fileReader.checkHeader(true))
      return false;
      
   if (fileReader.getID() != cXMXECFFileID)
      return false;

   if (!fileReader.check())
      return false;      
      
   const int chunkIndex = fileReader.findChunkByID(cXMXPackedDataChunkID);
   if (cInvalidIndex == chunkIndex)
      return false;
      
   const BECFChunkHeader* pChunk = fileReader.getChunkByIndex(chunkIndex);
   if (!pChunk->getResourceBitFlag(cECFChunkResFlagIsDeflateStream))
      return false;
   
   uint chunkDataLen = 0;
   const BYTE* pChunkData = fileReader.getChunkDataByIndex(chunkIndex, chunkDataLen);
   if (!pChunkData)
      return false;
   if ((chunkDataLen < sizeof(DWORD)) || (chunkDataLen >= 64*1024*1024))
      return false;

   BByteStream byteStream(pChunkData, chunkDataLen);
   
   BInflateStream inflateStream(byteStream);
   if (inflateStream.errorStatus())
      return false;
      
   if (!inflateStream.sizeKnown())      
      return false;
    
   const uint64 decompDataSize = inflateStream.size();
   if ((decompDataSize < sizeof(BPackedXMXData)) || (decompDataSize >= 256U*1024*1024U))
      return false;
      
   mpXMXData = reinterpret_cast<BPackedXMXData*>(alignedMalloc((uint)decompDataSize, 16U));
   if (!mpXMXData)
      return false;
      
   if (inflateStream.readBytes(mpXMXData, (uint)decompDataSize) != decompDataSize)
   {
      alignedFree(mpXMXData);
      mpXMXData = NULL;
      return false;
   }
   
   inflateStream.close();
   if (inflateStream.errorStatus())
   {
      alignedFree(mpXMXData);
      mpXMXData = NULL;
      return false;
   }
   
   if (!mpXMXData->unpack(BDataBuffer(mpXMXData, (uint)decompDataSize)))
   {
      alignedFree(mpXMXData);
      mpXMXData = NULL;
      return false;
   }
   
   mXMXVariantData.set(mpXMXData->getVariantData().getPtr(), mpXMXData->getVariantData().getSize());
   
   return true;
}

bool BXMLReader::load(BStream* pStream, eXMLFileType fileType)
{
   const bool success = loadFromStream(pStream, fileType);
   
   const char* pFileTypeDesc = (fileType == cXFTXML) ? "XML" : "XMB";
   if (!success)
   {
      gConsoleOutput.error("BXMLReader::load: Failed loading %s file: %s", pFileTypeDesc, pStream->getName().getPtr());
   }
   else
   {
      gConsoleOutput.resource("BXMLReader::load: Loaded %s file: %s", pFileTypeDesc, pStream->getName().getPtr());
   }
   
   return success;
}

bool BXMLReader::loadFromStream(BStream* pStream, eXMLFileType fileType)
{
   reset();
   
   if (!pStream)      
      return false;
      
   BByteArray fileData;
   if (pStream->sizeKnown())      
   {
      uint64 fileSize = pStream->size();
      if (fileSize >= 256U*1024U*1024U)
         return false;
         
      fileData.resize((uint)fileSize);
      
      if (pStream->readBytes(fileData.getPtr(), fileData.getSizeInBytes()) != fileData.getSizeInBytes())
         return false;
   }
   else
   {  
      const uint cBufSize = 4096;
      BYTE buf[cBufSize];
      for ( ; ; )
      {
         const uint bytesRead = pStream->readBytes(buf, cBufSize);
         if (bytesRead < cBufSize)
         {
            if (pStream->errorStatus())
               return false;
            
            if (!bytesRead)
               break;
         }
         
         fileData.pushBack(buf, bytesRead);
      }
   }
      
   bool success = false;
   
   if (cXFTXML == fileType)
      success = loadXML(fileData.getPtr(), fileData.getSizeInBytes());
   else if (cXFTXMB == fileType)
      success = loadXMB(fileData.getPtr(), fileData.getSizeInBytes());
   
   if (!success)   
   {
      reset();   
      return false;
   }
   
   mDirID = 0;
   mFilename = pStream->getName();
   mValid = true;   
   
   return true;
}

BXMLNode BXMLReader::getRootNode(void) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return BXMLNode(NULL, 0U);
   }
   
   return BXMLNode(this, 0U);   
}

#if XML_READER_DEBUG_LOGGING
void BXMLReader::debugLog(const char* pMsg, ...) const
{
   va_list args;
   va_start(args, pMsg);

   traceV(pMsg, args, true);
   
   va_end(args);
}
#endif

const char* BXMLAttribute::getNamePtr(void) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return gpEmptyString;
   }
      
   if (mpReader->mpXMLDoc)      
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      return attrib.mName.getPtr();
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      const char* pStr = BXMXVariantHelpers::getVariantAsANSIString(NULL, 0, mpReader->mXMXVariantData, attrib.mName);
      BDEBUG_ASSERT(pStr);
      return pStr;
   }
}

bool BXMLAttribute::getValueAsInt8(int8& val) const
{
   int32 temp;
   if (!getValueAsInt32(temp))
      return false;
   val = (int8)Math::Clamp<int32>(temp, INT8_MIN, INT8_MAX);
   return true;
}

bool BXMLAttribute::getValueAsUInt8(uint8& val) const
{
   uint32 temp;
   if (!getValueAsUInt32(temp))
      return false;
   val = (uint8)Math::Min<uint32>(temp, UINT8_MAX);
   return true;
}

bool BXMLAttribute::getValueAsInt16(int16& val) const
{
   int32 temp;
   if (!getValueAsInt32(temp))
      return false;
   val = (int16)Math::Clamp<int32>(temp, INT16_MIN, INT16_MAX);
   return true;
}

bool BXMLAttribute::getValueAsUInt16(uint16& val) const
{
   uint32 temp;
   if (!getValueAsUInt32(temp))
      return false;
   val = (uint16)Math::Min<uint32>(temp, UINT16_MAX);
   return true;
}

bool BXMLAttribute::getValueAsInt32(int32& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   bool success = false;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (!attrib.mText.isEmpty())
      {
         val = attrib.mText.asLong();
         success = true;
      }
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      success = BXMXVariantHelpers::convertVariantToInt(attrib.mText, val, mpReader->mXMXVariantData);
   }         

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsInt32: %u %i", success, val);
#endif   

   return success;
}

bool BXMLAttribute::getValueAsUInt32(uint32& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }
      
   bool success = false;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (!attrib.mText.isEmpty())
      {
         const int64 ival = attrib.mText.asInt64();
         val = (uint32)Math::Clamp<int64>(ival, 0, (int64)UINT32_MAX);
         success = true;
      }
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      success = BXMXVariantHelpers::convertVariantToUInt(attrib.mText, val, mpReader->mXMXVariantData);
   }         

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsUInt32: %u %u", success, val);
#endif   

   return success;
}

bool BXMLAttribute::getValueAsInt64(int64& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   bool success = false;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (!attrib.mText.isEmpty())
      {
         val = attrib.mText.asInt64();
         success = true;
      }
   }
   else
   {
      // only supporting xml documents for now, does anyone else want to use Int64 support in XMBs?
      //const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      //success = BXMXVariantHelpers::convertVariantToUInt(attrib.mText, val, mpReader->mXMXVariantData);
      success = false;
   }         

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsInt64: %u %I64d", success, val);
#endif

   return success;
}

bool BXMLAttribute::getValueAsUInt64(uint64& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   bool success = false;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (!attrib.mText.isEmpty())
      {
         val = attrib.mText.asUInt64();
         success = true;
      }
   }
   else
   {
      // only supporting xml documents for now, does anyone else want to use UInt64 support in XMBs?
      //const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      //success = BXMXVariantHelpers::convertVariantToUInt(attrib.mText, val, mpReader->mXMXVariantData);
      success = false;
   }         

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsUInt64: %u %I64u", success, val);
#endif

   return success;
}

bool BXMLAttribute::getValueAsLong(long& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   bool success;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (attrib.mText.isEmpty())
         return false;
      val = attrib.mText.asLong();
      success = true;
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      success = BXMXVariantHelpers::convertVariantToInt(attrib.mText, (int&)val, mpReader->mXMXVariantData);
   }

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsLong: %u %s %i", success, getNamePtr(), val);
#endif   
   
   return success;
}

bool BXMLAttribute::getValueAsFloat(float& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      // If this assert fires, you most likely didn't initialize out to something valid before calling this method!
      // I would rather just set out to 0 here, but I can't because the old BXMLAttribute class didn't.
      return false;
   }

   bool success;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (attrib.mText.isEmpty())
         success = false;
      else
      {
         val = attrib.mText.asFloat();
         success = true;
      }
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      success = BXMXVariantHelpers::convertVariantToFloat(attrib.mText, val, mpReader->mXMXVariantData);
      
   }  

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsFloat: %u %s %f", success, getNamePtr(), val);
#endif   
   
   if (!success)
   {
      return false;
   }    
   
   return true;
}

bool BXMLAttribute::getValueAsHalfFloat(BHalfFloat& val) const
{
   float temp;
   if (!getValueAsFloat(temp))
      return false;
   val = temp;
   return true;
}

bool BXMLAttribute::getValueAsAngle(float& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      // If this assert fires, you most likely didn't initialize out to something valid before calling this method!
      // I would rather just set out to 0 here, but I can't because the old BXMLAttribute class didn't.
      return false;
   }

   bool success;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (attrib.mText.isEmpty())
         success = false;
      else
      {
         val = attrib.mText.asFloat();
         success = true;
      }
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      success = BXMXVariantHelpers::convertVariantToFloat(attrib.mText, val, mpReader->mXMXVariantData);
   }  

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsAngle: %u %s %f", success, getNamePtr(), val);
#endif   
   
   if (!success)
   {
      return false;
   }
       
   val *= cRadiansPerDegree;
   return true;
}

bool BXMLAttribute::getValueAsDWORD(DWORD& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }
   
   bool success = false;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (!attrib.mText.isEmpty())
      {
         // This is what the original XML reader did, yes it's technically wrong.
         val = (DWORD)attrib.mText.asLong();
         success = true;
      }
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      
      int ival;
      success = BXMXVariantHelpers::convertVariantToInt(attrib.mText, ival, mpReader->mXMXVariantData);
      
      // This is what the original XML reader did, yes it's technically wrong.
      val = (DWORD)ival;
   }         

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsDWORD: %u %u", success, val);
#endif   
   
   if (!success)
   {
      return false;
   }
   
   return true;
}

bool BXMLAttribute::getValueAsBool(bool& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      BDEBUG_ASSERT(Utils::IsValidBool(val));
      return false;
   }
      
   bool success = false;       
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (!attrib.mText.isEmpty())
         success = convertTokenToBool(attrib.mText, val);
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      success = BXMXVariantHelpers::convertVariantToBool(attrib.mText, val, mpReader->mXMXVariantData);
   }   

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsBool: %u %s %u", success, getNamePtr(), val);
#endif   
   
   if (!success)
   {
      return false;
   }
   return true;
}

bool BXMLAttribute::getValueAsVector(BVector& val) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      val.set(0.0f, 0.0f, 0.0f);
      return false;
   }

   bool success;
   if (mpReader->mpXMLDoc)
   {
      const BXMLDocument::BAttribute& attrib = mpReader->getDocAttribute(mNodeIndex, mAttributeIndex);
      if (attrib.mText.isEmpty())
      {
         val.set(0.0f, 0.0f, 0.0f);
         success = false;
      }
      else
         success = attrib.mText.convertToVector3(&val.x);
   }
   else
   {
      const BPackedXMXData::BAttribute& attrib = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
      success = BXMXVariantHelpers::convertVariantToVector(attrib.mText, val, mpReader->mXMXVariantData);
   }   

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLAttribute::getValueAsVector: %u %s %f %f %f", success, getNamePtr(), val.x, val.y, val.z);
#endif   
   
   if (!success)
   {
      val.set(0.0f, 0.0f, 0.0f);
      return false;
   }
   
   return true;
}

bool BXMLAttribute::isValueEmpty(void) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return true;
   }

   if (mpReader->mpXMLDoc)
      return mpReader->getDocAttribute(mNodeIndex, mAttributeIndex).getText().isEmpty();

   const BPackedXMXData::BAttribute& attr = mpReader->getXMXAttribute(mNodeIndex, mAttributeIndex);
   const uint variantValue = attr.mText;

   const eXMXVariantType variantType = BXMXVariantHelpers::getVariantType(variantValue);

   if (variantType == cXMXVTNull)
      return true;

   BString temp;
   const char* p = getValuePtr(temp);
   return p[0] == '\0';
}

long BXMLAttribute::compareValue(const char* pStr, bool caseSensitive) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return -1;
   }

   BString temp;   
   const char* pDst = getValuePtr(temp);

   return caseSensitive ? strcmp(pDst, pStr) : _stricmp(pDst, pStr);
}

const char* BXMLNode::getNamePtr(void) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return gpEmptyString;
   }

   const char* pStr;
   if (mpReader->mpXMLDoc)      
   {
      const BXMLDocument::BNode& node = mpReader->getDocNode(mNodeIndex);
      pStr = node.getName().getPtr();
   }
   else
   {
      const BPackedXMXData::BNode& node = mpReader->getXMXNode(mNodeIndex);
      pStr = BXMXVariantHelpers::getVariantAsANSIString(NULL, 0, mpReader->mXMXVariantData, node.mName);
   }
   
   BDEBUG_ASSERT(pStr);
   return pStr;
}

long BXMLNode::compareText(const char* pStr, bool caseSensitive) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return -1;
   }
   
   BString temp;   
   const char* pDst = getTextPtr(temp);
   
   return caseSensitive ? strcmp(pDst, pStr) : _stricmp(pDst, pStr);
}

bool BXMLNode::isTextEmpty(void) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return true;
   }

   if (mpReader->mpXMLDoc)
      return mpReader->getDocNode(mNodeIndex).getText().isEmpty();
      
   const BPackedXMXData::BNode& node = mpReader->getXMXNode(mNodeIndex);
   const uint variantValue = node.mText;

   const eXMXVariantType variantType = BXMXVariantHelpers::getVariantType(variantValue);
   
   if (variantType == cXMXVTNull)
      return true;
    
   BString temp;
   const char* p = getTextPtr(temp);
   return p[0] == '\0';
}

bool BXMLNode::getTextAsFloat(float& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }
      
   bool success;
   if (mpReader->mpXMLDoc)
      success = mpReader->getDocNode(mNodeIndex).getTextAsFloat(out);
   else
      success = BXMXVariantHelpers::convertVariantToFloat(mpReader->getXMXNode(mNodeIndex).mText, out, mpReader->mXMXVariantData);

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getTextAsFloat: %u %s %f", success, getNamePtr(), out);
#endif   
   
   if (!success)
   {
      return false;
   }
   
   return true;
}

bool BXMLNode::getTextAsHalfFloat(BHalfFloat& out) const
{
   float temp;
   if (!getTextAsFloat(temp))
      return false;
   out = temp;
   return true;
}

bool BXMLNode::getTextAsAngle(float& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   if (!getTextAsFloat(out))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getTextAsAngle: %u %s %f", 0, getNamePtr(), out);
#endif      
      
      return false;
   }
            
   out *= cRadiansPerDegree;

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getTextAsAngle: %u %s %f", 1, getNamePtr(), out);
#endif   
   
   return true;
}

bool BXMLNode::getTextAsBool(bool& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   bool success;      
   if (mpReader->mpXMLDoc)
      success = mpReader->getDocNode(mNodeIndex).getTextAsBool(out);
   else
      success = BXMXVariantHelpers::convertVariantToBool(mpReader->getXMXNode(mNodeIndex).mText, out, mpReader->mXMXVariantData);

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getTextAsBool: %u %s %u", success, getNamePtr(), out);
#endif   
   
   if (!success)   
   {
      return false;
   }
      
   return true;
}

bool BXMLNode::getTextAsInt(int& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }
      
   bool success;
   if (mpReader->mpXMLDoc)
      success = mpReader->getDocNode(mNodeIndex).getTextAsInt(out);
   else
      success = BXMXVariantHelpers::convertVariantToInt(mpReader->getXMXNode(mNodeIndex).mText, out, mpReader->mXMXVariantData);

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getTextAsBool: %u %s %i", success, getNamePtr(), out);
#endif   
   
   if (!success)
   {
      return false;
   }
   
   return true;
}

bool BXMLNode::getTextAsLong(long& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }
   
   bool success;   
   if (mpReader->mpXMLDoc)
      success = mpReader->getDocNode(mNodeIndex).getTextAsInt((int&)out);
   else
      success = BXMXVariantHelpers::convertVariantToInt(mpReader->getXMXNode(mNodeIndex).mText, (int&)out, mpReader->mXMXVariantData);

#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLNode::getTextAsLong: %u %s %i", success, getNamePtr(), out);
#endif   
   
   if (!success)
   {
      return false;
   }

   return true;      
}

bool BXMLNode::getTextAsDWORD(DWORD& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }
   
   bool success;
   if (mpReader->mpXMLDoc)
      success = mpReader->getDocNode(mNodeIndex).getTextAsDWORD(out);
   else
   {
      // This is what the original XML reader did, yes it's technically wrong.
      int ival;
      success = BXMXVariantHelpers::convertVariantToInt(mpReader->getXMXNode(mNodeIndex).mText, ival, mpReader->mXMXVariantData);
      out = (DWORD)ival;
   }

#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLNode::getTextAsDWORD: %u %s %u", success, getNamePtr(), out);      
#endif   
   
   if (!success)
   {
      return false; 
   }      
      
   return true;      
}

bool BXMLNode::getTextAsInt8(int8& out) const
{
   int32 temp;
   if (!getTextAsInt32(temp))
      return false;
   out = (int8)Math::Clamp<int32>(temp, INT8_MIN, INT8_MAX);
   return true;
}

bool BXMLNode::getTextAsUInt8(uint8& out) const
{
   uint32 temp;
   if (!getTextAsUInt32(temp))
      return false;
   out = (uint8)Math::Min<uint32>(temp, UINT8_MAX);
   return true;
}

bool BXMLNode::getTextAsInt16(int16& out) const
{
   int32 temp;
   if (!getTextAsInt32(temp))
      return false;
   out = (int16)Math::Clamp<int32>(temp, INT16_MIN, INT16_MAX);
   return true;
}

bool BXMLNode::getTextAsUInt16(uint16& out) const
{
   uint32 temp;
   if (!getTextAsUInt32(temp))
      return false;
   out = (uint16)Math::Min<uint32>(temp, UINT16_MAX);
   return true;
}

bool BXMLNode::getTextAsInt32(int32& out) const
{
   return getTextAsInt(out);
}

bool BXMLNode::getTextAsUInt32(uint32& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   bool success;
   if (mpReader->mpXMLDoc)
   {
      int64 temp; 
      success = mpReader->getDocNode(mNodeIndex).getTextAsInt64(temp);
      if (success)
         out = (uint32)Math::Clamp<int64>(temp, 0, UINT32_MAX);
   }
   else
   {
      success = BXMXVariantHelpers::convertVariantToUInt(mpReader->getXMXNode(mNodeIndex).mText, out, mpReader->mXMXVariantData);
   }

#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLNode::getTextAsUInt32: %u %s %u", success, getNamePtr(), out);      
#endif   

   if (!success)
      return false; 

   return true;      
}

bool BXMLNode::getTextAsUInt64(uint64& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return false;
   }

   bool success;
   if (mpReader->mpXMLDoc)
   {
      int64 temp;
      success = mpReader->getDocNode(mNodeIndex).getTextAsInt64(temp);
      if (success)
         out = static_cast<uint64>(temp);
   }
   else
   {
      // does anyone want int64 support in xmb files?
      //success = BXMXVariantHelpers::convertVariantToUInt(mpReader->getXMXNode(mNodeIndex).mText, out, mpReader->mXMXVariantData);
      success = false;
   }

#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLNode::getTextAsUInt64: %u %s %I64u", success, getNamePtr(), out);      
#endif   

   if (!success)
      return false; 

   return true;      
}

bool BXMLNode::getTextAsVector(BVector& out) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      out.set(0.0f, 0.0f, 0.0f);
      return false;
   }
   
   bool success;
   if (mpReader->mpXMLDoc)
      success = mpReader->getDocNode(mNodeIndex).getTextAsVector(out);
   else
      success = BXMXVariantHelpers::convertVariantToVector(mpReader->getXMXNode(mNodeIndex).mText, out, mpReader->mXMXVariantData);

#if XML_READER_DEBUG_LOGGING
   mpReader->debugLog("BXMLNode::getTextAsLong: %u %s %f %f %f", success, getNamePtr(), out.x, out.y, out.z);
#endif   
   
   if (!success)
   {
      return false;  
   }      
      
   return true;
}

long BXMLNode::getAttributeCount(void) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(false);
      return 0;
   }      
      
   if (mpReader->mpXMLDoc)
      return mpReader->getDocNode(mNodeIndex).getNumAttributes();
   else
      return mpReader->getXMXNode(mNodeIndex).mAttributes.getSize();
}

BXMLAttribute BXMLNode::getAttribute(long index) const
{
   if ( (!getValid()) || ((index < 0) || (index >= getAttributeCount())) )
   {
      BDEBUG_ASSERT(false);
      return BXMLAttribute(NULL, 0, 0);
   }
      
   return BXMLAttribute(mpReader, mNodeIndex, index);
}

BXMLAttribute BXMLNode::getAttribute(const char* pName) const
{
   BDEBUG_ASSERT(getValid() && (pName));
   if ((getValid()) && (pName))
   {      
      if (mpReader->mpXMLDoc)
      {
         const BXMLDocument::BNode& node = mpReader->getDocNode(mNodeIndex);
         for (uint i = 0; i < node.getNumAttributes(); i++)
            if (node.getAttribute(i).mName == pName)
               return BXMLAttribute(mpReader, mNodeIndex, i);
      }
      else
      {
         const BPackedXMXData::BNode& node = mpReader->getXMXNode(mNodeIndex);
         
         for (uint i = 0; i < node.mAttributes.getSize(); i++)
         {
            const char* pAttrName = BXMXVariantHelpers::getVariantAsANSIString(mpReader->mStringBuffer.getPtr(), mpReader->mStringBuffer.getSize(), mpReader->mXMXVariantData, node.mAttributes[i].mName);
            
            if (pAttrName)
            {
               if (_stricmp(pAttrName, pName) == 0)
                  return BXMLAttribute(mpReader, mNodeIndex, i);
            }                  
         }
      }
   }

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getAttribute: %u %s", 0, pName);
#endif   
   
   return BXMLAttribute(NULL, 0, 0);
}

bool BXMLNode::getAttribute(const char* pName, BXMLAttribute* pAttrib) const
{
   BDEBUG_ASSERT(getValid() && (pName));
   if ((getValid()) && (pName))
   {
      if (mpReader->mpXMLDoc)
      {
         const BXMLDocument::BNode& node = mpReader->getDocNode(mNodeIndex);
         for (uint i = 0; i < node.getNumAttributes(); i++)
         {
            if (node.getAttribute(i).mName == pName)
            {
               if (pAttrib)
                  pAttrib->set(mpReader, mNodeIndex, i);
               return true;
            }
         }            
      }
      else
      {
         const BPackedXMXData::BNode& node = mpReader->getXMXNode(mNodeIndex);

         for (uint i = 0; i < node.mAttributes.getSize(); i++)
         {
            const char* pAttrName = BXMXVariantHelpers::getVariantAsANSIString(mpReader->mStringBuffer.getPtr(), mpReader->mStringBuffer.getSize(), mpReader->mXMXVariantData, node.mAttributes[i].mName);

            if (pAttrName)
            {
               if (_stricmp(pAttrName, pName) == 0)
               {
                  if (pAttrib)
                     pAttrib->set(mpReader, mNodeIndex, i);
                  return true;
               }
            }               
         }
      }
   }
   
   if (pAttrib)
      pAttrib->setInvalid();

#if XML_READER_DEBUG_LOGGING      
   mpReader->debugLog("BXMLNode::getAttribute: %u %s", 0, pName);
#endif   
      
   return false;
}

bool BXMLNode::getAttribValueAsBool(const char* pName, bool &out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }

   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getAttribValueAsBool: %u %s %u", false, pName, out);
#endif      
   
      return false;
   }
   
   if (!attr.getValueAsBool(out))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLNode::getAttribValueAsBool: %u %s %u", false, pName, out);  
#endif      
   
      return false;
   }

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getAttribValueAsBool: %u %s %u", true, pName, out);
#endif   
   
   return true;
}

bool BXMLNode::getAttribValueAsLong(const char* pName, long &out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLNode::getAttribValueAsBool: %u %s %u", false, pName, out);
#endif      
      
      return false;
   }

   return attr.getValueAsLong(out);
}

bool BXMLNode::getAttribValueAsFloat(const char* pName, float &out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING      
      mpReader->debugLog("BXMLNode::getAttribValueAsFloat: %u %s %f", false, pName, out);
#endif      
         
      return false;
   }

   return attr.getValueAsFloat(out);      
}

bool BXMLNode::getAttribValueAsHalfFloat(const char* pName, BHalfFloat& out) const
{
   float temp;
   if (!getAttribValueAsFloat(pName, temp))
      return false;
   out = temp;
   return true;
}

bool BXMLNode::getAttribValueAsAngle(const char* pName, float &out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getAttribValueAsAngle: %u %s %f", false, pName, out);
#endif      
      
      return false;
   }

   return attr.getValueAsAngle(out);      
}

bool BXMLNode::getAttribValueAsDWORD(const char* pName, DWORD &out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLNode::getAttribValueAsFloat: %u %s %u", false, pName, out);
#endif      
      
      return false;
   }

   return attr.getValueAsDWORD(out);          
}

bool BXMLNode::getAttribValueAsInt8(const char* pName, int8& out) const
{
   int32 temp;
   if (!getAttribValueAsInt32(pName, temp))
      return false;
   out = (int8)Math::Clamp<int32>(temp, INT8_MIN, INT8_MAX);
   return true;
}

bool BXMLNode::getAttribValueAsUInt8(const char* pName, uint8& out) const
{
   uint32 temp;
   if (!getAttribValueAsUInt32(pName, temp))
      return false;
   out = (uint8)Math::Min<uint32>(temp, UINT8_MAX);
   return true;
}

bool BXMLNode::getAttribValueAsInt16(const char* pName, int16& out) const
{
   int32 temp;
   if (!getAttribValueAsInt32(pName, temp))
      return false;
   out = (int16)Math::Clamp<int32>(temp, INT16_MIN, INT16_MAX);
   return true;
}

bool BXMLNode::getAttribValueAsUInt16(const char* pName, uint16& out) const
{
   uint32 temp;
   if (!getAttribValueAsUInt32(pName, temp))
      return false;
   out = (uint16)Math::Min<uint32>(temp, UINT16_MAX);
   return true;
}

bool BXMLNode::getAttribValueAsInt32(const char* pName, int32& out) const
{
   long temp;
   if (!getAttribValueAsLong(pName, temp))
      return false;
   out = temp;
   return true;
}

bool BXMLNode::getAttribValueAsUInt32(const char* pName, uint32& out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }

   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLNode::getAttribValueAsUInt32: %u %s %u", false, pName, out);
#endif      

      return false;
   }

   return attr.getValueAsUInt32(out);  
}

bool BXMLNode::getAttribValueAsUInt64(const char* pName, uint64& out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }

   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getAttribValueAsUInt64: %u %s %I64u", false, pName, out);
#endif      

      return false;
   }

   return attr.getValueAsUInt64(out);
}

bool BXMLNode::getAttribValueAsVector(const char* pName, BVector& out) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLAttribute attr;
   if (!getAttribute(pName, &attr))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getAttribValueAsVector: %u %s %f %f %f", false, pName, out.x, out.y, out.z);
#endif      
      
      return false;
   }

   return attr.getValueAsVector(out);           
}

BXMLNode BXMLNode::getParent(void) const
{
   if (!getValid())
   {
      return BXMLNode(NULL, 0);
   }
      
   BXMLNode result;
   if (mpReader->mpXMLDoc)      
      result.set(mpReader, mpReader->getDocNode(mNodeIndex).getParentNodeIndex());
   else
      result.set(mpReader, mpReader->getXMXNode(mNodeIndex).mParentNode);
   
   if ((int)result.getNodeIndex() == cInvalidIndex)
      return BXMLNode(NULL, 0);
      
   return result;      
}

long BXMLNode::getNumberChildren(void) const
{
   if (!getValid())
   {
      BDEBUG_ASSERT(0);
      return 0;
   }
   
   if (mpReader->mpXMLDoc)      
      return mpReader->getDocNode(mNodeIndex).getNumChildren();
   else
      return mpReader->getXMXNode(mNodeIndex).mChildren.getSize();
}

BXMLNode BXMLNode::getChild(long index) const
{
   if ( (!getValid()) || ((index < 0) || (index >= getNumberChildren())) )
   {
      BDEBUG_ASSERT(0);
      return BXMLNode(NULL, 0);
   }
      
   if (mpReader->mpXMLDoc)      
      return BXMLNode(mpReader, mpReader->getDocNode(mNodeIndex).getChildNodeIndex((uint)index));
   else
      return BXMLNode(mpReader, mpReader->getXMXNode(mNodeIndex).mChildren[index]);
}

BXMLNode BXMLNode::getChildNode(const char* pName) const
{
   BDEBUG_ASSERT(getValid() && (pName));
   if ((getValid()) && (pName))
   {
      if (mpReader->mpXMLDoc)
      {
         const BXMLDocument::BNode& node = mpReader->getDocNode(mNodeIndex);
         for (uint i = 0; i < node.getNumChildren(); i++)
         {
            if (node.getChild(i)->getName() == pName)
            {
               return BXMLNode(mpReader, node.getChildNodeIndex(i));
            }
         }            
      }
      else
      {
         const BPackedXMXData::BNode& node = mpReader->getXMXNode(mNodeIndex);

         for (uint i = 0; i < node.mChildren.getSize(); i++)
         {
            uint nodeIndex = node.mChildren[i];
            const char* pNodeName = BXMXVariantHelpers::getVariantAsANSIString(mpReader->mStringBuffer.getPtr(), mpReader->mStringBuffer.getSize(), mpReader->mXMXVariantData, mpReader->getXMXNode(nodeIndex).mName);

            if (pNodeName)
            {
               if (_stricmp(pNodeName, pName) == 0)
               {
                  return BXMLNode(mpReader, nodeIndex);
               }
            }               
         }
      }
   }

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getChild: %u %s", false, pName);
#endif   

   return BXMLNode(NULL, 0);
}

bool BXMLNode::getChild(const char* pName, BXMLNode* pNode) const
{
   BDEBUG_ASSERT(getValid() && (pName));
   if ((getValid()) && (pName))
   {
      if (mpReader->mpXMLDoc)
      {
         const BXMLDocument::BNode& node = mpReader->getDocNode(mNodeIndex);
         for (uint i = 0; i < node.getNumChildren(); i++)
         {
            if (node.getChild(i)->getName() == pName)
            {
               if (pNode)
                  pNode->set(mpReader, node.getChildNodeIndex(i));
               return true;
            }
         }            
      }
      else
      {
         const BPackedXMXData::BNode& node = mpReader->getXMXNode(mNodeIndex);

         for (uint i = 0; i < node.mChildren.getSize(); i++)
         {
            uint nodeIndex = node.mChildren[i];
            const char* pNodeName = BXMXVariantHelpers::getVariantAsANSIString(mpReader->mStringBuffer.getPtr(), mpReader->mStringBuffer.getSize(), mpReader->mXMXVariantData, mpReader->getXMXNode(nodeIndex).mName);

            if (pNodeName) 
            {
               if (_stricmp(pNodeName, pName) == 0)
               {
                  if (pNode)
                     pNode->set(mpReader, nodeIndex);
                  return true;
               }
            }               
         }
      }
   }

   if (pNode)
      pNode->set(NULL, 0);

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getChild: %u %s", false, pName);
#endif   
   
   return false;
}

bool BXMLNode::getChildValue(const char* pName, int& value) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLNode node;
   if (!getChild(pName, &node))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getChildValue<int>: %u %s %i", false, pName, value);
#endif      
      
      return false;
   }
      
   return node.getTextAsInt(value);            
}

bool BXMLNode::getChildValue(const char* pName, DWORD& value) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLNode node;
   if (!getChild(pName, &node))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getChildValue<DWORD>: %u %s %u", false, pName, value);
#endif      
      
      return false;
   }

   return node.getTextAsDWORD(value);        
}

bool BXMLNode::getChildValue(const char* pName, bool& value) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLNode node;
   if (!getChild(pName, &node))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getChildValue<bool>: %u %s %u", false, pName, value);
#endif      
      
      return false;
   }

   if (!node.getTextAsBool(value))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getChildValue<bool>: %u %s %u", false, pName, value);
#endif      
      
      return false;
   }

#if XML_READER_DEBUG_LOGGING   
   mpReader->debugLog("BXMLNode::getChildValue<bool>: %u %s %u", true, pName, value);
#endif   
   
   return true;
}

bool BXMLNode::getChildValue(const char* pName, long& value) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLNode node;
   if (!getChild(pName, &node))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLNode::getChildValue<long>: %u %s %i", false, pName, value);
#endif      
      
      return false;
   }
      
   return node.getTextAsLong(value);   
}

bool BXMLNode::getChildValue(const char* pName, float& value) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }
      
   BXMLNode node;
   if (!getChild(pName, &node))
   {
#if XML_READER_DEBUG_LOGGING   
      mpReader->debugLog("BXMLNode::getChildValue<float>: %u %s %f", false, pName, value);
#endif      
      
      return false;
   }

   return node.getTextAsFloat(value);   
}

bool BXMLNode::getChildValue(const char* pName, BHalfFloat& value) const
{
   float temp;
   if (!getChildValue(pName, temp))
      return false;
   value = temp;
   return true;
}

bool BXMLNode::getChildValue(const char* pName, int8& value) const
{
   int32 temp;
   if (!getChildValue(pName, temp))
      return false;
   value = (int8)Math::Clamp<int32>(temp, INT8_MIN, INT8_MAX);
   return true;
}

bool BXMLNode::getChildValue(const char* pName, uint8& value) const
{
   uint32 temp;
   if (!getChildValue(pName, temp))
      return false;
   value = (uint8)Math::Min<uint32>(temp, UINT8_MIN);
   return true;
}

bool BXMLNode::getChildValue(const char* pName, int16& value) const
{
   int32 temp;
   if (!getChildValue(pName, temp))
      return false;
   value = (int16)Math::Clamp<int32>(temp, INT16_MIN, INT16_MAX);
   return true;
}

bool BXMLNode::getChildValue(const char* pName, uint16& value) const
{
   uint32 temp;
   if (!getChildValue(pName, temp))
      return false;
   value = (uint16)Math::Min<uint32>(temp, UINT16_MIN);
   return true;
}

bool BXMLNode::getChildValue(const char* pName, uint32& value) const
{
   if ((!getValid()) || (!pName))
   {
      BDEBUG_ASSERT(0);
      return false;
   }

   BXMLNode node;
   if (!getChild(pName, &node))
   {
#if XML_READER_DEBUG_LOGGING
      mpReader->debugLog("BXMLNode::getChildValue<uint32>: %u %s %u", false, pName, value);
#endif      
      return false;
   }

   return node.getTextAsUInt32(value);   
}