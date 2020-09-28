//==============================================================================
// ContentFileManager.h
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

// this entire class should only be used on the xbox
// we already have file managers for win32 access
// the XContent APIs are strictly console (or vista at some point)
#pragma once

#ifdef XBOX

#include "AsyncTaskManager.h"

class BContentFile;
class BContentWriter;
class BContentReader;

//==============================================================================
//==============================================================================
class BContentFileNotify
{
   public:
      virtual void fileNotify(BContentFile* pFile, DWORD lastError) { pFile; lastError; }
      virtual void fileNotifyWriter(BContentWriter* pWriter, DWORD lastError) { pWriter; lastError; }
      virtual void fileNotifyReader(BContentReader* pReader, DWORD lastError) { pReader; lastError; }
};

//==============================================================================
// BXCreateContentAsyncTask - create content from device
//==============================================================================
class BXCreateContentAsyncTask : public BAsyncTask
{
   public:
      enum
      {
         cModeFile,
         cModeReader,
         cModeWriter
      };

      BXCreateContentAsyncTask();
      ~BXCreateContentAsyncTask();

      DWORD createContent(BContentFile* pContentFile, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags);

      DWORD createContent(BContentFile* pContentFile, DWORD userIndex, DWORD contentFlags);

      void setOpenFlags(uint flags) { mOpenFlags = flags; }
      uint getOpenFlags() const { return mOpenFlags; }

      void setShareFlags(uint flags) { mShareFlags = flags; }
      uint getShareFlags() const { return mShareFlags; }

      void setCreateDisposition(uint disposition) { mCreateDisposition = disposition; }
      uint getCreateDisposition() const { return mCreateDisposition; }

      void setFlags(DWORD flags) { mdwFlags = flags; }
      DWORD getFlags() const { return mdwFlags; }

      void setMode(uint mode) { mMode = mode; }
      DWORD getMode() const { return mMode; }

      void setFileNotify(BContentFileNotify* pNotify) { mpNotify=pNotify; }
      BContentFileNotify* getFileNotify() const { return mpNotify; }

      BContentFile* getContentFile() const { return mpContentFile; }

      void clean();

   protected:
      BContentFile* mpContentFile;
      BContentFileNotify* mpNotify;

      // extra flags
      uint mMode;
      uint mOpenFlags;
      uint mShareFlags;
      uint mCreateDisposition;
      DWORD mdwFlags;
};

//==============================================================================
// 
//==============================================================================
class BContentFileManager : BAsyncNotify
{
public:
   BContentFileManager();
   ~BContentFileManager();

   // Opens or creates the file and returns the handle asynchronously.
   DWORD openWriter(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
                  uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags);

   DWORD openReader(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
                  uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags);

   DWORD openFile(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
                  uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags);

   DWORD openFile(BContentFileNotify* pNotify, const XCONTENT_DATA& contentData, DWORD userIndex, DWORD dwContentFlags);

   // should return the result async since we will have to wait for I/O and thread decoupling.
   bool closeReader(BContentReader* pReader);
   bool closeWriter(BContentWriter* pWriter);
   bool closeFile(BContentFile* pFile);

   // Async Task Notifications
   void notify(DWORD eventID, void* pTask);

   // for closing out the files async
   void update();

protected:

   // Internal Open
   DWORD open(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
      uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags, uint mode);

   // Member variables
   BDynamicArray<BContentWriter*> mWritersToClose;
};

extern BContentFileManager gContentFileManager;

#endif // XBOX