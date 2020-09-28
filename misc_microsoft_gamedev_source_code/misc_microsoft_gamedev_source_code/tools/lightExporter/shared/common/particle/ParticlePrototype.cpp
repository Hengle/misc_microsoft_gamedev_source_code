// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "ParticleCommon.h"
#include "common/property/PropertyFile.h"
#include "Particles.h"

// Methods.

ParticlePrototype::ParticlePrototype()
{
	animationLayoutCols = 1;
	animationLayoutRows = 1;
	animationLength = 1;
	animationRepeat = 1;
	attack = 0;
	decay = 0;
	finalAlpha = 1;
	finalColor = D3DXVECTOR3(1, 1, 1);
	finalScale = 1;
	finalScaleVariance = 0;
	gravityFactor = 0;
	gravityFactorVariance = 0;
	initialAlpha = 1;
	initialColor = D3DXVECTOR3(1, 1, 1);
	initialScale = 1;
	initialScaleVariance = 0;
	lifeTime = 1;
	lifeTimeVariance = 0;
	sustainAlpha = 1;
	sustainColor = D3DXVECTOR3(1, 1, 1);
	sustainScale = 1;
	sustainScaleVariance = 0;
	textureSource.clear();
	windFactor = 0;
	windFactorVariance = 0;
}

D3DXVECTOR3 ParticlePrototype::ToLinearLight(const D3DXVECTOR3& color)
{
	return D3DXVECTOR3(color.x * color.x, color.y * color.y, color.z * color.z);
}

ParticlePrototype::ParticlePrototype(const PropertySet &propertySet)
{
	std::vector<unsigned int> animationLayout(2);
	animationLayout[0] = animationLayout[1] = 1;
	animationLayout = ReadVectorIntSized(propertySet, "AnimationLayout", animationLayout, 2);
	animationLayoutCols = animationLayout[0];
	animationLayoutRows = animationLayout[1];
	animationLength = ReadNumericInt(propertySet, "AnimationLength", 1);
	animationRepeat = ReadNumeric(propertySet, "AnimationRepeat", 1);
	attack = ReadNumeric(propertySet, "Attack", 0);
	decay = ReadNumeric(propertySet, "Decay", 0);
	finalAlpha = ReadNumeric(propertySet, "FinalAlpha", 1);
	finalColor = ToLinearLight(ReadVector3(propertySet, "FinalColor", D3DXVECTOR3(1, 1, 1)));
	finalScale = ReadNumeric(propertySet, "FinalScale", 1);
	finalScaleVariance = ReadNumeric(propertySet, "FinalScaleVariance", 0);
	gravityFactor = ReadNumeric(propertySet, "GravityFactor", 0);
	gravityFactorVariance = ReadNumeric(propertySet, "GravityFactorVariance", 0);
	initialAlpha = ReadNumeric(propertySet, "InitialAlpha", 1);
	initialColor = ToLinearLight(ReadVector3(propertySet, "InitialColor", D3DXVECTOR3(1, 1, 1)));
	initialScale = ReadNumeric(propertySet, "InitialScale", 1);
	initialScaleVariance = ReadNumeric(propertySet, "InitialScaleVariance", 0);
	lifeTime = ReadNumeric(propertySet, "LifeTime", 1);
	lifeTimeVariance = ReadNumeric(propertySet, "LifeTimeVariance", 0);
	sustainAlpha = ReadNumeric(propertySet, "SustainAlpha", 1);
	sustainColor = ToLinearLight(ReadVector3(propertySet, "SustainColor", D3DXVECTOR3(1, 1, 1)));
	sustainScale = ReadNumeric(propertySet, "SustainScale", 1);
	sustainScaleVariance = ReadNumeric(propertySet, "SustainScaleVariance", 0);
	textureSource = ReadString(propertySet, "TextureSource", "");
	windFactor = ReadNumeric(propertySet, "WindFactor", 0);
	windFactorVariance = ReadNumeric(propertySet, "WindFactorVariance", 0);
}

