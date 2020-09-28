// File: fileManagerAsyncIO.cpp
#include "xsystem.h"
#include "fileManagerAsyncIO.h"
#include "memory\alignedAlloc.h"

// rg [1/24/05] - Temporary, we need to rewrite this system.

BFileManagerAsyncFileIO gFileManagerAsyncIO;

BFileManagerAsyncFileIO::BFileManagerAsyncFileIO()
{
}

BFileManagerAsyncFileIO::~BFileManagerAsyncFileIO()
{
}

bool BFileManagerAsyncFileIO::readFile(int dirID, const char* pFilename, void** pData, uint* pDataLen)
{
   BFile file;
   if (!file.openReadOnly(dirID, BString(pFilename)))
      return false;

   unsigned long fileSize = 0;
   if (!file.getSize(fileSize))
      return false;

   void* pBuf = BAlignedAlloc::Malloc(fileSize);
   if (!file.read(pBuf, fileSize))
   {
      BAlignedAlloc::Free(pBuf);
      return false;
   }

   file.close();

   *pData = pBuf;
   *pDataLen = fileSize;

   return true;
}

bool BFileManagerAsyncFileIO::writeFile(int dirID, const char* pFilename, const void* pData, uint dataLen)
{
   BFile file;
   if (!file.openWriteable(dirID, BString(pFilename), BFILE_OPEN_OVERWRITE))
      return false;

   if ((pData) && (dataLen))
   {
      if (!file.write(pData, dataLen))
         return false;
   }      

   if (!file.close())
      return false;

   return true;
}
