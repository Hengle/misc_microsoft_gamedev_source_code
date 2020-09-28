//==============================================================================
// SyncManager.cpp
//
// Copyright (c) Ensemble Studios, 2000-2008
//==============================================================================

// Includes
#include "common.h"
#include "usermanager.h"
#include "user.h"
#include "database.h"
#include "game.h"
#include "world.h"
#include "gamesettings.h"
#include "syncmanager.h" 
#include "chunker.h"
#include "commlog.h"
#include "configsgame.h"
#include "mpcommheaders.h"
//#include "multiplayer.h"
#include "LiveSystem.h"
#include "recordgame.h"
#include "statsManager.h"
#include "syncmacros.h"
#include "syncsymboltable.h"
#include "xfs.h"
#include "memoryPacker.h"
#include "threading\workDistributor.h"
#include "gamemode.h"

#ifdef USE_BUILD_INFO
#include "build_info.inc"
#endif

// xsystem
#include "econfigenum.h"
#include "FileManager.h"

// xcore
#include "stream\bufferStream.h"
#include "threading\setThreadName.h"

// xrender
#include "debugText.h"

// xgranny
#include "grannyinstance.h"

const uint cSyncUpdateValueGrowSize = 4;
const uint cSyncUpdateStringGrowSize = 4;

IMPLEMENT_FREELIST(BChecksumHistory, 4, &gSyncHeap);

//==============================================================================
// Defines

//==============================================================================
// sync defines
//
// WARNING: If you add more sync defines, be sure to add the corresponding cConfig*Sync for
// when we adjust the sync on a per define basis from the config
// see configsgame.h/cpp under Multiplayer syncing
BSyncDefine syncDefines[] = 
{        // Tag               // Name              // default state
   {     cRandSync,           "Rand",              cXORSyncState,       },
   {     cPlayerSync,         "Player",            cXORSyncState,       },
   {     cTeamSync,           "Team",              cXORSyncState,       },
   {     cUnitGroupSync,      "UG",                cXORSyncState,       },
   {     cUnitSync,           "Unit",              cXORSyncState,       },
   {     cUnitDetailSync,     "UnitDetail",        cXORSyncState,       },
   {     cUnitActionSync,     "UnitAction",        cXORSyncState,       },
   {     cSquadSync,          "Squad",             cXORSyncState,       },
   {     cProjectileSync,     "Projectile",        cXORSyncState,       },
   {     cWorldSync,          "World",             cXORSyncState,       },   
   {     cTechSync,           "Tech",              cXORSyncState,       },
   {     cCommandSync,        "Command",           cXORSyncState,       },
   {     cPathingSync,        "Pathing",           cXORSyncState,       },
   {     cMovementSync,       "Movement",          cXORSyncState,       },
   {     cVisibilitySync,     "Visibility",        cXORSyncState,       },
   {     cFinalReleaseSync,   "FinalRelease",      cXORSyncState,       },
   {     cFinalDetailSync,    "FinalDetail",       cXORSyncState,       },
   {     cChecksumSync,       "Checksum",          cXORSyncState,       },
   {     cTriggerSync,        "Trigger",           cXORSyncState,       },
   {     cTriggerVarSync,     "TriggerVar",        cXORSyncState,       },
   {     cAnimSync,           "Anim",              cXORSyncState,       },
   {     cDoppleSync,         "Dopple",            cXORSyncState,       },
   {     cCommSync,           "Comm",              cXORSyncState,       },
   {     cPlatoonSync,        "Platoon",           cXORSyncState,       },
};

//bool gSyncRecord = false; // global for performance reasons



//==============================================================================
// Save game version for Sync Data
const DWORD BSyncValue::msSaveVersion = 1;
      DWORD BSyncValue::msLoadVersion = 0xFFFFFFFF;

const DWORD BSyncUpdate::msSaveVersion = 1;
      DWORD BSyncUpdate::msLoadVersion = 0xFFFFFFFF;

//==============================================================================
const DWORD BSyncManager::mCodeSyncValue = 0x140275e3;

//==============================================================================
// 
//==============================================================================
BSyncStream::BSyncStream() :
   BByteStream(),
   mpAllocator(NULL),
   mpCurrentBuffer(NULL),
   mpStream(NULL),
   mpSyncFIFO(NULL),
   mAllowDynamicAlloc(FALSE)
{
}

//==============================================================================
// 
//==============================================================================
void BSyncStream::init(BSyncBufferAllocator* pAllocator, BStream* pDstStream, BSyncFIFO* pFIFO, BOOL allowDynamicAlloc)
{
   mpAllocator = pAllocator;
   mpStream = pDstStream;
   mpSyncFIFO = pFIFO;
   mAllowDynamicAlloc = allowDynamicAlloc;
}

//==============================================================================
// 
//==============================================================================
uint BSyncStream::writeBytes(const void* p, uint n)
{
   if (n == 0)
      return 0;

   uint num = BByteStream::writeBytes(p, n);
   if (num == n)
      return num;

   allocBuffer();

   return num + BByteStream::writeBytes((uchar*)p + num, n - num);

   //if (bytesLeft() < n)
   //   allocBuffer();

   //return BByteStream::writeBytes(p, n);
}

//==============================================================================
// 
//==============================================================================
bool BSyncStream::flush()
{
   submitBuffer();

   BSyncEvent* pEvent = mpSyncFIFO->getBackPtr();
   pEvent->mpBuf = NULL;
   pEvent->mpStream = mpStream;
   pEvent->mSeek = true;
   pEvent->mHandle = INVALID_HANDLE_VALUE;
   mpSyncFIFO->pushBack();

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BSyncStream::close()
{
   submitBuffer();

   BSyncEvent* pEvent = mpSyncFIFO->getBackPtr();
   pEvent->mpBuf = NULL;
   pEvent->mpStream = mpStream;
   pEvent->mSeek = true;
   pEvent->mHandle = (HANDLE)mFlushEvent;
   mpSyncFIFO->pushBack();

   return true;
}

//==============================================================================
// 
//==============================================================================
void BSyncStream::flushWait()
{
   submitBuffer();

   if (WaitForSingleObject(mFlushEvent, 0) == WAIT_OBJECT_0)
      return;

   BSyncEvent* pEvent = mpSyncFIFO->getBackPtr();
   pEvent->mpBuf = NULL;
   pEvent->mpStream = mpStream;
   pEvent->mSeek = true;
   pEvent->mHandle = (HANDLE)mFlushEvent;
   mpSyncFIFO->pushBack();

   mFlushEvent.wait();
}

//==============================================================================
// 
//==============================================================================
void BSyncStream::allocBuffer()
{
   submitBuffer();

   BSyncBuffer* pBuffer = NULL;
   while (pBuffer == NULL)
   {
      pBuffer = mpAllocator->alloc();
      if (pBuffer != NULL)
      {
         pBuffer->mDynamic = FALSE;
      }
      else if (mAllowDynamicAlloc)
      {
         pBuffer = HEAP_NEW(BSyncBuffer, gSyncHeap);
         pBuffer->mDynamic = TRUE;
      }
      else
      {
         Sleep(1);
      }
   }

   mpCurrentBuffer = pBuffer;
   mpCurrentBuffer->mBufSize = 0;
   set(mpCurrentBuffer->mBuf, BSyncBuffer::cBufSize, cSFWritable | cSFSeekable);
}

//==============================================================================
// 
//==============================================================================
void BSyncStream::submitBuffer()
{
   if (mpCurrentBuffer)
   {
      mpCurrentBuffer->mBufSize = static_cast<uint>(curOfs());

      BSyncEvent* pEvent = mpSyncFIFO->getBackPtr();
      pEvent->mpBuf = mpCurrentBuffer;
      pEvent->mpStream = mpStream;
      pEvent->mSeek = false;
      pEvent->mHandle = INVALID_HANDLE_VALUE;
      mpSyncFIFO->pushBack();

      mpCurrentBuffer = NULL;
   }

   set((void*)NULL, 0);
}

//==============================================================================
//==============================================================================
// BSyncValue Methods
//==============================================================================
//==============================================================================

//==============================================================================
// BSyncValue::save
//==============================================================================
bool BSyncValue::save(BChunkWriter* writer)
{
   if (!writer)
      return(false);

   long result;
   CHUNKWRITESAFE(writer, DWORD, mDWORD);
   CHUNKWRITESAFE(writer, WORD, mCallIndex);
   
   return(true);
}

//==============================================================================
// BSyncValue::load
//==============================================================================
bool BSyncValue::load(BChunkReader* reader)
{
   if (!reader)
      return(false);

   long result;
   DWORD val;
   CHUNKREADSAFE(reader, DWORD, val); mDWORD = val;
   CHUNKREADSAFE(reader, WORD, mCallIndex);

   return(true);
}

//==============================================================================
// BSyncValue::writeVersion
//==============================================================================
bool BSyncValue::writeVersion(BChunkWriter* chunkWriter)
{
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   return(chunkWriter->writeTaggedDWORD(BCHUNKTAG("SD"), msSaveVersion) == 1);
}

//==============================================================================
// BSyncValue::readVersion
//==============================================================================
bool BSyncValue::readVersion(BChunkReader* chunkReader)
{
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }

   return(chunkReader->readTaggedDWORD(BCHUNKTAG("SD"), &msLoadVersion) == 1);
}

//==============================================================================
//==============================================================================
// BSyncCallEntry class Methods
//==============================================================================
//==============================================================================

//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cVOID)
{
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}

//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, long v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cLONG)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}

//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, int v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cINT)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}
   
//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, DWORD v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cDWORD)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}

//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, float v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cFLOAT)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}

//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, LPSTR v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cSTRING)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}

//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, LPCSTR v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cSTRING)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}
//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, BVector& v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cFLOAT)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}

//==============================================================================
// BSyncCallEntry::BSyncCallEntry
//==============================================================================
BSyncCallEntry::BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, const BVector& v) : 
   mlpszDescription(const_cast<char*>(lpszDescription)),
   mlpszFile(const_cast<char*>(lpszFile)),
   mwLine(wLine),
   mwIndex(0xffff),
   mbTag(bTag),
   mbType(cFLOAT)
{
   v;
   BSyncManager::getInstance()->getCurrentHistory()->assignSyncCallIndex(this);
}

//==============================================================================
// BSyncCallEntry::save
//==============================================================================
bool BSyncCallEntry::save(BChunkWriter *writer)
{
   long result;

   long length = strlen(mlpszDescription) + 1;
   result = writer->writeCharArray(length, mlpszDescription);
   if (!result)
   {
      blog("BSyncCallEntry::save -- failed to write mlpszDescription.");
      return false;
   }

   length = strlen(mlpszFile) + 1;
   result = writer->writeCharArray(length, mlpszFile);
   if (!result)
   {
      blog("BSyncCallEntry::save -- failed to write mlpszFile.");
      return false;
   }

   CHUNKWRITESAFE(writer, WORD, mwLine);
   CHUNKWRITESAFE(writer, WORD, mwIndex);
   CHUNKWRITESAFE(writer, BYTE, mbTag);
   CHUNKWRITESAFE(writer, BYTE, mbType);

   return true;
}

