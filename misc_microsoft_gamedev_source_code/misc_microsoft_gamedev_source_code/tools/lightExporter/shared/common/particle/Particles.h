// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _PARTICLES_H_
#define _PARTICLES_H_

#include "ParticleCommon.h"

// Defines.

//#define FRAMERATELOCK		30
#define DEAD				-1

// Classes.

struct Particle
{
	float		age;
	D3DXVECTOR3	position;
	float		orientation;
	D3DXVECTOR3	velocity;
	float		rotation;
	D3DXVECTOR3 forces;
	
	float		finalScale;
	float		gravityFactor;
	float		initialScale;
	float		lifeTime;
	float		sustainScale;
	float		windFactor;
	
	float		alpha;
	D3DXVECTOR3 color;
	float		scale;
	float		uvLeft;
	float		uvTop;
	float		uvRight;
	float		uvBottom;
	float		intensity;
};

class ParticlePrototype
{
	private:
		unsigned int	animationLayoutCols;
		unsigned int	animationLayoutRows;
		unsigned int	animationLength;
		float			animationRepeat;
		float			attack;
		float			decay;
		float			finalAlpha;
		D3DXVECTOR3		finalColor;
		float			finalScale;
		float			finalScaleVariance;
		float			gravityFactor;
		float			gravityFactorVariance;
		float			initialAlpha;
		D3DXVECTOR3		initialColor;
		float			initialScale;
		float			initialScaleVariance;
		float			lifeTime;
		float			lifeTimeVariance;
		float			sustainAlpha;
		D3DXVECTOR3		sustainColor;
		float			sustainScale;
		float			sustainScaleVariance;
		std::string		textureSource;
		float			windFactor;
		float			windFactorVariance;
		
		Random			random;
		
		static D3DXVECTOR3 ToLinearLight(const D3DXVECTOR3& color);
					
	public:
		ParticlePrototype();
		ParticlePrototype(const PropertySet &propertySet);
		
		ParticlePrototype(const ParticlePrototype &prototype);
		ParticlePrototype & operator =(const ParticlePrototype &prototype);
		~ParticlePrototype();
		
		unsigned int &	AnimationLayoutCols()	{ return animationLayoutCols; };
		unsigned int &	AnimationLayoutRows()	{ return animationLayoutRows; };
		unsigned int &	AnimationLength()		{ return animationLength; };
		float &			AnimationRepeat()		{ return animationRepeat; };
		float &			Attack()				{ return attack; };
		float &			Decay()					{ return decay; };
		float &			FinalAlpha()			{ return finalAlpha; };
		D3DXVECTOR3 &	FinalColor()			{ return finalColor; };
		float &			FinalScale()			{ return finalScale; };
		float &			FinalScaleVariance()	{ return finalScaleVariance; };
		float &			GravityFactor()			{ return gravityFactor; };
		float &			GravityFactorVariance()	{ return gravityFactorVariance; };
		float &			InitialAlpha()			{ return initialAlpha; };
		D3DXVECTOR3 &	InitialColor()			{ return initialColor; };
		float &			InitialScale()			{ return initialScale; };
		float &			InitialScaleVariance()	{ return initialScaleVariance; };
		float &			LifeTime()				{ return lifeTime; };
		float &			LifeTimeVariance()		{ return lifeTimeVariance; };
		float &			SustainAlpha()			{ return sustainAlpha; };
		D3DXVECTOR3 &	SustainColor()			{ return sustainColor; };
		float &			SustainScale()			{ return sustainScale; };
		float &			SustainScaleVariance()	{ return sustainScaleVariance; };
		std::string &	TextureSource()			{ return textureSource; };
		float &			WindFactor()			{ return windFactor; };
		float &			WindFactorVariance()	{ return windFactorVariance; };
		
		void Apply(Particle *particle);
};

class ParticleForces
{
	private:
		D3DXVECTOR3	gravityDirection;
		float		gravityForce;
		float		windDirection;
		float		windDirectionVariance;
		float		windSpeed;
		float		windSpeedVariance;
		float		windTurbulance;
		
		float		windTheta1;
		float		windTheta2;
		float		windPhi1;
		float		windPhi2;
		float		windPsi1;
		float		windPsi2;
		float		windDelta;
		
		D3DXVECTOR3 gravity;
		D3DXVECTOR3 wind;

		Random		random;
		
	public:
		ParticleForces();
		ParticleForces(const PropertySet &propertySet);
		
		ParticleForces(const ParticleForces &forces);
		ParticleForces & operator =(const ParticleForces &forces);
		~ParticleForces();
		
		D3DXVECTOR3 &	GravityDirection()	{ return gravityDirection; };
		float &			GravityForce()		{ return gravityForce; };

