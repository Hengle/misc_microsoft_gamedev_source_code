//============================================================================
// fileTest.cpp
// 
// This module compresses and decompresses all files in the current 
// directory and all subdirectories, or in the directory specified on the 
// command line. 
// Paths specified on the command line should end with a path separator 
// character (i.e. "c:\blah\").
// Tests the BDeflate class, and the BInflate class in buffered/unbuffered
// mode.
// Tested on Xbox with XDK 2732, under Win-32 with VS 2003.
//
// rgeldreich@ensemblestudios.com
//============================================================================
#include "ens.h"
#include "deflate.h"
#include "inflate.h"
#include "fastInflate.h"
#include "adler32.h"

#include <process.h>
#include <vector>
#include <algorithm>
#include <string>

using namespace ens;

typedef std::vector<std::string> StringVec;
typedef std::vector<uchar> UCharVec;

static void vtrace(const char* pMsg, va_list args)
{
   char buf[512];

#if defined(_XBOX) || (_MSC_VER >= 1400)
   vsprintf_s(buf, sizeof(buf), pMsg, args);
#else
   vsprintf(buf, pMsg, args);
#endif   

#ifdef _XBOX
   OutputDebugStringA(buf);
#else   
   printf("%s", buf);
#endif   
}

static void trace(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   vtrace(pMsg, args);
   va_end(args);
}

static void fatalError(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   vtrace(pMsg, args);
   va_end(args);

   DebugBreak();   

   exit(EXIT_FAILURE);
}

