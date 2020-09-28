//==============================================================================
// SyncManager.h
//
// Copyright (c) 1999-2008 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes

#include "syncdefines.h"
//#include "..\XNetwork\tlist.h"
//#include "memory/allocfixed.h"
#include "mpsync.h"
#include "Etl/singleton.hpp"

// xsystem
#include "poolable.h"
#include "Thread.h"

// xcore
#include "stream\byteStream.h"
#include "threading\synchronizedBlockAllocator.h"
#include "threading\commandFIFO.h"

//==============================================================================
struct BDynamicArraySyncHeapAllocatorPolicy { BMemoryHeap& getHeap(void) const { return gSyncHeap; } };

template<class ValueType, uint Alignment>
struct BDynamicArraySyncHeapAllocator      : BDynamicArrayFixedHeapAllocator<ValueType, Alignment, BDynamicArraySyncHeapAllocatorPolicy> { };

template<
   class ValueType, 
      uint Alignment                               = ALIGN_OF(ValueType), 
      template <class, uint>  class OptionsPolicy  = BDynamicArrayDefaultOptions,
      template <class, uint>  class PointerPolicy  = BDynamicArraySeparatePointers
>
class BDynamicSyncArray : public BDynamicArray<ValueType, Alignment, BDynamicArraySyncHeapAllocator, OptionsPolicy, PointerPolicy>
{
public:
   explicit BDynamicSyncArray(uint initialSize = 0, uint initialCapacity = 0) : BDynamicArray(initialSize, initialCapacity)
   {
   }
};

//==============================================================================

void CalculateCRC32(DWORD &theCRC, const BYTE* DataBuffer, long DataLen);
void Crc321Byte(DWORD &theCRC, BYTE theByte);
void Crc324Bytes(DWORD &theCRC, const DWORD *theDWORD);


//==============================================================================
// Forward Defines

class BChunkWriter;
class BChunkReader;
class BSyncSymbolTable;
class BStringSpace;
class BClient;

class BBufferStream;
class BWin32FileStream;
class BDeflateStream;
class BInflateStream;

// External Globals we need to know about

//extern bool gSyncRecord;
extern BSyncDefine syncDefines[];


class BSyncManager;
class BSyncCallEntry;
struct BSyncLogHeader;
class BSyncStrLog;


//==============================================================================
struct BSyncBuffer
{
   enum { cBufSize = 131072 };
   uchar    mBuf[cBufSize];
   uint     mBufSize;
   BOOL     mDynamic;
};

//==============================================================================
struct BSyncEvent
{
   BSyncBuffer* mpBuf;
   BStream*     mpStream; // the file stream that we'll be writing to in a different thread
   HANDLE       mHandle;
   bool         mSeek : 1;
};

//==============================================================================
typedef BSynchronizedBlockAllocator<BSyncBuffer, 16, false> BSyncBufferAllocator;
typedef BCommandFIFO<BSyncEvent, 64> BSyncFIFO;

//==============================================================================
class BSyncStream : public BByteStream
{
   public:
      BSyncStream();

      void init(BSyncBufferAllocator* pAllocator, BStream* pDstStream, BSyncFIFO* pFIFO, BOOL allowDynamicAlloc);

      virtual uint writeBytes(const void* p, uint n);

      virtual bool flush();
      virtual bool close();

      void flushWait();

   private:

      void allocBuffer();
      void submitBuffer();

      BSyncBufferAllocator*   mpAllocator;
      BSyncBuffer*            mpCurrentBuffer;
      BStream*                mpStream;
      BSyncFIFO*              mpSyncFIFO;
      BWin32Event             mFlushEvent;
      BOOL                    mAllowDynamicAlloc;
};

//==============================================================================
class BSyncStringValue
{
public:
   inline BSyncStringValue() 
   {
      BCOMPILETIMEASSERT(sizeof(BSyncStringValue) == sizeof(uint) * 8);
      
      // Must clear this struct to all 0's because the whole thing is compressed
      uint* pDst = reinterpret_cast<uint*>(this);
      pDst[0] = 0; pDst[1] = 0;
      pDst[2] = 0; pDst[3] = 0;
      pDst[4] = 0; pDst[5] = 0;
      pDst[6] = 0; pDst[7] = 0;
   }

   enum { cMaxStrLen = 31 };

