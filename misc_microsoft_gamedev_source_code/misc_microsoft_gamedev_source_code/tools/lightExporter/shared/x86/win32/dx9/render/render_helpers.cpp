// File: render_helpers.cpp
#include "render_helpers.h"
#include "render_engine.h"

namespace gr
{
	// Assumes texture and render target dimensions are equal!
	void RenderHelpers::makeSSQ(P4Vertex* quadVerts, bool viewVector, float width, float height, float ofsX, float ofsY)
  {
		const Matrix44& screenToView = D3D::getMatrix(eScreenToView);
    
    memset(quadVerts, 0, sizeof(P4Vertex) * 6);

		const D3DVIEWPORT9& curViewport = D3D::getViewport();
        
    for (int i = 0; i < 6; i++)
    {
			static const float gQuadXOrder[] = { 0,1,1, 0,1,0 };
			static const float gQuadYOrder[] = { 0,0,1, 0,1,1 };

      const float x = gQuadXOrder[i];
      const float y = gQuadYOrder[i];
						
			const Vec4 o(Vec4(ofsX + curViewport.X, ofsY + curViewport.Y, 0, 1.0f));
			const Vec4 d(Vec4::multiply(Vec4(width * curViewport.Width, height * curViewport.Height, 0, 0), Vec4(x, y, 0, 0)));
      
      const Vec4 screenPos(o + d);
                           
      const Vec4 viewPos((screenPos * screenToView).toPoint());

      quadVerts[i].position = Vec4(-.5f, -.5f, 0.0f, 0.0f) + screenPos;
      quadVerts[i].uv[0] = Vec<4>(screenPos.x / D3D::getRenderTargetWidth(), screenPos.y / D3D::getRenderTargetHeight(), 0, 1.0f);
      quadVerts[i].uv[1] = viewVector ? viewPos : Vec<4>(0,0,0,1);
      quadVerts[i].uv[2] = Vec<4>(0,0,0,1);
      quadVerts[i].uv[3] = Vec<4>(0,0,0,1);
    }
  }

	void RenderHelpers::renderSSQ(bool viewVector, float width, float height, float ofsX, float ofsY)
	{
		P4Vertex quadVerts[6];
		makeSSQ(quadVerts, viewVector, width, height, ofsX, ofsY);
	
		RenderEngine::vertexDeclManager().setToDevice(eP4Declaration);
			
		D3D::drawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadVerts, sizeof(quadVerts[0]));
	}
	
	void RenderHelpers::makeSSQ(P4Vertex* quadVerts, const RECT& dstRect, const RECT& texRect, int texWidth, int texHeight)
  {
    memset(quadVerts, 0, sizeof(P4Vertex) * 6);
		       
    for (int i = 0; i < 6; i++)
    {
			static const float gQuadXOrder[] = { 0,1,1, 0,1,0 };
			static const float gQuadYOrder[] = { 0,0,1, 0,1,1 };

      const float x = gQuadXOrder[i];
      const float y = gQuadYOrder[i];

      const Vec4 screenPos(
				dstRect.left + (dstRect.right - dstRect.left) * x,
				dstRect.top + (dstRect.bottom - dstRect.top) * y,
				0.0f,
				1.0f);
                                
      quadVerts[i].position = Vec4(-.5f, -.5f, 0.0f, 0.0f) + screenPos;
      
      const float texX = texRect.left + (texRect.right - texRect.left) * x;
      const float texY = texRect.top + (texRect.bottom - texRect.top) * y;
      quadVerts[i].uv[0] = Vec<4>(texX / float(texWidth), texY / float(texHeight), 0, 1.0f);
      quadVerts[i].uv[1] = Vec<4>(x,y,0,1);
      quadVerts[i].uv[2] = Vec<4>(0,0,0,1);
      quadVerts[i].uv[3] = Vec<4>(0,0,0,1);
    }
  }
	
	void RenderHelpers::renderSSQ(const RECT& dstRect, const RECT& texRect, int texWidth, int texHeight)
	{
		P4Vertex quadVerts[6];
		makeSSQ(quadVerts, dstRect, texRect, texWidth, texHeight);
	
		RenderEngine::vertexDeclManager().setToDevice(eP4Declaration);
			
		D3D::drawPrimitiveUP(D3DPT_TRIANGLELIST, 2, quadVerts, sizeof(quadVerts[0]));		
	}
	
	HRESULT RenderHelpers::saveRenderTarget(IDirect3DSurface9* pSurf, const char* pFilename)
	{
		D3DSURFACE_DESC desc;
		pSurf->GetDesc(&desc);
		
		IDirect3DTexture9* pTempTex = D3D::createTexture(
			desc.Width,
			desc.Height,
			1,
			0,
			desc.Format,
			D3DPOOL_SYSTEMMEM);

		IDirect3DSurface9* pTempSurf;
		pTempTex->GetSurfaceLevel(0, &pTempSurf);
		
		D3D::getRenderTargetData(pSurf, pTempSurf);
		
		HRESULT hres = D3DXSaveTextureToFile(pFilename, D3DXIFF_DDS, pTempTex, NULL);		
				
		pTempSurf->Release();
		pTempTex->Release();
		
		return hres;
	}
	
	void RenderHelpers::getSurfaceBits(std::vector<uchar>& bits, IDirect3DSurface9* pSrcSurf)
	{
		D3DSURFACE_DESC desc;
		pSrcSurf->GetDesc(&desc);
		
		D3DLOCKED_RECT rect;
		pSrcSurf->LockRect(&rect, NULL, D3DLOCK_READONLY);

		const uchar* pBits = reinterpret_cast<const uchar*>(rect.pBits);
		const int pitch = rect.Pitch;
		
		const int bytes = pitch * desc.Height;
		
		bits.resize(bytes);
		
		memcpy(&bits[0], pBits, bytes);
					
		pSrcSurf->UnlockRect();
	}
	
	void RenderHelpers::getRenderTargetBits(std::vector<uchar>& bits, IDirect3DSurface9* pSrcSurf)
  {
  	D3DSURFACE_DESC desc;
		pSrcSurf->GetDesc(&desc);

		IDirect3DSurface9* pDstSurf = D3D::createOffscreenPlainSurface(
			desc.Width,
			desc.Height,
			desc.Format,
			D3DPOOL_SYSTEMMEM);
            	
		D3D::getRenderTargetData(pSrcSurf, pDstSurf);

		getSurfaceBits(bits, pDstSurf);
				    
		pDstSurf->Release();
	}
			
} // namespace gr







