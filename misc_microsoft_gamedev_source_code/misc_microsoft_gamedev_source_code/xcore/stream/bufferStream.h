//============================================================================
//
//  File: bufferStream.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once

//============================================================================
// class BBufferStream
// This class adds basic buffering overtop of any BStream object.
// This class is intended to accelerate lots of small sequential reads/writes.
// The buffer is always flushed when transitioning between reading/writing.
// Seeking always invalidates the buffer if it contains dirty write data.
//============================================================================
class BBufferStream : public BStream
{
   BBufferStream(const BBufferStream& b);
   BBufferStream& operator=(const BBufferStream& b);
         
   BStream*    mpDstStream;
   
   int64       mFileOfs;

   BByteArray  mBuf;
   int64       mBufFileOfs;
   uint        mBufValidSize;
   bool        mBufWriting;
            
   bool flushBuffer(void)
   {
      bool success = true;
      
      // Flush the buffer if it's writeable data.
      if ((mBufFileOfs >= 0) && (mBufWriting) && (mBufValidSize))
      {
         if (!mpDstStream->seekOrSkip(mBufFileOfs))
            success = false;
         else
         {
            if (mpDstStream->writeBytes(mBuf.getPtr(), mBufValidSize) != mBufValidSize)
               success = false;
         }               
      }
      
      mBufFileOfs = -1;
      mBufValidSize = 0;
      mBufWriting = false;
      return success;
   }   
   
public:
   enum { cDefaultBufSize = 8192 };
   
   BBufferStream() :
      mpDstStream(NULL),
      mFileOfs(0),
      mBufFileOfs(-1),
      mBufValidSize(0),
      mBufWriting(false)
   {
   }      

   BBufferStream(BStream& dstStream, uint bufSize = cDefaultBufSize) :
      BStream(dstStream.getName(), dstStream.getFlags()),
      mpDstStream(&dstStream),
      mFileOfs(0),
      mBufFileOfs(-1),
      mBufValidSize(0),
      mBufWriting(false),
      mBuf(bufSize)
   { 
   }
   
   virtual bool close(void)
   {
      return close(false);
   }
   
   BStream* getDstStream(void)
   {
      return mpDstStream;
   }
   
   virtual bool opened(void) const 
   { 
      return NULL != mpDstStream;
   }
   
   bool close(bool closeDestStream)
   {
      if (!mpDstStream)
         return true;
         
      bool success = flushBuffer();
      
      if (closeDestStream)
      {
         if (!mpDstStream->close())
            success = false;
      }            
      else
      {
         if (!mpDstStream->flush())
            success = false;
      }
                              
      mpDstStream = NULL;
      
      mBuf.clear();
      
      return success;
   }

   // bufSize may be 0, which disables all buffering.
   bool open(BStream& dstStream, uint bufSize = cDefaultBufSize)
   {
      if (!close())
         return false;
      
      setName(dstStream.getName());
      setFlags(dstStream.getFlags());
      mpDstStream = &dstStream;
      mBuf.resize(bufSize);
            
      mFileOfs = 0;
      mBufFileOfs = -1;
      mBufValidSize = 0;
      mBufWriting = false;
      
      return true;
   }
   
   virtual bool flush(void) 
   {
      if (!mpDstStream)
         return false;
               
      bool success = flushBuffer();
      
      if (!mpDstStream->flush())
         success = false;
      
      return success;
   }

   virtual uint64 size(void) const
   {
      if (!mpDstStream)
         return 0;
      
      if (!mpDstStream->sizeKnown())
         return BSTREAM_UNKNOWN_SIZE;
         
      uint64 result = mpDstStream->size();
      
      if ((mBufFileOfs >= 0) && (mBufWriting))
         result = Math::Max<uint64>(mBufFileOfs + mBufValidSize, result);
      
      return result;
   }
   
   virtual bool sizeKnown(void) const
   {
      if (!mpDstStream)
         return false;
         
      return mpDstStream->sizeKnown();
   }
   
   virtual uint64 curOfs(void) const
   {
      if (!mpDstStream)
         return 0;
         
      return mFileOfs;
   }
   
   virtual uint64 bytesLeft(void) const
   {
      if (!mpDstStream)
         return 0;
      
      if (!sizeKnown())
         return INT64_MAX;
      
      return size() - curOfs();
   }

   virtual uint readBytes(void* p, uint n)
   {  
      BASSERT(p);
      
      if ((!mpDstStream) || (!getReadable()) || (!n))
         return 0;
            
      if (!mBuf.getSize())
      {
         if (!mpDstStream->seekOrSkip(mFileOfs))
            return 0;
            
         uint bytesRead = mpDstStream->readBytes(p, n);
         mFileOfs += bytesRead;
         return bytesRead;
      }
      
      uint totalBytesRead = 0;
         
      // First satisfy as much as possible from the read buffer -- but only if the read begins somewhere inside the read buffer.
      if ((mBufFileOfs >= 0) && (!mBufWriting) && (mFileOfs >= mBufFileOfs) && (mFileOfs < (mBufFileOfs + mBufValidSize)))
      {
         uint bytesToCopy = Math::Min<uint>(n, (uint)((mBufFileOfs + mBufValidSize) - mFileOfs));
         
         memcpy(p, mBuf.getPtr() + (uint)(mFileOfs - mBufFileOfs), bytesToCopy);
         
         p                  = static_cast<BYTE*>(p) + bytesToCopy;
         n                 -= bytesToCopy;
         totalBytesRead    += bytesToCopy;
         mFileOfs          += bytesToCopy;
         
         if (!n)
            return totalBytesRead;
      }               
            
      if (!flushBuffer())
         return 0;

      if (!mpDstStream->seekOrSkip(mFileOfs))
         return totalBytesRead;
               
      // Don't use the buffer if the read is too large.
      if (n >= mBuf.getSize())
      {
         uint bytesRead = mpDstStream->readBytes(p, n);
         mFileOfs       += bytesRead;
         totalBytesRead += bytesRead;
         return totalBytesRead;
      }
            
      // Fill the buffer as much as possible.
      mBufFileOfs = mFileOfs;
      mBufWriting = false;
      mBufValidSize = mpDstStream->readBytes(mBuf.getPtr(), mBuf.getSizeInBytes());
      if (mBufValidSize < mBuf.getSizeInBytes())
      {
         if (mpDstStream->errorStatus())
            return totalBytesRead;
      }
      
      // Copy what we can from the buffer.
      uint bytesToCopy = Math::Min<uint>(n, mBufValidSize);

      memcpy(p, mBuf.getPtr(), bytesToCopy);
      
      totalBytesRead    += bytesToCopy;
      mFileOfs          += bytesToCopy;
      
      return totalBytesRead;
   }

