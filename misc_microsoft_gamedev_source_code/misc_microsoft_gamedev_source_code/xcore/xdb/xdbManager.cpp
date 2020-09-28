//============================================================================
//  xdbManager.cpp
//  Copyright (c) 2006, Ensemble Studios
//============================================================================
#include "xcore.h"

#ifdef XBOX
#include "xdbManager.h"
#include "timer.h"

//============================================================================
// Globals
//============================================================================
BXDBManager gXDBManager;

//============================================================================
// BXDBManager::BXDBManager
//============================================================================
BXDBManager::BXDBManager() : 
   mInitialized(false),
   mXDBValid(false),
   mpXDBStream(NULL),
   mXEXChecksum(0),
   mDirID(0)
{
}

//============================================================================
// BXDBManager::~BXDBManager
//============================================================================
BXDBManager::~BXDBManager()
{
}

//============================================================================
// BXDBManager::init
//============================================================================
bool BXDBManager::init(IStreamFactory& streamFactory, long dirID, const BString& basePath)
{
   deinit();
   
   mInitialized = false;
   mXDBValid = false;
   
   mDirID = dirID;
   mBasePath = basePath;
   
   if (!mXEXInfo.getValid())
   {
      trace("BXDBManager::init: mXEXInfo.getValid() failed!");
      return false;
   }

   mXEXFilename = basePath;
   mXEXFilename += mXEXInfo.getModuleName();
   
   BStream* pXEXFileStream = streamFactory.create(dirID, mXEXFilename, (eStreamFlags)(cSFReadable | cSFSeekable));
   if (!pXEXFileStream)
   {
      trace("BXDBManager::init: Can't open XEX: %s", mXEXFilename.getPtr());
      return false;
   }
      
   BXEXChecksum xexChecksum;
   
   bool success = xexChecksum.compute(mXEXChecksum, *pXEXFileStream);
   
   delete pXEXFileStream;
   pXEXFileStream = NULL;
   
   if (!success)
   {
      trace("BXDBManager::init: Can't read from XEX: %s", mXEXFilename.getPtr());
      return false;
   }
   
   trace("BXDBManager::init: Success. XEX file %s, CRC 0x%08X", mXEXFilename.getPtr(), mXEXChecksum);
   
   mInitialized = true;
   
   return true;
}

//============================================================================
// BXDBManager::loadXDB
//============================================================================
bool BXDBManager::loadXDB(IStreamFactory& streamFactory, long dirID, const BString& basePath)
{
   if (!mInitialized)
      return false;
      
   mXDBValid = false;
   
   mXDBFilename = basePath;
   BString modName(mXEXInfo.getModuleName());
   strPathRemoveExtension(modName);
   mXDBFilename += modName;
   mXDBFilename += ".xdb";
   
   mpXDBStream = streamFactory.create(dirID, mXDBFilename, (eStreamFlags)(cSFReadable | cSFSeekable));
   if (!mpXDBStream)
   {
      trace("BXDBManager::init: Can't open XDB: %s", mXDBFilename.getPtr());
      return false;
   }

   if (!mXDBFileReader.open(mpXDBStream))
   {
      trace("BXDBManager::init: Can't read XDB: %s", mXDBFilename.getPtr());
      
      delete mpXDBStream;
      mpXDBStream = NULL;
      
      return false;
   }

   if (mXDBFileReader.getCheckSum() != mXEXChecksum)
   {
      mXDBFileReader.close();
      
      delete mpXDBStream;
      mpXDBStream = NULL;

      trace("BXDBManager::init: XDB file's checksum doesn't match the XEX file's checksum!");
      return false;
   }      

   trace("BXDBManager::init: Success. XDB file %s", mXDBFilename.getPtr());

   mXDBValid = true;
   return true;
}

//============================================================================
// BXDBManager::deinit
//============================================================================
void BXDBManager::deinit(void)
{
   mXDBFileReader.close();
   
   if (mpXDBStream)
   {
      delete mpXDBStream;
      mpXDBStream = NULL;
   }
   
   mDirID = 0;
   mXEXChecksum = 0;
}

//============================================================================
// BXDBManager::lookupAddress
//============================================================================
bool BXDBManager::lookupAddress(DWORD address)
{
   if ((!mInitialized) || (!mXDBValid))
      return false;
   
   address -= mXEXInfo.getBaseAddress();
   address += mXDBFileReader.getBaseAddress();
      
   return mXDBFileReader.lookup(address, mXDBLookupInfo);
}

//============================================================================
// BXDBManager::beginStackTrace
//============================================================================
int BXDBManager::beginStackTrace(void)
{
   if (!mXStackTrace.capture())
      return 0;
      
   return mXStackTrace.getNumLevels();      
}

//============================================================================
// BXDBManager::beginStackTrace
//============================================================================
int BXDBManager::beginStackTrace(_EXCEPTION_POINTERS* p)
{
   if (!mXStackTrace.capture(p))
      return 0;

   return mXStackTrace.getNumLevels();      
}

//============================================================================
// BXDBManager::getStackTraceAddress
//============================================================================
DWORD BXDBManager::getStackTraceAddress(uint level)
{
   return mXStackTrace.getLevelAddr(level);
}

//============================================================================
// BXDBManager::walkStackTrace
//============================================================================
bool BXDBManager::walkStackTrace(uint level)
{
   if (!mInitialized)
      return false;
      
   BDEBUG_ASSERT(level < mXStackTrace.getNumLevels());
   
   return lookupAddress(mXStackTrace.getLevelAddr(level));
}
#else
uint gXDBManagerDummy;
#endif // XBOX