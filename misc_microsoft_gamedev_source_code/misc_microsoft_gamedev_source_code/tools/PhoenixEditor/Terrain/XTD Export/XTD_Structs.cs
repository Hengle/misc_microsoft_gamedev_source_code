using System;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.IO;

//-------------------------------
namespace Export360
{

//THESE ARE STRUCTS AND VALUES THAT ARE DUPLICATED IN CODE\TERRAIN\TERRAINIO.H
//THEY MUST REMAIN IDENTICIAL FOR XTD IO TO WORK PROPERLY
//--------------------------------------------

  

   //------------------------
   //------------------------
   //------------------------
   //------------------------
   //------------------------
   //------------------------

   //------------------------
   //------------------------
   //------------------------
   public enum BDXTFormat
   {
      cDXTInvalid = 0,

      cDXT1,
      cDXT1A,
      cDXT3,
      cDXT5,
      cDXT5A,
      cDXN,
   };

   public enum eDXTQuality
   {
      cDXTQualityLowest = -1,
      cDXTQualityNormal = 0,
      cDXTQualityBest = 1,

      cDXTTotalQuality
   };


   public enum eFileVersions
   {
      cXTDVersion = 0x000C,
      cXTTVersion = 0x0004,
      cXTHVersion = 0x0001,
      cXSDVersion = 0x0004,
   };

   enum eECFChunkType
   {
      cVBChunk =0,
      cHOChunk,
      c3DChunk,
   };
   public enum eXTD_ChunkID
   {
      cXTD_XTDHeader = 0x1111,
      cXTD_TerrainChunk = 0x2222,
      cXTD_TerrainAtlasLinkChunk = 0x4444,
      cXTD_AtlasChunk = 0x8888,
      cXTD_TessChunk = 0xAAAA,
      cXTD_LightingChunk = 0xBBBB,
      cXTD_AOChunk = 0xCCCC,
      cXTD_AlphaChunk = 0xDDDD,
   };

   public enum eXTT_ChunkID
   {
      cXTT_XTTHeader = 0x1111,
      cXTT_TerrainAtlasLinkChunk = 0x2222,
      
      cXTT_AtlasChunkAlbedo = 0x6666,
      cXTT_RoadsChunk = 0x8888,

      cXTT_FoliageHeaderChunk = 0xAAAA,
      cXTT_FoliageQNChunk = 0xBBBB,
   };

   public enum eXTH_ChunkID
   {
      cXTH_XTHHeader = 0x1111,
      cXTH_TerrainHeightfield = 0x2222,
      cXTH_TerrainHeightfieldAlpha = 0x3333,

   };

   public enum eXSD_ChunkID
   {
      cXSD_XSDHeader       = 0x1111,
      cXSD_SimHeights      = 0x2222,
      cXSD_Obstructions    = 0x4444,
      cXSD_TileTypes       = 0x8888,
      cXSD_CameraHeights   = 0xAAAA,
      cXSD_FlightHeights   = 0xABBB,
      cXSD_Buildable       = 0xCCCC,
      cXSD_FloodObstructions = 0xDDDD,
      cXSD_ScarabObstructions= 0xEEEE,
   };

   

   [StructLayout(LayoutKind.Sequential)]
   public unsafe struct XTDVisualHeader
   {
      public int version;

      public int numXVerts;
      public int numXChunks;
      public float tileScale;
      public Vector3 worldMin;
      public Vector3 worldMax;

      public void endianSwap()
      {
         version = Xbox_EndianSwap.endSwapI32(version);
         numXVerts = Xbox_EndianSwap.endSwapI32(numXVerts);
         numXChunks = Xbox_EndianSwap.endSwapI32(numXChunks);
         tileScale = Xbox_EndianSwap.endSwapF32(tileScale);
         worldMin.X = Xbox_EndianSwap.endSwapF32(worldMin.X);
         worldMin.Y = Xbox_EndianSwap.endSwapF32(worldMin.Y);
         worldMin.Z = Xbox_EndianSwap.endSwapF32(worldMin.Z);
         worldMax.X = Xbox_EndianSwap.endSwapF32(worldMax.X);
         worldMax.Y = Xbox_EndianSwap.endSwapF32(worldMax.Y);
         worldMax.Z = Xbox_EndianSwap.endSwapF32(worldMax.Z);
      }
      public static int giveSize()
      {
         return (sizeof(int)*3) + sizeof(float)*7;
      }
      
   }

   [StructLayout(LayoutKind.Sequential)]
   public unsafe struct XTDTerrainChunk
   {
      public int gridLocX, gridLocZ;   //my location in the grid
      public Vector3 mMin, mMax; //world volume

