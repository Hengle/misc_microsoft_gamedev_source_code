using System;
using System.IO;
using System.Drawing;
using System.Runtime.InteropServices;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

using EditorCore;
using Terrain;
using Terrain.Refinement;
/*
 * CLM 04.17.06
 * The terrain refinement class is responsible for minimizing visual redundancy in the terrain mesh.
 * 
 */
namespace Export360
{

   public class Refine_Patches
   {
      ~Refine_Patches()
      {
         destroy();
      }
      //---------------------------------------
      public void init()
      {
         mPatchTemplates = new PatchLODTemplate[(int)PatchLODTemplate.ePatchLODLevel.cLOD_Count];
         for (int i = 0; i < mPatchTemplates.Length; i++)
         {
            mPatchTemplates[i] = new PatchLODTemplate();
            mPatchTemplates[i].construct((PatchLODTemplate.ePatchLODLevel)i);
         }

         mCurrPatchPoints = new Vector3[PatchLODTemplate.cNumPatchXVerts, PatchLODTemplate.cNumPatchYVerts];
         mCurrPatchNormals = new Vector3[PatchLODTemplate.cNumPatchXVerts, PatchLODTemplate.cNumPatchYVerts];
      }
      public void destroy()
      {
         if(mPatchTemplates!=null)
         {
            for (int i = 0; i < mPatchTemplates.Length; i++)
            {
               mPatchTemplates[i].destroy();
               mPatchTemplates[i] = null;
            }
            mPatchTemplates = null;
         }
         if(mCurrPatchPoints!=null)
         {
            mCurrPatchPoints = null;
         }
      }
      //---------------------------------------
      //---------------------------------------

      void genCurrPatch(int minXVert, int minZVert)
      {
         //fill mCurrPatchPoints with the patch starting at the lower left corner[0,0]
         for (int x = 0; x < PatchLODTemplate.cNumPatchXVerts; x++)
         {
            for (int y = 0; y < PatchLODTemplate.cNumPatchYVerts; y++)
            {
               mCurrPatchPoints[x, y] = TerrainGlobals.getTerrain().getPos(minXVert + x, minZVert + y);
               mCurrPatchNormals[x, y] = TerrainGlobals.getTerrain().getNormal(minXVert + x, minZVert + y);
            }
         }
      }
      void calcErrorMetric(PatchLODTemplate template, int minXVert, int minZVert, ref EditorCore.Statistics.Accumulator accum)
      {
         patch comparePatch = new patch();
         comparePatch.createFromTemplate(template, minXVert, minZVert);

         accum.Clear();

         for (int x = 0; x < PatchLODTemplate.cNumPatchXVerts; x++)
         {
            for (int y = 0; y < PatchLODTemplate.cNumPatchYVerts; y++)
            {
               float u = 0;
               Vector3 nrm = mCurrPatchNormals[x, y];
               Vector3 inrm = -nrm;
               //if (comparePatch.projectPoint(ref mCurrPatchPoints[x, y], out u))
               if (comparePatch.rayCast(ref mCurrPatchPoints[x, y], ref nrm, out u))
               {
                  accum.Add(u);
               }
               else if (comparePatch.rayCast(ref mCurrPatchPoints[x, y], ref inrm, out u))
               {
                  accum.Add(u);
               }
               else
               {
                  //NO INTERSECTION!?!?
               }
            }
         }

         comparePatch = null;
      }
      PatchLODTemplate.ePatchLODLevel chooseBestLevel(EditorCore.Statistics.Accumulator[] errorMetric,PatchLODTemplate.ePatchLODLevel maxLevel, float selectionBias)
      {
         PatchLODTemplate.ePatchLODLevel lowestLOD = maxLevel;
         float lowest = (float)(errorMetric[(int)(maxLevel)].Variance);
         for (int i = (int)(maxLevel)-1; i >= 0; i--)
         {
            if (errorMetric[i].Variance - selectionBias <= lowest)  // "<=" allows lower chunks with same variance to be chosen instead..
            {
               lowest = (float)errorMetric[i].Variance;
               lowestLOD = (PatchLODTemplate.ePatchLODLevel)(i);
            }
         }

         return lowestLOD;
      }
      public void calcLODs()
      {
         calcLODs(PatchLODTemplate.ePatchLODLevel.cLOD_15,0.0f);
      }
      public void calcLODs(PatchLODTemplate.ePatchLODLevel maxLevel, float selectionBias)
      {
         int numXPatches = (int)(TerrainGlobals.getTerrain().getNumXVerts() / PatchLODTemplate.cNumPatchXVerts);
         int numZPatches = (int)(TerrainGlobals.getTerrain().getNumZVerts() / PatchLODTemplate.cNumPatchYVerts);
         mPatchLevels = new PatchLODTemplate.ePatchLODLevel[numXPatches, numZPatches];

         EditorCore.Statistics.Accumulator[] errorMetric = new EditorCore.Statistics.Accumulator[(int)PatchLODTemplate.ePatchLODLevel.cLOD_Count];
         
            
         for (int xPatch = 0; xPatch < numXPatches; xPatch++)
         {
            for (int zPatch = 0; zPatch < numZPatches; zPatch++)
            {
               //generate our current patch
               genCurrPatch(xPatch * PatchLODTemplate.cNumPatchXVerts, zPatch * PatchLODTemplate.cNumPatchYVerts);

               //calculate difference in derivations between us and the patch templates
               for (int i = 0; i < mPatchTemplates.Length; i++)
               {
                  errorMetric[i] = new EditorCore.Statistics.Accumulator(); 
                  calcErrorMetric(mPatchTemplates[i],
                                 xPatch * PatchLODTemplate.cNumPatchXVerts, zPatch * PatchLODTemplate.cNumPatchYVerts,
                                 ref errorMetric[i]);
               }
               
               //choose the best template (given user input..)
               mPatchLevels[xPatch, zPatch] = chooseBestLevel(errorMetric, maxLevel, selectionBias);
            }
         }
         errorMetric = null;
      }

      #region members
      Vector3[,] mCurrPatchPoints = null;
      Vector3[,] mCurrPatchNormals = null;
      PatchLODTemplate[] mPatchTemplates = null;
      PatchLODTemplate.ePatchLODLevel[,] mPatchLevels = null;
      #endregion

