//-----------------------------------------------------------------------------
// File: vertex_decl_manager.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "vertex_decl_manager.h"

#include "common/utils/string.h"

#include <d3dx9.h>

namespace gr
{
	namespace 
	{
		// Vertex declarations

		D3DVERTEXELEMENT9 gPND1Elements[] = 
		{
			{ 0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,		0 },
			{ 0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
			{ 0, 0, D3DDECLTYPE_FLOAT2,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			D3DDECL_END()
		};

		D3DVERTEXELEMENT9 gP4Elements[] = 
		{
			{ 0, 0, D3DDECLTYPE_FLOAT4,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 },
			D3DDECL_END()
		};

		D3DVERTEXELEMENT9 gPTBND1Elements[] = 
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,		0 },
			{ 0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
			{ 0, 0, D3DDECLTYPE_FLOAT2,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			D3DDECL_END()
		};

		D3DVERTEXELEMENT9 gPIWTBND1Elements[] = 
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT4,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,		0 },
			{ 0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
			{ 0, 0, D3DDECLTYPE_FLOAT2,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			D3DDECL_END()
		};

		D3DVERTEXELEMENT9 gPIWND1Elements[] = 
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
			{ 0, 0, D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,		0 },
			{ 0, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
			{ 0, 0, D3DDECLTYPE_FLOAT2,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			D3DDECL_END()
		};

		D3DVERTEXELEMENT9 gPIWElements[] = 
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
			D3DDECL_END()
		};

		D3DVERTEXELEMENT9 gPIWNElements[] = 
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 },
			{ 0, 0, D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0 },
			{ 0, 0, D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,		0 },
			D3DDECL_END()
		};
		
