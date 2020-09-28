//============================================================================
//
//  win32FileStream.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "win32File.h"

//---------------------------------------------------------------------------------------------------------------------------------
// class BWin32FileStream
//---------------------------------------------------------------------------------------------------------------------------------
class BWin32FileStream : public BStream
{
public:  
   BWin32FileStream() : 
      BStream(),
      mpFile(NULL),
      mSize(0),
      mLeft(0),
      mOfs(0)
   {
   }

   virtual ~BWin32FileStream()
   {
      close();
   }

   // Takes ownership unless cSFNoAutoClose is specified. If ownsership is taken, BWin32File must have been allocated by plain operator new.
   void open(BWin32File* pFile, const BString& name = B(""), uint flags = cSFReadable | cSFSeekable | cSFNoAutoClose, int64 size = -1) 
   {
      close();

      setName(name);
      setFlags(flags);

      mpFile = debugCheckNull(pFile);

      mpFile->getOffset(mOfs);

      mSize = size;
      mLeft = size;
      if (size == -1)
      {
         mpFile->getSize(mSize);
         mLeft = mSize;
      }
   }

   // true on success
   virtual bool open(const char* pFilename, uint flags = cSFReadable | cSFSeekable, ILowLevelFileIO* pLowLevelFileIO = NULL)
   {
      debugCheckNull(pFilename);

      close();

      setName(pFilename);
      setFlags(flags);

      const bool openExisting = (flags & cSFOpenExisting) != 0;
      
      mpFile = &mFile;
      
      if (pLowLevelFileIO)
         mpFile->setLowLevelFileIO(pLowLevelFileIO);
      
      bool success = false;

      DWORD openFlags = 0;
      if (flags & cSFWritable)
      {
         openFlags = BWin32File::cWriteAccess;
         if (openExisting)
            openFlags |= (BWin32File::cAppend | BWin32File::cCreateIfNoExist);
         else
            openFlags |= BWin32File::cCreateAlways;
      }
      else if (flags & cSFReadable)
      {
         openFlags = 0;
      }

      if (flags & cSFOptimizeForRandomAccess)
         openFlags |= BWin32File::cRandomAccess;
   
      if (flags & cSFOptimizeForSequentialAccess)
         openFlags |= BWin32File::cSequentialAccess;         

      success = mpFile->open(pFilename, openFlags);

      if (!success)
      {
         mpFile = NULL;

         if (getErrorOnFailure())
            fatalError("Unable to open file \"%s\"!\n", pFilename);

         return false;
      }         

      mpFile->getOffset(mOfs);

      mpFile->getSize(mSize);
      mLeft = mSize;

      return true;
   }

   //virtual bool open(const char* pFilename, HANDLE hFile, uint flags = cSFReadable | cSFSeekable, ILowLevelFileIO* pLowLevelFileIO = NULL)
   //{
   //   debugCheckNull(pFilename);
   //   BDEBUG_ASSERT(hFile != INVALID_HANDLE_VALUE);

   //   close();

   //   setName(pFilename);
   //   setFlags(flags);

   //   mpFile = &mFile;

   //   if (pLowLevelFileIO)
   //      mpFile->setLowLevelFileIO(pLowLevelFileIO);

   //   bool success = mpFile->open(hFile);

   //   if (!success)
   //   {
   //      mpFile = NULL;

   //      if (getErrorOnFailure())
   //         fatalError("Unable to open file \"%s\"!\n", pFilename);

   //      return false;
   //   }         

   //   mpFile->getOffset(mOfs);

   //   mpFile->getSize(mSize);
   //   mLeft = mSize;

   //   return true;
   //}

   virtual bool opened(void) const
   {
      return NULL != mpFile;
   }

   virtual uint64 size(void) const 
   {
      if (!mpFile)
         return 0;

      return mSize; 
   }

   virtual uint readBytes(void* p, uint n)
   {
      debugCheckNull(p);

      if ((!mpFile) || (!readable()))
         return 0;

      if (!n)
         return 0;

      if (n > mLeft)
         n = static_cast<uint>(mLeft);

      const bool success = (mpFile->read(p, n) == n);

      if (!success)
      {
         if (getErrorOnFailure())
            fatalError("BWin32FileStream::readBytes: Unable to read from file \"%s\"!\n", getName().getPtr());

         return 0;
      }         

      mOfs += n;
      mLeft -= n;

      return n;
   }

   virtual uint writeBytes(const void* p, uint n)
   {
      debugCheckNull(p);

      if ((!mpFile) || (!writable()))
         return 0;

      if (!n)
         return 0;

      const uint64 endOfs = mOfs + n;

      mSize = Math::Max(mSize, endOfs);

      const bool success = (mpFile->write(p, n) == n);

      if (!success)
      {
         if (getErrorOnFailure())
            fatalError("BWin32FileStream::writeBytes: Unable to write to file \"%s\"!\n", getName().getPtr());
         return 0;            
      }            

      mOfs += n;

      if (n > mLeft)
         mLeft = 0;
      else 
         mLeft -= n;

      return n;
   }

