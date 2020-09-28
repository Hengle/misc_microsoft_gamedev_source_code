using System;
using System.IO;
using System.Windows.Forms;

namespace Terrain
{
   #region TerrainGlobals

   public class TerrainGlobals
   {
      static public BTerrain getTerrain() { return mTerrain; }

      static public BTerrainEditor     getEditor() { return mTerrainEditor; }
      static public BTerrainRender     getRender() { return mTerrainRender; }
      static public BTerrainTexturing  getTexturing() { return mTerrainTexturing; }
      static public BTerrainVisual     getVisual() { return mTerrainVisual; }
      static public BTerrainFrontend   getTerrainFrontEnd() { return mTerrainFrontEnd; }


      static public string getVersion() { return "1.0.0.1"; }
      static public string getClipArtVersion()   {  return "1.0";    }
      //------------------------Members
      static private BTerrainFrontend mTerrainFrontEnd = new BTerrainFrontend();
      static private BTerrain             mTerrain    = new BTerrain();
      static private BTerrainEditor       mTerrainEditor = new BTerrainEditor();
      static private BTerrainRender       mTerrainRender = new BTerrainRender();
      static private BTerrainTexturing    mTerrainTexturing = new BTerrainTexturing();
      static private BTerrainVisual       mTerrainVisual = new BTerrainVisual();


   }




   #endregion
}