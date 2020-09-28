//==============================================================================
// consolefuncsengine.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "consolefuncsgame.h"
#include "rendercontrol.h"
#include "scenario.h"

// xscript
#include "xssyscallmodule.h"
#include "xsconfigmacros.h"

// xrender
#include "renderDraw.h"
#include "sceneLightManager.h"
#include "xboxTextureHeap.h"

// xsystem
#include "config.h"
#include "RemoteToolsHost.h"

// xcore
#include "string\strPathHelper.h"
#include "reloadManager.h"

#include "memory\allocationLogger.h"

#include "tiledAA.h"

#include "globalObjects.h"

#ifndef BUILD_FINAL
#include "xbdm.h"
#include "memoryStats.h"

#include "usermanager.h"
#include "user.h"
#include "camera.h"
#include "TerrainVisual.h"
#include "ugxGeomUberSectionRenderer.h"

static void verifyAllHeapsCommand(void)
{
   gConsoleOutput.output(cMsgConsole, "Verifying all heaps");
   verifyAllHeaps();
   gConsoleOutput.output(cMsgConsole, "Verification complete");
}

static void memoryStats(void)
{
   BConsoleMessageCategory prevCategory = gConsoleOutput.getDefaultCategory(); 
   gConsoleOutput.setDefaultCategory(cMsgConsole);
         
   printKernelMemoryStats(gConsoleOutput);
   printHeapStats(gConsoleOutput, false);
   printDetailedHeapStats(gConsoleOutput);
      
   gConsoleOutput.setDefaultCategory(prevCategory);
}

#include "stream\cfilestream.h"
static void xboxTextureHeapStats(void)
{
   if (!gpXboxTextureHeap)
      return;
  
   DmMapDevkitDrive();
   
   BCFileStream stream;
   if (!stream.open("e:\\XboxTextureHeapStats.txt", cSFWritable))
   {
      gConsoleOutput.output(cMsgConsole, "Unable to open file XboxTextureHeapStats.txt!");
      return;
   }
   
   gpXboxTextureHeap->dumpValleyInfo(&stream);
   
   stream.close();
   
   gConsoleOutput.output(cMsgConsole, "Wrote file XboxTextureHeapStats.txt");
}

static void memoryMapPrint(const char* p)
{
   static bool outputDebugString = false;
   if (outputDebugString)
   {
      OutputDebugString(p);
      OutputDebugString("\n");
   }
   gConsoleOutput.output(cMsgConsole, p);
}

