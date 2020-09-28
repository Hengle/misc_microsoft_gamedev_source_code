//==============================================================================
// configsparticles.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "xparticlescommon.h"
#include "configsparticles.h"

//==============================================================================
// Defines
//==============================================================================
DEFINE_CONFIG(cConfigDisableParticleDistanceFade);
DEFINE_CONFIG(cConfigEmitterFadeStartDistance);
DEFINE_CONFIG(cConfigEmitterFadeEndDistance);

//==============================================================================
// BConfigsInput::registerConfigs
//==============================================================================
static bool registerParticleConfigs(bool)
{
   DECLARE_CONFIG(cConfigDisableParticleDistanceFade,   "DisableParticleDistanceFade",  "", 0, NULL);
   DECLARE_CONFIG(cConfigEmitterFadeStartDistance,      "EmitterFadeStartDistance",  "", 0, NULL);
   DECLARE_CONFIG(cConfigEmitterFadeEndDistance,        "EmitterFadeEndDistance",  "", 0, NULL);

   return true;
}

// This causes xcore to call registerParticleConfigs() after it initializes.
#pragma data_seg(".ENS$XIU") 
BXCoreInitFuncPtr gpRegisterParticleConfigs[] = { registerParticleConfigs };
#pragma data_seg() 
