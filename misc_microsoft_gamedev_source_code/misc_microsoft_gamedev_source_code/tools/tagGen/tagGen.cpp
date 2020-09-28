//============================================================================
//
//  File: tagGen.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xcorelib.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"
#include "file\win32FindFiles.h"
#include "file\win32FileUtils.h"
#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "utils\endianSwitch.h"
#include "resource\ecfUtils.h"
#include "resource\resourceTag.h"
#include "file\win32FileStream.h"

#include <conio.h>

#define PROGRAM_TITLE "TAGGEN" 
#define TAG_EXTENSION "tag"

//-------------------------------------------------

enum eProcessingMode
{
   cModeInvalid,
   
   // Multiple input files, no output files
   cModeInfo,
   
   // Multiple input file, no output file
   cModeVerify,
   
   // Multiple input files, multiple output files (may be same as input files)
   cModeRemove,
         
   // Single ECF input file, single binary input source file, single ECF output file
   cModeAdd,
   
   // Multiple input files, single output file (TAG files)
   cModeCreate,
};

class BCmdLineParams
{
public:
   BCommandLineParser::BStringArray mFileStrings;
   BCommandLineParser::BStringArray mExcludeStrings;
   BString mOutputPathString;
   BString mAppendString;
   BString mOutputFilename;
   BString mSourceFilename;
   
   bool mOutSameDirFlag;
   bool mDeepFlag;
   bool mRecreateFlag;
   bool mNoOverwrite;
   bool mIgnoreErrors;
   
   bool mCheckOut;
   
   int mProcessingMode;
   BString mCreatorToolCmdLine;
   int mCreatorToolVersion;
   BString mPlatform;

   BCmdLineParams() :
      mOutSameDirFlag(false),
      mDeepFlag(false),
      mRecreateFlag(false),
      mNoOverwrite(false),
      mIgnoreErrors(false),
      mCheckOut(false),
      mProcessingMode(cModeInvalid),
      mCreatorToolVersion(1)
   {
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"file",                cCLParamTypeBStringArrayPtr, &mFileStrings },
         {"outpath",             cCLParamTypeBStringPtr, &mOutputPathString },
         {"sourcefile",          cCLParamTypeBStringPtr, &mSourceFilename },
         {"outfile",             cCLParamTypeBStringPtr, &mOutputFilename },
         {"append",              cCLParamTypeBStringPtr, &mAppendString },
         {"outsamedir",          cCLParamTypeFlag, &mOutSameDirFlag },
         {"nooverwrite",         cCLParamTypeFlag, &mNoOverwrite },
         {"deep",                cCLParamTypeFlag, &mDeepFlag },
         {"recreate",            cCLParamTypeFlag, &mRecreateFlag },
         {"ignoreerrors",        cCLParamTypeFlag, &mIgnoreErrors },
         {"checkout",            cCLParamTypeFlag, &mCheckOut },
         {"exclude",             cCLParamTypeBStringArrayPtr, &mExcludeStrings },
         
         {"info",                cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeInfo },
         {"remove",              cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeRemove },
         {"verify",              cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeVerify },
         {"add",                 cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeAdd },
         {"create",              cCLParamTypeInt, &mProcessingMode, 0, NULL, cModeCreate },
         {"creatorCmdLine",      cCLParamTypeBStringPtr, &mCreatorToolCmdLine },
         {"creatorVersion",      cCLParamTypeIntPtr, &mCreatorToolVersion },
         {"platform",            cCLParamTypeBStringPtr, &mPlatform },
                  
