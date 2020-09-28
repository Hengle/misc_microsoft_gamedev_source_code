//-----------------------------------------------------------------------------
// File: ugx_render.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "ugx_render.h"
#include "texture_manager.h"

#include "common/math/quat.h"
#include "common/math/math.h"
#include "common/utils/tweaker.h"

#include "device.h"
#include "state_cache.h"
#include "render_engine.h"

#include "common/geom/trilist_optimizer.h"

#include "x86/win32/dx9/utils/tweaker_dx9.h"

#include "../../bin/shader_regs.inc"

namespace gr
{
	// also change shaders.fx!
	static const int MaxBonesPerModel = 75;
	static const float gSpecPowerLow = Unigeom::Material::SpecPowerLow;
	static const float gSpecPowerHigh = Unigeom::Material::SpecPowerHigh;
	
	//--------------------------

	static bool gEmissive = true;
	static bool gEnvMap = true;	
	static bool gAllWhiteAlbedo = false;
	static bool gAllWhiteSpec = false;
	static bool gAllBlackSpec = false;
	static bool gAllFlat = false;
	static float gBumpZScale = 150.0f;
	static float gShadowBufferZBias = 0.0f;
	static bool gWireframe = false;
	
	//--------------------------

	RenderSection::RenderSection(Effects& effects, const UGXGeom::Geom& geom, const UGXGeom::Section& section) : 
		mpGeom(&geom),
		mpSection(&section),
		mVertexDeclHandle(0),
		mClipMapped(false)
	{
		Utils::ClearObj(mpTextures);
		Utils::ClearObj(mpPixelShaders);
		Utils::ClearObj(mpVertexShaders);
		
		createVertexDecls();
		
		linkShaders(effects);
	}

	RenderSection::~RenderSection()
	{
	}
	
	void RenderSection::linkShaders(Effects& effects)
	{
		const UnivertPacker::ElementStats baseStats(mpSection->baseVertPacker().getStats());
		const UnivertPacker::ElementStats morphStats(mpSection->morphVertPacker().getStats());

		const bool morph = !mpSection->morphVertPacker().empty(); //mpGeom->numKeyframes() > 0;		
		const int skin = baseStats.numSkin != 0; //!mpSection->rigidOnly();
		
		const bool emissive = getNumTextures(Unigeom::Material::eSelf) > 0;
		const bool envMap = getNumTextures(Unigeom::Material::eEnv) > 0;
		
		mClipMapped = (getNumTextures(Unigeom::Material::eDiffuse) > 0) &&
			(strstr(material().map(Unigeom::Material::eDiffuse, 0).name(), "clip") != 0);
		
		Status("Clip mapped: %i\n", mClipMapped);
		
		if (mpGeom->shadowGeom())
		{
			for (int passNum = eSpotShadowPass; passNum <= eOmniShadowPass; passNum++)
			{
				std::vector<Effects::Annotation> vertAnno;
				vertAnno.push_back(Effects::Annotation("morph", morph));
				vertAnno.push_back(Effects::Annotation("skin", skin));
				vertAnno.push_back(Effects::Annotation("renderPass", passNum));
				
				mpVertexShaders[passNum] = effects.findVertexShader(vertAnno);
				Verify(mpVertexShaders[passNum]);
				
				std::vector<Effects::Annotation> pixelAnno;
				pixelAnno.push_back(Effects::Annotation("renderPass", passNum));
				
				mpPixelShaders[passNum] = effects.findPixelShader(pixelAnno);
				Verify(mpPixelShaders[passNum]);
			}
		}
		else
		{
			const int numBump = Math::Max(baseStats.numBasis, morphStats.numBasis);
			
			// find pass 1 vertex shader
			std::vector<Effects::Annotation> vertAnno;
			vertAnno.push_back(Effects::Annotation("morph", morph));
			vertAnno.push_back(Effects::Annotation("numBump", numBump));
			vertAnno.push_back(Effects::Annotation("skin", skin));
			vertAnno.push_back(Effects::Annotation("renderPass", eNormalPass));
			
			mpVertexShaders[eNormalPass] = effects.findVertexShader(vertAnno);
			Verify(mpVertexShaders[eNormalPass]);
			
			// find pass 1 pixel shader
			std::vector<Effects::Annotation> pixelAnno;
			pixelAnno.push_back(Effects::Annotation("numBump", numBump));
			pixelAnno.push_back(Effects::Annotation("renderPass", eNormalPass));
			pixelAnno.push_back(Effects::Annotation("envMap", envMap));
			pixelAnno.push_back(Effects::Annotation("clip", mClipMapped));
						
			mpPixelShaders[eNormalPass] = effects.findPixelShader(pixelAnno);
			Verify(mpPixelShaders[eNormalPass]);
			
			// find pass 2 vertex shader
			vertAnno.clear();
			vertAnno.push_back(Effects::Annotation("morph", morph));
			vertAnno.push_back(Effects::Annotation("renderPass", eColorPass));
			vertAnno.push_back(Effects::Annotation("skin", skin));
			mpVertexShaders[eColorPass] = effects.findVertexShader(vertAnno);
			Verify(mpVertexShaders[eColorPass]);
			
			// find pass 2 pixel shader
			pixelAnno.clear();
			pixelAnno.push_back(Effects::Annotation("renderPass", eColorPass));
			pixelAnno.push_back(Effects::Annotation("emissive", emissive));
			
			mpPixelShaders[eColorPass] = effects.findPixelShader(pixelAnno);
			Verify(mpPixelShaders[eColorPass]);
		}
	}

