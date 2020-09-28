//============================================================================
// particledatatracker.h
// Ensemble Studios (C) 2006
//============================================================================
#pragma once

#include "Settings.h"

class BParticleSystemData;
class BParticleDataTracker : public BSettings
{
   public:
      enum 
      {
         //-- Data Stats
         cEmitterDataCount = 0,

         cTiedToEmitterCount,
         cIgnoreRotationCount,

         cPalleteColorCount,
         cAppearanceWeightCount,
         cDepthBiasWeightCount,
         cDepthBiasFileCount,
         cOpacityStageCount,
         cScaleStageCount,
         cSpeedStageCount,
         cColorStageCount,
         cCollisionTypeCount,
         cEventCount,
         cIntensityStageCount,

         //-- Run Time Stats
         cRunTimeNumParticles,
         cRunTimeNumEmitters,
         cTotalDataCount
      };

      BParticleDataTracker();
     ~BParticleDataTracker();

      void increment(int type, int value = 1);
      void decrement(int type, int value = 1);

#if 0
      void loadAllData();
      void processData(BParticleSystemData* pData);
#endif      
};
extern BParticleDataTracker gParticleDataTracker;

