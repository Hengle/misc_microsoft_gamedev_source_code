//============================================================================
//
//  File.cpp
//
//  Copyright 2002 Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "archive.h"
#include "fileIO.h"

const long cWriteBufferMax=4096;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BFileStorage::BFileStorage()
{
   mError      = BFILE_NO_ERROR;
   mFlags      = 0;
   mhFile      = INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorage::close()
{
   //-- Close the file.
   if (mhFile != INVALID_HANDLE_VALUE)
   {
      if (!gFileIO.closeHandle(mhFile))
      {
         //-- Failure.  Cleanup and get out.
         mError = BFILE_CLOSE_FAILED;
         mFlags = 0;
         mhFile = INVALID_HANDLE_VALUE;
         mPath.empty();
         return(false);
      }
   }

   //-- Reset the members.
   mError = BFILE_NO_ERROR;
   mFlags = 0;
   mhFile = INVALID_HANDLE_VALUE;
   mPath.empty();
   
   //-- Success.
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorage::isOpen() const
{
   return (mhFile != INVALID_HANDLE_VALUE);
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BFileStorageWin32::BFileStorageWin32() :
   BFileStorage(),
   mpWriteBuffer(NULL),
   mWriteBufferOffset(0)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageWin32::close()
{
   if(mpWriteBuffer)
      releaseWriteBuffer();

   return BFileStorage::close();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageWin32::openReadOnly(const BString& path, unsigned long flags)
{
   //-- Sanity.
   BFATAL_ASSERT(mError == BFILE_NO_ERROR);
   BFATAL_ASSERT(mFlags == 0);
   BFATAL_ASSERT(mhFile == INVALID_HANDLE_VALUE);
   BFATAL_ASSERT(mPath.isEmpty());

   //-- Set up the Win32 flags.
   DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
   if (flags & BFILE_OPEN_DELETE_ON_CLOSE) dwFlags |= FILE_FLAG_DELETE_ON_CLOSE;
   if (flags & BFILE_OPEN_RANDOM_ACCESS)   dwFlags |= FILE_FLAG_RANDOM_ACCESS;
   else                                    dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;

   //-- Try to open the file.
   mhFile = gFileIO.createFileA(path.asNative(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, dwFlags, NULL);
   
	if (mhFile != INVALID_HANDLE_VALUE)
   {
		//-- Mark the private flags.
      mFlags |= FLAG_READONLY;

      //-- Success.
      mError = BFILE_NO_ERROR;
      mPath  = path;   
      return(true);
   }

	//-- Search the archives now.
   mError = BFILE_UNABLE_TO_OPEN_FILE;
   return(false);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageWin32::openWriteable(const BString& path, unsigned long flags)
{
   //-- Sanity.
   BFATAL_ASSERT(mError == BFILE_NO_ERROR);
   BFATAL_ASSERT(mFlags == 0);
   BFATAL_ASSERT(mhFile == INVALID_HANDLE_VALUE);
   BFATAL_ASSERT(mPath.isEmpty());
   
   //-- Set up the Win32 flags.
   DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
   if (flags & BFILE_OPEN_WRITE_THROUGH)   dwFlags |= FILE_FLAG_WRITE_THROUGH;
   if (flags & BFILE_OPEN_DELETE_ON_CLOSE) dwFlags |= FILE_FLAG_DELETE_ON_CLOSE;
   if (flags & BFILE_OPEN_RANDOM_ACCESS)   dwFlags |= FILE_FLAG_RANDOM_ACCESS;
   else                                    dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;

   DWORD disposition;
   DWORD dwShareMode = FILE_SHARE_READ;
   if(flags & BFILE_OPEN_APPEND)
      disposition=OPEN_ALWAYS;
   else
      disposition=CREATE_ALWAYS;

   //-- Try to open the file.
   mhFile = gFileIO.createFileA(path.asNative(), GENERIC_WRITE, dwShareMode, NULL, disposition, dwFlags, NULL);
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_UNABLE_TO_CREATE_FILE;
      return(false);
   }

   //-- Mark the flags.
   mFlags |= FLAG_WRITEABLE;

   // If append mode, hop to the end of the file.
   if(flags & BFILE_OPEN_APPEND)
   {
      bool ok=setOffset(0, BFILE_OFFSET_END);
      if(!ok)
         return(false);
   }

   //-- Success.
   mError = BFILE_NO_ERROR;
   mPath  = path;
   return(true);
}

//----------------------------------------------------------------------------
//  openReadWrite()
//  Opens a file for both reading and writing.  If the file exists, its
//  contents are not changed.  If the file doesn't exist, a new one is
//  created.
//
//  Optional flags are:

//  BFILE_OPEN_WRITE_THROUGH
//  BFILE_OPEN_RANDOM_ACCESS
//  BFILE_OPEN_DELETE_ON_CLOSE
//----------------------------------------------------------------------------
bool BFileStorageWin32::openReadWrite(const BString& path, unsigned long flags)
{
   //-- Sanity.
   BFATAL_ASSERT(mError == BFILE_NO_ERROR);
   BFATAL_ASSERT(mFlags == 0);
   BFATAL_ASSERT(mhFile == INVALID_HANDLE_VALUE);
   BFATAL_ASSERT(mPath.isEmpty());
   
   //-- Set up the Win32 flags.
   DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;
   if (flags & BFILE_OPEN_WRITE_THROUGH)   dwFlags |= FILE_FLAG_WRITE_THROUGH;
   if (flags & BFILE_OPEN_DELETE_ON_CLOSE) dwFlags |= FILE_FLAG_DELETE_ON_CLOSE;
   if (flags & BFILE_OPEN_RANDOM_ACCESS)   dwFlags |= FILE_FLAG_RANDOM_ACCESS;
   else                                    dwFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
   
   bool bCreateFile = false;

   DWORD disposition;
   DWORD dwShareMode = FILE_SHARE_READ;
   if(flags & BFILE_OPEN_APPEND)
   {
      disposition=OPEN_ALWAYS;

      // if file is being appended, and it doesn't already exist, then make sure we run create file logic first
      HANDLE hFile = gFileIO.createFileA(path.asNative(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (hFile != INVALID_HANDLE_VALUE)
         gFileIO.closeHandle(hFile);
      else
         bCreateFile = true;
   }
   else
   {
      disposition=CREATE_ALWAYS;
      bCreateFile=true;
   }

   //-- Try to open the file.
   mhFile = gFileIO.createFileA(path.asNative(), GENERIC_READ | GENERIC_WRITE, dwShareMode, NULL, disposition, dwFlags, NULL);
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_UNABLE_TO_CREATE_FILE;
      return(false);
   }

   //-- Mark the flags.
   mFlags |= FLAG_READWRITE;

   //-- Success.
   mError = BFILE_NO_ERROR;
   mPath  = path;
   return(true);
}

//============================================================================
//  FILE POINTER
//============================================================================
//----------------------------------------------------------------------------
//  setOffset()
//  Allows you to set the file pointer in either a positive or negative
//  direction.  You set the pointer relative to the beginning of the file,
//  end of the file, or current position via fromPosition.  
//
//  Notes:
//  - You can set the pointer beyond the end of the file.  If you do, future
//    writes will extend the size of file from that point, leaving unitialized
//    data in between.
//  - If you set the pointer beyond the end of the file and then close the
//    file without writing, the file size is not adjusted.
//  - If you try to set the file pointer to before the beginning of the file,
//    this function will behave as if you tried to set it to the beginning.
//----------------------------------------------------------------------------
bool BFileStorageWin32::setOffset(__int64 offset, unsigned long fromPosition)
{
   //-- Validate Parameters.
   BFATAL_ASSERT(fromPosition <= BFILE_OFFSET_END);

   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   if(mpWriteBuffer && mWriteBufferOffset>0)
      flushWriteBuffer();

   //-- Determine the "from" offset.
   unsigned __int64 fromOffset;
   switch (fromPosition)
   {
      case BFILE_OFFSET_BEGIN:
      {
         fromOffset = 0;
         break;
      }

      case BFILE_OFFSET_END:
      {
         if (!getSize(fromOffset))
         {
            mError = BFILE_GENERAL_ERROR;
            return(false);
         }
         break;
      }

      default:
      {
         if (!getOffset(fromOffset))
         {
            mError = BFILE_GENERAL_ERROR;
            return(false);
         }
      }
   }

   //-- Determine the desired offset.
   __int64 desiredOffset = fromOffset + offset;
   if (desiredOffset < 0)
      desiredOffset = 0;

   //-- Try to set the file pointer.
   LONG  low    = (LONG)(desiredOffset & 0x00000000FFFFFFFF);
   LONG  high   = (LONG)(desiredOffset >> 32);
   DWORD result = gFileIO.setFilePointer(mhFile, low, &high, FILE_BEGIN);
   if ((result == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
   {
      mError = BFILE_SET_POINTER_FAILED;
      return(false);
   }

   //-- Sanity.
   __int64 newOffset = ((static_cast<__int64>(high) << 32) | result);
   BASSERT(newOffset == desiredOffset);
   if (newOffset != desiredOffset)
   {
      mError = BFILE_SET_POINTER_FAILED;
      return(false);
   }

   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFileStorageWin32::getOffset(unsigned long& offset) const
{
   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   //-- Try to get the file pointer.
   LONG  high   = 0;
   DWORD result = gFileIO.setFilePointer(mhFile, 0, &high, FILE_CURRENT);
   if ((result == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
   {
      mError = BFILE_GET_POINTER_FAILED;
      return(false);
   }

   //-- See if the offset is too big.
   unsigned __int64 trueOffset = ((static_cast<unsigned __int64>(high) << 32) | result);
   if (trueOffset > cMaximumDWORD)
   {
      mError = BFILE_FILE_TOO_LARGE;
      return(false);
   }

   if(mpWriteBuffer && mWriteBufferOffset>0)
   {
      if (trueOffset+mWriteBufferOffset > cMaximumDWORD)
      {
         mError = BFILE_FILE_TOO_LARGE;
         return(false);
      }

      //-- Success.  Return the offset.
      offset = (unsigned long)(result+mWriteBufferOffset);
      mError = BFILE_NO_ERROR;
      return(true);
   }

   //-- Success.  Return the offset.
   offset = (unsigned long)result;
   mError = BFILE_NO_ERROR;
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFileStorageWin32::getOffset(unsigned __int64& offset) const
{
   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   //-- Try to get the file pointer.
   LONG  high   = 0;
   DWORD result = gFileIO.setFilePointer(mhFile, 0, &high, FILE_CURRENT);
   if ((result == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
   {
      mError = BFILE_GET_POINTER_FAILED;
      return(false);
   }

   //-- Success.  Return the offset.
   offset = ((static_cast<unsigned __int64>(high) << 32) | result);

   if(mpWriteBuffer && mWriteBufferOffset>0)
      offset+=mWriteBufferOffset;

   mError = BFILE_NO_ERROR;
   return(true);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFileStorageWin32::getSize(unsigned long& size) const
{
   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   //-- Try to get the file information.
   BY_HANDLE_FILE_INFORMATION fileInfo;
   if (gFileIO.getFileInformationByHandle(mhFile, &fileInfo) == 0)
   {
      mError = BFILE_GENERAL_ERROR;
      return(false);
   }

   //-- Get the true size.
   unsigned __int64 trueSize = ((static_cast<unsigned __int64>(fileInfo.nFileSizeHigh) << 32) | fileInfo.nFileSizeLow);

   if(mpWriteBuffer && mWriteBufferOffset>0)
   {
      if (trueSize+mWriteBufferOffset > cMaximumDWORD)
      {
         mError = BFILE_FILE_TOO_LARGE;
         return(false);
      }

      //-- Success.  Return the offset.
      size = (unsigned long)(trueSize+mWriteBufferOffset);
      mError = BFILE_NO_ERROR;
      return(true);
   }

   //-- See if its too big.
   if (trueSize > cMaximumDWORD)
   {
      mError = BFILE_FILE_TOO_LARGE;
      return(false);
   }

   //-- Success.
   size = (unsigned long)trueSize;
   mError = BFILE_NO_ERROR;
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFileStorageWin32::getSize(unsigned __int64& size) const
{
   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   //-- Try to get the file information.
   BY_HANDLE_FILE_INFORMATION fileInfo;
   if (gFileIO.getFileInformationByHandle(mhFile, &fileInfo) == 0)
   {
      mError = BFILE_GENERAL_ERROR;
      return(false);
   }

   //-- Success.
   size = ((static_cast<unsigned __int64>(fileInfo.nFileSizeHigh) << 32) | fileInfo.nFileSizeLow);

   if(mpWriteBuffer && mWriteBufferOffset>0)
      size+=mWriteBufferOffset;

   mError = BFILE_NO_ERROR;
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFileStorageWin32::getTime(BFileTime& time) const
{
   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return false;
   }

   //-- Try to get the file information.
   FILETIME lastWriteTime;
   if (gFileIO.getFileTime(mhFile, NULL, NULL, &lastWriteTime) == 0)
   {
      mError = BFILE_GENERAL_ERROR;
      return false;
   }

   //-- Convert to local time.  I'm not sure why we should do this, but the
   //-- MSDN example did it, so I copied it.
   FILETIME localTime;
   if (!FileTimeToLocalFileTime(&lastWriteTime, &localTime))
   {
      mError = BFILE_GENERAL_ERROR;
      return false;
   }

   //-- Convert to system time.
   SYSTEMTIME systemTime;
   if (!FileTimeToSystemTime(&localTime, &systemTime))
   {
      mError = BFILE_GENERAL_ERROR;
      return false;
   }

   //-- Convert to our format.
   time.mYear         = systemTime.wYear;
   time.mMonth        = systemTime.wMonth;
   time.mDayOfWeek    = systemTime.wDayOfWeek;
   time.mDay          = systemTime.wDay;
   time.mHour         = systemTime.wHour;
   time.mMinute       = systemTime.wMinute;
   time.mSecond       = systemTime.wSecond;
   time.mMilliseconds = systemTime.wMilliseconds;
   
   //-- Success.
   mError = BFILE_NO_ERROR;
   return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned long BFileStorageWin32::readEx(void* pBuffer, unsigned long numBytes)
{
   //-- Reading 0 bytes always succeeds.
   if (numBytes == 0)
   {
      mError = BFILE_NO_ERROR;
      return(0);
   }

   //-- Validate parameters.
   BASSERT(pBuffer);
   if (!pBuffer)
      return(0);

   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(0);
   }

   //-- Try the read.
   DWORD bytesRead = 0;
   BOOL  result    = gFileIO.readFile(mhFile, pBuffer, numBytes, &bytesRead, NULL);
   if (result == 0)
   {
      mError = BFILE_READ_FAILED;
      return(0);
   }

   //-- Success.
   mError = BFILE_NO_ERROR;
   return (unsigned long)bytesRead;
}

//----------------------------------------------------------------------------
//  read()
//  Returns true if the number of bytes specified by numBytes is read into
//  pBuffer.  Returns false otherwise.
//----------------------------------------------------------------------------
bool BFileStorageWin32::read(void* pBuffer, unsigned long numBytes)
{
   //-- Reading 0 bytes always succeeds.
   if (numBytes == 0)
   {
      mError = BFILE_NO_ERROR;
      return(true);
   }

   //-- Validate parameters.
   BASSERT(pBuffer);
   if (!pBuffer)
      return(false);

   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   //-- Try the read.
   DWORD bytesRead = 0;
   BOOL  result    = gFileIO.readFile(mhFile, pBuffer, numBytes, &bytesRead, NULL);
   if ((result == 0) || (bytesRead != numBytes))
   {
      mError = BFILE_READ_FAILED;
      return(false);
   }

   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//----------------------------------------------------------------------------
//  write()
//  Returns true if the number of bytes specified by numBytes is written to
//  the file.  Returns false otherwise.
//----------------------------------------------------------------------------
bool BFileStorageWin32::write(const void* pBuffer, unsigned long numBytes)
{
   //-- Writing 0 bytes always succeeds.
   if (numBytes == 0)
   {
      mError = BFILE_NO_ERROR;
      return(true);
   }

   //-- Validate parameters.
   BASSERT(pBuffer);
   if (!pBuffer)
      return(false);

   //-- Make sure we are open.
   if (mhFile == INVALID_HANDLE_VALUE)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   // Cached writing of files
   if(!mpWriteBuffer)
      createWriteBuffer();

   if(mpWriteBuffer)
   {
      const BYTE* pInBuffer=(BYTE*)pBuffer;
      while(numBytes>0)
      {
         DWORD writeBytes;
         if(mWriteBufferOffset+numBytes>cWriteBufferMax-1)
            writeBytes=cWriteBufferMax-mWriteBufferOffset;
         else
            writeBytes=numBytes;
         memcpy(mpWriteBuffer+mWriteBufferOffset, pInBuffer, writeBytes);
         pInBuffer+=writeBytes;
         numBytes-=writeBytes;
         mWriteBufferOffset+=writeBytes;
         if(mWriteBufferOffset>cWriteBufferMax)
         {
            BASSERT(0);
            mError = BFILE_WRITE_FAILED;
            return(false);
         }
         if(mWriteBufferOffset==cWriteBufferMax)
            flushWriteBuffer();
      }

      //-- Success.
      mError = BFILE_NO_ERROR;
      return(true);
   }

   //-- Try the write.
   DWORD bytesWritten = 0;
   BOOL  result       = gFileIO.writeFile(mhFile, pBuffer, numBytes, &bytesWritten, NULL);
   if ((result == 0) || (bytesWritten != numBytes))
   {
      mError = BFILE_WRITE_FAILED;
      return(false);
   }

   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//----------------------------------------------------------------------------
//  flush()
//----------------------------------------------------------------------------
void BFileStorageWin32::flush()
{
   flushWriteBuffer();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFileStorageWin32::createWriteBuffer()
{
   mpWriteBuffer=new BYTE[cWriteBufferMax];
   if(!mpWriteBuffer)
   {
      BASSERT(0);
      return(false);
   }
   mWriteBufferOffset=0;
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFileStorageWin32::flushWriteBuffer()
{
   if(!mpWriteBuffer)
      return(true);
   if(mWriteBufferOffset==0)
      return(true);
   DWORD numBytes=mWriteBufferOffset;
   mWriteBufferOffset=0;
   DWORD bytesWritten = 0;
   BOOL result = gFileIO.writeFile(mhFile, mpWriteBuffer, numBytes, &bytesWritten, NULL);
   if ((result == 0) || (bytesWritten != numBytes))
   {
      mError = BFILE_WRITE_FAILED;
      return(false);
   }
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFileStorageWin32::releaseWriteBuffer()
{
   if(mpWriteBuffer)
   {
      flushWriteBuffer();
      delete[] mpWriteBuffer;
      mpWriteBuffer=NULL;
   }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFileStorageWin32::setOwnerThread(DWORD threadID)
{
   gFileIO.setOwnerThread(mhFile, threadID);
}



#if 0
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BFileStorageArchive::BFileStorageArchive()
{
   mhArchiveHandle = 0;
   mpEntry = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BFileStorageArchive::~BFileStorageArchive()
{
   mhArchiveHandle = 0;
   delete mpEntry;
   mpEntry = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::openReadOnly(const BString& path, unsigned long flags)
{
   flags;

   //-- Sanity.
   BFATAL_ASSERT(mError == BFILE_NO_ERROR);
   BFATAL_ASSERT(mFlags == 0);
   BFATAL_ASSERT(mhArchiveHandle == 0);
   BFATAL_ASSERT(mPath.isEmpty());
   BFATAL_ASSERT(mpEntry == NULL);

   mpEntry = new BArchiveFileEntry;

   mhArchiveHandle = gFileManager.searchArchives(path, mpEntry);
   if(mhArchiveHandle != 0)
   {
      //-- Mark the private flags.
      mFlags |= FLAG_READONLY;

      //-- Success.
      mError = BFILE_NO_ERROR;
      mPath  = path;   
      mCurrentOffset = mpEntry->mdwOffset;
      return(true);
   }

   mError = BFILE_UNABLE_TO_OPEN_FILE;
   return(false);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::setOffset(__int64 offset, unsigned long fromPosition)
{
   //-- Validate Parameters.
   BFATAL_ASSERT(fromPosition <= BFILE_OFFSET_END);
   
   if(isOpen() == false)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }
   
   //-- Determine the "from" offset.
   unsigned __int64 fromOffset;
   switch (fromPosition)
   {
      case BFILE_OFFSET_BEGIN:
      {
         fromOffset = mpEntry->mdwOffset;
         break;
      }

      case BFILE_OFFSET_END:
      {
         if (!getSize(fromOffset))
         {
            mError = BFILE_GENERAL_ERROR;
            return(false);
         }
         fromOffset += mpEntry->mdwOffset; 
         break;
      }

      default:
      {
         if (!getOffset(fromOffset))
         {
            mError = BFILE_GENERAL_ERROR;
            return(false);
         }
         fromOffset += mpEntry->mdwOffset; 
      }
   }

   //-- Determine the desired offset.
   __int64 desiredOffset = fromOffset + offset;
   if (desiredOffset < mpEntry->mdwOffset)
      desiredOffset = mpEntry->mdwOffset;

   //-- Try to set the file pointer.
   mCurrentOffset = desiredOffset;
   
   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::getOffset(unsigned long& offset) const
{
   unsigned __int64 bigOffset = 0;
   bool success = getOffset(bigOffset);
   offset = (unsigned long)bigOffset;
   return(success);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::getOffset(unsigned __int64& offset) const
{
   if(isOpen() == false)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }
   
   offset = mCurrentOffset - mpEntry->mdwOffset;
   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::getSize(unsigned long& size) const
{
   if(isOpen() == false)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   size = mpEntry->mdwLength;
   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::getSize(unsigned __int64& size) const
{
   if(isOpen() == false)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   size = mpEntry->mdwLength;
   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::getTime(BFileTime& time) const
{
   if(isOpen() == false)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   time = mpEntry->mFileTime;
   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned long BFileStorageArchive::readEx(void* pBuffer, unsigned long numBytes)
{
   //-- Reading 0 bytes always succeeds.
   if (numBytes == 0)
   {
      mError = BFILE_NO_ERROR;
      return(0);
   }

   //-- Validate parameters.
   BASSERT(pBuffer);
   if (!pBuffer)
      return(0);

   //-- Make sure we are open.
   if (isOpen() == false)
   {
      mError = BFILE_NOT_OPEN;
      return(0);
   }

   BArchive* pArchive = gFileManager.getArchive(mhArchiveHandle);
   if(pArchive == NULL)
   {
      mError = BFILE_NOT_OPEN;
      return(0);
   }
   BFile* pFile = pArchive->getFile();
   if((pFile == NULL) || (pFile->isOpen()==false))
   {
      mError = BFILE_GENERAL_ERROR;
      return(0);
   }
   
   // Would this read go off the end of the virtual file inside the archive?
   __int64 overage = (mCurrentOffset + numBytes) - (mpEntry->mdwOffset + mpEntry->mdwLength);
   if(overage > 0)
      numBytes -= (unsigned long)overage;
   
   //-- Move to the correct place in the archive file.
   if(pArchive->mCachedOffset != mCurrentOffset)
      pFile->setOffset(mCurrentOffset);
      
   unsigned long bytesRead = pFile->readEx(pBuffer, numBytes);
   if(bytesRead == 0)
   {
      mError = BFILE_READ_FAILED;
      return(0);
   }
   
   //-- Move our virutal file pointer.
   // jce [7/19/2005] --  try doing the math ourselves since getOffset is slow.
   //pFile->getOffset(mCurrentOffset);
   mCurrentOffset += bytesRead;
   pArchive->mCachedOffset = (DWORD)mCurrentOffset;

   //-- Success.
   mError = BFILE_NO_ERROR;
   return(bytesRead);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::read(void* pBuffer, unsigned long numBytes)
{
   //-- Reading 0 bytes always succeeds.
   if (numBytes == 0)
   {
      mError = BFILE_NO_ERROR;
      return(true);
   }

   //-- Validate parameters.
   BASSERT(pBuffer);
   if (!pBuffer)
      return(false);

   //-- Make sure we are open.
   if (isOpen() == false)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }

   BArchive* pArchive = gFileManager.getArchive(mhArchiveHandle);
   if(pArchive == NULL)
   {
      mError = BFILE_NOT_OPEN;
      return(false);
   }
   BFile* pFile = pArchive->getFile();
   if((pFile == NULL) || (pFile->isOpen()==false))
   {
      mError = BFILE_GENERAL_ERROR;
      return(false);
   }
   
   // Would this read go off the end of the virtual file inside the archive?
   __int64 overage = (mCurrentOffset + numBytes) - (mpEntry->mdwOffset + mpEntry->mdwLength);
   if(overage > 0)
      numBytes -= (unsigned long)overage;
      
   //-- Move to the correct place in the archive file.
   if(pArchive->mCachedOffset != mCurrentOffset)
      pFile->setOffset(mCurrentOffset);

   if(pFile->read(pBuffer, numBytes) == false)
   {
      mError = BFILE_READ_FAILED;
      
      // Not 100% sure this is necessary.
      pFile->getOffset(pArchive->mCachedOffset);
      
      return(false);
   }
   //-- Move our virutal file pointer.
   // jce [7/19/2005] --  try doing the math ourselves since getOffset is slow.
   //pFile->getOffset(mCurrentOffset);
   mCurrentOffset += numBytes;
   pArchive->mCachedOffset = (DWORD)mCurrentOffset;

   //-- Success.
   mError = BFILE_NO_ERROR;
   return(true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BFileStorageArchive::isOpen() const
{
   return ((mhArchiveHandle != 0) && (mpEntry != NULL));
}
#endif