void dumpMemoryMap(void)
{
   memoryMapPrint("Virtual 4KB   : 0x00000000-0x3FFFFFFF");
   memoryMapPrint("Virtual 64KB  : 0x40000000-0x7FFFFFFF");
   memoryMapPrint("Image 64KB    : 0x80000000-0x8FFFFFFF");
   memoryMapPrint("Image 4KB     : 0x90000000-0x9FFFFFFF");
   memoryMapPrint("Physical 64KB : 0xA0000000-0xBFFFFFFF");
   memoryMapPrint("Physical 16MB : 0xC0000000-0xDFFFFFFF");
   memoryMapPrint("Physical 4KB  : 0xE0000000-0xFFFFFFFF");
   
   void* prevAllocBase = NULL;
   uint numAllocations = 0;

   BFixedString<1024> buf;
   uint numReservedPages = 0;
   uint numCommittedPages = 0;
   uint numFreePages = 0;
   
   uint stats[128];
   Utils::ClearObj(stats);

   uint s = 0;
   BDynamicSimArray< BFixedString<1024> > lines;
   lines.reserve(128);
   bool allFree = true;
   for (uint64 i = 0; i < ((uint64)4U)*1024U*1024U*1024U; i += 4096U)
   {
      MEMORY_BASIC_INFORMATION info;

      VirtualQuery((LPVOID)i, &info, sizeof(info));

      if (info.AllocationBase != prevAllocBase)   
      {
         prevAllocBase = info.AllocationBase;
         numAllocations++;
      }
      
      if (info.State == MEM_RESERVE)
         numReservedPages++;
      else if (info.State == MEM_COMMIT)
         numCommittedPages++;
      else if (info.State == MEM_FREE)
         numFreePages++;
      
      DWORD xinfo = XQueryMemoryProtect((LPVOID)i);

      char c = '?';
      
      if (xinfo & PAGE_NOCACHE)
      {
         c = 'U';
      }
      else if (xinfo & PAGE_WRITECOMBINE)
      {
         c = 'C';
      }
      else if (xinfo & PAGE_NOACCESS)
      {
         c = 'N';
      }
      else if (info.State == MEM_RESERVE)
      {
         c = 'v';
      }
      else if (xinfo & PAGE_READWRITE)
      {
         if (info.State == MEM_FREE)
            c = 'W';
         else 
            c = '*';
      }
      else if (xinfo & PAGE_READONLY)
      {
         if (info.State == MEM_FREE)
            c = 'R';
         else 
            c = 'r';
      }         
      else if (xinfo & PAGE_GUARD)
      {
         if (info.State == MEM_FREE)
            c = 'G';
         else
            c = 'g';
      }
      else if (info.State == MEM_COMMIT)
      {
         c = '*';
      }
      else if (info.State == MEM_FREE)
      {
         c = '.';
      }
      
      stats[c]++;
      
      if (c != '.')
         allFree = false;    
            
      if (lines.empty() && buf.getEmpty())
         s = static_cast<uint>(i);

      buf.appendChar(c);
      if (buf.getLen() == 128)
      {
         lines.pushBack(buf);

         if (lines.getSize() == 16)
         {
            BFixedString256 x(cVarArg, "0x%08X to 0x%08X", s, i);
            memoryMapPrint(x);

            if (allFree)   
               memoryMapPrint("All pages free");
            else
            {
               for (uint i = 0; i < lines.size(); i++)
               {
                  memoryMapPrint(lines[i].c_str());
                  //memoryMapPrint("");
               }
            }

            lines.resize(0);
            allFree = true;
         }

         buf.truncate(0);
      }      
   }

   if (!lines.empty())
   {
      BFixedString256 x(cVarArg, "0x%08X", s);
      memoryMapPrint(x);

      if (allFree)   
         memoryMapPrint("All pages free");
      else
      {
         for (uint i = 0; i < lines.size(); i++)
         {
            memoryMapPrint(lines[i].c_str());
            //memoryMapPrint("");
         }
         lines.resize(0);
      }
   }      
   
   buf.format("UNKNOWN        ? : %u", stats['?'] * 4096U);
   memoryMapPrint(buf);
   buf.format("NOCACHE        U : %u", stats['U'] * 4096U);
   memoryMapPrint(buf);
   buf.format("WRITECOMBINE   C : %u", stats['C'] * 4096U);
   memoryMapPrint(buf);
   buf.format("NOACCESS       N : %u", stats['N'] * 4096U);
   memoryMapPrint(buf);
   buf.format("RESERVE        v : %u", stats['v'] * 4096U);
   memoryMapPrint(buf);
   buf.format("READWRITE FREE W : %u", stats['W'] * 4096U);
   memoryMapPrint(buf);
   buf.format("READWRITE USED * : %u", stats['*'] * 4096U);
   memoryMapPrint(buf);
   buf.format("READONLY FREE  R : %u", stats['R'] * 4096U);
   memoryMapPrint(buf);
   buf.format("READONLY USED: r : %u", stats['r'] * 4096U);
   memoryMapPrint(buf);
   buf.format("GUARD FREE     G : %u", stats['G'] * 4096U);
   memoryMapPrint(buf);
   buf.format("GUARD USED     g : %u", stats['g'] * 4096U);
   memoryMapPrint(buf);
   buf.format("FREE           . : %u", stats['.'] * 4096U);
   memoryMapPrint(buf);

   buf.format("VirtualQuery() Page Stats: Allocs: %u, Free: %u, Reserved: %u, Committed: %u",
            numAllocations,
            numFreePages*4096U,
            numReservedPages*4096U,
            numCommittedPages*4096U);
   memoryMapPrint(buf);
   
   memoryMapPrint("\nKey:");
   memoryMapPrint("? Unknown");
   memoryMapPrint("U NoCache");
   memoryMapPrint("C WriteCombine");
   memoryMapPrint("N NoAccess");
   memoryMapPrint("v Reserved");
   memoryMapPrint("W Free+ReadWrite");
   memoryMapPrint("* Committed+ReadWrite");
   memoryMapPrint("R Free+ReadOnly");
   memoryMapPrint("r Committed+ReadOnly");
   memoryMapPrint("G Free+Guard");
   memoryMapPrint("g Committed+Guard");
   memoryMapPrint(". Free");   
      
   MEMORYSTATUS stat;
   GlobalMemoryStatus(&stat);
   
   buf = "GlobalMemoryStatus():";
   const uint MB = 1024*1024;
   buf.append( BFixedString256(cVarArg, "%4u total MB of virtual memory.\n", stat.dwTotalVirtual / MB ) );
   buf.append( BFixedString256(cVarArg, "%4u  free MB of virtual memory.\n", stat.dwAvailVirtual / MB ) );
   buf.append( BFixedString256(cVarArg, "%4u total MB of physical memory.\n", stat.dwTotalPhys / MB ) );
   buf.append( BFixedString256(cVarArg, "%4u  free MB of physical memory.", stat.dwAvailPhys / MB ) );

   memoryMapPrint(buf);
}

