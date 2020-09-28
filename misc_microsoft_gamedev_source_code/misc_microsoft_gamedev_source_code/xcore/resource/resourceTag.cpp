//============================================================================
//
//  File: resourceTag.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "resourceTag.h"
#include "file\win32FileStream.h"
#include "file\win32FileUtils.h"
#include "math\randomUtils.h"

BResourceTagBuilder::BResourceTagBuilder() :
   mTagData(sizeof(BResourceTagHeader)),
   mIsFinalized(false)
{

}

void BResourceTagBuilder::clear(void)
{
   mTagData.resize(sizeof(BResourceTagHeader));
   mTagData.setAll(0U);
   
   mTagFilename.empty();
   mSourceFilename.empty();
   mCreatorToolCommandLine.empty();

   mTagData.empty();

   mIsFinalized = false;
}

void BResourceTagBuilder::setSourceFilename(const BString& sourceFilename)
{
   mSourceFilename = sourceFilename;
   mIsFinalized = false;
}

bool BResourceTagBuilder::setSourceDigestAndTimeStamp(BStream& stream, uint64 timeStamp)
{
   const uint cBufSize = 8192;
   uchar buf[cBufSize];
   
   BSHA1Gen sha1Gen;
   uint64 totalBytesRead = 0;
   
   for ( ; ; )
   {
      const uint bytesRead = stream.readBytes(buf, cBufSize);
      if (!bytesRead)
         break;

      sha1Gen.update(buf, bytesRead);
      
      totalBytesRead += bytesRead;
   }
   
   if (stream.errorStatus())
      return false;
      
   BSHA1 sha1(sha1Gen.finalize());
  
   mHeader.mSourceDigest = sha1;
   mHeader.mSourceFileSize = totalBytesRead;
   mHeader.mSourceFileTimeStamp = timeStamp;
      
   mIsFinalized = false;
   
   return true;
}

bool BResourceTagBuilder::setSourceDigestAndTimeStamp(const char* pFilename)
{
   BWin32FileStream fileStream;
   
   if (!fileStream.open(pFilename))
      return false;
      
//-- FIXING PREFIX BUG ID 8036
   const BWin32File* pFile = fileStream.getFile();      
//--
   
   FILETIME creationTime, lastAccessTime, lastWriteTime;
   if (!pFile->getFileTime(&creationTime, &lastAccessTime, &lastWriteTime))
      return false;
      
   const uint64 timeStamp = Math::Max<uint64>(Utils::FileTimeToUInt64(lastWriteTime), Utils::FileTimeToUInt64(creationTime));
         
   bool success = setSourceDigestAndTimeStamp(fileStream, timeStamp);
   if (!success)
      return false;
   
   mIsFinalized = false;
   
   return true;
}

void BResourceTagBuilder::setCreatorToolInfo(const BString& creatorToolCommandLine, uchar version)
{
   mCreatorToolCommandLine = creatorToolCommandLine;
   mHeader.mCreatorToolVersion = version;
   mIsFinalized = false;
}

void BResourceTagBuilder::setPlatformID(BResourceTagHeader::ePlatformID platformID)
{
   mHeader.mPlatformID = static_cast<uchar>(platformID);
   mIsFinalized = false;
}

// Returns an offset, not a real pointer.
char* BResourceTagBuilder::createString(const char* pStr)
{
   if (!pStr)
      return NULL;

   const uint strLen = strlen(pStr);
         
   const uint curSize = mTagData.getSize();
   
   //memcpy(mTagData.enlarge(strLen + 1), pStr, strLen + 1);
   
   mTagData.pushBack((const BYTE*)pStr, strLen + 1);
   
   return (char*)curSize;
}

bool BResourceTagBuilder::finalize(void)
{  
   if (mIsFinalized)
      return false;
      
   SYSTEMTIME systemTime;
   GetSystemTime(&systemTime);
   
   FILETIME curTime;
   if (!SystemTimeToFileTime(&systemTime, &curTime))
      return false;
   
   RandomUtils::GenerateGUID(mHeader.mTagGUID);
            
   mHeader.mTagTimeStamp = Utils::FileTimeToUInt64(curTime);
   
   char buf[512];
   DWORD bufSize = sizeof(buf);

#ifdef XBOX   
   strcpy_s(buf, sizeof(buf), "XBOX");
#else   
   if (!GetComputerNameA(buf, &bufSize))
      return false;
#endif      
      
   mHeader.mpTagMachineName = createString(buf);
      
   bufSize = sizeof(buf);
#ifdef XBOX   
   strcpy_s(buf, sizeof(buf), "360");
#else   
   if (!GetUserNameA(buf, &bufSize))
      return false;
#endif      
      
   mHeader.mpTagUserName = createString(buf);      
   
   mHeader.mpSourceFilename = createString(mSourceFilename.getPtr());
   mHeader.mpCreatorToolCommandLine = createString(mCreatorToolCommandLine.getPtr());
      
   mHeader.mHeaderAdler32 = calcAdler32(reinterpret_cast<const DWORD*>(&mHeader) + BResourceTagHeader::cAdler32DWORDsToSkip, sizeof(BResourceTagHeader) - BResourceTagHeader::cAdler32DWORDsToSkip * sizeof(DWORD));
   
   mHeader.mHeaderSize = sizeof(BResourceTagHeader);
   mHeader.mDataSize = static_cast<ushort>(mTagData.getSize() - sizeof(BResourceTagHeader));
   mHeader.mSignature = BResourceTagHeader::cSignature;
   mHeader.mMajorVersion = BResourceTagHeader::cMajorVersion;
   mHeader.mMinorVersion = BResourceTagHeader::cMinorVersion;
   
   memcpy(mTagData.getPtr(), &mHeader, sizeof(mHeader));
   
   if (!reinterpret_cast<const BResourceTagHeader*>(mTagData.getPtr())->check())
      return false;

   mIsFinalized = true;
   
   return true;
}

