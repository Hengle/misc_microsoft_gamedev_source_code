//============================================================================
//
//  File: ecfArchiver.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "ecfArchiver.h"
#include "hash\digitalSignature.h"
#include "hash\tiger.h"
#include "hash\adler32.h"
#include "resource\ecfHeaderIDs.h"
#include "stream\dynamicStream.h"
#include "stream\encryptedStream.h"
#include "stream\byteStream.h"
#include "compressedStream.h"
#include "deflate.h"

#ifndef XBOX
   #include "utils\consoleAppHelper.h"
#endif

BECFArchiver::BECFArchiver() :
   mpSrcArchiveFilename(NULL),
   mpSrcFilelist(NULL),
   mpDstFilename(NULL),
   mpSignatureSecretKey(NULL),
   mChunkDataAlignmentLog2(0)
{
}

bool BECFArchiver::createArchiveFilename(BECFArchiveChunkHeader& chunkHeader, BByteArray& filenameData, const char* pBaseDirFilename, const BString& srcFilename)
{
   BString archiveFilename(srcFilename);
   archiveFilename.standardizePath();
   
   if (archiveFilename.findLeft("secretKey") != -1)
   {
      gConsoleOutput.error("BECFArchiver::createArchiveFilename: Refusing to create archive with secret key data: %s\n", srcFilename.getPtr());
      return false;
   }
      
   BString baseDirFilename(pBaseDirFilename);
   baseDirFilename.standardizePath();
   
   int i = archiveFilename.findLeft(baseDirFilename);
   if (-1 != i)
   {
      int firstChar = i + baseDirFilename.length();
      
      if (firstChar >= archiveFilename.length())
      {
         gConsoleOutput.error("BECFArchiver::createArchiveFilename: Failed creating archive filename!\n");
         return false;
      }
      
      archiveFilename.crop(firstChar, archiveFilename.length() - 1);
   }
   
   if (!archiveFilename.isEmpty())
   {
      if (archiveFilename.getChar(0) == '\\')
         archiveFilename.right(1);
   }
   
   chunkHeader.mNameOfs[0] = (uchar)((filenameData.getSize() >> 16) & 0xFF);
   chunkHeader.mNameOfs[1] = (uchar)((filenameData.getSize() >> 8) & 0xFF);
   chunkHeader.mNameOfs[2] = (uchar)(filenameData.getSize() & 0xFF);

   for (int i = 0; i < archiveFilename.length(); i++)
      filenameData.pushBack(archiveFilename.getChar(i));
   filenameData.pushBack(0);
   gConsoleOutput.printf("Archive filename: \"%s\"\n", archiveFilename.getPtr());   
   
   return true;
}

bool BECFArchiver::beginChunk(BECFArchiveChunkHeader& chunkHeader)
{
   if (mTempFileStream.curOfs() >= 0xFFFFFFFF)
   {
      gConsoleOutput.error("BECFArchiver::beginChunk: Archive is too big!\n");
      return false;
   }
   
   chunkHeader.setOfs((DWORD)mTempFileStream.curOfs());
   const uint bytesToAlignUp = Utils::BytesToAlignUpValue(chunkHeader.getOfs(), 1U << chunkHeader.getAlignmentLog2());
   if (bytesToAlignUp)
   {
      if (mTempFileStream.writeDuplicateBytes(0, bytesToAlignUp) != bytesToAlignUp)
      {
         gConsoleOutput.error("BECFArchiver::beginChunk: Failed writing to temp file!\n");
         return false;
      }
      chunkHeader.setOfs(chunkHeader.getOfs() + bytesToAlignUp);
   }
   return true;
}

