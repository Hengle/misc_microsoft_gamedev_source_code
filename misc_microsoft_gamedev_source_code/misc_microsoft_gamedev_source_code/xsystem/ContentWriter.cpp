//============================================================================
//  File: ContentWriter.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "xsystem.h"
#include "contentfile.h"
#include "contentwriter.h"
#include "buffercache.h"

#ifdef XBOX

//============================================================================
// We want to be able to run this is synchronous and async mode. It would be 
// nice to be able to use overlapped IO for the async stuff but the XBOX SDK 
// states that writes can pause for a long period of time even though 
// overlapped IO is being used and recommends putting the writes on a 
// separate thread.
//============================================================================
BContentWriter::BContentWriter(BContentFile* pFile) :
   mFileOfs(0),
   mpCache(NULL),
   mEventHandleIO(cInvalidEventReceiverHandle),
   mEventHandleSim(cInvalidEventReceiverHandle),
   mGoAsync(true),
   mpNotify(NULL),
   mIOCount(0),
   mCanDelete(false),
   mState(cStateOpen)
{
   mpContentFile = pFile;
   mpCache = new BBufferCache();

   mEventHandleIO = gEventDispatcher.addClient(this, cThreadIndexIO);
   mEventHandleSim = gEventDispatcher.addClient(this, cThreadIndexSim);
}

//============================================================================
//============================================================================
BContentWriter::~BContentWriter()
{
   // we should be able to remove this directly.
   if (mEventHandleSim != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientImmediate(mEventHandleSim);
      mEventHandleSim = cInvalidEventReceiverHandle;
   }

   if (mpCache)
      delete mpCache;
}

//============================================================================
//============================================================================
bool BContentWriter::deinit()
{
   // we can't shut down when there is work to do.
   if (mIOCount > 0)
      return false;

   // remove the registration on the IO helper. We have to wait for this to occur
   if (mEventHandleIO != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientDeferred(mEventHandleIO, true);
      mEventHandleIO = cInvalidEventReceiverHandle;
   }
   return true;
}

//============================================================================
//============================================================================
bool BContentWriter::flushBuffer()
{
   // Anything to write?
   if (mpCache->isCacheEmpty())
      return true;

   // write (non-async... for now)
   uint64 filepointer = mFileOfs;
   //uint bytesToWrite = mpCache->getCacheBytesAllocated();
   uint bytesToWrite = mpCache->bytesAvailToRead();
   uint bytesWritten = bytesToWrite;

   if (mGoAsync)
   {
      BBufferCache* pCache = mpCache;
      mpCache = new BBufferCache();          // fixme - we should use a recycled queue of these.

      pCache->setFilePointer(filepointer);

      // send this off to the other thread now.
      BContentWriterPayload* pPayload = new BContentWriterPayload(pCache);
      mIOCount++;  // flag that we are doing async IO
      gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleIO, cContentWriterEventWrite, 0, 0, pPayload);
   }
   else
   {
      bytesWritten = writeToFile(mpCache->getPtr(), bytesToWrite, filepointer);

      if (bytesToWrite == bytesWritten)
      {
         mpCache->resetCache();
      }
      else
      {
         mpCache->setReadIndex(bytesWritten);
      }

      //if (bytesToWrite != bytesWritten)
      //   return false;

      // reset the cache
      //mpCache->resetCache();
   }

   //mFileOfs += bytesToWrite;
   mFileOfs += bytesWritten;

   return true;
}   

//============================================================================
//============================================================================
bool BContentWriter::close()
{
   if (!mpContentFile)
      return true;

   bool success = flushBuffer();
   mpContentFile->close();
   return success;
}

//============================================================================
//============================================================================
bool BContentWriter::flush() 
{
   bool success = flushBuffer();
   return success;
}

//============================================================================
//============================================================================
uint BContentWriter::writeBytes(const void* p, uint n)
{
   BASSERT(p);

   if (n==0)
      return 0;

   // We need to write more than the cache will hold, so flush the current 
   // cache and then write out the buffer
   if (n >= mpCache->getCacheSize())
   {
      // write the data out and update the file offset pointer
      if (!flushBuffer())
         return 0;

      // fixme - this will cause problems
      //uint bytesWritten = writeToFile(p, n, mFileOfs);
      //mFileOfs += bytesWritten;             // move the file pointer along for the next write

      //return bytesWritten;
   }

   // Write to the cache 
   uint totalBytesWritten = 0;
   const void* pBufferOffset = p;
   uint bytesToWrite = n;
   while (totalBytesWritten < n)
   {
      // check for full conditions
      if (mpCache->isCacheFull())
      {
         if (!flushBuffer())
            return 0; // couldn't write to the file
      }

      // write to the cache
      uint bytesWritten = mpCache->writeToCache(pBufferOffset, bytesToWrite);
      pBufferOffset = static_cast<const BYTE*>(pBufferOffset) + bytesWritten;
      bytesToWrite -= bytesWritten;
      totalBytesWritten += bytesWritten;
   }

   return totalBytesWritten;
}