   inline void operator = (const char* p)  
   { 
      if (p) 
      {
         uint len = Math::Min<uint>(cMaxStrLen, strlen(p));
         memcpy(mSTR, p, len);
         mSTR[len] = '\0';
      }
   }
   
   inline uint length() const { return strlen(mSTR); }
   inline const char* getPtr() const { return mSTR; }

private:
   char mSTR[cMaxStrLen + 1];
};

//==============================================================================
// BSyncValue:
//
// A Sync Value consists of the actual value to sync and in index into the 
// sync call table.
//
// Disable alignment to save memory.
#pragma pack(push, 1)
class BSyncValue
{
public:
   BSyncValue()               : mValue(0), mCallIndex(USHRT_MAX) { }
   BSyncValue(long  val)      { mLong = val; }
   BSyncValue(DWORD val)      { mDWORD = val; }
   BSyncValue(float val)      { mFloat = val; }
   BSyncValue(int   val)      { mInt = val; }

   inline void operator = (const long r)   { mLong  = r; }
   inline void operator = (const int r)    { mInt  = r;  }
   inline void operator = (const DWORD r)  { mDWORD = r; }
   inline void operator = (const float r)  { mFloat = r; }
   
   inline bool operator== (const BSyncValue& rhs) const { return memcmp(&mValue, &rhs.mValue, sizeof(mValue)) == 0; }

   bool                       save(BChunkWriter *writer);
   bool                       load(BChunkReader *reader);  
   static bool                writeVersion(BChunkWriter* chunkWriter);
   static bool                readVersion(BChunkReader* chunkReader);
            
   __declspec(property(get = getInt, put = putInt))      int mInt;
   __declspec(property(get = getLong, put = putLong))    long mLong;
   __declspec(property(get = getDWORD, put = putDWORD))  DWORD mDWORD;
   __declspec(property(get = getFloat, put = putFloat))  float mFloat;

   // mValue may not be DWORD aligned!! Avoid reading it directly.
   union
   {
      DWORD mValue;
      long  lValue;
      int   iValue;
      float fValue;
   };

   WORD                       mCallIndex;

   static const DWORD         msSaveVersion;          // Save Data Format Version
   static DWORD               msLoadVersion;          // Loaded Data Stream version format

   //inline int getInt() const { int ret; memcpy(&ret, &mValue, sizeof(ret)); return ret; }
   //inline void putInt(int value) { memcpy(&mValue, &value, sizeof(mValue)); }
   //
   //inline long getLong() const { long ret; memcpy(&ret, &mValue, sizeof(ret)); return ret; }
   //inline void putLong(long value) { memcpy(&mValue, &value, sizeof(mValue)); }
   //
   //inline DWORD getDWORD() const { DWORD ret; memcpy(&ret, &mValue, sizeof(ret)); return ret; }
   //inline void putDWORD(DWORD value) { memcpy(&mValue, &value, sizeof(mValue)); }   
   //
   //inline float getFloat() const { float ret; memcpy(&ret, &mValue, sizeof(ret)); return ret; }
   //inline void putFloat(float value) { memcpy(&mValue, &value, sizeof(mValue)); }   

   inline int getInt() const { return iValue; }
   inline void putInt(int value) { iValue = value; }

   inline long getLong() const { return lValue; }
   inline void putLong(long value) { lValue = value; }

   inline DWORD getDWORD() const { return mValue; }
   inline void putDWORD(DWORD value) { mValue = value; }

   inline float getFloat() const { return fValue; }
   inline void putFloat(float value) { fValue = value; }
};
#pragma pack (pop)

//==============================================================================
// BSyncCallEntry:
//
// When a sync macro is executed for the first time, it creates a sync call entry
// into a sync call table in the sync manager. This class stores the information
// that is relevant to that call of the macro, such as file name, line number,
// and more importantly the type of values stored. On subsequent executions only 
// new values are added and linked to this single sync call entry.

class BSyncCallEntry
{
public:
   BSyncCallEntry() {}
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, long v);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, int v);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, DWORD v);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, float v);
   //BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, WORD v);
   //BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, SHORT v);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, LPSTR);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, LPCSTR);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, BVector& v);
   BSyncCallEntry(const char* lpszDescription, const char* lpszFile, WORD wLine, BYTE bTag, const BVector& v);

   bool                       save(BChunkWriter *writer);
   bool                       load(BChunkReader *reader);
   void                       clearAllocated();

   enum Type 
   {
      cVOID = 0,
      cINT,
      cLONG,
      cDWORD,
      cFLOAT,
      cWORD,
      cSHORT,
      cSTRING,
      cUnknown
   };

   char*       mlpszDescription;
   char*       mlpszFile;
   WORD        mwLine;
   WORD        mwIndex;
   BYTE        mbTag;
   BYTE        mbType;
};

