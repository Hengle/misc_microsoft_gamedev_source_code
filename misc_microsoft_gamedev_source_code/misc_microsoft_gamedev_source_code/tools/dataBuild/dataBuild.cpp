//============================================================================
//
//  File: dataBuild.cpp
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
#include "resource\ecfHeaderIDs.h"
#include "resource\resourceTag.h"
#include "file\win32FileStream.h"
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"
#include "containers\hashMap.h"
#include "ecfArchiver.h"
#include "xml\xmlDocument.h"
#include "utils\spawn.h"
#include "math\randomUtils.h"
#include "multiprocessManager.h"

#include "buildClient.h"
#include "buildServer.h"
#include "clientAdvertisement.h"

#define TEMP_FILENAME      "___tmp___.!tmp!"

#define PROGRAM_TITLE      "DATABUILD" 
#define ARCHIVE_EXTENSION  "ERA"
#define DEPGEN_FILENAME    "tools\\depgen\\depgen.exe"
#define ECFARC_FILENAME    "tools\\ecfarc\\ecfarc.exe"
#define ARCGEN_FILENAME    "tools\\arcgen\\arcgen.exe"
#define TAGGEN_FILENAME    "tools\\taggen\\taggen.exe"

class BCmdLineParams
{
public:
   BString                          mMode;
   BCommandLineParser::BStringArray mTargets;
   BCommandLineParser::BStringArray mScenarioTargets;
   
   BString                          mConfigFile;
   BCommandLineParser::BStringArray mDefines;
   BCommandLineParser::BStringArray mRawDefines;
   BCommandLineParser::BStringArray mDefineFiles;
   BString                          mArcGenDefineFile;
      
   BString                          mWorkPath;
   BString                          mIntermediatePath;
   BString                          mFinalPath;
   
   bool                             mIgnoreTagErrors;
   int                              mMaxProcesses;
   bool                             mFull;
   
   bool                             mFindClients;
   bool                             mUpdateClients;
   bool                             mUpdateServer;
   bool                             mDefineDepotRevision;
   bool                             mPostBuild;
   bool                             mCleanup;
      
   BCommandLineParser::BStringArray mClients;
   
   BString                          mBuildNumberFile;
   BString                          mDepotRevisionFile;
         
   BCmdLineParams() 
   {
      clear();
   }
   
   void clear(void)
   {
      mMode = "normal";
      mTargets.clear();
      mScenarioTargets.clear();
      mConfigFile.empty();
      mDefines.clear();
      mRawDefines.clear();      
      mDefineFiles.clear();
      mArcGenDefineFile = "defaultDefines.txt";
      mIgnoreTagErrors = false;
      mMaxProcesses = 8;
      mFull = false;
      mWorkPath.empty();
      mIntermediatePath.empty();
      mFinalPath.empty();
      
      mClients.clear();
      
      mFindClients = false;
      mUpdateClients = false;
      mUpdateServer = false;
      mDefineDepotRevision = false;
      
      mBuildNumberFile.empty();
      mDepotRevisionFile.empty();
      
      mPostBuild = false;
      mCleanup = false;
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"mode",                cCLParamTypeBStringPtr,       &mMode },
         {"target",              cCLParamTypeBStringArrayPtr,  &mTargets },
         {"scenarioTarget",      cCLParamTypeBStringArrayPtr,  &mScenarioTargets },
         {"ignoretagerrors",     cCLParamTypeFlag,             &mIgnoreTagErrors },
         {"maxprocesses",        cCLParamTypeIntPtr,           &mMaxProcesses},
         {"full",                cCLParamTypeFlag,             &mFull },
         {"configFile",          cCLParamTypeBStringPtr,       &mConfigFile },
         {"define",              cCLParamTypeBStringArrayPtr,  &mDefines },
         {"rawDefine",           cCLParamTypeBStringArrayPtr,  &mRawDefines },
         {"defineFile",          cCLParamTypeBStringArrayPtr,  &mDefineFiles },
         {"arcGenDefineFile",    cCLParamTypeBStringPtr,       &mArcGenDefineFile },
         {"workPath",            cCLParamTypeBStringPtr,       &mWorkPath },
         {"intermediatePath",    cCLParamTypeBStringPtr,       &mIntermediatePath },
         {"finalPath",           cCLParamTypeBStringPtr,       &mFinalPath },
         {"useClient",           cCLParamTypeBStringArrayPtr,  &mClients },
         {"findClients",         cCLParamTypeFlag,             &mFindClients },
         {"updateClients",       cCLParamTypeFlag,             &mUpdateClients },
         {"updateServer",        cCLParamTypeFlag,             &mUpdateServer },         
         {"defineDepotRevision", cCLParamTypeFlag,             &mDefineDepotRevision },         
         {"buildNumberFile",     cCLParamTypeBStringPtr,       &mBuildNumberFile },
         {"depotRevisionFile",   cCLParamTypeBStringPtr,       &mDepotRevisionFile },
         {"postBuild",           cCLParamTypeFlag,             &mPostBuild },
         {"cleanup",             cCLParamTypeFlag,             &mCleanup },
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
      
      mMaxProcesses = Math::Clamp(mMaxProcesses, 1, 16);