      //---------------------------------------
      //---------------------------------------
      public class PatchLODTemplate
      {
         public PatchLODTemplate()
         {

         }
         ~PatchLODTemplate()
         {
            destroy();
         }
         //--------------------------
         public void destroy()
         {
           if(mPatchTiles!=null)
           {
              mPatchTiles = null;
           }
         }
         //--------------------------
         public void construct(ePatchLODLevel overrideLevel)
         {
            destroy();
            mMyLODLevel = overrideLevel;

            //Create our triangle indexes in reference to the 2D local patch space.
            //essentially, these triangles become indicies into our patch.
            //NOTE, we assume patches are 17x17 verts ([0,16] index range)
            //NOTE, we assume that [0,0] is lower-left vertex
            //NOTE, these vertex layouts are mimmicked from the 360 tesselator..
            int stride = 0;
            switch (mMyLODLevel)
            {
            case ePatchLODLevel.cLOD_1: stride = 8; break;
            case ePatchLODLevel.cLOD_3: stride = 4; break;
          //  case ePatchLODLevel.cLOD_5: stride = 4; break;
            case ePatchLODLevel.cLOD_7: stride = 2; break;
           // case ePatchLODLevel.cLOD_9: stride = 2; break;
           // case ePatchLODLevel.cLOD_11: stride = 8; break;
          //  case ePatchLODLevel.cLOD_13: stride = 8; break;
            case ePatchLODLevel.cLOD_15: stride = 1; break;
               
            }

            mNumXTiles = (int)(cNumPatchXVerts / stride);
            mNumZTiles = (int)(cNumPatchYVerts / stride);
            int midXVert = cNumPatchXVerts>>1;
            int midYVert = cNumPatchYVerts>>1;

            mPatchTiles = new patchLODTile[mNumXTiles, mNumZTiles];

            for (int xTile = 0; xTile < mNumXTiles; xTile++)
            {
               int xMinVert = xTile * stride;
               int xMaxVert = xMinVert + stride;

               for (int yTile = 0; yTile < mNumZTiles; yTile++)
               {
                  int yMinVert = yTile * stride;
                  int yMaxVert = yMinVert + stride;

                  //Quadrants
                  // | 3  |  2  |
                  // | 0  |  1  |
                  int quadrant = 0;
                  if(xMaxVert > midXVert)
                  {
                     quadrant = 1;
                     if(yMaxVert > midYVert)
                        quadrant = 2;
                  }
                  else
                  {
                     quadrant = 0;
                     if (yMaxVert > midYVert)
                        quadrant = 3;
                  }


                  mPatchTiles[xTile, yTile] = new patchLODTile();
                  mPatchTiles[xTile, yTile].mMinX = xMinVert;
                  mPatchTiles[xTile, yTile].mMinZ = yMinVert;
                  mPatchTiles[xTile, yTile].mWidth = stride;
                  mPatchTiles[xTile, yTile].mHeight = stride;

                  //put together our two triangles for this patch.
                  if(quadrant ==0 || quadrant == 2)
                  {
                     mPatchTiles[xTile, yTile].mTri0 = new patchLODTri(xMinVert, yMinVert, xMaxVert, yMaxVert, xMaxVert, yMinVert);
                     mPatchTiles[xTile, yTile].mTri1 = new patchLODTri(xMinVert, yMinVert, xMaxVert, yMaxVert, xMinVert, yMaxVert);
                  }
                  else if (quadrant == 1 || quadrant == 3)
                  {
                     mPatchTiles[xTile, yTile].mTri0 = new patchLODTri(xMinVert, yMinVert, xMinVert, yMaxVert, xMaxVert, yMinVert);
                     mPatchTiles[xTile, yTile].mTri1 = new patchLODTri(xMinVert, yMaxVert, xMaxVert, yMaxVert, xMaxVert, yMinVert);
                  }
               }
            }
         }
         //--------------------------
         public class patchLODTri
         {
            public patchLODTri()
            {

            }
            public patchLODTri(int x0, int y0, int x1, int y1, int x2, int y2)
            {
               mPatchVerts[0] = new Point(x0, y0);
               mPatchVerts[1] = new Point(x1, y1);
               mPatchVerts[2] = new Point(x2, y2);
            }
            ~patchLODTri()
            {
               mPatchVerts = null;
            }
            public Point[] mPatchVerts = new Point[3];
         };
         public class patchLODTile
         {
            public int mMinX;
            public int mMinZ;
            public int mWidth;
            public int mHeight;
            public patchLODTri mTri0;
            public patchLODTri mTri1;
         };

         public enum ePatchLODLevel
         {
            cLOD_1 = 0,
            cLOD_3 = 1,
            //cLOD_5 = !,
            cLOD_7 = 2,
          //  cLOD_9 = !,
           // cLOD_11 = !,
          //  cLOD_13 = !,
            cLOD_15 = 3,

            cLOD_Count = 4,
         };

         public patchLODTile[,] mPatchTiles = null;
         public int mNumXTiles = 0;
         public int mNumZTiles = 0;
        

         
         public ePatchLODLevel mMyLODLevel = ePatchLODLevel.cLOD_1;

         public static int cNumPatchXVerts = 17;
         public static int cNumPatchYVerts = 17;

         PatchLODTemplate[] mPatchTemplates = new PatchLODTemplate[(int)PatchLODTemplate.ePatchLODLevel.cLOD_Count];
      };
      
      class patch
      {
         class patchTri
         {
            ~patchTri()
            {
               mVerts = null;
            }
            public Vector3[] mVerts = new Vector3[3];

            public bool projectPoint(ref Vector3 point, out float dist)
            {
               Plane triPlane = Plane.FromPoints(mVerts[0],mVerts[1],mVerts[2]);
               Matrix m = BMathLib.giveProjectMatrixForPlane(triPlane);

               Vector4 pt4 = BMathLib.vec4Transform(ref point, ref m);
               Vector3 pt = new Vector3(pt4.X,pt4.Y,pt4.Z);

               dist = 0;
               if (BMathLib.pointInsideConvexPolygon(ref pt,ref mVerts))
               {
                  dist = Vector3.Length(pt - point);
                  return (true);
               }
               return false;
            }
            public bool rayIntersect(ref Vector3 rayOrig, ref Vector3 rayDir, out float dist)
            {
               Vector3 pt = Vector3.Empty;
               dist = 0;
               if (BMathLib.raySegmentIntersectionTriangle(mVerts, ref rayOrig, ref rayDir, false, ref pt))
               {
                  dist = Vector3.Length(pt - rayOrig);
                  return (true);
               }
               return false;
            }
         };
         class patchTile
         {
            ~patchTile()
            {
               mTri0 = null;
               mTri1 = null;
            }
            public patchTri mTri0 = new patchTri();
            public patchTri mTri1 = new patchTri();

            
            public bool projectPoint(ref Vector3 point, out float dist)
            {
                dist = 0;

               if (mTri0.projectPoint(ref point, out dist))
                  return true;

               if (mTri1.projectPoint(ref point, out dist))
                  return true;

               return false;
            }