typedef BDynamicSyncArray<BSyncValue> BSyncValueArray;
typedef BDynamicSyncArray<BSyncStringValue> BSyncStringValueArray;

//==============================================================================
// BSyncUpdate:
//
// A Sync Update consists of all of the values synced for a single update in the 
// game. Strings are stored in a separate array, and referenced by the value array.
class BSyncUpdate
{
   public:      
   
      // Save/Load game version   
   
      static const DWORD         msSaveVersion;          // Save Data Format Version
      static DWORD               msLoadVersion;          // Loaded Data Stream version format

      static bool                writeVersion(BChunkWriter* chunkWriter);
      static bool                readVersion(BChunkReader* chunkReader);

      static void                setLoadVersion(DWORD ver)           {msLoadVersion = ver;};
   
         
      // C/Dtors
      BSyncUpdate();
      ~BSyncUpdate();
      
      BSyncUpdate(const BSyncUpdate& other);
      BSyncUpdate& operator= (const BSyncUpdate& rhs);

      // Functions        
      bool                       save(BChunkWriter *writer);
      bool                       load(BChunkReader *reader);  

      void                       writeChecksumsToBinaryFile(BFile& file, BSyncLogHeader&);
      void                       writeToBinaryFile(BFile& file, BSyncLogHeader&, BSyncStrLog&);

      void                       add(WORD wCallIndex, LPCSTR value);
      void                       add(WORD wCallIndex, LPSTR  value);
      void                       add(WORD wCallIndex, long   value);
      void                       add(WORD wCallIndex, DWORD  value);
      void                       add(WORD wCallIndex, float  value);
      void                       add(WORD wCallIndex, int    value);

      void                       crcData(long tag, long value)    {Crc324Bytes(mTagChecksum[tag], (DWORD*)(&value));}
      void                       crcData(long tag, float value)   {Crc324Bytes(mTagChecksum[tag], (DWORD*)(&value));}
      void                       crcData(long tag, DWORD value)   {Crc324Bytes(mTagChecksum[tag], (DWORD*)(&value));}
      void                       crcData(long tag, int value)     {Crc324Bytes(mTagChecksum[tag], (DWORD*)(&value));}
      void                       crcData(long tag, char byteValue){Crc321Byte(mTagChecksum[tag], (BYTE) byteValue);}
      void                       crcData(long tag, const char* str, long len){CalculateCRC32(mTagChecksum[tag], (BYTE*) str, len);}

      const BSyncValue*          getValue(uint index, bool useStream=true);
      const BSyncStringValue*    getStringValue(uint index);
      uint                       getCount() const { return mValueArraySize; }

      void                       compareAgainstPlaybackHistory(const BSyncValue& currentdata);

      void                       init(BOOL useCacheDrive, BOOL allowDynamicAlloc, uint id, uint fileIndex, BSyncBufferAllocator* pAllocator, BSyncFIFO* pFIFO, uint64 uniqueID);
      void                       clear(long updateNumber=0);
      void                       trim();

      void                       pack(bool waitForCompletion = false, bool repack=false);
      void                       unpack();
      void                       unpackForRead();
      bool                       getIsPacked() const { return mPackedValues.getSize() != 0; }

      long                       getUpdateNumber() const                         { return mUpdateNumber; }

      void                       setChecksum(long tag, DWORD checkVal)           { mTagChecksum[tag] = checkVal; }
      DWORD                      getChecksum(long tag)                           { return (mTagChecksum[tag]); }

      void                       logStats();

   private:
      void                       assign(const BSyncUpdate& other);
      static void                asyncPackFunc(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);
      void                       waitForAsyncPacking(void);

      void allocBuffer();
      void submitBuffer();

      // Function that fills out actual data member of Sync Record
      BDynamicSyncArray<BSyncValue>          mValueArray;
      BDynamicSyncArray<BSyncStringValue>    mStringArray;

