//-----------------------------------------------------------------------------
// File: effects.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "effects.h"
#include "render_engine.h"
#include "d3d_dev_spoof.h"

#define LOG 0

namespace gr
{
	EffectStateManager Effects::mEffectStateManager;

	EffectStateManager::EffectStateManager() : 
		mRefCount(1),
		mFirstBlockedVFReg(0),
		mNumBlockedVFRegs(0)
	{
	}
		
	EffectStateManager::~EffectStateManager()
	{
	}
			
	ULONG __stdcall EffectStateManager::AddRef(void)
	{
		mRefCount++;
		Verify(mRefCount >= 1);
		return mRefCount;
	}

	ULONG __stdcall EffectStateManager::Release(void)
	{
		mRefCount--;
		Verify(mRefCount >= 1);
		//if (mRefCount == 0)
		//	delete this;
		return mRefCount;
	}
	
	HRESULT __stdcall EffectStateManager::QueryInterface(REFIID iid, LPVOID *ppv)
	{
		if ((iid == IID_IUnknown) || (iid == IID_ID3DXEffectStateManager))
      *ppv = reinterpret_cast<ID3DXEffectStateManager*>(this);
		else
		{
			*ppv = 0;
			return E_NOINTERFACE;
		}    
  
		reinterpret_cast<IUnknown*>(*ppv)->AddRef();
		return NOERROR;
	}

