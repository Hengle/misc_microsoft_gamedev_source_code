// File: clientAdvertisement.cp
#include "xcore.h"
#include "clientAdvertisement.h"
#include "file\win32File.h"
#include "file\win32FindFiles.h"

#define CLIENT_ADVERTISEMENT_PATH "\\\\esfile\\phoenix\\dataBuild\\activeClients"

BCriticalSection BClientAdvertisementManager::mMutex;
BString          BClientAdvertisementManager::mClientAdvertisementFilename;   

bool BClientAdvertisementManager::create(const char* pAddress)
{
   BScopedCriticalSection lock(mMutex);

   mClientAdvertisementFilename.format(CLIENT_ADVERTISEMENT_PATH "\\%s", pAddress);
   BWin32File file;
   
   bool success = false;
   if (!file.create(mClientAdvertisementFilename.getPtr()))
      gConsoleOutput.error("Unable to create file: %s\n", mClientAdvertisementFilename.getPtr());
   else
   {
      gConsoleOutput.printf("Created file: %s\n", mClientAdvertisementFilename.getPtr());
      success = true;
   }

   file.close();
   
   return success;
}

bool BClientAdvertisementManager::destroy(void)
{
   BScopedCriticalSection lock(mMutex);

   gConsoleOutput.printf("Deleting file: %s\n", mClientAdvertisementFilename.getPtr());
   remove(mClientAdvertisementFilename.getPtr());
   
   return true;
}

bool BClientAdvertisementManager::discover(BDynamicArray<BString>& clients)
{
   BFindFiles findFiles;
   bool success = findFiles.scan(CLIENT_ADVERTISEMENT_PATH, "*.*.*.*", BFindFiles::FIND_FILES_WANT_FILES);
   if (!success)
      return false;
      
   for (uint i = 0; i < findFiles.numFiles(); i++)
   {
      const BString& filename = findFiles.getFile(i).filename();
      clients.pushBack(filename);
   }
   
   return true;
}












