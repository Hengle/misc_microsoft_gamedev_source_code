// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "ParticleCommon.h"
#include "common/property/PropertyFile.h"
#include "Particles.h"

// Static methods.

void ParticleSystemManager::DefineSystem(const std::string &name, const ParticleSystem &system)
{
	typedef std::pair<std::string, ParticleSystem> KeyPair;
	typedef std::pair<ParticleSystemMap::iterator, bool> InsertResult;
	
	InsertResult insertResult = systemMap.insert(KeyPair(name, system));
	
	if (!insertResult.second)
		insertResult.first->second = system;
}

ParticleSystem * ParticleSystemManager::CreateInstance(const std::string &name)
{
	ParticleSystemMap::const_iterator find = systemMap.find(name);
	
	if (find == systemMap.end())
		return NULL;
	
	ParticleSystem *system = new ParticleSystem(find->second);
	instances.insert(system);
	
	return system;
}

void ParticleSystemManager::DeleteInstance(ParticleSystem * &system)
{
	instances.erase(system);
	SAFE_DELETE(system);
}

void ParticleSystemManager::DeleteAllInstances()
{
	ParticleSystemSet::const_iterator iter = instances.begin();
	for (; iter != instances.end(); ++iter)
		delete (*iter);
	instances.clear();
}

void ParticleSystemManager::DeleteAllSystems()
{
	systemMap.clear();
}

void ParticleSystemManager::Clear(void)
{
	DeleteAllInstances();
	
	DeleteAllSystems();
}

std::vector<ParticleSystem *> ParticleSystemManager::GetAllInstances()
{
	std::vector<ParticleSystem *> systems;
	ParticleSystemSet::const_iterator iter = instances.begin();
	for (; iter != instances.end(); ++iter)
		systems.push_back(*iter);
	
	return systems;
}

void ParticleSystemManager::UpdateAllInstances(float elapsedTime)
{
	ParticleSystemSet::const_iterator iter = instances.begin();
	for (; iter != instances.end(); ++iter)
		(*iter)->Update(elapsedTime);
}