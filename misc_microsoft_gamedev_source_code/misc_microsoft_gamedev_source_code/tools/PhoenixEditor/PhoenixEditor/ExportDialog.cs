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
   public partial class ExportDialog : Form
   {
      static float cMinLODVal = 0.05f;
      static float cMaxLODVal = 0.25f;
      static float cRange = cMaxLODVal - cMinLODVal;

      public bool mIsQuickExport = false;
      public ExportDialog()
      {
         InitializeComponent();
      }

      private void settingsToControls()
      {
         lodFactorSlider.Value = (int)(((mExportSettings.RefineEpsilon - cMinLODVal) / cRange) * 20);
         lodlabel.Text = lodFactorSlider.Value.ToString();

         //AO
         if (mExportSettings.AmbientOcclusion == AmbientOcclusion.eAOQuality.cAO_Off) comboBox1.SelectedIndex = 0 ;
         if (mExportSettings.AmbientOcclusion == AmbientOcclusion.eAOQuality.cAO_Worst) comboBox1.SelectedIndex = 1;
         if (mExportSettings.AmbientOcclusion == AmbientOcclusion.eAOQuality.cAO_Medium) comboBox1.SelectedIndex = 2;
         if (mExportSettings.AmbientOcclusion == AmbientOcclusion.eAOQuality.cAO_Best) comboBox1.SelectedIndex = 3;
         if (mExportSettings.AmbientOcclusion == AmbientOcclusion.eAOQuality.cAO_Final) comboBox1.SelectedIndex = 4;

         checkBox1.Checked = mExportSettings.ObjectsInAO;


         //UNIQUE TEX RES
         if (mExportSettings.UniqueTexRes > -1)
         {
            int w = BTerrainTexturing.LODToWidth(mExportSettings.UniqueTexRes);
            unqLabel.Text = "Unique Tex Res (perChunk): " + w + "x" + w;
            UniRes.Value = 5 - mExportSettings.UniqueTexRes;
         }
         else
         {
            unqLabel.Text = "Unique Tex Res (perChunk): OFF";
            UniRes.Value = 2;
         }
         
      }


      private void ExportLevelListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (ExportLevelListBox.Text == "Quick")
         {
            mExportSettings.SettingsQuick();
         }
         else if (ExportLevelListBox.Text == "Final")
         {
            mExportSettings.SettingsFinal();
         }
         else if (ExportLevelListBox.Text == "Custom")
         {
         
         }
         settingsToControls();
      }
      public Export360.ExportSettings mExportSettings = null;

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.Close();
      }

      private void ExportButton_Click(object sender, EventArgs e)
      {
         if (!mIsQuickExport)
            TerrainGlobals.getTerrain().setExportSettings(mExportSettings);
         this.Close();

      }

      private void ExportDialog_Load(object sender, EventArgs e)
      {
         mExportSettings = TerrainGlobals.getTerrain().getExportSettings();
         settingsToControls();
      }

      private void doLODCheckbox_CheckedChanged(object sender, EventArgs e)
      {
        // mExportSettings.RefineTerrain = doLODCheckbox.Checked;
    //     lodBiasSlider.Enabled = doLODCheckbox.Checked;
     //    lodFactorSlider.Enabled = doLODCheckbox.Checked;
      }

      private void groupBox1_Enter(object sender, EventArgs e)
      {

      }

      private void lodFactorSlider_Scroll(object sender, EventArgs e)
      {
         
         mExportSettings.RefineEpsilon = cMinLODVal + ((lodFactorSlider.Value / 20.0f) * cRange);
         lodlabel.Text = lodFactorSlider.Value.ToString();
      }


      private void UniRes_Scroll(object sender, EventArgs e)
      {
         
         {
            int w = BTerrainTexturing.LODToWidth(5-UniRes.Value);
            unqLabel.Text = "Unique Tex Res (perChunk): " + w + "x" + w;
         }

         mExportSettings.UniqueTexRes = 5 - UniRes.Value;

      }


      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (comboBox1.SelectedIndex == 0) mExportSettings.AmbientOcclusion = AmbientOcclusion.eAOQuality.cAO_Off;
         if (comboBox1.SelectedIndex == 1) mExportSettings.AmbientOcclusion = AmbientOcclusion.eAOQuality.cAO_Worst;
         if (comboBox1.SelectedIndex == 2) mExportSettings.AmbientOcclusion = AmbientOcclusion.eAOQuality.cAO_Medium;
         if (comboBox1.SelectedIndex == 3) mExportSettings.AmbientOcclusion = AmbientOcclusion.eAOQuality.cAO_Best;
         if (comboBox1.SelectedIndex == 4) mExportSettings.AmbientOcclusion = AmbientOcclusion.eAOQuality.cAO_Final;

      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         mExportSettings.ObjectsInAO = checkBox1.Checked;
      }

    


   }
}