//==============================================================================
// reloadFile
//==============================================================================
static void reloadFile(const char* pFilename)
{
   gReloadManager.changeNotify(pFilename);
   gConsoleOutput.output(cMsgDebug, "changeNotify: %s", pFilename);
}

//==============================================================================
// causeException
//==============================================================================
static void causeException(int i)
{
   if (i == 666)
   {
#if 0   
      for ( ; ; )
      {
         void* p = malloc(4096);
         if (!p)
            break;
      }
#endif      
      volatile char* p = (char*)0;
      *p = 0;
   }
}

//==============================================================================
// causeAssert
//==============================================================================
static void causeAssert(int i)
{
   if (i == 666)
   {
#if 0   
      for ( ; ; )
      {
         void* p = malloc(4096);
         if (!p)
            break;
      }
#endif      
      BASSERT(0);
   }
}


//==============================================================================
// attachRemoteDebugger
//==============================================================================
void attachRemoteDebugger(char* sysName, long port)
{
#if defined(ENABLE_TIMELINE_PROFILER)   
   BString temp(sysName);
   if(temp.length() <= 0)
   {
      gRemoteToolsHost.update(0, true);
   }
   else
   {

      gRemoteToolsHost.shutdown();
      gRemoteToolsHost.startup(sysName, port);

   }
#endif
}

//==============================================================================
// causeFail
//==============================================================================
static void causeFail(int i)
{
   if (i == 666)
   {
#if 0   
      for ( ; ; )
      {
         void* p = malloc(4096);
         if (!p)
            break;
      }
#endif      
      BFAIL("!");
   }
}

//==============================================================================
// causeOOM
//==============================================================================
static void causeOOM(int i)
{
   if (i == 666)
   {
      for ( ; ; )
      {
         void* p = malloc(4096);
         if (!p)
            break;
      }
   }
}

//==============================================================================
// screenshot
//==============================================================================
static void screenshot(const char* pFilename)
{
   gRenderControl.screenshot(pFilename, false);
}

//==============================================================================
// HDRScreenshot
//==============================================================================
static void HDRScreenshot(const char* pFilename)
{
   gRenderControl.screenshot(pFilename, true);
}

