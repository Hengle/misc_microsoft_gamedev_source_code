using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using EditorCore;
using Rendering;

namespace Terrain
{

   public class SimFlightRep : SpatialQuadTree
   {
      uint mWidth = 32;
      uint mHeight = 32;
      uint mNumXVertsPerCell = 0;
      uint mNumZVertsPerCell = 0;

      float mTileScale;

      float[] mHeights;
      public bool mRenderHeights = false;
      static float cVisualHeightOffset = 0.1f;
      float mHeightGeneratedOffset = 16.0f;

      public float getHeightGenOffset() { return mHeightGeneratedOffset; }
      public void setHeightGenOffset(float height) { mHeightGeneratedOffset = height; }
      //----------------------------------
      public SimFlightRep()
      {

      }
      ~SimFlightRep()
      {
         destroy();
      }

      public void init()
      {
         mWidth = (uint)(TerrainGlobals.getTerrain().getNumXVerts() >> 6) + 1;
         mHeight = (uint)(TerrainGlobals.getTerrain().getNumZVerts() >> 6) + 1;
         mHeights = new float[mWidth * mHeight];
         initHeightOverride();

         float terrainVisToFlightMultipleX = (mWidth - 1) / (float)TerrainGlobals.getTerrain().getNumXVerts();
         float terrainVisToFlightMultipleZ = (mHeight - 1) / (float)TerrainGlobals.getTerrain().getNumZVerts();
         mTileScale = TerrainGlobals.getTerrain().getTileScale() * (1.0f / terrainVisToFlightMultipleX);

         createFlightHeightRepQuadTree(mWidth, mHeight);
      }

      new public void destroy()
      {
         base.destroy();
         destroyHeightOverride();
         if (mHeights != null)
         {
            mHeights = null;
         }
      }
      //----------------------------------
      public void recalculateHeights()
      {
         //generate our heights representation from the terrain, 
         //then add our jagged differences.
         int xRes = 256;
         float[] gpuHeightRepArray = new float[xRes * xRes];
         for (uint i = 0; i < xRes * xRes; i++)
            gpuHeightRepArray[i] = float.MinValue;
         HeightsGen hg = new HeightsGen();
         hg.computeHeightFieldDirectFloat((uint)xRes, (uint)xRes, true, -BMathLib.unitX, true, ref gpuHeightRepArray);

         hg.destroy();
         hg = null;


         //resize our terrain input to our local input.
         float[] tmpHeightRes = ImageManipulation.resizeF32Img(gpuHeightRepArray, xRes, xRes, (int)mWidth - 1, (int)mHeight - 1, ImageManipulation.eFilterType.cFilter_Linear);
         //  mHeights = null;
         //  mHeights = ImageManipulation.resizeF32Img(gpuHeightRepArray, xRes, xRes, (int)mWidth , (int)mHeight, ImageManipulation.eFilterType.cFilter_Linear); //new float[mWidth * mHeight];

         for (int x = 0; x < mWidth - 1; x++)
         {
            for (int z = 0; z < mHeight - 1; z++)
            {
               mHeights[x * mWidth + z] = tmpHeightRes[x * (mWidth - 1) + z] + mHeightGeneratedOffset;
            }
         }

         //fill our edges with the previous height val.
         for (int q = 0; q < mWidth - 1; q++)
         {
            mHeights[(mWidth - 1) * mWidth + q] = mHeights[(mWidth - 2) * mWidth + q];
            mHeights[q * mWidth + (mWidth - 1)] = mHeights[q * mWidth + (mWidth - 2)];
         }

         //lets choose the max value between two samples..
         int strideStep = (int)(xRes / (mWidth - 1));
         for (int i = 0; i < mWidth; i++)
         {
            for (int j = 0; j < mHeight; j++)
            {
               int offX = (int)BMathLib.Clamp((i * strideStep), 0, xRes - 1);
               int offZ = (int)BMathLib.Clamp((j * strideStep), 0, xRes - 1);

               float highRes = gpuHeightRepArray[offX + xRes * offZ];
               if (highRes > mHeights[i + mWidth * j])
                  mHeights[i + mWidth * j] = highRes;
            }
         }

         tmpHeightRes = null;
         gpuHeightRepArray = null;
         recalculateBBs();
         recalculateVisuals();
      }
      //----------------------------------
      public float getHeight(int x, int z)
      {
         if (x < 0 || z < 0)
            return cJaggedEmptyValue;

         if (x >= mWidth - 1) x = (int)(mWidth - 1u);
         if (z >= mHeight - 1) z = (int)(mHeight - 1u);


         return mHeights[x * mWidth + z];
      }
      public float getCompositeHeight(int x, int z)
      {
         if (x < 0 || z < 0)
            return cJaggedEmptyValue;

         if (x >= mWidth - 1) x = (int)(mWidth - 1u);
         if (z >= mHeight - 1) z = (int)(mHeight - 1u);


         float h = getHeight(x, z);
         float jh = getJaggedHeight(x, z);
         if (jh == cJaggedEmptyValue)
            return h;

         return jh;
      }
      public float3 getWorldspacePoint(int x, int z)
      {
         if (x < 0 || z < 0)
            return float3.Empty;

         if (x >= mWidth - 1) x = (int)(mWidth - 1);
         if (z >= mHeight - 1) z = (int)(mHeight - 1);


         return new float3(x * mTileScale, getCompositeHeight(x, z), z * mTileScale);
      }
      public uint getNumXPoints() { return mWidth; }
      public uint getNumZPoints() { return mHeight; }
      public float getTileScale() { return mTileScale; }
      public Vector3 getLastIntersectionPoint() { return mLastMoustIntPt.toVec3(); }
      //----------------------------------
      public static float cJaggedEmptyValue = -9999;
      private JaggedContainer<float> mHeightOverride = null;
      public void initHeightOverride()
      {
         mHeightOverride = new JaggedContainer<float>((int)(mWidth * mHeight));
         mHeightOverride.SetEmptyValue(cJaggedEmptyValue);
      }
      public void destroyHeightOverride()
      {
         if (mHeightOverride != null)
         {
            clearHeightOverride();
            mHeightOverride = null;
         }
      }
      public void clearHeightOverride()
      {
         mHeightOverride.Clear();
      }
      public void createJaggedFrom(JaggedContainer<float> v)
      {
         destroyHeightOverride();
         initHeightOverride();

         long id;
         float maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == cJaggedEmptyValue)
               continue;

