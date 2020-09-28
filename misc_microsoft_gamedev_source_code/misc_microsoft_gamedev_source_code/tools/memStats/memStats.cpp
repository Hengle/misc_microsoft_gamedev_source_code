// File: memStats.cpp
#include "xcore.h"
#include "xcoreLib.h"
#include "consoleOutput.h"
#include "containers\hashMap.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "stream\byteStream.h"
#include "xdb\xdb.h"
#include "writeTGA.h"
#include "RGBAImage.h"
#include "ImageUtils.h"

#include "memory\allocationLoggerPackets.h"

#define cXMLStringBufferSize 200

//-------------------------------------------------
class BAllocationLogParser
{
public:
   BAllocationLogParser() :
      mVersionPacketIndex(-1),
         mSequentialOffs(0)
   {
   }
   
   ~BAllocationLogParser()
   {
   }
   


   int startSequentialRead()
   {
      mpFileStream->seek(0,true);
      mSequentialOffs = 0;
      return mPacketOffsets.getSize();
   }

   const BALPacketBase* getNextSequentialPacket(byte* tempDat) const
   {
      const uint64 bytesLeft = mStreamSize - mSequentialOffs;   
      if (bytesLeft < sizeof(BALPacketBase))
         return NULL;

      bool OK = true;

      OK &= (mpFileStream->readBytes(&tempDat[0], sizeof(BALPacketBase)) == sizeof(BALPacketBase));
      BDEBUG_ASSERT(OK);

      BALPacketBase* pBase = ((BALPacketBase*)&tempDat[0]);

      if (cLittleEndianNative)
         pBase->endianSwitch();

      uint paketSize = pBase->mPacketSize - sizeof(BALPacketBase);

      if (cLittleEndianNative)
         pBase->endianSwitch();

      OK&= (mpFileStream->readBytes(&tempDat[sizeof(BALPacketBase)], paketSize) == paketSize);
      BDEBUG_ASSERT(OK);

      allocLogPacketEndianSwitch(pBase);

//      mSequentialOffs += pBase->mPacketSize;

      return reinterpret_cast<const BALPacketBase* const>(pBase);
   }



   bool read(BStream* pStream)
   {
      if ((pStream->size() < sizeof(BALPacketBase)))// || (pStream->size() >= 2048U*1024U*1024U))
         return false;
      
      mpFileStream = pStream;
      mStreamSize = (uint64)pStream->size();   

      bool foundEOFPacket = false;
      
      uint64 curOfs = 0;
      byte tempDat[256];
      while (curOfs < mStreamSize)
      {
         const uint64 bytesLeft = mStreamSize - curOfs;   
         if (bytesLeft < sizeof(BALPacketBase))
            return false;
      
         const uint packetIndex = mPacketOffsets.getSize();   
         mPacketOffsets.pushBack(curOfs);   
         
 
         if (pStream->readBytes(&tempDat[0], sizeof(BALPacketBase)) != sizeof(BALPacketBase))
            break;
 

         BALPacketBase* pBase = ((BALPacketBase*)&tempDat[0]);

         if (cLittleEndianNative)
            pBase->endianSwitch();

         uint paketSize = pBase->mPacketSize - sizeof(BALPacketBase);

         if (cLittleEndianNative)
            pBase->endianSwitch();

         if (pStream->readBytes(&tempDat[sizeof(BALPacketBase)], paketSize) != paketSize)
            break;
 
         


         
         BALPacketBase packetBase(*pBase);
         if (cLittleEndianNative)
            packetBase.endianSwitch();
         
         if (bytesLeft < packetBase.mPacketSize)                           
         {
            gConsoleOutput.warning("Allocation log ended prematurely!\n");
            break;
         }
         
         if ((packetBase.mPacketPrefix != cALPacketPrefix) || (packetBase.mPacketSize < sizeof(BALPacketBase)))
            return false;
            
         if (packetBase.mPacketType >= cALNumPacketTypes)
            return false;
                  
         if (packetBase.mPacketSize != allocLogPacketGetSize(packetBase.mPacketType))
            return false;
            
         if (cLittleEndianNative)
            allocLogPacketEndianSwitch(pBase);
                                      
         switch (packetBase.mPacketType)            
         { 
            case cALVersion:
            {  
               BALPacketVersion* pPacket = reinterpret_cast<BALPacketVersion*>(pBase);

               if (pPacket->mVersion != cALStreamVersion)
                  return false;
               
               mVersionPacketIndex = packetIndex;
               memcpy(&mVersionPacket,&tempDat[0],sizeof(mVersionPacket));

               break;
            }
            case cALEOF:
            {
               foundEOFPacket = true;
               break;  
            }
         }
                           
         curOfs += pBase->mPacketSize;

      }
      
      if (mVersionPacketIndex < 0) 
         return false;
      if (!foundEOFPacket)
      {
         gConsoleOutput.warning("Allocation log EOF packet not found!\n");
      }

      int64 seekres = mpFileStream->seek(0,true);
      
      return true;
   }
   
   uint getNumPackets(void) const { return mPacketOffsets.getSize(); }
   
   const BALPacketBase& getPacket(uint index, byte* tempDat) const
   {
      BDEBUG_ASSERT(index < getNumPackets());
      bool OK = true;
      const uint64 seekDest = mPacketOffsets[index];
      mpFileStream->seek(seekDest,true);
      
      OK &= (mpFileStream->readBytes(&tempDat[0], sizeof(BALPacketBase)) == sizeof(BALPacketBase));
      BDEBUG_ASSERT(OK);
         

      BALPacketBase* pBase = ((BALPacketBase*)&tempDat[0]);

      if (cLittleEndianNative)
         pBase->endianSwitch();

      uint paketSize = pBase->mPacketSize - sizeof(BALPacketBase);

      if (cLittleEndianNative)
         pBase->endianSwitch();

      OK&= (mpFileStream->readBytes(&tempDat[sizeof(BALPacketBase)], paketSize) == paketSize);
      BDEBUG_ASSERT(OK);

      allocLogPacketEndianSwitch(pBase);

      return *reinterpret_cast<const BALPacketBase*>(pBase);
      //return *reinterpret_cast<const BALPacketBase*>(mFileData.getPtr() + mPacketOffsets[index]);
   }
   
