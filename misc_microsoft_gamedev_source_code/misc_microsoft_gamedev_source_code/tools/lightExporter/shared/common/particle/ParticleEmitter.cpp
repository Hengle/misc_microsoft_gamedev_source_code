// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "ParticleCommon.h"
#include "Particles.h"

#include "common/property/PropertyFile.h"
#include "common/geom/unigeom.h"

using namespace gr;

// Methods.

namespace 
{
	const D3DXVECTOR3 gDefaultDir(0, 1, 0);
}

ParticleEmitter::ParticleEmitter()
{
	direction = gDefaultDir; 
	directionInfluence = 1;
	flowRate = 1;
	lifeTime = 1;
	rotation = 0;
	rotationVariance = 0;
	shapeRadius = 0;
	spread = 0;
	velocity = 1;
	velocityVariance = 0;
	
	points.push_back(D3DXVECTOR3(0, 0, 0));
	normals.push_back(gDefaultDir);
	D3DXMatrixIdentity(&followObject);

	age = 0;
	leftoverTime = 0;
	enabled = true;
	intensity = 1.0f;
}

void ParticleEmitter::initShapeSource(const std::string& meshFilename)
{
	std::auto_ptr<Stream> pStream = std::auto_ptr<Stream>(gFileSystem.createStream(meshFilename.c_str()));
	
	if (pStream.get())
	{
		Unigeom::Geom geom;
		
		*pStream >> geom;
		
		if ((geom.numMaterials() < 1) || (pStream->errorStatus()))
		{
			Error("ParticleEmitter::initShapeSource: Can't load geometry file \"%s\" as a shape source!\n", meshFilename.c_str());
		}
		else
		{		
			for (int i = 0; i < geom.numVerts(); i++)
			{
				const Univert& vert = geom.vert(i);
				
				// EVIL: Exporter should use similarity xform!!
				const Vec4 p(vert.p * Matrix44::makeMaxToD3D());
				const Vec4 n(vert.n * Matrix44::makeMaxToD3D());
				
				points.push_back(D3DXVECTOR3(p[0], p[1], p[2]));
				normals.push_back(D3DXVECTOR3(n[0], n[1], n[2]));									
			}
		}
	}
}

ParticleEmitter::ParticleEmitter(const PropertySet &propertySet)
{
	direction = ReadVector3(propertySet, "Direction", gDefaultDir);
	directionInfluence = ReadNumeric(propertySet, "DirectionInfluence", 1);
	flowRate = ReadNumeric(propertySet, "FlowRate", 1);
	lifeTime = ReadNumeric(propertySet, "LifeTime", 1);
	rotation = ReadNumeric(propertySet, "Rotation", 0);
	rotationVariance = ReadNumeric(propertySet, "RotationVariance", 0);
	shapeRadius = ReadNumeric(propertySet, "ShapeRadius", 0);
	spread = ReadNumeric(propertySet, "Spread", 0) * D3DX_PI / 180.0f;
	velocity = ReadNumeric(propertySet, "Velocity", 1);
	velocityVariance = ReadNumeric(propertySet, "VelocityVariance", 0);
	
	unsigned int poolSize =	ReadNumericInt(propertySet, "PoolSize", 1);
	pool.SetPoolSize(poolSize);
	
	points.clear();
	normals.clear();
		
	if (propertySet.HasPropertyValue("ShapeSource"))
	{
		std::string meshFilename(ReadString(propertySet, "ShapeSource", ""));
		
		if (!meshFilename.empty())
			initShapeSource(meshFilename);
	}
	else if (propertySet.HasPropertyValue("Shape"))
	{
		PropertyValue propertyValue = propertySet.GetPropertyValue("Shape");
						
		unsigned int elements = propertyValue.GetElementCount();
		for (unsigned int element = 0; element < elements; ++element)
		{
			std::vector<float> vector = propertyValue.GetVector(element);
			size_t s = vector.size();
			float replicate = vector.at(vector.size() - 1);
			while (vector.size() < 6)
				vector.push_back(replicate);
			
			points.push_back(D3DXVECTOR3(vector[0], vector[1], vector[2]));
			normals.push_back(D3DXVECTOR3(vector[3], vector[4], vector[5]));
			
			float v[6] = {
				vector[0], vector[1], vector[2],
				vector[3], vector[4], vector[5],
			};
			s = vector.size();
		}
	}
	
	if (points.empty())
	{
		points.push_back(D3DXVECTOR3(0, 0, 0));
		normals.push_back(gDefaultDir);
	}
	
	D3DXMatrixIdentity(&followObject);

	age = 0;
	leftoverTime = 0;
	enabled = true;
	intensity = 1.0f;
}

