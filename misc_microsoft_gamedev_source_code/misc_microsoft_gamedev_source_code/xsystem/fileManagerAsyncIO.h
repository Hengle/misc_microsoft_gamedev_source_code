// File: fileManagerAsyncIO.h
#pragma once

#include "asyncFileManager.h"

class BFileManagerAsyncFileIO : public BAsyncFileManager::BAsyncFileIOInterface
{
public:
   BFileManagerAsyncFileIO();
   virtual ~BFileManagerAsyncFileIO();
   virtual bool readFile(int dirID, const char* pFilename, void** pData, uint* pDataLen);
   virtual bool writeFile(int dirID, const char* pFilename, const void* pData, uint pDataLen);
};

// This object interfaces gFileManager with gAsyncFileManager.
extern BFileManagerAsyncFileIO gFileManagerAsyncIO;
