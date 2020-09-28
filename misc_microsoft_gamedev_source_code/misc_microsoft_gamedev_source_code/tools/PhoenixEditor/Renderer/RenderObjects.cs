
using System;
using System.Drawing;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Text;
using EditorCore;

namespace Rendering
{

   public class TextRendring
   {
      static bool mbFontsReady = false;
      static Microsoft.DirectX.Direct3D.Font mFont1 = null;
      static Microsoft.DirectX.Direct3D.Font mFont2 = null;
      static Microsoft.DirectX.Direct3D.Font mFont3 = null;
      static void Init()
      {
         mbFontsReady = true;

         string fontName = "Arial";
         mFont1 = new Microsoft.DirectX.Direct3D.Font(BRenderDevice.getDevice(), 16, 0, FontWeight.Regular,
             1, false, CharacterSet.Default, Precision.Default, FontQuality.Default,
             PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);
         mFont2 = new Microsoft.DirectX.Direct3D.Font(BRenderDevice.getDevice(), 24, 0, FontWeight.Regular,
             1, false, CharacterSet.Default, Precision.Default, FontQuality.Default,
             PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);
         mFont3 = new Microsoft.DirectX.Direct3D.Font(BRenderDevice.getDevice(), 36, 0, FontWeight.Regular,
             1, false, CharacterSet.Default, Precision.Default, FontQuality.Default,
             PitchAndFamily.DefaultPitch | PitchAndFamily.FamilyDoNotCare, fontName);
      }

      static public void renderText(string text, Vector3 pos3D, System.Drawing.Color color, float size)
      {
         if (mbFontsReady == false)
            Init();

         Matrix projMat = BRenderDevice.getDevice().GetTransform(TransformType.Projection);
         Matrix viewMat = BRenderDevice.getDevice().GetTransform(TransformType.View);
         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);

         Viewport vport;
         vport = BRenderDevice.getDevice().Viewport;

         Vector3 pos2D;

         pos2D = Vector3.Project(pos3D, vport, projMat, viewMat, worldMat);
         pos2D.X += 5.0f;
         pos2D.Y -= 15.0f;