ParticleEmitter::ParticleEmitter(const ParticleEmitter &emitter)
	: pool(emitter.pool)
{
	direction = emitter.direction;
	directionInfluence = emitter.directionInfluence;
	flowRate = emitter.flowRate;
	lifeTime = emitter.lifeTime;
	rotation = emitter.rotation;
	rotationVariance = emitter.rotationVariance;
	shapeRadius = emitter.shapeRadius;
	spread = emitter.spread;
	velocity = emitter.velocity;
	velocityVariance = emitter.velocityVariance;

	points = emitter.points;
	normals = emitter.normals;
	followObject = emitter.followObject;

	age = emitter.age;
	leftoverTime = emitter.leftoverTime;
	enabled = emitter.enabled;
	intensity = emitter.intensity;
}

ParticleEmitter & ParticleEmitter::operator =(const ParticleEmitter &emitter)
{
	pool = emitter.pool;

	direction = emitter.direction;
	directionInfluence = emitter.directionInfluence;
	flowRate = emitter.flowRate;
	lifeTime = emitter.lifeTime;
	rotation = emitter.rotation;
	rotationVariance = emitter.rotationVariance;
	shapeRadius = emitter.shapeRadius;
	spread = emitter.spread;
	velocity = emitter.velocity;
	velocityVariance = emitter.velocityVariance;

	points = emitter.points;
	normals = emitter.normals;
	followObject = emitter.followObject;

	age = emitter.age;
	leftoverTime = emitter.leftoverTime;
	enabled = emitter.enabled;
	intensity = emitter.intensity;

	return *this;
}

ParticleEmitter::~ParticleEmitter()
{
}

void ParticleEmitter::Update(float elapsedTime)
{
	age += elapsedTime;
	
	if (lifeTime == 0 || age < lifeTime)
	{
		float combinedTime = elapsedTime + leftoverTime;
		int births = static_cast<int>(floor(combinedTime * flowRate));
		leftoverTime = combinedTime - (static_cast<float>(births) / flowRate);
		
		if (!enabled)
			return;
		
		while (births > 0)
		{
			--births;
			Particle *particle = pool.GetNextAvailable();
			if (particle != NULL)
			{
				pool.Prototype().Apply(particle);
				particle->age = elapsedTime;
				
				unsigned int index = random.NextInteger();
				size_t s = points.size();

				particle->position = points[index % points.size()];
				if (shapeRadius != 0)
				{
					D3DXVECTOR3 offset = D3DXVECTOR3(
						random.NextFloat(-1.0f, 1.0f),
						random.NextFloat(-1.0f, 1.0f),
						random.NextFloat(-1.0f, 1.0f)
					);
					D3DXVec3Normalize(&offset, &offset);
					particle->position += offset * random.NextFloat(0, shapeRadius);
				}
				D3DXVec3TransformCoord(
					&particle->position,
					&particle->position,
					&followObject
				);
				
				D3DXVECTOR3 emitDir;
				
				if (directionInfluence == 1)
					emitDir = direction;
				else if (directionInfluence == 0)
					emitDir = normals[index % normals.size()];
				else
				{
					emitDir = normals[index % normals.size()];
					D3DXVECTOR3 cross;
					D3DXVec3Cross(&cross, &emitDir, &direction);
					float dot = D3DXVec3Dot(&emitDir, &direction);
					D3DXMATRIX rotate;
					D3DXMatrixRotationAxis(
						&rotate,
						&cross,
						directionInfluence * acos(dot)
					);
					D3DXVec3TransformNormal(&emitDir, &emitDir, &rotate);
				}
				D3DXVec3Normalize(&emitDir, &emitDir);
				
				float theta = D3DX_PI * random.NextFloat(-1, 1);
				float phi = spread * random.NextFloat();
				float cosTheta = cos(theta), sinTheta = sin(theta);
				float cosPhi = cos(phi), sinPhi = sin(phi);
				D3DXVECTOR3 perturb = D3DXVECTOR3(
					cosTheta * sinPhi, sinTheta * sinPhi, cosPhi
				);
				D3DXVECTOR3 unitZ = D3DXVECTOR3(0, 0, 1), cross;
				D3DXVec3Cross(&cross, &unitZ, &emitDir);
				float dot = D3DXVec3Dot(&unitZ, &emitDir);
				D3DXMATRIX rotate;
				D3DXMatrixRotationAxis(&rotate, &cross, acos(dot));
				D3DXMatrixMultiply(&rotate, &rotate, &followObject);
				D3DXVec3TransformNormal(&particle->velocity, &perturb, &rotate);
				particle->velocity *= velocity +
					random.NextFloat(-velocityVariance, velocityVariance);
				
				particle->orientation = 0;
				particle->rotation = rotation +
					random.NextFloat(-rotationVariance, rotationVariance);
				particle->intensity = intensity;
			}
		}
	}
}