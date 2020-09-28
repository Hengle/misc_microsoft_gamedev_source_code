//-----------------------------------------------------------------------------
// File: effects.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EFFECTS_H
#define EFFECTS_H
#pragma once

#include "common/core/core.h"
#include "common/utils/string.h"
#include "matrix_tracker.h"
#include "buffer_manager.h"

#include <d3dx9.h>

namespace gr
{
	class EffectStateManager : public ID3DXEffectStateManager
	{
	public:
		EffectStateManager();
		
		virtual ~EffectStateManager();
		
		virtual HRESULT __stdcall QueryInterface(REFIID iid, LPVOID *ppv);
					
		virtual ULONG __stdcall AddRef(void);

		virtual ULONG __stdcall Release(void);

		virtual HRESULT __stdcall SetTransform( D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix) ;
		virtual HRESULT __stdcall SetMaterial( const D3DMATERIAL9 *pMaterial) ;
		virtual HRESULT __stdcall SetLight( DWORD Index, const D3DLIGHT9 *pLight) ;
		virtual HRESULT __stdcall LightEnable( DWORD Index, BOOL Enable) ;
		virtual HRESULT __stdcall SetRenderState( D3DRENDERSTATETYPE State, DWORD Value) ;
		virtual HRESULT __stdcall SetTexture( DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture) ;
		virtual HRESULT __stdcall SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) ;
		virtual HRESULT __stdcall SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) ;
		virtual HRESULT __stdcall SetNPatchMode( FLOAT NumSegments) ;
		virtual HRESULT __stdcall SetFVF( DWORD FVF) ;
		virtual HRESULT __stdcall SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader) ;
		virtual HRESULT __stdcall SetVertexShaderConstantF( UINT RegisterIndex, const FLOAT *pConstantData, UINT RegisterCount) ;
		virtual HRESULT __stdcall SetVertexShaderConstantI( UINT RegisterIndex, const INT *pConstantData, UINT RegisterCount) ;
		virtual HRESULT __stdcall SetVertexShaderConstantB( UINT RegisterIndex, const BOOL *pConstantData, UINT RegisterCount) ;
		virtual HRESULT __stdcall SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader) ;
		virtual HRESULT __stdcall SetPixelShaderConstantF( UINT RegisterIndex, const FLOAT *pConstantData, UINT RegisterCount) ;
		virtual HRESULT __stdcall SetPixelShaderConstantI( UINT RegisterIndex, const INT *pConstantData, UINT RegisterCount) ;
		virtual HRESULT __stdcall SetPixelShaderConstantB( UINT RegisterIndex, const BOOL *pConstantData, UINT RegisterCount) ;

		void blockVertexShaderConstantFUploads(int startReg, int numRegs);

	private:
		int mRefCount;
		
		int mFirstBlockedVFReg;
		int mNumBlockedVFRegs;
	};

	class Effects
	{
  public:
		Effects(const char* pBasename = NULL);
			
		virtual ~Effects();
		
    LPD3DXEFFECT get(void) 
		{ 
			return DebugNull(mpEffects); 
		}
		
		void load(const char* pBasename = NULL);
		void reload(void);
		void clear(void);

		bool isLoaded(void) const;
		
		int getLoadCounter(void) const
		{
			return mLoadCounter;
		}

		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		void invalidateDeviceObjects(void);
		void restoreDeviceObjects(void);
				
		const BigString& getBasename(void) const;
		void setBasename(const char* pBasename);

		void beginTechnique(D3DXHANDLE hTechnique);
		void endTechnique(void);

		void setTexture(ETexture texture);
		void setTexture(const char* pName, LPDIRECT3DBASETEXTURE9 pTexture);
		
		void setMatrix(EMatrix matrix);
		void setMatrix(D3DXHANDLE handle, const Matrix44& m);
		
		void setFloat(D3DXHANDLE hParameter, float f);
		void setVector(D3DXHANDLE hParameter, const Vec4& v);
		void setVectorArray(D3DXHANDLE hParameter, const void* pV, int numVectors);
		void setMatrixArray(D3DXHANDLE hParameter, const void* pV, int numMatrices);

		void blockVertexShaderConstantFUploads(int startReg, int numRegs);
		
		void setPixelShader(D3DXHANDLE handle);
		void setVertexShader(D3DXHANDLE handle);
		
		struct Annotation
		{
			char name[60];
			int value;
			
			Annotation()
			{
			}
			
			Annotation(const char* pName, int val) : 
				value(val)
			{
				Assert(strlen(pName) < (sizeof(name) - 1));
				strcpy(name, pName);
			}
		};
		
		enum EShaderType
		{
			ePixelShader,
			eVertexShader
		};
		
		LPDIRECT3DVERTEXSHADER9 findVertexShader(const std::vector<Annotation>& annotations);
		LPDIRECT3DPIXELSHADER9 findPixelShader(const std::vector<Annotation>& annotations);
		D3DXHANDLE findShader(const std::vector<Annotation>& annotations, EShaderType shaderType);
						   
	private:
		Effects(const Effects& rhs);
		Effects& operator= (const Effects& rhs);

		BigString mBasename;
		LPD3DXEFFECT mpEffects;
		int mLoadCounter;

		static EffectStateManager mEffectStateManager;
	};

	  
} // namespace gr

#endif // EFFECTS_H

