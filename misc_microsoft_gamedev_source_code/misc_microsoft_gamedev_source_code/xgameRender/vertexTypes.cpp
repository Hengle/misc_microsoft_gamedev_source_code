//============================================================================
//
//  vertextypes.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================

#include "xgameRender.h"
#include "vertexTypes.h"
#include "renderDraw.h"

IDirect3DVertexDeclaration9 *BPU2Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BTLVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BTLT3DVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BTLT2Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPosDiffuseTLVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTVertex::msInstanceVertexDecl;
IDirect3DVertexDeclaration9 *BPTVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTIVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTTanVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTTanVertex::msInstanceVertexDecl;
IDirect3DVertexDeclaration9 *BPNTDVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNT2DSVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNT2Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNT4Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPDVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPDTVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPDT2Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPDT4Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPW4NTVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTDTanVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTanT2Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTanT4Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNCTVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPT4Vertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNTanDTVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPNDTTIVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BPVertex::msVertexDecl;
IDirect3DVertexDeclaration9 *BInstancedPrimitiveVertex::msVertexDecl;

//============================================================================
// createDeclarations
//============================================================================
#define createSimpleFVF(name, fvf) helper.clear(); helper.addFVF(fvf); helper.createVertexDeclaration(&name::msVertexDecl);
void BVertexTypes::createVertexDeclarations(void)
{
   // Create a decl helper.
   BVertexDeclHelper helper;

   // Do all the simple, old-sk00l FVF style ones the quick and easy way.
   // rg [6/14/05] - XYZRHW not available!
   createSimpleFVF(BTLVertex, D3DFVF_XYZW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1);
   createSimpleFVF(BTLT3DVertex, D3DFVF_XYZW | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE3(0));
   createSimpleFVF(BTLT2Vertex, D3DFVF_XYZW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2);
   createSimpleFVF(BPosDiffuseTLVertex, D3DFVF_XYZW | D3DFVF_DIFFUSE)
      
   createSimpleFVF(BPNTVertex, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)
   createSimpleFVF(BPTVertex, D3DFVF_XYZ  | D3DFVF_TEX1)
   
   helper.clear();
   helper.addDefault(0, D3DDECLTYPE_FLOAT16_4, D3DDECLUSAGE_POSITION, 0);
   helper.addDefault(0, D3DDECLTYPE_UBYTE4N, D3DDECLUSAGE_NORMAL, 0);
   helper.addDefault(0, D3DDECLTYPE_FLOAT16_2, D3DDECLUSAGE_TEXCOORD, 0);

   helper.createVertexDeclaration(&BPNTVertex::msInstanceVertexDecl);


   helper.clear();
   helper.addDefault(0, D3DDECLTYPE_FLOAT16_4, D3DDECLUSAGE_POSITION, 0);
   helper.addDefault(0, D3DDECLTYPE_FLOAT16_2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.createVertexDeclaration(&BPTVertex::msVertexDecl);

   
   createSimpleFVF(BPNTDVertex, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)
         
   createSimpleFVF(BPNT2DSVertex, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX2)
   createSimpleFVF(BPNT2Vertex, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX2)
   createSimpleFVF(BPNT4Vertex, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX4)
   createSimpleFVF(BPDVertex, D3DFVF_XYZ | D3DFVF_DIFFUSE)
   createSimpleFVF(BPDTVertex, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
   createSimpleFVF(BPDT2Vertex, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2)
   createSimpleFVF(BPDT4Vertex, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX4)
   createSimpleFVF(BPW4NTVertex, D3DFVF_XYZB4 | D3DFVF_LASTBETA_D3DCOLOR | D3DFVF_NORMAL | D3DFVF_TEX1)
      
   // BPNTTanVertex -- can't be expressed as an fvf.
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.createVertexDeclaration(&BPNTTanVertex::msVertexDecl);
   
   helper.clear();
   helper.addDefault(0, D3DDECLTYPE_FLOAT16_4, D3DDECLUSAGE_POSITION, 0);
   helper.addDefault(0, D3DDECLTYPE_UBYTE4N, D3DDECLUSAGE_NORMAL, 0);
   helper.addDefault(0, D3DDECLTYPE_UBYTE4N, D3DDECLUSAGE_TANGENT, 0);
   helper.addDefault(0, D3DDECLTYPE_FLOAT16_2, D3DDECLUSAGE_TEXCOORD, 0);

   helper.createVertexDeclaration(&BPNTTanVertex::msInstanceVertexDecl);

   // BPNTDTanVertex -- can't be expressed as an fvf.
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 0);
   helper.createVertexDeclaration(&BPNTDTanVertex::msVertexDecl);

   // BPNTanT2Vertex -- can't be expressed as an fvf.
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 1);
   helper.createVertexDeclaration(&BPNTanT2Vertex::msVertexDecl);

   // BPNTanT4Vertex -- can't be expressed as an fvf.
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 1);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 2);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 3);
   helper.createVertexDeclaration(&BPNTanT4Vertex::msVertexDecl);

   // BPNCTVertex -- can't be expressed as an fvf.
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_COLOR, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.createVertexDeclaration(&BPNCTVertex::msVertexDecl);
   
   // BPT4Vertex -- (RG TODO: Change to FVF?)
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, 1);
   helper.addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, 2);
   helper.addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, 3);
   helper.createVertexDeclaration(&BPT4Vertex::msVertexDecl);

   // BPNTanDTVertex -- can't be expressed as an fvf.
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 0);
   helper.addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.createVertexDeclaration(&BPNTanDTVertex::msVertexDecl);

   //-- BPNDTTIVertex
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3,   D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3,   D3DDECLUSAGE_NORMAL,   0);
   helper.addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR,    0);
   helper.addSimple(D3DDECLTYPE_FLOAT2,   D3DDECLUSAGE_TEXCOORD, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT2,   D3DDECLUSAGE_TEXCOORD, 1);
   helper.addSimple(D3DDECLTYPE_FLOAT1,   D3DDECLUSAGE_TEXCOORD, 2);
   helper.createVertexDeclaration(&BPNDTTIVertex::msVertexDecl);

   //-- BPNTIVertex
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3,   D3DDECLUSAGE_POSITION, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT3,   D3DDECLUSAGE_NORMAL,   0);
   helper.addSimple(D3DDECLTYPE_FLOAT2,   D3DDECLUSAGE_TEXCOORD, 0);
   helper.addSimple(D3DDECLTYPE_FLOAT1,   D3DDECLUSAGE_TEXCOORD, 1);
   helper.createVertexDeclaration(&BPNTIVertex::msVertexDecl);

   //-- BPVertex
   helper.clear();
   helper.addSimple(D3DDECLTYPE_FLOAT3,   D3DDECLUSAGE_POSITION, 0);
   helper.createVertexDeclaration(&BPVertex::msVertexDecl);

   //-- BInstancedPrimitiveVertex
   helper.clear();
   helper.addDefault(1, D3DDECLTYPE_FLOAT4,   D3DDECLUSAGE_TEXCOORD, 0);
   helper.addDefault(1, D3DDECLTYPE_FLOAT4,   D3DDECLUSAGE_TEXCOORD, 1);
   helper.addDefault(1, D3DDECLTYPE_FLOAT4,   D3DDECLUSAGE_TEXCOORD, 2);
   helper.addDefault(1, D3DDECLTYPE_FLOAT4,   D3DDECLUSAGE_TEXCOORD, 3);
   helper.addDefault(1, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR,    0);
   helper.createVertexDeclaration(&BInstancedPrimitiveVertex::msVertexDecl);   
   
   //-- BPU2Vertex
   helper.clear();
   helper.addDefault(0, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_POSITION, 0);
   helper.addDefault(0, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0);
   helper.addDefault(0, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 1); 
   helper.createVertexDeclaration(&BPU2Vertex::msVertexDecl);   
}

