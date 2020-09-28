//============================================================================
//
//  File: xmlComp.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xcorelib.h"
#include "compressedStream.h"
#include "stream\dynamicStream.h"
#include "xml\xmxDataBuilder.h"
#include "xml\xmxDataDumper.h"
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
#include "xmlReader.h"

#include <conio.h>

#define PROGRAM_TITLE "XMLCOMP" 
#define XMB_EXTENSION "xmb"

const uint cAssetTagCreatorToolVersion = 1;

//-------------------------------------------------

class BCmdLineParams
{
public:
   BCommandLineParser::BStringArray mFileStrings;
   BCommandLineParser::BStringArray mExcludeStrings;
   BString mOutputPathString;
   BString mAppendString;
   //BString mRulesFilename;
   
   bool mOutSameDirFlag;
   bool mTimestampFlag;
   bool mDeepFlag;
   bool mRecreateFlag;
   bool mNoOverwrite;
   bool mIgnoreErrors;
   
   bool mStatsFlag;

   bool mFileStats;
   //bool mConversionDetails;
   
   bool mDisableNumerics;
   bool mPermitUnicode;
   bool mForceUnicode;
   bool mLittleEndian;
   bool mDumpPackedData;
   bool mDisableTagChunk;
   bool mDeltaMode;

   bool mCheckOut;

   BCmdLineParams() :
      mOutSameDirFlag(false),
      mTimestampFlag(false),
      mDeepFlag(false),
      mRecreateFlag(false),
      mNoOverwrite(false),
      mIgnoreErrors(false),
      mStatsFlag(false),
      mFileStats(false),
//      mConversionDetails(false),
      mCheckOut(false),
      mDisableNumerics(false),
      mPermitUnicode(true),
      mForceUnicode(false),
      mLittleEndian(false),
      mDumpPackedData(false),
      mDisableTagChunk(false),
      mDeltaMode(false)
   {
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"file",                cCLParamTypeBStringArrayPtr, &mFileStrings },
         {"outpath",             cCLParamTypeBStringPtr, &mOutputPathString },
         {"append",              cCLParamTypeBStringPtr, &mAppendString },
         {"outsamedir",          cCLParamTypeFlag, &mOutSameDirFlag },
         {"nooverwrite",         cCLParamTypeFlag, &mNoOverwrite },
         {"timestamp",           cCLParamTypeFlag, &mTimestampFlag },
         {"deep",                cCLParamTypeFlag, &mDeepFlag },
         {"recreate",            cCLParamTypeFlag, &mRecreateFlag },
         {"ignoreerrors",        cCLParamTypeFlag, &mIgnoreErrors },
         {"stats",               cCLParamTypeFlag, &mStatsFlag },
         {"filestats",           cCLParamTypeFlag, &mFileStats },
         //{"details",             cCLParamTypeFlag, &mConversionDetails },
         {"checkout",            cCLParamTypeFlag, &mCheckOut },
         {"disableNumerics",     cCLParamTypeFlag, &mDisableNumerics },
         //{"permitUnicode",       cCLParamTypeFlag, &mPermitUnicode },
         {"forceUnicode",        cCLParamTypeFlag, &mForceUnicode },
         {"littleEndian",        cCLParamTypeFlag, &mLittleEndian },
         //{"rules",               cCLParamTypeBStringPtr, &mRulesFilename },
         {"dump",                cCLParamTypeFlag, &mDumpPackedData },
         {"exclude",             cCLParamTypeBStringArrayPtr, &mExcludeStrings },
         {"noTags",              cCLParamTypeFlag, &mDisableTagChunk },
         {"delta",               cCLParamTypeFlag, &mDeltaMode },
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
      gConsoleOutput.printf(" -append string    Append string to filename\n");
      gConsoleOutput.printf(" -outsamedir       Write output files to source path\n");
      gConsoleOutput.printf(" -timestamp        Compare timestamps and skip files that are not outdated\n");
      gConsoleOutput.printf(" -deep             Recurse subdirectories\n");
      gConsoleOutput.printf(" -recreate         Recreate directory structure\n");
      