      long                                   mUpdateNumber;
      uint                                   mValueArraySize;
      uint                                   mStringArraySize;
      
      BDynamicSyncArray<DWORD>               mPackedValues;
      BDynamicSyncArray<DWORD>               mPackedStrings;

      // the CRC values for each class of Sync Defines
      DWORD                                  mTagChecksum[cNumberOfSyncDefines];

      uint64                                 mUniqueID;

      struct BAsyncPackData
      {
         HANDLE                                 mDoneEventHandle;

         BSyncValueArray*                       mpValueArray;
         BSyncStringValueArray*                 mpStringArray;

         BDynamicSyncArray<DWORD>               mPackedValues;
         BDynamicSyncArray<DWORD>               mPackedStrings;
      };

      BSyncBufferAllocator*                  mpAllocator;
      BSyncFIFO*                             mpSyncFIFO;

      uint                                   mID;
      uint                                   mFileIndex;

      BAsyncPackData*                        mpAsyncPackData;
      BWin32Event                            mAsyncPackDone;

      BStream*                               mpStream;
      BWin32FileStream*                      mpRawStream;

      BSyncStream*                           mpSyncStream;

      BSyncBuffer*                           mpCurrentBuffer;

      BOOL                                   mStreamDirty;
      BOOL                                   mUseCacheDrive;
      BOOL                                   mAllowDynamicAlloc;
};

//==============================================================================
// BSyncHistory:
//
// The sync history keeps up with the last N number of updates while the game is
// running. The incoming syncs are from previous updates, so we have to keep a 
// running history and sync stuff as fast as we can. Typically when we go out of
// sync we try to get the prior and post update to the one that is actually out 
// of sync.
class BSyncHistory
{
   public:
      // C/Dtors
      BSyncHistory(long amountOfUpdatesInHistory);
      ~BSyncHistory();

      enum { cOOSSerializationWindowBefore = 2, cOOSSerializationWindowAfter = 1 }; // amount of updates around where we went out of sync to log

      // id used to differentiate sync cache files on disk
      void                       init(uint id, BSyncBufferAllocator* pAllocator, BSyncFIFO* pFIFO);

      void                       clear(); 

      // Functions        
      void                       nextUpdate();  // move to next update      
      void                       serializeToLog(long logHeaderID, long oosUpdateNumber=-1); // write out history to a log file

      void                       crcData(long tag, long &v)                      { getCurrentUpdate()->crcData(tag, v);  }
      void                       crcData(long tag, float &v)                     { getCurrentUpdate()->crcData(tag, v);  }
      void                       crcData(long tag, DWORD &v)                     { getCurrentUpdate()->crcData(tag, v);  }
      void                       crcData(long tag, int &v)                       { getCurrentUpdate()->crcData(tag, v); }
      void                       crcData(long tag, const char *v, long len)      { if (v != NULL) getCurrentUpdate()->crcData(tag, v, len); }
      void                       crcData(long tag, char &v)                      { getCurrentUpdate()->crcData(tag, v);  }      

      //void                       setupUpdateChecksums();

      BSyncUpdate                *getCurrentUpdate()                         { return &mUpdates[getCurrentUpdateOffset()]; }
      long                       getAmountOfUpdates();

      bool                       saveHeader(BChunkWriter *writer);
      bool                       loadHeader(BChunkReader *reader);

      bool                       saveChecksums(BChunkWriter *writer);
      bool                       loadChecksums(BChunkReader *reader);

      bool                       saveCallTable(BChunkWriter *writer);
      bool                       loadCallTable(BChunkReader *reader);

      DWORD                      getCurrentChecksum();
      void                       clearCurrentChecksum()                      { mHistoryChecksum = 0; }

      // Functions here only for #define compatibility

      void                       createSymbolTables()                        { void(0); };
      void                       destroySymbolTables()                       { void(0); };

      void                       writeToBinaryFile(BFile& file, BSyncLogHeader& lh, long lOOSUpdateNumber);

      void                       assignSyncCallIndex(BSyncCallEntry* pEntry);
      const BSyncCallEntry*      getSyncCallEntry(WORD index);

      void                       logStats();

   private:      


      long                       getCurrentUpdateOffset()                    { return mUpdateHead; }
      BSyncUpdate                *getUpdate(long offset);      

