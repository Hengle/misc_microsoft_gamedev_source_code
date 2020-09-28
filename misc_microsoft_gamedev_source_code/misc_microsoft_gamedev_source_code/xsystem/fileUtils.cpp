// File: fileUtils.cpp
#include "xsystem.h"
#include "fileUtils.h"
#include "file.h"

//#ifndef XBOX
//#error This module is currently Xbox only.
//#endif

static const BCHAR_T*     gpGamePath = B("game:\\");
static const BCHAR_T*     gpCachePath = B("cache:\\");

#ifdef XBOX
#include "xfs.h"

//==============================================================================
// GetModuleFileName
//==============================================================================
DWORD GetModuleFileName(HMODULE hModule, LPTSTR lpFilename, DWORD nSize)
{
   // rg [8/23/06] - Total hack.
   strcpy_s(lpFilename, nSize, BFileUtils::getXboxGamePath());

   // FIXME: Return actual executable name.
   strcat_s(lpFilename, nSize, 
#ifdef BUILD_DEBUG
      "xgameD.xex"
#elif defined(BUILD_PLAYTEST)
      "xgameP.xex"   
#else   
      "xgameF.xex"
#endif   
      );

   return strlen(lpFilename);
}

//============================================================================
// BFileUtils::loadFile
//============================================================================
bool BFileUtils::loadFile(long dirID, const BString& filename, void** ppFileData, unsigned long* pFileSize)
{
   BASSERT(ppFileData);
   if(pFileSize)
      *pFileSize=0;

   BFile file;
   if(!file.openReadOnly(dirID, filename))
      return false;
   DWORD size;
   if(!file.getSize(size))
      return false;

   void* pData = BAlignedAlloc::Malloc(size);

   if(!file.read(pData, size))
   {
      BAlignedAlloc::Free(pData);
      return false;
   }

   *ppFileData = pData;
   if(pFileSize)
      *pFileSize=size;

   return true;
}

//============================================================================
// BFileUtils::unloadFile
//============================================================================
void BFileUtils::unloadFile(void* pFileData)
{
   BASSERT(pFileData!=NULL);
   BAlignedAlloc::Free(pFileData);
}

//============================================================================
// BFileUtils::loadFilePhysicalMemory
//============================================================================
bool BFileUtils::loadFilePhysicalMemory(long dirID, const BString& filename, void** ppFileData, unsigned long* pFileSize, DWORD dataAlignment)
{
   BASSERT(ppFileData);
   if(pFileSize)
      *pFileSize=0;

   BFile file;
   if(!file.openReadOnly(dirID, filename))
      return false;

   DWORD size;
   if(!file.getSize(size))
      return false;

   void* pData = XPhysicalAlloc(size, MAXULONG_PTR, dataAlignment, PAGE_READWRITE);
   if(!pData)
      return false;

   if(!file.read(pData, size))
   {
      XPhysicalFree(pData);
      return false;
   }

   *ppFileData=pData;
   if(pFileSize)
      *pFileSize=size;

   return true;
}

//============================================================================
// BFileUtils::unloadFilePhysicalMemory
//============================================================================
void BFileUtils::unloadFilePhysicalMemory(void* pFileData)
{
   BASSERT(pFileData!=NULL);
   XPhysicalFree(pFileData);
}

//============================================================================
// BFileUtils::getXboxGamePath
// This directory cannot be written to!
//============================================================================
const BCHAR_T* BFileUtils::getXboxGamePath()
{
   return gpGamePath;
}

//============================================================================
// BFileUtils::isXboxGamePath
// true if the path is to a directory on the game partition (which is unwritable).
//============================================================================
bool BFileUtils::isXboxGamePath(const BString& path)
{
   const char* pGamePrefix = getXboxGamePath();
   if (strnicmp(path.getPtr(), pGamePrefix, strlen(pGamePrefix)) == 0)
      return true;
   return false;
}

//============================================================================
// BFileUtils::getXboxGamePath
// Directory on cache partition.
//============================================================================
const BCHAR_T* BFileUtils::getXboxTempPath(void)
{
   if(gXFS.isActive())
      return getXboxGamePath();
   else
   {
      static bool checkedDVD=false;
      static bool fromDVD=true;
      if(!checkedDVD)
      {
         HANDLE handle=CreateFile("game:\\dvd.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
         if(handle!=INVALID_HANDLE_VALUE)
         {
            fromDVD=false;
            CloseHandle(handle);
         }
         checkedDVD=true;
      }
      if(fromDVD)
         return gpCachePath;
      else
         return getXboxGamePath();
   }
}
#endif // XBOX


