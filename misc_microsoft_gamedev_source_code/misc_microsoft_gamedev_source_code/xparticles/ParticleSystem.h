//============================================================================
//
//  ParticleSystem.h
//
//  Copyright (c) 2000 Ensemble Studios
//
//============================================================================
#pragma once

//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
//class BParticleSystemInstance2;
class BParticleEmitter;
class BParticleSystemParticle;
class BParticleSystemManager;

//----------------------------------------------------------------------------
//  Callback Prototypes
//----------------------------------------------------------------------------
typedef bool (BPS_NEW_PARTICLE_FUNC)   (BParticleEmitter* pSystem, BParticleSystemParticle* pParticle, void* pParam);
typedef void (BPS_KILL_PARTICLE_FUNC)  (BParticleEmitter* pSystem, BParticleSystemParticle* pParticle, void* pParam);
typedef bool (BPS_PARTICLE_UPDATE_FUNC)(BParticleEmitter* pSystem, BParticleSystemParticle* pParticle, long lElapsedTime, void* pParam);


//----------------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------------
//#include "ParticleSystemData.h"
#include "particleeffectdata.h"
#include "ParticleEmitter.h"
#include "ParticleSystemParticle.h"

