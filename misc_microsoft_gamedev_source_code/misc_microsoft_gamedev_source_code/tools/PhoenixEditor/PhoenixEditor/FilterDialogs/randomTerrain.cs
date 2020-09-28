using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;

using Terrain;
using EditorCore;
using NoiseGeneration;



namespace PhoenixEditor.Filter_Dialogs
{
   public partial class randomTerrain : Form
   {
      public randomTerrain()
      {
         InitializeComponent();
         this.TopMost = true;
      }

      private void button2_Click(object sender, EventArgs e)
      {
         if (comboBox1.SelectedIndex == -1)
            comboBox1.SelectedIndex = 1;

         if(tabControl1.SelectedIndex==0)
         {
       
            MountainRidgeTerrainFilter mrt = new MountainRidgeTerrainFilter();
            //mrt.AddReplaceSubtract = (eFilterOperation)(Enum.Parse(typeof(eFilterOperation), comboBox1.SelectedIndex));
            mrt.AddReplaceSubtract = (eFilterOperation)(comboBox1.SelectedIndex);
            mrt.MacroSeed = trackBar1.Value;
            mrt.MicroSeed = trackBar2.Value;
            mrt.ScaleHeight = (float)numericUpDown1.Value;
            mrt.Frequency = trackBar3.Value * 0.001f;
            mrt.Detail = trackBar4.Value;
            mrt.Lacunarity = trackBar5.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;
            mrt.init();
            mrt.apply(checkBox1.Checked);

            mrt = null;
         }
         else if (tabControl1.SelectedIndex == 1)
         {
            BillowTerrainFilter mrt = new BillowTerrainFilter();
            mrt.mAddReplaceSubtract = comboBox1.SelectedIndex;
            mrt.mMacroSeed = trackBar9.Value;
            mrt.mMicroSeed = trackBar10.Value;
            mrt.mScaleHeight = (float)numericUpDown1.Value;
            mrt.mFrequency = trackBar8.Value * 0.001f;
            mrt.mDetail = trackBar7.Value;
            mrt.mLacunarity = trackBar6.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;
            mrt.init();
            mrt.apply(checkBox1.Checked);

            mrt = null;
         }
         else if (tabControl1.SelectedIndex == 2)
         {
            VoronoiTerrainFilter mrt = new VoronoiTerrainFilter();
            mrt.mAddReplaceSubtract = comboBox1.SelectedIndex;
            mrt.mMacroSeed = trackBar24.Value;
            mrt.mMicroSeed = trackBar23.Value;
            mrt.mScaleHeight = (float)numericUpDown1.Value;
            mrt.mFrequency = trackBar22.Value * 0.01f;
            mrt.mDisplacement = trackBar21.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;

            mrt.init();
            mrt.apply(checkBox1.Checked);

            mrt = null;
         }
         else if (tabControl1.SelectedIndex == 3)
         {
            fBMTerrainFilter mrt = new fBMTerrainFilter();
            mrt.mAddReplaceSubtract = comboBox1.SelectedIndex;
            mrt.mMacroSeed = trackBar28.Value;
            mrt.mMicroSeed = trackBar29.Value;
            mrt.mScaleHeight = (float)numericUpDown1.Value;
            mrt.mFrequency = trackBar27.Value * 0.001f;
            mrt.mDetail = trackBar26.Value;
            mrt.mLacunarity = trackBar25.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;
            mrt.init();
            mrt.apply(checkBox1.Checked);

            mrt = null;
         }

         this.Close();
      }

      void previewFractalToImage()
      {
         Bitmap output = new Bitmap(TerrainGlobals.getTerrain().getNumXVerts(),TerrainGlobals.getTerrain().getNumZVerts(), PixelFormat.Format32bppArgb);

         if (comboBox1.SelectedIndex == -1)
            comboBox1.SelectedIndex = 1;

         if (tabControl1.SelectedIndex == 0)
         {

            MountainRidgeTerrainFilter mrt = new MountainRidgeTerrainFilter();
            //mrt.AddReplaceSubtract = (eFilterOperation)(Enum.Parse(typeof(eFilterOperation), comboBox1.SelectedIndex));
            mrt.AddReplaceSubtract = (eFilterOperation)(comboBox1.SelectedIndex);
            mrt.MacroSeed = trackBar1.Value;
            mrt.MicroSeed = trackBar2.Value;
            mrt.ScaleHeight = (float)numericUpDown1.Value;
            mrt.Frequency = trackBar3.Value * 0.001f;
            mrt.Detail = trackBar4.Value;
            mrt.Lacunarity = trackBar5.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;

            mrt.previewToBitmap(ref output);

            mrt = null;
         }
         else if (tabControl1.SelectedIndex == 1)
         {
            BillowTerrainFilter mrt = new BillowTerrainFilter();
            mrt.mAddReplaceSubtract = comboBox1.SelectedIndex;
            mrt.mMacroSeed = trackBar9.Value;
            mrt.mMicroSeed = trackBar10.Value;
            mrt.mScaleHeight = (float)numericUpDown1.Value;
            mrt.mFrequency = trackBar8.Value * 0.001f;
            mrt.mDetail = trackBar7.Value;
            mrt.mLacunarity = trackBar6.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;

            mrt.previewToBitmap(ref output);

            mrt = null;
         }
         else if (tabControl1.SelectedIndex == 2)
         {
            VoronoiTerrainFilter mrt = new VoronoiTerrainFilter();
            mrt.mAddReplaceSubtract = comboBox1.SelectedIndex;
            mrt.mMacroSeed = trackBar24.Value;
            mrt.mMicroSeed = trackBar23.Value;
            mrt.mScaleHeight = (float)numericUpDown1.Value;
            mrt.mFrequency = trackBar22.Value * 0.01f;
            mrt.mDisplacement = trackBar21.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;


            mrt.previewToBitmap(ref output);

            mrt = null;
         }
         else if (tabControl1.SelectedIndex == 3)
         {
            fBMTerrainFilter mrt = new fBMTerrainFilter();
            mrt.mAddReplaceSubtract = comboBox1.SelectedIndex;
            mrt.mMacroSeed = trackBar28.Value;
            mrt.mMicroSeed = trackBar29.Value;
            mrt.mScaleHeight = (float)numericUpDown1.Value;
            mrt.mFrequency = trackBar27.Value * 0.001f;
            mrt.mDetail = trackBar26.Value;
            mrt.mLacunarity = trackBar25.Value * 0.01f;
            mrt.mMaskCreationType = mMaskGenType;

            mrt.previewToBitmap(ref output);

            mrt = null;
         }


         Bitmap rescale = new Bitmap(output,pictureBox1.Width, pictureBox1.Height);
         pictureBox1.Image = rescale;
         pictureBox1.Invalidate();
      }

      private void button1_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void randomTerrain_Load(object sender, EventArgs e)
      {
         comboBox1.SelectedIndex = 1;
         comboBox2.SelectedIndex = 0;
      }

      private void button3_Click(object sender, EventArgs e)
      {
         previewFractalToImage();
      }

      TerrainFilter.eFilterMaskCreation mMaskGenType = TerrainFilter.eFilterMaskCreation.cNoMask;
      private void comboBox2_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (comboBox2.SelectedIndex == 0) mMaskGenType = TerrainFilter.eFilterMaskCreation.cNoMask;
         if (comboBox2.SelectedIndex == 1) mMaskGenType = TerrainFilter.eFilterMaskCreation.cMaskAndErosion;
         if (comboBox2.SelectedIndex == 2) mMaskGenType = TerrainFilter.eFilterMaskCreation.cMaskOnly;
      }
   }
}