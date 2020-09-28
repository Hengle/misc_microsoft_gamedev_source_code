// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "ParticleCommon.h"
#include "common/property/PropertyFile.h"
#include "Particles.h"

// Methods.

ParticlePool::ParticlePool()
{
	SetPoolSize(1);

	next = 0;
}

ParticlePool::ParticlePool(const ParticlePool &pool)
	: prototype(pool.prototype), particles(pool.particles)
{
	next = pool.next;
}

ParticlePool & ParticlePool::operator =(const ParticlePool &pool)
{
	prototype = pool.prototype;
	particles = pool.particles;

	next = pool.next;

	return *this;
}

ParticlePool::~ParticlePool()
{
}

unsigned int ParticlePool::GetPoolSize() const
{
	return static_cast<unsigned int>(particles.size());
}

void ParticlePool::SetPoolSize(unsigned int poolSize)
{
	particles.resize(poolSize);
	std::vector<Particle>::iterator iter = particles.begin();
	for (; iter != particles.end(); ++iter)
		iter->age = DEAD;
}

const std::vector<Particle> & ParticlePool::GetParticles() const
{
	return particles;
}

Particle * ParticlePool::GetNextAvailable()
{
	Particle *particle = &particles[next];
	next = (next + 1) % GetPoolSize();
	return particle;
}

void ParticlePool::ApplyForces(const ParticleForces &forces)
{
	std::vector<Particle>::iterator iter = particles.begin();
	for (; iter != particles.end(); ++iter)
	{
		if (iter->age == DEAD)
			continue;
		
		iter->forces = D3DXVECTOR3(0, 0, 0);
		iter->forces += iter->gravityFactor * forces.GetGravity();
		iter->forces += iter->windFactor * forces.GetWind();
	}
}

void ParticlePool::Update(float elapsedTime)
{
	std::vector<Particle>::iterator iter = particles.begin();
	for (; iter != particles.end(); ++iter)
	{
		if (iter->age == DEAD)
			continue;
		
		iter->age += elapsedTime;
		if (iter->age >= iter->lifeTime)
		{
			iter->age = DEAD;
			continue;
		}
		
		iter->position += iter->forces * elapsedTime * elapsedTime / 2.0f;
		iter->position += iter->velocity * elapsedTime;
		iter->velocity += iter->forces * elapsedTime;
		iter->orientation += iter->rotation * elapsedTime;
		
		float delta = iter->age / iter->lifeTime;
		float attackFactor = (delta < prototype.Attack())
			? 1.0f - (delta / prototype.Attack()) : 0;
		float decayFactor = (1.0f - delta < prototype.Decay())
			? 1.0f - ((1.0f - delta) / prototype.Decay()) : 0;
		
		attackFactor = (1.0f - cos(attackFactor * D3DX_PI)) / 2.0f;
		decayFactor = (1.0f - cos(decayFactor * D3DX_PI)) / 2.0f;
		
		iter->alpha = prototype.SustainAlpha() +
			(attackFactor * (prototype.InitialAlpha() - prototype.SustainAlpha())) +
			(decayFactor * (prototype.FinalAlpha() - prototype.SustainAlpha()));
		iter->color = prototype.SustainColor() +
			(attackFactor * (prototype.InitialColor() - prototype.SustainColor())) +
			(decayFactor * (prototype.FinalColor() - prototype.SustainColor()));
		iter->scale = iter->sustainScale +
			(attackFactor * (iter->initialScale - iter->sustainScale)) +
			(decayFactor * (iter->finalScale - iter->sustainScale));
		
		int frame = static_cast<int>(floor(delta * prototype.AnimationRepeat()
			* prototype.AnimationLength()));
		frame %= prototype.AnimationLength();
		
		int row = frame / prototype.AnimationLayoutCols();
		row %= prototype.AnimationLayoutRows();
		int col = frame % prototype.AnimationLayoutCols();
		
		iter->uvLeft = static_cast<float>(col)
			/ static_cast<float>(prototype.AnimationLayoutCols());
		iter->uvTop = static_cast<float>(row)
			/ static_cast<float>(prototype.AnimationLayoutRows());
		iter->uvRight = iter->uvLeft + (1.0f
			/ static_cast<float>(prototype.AnimationLayoutCols()));
		iter->uvBottom = iter->uvTop + (1.0f
			/ static_cast<float>(prototype.AnimationLayoutRows()));
	}
}