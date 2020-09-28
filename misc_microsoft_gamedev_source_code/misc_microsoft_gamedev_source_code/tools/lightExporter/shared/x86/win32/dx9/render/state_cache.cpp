//-----------------------------------------------------------------------------
// File: state_cache.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "device.h"

namespace gr
{
	StateCache::StateCache()
	{
		invalidateTextures();
	}
	
	void StateCache::flush(void)
	{
		mRenderStateTracker.flush(renderStateSetFunc);
		mTextureStateTracker.flush(textureStateSetFunc);
		mSamplerStateTracker.flush(samplerStateSetFunc);

		flushTextures();
	}

	void StateCache::invalidate(void)
	{
		flush();

		mRenderStateTracker.invalidate();
		mTextureStateTracker.invalidate();
		mSamplerStateTracker.invalidate();

		invalidateTextures();
	}

	void StateCache::check(void)
	{
		flush();

		mRenderStateTracker.check(renderStateCheckFunc);
		mTextureStateTracker.check(textureStateCheckFunc);
		mSamplerStateTracker.check(samplerStateCheckFunc);

    checkTextures();
	}

	void StateCache::renderStateSetFunc(DWORD state, DWORD value)
	{
		D3D::setRenderStateUncached((D3DRENDERSTATETYPE)state, value);
	}

	void StateCache::textureStateSetFunc(DWORD state, DWORD value)
	{
		const DWORD type = state & (NumTextures - 1);
		const DWORD stage = state >> NumTexturesLog2;
		D3D::setTextureStageStateUncached(stage, (D3DTEXTURESTAGESTATETYPE)type, value);
	}

	void StateCache::samplerStateSetFunc(DWORD state, DWORD value)
	{
		const DWORD type = state & (NumSamplers - 1);
		const DWORD sampler = state >> NumSamplersLog2;
		D3D::setSamplerStateUncached(sampler, (D3DSAMPLERSTATETYPE)type, value);
	}

	void StateCache::flushTextures(void)
	{
		if (mTexturesChanged)
		{
			mTexturesChanged = false;
			
			for (int i = 0; i < NumTextures; i++)
			{
				if (mpDesiredTexture[i] != mpCurTexture[i])
				{
					mpCurTexture[i] = mpDesiredTexture[i];
					Assert(reinterpret_cast<IDirect3DBaseTexture9*>(InvalidTexturePointerVal) != mpDesiredTexture[i]);
					D3D::setTextureUncached(i, mpDesiredTexture[i]);
				}
			}
		}
	}

	void StateCache::invalidateTextures(void)
	{
		std::fill(mpCurTexture, mpCurTexture + NumTextures, reinterpret_cast<IDirect3DBaseTexture9*>(InvalidTexturePointerVal));
		std::fill(mpDesiredTexture, mpDesiredTexture + NumTextures, reinterpret_cast<IDirect3DBaseTexture9*>(InvalidTexturePointerVal));
		mTexturesChanged = true;
	}

	void StateCache::checkTextures(void)
	{
		for (int i = 0; i < NumTextures; i++)
		{
			if (mpCurTexture[i] != reinterpret_cast<IDirect3DBaseTexture9*>(InvalidTexturePointerVal))
			{
				IDirect3DBaseTexture9* pCur = D3D::getTextureUncached(i);
				if (pCur)
				{
					pCur->Release();
					Verify(pCur != mpCurTexture[i]);
				}
			}
		}
	}