bool BECFArchiver::compressFile(BECFArchiveChunkHeader& chunkHeader, const BString& srcFilename)
{
   chunkHeader.setFlags(cECFCompMethodDeflateRaw);

   BWin32FileStream srcFileStream;
   if (!srcFileStream.open(srcFilename, cSFReadable | cSFSeekable | cSFOptimizeForSequentialAccess))
   {
      gConsoleOutput.error("BECFArchiver::compressFile: Unable to open file for reading: %s\n", srcFilename.getPtr());
      return false; 
   }
   
   if (!beginChunk(chunkHeader))
      return false;

   BByteArray srcBuf(65536);
   BByteArray dstBuf(65536);

   BDeflate* pDeflate = new BDeflate;
   pDeflate->init();

   uint compAdler32 = INIT_ADLER32;
   uint decompAdler32 = INIT_ADLER32;
   BTigerHashGen decompTigerHashGen;
   BTigerHashGen compTigerHashGen;

   uint srcBytesRemaining = chunkHeader.mDecompSize;
   int compStatus = 0;
   do 
   {
      const uint srcBytesRead = Math::Min(srcBuf.getSize(), srcBytesRemaining);
      
      if (srcBytesRead)
      {
         if (srcFileStream.readBytes(srcBuf.getPtr(), srcBytesRead) != srcBytesRead)
         {
            delete pDeflate;
            pDeflate = NULL;

            gConsoleOutput.error("BECFArchiver::compressFile: Unable to read from source file: %s\n", srcFilename.getPtr());
            return false;
         }
         
         srcBytesRemaining -= srcBytesRead;
         
         decompAdler32 = calcAdler32(srcBuf.getPtr(), srcBytesRead, decompAdler32);
         decompTigerHashGen.update(srcBuf.getPtr(), srcBytesRead);
      }            
      
      const int eofFlag = (srcBytesRemaining == 0);
            
//-- FIXING PREFIX BUG ID 6277
      const uchar* pInBuf = srcBuf.getPtr();
//--
      uint inBufLeft = srcBytesRead;

      do
      {
         int inBufSize = inBufLeft;
         int outBufSize = dstBuf.getSize();

         // Returns DEFL_STATUS_OKAY or DEFL_STATUS_DONE.
         compStatus = pDeflate->compress(pInBuf, &inBufSize, dstBuf.getPtr(), &outBufSize, eofFlag, 1000);

         if (outBufSize)
         {
            compAdler32 = calcAdler32(dstBuf.getPtr(), outBufSize, compAdler32);
            compTigerHashGen.update(dstBuf.getPtr(), outBufSize);

            if (mTempFileStream.writeBytes(dstBuf.getPtr(), outBufSize) != (uint)outBufSize)
            {
               delete pDeflate;
               pDeflate = NULL;
               
               gConsoleOutput.error("BECFArchiver::compressFile: Failed writing to temp file!\n");
               return false;
            }
            
            if (((uint64)chunkHeader.getSize() + (uint64)outBufSize) >= 0xFFFFFFFF)
            {
               delete pDeflate;
               pDeflate = NULL;
               
               gConsoleOutput.error("BECFArchiver::compressFile: Compressed data is too big!\n");
               return false;
            }
            
            chunkHeader.setSize(chunkHeader.getSize() + outBufSize);
         }
         
         pInBuf += inBufSize;
         inBufLeft -= inBufSize;
         
      } while ((compStatus != DEFL_STATUS_DONE) && ((inBufLeft) || (eofFlag)));
      
   } while (compStatus != DEFL_STATUS_DONE);
   
   delete pDeflate;
   pDeflate = NULL;
   
   BTigerHash decompTigerHash(decompTigerHashGen.finalize());
   chunkHeader.setID(decompTigerHash.getQWORD(0));      
   
   BTigerHash compTigerHash(compTigerHashGen.finalize());
   for (uint i = 0; i < 16; i++)
      chunkHeader.mCompTiger128[i] = compTigerHash[i];
   
   chunkHeader.setAdler32(compAdler32);
   
   const float cStoreThreshold = .96f;
   if (chunkHeader.getSize() >= chunkHeader.mDecompSize * cStoreThreshold)
   {
      chunkHeader.setFlags(cECFCompMethodStored);
      
      gConsoleOutput.printf("File data isn't compressible enough, storing instead.\n");
      
      if (srcFileStream.seek(0) != 0)
      {
         gConsoleOutput.error("Failed seeking within source file!\n");
         return false;
      }
      
      if (mTempFileStream.seek(chunkHeader.getOfs()) != chunkHeader.getOfs())
      {
         gConsoleOutput.error("Failed seeking within temp file!\n");
         return false;
      }
      
      chunkHeader.setAdler32(decompAdler32);
      chunkHeader.setSize(chunkHeader.mDecompSize);
      for (uint i = 0; i < 16; i++)
         chunkHeader.mCompTiger128[i] = decompTigerHash[i];
      
      uint64 bytesCopied = 0;
      if (!BStream::copyStream(srcFileStream, mTempFileStream, &bytesCopied))
      {
         gConsoleOutput.error("Failed copying source data!\n");
         return false;
      }     
      
      if (bytesCopied != chunkHeader.getSize())
      {
         gConsoleOutput.error("Failed copying source data!\n");
         return false;
      }
      
      if (!mTempFileStream.getFile()->extendFile(mTempFileStream.curOfs()))
      {
         gConsoleOutput.error("Failed extending temp file!\n");
         return false;
      }
   }
               
   return true;
}

bool BECFArchiver::createFilenameChunk(BECFArchiveChunkHeader& chunkHeader, const BByteArray& filenameData)
{
   if (!beginChunk(chunkHeader))
      return false;
      
   chunkHeader.mDecompSize = filenameData.getSize();
   chunkHeader.setFlags(cECFCompMethodDeflateStream);
           
   BDynamicStream compFilenameData;
   BDeflateStream deflStream;
   if (!deflStream.open(compFilenameData, cSFWritable))
   {
      gConsoleOutput.error("BECFArchiver::createFilenameChunk: Failed compressing filename data!\n");
      return false;
   }
   
   if (deflStream.writeBytes(filenameData.getPtr(), filenameData.getSizeInBytes()) != filenameData.getSizeInBytes())
   {
      gConsoleOutput.error("BECFArchiver::createFilenameChunk: Failed compressing filename data!\n");
      return false;
   }
   
   if (!deflStream.close())
   {
      gConsoleOutput.error("BECFArchiver::createFilenameChunk: Failed compressing filename data!\n");
      return false;
   }
   
   gConsoleOutput.printf("Filename chunk compressed from %u to %u bytes\n", filenameData.getSizeInBytes(), compFilenameData.size());
   
   BTigerHashGen compTigerHashGen(compFilenameData.getBuf().getPtr(), (uint)compFilenameData.size());
   BTigerHash compTigerHash(compTigerHashGen.finalize());
   for (uint i = 0; i < 16; i++)
      chunkHeader.mCompTiger128[i] = compTigerHash[i];

   chunkHeader.setAdler32(calcAdler32(compFilenameData.getBuf().getPtr(), (uint)compFilenameData.size()));

   BTigerHashGen decompTigerHashGen(filenameData.getPtr(), filenameData.getSizeInBytes());
   BTigerHash decompTigerHash(decompTigerHashGen.finalize());
   chunkHeader.setID(decompTigerHash.getQWORD(0));
   chunkHeader.setSize((uint)compFilenameData.size());
   
   if (mTempFileStream.writeBytes(compFilenameData.getBuf().getPtr(), (uint)compFilenameData.size()) != compFilenameData.size())
   {
      gConsoleOutput.error("BECFArchiver::createFilenameChunk: Failed writing to temp file!\n");
      return false;
   }
   
   return true;
}

