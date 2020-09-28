using System;
using System.Windows.Forms;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using EditorCore;
using Rendering;

namespace LightingClient
{
   /// <summary>
   /// This is the quick, partial data transfered from the server
   /// </summary>
   public class LightingTerrainData
   {
      public void destroy()
      {
         if (mTerrainRelativePositions != null)
         {
            mTerrainRelativePositions = null;
         }
      }
      public Vector3[] mTerrainRelativePositions = null;
      public Vector3[] mTerrainNormals = null;
      public float[] mTerrainAOVals = null;

      public uint mNumXVerts;
      public uint mNumZVerts;
      public float mTileScale;
      public Vector3 mTerrainBBMin;
      public Vector3 mTerrainBBMax;

      public BTerrainQuadNodeDesc[] mQuadNodeDescArray = null;

      public Texture giveEntireTerrainInTexture()
      {
         try
         {
            int width = (int)mNumXVerts;
            int height = (int)mNumZVerts;
            Texture mainTex = new Texture(BRenderDevice.getDevice(), width, height, 1, Usage.None, Format.A32B32G32R32F, Pool.Managed);

            unsafe
            {
               GraphicsStream streamPos = mainTex.LockRectangle(0, LockFlags.None);
               Vector4* pos = (Vector4*)streamPos.InternalDataPointer;

               for (int x = 0; x < width; x++)
               {
                  for (int y = 0; y < height; y++)
                  {
                     int srcIndex = x * width + y;
                     int dstIndex = x * width + y;
                     Vector3 input = mTerrainRelativePositions[srcIndex];
                     pos[dstIndex].X = input.X;
                     pos[dstIndex].Y = input.Y;
                     pos[dstIndex].Z = input.Z;
                     pos[dstIndex].W = 1;
                  }
               }

               mainTex.UnlockRectangle(0);
            }

            return mainTex;
         }
         catch (OutOfVideoMemoryException eGPU)
         {
               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("ERROR : OUT OF GRAPHICS CARD MEMORY");
               Console.ForegroundColor = ConsoleColor.White;
            return null;
         }
         catch (OutOfMemoryException eCPU)
         {

               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("ERROR : OUT OF MAIN MEMORY");
               Console.ForegroundColor = ConsoleColor.White;
            return null;
         }
         return null;
      }

      public Vector3 getPos(int x, int y)
      {

         if (x >= mNumXVerts) x = (int)(mNumXVerts - 1);
         if (y >= mNumXVerts) y = (int)(mNumXVerts - 1);


         if (x > mNumXVerts) x = (int)(mNumXVerts - 1);
         int indx = (int)(x * mNumZVerts + y);
         Vector3 vec = mTerrainRelativePositions[indx];
         vec.X += x * mTileScale;
         vec.Z += y * mTileScale;
         return vec;


      }

      public Vector3 getNormal(int x, int y)
      {
         return mTerrainNormals[x * (int)mNumZVerts + y];
      }
      public float getAmbientOcclusion(int x, int y)
      {
         return mTerrainAOVals[x * (int)mNumZVerts + y];
      }

   };

}