//============================================================================
// test1.cpp
// 
// This module tests the buffered and unbuffered methods of BDeflate, BInflate,
// and BFastInflator classes with random data.
// This is not a very readable example -- intended for stress testing.
// Tested on Xbox with XDK 2732, under Win-32 with VS 2003 and 2005.
//
// rgeldreich@ensemblestudios.com
//============================================================================
#include "ens.h"
#include "deflate.h"
#include "inflate.h"
#include "fastInflate.h"
#include "random.h"

#include <process.h>

#include <vector>
#include <algorithm>

using namespace ens;

typedef std::vector<uchar> UCharVec;

static const char* pTestPhrases[] = 
{
   // http://www.emule.com/poetry/?page=poem;poem=3653
   "System manuals piled high and wasted paper on the floor, ",
   "Once upon a midnight dreary, fingers cramped and vision bleary, ",
   "Longing for the warmth of bed sheets, still I sat there doing spreadsheets. ",
   "Having reached the bottom line I took a floppy from the drawer, ",
   "I then invoked the SAVE command and waited for the disk to store, ",
   "Only this and nothing more. ",
   "Deep into the monitor peering, long I sat there wond'ring, fearing, ",
   "Doubting, while the disk kept churning, turning yet to churn some more. ",
   "But the silence was unbroken, and the stillness gave no token. ",
   "\"Save!\" I said, \"You cursed mother! Save my data from before!\" ",
   "One thing did the phosphors answer, only this and nothing more, ",
   "Just, \"Abort, Retry, Ignore?\" ",
   "Was this some occult illusion, some maniacal intrusion? ",
   "These were choices undesired, ones I'd never faced before. ",
   "Carefully I weighed the choices as the disk made impish noises. ",
   "The cursor flashed, insistent, waiting, baiting me to type some more. ",
   "Clearly I must press a key, choosing one and nothing more, ",
   "From \"Abort, Retry, Ignore?\" ",
   "With fingers pale and trembling, slowly toward the keyboard bending, ",
   "Longing for a happy ending, hoping all would be restored, ",
   "Praying for some guarantee, timidly, I pressed a key. ",
   "But on the screen there still persisted words appearing as before. ",
   "Ghastly grim they blinked and taunted, haunted, as my patience wore, ",
   "Saying \"Abort, Retry, Ignore?\" ",
   "I tried to catch the chips off guard, and pressed again, but twice as hard. ",
   "I pleaded with the cursed machine: I begged and cried and then I swore. ",
   "Now in mighty desperation, trying random combinations, ",
   "Still there came the incantation, just as senseless as before. ",
   "Cursor blinking, angrily winking, blinking nonsense as before. ",
   "Reading, \"Abort, Retry, Ignore?\" ",
   "There I sat, distraught, exhausted, by my own machine accosted. ",
   "Getting up I turned away and paced across the office floor. ",
   "And then I saw a dreadful sight: a lightning bolt cut through the night. ",
   "A gasp of horror overtook me, shook me to my very core. ",
   "The lightning zapped my previous data, lost and gone forevermore. ",
   "Not even, \"Abort, Retry, Ignore?\" ",
   "To this day I do not know the place to which lost data go. ",
   "What demonic nether world us wrought where lost data will be stored, ",
   "Beyond the reach of mortal souls, beyond the ether, into black holes? ",
   "But sure as there's C, Pascal, Lotus, Ashton-Tate and more, ",
   "You will be one day be left to wander, lost on some Plutonian shore, ",
   "Pleading, \"Abort, Retry, Ignore?\" "
};

static const uint cNumPhrases = sizeof(pTestPhrases)/sizeof(pTestPhrases[0]);

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