	HRESULT __stdcall EffectStateManager::SetTransform( D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix)
	{
		D3D::setTransform(State, pMatrix);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetMaterial( const D3DMATERIAL9 *pMaterial)
	{
		return D3D::getDevice()->SetMaterial(pMaterial);
	}

	HRESULT __stdcall EffectStateManager::SetLight( DWORD Index, const D3DLIGHT9 *pLight)
	{
		return D3D::getDevice()->SetLight(Index, pLight);
	}

	HRESULT __stdcall EffectStateManager::LightEnable( DWORD Index, BOOL Enable)
	{
		return D3D::getDevice()->LightEnable(Index, Enable);
	}

	HRESULT __stdcall EffectStateManager::SetRenderState( D3DRENDERSTATETYPE State, DWORD Value)
	{
		D3D::setRenderState(State, Value);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetTexture( DWORD Stage, LPDIRECT3DBASETEXTURE9 pTexture)
	{
		D3D::setTexture(Stage, pTexture);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetTextureStageState( DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
	{
		D3D::setTextureStageState(Stage, Type, Value);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetSamplerState( DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
	{
		D3D::setSamplerState(Sampler, Type, Value);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetNPatchMode( FLOAT NumSegments)
	{
		return D3D::getDevice()->SetNPatchMode(NumSegments);
	}

	HRESULT __stdcall EffectStateManager::SetFVF( DWORD FVF)
	{
		return D3D::getDevice()->SetFVF(FVF);
	}

	HRESULT __stdcall EffectStateManager::SetVertexShader( LPDIRECT3DVERTEXSHADER9 pShader)
	{
		D3D::setVertexShader(pShader);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetVertexShaderConstantF( UINT RegisterIndex, const FLOAT *pConstantData, UINT RegisterCount)
	{
		if (mNumBlockedVFRegs)
		{
			const int lastBlocked = mFirstBlockedVFReg + mNumBlockedVFRegs - 1;
			const int lastToSet = RegisterIndex + RegisterCount - 1;
			if ((lastToSet < mFirstBlockedVFReg) || (RegisterIndex > lastBlocked))
			{
				D3D::setVertexShaderConstantF(RegisterIndex, pConstantData, RegisterCount);
			}
			else
			{
				if (RegisterIndex < mFirstBlockedVFReg)
					D3D::setVertexShaderConstantF(RegisterIndex, pConstantData, mFirstBlockedVFReg - RegisterIndex);
				
				if (lastToSet > lastBlocked)
				{
					const int firstToSet = lastBlocked + 1;
					const int floatsToSkip = (firstToSet - RegisterIndex) * 4;
					const int numToSet = lastToSet - firstToSet + 1;
					D3D::setVertexShaderConstantF(firstToSet, pConstantData + floatsToSkip, numToSet);
				}
			}
    }
		else
			D3D::setVertexShaderConstantF(RegisterIndex, pConstantData, RegisterCount);

		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetVertexShaderConstantI( UINT RegisterIndex, const INT *pConstantData, UINT RegisterCount)
	{
		D3D::setVertexShaderConstantI(RegisterIndex, pConstantData, RegisterCount);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetVertexShaderConstantB( UINT RegisterIndex, const BOOL *pConstantData, UINT RegisterCount)
	{
		D3D::setVertexShaderConstantB(RegisterIndex, pConstantData, RegisterCount);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetPixelShader( LPDIRECT3DPIXELSHADER9 pShader)
	{
		D3D::setPixelShader(pShader);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetPixelShaderConstantF( UINT RegisterIndex, const FLOAT *pConstantData, UINT RegisterCount)
	{
		D3D::setPixelShaderConstantF(RegisterIndex, pConstantData, RegisterCount);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetPixelShaderConstantI( UINT RegisterIndex, const INT *pConstantData, UINT RegisterCount)
	{
		D3D::setPixelShaderConstantI(RegisterIndex, pConstantData, RegisterCount);
		return D3D_OK;
	}

	HRESULT __stdcall EffectStateManager::SetPixelShaderConstantB( UINT RegisterIndex, const BOOL *pConstantData, UINT RegisterCount)
	{
		D3D::setPixelShaderConstantB(RegisterIndex, pConstantData, RegisterCount);
		return D3D_OK;
	}

	void EffectStateManager::blockVertexShaderConstantFUploads(int startReg, int numRegs)
	{
		mFirstBlockedVFReg = startReg;
		mNumBlockedVFRegs = numRegs;
	}

	Effects::Effects(const char* pBasename) :
		mpEffects(NULL),
		mBasename(pBasename ? pBasename : ""),
		mLoadCounter(0)
	{
	}

	Effects::~Effects()
	{
		clear();
	}

	void Effects::load(const char* pBasename)
	{
		clear();

		if (pBasename)
			mBasename = pBasename;

		if (mBasename.empty())
			return;

		BigString path(mBasename + ".fxo");

		FILE* pFile = fopen(path, "rb");
		if (!pFile)
			path = mBasename + ".fx";
		else
			fclose(pFile);
				
		DWORD flags = 0;

#if DEBUG		
		//flags |= D3DXSHADER_DEBUG |  D3DXSHADER_SKIPOPTIMIZATION;
		flags |= D3DXSHADER_DEBUG;
#endif

		for ( ; ; )
		{
			Status("Effects::load: Loading \"%s\"\n", path.c_str());
			
			LPD3DXBUFFER pBuffer = NULL;
			HRESULT hr = D3DXCreateEffectFromFile(
				//D3D::getDevice(), 
				GetMyD3DDevice(),
				path, NULL, NULL, flags, NULL, &mpEffects, &pBuffer);
			if (SUCCEEDED(hr))
			{
				if (pBuffer)
					pBuffer->Release();
				break;
			}

			char buf[4096];
			sprintf(buf, "Effect File Compile Failed:\nFile: %s\n", path.c_str());

			if (pBuffer)
			{
				int l = static_cast<int>(strlen(buf));
				memcpy(buf + l, pBuffer->GetBufferPointer(), pBuffer->GetBufferSize());
				*(buf + l + pBuffer->GetBufferSize()) = '\0';
				
				pBuffer->Release();
			}
      
			int res = MessageBox(RenderEngine::mainWindow(), buf, "Error", MB_RETRYCANCEL | MB_ICONEXCLAMATION | MB_APPLMODAL);
			if (res == IDCANCEL)
				gSystem.abort();
		}

		if (mpEffects)
			mpEffects->SetStateManager(&mEffectStateManager);
			
		mLoadCounter++;
		
		Status("Effects::load: Done\n");
	}

	void Effects::reload(void)
	{
		load(getBasename());
	}

	void Effects::clear(void)
	{
		D3D::safeRelease(mpEffects);
	}

	bool Effects::isLoaded(void) const
	{
		return NULL != mpEffects;
	}

	void Effects::initDeviceObjects(void)
	{
		load();
	}

	void Effects::deleteDeviceObjects(void)
	{
		clear();
	}
		
	void Effects::invalidateDeviceObjects(void)
	{
		if (mpEffects)
			mpEffects->OnLostDevice();
	}

	void Effects::restoreDeviceObjects(void)
	{
		if (mpEffects)
			mpEffects->OnResetDevice();
	}

	const BigString& Effects::getBasename(void) const
	{
		return mBasename;
	}

	void Effects::setBasename(const char* pBasename)
	{
		mBasename = pBasename;
	}

  void Effects::setTexture(ETexture texture)
	{
		Assert(mpEffects);
		
		D3DXHANDLE handle = reinterpret_cast<D3DXHANDLE>(
			RenderEngine::bufferManager().getTextureEffectName(texture));
		
		HRESULT hr = mpEffects->SetTexture(handle, RenderEngine::bufferManager().getTexture(texture));
		Verify(SUCCEEDED(hr));
	}

	void Effects::setTexture(const char* pName, LPDIRECT3DBASETEXTURE9 pTexture)
	{
		Assert(mpEffects);
				
		D3DXHANDLE handle = reinterpret_cast<D3DXHANDLE>(pName);

		HRESULT hr = mpEffects->SetTexture(handle, pTexture);
		Verify(SUCCEEDED(hr));
	}
	
	void Effects::setMatrix(EMatrix matrix)
	{
		Assert(mpEffects);
		
		D3DXHANDLE handle = reinterpret_cast<D3DXHANDLE>(
			D3D::getMatrixTracker().getMatrixEffectName(matrix));

		const D3DXMATRIX* pMatrix = reinterpret_cast<const D3DXMATRIX*>(
			&D3D::getMatrixTracker().getMatrix(matrix, true));

		//const D3DXMATRIX* pMatrix = reinterpret_cast<const D3DXMATRIX*>(&getMatrix(matrix, false));
						    
		// FIXME: Use effect matrix handle for speed!
		HRESULT hr = mpEffects->SetMatrixTranspose(handle, pMatrix);
		//HRESULT hr = mpEffects->SetMatrix(handle, pMatrix);

		Verify(SUCCEEDED(hr));
	}

	void Effects::setMatrix(D3DXHANDLE handle, const Matrix44& m)
	{
		Assert(mpEffects);
		
		const D3DXMATRIX* pMatrix = reinterpret_cast<const D3DXMATRIX*>(&m);
						    
		HRESULT hr = mpEffects->SetMatrix(handle, pMatrix);
		
		Verify(SUCCEEDED(hr));
	}

	void Effects::beginTechnique(D3DXHANDLE hTechnique)
	{
		//D3D::invalidateCache();
		//D3D::flushCache();
		
    HRESULT hr = DebugNull(mpEffects)->SetTechnique(hTechnique);
		Verify(SUCCEEDED(hr));

		UINT numPasses;
		//hr = mpEffects->Begin(&numPasses, D3DXFX_DONOTSAVESHADERSTATE);// D3DXFX_DONOTSAVESTATE);
		hr = mpEffects->Begin(&numPasses, D3DXFX_DONOTSAVESHADERSTATE | D3DXFX_DONOTSAVESTATE);
		Verify(SUCCEEDED(hr));
		Verify(1 == numPasses);

		hr = mpEffects->Pass(0);
		Verify(SUCCEEDED(hr));
	}

	void Effects::endTechnique(void)
	{
		DebugNull(mpEffects)->End();
	}

	void Effects::setFloat(D3DXHANDLE hParameter, float f)
	{
		HRESULT hr = DebugNull(mpEffects)->SetFloat(hParameter, f);
		Verify(SUCCEEDED(hr));
	}

	void Effects::setVector(D3DXHANDLE hParameter, const Vec4& v)
	{
		HRESULT hr = DebugNull(mpEffects)->SetVector(hParameter, reinterpret_cast<const D3DXVECTOR4*>(&v));
		Verify(SUCCEEDED(hr));
	}

	void Effects::setVectorArray(D3DXHANDLE hParameter, const void* pV, int numVectors)
	{
		HRESULT hr = DebugNull(mpEffects)->SetValue(hParameter, reinterpret_cast<const void*>(pV), numVectors * sizeof(Vec4));
		Verify(SUCCEEDED(hr));
	}

	void Effects::setMatrixArray(D3DXHANDLE hParameter, const void* pV, int numMatrices)
	{
		HRESULT hr = DebugNull(mpEffects)->SetMatrixArray(hParameter, reinterpret_cast<const D3DXMATRIX*>(pV), numMatrices);
		Verify(SUCCEEDED(hr));
	}

	void Effects::blockVertexShaderConstantFUploads(int startReg, int numRegs)
	{
		mEffectStateManager.blockVertexShaderConstantFUploads(startReg, numRegs);
	}
	
	void Effects::setPixelShader(D3DXHANDLE handle)
	{
		LPDIRECT3DPIXELSHADER9 pShader;
		HRESULT hr = DebugNull(mpEffects)->GetPixelShader(handle, &pShader);
		Verify(SUCCEEDED(hr));
		
		D3D::setPixelShader(pShader);
	}
	
	void Effects::setVertexShader(D3DXHANDLE handle)
	{
		LPDIRECT3DVERTEXSHADER9 pShader;
		HRESULT hr = DebugNull(mpEffects)->GetVertexShader(handle, &pShader);
		Verify(SUCCEEDED(hr));
		
		D3D::setVertexShader(pShader);
	}
	
	LPDIRECT3DVERTEXSHADER9 Effects::findVertexShader(const std::vector<Annotation>& annotations)
	{
		D3DXHANDLE handle = findShader(annotations, eVertexShader);
		if (!handle)
			return NULL;
			
		LPDIRECT3DVERTEXSHADER9 pShader = NULL;
		HRESULT hr = DebugNull(mpEffects)->GetVertexShader(handle, &pShader);
		
		pShader->Release();
		
		return pShader;
	}
	
	LPDIRECT3DPIXELSHADER9 Effects::findPixelShader(const std::vector<Annotation>& annotations)
	{
		D3DXHANDLE handle = findShader(annotations, ePixelShader);
		if (!handle)
			return NULL;
			
		LPDIRECT3DPIXELSHADER9 pShader = NULL;
		HRESULT hr = DebugNull(mpEffects)->GetPixelShader(handle, &pShader);
		
		pShader->Release();
				
		return pShader;
	}
	
	D3DXHANDLE Effects::findShader(const std::vector<Annotation>& annotations, EShaderType shaderType)
	{
#if LOG		
		Status("Effects::findShader: Linking %s shader:\n", (shaderType == ePixelShader) ? "pixel" : "vertex");
		for (int i = 0; i < annotations.size(); i++)
			Status("  %s: %i\n", annotations[i].name, annotations[i].value);
#endif
		
		D3DXEFFECT_DESC effectDesc;
		mpEffects->GetDesc(&effectDesc);
		
		const int numParams = effectDesc.Parameters;
		
		for (int paramIndex = 0; paramIndex < numParams; paramIndex++)
		{
			D3DXHANDLE paramHandle = mpEffects->GetParameter(NULL, paramIndex);
			
			D3DXPARAMETER_DESC paramDesc;
			mpEffects->GetParameterDesc(paramHandle, &paramDesc);
			
			if (
					((paramDesc.Type == D3DXPT_PIXELSHADER) && (shaderType == ePixelShader)) ||
					((paramDesc.Type == D3DXPT_VERTEXSHADER) && (shaderType == eVertexShader))
					)
			{
				for (int i = 0; i < annotations.size(); i++)
				{
					D3DXHANDLE annotationHandle = mpEffects->GetAnnotationByName(paramHandle, annotations[i].name);
					if (NULL == annotationHandle)
						break;
					
					D3DXPARAMETER_DESC annoDesc;
					mpEffects->GetParameterDesc(annotationHandle, &annoDesc);
					
					if ((annoDesc.Type == D3DXPT_BOOL) || (annoDesc.Type == D3DXPT_INT))
					{
						int t;
						mpEffects->GetValue(annotationHandle, &t, sizeof(t));
						
						if (annotations[i].value != t)
							break;
					}
					else
						break;
				}
				
				if (i == annotations.size())
				{
#if LOG				
					Status("  Found shader: \"%s\"\n", paramDesc.Name);
#endif					
					return paramHandle;
				}
			}
		}

#if LOG		
		Status("Effects::findShader: Link failed!\n");
#endif		
		
		return NULL;
	}
			
} // namespace gr

