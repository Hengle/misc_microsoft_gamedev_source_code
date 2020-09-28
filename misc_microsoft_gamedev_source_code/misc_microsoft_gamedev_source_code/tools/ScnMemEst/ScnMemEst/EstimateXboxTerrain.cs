
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace ScnMemEst
{
   class Xbox_EndianSwap
   {
      //-----------------------------------------------------------------------------
      public static ushort endSwapI16(ushort i)
      {
         return (ushort)((i << 8) | (i >> 8));
      }
      //-----------------------------------------------------------------------------
      public static int endSwapI32(int i)
      {
         return endSwapI16((ushort)(i & 0x0000FFFF)) << 16 | endSwapI16((ushort)(i >> 16));
      }
      //-----------------------------------------------------------------------------
      public static Int64 endSwapI64(Int64 i)
      {
         Int64 big = endSwapI32((Int32)(i & 0x00000000FFFFFFFF));
         big = big << 32;
         Int32 small = endSwapI32((Int32)(i >> 32));
         return big | small;
      }

      //-----------------------------------------------------------------------------
      public static float endSwapF32(float f)
      {
         byte[] b = BitConverter.GetBytes(f);
         Array.Reverse(b);

         return BitConverter.ToSingle(b, 0);
      }
   }

   
   class XboxTerrainEstimate
   {
      string scnName = null;
      string gameDirectory = null;
      ScnMemoryEstimate mMemEst = null;

      public void estimateMemory(string scenarioName, string gameWorkDirectory, ScnMemoryEstimate memEst)
      {
         scnName = scenarioName;
         gameDirectory = gameWorkDirectory;
         mMemEst = memEst;

         calculateXTDMemory();
         calculateXTHMemory();
         calculateXTTMemory();
         calculateXSDMemory();
      }


      void calculateXTDMemory()
      {
         ECFReader ecfR = new ECFReader();

         string XTDName = Path.ChangeExtension(gameDirectory + @"\scenario\" + scnName, ".XTD");
         if (!ecfR.openForRead(XTDName))
            return;

         for (uint i = 0; i < ecfR.getNumChunks(); i++)
         {
            ECF.BECFChunkHeader chunkHeader = ecfR.getChunkHeader(i);
            eXTD_ChunkID id = (eXTD_ChunkID)chunkHeader.mID;
            switch (id)
            {
               case eXTD_ChunkID.cXTD_XTDHeader:
                  break;

               case eXTD_ChunkID.cXTD_AOChunk:
                  mMemEst.setOrAddMemoryElement("Terrain AO", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

               case eXTD_ChunkID.cXTD_AtlasChunk:
                  mMemEst.setOrAddMemoryElement("Terrain Verts & Normals", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

               case eXTD_ChunkID.cXTD_TessChunk:
                  mMemEst.setOrAddMemoryElement("Terrain Tessellation", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

               case eXTD_ChunkID.cXTD_LightingChunk:
                  mMemEst.setOrAddMemoryElement("Terrain Lighting", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

               case eXTD_ChunkID.cXTD_AlphaChunk:
                  mMemEst.setOrAddMemoryElement("Terrain Alpha", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;
            }
         }

         ecfR.close();
         ecfR = null;

      }

      void calculateXTHMemory()
      {
         ECFReader ecfR = new ECFReader();

         string XTDName = Path.ChangeExtension(gameDirectory + @"\scenario\" + scnName, ".XTH");
         if (!ecfR.openForRead(XTDName))
            return;

         for (uint i = 0; i < ecfR.getNumChunks(); i++)
         {
            ECF.BECFChunkHeader chunkHeader = ecfR.getChunkHeader(i);
            eXTH_ChunkID id = (eXTH_ChunkID)chunkHeader.mID;
            switch (id)
            {

               case eXTH_ChunkID.cXTH_TerrainHeightfield:
                  mMemEst.setOrAddMemoryElement("Terrain Decal Mesh", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

            }
         }

         ecfR.close();
         ecfR = null;
      }

      int giveTextureCacheMemoryRequirement(int numCachePages, int mip0Width, int numMips, int fmt)
      {
         int bitsPerPixel = 0;
         if (fmt == 0) //DXT1
            bitsPerPixel = 4;
         if (fmt == 1) //DXN / DXT5
            bitsPerPixel = 8;

         int mip0Size = (numCachePages * (mip0Width * mip0Width) * bitsPerPixel) >> 3;

         int mipChainSize = 0;
         for (int R = 1; R < numMips; R++)
            mipChainSize += mip0Size >> (2 * R);

         return mip0Size + mipChainSize;
      }

      public int giveDependentTextureMemoryFootprint(String inFilename, ref bool specCache, ref bool selfCache, ref bool envCache)
      {
         int totalMemory = 0;
         String filename = inFilename;
         String[] extentionTypes = new String[] { "_df", "_nm", "_sp", "_em", "_rm", "_op" };

         for (int i = 0; i < extentionTypes.Length; i++)
         {
            if (Path.GetFileNameWithoutExtension(inFilename).LastIndexOf(extentionTypes[i]) != -1)
            {
               filename = filename.Substring(0, filename.LastIndexOf(extentionTypes[i]));

               break;
            }
         }

         //remove extention if it exists in the filename
         if (Path.GetExtension(filename) != "")
            filename = Path.ChangeExtension(filename, "");


         for (int i = 0; i < extentionTypes.Length; i++)
         {
            String tPath = gameDirectory + @"\art\terrain\" + filename + extentionTypes[i] + ".ddx";
            if (File.Exists(tPath))
            {
               totalMemory += DDXBridge.give360TextureMemFootprint(tPath);

               if (i == 2) specCache |= true;
               if (i == 3) selfCache |= true;
               if (i == 4) envCache |= true;
            }

         }

         return totalMemory;
      }
      void calculateXTTMemory()
      {
         ECFReader ecfR = new ECFReader();

         string XTDName = Path.ChangeExtension(gameDirectory + @"\scenario\" + scnName, ".XTT");
         if (!ecfR.openForRead(XTDName))
            return;

         for (uint i = 0; i < ecfR.getNumChunks(); i++)
         {
            ECF.BECFChunkHeader chunkHeader = ecfR.getChunkHeader(i);
            eXTT_ChunkID id = (eXTT_ChunkID)chunkHeader.mID;
            switch (id)
            {

               case eXTT_ChunkID.cXTT_XTTHeader:

                  //CACHES
                  //add in our 360 cache data
                  int cacheMemCount = 0;
                  const int numCachePages = 20;
                  const int cachePageSize = 512;
                  const int numMips = 2;
                  bool albedoCache = true;
                  bool normalCache = true;
                  bool specCache = false;
                  bool selfCache = false;
                  bool envCache = false;


                  //find our textures in the list
                  int version = ecfR.readInt32();
                  int numActiveTextures = Xbox_EndianSwap.endSwapI32(ecfR.readInt32());
                  int numActiveDecals = Xbox_EndianSwap.endSwapI32(ecfR.readInt32());
                  int numActiveDecalInstances = ecfR.readInt32();

                  System.Text.Encoding enc = System.Text.Encoding.ASCII;

                  int totalArtistTextureMem = 0;
                  for (int k = 0; k < numActiveTextures;k++ )
                  {
                     byte[] fName = ecfR.readBytes(256);    
                     string textureName = enc.GetString(fName );
                     textureName = textureName.TrimEnd('\0');
                     textureName = textureName.TrimStart('\0');
                     totalArtistTextureMem += giveDependentTextureMemoryFootprint(textureName, ref specCache, ref selfCache, ref envCache);

                     int b = ecfR.readInt32();
                     b = ecfR.readInt32();
                     b = ecfR.readInt32();

                  }

                  for (int k = 0; k < numActiveDecals; k++)
                  {
                     byte[] fName = ecfR.readBytes(256);
                     string textureName = enc.GetString(fName);
                     textureName = textureName.TrimEnd('\0');
                     textureName = textureName.TrimStart('\0');
                     totalArtistTextureMem += giveDependentTextureMemoryFootprint(textureName, ref specCache, ref selfCache, ref envCache);
                  }

                  mMemEst.setOrAddMemoryElement("Terrain Artist Texture", totalArtistTextureMem, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);


                  if (albedoCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 0);   //DXT1 * numCachePages (mip0 & mip1)
                  if (normalCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 1); ;//DXN
                  if (specCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 0);  //DXT1
                  if (envCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 0);   //DXT1
                  if (selfCache) cacheMemCount += giveTextureCacheMemoryRequirement(numCachePages, cachePageSize, numMips, 1);  //DXT5
                  mMemEst.setOrAddMemoryElement("Terrain Texture Cache", cacheMemCount, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);



                  //cache calculation
                  break;

               case eXTT_ChunkID.cXTT_AtlasChunkAlbedo:
                  mMemEst.setOrAddMemoryElement("Terrain Skirt Texture", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

               case eXTT_ChunkID.cXTT_RoadsChunk:
                  mMemEst.setOrAddMemoryElement("Terrain Roads", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

               case eXTT_ChunkID.cXTT_FoliageQNChunk:
                  mMemEst.setOrAddMemoryElement("Terrain Foliage", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;

               case eXTT_ChunkID.cXTT_TerrainAtlasLinkChunk:
                  mMemEst.setOrAddMemoryElement("Terrain Blends", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;
                  
            }
         }

         ecfR.close();
         ecfR = null;
      }

      void calculateXSDMemory()
      {
         ECFReader ecfR = new ECFReader();

         string XTDName = Path.ChangeExtension(gameDirectory + @"\scenario\" + scnName, ".XSD");
         if (!ecfR.openForRead(XTDName))
            return;

         for (uint i = 0; i < ecfR.getNumChunks(); i++)
         {
            ECF.BECFChunkHeader chunkHeader = ecfR.getChunkHeader(i);
            eXSD_ChunkID id = (eXSD_ChunkID)chunkHeader.mID;
            switch (id)
            {

               case eXSD_ChunkID.cXSD_SimHeights:
               case eXSD_ChunkID.cXSD_Obstructions:
               case eXSD_ChunkID.cXSD_TileTypes:
               case eXSD_ChunkID.cXSD_CameraHeights:
               case eXSD_ChunkID.cXSD_Buildable:
               case eXSD_ChunkID.cXSD_FloodObstructions:
               case eXSD_ChunkID.cXSD_ScarabObstructions:

                  mMemEst.setOrAddMemoryElement("Terrain Sim Rep", chunkHeader.mSize, ScnMemoryEstimate.eMainCatagory.eCat_Terrain);
                  break;
            }
         }

         ecfR.close();
         ecfR = null;
      }
   }






   public enum eFileVersions
   {
      cXTDVersion = 0x000C,
      cXTTVersion = 0x0004,
      cXTHVersion = 0x0001,
      cXSDVersion = 0x0004,
   };

   enum eECFChunkType
   {
      cVBChunk = 0,
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

   };

   public enum eXSD_ChunkID
   {
      cXSD_XSDHeader = 0x1111,
      cXSD_SimHeights = 0x2222,
      cXSD_Obstructions = 0x4444,
      cXSD_TileTypes = 0x8888,
      cXSD_CameraHeights = 0xAAAA,
      cXSD_Buildable = 0xCCCC,
      cXSD_FloodObstructions = 0xDDDD,
      cXSD_ScarabObstructions = 0xEEEE,
   };
}