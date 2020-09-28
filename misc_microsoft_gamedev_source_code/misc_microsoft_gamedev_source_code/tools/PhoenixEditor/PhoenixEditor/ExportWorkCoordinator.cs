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
   public partial class ExportWorkCoordinator : UserControl
   {
      public ExportWorkCoordinator()
      {
         InitializeComponent();

         Dictionary<string, WorkTopic> topics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         mWorkTopics["Export"] = topics["Export"];


         this.ExportFilesWorkTopicControl.Topic = mWorkTopics["Export"];
         
         this.ExportFilesWorkTopicControl.StateChanged += new EventHandler(ExportFilesWorkTopicControl_StateChanged);

         ExportFilesWorkTopicControl.DoNotSaveOption = false;
         ExportFilesWorkTopicControl.OverwriteOption = true;


         UpdateState();

      }
      Dictionary<string, WorkTopic> mWorkTopics = new Dictionary<string,WorkTopic>();

      void UpdateState()
      {
         bool filesBlocked = false;

         Dictionary<string, WorkTopic>.Enumerator it = mWorkTopics.GetEnumerator();
         while (it.MoveNext())
         {
            //if (it.Current.Value.mDataPattern != WorkTopic.eDataPatterm.cSecondary)
            //   continue;

            if (it.Current.Value.Changed == true)
            {
               ICollection<string> blocked;
               if (it.Current.Value.DoNotSave == false && it.Current.Value.AreFilesWritetable(out blocked) == false)
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
         }
         if (filesBlocked)
         {
            this.NextButton.Enabled = false;
         }
         else
         {
            this.NextButton.Enabled = true;
         }

      }


      void ExportFilesWorkTopicControl_StateChanged(object sender, EventArgs e)
      {
         UpdateState();
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.ParentForm.DialogResult = DialogResult.Cancel;
         this.ParentForm.Close();
      }

      private void NextButton_Click(object sender, EventArgs e)
      {
         //CoreGlobals.sDontPromtResolvedSave = this.DontAskAgainCheckBox.Checked;
         this.ParentForm.DialogResult = DialogResult.OK;
         this.ParentForm.Close();
      }


   }
}
