//============================================================================
// particlevertextypes.cpp
// Ensemble Studios (C) 2006
//============================================================================
#include "xparticlescommon.h"
#include "particlevertextypes.h"

IDirect3DVertexDeclaration9* BPVertexPT::msVertexDecl = NULL;
IDirect3DVertexDeclaration9* BPTrailVertexPT::msVertexDecl = NULL;
IDirect3DVertexDeclaration9* BPBeamVertexPT::msVertexDecl = NULL;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void initParticleVertexDeclarations()
{
   ASSERT_RENDER_THREAD

   const D3DVERTEXELEMENT9 BPVertexPT_Elements[] =
   {    
      { 0, 0,   D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0 },
      { 0, 16,  D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  1 },
      { 0, 24,  D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  2 },
      { 0, 32,  D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  3 },
      { 0, 36,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
      { 0, 40,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     1 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BPVertexPT_Elements, &BPVertexPT::msVertexDecl);

   const D3DVERTEXELEMENT9 BPTrailVertexPT_Elements[] =
   {    
      { 0, 0,   D3DDECLTYPE_FLOAT4,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 16,  D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  1 },
      { 0, 24,  D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  2 },
      { 0, 32,  D3DDECLTYPE_FLOAT16_2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  3 },
      { 0, 36,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
      { 0, 40,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     1 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BPTrailVertexPT_Elements, &BPTrailVertexPT::msVertexDecl);

   const D3DVERTEXELEMENT9 BPBeamVertexPT_Elements[] =
   {    
      { 0, 0,   D3DDECLTYPE_FLOAT4,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 16,  D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  1 },
      { 0, 24,  D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  2 },
      { 0, 32,  D3DDECLTYPE_FLOAT16_4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  3 },
      { 0, 40,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },
      { 0, 44,  D3DDECLTYPE_D3DCOLOR,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     1 },
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BPBeamVertexPT_Elements, &BPBeamVertexPT::msVertexDecl);
}

void deInitParticleVertexDeclarations()
{
}
