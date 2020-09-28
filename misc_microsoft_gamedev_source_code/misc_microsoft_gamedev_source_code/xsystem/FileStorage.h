//============================================================================
//
//  FileStorage.h
//
//  Copyright 2002-2006 Ensemble Studios
//
//============================================================================
#pragma once

class BArchiveFileEntry;

__declspec(selectany) extern const char gCompressedFileHeader[] = {'l', '3', '3', 't'};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BFileStorage
{
   public:
      BFileStorage();
      virtual ~BFileStorage(){close();}

      virtual bool close();

      virtual bool      openReadOnly   (const BString& path, unsigned long flags){path; flags; return(false);}
      virtual bool      openWriteable  (const BString& path, unsigned long flags){path; flags; return(false);}
      virtual bool      openReadWrite  (const BString& path, unsigned long flags){path; flags; return(false);}

      virtual bool      setOffset      (__int64 offset, unsigned long fromPosition){offset; fromPosition; return(false);}
      virtual bool      getOffset      (unsigned long& offset) const {offset; return(false);}
      virtual bool      getOffset      (unsigned __int64& offset) const {offset; return(false);}
   
      virtual bool      getSize        (unsigned long& size) const {size; return(false);}
      virtual bool      getSize        (unsigned __int64& size) const {size; return(false);}
      virtual bool      getTime        (BFileTime& time) const {time; return(false);}

      virtual unsigned long    readEx         (void* pBuffer, unsigned long numBytes){pBuffer; numBytes; return(0);}
      virtual bool      read           (void* pBuffer, unsigned long numBytes){pBuffer; numBytes; return(false);}
      virtual bool      write          (const void* pBuffer, unsigned long numBytes){pBuffer; numBytes; return(false);}
      virtual void      flush          () {};

      virtual BYTE*  getAndIncrementPtr(DWORD dwbytes){dwbytes; return(NULL);}

      virtual bool      isOpen         () const;
      unsigned long     getError       () const {return(mError);}
      const BString&    getPath          () const {return(mPath);}
      unsigned long     getFlags       () const {return(mFlags);}
      HANDLE            getFileHandle  () const {return(mhFile);}

      void              setFlags       (unsigned long flags){mFlags=flags;}
      virtual void      setOwnerThread (DWORD threadID){}

   protected:
      //-- Data
      BString						mPath;
      unsigned long				mFlags;
      HANDLE						mhFile;
      mutable unsigned long	mError;

      
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BFileStorageWin32 : public BFileStorage
{
   public:
      BFileStorageWin32();
      virtual ~BFileStorageWin32(){close();}

      virtual bool close();

      virtual bool openReadOnly  (const BString& path, unsigned long flags);
      virtual bool openWriteable (const BString& path, unsigned long flags);
      virtual bool openReadWrite (const BString& path, unsigned long flags);
      
      virtual bool setOffset     (__int64 offset, unsigned long fromPosition);
      virtual bool getOffset     (unsigned long& offset) const;
      virtual bool getOffset     (unsigned __int64& offset) const;

      virtual bool getSize       (unsigned long& size) const;
      virtual bool getSize       (unsigned __int64& size) const;
      virtual bool getTime       (BFileTime& time) const;

      virtual unsigned long readEx      (void* pBuffer, unsigned long numBytes);
      virtual bool   read           (void* pBuffer, unsigned long numBytes);
      virtual bool   write          (const void* pBuffer, unsigned long numBytes);
      virtual void   flush          ();
      virtual void   setOwnerThread (DWORD threadID);

   protected:

      // Cached writing of files
      bool         createWriteBuffer();
      bool         flushWriteBuffer();
      void         releaseWriteBuffer();
      BYTE*        mpWriteBuffer;
      long         mWriteBufferOffset;
};

#if 0
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BFileStorageArchive : public BFileStorage
{
   public:
      BFileStorageArchive();
      virtual ~BFileStorageArchive();
      
      virtual bool openReadOnly   (const BString& path, unsigned long flags);

      virtual bool setOffset      (__int64 offset, unsigned long fromPosition);
      virtual bool getOffset      (unsigned long& offset) const;
      virtual bool getOffset      (unsigned __int64& offset) const;

      virtual bool getSize        (unsigned long& size) const;
      virtual bool getSize        (unsigned __int64& size) const;
      virtual bool getTime        (BFileTime& time) const;

      virtual unsigned long readEx       (void* pBuffer, unsigned long numBytes);
      virtual bool   read         (void* pBuffer, unsigned long numBytes);

      virtual bool   isOpen       () const;

   protected:
      BHandle              mhArchiveHandle;
      BArchiveFileEntry*   mpEntry;
      unsigned __int64              mCurrentOffset;
};
#endif
