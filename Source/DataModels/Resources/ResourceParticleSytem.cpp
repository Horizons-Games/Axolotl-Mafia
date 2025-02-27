#include "StdAfx.h"

#include "ResourceParticleSystem.h"
#include "ParticleSystem/ParticleEmitter.h"


ResourceParticleSystem::ResourceParticleSystem(UID resourceUID, const std::string& fileName, 
	const std::string& assetsPath, const std::string& libraryPath) : 
	Resource(resourceUID, fileName, assetsPath, libraryPath)
{
}

ResourceParticleSystem::~ResourceParticleSystem()
{
}
