using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class AutoSaveDlg : Form
   {
      public AutoSaveDlg()
      {
         InitializeComponent();
      }

      private void AutoSave_Load(object sender, EventArgs e)
      {
         checkBox1.Checked = CoreGlobals.getSettingsFile().AutoSaveEnabled;
         groupBox1.Enabled = checkBox1.Checked;
         radioButton1.Checked = !CoreGlobals.getSettingsFile().AutoSaveToCustom;
         radioButton2.Checked = CoreGlobals.getSettingsFile().AutoSaveToCustom;
      }
       
      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         CoreGlobals.getSettingsFile().AutoSaveEnabled = checkBox1.Checked;
         groupBox1.Enabled = checkBox1.Checked;
         CoreGlobals.getSettingsFile().Save();
      }


      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         CoreGlobals.getSettingsFile().AutoSaveTimeInMinutes = (int)numericUpDown1.Value;
         CoreGlobals.getSettingsFile().Save();
      }

      private void radioButton1_CheckedChanged(object sender, EventArgs e)
      {
         if (radioButton1.Checked == true)
            CoreGlobals.getSettingsFile().AutoSaveToCustom = false ;
         else
            CoreGlobals.getSettingsFile().AutoSaveToCustom = true;

         CoreGlobals.getSettingsFile().Save();
         
      }

      private void radioButton2_CheckedChanged(object sender, EventArgs e)
      {
         if (radioButton1.Checked == true)
            CoreGlobals.getSettingsFile().AutoSaveToCustom = false;
         else
            CoreGlobals.getSettingsFile().AutoSaveToCustom = true;

         CoreGlobals.getSettingsFile().Save();
      }
   }
}