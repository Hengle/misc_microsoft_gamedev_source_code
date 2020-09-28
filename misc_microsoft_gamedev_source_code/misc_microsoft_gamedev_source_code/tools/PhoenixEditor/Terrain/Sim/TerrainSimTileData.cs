using System;
using System.IO;
using System.Drawing;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using SimEditor;
using EditorCore;
using Rendering;

namespace Terrain
{

   public class SimTileData //CLM this class is ment to go hand-in-hand with SimRepHeights
   {
      int mNumXTiles = 0;
      float mTileScale = 0;
      float mVisToSimMultiplier = 0;

      public int getNumXTiles()
      {
         return mNumXTiles;
      }
      public float getVisToSimScale()
      {
         return mVisToSimMultiplier;
      }

      public void init(int numXTiles, int numZTiles, float tileScale, float visToSimMultiplier)
      {
         mTileScale = tileScale;
         mNumXTiles = numXTiles;
         mVisToSimMultiplier = visToSimMultiplier;

         mSimLandObstructions = new bool[numXTiles * numXTiles];
         mSimLandBuildable = new bool[numXTiles * numXTiles];
         mSimLandFloodObstruction = new bool[numXTiles * numXTiles];
         mSimLandScarabObstruction = new bool[numXTiles * numXTiles];
      }
      public void destroy()
      {
         mSimLandObstructions = null;
         mSimLandBuildable = null;
         mSimLandFloodObstruction = null;
         mSimLandScarabObstruction = null;
      }
      public void updateAfterPainted(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         update(minXTile, minZTile, maxXTile, maxZTile);
      }
      public void update()
      {
         update(0, 0, mNumXTiles, mNumXTiles);
      }
      public void update(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         updateLandObstructions(TerrainGlobals.getTerrain().getTileScale(), minXTile, minZTile, maxXTile, maxZTile);
         updateLandObstructionsPainted(minXTile, minZTile, maxXTile, maxZTile);

         int visVertsX = TerrainGlobals.getTerrain().getNumXVerts();
         updateBoundsObstructions(visVertsX, 0, 0, visVertsX, visVertsX);

         updateBuildable(minXTile, minZTile, maxXTile, maxZTile);
         updateBuildablePainted(minXTile, minZTile, maxXTile, maxZTile);

         updateFloodPassable(minXTile, minZTile, maxXTile, maxZTile);
         updateFloodPassablePainted(minXTile, minZTile, maxXTile, maxZTile);

         updateScarabPassable(minXTile, minZTile, maxXTile, maxZTile);
         updateScarabPassablePainted(minXTile, minZTile, maxXTile, maxZTile);
      }


      #region tile types

      eTileTypeOverrideVal mCurrTileTypeMode = eTileTypeOverrideVal.cTileType_None;
      public enum eTileTypeOverrideVal
      {
         cTileType_None = 0,
         cTileType_Override = 1,
      }
      string mCurrTileTypeOverride = "UNDEFINED"; //see data\terraintiletypes.xml for more
      public void setTileTypeBrushState(eTileTypeOverrideVal v)
      {
         mCurrTileTypeMode = v;
      }
      public eTileTypeOverrideVal getTileTypeBrushState()
      {
         return mCurrTileTypeMode;
      }
      public string getTileTypeOverrideSelection()
      {
         return mCurrTileTypeOverride;
      }
      public void setTileTypeOverrideSelection(string ovr)
      {
         mCurrTileTypeOverride = ovr;
      }

      public void updateTileTypePassable(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         //We don't do this calculation @ runtime...
      }
      public void updateTileTypePainted(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         //We don't do this calculation @ runtime...
      }

      private JaggedContainer<int> mSimTileTypeOverride = null;
      public void initTileTypeOverride()
      {
         mSimTileTypeOverride = new JaggedContainer<int>(mNumXTiles * mNumXTiles);
         mSimTileTypeOverride.SetEmptyValue(0);
      }
      public void destroyTileTypeOverride()
      {
         if (mSimTileTypeOverride != null)
         {
            clearTileTypeOverride();
            mSimTileTypeOverride = null;
         }
      }
      public void clearTileTypeOverride()
      {
         mSimTileTypeOverride.Clear();
      }
      public void createJaggedTileTypeFrom(JaggedContainer<int> v)
      {
         destroyTileTypeOverride();
         initTileTypeOverride();

         if (v == null)
            return;

         long id;
         int maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0)
               continue;

