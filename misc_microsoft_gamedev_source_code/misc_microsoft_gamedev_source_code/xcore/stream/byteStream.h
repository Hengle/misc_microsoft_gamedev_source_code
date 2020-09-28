//============================================================================
//
//  byteStream.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//============================================================================
// class BByteStream
//============================================================================
class BByteStream : public BStream
{
   union
   {
      const uchar* mpBufConst;
      uchar* mpBuf;
   };
   uint mBufOfs;
   uint mBufSize;

public:
   BByteStream() :
      mpBuf(NULL),
      mBufOfs(0),
      mBufSize(0),
      BStream()
   {
   }
   
   BByteStream(const BByteStream& b) :
      BStream(b)
   {
      *this = b;
   }

   BByteStream& operator= (const BByteStream& rhs)
   {
      if (this != &rhs)
      {
         static_cast<BStream&>(*this) = rhs;

         mpBuf = rhs.mpBuf;
         mBufOfs = rhs.mBufOfs;
         mBufSize = rhs.mBufSize;
      }

      return *this;
   }

   BByteStream(const void* p, uint n, uint flags = cSFDefaultFlags, const BString& name = B("")) :
      BStream(name, flags),
      mpBufConst(reinterpret_cast<const uchar*>(p)),
      mBufOfs(0),
      mBufSize(n)
   { 
      if (NULL == mpBuf)
         mBufSize = 0;
   }

   BByteStream(void* p, uint n, uint flags = cSFDefaultFlags, const BString& name = B("")) :
      BStream(name, flags),
      mpBuf(reinterpret_cast<uchar*>(p)),
      mBufOfs(0),
      mBufSize(n)
   { 
      if (NULL == mpBuf)
         mBufSize = 0;
   }
   
   void set(void* p, uint n, uint flags = cSFDefaultFlags, const BString& name = B(""))
   {
      mName = name;
      mFlags = flags;
      mpBuf = reinterpret_cast<uchar*>(p);
      mBufOfs = 0;
      mBufSize = n;
      if (NULL == mpBuf)
         mBufSize = 0;
   }
   
   void set(const void* p, uint n, uint flags = cSFDefaultFlags, const BString& name = B(""))
   {
      BDEBUG_ASSERT((flags & cSFWritable) == 0);
            
      mName = name;
      mFlags = flags;
      mpBufConst = reinterpret_cast<const uchar*>(p);
      mBufOfs = 0;
      mBufSize = n;
      if (NULL == mpBuf)
         mBufSize = 0;
   }

   virtual uint64 size(void) const
   {
      return mBufSize;
   }

   virtual uint readBytes(void* p, uint n)
   {  
      BASSERT(p);
      
      if (!readable())
         return 0;
      
      if ((!n) || (!mBufSize))
         return 0;

      n = Math::Min(mBufSize - mBufOfs, n);
      memcpy(p, mpBufConst + mBufOfs, n);
      mBufOfs += n;
      return n;
   }

   virtual uint writeBytes(const void* p, uint n)
   {
      BASSERT(p);
      
      if (!writable())
         return 0;
      
      if ((!n) || (!mBufSize))
         return 0;

      const uint bytesToCopy = Math::Min(mBufSize - mBufOfs, n);

      memcpy(mpBuf + mBufOfs, p, bytesToCopy);
      mBufOfs += bytesToCopy;

      return bytesToCopy;
   }

   virtual uint64 curOfs(void) const
   {
      return mBufOfs;
   }

   virtual uint64 bytesLeft(void) const
   {
      return mBufSize - mBufOfs;
   }

   virtual int64 seek(int64 ofs64, bool absolute = true)
   {
      if (!seekable())
         return -1;
         
      if ((ofs64 < INT_MIN) || (ofs64 > INT_MAX))
         return -1;

      int ofs = static_cast<int>(ofs64); 
      
      uint aOfs;
      if (absolute)
         aOfs = static_cast<uint>(ofs);
      else
      {  
         if ((ofs < 0) && ((uint)(-ofs) > mBufOfs))
            aOfs = 0;
         else
            aOfs = mBufOfs + ofs;
      }

      mBufOfs = Math::Min(aOfs, mBufSize);
      return mBufOfs;
   }

   virtual const void* ptr(uint len = 0)
   { 
      if ((len > mBufSize) || (!mBufSize))
         return NULL;
      return mpBuf;
   } 
}; // class BByteStream

//---------------------------------------------------------------------------------------------------------------------------------
// class BDummyStream
//---------------------------------------------------------------------------------------------------------------------------------
class BDummyStream : public BStream
{      
   uint64 mBufOfs;
   uint64 mBufSize;

public:
   BDummyStream() :
      BStream()
   {
   }
   
   BDummyStream(const BDummyStream& b) :
      BStream(b)
   {
      *this = b;
   }

   BDummyStream& operator= (const BDummyStream& rhs)
   {
      if (this != &rhs)
      {
         static_cast<BStream&>(*this) = rhs;

         mBufOfs = rhs.mBufOfs;
         mBufSize = rhs.mBufSize;
      }

      return *this;
   }

   BDummyStream(uint64 size, const BString& name = B(""), uint flags = cSFReadable | cSFWritable | cSFSeekable) :
      BStream(name, flags),
      mBufOfs(0),
      mBufSize(size)
   { 
   }

   virtual uint64 size(void) const
   {
      return mBufSize;
   }

   virtual uint readBytes(void* p, uint n)
   {  
      BASSERT(p);
      
      if (!readable())
         return 0;
      
      n = (uint)Math::Min<uint64>(mBufSize - mBufOfs, n);
      memset(p, 0, n);
      mBufOfs += n;
      return n;
   }

   virtual uint writeBytes(const void* p, uint n)
   {
      p;
      BASSERT(p);
      
      if (!writable())
         return 0;
               
      mBufOfs += n;
      mBufSize = Math::Max<uint64>(mBufSize, mBufOfs);

      return n;
   }

   virtual uint64 curOfs(void) const
   {
      return mBufOfs;
   }

   virtual uint64 bytesLeft(void) const
   {
      return mBufSize - mBufOfs;
   }

   virtual int64 seek(int64 ofs, bool absolute = true)
   {
      if (!seekable())
         return -1;

      uint64 aOfs;
      if (absolute)
         aOfs = ofs;
      else
      {  
         if ((ofs < 0) && ((uint)(-ofs) > mBufOfs))
            aOfs = 0;
         else
            aOfs = mBufOfs + ofs;
      }

      mBufOfs = Math::Min<uint64>(aOfs, mBufSize);
      return mBufOfs;
   } 
}; // class BByteStream

