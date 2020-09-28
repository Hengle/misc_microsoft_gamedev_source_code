//==============================================================================
// contentfile.h
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

// this entire class should only be used on the xbox
// we already have file managers for win32 access
// the XContent APIs are strictly console (or vista at some point)
#pragma once

#ifdef XBOX

//==============================================================================
// 
//==============================================================================
class BContentFile
{
   public:
      //-- Construction/Destruction
      BContentFile();
      BContentFile(const XCONTENT_DATA& contentData);
      ~BContentFile();

      DWORD createContent(const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags, XOVERLAPPED* pOverlapped=NULL);

      DWORD createContent(DWORD userIndex, DWORD contentFlags, XOVERLAPPED* pOverlapped=NULL);

      BOOL open(uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags);

      BOOL setFilePointer(uint64 filepointer, DWORD moveMethod);
      BOOL read (void* pBuffer, uint numBytes, uint* pNumBytesActual, LPOVERLAPPED overlapped=NULL);
      BOOL write(const void* pBuffer, uint numBytes, uint* pNumBytesActual, LPOVERLAPPED overlapped=NULL);
      BOOL close(void);
      void flush(void);

      DWORD getLastError();

      HANDLE getHandle() { return mHandle; }
      
   protected:

      void init();

      HANDLE         mHandle;       // file handle

      XCONTENT_DATA  mContentData;  // instantiate this so it persists for overlapped IO

      BString        mRootname;     // ie "phx" we map a root to this: "phx:\\file.txt"
      BString        mFilename;     // "phx:\\save.txt"

      bool isValidHandle() const { return (mHandle != INVALID_HANDLE_VALUE);}
};

#endif // XBOX