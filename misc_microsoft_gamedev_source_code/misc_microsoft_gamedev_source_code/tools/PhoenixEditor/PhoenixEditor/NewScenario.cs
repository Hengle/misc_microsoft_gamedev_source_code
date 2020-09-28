using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Terrain;

namespace PhoenixEditor
{
   public partial class NewScenario : Form
   {
      public NewScenario()
      {
         InitializeComponent();
      }

      int[] cSimTileOptions = new int[] { (int)eMapSizes.cMap_Mico+1, 
                                          (int)eMapSizes.cMap_Tiny+1,
                                          (int)eMapSizes.cMap_Small+1, 
                                          (int)eMapSizes.cMap_Small_Med+1,
                                          (int)eMapSizes.cMap_Medium+1,
                                          (int)eMapSizes.cMap_Med_Large+1,
                                          (int)eMapSizes.cMap_Large+1};
      
      public TerrainCreationParams mParams = new TerrainCreationParams();

      private void NewScenario_Load(object sender, EventArgs e)
      {
         simXTrackbar.Minimum = 0;
         simXTrackbar.Maximum = cSimTileOptions.Length-1;
         simZTrackbar.Minimum = 0;
         simZTrackbar.Maximum = cSimTileOptions.Length-1;
         simTileSpacingBar.Minimum = 1;
         simTileSpacingBar.Maximum = 8;
         simTileSpacingBar.Value = 1;
         VisualDensityBar.Minimum = 0;
         VisualDensityBar.Maximum = 4;
         VisualDensityBar.Value = 1;

         updateValues();
      }

      private void updateValues()
      {
         updateMemberValues();
         updateTextValues();
      }

      private void updateMemberValues()
      {
         mParams.mNumSimXTiles = cSimTileOptions[simXTrackbar.Value];
         mParams.mNumSimZTiles = cSimTileOptions[simZTrackbar.Value];
         mParams.mSimTileSpacing = simTileSpacingBar.Value ;

         mParams.mSimToVisMultiplier = (int)Math.Pow(2, VisualDensityBar.Value);

         mParams.mVisTileSpacing = mParams.mSimTileSpacing / mParams.mSimToVisMultiplier;
         mParams.mNumVisXVerts = (int)(mParams.mNumSimXTiles * mParams.mSimToVisMultiplier);
         mParams.mNumVisZVerts = (int)(mParams.mNumSimZTiles * mParams.mSimToVisMultiplier);

         
      }

      private void updateTextValues()
      {
         NumXSimTilesLabel.Text = mParams.mNumSimXTiles.ToString();
         NumZSimTilesLabel.Text = mParams.mNumSimXTiles.ToString();

         simTileSpacingLabel.Text = mParams.mSimTileSpacing.ToString() + " meters";

         worldSpaceLabel.Text = "Worldspace Size :     " + mParams.mNumSimXTiles * mParams.mSimTileSpacing + "x" + mParams.mNumSimZTiles * mParams.mSimTileSpacing + " meters";

         visualDensityLabel.Text = "x" + mParams.mSimToVisMultiplier + " (" + mParams.mNumVisXVerts + "x" + mParams.mNumVisZVerts + " @ " + mParams.mVisTileSpacing.ToString() + ")";         
      }

      private void simXTrackbar_Scroll(object sender, EventArgs e)
      {
         //CLM this will go away once we get refinement in
         simZTrackbar.Value = simXTrackbar.Value;

         updateValues();
      }

      private void trackBar1_Scroll(object sender, EventArgs e)
      {
         //CLM this will go away once we get refinement in
         simXTrackbar.Value = simZTrackbar.Value;

         updateValues();
      }

      private void simTileSpacingBar_Scroll(object sender, EventArgs e)
      {
         updateValues();
      }

      private void VisualDensityBar_Scroll(object sender, EventArgs e)
      {
         updateValues();
      }

      private void buttonOK_Click(object sender, EventArgs e)
      {
         updateMemberValues();

         //CLM we're actually lying.. we need our verts to be POW2, so our tiles are POW2-1
         mParams.mNumSimXTiles--;
         mParams.mNumSimZTiles--;

         this.DialogResult = DialogResult.OK;
         this.Close();
      }

      private void buttonNO_Click(object sender, EventArgs e)
      {
         this.DialogResult = DialogResult.No;
         this.Close();
      }


   }
}