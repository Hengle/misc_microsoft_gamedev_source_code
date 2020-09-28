//==============================================================================
//
// File: win32FileUtils.cpp
//
// Copyright (c) 2002-2007, Ensemble Studios
//
//==============================================================================
#include "xcore.h"
#include "win32FileUtils.h"
#include "win32File.h"
#include "win32FileStream.h"
#include "stream\bufferStream.h"
#include "hash\bsha1.h"
#include "hash\crc.h"
#include "hash\adler32.h"

#ifndef INVALID_FILE_ATTRIBUTES
   #define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

bool BWin32FileUtils::computeFileDigest(const char* pFilename, uint64* pFileSize, BSHA1* pSha1, DWORD* pCRC32, DWORD* pAdler32)
{
   if (pFileSize)
      *pFileSize = 0;
   if (pSha1) 
      pSha1->clear();
   if (pCRC32) 
      *pCRC32 = cInitCRC32;
   if (pAdler32) 
      *pAdler32 = INIT_ADLER32;
   
   BWin32File file;

   if (!file.open(pFilename, BWin32File::cSequentialAccess))
      return false;
      
   const uint64 fileSize = file.getSize();
   if (UINT64_MAX == fileSize)
      return false;

   if (pFileSize)
      *pFileSize = fileSize;      

   if ((pSha1) || (pCRC32) || (pAdler32))
   {
      const uint cBufSize = 8192U;
      uchar buf[cBufSize];    
      
      BSHA1Gen sha1Gen;
      DWORD curCRC32 = cInitCRC32;
      DWORD curAdler32 = INIT_ADLER32;
      
      uint64 bytesLeft = fileSize;
      for ( ; ; )
      {
         const uint bytesToRead = static_cast<uint>(Math::Min<uint64>(bytesLeft, cBufSize));
         if (!bytesToRead)
            break;
                     
         if (file.read(buf, bytesToRead) != bytesToRead)
            return false;
         
         if (pSha1)
            sha1Gen.update(buf, bytesToRead);
            
         if (pCRC32)
            curCRC32 = calcCRC32(buf, bytesToRead, curCRC32);
         
         if (pAdler32)
            curAdler32 = calcAdler32(buf, bytesToRead, curAdler32);
            
         bytesLeft -= bytesToRead;
      }
      
      if (pSha1)
         *pSha1 = sha1Gen.finalize();
      
      if (pCRC32)
         *pCRC32 = curCRC32;
         
      if (pAdler32)
         *pAdler32 = curAdler32;
   }      
   
   return true;
}

