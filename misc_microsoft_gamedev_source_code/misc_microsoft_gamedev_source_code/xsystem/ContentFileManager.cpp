//==============================================================================
// ContentFileManager.cpp
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

// this entire class should only be used on the xbox
// we already have file managers for win32 access
// the XContent APIs are strictly console (or vista at some point)
#include "xsystem.h"
#include "ContentFileManager.h"

#include "contentfile.h"
#include "ContentWriter.h"
#include "ContentReader.h"

#ifdef XBOX

// Globals
BContentFileManager gContentFileManager;

//==============================================================================
// BXCreateContentAsyncTask::BXCreateContentAsyncTask
//==============================================================================
BXCreateContentAsyncTask::BXCreateContentAsyncTask() :
   BAsyncTask(),
   mpContentFile(NULL),
   mpNotify(NULL),
   mMode(cModeFile),
   mOpenFlags(GENERIC_READ),
   mShareFlags(FILE_SHARE_READ),
   mCreateDisposition(OPEN_EXISTING),
   mdwFlags(FILE_ATTRIBUTE_NORMAL)
{
}

//==============================================================================
// BXCreateContentAsyncTask::~BXCreateContentAsyncTask
//==============================================================================
BXCreateContentAsyncTask::~BXCreateContentAsyncTask()
{
}

//==============================================================================
DWORD BXCreateContentAsyncTask::createContent(BContentFile* pContentFile, const char* pFilename, const WCHAR* pDisplayName, 
                                              DWORD contentType, XCONTENTDEVICEID deviceID, DWORD port, DWORD contentFlags)
{
   //   HANDLE hEventComplete = CreateEvent( NULL, FALSE, FALSE, NULL );
   //   xOverlapped.hEvent = hEventComplete;

   DWORD result = pContentFile->createContent(pFilename, pDisplayName, contentType, deviceID, port, contentFlags, &mOverlapped);

   mpContentFile = pContentFile;

   return result;
}

//==============================================================================
// 
//==============================================================================
DWORD BXCreateContentAsyncTask::createContent(BContentFile* pContentFile, DWORD userIndex, DWORD contentFlags)
{
   BDEBUG_ASSERT(pContentFile);

   DWORD result = pContentFile->createContent(userIndex, contentFlags, &mOverlapped);

   mpContentFile = pContentFile;

   return result;
}

//==============================================================================
// Only called when cleaning up a failed task
//==============================================================================
void BXCreateContentAsyncTask::clean()
{
   delete mpContentFile;
   mpContentFile = NULL;

   mpNotify = NULL;
}

//==============================================================================
// BFileManager
// Note: The file handles for the content file starts at 1, 0 is invalid.
//==============================================================================
BContentFileManager::BContentFileManager()
{
}

//==============================================================================
// BFileManager
//==============================================================================
BContentFileManager::~BContentFileManager()
{
}

//==============================================================================
// BFileManager::openFile
// 
// pNotify - the call back to notify the user on.
// pFilename - the name of the file
// pDisplayName - the name to display
// contentType - will almost always be XCONTENTTYPE_SAVEDGAME
// deviceID - return from XShowDeviceSelectorUI()
// userIndex - the port number of the user (corresponds to controller index)
// contentFlags - See XContentCreate for a list of valid flags
// openFlags - see CreateFile for options
// shareFlags - see CreateFile for options
// createDisposition - see CreateFile for options
// dwFlags - see CreateFile for options
// TODO: simplify the above options.
//==============================================================================

DWORD BContentFileManager::openWriter(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
                                       uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags)
{
   return open(pNotify, pFilename, pDisplayName, contentType, deviceID, userIndex, contentFlags, openFlags, shareFlags, createDisposition, dwFlags, BXCreateContentAsyncTask::cModeWriter);
}

