// File: ecfCheck.cpp
#include "xcore.h"
#include "xcoreLib.h"
#include "resource\ecfUtils.h"
#include "file\win32FindFiles.h"
#include "file\win32File.h"
#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"

static void printUsage(void)
{
   printf("Usage: ecfCheck filespec\n");
   printf("-deep                    Recurse subdirectories\n");
   
   BConsoleAppHelper::printHelp();
}

static void exitFailure(void)
{
   BConsoleAppHelper::deinit();
   XCoreRelease();
   exit(EXIT_FAILURE);
}

int main(int argC, const char* argV[])
{
   XCoreCreate();
   BConsoleAppHelper::setup();
   
   BCommandLineParser::BStringArray args;
   BConsoleAppHelper::init(args, argC, argV);
   
   if (args.getSize() < 2)
   {
      printUsage();
      return EXIT_FAILURE;
   }

   bool recurseFlag = false;
   
   const BCLParam clParams[] =
   {
      {"deep", cCLParamTypeFlag, &recurseFlag },
      { NULL }
   };

   BCommandLineParser parser(clParams);

   bool success = parser.parse(args, true, false);
   
   if (!success)
   {
      gConsoleOutput.error("%s\n", parser.getErrorString());
      exitFailure();
   }

   if (parser.getUnparsedParams().empty())
   {
      gConsoleOutput.error("Must specify filename!\n");
      exitFailure();
   }
   
   BString filespec(argV[parser.getUnparsedParams()[0]]);
   BString path, mask;
   
   strPathSplit(filespec, path, mask);
   if (mask.isEmpty())
      mask = "*";
   
   BFindFiles findFiles;
   success = findFiles.scan(
      path, 
      mask, 
      BFindFiles::FIND_FILES_WANT_FILES | (recurseFlag ? BFindFiles::FIND_FILES_RECURSE_SUBDIRS : 0));
      
   if (!success)
   {
      gConsoleOutput.error("Unable to find files: %s\n", filespec.getPtr());
      exitFailure();
   }      
   
   if (findFiles.numFiles() == 0)
   {
      gConsoleOutput.error("No files found: %s\n", filespec.getPtr());
      exitFailure();
   }
      
   uint numGoodFiles = 0;
   uint numBadFiles = 0;
   uint numFailedFiles = 0;
   
   for (uint i = 0; i < findFiles.numFiles(); i++)
   {
      const BString& filename = findFiles.getFile(i).fullFilename();
      
      BWin32File file;
      if (!file.open(filename))
      {
         gConsoleOutput.error("Unable to open file: %s\n", filename.getPtr());
         numFailedFiles++;
         continue;
      }
      
      BByteArray buf;
      if (!file.readArray(buf))
      {
         gConsoleOutput.error("Unable to read file: %s\n", filename.getPtr());
         numFailedFiles++;
         continue;
      }
      
      BECFFileReader ecfReader(BConstDataBuffer(buf.getPtr(), buf.getSize()));
      
      if (!ecfReader.check())
      {
         gConsoleOutput.error("File failed ECF header or Adler-32 check: %s\n", filename.getPtr());
         numBadFiles++;
         continue;
      }
      
      gConsoleOutput.printf("OK, ID 0x%08X, Chunks: %u: %s\n", ecfReader.getHeader()->getID(), ecfReader.getHeader()->getNumChunks(), filename.getPtr());
      
      numGoodFiles++;
   }
   
   gConsoleOutput.printf("Good Files: %u, Bad Files: %u, Failed Files: %u\n", numGoodFiles, numBadFiles, numFailedFiles);
   
   BConsoleAppHelper::deinit();
   
   XCoreRelease();
      
   return success ? EXIT_SUCCESS : EXIT_FAILURE;
}