//============================================================================
//
//  cfileStream.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//---------------------------------------------------------------------------------------------------------------------------------
// class BCFileStream
//---------------------------------------------------------------------------------------------------------------------------------
class BCFileStream : public BStream
{
public:  
   BCFileStream() : 
      BStream(),
      mpFile(NULL),
      mSize(0),
      mLeft(0),
      mOfs(0)
   {
   }
   
   BCFileStream(const char* pFilename, uint flags = cSFReadable | cSFSeekable) : 
      BStream(),
      mpFile(NULL),
      mSize(0),
      mLeft(0),
      mOfs(0)
   {
      open(pFilename, flags);
   } 
   
   virtual ~BCFileStream()
   {
      close();
   }
   
   void open(FILE* pFile, const BString& name = B(""), uint flags = cSFReadable | cSFSeekable | cSFNoAutoClose, int size = -1) 
   {
      close();

      setName(name);
      setFlags(flags);

      mpFile = debugCheckNull(pFile);
      mOfs = _ftelli64(mpFile);

      mSize = size;
      mLeft = size;
      if (size == -1)
      {
         _fseeki64(mpFile, 0, SEEK_END);
         mSize = mLeft = _ftelli64(mpFile);
         _fseeki64(mpFile, mOfs, SEEK_SET);
      }
   }

   // true on success
   //bool open(const char* pFilename, uint flags = cSFReadable | cSFSeekable | cSFErrorOnFailure)
   bool open(const char* pFilename, uint flags = cSFReadable | cSFSeekable)
   {
      debugCheckNull(pFilename);

      close();

      setName(pFilename);
      setFlags(flags);

      char mode[8];

      const bool openExisting = (flags & cSFOpenExisting) != 0;
      if ((flags & cSFWritable) && (flags & cSFReadable))
         strcpy_s(mode, sizeof(mode), openExisting ? "r+b" : "w+b");
      else if (flags & cSFWritable)
         strcpy_s(mode, sizeof(mode), openExisting ? "ab" : "wb");
      else if (flags & cSFReadable)
         strcpy_s(mode, sizeof(mode), "rb");
      else
         return false;

      mpFile = NULL;
      fopen_s(&mpFile, pFilename, mode);

      if ((getErrorOnFailure()) && (!mpFile))
         fatalError("Unable to open file \"%s\"!\n", pFilename);

      if (mpFile)
      {
         mOfs = _ftelli64(mpFile);

         _fseeki64(mpFile, 0, SEEK_END);
         mSize = mLeft = _ftelli64(mpFile);

         _fseeki64(mpFile, mOfs, SEEK_SET);
      }

      return NULL != mpFile;
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

      BDEBUG_ASSERT(_ftelli64(mpFile) == (int64)mOfs);

      const uint ret = fread(p, 1, n, mpFile);
      
      if ((getErrorOnFailure()) && (ret != n))
         fatalError("BCFileStream::readBytes: Unable to read from file \"%s\"!\n", getName().getPtr());

      mOfs += ret;
      mLeft -= ret;

      return ret;
   }

   virtual uint writeBytes(const void* p, uint n)
   {
      debugCheckNull(p);
      
      if ((!mpFile) || (!writable()))
         return 0;

      if (!n)
         return 0;

      BDEBUG_ASSERT(_ftelli64(mpFile) == (int64)mOfs);

      const uint64 endOfs = mOfs + n;

      mSize = Math::Max<int64>(mSize, endOfs);

      const uint ret = fwrite(p, 1, n, mpFile);
      
      if ((getErrorOnFailure()) && (ret != n))
         fatalError("BCFileStream::writeBytes: Unable to write to file \"%s\"!\n", getName().getPtr());

      mOfs += ret;
      if (ret > mLeft)
         mLeft = 0;
      else
         mLeft -= ret;
      
      return ret;
   }

   virtual uint64 curOfs(void) const
   {
      if (!mpFile) 
         return 0;
         
      BDEBUG_ASSERT(_ftelli64(mpFile) == (int64)mOfs);
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
      
      if (0 != _fseeki64(mpFile, mOfs, SEEK_SET))
      {
         if (getErrorOnFailure())
            fatalError("BCFileStream::seek: Unable to seek within file \"%s\"!\n", getName().getPtr());
         
         return -1;
      }

      return aOfs;
   }

   virtual bool errorStatus(void) const
   {
      return (!mpFile) || (ferror(mpFile) != 0);
   }

   FILE* getFILE(void) const
   {
      return mpFile;
   }

   virtual bool close(void)
   {
      mSize = mLeft = mOfs = 0;
      
      if (mpFile)
      {
         if (getAutoClose())
         {
            const bool success = (0 == fclose(mpFile));
            mpFile = NULL;
            
            if (!success)
            {
               if (getErrorOnFailure())
                  fatalError("BCFileStream::close: Unable to close file \"%s\"!\n", getName().getPtr());
               return false;
            }
         }
            
         mpFile = NULL;
      }

      return true;
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
   
   FILE* mpFile;
   uint64 mSize;
   uint64 mLeft;
   uint64 mOfs;
      
private:
   BCFileStream(const BCFileStream& b);
   BCFileStream& operator= (const BCFileStream& rhs);
   
}; // class BCFileStream

