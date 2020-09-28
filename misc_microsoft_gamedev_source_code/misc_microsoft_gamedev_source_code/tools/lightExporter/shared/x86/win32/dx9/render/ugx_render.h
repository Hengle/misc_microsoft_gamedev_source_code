//-----------------------------------------------------------------------------
// File: ugx_render.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef UGX_RENDER_H

#include "texture_manager.h"

#include "common/geom/ugx_geom.h"

#include "x86/win32/dx9/render/light_tilizer.h"
#include "x86/win32/dx9/render/dynamic_vb.h"

#include "anim.h"
#include "render_engine.h"

#include <d3dx9math.h>
#include <d3dx9tex.h>

namespace gr
{
	enum ERenderPass
	{
		eNormalPass = 0,
		eColorPass = 1,
		eSpotShadowPass = 2,
		eOmniShadowPass = 3,
		
		eNumPasses
	};
	
	class RenderSection 
	{
	public:
		enum 
		{ 
			NumTextureTypes = Unigeom::Material::NumMapTypes,
			NumTextureSlots = Unigeom::Material::MaxMapsPerType,
			NumDiffTextureSlots = 2,
			NumBumpTextureSlots = 2
		};
		
		enum { MaxPasses = eNumPasses };
		
		RenderSection(Effects& effects, const UGXGeom::Geom& geom, const UGXGeom::Section& section);
		
		RenderSection(const RenderSection& b)
		{
			*this = b;
		}
		
		~RenderSection();
		
		RenderSection& operator= (const RenderSection& rhs)
		{
			if (this == &rhs)
				return *this;
			
			mpGeom = rhs.mpGeom;
			mpSection = rhs.mpSection;
			
			for (int i = 0; i < NumTextureTypes; i++)
				for (int j = 0; j < NumTextureSlots; j++)
					mpTextures[i][j] = rhs.mpTextures[i][j];
			
			mVertexDeclHandle = rhs.mVertexDeclHandle;
								
			std::copy(rhs.mpPixelShaders, rhs.mpPixelShaders + MaxPasses, mpPixelShaders);
			std::copy(rhs.mpVertexShaders, rhs.mpVertexShaders + MaxPasses, mpVertexShaders);
			
			mClipMapped = rhs.mClipMapped;
												
			return *this;
		}
		
		void loadTextures(void);
		
		const Unigeom::Material& material(void) const
		{
			return mpGeom->material(mpSection->materialIndex());

		}
								
		int getNumTextures(Unigeom::Material::EMapType type) const
		{
			return material().numMaps(type);
		}

		TextureProxy* getTexture(Unigeom::Material::EMapType type, int slot) const
		{
			return mpTextures[DebugRange(type, NumTextureTypes)][DebugRange(slot, NumTextureSlots)];
		}
				
		void setTextureToDevice(Unigeom::Material::EMapType type, int slot, int stage, IDirect3DTexture9* pDefSurf) const;
		
		LPDIRECT3DPIXELSHADER9 getPixelShader(int pass) const
		{
			return mpPixelShaders[DebugRange(pass, static_cast<int>(MaxPasses))];
		}
		
		LPDIRECT3DVERTEXSHADER9 getVertexShader(int pass) const
		{
			return mpVertexShaders[DebugRange(pass, static_cast<int>(MaxPasses))];
		}
		
		const UGXGeom::Geom* getGeom(void) const { return mpGeom; }
		const UGXGeom::Section* getUGXSection(void) const { return mpSection; }
		
		const VertexDeclManager::Handle getVertexDecl(void) const
		{
			return mVertexDeclHandle;
		}
		
		void draw(IDirect3DVertexBuffer9* pVB) const;
		void draw(IDirect3DVertexBuffer9* pVB, int* pLockOfs, DynamicVB* pDynVB) const;
								
		void setBumpTextures(void) const;
		void setSpecTexture(void) const;
		void setDiffTextures(void) const;
		bool setEmissiveTexture(void) const;
		bool setEnvTexture(void) const;
		void setShaders(int pass) const;
		void setVertexDecl(void) const;
		void setMaterialConstants(int pass) const;
		
		void linkShaders(Effects& effects);
		
		bool clipMapped(void) const { return mClipMapped; }
				
	private:
		const UGXGeom::Geom* mpGeom;
		const UGXGeom::Section* mpSection;
				
		LPDIRECT3DPIXELSHADER9 mpPixelShaders[MaxPasses];
		LPDIRECT3DVERTEXSHADER9 mpVertexShaders[MaxPasses];
					
		TextureProxy* mpTextures[NumTextureTypes][NumTextureSlots];
		VertexDeclManager::Handle mVertexDeclHandle;
		
		bool mClipMapped;
		
		void createVertexDecls(void);
	};

	class RenderMesh
	{
	public:
		RenderMesh();
		RenderMesh(const UGXGeom::Geom& geom);
		~RenderMesh();

		void oneTimeSceneInit(void);
		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
		void startOfFrame(void);
				
		void clear(void);

		bool load(Stream& stream);
		bool valid(void) const;
						
		void render(const Matrix44& modelToWorld, const Anim* pAnim, float time, int pass);
		void renderHier(const Matrix44& modelToWorld, const Anim* pAnim, float time);
		
		BodyBounds& getBodyBounds(BodyBounds& dst, const Matrix44& modelToWorld, const Anim* pAnim, float time);
					
		int numBones(void) const
		{
			if (!valid())
				return 0;
			return mGeom.numBones();
		}
		
		Sphere calcBoundingSphere(Anim* pAnim);
				
		bool shadowGeom(void) const
		{
			if (!mLoadedGeom)
				return false;
				
			return mGeom.shadowGeom();
		}

		static void initStaticTweakers(void);

	private:
		UGXGeom::Geom mGeom;
		
		Matrix44* mpHier;
		Matrix44* mpAHier;
		
		bool mLoadedGeom;
		
		IDirect3DVertexBuffer9* mpVB;
		IDirect3DIndexBuffer9* mpIB;
						
		Effects& mEffects;
		int mEffectLoadCount;

		std::vector<RenderSection> mSections;
		
		DynamicVB mDynVB[2];
		
		void loadTextures(void);
		void clearSections(void);
		void initSections(void);
		void createIB(void);
		void createVB(void);
		void createDynVB(void);
		
		void setShaderBoneMatrices(void) const;
		void setBoneToWorld(Matrix44* pDst, const Matrix44& modelToWorld) const;
		void flattenHier(Matrix44* pDst) const;
		void bindToBoneConcat(Matrix44* pDst) const;
		void setupHier(Matrix44* pDst, const Matrix44& modelToWorld, const Anim* pAnim, float time) const;

		void setShaderMatrices(void) const;
		void setSectionMatrix(const RenderSection& renderSection);
		void setSectionConstants(const RenderSection& renderSection);
		void renderVisible(float time);
		void initRenderStatesP0(void);
		void deinitRenderStatesP0(void);
		void initRenderStatesP1(void);
		void deinitRenderStatesP1(void);
		void initRenderStatesShadow(int pass);
		void renderVisibleP0(float time);
		void renderVisibleP1(float time);
		void renderShadow(float time, int pass);
		void relinkSectionShaders(void);
		void findKeyframes(int* pLockOfs, float time);
		bool newGeomInit(void);
	};
} // namespace gr

#endif // UGX_RENDER_H
