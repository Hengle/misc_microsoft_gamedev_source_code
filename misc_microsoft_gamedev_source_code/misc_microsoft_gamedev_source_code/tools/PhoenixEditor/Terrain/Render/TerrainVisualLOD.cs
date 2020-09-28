using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using Rendering;
//
//--------------------------------------------
namespace Terrain
{

   // ///////////////////////////////////////
   // ///////////////////////////////////////
   // ///////////////////////////////////////
   // ///////////////////////////////////////
   // ///////////////////////////////////////


   class BTerrainVisualLODData
   {
      public BTerrainVisualLODData()
      {

      }
      ~BTerrainVisualLODData()
      {
         destroy();
         mVertexBuffers = null;


      }
      public void init()
      {
         destroy();

         //Create VB's
         makeLODVB(ref mVertexBuffers[0], ref mNumVerts[0], BTerrainVisual.eLODLevel.cLOD0);
         makeLODVB(ref mVertexBuffers[1], ref mNumVerts[1], BTerrainVisual.eLODLevel.cLOD1);
         makeLODVB(ref mVertexBuffers[2], ref mNumVerts[2], BTerrainVisual.eLODLevel.cLOD2);
         makeLODVB(ref mVertexBuffers[3], ref mNumVerts[3], BTerrainVisual.eLODLevel.cLOD3);

         makeCursorLODVB(ref mCursorVertexBuffers[0], ref mNumCursorVerts[0], BTerrainVisual.eLODLevel.cLOD0);
         makeCursorLODVB(ref mCursorVertexBuffers[1], ref mNumCursorVerts[1], BTerrainVisual.eLODLevel.cLOD1);
         makeCursorLODVB(ref mCursorVertexBuffers[2], ref mNumCursorVerts[2], BTerrainVisual.eLODLevel.cLOD2);
         makeCursorLODVB(ref mCursorVertexBuffers[3], ref mNumCursorVerts[3], BTerrainVisual.eLODLevel.cLOD3);
      }
      public void destroy()
      {
         //VBs
         if (mVertexBuffers != null)
         {
            for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
            {
               if (mVertexBuffers[i] != null)
               {
                  mVertexBuffers[i].Dispose();
                  mVertexBuffers[i] = null;
               }
            }
         }

         if (mCursorVertexBuffers != null)
         {
            for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
            {
               if (mCursorVertexBuffers[i] != null)
               {
                  mCursorVertexBuffers[i].Dispose();
                  mCursorVertexBuffers[i] = null;
               }
            }
         }
      }

      //queary
      public BTerrainVisual.eLODLevel getLODLevel(Vector3 pt, Vector3 Eye)
      {
         if (!TerrainGlobals.getVisual().isDynamicLODEnabled())
         {
            return TerrainGlobals.getVisual().getStaticLODLevel();
         }

         Vector3 d = pt - Eye;
         float dist = d.Length();

         if (dist > (float)BTerrainVisual.eLODLevelDists.cLOD3Dist)
            return BTerrainVisual.eLODLevel.cLOD3;
         else if (dist > (float)BTerrainVisual.eLODLevelDists.cLOD2Dist)
            return BTerrainVisual.eLODLevel.cLOD2;
         else if (dist > (float)BTerrainVisual.eLODLevelDists.cLOD1Dist)
            return BTerrainVisual.eLODLevel.cLOD1;

         return BTerrainVisual.eLODLevel.cLOD0;
      }

      //lodVB
      public void getLODVB(ref VertexBuffer vb, ref uint numVerts, BTerrainVisual.eLODLevel lod)
      {
         vb = mVertexBuffers[(int)lod];
         numVerts = mNumVerts[(int)lod];
      }
      public void getCursorLODVB(ref VertexBuffer vb, ref uint numVerts, BTerrainVisual.eLODLevel lod)
      {
         vb = mCursorVertexBuffers[(int)lod];
         numVerts = mNumCursorVerts[(int)lod];
      }


