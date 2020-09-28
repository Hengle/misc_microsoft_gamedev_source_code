// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "Particles.h"

// Methods.

ParticleSystem::ParticleSystem()
{
	leftoverTime = 0;
}

ParticleSystem::ParticleSystem(const PropertyFile &propertyFile)
{
	if (propertyFile.HasPropertySet("Emitter"))
		emitter = ParticleEmitter(propertyFile.GetPropertySet("Emitter"));
	if (propertyFile.HasPropertySet("Emitter"))
		emitter.Pool().Prototype() = ParticlePrototype(propertyFile.GetPropertySet("Prototype"));
	if (propertyFile.HasPropertySet("Forces"))
		forces = ParticleForces(propertyFile.GetPropertySet("Forces"));

	initialEmitter = emitter;
	initialForces = forces;
	
	leftoverTime = 0;
}

ParticleSystem::ParticleSystem(const ParticleSystem &system)
	: emitter(system.emitter), forces(system.forces)
{
	leftoverTime = system.leftoverTime;
	
	initialEmitter = emitter;
	initialForces = forces;
}

ParticleSystem & ParticleSystem::operator =(const ParticleSystem &system)
{
	emitter = system.emitter;
	forces = system.forces;
	leftoverTime = system.leftoverTime;
	
	initialEmitter = emitter;
	initialForces = forces;
	
	return *this;
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::Update(float elapsedTime)
{
#ifdef FRAMERATELOCK

	const float lockStep = 1.0f / FRAMERATELOCK;
	const float lockError = 1.0f - (lockStep * FRAMERATELOCK);
	
	elapsedTime *= FRAMERATELOCK;
	elapsedTime += leftoverTime;
	while (elapsedTime >= 1)
	{
		forces.Update(lockStep);
		emitter.Pool().ApplyForces(forces);
		emitter.Pool().Update(lockStep);
		emitter.Update(lockStep);
		
		--elapsedTime;
	}
	leftoverTime = elapsedTime;

#else /* FRAMERATELOCK */

	forces.Update(elapsedTime);
	emitter.Pool().ApplyForces(forces);
	emitter.Pool().Update(elapsedTime);
	emitter.Update(elapsedTime);
	
#endif /* FRAMERATELOCK */
}

void ParticleSystem::Reset(void)
{
	emitter = initialEmitter;
	forces = initialForces;
}
