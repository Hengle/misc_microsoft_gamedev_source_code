using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.IO;
using System.Windows.Forms;

namespace Terrain.Controls
{
   public partial class AOGenDialog : Form
   {
      DistributedAOGen daog = null;

      public AOGenDialog()
      {
         InitializeComponent();
         try
         {
            this.StartPosition = FormStartPosition.CenterScreen;
            comboBox1.SelectedIndex = 0;
            radioButton1.Enabled = true;
            radioButton1.Checked = true;
            radioButton2.Checked = false;

            //don't allow us to use network access if we can't connect
            if(!DistributedAOGen.networkAOInterface.networkAccessAvailable())
            {
               radioButton1.Enabled = false;
               radioButton1.Checked = false;
               radioButton2.Checked = true;
            }
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
      }

      public void setNumWorkUnits(int numUnits)
      {
         progressBar1.Maximum = numUnits;
         progressBar1.Minimum = 0;
      }
      public void increaseWorkUnitCount()
      {
         progressBar1.Value++;
         if(progressBar1.Value == progressBar1.Maximum)
         {
            button2.Enabled = false;
            button1.Enabled = true;
            daog = null;
            this.Close();
         }
      }


      public void cancel()
      {
         radioButton1.Enabled = true;
         radioButton2.Enabled = true;
         button1.Enabled = true;
         comboBox1.Enabled = true;
         checkBox1.Enabled = true;
         button2.Enabled = false;

         if (daog != null)
         {
            daog.cancelAOGenJob();
            daog = null;
         }
         progressBar1.Value = 0;
      }

      private void button1_Click(object sender, EventArgs e)
      {
         bool doNetworkGen = radioButton1.Checked;

         if(doNetworkGen)
         {
            if(DistributedAOGen.networkAOInterface.doesWorkExist())
            {
               DialogResult res = MessageBox.Show("There is prior work being done on the network. Would you like to wait in line? \n\n Click YES to wait in line for the network. \n\n Click NO to locally generate AO. \n\n Click CANCEL to.. ","Think about it..", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Exclamation);
               if (res == DialogResult.Yes)
                  doNetworkGen = true;
               else if (res == DialogResult.No)
                  doNetworkGen = false;
               else
                  return;
            }
         }

         AmbientOcclusion.eAOQuality quality = AmbientOcclusion.eAOQuality.cAO_Off;
         if (comboBox1.SelectedIndex == 0) quality = AmbientOcclusion.eAOQuality.cAO_Worst;
         if (comboBox1.SelectedIndex == 1) quality = AmbientOcclusion.eAOQuality.cAO_Medium;
         if (comboBox1.SelectedIndex == 2) quality = AmbientOcclusion.eAOQuality.cAO_Best;
         if (comboBox1.SelectedIndex == 3) quality = AmbientOcclusion.eAOQuality.cAO_Final;

         bool includeObjects = checkBox1.Checked;

         if (doNetworkGen && Directory.Exists(@"\\esfile\phoenix\Tools\DistributedLighting\"))
         {

            daog = new DistributedAOGen();
            if(!daog.issueAOGenJob(quality,includeObjects, this))
            {
               //something canceled internally.
               cancel();
            }          
         }
         else
         {
            daog = new DistributedAOGen();
            daog.issueAOGenLocal(quality,includeObjects, this);
         }

         comboBox1.Enabled = false;
         checkBox1.Enabled = false;
         button1.Enabled = false;
         button2.Enabled = true;
         radioButton1.Enabled = false;
         radioButton2.Enabled = false;
         progressBar1.Value = 0;
         
      }

      private void button2_Click(object sender, EventArgs e)
      {
         cancel();

      }

      private void AOGenDialog_FormClosing(object sender, FormClosingEventArgs e)
      {
         if (daog != null)
         {
            daog.cancelAOGenJob();
         }
      }
   }
}