//============================================================================
// releaseDeclarations
//============================================================================
#define RELEASE(x) if(x) {gRenderDraw.releaseD3DResource(x); x=NULL;}
void BVertexTypes::releaseVertexDeclarations(void)
{
   RELEASE(BPT4Vertex::msVertexDecl);
   RELEASE(BTLVertex::msVertexDecl)
   RELEASE(BTLT3DVertex::msVertexDecl)
   RELEASE(BTLT2Vertex::msVertexDecl)
   RELEASE(BPosDiffuseTLVertex::msVertexDecl)
   RELEASE(BPNTVertex::msVertexDecl)
   RELEASE(BPNTVertex::msInstanceVertexDecl)
   RELEASE(BPTVertex::msVertexDecl)
   RELEASE(BPNTIVertex::msVertexDecl)
   RELEASE(BPNTTanVertex::msVertexDecl)
   RELEASE(BPNTTanVertex::msInstanceVertexDecl)
   RELEASE(BPNTDVertex::msVertexDecl)
   RELEASE(BPNT2DSVertex::msVertexDecl)
   RELEASE(BPNT2Vertex::msVertexDecl)
   RELEASE(BPNT4Vertex::msVertexDecl)
   RELEASE(BPDVertex::msVertexDecl)
   RELEASE(BPDTVertex::msVertexDecl)
   RELEASE(BPDT2Vertex::msVertexDecl)
   RELEASE(BPDT4Vertex::msVertexDecl)
   RELEASE(BPW4NTVertex::msVertexDecl)
   RELEASE(BPNTDTanVertex::msVertexDecl)
   RELEASE(BPNTanT2Vertex::msVertexDecl)
   RELEASE(BPNTanT4Vertex::msVertexDecl)
   RELEASE(BPNCTVertex::msVertexDecl)
   RELEASE(BPNTanDTVertex::msVertexDecl)
   RELEASE(BPNDTTIVertex::msVertexDecl)
   RELEASE(BPVertex::msVertexDecl);
   RELEASE(BInstancedPrimitiveVertex::msVertexDecl);
   RELEASE(BPU2Vertex::msVertexDecl);
}


