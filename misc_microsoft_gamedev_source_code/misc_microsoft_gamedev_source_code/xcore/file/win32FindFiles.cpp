//============================================================================
//
// File: win32FindFiles.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "win32FindFiles.h"
#include "win32File.h"
 
BFileDesc::BFileDesc(const char* pBasePathname, const char* pRelPathname, const char* pFilename, const char* pShortFilename, const WIN32_FIND_DATA& findData, uint64 lcn)
{
   mBasePathname = pBasePathname ? pBasePathname : B("");
   mRelPathname = pRelPathname ? pRelPathname : B("");
   mFilename = pFilename ? pFilename : findData.cFileName;
   mShortFilename = pShortFilename ? pShortFilename : findData.cAlternateFileName;
   mAttributes = findData.dwFileAttributes;
   mSize = createUInt64(findData.nFileSizeLow, findData.nFileSizeHigh);
   mCreateTime = fileTimeToUInt64(findData.ftCreationTime);
   mAccessTime = fileTimeToUInt64(findData.ftLastAccessTime);
   mWriteTime = fileTimeToUInt64(findData.ftLastWriteTime);
   mLCN = lcn;
}

BFileDesc::BFileDesc(const char* pBasePathname, const char* pRelPathname, const char* pFilename, const char* pShortFilename, DWORD attributes, uint64 size, uint64 createTime, uint64 accessTime, uint64 writeTime, uint64 lcn) :
   mBasePathname(pBasePathname ? pBasePathname : B("")),
   mRelPathname(pRelPathname ? pRelPathname : B("")),
   mFilename(pFilename ? pFilename : ""),
   mShortFilename(pShortFilename ? pShortFilename : B("")),
   mAttributes(attributes),
   mSize(size),
   mCreateTime(createTime),
   mAccessTime(accessTime),
   mWriteTime(writeTime),
   mLCN(lcn)
{
}

void BFileDesc::clear(void)
{
   mBasePathname.empty();
   mRelPathname.empty();
   mFilename.empty();
   mShortFilename.empty();
   mAttributes = 0;
   mSize = 0;
   mCreateTime = 0;
   mAccessTime = 0;
   mWriteTime = 0;
   mLCN = 0;
}

BString BFileDesc::fullFilename(void) const
{
   BString file(mBasePathname);
   strPathAddBackSlash(file, true);
   file += mRelPathname;
   strPathAddBackSlash(file, true);
   file += mFilename;
   return file;
}

BString BFileDesc::relFilename(bool convertEmptyPathToDot) const
{
   BString file(mRelPathname);
   strPathAddBackSlash(file, convertEmptyPathToDot);
   file += mFilename;
   return file;
}

BString BFileDesc::relShortFilename(void) const
{
   BString file(mRelPathname);
   strPathAddBackSlash(file, true);
   if (mShortFilename.isEmpty())
      file += mFilename;
   else
      file += mShortFilename;
   return file;
}

BFindFiles::BFindFiles() :
   mSuccess(false),
   mpLowLevelFileIO(ILowLevelFileIO::getDefault())
{
}

BFindFiles::BFindFiles(
   const BString& path, 
   const BString& mask, 
   int flags) :
   mpLowLevelFileIO(ILowLevelFileIO::getDefault())
{
   clear();
   mSuccess = scan(path, mask, flags);
}

void BFindFiles::clear(void)
{
   mSuccess = false;
   mFiles.clear();
}

bool BFindFiles::scan(
   const BString& path, 
   const BString& mask, 
   int flags)
{
   mSuccess = recursiveScan(path, B(""), mask, flags);   
   return mSuccess;
}

// true on success
bool BFindFiles::recursiveScan(
   const BString& base, 
   const BString& rel,
   const BString& mask, 
   int flags)
{
   BString basePath(base);
   strPathAddBackSlash(basePath, true);

   BString relPath(rel);
   strPathAddBackSlash(relPath, false);

   const BString findFilePath = basePath + relPath + mask;

   const BString findDirPath(basePath + relPath + B("*"));

   //-- Search Vars
   WIN32_FIND_DATA   findData;
   HANDLE            hFind;
   bool              bDone;
   BString        foundPath;

   const bool bWantDirs    = (flags & FIND_FILES_WANT_DIRS) ? true : false;
   const bool bWantFiles   = (flags & FIND_FILES_WANT_FILES) ? true : false;

   //-- Search the current directory for files that match.
   hFind = mpLowLevelFileIO->findFirstFile(findFilePath.getPtr(), &findData); 
   if (hFind == INVALID_HANDLE_VALUE)
   {
      HRESULT hres = GetLastError();
      if ((hres != NO_ERROR) && (hres != ERROR_FILE_NOT_FOUND))
         return false;
   }
   else
   {
      //-- Process each file
      bDone = FALSE;
      while (!bDone)
      {
         //-- ignore the types we don't care about.
         const bool bDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

         if ((bDir && !bWantDirs) || (!bDir && !bWantFiles))
         {
            bDone = !mpLowLevelFileIO->findNextFile(hFind, &findData);
            continue;
         }

         //-- Ignore "." and ".."
         bool skip = false;
         if (strcmp(findData.cFileName, ".") == 0) skip = true;
         if (strcmp(findData.cFileName, "..") == 0) skip = true;
         if (skip)
         {
            bDone = !mpLowLevelFileIO->findNextFile(hFind, &findData);
            continue;
         }	

         //-- We found a file that we want.  Format it.
         foundPath = basePath + relPath + findData.cFileName;

         uint64 lcn = 0;

         if ((!bDir) && (flags & FIND_FILES_WANT_LCNS))
         {
            BWin32File file;
            if (file.open(foundPath))
            {
               if (!file.getFirstLogicalClusterNumber(lcn))
                  lcn = 0;
            }
         }

         if (bDir)
         {
            findData.nFileSizeLow = 0;
            findData.nFileSizeHigh = 0;
         }

         //printf("%i %s %s %i\n", mFiles.size(), path.c_str(), findData.cFileName, findData.nFileSizeLow);

         {
            BFileDesc desc(base.getPtr(), rel.getPtr(), findData.cFileName, findData.cAlternateFileName, findData, lcn);
            mFiles.pushBack(desc);
         }

         bDone = !mpLowLevelFileIO->findNextFile(hFind, &findData);
      }
      mpLowLevelFileIO->findClose(hFind);
   }

   //-- Should we keep going or not?
   if(!(flags & FIND_FILES_RECURSE_SUBDIRS))
      return(true);

   //-- Search the current directory for files that match.
   hFind = mpLowLevelFileIO->findFirstFile(findDirPath.getPtr(), &findData); 
   if (hFind == INVALID_HANDLE_VALUE)
   {
      if (GetLastError() != NO_ERROR)
         return false;
   }
   else
   {
      //-- Process each file
      bDone = FALSE;
      while (!bDone)
      {
         //-- skip all but dirs.
         if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
         {
            bDone = !mpLowLevelFileIO->findNextFile(hFind, &findData);
            continue;
         }

         //-- Ignore "." and ".."
         bool skip = false;
         if (strcmp(findData.cFileName, ".") == 0) skip = true;
         if (strcmp(findData.cFileName, "..") == 0) skip = true;
         if (skip)
         {
            bDone = !mpLowLevelFileIO->findNextFile(hFind, &findData);
            continue;
         }	

         recursiveScan(base, relPath + findData.cFileName, mask, flags);
         bDone = !mpLowLevelFileIO->findNextFile(hFind, &findData);
      }
      mpLowLevelFileIO->findClose(hFind);
   }

   return(true);
}

