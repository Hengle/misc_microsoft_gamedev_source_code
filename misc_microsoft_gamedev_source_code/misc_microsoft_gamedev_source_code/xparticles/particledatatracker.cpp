//============================================================================
// particledatatracker.cpp
// Ensemble Studios (C) 2006
//============================================================================
#include "xparticlescommon.h"
#include "particledatatracker.h"
#include "workdirsetup.h"
#include "particleworkdirsetup.h"
#include "FileManager.h"
#include "ParticleSystemData.h"
#include "dataentry.h"
#include "settings.h"

BParticleDataTracker gParticleDataTracker;

#define P(type) {BParticleDataTracker::type, B(#type), BDataEntry::cTypeLong, 0},
#define INCDATA(type) {increment(BParticleDataTracker::type);}

BSettingDef gParticleDataTrackerDefinition[BParticleDataTracker::cTotalDataCount]=
{
   P(cEmitterDataCount)
   P(cTiedToEmitterCount)
   P(cIgnoreRotationCount)
   P(cPalleteColorCount)
   P(cAppearanceWeightCount)
   P(cDepthBiasWeightCount)
   P(cDepthBiasFileCount)
   P(cOpacityStageCount)
   P(cScaleStageCount)
   P(cSpeedStageCount)
   P(cColorStageCount)
   P(cCollisionTypeCount)
   P(cEventCount)
   P(cIntensityStageCount)   
   P(cRunTimeNumParticles)
   P(cRunTimeNumEmitters)
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleDataTracker::BParticleDataTracker() :
   BSettings(gParticleDataTrackerDefinition, BParticleDataTracker::cTotalDataCount)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BParticleDataTracker::~BParticleDataTracker()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleDataTracker::increment(int type, int value)
{
   BDEBUG_ASSERT(type < BParticleDataTracker::cTotalDataCount);

   long curValue;
   getLong(type, curValue);
   curValue+=value;
   setLong(type, curValue);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleDataTracker::decrement(int type, int value)
{
   BDEBUG_ASSERT(type < BParticleDataTracker::cTotalDataCount);

   long curValue;
   getLong(type, curValue);
   curValue-=value;
   setLong(type, curValue);
}

#undef P
#undef INCDATA
