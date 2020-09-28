//==============================================================================
// filestream.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#ifndef _FILESTREAM_H_
#define _FILESTREAM_H_

//==============================================================================
// Includes
#include "datastream.h"

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
class BFileStream :  public BDataStream
{
   public:
      BFileStream(BDataStreamTransport *transport, unsigned char streamID);
      virtual ~BFileStream();

      virtual bool            startFileSend(long dir, const BCHAR_T* filename, long size=-1);
      virtual bool            startFileRecv(long dir, const BCHAR_T* filename, long size=-1, DWORD crc=0);
      virtual void            stopFileTransfer(void);

      float                   getFilePercent(void) const { return(mFilePercent); }
      DWORD                   getFileCRC(void) const { return(mFileCRC); }
      long                    getFileSize(void) const;

      long                    getStreamID() { return (long)mStreamID; }

      BCHAR_T*                  getFilename() { return mFilename; }

      // from BDataStream
      virtual DWORD           getDataFromSource(unsigned char &streamID, void* buffer, DWORD size);
      virtual bool            putDataToDestination(const void* buffer, DWORD size);

   protected:
      long                    getFilePosition(void) const;
      virtual void            streamComplete(void);

   private:
      BFile                   *mpFile;
      BCHAR_T                   mFilename[MAX_PATH];
      DWORD                   mFileCRC;
      long                    mEndPos;
      long                    mStartPos;
      float                   mFilePercent;
};

//==============================================================================
#endif // _FILESTREAM_H_

//==============================================================================
// eof: filestream.h
//==============================================================================