            mHeightOverride.SetValue(id, maskValue);
         }
      }
      public void setJaggedHeight(int x, int z, float val)
      {
         if (x < 0 || x >= mWidth || z < 0 || z >= mHeight)
            return;

         int idx = (int)(x * mWidth + z);
         mHeightOverride.SetValue(idx, val);
      }
      public float getJaggedHeight(int x, int z)
      {
         if (x < 0 || x >= mWidth || z < 0 || z >= mHeight)
            return cJaggedEmptyValue;
         return getJaggedHeight((int)(x * mWidth + z));
      }
      public float getJaggedHeight(int idx)
      {
         return mHeightOverride.GetValue(idx);
      }
      public JaggedContainer<float> getJaggedHeight()
      {
         return mHeightOverride;
      }

      //----------------------------------
      public void recalculateBBs()
      {
         for (uint minx = 0; minx < mNumXLeafCells; minx++)
         {
            for (uint minz = 0; minz < mNumZLeafCells; minz++)
            {
               SpatialQuadTreeCell spatialCell = getLeafCellAtGridLoc(minx, minz);
               recalculateCellBB(spatialCell);
            }
         }
         mRootCell.updateBoundsFromChildren();
      }
      void recalculateCellBB(SpatialQuadTreeCell spatialCell)
      {
         BBoundingBox bb = new BBoundingBox();
         bb.empty();
         FlightHeightRepQuadCell cell = ((FlightHeightRepQuadCell)spatialCell.mExternalData);
         for (uint i = (uint)cell.mMinXVert; i <= cell.mMinXVert + mNumXVertsPerCell; i++)
         {
            for (uint j = (uint)cell.mMinZVert; j <= cell.mMinZVert + mNumZVertsPerCell; j++)
            {
               if (i >= mWidth || j >= mHeight)
                  continue;

               float3 worldPos = getWorldspacePoint((int)i, (int)j);
               bb.addPoint(worldPos.toVec3());

            }
         }
         spatialCell.setBounds(new float3(bb.min), new float3(bb.max));
         bb = null;
      }
      //----------------------------------
      public void render(bool renderCursor)
      {
         if (!mRenderHeights)
            return;

         List<SpatialQuadTreeCell> nodes = new List<SpatialQuadTreeCell>();
         getVisibleNodes(nodes, TerrainGlobals.getTerrain().getFrustum());

         //update any visual handles that need it..
         for (int i = 0; i < nodes.Count; i++)
         {
            FlightHeightRepQuadCell cell = ((FlightHeightRepQuadCell)nodes[i].mExternalData);
            Debug.Assert(cell != null);

            if (cell.mVisualHandle == null)
               cell.mVisualHandle = newVisualHandle((int)cell.mMinXVert, (int)cell.mMinZVert);

            renderCell(cell.mVisualHandle);
         }

         if (renderCursor)
            ((BTerrainFlightBrush)TerrainGlobals.getEditor().getCurrentBrush()).render();
      }
      void renderCell(FlightHeightVisualData handle)
      {
         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos_Color.vertDecl;
         BRenderDevice.getDevice().VertexFormat = VertexTypes.Pos_Color.FVF_Flags;

         BRenderDevice.getDevice().VertexShader = null;
         BRenderDevice.getDevice().PixelShader = null;
         BRenderDevice.getDevice().SetTexture(0, null);

         BRenderDevice.getDevice().SetStreamSource(0, handle.mVB, 0);
         BRenderDevice.getDevice().Indices = handle.mIB;

         BRenderDevice.getDevice().RenderState.CullMode = Cull.None;

         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);
         //BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, handle.mNumPrims);
         BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, handle.mNumVerts, 0, handle.mNumPrims);
         BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);

         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable, false);

         BRenderDevice.getDevice().SetRenderState(RenderStates.SourceBlend, (int)Blend.SourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.DestinationBlend, (int)Blend.InvSourceAlpha);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, true);
         //BRenderDevice.getDevice().DrawPrimitives(PrimitiveType.TriangleList, 0, handle.mNumPrims);
         BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, handle.mNumVerts, 0, handle.mNumPrims);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaBlendEnable, false);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ZBufferWriteEnable, true);
         BRenderDevice.getDevice().RenderState.CullMode = Cull.CounterClockwise;

      }
      unsafe FlightHeightVisualData newVisualHandle(int minX, int minZ)
      {
         int width = (int)mNumXVertsPerCell;
         int vd = width + 1;
         int tw = width;
         int td = width;


         FlightHeightVisualData svd = new FlightHeightVisualData();
         svd.mNumVerts = vd * vd;
         svd.mVB = new VertexBuffer(typeof(VertexTypes.Pos_Color), (int)svd.mNumVerts, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_Color.FVF_Flags, Pool.Managed);

         //standard IB
         svd.mNumPrims = vd * vd * 2;
         svd.mIB = new IndexBuffer(typeof(int), svd.mNumPrims * 3, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
         GraphicsStream stream = svd.mIB.Lock(0, 0, LockFlags.None);
         int* inds = (int*)stream.InternalDataPointer;

         //fill our intex buffer
         int counter = 0;
         for (int z = 0; z < td; z++)
         {
            for (int x = 0; x < tw; x++)
            {
               int k = (x + (z * vd));
               inds[counter++] = (k);
               inds[counter++] = (k + vd);
               inds[counter++] = (k + 1);

               inds[counter++] = (k + 1);
               inds[counter++] = (k + vd);
               inds[counter++] = (k + vd + 1);
            }
         }
         svd.mIB.Unlock();

         //update and fill our vertex buffer
         updateVisualHandle(ref svd, minX, minZ);

         return svd;

      }
      unsafe void updateVisualHandle(ref FlightHeightVisualData handle, int minX, int minZ)
      {
         if (handle == null)
            return;

         int width = (int)(mNumXVertsPerCell + 1);

         GraphicsStream stream = handle.mVB.Lock(0, handle.mNumVerts * sizeof(VertexTypes.Pos_Color), LockFlags.None);
         VertexTypes.Pos_Color* verts = (VertexTypes.Pos_Color*)stream.InternalDataPointer;

         uint obsCol = 0x44D9D9F3;
         //generate each tile as a seperate triList
         int counter = 0;
         for (int x = 0; x < width; x++)
         {
            for (int z = 0; z < width; z++)
            {
               int offX = (int)BMathLib.Clamp(minX + x, 0, mWidth - 1);
               int offZ = (int)BMathLib.Clamp(minZ + z, 0, mHeight - 1);
               float3 wsp = new float3(offX * mTileScale, getCompositeHeight(offX, offZ), offZ * mTileScale);
               verts[counter].x = wsp.X;
               verts[counter].y = wsp.Y + cVisualHeightOffset;
               verts[counter].z = wsp.Z;
               verts[counter].color = (int)obsCol;
               counter++;
            }
         }
         handle.mVB.Unlock();
      }
      public void recalculateVisuals()
      {
         for (uint minx = 0; minx < mNumXLeafCells; minx++)
         {
            for (uint minz = 0; minz < mNumZLeafCells; minz++)
            {
               SpatialQuadTreeCell spatialCell = getLeafCellAtGridLoc(minx, minz);
               FlightHeightRepQuadCell cell = ((FlightHeightRepQuadCell)spatialCell.mExternalData);
               updateVisualHandle(ref cell.mVisualHandle, cell.mMinXVert, cell.mMinZVert);
            }
         }
      }
      public void updateAfterPainted(int minX, int minZ, int maxX, int maxZ)
      {
         //verts are in our local stride.
         //find any chunks intersecting these numbers and update them.
         uint minXCell = (uint)BMathLib.Clamp((minX / mNumXVertsPerCell) - 1, 0, mNumXLeafCells);
         uint minZCell = (uint)BMathLib.Clamp((minZ / mNumZVertsPerCell) - 1, 0, mNumXLeafCells);
         uint maxXCell = (uint)BMathLib.Clamp((maxX / mNumXVertsPerCell) + 1, 0, mNumXLeafCells);
         uint maxZCell = (uint)BMathLib.Clamp((maxZ / mNumZVertsPerCell) + 1, 0, mNumXLeafCells);


         for (uint minx = minXCell; minx < maxXCell; minx++)
         {
            for (uint minz = minZCell; minz < maxZCell; minz++)
            {
               SpatialQuadTreeCell spatialCell = getLeafCellAtGridLoc(minx, minz);
               FlightHeightRepQuadCell cell = ((FlightHeightRepQuadCell)spatialCell.mExternalData);
               updateVisualHandle(ref cell.mVisualHandle, cell.mMinXVert, cell.mMinZVert);
               recalculateCellBB(spatialCell);
            }
         }


         mRootCell.updateBoundsFromChildren();
      }
      //----------------------------------
      public void createFlightHeightRepQuadTree(uint numXVerts, uint numZVerts)
      {
         Vector3 min = new Vector3(0, 0, 0);
         Vector3 max = new Vector3(numXVerts * mTileScale, 10, numZVerts * mTileScale);


         uint numLevels = 1;
         uint numXNodes = mWidth;// (uint)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         // while (numXNodes > 1) { numXNodes = numXNodes >> 1; numLevels++; }

         base.createSpatialQuadTree(new float3(min), new float3(max), numXNodes, numXNodes);//numLevels);

         mNumXVertsPerCell = mWidth / mNumXLeafCells;
         mNumZVertsPerCell = mHeight / mNumZLeafCells;

         //initalize our leaf nodes
         for (uint i = 0; i < mNumXLeafCells; i++)
         {
            for (uint j = 0; j < mNumZLeafCells; j++)
            {
               SpatialQuadTreeCell cell = getLeafCellAtGridLoc(i, j);
               if (cell != null)
               {
                  cell.mExternalData = new FlightHeightRepQuadCell();
                  ((FlightHeightRepQuadCell)cell.mExternalData).mMinXVert = (int)(i * mNumXVertsPerCell);
                  ((FlightHeightRepQuadCell)cell.mExternalData).mMinZVert = (int)(j * mNumZVertsPerCell);
               }
            }
         }
      }
      //----------------------------------
      void getPointsIntersectingSphere(List<int> points, float3 sphereCenter, float sphereRadius)
      {
         List<SpatialQuadTreeCell> nodes = new List<SpatialQuadTreeCell>();
         getLeafCellsIntersectingSphere(nodes, sphereCenter, sphereRadius);
         if (nodes.Count == 0)
            return;

         Vector3 center = sphereCenter.toVec3();
         for (int q = 0; q < nodes.Count; q++)
         {
            FlightHeightRepQuadCell cell = ((FlightHeightRepQuadCell)nodes[q].mExternalData);
            Debug.Assert(cell != null);
            for (uint i = (uint)cell.mMinXVert; i < (uint)cell.mMinXVert + mNumXVertsPerCell; i++)
            {
               for (uint j = (uint)cell.mMinZVert; j < (uint)cell.mMinZVert + mNumZVertsPerCell; j++)
               {
                  uint index = (uint)((i) + mWidth * (j));
                  Vector3 vert = getWorldspacePoint((int)(i), (int)(j)).toVec3();
                  if (BMathLib.pointSphereIntersect(ref center, sphereRadius, ref vert))
                  {
                     points.Add((int)index);
                  }
               }
            }

         }

      }
      bool getClosestIntersectionPoint(float3 rayOrig, float3 rayDir, out float3 intPt)
      {
         intPt = float3.Empty;

         List<SpatialQuadTreeCell> nodes = new List<SpatialQuadTreeCell>();
         getLeafCellsIntersectingRay(nodes, rayOrig, rayDir);
         if (nodes.Count == 0)
            return false;


         //walk through each cell returned, do a per polygon intersection with each one..

         Vector3 origin = rayOrig.toVec3();
         Vector3 dir = rayDir.toVec3();

         Vector3 closestIntersect = Vector3.Empty;
         float closestDist = float.MaxValue;
         Vector3[] verts = new Vector3[3];
         bool hit = false;
         for (int q = 0; q < nodes.Count; q++)
         {
            FlightHeightRepQuadCell cell = ((FlightHeightRepQuadCell)nodes[q].mExternalData);
            Debug.Assert(cell != null);

            int minx = (int)(cell.mMinXVert);
            int minz = (int)(cell.mMinZVert);
            for (uint i = 0; i < mNumXVertsPerCell; i++)
            {
               for (uint j = 0; j < mNumZVertsPerCell; j++)
               {
                  int tileI = (int)(minx + i);
                  int tileJ = (int)(minz + j);
                  if (tileI >= mWidth - 1 || tileJ >= mHeight - 1)
                     continue;


                  bool tHit = false;
                  Vector3 pt = Vector3.Empty;

                  float[] h = new float[4];
                  h[0] = getCompositeHeight(tileI, tileJ);
                  h[1] = getCompositeHeight(tileI, tileJ + 1);
                  h[2] = getCompositeHeight(tileI + 1, tileJ);
                  h[3] = getCompositeHeight(tileI + 1, tileJ + 1);


                  if (h[0] != cJaggedEmptyValue && h[3] != cJaggedEmptyValue && h[1] != cJaggedEmptyValue)
                  {
                     verts[0] = new Vector3(tileI * mTileScale, h[0], (tileJ) * mTileScale);
                     verts[1] = new Vector3((tileI + 1) * mTileScale, h[3], (tileJ + 1) * mTileScale);
                     verts[2] = new Vector3(tileI * mTileScale, h[1], (tileJ + 1) * mTileScale);

                     if (BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt))
                     {
                        Vector3 vec = pt - origin;
                        //ensure this hit point is the closest to the origin
                        float len = vec.Length();
                        if (len < closestDist)
                        {
                           closestDist = len;
                           closestIntersect = pt;
                        }
                        hit = true;

                     }

                  }

                  if (h[0] != cJaggedEmptyValue && h[3] != cJaggedEmptyValue && h[2] != cJaggedEmptyValue)
                  {
                     verts[0] = new Vector3(tileI * mTileScale, h[0], (tileJ) * mTileScale);
                     verts[1] = new Vector3((tileI + 1) * mTileScale, h[3], (tileJ + 1) * mTileScale);
                     verts[2] = new Vector3((tileI + 1) * mTileScale, h[2], (tileJ) * mTileScale);

                     if (BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt))
                     {
                        Vector3 vec = pt - origin;
                        //ensure this hit point is the closest to the origin
                        float len = vec.Length();
                        if (len < closestDist)
                        {
                           closestDist = len;
                           closestIntersect = pt;
                        }
                        hit = true;

                     }
                  }
               }
            }

         }

         intPt.X = closestIntersect.X;
         intPt.Y = closestIntersect.Y;
         intPt.Z = closestIntersect.Z;
         return hit;
      }
      //----------------------------------
      float3 mLastMoustIntPt = float3.Empty;
      public void input()
      {

      }

      void getVertCursorWeights(float3 intPt)
      {

      }
      public Vector3 getIntersectPointFromScreenCursor()
      {
         float3 currIntersectionPt = float3.Empty;
         Vector3 orig = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
         Vector3 dir = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - orig;
         dir = BMathLib.Normalize(dir);

         //get our intersect point
         if (!getClosestIntersectionPoint(new float3(orig), new float3(dir), out currIntersectionPt))
            return Vector3.Empty;

         return currIntersectionPt.toVec3();
      }
      public void applyFlightRepBrush(bool alternate)
      {
         if (TerrainGlobals.getEditor().getStrokeInputType() == BTerrainEditor.eEditorStrokeInput.cStrokeInputMouse)
         {
            //raycast and get our intersection
            Vector3 orig = TerrainGlobals.getEditor().getRayPosFromMouseCoords(false);
            Vector3 dir = TerrainGlobals.getEditor().getRayPosFromMouseCoords(true) - orig;
            dir = BMathLib.Normalize(dir);

            //get our intersect point
            if (!getClosestIntersectionPoint(new float3(orig), new float3(dir), out mLastMoustIntPt))
               return;

            // Undo info
            // addVertexUndoNodes(nodes, false);

            // Find affected points
            List<int> points = new List<int>();
            getPointsIntersectingSphere(points, mLastMoustIntPt, TerrainGlobals.getEditor().mBrushInfo.mRadius);

            Vector3 intPoint = mLastMoustIntPt.toVec3();
            Vector3 intNormal = BMathLib.unitY;
            BrushInfo bi = TerrainGlobals.getEditor().getCurrentBrushInfo();
            ((BTerrainFlightBrush)TerrainGlobals.getEditor().getCurrentBrush()).applyOnBrush(points, ref intPoint, ref intNormal,
               ref bi,
               alternate);

            // addVertexUndoNodes(nodes, true);
         }
         else if (TerrainGlobals.getEditor().getStrokeInputType() == BTerrainEditor.eEditorStrokeInput.cStrokeInputKeyboard)
         {


            //int multiplier = 10;
            //// Apply on selection

            //List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
            //TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, Masking.mCurrSelectionMaskExtends.minX, Masking.mCurrSelectionMaskExtends.maxX,
            //                                                                         Masking.mCurrSelectionMaskExtends.minZ, Masking.mCurrSelectionMaskExtends.maxZ);




            //((BTerrainFoliageBrush)mCurrBrush).applyOnSelection(Masking.getCurrSelectionMaskWeights(), this.mBrushInfo.mIntensity,
            //   alternate || mCurrMode == eEditorMode.cModeFoliageErase);


         }

      }


      //----------------------------------
   };

   public class FlightHeightVisualData
   {
      public FlightHeightVisualData()
      {

      }

      ~FlightHeightVisualData()
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
      }
      public VertexBuffer mVB = null;
      public IndexBuffer mIB = null;
      public int mNumVerts = 0;
      public int mNumPrims = 0;
   }

   public class FlightHeightRepQuadCell
   {
      public int mMinXVert = 0;
      public int mMinZVert = 0;
      public FlightHeightVisualData mVisualHandle = null;

      public FlightHeightRepQuadCell()
      {

      }
      ~FlightHeightRepQuadCell()
      {
         destroy();
      }
      public void destroy()
      {
         if (mVisualHandle != null)
         {
            mVisualHandle.destroy();
            mVisualHandle = null;
         }
      }
   };

}