      BDynamicSyncArray<BSyncUpdate>       mUpdates;         // 108 * 64 (defaults to 64 updateS), circular array
                                                            // BSyncUpdate allocates additional memory (non final):
                                                            // 24000 + 7360 == 31360
                                                            // 31360 * 64 == 2007040
      BDynamicSyncArray<BSyncCallEntry*>   mSyncCallTable;   // 4 * ?

      uint64                     mUniqueID;

      BSyncBufferAllocator*      mpAllocator;
      BSyncFIFO*                 mpSyncFIFO;

      long                       mUpdateHead;   // cicular array ptrs
      long                       mUpdateTail;

      uint                       mUpdateNumber;
      DWORD                      mHistoryChecksum;                               // The combined Checksum; generate on demand

      uint                       mID;

      BOOL                       mCompression;
      BOOL                       mUseCacheDrive;
      BOOL                       mAllowDynamicAlloc;

      bool                       mbSyncCallAllocated : 1;
};

//==============================================================================
class BChecksumHistory : public IPoolable
{
   public:
      BChecksumHistory();
      //BChecksumHistory(long ID, void* pChecksum, long checksumSize, long syncedPlayers, long fromClientID);

      void init(long ID, uint checksum, long syncedPlayers, long fromClientID);

      // IPoolable
      void onAcquire();
      void onRelease() {}
      DECLARE_FREELIST(BChecksumHistory, 4);

      long  mID;
      uint  mChecksum;
      long  mRequiredSyncedPlayers; // the amount of players I have to hear from before this
                                                         // history can go away
      long  mFromClientID;
};