bool BWin32FileUtils::getFileSize(const char* pFilename, uint64& fileSize)
{
   fileSize = 0;
   
   WIN32_FILE_ATTRIBUTE_DATA attr;

   if (0 == ILowLevelFileIO::getDefault()->getFileAttributesEx(pFilename, GetFileExInfoStandard, &attr))
      return false;

   if (attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      return false;

   fileSize = (uint64)attr.nFileSizeLow | ((uint64)attr.nFileSizeHigh << 32U);

   return true;
}

bool BWin32FileUtils::doesFileExist(const char* pFilename)
{
   const DWORD fullAttributes = ILowLevelFileIO::getDefault()->getFileAttributes(pFilename);

   if (fullAttributes == INVALID_FILE_ATTRIBUTES) 
      return false;
      
   if (fullAttributes & FILE_ATTRIBUTE_DIRECTORY)
      return false;

   return true;
}

bool BWin32FileUtils::doesDirectoryExist(const char* pDirectory)
{
   //-- Get the file attributes.
   DWORD fullAttributes = ILowLevelFileIO::getDefault()->getFileAttributes(pDirectory);

   if (fullAttributes == INVALID_FILE_ATTRIBUTES)
      return false;

   if (fullAttributes & FILE_ATTRIBUTE_DIRECTORY)
      return true;

   return false;
}

bool BWin32FileUtils::readFileData(const char* pFilename, BByteArray& data)
{
   BWin32File file;
   
   if (!file.open(pFilename, BWin32File::cSequentialAccess))
      return false;

   return file.readArray(data);
}

bool BWin32FileUtils::writeFileData(const char* pFilename, const BByteArray& data, bool retry)
{
   BWin32File file;

   const uint cMaxRetries = retry ? 4 : 1;

   uint retryIndex;
   for (retryIndex = 0; retryIndex < cMaxRetries; retryIndex++)
   {
      if (file.open(pFilename, BWin32File::cWriteAccess | BWin32File::cCreateAlways | BWin32File::cSequentialAccess))
         break;
         
      Sleep(1500);
   }         
   
   if (retryIndex == cMaxRetries) 
      return false;

   if (!file.writeArray(data))
      return false;
   
   if (!file.close())
      return false;
   
   return true;
}

bool BWin32FileUtils::writeFileData(const char* pDstFilename, BECFFileBuilder& ecfBuilder, uint64* pOutputFileSize, bool retry)
{ 
   BWin32FileStream dstStream;
   
   const uint cMaxRetries = retry ? 4 : 1;
   
   uint retryIndex;
   for (retryIndex = 0; retryIndex < cMaxRetries; retryIndex++)
   {
      if (dstStream.open(pDstFilename, cSFWritable | cSFSeekable | cSFOptimizeForSequentialAccess))
         break;
         
      Sleep(1500);
   }         
   
   if (retryIndex == cMaxRetries) 
      return false;

   if (!ecfBuilder.writeToStream(dstStream))
      return false;

   if (pOutputFileSize)
      *pOutputFileSize = dstStream.curOfs();
      
   return dstStream.close();
}

bool BWin32FileUtils::readStringFile(const char* pFilename, BDynamicArray<BString>& stringArray)
{
   BWin32FileStream fileStream;
   if (!fileStream.open(pFilename))
      return false;

   BBufferStream bufferStream(fileStream);
      
   BString str;   
   
   for ( ; ; )
   {
      if (!bufferStream.readLine(str))
         break;
         
      stringArray.enlarge(1);
      stringArray.back().swap(str);
   }
            
   return true;
}

void BWin32FileUtils::createDirectories(const char* pPath, bool removeFilename)
{
   BString dstPath(pPath);
   dstPath.standardizePath();
   strPathMakeAbsolute(BString(dstPath), dstPath);

   if (removeFilename)
   {
      int i = dstPath.findRight("\\");
      if (i >= 0)
         dstPath.left(i);
   }         

   strPathCreateFullPath(dstPath);
}

bool BWin32FileUtils::copyFile(const char* pSrcFilename, const char* pDstFilename, bool createDestinationPath)
{
   BWin32File srcFile;
   if (!srcFile.open(pSrcFilename, BWin32File::cSequentialAccess))
      return false;
      
   if (createDestinationPath)
      createDirectories(pDstFilename);

   BWin32File dstFile;   
   if (!dstFile.open(pDstFilename, BWin32File::cSequentialAccess | BWin32File::cWriteAccess | BWin32File::cCreateAlways))
      return false;
      
   uint64 fileSize = 0;      
   if (!srcFile.getSize(fileSize))
      return false;
    
   if (!dstFile.extendFile(fileSize))
      return false;
      
   const uint cBufSize = 1024U*1024U;
   BByteArray buf(cBufSize);
   
   uint64 bytesLeft = fileSize;
   
   while (bytesLeft > 0)
   {
      const uint bytesToRead = static_cast<uint>(Math::Min<uint64>(cBufSize, bytesLeft));
      
      if (srcFile.read(buf.getPtr(), bytesToRead) != bytesToRead)
         return false;
      
      if (dstFile.write(buf.getPtr(), bytesToRead) != bytesToRead)
         return false;
      
      bytesLeft -= bytesToRead;
   }

   if (!dstFile.close())
      return false;
         
   if (!srcFile.close())
      return false;
      
   return true;
}
