//==============================================================================
//  File: particleheap.cpp
//
//  Copyright (c) 2000-2006 Ensemble Studios
//==============================================================================
#include "xparticlescommon.h"
#include "ParticleSystemManager.h"
#include "memory\allocationLogger.h"

#pragma warning(disable: 4073)
#pragma init_seg(lib)
#pragma warning(default: 4073)

#ifdef USE_SEPERATE_PARTICLE_HEAP
BParticleHeapType gParticleHeap(cDefaultHeapType, true, "Particle", false, NULL, false);
#endif

BParticleHeapType gParticleBlockHeap(cDefaultHeapType, true, "ParticleBlock", false, NULL, false, cAllocLogTypeParticles);