//----------------------------------------------------------------------------
//
//  File.cpp
//
//  Copyright 2002-2007 Ensemble Studios
//
//----------------------------------------------------------------------------
#include "xsystem.h"
#include "workdirsetup.h"
#include "string\bsnprintf.h"
#include "file\win32FileStream.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFile::BFile()
{
   mFMIndex = -1;
   mOpenFlags = 0;
   mpStream = NULL;
   setLastError(BFILE_NO_ERROR);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFile::~BFile()
{
   close();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::close(void)
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }

   bool success = true;
   
   if (!mpStream->close())
      success = false;

   if (mFMIndex >= 0)
   {
      if (cFME_SUCCESS != gFileManager.removeFile(mFMIndex))
         success = false;

      mFMIndex = -1;
   }
   
   mpStream = NULL;

   if (!success)
      setLastError(BFILE_CLOSE_FAILED);

   return success;     
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::open(long dirID, const char* pFilename, uint flags)
{
	//-- We really want the user the close the file manually before they try
	//-- to open a new one.  This encourages them to handle file close errors.
	if (isOpen())
	{
	   setLastError(BFILE_NOT_CLOSED);
		return false;
	}
	
   mOpenFlags = flags;
   
   const uint fileSystemFlags = 0;
   eFileManagerError error = gFileManager.openFile(dirID, pFilename, flags, this, fileSystemFlags, &mpStream, mFMIndex);

   if (error != cFME_SUCCESS)
   {  
      if (error == cFME_WOULD_HAVE_BLOCKED)
         setLastError(BFILE_WOULD_HAVE_WAITED);
      else
         setLastError(BFILE_UNABLE_TO_OPEN_FILE);
      return false;
   }
      
   setLastError(BFILE_NO_ERROR);
   
   return true;
}

//----------------------------------------------------------------------------
//  read()
//  Returns true if the number of bytes specified by numBytes is read into
//  pBuffer.  Returns false otherwise.
//----------------------------------------------------------------------------
bool BFile::read(void* pBuffer, uint numBytes)
{
   BDEBUG_ASSERT(pBuffer);
   
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }
   
   if (!numBytes)
   {
      setLastError(BFILE_NO_ERROR);
      return true;
   }

   uint bytesRead = mpStream->readBytes(pBuffer, numBytes);
   if (bytesRead != numBytes)
   {
      setLastError(BFILE_READ_FAILED);
      return false;
   }

   return true;
}

//----------------------------------------------------------------------------
//  readEx()
//  Returns the number of bytes read into the buffer.  The behavior of this
//  function is slightly different than the standard read().  This function
//  allows you to receive a partial buffer back if you request data
//  beyond the end of the file.  Returns 0 on failure.
//----------------------------------------------------------------------------
uint BFile::readEx(void* pBuffer, uint numBytes)
{
   BDEBUG_ASSERT(pBuffer);
   
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return 0;
   }
      
   setLastError(BFILE_NO_ERROR);
   
   if (!numBytes)
      return 0;
      
   if (mpStream->sizeKnown())
   {
      if (!mpStream->bytesLeft())
      {
         setLastError(BFILE_READ_PAST_EOF);
         return 0;
      }
   }
         
   const uint bytesRead = mpStream->readBytes(pBuffer, numBytes);
      
   if (numBytes != bytesRead)
   {
      if ((mpStream->errorStatus()) || (!bytesRead))
         setLastError(BFILE_READ_FAILED);
      else
         setLastError(BFILE_READ_PAST_EOF);
   }         
   
   return bytesRead;
}

//----------------------------------------------------------------------------
//  writeEx()
//  Returns the number of bytes written to the file.
//----------------------------------------------------------------------------
uint BFile::writeEx(const void* pBuffer, uint numBytes)
{
   BDEBUG_ASSERT(pBuffer);

   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return 0;
   }

   uint bytesWritten = mpStream->writeBytes(pBuffer, numBytes);
   if (bytesWritten != numBytes)
   {
      setLastError(BFILE_WRITE_FAILED);
      return 0;
   }

   return bytesWritten;
}

