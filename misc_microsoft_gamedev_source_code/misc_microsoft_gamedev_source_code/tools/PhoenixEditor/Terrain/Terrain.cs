
//#define OLD_computeBasis
#define NEW_computeBasis

//#define OLD_computeBasisCurr
#define NEW_computeBasisCurr

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
using Rendering;
using EditorCore;


namespace Terrain
{


#region Terrain


//-----------------------------------------------------
public struct BTerrainDesc
{
   public int mNumXVerts;
   public int mNumZVerts;
   public float mTileScale;
   public float mWorldSizeX;
   public float mWorldSizeZ;


   public int mNumSkirtXVertsPerQuadrant;
   public int mNumSkirtZVertsPerQuadrant;
   public int mTotalSkirtXVerts;
   public int mTotalSkirtZVerts;

   static public int mSkirtFactor = 128;//128;//8;//32;       // Number of world verts per skirt vert.

};

   

//-----------------------------------------------------
public class BTerrain
{

   //---------------------------------------
   public BTerrain()
   {
      init();
   }
   //---------------------------------------
   ~BTerrain()
   {
      destroy();
   }
   //---------------------------------------
   public void init()
   {   
   }
   public void destroy()
   {
      mFrustum = null;
      tempFaceNormalArray = null;
      mQuadNodeRoot = null;
      mLeafQuadNodes = null;

      TerrainGlobals.getEditor().destroy();
      TerrainGlobals.getTexturing().destroy();
      TerrainGlobals.getVisual().destroy();
      TerrainGlobals.getRender().destroy();
      LightManager.destroy();
   }

   
   #region creation
   
   private bool postCreate()
   {

      //Visual Interface
      TerrainGlobals.getVisual().init(TerrainGlobals.getTerrainFrontEnd().NumTerrainHandles);
      TerrainGlobals.getRender().init();
      TerrainGlobals.getTexturing().init();

      mFrustum = new BFrustum();

      //create a temp array for our basis calcs
      //tempFaceNormalArray = new Vector3[(BTerrainQuadNode.getMaxNodeDepth() + 1) * (BTerrainQuadNode.getMaxNodeWidth() + 1)];
      //tempFaceNormalArray = new Vector3[mDesc.mNumXVerts * mDesc.mNumZVerts];
      tempFaceNormalArray = null;

      //CREATE OUR QUADNODE (has to be done after visual interfaces has been initalized..
      mQuadNodeRoot = new BTerrainQuadNode();
      mQuadNodeRoot.createFromHeightMap(0, 0, mDesc.mNumXVerts, mDesc.mNumZVerts, BRenderDevice.getDevice(), ref mLeafQuadNodes);

      int numXChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
      LightManager.init(numXChunks, numXChunks);

      TerrainGlobals.getEditor().postCreate();

      using (PerfSection p2 = new PerfSection("postCreate.UpdateMemory"))
      {
         //TerrainGlobals.getTerrainFrontEnd().updateMemoryEstimate(false, true);
      }
      return true;
   }
   public bool createBlank(TerrainCreationParams param)
   {
      mDesc.mTileScale = param.mVisTileSpacing;
      mDesc.mNumXVerts = param.mNumVisXVerts;
      mDesc.mNumZVerts = param.mNumVisZVerts;
      mDesc.mWorldSizeX = mDesc.mNumXVerts * mDesc.mTileScale;
      mDesc.mWorldSizeZ = mDesc.mNumZVerts * mDesc.mTileScale;

      mDesc.mNumSkirtXVertsPerQuadrant = param.mNumVisXVerts / BTerrainDesc.mSkirtFactor + 1;
      mDesc.mNumSkirtZVertsPerQuadrant = param.mNumVisZVerts / BTerrainDesc.mSkirtFactor + 1;
      mDesc.mTotalSkirtXVerts = param.mNumVisXVerts / BTerrainDesc.mSkirtFactor * 3 + 1;
      mDesc.mTotalSkirtZVerts = param.mNumVisXVerts / BTerrainDesc.mSkirtFactor * 3 + 1;

      mFrustum = null;
      tempFaceNormalArray = null;
      mQuadNodeRoot = null;
       
      CoreGlobals.mPlayableBoundsMinX = 0;
      CoreGlobals.mPlayableBoundsMinZ = 0;
      CoreGlobals.mPlayableBoundsMaxX = mDesc.mNumXVerts;
      CoreGlobals.mPlayableBoundsMaxZ = mDesc.mNumZVerts;


      TerrainGlobals.getEditor().init(param);

      postCreate();
      return true;
   }
   public void clear()
   {
   }
   public void recreate()
   {
      //TerrainGlobals.getEditor().init();
      getQuadNodeRoot().clearVisibleDatHandle();

      TerrainGlobals.getVisual().init(TerrainGlobals.getTerrainFrontEnd().NumTerrainHandles);
      TerrainGlobals.getTexturing().init();

      rebuildDirty(BRenderDevice.getDevice());
      getQuadNodeRoot().clearVisibleDatHandle();
   }
   public bool createFromTED(TerrainCreationParams param, Vector3[] dp, byte[] av, float[] aov,
                     JaggedContainer<int> mSimBuildableOverrideValues, 
                     JaggedContainer<int> mSimPassableOverrideValues, 
                     JaggedContainer<float> mSimHeightOverrideValues, 
                     JaggedContainer<byte> tessOverride,
                     JaggedContainer<float> cameraHeightOverride,
                     JaggedContainer<float> flightHeightOverride,
                     JaggedContainer<int> mSimFloodPassableOverrideValues, 
                     JaggedContainer<int> mSimScarabPassableOverrideValues,
                     JaggedContainer<int> mSimTileTypeOverrideValues)
   {
      mDesc.mTileScale = param.mVisTileSpacing;
      mDesc.mNumXVerts = param.mNumVisXVerts;
      mDesc.mNumZVerts = param.mNumVisZVerts;
      mDesc.mWorldSizeX = mDesc.mNumXVerts * mDesc.mTileScale;
      mDesc.mWorldSizeZ = mDesc.mNumZVerts * mDesc.mTileScale;

      mDesc.mNumSkirtXVertsPerQuadrant = param.mNumVisXVerts / BTerrainDesc.mSkirtFactor + 1;
      mDesc.mNumSkirtZVertsPerQuadrant = param.mNumVisZVerts / BTerrainDesc.mSkirtFactor + 1;
      mDesc.mTotalSkirtXVerts = param.mNumVisXVerts / BTerrainDesc.mSkirtFactor * 3 + 1;
      mDesc.mTotalSkirtZVerts = param.mNumVisXVerts / BTerrainDesc.mSkirtFactor * 3 + 1;

      mFrustum = null;
      tempFaceNormalArray = null;
      mQuadNodeRoot = null;

      TerrainGlobals.getEditor().initFromMem(param, dp, av,aov, 
         mSimBuildableOverrideValues,
         mSimPassableOverrideValues, 
         mSimHeightOverrideValues, 
         tessOverride, 
         cameraHeightOverride,
         flightHeightOverride,
         mSimFloodPassableOverrideValues,
         mSimScarabPassableOverrideValues, mSimTileTypeOverrideValues);

      postCreate();
      return true;
   }
  

   #endregion