      //creation
      private void makeLODVB(ref VertexBuffer vb, ref uint numVerts, BTerrainVisual.eLODLevel lod)
      {
         VertexTypes.Pos[] verts = null;

         uint width = BTerrainQuadNode.getMaxNodeWidth();
         uint vd = width + 1;
         uint tw = width;
         uint td = width;
         int counter = 0;

         switch (lod)
         {
            case BTerrainVisual.eLODLevel.cLOD0:
               {
                  numVerts = (BTerrainQuadNode.getMaxNodeWidth() + 1) * (BTerrainQuadNode.getMaxNodeDepth() + 1) * 6;   //*3 (TRILIST) *2 (per tile)
                  verts = new VertexTypes.Pos[numVerts];


                  vd = width * 2;// width + 1;
                  tw = width;
                  td = width;
                  for (int z = 0; z < td; z++)
                  {
                     for (int x = 0; x < tw; x++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + 1);

                        verts[counter++].x = (k + 1);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + vd + 1);
                     }
                  }
               }
               break;
            case BTerrainVisual.eLODLevel.cLOD1:
               {
                  numVerts = (1 + (width >> 1)) * (1 + (width >> 1)) * 6;   //*3 (TRILIST) *2 (per tile)
                  verts = new VertexTypes.Pos[numVerts];

                  vd = (width >> 1) * 2;// width + 1;
                  tw = (width >> 1);
                  td = (width >> 1);
                  for (int z = 0; z < td; z++)
                  {
                     for (int x = 0; x < tw; x++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + 1);

                        verts[counter++].x = (k + 1);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + vd + 1);
                     }
                  }
               }
               break;
            case BTerrainVisual.eLODLevel.cLOD2:
               {
                  numVerts = (1 + (width >> 2)) * (1 + (width >> 2)) * 6;   //*3 (TRILIST) *2 (per tile)
                  verts = new VertexTypes.Pos[numVerts];

                  vd = (width >> 2) * 2;// width + 1;
                  tw = (width >> 2);
                  td = (width >> 2);
                  for (int z = 0; z < td; z++)
                  {
                     for (int x = 0; x < tw; x++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + 1);

                        verts[counter++].x = (k + 1);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + vd + 1);
                     }
                  }
               }
               break;
            case BTerrainVisual.eLODLevel.cLOD3:
               {
                  numVerts = (1 + (width >> 3)) * (1 + (width >> 3)) * 6;   //*3 (TRILIST) *2 (per tile)
                  verts = new VertexTypes.Pos[numVerts];

                  vd = (width >> 3) * 2;// width + 1;
                  tw = (width >> 3);
                  td = (width >> 3);
                  for (int z = 0; z < td; z++)
                  {
                     for (int x = 0; x < tw; x++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + 1);

                        verts[counter++].x = (k + 1);
                        verts[counter++].x = (k + vd);
                        verts[counter++].x = (k + vd + 1);
                     }
                  }
               }
               break;
            default:
               return;

         }

         vb = new VertexBuffer(typeof(VertexTypes.Pos), (int)numVerts, BRenderDevice.getDevice(), Usage.None, VertexFormats.Position, Pool.Managed);
         GraphicsStream stream = vb.Lock(0, (int)numVerts * sizeof(float) * 3, LockFlags.None);
         stream.Write(verts);
         vb.Unlock();

         verts = null;
      }
      private void makeCursorLODVB(ref VertexBuffer vb, ref uint numVerts, BTerrainVisual.eLODLevel lod)
      {
         VertexTypes.Pos[] verts = null;

         uint width = BTerrainQuadNode.getMaxNodeWidth();
         uint vd = width * 2;
         uint tw = width;
         uint td = width;
         int counter = 0;

         switch (lod)
         {
            case BTerrainVisual.eLODLevel.cLOD0:
               {
                  numVerts = (width + 1) * (width + 1);
                  verts = new VertexTypes.Pos[numVerts];

                  vd = width * 2;
                  tw = width;
                  td = width;
                  for (int z = 0; z < td; z++)
                  {
                     for (int x = 0; x < tw; x++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);
                     }
                  }
               }
               break;
            case BTerrainVisual.eLODLevel.cLOD1:
               {
                  numVerts = (1 + (width >> 1)) * (1 + (width >> 1));
                  verts = new VertexTypes.Pos[numVerts];

                  tw = width >> 1;
                  td = width >> 1;
                  vd = 2 * (width >> 1);
                  for (int x = 0; x < tw; x++)
                  {
                     for (int z = 0; z < td; z++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);
                     }
                  }
               }
               break;
            case BTerrainVisual.eLODLevel.cLOD2:
               {
                  numVerts = (1 + (width >> 2)) * (1 + (width >> 2));
                  verts = new VertexTypes.Pos[numVerts];

                  tw = width >> 2;
                  td = width >> 2;
                  vd = 2 * (width >> 2);
                  for (int x = 0; x < tw; x++)
                  {
                     for (int z = 0; z < td; z++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);

                     }
                  }
               }
               break;
            case BTerrainVisual.eLODLevel.cLOD3:
               {
                  numVerts = (1 + (width >> 3)) * (1 + (width >> 3));
                  verts = new VertexTypes.Pos[numVerts];

                  tw = width >> 3;
                  td = width >> 3;
                  vd = 2 * (width >> 3);
                  for (int x = 0; x < tw; x++)
                  {
                     for (int z = 0; z < td; z++)
                     {
                        int k = z + (int)(x * vd);
                        verts[counter++].x = (k);

                     }
                  }
               }
               break;
            default:
               return;

         }

         vb = new VertexBuffer(typeof(VertexTypes.Pos), (int)numVerts, BRenderDevice.getDevice(), Usage.None, VertexFormats.Position, Pool.Managed);
         GraphicsStream stream = vb.Lock(0, (int)numVerts * sizeof(float) * 3, LockFlags.None);
         stream.Write(verts);
         vb.Unlock();

         verts = null;
      }

      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------

      //LOD Vertex Buffers
      private VertexBuffer[] mVertexBuffers = new VertexBuffer[(int)BTerrainVisual.eLODLevel.cLODCount];
      private uint[] mNumVerts = new uint[(int)BTerrainVisual.eLODLevel.cLODCount];

      private VertexBuffer[] mCursorVertexBuffers = new VertexBuffer[(int)BTerrainVisual.eLODLevel.cLODCount];
      private uint[] mNumCursorVerts = new uint[(int)BTerrainVisual.eLODLevel.cLODCount];
   }

}