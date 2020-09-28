//============================================================================
//
//  timelineprofiler.h
//  
//  Copyright (c) 2005, Ensemble Studios
//
//  Also see timelineprofilersample.h.  Most modules can just include 
//  timelineprofilersample.h and not this module, as it defines everything you
//  need to begin/end samples.
//============================================================================
#pragma once

#if defined(ENABLE_TIMELINE_PROFILER)


#ifndef XBOX
#include "uint128.h"
#endif

#include "timelineprofilersample.h"
#include "fpuprecision.h"

typedef ushort BSectionID;

class BProfileSystem;
class BProfileFrameBuffer;
class BDummyThreadTester;

//---------------------------------------------------------------------------------------------------
// Helpers to convert from time to scaled time and vice versa
// .1 microsecond res.
// Don't set the res too low (i.e. TimeToScaledTimeMul too high) because the scaled times should fit into 32 bits
//---------------------------------------------------------------------------------------------------
const double TimeToScaledTimeMul = 10000000.0f;
const double OOTimeToScaledTimeMul = 1.0f / TimeToScaledTimeMul;

inline uint ScaleProfileSampleTime(double t)
{
   // Ugh casts suck. Try SSE2? Or magic const?
   return static_cast<uint>(t * TimeToScaledTimeMul);
}

inline double UnscaleProfileSampleTime(uint t)
{
   return t * OOTimeToScaledTimeMul;
}

//---------------------------------------------------------------------------------------------------
// struct BRetiredProfileSample - Retired profile sample struct
//---------------------------------------------------------------------------------------------------
struct BRetiredProfileSample
{
   // Scaled CPU start/end times 
   uint mCPUStartScaledTime;
   uint mCPUEndScaledTime;
   
   // GPU start/end times - if 0, no GPU information is available for this sample
   uint mGPUStartScaledTime;
   uint mGPUEndScaledTime;           
   
   // User provided ID (from perfEndID for example).
   uint mUserID;                 
   
   // Nesting level.
   uchar mLevel;                 
         
   BSectionID mSectionID;
   
   // True if the GPU times are valid -- they could still be invalid even if the CPU section is NOT marked as CPU only!
   bool hasGPUTimes(void) const
   {
      return mGPUStartScaledTime != mGPUEndScaledTime;
   }
   
   // Non-basetime versions for speed reasons (done every retired sample)      
   double getCPUStartTime(double baseTime)   const { return UnscaleProfileSampleTime(mCPUStartScaledTime) + baseTime; }
   double getCPUStartTime(void)              const { return UnscaleProfileSampleTime(mCPUStartScaledTime); }

   double getCPUEndTime(double baseTime)     const { return UnscaleProfileSampleTime(mCPUEndScaledTime) + baseTime; }
   double getCPUEndTime(void)                const { return UnscaleProfileSampleTime(mCPUEndScaledTime); }

   double getGPUStartTime(double baseTime)   const { return UnscaleProfileSampleTime(mGPUStartScaledTime) + baseTime; }
   double getGPUStartTime(void)              const { return UnscaleProfileSampleTime(mGPUStartScaledTime); }

   double getGPUEndTime(double baseTime)     const { return UnscaleProfileSampleTime(mGPUEndScaledTime) + baseTime; }
   double getGPUEndTime(void)                const { return UnscaleProfileSampleTime(mGPUEndScaledTime); }
};

//---------------------------------------------------------------------------------------------------
// class BProfileVisualizerInterface - Defines interface used by the timeline profiler manager
// to communicate sections/samples to a visualizer.
//---------------------------------------------------------------------------------------------------
class BProfileVisualizerInterface
{
public:
   BProfileVisualizerInterface() { };
   virtual ~BProfileVisualizerInterface() { };
   
