using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Collections.Generic;
using System;

using EditorCore;
using Rendering;
//-----------------------------------------------------
namespace Terrain
{
    #region TerrainQuadNode


    public struct CollisionNode
    {
       public BTerrainQuadNode quadNode;
       public Matrix           matrix;
    }
    //-----------------------------------------------------
    public struct BTerrainQuadNodeDesc
    {
        public int mMinXTile;
        public int mMaxXTile;
        public int mMinZTile;
        public int mMaxZTile;

        public int mMinXVert;
        public int mMaxXVert;
        public int mMinZVert;
        public int mMaxZVert;

        public Vector3 m_min;
        public Vector3 m_max;

        // PostDeform quadnode min/max extends.  These are different than the 
        // m_min/m_max since they represent the bounds of the geometry while 
        // a brush stroke is still active.  That is while the mouse click has
        // has been pressed but not release.  Both BB extends are needed since 
        // collision are often done with the pre-stroke terrain while visually
        // we are always looking at the deformed terrain.  The deformations
        // are submitted (and the m_min/m_max updated) the moment the stroke
        // ends.
        public Vector3 m_minPostDeform;
        public Vector3 m_maxPostDeform;

       //CLM - this is set every BTerrainEditor::render();
       //IF YOU ARE NOT THAT FUCTION, DO NOT SCREW WITH THIS
        public bool mIsVisibleThisFrame;
    };
    //---------------------------------------

    //
    public class BTerrainQuadNodeRenderInstance
    {
         public Matrix matrix;
         public int quadrant;
         public int geometryLOD;
         public int texturingLOD;
    };


    public class BTerrainQuadNode
    {
    

      public BTerrainQuadNode()
        {
            m_kids = null;
         
            for(int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
            {
               mVisualDataIndxPerLOD[i] = -1;
            }
            for (int i = 0; i < Terrain.BTerrainTexturing.cMaxNumLevels; i++)
            {
               mTextureDataPerLOD[i] = null;
            }
        }
      ~BTerrainQuadNode()
        {
            m_kids = null;
            //mTextureData = null;
           
            /*
            if (mVisualDataIndx != -1)
            {
               TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndx);
               mVisualDataIndx = -1;
            }
            */

            for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
            {
               if (mVisualDataIndxPerLOD[i] != -1)
               {
                  TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndxPerLOD[i]);
                  mVisualDataIndxPerLOD[i] = -1;
               }
            }

            for (int i = 0; i < Terrain.BTerrainTexturing.cMaxNumLevels; i++)
            {
               if (mTextureDataPerLOD[i] != null)
               {
                  mTextureDataPerLOD[i].Dispose();
                  mTextureDataPerLOD[i] = null;
               }
            }
        }
       public void Dispose()
       {
          /*
          if (mTextureData != null)
             mTextureData.Dispose();
          */

          //if (mVisualDataIndx != -1)
          //{
          //   TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndx);
          //   mVisualDataIndx = -1;
          //}

          for (int i = 0; i < Terrain.BTerrainTexturing.cMaxNumLevels; i++)
          {
             if (mTextureDataPerLOD[i] != null)
             {
                mTextureDataPerLOD[i].Dispose();
                mTextureDataPerLOD[i] = null;
             }
          }
       }

       public bool createFromHeightMap(float minX, float minZ, float maxX, float maxZ, Microsoft.DirectX.Direct3D.Device device,ref BTerrainQuadNode []mLeafQuadNodes)
       {
          //CLM For non pow2 maps, we need to create the tree from the bottom up!
          //otherwise our quadnode division gets all crazy 'n shit

          int numXChunks = (int)(maxZ / cMaxWidth);
          int numZChunks = (int)(maxZ / cMaxHeight);
          BTerrainQuadNode[,] leafs = new BTerrainQuadNode[numXChunks, numZChunks];
          mLeafQuadNodes = new BTerrainQuadNode[numXChunks * numZChunks];
          for (int i = 0; i < numXChunks; i++)
          {
             for (int j = 0; j < numZChunks; j++)
             {
                leafs[i, j] = new BTerrainQuadNode();
                leafs[i, j].mDesc.mMinXVert = (int)(i * cMaxWidth);
                leafs[i, j].mDesc.mMinZVert = (int)(j * cMaxHeight);
                leafs[i, j].mDesc.mMaxXVert = (int)((i + 1) * cMaxWidth);
                leafs[i, j].mDesc.mMaxZVert = (int)((j + 1) * cMaxHeight);

                leafs[i, j].mDesc.mMinXTile = (int)leafs[i, j].mDesc.mMinXVert;
                leafs[i, j].mDesc.mMaxXTile = (int)leafs[i, j].mDesc.mMaxXVert - 1;
                leafs[i, j].mDesc.mMinZTile = (int)leafs[i, j].mDesc.mMinZVert;
                leafs[i, j].mDesc.mMaxZTile = (int)leafs[i, j].mDesc.mMaxZVert - 1;

                leafs[i, j].mDesc.m_min.X = leafs[i, j].mDesc.m_minPostDeform.X = leafs[i, j].mDesc.mMinXVert * TerrainGlobals.getTerrain().getTileScale();
                leafs[i, j].mDesc.m_min.Y = leafs[i, j].mDesc.m_minPostDeform.Y = 0;
                leafs[i, j].mDesc.m_min.Z = leafs[i, j].mDesc.m_minPostDeform.Z = leafs[i, j].mDesc.mMinZVert * TerrainGlobals.getTerrain().getTileScale();

                leafs[i, j].mDesc.m_max.X = leafs[i, j].mDesc.m_minPostDeform.X = leafs[i, j].mDesc.mMaxXVert * TerrainGlobals.getTerrain().getTileScale();
                leafs[i, j].mDesc.m_max.Y = leafs[i, j].mDesc.m_maxPostDeform.Y = 16;
                leafs[i, j].mDesc.m_max.Z = leafs[i, j].mDesc.m_maxPostDeform.Z = leafs[i, j].mDesc.mMaxZVert * TerrainGlobals.getTerrain().getTileScale();

                leafs[i, j].m_kids = null;
                leafs[i, j].CreateBB();

                /*
                if (leafs[i, j].mVisualDataIndx != -1)
                {
                   TerrainGlobals.getVisual().freeVisualHandle(leafs[i, j].mVisualDataIndx);
                   leafs[i, j].mVisualDataIndx = -1;
                }
                */
                for (int lod = 0; lod < (int)BTerrainVisual.eLODLevel.cLODCount; lod++)
                {
                   if (mVisualDataIndxPerLOD[lod] != -1)
                   {
                      TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndxPerLOD[lod]);
                      mVisualDataIndxPerLOD[lod] = -1;
                   }
                }

                /*
                leafs[i, j].mTextureData = null;
                if (leafs[i, j].mTextureData == null)
                {
                   leafs[i, j].mLayerContainer = TerrainGlobals.getTexturing().getNewContainer();

                   leafs[i, j].mTextureData = new BTerrainTexturingDataHandle();
                }
                */
                leafs[i, j].mLayerContainer = TerrainGlobals.getTexturing().getNewContainer();

                for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
                {
                   leafs[i, j].mTextureDataPerLOD[lod] = null;
                   if (leafs[i, j].mTextureDataPerLOD[lod] == null)
                   {
                      leafs[i, j].mTextureDataPerLOD[lod] = new BTerrainTexturingDataHandle();
                      leafs[i, j].mTextureDataPerLOD[lod].mTextureLODLevel = lod;
                   }
                }


                //calc normals
                BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                       TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                       (int)leafs[i, j].mDesc.mMinXVert, (int)leafs[i, j].mDesc.mMaxXVert, (int)leafs[i, j].mDesc.mMinZVert, (int)leafs[i, j].mDesc.mMaxZVert);
                mLeafQuadNodes[i+j*numXChunks]=leafs[i, j];
             }
          }

