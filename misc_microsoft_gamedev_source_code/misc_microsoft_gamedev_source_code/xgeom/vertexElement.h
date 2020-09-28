// File: vertexElement.h
// Copyright (c) Microsoft Corporation. All rights reserved.
#pragma once

#include "math\halfFloat.h"
#include "math\generalVector.h"

// Full vertex elements:
// P = position
// B# = basis vector
// A# = tangent vector
// N = normal vector
// T# = texcoords
// S = indices/weights
// D = diffuse
// I = index
// Partial elements:
// X# = basis vector scale

enum EVertElementSpec
{
   ePOS_SPEC            = 'P',
   eBASIS_SPEC          = 'B',
   eTANGENT_SPEC        = 'A',
   eNORM_SPEC           = 'N',
   eTEXCOORDS_SPEC      = 'T',
   eSKIN_SPEC           = 'S',
   eDIFFUSE_SPEC        = 'D',
   eINDEX_SPEC          = 'I',
   eBASIS_SCALE_SPEC    = 'X',
   
   eNumVertElementIDs,
   
   eForceDWORD          = 0xFFFFFFFF
};
#define DEFAULT_UNIVERT_ELEMENT_ORDER "PB0B1NT0T1T2T3SDI"

struct VertexElement
{
   enum EType
   {
      eIGNORE,    // 0, nothing written
      
      eFLOAT1,       // 4
      eFLOAT2,       // 8
      eFLOAT3,       // 12
      eFLOAT4,       // 16
      eD3DCOLOR,     // 4, 4D packed unsigned bytes mapped to 0. to 1. range Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
      eUBYTE4,       // 4, 4D unsigned byte
      eSHORT2,       // 4, 2D signed short expanded to (value, value, 0., 1.)
      eSHORT4,       // 8, 4D signed short
      eUBYTE4N,      // 4, Each of 4 bytes is normalized by dividing to 255.0
      eSHORT2N,      // 4, 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
      eSHORT4N,      // 8, 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
      eUSHORT2N,     // 4, 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
      eUSHORT4N,     // 8, 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
      eUDEC3,        // 4, 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
      eDEC3N,        // 4, 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
      eHALFFLOAT2,   // 4, Two 16-bit floating point values, expanded to (value, value, 0, 1)
      eHALFFLOAT4,   // 8, Four 16-bit floating point values
      
      // Non-standard/incomplete types:
      // NOTE: This is not actually used, just an idea in case I want to pack scalers into D3D vectors.
      eHALFFLOAT1,   // 2, One 16-bit floating point value
      eUDEC3N,       // 4, 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
      eDHEN3N,       // 4 3D signed 10 11 11 format normalized and expanded to (v[0]/511.0, v[1]/1023.0, v[2]/1023.0, 1)
      
      eNumDeclTypes,
      
      //eForceDWORD = 0xFFFFFFFF
   };
   
   static const char* name(EType type)
   {
      switch (type)
      {
         case eIGNORE:        return "IGNORE";
         case eFLOAT1:        return "FLOAT1";
         case eFLOAT2:        return "FLOAT2";
         case eFLOAT3:        return "FLOAT3";
         case eFLOAT4:        return "FLOAT4";
         case eD3DCOLOR:      return "D3DCOLOR";
         case eUBYTE4:        return "UBYTE4";
         case eSHORT2:        return "SHORT2";
         case eSHORT4:        return "SHORT4";
         case eUBYTE4N:       return "UBYTE4N";
         case eSHORT2N:       return "SHORT2N";
         case eSHORT4N:       return "SHORT4N";
         case eUSHORT2N:      return "USHORT2N";
         case eUSHORT4N:      return "USHORT4N";
         case eUDEC3:         return "UDEC3";
         case eDEC3N:         return "DEC3N";
         case eHALFFLOAT2:    return "HALFFLOAT2";
         case eHALFFLOAT4:    return "HALFFLOAT4";
         case eHALFFLOAT1:    return "HALFFLOAT1";
         case eUDEC3N:        return "UDEC3N";
         case eDHEN3N:        return "DHEN3N";
         default:
            BVERIFY(false); 
      }
      return "";
   }

