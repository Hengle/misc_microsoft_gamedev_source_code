#define useNewSimRepGenerationCode

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

//----------------------------------
namespace Terrain
{

   #region TerrainSimRep

   public class BTerrainSimVisualData
   {
      public BTerrainSimVisualData()
      {

      }

      ~BTerrainSimVisualData()
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
   public class BTerrainSimRep
   {

      SimHeightRep mHeightRep = new SimHeightRep();
      public SimHeightRep getHeightRep() { return mHeightRep; }

      SimFlightRep mFlightRep = new SimFlightRep();
      public SimFlightRep getFlightRep() { return mFlightRep; }

      SimTileData mDataTiles = new SimTileData();
      public SimTileData getDataTiles() { return mDataTiles; }

      //CLM [10.03.07] this MUST be in this order so that we can be sure this aligns with the game's enums
      public enum eObstructionType
      { 
         cNoObstruction = 0,
         cObstrtuction_Land,
         cObstrtuction_Water, //Holder
         cObstrtuction_Land_Flood_Scarab, 
         cObstrtuction_IsNonSolid,
         cObstrtuction_Flood,
         cObstrtuction_Scarab,
         cObstrtuction_Land_Flood,
         cObstrtuction_Land_Scarab,
         cObstrtuction_Flood_Scarab,


      };

      public enum eChannels
      {
         cNoChannel = 0,
         cObstrtuctionChannel,
         cBuildableChannel,
         cFloodObstructionChannel,
         cScarabObstructionChannel,
         cTileTypeChannel
      };

      int[] eChannelPosColors = new int[]
      {
         /*cNoChannel                 = */0,
         /*cObstrtuctionChannel       = */0x4400FF00,
         /*cBuildableChannel          = */0x44FFE303,
         /*cFloodObstructionChannel   = */0x4440E0D0,
         /*cScarabObstructionChannel  = */0x4491219E,
         /*cTileTypeChannel           = */0x00000000,
      };

      int[] eChannelNegColors = new int[]
      {
         /*cNoChannel                 = */0,
         /*cObstrtuctionChannel       = */0x44FF0000,
         /*cBuildableChannel          = */0x44FF0000,
         /*cFloodObstructionChannel   = */0x44FF0000,
         /*cScarabObstructionChannel  = */0x44FF0000,
         /*cTileTypeChannel           = */0x00000000,
      };

      eChannels mCurrentChannel = eChannels.cObstrtuctionChannel;
      public void setChannel(eChannels chan)
      {
         mCurrentChannel = chan;
      } 
      public eChannels getChannel()
      {
         return mCurrentChannel;
      }

      public int getChannelPosColor(eChannels chan)
      {
         return eChannelPosColors[(int)chan];
      }
      public int getChannelNegColor(eChannels chan)
      {
         return eChannelNegColors[(int)chan];
      }

      //----------------------------------
      public BTerrainSimRep()
      {
          

      }
      ~BTerrainSimRep()
      {
         destroy();
      }

      //----------------------------------
      public void reinit(int numXVerts, int numZVerts, float tileScale, float visToSimMultiplier)
      {
         destroy();
         init(numXVerts, numZVerts, tileScale, visToSimMultiplier);
      }
      public void init(int numXVerts, int numZVerts, float tileScale, float visToSimMultiplier)
      {
         mVisToSimMultiplier = visToSimMultiplier;

         mHeightRep.init(numXVerts , numZVerts , tileScale);
         mFlightRep.init();

         mDataTiles.init(numXVerts , numXVerts , tileScale, visToSimMultiplier);

      }
      public void destroy()
      {
         mHeightRep.destroy();
         mDataTiles.destroy();
         mFlightRep.destroy();

      }
      public void update()
      {
         update(false,true);
      }
      
      public void updateAfterPainted(int minX, int minZ, int maxX, int maxZ)
      {
         mDataTiles.updateAfterPainted(minX - 1, minZ - 1, maxX + 1, maxZ + 1);

         mHeightRep.updateAfterPainted(minX, minZ, maxX, maxZ);

         mFlightRep.updateAfterPainted(minX, minZ, maxX, maxZ);
      }
      public void updateForExport()
      {
         mHeightRep.recalculateHeights(false, true);
         mFlightRep.recalculateHeights();

         getDataTiles().update();
      }

      public void update(bool highAccuracyHeightUpdate,bool doUpdateVisualHandle)
      {
         mHeightRep.recalculateHeights(false, highAccuracyHeightUpdate);
         mFlightRep.recalculateHeights();

         getDataTiles().update();

         if (doUpdateVisualHandle)
         {
            mHeightRep.recalculateVisuals();
            mFlightRep.recalculateVisuals();
         }
      }

      public int getNumXVerts() { return (int)mHeightRep.getNumXPoints(); }
      public int getNumXTiles() { return mDataTiles.getNumXTiles(); }
      public float getTileScale() { return mHeightRep.getTileScale(); }
      public float getVisToSimScale() { return mVisToSimMultiplier; }
      public void giveSimTileXZfromVisVertXZ(int visVertX, int visVertZ, ref int simTileX, ref int simTileZ)
      {
         simTileX = (int)(visVertX * mVisToSimMultiplier);
         simTileZ = (int)(visVertZ * mVisToSimMultiplier);
      }
      public void giveSimTileBounds(out int numXTiles, out int numZTiles)
      {
         numXTiles = (getNumXVerts() - 1);
         numZTiles = (getNumXVerts() - 1);
      }

      private float mVisToSimMultiplier;


   }
}

    #endregion

