//#define DEBUG_AMBIENT_OCCLUSION


using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Threading;
using System.Runtime.InteropServices;
using System.Diagnostics;


using EditorCore;
using Rendering;


//----------------------------------

namespace Terrain
{
   namespace Refinement
   {
      public class ErrorMetricRefine
      {
         //-------------------------------
         bool[,] mMarkedGrid = null;
         int mWidth = 0;
         int mHeight = 0;

         private int mRootX;
         private int mRootZ;
         //-------------------------------
         ~ErrorMetricRefine()
         {
            destroy();
         }
         public void destroy()
         {
            if(mMarkedGrid!=null)
            {
               mMarkedGrid = null;
            }
         }
         //-------------------------------
         void clearMarked()
         {
            if (mMarkedGrid==null)
            {
               mMarkedGrid = new bool[mWidth, mHeight];
            }
            for(int x =0; x < mWidth; x++)
            {
               for (int y = 0; y < mHeight; y++)
               {
                  mMarkedGrid[x, y] = false;
               }
            }
         }
         //-------------------------------
         public void init(int width, int height)
         {
            mWidth = width;
            mHeight = height;
            clearMarked();
         }
         public void refine(float worldSpaceDistanceMetric)
         {
            if(mMarkedGrid==null)
               return;

            mRootX = 0;
            mRootZ = 0;

            setMarked(worldSpaceDistanceMetric);
         }
         //-------------------------------

         bool isCenterVert(int x, int z, int blockSize)
         {
            //return true if this point is the CENTER OF THE CURRENT BLOCK
            if (x % blockSize != 0 && z % blockSize != 0)
               return true;

            return false;
         }
         bool isHorizEdge(int x, int z, int blockSize)
         {
            if ((x % blockSize) == 0)
               return true;

            return false;
         }
         bool isVerticalEdge(int x, int z, int blockSize)
         {
            if ((z % blockSize) == 0)
               return true;

            return false;
         }
         void markPointDependents(int x, int z, int blockSize)
         {
            markPoint(x, z, true);

            int bk = 2 * blockSize;
            int bz = blockSize / 2;

            if (isCenterVert(x, z, blockSize))
            {
               markPoint(x - bz, z - bz, true);
               markPoint(x + bz, z - bz, true);
               markPoint(x + bz, z + bz, true);
               markPoint(x - bz, z + bz, true);
            }
            else if (isHorizEdge(x, z, blockSize))
            {
               markPoint(x + bz, z, true);
               markPoint(x - bz, z, true);
            }
            else if (isVerticalEdge(x, z, blockSize))
            {
               markPoint(x, z + bz, true);
               markPoint(x, z - bz, true);
            }
         }
         void evalBlockLine(int x, int z, float metric, int blockSize)
         {
            int max = blockSize;
            int mid = blockSize / 2;

            int level = blockSize >> 1;


            //TOP
            if (getMarked(x + mid, z + max) || BMathLib.pointLineDistance(getPt(x, z + max), getPt(x + max, z + max), getPt(x + mid, z + max)) > metric)
               markPointDependents(x + mid, z + max, blockSize);

            //BOTTOM
            if (getMarked(x + mid, z) || BMathLib.pointLineDistance(getPt(x, z), getPt(x + max, z), getPt(x + mid, z)) > metric)
               markPointDependents(x + mid, z, blockSize);

            //LEFT
            if (getMarked(x, z + mid) || BMathLib.pointLineDistance(getPt(x, z), getPt(x, z + max), getPt(x, z + mid)) > metric)
               markPointDependents(x, z + mid, blockSize);

            //RIGHT
            if (getMarked(x + max, z + mid) || BMathLib.pointLineDistance(getPt(x + max, z), getPt(x + max, z + max), getPt(x + max, z + mid)) > metric)
               markPointDependents(x + max, z + mid, blockSize);

            //MIDDLE
            if (getMarked(x + mid, z + mid) ||
               getMarked(x + mid, z + max) ||
               getMarked(x + mid, z) ||
               getMarked(x, z + mid) ||
               getMarked(x + max, z + mid) ||
               (BMathLib.pointLineDistance(getPt(x + mid, z), getPt(x + mid, z + max), getPt(x + mid, z + mid)) > metric) ||
               (BMathLib.pointLineDistance(getPt(x, z + mid), getPt(x + max, z + mid), getPt(x + mid, z + mid)) > metric))
               markPointDependents(x + mid, z + mid, blockSize);
         }
         void setMarked(float metric)
         {
            int blockSize = 2;

            for (int k = 2; k < mWidth; k *= 2)
            {
               blockSize = k;
               //visit non-'center point' verts first
               for (int x = 0; x < mWidth - blockSize; x += blockSize)
               {
                  for (int z = 0; z < mHeight - blockSize; z += blockSize)
                  {
                     evalBlockLine(z, x, metric, blockSize);
                  }
               }
            }
         }
         //-----------------------------
         bool getMarked(int x, int z)
         {
            if (x >= 0 && z >= 0 && mRootX + x < mWidth && mRootZ + z < mHeight)
               return mMarkedGrid[mRootX + x,mRootZ + z];

            return false;
         }
         void markPoint(int x, int z, bool val)
         {
            if (x >= 0 && z >= 0 && mRootX + x < mWidth && mRootZ + z < mHeight)
               mMarkedGrid[mRootX + x,mRootZ + z] = val;
         }
         Vector3 getPt(int x, int z)
         {
            return TerrainGlobals.getTerrain().getPos(mRootX + x, mRootZ + z);
         }

         //-----------------------------
         public bool getMarkedPt(int x, int z)
         {
            if (x >= 0 && z >= 0 && x < mWidth && z < mHeight)
               return mMarkedGrid[ x, z];

            return false;
         }
         public void setMarkedPt(int x, int z, bool val)
         {
            if (x >= 0 && z >= 0 &&  x < mWidth &&  z < mHeight)
               mMarkedGrid[ x,  z] = val;
         }

         //-----------------------------
         
         public void giveStatsInArea(int topX, int topZ, int width, int height, out int numPassed, out int numFailed)
         {
            int eX = (int)(BMathLib.Clamp(topX + width, 0, mWidth - 1));
            int eZ = (int)(BMathLib.Clamp(topZ + height, 0, mWidth - 1));
            numPassed = 0;
            numFailed = 0;
            for (int x = topX; x < eX; x++)
            {
               for(int z = topZ; z < eZ; z++)
               {
                  if (mMarkedGrid[x, z])
                     numPassed++;
                  else
                     numFailed++;
               }
            }
            
         }

      };
   }
}