   int getVersionPacketIndex(void) const { return mVersionPacketIndex; }
   const BALPacketVersion& getVersionPacket(void) const { return mVersionPacket; }
   
private:
   BStream* mpFileStream;
   int mVersionPacketIndex;
   BALPacketVersion  mVersionPacket;
   
   
   uint64 mStreamSize;
   uint64 mSequentialOffs;

   BDynamicArray<uint64> mPacketOffsets;
};

//-------------------------------------------------

class BMemStatsUtil
{
public:
   BMemStatsUtil() :
      mLeafNodeFiltering(false),
      mSaveActiveAllocsToXML(false),
      mEndSnapshotIndex(-1),
      mStartSnapshotIndex(-1),
      mNumFilenames(0),
      mXDBStream((const void*)NULL, 0),
      mNumStrayDeletes(0),
      mDetailAddressInt(0xFFFFFFFF)
   {
      //mLeafNodeFiltering = true;
      //mEndSnapshotIndex = 1;
      //mHeapName = "string";
   }

   bool process(BCommandLineParser::BStringArray& args)
   {
      if (!parseArgs(args))
         return false;
   
      if (!openOutputFile())
         return false;         
         
      if (!openXDBFile())
         return false;
      
      if (!openAllocLog())
         return false;
            
      createActiveAllocs(mStartSnapshotIndex, mEndSnapshotIndex);
      
      if (!createHeapBitmap())
         return false;
      
      createSymbolTable();

      // Only save the active allocs to the xml file
      if (mSaveActiveAllocsToXML)
      {
         dumpActiveAllocsToXML();
      }
      else
      {
         dumpSummary();

         dumpFileStats();

         dumpSymbolStats();

         dumpActiveAllocs();

         dumpSymbolTable();
      }

      closeOutputFile();

      closeAllocLog();

      closeXDBFile();

      return true;
   }

private:
   bool mLeafNodeFiltering;
   bool mSaveActiveAllocsToXML;
   int mEndSnapshotIndex;
   int mStartSnapshotIndex;
   BString mHeapName;
      
   BString mFilenames[3];
   uint mNumFilenames;
   
   BByteArray mXDBFileData;
   BByteStream mXDBStream;
   BXDBFileReader mXDBFileReader;
   
   BCFileStream mAllocLogStream;
   BAllocationLogParser mAllocLog;
   
   BCommandLineParser::BStringArray mLeafSymbolsToIgnore;
   
   BString mDetailAddress;
   uint mDetailAddressInt;
   
   BString mHeapBitmapFilename;
   
   uint mAddressBias;
   
   BCFileStream mOutputStream;
         
   enum
   {
      cCached = 0,
      cPhysical = 1
   };
   
   class BAllocStats
   {
   public:
      BAllocStats()
      {
         clear();
      }
      
      void clear(void)
      {
         Utils::ClearObj(mTotalAllocs);
         Utils::ClearObj(mTotalAllocBytes);
         Utils::ClearObj(mLargestAlloc);
      }

      void add(uint size, bool physical, bool inclusive)
      {
         mTotalAllocs[inclusive][physical]++;
         mTotalAllocBytes[inclusive][physical] += size;
                           
         mLargestAlloc[inclusive][physical] = Math::Max(mLargestAlloc[inclusive][physical], size);
      }
      
      BAllocStats& operator+= (const BAllocStats& rhs)
      {
         for (uint i = 0; i < 2; i++)
            for (uint j = 0; j < 2; j++)
            {
               mTotalAllocs[i][j] += rhs.mTotalAllocs[i][j];
               mTotalAllocBytes[i][j] += rhs.mTotalAllocBytes[i][j];
               mLargestAlloc[i][j] = Math::Max(mLargestAlloc[i][j], rhs.mLargestAlloc[i][j]);
            }  
         return *this;            
      }
      
      uint getTotalAllocs(bool inclusive) const
      {
         uint total = 0;
         for (uint j = 0; j < 2; j++)
            total += mTotalAllocs[inclusive][j];
         return total;               
      }
      
      uint64 getTotalAllocBytes(bool inclusive) const
      {
         uint64 total = 0;
         for (uint j = 0; j < 2; j++)
            total += mTotalAllocBytes[inclusive][j];
         return total;               
      }
           
      // [inclusive][physical]
      uint mTotalAllocs[2][2];
      int64 mTotalAllocBytes[2][2];
      uint mLargestAlloc[2][2];
   };
   
   class BActiveAlloc
   {
   public:
      BActiveAlloc() :
         mAddress(0),
         mThread(0),
         mCurSize(0),
         mPhysical(false),
         mResizes(0),
         mIndex(0),
         mSnapshot(0)
      {
      }
      
      BActiveAlloc(uint64 address, uint thread, uint curSize, bool physical, uint snapshot) :
         mAddress(address),
         mThread(thread),
         mCurSize(curSize),
         mPhysical(physical),
         mResizes(0),
         mIndex(0),
         mSnapshot(snapshot)
      {
      }
      
      ~BActiveAlloc()
      {
      }
      
      uint64 mAddress;      
      uint mThread;
      
      UIntArray mPacketIndices;
      BALContext mContext;
            
      uint mCurSize;
      uint mResizes;
      uint mIndex;
      bool mPhysical;
      uint mSnapshot;
      
      bool operator< (const BActiveAlloc& rhs) const
      {
         return mCurSize < rhs.mCurSize;
      }
   };
   
   typedef BHashMap<uint64, BActiveAlloc> BActiveAllocHashMap;
   BActiveAllocHashMap mActiveAllocs;
   BDynamicArray<BActiveAllocHashMap::iterator> mSortedActiveAllocIters;
         
   class BSymbolInfo
   {
   public:
      BSymbolInfo() : mAddress(0), mLine(0), mFlags(0), mIndex(0)
      { 
      }
      