//============================================================================
//============================================================================
uint32 BContentWriter::writeToFile(const void* p, uint n, uint64 filepointer)
{
   if (!mpContentFile->setFilePointer(filepointer, FILE_BEGIN))
      return 0;

   uint bytesWritten = 0;
   mpContentFile->write(p, n, &bytesWritten);

   return bytesWritten;
}

//============================================================================
//============================================================================
BOOL BContentWriter::setFilePointer(uint64 filepointer)
{
   // flush what I have currently in the buffer at the current file offset
   flushBuffer();

   // move the file offset to the new location
   if (!mpContentFile->setFilePointer(filepointer, FILE_BEGIN))
      return false;

   mFileOfs = filepointer;

   return true;
}

//==============================================================================
// writeInt
//==============================================================================
uint BContentWriter::writeInt(int data)
{
#ifdef XBOX
   EndianSwitchDWords((DWORD*)&data, 1);
#endif
   return writeBytes((void*)&data, sizeof(data));
} //writeInt

//==============================================================================
// writeInt
//==============================================================================
uint BContentWriter::writeUint(uint data)
{
#ifdef XBOX
   EndianSwitchDWords((DWORD*)&data, 1);
#endif
   return writeBytes((void*)&data, sizeof(data));
} //writeInt

//==============================================================================
// BContentWriter::writeBool
//==============================================================================
uint BContentWriter::writeBool(bool data)
{
   return writeBytes((void*)&data, sizeof(data));
}

//==============================================================================
// BContentWriter::writeBString
//==============================================================================
bool BContentWriter::writeBString(BString& string)
{
   DWORD length = string.length();

   DWORD outLength = length;

#ifndef UNICODE
   outLength |= 0x80000000;
#endif

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&outLength, 1);
#endif

   if (!writeBytes(&outLength, sizeof(outLength)))
      return(false);

   if (length == 0)
      return 0;

#if defined(XBOX) && defined(UNICODE)
   BString outString(string.getPtr());
   outString.endianSwap();
   if (!writeBytes(const_cast<BCHAR_T*>(outString.getPtr()), length*sizeof(BCHAR_T)))
      return(false);
#else
   if (!writeBytes(const_cast<BCHAR_T*>(string.getPtr()), length*sizeof(BCHAR_T)))
      return(false);
#endif

   return(true);
}

//==============================================================================
// BContentWriter::receiveEvent
//==============================================================================
bool BContentWriter::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (threadIndex)
   {
      case cThreadIndexIO:
         {
            switch (event.mEventClass)
            {
               case cEventClassClientRemove:
                  {
                     // pass a message back to the other thread saying we are ready for delete.
                     gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cContentWriterEventRemoved);
                     break;
                  }

               case cContentWriterEventWrite:
                  {
                     // write the data out.
                     const BContentWriterPayload* pPayload = static_cast<const BContentWriterPayload*>(event.mpPayload);

                     BBufferCache* pCache = pPayload->mpCache;

                     // write out the data synchronously on this thread
                     uint bytesToWrite = pCache->getCacheBytesAllocated();
                     uint64 filepointer = pCache->getFilePointer();
                     uint bytesWritten = writeToFile(pCache->getPtr(), bytesToWrite, filepointer);

                     if (bytesToWrite != bytesWritten)
                     {
                        // fixme - how to I get errors back to the other thread and handle them cleanly
                     }

                     // clean this up, fixme - we might want to recycle this to minimize memory thrashing
                     BContentWriterPayloadReply* pPayloadReply = new BContentWriterPayloadReply(pCache);
                     gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandleSim, cContentWriterEventWriteReply, 0, 0, pPayloadReply);

                  }
                  break;
            }
         }
         break;

      case cThreadIndexSim:
         {
            switch (event.mEventClass)
            {
               case cContentWriterEventRemoved:
                  {
                     mCanDelete = true;
                  }
                  break;

               case cContentWriterEventWriteReply:
                  {
                     // write the data out.
                     //const BContentWriterPayloadReply* pPayload = static_cast<const BContentWriterPayloadReply*>(event.mpPayload);

                     //BBufferCache* pCache = pPayload->mpCache;
                     // the deletion will occur by the event dispatcher since we are returning false
                     // if we wish to delete the buffer cache here, then we need to either return false
                     // or cleanup the payload so we don't crash when the event dispatcher deletes it
                     //delete pCache;

                     mIOCount--;  // flag that we have finished an async IO request

                     // fixme - should I do anything else here? Errors?
                  }
                  break;
            }

         }
         break;
   }

   return false;
}

#endif // XBOX