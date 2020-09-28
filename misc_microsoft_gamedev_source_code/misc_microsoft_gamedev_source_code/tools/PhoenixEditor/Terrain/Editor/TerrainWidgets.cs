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
   public class TerrainWidget
   {
      public virtual void init()
      {
      }
      public virtual void destroy()
      {
      }
      public virtual void recreateVisuals()
      {
      }
      public virtual void render()
      {
      }
   }

   public class TerrainAlignedQuad : TerrainWidget
   {
      int mCenterX;
      int mCenterZ;
      int mWidth;
      int mHeight;
      Color mColor = Color.Black;

      public TerrainAlignedQuad(int centerx, int centerz, int width, int height, Color col)
      {
         mCenterX = centerx;
         mCenterZ = centerz;
         mWidth = width;
         mHeight = height;
         mColor = col;
      }
      ~TerrainAlignedQuad()
      {
         destroy();
      }
      public override void  init()
      {
 	       base.init();

      }
      public override void destroy()
      {
         base.destroy();
      }
      public override void recreateVisuals()
      {
         base.recreateVisuals();
    
      }
      private void setShaderParams()
      {
         //specific to cursor rendering
         Terrain.BrushInfo bi = TerrainGlobals.getEditor().getCurrentBrushInfo();
         Vector4 brushInfo = new Vector4(bi.mRadius,
                                         (bi.mHotspot * bi.mRadius),
                                          0, 0);

        

         Vector4 selectionColor = new Vector4(1, 1, 0, 1);



         brushInfo.Y = TerrainGlobals.getEditor().mBrushInfo.mIntensity;
         brushInfo.Z = TerrainGlobals.getEditor().mBrushInfo.mRotation;
         brushInfo.W = 1;


        // TerrainGlobals.getRender().mTerrainGPUWidgetShader.mShader.SetValue(mWidgetColorHandle, mCursorColorTint);



         // mTerrainGPUWidgetShader.mShader.SetValue(mWidgetShaderBrushHandle, brushInfo);
         // mTerrainGPUWidgetShader.mShader.SetValue(mWidgetShaderInterPointHandle, shaderIntPt);
      }
      public override void render()
      {
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);

         int xRad = mWidth >> 1;
         int zRad = mHeight >> 1;

         

         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getVertBoundsIntersection(nodes, mCenterX - xRad, mCenterX + xRad, mCenterZ - zRad, mCenterZ + zRad);
         for (int i = 0; i < nodes.Count; i++)
            nodes[i].renderWidget(0);


         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.CounterClockwise);

         base.render();
      }
   }
   public class TerrainAlignedLine : TerrainWidget
   {
      TerrainAlignedLine()
      {
      }
      ~TerrainAlignedLine()
      {
      }
      public override void init()
      {
         base.init();
      }
      public override void destroy()
      {
         base.destroy();
      }
      public override void recreateVisuals()
      {
         base.recreateVisuals();

      }
      public override void render()
      {

         base.render();
      }
   }
}
/*
 *         private void drawTextureModeCursor()
{
  if (cNumTexCursorVerts > 0)
  {
     // Set circle color based on mode
              
     bool selectionMode = UIManager.GetAsyncKeyStateB(Key.RightControl) || UIManager.GetAsyncKeyStateB(Key.LeftControl);
     if (!selectionMode)
     {
        BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorOperation, (int)TextureOperation.SelectArg1);
        BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.ColorArgument1, (int)TextureArgument.TextureColor);
        BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaOperation, (int)TextureOperation.SelectArg1);
        BRenderDevice.getDevice().SetTextureStageState(0, TextureStageStates.AlphaArgument1, (int)TextureArgument.TextureColor);

        BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
        BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceColor);
        BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceColor);

        float ptSize = 2;
        BRenderDevice.getDevice().SetRenderState(RenderStates.PointSize, ptSize);
        BRenderDevice.getDevice().VertexFormat = Rendering.VertexTypes.Pos_uv0.FVF_Flags;
        BRenderDevice.getDevice().SetStreamSource(0, gTexCursorVB, 0);
        BRenderDevice.getDevice().Indices = gTexCursorIB;
        BRenderDevice.getDevice().VertexShader = null;
        BRenderDevice.getDevice().PixelShader = null;
        BRenderDevice.getDevice().SetTexture(0, TerrainGlobals.getTerrainFrontEnd().getSelectedMaskTexture());
        //BRenderDevice.getDevice().SetTexture(0, TerrainGlobals.getTerrainFrontEnd().getSelectedTexture());
                 
        BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, cNumTexCursorVerts, 0, cNumTexCursorInds / 3);
   }
 }
 }
 */