//============================================================================
// getDeclTypeSize
//============================================================================
BYTE getDeclTypeSize(DWORD type)
{
   switch(type)
   {
      case D3DDECLTYPE_FLOAT1:
         return(4);

      case D3DDECLTYPE_FLOAT2:
         return(8);

      case D3DDECLTYPE_FLOAT3:
         return(12);

      case D3DDECLTYPE_FLOAT4:
         return(16);

      case D3DDECLTYPE_D3DCOLOR:
         return(4);

      case D3DDECLTYPE_UBYTE4:
      case D3DDECLTYPE_UBYTE4N:
         return(4);

      case D3DDECLTYPE_SHORT2:
      case D3DDECLTYPE_SHORT2N:
      case D3DDECLTYPE_USHORT2N:
         return(4);

      case D3DDECLTYPE_SHORT4:
      case D3DDECLTYPE_USHORT4N:
         return(8);

      case D3DDECLTYPE_UDEC3:
      case D3DDECLTYPE_DEC3N:
         //BFAIL("If you know what the hell this is and how big it is, fix this.  I think it might be 4 bytes but the DX9 docs suck.");
         return(4);

      case D3DDECLTYPE_FLOAT16_2:
         return(4);

      case D3DDECLTYPE_FLOAT16_4:
         return(8);
   }

   BFAIL("Unknown vertex decl type.");
   return(0);
}


//============================================================================
// BVertexDeclHelper::clear
//============================================================================
void BVertexTypes::BVertexDeclHelper::clear(void)
{
   // Reset offset.
   memset(mOffset, 0, cNumStreams*sizeof(mOffset[0]));

   // Clear out elements.
   mElements.setNumber(0);
};


