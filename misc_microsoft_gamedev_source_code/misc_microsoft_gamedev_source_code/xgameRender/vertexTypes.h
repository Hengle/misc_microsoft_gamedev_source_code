//============================================================================
//
//  vertextypes.h
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================
#pragma once

#include "math\generalVector.h"

//============================================================================
// BTLVertex
//
// Pre-transformed/lit vertex with one texture coordinate
//============================================================================
struct BTLVertex
{
   float x;
   float y;
   float z;
   float rhw;
   DWORD diffuse;
   DWORD specular;
   float tu;
   float tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BTLT3DVertex
//
// Pre-transformed/lit vertex with one 3D texture coordinate
// No specular color to get it to 32 bytes
//============================================================================
struct BTLT3DVertex
{
   float x;
   float y;
   float z;
   float rhw;
   DWORD diffuse;
   float tu;
   float tv;
   float ts;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BTLT2Vertex
//
// Pre-transformed/lit vertex with two texture coordinates.
//============================================================================
struct BTLT2Vertex
{
   float x;
   float y;
   float z;
   float rhw;
   DWORD diffuse;
   DWORD specular;
   float tu;
   float tv;
   float tu2;
   float tv2;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BPosDiffuseTLVertex
//
// Pre-transformed/lit vertex with only position and diffuse
//============================================================================
struct BPosDiffuseTLVertex
{
   float x;
   float y;
   float z;
   float rhw;
   DWORD diffuse;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};



//============================================================================
// BPNTVertex
//
// Vertex with a position, normal, and single texture coordinate.
//============================================================================
struct BPNTVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   float tu;
   float tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
   static IDirect3DVertexDeclaration9 *msInstanceVertexDecl;
};


//============================================================================
// BPTVertex
//
// Vertex with a position, and single texture coordinate.
//============================================================================
struct BPTVertex
{
   D3DXFLOAT16 x;
   D3DXFLOAT16 y;
   D3DXFLOAT16 z;
   ushort padd;
   D3DXFLOAT16 tu;
   D3DXFLOAT16 tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNTIVertex
//
// Vertex with a position, normal, single texture coordinate, and intensity.
//============================================================================
struct BPNTIVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   float tu;
   float tv;
   float intensity;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNTTanVertex
//
// Vertex with a position, normal, single texture coordinate, tangent vector
//============================================================================
struct BPNTTanVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   D3DVECTOR tangent;
   float tu;
   float tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
   static IDirect3DVertexDeclaration9 *msInstanceVertexDecl;
};


//============================================================================
// BPNTDVertex
//
// Vertex with a position, normal, diffuse color, and single texture coordinate.
//============================================================================
struct BPNTDVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   DWORD diffuse;
   float tu;
   float tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;

   bool operator == (const BPNTDVertex& v) const
   {
      return (memcmp(this, &v, sizeof(*this)) == 0);
   }
};

//============================================================================
// BPNDTTIVertex
//
// Vertex with a position, normal, diffuse color, and single texture coordinate
// and hdr intensity
//============================================================================
struct BPNDTTIVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   DWORD     diffuse;
   float     tu;
   float     tv;
   float     tu2;
   float     tv2;
   float     intensity;

   static IDirect3DVertexDeclaration9 *msVertexDecl;

   bool operator == (const BPNDTTIVertex& v) const
   {
      return (memcmp(this, &v, sizeof(*this)) == 0);
   }
};

//============================================================================
// BPNT2DSVertex
//
// Vertex with a position, normal, diffuse color, specular color, and two texture coordinates.
//============================================================================
struct BPNT2DSVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   DWORD diffuse;
   DWORD specular;
   float tu;
   float tv;
   float tu2;
   float tv2;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BPNT2Vertex
//
// Vertex with a position, normal, and two texture coordinates.
//============================================================================
struct BPNT2Vertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   float tu;
   float tv;
   float tu2;
   float tv2;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNT4Vertex
//
// Vertex with a position, normal, and four texture coordinates.
//============================================================================
struct BPNT4Vertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   float tu;
   float tv;
   float tu2;
   float tv2;
   float tu3;
   float tv3;
   float tu4;
   float tv4;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPDVertex
//
// Vertex with a position, normal, diffuse color, and single texture coordinate.
//============================================================================
struct BPDVertex
{
   D3DVECTOR pos;
   DWORD diffuse;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BPDTVertex
//
// Vertex with a position, diffuse color, and single texture coordinate.
//============================================================================
struct BPDTVertex
{
   D3DVECTOR pos;
   DWORD diffuse;
   float tu;
   float tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BPDT2Vertex
//
// Vertex with a position, diffuse color, and two sets of texture coordinates.
//============================================================================
struct BPDT2Vertex
{
   D3DVECTOR pos;
   DWORD diffuse;
   float tu;
   float tv;
   float tu2;
   float tv2;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BPDT4Vertex
//
// Vertex with a position, diffuse color, and three sets of texture coordinates.
//============================================================================
struct BPDT4Vertex
{
   D3DVECTOR pos;
   DWORD diffuse;
   float tu;
   float tv;
   float tu2;
   float tv2;
   float tu3;
   float tv3;
   float tu4;
   float tv4;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};



//============================================================================
// BPW4NTVertex
//
// Vertex with a position, four weights with indices, normal, and single texture coordinate.
//============================================================================
struct BPW4NTVertex
{
   D3DXVECTOR3 p;
   float weights[3];
   unsigned char indices[4];

