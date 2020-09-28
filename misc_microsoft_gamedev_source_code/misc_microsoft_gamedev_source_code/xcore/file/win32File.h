//==============================================================================
//
// File: win32File.h
//
// Copyright (c) 2002-2007, Ensemble Studios
// 
//==============================================================================
#pragma once
#include "lowLevelFileIO.h"

//==============================================================================
// BWin32File
// Named BWin32File, not BFile, to prevents conflicts with Age/Wrench's BFile 
// class.
//==============================================================================
class BWin32File
{
   public:
      // Open/create flags
      enum
      {
         cWriteAccess      =  1,
         cRandomAccess     =  2,
         cSequentialAccess =  4,
         
         cAppend           =  8,
         
         // The create flags are mutually exclusive
         cCreateIfNoExist  = 16,
         cCreateAlways     = 32,
      };

      // Seek flags
      enum
      {
         cSeekBegin        = FILE_BEGIN,
         cSeekCurrent      = FILE_CURRENT,
         cSeekEnd          = FILE_END,
      };

                              BWin32File();
                              ~BWin32File();

      bool                    open(const char* path, DWORD flags = 0);
      
      // Takes ownership.
      bool                    open(HANDLE hFile);

      //bool                    openWriteable(long dirID, const char* pFilename, uint flags) { dirID; flags; return open(pFilename, cWriteAccess | cCreateIfNoExist); };

      bool                    create(const char* path, DWORD flags = 0);
      bool                    close();
      void                    flush();

      DWORD                   read(void* buffer, DWORD bytes);

      DWORD                   write(const void* pBuffer, uint numBytes);
      uint                    writeEx(const void* pBuffer, uint numBytes) { return write(pBuffer, numBytes); }

      DWORD                   writeString(const char* buffer);

      uint                    fprintf(const char* format, ...);
      
      int64                   seek(long seekType, int64 bytes);

      // Returns UINT64_MAX on error.
      uint64                  getSize() const;
      bool                    getSize(uint64& size) const;
      
      // false is returned if the filesize is too big for a uint.
      bool                    getSize(uint& size) const;
      
      bool                    getFileTime(FILETIME* pCreationTime, FILETIME* pLastAccessTime, FILETIME* pLastWriteTime) const;
      bool                    setFileTime(const FILETIME* pCreationTime, const FILETIME* pLastAccessTime, const FILETIME* pLastWriteTime);
      uint64                  getOffset() const;
      bool                    getOffset(uint64& offset) const;
      bool                    isNewer(BWin32File* pFile) const;
      bool                    isOpen() const {return (mFileHandle != INVALID_HANDLE_VALUE);}

      HANDLE                  getHandle(void) const { return mFileHandle; }
      
      bool                    getFirstLogicalClusterNumber(uint64& lcn) const;
      
      bool                    extendFile(uint64 newSize);
      
      // Reads entire file into array. File must be <2GB.
      bool                    readArray(BByteArray& buf);
      
      // Writes array's contents to file at current position.
      bool                    writeArray(const BByteArray& buf);
      
      void                    setOwnerThread(DWORD threadID);
      
      ILowLevelFileIO*        getLowLevelFileIO(void) const { return mpLowLevelFileIO; }
      void                    setLowLevelFileIO(ILowLevelFileIO* pLowLevelFileIO) { close(); mpLowLevelFileIO = pLowLevelFileIO ? pLowLevelFileIO : ILowLevelFileIO::getDefault(); }
      
      static bool             getFullPathName(BFixedStringMaxPath& path, ILowLevelFileIO* pLowLevelFileIO = NULL);
      
   protected:
      HANDLE                  mFileHandle;
      ILowLevelFileIO*        mpLowLevelFileIO;
      
   private:
      BWin32File(const BWin32File& a);
      BWin32File& operator= (const BWin32File& rhs);      
};
