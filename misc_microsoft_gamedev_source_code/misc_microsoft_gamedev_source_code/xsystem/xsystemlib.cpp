//==============================================================================
// xsystemlib.cpp
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "xsystemlib.h"
#include "xcorelib.h"
#include "config.h"
#include "debugchannel.h"
#include "econfigenum.h"
#include "filemanager.h"
#include "fileUtils.h"
#include "asyncFileManager.h"
#include "perf.h"
#include "workdirsetup.h"
#include "xmlreader.h"
#include "debugConnection.h"
#include "bfileStream.h"

#define ROOT_FILE_CACHE_FILENAME "d:\\root.era"

#ifdef XBOX
#include "xexception\xexception.h"
#include "xdb\xdbManager.h"
#include "file\win32FileStream.h"
#include "bfileStream.h"
#include "file\xboxFileUtils.h"
#endif

#include "memory\allocationLogger.h"
#include "bfileStream.h"
#include "hash\teaCrypt.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

// Globals
static long          gXSystemRefCount;
static XSystemInfo   gXSystemInfo;
BHandle       gRootFileCacheHandle;
BHandle       gLocaleFileCacheHandle;

#ifdef ALLOCATION_LOGGER
static BWin32FileStream* gpAllocationLogStream;

//==============================================================================
// initAllocationLogger
//==============================================================================
static void initAllocationLogger(void)
{
   if (!gConfig.isDefined(cConfigAllocationLogging))
   {
      trace("Disabling allocation logging.\n");
      // This will free up the allocation logger's buffers.
      getAllocationLogger().enableTracking(false);
      return;
   }
   
   trace("Enabling allocation logging.\n");
   
   getAllocationLogger().enableRemoteLogging(gConfig.isDefined(cConfigAllocationLoggingRemote));

   getAllocationLogger().enableTracking(true);

   // Only log sim-related allocations
   if (gConfig.isDefined(cConfigAllocationLoggingSimOnly))
   {
      getAllocationLogger().setLoggingTypeFilter(cAllocLogTypeSim | cAllocLogTypePrimary);
   }

      

#ifndef BUILD_FINAL   
   DmMapDevkitDrive();
#endif   
   
   if(gConfig.isDefined(cConfigAllocationLoggingRemote))
   {
      getAllocationLogger().init(gXFS.getServerIP());
   }
   else
   {
      getAllocationLogger().enableRemoteLogging(false);
      gpAllocationLogStream = new BWin32FileStream;
      if (gpAllocationLogStream->open("E:\\allocationLog.bin", cSFWritable | cSFSeekable | cSFEnableBuffering, &gWin32LowLevelFileIO))
      {
         getAllocationLogger().init(gpAllocationLogStream);
      }
      else
      {
         trace("Unable to open allocation log file!");

         getAllocationLogger().enableTracking(false);

         delete gpAllocationLogStream;
         gpAllocationLogStream = NULL;
      }
   }

   // This is a list of files to ignore when determining whether or not a symbol is a leaf node for allocation tracking purposes.
   static const char* pFilesToIgnore[] = 
   {
      "alignedalloc.cpp",
      "memoryheap.cpp",
      "memoryheap.h",
      "dynamicarray.cpp",
      "dynamicarray.h",
      "dynamicarrayallocators.inl",
      "xboxmemory.cpp",
      "xmemory.cpp",
      "memory.cpp",
      "alignedalloc.inl",
      "allocators.h",
      "memory.hpp",
      "allocfixed.cpp",
      "batchallocator.cpp",
      "texture.cpp",
      "d3d9.h",
      "renderdraw.cpp",
      "initialize.cpp",
      "device.cpp",
      "bstring.inl",
      "utils.h",
      "d3dtextureloader.cpp",
      "xlaunch.c",
      "stream.h",
      "wsprintfw.h",
      "memoryheapmacros.h",
      "segmentedarray.h"
   };
   const uint cNumFilesToIgnore = sizeof(pFilesToIgnore)/sizeof(pFilesToIgnore[0]);

   for (uint i = 0; i < cNumFilesToIgnore; i++)
      getAllocationLogger().logIgnoreLeaf(pFilesToIgnore[i]);


}   