         { NULL } 
      };

      BCommandLineParser parser(clParams);

      const bool success = parser.parse(args, false, false);

      if (!success)
      {
         gConsoleOutput.error("%s\n", parser.getErrorString());
         return false;
      }

      if (parser.getUnparsedParams().size())
      {
         gConsoleOutput.error("Invalid parameter: %s\n", args[parser.getUnparsedParams()[0]].getPtr());
         return false;
      }

      return true;
   }

   void printHelp(void)
   {
      //      --------------------------------------------------------------------------------
      gConsoleOutput.printf("Usage: xmlcomp <options>\n");
      gConsoleOutput.printf("Options:\n");

      gConsoleOutput.printf(" -file filename    Specify source filename (wildcards okay), may be\n");
      gConsoleOutput.printf("                   specified multiple times.\n");
      gConsoleOutput.printf(" -outpath path     Specify output path\n");
      gConsoleOutput.printf(" -outfile filename Specify output filename\n");
      gConsoleOutput.printf(" -sourcefile filename Specify source filename\n");
      gConsoleOutput.printf(" -append string    Append string to filename\n");
      gConsoleOutput.printf(" -outsamedir       Write output files to source path\n");
      gConsoleOutput.printf(" -timestamp        Compare timestamps and skip files that are not outdated\n");
      gConsoleOutput.printf(" -deep             Recurse subdirectories\n");
      gConsoleOutput.printf(" -recreate         Recreate directory structure\n");
      gConsoleOutput.printf(" -nooverwrite      Don't overwrite existing files\n");
      gConsoleOutput.printf(" -ignoreerrors     Don't stop on failed files\n");
      gConsoleOutput.printf(" -checkout         Use P4 to check out output file if read only\n");
      gConsoleOutput.printf(" -exclude substr   Exclude files that contain substr (multiple OK)\n");
      gConsoleOutput.printf(" -info             Display resource tags\n");
      gConsoleOutput.printf(" -remove           Remove resource tags\n");
      gConsoleOutput.printf(" -verify           Verify resource tags\n");
      gConsoleOutput.printf(" -add              Add resource tag to ECF file (must specify -sourcefile)\n");
      gConsoleOutput.printf(" -create           Create .TAG file for multiple files (must specify -outfile)\n");
      gConsoleOutput.printf(" -creatorCmdLine string   Creator tool name/command line\n");
      gConsoleOutput.printf(" -creatorVersion int Creator tool version\n");
      gConsoleOutput.printf(" -platform string  File platform (any, pc, or xbox)\n");

      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams

class BTagGen
{
   BCmdLineParams mCmdLineParams;

   uint mNumFilesProcessed;
   uint mNumFilesSkipped;
   uint mNumFailedFiles;
      
   BString mWorkDirectory;
     
   class BFilePath
   {
   public:
      BFilePath() { }

      BFilePath(const BString& basePathname, const BString& relPathname, const BString& filename) :
         mBasePathname(basePathname),
         mRelPathname(relPathname),
         mFilename(filename)
      {
      }         

      BFilePath(const BFileDesc& fileDesc) :
         mBasePathname(fileDesc.basePathname()),
         mRelPathname(fileDesc.relPathname()),
         mFilename(fileDesc.filename())
      {
      }         

      BString& basePathname(void) { return mBasePathname; }
      BString& relPathname(void) { return mRelPathname; }
      BString& filename(void) { return mFilename; }

      const BString& basePathname(void) const { return mBasePathname; }
      const BString& relPathname(void) const { return mRelPathname; }
      const BString& filename(void) const { return mFilename; }

      BString fullFilename(void) const
      {
         BString filename(mBasePathname);
         strPathAddBackSlash(filename, true);
         filename += mRelPathname;
         strPathAddBackSlash(filename, true);
         filename += mFilename;
         return filename;
      }

   private:
      BString mBasePathname;
      BString mRelPathname;
      BString mFilename;
   };

   BDynamicArray<BFilePath> mSourceFiles;
   
   enum BProcessFileStatus
   {
      cPFSFailed = -1,
      cPFSSucceeded = 0,
      cPFSSkipped = 1,
   };

   bool processParams(BCommandLineParser::BStringArray& args)
   {
      if (args.getSize() < 2)
      {
         mCmdLineParams.printHelp();
         return false;
      }

      if (!mCmdLineParams.parse(args))
         return false;

      if (mCmdLineParams.mFileStrings.isEmpty())
      {
         gConsoleOutput.error("No files specified to process!\n");
         return false;  
      }         

      return true;
   }

   bool findFiles(void)
   {
      mSourceFiles.clear();

      int findFilesFlag = BFindFiles::FIND_FILES_WANT_FILES;
      if (mCmdLineParams.mDeepFlag)
         findFilesFlag |= BFindFiles::FIND_FILES_RECURSE_SUBDIRS;

      for (uint fileIndex = 0; fileIndex < mCmdLineParams.mFileStrings.getSize(); fileIndex++)
      {         
         const BString& fileString = mCmdLineParams.mFileStrings[fileIndex];

         TCHAR fullpath[_MAX_PATH];
         LPTSTR pFilePart = NULL;

         DWORD result = GetFullPathName(fileString.getPtr(), _MAX_PATH, fullpath, &pFilePart);
         if (0 == result)            
         {
            gConsoleOutput.error("Can't resolve path: %s\n", fileString);
            return false;
         }

         BString path;
         BString filename;
         path.set(fullpath, pFilePart - fullpath);
         filename.set(pFilePart);

         const DWORD fullAttributes = GetFileAttributes(fullpath);

         if ((fullAttributes != INVALID_FILE_ATTRIBUTES) && (fullAttributes & FILE_ATTRIBUTE_DIRECTORY))
         {
            // If the full path points to a directory then change the filename part to be a wildcard.
            path.set(fullpath);
            filename.set(B("*.*"));
         }
         else
         {
            // Remove backslash from end of path
            const BCHAR_T lastChar = path.getPtr()[path.length() - 1];
            if ((lastChar == B('\\')) || (lastChar == B('/')))
            {
               path.set(path.getPtr(), path.length() - 1);
            }
         }

         BFindFiles findFiles(path, filename, findFilesFlag);

         if (!findFiles.success())
         {
            gConsoleOutput.error("Can't find files: %s\n", fileString);
            return false;
         }

         for (uint fileIndex = 0; fileIndex < findFiles.numFiles(); fileIndex++)
            mSourceFiles.pushBack(BFilePath(findFiles.getFile(fileIndex)));
      }            

      return true;
   }

   typedef BDynamicArray<BString> BStringArray;
   
   bool generateFileList(BStringArray& srcFilenames, BStringArray* pDstFilenames)
   {
      srcFilenames.clear();
      
      if (pDstFilenames)
         pDstFilenames->clear();
      
      if (!findFiles())
         return false;
     
      for (uint fileIndex = 0; fileIndex < mSourceFiles.size(); fileIndex++)
      {
         const BFilePath& srcFilePath = mSourceFiles[fileIndex];
         
         BString srcFullFilename(srcFilePath.fullFilename());
         
         uint i;
         for (i = 0; i < mCmdLineParams.mExcludeStrings.getSize(); i++)
         {
            if (strstr(srcFullFilename.getPtr(), mCmdLineParams.mExcludeStrings[i].getPtr()) != NULL)
            {
               gConsoleOutput.printf("Skipping excluded file: %s\n", srcFullFilename.getPtr());
               mNumFilesSkipped++;
               break;
            }
         }
         if (i < mCmdLineParams.mExcludeStrings.getSize())
            continue;
         
         srcFilenames.pushBack(srcFullFilename);
         
         if (pDstFilenames)
         {
            BString srcRelPathname(srcFilePath.relPathname());
            
            BString srcPath;
            BString srcFilename;
            strPathSplit(srcFullFilename, srcPath, srcFilename);
                                    
            BString dstFullFilename;

            dstFullFilename.set((mCmdLineParams.mOutSameDirFlag ? srcPath : mCmdLineParams.mOutputPathString));

            strPathAddBackSlash(dstFullFilename, false);

            if ((!mCmdLineParams.mOutSameDirFlag) && (mCmdLineParams.mRecreateFlag))
            {
               dstFullFilename += srcRelPathname;       

               strPathAddBackSlash(dstFullFilename, false);  
            }

            if ((!mCmdLineParams.mOutSameDirFlag) && (mCmdLineParams.mRecreateFlag))
               strPathCreateFullPath(dstFullFilename);

            dstFullFilename += srcFilename;

            WIN32_FILE_ATTRIBUTE_DATA dstFileAttributes;
            const BOOL dstFileExists = GetFileAttributesEx(dstFullFilename, GetFileExInfoStandard, &dstFileAttributes);

            if (dstFileExists)
            {
               if (mCmdLineParams.mNoOverwrite)
               {
                  gConsoleOutput.printf("Skipping already existing file: %s\n", dstFullFilename.getPtr());
                  mNumFilesSkipped++;
                  continue;
               }
            }
            
            pDstFilenames->pushBack(dstFullFilename);
         }            
      }

      return true;
   }

   void clear(void)
   {
      mWorkDirectory.empty();
      mSourceFiles.clear();

      mNumFilesProcessed = 0;
      mNumFilesSkipped = 0;
      mNumFailedFiles = 0;
   }
   
   BProcessFileStatus processFileInfo(const BString& srcFilename)
   {
      BWin32FileStream fileStream;
      if (!fileStream.open(srcFilename))
      {
         gConsoleOutput.error("Unable to open file: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }
      
      BECFFileStream ecfFileStream;
      
      if (!ecfFileStream.open(&fileStream))
      {
         gConsoleOutput.error("Failed reading ECF file: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }
      
      uint totalChunksFound = 0;
      for (uint index = 0; ; index++)
      {
         const uint64 chunkID = (uint64)cResourceTagECFChunkID | ((uint64)index << 32U);
         
         const int chunkIndex = ecfFileStream.findChunkByID(chunkID);
         if (cInvalidIndex == chunkIndex)
            break;
            
         if (!ecfFileStream.seekToChunk(chunkIndex))
         {
            gConsoleOutput.error("Failed reading ECF file: %s\n", srcFilename.getPtr());
            return cPFSFailed;
         }
         
         BByteArray chunkData;
         const DWORD chunkSize = ecfFileStream.getChunkHeader(chunkIndex).getSize();
         if (chunkSize > 65536)
         {
            gConsoleOutput.error("Resource tag %u is too big for file: %s\n", index, srcFilename.getPtr());
            return cPFSFailed;
         }
         
         chunkData.resize(chunkSize);
         
         if (fileStream.readBytes(chunkData.getPtr(), chunkSize) != chunkSize)
         {
            gConsoleOutput.error("Failed reading ECF file: %s\n", srcFilename.getPtr());
            return cPFSFailed;
         }
         
         BResourceTagHeader& resourceTagHeader = *reinterpret_cast<BResourceTagHeader*>(chunkData.getPtr());
         if (!resourceTagHeader.check())
         {
            gConsoleOutput.error("Resource tag %u is invalid for file: %s\n", index, srcFilename.getPtr());
            return cPFSFailed;
         }
         
         resourceTagHeader.pointerize(&resourceTagHeader);
         
         gConsoleOutput.printf("Resource Tag Index: %u\n", index);
         if (!BResourceTagUtils::printInfo(BConsoleTextDispatcher(gConsoleOutput), resourceTagHeader))
         {
            gConsoleOutput.error("Resource tag %u is invalid for file: %s\n", index, srcFilename.getPtr());
            return cPFSFailed;
         }
                           
         totalChunksFound++;
      }
      
      if (!totalChunksFound)
      {
         gConsoleOutput.error("Unable to find any resource tags: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }
      
      return cPFSSucceeded;
   }
   
   bool processInfo(void)
   {
      BStringArray srcFilenames;
      if (!generateFileList(srcFilenames, NULL))
         return false;

      if (srcFilenames.isEmpty())
      {
         gConsoleOutput.error("No files found to process!\n");
         return false;
      }
      
      for (uint fileIndex = 0; fileIndex < srcFilenames.getSize(); fileIndex++)
      {
         const BString& srcFilename = srcFilenames[fileIndex];
         
         gConsoleOutput.printf("Processing file: %s\n", srcFilename.getPtr());
         
         BProcessFileStatus status = processFileInfo(srcFilename);
                           
         if (status == cPFSFailed)
         {
            mNumFailedFiles++;

            gConsoleOutput.error("Failed processing file: %s\n", srcFilename.getPtr());

            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
         else if (status == cPFSSkipped)
         {
            mNumFilesSkipped++;
         }
         else
         {
            mNumFilesProcessed++;
         }
         
         gConsoleOutput.printf("\n");
      }         
      
      return true;
   }
      
   BProcessFileStatus processFileVerify(const BString& srcFilename)
   {
      BWin32FileStream fileStream;
      if (!fileStream.open(srcFilename))
      {
         gConsoleOutput.error("Unable to open file: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }

      BECFFileStream ecfFileStream;

      if (!ecfFileStream.open(&fileStream))
      {
         gConsoleOutput.error("Failed reading ECF file: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }

      uint totalChunksFound = 0;
      for (uint index = 0; ; index++)
      {
         const uint64 chunkID = (uint64)cResourceTagECFChunkID | ((uint64)index << 32U);

         const int chunkIndex = ecfFileStream.findChunkByID(chunkID);
         if (cInvalidIndex == chunkIndex)
            break;

         if (!ecfFileStream.seekToChunk(chunkIndex))
         {
            gConsoleOutput.error("Failed reading ECF file: %s\n", srcFilename.getPtr());
            return cPFSFailed;
         }

         BByteArray chunkData;
         const DWORD chunkSize = ecfFileStream.getChunkHeader(chunkIndex).getSize();
         if (chunkSize > 65536)
         {
            gConsoleOutput.error("Resource tag %u is too big for file: %s\n", index, srcFilename.getPtr());
            return cPFSFailed;
         }

         chunkData.resize(chunkSize);

         if (fileStream.readBytes(chunkData.getPtr(), chunkSize) != chunkSize)
         {
            gConsoleOutput.error("Failed reading ECF file: %s\n", srcFilename.getPtr());
            return cPFSFailed;
         }

         BResourceTagHeader& resourceTagHeader = *reinterpret_cast<BResourceTagHeader*>(chunkData.getPtr());
         if (!resourceTagHeader.check())
         {
            gConsoleOutput.error("Resource tag %u is invalid for file: %s\n", index, srcFilename.getPtr());
            return cPFSFailed;
         }

         resourceTagHeader.pointerize(&resourceTagHeader);

         gConsoleOutput.printf("Resource tag index: %u\n", index);
         gConsoleOutput.printf("Original source file: %s\n", resourceTagHeader.mpSourceFilename);
         
         BString sourceFilename(resourceTagHeader.mpSourceFilename);
         sourceFilename.standardizePath();
                  
         int i = sourceFilename.findLeft("\\work\\");
         if (i != -1)
         {
            sourceFilename.crop(i + 6, sourceFilename.length() - 1);
            sourceFilename = mWorkDirectory + sourceFilename;
         }                  
         
         gConsoleOutput.printf("Attempting to read local file: %s\n", sourceFilename.getPtr());
         
         uint64 sourceFileSize = 0;
         BSHA1 sourceFileDigest;
         if (!BWin32FileUtils::computeFileDigest(sourceFilename, &sourceFileSize, &sourceFileDigest))
         {
            gConsoleOutput.error("Unable to read file: %s\n", sourceFilename.getPtr());
            return cPFSFailed;
         }
         
         gConsoleOutput.printf("Source file size: %I64u\n", sourceFileSize);
         gConsoleOutput.printf("Source file digest: 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\n", sourceFileDigest.getDWORD(0), sourceFileDigest.getDWORD(1), sourceFileDigest.getDWORD(2), sourceFileDigest.getDWORD(3), sourceFileDigest.getDWORD(4));
         
         if (sourceFileSize != resourceTagHeader.mSourceFileSize)
         {
            gConsoleOutput.error("Source file's size doesn't match (should be %I64u)\n", resourceTagHeader.mSourceFileSize);
            return cPFSFailed;
         }
         
         if (sourceFileDigest != resourceTagHeader.mSourceDigest)         
         {
            gConsoleOutput.error("Source file's digest doesn't match (should be 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X)\n", resourceTagHeader.mSourceDigest.getDWORD(0), resourceTagHeader.mSourceDigest.getDWORD(1), resourceTagHeader.mSourceDigest.getDWORD(2), resourceTagHeader.mSourceDigest.getDWORD(3), resourceTagHeader.mSourceDigest.getDWORD(4));
            return cPFSFailed;
         }
         
         gConsoleOutput.printf("Source file OK.\n");
         
         totalChunksFound++;
      }

      if (!totalChunksFound)
      {
         gConsoleOutput.error("Unable to find any resource tags: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }

      return cPFSSucceeded;
   }
   
   bool processVerify(void)
   {
      BStringArray srcFilenames;
      if (!generateFileList(srcFilenames, NULL))
         return false;

      if (srcFilenames.isEmpty())
      {
         gConsoleOutput.error("No files found to process!\n");
         return false;
      }

      for (uint fileIndex = 0; fileIndex < srcFilenames.getSize(); fileIndex++)
      {
         const BString& srcFilename = srcFilenames[fileIndex];

         gConsoleOutput.printf("Processing file: %s\n", srcFilename.getPtr());

         BProcessFileStatus status = processFileVerify(srcFilename);

         if (status == cPFSFailed)
         {
            mNumFailedFiles++;

            gConsoleOutput.error("Failed processing file: %s\n", srcFilename.getPtr());

            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
         else if (status == cPFSSkipped)
         {
            mNumFilesSkipped++;
         }
         else
         {
            mNumFilesProcessed++;
         }
         
         gConsoleOutput.printf("\n");
      }         

      return true;
   }
   
   BProcessFileStatus processFileRemove(const BString& srcFilename, const BString& dstFilename)
   {
      BWin32FileStream fileStream;
      if (!fileStream.open(srcFilename, cSFReadable | cSFSeekable | cSFOptimizeForSequentialAccess))
      {
         gConsoleOutput.error("Unable to open file: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }

      BECFFileBuilder ecfFileBuilder;

      if (!ecfFileBuilder.readFromStream(fileStream))
      {
         gConsoleOutput.error("Failed reading ECF file: %s\n", srcFilename.getPtr());
         return cPFSFailed;
      }
      
      fileStream.close();

      uint totalChunksFound = 0;
      for (uint chunkIndex = 0; chunkIndex < ecfFileBuilder.getNumChunks(); chunkIndex++)
      {
         const BECFChunkData& chunkData = ecfFileBuilder.getChunkByIndex(chunkIndex);
         if ((chunkData.getID() & 0xFFFFFFFF) != (uint)cResourceTagECFChunkID)
            continue;
         
         ecfFileBuilder.removeChunkByIndex(chunkIndex);
         chunkIndex--;
         
         totalChunksFound++;
      }
            
      gConsoleOutput.printf("Removed %i resource chunks\n", totalChunksFound);
      
      if (!ecfFileBuilder.getNumChunks())
      {
         gConsoleOutput.warning("All chunks deleted: skipping file!\n");
         return cPFSSkipped;
      }
                  
      gConsoleOutput.printf("Writing file: %s\n", dstFilename.getPtr());
      
      if (!BConsoleAppHelper::checkOutputFileAttribs(dstFilename, mCmdLineParams.mCheckOut))
         return cPFSFailed;
               
      if (!fileStream.open(dstFilename, cSFWritable | cSFSeekable | cSFOptimizeForSequentialAccess))
      {
         gConsoleOutput.printf("Unable to open file for writing: %s\n", dstFilename.getPtr());
         return cPFSFailed;
      }
      
      if (!ecfFileBuilder.writeToStream(fileStream))
      {
         gConsoleOutput.printf("Unable to write to file: %s\n", dstFilename.getPtr());
         return cPFSFailed;
      }
      
      if (!fileStream.close())
      {
         gConsoleOutput.printf("Unable to write to file: %s\n", dstFilename.getPtr());
         return cPFSFailed;
      }

      return cPFSSucceeded;
   }
   
   bool processRemove(void)
   {
      BStringArray srcFilenames;
      BStringArray dstFilenames;      
      if (!generateFileList(srcFilenames, &dstFilenames))
         return false;

      if (srcFilenames.isEmpty())
      {
         gConsoleOutput.error("No files found to process!\n");
         return false;
      }

      for (uint fileIndex = 0; fileIndex < srcFilenames.getSize(); fileIndex++)
      {
         const BString& srcFilename = srcFilenames[fileIndex];
         const BString& dstFilename = dstFilenames[fileIndex];

         gConsoleOutput.printf("Processing file: %s\n", srcFilename.getPtr());

         BProcessFileStatus status = processFileRemove(srcFilename, dstFilename);

         if (status == cPFSFailed)
         {
            mNumFailedFiles++;

            gConsoleOutput.error("Failed processing file: %s\n", srcFilename.getPtr());

            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
         else if (status == cPFSSkipped)
         {
            mNumFilesSkipped++;
         }
         else
         {
            mNumFilesProcessed++;
         }
         
         gConsoleOutput.printf("\n");
      }         

      return true;
   }
   
   bool processAdd(void)
   {
      if (mCmdLineParams.mFileStrings.getSize() != 1)
      {
         gConsoleOutput.error("Must specify one ECF file using the -file option!\n");
         return false;
      }
      
      BString ecfFilename(mCmdLineParams.mFileStrings[0]);
      
      if ((ecfFilename.findLeft('*') != -1) || (ecfFilename.findLeft('?') != -1))
      {
         gConsoleOutput.error("Wildcards unsupported in add mode!\n");
         return false;
      }
      
      if (mCmdLineParams.mSourceFilename.isEmpty())
      {
         gConsoleOutput.error("Must specify a source file using the -sourcefile option!\n");
         return false;
      }
      
      BString sourceFilename(mCmdLineParams.mSourceFilename);
                     
      BWin32FileStream fileStream;
      if (!fileStream.open(ecfFilename, cSFReadable | cSFSeekable | cSFOptimizeForSequentialAccess))
      {
         gConsoleOutput.error("Unable to open file: %s\n", ecfFilename.getPtr());
         return false;
      }

      BECFFileBuilder ecfFileBuilder;

      if (!ecfFileBuilder.readFromStream(fileStream))
      {
         gConsoleOutput.error("Failed reading ECF file: %s\n", ecfFilename.getPtr());
         return false;
      }

      fileStream.close();
      
      BResourceTagBuilder resourceTagBuilder;
      resourceTagBuilder.setSourceFilename(sourceFilename);
      resourceTagBuilder.setCreatorToolInfo(mCmdLineParams.mCreatorToolCmdLine, (uchar)mCmdLineParams.mCreatorToolVersion);

      if (mCmdLineParams.mPlatform == "pc")
         resourceTagBuilder.setPlatformID(BResourceTagHeader::cPIDPC);
      else if (mCmdLineParams.mPlatform == "xbox")
         resourceTagBuilder.setPlatformID(BResourceTagHeader::cPIDXbox);

      if (!resourceTagBuilder.setSourceDigestAndTimeStamp(sourceFilename))
      {
         gConsoleOutput.error("Failed computing source file digest: %s\n", sourceFilename.getPtr());
         return false;
      }
      
      if (!resourceTagBuilder.finalize())
      {
         gConsoleOutput.error("Failed finalizing resource tag!");
         return false;
      }
      
      uint totalChunksFound = 0;
      for (uint chunkIndex = 0; chunkIndex < ecfFileBuilder.getNumChunks(); chunkIndex++)
      {
         const BECFChunkData& chunkData = ecfFileBuilder.getChunkByIndex(chunkIndex);
         if ((chunkData.getID() & 0xFFFFFFFF) != (uint)cResourceTagECFChunkID)
            continue;

         ecfFileBuilder.removeChunkByIndex(chunkIndex);
         chunkIndex--;

         totalChunksFound++;
      }

      gConsoleOutput.printf("Removed %i existing resource chunks\n", totalChunksFound);
      
      ecfFileBuilder.addChunk((uint64)cResourceTagECFChunkID, resourceTagBuilder.getFinalizedData());
            
      if (!ecfFileBuilder.getNumChunks())
      {
         gConsoleOutput.warning("All chunks deleted: skipping file!\n");
         return false;
      }
      
      BString dstFilename(mCmdLineParams.mOutputFilename);
      if (dstFilename.isEmpty())
         dstFilename = ecfFilename;

      gConsoleOutput.printf("Writing file: %s\n", dstFilename.getPtr());

      if (!BConsoleAppHelper::checkOutputFileAttribs(dstFilename, mCmdLineParams.mCheckOut))
         return false;

      if (!fileStream.open(dstFilename, cSFWritable | cSFSeekable | cSFOptimizeForSequentialAccess))
      {
         gConsoleOutput.printf("Unable to open file for writing: %s\n", dstFilename.getPtr());
         return false;
      }

      if (!ecfFileBuilder.writeToStream(fileStream))
      {
         gConsoleOutput.printf("Unable to write to file: %s\n", dstFilename.getPtr());
         return false;
      }

      if (!fileStream.close())
      {
         gConsoleOutput.printf("Unable to write to file: %s\n", dstFilename.getPtr());
         return false;
      }
      
      gConsoleOutput.printf("\n");

      return true;
   }
   
   bool processCreate(void)
   {
      BStringArray srcFilenames;
      if (!generateFileList(srcFilenames, NULL))
         return false;
      
      if (srcFilenames.isEmpty())
      {
         gConsoleOutput.error("No files found to process!\n");
         return false;
      }
      
      BString dstFilename(mCmdLineParams.mOutputFilename);
      if (dstFilename.isEmpty())
      {
         gConsoleOutput.error("Must specify an output filename with the -outfile option!");
         return false;
      }
      
      BECFFileBuilder ecfFileBuilder;
      
      for (uint fileIndex = 0; fileIndex < srcFilenames.getSize(); fileIndex++)
      {
         const BString& sourceFilename = srcFilenames[fileIndex];
         
         BResourceTagBuilder resourceTagBuilder;
         resourceTagBuilder.setSourceFilename(sourceFilename);
         resourceTagBuilder.setCreatorToolInfo(mCmdLineParams.mCreatorToolCmdLine, (uchar)mCmdLineParams.mCreatorToolVersion);

         if (mCmdLineParams.mPlatform == "pc")
            resourceTagBuilder.setPlatformID(BResourceTagHeader::cPIDPC);
         else if (mCmdLineParams.mPlatform == "xbox")
            resourceTagBuilder.setPlatformID(BResourceTagHeader::cPIDXbox);

         if (!resourceTagBuilder.setSourceDigestAndTimeStamp(sourceFilename))
         {
            gConsoleOutput.error("Failed computing source file digest: %s\n", sourceFilename.getPtr());
            return false;
         }
         
         if (!resourceTagBuilder.finalize())
         {
            gConsoleOutput.error("Failed finalizing resource tag!");
            return false;
         }
         
         ecfFileBuilder.addChunk((uint64)cResourceTagECFChunkID | (((uint64)fileIndex) << 32U), resourceTagBuilder.getFinalizedData());
         
         gConsoleOutput.printf("Added file: %s\n", sourceFilename.getPtr());
      }
      
      gConsoleOutput.printf("Writing file: %s\n", dstFilename.getPtr());

      if (!BConsoleAppHelper::checkOutputFileAttribs(dstFilename, mCmdLineParams.mCheckOut))
         return false;

      BWin32FileStream fileStream;
      if (!fileStream.open(dstFilename, cSFWritable | cSFSeekable | cSFOptimizeForSequentialAccess))
      {
         gConsoleOutput.printf("Unable to open file for writing: %s\n", dstFilename.getPtr());
         return false;
      }

      if (!ecfFileBuilder.writeToStream(fileStream))
      {
         gConsoleOutput.printf("Unable to write to file: %s\n", dstFilename.getPtr());
         return false;
      }

      if (!fileStream.close())
      {
         gConsoleOutput.printf("Unable to write to file: %s\n", dstFilename.getPtr());
         return false;
      }

      gConsoleOutput.printf("\n");
      
      return true;
   }
   
   bool findWorkDirectory(void)
   {
      char buf[MAX_PATH];
      if (!GetModuleFileNameA(GetModuleHandle(NULL), buf, sizeof(buf)))
         return false;

      mWorkDirectory = buf;
      mWorkDirectory.toLower();

      int i = mWorkDirectory.findRight("\\tools\\taggen\\taggen.exe");
      if (i == -1)
         i = mWorkDirectory.findRight("\\tools\\taggen\\taggend.exe");
      if (i == -1)
         return false;

      mWorkDirectory.crop(0, i);
      
      return true;
   }

public:
   BTagGen()
   {
      clear();
   }

   bool process(BCommandLineParser::BStringArray& args)
   {
      clear();
      
      if (!findWorkDirectory())
      {
         gConsoleOutput.error("Unable to find work directory!\n");
         return false;
      }

      if (!processParams(args))
         return false;
         
      if (cModeInvalid == mCmdLineParams.mProcessingMode)
      {
         gConsoleOutput.error("Must specify processing mode (-info, -create, etc.)!\n");
         return false;  
      }
         
      bool status = false;
      switch (mCmdLineParams.mProcessingMode)
      {
         case cModeInfo:   status = processInfo(); break;
         case cModeVerify: status = processVerify(); break;
         case cModeRemove: status = processRemove(); break;
         case cModeAdd:    status = processAdd(); break;
         case cModeCreate: status = processCreate(); break;
      }
      
      gConsoleOutput.printf("Files processed successfully: %i, skipped: %i, failed: %i\n", mNumFilesProcessed, mNumFilesSkipped, mNumFailedFiles);
      gConsoleOutput.printf("Total errors: %u, Total warnings: %u\n", BConsoleAppHelper::getTotalErrorMessages(), BConsoleAppHelper::getTotalWarningMessages());

      return status;    
   }
};

int main(int argC, const char** argV)
{
   XCoreCreate();

   BConsoleAppHelper::setup();

   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argC, argV))
   {
      BConsoleAppHelper::deinit();

      XCoreRelease();
      return EXIT_FAILURE;
   }

   gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s\n\n", __DATE__, __TIME__);

   BTagGen xmlComp;
   const bool success = xmlComp.process(args);

   BConsoleAppHelper::deinit();

   XCoreRelease();
   return success ? EXIT_SUCCESS : EXIT_FAILURE;  
}