//==============================================================================
// BFileManager::openFile
// 
// pNotify - the call back to notify the user on.
// pFilename - the name of the file
// pDisplayName - the name to display
// contentType - will almost always be XCONTENTTYPE_SAVEDGAME
// deviceID - return from XShowDeviceSelectorUI()
// userIndex - the port number of the user (corresponds to controller index)
// contentFlags - See XContentCreate for a list of valid flags
// openFlags - see CreateFile for options
// shareFlags - see CreateFile for options
// createDisposition - see CreateFile for options
// dwFlags - see CreateFile for options
// TODO: simplify the above options.
//==============================================================================
DWORD BContentFileManager::openReader(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
                                uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags)
{
   return open(pNotify, pFilename, pDisplayName, contentType, deviceID, userIndex, contentFlags, openFlags, shareFlags, createDisposition, dwFlags, BXCreateContentAsyncTask::cModeReader);
}

//==============================================================================
// BFileManager::openFile
// 
// pNotify - the call back to notify the user on.
// pFilename - the name of the file
// pDisplayName - the name to display
// contentType - will almost always be XCONTENTTYPE_SAVEDGAME
// deviceID - return from XShowDeviceSelectorUI()
// userIndex - the port number of the user (corresponds to controller index)
// contentFlags - See XContentCreate for a list of valid flags
// openFlags - see CreateFile for options
// shareFlags - see CreateFile for options
// createDisposition - see CreateFile for options
// dwFlags - see CreateFile for options
// TODO: simplify the above options.
//==============================================================================
DWORD BContentFileManager::openFile(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
                                uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags)
{
   return open(pNotify, pFilename, pDisplayName, contentType, deviceID, userIndex, contentFlags, openFlags, shareFlags, createDisposition, dwFlags, BXCreateContentAsyncTask::cModeFile);
}

//==============================================================================
// The XCONTENT_DATA structure will be populated elsewhere
//==============================================================================
DWORD BContentFileManager::openFile(BContentFileNotify* pNotify, const XCONTENT_DATA& contentData, DWORD userIndex, DWORD dwContentFlags)
{
   BContentFile* pContentFile = new BContentFile(contentData);

   // fire off the creating save content state
   BXCreateContentAsyncTask* pTask = new BXCreateContentAsyncTask();

   DWORD result = pTask->createContent(pContentFile, userIndex, dwContentFlags);
   pTask->setFileNotify(pNotify);
   pTask->setMode(BXCreateContentAsyncTask::cModeFile);

   if (result != ERROR_IO_PENDING)
   {
      delete pTask;
      return result;
   }

   pTask->setNotify(this);

   // Let the task manager finish the call
   gAsyncTaskManager.addTask(pTask);

   return result;
}

//==============================================================================
// BFileManager::openFile
// 
// pNotify - the call back to notify the user on.
// pFilename - the name of the file
// pDisplayName - the name to display
// contentType - will almost always be XCONTENTTYPE_SAVEDGAME
// deviceID - return from XShowDeviceSelectorUI()
// userIndex - the port number of the user (corresponds to controller index)
// contentFlags - See XContentCreate for a list of valid flags
// openFlags - see CreateFile for options
// shareFlags - see CreateFile for options
// createDisposition - see CreateFile for options
// dwFlags - see CreateFile for options
// TODO: simplify the above options.
DWORD BContentFileManager::open(BContentFileNotify* pNotify, const char* pFilename, const WCHAR* pDisplayName, DWORD contentType, XCONTENTDEVICEID deviceID, DWORD userIndex, DWORD contentFlags,
                                    uint openFlags, uint shareFlags, uint createDisposition, DWORD dwFlags, uint mode)
{
   BContentFile* pContentFile = new BContentFile();

   // fire off the creating save content state
   BXCreateContentAsyncTask* pTask = new BXCreateContentAsyncTask();

   BOOL result = pTask->createContent(pContentFile, pFilename, pDisplayName, XCONTENTTYPE_SAVEDGAME, deviceID, userIndex, XCONTENTFLAG_OPENEXISTING);
   pTask->setOpenFlags(openFlags);
   pTask->setShareFlags(shareFlags);
   pTask->setCreateDisposition(createDisposition);
   pTask->setFlags(dwFlags);
   pTask->setFileNotify(pNotify);
   pTask->setMode(mode);

   if (result != ERROR_IO_PENDING)
   {
      delete pTask;
      return result;
   }

   pTask->setNotify(this);

   // Let the task manager finish the call
   gAsyncTaskManager.addTask(pTask);

   return result;
}