	void RenderSection::loadTextures(void)
	{
		const Unigeom::Material& mat = material();
		
		for (int type = Unigeom::Material::eFirstMapType; type < Unigeom::Material::NumMapTypes; type++)
		{
			for (int slot = 0; slot < mat.numMaps(type); slot++)
			{
				BigString name(mat.map(type, slot).name());
				
				if (name.trim().len())
					mpTextures[type][slot] = RenderEngine::textureManager().create(name);
			}
		}
  }

	void RenderSection::setTextureToDevice(Unigeom::Material::EMapType type, int slot, int stage, IDirect3DTexture9* pDefSurf) const
	{
		TextureProxy* pTexture = NULL;
		if (slot < getNumTextures(type))
			pTexture = getTexture(type, slot);
			
		D3D::setTexture(stage, pTexture ? pTexture->get() : pDefSurf);
	}
	
	void RenderSection::createVertexDecls(void)
	{
		std::vector<UnivertPacker> packers;
		
		const UnivertPacker::ElementStats baseStats(mpSection->baseVertPacker().getStats());
		const UnivertPacker::ElementStats morphStats(mpSection->morphVertPacker().getStats());

		if (mpGeom->shadowGeom())
		{
			Assert(0 == baseStats.numTexCoords);
		}
		else
		{
			Assert((baseStats.numTexCoords >= 2) && (baseStats.numTexCoords <= 4));						
		}
		
		if (mpSection->morphVertPacker().empty())
		{
			Assert(1 == baseStats.numPos);
									
			UnivertPacker basePacker(mpSection->baseVertPacker());
			
			IntVec basisMap;
			IntVec basisScalesMap;
			IntVec uvMap;
			
			Assert(baseStats.numTexCoords <= 4);
			for (int i = 0; i < 4; i++)
				uvMap.push_back(i);
			
			Assert(baseStats.numBasis <= 2);
			Assert(baseStats.numBasisScales <= 2);
			for (int i = 0; i < 2; i++)
			{
				basisMap.push_back(i);
				basisScalesMap.push_back(4 + i);
			}
			
			basePacker.setDeclOrder(0, basisMap, basisScalesMap, 0, uvMap);
			
			packers.push_back(basePacker);
		}
		else
		{
			Assert(0 == baseStats.numPos);
			Assert(0 == baseStats.numNorm);
			Assert(0 == baseStats.numBasis);
			Assert(0 == baseStats.numBasisScales);
			Assert(1 == morphStats.numPos);
									
			UnivertPacker basePacker(mpSection->baseVertPacker());
			
			IntVec basisMap;
			IntVec basisScalesMap;
			IntVec uvMap;

			Assert(baseStats.numTexCoords <= 4);						
			for (int i = 0; i < 4; i++)
				uvMap.push_back(i);
			
			basePacker.setDeclOrder(0, basisMap, basisScalesMap, 0, uvMap);
				
			UnivertPacker morph0Packer(mpSection->morphVertPacker());
			UnivertPacker morph1Packer(mpSection->morphVertPacker());
			
			uvMap.clear();
			basisMap.clear();
			basisMap.push_back(0);
			basisMap.push_back(1);
			
			morph0Packer.setDeclOrder(0, basisMap, basisScalesMap, 0, uvMap);
			
			basisMap.clear();
			basisMap.push_back(2);
			basisMap.push_back(3);
			
			morph1Packer.setDeclOrder(1, basisMap, basisScalesMap, 1, uvMap);
			
			packers.push_back(basePacker);
			packers.push_back(morph0Packer);
			packers.push_back(morph1Packer);
		}
		
		mVertexDeclHandle = RenderEngine::vertexDeclManager().create(packers);		
	}
	
	void RenderSection::draw(IDirect3DVertexBuffer9* pVB) const
	{					
		D3D::setStreamSource(0, pVB, mpSection->VBOfs(), mpSection->vertSize());
	
		D3D::drawIndexedPrimitive(
			D3DPT_TRIANGLELIST,
			0,			// BaseVertexIndex
			0,			// MinIndex
			mpSection->numVerts(),		// NumVertices
			mpSection->IBOfs(),				// StartIndex
			mpSection->numTris());		// PrimitiveCount
	}
			
	void RenderSection::draw(IDirect3DVertexBuffer9* pVB, int* pLockOfs, DynamicVB* pDynVB) const
	{					
		D3D::setStreamSource(1, pDynVB[0], mpSection->morphVBOfs() + pLockOfs[0], mpSection->morphVertSize());
		D3D::setStreamSource(2, pDynVB[1], mpSection->morphVBOfs() + pLockOfs[1], mpSection->morphVertSize());
		
		draw(pVB);
	}
			