      gConsoleOutput.printf(" -nooverwrite      Don't overwrite existing files\n");
      gConsoleOutput.printf(" -ignoreerrors     Don't stop on failed files\n");
      gConsoleOutput.printf(" -stats            Display statistics\n");
      //gConsoleOutput.printf(" -details          Print conversion information to log file\n");
      gConsoleOutput.printf(" -checkout         Use P4 to check out output file if read only\n");
      gConsoleOutput.printf(" -disableNumeric   Disable XML attrib/element text field compression\n");
      //gConsoleOutput.printf(" -permitUnicode    Unicode attrib/element text fields\n");
      gConsoleOutput.printf(" -forceUnicode     Force Unicode attrib/element text fields\n");
      gConsoleOutput.printf(" -littleEndian     Pack output for little endian machines\n");
      gConsoleOutput.printf(" -dump             Dump packed data to new XML file\n");
      gConsoleOutput.printf(" -rules filename   Use XML rules file\n");
      gConsoleOutput.printf(" -exclude substr   Exclude files that contain substr (multiple OK)\n");
      gConsoleOutput.printf(" -noTags           Don't include asset tag chunk in output file\n");
      gConsoleOutput.printf(" -delta            Only process changed files (required resource tags)\n");

      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams

class BXMLComp
{
   BCmdLineParams mCmdLineParams;

   uint mNumFilesProcessed;
   uint mNumFilesSkipped;
   uint mNumFailedFiles;
   uint mNumNonXMLFiles;

   struct BFileStats 
   {
      uint64 mTotalSize;
      uint mNumFiles;

      void update(uint64 size)
      {
         mTotalSize += size;            
         mNumFiles++;
      }
   };

   BFileStats mOverallFileStats;

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
         
   enum BConvertFileStatus
   {
      cCFSFailed = -1,
      cCFSSucceeded = 0,
      cCFSSkipped = 1,
   };
           
