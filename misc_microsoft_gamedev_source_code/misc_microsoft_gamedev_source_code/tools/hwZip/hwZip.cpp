//============================================================================
//
//  File: hwZip.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "xcorelib.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"
#include "file\win32FileUtils.h"
#include "utils\commandLineParser.h"
#include "file\win32FileStream.h"
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"
#include "compressedStream.h"

#define PROGRAM_TITLE "hwZip"

//============================================================================
// 
//============================================================================
class BCmdLineParams
{
   public:
      BCommandLineParser::BStringArray mFilenames;
      BString                          mSuffix;

      bool mDecompress;
      bool mFaster;
      bool mBetter;
      bool mRecurse;

      BCmdLineParams() :
         mDecompress(false),
         mFaster(false),
         mBetter(true)
      {
         mSuffix = ".hwz";
      }

      void clear(void)
      {
      }

      bool parse(BCommandLineParser::BStringArray& args)
      {
         const BCLParam clParams[] =
         {
            {"d",          cCLParamTypeFlag,             &mDecompress },
            {"1",          cCLParamTypeFlag,             &mFaster },
            {"9",          cCLParamTypeFlag,             &mBetter },
            {"S",          cCLParamTypeBStringPtr,       &mSuffix },
            {"r",          cCLParamTypeFlag,             &mRecurse },

            { NULL } 
         };

         BCommandLineParser parser(clParams);

         const bool success = parser.parse(args, false, false);

         if (!success)
         {
            gConsoleOutput.error("%s\n", parser.getErrorString());
            return false;
         }

         const BDynamicArray<uint>& unparsedParams = parser.getUnparsedParams();

         for (uint j = 0; j < unparsedParams.getSize(); j++)
         {
            const uint i = unparsedParams[j];

            if (args[i].isEmpty())
               continue;

            mFilenames.pushBack(args[i]);
         }

         // if no filenames, use stdin

         return true;
      }

      void printHelp(void)
      {
         //      --------------------------------------------------------------------------------
         gConsoleOutput.printf("Usage: hwZip [-d19] [-S suffix] [file ...]\n");
         gConsoleOutput.printf("\n");
         gConsoleOutput.printf("Options:\n");
         gConsoleOutput.printf(" -d      decompress\n");
         gConsoleOutput.printf(" -1      compress faster\n");
         gConsoleOutput.printf(" -9      compress better\n");
         gConsoleOutput.printf(" -S      use suffix on compressed files\n");
         gConsoleOutput.printf(" -r      recurse subdirectories\n");
         gConsoleOutput.printf(" file... files to (de)compress.\n");
         gConsoleOutput.printf("\n");

         BConsoleAppHelper::printHelp();
      }
}; // class BCmdLineParams

//============================================================================
// 
//============================================================================
static bool compress(const BCmdLineParams& params, const char* pSrcFilename)
{
   BString dstFileName;
   
   dstFileName.format("%s%s", pSrcFilename, params.mSuffix);

   BByteArray srcFileData;
   if (!BWin32FileUtils::readFileData(pSrcFilename, srcFileData))
   {
      gConsoleOutput.error("Failed reading file data: %s\n", pSrcFilename);
      return false;
   }

   int maxCompares = 500;
   bool greedy = false;

   if (params.mBetter)
   {
      maxCompares = 1000;
      greedy = false;
   }
   else if (params.mFaster)
   {
      maxCompares = 128;
      greedy = true;
   }

   BDynamicStream compressedData;
   BDeflateStream compressedStream(compressedData, cSFWritable, maxCompares, DEFL_ALL_BLOCKS, greedy);
   uint numBytesWritten = compressedStream.writeBytes(srcFileData.getPtr(), srcFileData.getSizeInBytes());
   if (numBytesWritten != srcFileData.getSizeInBytes())
   {
      gConsoleOutput.error("Failed compressing data for file: %s\n", pSrcFilename);
      return false;
   }

   if (!compressedStream.close())
   {
      gConsoleOutput.error("Failed to close compressed data stream for file: %s\n", pSrcFilename);
      return false;
   }

   // save out the compressed data to disk
   if (!BWin32FileUtils::writeFileData(dstFileName.getPtr(), compressedData.getBuf(), false))
   {
      gConsoleOutput.error("Failed writing compressed data for file %s to %s\n", pSrcFilename, dstFileName.getPtr());
      return false;
   }

   return true;
}

