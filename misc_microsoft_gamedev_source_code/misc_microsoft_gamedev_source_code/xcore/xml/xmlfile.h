#if 0
// File: xmlfile.h

#pragma once

class BIXMLFile
{
public:
      virtual bool  openReadOnly (long dirID, const BString& filename) = 0;
      virtual bool  openWriteable(long dirID, const BString& filename) = 0;
      virtual bool  close        (void) = 0;
      
      virtual bool  setOffset(__int64 offset) = 0;
      virtual bool  getOffset(unsigned long& offset) const = 0;
            
      virtual bool  read         (void* pBuffer, unsigned long numBytes) = 0;
      virtual bool  write        (const void* pBuffer, unsigned long numBytes) = 0;
   
      virtual bool  getSize    (unsigned long& size) const = 0;
      
      virtual unsigned long      fprintf           (const char *format, ...) = 0;
      virtual long               fputs             (const char *pString) = 0;
};

#endif