            public bool rayIntersect(ref Vector3 rayOrig, ref Vector3 rayDir, out float dist)
            {
               dist = 0;

               if (mTri0.rayIntersect(ref rayOrig, ref rayDir, out dist))
                  return true;

               if (mTri1.rayIntersect(ref rayOrig, ref rayDir, out dist))
                  return true;

               return false;
            }
         };

         patchTile[,] mPatchTiles = null;
         int mNumXTiles = 0;
         int mNumZTiles = 0;
         Vector2 mMinXZ = new Vector2();
         Vector2 mMaxXZ = new Vector2();

         public void createFromTemplate(PatchLODTemplate targetTemplate, int minXVert, int minZVert)
         {
            Vector3 minv= TerrainGlobals.getTerrain().getPos(minXVert, minZVert);
            mMinXZ.X = minv.X;
            mMinXZ.Y = minv.Z;

            mNumXTiles = targetTemplate.mNumXTiles;
            mNumZTiles = targetTemplate.mNumZTiles;

            mPatchTiles = new patchTile[mNumXTiles, mNumZTiles];
            for (int x = 0; x < mNumXTiles; x++)
            {
               for (int y = 0; y < mNumZTiles; y++)
               {
                  mPatchTiles[x, y] = new patchTile();
                  for (int k = 0; k < 3; k++)
                  {
                     mPatchTiles[x, y].mTri0.mVerts[k] = TerrainGlobals.getTerrain().getPos(minXVert + targetTemplate.mPatchTiles[x, y].mTri0.mPatchVerts[k].X,
                                                                                               minZVert + targetTemplate.mPatchTiles[x, y].mTri0.mPatchVerts[k].Y);

                     mPatchTiles[x, y].mTri1.mVerts[k] = TerrainGlobals.getTerrain().getPos(minXVert + targetTemplate.mPatchTiles[x, y].mTri1.mPatchVerts[k].X,
                                                                                               minZVert + targetTemplate.mPatchTiles[x, y].mTri1.mPatchVerts[k].Y);
                  }
               }
            }
         }

         public bool projectPoint(ref Vector3 point, out float u)
         {
            u = 0;
            //CLM SPEED THIS UP!!
            for (int xTile = 0; xTile < mNumXTiles; xTile++)
            {
               for (int zTile = 0; zTile < mNumZTiles; zTile++)
               {
                  if (mPatchTiles[xTile, zTile].projectPoint(ref point, out u))
                  {
                     return true;
                  }
               }
            }
            return false;
         }
         public bool rayCast(ref Vector3 rayOrig, ref Vector3 rayDir, out float u)
         {
            u = 0;
            //CLM SPEED THIS UP!!
            for (int xTile = 0; xTile < mNumXTiles; xTile++)
            {
               for (int zTile = 0; zTile < mNumZTiles; zTile++)
               {
                  if(mPatchTiles[xTile,zTile].rayIntersect(ref rayOrig,ref rayDir,out u))
                  {
                     return true;
                  }
               }
            }
            return false;
         }
      };
      //---------------------------------------
      //---------------------------------------


   }



   public class XTD_Refine
   {

      public bool isChunkHeightmapOnly(int offX, int offZ)
      {
         for (int z = 0; z < mNumXPoints; z++)
         {
            for (int x = 0; x < mNumXPoints; x++)
            {
               if (mMetricRefiner.getMarkedPt(z + offX, x + offZ))
               {
                  Vector3 t = getRelativePt(z + offX, x + offZ);
                  if (t.X != 0.0f || t.Z != 0.0f)
                     return false;
               }
            }
         }

         return true;
      }




      public class StrideHistogram
      {
         public class strideHistoLevel
         {
            public int numVertStride;
            public int numPointsAtLevel;
         }
         ~StrideHistogram()
         {
            destroy();
            
         }

         public void destroy()
         {
            if (mLevels != null)
            {
               for (int i = 0; i < mLevels.Count; i++)
                  mLevels[i] = null;
               mLevels.Clear();
               mLevels = null;
            }
         }

         public List<strideHistoLevel> mLevels = null;
      }   



      public StrideHistogram getStrideHistogram(int offX, int offZ,bool noOverlapBetweenLevels)
      {
         StrideHistogram histo = new StrideHistogram();
         histo.mLevels = new List<StrideHistogram.strideHistoLevel>();

         int level = mNumXPoints;
         int midLevel = level / 2; ;
         int c=0;
         while (level >1)
         {
            StrideHistogram.strideHistoLevel histoLevel = new StrideHistogram.strideHistoLevel();
            histoLevel.numVertStride=level;
            histoLevel.numPointsAtLevel=0;
            
            
            //find out how many in this level match up with our pristine
            for (int z = 0; z < mNumXPoints; z += midLevel)
            {
               for (int x = 0; x < mNumXPoints; x += midLevel)
               {
                  if (mPristineLevels[c].mPristineLayout[x, z] && mMetricRefiner.getMarkedPt(z + offX, x + offZ))
                  {
                     histoLevel.numPointsAtLevel++;
                     if (noOverlapBetweenLevels && c > 0 && mPristineLevels[c - 1].mPristineLayout[x, z])
                     {
                        histoLevel.numPointsAtLevel--;   
                     }
                  }
               }
            }

           

            histo.mLevels.Add(histoLevel);

            level /=2;
            midLevel = level / 2;
            c++;
         }

         return histo;
      }
      public int getMaxStride(int offX, int offZ,float minorityBias)
      {
         XTD_Refine.StrideHistogram histo = getStrideHistogram(offX, offZ, true);

         for (int i = histo.mLevels.Count - 1; i >= 0; i--)
            if (histo.mLevels[i].numPointsAtLevel != 0 && 
                histo.mLevels[i].numPointsAtLevel > mPristineLevels[i].numUniquePointsAtLevel*minorityBias)
               return histo.mLevels[i].numVertStride;

         return 1;

         /*
         mRootX = offX;
         mRootZ = offZ;
         int maxStride = 0;
         for (int z = 0; z < mNumXPoints; z++)
         {
            int counter = 0;
            for (int x = 0; x < mNumXPoints; x++)
            {
               if (getMarked(z, x))
                  counter++;
            }
            if (counter > maxStride)
               maxStride = counter;
         }

         for (int z = 0; z < mNumXPoints; z++)
         {
            int counter = 0;
            for (int x = 0; x < mNumXPoints; x++)
            {
               if (getMarked(x, z))
                  counter++;
            }
            if (counter > maxStride)
               maxStride = counter;
         }

         
         return maxStride;*/
      }