      public bool heightmapOnly;
      public int maxVertStride;
   }



   //------------------------
   [StructLayout(LayoutKind.Sequential)]
   public unsafe struct XTD3DVisual
   {
      [StructLayout(LayoutKind.Sequential)]
      unsafe public struct XTDVisualHeader
      {
         public int numXVerts;
         public float tileScale;     //VISUAL SCALING OF THE TILES FOR VISUAL

         public int numXBlocks;    //numXVerts>>1
         public int numZBlocks;    //numXVerts>>1

         //yAxis
         public int yAxis_hvMemSize;
         public int yAxis_dcMemSize;
         public Vector3 yAxis_ranges;//d3dxvector3

         //xzAxis
         public int xzAxis_dcMemSize;
         public int xAxis_hvMemSize;
         public int zAxis_hvMemSize;
         public Vector3 xAxis_ranges;//d3dxvector3
         public Vector3 zAxis_ranges;//d3dxvector3

         //basis
         public int xyBasis_hvMemSize;
         public int zwBasis_hvMemSize;

         public void endianSwap()
         {
            numZBlocks = Xbox_EndianSwap.endSwapI32(numZBlocks);
            numXBlocks = Xbox_EndianSwap.endSwapI32(numXBlocks);
            numXVerts = Xbox_EndianSwap.endSwapI32(numXVerts);
            tileScale = Xbox_EndianSwap.endSwapF32(tileScale);
            yAxis_hvMemSize = Xbox_EndianSwap.endSwapI32(yAxis_hvMemSize);
            yAxis_dcMemSize = Xbox_EndianSwap.endSwapI32(yAxis_dcMemSize);
            xAxis_hvMemSize = Xbox_EndianSwap.endSwapI32(xAxis_hvMemSize);
            zAxis_hvMemSize = Xbox_EndianSwap.endSwapI32(zAxis_hvMemSize);
            xzAxis_dcMemSize = Xbox_EndianSwap.endSwapI32(xzAxis_dcMemSize);
            yAxis_ranges = new Vector3(0, Xbox_EndianSwap.endSwapF32(yAxis_ranges.Y), Xbox_EndianSwap.endSwapF32(yAxis_ranges.Z));
            xAxis_ranges = new Vector3(Xbox_EndianSwap.endSwapF32(xAxis_ranges.X), Xbox_EndianSwap.endSwapF32(xAxis_ranges.Y), Xbox_EndianSwap.endSwapF32(xAxis_ranges.Z));
            zAxis_ranges = new Vector3(Xbox_EndianSwap.endSwapF32(zAxis_ranges.X), Xbox_EndianSwap.endSwapF32(zAxis_ranges.Y), Xbox_EndianSwap.endSwapF32(zAxis_ranges.Z));
            xyBasis_hvMemSize = Xbox_EndianSwap.endSwapI32(xyBasis_hvMemSize);
            zwBasis_hvMemSize = Xbox_EndianSwap.endSwapI32(zwBasis_hvMemSize);
         }
      };

      [StructLayout(LayoutKind.Sequential)]
      unsafe public struct XTDYAxisData
      {
         public short[] dCValues;           //dc coeffs - DXFLOAT16 - D3DFMT_R16
         public byte[] hvCoeffs;           //hv coeffs - DXTN - D3DFMT_DXN
      };
      [StructLayout(LayoutKind.Sequential)]
      unsafe public struct XTDXZAxisData
      {
         public byte[] dCValues;              //dc coeffs - 2 channel companded u8 - D3DFMT_R8G8
         public byte[] XhvCoeffs;             //hv coeffs - DXTN - D3DFMT_DXN
         public byte[] ZhvCoeffs;             //hv coeffs - DXTN - D3DFMT_DXN   
      };
      [StructLayout(LayoutKind.Sequential)]
      unsafe public struct XTDBasisData
      {
         public byte[] xyValues;              //xy values - DXTN - D3DFMT_DXN   
         public byte[] zwValues;              //zw values - DXTN - D3DFMT_DXN   
      };


      public XTDVisualHeader header;
      public XTDYAxisData yAxis;
      public XTDXZAxisData xzAxis;
      public XTDBasisData basis;

      public void clean()
      {
         yAxis.dCValues = null;
         yAxis.hvCoeffs = null;
         xzAxis.dCValues = null;
         xzAxis.XhvCoeffs = null;
         xzAxis.ZhvCoeffs = null;
         basis.xyValues = null;
         basis.zwValues = null;
      }
   };



}