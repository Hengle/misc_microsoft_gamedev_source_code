//============================================================================
//  File: ContentReader.h
//
//  Copyright (c) 2007, Ensemble Studios
//============================================================================
#pragma once

#ifdef XBOX

#include "buffercache.h"

class BContentFile;

//============================================================================
// 
//============================================================================
class BContentReader
{
   public:
      enum { cDefaultCacheSize = 8192, };

      BContentReader(BContentFile *file);
      ~BContentReader();

      bool close(void);
      BOOL setFilePointer(uint64 filepointer);

      BOOL readBytes(void * p, uint n, uint * numActual);
      uint bytesAvailable() { return mpCache->bytesAvailToRead(); }
      bool atEOF();


      // Helper methods
      BOOL readInt(int* data);
      BOOL readUint(uint* data);
      BOOL readBool(bool* data);
      BOOL readBString(BString& string);
      BContentFile* getFile() { return mpContentFile;}

   protected:
      BOOL fillCache();

      BBufferCache*  mpCache;          // buffered data on the read.
      int64          mFileOfs;
      BContentFile   *mpContentFile;
}; // class BContentWriter

#endif // XBOX