      BSymbolInfo(const BXDBFileReader::BLookupInfo& info)
      {
         mAddress = info.mAddress;
         mLine = info.mFoundLine ? (ushort)info.mLine : 0;
         mFlags = 0;
         
         if (info.mFoundSymbol) mFlags |= cFlagFoundSymbol;
         if (info.mFoundLine) mFlags |= cFlagFoundLine;
         
         mSymbol = info.mSymbol;
         
         mFilename = info.mFilename;
         
         mAllocStats.clear();
         
         mIndex = 0;
      }
      
      BAllocStats mAllocStats;
      
      uint mAddress;
                  
      BString mSymbol;
      BString mFilename;
      
      ushort mLine;   
      
      uint mIndex;
      
      enum
      {
         cFlagFoundSymbol = 1,
         cFlagFoundLine = 2,
      };
      ushort mFlags;
            
      operator size_t()
      {
         return hashFast(&mAddress, sizeof(mAddress));
      }
      
      bool operator== (const BSymbolInfo& rhs) const
      {
         return (mAddress == rhs.mAddress);
      }
      
      bool operator< (const BSymbolInfo& rhs) const
      {
         return mAddress < rhs.mAddress;
      }
   };
      
   typedef BHashMap<uint, BSymbolInfo> BSymbolHashMap;
   BSymbolHashMap mSymbolHashMap;
   
   typedef BHashMap<BString, BAllocStats> BAllocStatsHashMap;
   BAllocStatsHashMap mSymStatsHashMap;
   BAllocStatsHashMap mFileStatsHashMap;
   
   class BHeapInfo
   {
   public:
      BHeapInfo() :
         mPhysical(false)
      {
      }
      
      BHeapInfo(const BString name, bool physical) : 
         mName(name), 
         mPhysical(physical)
      {
      }
      
      BString mName;
      
      bool mPhysical;
   };
   
   typedef BHashMap<uint, BHeapInfo> BHeapHashMap;
   BHeapHashMap mHeapHashMap;
         
   uint mNumStrayDeletes;
   
   BHeapInfo& findHeap(void* pHeap)
   {
      BHeapHashMap::iterator it = mHeapHashMap.find((uint)pHeap);
      if (it != mHeapHashMap.end())
         return it->second;
      
      BHeapInfo heapInfo;
      
      BHeapHashMap::InsertResult insertRes = mHeapHashMap.insert((uint)pHeap, heapInfo);
      return (insertRes.first)->second;
   }
   
   BSymbolHashMap::iterator findSymbolByAddress(uint address)
   {
      address += mAddressBias;
      
      BSymbolHashMap::iterator it = mSymbolHashMap.find(address);
      if (it != mSymbolHashMap.end())
         return it;
            
      BXDBFileReader::BLookupInfo lookupInfo;
      const bool found = mXDBFileReader.lookup(address, lookupInfo);
      found;
            
      BSymbolInfo symInfo(lookupInfo);
      
      BSymbolHashMap::InsertResult insertRes = mSymbolHashMap.insert(address, symInfo);
      return insertRes.first;
   }
         
   static bool activeAllocSortFunctor(BActiveAllocHashMap::iterator& lhsIter, BActiveAllocHashMap::iterator& rhsIter)
   {
      const BActiveAlloc& lhs = lhsIter->second;
      const BActiveAlloc& rhs = rhsIter->second;      
      return rhs < lhs;
   }
      
   void dumpActiveAllocs(void)
   {
      mOutputStream.printf("--------------------- Active allocations:\n");
                  
      mSortedActiveAllocIters.reserve(mActiveAllocs.getSize());
      mSortedActiveAllocIters.resize(0);
      for (BActiveAllocHashMap::iterator it = mActiveAllocs.begin(); it != mActiveAllocs.end(); it++)
         mSortedActiveAllocIters.pushBack(it);
         
      std::sort(mSortedActiveAllocIters.begin(), mSortedActiveAllocIters.end(), activeAllocSortFunctor);
      
      uint totalCached = 0;
      uint totalPhys = 0;
      
      for (uint i = 0; i < mActiveAllocs.getSize(); i++)
      {
         const BActiveAlloc& alloc = mSortedActiveAllocIters[i]->second;
         
         if (alloc.mPhysical)
            totalPhys += alloc.mCurSize;
         else
            totalCached += alloc.mCurSize;
            
         BHeapHashMap::const_iterator heapIter = mHeapHashMap.find((uint)(alloc.mAddress>>32));
         const char* pHeapName = "?";
         if (heapIter != mHeapHashMap.end())
            pHeapName = heapIter->second.mName.getPtr();
                           
         mOutputStream.printf("%u: Heap: %s, Address: 0x%08X, Thread: %u, Physical: %u\n", i, pHeapName, (uint)alloc.mAddress, alloc.mThread, alloc.mPhysical);
         mOutputStream.printf(" Size: %u\n", alloc.mCurSize);
         mOutputStream.printf(" Resizes: %u\n", alloc.mResizes);
         
         const UIntArray& packetIndices = alloc.mPacketIndices;
         
         const BALContext& context = alloc.mContext;
         
         for (uint backTraceIndex = 0; backTraceIndex < context.mBackTraceSize; backTraceIndex++)
         {
            const uint address = context.mBackTrace[backTraceIndex];
            BSymbolInfo& symbolInfo = findSymbolByAddress(address)->second;
            mOutputStream.printf("%08X %s(%u): %s\n", symbolInfo.mAddress, symbolInfo.mFilename.getPtr(), symbolInfo.mLine, symbolInfo.mSymbol.getPtr());
         }
                  
         mOutputStream.printf("\n");
      }
      
      mOutputStream.printf("Total Blocks: %u, Bytes: %u, Cached: %u, Physical: %u\n", mActiveAllocs.getSize(), totalCached + totalPhys, totalCached, totalPhys);
      mOutputStream.printf("\n");
   }
         