static void generateTestStream(Random& rand, UCharVec& testStream)
{
   testStream.erase(testStream.begin(), testStream.end());
   
   if ((rand.uRand() & 15) == 0)
   {
      const uint totalBytes = rand.uRand() & (1024*1024-1);
      for (uint k = 0; k < totalBytes; k++)
         testStream.push_back(static_cast<uchar>(rand.uRand() >> 8) & 0xFF);
   }
   else
   {
      const uint totalPhrases = rand.uRand() & 8191;

      const uint numRandomChars = rand.uRand() & 7;

      testStream.erase(testStream.begin(), testStream.end());

      const bool useRuns = rand.uRand() & 1;

      for (uint i = 0; i < totalPhrases; i++)
      {
         const uint phraseIndex = rand.uRand() % cNumPhrases;
         const uint phraseLen = strlen(pTestPhrases[phraseIndex]);

         const uint startIndex = rand.uRand() % phraseLen;
         const uint endIndex = startIndex + (rand.uRand() % (phraseLen - startIndex));
         assert(endIndex <= phraseLen);

         testStream.insert(testStream.end(), pTestPhrases[phraseIndex] + startIndex, pTestPhrases[phraseIndex] + endIndex);

         for (uint k = 0; k < numRandomChars; k++)
            testStream.push_back(static_cast<uchar>(rand.uRand() >> 8) & 0xFF);

         if (useRuns)
         {
            const uint runSize = 1U + static_cast<uint>(fabs(rand.fRandGaussian(0.0f, 1.0)) * 400.0f);
            testStream.insert(testStream.end(), runSize, static_cast<uchar>(rand.uRand() >> 8));
         }
         
         if ((rand.uRand() & 63) == 0)
         {
            const uint totalRandBytes = rand.uRand() & 1023;
            for (uint k = 0; k < totalRandBytes; k++)
               testStream.push_back(static_cast<uchar>(rand.uRand() >> 8) & 0xFF);
         }               
      }
   }         

   if (rand.uRand() & 1)
      std::reverse(testStream.begin(), testStream.end());
}

static void testUnbuffered(void* pData)
{
   uint64 qpcFreq;
   QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&qpcFreq));
   const double ooQPCFreq = 1.0 / qpcFreq;

   const uint processorIndex = reinterpret_cast<uint>(pData);

#ifdef _XBOX   
   XSetThreadProcessor(GetCurrentThread(), processorIndex);
