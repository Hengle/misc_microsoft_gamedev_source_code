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


//------------------------------------------
namespace Terrain
{
   #region Terrain Foliage Brushes
   public class BTerrainFoliageBrush : BTerrainBrush
   {
      public BTerrainFoliageBrush() { m_bApplyOnSelection = true; }
      ~BTerrainFoliageBrush() { }

      Random rand = new Random();
      public void apply(List<VertIndexWeight> verts, float intesinty, bool alternate)
      {
         //CLM DEBUGGING
       //  return;

         
      //   FoliageManager.setSelectedBladeToGrid(0, 0, alternate , false);
      //   FoliageManager.setSelectedBladeToGrid(16, 16, alternate, false);
      //   FoliageManager.setSelectedBladeToGrid(20, 20, alternate, false);
      //   return;


         // Go through points and adjust accordingly.
         for (int i = 0; i < verts.Count; i++)
         {
            int index = verts[i].index;
            float weight = verts[i].weight*0.25f;

            double rnd = rand.NextDouble();
            //use our 'weight' as a randomization factor..
            if (rnd > (double)weight)
               continue;

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            FoliageManager.setSelectedBladeToGrid(x, z, alternate, 
                                                   TerrainGlobals.getEditor().getMode() == BTerrainEditor.eEditorMode.cModeFoliageErase);
         }
      }


      // applyOnBrush
      public void applyOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
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
               //Vector3 vertCurrentPos = TerrainGlobals.getEditor().getSimRep().getOverridePos(x, z);
               //Vector3 vertCurrentPos = TerrainGlobals.getEditor().getSimRep().getHeightRep().getWorldspacePoint(x, z).toVec3();

               float factor = 0f;
               if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertCurrentPos, ref brushApplyPt, brushInfo.mRadius, brushInfo.mHotspot * brushInfo.mRadius, brushInfo.mCurveType, brushInfo.mIntersectionShape, out factor))
                  continue;


               verts.Add(new VertIndexWeight(index, vertSelectionWeight * factor));

            }
            apply(verts, brushInfo.mIntensity, alternate);

         }
      }



      public void applyOnSelection(IMask selection, float intensity, bool alternate)
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

      public void applySkirt(List<VertIndexWeight> verts, float intesinty, bool alternate) { }


      // applyOnBrush
      public void applySkirtOnBrush(List<int> indexes, ref Vector3 brushApplyPt, ref Vector3 brushApplyNormal, ref BrushInfo brushInfo, bool alternate)
      {
      }


      public void render()
      {
         // if (TerrainGlobals.getEditor().mBrushShowVerts)
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

   #endregion
}