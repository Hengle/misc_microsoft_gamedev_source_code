using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using EditorCore;

namespace PhoenixEditor
{
   public partial class ScenarioSourceControl2 : UserControl
   {
      public ScenarioSourceControl2()
      {
         InitializeComponent();

         RefreshButton.Visible = true;
         this.CheckInAllButton.Visible = false;

      }

      public void DynamicLoadUI()
      {
 
         int ypos = 60;
         int xpos = 30;
         Dictionary<string,WorkTopic>.Enumerator it = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics().GetEnumerator();
         this.SuspendLayout();
         while (it.MoveNext())
         {            
            WorkTopicControl topicControl = new WorkTopicControl();
            it.Current.Value.UpdateState();
            topicControl.Topic = it.Current.Value;
            topicControl.StateChanged += new EventHandler(topicControl_StateChanged);

            this.Controls.Add(topicControl);
            topicControl.Left = xpos;
            topicControl.Top = ypos;

            if (it.Current.Value.Name == "Export") // special case separation
            {
               topicControl.Top += 40;

            }
            
            ypos = topicControl.Bottom + 20;           


            mTopicControls.Add(topicControl);
         }
         this.ResumeLayout();


         CoreGlobals.getScenarioWorkTopicManager().NotifyTopicsUpdated();
      }

      List<WorkTopicControl> mTopicControls = new List<WorkTopicControl>();

      public void Reset()
      {
         RefreshButton.Visible = true;
         CheckInAllButton.Visible = false;
         foreach (Control c in mTopicControls)
         {
            this.Controls.Remove(c);
         }
         mTopicControls.Clear();
      }
      void topicControl_StateChanged(object sender, EventArgs e)
      {
         //UpdateUI();
         CoreGlobals.getScenarioWorkTopicManager().NotifyTopicsUpdated();

      }


      private void CheckInAllButton_Click(object sender, EventArgs e)
      {
         CoreGlobals.getScenarioWorkTopicManager().mChangeList.Submitchanges();
      }

      private void button1_Click(object sender, EventArgs e)
      {
         Cursor.Current = Cursors.WaitCursor;

         using (PerfSection p = new PerfSection("ScenarioSourceInfo Refresh"))
         {
            Reset();
            DynamicLoadUI();
            //RefreshButton.Visible = false;
         }

         Cursor.Current = Cursors.Default;

      }

      public void UpdateUI()
      {
         foreach (WorkTopicControl c in mTopicControls)
         {
            c.UpdateUI();
         }

         AllowOverwriteCheckBox.Checked = CoreGlobals.getScenarioWorkTopicManager().sAllowOverwrite;

      }

      private void AllowOverwriteCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         CoreGlobals.getScenarioWorkTopicManager().sAllowOverwrite = AllowOverwriteCheckBox.Checked;
         this.UpdateUI();
      }
   }
}