      return true;
   }

   void printHelp(void)
   {
      //      --------------------------------------------------------------------------------
      gConsoleOutput.printf("Usage: dataBuild <options>\n");
      gConsoleOutput.printf("\n");
      gConsoleOutput.printf("Options:\n");
      gConsoleOutput.printf(" -mode quick,normal,final                Processing mode (defaults to normal)\n");
      gConsoleOutput.printf(" -target allStandalone, targetName, scenarioFilename, Build target (multiple OK)\n");
      gConsoleOutput.printf(" -scenarioTarget scenarioType            Scenario type build target - uses scenariodescriptions.xml (multiple OK)\n");
      gConsoleOutput.printf(" -configFile filename.xml                Configuration file (defaults to config.xml)\n");
      gConsoleOutput.printf(" -ignoreTagErrors                        Ignore tag validation errors\n");
      gConsoleOutput.printf(" -maxProcesses value                     Set max number of parallel processes\n");
      gConsoleOutput.printf(" -full                                   Ignore file timestamps\n");
      gConsoleOutput.printf(" -define name=value                      Define variable\n");
      gConsoleOutput.printf(" -defineFile filename.txt                Read file containing defines\n");
      gConsoleOutput.printf(" -arcGenDefineFile filename.txt          arcgen define file (defaults to defaultDefines.txt)\n");
      gConsoleOutput.printf(" -workPath path                          Set work path\n");
      gConsoleOutput.printf(" -intermediatePath path                  Set intermediate path\n");
      gConsoleOutput.printf(" -finalPath path                         Set final path\n");
      gConsoleOutput.printf(" -client                                 Run as client\n");
      gConsoleOutput.printf(" -useClient address                      Connect to client IP\n");
      gConsoleOutput.printf(" -findClients                            Find clients by accessing esfile\n");
      gConsoleOutput.printf(" -updateClients                          Update client machines\n");
      gConsoleOutput.printf(" -updateServer                           Update server\n");
      gConsoleOutput.printf(" -postBuild                              Execute post build\n");
      gConsoleOutput.printf(" -cleanup                                Execute cleanup commands\n");
      gConsoleOutput.printf(" -defineDepotRevision                    Use current data/time for depot revision\n");
      gConsoleOutput.printf("\n");
      gConsoleOutput.printf("Client options only:\n");
      gConsoleOutput.printf(" -enableExecUpdate                       Automatically update client executable\n");
      gConsoleOutput.printf(" -listenAddress address                  Listen for connections at specified address\n");
      gConsoleOutput.printf(" -buildNumberFile filename               Update build number text file\n");
      gConsoleOutput.printf(" -depotRevisionFile filename             Write depot revision to text file\n");
      gConsoleOutput.printf("\n");
            
      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams

class BVariableManager
{
public:
   BVariableManager()
   {
   }
   
   void clear(void)
   {
      mVariables.clear();
   }
   
   bool isDefined(const char* pName) const
   {
      BDEBUG_ASSERT(pName);
      
      BFixedString256 name(pName);
      name.tolower();
      
      return mVariables.find(name.getPtr()) != mVariables.end();
   }
   
   void define(const char* pName, const char* pValue = NULL)
   {
      BDEBUG_ASSERT(pName);
      
      BFixedString256 name(pName);
      name.tolower();
                  
      undefine(name);
      
      mVariables.insert(name.getPtr(), pValue ? pValue : "");
   }
   
   bool undefine(const char* pName)
   {
      BDEBUG_ASSERT(pName);
      
      BFixedString256 name(pName);
      name.tolower();
      
      return mVariables.erase(name.getPtr());
   }
   
   const char* getValue(const char* pName) const
   {
      BDEBUG_ASSERT(pName);
      
      BFixedString256 name(pName);
      name.tolower();
      
      BVariableHashMap::const_iterator it = mVariables.find(name.getPtr());
      if (it == mVariables.end())
         return NULL;
      return it->second.getPtr();
   }
         
   typedef BHashMap<BString, BString> BVariableHashMap;
   
   const BVariableHashMap& getTable(void) const   { return mVariables; }
         BVariableHashMap& getTable(void)         { return mVariables; }
            
private:
   BVariableHashMap mVariables;
};

class BDataBuild
{
public:
   BDataBuild()
   {
      clear();
   }
   
   bool process(BCommandLineParser::BStringArray& args)
   {
      const bool success = processInternal(args);
      
      executeCleanupCommand(success);
      
      return success;
   }
   
   bool processInternal(BCommandLineParser::BStringArray& args)
   {
      if (!processParams(args))
         return false;

      printCurrentTime();            
      
      if (!findDirectories())
      {
         gConsoleOutput.error("Unable to find work/intermediate directories!\n");
         return false;
      }
            
      mVariables.clear();
      
      if (!readConfig())
         return false;
      
      if (!initVariables())
         return false;
         
      if (!cleanCache())
         return false;
         
      if (!mCmdLineParams.mBuildNumberFile.isEmpty())         
      {
         if (!createBuildNumberFile())
            return false;
      }
      
      if (mCmdLineParams.mDefineDepotRevision)
      {
         if (!defineDepotRevision())
            return false;
      }
      
      if (!mCmdLineParams.mDepotRevisionFile.isEmpty())
      {
         if (!writeDepotRevisionFile())
            return false;
      }
      
      if (mCmdLineParams.mUpdateServer)
      {
         if (!updateServer())
            return false;
      }
      
      if (!initBuildServer())         
         return false;
         
      bool success = buildTargets();
                           
      deinitBuildServer();
      
      if (success)
      {
         if (mCmdLineParams.mPostBuild)  
         {
            if (!executePostBuildCommand())
               success = false;
         }
      }
      
      return success;
   }
   
private:
   typedef BHashMap<BString>        BStringMap;
   typedef BHashMap<BString, uint>  BStringIndexMap;
   typedef BDynamicArray<BString>   BStringArray;
   
   BString           mWorkDirectory;
   BString           mExecDirectory;
   BString           mIntermediateDirectory;
   BString           mFinalDirectory;
   
   BCmdLineParams    mCmdLineParams;
   
   BVariableManager  mVariables;
      
   enum eMode
   {
      cQuick,
      cNormal,
      cFinal
   };
   eMode             mMode;
   
   BMultiprocessManager mMultiprocessManager;
   BBuildServer         mBuildServer;
   
   uint64 mRandSeed;
   
   void generateRandomFilename(BString& randName, const char* pPrefix, const char* pExt)
   {
      if (!mRandSeed)
         mRandSeed = RandomUtils::GenerateRandomSeed();

      uchar randBuf[16];
      mRandSeed = RandomUtils::RandomFill64(randBuf, sizeof(randBuf), mRandSeed);

      if (pPrefix)
         randName.set(pPrefix);
         
      for (uint i = 0; i < sizeof(randBuf); i++)
      {
         char buf[2] = { generateHexChar(randBuf[i] >> 4), generateHexChar(randBuf[i] & 0xF) };
         randName.append(buf, 2);
      }

      if (pExt)
         randName += pExt;
   }
            
   void clear(void)
   {
      mRandSeed = 0;
      
      mCmdLineParams.clear();
      
      mWorkDirectory.empty();
      mExecDirectory.empty();
      mIntermediateDirectory.empty();
      mFinalDirectory.empty();
      mVariables.clear();
      
      mMode = cNormal;
      
      mFileGroups.clear();
      mTools.clear();
      mArcGen.clear();
      mTargets.clear();
            
      mUpdateClientExec.empty();
      mUpdateClientArgs.empty();
      
      mUpdateServerExec.empty();
      mUpdateServerArgs.empty();
      
      mPostBuildExec.empty();
      mPostBuildArgs.empty();
      
      mCleanupExec.empty();
      mCleanupArgs.empty();
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
         
      mMultiprocessManager.setMaxProcesses(mCmdLineParams.mMaxProcesses);
      mMultiprocessManager.setFinalizeCallback(finalizeFunc);
      
      mBuildServer.setMaxActiveProcesses(mCmdLineParams.mMaxProcesses);
         
      if (mCmdLineParams.mMode == "quick")
         mMode = cQuick;
      else if (mCmdLineParams.mMode == "normal")
         mMode = cNormal;
      else if (mCmdLineParams.mMode == "final")
         mMode = cFinal;
      else
      {
         gConsoleOutput.error("Invalid processing mode: %s\n", mCmdLineParams.mMode.getPtr());
         return false;
      }

      return true;
   }
         
   static void updateTaskCompletionCallback(const BBuildTaskDesc& desc, int resultCode, uint numMissingFiles, const BDynamicArray<BString>* pFilesWritten)
   {
      pFilesWritten;
      numMissingFiles;
      
      bool* pClientFailedUpdating = reinterpret_cast<bool*>((uint)desc.mCompletionCallbackData);
            
      BDataBuild* pDataBuild = static_cast<BDataBuild*>(desc.mpCompletionCallbackDataPtr);
      pDataBuild;

      if (resultCode != 0)
      {
         gConsoleOutput.error("A client has failed executing the update task!\n");
         
         *pClientFailedUpdating = true;
      }
   }

   bool updateClients(BDynamicArray<BString>& clients)
   {
      if (!mVariables.isDefined("DepotRevision"))
      {
         gConsoleOutput.error("\"DepotRevision\" variable must be defined with the /define option!\n");
         return false;
      }
      
      BBuildServer::BStatus origStatus;
      if (!mBuildServer.getStatus(origStatus))
      {
         gConsoleOutput.error("Failed getting server status\n");
         return false;
      }
      
      if (mUpdateClientExec.isEmpty())
      {
         gConsoleOutput.error("Update client command is not defined in config file!\n");
         return false;
      }

      BBuildTaskDesc taskDesc;
      taskDesc.mAllClients = true;

      taskDesc.mPath = mWorkDirectory;
      taskDesc.mPath.removeTrailingPathSeperator();

      taskDesc.mExec = mUpdateClientExec;
      if (!resolveMacros(taskDesc.mExec, mUpdateClientExec))
         return false;
      
      BString args;
      if (!resolveMacros(args, mUpdateClientArgs))
         return false;
      
      taskDesc.mArgs.format("\"%s\" %s", taskDesc.mExec.getPtr(), args.getPtr());
         
      gConsoleOutput.printf("Submitting update task to all clients: Exec: \"%s\", Args: \"%s\"\n", taskDesc.mExec.getPtr(), taskDesc.mArgs.getPtr());

      bool clientFailedUpdating = false;            

      taskDesc.mpCompletionCallback = updateTaskCompletionCallback;
      taskDesc.mpCompletionCallbackDataPtr = this;
      taskDesc.mCompletionCallbackData = (uint64)&clientFailedUpdating;

      if (!mBuildServer.addTask(taskDesc))
      {
         gConsoleOutput.error("Failed submitting update task!\n");
         return false;  
      }

      if (!mBuildServer.waitForAllTasks())
      {
         gConsoleOutput.error("Failed waiting for update tasks to complete!\n");  
         return false;
      }
      
      if (clientFailedUpdating)
      {
         gConsoleOutput.error("One or more clients failed the auto update task!\n");
         return false;
      }

      gConsoleOutput.printf("Clients successfully updated.\n");

      if (!mBuildServer.submitAutoUpdate())
      {
         gConsoleOutput.error("Failed submitting auto update commands to clients!\n");
         return false;
      }
      
      gConsoleOutput.printf("Pausing\n");

      //Sleep(2000);

      mBuildServer.deinit();

      Sleep(10000);
      
      gConsoleOutput.printf("Attempting to reestablish client connections\n");

      if (!mBuildServer.init(clients))
      {
         gConsoleOutput.error("initBuildServer: Failed reinitializing server!\n");
         return false;
      }
      
      BBuildServer::BStatus newStatus;
      if (!mBuildServer.getStatus(newStatus))
      {
         gConsoleOutput.error("Failed getting server status\n");
         return false;
      }
      
      if (newStatus.mNumHelpers != origStatus.mNumHelpers)
         gConsoleOutput.error("initBuildServer: One or more clients was lost during update (%u original, %u current)!\n", origStatus.mNumHelpers, newStatus.mNumHelpers);
      else
         gConsoleOutput.printf("initBuildServer: All clients successfully updated and reconnected\n");
      
      return true;
   }
      
   bool initBuildServer(void)
   {
      BDynamicArray<BString> clients;
      if (mCmdLineParams.mFindClients)
      {
         if (!BClientAdvertisementManager::discover(clients))
            gConsoleOutput.error("initBuildServer: Unable to get list of active clients (is esfile available?)\n");
      }
      
      clients.append(mCmdLineParams.mClients);
   
      if (clients.getSize())
      {
         if (!mBuildServer.init(clients))
         {
            gConsoleOutput.error("initBuildServer: Failed initializing server, running in standalone mode\n");
         }
         else
         {
            gConsoleOutput.printf("initBuildServer: Initialized\n");
         }
      }
      
      if (mBuildServer.getInitialized())
      {
         BBuildServer::BStatus status;
         if (mBuildServer.getStatus(status))
         {
            gConsoleOutput.printf("Total Helpers: %u\nTotal CPUs: %u\n", status.mNumHelpers, status.mTotalCPUs);
            
            if (mCmdLineParams.mUpdateClients) 
            {
               if (!updateClients(clients))
                  return false;
            }
         }
      }         
      
      return true;
   }
   
   void deinitBuildServer(void)
   {
      mBuildServer.deinit();
   }
   
   bool copyLogFile(FILE* pSrcFile, const char* pDstFilename)
   {
      rewind(pSrcFile);
      
      FILE* pDstFile = fopen(pDstFilename, "w");
      if (pDstFile)
      {
         while (!feof(pSrcFile))
         {
            char buf[2048];
            if (!fgets(buf, sizeof(buf), pSrcFile))
               break;
               
            fputs(buf, pDstFile);
         }
         
         fclose(pDstFile);
      }
      else
      {
         fseek(pSrcFile, 0, SEEK_END);
         
         gConsoleOutput.error("Unable to open file \"%s\" for writing!\n", pDstFilename);
         return false;
      }
      
      fseek(pSrcFile, 0, SEEK_END);
      
      gConsoleOutput.printf("Wrote log file \"%s\".\n", pDstFilename);
            
      return true;
   }
   
   bool executePostBuildCommand(void)
   {
      if ((!mVariables.isDefined("DepotRevision")) || (!mVariables.isDefined("BuildNumber")))
      {
         gConsoleOutput.error("\"DepotRevision\" and \"BuildNumber\" variables must be defined!\n");
         return false;
      }
      
      if ((mPostBuildArgs.isEmpty()) && (mPostBuildExec.isEmpty()))
      {
         gConsoleOutput.warning("-postBuild option specified, but both post build exec and args options defined in config file are empty!\n");
         return true;
      }
            
      BString exec, args;

      if (!resolveMacros(exec, mPostBuildExec))
         return false;

      if (!resolveMacros(args, mPostBuildArgs))
         return false;

      gConsoleOutput.printf("Executing process \"%s\" parameters \"%s\"\n", exec.getPtr(), args.getPtr());         

      BSpawnCommand spawn;
      if (!spawn.set(exec, args))
      {
         gConsoleOutput.error("Unable to execute %s args %s!\n", exec.getPtr(), args.getPtr());
         return false;
      }
      
      if (BConsoleAppHelper::getLogFile())      copyLogFile(BConsoleAppHelper::getLogFile(), "buildLog.txt");
      if (BConsoleAppHelper::getErrorLogFile()) copyLogFile(BConsoleAppHelper::getErrorLogFile(), "buildErrorLog.txt");
            
      //BConsoleAppHelper::closeLogFile();
      //BConsoleAppHelper::closeErrorLogFile();
            
      int result = spawn.run();
            
      //BConsoleAppHelper::openLogFile(NULL, true);
      //BConsoleAppHelper::openErrorLogFile(NULL, true);
      
      gConsoleOutput.printf("Post build command returned status %i\n", result);
      
      if (result != 0)
      {
         gConsoleOutput.error("Failed executing post build command! Tool \"%s\" returned exit status %u.\n", exec.getPtr(), result);
         return false;
      }

      gConsoleOutput.printf("Post build command executed successfully\n");

      return true;
   }
   
   bool executeCleanupCommand(int status)
   {
      if (!mCmdLineParams.mCleanup)
         return true;
         
      if ((!mVariables.isDefined("DepotRevision")) || (!mVariables.isDefined("BuildNumber")))
      {
         gConsoleOutput.error("\"DepotRevision\" and \"BuildNumber\" variables must be defined!\n");
         return false;
      }

      if ((mCleanupExec.isEmpty()) && (mCleanupArgs.isEmpty()))
      {
         gConsoleOutput.warning("-cleanup option specified, but both cleanup exec and args options defined in config file are empty!\n");
         return true;
      }
      
      mVariables.define("Status", BFixedString32(cVarArg, "%u", status));
            
      BString exec, args;

      if (!resolveMacros(exec, mCleanupExec))
         return false;

      if (!resolveMacros(args, mCleanupArgs))
         return false;

      gConsoleOutput.printf("Executing process \"%s\" parameters \"%s\"\n", exec.getPtr(), args.getPtr());         

      BSpawnCommand spawn;
      if (!spawn.set(exec, args))
      {
         gConsoleOutput.error("Unable to execute %s args %s!\n", exec.getPtr(), args.getPtr());
         return false;
      }

      if (BConsoleAppHelper::getLogFile())      copyLogFile(BConsoleAppHelper::getLogFile(), "buildLog.txt");
      if (BConsoleAppHelper::getErrorLogFile()) copyLogFile(BConsoleAppHelper::getErrorLogFile(), "buildErrorLog.txt");
      
      //BConsoleAppHelper::closeLogFile();
      //BConsoleAppHelper::closeErrorLogFile();

      int result = spawn.run();

      //BConsoleAppHelper::openLogFile(NULL, true);
      //BConsoleAppHelper::openErrorLogFile(NULL, true);
      
      gConsoleOutput.printf("Cleanup command returned status %i\n", result);

      if (result != 0)
      {
         gConsoleOutput.error("Failed executing cleanup command! Tool \"%s\" returned exit status %u.\n", exec.getPtr(), result);
         return false;
      }

      gConsoleOutput.printf("Cleanup command executed successfully\n");

      return true;
   }
   
   bool updateServer(void)
   {
      if (!mVariables.isDefined("DepotRevision"))
      {
         gConsoleOutput.error("\"DepotRevision\" variable must be defined with the /define option!\n");
         return false;
      }
      
      if ((mUpdateServerExec.isEmpty()) && (mUpdateServerArgs.isEmpty()))
      {
         gConsoleOutput.warning("-updateServer option specified, but both update server exec and args options defined in config file are empty!\n");
         return true;
      }
      
      BString exec, args;
            
      if (!resolveMacros(exec, mUpdateServerExec))
         return false;
         
      if (!resolveMacros(args, mUpdateServerArgs))
         return false;
         
      gConsoleOutput.printf("Executing process \"%s\" parameters \"%s\"\n", exec.getPtr(), args.getPtr());         
         
      BSpawnCommand spawn;
      if (!spawn.set(exec, args))
      {
         gConsoleOutput.error("Unable to execute %s args %s!\n", exec.getPtr(), args.getPtr());
         return false;
      }
      
      int result = spawn.run();
      
      gConsoleOutput.printf("UpdateServer command returned status %i\n", result);
      
      if (result != 0)
      {
         gConsoleOutput.error("Failed updating server! Tool \"%s\" returned exit status %u.\n", exec.getPtr(), result);
         return false;
      }
      
      gConsoleOutput.printf("Server updated successfully.\n");
      
      return true;
   }
   
   bool createBuildNumberFile(void)
   {
      uint buildNumber = 0;
      
      if (BWin32FileUtils::doesFileExist(mCmdLineParams.mBuildNumberFile))
      {
         BWin32FileStream stream;
         if (!stream.open(mCmdLineParams.mBuildNumberFile))
            gConsoleOutput.warning("Build number file doesn't exist: \"%s\"\n", mCmdLineParams.mBuildNumberFile.getPtr());
         else
         {
            BString line;
            if (!stream.readLine(line))
               gConsoleOutput.error("Unable to read from file: \"%s\"\n", mCmdLineParams.mBuildNumberFile.getPtr());
            else
               buildNumber = static_cast<uint>(Math::Clamp<int64>(line.asInt64(), 0, UINT_MAX));
         }
      }
      else
         gConsoleOutput.warning("Build number file doesn't exist: \"%s\"\n", mCmdLineParams.mBuildNumberFile.getPtr());

      buildNumber++;
      
      BWin32FileStream outStream;
      if (outStream.open(mCmdLineParams.mBuildNumberFile, cSFWritable | cSFSeekable))
      {
         BString str;
         str.format("%u\r\n", buildNumber);
         
         if (!outStream.writeLine(str))
            gConsoleOutput.error("Unable to write build number file: \"%s\"\n", mCmdLineParams.mBuildNumberFile.getPtr());
         else
            gConsoleOutput.printf("Updated build number file: \"%s\"\n", mCmdLineParams.mBuildNumberFile.getPtr());
      }
      else
         gConsoleOutput.error("Unable to write build number file: \"%s\"\n", mCmdLineParams.mBuildNumberFile.getPtr());
      
      gConsoleOutput.printf("Using build number: %u\n", buildNumber);
      
      BString buildNumberStr;
      buildNumberStr.format("%u", buildNumber);
      mVariables.define("BuildNumber", buildNumberStr);
      
      return true;   
   }
   
   bool defineDepotRevision(void)
   {
      SYSTEMTIME localTime;
      GetLocalTime(&localTime);
      
      BString depotRevision;
      depotRevision.format("@%04u/%02u/%02u:%02u:%02u:%02u",
         localTime.wYear,
         localTime.wMonth,
         localTime.wDay,
         localTime.wHour,
         localTime.wMinute,
         localTime.wSecond);
         
      mVariables.define("DepotRevision", depotRevision);
      
      gConsoleOutput.printf("Defined variable \"DepotRevision\" to: %s\n", depotRevision.getPtr());

      return true;
   }
   
   bool writeDepotRevisionFile(void)
   {
      if (!mVariables.isDefined("DepotRevision"))
         return true;
         
      BWin32FileStream outStream;
      if (outStream.open(mCmdLineParams.mDepotRevisionFile, cSFWritable | cSFSeekable))
      {
         BString str;
         str.format("%s\r\n", mVariables.getValue("DepotRevision"));
         
         if (!outStream.writeLine(str))
            gConsoleOutput.error("Unable to write depot revision file: \"%s\"\n", mCmdLineParams.mDepotRevisionFile.getPtr());
         else
            gConsoleOutput.printf("Wrote depot revision file: \"%s\"\n", mCmdLineParams.mDepotRevisionFile.getPtr());
      }
      else
         gConsoleOutput.error("Unable to write depot revision file: \"%s\"\n", mCmdLineParams.mDepotRevisionFile.getPtr());
            
      return true;
   }
   
   bool printCurrentTime(void)
   {
      SYSTEMTIME localTime;
      GetLocalTime(&localTime);

      BString curTime;
      curTime.format("%04u/%02u/%02u %02u:%02u:%02u:%03u",
         localTime.wYear,
         localTime.wMonth,
         localTime.wDay,
         localTime.wHour,
         localTime.wMinute,
         localTime.wSecond,
         localTime.wMilliseconds);

      gConsoleOutput.printf("Current local time is: %s\n", curTime.getPtr());

      return true;
   }
   
   struct BFileGroup
   {
      BFileGroup() { clear(); }
      
      void clear(void)
      {  
         mExtensions.clear();
         mSourceExtensions.clear();
         mTool.empty();
         mHasEmbeddedTag = false;
         mSourceRemoveExtension = false;
      }
      
      BStringArray   mExtensions;
      BStringArray   mSourceExtensions;
      BString        mTool;
      bool           mHasEmbeddedTag;
      bool           mSourceRemoveExtension;
   };
   typedef BDynamicArray<BFileGroup> BFileGroupArray;
   
   struct BTool
   {
      BTool() { clear(); }
      
      void clear(void)
      {
         mName.empty();
         mExec.empty();
         mNormalArgs.empty();
         mFinalArgs.empty();
         mMaxInstances = 1;
         mWritesLogFile = false;
         mWritesErrorLogFile = false;   
      }
      
      BString  mName;
      BString  mExec;
      BString  mNormalArgs;
      BString  mFinalArgs;
      int      mMaxInstances;
      bool     mWritesLogFile;
      bool     mWritesErrorLogFile;
   };
   typedef BDynamicArray<BTool> BToolArray;
         
   struct BArcGen
   {
      BArcGen() { clear(); }
      
      void clear(void)
      {
         mWorkPath.empty();
         mFileListArgs.empty();
         mArchiveArgs.empty();
      }
      
      BString mWorkPath;
      BString mFileListArgs;
      BString mArchiveArgs;
   };
      
   struct BTarget 
   {
      BTarget() { clear(); }
      
      void clear(void)
      {
         mType.empty();
         mName.empty();
         mFileListArgs.empty();
         mArchiveArgs.empty();
      }
      
      BString mType;
      BString mName;
      BString mFileListArgs;
      BString mArchiveArgs;
   };
   typedef BDynamicArray<BTarget> BTargetArray;
   
   BFileGroupArray   mFileGroups;
   BToolArray        mTools;
   BArcGen           mArcGen;
   BTargetArray      mTargets;
   
   BString           mUpdateClientExec;
   BString           mUpdateClientArgs;
   
   BString           mUpdateServerExec;
   BString           mUpdateServerArgs;
   
   BString           mPostBuildExec;
   BString           mPostBuildArgs;
   
   BString           mCleanupExec;
   BString           mCleanupArgs;
   
   enum 
   {
      cCopyToolIndex = 10000
   };
   
   static void parseCommaSepFields(BStringArray& strings, const char* p)
   {
      BString temp(p);
      
      for ( ; ; )
      {
         int i = temp.findLeft(',');
         if (i == -1)
         {
            strings.pushBack(temp);
            break;
         }
         
         BString str(temp);
         str.left(i);
         strings.pushBack(str);
         temp.right(i + 1);
      }
   }
         
   int findTool(const char* pName)
   {
      for (uint i = 0; i < mTools.getSize(); i++)
         if (mTools[i].mName == pName)
            return i;
      
      if (_stricmp(pName, "copy") == 0)
         return cCopyToolIndex;
         
      return -1;
   }
   
   bool readConfig(void)
   {
      mFileGroups.clear();
      mTools.clear();
      mArcGen.clear();
      mArcGen.mWorkPath = mWorkDirectory + "tools\\arcgen\\scripts";
      mTargets.clear();
      mUpdateClientExec.empty();
      mUpdateClientArgs.empty();
      mUpdateServerExec.empty();
      mUpdateServerArgs.empty();
      mPostBuildExec.empty();
      mPostBuildArgs.empty();
      mCleanupExec.empty();
      mCleanupArgs.empty();
   
      BString filename(mCmdLineParams.mConfigFile);
      if (filename.isEmpty())
         filename = mExecDirectory + "config.xml";
      
      BXMLDocument doc;
      if (FAILED(doc.parse(filename.getPtr())))
      {
         gConsoleOutput.error("Failed reading config file: %s\n", filename.getPtr());
         return false;
      }
      
      const BXMLDocument::BNode* pRootNode = doc.getRoot();
      
      // Process any "define" nodes
      for (uint i = 0; i < pRootNode->getNumChildren(); i++)
      {
         const BXMLDocument::BNode* pChildNode = pRootNode->getChild(i);
         if (pChildNode->getName() == "define")
         {
            bool onlyIfUndefined = false;
            pChildNode->getAttributeAsBool("onlyIfUndefined", onlyIfUndefined);
            
            for (uint j = 0; j < pChildNode->getNumAttributes(); j++)
            {
               const BXMLDocument::BAttribute& attrib = pChildNode->getAttribute(j);
               
               BString name(attrib.getName());
               BString value(attrib.getText());
               
               if (name == "onlyIfUndefined")
                  continue;
                  
               if (onlyIfUndefined)
               {
                  if (mVariables.isDefined(name))
                     continue;
               }
               
               if (!resolveMacros(name, BString(name)))
                  return false;

               BStringArray valueArray;
               if (!resolveText(value, valueArray, 0, false))
                  return false;

               if (valueArray.getSize() > 1)
                  gConsoleOutput.warning("Define \"%s\" has multiple values - only using first value!\n", name.getPtr());

               name.toLower();
               
               if (valueArray.isEmpty())
                  mVariables.define(name);
               else
                  mVariables.define(name, valueArray[0]);
            }
         }
      }
                  
      const BXMLDocument::BNode* pUpdateNode = pRootNode->findChild("updateClient");
      if (pUpdateNode)
      {
         pUpdateNode->getChildAsString("exec", mUpdateClientExec);
         pUpdateNode->getChildAsString("args", mUpdateClientArgs);
      }
      
      pUpdateNode = pRootNode->findChild("updateServer");
      if (pUpdateNode)
      {
         pUpdateNode->getChildAsString("exec", mUpdateServerExec);
         pUpdateNode->getChildAsString("args", mUpdateServerArgs);
      }
      
      const BXMLDocument::BNode* pPostBuildNode = pRootNode->findChild("postBuild");
      if (pPostBuildNode)
      {
         pPostBuildNode->getChildAsString("exec", mPostBuildExec);
         pPostBuildNode->getChildAsString("args", mPostBuildArgs);
      }
      
      const BXMLDocument::BNode* pCleanupNode = pRootNode->findChild("cleanup");
      if (pCleanupNode)
      {
         pCleanupNode->getChildAsString("exec", mCleanupExec);
         pCleanupNode->getChildAsString("args", mCleanupArgs);
      }
      
      const BXMLDocument::BNode* pFileGroupsNode = pRootNode->findChild("fileGroups");
      if (pFileGroupsNode)
      {
         for (uint i = 0; i < pFileGroupsNode->getNumChildren(); i++)
         {
            const BXMLDocument::BNode* pNode = pFileGroupsNode->getChild(i);
            
            BFileGroup fileGroup;
            
            if (pNode->getName() == "fileGroup")
            {
               BString extensions;
               pNode->getAttributeAsString("extensions", extensions);
               parseCommaSepFields(fileGroup.mExtensions, extensions);
               
               pNode->getAttributeAsBool("hasEmbeddedTag", fileGroup.mHasEmbeddedTag);
               pNode->getAttributeAsBool("sourceRemoveExtension", fileGroup.mSourceRemoveExtension);
               
               BString sourceExtensions;
               pNode->getAttributeAsString("sourceExtensions", sourceExtensions);
               parseCommaSepFields(fileGroup.mSourceExtensions, sourceExtensions);
               
               pNode->getAttributeAsString("tool", fileGroup.mTool);
            }
            
            mFileGroups.pushBack(fileGroup);
         }
      }
      
      const BXMLDocument::BNode* pToolNode = pRootNode->findChild("tools");
      if (pToolNode)
      {
         for (uint i = 0; i < pToolNode->getNumChildren(); i++)
         {
            const BXMLDocument::BNode* pNode = pToolNode->getChild(i);
            
            BTool tool;
            
            if (pNode->getName() == "tool")
            {
               pNode->getAttributeAsString("name", tool.mName);
               pNode->getAttributeAsInt("maxInstances", tool.mMaxInstances);
               pNode->getAttributeAsBool("writesLogFile", tool.mWritesLogFile);
               pNode->getAttributeAsBool("writesErrorLogFile", tool.mWritesErrorLogFile);
                              
               pNode->getChildAsString("exec", tool.mExec);
               pNode->getChildAsString("normalArgs", tool.mNormalArgs);
               pNode->getChildAsString("finalArgs", tool.mFinalArgs);
            }
            
            mTools.pushBack(tool);
         }
      }
      
      const BXMLDocument::BNode* pArcGenNode = pRootNode->findChild("arcGen");
      if (pArcGenNode)
      {
         pArcGenNode->getChildAsString("workPath", mArcGen.mWorkPath);
         pArcGenNode->getChildAsString("fileListArgs", mArcGen.mFileListArgs);
         pArcGenNode->getChildAsString("archiveArgs", mArcGen.mArchiveArgs);         
      }
      
      const BXMLDocument::BNode* pTargetNode = pRootNode->findChild("targets");
      if (pTargetNode)
      {
         for (uint i = 0; i < pTargetNode->getNumChildren(); i++)
         {
            const BXMLDocument::BNode* pNode = pTargetNode->getChild(i);
            
            BTarget target;
            
            if (pNode->getName() == "target")
            {
               pNode->getAttributeAsString("type", target.mType);
               pNode->getAttributeAsString("name", target.mName);
               pNode->getChildAsString("fileListArgs", target.mFileListArgs);
               pNode->getChildAsString("archiveArgs", target.mArchiveArgs);
            }
            
            mTargets.pushBack(target);
         }
      }
      
      gConsoleOutput.printf("FileGroups: %i, Tools: %i, Targets: %i\n", mFileGroups.getSize(), mTools.getSize(), mTargets.getSize());
      
      for (uint i = 0; i < mFileGroups.getSize(); i++)
      {
         if (findTool(mFileGroups[i].mTool) == -1)
         {
            gConsoleOutput.error("Unable to find tool \"%s\" for file group %u!\n", mFileGroups[i].mTool.getPtr(), i);
            return false;
         }
      }
      
      return true;
   }
      
   bool findDirectories(void)
   {
      char buf[MAX_PATH];
      if (!GetModuleFileNameA(GetModuleHandle(NULL), buf, sizeof(buf)))
         return false;
      
      BString path, name;
      strPathSplit(BString(buf), path, name);
      mExecDirectory = path;
      
      if (!mCmdLineParams.mWorkPath.isEmpty())
      {
         mWorkDirectory = mCmdLineParams.mWorkPath;
         mWorkDirectory.standardizePath();
         mWorkDirectory.removeTrailingPathSeperator();
         mWorkDirectory += "\\";
      }
      else
      {
         mWorkDirectory = buf;
         mWorkDirectory.toLower();

         int i = mWorkDirectory.findRight("\\tools\\databuild");
         if (i == -1)
            i = mWorkDirectory.findRight("\\tools\\databuild");
         if (i == -1)
            return false;

         mWorkDirectory.crop(0, i);
      }         
      
      if (!mCmdLineParams.mIntermediatePath.isEmpty())        
      {
         mIntermediateDirectory = mCmdLineParams.mIntermediatePath;
      }
      else
      {
         if (!strPathMakeAbsolute(mWorkDirectory + "..\\intermediate", mIntermediateDirectory))
            return false;
      }
      
      if (!mCmdLineParams.mFinalPath.isEmpty())
      {
         mFinalDirectory = mCmdLineParams.mFinalPath;
      }
      else
      {
         if (!strPathMakeAbsolute(mWorkDirectory + "..\\final", mFinalDirectory))
            return false;
      }
      
      mWorkDirectory.standardizePath();
      mIntermediateDirectory.standardizePath();
      mFinalDirectory.standardizePath();
      
      strPathAddBackSlash(mWorkDirectory);
      strPathAddBackSlash(mIntermediateDirectory);
      strPathAddBackSlash(mFinalDirectory);
      
      gConsoleOutput.printf("        Work directory: %s\n", mWorkDirectory.getPtr());
      gConsoleOutput.printf("Intermediate directory: %s\n", mIntermediateDirectory.getPtr());
      gConsoleOutput.printf("       Final directory: %s\n", mFinalDirectory.getPtr());
      
      return true;
   }
   
   bool massageFilename(BString& filename, bool convertToAbsolute = true, bool resolveMacros = true)
   {
      filename.trimLeft();
      filename.trimRight();

      BString newFilename(filename);
      if (resolveMacros)
      {
         if (!this->resolveMacros(newFilename, filename))
            return false;
      }

      if ((convertToAbsolute) && (newFilename.length()))
      {
         if (!strPathIsAbsolute(newFilename))
         {
            const char* pInputPath = mVariables.getValue("inputpath");

            if (pInputPath) 
            {
               char c = newFilename.getChar(0);
               if ((c == '/') || (c == '\\'))
                  newFilename = BString(pInputPath) + newFilename;
               else
                  newFilename = BString(pInputPath) + "\\" + newFilename;
            }
         } 

         CHAR fullpath[_MAX_PATH];
         LPTSTR pFilePart = NULL;

         if (GetFullPathName(newFilename.getPtr(), _MAX_PATH, fullpath, &pFilePart))
            newFilename.set(fullpath);
         else
            gConsoleOutput.warning("Couldn't resolve path: %s\n", newFilename.getPtr());
      }            

      newFilename.standardizePath();

      filename.swap(newFilename);

      return true;
   }

   bool initDerivedVariables(void)
   {   
      if (mVariables.isDefined("scenariofilename"))
      {
         BString scenarioFilename(mVariables.getValue("scenariofilename"));
         if (!massageFilename(scenarioFilename, true))
            return false;
         
         if (!BWin32FileUtils::doesFileExist(scenarioFilename))
         {
            gConsoleOutput.error("Scenario file does not exist: %s\n", scenarioFilename.getPtr());
            return false;
         }

         mVariables.define("scenariofilename", scenarioFilename);
                  
         BString path;
         BString name;
         strPathSplit(scenarioFilename, path, name);

         path.removeTrailingPathSeperator();
         mVariables.define("scenariopath", path);

         name.removeExtension();
         mVariables.define("scenarioname", name);
      }
      
      return true;
   }
   
   static bool unquote(BString& str)
   {
      if (!str.length())
         return true;

      if (str.getChar(0) == '"') 
      {
         if (str.length() < 2)
            return false;

         if (str.getChar(str.length() - 1) != '"')
         {
            gConsoleOutput.error("Missing end quote: %s\n", str.getPtr());
            return false;
         }

         str.substring(1, str.length() - 1);
      }

      return true;
   }
   
   bool processDefine(const char* pDef, bool disableFileListSupport)
   {
      BString define(pDef);
      define.trimLeft();
      define.trimRight();
      if (define.isEmpty())
         return true;

      int i = define.findLeft("=");
      
      BString name, value;
      if (i >= 0)
      {
         name = define;
         name.left(i);

         value = define;
         value.right(i + 1);
      }     
      else
      {
         name = define;
      }
      
      if (value.length())
      {
         // In practice this doesn't seem to work because the quote char is filtered out before we get the arguments!
         if (value.getChar(0) == '"')
            disableFileListSupport = true;
      }
      
      if ((!unquote(name)) || (!unquote(value)))
         return false;
                  
      if (!resolveMacros(name, BString(name)))
         return false;
         
      BStringArray valueArray;
      if (!resolveText(value, valueArray, 0, disableFileListSupport))
         return false;
      
      if (valueArray.getSize() > 1)
         gConsoleOutput.warning("Define \"%s\" has multiple values - only using first value!\n", name.getPtr());
            
      name.toLower();
      
      if (valueArray.isEmpty())
         mVariables.define(name);
      else
         mVariables.define(name, valueArray[0]);
         
      return true;
   }
      
   bool initVariables(void)
   {
      mVariables.define("BuildNumber", "0");
      
      BString workPath(mWorkDirectory);
      workPath.removeTrailingPathSeperator();
      mVariables.define("workpath", workPath);
            
      if (!mVariables.isDefined("temppath"))
      {
#if 0 
         char buf[MAX_PATH];     
         if (GetTempPath(MAX_PATH, buf))
         {
            BString tempPath(buf);
            tempPath.removeTrailingPathSeperator();
            mVariables.define("temppath", tempPath);
         }
#endif         
         const char* pTempPath = "c:\\temp";
         mVariables.define("temppath", pTempPath);
         _mkdir(pTempPath);
      }
            
      BString depGenFilename(mWorkDirectory);
      depGenFilename += DEPGEN_FILENAME;
      mVariables.define("depgen", depGenFilename);
      
      {
         BString path, name;
         strPathSplit(depGenFilename, path, name);
         path.removeTrailingPathSeperator();
         mVariables.define("depgenpath", path);
      }
      
      BString ecfArcFilename(mWorkDirectory);
      ecfArcFilename += ECFARC_FILENAME;
      mVariables.define("ecfarc", ecfArcFilename);
      
      {
         BString path, name;
         strPathSplit(ecfArcFilename, path, name);
         path.removeTrailingPathSeperator();
         mVariables.define("ecfarcpath", path);
      }
      
      BString arcGenFilename(mWorkDirectory);
      arcGenFilename += ARCGEN_FILENAME;
      mVariables.define("arcgen", arcGenFilename);

      {
         BString path, name;
         strPathSplit(arcGenFilename, path, name);
         path.removeTrailingPathSeperator();
         mVariables.define("arcgenpath", path);
      }
                                 
      BString path(mExecDirectory);
      path.removeTrailingPathSeperator();
      mVariables.define("execpath", path);
      
      path = mIntermediateDirectory;
      path.removeTrailingPathSeperator();
      mVariables.define("intermediatepath", path);
      
      path = mFinalDirectory;
      path.removeTrailingPathSeperator();
      mVariables.define("finalpath", path);
      
      mVariables.define("arcGenDefineFilename", mCmdLineParams.mArcGenDefineFile);
      
      for (uint defineIndex = 0; defineIndex < mCmdLineParams.mDefines.getSize(); defineIndex++)
      {
         if (!processDefine(mCmdLineParams.mDefines[defineIndex], false))
            return false;
      }
      
      for (uint defineIndex = 0; defineIndex < mCmdLineParams.mRawDefines.getSize(); defineIndex++)
      {
         if (!processDefine(mCmdLineParams.mRawDefines[defineIndex], true))
            return false;
      }
        
      for (uint i = 0; i < mCmdLineParams.mDefineFiles.getSize(); i++)
      {
         gConsoleOutput.printf("Reading define file: %s\n", mCmdLineParams.mDefineFiles[i].getPtr());
         
         BStringArray defines;
         if (!BWin32FileUtils::readStringFile(mCmdLineParams.mDefineFiles[i], defines))
         {
            gConsoleOutput.error("Failed reading define file: %s\n", mCmdLineParams.mDefineFiles[i].getPtr());
            return false;
         }
         
         for (uint j = 0; j < defines.getSize(); j++)
         {
            if (!processDefine(defines[j], false))
               return false;
         }
      }
            
      if (!initDerivedVariables())
         return false;
      
      checkPath("inputpath", mVariables.getValue("workpath"));
      checkPath("outputpath", mVariables.getValue("workpath"));
                  
      gConsoleOutput.printf("Defined variables:\n");
      const BVariableManager::BVariableHashMap& variables = mVariables.getTable();
      for (BVariableManager::BVariableHashMap::const_iterator it = variables.begin(); it != variables.end(); ++it)
         gConsoleOutput.printf("  \"%s\" = \"%s\"\n", it->first.getPtr(), it->second.getPtr());
         
      return true;
   }
   
   bool cleanDirectory(const char* pPath, const char* pMask = "*")
   {
      gConsoleOutput.printf("Finding files under path: \"%s\" with mask: \"%s\"\n", pPath, pMask);
      
      BFindFiles findFiles(pPath, pMask, BFindFiles::FIND_FILES_WANT_FILES | BFindFiles::FIND_FILES_RECURSE_SUBDIRS);

      if (!findFiles.success())
         return false;

      for (uint i = 0; i < findFiles.numFiles(); i++)
      {
         gConsoleOutput.printf("Deleting file: %s\n", findFiles[i].fullFilename().getPtr());

         remove(findFiles[i].fullFilename().getPtr());
      }
      
      return true;
   }
   
   bool cleanCache(void)
   {
      BString dataBuildCachePath(mExecDirectory + "cache");
      
      if (!resolveMacros(dataBuildCachePath, BString(dataBuildCachePath)))
      {
         gConsoleOutput.error("Failed resolving macros: \"%s\"\n", dataBuildCachePath.getPtr());
         return false;
      }
            
      gConsoleOutput.printf("Cleaning databuild cache directory: \"%s\"\n", dataBuildCachePath.getPtr());
      if (!cleanDirectory(dataBuildCachePath))
      {
         gConsoleOutput.error("Failed finding files under databuild cache directory\n");
         return false;
      }
      
      BString arcGenCachePath(mArcGen.mWorkPath + "\\cache");
      
      if (!resolveMacros(arcGenCachePath, BString(arcGenCachePath)))
      {
         gConsoleOutput.error("Failed resolving macros: \"%s\"\n", arcGenCachePath.getPtr());
         return false;
      }
      
      gConsoleOutput.printf("Cleaning arcgen cache directory: \"%s\"\n", arcGenCachePath.getPtr());
      if (!cleanDirectory(arcGenCachePath))
      {
         gConsoleOutput.error("Failed finding files under arcgen cache directory\n");
         return false;
      }
                                         
      return true;
   }
   
   void checkPath(const char* pName, const char* pDefValue)
   {
      BString path;
      if (!mVariables.isDefined(pName))
      {
         if (!pDefValue)
            return;
         path = pDefValue;
      }
      else
         path = mVariables.getValue(pName);
               
      massageFilename(path, true);
      path.removeTrailingPathSeperator();
      mVariables.define(pName, path);
   }      
           
   bool resolveMacros(BString& output, const BString& input)
   {
      output = input;
      
      for ( ; ; )
      {
         int i = output.findLeft("$(");
         if (i < 0)
         {
            i = output.findLeft("!(");
            if (i < 0)
               break;
         }
            
         int j = output.findLeft(')', i + 1);
         if (j < i)
         {
            gConsoleOutput.error("Macro substitution failed while processing string: %s\n", input.getPtr());
            return false; 
         }
         
         BString name(output);
         name.substring(i + 2, j);
         name.toLower();
         
         if (!mVariables.isDefined(name))
         {  
            gConsoleOutput.error("Undefined macro \"%s\" used in string: %s\n", name.getPtr(), input.getPtr());
            return false;
         }
         
         const char* pValue = mVariables.getValue(name);
         
         BString start(output);
         start.left(i);
         
         BString end(output);            
         end.right(j + 1);
         
         BString middle(pValue ? pValue : "");
         if ((middle.findLeft("$(") != -1) || (middle.findLeft("!(") != -1))
         {
            gConsoleOutput.error("Invalid macro value: %s\n", middle.getPtr());
            return false;
         }
         
         output = start + middle + end;
      }
      
      return true;
   }
   
   bool resolveText(const BString& text, BStringArray& strings, uint recursionLevel = 0, bool disableFileListSupport = false)
   {
      if (recursionLevel > 10)
      {
         gConsoleOutput.error("Max recursion level exceeded!");
         return false;
      }
                  
      BString resolvedText;
      if (!resolveMacros(resolvedText, text))
         return false;
      resolvedText.trimLeft();
      resolvedText.trimRight();
      
      if (resolvedText.isEmpty())
         return true;
         
      if ((resolvedText.getChar(0) == '@') && (!disableFileListSupport))
      {
         BString filename(resolvedText);
         filename.right(1);
         
         gConsoleOutput.printf("Reading file: %s\n", filename.getPtr());
         
         BStringArray fileStrings;
         if (!BWin32FileUtils::readStringFile(filename, fileStrings))
         {
            gConsoleOutput.error("Failed reading file: %s\n", filename.getPtr());
            return false;
         }
         
         for (uint i = 0; i < fileStrings.getSize(); i++)
         {
            BString resolvedFileText;
            resolveMacros(resolvedFileText, fileStrings[i]);
            resolvedFileText.trimLeft();
            resolvedFileText.trimRight();
            
            if (resolvedFileText.isEmpty())
               continue;
               
            if (resolvedFileText.getChar(0) == '@')
            {
               if (!resolveText(resolvedFileText, strings, recursionLevel + 1))
                  return false;
            }
            else
            {
               strings.pushBack(resolvedFileText);  
            }
         }
      }
      else
      {
         strings.pushBack(resolvedText);
      }
      
      return true;
   }
   
   int runArcGen(const char* pArgs)
   {
      char curDir[MAX_PATH];
      GetCurrentDirectory(MAX_PATH, curDir);
      
      BSpawnCommand spawn;
      BString args(BString("\"") + mVariables.getValue("arcgen") + BString("\" ") + BString(pArgs));
      
      gConsoleOutput.printf("Running arcgen: %s\n", args.getPtr());
      
      if (!spawn.set(args))
         return -1;
      
      BString workPath;
      if (!resolveMacros(workPath, mArcGen.mWorkPath))
         return -1;
         
      SetCurrentDirectory(workPath);
          
      int status = spawn.run();
      gConsoleOutput.printf("Arcgen command returned status %i\n", status);
            
      SetCurrentDirectory(curDir);
      return status;
   }
   
   int findTarget(const char* pName)
   {
      for (uint i = 0; i < mTargets.getSize(); i++)
         if (mTargets[i].mName == pName)
            return i;
      
      if (_stricmp(pName, "scenario") == 0)
      {
         for (uint i = 0; i < mTargets.getSize(); i++)
            if (mTargets[i].mType == "scenario")
               return i;      
      }
      
      return -1;
   }
   
   struct BGroup
   {
      BGroup() { clear(); }
      
      void clear(void)
      {
         mSourceFiles.clear();
         mLeafFileIndices.clear();
         mToolIndex = -1;
         mIsDirty = false;
      }
      
      BStringArray         mSourceFiles;
      BDynamicArray<uint>  mLeafFileIndices;
      int                  mToolIndex;
      bool                 mIsDirty;
   };
   
   typedef BDynamicArray<BGroup> BGroupArray;
   BGroupArray mLeafFileGroups;
   
   BStringArray mLeafFiles;
   BStringArray mIntermediateFiles;
   BStringIndexMap mLeafFileMap;
   
   bool createFileGroups(void)
   {
      gConsoleOutput.printf("Finding file groups for %u files\n", mLeafFiles.getSize());
      
      mLeafFileMap.clear();
      mIntermediateFiles.clear();
      for (uint i = 0; i < mLeafFiles.getSize(); i++)
      {
         mLeafFiles[i].standardizePath();
         mLeafFileMap.insert(mLeafFiles[i], i);
         
         BString filename(mLeafFiles[i]);
         int index = filename.findLeft(mWorkDirectory);
         if (index != 0)
         {
            gConsoleOutput.error("Leaf filename is not relative to work directory: %s\n", mLeafFiles[i].getPtr());
            return false;
         }
         
         filename.right(mWorkDirectory.length());
         filename = mIntermediateDirectory + filename;
         mIntermediateFiles.pushBack(filename);
      }
      
      BDynamicArray<bool> removedFlag(mLeafFiles.getSize());
      
      for (uint fileGroupIndex = 0; fileGroupIndex < mFileGroups.getSize(); fileGroupIndex++)
      {
         const BFileGroup& fileGroup = mFileGroups[fileGroupIndex];
         if (fileGroup.mExtensions.isEmpty())
            continue;
         
         for (uint leafFileIndex = 0; leafFileIndex < mLeafFiles.getSize(); leafFileIndex++)
         {
            if (removedFlag[leafFileIndex])
               continue;
            
            const BString& leafFile = mLeafFiles[leafFileIndex];
            
            BString ext;
            strPathGetExtension(leafFile, ext);
            if ((!ext.isEmpty()) && (ext.getChar(0) == '.'))
               ext.right(1);
            
            if (ext != fileGroup.mExtensions[0])
               continue;
               
            BDynamicArray<int> leafFileIndices;
            leafFileIndices.pushBack(leafFileIndex);
           
            for (uint extIndex = 1; extIndex < fileGroup.mExtensions.getSize(); extIndex++)
            {
               BString filename(leafFile);
               
               filename.removeExtension();
               filename += ".";
               filename += fileGroup.mExtensions[extIndex];
               
               BStringIndexMap::const_iterator it = mLeafFileMap.find(filename);
               if (it == mLeafFileMap.end())
                  break;
               
               if (removedFlag[it->second])
                  break;
               
               leafFileIndices.pushBack(it->second);
            }
            
            if (leafFileIndices.size() == fileGroup.mExtensions.getSize())
            {
               BGroup group;
               group.mToolIndex = findTool(fileGroup.mTool);
               group.mLeafFileIndices = leafFileIndices;
                              
               BString sourceFilename(leafFile);
               sourceFilename.removeExtension();
               
               if (!fileGroup.mSourceRemoveExtension)
               {
                  BString tempSourceFilename;
                  
                  uint sourceExtIndex;
                  for (sourceExtIndex = 0; sourceExtIndex < fileGroup.mSourceExtensions.getSize(); sourceExtIndex++)
                  {
                     tempSourceFilename = sourceFilename;
                     tempSourceFilename += ".";
                     tempSourceFilename += fileGroup.mSourceExtensions[sourceExtIndex];
                     
                     if (BWin32FileUtils::doesFileExist(tempSourceFilename.getPtr()))
                     {
                        sourceFilename = tempSourceFilename;
                        break;                     
                     }
                  }
                  
                  if (sourceExtIndex == fileGroup.mSourceExtensions.getSize())
                  {
                     gConsoleOutput.error("Unable to find source file for leaf file: %s\n", leafFile.getPtr());
                     
                     sourceFilename += ".";
                     sourceFilename += fileGroup.mSourceExtensions[0];
                  }
               }
               
               group.mSourceFiles.pushBack(sourceFilename);
               
               mLeafFileGroups.pushBack(group);
                              
               for (uint i = 0; i < leafFileIndices.getSize(); i++)
                  removedFlag[leafFileIndices[i]] = true;  
            }
         }
      }   
      
      gConsoleOutput.printf("File groups found: %u\n", mLeafFileGroups.getSize());
      
      uint numUngroupedFiles = 0;
      
      for (uint i = 0; i < mLeafFiles.getSize(); i++)
      {
         if (removedFlag[i])
            continue;
            
         BGroup group;
         group.mToolIndex = cCopyToolIndex;
         group.mLeafFileIndices.pushBack(i);
         group.mSourceFiles.pushBack(mLeafFiles[i]);
       
         mLeafFileGroups.pushBack(group);
         
         numUngroupedFiles++;
      }
      
      gConsoleOutput.printf("Ungrouped files: %u\n", numUngroupedFiles);
      
      return true;
   }
   
   bool findFilesToUpdate(void)
   {
      gConsoleOutput.printf("Finding dirty files\n");
      
      uint numDirtyGroups = 0;
      if (mCmdLineParams.mFull)
      {
         for (uint i = 0; i < mLeafFileGroups.getSize(); i++)
            mLeafFileGroups[i].mIsDirty = true;
            
         numDirtyGroups = mLeafFileGroups.getSize();
      }
      else
      {
         for (uint i = 0; i < mLeafFileGroups.getSize(); i++)
         {
            BGroup& group = mLeafFileGroups[i];
            
            uint64 sourceTime = 0;
            
            uint sourceFileIndex;
            for (sourceFileIndex = 0; sourceFileIndex < group.mSourceFiles.getSize(); sourceFileIndex++)
            {
               WIN32_FILE_ATTRIBUTE_DATA attr;
               if (GetFileAttributesEx(group.mSourceFiles[sourceFileIndex].getPtr(), GetFileExInfoStandard, &attr))
                  sourceTime = Math::Max(sourceTime, Utils::FileTimeToUInt64(attr.ftLastWriteTime));
            }
            
            uint leafFileIndex;            
            for (leafFileIndex = 0; leafFileIndex < group.mLeafFileIndices.getSize(); leafFileIndex++)
            {
               WIN32_FILE_ATTRIBUTE_DATA attr;
               if (GetFileAttributesEx(mLeafFiles[group.mLeafFileIndices[leafFileIndex]], GetFileExInfoStandard, &attr))
                  sourceTime = Math::Max(sourceTime, Utils::FileTimeToUInt64(attr.ftLastWriteTime));
            }
            
            uint64 destTime = UINT64_MAX;
                        
            for (leafFileIndex = 0; leafFileIndex < group.mLeafFileIndices.getSize(); leafFileIndex++)
            {
               WIN32_FILE_ATTRIBUTE_DATA attr;
               if (GetFileAttributesEx(mIntermediateFiles[group.mLeafFileIndices[leafFileIndex]], GetFileExInfoStandard, &attr))
                  destTime = Math::Min(destTime, Utils::FileTimeToUInt64(attr.ftLastWriteTime));
               else
                  destTime = 0;
            }
            
            if (sourceTime > destTime)
            {
               group.mIsDirty = true;
               numDirtyGroups++;  
            }
         }
      }
      
      gConsoleOutput.printf("Found %u dirty file groups\n", numDirtyGroups);
      
      return true;
   }
   
   void copyLeafFileGroup(const BGroup& group, uint& numFileCopiesFailed)
   {
      numFileCopiesFailed = 0;
      
      for (uint fileIndex = 0; fileIndex < group.mLeafFileIndices.getSize(); fileIndex++)
      {
         uint leafFileIndex = group.mLeafFileIndices[fileIndex];
         const BString& leafFilename = mLeafFiles[leafFileIndex];
         const BString& dstFilename = mIntermediateFiles[leafFileIndex];

         gConsoleOutput.printf("Copying \"%s\" to \"%s\"\n", leafFilename.getPtr(), dstFilename.getPtr());

         bool success = BWin32FileUtils::copyFile(leafFilename, dstFilename, true);
         if (!success)
         {
            remove(dstFilename);

            gConsoleOutput.error("File copy failed: %s\n", dstFilename.getPtr());

            numFileCopiesFailed++;
         }
      }
   }
            
   struct BFinalizeLeafFileGroupData
   {
      uint     mLeafFileGroupIndex;
      uint     mToolIndex;
      BString  mArgs;
      BString  mLogFilename;
      BString  mErrorLogFilename;
   };
   
   void appendLogFile(const char* pLogFilename, FILE* pFile, int status, const char* pArgs, bool alwaysPrintSummary)
   { 
      FILE* pSrcFile;
      if (fopen_s(&pSrcFile, pLogFilename, "r"))
      {
         gConsoleOutput.error("Unable to open log file: %s\n", pLogFilename);
         return;
      }

      gConsoleOutput.printf("Reading log file: %s\n", pLogFilename);
      
      bool printedSummary = false;         
      if (alwaysPrintSummary)
      {
         fprintf(pFile, "------------- Tool status: %i, args: %s\n", status, pArgs);
         printedSummary = true;
      }
      
      for ( ; ; )
      {
         char buf[8192];
         if (!fgets(buf, sizeof(buf), pSrcFile))
            break;
            
         if ((strcmp(buf, "\n") == 0) || (strcmp(buf, "\n\r") == 0) || (strcmp(buf, "\r\n") == 0))
            continue;

         if (!printedSummary)
         {
            fprintf(pFile, "------------- Tool status: %i, args: %s\n", status, pArgs);
            printedSummary = true;
         }
         
         fprintf(pFile, "%s", buf);
      }
      
      fclose(pSrcFile);
      
      if (printedSummary)
         fprintf(pFile, "-------------\n");
   }
   
   BCriticalSection mLogFileMutex;
   BCriticalSection mErrorLogFileMutex;
      
   bool finalizeLeafFileGroup(int status, uint leafFileGroupIndex, uint toolIndex, const char* pLogFilename, const char* pErrorLogFilename, const char* pArgs)
   {
      const BGroup& group = mLeafFileGroups[leafFileGroupIndex];
            
      if (BConsoleAppHelper::getLogFile())
      {
         BScopedCriticalSection lock(mLogFileMutex);
         appendLogFile(pLogFilename, BConsoleAppHelper::getLogFile(), status, pArgs, true);
      }
               
      if (BConsoleAppHelper::getErrorLogFile())
      {
         BScopedCriticalSection lock(mErrorLogFileMutex);
         appendLogFile(pErrorLogFilename, BConsoleAppHelper::getErrorLogFile(), status, pArgs, false);
      }
      
      remove(pLogFilename);
      remove(pErrorLogFilename);
      
      if (status != 0)
      {
         gConsoleOutput.error("Tool %s exited with status %i! Attempting to copy leaf files from work directory.\n", mTools[toolIndex].mName.getPtr(), status);

         uint numFileCopiesFailed = 0;
         copyLeafFileGroup(group, numFileCopiesFailed);
         if (numFileCopiesFailed == 0)
            gConsoleOutput.warning("Successfully copied leaf files from work to intermediate directory\n");
         else
            gConsoleOutput.error("Failed copying all leaf files from work to intermediate directory\n");

         return true;
      }

      uint numMissingOutputFiles = 0;
      for (uint fileIndex = 0; fileIndex < group.mLeafFileIndices.getSize(); fileIndex++)
      {
         uint leafFileIndex = group.mLeafFileIndices[fileIndex];
         const BString& intermediateFilename = mIntermediateFiles[leafFileIndex];

         if (!BWin32FileUtils::doesFileExist(intermediateFilename))
         {
            gConsoleOutput.error("Missing output file: %s\n", intermediateFilename.getPtr());
            numMissingOutputFiles++;
         }
      }

      if (numMissingOutputFiles)
      {
         gConsoleOutput.error("Tool %s did not write all output files!\n", mTools[toolIndex].mName.getPtr());
         return false;
      }
      else
      {
         gConsoleOutput.printf("Tool %s source file \"%s\" executed successfully\n", mTools[toolIndex].mName.getPtr(), group.mSourceFiles[0].getPtr());
      }

      return true;  
   }
   
   bool finalizeLeafFileGroup(DWORD exitCode, const BFinalizeLeafFileGroupData* pData)
   {
      bool status = finalizeLeafFileGroup((int)exitCode, pData->mLeafFileGroupIndex, pData->mToolIndex, pData->mLogFilename, pData->mErrorLogFilename, pData->mArgs);
      
      delete pData;
      
      return status;
   }
   
   static bool finalizeFunc(DWORD exitCode, void* pUserPtr, uint64 userData)
   {
      static_cast<BDataBuild*>(pUserPtr)->finalizeLeafFileGroup(exitCode, reinterpret_cast<BFinalizeLeafFileGroupData*>((uint)userData));
      return true;
   }
   
   static void taskCompletionCallback(const BBuildTaskDesc& desc, int resultCode, uint numMissingFiles, const BDynamicArray<BString>* pFilesWritten)
   {
      pFilesWritten;
      numMissingFiles;
      
      BDataBuild* pDataBuild = static_cast<BDataBuild*>(desc.mpCompletionCallbackDataPtr);
      const BFinalizeLeafFileGroupData* pData = reinterpret_cast<BFinalizeLeafFileGroupData*>((uint)desc.mCompletionCallbackData);
            
      pDataBuild->finalizeLeafFileGroup(resultCode, pData->mLeafFileGroupIndex, pData->mToolIndex, pData->mLogFilename, pData->mErrorLogFilename, pData->mArgs);
            
      delete pData;
   }
   
   static char generateHexChar(uint i)
   {
      BDEBUG_ASSERT(i < 16);
      
      if (i < 10)
         return (char)('0' + i);
      else
         return (char)('A' + (i - 10));
   }
         
   bool updateIntermediateFiles(void)
   {
      BDynamicArray<IntArray> leafFileGroupsToUpdate;
      leafFileGroupsToUpdate.resize(mTools.getSize());
      
      for (uint leafFileGroupIndex = 0; leafFileGroupIndex < mLeafFileGroups.getSize(); leafFileGroupIndex++)
      {
         const BGroup& group = mLeafFileGroups[leafFileGroupIndex];
         if (!group.mIsDirty)
            continue;
            
         uint numMissingSourceFiles = 0;
         bool allSourceFilesExist = true;

         for (uint sourceFileIndex = 0; sourceFileIndex < group.mSourceFiles.getSize(); sourceFileIndex++)
         {
            const BString& sourceFilename = group.mSourceFiles[sourceFileIndex];

            if (!BWin32FileUtils::doesFileExist(sourceFilename))
            {
               allSourceFilesExist = false;
               numMissingSourceFiles++;
               gConsoleOutput.error("Missing source file: %s\n", sourceFilename.getPtr());
            }
         }
            
         uint numMissingLeafFiles = 0;
         bool allLeafFilesExist = true;
         
         for (uint fileIndex = 0; fileIndex < group.mLeafFileIndices.getSize(); fileIndex++)
         {
            uint leafFileIndex = group.mLeafFileIndices[fileIndex];
            const BString& leafFilename = mLeafFiles[leafFileIndex];

            if (!BWin32FileUtils::doesFileExist(leafFilename))
            {
               allLeafFilesExist = false;
               numMissingLeafFiles++;
               gConsoleOutput.error("Missing leaf file: %s\n", leafFilename.getPtr());
            }
         }
                             
         if ((mMode == cQuick) || (group.mToolIndex == cCopyToolIndex) || (!allSourceFilesExist))
         {
            uint numFileCopiesFailed = 0;
            copyLeafFileGroup(group, numFileCopiesFailed);
                        
            if (numFileCopiesFailed == 0)
               continue;
               
            if ((group.mToolIndex == cCopyToolIndex) || (!allSourceFilesExist))
            {
               gConsoleOutput.error("One or more leaf files are missing, and one or more source files are missing. Skipping file group.\n");
               continue;
            }
         }
         
         leafFileGroupsToUpdate[group.mToolIndex].pushBack(leafFileGroupIndex);
      }
                              
      for (uint toolIndex = 0; toolIndex < mTools.getSize(); toolIndex++)
      {
         const IntArray& fileGroupIndices = leafFileGroupsToUpdate[toolIndex];
         if (fileGroupIndices.isEmpty())
            continue;
         
         const BTool& tool = mTools[toolIndex];
         
         const uint maxProcesses = Math::Min(Math::Clamp<int>(tool.mMaxInstances, 1, 16), mCmdLineParams.mMaxProcesses);
         gConsoleOutput.printf("Updating all files using tool: %s, max processes: %i\n", mTools[toolIndex].mName.getPtr(), maxProcesses);
         
         if (mBuildServer.getInitialized())
            mBuildServer.setMaxActiveProcesses(maxProcesses);   
         else
            mMultiprocessManager.setMaxProcesses(maxProcesses);
                  
         for (uint fileIndex = 0; fileIndex < fileGroupIndices.getSize(); fileIndex++)
         {
            const BGroup& group = mLeafFileGroups[fileGroupIndices[fileIndex]];
                        
            BString logFilename;
            BString errorLogFilename;

            for ( ; ; )
            {
               generateRandomFilename(logFilename, BString(mVariables.getValue("temppath")) + "\\log", ".txt");
               generateRandomFilename(errorLogFilename, BString(mVariables.getValue("temppath")) + "\\errorLog", ".txt");
               
               if ((!BWin32FileUtils::doesFileExist(logFilename.getPtr())) && (!BWin32FileUtils::doesFileExist(errorLogFilename.getPtr())))
                  break;
            }

            remove(logFilename.getPtr());
            remove(errorLogFilename.getPtr());
            
            mVariables.define("logfilename", logFilename);
            mVariables.define("errorlogfilename", errorLogFilename);
            
            mVariables.define("sourcefilename", group.mSourceFiles[0]);
            
            for (uint i = 0; i < group.mLeafFileIndices.getSize(); i++)
            {
               const BString& intermediateFilename = mIntermediateFiles[group.mLeafFileIndices[i]];
               
               BWin32FileUtils::createDirectories(intermediateFilename);
               
               remove(intermediateFilename);
            }
           
            BString destPath;
            strPathGetDirectory(mIntermediateFiles[group.mLeafFileIndices[0]], destPath, true);
            destPath.removeTrailingPathSeperator();
            mVariables.define("destpath", destPath);
            
            BString args(tool.mNormalArgs);
            if ((mMode == cFinal) && (!tool.mFinalArgs.isEmpty()))
               args = tool.mFinalArgs;
            
            BStringArray argStrings;
            if (!resolveText(args, argStrings))
            {
               if (mBuildServer.getInitialized())
                  mBuildServer.cancelAllTasks();
               else
                  mMultiprocessManager.sync();
               return false;
            }
            
            if (argStrings.isEmpty())
            {
               if (mBuildServer.getInitialized())
                  mBuildServer.cancelAllTasks();
               else
                  mMultiprocessManager.sync();
               return false;
            }
            
            BString exec;
            if (!resolveMacros(exec, tool.mExec))
            {
               if (mBuildServer.getInitialized())
                  mBuildServer.cancelAllTasks();
               else
                  mMultiprocessManager.sync();
               return false;
            }
            
            BString finalArgs;
            finalArgs = "\"" + exec + "\" " + argStrings[0];
            
            gConsoleOutput.printf("Running tool: %s\n", finalArgs.getPtr());
            
            BFinalizeLeafFileGroupData* pData = new BFinalizeLeafFileGroupData;
            pData->mLeafFileGroupIndex = fileGroupIndices[fileIndex];
            pData->mArgs               = finalArgs;
            pData->mLogFilename        = logFilename;
            pData->mErrorLogFilename   = errorLogFilename;
            pData->mToolIndex          = toolIndex;

            if (mBuildServer.getInitialized())
            {
               BBuildTaskDesc taskDesc;
               
               taskDesc.mExec = exec;
               taskDesc.mArgs = finalArgs;
               strPathGetDirectory(exec, taskDesc.mPath, true);
               taskDesc.mPath.removeTrailingPathSeperator();
               
               for (uint i = 0; i < group.mLeafFileIndices.getSize(); i++)
               {
                  const BString& intermediateFilename = mIntermediateFiles[group.mLeafFileIndices[i]];
                  taskDesc.mOutputFilenames.pushBack(intermediateFilename);
               }  
               taskDesc.mOutputFilenames.pushBack(logFilename);
               taskDesc.mOutputFilenames.pushBack(errorLogFilename);
               
               taskDesc.mpCompletionCallback = taskCompletionCallback;
               taskDesc.mpCompletionCallbackDataPtr = this;
               taskDesc.mCompletionCallbackData = (uint64)pData;
               
               if (!mBuildServer.addTask(taskDesc))
               {
                  gConsoleOutput.error("updateIntermediateFiles: mBuildServer.addTask() failed!\n");

                  mBuildServer.cancelAllTasks();
                  return false;
               }
            }
            else
            {
               if (!mMultiprocessManager.queue(finalArgs.getPtr(), this, (uint64)pData))
               {
                  gConsoleOutput.error("updateIntermediateFiles: queue() failed!\n");
                                 
                  mMultiprocessManager.sync();
                  return false;
               }
            }               
         }  
         
         if (mBuildServer.getInitialized())
         {
            if (!mBuildServer.waitForAllTasks())
            {
               gConsoleOutput.error("updateIntermediateFiles: mBuildServer.waitForAllTasks() failed!\n");
               return false;
            }
         }
         else
         {
            if (!mMultiprocessManager.sync())
            {
               gConsoleOutput.error("updateIntermediateFiles: sync() failed!\n");
               return false;
            }
         }            
      }
   
      return true;
   }
   
   bool createArchive(const BTarget& target)
   {
      BString logFilename(BString(mVariables.getValue("temppath")) + "\\log.txt");
      mVariables.define("logfilename", logFilename);

      BString errorLogFilename(BString(mVariables.getValue("temppath")) + "\\errorLog.txt");
      mVariables.define("errorlogfilename", errorLogFilename);      

      remove(logFilename);
      remove(errorLogFilename);
      
      BString archiveArgs(mArcGen.mArchiveArgs + " ");
      archiveArgs += target.mArchiveArgs;

      BStringArray strings;
      if ((!resolveText(archiveArgs, strings)) || (strings.isEmpty()))
         return false;
         
      remove(mVariables.getValue("archivefilename"));
      
      BStringArray fileList;
      
      uint numMissingFiles = 0;
      for (uint i = 0; i < mIntermediateFiles.getSize(); i++)
      {
         if (BWin32FileUtils::doesFileExist(mIntermediateFiles[i]))
         {
            fileList.pushBack(mIntermediateFiles[i]);
         }
         else
         {
            gConsoleOutput.error("Intermediate file missing: %s\n", mIntermediateFiles[i].getPtr());
            numMissingFiles++;
         }
      }
      
      if (numMissingFiles)
         gConsoleOutput.error("Total missing files: %u\n", numMissingFiles);
         
      BString listFilename(mVariables.getValue("listfilename"));
      
      FILE* pFile = NULL;
      if (fopen_s(&pFile, listFilename.getPtr(), "w"))
      {
         gConsoleOutput.error("Failed creating file: %s\n", listFilename.getPtr());
         return false;
      }

      for (uint j = 0; j < fileList.getSize(); j++)
         fprintf(pFile, "%s\n", fileList[j].getPtr());

      if (fclose(pFile) != 0)
      {
         gConsoleOutput.error("Failed writing to file: %s\n", listFilename.getPtr());
         return false;
      }
                              
      int status = runArcGen(strings[0].getPtr());
      
      if (BConsoleAppHelper::getLogFile())
         appendLogFile(logFilename.getPtr(), BConsoleAppHelper::getLogFile(), status, strings[0].getPtr(), true);
      remove(logFilename);

      if (BConsoleAppHelper::getErrorLogFile())
         appendLogFile(errorLogFilename.getPtr(), BConsoleAppHelper::getErrorLogFile(), status, strings[0].getPtr(), false);
      remove(errorLogFilename);
            
      if (status != 0)      
      {
         remove(mVariables.getValue("archivefilename"));
         
         gConsoleOutput.error("ARCGEN failed with status: %i\n", status);
         return false;
      }
      
      return true;
   }
   
   bool buildTarget(const BString& targetName)
   {
      gConsoleOutput.printf("Bulding target: %s\n", targetName.getPtr());
      
      BString archiveFilename;
      
      bool scenarioTarget = false;
      
      int targetIndex = 0;
      if (targetName.findLeft(".scn") >= 0)
      {
         scenarioTarget = true;
         targetIndex = findTarget("scenario");
         
         mVariables.define("scenariofilename", targetName.getPtr());
         
         archiveFilename = targetName;
         strPathGetFilename(targetName, archiveFilename);
         while (strPathHasExtension(archiveFilename))
            strPathRemoveExtension(archiveFilename);
         
         archiveFilename = mFinalDirectory + archiveFilename + ".era";
      }
      else
      {
         targetIndex = findTarget(targetName);
         
         mVariables.undefine("scenariofilename");
         
         archiveFilename = mFinalDirectory + targetName + ".era";
      }
      
      if (targetIndex < 0)
      {
         gConsoleOutput.error("Unable to find target: %s\n", targetName.getPtr());
         return false;
      }

      const BTarget& target = mTargets[targetIndex];
      
      mVariables.define("archivefilename", archiveFilename.getPtr());
      
      BString listFilename(BString(mVariables.getValue("temppath")) + "\\fileList.txt");
      mVariables.define("listfilename", listFilename);
      remove(listFilename);
      
      BString logFilename(BString(mVariables.getValue("temppath")) + "\\log.txt");
      mVariables.define("logfilename", logFilename);
      
      BString errorLogFilename(BString(mVariables.getValue("temppath")) + "\\errorLog.txt");
      mVariables.define("errorlogfilename", errorLogFilename);
            
      BString fileListArgs = mArcGen.mFileListArgs + " ";
      fileListArgs += target.mFileListArgs;
      
      BStringArray strings;
      if ((!resolveText(fileListArgs, strings)) || (strings.isEmpty()))
         return false;
      
      int status = runArcGen(strings[0].getPtr());
      
      if (BConsoleAppHelper::getLogFile())
         appendLogFile(logFilename.getPtr(), BConsoleAppHelper::getLogFile(), status, strings[0].getPtr(), true);
      remove(logFilename);

      if (BConsoleAppHelper::getErrorLogFile())
         appendLogFile(errorLogFilename.getPtr(), BConsoleAppHelper::getErrorLogFile(), status, strings[0].getPtr(), false);
      remove(errorLogFilename);
      
      if (status != 0)      
      {
         gConsoleOutput.error("ARCGEN failed with status: %i\n", status);
         return false;
      }
      
      mLeafFileGroups.clear();
      mLeafFiles.clear();
      
      if (!BWin32FileUtils::readStringFile(listFilename, mLeafFiles))
      {
         gConsoleOutput.error("Unable to read list file: %s\n", listFilename.getPtr());
         return false;
      }
      
      if (!createFileGroups())
         return false;
      
      if (!findFilesToUpdate())
         return false;
      
      if (!updateIntermediateFiles())
         return false;
      
      if (!createArchive(target))
         return false;
      
      gConsoleOutput.printf("Target \"%s\" built successfully\n", targetName.getPtr());
      
      return true;
   }
   
   struct BScenarioDesc
   {
      BString mName;
      BString mType;
      
      BScenarioDesc() { }
      BScenarioDesc(const char* pName, const char* pType) : mName(pName), mType(pType) { }
   };
   
   typedef BDynamicArray<BScenarioDesc> BScenarioDescArray;
   BScenarioDescArray mScenarios;
   
   bool loadScenarioDescriptions(void)
   {
      mScenarios.resize(0);
      
      BXMLDocument xmlDoc;
      BString scenarioDescriptionsFilename(mWorkDirectory + "data\\scenariodescriptions.xml");
      if (!BWin32FileUtils::doesFileExist(scenarioDescriptionsFilename))
      {
         gConsoleOutput.error("File doesn't exist: %s\n", scenarioDescriptionsFilename.getPtr());
         return false;
      }

      if (FAILED(xmlDoc.parse(scenarioDescriptionsFilename)))
      {  
         gConsoleOutput.error("Failed parsing scenario descriptions file: %s\n", scenarioDescriptionsFilename.getPtr());
         return false;
      }
      
      const BXMLDocument::BNode* pRootNode = xmlDoc.getRoot();
      if (pRootNode->getName() != "ScenarioDescriptions")
      {
         gConsoleOutput.error("Failed parsing scenario descriptions file: %s\n", scenarioDescriptionsFilename.getPtr());
         return false;
      }
      
      for (uint i = 0; i < pRootNode->getNumChildren(); i++)
      {
         const BXMLDocument::BNode* pChild = pRootNode->getChild(i);
         
         if (pChild->getName() != "ScenarioInfo")
            continue;
         
         BString name, type;
         pChild->getAttributeAsString("File", name);
         pChild->getAttributeAsString("Type", type);
         
         mScenarios.pushBack(BScenarioDesc(name, type));         
      }
      
      return true;
   }
   
   bool buildTargets(void)
   {
      if ((mCmdLineParams.mTargets.isEmpty()) && (mCmdLineParams.mScenarioTargets.isEmpty()))
      {
         gConsoleOutput.warning("No build targets specified, exiting\n");
         return true;
      }
      
      if (!loadScenarioDescriptions())
         return false;
         
      for (uint i = 0; i < mCmdLineParams.mScenarioTargets.getSize(); i++)
      {
         uint scenarioIndex;
         for (scenarioIndex = 0; scenarioIndex < mScenarios.getSize(); scenarioIndex++)
            if (mScenarios[scenarioIndex].mType == mCmdLineParams.mScenarioTargets[i])
            {
               BString filename(mWorkDirectory + "scenario\\" + mScenarios[scenarioIndex].mName);

               if (!buildTarget(filename))
                  return false;
            }

         if (scenarioIndex == mScenarios.getSize())
            gConsoleOutput.warning("No maps found matching scenario type in scenariodescriptions.xml: %s\n", mCmdLineParams.mScenarioTargets[i].getPtr());
      }            
      
      for (uint i = 0; i < mCmdLineParams.mTargets.getSize(); i++)
      {
         if (mCmdLineParams.mTargets[i] == "allstandalone")
         {
            for (uint j = 0; j < mTargets.getSize(); j++)
            {
               if (mTargets[j].mType == "standalone")
               {
                  if (!buildTarget(mTargets[j].mName))
                     return false;
               }
            }
         }
         else
         {  
            if (!buildTarget(mCmdLineParams.mTargets[i]))
               return false;
         }               
      }
      
      return true;
   }
   
};

static int mainInternalClient(BCommandLineParser::BStringArray& args)
{
   BBuildClient buildClient;
   
   if (!buildClient.run(args))
      return EXIT_FAILURE;
   
   return EXIT_SUCCESS;
}

static int mainInternal(int argC, const char* argV[])
{
   XCoreCreate();

   BConsoleAppHelper::setup();
      
   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argC, argV))
   {
      BConsoleAppHelper::deinit();
      XCoreRelease();
      return 100;
   }
   
   if (!BTcpClient::winsockSetup())
   {
      gConsoleOutput.error("Winsock2 setup failed!\n");
      BConsoleAppHelper::deinit();
      XCoreRelease();
      return 100;
   }

   gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s\n", __DATE__, __TIME__);
         
   bool clientMode = false;
   
   for (uint i = 0; i < args.getSize(); i++)
   {
      if ((args[i] == "-client") || (args[i] == "/client"))
      {
         clientMode = true;
         break;
      }
   }
   
   int result = EXIT_FAILURE;
   
   if (clientMode)
   {
      gConsoleOutput.printf("Running as a build client\n");

      result = mainInternalClient(args);
   }
   else
   {
      BDataBuild dataBuild;
      const bool success = dataBuild.process(args);
      
      if (success)
         result = EXIT_SUCCESS;
   }    
   
   if (EXIT_SUCCESS == result)
      gConsoleOutput.printf(PROGRAM_TITLE ": Done\n");
   else
      gConsoleOutput.error(PROGRAM_TITLE ": Failed\n");

   BConsoleAppHelper::deinit();   
   
   BTcpClient::winsockCleanup();
   
   XCoreRelease();
      
   return result;
}

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
      return 100;
   }
#endif   

   return status;
}










