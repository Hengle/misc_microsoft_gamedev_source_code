using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Threading;
using Rendering;
//
//--------------------------------------------
namespace LightingClient
{

   // ///////////////////////////////////////
   // ///////////////////////////////////////
   // ///////////////////////////////////////
   // ///////////////////////////////////////
   // ///////////////////////////////////////


   public class BTerrainVisualLODData
   {
      public enum eLODLevel
      {
         cLOD0 = 0,
         cLOD1,
         cLOD2,
         cLOD3,
         cLODCount
      };

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
         makeLODVB(ref mVertexBuffers[0], ref mNumVerts[0], eLODLevel.cLOD0);
         makeLODVB(ref mVertexBuffers[1], ref mNumVerts[1], eLODLevel.cLOD1);
         makeLODVB(ref mVertexBuffers[2], ref mNumVerts[2], eLODLevel.cLOD2);
         makeLODVB(ref mVertexBuffers[3], ref mNumVerts[3], eLODLevel.cLOD3);

      }
      public void destroy()
      {
         //VBs
         if (mVertexBuffers != null)
         {
            for (int i = 0; i < (int)eLODLevel.cLODCount; i++)
            {
               if (mVertexBuffers[i] != null)
               {
                  mVertexBuffers[i].Dispose();
                  mVertexBuffers[i] = null;
               }
            }
         }

      
      }

      //queary
    

      //lodVB
      public void getLODVB(ref VertexBuffer vb, ref uint numVerts, eLODLevel lod)
      {
         vb = mVertexBuffers[(int)lod];
         numVerts = mNumVerts[(int)lod];
      }



      //creation
      private void makeLODVB(ref VertexBuffer vb, ref uint numVerts, eLODLevel lod)
      {
         VertexTypes.Pos[] verts = null;

         uint width = BTerrainQuadNodeDesc.cMaxWidth;
         uint vd = width + 1;
         uint tw = width;
         uint td = width;
         int counter = 0;

         switch (lod)
         {
            case eLODLevel.cLOD0:
               {
                  numVerts = (BTerrainQuadNodeDesc.cMaxWidth + 1) * (BTerrainQuadNodeDesc.cMaxHeight + 1) * 6;   //*3 (TRILIST) *2 (per tile)
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
            case eLODLevel.cLOD1:
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
            case eLODLevel.cLOD2:
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
            case eLODLevel.cLOD3:
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

      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------
      //----------------------------------------------

      //LOD Vertex Buffers
      private VertexBuffer[] mVertexBuffers = new VertexBuffer[(int)eLODLevel.cLODCount];
      private uint[] mNumVerts = new uint[(int)eLODLevel.cLODCount];

   }

}