//==============================================================================
// BSyncCallEntry::load
//==============================================================================
bool BSyncCallEntry::load(BChunkReader *reader)
{
   long result, length=0, read;

   result = reader->peekArrayLength(&length);
   if (!result)
   {
      blog("BSyncCallEntry::load -- failed to retrieve array length.");
      return false;
   }

   if (length > 0)
   {
      mlpszDescription = reinterpret_cast<char*>(gSyncHeap.New(length));
      result = reader->readCharArray(&read, (char*)mlpszDescription, length);
      if (!result || read != length)
      {
         blog("BSyncCallEntry::load -- failed to read mlpszDescription.");
         return false;
      }
   }

   result = reader->peekArrayLength(&length);
   if (!result)
   {
      blog("BSyncCallEntry::load -- failed to retrieve array length.");
      return false;
   }

   if (length > 0)
   {
      mlpszFile = reinterpret_cast<char*>(gSyncHeap.New(length));
      result = reader->readCharArray(&read, (char*)mlpszFile, length);
      if (!result || read != length)
      {
         blog("BSyncCallEntry::load -- failed to read mlpszFile.");
         return false;
      }
   }

   CHUNKREADSAFE(reader, WORD, mwLine);
   CHUNKREADSAFE(reader, WORD, mwIndex);
   CHUNKREADSAFE(reader, BYTE, mbTag);
   CHUNKREADSAFE(reader, BYTE, mbType);

   return true;
}

//==============================================================================
// BSyncCallEntry::clearAllocated
//==============================================================================
void BSyncCallEntry::clearAllocated()
{
   if (mlpszDescription)
      gSyncHeap.Delete(mlpszDescription);
   if (mlpszFile)
      gSyncHeap.Delete(mlpszFile);
   mlpszDescription = NULL;
   mlpszFile = NULL;
}

//==============================================================================
//==============================================================================
// BSyncUpdate class Methods
//==============================================================================
//==============================================================================

//==============================================================================
// BSyncUpdate::BSyncUpdate constructor
//==============================================================================
BSyncUpdate::BSyncUpdate() :
   mUpdateNumber(0),
   mValueArraySize(0),
   mStringArraySize(0),
   mUniqueID(0),
   mpAllocator(NULL),
   mpSyncFIFO(NULL),
   mID(0),
   mFileIndex(0),
   mpAsyncPackData(NULL),
   mpStream(NULL),
   mpRawStream(NULL),
   mpSyncStream(NULL),
   mpCurrentBuffer(NULL),
   mStreamDirty(FALSE),
   mUseCacheDrive(FALSE),
   mAllowDynamicAlloc(FALSE)
{
   Utils::ClearObj(mTagChecksum);
}

//==============================================================================
// BSyncUpdate::BSyncUpdate
//==============================================================================
BSyncUpdate::BSyncUpdate(const BSyncUpdate& other) :
   mUpdateNumber(0),
   mValueArraySize(0),
   mStringArraySize(0),
   mUniqueID(0),
   mpAllocator(NULL),
   mpSyncFIFO(NULL),
   mID(0),
   mFileIndex(0),
   mpAsyncPackData(NULL),
   mpStream(NULL),
   mpRawStream(NULL),
   mpSyncStream(NULL),
   mpCurrentBuffer(NULL),
   mStreamDirty(FALSE),
   mUseCacheDrive(FALSE),
   mAllowDynamicAlloc(FALSE)
{
   assign(other);
}

//==============================================================================
// BSyncUpdate::~BSyncUpdate
//==============================================================================
BSyncUpdate::~BSyncUpdate()
{   
   waitForAsyncPacking();

   if (mpSyncStream != NULL)
      HEAP_DELETE(mpSyncStream, gSyncHeap);
   mpSyncStream = NULL;

   if (mpRawStream != NULL)
   {
      mpRawStream->close();
      HEAP_DELETE(mpRawStream, gSyncHeap);
   }
   mpRawStream = NULL;

   mpStream = NULL;
}

//==============================================================================
// BSyncUpdate::operator=
//==============================================================================
BSyncUpdate& BSyncUpdate::operator= (const BSyncUpdate& rhs)
{
   assign(rhs);
   return *this;
}

//==============================================================================
// BSyncUpdate::assign
//==============================================================================
void BSyncUpdate::assign(const BSyncUpdate& other)
{
   waitForAsyncPacking();
   const_cast<BSyncUpdate&>(other).waitForAsyncPacking();
   
   mValueArray = other.mValueArray;
   mStringArray = other.mStringArray;

   mPackedValues = other.mPackedValues;
   mPackedStrings = other.mPackedStrings;
   
   mUpdateNumber = other.mUpdateNumber;
   mValueArraySize = other.mValueArraySize;
   mStringArraySize = other.mStringArraySize;

   Utils::Copy(other.mTagChecksum, other.mTagChecksum + cNumberOfSyncDefines, mTagChecksum);
}

//==============================================================================
// 
//==============================================================================
void BSyncUpdate::init(BOOL useCacheDrive, BOOL allowDynamicAlloc, uint id, uint fileIndex, BSyncBufferAllocator* pAllocator, BSyncFIFO* pFIFO, uint64 uniqueID)
{
   mpAllocator = pAllocator;
   mpSyncFIFO = pFIFO;

   mUseCacheDrive = useCacheDrive;
   mAllowDynamicAlloc = allowDynamicAlloc;
   mID = id;
   mFileIndex = fileIndex;
   mUniqueID = uniqueID;

   clear();

   if (mpRawStream != NULL)
   {
      // reset the streams
      if (mpSyncStream != NULL)
         mpSyncStream->flushWait();
      else
         mpRawStream->seek(0);

      mpStream = mpRawStream;

      if (mpSyncStream != NULL)
         mpStream = mpSyncStream;

      if (mpStream != NULL)
      {
         mpStream->writeObj<uint64>(mUniqueID);
         mpStream->writeObj<long>(mUpdateNumber);
      }

      return;
   }

   // if we want to immediately stream to disk and we've enabled the cache drive
   if (!mUseCacheDrive)
      return;

   BString filename;
   //filename.format("cache:\\%d.%d.sync", mID, mFileIndex);
   filename.format("game:\\%d.%d.sync", mID, mFileIndex);

   BWin32FileStream* pTempStream = HEAP_NEW(BWin32FileStream, gSyncHeap);

   uint flags = cSFReadable | cSFWritable | cSFSeekable | cSFEnableBuffering;
   if (pTempStream->open(filename, flags, &gWin32LowLevelFileIO))
   {
      mpRawStream = pTempStream;
      mpStream = mpRawStream;
   }
   else
   {
      HEAP_DELETE(pTempStream, gSyncHeap);
   }

   if (mpStream != NULL)
   {
      mpSyncStream = HEAP_NEW(BSyncStream, gSyncHeap);
      mpSyncStream->init(mpAllocator, mpStream, mpSyncFIFO, mAllowDynamicAlloc);
      mpStream = mpSyncStream;
   }

   if (mpStream != NULL)
   {
      mpStream->writeObj<uint64>(mUniqueID);
      mpStream->writeObj<long>(mUpdateNumber);
   }
}

//==============================================================================
// BSyncUpdate::clear
//   Resets the contents of this update
//==============================================================================
void BSyncUpdate::clear(long updateNumber)
{
   waitForAsyncPacking();

   mUpdateNumber = updateNumber;
   mValueArraySize = 0;
   mStringArraySize = 0;

   // Reset # of checksums
   for (int i = 0; i < cNumberOfSyncDefines; i++)
      mTagChecksum[i] = 0;

   // rg [5/6/08] - Clear these arrays to prevent memory from slowly growing over time.
   mValueArray.clear();
   mStringArray.clear();
   mPackedValues.clear();
   mPackedStrings.clear();

   if (mUseCacheDrive && mpRawStream)
   {
      if (mpSyncStream != NULL && mpSyncStream->curOfs() == 12)
         return;

      if (mpSyncStream != NULL)
         mpSyncStream->flushWait();
      else
         mpRawStream->seek(0);

      mpStream = mpRawStream;

      if (mpSyncStream != NULL)
         mpStream = mpSyncStream;

      if (mpStream != NULL)
      {
         mpStream->writeObj<uint64>(mUniqueID);
         mpStream->writeObj<long>(mUpdateNumber);
      }
   }
}

//==============================================================================
// BSyncUpdate::trim
//==============================================================================
void BSyncUpdate::trim()
{
   waitForAsyncPacking();

   mValueArray.reserve(0);
   mStringArray.reserve(0);
}

namespace
{
   // srcArray must be an array of simple POD structs!
   template<class ArrayType> void packArray(ArrayType& srcArray, BDynamicSyncArray<DWORD>& dstArray)
   {
      const uint numSrcDWORDs = (srcArray.getSizeInBytes() + sizeof(DWORD) - 1) / sizeof(DWORD);
      dstArray.resize(BMemoryPacker::getMinimumCompressedDWORDs(numSrcDWORDs));

      BMemoryPacker packer;
      uint numDstDWORDs = dstArray.getSize();
      const bool success = packer.pack(reinterpret_cast<const DWORD*>(srcArray.getPtr()), numSrcDWORDs, dstArray.getPtr(), numDstDWORDs);

      if (!success)
         dstArray.clear();
      else
      {
         dstArray.resize(numDstDWORDs);
         dstArray.reserve(0);

         srcArray.clear();
      }
   }
   
   template<class ArrayType> bool unpackArray(BDynamicSyncArray<DWORD>& srcArray, ArrayType& dstArray)
   {
      uint numDstDWORDs = BMemoryUnpacker::numDstDWORDs(srcArray.getPtr());

      dstArray.resize((numDstDWORDs * sizeof(DWORD) + sizeof(ArrayType::valueType) - 1) / sizeof(ArrayType::valueType));

      BDEBUG_ASSERT(dstArray.getSizeInBytes() >= (numDstDWORDs * sizeof(DWORD)));

      BMemoryUnpacker unpacker;
      const bool success = unpacker.unpack(srcArray.getPtr(), srcArray.getSize(), reinterpret_cast<DWORD*>(dstArray.getPtr()), numDstDWORDs);
      BASSERT(success);

      srcArray.clear();
   
      return success;
   }
}   

//==============================================================================
// BSyncUpdate::asyncPackFunc
//==============================================================================
void BSyncUpdate::asyncPackFunc(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(BSyncUpdate_AsyncPackFunc)

   BAsyncPackData* pData = static_cast<BAsyncPackData*>(privateData0);

   packArray(*(pData->mpValueArray), pData->mPackedValues);
   packArray(*(pData->mpStringArray), pData->mPackedStrings);

   SetEvent(pData->mDoneEventHandle);
}

//==============================================================================
// BSyncUpdate::waitForAsyncPacking
//==============================================================================
void BSyncUpdate::waitForAsyncPacking()
{
   if (!mpAsyncPackData)
      return;

   SCOPEDSAMPLE(BSyncUpdate_WaitForAsyncPacking)
   
   gWorkDistributor.waitSingle(mAsyncPackDone);

   mPackedValues.swap(mpAsyncPackData->mPackedValues);
   mPackedStrings.swap(mpAsyncPackData->mPackedStrings);

   HEAP_DELETE(mpAsyncPackData, gSyncHeap);
   mpAsyncPackData = NULL;
}

//==============================================================================
// BSyncUpdate::pack
//==============================================================================
void BSyncUpdate::pack(bool waitForCompletion, bool repack)
{
   if (mUseCacheDrive)
   {
      if (repack)
      {
         clear(mUpdateNumber);
         return;
      }

      if (mpStream)
      {
         mStreamDirty = TRUE;

         mpStream->writeObj<uint8>(2);
         mpStream->close();
      }
      return;
   }

   SCOPEDSAMPLE(BSyncUpdate_Pack)

   waitForAsyncPacking();
   
   if (mPackedValues.getSize())
      return;

   mpAsyncPackData = HEAP_NEW(BAsyncPackData, gSyncHeap);
   mpAsyncPackData->mDoneEventHandle = mAsyncPackDone.getHandle();
   mpAsyncPackData->mpValueArray = &mValueArray;
   mpAsyncPackData->mpStringArray = &mStringArray;

   gWorkDistributor.queue(asyncPackFunc, mpAsyncPackData);
   gWorkDistributor.flush();

   if (waitForCompletion)
      waitForAsyncPacking();
}