//----------------------------------------------------------------------------
//  flush()
//----------------------------------------------------------------------------
void BFile::flush(void)
{
   if (mpStream)
      mpStream->flush();
}

//============================================================================
//  FILE POINTER
//============================================================================
bool BFile::setOffset(__int64 offset, uint fromPosition)
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }

   const int64 size = mpStream->size();
   const int64 origOfs = mpStream->curOfs();;
   
   int64 newOfs = origOfs;
                     
   switch (fromPosition)
   {  
      case BFILE_OFFSET_BEGIN:
      {
         newOfs = offset;
         break;
      }
      case BFILE_OFFSET_CURRENT:
      {
         newOfs += offset;
         break;
      }
      case BFILE_OFFSET_END:
      {
         if (size == BSTREAM_UNKNOWN_SIZE)
         {
            setLastError(BFILE_SEEK_FAILED);
            return false;
         }
            
         newOfs = size + offset;
         break;
      }
   }      
   
   if (newOfs < 0)
      newOfs = 0;
   else if (newOfs > size)
      newOfs = size;
      
   if (!mpStream->seekable())
   {
      if (newOfs < origOfs)
      {
         setLastError(BFILE_SEEK_FAILED);
         
         return false;
      }
      else if (newOfs > origOfs)
      {
         uint64 bytesToSkip = newOfs - origOfs;
         if (mpStream->skipBytes(bytesToSkip) != bytesToSkip)
         {
            setLastError(BFILE_SEEK_FAILED);
            return false;
         }
      }         
   }
   else if (mpStream->seek(newOfs, true) != newOfs)
   {
      setLastError(BFILE_SEEK_FAILED);
      return false;
   }
    
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
uint64 BFile::getOffset() const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return 0;
   }

   return mpStream->curOfs();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getOffset(DWORD& offset) const
{
   offset = 0;
   
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }

   if (mpStream->curOfs() >> 32U)
   {
      setLastError(BFILE_FILE_TOO_LARGE);
      return false;
   }
   
   offset = (uint)mpStream->curOfs();
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getOffset(unsigned __int64& offset) const
{
   offset = 0;
   
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }
      
   offset = mpStream->curOfs();
         
   return true;
}

//============================================================================
//  OTHER INFO
//============================================================================
bool BFile::isOpen(void) const
{
   return mpStream != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::isReadable(void) const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }
      
   return mpStream->readable();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::isWriteable(void) const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }

   return mpStream->writable();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool  BFile::isFullySeekable(void) const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }

   return mpStream->seekable();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getSize(DWORD& size) const
{
   size = 0;
   
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }
   
   if (mpStream->size() >> 32U)
   {
      setLastError(BFILE_FILE_TOO_LARGE);
      return false;  
   }
      
   size = (uint)mpStream->size();
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getSize(unsigned __int64& size) const
{
   size = 0;
   
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }
      
   size = mpStream->size();
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getTime(uint64& time) const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }

   if (!mpStream->getTime(time))
   {
      setLastError(BFILE_GENERAL_ERROR);
      return false;
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getTime(BFileTime& time) const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }
   
   uint64 fileTime64 = 0;
   if (!mpStream->getTime(fileTime64))
   {
      setLastError(BFILE_GENERAL_ERROR);
      return false;
   }
   
   FILETIME fileTime = Utils::UInt64ToFileTime(fileTime64);
   
   SYSTEMTIME systemTime;
   const BOOL success = FileTimeToSystemTime(&fileTime, &systemTime);
   if (!success)
   {
      setLastError(BFILE_GENERAL_ERROR);
      return false;
   }

   time.mYear           = systemTime.wYear;
   time.mMonth          = systemTime.wMonth;
   time.mDayOfWeek      = systemTime.wDayOfWeek;
   time.mDay            = systemTime.wDay;
   time.mHour           = systemTime.wHour;
   time.mMinute         = systemTime.wMinute;
   time.mSecond         = systemTime.wSecond;
   time.mMilliseconds   = systemTime.wMilliseconds;
   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getPath(BString& path) const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }
    
   path.set(mpStream->getName().getPtr());
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::getPath(BSimString& path) const
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return false;
   }

   path.set(mpStream->getName().getPtr());
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::readBString(BString& dstString)
{
   if(isOpen()==false)
   {
      setLastError(BFILE_NOT_OPEN);
      return(false);
   }
   
   //-- JER [1/9/2003] Read how far we need to read to get the whole string.
   long length=0;
   if(read(&length, sizeof(length)) == false)
      return(false);

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&length, 1);
   if(length>1024)
   {
      BASSERT(0); // possible endian switch error
      return(false);
   }