/*
 private void drawVertModeCursor()
{
   // Draw affected brush verts
           
      if (cNumVertCursorVerts > 0)
      {
         float ptSize = 2;
         BRenderDevice.getDevice().SetRenderState(RenderStates.PointSize, ptSize);
         BRenderDevice.getDevice().VertexFormat = VertexFormats.Position | VertexFormats.Diffuse;
         BRenderDevice.getDevice().SetStreamSource(0, gVertCursorVB, 0);
         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         BRenderDevice.getDevice().SetTexture(0, null);
         BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.PointList, 0, cNumVertCursorVerts);
      }
   }

 */

/*
     *         private void updateTextureModeCursor()
   {
       //get our view ray
       Vector3 orig = getRayPosFromMouseCoords(false);
       Vector3 dir = getRayPosFromMouseCoords(true) - orig;
       dir = BTerrain.Normalize(dir);

       //get our intersect point
       Vector3 intpt = Vector3.Empty;
       BTerrainQuadNode node = null;
       if (TerrainGlobals.getTerrain().rayIntersects(ref orig, ref dir, ref intpt, ref node, false))
       {
          {
             float i = 0; float j = 0;
             // in this node, find the CLOSEST vertex to this actual position.
             findClosestVertex(ref i, ref j, ref intpt, node);
             intpt.X = i *TerrainGlobals.getTerrain().getTileScale();
             intpt.Z = j *TerrainGlobals.getTerrain().getTileScale();
          }
               
           float scale = mBrushInfo.mRadius / 4f;
           float validRadius = 6 * ((mMaskScalar * ((BTerrainPaintBrush)mCurrBrush).getMaskWidth()) / (BTerrainQuadNode.getMaxNodeWidth() / muScale)) / 2f;

           // Get bounding box of our circular area.
           float RecipTileSize = 1 / TerrainGlobals.getTerrain().getTileScale();
           long minXVertex = (long)((intpt.X - scale*5) * RecipTileSize);
           long minZVertex = (long)( (intpt.Z - scale *5) * RecipTileSize);
           long maxXVertex = (long)( (intpt.X + scale *5 + 1.0f) * RecipTileSize);
           long maxZVertex = (long)((intpt.Z + scale * 5 + 1.0f) * RecipTileSize);

           int desminX = (int)minXVertex;
           int desminZ = (int)minZVertex;
           int desxWidth = (int)(maxXVertex - minXVertex);
           int deszWidth = (int)(maxZVertex - minZVertex);

           //make Valid Vertex Extents
           if (minXVertex < 0) minXVertex = 0;
           if (minZVertex < 0) minZVertex = 0;
           if (maxXVertex >= TerrainGlobals.getTerrain().getNumXVerts()) maxXVertex = TerrainGlobals.getTerrain().getNumXVerts() - 1;
           if (maxZVertex >= TerrainGlobals.getTerrain().getNumZVerts()) maxZVertex = TerrainGlobals.getTerrain().getNumZVerts() - 1;

           int xWidth = (int)(maxXVertex - minXVertex);
           int zWidth = (int)(maxZVertex - minZVertex);
           //grab the edges of our brush
           int numVerts = (int)(xWidth * zWidth);
           Rendering.VertexTypes.Pos_uv0[] cursorVerts = new Rendering.VertexTypes.Pos_uv0[numVerts];
           int counter = 0;
             for(long x =minXVertex;x<maxXVertex;x++)
             {
                for (long z = minZVertex; z <maxZVertex; z++)
                {
                   long index = (x) * TerrainGlobals.getTerrain().getNumZVerts() + (z);
                   Vector3 pos = TerrainGlobals.getTerrain().getPos(x, z);
                   cursorVerts[counter].x = pos.X;
                   cursorVerts[counter].y = pos.Y;
                   cursorVerts[counter].z = pos.Z;
                   cursorVerts[counter].v0 = (float)(x - desminX) / (float)(desxWidth);
                   cursorVerts[counter].u0 = (float)(z - desminZ) / (float)(deszWidth);


                   counter++;
                }
             }
               
           if (numVerts > cNumTexCursorVerts)
           {
               if (gTexCursorVB != null)
                   gTexCursorVB.Dispose();
                gTexCursorVB = new VertexBuffer(typeof(Rendering.VertexTypes.Pos_uv0), numVerts, BRenderDevice.getDevice(), Usage.None, Rendering.VertexTypes.Pos_uv0.FVF_Flags, Pool.Managed);
           }
           cNumTexCursorVerts = numVerts;

           //copy verts over
           unsafe
           {

              using (GraphicsStream stream = gTexCursorVB.Lock(0, cNumTexCursorVerts * sizeof(Rendering.VertexTypes.Pos_uv0), LockFlags.None))
               {
                   stream.Write(cursorVerts);
                   gTexCursorVB.Unlock();
               }
           }

          //create our index buffer (ugggg...)
                
               

                
           cNumTexCursorInds = (xWidth * zWidth) * 6;
               

           {
              if (gTexCursorIB != null)
                 gTexCursorIB.Dispose();
              gTexCursorIB = new IndexBuffer(typeof(int), cNumTexCursorInds, BRenderDevice.getDevice(), Usage.None,Pool.Managed);
              unsafe
             {
                using (GraphicsStream stream = gTexCursorIB.Lock(0, sizeof(int) * cNumTexCursorInds, LockFlags.None))
                 {
                     int* inds = (int*)stream.InternalDataPointer;//buffer;

                     counter = 0;
                     int tw = xWidth-1;
                     int td = zWidth-1;
                     int vd = zWidth;
                     for (int x = 0; x < tw; x++)
                     {
                         for (int z = 0; z < td; z++)
                         {
                             int k = (x * vd + z);

                             inds[counter++] = k;
                             inds[counter++] = (k + vd);
                             inds[counter++] = (k + 1);

                             inds[counter++] = (k + 1);
                             inds[counter++] = (k + vd);
                             inds[counter++] = (k + vd +1);
                          }
                     }
                     gTexCursorIB.Unlock();
                 }
             }
           }

       }
   }

     */