   ////////////////////////////////////////////////////////////////////////////////////////
   // Saves active allocs to an xml file that can be read by the Age3 memview tool
   ////////////////////////////////////////////////////////////////////////////////////////
   void dumpActiveAllocsToXML(void)
   {
      mOutputStream.printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
      mOutputStream.printf("<MemStats>\n");
                  
      mSortedActiveAllocIters.reserve(mActiveAllocs.getSize());
      mSortedActiveAllocIters.resize(0);
      for (BActiveAllocHashMap::iterator it = mActiveAllocs.begin(); it != mActiveAllocs.end(); it++)
         mSortedActiveAllocIters.pushBack(it);
         
      std::sort(mSortedActiveAllocIters.begin(), mSortedActiveAllocIters.end(), activeAllocSortFunctor);
      
      uint totalCached = 0;
      uint totalPhys = 0;
      
      for (uint i = 0; i < mActiveAllocs.getSize(); i++)
      {
         const BActiveAlloc& alloc = mSortedActiveAllocIters[i]->second;
         
         if (alloc.mPhysical)
            totalPhys += alloc.mCurSize;
         else
            totalCached += alloc.mCurSize;

         BHeapHashMap::const_iterator heapIter = mHeapHashMap.find((uint)(alloc.mAddress>>32));
         const char* pHeapName = "?";
         if (heapIter != mHeapHashMap.end())
            pHeapName = heapIter->second.mName.getPtr();

         mOutputStream.printf("   <alloc size='%d' address='%u'>\n", alloc.mCurSize, alloc.mAddress);

         const BALContext& context = alloc.mContext;
         
         for (uint backTraceIndex = 0; backTraceIndex < context.mBackTraceSize; backTraceIndex++)
         {
            const uint address = context.mBackTrace[backTraceIndex];
            BSymbolInfo& symbolInfo = findSymbolByAddress(address)->second;

            static WCHAR fileName[cXMLStringBufferSize];
            static WCHAR symbolName[cXMLStringBufferSize];
            static BStringTemplate<WCHAR> buffer;

            symbolInfo.mFilename.asUnicode(buffer);
            strwEscapeForXML(buffer.getPtr(), fileName, cXMLStringBufferSize);

            symbolInfo.mSymbol.asUnicode(buffer);
            strwEscapeForXML(buffer.getPtr(), symbolName, cXMLStringBufferSize);

            mOutputStream.printf("      <stack file='%S' line='%d'>%S</stack>\n", fileName, symbolInfo.mLine, symbolName);
         }
                  
         mOutputStream.printf("   </alloc>\n");
      }
      
      mOutputStream.printf("</MemStats>\n");
   }

   void dumpSymbolTable(void)
   {
      mOutputStream.printf("--------------------- By address symbol table:\n");
      
      for (BSymbolHashMap::const_iterator it = mSymbolHashMap.begin(); it != mSymbolHashMap.end(); ++it)
      {
         const BSymbolInfo& symInfo = it->second;
         const BAllocStats& stats = symInfo.mAllocStats;
                                    
         mOutputStream.printf("%u: Code Address: 0x%08X\n", symInfo.mIndex, symInfo.mAddress);
         mOutputStream.printf(" Symbol: \"%s\"\n", symInfo.mSymbol.getPtr());
         mOutputStream.printf(" Filename: \"%s\"\n", symInfo.mFilename.getPtr());
         mOutputStream.printf(" Line: %u\n", symInfo.mLine);
         
         uint first = 0;
         if (!stats.getTotalAllocs(false))
            first = 1;
         for (uint i = first; i < 2; i++)
         {
            mOutputStream.printf(i ? " Inclusive:\n" : " Exclusive:\n");
            
            mOutputStream.printf("        Total Allocs: %u, %u %u\n", stats.mTotalAllocs[i][0] + stats.mTotalAllocs[i][1], stats.mTotalAllocs[i][0], stats.mTotalAllocs[i][1]);
            mOutputStream.printf("   Total Alloc Bytes: %I64i, %I64i %I64i\n", stats.mTotalAllocBytes[i][0] + stats.mTotalAllocBytes[i][1], stats.mTotalAllocBytes[i][0], stats.mTotalAllocBytes[i][1]);
            mOutputStream.printf("       Largest Alloc: %u, %u %u\n", stats.mLargestAlloc[i][0] + stats.mLargestAlloc[i][1], stats.mLargestAlloc[i][0], stats.mLargestAlloc[i][1]);
         }
         
         mOutputStream.printf("\n");
      }         
      
      mOutputStream.printf("\n");
   }
   
   static bool statsSortFunctor(BAllocStatsHashMap::const_iterator& lhsIter, BAllocStatsHashMap::const_iterator& rhsIter)
   {
      const BAllocStats& lhs = lhsIter->second;
      const BAllocStats& rhs = rhsIter->second;
      if (rhs.getTotalAllocBytes(false) < lhs.getTotalAllocBytes(false))
         return true;
      else if (rhs.getTotalAllocBytes(false) == lhs.getTotalAllocBytes(false))
         return (rhs.getTotalAllocBytes(true) < lhs.getTotalAllocBytes(true));
      
      return false;
   }
   
   void dumpStatsHashMap(const BAllocStatsHashMap& stats, bool inclusiveStats)
   {
      BDynamicArray<BAllocStatsHashMap::const_iterator> statsIterArray;
      statsIterArray.reserve(stats.getSize());
      statsIterArray.resize(0);
      for (BAllocStatsHashMap::const_iterator it = stats.begin(); it != stats.end(); ++it)
         statsIterArray.pushBack(it);

      std::sort(statsIterArray.begin(), statsIterArray.end(), statsSortFunctor);         
            
      for (uint i = 0; i < statsIterArray.getSize(); i++)
      {
         BAllocStatsHashMap::const_iterator it = statsIterArray[i];
         const BString& name = it->first;
         const BAllocStats& stats = it->second;

         mOutputStream.printf("%u: %s\n", i, name.getPtr());

         uint first = 0;
         if ((!stats.getTotalAllocs(false)) && (inclusiveStats))
            first = 1;
         uint last = inclusiveStats ? 2 : 1;
         for (uint i = first; i < last; i++)
         {
            mOutputStream.printf(i ? " Inclusive:\n" : " Exclusive:\n");

            mOutputStream.printf("        Total Allocs: %u, %u %u\n", stats.mTotalAllocs[i][0] + stats.mTotalAllocs[i][1], stats.mTotalAllocs[i][0], stats.mTotalAllocs[i][1]);
            mOutputStream.printf("   Total Alloc Bytes: %I64i, %I64i %I64i\n", stats.mTotalAllocBytes[i][0] + stats.mTotalAllocBytes[i][1], stats.mTotalAllocBytes[i][0], stats.mTotalAllocBytes[i][1]);
            mOutputStream.printf("       Largest Alloc: %u, %u %u\n", stats.mLargestAlloc[i][0] + stats.mLargestAlloc[i][1], stats.mLargestAlloc[i][0], stats.mLargestAlloc[i][1]);
         }

         mOutputStream.printf("\n");
      }            
   }
   