bool BECFArchiver::writeArchiveHeader(uint conservativeHeaderSize)
{
   BByteArray archiveHeader(conservativeHeaderSize);

   BECFArchiveHeader& archiveChunkHeader = *reinterpret_cast<BECFArchiveHeader*>(archiveHeader.getPtr());

   archiveChunkHeader.setID(cArchiveECFHeaderID);
   archiveChunkHeader.setChunkExtraDataSize(sizeof(BECFArchiveChunkHeader) - sizeof(BECFChunkHeader));
   archiveChunkHeader.setMagic((DWORD)BECFHeader::cECFHeaderMagic);
   archiveChunkHeader.setSize(conservativeHeaderSize);
   archiveChunkHeader.setNumChunks(mArchiveChunkHeaders.getSize());
   if (mTempFileStream.size() >= 0xFFFFFFFE)
   {
      gConsoleOutput.error("BECFArchiver::writeArchiveHeader: Archive is too big!\n");
      return false;
   }
   
   archiveChunkHeader.setFileSize((DWORD)mTempFileStream.size());
   
   BSHA1Gen sha1Gen;
   sha1Gen.update32(0xA7F95F9C);
   sha1Gen.update32(conservativeHeaderSize);
   sha1Gen.update32(archiveChunkHeader.getNumChunks());
   sha1Gen.update32(archiveChunkHeader.getChunkExtraDataSize());
   sha1Gen.update32(archiveChunkHeader.getFileSize());
   sha1Gen.update(mArchiveChunkHeaders.getPtr(), mArchiveChunkHeaders.getSizeInBytes());
   BSHA1 archiveDigest(sha1Gen.finalize());
   
   gConsoleOutput.printf("Computing digital signature\n");
   
   BDynamicStream signatureStream;
   BDigitalSignature digitalSignature;
   if (!digitalSignature.signMessage(signatureStream, archiveDigest, *mpSignatureSecretKey))
   {  
      gConsoleOutput.error("BECFArchiver::writeArchiveHeader: Failed computing signature!\n");      
      return false;
   }

   gConsoleOutput.printf("Digital signature size: %u bytes\n", signatureStream.size());

   archiveChunkHeader.mArchiveHeaderMagic = BECFArchiveHeader::cMagic;
   archiveChunkHeader.mSignatureSize = (uint)signatureStream.size();

   const uint actualHeaderSize = sizeof(BECFArchiveChunkHeader) + (uint)signatureStream.size();

   if (actualHeaderSize > conservativeHeaderSize)      
   {
      gConsoleOutput.error("BECFArchiver::writeArchiveHeader: Actual header size is too large!\n");
      return false;  
   }
   
   memcpy(&archiveChunkHeader + 1, signatureStream.getBuf().getPtr(), (size_t)signatureStream.size());
   
   archiveChunkHeader.setAdler32(
      calcAdler32((DWORD*)&archiveChunkHeader + BECFHeader::cECFAdler32DWORDsToSkip, conservativeHeaderSize - BECFHeader::cECFAdler32DWORDsToSkip * sizeof(DWORD)));
   
   if (mTempFileStream.seek(0) != 0)
   {
      gConsoleOutput.error("BECFArchiver::writeArchiveHeader: Failed writing to temp file!\n");
      return false;  
   }   
   
   if (mTempFileStream.writeBytes(archiveHeader.getPtr(), archiveHeader.getSize()) != archiveHeader.getSize())
   {
      gConsoleOutput.error("BECFArchiver::writeArchiveHeader: Failed writing to temp file!\n");
      return false;  
   }

   return true;
}