//==============================================================================
// AllocSnapshot
//==============================================================================
static void AllocSnapshot(int i)
{
#ifdef ALLOCATION_LOGGER
   
   if (!getAllocationLogger().getInitialized())
      gConsoleOutput.output(cMsgConsole, "Allocation logger not enabled!");
   else
   {
      getAllocationLogger().logSnapshot(i);
      
      gConsoleOutput.output(cMsgConsole, "Created allocation snapshot.");
   }
#endif   
}

//==============================================================================
// AllocSnapshot
//==============================================================================
static void CloseAllocLog()
{
#ifdef ALLOCATION_LOGGER
   if (!getAllocationLogger().getInitialized())
      gConsoleOutput.output(cMsgConsole, "Allocation logger not enabled!");
   else
   {
      // FIXME: xsystemlib.cpp extern
      extern void deinitAllocationLogger(void);
      deinitAllocationLogger();
      gConsoleOutput.output(cMsgConsole, "Deinitialized allocation logger.");
   }
#endif   
}

//==============================================================================
// megaScreenshot
//==============================================================================
static void megaScreenshot(const char* pStr, int res, const char *pStrQuality)
{
   BFixedStringMaxPath filename;
   if ((!pStr) || (strlen(pStr) == 0))
   {
      SYSTEMTIME systemTime;
      GetLocalTime(&systemTime);

      char systemName[256] = "Unknown ";

      DWORD size = sizeof(systemName);
      DmGetXboxName(systemName, &size);

      filename.format("%s %02u-%02u %02u-%02u-%02u", systemName, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);  
   }
   else
   {
      filename.set(pStr);
   }
   
   res = Math::Clamp(res, 1, 1280*24);
   
   const uint numTiles = Math::Clamp<uint>((uint)ceil(res / 1280.0f), 1, 24);
   
   bool doJitter = true;
   int jitterAAQuality = 1;

   if (strstr(pStrQuality, "til"))
      doJitter = false;
   else if (strstr(pStrQuality, "med"))
      jitterAAQuality = 4;
   else if (strstr(pStrQuality, "hi"))
      jitterAAQuality = 16;
         
   if (!gRenderControl.megaScreenshot(filename, numTiles, numTiles, doJitter, jitterAAQuality))
      gConsoleOutput.output(cMsgConsole, "Unable to create megascreenshot");
   else if (!doJitter)
      gConsoleOutput.output(cMsgConsole, "Creating tiled megascreenshot %s with %ix%i tiles", filename.c_str(), numTiles, numTiles);
   else
      gConsoleOutput.output(cMsgConsole, "Creating jittered megascreenshot %s with %ix%i tiles, %i AASamples", filename.c_str(), numTiles, numTiles, jitterAAQuality);
}

//==============================================================================
// createMinimap
//==============================================================================
static void createMiniMap(int zoomLevel)
{
   if (zoomLevel < 0)
      zoomLevel = 0;

   gRenderControl.generateMinimap(NULL, zoomLevel);   
}

//==============================================================================
// setAA
//==============================================================================
static void setAA(int level)
{
   if (level <= 1)
      gTiledAAManager.setNumTiles(1);
   else if (level == 2)
      gTiledAAManager.setNumTiles(2);
   else
      gTiledAAManager.setNumTiles(3);
      
   gConsoleOutput.output(cMsgConsole, "Tiled AA manager is now using %i tiles", gTiledAAManager.getNumTiles());
}

//==============================================================================
// resetSHFill
//==============================================================================
static void resetSHFill(void)
{
   gSimSceneLightManager.resetSHFillLights();
}

//==============================================================================
// saveSHFill
//==============================================================================
static void saveSHFill(void)
{
   gScenario.saveSHFillLight(0);
}