         Microsoft.DirectX.Direct3D.Font font = null;
         if (size > 5 && size <= 16)
         {
            font = mFont1;
         }
         else if (size > 16 && size <= 24)
         {
            font = mFont2;
         }
         else if (size > 24 )//&& size <= 36)
         {
            font = mFont3;
         }
         if (font != null)
         {
            font.DrawText(null, text, new System.Drawing.Rectangle((int)pos2D.X, (int)pos2D.Y, 0, 0),
                                          DrawTextFormat.NoClip, color);
         }
      }

   }


   //----------------------------
   public class BRenderDebugSphere : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();

      public BRenderDebugSphere(float rad,int color)
      {
         createSphere(rad, 3, color);
      }
      public BRenderDebugSphere(float rad, int numSubdivisions, int color)
      {
         createSphere(rad, numSubdivisions, color);
      }
      private void createSphere(float rad, int numSubdivisions, int color)
      {
         mMeshType = (int)eMeshType.cMeshDebug;
         mColor = color;

         // CLM [04.17.06]
	      // I got this method for calculating the coordinates of a icosahedron from the website
	      // http://www.georgehart.com/virtual-polyhedra/ex-pr1.html


         int nNumSubdivisions = numSubdivisions;
         
         float size=rad;

         int nNumIcoPositions = 12;
         int nNumIcoIndices = 60;
      

	      float tao = 1.61803f;
         double dtao = (double)(1 + tao * tao );
	      float mag = (float)(Math.Sqrt( dtao));
	      float a = (1.0f / mag);
	      float b = (tao / mag) ;

	      Vector3 [] icopoints =  new Vector3[]
         {
		      new Vector3( a, b, 0 ),			// 0
		      new Vector3( -a, b, 0 ),		// 1
		      new Vector3( a, -b, 0 ),		// 2
		      new Vector3( -a, -b, 0 ),		// 3
		      new Vector3( 0, a, b ),			// 4
		      new Vector3( 0, -a, b ),		// 5
		      new Vector3( 0, a, -b ),		// 6
		      new Vector3( 0, -a, -b ),		// 7
		      new Vector3( b, 0, a ),			// 8
		      new Vector3( b, 0, -a ),		// 9
		      new Vector3( -b, 0, a ),		// 10
		      new Vector3( -b, 0, -a )		// 11
	      };

	      int []icotriangles = new int[]
	      {
		      1, 0, 6,
		      0, 1, 4,
		      2, 3, 7,
		      3, 2, 5,
		      8, 9, 0,
		      9, 8, 2,
		      10, 11, 3,
		      11, 10, 1,
		      4, 5, 8,
		      5, 4, 10,
		      6, 7, 11,
		      7, 6, 9,
		      0, 9, 6,
		      1, 6, 11,
		      1, 10, 4,
		      0, 4, 8,
		      2, 7, 9,
		      3, 11, 7,
		      3, 5, 10,
		      2, 8, 5
	      };

	      // Allocate the index array and position array
	      int m_nNumPositions = nNumIcoPositions;
	      int nNumSplitEdges = (nNumSubdivisions>0) ? 30 : 0;
	      for( int i = 0; i < nNumSubdivisions; ++i )
	      {
		      m_nNumPositions += nNumSplitEdges;
		      nNumSplitEdges *= 4;
	      }

	      int m_nNumIndices = nNumIcoIndices * ( 1 << nNumSubdivisions * 2 );


	      //grab our verts
	      VertexTypes.Pos []m_verts = new VertexTypes.Pos[m_nNumPositions];
	      for(int i=0;i<nNumIcoPositions;i++)
	      {
            Vector3 norm = Vector3.Normalize(icopoints[i]) * size;
		      m_verts[i].x = norm.X;
            m_verts[i].y = norm.Y;
            m_verts[i].z = norm.Z;
	      }


	      //Grab our inds
	      int []m_inds = new int[m_nNumIndices];
         for (int i = 0; i < icotriangles.Length; i++)
            m_inds[i] = icotriangles[i];

	      // Subdivide the triangles
	      int ni = nNumIcoIndices;

	      int nNumIndices = nNumIcoIndices;
	      int nNumPositions = nNumIcoPositions;

         int[][] edge = new int[3][];
         for (int s = 0; s < 3; s++)
            edge[s] = new int[2];

	      for( int level = 0; level < nNumSubdivisions; ++level )
	      {
		      for( int i = 0; i < nNumIndices; i += 3 )
		      {
			      // Move corners of triangle to subdivided indices
			      m_inds[ni + 0] = m_inds[i + 0];
			      m_inds[ni + 3] = m_inds[i + 1];
			      m_inds[ni + 6] = m_inds[i + 2];

			      // Get the three edges of the current triangle
			      

               edge[2][1] = edge[0][0] = m_inds[i + 0];
			      edge[0][1] = edge[1][0] = m_inds[i + 1];
			      edge[1][1] = edge[2][0] = m_inds[i + 2];

			      for( int k = 0; k < 3; ++k )
			      {
				      bool bNew = true;
				      int offset = ni + 1 + k * 3;

				      // For each of the three edges see if
				      // a shared edge has already been subdivided by
				      // looking at the new index list for the current 
				      // subdivision pass.
				      for( int n = nNumIndices; n < ni; n += 9 )
				      {
					      if( m_inds[n + 3] == edge[k][0] && m_inds[n + 0] == edge[k][1] )
					      {
						      m_inds[offset] = m_inds[n + 1];
						      bNew = false;
					      }
					      else
						      if( m_inds[n + 6] == edge[k][0] && m_inds[n + 3] == edge[k][1] )
						      {
							      m_inds[offset] = m_inds[n + 4];
							      bNew = false;
						      }
						      else
							      if( m_inds[n + 0] == edge[k][0] && m_inds[n + 6] == edge[k][1] )
							      {
								      m_inds[offset] = m_inds[n + 7];
								      bNew = false;
							      }
				      }

				      if( bNew )
				      {
					      // Not a shared edge (yet).  Split the edge 
					      // and create a new position vector.
					      m_inds[offset] =  nNumPositions;
                     Vector3 ak = new Vector3(m_verts[edge[k][0]].x, m_verts[edge[k][0]].y, m_verts[edge[k][0]].z);
                     Vector3 bk = new Vector3(m_verts[edge[k][1]].x, m_verts[edge[k][1]].y, m_verts[edge[k][1]].z);
                     Vector3 tmp = ( ak + bk ) * .5f;

					      // Normalize new position vector
                     Vector3 norm = Vector3.Normalize(tmp) * size;

					      m_verts[nNumPositions].x =  norm.X;
                     m_verts[nNumPositions].y = norm.Y;
                     m_verts[nNumPositions].z = norm.Z;

					      nNumPositions++;
				      }
			      }

			      // Finish exterior triangles
			      m_inds[ni + 2] = m_inds[ni + 7];
			      m_inds[ni + 5] = m_inds[ni + 1];
			      m_inds[ni + 8] = m_inds[ni + 4];

			      // Replace original triangle indices
			      // with interior triangle
			      m_inds[i + 0] = m_inds[ni + 1];
			      m_inds[i + 1] = m_inds[ni + 4];
			      m_inds[i + 2] = m_inds[ni + 7];

			      ni += 9;
		      }

		      nNumIndices = ni;
	      }

	      if( nNumIndices != m_nNumIndices )return;
	      if( nNumPositions != m_nNumPositions )return;

         /*
         //fill in our color for howmany ever verts we ended up with
         for (int i = 0; i < m_nNumPositions; i++)
         {
            m_verts[i].color = color;
         }
         */

         //Calculate normals whilst we're here
            /*
            if( bFaceted )
            {
               // Fill in normals
            //	WORD n = 0;
               for( int i = 0; i < m_nNumIndices; i += 3 )
               {
                  // Use cross product to determine face normal
                  Vector3 v0 = m_verts[m_inds[i + 0]].xyz;
                  Vector3 v1 = m_verts[m_inds[i + 1]].xyz;
                  Vector3 v2 = m_verts[m_inds[i + 2]].xyz;

                  Vector3 v01 = v1 - v0;
                  Vector3 v02 = v2 - v0;
                  Vector3 vCross;

                  vCross= Vector3.Cross(v01, v02 );

                  // Normalize cross product
                  vCross.Normalize();

                  m_verts[m_inds[i + 0]].normal = vCross;;
                  m_verts[m_inds[i + 1]].normal = vCross;
                  m_verts[m_inds[i + 2]].normal = vCross;

               }
            }
            else
            {
               for(int i=0;i<m_nNumPositions;i++)
               {
             //     m_verts[i].normal = m_verts[i].xyz;
               }
            }
            */



         mNumVerts = m_nNumPositions;
         mNumInds = m_nNumIndices;


         mVB = new VertexBuffer(typeof(VertexTypes.Pos), mNumVerts, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);
         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(m_verts);
         mVB.Unlock();


         mIB = new IndexBuffer(typeof(int), mNumInds, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
         gStream = mIB.Lock(0, 0, LockFlags.None);
         gStream.Write(m_inds);
         mIB.Unlock();


      }
      ~BRenderDebugSphere()
      {
         
         destroy();
      }
      public override void render()
      {
         render(true,false);
      }
      public void render(bool wireframe,bool alpha)
      {
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         //BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos_Color.FVF_Flags;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Indices = mIB;

         if(wireframe)
            BRenderDevice.getDevice().RenderState.FillMode = FillMode.WireFrame;
        // BRenderDevice.getDevice().RenderState.CullMode = Cull.None;
         if(alpha)
         {
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         }

         BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, mNumVerts, 0, mNumInds/3);

         if(alpha)
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().RenderState.FillMode = FillMode.Solid;
     //    BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }

   }
   //----------------------------
   public class BRenderDebugOOBB : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();
      Matrix mMatrix = new Matrix();

      static VertexBuffer s_mVB = null;
      static IndexBuffer s_mIB = null;
      static IndexBuffer s_mLineListIB = null;

      public BRenderDebugOOBB(Vector3 center, Vector3 extents, Quaternion orientation, int color, bool solid)
      {
         mSolid = solid;
         mMeshType = (int)eMeshType.cMeshDebug;

         mMatrix = Matrix.Scaling(extents * 2.0f);
         mMatrix.Multiply(Matrix.RotationQuaternion(orientation));
         mMatrix.Multiply(Matrix.Translation(center));

         mColor = color;

         if (s_mVB == null)
         {
            Vector3 min = new Vector3(-0.5f, -0.5f, -0.5f);
            Vector3 max = new Vector3(0.5f, 0.5f, 0.5f);

            VertexTypes.Pos[] cubeVertices =
		      {
			      new VertexTypes.Pos(min.X,max.Y,min.Z),
			      new VertexTypes.Pos(max.X,max.Y,min.Z),
			      new VertexTypes.Pos(min.X,min.Y,min.Z),
			      new VertexTypes.Pos(max.X,min.Y,min.Z),

			      new VertexTypes.Pos(min.X,max.Y,max.Z),
			      new VertexTypes.Pos(min.X,min.Y,max.Z),   
			      new VertexTypes.Pos(max.X,max.Y,max.Z),  
			      new VertexTypes.Pos(max.X,min.Y,max.Z),    
		      };

            int[] cubeIndices =
		      {
			      0, 1, 2, 
               2, 1, 3, 

			      4, 6, 5, 
               5, 6, 7, 

			      4, 6, 0,
               0, 6, 1,

			      5, 2, 7,
               7, 2, 3,

			      1, 6, 3,
               3, 6, 7, 

			      0, 2, 4,
               4, 2, 5  
		      };

            int[] lineListIndices =
		      {
			      0, 1,
               1, 3,
               3, 2,
               2, 0,

               4, 6,
               6, 7,
               7, 5,
               5, 4,

               0, 4,
               1, 6,
               3, 7,
               2, 5
		      };

            s_mVB = new VertexBuffer(typeof(VertexTypes.Pos), cubeVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);

            GraphicsStream gStream = s_mVB.Lock(0, 0, LockFlags.None);
            gStream.Write(cubeVertices);
            s_mVB.Unlock();

            s_mIB = new IndexBuffer(typeof(int), cubeIndices.Length, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
            gStream = s_mIB.Lock(0, 0, LockFlags.None);
            gStream.Write(cubeIndices);
            s_mIB.Unlock();

            s_mLineListIB = new IndexBuffer(typeof(int), lineListIndices.Length, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
            gStream = s_mLineListIB.Lock(0, 0, LockFlags.None);
            gStream.Write(lineListIndices);
            s_mLineListIB.Unlock();
         }
      }
      ~BRenderDebugOOBB()
      {
         destroy();
      }

      public override void render()
      {
         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);


         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         //BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos_Color.FVF_Flags;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, s_mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         if (!mSolid)
            BRenderDevice.getDevice().Indices = s_mLineListIB;
         else
            BRenderDevice.getDevice().Indices = s_mIB;

         BRenderDevice.getDevice().RenderState.CullMode = Cull.None;


         Matrix compositeMatrix = Matrix.Multiply(mMatrix, worldMat);
         BRenderDevice.getDevice().SetTransform(TransformType.World, compositeMatrix);

         if (!mSolid)
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.LineList, 0, 0, 8, 0, 12);
         else
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, 8, 0, 12);

         BRenderDevice.getDevice().SetTransform(TransformType.World, worldMat);


         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
      bool mSolid = false;
   };
   //----------------------------
   public class BRenderDebugCube : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();
      Matrix mMatrix = new Matrix();

      static VertexBuffer s_mVB = null;
      static IndexBuffer s_mIB = null;
      static IndexBuffer s_mLineListIB = null;

      public BRenderDebugCube(Vector3 min, Vector3 max, int color,bool solid)
      {
         mSolid = solid;
         mMeshType = (int)eMeshType.cMeshDebug;

         Vector3 size = Vector3.Subtract(max, min);
         Vector3 center = Vector3.Multiply((min + max), 0.5f);

         mMatrix = Matrix.Scaling(size.X, size.Y, size.Z);
         mMatrix.Multiply(Matrix.Translation(center.X, center.Y, center.Z));
         mColor = color;

         if (s_mVB == null)
         {
            Vector3 unitMin = new Vector3(-0.5f, -0.5f, -0.5f);
            Vector3 unitMax = new Vector3(0.5f, 0.5f, 0.5f);
            VertexTypes.Pos[] cubeVertices =
		      {
			      new VertexTypes.Pos(unitMin.X,unitMax.Y,unitMin.Z),
			      new VertexTypes.Pos(unitMax.X,unitMax.Y,unitMin.Z),
			      new VertexTypes.Pos(unitMin.X,unitMin.Y,unitMin.Z),
			      new VertexTypes.Pos(unitMax.X,unitMin.Y,unitMin.Z),

			      new VertexTypes.Pos(unitMin.X,unitMax.Y,unitMax.Z),
			      new VertexTypes.Pos(unitMin.X,unitMin.Y,unitMax.Z),   
			      new VertexTypes.Pos(unitMax.X,unitMax.Y,unitMax.Z),  
			      new VertexTypes.Pos(unitMax.X,unitMin.Y,unitMax.Z),    
		      };

            int[] cubeIndices =
		      {
			      0, 1, 2, 
               2, 1, 3, 

			      4, 6, 5, 
               5, 6, 7, 

			      4, 6, 0,
               0, 6, 1,

			      5, 2, 7,
               7, 2, 3,

			      1, 6, 3,
               3, 6, 7, 

			      0, 2, 4,
               4, 2, 5  
		      };

            int[] lineListIndices =
		      {
			      0, 1,
               1, 3,
               3, 2,
               2, 0,

               4, 6,
               6, 7,
               7, 5,
               5, 4,

               0, 4,
               1, 6,
               3, 7,
               2, 5
		      };

            s_mVB = new VertexBuffer(typeof(VertexTypes.Pos), cubeVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);

            GraphicsStream gStream = s_mVB.Lock(0, 0, LockFlags.None);
            gStream.Write(cubeVertices);
            s_mVB.Unlock();

            s_mIB = new IndexBuffer(typeof(int), cubeIndices.Length, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
            gStream = s_mIB.Lock(0, 0, LockFlags.None);
            gStream.Write(cubeIndices);
            s_mIB.Unlock();

            s_mLineListIB = new IndexBuffer(typeof(int), lineListIndices.Length, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
            gStream = s_mLineListIB.Lock(0, 0, LockFlags.None);
            gStream.Write(lineListIndices);
            s_mLineListIB.Unlock();
         }
      }
      ~BRenderDebugCube()
      {
         destroy();
      }

      public override void render()
      {
         render(false);
      }
      public void render(bool alpha)
      {
         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         //BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos_Color.FVF_Flags;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, s_mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         if (!mSolid)
            BRenderDevice.getDevice().Indices = s_mLineListIB;
         else
            BRenderDevice.getDevice().Indices = s_mIB;

         BRenderDevice.getDevice().RenderState.CullMode = Cull.None;


         Matrix compositeMatrix = Matrix.Multiply(mMatrix, worldMat);
         BRenderDevice.getDevice().SetTransform(TransformType.World, compositeMatrix);

         if (alpha)
         {
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         }

         if (!mSolid)
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.LineList, 0, 0, 8, 0, 12);
         else
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, 8, 0, 12);

         if (alpha)
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);

         BRenderDevice.getDevice().SetTransform(TransformType.World, worldMat);

         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
      public bool mSolid = false;
   };
   //----------------------------
   public class BRenderDebugCubeHighlight : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();

      public BRenderDebugCubeHighlight(Vector3 min, Vector3 max, int color, float cornerDist)
      {
         mMeshType = (int)eMeshType.cMeshDebug;
         mColor = color;

         VertexTypes.Pos[] cubeVertices =
		   {
			   new VertexTypes.Pos(min.X,min.Y,min.Z),
			   new VertexTypes.Pos(min.X+cornerDist,min.Y,min.Z),
			   new VertexTypes.Pos(min.X,min.Y+cornerDist,min.Z),
			   new VertexTypes.Pos(min.X,min.Y,min.Z+cornerDist),

            
			   new VertexTypes.Pos(min.X,min.Y,max.Z),
			   new VertexTypes.Pos(min.X+cornerDist,min.Y,max.Z),
			   new VertexTypes.Pos(min.X,min.Y+cornerDist,max.Z),
			   new VertexTypes.Pos(min.X,min.Y,max.Z-cornerDist),


			   new VertexTypes.Pos(min.X,max.Y,min.Z),
			   new VertexTypes.Pos(min.X+cornerDist,max.Y,min.Z),
			   new VertexTypes.Pos(min.X,max.Y-cornerDist,min.Z),
			   new VertexTypes.Pos(min.X,max.Y,min.Z+cornerDist),


			   new VertexTypes.Pos(max.X,min.Y,min.Z),
			   new VertexTypes.Pos(max.X-cornerDist,min.Y,min.Z),
			   new VertexTypes.Pos(max.X,min.Y+cornerDist,min.Z),
			   new VertexTypes.Pos(max.X,min.Y,min.Z+cornerDist),


			   new VertexTypes.Pos(min.X,max.Y,max.Z),
			   new VertexTypes.Pos(min.X+cornerDist,max.Y,max.Z),
			   new VertexTypes.Pos(min.X,max.Y-cornerDist,max.Z),
			   new VertexTypes.Pos(min.X,max.Y,max.Z-cornerDist),


			   new VertexTypes.Pos(max.X,min.Y,max.Z),
			   new VertexTypes.Pos(max.X-cornerDist,min.Y,max.Z),
			   new VertexTypes.Pos(max.X,min.Y+cornerDist,max.Z),
			   new VertexTypes.Pos(max.X,min.Y,max.Z-cornerDist),


			   new VertexTypes.Pos(max.X,max.Y,min.Z),
			   new VertexTypes.Pos(max.X-cornerDist,max.Y,min.Z),
			   new VertexTypes.Pos(max.X,max.Y-cornerDist,min.Z),
			   new VertexTypes.Pos(max.X,max.Y,min.Z+cornerDist),


			   new VertexTypes.Pos(max.X,max.Y,max.Z),
			   new VertexTypes.Pos(max.X-cornerDist,max.Y,max.Z),
			   new VertexTypes.Pos(max.X,max.Y-cornerDist,max.Z),
			   new VertexTypes.Pos(max.X,max.Y,max.Z-cornerDist),
		   };

         int[] cubeIndices =
		   {
			   0, 1, 
            0, 2,
            0, 3,

			   4, 5, 
            4, 6,
            4, 7,

			   8, 9, 
            8, 10,
            8, 11,

			   12, 13, 
            12, 14,
            12, 15,

			   16, 17, 
            16, 18,
            16, 19,

			   20, 21, 
            20, 22,
            20, 23,

			   24, 25, 
            24, 26,
            24, 27,

			   28, 29, 
            28, 30,
            28, 31,
		   };


         mVB = new VertexBuffer(typeof(VertexTypes.Pos), cubeVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(cubeVertices);
         mVB.Unlock();



         mIB = new IndexBuffer(typeof(int), cubeIndices.Length, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
         gStream = mIB.Lock(0, 0, LockFlags.None);
         gStream.Write(cubeIndices);
         mIB.Unlock();
      }
      ~BRenderDebugCubeHighlight()
      {
         destroy();
      }

      public override void render()
      {
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         //BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos_Color.FVF_Flags;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Indices = mIB;

         // Set state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Always);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, false);

         // draw
         BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.LineList, 0, 0, 32, 0, 48);

         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, true);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
   };
   //----------------------------
   public class BRenderDebugDiamond : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();
      public int color
      {
         get { return mColor; }
         set { mColor = value; }
      }

      Matrix mMatrix = new Matrix();

      static VertexBuffer s_mVB = null;
      static IndexBuffer s_mIB = null;

      public BRenderDebugDiamond(float height, int color, bool solid)
      {
         mSolid = solid;
         mMeshType = (int)eMeshType.cMeshDebug;

         mMatrix = Matrix.Scaling(height, height, height);
         mColor = color;

         if (s_mVB == null)
         {
            float size = 1.0f;
            float half_size = 1.0f / 2.0f;

            VertexTypes.Pos[] cubeVertices =
		      {
			      new VertexTypes.Pos(-half_size,0.0f,-half_size),
			      new VertexTypes.Pos(-half_size,0.0f,half_size),
			      new VertexTypes.Pos(half_size,0.0f,half_size),
			      new VertexTypes.Pos(half_size,0.0f,-half_size),

			      new VertexTypes.Pos(0.0f,size,0.0f),
			      new VertexTypes.Pos(0.0f,-size,0.0f),
		      };

            int[] cubeIndices =
		      {
			      0, 4, 1,
               1, 4, 2,
               2, 4, 3,
               3, 4, 0,

			      0, 5, 1,
               1, 5, 2,
               2, 5, 3,
               3, 5, 0,
		      };


            s_mVB = new VertexBuffer(typeof(VertexTypes.Pos), cubeVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);

            GraphicsStream gStream = s_mVB.Lock(0, 0, LockFlags.None);
            gStream.Write(cubeVertices);
            s_mVB.Unlock();

            s_mIB = new IndexBuffer(typeof(int), cubeIndices.Length, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
            gStream = s_mIB.Lock(0, 0, LockFlags.None);
            gStream.Write(cubeIndices);
            s_mIB.Unlock();
         }
      }
      ~BRenderDebugDiamond()
      {
         destroy();
      }

      public override void render()
      {
         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);


         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         //BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos_Color.FVF_Flags;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, s_mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Indices = s_mIB;

         if (!mSolid)
            BRenderDevice.getDevice().RenderState.FillMode = FillMode.WireFrame;
         BRenderDevice.getDevice().RenderState.CullMode = Cull.None;


         Matrix compositeMatrix = Matrix.Multiply(mMatrix, worldMat);
         BRenderDevice.getDevice().SetTransform(TransformType.World, compositeMatrix);

            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, 6, 0, 8);

            BRenderDevice.getDevice().SetTransform(TransformType.World, worldMat);

         BRenderDevice.getDevice().RenderState.FillMode = FillMode.Solid;
         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;


         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
      bool mSolid = false;
   };
   //----------------------------
   public class BRenderDebugAxis : BRenderPrimitive
   {
      public BRenderDebugAxis(float scale)
      {
         mMeshType = (int)eMeshType.cMeshDebug;

         VertexTypes.Pos_Color[] axisVertices =
		   {
			   new VertexTypes.Pos_Color(0,0,0,       System.Drawing.Color.Red.ToArgb() ),
			   new VertexTypes.Pos_Color(scale,0,0,   System.Drawing.Color.Red.ToArgb() ),

			   new VertexTypes.Pos_Color(0,0,0,       System.Drawing.Color.Green.ToArgb() ),
			   new VertexTypes.Pos_Color(0,scale,0,   System.Drawing.Color.Green.ToArgb() ),

			   new VertexTypes.Pos_Color(0,0,0,       System.Drawing.Color.Blue.ToArgb() ),
			   new VertexTypes.Pos_Color(0,0,scale,   System.Drawing.Color.Blue.ToArgb() ),   
		   };

         mVB = new VertexBuffer(typeof(VertexTypes.Pos_Color), axisVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_Color.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(axisVertices);
         mVB.Unlock();
      }

      ~BRenderDebugAxis()
      {
         destroy();
      }

      public override void render()
      {
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_Color.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);

         // Set state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);

         BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList,0,3);

         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
      }
   }
   //----------------------------
   public class BRenderDebugCone : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();

      public BRenderDebugCone(float height,float baseRad, int color, bool topOrigin,bool solid)
      {
         mMeshType = (int)eMeshType.cMeshDebug;
         mSolid = solid;
         mColor = color;

         int cNumCircleVerts = 20;
         mNumVerts = cNumCircleVerts + 1;

         VertexTypes.Pos[] axisVertices = new VertexTypes.Pos[cNumCircleVerts + 1];

         axisVertices[0] = new VertexTypes.Pos(0, topOrigin?0:height, 0);

         float angle = 0;
         float angleInc = (float)((Math.PI * 2.0f) / (cNumCircleVerts - 1));

         for (int i = 0; i < cNumCircleVerts; i++)
         {
            float x = (float)(Math.Cos(angle) * baseRad);
            float y = (float)(Math.Sin(angle) * baseRad);
            axisVertices[i + 1] = new VertexTypes.Pos(x, topOrigin?-height:0, y);
            angle += angleInc;
         }

         mVB = new VertexBuffer(typeof(VertexTypes.Pos), axisVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);
         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(axisVertices);
         mVB.Unlock();


         axisVertices = null;
         int[] inds;
         if (mSolid)
         {
            //create our index buffer
            mNumInds = ((cNumCircleVerts - 1) * 3) * (solid ? 2 : 1);
            inds = new int[mNumInds];

            int c = 0;
            //main cone
            for (int i = 0; i < cNumCircleVerts - 1; i++)
            {
               inds[c++] = i + 1;
               inds[c++] = 0;
               inds[c++] = i + 2;
            }
            if (solid)
            {
               //solid base
               for (int i = 1; i < cNumCircleVerts - 2; i++)
               {
                  inds[c++] = i + 2;
                  inds[c++] = 1;
                  inds[c++] = i + 1;
               }
            }
         }
         else
         {
            //create our index buffer
            mNumInds = ((cNumCircleVerts - 1) * 2) + (4 * 2);
            inds = new int[mNumInds];

            int c = 0;
            //base
            for (int i = 1; i < cNumCircleVerts; i++)
            {
               inds[c++] = i;
               inds[c++] = i + 1;
            }
            //sides
            inds[c++] = 1;
            inds[c++] = 0;
            inds[c++] = (int)(cNumCircleVerts * 0.25f);
            inds[c++] = 0;
            inds[c++] = (int)(cNumCircleVerts * 0.5f);
            inds[c++] = 0;
            inds[c++] = (int)(cNumCircleVerts * 0.75f);
            inds[c++] = 0;
            
           
         }


         mIB = new IndexBuffer(typeof(int), mNumInds, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
         gStream = mIB.Lock(0, 0, LockFlags.None);
         gStream.Write(inds);
         mIB.Unlock();

         inds = null;
      }

      ~BRenderDebugCone()
      {
         destroy();
      }

      public override void render()
      {
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Indices = mIB;

         if(mSolid)
         { 
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, mNumVerts, 0, mNumInds / 3);
         }
         else
         {
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.LineList, 0, 0, mNumVerts, 0, mNumInds / 2);
         }

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
      bool mSolid = false;
   }
   //----------------------------
   public class BRenderDebugCylinder : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();

      public BRenderDebugCylinder(float height, float baseRad, int color)
      {
         mMeshType = (int)eMeshType.cMeshDebug;
         mColor = color;

         int cNumCircleVerts = 20;
         mNumVerts = cNumCircleVerts * 2;

         VertexTypes.Pos[] axisVertices = new VertexTypes.Pos[mNumVerts];

         
         float angle = 0;
         float angleInc = (float)((Math.PI * 2.0f) / (cNumCircleVerts - 1));

         //bottom ring
         int c = 0;
         for (int i = 0; i < cNumCircleVerts; i++)
         {
            float x = (float)(Math.Cos(angle) * baseRad);
            float y = (float)(Math.Sin(angle) * baseRad);
            axisVertices[c++] = new VertexTypes.Pos(x, 0, y);
            angle += angleInc;
         }
         //top ring
         for (int i = 0; i < cNumCircleVerts; i++)
         {
            float x = (float)(Math.Cos(angle) * baseRad);
            float y = (float)(Math.Sin(angle) * baseRad);
            axisVertices[c++] = new VertexTypes.Pos(x, height, y);
            angle += angleInc;
         }

         mVB = new VertexBuffer(typeof(VertexTypes.Pos), axisVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);
         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(axisVertices);
         mVB.Unlock();

         axisVertices = null;

         //create our index buffer
         mNumInds = (cNumCircleVerts - 1) * 6;     //2 tris per vertical slice
         mNumInds += (2 * ((cNumCircleVerts - 1) * 3));  //3 verts per horizontal slice
         int[] inds = new int[mNumInds];

          c = 0;
         for (int i = 0; i < cNumCircleVerts - 1; i++)
         {
            inds[c++] = i;
            inds[c++] = i + cNumCircleVerts;
            inds[c++] = i + 1;

            inds[c++] = i + 1 ;
            inds[c++] = i + cNumCircleVerts;
            inds[c++] = i + cNumCircleVerts + 1;
         }
         //bottom cap
         for (int i = 0; i < cNumCircleVerts - 2; i++)
         {
            inds[c++] = i +2;
            inds[c++] = 0;
            inds[c++] = i +1;
         }
         //top cap
         for (int i = 1; i < cNumCircleVerts - 2; i++)
         {
            inds[c++] = i + 2 + cNumCircleVerts;
            inds[c++] = 1 + cNumCircleVerts;
            inds[c++] = i + 1 + cNumCircleVerts;
         }


         mIB = new IndexBuffer(typeof(int), mNumInds, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
         gStream = mIB.Lock(0, 0, LockFlags.None);
         gStream.Write(inds);
         mIB.Unlock();

         inds = null;
      }

      ~BRenderDebugCylinder()
      {
         destroy();
      }

      public override void render()
      {
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;

         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Indices = mIB;

         BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, mNumVerts, 0, mNumInds / 3);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }

  }
   //----------------------------
   public class BRenderDebug2DQuad : BRenderPrimitive
   {
      public BRenderDebug2DQuad(Vector2 min, Vector2 max, int color, bool solid)
      {
         mMeshType = (int)eMeshType.cMeshDebug;

         VertexTypes.Pos_Color[] cubeVertices ;

         mSolid = solid;

         if (solid)
         {
            cubeVertices = new VertexTypes.Pos_Color[]
		      {
			      new VertexTypes.Pos_Color(min.X,0,min.Y,  color ),
			      new VertexTypes.Pos_Color(min.X,0,max.Y,  color ),
			      new VertexTypes.Pos_Color(max.X,0,min.Y,  color ),

               new VertexTypes.Pos_Color(max.X,0,min.Y,  color ),
               new VertexTypes.Pos_Color(min.X,0,max.Y,  color ),
               new VertexTypes.Pos_Color(max.X,0,max.Y,  color ),
		      };
         }
         else
         {
            cubeVertices = new VertexTypes.Pos_Color[]
		      {
			      new VertexTypes.Pos_Color(min.X,0,min.Y,  color ),
			      new VertexTypes.Pos_Color(min.X,0,max.Y,  color ),

			      new VertexTypes.Pos_Color(min.X,0,max.Y,  color ),
               new VertexTypes.Pos_Color(max.X,0,max.Y,  color ),

               new VertexTypes.Pos_Color(max.X,0,max.Y,  color ),
               new VertexTypes.Pos_Color(max.X,0,min.Y,  color ),

               new VertexTypes.Pos_Color(max.X,0,min.Y,  color ),
               new VertexTypes.Pos_Color(min.X,0,min.Y,  color ),
		      };
         }

         mNumVerts = cubeVertices.Length;

         mVB = new VertexBuffer(typeof(VertexTypes.Pos_Color), cubeVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_Color.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(cubeVertices);
         mVB.Unlock();

      }
      ~BRenderDebug2DQuad()
      {
         destroy();
      }

       public override void render()
       {
          render(false);
       }
      public void render(bool alpha)
      {
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;

         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_Color.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);


         if (alpha)
         {
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         }

         if (mSolid)
         {
            BRenderDevice.getDevice().RenderState.CullMode = Cull.None;
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, mNumVerts / 3);
            BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         }
         else
         {
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineList, 0, mNumVerts / 2);
         }

         if (alpha)
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         
      }
      bool mSolid = false;
   };
   //----------------------------
   public class BRenderDebug2DCircle : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();

      public BRenderDebug2DCircle(float baseRad, int color)
      {
         init(baseRad, color, false);
      }
      public BRenderDebug2DCircle(float baseRad, int color,bool solid)
      {
         init(baseRad, color, solid);
      }
      private void init(float baseRad, int color,bool solid)
      {
         mMeshType = (int)eMeshType.cMeshDebug;
         mColor = color;

         int cNumCircleVerts = 30;
         mNumVerts = cNumCircleVerts;

         VertexTypes.Pos[] axisVertices = new VertexTypes.Pos[mNumVerts];

           
         float angle = 0;
         float angleInc = (float)((Math.PI * 2.0f) / (cNumCircleVerts - 1));

         //bottom ring
         int c = 0;
         for (int i = 0; i < cNumCircleVerts; i++)
         {
            float x = (float)(Math.Cos(angle) * baseRad);
            float y = (float)(Math.Sin(angle) * baseRad);
            axisVertices[c++] = new VertexTypes.Pos(x, 0, y);
            angle += angleInc;
         }

         mVB = new VertexBuffer(typeof(VertexTypes.Pos), axisVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);
         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(axisVertices);
         mVB.Unlock();

         axisVertices = null;
         int[] inds;
        
         //create our index buffer
         mNumInds = ((cNumCircleVerts - 1) * 3) * (solid ? 2 : 1);
         inds = new int[mNumInds];

            c = 0;
           
               //solid base
               for (int i = 1; i < cNumCircleVerts - 2; i++)
               {
                  inds[c++] = i + 2;
                  inds[c++] = 1;
                  inds[c++] = i + 1;
               }
            
            mIB = new IndexBuffer(typeof(int), mNumInds, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
            gStream = mIB.Lock(0, 0, LockFlags.None);
            gStream.Write(inds);
            mIB.Unlock();

            inds = null;
      }
      ~BRenderDebug2DCircle()
      {
          destroy();
      }
      public void render()
      {
         render(false,false,0.0f, 1.0f);
      }
      public void render(bool solid, bool alpha,float start, float end)
      {
         if (end > 1.0f) end = 1.0f;
         if (start < 0.0f) start = 0.0f;

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;

         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);

         if (!solid)
         {
            int startVert = (int)(mNumVerts * start);
            int endVert = (int)(mNumVerts * end);
            int primCount = endVert - startVert;
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineStrip, startVert, primCount - 1);
         }
         else
         {
            if (alpha)
            {
               BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
               BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
               BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
            }

            BRenderDevice.getDevice().RenderState.CullMode = Cull.None;
            BRenderDevice.getDevice().Indices = mIB;
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, mNumVerts, 0, mNumInds / 3);
            BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;

            if (alpha)
               BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         }

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
   }
   //----------------------------
   public class BRenderDebug2CircleHighlight : BRenderPrimitive
   {
      int mColor = Color.White.ToArgb();
      Matrix mMatrix = new Matrix();

      static VertexBuffer s_mVB = null;
      static IndexBuffer s_mIB = null;

      static int s_totalNumVerts = 0;
      static int s_totalNumIndices = 0;

      public BRenderDebug2CircleHighlight(float baseRad, int color)
      {
         mMeshType = (int)eMeshType.cMeshDebug;

         mMatrix = Matrix.Scaling(baseRad, baseRad, baseRad);
         mColor = color;

         if (s_mVB == null)
         {
            int cNumVertsPerLine = 10;
            int cNumLines = 6;
            float cSize = 1.0f;

            s_totalNumVerts = cNumVertsPerLine * cNumLines;
            s_totalNumIndices = (cNumVertsPerLine - 1) * 2 * cNumLines;

            float angleIncrementPerLine = (float)((Math.PI * 2.0f) / (cNumLines * 2.0f));
            float angleIncrementPerVert = angleIncrementPerLine / (cNumVertsPerLine - 1);


            VertexTypes.Pos[] circleVertices = new VertexTypes.Pos[s_totalNumVerts];


            int vertIdx = 0;
            for (int lineIdx = 0; lineIdx < cNumLines; lineIdx++)
            {
               for (int vertInLineIdx = 0; vertInLineIdx < cNumVertsPerLine; vertInLineIdx++)
               {
                  float angle = (angleIncrementPerLine / 2.0f) + (angleIncrementPerLine * 2.0f * lineIdx) + (angleIncrementPerVert * vertInLineIdx);

                  float x = (float)(Math.Cos(angle) * cSize);
                  float y = (float)(Math.Sin(angle) * cSize);

                  circleVertices[vertIdx++] = new VertexTypes.Pos(x, y, 0);
               }
            }

            s_mVB = new VertexBuffer(typeof(VertexTypes.Pos), circleVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos.FVF_Flags, Pool.Managed);
            GraphicsStream gStream = s_mVB.Lock(0, 0, LockFlags.None);
            gStream.Write(circleVertices);
            s_mVB.Unlock();


            int[] inds = new int[s_totalNumIndices];

            int indexIdx = 0;
            for (int lineIdx = 0; lineIdx < cNumLines; lineIdx++)
            {
               int startIdx = lineIdx * cNumVertsPerLine;
               for (int vertLineIdx = 0; vertLineIdx < cNumVertsPerLine - 1; vertLineIdx++)
               {
                  inds[indexIdx++] = startIdx + vertLineIdx;
                  inds[indexIdx++] = startIdx + vertLineIdx + 1;
               }
            }


            s_mIB = new IndexBuffer(typeof(int), s_totalNumIndices, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
            gStream = s_mIB.Lock(0, 0, LockFlags.None);
            gStream.Write(inds);
            s_mIB.Unlock();
         }
      }

      ~BRenderDebug2CircleHighlight()
      {
         destroy();
      }

      public override void render()
      {
         Matrix worldMat = BRenderDevice.getDevice().GetTransform(TransformType.World);


         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);

         BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, mColor);


         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, s_mVB, 0);
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Indices = s_mIB;

         // Set state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Always);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, false);


         Matrix compositeMatrix = Matrix.Multiply(mMatrix, worldMat);
         BRenderDevice.getDevice().SetTransform(TransformType.World, compositeMatrix);

         // draw
         BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.LineList, 0, 0, s_totalNumVerts, 0, s_totalNumIndices);


         BRenderDevice.getDevice().SetTransform(TransformType.World, worldMat);

         // Restore state
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, true);

         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
      }
   };
   //----------------------------
   public class BRenderDebugBillboard : BRenderPrimitive
   {
      Vector3 mPosition;
      public BRenderDebugBillboard(Vector3 position,Vector2 extents, int color)
      {
         mMeshType = (int)eMeshType.cMeshDebug;

         mPosition = position;

         VertexTypes.Pos_Color_uv0[] cubeVertices;

         {
            cubeVertices = new VertexTypes.Pos_Color_uv0[]
		      {
			      new VertexTypes.Pos_Color_uv0(-extents.X,0,-extents.Y,  color ,0,0),
			      new VertexTypes.Pos_Color_uv0(-extents.X,0,extents.Y,  color ,0,1),
			      new VertexTypes.Pos_Color_uv0(extents.X,0,-extents.Y,  color ,1,0),

               new VertexTypes.Pos_Color_uv0(extents.X,0,-extents.Y,  color ,1,0),
               new VertexTypes.Pos_Color_uv0(-extents.X,0,extents.Y,  color ,0,1),
               new VertexTypes.Pos_Color_uv0(extents.X,0,extents.Y,  color ,1,1),
		      };
         }
        

         mNumVerts = cubeVertices.Length;

         mVB = new VertexBuffer(typeof(VertexTypes.Pos_Color_uv0), cubeVertices.Length, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_Color_uv0.FVF_Flags, Pool.Managed);

         GraphicsStream gStream = mVB.Lock(0, 0, LockFlags.None);
         gStream.Write(cubeVertices);
         mVB.Unlock();

      }
      ~BRenderDebugBillboard()
      {
         destroy();
      }

      public override void render()
      {
         render(BMathLib.unitY,null);
      }
      public void render(Vector3 posToLookAt,Texture tex)
      {
         Vector3 dir = mPosition - posToLookAt;
         dir.Normalize();
         Matrix world = BMathLib.makeRotateMatrix(BMathLib.unitY, dir);
         BRenderDevice.getDevice().Transform.World = world;

         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;

         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_Color_uv0.vertDecl;
         BRenderDevice.getDevice().SetStreamSource(0, mVB, 0);
         if(tex!=null)
            BRenderDevice.getDevice().SetTexture(0, tex);
         else
            BRenderDevice.getDevice().SetTexture(0, null);


        
         {
      //      BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         }

         {
            BRenderDevice.getDevice().RenderState.CullMode = Cull.None;
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, mNumVerts / 3);
            BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;
         }

         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().Transform.World = Matrix.Identity;

      }
      
   };
   
}