bool BECFArchiver::build(
   const char* pBaseDirFilename,
   const char* pDstFilename,
   const char* pTmpFilename,
   const BDynamicArray<BString>& srcFilenames,
   BStream& signatureSecretKey,
   uint chunkDataAlignmentLog2,
   BString* pEncryptionPassword)
{
   BDEBUG_ASSERT(pBaseDirFilename);
   BDEBUG_ASSERT(pDstFilename);
   BDEBUG_ASSERT(pTmpFilename);
   BDEBUG_ASSERT(chunkDataAlignmentLog2 >= 2);
   BDEBUG_ASSERT(signatureSecretKey.getFlags() & cSFSeekable);
      
   mpDstFilename = pDstFilename;
   mpSrcFilelist = &srcFilenames;
   mpSignatureSecretKey = &signatureSecretKey;
   mChunkDataAlignmentLog2 = chunkDataAlignmentLog2;
   
   if (pEncryptionPassword)
   {
      if (pEncryptionPassword->length() < 6)
      {
         gConsoleOutput.error("BECFArchiver::build: Archive encryption password must be at least 6 characters!\n");
         return false;
      }
   }
   
   if (srcFilenames.isEmpty())
   {
      gConsoleOutput.error("BECFArchiver::build: No files to process!\n");
      return false;
   }
         
   if (!mTempFileStream.open(pTmpFilename, cSFReadable | cSFWritable | cSFSeekable | cSFOptimizeForSequentialAccess))
   {
      gConsoleOutput.error("BECFArchiver::build: Failed opening temp file: %s\n", pTmpFilename);
      return false;
   }
      
   const uint64 origSecretKeyOfs = signatureSecretKey.curOfs();
   BDigitalSignature digitalSignature;
   BSHA1 messageDigest;
   BDynamicStream signatureStream;
   
   if (!digitalSignature.signMessage(signatureStream, messageDigest, signatureSecretKey))
   {
      gConsoleOutput.error("BECFArchiver::build: Failed computing test signature!\n");
      return false;
   }
   
   if ((uint64)signatureSecretKey.seek(origSecretKeyOfs) != origSecretKeyOfs)
   {
      gConsoleOutput.error("BECFArchiver::build: Failed computing test signature!\n");
      return false;
   }
   
   // This won't be the exact header size, because the signature may vary a bit depending on the exact authentication path 
   // used by the digital signature code. But it should be close.
   const int conservativeHeaderSize = Utils::AlignUpValue(sizeof(BECFArchiveChunkHeader) + (uint)signatureStream.size() + 512, 512);
   
   if (mTempFileStream.writeDuplicateBytes(0, conservativeHeaderSize) != conservativeHeaderSize)
   {
      gConsoleOutput.error("BECFArchiver::build: Failed writing to temp file: %s\n", pTmpFilename);
      return false;
   }
   
   mArchiveChunkHeaders.resize(1 + mpSrcFilelist->getSize());
   for (uint i = 0; i < mArchiveChunkHeaders.getSize(); i++)
      mArchiveChunkHeaders[i].setAlignmentLog2(mChunkDataAlignmentLog2);
   
   if (mTempFileStream.writeDuplicateBytes(0, mArchiveChunkHeaders.getSizeInBytes()) != mArchiveChunkHeaders.getSizeInBytes())
   {
      gConsoleOutput.error("BECFArchiver::build: Failed writing to temp file: %s\n", pTmpFilename);
      return false;
   }
      
   BByteArray filenameData;
   gConsoleOutput.printf("Scanning source files\n");
   
   for (uint srcFileIndex = 0; srcFileIndex < srcFilenames.getSize(); srcFileIndex++)
   {
      BECFArchiveChunkHeader& chunkHeader = mArchiveChunkHeaders[1 + srcFileIndex];
      
      const BString& srcFilename = srcFilenames[srcFileIndex];
                            
      WIN32_FILE_ATTRIBUTE_DATA srcFileAttribs;
          
      if (!GetFileAttributesEx(srcFilename, GetFileExInfoStandard, &srcFileAttribs))
      {
         gConsoleOutput.error("BECFArchiver::build: Unable to get attributes of file: %s\n", srcFilename.getPtr());
         return false;
      }
      
      if ((srcFileAttribs.nFileSizeLow | srcFileAttribs.nFileSizeHigh) == 0)
      {
         // I will probably regret this limitation.
         gConsoleOutput.error("BECFArchiver::build: Sorry, archiver doesn't support zero length files: %s\n", srcFilename.getPtr());
         return false;
      }
      
      if ((srcFileAttribs.nFileSizeHigh) || (srcFileAttribs.nFileSizeLow == 0xFFFFFFFF))
      {
         gConsoleOutput.error("BECFArchiver::build: Sorry, file is too big: %s\n", srcFilename.getPtr());
         return false;
      }
                  
      chunkHeader.mDate = Math::Max<uint64>(Utils::FileTimeToUInt64(srcFileAttribs.ftCreationTime), Utils::FileTimeToUInt64(srcFileAttribs.ftLastWriteTime));
      chunkHeader.mDecompSize = srcFileAttribs.nFileSizeLow;
      
      if (!createArchiveFilename(chunkHeader, filenameData, pBaseDirFilename, srcFilename))
         return false;
   }
   
   if (!createFilenameChunk(mArchiveChunkHeaders[0], filenameData))
      return false;
   
   for (uint srcFileIndex = 0; srcFileIndex < srcFilenames.getSize(); srcFileIndex++)         
   {  
      BECFArchiveChunkHeader& chunkHeader = mArchiveChunkHeaders[1 + srcFileIndex];
      const BString& srcFilename = srcFilenames[srcFileIndex];
      
      gConsoleOutput.printf("Compressing file %i of %i: %s, %u bytes\n", srcFileIndex + 1, srcFilenames.getSize(), srcFilename.getPtr(), (uint)chunkHeader.mDecompSize);
                             
      if (!compressFile(chunkHeader, srcFilename))
         return false;      
         
      gConsoleOutput.printf("Compressed to %u bytes (%3.1f%%)\nComp Adler32: 0x%08X, Tiger128: 0x%02X%02X%02X%02X%02X%02X%02X%02X 0x%02X%02X%02X%02X%02X%02X%02X%02X\n", 
         chunkHeader.getSize(), 
         (chunkHeader.getSize() * 100.0f) / Math::Max<uint>(1U, chunkHeader.mDecompSize),
         chunkHeader.getAdler32(),
         chunkHeader.mCompTiger128[15], chunkHeader.mCompTiger128[14], chunkHeader.mCompTiger128[13], chunkHeader.mCompTiger128[12],
         chunkHeader.mCompTiger128[11], chunkHeader.mCompTiger128[10], chunkHeader.mCompTiger128[9], chunkHeader.mCompTiger128[8], 
         chunkHeader.mCompTiger128[7], chunkHeader.mCompTiger128[6], chunkHeader.mCompTiger128[5], chunkHeader.mCompTiger128[4], 
         chunkHeader.mCompTiger128[3], chunkHeader.mCompTiger128[2], chunkHeader.mCompTiger128[1], chunkHeader.mCompTiger128[0]);

      gConsoleOutput.printf("Original Tiger64: 0x%08I64X\n", chunkHeader.getID());
   }
   
   if (mTempFileStream.writeDuplicateBytes(0, 4095) != 4095)
   {
      gConsoleOutput.error("BECFArchiver::build: Failed writing to temp file: %s\n", pTmpFilename);
      return false;
   }
   
   const uint64 bytesToAlignUp = Utils::BytesToAlignUpValue<uint64>(mTempFileStream.size(), 4096);
   if (bytesToAlignUp)
   {
      if (mTempFileStream.writeDuplicateBytes(0, bytesToAlignUp) != bytesToAlignUp)
      {
         gConsoleOutput.error("BECFArchiver::build: Failed writing to temp file: %s\n", pTmpFilename);
         return false;
      }
   }
   
   if (!writeArchiveHeader(conservativeHeaderSize))
      return false;
      
   BVERIFY(mTempFileStream.curOfs() == conservativeHeaderSize);      
      
   if (mTempFileStream.writeBytes(mArchiveChunkHeaders.getPtr(), mArchiveChunkHeaders.getSizeInBytes()) != mArchiveChunkHeaders.getSizeInBytes())
   {
      gConsoleOutput.error("BECFArchiver::build: Failed writing to temp file: %s\n", pTmpFilename);
      return false;
   }
   
   BVERIFY(mTempFileStream.curOfs() == mArchiveChunkHeaders[0].getOfs());
   
   const uint64 archiveSize = mTempFileStream.size();
   if (archiveSize >= 0xFFFFFFFF)
   {
      gConsoleOutput.error("BECFArchiver::build: Archive is too big!\n");
      return false;
   }
         
   DeleteFile(pDstFilename);
   
   if (pEncryptionPassword)
   {
      gConsoleOutput.printf("Encrypting archive\n");
      
      BWin32FileStream dstFileStream;
      if (!dstFileStream.open(pDstFilename, cSFWritable | cSFSeekable | cSFOptimizeForSequentialAccess))
      {
         gConsoleOutput.error("BECFArchiver::build: Failed creating destination file: %s\n", pDstFilename);
         return false;
      }
      
      BEncryptedStream encryptedStream;
      if (!encryptedStream.open(&dstFileStream, *pEncryptionPassword))
      {
         dstFileStream.close();
         DeleteFile(pDstFilename);
         
         gConsoleOutput.error("BECFArchiver::build: Failed opening encryption stream!\n");
         return false;
      }
      
      mTempFileStream.seek(0);
      
      if (!BStream::copyStream(mTempFileStream, encryptedStream, NULL))
      {
         dstFileStream.close();
         DeleteFile(pDstFilename);
         
         gConsoleOutput.error("BECFArchiver::build: Failed encrypting file!\n");
         return false;
      }
      
      if (!dstFileStream.close())
      {
         gConsoleOutput.error("BECFArchiver::build: Failed writing to destination file: %s\n", pDstFilename);
         return false;
      }
      
      mTempFileStream.close();
      DeleteFile(pTmpFilename);
   }
   else
   {
      if (!mTempFileStream.close())
      {  
         gConsoleOutput.error("BECFArchiver::build: Failed writing to temp file: %s\n", pTmpFilename);
         return false;
      }
      
      if (0 == MoveFile(pTmpFilename, pDstFilename))
      {
         gConsoleOutput.error("BECFArchiver::build: Failed renaming temp filename from \"%s\" to destination filename \"%s\"!\n", pTmpFilename, pDstFilename);
         return false;
      }
   }      
   
   gConsoleOutput.printf("Successfully created archive: %I64u bytes\n", archiveSize);
         
   return true;
}   