	void StateCache::renderStateCheckFunc(DWORD state, DWORD value)
	{
		switch ((D3DRENDERSTATETYPE)state)
		{
			case     D3DRS_ZENABLE                     :
			case     D3DRS_FILLMODE                    :
			case     D3DRS_SHADEMODE                   :
			case     D3DRS_ZWRITEENABLE                :
			case     D3DRS_ALPHATESTENABLE             :
			case     D3DRS_LASTPIXEL                   :
			case     D3DRS_SRCBLEND                    :
			case     D3DRS_DESTBLEND                   :
			case     D3DRS_CULLMODE                    :
			case     D3DRS_ZFUNC                       :
			case     D3DRS_ALPHAREF                    :
			case     D3DRS_ALPHAFUNC                   :
			case     D3DRS_DITHERENABLE                :
			case     D3DRS_ALPHABLENDENABLE            :
			case     D3DRS_FOGENABLE                   :
			case     D3DRS_SPECULARENABLE              :
			case     D3DRS_FOGCOLOR                    :
			case     D3DRS_FOGTABLEMODE                :
			case     D3DRS_FOGSTART                    :
			case     D3DRS_FOGEND                      :
			case     D3DRS_FOGDENSITY                  :
			case     D3DRS_RANGEFOGENABLE              :
			case     D3DRS_STENCILENABLE               :
			case     D3DRS_STENCILFAIL                 :
			case     D3DRS_STENCILZFAIL                :
			case     D3DRS_STENCILPASS                 :
			case     D3DRS_STENCILFUNC                 :
			case     D3DRS_STENCILREF                  :
			case     D3DRS_STENCILMASK                 :
			case     D3DRS_STENCILWRITEMASK            :
			case     D3DRS_TEXTUREFACTOR               :
			case     D3DRS_WRAP0                       :
			case     D3DRS_WRAP1                       :
			case     D3DRS_WRAP2                       :
			case     D3DRS_WRAP3                       :
			case     D3DRS_WRAP4                       :
			case     D3DRS_WRAP5                       :
			case     D3DRS_WRAP6                       :
			case     D3DRS_WRAP7                       :
			case     D3DRS_CLIPPING                    :
			case     D3DRS_LIGHTING                    :
			case     D3DRS_AMBIENT                     :
			case     D3DRS_FOGVERTEXMODE               :
			case     D3DRS_COLORVERTEX                 :
			case     D3DRS_LOCALVIEWER                 :
			case     D3DRS_NORMALIZENORMALS            :
			case     D3DRS_DIFFUSEMATERIALSOURCE       :
			case     D3DRS_SPECULARMATERIALSOURCE      :
			case     D3DRS_AMBIENTMATERIALSOURCE       :
			case     D3DRS_EMISSIVEMATERIALSOURCE      :
			case     D3DRS_VERTEXBLEND                 :
			case     D3DRS_CLIPPLANEENABLE             :
			case     D3DRS_POINTSIZE                   :
			case     D3DRS_POINTSIZE_MIN               :
			case     D3DRS_POINTSPRITEENABLE           :
			case     D3DRS_POINTSCALEENABLE            :
			case     D3DRS_POINTSCALE_A                :
			case     D3DRS_POINTSCALE_B                :
			case     D3DRS_POINTSCALE_C                :
			case     D3DRS_MULTISAMPLEANTIALIAS        :
			case     D3DRS_MULTISAMPLEMASK             :
			case     D3DRS_PATCHEDGESTYLE              :
			case     D3DRS_DEBUGMONITORTOKEN           :
			case     D3DRS_POINTSIZE_MAX               :
			case     D3DRS_INDEXEDVERTEXBLENDENABLE    :
			case     D3DRS_COLORWRITEENABLE            :
			case     D3DRS_TWEENFACTOR                 :
			case     D3DRS_BLENDOP                     :
			case     D3DRS_POSITIONDEGREE              :
			case     D3DRS_NORMALDEGREE                :
			case     D3DRS_SCISSORTESTENABLE           :
			case     D3DRS_SLOPESCALEDEPTHBIAS         :
			case     D3DRS_ANTIALIASEDLINEENABLE       :
			case     D3DRS_MINTESSELLATIONLEVEL        :
			case     D3DRS_MAXTESSELLATIONLEVEL        :
			case     D3DRS_ADAPTIVETESS_X              :
			case     D3DRS_ADAPTIVETESS_Y              :
			case     D3DRS_ADAPTIVETESS_Z              :
			case     D3DRS_ADAPTIVETESS_W              :
			case     D3DRS_ENABLEADAPTIVETESSELLATION  :
			case     D3DRS_TWOSIDEDSTENCILMODE         :
			case     D3DRS_CCW_STENCILFAIL             :
			case     D3DRS_CCW_STENCILZFAIL            :
			case     D3DRS_CCW_STENCILPASS             :
			case     D3DRS_CCW_STENCILFUNC             :
			case     D3DRS_COLORWRITEENABLE1           :
			case     D3DRS_COLORWRITEENABLE2           :
			case     D3DRS_COLORWRITEENABLE3           :
			case     D3DRS_BLENDFACTOR                 :
			case     D3DRS_SRGBWRITEENABLE             :
			case     D3DRS_DEPTHBIAS                   :
			case     D3DRS_WRAP8                       :
			case     D3DRS_WRAP9                       :
			case     D3DRS_WRAP10                      :
			case     D3DRS_WRAP11                      :
			case     D3DRS_WRAP12                      :
			case     D3DRS_WRAP13                      :
			case     D3DRS_WRAP14                      :
			case     D3DRS_WRAP15                      :
			case     D3DRS_SEPARATEALPHABLENDENABLE    :
			case     D3DRS_SRCBLENDALPHA               :
			case     D3DRS_DESTBLENDALPHA              :
			case     D3DRS_BLENDOPALPHA                :
			{
				Verify(value == D3D::getRenderStateUncached((D3DRENDERSTATETYPE)state));
				break;
			}
			default:
				break;
		}
	}

