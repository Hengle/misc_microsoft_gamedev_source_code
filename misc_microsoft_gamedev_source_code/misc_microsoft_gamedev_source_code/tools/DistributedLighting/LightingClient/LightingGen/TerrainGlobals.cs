using System;
using System.IO;
using System.Windows.Forms;

using EditorCore;

namespace LightingClient
{
   #region TerrainGlobals

   public class TerrainGlobals
   {
      static public LightingTerrainData getTerrain() { return mTerrain; }
      static public BTerrainVisualLODData getLODVB() { return mLODData; }
      static public ExportRenderTerrain getTerrainExportRender() { return mExportRenderData; }
      static public PerforceManager getPerforce() { return mPerforceManager; }

      //------------------------Members
      static private LightingTerrainData mTerrain = new LightingTerrainData();
      static private BTerrainVisualLODData mLODData = new BTerrainVisualLODData();
      static private ExportRenderTerrain mExportRenderData = new ExportRenderTerrain();
      static private PerforceManager mPerforceManager = new PerforceManager();
      static public ProcessorInfo mProcessorInfo = new ProcessorInfo();



      static public String mGameDir;

      static public String mPerforceRoot = @"c:\depot\phoenix\xbox\";

      static public String mArtPath = @"work\art\";
   }




   #endregion
}