          //recursivly create our upper containers
          BTerrainQuadNode[,] lowerNodes = leafs;
          BTerrainQuadNode[,] containerNodes = null;
          while (numXChunks != 2)
          {
             int lowerLengthsX = numXChunks;
             int lowerLengthsZ = numZChunks;

             numXChunks /= 2;
             numZChunks /= 2;

             if (numXChunks % 2 == 1) numXChunks++;
             if (numZChunks % 2 == 1) numZChunks++;

             containerNodes = new BTerrainQuadNode[(numXChunks), (numZChunks)];
             for (int i = 0; i < numXChunks; i++)
             {
                for (int j = 0; j < numZChunks; j++)
                {
                   containerNodes[i, j] = new BTerrainQuadNode();
                   if (containerNodes[i, j].m_kids == null)
                      containerNodes[i, j].m_kids = new BTerrainQuadNode[(int)ekidEnum.cNumTerrainKids];

                   int k = i * 2;
                   int l = j * 2;
                   int m = k + 1;
                   int n = l + 1;
                   containerNodes[i, j].m_kids[(int)ekidEnum.cTopLeft] = (k >= lowerLengthsX || n >= lowerLengthsZ) ? null : lowerNodes[k, n];
                   containerNodes[i, j].m_kids[(int)ekidEnum.cTopRight] = (m >= lowerLengthsX || n >= lowerLengthsZ) ? null : lowerNodes[m, n];
                   containerNodes[i, j].m_kids[(int)ekidEnum.cBotLeft] = (k >= lowerLengthsX || l >= lowerLengthsZ) ? null : lowerNodes[k, l];
                   containerNodes[i, j].m_kids[(int)ekidEnum.cBotRight] = (m >= lowerLengthsX || l >= lowerLengthsZ) ? null : lowerNodes[m, l];
                   containerNodes[i, j].CreateBB();
                }
             }
             lowerNodes = containerNodes;



          }

          //WE'RE THE ROOT!
          if (m_kids == null)
             m_kids = new BTerrainQuadNode[(int)ekidEnum.cNumTerrainKids];

          m_kids[(int)ekidEnum.cTopLeft] = lowerNodes[0, 1];
          m_kids[(int)ekidEnum.cTopRight] = lowerNodes[1, 1];
          m_kids[(int)ekidEnum.cBotLeft] = lowerNodes[0, 0];
          m_kids[(int)ekidEnum.cBotRight] = lowerNodes[1, 0];
          CreateBB();

