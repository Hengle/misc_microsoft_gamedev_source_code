//============================================================================
// File: pdbPack.cpp
//============================================================================
#include "xcore.h"
#include "xcoreLib.h"
#include "xdb\xdb.h"
#include "consoleOutput.h"

#define unknown 
#define __out_xcount(x)
#include "dbghelp/dbghelp.h"
#undef __out_xcount
#undef unknown

#include <conio.h>

//============================================================================
// Globals
//============================================================================
static bool gQuietFlag;
static FILE* gpLogFile;
static FILE* gpErrorLogFile;
static bool gErrorMessageBox;
static uint gTotalErrorMessages;
static uint gTotalWarningMessages;
static BXDBFileBuilder gXDBFileBuilder;

//============================================================================
// ConsoleOutputFunc
//============================================================================
static void ConsoleOutputFunc(DWORD data, BConsoleMessageCategory category, const char* pMessage)
{
   data;

   char buf[1024];

   static const char* pCategoryStrings[] = { "", "Debug: ", "", "Warning: ", "Error: " };
   const uint cNumCategoryStrings = sizeof(pCategoryStrings)/sizeof(pCategoryStrings[0]);
   cNumCategoryStrings;

   BDEBUG_ASSERT((category >= 0) && (category < cNumCategoryStrings));

   StringCchPrintfA(buf, sizeof(buf), "%s%s", pCategoryStrings[category], pMessage, pCategoryStrings[category]);

   if (!gQuietFlag)
   {
      // rg - This should detect when stdout is redirected.

      HANDLE currentConsoleHandle = INVALID_HANDLE_VALUE;
      if ((category == cMsgWarning) || (category == cMsgError))
      {
         currentConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

         const DWORD attr = ((category == cMsgWarning) ? FOREGROUND_GREEN : 0) | FOREGROUND_RED | FOREGROUND_INTENSITY;

         if (INVALID_HANDLE_VALUE != currentConsoleHandle)
            SetConsoleTextAttribute(currentConsoleHandle, (WORD)attr);
      }

      printf("%s", buf);

      if (INVALID_HANDLE_VALUE != currentConsoleHandle)
         SetConsoleTextAttribute(currentConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
   }

   if (gpLogFile)
   {
      fprintf(gpLogFile, "%s", buf);
      fflush(gpLogFile);
   }

   if ((gpErrorLogFile) && (category == cMsgError))
   {
      fprintf(gpErrorLogFile, "%s", buf);
      fflush(gpErrorLogFile);
   }

   if ((gErrorMessageBox) && (category == cMsgError) && (buf[0]))
   {
      MessageBox(NULL, buf, "GR2UGX Error", MB_OK | MB_ICONERROR);
   }

   if (category == cMsgError)
      gTotalErrorMessages++;
   else if (category == cMsgWarning)
      gTotalWarningMessages++;
}

//============================================================================
// convWarning
//============================================================================
static void convWarning(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[512];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.output(cMsgWarning, "%s", buf);
}

//============================================================================
// convError
//============================================================================
static void convError(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[512];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.output(cMsgError, "%s", buf);
}

//============================================================================
// convPrintf
//============================================================================
static void convPrintf(const char* pMsg, ...)
{
   if (gQuietFlag)
      return;

   va_list args;
   va_start(args, pMsg);
   char buf[512];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   gConsoleOutput.output(cMsgStatus, "%s", buf);
}

//============================================================================
// enumSymbolsCallback
//============================================================================
static BOOL CALLBACK enumSymbolsCallback(
   PSYMBOL_INFO pSymInfo,
   ULONG SymbolSize,
   PVOID UserContext)
{
   UserContext;

#if 0
   BFixedString<4096> str;
   str.format("%08X %4u %s", (DWORD)(pSymInfo->Address>>32), SymbolSize, pSymInfo->Name + 4);
   printf("%s\n", str.c_str());
#endif   
   
   gXDBFileBuilder.addSymbol((DWORD)(pSymInfo->Address >> 32U), SymbolSize, pSymInfo->Name + 4);

   return TRUE;
}

//============================================================================
// enumLinesCallback
//============================================================================
static BOOL CALLBACK enumLinesCallback(
                                       PSRCCODEINFO LineInfo,
                                       PVOID UserContext)
{
   UserContext;

#if 0   
   printf("Obj:%s FileName:%s LineNumber:%u Address:0x%I64X\n",
      LineInfo->Obj,
      LineInfo->FileName,
      LineInfo->LineNumber,
      LineInfo->Address);
#endif

   gXDBFileBuilder.addLine((DWORD)LineInfo->Address, LineInfo->LineNumber, LineInfo->FileName);

   return TRUE;
}

//============================================================================
// main
//============================================================================
int main(int argc, const char *argv[])
{
   XCoreCreate();

   gConsoleOutput.init(ConsoleOutputFunc, 0);

   convPrintf("pdbPack Compiled %s %s\n", __DATE__, __TIME__);

   if (argc < 4) 
   {
      convError("Usage: pdbpack filename.exe filename.xex filename.xdb");
      XCoreRelease();
      return EXIT_FAILURE;
   }
   
   const char* pExeFilename = argv[1];
   const char* pXexFilename = argv[2];
   const char* pDstFilename = argv[3];
   BCFileStream outStream;
   if (!outStream.open(pDstFilename, cSFWritable|cSFSeekable))
   {
      convError("Unable to open file for writing: %s\n", pDstFilename);
      XCoreRelease();
      return EXIT_FAILURE;
   }

   BCFileStream xexFileStream;
   if (!xexFileStream.open(pXexFilename, cSFReadable))
   {
      convError("SymInitialize() failed!");
      XCoreRelease();
      return EXIT_FAILURE;
   }
   
   uchar buf[4096];
   const uint numBytesRead = xexFileStream.readBytes(buf, 4096);
   const uint checksum = calcAdler32(buf, numBytesRead);
   gXDBFileBuilder.setChecksum(checksum);
   xexFileStream.close();  
      
   convPrintf("XEX checksum: 0x%08X\n", checksum);

   SymSetOptions(SYMOPT_UNDNAME);

   HANDLE hProcess = 0;

   if (!SymInitialize(hProcess, "", FALSE))
   {
      convError("SymInitialize() failed!");
      XCoreRelease();
      return EXIT_FAILURE;
   }
         
   convPrintf("Loading module\n");
   
   PVOID pvModuleBase = (PVOID)SymLoadModuleEx( hProcess, NULL, pExeFilename, NULL, 0, 0, 0, 0 );

   ULONG64 moduleBase = (ULONG64)((DWORD)(pvModuleBase));

   if (!pvModuleBase)
   {
      // SymLoadModule64 failed
      HRESULT error = GetLastError();
      convError("SymLoadModule64() returned error : %d\n", error);
      XCoreRelease();
      return EXIT_FAILURE;
   }

   BOOL result;

   convPrintf("Enumerating lines\n");
   
   result = SymEnumLines(
      hProcess, 
      moduleBase, 
      NULL, NULL, enumLinesCallback, hProcess); 

   if (!result)
   {
      HRESULT error = GetLastError();
      convError("SymEnumLines() returned error: %d\n", error);
      XCoreRelease();
      return EXIT_FAILURE;
   }  
   
   convPrintf("Read %u lines\n", gXDBFileBuilder.getNumLines());
   
   convPrintf("Enumerating symbols\n");

   result = SymEnumSymbols(hProcess, 
      moduleBase, 
      0, 
      enumSymbolsCallback,
      pvModuleBase);

   if (!result)
   {
      HRESULT error = GetLastError();
      convError("SymEnumSymbols() returned error: %d\n", error);
      XCoreRelease();
      return EXIT_FAILURE;
   }
   
   convPrintf("Read %u symbols\n", gXDBFileBuilder.getNumSymbols());
   
   convPrintf("Writing XDB file\n");
      
   gXDBFileBuilder.setBaseAddress((DWORD)pvModuleBase); 
   if (!gXDBFileBuilder.writeFile(outStream))
   {
      convError("Failed writing to output file %s\n", pDstFilename);
      XCoreRelease();
      return EXIT_FAILURE;
   }
   
   convPrintf("Success.\n");

#if 0
   BCFileStream stream;
   stream.open("G:\\phoenix\\xbox\\work\\tools\\pdbPack\\1");
   
   BXDBFileReader reader;
   bool success = reader.open(&stream);
   
   BXDBFileReader::BLookupInfo info;
   
   //Obj:g:\phoenix\xbox\code\xgame\xbox\debug\usermanager.obj FileName:g:\phoenix\xbox\code\xcore\containers\dynamicarray.h LineNumber:1248 Address:0x82EA2980
   //success = reader.lookup(0x82EA2980, info);
   success = reader.lookup(0x82E942A4, info);
#endif
   
   XCoreRelease();

   return EXIT_SUCCESS;
}