//==============================================================================
// BSyncMananger:
// 
// The Brains or something like that. 
class BSyncManager : public BSingleton<BSyncManager>, 
                     public BMPSyncNotify
{

   public:      
      #ifndef BUILD_FINAL
         enum { cDefaultSyncUpdateInterval = 32 }; // updates per sync transmit      
      #else
         enum { cDefaultSyncUpdateInterval = 128 }; // updates per sync transmit      
      #endif            

      // C/Dtors      
      ~BSyncManager( void );

      bool                       setup();
      void                       shutdown();

      // Functions                   
      void                       setSyncedObject(BMPSyncObject *object);
      void                       nextUpdate();      
      BSyncHistory*              getCurrentHistory() const  { return mpCurrentHistory; }
      BSyncHistory*              getPlaybackHistory() const { return mpPlaybackHistory; }

      void                       writeToBinaryFile(long lOOSUpdate);
      void                       serializeToLog();
      bool                       isSerialized()                  { return mSerialized; }
      bool                       isOOS() { return isSerialized(); }
      void                       setSerialized(bool v)               { mSerialized = v; }      

      void                       outOfSync(long oosUpdateNumber=-1);

      void                       setSyncing(bool v)                  { mSyncing = v; }
      bool                       getSyncing() const                  { return mSyncing; }

      void                       setAIUpdating(bool v);
      bool                       getAIUpdating() const { return (mAIUpdatingCount>0); }

      // Determines if the tag should be synced based on the state of mSyncing and mAIUpdatingCount.
      bool                       shouldSync(long tag);

      DWORD                      checksumSyncStates();

      DWORD                      getCurrentChecksum(long tag);

      // Multiplayer Stuff
      void                       commChecksum(); // call this function once per update
      void                       addSyncedPlayer(); 
      void                       removeSyncedPlayer(long id); 
      void                       destroyChecksumHistory(); 
      void                       incommingChecksumData(BClient *fromClient, long checksumID, void *checksum, long checksumSize);

      // BMPSyncNotify
      virtual bool               incomingSyncData(long fromID, long checksumID, uint checksum);
      
      // Macroized Functions

      // define a addSync macro in SyncDefines.h, don't use these methods directly
      inline bool shouldSync(BSyncCallEntry& entry)
      {
        // SCOPEDSAMPLE(shouldSync);
 
         // Only sync if currently in the sim thread
         if(gEventDispatcher.getThreadIndex() != cThreadIndexSim)
            return false;

         if (mSyncing && syncDefines[entry.mbTag].mState)
            return true;

         if ((mAIUpdatingSaveSyncing && entry.mbTag==cRandSync))
            return true;

         return false;
      }
      //
      //! add a code sync
      //
      inline void syncCode(BSyncCallEntry& entry)
      {
         if (shouldSync(entry))
         {
            mpCurrentHistory->crcData(entry.mbTag, (long&) mCodeSyncValue); 
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, mCodeSyncValue);           
         }
      }
      //
      //! add a data sync of a long
      //
      inline void syncData(BSyncCallEntry& entry, long value)
      {  
         if (shouldSync(entry))
         {
            mpCurrentHistory->crcData(entry.mbTag, (long&)value); 
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);           
         }
      }
      //
      //! add a data sync of an int
      //
      inline void syncData(BSyncCallEntry& entry, int value)
      {  
         if (shouldSync(entry))
         {
            mpCurrentHistory->crcData(entry.mbTag, (long&)value); 
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);           
         }
      }
      //
      //! add a data sync of a DWORD
      //
      inline void syncData(BSyncCallEntry& entry, DWORD value)
      {  
         if (shouldSync(entry))
         {
            mpCurrentHistory->crcData(entry.mbTag, (long&)value); 
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);           
         }
      }
      //
      //! add a data sync of a float
      //
      inline void syncData(BSyncCallEntry& entry, float value)
      {  
         if (shouldSync(entry))
         {
            mpCurrentHistory->crcData(entry.mbTag, (long&)value); 
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);           
         }
      }
      //
      //! add a data sync of a SHORT
      //
      /*
      inline void syncData(BSyncCallEntry& entry, SHORT value)
      {  
         if (shouldSync(entry))
         {
            // jce [8/19/2005] -- copy to temp of the right size
            long temp = value; 
            mpCurrentHistory->crcData(entry.mbTag, (long&)temp); 
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);
         }
      }
      //
      //! add a data sync of a WORD
      //
      inline void syncData(BSyncCallEntry& entry, WORD value)
      {  
         if (shouldSync(entry))
         {
            // jce [8/19/2005] -- copy to temp of the right size
            long temp = value; 
            mpCurrentHistory->crcData(entry.mbTag, (long&)temp); 
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);           
         }
      }
      */
      //
      //! add a data sync of a const string. This does not CRC a value.
      //
      inline void syncData(BSyncCallEntry& entry, LPCSTR value)
      {
         if (shouldSync(entry))
         {
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);
         }
      }
      //
      //! add a data sync of a const string. This does not CRC a value.
      //
      inline void syncData(BSyncCallEntry& entry, LPSTR value)
      {
         if (shouldSync(entry))
         {
            if (isFullSync(entry.mbTag))
               mpCurrentUpdate->add(entry.mwIndex, value);
         }
      }
      //
      //! add a data sync of a vector
      //
      inline void syncData(BSyncCallEntry& entry, BVector& v)
      {
         if (shouldSync(entry))
         {
            mpCurrentHistory->crcData(entry.mbTag, (long&)v.x); 
            mpCurrentHistory->crcData(entry.mbTag, (long&)v.y); 
            mpCurrentHistory->crcData(entry.mbTag, (long&)v.z); 
            if (isFullSync(entry.mbTag))
            {
               mpCurrentUpdate->add(entry.mwIndex, v.x);
               mpCurrentUpdate->add(entry.mwIndex, v.y);
               mpCurrentUpdate->add(entry.mwIndex, v.z);
            }
         }
      }
      //
      //! add a data sync of a const Vector
      //
      inline void syncData(BSyncCallEntry& entry, const BVector& v)
      {
         if (shouldSync(entry))
         {
            mpCurrentHistory->crcData(entry.mbTag, (long&)v.x); 
            mpCurrentHistory->crcData(entry.mbTag, (long&)v.y); 
            mpCurrentHistory->crcData(entry.mbTag, (long&)v.z); 
            if (isFullSync(entry.mbTag))
            {
               mpCurrentUpdate->add(entry.mwIndex, v.x);
               mpCurrentUpdate->add(entry.mwIndex, v.y);
               mpCurrentUpdate->add(entry.mwIndex, v.z);
            }
         }
      }

      //bool                       getSyncRecord()                       { return gSyncRecord; }
      //void                       setSyncRecord(bool v)                     { gSyncRecord = v; }

      long                       getLoadedNumberOfSyncDefines()        { return mLoadedNumberOfSyncDefines; }
      void                       setLoadedNumberOfSyncDefines(long v)      { mLoadedNumberOfSyncDefines = v; }

      void                       reset();

      bool                       save(BChunkWriter* writer);
      bool                       load(BChunkReader* reader);

      long                       getSyncUpdateInterval()               { return mSyncUpdateInterval; }

      long                       getOOSAfterUpdate() const                 { return mOOSAfterUpdate; }
      void                       setOOSAfterUpdate(long number)            { mOOSAfterUpdate = number; }

      void                       activatePlaybackHistory();
      void                       deactivatePlaybackHistory();

      void                       setFull1v1(bool enable)                { mFull1v1 = enable; }
      BSyncManager();

   private:

      enum
      {
         cVersion = 2,
         cDefaultHistorySize = /*128*/cDefaultSyncUpdateInterval*2, // pask updates kept in the history  (should b2 2 x cDefaultSyncUpdateInterval + 1???? No, delayed packets problem)
      };

      void                       outOfSyncInternal(long oosUpdateNumber);
      void                       destroySyncHistory();

      inline bool isFullSync(BYTE tag) const
      {
         return ((syncDefines[tag].mState == cFullSyncState) || (mFull1v1 && (syncDefines[tag].mState == cFull1v1SyncState)));
      }

      void* syncThread();
      static void* _cdecl syncThreadCallback(void* pVal);

      BPointerList<BChecksumHistory> mChecksumHistory;

      BWin32Event                mSyncThreadExitEvent;

      BMPSyncObject*             mpSyncedObject;
      BSyncHistory*              mpCurrentHistory;
      BSyncHistory*              mpPlaybackHistory;
      BSyncUpdate*               mpCurrentUpdate;

      BSyncBufferAllocator*      mpSyncBufferAllocator;
      BSyncFIFO*                 mpSyncFIFO;
      BThread*                   mpSyncThread;

      long                       mAIUpdatingCount;

      // Multiplayer Stuff 
      long                       mSyncCounter;
      long                       mLoadedNumberOfSyncDefines;
      //long                       mChecksumSize;
      long                       mSyncUpdateInterval;
      long                       mOOSAfterUpdate;

      bool                       mSyncing : 1;
      bool                       mAIUpdatingSaveSyncing : 1;
      bool                       mSerialized : 1;
      bool                       mSyncTagControl : 1;
      bool                       mLogStats : 1;
      bool                       mFull1v1 : 1;

      BSyncManager(const BSyncManager &other) {other;}         // prevent default copy constructor from being created

      // Static stuff

      static const DWORD         mCodeSyncValue;               // Magic Number to swizzle CRC with when code position syncing
};