//============================================================================
// BVertexDeclHelper::add
//============================================================================
void BVertexTypes::BVertexDeclHelper::add(BYTE stream, DWORD type, DWORD method, DWORD usage, DWORD usageIndex)
{
   // Check stream range.
   if(stream>=cNumStreams)
   {
      BFAIL("Too many streams for BVertexDeclHelper.  Increase cNumStreams or use fewer streams.");
      return;
   }

   // Make space.
   D3DVERTEXELEMENT9 &element = mElements.grow();
   
   // Fill in data.
   element.Stream = stream;
   element.Offset = mOffset[stream];
   element.Type = type;
   element.Method = static_cast<BYTE>(method);
   element.Usage = static_cast<BYTE>(usage);
   element.UsageIndex = static_cast<BYTE>(usageIndex);

   // Update offset in this stream.
   DWORD newOffset = mOffset[stream] + getDeclTypeSize(type);
   if(newOffset >= 255)
   {
      newOffset=255;
      BFAIL("Vertex you're declaring is too big to fit in the BYTE offsets provided by D3D.");
   }
   mOffset[stream] = BYTE(newOffset);
}


//============================================================================
// BVertexDeclHelper::addDefault
//============================================================================
void BVertexTypes::BVertexDeclHelper::addDefault(BYTE stream, DWORD type, DWORD usage, DWORD usageIndex)
{
   add(stream, type, D3DDECLMETHOD_DEFAULT, usage, usageIndex);
}


//============================================================================
// BVertexDeclHelper::addSimple
//============================================================================
void BVertexTypes::BVertexDeclHelper::addSimple(DWORD type, DWORD usage, DWORD usageIndex)
{
   add(0, type, D3DDECLMETHOD_DEFAULT, usage, usageIndex);
}


