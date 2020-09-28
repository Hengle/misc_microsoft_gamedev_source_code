//============================================================================
//
//  File: ecfArchiver.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "resource\ecfArchiveTypes.h"
#include "file\win32FileStream.h"
#include "hash\bsha1.h"
#include "stream\encryptedStream.h"

class BECFArchiver
{
public:
   BECFArchiver();

   bool build(
      const char* pBaseDirFilename,
      const char* pDstFilename,
      const char* pTmpFilename,
      const BDynamicArray<BString>& srcFilenames,
      BStream& signatureSecretKey,
      uint chunkDataAlignmentLog2 = 2,
      BString* pEncryptionPassword = NULL);
      
   enum eVerifyMode
   {
      cQuick,
      cFull,
      cExtract
   };
   
   class BVerifyParams
   {
   public:
      BVerifyParams() :
         mMode(cQuick),
         mpSrcFilename(NULL),
         mpDstPath(NULL),
         mpSignaturePublicKeyArray(NULL),
         mIgnoreChunkProcessingErrors(false),
         mpEncryptionPassword(false),
         mNoOverwrite(false),
         mCheckOut(false)
      {
      }
   
      eVerifyMode                   mMode;
      
      const char*                   mpSrcFilename;
      const char*                   mpDstPath;
      const BDynamicArray<BSHA1>*   mpSignaturePublicKeyArray;
      
      BString*                      mpEncryptionPassword;
      
      bool                          mIgnoreChunkProcessingErrors;
      bool                          mNoOverwrite;
      bool                          mCheckOut;
   };
   
   bool verify(const BVerifyParams& params, uint& numSuccessfulFiles, uint& numFailedFiles, uint& numSkippedFiles);
         
private:      
   const char*                   mpSrcArchiveFilename;
   const char*                   mpDstFilename;
   const BDynamicArray<BString>* mpSrcFilelist;
   BStream*                      mpSignatureSecretKey;
   uint                          mChunkDataAlignmentLog2;
   
   BECFArchiveHeader             mArchiveHeader;
   
   typedef BDynamicArray<BECFArchiveChunkHeader> BArchiveChunkHeaderArray;
   BArchiveChunkHeaderArray      mArchiveChunkHeaders;
   
   BDecryptedStream              mSrcArchiveDecryptedStream;
   BWin32FileStream              mSrcArchiveFileStream;
   BStream*                      mpSrcArchiveStream;
   
   BWin32FileStream              mTempFileStream;
   
   bool                          createArchiveFilename(BECFArchiveChunkHeader& chunkHeader, BByteArray& filenameData, const char* pBaseDirFilename, const BString& srcFilename);
   bool                          compressFile(BECFArchiveChunkHeader& chunkHeader, const BString& srcFilename);
   bool                          createFilenameChunk(BECFArchiveChunkHeader& chunkHeader, const BByteArray& filenameData);
   bool                          beginChunk(BECFArchiveChunkHeader& chunkHeader);
   bool                          writeArchiveHeader(uint conservativeHeaderSize);
   bool                          readFilenameData(BByteArray& filenameData);
   bool                          verifyChunk(uint chunkIndex, const BByteArray& filenameData, const char* pDstPath, eVerifyMode verifyMode, bool noOverwrite, bool checkOut, bool& fileSkipped);
};





