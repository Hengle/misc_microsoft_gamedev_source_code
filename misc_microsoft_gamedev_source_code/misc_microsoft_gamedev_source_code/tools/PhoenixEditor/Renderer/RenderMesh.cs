using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Collections.Generic;
using EditorCore;
//--------------
namespace Rendering
{
   //----------------------
   public enum eMeshType
   {
      cMeshDebug     = (1 << 0),
      cMeshSkinned   = (1 << 1),
      cMeshRigid     = (1 << 2),
   }; 
   //----------------------
   public class BRenderMaterial
   {
      public void destroy()
      {
         if (mTextures != null)
         {
            for (int i = 0; i < mTextures.Count; i++)
            {
               mTextures[i].destroy();
               mTextures[i] = null;
            }
            mTextures = null;
         }

         if (mShader != null)
         {
            mShader.Dispose();
            mShader = null;
         }
      }
      public List<TextureHandle> mTextures = null;
      public Effect mShader = null;
   };
   //----------------------
   public class BRenderMaterialGroup
   {
      public void destroy()
      {
         if (mMaterial != null)
         {
            mMaterial.destroy();
            mMaterial = null;
         }
      }
      public BRenderMaterial mMaterial = null;
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

         if(mGroups !=null)
         {
            for(int i=0;i<mGroups.Count;i++)
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
         if (mPrimitives!=null)            
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
      public BBoundingBox mBounding;
      public List <BRenderPrimitive> mPrimitives;
   };
   //----------------------------
   public class VertexTypes
   {
      //CLM [03.20.06] ENSURE THAT YOU'RE ADDING ANY NEW VERTEX TYPES TO BRenderDevice.InitVertexTypes()
      public struct Pos
      {
         public float x, y, z;
         
        
         public Pos(float _x, float _y, float _z)
         {
            x = _x; y = _y; z = _z;

            if(Pos.vertDecl==null)
            {
               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               Pos.vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);

            }
         
         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position ;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_uv0
      {
         public float x, y, z;
         public float u0,v0;
         

         public Pos_uv0(float _x, float _y, float _z,float _u0, float _v0)
         {
            x = _x; y = _y; z = _z;
            u0=_u0;
            v0=_v0;

            if(Pos_uv0.vertDecl==null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
            

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      };
      public struct Pos_uv0_uv1
      {
         public float x, y, z;
         public float u0, v0;
         public float u1, v1;


         public Pos_uv0_uv1(float _x, float _y, float _z, float _u0, float _v0, float _u1, float _v1)
         {
            x = _x; y = _y; z = _z;
            u0 = _u0;
            v0 = _v0;
            u1 = _u1;
            v1 = _v1;

            if (Pos_uv0_uv1.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
               new VertexElement(0, 20, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 1),
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }


         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Texture2;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_Color
      {
         public float x, y, z;
         public int color;
         

         public Pos_Color(float _x, float _y, float _z, int _color)
         {
            x = _x; y = _y; z = _z;
            color = _color;

            if(Pos_Color.vertDecl==null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
				   
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
            
         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Diffuse;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_Color_uv0
      {
         public float x, y, z;
         public int color;
         public float u,v;


         public Pos_Color_uv0(float _x, float _y, float _z, float _u, float _v, int _color)
         {
            x = _x; y = _y; z = _z;
            u=_u;v=_v;
            color = _color;
            
            if(Pos_Color_uv0.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			   {
				   new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				   new VertexElement(0, 12, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
               new VertexElement(0, 24, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				   VertexElement.VertexDeclarationEnd,
			   };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
          
         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Diffuse | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      };

      public struct Pos_Normal_uv0
      {
         public float x, y, z;
         public float nx, ny, nz;
         public float u, v;
         

         public Pos_Normal_uv0(float _x, float _y, float _z, float _nx, float _ny, float _nz, float _u, float _v)
         {
            x = _x; y = _y; z = _z;
            nx = _nx; ny = _ny; nz = _nz;
            u = _u; v = _v;

            if (Pos_Normal_uv0.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 12, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Normal, 0),
                  new VertexElement(0, 24, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				      new VertexElement(0, 32, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Tangent, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Normal | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      };

      public struct PosW_uv0
      {
         public float x, y, z, w;
         public float u, v;
         public PosW_uv0(float _x, float _y, float _z, float _w, float _u0, float _v0)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            u = _u0;
            v = _v0;
            if (PosW_uv0.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 16, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }
         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      }
      public struct PosW_uv0_uv1
      {
         public float x, y, z, w;
         public float u0, v0;
         public float u1, v1;
         public PosW_uv0_uv1(float _x, float _y, float _z, float _w, float _u0, float _v0, float _u1, float _v1)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            u0 = _u0;
            v0 = _v0;
            u1 = _u1;
            v1 = _v1;
            if (PosW_uv0_uv1.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 16, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
                  new VertexElement(0, 24, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 1),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }
         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Texture2;
         public static VertexDeclaration vertDecl = null;
      }
      public struct PosW_uv03
      {
         public float x, y, z, w;
         public float u, v, t;
         public PosW_uv03(float _x, float _y, float _z, float _w, float _u0, float _v0, float _t0)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            u = _u0;
            v = _v0;
            t = _t0;
            if (PosW_uv03.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      new VertexElement(0, 16, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }
         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Texture1;
         public static VertexDeclaration vertDecl = null;
      }
      public struct PosW_color
      {
         public float x, y, z, w;
         public int color;

         public PosW_color(float _x, float _y, float _z, float _w, int _color)
         {
            x = _x;
            y = _y;
            z = _z;
            w = _w;
            color = _color;
            if (PosW_color.vertDecl == null)
            {

               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.PositionTransformed, 0),
				      new VertexElement(0, 16, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);
            }
         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Transformed | VertexFormats.Diffuse;
         public static VertexDeclaration vertDecl = null;




      };


      public struct Pos16
      {
         public float16 x, y, z, w;


         public Pos16(float _x, float _y, float _z)
         {
            x = new float16(_x); y = new float16(_y); z = new float16(_z);
            w = new float16(0);

            if (Pos16.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float16Four, DeclarationMethod.Default, DeclarationUsage.Position, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               Pos16.vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);

            }

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position;
         public static VertexDeclaration vertDecl = null;
      };
      public struct Pos16_Color
      {
         public ushort x, y, z, w;
         public int color;


         public Pos16_Color(float _x, float _y, float _z, int _color)
         {
            float16 tmp = new float16(0);
            tmp.fromFloat(_x); x = tmp.getInternalDat();
            tmp.fromFloat(_y); y = tmp.getInternalDat();
            tmp.fromFloat(_z); z = tmp.getInternalDat();
            w =0;

            color = _color;

            if (Pos16_Color.vertDecl == null)
            {
               VertexElement[] elements = new VertexElement[]
			      {
				      new VertexElement(0, 0, DeclarationType.Float16Four, DeclarationMethod.Default, DeclarationUsage.Position, 0),
                  new VertexElement(0, 8, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0),
				      VertexElement.VertexDeclarationEnd,
			      };
               Pos16_Color.vertDecl = new VertexDeclaration(BRenderDevice.getDevice(), elements);

            }

         }

         public static readonly VertexFormats FVF_Flags = VertexFormats.Position | VertexFormats.Diffuse;
         public static VertexDeclaration vertDecl = null;
      };

      public enum eVertexDeclElement
      {
         cVDE_Position = 0,
         cVDE_BlendWeight1,
         cVDE_BlendWeight4,

         cVDE_BlendIndicies,

         cVDE_Normal,

         cVDE_ColorDWORD,

         cVDE_TexCoord1,
         cVDE_TexCoord2,
         cVDE_TexCoord3,
         
         
         cVDE_Tangent, 
         cVDE_BiNormal, 

         cVDE_Pos16_4,
      };
      public static VertexDeclaration genVertexDecl(List<eVertexDeclElement> decls, bool sortForFVF, ref short vertexStructSize)
      {
         return genVertexDecl(decls, sortForFVF, ref vertexStructSize, false);
      }
      public static VertexDeclaration genVertexDecl(List<eVertexDeclElement> decls, bool sortForFVF, ref short vertexStructSize, bool addInstancingDecl)
      {
         if (sortForFVF)
            decls.Sort();
         List<VertexElement> vList = new List<VertexElement>();
         vertexStructSize=0;
         int textureCount = 0;
         for(int i=0;i<decls.Count;i++)
         {
            switch(decls[i])
            {
               case eVertexDeclElement.cVDE_Position:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Position, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;

               case eVertexDeclElement.cVDE_BlendIndicies:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.BlendIndices, 0));
                  vertexStructSize += sizeof(int);
                  break;

               case eVertexDeclElement.cVDE_BlendWeight1:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float1, DeclarationMethod.Default, DeclarationUsage.BlendWeight, 0));
                  vertexStructSize += sizeof(float);
                  break;
             
               case eVertexDeclElement.cVDE_BlendWeight4:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.BlendWeight, 0));
                  vertexStructSize += sizeof(int);
                  break;

               case eVertexDeclElement.cVDE_Normal:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Normal, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;
               case eVertexDeclElement.cVDE_Tangent:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.Tangent, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;
               case eVertexDeclElement.cVDE_BiNormal:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float3, DeclarationMethod.Default, DeclarationUsage.BiNormal, 0));
                  vertexStructSize += sizeof(float) * 3;
                  break;

               case eVertexDeclElement.cVDE_TexCoord2:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float2, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, (byte)textureCount));
                  vertexStructSize += sizeof(float) * 2;
                  textureCount++;
                  break;

               case eVertexDeclElement.cVDE_ColorDWORD:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Color, DeclarationMethod.Default, DeclarationUsage.Color, 0));
                  vertexStructSize += sizeof(int);
                  break;

               case eVertexDeclElement.cVDE_Pos16_4:
                  vList.Add(new VertexElement(0, vertexStructSize, DeclarationType.Float16Four, DeclarationMethod.Default, DeclarationUsage.Position, 0));
                  vertexStructSize += sizeof(short)*4;
                  break;
               
            }
            
         }

         if (addInstancingDecl)
         {
            vList.Add(new VertexElement(1, 0, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 5));
            vList.Add(new VertexElement(1, 16, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 6));
            vList.Add(new VertexElement(1, 32, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 7));
            vList.Add(new VertexElement(1, 48, DeclarationType.Float4, DeclarationMethod.Default, DeclarationUsage.TextureCoordinate, 8));
         }

         VertexElement[] elements = new VertexElement[vList.Count+1];
         for (int i = 0; i < vList.Count; i++)
         {
            elements[i] = vList[i];
         }
         elements[vList.Count ] = VertexElement.VertexDeclarationEnd;

         return new VertexDeclaration(BRenderDevice.getDevice(), elements);


      }
   };
}