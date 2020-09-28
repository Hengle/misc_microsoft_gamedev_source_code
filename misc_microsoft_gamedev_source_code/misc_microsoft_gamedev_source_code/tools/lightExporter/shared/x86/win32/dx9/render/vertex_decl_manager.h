//-----------------------------------------------------------------------------
// File: vertex_decl_manager.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef VERTEX_DECL_MANAGER_H
#define VERTEX_DECL_MANAGER_H

#include <d3d9.h>
#include <d3dx9.h>

#include "common/math/vector.h"
#include "x86/win32/dx9/render/device.h"
#include "common/geom/univert_packer.h"

struct D3DXVECTOR4;
struct D3DXVECTOR3;

namespace gr
{
	// Vertex Declarations

	enum EVertexDeclaration
	{
		eInvalidDeclaration = -1,

		ePND1Declaration,
		ePTBND1Declaration,
		ePIWTBND1Declaration,
		ePIWND1Declaration,
		ePIWDeclaration,
		ePIWNDeclaration,
		eP4Declaration,
		eTileVertexDeclaration,
		eTileColorVertexDeclaration,
		eParticleVertexDeclaration,
    
		eNumVertexDeclarations
	};

	struct VertexDeclaration
	{
		EVertexDeclaration declID;
		D3DVERTEXELEMENT9* pElements;
		bool autoOffsets;
		int size;
	};

	// Vertex Structs

	struct PND1Vertex
	{
		Vec3 position;
		Vec3 normal;
		D3DCOLOR diffuse;
		Vec2 uv;
	};

	struct PTBND1Vertex
	{
		Vec3 position;
		Vec<4> tangent;
		Vec<4> binormal;
		Vec3 normal;
		D3DCOLOR diffuse;
		Vec2 uv;
	};

	struct PIWVertex
	{
		Vec3 position;
		uchar indices[4];
		uchar weights[4];
	};

	struct PIWNVertex : PIWVertex
	{
		Vec3 normal;
	};

	struct PIWTBND1Vertex : PIWVertex
	{
		Vec<4> tangent;
		Vec<4> binormal;
		Vec3 normal;
		D3DCOLOR diffuse;
		Vec2 uv;
	};

	struct PIWND1Vertex : PIWVertex
	{
		Vec3 normal;
		D3DCOLOR diffuse;
		Vec2 uv;
	};
	
	struct P4Vertex
	{
		Vec<4> position;
		Vec<4> uv[4];
	};

	struct TileVertex
	{
		ushort x;
		ushort y;
		float z;
		
		void set(ushort X, ushort Y, float Z)
		{
			x = X;
			y = Y;
			z = Z;
		}
	};

	struct TileColorVertex
	{
		ushort x;
		ushort y;
		float z;
		D3DCOLOR color;
		
		void set(ushort X, ushort Y, float Z, D3DCOLOR C)
		{
			x = X;
			y = Y;
			z = Z;
			color = C;
		}
	};
	
	struct ParticleVertex
	{
		D3DXVECTOR3 position;
		DWORD diffuse;
		float intensity;
		float	tu, tv;
	};
			
	class VertexDecl
	{
	public:
		enum { MaxStreams = 8 };
								
		VertexDecl() :
			mpDecl(NULL)
		{
		}
		
		VertexDecl(const std::vector<UnivertPacker>& packers) :
			mpDecl(NULL),
			mUnivertPackers(packers)
		{
			create();
		}
		
		VertexDecl(const VertexDecl& rhs) :
			mpDecl(NULL)
		{
			*this = rhs;
		}
		
		~VertexDecl()
		{
			clear();
		}
						
		VertexDecl& operator= (const VertexDecl& rhs)
		{
			if (this == &rhs)
				return *this;
				
			clear();
			
			mUnivertPackers = rhs.mUnivertPackers;
			
			create();
				
			return *this;
		}
		
		bool operator== (const VertexDecl& rhs) const
		{
			return *this == rhs.mUnivertPackers;
		}
		
		bool operator== (const std::vector<UnivertPacker>& packers) const;
			
		IDirect3DVertexDeclaration9* get(void) const 
		{
			return mpDecl;	
		}
		
		void setToDevice(void) const
		{	
			D3D::setVertexDeclaration(get());	
		}
		
		void initDeviceObjects(void)
		{
			create();
		}
		
		void deleteDeviceObjects(void)
		{
			D3D::safeRelease(mpDecl);
		}
					
	private:
		IDirect3DVertexDeclaration9* mpDecl;
		std::vector<UnivertPacker> mUnivertPackers;
		
		void clear(void)
		{
			D3D::safeRelease(mpDecl);
			mUnivertPackers.clear();
		}
		
		void create(void);
				
		static BYTE convertType(VertexElement::EType type);
	};

	class VertexDeclManager
	{
	public:
		VertexDeclManager();
		~VertexDeclManager();
		
		void initDeviceObjects(void);
		void deleteDeviceObjects(void);
		
		void setToDevice(EVertexDeclaration vertexDeclarationIndex) const;
		int getSize(EVertexDeclaration vertexDeclarationIndex) const;

		typedef int Handle;		
		const Handle create(const std::vector<UnivertPacker>& packers);
		void setToDevice(Handle handle) const;
						
	private:
		std::vector<IDirect3DVertexDeclaration9*> mFixedVertexDecls;
		std::vector<VertexDecl> mVertexDeclCache;

		void clear(void);
		void initVertexDeclarations(void);
	
		static int getVertexDeclarationTypeSize(BYTE type);
		static int createVertexDeclarationOffsets(D3DVERTEXELEMENT9* pElements);
	};
		  
} // namespace gr

#endif // VERTEX_DECL_MANAGER_H