   static int size(EType type)
   {
      switch (type)
      {
         case eIGNORE:        return 0;
         case eFLOAT1:        return sizeof(float)*1;
         case eFLOAT2:        return sizeof(float)*2;
         case eFLOAT3:        return sizeof(float)*3;
         case eFLOAT4:        return sizeof(float)*4;
         case eD3DCOLOR:      return sizeof(uint);
         case eUBYTE4:        return sizeof(uint);
         case eSHORT2:        return sizeof(short)*2;
         case eSHORT4:        return sizeof(short)*4;
         case eUBYTE4N:       return sizeof(uint);
         case eSHORT2N:       return sizeof(short)*2;
         case eSHORT4N:       return sizeof(short)*4;
         case eUSHORT2N:      return sizeof(ushort)*2;
         case eUSHORT4N:      return sizeof(ushort)*4;
         case eUDEC3:         return sizeof(uint);
         case eDEC3N:         return sizeof(uint);
         case eHALFFLOAT2:    return sizeof(ushort)*2;
         case eHALFFLOAT4:    return sizeof(ushort)*4;
         case eHALFFLOAT1:    return sizeof(ushort)*1;
         case eUDEC3N:        return sizeof(uint);
         case eDHEN3N:        return sizeof(uint);
         default:             
            BASSERT(false);
      }
      return 0;
   }
               