   virtual void registerSection(BSectionID id, const BProfileSection& profSection) { id; profSection; };
   virtual void insertSample(bool startOfFrame, double frameBaseTime, const BRetiredProfileSample& sample, int threadID, uint64 frameNumber)  { startOfFrame, frameBaseTime; sample; threadID; frameNumber;};
   virtual void insertFrame(unsigned long frameNumber, double frameBaseTime, BDynamicArray<BRetiredProfileSample> *pFrame, const BDynamicArray<BProfileSection*> *pIdentifiers, int threadID){frameBaseTime;pFrame;pIdentifiers;threadID;frameNumber;};

};

//---------------------------------------------------------------------------------------------------
// class BProfileManager - Timeline profiler manager.
// Only one instance of this class may be active.
//---------------------------------------------------------------------------------------------------
class BProfileManager
{
public:
   //BProfileManager(/*double OOCompCPUFreq, double OOGPUFreq, bool GPUQueriesAvailable, bool cpuOnly*/);
   BProfileManager(int threadID);
   ~BProfileManager();
      
   void init(IDirect3DDevice9* pDev);
   void deinit(void);

   // startSample/endSample may be called multiple times for a given sampleID.
   // This method does not check if the section is enabled!
   // Note: ProfileManagerStartSample(), ProfileManagerEndSample(), and ProfileManagerEndSampleID() are the primary functions to start/end samples.
   void startSample(BProfileSection& sampleID, bool startOfFrame = false, uint userID = 0);
   
   // endSample always pops off the previous startSample (that's why sampleID isn't needed).
   // These methods do not check if the section is enabled!
   void endSample(void) { endSampleCommon(0, false); }
   void endSampleID(uint userID) { endSampleCommon(userID, true); } 
   void endSampleCommon(uint userID, bool hasUserID, BProfileSection* pSection = NULL);

   void startOfFrame(void);
   void endOfFrame(void);
   
   void getRetiredSamples(void);
   
   // Sets/gets the current visualizer
   // Note: If you set a new visualizer, you should probably resend all registered profile sections (resendAllSections()).
   void setVisualizer(BProfileVisualizerInterface* pVisualizer) { mpVisualizer = pVisualizer; mLastRegisteredSectionID = 0; }
   BProfileVisualizerInterface* getVisualizer(void) const { return mpVisualizer; }
      
   void retireSamples(void);
   
private:
   enum { MaxQueries = 4096 };

	struct BQuery
	{
		IDirect3DQuery9* mpQuery;
	};
	
	
	struct BSample
	{
		BProfileSection* mpSection;

		IDirect3DQuery9* mpStartQuery;
		IDirect3DQuery9* mpEndQuery;

		// FIXME: 64-bit samples here are probably overkill/unnecessary!
		uint64 mCPUStartStamp;
		uint64 mCPUEndStamp;
		uint64 mGPUStartStamp;
		uint64 mGPUEndStamp;

		uint mUserID;

		// Copy of section id so the retirement step doesn't need to fetch this id from the section structs.
		ushort mSectionID;   

		// MSB of level is set of sample is active
		uchar mLevel; 

		bool mStartOfFrame;
	};


	enum { MaxSamples = 65536 };
	enum { SampleStackSize = 128 };
	

	BSample mSamples[MaxSamples];
	BQuery mQueryPool[MaxQueries];
	int mSampleStack[SampleStackSize];

   uint64 mStartOfFrameCPUStamp;
	double mStartOfFrameBaseTime;
	
	uint mNextFreeQuery;
	uint mFirstBusyQuery;

	uint mNextFreeSample;
	uint mNextActiveSample;
	uint mNextRetiredSample;

	uint mSampleStackTop;
	uint mLastRegisteredSectionID;

   int mThreadID;

	IDirect3DDevice9* mpDev;
	BProfileVisualizerInterface* mpVisualizer;

   void              clearQueriesAndSamples(void);

   bool              anyQueriesAvailable(void) const { return Math::NextWrap<uint>(mNextFreeQuery, MaxQueries) != mFirstBusyQuery; }
   IDirect3DQuery9*  getNextQuery(void);

   void              freeQuery(void);
   
   uint64 mFrameNumber;

