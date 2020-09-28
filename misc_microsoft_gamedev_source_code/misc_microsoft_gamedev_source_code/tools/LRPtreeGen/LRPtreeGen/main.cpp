//============================================================================
//
//  File: main.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================

//XCORE
#include "xcore.h"
#include "xcorelib.h"

//#include "xsystemlib.h"

#include "file\win32FileStream.h"
#include "resource/resourceTag.h"
#include "resource/ecfFileData.h"
#include "resource\ecfUtils.h"

#include "stream\dynamicStream.h"
#include "xml\xmxDataBuilder.h"
#include "xml\xmxDataDumper.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"

#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "utils\endianSwitch.h"
#include "resource\ecfUtils.h"
#include "resource\resourceTag.h"

#include "obstructionmanager.h"
#include "lrpTree.h"
#include "TerrainSimRep.h"
#include "pather.h"

#include <fstream>
#include <vector>


//general
#include <conio.h>


#define PROGRAM_TITLE "GENLRP" 

enum 
{
   SUCCESS = 0,
   FAIL_COULD_NOT_INIT = 1,
   FAIL_NO_TERRAIN_FILE =  2,
   FAIL_TO_SAVE_LRP_FILE  = 3,
};

//--------------------------------------------------
class BCmdLineParams
{
public:
   BString mTerrainFile;
   BString mLrpTreeFile;
   //BCommandLineParser::BStringArray mExcludeStrings;
   BString mOutputPathString;
   BString mAppendString;
   //BString mRulesFilename;

   bool mOutSameDirFlag;
   bool mTimestampFlag;
   bool mDeepFlag;
   bool mRecreateFlag;
   bool mNoOverwrite;
   bool mIgnoreErrors;

   bool mSimulateFlag;
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
      mSimulateFlag(false),
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
         {"terrainFile",           cCLParamTypeBStringPtr, &mTerrainFile },
         {"lrpTreeFile",           cCLParamTypeBStringPtr, &mLrpTreeFile },
         //{"append",              cCLParamTypeBStringPtr, &mAppendString },
         //{"outsamedir",          cCLParamTypeFlag, &mOutSameDirFlag },
         //{"nooverwrite",         cCLParamTypeFlag, &mNoOverwrite },
         //{"timestamp",           cCLParamTypeFlag, &mTimestampFlag },
         //{"deep",                cCLParamTypeFlag, &mDeepFlag },
         //{"recreate",            cCLParamTypeFlag, &mRecreateFlag },
         //{"ignoreerrors",        cCLParamTypeFlag, &mIgnoreErrors },
         //{"simulate",            cCLParamTypeFlag, &mSimulateFlag },
         //{"stats",               cCLParamTypeFlag, &mStatsFlag },
         //{"filestats",           cCLParamTypeFlag, &mFileStats },
         ////{"details",             cCLParamTypeFlag, &mConversionDetails },
         //{"checkout",            cCLParamTypeFlag, &mCheckOut },
         //{"disableNumerics",     cCLParamTypeFlag, &mDisableNumerics },
         ////{"permitUnicode",       cCLParamTypeFlag, &mPermitUnicode },
         //{"forceUnicode",        cCLParamTypeFlag, &mForceUnicode },
         //{"littleEndian",        cCLParamTypeFlag, &mLittleEndian },
         ////{"rules",               cCLParamTypeBStringPtr, &mRulesFilename },
         //{"dump",                cCLParamTypeFlag, &mDumpPackedData },
         //{"exclude",             cCLParamTypeBStringArrayPtr, &mExcludeStrings },
         //{"noTags",              cCLParamTypeFlag, &mDisableTagChunk },
         //{"delta",               cCLParamTypeFlag, &mDeltaMode },
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
      gConsoleOutput.printf(" ");
      gConsoleOutput.printf("Usage: genLRPTree <options>\n");
      gConsoleOutput.printf("Options:\n");

