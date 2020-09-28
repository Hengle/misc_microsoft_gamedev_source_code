//============================================================================
//
//  timelineprofilersample.h
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#pragma once

#ifdef XBOX

   #if defined(BUILD_DEBUG)

   // rg [1/28/08] - Enabling these configs was breaking several Xbox tools - they wouldn't link due to missing network funcs
   //#define ENABLE_TIMELINE_PROFILER

   #elif defined(BUILD_PLAYTEST)

   #elif defined(BUILD_PROFILE)

   #define ENABLE_TIMELINE_PROFILER
   //#define ENABLE_GPU_PROFILER

   #else  

   #endif
   
#else

#endif   


#if defined(ENABLE_TIMELINE_PROFILER) 

//#include "timelineprofiler.h"

// Types
class BProfileSystem;
class BProfileManager;
class BProfileSection;

// Globals
extern BProfileSystem gBProfileSystem;

// Profile section class.

// Very important: BProfileSection must be global and can never go out of scope! 
class BProfileSection
{
public:
   BProfileSection(const char* pName, bool cpuOnly);// : mpName(pName), mCPUOnly(cpuOnly), mEnabled(true), mID(static_cast<unsigned short>(gBProfileSystem.getNumSections()))) { gBProfileSystem.registerSection(*this); }
         
   const char* name(void) const           { return mpName; }
   bool        cpuOnly(void) const        { return mCPUOnly; }
   bool        enabled(void) const        { return mEnabled; }
   unsigned short id(void) const          { return mID; }
         
   //void        setName(const char* pName) { mpName = pName; }
   //void        setCPUOnly(bool cpuOnly)   { mCPUOnly = cpuOnly; }
   void        setEnabled(bool enabled)   { mEnabled = enabled; }
   void        setID(unsigned short id)   { mID = id; }
   
private:
   bool           mEnabled;
   bool           mCPUOnly;
   unsigned short mID;
   const char*    mpName;
               
   // undefined
   BProfileSection();
   BProfileSection(const BProfileSection& rhs);
   BProfileSection& operator= (const BProfileSection& rhs);
};

// ProfileManagerStartSample(), ProfileManagerEndSample(), and ProfileManagerEndSampleID() are the primary functions to start/end samples.

// Scoped sample class.
class BScopedSample
{
   BProfileSection& mSection;
public:
   BScopedSample(BProfileSection& section, unsigned int userID = 0);
   ~BScopedSample();
   //BScopedSample(BProfileSection& section, unsigned int userID = 0) : mSection(section) { if (gBProfileSystem.isEnabled() && (section.enabled())) gBProfileSystem.startSample(section, userID); }
   //~BScopedSample() { if (gBProfileSystem.isEnabled() && (mSection.enabled())) gBProfileSystem.endSample(mSection); }
};

//
//inline BScopedSample::BScopedSample(BProfileSection& section, unsigned int userID) : mSection(section) { if (gBProfileSystem.isEnabled() && (section.enabled())) gBProfileSystem.startSample(section, userID); }
//inline BScopedSample::~BScopedSample() { if (gBProfileSystem.isEnabled() && (mSection.enabled())) gBProfileSystem.endSample(mSection); }


// Helper macros.
#define SCOPEDSAMPLE(name) static BProfileSection __gTProfSection##name(#name, false); BScopedSample __TProfSample##name(__gTProfSection##name); BScopedPIXNamedEvent PIXNamedEvent(#name);
#define SCOPEDCPUSAMPLE(name) static BProfileSection __gTProfSection##name(#name, true); BScopedSample __TProfSample##name(__gTProfSection##name); BScopedPIXNamedEvent PIXNamedEvent(#name);

#define SCOPEDSAMPLEID(name, id) static BProfileSection __gTProfSection##name(#name, false); BScopedSample __TProfSample##name(__gTProfSection##name, id); BScopedPIXNamedEvent PIXNamedEvent(#name, id);
#define SCOPEDCPUSAMPLEID(name, id) static BProfileSection __gTProfSection##name(#name, true); BScopedSample __TProfSample##name(__gTProfSection##name, id); BScopedPIXNamedEvent PIXNamedEvent(#name, id);

#else

#define SCOPEDSAMPLE(name) BScopedPIXNamedEvent PIXNamedEvent(#name);
#define SCOPEDCPUSAMPLE(name) BScopedPIXNamedEvent PIXNamedEvent(#name);

#define SCOPEDSAMPLEID(name, id) BScopedPIXNamedEvent PIXNamedEvent(#name, id);
#define SCOPEDCPUSAMPLEID(name, id) BScopedPIXNamedEvent PIXNamedEvent(#name, id);

#endif   