   // undefined
   BProfileManager(const BProfileManager&);
   BProfileManager& operator= (const BProfileManager&);
};

#ifdef XBOX

uint64 ReadCycleCounterNoFlush(void);
uint64 ReadCycleCounter(void);

#endif

class BDummyThreadTester
{
public:
   BDummyThreadTester();
   ~BDummyThreadTester();

   void init();
   void startThread();
   void endThread();
   HANDLE mHelperThread;


   static unsigned int __stdcall helperThreadProc(void* pArguments);
   static int mNumThreads;
   static bool mbActive;

   BCriticalSection mInitCrit;


};

//---------------------------------------------------------------------------------------------------
// class BProfileSystem - Main interface to the real time profiling system
//
//---------------------------------------------------------------------------------------------------
class BProfileSystem
{
public:
   BProfileSystem();
   ~BProfileSystem();

   bool isEnabled(void);
   bool isGPUSamplingEnabled(void);
   bool isActive(void);  
   void setActive(bool value);
   void setVisualizer(BProfileVisualizerInterface* pVisualizer);
   void startOfFrame(void);
   void endOfFrame(void);
   void registerSection(BProfileSection& section);
   void startSample(BProfileSection& section, bool startOfFrame = false, uint userID = 0);
   void endSample(BProfileSection& section);
   void endSampleID(BProfileSection& section, uint userID);
   void testTls(void);
   void setDevice(IDirect3DDevice9* pDev);
   void init(void);
   void deinit(void);
   void destroy(void);
   void tickTimeLineSystem(void);
   void resendAllSections(void);
   void setProfileDirectory(const BCHAR_T *profiledir);

   // Enables/disables a section. Important: The section is not instantly enabled/disabled. This can't be done while samples are outstanding.
   // Instead, enable/disable requests are queued up and processed at the end of a frame.
   uint getNumSections(void) ;//const ;
   void enableAllSections(bool enable);
   void enableSection(BSectionID id, bool enable);   
   void enableSection(const char* pStr, bool enable);   
   void enableSectionSubStr(const char* pStr, bool enable);

//public calibration info
   bool areGPUQueriesAvailable(){ return mGPUQueriesAvailable;}
   double getOOCompCPUFreq(){return mOOCompCPUFreq;}
   double getOOGPUFreq(){return mOOGPUFreq;}
   uint64 getCPUSessionTimestamp(){return mCPUSessionTimestamp;}
   uint64 getGPUSessionTimestamp(){return mGPUSessionTimestamp;}

   const BDynamicArray<BProfileSection*> *getSections(){return &mSections;};

   uint64 getFrameNumber(){return mFrameNumber;}

   unsigned short mMainFrameID;

private:
   BDummyThreadTester mDummyTest;


   BProfileVisualizerInterface* mpFrameVisualizer;
   BProfileManager* allocateTimelineProfiler(void);
   void initTLS(void);
   void deinitTLS(void);
   BProfileManager* getManager(void);
   void processSectionEvents(void);   

   //section stuff
   const BProfileSection* getSection(BSectionID id) ;//const ;

   
   bool mbEnabled;
   bool mbGPUSamplingEnabled;
   bool mbActive;
   bool mbSetActive;
   BDynamicArray< BProfileManager*> mProfileManagers;
   BDynamicArray< BProfileFrameBuffer*> mFrameBuffers;
   DWORD mTLSIndex;
   bool mbTLSInitialized;
   BString mProfileTimelineDirectory;  
   IDirect3DDevice9* mpDev;

   uint64 mFrameNumber;


   struct BSectionEnableEvent
   {
   	BSectionID mSectionId;
   	bool              mEnabled;
   	BSectionEnableEvent() { }
   	BSectionEnableEvent(BSectionID sectionID, bool enabled) : mSectionId(sectionID), mEnabled(enabled) { }
   };
   BCriticalSection mSectionCrit;
   BCriticalSection mProfileCrit;
   BCriticalSection mManagerCrit;