//============================================================================
// BVertexDeclHelper::addFVF
//============================================================================
void BVertexTypes::BVertexDeclHelper::addFVF(DWORD fvf)
{
   // Check all the FVF flags in the proper order and add them in.

   // First figure out which type of position data we have.
   DWORD positionType = fvf & D3DFVF_POSITION_MASK;

   // Based on that, add the decl data a specified in the D3D docs.
   switch(positionType)
   {
      case D3DFVF_XYZ:
         addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
         break;

      case D3DFVF_XYZB1:
         addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);

         if(fvf & D3DFVF_LASTBETA_UBYTE4)
            addSimple(D3DDECLTYPE_UBYTE4, D3DDECLUSAGE_BLENDINDICES, 0);
         else if(fvf & D3DFVF_LASTBETA_D3DCOLOR)
            addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
         else
            addSimple(D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDWEIGHT, 0);

         break;

      case D3DFVF_XYZB2:
         addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);

         if(fvf & D3DFVF_LASTBETA_UBYTE4)
         {
            addSimple(D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDWEIGHT, 0);
            addSimple(D3DDECLTYPE_UBYTE4, D3DDECLUSAGE_BLENDINDICES, 0);
         }
         else if(fvf & D3DFVF_LASTBETA_D3DCOLOR)
         {
            addSimple(D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDWEIGHT, 0);
            addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
         }
         else
            addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_BLENDWEIGHT, 0);

         break;

      case D3DFVF_XYZB3:
         addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);

         if(fvf & D3DFVF_LASTBETA_UBYTE4)
         {
            addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_BLENDWEIGHT, 0);
            addSimple(D3DDECLTYPE_UBYTE4, D3DDECLUSAGE_BLENDINDICES, 0);
         }
         else if(fvf & D3DFVF_LASTBETA_D3DCOLOR)
         {
            addSimple(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_BLENDWEIGHT, 0);
            addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
         }
         else
            addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_BLENDWEIGHT, 0);
         break;

      case D3DFVF_XYZB4:
         addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);

         if(fvf & D3DFVF_LASTBETA_UBYTE4)
         {
            addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_BLENDWEIGHT, 0);
            addSimple(D3DDECLTYPE_UBYTE4, D3DDECLUSAGE_BLENDINDICES, 0);
         }
         else if(fvf & D3DFVF_LASTBETA_D3DCOLOR)
         {
            addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_BLENDWEIGHT, 0);
            addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
         }
         else
            addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_BLENDWEIGHT, 0);
         break;

      case D3DFVF_XYZB5:
         addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0);
         addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_BLENDWEIGHT, 0);

         if(fvf & D3DFVF_LASTBETA_UBYTE4)
            addSimple(D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDINDICES, 0);
         else if(fvf & D3DFVF_LASTBETA_D3DCOLOR)
            addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0);
         else
            addSimple(D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDINDICES, 0);

         break;

      case D3DFVF_XYZW:
         addSimple(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITION, 0);
         break;
   }

   // Normal.
   if(fvf & D3DFVF_NORMAL)
      addSimple(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0);

   // Point size.
   if(fvf & D3DFVF_PSIZE)
      addSimple(D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_PSIZE, 0);

   // Diffuse.
   if(fvf & D3DFVF_DIFFUSE)
      addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0);

   // Specular.
   if(fvf & D3DFVF_SPECULAR)
      addSimple(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 1);

   // Figure out how many texture coordinates we have.
   DWORD texCoord = fvf & D3DFVF_TEXCOUNT_MASK;
   long numTxCoords = 0;
   switch(texCoord)
   {
      case D3DFVF_TEX1:
         numTxCoords = 1;
         break;
      case D3DFVF_TEX2:
         numTxCoords = 2;
         break;
      case D3DFVF_TEX3:
         numTxCoords = 3;
         break;
      case D3DFVF_TEX4:
         numTxCoords = 4;
         break;
      case D3DFVF_TEX5:
         numTxCoords = 5;
         break;
      case D3DFVF_TEX6:
         numTxCoords = 6;
         break;
      case D3DFVF_TEX7:
         numTxCoords = 7;
         break;
      case D3DFVF_TEX8:
         numTxCoords = 8;
         break;
   }

   // Add each texture coordinate.
   for(BYTE i=0; i<numTxCoords; i++)
   {
      // Get size of texture coordinate (1 through 4 floats).
      // Reverse engineered from D3DFVF_TEXCOORDSIZEn macros.
      DWORD shift = i*2 + 16;
      DWORD mask = 0x3 << shift;
      DWORD flags = (fvf&mask)>>shift;

      // No flags == D3DDECLTYPE_FLOAT2 
      DWORD type = D3DDECLTYPE_FLOAT2;
      if(flags == D3DFVF_TEXTUREFORMAT1)
         type = D3DDECLTYPE_FLOAT1;
      else if(flags == D3DFVF_TEXTUREFORMAT3)
         type = D3DDECLTYPE_FLOAT3;
      else if(flags == D3DFVF_TEXTUREFORMAT4)
         type = D3DDECLTYPE_FLOAT4;

      // Add.
      addSimple(type, D3DDECLUSAGE_TEXCOORD, i);
   }
}


//============================================================================
// BVertexDeclHelper::createVertexDeclaration
//============================================================================
void BVertexTypes::BVertexDeclHelper::createVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
{
   // Check params
   BDEBUG_ASSERT(ppDecl);
   
   // Shove in the end sentinel.  Couldn't the function just take a count?
   const D3DVERTEXELEMENT9 endSentinel = D3DDECL_END();
   mElements.add(endSentinel);

   // Create.
   HRESULT hr = gRenderDraw.createVertexDeclaration(mElements.getData(), ppDecl);
   if (FAILED(hr))
   {
      BFAIL("Vertex decl create failed");
   }

   // Remove the crappy end sentinel in case the user is doing something crazy with
   // adding stuff/creating/adding stuff/creating (without a clear).
   mElements.setNumber(mElements.getNumber()-1);
}


//============================================================================
// eof: vertextypes.cpp
//============================================================================