   void dumpSymbolStats(void)
   {
      mOutputStream.printf("--------------------- Statistics by symbol:\n");
      
      dumpStatsHashMap(mSymStatsHashMap, true);
   }
   
   void dumpSummary(void)
   {
      uint totalCached = 0;
      uint totalPhys = 0;

      for (BActiveAllocHashMap::const_iterator it = mActiveAllocs.begin(); it != mActiveAllocs.end(); it++)
      {
         const BActiveAlloc& alloc = it->second;

         if (alloc.mPhysical)
            totalPhys += alloc.mCurSize;
         else
            totalCached += alloc.mCurSize;
      }

      mOutputStream.printf("Total Blocks: %u, Bytes: %u, Cached: %u, Physical: %u\n", mActiveAllocs.getSize(), totalCached + totalPhys, totalCached, totalPhys);
      mOutputStream.printf("\n");
   }
   
   void dumpFileStats(void)
   {
      mOutputStream.printf("--------------------- Statistics by file:\n");
      
      dumpStatsHashMap(mFileStatsHashMap, false);
   }
         
   void dumpPacketDetails(const char* pDesc, void* pAddr, uint size, const BHeapInfo& heap, const BALContext& context, int origSize = -1)
   {
      mOutputStream.printf("%s: Heap: %s, Address: 0x%08X, Thread: %u, Physical: %u\n", pDesc, heap.mName.getPtr(), pAddr, context.mThreadIndex, heap.mPhysical);
      if (origSize != -1)
         mOutputStream.printf(" OrigSize: %i, NewSize: %u\n", origSize, size);
      else
         mOutputStream.printf(" Size: %u\n", size);
      
      for (uint backTraceIndex = 0; backTraceIndex < context.mBackTraceSize; backTraceIndex++)
      {
         const uint address = context.mBackTrace[backTraceIndex];
         BSymbolInfo& symbolInfo = findSymbolByAddress(address)->second;
         mOutputStream.printf("%08X %s(%u): %s\n", symbolInfo.mAddress, symbolInfo.mFilename.getPtr(), symbolInfo.mLine, symbolInfo.mSymbol.getPtr());
      }
      mOutputStream.printf("\n\n");
   }
   