//==============================================================================
//==============================================================================
bool BContentFileManager::closeReader(BContentReader* pReader)
{
   BContentFile* pFile = pReader->getFile();

   pReader->close();
   delete pReader;

   closeFile(pFile);

   return true;
}

//==============================================================================
//==============================================================================
bool BContentFileManager::closeWriter(BContentWriter* pWriter)
{
   pWriter->flush();
   pWriter->setState(BContentWriter::cStateIOPending);
   mWritersToClose.add(pWriter);
   return true;
}

//==============================================================================
//==============================================================================
bool BContentFileManager::closeFile(BContentFile* pFile)
{
   pFile->close();
   delete pFile;

   return true;
}

//==============================================================================
//==============================================================================
void BContentFileManager::notify(DWORD eventID, void* pTask)
{
   BDEBUG_ASSERT(pTask);

   // This is the BXCreateContentAsyncTask
   BXCreateContentAsyncTask* pContentTask = reinterpret_cast<BXCreateContentAsyncTask*>(pTask);
   DWORD result = pContentTask->getResult();
   if (FAILED(result))
   {
      switch (pContentTask->getMode())
      {
         case BXCreateContentAsyncTask::cModeFile:
            pContentTask->getFileNotify()->fileNotify(NULL, result);
            break;
         case BXCreateContentAsyncTask::cModeWriter:
            pContentTask->getFileNotify()->fileNotifyWriter(NULL, result);
            break;
         case BXCreateContentAsyncTask::cModeReader:
            pContentTask->getFileNotify()->fileNotifyReader(NULL, result);
            break;
      }
      pContentTask->clean();
      return;
   }

   BContentFile* pContentFile = pContentTask->getContentFile();
   BDEBUG_ASSERT(pContentFile);

   // do not attempt to open the file if we have not requested a reader/writer
   if (pContentTask->getMode() == BXCreateContentAsyncTask::cModeFile)
   {
      pContentTask->getFileNotify()->fileNotify(pContentFile, ERROR_SUCCESS);
      return;
   }

   // open the file and send the result to the client.
   BOOL res2 = pContentFile->open(pContentTask->getOpenFlags(), pContentTask->getShareFlags(), pContentTask->getCreateDisposition(), pContentTask->getFlags());
   if (!res2)
   {
      switch (pContentTask->getMode())
      {
         case BXCreateContentAsyncTask::cModeWriter:
            pContentTask->getFileNotify()->fileNotifyWriter(NULL, pContentFile->getLastError());
            break;
         case BXCreateContentAsyncTask::cModeReader:
            pContentTask->getFileNotify()->fileNotifyReader(NULL, pContentFile->getLastError());
            break;
      }
      pContentTask->clean();
      return;
   }

   switch (pContentTask->getMode())
   {
      case BXCreateContentAsyncTask::cModeWriter:
         {
            // wrap this in a writer
            BContentWriter* pWriter = new BContentWriter(pContentFile);
            pContentTask->getFileNotify()->fileNotifyWriter(pWriter, ERROR_SUCCESS);
            break;
         }
      case BXCreateContentAsyncTask::cModeReader:
         {
            // wrap this in a writer
            BContentReader* pReader = new BContentReader(pContentFile);
            pContentTask->getFileNotify()->fileNotifyReader(pReader, ERROR_SUCCESS);
            break;
         }
   }

   return;
}

//==============================================================================
//==============================================================================
void BContentFileManager::update()
{
   for(int i=0; i<mWritersToClose.getNumber(); i++)
   {
      BContentWriter *pWriter = mWritersToClose[i];

      switch (pWriter->getState())
      {
         case BContentWriter::cStateOpen:
            break;
         case BContentWriter::cStateIOPending:
            {
               // all pending IO must be flushed before we delete the object.
               if (!pWriter->isIOPending())
               {
                  if (pWriter->deinit())
                     pWriter->setState(BContentWriter::cStateWaitForDelete);
               }
            }
            break;
         case BContentWriter::cStateWaitForDelete:
            {
               if (pWriter->canDelete())
               {
                  BContentFile * pFile = pWriter->getFile();

                  // remove it from this list
                  mWritersToClose.removeIndex(i);
                  i--;

                  delete pWriter;
                  delete pFile;
               }
            }
            break;
      }
   }
   return;
}

#endif // XBOX