          return true;
       }
       public bool createFromHeightMapRECURV(float minX, float minZ, float maxX, float maxZ, Microsoft.DirectX.Direct3D.Device device)
       {
          BTerrainDesc desc;
          desc = TerrainGlobals.getTerrain().getDesc();

          //create our local description
          mDesc.mMinXVert = (int)minX;
          mDesc.mMaxXVert = (int)maxX;
          mDesc.mMinZVert = (int)minZ;
          mDesc.mMaxZVert = (int)maxZ;
          if (mDesc.mMaxXVert > desc.mNumXVerts) mDesc.mMaxXVert = desc.mNumXVerts;
          if (mDesc.mMaxZVert > desc.mNumZVerts) mDesc.mMaxZVert = desc.mNumZVerts;

          mDesc.mMinXTile = (int)minX;
          mDesc.mMaxXTile = (int)maxX - 1;
          mDesc.mMinZTile = (int)minZ;
          mDesc.mMaxZTile = (int)maxZ - 1;

          mDesc.m_min.X = mDesc.m_minPostDeform.X = minX * desc.mTileScale;
          mDesc.m_min.Y = mDesc.m_minPostDeform.Y = 0;
          mDesc.m_min.Z = mDesc.m_minPostDeform.Z = minZ * desc.mTileScale;

          mDesc.m_max.X = mDesc.m_minPostDeform.X = maxX * desc.mTileScale;
          mDesc.m_max.Y = mDesc.m_maxPostDeform.Y = 16;
          mDesc.m_max.Z = mDesc.m_maxPostDeform.Z = maxZ * desc.mTileScale;

          //are we a child node?
          if (maxX - minX > cMaxWidth || maxZ - minZ > cMaxHeight)
          {
             if (m_kids == null)
                m_kids = new BTerrainQuadNode[(int)ekidEnum.cNumTerrainKids];
             bool ok = true;

             float halfX = minX + ((maxX - minX) / 2f);
             float halfZ = minZ + ((maxZ - minZ) / 2f);

             m_kids[(int)ekidEnum.cTopLeft] = new BTerrainQuadNode();
             m_kids[(int)ekidEnum.cTopRight] = new BTerrainQuadNode();
             m_kids[(int)ekidEnum.cBotLeft] = new BTerrainQuadNode();
             m_kids[(int)ekidEnum.cBotRight] = new BTerrainQuadNode();

             ok &= m_kids[(int)ekidEnum.cTopLeft].createFromHeightMapRECURV(minX, halfZ, halfX, maxZ, device);
             ok &= m_kids[(int)ekidEnum.cTopRight].createFromHeightMapRECURV(halfX, halfZ, maxX, maxZ, device);
             ok &= m_kids[(int)ekidEnum.cBotLeft].createFromHeightMapRECURV(minX, minZ, halfX, halfZ, device);
             ok &= m_kids[(int)ekidEnum.cBotRight].createFromHeightMapRECURV(halfX, minZ, maxX, halfZ, device);

             CreateBB();

             return ok;
          }
          m_kids = null;

          //we're a leaf, make our data.(DO IT!)
          BTerrainMetrics.addQuadNode();

          /*
          if (mVisualDataIndx != -1)
          {
             TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndx);
             mVisualDataIndx = -1;
          }
          */
          for (int lod = 0; lod < (int)BTerrainVisual.eLODLevel.cLODCount; lod++)
          {
             if (mVisualDataIndxPerLOD[lod] != -1)
             {
                TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndxPerLOD[lod]);
                mVisualDataIndxPerLOD[lod] = -1;
             }
          }

          //create our local BB
          CreateBB();
          /*
          mTextureData = null;
          if (mTextureData == null)
          {
             mTextureData = new BTerrainTexturingDataHandle();
             BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                                 TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                                 (int)minX, (int)maxX, (int)minZ, (int)maxZ);
          }
          */

          for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
          {
             mTextureDataPerLOD[lod] = null;
             if (mTextureDataPerLOD[lod] == null)
             {
                mTextureDataPerLOD[lod] = new BTerrainTexturingDataHandle();
                mTextureDataPerLOD[lod].mTextureLODLevel = lod;
             }
          }
          BTerrain.computeBasis(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getEditor().getNormals(),
                                              TerrainGlobals.getTerrain().getTileScale(), TerrainGlobals.getTerrain().getNumXVerts(),
                                              (int)minX, (int)maxX, (int)minZ, (int)maxZ);


          return true;
       }


      public bool rebuildDirty(Microsoft.DirectX.Direct3D.Device device)
       {
          bool ok = false;
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
             {
                if (m_kids[i] != null)
                  ok |= m_kids[i].rebuildDirty(device);
             }

             if(ok)
               CreateBB();
          }
          else
          {
             if (mDirty)
             {
                /*
                if (mVisualDataIndx != -1)
                {
                   TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndx);
                   mVisualDataIndx = -1;
                }
                */
                for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
                {
                   if (mVisualDataIndxPerLOD[i] != -1)
                   {
                      TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndxPerLOD[i]);
                      mVisualDataIndxPerLOD[i] = -1;
                   }
                }

                CreateBB();
                ok = true;
             }             
          }
          mDirty = false;
          return ok;
       }

      public bool rebuildDirtyPostDeform(Microsoft.DirectX.Direct3D.Device device)
       {
          bool ok = false;
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
             {
                if (m_kids[i] != null)
                  ok |= m_kids[i].rebuildDirtyPostDeform(device);
             }

             if (ok)
                CreateBBPostDeform();
          }
          else
          {
             if (mDirty)
             {
                /*
                if (mVisualDataIndx != -1)
                {
                   TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndx);
                   mVisualDataIndx = -1;
                }
                */
                for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
                {
                   if (mVisualDataIndxPerLOD[i] != -1)
                   {
                      TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndxPerLOD[i]);
                      mVisualDataIndxPerLOD[i] = -1;
                   }
                }

                CreateBBPostDeform();
                ok = true;
             }
          }
          mDirty = false;
          return ok;
       }


       public void render(BTerrainQuadNodeRenderInstance nodeInstance)
        {
            if (m_kids != null)
            {
               for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
               {
                  if (m_kids[i] != null)
                     m_kids[i].render(nodeInstance);
               }
            }
            else
            {

               TerrainGlobals.getRender().render(this,TerrainGlobals.getVisual().getVisualHandle(mVisualDataIndxPerLOD[nodeInstance.geometryLOD]), mTextureDataPerLOD[nodeInstance.texturingLOD], nodeInstance.matrix, nodeInstance.quadrant);
            //   if (TerrainGlobals.getEditor().mRenderSimHeights)
             //     TerrainGlobals.getEditor().getSimRep().render(mSimVisHandle);
            }
        }
       public void renderCursor()
       {
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].renderCursor();
          }
          else
          {
             // Use highest lod
             for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
             {
                if (mVisualDataIndxPerLOD[i] != -1)
                {
                   TerrainGlobals.getRender().renderCursor(TerrainGlobals.getVisual().getVisualHandle(mVisualDataIndxPerLOD[i]));
                   break;
                }
             }
         }
       }
       public void renderWidget(int passNum)
       {
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].renderWidget(passNum);
          }
          else
          {
             // Use highest lod
             for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
             {
                if (mVisualDataIndxPerLOD[i] != -1)
                {
                   TerrainGlobals.getRender().renderWidget(TerrainGlobals.getVisual().getVisualHandle(mVisualDataIndxPerLOD[i]), passNum);
                   break;
                }
             }
          }
       }


       public int evaluateCost(int lod)
       {
          int nLayers = mLayerContainer.getNumLayers();
          float apc = 0;

          //if(mTextureData!=null)
          //   apc = mTextureData.evaluateAPC();
          if (mTextureDataPerLOD[lod] != null)
          {
             apc = mTextureDataPerLOD[lod].evaluateAPC();
          }

          return (int)(nLayers * apc);
       }
       public void update()
       {
          //if (mTextureData != null)
          //  mTextureData.updateLOD();

         for (int i = 0; i < Terrain.BTerrainTexturing.cMaxNumLevels; i++)
         {
            if (mTextureDataPerLOD[i] != null)
            {
               mTextureDataPerLOD[i].updateLOD();
            }
         }
      }
       public void evaluate(int lod)//only called on visible nodes
       {
          /*
          //we're visible, 
          if(mTextureData!=null)
          {
             Vector3 center  = (mDesc.m_min + mDesc.m_max) * 0.5f;
             mTextureData.evaluateLOD(center, TerrainGlobals.getTerrainFrontEnd().mCameraManager.mEye,mLayerContainer,this);
          }
          */
          if (mTextureDataPerLOD[lod] != null)
          {
             mTextureDataPerLOD[lod].evaluateLOD();
          }
       }
       public void freeCacheHandles()
       {
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
             {
                if (m_kids[i] != null)
                   m_kids[i].freeCacheHandles();
             }
          }
          else
          {
             //mTextureData.free();
             for (int i = 0; i < Terrain.BTerrainTexturing.cMaxNumLevels; i++)
             {
                mTextureDataPerLOD[i].free();
             }
          }
       }


       //---------------------------------------
        public BTerrainQuadNodeDesc getDesc()
        {
            return mDesc;
        }
       public bool IsVisibleThisFrame
       {
          get
          {
             return mDesc.mIsVisibleThisFrame;
          }
          set
          {
             mDesc.mIsVisibleThisFrame=value;
          }
       }


       #region GET NODES FROM PARAMS
       public void getVisibleNodes(List<BTerrainQuadNode> nodes, List<int> handles, List<BTerrainQuadNodeRenderInstance> nodeInstances, BFrustum frust, Matrix mat, int inQuadrant)
       {

          Vector3 coord1 = mDesc.m_minPostDeform;
          Vector3 coord2 = mDesc.m_maxPostDeform;
          coord1.TransformCoordinate(mat);
          coord2.TransformCoordinate(mat);

          Vector3 minCoord, maxCoord;
          minCoord = new Vector3(Math.Min(coord1.X, coord2.X), Math.Min(coord1.Y, coord2.Y), Math.Min(coord1.Z, coord2.Z));
          maxCoord = new Vector3(Math.Max(coord1.X, coord2.X), Math.Max(coord1.Y, coord2.Y), Math.Max(coord1.Z, coord2.Z));

          if (frust.AABBVisible(minCoord, maxCoord))
          {
             if (m_kids != null)
             {
                for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                   if (m_kids[i]  != null)
                      m_kids[i].getVisibleNodes(nodes, handles, nodeInstances, frust, mat, inQuadrant);
             }
             else
             {
                /*
                Vector3 center  = (mDesc.m_min + mDesc.m_max) * (1f/2f);
                
                if (mVisualDataIndx == -1)
                   mVisualDataIndx = TerrainGlobals.getVisual().newVisualHandle(mDesc.mMinXVert, mDesc.mMinZVert, mDesc.mMaxXVert, mDesc.mMaxZVert, center);
                else
                {
                   int currLOD = TerrainGlobals.getVisual().getVisualHandle(mVisualDataIndx).LOD;
                   int lod = TerrainGlobals.getVisual().checkHandleLOD(mVisualDataIndx, center);
                   if (currLOD != lod)
                   {
                      TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndx);
                      mVisualDataIndx = TerrainGlobals.getVisual().newVisualHandle(mDesc.mMinXVert, mDesc.mMinZVert, mDesc.mMaxXVert, mDesc.mMaxZVert, center);
                   }
                }

                if (mVisualDataIndx != -1)
                   nodes.Add(this);
                */

                Vector3 center = (minCoord + maxCoord) * (1f / 2f);

                int geomLODLevel = TerrainGlobals.getVisual().getLODLevel(center);
                int textLODLevel = TerrainGlobals.getTexturing().getTextureLODLevel(center, TerrainGlobals.getTerrainFrontEnd().mCameraManager.mEye);

                if(mVisualDataIndxPerLOD[geomLODLevel] == -1)
                {
                   mVisualDataIndxPerLOD[geomLODLevel] = TerrainGlobals.getVisual().newVisualHandle(mDesc.mMinXVert, mDesc.mMinZVert, mDesc.mMaxXVert, mDesc.mMaxZVert, center, mDesc.m_min, mDesc.m_max);
                }

                if (mVisualDataIndxPerLOD[geomLODLevel] != -1)
                {
                   nodes.Add(this);
                   handles.Add(mVisualDataIndxPerLOD[geomLODLevel]);

                   BTerrainQuadNodeRenderInstance nodeInstance = new BTerrainQuadNodeRenderInstance();
                   nodeInstance.matrix = mat;
                   nodeInstance.quadrant = inQuadrant;
                   nodeInstance.geometryLOD = geomLODLevel;
                   nodeInstance.texturingLOD = textLODLevel;

                   nodeInstances.Add(nodeInstance);
                }
            }
         }
         /*
         else
         {
            clearVisibleDatHandle(); 
         }
         */
          
       }

       public void clearNonVisibleNodes(List<int> phandles)
       {
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].clearNonVisibleNodes(phandles);
          }
          else
          {
             for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
             {
                if (mVisualDataIndxPerLOD[i] != -1)
                {
                   if (!phandles.Contains(mVisualDataIndxPerLOD[i]))
                   {
                      TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndxPerLOD[i]);
                      mVisualDataIndxPerLOD[i] = -1;
                   }
                }
             }
          }
       }


       public void getVisibleNodes2(List<BTerrainQuadNode> nodes, BFrustum frust)
       {
          if (frust.AABBVisible(mDesc.m_minPostDeform, mDesc.m_maxPostDeform))
          {
             if (m_kids != null)
             {
                Vector3 lookat = TerrainGlobals.getTerrainFrontEnd().mCameraManager.mLookAt;
                SortedDictionary<float, BTerrainQuadNode> distanceHash = new SortedDictionary<float, BTerrainQuadNode>();
                
                for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                {
                    if(m_kids[i]  != null)
                    {
                       float x = (lookat.X - m_kids[i].getDesc().m_min.X);
                       float z = (lookat.X - m_kids[i].getDesc().m_min.Z);
                       float dist = (float)Math.Sqrt(x * x + z * z);

                       if (distanceHash.ContainsKey(dist))
                          dist += 0.00001f;
                       distanceHash[dist] = m_kids[i];
                    }
                 
                }
                foreach (float f in distanceHash.Keys)
                {
                   distanceHash[f].getVisibleNodes2(nodes, frust);
                }
             }
             else
             {
                Vector3 center = (mDesc.m_min + mDesc.m_max) * (1f / 2f);
                /*
                if (mVisualDataIndx == -1)
                   mVisualDataIndx = TerrainGlobals.getVisual().newVisualHandle(mDesc.mMinXVert, mDesc.mMinZVert, mDesc.mMaxXVert, mDesc.mMaxZVert,center);

                if (mVisualDataIndx != -1)
                   nodes.Add(this);
                */
                int lodLevel = TerrainGlobals.getVisual().getLODLevel(center);

                if (mVisualDataIndxPerLOD[lodLevel] == -1)
                   mVisualDataIndxPerLOD[lodLevel] = TerrainGlobals.getVisual().newVisualHandle(mDesc.mMinXVert, mDesc.mMinZVert, mDesc.mMaxXVert, mDesc.mMaxZVert, center, mDesc.m_min, mDesc.m_max);

                if (mVisualDataIndxPerLOD[lodLevel] != -1)
                   nodes.Add(this);
             }
          }
          else
          {
             clearVisibleDatHandle();
          }
       }

       public void getSortedVisibleNodes(ref Vector3 refpoint, SortedDictionary<float, BTerrainQuadNode> distanceHash, BFrustum frust)
       {
          if (frust.AABBVisible(mDesc.m_minPostDeform, mDesc.m_maxPostDeform))
          {
             if (m_kids != null)
             {
                //Vector3 lookat = TerrainGlobals.getTerrainFrontEnd().mCameraManager.LookAt;//.Eye;//
                SortedDictionary<float, BTerrainQuadNode> localdistanceHash = new SortedDictionary<float, BTerrainQuadNode>();

                for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                {
                    if(m_kids[i]  != null)
                    {
                       float nx = (m_kids[i].getDesc().m_min.X + m_kids[i].getDesc().m_max.X) / 2f;
                       float nz = (m_kids[i].getDesc().m_min.Z + m_kids[i].getDesc().m_max.Z) / 2f;

                       float x = (refpoint.X - nx);
                       float z = (refpoint.Z - nz);
                       float dist = (float)Math.Sqrt(x * x + z * z);

                       while (localdistanceHash.ContainsKey(dist))
                          dist += 0.1f;
                       localdistanceHash[dist] = m_kids[i];

                       if (m_kids[i].m_kids == null)
                       {
                          while (distanceHash.ContainsKey(dist))
                             dist += 0.1f;
                          distanceHash[dist] = m_kids[i];
                       }
                    }
               
                }
                foreach (float f in localdistanceHash.Keys)
                {
                   if (localdistanceHash[f].m_kids != null)
                     localdistanceHash[f].getSortedVisibleNodes(ref refpoint, distanceHash, frust);
                }
             }
          }
          else
          {
             clearVisibleDatHandle();
          }
       }

       public void getVisibleNodes3(ref Vector3 refpoint,  List<BTerrainQuadNode> nodes, BFrustum frust)
       {
          SortedDictionary<float, BTerrainQuadNode> distanceHash = new SortedDictionary<float, BTerrainQuadNode>();
          getSortedVisibleNodes(ref refpoint, distanceHash, frust);
          int hadHandles = 0;
          int failedHandles = 0;

          int toFree = distanceHash.Keys.Count - TerrainGlobals.getTerrainFrontEnd().NumTerrainHandles;
          int used = 0;

          foreach (float f in distanceHash.Keys)
          {
             if (used > TerrainGlobals.getTerrainFrontEnd().NumTerrainHandles)
             {
                distanceHash[f].clearVisibleDatHandle();
             }
             used++;
          }

          used = 0;
          foreach (float f in distanceHash.Keys)
          {

             BTerrainQuadNode node = distanceHash[f];
             Vector3 center = (mDesc.m_min + mDesc.m_max) * (1f / 2f);
             /*
             if (node.mVisualDataIndx == -1)
             {
                node.mVisualDataIndx = TerrainGlobals.getVisual().newVisualHandle(node.getDesc().mMinXVert, node.getDesc().mMinZVert, node.getDesc().mMaxXVert, node.getDesc().mMaxZVert,center);
             }
             else
             {
                hadHandles++;
             }
             if (node.mVisualDataIndx != -1)
             {
                nodes.Add(node);
             }
             else
             {
                failedHandles++;
             }
             */
             int lodLevel = TerrainGlobals.getVisual().getLODLevel(center);

             if (node.mVisualDataIndxPerLOD[lodLevel] == -1)
             {
                node.mVisualDataIndxPerLOD[lodLevel] = TerrainGlobals.getVisual().newVisualHandle(node.getDesc().mMinXVert, node.getDesc().mMinZVert, node.getDesc().mMaxXVert, node.getDesc().mMaxZVert, center, mDesc.m_min, mDesc.m_max);
             }
             else
             {
                hadHandles++;
             }

             if (node.mVisualDataIndxPerLOD[lodLevel] != -1)
             {
                nodes.Add(this);
             }
             else
             {
                failedHandles++;
             }


             used++;

             if (used > TerrainGlobals.getTerrainFrontEnd().NumTerrainHandles)
                return;
          }
       }

       public void getLeafNodes(List<BTerrainQuadNode> nodes)
       {
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if(m_kids[i]!=null)
                  m_kids[i].getLeafNodes(nodes);
          }
          else
          {
             nodes.Add(this);
          }
       }

       public BTerrainQuadNode getLeafNodeContainingPoint(int x, int z)
       {
          
          {
             if (m_kids != null)
             {
                BTerrainQuadNode tNode = null;
                for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                   if (m_kids[i] != null)
                   {
                      tNode = m_kids[i].getLeafNodeContainingPoint(x,z);
                      if (tNode != null)
                         return tNode;
                   }
                return null;
             }

             if (x >= mDesc.mMinXVert && x <= mDesc.mMaxXVert && z >= mDesc.mMinZVert && z <= mDesc.mMaxZVert)
             {
                return this;
             }
          }
          return null;
       }
       public BTerrainQuadNode getLeafNodeContainingTile(int x, int z)
       {
          {
             if (m_kids != null)
             {
                BTerrainQuadNode tNode = null;
                for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                   if (m_kids[i] != null)
                   {
                      tNode = m_kids[i].getLeafNodeContainingTile(x, z);
                      if (tNode != null)
                         return tNode;
                   }
                return null;
             }

             if (x >= mDesc.mMinXTile && x <= mDesc.mMaxXTile && z >= mDesc.mMinZTile && z <= mDesc.mMaxZTile)
             {
                return this;
             }
          }
          return null;
       }
       public BTerrainQuadNode getNeighborNode(int xNodesOffet, int yNodesOffset)
       {
          

          int fullNode = (int)BTerrainQuadNode.cMaxWidth;
          int halfNode = (int)(BTerrainQuadNode.cMaxWidth>>1);

          xNodesOffet = (int)BMathLib.Clamp(xNodesOffet, -1, 1);
          yNodesOffset = (int)BMathLib.Clamp(yNodesOffset, -1, 1);

          int tileDesX = 0;
          int tileDesY = 0;
          if(xNodesOffet ==0)
          {
             tileDesX = mDesc.mMinXVert;
          }
          else if (xNodesOffet < 0)
          {
             tileDesX = (int)(mDesc.mMinXVert - (halfNode + fullNode*(Math.Abs(xNodesOffet)-1)));
          }
          else
          {
             tileDesX = (int)(mDesc.mMinXVert + (halfNode + fullNode * xNodesOffet));
          }


          if (yNodesOffset == 0)
          {
             tileDesY = mDesc.mMinZVert;
          }
          else if (yNodesOffset < 0)
          {
             tileDesY = (int)(mDesc.mMinZVert - (halfNode + fullNode * (Math.Abs(yNodesOffset) - 1)));
          }
          else
          {
             tileDesY = (int)(mDesc.mMinZVert + (halfNode + fullNode * yNodesOffset));
          }


          if (tileDesX < 0 || tileDesX >= TerrainGlobals.getTerrain().getNumXVerts() ||
             tileDesY < 0 || tileDesY >= TerrainGlobals.getTerrain().getNumZVerts())
             return null;

          return TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingTile(tileDesX, tileDesY);
       }
       public void getLeafMaskedNodes(List<BTerrainQuadNode> nodes)
       {
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].getLeafMaskedNodes(nodes);
          }
          else
          {
             float val = 0;
             for (int x = 0; x < BTerrainQuadNode.cMaxWidth; x++)
             {
                for (int z = 0; z < BTerrainQuadNode.cMaxHeight; z++)
                {
                   if (Masking.isPointSelected(mDesc.mMinXVert + x, mDesc.mMinZVert + z, ref val) && val !=0)
                   {
                      nodes.Add(this);
                      return;
                   }
                }
             }   
          }
       }

       #endregion

       #region INTERSECTION
       public void getSortedIntersectedNodes(ref Vector3 origin, ref Vector3 dir, SortedDictionary<float, CollisionNode> distanceHash, Matrix mat)
       {
          float tt = 0;
          Vector3 tc = Vector3.Empty;
          Vector3 mn = mDesc.m_min;
          mn.X -= 1.0f;
          mn.Z -= 1.0f;
          Vector3 mx = mDesc.m_max;
          mx.X += 1.0f;
          mx.Z += 1.0f;
          if (BMathLib.ray3AABB(ref tc, ref tt, ref origin, ref dir, ref mn, ref mx))
          {
             if (m_kids == null)
             {
                float dist = tt;
                while (distanceHash.ContainsKey(dist))
                   dist += 0.1f;

                CollisionNode cNode;// = new CollisionNode();
                cNode.quadNode = this;
                cNode.matrix = mat;
                distanceHash[dist] = cNode;
             }
             else
             {
                for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                   if (m_kids[i]  != null)
                     m_kids[i].getSortedIntersectedNodes(ref origin, ref dir, distanceHash, mat);
             }
          }
       }

       //----------------------------------------------
       // Returns if collision (any) is found.  This method is the fastest since it won't look for the closest
       // collision nor for the intersection point.  As soon as and intersection is found it exits.  Good for
       // ambient occlusion.
       public bool rayIntersectsFAST(ref Vector3 origin, ref Vector3 dir)
       {
          float tt = 0;
          Vector3 tc = Vector3.Empty;
          if (!BMathLib.ray3AABB(ref tc, ref tt, ref origin, ref dir, ref mDesc.m_min, ref mDesc.m_max))
             return false;

          if (m_kids == null)
          {
             if (rayIntersectsLeafFAST(ref origin, ref dir))
                return (true);
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
             {
                if (m_kids[i]  != null)
                  if (m_kids[i].rayIntersectsFAST(ref origin, ref dir))
                   return (true);
             }
          }
          return (false);
       }

       // Returns closest collision point and node
       public bool rayIntersects(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref BTerrainQuadNode ownerNode)
       {
          int visTileX = 0;
          int visTileZ = 0;
          Vector3 intersectionNormal = new Vector3(0, 0, 0);
          return rayIntersects(ref origin, ref dir, ref intersectionPt, ref intersectionNormal, ref visTileX, ref visTileZ, ref ownerNode);
       }
       public bool rayIntersects(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal,ref BTerrainQuadNode ownerNode)
       {
          int visTileX = 0;
          int visTileZ = 0;
          return rayIntersects(ref origin, ref dir, ref intersectionPt, ref intersectionNormal,ref visTileX, ref visTileZ,ref ownerNode);
       }
       public bool rayIntersects(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal, ref int visTileX, ref int visTileZ, ref BTerrainQuadNode ownerNode)
       {
          SortedDictionary<float, CollisionNode> distanceHash = new SortedDictionary<float, CollisionNode>();
          getSortedIntersectedNodes(ref origin, ref dir, distanceHash, Matrix.Identity);
          Vector3 posX, dirX;


          // test mirror terrain
          Matrix mat1 = Matrix.Identity;
          mat1.M11 = -1;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat1);
          dirX.TransformNormal(mat1);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat1);

          Matrix mat2 = Matrix.Identity;
          mat2.M11 = -1;
          mat2.M33 = -1;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat2);
          dirX.TransformNormal(mat2);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat2);

          Matrix mat3 = Matrix.Identity;
          mat3.M33 = -1;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat3);
          dirX.TransformNormal(mat3);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat3);

          Matrix mat4 = Matrix.Identity;
          mat4.M11 = -1;
          mat4.M33 = -1;
          mat4.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat4);
          dirX.TransformNormal(mat4);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat4);

          Matrix mat5 = Matrix.Identity;
          mat5.M11 = -1;
          mat5.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat5);
          dirX.TransformNormal(mat5);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat5);

          Matrix mat6 = Matrix.Identity;
          mat6.M11 = -1;
          mat6.M33 = -1;
          mat6.M41 = TerrainGlobals.getTerrain().getWorldSizeX() * 2;
          mat6.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat6);
          dirX.TransformNormal(mat6);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat6);


          Matrix mat7 = Matrix.Identity;
          mat7.M33 = -1;
          mat7.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat7);
          dirX.TransformNormal(mat7);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat7);


          Matrix mat8 = Matrix.Identity;
          mat8.M11 = -1;
          mat8.M33 = -1;
          mat8.M43 = TerrainGlobals.getTerrain().getWorldSizeZ() * 2;

          posX = origin; dirX = dir;
          posX.TransformCoordinate(mat8);
          dirX.TransformNormal(mat8);
          getSortedIntersectedNodes(ref posX, ref dirX, distanceHash, mat8);


          foreach (float f in distanceHash.Keys)
          {
             if (distanceHash[f].quadNode.rayIntersectsLeafXform(ref origin, ref dir, ref intersectionPt, ref intersectionNormal, ref visTileX, ref visTileZ, distanceHash[f].matrix))
             {
                ownerNode = distanceHash[f].quadNode;
                return (true);
             }
          }

          return (false);
       }


       //----------------------------------------------
       Vector3[] verts = new Vector3[3];
       //OPTOMIZED VERSION FOR AO!!
       public bool rayIntersectsLeafFAST(ref Vector3 origin, ref Vector3 dir)
       {
          // this function is intended for leaf nodes only, assert if not
          System.Diagnostics.Debug.Assert(m_kids == null);

          Vector3 pt = Vector3.Empty;
          //       Vector3[] verts = new Vector3[3];

          for (int x = mDesc.mMinXTile; x <= mDesc.mMaxXTile; x++)
          {
             for (int z = mDesc.mMinZTile; z <= mDesc.mMaxZTile; z++)
             {
                // Check top triangle.
                TerrainGlobals.getTerrain().getPos(ref verts[0], x, z);
                TerrainGlobals.getTerrain().getPos(ref verts[1], x, z + 1);
                TerrainGlobals.getTerrain().getPos(ref verts[2], x + 1, z + 1);
                if (BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt))
                   return (true);

                // Check bottom triangle
                verts[1] = verts[2];
                TerrainGlobals.getTerrain().getPos(ref verts[2], x + 1, z);
                if (BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt))
                   return (true);
             }
          }
          return (false);
       }

       public bool rayIntersectsLeaf(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt)
       {
          int visTileX = 0;
          int visTileZ = 0;
          Vector3 intersectionNormal = new Vector3(0, 0, 0);
          return rayIntersectsLeaf(ref origin, ref dir, ref intersectionPt, ref intersectionNormal, ref visTileX, ref visTileZ);
       }
       public bool rayIntersectsLeaf(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal)
       {
          int visTileX = 0;
          int visTileZ = 0;
          return rayIntersectsLeaf(ref origin, ref dir, ref intersectionPt, ref intersectionNormal, ref visTileX, ref visTileZ);
       }
       public bool rayIntersectsLeaf(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal,ref int visTileX, ref int visTileZ)
       {
          // this function is intended for leaf nodes only, assert if not
          System.Diagnostics.Debug.Assert(m_kids == null);

          int maxX = Math.Min(mDesc.mMaxXTile + 1, TerrainGlobals.getTerrain().getDesc().mNumXVerts - 2);
          int maxZ = Math.Min(mDesc.mMaxZTile + 1, TerrainGlobals.getTerrain().getDesc().mNumZVerts - 2);
          bool hit = false;
          float closestDist = float.MaxValue;
          for (int x = mDesc.mMinXTile; x <= maxX; x++)
          {
             for (int z = mDesc.mMinZTile; z <= maxZ; z++)
             {
                bool tHit = false;
                Vector3 pt = Vector3.Empty;
                // Check top triangle.
                //Vector3[] verts = new Vector3[3];

                TerrainGlobals.getTerrain().getPos(ref verts[0], x, z);
                TerrainGlobals.getTerrain().getPos(ref verts[1], x, z + 1);
                TerrainGlobals.getTerrain().getPos(ref verts[2], x + 1, z + 1);
                tHit |= BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt);
                if (tHit)
                {
                   Vector3 vec = pt - origin;
                   //ensure this hit point is the closest to the origin
                   float len = vec.Length();
                   if (len < closestDist)
                   {
                      closestDist = len;
                      intersectionPt = pt;
                      visTileX = x;
                      visTileZ = z;

                      // TODO: find the actual interpolated normal here instead of the normal of one of the points
                      // in the triangle
                      intersectionNormal = TerrainGlobals.getTerrain().getNormal(x, z);
                   }
                   hit = true;
                }

                // Check bottom triangle
                verts[1] = verts[2];
                TerrainGlobals.getTerrain().getPos(ref verts[2], x + 1, z);
                tHit |= BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt);
                if (tHit)
                {
                   Vector3 vec = pt - origin;
                   //ensure this hit point is the closest to the origin
                   float len = vec.Length();
                   if (len < closestDist)
                   {
                      closestDist = len;
                      intersectionPt = pt;

                      visTileX = x;
                      visTileZ = z;

                      // TODO: find the actual interpolated normal here instead of the normal of one of the points
                      // in the triangle
                      intersectionNormal = TerrainGlobals.getTerrain().getNormal(x, z);
                   }
                   hit = true;
                }

                //verts = null;
             }
          }
          return (hit);
       }

       public bool rayIntersectsLeafXform(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal, ref int visTileX, ref int visTileZ, Matrix mat)
       {
          Vector3 posX, dirX;
          posX = origin;
          dirX = dir;

          posX.TransformCoordinate(mat);
          dirX.TransformNormal(mat);

          bool retValue = rayIntersectsLeaf(ref posX, ref dirX, ref intersectionPt, ref intersectionNormal, ref visTileX, ref visTileZ);

          intersectionPt.TransformCoordinate(mat);
          intersectionNormal.TransformNormal(mat);
          return (retValue);
       }


       //----------------------------------------------
       //----------------------------------------------
       //----------------------------------------------
       public bool rayIntersectsSimOverrideHeights(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal, ref int visTileX, ref int visTileZ, ref BTerrainQuadNode ownerNode)
       {
          SortedDictionary<float, CollisionNode> distanceHash = new SortedDictionary<float, CollisionNode>();
          getSortedIntersectedNodes(ref origin, ref dir, distanceHash, Matrix.Identity);

          foreach (float f in distanceHash.Keys)
          {
             if (distanceHash[f].quadNode.rayIntersectsSimOverrideLeaf(ref origin, ref dir, ref intersectionPt, ref intersectionNormal, ref visTileX, ref visTileZ))
             {
                ownerNode = distanceHash[f].quadNode;
                return (true);
             }
          }
          return false;
       }
       public bool rayIntersectsSimOverrideLeaf(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal, ref int visTileX, ref int visTileZ)
       {
          // this function is intended for leaf nodes only, assert if not
          System.Diagnostics.Debug.Assert(m_kids == null);

          

          int maxX = Math.Min(mDesc.mMaxXTile + 1, TerrainGlobals.getTerrain().getDesc().mNumXVerts - 2);
          int maxZ = Math.Min(mDesc.mMaxZTile + 1, TerrainGlobals.getTerrain().getDesc().mNumZVerts - 2);
          bool hit = false;
          float closestDist = float.MaxValue;
          for (int x = mDesc.mMinXTile; x <= maxX; x++)
          {
             for (int z = mDesc.mMinZTile; z <= maxZ; z++)
             {
                bool tHit = false;
                Vector3 pt = Vector3.Empty;

                int index = x * TerrainGlobals.getTerrain().getNumXVerts() + z;



                TerrainGlobals.getTerrain().getPos(ref verts[0], x, z); verts[0].Y = TerrainGlobals.getTerrain().getSimOverrideHeight(x, z);
                TerrainGlobals.getTerrain().getPos(ref verts[1], x, z + 1); verts[1].Y = TerrainGlobals.getTerrain().getSimOverrideHeight(x, z+1);
                TerrainGlobals.getTerrain().getPos(ref verts[2], x + 1, z + 1); verts[2].Y = TerrainGlobals.getTerrain().getSimOverrideHeight(x+1, z+1);

                
                tHit |= BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt);
                if (tHit)
                {
                   Vector3 vec = pt - origin;
                   //ensure this hit point is the closest to the origin
                   float len = vec.Length();
                   if (len < closestDist)
                   {
                      closestDist = len;
                      intersectionPt = pt;
                      visTileX = x;
                      visTileZ = z;

                      // TODO: find the actual interpolated normal here instead of the normal of one of the points
                      // in the triangle
                      intersectionNormal = TerrainGlobals.getTerrain().getNormal(x, z);
                   }
                   hit = true;
                }

                // Check bottom triangle
                verts[1] = verts[2];
                TerrainGlobals.getTerrain().getPos(ref verts[2], x + 1, z); verts[2].Y = TerrainGlobals.getTerrain().getSimOverrideHeight(x+1, z);
                tHit |= BMathLib.raySegmentIntersectionTriangle(verts, ref origin, ref dir, false, ref pt);
                if (tHit)
                {
                   Vector3 vec = pt - origin;
                   //ensure this hit point is the closest to the origin
                   float len = vec.Length();
                   if (len < closestDist)
                   {
                      closestDist = len;
                      intersectionPt = pt;

                      visTileX = x;
                      visTileZ = z;

                      // TODO: find the actual interpolated normal here instead of the normal of one of the points
                      // in the triangle
                      intersectionNormal = TerrainGlobals.getTerrain().getNormal(x, z);
                   }
                   hit = true;
                }

                //verts = null;
             }
          }
          return (hit);
       }

    

       public void getSphereIntersection(List<int> indexes, ref Vector3 center, float radius)
       {
          if (m_kids == null)
          {
             int numZVerts = TerrainGlobals.getTerrain().getNumZVerts();

             if (BMathLib.sphereAABBIntersect(center, radius, mDesc.m_min, mDesc.m_max))
             {
                for (int x = mDesc.mMinXTile; x <= mDesc.mMaxXTile; x++)
                {
                   for (int z = mDesc.mMinZTile; z <= mDesc.mMaxZTile; z++)
                   {
                      Vector3 vert = TerrainGlobals.getTerrain().getPos(x, z);

                      if (BMathLib.pointSphereIntersect(ref center, radius, ref vert))
                      {
                         int indx = x * numZVerts + z;
                         indexes.Add(indx);
                      }
                   }
                }
             }
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i]  != null)
                  m_kids[i].getSphereIntersection(indexes, ref center, radius);
          }
       }
       public void getSphereIntersection(List<BTerrainQuadNode> nodes, ref Vector3 center, float radius)
       {
          if (m_kids == null)
          {
             if (BMathLib.sphereAABBIntersect(center, radius, mDesc.m_min, mDesc.m_max))
             {
                nodes.Add(this);
             }
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i]  != null)
                  m_kids[i].getSphereIntersection(nodes, ref center, radius);
          }
       }

       public void getCylinderIntersection(List<int> indexes, ref Vector3 center, float radius)
       {
          if (m_kids == null)
          {
             int numZVerts = TerrainGlobals.getTerrain().getNumZVerts();


             Vector3 min = center;
             Vector3 max = center;
             min.X -= radius; min.Y = float.MinValue; min.Z -= radius;
             max.X += radius; max.Y = float.MaxValue; max.Z += radius;

             if (BMathLib.aabbsIntersect(ref min, ref max, ref mDesc.m_min, ref mDesc.m_max))
             {
                for (int x = mDesc.mMinXTile; x <= mDesc.mMaxXTile; x++)
                {
                   for (int z = mDesc.mMinZTile; z <= mDesc.mMaxZTile; z++)
                   {
                      Vector3 vert = TerrainGlobals.getTerrain().getPos(x, z);

                      if (BMathLib.pointCylinderIntersect(ref center, radius, ref vert))
                      {
                         int indx = x * numZVerts + z;
                         indexes.Add(indx);
                      }
                   }
                }
             }
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].getCylinderIntersection(indexes, ref center, radius);
          }
       }

       public void getCylinderIntersection(List<BTerrainQuadNode> nodes, ref Vector3 center, float radius)
       {
          if (m_kids == null)
          {
             Vector3 min = center;
             Vector3 max = center;
             min.X -= radius; min.Y = float.MinValue; min.Z -= radius;
             max.X += radius; max.Y = float.MaxValue; max.Z += radius;

             if (BMathLib.aabbsIntersect(ref min, ref max, ref mDesc.m_min, ref mDesc.m_max))
             {
                nodes.Add(this);
             }
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].getCylinderIntersection(nodes, ref center, radius);
          }
       }


       public void getCylinderIntersectionSkirt(List<int> indexes, ref Vector3 center, float radius)
       {
          if (m_kids == null)
          {
             int numSkirtZVerts = TerrainGlobals.getTerrain().getTotalSkirtZVerts();


             Vector3 min = center;
             Vector3 max = center;
             min.X -= radius; min.Y = float.MinValue; min.Z -= radius;
             max.X += radius; max.Y = float.MaxValue; max.Z += radius;

             if (BMathLib.aabbsIntersect(ref min, ref max, ref mDesc.m_min, ref mDesc.m_max))
             {
                int minXTile = mDesc.mMinXTile / 8;
                int maxXTile = mDesc.mMaxXTile / 8;
                int minZTile = mDesc.mMinZTile / 8;
                int maxZTile = mDesc.mMaxZTile / 8;

                for (int x = minXTile; x <= maxXTile; x++)
                {
                   for (int z = minZTile; z <= maxZTile; z++)
                   {
                      Vector3 vert = TerrainGlobals.getTerrain().getSkirtPos(x, z);

                      if (BMathLib.pointCylinderIntersect(ref center, radius, ref vert))
                      {
                         int indx = x * numSkirtZVerts + z;
                         indexes.Add(indx);
                      }
                   }
                }
             }
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].getCylinderIntersectionSkirt(indexes, ref center, radius);
          }
       }

        public void getAABBIntersection(List<BTerrainQuadNode> nodes, ref Vector3 bbMin, ref Vector3 bbMax)
        {
            if (m_kids == null)
            {
                if (BMathLib.aabbsIntersect(ref mDesc.m_min, ref mDesc.m_max, ref bbMin, ref bbMax))
                {
                    nodes.Add(this);
                }
            }
            else
            {
                for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                   if (m_kids[i]  != null)
                     m_kids[i].getAABBIntersection(nodes, ref bbMin, ref bbMax);
            }
        }
       public void getBoxIntersection(List<BTerrainQuadNode> nodes, Vector3[] points)
       {
          if (m_kids == null)
          {
             if (BMathLib.eVolumeIntersectionType.cVIT_Empty != BMathLib.boxAABBIntersect(mDesc.m_min, mDesc.m_max, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]))
             {
                nodes.Add(this);
             }
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].getBoxIntersection(nodes, points);
          }
       }
       public void getAddBoxIntersectionVertsToSelection(Vector3[] points,bool delFromSet)
       {
          if (m_kids == null)
          {
            
             Vector3 c = Vector3.Multiply(mDesc.m_min + mDesc.m_max, 0.5f);
             float r = Vector3.Length(mDesc.m_min - mDesc.m_max) * 0.5f;
             BMathLib.eVolumeIntersectionType intType = BMathLib.boxSphereIntersect(c,r, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]);
         //    BMathLib.eVolumeIntersectionType intType = BMathLib.boxAABBIntersect(mDesc.m_min, mDesc.m_max, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]);
             if (intType == BMathLib.eVolumeIntersectionType.cVIT_Empty)
                return;
            
             Matrix worldViewProjection = Matrix.Identity;
             if (TerrainGlobals.getEditor().mbIsDoingDragImage)
             {
                worldViewProjection = BRenderDevice.getDevice().Transform.World * BRenderDevice.getDevice().Transform.View * BRenderDevice.getDevice().Transform.Projection;
             }

             if (intType == BMathLib.eVolumeIntersectionType.cVIT_Contains)
             {
                 for (int x = mDesc.mMinXVert; x < mDesc.mMaxXVert; x++)
                {
                   for (int z = mDesc.mMinZVert; z < mDesc.mMaxZVert; z++)
                   {
                      float aval=delFromSet?0:TerrainGlobals.getEditor().getDragMaskTextureValue(x,z, ref worldViewProjection);
                      Masking.addSelectedVert(x, z,aval );
                   }
                }
            
             }
            else 
             {
                for (int x = mDesc.mMinXVert; x < mDesc.mMaxXVert; x++)
                {
                   for (int z = mDesc.mMinZVert; z < mDesc.mMaxZVert; z++)
                   {
                      Vector3 pt = TerrainGlobals.getTerrain().getPostDeformPos(x, z);

                      if (BMathLib.pointBoxIntersect(ref points[0], ref  points[1], ref points[2], ref points[3], ref points[4],
                                                     ref points[5], ref points[6], ref points[7], ref pt))
                      {
                         float aval=delFromSet?0:TerrainGlobals.getEditor().getDragMaskTextureValue(x, z, ref worldViewProjection);
                         Masking.addSelectedVert(x, z, aval);
                      }
                      
                   }
                }
             }
              
          }
          else
          {
             BMathLib.eVolumeIntersectionType intType = BMathLib.boxAABBIntersect(mDesc.m_min, mDesc.m_max, points[0], points[1], points[2], points[3], points[4], points[5], points[6], points[7]);
             if (intType == BMathLib.eVolumeIntersectionType.cVIT_Empty)
                return;

             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].getAddBoxIntersectionVertsToSelection(points, delFromSet);
          }
       }
       

       public void getTileBoundsIntersection(List<BTerrainQuadNode> nodes, int minX, int maxX, int minZ, int maxZ)
       {
          if (m_kids == null)
          {
             // test X axis
             if ((minX > mDesc.mMaxXVert) || (maxX < mDesc.mMinXVert))
               return;

             // test Z axis
             if ((minZ > mDesc.mMaxZVert) || (maxZ < mDesc.mMinZVert))
               return;

             // add (since there is overlap)
             nodes.Add(this);
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i]  != null)
                m_kids[i].getTileBoundsIntersection(nodes, minX, maxX, minZ, maxZ);
          }
       }

       public void getVertBoundsIntersection(List<BTerrainQuadNode> nodes, int minX, int maxX, int minZ, int maxZ)
       {
          if (m_kids == null)
          {
             // test X axis
             if ((minX > mDesc.mMaxXVert) || (maxX < mDesc.mMinXVert))
                return;

             // test Z axis
             if ((minZ > mDesc.mMaxZVert) || (maxZ < mDesc.mMinZVert))
                return;

             // add (since there is overlap)
             nodes.Add(this);
          }
          else
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].getVertBoundsIntersection(nodes, minX, maxX, minZ, maxZ);
          }
       }
       #endregion

        #region DEFORMATIONS & BBs
         private void CreateBB()
        {
           BTerrainDesc desc;
           desc = TerrainGlobals.getTerrain().getDesc();

           mDesc.m_min.X = mDesc.m_minPostDeform.X = desc.mWorldSizeX;
           mDesc.m_min.Y = mDesc.m_minPostDeform.Y = 1000;
           mDesc.m_min.Z = mDesc.m_minPostDeform.Z = desc.mWorldSizeZ;

           mDesc.m_max.X = mDesc.m_maxPostDeform.X = -desc.mWorldSizeX;
           mDesc.m_max.Y = mDesc.m_maxPostDeform.Y = -1000;
           mDesc.m_max.Z = mDesc.m_maxPostDeform.Z = -desc.mWorldSizeZ;

           if (m_kids != null)
           {
              for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
              {
                 BTerrainQuadNodeDesc kDesc;
                 if (m_kids[i] == null)
                    continue;
                 kDesc = m_kids[i].getDesc();
                 if (kDesc.m_min.X < mDesc.m_min.X) mDesc.m_min.X = mDesc.m_minPostDeform.X = kDesc.m_min.X;
                 if (kDesc.m_min.Y < mDesc.m_min.Y) mDesc.m_min.Y = mDesc.m_minPostDeform.Y = kDesc.m_min.Y;
                 if (kDesc.m_min.Z < mDesc.m_min.Z) mDesc.m_min.Z = mDesc.m_minPostDeform.Z = kDesc.m_min.Z;

                 if (kDesc.m_max.X > mDesc.m_max.X) mDesc.m_max.X = mDesc.m_maxPostDeform.X = kDesc.m_max.X;
                 if (kDesc.m_max.Y > mDesc.m_max.Y) mDesc.m_max.Y = mDesc.m_maxPostDeform.Y = kDesc.m_max.Y;
                 if (kDesc.m_max.Z > mDesc.m_max.Z) mDesc.m_max.Z = mDesc.m_maxPostDeform.Z = kDesc.m_max.Z;
              }
           }
           else
           {
              for (uint x = 0; x < mDesc.mMaxXVert - mDesc.mMinXVert; x++)
              {
                 for (uint z = 0; z < mDesc.mMaxZVert - mDesc.mMinZVert; z++)
                 {
                    Vector3 pos = TerrainGlobals.getTerrain().getPos(mDesc.mMinXVert + x, mDesc.mMinZVert + z);

                    if (pos.X < mDesc.m_min.X) mDesc.m_min.X = mDesc.m_minPostDeform.X = pos.X;
                    if (pos.Y < mDesc.m_min.Y) mDesc.m_min.Y = mDesc.m_minPostDeform.Y = pos.Y;
                    if (pos.Z < mDesc.m_min.Z) mDesc.m_min.Z = mDesc.m_minPostDeform.Z = pos.Z;

                    if (pos.X > mDesc.m_max.X) mDesc.m_max.X = mDesc.m_maxPostDeform.X = pos.X;
                    if (pos.Y > mDesc.m_max.Y) mDesc.m_max.Y = mDesc.m_maxPostDeform.Y = pos.Y;
                    if (pos.Z > mDesc.m_max.Z) mDesc.m_max.Z = mDesc.m_maxPostDeform.Z = pos.Z;
                 }
              }
           }
        }
         private void CreateBBPostDeform()
       {
          BTerrainDesc desc;
          desc = TerrainGlobals.getTerrain().getDesc();

          mDesc.m_minPostDeform.X = desc.mWorldSizeX;
          mDesc.m_minPostDeform.Y = 1000;
          mDesc.m_minPostDeform.Z = desc.mWorldSizeZ;

          mDesc.m_maxPostDeform.X = -desc.mWorldSizeX;
          mDesc.m_maxPostDeform.Y = -1000;
          mDesc.m_maxPostDeform.Z = -desc.mWorldSizeZ;

          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
             {
                BTerrainQuadNodeDesc kDesc;
                if (m_kids[i] == null)
                   continue;
                kDesc = m_kids[i].getDesc();
                if (kDesc.m_minPostDeform.X < mDesc.m_minPostDeform.X) mDesc.m_minPostDeform.X = kDesc.m_minPostDeform.X;
                if (kDesc.m_minPostDeform.Y < mDesc.m_minPostDeform.Y) mDesc.m_minPostDeform.Y = kDesc.m_minPostDeform.Y;
                if (kDesc.m_minPostDeform.Z < mDesc.m_minPostDeform.Z) mDesc.m_minPostDeform.Z = kDesc.m_minPostDeform.Z;

                if (kDesc.m_maxPostDeform.X > mDesc.m_maxPostDeform.X) mDesc.m_maxPostDeform.X = kDesc.m_maxPostDeform.X;
                if (kDesc.m_maxPostDeform.Y > mDesc.m_maxPostDeform.Y) mDesc.m_maxPostDeform.Y = kDesc.m_maxPostDeform.Y;
                if (kDesc.m_maxPostDeform.Z > mDesc.m_maxPostDeform.Z) mDesc.m_maxPostDeform.Z = kDesc.m_maxPostDeform.Z;
             }
          }
          else
          {
             for (uint x = 0; x < mDesc.mMaxXVert - mDesc.mMinXVert; x++)
             {
                for (uint z = 0; z < mDesc.mMaxZVert - mDesc.mMinZVert; z++)
                {
                   Vector3 pos = TerrainGlobals.getTerrain().getPostDeformPos(mDesc.mMinXVert + x, mDesc.mMinZVert + z);

                   if (pos.X < mDesc.m_minPostDeform.X) mDesc.m_minPostDeform.X = pos.X;
                   if (pos.Y < mDesc.m_minPostDeform.Y) mDesc.m_minPostDeform.Y = pos.Y;
                   if (pos.Z < mDesc.m_minPostDeform.Z) mDesc.m_minPostDeform.Z = pos.Z;

                   if (pos.X > mDesc.m_maxPostDeform.X) mDesc.m_maxPostDeform.X = pos.X;
                   if (pos.Y > mDesc.m_maxPostDeform.Y) mDesc.m_maxPostDeform.Y = pos.Y;
                   if (pos.Z > mDesc.m_maxPostDeform.Z) mDesc.m_maxPostDeform.Z = pos.Z;
                }
             }
          }
       }
         public void flushBrushDeformations()
       {
          BTerrainDesc desc;
          desc = TerrainGlobals.getTerrain().getDesc();

          mDesc.m_min.X = mDesc.m_minPostDeform.X;
          mDesc.m_min.Y = mDesc.m_minPostDeform.Y;
          mDesc.m_min.Z = mDesc.m_minPostDeform.Z;
          mDesc.m_max.X = mDesc.m_maxPostDeform.X;
          mDesc.m_max.Y = mDesc.m_maxPostDeform.Y;
          mDesc.m_max.Z = mDesc.m_maxPostDeform.Z;   

          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
             {
                if (m_kids[i] != null)
                  m_kids[i].flushBrushDeformations();
             }
          }
       }
        #endregion




      #region EDITOR SPECIFIC
       /*
       public BTerrainTexturingDataHandle getTextureData()
       {
          return mTextureData;
       }
       */
       public BTerrainTexturingDataHandle getTextureData(int texturingLODLevel)
       {
          return mTextureDataPerLOD[texturingLODLevel];
       }


       public void clearVisibleDatHandle()
       {
          if (m_kids != null)
          {
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
                if (m_kids[i] != null)
                   m_kids[i].clearVisibleDatHandle();
          }
          else
          {
             /*
             if (mVisualDataIndx != -1)
             {
                TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndx);
                mVisualDataIndx = -1;
             }
             */
             for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
             {
                if (mVisualDataIndxPerLOD[i] != -1)
                {
                   TerrainGlobals.getVisual().freeVisualHandle(mVisualDataIndxPerLOD[i]);
                   mVisualDataIndxPerLOD[i] = -1;
                }
             }
          }
       }

       public BTerrainQuadNode copy()
       {
          BTerrainQuadNode newNode = new BTerrainQuadNode();
          newNode.mDesc = mDesc;

          if (m_kids != null)
          {
             BTerrainQuadNode[] kidsTemp = new BTerrainQuadNode[(int)ekidEnum.cNumTerrainKids];
             for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
             {
                if (m_kids[i]  != null)
                 kidsTemp[i] = m_kids[i].copy();
             }
             newNode.setKids(kidsTemp);
          }

          /*
          newNode.mVisualDataIndx = mVisualDataIndx;
          */
          for (int lod = 0; lod < (int)BTerrainVisual.eLODLevel.cLODCount; lod++)
          {
             newNode.mVisualDataIndxPerLOD[lod] = mVisualDataIndxPerLOD[lod];
          }

          newNode.mDirty = true;

          /*
          if (mTextureData != null)
          {
             //newNode.mTextureData.Dispose();
             newNode.mTextureData = mTextureData.copy();
          }
          */

          for (int lod = 0; lod < Terrain.BTerrainTexturing.cMaxNumLevels; lod++)
          {
             if (mTextureDataPerLOD[lod] != null)
             {
                //newNode.mTextureDataPerLOD[lod].Dispose();
                newNode.mTextureDataPerLOD[lod] = mTextureDataPerLOD[lod].copy();
             }
          }

          return newNode;

       }
       public override int GetHashCode()
       {
          //return base.GetHashCode();
          //return mDesc.GetHashCode();
          return mDesc.mMaxXTile * 4096 + mDesc.mMaxZTile;
       }
       public void DetachFromVis()
       {
          /*
          mVisualDataIndx = -1;
          */
          for (int i = 0; i < (int)BTerrainVisual.eLODLevel.cLODCount; i++)
          {
             mVisualDataIndxPerLOD[i] = -1;
          }
       }

       public void merge(Dictionary<int, BTerrainQuadNode> mergeData, int searchDepth)
       {
          searchDepth--;
          if ((m_kids == null) || mergeData.Count == 0) 
             return;
          for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
          {
             if(searchDepth > 0)
             {
                if(m_kids[i] != null)
                  m_kids[i].merge(mergeData,searchDepth);
             }       
          }
          for (int i = 0; i < (int)ekidEnum.cNumTerrainKids; i++)
          {
             if (mergeData.ContainsKey(m_kids[i].GetHashCode()))
             {
                m_kids[i].DetachFromVis();
                m_kids[i] = mergeData[m_kids[i].GetHashCode()];
                mergeData.Remove(m_kids[i].GetHashCode());


                //m_kids[i].mVisualDataIndx = -1;
                //m_kids = null;
             }
          }            


       }

       protected void setDesc(BTerrainQuadNodeDesc desc) { mDesc = desc; }
       protected void setKids(BTerrainQuadNode[] kids) { m_kids = kids; }
      #endregion

       //---------------------------------------MEMBERS

       //---------------------------------------
       static public uint getMaxNodeWidth()
       {
          return cMaxWidth;
       }
       static public uint getMaxNodeDepth()
       {
          return cMaxWidth;
       }


        private BTerrainQuadNodeDesc mDesc;

        private enum ekidEnum
        {
            cTopLeft = 0,
            cTopRight = 1,
            cBotLeft = 2,
            cBotRight = 3,
            cNumTerrainKids
        };
        public BTerrainQuadNode[] m_kids;

        //public int mVisualDataIndx=-1;
        public int[] mVisualDataIndxPerLOD = new int[(int)Terrain.BTerrainVisual.eLODLevel.cLODCount];
        
        //BTerrainTexturingDataHandle mTextureData = null;
       BTerrainTexturingDataHandle[] mTextureDataPerLOD = new BTerrainTexturingDataHandle[Terrain.BTerrainTexturing.cMaxNumLevels];

        public bool mDirty = false;

        public static uint cMaxWidth = 64;
        public static uint cMaxHeight = 64;
       

       public BTerrainLayerContainer mLayerContainer = null;

    };
    #endregion
}