ParticlePrototype::ParticlePrototype(const ParticlePrototype &prototype)
{
	animationLayoutCols = prototype.animationLayoutCols;
	animationLayoutRows = prototype.animationLayoutRows;
	animationLength = prototype.animationLength;
	animationRepeat = prototype.animationRepeat;
	attack = prototype.attack;
	decay = prototype.decay;
	finalAlpha = prototype.finalAlpha;
	finalColor = prototype.finalColor;
	finalScale = prototype.finalScale;
	finalScaleVariance = prototype.finalScaleVariance;
	gravityFactor = prototype.gravityFactor;
	gravityFactorVariance = prototype.gravityFactorVariance;
	initialAlpha = prototype.initialAlpha;
	initialColor = prototype.initialColor;
	initialScale = prototype.initialScale;
	initialScaleVariance = prototype.initialScaleVariance;
	lifeTime = prototype.lifeTime;
	lifeTimeVariance = prototype.lifeTimeVariance;
	sustainAlpha = prototype.sustainAlpha;
	sustainColor = prototype.sustainColor;
	sustainScale = prototype.sustainScale;
	sustainScaleVariance = prototype.sustainScaleVariance;
	textureSource = prototype.textureSource;
	windFactor = prototype.windFactor;
	windFactorVariance = prototype.windFactorVariance;
}

ParticlePrototype & ParticlePrototype::operator =(const ParticlePrototype &prototype)
{
	animationLayoutCols = prototype.animationLayoutCols;
	animationLayoutRows = prototype.animationLayoutRows;
	animationLength = prototype.animationLength;
	animationRepeat = prototype.animationRepeat;
	attack = prototype.attack;
	decay = prototype.decay;
	finalAlpha = prototype.finalAlpha;
	finalColor = prototype.finalColor;
	finalScale = prototype.finalScale;
	finalScaleVariance = prototype.finalScaleVariance;
	gravityFactor = prototype.gravityFactor;
	gravityFactorVariance = prototype.gravityFactorVariance;
	initialAlpha = prototype.initialAlpha;
	initialColor = prototype.initialColor;
	initialScale = prototype.initialScale;
	initialScaleVariance = prototype.initialScaleVariance;
	lifeTime = prototype.lifeTime;
	lifeTimeVariance = prototype.lifeTimeVariance;
	sustainAlpha = prototype.sustainAlpha;
	sustainColor = prototype.sustainColor;
	sustainScale = prototype.sustainScale;
	sustainScaleVariance = prototype.sustainScaleVariance;
	textureSource = prototype.textureSource;
	windFactor = prototype.windFactor;
	windFactorVariance = prototype.windFactorVariance;

	return *this;
}

ParticlePrototype::~ParticlePrototype()
{
}

void ParticlePrototype::Apply(Particle *particle)
{
	particle->finalScale = finalScale;
	particle->gravityFactor = gravityFactor;
	particle->initialScale = initialScale;
	particle->lifeTime = lifeTime;
	particle->sustainScale = sustainScale;
	particle->windFactor = windFactor;
	
	particle->finalScale += random.NextFloat(-finalScaleVariance, finalScaleVariance);
	particle->gravityFactor += random.NextFloat(-gravityFactorVariance, gravityFactorVariance);
	particle->initialScale += random.NextFloat(-initialScaleVariance, initialScaleVariance);
	particle->lifeTime += random.NextFloat(-lifeTimeVariance, lifeTimeVariance);
	particle->sustainScale += random.NextFloat(-sustainScaleVariance, sustainScaleVariance);
	particle->windFactor += random.NextFloat(-windFactorVariance, windFactorVariance);
	
	particle->alpha = initialAlpha;
	particle->color = initialColor;
	particle->scale = particle->initialScale;

	particle->uvLeft = 0;
	particle->uvTop = 0;
	particle->uvRight = particle->uvLeft + (1.0f
		/ static_cast<float>(animationLayoutCols));
	particle->uvBottom = particle->uvTop + (1.0f
		/ static_cast<float>(animationLayoutRows));
}