   virtual uint64 curOfs(void) const
   {
      if (!mpFile) 
         return 0;

      return mOfs;
   }

   virtual uint64 bytesLeft(void) const
   {
      if (!mpFile)
         return 0;

      return mLeft;
   }

   virtual int64 seek(int64 ofs, bool absolute = true)
   {
      if ((!mpFile) || (!seekable()))
         return -1;

      int64 aOfs = ofs;

      if (!absolute)
         aOfs += curOfs();

      aOfs = Math::Clamp<int64>(aOfs, 0, mSize);

      mOfs = aOfs;
      mLeft = mSize - aOfs;

      int64 newOffset = mpFile->seek(BWin32File::cSeekBegin, (int64)mOfs);
      if ((uint64)newOffset != mOfs)
      {
         if (getErrorOnFailure())
            fatalError("BWin32FileStream::seek: Unable to seek within file \"%s\"!\n", getName().getPtr());

         return -1;
      }

      return aOfs;
   }

   virtual bool errorStatus(void) const
   {
      return (!mpFile);
   }

   BWin32File* getFile(void) const
   {
      return mpFile;
   }
   
   bool getHandle(HANDLE& handle) const
   {
      if (!mpFile)
         return false;
      handle = mpFile->getHandle();
      return true;
   }

   virtual bool close(void)
   {
      mSize = mLeft = mOfs = 0;

      if ( (mpFile) && (getAutoClose()) )
      {
         const bool success = mpFile->close();

         mpFile = NULL;

         if (!success)
         {
            if (getErrorOnFailure())
               fatalError("BWin32FileStream::close: Unable to close file \"%s\"!\n", getName().getPtr());
            return false;
         }
      }

      mpFile = NULL;

      return true;
   }
   
   virtual bool getTime(uint64& time) const 
   { 
      time = 0;
      
      if (!mpFile)
         return false;
      
      FILETIME create, lastAccess, lastWrite;
      if (!mpFile->getFileTime(&create, &lastAccess, &lastWrite))
         return false;
      
      time = Math::Max<uint64>(Utils::FileTimeToUInt64(create), Utils::FileTimeToUInt64(lastWrite));
      
      return true;
   }
   
   virtual void setOwnerThread(DWORD threadID) 
   {
      if (mpFile)
         mpFile->setOwnerThread(threadID);
   }

   bool getErrorOnFailure(void) const { return Utils::GetBitFlag(mFlags, cSFErrorOnFailure); }
   void setErrorOnFailure(bool f) { mFlags = Utils::SetBitFlag(mFlags, cSFErrorOnFailure, f); }      

   bool getAutoClose(void) const { return Utils::GetBitFlag(mFlags, cSFNoAutoClose) == false; }
   void setAutoClose(bool f) { mFlags = Utils::SetBitFlag(mFlags, cSFNoAutoClose, !f); }
      
protected:
   void fatalError(const char* pFmt, ...)
   {
      BFixedString<512> msg;

      va_list args;
      va_start(args, pFmt);

      msg.formatArgs(pFmt, args);

      va_end(args);

      BFATAL_FAIL(msg);
   }
   
   uint64 mSize;
   uint64 mLeft;
   uint64 mOfs;
   BWin32File mFile;
   BWin32File* mpFile;
   
private:   
   BWin32FileStream(const BWin32FileStream& b);
   BWin32FileStream& operator= (const BWin32FileStream& rhs);

}; // class BWin32FileStream

//---------------------------------------------------------------------------------------------------------------------------------
// class BWin32FileStreamFactory
//---------------------------------------------------------------------------------------------------------------------------------
class BWin32FileStreamFactory : public IStreamFactory
{
   ILowLevelFileIO* mpLowLevelFileIO;
public:
   BWin32FileStreamFactory() : mpLowLevelFileIO(NULL) { }
   virtual ~BWin32FileStreamFactory() { }
   
   void setLowLevelFileIO(ILowLevelFileIO* pLowLevelFileIO) { mpLowLevelFileIO = pLowLevelFileIO; }
   ILowLevelFileIO* getLowLevelFileIO(void) const { return mpLowLevelFileIO; }

   virtual BStream* create(long dirID, const BString& filename, eStreamFlags flags)
   {
      dirID;
      
      BWin32FileStream* pStream = new BWin32FileStream;
                     
      if (!pStream->open(filename.getPtr(), flags, mpLowLevelFileIO))
      {
         delete pStream;
         return false;
      }
      
      return pStream;
   }
   
   virtual bool getFileTime(long dirID, const BString& filename, uint64& fileTime)
   {
      dirID;
      fileTime = 0;
      
      WIN32_FILE_ATTRIBUTE_DATA data;
      ILowLevelFileIO* pLowLevelFileIO = mpLowLevelFileIO ? mpLowLevelFileIO : ILowLevelFileIO::getDefault();
               
      BOOL result = pLowLevelFileIO->getFileAttributesEx(filename.getPtr(), GetFileExInfoStandard, &data);
      if (!result)
         return false;
      
      fileTime = Utils::FileTimeToUInt64(data.ftLastWriteTime);
      
      return true;
   }
};
