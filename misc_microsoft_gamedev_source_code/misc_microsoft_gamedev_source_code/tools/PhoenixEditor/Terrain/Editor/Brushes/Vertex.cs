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

using EditorCore;
using Rendering;
//


//------------------------------------------
namespace Terrain
{
   #region Terrain Vertex Brushes


   public class VertIndexWeight
   {
      public int     index;
      public float   weight;

      public VertIndexWeight(int vertIndex, float vertWeight)
      {
         index = vertIndex;
         weight = vertWeight;
      }
   }

   public class VertUniformInfo
   {
      public Vector3 pos;
      public Vector3 normal;
      public float weight;

      public Vector3 estimatedPosX;
      public Vector3 estimatedPosZ;

      public VertUniformInfo(Vector3 vertPos, Vector3 vertNormal, float vertWeight)
      {
         pos = vertPos;
         normal = vertNormal;
         weight = vertWeight;

         estimatedPosX = pos;
         estimatedPosZ = pos;
      }
   }



   public abstract class BTerrainVertexBrush : BTerrainBrush
   {
      public BTerrainVertexBrush() { }
      ~BTerrainVertexBrush() { }

       public virtual void apply(List<VertIndexWeight> verts, float intesinty, bool alternate) { }


      // applyOnBrush
      public virtual void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate) 
      {
          //using (PerfSection p = new PerfSection("applyOnBrush"))
          {
              if (brushInfo.mRadius < 0.001) return;

              List<VertIndexWeight> verts = new List<VertIndexWeight>();

              // Loop through all points
              for (int i = 0; i < indexes.Count; i++)
              {
                 int index = indexes[i];

                 int x = index / TerrainGlobals.getTerrain().getNumZVerts();
                 int z = index % TerrainGlobals.getTerrain().getNumZVerts();

                 float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
                 if (vertSelectionWeight == 0.0f)
                    continue;

                 Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getPostDeformPos(x, z);

                 float factor = 0f;
                 if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
                    continue;

                 float noiseFactor = 1.0f;
                 if (TerrainGlobals.getTerrainFrontEnd().mbPerlin)
                 {
                    noiseFactor = TerrainGlobals.getTerrainFrontEnd().getNoiseFunction().compute(x, z);
                 }

                 verts.Add(new VertIndexWeight(index, vertSelectionWeight * factor * noiseFactor));

              }
              apply(verts, brushInfo.mIntensity, alternate);

          }
      }



      public virtual void applyOnSelection(IMask selection, float intensity, bool alternate) 
      {
          //using (PerfSection p = new PerfSection("applyOnSelection"))
          {
              if (!isApplyOnSelectionEnabled())
                  return;

              List<VertIndexWeight> verts = new List<VertIndexWeight>();

              //Dictionary<long, float>.Enumerator pointsIterator = points.GetEnumerator();
              long index;
              float weight;
              selection.ResetIterator();
              while (selection.MoveNext(out index, out weight))
              {
                  if (weight == 0.0f)
                      continue;

                  verts.Add(new VertIndexWeight((int)index, weight));
              }

              apply(verts, intensity, alternate);

          }
      }

      public virtual void applySkirt(List<VertIndexWeight> verts, float intesinty, bool alternate) { }


