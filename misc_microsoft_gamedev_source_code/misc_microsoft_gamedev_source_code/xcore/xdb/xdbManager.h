//============================================================================
//  xdbManager.h
//  Copyright (c) 2006, Ensemble Studios
//============================================================================
#pragma once

#ifdef XBOX
#include "xdb.h"
#include "xexInfo.h"
#include "xstackTrace.h"

class BXDBManager
{
public:
   BXDBManager();
   ~BXDBManager();
   
   bool init(IStreamFactory& streamFactory, long dirID, const BString& basePath);
   bool loadXDB(IStreamFactory& streamFactory, long dirID, const BString& basePath);
   void deinit(void);
                    
   bool getInitialized(void) const { return mInitialized; }
   bool getXDBValid(void) const { return mXDBValid; }
      
   // Call getXDBLookupInfo() to get the look info.
   bool lookupAddress(DWORD address);
   
   // Returns 0 on failure. BXDBManager does not need to be initialized to use this method.
   int beginStackTrace(void);
   int beginStackTrace(_EXCEPTION_POINTERS* p);
   
   int getStackTraceLevels(void) const { return mXStackTrace.getNumLevels(); }
   
   // BXDBManager does not need to be initialized to use this method.
   DWORD getStackTraceAddress(uint level);
   
   // Call getXDBLookupInfo() to get the lookup info.
   bool walkStackTrace(uint level);
   
   const BXDBFileReader::BLookupInfo& getXDBLookupInfo(void) const { return mXDBLookupInfo; }
   
   BXEXInfo& getXEXInfo(void) { return mXEXInfo; }
   DWORD getXEXChecksum(void) const { return mXEXChecksum; }
   BXStackTrace& getStackTrace(void) { return mXStackTrace; } 
   BXDBFileReader& getXDBFileReader(void) { return mXDBFileReader; }
      
private:
   long mDirID;
   BString mBasePath;
   BString mXEXFilename;
   BString mXDBFilename;
   BStream* mpXDBStream;
   DWORD mXEXChecksum;
   
   BXEXInfo mXEXInfo;
   BXStackTrace mXStackTrace;
   
   BXDBFileReader mXDBFileReader;
   BXDBFileReader::BLookupInfo mXDBLookupInfo;
   
   bool mInitialized;
   bool mXDBValid;
};

extern BXDBManager gXDBManager;
#endif // XBOX