//============================================================================
// 
//============================================================================
static bool decompress(const BCmdLineParams& params, const char* pSrcFilename)
{
   BString dstFileName(pSrcFilename);

   int suffixIndex = dstFileName.findRight(params.mSuffix);
   if (suffixIndex == -1)
   {
      gConsoleOutput.error("Unknown suffix for: %s\n", pSrcFilename);
      return false;
   }

   dstFileName.crop(0, suffixIndex - 1);

   BByteArray srcFileData;
   if (!BWin32FileUtils::readFileData(pSrcFilename, srcFileData))
   {
      gConsoleOutput.error("Failed reading file data: %s\n", pSrcFilename);
      return false;
   }

   BByteStream byteStream(srcFileData.getPtr(), srcFileData.getSize());
   BInflateStream inflateStream;
   if (!inflateStream.open(byteStream))
   {
      gConsoleOutput.error("Failed opening compressed file: %s\n", pSrcFilename);
      return false;
   }

   if (!inflateStream.sizeKnown())
   {
      gConsoleOutput.error("Unknown size for file: %s\n", pSrcFilename);
      return false;
   }

   if (inflateStream.size() > UINT_MAX)
   {
      gConsoleOutput.error("Inflate size > UINT_MAX for file: %s\n", pSrcFilename);
      return false;
   }

   uint64 size = inflateStream.size();
   if (size == 0)
   {
      BWin32FileUtils::copyFile(pSrcFilename, dstFileName.getPtr());
      return true;
   }

   if (size == BSTREAM_UNKNOWN_SIZE)
   {
      gConsoleOutput.error("Unknown size for file: %s\n", pSrcFilename);
      return false;
   }

   BByteArray decompressedData(static_cast<uint>(size));
   if (decompressedData.getSizeInBytes() != inflateStream.readBytes(decompressedData.getPtr(), decompressedData.getSizeInBytes()))
   {
      gConsoleOutput.error("Failed decompressing file: %s\n", pSrcFilename);
      return false;
   }

   if (!inflateStream.close())
   {
      gConsoleOutput.error("Failed to close the inflated stream for file: %s\n", pSrcFilename);
      return false;
   }

   // save out the decompressedData to disk
   if (!BWin32FileUtils::writeFileData(dstFileName.getPtr(), decompressedData, false))
   {
      gConsoleOutput.error("Failed writing decompressed file %s to %s\n", pSrcFilename, dstFileName.getPtr());
      return false;
   }

   return true;
}

//============================================================================
// 
//============================================================================
static bool compress(BCommandLineParser::BStringArray args)
{
   // parse the args and then either compress/decompress a given file
   BCmdLineParams mCmdLineParams;

   if (args.getSize() < 2)
   {
      mCmdLineParams.printHelp();
      return false;
   }

   if (!mCmdLineParams.parse(args))
      return false;

   // go through each file and (de)compress
   for (uint i=0; i < mCmdLineParams.mFilenames.getSize(); ++i)
   {
      if (mCmdLineParams.mDecompress)
      {
         if (!decompress(mCmdLineParams, mCmdLineParams.mFilenames.get(i)))
            return false;
      }
      else
      {
         if (!compress(mCmdLineParams, mCmdLineParams.mFilenames.get(i)))
            return false;
      }
   }

   return true;
}

//============================================================================
// 
//============================================================================
static int mainInternal(int argC, const char* argV[])
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

   bool success = compress(args);

   XCoreRelease();

   if (!success)
      return EXIT_FAILURE;

   return EXIT_SUCCESS;
}

//============================================================================
// 
//============================================================================
int main(int argC, const char* argV[])
{
   int status;

#ifndef BUILD_DEBUG   
   __try
#endif   
   {
      status = mainInternal(argC, argV);
   }
#ifndef BUILD_DEBUG   
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      fprintf(stderr, PROGRAM_TITLE ": Unhandled exception!");
      return 1;
   }
#endif   

   return status;
}