      // applyOnBrush
      public virtual void applySkirtOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         //using (PerfSection p = new PerfSection("applyOnBrush"))
         {
            if (brushInfo.mRadius < 0.001) return;

            List<VertIndexWeight> verts = new List<VertIndexWeight>();

            // Loop through all points
            for (int i = 0; i < indexes.Count; i++)
            {
               int index = indexes[i];

               int x = index / TerrainGlobals.getTerrain().getTotalSkirtZVerts();
               int z = index % TerrainGlobals.getTerrain().getTotalSkirtZVerts();

               Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getSkirtPos(x, z);

               float factor = 0f;
               if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
                  continue;

               verts.Add(new VertIndexWeight(index, factor));

            }
            applySkirt(verts, brushInfo.mIntensity, alternate);

         }
      }


      public void render()
      {
         if (TerrainGlobals.getEditor().mBrushShowVerts)
         {

            float ptSize = 2;
            BRenderDevice.getDevice().SetRenderState(RenderStates.PointSize, ptSize);
            {


               BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
               BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
               BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);

               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);

               List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

               float validRadius = (TerrainGlobals.getEditor().getCurrentBrushInfo().mRadius * 1.5f);   //scaled out for dramatic changes
               TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(nodes, ref TerrainGlobals.getEditor().mBrushIntersectionPoint, validRadius);
               for (int i = 0; i < nodes.Count; i++)
                  nodes[i].renderCursor();

               BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
               BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument2, (int)TextureArgument.TextureColor);

               BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, true);
            }
         }
         else
         {
            // Draw 2D radius and hotspot circle
            BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos.FVF_Flags;
            BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;
            BRenderDevice.getDevice().SetStreamSource(0, gCircleVB, 0);
            BRenderDevice.getDevice().VertexShader = null;
            BRenderDevice.getDevice().PixelShader = null;
            BRenderDevice.getDevice().SetTexture(0, null);


            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TFactor);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TFactor);
            BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.ColorOperation, (int)TextureOperation.Disable);
            BRenderDevice.getDevice().SetTextureStageState(1, TextureStageStates.AlphaOperation, (int)TextureOperation.Disable);

            BRenderDevice.getDevice().SetRenderState(RenderStates.Lighting, false);
            BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
            BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);

            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.Always);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, false);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, true);



            // Set circle color based on mode
            Vector3 brushColor;
            bool selectionMode = (UIManager.GetAsyncKeyStateB(Key.RightControl) || UIManager.GetAsyncKeyStateB(Key.LeftControl)) && (!UIManager.GetAsyncKeyStateB(Key.LeftShift) && !UIManager.GetAsyncKeyStateB(Key.RightShift));
            if (!selectionMode)
               brushColor = new Vector3(1, 1, 1);
            else
               brushColor = new Vector3(1, 1, 0);


            // Compute matrices for circles
            Matrix viewMat = BRenderDevice.getDevice().GetTransform(TransformType.View);
            viewMat.Invert();

            Matrix circleMatrix = Matrix.Translation(TerrainGlobals.getEditor().mBrushIntersectionPoint);
            circleMatrix.M11 = viewMat.M11; circleMatrix.M12 = viewMat.M12; circleMatrix.M13 = viewMat.M13;
            circleMatrix.M21 = viewMat.M21; circleMatrix.M22 = viewMat.M22; circleMatrix.M23 = viewMat.M23;
            circleMatrix.M31 = viewMat.M31; circleMatrix.M32 = viewMat.M32; circleMatrix.M33 = viewMat.M33;

            float hotSpotDist = TerrainGlobals.getEditor().getCurrentBrushInfo().mRadius * TerrainGlobals.getEditor().getCurrentBrushInfo().mHotspot;
            Matrix WorldInnerRadius = Matrix.Scaling(hotSpotDist, hotSpotDist, hotSpotDist);
            WorldInnerRadius.Multiply(circleMatrix);
            Matrix WorldOuterRadius = Matrix.Scaling(TerrainGlobals.getEditor().getCurrentBrushInfo().mRadius, TerrainGlobals.getEditor().getCurrentBrushInfo().mRadius, TerrainGlobals.getEditor().getCurrentBrushInfo().mRadius);
            WorldOuterRadius.Multiply(circleMatrix);

            // Render circles with their corresponding colors
            BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (int)BRenderDevice.D3DCOLOR_COLORVALUE(brushColor.X, brushColor.Y, brushColor.Z, 0.5f));
            BRenderDevice.getDevice().SetTransform(TransformType.World, WorldInnerRadius);
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineStrip, 0, cNumCircleVBVerts - 1);

            BRenderDevice.getDevice().SetRenderState(RenderStates.TextureFactor, (int)BRenderDevice.D3DCOLOR_COLORVALUE(brushColor.X, brushColor.Y, brushColor.Z, 1));
            BRenderDevice.getDevice().SetTransform(TransformType.World, WorldOuterRadius);
            BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.LineStrip, 0, cNumCircleVBVerts - 1);

            // Restore states
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
            BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferFunction, (int)Compare.LessEqual);
            BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, true);
            BRenderDevice.getDevice().SetRenderState(RenderStates.AntialiasedLineEnable, false);

            // Restore transform
            BRenderDevice.getDevice().SetTransform(TransformType.World, Matrix.Identity);
         }
      }
      static public void createCursor()
      {
         // Release first
         if (gCircleVB != null)
         {
            gCircleVB.Dispose();
            gCircleVB = null;
         }

         gCircleVB = new VertexBuffer(typeof(VertexTypes.Pos), cNumCircleVBVerts, BRenderDevice.getDevice(), Usage.WriteOnly, VertexTypes.Pos.FVF_Flags, Pool.Managed);

         VertexTypes.Pos[] circularVerts = new VertexTypes.Pos[cNumCircleVBVerts];

         float angle = 0;
         float angleInc = (float)((Math.PI * 2.0f) / (cNumCircleVBVerts - 1));
         float radius = TerrainGlobals.getEditor().getCurrentBrushInfo().mRadius;
         for (int i = 0; i < cNumCircleVBVerts; i++)
         {
            float x = (float)(Math.Cos(angle));
            float y = (float)(Math.Sin(angle));
            circularVerts[i].x = x;
            circularVerts[i].y = y;
            circularVerts[i].z = 0;
            
            angle += angleInc;
         }

         //copy verts over
         unsafe
         {
            using (GraphicsStream stream = gCircleVB.Lock(0, cNumCircleVBVerts * sizeof(VertexTypes.Pos), LockFlags.None))// (void**)&pVertices, 0))
            {
               stream.Write(circularVerts);
               gCircleVB.Unlock();
            }
         }
      }
      static public void destroyCursor()
      {
         if (gCircleVB != null)
         {
            gCircleVB.Dispose();
            gCircleVB = null;
         }
      }

      static VertexBuffer gCircleVB = null;
      static int cNumCircleVBVerts = 80;
   };







   //-------------------------------------------
   //-------------------------------------------
   //-------------------------------------------
   public class BTerrainHeightBrush : BTerrainVertexBrush
   {
      public BTerrainHeightBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainHeightBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 1.0f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         float factor = speedFactor * intensity;
         if (alternate)
            factor *= -1.0f;

         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            // Change the vertex.
            detail[index].Y += factor * weight;

            vertexTouchedExtends.addPoint(x, z);
         }

         // Rebuild normals
         BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                   TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                   vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }
   }


   //-------------------------------------------
   public class BTerrainPushBrush : BTerrainVertexBrush
   {
      public BTerrainPushBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainPushBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 0.4f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         //channel energy into the axis we are currently viewing into
         Vector3 applyDirection = new Vector3();
         Vector3 orig = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);

         applyDirection = orig - TerrainGlobals.getEditor().getRayPosFromMouseCoords(true);
         applyDirection = BMathLib.Normalize(applyDirection);

         Vector3 incAmt = applyDirection;

         incAmt = BMathLib.Normalize(incAmt);
         incAmt.Scale(intensity * speedFactor);

         if (alternate)
            incAmt.Scale(-1.0f);


         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            // Change the vertex.
            Vector3 newDeformation = incAmt * weight;

            // Fixed edge to not move along the X-Z plane
            if((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
               newDeformation.X = 0.0f;
            if((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
               newDeformation.Z = 0.0f;

            detail[index] += newDeformation;



            vertexTouchedExtends.addPoint(x, z);
         }

         // Rebuild normals
         BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(),TerrainGlobals.getEditor().getNormals(),
                                                   TerrainGlobals.getTerrain().getTileScale(),TerrainGlobals.getTerrain().getNumXVerts(),
                                                   vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }
   }


   //-------------------------------------------
   public class BTerrainAvgHeightBrush : BTerrainVertexBrush
   {

      public bool mUseIntersectionNormal = false;

      public BTerrainAvgHeightBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainAvgHeightBrush() { }


      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         
         float speedFactor = 0.3f;

         //channel energy into the axis we're moving upon
         Vector3 applyDirection = Vector3.Empty ;
         
         
         if(mUseIntersectionNormal)
         {
            applyDirection = TerrainGlobals.getEditor().mBrushIntersectionNormal;
         }
         else
         {
            applyDirection = new Vector3(0.0f, 1.0f, 0.0f);
         }

         Plane pl = Plane.FromPointNormal(TerrainGlobals.getEditor().mBrushIntersectionPoint, applyDirection);

         

         Vector3 incAmt = applyDirection;
         //incAmt.Scale(intensity * speedFactor);


         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();


         // Calculate average height
         float hVal = 0;
         int vertCount = 0;
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

            hVal += BMathLib.pointPlaneDistance(vertOriginalPos, pl);
            vertCount++;
         }

         hVal /= vertCount;


         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);


            // Change the height.
            float distVal = (hVal - BMathLib.pointPlaneDistance(vertOriginalPos, pl)) * weight * intensity;

            Vector3 val2 = applyDirection * distVal;

            detail[index] += val2;

            vertexTouchedExtends.addPoint(x, z);
         }

         // Rebuild normals
         BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                   TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(), 
                                                   vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }
      //public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      //{
      //   if (brushInfo.mRadius < 0.001) return;

      //   //channel energy into the axis we're moving upon
      //   Vector3 incAmt = brushApplyNormal;
        
      //   if (alternate)
      //      incAmt.Scale(-1.0f);


      //   BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();
      //   Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();


      //   //channel energy into the axis we're moving upon

      //   Plane pl = Plane.FromPointNormal(brushApplyPt, brushApplyNormal);

      //   // Calculate average distance
      //   float hVal = 0;
      //   int vertCount = 0;
      //   for (int i = 0; i < indexes.Count; i++)
      //   {
      //      int x = indexes[i] / TerrainGlobals.getTerrain().getNumZVerts();
      //      int z = indexes[i] % TerrainGlobals.getTerrain().getNumZVerts();


      //      Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

      //      float factor = 0f;
      //      if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertOriginalPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
      //         continue;

      //      float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
      //      if (vertSelectionWeight == 0.0f)
      //         continue;

      //      hVal += BMathLib.pointPlaneDistance(vertOriginalPos, pl);
      //      vertCount++;
      //   }

      //   hVal /= vertCount;




      //   // Go through points and adjust accordingly.
      //   for (int i = 0; i < indexes.Count; i++)
      //   {
      //      int index = indexes[i];

      //      int x = index / TerrainGlobals.getTerrain().getNumZVerts();
      //      int z = index % TerrainGlobals.getTerrain().getNumZVerts();


      //      Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x,z);

      //      float factor = 0f;
      //      if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertOriginalPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
      //         continue;

      //      float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
      //      if (vertSelectionWeight == 0.0f)
      //         continue;

            

      //      // Change the height.
      //      float distVal = (hVal - BMathLib.pointPlaneDistance(vertOriginalPos, pl)) *factor * brushInfo.mIntensity;

      //      Vector3 val2 = brushApplyNormal * distVal;

      //      detail[index] += val2;
           
      //   }

      //   TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
      //   TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

      //   // Rebuild normals
      //   TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      //}

   }


   //-------------------------------------------
   public class BTerrainStdBrush : BTerrainVertexBrush
   {
      public BTerrainStdBrush() 
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainStdBrush() { }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         if (brushInfo.mRadius < 0.001) return;

         //channel energy into the axis we're moving upon
         Vector3 incAmt = brushApplyNormal;
         incAmt = BMathLib.Normalize(incAmt);
         incAmt *= brushInfo.mIntensity;

         if (alternate)
            incAmt.Scale(-1.0f);


         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < indexes.Count; i++)
         {
            int index = indexes[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

            float factor = 0f;
            if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertOriginalPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
               continue;

            float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
            if (vertSelectionWeight == 0.0f)
               continue;

            if (TerrainGlobals.getTerrainFrontEnd().mbPerlin)
            {
               //perlin hack
               //float freq = 0.20f;
               //float amplitude = 0.1f;
               //factor = factor * amplitude * (float)((Perlin.FinalNoise(x * freq, z * freq)));
               factor = TerrainGlobals.getTerrainFrontEnd().getNoiseFunction().compute(x, z);
            }


            // Change the height.
            Vector3 val = incAmt * factor * vertSelectionWeight;
            //detail[index] += val;

            // Compute maximum displacement
            float max_displacement = factor * brushInfo.mRadius;


            //if (!TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(index))
            //{
            //   TerrainGlobals.getEditor().getCurrBrushDeformations()[index] = new Vector3(0, 0, 0);
            //}
            Vector3 currentDef = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);

            //float currLength = currBrushDeformation[index].Length();
            float currLength = currentDef.Length();


            if (currLength < max_displacement)
            {
               //Vector3 newDeformation = currBrushDeformation[index] + val;
               Vector3 newDeformation = currentDef + val;
               float newLength = newDeformation.Length();

               if (newLength > max_displacement)
               {
                  newDeformation = BMathLib.Normalize(newDeformation);
                  newDeformation.Scale(max_displacement);
               }

               // Fixed edge to not move along the X-Z plane
               if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
                  newDeformation.X = 0.0f;
               if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
                  newDeformation.Z = 0.0f;

               TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

               vertexTouchedExtends.addPoint(x, z);
            }
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }

      public override void apply(List<VertIndexWeight> verts, float intesinty, bool alternate)
      {

         //channel energy into the axis we're moving upon
         //Vector3 incAmt = TerrainGlobals.getEditor().getNormals() //brushApplyNormal;
         //incAmt = BMathLib.Normalize(incAmt);
         //incAmt *= intesinty;

         //if (alternate)
         //   incAmt.Scale(-1.0f);


         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         foreach (VertIndexWeight vertindex in verts)
         {
            int index = vertindex.index;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            Vector3 incAmt = TerrainGlobals.getEditor().getNormals()[index]; //brushApplyNormal;
            incAmt = BMathLib.Normalize(incAmt);
            incAmt *= intesinty;

            if (alternate)
               incAmt.Scale(-1.0f);

            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

            float factor = 1;
            float vertSelectionWeight = vertindex.weight;
            if (vertSelectionWeight == 0.0f)
               continue;

            //if (TerrainGlobals.getTerrainFrontEnd().mbPerlin)
            //{
            //   //perlin hack
            //   //float freq = 0.20f;
            //   //float amplitude = 0.1f;
            //   //factor = factor * amplitude * (float)((Perlin.FinalNoise(x * freq, z * freq)));
            //   factor = TerrainGlobals.getTerrainFrontEnd().getNoiseFunction().compute(x, z);
            //}

            // Change the height.
            Vector3 val = incAmt * factor * vertSelectionWeight;

            Vector3 currentDef = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);

            float currLength = currentDef.Length();


            //if (currLength < max_displacement)
            {
               //Vector3 newDeformation = currBrushDeformation[index] + val;
               Vector3 newDeformation = currentDef + val;
               float newLength = newDeformation.Length();

               //if (newLength > max_displacement)
               //{
               //   newDeformation = BMathLib.Normalize(newDeformation);
               //   newDeformation.Scale(max_displacement);
               //}

               // Fixed edge to not move along the X-Z plane
               if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
                  newDeformation.X = 0.0f;
               if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
                  newDeformation.Z = 0.0f;


               TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

               vertexTouchedExtends.addPoint(x, z);
            }
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);


      }



   };


   //-------------------------------------------
   public class BTerrainLayerBrush : BTerrainVertexBrush
   {
      public BTerrainLayerBrush() { }
      ~BTerrainLayerBrush() { }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         if (brushInfo.mRadius < 0.001) return;

         //channel energy into the axis we're moving upon
         Vector3 incAmt = brushApplyNormal;
         incAmt = BMathLib.Normalize(incAmt);
         incAmt *= 10.0f;

         if (alternate)
            incAmt.Scale(-1.0f);


         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < indexes.Count; i++)
         {
            int index = indexes[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

            float factor = 0f;
            if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertOriginalPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
               continue;


            float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
            if (vertSelectionWeight == 0.0f)
               continue;

            // Change the height.
            Vector3 val = incAmt * factor * vertSelectionWeight;
            //detail[index] += val;

            // Compute maximum displacement
            float max_displacement = factor * brushInfo.mRadius * brushInfo.mIntensity;

            //if (!TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(index))
            //{
            //   TerrainGlobals.getEditor().getCurrBrushDeformations()[index] = new Vector3(0, 0, 0);
            //}
            Vector3 currentDef = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);

            float currLength = currentDef.Length();

            if (currLength < max_displacement)
            {
               Vector3 newDeformation = currentDef + val;
               float newLength = newDeformation.Length();

               if (newLength > max_displacement)
               {
                  newDeformation = BMathLib.Normalize(newDeformation);
                  newDeformation.Scale(max_displacement);
               }

               // Fixed edge to not move along the X-Z plane
               if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
                  newDeformation.X = 0.0f;
               if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
                  newDeformation.Z = 0.0f;


               TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

               vertexTouchedExtends.addPoint(x, z);
            }
         }
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      
      }
   };

   //-------------------------------------------
   public class BTerrainInflateBrush : BTerrainVertexBrush
   {
      public BTerrainInflateBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainInflateBrush() { }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         if (brushInfo.mRadius < 0.001) return;


         float inflateSpeed = 0.2f;
         Vector3 incAmt = new Vector3(0.0f, 0.0f, 0.0f);

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < indexes.Count; i++)
         {
            int index = indexes[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

            float factor = 0f;
            if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertOriginalPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
               continue;


            // For inflate we must get the vertex normal at each vert
            incAmt = TerrainGlobals.getTerrain().getPostDeformNormal((int)x, (int)z);
            incAmt = BMathLib.Normalize(incAmt);
            incAmt *= brushInfo.mIntensity * inflateSpeed;
            if (alternate)
               incAmt.Scale(-1.0f);


            float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
            if (vertSelectionWeight == 0.0f)
               continue;

            // Change the height.
            Vector3 val = incAmt * factor * vertSelectionWeight;
            //detail[index] += val;


            // Get distance from this vertex to the center point.
            Vector3 diff = brushApplyPt - vertOriginalPos;
            float distance = diff.Length();

            float max_displacement = (float)Math.Sqrt(brushInfo.mRadius * brushInfo.mRadius - distance * distance);

            //if (!TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(index))
            //{
            //   TerrainGlobals.getEditor().getCurrBrushDeformations()[index] = new Vector3(0, 0, 0);
            //}
            Vector3 currentDef = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);


            float currLength = currentDef.Length();

            if (currLength < max_displacement)
            {
               Vector3 newDeformation = currentDef + val;
               float newLength = newDeformation.Length();

               if (newLength > max_displacement)
               {
                  newDeformation = BMathLib.Normalize(newDeformation);
                  newDeformation.Scale(max_displacement);
               }

               // Fixed edge to not move along the X-Z plane
               if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
                  newDeformation.X = 0.0f;
               if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
                  newDeformation.Z = 0.0f;


               TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

               vertexTouchedExtends.addPoint(x, z);
            }
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }


      public override void apply(List<VertIndexWeight> verts, float intesinty, bool alternate)       
      {
  
         float inflateSpeed = 0.2f;
         Vector3 incAmt = new Vector3(0.0f, 0.0f, 0.0f);

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         foreach(VertIndexWeight vertindex in verts)
         {
            int index = vertindex.index;// indexes[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

            float factor = 1;// 0f;

            // For inflate we must get the vertex normal at each vert
            incAmt = TerrainGlobals.getTerrain().getPostDeformNormal((int)x, (int)z);
            incAmt = BMathLib.Normalize(incAmt);
            incAmt *= intesinty * inflateSpeed;
            if (alternate)
               incAmt.Scale(-1.0f);


            float vertSelectionWeight = vertindex.weight;// vertindex.weight;
            if (vertSelectionWeight == 0.0f)
               continue;

            // Change the height.
            Vector3 val = incAmt * factor * vertSelectionWeight;

            Vector3 currentDef = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);

            float currLength = currentDef.Length();


            Vector3 newDeformation = currentDef + val;
            float newLength = newDeformation.Length();

            // Fixed edge to not move along the X-Z plane
            if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
               newDeformation.X = 0.0f;
            if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
               newDeformation.Z = 0.0f;


            TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

            vertexTouchedExtends.addPoint(x, z);
            
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);

      }


   };



   //-------------------------------------------
   public class BTerrainSetHeightBrush : BTerrainSmoothBrush
   {
      public BTerrainSetHeightBrush()
      {
         m_bApplyOnSelection = true;
         mSampledHeight = 0.0f;
      }
      ~BTerrainSetHeightBrush() { }


      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         if (alternate)
            return;

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);
            Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getPostDeformRelPos(x, z);


            float maxHeightDisp = (mSampledHeight - vertOriginalPos.Y) * weight;

            // Smooth on XZ plane
            //CLM [21.06.06] Taken out for speed
            // Vector3 smoothVertPos = applySpacialConvolutionFilter(x, z, mConvolutionSize, mConvolutionMaskLowPass);
            // Vector3 xz_diff = smoothVertPos - vertCurrentPos;


            //if (!TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(index))
            //{
            //   TerrainGlobals.getEditor().getCurrBrushDeformations()[index] = new Vector3(0, 0, 0);
            //}
            //Vector3 currDeformation = TerrainGlobals.getEditor().getCurrBrushDeformations()[index];
            Vector3 currDeformation = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);



            if (Math.Abs(currDeformation.Y) < Math.Abs(maxHeightDisp))
            {
               currDeformation.Y = maxHeightDisp;
            }

            //CLM [21.06.06] Taken out for speed
            currDeformation.X = 0;//+= (xz_diff.X * weight * intensity);
            currDeformation.Z = 0;//+= (xz_diff.Z * weight * intensity);

            TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, currDeformation);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         // If we are just sampling just store the height of the center of the brush
         if (alternate)
         {
            mSampledHeight = brushApplyPt.Y;
            return;
         }

         if (brushInfo.mRadius < 0.001) return;

         List<VertIndexWeight> verts = new List<VertIndexWeight>();

         // Loop through all points
         for (int i = 0; i < indexes.Count; i++)
         {
            int index = indexes[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
            if (vertSelectionWeight == 0.0f)
               continue;

            Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getPostDeformPos(x, z);

            float factor = 0f;
            if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, BrushInfo.eIntersectionShape.Cylinder, out factor))
               continue;

            float noiseFactor = 1.0f;
            if (TerrainGlobals.getTerrainFrontEnd().mbPerlin)
            {
               noiseFactor = TerrainGlobals.getTerrainFrontEnd().getNoiseFunction().compute(x, z);
            }

            verts.Add(new VertIndexWeight(index, vertSelectionWeight * factor * noiseFactor));
         }

         apply(verts, brushInfo.mIntensity, alternate);
      }

      private float mSampledHeight;

   };


   //-------------------------------------------
   public class BTerrainPinchBrush : BTerrainVertexBrush
   {
      public BTerrainPinchBrush() { }
      ~BTerrainPinchBrush() { }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         if (brushInfo.mRadius < 0.001) return;

         //get the closest vertex to this vertex, 
         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();
         //Vector3[] currBrushDeformation = TerrainGlobals.getEditor().getCurrBrushDeformations();

         // Scale to the desired speed
         float pinchSpeed = brushInfo.mIntensity * 0.06f;

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < indexes.Count; i++)
         {
            int index = indexes[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            Vector3 vertOriginalPos = TerrainGlobals.getTerrain().getPos(x, z);

            float factor = 0f;
            if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertOriginalPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
               continue;

            float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
            if (vertSelectionWeight == 0.0f)
               continue;

            Vector3 curVertPos = TerrainGlobals.getTerrain().getPostDeformPos((int)x, (int)z);
            Vector3 movementDir = curVertPos - brushApplyPt;
            float lengthToCenterPoint = movementDir.Length();

            float disp = factor * pinchSpeed * vertSelectionWeight;

            if (!alternate)
            {
               // Do nothing if already at center point 
               if (lengthToCenterPoint == 0)
                  continue;

               // Don't displace more than distance to center point
               if (disp >= lengthToCenterPoint)
                  disp = lengthToCenterPoint;

               movementDir = BMathLib.Normalize(movementDir);
               movementDir.Scale(-disp);
            }
            else
            {
               // Do nothing if already at rim
               if (lengthToCenterPoint >= brushInfo.mRadius)
                  continue;

               // Don't displace more than distance to center point
               if (disp >= brushInfo.mRadius - lengthToCenterPoint)
                  disp = brushInfo.mRadius - lengthToCenterPoint;

               movementDir = BMathLib.Normalize(movementDir);
               movementDir.Scale(disp);
            }


            //if (!TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(index))
            //{
            //   TerrainGlobals.getEditor().getCurrBrushDeformations()[index] = new Vector3(0, 0, 0);
            //}
            Vector3 currDeformation = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);
            Vector3 newDeformation = currDeformation + movementDir;

            // Fixed edge to not move along the X-Z plane
            if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
               newDeformation.X = 0.0f;
            if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
               newDeformation.Z = 0.0f;


            TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }

   };


   //-------------------------------------------
   public class BTerrainSmoothBrush : BTerrainVertexBrush
   {
      public BTerrainSmoothBrush() 
      {
         m_bApplyOnSelection = true;

         mConvolutionSize = 3;
         mHalfConvolutionSize = (short) (mConvolutionSize / 2);
         mConvolutionMaskLowPass = new float[mConvolutionSize * mConvolutionSize];

         int numConvolutionCoefficients = mConvolutionSize * mConvolutionSize;

         // Defined low-pass (smooth) convolution coefficients
         float invNumConvCoeff = 1.0f / numConvolutionCoefficients;
         int i, j;
         for(i = 0; i < mConvolutionSize; i++)
         {
            for(j = 0; j < mConvolutionSize; j++)
            {
               mConvolutionMaskLowPass[(i * mConvolutionSize) + j] = invNumConvCoeff;
            }
         }


         // Defined high-pass (sharpen) convolution coefficients
         /*
         mConvolutionMaskHighPass = new float[mConvolutionSize * mConvolutionSize];
         for (i = 0; i < mConvolutionSize; i++)
         {
            for(j = 0; j < mConvolutionSize; j++)
            {
               mConvolutionMaskHighPass[(i * mConvolutionSize) + j] = -1.0f;
            }
         }
         int center = mConvolutionSize / 2;
         mConvolutionMaskHighPass[(center * mConvolutionSize) + center] = numConvolutionCoefficients;
         */
      }
      ~BTerrainSmoothBrush() { }



      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();


         // Set convolution size based on intesity.
         if(intensity <= 0.5f)
         {
            mConvolutionSize = 3;
         }
         else if(intensity <= 0.75f)
         {
            mConvolutionSize = 5;
         }
         else if (intensity <= 0.9f)
         {
            mConvolutionSize = 7;
         }
         else if (intensity <= 1.0f)
         {
            mConvolutionSize = 11;
         }

         mHalfConvolutionSize = (short)(mConvolutionSize / 2);
         int numConvolutionCoefficients = mConvolutionSize * mConvolutionSize;


         Vector3 newDeformation = new Vector3();
         Vector3 curVertPos = new Vector3();
         Vector3 curDef = new Vector3();

         float newVertPos_X, newVertPos_Y, newVertPos_Z;


         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight_scaled = verts[i].weight * intensity;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            curVertPos = TerrainGlobals.getTerrain().getPostDeformRelPos((int)x, (int)z);
            curDef = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index);

            applyFastLowPassFilter((int)x, (int)z, out newVertPos_X, out newVertPos_Y, out newVertPos_Z);

            newDeformation.X = curDef.X + ((newVertPos_X - curVertPos.X) * weight_scaled);
            newDeformation.Y = curDef.Y + ((newVertPos_Y - curVertPos.Y) * weight_scaled);
            newDeformation.Z = curDef.Z + ((newVertPos_Z - curVertPos.Z) * weight_scaled);

            // Fixed edge to not move along the X-Z plane
            if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
               newDeformation.X = 0.0f;
            if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
               newDeformation.Z = 0.0f;


            TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }

      public void applySpacialConvolutionFilter(int xPos, int zPos, out float newVertRelPos_X, out float newVertRelPos_Y, out float newVertRelPos_Z)
      {
         
         int minX = xPos - mHalfConvolutionSize;
         int maxX = xPos + mHalfConvolutionSize;
         int minZ = zPos - mHalfConvolutionSize;
         int maxZ = zPos + mHalfConvolutionSize;


         // Clamp to valid values
         if (minX < 0) minX = 0;
         if (minZ < 0) minZ = 0;
         if (maxX >= TerrainGlobals.getTerrain().getNumXVerts()) maxX = TerrainGlobals.getTerrain().getNumXVerts() - 1;
         if (maxZ >= TerrainGlobals.getTerrain().getNumZVerts()) maxZ = TerrainGlobals.getTerrain().getNumZVerts() - 1;
         

         newVertRelPos_X = 0.0f;
         newVertRelPos_Y = 0.0f;
         newVertRelPos_Z = 0.0f;

         int x, z, i, j;
         for(x = minX; x <= maxX; x++)
         {
            i = (x - xPos) + mHalfConvolutionSize;
            for(z = minZ; z <= maxZ; z++)
            {
               j = (z - zPos) + mHalfConvolutionSize;
               float convWeight = mConvolutionMaskLowPass[(i * mConvolutionSize) + j];
               Vector3 samplePos = TerrainGlobals.getTerrain().getPostDeformRelPos(x, z);

               newVertRelPos_X += convWeight * samplePos.X;
               newVertRelPos_Y += convWeight * samplePos.Y;
               newVertRelPos_Z += convWeight * samplePos.Z;
            }
         }
      }

      public void applyFastLowPassFilter(int xPos, int zPos, out float newVertPos_X, out float newVertPos_Y, out float newVertPos_Z)
      {

         int minX = xPos - mHalfConvolutionSize;
         int maxX = xPos + mHalfConvolutionSize;
         int minZ = zPos - mHalfConvolutionSize;
         int maxZ = zPos + mHalfConvolutionSize;


         // Clamp to valid values
         if (minX < 0) minX = 0;
         if (minZ < 0) minZ = 0;
         if (maxX >= TerrainGlobals.getTerrain().getNumXVerts()) maxX = TerrainGlobals.getTerrain().getNumXVerts() - 1;
         if (maxZ >= TerrainGlobals.getTerrain().getNumZVerts()) maxZ = TerrainGlobals.getTerrain().getNumZVerts() - 1;


         int numCoefficients = TerrainGlobals.getTerrain().getAddedPostDeformRelPos(minX, minZ, maxX, maxZ, out newVertPos_X, out newVertPos_Y, out newVertPos_Z);

         float invNumConvCoefficients = 1.0f / numCoefficients;
         newVertPos_X *= invNumConvCoefficients;
         newVertPos_Y *= invNumConvCoefficients;
         newVertPos_Z *= invNumConvCoefficients;
      }  

      protected short      mConvolutionSize;
      protected float[]    mConvolutionMaskLowPass;
      protected short      mHalfConvolutionSize;
      //private float[]    mConvolutionMaskHighPass;

   };



   //-------------------------------------------
   public class BTerrainUniformBrush : BTerrainVertexBrush
   {
      public BTerrainUniformBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainUniformBrush() { }


      private float getWeight(Dictionary<long, VertUniformInfo> vertInfoHash, int x, int z)
      {
         int index = x * TerrainGlobals.getTerrain().getNumZVerts() + z;
         return (vertInfoHash[index].weight);
      }
      private Vector3 getPos(Dictionary<long, VertUniformInfo> vertInfoHash, int x, int z)
      {
         int index = x * TerrainGlobals.getTerrain().getNumZVerts() + z;
         return (vertInfoHash[index].pos);
      }


      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 0.08f;

         // Find extends
         //

         BTileBoundingBox tileExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            float weight = verts[i].weight;

            if (weight == 0.0f)
               continue;

            int index = verts[i].index;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            tileExtends.addPoint(x, z);
         }


         int minX = tileExtends.minX;
         int minZ = tileExtends.minZ;
         int maxX = tileExtends.maxX;
         int maxZ = tileExtends.maxZ;


         // Cache all working data into a hash table.
         //
         Dictionary<long, VertUniformInfo> workingVertsHash = new Dictionary<long, VertUniformInfo>();

         for (int x = minX; x <= maxX; x++)
         {
            for (int z = minZ; z <= maxZ; z++)
            {
               int index = (int)(x * TerrainGlobals.getTerrain().getNumZVerts() + z);

               Vector3 pos = TerrainGlobals.getTerrain().getPostDeformPos(x, z);
               Vector3 normal = TerrainGlobals.getTerrain().getPostDeformNormal(x, z);
               float weight = 0.0f;

               workingVertsHash[index] = new VertUniformInfo(pos, normal, weight);
            }
         }

         // fill weights
         for (int i = 0; i < verts.Count; i++)
         {
            workingVertsHash[verts[i].index].weight = verts[i].weight;
         }



         for (int x = minX; x <= maxX; x++)
         {
            int z = minZ;

            while (z <= maxZ)
            {
               int startAnchor = -1;
               int endAnchor = -1;

               if (z == minZ)
                  startAnchor = minZ;

               // find the first non-zero weighted vert
               while ((z <= maxZ) && getWeight(workingVertsHash, x, z) == 0.0f)
               {
                  startAnchor = z;
                  z++;
               }

               // find the first zero weighted vert
               while ((z <= maxZ) && getWeight(workingVertsHash, x, z) != 0.0f)
               {
                  endAnchor = z;
                  z++;
               }

               // use last point as end anchor if we have go through the whole row without setting one.
               if ((startAnchor != -1) && (endAnchor == -1) && (z > maxZ))
               {
                  endAnchor = maxZ;
               }

               if ((startAnchor != -1) && (endAnchor != -1) && (startAnchor != endAnchor))
               {
                  List<Vector3> curPointLine = new List<Vector3>();
                  List<Vector3> newPointLine = new List<Vector3>();

                  int xindex = x;

                  // add to list of points
                  for (int zindex = startAnchor; zindex <= endAnchor; zindex++)
                  {
                     curPointLine.Add(getPos(workingVertsHash, xindex, zindex));
                  }

                  uniformDistributionLine(ref curPointLine, ref newPointLine);

                  // Fill working hash table
                  for (int i = 0, zindex = startAnchor; i < curPointLine.Count; i++, zindex++)
                  {
                     int index = (int)(xindex * TerrainGlobals.getTerrain().getNumZVerts() + zindex);

                     VertUniformInfo vertInfo = workingVertsHash[index];
                     vertInfo.estimatedPosX = newPointLine[i];

                     workingVertsHash[index] = vertInfo;
                  }
               }
            }
         }


         for (int z = minZ; z <= maxZ; z++)
         {
            int x = minX;

            while (x <= maxX)
            {
               int startAnchor = -1;
               int endAnchor = -1;

               if (x == minX)
                  startAnchor = minX;

               // find the first non-zero weighted vert
               while ((x <= maxX) && getWeight(workingVertsHash, x, z) == 0.0f)
               {
                  startAnchor = x;
                  x++;
               }

               // find the first zero weighted vert
               while ((x <= maxX) && getWeight(workingVertsHash, x, z) != 0.0f)
               {
                  endAnchor = x;
                  x++;
               }

               // use last point as end anchor if we have go through the whole row without setting one.
               if ((startAnchor != -1) && (endAnchor == -1) && (x > maxX))
               {
                  endAnchor = maxX;
               }

               if ((startAnchor != -1) && (endAnchor != -1) && (startAnchor != endAnchor))
               {
                  List<Vector3> curPointLine = new List<Vector3>();
                  List<Vector3> newPointLine = new List<Vector3>();

                  int zindex = z;

                  // add to list of points
                  for (int xindex = startAnchor; xindex <= endAnchor; xindex++)
                  {
                     curPointLine.Add(getPos(workingVertsHash, xindex, zindex));
                  }

                  uniformDistributionLine(ref curPointLine, ref newPointLine);

                  // Fill working hash table
                  for (int i = 0, xindex = startAnchor; i < curPointLine.Count; i++, xindex++)
                  {
                     int index = (int)(xindex * TerrainGlobals.getTerrain().getNumZVerts() + zindex);

                     VertUniformInfo vertInfo = workingVertsHash[index];
                     vertInfo.estimatedPosZ = newPointLine[i];

                     workingVertsHash[index] = vertInfo;
                  }
               }
            }
         }



         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();


         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            float weight = verts[i].weight;
            int index = verts[i].index;

            if (weight == 0.0f)
               continue;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            VertUniformInfo vertInfo = workingVertsHash[index];


            Vector3 vertOffset = (vertInfo.estimatedPosX - vertInfo.pos) + (vertInfo.estimatedPosZ - vertInfo.pos);
            vertOffset.Scale(vertInfo.weight * speedFactor);


            Vector3 newPoint = vertInfo.pos + vertOffset;


            // collide with terrain to ensure that the new point does not
            // change current deformations.
            //Vector3 intpt = Vector3.Empty;
            //BTerrainQuadNode node = null;
            //Vector3 startPoint = vertInfo.normal;
            //startPoint.Scale(-1.0f);
            //startPoint.Add(newPoint);
            //if (TerrainGlobals.getTerrain().rayIntersects(ref startPoint, ref vertInfo.normal, ref intpt, ref node, false))
            //{
            //   newPoint = intpt;
            //}


            Vector3 deformation = newPoint - vertInfo.pos;

            //if (!TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(index))
            //{
            //   TerrainGlobals.getEditor().getCurrBrushDeformations()[index] = new Vector3(0, 0, 0);
            //}
            Vector3 newDeformation = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(index) + deformation;

            // Fixed edge to not move along the X-Z plane
            if ((x == 0) || (x == TerrainGlobals.getTerrain().getNumXVerts() - 1))
               newDeformation.X = 0.0f;
            if ((z == 0) || (z == TerrainGlobals.getTerrain().getNumZVerts() - 1))
               newDeformation.Z = 0.0f;


            TerrainGlobals.getEditor().getCurrBrushDeformations().SetValue(index, newDeformation);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.minX, vertexTouchedExtends.minZ);
         TerrainGlobals.getEditor().extendCurrBrushDeformation(vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

         // Rebuild normals
         TerrainGlobals.getTerrain().computeBasisCurr(vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }

      public void uniformDistributionLine(ref List<Vector3> curPointLine, ref List<Vector3> newPointLine)
      {
         // get the average segment distance
         float sumSegmentsDist = 0;
         int numSegments = 0;
         Vector3 prevPoint = curPointLine[0];

         for (int i = 1; i < curPointLine.Count; i++)
         {
            Vector3 newPoint = curPointLine[i];
            Vector3 temp = newPoint - prevPoint;

            sumSegmentsDist += temp.Length();
            numSegments++;

            prevPoint = newPoint;
         }

         float averageSegmentDist = sumSegmentsDist / numSegments;


         // move verts to new positions (into new list)
         newPointLine.Add(curPointLine[0]);
         for (int i = 1; i < curPointLine.Count - 1; i++)
         {
            Vector3 estimatedPoint;
            float estimatedDistance = i * averageSegmentDist;
            float curDistance = 0;

            // find point at estimated distance
            prevPoint = curPointLine[0];
            for (int j = 1; j < curPointLine.Count; j++)
            {
               Vector3 newPoint = curPointLine[j];
               Vector3 temp = newPoint - prevPoint;

               float curSegmentLength = temp.Length();
               curDistance += curSegmentLength;

               if (curDistance >= estimatedDistance)
               {
                  estimatedPoint = prevPoint;
                  temp.Scale((estimatedDistance - (curDistance - curSegmentLength)) / curSegmentLength);
                  estimatedPoint.Add(temp);

                  newPointLine.Add(estimatedPoint);
                  break;
               }

               prevPoint = newPoint;
            }
         }
         newPointLine.Add(curPointLine[curPointLine.Count - 1]);
      }

   }

   //-------------------------------------------
   public class BTerrainAlphaBrush : BTerrainVertexBrush
   {
      public BTerrainAlphaBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainAlphaBrush() { }


      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         
         Byte[] alphas = TerrainGlobals.getEditor().getAlphaValues();
         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            Byte bInt = (Byte)BMathLib.Clamp(weight * intensity * 255, 0, 255);

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            byte currAlpha = TerrainGlobals.getTerrain().getAlphaValue(x, z);
            if (alternate)
               alphas[index] = (Byte)BMathLib.Clamp(alphas[index] + bInt, 0, 255);
            else
               alphas[index] = (Byte)BMathLib.Clamp(alphas[index] - bInt, 0, 255);

            vertexTouchedExtends.addPoint(x, z);
         }
      }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {

         if (brushInfo.mRadius < 0.001) return;

         List<VertIndexWeight> verts = new List<VertIndexWeight>();

         // Loop through all points
         for (int i = 0; i < indexes.Count; i++)
         {
            int index = indexes[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
            if (vertSelectionWeight == 0.0f)
               continue;

            Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getPostDeformPos(x, z);

            float factor = 0f;
            if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, BrushInfo.eIntersectionShape.Cylinder, out factor))
               continue;


            verts.Add(new VertIndexWeight(index, vertSelectionWeight * factor));
         }

         apply(verts, brushInfo.mIntensity, alternate);
      }

      private float mSampledHeight;

   };

   //-------------------------------------------
   public class BTerrainScalarBrush : BTerrainVertexBrush
   {
      public bool mOnlyDoXZ = false;
      public BTerrainScalarBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainScalarBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 0.4f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         //channel energy into the axis we are currently viewing into
        
         

         float scalarAmt = intensity*0.25f;

         if (scalarAmt > 0.9f)  //Clamp output
         {
            scalarAmt = 1.0f;  
         }

         if (alternate)
            scalarAmt = -scalarAmt;
         

         Vector3[] detail = TerrainGlobals.getEditor().getDetailPoints();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();


            // Change the vertex.
            Vector3 newDeformation = detail[index];

            if (newDeformation.Y < -300 || newDeformation.Y > 300)
            {
               detail[index].Y = BMathLib.Clamp(detail[index].Y, -300, 300);
               newDeformation.Y = BMathLib.Clamp(newDeformation.Y, -300, 300);
            }

            newDeformation.X *= (scalarAmt * weight);
            newDeformation.Z *= (scalarAmt * weight);

            if (!mOnlyDoXZ)
               newDeformation.Y *= (scalarAmt * weight);
            else
               newDeformation.Y = 0;

            Vector3 diff = newDeformation - detail[index];
            float dist = diff.Length();
            if (dist < BMathLib.cTinyEpsilon)
               detail[index] = Vector3.Empty;
            else
               detail[index] += newDeformation;


            vertexTouchedExtends.addPoint(x, z);
         }

         // Rebuild normals
         BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                   TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                   vertexTouchedExtends.minX, vertexTouchedExtends.maxX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxZ);
      }
   }


   //-------------------------------------------
   //-------------------------------------------
   public class BTerrainSkirtHeightBrush : BTerrainVertexBrush
   {
      public BTerrainSkirtHeightBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainSkirtHeightBrush() { }

      public override void applySkirt(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 3.0f;

         float factor = speedFactor * intensity;
         if (alternate)
            factor *= -1.0f;

         float[] detail = TerrainGlobals.getEditor().getSkirtHeights();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            // Change the vertex.
            detail[index] += factor * weight;

            // Clamp here
            float range = 150.0f;
            detail[index] = BMathLib.Clamp(detail[index], -range, range);
         }
      }
   }

   //-------------------------------------------
   public class BTerrainTesselationBrush : BTerrainVertexBrush
   {
      public BTerrainTesselationBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainTesselationBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 0.4f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         float factor = speedFactor * intensity;
         if (alternate)
            factor *= -1.0f;

         JaggedContainer<byte> tessVals = TerrainGlobals.getEditor().getJaggedTesselation();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            // Change the vertex.
            int transIndex = x * TerrainGlobals.getTerrain().getNumXVerts() + z;

            if (mTessOverideSetting == BTerrainEditor.eTessOverrideVal.cTess_None)
               tessVals.SetValue(transIndex, (byte)BTerrainEditor.cTesselationEmptyVal);
            else 
               tessVals.SetValue(transIndex, (byte)mTessOverideSetting);

            vertexTouchedExtends.addPoint(x, z);
         }

      }
      //SET THIS WHEN THE BRUSH STROKE OCCURS!
      public BTerrainEditor.eTessOverrideVal mTessOverideSetting = BTerrainEditor.eTessOverrideVal.cTess_None;
   }

   #endregion
}








