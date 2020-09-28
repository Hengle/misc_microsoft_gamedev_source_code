//============================================================================
//
//  BFileSystemStream.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "file.h"

//---------------------------------------------------------------------------------------------------------------------------------
// class BFileSystemStream
//---------------------------------------------------------------------------------------------------------------------------------
class BFileSystemStream : public BStream
{
public:  
   BFileSystemStream() : 
      BStream(),
      mpFile(NULL),
      mSize(0),
      mLeft(0),
      mOfs(0)
   {
   }
      
   virtual ~BFileSystemStream()
   {
      close();
   }
   
   // Takes ownership unless cSFNoAutoClose is specified. If ownership is taken, BFile must have been allocated by plain operator new.
   void open(BFile* pFile, const BString& name = B(""), uint flags = cSFReadable | cSFSeekable | cSFNoAutoClose, int64 size = -1) 
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
   bool open(long dirID, const char* pFilename, uint flags = cSFReadable | cSFSeekable)
   {
      debugCheckNull(pFilename);

      close();

      setName(pFilename);
      setFlags(flags);

      const bool openExisting = (flags & cSFOpenExisting) != 0;

      mpFile = new BFile;

      bool success = false;
      
      uint openFlags = 0;
      if (flags & cSFEnableBuffering)
         openFlags |= BFILE_OPEN_ENABLE_BUFFERING;
      
      if (flags & cSFOptimizeForRandomAccess)
         openFlags |= BFILE_OPEN_OPTIMIZE_FOR_RANDOM_ACCESS;
      else if (flags & cSFOptimizeForSequentialAccess)
         openFlags |= BFILE_OPEN_OPTIMIZE_FOR_SEQUENTIAL_ACCESS;
         
      if (flags & cSFDiscardOnClose)
         openFlags |= BFILE_OPEN_DISCARD_ON_CLOSE;
      
      if (flags & cSFSeekable)
         openFlags |= BFILE_OPEN_BACKWARD_SEEKS;

      if (flags & cSFForceLoose)
         openFlags |= BFILE_OPEN_FORCE_LOOSE;

      if ((flags & cSFWritable) && (flags & cSFReadable))
      {
         success = mpFile->openReadWrite(dirID, pFilename, openFlags | (openExisting ? BFILE_OPEN_APPEND : 0));
      }
      else if (flags & cSFWritable)
      {
         success = mpFile->openWriteable(dirID, pFilename, openFlags | (openExisting ? BFILE_OPEN_APPEND : 0));
      }
      else if (flags & cSFReadable)
      {
         success = mpFile->openReadOnly(dirID, pFilename, openFlags);
      }

      if (!success)
      {
         delete mpFile;
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
      
      const bool success = mpFile->read(p, n);

      if (!success)
      {
         if (getErrorOnFailure())
            fatalError("BFileSystemStream::readBytes: Unable to read from file \"%s\"!\n", getName().getPtr());

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

      const bool success = mpFile->write(p, n);
      
      if (!success)
      {
         if (getErrorOnFailure())
            fatalError("BFileSystemStream::writeBytes: Unable to write to file \"%s\"!\n", getName().getPtr());
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

      if (!mpFile->setOffset((int64)mOfs))
      {
         trace("BFileSystemStream::seek: Seek failed");
         
         if (getErrorOnFailure())
            fatalError("BFileSystemStream::seek: Unable to seek within file \"%s\"!\n", getName().getPtr());

         return -1;
      }

      return aOfs;
   }

   virtual bool errorStatus(void) const
   {
      return (!mpFile);
   }
      
   BFile* getFile(void) const
   {
      return mpFile;
   }
   
   virtual bool close(void)
   {
      mSize = mLeft = mOfs = 0;

      if ( (mpFile) && (getAutoClose()) )
      {
         const bool success = mpFile->close();
         
         delete mpFile;
         mpFile = NULL;
      
         if (!success)
         {
            if (getErrorOnFailure())
               fatalError("BFileSystemStream::close: Unable to close file \"%s\"!\n", getName().getPtr());
            return false;
         }
      }
      
      mpFile = NULL;

      return true;
   }

   bool getTime(uint64& time) const
   {
      if (mpFile)
      {
         return mpFile->getTime(time);
      }
      time = 0;
      return false;
   }

   bool getErrorOnFailure(void) const { return Utils::GetBitFlag(mFlags, cSFErrorOnFailure); }
   void setErrorOnFailure(bool f) { mFlags = Utils::SetBitFlag(mFlags, cSFErrorOnFailure, f); }      

   bool getAutoClose(void) const { return Utils::GetBitFlag(mFlags, cSFNoAutoClose) == false; }
   void setAutoClose(bool f) { mFlags = Utils::SetBitFlag(mFlags, cSFNoAutoClose, !f); }
   
   virtual void setOwnerThread(DWORD threadID)
   {
      if (mpFile)
         mpFile->setOwnerThread(threadID);
   }

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
   
   BFile* mpFile;
   uint64 mSize;
   uint64 mLeft;
   uint64 mOfs;
   
private:   
   BFileSystemStream(const BFileSystemStream& b);
   BFileSystemStream& operator= (const BFileSystemStream& rhs);
   
}; // class BFileSystemStream

//---------------------------------------------------------------------------------------------------------------------------------
// class BFileSystemStreamFactory
//---------------------------------------------------------------------------------------------------------------------------------
class BFileSystemStreamFactory : public IStreamFactory
{
public:
   BFileSystemStreamFactory() { }
   virtual ~BFileSystemStreamFactory() { }

   virtual BStream* create(long dirID, const BString& filename, eStreamFlags flags)
   {
      BFileSystemStream* pStream = new BFileSystemStream;
      if (!pStream->open(dirID, filename.getPtr(), flags))
      {
         delete pStream;
         return false;
      }
      return pStream;
   }
   
   virtual bool getFileTime(long dirID, const BString& filename, uint64& fileTime)
   {
      fileTime = 0;
      
      BFileManager::BFileDesc desc;
      if (cFME_SUCCESS != gFileManager.getFileDesc(dirID, filename, desc))
         return false;
         
      fileTime = desc.mTime;
      
      return true;
   }
};

extern BFileSystemStreamFactory gFileSystemStreamFactory;
