// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "ParticleCommon.h"
#include "common/property/PropertyFile.h"
#include "Particles.h"

// Methods.

namespace
{
	const D3DXVECTOR3 gDefaultGravDir(0, -1, 0);
}

ParticleForces::ParticleForces()
{
	gravityDirection = gDefaultGravDir; 
	gravityForce = 0;
	windDirection = 0;
	windDirectionVariance = 0;
	windSpeed = 0;
	windSpeedVariance = 0;
	windTurbulance = 0;
	
	windTheta1 = 0;
	windTheta2 = 0;
	windPhi1 = 0;
	windPhi2 = 0;
	windPsi1 = 0;
	windPsi2 = 0;
	windDelta = 2; // Forces new random wind offsets.
}

ParticleForces::ParticleForces(const PropertySet &propertySet)
{
	gravityDirection = ReadVector3(propertySet, "GravityDirection", gDefaultGravDir);
	gravityForce = ReadNumeric(propertySet, "GravityForce", 0);
	windDirection = ReadNumeric(propertySet, "WindDirection", 0) * D3DX_PI / 180.0f;
	windDirectionVariance = ReadNumeric(propertySet, "WindDirectionVariance", 0);
	windSpeed = ReadNumeric(propertySet, "WindSpeed", 0);
	windSpeedVariance = ReadNumeric(propertySet, "WindSpeedVariance", 0);
	windTurbulance = ReadNumeric(propertySet, "WindTurbulance", 0);
	
	windTheta1 = 0;
	windTheta2 = 0;
	windPhi1 = 0;
	windPhi2 = 0;
	windPsi1 = 0;
	windPsi2 = 0;
	windDelta = 2; // Forces new random wind offsets.
}

ParticleForces::ParticleForces(const ParticleForces &forces)
{
	gravityDirection = forces.gravityDirection;
	gravityForce = forces.gravityForce;
	windDirection = forces.windDirection;
	windDirectionVariance = forces.windDirectionVariance;
	windSpeed = forces.windSpeed;
	windSpeedVariance = forces.windSpeedVariance;
	windTurbulance = forces.windTurbulance;
	
	windTheta1 = forces.windTheta1;
	windTheta2 = forces.windTheta2;
	windPhi1 = forces.windPhi1;
	windPhi2 = forces.windPhi2;
	windPsi1 = forces.windPsi1;
	windPsi2 = forces.windPsi2;
	windDelta = forces.windDelta;
}

ParticleForces & ParticleForces::operator =(const ParticleForces &forces)
{
	gravityDirection = forces.gravityDirection;
	gravityForce = forces.gravityForce;
	windDirection = forces.windDirection;
	windDirectionVariance = forces.windDirectionVariance;
	windSpeed = forces.windSpeed;
	windSpeedVariance = forces.windSpeedVariance;
	windTurbulance = forces.windTurbulance;

	windTheta1 = forces.windTheta1;
	windTheta2 = forces.windTheta2;
	windPhi1 = forces.windPhi1;
	windPhi2 = forces.windPhi2;
	windPsi1 = forces.windPsi1;
	windPsi2 = forces.windPsi2;
	windDelta = forces.windDelta;

	return *this;
}

ParticleForces::~ParticleForces()
{
}

void ParticleForces::Update(float elapsedTime)
{
	// Gravity.
	
	gravity = gravityDirection * gravityForce;
	
	// Wind.

	windDelta += fabs(elapsedTime * windTurbulance);
	if (windDelta >= 1)
	{
		if (windDelta >= 2)
		{
			windTheta1 = D3DX_PI * random.NextFloat(-1, 1);
			windPhi1 = windDirectionVariance * random.NextFloat();
			windPsi1 = random.NextFloat(-1, 1);
		}
		else
		{
			windTheta1 = windTheta2;
			windPhi1 = windPhi2;
			windPsi1 = windPsi2;
		}

		windTheta2 = D3DX_PI * random.NextFloat(-1, 1);
		windPhi2 = windDirectionVariance * random.NextFloat();
		windPsi2 = random.NextFloat(-1, 1);
		
		windDelta = windDelta - floor(windDelta);
	}

	float theta = windTheta1 + ((windTheta2 - windTheta1) * windDelta);
	float phi = windPhi1 + ((windPhi2 - windPhi1) * windDelta);
	float psi = windPsi1 + ((windPsi2 - windPsi1) * windDelta);

	wind = D3DXVECTOR3(cos(windDirection), 0, sin(windDirection));

	float cosTheta = cos(theta), sinTheta = sin(theta);
	float cosPhi = cos(phi), sinPhi = sin(phi);
	D3DXVECTOR3 perturb = D3DXVECTOR3(
		cosTheta * sinPhi, sinTheta * sinPhi, cosPhi
	);
	D3DXVECTOR3 unitZ = D3DXVECTOR3(0, 0, 1), cross;
	D3DXVec3Cross(&cross, &unitZ, &wind);
	float dot = D3DXVec3Dot(&unitZ, &wind);
	D3DXMATRIX rotate;
	D3DXMatrixRotationAxis(&rotate, &cross, acos(dot));
	D3DXVec3TransformNormal(&wind, &perturb, &rotate);
	wind *= windSpeed + (psi * windSpeedVariance);
}
