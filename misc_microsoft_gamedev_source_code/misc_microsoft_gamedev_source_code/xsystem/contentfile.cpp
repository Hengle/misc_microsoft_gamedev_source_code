//==============================================================================
// contentfile.cpp
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

// this entire class should only be used on the xbox
// we already have file managers for win32 access
// the XContent APIs are strictly console (or vista at some point)
#include "xsystem.h"
#include "contentfile.h"
#include "file\win32FileStream.h"

#ifdef XBOX

//----------------------------------------------------------------------------
// this is not async yet, but will be after it's working
//----------------------------------------------------------------------------
BContentFile::BContentFile() : 
   mHandle(INVALID_HANDLE_VALUE)
{
   init();
}

//==============================================================================
// 
//==============================================================================
BContentFile::BContentFile(const XCONTENT_DATA& contentData) :
   mHandle(INVALID_HANDLE_VALUE)
{
   init();

   Utils::FastMemCpy(&mContentData, &contentData, sizeof(XCONTENT_DATA));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BContentFile::~BContentFile()
{
   close();
}

//==============================================================================
// 
//==============================================================================
void BContentFile::init()
{
   Utils::FastMemSet(&mContentData, 0, sizeof(XCONTENT_DATA));

   mRootname.set("phx");
}

//----------------------------------------------------------------------------
// Can be called Async.
//----------------------------------------------------------------------------
DWORD BContentFile::createContent(const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags, XOVERLAPPED* pOverlapped)
{
   int fileLength = strLength(pFilename)+1;
   int displayLength = strLength(pDisplayName)+1;
   BDEBUG_ASSERT(fileLength <= XCONTENT_MAX_FILENAME_LENGTH);
   BDEBUG_ASSERT(displayLength <= XCONTENT_MAX_DISPLAYNAME_LENGTH);

   strcpy_s( mContentData.szFileName, min(XCONTENT_MAX_FILENAME_LENGTH, fileLength), pFilename);
   wcscpy_s( mContentData.szDisplayName, min(XCONTENT_MAX_DISPLAYNAME_LENGTH, displayLength), pDisplayName);
   mContentData.dwContentType = contentType;
   mContentData.DeviceID = deviceID;

   // eg. phx:\\save.bin
   mFilename.format("%s:\\%s", mRootname.getPtr(), mContentData.szFileName);

   // Mount the device associated with the display name for writing
   DWORD dwResult = XContentCreate(userIndex, mRootname, &mContentData, contentFlags, NULL, NULL, pOverlapped);

   return dwResult;
}

//==============================================================================
// 
//==============================================================================
DWORD BContentFile::createContent(DWORD userIndex, DWORD contentFlags, XOVERLAPPED* pOverlapped)
{
   // eg. phx:\\save.bin
   mFilename.format("%s:\\%s", mRootname.getPtr(), mContentData.szFileName);

   return XContentCreate(userIndex, mRootname, &mContentData, contentFlags, NULL, NULL, pOverlapped);
}

//----------------------------------------------------------------------------
// should set the async flag if overlap IO is wanted
//----------------------------------------------------------------------------
BOOL BContentFile::open(uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags)
{
   // do not open if already open.
   if (isValidHandle())
      return false;

   // Create the file
   mHandle = CreateFile(mFilename, openFlags, shareFlags, NULL, createDisposition, dwFlags, NULL);
   if (!isValidHandle())
      return false;
      
   return true;
}

//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
BOOL BContentFile::close(void)
{
   if (!isValidHandle())
      return false;

   CloseHandle( mHandle );

   XContentClose(mRootname, NULL);

   mHandle = INVALID_HANDLE_VALUE;
   return true;
}

//----------------------------------------------------------------------------
//  read()
//  Returns true if the number of bytes specified by numBytes is read into
//  pBuffer.  Returns false otherwise.
//----------------------------------------------------------------------------
BOOL BContentFile::read (void* pBuffer, uint numBytes, uint* pNumBytesActual, LPOVERLAPPED overlapped)
{
   BDEBUG_ASSERT(pBuffer);
   BDEBUG_ASSERT(isValidHandle());
   if (!isValidHandle())
      return false;

   // read the file.
   BOOL result = ReadFile(mHandle, pBuffer, numBytes, (DWORD*)pNumBytesActual, overlapped);

   if (!result)
      blogerrortrace("BContentFile: read failed - Last Error: %d", GetLastError());

   return result;
}

//----------------------------------------------------------------------------
//  getLastError()
//  A thin wrapper around the WIN32 API.
//----------------------------------------------------------------------------
DWORD BContentFile::getLastError()
{
   return GetLastError();
}

//----------------------------------------------------------------------------
//  write()
//  Returns true if the number of bytes specified by numBytes is written to
//  the file.  Returns false otherwise.
//----------------------------------------------------------------------------
BOOL BContentFile::write(const void* pBuffer, uint numBytes, uint* pNumBytesActual, LPOVERLAPPED overlapped)
{
   BDEBUG_ASSERT(pNumBytesActual != NULL);
   *pNumBytesActual=0;
   BDEBUG_ASSERT(pBuffer);
   BDEBUG_ASSERT(isValidHandle());
   if (!isValidHandle())
      return false;

   BOOL result = WriteFile(mHandle, pBuffer, numBytes, (LPDWORD)pNumBytesActual, overlapped);

   if (!result)
      blogerror("BContentFile: read failed - Last Error: %d", GetLastError());

   return result;
}

//----------------------------------------------------------------------------
//  flush()
//----------------------------------------------------------------------------
void BContentFile::flush(void)
{
   BDEBUG_ASSERT(isValidHandle());
   if (!isValidHandle())
      return;

   FlushFileBuffers(mHandle);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BContentFile::setFilePointer(uint64 filepointer, DWORD moveMethod)
{
   LARGE_INTEGER li;
   li.QuadPart = filepointer;

   li.LowPart = SetFilePointer(mHandle, li.LowPart, &li.HighPart, moveMethod);

   // From the SDK on testing for failures.
   // If the return value is INVALID_SET_FILE_POINTER and if lpDistanceToMoveHigh is non-NULL, 
   // an application must call GetLastError to determine whether the function has succeeded 
   // or failed. The following sample code illustrates this point: 
   DWORD dwError=0;
   if (li.LowPart == INVALID_SET_FILE_POINTER && (dwError = GetLastError()) != NO_ERROR )
   {
      return false;
   }

   return true;
}

#endif // XBOX
