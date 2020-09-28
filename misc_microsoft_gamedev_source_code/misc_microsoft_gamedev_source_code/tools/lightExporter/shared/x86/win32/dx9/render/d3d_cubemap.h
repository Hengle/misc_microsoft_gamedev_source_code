// File: d3d_cubemap.h
#pragma once
#ifndef D3D_CUBEMAP_H
#define D3D_CUBEMAP_H

#include "common/render/cubemap.h"
#include "d3d9.h"
#include <d3dx9.h>

namespace gr
{
	class D3DCubeMap : public CubeMap
	{
	public:
		typedef std::vector<Vec3> Vec3Vec;
		
		D3DCubeMap(int width, int height);
		D3DCubeMap(IDirect3DCubeTexture9* pCubeTex);
						
		IDirect3DCubeTexture9* get(IDirect3DCubeTexture9* pCubeTex) const;
		IDirect3DCubeTexture9* set(IDirect3DCubeTexture9* pCubeTex);
	};
	
} // namespace gr

#endif // D3D_CUBEMAP_H