//==============================================================================
// saveSHFill
//==============================================================================
static void saveSHFillAt(float x, float y, float z,const char* pOutputFilename)
{
   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (!pUser)
      return;
   pUser->getCamera()->setCameraLoc(BVector(x,y,z));

   gRenderControl.generateCubeMap(pOutputFilename);
}

//==============================================================================
// reloadLightSet
//==============================================================================
static void reloadLightSet(void)
{
   gScenario.reloadLightSet();
}

//==============================================================================
// saveHeightfield
//==============================================================================
static void saveHeightfield(void)
{
   gRenderControl.generateHeightfield();
}

//==============================================================================
// saveFileOpenLog
//==============================================================================
static void saveFileOpenLog(const char* pStr)
{
   BFixedStringMaxPath path("fileOpenLog.txt");
   if ((pStr) && (strlen(pStr)))
      path.set(pStr);
   
   eFileManagerError status = gFileManager.saveFileOpenLog(cDirProduction, path);
   
   gConsoleOutput.output(cMsgConsole, (status == cFME_SUCCESS) ? "Successfully wrote file open log" : "Failed writing file open log");
}

//==============================================================================
// clearFileOpenLog
//==============================================================================
static void clearFileOpenLog(void)
{
   gFileManager.clearFileOpenLog();
   gConsoleOutput.output(cMsgConsole, "Cleared file open log");
}

//==============================================================================
// dumpShaderMacros
//==============================================================================
static void dumpShaderMacros(int optimized)
{
   BUGXGeomUberSectionRendererManager::dumpShaderMacros(optimized != 0);
   gConsoleOutput.output(cMsgConsole, "Dumped shader macros");
}

//==============================================================================
// meshDCBRendering
//==============================================================================
static void meshDCBRendering(int enabled)
{
   gRenderControl.setFlag(BRenderControl::cFlagDisableMeshDCBRendering, !enabled);
   
   gConsoleOutput.output(cMsgConsole, "Mesh DCB rendering %s", gRenderControl.getFlag(BRenderControl::cFlagDisableMeshDCBRendering) ? "disabled" : "enabled");
}

//==============================================================================
// dumpRockallStatsCommand
//==============================================================================
static void dumpRockallStatsCommand(void)
{
   dumpRockallStats();   
}

//==============================================================================
// startVideo
//==============================================================================
static void startVideo(const char* pFilename, int downsample, int autoConvert, float fpsLockRate, bool raw)
{
   BFixedStringMaxPath filename;
   if ((!pFilename) || (strlen(pFilename) == 0))
   {
      SYSTEMTIME systemTime;
      GetLocalTime(&systemTime);

      char systemName[256] = "Unknown ";

      DWORD size = sizeof(systemName);
      DmGetXboxName(systemName, &size);

      filename.format("%s %02u-%02u %02u-%02u-%02u", systemName, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);  
   }
   else
   {
      filename.set(pFilename);
   }
   
   gRenderControl.startVideo(filename, downsample != 0, autoConvert != 0, fpsLockRate, raw);
}

//==============================================================================
// endVideo
//==============================================================================
static void endVideo(void)
{
   gRenderControl.endVideo();
}

//==============================================================================
// assetStats
//==============================================================================
void assetStats(const char* pFilename)
{
   createAssetStatsCSV(pFilename);
}

//==============================================================================
//==============================================================================
static void writeMemTracker(const char* pStr, bool snapshot)
{
   bool ok = getAllocationLogger().writeTrackerStats(pStr, snapshot);
   if(ok)
      gConsoleOutput.output(cMsgConsole, "Wrote tracker stats to %s", pStr);
   else
      gConsoleOutput.output(cMsgConsole, "Failed to write tracker states");
}


//==============================================================================
//==============================================================================
static void clearMemTrackerSnapshot()
{
   getAllocationLogger().clearTrackerSnapshot();
   gConsoleOutput.output(cMsgConsole, "Mem tracker snapshot cleared.");
}

