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
using SimEditor;
//

//------------------------------------------
namespace Terrain
{
   #region Terrain Sim Brushes

   public abstract class BTerrainSimBrush : BTerrainBrush
   {
      public BTerrainSimBrush() { }
      ~BTerrainSimBrush() { }

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

               int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
               int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());

               if (z > TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() ||
                  x > TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints())
                  continue;
               float vertSelectionWeight = 1.0f;// TerrainGlobals.getTerrain().getSoftSelectionWeight(x, z);
               //if (vertSelectionWeight == 0.0f)
               //   continue;

               //Vector3 vertCurrentPos = TerrainGlobals.getTerrain().getPostDeformPos(x, z);
               //Vector3 vertCurrentPos = TerrainGlobals.getEditor().getSimRep()getOverridePos(x, z);
               Vector3 vertCurrentPos = TerrainGlobals.getEditor().getSimRep().getHeightRep().getWorldspacePoint(x, z).toVec3();

               float factor = 0f;
               if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
                  continue;


               verts.Add(new VertIndexWeight(index, vertSelectionWeight * factor));

            }

            if(verts.Count >0)
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
      }


      public void render()
      {
       
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
   public class BSimTileTypeBrush : BTerrainSimBrush
   {
      public BSimTileTypeBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BSimTileTypeBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());

            SimTileData.eTileTypeOverrideVal passV = TerrainGlobals.getEditor().getSimRep().getDataTiles().getTileTypeBrushState();
            if (passV == SimTileData.eTileTypeOverrideVal.cTileType_None || alternate)
            {
               TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedTileType(x - 1, z - 1, 0);
            }
            else
            {
               {
                  //find the override index, and set it to the type.
                  int idx = SimTerrainType.getTileTypeIndexByName(TerrainGlobals.getEditor().getSimRep().getDataTiles().getTileTypeOverrideSelection());
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedTileType(x - 1, z - 1, idx);
               }
            }
            vertexTouchedExtends.addPoint(x, z);
         }
         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }

      public override void applyOnSelection(IMask selection, float intensity, bool alternate)
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

               //mask indicies are in vis-vert space
               int x = (int)(index / TerrainGlobals.getTerrain().getNumZVerts());
               int z = (int)(index % TerrainGlobals.getTerrain().getNumZVerts());

               //convert to local vert space (scale values here to match up with what "apply" expects)
               x = (int)Math.Ceiling(x * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;
               z = (int)Math.Ceiling(z * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;

               if (x < 0 || z < 0)
                  continue;

               index = x + TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() * z;
               verts.Add(new VertIndexWeight((int)index, weight));
            }

            apply(verts, intensity, alternate);

         }
      }
   }

   //-------------------------------------------
   public class BSimScarabPassabilityBrush : BTerrainSimBrush
   {
      public BSimScarabPassabilityBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BSimScarabPassabilityBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());

            SimTileData.eScarabPassOverrideVal passV = TerrainGlobals.getEditor().getSimRep().getDataTiles().getScarabPassableBrushState();
            if (passV == SimTileData.eScarabPassOverrideVal.cScrbPss_None)
            {
               TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedFloodPassable(x - 1, z - 1, 0);
            }
            else
            {
               if (passV == SimTileData.eScarabPassOverrideVal.cScrbPss_UnScarabPassable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedScarabPassable(x - 1, z - 1, alternate ? 1 : -1);
               else if (passV == SimTileData.eScarabPassOverrideVal.cScrbPss_ScarabPassable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedScarabPassable(x - 1, z - 1, alternate ? -1 : 1);
            }
            vertexTouchedExtends.addPoint(x, z);
         }
         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }

      public override void applyOnSelection(IMask selection, float intensity, bool alternate)
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

               //mask indicies are in vis-vert space
               int x = (int)(index / TerrainGlobals.getTerrain().getNumZVerts());
               int z = (int)(index % TerrainGlobals.getTerrain().getNumZVerts());

               //convert to local vert space (scale values here to match up with what "apply" expects)
               x = (int)Math.Ceiling(x * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;
               z = (int)Math.Ceiling(z * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;

               if (x < 0 || z < 0)
                  continue;

               index = x + TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() * z;
               verts.Add(new VertIndexWeight((int)index, weight));
            }

            apply(verts, intensity, alternate);

         }
      }
   }

   //-------------------------------------------
   public class BSimFloodPassabilityBrush : BTerrainSimBrush
   {
      public BSimFloodPassabilityBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BSimFloodPassabilityBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());

            SimTileData.eFloodPassOverrideVal passV = TerrainGlobals.getEditor().getSimRep().getDataTiles().getFloodPassableBrushState();
            if (passV == SimTileData.eFloodPassOverrideVal.cFldPss_None)
            {
               TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedFloodPassable(x - 1, z - 1, 0);
            }
            else
            {
               if (passV == SimTileData.eFloodPassOverrideVal.cFldPss_UnFloodPassable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedFloodPassable(x - 1, z - 1, alternate ? 1 : -1);
               else if (passV == SimTileData.eFloodPassOverrideVal.cFldPss_FloodPassable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedFloodPassable(x - 1, z - 1, alternate ? -1 : 1);
            }
            vertexTouchedExtends.addPoint(x, z);
         }
         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }

      public override void applyOnSelection(IMask selection, float intensity, bool alternate)
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

               //mask indicies are in vis-vert space
               int x = (int)(index / TerrainGlobals.getTerrain().getNumZVerts());
               int z = (int)(index % TerrainGlobals.getTerrain().getNumZVerts());

               //convert to local vert space (scale values here to match up with what "apply" expects)
               x = (int)Math.Ceiling(x * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;
               z = (int)Math.Ceiling(z * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;

               if (x < 0 || z < 0)
                  continue;

               index = x + TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() * z;
               verts.Add(new VertIndexWeight((int)index, weight));
            }

            apply(verts, intensity, alternate);

         }
      }
   }
   //-------------------------------------------
   public class BSimBuildabilityBrush : BTerrainSimBrush
   {
      public BSimBuildabilityBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BSimBuildabilityBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());

            SimTileData.eBuildOverrideVal passV = TerrainGlobals.getEditor().getSimRep().getDataTiles().getBuildibleBrushState();
            if (passV == SimTileData.eBuildOverrideVal.cBuild_None)
            {
               TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedBuildable(x - 1, z - 1, 0);
            }
            else
            {
               if (passV == SimTileData.eBuildOverrideVal.cBuild_Unbuildable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedBuildable(x - 1, z - 1, alternate ? 1 : -1);
               else if (passV == SimTileData.eBuildOverrideVal.cBuild_Buildable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedBuildable(x - 1, z - 1, alternate ? -1 : 1);
            }
            vertexTouchedExtends.addPoint(x, z);
         }
         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }

      public override void applyOnSelection(IMask selection, float intensity, bool alternate)
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

               //mask indicies are in vis-vert space
               int x = (int)(index / TerrainGlobals.getTerrain().getNumZVerts());
               int z = (int)(index % TerrainGlobals.getTerrain().getNumZVerts());

               //convert to local vert space (scale values here to match up with what "apply" expects)
               x = (int)Math.Ceiling(x * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;
               z = (int)Math.Ceiling(z * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;

               if (x < 0 || z < 0)
                  continue;

               index = x + TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() * z;
               verts.Add(new VertIndexWeight((int)index, weight));
            }

            apply(verts, intensity, alternate);

         }
      }
   }
   //-------------------------------------------
   public class BSimPassibilityBrush : BTerrainSimBrush
   {
      public BSimPassibilityBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BSimPassibilityBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints()) - 1;
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints()) - 1;

            //CLM the -1's here are to convert from simrep verts to simrep tiles
            SimTileData.ePassOverrideVal passV = TerrainGlobals.getEditor().getSimRep().getDataTiles().getPassableBrushState();
            if (passV == SimTileData.ePassOverrideVal.cPass_None)
            {
               TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedPassable(x , z , 0);
            }
            else
            {
               if (passV == SimTileData.ePassOverrideVal.cPass_Unpassable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedPassable(x , z , alternate ? 1 : -1);
               else if (passV == SimTileData.ePassOverrideVal.cPass_Passable)
                  TerrainGlobals.getEditor().getSimRep().getDataTiles().setJaggedPassable(x , z , alternate ? -1 : 1);
            }
            vertexTouchedExtends.addPoint(x, z);
         }
         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }

      public override void applyOnSelection(IMask selection, float intensity, bool alternate)
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

               //mask indicies are in vis-vert space 
               int x = (int)(index / TerrainGlobals.getTerrain().getNumZVerts());
               int z = (int)(index % TerrainGlobals.getTerrain().getNumZVerts());

               //convert to local vert space (scale values here to match up with what "apply" expects)
               x = (int)Math.Ceiling(x * TerrainGlobals.getEditor().getSimRep().getVisToSimScale())+1;
               z = (int)Math.Ceiling(z * TerrainGlobals.getEditor().getSimRep().getVisToSimScale()) + 1;

               if (x < 0 || z < 0)
                  continue;

               index = x + TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() * z;
               verts.Add(new VertIndexWeight((int)index, weight));
            }

            apply(verts, intensity, alternate);

         }
      }
   }
   //-------------------------------------------
   public class BSimHeightBrush : BTerrainSimBrush
   {
      public BSimHeightBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BSimHeightBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

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

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());


            // Change the vertex.
            float overideHeight = TerrainGlobals.getEditor().getSimRep().getHeightRep().getJaggedHeight(x,z);
            if (overideHeight != SimHeightRep.cJaggedEmptyValue)
            {
               overideHeight += factor * weight;
            }
            else
            {
               overideHeight = TerrainGlobals.getEditor().getSimRep().getHeightRep().getHeight(x,z) + (factor * weight);
            }

            TerrainGlobals.getEditor().getSimRep().getHeightRep().setJaggedHeight(x, z, overideHeight);
         
            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);
      }
   }
   //-------------------------------------------
   public class BSimSetHeightBrush : BTerrainSimBrush
   {
      public BSimSetHeightBrush()
      {
         m_bApplyOnSelection = true;
         mSampledHeight = 0.0f;
      }
      ~BSimSetHeightBrush() { }


      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;
         BTileBoundingBox vertexTouchedExtends = new BTileBoundingBox();

         if (alternate)
            return;
         

         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight;

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());

            
            //float currHeight = TerrainGlobals.getTerrain().getPos(z, x).Y;
            //float suggHeight = mSampledHeight * weight;

            //float targetHeight = (Math.Abs(mSampledHeight - suggHeight) > Math.Abs(mSampledHeight - currHeight)) ? currHeight : suggHeight;

          
            //// Change the vertex.
            //float overideHeight = heights.GetValue(transIndex);
            //if (overideHeight != BTerrainSimRep.cJaggedEmptyValue)
            //{
            //   targetHeight = (Math.Abs(mSampledHeight - overideHeight) > Math.Abs(mSampledHeight - targetHeight)) ? targetHeight : overideHeight;
            //}


            TerrainGlobals.getEditor().getSimRep().getHeightRep().setJaggedHeight(x,z, mSampledHeight);

            vertexTouchedExtends.addPoint(x, z);
         }


         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

      }

      public override void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
         // If we are just sampling just store the height of the center of the brush
         if (alternate)
         {
            mSampledHeight = brushApplyPt.Y;
            return;
         }

        base.applyOnBrush(indexes, ref brushApplyPt, ref brushApplyNormal, ref brushInfo,  alternate);
      }

      private float mSampledHeight;

   };
   //-------------------------------------------
   public class BSimSmoothBrush : BTerrainSimBrush
   {
      public BSimSmoothBrush()
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
      ~BSimSmoothBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

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

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());


            applyFastLowPassFilter((int)x, (int)z, out newVertPos_Y);

            TerrainGlobals.getEditor().getSimRep().getHeightRep().setJaggedHeight(x, z, newVertPos_Y);

            vertexTouchedExtends.addPoint(x, z);
         }

         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

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
         if (maxX >= TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints()) maxX = (int)(TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() - 1);
         if (maxZ >= TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints()) maxZ = (int)(TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints() - 1);

         newVertPos_Y = 0.0f;

         float vec = 0;
         int numCoefficents = 0;
         int x, y;
         for (x = minX; x <= maxX; x++)
         {
            for (y = minZ; y <= maxZ; y++)
            {
               float yJ = TerrainGlobals.getEditor().getSimRep().getHeightRep().getJaggedHeight(x, y);
               if(yJ == SimHeightRep.cJaggedEmptyValue)
                  yJ = TerrainGlobals.getEditor().getSimRep().getHeightRep().getHeight(x, y);
               newVertPos_Y += yJ;
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
   //-------------------------------------------
   public class BSimHeightEraseBrush : BTerrainSimBrush
   {
      public BSimHeightEraseBrush()
      {
         m_bApplyOnSelection = true;
      }
      ~BSimHeightEraseBrush() { }

      public override void apply(List<VertIndexWeight> verts, float intensity, bool alternate)
      {
         if (verts.Count == 0)
            return;

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

            int z = (int)(index / TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumXPoints());
            int x = (int)(index % TerrainGlobals.getEditor().getSimRep().getHeightRep().getNumZPoints());

            // Change the vertex.
            TerrainGlobals.getEditor().getSimRep().getHeightRep().setJaggedHeight(x, z, SimHeightRep.cJaggedEmptyValue);

            vertexTouchedExtends.addPoint(x, z);
         }


         TerrainGlobals.getEditor().getSimRep().updateAfterPainted(vertexTouchedExtends.minX, vertexTouchedExtends.minZ, vertexTouchedExtends.maxX, vertexTouchedExtends.maxZ);

      }
   }

  


   #endregion
}