#endif

   const bool unicodeString = (length & 0x80000000) == 0;
   length &= 0x7FFFFFFF;

   //-- Handle an empty string.
   if (length <= 0)
   {
      dstString.empty();
      return(true);
   }
   
   if (unicodeString)
   {
      BUString temp;
      temp.makeRawString(length);
      
      // Read.
      if( read(temp.getString(), sizeof(WCHAR) * length) == false)
         return(false);
         
      // Terminate the string since we don't save out the null.
      temp.getString()[length] = L'\0';
      
#ifdef XBOX
      temp.endianSwap();
#endif
      
      temp.check();
      
      dstString.set(temp);
   }
   else
   {
	   // Manually acquire space in the string.
	   dstString.makeRawString(length);

	   // Read.
      if( read(dstString.getString(), sizeof(BCHAR_T) * length) == false)
         return(false);
   
      // Terminate the string since we don't save out the null.
      dstString.getString()[length] = B('\0');
   }      
   
   dstString.check();
   
   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::writeBString(const BString& string)
{
   if(isOpen()==false)
   {
      setLastError(BFILE_NOT_OPEN);
      return(false);
   }
   
   DWORD length = string.length();

   DWORD outLength = length;

#ifndef UNICODE
   outLength |= 0x80000000;
#endif

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&outLength, 1);
#endif
   
   if (!write(&outLength, sizeof(outLength)))
      return(false);

   if (length == 0)
      return(true);

#if defined(XBOX) && defined(UNICODE)
   BString outString(string.getPtr());
   outString.endianSwap();
   if (!write(const_cast<BCHAR_T*>(outString.getPtr()), length*sizeof(BCHAR_T)))
      return(false);
#else
   if (!write(const_cast<BCHAR_T*>(string.getPtr()), length*sizeof(BCHAR_T)))
      return(false);
#endif

   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::readBString(BSimString& dstString)
{
   if(isOpen()==false)
   {
      setLastError(BFILE_NOT_OPEN);
      return(false);
   }

   //-- JER [1/9/2003] Read how far we need to read to get the whole string.
   long length=0;
   if(read(&length, sizeof(length)) == false)
      return(false);

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&length, 1);
   if(length>1024)
   {
      BASSERT(0); // possible endian switch error
      return(false);
   }
#endif

   const bool unicodeString = (length & 0x80000000) == 0;
   length &= 0x7FFFFFFF;

   //-- Handle an empty string.
   if (length <= 0)
   {
      dstString.empty();
      return(true);
   }

   if (unicodeString)
   {
      BUString temp;
      temp.makeRawString(length);

      // Read.
      if( read(temp.getString(), sizeof(WCHAR) * length) == false)
         return(false);

      // Terminate the string since we don't save out the null.
      temp.getString()[length] = L'\0';

#ifdef XBOX
      temp.endianSwap();
#endif

      temp.check();

      dstString.set(temp.getPtr());
   }
   else
   {
      // Manually acquire space in the string.
      dstString.makeRawString(length);

      // Read.
      if( read(dstString.getString(), sizeof(BCHAR_T) * length) == false)
         return(false);

      // Terminate the string since we don't save out the null.
      dstString.getString()[length] = B('\0');
   }      

   dstString.check();

   return(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFile::writeBString(const BSimString& string)
{
   if(isOpen()==false)
   {
      setLastError(BFILE_NOT_OPEN);
      return(false);
   }

   DWORD length = string.length();

   DWORD outLength = length;

#ifndef UNICODE
   outLength |= 0x80000000;
#endif

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&outLength, 1);
#endif

   if (!write(&outLength, sizeof(outLength)))
      return(false);

   if (length == 0)
      return(true);

#if defined(XBOX) && defined(UNICODE)
   BSimString outString(string.getPtr());
   outString.endianSwap();
   if (!write(const_cast<BCHAR_T*>(outString.getPtr()), length*sizeof(BCHAR_T)))
      return(false);
#else
   if (!write(const_cast<BCHAR_T*>(string.getPtr()), length*sizeof(BCHAR_T)))
      return(false);
#endif

   return(true);
}


