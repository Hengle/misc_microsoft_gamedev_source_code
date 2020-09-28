
using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using Rendering;
using EditorCore;

namespace Sim
{

   //----------------------
   public class BRenderMaterialGroup
   {
      public void destroy()
      {
         
      }
      public int mStartIndex;
      public int mPrimCount;
   }
   //----------------------
   public class BRenderPrimitive
   {
      public BRenderPrimitive()
      {
         mGroups = new List<BRenderMaterialGroup>();
      }
      ~BRenderPrimitive()
      {
         destroy();
      }


      public void destroy()
      {
         if (mVB != null)
         {
            mVB.Dispose();
            mVB = null;
         }

         if (mIB != null)
         {
            mIB.Dispose();
            mIB = null;
         }

         if (mVDecl != null)
         {
            mVDecl.Dispose();
            mVDecl = null;
         }

         if (mGroups != null)
         {
            for (int i = 0; i < mGroups.Count; i++)
            {
               mGroups[i].destroy();
               mGroups[i] = null;
            }
            mGroups = null;
         }
      }

      public virtual void render()
      {
      }
      //------------------
      public int mMeshType;                  //any combination of eMeshType;
      public int mNumVerts;
      public int mNumInds;
      public VertexBuffer mVB = null;
      public IndexBuffer mIB = null;
      public VertexDeclaration mVDecl = null;
      public int mVertexSize;

      public List<BRenderMaterialGroup> mGroups;

   };
   //----------------------
   public class BRenderMesh
   {
      public void destroy()
      {
         if (mPrimitives != null)
         {
            for (int i = 0; i < mPrimitives.Count; i++)
            {
               if (mPrimitives[i] != null)
               {
                  mPrimitives[i].destroy();
                  mPrimitives[i] = null;
               }
            }
            mPrimitives = null;
         }
      }

      public virtual void render() { }
      public List<BRenderPrimitive> mPrimitives;
   };

   //----------------------
   public class BRenderGrannyMesh : BRenderMesh
   {
      public BRenderGrannyMesh()
      {
         mPrimitives = new List<BRenderPrimitive>();
        
      }
      ~BRenderGrannyMesh()
      {
         destroy();
      }

      public int GetNumPrimitives()
      {
         return mPrimitives.Count;
      }

      public override void render()
      {
         if (mPrimitives == null) return;

         for (int k = 0; k < mPrimitives.Count; k++)
         {
            //BRenderDevice.getDevice().VertexShader = null;
          //  BRenderDevice.getDevice().PixelShader = null;
            BRenderDevice.getDevice().VertexDeclaration = mPrimitives[k].mVDecl;

            BRenderDevice.getDevice().SetStreamSource(0, mPrimitives[k].mVB, 0, mPrimitives[k].mVertexSize);

            if (mPrimitives[k].mIB != null)
               BRenderDevice.getDevice().Indices = mPrimitives[k].mIB;

            // And now render the groups with the right textures and materials.
           // BRenderDevice.getDevice().SetTexture(0, null);
            for (int i = 0; i < mPrimitives[k].mGroups.Count; i++)
            {
               BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, mPrimitives[k].mNumVerts, mPrimitives[k].mGroups[i].mStartIndex, mPrimitives[k].mGroups[i].mPrimCount);

            }
         }
      }

      //-----------------
      public unsafe void addRenderPrimitive(BRenderPrimitive prim)
      {
         mPrimitives.Add(prim);
      }
   }
}