using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace GenerateScenarioOpenFileLists
{
   public partial class Form1 : Form
   {
      public Form1()
      {
         InitializeComponent();
      }

      private void populateScenarioListBox()
      {
         scenarioListBox.Items.Clear();

         List<string> scnList = GenearateOpenFileList.generateScenarioList((GenearateOpenFileList.eListFilter)comboBox1.SelectedIndex);
         for (int i = 0; i < scnList.Count; i++)
            scenarioListBox.Items.Add(scnList[i]);
      }
      private void Form1_Load(object sender, EventArgs e)
      {
         comboBox1.SelectedIndex = 0;


      }

      private void button2_Click(object sender, EventArgs e)
      {
         for (int i = 0; i < scenarioListBox.Items.Count; i++)
            scenarioListBox.SetSelected(i, true);
      }

      private void button1_Click(object sender, EventArgs e)
      {
         if (GenearateOpenFileList.IsWorking())
         {
            GenearateOpenFileList.stopWork();
            return;
         }

         statusListBox.Items.Clear();
         timer1.Enabled = true;

         List<string> scnList = new List<string>();
         for (int i = 0; i < scenarioListBox.SelectedItems.Count; i++)
            scnList.Add(scenarioListBox.SelectedItems[i] as string);

         GenearateOpenFileList.generateFileLists(scnList);

         button1.Text = "STOP";
      }

      private void timer1_Tick(object sender, EventArgs e)
      {
         lock(Program.mMessageList)
         {
            int ic = Program.mMessageList.Count();
            for(int i=0;i<ic;i++)
               Program.mMessageList.dequeueProcessMessage();
         }
      }

      public void addStatusString(string str)
      {
         if (str[0] == '!')
         {
            if (str.Contains("!DONE"))
            {
               button1.Text = "GO!";

               statusListBox.Items.Add("");
               statusListBox.Items.Add("");
               statusListBox.Items.Add("");
            }
         }
         else
         {
            statusListBox.Items.Add(str);
            statusListBox.SelectedIndex = statusListBox.Items.Count - 1;
         }
      }

      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         populateScenarioListBox();
      }


   }
}