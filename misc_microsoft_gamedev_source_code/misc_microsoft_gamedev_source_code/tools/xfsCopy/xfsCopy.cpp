//============================================================================
// xfsCopy.cpp
//============================================================================

// Includes
#include "xcore.h"
#include "..\shared\consoleGameHelper.h"
#include "timer.h"
#include "xfs.h"
#include "file\win32FindFiles.h"

// Globals
BDynamicArray<BSimString> mSourceFiles;
BDynamicArray<BSimString> mDestFiles;
BDynamicArray<BSimString> mSourceDirs;
BDynamicArray<BSimString> mDestDirs;

//============================================================================
//============================================================================
bool recursiveGetFiles(const BSimString& base, const BSimString& rel, const BSimString& mask, const BSimString& baseDest, const BSimString& relDest)
{
   BSimString basePath(base);
   strPathAddBackSlash(basePath, true);

   BSimString relPath(rel);
   strPathAddBackSlash(relPath, false);

   BSimString baseDestPath(baseDest);
   strPathAddBackSlash(baseDestPath, true);

   BSimString relDestPath(relDest);
   strPathAddBackSlash(relDestPath, false);

   const BSimString findFilePath = basePath + relPath + mask;

   const BSimString findDirPath(basePath + relPath + B("*"));

   //-- Search Vars
   WIN32_FIND_DATA findData;
   HANDLE hFind;
   bool bDone;
   BSimString foundPath, foundDestPath;

   const bool bWantDirs = true;
   const bool bWantFiles = true;

   //-- Search the current directory for files that match.
   hFind = gXFS.findFirstFile(findFilePath.getPtr(), &findData); 
   if (hFind == INVALID_HANDLE_VALUE)
   {
      HRESULT hres = GetLastError();
      if ((hres != NO_ERROR) && (hres != ERROR_FILE_NOT_FOUND))
         return false;
   }
   else
   {
      //-- Process each file
      bDone = FALSE;
      while (!bDone)
      {
         //-- ignore the types we don't care about.
         const bool bDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;

         if ((bDir && !bWantDirs) || (!bDir && !bWantFiles))
         {
            bDone = !gXFS.findNextFile(hFind, &findData);
            continue;
         }

         //-- Ignore "." and ".."
         bool skip = false;
         if (strcmp(findData.cFileName, ".") == 0) skip = true;
         if (strcmp(findData.cFileName, "..") == 0) skip = true;
         if (skip)
         {
            bDone = !gXFS.findNextFile(hFind, &findData);
            continue;
         }	

         //-- We found a file that we want.  Format it.
         foundPath = basePath + relPath + findData.cFileName;
         foundDestPath = baseDestPath + relDestPath + findData.cFileName;

         if (bDir)
         {
            mSourceDirs.pushBack(foundPath);
            mDestDirs.pushBack(foundDestPath);
         }
         else
         {
            mSourceFiles.pushBack(foundPath);
            mDestFiles.pushBack(foundDestPath);
         }

         bDone = !gXFS.findNextFile(hFind, &findData);
      }
      gXFS.findClose(hFind);
   }

   //-- Search the current directory for files that match.
   hFind = gXFS.findFirstFile(findDirPath.getPtr(), &findData); 
   if (hFind == INVALID_HANDLE_VALUE)
   {
      if (GetLastError() != NO_ERROR)
         return false;
   }
   else
   {
      //-- Process each file
      bDone = FALSE;
      while (!bDone)
      {
         //-- skip all but dirs.
         if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
         {
            bDone = !gXFS.findNextFile(hFind, &findData);
            continue;
         }

         //-- Ignore "." and ".."
         bool skip = false;
         if (strcmp(findData.cFileName, ".") == 0) skip = true;
         if (strcmp(findData.cFileName, "..") == 0) skip = true;
         if (skip)
         {
            bDone = !gXFS.findNextFile(hFind, &findData);
            continue;
         }	

         recursiveGetFiles(base, relPath + findData.cFileName, mask, baseDest, relDestPath + findData.cFileName);
         bDone = !gXFS.findNextFile(hFind, &findData);
      }
      gXFS.findClose(hFind);
   }

   return(true);
}