	void RenderSection::setBumpTextures(void) const
	{
		for (int bumpTextureSlot = 0; bumpTextureSlot < NumBumpTextureSlots; bumpTextureSlot++)
		{
			if (gAllFlat)
				D3D::setTexture(bumpTextureSlot, RenderEngine::bufferManager().getTexture(eDefaultNormal));
			else
			{
				setTextureToDevice(
					Unigeom::Material::eBump, 
					bumpTextureSlot, 
					bumpTextureSlot, 
					RenderEngine::bufferManager().getTexture(eDefaultNormal)
				);
			}
		}
	}
	
	void RenderSection::setSpecTexture(void) const
	{
		if (gAllWhiteSpec)
			D3D::setTexture(SPEC_TEX_P2_SAMPLER, RenderEngine::bufferManager().getTexture(eDefaultDiffuse));
		else if (gAllBlackSpec)
			D3D::setTexture(SPEC_TEX_P2_SAMPLER, RenderEngine::bufferManager().getTexture(eBlackTexture));
		else
		{
			setTextureToDevice(
				Unigeom::Material::eSpecular,
				0,
				SPEC_TEX_P2_SAMPLER,
				RenderEngine::bufferManager().getTexture(eDefaultDiffuse)
			);
		}
	}
	
	void RenderSection::setDiffTextures(void) const
	{
		for (int diffTextureSlot = 0; diffTextureSlot < NumDiffTextureSlots; diffTextureSlot++)
		{
			const int diffTextureStage = diffTextureSlot + DIFF_TEX0_P2_SAMPLER;
			if (gAllWhiteAlbedo)
				D3D::setTexture(diffTextureStage, RenderEngine::bufferManager().getTexture(eDefaultDiffuse));
			else
			{
				setTextureToDevice(
					Unigeom::Material::eDiffuse, 
					diffTextureSlot,
					diffTextureStage,
					RenderEngine::bufferManager().getTexture(eDefaultDiffuse)
				);
			}
		}
	}
	
	bool RenderSection::setEmissiveTexture(void) const
	{
		if ((getNumTextures(Unigeom::Material::eSelf)) && (gEmissive))
		{
			setTextureToDevice(
				Unigeom::Material::eSelf,
				0,
				EMISSIVE_P2_SAMPLER,
				RenderEngine::bufferManager().getTexture(eDefaultDiffuse));
			
			return true;
		}
		
		D3D::setTexture(EMISSIVE_P2_SAMPLER, NULL);
			
		return false;
	}
	
	bool RenderSection::setEnvTexture(void) const
	{
		if ((getNumTextures(Unigeom::Material::eEnv)) && (gEnvMap))
		{
			setTextureToDevice(
				Unigeom::Material::eEnv,
				0,
				ENV_P1_SAMPLER,
				RenderEngine::bufferManager().getTexture(eDefaultDiffuse));
			
			return true;
		}
		
		D3D::setTexture(ENV_P1_SAMPLER, NULL);
			
		return false;
	}
	
	void RenderSection::setShaders(int pass) const
	{
		D3D::setPixelShader(getPixelShader(pass));
		D3D::setVertexShader(getVertexShader(pass));
		
		BOOL boneFlags[2];
		
		boneFlags[0] = (mpSection->maxBones() == 1);
		boneFlags[1] = (mpSection->maxBones() > 2);

		Assert(FOUR_BONES == (ONE_BONE + 1));
		D3D::setVertexShaderConstantB(ONE_BONE, boneFlags, 2);		
	}
	
	void RenderSection::setVertexDecl(void) const
	{
		RenderEngine::vertexDeclManager().setToDevice(mVertexDeclHandle);
	}
	
	void RenderSection::setMaterialConstants(int pass) const
	{
		switch (pass)
		{
			case eNormalPass:
			{
				const Unigeom::Material& material = this->material();
				
				const float bumpZScale = gBumpZScale * material.bumpiness();
				const float specPower = material.specPower();
				float envInten = material.envIntensity();
			
				D3D::setPixelShaderConstantF(BUMP_Z_SCALE, 
					Vec4(1.0f / bumpZScale, 0, 0, 0), 
					1);
				
				D3D::setPixelShaderConstantF(MATERIAL_PARAMS,
					Vec4(0, envInten, 0, Math::Clamp((specPower - gSpecPowerLow) / (gSpecPowerHigh - gSpecPowerLow), 0.0f, 1.0f)), 
					1);
								
				break;
			}
			case eColorPass:
			{
				const Unigeom::Material& material = this->material();
				D3D::setPixelShaderConstantF(MATERIAL_PARAMS, Vec4(material.selfIntensity(), 0, 0, 0.0f), 1);
				break;
			}
		}
	}
						
	//--------------------------
	