   void createActiveAllocs(int startSnapshotIndex, int endSnapshotIndex)
   {
      gConsoleOutput.printf("Creating active allocation table.\n");
      
      mHeapHashMap.clear();

      uint maxFrameIndex = 0;                        
      uint maxSnapshotIndex = 0;
      byte tempDat[256];
      const uint numPackets = mAllocLog.startSequentialRead();
      for (uint packetIndex = 0; packetIndex < numPackets; packetIndex++)
      {
         const BALPacketBase& packetBase = *mAllocLog.getNextSequentialPacket(tempDat); //getPacket(packetIndex,tempDat);
         
         switch (packetBase.mPacketType)
         {
            case cALRegisterHeap:
            {
               const BALPacketRegisterHeap& packet = (const BALPacketRegisterHeap&)packetBase;
               
               BHeapInfo heapInfo(packet.mName.c_str(), (packet.mFlags & BALPacketRegisterHeap::cFlagPhysical) != 0);
               mHeapHashMap.insert((uint)packet.mPtr, heapInfo);
               
               break;
            }
            case cALIgnoreLeaf:
            {
               const BALPacketIgnoreLeaf& packet = (const BALPacketIgnoreLeaf&)packetBase;
               
               mLeafSymbolsToIgnore.pushBack(packet.mSymbol.c_str());
               
               break;
            }
            case cALFrame:
            {
               const BALPacketFrame& packet = (const BALPacketFrame&)packetBase;
               maxFrameIndex = Math::Max<uint>(packet.mIndex, maxFrameIndex);
               break;
            }
            case cALSnapshot:
            {
               const BALPacketSnapshot& packet = (const BALPacketSnapshot&)packetBase;
               maxSnapshotIndex = Math::Max<uint>(maxSnapshotIndex, packet.mIndex);
               gConsoleOutput.printf("Found snapshot index %u.\n", packet.mIndex);
               break;
            }
         }
      }
      
      gConsoleOutput.printf("Max frame index: %u\n", maxFrameIndex);
      gConsoleOutput.printf("Max snapshot index: %u\n", maxSnapshotIndex);
      
      mActiveAllocs.clear();
      
      mNumStrayDeletes = 0;
      
      bool foundStartSnapshot = false;
      if (startSnapshotIndex < 0)
         foundStartSnapshot = true;
         
      bool foundEndSnapshot = false;
      uint curSnapshotIndex = 0;
      
      mAllocLog.startSequentialRead();
      for (uint packetIndex = 0; packetIndex < numPackets; packetIndex++)
      {
         const BALPacketBase& packetBase = *mAllocLog.getNextSequentialPacket(tempDat);//getPacket(packetIndex,tempDat);
         
         switch (packetBase.mPacketType)
         {              
            case cALVersion:
            {
               break;
            }    
            case cALSnapshot:
            {
               const BALPacketSnapshot& packet = (const BALPacketSnapshot&)packetBase;
               
               if ((startSnapshotIndex != -1) && (packet.mIndex == startSnapshotIndex))
                  foundStartSnapshot = true;
                  
               if ((endSnapshotIndex != -1) && (packet.mIndex == endSnapshotIndex))
                  foundEndSnapshot = true;
               
               curSnapshotIndex = packet.mIndex;
               
               break;
            }
            case cALNew:
            {
               if (!foundStartSnapshot)
                  continue;
                  
               const BALPacketNew& packet = (const BALPacketNew&)packetBase;
                                             
               BHeapInfo& heapInfo = findHeap(packet.mpHeap);
               
               if ((mHeapName.length()) && (heapInfo.mName != mHeapName))
                  continue;
               
               if (packet.mpBlock == (void*)mDetailAddressInt)               
                  dumpPacketDetails("New", packet.mpBlock, packet.mSize, heapInfo, packet.mContext);
                                 
               const uint64 fullAddress = (uint)packet.mpBlock | (((uint64)(uint)packet.mpHeap)<<32);
               BActiveAlloc activeAlloc(fullAddress, packet.mContext.mThreadIndex, packet.mBlockSize, heapInfo.mPhysical, curSnapshotIndex);
               activeAlloc.mPacketIndices.pushBack(packetIndex);
               activeAlloc.mContext = packet.mContext;
               if (!mActiveAllocs.insert(fullAddress, activeAlloc).second)
                  gConsoleOutput.error("Duplicate New at Heap: 0x%08X Address: 0x%08X\n", packet.mpHeap, packet.mpBlock);

               break;
            }
            case cALResize:
            {
               if (!foundStartSnapshot)
                  continue;
                  
               const BALPacketResize& packet = (const BALPacketResize&)packetBase;
               
               BHeapInfo& heapInfo = findHeap(packet.mpHeap);
               if ((mHeapName.length()) && (heapInfo.mName != mHeapName))
                  continue;
                              
               uint origSize = 0;
               
               const uint64 origFullAddress = (uint)packet.mpOrigBlock | (((uint64)(uint)packet.mpHeap)<<32);
               BActiveAllocHashMap::iterator origActiveAllocIter = mActiveAllocs.find(origFullAddress);
               if (origActiveAllocIter != mActiveAllocs.end())
                  origSize = origActiveAllocIter->second.mCurSize;
               
               if (packet.mpNewBlock == (void*)mDetailAddressInt)               
                  dumpPacketDetails("Resize", packet.mpNewBlock, packet.mNewSize, heapInfo, packet.mContext, origSize);
                     
               const uint64 newFullAddress = (uint)packet.mpNewBlock | (((uint64)(uint)packet.mpHeap)<<32);
               if (origFullAddress != newFullAddress)
               {
                  if (origActiveAllocIter != mActiveAllocs.end())
                     mActiveAllocs.erase(origActiveAllocIter);
               }
                                             
               if (packet.mpNewBlock)
               {
                  BActiveAllocHashMap::iterator newActiveAllocIter = mActiveAllocs.find(newFullAddress);
                  
                  if (newActiveAllocIter != mActiveAllocs.end())
                  {
                     newActiveAllocIter->second.mSnapshot = curSnapshotIndex;
                     newActiveAllocIter->second.mResizes++;
                     newActiveAllocIter->second.mCurSize = packet.mNewSize;
                  }
                  else
                  {
                     BActiveAlloc newActiveAlloc(newFullAddress, packet.mContext.mThreadIndex, packet.mNewSize, heapInfo.mPhysical, curSnapshotIndex);
                     newActiveAlloc.mPacketIndices.pushBack(packetIndex);
                     newActiveAlloc.mContext = packet.mContext;
                     mActiveAllocs.insert(newFullAddress, newActiveAlloc);
                  }
               }                  

               break;
            }
            case cALDelete:
            {
               if (!foundStartSnapshot)
                  continue;
                  
               const BALPacketDelete& packet = (const BALPacketDelete&)packetBase;
               
               BHeapInfo& heapInfo = findHeap(packet.mpHeap);
               if ((mHeapName.length()) && (heapInfo.mName != mHeapName))
                  continue;
                  
               if (packet.mpBlock == (void*)mDetailAddressInt)               
                  dumpPacketDetails("Delete", packet.mpBlock, 0, heapInfo, packet.mContext);
                  
               uint blockSize = 0;
               
               const uint64 fullAddress = (uint)packet.mpBlock | (((uint64)(uint)packet.mpHeap)<<32);
               BActiveAllocHashMap::iterator activeAllocIter = mActiveAllocs.find(fullAddress);
               const bool allocFound = (activeAllocIter != mActiveAllocs.end());
               if (allocFound)
               {
                  blockSize = activeAllocIter->second.mCurSize;
                  
                  mActiveAllocs.erase(activeAllocIter);
               }
               else
               {
                  mNumStrayDeletes++;
               }
               
               break;
            }
         }
         
         if (foundEndSnapshot)
            break;
      }
      
      if (startSnapshotIndex != -1)
      {
         if (foundStartSnapshot)
            gConsoleOutput.printf("Found start allocation snapshot %u.\n", startSnapshotIndex);
         else
            gConsoleOutput.error("Start allocation snapshot %u not found!\n", startSnapshotIndex);
      }
      
      if (endSnapshotIndex != -1)
      {
         if (foundEndSnapshot)
            gConsoleOutput.printf("Found end allocation snapshot %u.\n", endSnapshotIndex);
         else
            gConsoleOutput.warning("End allocation snapshot %u not found!\n", endSnapshotIndex);
      }            
      
      uint index = 0;
      for (BActiveAllocHashMap::iterator it = mActiveAllocs.begin(); it != mActiveAllocs.end(); ++it)
      {
         BActiveAlloc& alloc = it->second;
         alloc.mIndex = index;

         index++;
      }       
            
      gConsoleOutput.printf("Total active allocations: %u\n", mActiveAllocs.getSize());
      if (!mSaveActiveAllocsToXML)
         mOutputStream.printf("Total stray deletes: %u\n", mNumStrayDeletes);
   }
   
