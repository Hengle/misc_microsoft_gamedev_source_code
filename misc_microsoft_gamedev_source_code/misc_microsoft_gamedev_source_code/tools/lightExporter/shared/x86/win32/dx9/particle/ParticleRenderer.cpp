// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.
#include "ParticleRenderer.h"

#include "common/particle/ParticleCommon.h"
#include "common/property/PropertyFile.h"
#include "common/particle/Particles.h"

#include "x86/win32/dx9/render/render_engine.h"
#include "x86/win32/dx9/render/render_helpers.h"
#include "x86/win32/dx9/render/device.h"

namespace gr 
{
	namespace
	{
		const float gSceneParticleIntensity = 8.0f;
		const float gOOSceneParticleIntensity = 1.0f / gSceneParticleIntensity;
		const float gParticleColorAccumScale = 3.0f;
	}

	// Methods.

	ParticleRenderer::ParticleRenderer() : 
		mLoadedTextures(false),
		mClearBuffer(false)
	{
	}

	ParticleRenderer::~ParticleRenderer()
	{
		deleteDeviceObjects();
	}

	void ParticleRenderer::oneTimeSceneInit(void)
	{
		mEffects.setBasename(RenderEngine::effectPath() + "particles");	
	}

	void ParticleRenderer::loadTextures(ParticleSystemManager* pManager)
	{
		mLoadedTextures = true;
		
		std::vector<ParticleSystem *> systems = pManager->GetAllInstances();
		std::vector<ParticleSystem *>::const_iterator iter = systems.begin();
		for (; iter != systems.end(); ++iter)
		{
			std::string textureName = (*iter)->Emitter().Pool().Prototype().TextureSource();
			if (textureName.empty())
				continue;
				
			TextureProxy* texture = RenderEngine::textureManager().create(textureName.c_str());
			
			if (!texture)
				continue;
				
			typedef std::pair<std::string, TextureProxy *> KeyPair;
			typedef std::pair<TextureMap::iterator, bool> InsertResult;
			
			InsertResult insertResult = mTextureMap.insert(KeyPair(textureName, texture));
			
			if (!insertResult.second)
			{
				insertResult.first->second = texture;
			}
		}
	}
			
	void ParticleRenderer::initDeviceObjects(void)
	{
		mEffects.initDeviceObjects();
		
		mDynamicVB.resize(ParticleVBSize * sizeof(ParticleVertex));
	}

	void ParticleRenderer::deleteDeviceObjects(void)
	{
		mDynamicVB.release();
		
		mEffects.deleteDeviceObjects();
	}

	void ParticleRenderer::invalidateDeviceObjects(void)
	{
		mDynamicVB.release();
		
		mEffects.invalidateDeviceObjects();
	}

	void ParticleRenderer::restoreDeviceObjects(void)
	{
		mDynamicVB.restore();

		mEffects.restoreDeviceObjects();
	}

	void ParticleRenderer::flushTextures(void)
	{
		mTextureMap.clear();
		mLoadedTextures = false;
		mClearBuffer = true;
	}

	void ParticleRenderer::reloadEffects(void)
	{
		mEffects.reload();
	}

