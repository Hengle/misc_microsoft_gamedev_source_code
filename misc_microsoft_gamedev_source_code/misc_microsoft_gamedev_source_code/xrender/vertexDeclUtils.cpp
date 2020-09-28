//============================================================================
//
//  vertexDeclUtils.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "vertexDeclUtils.h"

// vertex declaration helper method
int BVertexDeclUtils::getVertexDeclarationTypeSize(DWORD type)
{
   int size = 0;
   switch (type)
   {
   case D3DDECLTYPE_FLOAT1    :  size = 4 * 1; break; // 1D float expanded to (value, 0., 0., 1.)
   case D3DDECLTYPE_FLOAT2    :  size = 4 * 2; break; // 2D float expanded to (value, value, 0., 1.)
   case D3DDECLTYPE_FLOAT3    :  size = 4 * 3; break; // 3D float expanded to (value, value, value, 1.)
   case D3DDECLTYPE_FLOAT4    :  size = 4 * 4; break; // 4D float
   case D3DDECLTYPE_D3DCOLOR  :  size = 4; break; // 4D packed unsigned bytes mapped to 0. to 1. range Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
   case D3DDECLTYPE_UBYTE4    :  size = 4; break; // 4D unsigned byte
   case D3DDECLTYPE_SHORT2    :  size = 4; break; // 2D signed short expanded to (value, value, 0., 1.)
   case D3DDECLTYPE_SHORT4    :  size = 8; break; // 4D signed short
   case D3DDECLTYPE_UBYTE4N   :  size = 4; break; // Each of 4 bytes is normalized by dividing to 255.0
   case D3DDECLTYPE_SHORT2N   :  size = 4; break; // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
   case D3DDECLTYPE_SHORT4N   :  size = 8; break; // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
   case D3DDECLTYPE_USHORT2N  :  size = 4; break; // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
   case D3DDECLTYPE_USHORT4N  :  size = 8; break; // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
   case D3DDECLTYPE_UDEC3     :  size = 4; break; // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
   case D3DDECLTYPE_DEC3N     :  size = 4; break; // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
   case D3DDECLTYPE_FLOAT16_2 :  size = 4; break; // Two 16-bit floating point values, expanded to (value, value, 0, 1)
   case D3DDECLTYPE_FLOAT16_4 :  size = 8; break; // Four 16-bit floating point values
   case D3DDECLTYPE_DHEN3N:      size = 4; break; // 3D signed 10 11 11 format normalized and expanded to (v[0]/511.0, v[1]/1023.0, v[2]/1023.0, 1)
   default:
      BVERIFY(false);
   }
   return size;
}

const char* BVertexDeclUtils::getVertexDeclarationTypeName(DWORD type)
{
   switch (type)
   {
   case D3DDECLTYPE_FLOAT1    :  return "FLOAT1"; break; // 1D float expanded to (value, 0., 0., 1.)
   case D3DDECLTYPE_FLOAT2    :  return "FLOAT2"; break; // 2D float expanded to (value, value, 0., 1.)
   case D3DDECLTYPE_FLOAT3    :  return "FLOAT3"; break; // 3D float expanded to (value, value, value, 1.)
   case D3DDECLTYPE_FLOAT4    :  return "FLOAT4"; break; // 4D float
   case D3DDECLTYPE_D3DCOLOR  :  return "D3DCOLOR"; break; // 4D packed unsigned bytes mapped to 0. to 1. range Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
   case D3DDECLTYPE_UBYTE4    :  return "UBYTE4"; break; // 4D unsigned byte
   case D3DDECLTYPE_SHORT2    :  return "SHORT2"; break; // 2D signed short expanded to (value, value, 0., 1.)
   case D3DDECLTYPE_SHORT4    :  return "SHORT4"; break; // 4D signed short
   case D3DDECLTYPE_UBYTE4N   :  return "UBYTE4N"; break; // Each of 4 bytes is normalized by dividing to 255.0
   case D3DDECLTYPE_SHORT2N   :  return "SHORT2N"; break; // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
   case D3DDECLTYPE_SHORT4N   :  return "SHORT4N"; break; // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
   case D3DDECLTYPE_USHORT2N  :  return "USHORT2N"; break; // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
   case D3DDECLTYPE_USHORT4N  :  return "USHORT4N"; break; // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
   case D3DDECLTYPE_UDEC3     :  return "UDEC3"; break; // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
   case D3DDECLTYPE_DEC3N     :  return "DEC3N"; break; // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
   case D3DDECLTYPE_FLOAT16_2 :  return "FLOAT16_2"; break; // Two 16-bit floating point values, expanded to (value, value, 0, 1)
   case D3DDECLTYPE_FLOAT16_4 :  return "FLOAT16_4"; break; // Four 16-bit floating point values
   case D3DDECLTYPE_DHEN3N:      return "DHEN3N"; break;    // 3D signed 10 11 11 format normalized and expanded to (v[0]/511.0, v[1]/1023.0, v[2]/1023.0, 1)
   default:
      BVERIFY(false);
   }
   return "";
}