   bool createHeapBitmap()
   {
      if (mHeapBitmapFilename.isEmpty())
         return true;
         
      gConsoleOutput.printf("Creating heap bitmap\n");
      
      const uint cMaxPages = 1U << 20U;
      BDynamicArray<ushort> bytesPerPage(cMaxPages);
                  
      for (BActiveAllocHashMap::const_iterator it = mActiveAllocs.begin(); it != mActiveAllocs.end(); it++)
      {
         const BActiveAlloc& alloc = it->second;
         
         const uint size = alloc.mCurSize;
         const uint startAddr = (alloc.mAddress & 0xFFFFFFFF);
         const uint endAddr = startAddr + size;
         
         const uint firstPage = startAddr >> 12;
         const uint lastPage = (endAddr - 1) >> 12;
         
         for (uint pageIndex = firstPage; pageIndex <= lastPage; pageIndex++)
         {
            const uint pageAddrLo = pageIndex << 12;
            const uint pageAddrHi = (pageIndex << 12) + 4096;
            
            const uint startAlloc = Math::Max(pageAddrLo, startAddr);
            const uint endAlloc = Math::Min(pageAddrHi, endAddr);
            const uint pageUsed = endAlloc - startAlloc;
                        
            bytesPerPage[pageIndex] += pageUsed;
         }
      }
   
      const uint cBlockSize = 2;
      BRGBAImage image(1024 * cBlockSize, 1024 * cBlockSize);
      
      for (uint pageIndex = 0; pageIndex < cMaxPages; pageIndex++)
      {
         if (!bytesPerPage[pageIndex])
            continue;
         
         uint r = 0, g = 0, b = 0;
         
         if (bytesPerPage[pageIndex] > 4096)
            r = b = 255;
         else if (bytesPerPage[pageIndex] == 4096)
            g = 255;
         else if (bytesPerPage[pageIndex] <= 32)
            r = 255;
         else
            r = g = b = ((bytesPerPage[pageIndex] * 200) / 4096) + 55;
         
         const uint x = cBlockSize * (pageIndex % 1024);
         const uint y = cBlockSize * (pageIndex / 1024);
         image.fillRect(x, y, cBlockSize, cBlockSize, BRGBAColor(r, g, b, 255));
      }
      
      BCFileStream stream;
      if (!stream.open(mHeapBitmapFilename, cSFWritable | cSFSeekable))
      {
         gConsoleOutput.error("Unable to open file \"%s\" for writing!", mHeapBitmapFilename.getPtr());
         return false;
      }
      
      if (!BImageUtils::writeTGA(stream, image, cTGAImageTypeBGR))
      {
         gConsoleOutput.error("Unable to write to file \"%s\"!", mHeapBitmapFilename.getPtr());
         return false;         
      }
         
      return true;
   }
   
   bool shouldIgnoreSymbol(const BSymbolInfo& symbolInfo)
   {
      for (uint i = 0; i < mLeafSymbolsToIgnore.getSize(); i++)
         if (stricmp(mLeafSymbolsToIgnore[i].getPtr(), symbolInfo.mFilename.getPtr()) == 0)
            return true;
            
      return false;   
   }
   
   void createSymbolTable(void)
   {
      mSymbolHashMap.clear();
      
      for (BActiveAllocHashMap::const_iterator it = mActiveAllocs.begin(); it != mActiveAllocs.end(); ++it)
      {
         const BActiveAlloc& alloc = it->second;
         
         bool leafNode = true;
         const BALContext& context = alloc.mContext;
         for (uint backTraceIndex = 0; backTraceIndex < context.mBackTraceSize; backTraceIndex++)
         {
            const uint address = context.mBackTrace[backTraceIndex];

            BSymbolInfo& symbolInfo = findSymbolByAddress(address)->second;
            
            if (mLeafNodeFiltering)
            {
               if (shouldIgnoreSymbol(symbolInfo))
                  continue;
            }

            symbolInfo.mAllocStats.add(alloc.mCurSize, alloc.mPhysical, !leafNode);
            leafNode = false;
         }
      }
   
      uint index = 0;
      for (BSymbolHashMap::iterator it = mSymbolHashMap.begin(); it != mSymbolHashMap.end(); ++it)
      {
         BSymbolInfo& symInfo = it->second;
         symInfo.mIndex = index;
         
         {  
            BString symbol(symInfo.mSymbol);
            BAllocStats allocStats;                                 
            BAllocStatsHashMap::iterator allocStatsIt = mSymStatsHashMap.insert(symbol).first;
            allocStatsIt->second += symInfo.mAllocStats;
         }
         
         {  
            BString symbol(symInfo.mFilename);
            BAllocStats allocStats;                                 
            BAllocStatsHashMap::iterator allocStatsIt = mFileStatsHashMap.insert(symbol).first;
            allocStatsIt->second += symInfo.mAllocStats;
         }

         index++;
      }
   }
   
   bool openXDBFile(void)
   {
      gConsoleOutput.printf("Reading XDB file: %s\n", mFilenames[0].getPtr());
      
      BCFileStream xdbFileStream;
      if (!xdbFileStream.open(mFilenames[0]))
      {
         gConsoleOutput.error("Unable to open XDB file: %s\n", mFilenames[0].getPtr());
         return false;
      }
      
      if ((xdbFileStream.size() == 0) || (xdbFileStream.size() >= 1024*1024*1024))
      {
         gConsoleOutput.error("XDB file is invalid: %s\n", mFilenames[0].getPtr());
         return false;
      }
      
      mXDBFileData.resize((uint)xdbFileStream.size());
      if (xdbFileStream.readBytes(mXDBFileData.getPtr(), mXDBFileData.getSizeInBytes()) != mXDBFileData.getSizeInBytes())
      {
         gConsoleOutput.error("Can't read XDB file: %s\n", mFilenames[0].getPtr()); 
         return false;
      }
         
      mXDBStream.set(mXDBFileData.getPtr(), mXDBFileData.getSizeInBytes());               
      
      if (!mXDBFileReader.open(&mXDBStream))
      {
         gConsoleOutput.error("Unable to parse XDB file: %s\n", mFilenames[0].getPtr());
         return false;
      }
      
      gConsoleOutput.printf("XDB XEX Checksum: 0x%08X\n", mXDBFileReader.getCheckSum());
      
      return true;
   }
   