   #region Normals Calc
   static private Vector3[] tempFaceNormalArray;
   static private int tempArrayIndex(int x, int z, int minX, int maxX, int minZ, int maxZ)
   {
      return ((x - minX) * (maxZ - minZ + 1) + (z - minZ));
   }

#if NEW_computeBasis
   static private Vector3 _op_getAbsPosFromRelInput = Vector3.Empty;
   static public Vector3 getAbsPosFromRelInput(Vector3[] pos, int numXVerts, int x, int z, float tileScale)
   {

      if (x >= numXVerts)
         x = numXVerts - 1;
      if (z >= numXVerts)
         z = numXVerts - 1;

      int indx = x * (int)numXVerts + z;

      //Vector3 relPos = pos[indx];
      //return new Vector3(relPos.X + x * tileScale, relPos.Y, relPos.Z + z * tileScale);
      _op_getAbsPosFromRelInput.X = pos[indx].X + x * tileScale;
      _op_getAbsPosFromRelInput.Y = pos[indx].Y;
      _op_getAbsPosFromRelInput.Z = pos[indx].Z + z * tileScale;
      return _op_getAbsPosFromRelInput;
   }
#endif
#if OLD_computeBasis
   static public Vector3 getAbsPosFromRelInput(Vector3[] pos, int numXVerts, int x, int z, float tileScale)
   {

      if (x >= numXVerts) x = numXVerts - 1;
      if (z >= numXVerts) z = numXVerts - 1;

      int indx = x * (int)numXVerts + z;

      Vector3 relPos = pos[indx];
      return new Vector3(relPos.X + x * tileScale, relPos.Y, relPos.Z + z * tileScale);
   }
#endif
   static public void computeBasis(Vector3 []relPos, Vector3 []normals,float tileScale,int numXVerts, int minXVertex, int maxXVertex, int minZVertex, int maxZVertex)
   {
      
      unsafe
      {
         //using (PerfSection p = new PerfSection("computeBasis"))
         {
#if NEW_computeBasis
            //bounds / reality check
            if (minXVertex < 0) minXVertex = 0;
            if (minZVertex < 0) minZVertex = 0;
            if (maxXVertex >= numXVerts) maxXVertex = numXVerts - 1;
            if (maxZVertex >= numXVerts) maxZVertex = numXVerts - 1;

            int minXVertexFace = (minXVertex > 0) ? minXVertex - 1 : minXVertex;
            int minZVertexFace = (minZVertex > 0) ? minZVertex - 1 : minZVertex;
            int maxXVertexFace = (maxXVertex < numXVerts - 1) ? maxXVertex + 1 : maxXVertex;
            int maxZVertexFace = (maxZVertex < numXVerts - 1) ? maxZVertex + 1 : maxZVertex;


            //Vector3[] normals = TerrainGlobals.getEditor().getNormals();

            int size = (maxZVertexFace - minZVertexFace + 1) * (maxXVertexFace - minXVertexFace + 1);
            if (tempFaceNormalArray == null || tempFaceNormalArray.Length < size)
            {
               tempFaceNormalArray = new Vector3[size];
            }
            int counter = 0;

            //fixed (Vector3* pRelPos = relPos)
            //{
            //}

            Vector3 empty = Vector3.Empty;
            // Compute all face normals, this is for temporary use only
            for (int x = minXVertexFace; x <= maxXVertexFace; x++)
            {
               for (int z = minZVertexFace; z <= maxZVertexFace; z++)
               {
                  // Compute normal
                  Vector3 normal = empty;//new Vector3(0.0f, 0.0f, 0.0f);
                  Vector3 vec1, vec2;
                  Vector3 a, b, c;   
        
                  int indx = x * numXVerts + z;
                  float tilex = x * tileScale;
                  float tilez = z * tileScale;
                  Vector3 x0z0 = relPos[indx];

                  //a = getAbsPosFromRelInput(relPos, numXVerts, x, z, tileScale);
                  float a_X, a_Y, a_Z;


                  a_X = x0z0.X + tilex;
                  a_Y = x0z0.Y;
                  a_Z = x0z0.Z + tilez;

                  float vec1_X, vec1_Y, vec1_Z, vec2_X, vec2_Y, vec2_Z, normal_X=0,normal_Y=0,normal_Z=0;

                  if ((x < numXVerts - 1) && (z < numXVerts - 1))
                  {

                     //b = getAbsPosFromRelInput(relPos, numXVerts, x + 1, z, tileScale); //getPostDeformPos(x + 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z + 1, tileScale);// getPostDeformPos(x, z + 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx + numXVerts];
                     vec2 = relPos[indx + 1];

                     vec1_X = vec1.X + tilex + tileScale - a_X;
                     vec1_Y = vec1.Y - a_Y;
                     vec1_Z = vec1.Z + tilez - a_Z;
                     vec2_X = vec2.X + tilex - a_X;
                     vec2_Y = vec2.Y - a_Y;
                     vec2_Z = vec2.Z + tilez + tileScale - a_Z;

                     normal_X += vec2_Y * vec1_Z - vec2_Z * vec1_Y;
                     normal_Y += vec2_Z * vec1_X - vec2_X * vec1_Z;
                     normal_Z += vec2_X * vec1_Y - vec2_Y * vec1_X;


                     //normal += BMathLib.Cross(vec2, vec1);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);
                  }

                  if ((x > 0) && (z < numXVerts - 1))
                  {
                     //b = getAbsPosFromRelInput(relPos, numXVerts, x - 1, z, tileScale); //getPostDeformPos(x - 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z + 1, tileScale); //getPostDeformPos(x, z + 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx - numXVerts];
                     vec2 = relPos[indx + 1];
                     vec1_X = vec1.X + tilex - tileScale - a_X;
                     vec1_Y = vec1.Y - a_Y;
                     vec1_Z = vec1.Z + tilez - a_Z;
                     vec2_X = vec2.X + tilex - a_X;
                     vec2_Y = vec2.Y - a_Y;
                     vec2_Z = vec2.Z + tilez + tileScale - a_Z;


                     normal_X += vec1_Y * vec2_Z - vec1_Z * vec2_Y;
                     normal_Y += vec1_Z * vec2_X - vec1_X * vec2_Z;
                     normal_Z += vec1_X * vec2_Y - vec1_Y * vec2_X;

                     //normal += BMathLib.Cross(vec1, vec2);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);

                  }

                  if ((x < numXVerts - 1) && (z > 0))
                  {
                     //b = getAbsPosFromRelInput(relPos, numXVerts, x + 1, z, tileScale); //getPostDeformPos(x + 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z - 1, tileScale); //getPostDeformPos(x, z - 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx + numXVerts];
                     vec2 = relPos[indx - 1];
                     vec1_X = vec1.X + tilex + tileScale - a_X;
                     vec1_Y = vec1.Y - a_Y;
                     vec1_Z = vec1.Z + tilez - a_Z;
                     vec2_X = vec2.X + tilex - a_X;
                     vec2_Y = vec2.Y - a_Y;
                     vec2_Z = vec2.Z + tilez - tileScale - a_Z;

                     normal_X += vec1_Y * vec2_Z - vec1_Z * vec2_Y;
                     normal_Y += vec1_Z * vec2_X - vec1_X * vec2_Z;
                     normal_Z += vec1_X * vec2_Y - vec1_Y * vec2_X;

                     //normal += BMathLib.Cross(vec1, vec2);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);

                  }

                  if ((x > 0) && (z > 0))
                  {
                     //b = getAbsPosFromRelInput(relPos, numXVerts, x - 1, z, tileScale); //getPostDeformPos(x - 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z - 1, tileScale); //getPostDeformPos(x, z - 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx - numXVerts];
                     vec2 = relPos[indx - 1];
                     vec1_X = vec1.X + tilex - tileScale - a_X;
                     vec1_Y = vec1.Y - a_Y;
                     vec1_Z = vec1.Z + tilez - a_Z;
                     vec2_X = vec2.X + tilex - a_X;
                     vec2_Y = vec2.Y - a_Y;
                     vec2_Z = vec2.Z + tilez - tileScale - a_Z;

                     normal_X += vec2_Y * vec1_Z - vec2_Z * vec1_Y;
                     normal_Y += vec2_Z * vec1_X - vec2_X * vec1_Z;
                     normal_Z += vec2_X * vec1_Y - vec2_Y * vec1_X;

                     //normal += BMathLib.Cross(vec2, vec1);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);

                  }

                  float len = (float)Math.Sqrt((float)(normal_X * normal_X + normal_Y * normal_Y + normal_Z * normal_Z));
                  if (Math.Abs(len) > 0.0001)
                  {
                     normal_X /= len;
                     normal_Y /= len;
                     normal_Z /= len;
                  }
                  normal.X = normal_X;
                  normal.Y = normal_Y;
                  normal.Z = normal_Z;


                  //normals[x * (int)numXVerts + z] = normal;                  
                  tempFaceNormalArray[counter++] = normal;
                  //tempFaceNormalArray[counter++] = BMathLib.Normalize(normal);

       
                  //normal = BMathLib.Normalize(normal);
                  //tempFaceNormalArray[counter++] = normal;
               }
            }
#if NEW_computeBasis

   //            static private int tempArrayIndex(int x, int z, int minX, int maxX, int minZ, int maxZ)
   //{
   //   return ((x - minX) * (maxZ - minZ + 1) + (z - minZ));
   //   return ((x - minX) * (C2) + (z - minZ));
   //}
            //using (PerfSection p2 = new PerfSection("computeBasisB"))
            {
               for (int x = minXVertex; x <= maxXVertex; x++)
               {
                  for (int z = minZVertex; z <= maxZVertex; z++)
                  {
                     float normal_X,normal_Y,normal_Z;

                     // Average normal
                     Vector3 normal = tempFaceNormalArray[tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];
                     Vector3 temp;
                     normal_X = normal.X;
                     normal_Y = normal.Y;
                     normal_Z = normal.Z;

                     //int indx = tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace);
                     //int indx_Xpos = maxZVertexFace - minZVertexFace + 1;

                     //float factor = 1.0f;

                     if (x > 0)
                     {
                        //normal += tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        temp = tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;

                        normal_X = normal_X + temp.X;
                        normal_Y = normal_Y + temp.Y;
                        normal_Z = normal_Z + temp.Z;

                     }
                     if (x < numXVerts - 1)
                     {
                        //normal += tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        temp = tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;

                        normal_X = normal_X + temp.X;
                        normal_Y = normal_Y + temp.Y;
                        normal_Z = normal_Z + temp.Z;
                     }
                     if (z > 0)
                     {
                        //normal += tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        temp = tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;

                        normal_X = normal_X + temp.X;
                        normal_Y = normal_Y + temp.Y;
                        normal_Z = normal_Z + temp.Z;
                     }
                     if (z < numXVerts - 1)
                     {
                        //normal += tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        temp = tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;

                        normal_X = normal_X + temp.X;
                        normal_Y = normal_Y + temp.Y;
                        normal_Z = normal_Z + temp.Z;
                     }
                     //factor = 1.0f;
                     if (x > 0)
                     {
                        if (z > 0)
                        {
                           //normal += tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                           temp = tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;
                           normal_X = normal_X + temp.X;
                           normal_Y = normal_Y + temp.Y;
                           normal_Z = normal_Z + temp.Z;
                        }
                        if (z < numXVerts - 1)
                        {
                           //normal += tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                           temp = tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;
                           normal_X = normal_X + temp.X;
                           normal_Y = normal_Y + temp.Y;
                           normal_Z = normal_Z + temp.Z;
                        }
                     }
                     if (x < numXVerts - 1)
                     {
                        if (z > 0)
                        {
                           //normal += tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                           temp = tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;
                           normal_X = normal_X + temp.X;
                           normal_Y = normal_Y + temp.Y;
                           normal_Z = normal_Z + temp.Z;
                        }
                        if (z < numXVerts - 1)
                        {
                           //normal += tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                           temp = tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] ;
                           normal_X = normal_X + temp.X;
                           normal_Y = normal_Y + temp.Y;
                           normal_Z = normal_Z + temp.Z;
                        }
                     }
                     float len = (float)Math.Sqrt((float)(normal_X * normal_X + normal_Y * normal_Y + normal_Z * normal_Z));
                     if (Math.Abs(len) > 0.0001)
                     {
                        normal_X /= len;
                        normal_Y /= len;
                        normal_Z /= len;
                     }
                     normal.X = normal_X;
                     normal.Y = normal_Y;
                     normal.Z = normal_Z;

                     normals[x * (int)numXVerts + z] = normal;

                     //normal = BMathLib.Normalize(normal);
                     //normals[x * (int)numXVerts + z] = normal;
                  }
               }
            }

#else
            using (PerfSection p2 = new PerfSection("computeBasisB"))
            {
               for (int x = minXVertex; x <= maxXVertex; x++)
               {
                  for (int z = minZVertex; z <= maxZVertex; z++)
                  {
                     // Average normal
                     Vector3 normal = tempFaceNormalArray[tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

                     float factor = 1.0f;
                     if (x > 0)
                        normal += tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     if (x < numXVerts - 1)
                        normal += tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     if (z > 0)
                        normal += tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     if (z < numXVerts - 1)
                        normal += tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;

                     factor = 1.0f;
                     if (x > 0)
                     {
                        if (z > 0)
                           normal += tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        if (z < numXVerts - 1)
                           normal += tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     }
                     if (x < numXVerts - 1)
                     {
                        if (z > 0)
                           normal += tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        if (z < numXVerts - 1)
                           normal += tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     }


                     normal = BMathLib.Normalize(normal);
                     normals[x * (int)numXVerts + z] = normal;
                  }
               }
            }
#endif
#endif

#if NEW2_computeBasis
            //bounds / reality check
            if (minXVertex < 0) minXVertex = 0;
            if (minZVertex < 0) minZVertex = 0;
            if (maxXVertex >= numXVerts) maxXVertex = numXVerts - 1;
            if (maxZVertex >= numXVerts) maxZVertex = numXVerts - 1;

            int minXVertexFace = (minXVertex > 0) ? minXVertex - 1 : minXVertex;
            int minZVertexFace = (minZVertex > 0) ? minZVertex - 1 : minZVertex;
            int maxXVertexFace = (maxXVertex < numXVerts - 1) ? maxXVertex + 1 : maxXVertex;
            int maxZVertexFace = (maxZVertex < numXVerts - 1) ? maxZVertex + 1 : maxZVertex;


            //Vector3[] normals = TerrainGlobals.getEditor().getNormals();

            int size = (maxZVertexFace - minZVertexFace + 1) * (maxXVertexFace - minXVertexFace + 1);
            if (tempFaceNormalArray == null || tempFaceNormalArray.Length < size)
            {
               tempFaceNormalArray = new Vector3[size];
            }
            int counter = 0;

            //fixed (Vector3* pRelPos = relPos)
            //{
            //}


            // Compute all face normals, this is for temporary use only
            for (int x = minXVertexFace; x <= maxXVertexFace; x++)
            {
               for (int z = minZVertexFace; z <= maxZVertexFace; z++)
               {
                  // Compute normal
                  Vector3 normal = Vector3.Empty;//new Vector3(0.0f, 0.0f, 0.0f);
                  Vector3 vec1, vec2;
                  Vector3 a, b, c;   
        
                  int indx = x * numXVerts + z;
                  float tilex = x * tileScale;
                  float tilez = z * tileScale;
                  Vector3 x0z0 = relPos[indx];

                  //a = getAbsPosFromRelInput(relPos, numXVerts, x, z, tileScale);
                  float a_X, a_Y, a_Z;


                  a_X = x0z0.X + tilex;
                  a_Y = x0z0.Y;
                  a_Z = x0z0.Z + tilez;

                  if ((x < numXVerts - 1) && (z < numXVerts - 1))
                  {

                     //b = getAbsPosFromRelInput(relPos, numXVerts, x + 1, z, tileScale); //getPostDeformPos(x + 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z + 1, tileScale);// getPostDeformPos(x, z + 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx + numXVerts];
                     vec2 = relPos[indx + 1];

                     vec1.X = vec1.X + tilex + tileScale - a_X;
                     vec1.Y -= a_Y;
                     vec1.Z = vec1.Z + tilez - a_Z;
                     vec2.X = vec2.X + tilex - a_X;
                     vec2.Y -= a_Y;
                     vec2.Z = vec2.Z + tilez + tileScale - a_Z;


                     normal += BMathLib.Cross(vec2, vec1);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);
                  }

                  if ((x > 0) && (z < numXVerts - 1))
                  {
                     //b = getAbsPosFromRelInput(relPos, numXVerts, x - 1, z, tileScale); //getPostDeformPos(x - 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z + 1, tileScale); //getPostDeformPos(x, z + 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx - numXVerts];
                     vec2 = relPos[indx + 1];
                     vec1.X = vec1.X + tilex - tileScale - a_X;
                     vec1.Y -= a_Y;
                     vec1.Z = vec1.Z + tilez - a_Z;
                     vec2.X = vec2.X + tilex - a_X;
                     vec2.Y -= a_Y;
                     vec2.Z = vec2.Z + tilez + tileScale - a_Z;

                     normal += BMathLib.Cross(vec1, vec2);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);

                  }

                  if ((x < numXVerts - 1) && (z > 0))
                  {
                     //b = getAbsPosFromRelInput(relPos, numXVerts, x + 1, z, tileScale); //getPostDeformPos(x + 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z - 1, tileScale); //getPostDeformPos(x, z - 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx + numXVerts];
                     vec2 = relPos[indx - 1];
                     vec1.X = vec1.X + tilex + tileScale - a_X;
                     vec1.Y -= a_Y;
                     vec1.Z = vec1.Z + tilez - a_Z;
                     vec2.X = vec2.X + tilex - a_X;
                     vec2.Y -= a_Y;
                     vec2.Z = vec2.Z + tilez - tileScale - a_Z;

                     normal += BMathLib.Cross(vec1, vec2);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);

                  }

                  if ((x > 0) && (z > 0))
                  {
                     //b = getAbsPosFromRelInput(relPos, numXVerts, x - 1, z, tileScale); //getPostDeformPos(x - 1, z);
                     //c = getAbsPosFromRelInput(relPos, numXVerts, x, z - 1, tileScale); //getPostDeformPos(x, z - 1);
                     //vec1 = Vector3.Subtract(b, a);
                     //vec2 = Vector3.Subtract(c, a);

                     vec1 = relPos[indx - numXVerts];
                     vec2 = relPos[indx - 1];
                     vec1.X = vec1.X + tilex - tileScale - a_X;
                     vec1.Y -= a_Y;
                     vec1.Z = vec1.Z + tilez - a_Z;
                     vec2.X = vec2.X + tilex - a_X;
                     vec2.Y -= a_Y;
                     vec2.Z = vec2.Z + tilez - tileScale - a_Z;


                     normal += BMathLib.Cross(vec2, vec1);
                     //BMathLib.CrossRef(ref vec1, ref vec2, ref normal);

                  }

                  normal = BMathLib.Normalize(normal);
                  tempFaceNormalArray[counter++] = normal;
               }
            }
            using (PerfSection p2 = new PerfSection("computeBasisB"))
            {
               for (int x = minXVertex; x <= maxXVertex; x++)
               {
                  for (int z = minZVertex; z <= maxZVertex; z++)
                  {
                     // Average normal
                     Vector3 normal = tempFaceNormalArray[tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

                     float factor = 1.0f;
                     if (x > 0)
                        normal += tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     if (x < numXVerts - 1)
                        normal += tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     if (z > 0)
                        normal += tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     if (z < numXVerts - 1)
                        normal += tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;

                     factor = 1.0f;
                     if (x > 0)
                     {
                        if (z > 0)
                           normal += tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        if (z < numXVerts - 1)
                           normal += tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     }
                     if (x < numXVerts - 1)
                     {
                        if (z > 0)
                           normal += tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                        if (z < numXVerts - 1)
                           normal += tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                     }


                     normal = BMathLib.Normalize(normal);
                     normals[x * (int)numXVerts + z] = normal;
                  }
               }
            }
#endif

#if OLD_computeBasis
     //bounds / reality check
      if (minXVertex < 0) minXVertex = 0;
      if (minZVertex < 0) minZVertex = 0;
      if (maxXVertex >= numXVerts) maxXVertex = numXVerts - 1;
      if (maxZVertex >= numXVerts) maxZVertex = numXVerts - 1;

      int minXVertexFace = (minXVertex > 0) ? minXVertex - 1 : minXVertex;
      int minZVertexFace = (minZVertex > 0) ? minZVertex - 1 : minZVertex;
      int maxXVertexFace = (maxXVertex < numXVerts - 1) ? maxXVertex + 1 : maxXVertex;
      int maxZVertexFace = (maxZVertex < numXVerts - 1) ? maxZVertex + 1 : maxZVertex;

      
      //Vector3[] normals = TerrainGlobals.getEditor().getNormals();

      int size = (maxZVertexFace - minZVertexFace + 1) * (maxXVertexFace - minXVertexFace + 1);
      if (tempFaceNormalArray == null || tempFaceNormalArray.Length < size)
      {
         tempFaceNormalArray = new Vector3[size];
      }
      int counter = 0;

      // Compute all face normals, this is for temporary use only
      for( int x = minXVertexFace; x <= maxXVertexFace; x++)
      {
         for( int z = minZVertexFace; z <= maxZVertexFace; z++)
         {
            // Compute normal
            Vector3 normal = new Vector3(0.0f, 0.0f, 0.0f);
            Vector3 vec1, vec2;
            Vector3 a, b, c;

            a = getAbsPosFromRelInput(relPos, numXVerts, x, z, tileScale);

            if ((x < numXVerts - 1) && (z < numXVerts - 1))
               {
                  b = getAbsPosFromRelInput(relPos, numXVerts, x +1, z, tileScale); //getPostDeformPos(x + 1, z);
                  c = getAbsPosFromRelInput(relPos, numXVerts, x, z+1, tileScale);// getPostDeformPos(x, z + 1);

                  vec1 = Vector3.Subtract(b, a);
                  vec2 = Vector3.Subtract(c, a);
                  normal += BMathLib.Cross(vec2, vec1);
               }

               if ((x > 0) && (z < numXVerts - 1))
            {
               b = getAbsPosFromRelInput(relPos, numXVerts, x -1, z, tileScale); //getPostDeformPos(x - 1, z);
               c = getAbsPosFromRelInput(relPos, numXVerts, x, z+1, tileScale); //getPostDeformPos(x, z + 1);

               vec1 = Vector3.Subtract(b, a);
               vec2 = Vector3.Subtract(c, a);
               normal += BMathLib.Cross(vec1, vec2);
            }

            if ((x < numXVerts - 1) && (z > 0))
            {
               b = getAbsPosFromRelInput(relPos, numXVerts, x+1, z, tileScale); //getPostDeformPos(x + 1, z);
               c = getAbsPosFromRelInput(relPos, numXVerts, x, z-1, tileScale); //getPostDeformPos(x, z - 1);

               vec1 = Vector3.Subtract(b, a);
               vec2 = Vector3.Subtract(c, a);
               normal += BMathLib.Cross(vec1, vec2);
            }

            if ((x > 0) && (z > 0))
            {
               b = getAbsPosFromRelInput(relPos, numXVerts, x-1, z, tileScale); //getPostDeformPos(x - 1, z);
               c = getAbsPosFromRelInput(relPos, numXVerts, x, z-1, tileScale); //getPostDeformPos(x, z - 1);

               vec1 = Vector3.Subtract(b, a);
               vec2 = Vector3.Subtract(c, a);
               normal += BMathLib.Cross(vec2, vec1);
            }
            
            normal = BMathLib.Normalize(normal);
            tempFaceNormalArray[counter++] = normal;
         }
      }

      for (int x = minXVertex; x <= maxXVertex; x++)
      {
         for (int z = minZVertex; z <= maxZVertex; z++)
         {
            // Average normal
            Vector3 normal = tempFaceNormalArray[tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

            float factor = 1.0f;
            if (x > 0)
               normal += tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            if (x < numXVerts - 1)
               normal += tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            if (z > 0)
               normal += tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            if (z < numXVerts - 1)
               normal += tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;

            factor = 1.0f;
            if (x > 0)
            {
               if (z > 0)
                  normal += tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               if (z < numXVerts - 1)
                  normal += tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            }
            if (x < numXVerts - 1)
            {
               if (z > 0)
                  normal += tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               if (z < numXVerts - 1)
                  normal += tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            }


            normal = BMathLib.Normalize(normal);
            normals[x * (int)numXVerts + z] = normal;
         }
      }

#endif

         }
      }
   }

#if NEW_computeBasisCurr
   public void computeBasisCurr(int minXVertex, int maxXVertex, int minZVertex, int maxZVertex)
   {


      //bounds / reality check
      if (minXVertex < 0) minXVertex = 0;
      if (minZVertex < 0) minZVertex = 0;
      if (maxXVertex >= mDesc.mNumXVerts) maxXVertex = mDesc.mNumXVerts - 1;
      if (maxZVertex >= mDesc.mNumZVerts) maxZVertex = mDesc.mNumZVerts - 1;

      int minXVertexFace = (minXVertex > 0) ? minXVertex - 1 : minXVertex;
      int minZVertexFace = (minZVertex > 0) ? minZVertex - 1 : minZVertex;
      int maxXVertexFace = (maxXVertex < mDesc.mNumXVerts - 1) ? maxXVertex + 1 : maxXVertex;
      int maxZVertexFace = (maxZVertex < mDesc.mNumXVerts - 1) ? maxZVertex + 1 : maxZVertex;


      Vector3[] normals = TerrainGlobals.getEditor().getNormals();
      //Vector3[] defNormals = TerrainGlobals.getEditor().getCurrBrushDeformationNormals();

      int size = (maxZVertexFace - minZVertexFace + 2) * (maxXVertexFace - minXVertexFace + 2);
      if (tempFaceNormalArray == null || tempFaceNormalArray.Length < size)
      {
         tempFaceNormalArray = new Vector3[size];
      }
      int counter = 0;

      Vector3 empty = new Vector3(0.0f, 0.0f, 0.0f);
      // Compute all face normals, this is for temporary use only
      for (int x = minXVertexFace; x <= maxXVertexFace; x++)
      {
         for (int z = minZVertexFace; z <= maxZVertexFace; z++)
         {
            // Compute normal
            Vector3 normal = empty;
            Vector3 vec1, vec2;
            Vector3 a, b, c;

            a = getPostDeformPos(x, z);
            float a_X, a_Y, a_Z;
            float vec1_X, vec1_Y, vec1_Z, vec2_X, vec2_Y, vec2_Z, normal_X = 0, normal_Y = 0, normal_Z = 0;

            a_X = a.X;
            a_Y = a.Y;
            a_Z = a.Z;

            if ((x < mDesc.mNumXVerts - 1) && (z < mDesc.mNumZVerts - 1))
            {
               //b = getPostDeformPos(x + 1, z);
               //c = getPostDeformPos(x, z + 1);
               //vec1 = Vector3.Subtract(b, a);
               //vec2 = Vector3.Subtract(c, a);
               //normal += Vector3.Cross(vec2, vec1);

               vec1 = getPostDeformPos(x + 1, z);
               vec2 = getPostDeformPos(x, z + 1);

               vec1_X = vec1.X - a_X;
               vec1_Y = vec1.Y - a_Y;
               vec1_Z = vec1.Z - a_Z;
               vec2_X = vec2.X - a_X;
               vec2_Y = vec2.Y - a_Y;
               vec2_Z = vec2.Z - a_Z;

               normal_X += vec2_Y * vec1_Z - vec2_Z * vec1_Y;
               normal_Y += vec2_Z * vec1_X - vec2_X * vec1_Z;
               normal_Z += vec2_X * vec1_Y - vec2_Y * vec1_X;

            }

            if ((x > 0) && (z < mDesc.mNumZVerts - 1))
            {
               //b = getPostDeformPos(x - 1, z);
               //c = getPostDeformPos(x, z + 1);

               //vec1 = Vector3.Subtract(b, a);
               //vec2 = Vector3.Subtract(c, a);
               //normal += Vector3.Cross(vec1, vec2);

               vec1 = getPostDeformPos(x - 1, z);
               vec2 = getPostDeformPos(x, z + 1);

               vec1_X = vec1.X - a_X;
               vec1_Y = vec1.Y - a_Y;
               vec1_Z = vec1.Z - a_Z;
               vec2_X = vec2.X - a_X;
               vec2_Y = vec2.Y - a_Y;
               vec2_Z = vec2.Z - a_Z;

               normal_X += vec1_Y * vec2_Z - vec1_Z * vec2_Y;
               normal_Y += vec1_Z * vec2_X - vec1_X * vec2_Z;
               normal_Z += vec1_X * vec2_Y - vec1_Y * vec2_X;
            }

            if ((x < mDesc.mNumXVerts - 1) && (z > 0))
            {
               //b = getPostDeformPos(x + 1, z);
               //c = getPostDeformPos(x, z - 1);

               //vec1 = Vector3.Subtract(b, a);
               //vec2 = Vector3.Subtract(c, a);
               //normal += Vector3.Cross(vec1, vec2);

               vec1 = getPostDeformPos(x + 1, z);
               vec2 = getPostDeformPos(x, z - 1);

               vec1_X = vec1.X - a_X;
               vec1_Y = vec1.Y - a_Y;
               vec1_Z = vec1.Z - a_Z;
               vec2_X = vec2.X - a_X;
               vec2_Y = vec2.Y - a_Y;
               vec2_Z = vec2.Z - a_Z;
               //?
               normal_X += vec1_Y * vec2_Z - vec1_Z * vec2_Y;
               normal_Y += vec1_Z * vec2_X - vec1_X * vec2_Z;
               normal_Z += vec1_X * vec2_Y - vec1_Y * vec2_X;
            }

            if ((x > 0) && (z > 0))
            {
               //b = getPostDeformPos(x - 1, z);
               //c = getPostDeformPos(x, z - 1);

               //vec1 = Vector3.Subtract(b, a);
               //vec2 = Vector3.Subtract(c, a);
               //normal += Vector3.Cross(vec2, vec1);

               vec1 = getPostDeformPos(x - 1, z);
               vec2 = getPostDeformPos(x, z - 1);

               vec1_X = vec1.X - a_X;
               vec1_Y = vec1.Y - a_Y;
               vec1_Z = vec1.Z - a_Z;
               vec2_X = vec2.X - a_X;
               vec2_Y = vec2.Y - a_Y;
               vec2_Z = vec2.Z - a_Z;

               normal_X += vec2_Y * vec1_Z - vec2_Z * vec1_Y;
               normal_Y += vec2_Z * vec1_X - vec2_X * vec1_Z;
               normal_Z += vec2_X * vec1_Y - vec2_Y * vec1_X;

            }
            float len = (float)Math.Sqrt((float)(normal_X * normal_X + normal_Y * normal_Y + normal_Z * normal_Z));
            if (Math.Abs(len) > 0.0001)
            {
               normal_X /= len;
               normal_Y /= len;
               normal_Z /= len;
            }
            normal.X = normal_X;
            normal.Y = normal_Y;
            normal.Z = normal_Z;


            //normal = BMathLib.Normalize(normal);
            tempFaceNormalArray[counter++] = normal;

            //TerrainGlobals.getEditor().getCurrBrushDeformationNormals()[x * (int)mDesc.mNumZVerts + z] = normal - normals[x * (int)mDesc.mNumZVerts + z];

         }
      }
      int numXVerts = mDesc.mNumXVerts;
      for (int x = minXVertex; x <= maxXVertex; x++)
      {
         for (int z = minZVertex; z <= maxZVertex; z++)
         {
            float normal_X, normal_Y, normal_Z;

            // Average normal
            Vector3 normal = tempFaceNormalArray[tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];
            Vector3 temp;
            normal_X = normal.X;
            normal_Y = normal.Y;
            normal_Z = normal.Z;


            if (x > 0)
            {
               //normal += tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               temp = tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

               normal_X = normal_X + temp.X;
               normal_Y = normal_Y + temp.Y;
               normal_Z = normal_Z + temp.Z;

            }
            if (x < numXVerts - 1)
            {
               //normal += tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               temp = tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

               normal_X = normal_X + temp.X;
               normal_Y = normal_Y + temp.Y;
               normal_Z = normal_Z + temp.Z;
            }
            if (z > 0)
            {
               //normal += tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               temp = tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

               normal_X = normal_X + temp.X;
               normal_Y = normal_Y + temp.Y;
               normal_Z = normal_Z + temp.Z;
            }
            if (z < numXVerts - 1)
            {
               //normal += tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               temp = tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

               normal_X = normal_X + temp.X;
               normal_Y = normal_Y + temp.Y;
               normal_Z = normal_Z + temp.Z;
            }
            //factor = 1.0f;
            if (x > 0)
            {
               if (z > 0)
               {
                  //normal += tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                  temp = tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];
                  normal_X = normal_X + temp.X;
                  normal_Y = normal_Y + temp.Y;
                  normal_Z = normal_Z + temp.Z;
               }
               if (z < numXVerts - 1)
               {
                  //normal += tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                  temp = tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];
                  normal_X = normal_X + temp.X;
                  normal_Y = normal_Y + temp.Y;
                  normal_Z = normal_Z + temp.Z;
               }
            }
            if (x < numXVerts - 1)
            {
               if (z > 0)
               {
                  //normal += tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                  temp = tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];
                  normal_X = normal_X + temp.X;
                  normal_Y = normal_Y + temp.Y;
                  normal_Z = normal_Z + temp.Z;
               }
               if (z < numXVerts - 1)
               {
                  //normal += tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
                  temp = tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];
                  normal_X = normal_X + temp.X;
                  normal_Y = normal_Y + temp.Y;
                  normal_Z = normal_Z + temp.Z;
               }
            }
            float len = (float)Math.Sqrt((float)(normal_X * normal_X + normal_Y * normal_Y + normal_Z * normal_Z));
            if (Math.Abs(len) > 0.0001)
            {
               normal_X /= len;
               normal_Y /= len;
               normal_Z /= len;
            }
            normal.X = normal_X;
            normal.Y = normal_Y;
            normal.Z = normal_Z;

            //normals[x * (int)numXVerts + z] = normal;


            //normal = BMathLib.Normalize(normal);
            //defNormals[x * stride + z] = normal - normals[x * stride + z];
            TerrainGlobals.getEditor().getCurrBrushDeformationNormals().SetValue(x * (int)mDesc.mNumZVerts + z,  normal - normals[x * (int)mDesc.mNumZVerts + z] );
         }
      }  