//============================================================================
//============================================================================
bool getFiles(const BSimString& path, const BSimString& destPath)
{
   BSimString filePath, searchMask;
   strPathGetDirectory(path, filePath, true);
   strPathGetFilename(path, searchMask);

   BSimString destPath2 = destPath;
   strPathAddBackSlash(destPath2, false);
   BSimString destFilePath;
   strPathGetDirectory(destPath2, destFilePath, true);

   return recursiveGetFiles(filePath, B(""), searchMask, destFilePath, "");
}

//============================================================================
//============================================================================
void showProgress(int i, double totalSec, DWORD totalCopied)
{
   float min = (float)((long)(totalSec / 60.0));
   float sec = (float)(totalSec - (min * 60.0));
   BConsoleGameHelper::consoleClear();
   gConsoleOutput.printf("xfsCopy v1.02\n");
   gConsoleOutput.printf("Source:      %s\n", mSourceFiles[i].getPtr());
   gConsoleOutput.printf("Destination: %s\n", mDestFiles[i].getPtr());
   gConsoleOutput.printf("Speed:       %01.01fmb/s\n", (totalCopied > 0 ? totalCopied / totalSec : 0)/(1024*1024));
   gConsoleOutput.printf("Elapsed:     %01.fm %02.fs\n", min, sec);
   gConsoleOutput.printf("Copied:      %01.01fmb\n", (float)totalCopied / (float)(1024*1024));
   gConsoleOutput.printf("Hold Start Button to quit\n");
   BConsoleGameHelper::consoleRender();
}