//==============================================================================
// BSyncUpdate::unpack
//==============================================================================
void BSyncUpdate::unpack()
{
   if (mUseCacheDrive)
      return;

   SCOPEDSAMPLE(BSyncUpdate_Unpack)

   waitForAsyncPacking();

   if (mPackedValues.isEmpty())
      return;

   if (!unpackArray(mPackedValues, mValueArray))
      mValueArray.resize(mValueArraySize);

   if (!unpackArray(mPackedStrings, mStringArray))
      mStringArray.resize(mStringArraySize);

   BDEBUG_ASSERT(mValueArray.getSize() >= mValueArraySize);
   BDEBUG_ASSERT(mStringArray.getSize() >= mStringArraySize);
}

//==============================================================================
// 
//==============================================================================
void BSyncUpdate::unpackForRead()
{
   if (!mUseCacheDrive)
   {
      unpack();
      return;
   }

   if (!mStreamDirty)
      return;

   SCOPEDSAMPLE(BSyncUpdate_Unpack)

   if (mpRawStream != NULL)
   {
      // need to wait until all pending writes have completed before I can operate on the stream
      //
      // issue a flush on the stream but send an event along with the flush that gets signaled when the flush completes
      if (mpSyncStream != NULL)
         mpSyncStream->flushWait();

      mpRawStream->seek(0);
      mpStream = mpRawStream;

      BStream* pTempStream = mpRawStream;

      mValueArraySize = 0;
      mStringArraySize = 0;
      mUpdateNumber = 0;

      uint64 uniqueID = 0;
      if (pTempStream->readObj<uint64>(uniqueID) && uniqueID == mUniqueID)
      {
         pTempStream->readObj<long>(mUpdateNumber);

         uint8 type = 0;

         while (true)
         {
            if (!pTempStream->readObj<uint8>(type))
               break;
            if (type == 0)
            {
               // BSyncValue
               BSyncValue value;
               if (!pTempStream->readObj<BSyncValue>(value))
                  break;

               if (mValueArraySize >= mValueArray.getSize())
                  mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);

               mValueArray[mValueArraySize] = value;
               mValueArraySize++;
            }
            else if (type == 1)
            {
               // BSyncValue
               // BSyncStringValue
               BSyncValue value;
               if (!pTempStream->readObj<BSyncValue>(value))
                  break;

               BSyncStringValue string;
               if (!pTempStream->readObj<BSyncStringValue>(string))
                  break;

               if (mValueArraySize >= mValueArray.getSize())
                  mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);

               if (mStringArraySize >= mStringArray.getSize())
                  mStringArray.setNumber(mStringArraySize + cSyncUpdateStringGrowSize);

               mValueArray[mValueArraySize] = value;
               mStringArray[mStringArraySize++] = string;
               mValueArraySize++;
            }
            else if (type == 2)
               break; // EOF
         }
      }
   }

   mStreamDirty = FALSE;
}

//==============================================================================
// BSyncUpdate::writeChecksumsToBinaryFile
//==============================================================================
void BSyncUpdate::writeChecksumsToBinaryFile(BFile& file, BSyncLogHeader& lh)
{
   // log out checksums
   lh.dwChecksumCount += cNumberOfSyncDefines;

   for (long j=0; j < cNumberOfSyncDefines; j++)
   {
      BSyncChecksumRecord entry;

      memset(&entry, 0, sizeof(entry));
      entry.dwUpdate = mUpdateNumber;
      entry.dwChecksum = getChecksum(j);
      memcpy(entry.szName, syncDefines[j].mName, sizeof(entry.szName));
      entry.szName[sizeof(entry.szName)-1]=0;
      file.write(&entry, sizeof(entry));
   }
}

//==============================================================================
// BSyncUpdate::writeToBinaryFile
//==============================================================================
void BSyncUpdate::writeToBinaryFile(BFile& /*file*/, BSyncLogHeader& /*lh*/, BSyncStrLog& /*sl*/)
{
}

//==============================================================================
// BSyncUpdate::add
//==============================================================================
void BSyncUpdate::add(WORD wCallIndex, LPCSTR pStr)
{
   if (!pStr)
   {
      BASSERT(0);
      return;
   }
   
   if (mUseCacheDrive)
   {
      if (mpStream == NULL)
         return;

      BSyncValue value;
      BSyncStringValue string;

      value.mDWORD = mStringArraySize;
      value.mCallIndex = wCallIndex;

      string = pStr;

      mpStream->writeObj<uint8>(1);
      mpStream->writeObj<BSyncValue>(value);
      mpStream->writeObj<BSyncStringValue>(string);

      mStringArraySize++;

      compareAgainstPlaybackHistory(value);

      mValueArraySize++;

      return;
   }

   unpack();

   // grow array by defined size if need be
   if (mValueArraySize >= mValueArray.getSize())
      mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);  

   if (mStringArraySize >= mStringArray.getSize())
      mStringArray.setNumber(mStringArraySize + cSyncUpdateStringGrowSize);

   mValueArray[mValueArraySize].mDWORD       = mStringArraySize;
   mValueArray[mValueArraySize].mCallIndex   = wCallIndex;
   mStringArray[mStringArraySize++] = pStr;

   // if we're replaying a game, compare our current data against the old data
   compareAgainstPlaybackHistory(mValueArray[mValueArraySize]);

   ++mValueArraySize;
}

//==============================================================================
// BSyncUpdate::add
//==============================================================================
void BSyncUpdate::add(WORD wCallIndex, LPSTR pStr)
{
   if (!pStr)
   {
      BASSERT(0);
      return;
   }
   
   if (mUseCacheDrive)
   {
      if (mpStream == NULL)
         return;

      BSyncValue value;
      BSyncStringValue string;

      value.mDWORD = mStringArraySize;
      value.mCallIndex = wCallIndex;

      string = pStr;

      mpStream->writeObj<uint8>(1);
      mpStream->writeObj<BSyncValue>(value);
      mpStream->writeObj<BSyncStringValue>(string);

      mStringArraySize++;

      compareAgainstPlaybackHistory(value);

      mValueArraySize++;

      return;
   }

   unpack();

   // grow array by defined size if need be
   if (mValueArraySize >= mValueArray.getSize())
      mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);  

   if (mStringArraySize >= mStringArray.getSize())
      mStringArray.setNumber(mStringArraySize + cSyncUpdateStringGrowSize);

   mValueArray[mValueArraySize].mDWORD       = mStringArraySize;
   mValueArray[mValueArraySize].mCallIndex   = wCallIndex;
   mStringArray[mStringArraySize++] = pStr;

   // if we're replaying a game, compare our current data against the old data
   compareAgainstPlaybackHistory(mValueArray[mValueArraySize]);

   ++mValueArraySize;
}

//==============================================================================
// BSyncUpdate::add
//==============================================================================
void BSyncUpdate::add(WORD wCallIndex, long lValue)
{
   if (mUseCacheDrive)
   {
      if (mpStream == NULL)
         return;

      BSyncValue value;

      value.mLong = lValue;
      value.mCallIndex = wCallIndex;

      mpStream->writeObj<uint8>(0);
      mpStream->writeObj<BSyncValue>(value);

      compareAgainstPlaybackHistory(value);

      mValueArraySize++;

      return;
   }

   unpack();

   // grow array by defined size if need be
   if (mValueArraySize >= mValueArray.getSize())
      mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);  

   mValueArray[mValueArraySize].mLong      = lValue;
   mValueArray[mValueArraySize].mCallIndex = wCallIndex;

   // if we're replaying a game, compare our current data against the old data
   compareAgainstPlaybackHistory(mValueArray[mValueArraySize]);

   ++mValueArraySize;
}

//==============================================================================
// BSyncUpdate::add
//==============================================================================
void BSyncUpdate::add(WORD wCallIndex, DWORD dwValue)
{
   if (mUseCacheDrive)
   {
      if (mpStream == NULL)
         return;

      BSyncValue value;

      value.mDWORD = dwValue;
      value.mCallIndex = wCallIndex;

      mpStream->writeObj<uint8>(0);
      mpStream->writeObj<BSyncValue>(value);

      compareAgainstPlaybackHistory(value);

      mValueArraySize++;

      return;
   }

   unpack();

   // grow array by defined size if need be
   if (mValueArraySize >= mValueArray.getSize())
      mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);

   mValueArray[mValueArraySize].mDWORD     = dwValue;
   mValueArray[mValueArraySize].mCallIndex = wCallIndex;

   // if we're replaying a game, compare our current data against the old data
   compareAgainstPlaybackHistory(mValueArray[mValueArraySize]);

   ++mValueArraySize;
}

//==============================================================================
// BSyncUpdate::add
//==============================================================================
void BSyncUpdate::add(WORD wCallIndex, float fValue)
{
   if (mUseCacheDrive)
   {
      if (mpStream == NULL)
         return;

      BSyncValue value;

      value.mFloat = fValue;
      value.mCallIndex = wCallIndex;

      mpStream->writeObj<uint8>(0);
      mpStream->writeObj<BSyncValue>(value);

      compareAgainstPlaybackHistory(value);

      mValueArraySize++;

      return;
   }

   unpack();

   // grow array by defined size if need be
   if (mValueArraySize >= mValueArray.getSize())
      mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);

   mValueArray[mValueArraySize].mFloat     = fValue;
   mValueArray[mValueArraySize].mCallIndex = wCallIndex;

   // if we're replaying a game, compare our current data against the old data
   compareAgainstPlaybackHistory(mValueArray[mValueArraySize]);

   ++mValueArraySize;
}

//==============================================================================
// BSyncUpdate::add
//==============================================================================
void BSyncUpdate::add(WORD wCallIndex, int nValue)
{
   if (mUseCacheDrive)
   {
      if (mpStream == NULL)
         return;

      BSyncValue value;

      value.mInt = nValue;
      value.mCallIndex = wCallIndex;

      mpStream->writeObj<uint8>(0);
      mpStream->writeObj<BSyncValue>(value);

      compareAgainstPlaybackHistory(value);

      mValueArraySize++;

      return;
   }

   unpack();

   // grow array by defined size if need be
   if (mValueArraySize >= mValueArray.getSize())
      mValueArray.setNumber(mValueArraySize + cSyncUpdateValueGrowSize);

   mValueArray[mValueArraySize].mInt       = nValue;
   mValueArray[mValueArraySize].mCallIndex = wCallIndex;

   // if we're replaying a game, compare our current data against the old data
   compareAgainstPlaybackHistory(mValueArray[mValueArraySize]);

   ++mValueArraySize;
}

//==============================================================================
// 
//==============================================================================
const BSyncValue* BSyncUpdate::getValue(uint index, bool useStream)
{ 
   unpackForRead();

   if (index >= mValueArray.getSize())
      return NULL;
   return &mValueArray[index];
}

//==============================================================================
// 
//==============================================================================
const BSyncStringValue* BSyncUpdate::getStringValue(uint index)
{ 
   unpackForRead();

   if (index >= mStringArray.getSize())
      return NULL;

   return &mStringArray[index];
}

