//============================================================================
//  xdb.cpp
//  Copyright (c) 2006, Ensemble Studios
//============================================================================
#include "xcore.h"
#include "xdb.h"

//============================================================================
// BXDBFileBuilder::BXDBFileBuilder
//============================================================================
BXDBFileBuilder::BXDBFileBuilder() :
   mBaseAddress(0),
   mCheckSum(0)
{
}

//============================================================================
// BXDBFileBuilder::~BXDBFileBuilder
//============================================================================
BXDBFileBuilder::~BXDBFileBuilder()
{
}

//============================================================================
// BXDBFileBuilder::clear
//============================================================================
void BXDBFileBuilder::clear(void)
{
   mUniqueFilenames.clear();
   mSymbolNames.clear();
   mSymDescArray.clear();
   mLineDescArray.clear();
}

//============================================================================
// BXDBFileBuilder::addSymbol
//============================================================================
void BXDBFileBuilder::addSymbol(DWORD address, DWORD size, const char* pName)
{
   const DWORD nameOfs = mSymbolNames.getSize();
   mSymbolNames.pushBack(reinterpret_cast<const uchar*>(pName), strlen(pName) + 1);

   BXDBSymDesc* p = mSymDescArray.pushBackNoConstruction(1);
   p->mAddress = address;
   p->mNameOffset = nameOfs;
   p->mSize = size;
}

//============================================================================
// BXDBFileBuilder::addLine
//============================================================================
bool BXDBFileBuilder::addLine(DWORD address, DWORD line, const char* pFilename)
{
   const BStringUnifier::InsertResult insertResult = mUniqueFilenames.insert(BString(pFilename));
   const DWORD filenameIndex = insertResult.first;

   if (filenameIndex > 0xFFFF)
      return false;

   BXDBLineDesc* p = mLineDescArray.pushBackNoConstruction(1);
   p->mAddress = address;
   p->mFilenameIndex = (WORD)filenameIndex;
   p->mLine = (WORD)Math::Min<DWORD>(0xFFFF, line);

   return true;
}

//============================================================================
// BXDBFileBuilder::setBaseAddress
//============================================================================
void BXDBFileBuilder::setBaseAddress(DWORD baseAddress)
{
   mBaseAddress = baseAddress;
}

//============================================================================
// BXDBFileBuilder::setChecksum
//============================================================================
void BXDBFileBuilder::setChecksum(DWORD checkSum)
{
   mCheckSum = checkSum;
}

//============================================================================
// BXDBFileBuilder::writeFile
//============================================================================
bool BXDBFileBuilder::writeFile(BStream& stream)
{
   sort();

   BECFFileBuilder ecfBuilder;
   ecfBuilder.setID(cXDBMagic);

   BXDBFileDesc fileDesc;
   Utils::ClearObj(fileDesc);
   fileDesc.mCheckSum = mCheckSum;
   fileDesc.mBaseAddress = mBaseAddress;

   ecfBuilder.addChunk((uint64)cXDBFileDescChunkID, reinterpret_cast<const BYTE*>(&fileDesc), sizeof(fileDesc));

   createSymbolNamesChunk(fileDesc, ecfBuilder);
   createFilenameChunk(fileDesc, ecfBuilder);  

   createSymbolChunk(fileDesc, ecfBuilder);
   createLineChunk(fileDesc, ecfBuilder);

   BECFChunkData& fileDescChunkData = ecfBuilder.getChunkByIndex(0);      
   fileDescChunkData.getDataArray().resize(sizeof(fileDesc));
   memcpy(fileDescChunkData.getDataArray().getPtr(), &fileDesc, sizeof(fileDesc));

   return ecfBuilder.writeToStream(stream);
}

//============================================================================
// BXDBFileBuilder::sort
//============================================================================
void BXDBFileBuilder::sort(void)
{
   mLineDescArray.sort();
   mSymDescArray.sort();
}