		float &			WindDirection()			{ return windDirection; };
		float &			WindDirectionVariance()	{ return windDirectionVariance; };
		float &			WindSpeed()				{ return windSpeed; };
		float &			WindSpeedVariance()		{ return windSpeedVariance; };
		float &			WindTurbulance()		{ return windTurbulance; };

		D3DXVECTOR3	GetGravity()	const { return gravity; };
		D3DXVECTOR3	GetWind()		const { return wind; };

		void Update(float elapsedTime);
};

class ParticlePool
{
	private:
		ParticlePrototype		prototype;
		std::vector<Particle>	particles;
		unsigned int			next;
	
	public:
		ParticlePool();
		
		ParticlePool(const ParticlePool &pool);
		ParticlePool & operator =(const ParticlePool &pool);
		~ParticlePool();
		
		ParticlePrototype & Prototype() { return prototype; };
		
		unsigned int GetPoolSize() const;
		void SetPoolSize(unsigned int poolSize);
		
		const std::vector<Particle> & GetParticles() const;
		Particle * GetNextAvailable();

		void ApplyForces(const ParticleForces &forces);
		void Update(float elapsedTime);
};

class ParticleEmitter
{
	private:
		D3DXVECTOR3					direction;
		float						directionInfluence;
		float						flowRate;
		float						lifeTime;
		float						rotation;
		float						rotationVariance;
		float						shapeRadius;
		float						spread;
		float						velocity;
		float						velocityVariance;
		float						intensity;

		std::vector<D3DXVECTOR3>	points;
		std::vector<D3DXVECTOR3>	normals;
		D3DXMATRIX					followObject;
		
		ParticlePool				pool;
		float						age;
		float						leftoverTime;
		Random						random;
		
		bool enabled;
		
		void initShapeSource(const std::string& meshFilename);
	
	public:
		ParticleEmitter();
		ParticleEmitter(const PropertySet &propertySet);
		
		ParticleEmitter(const ParticleEmitter &emitter);
		ParticleEmitter & operator =(const ParticleEmitter &emitter);
		~ParticleEmitter();

		D3DXVECTOR3 &	Direction()				{ return direction; };
		float &			DirectionInfluence()	{ return directionInfluence; };
		float &			FlowRate()				{ return flowRate; };
		float &			LifeTime()				{ return lifeTime; };
		float &			Rotation()				{ return rotation; };
		float &			RotationVariance()		{ return rotationVariance; };
		float &			ShapeRadius()			{ return shapeRadius; };
		float &			Spread()				{ return spread; };
		float &			Velocity()				{ return velocity; };
		float &			VelocityVariance()		{ return velocityVariance; };
		bool& Enabled() { return enabled; }
		float &			Intensity() { return intensity; }

		std::vector<D3DXVECTOR3> &	Points()		{ return points; };
		std::vector<D3DXVECTOR3> &	Normals()		{ return normals; };
		D3DXMATRIX &				FollowObject()	{ return followObject; };

		ParticlePool &	Pool()	{ return pool; };

		void Update(float elapsedTime);
};

class ParticleSystem
{
	private:
		ParticleEmitter		emitter;
		ParticleForces		forces;
		
		ParticleEmitter initialEmitter;
		ParticleForces initialForces;
		
		float				leftoverTime;
					
	public:
		ParticleSystem();
		ParticleSystem(const PropertyFile &propertyFile);
		
		ParticleSystem(const ParticleSystem &system);
		ParticleSystem & operator =(const ParticleSystem &system);
		~ParticleSystem();
		
		ParticleEmitter &	Emitter()	{ return emitter; };
		ParticleForces &	Forces()	{ return forces; };
		
		void Update(float elapsedTime);
		void Reset(void);
};

class ParticleSystemManager
{
	private:
		typedef std::map<std::string, ParticleSystem> ParticleSystemMap;
		ParticleSystemMap systemMap;
		
		typedef std::set<ParticleSystem *> ParticleSystemSet;
		ParticleSystemSet instances;
		
	public:
		ParticleSystemManager()
		{
		}
		
		void DefineSystem(const std::string &name, const ParticleSystem &system);
		
		ParticleSystem * CreateInstance(const std::string &name);
		void DeleteInstance(ParticleSystem * &system);
		void DeleteAllInstances();
		void DeleteAllSystems();
		
		// deletes all instances/systems
		void Clear();

		std::vector<ParticleSystem *> GetAllInstances();
		void UpdateAllInstances(float elapsedTime);
		
		static ParticleSystemManager* CreateParticleSystemManager(void)
		{
			return new ParticleSystemManager();
		}
};

#endif /* _PARTICLES_H_ */