   BConvertFileStatus convertFile(const char* pSrcFilename, const char* pDstFilename)
   {
      gConsoleOutput.printf("Processing file: %s\n", pSrcFilename);
      
      if (!BConsoleAppHelper::checkOutputFileAttribs(pDstFilename, mCmdLineParams.mCheckOut))
         return cCFSFailed;

      BByteArray srcFileData;
      if (!BWin32FileUtils::readFileData(pSrcFilename, srcFileData))
      {
         gConsoleOutput.error("Failed reading file data: %s\n", pSrcFilename);
         return cCFSFailed;
      }
                  
      BXMLDocument xmlReader;
      if (FAILED(xmlReader.parse((const char*)srcFileData.getPtr(), srcFileData.getSizeInBytes())))
      {
         if (xmlReader.getErrorMessage().isEmpty())
            gConsoleOutput.error("XML parse failed for file: %s\n", pSrcFilename);
         else
            gConsoleOutput.error("XML parse failed for file: %s\nReason: %s\n", pSrcFilename, xmlReader.getErrorMessage().getPtr());
         return cCFSFailed;
      }
      
      gConsoleOutput.printf("Used Character Set: %s\n", xmlReader.getHasUnicodeChars() ? "Unicode" : "ANSI");
      gConsoleOutput.printf("Total Nodes: %u\n", xmlReader.getNumNodes());
      gConsoleOutput.printf("File size: %u\n", srcFileData.getSize());
      
      BByteArray packedData;
      BXMXStats stats;
      
      if (!mCmdLineParams.mLittleEndian)
      {  
         BXMXDataBuilder<true> dataBuilder(xmlReader, mCmdLineParams.mDisableNumerics, mCmdLineParams.mPermitUnicode, mCmdLineParams.mForceUnicode);
         
         if (!dataBuilder.getSuccess())
         {
            gConsoleOutput.error("XMX data build failed for file: %s\n", pSrcFilename);
            return cCFSFailed;
         }
         
         packedData = dataBuilder.getPackedData();
         stats = dataBuilder.getStats();
      }
      else
      {
         BXMXDataBuilder<false> dataBuilder(xmlReader, mCmdLineParams.mDisableNumerics, mCmdLineParams.mPermitUnicode, mCmdLineParams.mForceUnicode);
         
         if (!dataBuilder.getSuccess())
         {
            gConsoleOutput.error("XMX data build failed for file: %s\n", pSrcFilename);
            return cCFSFailed;
         }
         
         packedData = dataBuilder.getPackedData();
         stats = dataBuilder.getStats();
      }
      
      if (mCmdLineParams.mDumpPackedData)
      {
         BXMXDataBuilder<cBigEndianNative> dataBuilder(xmlReader, mCmdLineParams.mDisableNumerics, mCmdLineParams.mPermitUnicode, mCmdLineParams.mForceUnicode);

         if (!dataBuilder.getSuccess())
         {
            gConsoleOutput.error("XMX data build failed for file: %s\n", pSrcFilename);
            return cCFSFailed;
         }
         
         BString dumpFilename(pDstFilename);
         dumpFilename += ".xmldump";
         BFILETextDispatcher dispatcher(dumpFilename);
         if (!dispatcher.opened())
         {
            gConsoleOutput.error("Unable to open file for writing: %s\n", dumpFilename.getPtr());
         }

         BXMXDataDumper dumper;
         bool success = dumper.dump(dispatcher, BConstDataBuffer(dataBuilder.getPackedData().getPtr(), dataBuilder.getPackedData().getSizeInBytes()));
         if (!success)
         {
            gConsoleOutput.error("Failed dumping packed data for file: %s\n", pSrcFilename);
            return cCFSFailed;
         }
         
         gConsoleOutput.printf("Sucessfully wrote dump file: %s\n", pSrcFilename);
      }
      
      BDynamicStream compressedData;
      BDeflateStream compressedStream(compressedData);
      uint numBytesWritten = compressedStream.writeBytes(packedData.getPtr(), packedData.getSizeInBytes());
      if (numBytesWritten != packedData.getSizeInBytes())
      {  
         gConsoleOutput.error("Failed compressing packed data for file: %s\n", pSrcFilename);
         return cCFSFailed;
      }
      
      if (!compressedStream.close())
      {
         gConsoleOutput.error("Failed compressing packed data for file: %s\n", pSrcFilename);
         return cCFSFailed;
      }
      
      compressedData.seek(0);
      
      BInflateStream inflateStream;
      if (!inflateStream.open(compressedData))
      {
         gConsoleOutput.error("Failed compressing packed data for file: %s\n", pSrcFilename);
         return cCFSFailed;
      }
      
      BByteArray decompressedData(numBytesWritten);
      if (decompressedData.getSizeInBytes() != inflateStream.readBytes(decompressedData.getPtr(), decompressedData.getSizeInBytes()))
      {
         gConsoleOutput.error("Failed compressing packed data for file: %s\n", pSrcFilename);
         return cCFSFailed;
      }
      
      if (decompressedData != packedData)
      {
         gConsoleOutput.error("Failed compressing packed data for file: %s\n", pSrcFilename);
         return cCFSFailed;
      }
      
      gConsoleOutput.printf("Packed data verified OK.\n");
      
      gConsoleOutput.printf("Packed Data Size: %u\nCompressed Packed Data Size: %u\n", packedData.getSize(), compressedData.getBuf().getSize());
      
      BECFFileBuilder ecfFileBuilder;
      ecfFileBuilder.setID(cXMXECFFileID);
      
      BECFChunkData& packedDataChunk = ecfFileBuilder.addChunk((uint64)cXMXPackedDataChunkID, compressedData.getBuf());
      packedDataChunk.setAlignmentLog2(4);
      packedDataChunk.setResourceBitFlag(cECFChunkResFlagIsDeflateStream, true);
      
      if (!mCmdLineParams.mDisableTagChunk)
      {
         BResourceTagBuilder resourceTagBuilder;
         
         resourceTagBuilder.setPlatformID(mCmdLineParams.mLittleEndian ? BResourceTagHeader::cPIDPC : BResourceTagHeader::cPIDXbox);
         
         BString commandLine;
         commandLine.set("XMLCOMP -file %s");
         
         if (mCmdLineParams.mLittleEndian)      commandLine += " -littleEndian";
         if (mCmdLineParams.mDisableNumerics)   commandLine += " -disableNumerics";
         if (mCmdLineParams.mForceUnicode)      commandLine += " -forceUnicode";
                  
         resourceTagBuilder.setCreatorToolInfo(commandLine, cAssetTagCreatorToolVersion);
         
         resourceTagBuilder.setSourceFilename(pSrcFilename);
         if (!resourceTagBuilder.setSourceDigestAndTimeStamp(pSrcFilename))
         {
            gConsoleOutput.error("Failed computing source file digest for file: %s\n", pSrcFilename);
            return cCFSFailed;
         }
         
         if (!resourceTagBuilder.finalize())
         {
            gConsoleOutput.error("Failed computing resource tag for file: %s\n", pSrcFilename);
            return cCFSFailed;
         }
         
         const BByteArray& tagData = resourceTagBuilder.getFinalizedData();
         
         BECFChunkData& resourceChunk = ecfFileBuilder.addChunk((uint64)cResourceTagECFChunkID, tagData);
         resourceChunk.setResourceBitFlag(cECFChunkResFlagIsResourceTag, true);
      }
      
      uint64 outputFileSize = 0;
      if (!BWin32FileUtils::writeFileData(pDstFilename, ecfFileBuilder, &outputFileSize, true))      
      {
         gConsoleOutput.error("Failed writing output file: %s\n", pDstFilename);
         return cCFSFailed;  
      }      
      
      gConsoleOutput.printf("Successfully write output file: %s\n", pDstFilename);
      gConsoleOutput.printf("Output file size: %u\n", (uint)outputFileSize);
                  
      if (mCmdLineParams.mStatsFlag)
      {
         gConsoleOutput.printf("\n");
         gConsoleOutput.printf("Packed file statistics:\n");
         gConsoleOutput.printf("  NumNodes: %u\n", stats.mNumNodes);
         gConsoleOutput.printf("  NumAttributes: %u\n", stats.mNumAttributes);
         gConsoleOutput.printf("  NumInputVariants: %u\n", stats.mNumInputVariants);
         gConsoleOutput.printf("  NumRedundantVariants: %u\n", stats.mNumRedundantVariants);
         gConsoleOutput.printf("  NumDirectVariants: %u\n", stats.mNumDirectVariants);
         gConsoleOutput.printf("  NumIndirectVariants: %u\n", stats.mNumIndirectVariants);
         gConsoleOutput.printf("Variant statistics:\n");
         gConsoleOutput.printf("  NumBool: %u\n", stats.mNumBool);
         gConsoleOutput.printf("  NumInt24: %u\n", stats.mNumInt24);
         gConsoleOutput.printf("  NumFract24: %u\n", stats.mNumFract24);
         gConsoleOutput.printf("  NumInt32: %u\n", stats.mNumInt32);
         gConsoleOutput.printf("  NumFloat24: %u\n", stats.mNumFloat24);
         gConsoleOutput.printf("  NumFloat: %u\n", stats.mNumFloat);
         gConsoleOutput.printf("  NumDouble: %u\n", stats.mNumDouble);
         gConsoleOutput.printf("  NumDirectAnsiStrings: %u\n", stats.mNumDirectAnsiStrings);
         gConsoleOutput.printf("  NumIndirectAnsiStrings: %u\n", stats.mNumIndirectAnsiStrings);
         gConsoleOutput.printf("  NumIndirectUnicodeStrings: %u\n", stats.mNumIndirectUnicodeStrings);
         gConsoleOutput.printf("  NumIndirectStringBytes: %u\n", stats.mNumIndirectStringBytes);
         gConsoleOutput.printf("  NumNull: %u\n", stats.mNumNull);
         gConsoleOutput.printf("  NumVariantBytes: %u\n", stats.mNumVariantBytes);
         gConsoleOutput.printf("  NumFloatVecs: %u\n", stats.mNumFloatVecs);
      }         
      
      return cCFSSucceeded;
   }   

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