// TODO: Figure out if we REALLY need this crap below. It is all part of the binary writing.
#pragma pack(push)
#pragma pack(1)
struct BSyncLogHeader
{
   DWORD   dwCallRecordCount;
   DWORD   dwUpdateRecordCount;
   DWORD   dwChecksumCount;
   DWORD   dwStringRecordCount;
   DWORD   dwCallRecordFirstOffset;
   DWORD   dwUpdateFirstOffset;
   DWORD   dwChecksumFirstOffset;
   DWORD   dwStringRecordOffset;
};

struct BSyncChecksumRecord
{
   DWORD dwUpdate;
   DWORD dwChecksum;
   char szName[32];
};

//struct BSyncLogCallRecord
//{
//   long lType;
//   long lState;
//   WORD wLine;
//   char szDescription[64];
//   char szFileName[512];
//   BYTE bTag;
//};

struct BSyncUpdateRecord
{
   WORD  wCallIndex;
   WORD  wValueType;
   union
   {
      DWORD dwValue;
      LONG  lValue;
   };
   DWORD dwUpdateNumber;
};
#pragma pack(pop)


class BSyncStrLog
{
public:
   BSyncStrLog();
   ~BSyncStrLog();

   void writeToBinaryFile(BFile& file);

   DWORD insert(float f);
   DWORD insert(const char* s);
   DWORD insert(char* s);

   DWORD getCount() const;

   enum { cMaxStrLen = 64 };

   struct Node
   {
      Node() : pNext(0) {memset(&info, 0, sizeof(info));}
#pragma pack(push)
#pragma pack(1)
      struct StrInfo
      {
         DWORD    dwIndex;
         char     szValue[cMaxStrLen];
      };
      StrInfo  info;
#pragma pack(pop)
      Node* pNext;
   };

private:
   DWORD insertNode(Node*);

   Node* mpFirst;
   Node* mpLast;
   DWORD mdwCount;
};