   virtual uint writeBytes(const void* p, uint n)
   {
      BASSERT(p);
      
      if ((!mpDstStream) || (!getWritable()) || (!n))
         return 0;

      if (n >= mBuf.getSizeInBytes())
      {
         if (!flushBuffer())
            return 0;
            
         if (!mpDstStream->seekOrSkip(mFileOfs))
            return 0;

         uint bytesWritten = mpDstStream->writeBytes(p, n);
         mFileOfs += bytesWritten;
         return bytesWritten;
      }
      
      uint totalBytesWritten = 0;
            
      do
      {
         if ((mBufFileOfs < 0) || (!mBufWriting))
         {
            mBufFileOfs = mFileOfs;
            mBufValidSize = 0;
            mBufWriting = true;
         }
         
         if ((mFileOfs >= mBufFileOfs) && (mFileOfs <= (mBufFileOfs + mBufValidSize)))
         {
            uint bytesToCopy = Math::Min<uint>(n, (uint)((mBufFileOfs + mBuf.getSizeInBytes()) - mFileOfs));
            
            memcpy(mBuf.getPtr() + (uint)(mFileOfs - mBufFileOfs), p, bytesToCopy);

            mBufValidSize     = Math::Max<uint>(mBufValidSize, (uint)(mFileOfs + bytesToCopy - mBufFileOfs));
            
            if (mBufValidSize == mBuf.getSizeInBytes())
            {
               if (!flushBuffer())
                  return 0;
            }
            
            p                  = static_cast<const BYTE*>(p) + bytesToCopy;
            n                 -= bytesToCopy;
            totalBytesWritten += bytesToCopy;
            mFileOfs          += bytesToCopy;
         }  
         else if (!flushBuffer())
            return 0;
            
      } while (n);           
      
      return totalBytesWritten;
   }
   
   virtual int64 seek(int64 ofs64, bool absolute = true)
   {
      if (!mpDstStream)
         return -1;
      
      int64 newOfs = absolute ? ofs64 : (mFileOfs + ofs64);
      if (newOfs < 0)
         newOfs = 0;
      else if (sizeKnown())
      {
         if ((uint64)newOfs >= size())
            newOfs = size();
      }
      
      // Always flush dirty write buffers.
      if ((mBufFileOfs >= 0) && (mBufWriting))
      { 
         if (!flushBuffer())     
            return -1;
      }
                  
      if (!getSeekable())
      {
         // Purposely don't let the caller seek backwards if the underlying stream doesn't support seeking, even if 
         // the buffer could sometimes satisfy the seek request.
         if (newOfs < mFileOfs)
            return -1;
      }
      
      mFileOfs = newOfs;
      
      return newOfs;
   }
         
   virtual const void* ptr(uint len = 0)
   { 
      if (!mpDstStream)
         return NULL;
      
      if (!flushBuffer())
         return NULL;
                  
      return mpDstStream->ptr(len);
   } 
   
   virtual bool setReadable(bool flag) 
   { 
      if (!mpDstStream)
         return false;

      if (!flushBuffer())
         return false; 
      
      if (flag == getReadable())
         return true;
      
      if (flag)
      {
         if (!mpDstStream->getReadable())
            return false;
      }
      
      BStream::setReadable(flag);
      return true;
   }
   
   virtual bool setWritable(bool flag) 
   { 
      if (!mpDstStream)
         return false;
      
      if (!flushBuffer())
         return false; 
         
      if (flag == getWritable())
         return true;
            
      if (flag)
      {
         if (!mpDstStream->getWritable())  
            return false;
      }
      
      BStream::setWritable(flag);
      return true;
   }
   
   virtual bool setSeekable(bool flag)
   {
      if (!mpDstStream)
         return false;
         
      if (!flushBuffer())
         return false;
            
      if (flag == getSeekable())
         return true;
         
      if (flag) 
      {
         if (!mpDstStream->getSeekable())
            return false;
      }
      else
      {
         if (!mpDstStream->seekOrSkip(mFileOfs))
            return false;
      }
      
      BStream::setSeekable(flag);
      return true;
   }
   
   virtual void setOwnerThread(DWORD threadID) 
   { 
      if (mpDstStream)
         mpDstStream->setOwnerThread(threadID);
   }
   
   virtual bool errorStatus(void) const 
   {
      if (mpDstStream) 
         return mpDstStream->errorStatus();
         
      return false;
   } 
   
}; // class BBufferStream























































