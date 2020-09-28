//============================================================================
//
//  timelineprofiler.cpp
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#include "xsystem.h"

#if defined(ENABLE_TIMELINE_PROFILER)


#include "timelineprofiler.h"
#ifndef XBOX
#include "sseutils.h"
#endif
#include "leastsquareslinefitter.h"
#include "rangecoder.h"

#include "perf.h"
#include "config.h"

#include "timelineprofilersample.h"
//#define DUMP_OVERHEAD_TIME_TO_TRACE      

//---------------------------------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------------------------------
BProfileSystem gBProfileSystem;

#ifdef BUILD_FINAL
   static const int MaxRegisteredSections = 1;
#else
   static const int MaxRegisteredSections = 256;
#endif


void StartGPU(DWORD Context)
{
   //*reinterpret_cast<uint64*>(Context) = (uint64)GetCurrentProcessorNumber();
   *reinterpret_cast<uint64*>(Context) = ReadCycleCounterNoFlush();

}
void EndGPU(DWORD Context)
{
   
   //*reinterpret_cast<uint64*>(Context) = (uint64)GetCurrentProcessorNumber();
   *reinterpret_cast<uint64*>(Context) = ReadCycleCounterNoFlush();
}

//---------------------------------------------------------------------------------------------------
// Statics
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
// BProfileManager::BProfileManager
//---------------------------------------------------------------------------------------------------
BProfileManager::BProfileManager(int threadID/*double OOCompCPUFreq, double OOGPUFreq, bool GPUQueriesAvailable, bool cpuOnly*/) :
   mpDev(NULL),
   mStartOfFrameCPUStamp(0),
   mNextFreeQuery(0),
   mFirstBusyQuery(0),
   mNextFreeSample(0),
   mNextRetiredSample(0),
   mNextActiveSample(0),
   mSampleStackTop(0),
   mpVisualizer(NULL),
   mStartOfFrameBaseTime(0),
   mLastRegisteredSectionID(0),
   mFrameNumber(0),
   mThreadID(threadID)
{
   clearQueriesAndSamples();
   
   mpDev = NULL;
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::~BProfileManager
//---------------------------------------------------------------------------------------------------
BProfileManager::~BProfileManager()
{
   deinit();
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::init
//---------------------------------------------------------------------------------------------------
void BProfileManager::init(IDirect3DDevice9* pDev)
{         
   mpDev = pDev;

   deinit();
   
      
#ifndef XBOX
   if (gBProfileSystem.areGPUQueriesAvailable())
   {
      for (int i = 0; i < MaxQueries; i++)
      {
         HRESULT hres = mpDev->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &mQueryPool[i].mpQuery);
         BVERIFY(SUCCEEDED(hres));
      }
   } 
#endif
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::deinit
//---------------------------------------------------------------------------------------------------
void BProfileManager::deinit(void)
{
#ifdef XBOX
   if(mpDev != NULL)
   {
      mpDev->BlockUntilIdle();
   }
#endif

   for (int i = 0; i < MaxQueries; i++)
   {
      if (mQueryPool[i].mpQuery)
      {
         mQueryPool[i].mpQuery->Release();
         mQueryPool[i].mpQuery = NULL;
      }
   }

   clearQueriesAndSamples();
}




//---------------------------------------------------------------------------------------------------
// BProfileManager::startSample
//---------------------------------------------------------------------------------------------------
void BProfileManager::startSample(BProfileSection& section, bool startOfFrame, uint userID)
{
   uint64 frameNumber = gBProfileSystem.getFrameNumber();

   if(mFrameNumber < frameNumber)
   {
      getRetiredSamples();
      mFrameNumber = frameNumber;
      startOfFrame = true;
   }

   const uint64 cpuCycleCounter = ReadCycleCounterNoFlush();
   
   BDEBUG_ASSERT(section.name());
   BVERIFY(mSampleStackTop < SampleStackSize);
   
   const bool sampleBufferFull = Math::NextWrap<uint>(mNextFreeSample, MaxSamples) == mNextRetiredSample;
   if (sampleBufferFull)
   {
      // if sample buffer is full, create a dummy stack entry
      mSampleStack[mSampleStackTop] = -1;
   }
   else
   {
      BSample& sample = mSamples[mNextFreeSample];

      sample.mCPUStartStamp = cpuCycleCounter;
      sample.mpStartQuery = NULL;      
#ifdef XBOX
      if(!section.cpuOnly())
      {
         if(mpDev)
            mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE , &StartGPU,  (DWORD)(&(sample.mGPUStartStamp)));
      }
#else      
      if ((gBProfileSystem.areGPUQueriesAvailable()) && (!section.cpuOnly()) && (anyQueriesAvailable()))
      {
         sample.mpStartQuery = getNextQuery();
         sample.mpStartQuery->Issue(D3DISSUE_END);
      }
#endif
      sample.mpSection = &section;
      sample.mCPUEndStamp = 0;
      sample.mGPUStartStamp = 0;
      sample.mGPUEndStamp = 0;
      sample.mpEndQuery = NULL;
      sample.mStartOfFrame = startOfFrame;
      sample.mLevel = static_cast<uchar>(mSampleStackTop) | 0x80;
      sample.mUserID = userID;
      sample.mSectionID = section.id();

#ifdef XBOX
      sample.mUserID = static_cast<int>(GetCurrentProcessorNumber());
#endif

      mSampleStack[mSampleStackTop] = mNextFreeSample;
      mNextFreeSample = Math::NextWrap<uint>(mNextFreeSample, MaxSamples);
   }
   
   mSampleStackTop++;
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::endSampleCommon
//---------------------------------------------------------------------------------------------------
void BProfileManager::endSampleCommon(uint userID, bool hasUserID, BProfileSection* pSection)
{
   // umm, this shouldn't happen
   if (mSampleStackTop == 0)
      return;
      
   const uint64 cpuCycleCounter = ReadCycleCounterNoFlush();

   mSampleStackTop--;
   if (-1 != mSampleStack[mSampleStackTop])
   {
      BSample& sample = mSamples[mSampleStack[mSampleStackTop]];
      if (pSection)
      {
         // if this assert fires, somebody probably messed up their perfBegin()/perfEnd() macros
         // fixme: should find a better way to deal with this
         //BDEBUG_ASSERT(sample.mpSection == pSection);
      }
      
      sample.mCPUEndStamp = cpuCycleCounter;
      sample.mLevel &= 0x7F;
      if (hasUserID)
         sample.mUserID = userID;

#ifdef XBOX
      if(pSection && !pSection->cpuOnly())
      {
         if(mpDev)
            mpDev->InsertCallback(D3DCALLBACK_IMMEDIATE , &EndGPU,  (DWORD)(&(sample.mGPUEndStamp)));
      }
#else
      if (sample.mpStartQuery)
      {
         if (!anyQueriesAvailable())
         {
            freeQuery();
            sample.mpStartQuery = NULL;
         }
         else
         {
            sample.mpEndQuery = getNextQuery();
            sample.mpEndQuery->Issue(D3DISSUE_END);
         }
      }
#endif
   }
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::startOfFrame
//---------------------------------------------------------------------------------------------------
void BProfileManager::startOfFrame(void)
{
   // this shouldn't happen!
   if (mSampleStackTop != 0)
      endOfFrame();
   
   mStartOfFrameCPUStamp = ReadCycleCounterNoFlush();

   static BProfileSection startOfFrameID("Frame", true);
   startSample(startOfFrameID, true);
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::endOfFrame
//---------------------------------------------------------------------------------------------------
void BProfileManager::endOfFrame(void)
{
   // If the user called deinit() inside a frame (resizing screen, device lost), don't end the startOfFrame sample. (rg [1/30/05] what??)
   
   // This loop shouldn't be necessary, but if the user screwed up and didn't match up their starts/ends this loop should 
   // fix things up so we can at least continue without crashing.
   while (mSampleStackTop != 0)
   {
      endSample();
   }      
   
   if (mNextActiveSample != mNextFreeSample)   
   {
      const BSample& sample = mSamples[mNextActiveSample];
      sample;

      // All active samples should be retired here!
      BASSERT((sample.mLevel & 0x80) == 0);
   }   
}   
   
//---------------------------------------------------------------------------------------------------
// BProfileManager::getRetiredSamples
//---------------------------------------------------------------------------------------------------
void BProfileManager::getRetiredSamples(void)
{
#ifdef DUMP_OVERHEAD_TIME_TO_TRACE      
   uint64 startTime = ReadCycleCounter();
#endif

   BFPUPrecision fpuPrecision(true);
   
   retireSamples();
      
   if (mpVisualizer)
   {
      uint sampleIndex = mNextRetiredSample;
      while (sampleIndex != mNextActiveSample)
      {
         const BSample& sample = mSamples[sampleIndex];
                  
         // Use sample from first frame as the start of frame base time which all scaled times are relative to
         if (sample.mStartOfFrame)
            mStartOfFrameBaseTime = (sample.mCPUStartStamp - gBProfileSystem.getCPUSessionTimestamp()) * gBProfileSystem.getOOCompCPUFreq();

         BRetiredProfileSample retiredSample;
                              
         retiredSample.mSectionID = sample.mSectionID;
         BDEBUG_ASSERT(sample.mSectionID < gBProfileSystem.getNumSections());
                           
         retiredSample.mCPUStartScaledTime = ScaleProfileSampleTime((sample.mCPUStartStamp - gBProfileSystem.getCPUSessionTimestamp()) * gBProfileSystem.getOOCompCPUFreq() - mStartOfFrameBaseTime);
         retiredSample.mCPUEndScaledTime   = ScaleProfileSampleTime((sample.mCPUEndStamp   - gBProfileSystem.getCPUSessionTimestamp()) * gBProfileSystem.getOOCompCPUFreq() - mStartOfFrameBaseTime);

         // Check GPU start stamp against 0 because it may have been forced to retire
         retiredSample.mGPUStartScaledTime = 0;
         retiredSample.mGPUEndScaledTime = 0;
         
         if (0 != sample.mGPUStartStamp)
         {
#ifdef XBOX
            retiredSample.mGPUStartScaledTime = ScaleProfileSampleTime((sample.mGPUStartStamp - gBProfileSystem.getGPUSessionTimestamp()) * gBProfileSystem.getOOCompCPUFreq() - mStartOfFrameBaseTime);
            retiredSample.mGPUEndScaledTime   = ScaleProfileSampleTime((sample.mGPUEndStamp   - gBProfileSystem.getGPUSessionTimestamp()) * gBProfileSystem.getOOCompCPUFreq() - mStartOfFrameBaseTime);
#else
            retiredSample.mGPUStartScaledTime = ScaleProfileSampleTime((sample.mGPUStartStamp - gBProfileSystem.getGPUSessionTimestamp()) * gBProfileSystem.getOOGPUFreq() - mStartOfFrameBaseTime);
            retiredSample.mGPUEndScaledTime   = ScaleProfileSampleTime((sample.mGPUEndStamp   - gBProfileSystem.getGPUSessionTimestamp()) * gBProfileSystem.getOOGPUFreq() - mStartOfFrameBaseTime);
#endif
         }

         retiredSample.mLevel  = sample.mLevel;
         retiredSample.mUserID = sample.mUserID;

         mpVisualizer->insertSample(sample.mStartOfFrame, mStartOfFrameBaseTime, retiredSample,mThreadID, mFrameNumber);


         sampleIndex = Math::NextWrap<uint>(sampleIndex, MaxSamples);
      }
   }      

   mNextRetiredSample = mNextActiveSample;

#ifdef DUMP_OVERHEAD_TIME_TO_TRACE   
   uint64 endTime = ReadCycleCounter();
   
   static uint64 totalTime;
   totalTime += (endTime - startTime);
   static uint frameCount;
   frameCount++;
   if (frameCount == 60)
   {
      blogtrace("retire: %4.3f ms/frame", totalTime / float(frameCount) * mOOCompCPUFreq * 1000.0f);
      
      frameCount = 0;
      totalTime = 0;
   }
#endif   
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::retireSamples
//---------------------------------------------------------------------------------------------------
void BProfileManager::retireSamples(void)
{
//wtf???
//   resendAllSections();

   const double curCPUTime = ReadCycleCounterNoFlush() * gBProfileSystem.getOOCompCPUFreq();
            
   while (mNextActiveSample != mNextFreeSample)   
   {
      BSample& sample = mSamples[mNextActiveSample];
      
      // stop if sample still active
      if (sample.mLevel & 0x80)
         break;
         
      // if sample is too old, force it to retire
      const double tooOldThresholdTime = .25;
      if ((curCPUTime - (sample.mCPUStartStamp)) > tooOldThresholdTime)
      {
         if (sample.mpStartQuery)
         {
            freeQuery();
            sample.mpStartQuery = NULL;
         }
         
         if (sample.mpEndQuery)
         {
            freeQuery();
            sample.mpEndQuery = NULL;
         }
         
         sample.mGPUStartStamp = 0;
         sample.mGPUEndStamp = 0;
      }
#ifdef XBOX
      else
      {
         if(sample.mpSection->cpuOnly() == false)
         {
            if((sample.mGPUStartStamp != 0) && (sample.mGPUEndStamp == 0))
            {
               break;
            }
         }
      }
#else
      else
      {
         // sample is not too old, check the GPU queries to see if they are ready
         if (sample.mpEndQuery)
         {
            if (sample.mpStartQuery)
            {
               HRESULT hres = sample.mpStartQuery->GetData(&sample.mGPUStartStamp, sizeof(uint64), 0 );
               if (S_OK == hres)
               {
                  freeQuery();
                  sample.mpStartQuery = NULL;
               }
               else
               {
                  break;
               }
            }

            HRESULT hres = sample.mpEndQuery->GetData(&sample.mGPUEndStamp, sizeof(uint64), 0 );
            if (S_OK == hres)
            {
               freeQuery();
               sample.mpEndQuery = NULL;
            }
            else
            {
               break;
            }
         }
      }         
#endif      
      mNextActiveSample = Math::NextWrap<uint>(mNextActiveSample, MaxSamples);
   }
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::clearQueriesAndSamples
//---------------------------------------------------------------------------------------------------
void BProfileManager::clearQueriesAndSamples(void)
{
   mFirstBusyQuery = 0;
   mNextFreeQuery = 0;

   mNextFreeSample = 0;
   mNextRetiredSample = 0;
   mNextActiveSample = 0;

   mSampleStackTop = 0;
   
   mLastRegisteredSectionID = 0;

   memset(mQueryPool, 0, sizeof(mQueryPool));
   memset(mSamples, 0, sizeof(mSamples));
   memset(mSampleStack, 0, sizeof(mSampleStack));  
}

//--------------------------------------------------------------------------------------------------- 
// BProfileManager::getNextQuery
//--------------------------------------------------------------------------------------------------- 
IDirect3DQuery9* BProfileManager::getNextQuery(void)
{
   IDirect3DQuery9* pQuery = mQueryPool[mNextFreeQuery].mpQuery;
   mNextFreeQuery = Math::NextWrap<uint>(mNextFreeQuery, MaxQueries);
   BVERIFY(mNextFreeQuery != mFirstBusyQuery);
   return pQuery;
}

//---------------------------------------------------------------------------------------------------
// BProfileManager::freeQuery
//---------------------------------------------------------------------------------------------------
void BProfileManager::freeQuery(void)
{
   BVERIFY(mFirstBusyQuery != mNextFreeQuery);
   mFirstBusyQuery = Math::NextWrap<uint>(mFirstBusyQuery, MaxQueries);
}


#ifdef XBOX

LARGE_INTEGER start;
uint64 ReadCycleCounterNoFlush(void)
{
   LARGE_INTEGER value;
   QueryPerformanceCounter(&value);
   LONGLONG ticks = value.QuadPart - start.QuadPart;
   return ticks;
}

uint64 ReadCycleCounter(void)
{
   LARGE_INTEGER value;
   QueryPerformanceCounter(&value);
   LONGLONG ticks =  value.QuadPart - start.QuadPart;
   //start = value;
   return ticks;
}
#endif


//---------------------------------------------------------------------------------------------------
// BProfileManagerManager::BProfileManagerManager
//---------------------------------------------------------------------------------------------------
BProfileSystem::BProfileSystem() :
   mbTLSInitialized(false),
   mbEnabled(false),
   mFrameNumber(0),
   mTotalThreadCount(0),
   mbGPUSamplingEnabled(false),
   mbActive(false),
   mbSetActive(false),
   mpFrameVisualizer(NULL)
{
   mSectionEnableEvents.reserve(128);

   //mpDev = NULL;

   //mDummyTest.init();  //enable this line to add test data on multiple frames
}

BProfileSystem::~BProfileSystem()
{
   deinit();
};

//---------------------------------------------------------------------------------------------------
// BProfileSystem::startOfFrame
//---------------------------------------------------------------------------------------------------
void BProfileSystem::startOfFrame(void)
{

   if((!mbEnabled ))// || !mbActive) && !mbSetActive)
      return;
   getManager()->startOfFrame();

   if(mbSetActive == true)
   {
      mbActive = true;
   }
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::endOfFrame
//---------------------------------------------------------------------------------------------------
void BProfileSystem::endOfFrame(void)
{
   if(!mbEnabled  )//|| !mbActive)
      return;
   getManager()->endOfFrame();

#ifdef XBOX
   //InterlockedIncrement64(reinterpret_cast<LONGLONG*>(&mFrameNumber));
   InterlockedIncrement64((LONGLONG*)(&mFrameNumber));
#else
   mSectionCrit.lock();
      mFrameNumber++;  
   mSectionCrit.unlock();
#endif

   processSectionEvents();

   if(mbSetActive == false)
   {
      mbActive = false;
   }
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::registerSection
//---------------------------------------------------------------------------------------------------
void BProfileSystem::registerSection(BProfileSection& section)
{

   mSectionCrit.lock();

   section.setID( static_cast<unsigned short>(mSections.size()) );

   mSections.pushBack(&section);
   if(_strnicmp(section.name() , "Frame",5) == 0)
   {
      mMainFrameID = section.id();
   }

   mSectionCrit.unlock();
}
//---------------------------------------------------------------------------------------------------
// BProfileSystem::startSample
//---------------------------------------------------------------------------------------------------
void BProfileSystem::startSample(BProfileSection& section, bool startOfFrame, uint userID)
{
   if(!mbEnabled)
      return;
   getManager()->startSample(section, startOfFrame, userID);
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::endSample
//---------------------------------------------------------------------------------------------------
void BProfileSystem::endSample(BProfileSection& section)
{
   if(!mbEnabled)
      return;
   getManager()->endSampleCommon(0, false, &section);
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::endSampleID
//---------------------------------------------------------------------------------------------------
void BProfileSystem::endSampleID(BProfileSection& section, uint userID)
{
   if(!mbEnabled)
      return;
   getManager()->endSampleCommon(userID, true, &section);
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::GetManager
//---------------------------------------------------------------------------------------------------
BProfileManager* BProfileSystem::getManager(void)
{
   if(!mbTLSInitialized)
   {
      initTLS();
      //return NULL;
   }

   LPVOID value = TlsGetValue(mTLSIndex);
   BProfileManager* pMgr = NULL;
   if(reinterpret_cast<BProfileManager*>(value) != NULL)
   {
      pMgr = reinterpret_cast<BProfileManager*>(value);
   }
   else
   {
      mManagerCrit.lock();

      //Re-read tls once we get inside the lock.
      //This allows us not lock for most calls to getmanager() and still have safe code.
      value = TlsGetValue(mTLSIndex);
      pMgr = NULL;
      if(reinterpret_cast<BProfileManager*>(value) != NULL)
      {
         pMgr = reinterpret_cast<BProfileManager*>(value);
      }
      else
      {         
         pMgr = allocateTimelineProfiler();
         TlsSetValue(mTLSIndex,pMgr);
      }
      mManagerCrit.unlock();

   }
   return pMgr;
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::initTLS
//---------------------------------------------------------------------------------------------------
void BProfileSystem::initTLS(void)
{
   if(!mbTLSInitialized)
   {
      mTLSIndex = TlsAlloc();
      if(mTLSIndex == TLS_OUT_OF_INDEXES)
      {
         BFAIL("TLS_OUT_OF_INDEXES");
      }
      else
      {
         mbTLSInitialized = true;
      }
   }
}
void BProfileSystem::deinitTLS(void)
{
   if(mbTLSInitialized)
   {
      mbTLSInitialized = false;
      TlsFree(mTLSIndex);
   }
}


void BProfileSystem::testTls(void)
{
   if(isEnabled())
      return;

   int j = 123;
   LPVOID asdf = &j;
   TlsSetValue(mTLSIndex,asdf);
   for (int i=0; i<500; i++)
   {
      LPVOID value = TlsGetValue(mTLSIndex);
      if(123 == (*reinterpret_cast<int*>(value)))
      {
         j++;
      }
   }
}

void BProfileSystem::setDevice(IDirect3DDevice9* pDev)
{
   if(mbEnabled == false)
      return;

   if(mbGPUSamplingEnabled == true)
      return;
   //mpDev = pDev;
   initTLS();
   getManager()->init(pDev);//mpDev);

   mbGPUSamplingEnabled = true;
}

void BProfileSystem::init(void)
{
   initTLS();

   getManager();

   BFPUPrecision fpuPrecision(true);

   // hack hack - try to read comp. CPU freq from user startup dir 
   //extern long cDirUserWriteableGame;                                                  //These are only defined in age3 
   //BString usUserWriteableGameDir = gFileManager.getDirectory(cDirUserWriteableGame);  //These are only defined in age3

   
   uint64 mCompCPUFreq = 0;//3200160008;

#ifndef XBOX
   BString usUserWriteableGameDir( mProfileTimelineDirectory );
   usUserWriteableGameDir.append(B("startup\\timelineprofile.txt"));

   FILE* pFile = fopen(usUserWriteableGameDir.getPtr(), "r");
   if (pFile)
   {
      if (fscanf_s(pFile, "%I64u", &mCompCPUFreq) != 1)
         mCompCPUFreq = 0;

      if (mCompCPUFreq < 500000000)
         mCompCPUFreq = 0;

      fclose(pFile);
   }
#else
   LARGE_INTEGER TicksPerSecond;
   QueryPerformanceFrequency( &TicksPerSecond );
   mCompCPUFreq = TicksPerSecond.QuadPart;
#endif

   bool CpuOnly = true;
   if (mCompCPUFreq)
      CpuOnly = false;
   else 
      mCompCPUFreq = static_cast<uint64>(gPerf.getFrequency());
   mOOCompCPUFreq = 1.0 / double(mCompCPUFreq);
   if (mCompCPUFreq)
   {
      mbEnabled = true;
   }
}
void BProfileSystem::deinit(void)
{
   //if(mpDev == NULL)
   //   return;
   deinitTLS();

   mbEnabled = false;
   mbActive = false;

   mFrameBuffers.clear();
   mProfileManagers.clear();


   for (uint i = 0; i < mProfileManagers.size(); i++)
   {
      (mProfileManagers[i])->deinit();
   }
}
void BProfileSystem::destroy(void)
{
   mbEnabled = false;
   mbActive = false;

   for (uint i = 0; i < mFrameBuffers.size(); i++)
   {
      delete mFrameBuffers[i];
   }
   mFrameBuffers.clear();
   for (uint i = 0; i < mProfileManagers.size(); i++)
   {
      (mProfileManagers[i])->deinit();
      delete mProfileManagers[i];
   }
   mFrameBuffers.clear();
}
//---------------------------------------------------------------------------------------------------
// BProfileManager::resendAllSections
//---------------------------------------------------------------------------------------------------
void BProfileSystem::resendAllSections(void)
{
   //BFAIL("notimplemented");

/*   if (mpVisualizer)
   {
      for (uint i = 0; i < gNextProfileSectionID; i++)
         mpVisualizer->registerSection(static_cast<BSectionID>(i), *gRegisteredProfileSections[i]);
   } */     
}
//============================================================================
// BProfileManagerManager::allocateTimelineProfiler
//============================================================================
BProfileManager* BProfileSystem::allocateTimelineProfiler(void)
{
   if(!mbEnabled)
      return NULL;
   BProfileManager* pProfileManager = new BProfileManager(mTotalThreadCount); 
   BProfileFrameBuffer* pProfileFrameBuffer = new BProfileFrameBuffer();
   pProfileFrameBuffer->setVisualizer(mpFrameVisualizer);
   pProfileManager->setVisualizer(pProfileFrameBuffer);
   mProfileManagers.pushBack(pProfileManager);
   mFrameBuffers.pushBack(pProfileFrameBuffer);
   mTotalThreadCount++;
   return pProfileManager;
}

void BProfileSystem::setVisualizer(BProfileVisualizerInterface* pVisualizer)
{
   mpFrameVisualizer = pVisualizer;
   for (uint i = 0; i < mFrameBuffers.size(); i++)
   {
      //const BRetiredProfileSample& sample = (*pFrame)[i];
      (mFrameBuffers[i])->setVisualizer(pVisualizer);
   }
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::tickTimeLineSystem
//---------------------------------------------------------------------------------------------------
void BProfileSystem::tickTimeLineSystem(void)
{
   if (isEnabled())
   {
      //getManager()->getRetiredSamples();   
      //

      //mDummyThreadTester.init();

   }
   
   processSectionEvents();

}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::enableSection
//---------------------------------------------------------------------------------------------------
void BProfileSystem::enableSection(BSectionID id, bool enable)
{
   if(!isEnabled())
      return;
   mSectionEnableEvents.pushBack(BSectionEnableEvent(id, enable)); 
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::getNumSections
//---------------------------------------------------------------------------------------------------
uint BProfileSystem::getNumSections(void) 
{ 
   if(!isEnabled())
      return 0;
   return mSections.size(); 
}
//---------------------------------------------------------------------------------------------------
// BProfileSystem::getSection
//---------------------------------------------------------------------------------------------------
const BProfileSection* BProfileSystem::getSection(BSectionID id) 
{ 
   BASSERT(id < getNumSections()); 
   return (mSections[id]); 
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::enableAllSections
//---------------------------------------------------------------------------------------------------
void BProfileSystem::enableAllSections(bool enable)
{
   if(!isEnabled())
      return;
   for (uint i = 0; i < getNumSections(); i++)
      enableSection(static_cast<BSectionID>(i), enable);
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::enableSection
//---------------------------------------------------------------------------------------------------
void BProfileSystem::enableSection(const char* pStr, bool enable)
{
   if(!isEnabled())
      return;
   for (BSectionID i = 0; i < getNumSections(); i++)
   {
      const BProfileSection* pSection = getSection(i);//static_cast<BSectionID>(i));
      if (0 == stricmp(pSection->name(), pStr))
         enableSection(pSection->id(), enable);
   }
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::enableSectionSubStr
//---------------------------------------------------------------------------------------------------
void BProfileSystem::enableSectionSubStr(const char* pStr, bool enable)
{
   if(!isEnabled())
      return;
   for (BSectionID i = 0; i < getNumSections(); i++)
   {
      const BProfileSection* pSection = getSection(i);
      if (NULL != strstr(pSection->name(), pStr))
         enableSection(pSection->id(), enable);
   }
}
void BProfileSystem::processSectionEvents(void)
{
   // Process all outstanding section enable/disable events.
   // It's safe to do this here because no sections should be active.
   for (uint i = 0; i < mSectionEnableEvents.size(); i++)
   {
      mSections[mSectionEnableEvents[i].mSectionId]->setEnabled(mSectionEnableEvents[i].mEnabled);  
      //mSections[mSectionEnableEvents[i].mSectionId]->cpuOnly();
   }
   mSectionEnableEvents.resize(0);
}

void BProfileSystem::setProfileDirectory(const BCHAR_T *profiledir)
{
   mProfileTimelineDirectory.set(profiledir);
}

bool BProfileSystem::isEnabled(void)
{
   return mbEnabled;
}

bool BProfileSystem::isActive(void)
{
   return mbActive;
}
void BProfileSystem::setActive(bool value)
{
   mbSetActive = value;
   mbActive = value;
}

bool BProfileSystem::isGPUSamplingEnabled(void)
{
   return mbGPUSamplingEnabled;
}

///////Calibration stuff
//--------------------------------------------------------------------------------------------------- 
// BProfileSystem::getGPUFreq
//--------------------------------------------------------------------------------------------------- 
bool BProfileSystem::getGPUFreq(void)
{
#ifndef XBOX
   BFPUPrecision fpuPrecision(true);

   IDirect3DQuery9* pFreqQuery = NULL;
   HRESULT hres = mpDev->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &pFreqQuery);
   if ((FAILED(hres)) || (!pFreqQuery))
      return true;

   pFreqQuery->Issue(D3DISSUE_END);

   while(S_FALSE == pFreqQuery->GetData(&mGPUFreq, sizeof(uint64), D3DGETDATA_FLUSH))
      ;

   pFreqQuery->Release();

   mOOGPUFreq = 1.0 / double(mGPUFreq);

   return false;

#else
   return false;
#endif
}


//============================================================================
// BProfileSystem::calibrateProfileTimeline
//============================================================================
void BProfileSystem::calibrateProfileTimeline(void)
{

   if (this->isEnabled())
   {
      BFPUPrecision fpuPrecision(true);

      mCompCPUFreq = calculateCompensatedCPUFreq(static_cast<uint64>(gPerf.getFrequency()));
      mOOCompCPUFreq = 1.0 / double(mCompCPUFreq);

      if (mCompCPUFreq)
      {
         // hack hack - write comp. CPU freq to user startup dir 
         //extern long cDirUserWriteableGame;                                                   //These are only defined in age3
         //BString usUserWriteableGameDir = gFileManager.getDirectory(cDirUserWriteableGame);   //These are only defined in age3
         BString usUserWriteableGameDir(mProfileTimelineDirectory);
         usUserWriteableGameDir.append(B("startup\\timelineprofile.txt"));

         FILE* pFile = NULL;
#ifdef XBOX         
         fopen_s(&pFile, usUserWriteableGameDir.getPtr(), "w");
#else
         pFile = fopen(usUserWriteableGameDir.getPtr(), "w");
#endif         
         if (pFile)
         {
            fprintf(pFile, "%I64u\n", mCompCPUFreq);
            fclose(pFile);
         }
      }         
   }
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::calibrate
//---------------------------------------------------------------------------------------------------
void BProfileSystem::calibrate(void)
{
#ifndef XBOX

   BFPUPrecision fpuPrecision(true);

   mGPUQueriesAvailable  = false;

   mCPUSessionTimestamp = ReadCycleCounterNoFlush();                  

   if ((mCPUOnly) || (getGPUFreq()))
      return;

   IDirect3DQuery9* pQuery = NULL;
   HRESULT hres = mpDev->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &pQuery);
   if ((FAILED(hres)) || (!pQuery))
      return;

   uint64 gpuTimestamp;

   pQuery->Issue(D3DISSUE_END);
   while(S_FALSE == pQuery->GetData( &gpuTimestamp, sizeof(uint64), D3DGETDATA_FLUSH ))
      ;

   mCPUSessionTimestamp = ReadCycleCounterNoFlush();                  

   mGPUSessionTimestamp = gpuTimestamp;

   uint128 aveTimeStampDelta(0U);

   uint numSamples = 100;
   for (uint r = 0; r < numSamples; r++)
   {
      pQuery->Issue(D3DISSUE_END);
      while(S_FALSE == pQuery->GetData( &gpuTimestamp, sizeof(uint64), D3DGETDATA_FLUSH ))
         ;

      uint64 cpuTimestamp = ReadCycleCounterNoFlush();

      uint128 cScaled(0U);
      cScaled[1] = (uint)cpuTimestamp;
      cScaled[2] = (uint)(cpuTimestamp >> 32);

      uint128 gScaled(0U);
      gScaled[1] = (uint)gpuTimestamp;
      gScaled[2] = (uint)(gpuTimestamp >> 32);

      cScaled = uint128::divideQuot(cScaled, mCompCPUFreq);
      gScaled = uint128::divideQuot(gScaled, mGPUFreq);

      aveTimeStampDelta += cScaled - gScaled;
   }      

   mScaledGPUTimeDelta = uint128::signedDivideQuot(aveTimeStampDelta, numSamples);

   pQuery->Release();

   uint128 CPUDelta(0U);
   CPUDelta[1] = (uint)mGPUSessionTimestamp;
   CPUDelta[2] = (uint)(mGPUSessionTimestamp >> 32);
   CPUDelta = uint128::divideQuot(CPUDelta, mGPUFreq);
   CPUDelta += mScaledGPUTimeDelta;
   CPUDelta = uint128::signedMultiply(CPUDelta, mCompCPUFreq);
   CPUDelta += uint128(0U, 0U, 0U, 0x80000000);
   CPUDelta.arithShiftRight(32);
   mCPUSessionTimestamp = CPUDelta.getUint64();

   mGPUQueriesAvailable = true;

#endif
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::recalibrate
// returns GPU time delta in seconds scaled by 2^32
//---------------------------------------------------------------------------------------------------
uint64 BProfileSystem::recalibrate(uint64 compCPUFreq)
{
   BFPUPrecision fpuPrecision(true);

   mCompCPUFreq = compCPUFreq;
   mOOCompCPUFreq = 1.0 / compCPUFreq;

   calibrate();

#ifdef XBOX
   return mScaledGPUTimeDelta;
#else
   return mScaledGPUTimeDelta.getUint64();
#endif
}

//---------------------------------------------------------------------------------------------------
// BProfileSystem::calculateCompensatedCPUFreq
//---------------------------------------------------------------------------------------------------
uint64 BProfileSystem::calculateCompensatedCPUFreq(uint64 actualCPUFreq)
{
#ifndef XBOX
   if (!mpDev)
      return 0;

   BFPUPrecision fpuPrecision(true);         

   bool oldCPUOnly = mCPUOnly;
   uint64 oldCPUSessionTimestamp = mCPUSessionTimestamp;

   mCPUOnly = false;

   calibrate();

   if (!mGPUQueriesAvailable)
   {
      mCPUOnly = oldCPUOnly;
      mCPUSessionTimestamp = oldCPUSessionTimestamp;
      return 0;
   }

   HCURSOR hcursor = SetCursor(LoadCursor(0, IDC_WAIT));

   uint64 cpuFreqLow =  actualCPUFreq - 250000000;
   if (cpuFreqLow < 1)
      cpuFreqLow = 1;

   uint64 cpuFreqHigh = actualCPUFreq + 250000000;

   HANDLE hthread = GetCurrentThread();
   HANDLE hprocess = GetCurrentProcess();
   long nOldPriority = GetThreadPriority(hthread);
   DWORD dwOldClass = GetPriorityClass(hprocess);

   SetPriorityClass(hprocess, REALTIME_PRIORITY_CLASS);
   SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL);

   const int N = 2048;
   BLeastSquaresLineFitter<double>::BPoint* samples = new BLeastSquaresLineFitter<double>::BPoint[N];

   for (int j = 0; j < 25; j++)
   {
      uint64 cpuFreqTrial = (cpuFreqLow + cpuFreqHigh) >> 1;

      uint64 firstDelta = recalibrate(cpuFreqTrial);
      uint64 startTime = ReadCycleCounter();

      int i;
      double maxDelta = -999999999999999999999.0;
      for (i = 0; i < N; i++)
      {
         const double delta = (double)((int64)(recalibrate(cpuFreqTrial) - firstDelta));
         samples[i].x = (ReadCycleCounter() - startTime) / double(actualCPUFreq);
         samples[i].y = delta;
         maxDelta = max(maxDelta, delta);
      }

      if (maxDelta != 0.0f)
      {
         for (i = 0; i < N; i++)
            samples[i].y /= fabs(maxDelta);
      }            

      double b1, b2;
      BLeastSquaresLineFitter<double>::fit(b1, b2, samples, N);

      char buf[1024];
      StringCchPrintfA(buf, ( sizeof( buf ) / sizeof( ( buf )[0] ) ), "%i Compensated CPU Frequency: %I64u, Clock Drift: %f\n", j, cpuFreqTrial, b1);
      OutputDebugStringA(buf);

      if (b1 > 0)
      {
         cpuFreqLow = cpuFreqTrial;
      }
      else 
      {
         cpuFreqHigh = cpuFreqTrial;
      }

      Sleep(10);
   }

   delete [] samples;

   SetPriorityClass(hprocess, dwOldClass);
   SetThreadPriority(hthread, nOldPriority);

   SetCursor(hcursor);

   mCPUOnly = oldCPUOnly;

   if (mCPUOnly)
   {
      mGPUQueriesAvailable = false;
      mCPUSessionTimestamp = oldCPUSessionTimestamp;   
      mGPUSessionTimestamp = 0;
      mGPUFreq = 0;
      mOOGPUFreq = 0;
      mScaledGPUTimeDelta = 0U;
   }

   return (cpuFreqLow + cpuFreqHigh + 1) / 2;
#else
   return 0;
#endif
}


//---------------------------------------------------------------------------------------------------
// BProfileFrameBuffer::BProfileFrameBuffer
//---------------------------------------------------------------------------------------------------
BProfileFrameBuffer::BProfileFrameBuffer() :
   mpVisualizer(NULL)
{

}
void BProfileFrameBuffer::insertSample(bool startOfFrame, double frameBaseTime, const BRetiredProfileSample& sample, int threadID, uint64 frameNumber)
{
   if((startOfFrame) && (mNextFrame.size() > 0))
   {
      mCurFrame.swap(mNextFrame);
      if(mpVisualizer)
      {
         //mpVisualizer->insertFrame(static_cast<unsigned long>(gBProfileSystem.getFrameNumber()), mFrameBaseTime, &mCurFrame, gBProfileSystem.getSections(), threadID);
         mpVisualizer->insertFrame((unsigned long)frameNumber, mFrameBaseTime, &mCurFrame, gBProfileSystem.getSections(), threadID);
      }
      mNextFrame.resize(0);
   }
   mFrameBaseTime = frameBaseTime;
   mNextFrame.pushBack(sample); 
}
//BDummyThreadTester/////////////
BDummyThreadTester::BDummyThreadTester()
{
   mNumThreads = 0;

}
BDummyThreadTester::~BDummyThreadTester()
{
   mbActive = false;
}

int BDummyThreadTester::mNumThreads;
bool BDummyThreadTester::mbActive;

void BDummyThreadTester::init()
{
   mInitCrit.lock();
   if(mNumThreads == 0)
   { 
      startThread();
      startThread();
      startThread();
      startThread();
      startThread();
   }
   mInitCrit.unlock();
}

void BDummyThreadTester::startThread()
{
   unsigned int threadID = 0;

   mbActive = true;
   mHelperThread = (HANDLE)_beginthreadex(NULL, 0, &helperThreadProc, NULL, 0, &threadID);


}
void BDummyThreadTester::endThread()
{

}

void FakeStack(int depth)
{
   Sleep(1);
   SCOPEDCPUSAMPLE(FakeStack)
   if(depth > 0)
   {  
      Sleep(1);
      FakeStack(depth-1);
      FakeStack(depth-2);
   }
   Sleep(1);

   int cpuID = 0;

#ifdef XBOX
   cpuID = static_cast<int>(GetCurrentProcessorNumber());
#endif

   if(cpuID == 0)
   {
      SCOPEDCPUSAMPLE(INVALID_CPU_WTF)
      Sleep(5);
   }

}

unsigned int __stdcall BDummyThreadTester::helperThreadProc(void* pArguments)
{
   pArguments;
   int cpuID;
   mNumThreads++;   
   int threadID = mNumThreads;

   cpuID = threadID;

#ifdef XBOX
   XSetThreadProcessor(GetCurrentThread(), cpuID);
#endif

   while (mbActive) 
   {
      uint64 frameid = gBProfileSystem.getFrameNumber();
      if((frameid % 4) == 0) 
      {
         if(threadID == 1)
         {         
            SCOPEDCPUSAMPLE(TestThreadSleep_0_1)
            Sleep(5);
            FakeStack(2);      
         }
         else if(threadID == 2)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_0_2)
            Sleep(2);
            FakeStack(4);
         }
         else if(threadID == 3)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_0_3)
            Sleep(1);
         }
         else if(threadID == 4)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_0_4)
            Sleep(3);
         }
         else if(threadID == 5)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_0_5)
            Sleep(5);
         }
         Sleep(1);

      }
      else if((frameid % 4) == 1) 
      {
         if(threadID == 1)
         {         
            SCOPEDCPUSAMPLE(TestThreadSleep_1_1)
            Sleep(5);
            FakeStack(2);      
         }
         else if(threadID == 2)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_1_2)
            Sleep(2);
            FakeStack(4);
         }
         else if(threadID == 3)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_1_3)
            Sleep(1);
         }
         else if(threadID == 4)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_1_4)
            Sleep(3);
         }
         else if(threadID == 5)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_1_5)
            Sleep(5);
         }
         Sleep(1);
      }
      else if((frameid % 4) == 2) 
      {
         if(threadID == 1)
         {         
            SCOPEDCPUSAMPLE(TestThreadSleep_2_1)
            Sleep(5);
            FakeStack(2);      
         }
         else if(threadID == 2)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_2_2)
            Sleep(2);
            FakeStack(4);
         }
         else if(threadID == 3)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_2_3)
            Sleep(1);
         }
         else if(threadID == 4)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_2_4)
            Sleep(3);
         }
         else if(threadID == 5)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_2_5)
            Sleep(5);
         }
         Sleep(1);
      }
      else if((frameid % 4) == 3) 
      {
         if(threadID == 1)
         {         
            SCOPEDCPUSAMPLE(TestThreadSleep_3_1)
            Sleep(5);
            FakeStack(2);      
         }
         else if(threadID == 2)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_3_2)
            Sleep(2);
            FakeStack(4);
         }
         else if(threadID == 3)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_3_3)
            Sleep(1);
         }
         else if(threadID == 4)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_3_4)
            Sleep(3);
         }
         else if(threadID == 5)
         {
            SCOPEDCPUSAMPLE(TestThreadSleep_3_5)
            Sleep(5);
         }
         Sleep(1);
      }
#ifdef XBOX
      cpuID = static_cast<int>(GetCurrentProcessorNumber());
#endif
   }    

   return 0;
}



#else
uint gTimeLineProfilerDummy;
#endif
