// File: d3d_cubemap.cpp
#include "d3d_cubemap.h"
#include "common/math/half_float.h"

namespace gr
{
	D3DCubeMap::D3DCubeMap(int width, int height) : CubeMap(width, height)
	{
	}
			
	D3DCubeMap::D3DCubeMap(IDirect3DCubeTexture9* pCubeTex) : CubeMap(0, 0)
	{
		set(pCubeTex);
	}
	
	IDirect3DCubeTexture9* D3DCubeMap::get(IDirect3DCubeTexture9* pCubeTex) const
	{
		for (int i = 0; i < 6; i++)
		{
			IDirect3DSurface9* pCubeSurf;
			Verify(SUCCEEDED(pCubeTex->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES>(i), 0, &pCubeSurf)));
							
			D3DSURFACE_DESC desc;
			pCubeSurf->GetDesc(&desc);
			
			Verify(desc.Format == D3DFMT_A16B16G16R16F);
			Verify(desc.Width == mWidth);
			Verify(desc.Height == mHeight);
									
			D3DLOCKED_RECT rect;
			Verify(SUCCEEDED(pCubeSurf->LockRect(&rect, NULL, 0)));			
			
			const int bytesPerComp = sizeof(uint16);
			const int bytesPerPixel = 4 * bytesPerComp;
			
			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					for (int c = 0; c < 4; c++)
					{	
						uint16* pDst = reinterpret_cast<uint16*>(
							reinterpret_cast<uchar*>(rect.pBits) + 
							x * bytesPerPixel + 
							c * bytesPerComp + 
							y * rect.Pitch 
						);
												
						if (c < 3)
							*pDst = Half::FloatToHalf(mFaces[i].at(x + y * mWidth)[c], false);
						else
							*pDst = Half::FloatToHalf(1.0f);
					}
				}
			}
			
			pCubeSurf->UnlockRect();
						
			pCubeSurf->Release();
		}
		
		return pCubeTex;
	}
	
	IDirect3DCubeTexture9* D3DCubeMap::set(IDirect3DCubeTexture9* pCubeTex)
	{
		for (int i = 0; i < 6; i++)
		{
			IDirect3DSurface9* pCubeSurf;
			Verify(SUCCEEDED(pCubeTex->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES>(i), 0, &pCubeSurf)));
							
			D3DSURFACE_DESC desc;
			pCubeSurf->GetDesc(&desc);
			
			Verify(desc.Format == D3DFMT_A16B16G16R16F);
			
			mWidth = desc.Width;
			mHeight = desc.Height;
			
			mFaces[i].resize(mWidth * mHeight);
			
			D3DLOCKED_RECT rect;
			Verify(SUCCEEDED(pCubeSurf->LockRect(&rect, NULL, 0)));			
			
			const int bytesPerComp = sizeof(uint16);
			const int bytesPerPixel = 4 * bytesPerComp;
			
			for (int y = 0; y < mHeight; y++)
			{
				for (int x = 0; x < mWidth; x++)
				{
					for (int c = 0; c < 3; c++)
					{	
						const uint16 h = *reinterpret_cast<const uint16*>(
							reinterpret_cast<const uchar*>(rect.pBits) + 
							x * bytesPerPixel + 
							c * bytesPerComp + 
							y * rect.Pitch 
						);
												
						mFaces[i].at(x + y * mWidth)[c] = Half::HalfToFloat(h, true);
					}
				}
			}
			
			pCubeSurf->UnlockRect();
						
			pCubeSurf->Release();
		}
		
		return pCubeTex;
	}
		
} // namespace gr

