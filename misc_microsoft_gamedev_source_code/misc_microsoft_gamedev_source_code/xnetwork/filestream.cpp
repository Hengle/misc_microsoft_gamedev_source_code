//==============================================================================
// filestream.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "filestream.h"
//#include "netpackets.h"
//#/include "filesystem.h"


//==============================================================================
// BFileStream::BFileStream
//==============================================================================
BFileStream::BFileStream(BDataStreamTransport *transport, unsigned char streamID) :  BDataStream(transport, streamID),
                              mpFile(NULL), mStartPos(0), mEndPos(-1), 
                              mFileCRC(0), mFilePercent(0.0f)
{
}

//==============================================================================
// BFileStream::~BFileStream
//==============================================================================
BFileStream::~BFileStream() 
{ 
   if((mpFile) && (mpFile->isOpen())) 
      stopFileTransfer(); 
}

//==============================================================================
// BFileStream::startFileSend
//==============================================================================
bool BFileStream::startFileSend(long dir, const BCHAR_T *filename, long size)
{
   if (size <= 0)
      return(false);

   mpFile = new BFile;
   if (!mpFile || !mpFile->openReadOnly(dir, filename, 0))
   {
      delete mpFile; // yes it is 'ok' to delete NULL 
      return(false);
   }

   DWORD uSize;
   mpFile->getSize(uSize);   
   mpFile->getOffset((unsigned long&)mStartPos);   

   mEndPos = mStartPos + min((DWORD)size, uSize);

   return startDataSend();
}

//==============================================================================
// BFileStream::startFileRecv
//==============================================================================
bool BFileStream::startFileRecv(long dir, const BCHAR_T *filename, long size, DWORD crc)
{
   mpFile = new BFile;
   if (!mpFile || !mpFile->openWriteable(dir, filename, BFILE_OPEN_ENABLE_BUFFERING))
   {
      delete mpFile; // yes it is 'ok' to delete NULL 
      return(false);
   }

   mpFile->getOffset((unsigned long&)mStartPos);
   mEndPos = mStartPos+size;
   mFileCRC = crc;

   return startDataRecv();
}

//==============================================================================
// BFileStream::stopFileTransfer
//==============================================================================
void BFileStream::stopFileTransfer(void)
{
   stopDataTransfer();
   if (!mpFile)
      return;

   mpFile->close();
   delete mpFile;
   mpFile = NULL;
}

//==============================================================================
// BFileStream::getFilePosition
//==============================================================================
long BFileStream::getFilePosition(void) const
{
   if (!mpFile || (!mpFile->isOpen()) || (mStartPos == -1))
      return(-1);
   unsigned long offset;
   if(mpFile->getOffset(offset) == false)
      return(-1);
   return(offset-mStartPos);
}

//==============================================================================
// BFileStream::getDataFromSource
//==============================================================================
DWORD BFileStream::getDataFromSource(unsigned char &streamID, void* buffer, DWORD size)
{
   if (!buffer || !size || !mpFile || !mpFile->isOpen())
   {
      BASSERT(0);
      return 0;
   }

   streamID = mStreamID;

   if (mEndPos != -1)
   {
      unsigned long offset;
      mpFile->getOffset(offset);
      // if already at eof, return 0 bytes read
      if (offset >= (DWORD)mEndPos)
         return 0;
      // else if there is less data than requested, shrink size
      else if ((offset+size) > (unsigned)mEndPos)
         size = mEndPos-offset;
   }

   // read data from the file into the buffer
   DWORD bytesRead = mpFile->readEx(buffer, size);
   mFilePercent = (static_cast<float>(getFilePosition()) / static_cast<float>(getFileSize()));
   return(bytesRead);
}

//==============================================================================
// BFileStream::putDataToDestination
//==============================================================================
bool BFileStream::putDataToDestination(const void* buffer, DWORD size)
{
   BASSERT(buffer);
   BASSERT(mpFile);
   BASSERT(mpFile->isOpen());
   
   if ((!mpFile) || (!mpFile->isOpen()))
      return(false);

   if (!mpFile->write(buffer, size))   
      return(false);
      
   mFilePercent = (static_cast<float>(getFilePosition()) / static_cast<float>(getFileSize()));
   return(true);
}


//==============================================================================
// BFileStream::getFileSize
//==============================================================================
long BFileStream::getFileSize(void) const 
{
   long size = 0;

   if (mpFile)
   {
      if (mEndPos == -1)
      {
         unsigned long uSize = 0;
         bool result = mpFile->getSize(uSize);
         result;
         BASSERT(result);
         size = (long)uSize;
      }
      else
      {
         size = mEndPos - mStartPos;
      }
   }

   return size;
}

//==============================================================================
// BFileStream::streamComplete
//==============================================================================
void BFileStream::streamComplete(void)
{
   BDataStream::streamComplete();

   if (!mpFile)
      return;

   mpFile->close();
   delete mpFile;
   mpFile = NULL;
}
