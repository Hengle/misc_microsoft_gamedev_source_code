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
   public partial class SingleTopicCoordinator : UserControl
   {
      public SingleTopicCoordinator()
      {
         InitializeComponent();
         OKButton.Enabled = false;
      }

      public void SetTopic(string topicName)
      {
         Dictionary<string, WorkTopic> topics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         mTopic = topics[topicName];

         this.workTopicControl1.Topic = mTopic;

         this.workTopicControl1.StateChanged += new EventHandler(workTopicControl1_StateChanged);
         workTopicControl1.DoNotSaveOption = true;
         workTopicControl1.OverwriteOption = true;


         UpdateState();

         //this.ParentForm.Text = "
      }
      WorkTopic mTopic = null;

      void workTopicControl1_StateChanged(object sender, EventArgs e)
      {
         UpdateState();

      }

      void UpdateState()
      {
         bool filesBlocked = false;

         if (mTopic.Changed == true)
         {
            ICollection<string> blocked;
            if (mTopic.DoNotSave == false && mTopic.AreFilesWritetable(out blocked) == false)
            {
               filesBlocked = true;
            }
            else
            {

            }

         }
         else
         {

         }
         
         if (filesBlocked)
         {
            this.OKButton.Enabled = false;
         }
         else
         {
            this.OKButton.Enabled = true;
         }
         workTopicControl1.UpdateUI();
      }
      private void workTopicControl1_Load(object sender, EventArgs e)
      {

      }

      private void OKButton_Click(object sender, EventArgs e)
      {
         this.ParentForm.DialogResult = DialogResult.OK;
         this.ParentForm.Close();
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.ParentForm.DialogResult = DialogResult.Cancel;
         this.ParentForm.Close();
      }
   }
}