//==============================================================================
// deinitAllocationLogger
// FIXME: This is public so we can call this from a console command callback.
//==============================================================================
void deinitAllocationLogger(void)
{
   if (gpAllocationLogStream)
   {
      getAllocationLogger().deinit();
      
      getAllocationLogger().enableTracking(false);
      
      gpAllocationLogStream->setOwnerThread(GetCurrentThreadId());
      
      delete gpAllocationLogStream;
      gpAllocationLogStream = NULL;
   }
}
#endif   

//============================================================================
// fileManagerInit
//============================================================================
static void initFileManager(bool disableLooseFiles, bool enableArchives)
{
   #define ARCHIVE_PASSWORD "3zDdptN*rV=qOkRbE*NAuWM6"

   uint64 decryptKey[3];
   teaCryptInitKeys(ARCHIVE_PASSWORD, decryptKey[0], decryptKey[1], decryptKey[2]);

   BSHA1 signatureKeys[2];
   uint numKeys = 0;
   
   const unsigned char releasePublicKey[] = { 0xFD,0x01,0x6B,0xE7,0x19,0xC2,0x1B,0xD1,0x4F,0x84,0xCA,0x99,0x61,0xA3,0xF3,0xCB,0x18,0x22,0x1F,0x25 };
   signatureKeys[numKeys].set(releasePublicKey);
   numKeys++;
   
#ifndef BUILD_FINAL      
   const unsigned char testPublicKey[] = { 0x67,0xFB,0xA6,0x9F,0x4B,0x1F,0x42,0xB6,0xFC,0x11,0x71,0x61,0xD2,0x59,0x65,0x97,0x6C,0xDB,0x41,0xE7 };
   signatureKeys[numKeys].set(testPublicKey);
   numKeys++;
#endif
   gAsyncFileManager.init();            
               
   uint fileManagerConfigFlags = BFileManager::cPanicOnFailedArchives;
#ifndef BUILD_FINAL      
   fileManagerConfigFlags |= BFileManager::cRecordOpenedFiles;
#endif      
   if (!disableLooseFiles)
      fileManagerConfigFlags |= BFileManager::cEnableLooseFiles;
   
   if (enableArchives)
   {
      fileManagerConfigFlags |= BFileManager::cWillBeUsingArchives;

#ifdef BUILD_FINAL
      fileManagerConfigFlags |= BFileManager::cBypassXFSForArchives;
#else
      DWORD win32Attributes = gWin32LowLevelFileIO.getFileAttributes("d:\\root.era");
               
      // If we can't find the root archive using Win-32, then try XFS.
      if ((win32Attributes == INVALID_FILE_ATTRIBUTES) || (win32Attributes & FILE_ATTRIBUTE_DIRECTORY))
      {
         if (gXFS.isActive())
         {
            DWORD xfsAttributes = gXFSLowLevelFileIO.getFileAttributes("d:\\root.era");
            
            if ((xfsAttributes == INVALID_FILE_ATTRIBUTES) || (xfsAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
               // Can't find using XFS either.
               fileManagerConfigFlags |= BFileManager::cBypassXFSForArchives;
            }
         }            
      }
      else
      {
         fileManagerConfigFlags |= BFileManager::cBypassXFSForArchives;
      }
#endif
   }

   gFileManager.init(fileManagerConfigFlags, numKeys, signatureKeys, decryptKey);
   
   gFileManager.setDirList(gFileManager.getProductionDirListID(), gXSystemInfo.mProductionDir);

   eFileManagerError result;
   if (enableArchives)
   {
      // All the locale archive first, because it's much smaller.
      result = gFileManager.addFileCache(gXSystemInfo.mLocEraFile, gLocaleFileCacheHandle, false, true);
      if (result == cFME_SUCCESS)
         gFileManager.enableFileCache(gLocaleFileCacheHandle);
         
      result = gFileManager.addFileCache(ROOT_FILE_CACHE_FILENAME, gRootFileCacheHandle, false, true);
      if (result == cFME_SUCCESS)
         gFileManager.enableFileCache(gRootFileCacheHandle);
   }
}      

//============================================================================
// XSystemGetFileSystemSettings
//============================================================================
bool XSystemGetFileSystemSettings(bool& enableArchivesOut, bool& disableLooseFilesOut, bool& disableNoDelay, char* pServerIPOut, int maxServerIPLen, bool xfsCopy)
{
   enableArchivesOut=false;
   disableLooseFilesOut=false;
   disableNoDelay=false;
   if(maxServerIPLen>0)
      pServerIPOut[0]=NULL;

#ifdef BUILD_FINAL
   // Final builds don't use xfs.txt for security reasons.
   enableArchivesOut=true;
   disableLooseFilesOut=true;
   disableNoDelay=false;
   gFinalBuild=true;      
#else
   #ifdef XBOX
      const char* cSystemIdentifier="xbox";
      FILE* xfsFile = NULL;
      fopen_s(&xfsFile, xfsCopy ? "game:\\xfsCopy.txt" : "game:\\xfs.txt", "rt");
      #ifndef BUILD_FINAL      
         if (!xfsFile)
         {
            if (SUCCEEDED(DmMapDevkitDrive()))
            {
               fopen_s(&xfsFile, xfsCopy ? "e:\\xfsCopy.txt" : "e:\\xfs.txt", "rt");
            }
         }
      #endif         
   #else
      const char* cSystemIdentifier="win32";
      FILE* xfsFile=fopen(xfsCopy ? "xfsCopy.txt" : "xfs.txt", "rt");
   #endif

   if(!xfsFile)
   {
      // Can't find xfs.txt - so let's assume we're running with archives.
      enableArchivesOut=true;
      disableLooseFilesOut=true;
      
      blogtrace("XSystemGetFileSystemSettings: Unable to open file xfs.txt");
      return false;
   }

   char token[128];
   for(;;)
   {
      int retval=fscanf_s(xfsFile, "%s", token, sizeof(token));
      if(retval!=1)
         break;
      if(stricmp(token, cSystemIdentifier)==0)
      {
         retval=fscanf_s(xfsFile, "%s", token, sizeof(token));
         if(retval==1)
         {
            int len=strlen(token);
            if(len<maxServerIPLen)
            {
               memcpy(pServerIPOut, token, len);
               pServerIPOut[len]=NULL;
               trace("Using XFS Server %s", pServerIPOut);
            }
         }
      }
      else if(stricmp(token, "enableArchives")==0)
      {
         enableArchivesOut=true;
         trace("Archives enabled");
      }
      else if(stricmp(token, "disableLooseFiles")==0)
      {
         disableLooseFilesOut=true;
         trace("Disabling loose files");
      }
      else if(stricmp(token, "disableNoDelay")==0)
      {
         disableNoDelay=true;
         trace("Disable nodelay");
      }
      else if(stricmp(token, "finalBuild")==0)
      {
         gFinalBuild=true;
         trace("Final build enabled");
      }
   }
   fclose(xfsFile);
#endif

   return true;
}

//==============================================================================
// XSystemCreate
//==============================================================================
bool XSystemCreate(const XSystemInfo* pInfo, BHandle* pRootFileCacheHandle)
{
   if (gXSystemRefCount==0)
   {
      XCoreCreate();
                     
      if (pInfo)
         gXSystemInfo = *pInfo;

#ifndef XBOX
      timeBeginPeriod(1);
#endif

      // File system settings
      bool enableArchives = false;
      bool disableLooseFiles = false;
      bool disableNoDelay = false;
      char xfsServerIP[16] = { '\0' };
      XSystemGetFileSystemSettings(enableArchives, disableLooseFiles, disableNoDelay, xfsServerIP, sizeof(xfsServerIP), pInfo ? pInfo->mXFSCopy : false);

      // XFS and the debug connection
      if ((gXSystemInfo.mUseXFS) && (xfsServerIP[0]))
      {
         if (!gXFS.setup(xfsServerIP, disableNoDelay, gXSystemInfo.mXFSMessageCallback, gXSystemInfo.mXFSConnectionPort))
         {
            XCoreRelease();         
            return false;
         }
         
         gDebugConnection.init(gXFS.getServerIP());
      }

      // Assert callbacks
      if (gXSystemInfo.mPreAssertCallback)
         gAssertionSystem.addPreCallback(gXSystemInfo.mPreAssertCallback, gXSystemInfo.mPreAssertParam1, gXSystemInfo.mPreAssertParam2);

      if (gXSystemInfo.mPostAssertCallback)
         gAssertionSystem.addPostCallback(gXSystemInfo.mPostAssertCallback, gXSystemInfo.mPostAssertParam1, gXSystemInfo.mPostAssertParam2);

#ifdef XBOX
      // Xbox exceptions
      X_Exception::Init();

      // Xbox cache
      BXboxFileUtils::initXboxCachePartition(); 

      // Exception callback
      if (gXFS.getExceptionCallback())
         X_Exception::SetCallback(gXFS.getExceptionCallback());
#endif

#if !defined(BUILD_FINAL) && defined(XBOX)
      BWin32FileStreamFactory win32FileStreamFactory;
      win32FileStreamFactory.setLowLevelFileIO(&gWin32LowLevelFileIO);
      
      bool success = gXDBManager.init(win32FileStreamFactory, 0, "d:\\");
      if (success)
      {  
         success = gXDBManager.loadXDB(win32FileStreamFactory, 0, "d:\\");
         
         if (!success)
            gDebugConnection.sendOutput("XSystemCreate: Failed loading XDB symbols from Win32");
         else
            gDebugConnection.sendOutput("XSystemCreate: Loaded XDB symbols from Win32");
      }
#endif

      BXMLReader::setStreamFactory(&gFileSystemStreamFactory);
      
      initFileManager(disableLooseFiles, enableArchives);
      
#if !defined(BUILD_FINAL) && defined(XBOX)
      if ((gXDBManager.getInitialized()) && (!gXDBManager.getXDBValid()))
      {
         BFileSystemStreamFactory fileSystemStreamFactory;      
         bool success = gXDBManager.loadXDB(fileSystemStreamFactory, 0, "");
         if (!success)
            gDebugConnection.sendOutput("XSystemCreate: Failed loading XDB symbols from the file system");
         else
            gDebugConnection.sendOutput("XSystemCreate: Loaded XDB symbols from the file system");
      }
#endif         
                        
      initializeCoreWorkingDirectoryIDs();
            
      eFileManagerError result = gFileManager.createDirList(gXSystemInfo.mStartupDir, cDirStartup);
      BVERIFY(cFME_SUCCESS == result);
            
      // Config system here
      gConfig.init(NULL);
      gConfig.read(cDirStartup, B("game.cfg"), BConfigData::cSourceGameCfg, false, false, true);
      gConfig.read(cDirStartup, B("locale.cfg"), BConfigData::cSourceLocaleCfg, false, false, true);
      if (gFinalBuild)
         gConfig.read(cDirStartup, B("final.cfg"), BConfigData::cSourceFinalCfg, false, false, true);
      gConfig.read(cDirStartup, B("user.cfg"), BConfigData::cSourceUserCfg, false, false, true);

#ifdef ALLOCATION_LOGGER      
      initAllocationLogger();
#endif      

#ifndef BUILD_FINAL
      if (gConfig.isDefined("filemanagerLogging"))
      {
         trace("File manager logging enabled\n");
         gFileManager.setConfigFlags(gFileManager.getConfigFlags() | BFileManager::cEnableLogging);
      }
      
      if (!gConfig.isDefined("recordOpenFiles"))
      {
         trace("File manager will NOT be recording open files\n");
         gFileManager.enableFileOpenLogging(false);
      }
      else
      {
         trace("File manager is recording open files\n");
         gFileManager.enableFileOpenLogging(true);
      }
#endif      

#ifndef BUILD_FINAL      
      if (gConfig.isDefined(cConfigEnableXMB))
      {
         BXMLReader::setXMBEnabled(true);
         
         gDebugConnection.sendOutput("XSystemCreate: XMB files enabled");
      }
      else
      {
         BXMLReader::setXMBEnabled(false);

         gDebugConnection.sendOutput("XSystemCreate: XMB files disabled");
      }
#endif    
      
#ifndef BUILD_FINAL
      // Assert system options
      if(gConfig.isDefined(cConfigNoAssert))
         gAssertionSystem.setIgnoreAsserts(true);
      if(gConfig.isDefined(cConfigNoAssertDialog))
         gAssertionSystem.setShowDialog(false);
#endif         

      // Perf monitor
      if(!gPerf.init())
      {
         XCoreRelease();         
         return false;
      }

#ifndef BUILD_FINAL
      // Log manager
      if(gConfig.isDefined(cConfigSystemLogging))
      {
         gLogManager.setBaseDirectoryID(gFileManager.getProductionDirListID());
         
         BSimString logName;
         logName="xlog.txt";
         long logFileResult;

#ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigCloseLogOnWrite))
            logFileResult = gLogManager.openLogFile(logName, BLogManager::cPostWriteClose, false, 0, false);
         else if(gConfig.isDefined(cConfigFlushLogOnWrite))
            logFileResult = gLogManager.openLogFile(logName, BLogManager::cPostWriteFlush, false, 0, false);
         else
#endif      
            logFileResult = gLogManager.openLogFile(logName, BLogManager::cPostWriteNone, false, 0, false);
         if(logFileResult >= 0)
            cDefaultHeader = gLogManager.createHeader("default", logFileResult, 0L, false, true, false, false);

#ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigTraceLog))
            gLogManager.setAlwaysTrace(true);
#endif         

         if (gConfig.isDefined("logwarnings"))
         {
            BSimString warningsLogFilename;
            long logFileWarnings;
            if (gConfig.get("logwarnings", warningsLogFilename) && warningsLogFilename.length() > 0)
               logFileWarnings = gLogManager.openLogFile(warningsLogFilename, BLogManager::cPostWriteNone, false, 0, false, true, false);
            else
               logFileWarnings = gLogManager.openLogFile(BSimString("logwarnings.txt"), BLogManager::cPostWriteNone, false, 0, false, true, false);
            if(logFileWarnings >= 0)
               cWarningsHeader = gLogManager.createHeader("WARNING", logFileWarnings, BLogManager::cBaseHeader, false, false, false, false);
         }

         if (gConfig.isDefined("logerrors"))
         {
            BSimString errorsLogFilename;
            long logFileErrors;
            if (gConfig.get("logerrors", errorsLogFilename) && errorsLogFilename.length() > 0)
               logFileErrors = gLogManager.openLogFile(errorsLogFilename, BLogManager::cPostWriteNone, false, 0, false, true, false);
            else
               logFileErrors = gLogManager.openLogFile(BSimString("logerrors.txt"), BLogManager::cPostWriteNone, false, 0, false, true, false);
            if(logFileErrors >= 0)
               cErrorsHeader = gLogManager.createHeader("ERROR", logFileErrors, BLogManager::cBaseHeader, false, false, false, false);
         }
      }
#endif      

#ifndef BUILD_FINAL
      if(gConfig.isDefined(cConfigDumpDirectory))
      {
         // Get time from system
         SYSTEMTIME sysTime;
         GetLocalTime( &sysTime );      
         
         char pFailReportPath[256];
         memset( pFailReportPath, NULL, sizeof( pFailReportPath ) );

         // Setup directory as date
         BSimString failReportDirectory;         
         if(gConfig.get(cConfigDumpDirectory, failReportDirectory) && failReportDirectory.length() > 0)
         {
            sprintf_s( pFailReportPath, sizeof( pFailReportPath ), "%s\\%d-%d-%d", failReportDirectory.getPtr(), sysTime.wMonth, sysTime.wDay, sysTime.wYear );
         }
         else
         {
            sprintf_s( pFailReportPath, sizeof( pFailReportPath ), ".\\%d-%d-%d", sysTime.wMonth, sysTime.wDay, sysTime.wYear );
         }         

         gXFS.setFailReportDirectory( pFailReportPath );
      }
#endif

#ifndef BUILD_FINAL
      if(gXSystemInfo.mpConsoleInterface)
      {
         // Connect the plumbing between XSystem's debug channel and the console
         gDebugChannel.init();
         gDebugChannel.registerConsole(gXSystemInfo.mpConsoleInterface);
      }
#endif      

#ifndef XBOX
      timeEndPeriod(1);
#endif
   }

   gXSystemRefCount++;
   
   if (pRootFileCacheHandle)
      *pRootFileCacheHandle = gRootFileCacheHandle;
      
   return true;
}

//==============================================================================
// XSystemRelease
//==============================================================================
void XSystemRelease()
{
   if (gXSystemRefCount==0)
   {
      BASSERT(0);
      return;
   }

   gXSystemRefCount--;

   if (gXSystemRefCount==0)
   {
      gLogManager.flushAllLogs();
            
#ifndef BUILD_FINAL
#ifdef XBOX      
      gXDBManager.deinit();
#endif      
#endif

#ifdef ALLOCATION_LOGGER      
      deinitAllocationLogger();
#endif   

      eFileManagerError error = gFileManager.deinit(); 
      if (error != cFME_SUCCESS)
         trace("XSystemRelease: File manager deinit failed. One or more files must still be open!");
      
      gAsyncFileManager.deinit();            
      
      if (gXSystemInfo.mUseXFS)
      {
         gDebugConnection.deinit();
         
         gXFS.shutdown();
      }

      XCoreRelease();     
   }
}