#endif   
   
   Random rand;   
        
   uint64 qpc;
   QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&qpc));
   const DWORD seed = GetTickCount() + static_cast<DWORD>(qpc) + processorIndex * 257 + static_cast<DWORD>(GetCurrentThreadId()) + reinterpret_cast<DWORD>(&rand);
   rand.setSeed(seed);
   
   trace("processor: %u, rand seed: %u\n", processorIndex, seed);
      
   UCharVec srcBuf;
   srcBuf.reserve(8192*1024);
   
   UCharVec dstBuf;
   dstBuf.reserve(8192*1024);
   
   UCharVec decompBuf;
   decompBuf.reserve(8192*1024);
      
   for ( ; ; )
   {
      generateTestStream(rand, srcBuf);
      
      const int srcBufLen = srcBuf.size();
         
      // Dummy byte
      if (srcBuf.empty())         
         srcBuf.push_back(0);
                  
      const uchar* pSrcBuf = &srcBuf.front();
      
      // Test deflator in unbuffered mode
      dstBuf.resize(srcBufLen * 2 + 128);
      uchar* pDstBuf = &dstBuf.front();
      int dstBufLen = dstBuf.size();
      
      double compSpeed, decompSpeed, fastDecompSpeed;
      
      {
         BDeflate* pDeflate = new BDeflate;
         pDeflate->init();
               
         const uint maxCompares = 1 + (rand.uRand() & 8191);               
         const bool greedy = 0 != (rand.uRand() & 1);
         
         int blockTypes = DEFL_ALL_BLOCKS;
         switch (rand.uRand() % 3)
         {
            case 1: DEFL_STATIC_BLOCKS; break;
            case 2: DEFL_DYNAMIC_BLOCKS; break;
         }
         
         uint64 startTime;
         QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));
         
         int status = pDeflate->compressAll(
            pSrcBuf, srcBufLen,
            pDstBuf, dstBufLen,
            maxCompares, blockTypes, greedy);
            
         uint64 endTime;
         QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&endTime));
         
         compSpeed = srcBufLen / (ooQPCFreq * (endTime - startTime));
            
         if (DEFL_STATUS_DONE != status)
            fatalError("compress() failed\n");
            
         delete pDeflate;         
      }
      
      if (dstBuf.empty())
         dstBuf.push_back(0);
                  
      // Test inflator in unbuffered mode
      const uint decompBufMax = srcBufLen;
      decompBuf.resize(Math::Max(1U, decompBufMax));
      uchar* pDecompBuf = &decompBuf.front();
         
      {
         BInflate* pInflate = new BInflate;
         
         uint64 startTime;
         QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));
         
         pInflate->init();
         
         int srcRead = 0;
         int dstWritten = 0;
         int status = pInflate->decompressAll(
            pDstBuf, dstBufLen,
            pDecompBuf, decompBufMax,
            srcRead,
            dstWritten);
         
         uint64 endTime;
         QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&endTime));
         
         if (INFL_STATUS_DONE != status)
            fatalError("decompressAll() failed\n");
         
         if (dstWritten != srcBufLen)
            fatalError("decompressAll() failed to fully decompress\n");
            
         if (srcRead != dstBufLen)
            fatalError("decompressAll() failed to read all source data\n");
                     
         if (srcBufLen)
         {
            if (memcmp(pDecompBuf, pSrcBuf, srcBufLen) != 0)
               fatalError("decompressAll() failed to properly decompress\n");
         }               
            
         delete pInflate;         
         
         decompSpeed = srcBufLen / (ooQPCFreq * (endTime - startTime));
      }
      
      // Test fast inflator (always unbuffered)
      Utils::FastMemSet(pDecompBuf, 0, decompBufMax);
      
      // Guarantee the compressed data buffer is padded by at least FAST_INFLATOR_IN_BUF_PADDING bytes, because it may read a
      // little bit beyond the end of the input buffer.
      dstBuf.resize(dstBuf.size() + FAST_INFL_IN_BUF_PADDING);
      
      {
         BFastInflator* pFastInflate = new BFastInflator;
         
         uint64 startTime;
         QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));
         
         int dstWritten = pFastInflate->decompress(pDstBuf, dstBufLen, pDecompBuf, decompBufMax);
         if (dstWritten != srcBufLen)
            fatalError("Fast inflator failed to fully decompress\n");
            
         uint64 endTime;
         QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&endTime));
            
         if (srcBufLen)
         {
            if (memcmp(pDecompBuf, pSrcBuf, srcBufLen) != 0)
               fatalError("decompressAll() failed to properly decompress\n");         
         }      
         
         delete pFastInflate;
         
         fastDecompSpeed = srcBufLen / (ooQPCFreq * (endTime - startTime));
      }
      
      Utils::FastMemSet(pDecompBuf, 0, decompBufMax);
      
      trace("Thread: %u Input: %u Output: %u, CompSpeed: %8.1f, DecompSpeed: %8.1f, FastDecompSpeed: %8.1f\n", processorIndex, srcBufLen, dstBufLen, compSpeed/(1024*1024), decompSpeed/(1024*1024), fastDecompSpeed/(1024*1024));
   }
}

