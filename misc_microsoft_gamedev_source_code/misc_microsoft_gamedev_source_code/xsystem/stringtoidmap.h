//=============================================================================
// stringtoidmap.h
//
// Copyright (c) 1999, 2000, 2001 Ensemble Studios
//
// Maps strings to ids.
//=============================================================================

#ifndef _STRINGTOIDMAP_H_
#define _STRINGTOIDMAP_H_

class BChunkReader;
class BChunkWriter;

//=============================================================================
class BStringToIDMap
{
   public:
      BStringToIDMap(void);
      ~BStringToIDMap(void);

      bool                    translate(long id, long &newID);
      bool                    save(BChunkWriter *chunkWriter);
      bool                    load(BChunkReader *chunkReader);
      void                    setReflection(bool flag) {mbReflection = flag;}
   protected:
      void                    allocateIDs(long num);
      void                    add(long id, long newID);

      // These are what derived classes should override.
      virtual long            getIDFromString(const char *str) const=0;
      virtual const char      *getStringFromID(long id) const=0;
      virtual long            getCurrentIDCount(void) const=0;

      long                    mIDCount;
      long                    *mNewIDs;

      bool                    mbReflection;
      static const DWORD      msSaveVersion;
};

//=============================================================================

#endif // _TYPEMAP_H_

//=============================================================================
// eof: stringtoidmap.h
//=============================================================================