static bool findFiles(const std::string& pathname, const std::string& filename, StringVec& files, bool recursive)
{
   WIN32_FIND_DATA findData;
   HANDLE findHandle = FindFirstFile((pathname + filename).c_str(), &findData);
   if (INVALID_HANDLE_VALUE == findHandle)
      return false;
      
   StringVec paths;
   
   do
   {
      const bool directory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      const bool system =  (findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;
      
      std::string filename(findData.cFileName);
      
      if ((!directory) && (!system))
         files.push_back(pathname + filename);
         
      if ((recursive) && (directory))
         paths.push_back(filename);
      
   } while (FindNextFile(findHandle, &findData));

   FindClose(findHandle);
   
   if (recursive)   
   {
      for (uint i = 0; i < paths.size(); i++)
      {
         const std::string& path = paths[i];
         if (path[0] == '.')
            continue;
            
         findFiles(pathname + path + "\\", filename, files, true);
      }
   }

   return true;
}

// Compress a file to another file using BDeflate.
static bool compressFileToFile(const std::string& srcFilename, const std::string& dstFilename, uint64& srcFileSize, uint64& dstFileSize, uint& srcFileAdler32)
{
   srcFileSize = 0;
   dstFileSize = 0;
   srcFileAdler32 = INIT_ADLER32;
   
   const int deflStrategy = DEFL_ALL_BLOCKS;
   const int deflMaxCompares = 1500;
   const bool deflGreedy = false;
   
   FILE* pSrcFile = fopen(srcFilename.c_str(), "rb");
   if (!pSrcFile)
      return false;
      
   FILE* pDstFile = fopen(dstFilename.c_str(), "wb");
   if (!pDstFile)
   {
      fclose(pSrcFile);
      return false;
   }
   
   fseek(pSrcFile, 0, SEEK_END);
   fgetpos(pSrcFile, reinterpret_cast<fpos_t*>(&srcFileSize));
   fseek(pSrcFile, 0, SEEK_SET);
      
   const uint cBufSize = 512 * 1024;
   UCharVec srcBuf(cBufSize + 8);
   UCharVec dstBuf(cBufSize);         
         
   BDeflate* pDeflate = new BDeflate();
   pDeflate->init();
   
   uint64 srcFileBytesRemaining = srcFileSize;
   
   int status = 0; 
   bool eofFlag = false;
   int srcBufBytesRemaining = 0;
   int srcBufOfs = 0;
   bool failed = false;
      
   for ( ; ; )
   {
      if ((!eofFlag) && (!srcBufBytesRemaining))
      {
         srcBufBytesRemaining = static_cast<uint>(Math::Min<uint64>(srcFileBytesRemaining, cBufSize));
         srcBufOfs = 0;
         
         srcFileBytesRemaining -= srcBufBytesRemaining;
         eofFlag = (srcFileBytesRemaining == 0);            
         
         if (srcBufBytesRemaining)
         {
            if (fread(&srcBuf.front(), srcBufBytesRemaining, 1, pSrcFile) != 1)
            {
               failed = true;
               break;
            }
            
            srcFileAdler32 = calcAdler32(&srcBuf.front(), srcBufBytesRemaining, srcFileAdler32);
         }
      }         
      
      int srcBufSize = srcBufBytesRemaining;
      int dstBufSize = cBufSize;
      status = pDeflate->compress(
         &srcBuf.front() + srcBufOfs, &srcBufSize, 
         &dstBuf.front(), &dstBufSize,
         eofFlag, deflMaxCompares, deflStrategy, deflGreedy);
      
      srcBufBytesRemaining -= srcBufSize;
      srcBufOfs += srcBufSize;
      
      if (dstBufSize)
      {
         if (fwrite(&dstBuf.front(), dstBufSize, 1, pDstFile) != 1)
         {
            failed = true;
            break;
         }
         
         dstFileSize += dstBufSize;
      }
               
      if (status == DEFL_STATUS_DONE) 
         break;
      else if (status != DEFL_STATUS_OKAY)
      {
         failed = true;
         break;
      }   
   }
         
   delete pDeflate;
   pDeflate = NULL;
      
   fclose(pSrcFile);
   if (fclose(pDstFile))
      failed = true;
      
   return !failed;
}

// Decompress a file to another file using BInflate in buffered mode.
static bool decompressFileToFile(const std::string& srcFilename, const std::string& dstFilename, uint64& srcFileBytesRead, uint64& dstFileSize, uint64 expectedSize = UINT64_MAX)
{
   srcFileBytesRead = 0;
   dstFileSize = 0;
      
   FILE* pSrcFile = fopen(srcFilename.c_str(), "rb");
   if (!pSrcFile)
      return false;

   FILE* pDstFile = fopen(dstFilename.c_str(), "wb");
   if (!pDstFile)
   {
      fclose(pSrcFile);
      return false;
   }

   fseek(pSrcFile, 0, SEEK_END);
   uint64 srcFileSize;
   fgetpos(pSrcFile, reinterpret_cast<fpos_t*>(&srcFileSize));
   fseek(pSrcFile, 0, SEEK_SET);

   const uint cBufSize = 512 * 1024;
   UCharVec srcBuf(cBufSize);
   UCharVec dstBuf(cBufSize);         

   BInflate* pInflate = new BInflate();
   const bool inflUseFastMemCpy = true;
   pInflate->init(inflUseFastMemCpy);

   uint64 srcFileBytesRemaining = srcFileSize;
      
   int status = 0; 
   bool eofFlag = false;
   int srcBufBytesRemaining = 0;
   int srcBufOfs = 0;
   bool failed = false;
      
   for ( ; ; )
   {
      if ((!eofFlag) && (!srcBufBytesRemaining))
      {
         srcBufBytesRemaining = static_cast<uint>(Math::Min<uint64>(srcFileBytesRemaining, cBufSize));
         srcBufOfs = 0;

         srcFileBytesRemaining -= srcBufBytesRemaining;
         eofFlag = (srcFileBytesRemaining == 0);            

         if (srcBufBytesRemaining)
         {
            if (fread(&srcBuf.front(), srcBufBytesRemaining, 1, pSrcFile) != 1)
            {
               failed = true;
               break;
            }
         }
      }         

      int srcBufSize = srcBufBytesRemaining;
      int dstBufSize = cBufSize;
      if (UINT_MAX != expectedSize)
         dstBufSize = static_cast<uint>(Math::Min<int64>(dstBufSize, expectedSize - dstFileSize));
      
      status = pInflate->decompress(
         &srcBuf.front() + srcBufOfs, &srcBufSize, 
         &dstBuf.front(), &dstBufSize,
         eofFlag);
      
      // srcBufSize could be < 0 here if the inflator read too many bytes into its bit buffer before deciding the stream had ended.

      srcBufBytesRemaining -= srcBufSize;
      srcBufOfs += srcBufSize;
      srcFileBytesRead += srcBufSize;

      if (dstBufSize)
      {
         if (fwrite(&dstBuf.front(), dstBufSize, 1, pDstFile) != 1)
         {
            failed = true;
            break;
         }

         dstFileSize += dstBufSize;
      }

      if (status == INFL_STATUS_DONE) 
         break;
      else if (status != INFL_STATUS_OKAY)
      {
         failed = true;
         break;
      }   
   }

   delete pInflate;
   pInflate = NULL;

   fclose(pSrcFile);
   if (fclose(pDstFile))
      failed = true;
      
   return !failed;
}

// Decompress a compressed stream into a buffer in memory using BInflate in unbuffered mode.
static bool decompressFileToMemory(const std::string& srcFilename, uchar*& pDecompData, uint64& srcFileBytesRead, uint64& dstFileSize, uint64 expectedSize, uint& decompAdler32)
{
   srcFileBytesRead = 0;
   dstFileSize = 0;
   decompAdler32 = INIT_ADLER32;
   
   pDecompData = static_cast<uchar*>(malloc(static_cast<uint>(expectedSize)));
   if (!pDecompData)
      return false;
   
   FILE* pSrcFile = fopen(srcFilename.c_str(), "rb");
   if (!pSrcFile)
   {
      free(pDecompData);
      pDecompData = NULL;
      return false;
   }
            
   fseek(pSrcFile, 0, SEEK_END);
   uint64 srcFileSize;
   fgetpos(pSrcFile, reinterpret_cast<fpos_t*>(&srcFileSize));
   fseek(pSrcFile, 0, SEEK_SET);

   const uint cBufSize = 512 * 1024;
   UCharVec srcBuf(cBufSize);

   BInflate* pInflate = new BInflate();
   const bool inflUseFastMemCpy = true;
   pInflate->init(inflUseFastMemCpy);

   uint64 srcFileBytesRemaining = srcFileSize;

   int status = 0; 
   bool eofFlag = false;
   int srcBufBytesRemaining = 0;
   int srcBufOfs = 0;
   bool failed = false;
         
   for ( ; ; )
   {
      if ((!eofFlag) && (!srcBufBytesRemaining))
      {
         srcBufBytesRemaining = static_cast<uint>(Math::Min<uint64>(srcFileBytesRemaining, cBufSize));
         srcBufOfs = 0;

         srcFileBytesRemaining -= srcBufBytesRemaining;
         eofFlag = (srcFileBytesRemaining == 0);            

         if (srcBufBytesRemaining)
         {
            if (fread(&srcBuf.front(), srcBufBytesRemaining, 1, pSrcFile) != 1)
            {
               failed = true;
               break;
            }
         }
      }         

      int srcBufSize = srcBufBytesRemaining;
      int dstBufSize = static_cast<uint>(expectedSize - dstFileSize);

      status = pInflate->decompress(
         &srcBuf.front() + srcBufOfs, &srcBufSize, 
         pDecompData + dstFileSize, &dstBufSize, 
         eofFlag, 
         false, pDecompData);
         
      if (dstBufSize)         
      {
         decompAdler32 = calcAdler32(pDecompData + dstFileSize, dstBufSize, decompAdler32);
         dstFileSize += dstBufSize;
      }

      // srcBufSize could be < 0 here if the inflator read too many bytes into its bit buffer before deciding the stream had ended.
      // (This can only occur if there are additional bytes after the compressed stream, say in an archive file. In this example, this
      // can't occur because the input stream only consists of a single compressed file.)
      
      srcBufBytesRemaining -= srcBufSize;
      srcBufOfs += srcBufSize;
      srcFileBytesRead += srcBufSize;
            
      if (status == INFL_STATUS_DONE) 
         break;
      else if (status != INFL_STATUS_OKAY)
      {
         failed = true;
         break;
      }   
   }

   delete pInflate;
   pInflate = NULL;

   fclose(pSrcFile);
   
   if (failed)
   {
      free(pDecompData);
      pDecompData = NULL;
   }
   
   return !failed;
}

static bool compareFiles(const std::string& filename1, const std::string& filename2)
{
   FILE* pFile1 = fopen(filename1.c_str(), "rb");
   if (!pFile1)
      return false;
   
   FILE* pFile2 = fopen(filename2.c_str(), "rb");
   if (!pFile2)
   {
      fclose(pFile1);
      return false;
   }
   
   fseek(pFile1, 0, SEEK_END);
   uint64 fileSize1;
   fgetpos(pFile1, reinterpret_cast<fpos_t*>(&fileSize1));
   fseek(pFile1, 0, SEEK_SET);
   
   fseek(pFile2, 0, SEEK_END);
   uint64 fileSize2;
   fgetpos(pFile2, reinterpret_cast<fpos_t*>(&fileSize2));
   fseek(pFile2, 0, SEEK_SET);
   
   if (fileSize1 != fileSize2)
   {
      fclose(pFile1);
      fclose(pFile2);
      return false;
   }
   
   const uint cBufSize = 512 * 1024;
   UCharVec buf1(cBufSize);
   UCharVec buf2(cBufSize);
   
   uint64 bytesRemaining = fileSize1;
   while (bytesRemaining)
   {
      const uint bytesToRead = static_cast<uint>(Math::Min<uint64>(cBufSize, bytesRemaining));
      
      if (fread(&buf1.front(), bytesToRead, 1, pFile1) != 1)
      {
         fclose(pFile1);
         fclose(pFile2);
         return false;
      }
      
      if (fread(&buf2.front(), bytesToRead, 1, pFile2) != 1)
      {
         fclose(pFile1);
         fclose(pFile2);
         return false;
      }
      
      if (memcmp(&buf1.front(), &buf2.front(), bytesToRead) != 0)
      {
         fclose(pFile1);
         fclose(pFile2);
         return false;
      }
   
      bytesRemaining -= bytesToRead;
   }   

   fclose(pFile1);
   fclose(pFile2);
   return true;
}

int main(int argC, char* argV[])
{
#ifdef _XBOX
   XMountUtilityDrive(FALSE, 8192, 512 * 1024);
   XSetFileCacheSize(512 * 1024);
   const char* pSrcPathname = "game:\\";
   const std::string tempPathname("cache:\\");
#else
   const char* pSrcPathname = (argC > 1) ? argV[1] : ".\\";
   const std::string tempPathname(".\\");
#endif

   StringVec files;
   bool success = findFiles(pSrcPathname, "*", files, true);
   if (!success)
   {
      trace("No files found\n");
      return EXIT_FAILURE;
   }
   
   for (uint i = 0; i < files.size(); i++)
   {
      if ( (strstr(files[i].c_str(), "\\comp.bin") != NULL) ||
           (strstr(files[i].c_str(), "\\decomp.bin") != NULL) )
      {
         continue;
      }         
      
      // Readability check
      FILE* pFile = fopen(files[i].c_str(), "rb");
      if (pFile) fclose(pFile);
      if (!pFile)
         continue;
      
      trace("Compressing file %u of %u: %s\n", i + 1, files.size(), files[i].c_str());
      
      uint64 srcFileSize, compFileSize;
      uint srcFileAdler32;
      bool success = compressFileToFile(files[i], tempPathname + "comp.bin", srcFileSize, compFileSize, srcFileAdler32);
      if (!success)
         fatalError("compressFile() failed\n");
      
      trace("Compressed %I64u to %I64u bytes\n", srcFileSize, compFileSize);
      
      if (srcFileSize < 400*1024*1024)
      {
         trace("Decompressing to memory\n");
         
         uint64 decompSrcFileBytesRead;
         uint64 decompDstFileSize;
         uchar* pDecompData;
         uint decompAdler32;
         
         success = decompressFileToMemory(tempPathname + "comp.bin", pDecompData, decompSrcFileBytesRead, decompDstFileSize, srcFileSize, decompAdler32);
         if (!success)
            fatalError("decompressFileToMemory() failed\n");
            
         if (decompSrcFileBytesRead != compFileSize)          
            fatalError("decompressFileToMemory() failed to read all input bytes\n");

         if (decompDstFileSize != srcFileSize)
            fatalError("decompressFileToMemory() failed to fully decompress the output file\n");
            
         if (srcFileAdler32 != decompAdler32)
            fatalError("decompressFileToMemory() failed\n");
            
         // Calc. Adler-32 again on entire buffer -- just being paranoid here.
         const uint adler32 = calcAdler32(pDecompData, static_cast<uint>(srcFileSize));
         if (srcFileAdler32 != adler32)
            fatalError("decompressFileToMemory() failed\n");
                     
         free(pDecompData);            
      }
         
      trace("Decompressing to file\n");
      
      uint64 decompSrcFileBytesRead;
      uint64 decompDstFileSize;   
      success = decompressFileToFile(tempPathname + "comp.bin", tempPathname + "decomp.bin", decompSrcFileBytesRead, decompDstFileSize, srcFileSize);
      if (!success)
         fatalError("decompressFileToFile() failed\n");

      if (decompSrcFileBytesRead != compFileSize)          
         fatalError("decompressFileToFile() failed to read all input bytes\n");
      
      if (decompDstFileSize != srcFileSize)
         fatalError("decompressFileToFile() failed to fully decompress the output file\n");
            
      trace("Comparing\n");
         
      success = compareFiles(files[i], tempPathname + "decomp.bin");
      if (!success)
         fatalError("compareFiles() failed\n");
         
      MEMORYSTATUS memStatus;
      GlobalMemoryStatus(&memStatus);
      
      // This example doesn't use FP, not purposely anyway. Prevent "runtime error R6002 - floating point not loaded" trap.
#ifdef _XBOX      
      const double avail = memStatus.dwAvailPhys;
#else
      const double avail = memStatus.dwAvailVirtual;
#endif      
      trace("Free: %f\n", avail);
   }

   return EXIT_SUCCESS;
}

