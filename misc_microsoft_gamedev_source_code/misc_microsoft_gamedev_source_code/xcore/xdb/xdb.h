//============================================================================
//  xdb.h
//  Copyright (c) 2006, Ensemble Studios
//============================================================================
#pragma once
#include "stream\cfileStream.h"
#include "resource\ecfUtils.h"
#include "containers\unifier.h"
#include "string\strPathHelper.h"
#include "hash\adler32.h"

//============================================================================
// XDB Chunk ID's
//============================================================================
enum 
{
   cXDBMagic               = 0x34FFDB98,
   cXDBFileDescChunkID     = 0xFF320000,
   cXDBFilenamesChunkID    = 0xFF340001,
   cXDBLineChunkID         = 0xFF340002,
   cXDBSymbolChunkID       = 0xFF320003, 
   cXDBSymbolNamesChunkID  = 0xFF320004 
};

//============================================================================
// struct BXDBFileDesc
//============================================================================
struct BXDBFileDesc
{
   DWORD mCheckSum;
   DWORD mBaseAddress;
   DWORD mNumFilenames;
   DWORD mNumLines;
   DWORD mNumSymbols;

   void endianSwap(void)
   {
      EndianSwitchDWords(reinterpret_cast<DWORD*>(this), 5);
   }
};

//============================================================================
// struct BXDBFilenameDesc
//============================================================================
struct BXDBFilenameDesc
{
   enum { cFilenameBufSize = 32 };
   char mFilename[cFilenameBufSize];
};

//============================================================================
// struct BXDBLineDesc
//============================================================================
struct BXDBLineDesc
{
   DWORD mAddress;
   WORD mLine;
   WORD mFilenameIndex;
   
   void clear(void)
   {
      Utils::ClearObj(*this);
   }

   void endianSwap(void)
   {
      EndianSwitchDWords(&mAddress, 1);
      EndianSwitchWords(&mLine, 2);
   }

   friend bool operator< (const BXDBLineDesc& lhs, const BXDBLineDesc& rhs)
   {
      return lhs.mAddress < rhs.mAddress;
   }
};

//============================================================================
// struct BXDBSymDesc
//============================================================================
struct BXDBSymDesc
{
   DWORD mAddress;
   DWORD mSize;
   DWORD mNameOffset;
   
   void clear(void)
   {
      Utils::ClearObj(*this);
   }

   void endianSwap(void)
   {
      EndianSwitchDWords(reinterpret_cast<DWORD*>(this), 3);
   }

   friend bool operator< (const BXDBSymDesc& lhs, const BXDBSymDesc& rhs)
   {
      return lhs.mAddress < rhs.mAddress;
   }
};

//============================================================================
// class BXDBFileBuilder
//============================================================================
class BXDBFileBuilder
{
public:
   BXDBFileBuilder();
   ~BXDBFileBuilder();

   void clear(void);
   
   void addSymbol(DWORD address, DWORD size, const char* pName);

   uint getNumSymbols(void) const { return mSymDescArray.getSize(); }

   bool addLine(DWORD address, DWORD line, const char* pFilename);

   uint getNumLines(void) const { return mLineDescArray.getSize(); }

   void setBaseAddress(DWORD baseAddress);

   void setChecksum(DWORD checkSum);

   bool writeFile(BStream& stream);

private:
   typedef Unifier<BString> BStringUnifier;
   BStringUnifier mUniqueFilenames;

   typedef BDynamicArray<BXDBLineDesc> BLineDescArray;
   BLineDescArray mLineDescArray;

   typedef BDynamicArray<BXDBSymDesc> BSymDescArray;
   BSymDescArray mSymDescArray;

   BDynamicBYTEArray mSymbolNames;

   DWORD mBaseAddress;
   DWORD mCheckSum;

   void sort(void);

   void createFilenameChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder);

   void createLineChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder);

   void createSymbolChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder);

   void createSymbolNamesChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder);
};

//============================================================================
// class BXDBFileReader
//============================================================================
class BXDBFileReader
{
public:
   BXDBFileReader();
   ~BXDBFileReader();

   bool open(BStream* pStream);

   void close(void);

   struct BLookupInfo
   {
      DWORD mAddress;
      int mLine;
      bool mFoundSymbol;
      bool mFoundLine;
      BFixedString256 mSymbol;
      BFixedString<BXDBFilenameDesc::cFilenameBufSize> mFilename;
      
      void clear(void);
   };

   bool lookup(DWORD address, BLookupInfo& info);
   
   uint getCheckSum(void) const { return mFileDesc.mCheckSum; }
   uint getBaseAddress(void) const { return mFileDesc.mBaseAddress; }
   
   const BXDBFileDesc& getFileDesc(void) const { return mFileDesc; }

private:
   BStream* mpStream;
   BECFFileStream mECFStream;
   BXDBFileDesc mFileDesc;

   int mFilenamesChunkOfs;
   int mLineChunkOfs;
   int mSymbolChunkOfs;
   int mSymbolNamesChunkOfs;
};