//==============================================================================
// BFile::getPtr
//==============================================================================
const BYTE* BFile::getPtr(void)
{
   if (!mpStream)
   {
      setLastError(BFILE_NOT_OPEN);
      return NULL;
   }
      
   if (mpStream->size() >> 32U)
   {
      setLastError(BFILE_FILE_TOO_LARGE);
      return NULL;      
   }
   
   const void* p = mpStream->ptr((uint)mpStream->size());
 
   return static_cast<const BYTE*>(p);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
uint BFile::fprintf(const char *format, ...)
{
   // jce [1/6/2003] -- stolen from crappy old filesystem
   // hopefully this can die some day.
   long nBuf;
   char szBuffer[8192];

   va_list args;
   va_start(args, format);

   nBuf = bvsnprintf(szBuffer, sizeof(szBuffer) / sizeof(szBuffer[0]), format, args);

   if (!nBuf)
      return 0;

   BASSERT(nBuf >= 0);               // no errors
   BASSERT(nBuf < sizeof(szBuffer)); // it fit

   va_end(args);

   long len=strlen(szBuffer);
   bool ok=write(szBuffer, len);
   if(!ok)
      return(0);
   return(len);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
uint BFile::fprintf(const WCHAR *format, ...)
{
   // jce [1/6/2003] -- stolen from crappy old filesystem
   // hopefully this can die some day.
   long nBuf;
   WCHAR szBuffer[8192];

   va_list args;
   va_start(args, format);

   nBuf = bvsnwprintf(szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]), format, args);

   if (!nBuf)
      return 0;

   BASSERT(nBuf >= 0);               // no errors
   BASSERT(nBuf < sizeof(szBuffer)); // it fit

   va_end(args);

   long len=wcslen(szBuffer);
   bool ok=write(szBuffer, len*sizeof(WCHAR));
   if(!ok)
      return(0);
   return(len);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BFile::fputs(const char *pString)
{
   return write(pString, strlen(pString));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BFile::fputs(const WCHAR *pString)
{
   return write(pString, wcslen(pString) * sizeof(WCHAR));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *BFile::fgets(char *string, DWORD dwMaxBytes)
{
   // jce [1/6/2003] -- stolen from crappy old filesystem
   // hopefully this can die some day.
   if (!string)
   {
      BFAIL("Cannot read data into null string");
      return string;
   }

   DWORD dwlen = dwMaxBytes;
   char *pStart = string;

   *string = 0;

   do
   {
      bool ok = read(string, sizeof(char));
      if (!ok)
      {
         *string = 0;
         break;
      }

      // jce [7/17/2002] -- fgets is supposed to hand back the new line.
      if(*string == '\n')
      {
         dwlen--;
         string++;
         break;
      }
      
      if(!*string)
         break;

      dwlen--;
      string++;

   } while (dwlen);

   if (!dwlen)
      pStart[dwMaxBytes - 1] = 0;
   else
      *string = 0;

   if (!*pStart)
      return 0;

   return pStart;
}

//==============================================================================
// BFile::setOwnerThread
//==============================================================================
void BFile::setOwnerThread(DWORD threadID)
{
   if (mpStream)
      mpStream->setOwnerThread(threadID);
}

//==============================================================================
// BFile::setLastError
//==============================================================================
void BFile::setLastError(BFileErrors error) const 
{ 
   mLastError = error; 
   
   if (mLastError == BFILE_SEEK_FAILED)
   {
      trace("BFile::setLastError: Seek FAILED!");
   }
}
