//==============================================================================
//
// File: win32File.cpp
//
// Copyright (c) 2002-2007, Ensemble Studios
//
//==============================================================================

// Includes
#include "xcore.h"
#include "win32file.h"

#ifndef XBOX
   #include <Winioctl.h>
#endif   

//==============================================================================
// BWin32File::BWin32File
//==============================================================================
BWin32File::BWin32File() :
   mFileHandle(INVALID_HANDLE_VALUE),
   mpLowLevelFileIO(ILowLevelFileIO::getDefault())
{
}

//==============================================================================
// BWin32File::~BWin32File
//==============================================================================
BWin32File::~BWin32File()
{
   close();
}

//==============================================================================
// BWin32File::open
//==============================================================================
bool BWin32File::open(const char* path, DWORD flags)
{
   close();

   DWORD accessFlags = GENERIC_READ;
   if (flags & cWriteAccess)
   {
      accessFlags |= GENERIC_WRITE;
   }

   DWORD attributeFlags=FILE_ATTRIBUTE_NORMAL;
   if(flags & cRandomAccess)
      attributeFlags |= FILE_FLAG_RANDOM_ACCESS;
   else if (flags & cSequentialAccess)
      attributeFlags |= FILE_FLAG_SEQUENTIAL_SCAN;

   DWORD creationDisposition = OPEN_EXISTING;
   if (flags & cCreateIfNoExist)
      creationDisposition = OPEN_ALWAYS;
   else if (flags & cCreateAlways)
      creationDisposition = CREATE_ALWAYS;

   mFileHandle = mpLowLevelFileIO->createFileA(path, accessFlags, FILE_SHARE_READ, NULL, creationDisposition, attributeFlags, NULL);

   if (mFileHandle != INVALID_HANDLE_VALUE)
   {
      // success
      return true;
   }

   //DWORD error=GetLastError();
   //BString str;
   //FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, 0, str.getData(), BString::cStringSize, NULL);
   //str.empty();

   // failure
   return (false);
   
}

//==============================================================================
// 
//==============================================================================
bool BWin32File::open(HANDLE hFile)
{
   close();

   mFileHandle = hFile;

   if (mFileHandle != INVALID_HANDLE_VALUE)
   {
      // success
      return true;
   }

   // failure
   return (false);
}

//==============================================================================
// BWin32File::create
//==============================================================================
bool BWin32File::create(const char* path, DWORD flags)
{
   close();
         
   //mpLowLevelFileIO->setFileAttributes(path, FILE_ATTRIBUTE_NORMAL);

   DWORD accessFlags = GENERIC_READ|GENERIC_WRITE;

   DWORD attributeFlags = 0;
   if(flags & cRandomAccess)
      attributeFlags |= FILE_FLAG_RANDOM_ACCESS;
   else if (flags & cSequentialAccess)
      attributeFlags |= FILE_FLAG_SEQUENTIAL_SCAN;         

   mFileHandle = mpLowLevelFileIO->createFileA(path, accessFlags, FILE_SHARE_READ, NULL, CREATE_ALWAYS, attributeFlags, NULL);

   return (mFileHandle!=INVALID_HANDLE_VALUE);
}

//==============================================================================
// BWin32File::close
//==============================================================================
bool BWin32File::close()
{   
   bool success = true;  
      
   if(mFileHandle != INVALID_HANDLE_VALUE)
   {
      if (0 == mpLowLevelFileIO->closeHandle(mFileHandle))
         success = false;
         
      mFileHandle = INVALID_HANDLE_VALUE;
   }
   
   return success;
}

//==============================================================================
// BWin32File::flush
//==============================================================================
void BWin32File::flush()
{   
   if (mFileHandle != INVALID_HANDLE_VALUE)
   {
      mpLowLevelFileIO->flush(mFileHandle);
   }
}

//==============================================================================
// BWin32File::read
//==============================================================================
DWORD BWin32File::read(void* buffer, DWORD bytes)
{
   if(INVALID_HANDLE_VALUE == mFileHandle)
      return 0;
   DWORD bytesRead=0;
   if(!mpLowLevelFileIO->readFile(mFileHandle, buffer, bytes, &bytesRead, NULL))
      return 0;
   return bytesRead;
}