bool BECFArchiver::readFilenameData(BByteArray& filenameData)
{
   const BECFArchiveChunkHeader& filenameChunk = mArchiveChunkHeaders[0];
   if ((filenameChunk.getFlags() & cECFCompMethodMask) != cECFCompMethodDeflateStream)
   {
      gConsoleOutput.error("BECFArchiver::readFilenameData: Filename chunk is invalid: %s\n", mpSrcArchiveFilename);
      return false;
   }

   int64 compFilenamesOfs = filenameChunk.getOfs();
   int64 compFilenameSize = filenameChunk.getSize();
   if ((cInvalidIndex == compFilenamesOfs) || (compFilenameSize < 1) || (compFilenameSize > 25*1024*1024))
   {  
      gConsoleOutput.error("BECFArchiver::readFilenameData: Filename chunk is invalid: %s\n", mpSrcArchiveFilename);
      return false;
   }

   if (mpSrcArchiveStream->seek(compFilenamesOfs) != compFilenamesOfs)
   {  
      gConsoleOutput.error("BECFArchiver::readFilenameData: Failed reading compressed filename chunk: %s\n", mpSrcArchiveFilename);
      return false;
   }
   
   BByteArray compFilenameData((uint)compFilenameSize);
   if (mpSrcArchiveStream->readBytes(compFilenameData.getPtr(), compFilenameData.getSize()) != compFilenameData.getSize())
   {
      gConsoleOutput.error("BECFArchiver::readFilenameData: Failed reading compressed filename chunk: %s\n", mpSrcArchiveFilename);
      return false;
   }
   
   BTigerHashGen tigerHashGen(compFilenameData.getPtr(), compFilenameData.getSize());
   BTigerHash compTigerHash(tigerHashGen.finalize());
   
   for (uint i = 0; i < 16; i++)
   {
      if (compTigerHash[i] != filenameChunk.mCompTiger128[i])
      {
         gConsoleOutput.error("BECFArchiver::readFilenameData: Compressed filename chunk is invalid: %s\n", mpSrcArchiveFilename);
         return false;
      }
   }
   
   if (calcAdler32(compFilenameData.getPtr(), compFilenameData.getSize()) != filenameChunk.getAdler32())
   {
      gConsoleOutput.error("BECFArchiver::readFilenameData: Compressed filename chunk is invalid: %s\n", mpSrcArchiveFilename);
      return false;
   }

   filenameData.resize(filenameChunk.mDecompSize);

   BByteStream compFilenameDataStream(compFilenameData.getPtr(), compFilenameData.getSize());
   
   BInflateStream inflateStream;
   if (!inflateStream.open(compFilenameDataStream))
   {
      gConsoleOutput.error("BECFArchiver::readFilenameData: Failed reading compressed filename chunk: %s\n", mpSrcArchiveFilename);
      return false;
   }

   if (inflateStream.readBytes(filenameData.getPtr(), filenameData.getSize()) != filenameData.getSize())
   {
      gConsoleOutput.error("BECFArchiver::readFilenameData: Failed decompressing filename chunk: %s\n", mpSrcArchiveFilename);
      return false;
   }

   if (!inflateStream.close())
   {
      gConsoleOutput.error("BECFArchiver::readFilenameData: Failed decompressing filename chunk: %s\n", mpSrcArchiveFilename);
      return false;
   }
   
   tigerHashGen.clear();
   tigerHashGen.update(filenameData.getPtr(), filenameData.getSize());
   BTigerHash decompTigerHash(tigerHashGen.finalize());
   
   if (decompTigerHash.getQWORD(0) != filenameChunk.getID())
   {
      gConsoleOutput.error("BECFArchiver::readFilenameData: Failed decompressing filename chunk: %s\n", mpSrcArchiveFilename);  
      return false;
   }
   
   return true;
}