/*
 *         private void updateVertModeCursor()
{
   //get our view ray
   Vector3 orig = getRayPosFromMouseCoords(false);
   Vector3 dir = getRayPosFromMouseCoords(true) - orig;
   dir = BTerrain.Normalize(dir);
   //UIManager.Debug(dir.X.ToString() + "  " + dir.Y.ToString());

   //get our intersect point
   Vector3 intPoint = Vector3.Empty;
   BTerrainQuadNode node = null;
   if (TerrainGlobals.getTerrain().rayIntersects(ref orig, ref dir, ref intPoint, ref node, true))
   {
      if (mBrushShowVerts)
      {
         // Find affected points
         List<int> points = new List<int>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getSphereIntersection(points, ref intPoint, mBrushInfo.mRadius);

         int numVerts = points.Count;
         CursorColorVert[] cursorVerts = new CursorColorVert[numVerts];
         int counter = 0;
         uint a = BRenderDevice.D3DCOLOR_COLORVALUE(1, 0, 0, 1);

         // Go through points and adjust accordingly.
         for (int i = 0; i < points.Count; i++)
         {
            int index = points[i];

            int x = index / TerrainGlobals.getTerrain().getNumZVerts();
            int z = index % TerrainGlobals.getTerrain().getNumZVerts();

            Vector3 vertPos = TerrainGlobals.getTerrain().getPostDeformPos(x, z);

            float factor = 0f;
            if (!TerrainGlobals.getEditor().vertInBrushArea(ref vertPos, ref intPoint, mBrushInfo.mRadius, mBrushInfo.mRadius * mBrushInfo.mHotspot, out factor))
               continue;

            cursorVerts[counter].xyz = vertPos;
            cursorVerts[counter].color = BRenderDevice.D3DCOLOR_COLORVALUE(factor, 0, 0, 1);
            counter++;
         }


         //if our vertcount is wierd, fix it
         if (numVerts > cNumVertCursorVerts || gVertCursorVB == null)
         {
            if (gVertCursorVB != null)
               gVertCursorVB.Dispose();
            gVertCursorVB = new VertexBuffer(typeof(CursorColorVert), numVerts, BRenderDevice.getDevice(), Usage.WriteOnly, VertexFormats.Position | VertexFormats.Diffuse, Pool.Default);//, &gVertCursorVB, null);

         }
         cNumVertCursorVerts = numVerts;
         //copy verts over
         unsafe
         {
            using (GraphicsStream stream = gVertCursorVB.Lock(0, cNumVertCursorVerts * sizeof(CursorColorVert), LockFlags.None))// (void**)&pVertices, 0))
            {
               stream.Write(cursorVerts);
               gVertCursorVB.Unlock();
            }
         }
      }

 */