//==============================================================================
// 
//==============================================================================
DWORD BWin32File::write(const void* pBuffer, uint numBytes)
{
   if(INVALID_HANDLE_VALUE == mFileHandle)
      return 0;
   DWORD bytesWritten=0;
   if(!mpLowLevelFileIO->writeFile(mFileHandle, pBuffer, numBytes, &bytesWritten, NULL))
      return 0;
   return bytesWritten;
}

//==============================================================================
// BWin32File::seek
//==============================================================================
int64 BWin32File::seek(long seekType, int64 bytes)
{
   if(INVALID_HANDLE_VALUE == mFileHandle)
      return -1;

   DWORD low;
   LONG high;
   Utils::SeparateInt64(bytes, low, high);
   low = mpLowLevelFileIO->setFilePointer(mFileHandle, low, &high, seekType);
   
   if (low == INVALID_SET_FILE_POINTER)
   {
      if (GetLastError() != NO_ERROR)
         return -1;
   }
   
   return Utils::CreateUInt64(low, high);
}

//==============================================================================
// BWin32File::getSize
//==============================================================================
uint64 BWin32File::getSize() const
{
   DWORD high;
   const DWORD low = mpLowLevelFileIO->getFileSize(mFileHandle, &high); 
   if (INVALID_FILE_SIZE == low)
   {
      if (NO_ERROR != GetLastError())
         return UINT64_MAX;
   }         
   return Utils::CreateUInt64(low, high);
}

//==============================================================================
// 
//==============================================================================
bool BWin32File::getSize(uint64& size) const
{
   if(INVALID_HANDLE_VALUE == mFileHandle)
      return false;
   else
   {
      size = getSize();
      return true;
   }
}

//==============================================================================
// BWin32File::getSize
//==============================================================================
bool BWin32File::getSize(uint& size) const
{
   if(INVALID_HANDLE_VALUE == mFileHandle)
      return false;
   else
   {
      uint64 size64 = getSize();
      if (size64 > UINT_MAX)
         return false;
      
      size = (uint)size64;
      
      return true;
   }
}

