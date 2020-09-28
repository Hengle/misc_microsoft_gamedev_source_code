using System;
using System.Drawing;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using System.IO;


using Rendering;
using EditorCore;

namespace ModelSystem
{
   //----------------------------
   public class BRenderGrannyMesh : BRenderMesh
   {
      
      public BRenderGrannyMesh()
      {
         mPrimitives = new List<BRenderPrimitive>();
         mBBox = new BBoundingBox();
      }
      ~BRenderGrannyMesh()
      {
         destroy();
      }

      public List<string> mTextureFilenames = new List<string>();
      //public List<string> mAllTextureFilenames = new List<string>();
      public List<string> mAllFilenames = new List<string>();
      //public List<string> mAllListedRelativeTextureFilenames = new List<string>();
      //public List<string> mAllListedNotRelativeFilenames = new List<string>();
      //public List<string> mAllListedOtherRelativeFilenames = new List<string>();


      public Dictionary<string, TextureHandle> mTextureCache = new Dictionary<string, TextureHandle>();

      public int GetNumPrimitives()
      {
         return mPrimitives.Count;
      }

      public override void render()
      {
         if (mPrimitives == null) return;

         for (int k = 0; k < mPrimitives.Count; k++)
         {
            BRenderDevice.getDevice().VertexShader = null;
            BRenderDevice.getDevice().PixelShader = null;
            BRenderDevice.getDevice().VertexDeclaration = mPrimitives[k].mVDecl;

            BRenderDevice.getDevice().SetStreamSource(0, mPrimitives[k].mVB, 0, mPrimitives[k].mVertexSize);

            if (mPrimitives[k].mIB != null)
               BRenderDevice.getDevice().Indices = mPrimitives[k].mIB;

            // And now render the groups with the right textures and materials.
            BRenderDevice.getDevice().SetTexture(0, null);
            for (int i = 0; i < mPrimitives[k].mGroups.Count; i++)
            {
               if (mPrimitives[k].mGroups[i].mMaterial != null)
               {
                  if(mPrimitives[k].mGroups[i].mMaterial.mTextures[0].mTexture!=null)
                     BRenderDevice.getDevice().SetTexture(0, mPrimitives[k].mGroups[i].mMaterial.mTextures[0].mTexture);
                  else
                     BRenderDevice.getDevice().SetTexture(0, BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_Red));
               }
               else
               {
                  BRenderDevice.getDevice().SetTexture(0, null);
               }
               BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, mPrimitives[k].mNumVerts, mPrimitives[k].mGroups[i].mStartIndex, mPrimitives[k].mGroups[i].mPrimCount);
               
            }
         }
      }

      //-----------------
      public unsafe void addRenderPrimitive(BRenderPrimitive prim)
      {
         mPrimitives.Add(prim);
      }

      public unsafe bool testRayIntersection(Vector3 r0, Vector3 rD, Matrix worldMat)
      {
          if (mPrimitives == null) return true;
          bool hit = false;

          for (int k = 0; k < mPrimitives.Count; k++)
          {
             GraphicsStream VBStream = mPrimitives[k].mVB.Lock(0, 0,LockFlags.ReadOnly);
             GraphicsStream IBStream = mPrimitives[k].mIB.Lock(0, 0,LockFlags.ReadOnly);
             int* inds = (int*)IBStream.InternalDataPointer;
             float* verts = (float*)VBStream.InternalDataPointer;

             int vSize = mPrimitives[k].mVertexSize / sizeof(float);
             int numTris = mPrimitives[k].mNumInds/3;

             Vector3[] kverts = new Vector3[3];
             int vIndex = 0;
             Vector3 iPoint = Vector3.Empty;

             int c = 0;
             for (int i = 0; i < numTris;i++ )
             {
                kverts[0] = Vector3.Empty;
                vIndex = inds[c++];
                kverts[0].X = verts[vIndex * vSize + 0];
                kverts[0].Y = verts[vIndex * vSize + 1];
                kverts[0].Z = verts[vIndex * vSize + 2];

                kverts[1] = Vector3.Empty;
                vIndex = inds[c++];
                kverts[1].X = verts[vIndex * vSize + 0];
                kverts[1].Y = verts[vIndex * vSize + 1];
                kverts[1].Z = verts[vIndex * vSize + 2];

                kverts[2] = Vector3.Empty;
                vIndex = inds[c++];
                kverts[2].X = verts[vIndex * vSize + 0];
                kverts[2].Y = verts[vIndex * vSize + 1];
                kverts[2].Z = verts[vIndex * vSize + 2];

                //transform the verts by the world matrix
                for (int q = 0; q < 3;q++ )
                {
                   Vector4 p = Vector3.Transform(kverts[q],worldMat);
                   kverts[q].X = p.X;
                   kverts[q].Y = p.Y;
                   kverts[q].Z = p.Z;
                }

                   if (BMathLib.raySegmentIntersectionTriangle(kverts, ref r0, ref rD, false, ref iPoint))
                   {
                      hit = true;
                      break;
                   }
             }

             mPrimitives[k].mVB.Unlock();
             mPrimitives[k].mIB.Unlock();
          }
          return hit;
      }
      public BBoundingBox mBBox;
   }

}  
