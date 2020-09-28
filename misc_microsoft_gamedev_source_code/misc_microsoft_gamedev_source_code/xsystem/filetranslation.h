//============================================================================
//
//  FileTranslation.h
//
//  Copyright 2002 Ensemble Studios
//
//============================================================================
#pragma once

class BFileStorage;

class BFileTranslation
{
   public:
      BFileTranslation();
      virtual ~BFileTranslation();

      virtual bool   close          ();
      virtual bool   openReadOnly   (const BString& path, unsigned long flags);
      virtual bool   openWriteable  (const BString& path, unsigned long flags, BYTE compressionLevel);
      virtual bool   openReadWrite  (const BString& path, unsigned long flags);
      
      virtual bool   setOffset      (unsigned __int64 offset, unsigned long fromPosition);
      virtual bool   getOffset      (unsigned long& offset, bool realPositionIfCompressed = false) const;
      virtual bool   getOffset      (unsigned __int64& offset, bool realPositionIfCompressed = false) const;
      virtual bool   getSize        (unsigned long& size, bool realSizeIfCompressed = false) const;
      virtual bool   getSize        (unsigned __int64& size, bool realSizeIfCompressed = false) const;
      bool           getTime        (BFileTime& time) const;
      
      virtual unsigned long readEx         (void* pBuffer, unsigned long numBytes);
      virtual bool   read           (void* pBuffer, unsigned long numBytes);
      virtual bool   write          (const void* pBuffer, unsigned long numBytes);
      virtual void   flush          ();

      virtual BYTE*  getAndIncrementPtr(DWORD dwbytes);
      
      bool            isOpen       () const;
      unsigned long   getFlags     () const;
      const BString*  getPath      () const;
      unsigned long   getError     () const;
      HANDLE          getFileHandle() const;
      void            setOwnerThread(DWORD threadID);

   protected:
      BFileStorage*   mpStorage;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BFileTranslationCompressed : public BFileTranslation
{
   public:
      BFileTranslationCompressed();
      virtual ~BFileTranslationCompressed();
      
      virtual bool   close          ();
      virtual bool   openReadOnly   (const BString& path, unsigned long flags);
      virtual bool   openWriteable  (const BString& path, unsigned long flags, BYTE compressionLevel);
      virtual bool   openReadWrite  (const BString& path, unsigned long flags);

      virtual bool   setOffset      (unsigned __int64 offset, unsigned long fromPosition);
      virtual bool   getOffset      (unsigned long& offset, bool realPositionIfCompressed = false) const;
      virtual bool   getOffset      (unsigned __int64& offset, bool realPositionIfCompressed = false) const;
      virtual bool   getSize        (unsigned long& size, bool realSizeIfCompressed = false) const;
      virtual bool   getSize        (unsigned __int64& size, bool realSizeIfCompressed = false) const;

      virtual unsigned long readEx         (void* pBuffer, unsigned long numBytes);
      virtual bool   read           (void* pBuffer, unsigned long numBytes);
      virtual bool   write          (const void* pBuffer, unsigned long numBytes);

      virtual BYTE*  getAndIncrementPtr(DWORD dwbytes){dwbytes; return(FALSE);}
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BFileTranslationUnCompressed : public BFileTranslation
{
   public:
      BFileTranslationUnCompressed(){}
      virtual ~BFileTranslationUnCompressed(){}
};