      //for (int x = minXVertex; x <= maxXVertex; x++)
      //{
      //   for (int z = minZVertex; z <= maxZVertex; z++)
      //   {
      //      // Average normal
      //      Vector3 normal = tempFaceNormalArray[tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

      //      float factor = 1.0f;
      //      if (x > 0)
      //         normal += tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
      //      if (x < mDesc.mNumXVerts - 1)
      //         normal += tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
      //      if (z > 0)
      //         normal += tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
      //      if (z < mDesc.mNumZVerts - 1)
      //         normal += tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;

      //      factor = 1.0f;
      //      if (x > 0)
      //      {
      //         if (z > 0)
      //            normal += tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
      //         if (z < mDesc.mNumZVerts - 1)
      //            normal += tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
      //      }
      //      if (x < mDesc.mNumXVerts - 1)
      //      {
      //         if (z > 0)
      //            normal += tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
      //         if (z < mDesc.mNumZVerts - 1)
      //            normal += tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
      //      }


      //      normal = BMathLib.Normalize(normal);
      //      //defNormals[x * stride + z] = normal - normals[x * stride + z];
      //      TerrainGlobals.getEditor().getCurrBrushDeformationNormals()[x * (int)mDesc.mNumZVerts + z] = normal - normals[x * (int)mDesc.mNumZVerts + z];
      //   }
      //}   

  
   }
#endif

#if OLD_computeBasisCurr
   public void computeBasisCurr(int minXVertex, int maxXVertex, int minZVertex, int maxZVertex)
   {
      
      //bounds / reality check
      if (minXVertex < 0) minXVertex = 0;
      if (minZVertex < 0) minZVertex = 0;
      if (maxXVertex >= mDesc.mNumXVerts) maxXVertex = mDesc.mNumXVerts - 1;
      if (maxZVertex >= mDesc.mNumZVerts) maxZVertex = mDesc.mNumZVerts - 1;

      int minXVertexFace = (minXVertex > 0) ? minXVertex - 1 : minXVertex;
      int minZVertexFace = (minZVertex > 0) ? minZVertex - 1 : minZVertex;
      int maxXVertexFace = (maxXVertex < mDesc.mNumXVerts - 1) ? maxXVertex + 1 : maxXVertex;
      int maxZVertexFace = (maxZVertex < mDesc.mNumXVerts - 1) ? maxZVertex + 1 : maxZVertex;


      Vector3[] normals = TerrainGlobals.getEditor().getNormals();
      //Vector3[] defNormals = TerrainGlobals.getEditor().getCurrBrushDeformationNormals();

      int size = (maxZVertexFace - minZVertexFace + 2) * (maxXVertexFace - minXVertexFace + 2);
      if (tempFaceNormalArray == null || tempFaceNormalArray.Length < size)
      {
         tempFaceNormalArray = new Vector3[size];
      }
      int counter = 0;


      // Compute all face normals, this is for temporary use only
      for (int x = minXVertexFace; x <= maxXVertexFace; x++)
      {
         for (int z = minZVertexFace; z <= maxZVertexFace; z++)
         {
            // Compute normal
            Vector3 normal = new Vector3(0.0f, 0.0f, 0.0f);
            Vector3 vec1, vec2;
            Vector3 a, b, c;

            a = getPostDeformPos(x, z);

            if ((x < mDesc.mNumXVerts - 1) && (z < mDesc.mNumZVerts - 1))
            {
               b = getPostDeformPos(x + 1, z);
               c = getPostDeformPos(x, z + 1);

               vec1 = Vector3.Subtract(b, a);
               vec2 = Vector3.Subtract(c, a);
               normal += Vector3.Cross(vec2, vec1);
            }

            if ((x > 0) && (z < mDesc.mNumZVerts - 1))
            {
               b = getPostDeformPos(x - 1, z);
               c = getPostDeformPos(x, z + 1);

               vec1 = Vector3.Subtract(b, a);
               vec2 = Vector3.Subtract(c, a);
               normal += Vector3.Cross(vec1, vec2);
            }

            if ((x < mDesc.mNumXVerts - 1) && (z > 0))
            {
               b = getPostDeformPos(x + 1, z);
               c = getPostDeformPos(x, z - 1);

               vec1 = Vector3.Subtract(b, a);
               vec2 = Vector3.Subtract(c, a);
               normal += Vector3.Cross(vec1, vec2);
            }

            if ((x > 0) && (z > 0))
            {
               b = getPostDeformPos(x - 1, z);
               c = getPostDeformPos(x, z - 1);

               vec1 = Vector3.Subtract(b, a);
               vec2 = Vector3.Subtract(c, a);
               normal += Vector3.Cross(vec2, vec1);
            }
            normal = BMathLib.Normalize(normal);
            tempFaceNormalArray[counter++] = normal;
         }
      }

      for (int x = minXVertex; x <= maxXVertex; x++)
      {
         for (int z = minZVertex; z <= maxZVertex; z++)
         {
            // Average normal
            Vector3 normal = tempFaceNormalArray[tempArrayIndex(x, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)];

            float factor = 1.0f;
            if (x > 0)
               normal += tempFaceNormalArray[tempArrayIndex(x - 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            if (x < mDesc.mNumXVerts - 1)
               normal += tempFaceNormalArray[tempArrayIndex(x + 1, z, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            if (z > 0)
               normal += tempFaceNormalArray[tempArrayIndex(x, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            if (z < mDesc.mNumZVerts - 1)
               normal += tempFaceNormalArray[tempArrayIndex(x, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;

            factor = 1.0f;
            if (x > 0)
            {
               if (z > 0)
                  normal += tempFaceNormalArray[tempArrayIndex(x - 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               if (z < mDesc.mNumZVerts - 1)
                  normal += tempFaceNormalArray[tempArrayIndex(x - 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            }
            if (x < mDesc.mNumXVerts - 1)
            {
               if (z > 0)
                  normal += tempFaceNormalArray[tempArrayIndex(x + 1, z - 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
               if (z < mDesc.mNumZVerts - 1)
                  normal += tempFaceNormalArray[tempArrayIndex(x + 1, z + 1, minXVertexFace, maxXVertexFace, minZVertexFace, maxZVertexFace)] * factor;
            }


            normal = BMathLib.Normalize(normal);
            //defNormals[x * stride + z] = normal - normals[x * stride + z];
            TerrainGlobals.getEditor().getCurrBrushDeformationNormals()[x * (int)mDesc.mNumZVerts + z] = normal - normals[x * (int)mDesc.mNumZVerts + z];
         }
      }
   }

#endif
   float computeCanonicalAngle(Vector3 pNormal, ref Vector3 pTangent)
   {
      
      // Compute canonical angle
      Vector3 reference;
      Vector3 axisX = new Vector3(1.0f, 0.0f, 0.0f);
      Vector3 axisZ = new Vector3(0.0f, 0.0f, 1.0f);

      float canonicalAngle = 0.0f;
      float dotX = BMathLib.Dot(ref pNormal, ref axisX);
      float dotZ = BMathLib.Dot(ref pNormal, ref axisZ);

      if(Math.Abs(dotX) < Math.Abs(dotZ))
      {
         reference= Vector3.Cross(axisX, pNormal);
         reference = BMathLib.Normalize(reference);

         float dot = BMathLib.Dot(ref pTangent, ref reference);
         if(dot < -1.0f) dot = -1.0f;
         if(dot > 1.0f) dot = 1.0f;
         canonicalAngle = (float)Math.Acos(dot) - (3.1415926536f * .5f);    // 90
      }
      else
      {
         reference = Vector3.Cross( pNormal, axisZ);
         reference = BMathLib.Normalize(reference);

         float dot = BMathLib.Dot(ref pTangent, ref reference);
         if(dot < -1.0f) dot = -1.0f;
         if(dot > 1.0f) dot = 1.0f;
         canonicalAngle = (float)Math.Acos(dot);
      }


      // Make sure angle is alway positive from 0 - 2PI
      if(canonicalAngle < 0.0f)
      {
         canonicalAngle += (3.1415926536f * 2.0f);    // 180
      }

      return(canonicalAngle);
   }
   #endregion

   #region Intersection
   //intersection
   public bool rayIntersects(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref BTerrainQuadNode ownerNode, bool intersectWithZero)
   {
      bool hit = false;
      hit = mQuadNodeRoot.rayIntersects(ref origin, ref dir, ref intersectionPt, ref ownerNode);

      // If we haven't found a hit check with the y=0 plane
      if (!hit && intersectWithZero)
      {
         Vector3 planeNormal = new Vector3(0.0f, 1.0f, 0.0f);
         float incidentAngle = BMathLib.Dot(ref planeNormal, ref dir);

         if(incidentAngle != 0)
         {
            float t = -origin.Y / dir.Y;

            intersectionPt = origin;
            intersectionPt += (dir * t);

            hit = true;
         }
      }
      return hit;
   }
   public bool rayIntersects(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal, ref BTerrainQuadNode ownerNode, bool intersectWithZero)
   {
       int visTileX = 0;
          int visTileZ = 0;
      return rayIntersects(ref origin, ref dir, ref intersectionPt, ref intersectionNormal,ref visTileX, ref visTileZ, ref ownerNode, intersectWithZero);
   }
   public bool rayIntersects(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref Vector3 intersectionNormal,ref int visTileX, ref int visTileZ, ref BTerrainQuadNode ownerNode, bool intersectWithZero)
   {
      bool hit = false;
      hit = mQuadNodeRoot.rayIntersects(ref origin, ref dir, ref intersectionPt, ref intersectionNormal, ref visTileX, ref visTileZ,ref ownerNode);

      // If we haven't found a hit check with the y=0 plane
      if (!hit && intersectWithZero)
      {
         Vector3 planeNormal = new Vector3(0.0f, 1.0f, 0.0f);
         float incidentAngle = BMathLib.Dot(ref planeNormal, ref dir);

         if (incidentAngle != 0)
         {
            float t = -origin.Y / dir.Y;

            intersectionPt = origin;
            intersectionPt += (dir * t);

            intersectionNormal = planeNormal;

            hit = true;
         }
      }
      return hit;
   }
   public bool rayIntersectsGuaranteed(ref Vector3 origin, ref Vector3 dir, ref Vector3 intersectionPt, ref BTerrainQuadNode ownerNode)    //if misses terrain, return XZ plane intersect  
   {
      if (!mQuadNodeRoot.rayIntersects(ref origin, ref dir, ref intersectionPt, ref ownerNode))
      {
         Plane pl = Plane.FromPointNormal(new Vector3(0, 0, 0), new Vector3(0, 1, 0));
         float tVal = 0;
         if (BMathLib.rayPlaneIntersect(pl, origin, dir, false, ref tVal))
         {
            intersectionPt = dir * tVal;
         }
         else
         {
            intersectionPt.X = origin.X;
            intersectionPt.Y = 0;
            intersectionPt.Z = origin.Z;
         }
      }
      return true;
   }
   public bool segmentIntersects(ref Vector3 pt0, ref Vector3 pt1, ref Vector3 intersectionPt, ref BTerrainQuadNode ownerNode)
   {
      Vector3 dir;
      dir.X = pt1.X - pt0.X;
      dir.Y = pt1.Y - pt0.Y;
      dir.Z = pt1.Z - pt0.Z;

      bool hit = false;
      hit = mQuadNodeRoot.rayIntersects(ref pt0, ref dir, ref intersectionPt,ref ownerNode);


      return hit;
   }
   //WILL CAST A RAY!!
   public float getHeightAtXZ(int x, int z)
   {
      float height = 0;

      Vector3 intersectionPt = Vector3.Empty;
      //Vector3 origin = new Vector3(x,1000f,z);
      Vector3 dir = new Vector3(0, -1f, 0);
      BTerrainQuadNode ownerNode = null;

      Vector3 origin = new Vector3(x * mDesc.mTileScale, 1000f, z * mDesc.mTileScale);
      if (mQuadNodeRoot.rayIntersects(ref origin, ref dir, ref intersectionPt, ref ownerNode))
      //if (mQuadNodeRoot.rayIntersects(ref origin, ref dir, ref intersectionPt, ref ownerNode))
      {
         height = intersectionPt.Y;
      }

      return height;
   }

   public float getHeightAtXZ(float x, float z)
   {
      float height = 0;

      Vector3 intersectionPt = Vector3.Empty;
      //Vector3 origin = new Vector3(x,1000f,z);
      Vector3 dir = new Vector3(0, -1f, 0);
      BTerrainQuadNode ownerNode = null;

      Vector3 origin = new Vector3(x , 1000f, z );
      if (mQuadNodeRoot.rayIntersects(ref origin, ref dir, ref intersectionPt, ref ownerNode))
      //if (mQuadNodeRoot.rayIntersects(ref origin, ref dir, ref intersectionPt, ref ownerNode))
      {
         height = intersectionPt.Y;
      }

      return height;
   }

   public float getHeightParametric(float u, float v)
   {
      float x = u * getNumXVerts();
      float z = v * getNumZVerts();
      long  iX = (long)x;
      long  iZ = (long)z;

      float y0= getPos(iX, iZ).Y;
      float y1 = getPos(iX, iZ + 1).Y;
      float y2 = getPos(iX + 1, iZ + 1).Y;
      float y3 = getPos(iX + 1, iZ).Y;

      float uFrac = x - iX;
      float vFrac = z - iZ;

      float topX = BMathLib.LERP(y1, y2, uFrac);
      float botX = BMathLib.LERP(y0, y3, uFrac);

      float retHeight = BMathLib.LERP(botX, topX, vFrac);

      return retHeight;

   }

   public Vector3 getRelVectorParametric(float u, float v)
   {
      float x = u * getNumXVerts();
      float z = v * getNumZVerts();
      int iX = (int)x;
      int iZ = (int)z;
      float uFrac = x - iX;
      float vFrac = z - iZ;

      Vector3 y0 = getRelPos(iX, iZ);
      Vector3 y1 = getRelPos(iX, iZ + 1);
      Vector3 y2 = getRelPos(iX + 1, iZ + 1);
      Vector3 y3 = getRelPos(iX + 1, iZ);


      Vector3 top = new Vector3();
      top.X = BMathLib.LERP(y1.X, y2.X, uFrac);
      top.Y = BMathLib.LERP(y1.Y, y2.Y, uFrac);
      top.Z = BMathLib.LERP(y1.Z, y2.Z, uFrac);
      Vector3 bot = new Vector3();
      bot.X = BMathLib.LERP(y0.X, y3.X, uFrac);
      bot.Y = BMathLib.LERP(y0.Y, y3.Y, uFrac);
      bot.Z = BMathLib.LERP(y0.Z, y3.Z, uFrac);

      Vector3 retVec = new Vector3();
      retVec.X = BMathLib.LERP(bot.X, top.X, vFrac);
      retVec.Y = BMathLib.LERP(bot.Y, top.Y, vFrac);
      retVec.Z = BMathLib.LERP(bot.Z, top.Z, vFrac);


      return retVec;

   }
   #endregion

   #region position requests
   //returns the actual vertex position at texture position XZ
   public Vector3 getPos(long x, long y)
   {
      return getPos((int)x, (int)y);
   }
   public void getPos(ref Vector3 vec, long x, long y)
   {

      if (x >= mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      if (y >= mDesc.mNumXVerts) y = mDesc.mNumXVerts - 1;

      //unsafe
      {
         if (x > mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
         int indx = (int)(x * mDesc.mNumZVerts + y);
         //vec = TerrainGlobals.getEditor().getDetailPoints()[indx].X;
         Vector3[] points = TerrainGlobals.getEditor().getDetailPoints();
         vec.X = points[indx].X + x * mDesc.mTileScale;
         vec.Y = points[indx].Y;
         vec.Z = points[indx].Z + y * mDesc.mTileScale;
      }
   }
	public Vector3	getPos(int x, int y)      
   {

      if (x >= mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      if (y >= mDesc.mNumXVerts) y = mDesc.mNumXVerts - 1;


      if (x > mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      int indx = x * mDesc.mNumZVerts + y;
      Vector3 vec = TerrainGlobals.getEditor().getDetailPoints()[indx];
      vec.X += x * mDesc.mTileScale;
      vec.Z += y * mDesc.mTileScale;
      return vec;


   }
   public Vector3	getRelPos(int x, int y)      
   {


      if (x >= mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      if (y >= mDesc.mNumXVerts) y = mDesc.mNumXVerts - 1;

      int indx = x * (int)mDesc.mNumZVerts + y;
      Vector3 vec = TerrainGlobals.getEditor().getDetailPoints()[indx];
      //vec.Y += TerrainGlobals.getEditor().getControlPoints()[indx];
      return vec;
   }
   public Vector3 getPostDeformPos(long x, long y)
   {
      return getPostDeformPos((int)x, (int)y);
   }


   public Vector3 getPostDeformPos(int x, int y)
   {
 

       if (x >= mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
       if (y >= mDesc.mNumXVerts) y = mDesc.mNumXVerts - 1;

      int indx = x * (int)mDesc.mNumZVerts + y;
       Vector3 vec = TerrainGlobals.getEditor().getDetailPoints()[indx];
       if (TerrainGlobals.getEditor().hasBrushData(x, y))
       {
           //if (TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(indx))
           //    vec += TerrainGlobals.getEditor().getCurrBrushDeformations()[indx];

          vec += TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(indx);

           //vec.Y += TerrainGlobals.getEditor().getControlPoints()[indx];
       }
       vec.X += x * mDesc.mTileScale;
       vec.Z += y * mDesc.mTileScale;
       return vec;
   }
   public Vector3 getPostDeformRelPos(int x, int y)
   {
   
      if (x >= mDesc.mNumZVerts - 1) x = mDesc.mNumZVerts - 1;
      if (y >= mDesc.mNumZVerts - 1) y = mDesc.mNumZVerts - 1;


      int indx = x * (int)mDesc.mNumZVerts + y;
      Vector3 vec = TerrainGlobals.getEditor().getDetailPoints()[indx];
      if (TerrainGlobals.getEditor().hasBrushData(x, y))
      {
         //if (TerrainGlobals.getEditor().getCurrBrushDeformations().ContainsKey(indx))
         //   vec += TerrainGlobals.getEditor().getCurrBrushDeformations()[indx];

         vec += TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(indx);

      }
      //vec.Y += TerrainGlobals.getEditor().getControlPoints()[indx];
      return vec;
   }

   public float getSimOverrideHeight(long x, long y)
   {
      if (x >= mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      if (y >= mDesc.mNumXVerts) y = mDesc.mNumXVerts - 1;

      
      if (x > mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      int indx = (int)(x * mDesc.mNumZVerts + y);

      
      float overideHeight = TerrainGlobals.getEditor().getSimRep().getHeightRep().getJaggedHeight(indx);
      if (overideHeight != SimHeightRep.cJaggedEmptyValue)
      {
         return overideHeight;
      }

      return TerrainGlobals.getEditor().getDetailPoints()[indx].Y;
      
   }

   public byte getTessOverride(long x, long y)
   {
      if (x >= mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      if (y >= mDesc.mNumXVerts) y = mDesc.mNumXVerts - 1;


      if (x > mDesc.mNumXVerts) x = mDesc.mNumXVerts - 1;
      int indx = (int)(x * mDesc.mNumZVerts + y);

      JaggedContainer<byte> heights = TerrainGlobals.getEditor().getJaggedTesselation();

      byte overideHeight = heights.GetValue(indx);
      if (overideHeight != BTerrainEditor.cTesselationEmptyVal)
      {
         return overideHeight;
      }

      return 0;

   }


   public int getAddedPostDeformRelPos(int minX, int minY, int maxX, int maxY, out float sum_X, out float sum_Y, out float sum_Z)
   {
      sum_X = 0.0f;
      sum_Y = 0.0f;
      sum_Z = 0.0f;

      Vector3 vec;
      Vector3[] detailedPoints = TerrainGlobals.getEditor().getDetailPoints();

      int numCoefficents = 0;
      int x, y;
      for (x = minX; x <= maxX; x++)
      {
         for (y = minY; y <= maxY; y++)
         {
            int indx = x * (int)mDesc.mNumZVerts + y;
            vec = detailedPoints[indx];

            sum_X += vec.X;
            sum_Y += vec.Y;
            sum_Z += vec.Z;

            if (TerrainGlobals.getEditor().hasBrushData(x, y))
            {
               vec = TerrainGlobals.getEditor().getCurrBrushDeformations().GetValue(indx);
               sum_X += vec.X;
               sum_Y += vec.Y;
               sum_Z += vec.Z;
            }

            numCoefficents++;
         }
      }

      return numCoefficents;
   }

   public Vector3 getNormal(int x, int y)      //returns the normal at texture position XY?
   {
      return (TerrainGlobals.getEditor().getNormals()[x * (int)mDesc.mNumZVerts + y]);
   }
   public Vector3 getPostDeformNormal(int x, int y)
   {
 
      if (x >= mDesc.mNumZVerts - 1) x = mDesc.mNumZVerts - 1;
      if (y >= mDesc.mNumZVerts - 1) y = mDesc.mNumZVerts - 1;

      int indx = x * (int)mDesc.mNumZVerts + y;
      Vector3 vec = TerrainGlobals.getEditor().getNormals()[indx];
      //vec += TerrainGlobals.getEditor().getCurrBrushDeformationNormals()[indx];

      //if (TerrainGlobals.getEditor().hasBrushData(x, y))
      //{
      //   if (TerrainGlobals.getEditor().getCurrBrushDeformationNormals().ContainsKey(indx))
      //      vec += TerrainGlobals.getEditor().getCurrBrushDeformationNormals()[indx];
      //}
      if (TerrainGlobals.getEditor().hasBrushData(x, y))
      {
         //if (TerrainGlobals.getEditor().getCurrBrushDeformationNormals().ContainsKey(indx))
            vec += TerrainGlobals.getEditor().getCurrBrushDeformationNormals().GetValue(indx);
      }

      return vec;
   }
   public float getAmbientOcclusion(int x, int y)
   {
      if (x >= mDesc.mNumZVerts - 1) x = mDesc.mNumZVerts - 1;
      if (y >= mDesc.mNumZVerts - 1) y = mDesc.mNumZVerts - 1;

      int indx = x * (int)mDesc.mNumZVerts + y;
      float ao = TerrainGlobals.getEditor().getAmbientOcclusionValues()[indx];
      return (ao);
   }
   public float getSoftSelectionWeight(int x, int y)
   {
      float weight = getSoftSelectionWeightRaw(x, y);
      weight = 1.0f - weight;

      return (weight);
   }
   public float getSoftSelectionWeightRaw(int x, int y)
   {
      int indx = x * (int)mDesc.mNumZVerts + y;

      return Masking.getCurrSelectionMaskWeights().GetMaskWeight((long)indx);

      //if (Masking.hasSelectionMaskData(x, y))
      //{
      //   if (Masking.getCurrSelectionMaskWeights().ContainsKey(indx))
      //      return (Masking.getCurrSelectionMaskWeights()[indx]);
      //}
      //return (0.0f);
   }
   public byte getAlphaValue(int x, int y)
   {
      if (x >= mDesc.mNumZVerts - 1) x = mDesc.mNumZVerts - 1;
      if (y >= mDesc.mNumZVerts - 1) y = mDesc.mNumZVerts - 1;

      int indx = x * (int)mDesc.mNumZVerts + y;

      return TerrainGlobals.getEditor().getAlphaValues()[indx];

   }

   public Vector3 getLightValue(int x, int y)
   {
      if (x >= mDesc.mNumZVerts - 1) x = mDesc.mNumZVerts - 1;
      if (y >= mDesc.mNumZVerts - 1) y = mDesc.mNumZVerts - 1;

      int indx = x * (int)mDesc.mNumZVerts + y;

      return TerrainGlobals.getEditor().getLightValues()[indx];

   }

   public float getSkirtHeight(int x, int y)
   {
      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (x >= mDesc.mTotalSkirtXVerts - 1) x = mDesc.mTotalSkirtXVerts - 1;
      if (y >= mDesc.mTotalSkirtZVerts - 1) y = mDesc.mTotalSkirtZVerts - 1;


      int indx = x * (int)mDesc.mTotalSkirtZVerts + y;
      float height = TerrainGlobals.getEditor().getSkirtHeights()[indx];
 
      return height;
   }


   public Vector3 getSkirtPos(int x, int y)
   {
      if (x >= mDesc.mTotalSkirtXVerts - 1) x = mDesc.mTotalSkirtXVerts - 1;
      if (y >= mDesc.mTotalSkirtZVerts - 1) y = mDesc.mTotalSkirtZVerts - 1;


      Vector3 vec;
      int indx = x * (int)mDesc.mTotalSkirtZVerts + y;
      float height = TerrainGlobals.getEditor().getSkirtHeights()[indx];

      vec.X = x * ((mDesc.mWorldSizeX * 3.0f) / (mDesc.mTotalSkirtXVerts - 1)) - mDesc.mWorldSizeX;
      vec.Y = height;
      vec.Z = y * ((mDesc.mWorldSizeZ * 3.0f) / (mDesc.mTotalSkirtZVerts - 1)) - mDesc.mWorldSizeX;

      return vec;
   }

   public BBoundingBox getDisplacementBB()
   {
      BBoundingBox bb = new BBoundingBox();
      for (int x = 0; x < TerrainGlobals.getTerrain().getNumXVerts(); x++)
      {
         for (int z = 0; z < TerrainGlobals.getTerrain().getNumZVerts(); z++)
         {
            Vector3 v = TerrainGlobals.getTerrain().getRelPos(x, z);
            bb.addPoint(v);
         }
      }
      return bb;
   }
   #endregion

   #region TERRAIN ANALYSIS

   //return if we're a local minima
   //NOTE check this against semi-flat areas
   public bool isLocalMinima(int x, int y)
   {
      float h = getPostDeformPos(x, y).Y;
      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(y + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(y - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      bool lower = true;

      float epsl = 0.002f;
      lower &= (h < getPostDeformPos(nx, ny).Y - epsl ? true : false);
      lower &= (h < getPostDeformPos(x, ny).Y - epsl ? true : false);
      lower &= (h < getPostDeformPos(px, ny).Y - epsl ? true : false);
      lower &= (h < getPostDeformPos(nx, y).Y - epsl ? true : false);
      lower &= (h < getPostDeformPos(px, y).Y - epsl ? true : false);
      lower &= (h < getPostDeformPos(nx, py).Y - epsl ? true : false);
      lower &= (h < getPostDeformPos(x, py).Y - epsl ? true : false);
      lower &= (h < getPostDeformPos(px, py).Y - epsl ? true : false);

      return lower;

   }

   //give the grid index of the lowest neighbor
   //NOTE does this handle flat areas well?
   public bool findLowestNeighbor(int x, int z, out int nX, out int nZ, out float difference)
   {
      nX = x;
      nZ = z;
      difference = 0;

      float h = TerrainGlobals.getTerrain().getPostDeformPos(x, z).Y;
      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(z + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(z - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      float largest = float.MinValue;


      int[] offs = new int[]{nx,ny,  
                                 x, ny,
                                 px, ny,
                                 nx, z,
                                 px, z,
                                 nx, py,
                                 x, py,
                                 px, py};

      for (int i = 0; i < 8; i++)
      {
         float diff = h - TerrainGlobals.getTerrain().getPostDeformPos(offs[(2 * i)], offs[(2 * i) + 1]).Y;
         if (diff <= 0)
            continue;

         if (diff > largest)
         {
            nX = offs[(2 * i)];
            nZ = offs[(2 * i) + 1];
            largest = diff;
         }
      }

      if (largest == float.MinValue)
      {
         return false;
      }

      difference = largest;

      return true;
   }


   //CLM The gradient of a scalar field is a vector of partial derivatives of the scalar field.
   public Vector2 getGradiant(int x, int y)
   {
      Vector2 v = new Vector2();

      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(y + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(y - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      v.X = (getPostDeformPos(px, y).Y - getPostDeformPos(nx, y).Y) / (2 * TerrainGlobals.getTerrain().getTileScale());
      v.Y = (getPostDeformPos(x, py).Y - getPostDeformPos(x, ny).Y) / (2 * TerrainGlobals.getTerrain().getTileScale());
      return v;
   }
   /*
    * Divergence is the rate at which “density” exits a given region of space. In the Navier-Stokes equations, it
      is applied to the velocity of the flow, and it measures the net change in velocity across a
      surface surrounding a small piece of the fluid. 
      The dot product in the divergence operator results in a sum of partial derivatives
      (rather than a vector, as with the gradient operator). This means that the divergence
      operator can be applied only to a vector field, such as the velocity, u = (u, v).
    */
   public float getDivergence(int x, int y)
   {
      float v = 0;
      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(y + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(y - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      v = (getPostDeformPos(px, y).X - getPostDeformPos(nx, y).X) / (2 * TerrainGlobals.getTerrain().getTileScale());
      v += (getPostDeformPos(x, py).Y - getPostDeformPos(x, ny).Y) / (2 * TerrainGlobals.getTerrain().getTileScale());

      return v;
   }
   /*
    * If the divergence operator is applied to the result of the gradient
      operator, the result is the Laplacian operator
    */
   public float getLaplacian(int x, int y)
   {
      float v = 0; ;
      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(y + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(y - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      v = (getPostDeformPos(px, y).Y - (2 * getPostDeformPos(x, y).Y + getPostDeformPos(nx, y).Y)) / (TerrainGlobals.getTerrain().getTileScale() * TerrainGlobals.getTerrain().getTileScale());
      v += (getPostDeformPos(x, py).Y - (2 * getPostDeformPos(x, y).Y + getPostDeformPos(x, ny).Y)) / (TerrainGlobals.getTerrain().getTileScale() * TerrainGlobals.getTerrain().getTileScale());

      return v;
   }


   //Change in XZ over change in Height, (does not include center point)
   public Vector2 getXZDerivitives(int x, int z)   
   {
      float scale = getTileScale();
      Vector2 slope = new Vector2();
      if (x - 1 < 0)
         slope.X = (getPostDeformPos(x + 1, z).Y - getPostDeformPos(x, z).Y) / scale;
      else if (x + 1 >= getNumXVerts())
         slope.X = (getPostDeformPos(x - 1, z).Y - getPostDeformPos(x, z).Y) / scale;
      else 
         slope.X = (getPostDeformPos(x - 1, z).Y - getPostDeformPos(x+1, z).Y) / (2*scale);

      if (z - 1 < 0)
         slope.Y = (getPostDeformPos(x, z + 1).Y - getPostDeformPos(x, z).Y) / scale;
      else if (z + 1 >= getNumXVerts())
         slope.Y = (getPostDeformPos(x, z - 1).Y - getPostDeformPos(x, z).Y) / scale;
      else
         slope.Y = (getPostDeformPos(x , z - 1).Y - getPostDeformPos(x , z+ 1).Y) / (2 * scale);

      return slope;
   }

   //Calculate slope using Finite Differences
   //This formula depends only on the elevations in the four cardinal directions.
   public float getSlopeFiniteDiff(int x, int z)
   {
      return (float)Math.Sqrt(getP(x,z));
   }

   /* Calculate slipe using the D8 method
    * D8 method gives slightly smaller average slopes than the finite difference method, 
    * because the direction in which the elevation difference is calculated is not alwayas that of the steepest decent.
    */
   public float getSlopeD8(int x, int z)
   {
      //calculate slope using D8
      float h = TerrainGlobals.getTerrain().getPostDeformPos(x, z).Y;
      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(z + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(z - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      float largest = float.MinValue;


      int[] offs = new int[]{nx,ny,  
                                 x, ny,
                                 px, ny,
                                 nx, z,
                                 px, z,
                                 nx, py,
                                 x, py,
                                 px, py};
      float root2 = (float)Math.Sqrt(2);
      float[] hPhi = new float[]{root2, 
                                 1,
                                 root2,
                                 1,
                                 1,
                                 root2,
                                 1,
                                 root2};

      float Slope =0;

      for (int i = 0; i < 8; i++)
      {
         float diff = h - TerrainGlobals.getTerrain().getPostDeformPos(offs[(2 * i)], offs[(2 * i) + 1]).Y;

         float s = diff / (getTileScale() * hPhi[i]);

         if (diff > Slope)
         {
            Slope = diff;
         }
      }

      return Slope;
   }

   public Vector2 getXZCurvature(int x, int z)
   {
      Vector2 v = new Vector2();

      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(z + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(z - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      v.X = (getPostDeformPos(px, z).Y - (2 * getPostDeformPos(x, z).Y) + getPostDeformPos(nx, z).Y) / (getTileScale() * getTileScale());
      v.Y = (getPostDeformPos(x, py).Y - (2 * getPostDeformPos(x, z).Y) + getPostDeformPos(x, ny).Y) / (getTileScale() * getTileScale());

      return v;
   }

   //Get the change in X height wrt to the change in Z height
   public float getTwistingAmt(int x, int z)
   {
      int px = (int)BMathLib.Clamp(x + 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int py = (int)BMathLib.Clamp(z + 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      int nx = (int)BMathLib.Clamp(x - 1, 0, TerrainGlobals.getTerrain().getNumXVerts() - 1);
      int ny = (int)BMathLib.Clamp(z - 1, 0, TerrainGlobals.getTerrain().getNumZVerts() - 1);

      float sum = -getPostDeformPos(nx, py).Y + getPostDeformPos(px, py).Y + getPostDeformPos(px, ny).Y;
      return sum / (4 * (getTileScale() * getTileScale()));
   }

   //two general equations that are useful elsewhere
   public float getP(int x, int z)
   {
      Vector2 derv = getXZDerivitives(x, z);
      float p = derv.X * derv.X + derv.Y * derv.Y;
      return p;
   }
   public float getQ(int x, int z)
   {
      return getP(x, z) + 1;
   }



   #endregion

   public void refreshTerrainVisuals()
   {
      mQuadNodeRoot.clearVisibleDatHandle();
      mQuadNodeRoot.freeCacheHandles();

   }
   //exportation stuff
   Export360.ExportSettings mCurrExportSettings = new Export360.ExportSettings();

   public Export360.ExportSettings getExportSettings()
   {
      return mCurrExportSettings;
   }
   public void setExportSettings(Export360.ExportSettings set)
   {
      mCurrExportSettings = set;
   }


   public BTerrainDesc getDesc()
   {
      return mDesc;
   }

   public int  getNumXVerts()                   {return mDesc.mNumXVerts; }
   public int  getNumZVerts()                   {return mDesc.mNumZVerts; }
   public float  getWorldSizeX()                {return mDesc.mWorldSizeX; }
   public float  getWorldSizeZ()                {return mDesc.mWorldSizeZ; }
   public float  getTileScale()                 {return mDesc.mTileScale; }

   public Vector3 getBBMin() { return mQuadNodeRoot.getDesc().m_min; }
   public Vector3 getBBMax() { return mQuadNodeRoot.getDesc().m_max; }

   public BTerrainQuadNode getQuadNodeRoot()    {return mQuadNodeRoot;}
   public List<BTerrainQuadNode> getQuadNodeLeafList() 
   {
      List<BTerrainQuadNode> mNodes = new List<BTerrainQuadNode>();
      for (int i = 0; i < mLeafQuadNodes.Length; i++)
         mNodes.Add(mLeafQuadNodes[i]);
      return mNodes; 
   }
   public BTerrainQuadNode[] getQuadNodeLeafArray() { return mLeafQuadNodes; }
   public BTerrainQuadNode getLeafNode(int indexX, int indexZ)
   {
      int numXNodes = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
      if (indexX < 0 || indexX >= numXNodes || indexZ < 0 || indexZ >= numXNodes)
         return null;
      return mLeafQuadNodes[indexX + numXNodes * indexZ];
   }

   public int getNumSkirtXVertsPerQuadrant()               { return mDesc.mNumSkirtXVertsPerQuadrant; }
   public int getNumSkirtZVertsPerQuadrant()               { return mDesc.mNumSkirtZVertsPerQuadrant; }
   public int getTotalSkirtXVerts()             { return mDesc.mTotalSkirtXVerts; }
   public int getTotalSkirtZVerts()             { return mDesc.mTotalSkirtZVerts; }

   //EDITOR SPECIFIC
   public BFrustum       getFrustum() { return mFrustum;}

   //Interfaces
   public void rebuildDirty(Microsoft.DirectX.Direct3D.Device device)
   {
      mQuadNodeRoot.rebuildDirty(device);
   }
   public void rebuildDirtyPostDeform(Microsoft.DirectX.Direct3D.Device device)
   {
      mQuadNodeRoot.rebuildDirtyPostDeform(device);
   }


   //---------------------------------------MEMBERS
  


	private BTerrainDesc		      mDesc;
	private BTerrainQuadNode		mQuadNodeRoot = null;
   private BTerrainQuadNode      []mLeafQuadNodes = null; //CLM list to keep others from clearing it...
   private BFrustum              mFrustum = null;
};

#endregion

}
