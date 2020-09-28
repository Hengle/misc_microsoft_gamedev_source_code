//============================================================================
//
// File: win32FindFiles.h
// Copyright (c) 2005-2006, Ensemble Studios
//
// Ported from Wrench
//
//============================================================================
#pragma once
#include "lowLevelFileIO.h"

class BFileDesc
{
   static uint64 createUInt64(DWORD low, DWORD high) { return (static_cast<uint64>(high) << 32) | low; }
      
public:
   BFileDesc()
   {
      clear();
   }
   
   BFileDesc(const char* pBasePathname, const char* pRelPathname, const char* pFilename, const char* pShortFilename, const WIN32_FIND_DATA& findData, uint64 lcn = 0);
   
   BFileDesc(const char* pBasePathname, const char* pRelPathname, const char* pFilename, const char* pShortFilename, DWORD attributes, uint64 size, uint64 createTime, uint64 accessTime, uint64 writeTime, uint64 lcn = 0);

   BFileDesc(const BFileDesc& source) { *this=source; }

   void clear(void);
      
   BString fullFilename(void) const;
      
   BString relFilename(bool convertEmptyPathToDot = true) const;
   
   BString relShortFilename(void) const;
   
   const BString& basePathname(void) const { return mBasePathname; }
   const BString& relPathname(void) const { return mRelPathname; }
   const BString& filename(void) const { return mFilename; }
   const BString& shortFilename(void) const { return mShortFilename; }
   DWORD attributes(void) const { return mAttributes; }
   uint64 size(void) const { return mSize; }
   uint64 createTime(void) const { return mCreateTime; }
   uint64 accessTime(void) const { return mAccessTime; }
   uint64 writeTime(void) const { return mWriteTime; }
   uint64 lcn(void) const { return mLCN; }
   
   void setBasePathname(const char* pBasePathname) { mBasePathname = BString(pBasePathname); }
   void setRelPathname(const char* pRelPathname) { mRelPathname = BString(pRelPathname); }
   void setFilename(const char* pFilename) { mFilename = BString(pFilename); }
   void setShortFilename(const char* pShortFilename) { mShortFilename = BString(pShortFilename); }
   void setAttributes(DWORD attributes) { mAttributes = attributes; }
   void setSize(uint64 size) { mSize = size; }
   void setCreateTime(uint64 createTime) { mCreateTime = createTime; }
   void setAccessTime(uint64 accessTime) { mAccessTime = accessTime; }
   void setWriteTime(uint64 writeTime) { mWriteTime = writeTime; }
   void setLCN(uint64 lcn) { mLCN = lcn; }
   
   bool isDir(void) const { return 0 != (attributes() & FILE_ATTRIBUTE_DIRECTORY); }
   bool isFile(void) const { return 0 == (attributes() & FILE_ATTRIBUTE_DIRECTORY); }

   BFileDesc& operator= (const BFileDesc& source)
   {
      if (this == &source)
         return *this;

      clear();

      mBasePathname = source.mBasePathname;
      mRelPathname = source.mRelPathname;
      mFilename = source.mFilename;
      mShortFilename = source.mShortFilename;
      mAttributes = source.mAttributes;
      mSize = source.mSize;
      mCreateTime = source.mCreateTime;
      mAccessTime = source.mAccessTime;
      mWriteTime = source.mWriteTime;
      mLCN = source.mLCN;

      return *this;
   }

   friend bool operator== (const BFileDesc& lhs, const BFileDesc& rhs)
   {
      return 
         (lhs.mBasePathname == rhs.mBasePathname)    && 
         (lhs.mRelPathname == rhs.mRelPathname)    && 
         (lhs.mFilename    == rhs.mFilename)    &&
         (lhs.mAttributes  == rhs.mAttributes)  &&
         (lhs.mSize        == rhs.mSize)        &&
         (lhs.mCreateTime  == rhs.mCreateTime)  &&
         (lhs.mAccessTime  == rhs.mAccessTime)  &&
         (lhs.mWriteTime   == rhs.mWriteTime)   &&
         (lhs.mLCN         == rhs.mLCN);
   }

   static bool sortByLCN(const BFileDesc& lhs, const BFileDesc& rhs)
   {
      if (lhs.mLCN < rhs.mLCN)
         return true;
      else if (lhs.mLCN == rhs.mLCN)
         return lhs.mFilename < rhs.mFilename;
         
      return false;
   }
   
   static uint64 fileTimeToUInt64(const FILETIME& fileTime)
   {
      return createUInt64(fileTime.dwLowDateTime, fileTime.dwHighDateTime);
   }

   static FILETIME uint64ToFileTime(uint64 time)
   {
      FILETIME ret;
      ret.dwLowDateTime = static_cast<DWORD>(time & 0xFFFFFFFF);
      ret.dwHighDateTime = static_cast<DWORD>(time >> 32);
      return ret;
   }

private:
   BString mBasePathname;
   BString mRelPathname;
   BString mFilename;
   BString mShortFilename;
   DWORD mAttributes;
   uint64 mSize;
   uint64 mCreateTime;
   uint64 mAccessTime;
   uint64 mWriteTime;
   uint64 mLCN;
};

typedef BDynamicArray<BFileDesc> BFileDescArray;

class BFindFiles
{
public:
   enum EFlags
   {  
      FIND_FILES_WANT_DIRS            = 0x00000001,
      FIND_FILES_WANT_FILES           = 0x00000002,
      FIND_FILES_RECURSE_SUBDIRS      = 0x00000004,
      FIND_FILES_WANT_LCNS            = 0x00000008
   };

   BFindFiles();

   BFindFiles(
      const BString& path, 
      const BString& mask, 
      int flags = FIND_FILES_WANT_FILES | FIND_FILES_RECURSE_SUBDIRS);
   
   bool success(void) const { return mSuccess; }

   void clear(void);

   bool scan(
      const BString& path, 
      const BString& mask, 
      int flags = FIND_FILES_WANT_FILES | FIND_FILES_RECURSE_SUBDIRS);

   uint numFiles(void) const { return mFiles.size(); }
   const BFileDesc& getFile(uint i) const { return mFiles[debugRangeCheck(i, numFiles())]; }
   
   uint size(void) const { return mFiles.size(); }
   bool empty(void) const { return 0 == size(); }
   const BFileDesc& operator[](uint i) const { return mFiles[debugRangeCheck(i, numFiles())]; }
   
   const BFileDescArray& getFileDescVec(void) const { return mFiles; }
         BFileDescArray& getFileDescVec(void)       { return mFiles; }
         
   ILowLevelFileIO*        getLowLevelFileIO(void) const { return mpLowLevelFileIO; }
   void                    setLowLevelFileIO(ILowLevelFileIO* pLowLevelFileIO) { mpLowLevelFileIO = pLowLevelFileIO ? pLowLevelFileIO : ILowLevelFileIO::getDefault(); }         

private:
   bool mSuccess;
   BFileDescArray mFiles;
   
   ILowLevelFileIO* mpLowLevelFileIO;

   // true on success
   bool recursiveScan(
      const BString& base, 
      const BString& rel,
      const BString& mask, 
      int flags);
};