            mSimTileTypeOverride.SetValue(id, maskValue);
         }
      }
      public void setJaggedTileType(int x, int z, int val)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return;

         int idx = x * mNumXTiles + z;
         mSimTileTypeOverride.SetValue(idx, val);
      }
      public int getJaggedTileType(int x, int z)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return 0;

         return getJaggedTileType(x * mNumXTiles + z);
      }
      public int getJaggedTileType(int idx)
      {
         return mSimTileTypeOverride.GetValue(idx);
      }
      public JaggedContainer<int> getJaggedTileType()
      {
         return mSimTileTypeOverride;
      }

      public uint getTileTypeColor(int tileTypeIndex)
      {
         TerrainTileType type = SimTerrainType.getTileTypeByIndex(tileTypeIndex);
         return Convert.ToUInt32(type.EditorColor, 16); ;
      }
      #endregion

      #region obstructions
      public bool isLandObstructedComposite(float normalizedX, float normalizedZ)
      {
         int x = (int)(normalizedX * mNumXTiles);
         int z = (int)(normalizedZ * mNumXTiles);

         return isLandObstructedComposite(x, z);
      }

      public bool isLandObstructedComposite(int x, int z)
      {
         ePassOverrideVal ovr = (ePassOverrideVal)getJaggedPassable(x, z);
         if (ovr == ePassOverrideVal.cPass_Passable)
            return false;
         else if (ovr == ePassOverrideVal.cPass_Unpassable)
            return true;

         return isTileLandObstructed(x, z);
      }

      private bool[] mSimLandObstructions;

     
   
      public bool isTileLandObstructed(int x, int z)
      {
         x = (int)BMathLib.Clamp(x, 0, getNumXTiles() - 1);
         z = (int)BMathLib.Clamp(z, 0, getNumXTiles() - 1);
         return mSimLandObstructions[x * getNumXTiles() + z];
      }

      public bool[] getSimLandObstructions() { return mSimLandObstructions; }

      //----------------------------------
      public void updateLandObstructionsPainted(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         int numX = (int)((maxXTile - minXTile));
         int numZ = (int)((maxZTile - minZTile));

         for (int i = 0; i < numX; i++)
         {
            for (int j = 0; j < numZ; j++)
            {
               int xs = (int)BMathLib.Clamp((i + minXTile), 0, mNumXTiles - 1);
               int zs = (int)BMathLib.Clamp((minZTile + j), 0, mNumXTiles - 1);
               ePassOverrideVal p = (ePassOverrideVal)getJaggedPassable(xs, zs);

               if (p == ePassOverrideVal.cPass_Unpassable)
                  mSimLandObstructions[xs * mNumXTiles + zs] = true;
               if (p == ePassOverrideVal.cPass_Passable)
                  mSimLandObstructions[xs * mNumXTiles + zs] = false;
            }
         }
      }

      public void updateBoundsObstructions(int numVisXVerts, int minXVis, int minZVis, int maxXVis, int maxZVis)
      {
         int xLen = maxXVis - minXVis;
         int zLen = maxZVis - minZVis;
         int numX = (int)((maxXVis - minXVis) * mVisToSimMultiplier);
         int numZ = (int)((maxZVis - minZVis) * mVisToSimMultiplier);

         int minX = (int)(minXVis * mVisToSimMultiplier);
         int minZ = (int)(minZVis * mVisToSimMultiplier);
         int maxX = (int)(maxXVis * mVisToSimMultiplier);
         int maxZ = (int)(maxZVis * mVisToSimMultiplier);


         for (int x = 0; x < xLen; x++)
         {
            for (int z = 0; z < zLen; z++)
            {
               int xs = (int)BMathLib.Clamp((minXVis + x), 0, numVisXVerts - 1);
               int zs = (int)BMathLib.Clamp((minZVis + z), 0, numVisXVerts - 1);
               if (xs < CoreGlobals.mPlayableBoundsMinX ||
                     zs < CoreGlobals.mPlayableBoundsMinZ ||
                     xs >= CoreGlobals.mPlayableBoundsMaxX ||
                     zs >= CoreGlobals.mPlayableBoundsMaxZ)
               {

                  //what sim tile do i fall into?
                  int sX = (int)BMathLib.Clamp((xs * mVisToSimMultiplier), 0, mNumXTiles - 1);
                  int sZ = (int)BMathLib.Clamp((zs * mVisToSimMultiplier), 0, mNumXTiles - 1);


                  mSimLandObstructions[sX * mNumXTiles + sZ] = true;

               }
            }
         }
      }
      public void updateLandObstructions(float visTileScale,int minX, int minZ, int maxX, int maxZ)
      {
         //use height
         const double maxSlopeAngle = 35;
         float maxSlope = (float)Math.Sin(Math.PI * maxSlopeAngle / 180);
         float tileScale = visTileScale / mVisToSimMultiplier;

         float maxDotSlope = 0.7f;


         for (int x = minX; x < maxX; x++)
         {
            for (int z = minZ; z < maxZ; z++)
            {
               int xIndex = (int)BMathLib.Clamp(x + 1, 0, mNumXTiles);
               int zIndex = (int)BMathLib.Clamp(z + 1, 0, mNumXTiles);



               // Calculate the 4 corners
               Vector3 v0 = new Vector3(x * tileScale,
                                         TerrainGlobals.getEditor().getSimRep().getHeightRep().getCompositeHeight(x,z),
                                        z * tileScale);
               Vector3 v1 = new Vector3((x + 1) * tileScale,
                                        TerrainGlobals.getEditor().getSimRep().getHeightRep().getCompositeHeight(xIndex, z ),
                                        z * tileScale);
               Vector3 v2 = new Vector3(x * tileScale,
                                        TerrainGlobals.getEditor().getSimRep().getHeightRep().getCompositeHeight(x, zIndex),
                                        (z + 1) * tileScale);
               Vector3 v3 = new Vector3((x + 1) * tileScale,
                                        TerrainGlobals.getEditor().getSimRep().getHeightRep().getCompositeHeight(xIndex, zIndex),
                                        (z + 1) * tileScale);

               float highest = v0.Y > v1.Y ? v0.Y : v1.Y;
               highest = highest > v2.Y ? highest : v2.Y;
               highest = highest > v3.Y ? highest : v3.Y;

               float lowest = v0.Y < v1.Y ? v0.Y : v1.Y;
               lowest = lowest < v2.Y ? lowest : v2.Y;
               lowest = lowest < v3.Y ? lowest : v3.Y;


               float slp = (highest - lowest) / mTileScale;
              

               xIndex = (int)BMathLib.Clamp(x, 0, mNumXTiles - 1);
               zIndex = (int)BMathLib.Clamp(z, 0, mNumXTiles - 1);

               if (slp > maxSlope)
                  mSimLandObstructions[xIndex * mNumXTiles + zIndex] = true;
               else
                  mSimLandObstructions[xIndex * mNumXTiles + zIndex] = false;
            }
         }
      }

      #endregion

      #region passabilityOverride
      ePassOverrideVal mCurrPassMode = ePassOverrideVal.cPass_Unpassable;
      public enum ePassOverrideVal
      {
         cPass_Unpassable = -1,
         cPass_None = 0,
         cPass_Passable = 1,
      }
      public void setPassableBrushState(ePassOverrideVal v)
      {
         mCurrPassMode = v;
      }
      public ePassOverrideVal getPassableBrushState()
      {
         return mCurrPassMode;
      }

      private JaggedContainer<int> mSimPassableOverride = null;
      public void initPassableOverride()
      {
         mSimPassableOverride = new JaggedContainer<int>(mNumXTiles * mNumXTiles);
         mSimPassableOverride.SetEmptyValue(0);
      }
      public void destroyPassableOverride()
      {
         if (mSimPassableOverride != null)
         {
            clearPassableOverride();
            mSimPassableOverride = null;
         }
      }
      public void clearPassableOverride()
      {
         mSimPassableOverride.Clear();
      }
      public void createJaggedPassableFrom(JaggedContainer<int> v)
      {
         destroyPassableOverride();
         initPassableOverride();

         long id;
         int maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0)
               continue;

            mSimPassableOverride.SetValue(id, maskValue);
         }
      }
      public void setJaggedPassable(int x, int z, int val)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return;

         int idx = x * mNumXTiles + z;
         mSimPassableOverride.SetValue(idx, val);
      }
      public int getJaggedPassable(int x, int z)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return 0 ;

         return getJaggedPassable(x * mNumXTiles + z);
      }
      public int getJaggedPassable(int idx)
      {
         return mSimPassableOverride.GetValue(idx);
      }
      public JaggedContainer<int> getJaggedPassable()
      {
         return mSimPassableOverride;
      }

      #endregion

      #region buildabilityOverride
      eBuildOverrideVal mCurrBuildMode = eBuildOverrideVal.cBuild_Unbuildable;
      public enum eBuildOverrideVal
      {
         cBuild_Unbuildable = -1,
         cBuild_None = 0,
         cBuild_Buildable = 1,
      }
      public void setBuildibleBrushState(eBuildOverrideVal v)
      {
         mCurrBuildMode = v;
      }
      public eBuildOverrideVal getBuildibleBrushState()
      {
         return mCurrBuildMode;
      }
      private bool[] mSimLandBuildable;
      public bool[] getSimLandBuildable() { return mSimLandBuildable; }
      public bool isBuildable(int x, int z)
      {

         x = (int)BMathLib.Clamp(x, 0, getNumXTiles() - 1);
         z = (int)BMathLib.Clamp(z, 0, getNumXTiles() - 1);
         return mSimLandBuildable[x * getNumXTiles() + z];
      }

      public void updateBuildable(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         //if you can walk there, you can build there...
         for (int x = minXTile; x < maxXTile; x++)
         {
            for (int z = minZTile; z < maxZTile; z++)
            {
               if (x<0 || z<0 || x >= mNumXTiles || z >= mNumXTiles)
                  continue;

               mSimLandBuildable[x * getNumXTiles() + z] = mSimLandObstructions[x * getNumXTiles() + z];
            }
         }
      }
      public void updateBuildablePainted(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         int numX = (int)((maxXTile - minXTile));
         int numZ = (int)((maxZTile - minZTile));

         for (int i = 0; i < numX; i++)
         {
            for (int j = 0; j < numZ; j++)
            {
               int xs = (int)BMathLib.Clamp((i + minXTile), 0, mNumXTiles - 1);
               int zs = (int)BMathLib.Clamp((minZTile + j), 0, mNumXTiles - 1);
               eBuildOverrideVal p = (eBuildOverrideVal)getJaggedBuildable(xs, zs);

               if (p == eBuildOverrideVal.cBuild_Unbuildable)
                  mSimLandBuildable[xs * mNumXTiles + zs] = true;
               if (p == eBuildOverrideVal.cBuild_Buildable)
                  mSimLandBuildable[xs * mNumXTiles + zs] = false;
            }
         }
      }


      private JaggedContainer<int> mSimBuildableOverride = null;
      public void initBuildableOverride()
      {
         mSimBuildableOverride = new JaggedContainer<int>(mNumXTiles * mNumXTiles);
         mSimBuildableOverride.SetEmptyValue(0);
      }
      public void destroyBuildableOverride()
      {
         if (mSimBuildableOverride != null)
         {
            clearBuildableOverride();
            mSimBuildableOverride = null;
         }
      }
      public void clearBuildableOverride()
      {
         mSimBuildableOverride.Clear();
      }
      public void createJaggedBuildableFrom(JaggedContainer<int> v)
      {
         destroyBuildableOverride();
         initBuildableOverride();

         long id;
         int maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0)
               continue;

            mSimBuildableOverride.SetValue(id, maskValue);
         }
      }
      public void setJaggedBuildable(int x, int z, int val)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return;

         int idx = x * mNumXTiles + z;
         mSimBuildableOverride.SetValue(idx, val);
      }
      public float getJaggedBuildable(int x, int z)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return 0 ;

         return getJaggedBuildable(x * mNumXTiles + z);
      }
      public int getJaggedBuildable(int idx)
      {
         return mSimBuildableOverride.GetValue(idx);
      }
      public JaggedContainer<int> getJaggedBuildable()
      {
         return mSimBuildableOverride;
      }

      #endregion



      #region FloodPassablityOverride
      eFloodPassOverrideVal mCurrFloodPassMode = eFloodPassOverrideVal.cFldPss_UnFloodPassable;
      public enum eFloodPassOverrideVal
      {
         cFldPss_UnFloodPassable = -1,
         cFldPss_None = 0,
         cFldPss_FloodPassable = 1,
      }
      public void setFloodPassableBrushState(eFloodPassOverrideVal v)
      {
         mCurrFloodPassMode = v;
      }
      public eFloodPassOverrideVal getFloodPassableBrushState()
      {
         return mCurrFloodPassMode;
      }
      private bool[] mSimLandFloodObstruction;
      public bool[] getSimLandFloodObstructed() { return mSimLandFloodObstruction; }
      public bool isFloodObstructed(int x, int z)
      {

         x = (int)BMathLib.Clamp(x, 0, getNumXTiles() - 1);
         z = (int)BMathLib.Clamp(z, 0, getNumXTiles() - 1);
         return mSimLandFloodObstruction[x * getNumXTiles() + z];
      }
      public bool isFloodObstructedComposite(int x, int z)
      {
         eFloodPassOverrideVal ovr = (eFloodPassOverrideVal)getJaggedFloodPassable(x,z);
         if(ovr == eFloodPassOverrideVal.cFldPss_FloodPassable)
            return false;
         else if (ovr == eFloodPassOverrideVal.cFldPss_UnFloodPassable)
            return true;

         return isFloodObstructed(x, z);
      }

      public void updateFloodPassable(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         //if you can walk there, you can build there...
         for (int x = minXTile; x < maxXTile; x++)
         {
            for (int z = minZTile; z < maxZTile; z++)
            {
               if (x < 0 || z < 0 || x >= mNumXTiles || z >= mNumXTiles)
                  continue;

               mSimLandFloodObstruction[x * getNumXTiles() + z] = mSimLandObstructions[x * getNumXTiles() + z];
            }
         }
      }
      public void updateFloodPassablePainted(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         int numX = (int)((maxXTile - minXTile));
         int numZ = (int)((maxZTile - minZTile));

         for (int i = 0; i < numX; i++)
         {
            for (int j = 0; j < numZ; j++)
            {
               int xs = (int)BMathLib.Clamp((i + minXTile), 0, mNumXTiles - 1);
               int zs = (int)BMathLib.Clamp((minZTile + j), 0, mNumXTiles - 1);
               eFloodPassOverrideVal p = (eFloodPassOverrideVal)getJaggedFloodPassable(xs, zs);

               if (p == eFloodPassOverrideVal.cFldPss_UnFloodPassable)
                  mSimLandFloodObstruction[xs * mNumXTiles + zs] = true;
               if (p == eFloodPassOverrideVal.cFldPss_FloodPassable)
                  mSimLandFloodObstruction[xs * mNumXTiles + zs] = false;
            }
         }
      }


      private JaggedContainer<int> mSimFloodPassableOverride = null;
      public void initFloodPassableOverride()
      {
         mSimFloodPassableOverride = new JaggedContainer<int>(mNumXTiles * mNumXTiles);
         mSimFloodPassableOverride.SetEmptyValue(0);
      }
      public void destroyFloodPassableOverride()
      {
         if (mSimFloodPassableOverride != null)
         {
            clearFloodPassableOverride();
            mSimFloodPassableOverride = null;
         }
      }
      public void clearFloodPassableOverride()
      {
         mSimFloodPassableOverride.Clear();
      }
      public void createJaggedFloodPassableFrom(JaggedContainer<int> v)
      {
         destroyFloodPassableOverride();
         initFloodPassableOverride();

         long id;
         int maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0)
               continue;

            mSimFloodPassableOverride.SetValue(id, maskValue);
         }
      }
      public void setJaggedFloodPassable(int x, int z, int val)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return;

         int idx = x * mNumXTiles + z;
         mSimFloodPassableOverride.SetValue(idx, val);
      }
      public int getJaggedFloodPassable(int x, int z)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return 0;

         return getJaggedFloodPassable(x * mNumXTiles + z);
      }
      public int getJaggedFloodPassable(int idx)
      {
         return mSimFloodPassableOverride.GetValue(idx);
      }
      public JaggedContainer<int> getJaggedFloodPassable()
      {
         return mSimFloodPassableOverride;
      }

      #endregion


      #region ScarabPassablityOverride
      eScarabPassOverrideVal mCurrScarabPassMode = eScarabPassOverrideVal.cScrbPss_UnScarabPassable;
      public enum eScarabPassOverrideVal
      {
         cScrbPss_UnScarabPassable = -1,
         cScrbPss_None = 0,
         cScrbPss_ScarabPassable = 1,
      }
      public void setScarabPassableBrushState(eScarabPassOverrideVal v)
      {
         mCurrScarabPassMode = v;
      }
      public eScarabPassOverrideVal getScarabPassableBrushState()
      {
         return mCurrScarabPassMode;
      }
      private bool[] mSimLandScarabObstruction;
      public bool[] getSimLandScarabPassable() { return mSimLandScarabObstruction; }
      public bool isScarabObstructed(int x, int z)
      {

         x = (int)BMathLib.Clamp(x, 0, getNumXTiles() - 1);
         z = (int)BMathLib.Clamp(z, 0, getNumXTiles() - 1);
         return mSimLandScarabObstruction[x * getNumXTiles() + z];
      }
      public bool isScarabObstructedComposite(int x, int z)
      {
         eScarabPassOverrideVal ovr = (eScarabPassOverrideVal)getJaggedScarabPassable(x, z);
         if (ovr == eScarabPassOverrideVal.cScrbPss_ScarabPassable)
            return false;
         else if (ovr == eScarabPassOverrideVal.cScrbPss_UnScarabPassable)
            return true;

         return isScarabObstructed(x, z);
      }
      public void updateScarabPassable(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         //if you can walk there, you can build there...
         for (int x = minXTile; x < maxXTile; x++)
         {
            for (int z = minZTile; z < maxZTile; z++)
            {
               if (x < 0 || z < 0 || x >= mNumXTiles || z >= mNumXTiles)
                  continue;

               mSimLandScarabObstruction[x * getNumXTiles() + z] = mSimLandObstructions[x * getNumXTiles() + z];
            }
         }
      }
      public void updateScarabPassablePainted(int minXTile, int minZTile, int maxXTile, int maxZTile)
      {
         int numX = (int)((maxXTile - minXTile));
         int numZ = (int)((maxZTile - minZTile));

         for (int i = 0; i < numX; i++)
         {
            for (int j = 0; j < numZ; j++)
            {
               int xs = (int)BMathLib.Clamp((i + minXTile), 0, mNumXTiles - 1);
               int zs = (int)BMathLib.Clamp((minZTile + j), 0, mNumXTiles - 1);
               eScarabPassOverrideVal p = (eScarabPassOverrideVal)getJaggedScarabPassable(xs, zs);

               if (p == eScarabPassOverrideVal.cScrbPss_UnScarabPassable)
                  mSimLandScarabObstruction[xs * mNumXTiles + zs] = true;
               if (p == eScarabPassOverrideVal.cScrbPss_ScarabPassable)
                  mSimLandScarabObstruction[xs * mNumXTiles + zs] = false;
            }
         }
      }


      private JaggedContainer<int> mSimScarabPassableOverride = null;
      public void initScarabPassableOverride()
      {
         mSimScarabPassableOverride = new JaggedContainer<int>(mNumXTiles * mNumXTiles);
         mSimScarabPassableOverride.SetEmptyValue(0);
      }
      public void destroyScarabPassableOverride()
      {
         if (mSimScarabPassableOverride != null)
         {
            clearScarabPassableOverride();
            mSimScarabPassableOverride = null;
         }
      }
      public void clearScarabPassableOverride()
      {
         mSimScarabPassableOverride.Clear();
      }
      public void createJaggedScarabPassableFrom(JaggedContainer<int> v)
      {
         destroyScarabPassableOverride();
         initScarabPassableOverride();

         long id;
         int maskValue;
         v.ResetIterator();
         while (v.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0)
               continue;

            mSimScarabPassableOverride.SetValue(id, maskValue);
         }
      }
      public void setJaggedScarabPassable(int x, int z, int val)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return;

         int idx = x * mNumXTiles + z;
         mSimScarabPassableOverride.SetValue(idx, val);
      }
      public float getJaggedScarabPassable(int x, int z)
      {
         if (x < 0 || x >= mNumXTiles || z < 0 || z >= mNumXTiles)
            return 0;

         return getJaggedScarabPassable(x * mNumXTiles + z);
      }
      public int getJaggedScarabPassable(int idx)
      {
         return mSimScarabPassableOverride.GetValue(idx);
      }
      public JaggedContainer<int> getJaggedScarabPassable()
      {
         return mSimScarabPassableOverride;
      }

      #endregion
   }
}