//============================================================================
//============================================================================
void main(void)
{
   // Setup console
   if (!BConsoleGameHelper::setup(true, true))
      return;
   if (!BConsoleGameHelper::consoleInit())
      return;      
   BConsoleGameHelper::setNoAutoRender(true);

   // XFS must be active
   if (!gXFS.isActive())
   {
      BConsoleGameHelper::deinit();
      return;
   }

   gConsoleOutput.printf("xfsCopy\n");
   BConsoleGameHelper::consoleRender();

   // Read in settings from xfsCopy.txt file
   BSimString lineStr, lineType, lineData, sourcePath, destPath;

   FILE* pSettingsFile = NULL;
   fopen_s(&pSettingsFile, "game:\\xfsCopy.txt", "rt");
   if (!pSettingsFile)
   {
      BConsoleGameHelper::deinit();
      return;
   }
   char line[1024];
   for (;;)
   {
      if (!fgets(line, 1024, pSettingsFile))
      {
         fclose(pSettingsFile);
         break;
      }
      int len = strlen(line);
      if (len > 0 && line[len-1] == '\n')
         line[len-1] = NULL;
      lineStr = line;
      int loc = lineStr.findLeft(' ');
      if (loc == -1)
         continue;
      lineType = lineStr;
      lineType.left(loc);

      lineData = lineStr;
      lineData.mid(loc+1, len-loc);

      if (lineType == "sourcePath")
         sourcePath = lineData;
      else if (lineType == "destPath")
         destPath = lineData;
   }
   fclose(pSettingsFile);

   // Get list of files to copy
   gConsoleOutput.printf("Searching for files...\n");
   BConsoleGameHelper::consoleRender();
   getFiles(sourcePath, destPath);

   // Create base directories
   int start = destPath.findLeft(':');
   if (start != -1)
   {
      start++;
      if (start < destPath.length())
      {
         if (destPath.getChar(start) == '\\')
            start++;
      }
   }
   else
      start = 0;
   for (;;)
   {
      int loc = destPath.findLeft('\\', start);
      if (loc == -1)
      {
         CreateDirectory(destPath, NULL);
         break;
      }
      BSimString dir = destPath;
      dir.left(loc);
      CreateDirectory(dir, NULL);
      start = loc + 1;
   }

   // Create sub directories
   mDestDirs.sort();
   for (uint i=0; i<mDestDirs.size(); i++)
   {
      BSimString dir = mDestDirs[i];
      CreateDirectory(dir.getPtr(), NULL);
   }

   // Create buffer for copying files
   const DWORD cBufferSize = 5 * 1024 * 1024;
   BYTE* pBuffer = new BYTE[cBufferSize];
   if (!pBuffer)
   {
      BConsoleGameHelper::deinit();
      return;
   }

   // Copy all of the files
   bool error = false;
   DWORD errorCode = 0;
   DWORD totalCopied = 0;
   bool quit = false;
   bool showErrors = true;

   BTimer timer;
   timer.start();

   BDynamicArray<BSimString> mErrorFiles;

   for (uint i=0; i<mSourceFiles.size(); i++)
   {
      showProgress(i, timer.getElapsedSeconds(), totalCopied);
      if ((BConsoleGameHelper::getButtons() & XINPUT_GAMEPAD_START) != 0)
      {
         quit = true;
         break;
      }

      HANDLE hSourceFile = gXFS.createFile(mSourceFiles[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (hSourceFile == INVALID_HANDLE_VALUE)
      {
         errorCode = GetLastError();
         error = true;
         mErrorFiles.add(mSourceFiles[i]);
         quit = true;
         // Don't show errors because likely the user cancelled the copy from GetBuild
         showErrors = false;
      }
      else
      {
         if (GetFileAttributes(mDestFiles[i]) != -1)
         {
            SetFileAttributes(mDestFiles[i], FILE_ATTRIBUTE_NORMAL);
            DeleteFile(mDestFiles[i]);
         }
         HANDLE hDestFile = CreateFile(mDestFiles[i], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
         if (hDestFile == INVALID_HANDLE_VALUE)
         {
            errorCode = GetLastError();
            error = true;
            mErrorFiles.add(mDestFiles[i]);
         }
         else
         {
            DWORD sourceSize = gXFS.getFileSize(hSourceFile, NULL);
            DWORD bytesLeft = sourceSize;
            while (bytesLeft > 0)
            {
               showProgress(i, timer.getElapsedSeconds(), totalCopied);
               if ((BConsoleGameHelper::getButtons() & XINPUT_GAMEPAD_START) != 0)
               {
                  quit = true;
                  break;
               }

               DWORD bytesToRead = min(bytesLeft, cBufferSize);
               DWORD bytesRead = 0;
               gXFS.readFile(hSourceFile, pBuffer, bytesToRead, &bytesRead, NULL);
               if (bytesRead != bytesToRead)
               {
                  error = true;
                  mErrorFiles.add(mSourceFiles[i]);
                  quit = true;
                  // Don't show errors because likely the user cancelled the copy from GetBuild
                  showErrors = false;
                  break;
               }
               DWORD bytesWritten = 0;
               if (!WriteFile(hDestFile, pBuffer, bytesRead, &bytesWritten, NULL))
               {
                  errorCode = GetLastError();
                  error = true;
                  mErrorFiles.add(mSourceFiles[i]);
                  break;
               }
               bytesLeft -= bytesRead;
               totalCopied += bytesRead;
            }
            CloseHandle(hDestFile);

            if (quit)
               break;
         }
         gXFS.closeHandle(hSourceFile);
      }

     // if (error)
     //    break;
   }

   // Delete the file buffer
   delete[] pBuffer;

   // Display list of files that had errors
   if (mErrorFiles.size() > 0 && showErrors)
   {
      FILE* pErrorFile = NULL;
      fopen_s(&pErrorFile, "game:\\copyError.txt", "wt");
      BConsoleGameHelper::consoleClear();
      for (uint i=0; i<mErrorFiles.size(); i++)
      {
         BSimString& fileName = mErrorFiles[i];
         if (pErrorFile)
         {
            fputs(fileName.getPtr(), pErrorFile);
            fputs("\n", pErrorFile);
         }
         gConsoleOutput.printf("file error: %s\n", fileName.getPtr());
         BConsoleGameHelper::consoleRender();
      }
      if (pErrorFile)
         fclose(pErrorFile);
      gConsoleOutput.printf("Press Start Button to exit.\n");
      BConsoleGameHelper::consoleRender();
      BConsoleGameHelper::waitForButtonPress(XINPUT_GAMEPAD_START);
   }

   // Done with the console
   BConsoleGameHelper::deinit();
}