bool BResourceTagUtils::fileIsUnchanged(const char* pSrcFilename, const char* pDstFilename, const uint* pCreatorToolVersion)
{
   BByteArray dstFileData;
   if (!BWin32FileUtils::readFileData(pDstFilename, dstFileData))
      return false;

   if (dstFileData.isEmpty())
      return false;

   BECFFileReader ecfReader(BConstDataBuffer(dstFileData.getPtr(), dstFileData.getSize()));
   if (!ecfReader.check())
      return false;

   uint chunkLen;
   const BYTE* pChunkData = ecfReader.getChunkDataByID((uint64)cResourceTagECFChunkID, chunkLen);
   if ((!pChunkData) || (chunkLen < sizeof(BResourceTagHeader)))
      return false;

   BByteArray resourceTagData(chunkLen, pChunkData);
   BResourceTagHeader& resourceHeader = (BResourceTagHeader&)*resourceTagData.getPtr();
   if (!resourceHeader.check())
      return false;

   resourceHeader.pointerize(resourceTagData.getPtr());

   BByteArray srcFileData;
   if (!BWin32FileUtils::readFileData(pSrcFilename, srcFileData))
      return false;

   if (srcFileData.getSize() != resourceHeader.mSourceFileSize)
      return false;
   
   BSHA1Gen sha1Gen;
   sha1Gen.update(srcFileData.getPtr(), srcFileData.getSize());

   BSHA1 sha1(sha1Gen.finalize());

   if (sha1 != resourceHeader.mSourceDigest)
      return false;
      
   if (pCreatorToolVersion)
   {
      if (resourceHeader.mCreatorToolVersion < *pCreatorToolVersion)
         return false;
   }
   
   return true;
}

void BResourceTagUtils::printFileTime(BTextDispatcher& textDispatcher, const uint64& fileTime)
{
   SYSTEMTIME localSysTime;
   FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(&fileTime), &localSysTime );
   
   textDispatcher.printf("%02d/%02d/%04d %02d:%02d:%02d", localSysTime.wMonth, localSysTime.wDay, localSysTime.wYear, localSysTime.wHour, localSysTime.wMinute, localSysTime.wSecond);
}

bool BResourceTagUtils::printInfo(BTextDispatcher& textDispatcher, const BResourceTagHeader& header)
{
   textDispatcher.printf("     Signature: 0x%04X\n", header.mSignature);
   textDispatcher.printf("  MajorVersion: 0x%02X\n", header.mMajorVersion);     
   textDispatcher.printf("  MinorVersion: 0x%02X\n", header.mMinorVersion);
   textDispatcher.printf("    HeaderSize: 0x%04X\n", header.mHeaderSize);
   textDispatcher.printf("      DataSize: 0x%04X\n", header.mDataSize);
   
   textDispatcher.printf("       TagTime: ");
   printFileTime(textDispatcher, header.mTagTimeStamp);
   textDispatcher.printf("\n");
   
   textDispatcher.printf("       TagGUID: 0x%04X 0x%04X 0x%04X 0x%04X\n", header.mTagGUID[0], header.mTagGUID[1], header.mTagGUID[2], header.mTagGUID[3]);
   textDispatcher.printf("TagMachineName: \"%s\"\n", header.mpTagMachineName ? header.mpTagMachineName : "");
   textDispatcher.printf("   TagUserName: \"%s\"\n", header.mpTagUserName ? header.mpTagUserName : "");
   
   textDispatcher.printf("SourceFilename: \"%s\"\n", header.mpSourceFilename ? header.mpSourceFilename : "");
   textDispatcher.printf("SourceFileSize: %I64u\n", header.mSourceFileSize);
   
   textDispatcher.printf("SourceFileTime: ");
   printFileTime(textDispatcher, header.mSourceFileTimeStamp);
   textDispatcher.printf("\n");
   
   textDispatcher.printf("  SourceDigest: 0x%04X 0x%04X 0x%04X 0x%04X 0x%04X\n", header.mSourceDigest.getDWORD(0), header.mSourceDigest.getDWORD(1), header.mSourceDigest.getDWORD(2), header.mSourceDigest.getDWORD(3), header.mSourceDigest.getDWORD(4));
   
   textDispatcher.printf("CreatorToolCmdLine: \"%s\"\n", header.mpCreatorToolCommandLine ? header.mpCreatorToolCommandLine : "");
   textDispatcher.printf("CreatorToolVersion: 0x%02X\n", header.mCreatorToolVersion);
   textDispatcher.printf("        PlatformID: %u\n", header.mPlatformID);
   
   return true;
}


