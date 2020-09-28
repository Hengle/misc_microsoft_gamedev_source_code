//============================================================================
//
//  dynamicStream.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//---------------------------------------------------------------------------------------------------------------------------------
// class BDynamicStream
//---------------------------------------------------------------------------------------------------------------------------------
class BDynamicStream : public BStream
{
protected:
   BByteArray mBuf;
   uint mBufOfs;

public:
   BDynamicStream(const BDynamicStream& b) :
      BStream(b)
   {
      mBuf = b.mBuf;
      mBufOfs = b.mBufOfs;
   }

   BDynamicStream& operator= (const BDynamicStream& rhs)
   {
      if (this != &rhs)
      {
         static_cast<BStream&>(*this) = rhs;
         mBuf = rhs.mBuf;
         mBufOfs = rhs.mBufOfs;
      }

      return *this;
   }

   BDynamicStream(uint flags = cSFReadable | cSFWritable | cSFSeekable, int reserveSize = -1, const BString& name = B("")) :
      BStream(name, flags),
      mBufOfs(0)
   { 
      if (-1 != reserveSize)
         mBuf.reserve(reserveSize);
   }

   virtual uint64 size(void) const
   {
      return static_cast<uint64>(mBuf.size());
   }

   void resize(uint newSize)
   {
      mBuf.resize(newSize);
      mBufOfs = Math::Min(mBufOfs, newSize);
   }

   virtual uint readBytes(void* p, uint n)
   {
      BASSERT(p);
      
      if (!readable())
         return 0;
      
      const uint maxBytes = mBuf.size() - mBufOfs;

      n = Math::Min(n, maxBytes);
      
      if (n)
      {
         Utils::FastMemCpy(p, &mBuf[mBufOfs], n);

         mBufOfs += n;
      }

      return n;
   }

   virtual uint writeBytes(const void* p, uint n)
   {
      BASSERT(p);
      
      if (!writable())
         return 0;
      
      if (!n)
         return 0;

      const uchar* pBytes = reinterpret_cast<const uchar*>(p);

      uint bytesLeft = n;
      
      if (mBufOfs < mBuf.size())
      {
         const uint bytesToCopy = Math::Min(mBuf.size() - mBufOfs, bytesLeft);

         Utils::FastMemCpy(&mBuf[mBufOfs], pBytes, bytesToCopy);
         mBufOfs += bytesToCopy;

         pBytes += bytesToCopy;
         bytesLeft -= bytesToCopy;
      }

      if (bytesLeft)
      {
         mBuf.pushBack(pBytes, bytesLeft);
         mBufOfs += bytesLeft;
      }

      return n;
   }

   virtual uint64 curOfs(void) const
   {
      return mBufOfs;
   }

   virtual uint64 bytesLeft(void) const
   {
      return mBuf.size() - mBufOfs;
   }

   virtual int64 seek(int64 ofs64, bool absolute = true)
   {
      if (!seekable())
         return -1;
         
      if ((ofs64 < INT_MIN) || (ofs64 > INT_MAX))
         return -1;
         
      int ofs = static_cast<int>(ofs64);         

      if (absolute)
         mBufOfs = static_cast<uint>(ofs);
      else
      {
         if ((ofs < 0) && ((uint)(-ofs) > mBufOfs))
            mBufOfs = 0;
         else
            mBufOfs += static_cast<uint>(ofs);
      }
      mBufOfs = Math::Min(mBufOfs, mBuf.size());
      return mBufOfs;
   }

   virtual const void* ptr(uint len = 0) 
   { 
      if ((len > mBuf.size()) || (mBuf.empty()))
         return NULL;
      return &mBuf[0];
   } 
   
   const BByteArray& getBuf(void) const  { return mBuf; }
         BByteArray& getBuf(void)        { return mBuf; }
         
};  // class BDynamicStream