   BDynamicArray<BSectionEnableEvent> mSectionEnableEvents;
   BDynamicArray<BProfileSection*> mSections;


//Calibration
#ifdef XBOX
   uint64 mScaledGPUTimeDelta; // it may not be worth the time to use uint128 on xbox...
#else
   uint128 mScaledGPUTimeDelta; // seconds multiplied by 2^32
#endif
   uint64 mCPUSessionTimestamp;
   uint64 mGPUSessionTimestamp;
   uint64 mCompCPUFreq;
   uint64 mGPUFreq;
   double mOOGPUFreq;
   double mOOCompCPUFreq;
   bool mCPUOnly;
   bool mGPUQueriesAvailable;

   int mTotalThreadCount;

   void calibrateProfileTimeline(void);
   // Returns GPU time delta in seconds scaled by 2^32.
   uint64 recalibrate(uint64 compCPUFreq);
   // Calibrates the profiler - this will take awhile! 
   uint64 calculateCompensatedCPUFreq(uint64 actualCPUFreq);   
   // Returns 1.0/compensated cpu frequency.
   double getOOCompCPUFreq(void) const { return mOOCompCPUFreq; }
   bool              getGPUFreq(void);
   void              calibrate(void);

   // undefined
   BProfileSystem(const BProfileSystem&);
   BProfileSystem& operator= (const BProfileSystem&);


   BDummyThreadTester mDummyThreadTester;


};




class BProfileFrameBuffer : public BProfileVisualizerInterface
{
public:
   BProfileFrameBuffer();
   void setVisualizer(BProfileVisualizerInterface* pVisualizer) { mpVisualizer = pVisualizer;}
   BProfileVisualizerInterface* getVisualizer(void) const { return mpVisualizer; }
   virtual void insertSample(bool startOfFrame, double frameBaseTime, const BRetiredProfileSample& sample, int threadID, uint64 frameNumber);
private:
   BDynamicArray<BRetiredProfileSample> mCurFrame;
   BDynamicArray<BRetiredProfileSample> mNextFrame;
   BProfileVisualizerInterface* mpVisualizer;
   double mFrameBaseTime;

//   uint64 mFrameNumber;
};

//---------------------------------------------------------------------------------------------------
// BProfileManagerFrameProfHelper
//---------------------------------------------------------------------------------------------------
class BProfileManagerFrameProfHelper
{
public:
   BProfileManagerFrameProfHelper()
   {
      gBProfileSystem.startOfFrame();
   }

   ~BProfileManagerFrameProfHelper()
   {  
      SCOPEDSAMPLE(LastSample)

      gBProfileSystem.endOfFrame();
   }
};

inline BScopedSample::BScopedSample(BProfileSection& section, unsigned int userID) : mSection(section) { if (gBProfileSystem.isActive() && (section.enabled())) gBProfileSystem.startSample(section, false, userID); }
inline BScopedSample::~BScopedSample() { if (gBProfileSystem.isActive() && (mSection.enabled())) gBProfileSystem.endSample(mSection); }
inline BProfileSection::BProfileSection(const char* pName, bool cpuOnly) : mpName(pName), mCPUOnly(cpuOnly), mEnabled(true) { gBProfileSystem.registerSection(*this); }


inline void ProfileManagerStartSample(BProfileSection& section, unsigned int userID = 0){ if (gBProfileSystem.isActive() && (section.enabled())) gBProfileSystem.startSample(section, false, userID); }
inline void ProfileManagerEndSample(BProfileSection& section) { if (gBProfileSystem.isActive() && (section.enabled())) gBProfileSystem.endSample(section); }
inline void ProfileManagerEndSampleID(BProfileSection& section, unsigned int userID) { if (gBProfileSystem.isEnabled() && (section.enabled())) gBProfileSystem.endSampleID(section, userID); }

#else
#define ProfileManagerStartSample __noop
#define ProfileManagerEndSample __noop
#define ProfileManagerEndSampleID __noop
#endif

