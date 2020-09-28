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
   public abstract class BTerrainCameraBrush : BTerrainBrush
   {
      public BTerrainCameraBrush() { }
      ~BTerrainCameraBrush() { }

      public virtual void apply(List<VertIndexWeight> verts, float intesinty, bool alternate) { }


      // applyOnBrush
      public virtual void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         
         {
            if (brushInfo.mRadius < 0.001) return;

            List<VertIndexWeight> verts = new List<VertIndexWeight>();

            // Loop through all points
            for (int i = 0; i < indexes.Count; i++)
            {
               int index = indexes[i];

               int z = (int)(index / TerrainGlobals.getEditor().getCameraRep().getNumXPoints());
               int x = (int)(index % TerrainGlobals.getEditor().getCameraRep().getNumZPoints());

               float vertSelectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight(z, x);
               if (vertSelectionWeight == 0.0f)
                  continue;

               Vector3 vertCurrentPos = TerrainGlobals.getEditor().getCameraRep().getWorldspacePoint(x,z).toVec3();

               float factor = 0f;
               if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
                  continue;


               verts.Add(new VertIndexWeight(index, vertSelectionWeight * factor));

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
         ////using (PerfSection p = new PerfSection("applyOnBrush"))
         //{
         //   if (brushInfo.mRadius < 0.001) return;

         //   List<VertIndexWeight> verts = new List<VertIndexWeight>();

         //   // Loop through all points
         //   for (int i = 0; i < indexes.Count; i++)
         //   {
         //      int index = indexes[i];

         //      int x = index / TerrainGlobals.getTerrain().getTotalSkirtZVerts();
         //      int z = index % TerrainGlobals.getTerrain().getTotalSkirtZVerts();

         //      Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getSkirtPos(x, z);

         //      float factor = 0f;
         //      if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
         //         continue;

         //      verts.Add(new VertIndexWeight(index, factor));

         //   }
         //   applySkirt(verts, brushInfo.mIntensity, alternate);

         //}
      }


      public void render()
      {
         // if (TerrainGlobals.getEditor().mBrushShowVerts)
         //{

         //   float ptSize = 2;
         //   BRenderDevice.getDevice().SetRenderState(RenderStates.PointSize, ptSize);
         //   {


         //      BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         //      BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         //      BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);

         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);

         //      List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();

         //      float validRadius = (TerrainGlobals.getEditor().getCurrentBrushInfo().mRadius * 1.5f);   //scaled out for dramatic changes
         //      TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(nodes, ref TerrainGlobals.getEditor().mBrushIntersectionPoint, validRadius);
         //      for (int i = 0; i < nodes.Count; i++)
         //         nodes[i].renderCursor();

         //      BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.Modulate);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.Diffuse);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument2, (int)TextureArgument.TextureColor);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.Modulate);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.Diffuse);
         //      BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument2, (int)TextureArgument.TextureColor);

         //      BRenderDevice.getDevice().SetRenderState(RenderStates.ZEnable, true);
         //   }
         //}
         //else
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
            Vector3 brushColor= new Vector3(1, 1, 1);
            
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



   public class BTerrainCameraHeightBrush : BTerrainCameraBrush
   {
      public BTerrainCameraHeightBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainCameraHeightBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 0.4f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         float factor = speedFactor * intensity;
         if (alternate)
            factor *= -1.0f;

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;
            
            int z = (int)(index / TerrainGlobals.getEditor().getCameraRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getCameraRep().getNumZPoints());

            // Change the vertex.
            float overideHeight = TerrainGlobals.getEditor().getCameraRep().getJaggedHeight(x,z);
            if (overideHeight != CameraHeightRep.cJaggedEmptyValue)
            {
               overideHeight += factor * weight;
            }
            else
            {
               overideHeight = TerrainGlobals.getEditor().getCameraRep().getHeight(x, z) + (factor * weight);
            }

            TerrainGlobals.getEditor().getCameraRep().setJaggedHeight(x, z, overideHeight);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().getCameraRep().updateAfterPainted(vertexTouchedExtends.minX,vertexTouchedExtends.minZ,vertexTouchedExtends.maxX,vertexTouchedExtends.maxZ);
      }
   }

   public class BTerrainCameraSetHeightBrush : BTerrainCameraBrush
   {
      private float mSampledHeight;
      public BTerrainCameraSetHeightBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainCameraSetHeightBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 0.4f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         float factor = speedFactor * intensity;
         if (alternate)
            factor *= -1.0f;

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getCameraRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getCameraRep().getNumZPoints());

            // Change the vertex.
            //float overideHeight = TerrainGlobals.getEditor().getCameraRep().getJaggedHeight(x, z);
            //if (overideHeight != CameraHeightRep.cJaggedEmptyValue)
            //{
            //   overideHeight += factor * weight;
            //}
            //else
            //{
            //   overideHeight = TerrainGlobals.getEditor().getCameraRep().getHeight(x, z) + (factor * weight);
            //}

            TerrainGlobals.getEditor().getCameraRep().setJaggedHeight(x, z, mSampledHeight);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().getCameraRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         // If we are just sampling just store the height of the center of the brush
         if (alternate)
         {
            mSampledHeight = brushApplyPt.Y;
            return;
         }

         base.applyOnBrush(indexes,ref brushApplyPt, ref brushApplyNormal, ref brushInfo,  alternate);
      }
   }

   public class BTerrainCameraSmoothBrush : BTerrainCameraBrush
   {
      public BTerrainCameraSmoothBrush()
      {
         m_bApplyOnSelection = true;

         mConvolutionSize = 3;
         mHalfConvolutionSize = (short)(mConvolutionSize / 2);
         mConvolutionMaskLowPass = new float[mConvolutionSize * mConvolutionSize];

         int numConvolutionCoefficients = mConvolutionSize * mConvolutionSize;

         // Defined low-pass (smooth) convolution coefficients
         float invNumConvCoeff = 1.0f / numConvolutionCoefficients;
         int i, j;
         for (i = 0; i < mConvolutionSize; i++)
         {
            for (j = 0; j < mConvolutionSize; j++)
            {
               mConvolutionMaskLowPass[(i * mConvolutionSize) + j] = invNumConvCoeff;
            }
         }
      }
      ~BTerrainCameraSmoothBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         // Set convolution size based on intesity.
         if (intensity <= 0.5f)
         {
            mConvolutionSize = 3;
         }
         else if (intensity <= 0.75f)
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
         float newVertPos_Y = 0;

            float speedFactor = 0.4f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         float factor = speedFactor * intensity;
         if (alternate)
            factor *= -1.0f;

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;
            float weight_scaled = verts[i].weight * intensity;
            
            int z = (int)(index / TerrainGlobals.getEditor().getCameraRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getCameraRep().getNumZPoints());

            applyFastLowPassFilter((int)x, (int)z, out newVertPos_Y);

            float curVertPos = TerrainGlobals.getEditor().getCameraRep().getHeight(x,z);
            float curDef = TerrainGlobals.getEditor().getCameraRep().getCompositeHeight(x,z);


            float newDeformation = curDef + ((newVertPos_Y - curVertPos) * weight_scaled);

            TerrainGlobals.getEditor().getCameraRep().setJaggedHeight(x, z, newVertPos_Y);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().getCameraRep().updateAfterPainted(vertexTouchedExtends.minX,vertexTouchedExtends.minZ,vertexTouchedExtends.maxX,vertexTouchedExtends.maxZ);
      }
       

      
      public void applyFastLowPassFilter(int xPos, int zPos, out float newVertPos_Y)
      {

         int minX = xPos - mHalfConvolutionSize;
         int maxX = xPos + mHalfConvolutionSize;
         int minZ = zPos - mHalfConvolutionSize;
         int maxZ = zPos + mHalfConvolutionSize;


         // Clamp to valid values
         if (minX < 0) minX = 0;
         if (minZ < 0) minZ = 0;
         if (maxX >= TerrainGlobals.getEditor().getCameraRep().getNumXPoints()) maxX = (int)(TerrainGlobals.getEditor().getCameraRep().getNumXPoints() - 1);
         if (maxZ >= TerrainGlobals.getEditor().getCameraRep().getNumZPoints()) maxZ = (int)(TerrainGlobals.getEditor().getCameraRep().getNumZPoints() - 1);

         newVertPos_Y = 0.0f;

         float vec = 0;
         int numCoefficents = 0;
         int x, y;
         for (x = minX; x <= maxX; x++)
         {
            for (y = minZ; y <= maxZ; y++)
            {
               newVertPos_Y += TerrainGlobals.getEditor().getCameraRep().getCompositeHeight(x, y);
               numCoefficents++;
            }
         }

       //  int numCoefficients = TerrainGlobals.getTerrain().getAddedPostDeformRelPos(minX, minZ, maxX, maxZ, out newVertPos_Y);

         float invNumConvCoefficients = 1.0f / numCoefficents;
         newVertPos_Y *= invNumConvCoefficients;
      }
      protected short mConvolutionSize;
      protected float[] mConvolutionMaskLowPass;
      protected short mHalfConvolutionSize;
   }

   public class BTerrainCameraEraseOverrideBrush : BTerrainCameraBrush
   {
      public BTerrainCameraEraseOverrideBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BTerrainCameraEraseOverrideBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         float speedFactor = 0.4f;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         float factor = speedFactor * intensity;
         if (alternate)
            factor *= -1.0f;

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getCameraRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getCameraRep().getNumZPoints());

            TerrainGlobals.getEditor().getCameraRep().setJaggedHeight(x, z, CameraHeightRep.cJaggedEmptyValue);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().getCameraRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }
   }
}