//============================================================================
// BXDBFileBuilder::createFilenameChunk
//============================================================================
void BXDBFileBuilder::createFilenameChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder)
{
   typedef BDynamicArray<BXDBFilenameDesc> BFilenameDescArray;
   BFilenameDescArray filenameDescArray;

   const BStringUnifier::ObjectVector& filenames = mUniqueFilenames.getObjects();
   if (filenames.empty())
      return;

   for (uint i = 0; i < filenames.getSize(); i++)
   {
      BString path;
      BString filename;
      strPathSplit(filenames[i], path, filename);

      BFixedString<BXDBFilenameDesc::cFilenameBufSize> name;
      name.clear();
      name.set(filename.getPtr());

      BXDBFilenameDesc filenameDesc;
      memcpy(filenameDesc.mFilename, &name, sizeof(filenameDesc.mFilename));
      filenameDescArray.pushBack(filenameDesc);
   }

   ecfBuilder.addChunk((uint64)cXDBFilenamesChunkID, reinterpret_cast<const BYTE*>(filenameDescArray.getPtr()), filenameDescArray.getSizeInBytes());

   fileDesc.mNumFilenames = filenames.getSize();
}

//============================================================================
// BXDBFileBuilder::createLineChunk
//============================================================================
void BXDBFileBuilder::createLineChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder)
{
   if (mLineDescArray.empty())
      return;
   ecfBuilder.addChunk((uint64)cXDBLineChunkID, reinterpret_cast<const BYTE*>(mLineDescArray.getPtr()), mLineDescArray.getSizeInBytes());
   fileDesc.mNumLines = mLineDescArray.getSize();
}

//============================================================================
// BXDBFileBuilder::createSymbolChunk
//============================================================================
void BXDBFileBuilder::createSymbolChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder)
{
   if (mSymDescArray.empty())
      return;

   ecfBuilder.addChunk((uint64)cXDBSymbolChunkID, reinterpret_cast<const BYTE*>(mSymDescArray.getPtr()), mSymDescArray.getSizeInBytes());
   fileDesc.mNumSymbols = mSymDescArray.getSize();
}

//============================================================================
// BXDBFileBuilder::createSymbolNamesChunk
//============================================================================
void BXDBFileBuilder::createSymbolNamesChunk(BXDBFileDesc& fileDesc, BECFFileBuilder& ecfBuilder)
{
   fileDesc;

   if (mSymbolNames.empty())
      return;

   ecfBuilder.addChunk((uint64)cXDBSymbolNamesChunkID, mSymbolNames.getPtr(), mSymbolNames.getSizeInBytes());
}

//============================================================================
// BXDBFileReader::BXDBFileReader
//============================================================================   
BXDBFileReader::BXDBFileReader() :
   mpStream(NULL),
   mFilenamesChunkOfs(-1),
   mLineChunkOfs(-1),
   mSymbolChunkOfs(-1),
   mSymbolNamesChunkOfs(-1)
{
}

//============================================================================
// BXDBFileReader::~BXDBFileReader
//============================================================================
BXDBFileReader::~BXDBFileReader()
{
}

//============================================================================
// BXDBFileReader::open
//============================================================================
bool BXDBFileReader::open(BStream* pStream)
{
   close();
      
   if (!mECFStream.open(pStream))
      return false;

   const BECFHeader* pHeader;
   if (!mECFStream.getHeaderPtr(pHeader))
      return false;

   if (pHeader->getID() != cXDBMagic)
      return false;

   int chunkIndex = mECFStream.findChunkByID((uint64)cXDBFileDescChunkID, 0);
   if (chunkIndex < 0)
      return false;

   if (!mECFStream.seekToChunk(chunkIndex))
      return false;

   if (!pStream->readBytes(&mFileDesc, sizeof(mFileDesc)))
      return false;

   if (cBigEndianNative)
      mFileDesc.endianSwap();

   mFilenamesChunkOfs = (int)mECFStream.getChunkOfsByID((uint64)cXDBFilenamesChunkID);
   mLineChunkOfs = (int)mECFStream.getChunkOfsByID((uint64)cXDBLineChunkID);
   mSymbolChunkOfs = (int)mECFStream.getChunkOfsByID((uint64)cXDBSymbolChunkID);
   mSymbolNamesChunkOfs = (int)mECFStream.getChunkOfsByID((uint64)cXDBSymbolNamesChunkID);

   if ((mFilenamesChunkOfs < 0) || (mLineChunkOfs < 0) || (mSymbolChunkOfs < 0) || (mSymbolNamesChunkOfs < 0))
      return false;

   mpStream = pStream;
   
   return true;
}