		D3DVERTEXELEMENT9 gTileVertexElements[] = 
		{
			{ 0, 0, D3DDECLTYPE_SHORT2,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT1,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			D3DDECL_END()
		};

		D3DVERTEXELEMENT9 gTileColorVertexElements[] = 
		{
			{ 0, 0, D3DDECLTYPE_SHORT2,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_FLOAT1,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			{ 0, 0, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0 },
			D3DDECL_END()
		};
		
		D3DVERTEXELEMENT9 gParticleVertexElements[] = 
		{
			{ 0, 0, D3DDECLTYPE_FLOAT3,	  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0 },
			{ 0, 0, D3DDECLTYPE_UDEC3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
			{ 0, 0, D3DDECLTYPE_FLOAT1,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
			{ 0, 0, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
			D3DDECL_END()
		};

		VertexDeclaration gVertexDeclarations[] =
		{
			{ ePND1Declaration,				gPND1Elements,				true },
			{ ePTBND1Declaration,			gPTBND1Elements,			true },
			{ ePIWTBND1Declaration,		gPIWTBND1Elements,		true },
			{ ePIWND1Declaration,			gPIWND1Elements,			true },
			{ ePIWDeclaration,				gPIWElements,					true },
			{ ePIWNDeclaration,				gPIWNElements,				true },
			
			{ eP4Declaration,					gP4Elements,					true },
			{ eTileVertexDeclaration, gTileVertexElements,	true },
			{ eTileColorVertexDeclaration, gTileColorVertexElements,	true },
			{ eParticleVertexDeclaration, gParticleVertexElements, true },
		};

		const int NumVertexDeclarations = sizeof(gVertexDeclarations) / sizeof(gVertexDeclarations[0]);

	} // anonymous namespace
	
	bool VertexDecl::operator== (const std::vector<UnivertPacker>& packers) const
	{
		if (mUnivertPackers.size() != packers.size())
			return false;
		for (int i = 0; i < mUnivertPackers.size(); i++)
			if (mUnivertPackers[i] != packers[i])
				return false;
		return true;
	}
	
	// Creates a canonicalized vertex declaration.
	// Note that not every possible vertex format configuration is supported!
	void VertexDecl::create(void)
	{
		D3D::safeRelease(mpDecl);			
		
		if (mUnivertPackers.empty())
			return;

		const int numStreams = static_cast<int>(mUnivertPackers.size());
		Assert((numStreams >= 1) && (numStreams <= MaxStreams));

		const int MaxElements = 64;						
		D3DVERTEXELEMENT9 elements[MaxElements];
		Utils::ClearObj(elements);
		
		D3DVERTEXELEMENT9* pNext = elements;
														
		for (int streamIndex = 0; streamIndex < numStreams; streamIndex++)
		{
			const UnivertPacker& packer = mUnivertPackers[streamIndex];
									
			const char* pPackOrder = packer.declOrder();
			while (*pPackOrder)
			{
				const char c = toupper(*pPackOrder++);
				const char n = *pPackOrder++;
				const int usageIndex = Utils::ConvertHexChar(n);
				Assert(usageIndex >= 0);
				
				pNext->Stream = streamIndex;
				pNext->Method = D3DDECLMETHOD_DEFAULT;
				pNext->UsageIndex = usageIndex;
									
				switch (c)
				{
					// POS and NORM may appear once per stream.
					case ePOS_SPEC:
					{
						pNext->Type = convertType(packer.pos());
						pNext->Usage = D3DDECLUSAGE_POSITION;
						break;
					}
					case eNORM_SPEC:
					{
						pNext->Type = convertType(packer.norm());
						pNext->Usage = D3DDECLUSAGE_NORMAL;
						break;
					}
					// BASIS may appear multiple times per stream, in more than one stream.
					case eBASIS_SPEC:
					{
						pNext->Type = convertType(packer.basis());
						pNext->Usage = D3DDECLUSAGE_TANGENT;
						pNext++;
						
						pNext->Stream = streamIndex;
						pNext->Method = D3DDECLMETHOD_DEFAULT;
						pNext->Type = convertType(packer.basis());
						pNext->Usage = D3DDECLUSAGE_BINORMAL;
						pNext->UsageIndex = usageIndex;
						
						break;
					}
					// BASIS_SCALE may appear multiple times in one stream.
					// Each BASIS_SCALE is assigned to texcoords, starting at BasisScalesFirstTexcoordIndex.
					case eBASIS_SCALE_SPEC:
					{
						pNext->Type = convertType(packer.basisScales());
						pNext->Usage = D3DDECLUSAGE_TEXCOORD;
						break;
					}
					// TEXCOORDS may appear multiple times in one stream.
					case eTEXCOORDS_SPEC:
					{
						pNext->Type = convertType(packer.UV());
						pNext->Usage = D3DDECLUSAGE_TEXCOORD;
						break;
					}
					// SKIN may appear one time in a stream.
					case eSKIN_SPEC:
					{
						pNext->Type = convertType(packer.indices());
						pNext->Usage = D3DDECLUSAGE_BLENDINDICES;
						pNext++;
						
						pNext->Stream = streamIndex;
						pNext->Method = D3DDECLMETHOD_DEFAULT;
						pNext->Type = convertType(packer.weights());
						pNext->Usage = D3DDECLUSAGE_BLENDWEIGHT;
						pNext->UsageIndex = usageIndex;
						break;
					}
					// DIFFUSE may appear one time in a stream.
					case eDIFFUSE_SPEC:
					{
						pNext->Type = convertType(packer.diffuse());
						pNext->Usage = D3DDECLUSAGE_COLOR;
						break;
					}
					// INDEX may appear one time in a stream.
					case eINDEX_SPEC:
					{
						pNext->Type = convertType(packer.index());
						pNext->Usage = D3DDECLUSAGE_BLENDINDICES;
						break;
					}
					default:
						Assert(false);
				}
				
				pNext++;
			}
		}
		
		const D3DVERTEXELEMENT9 endDecl = {0xFF, 0, D3DDECLTYPE_UNUSED, 0, 0, 0};
		*pNext++ = endDecl;
		
		Assert((pNext - elements)	<= MaxElements);
		
		D3D::setVertexDeclarationOffsets(elements);
		
		D3D::dumpVertexDeclaration(elements);
		
		mpDecl = D3D::createVertexDeclaration(elements);
		Verify(mpDecl);
	}
	
	BYTE VertexDecl::convertType(VertexElement::EType type)
	{
		switch (type)
		{
			case VertexElement::eFLOAT1:			return D3DDECLTYPE_FLOAT1;
			case VertexElement::eFLOAT2:			return D3DDECLTYPE_FLOAT2;
			case VertexElement::eFLOAT3:			return D3DDECLTYPE_FLOAT3;
			case VertexElement::eFLOAT4:			return D3DDECLTYPE_FLOAT4;
			case VertexElement::eD3DCOLOR:		return D3DDECLTYPE_D3DCOLOR;
			case VertexElement::eUBYTE4:			return D3DDECLTYPE_UBYTE4;
			case VertexElement::eSHORT2:			return D3DDECLTYPE_SHORT2;
			case VertexElement::eSHORT4:			return D3DDECLTYPE_SHORT4;
			case VertexElement::eUBYTE4N:			return D3DDECLTYPE_UBYTE4N;
			case VertexElement::eSHORT2N:			return D3DDECLTYPE_SHORT2N;
			case VertexElement::eSHORT4N:			return D3DDECLTYPE_SHORT4N;
			case VertexElement::eUSHORT2N:		return D3DDECLTYPE_USHORT2N;
			case VertexElement::eUSHORT4N:		return D3DDECLTYPE_USHORT4N;
			case VertexElement::eUDEC3N:			return D3DDECLTYPE_UDEC3;
			case VertexElement::eUDEC3:				return D3DDECLTYPE_UDEC3; 
			case VertexElement::eDEC3N:				return D3DDECLTYPE_DEC3N;
			case VertexElement::eHALFFLOAT2:	return D3DDECLTYPE_FLOAT16_2;
			case VertexElement::eHALFFLOAT4:	return D3DDECLTYPE_FLOAT16_4;
		}
		
		Verify(false);
		return 0;
	}
			
	void VertexDeclManager::clear(void)
	{
		D3D::safeReleaseVec(mFixedVertexDecls);
	}

	VertexDeclManager::VertexDeclManager() : 
		mFixedVertexDecls(NumVertexDeclarations)
	{
		clear();
		
		mVertexDeclCache.reserve(128);
	}

	VertexDeclManager::~VertexDeclManager()
	{
	}
	
	void VertexDeclManager::initVertexDeclarations(void)
	{
		Assert(NumVertexDeclarations == eNumVertexDeclarations);

		for (int i = 0; i < NumVertexDeclarations; i++)
		{
			VertexDeclaration& decl = gVertexDeclarations[i];

      Assert(i == decl.declID);
			
			if (decl.autoOffsets)
				decl.size = D3D::setVertexDeclarationOffsets(decl.pElements);
			else
				DebugNull(decl.size);
			
			mFixedVertexDecls[i] = D3D::createVertexDeclaration(decl.pElements);
		}
	}

	void VertexDeclManager::initDeviceObjects(void)
	{
		clear();
		
		initVertexDeclarations();
		
		for (int i = 0; i < mVertexDeclCache.size(); i++)
			mVertexDeclCache[i].initDeviceObjects();
	}

	void VertexDeclManager::deleteDeviceObjects(void)
	{
		clear();
		
		for (int i = 0; i < mVertexDeclCache.size(); i++)
			mVertexDeclCache[i].deleteDeviceObjects();
	}

	void VertexDeclManager::setToDevice(EVertexDeclaration vertexDeclarationIndex) const
	{
		if (eInvalidDeclaration == vertexDeclarationIndex)
		{
			D3D::setVertexDeclaration(NULL);
			return;
		}

		D3D::setVertexDeclaration(mFixedVertexDecls[DebugRange<int>(vertexDeclarationIndex, eNumVertexDeclarations)]);
	}
	
	int VertexDeclManager::getSize(EVertexDeclaration vertexDeclarationIndex) const
	{
		return gVertexDeclarations[DebugRange<int>(vertexDeclarationIndex, eNumVertexDeclarations)].size;
	}
	
	const VertexDeclManager::Handle VertexDeclManager::create(const std::vector<UnivertPacker>& packers)
	{
		for (int i = 0; i < mVertexDeclCache.size(); i++)
			if (mVertexDeclCache[i] == packers)
				return i;
				
		Status("VertexDeclManager::create: Cache miss, %i entries\n", mVertexDeclCache.size());
		
		mVertexDeclCache.push_back(VertexDecl(packers));
						
		return mVertexDeclCache.size() - 1;
	}
	
	void VertexDeclManager::setToDevice(VertexDeclManager::Handle handle) const
	{
		mVertexDeclCache[DebugRange(handle, mVertexDeclCache.size())].setToDevice();
	}
  
} // namespace gr