bool BECFArchiver::verifyChunk(uint chunkIndex, const BByteArray& filenameData, const char* pDstPath, eVerifyMode verifyMode, bool noOverwrite, bool checkOut, bool& fileSkipped)
{
   fileSkipped = false;
   
   const BECFArchiveChunkHeader& chunkHeader = mArchiveChunkHeaders[chunkIndex];
   const uint nameOfs = (chunkHeader.mNameOfs[0] << 16) + (chunkHeader.mNameOfs[1] << 8) | chunkHeader.mNameOfs[2];
   if (nameOfs >= filenameData.getSize())
   {
      gConsoleOutput.error("BECFArchiver::verifyChunk: Chunk header %u name offset is too big: %u\n", chunkIndex, nameOfs);
      return false;
   }

   const char* pChunkFilename = (const char*)filenameData.getPtr() + nameOfs;
   const uint nameLen = strlen(pChunkFilename);
   if (nameLen > 512)
   {
      gConsoleOutput.error("BECFArchiver::verifyChunk: Chunk header %u filename is too long!\n");
      return false;
   }

   gConsoleOutput.printf("File: %s\n", pChunkFilename);

   const uint compMethod = chunkHeader.getFlags() & cECFCompMethodMask;

   if ((compMethod != cECFCompMethodDeflateRaw) && (compMethod != cECFCompMethodStored))
   {
      gConsoleOutput.error("BECFArchiver::verifyChunk: Filename chunk is invalid: %s\n", mpSrcArchiveFilename);
      return false;
   }
   gConsoleOutput.printf("         Compression: ");
   if (compMethod == cECFCompMethodDeflateRaw)
      gConsoleOutput.printf("Deflate\n");
   else
      gConsoleOutput.printf("Raw\n");

   gConsoleOutput.printf("   Decompressed size: %I64u\n", (uint64)chunkHeader.mDecompSize);
   gConsoleOutput.printf("Decompressed Tiger64: 0x%08I64X\n", (uint64)chunkHeader.getID());

   gConsoleOutput.printf("     Compressed size: %I64u\n", (uint64)chunkHeader.getSize());
   gConsoleOutput.printf("  Compressed Adler32: 0x%04X\n", (uint)chunkHeader.getAdler32());

   gConsoleOutput.printf(" Compressed Tiger128: 0x%02X%02X%02X%02X%02X%02X%02X%02X 0x%02X%02X%02X%02X%02X%02X%02X%02X\n", 
      chunkHeader.mCompTiger128[15], chunkHeader.mCompTiger128[14], chunkHeader.mCompTiger128[13], chunkHeader.mCompTiger128[12],
      chunkHeader.mCompTiger128[11], chunkHeader.mCompTiger128[10], chunkHeader.mCompTiger128[9], chunkHeader.mCompTiger128[8], 
      chunkHeader.mCompTiger128[7], chunkHeader.mCompTiger128[6], chunkHeader.mCompTiger128[5], chunkHeader.mCompTiger128[4], 
      chunkHeader.mCompTiger128[3], chunkHeader.mCompTiger128[2], chunkHeader.mCompTiger128[1], chunkHeader.mCompTiger128[0]);

   gConsoleOutput.printf("Chunk file ofs: %I64u\n", (uint64)chunkHeader.getOfs());

   const int64 chunkFileOfs = chunkHeader.getOfs();
   const int64 chunkFileSize = chunkHeader.getSize();

   if ( (chunkFileSize < 1U) || (chunkFileSize >= (int64)mpSrcArchiveStream->size()) || 
      (chunkFileOfs < (int64)mArchiveChunkHeaders[1].getOfs()) || (chunkFileOfs >= (int64)mpSrcArchiveStream->size()) )
   {
      gConsoleOutput.error("BECFArchiver::verifyChunk: Chunk's size or offset is invalid: %s\n", pChunkFilename);
      return false;
   }

   if (verifyMode == cQuick)
      return true;

   if (mpSrcArchiveStream->seek(chunkFileOfs) != chunkFileOfs)
   {
      gConsoleOutput.error("BECFArchiver::verifyChunk: Failed seeking in source file: %s\n", mpSrcArchiveFilename);
      return false;
   }

   BTigerHashGen compTigerHashGen;
   BTigerHashGen decompTigerHashGen;
   uint compAdler32 = INIT_ADLER32;
   uint64 totalSrcBytesLeft = chunkFileSize;
   uint64 totalDstBytesLeft = chunkHeader.mDecompSize;

   BByteArray srcBuf(65536*4);
   BByteArray dstBuf(65536*4);

   std::auto_ptr<BInflate> pInflate(new BInflate);
   pInflate->init();

   BString dstFilename;
   BWin32FileStream dstFileStream;
   if (verifyMode == cExtract)
   {
      if (!pDstPath)
      {
         gConsoleOutput.error("BECFArchiver::verifyChunk: Must specify destination path!\n");
         return false;
      }
      
      dstFilename = pDstPath;
      strPathAddBackSlash(dstFilename);
      dstFilename += pChunkFilename;
      
      dstFilename.standardizePath();
      BString absDstFilename(dstFilename);
      strPathMakeAbsolute(dstFilename, absDstFilename);
      dstFilename = absDstFilename;
      
      BString dstPath;
      strPathGetDirectory(dstFilename, dstPath, true);
      strPathCreateFullPath(dstPath);
      
      WIN32_FILE_ATTRIBUTE_DATA dstFileAttributes;
      const BOOL dstFileExists = GetFileAttributesEx(dstFilename.getPtr(), GetFileExInfoStandard, &dstFileAttributes);
      
      if (dstFileExists)
      {
         if (noOverwrite)
         {
            gConsoleOutput.warning("BECFArchiver::verifyChunk: Skipping already existing file: %s\n", dstFilename.getPtr());
            fileSkipped = true;
            return false;
         }
      }

#ifndef XBOX      
      if (!BConsoleAppHelper::checkOutputFileAttribs(dstFilename, checkOut))
         return false;
#endif      
            
      if (!dstFileStream.open(dstFilename, cSFWritable | cSFSeekable | cSFOptimizeForSequentialAccess))
      {
         gConsoleOutput.error("BECFArchiver::verifyChunk: Failed opening destination file: %s\n", dstFilename.getPtr());
         return false;
      }

      gConsoleOutput.printf("Writing file: %s\n", dstFilename.getPtr());
   }

   int decompStatus = 0;            

   uint64 totalUnusedBytes = 0;
   while (totalSrcBytesLeft)
   {
      const uint srcBytesToRead = (uint)Math::Min<uint64>(totalSrcBytesLeft, srcBuf.getSize());

      if (mpSrcArchiveStream->readBytes(srcBuf.getPtr(), srcBytesToRead) != srcBytesToRead)
      {
         gConsoleOutput.error("BECFArchiver::verifyChunk: Failed reading from source file: %s\n", mpSrcArchiveFilename);
         return false;
      }

      compTigerHashGen.update(srcBuf.getPtr(), srcBytesToRead);
      compAdler32 = calcAdler32(srcBuf.getPtr(), srcBytesToRead, compAdler32);

      totalSrcBytesLeft -= srcBytesToRead;

      if (compMethod == cECFCompMethodStored)
      {
         if (dstFileStream.opened())
         {
            if (dstFileStream.writeBytes(srcBuf.getPtr(), srcBytesToRead) != srcBytesToRead)
            {
               gConsoleOutput.error("BECFArchiver::verifyChunk: Failed writing to destination file: %s\n", dstFilename.getPtr());
               return false;
            }               
         }          
         continue;     
      }

      BDEBUG_ASSERT(compMethod == cECFCompMethodDeflateRaw);

      if (decompStatus == INFL_STATUS_DONE)
      {
         totalUnusedBytes += srcBytesToRead;

         continue;
      }

      uint inBufOfs = 0;
      const bool eofFlag = (totalSrcBytesLeft == 0);

      while ( ((eofFlag) || (inBufOfs < srcBytesToRead)) && (decompStatus != INFL_STATUS_DONE) )
      {
         int inBufBytes = srcBytesToRead - inBufOfs;
         int outBufBytes = dstBuf.getSize();

         decompStatus = pInflate->decompress(
            srcBuf.getPtr() + inBufOfs, &inBufBytes,
            dstBuf.getPtr(), &outBufBytes, 
            eofFlag);

         if (decompStatus < 0)
         {
            gConsoleOutput.error("BECFArchiver::verifyChunk: Decompression of chunk failed: %s\n", pChunkFilename);
            return false;
         }            

         if (outBufBytes)
         {
            totalDstBytesLeft -= outBufBytes;
            if (totalDstBytesLeft < 0)
            {
               gConsoleOutput.error("BECFArchiver::verifyChunk: Decompression of chunk failed: %s\n", pChunkFilename);
               return false;
            }

            decompTigerHashGen.update(dstBuf.getPtr(), outBufBytes);

            if (dstFileStream.opened())
            {
               if (dstFileStream.writeBytes(dstBuf.getPtr(), outBufBytes) != (uint)outBufBytes)
               {
                  gConsoleOutput.error("BECFArchiver::verifyChunk: Failed writing to destination file: %s\n", dstFilename.getPtr());
                  return false;
               }               
            }          
         }               

         inBufOfs += inBufBytes;
      } 
   }

   if (totalUnusedBytes)
      gConsoleOutput.warning("BECFArchiver::verifyChunk: Chunk contains %I64u unused bytes after compressed data!\n", totalUnusedBytes);

   if (compMethod == cECFCompMethodDeflateRaw)
   {
      if ((decompStatus != INFL_STATUS_DONE) || (totalDstBytesLeft != 0))
      {
         gConsoleOutput.error("BECFArchiver::verifyChunk: Decompression of chunk failed: %s\n", pChunkFilename);
         return false;
      }
   }         

   if (dstFileStream.opened())
   {
      if (!dstFileStream.close())
      {
         gConsoleOutput.error("BECFArchiver::verifyChunk: Failed writing to destination file: %s\n", dstFilename.getPtr());
         return false;
      }     
   }

   BTigerHash compTigerHash(compTigerHashGen.finalize());

   BTigerHash decompTigerHash;
   if (compMethod == cECFCompMethodStored)
      decompTigerHash = compTigerHash;
   else
      decompTigerHash = decompTigerHashGen.finalize();

   for (uint i = 0; i < 16; i++)
   {
      if (compTigerHash[i] != chunkHeader.mCompTiger128[i])
      {
         gConsoleOutput.error("BECFArchiver::verifyChunk: Compressed chunk data failed Tiger128 check: %s\n", pChunkFilename);
         return false;
      }
   }    

   if (decompTigerHash.getQWORD(0) != chunkHeader.getID())
   {
      gConsoleOutput.error("BECFArchiver::verifyChunk: Decompressed chunk data failed Tiger64 check: %s\n", pChunkFilename);
      return false;
   }

   if (compAdler32 != chunkHeader.getAdler32())
   {
      gConsoleOutput.error("BECFArchiver::verifyChunk: Compressed chunk data failed Adler32 check: %s\n", pChunkFilename);
      return false;
   }      
   
   return true;
}