static void testStreaming(void* pData)
{
   const uint processorIndex = reinterpret_cast<uint>(pData);

#ifdef _XBOX   
   XSetThreadProcessor(GetCurrentThread(), processorIndex);
#endif   
   
   Random rand;   

   uint64 qpc;
   QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&qpc));
   const DWORD seed = GetTickCount() + static_cast<DWORD>(qpc) + processorIndex * 257 + static_cast<DWORD>(GetCurrentThreadId()) + reinterpret_cast<DWORD>(&rand);
   rand.setSeed(seed);

   trace("processor: %u, rand seed: %u\n", processorIndex, seed);

   UCharVec srcBuf;
   srcBuf.reserve(8192*1024);

   UCharVec dstBuf;
   dstBuf.reserve(8192*1024);

   UCharVec decompBuf;
   decompBuf.reserve(8192*1024);

   for ( ; ; )
   {
      MEMORYSTATUS buf;
      GlobalMemoryStatus(&buf);
#ifdef _XBOX      
      trace("Free: %u\n", buf.dwAvailPhys/(1024U*1024U));
#else
      trace("Free: %u\n", buf.dwAvailVirtual/(1024U*1024U));
#endif     

      generateTestStream(rand, srcBuf);
            
      const int srcBufLen = srcBuf.size();
      
      // Dummy byte
      if (srcBuf.empty())         
         srcBuf.push_back(0);

      const uchar* pSrcBuf = &srcBuf.front();
      
      // Test deflator in streaming/buffered mode
      dstBuf.resize(srcBufLen * 2 + 128);
      uchar* pDstBuf = &dstBuf.front();
      int dstBufLen = dstBuf.size();
      
      {
         BDeflate* pDeflate = new BDeflate;
         pDeflate->init();

         const uint maxCompares = 1 + (rand.uRand() & 8191);               
         const bool greedy = 0 != (rand.uRand() & 1);

         int blockTypes = DEFL_ALL_BLOCKS;
         switch (rand.uRand() % 3)
         {
            case 1: DEFL_STATIC_BLOCKS; break;
            case 2: DEFL_DYNAMIC_BLOCKS; break;
         }

         int status;                  
         
         {
            const uint maxSrcBytesPerPass = rand.iRand(1, 1024);
            const uint maxDstBytesPerPass = rand.iRand(1, 1024);
            
            int srcOfs = 0;
            int dstOfs = 0;
            
            do
            {
               int inBufSize = Math::Min<uint>(srcBufLen - srcOfs, maxSrcBytesPerPass);
               int outBufSize = Math::Min<uint>(dstBufLen - dstOfs, maxDstBytesPerPass);

               const bool eofFlag = (inBufSize == (srcBufLen - srcOfs));
               
               status = pDeflate->compress(
                  pSrcBuf + srcOfs, &inBufSize, 
                  pDstBuf + dstOfs, &outBufSize,
                  eofFlag,
                  maxCompares, blockTypes, greedy);

               srcOfs += inBufSize;
               dstOfs += outBufSize;
                              
            } while (DEFL_STATUS_OKAY == status);
            
            dstBufLen = dstOfs;
         }            
         
         if (DEFL_STATUS_DONE != status)
            fatalError("compress() failed\n");

         delete pDeflate;         
      }

      if (dstBuf.empty())
         dstBuf.push_back(0);
                  
      // Test inflator in streaming/buffered mode
      const uint decompBufMax = srcBufLen;
      decompBuf.resize(Math::Max(1U, decompBufMax));
      uchar* pDecompBuf = &decompBuf.front();

      {
         BInflate* pInflate = new BInflate;

         pInflate->init();

         int srcBufOfs = 0;
         int dstBufOfs = 0;
         
         const uint maxSrcBytesPerPass = rand.iRand(1, 1024);
         const uint maxDstBytesPerPass = rand.iRand(1, 1024);
         
         int status;
         do
         {
            int srcBytes = Math::Min<uint>(dstBufLen - srcBufOfs, maxSrcBytesPerPass);
            int dstBytes = Math::Min<uint>(decompBufMax - dstBufOfs, maxDstBytesPerPass);
            const bool eofFlag = (srcBytes == (dstBufLen - srcBufOfs));
            
            status = pInflate->decompress(pDstBuf + srcBufOfs, &srcBytes, pDecompBuf + dstBufOfs, &dstBytes, eofFlag, true, pDecompBuf);
            
            srcBufOfs += srcBytes;
            dstBufOfs += dstBytes;
            
         } while (INFL_STATUS_OKAY == status);
         
         const int srcRead = srcBufOfs;
         const int dstWritten = dstBufOfs;
         
         if (INFL_STATUS_DONE != status)
            fatalError("decompressAll() failed\n");

         if (dstWritten != srcBufLen)
            fatalError("decompressAll() failed to fully decompress\n");

         if (srcRead != dstBufLen)
            fatalError("decompressAll() failed to read all source data\n");

         if (srcBufLen)
         {
            if (memcmp(pDecompBuf, pSrcBuf, srcBufLen) != 0)
               fatalError("decompressAll() failed to properly decompress\n");
         }               

         delete pInflate;         
      }

      // Test fast inflator (always unbuffered)
      Utils::FastMemSet(pDecompBuf, 0, decompBufMax);
      
      // Guarantee the compressed data buffer is padded by at least FAST_INFLATOR_IN_BUF_PADDING bytes, because it may read a
      // little bit beyond the end of the input buffer.
      dstBuf.resize(dstBuf.size() + FAST_INFL_IN_BUF_PADDING);
      
      {
         BFastInflator* pFastInflate = new BFastInflator;
         
         int dstWritten = pFastInflate->decompress(pDstBuf, dstBufLen, pDecompBuf, decompBufMax);
         if (dstWritten != srcBufLen)
            fatalError("Fast inflator failed to fully decompress\n");
        
         if (srcBufLen)
         {
            if (memcmp(pDecompBuf, pSrcBuf, srcBufLen) != 0)
               fatalError("decompressAll() failed to properly decompress\n");         
         }      

         delete pFastInflate;
      }

      Utils::FastMemSet(pDecompBuf, 0, decompBufMax);

      trace("Thread: %u Input: %u Output: %u\n", processorIndex, srcBufLen, dstBufLen);
   }
}