//============================================================================
// BXDBFileReader::close
//============================================================================
void BXDBFileReader::close(void)
{
   mECFStream.close();

   mpStream = NULL;   

   mFilenamesChunkOfs = -1;
   mLineChunkOfs = -1;
   mSymbolChunkOfs = -1;
   mSymbolNamesChunkOfs = -1;      
}

//============================================================================
// BXDBFileReader::lookup
//============================================================================
bool BXDBFileReader::lookup(DWORD address, BLookupInfo& info)
{
   __try
   {
      info.mAddress = address;
      info.mLine = -1;
      info.mFilename.set("?");
      info.mSymbol.clear();
      info.mSymbol.set("?");
      info.mFoundLine = false;
      info.mFoundSymbol = false;
      
      if (!mpStream)
         return false;

      int l = 0;
      int r = mFileDesc.mNumSymbols - 1;

      BXDBSymDesc symDesc;
      symDesc.clear();
      while (r >= l)
      {
         const int m = (l + r) >> 1;

         if (!mpStream->seek(mSymbolChunkOfs + m * sizeof(BXDBSymDesc)))
            return false;

         if (mpStream->readBytes(&symDesc, sizeof(BXDBSymDesc)) < sizeof(BXDBSymDesc))
            return false;

         if (cBigEndianNative)
            symDesc.endianSwap();

         if (symDesc.mSize == 0)
         {
            if (address == symDesc.mAddress)
               break;
         }
         else
         {
            if ((address >= symDesc.mAddress) && (address < (symDesc.mAddress + symDesc.mSize)))
               break;
         }

         if (address < symDesc.mAddress)
            r = m - 1;
         else
            l = m + 1;
      }

      info.mFoundSymbol = (r >= l);

      if (info.mFoundSymbol)
      {
         const uint nameOffset = symDesc.mNameOffset;

         if (!mpStream->seek(mSymbolNamesChunkOfs + nameOffset))
            return false;

         mpStream->readBytes(&info.mSymbol, sizeof(info.mSymbol) - 1);
      }         

      l = 0;
      r = mFileDesc.mNumLines - 1;
      BXDBLineDesc lineDesc0;
      BXDBLineDesc lineDesc1;
      lineDesc0.clear();
      lineDesc1.clear();

      while (r >= l)
      {
         const int m = (l + r) >> 1;

         if (!mpStream->seek(mLineChunkOfs + m * sizeof(BXDBLineDesc)))
            return false;

         if (mpStream->readBytes(&lineDesc0, sizeof(BXDBLineDesc)) < sizeof(BXDBLineDesc))
            return false;

         if (cBigEndianNative)
            lineDesc0.endianSwap();

         if (m < (int)mFileDesc.mNumLines - 1)
         {
            if (mpStream->readBytes(&lineDesc1, sizeof(BXDBLineDesc)) < sizeof(BXDBLineDesc))
               return false;

            if (cBigEndianNative)               
               lineDesc1.endianSwap();
         }
         else
         {
            lineDesc1 = lineDesc0;
            lineDesc1.mAddress += 65536;
         }

         if ((address >= lineDesc0.mAddress) && (address < lineDesc1.mAddress))
            break;

         if (address < lineDesc0.mAddress)
            r = m - 1;
         else
            l = m + 1;
      }

      if (r < l)
         return info.mFoundSymbol;

      info.mFoundLine = true;
      info.mLine = lineDesc0.mLine;

      const uint filenameOffset = lineDesc0.mFilenameIndex;

      if (!mpStream->seek(mFilenamesChunkOfs + filenameOffset * sizeof(BXDBFilenameDesc)))
         return false;

      BXDBFilenameDesc filenameDesc;
      if (mpStream->readBytes(&filenameDesc, sizeof(BXDBFilenameDesc)) < sizeof(BXDBFilenameDesc))
         return false;

      info.mFilename.set(filenameDesc.mFilename);
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return false;
   }   

   return true;
}

//============================================================================
// BXDBFilenameDesc::BLookupInfo::clear
//============================================================================
void BXDBFileReader::BLookupInfo::clear(void)
{
   mAddress = 0;
   mLine = 0;
   mFoundSymbol = 0;
   mFoundLine = 0;
   mSymbol.empty();
   mFilename.empty();
}