	void RenderMesh::initStaticTweakers(void)
	{
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Bump Z scale", &gBumpZScale, 0.00125f, 999.0f, .5f, false, 0, NULL, false));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Emissive", &gEmissive));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Env mapping", &gEnvMap));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Force white albedo", &gAllWhiteAlbedo));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Force white specular", &gAllWhiteSpec));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Force black specular", &gAllBlackSpec));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Force flat", &gAllFlat));
		gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Wireframe", &gWireframe));
		gSharedTweaker.createEntry(eShadows, new IntegralTweakerEntry("Shadow buffer Z bias", &gShadowBufferZBias, -999.0f, 999.0f, .1f, false, 0, NULL, false));
	}

	RenderMesh::RenderMesh() : 
		mpVB(NULL), 
		mpIB(NULL),
		mpHier(NULL),
		mEffects(RenderEngine::effects()),
		mEffectLoadCount(0)
	{
		clear();
	}
	
	RenderMesh::RenderMesh(const UGXGeom::Geom& geom) :
		mpVB(NULL), 
		mpIB(NULL),
		mpHier(NULL),
		mEffects(RenderEngine::effects()),
		mEffectLoadCount(0)
	{
		clear();
		mGeom = geom;
		newGeomInit();
	}

	RenderMesh::~RenderMesh()
	{
		free(mpHier);
		deleteDeviceObjects();
	}

	void RenderMesh::oneTimeSceneInit(void)
	{
	}

	void RenderMesh::initDeviceObjects(void)
	{
		if (!mLoadedGeom)
			return;

		createVB();
		createIB();
			
		mDynVB[0].restore();
		mDynVB[1].restore();
	}

	void RenderMesh::deleteDeviceObjects(void)
	{
		if (mpVB)
		{
			DebugNull(D3D::getDevice());
			D3D::setStreamSource(0, NULL, 0, 0);
			D3D::safeRelease(mpVB);
		}
							
		if (mpIB)
		{
			DebugNull(D3D::getDevice());
			D3D::setIndices(NULL);
			D3D::safeRelease(mpIB);
		}
		
		mDynVB[0].release();
		mDynVB[1].release();
	}

	void RenderMesh::invalidateDeviceObjects(void)
	{
		deleteDeviceObjects();
	}
	
	void RenderMesh::restoreDeviceObjects(void)
	{
		initDeviceObjects();
	}
		
	
	void RenderMesh::startOfFrame(void)
	{
	}
	
	void RenderMesh::clear(void)
	{
		deleteDeviceObjects();
		clearSections();
		
		free(mpHier);
		mpHier = NULL;

		mGeom.clear();
				
		mLoadedGeom = false;
	}

	void RenderMesh::clearSections(void)
	{
		mSections.clear();
	}

	void RenderMesh::initSections(void)
	{
		clearSections();
		
		if (!mLoadedGeom)
			return;
			
		for (int sectionIndex = 0; sectionIndex < mGeom.numSections(); sectionIndex++)
		{
#if LOG		
			Status("Section: %i\n", sectionIndex);
#endif			
			mSections.push_back(RenderSection(mEffects, mGeom, mGeom.section(sectionIndex)));
		}
	}

	void RenderMesh::loadTextures(void)
	{
		for (int sectionIndex = 0; sectionIndex < mSections.size(); sectionIndex++)
			mSections[sectionIndex].loadTextures();
	}
	
	bool RenderMesh::newGeomInit(void)
	{
		if (mGeom.numBones() > MaxBonesPerModel)
		{
			Error("RenderMesh::load: Mesh has too many bones!\n");
			clear();
			return true;
		}
				
		if (!mGeom.numSections())
		{
			Error("RenderMesh::load: Mesh has no sections!\n");
			clear();
			return true;
		}
		
		

		mLoadedGeom = true;
						
		Status("RenderMesh::load: Initializing render sections:\n");
		
		initSections();

		Status("RenderMesh::load: Loading textures:\n");
		
		loadTextures();
		
		Status("RenderMesh::load: Creating buffers:\n");
		
		createVB();
		createDynVB();
		createIB();
						
		const int numMatrixCacheBytes = 15 + (mGeom.numBones() + 1) * sizeof(Matrix44);
		
		Status("RenderMesh::load: Allocating %i bytes for local aligned matrix cache\n", numMatrixCacheBytes);
		
		mpHier = reinterpret_cast<Matrix44*>(malloc(numMatrixCacheBytes));
		mpAHier = Utils::AlignUp(mpHier, 16);
		
		mEffectLoadCount = mEffects.getLoadCounter();
						
		Status("RenderMesh::load: Success\n");
		
		return false;
	}
	
	// true on failure
	bool RenderMesh::load(Stream& stream)
	{
		clear();
		
		Status("RenderMesh::load: Deserializing from \"%s\"\n", stream.getName().c_str());
		    				
		stream >> mGeom;
		
		Status("RenderMesh::load: Deserialization done\n");
		
		return newGeomInit();
	}
	
	bool RenderMesh::valid(void) const
	{
		return mLoadedGeom;
	}

	void RenderMesh::createIB(void)
	{
		D3D::setIndices(NULL);
		D3D::safeRelease(mpIB);

		StaticAssert(sizeof(UGXGeom::Geom::IndexType) == sizeof(WORD));
		const D3DFORMAT IndexFormat = D3DFMT_INDEX16;
		
		const int IBSize = mGeom.IB().size() * sizeof(mGeom.IB()[0]);
		Assert(IBSize > 0);
		
		Status("RenderMesh::createIB: Creating index buffer of %i bytes\n", IBSize);
		
		mpIB = D3D::createIndexBuffer(
			IBSize,
			D3DUSAGE_WRITEONLY, 
			IndexFormat, 
			D3DPOOL_DEFAULT);

		BYTE* pDst;
		D3D::errCheck(mpIB->Lock(0, 0, reinterpret_cast<void**>(&pDst), 0));
				
		const BYTE* pSrc = reinterpret_cast<const BYTE*>(&mGeom.IB()[0]);
		
		memcpy(pDst, pSrc, IBSize);
		
		mpIB->Unlock();
	}
	
	void RenderMesh::createDynVB(void)
	{
		if (0 == mGeom.numKeyframes())
			return;
			
		const int dynVBSize = mGeom.keyframe(0).verts().size() * 2 * 3;
		
		Status("RenderMesh::createDynVB: Resizing dynamic VB's to %i bytes\n", dynVBSize);
		
		mDynVB[0].resize(dynVBSize, 0);
		mDynVB[1].resize(dynVBSize, 0);
	}
	
	void RenderMesh::createVB(void)
	{
		D3D::setStreamSource(0, NULL, 0, 0);
		D3D::safeRelease(mpVB);

		const int VBSize = mGeom.VB().size();
		if (VBSize)
		{
			Status("RenderMesh::createVB: Creating vertex buffer of %i bytes\n", VBSize);
			
			mpVB = D3D::createVertexBuffer(
				VBSize, 
				D3DUSAGE_WRITEONLY, 
				0, 
				D3DPOOL_DEFAULT);

			BYTE* pDst;
			D3D::errCheck(mpVB->Lock(0, 0, reinterpret_cast<void**>(&pDst), 0));

			const BYTE* pSrc = reinterpret_cast<const BYTE*>(&mGeom.VB()[0]);		
			
			memcpy(pDst, pSrc, VBSize);
					
			mpVB->Unlock();
		}
	}

	void RenderMesh::setShaderBoneMatrices(void) const
	{
		if ((!mGeom.rigidOnly()) && (mGeom.allSectionsRigid()))
			return;
			
		const Matrix44& worldToView = D3D::getMatrix(eWorldToView);
						
		const int numShaderBones = 1 + mGeom.numBones();
		const int numShaderBoneVectors = numShaderBones * 3;
					
		Vec4 boneMatrices[MaxBonesPerModel * 3];
		
		Vec4* pDst = boneMatrices;
		
		for (int i = 0; i < numShaderBones; i++)
		{
			const Matrix44 temp(mpAHier[i] * worldToView);
			pDst[0] = temp.getColumn(0);
			pDst[1] = temp.getColumn(1);
			pDst[2] = temp.getColumn(2);
			pDst += 3;
		}
				
		if (mGeom.rigidOnly())
		{
			const int matrixIndex = DebugRange(mGeom.rigidBoneIndex() + 1, numShaderBones);
						
			D3D::setVertexShaderConstantF(
				RIGID_BIND_TO_VIEW, 
				reinterpret_cast<const float*>(&boneMatrices[matrixIndex * 3]), 
				3);
		}
		else
		{
			D3D::setVertexShaderConstantF(BIND_TO_VIEW_MATRICES, reinterpret_cast<const float*>(boneMatrices), numShaderBoneVectors);
		}
	}
	
	void RenderMesh::setShaderMatrices(void) const
	{
		setShaderBoneMatrices();
		
		D3D::setVertexShaderConstantF(VIEW_TO_PROJ_ADD, D3D::getMatrixTracker().getViewToProjAdd(), 1);
		D3D::setVertexShaderConstantF(VIEW_TO_PROJ_MUL, D3D::getMatrixTracker().getViewToProjMul(), 1);
		
		D3D::setPixelShaderConstantF(VIEW_TO_WORLD, eViewToWorld, true);
	}
	
	void RenderMesh::setSectionMatrix(const RenderSection& renderSection)
	{
		const UGXGeom::Section& UGXSection = *DebugNull(renderSection.getUGXSection());
		
		if (!mGeom.rigidOnly())
		{
			if (UGXSection.rigidOnly())
			{
				const int numShaderBones = 1 + mGeom.numBones();
				const int matrixIndex = DebugRange(UGXSection.rigidBoneIndex() + 1, numShaderBones);
								
				const Matrix44 temp(mpAHier[matrixIndex] * D3D::getMatrix(eWorldToView));
				
				Vec4 boneMatrices[3];
				boneMatrices[0] = temp.getColumn(0);
				boneMatrices[1] = temp.getColumn(1);
				boneMatrices[2] = temp.getColumn(2);

				D3D::setVertexShaderConstantF(
					RIGID_BIND_TO_VIEW, 
					reinterpret_cast<float*>(boneMatrices), 
					3);
			}
		}
	}
					
	void RenderMesh::initRenderStatesP0(void)
	{
		D3D::setIndices(mpIB);
		
		D3D::clearTextures();
		
		for (int samplerIndex = FIRST_P1_SAMPLER; samplerIndex <= LAST_P1_SAMPLER; samplerIndex++)
		{
			D3D::setWrapAddressing(samplerIndex);
			D3D::setAnisotropicFiltering(samplerIndex);
			D3D::setMaxAnisotropy(samplerIndex, 4);
			D3D::setSRGBTexture(samplerIndex, false);
		}
		
		D3D::setClampAddressing(ENV_P1_SAMPLER);
		D3D::setLinearFiltering(ENV_P1_SAMPLER);
		D3D::setMaxAnisotropy(ENV_P1_SAMPLER, 1);
		D3D::setSRGBTexture(ENV_P1_SAMPLER, true);
		
		//D3D::setSamplerState(ENV_P1_SAMPLER, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	}
			
	void RenderMesh::initRenderStatesP1(void)
	{
		D3D::setIndices(mpIB);
		
		D3D::clearTextures();
		
		for (int samplerIndex = DIFF_TEX0_P2_SAMPLER; samplerIndex <= SPEC_TEX_P2_SAMPLER; samplerIndex++)
		{
			D3D::setWrapAddressing(samplerIndex);
			D3D::setAnisotropicFiltering(samplerIndex);
			D3D::setMaxAnisotropy(samplerIndex, 4);
			D3D::setSRGBTexture(samplerIndex, false);
		}
		
		D3D::setWrapAddressing(EMISSIVE_P2_SAMPLER);
		D3D::setAnisotropicFiltering(EMISSIVE_P2_SAMPLER);
		D3D::setMaxAnisotropy(EMISSIVE_P2_SAMPLER, 4);
		D3D::setSRGBTexture(EMISSIVE_P2_SAMPLER, true);
		
		D3D::setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		D3D::setRenderState(D3DRS_ALPHAREF, 0);
		D3D::setRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	}
	
	void RenderMesh::initRenderStatesShadow(int pass)
	{
		D3D::setIndices(mpIB);
		
		D3D::clearTextures();
		
		D3D::setVertexShaderConstantF(SHADOW_BUFFER_Z_BIAS, Vec4(gShadowBufferZBias, 0, 0, 0));
	}
		
	void RenderMesh::deinitRenderStatesP0(void)
	{
		D3D::clearTextures();
		
		for (int samplerIndex = FIRST_P1_SAMPLER; samplerIndex <= LAST_P1_SAMPLER; samplerIndex++)
		{
			D3D::setWrapAddressing(samplerIndex);
			D3D::setLinearFiltering(samplerIndex);
			D3D::setMaxAnisotropy(samplerIndex, 1);
			D3D::setSRGBTexture(samplerIndex, false);
		}
	}
	
	void RenderMesh::deinitRenderStatesP1(void)
	{
		D3D::setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		
		D3D::clearTextures();
		
		for (int samplerIndex = FIRST_P2_SAMPLER; samplerIndex <= LAST_P2_SAMPLER; samplerIndex++)
		{
			D3D::setWrapAddressing(samplerIndex);
			D3D::setLinearFiltering(samplerIndex);
			D3D::setMaxAnisotropy(samplerIndex, 1);
			D3D::setSRGBTexture(samplerIndex, false);
		}
	}
	
	void RenderMesh::findKeyframes(int* pLockOfs, float time)
	{
		int frameA, frameB;
		const float duration = mGeom.keyframe(mGeom.numKeyframes() - 1).time();
				
		Assert(duration > 0.0f);

		bool loop = true;
		if (loop)
			time = Math::fPosMod(time, duration);
							
		if (time >= duration)
			frameA = frameB = mGeom.numKeyframes() - 1;
		else
		{
			for (int i = 0; i < mGeom.numKeyframes(); i++)
				if (time < mGeom.keyframe(i).time())
					break;

			frameA = Math::Max(0, i - 1);
			frameB = i;
		}

		DebugRange(frameA, mGeom.numKeyframes());
		DebugRange(frameB, mGeom.numKeyframes());
		
		const float frameATime = mGeom.keyframe(frameA).time();
		const float frameBTime = mGeom.keyframe(frameB).time();
		Assert(frameBTime >= frameATime);
		Assert(time >= frameATime);

		const float keyTime = frameBTime - frameATime;
		float interpFactor = duration ? ((time - frameATime) / keyTime) : 0.0f;
						
		DebugRangeIncl(interpFactor, 0.0f, 1.0f);
				
		const AABB& bounds = mGeom.bounds();
		
		D3D::setVertexShaderConstantF(MORPH_VERT_MUL, Vec4(bounds.diagonal() * (1.0f/1023.0f)));
		D3D::setVertexShaderConstantF(MORPH_VERT_ADD, Vec4(bounds.low()));
						
		D3D::setVertexShaderConstantF(MORPH_LERP_FACTOR, Vec4(interpFactor, 0, 0, 0));
								
		int frames[2];
		frames[0] = frameA;
		frames[1] = frameB;
		
		for (int i = 0; i < 2; i++)
		{
			const int size = mGeom.keyframe(frames[0]).size();
			uchar* pDst = mDynVB[i].lock<uchar>(size, pLockOfs[i]);
			
			memcpy(pDst, &mGeom.keyframe(frames[i]).verts()[0], size);
			
			mDynVB[i].unlock();
		}
	}

	void RenderMesh::renderVisibleP0(float time)
	{
		const bool morph = mGeom.numKeyframes() > 0;
		
		int lockOfs[2];
		if (morph)
			findKeyframes(lockOfs, time);
								
		initRenderStatesP0();
		
		setShaderMatrices();
				
		for (int sectionIndex = 0; sectionIndex < mGeom.numSections(); sectionIndex++)
		{
			const RenderSection& renderSection = mSections[sectionIndex];
			
			setSectionMatrix(renderSection);
			
			renderSection.setMaterialConstants(eNormalPass);
			renderSection.setVertexDecl();
			renderSection.setShaders(eNormalPass);
			renderSection.setBumpTextures();
			renderSection.setEnvTexture();
			
			if (renderSection.clipMapped())
			{
				renderSection.setTextureToDevice(
					Unigeom::Material::eDiffuse, 
					0,
					DIFF_TEX0_P1_SAMPLER,
					RenderEngine::bufferManager().getTexture(eDefaultDiffuse)
				);
			}
			else
			{
				D3D::setTexture(DIFF_TEX0_P1_SAMPLER, NULL);
			}
																				
			if (morph)
				renderSection.draw(mpVB, lockOfs, mDynVB);
			else
				renderSection.draw(mpVB);
		}
		
		deinitRenderStatesP0();
	}
	
	void RenderMesh::renderVisibleP1(float time)
	{
		const bool morph = mGeom.numKeyframes() > 0;
		
		int lockOfs[2];
		if (morph)
			findKeyframes(lockOfs, time);
								
		initRenderStatesP1();
		
		setShaderMatrices();
				
		for (int sectionIndex = 0; sectionIndex < mGeom.numSections(); sectionIndex++)
		{
			const RenderSection& renderSection = mSections[sectionIndex];
			
			setSectionMatrix(renderSection);
			
			renderSection.setMaterialConstants(eColorPass);
			renderSection.setVertexDecl();
			renderSection.setShaders(eColorPass);
			renderSection.setDiffTextures();
			renderSection.setSpecTexture();
			renderSection.setEmissiveTexture();
			
			D3D::setRenderState(D3DRS_ALPHATESTENABLE, renderSection.clipMapped());
																			
			if (morph)
				renderSection.draw(mpVB, lockOfs, mDynVB);
			else
				renderSection.draw(mpVB);
		}
		
		deinitRenderStatesP1();
	}
	
	void RenderMesh::renderShadow(float time, int pass)
	{
		const bool morph = mGeom.numKeyframes() > 0;
		
		int lockOfs[2];
		if (morph)
			findKeyframes(lockOfs, time);
								
		initRenderStatesShadow(pass);
		
		setShaderMatrices();
								
		for (int sectionIndex = 0; sectionIndex < mGeom.numSections(); sectionIndex++)
		{
			const RenderSection& renderSection = mSections[sectionIndex];
			
			setSectionMatrix(renderSection);
			
			renderSection.setVertexDecl();
			renderSection.setShaders(pass);
																		
			if (morph)
				renderSection.draw(mpVB, lockOfs, mDynVB);
			else
				renderSection.draw(mpVB);
		}
	}
		
	void RenderMesh::relinkSectionShaders(void)
	{
		Status("RenderMesh::relinkSectionShaders: Relinking shaders\n");
		
		for (int sectionIndex = 0; sectionIndex < mSections.size(); sectionIndex++)
			mSections[sectionIndex].linkShaders(mEffects);
		mEffectLoadCount = mEffects.getLoadCounter();
	}
		
	void RenderMesh::render(const Matrix44& modelToWorld, const Anim* pAnim, float time, int pass)
	{
		if (!valid())
			return;
			
		if (mEffectLoadCount != mEffects.getLoadCounter())
			relinkSectionShaders();

		if ((pAnim) && ((!pAnim->valid()) || (pAnim->numBones() != numBones())))
			pAnim = NULL;
									
    setupHier(mpAHier, modelToWorld, pAnim, time);
		
		bindToBoneConcat(mpAHier);
				
		switch (pass)
		{
			case eNormalPass:
			{
				if (gWireframe)
					D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
				
				renderVisibleP0(time);
				
				if (gWireframe)
					D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
					
				break;
			}
			case eColorPass:
			{
				if (gWireframe)
					D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
					
				renderVisibleP1(time);
				
				if (gWireframe)
					D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
					
				break;
			}
			case eSpotShadowPass:
			case eOmniShadowPass:
			{
				Assert(shadowGeom());
				renderShadow(time, pass);
				break;
			}
			default:
				Assert(false);
		}
	}
			
	void RenderMesh::renderHier(const Matrix44& modelToWorld, const Anim* pAnim, float time)
	{
		if (!valid())
			return;

		if ((pAnim) && ((!pAnim->valid()) || (pAnim->numBones() != numBones())))
			pAnim = NULL;
									
		setupHier(mpAHier, modelToWorld, pAnim, time);
				
		for (int i = 0; i < numBones(); i++)
		{
			const AABB& boneBounds = mGeom.boneBounds(i);
			if (boneBounds.low()[0] > boneBounds.high()[0])
				continue;
				
			for (int axis = 0; axis < 3; axis++)
			{
				const int axis1 = (axis + 1) % 3;
				const int axis2 = (axis + 2) % 3;
				
				for (int side = 0; side < 2; side++)
				{
					Vec3 v[4];
					
					for (int k = 0; k < 4; k++)
					{
						v[k][axis] = boneBounds[side][axis];
						v[k][axis1] = boneBounds[k >> 1][axis1];
						v[k][axis2] = boneBounds[(k & 1) ^ (k >> 1)][axis2];
					}
					
					for (int k = 0; k < 4; k++)
					{
						Vec4 s(Vec4(v[k], 1.0f) * mpAHier[i + 1]);
						Vec4 e(Vec4(v[(k+1)%4], 1.0f) * mpAHier[i + 1]);
						
						RenderEngine::drawLine(s, e, Vec3(.7f, .7f, .7f));
					}
				}
			}	
		}
		
		for (int i = 0; i < numBones(); i++)
		{
			const int parentIndex = mGeom.bone(i).parentBoneIndex();
    				
			const float L = 2.0f;
			
			RenderEngine::drawLine(
				Vec4(0,0,0,1) * mpAHier[i + 1], 
				Vec4(0,0,0,1) * mpAHier[parentIndex + 1],
				Vec3(1.0f, 1.0f, .6f)
				);
				
			RenderEngine::drawLine(
				Vec4(0,0,0,1) * mpAHier[i + 1], 
				Vec4(L,0,0,1) * mpAHier[i + 1],
				Vec3(1.0f, 0.0f, 0.0f)
				);				
				
			RenderEngine::drawLine(
				Vec4(0,0,0,1) * mpAHier[i + 1], 
				Vec4(0,L,0,1) * mpAHier[i + 1],
				Vec3(0.0f, 1.0f, 0.0f)
				);
				
			RenderEngine::drawLine(
				Vec4(0,0,0,1) * mpAHier[i + 1], 
				Vec4(0,0,L,1) * mpAHier[i + 1],
				Vec3(0.0f, 0.0f, 1.0f)
				);
    }
	}
	
	BodyBounds& RenderMesh::getBodyBounds(BodyBounds& dst, const Matrix44& modelToWorld, const Anim* pAnim, float time)
	{
		if (!valid())
			return dst;

		if ((pAnim) && ((!pAnim->valid()) || (pAnim->numBones() != numBones())))
			pAnim = NULL;
									
		setupHier(mpAHier, modelToWorld, pAnim, time);
			
		for (int i = 0; i < numBones(); i++)
		{
			const AABB& boneBounds = mGeom.boneBounds(i);
			if (boneBounds.low()[0] > boneBounds.high()[0])
				continue;
			
			dst.insert(NodeBounds(boneBounds, mpAHier[i + 1]));
		}
		
		return dst;
	}
			
	// returns local_bone->world matrices
	void RenderMesh::setBoneToWorld(Matrix44* pDst, const Matrix44& modelToWorld) const
	{
		for (int i = 0; i < numBones(); i++)
		{
			Matrix44 bindToBone(Quat::createMatrix(mGeom.bone(i).modelToBone().getQ()));
			bindToBone.setTranslate(mGeom.bone(i).modelToBone().getT(), 1.0f);
										
			pDst[i + 1] = bindToBone.inverse() * modelToWorld;
		}
	}

		// returns local_bone->world matrices
	void RenderMesh::setupHier(Matrix44* pDst, const Matrix44& modelToWorld, const Anim* pAnim, float frame) const
	{
		pDst[0] = modelToWorld;

		if (!pAnim)
			setBoneToWorld(pDst, modelToWorld);
		else
		{
			Assert(pAnim->numBones() == numBones());

			pAnim->interpolateKeyFrames(pDst, frame);

			flattenHier(pDst);
		}
	}

	void RenderMesh::flattenHier(Matrix44* pDst) const
	{
		// hierarchical matrix concatenation
		for (int i = 0; i < numBones(); i++)
		{
			const int parentIndex = mGeom.bone(i).parentBoneIndex();
			DebugRange(parentIndex, -1, i);

			pDst[i + 1] = pDst[i + 1] * pDst[parentIndex + 1];
		}
	}

	// converts local_bone->root to bind_pose->local_bone->root (or bind_pose->root)
	void RenderMesh::bindToBoneConcat(Matrix44* pDst) const
	{
		for (int i = 0; i < numBones(); i++)
		{
			Matrix44 bindToBone(Quat::createMatrix(mGeom.bone(i).modelToBone().getQ()));
			bindToBone.setTranslate(mGeom.bone(i).modelToBone().getT(), 1.0f);
										
			pDst[i + 1] = bindToBone * pDst[i + 1];
		}
	}
	
	Sphere RenderMesh::calcBoundingSphere(Anim* pAnim)
	{
		if (!valid())	
			return Sphere(eClear);
			
		return mGeom.boundingSphere();		
	}
	
} // namespace gr