      gConsoleOutput.printf(" -terrainFile filename    Specify source terrain file (*.??)\n");
      gConsoleOutput.printf(" -lrpTreeFile filename     Specify output lrpTree file (*.LRP)\n");
      //gConsoleOutput.printf(" -outfile filename Specify output filename\n");
      //gConsoleOutput.printf(" -append string    Append string to filename\n");
      //gConsoleOutput.printf(" -outsamedir       Write output files to source path\n");
      //gConsoleOutput.printf(" -timestamp        Compare timestamps and skip files that are not outdated\n");
      //gConsoleOutput.printf(" -deep             Recurse subdirectories\n");
      //gConsoleOutput.printf(" -recreate         Recreate directory structure\n");
      //gConsoleOutput.printf(" -simulate         Only print filenames of files to be processed\n");
      //gConsoleOutput.printf(" -nooverwrite      Don't overwrite existing files\n");
      //gConsoleOutput.printf(" -ignoreerrors     Don't stop on failed files\n");
      //gConsoleOutput.printf(" -stats            Display statistics\n");
      //gConsoleOutput.printf(" -details          Print conversion information to log file\n");
      //gConsoleOutput.printf(" -checkout         Use P4 to check out output file if read only\n");
      //gConsoleOutput.printf(" -disableNumeric   Disable XML attrib/element text field compression\n");
      //gConsoleOutput.printf(" -permitUnicode    Unicode attrib/element text fields\n");
      //gConsoleOutput.printf(" -forceUnicode     Force Unicode attrib/element text fields\n");
      //gConsoleOutput.printf(" -littleEndian     Pack output for little endian machines\n");
      //gConsoleOutput.printf(" -dump             Dump packed data to new XML file\n");
      //gConsoleOutput.printf(" -rules filename   Use XML rules file\n");
      //gConsoleOutput.printf(" -exclude substr   Exclude files that contain substr (multiple OK)\n");
      //gConsoleOutput.printf(" -noTags           Don't include asset tag chunk in output file\n");
      //gConsoleOutput.printf(" -delta            Only process changed files (uses XMB resource tags)\n");
      gConsoleOutput.printf(" ");
      gConsoleOutput.printf(" ");
      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams



BCmdLineParams mCmdLineParams;


//-----------------------------------------


//==============================================================================
// genLRPFile
//==============================================================================
bool genLRPFile()
{
   gObsManager.initialize();
   initObstructionRotationTables();

   // Test for pre-loading obstructions
   gObsManager.generateWorldBoundries();
   gObsManager.generateTerrainObstructions(gTerrainSimRep.getObstructionMap());
   if (gPather.saveLRPTree(mCmdLineParams.mLrpTreeFile))
      return true;
   else
      return false;

} // genLRPFile

//--------------------------------------------------

bool processParams(BCommandLineParser::BStringArray& args)
{
   if (args.getSize() < 2)
   {
      mCmdLineParams.printHelp();
      return false;
   }

   if (!mCmdLineParams.parse(args))
      return false;

   if (mCmdLineParams.mTerrainFile=="")
   {
      gConsoleOutput.error("No files specified to process!\n");
      return false;  
   }         

   return true;
}


//============================================================================
// fileManagerInit
//============================================================================
static void initFileManager(bool disableLooseFiles, bool enableArchives)
{
#define ARCHIVE_PASSWORD "3zDdptN*rV=qOkRbE*NAuWM6"

   uint64 decryptKey[3];
//   teaCryptInitKeys(ARCHIVE_PASSWORD, decryptKey[0], decryptKey[1], decryptKey[2]);

   BSHA1 signatureKeys[2];
   uint numKeys = 0;

   const unsigned char releasePublicKey[] = { 0x9C,0xEA,0x65,0xF5,0xB4,0xB9,0x66,0xD0,0xDD,0x99,0x31,0xCD,0xA7,0x9F,0x94,0x47,0x2D,0xF3,0xAB,0xC8 };
   signatureKeys[numKeys].set(releasePublicKey);
   numKeys++;

#ifndef BUILD_FINAL      
   const unsigned char testPublicKey[] = { 0x67,0xFB,0xA6,0x9F,0x4B,0x1F,0x42,0xB6,0xFC,0x11,0x71,0x61,0xD2,0x59,0x65,0x97,0x6C,0xDB,0x41,0xE7 };
   signatureKeys[numKeys].set(testPublicKey);
   numKeys++;
#endif
//   gAsyncFileManager.init();            

   //const uint fileManagerConfigFlags = BFileManager::cEnableLooseFiles | BFileManager::cDisableFailedArchives | BFileManager::cRecordOpenedFiles | BFileManager::cBypassXFSForArchives;
   //const uint fileManagerConfigFlags = BFileManager::cPanicOnFailedArchives | BFileManager::cRecordOpenedFiles | BFileManager::cBypassXFSForArchives;
   uint fileManagerConfigFlags = BFileManager::cPanicOnFailedArchives | BFileManager::cBypassXFSForArchives;
#ifndef BUILD_FINAL      
   fileManagerConfigFlags |= BFileManager::cRecordOpenedFiles;
#endif      
   if (!disableLooseFiles)
      fileManagerConfigFlags |= BFileManager::cEnableLooseFiles;
   if (enableArchives)
      fileManagerConfigFlags |= BFileManager::cWillBeUsingArchives;

   gFileManager.init(fileManagerConfigFlags, numKeys, signatureKeys, decryptKey);

//   gFileManager.setDirList(gFileManager.getProductionDirListID(), gXSystemInfo.mProductionDir);

   eFileManagerError result;
   if (enableArchives)
   {
      BHandle rootFileCacheHandle;
      result = gFileManager.addFileCache("d:\\root.era", rootFileCacheHandle, false, true);
      if (result == cFME_SUCCESS)
      {
         gFileManager.enableFileCache(rootFileCacheHandle);
      }
   }         
}      


//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
int main(int argC, const char** argV)
{
   XCoreCreate();

   initFileManager(false, false);

   BConsoleAppHelper::setup();

   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argC, argV))
   {
      BConsoleAppHelper::deinit();

      XCoreRelease();
      return FAIL_COULD_NOT_INIT;
   }

   if (!BConsoleAppHelper::getQuiet())
      gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s, Creating LRPTree Data Version 0x%08X\n", __DATE__, __TIME__, cLRPHeaderID);


   bool success = true;
   if(processParams(args))
   {
      if(!gTerrainSimRep.loadECFXSDInternal(cDirBase, mCmdLineParams.mTerrainFile))
      {
         gConsoleOutput.printf("FAILED: Failed to read Terrain File - ensure source path and file name are correct\n");
         BConsoleAppHelper::pause();
         return FAIL_NO_TERRAIN_FILE;
      }

      success = genLRPFile();
   }

   BConsoleAppHelper::deinit();

   XCoreRelease();

   if(!success)
   {
      gConsoleOutput.printf("FAILED: Failed to save LRP File - ensure destination path and file name are correct\n");
      BConsoleAppHelper::pause();
      return FAIL_TO_SAVE_LRP_FILE;
   }

   return SUCCESS;  
}