	void StateCache::textureStateCheckFunc(DWORD state, DWORD value)
	{
		const D3DTEXTURESTAGESTATETYPE type = (D3DTEXTURESTAGESTATETYPE)(state & (NumTextures - 1));
		const DWORD stage = state >> NumTexturesLog2;

		switch (type)
		{
      case    D3DTSS_COLOROP        :
			case    D3DTSS_COLORARG1      :
			case    D3DTSS_COLORARG2      :
			case    D3DTSS_ALPHAOP        :
			case    D3DTSS_ALPHAARG1      :
			case    D3DTSS_ALPHAARG2      :
			case    D3DTSS_BUMPENVMAT00   :
			case    D3DTSS_BUMPENVMAT01   :
			case    D3DTSS_BUMPENVMAT10   :
			case    D3DTSS_BUMPENVMAT11   :
			case    D3DTSS_TEXCOORDINDEX  :
			case    D3DTSS_BUMPENVLSCALE  :
			case    D3DTSS_BUMPENVLOFFSET :
			case    D3DTSS_TEXTURETRANSFORMFLAGS  :
			case    D3DTSS_COLORARG0      :
			case    D3DTSS_ALPHAARG0      :
			case    D3DTSS_RESULTARG      :
			case    D3DTSS_CONSTANT       :
			{
				Verify(value == D3D::getTextureStageStateUncached(stage, type));
				break;
			}
			default:
				break;
		}
	}

	void StateCache::samplerStateCheckFunc(DWORD state, DWORD value)
	{
		const D3DSAMPLERSTATETYPE type = (D3DSAMPLERSTATETYPE)(state & (NumSamplers - 1));
		const DWORD sampler = state >> NumSamplersLog2;

		switch (type)
		{
			case    D3DSAMP_ADDRESSU       : 
			case    D3DSAMP_ADDRESSV       : 
			case    D3DSAMP_ADDRESSW       : 
			case    D3DSAMP_BORDERCOLOR    : 
			case     D3DSAMP_MAGFILTER      : 
			case     D3DSAMP_MINFILTER      : 
			case     D3DSAMP_MIPFILTER      : 
			case     D3DSAMP_MIPMAPLODBIAS  : 
			case     D3DSAMP_MAXMIPLEVEL    : 
			case     D3DSAMP_MAXANISOTROPY  : 
			case     D3DSAMP_SRGBTEXTURE    :
			case     D3DSAMP_ELEMENTINDEX   : 
			case     D3DSAMP_DMAPOFFSET     : 
			{
				Verify(value == D3D::getSamplerStateUncached(sampler, type));
				break;
			}
			default:
				break;
		}
	}
} // namespace gr