      void recalcNormals(int offX, int offZ)
      {
         //clear our normals for this block
         //calculate the polygon normal, add it to each vertex
         //normalize all points in this block.
         //CLM - this is going to give us problems at the edges
      }

      //-------------------------------------------------
      //-------------------------------------------------

      ushort findBlockType(int x, int z, ref bool xflip,ref bool zflip, ref bool trans)
      {
         //create our hash value for this block
         ushort val=0;
         for(int i=0;i<3;i++)
         {
            for(int k=0;k<3;k++)
            {
               val |= (ushort)(usedVals[i][k]?1:0);
               val = (ushort)(val << 1);
            }
         }
         val = (ushort)(val>>1);

         //direct lookup of our translation and flipppage.

          ushort blockType=0;

    
         //CLM - a case lookup is much faster than a hash table
         //THESE NUMBERS DONE BY HAND. DO NOT FUCK WITH UNLESS YOU KNOW WHAT THEY ARE DOING!!
         switch(val)
         {
            case 0x0145:   blockType=0;   xflip=false; zflip=false; trans=false;  break;

            case 0x0155:   blockType=1;   xflip=false; zflip=false; trans=false;  break;


            case 0x0157:   blockType=2;   xflip=false; zflip=false; trans=false;  break;
            case 0x015D:   blockType=2;   xflip=false; zflip=false; trans=true;  break;
            case 0x0175:   blockType=2;   xflip=true; zflip=false; trans=true;  break;
            case 0x01D5:   blockType=2;   xflip=false; zflip=true; trans=false;  break;
              

            case 0x01F5:   blockType=3;   xflip=false; zflip=false; trans=false;  break;
            case 0x01DD:   blockType=3;   xflip=true; zflip=false; trans=false;  break;
            case 0x0177:   blockType=3;   xflip=false; zflip=true; trans=false;  break;
            case 0x015F:   blockType=3;   xflip=true; zflip=true; trans=false;  break;


            case 0x01D7:   blockType=4;   xflip=false; zflip=false; trans=false;  break;
            case 0x017D:   blockType=4;   xflip=false; zflip=false; trans=true;  break;


            case 0x01F7:   blockType=5;   xflip=false; zflip=false; trans=false;  break;
            case 0x01DF:   blockType=5;   xflip=true; zflip=false; trans=false;  break;
            case 0x017F:   blockType=5;   xflip=false; zflip=true; trans=true;  break;
            case 0x01FD:   blockType=5;   xflip=false; zflip=false; trans=true;  break;


            case 0x01FF:   blockType=6;   xflip=false; zflip=false; trans=false;  break;

            default:
               blockType=0;
               break;
            
         }

         return blockType;
      }
      void assignUsedVals(int rootX, int rootZ, int blockSize)
      {
         int max = blockSize;
         int mid = blockSize /2;

         for(int j=0;j<3;j++)
         {
            for(int i=0;i<3;i++)
            {
               usedVals[j][i] = mMetricRefiner.getMarkedPt(rootX + (j * mid), rootZ + (i * mid));
            }
         }
      }
      int triIndx(bool xflip, bool zflip, bool trans, int max, int mid,    int x, int z,       int i, int j)
      {
         return (trans)?(z+(xflip?max-j:j)) + ((x+(zflip?max-i:i))*mNumXPoints) : (z+(xflip?max-i:i)) + ((x+(zflip?max-j:j))*mNumXPoints);
      }
      void giveTris(int x, int z,int blockSize,bool isLeaf)
      {
         assignUsedVals(x, z, blockSize);

         //find the matching tile for this 3x3 block
         bool xf=false;
         bool zf=false;
         bool tr  = false;
         int blocktype = findBlockType(x, z, ref xf,ref zf, ref tr);

         

         //now that we've found our block, add appropriate triangles
         int width = mNumXPoints - 1;
         int vd = mNumXPoints;
         int tw = width;
         int td = width;

         int max = blockSize;
         int mid = blockSize /2;

         
         tri t0;


         //#define triIndx(i,j)(z+i) + ((x+j)*vd)
       //  #define triIndx(xf,   zf,   tr,   max,   mid,   i,j)(trans)?(z+(xflip?max-j:j)) + ((x+(zflip?max-i:i))*vd) : (z+(xflip?max-i:i)) + ((x+(zflip?max-j:j))*vd)

        
            switch(blocktype)
            {
            case 0:
               {
                  int m = x%(2*blockSize);
                  int n = z%(2*blockSize);
                  if(m ==n)
                  {

                     t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,   x,z,   0,0);
                     t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,max);
                     t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,0);
                     mTriList.Add(t0);;

                     t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       0,0);
                     t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       0,max);
                     t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,max);
                     mTriList.Add(t0);;

                  }
                  else
                  {
                     t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       0,0);
                     t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       0,max);
                     t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,0);
                     mTriList.Add(t0);;

                     t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,0);
                     t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       0,max);
                     t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,max);
                     mTriList.Add(t0);;
                  }

                  
                  
               
               }
               break;
            case 1:
               {

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
               mTriList.Add(t0);;

               t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
               t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       mid,mid);
               t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,0);
               mTriList.Add(t0);;

               t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,0);
               t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       mid,mid);
               t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,max);
               mTriList.Add(t0);;

               t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       0,max);
               t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       mid,mid);
               t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,max);
               mTriList.Add(t0);
               }

               break;
            case 2:
               {
                  t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       0,0);
                  t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       mid,mid);
                  t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,0);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,0);
                  t0.i1 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       mid,mid);
                  t0.i2 = triIndx(xf,   zf,   tr,   max,   mid,      x,z,       max,max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;

               }
               break;
            case 3:
               {
                  
                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, mid);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  mTriList.Add(t0);;

                  if(isLeaf)
                  {
                     t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, mid);
                     t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                     t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                     mTriList.Add(t0);;

                     t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                     t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                     t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                     mTriList.Add(t0);;
                  }
                 
               }
               break;
            case 4:
               {
                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;
               }
               break;
            case 5:
               {

                  
                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  mTriList.Add(t0);;


            
                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;

                  if(isLeaf)
                  {
                     t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, mid);
                     t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                     t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                     mTriList.Add(t0); ;



                     t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                     t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                     t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                     mTriList.Add(t0);;

                     t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                     t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                     t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, mid);
                     mTriList.Add(t0);;

                     t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                     t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                     t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                     mTriList.Add(t0);;
                  }
                  
 
               }
               break;
            case 6:
               {
                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, max, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, mid);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, max, mid);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, mid, max);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, max, max);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, 0);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, mid);
                  mTriList.Add(t0);;

                  t0.i0 = triIndx(xf, zf, tr, max, mid, x, z, 0, mid);
                  t0.i1 = triIndx(xf, zf, tr, max, mid, x, z, mid, mid);
                  t0.i2 = triIndx(xf, zf, tr, max, mid, x, z, 0, max);
                  mTriList.Add(t0);;
               }
               break;
            }
         
       

      }
      int buildBlocks(int offX, int offZ, int rootX, int rootZ, int blockSize)
      {
         int max = blockSize;
         int mid = blockSize /2;

         if(mid==0) return 0;

         bool cornersValid = (mMetricRefiner.getMarkedPt(offX + rootX, offZ + rootZ)) &&
                             (mMetricRefiner.getMarkedPt(offX + rootX, offZ + rootZ + max)) &&
                             (mMetricRefiner.getMarkedPt(offX + rootX + max, offZ + rootZ)) &&
                             (mMetricRefiner.getMarkedPt(offX + rootX + max, offZ + rootZ + max));
         bool centerValid = mMetricRefiner.getMarkedPt(offX + rootX + mid, offZ + rootZ + mid);
         


         if(cornersValid)
         {
            if(!centerValid)  //we're a type 0 block
            {
               giveTris(offX + rootX, offZ + rootZ, blockSize, true);
               return 1;
            }
            else
            {


               int a = (buildBlocks(offX, offZ , rootX, rootZ, mid));
               int b = (buildBlocks(offX, offZ , rootX, rootZ + mid, mid));
               int c = (buildBlocks(offX, offZ,  rootX + mid,  rootZ, mid));
               int d = (buildBlocks(offX, offZ,  rootX + mid,  rootZ + mid, mid));

               int kidCount = a+b+c+d;
               if(kidCount==0)  //I have no kids, assign me.
               {
                  giveTris(offX + rootX, offZ + rootZ, blockSize, true);
               }
               else if(kidCount !=4)   //we've got some odd mixing
               {
                  giveTris(offX + rootX, offZ + rootZ, blockSize, false);
               }



               return 1;
            }
         }

         return 0;
      }

      //GENERATE THE IB FOR A REFINED TERRAIN CHUNK (assuming texture based rendering)
      public unsafe void genChunkInds(int offX, int offZ, ref int[] outInds, ref int outIndCount, ref int outPrimCount)
      {
         mTriList = new List<tri>(mNumXPoints * mNumXPoints * 6);

         buildBlocks(offX, offZ, 0, 0, mNumXPoints - 1);

         outPrimCount = mTriList.Count;
         outIndCount = mTriList.Count * 3;
         outInds = new int[outIndCount];
         int c = 0;
         for (int i = 0; i < mTriList.Count; i++)
         {
            outInds[c++] = mTriList[i].i0;
            outInds[c++] = mTriList[i].i1;
            outInds[c++] = mTriList[i].i2;
         }


         mTriList.Clear();
         mTriList = null;

      }
      //-----------------------------------------
      
      //GENERATE THE VB FOR A REFINED TERRAIN CHUNK (assuming texture based rendering)
      public unsafe void genChunkVerts(int offX, int offZ, int numXPoints, ref Vector3[] outVerts, ref Vector3[] normals, ref float[] ambOcclu, ref bool outHeightMapOnly, ref int maxVertStride,ref Vector3 min, ref Vector3 max)
      {

         maxVertStride = BMathLib.nexPow2(getMaxStride(offX,offZ,0.5f));
         outHeightMapOnly = isChunkHeightmapOnly(offX,offZ);

         outVerts = new Vector3[maxVertStride * maxVertStride];
         normals = new Vector3[maxVertStride * maxVertStride];
         ambOcclu = new float[maxVertStride * maxVertStride];

         min = new Vector3(9000, 9000, 9000);
         max = new Vector3(-9000, -9000, -9000);
         int counter = 0;
         for (int z = 0; z < numXPoints; z++)
         {
            for (int x = 0; x < numXPoints; x++)
            {
               if (mMetricRefiner.getMarkedPt(z + offX, x + offZ))
               {
                  outVerts[counter] = getRelativePt(z, x);

                  normals[counter] = getRelativeNormal(z, x);
                  ambOcclu[counter] = getAmbOcclu(z, x);

                  Vector3 v = getPt(z, x);
                  if (v.X < min.X) min.X = v.X;
                  if (v.Y < min.Y) min.Y = v.Y;
                  if (v.Z < min.Z) min.Z = v.Z;
                  if (v.X > max.X) max.X = v.X;
                  if (v.Y > max.Y) max.Y = v.Y;
                  if (v.Z > max.Z) max.Z = v.Z;
                  counter++;
               }
            }
         }
      }
      //-------------------------------------------------




      //-------------------------------------------------
      //-------------------------------------------------

      Vector3 getPt(int x, int z)
      {
         return TerrainGlobals.getTerrain().getPos(x, z);
      }
      Vector3 getRelativePt(int x, int z)
       {
          return TerrainGlobals.getTerrain().getRelPos(x, z);
       }
      Vector3 getRelativeNormal(int x, int z)
      {
         return TerrainGlobals.getTerrain().getNormal(x, z);
      }
      float getAmbOcclu(int x, int z)
      {
         return TerrainGlobals.getTerrain().getAmbientOcclusion(x, z);
      }
      

      public unsafe void refineTerrain(float metric, int numXPoints)
      {
         mMetricRefiner = new ErrorMetricRefine();
         mMetricRefiner.init(numXPoints, numXPoints);

         mNumXPoints = numXPoints;

         //mark our chunk transition points
         for (int mRootX = 0; mRootX < mWorldNumXPoints; mRootX += mNumXPoints - 1)
         {
            for (int mRootZ = 0; mRootZ < mWorldNumXPoints; mRootZ += mNumXPoints - 1)
            {
               mMetricRefiner.setMarkedPt(mRootX + 0,                mRootZ + 0, true);
               mMetricRefiner.setMarkedPt(mRootX + 0,                mRootZ + (mNumXPoints - 1), true);
               mMetricRefiner.setMarkedPt(mRootX + (mNumXPoints - 1),  mRootZ + 0, true);
               mMetricRefiner.setMarkedPt(mRootX + (mNumXPoints - 1),  mRootZ + (mNumXPoints - 1), true);
            }
         }

         //now, refine the rest of the world
         mMetricRefiner.refine(metric);


         fillPristineLevels();
      }

      //-------------------------------------------------
      //-------------------------------------------------

      public XTD_Refine()
      {

         mWorldNumXPoints = TerrainGlobals.getTerrain().getNumXVerts() + 1;
         

         usedVals = new bool[3][];
         for (int i = 0; i < 3; i++)
            usedVals[i] = new bool[3];


      }

      ~XTD_Refine()
      {
         destroy();
      }

      public void destroy()
      {

         usedVals = null;

         if (mPristineLevels != null)
         {
            for (int i = 0; i < mPristineLevels.Count; i++)
               mPristineLevels[i] = null;
            mPristineLevels.Clear();
            mPristineLevels = null;
         }


         if (mTriList != null)
         {
            mTriList.Clear();
            mTriList = null;
         }
      }
      //----------------------------------------------------------------------
      

      void fillPristineLevels()
      {
         mPristineLevels = new List<pristineLevel>();
         int level = mNumXPoints;
         int midLevel = level / 2; ;
         while (level > 1)
         {
            mPristineLevels.Add(new pristineLevel());
            pristineLevel prsLevel = mPristineLevels[mPristineLevels.Count-1];
            prsLevel.numPointsOverlapWithPrevLevel = 0;
            prsLevel.numUniquePointsAtLevel = 0;
            prsLevel.numVertStride = level;

            //fill our pristine layout
            prsLevel.mPristineLayout = new bool[mNumXPoints, mNumXPoints];
            for (int i = 0; i < mNumXPoints; i++)
               for (int j = 0; j < mNumXPoints; j++)
                  prsLevel.mPristineLayout[i, j] = false;

            for (int z = 0; z < mNumXPoints; z += midLevel)
            {
               for (int x = 0; x < mNumXPoints; x += midLevel)
               {
                  prsLevel.mPristineLayout[x, z] = true;
                  prsLevel.numUniquePointsAtLevel++;
                  if (mPristineLevels.Count > 0)
                  {
                     if (mPristineLevels[mPristineLevels.Count - 1].mPristineLayout[x, z])
                     {
                        prsLevel.numPointsOverlapWithPrevLevel++;
                        prsLevel.numUniquePointsAtLevel--;
                     }
                  }
               }
            }

            level /= 2;
            midLevel = level / 2;
         }
      }


      //----------------------------------------------------------------------
      class pristineLevel
      {
         ~pristineLevel()
         {
            destroy();
         }
         public void destroy()
         {
            mPristineLayout = null;
         }
         public bool[,] mPristineLayout;
         public int numPointsOverlapWithPrevLevel;
         public int numUniquePointsAtLevel;
         public int numVertStride;
      }

      List<pristineLevel> mPristineLevels =null;


      [StructLayout(LayoutKind.Sequential)]
      private struct tri
      {
         public int i0;
         public int i1;
         public int i2;
      };
      List<tri>         mTriList=null;

      private bool[][]  usedVals = null;

      private int       mWorldNumXPoints = 0;

      private int       mNumXPoints =0;

      ErrorMetricRefine mMetricRefiner = new ErrorMetricRefine();

      public class vHolder
      {
         public vHolder(Vector3 v, Vector3 n, float a, int ax, int az)
         {
            vert = v; normal = n; ao = a;
            x = ax;
            z = az;
         }
         public Vector3 vert;
         public Vector3 normal;
         public float ao;
         public int x;
         public int z;
      }

   }



   public class XTD_DeulanyTIN
   {
      ~XTD_DeulanyTIN()
      {
         mVertexList.Clear();
         mVertexList = null;
         mVertConnList.Clear();
         mVertConnList = null;
         mTriangleList.Clear();
         mTriangleList = null;
      }
      public class vertexConnectivity
      {
         public List<triangle> mTrisUsingMe = new List<triangle>();
      }
      public class triangle
      {
         public int[] vertIndex = new int[3] { 0, 0, 0 };

         public triangle[] sharedTriEdge = new triangle[3] { null, null, null };
         
         public int myIndex;

         public int giveSharedTriEdge(triangle b)
         {
            if (sharedTriEdge[0] == b) return 0;
            if (sharedTriEdge[1] == b) return 1;
            if (sharedTriEdge[2] == b) return 2;

            return -1;
         }
         public void clearEdgeConnection(triangle b)
         {
            int i = giveSharedTriEdge(b);
            if (i == -1) return;

            sharedTriEdge[i] = null;
         }
      }

      List<Vector3> mVertexList = new List<Vector3>();
      List<vertexConnectivity> mVertConnList = new List<vertexConnectivity>();
      List<triangle> mTriangleList = new List<triangle>();

      Vector2 mMinBounds = new Vector2(float.MaxValue, float.MaxValue);
      Vector2 mMaxBounds = new Vector2(float.MinValue, float.MinValue);
      Vector2 mMidBounds = Vector2.Empty;

      public void addVertex(Vector3 vertPos)
      {
         mVertexList.Add(vertPos);
         mVertConnList.Add(new vertexConnectivity());

         addBounds(vertPos);
      }
      void addBounds(Vector3 vertPos)
      {
         if (vertPos.X < mMinBounds.X) mMinBounds.X = vertPos.X;
         if (vertPos.Z < mMinBounds.Y) mMinBounds.Y = vertPos.Z;

         if (vertPos.X > mMaxBounds.X) mMaxBounds.X = vertPos.X;
         if (vertPos.Z > mMaxBounds.Y) mMaxBounds.Y = vertPos.Z;

         mMidBounds.X = (mMaxBounds.X + mMinBounds.X) * 0.5f;
         mMidBounds.Y = (mMaxBounds.Y + mMinBounds.Y) * 0.5f;
      }
      //----------------------
      void addTri(int v0, int v1, int v2)
      {
         triangle tt = new triangle();
         tt.vertIndex[0] = v0;
         tt.vertIndex[1] = v1;
         tt.vertIndex[2] = v2;
         tt.myIndex = -1;
         mTriangleList.Add(tt);

         vertAddConnectivity(tt.vertIndex[0], mTriangleList[mTriangleList.Count - 1]);
         vertAddConnectivity(tt.vertIndex[1], mTriangleList[mTriangleList.Count - 1]);
         vertAddConnectivity(tt.vertIndex[2], mTriangleList[mTriangleList.Count - 1]);
      }
      void removeTri(int triIndex)
      {
         vertRemoveConnectivity(mTriangleList[triIndex].vertIndex[0], mTriangleList[triIndex]);
         vertRemoveConnectivity(mTriangleList[triIndex].vertIndex[1], mTriangleList[triIndex]);
         vertRemoveConnectivity(mTriangleList[triIndex].vertIndex[2], mTriangleList[triIndex]);

         mTriangleList.RemoveAt(triIndex);
      }
      //----------------------
      void addVert(float x, float y, float z)
      {
         mVertexList.Add(new Vector3(x,y,z));
         mVertConnList.Add(new vertexConnectivity());
      }
      void removeVert(int index)
      {
         mVertexList.RemoveAt(index);
         mVertConnList.RemoveAt(index);
      }
      void vertRemoveConnectivity(int vertIndex,triangle triToRemove)
      {
         if (vertIndex <= 0 || vertIndex >= mVertexList.Count)
            return;

         mVertConnList[vertIndex].mTrisUsingMe.Remove(triToRemove);
      }
      void vertAddConnectivity(int vertIndex, triangle triToAdd)
      {
         if (vertIndex <= 0 || vertIndex >= mVertexList.Count)
            return;
         mVertConnList[vertIndex].mTrisUsingMe.Add(triToAdd);
      }
      //----------------------
    
    

      //----------------------
      int WhichSide(ref float xp, ref float yp, ref float x1, ref float y1, ref float x2, ref float y2)
      {
         // Determines which side of a line the point (xp,yp) lies.
         // The line goes from (x1,y1) to (x2,y2)
         // Returns -1 for a point to the left
         //          0 for a point on the line
         //         +1 for a point to the right
         double equation;
         equation = ((yp - y1) * (x2 - x1)) - ((y2 - y1) * (xp - x1));
         if (equation > 0)
         {
            return -1;
         }
         else if (equation == 0)
         {
            return 0;
         }
         else
         {
            return 1;
         }
      }
      bool InCircle(ref Vector3 point, triangle tri, ref double xc, ref double yc, ref double r)
      {
         float xp = point.X;
         float yp = point.Z;
         float x1 = mVertexList[tri.vertIndex[0]].X;
         float y1 = mVertexList[tri.vertIndex[0]].Z;
         float x2 = mVertexList[tri.vertIndex[1]].X;
         float y2 = mVertexList[tri.vertIndex[1]].Z;
         float x3 = mVertexList[tri.vertIndex[2]].X;
         float y3 = mVertexList[tri.vertIndex[2]].Z;

         //Return TRUE if the point (xp,yp) lies inside the circumcircle
         //made up by points (x1,y1) (x2,y2) (x3,y3)
         //The circumcircle centre is returned in (xc,yc) and the radius r
         //NOTE: A point on the edge is inside the circumcircle
         bool TheResult;

         double eps;
         double m1;
         double m2;
         double mx1;
         double mx2;
         double my1;
         double my2;
         double dx;
         double dy;
         double rsqr;
         double drsqr;

         TheResult = false;
         eps = 0.000001;

         if (System.Math.Abs(y1 - y2) < eps && System.Math.Abs(y2 - y3) < eps)
         {
            //MessageBox.Show("INCIRCUM - F - Points are coincident !!");
            TheResult = false;
            return TheResult;
         }


         if (Math.Abs(y2 - y1) < eps)
         {
            m2 = (double)-(x3 - x2) / (y3 - y2);
            mx2 = (double)(x2 + x3) / 2;
            my2 = (double)(y2 + y3) / 2;
            xc = (x2 + x1) / 2;
            yc = m2 * (xc - mx2) + my2;
         }
         else if (Math.Abs(y3 - y2) < eps)
         {
            m1 = (double)-(x2 - x1) / (y2 - y1);
            mx1 = (double)(x1 + x2) / 2;
            my1 = (double)(y1 + y2) / 2;
            xc = (x3 + x2) / 2;
            yc = m1 * (xc - mx1) + my1;
         }
         else
         {
            m1 = (double)-(x2 - x1) / (y2 - y1);
            m2 = (double)-(x3 - x2) / (y3 - y2);
            mx1 = (double)(x1 + x2) / 2;
            mx2 = (double)(x2 + x3) / 2;
            my1 = (double)(y1 + y2) / 2;
            my2 = (double)(y2 + y3) / 2;
            xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
            yc = m1 * (xc - mx1) + my1;
         }

         dx = x2 - xc;
         dy = y2 - yc;
         rsqr = dx * dx + dy * dy;
         r = Math.Sqrt(rsqr);
         dx = xp - xc;
         dy = yp - yc;
         drsqr = dx * dx + dy * dy;

         if (drsqr <= rsqr)
         {
            TheResult = true;

         }
         return TheResult;

      }
      //----------------------
      public void Triangulate()
      {

         // Takes as input mVertexList.Count vertices in arrays Vertex()
         // Returned is a list of NTRI triangular faces in the array
         // Triangle(). These triangles are arranged in clockwise order.
         mTriangleList.Clear();
         List<Point> Edges = new List<Point>();


         // For Super Triangle
         double dx = mMaxBounds.X - mMinBounds.X;
         double dy = mMaxBounds.Y - mMinBounds.Y;
         double dmax = Math.Max(dx, dy);

         double xc = 0;
         double yc = 0;
         double r = 0;


         // Set up the supertriangle
         // This is a triangle which encompasses all the sample points.
         // The supertriangle coordinates are added to the end of the
         // vertex list. The supertriangle is the first triangle in
         // the triangle list.

         int nvert = mVertexList.Count;
         addVert((float)(mMidBounds.X - 2 * dmax), 0, (float)(mMidBounds.Y - dmax));
         addVert((float)mMidBounds.X, 0, (float)(mMidBounds.Y + 2 * dmax));
         addVert((float)(mMidBounds.X + 2 * dmax), 0, (float)(mMidBounds.Y - dmax));

         addTri(nvert + 0, nvert + 1, nvert + 2);

         // Include each point one at a time into the existing mesh
         for (int v = 0; v < nvert; v++)
         {
            //gather the edges of the triangles that enclose me
            Edges.Clear();

            for (int j = 0; j < mTriangleList.Count; j++)
            {
               //if this tri encloses the point
               Vector3 p = mVertexList[v];
               if (InCircle(ref p, mTriangleList[j], ref xc, ref yc, ref r))
               {
                  //add it's edges to the list
                  Edges.Add(new Point(mTriangleList[j].vertIndex[0], mTriangleList[j].vertIndex[1]));
                  Edges.Add(new Point(mTriangleList[j].vertIndex[1], mTriangleList[j].vertIndex[2]));
                  Edges.Add(new Point(mTriangleList[j].vertIndex[2], mTriangleList[j].vertIndex[0]));

                  //remove it from the list
                  removeTri(j);
                  j--;
               }

            }

            //delete duplicate edges.. this will leave just the outer convex hull edges of the containment
            int q = 0;
            for (int k = 0; k < Edges.Count; k++)
            {
               bool flagToDel = false;
               for (q = k + 1; q < Edges.Count; q++)
               {
                  if (Edges[k].X == Edges[q].X && Edges[k].Y == Edges[q].Y ||
                     Edges[k].X == Edges[q].Y && Edges[k].Y == Edges[q].X)
                  {
                     flagToDel = true;
                     break;
                  }
               }

               if (flagToDel)
               {
                  Edges.RemoveAt(q);
                  Edges.RemoveAt(k);
                  k--;
               }
            }

            //form new triangles based upon edges & our added point
            //CLM i believe this winds incorrectly.. is it the responsibility of this method to handle that?
            for (int e = 0; e < Edges.Count; e++)
            {
               addTri(Edges[e].X, Edges[e].Y, v);
            }
         }



         //remove any triangles using the super-tri
         for (int k = 0; k < mTriangleList.Count; k++)
         {
            if (mTriangleList[k].vertIndex[0] >= nvert ||
               mTriangleList[k].vertIndex[1] >= nvert ||
               mTriangleList[k].vertIndex[2] >= nvert)
            {
               removeTri(k);
               k--;
            }
         }


         ////remove our super triangle..
         removeVert(mVertexList.Count - 1);
         removeVert(mVertexList.Count - 1);
         removeVert(mVertexList.Count - 1);

         Edges = null;

         generateEdgeConnections();
      }
      //----------------------
      bool sameEdge(int a0, int a1, int b0, int b1)
      {
         return ((a0 == b0 || a0 == b1) && (a1 == b0 || a1 == b1));
      }
      int sharedEdgeIndex(triangle a, triangle b)
      {
         if (a == null || b == null)
            return -1;

         //return the edge relative to A
         if (sameEdge(a.vertIndex[0], a.vertIndex[1], b.vertIndex[0], b.vertIndex[1])||
             sameEdge(a.vertIndex[0], a.vertIndex[1], b.vertIndex[1], b.vertIndex[2]) ||
             sameEdge(a.vertIndex[0], a.vertIndex[1], b.vertIndex[2], b.vertIndex[0]))
            return 0;

         if (sameEdge(a.vertIndex[1], a.vertIndex[2], b.vertIndex[0], b.vertIndex[1]) ||
             sameEdge(a.vertIndex[1], a.vertIndex[2], b.vertIndex[1], b.vertIndex[2]) ||
             sameEdge(a.vertIndex[1], a.vertIndex[2], b.vertIndex[2], b.vertIndex[0]))
            return 1;

         if (sameEdge(a.vertIndex[2], a.vertIndex[0], b.vertIndex[0], b.vertIndex[1]) ||
             sameEdge(a.vertIndex[2], a.vertIndex[0], b.vertIndex[1], b.vertIndex[2]) ||
             sameEdge(a.vertIndex[2], a.vertIndex[0], b.vertIndex[2], b.vertIndex[0]))
            return 2;

         return -1;
      }
      void setSharedEdge(triangle a, triangle b)
      {
         int edgeIndexA = sharedEdgeIndex(a, b);
         if (edgeIndexA == -1)
            return;

         int edgeIndexB = sharedEdgeIndex(b, a);

         a.sharedTriEdge[edgeIndexA] = b;
         b.sharedTriEdge[edgeIndexB] = a;
         
      }
      bool connected(triangle a, triangle b)
      {
         return (sharedEdgeIndex(a, b) != -1 || sharedEdgeIndex(a, b) != -1);
      }
      void generateEdgeConnections()
      {
         for(int i=0;i<mVertConnList.Count;i++)
         {
            for (int j = 0; j < mVertConnList[i].mTrisUsingMe.Count; j++)
            {
               for (int k = 0; k < mVertConnList[i].mTrisUsingMe.Count; k++)
               {
                  if (k == j) continue;
                  setSharedEdge(mVertConnList[i].mTrisUsingMe[j], mVertConnList[i].mTrisUsingMe[k]);
               }
            }
         }

         //do a double check here
         //for(int i=0;i<mTriangleList.Count;i++)
         //{
         //   Debug.Assert(        connected(mTriangleList[i],mTriangleList[i].sharedTriEdge[0]) ||
         //                        connected(mTriangleList[i],mTriangleList[i].sharedTriEdge[1]) ||
         //                        connected(mTriangleList[i],mTriangleList[i].sharedTriEdge[2]));
            
         //}
      }
      //---------------------------
      public triangle getTriangle(int index)
      {
         if (index < 0 || index > mTriangleList.Count)
            return null;

         return mTriangleList[index];
      }
      public List<triangle> getTriList()
      {
         return mTriangleList;
      }
      public List<Vector3> getVertList()
      {
         return mVertexList;
      }
      public Vector3 getVertex(int index)
      {
         if (index < 0 || index > mVertexList.Count)
            return Vector3.Empty;

         return mVertexList[index];
      }
      public int getNumTris()
      {
         return mTriangleList.Count;
      }

   }
}