	void ParticleRenderer::renderQuads(ParticleSystemManager* pManager, const RenderViewport& renderViewport)
	{
		std::vector<ParticleSystem *> systems = pManager->GetAllInstances();
		if ((systems.empty()) && (!mClearBuffer))
			return;
			
		mClearBuffer = false;
			
		RenderEngine::bufferManager().setRenderTarget(0, ePLBuffer);
		RenderEngine::bufferManager().setRenderTarget(1, ePHBuffer);
		D3D::setRenderTarget(2, NULL);
		D3D::setRenderTarget(3, NULL);
				
		D3D::clear(D3DCLEAR_TARGET);
			
		renderViewport.setToDevice(SET_VIEWPORT|SET_MATRICES);

		const D3DXMATRIX view = *reinterpret_cast<const D3DXMATRIX*>(&renderViewport.getMatrix(eWorldToView));
		const D3DXMATRIX inverseView = *reinterpret_cast<const D3DXMATRIX*>(&renderViewport.getMatrix(eViewToWorld));
		
		D3D::setTransform(D3DTS_WORLD, Matrix44::I);
					
		D3DXVECTOR3 camUp = D3DXVECTOR3(0, 1.0f, 0);
		D3DXVec3TransformNormal(&camUp, &camUp, &inverseView);
		D3DXVECTOR3 camRight = D3DXVECTOR3(1.0f, 0, 0);
		D3DXVec3TransformNormal(&camRight, &camRight, &inverseView);
		
		ParticleVertex *vertices = NULL;
		ParticleVertex corners[4];
		unsigned int offset = 0, faces = 0;
		
		RenderEngine::vertexDeclManager().setToDevice(eParticleVertexDeclaration);
		
		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3D::setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		D3D::setRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		D3D::setRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
		D3D::setRenderState(D3DRS_ALPHAREF, 0);
				
		D3D::setSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		D3D::setSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		D3D::setSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		D3D::setSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		D3D::clearTextures();
		
		mEffects.beginTechnique("ParticleRender");
		
		mEffects.setMatrix(eModelToProj);
			
		mParticleCount = 0;
			
		mEffects.setFloat("gParticleSampleScale", gParticleColorAccumScale);
		mEffects.setFloat("gParticleSampleRenderScale", gParticleColorAccumScale / 1023.0f);
		mEffects.setVector("gParticleIntensity", Vec4(gSceneParticleIntensity / gParticleColorAccumScale));
				
		std::vector<ParticleSystem *>::const_iterator iter = systems.begin();
		for (; iter != systems.end(); ++iter)
		{
			const std::string& textureName = (*iter)->Emitter().Pool().Prototype().TextureSource();
			TextureMap::const_iterator find = mTextureMap.find(textureName);
			if (find != mTextureMap.end())
				D3D::setTexture(0, find->second->get());
			else
				D3D::setTexture(0, NULL);

			const std::vector<Particle> &particles =
				(*iter)->Emitter().Pool().GetParticles();
			std::vector<Particle>::const_iterator iter2 = particles.begin();
			
			while (iter2 != particles.end())
			{
				const int MinParticlesPerPass = 16;
				
				int maxVBBytes, VBOffset;
				vertices = mDynamicVB.beginLock<ParticleVertex>(sizeof(ParticleVertex) * MinParticlesPerPass, maxVBBytes, VBOffset);
				
				const int maxParticleVertsToWrite = maxVBBytes / sizeof(ParticleVertex);
				
				offset = faces = 0;

				for (; iter2 != particles.end(); ++iter2)
				{
					if (iter2->age == DEAD)
						continue;
						
					const float particleInten = Math::Clamp(iter2->alpha, 0.0f, 1.0f) * iter2->intensity;
					if (particleInten < .00125f)
						continue;
										
					if (offset + 6 > maxParticleVertsToWrite)
						break;

					corners[0].diffuse = corners[1].diffuse =
						corners[2].diffuse = corners[3].diffuse =
							(Math::Clamp(Math::FloatToIntRound(iter2->color.z * 1023.0f), 0, 1023)<<20)|
							(Math::Clamp(Math::FloatToIntRound(iter2->color.y * 1023.0f), 0, 1023)<<10)|
							(Math::Clamp(Math::FloatToIntRound(iter2->color.x * 1023.0f), 0, 1023));
										
					corners[0].intensity = corners[1].intensity =
						corners[2].intensity = corners[3].intensity = 
							particleInten * 
							gOOSceneParticleIntensity;

					corners[0].tu = iter2->uvLeft; corners[0].tv = iter2->uvTop;
					corners[1].tu = iter2->uvRight; corners[1].tv = iter2->uvTop;
					corners[2].tu = iter2->uvRight; corners[2].tv = iter2->uvBottom;
					corners[3].tu = iter2->uvLeft; corners[3].tv = iter2->uvBottom;

					for (int index = 0; index < 4; ++index)
					{
						float angle = iter2->orientation + (D3DX_PI * 0.75f) +
							(static_cast<float>(index) * D3DX_PI / -2.0f);
						
						corners[index].position = iter2->position +
							(camRight * cos(angle) * iter2->scale) +
							(camUp * sin(angle) * iter2->scale);
					}
					
					vertices[offset++] = corners[0];
					vertices[offset++] = corners[1];
					vertices[offset++] = corners[2];
					++faces;

					vertices[offset++] = corners[0];
					vertices[offset++] = corners[2];
					vertices[offset++] = corners[3];
					++faces;
					
					++mParticleCount;
				}

				mDynamicVB.endLock(offset * sizeof(ParticleVertex));
				
				if (faces > 0)
				{
					D3D::setStreamSource(0, mDynamicVB.getVB(), VBOffset, sizeof(ParticleVertex));
					
					D3D::drawPrimitive(D3DPT_TRIANGLELIST, 0, faces);
				}
			}
		}
		
		mEffects.endTechnique();
				
		D3D::enableCulling();
		D3D::setRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		D3D::setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);
		D3D::setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		D3D::setRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		D3D::setRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		D3D::setSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
		
		D3D::clearTextures();

		renderViewport.setToDevice();
		D3D::setRenderTarget(1, NULL);	
		D3D::setRenderTarget(2, NULL);
		D3D::setRenderTarget(3, NULL);
		
		D3D::setPixelShader(NULL);
	}

	void ParticleRenderer::hdrAccum(const RenderViewport& renderViewport)
	{
		if (!mParticleCount)
			return;
			
		mEffects.beginTechnique("ParticleAccumRender");

		RenderEngine::bufferManager().setRenderTarget(0, eFBuffer);
		
		renderViewport.setToDevice(SET_VIEWPORT|SET_MATRICES);

		D3D::clearTextures();	
		
		D3D::setTextureClampPoint(0, RenderEngine::bufferManager().getTexture(ePLBuffer));
		D3D::setTextureClampPoint(1, RenderEngine::bufferManager().getTexture(ePHBuffer));
		D3D::setTextureClampPoint(2, RenderEngine::bufferManager().getTexture(eFBuffer));
					
		D3D::disableCulling();
		D3D::setRenderState(D3DRS_ZWRITEENABLE, FALSE);
		
		RenderHelpers::renderSSQ();
		
		D3D::setRenderState(D3DRS_ZWRITEENABLE, TRUE);
		D3D::enableCulling();
												
		mEffects.endTechnique();

		D3D::clearTextures();
		
		renderViewport.setToDevice();
		
		D3D::setVertexShader(NULL);
		D3D::setPixelShader(NULL);
	}

	bool ParticleRenderer::render(ParticleSystemManager* pManager, const RenderViewport& renderViewport)
	{
		if (!mLoadedTextures)
			loadTextures(pManager);
			
		renderQuads(pManager, renderViewport);
		
		hdrAccum(renderViewport);
				
		return true;
	}

} // namespace gr