   void closeXDBFile(void)
   {
      mXDBFileReader.close();
                 
      mXDBStream.set((void*)NULL, 0);
      
      mXDBFileData.resize(0);
   }
   
   bool openAllocLog(void)
   {
      gConsoleOutput.printf("Reading allocation log file: %s\n", mFilenames[1].getPtr());
      
      if (!mAllocLogStream.open(mFilenames[1]))
      {
         gConsoleOutput.error("Unable to open allocation log file: %s\n", mFilenames[1].getPtr());
         return false;
      }

      if (!mAllocLog.read(&mAllocLogStream))
      {
         gConsoleOutput.error("Unable to parse allocation log file: %s\n", mFilenames[1].getPtr());
         return false;
      }
      
      const BALPacketVersion& versionPacket = mAllocLog.getVersionPacket();
      
      gConsoleOutput.printf("Allocation log total packets: %u\n", mAllocLog.getNumPackets());
      gConsoleOutput.printf("Allocation log XEX Checksum: 0x%08X\n", versionPacket.mXEXChecksum);
      
      if (versionPacket.mXEXChecksum != mXDBFileReader.getCheckSum())
      {
         gConsoleOutput.warning("XDB and allocation log XEX checksums don't match!\n");
      }
      
      mAddressBias = -mAllocLog.getVersionPacket().mXEXBaseAddress + mXDBFileReader.getBaseAddress();
      
      return true;
   }
   
   void closeAllocLog(void)
   {
      mAllocLogStream.close();
   }
   
   bool openOutputFile(void)
   {
      gConsoleOutput.printf("Opening output file: %s\n", mFilenames[2].getPtr());
      
      if (!mOutputStream.open(mFilenames[2], cSFWritable | cSFSeekable))
      {
         gConsoleOutput.error("Unable to open output file: %s\n", mFilenames[2].getPtr());
         return false;
      }
      
      return true;
   }
   
   void closeOutputFile(void)
   {
      mOutputStream.close();
   }
   
   void printHelp(void)
   {
      gConsoleOutput.printf("Usage: memStats database.xdb log.bin destination.txt\n");
      gConsoleOutput.printf("Options:\n");
      gConsoleOutput.printf(" -leafNodeFiltering\n");
      gConsoleOutput.printf(" -saveActiveAllocsToXML\n");
      gConsoleOutput.printf(" -startSnapshotIndex index\n");
      gConsoleOutput.printf(" -endSnapshotIndex index\n");
      gConsoleOutput.printf(" -heapName name\n");
      gConsoleOutput.printf(" -ignoreLeaf name\n");
      gConsoleOutput.printf(" -detailAddress 0x00000000 Dump details about address\n");
      gConsoleOutput.printf(" -heapBitmap filename Create bitmap of allocated blocks\n");
      BConsoleAppHelper::printHelp();
   }
   
   bool parseArgs(BCommandLineParser::BStringArray& args)
   {
      const BCLParam params[] = 
      {
         { "leafNodeFiltering", cCLParamTypeFlag, &mLeafNodeFiltering },
         { "saveActiveAllocsToXML", cCLParamTypeFlag, &mSaveActiveAllocsToXML },
         { "startSnapshotIndex", cCLParamTypeIntPtr, &mStartSnapshotIndex },
         { "endSnapshotIndex", cCLParamTypeIntPtr, &mEndSnapshotIndex },
         { "heapName", cCLParamTypeBStringPtr, &mHeapName },
         { "ignoreLeaf", cCLParamTypeBStringArrayPtr, &mLeafSymbolsToIgnore },
         { "detailAddress", cCLParamTypeBStringPtr, &mDetailAddress },
         { "heapBitmap", cCLParamTypeBStringPtr, &mHeapBitmapFilename }
      };
      const uint cNumCommandLineParams = sizeof(params)/sizeof(params[0]);
      
      BCommandLineParser parser(params, cNumCommandLineParams);
      const bool success = parser.parse(args, true, false);
      if (!success)
      {
         gConsoleOutput.error("%s\n", parser.getErrorString());
         return false;
      }
      
      const BDynamicArray<uint>& unparsedParams = parser.getUnparsedParams();
   
      mNumFilenames = 0;
      
      for (uint j = 0; j < unparsedParams.getSize(); j++)
      {
         const uint i = unparsedParams[j];
         
         if (args[i].isEmpty())
            continue;
            
         if (mNumFilenames >= 3)
         {
            gConsoleOutput.error("Too many filenames!");
            return false;  
         }
                           
         mFilenames[mNumFilenames++] = args[i];
      }
      
      if (mNumFilenames != 3) 
      {
         printHelp();
         return false;
      }
      
      if (mLeafSymbolsToIgnore.getSize())
         mLeafNodeFiltering = true;
         
      if (mDetailAddress.length())
      {
         mDetailAddress.trimLeft();
         mDetailAddress.trimRight();
         mDetailAddress.toLower();
         
         if ((mDetailAddress.length() < 3) || (mDetailAddress.getChar(0) != '0') || (mDetailAddress.getChar(1) != 'x'))
         {
            gConsoleOutput.error("Detail address must be hexidecimal and start with \"0x\": %s", mDetailAddress.getPtr());
            return false;
         }
         
         sscanf_s(mDetailAddress.getPtr(), "0x%X", &mDetailAddressInt);
      }
            
      return true;
   }
      
};

//-------------------------------------------------

int main(int argc, const char *argv[])
{
   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argc, argv))
      return EXIT_FAILURE;
   
   gConsoleOutput.printf("MemStats Compiled %s %s\n", __DATE__, __TIME__);
   
   BMemStatsUtil memStatsUtil;
  
   if (!memStatsUtil.process(args))
   {
      BConsoleAppHelper::deinit();
      return EXIT_FAILURE;
   }

   
   
   BConsoleAppHelper::deinit();
   return EXIT_SUCCESS;
}