//==============================================================================
// BWin32File::getFileTime
//==============================================================================
bool BWin32File::getFileTime(FILETIME* pCreationTime, FILETIME* pLastAccessTime, FILETIME* pLastWriteTime) const
{
   if (mFileHandle == INVALID_HANDLE_VALUE)
      return false;
      
   if (!mpLowLevelFileIO->getFileTime(mFileHandle, pCreationTime, pLastAccessTime, pLastWriteTime))
      return false;

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BWin32File::setFileTime(const FILETIME* pCreationTime, const FILETIME* pLastAccessTime, const FILETIME* pLastWriteTime) 
{
   if (mFileHandle == INVALID_HANDLE_VALUE)
      return false;

   if (!mpLowLevelFileIO->setFileTime(mFileHandle, pCreationTime, pLastAccessTime, pLastWriteTime))
      return false;

   return true;
}

//==============================================================================
// 
//==============================================================================
uint64 BWin32File::getOffset() const
{
   if ((INVALID_HANDLE_VALUE == mFileHandle) || !isOpen())
      return 0;

   DWORD low = 0;
   LONG high = 0;
   return mpLowLevelFileIO->setFilePointer(mFileHandle, low, &high, cSeekCurrent);
}

//==============================================================================
// 
//==============================================================================
bool BWin32File::getOffset(uint64& offset) const
{
   if(INVALID_HANDLE_VALUE == mFileHandle)
      return false;
   else
   {
      offset = getOffset();
      return true;
   }
}

//==============================================================================
// BWin32File::isNewer()
//==============================================================================
bool BWin32File::isNewer(BWin32File* pFile) const
{
   //-- if the passed in file is bad then we are newer.
   if (!pFile)
      return true;
   
   //-- if we can't get the time for the passed in file, then we assume that this file
   //-- is newer
   FILETIME referenceFileWriteTime;
   if (!pFile->getFileTime(NULL, NULL, &referenceFileWriteTime))
      return true;

   //-- if we can't get the time for this current file then we assume that
   //-- this file is not newer
   FILETIME currentFileWriteTime;
   if (!getFileTime(NULL, NULL, &currentFileWriteTime))
      return false;
      
   const LONG compRes = CompareFileTime(&currentFileWriteTime, &referenceFileWriteTime);
   
   return compRes > 0;
}

//==============================================================================
// BWin32File::fprintf
//==============================================================================
uint BWin32File::fprintf(const char* format, ...)
{
   char buf[2048];
   va_list args;
   va_start(args, format);
   int size = _vsnprintf_s(buf, sizeof(buf), format, args);
   va_end(args);
         
   if(size <= 0)
      return size;

   return write(buf, size);
}

//==============================================================================
// BWin32File::getFirstLogicalClusterNumber
// false on failure
//==============================================================================
#ifdef XBOX
bool BWin32File::getFirstLogicalClusterNumber(uint64& lcn) const
{
   lcn = 0;
   
   return true;
}
#else
bool BWin32File::getFirstLogicalClusterNumber(uint64& lcn) const
{
   lcn = 0;
   
   if (mFileHandle == INVALID_HANDLE_VALUE)
      return false;
      
   if (strcmp(mpLowLevelFileIO->getName(), "Win32") != 0)
      return false;
   
   STARTING_VCN_INPUT_BUFFER startingVCN;
   Utils::ClearObj(startingVCN);

   const DWORD PointersBufferSize = 512;
   RETRIEVAL_POINTERS_BUFFER* pPointersBuffer = reinterpret_cast<RETRIEVAL_POINTERS_BUFFER*>(malloc(PointersBufferSize));

   DWORD bytesReturned = 0;

   BOOL result = DeviceIoControl(
      mFileHandle,
      FSCTL_GET_RETRIEVAL_POINTERS, // dwIoControlCode
      &startingVCN,                 // input buffer
      sizeof(startingVCN),          // size of input buffer
      pPointersBuffer,              // output buffer
      PointersBufferSize,           // size of output buffer
      &bytesReturned,               // number of bytes returned
      NULL                          // OVERLAPPED structure
      );

   if (result == 0)
   {
      HRESULT hres = GetLastError();
      if (hres != ERROR_MORE_DATA)
      {
         free(pPointersBuffer);
         return false;
      }
   }

   if (pPointersBuffer->ExtentCount > 0)
      lcn = Utils::LargeIntegerToUInt64(pPointersBuffer->Extents[0].Lcn);

   free(pPointersBuffer);
   return true;
}
#endif

//==============================================================================
// BWin32File::extendFile
// false on failure
//==============================================================================
bool BWin32File::extendFile(uint64 newSize)
{
   if (mFileHandle == INVALID_HANDLE_VALUE)
      return false;
   
   const int64 curOfs = getOffset();
   
   const int64 newOfs = seek(cSeekBegin, newSize);
   if (newOfs != static_cast<int64>(newSize))
      return false;
   
   if (0 == mpLowLevelFileIO->setEndOfFile(mFileHandle))
      return false;
      
   if (curOfs != seek(cSeekBegin, curOfs))
      return false;
      
   return true;
}

//==============================================================================
// BWin32File::readArray
// false on failure
//==============================================================================
bool BWin32File::readArray(BByteArray& buf)
{
   const uint64 size = getSize();
   if (size >= 2048U*1024U*1024U)
      return false;
      
   buf.resize(static_cast<uint>(size));
         
   seek(cSeekBegin, 0);
   
   if (!size)
      return true;
   
   return read(buf.getPtr(), static_cast<DWORD>(size)) == static_cast<DWORD>(size);
}

//==============================================================================
// BWin32File::writeArray
// false on failure
//==============================================================================
bool BWin32File::writeArray(const BByteArray& buf)
{
   const DWORD size = buf.getSize();
   if (!size)
      return true;
         
   return write(buf.getPtr(), static_cast<DWORD>(size)) == static_cast<DWORD>(size);
}

//==============================================================================
// BWin32File::setOwnerThread
//==============================================================================
void BWin32File::setOwnerThread(DWORD threadID)
{
   if (INVALID_HANDLE_VALUE != mFileHandle)
   {
      mpLowLevelFileIO->setOwnerThread(mFileHandle, threadID);
   }
}

//==============================================================================
// BWin32File::getFullPathName
// false on failure
//==============================================================================
bool BWin32File::getFullPathName(BFixedStringMaxPath& path, ILowLevelFileIO* pLowLevelFileIO)
{
   if (!pLowLevelFileIO)
      pLowLevelFileIO = ILowLevelFileIO::getDefault();
      
   const BFixedStringMaxPath srcPath(path);
   
   if (0 == pLowLevelFileIO->getFullPathName(
      srcPath.c_str(),
      path.getBufSize(),
      &path[0], NULL))
   {
      path = srcPath;
      return false;
   }
   
   return true;
}