bool BECFArchiver::verify(const BVerifyParams& params, uint& numSuccessfulFiles, uint& numFailedFiles, uint& numSkippedFiles)
{
   mpSrcArchiveFilename = params.mpSrcFilename;
   BDEBUG_ASSERT(mpSrcArchiveFilename);
   
   mpDstFilename = NULL;
   mpSrcFilelist = NULL;
   mpSignatureSecretKey = NULL;
   
   if (!mSrcArchiveFileStream.open(params.mpSrcFilename, cSFReadable | cSFSeekable))
   {
      gConsoleOutput.error("BECFArchiver::verify: Failed opening source file: %s\n", params.mpSrcFilename);
      return false;
   }
   
   mpSrcArchiveStream = &mSrcArchiveFileStream;
   if (params.mpEncryptionPassword)
   {
      if (!mSrcArchiveDecryptedStream.open(&mSrcArchiveFileStream, *params.mpEncryptionPassword))
      {
         gConsoleOutput.error("BECFArchiver::verify: Failed opening source file as an encryption stream: %s\n", params.mpSrcFilename);
         return false;
      }  
      mpSrcArchiveStream = &mSrcArchiveDecryptedStream;
   }
      
   BECFFileStream ecfFileStream;
   if (!ecfFileStream.open(mpSrcArchiveStream))
   {
      gConsoleOutput.error("BECFArchiver::verify: Failed opening source file as an ECF stream: %s\n", params.mpSrcFilename);
      if (params.mpEncryptionPassword)
         gConsoleOutput.error("Password was probably wrong.\n");
      else
         gConsoleOutput.error("The file may be encrypted, or corrupted.\n");
      return false;
   }
   
   if (ecfFileStream.getNumChunks() < 2) 
   {
      gConsoleOutput.error("BECFArchiver::verify: Source file is empty or not valid: %s\n", params.mpSrcFilename);
      return false;
   }

   const BECFArchiveHeader& header = (const BECFArchiveHeader&)ecfFileStream.getHeader();
   if (header.getSize() < sizeof(BECFArchiveHeader))
   {
      gConsoleOutput.error("BECFArchiver::verify: File is an ECF file, but does not have a valid archive header: %s\n", params.mpSrcFilename);
      return false;
   }
   
   if (
      ((uint64)header.getFileSize() != mpSrcArchiveStream->size()) ||
      ((mpSrcArchiveStream->size() & 4095U) != 0) ||
      (header.getID() != (DWORD)cArchiveECFHeaderID) || 
      (header.mArchiveHeaderMagic != BECFArchiveHeader::cMagic) ||
      (header.getChunkExtraDataSize() != (sizeof(BECFArchiveChunkHeader) - sizeof(BECFChunkHeader))) ||
      (header.mSignatureSize < sizeof(DWORD) * 2)
      )
   {
      gConsoleOutput.error("BECFArchiver::verify: File is an ECF file, but does not have a valid archive header: %s\n", params.mpSrcFilename);
      return false;
   }
   
   gConsoleOutput.printf("Header:\n");
   gConsoleOutput.printf("              Size: 0x%04X\n", header.getSize());
   gConsoleOutput.printf("           Adler32: 0x%04X\n", header.getAdler32());
   gConsoleOutput.printf("          FileSize: %I64u\n", (uint64)header.getSize());
   gConsoleOutput.printf("         NumChunks: %u\n", header.getNumChunks());
   gConsoleOutput.printf("             Flags: 0x%04X\n", header.getFlags());
   gConsoleOutput.printf("                ID: 0x%04X\n", header.getID());
   gConsoleOutput.printf("ChunkExtraDataSize: %u\n", header.getChunkExtraDataSize());
   gConsoleOutput.printf("     SignatureSize: %u\n", (uint)header.mSignatureSize);
   gConsoleOutput.printf("TotalChunkHeaderSize: %u\n", (uint)ecfFileStream.getChunkHeadersArray().getSizeInBytes());
      
   mArchiveChunkHeaders.resize(header.getNumChunks());
   BDEBUG_ASSERT(mArchiveChunkHeaders.getSizeInBytes() == ecfFileStream.getChunkHeadersArray().getSizeInBytes());
   
   memcpy(mArchiveChunkHeaders.getPtr(), ecfFileStream.getChunkHeadersArray().getPtr(), mArchiveChunkHeaders.getSizeInBytes());
        
      
   BSHA1Gen sha1Gen;
   sha1Gen.update32(0xA7F95F9C);
   sha1Gen.update32(header.getSize());
   sha1Gen.update32(header.getNumChunks());
   sha1Gen.update32(header.getChunkExtraDataSize());
   sha1Gen.update32(header.getFileSize());
   sha1Gen.update(mArchiveChunkHeaders.getPtr(), mArchiveChunkHeaders.getSizeInBytes());
   BSHA1 archiveDigest(sha1Gen.finalize());
   
   if (!params.mpSignaturePublicKeyArray)
   {
      gConsoleOutput.error("BECFArchiver::verify: Signature public key array not specified!\n");
      return false;
   }
      
   uint i;
   for (i = 0; i < params.mpSignaturePublicKeyArray->getSize(); i++)
   {
      BDigitalSignature digitalSignature;
      BByteStream signatureStream(&header + 1, header.mSignatureSize);
      if (digitalSignature.verifyMessage(signatureStream, (*params.mpSignaturePublicKeyArray)[i], archiveDigest))
      {
         gConsoleOutput.printf("File successfully verified against digital signature %i\n", i);
         break;
      }
   }
   
   if (i == params.mpSignaturePublicKeyArray->getSize())
   {
      gConsoleOutput.error("BECFArchiver::verify: File failed digital signature verification: %s\n", params.mpSrcFilename);
      return false;
   }
   
   BByteArray filenameData;
   if (!readFilenameData(filenameData))
      return false;
      
   for (uint chunkIndex = 0; chunkIndex < header.getNumChunks() - 1; chunkIndex++)
   {
      const BECFArchiveChunkHeader& chunkHeader = mArchiveChunkHeaders[chunkIndex];
      const BECFArchiveChunkHeader& nextChunkHeader = mArchiveChunkHeaders[chunkIndex + 1];
      
      if (((uint64)chunkHeader.getOfs() + (uint64)chunkHeader.getSize()) > (uint64)nextChunkHeader.getOfs())
      {
         gConsoleOutput.error("BECFArchiver::verify: Chunk headers are invalid: %s\n", mpSrcArchiveFilename);
         return false;
      }
   }
   
   gConsoleOutput.printf("\n");
   gConsoleOutput.printf("Chunks:\n");
      
   for (uint chunkIndex = 1; chunkIndex < header.getNumChunks(); chunkIndex++)
   {
      gConsoleOutput.printf("\nProcessing Chunk: %u of %u\n", chunkIndex + 1, mArchiveChunkHeaders.getSize());
      
      bool fileSkipped = false;
      const bool status = verifyChunk(chunkIndex, filenameData, params.mpDstPath, params.mMode, params.mNoOverwrite, params.mCheckOut, fileSkipped);
      
      if (!status)
      {
         if (fileSkipped)
         {
            gConsoleOutput.warning("Skipped processing chunk: %u\n", chunkIndex);
            
            numSkippedFiles++;
         }
         else
         {
            gConsoleOutput.error("Failed processing chunk: %u\n", chunkIndex);
            
            numFailedFiles++;
            
            if (!params.mIgnoreChunkProcessingErrors)   
               return false;
         }               
      }     
      else
      {
         numSuccessfulFiles++;
      }
   }
   
   return true;
}