//==============================================================================
// BSyncUpdate::compareAgainstPlaybackHistory
//==============================================================================
void BSyncUpdate::compareAgainstPlaybackHistory(const BSyncValue& currentdata)
{
   if (!gRecordGame.isPlaying() || !gRecordGame.isSyncing())
      return;

   if (BSyncManager::getInstance()->getOOSAfterUpdate() != -1)
   {
      if (mUpdateNumber > BSyncManager::getInstance()->getOOSAfterUpdate())
         BSyncManager::getInstance()->outOfSync(BSyncManager::getInstance()->getOOSAfterUpdate() - 1);
      return;
   }

   if (mValueArraySize <= BSyncManager::getInstance()->getPlaybackHistory()->getCurrentUpdate()->getCount())
   {
      const BSyncValue* pOld = BSyncManager::getInstance()->getPlaybackHistory()->getCurrentUpdate()->getValue(mValueArraySize, false);
      if (pOld && currentdata == *pOld)
         return;
   }

   //if (mUpdateNumber == 0)
   //   return;

   if(BSyncManager::getInstance()->getOOSAfterUpdate() == -1)
      BSyncManager::getInstance()->setOOSAfterUpdate(mUpdateNumber+1);
}

//==============================================================================
// BSyncUpdate::writeVersion
//==============================================================================
bool BSyncUpdate::writeVersion(BChunkWriter* chunkWriter)
{
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      {setBlogError(2591); blogerror("BSyncUpdate::writeVersion -- bad chunkWriter.");}
      return(false);
   }

   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("SU"), msSaveVersion);
   if (!result)
   {
      {setBlogError(2592); blogerror("BSyncUpdate::writeVersion -- failed to write version.");}
      return(false);
   }
   return(true);
}

//==============================================================================
// BSyncUpdate::readVersion
//==============================================================================
bool BSyncUpdate::readVersion(BChunkReader* chunkReader)
{
   if (chunkReader == NULL)
   {
      BASSERT(0);
      {setBlogError(2593); blogerror("BSyncUpdate::readVersion -- bad chunkReader.");}
      return(false);
   }

   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("SU"), &msLoadVersion);
   if (!result)
   {
      {setBlogError(2594); blogerror("BSyncUpdate::readVersion -- failed to read version.");}
      return(false);
   }
   return(true);
}

//==============================================================================
// BSyncUpdate::save
//==============================================================================
bool BSyncUpdate::save(BChunkWriter *writer)
{
   if (!writer)
      return(false);

   unpack();

   long result;
   CHUNKWRITESAFE(writer, Long, mValueArraySize);

   for (uint i=0;i<mValueArraySize;i++)
   {
      if (!mValueArray[i].save(writer))
         return(false);
   }

   CHUNKWRITESAFE(writer, Long, mStringArraySize);
   for (uint i=0; i<mStringArraySize; i++)
      writer->writeCharArray(mStringArray[i].length() + 1, mStringArray[i].getPtr());  //+1 to write the NULL

   long count = cNumberOfSyncDefines;
   CHUNKWRITESAFE(writer, Long, count);

   for (long i=0;i<cNumberOfSyncDefines;i++)
   {
      CHUNKWRITESAFE(writer, DWORD, mTagChecksum[i]);
   }

   return(true);
}

//==============================================================================
// BSyncUpdate::load
//==============================================================================
bool BSyncUpdate::load(BChunkReader *reader)
{   
   if (!reader)
      return(false);

   unpack();
   
   long result;
   long size;
   CHUNKREADSAFE(reader, Long, size);
   mValueArray.setNumber(size);
   mValueArraySize = size;

   for (uint i=0;i<mValueArraySize;i++)
   {
      if (!mValueArray[i].load(reader))
         return(false);
   }

   CHUNKREADSAFE(reader, Long, size);
   mStringArray.setNumber(size);
   mStringArraySize = size;

   for (uint i=0; i<mStringArraySize; i++)
   {
      char buf[BSyncStringValue::cMaxStrLen + 1];
      reader->readCharArray(&size, buf, BSyncStringValue::cMaxStrLen + 1); // +1 to read the NULL
      
      mStringArray[i] = buf;
   }

   long numSyncsSaved = 0;
   CHUNKREADSAFE(reader, Long, numSyncsSaved);

   for (long i = 0; i < numSyncsSaved; i++)
   {
      DWORD Checksum;
      CHUNKREADSAFE(reader, DWORD, Checksum);

      if (i < cNumberOfSyncDefines)
      {
         mTagChecksum[i] = Checksum;
      }
   }
   return(true);
}

//==============================================================================
// 
//==============================================================================
void BSyncUpdate::logStats()
{
#ifndef BUILD_FINAL
   // log the current number of values/strings used
   nlog(cSyncCL, "SYNC-UPDATE-STATS, Number: %d, Values: %d, Strings: %d, Value Size: %d, String Size: %d", mUpdateNumber, mValueArraySize, mStringArraySize, mValueArraySize*sizeof(BSyncValue), mStringArraySize*sizeof(BSyncStringValue));
#endif
}

//==============================================================================
//==============================================================================
// BSyncHistory class Methods
//==============================================================================
//==============================================================================


//==============================================================================
// BSyncHistory::BSyncHistory  Constructor
//==============================================================================
BSyncHistory::BSyncHistory(long amountOfUpdatesInHistory) :
   mUniqueID(0),
   mpAllocator(NULL),
   mpSyncFIFO(NULL),
   mUpdateHead(0),
   mUpdateTail(0),
   mUpdateNumber(0),
   mHistoryChecksum(0),
   mCompression(TRUE),
   mUseCacheDrive(FALSE),
   mAllowDynamicAlloc(FALSE),
   mbSyncCallAllocated(false)
{
   mUpdates.setNumber(amountOfUpdatesInHistory);

#ifndef BUILD_FINAL
   if (gConfig.isDefined("syncManagerNoCompression"))
      mCompression = FALSE;
   if (gConfig.isDefined("syncManagerUseCacheDrive"))
      mUseCacheDrive = TRUE;
   if (gConfig.isDefined("syncManagerAllowDynamicAlloc"))
      mAllowDynamicAlloc = TRUE;

   //if (!BXboxFileUtils::isCacheInitialized())
   //   mUseCacheDrive = FALSE;
#endif
}

//==============================================================================
// BSyncHistory::~BSyncHistory  Destructor
//==============================================================================
BSyncHistory::~BSyncHistory()
{
   if (mbSyncCallAllocated)
   {
      long count = mSyncCallTable.getNumber();
      for (long idx=0; idx<count; idx++)
      {
         mSyncCallTable[idx]->clearAllocated();
         HEAP_DELETE(mSyncCallTable[idx], gSyncHeap);
      }
      mSyncCallTable.setNumber(0);
   }
}

//==============================================================================
// 
//==============================================================================
void BSyncHistory::init(uint id, BSyncBufferAllocator* pAllocator, BSyncFIFO* pFIFO)
{
   mpAllocator = pAllocator;
   mpSyncFIFO = pFIFO;

   mID = id;

   clear();
}

//==============================================================================
// BSyncHistory::clear
//==============================================================================
void BSyncHistory::clear()
{
   XNetRandom((BYTE*)&mUniqueID, sizeof(mUniqueID));

   for (uint i=0; i < mUpdates.getSize(); i++)
      mUpdates[i].init(mUseCacheDrive, mAllowDynamicAlloc, mID, i+1, mpAllocator, mpSyncFIFO, mUniqueID);

   mUpdateHead = 0;
   mUpdateTail = 0;
   mUpdateNumber = 0;
   mHistoryChecksum = 0;

   //setupUpdateChecksums();
} 

//==============================================================================
// BSyncHistory::getUpdate
//==============================================================================
BSyncUpdate* BSyncHistory::getUpdate(long offset)
{
   if (offset < 0)
      return NULL;

   offset = mUpdateTail+offset;
   if (offset >= mUpdates.getNumber())
   {
      offset -= mUpdates.getNumber();
   }
   return &mUpdates[offset];
}


//==============================================================================
// BSyncHistory::nextUpdate
//==============================================================================
void BSyncHistory::nextUpdate()
{
   // Add current updates tags to the combined checksum.
   for (int i = 0; i < cNumberOfSyncDefines; i++)
   {
      if(getCurrentUpdate())
      {
         DWORD systemCRC = getCurrentUpdate()->getChecksum(i);
         Crc324Bytes(mHistoryChecksum, &systemCRC);
      }
   }

   {
      SCOPEDSAMPLE(BSyncHistory_NextUpdate)

      if (!mCompression && !mUseCacheDrive)
         mUpdates[mUpdateHead].trim();
      else
         mUpdates[mUpdateHead].pack();
   }

   // Advance the round robin list to the next update
   if (++mUpdateHead >= mUpdates.getNumber())
   {
      mUpdateHead = 0;
   }
   if (mUpdateHead == mUpdateTail)
   {
      mUpdateTail++;
      if (mUpdateTail >= mUpdates.getNumber())
      {
         mUpdateTail = 0;
      }
   }

   mUpdateNumber++;

   // Reset the update space we are going to use for the next update

   mUpdates[mUpdateHead].clear(mUpdateNumber);
}

//==============================================================================
// BSyncHistory::getCurrentChecksum
//==============================================================================
DWORD BSyncHistory::getCurrentChecksum()
{
   return mHistoryChecksum;
}

//==============================================================================
// BSyncHistory::setupUpdateChecksums
//==============================================================================
//void BSyncHistory::setupUpdateChecksums()
//{
//   for (long i=0; i < cNumberOfSyncDefines; i++)
//   {      
//      getCurrentUpdate()->setChecksum(i, 0);
//   }
//}

//==============================================================================
// BSyncHistory::getAmountOfUpdates
//==============================================================================
long BSyncHistory::getAmountOfUpdates() 
{ 
   // rg [5/6/08] - TODO: This logic looks flawed - mUpdateTail will occasionally be 0, even after the head starts wrapping. 
   
   if (mUpdateTail == 0)
   {
      return (mUpdateHead-mUpdateTail);
   }
   else 
   {
      return mUpdates.getNumber(); 
   }
}

//==============================================================================
// BSyncHistory::writeToBinaryFile
//==============================================================================
void BSyncHistory::writeToBinaryFile(BFile& /*file*/, BSyncLogHeader& /*lh*/, long /*lOOSUpdateNumber*/)
{
}