   bool processFiles(void)
   {
      if (!mSourceFiles.size())
      {
         gConsoleOutput.error("No files found to process.\n");
         return false;
      }
      
      for (uint fileIndex = 0; fileIndex < mSourceFiles.size(); fileIndex++)
      {
         const BFilePath& srcFilePath = mSourceFiles[fileIndex];
         BString srcRelPathname(srcFilePath.relPathname());
         BString srcFullFilename(srcFilePath.fullFilename());

         BString srcPath;
         BString srcFilename;
         strPathSplit(srcFullFilename, srcPath, srcFilename);
         
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

         char srcFName[_MAX_FNAME];
         char srcExt[_MAX_EXT];
         _splitpath_s(srcFilename.getPtr(), NULL, 0, NULL, 0, srcFName, sizeof(srcFName), srcExt, sizeof(srcExt));
         
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
         dstFullFilename += B(".");
         dstFullFilename += B(XMB_EXTENSION);
         
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

            if (mCmdLineParams.mTimestampFlag)
            {
               WIN32_FILE_ATTRIBUTE_DATA srcFileAttributes;
               const BOOL srcFileExists = GetFileAttributesEx(srcFullFilename, GetFileExInfoStandard, &srcFileAttributes);

               if (srcFileExists)
               {
                  LONG timeComp = CompareFileTime(&srcFileAttributes.ftLastWriteTime, &dstFileAttributes.ftLastWriteTime);
                  if (timeComp <= 0)
                  {
                     gConsoleOutput.printf("Skipping up to date file: %s\n", dstFullFilename.getPtr());
                     mNumFilesSkipped++;
                     continue;
                  }
               }                  
            }
            
            if (mCmdLineParams.mDeltaMode)
            {
               if (BResourceTagUtils::fileIsUnchanged(srcFullFilename, dstFullFilename, &cAssetTagCreatorToolVersion))
               {
                  gConsoleOutput.printf("Skipping up to date file: %s\n", dstFullFilename.getPtr());
                  mNumFilesSkipped++;
                  continue;
               }
            }
         }

         gConsoleOutput.printf("Processing file %u of %u: %s\n", 1 + fileIndex, mSourceFiles.size(), srcFullFilename.getPtr());                                                      
         BConvertFileStatus success = convertFile(srcFullFilename, dstFullFilename);

         if (success == cCFSFailed)
         {
            mNumFailedFiles++;

            gConsoleOutput.error("Failed processing file: %s\n", srcFullFilename.getPtr());

            if (!mCmdLineParams.mIgnoreErrors)
               return false;
         }
         else if (success == cCFSSkipped)
         {
            mNumFilesSkipped++;
         }
         else
         {
            mNumFilesProcessed++;
         }
      }