   D3DXVECTOR3 n;
   FLOAT       tu, tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};


//============================================================================
// BPNTDTanVertex
//
// Vertex with a position, normal, single texture coordinate, diffuse color,
// and tangent vector
//============================================================================
struct BPNTDTanVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   float tu;
   float tv;
   DWORD diffuse;
   D3DVECTOR tangent;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNTanT2Vertex
//
// Vertex with a position, normal, tangent and two texture coordinates.
//============================================================================
struct BPNTanT2Vertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   D3DVECTOR tangent;
   float tu;
   float tv;
   float tu2;
   float tv2;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNTanT4Vertex
//
// Vertex with a position, normal, tangent and four texture coordinates.
//============================================================================
struct BPNTanT4Vertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   D3DVECTOR tangent;
   float tu;
   float tv;
   float tu2;
   float tv2;
   float tu3;
   float tv3;
   float tu4;
   float tv4;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNCTVertex
//
// Vertex with a position, normal, diffuse color 3 floats, and single texture coordinate.
//============================================================================
struct BPNCTVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   float color[4];
   float tu;
   float tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;

   bool operator == (const BPNCTVertex& v) const
   {
      return (memcmp(this, &v, sizeof(*this)) == 0);
   }
};

//============================================================================
// BPT4Vertex
//
// Vertex with 4D transformed position (i.e. D3DDECLUSAGE_POSITIONT) and 
// four 4D texture coordinates.
//============================================================================
struct BPT4Vertex
{
   BVecN<4> pos;
   BVecN<4> uv[4];
   
   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNTanDTVertex
//
// Vertex with a position, normal, tangent,  diffuse and texture coordinate
//============================================================================
struct BPNTanDTVertex
{
   D3DVECTOR pos;
   D3DVECTOR normal;
   D3DVECTOR tangent;
   DWORD diffuse;
   float tu;
   float tv;

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// BPNTTanPackedVertex
//============================================================================
struct BPNTTanPackedVertex
{
   WORD pos[4];
   DWORD normal;
   DWORD tangent;
   WORD tu;
   WORD tv;
};

//============================================================================
// BPNTPackedVertex
//============================================================================
struct BPNTPackedVertex
{
   WORD pos[4];
   DWORD normal;
   WORD tu;
   WORD tv;
};

//============================================================================
// struct BPVertex
//============================================================================
struct BPVertex
{
   D3DVECTOR   pos;   
   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// struct BInstancedPrimitiveVertex
//============================================================================
struct BInstancedPrimitiveVertex
{
   XMVECTOR    matrixRow0;
   XMVECTOR    matrixRow1;
   XMVECTOR    matrixRow2;
   XMVECTOR    matrixRow3;
   DWORD       color;
   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//==============================================================================
// BPU2Vertex
//==============================================================================
struct BPU2Vertex
{
   XMFLOAT2 pos;
   XMFLOAT2 uv0;  // UV's controlled by uLo, uHi, vLo, vHi parameters
   XMFLOAT2 uv1;  // normalized UV's [0,1]

   static IDirect3DVertexDeclaration9 *msVertexDecl;
};

//============================================================================
// Functions to create/release the static declarations for the various vertex
// types.  These must be updated when you add/change/remove a vertex type.
//============================================================================
class BVertexTypes
{
public:
   static void createVertexDeclarations(void);
   static void releaseVertexDeclarations(void);

   //============================================================================
   // class BVertexDeclHelper
   // 
   // Class that makes create declarations much less error prone, but slightly
   // less efficient.  Since this is just done a few times at startup the 
   // efficiency should be irrelevant.  The assumption this makes is that each
   // stream is packed individually starting at offset 0.  If that is a bad 
   // assumption for your vertex type, make an array of D3DVERTEXELEMENT9's yourself.
   //============================================================================
   class BVertexDeclHelper
   {
      public:
                                 BVertexDeclHelper() { clear(); }
                                 
         void                    clear(void);

         // Add that let's you specify all parameters.
         void                    add(BYTE stream, DWORD type, DWORD method, DWORD usage, DWORD usageIndex);
      
         // This assumes method is D3DDECLMETHOD_DEFAULT.
         void                    addDefault(BYTE stream, DWORD type, DWORD usage, DWORD usageIndex);

         // This assumes method is D3DDECLMETHOD_DEFAULT and stream is 0.
         void                    addSimple(DWORD type, DWORD usage, DWORD usageIndex);

         // Add's a declaration from a set of FVF flags.
         void                    addFVF(DWORD fvf);

         // Create.
         void                    createVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl);

      protected:
         enum {cNumStreams=16};

         BYTE                    mOffset[cNumStreams];
         BDynamicArray<D3DVERTEXELEMENT9> mElements;
   };

};