static void testCorruption(void* pData)
{
   const uint processorIndex = reinterpret_cast<uint>(pData);

#ifdef _XBOX   
   XSetThreadProcessor(GetCurrentThread(), processorIndex);
#endif   

   Random rand;   

   uint64 qpc;
   QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&qpc));
   const DWORD seed = GetTickCount() + static_cast<DWORD>(qpc) + processorIndex * 257 + static_cast<DWORD>(GetCurrentThreadId()) + reinterpret_cast<DWORD>(&rand);
   rand.setSeed(seed);

   trace("processor: %u, rand seed: %u\n", processorIndex, seed);

   UCharVec srcBuf;
   srcBuf.reserve(8192*1024);

   UCharVec dstBuf;
   dstBuf.reserve(8192*1024);

   UCharVec decompBuf;
   decompBuf.reserve(8192*1024);
   
   int inflErrorHist[16];
   memset(inflErrorHist, 0, sizeof(inflErrorHist));

   for ( ; ; )
   {
      generateTestStream(rand, srcBuf);

      const int srcBufLen = srcBuf.size();

      // Dummy byte
      if (srcBuf.empty())         
         srcBuf.push_back(0);

      const uchar* pSrcBuf = &srcBuf.front();

      // Test deflator in unbuffered mode
      dstBuf.resize(srcBufLen * 2 + 128);
      uchar* pDstBuf = &dstBuf.front();
      int dstBufLen = dstBuf.size();

      {
         BDeflate* pDeflate = new BDeflate;
         pDeflate->init();

         const uint maxCompares = 1 + (rand.uRand() & 8191);               
         const bool greedy = 0 != (rand.uRand() & 1);

         int blockTypes = DEFL_ALL_BLOCKS;
         switch (rand.uRand() % 3)
         {
            case 1: DEFL_STATIC_BLOCKS; break;
            case 2: DEFL_DYNAMIC_BLOCKS; break;
         }

         int status = pDeflate->compressAll(
            pSrcBuf, srcBufLen,
            pDstBuf, dstBufLen,
            maxCompares, blockTypes, greedy);

         if (DEFL_STATUS_DONE != status)
            fatalError("compress() failed\n");

         delete pDeflate;         
      }

      if (dstBuf.empty())
         dstBuf.push_back(0);

      // Test inflator in unbuffered mode
      const uint decompBufMax = srcBufLen;
      decompBuf.resize(decompBufMax + 4);
      decompBuf[decompBuf.size() - 4] = 'D';
      decompBuf[decompBuf.size() - 3] = 'E';
      decompBuf[decompBuf.size() - 2] = 'A';
      decompBuf[decompBuf.size() - 1] = 'D';
      
      uchar* pDecompBuf = &decompBuf.front();

      BInflate* pInflate = new BInflate;
      
      for (uint t = 0; t < 32; t++)
      {
         UCharVec tempBuf;
         uint tempBufLen;
         
         if ((rand.uRand() & 7) == 0)
         {
            tempBufLen = rand.uRand() & 4095;
            for (uint r = 0; r < tempBufLen; r++)
               tempBuf.push_back(static_cast<uchar>(rand.uRand() >> 8));
         }
         else
         {
            tempBuf = dstBuf;
            tempBufLen = dstBufLen;
                        
            const uint corruptIndex = rand.iRand(0, tempBufLen);
            
            switch (rand.uRand() & 3)
            {
               case 0:
               {
                  tempBuf[corruptIndex] ^= (1 << (rand.uRand() & 7));
                  break;
               }
               case 1:
               {
                  tempBuf[corruptIndex] = static_cast<uchar>(rand.uRand() >> 8);
                  break;
               }
               case 2:
               {
                  const uint bytesToInsert = rand.iRand(1, 16);
                  for (uint l = 0; l < bytesToInsert; l++)
                     tempBuf.insert(tempBuf.begin() + corruptIndex, static_cast<uchar>(rand.uRand() >> 8));
                  tempBufLen += bytesToInsert;
                  break;
               }
               case 3:
               {
                  const int copyDist = Math::Max<int>(-static_cast<int>(corruptIndex), rand.iRand(-256, 1));
                  const int bytesToCopy = Math::Min<int>(tempBufLen - (corruptIndex + copyDist), rand.iRand(1, 16));
                                 
                  for (int l = 0; l < bytesToCopy; l++)
                     tempBuf.insert(tempBuf.begin() + corruptIndex + l, *(tempBuf.begin() + corruptIndex + l + copyDist));
                     
                  tempBufLen += bytesToCopy;
                  
                  break;
               }
            }
         }            
         
         // Dummy byte
         if (tempBuf.empty())
            tempBuf.push_back(0);
                           
         pInflate->init();

         int srcRead = 0;
         int dstWritten = 0;
         int status = pInflate->decompressAll(
            &tempBuf.front(), tempBufLen,
            pDecompBuf, decompBufMax,
            srcRead,
            dstWritten);
            
         // It's possible for decompression to be successful here if the bit(s) corrupted where in the last byte after the last symbol.
         
         if ((srcRead > static_cast<int>(tempBufLen)) || (dstWritten > static_cast<int>(decompBufMax)))
            fatalError("decompressAll() buffer overrun\n");
            
         if ((decompBuf[decompBuf.size() - 4] != 'D') ||
             (decompBuf[decompBuf.size() - 3] != 'E') ||
             (decompBuf[decompBuf.size() - 2] != 'A') ||
             (decompBuf[decompBuf.size() - 1] != 'D'))
            fatalError("decompressAll() buffer overrun\n");
            
         if ((status <= 1) && (status >= -15))
            inflErrorHist[-(status - 1)]++;
      }
      
      delete pInflate;         
     
      trace("Thread: %u Input: %u Output: %u ", processorIndex, srcBufLen, dstBufLen);

      trace("ErrorHist: ");      
      for (int i = -9; i <= 1; i++)
         trace("%u ", inflErrorHist[-(i - 1)]);
      trace("\n");
   }
}

void main(int argC, char* argV[])
{
   argC;
   argV;

   _beginthread(testUnbuffered, 65536, (void*)1);
#ifdef _XBOX          
   _beginthread(testStreaming, 65536, (void*)2);
   _beginthread(testStreaming, 65536, (void*)3);
   _beginthread(testUnbuffered, 65536, (void*)4);
   _beginthread(testCorruption, 65536, (void*)5);
#endif   

   testCorruption(0);
}