#endif // BUILD_FINAL

//==============================================================================
// addEngineFuncs
//==============================================================================
bool addEngineFuncs(BXSSyscallModule *sm)
{
   if (sm == NULL)
      return(false);

#ifndef BUILD_FINAL
   XS_SYSCALL("memoryMap", BXSVariableEntry::cVoidVariable, &dumpMemoryMap, 0)
   XS_HELP("memoryMap(): a detailed system memory map")
   
   XS_SYSCALL("verifyAllHeaps", BXSVariableEntry::cVoidVariable, &verifyAllHeapsCommand, 0)
   XS_HELP("verifyAllHeaps(): Check all memory heaps for corruption")
   
   XS_SYSCALL("exception", BXSVariableEntry::cVoidVariable, &causeException, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("exception(666): cause an exception for testing purposes")
   
   XS_SYSCALL("assert", BXSVariableEntry::cVoidVariable, &causeAssert, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("assert(666): cause an assertion for testing purposes")
   
   XS_SYSCALL("attachRemoteDebugger", BXSVariableEntry::cVoidVariable, &attachRemoteDebugger, 0)
      XS_STRING_PARM("") 
      XS_INTEGER_PARM(1337)
   XS_HELP("attachRemoteDebugger() : Attach to the remote debugger if its running.")

   XS_SYSCALL("fail", BXSVariableEntry::cVoidVariable, &causeFail, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("fail(666): cause a failure for testing purposes")
   
   XS_SYSCALL("oom", BXSVariableEntry::cVoidVariable, &causeOOM, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("oom(666): cause an out of memory condition for testing purposes")

   XS_SYSCALL("reloadFile", BXSVariableEntry::cVoidVariable, &reloadFile, 0)
      XS_STRING_PARM("")
   XS_HELP("reloadFile(\"filename\")")
   
   XS_SYSCALL("screenshot", BXSVariableEntry::cVoidVariable, &screenshot, 0)
      XS_STRING_PARM("");
   XS_HELP("screenshot(\"filename\"): Saves an LDR screenshot (filename optional)")
   
   XS_SYSCALL("HDRScreenshot", BXSVariableEntry::cVoidVariable, &HDRScreenshot, 0)
      XS_STRING_PARM("");
   XS_HELP("HDRScreenshot(\"filename\"): Saves an HDR screenshot (filename optional)")
   
   XS_SYSCALL("allocSnapshot", BXSVariableEntry::cVoidVariable, &AllocSnapshot, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("allocSnapshot(int index): Create a memory allocation snapshot packet (allocation tracking must be enabled)")
   
   XS_SYSCALL("closeAllocLog", BXSVariableEntry::cVoidVariable, &CloseAllocLog, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("closeAllocLog(): Deinitializes the allocation logger (allocation tracking must be enabled)")
   
   XS_SYSCALL("setAA", BXSVariableEntry::cVoidVariable, &setAA, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("setAA(int level): Set AA level")
   
   XS_SYSCALL("resetSHFill", BXSVariableEntry::cVoidVariable, &resetSHFill, 0)
   XS_HELP("resetSHFill(): Sets SH fill to all-black")
   
   XS_SYSCALL("saveSHFill", BXSVariableEntry::cVoidVariable, &saveSHFill, 0)
   XS_HELP("saveSHFill(): Saves the scenario's SH fill lights")

   XS_SYSCALL("saveSHFillAt",BXSVariableEntry::cVoidVariable,&saveSHFillAt,0 )
   XS_FLOAT_PARM(0.0f)
   XS_FLOAT_PARM(0.0f)
   XS_FLOAT_PARM(0.0f)
   XS_STRING_PARM("");
   XS_HELP("saveSHFill(x,y,z, outputFilename): Moves camera to x,y,z; saves the scenario's SH fill lights; If specified to the output filename")
    
   
   XS_SYSCALL("reloadLightSet", BXSVariableEntry::cVoidVariable, &reloadLightSet, 0)
   XS_HELP("reloadLightSet(): Reloads the current scenario's light set/SH fill lights")   
   
   XS_SYSCALL("megaScreenshot", BXSVariableEntry::cVoidVariable, &megaScreenshot, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(1280*4)
      XS_STRING_PARM("Low")
   XS_HELP("megaScreenshot(\"filename\", int res, \"tiled, low, medium, or high\"): Create a mega screenshot - default is low quality");

   XS_SYSCALL("createMiniMap", BXSVariableEntry::cVoidVariable, &createMiniMap, 0)      
      XS_INTEGER_PARM(5)
      XS_HELP("createMiniMap(int zoomLevel): Create a minimap screenshot");
      
   XS_SYSCALL("memoryStats", BXSVariableEntry::cVoidVariable, &memoryStats, 0)
   XS_HELP("memoryStats()");

   XS_SYSCALL("xboxTextureHeapStats", BXSVariableEntry::cVoidVariable, &xboxTextureHeapStats, 0)
   XS_HELP("xboxTextureHeapStats()");
   
   XS_SYSCALL("saveHeightfield", BXSVariableEntry::cVoidVariable, &saveHeightfield, 0)
   XS_HELP("saveHeightfield()");
   
   XS_SYSCALL("saveFileOpenLog", BXSVariableEntry::cVoidVariable, &saveFileOpenLog, 0)
      XS_STRING_PARM("")
   XS_HELP("saveFileOpenLog(\"filename\")");
   
   XS_SYSCALL("clearFileOpenLog", BXSVariableEntry::cVoidVariable, &clearFileOpenLog, 0)
   XS_HELP("clearFileOpenLog()");
   
   XS_SYSCALL("dumpShaderMacros", BXSVariableEntry::cVoidVariable, &dumpShaderMacros, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("dumpShaderMacros(int optimized): Dump UGX shader macros");
   
   XS_SYSCALL("meshDCBRendering", BXSVariableEntry::cVoidVariable, &meshDCBRendering, 0)
      XS_INTEGER_PARM(0)
   XS_HELP("meshDCBRendering(int enabled): Enable/disable mesh DCB usage");
   
   XS_SYSCALL("dumpRockallStats", BXSVariableEntry::cVoidVariable, &dumpRockallStatsCommand, 0)
   XS_HELP("dumpRockallStats(): Call Rockall's PrintMemStatistics() method");
   
   XS_SYSCALL("startVideo", BXSVariableEntry::cVoidVariable, &startVideo, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(1)
      XS_INTEGER_PARM(1)
      XS_FLOAT_PARM(-1.0f)
      XS_BOOL_PARM(false)
   XS_HELP("startVideo(name, downsampleFlag, autoConvertFlag, fpsLockRate, raw): Start video recording");
   
   XS_SYSCALL("endVideo", BXSVariableEntry::cVoidVariable, &endVideo, 0)
   XS_HELP("endVideo(): End video recording");
   
   XS_SYSCALL("assetStats", BXSVariableEntry::cVoidVariable, &assetStats, 0)
      XS_STRING_PARM("stats.csv")
   XS_HELP("assetStats(filename): Dump major assets stats to .CSV file");

   XS_SYSCALL("writeMemTracker", BXSVariableEntry::cVoidVariable, &writeMemTracker, 0)
      XS_STRING_PARM("")
      XS_BOOL_PARM(false)
   XS_HELP("writeMemTracker(string filename, bool snapshot): writes the memtracker's stats to the specified file.  If snapshot is true, data is relative to last time tracker snapshot was cleared.");

   XS_SYSCALL("clearMemTrackerSnapshot", BXSVariableEntry::cVoidVariable, &clearMemTrackerSnapshot, 0)
   XS_HELP("clearMemTrackerSnapshot");
   
#endif // BUILD_FINAL

   return(true);
}