//==============================================================================
// BSyncHistory::serializeToLog
//==============================================================================
void BSyncHistory::serializeToLog(long logHeaderID, long oosUpdateNumber)
{
   UNREFERENCED_PARAMETER(logHeaderID);

   const char *userInfo=NULL;
   const char *file=NULL;
   long sourceLine;

   nextUpdate();
   //setupUpdateChecksums();

   SYSTEMTIME t;
   GetLocalTime(&t);

   // write out the system time first, this will always be slightly off on all machines
   finalBlogh(logHeaderID, "System Time: %04d-%02d-%02d %02d:%02d:%02d.%03d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

#ifdef USE_BUILD_INFO
   finalBlogh(logHeaderID, "Build Info: DEPOT:"DEPOT_REVISION"|BUILD:"BUILD_NUMBER"|CHANGELIST:"CHANGELIST_NUMBER);
#else
   finalBlogh(logHeaderID, "Build Info: NO_BUILD_INFO");
#endif

#if defined(XBOX) && !defined(BUILD_FINAL)
   DM_XBE xbeInfo;
   HRESULT hr = DmGetXbeInfo(NULL, &xbeInfo);
   if (hr == XBDM_NOERR)
   {
      finalBlogh(logHeaderID, "XBE Launch Path: %s", xbeInfo.LaunchPath);
      finalBlogh(logHeaderID, "XBE TimeStamp: %d", xbeInfo.TimeStamp);
      finalBlogh(logHeaderID, "XBE CheckSum: %d", xbeInfo.CheckSum);
   }
   char dmXboxName[256];
   DWORD size = sizeof(dmXboxName);
   hr = DmGetXboxName(dmXboxName, &size);
   if (hr == XBDM_NOERR)
   {
      finalBlogh(logHeaderID, "Xbox Name: %s", dmXboxName);
   }
#endif

   // dump the map and all the players in the game
   BSimString gameID;
   BSimString mapName;
   gDatabase.getGameSettings()->getString(BGameSettings::cGameID, gameID);
   gDatabase.getGameSettings()->getString(BGameSettings::cMapName, mapName);

   finalBlogh(logHeaderID, "Game ID: %s", gameID.getPtr());
   finalBlogh(logHeaderID, "Map: %s", mapName.getPtr());
   finalBlogh(logHeaderID, "Archives: %s", (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) ? "ON" : "OFF");
   finalBlogh(logHeaderID, "Loose Files: %s", (gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles) ? "ON" : "OFF");
   finalBlogh(logHeaderID, "Game Count: %i", gGame.getGameCount());

   long gameMode;
   gDatabase.getGameSettings()->getLong(BGameSettings::cGameMode, gameMode);
//-- FIXING PREFIX BUG ID 3729
   const BGameMode* pGameMode = gDatabase.getGameModeByID(gameMode);
//--
   finalBlogh(logHeaderID, "Game Mode: %s", pGameMode ? pGameMode->getName().getPtr() : "INVALID");

   if (gWorld)
   {
      int totalPlayers = gWorld->getNumberPlayers();
      finalBlogh(logHeaderID, "Total Players: %d", totalPlayers);
      for (int i=0; i < totalPlayers; ++i)
      {
         const BPlayer* pPlayer = gWorld->getPlayer(i);
         if (pPlayer)
            finalBlogh(logHeaderID, "Player (%d) %s   Team (%d)", i, pPlayer->getName().getPtr(), pPlayer->getTeamID());
      }
   }

   finalBlogh(logHeaderID, "oosUpdateNumber %ld", oosUpdateNumber);
   finalBlogh(logHeaderID, "getAmountOfUpdates %ld", getAmountOfUpdates());

   long windowBefore = cOOSSerializationWindowBefore;
   long windowAfter = cOOSSerializationWindowAfter;
   if (!gConfig.get("oosSerializationWindowBefore", &windowBefore))
      windowBefore = cOOSSerializationWindowBefore;
   if (!gConfig.get("oosSerializationWindowAfter", &windowAfter))
      windowAfter = cOOSSerializationWindowAfter;

   long startIndex = 0;

   for (long i=0, minUpdateNumber=oosUpdateNumber; oosUpdateNumber != -1 && i < getAmountOfUpdates(); i++)
   {
      BSyncUpdate* pUpdate = getUpdate(i);
      if (pUpdate == NULL)
         continue;

      // if this update number + the window before our oos is equal to our oos number, then this
      // is the position we want to start from
      if (pUpdate->getUpdateNumber() + windowBefore == oosUpdateNumber)
      {
         minUpdateNumber = pUpdate->getUpdateNumber();
         startIndex = i;
         break;
      }

      // otherwise we're looking for the smallest update number to start from so we get something
      if (pUpdate->getUpdateNumber() < minUpdateNumber)
      {
         minUpdateNumber = pUpdate->getUpdateNumber();
         startIndex = i;
      }
   }

   // it's fine if the stopIndex is beyond our total update count
   // since we're going to stop when either condition is met
   long stopIndex = startIndex + windowBefore + windowAfter + 1;
   if (oosUpdateNumber == -1)
      stopIndex = getAmountOfUpdates();

   for (long i = startIndex; i < getAmountOfUpdates() && i < stopIndex; i++)
   {
      BSyncUpdate* pUpdate = getUpdate(i);     
      if (pUpdate == NULL)
      {
         BASSERT(0);
         continue;
      }

      // need to unpack the update in order to get an accurate count
      pUpdate->unpackForRead();

      // ajl 9/13/02 - changed this back... it needs to write out the full logs if the interval isn't one
      // otherwise the logs that are written out frequently won't show the intial OOS since the game isn't
      // sync checked every turn.
      // why were we doing this? it makes the sync logs HUGE - pdb 9/10/02
      //if ((oosUpdateNumber != -1) && (BSyncManager::getInstance()->getSyncUpdateInterval() == 1))
      //{
      //   //if (abs(getUpdate(i)->getUpdateNumber()-oosUpdateNumber) > cOOSSerializationWindow)
      //   long updateNumber = pUpdate->getUpdateNumber();
      //   if (updateNumber < oosUpdateNumber - windowBefore || updateNumber > oosUpdateNumber + windowAfter)
      //   {
      //      //finalBlogh(logHeaderID, "  continuing - %ld, %ld", abs(getUpdate(i)->getUpdateNumber()-oosUpdateNumber), cOOSSerializationWindow);
      //      continue; 
      //   }
      //}

      finalBlogh(logHeaderID, "SYNC-UPDATE %ld ---------------------------------", pUpdate->getUpdateNumber());

      uint count = pUpdate->getCount();
      for (uint line=0; line < count; line++)
      {
         const BSyncValue* pCurData = pUpdate->getValue(line);
         if (!pCurData)
            continue;

         const BSyncCallEntry* pEntry = getSyncCallEntry(pCurData->mCallIndex);
         if (!pEntry)
         {
            finalBlogh(logHeaderID, "Invalid CallIndex    : %ld", pCurData->mCallIndex);
            continue;
         }

         userInfo = pEntry->mlpszDescription;
         file = pEntry->mlpszFile;
         sourceLine = pEntry->mwLine;

         switch (pEntry->mbType)
         {            
            case BSyncCallEntry::cVOID:
            {
               finalBlogh(logHeaderID, "%-32s (CODE)    : \"%s\", %ld", userInfo, file, sourceLine);
            }
            break;

            case BSyncCallEntry::cFLOAT:
            {
               finalBlogh(logHeaderID, "%-32s (FLOAT)   : %.24f - \"%s\", %ld", userInfo, pCurData->mFloat, file, sourceLine);

               #if 0
               for (long i=0;i<4;i++)
               {
                  long l = ((char *) &(pCurData->mFloat))[i];
                  finalBlogh(logHeaderID, "  (FLOAT-%ld)   : %ld", i, l);
               }
               #endif
            }
            break;

            case BSyncCallEntry::cINT:
            case BSyncCallEntry::cLONG:
            {
               finalBlogh(logHeaderID, "%-32s (LONG)    : %ld - \"%s\", %ld", userInfo, pCurData->mLong, file, sourceLine);
            }
            break;

            case BSyncCallEntry::cDWORD:
            {
               finalBlogh(logHeaderID, "%-32s (DWORD)   : %lu - \"%s\", %ld", userInfo, pCurData->mDWORD, file, sourceLine);
            }
            break;

            case BSyncCallEntry::cSTRING:
            {
               //BASSERT(pCurData->mDWORD >= 0);
               const BSyncStringValue* pStringValue = pUpdate->getStringValue(pCurData->mDWORD);
               finalBlogh(logHeaderID, "%-32s (STRING)  : %s - \"%s\", %ld", userInfo, (pStringValue ? pStringValue->getPtr() : "error"), file, sourceLine);
            }
            break;

            default:
            {
               BASSERT(0);
               finalBlogh(logHeaderID, "%-32s (ERR %ld) : %ld - \"%s\", %ld", userInfo, pCurData->mDWORD, 0, file, sourceLine);
            }
            break;
         }
      }

      // log out checksums
      for (long j=0; j < cNumberOfSyncDefines; j++)
      {
         finalBlogh(logHeaderID, "%-32s    : %lu", syncDefines[j].mName, pUpdate->getChecksum(j));
      }

      // repack the update to save memory
      pUpdate->pack(true, true);
   }

   finalBlogh(logHeaderID, "=====================================================================================");
   finalBlogh(logHeaderID, "The following is not sync'd data.  This is a dump of the units in the game. ");
   finalBlogh(logHeaderID, "=====================================================================================");
   if (gWorld)
   {
      BEntityHandle handle = cInvalidObjectID;
      BEntity* pEntity = gWorld->getNextUnit(handle);
      while (pEntity)
      {
         const BSimString * name = NULL;
         if( pEntity->getUnit() && pEntity->getUnit()->getProtoObject() )
            name = &pEntity->getUnit()->getProtoObject()->getName();

         finalBlogh(logHeaderID, "Unit id (%d) name (%32s) player (%d) position (%f, %f, %f)", pEntity->getID(), 
            name ? name->getPtr() : NULL, pEntity->getPlayerID(), pEntity->getPosition().x, pEntity->getPosition().y, pEntity->getPosition().z );
         pEntity = gWorld->getNextUnit(handle);
      }
   }

   if (gLiveSystem != NULL)
   {
      if (gLiveSystem->isMultiplayerGameActive() && gLiveSystem->getMPSession() && gLiveSystem->getMPSession()->getSession() && gLiveSystem->getMPSession()->getSession()->getTimeSync())
      {
         finalBlogh(logHeaderID, "=====================================================================================");
         finalBlogh(logHeaderID, "The following is not sync'd data.  This is a dump of the last several timing values");
         finalBlogh(logHeaderID, "=====================================================================================");

         const BTimingLog& timingLog = gLiveSystem->getMPSession()->getSession()->getTimeSync()->getTimingLog();
         finalBlogh(logHeaderID, "Time Sync Game Time: %d", timingLog.getGameTime());

         for (uint i=0, j=timingLog.getStartingIndex(); i < BTimingLog::cMaxEntries; ++i)
         {
            const BTimingLogEntry& entry = timingLog.getEntry(j);

            if (entry.getType() == BTimingLogEntry::cTiming)
               finalBlogh(logHeaderID, "Time Received: %d:%d:%d", entry.getClientID(), entry.getSendTime(), entry.getTiming());

            if (++j >= BTimingLog::cMaxEntries)
               j = 0;
         }
         for (uint i=0, j=timingLog.getStartingIndex(); i < BTimingLog::cMaxEntries; ++i)
         {
            const BTimingLogEntry& entry = timingLog.getEntry(j);

            if (entry.getType() == BTimingLogEntry::cAdvanceTime)
               finalBlogh(logHeaderID, "Advance Recv Time: %d:%d:%d:%d (game time: %d)", entry.getRecvTime(), entry.getRecvUpdateInterval(), entry.getEarliestClientTimeIndex(), entry.getEarliestAllowedRecvTime(), entry.getGameTime());

            if (++j >= BTimingLog::cMaxEntries)
               j = 0;
         }
      }
   }
}


//==============================================================================
// BSyncHistory::saveHeader
//==============================================================================
bool BSyncHistory::saveHeader(BChunkWriter *writer)
{
   // save tag states  
   long result;
   long count = cNumberOfSyncDefines;
   CHUNKWRITESAFE(writer, Long, count);

   for (long idx=0; idx<count; idx++)
   {
      CHUNKWRITESAFE(writer, Long, syncDefines[idx].mState);
   }
   
   return true;
}

//==============================================================================
// BSyncHistory::loadHeader
//==============================================================================
bool BSyncHistory::loadHeader(BChunkReader *reader)
{
   // load tag states

   long count,s;
   long result;
   CHUNKREADSAFE(reader, Long, count);

   BASSERT(cNumberOfSyncDefines >= count);

   BSyncManager::getInstance()->setLoadedNumberOfSyncDefines(count);

   for (long idx=0; idx<count; idx++)  
   {
      CHUNKREADSAFE(reader, Long, s);
      syncDefines[idx].mState = s;
   }

   return true;
}


//==============================================================================
// BSyncHistory::saveChecksums
//==============================================================================
bool BSyncHistory::saveChecksums(BChunkWriter *writer)
{
   long result;
   CHUNKWRITESAFE(writer, DWORD, mHistoryChecksum);

   BSyncUpdate* update=getCurrentUpdate();

   CHUNKWRITESAFE(writer, Long, cNumberOfSyncDefines);

   for (int i = 0; i < cNumberOfSyncDefines; i++)
   {
      if(update)
         CHUNKWRITESAFE(writer, DWORD, update->getChecksum(i));
      else
         CHUNKWRITESAFE(writer, DWORD, 0);
   }

   return true;
}


//==============================================================================
// BSyncHistory::loadChecksums
//==============================================================================
bool BSyncHistory::loadChecksums(BChunkReader *reader)
{
   long result;
   CHUNKREADSAFE(reader, DWORD, mHistoryChecksum);

   BSyncUpdate* update=getCurrentUpdate();

   long numSyncDefines=0;
   CHUNKREADSAFE(reader, Long, numSyncDefines);

   for (int i = 0; i < numSyncDefines; i++)
   {
      DWORD checksum;
      CHUNKREADSAFE(reader, DWORD, checksum);

      if(i<cNumberOfSyncDefines && update)
         update->setChecksum(i, checksum);
   }

   return true;
}

//==============================================================================
// BSyncHistory::saveCallTable
//==============================================================================
bool BSyncHistory::saveCallTable(BChunkWriter *writer)
{
   long result;
   long count = mSyncCallTable.getNumber();
   CHUNKWRITESAFE(writer, Long, count);

   for (long idx=0; idx<count; idx++)
   {
      if (!mSyncCallTable[idx]->save(writer))
         return false;
   }

   return true;
}

//==============================================================================
// BSyncHistory::loadCallTable
//==============================================================================
bool BSyncHistory::loadCallTable(BChunkReader *reader)
{
   BFATAL_ASSERT(mSyncCallTable.getNumber() == 0); // you really shouldn't be loading over the history unless you know what you are doing. 
   // And if you do, then you can fix the memory leak that is about to happen.

   long result;
   long count = 0;
   CHUNKREADSAFE(reader, Long, count);

   mbSyncCallAllocated = true;

   mSyncCallTable.setNumber(count);
   for (long idx=0; idx<count; idx++)
   {
      mSyncCallTable[idx] = HEAP_NEW(BSyncCallEntry, gSyncHeap);
      if (!mSyncCallTable[idx]->load(reader))
         return false;
   }

   return true;
}

//==============================================================================
// BSyncHistory::getSyncCallEntry
//==============================================================================
const BSyncCallEntry* BSyncHistory::getSyncCallEntry(WORD wIndex)
{
   if (wIndex >= mSyncCallTable.getNumber())
   {
      BFAIL("BSyncHistory::getSyncCallEntry -- invalid index.");
      return(NULL);
   }

   return mSyncCallTable[wIndex];
}

//==============================================================================
// BSyncHistory::assignSyncCallIndex
//==============================================================================
void BSyncHistory::assignSyncCallIndex(BSyncCallEntry* pEntry)
{
   long index = mSyncCallTable.add(pEntry);
   pEntry->mwIndex = (WORD)index;
}

//==============================================================================
// 
//==============================================================================
void BSyncHistory::logStats()
{
   // grab the current update
   // log the number of values/strings used
#ifndef BUILD_FINAL
   BSyncUpdate* pUpdate = getCurrentUpdate();
   if (pUpdate != NULL)
      pUpdate->logStats();
#endif
}

//==============================================================================
//==============================================================================
// BSyncManager class Methods
//==============================================================================
//==============================================================================

//==============================================================================
// BSyncManager::BSyncManager
//==============================================================================
BSyncManager::BSyncManager() :
   mChecksumHistory(0, BPLIST_GROW_EXPONENTIAL, &gSyncHeap),
   mpSyncedObject(NULL),
   mpCurrentHistory(NULL), 
   mpPlaybackHistory(NULL), 
   mpCurrentUpdate(NULL),
   mpSyncBufferAllocator(NULL),
   mpSyncFIFO(NULL),
   mpSyncThread(NULL),
   mAIUpdatingCount(0),
   mSyncCounter(0),
   mLoadedNumberOfSyncDefines(0),
   mSyncUpdateInterval(cDefaultSyncUpdateInterval),
   mOOSAfterUpdate(-1),
   mSyncing(false),
   mAIUpdatingSaveSyncing(false),
   mSerialized(false),
   mSyncTagControl(false),
   mLogStats(false),
   mFull1v1(false)
{
}

//==============================================================================
// BSyncManager::~BSyncManager
//==============================================================================
BSyncManager::~BSyncManager()
{
}

//==============================================================================
// BSyncManager::setup
//==============================================================================
bool BSyncManager::setup()
{
#ifndef BUILD_FINAL
   mpSyncBufferAllocator = HEAP_NEW(BSyncBufferAllocator, gSyncHeap);
   mpSyncFIFO = HEAP_NEW(BSyncFIFO, gSyncHeap);

   mpSyncThread = HEAP_NEW(BThread, gSyncHeap);
   if (!mpSyncThread->createThread(syncThreadCallback, this, 0, false))
      return false;
   mpSyncThread->setThreadProcessor(5);
#endif

   long amountOfUpdatesInHistory = cDefaultHistorySize;

   if (gConfig.isDefined("syncHistorySize"))   
   {
      gConfig.get("syncHistorySize", &amountOfUpdatesInHistory);
   }

   mpCurrentHistory = HEAP_NEW_INIT(BSyncHistory, gSyncHeap, amountOfUpdatesInHistory);
   if (!mpCurrentHistory)
      return false;
   mpCurrentHistory->init(0, mpSyncBufferAllocator, mpSyncFIFO);

#ifndef BUILD_FINAL
   mLogStats = gConfig.isDefined("SyncManagerLogStats");
#endif

   reset();

#ifdef SYNC_Anim
   BGrannyInstance::setUpdateSyncCallback(BObject::updateGrannyInstanceSyncCallback);
   BVisualItem::setUpdateSyncCallback(BObject::updateVisualItemSyncCallback);
#endif

   return true;
}

//==============================================================================
// BSyncManager::shutdown
//==============================================================================
void BSyncManager::shutdown()
{
   mSyncThreadExitEvent.set();

   if (mpSyncThread != NULL)
   {
      mpSyncThread->waitForThread();
      HEAP_DELETE(mpSyncThread, gSyncHeap);
      mpSyncThread = NULL;
   }

   if (mpCurrentHistory)
   {
      HEAP_DELETE(mpCurrentHistory, gSyncHeap);
      mpCurrentHistory = NULL;
   }

   if (mpPlaybackHistory)
   {
      HEAP_DELETE(mpPlaybackHistory, gSyncHeap);
      mpPlaybackHistory = NULL;
   }

   destroyChecksumHistory();

   if (mpSyncBufferAllocator != NULL)
   {
      HEAP_DELETE(mpSyncBufferAllocator, gSyncHeap);
      mpSyncBufferAllocator = NULL;
   }

   if (mpSyncFIFO != NULL)
   {
      HEAP_DELETE(mpSyncFIFO, gSyncHeap);
      mpSyncFIFO = NULL;
   }
}

//==============================================================================
// BSyncManager::reset
//==============================================================================
void BSyncManager::reset()
{
   blog("BSyncManager::reset");

   mpSyncedObject = NULL;

   mAIUpdatingCount=0;
   mAIUpdatingSaveSyncing=false;

   if (mpCurrentHistory)
      mpCurrentHistory->clear();

   if (mpPlaybackHistory)
      mpPlaybackHistory->clear();

   // mSyncing?
   destroyChecksumHistory();

   mSyncCounter = 0;
   setSerialized(false);

   mOOSAfterUpdate=-1;

   if (mpCurrentHistory)
      mpCurrentUpdate = mpCurrentHistory->getCurrentUpdate();

   // mSyncTagControl -- not used?
   // mLoadedNumberSyncDefines -- not used?
   // FIXME: if we want to make it so you can rev sync tags before you start the game, then reset these values

#ifdef BUILD_FINAL
   for (long i=0;i<cNumberOfSyncDefines;i++)
      syncDefines[i].mState = cDisabledSyncState;
   if (gConfig.isDefined(cConfigAlpha))
      syncDefines[cFinalReleaseSync].mState = cFullSyncState; // need full syncing on for the alpha
   else
      syncDefines[cFinalReleaseSync].mState = cXORSyncState; // no matter what, this sync must be on in final release builds
   syncDefines[cFinalDetailSync].mState = cXORSyncState; // no matter what, this sync must be on in final release builds
   //gSyncRecord = gConfig.isDefined("finalReleaseRecordSync");
#else
   bool fullstate=false;
   BSimString foo;

   for (long i=0;i<cNumberOfSyncDefines;i++)
   {
      if (gConfig.isDefined(cConfigFullAllSync))
      {
         syncDefines[i].mState = cFullSyncState;
         fullstate=true;
      }
      else if (gConfig.isDefined(cConfigXORAllSync))
         syncDefines[i].mState = cXORSyncState; 
      else if (gConfig.isDefined(cConfigDisableAllSync))
         syncDefines[i].mState = cDisabledSyncState;

      foo = B("");
      gConfig.get(cConfigRandSync+i, foo);

      if (!foo.compare(B("disable")))
         syncDefines[i].mState = cDisabledSyncState;
      else if (!foo.compare(B("xor")))
         syncDefines[i].mState = cXORSyncState;
      else if (!foo.compare(B("full")))
      {
         syncDefines[i].mState = cFullSyncState;
         fullstate=true;
      }
      else if (!foo.compare(B("full1v1")))
      {
         syncDefines[i].mState = cFull1v1SyncState;
         fullstate=true;
      }
   }

   // ajl 9/13/02 - Sync every turn if any tag is set to "full".
   if (fullstate)
      mSyncUpdateInterval=1;
   else
   {
      if (!gConfig.get(cConfigSyncUpdateInterval, &mSyncUpdateInterval))
         mSyncUpdateInterval = cDefaultSyncUpdateInterval;
   }

   //gSyncRecord = gConfig.isDefined(cConfigSyncRecord);
#endif
}


//==============================================================================
// BSyncManager::save
//==============================================================================
bool BSyncManager::save(BChunkWriter* writer)
{
   writer;
   return true;
}


//==============================================================================
// BSyncManager::load
//==============================================================================
bool BSyncManager::load(BChunkReader* reader)
{
   reader;
   return true;
}

//==============================================================================
// BSyncManager::activatePlaybackHistory
//==============================================================================
void BSyncManager::activatePlaybackHistory()
{
   long amountOfUpdatesInHistory = cDefaultHistorySize;

   if (gConfig.isDefined("syncHistorySize"))   
      gConfig.get("syncHistorySize", &amountOfUpdatesInHistory);

   if (mpPlaybackHistory == NULL)
   {
      mpPlaybackHistory = HEAP_NEW_INIT(BSyncHistory, gSyncHeap, amountOfUpdatesInHistory);
      BASSERT(mpPlaybackHistory);
      mpPlaybackHistory->init(1, mpSyncBufferAllocator, mpSyncFIFO);
   }
   else
      mpPlaybackHistory->clear();
}

//==============================================================================
// BSyncManager::deactivatePlaybackHistory
//==============================================================================
void BSyncManager::deactivatePlaybackHistory()
{
   if (mpPlaybackHistory != NULL)
   {
      HEAP_DELETE(mpPlaybackHistory, gSyncHeap);
      mpPlaybackHistory = NULL;
   }
}

//==============================================================================
// BSyncManager::checksumSyncStates
//   Returns a CRC indicating which Sync Systems are set to what
//   Used to compare at the start of a game that things are set the same
//   After the new sync system was installed, this is kinda pointless...
//==============================================================================
DWORD BSyncManager::checksumSyncStates()
{
   DWORD checksum=0;

   for (long i = 0; i < cNumberOfSyncDefines; i++)
   {
      Crc324Bytes(checksum, (DWORD*) &syncDefines[i].mState);
   }

   return(checksum);
}


//==============================================================================
// BSyncManager::getCurrentChecksum
//==============================================================================
DWORD BSyncManager::getCurrentChecksum(long tag)
{
   if(mpCurrentHistory)
   {
      BSyncUpdate* update=mpCurrentHistory->getCurrentUpdate();
      if(update)
         return update->getChecksum(tag);
   }
   return 0;
}

//==============================================================================
// BSyncManager::destroyChecksumHistory
//==============================================================================
void BSyncManager::destroyChecksumHistory()
{
   BHandle hItem;

   BChecksumHistory* pChecksum = mChecksumHistory.getHead(hItem);
   while (pChecksum)
   {
      BChecksumHistory::releaseInstance(pChecksum);

      pChecksum = mChecksumHistory.getNext(hItem);
   }
   mChecksumHistory.reset();

   BChecksumHistory::mFreeList.clear();
}

//==============================================================================
// Calls outOfSyncInternal to write out the sync log before popping the ASSERT
// What was happening was people were rebooting their xboxes without
//    continuing past the assert so the log would never get written
//==============================================================================
void BSyncManager::outOfSync(long oosUpdateNumber)
{
   if (gConfig.isDefined(cConfigDontGoOOS))
      return;

   if (BSyncManager::getInstance()->isSerialized())
      return;

   outOfSyncInternal(oosUpdateNumber);

   BASSERTM(0, "Out of sync");
}

//==============================================================================
// 
//==============================================================================
void BSyncManager::outOfSyncInternal(long oosUpdateNumber)
{
   if (!BSyncManager::getInstance()->isSerialized())
   {  
      BSyncManager::getInstance()->setSerialized(true);

      if (gConfig.isDefined(cConfigDontGoOOS))
         return;

#ifndef BUILD_FINAL
      uint y = 0;
      BDebugText::renderRaw(0, y++, "Out of sync - writing out sync log, please stand by");
#endif

      BCommLog::dumpHistory();

      nlog(cSyncCL, "Out of sync");

//-- FIXING PREFIX BUG ID 3725
      const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
      if (!pUser)
         return;

      SYSTEMTIME t;
      GetLocalTime(&t);

      char syncLogName[512];

      if (gConfig.isDefined(cConfigLogToCacheDrive)
#ifndef BUILD_FINAL
         || !gXFS.isActive()
#else
         || true
#endif
         )
      {
         // we also want to write out the current time
         // and the name of all the players in the game
         ushort id;
         XNetRandom((BYTE*)&id, sizeof(id));
         sprintf_s(syncLogName, sizeof(syncLogName), "synclog_%04X_%04d%02d%02dT%02d%02d%02d.txt", id, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
      }
#ifndef BUILD_FINAL
      else if (gXFS.isActive())
      {
         BSimString logName;
         logName.format("synclog-%s", pUser->getName().getPtr());
         gXFS.setFailReportFileName(logName.getPtr(), syncLogName, sizeof(syncLogName));
      }
#endif

#ifndef BUILD_FINAL
      BDebugText::renderRaw(0, y++, "* Opening sync log");
#endif

      long logFile = gLogManager.openLogFile(syncLogName, BLogManager::cPostWriteNone, false, 0, true, false);
      if (logFile < 0)
      {
         BFAIL("BSyncManager::outOfSync -- failed to open sync log for writing. Check disk space or permissions.");
         return;
      }

#ifndef BUILD_FINAL
      BDebugText::renderRaw(0, y++, "* Serializing history - this may take a minute");
#endif

      gLogManager.setLineNumbering(logFile, false);
      gLogManager.setTitleStamp(logFile, false);
      long syncLogHeaderID = gLogManager.createHeader("SYNC", logFile, BLogManager::cBaseHeader, false, false, false, false, false);
      BSyncManager::getInstance()->getCurrentHistory()->serializeToLog(syncLogHeaderID, oosUpdateNumber);
      if (syncLogHeaderID != -1)
      {
         finalBlogh(syncLogHeaderID, "=====================================================================================");
         finalBlogh(syncLogHeaderID, "The following is not sync'd data.  This is a dump of the checksum history");
         finalBlogh(syncLogHeaderID, "=====================================================================================");

         BHandle hItem;
         BChecksumHistory* pChecksum = mChecksumHistory.getHead(hItem);
         while (pChecksum)
         {
            finalBlogh(syncLogHeaderID, "CHECKSUM: id[%d] syncedPlayers[%d] fromClientID[%d]", pChecksum->mID, pChecksum->mRequiredSyncedPlayers, pChecksum->mFromClientID);
            pChecksum = mChecksumHistory.getNext(hItem);
         }
      }
      gLogManager.flushLogFile(logFile);
      gLogManager.closeLogFile(logFile);

      BSyncManager::getInstance()->writeToBinaryFile(oosUpdateNumber);

      if (BSyncManager::getInstance()->getPlaybackHistory() && gRecordGame.isPlaying())
      {
#ifndef BUILD_FINAL
         BDebugText::renderRaw(0, y++, "* Saving old sync log");
#endif
         char syncOldName[512];

         // if we've specified LogToCacheDrive or XFS is inactive
         if (gConfig.isDefined(cConfigLogToCacheDrive)
#ifndef BUILD_FINAL
            || !gXFS.isActive()
#else
            || true
#endif
            )
         {
            // we also want to write out the current time
            // and the name of all the players in the game
            sprintf_s(syncOldName, sizeof(syncOldName), "syncold_%04d%02d%02dT%02d%02d%02d.txt", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
         }
#ifndef BUILD_FINAL
         else if (gXFS.isActive())
         {
            BSimString logName;
            logName.format("syncold-%s", pUser->getName().getPtr());
            gXFS.setFailReportFileName(logName.getPtr(), syncOldName, sizeof(syncOldName));
         }
#endif

#ifndef BUILD_FINAL
         BDebugText::renderRaw(0, y++, "* Opening old sync log");
#endif

         long logFile2 = gLogManager.openLogFile(syncOldName, BLogManager::cPostWriteNone, false, 0, true, false);
         if (logFile2 < 0)
         {
            BFAIL("BSyncManager::outOfSync -- failed to open sync old file for writing. Check disk space or permissions.");
            return;
         }
#ifndef BUILD_FINAL
         BDebugText::renderRaw(0, y++, "* Serializing playback history - this may take a minute");
#endif
         gLogManager.setLineNumbering(logFile2, false);
         gLogManager.setTitleStamp(logFile2, false);
         BSyncManager::getInstance()->getPlaybackHistory()->serializeToLog(gLogManager.createHeader("SYNCOLD", logFile2, BLogManager::cBaseHeader, false, false, false, false, false), oosUpdateNumber);
         gLogManager.flushLogFile(logFile2);
         gLogManager.closeLogFile(logFile2);
      }

      // this closes existing log files and opens new ones under their old names - bad
      // the previous flush/close statements should be sufficient
      //gLogManager.flushAllLogs();

      #ifndef BUILD_FINAL
      {
         char saveName[512];
         sprintf_s(saveName, sizeof(saveName), "syncsave_%.10s_%.04d%.02d%.02dT%.02d%.02d%.02d", pUser->getName().getPtr(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

         gSaveGame.saveGame(saveName);
         gSaveGame.doSaveGameBegin();
         gSaveGame.doSaveGameEnd();
      }
      #endif

      /*
      game->copyLogsUp();
      game->writeFinalStats("oos");
      if (!game->isLoadingOrSaving())
      {
         game->save(cDirUWSavegame, B("oossave"), false);
         if(!isReplay)
            game->copySavegameUp("oossave");
      }

      if (gConsole)
         gConsole->executeNextFrame("dropToMainMenu");
      */

      setSyncing(false);
#ifndef BUILD_FINAL
      BDebugText::renderRaw(0, y++, "* Complete");
#endif
   }
}

//==============================================================================
// BSyncManager::setSyncedObject
//==============================================================================
void BSyncManager::setSyncedObject(BMPSyncObject *object)
{
   mpSyncedObject = object;
   if (mpSyncedObject)
      mpSyncedObject->attachSyncNotify(this);
}

//==============================================================================
// BSyncManager::nextUpdate
//==============================================================================
void BSyncManager::nextUpdate()
{
   SCOPEDSAMPLE(BSyncManager_NextUpdate)

   //long currentSize, pastSize;
   //currentSize = 0; pastSize = 0;

   if (gLiveSystem->isMultiplayerGameActive())
      commChecksum();
   
   /*
   if (game->getRecordGame() && gSyncRecord && !game->getRecordGame()->getOOS() && getPlaybackHistory() && game->getRecordGame()->getState() == BRecordGame::cStatePlayback && !game->getRecordGame()->getDoneReplaying())
   {    
      // check that update sizes match
      bool checkSizes=true;
      if(game->getRecordGame()->getSkipNextSync())
      {
         checkSizes=false;
         game->getRecordGame()->setSkipNextSync(false);
      }
      currentSize  = getCurrentHistory()->getCurrentUpdate()->getCount();
      pastSize     = getPlaybackHistory()->getCurrentUpdate()->getCount();
      if (checkSizes && currentSize != pastSize)
      {
         outOfSync();           
      }
      else
      {
         // check that tag checksums match
         for (long i=0; i<cNumberOfSyncDefines; i++)
         {
            if (syncDefines[i].mState == cXORSyncState)
            {

               DWORD currentChecksum = getCurrentHistory()->getCurrentUpdate()->getChecksum(i);
               DWORD pastChecksum    = getPlaybackHistory()->getCurrentUpdate()->getChecksum(i);
               if (currentChecksum != pastChecksum)
               {
                  outOfSync();
                  break;
               }
            }
         }      
      }
   }
   */

   if (mpCurrentHistory)
   {
      if(gRecordGame.isRecording())
         gRecordGame.recordSync();

#ifndef BUILD_FINAL
      if (mLogStats)
         mpCurrentHistory->logStats();
#endif
      mpCurrentHistory->nextUpdate();
   }

   if (mpPlaybackHistory)
   {
#ifndef BUILD_FINAL
      if (mLogStats)
         mpPlaybackHistory->logStats();
#endif
      mpPlaybackHistory->nextUpdate();
      if(gRecordGame.isPlaying())
         gRecordGame.playSync();
   }

   if (mpCurrentHistory)
   {
      //mpCurrentHistory->setupUpdateChecksums();   
      mpCurrentUpdate = mpCurrentHistory->getCurrentUpdate();
   }

   nlog(cSyncCL, "nextUpdate - updateNumber %ld", getCurrentHistory()->getCurrentUpdate()->getUpdateNumber());   
}

//==============================================================================
// BSyncManager::commChecksum
//==============================================================================
void BSyncManager::commChecksum()
{
   // If our client is gone, don't send out any sync data.
   if (!mpSyncedObject)
      return;

   if (!(mSyncCounter%mSyncUpdateInterval))
   {
      syncCommData("sending sync", mpCurrentHistory->getCurrentUpdate()->getUpdateNumber());
      syncCommData("  syncupdateinterval", mSyncUpdateInterval);
      syncCommData("  mpCurrentHistory->getAmountOfUpdates()", mpCurrentHistory->getAmountOfUpdates());

      DWORD newCRC = mpCurrentHistory->getCurrentChecksum();

      nlog(cLowLevelSyncCL, "  SYNC BLOCK");
      nlog(cLowLevelSyncCL, "    0x%08x", newCRC);
      syncCommData("  syncBlock", newCRC);
      syncCommData("  mpCurrentHistory->getUpdateNumber()", mpCurrentHistory->getCurrentUpdate()->getUpdateNumber());

      nlog(cSyncCL, "Sending out sync - sync[0x%08x] ID[%ld] clientAmount[%ld]", newCRC, mSyncCounter, mpSyncedObject->getSyncedCount());
      nlog(cSyncCL, "  mpCurrentHistory->getUpdateNumber() %ld", mpCurrentHistory->getCurrentUpdate()->getUpdateNumber());

      // send out the sync checksum
      mpSyncedObject->sendSyncData(mpCurrentHistory->getCurrentUpdate()->getUpdateNumber(), newCRC);
   }

   mSyncCounter++;
}

//==============================================================================
// BSyncManager::addSyncedPlayer
//==============================================================================
void BSyncManager::addSyncedPlayer()
{      
   mpCurrentHistory->clear();
}

//==============================================================================
// BSyncManager::removeSyncedPlayer
//==============================================================================
void BSyncManager::removeSyncedPlayer(long id)
{
   id;
   // iterate over history, removing one player from each
   BHandle hItem;
   BChecksumHistory* pChecksum = mChecksumHistory.getHead(hItem);
   while (pChecksum)
   {
      pChecksum->mRequiredSyncedPlayers--;

      pChecksum = mChecksumHistory.getNext(hItem);
   }
}

//==============================================================================
// 
//==============================================================================
bool BSyncManager::incomingSyncData(long fromID, long checksumID, uint checksum)
{   
   nlog(cSyncCL, "Incomming sync data - fromClient %ld", fromID);

   bool checksumFound = false;

   // find history info with matching uid, compare the checksum and decrement the players
   // itterate over history, removing one player from each
   BHandle hItem;
   BChecksumHistory* pHistory = mChecksumHistory.getHead(hItem);
   while (pHistory)
   {
      if (pHistory->mID == checksumID)
      {
         nlog(cSyncCL, "Attempting to sync ID %ld from client %ld ", checksumID, fromID);

         checksumFound = true;
         pHistory->mRequiredSyncedPlayers--;
         nlog(cSyncCL, "Syncing ID[%ld] sync[0x%08x] against sync[0x%08x] it has %ld players left", checksumID, checksum, pHistory->mChecksum, pHistory->mRequiredSyncedPlayers);
         if (!mSerialized && checksum != pHistory->mChecksum)
         {
            nlog(cSyncCL, "Out of sync on checksumID %ld, client %ld went out with client %ld", checksumID, fromID, pHistory->mFromClientID);

            nlog(cSyncCL, "Out of sync ----------------------");

            nlog(cSyncCL, "SYNC BLOCK");
            nlog(cSyncCL, "  0x%08x : 0x%08x", checksum, pHistory->mChecksum);

            gStatsManager.setOoS(fromID, pHistory->mFromClientID, checksumID);

            outOfSync(checksumID);

            BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
            if (pUser)
               pUser->endGameOOS();
         }
         if (pHistory->mRequiredSyncedPlayers <= 0)
         {
            nlog(cSyncCL, "mRequiredSyncedPlayers %ld <= 0 for ID %ld - erasing from mChecksumHistory", pHistory->mRequiredSyncedPlayers, checksumID);
            BChecksumHistory::releaseInstance(pHistory);
            pHistory = mChecksumHistory.removeAndGetNext(hItem);
         }
         else
            pHistory = mChecksumHistory.getNext(hItem);
      }
      else
         pHistory = mChecksumHistory.getNext(hItem);
   }

   // if we don't have a matching sync page, stuff it into the history 
   if (!checksumFound && mpSyncedObject != NULL)
   {  
      long remaining = mpSyncedObject->getSyncedCount()-1;

      // if we have no more active players, then don't bother pushing this data onto the history
      if (remaining == 0)
         return true;

      nlog(cSyncCL, "Stuffing sync ID %ld sync[0x%08x] from client %ld into history, it has %ld players left", checksumID, checksum, fromID, remaining);

      BChecksumHistory* pHistory = BChecksumHistory::getInstance();
      pHistory->init(checksumID, checksum, remaining, fromID);
      mChecksumHistory.addToTail(pHistory);
   }
   return(true);
}

//==============================================================================
// 
//==============================================================================
void BSyncManager::writeToBinaryFile(long /*lOOSUpdate*/)
{
}

//==============================================================================
// 
//==============================================================================
void BSyncManager::serializeToLog()
{ 
//-- FIXING PREFIX BUG ID 3726
   const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
   if (!pUser)
      return;

   SYSTEMTIME t;
   GetLocalTime(&t);

   char syncLogName[512];

   // if LogToCacheDrive is defined or XFS is disabled
   if (gConfig.isDefined(cConfigLogToCacheDrive)
#ifndef BUILD_FINAL
      || !gXFS.isActive()
#else
      || true
#endif
      )
   {
      // we also want to write out the current time
      // and the name of all the players in the game
      sprintf_s(syncLogName, sizeof(syncLogName), "synclog_%04d%02d%02dT%02d%02d%02d.txt", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
   }
#ifndef BUILD_FINAL
   else if (gXFS.isActive())
   {
      BSimString logName;
      logName.format("synclog-%s", pUser->getName().getPtr());
      gXFS.setFailReportFileName(logName.getPtr(), syncLogName, sizeof(syncLogName));
   }
#endif

   long logFile = gLogManager.openLogFile(syncLogName);

   getCurrentHistory()->serializeToLog(gLogManager.createHeader("SYNC", logFile, BLogManager::cBaseHeader, false, false, false, false, false)); 
   gLogManager.flushLogFile(logFile);
   gLogManager.closeLogFile(logFile);
}

//==============================================================================
// BSyncManager::setAIUpdating
//==============================================================================
void BSyncManager::setAIUpdating(bool val)
{
   if(val)
   {
      if(mAIUpdatingCount==0)
      {
         mAIUpdatingSaveSyncing=mSyncing;
         mSyncing=false;
      }
      mAIUpdatingCount++;
   }
   else
   {
      if(mAIUpdatingCount==0)
      {
         BASSERT(0);
      }
      else
      {
         mAIUpdatingCount--;
         if(mAIUpdatingCount==0)
         {
            mSyncing=mAIUpdatingSaveSyncing;
            mAIUpdatingSaveSyncing=false;
         }
      }
   }
}

//==============================================================================
// BSyncManager::shouldSync
//==============================================================================
bool BSyncManager::shouldSync(long tag)
{
   if(mSyncing)
      return true;

   if(mAIUpdatingSaveSyncing && tag==cRandSync)
   {
      // The AI is updating and its random numbers are not synced.
      // So this must be a sim random number that it's not supposed
      // to be using.
      //BASSERT(0);
      return true;
   }

   return false;
}

//==============================================================================
// 
//==============================================================================
void* BSyncManager::syncThread()
{
   for (;;)
   {
      const BSyncFIFO::Element_Type* ppBuf = NULL;
      BSyncFIFO::eGetPtrStatus status = mpSyncFIFO->getFrontPtr(ppBuf, mSyncThreadExitEvent, INFINITE);

      if (BSyncFIFO::eHandleSignaled == status)
         break;
      else if (BSyncFIFO::eAcquiredPtr != status)
         continue;

      const BSyncEvent* pEvent = ppBuf;

      BDEBUG_ASSERT(pEvent);

      if (pEvent == NULL)
         continue;

      if (pEvent->mpStream != NULL)
      {
         if (pEvent->mpBuf != NULL && (pEvent->mpBuf)->mBufSize > 0)
            (pEvent->mpStream)->writeBytes((pEvent->mpBuf)->mBuf, (pEvent->mpBuf)->mBufSize);

         if (pEvent->mSeek)
         {
            (pEvent->mpStream)->seek(0);

            if (pEvent->mHandle != INVALID_HANDLE_VALUE)
               SetEvent(pEvent->mHandle);
         }
      }

      if (pEvent->mpBuf != NULL)
      {
         if ((pEvent->mpBuf)->mDynamic)
            HEAP_DELETE(pEvent->mpBuf, gSyncHeap);
         else
            mpSyncBufferAllocator->free(pEvent->mpBuf);
      }

      mpSyncFIFO->popFront();
   }

   return 0;
}

//==============================================================================
// 
//==============================================================================
void* _cdecl BSyncManager::syncThreadCallback(void* pVal)
{
   pVal;
   SetThreadName(GetCurrentThreadId(), "SyncThread");

   return static_cast<BSyncManager*>(pVal)->syncThread();
}

//==============================================================================
// 
//==============================================================================
BChecksumHistory::BChecksumHistory() : 
   mID(0),
   mChecksum(0),
   mRequiredSyncedPlayers(0),
   mFromClientID(-1)
{
}

//==============================================================================
// 
//==============================================================================
//BChecksumHistory::BChecksumHistory(long ID, void *checksum, long checksumSize, long syncedPlayers, long fromClientID) : 
//   mID(ID),
//   mChecksum(checksum),
//   mChecksumSize(checksumSize), 
//   mRequiredSyncedPlayers(syncedPlayers),
//   mFromClientID(fromClientID)
//{
//}

//==============================================================================
// 
//==============================================================================
void BChecksumHistory::init(long ID, uint checksum, long syncedPlayers, long fromClientID)
{
   mID = ID;
   mChecksum = checksum;
   mRequiredSyncedPlayers = syncedPlayers;
   mFromClientID = fromClientID;
}

//==============================================================================
// 
//==============================================================================
void BChecksumHistory::onAcquire()
{
   mID = 0;
   mChecksum = 0;
   mRequiredSyncedPlayers = 0;
   mFromClientID = -1;
}

//==============================================================================
// BSyncStrLog::BSyncStrLog
// This is some crazy shit left over from Byron and used in the binary writes
// that I don't currently use.
//==============================================================================
BSyncStrLog::BSyncStrLog() : 
mpFirst(0),
mpLast(0),
mdwCount(0)
{
}

BSyncStrLog::~BSyncStrLog()
{
   Node* pNode = mpFirst;
   while (pNode)
   {
      mpFirst = pNode->pNext;
      HEAP_DELETE(pNode, gSyncHeap);
      pNode = mpFirst;
   }
}

DWORD BSyncStrLog::insertNode(Node* pNode)
{
   pNode->info.dwIndex = mdwCount++;
   if (!mpLast)
   {
      mpLast  = pNode;
      mpFirst = pNode;
      pNode->pNext = 0;
   }
   else
   {
      mpLast->pNext = pNode;
      mpLast = mpLast->pNext;
   }
   return pNode->info.dwIndex;
}

DWORD BSyncStrLog::insert(float f)
{
   Node* pNode = HEAP_NEW(Node, gSyncHeap);
   bsnprintf(pNode->info.szValue, cMaxStrLen, "%.24f", f);
   return insertNode(pNode);
}  

DWORD BSyncStrLog::insert(char* s)
{
   Node* pNode = HEAP_NEW(Node, gSyncHeap);
   StringCchCopyNExA(pNode->info.szValue, cMaxStrLen, s, cMaxStrLen-1, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
   return insertNode(pNode);
}

DWORD BSyncStrLog::insert(const char* s)
{
   Node* pNode = HEAP_NEW(Node, gSyncHeap);
   StringCchCopyNExA(pNode->info.szValue, cMaxStrLen, s, cMaxStrLen, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
   return insertNode(pNode);
}

void BSyncStrLog::writeToBinaryFile(BFile& file)
{
   Node* pNode = mpFirst;
   while (pNode)
   {
      file.write(&pNode->info, sizeof(pNode->info));
      pNode = pNode->pNext;
   }
}

DWORD BSyncStrLog::getCount() const
{
   return mdwCount;
}