   // Input value ranges:
   // Normalized types (including D3DCOLOR) accept [0,1] or [-1,1]
   // all others accept raw values.
   static void* pack(void* p, EType type, const BVecN<4>& value, bool bigEndian = true)
   {
      switch (type)
      {
         case eIGNORE:
            break;
         case eFLOAT1:
            p = Utils::WriteValue(p, value[0], bigEndian);
            break;
         case eFLOAT2:
            p = Utils::WriteValue(p, value[0], bigEndian);
            p = Utils::WriteValue(p, value[1], bigEndian);
            break;
         case eFLOAT3:
            p = Utils::WriteValue(p, value[0], bigEndian);
            p = Utils::WriteValue(p, value[1], bigEndian);
            p = Utils::WriteValue(p, value[2], bigEndian);
            break;
         case eFLOAT4:
            p = Utils::WriteValue(p, value[0], bigEndian);
            p = Utils::WriteValue(p, value[1], bigEndian);
            p = Utils::WriteValue(p, value[2], bigEndian);
            p = Utils::WriteValue(p, value[3], bigEndian);
            break;      
         case eD3DCOLOR:
         {
            const int a = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[3]));
            const int r = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[0]));
            const int g = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[1]));
            const int b = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[2]));
            const uint c = (a << 24) | (r << 16) | (g << 8) | b;
            p = Utils::WriteValue(p, c, bigEndian);
            break;
         }
         case eUBYTE4:
         {
            const int x = Math::iClampToByte(Math::FloatToIntRound(value[0]));
            const int y = Math::iClampToByte(Math::FloatToIntRound(value[1]));
            const int z = Math::iClampToByte(Math::FloatToIntRound(value[2]));
            const int w = Math::iClampToByte(Math::FloatToIntRound(value[3]));
            const uint c = (w << 24) | (z << 16) | (y << 8) | x;
            p = Utils::WriteValue(p, c, bigEndian);
            break;
         }
         case eSHORT2:
         {
            for (int i = 0; i < 2; i++)
            {
               const short v = static_cast<short>(Math::Clamp(Math::FloatToIntRound(value[i]), SHRT_MIN, SHRT_MAX));
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;
         }
         case eSHORT4:
         {
            for (int i = 0; i < 4; i++)
            {
               const short v = static_cast<short>(Math::Clamp(Math::FloatToIntRound(value[i]), SHRT_MIN, SHRT_MAX));
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;
         }
         case eUBYTE4N:
         {
            int v[4];
            v[0] = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[0]));
            v[1] = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[1]));
            v[2] = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[2]));
            v[3] = Math::iClampToByte(Math::FloatToIntRound(255.0f * value[3]));
            
            if (Math::EqualTol(value.horizontalAdd(), 1.0f))
            {
               int sum = v[0] + v[1] + v[2] + v[3];
               int toDistribute = 255 - sum;
               int newSum = 0;
               
               for (int k = 0; k < 4; k++)
               {
                  int oldWeight = v[k];
                  int newWeight = Math::iClampToByte(oldWeight + toDistribute);
                  
                  toDistribute -= (newWeight - oldWeight);
                  newSum += newWeight;                   

                  v[k] = newWeight;
               }
               
               BASSERT(255 == newSum);
               BASSERT(0 == toDistribute);
            }
                                    
            const uint c = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
            p = Utils::WriteValue(p, c, bigEndian);
            break;
         }
         case eSHORT2N:
         {
            for (int i = 0; i < 2; i++)
            {
               const short v = static_cast<short>(Math::Clamp(Math::FloatToIntRound(value[i] * 32767.0f), SHRT_MIN, SHRT_MAX));
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;
         }
         case eSHORT4N:
         {
            for (int i = 0; i < 4; i++)
            {
               const short v = static_cast<short>(Math::Clamp(Math::FloatToIntRound(value[i] * 32767.0f), SHRT_MIN, SHRT_MAX));
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;         
         }
         case eUSHORT2N:
         {
            for (int i = 0; i < 2; i++)
            {
               const short v = static_cast<short>(Math::Clamp(Math::FloatToIntRound(value[i] * 65535.0f), 0, USHRT_MAX));
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;
         }
         case eUSHORT4N:
         {
            for (int i = 0; i < 4; i++)
            {
               const short v = static_cast<short>(Math::Clamp(Math::FloatToIntRound(value[i] * 65535.0f), 0, USHRT_MAX));
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;
         }
         case eUDEC3:
         {
            const uint x = Math::Clamp(Math::FloatToIntRound(value[0]), 0, 1023);
            const uint y = Math::Clamp(Math::FloatToIntRound(value[1]), 0, 1023);
            const uint z = Math::Clamp(Math::FloatToIntRound(value[2]), 0, 1023);            
            const uint v = (z << 20) | (y << 10) | x;
            p = Utils::WriteValue(p, v, bigEndian);
            break;
         }
         case eDEC3N:
         {
            const uint x = 0x3ff & Math::Clamp(Math::FloatToIntRound(value[0] * 511.0f), -512, 511);
            const uint y = 0x3ff & Math::Clamp(Math::FloatToIntRound(value[1] * 511.0f), -512, 511);
            const uint z = 0x3ff & Math::Clamp(Math::FloatToIntRound(value[2] * 511.0f), -512, 511);
            const uint v = (z << 20) | (y << 10) | x;
            p = Utils::WriteValue(p, v, bigEndian);
            break;
         }
         case eHALFFLOAT2:
         {
            for (int i = 0; i < 2; i++)
            {
               const ushort v = HalfFloat::FloatToHalf(value[i]);
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;
         }
         case eHALFFLOAT4:
         {
            for (int i = 0; i < 4; i++)
            {
               const ushort v = HalfFloat::FloatToHalf(value[i]);
               p = Utils::WriteValue(p, v, bigEndian);
            }
            break;
         }
         case eHALFFLOAT1:
         {
            const ushort v = HalfFloat::FloatToHalf(value[0]);
            p = Utils::WriteValue(p, v, bigEndian);
            break;
         }
         case eUDEC3N:
         {
            const uint x = Math::Clamp(Math::FloatToIntRound(value[0] * 1023.0f), 0, 1023);
            const uint y = Math::Clamp(Math::FloatToIntRound(value[1] * 1023.0f), 0, 1023);
            const uint z = Math::Clamp(Math::FloatToIntRound(value[2] * 1023.0f), 0, 1023);           
            const uint v = (z << 20) | (y << 10) | x;
            p = Utils::WriteValue(p, v, bigEndian);
            break;
         }
         case eDHEN3N:
         {
            // 3D signed 10 11 11 format normalized and expanded to (v[0]/511.0, v[1]/1023.0, v[2]/1023.0, 1)
            const uint x = Math::Clamp(Math::FloatToIntRound(value[0] * 511.0f) + 512, 0, 1023);
            const uint y = Math::Clamp(Math::FloatToIntRound(value[1] * 1023.0f) + 1024, 0, 2047);
            const uint z = Math::Clamp(Math::FloatToIntRound(value[2] * 1023.0f) + 1024, 0, 2047);           
            const uint v = (z << 21) | (y << 10) | x;
            p = Utils::WriteValue(p, v, bigEndian);
            break;
         }
         default:
            BASSERT(false);
      }
      return p;
   }
}; // struct VertexElements

