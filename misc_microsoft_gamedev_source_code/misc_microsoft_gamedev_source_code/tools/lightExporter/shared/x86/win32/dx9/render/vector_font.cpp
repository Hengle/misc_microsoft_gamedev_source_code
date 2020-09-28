//-----------------------------------------------------------------------------
// File: vector_font.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "vector_font.h"
#include "device.h"

#include "common\render\simplex_font.h"

namespace gr
{
	struct ModelVertex
	{
		Vec3 position;       // vertex position
		D3DCOLOR diffuse;
	};
	#define D3DFVF_MODELVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

	struct RHWVertex
	{
		Vec4 position;       // vertex position
		D3DCOLOR diffuse;
	};
	#define D3DFVF_RHWVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
	
	void VectorFont::textOutModelSpace(
		const char* pStr, 
		const Vec3& modelPos, 
		const Vec3& modelUp,
		const Vec3& modelRight, 
		const uint color)
	{
		if (!pStr)
			return;

		const int NumVertexBufferElements = 128;
		ModelVertex vertexBuffer[NumVertexBufferElements];

		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setFVF(D3DFVF_MODELVERTEX);
		
		int vbNext = 0;

		Vec3 pos(modelPos);

		while (*pStr)
		{
			int c = *pStr;
			if ((c < 32) || (c > 126)) c = 32;

      const int* pSrc = &simplex[c - 32][0];

			for (int n = 0; n < pSrc[0]; n++)
			{
				const int xOfs = pSrc[2+n*2+0];
				const int yOfs = pSrc[2+n*2+1];
				
				if (-1 == xOfs)
				{
					if (vbNext > 1)
						D3D::drawPrimitiveUP(D3DPT_LINESTRIP, vbNext - 1, &vertexBuffer[0], sizeof(ModelVertex));
					vbNext = 0;
				}
				else
				{
					Assert(vbNext < NumVertexBufferElements);
					vertexBuffer[vbNext].position = pos + modelRight * xOfs + modelUp * yOfs;
					vertexBuffer[vbNext].diffuse = color;
					vbNext++;
				}
			}

			if (vbNext > 1)
				D3D::drawPrimitiveUP(D3DPT_LINESTRIP, vbNext - 1, &vertexBuffer[0], sizeof(ModelVertex));
			vbNext = 0;
			
			pos += modelRight * pSrc[1];
      			
			pStr++;
		}
    
		D3D::setRenderState(D3DRS_LIGHTING, TRUE);
	}

	void VectorFont::textOutScreenSpace(
		const char* pStr, 
		const Vec2& screenPos, 
		const Vec2& screenUp,
		const Vec2& screenRight, 
		const uint color)
	{
		if (!pStr)
			return;

		D3D::flushCache();

		const int NumVertexBufferElements = 128;
		RHWVertex vertexBuffer[NumVertexBufferElements];

		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setFVF(D3DFVF_RHWVERTEX);
				
		int vbNext = 0;

		Vec4 pos(screenPos[0], screenPos[1], 0.0f, 1.0f);

		while (*pStr)
		{
			int c = *pStr;
			if ((c < 32) || (c > 126)) c = 32;

      const int* pSrc = &simplex[c - 32][0];

			for (int n = 0; n < pSrc[0]; n++)
			{
				const int xOfs = pSrc[2+n*2+0];
				const int yOfs = pSrc[2+n*2+1];
				
				if (-1 == xOfs)
				{
					if (vbNext > 1)
						D3D::drawPrimitiveUP(D3DPT_LINESTRIP, vbNext - 1, &vertexBuffer[0], sizeof(RHWVertex));
					
					vbNext = 0;
				}
				else
				{
					Assert(vbNext < NumVertexBufferElements);
					vertexBuffer[vbNext].position = pos + Vec4(screenRight * xOfs + screenUp * yOfs);
					vertexBuffer[vbNext].diffuse = color;
					vbNext++;
				}
			}

			if (vbNext > 1)
				D3D::drawPrimitiveUP(D3DPT_LINESTRIP, vbNext - 1, &vertexBuffer[0], sizeof(RHWVertex));
				
			vbNext = 0;
			
			pos += Vec4(screenRight * pSrc[1]);
      			
			pStr++;
		}
    
		D3D::setRenderState(D3DRS_LIGHTING, TRUE);
	}

	void VectorFont::textOutInit(void)
	{
		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		D3D::setTexture(0, NULL);
	}

	void VectorFont::textOutDeinit(void)
	{
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		D3D::setRenderState(D3DRS_LIGHTING, TRUE);
	}

	void VectorFont::renderLineInit(void)
	{
		D3D::setFVF(D3DFVF_MODELVERTEX);
		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setTexture(0, NULL);
	}

	void VectorFont::renderLineDeinit(void)
	{
		D3D::setRenderState(D3DRS_LIGHTING, TRUE);
	}

	void VectorFont::renderLine(
			const Vec3& s, 
			const Vec3& e,
			const uint color)
	{
		ModelVertex vertexBuffer[2];

		vertexBuffer[0].position = s;
		vertexBuffer[0].diffuse = color;

		vertexBuffer[1].position = e;
		vertexBuffer[1].diffuse = color;
		
		D3D::drawPrimitiveUP(D3DPT_LINELIST, 1, &vertexBuffer[0], sizeof(ModelVertex));
	}

} // namespace gr



