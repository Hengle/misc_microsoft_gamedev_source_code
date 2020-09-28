//==============================================================================
//
// File: win32FileUtils.cpp
//
// Copyright (c) 2002-2007, Ensemble Studios
//
//==============================================================================
#pragma once
#include "resource\ecfUtils.h"

class BSHA1;

class BWin32FileUtils
{
public:
   static bool computeFileDigest(const char* pFilename, uint64* pFileSize = NULL, BSHA1* pSha1 = NULL, DWORD* pCRC32 = NULL, DWORD* pAdler32 = NULL);
   static bool getFileSize(const char* pFilename, uint64& fileSize);
   static bool doesFileExist(const char* pFilename);
   static bool doesDirectoryExist(const char* pDirectory);
   static bool readFileData(const char* pFilename, BByteArray& data);
   static bool writeFileData(const char* pFilename, const BByteArray& data, bool retry = false);
   static bool writeFileData(const char* pDstFilename, BECFFileBuilder& ecfBuilder, uint64* pOutputFileSize = NULL, bool retry = false);
   static bool readStringFile(const char* pFilename, BDynamicArray<BString>& stringArray);
   static void createDirectories(const char* pPath, bool removeFilename = true);
   static bool copyFile(const char* pSrcFilename, const char* pDstFilename, bool createDestinationPath = false);
};