const char* BVertexDeclUtils::getVertexDeclarationUsageName(DWORD usage)
{
   switch (usage)
   {
   case D3DDECLUSAGE_POSITION       : return "POSITION";
   case D3DDECLUSAGE_BLENDWEIGHT    : return "BLENDWEIGHT";
   case D3DDECLUSAGE_BLENDINDICES   : return "BLENDINDICES";
   case D3DDECLUSAGE_NORMAL         : return "NORMAL";
   case D3DDECLUSAGE_PSIZE          : return "PSIZE";
   case D3DDECLUSAGE_TEXCOORD       : return "TEXCOORD";
   case D3DDECLUSAGE_TANGENT        : return "TANGENT";
   case D3DDECLUSAGE_BINORMAL       : return "BINORMAL";
   case D3DDECLUSAGE_TESSFACTOR     : return "TESSFACTOR";
#ifndef XBOX   
   case D3DDECLUSAGE_POSITIONT      : return "POSITIONT";
#endif   
   case D3DDECLUSAGE_COLOR          : return "COLOR";
   case D3DDECLUSAGE_FOG            : return "FOG";
   case D3DDECLUSAGE_DEPTH          : return "DEPTH";
   case D3DDECLUSAGE_SAMPLE         : return "SAMPLE";
   default:
      BVERIFY(false);
   }
   return "";
}

// vertex declaration helper method
int BVertexDeclUtils::setVertexDeclarationOffsets(D3DVERTEXELEMENT9* pElements)
{
   int curOfs = 0;
   int prevStream = -1;

   while (0xFF != pElements->Stream)
   {
      if (pElements->Stream != prevStream)
      {
         prevStream = pElements->Stream;
         curOfs = 0;
      }

      const int size = getVertexDeclarationTypeSize(pElements->Type);

      pElements->Offset = (WORD)curOfs;

      curOfs += size;

      pElements++;
   }

   return curOfs;
}

uint BVertexDeclUtils::getVertexDeclarationStreamVertexSize(const D3DVERTEXELEMENT9* pElements, uint streamIndex)
{
   uint vertexSize = 0;
   
   while (0xFF != pElements->Stream)
   {
      const int size = getVertexDeclarationTypeSize(pElements->Type);
                  
      if (pElements->Stream == streamIndex)
         vertexSize = Math::Max<uint>(vertexSize, pElements->Offset + size);

      pElements++;
   }

   return vertexSize;
}

void BVertexDeclUtils::dumpVertexDeclaration(BTextDispatcher& dispatcher, D3DVERTEXELEMENT9* pElements)
{
   dispatcher.printf("D3D::dumpVertexDeclaration:\n");

   int totalSize = 0;
   while (0xFF != pElements->Stream)
   {
      const char* pTypeName = getVertexDeclarationTypeName(pElements->Type);
      const char* pUsageName = getVertexDeclarationUsageName(pElements->Usage);
      const int size = getVertexDeclarationTypeSize(pElements->Type);

      dispatcher.printf("  Stream: %i, Ofs: %02i, Typ: %s, Len: %i, Mthd: %i, Usage: %s, UsgIdx: %i\n",
         pElements->Stream,
         pElements->Offset,
         pTypeName,
         size,
         pElements->Method,
         pUsageName, 
         pElements->UsageIndex);

      totalSize += size;

      pElements++;
   }

   dispatcher.printf("  Total size: %i\n", totalSize);
}
