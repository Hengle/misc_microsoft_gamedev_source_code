// File: render_helpers.h
#ifndef RENDER_HELPERS_H
#define RENDER_HELPERS_H
#pragma once

#include "vertex_decl_manager.h"

namespace gr
{
	struct RenderHelpers
	{
    static void makeSSQ(P4Vertex* quadVerts, bool viewVector = false, float width = 1.0f, float height = 1.0f, float ofsX = 0.0f, float ofsY = 0.0f);
		static void renderSSQ(bool viewVector = false, float width = 1.0f, float height = 1.0f, float ofsX = 0.0f, float ofsY = 0.0f);
		
		static void makeSSQ(P4Vertex* quadVerts, const RECT& dstRect, const RECT& texRect, int texWidth, int texHeight);
		static void renderSSQ(const RECT& dstRect, const RECT& texRect, int texWidth, int texHeight);
		
		static HRESULT saveRenderTarget(IDirect3DSurface9* pSurf, const char* pFilename);
		static void getRenderTargetBits(std::vector<uchar>& bits, IDirect3DSurface9* pSrcSurf);
		static void getSurfaceBits(std::vector<uchar>& bits, IDirect3DSurface9* pSrcSurf);
	};

} // namespace gr

#endif // RENDER_HELPERS_H