      return true;
   }

   void clear(void)
   {
      mSourceFiles.clear();

      mNumFilesProcessed = 0;
      mNumFilesSkipped = 0;
      mNumFailedFiles = 0;
      mNumNonXMLFiles = 0;

      Utils::ClearObj(mOverallFileStats);
   }

public:
   BXMLComp()
   {
      clear();
   }

   bool process(BCommandLineParser::BStringArray& args)
   {
      clear();

      if (!processParams(args))
         return false;

      if (!findFiles())
         return false;

      bool status = processFiles();

      gConsoleOutput.printf("Files processed successfully: %i, skipped: %i, failed: %i, non-XML files skipped: %i\n", mNumFilesProcessed, mNumFilesSkipped, mNumFailedFiles, mNumNonXMLFiles);

      if (mCmdLineParams.mFileStats)
      {
         const BFileStats& stats = mOverallFileStats;

         gConsoleOutput.printf("Overall file stats:\n");

         gConsoleOutput.printf("    Num files: %u\n", stats.mNumFiles);
         gConsoleOutput.printf("    Total size: %u\n", stats.mTotalSize);
      }

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
   
   gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s, Creating XMX Data Version 0x%08X\n", __DATE__, __TIME__, BXMXData<true, false>::cSig);
   
   BXMLComp xmlComp;
   const bool success = xmlComp.process(args);
   
   BConsoleAppHelper::deinit();
 
   XCoreRelease();
   return success ? EXIT_SUCCESS : EXIT_FAILURE;  
}