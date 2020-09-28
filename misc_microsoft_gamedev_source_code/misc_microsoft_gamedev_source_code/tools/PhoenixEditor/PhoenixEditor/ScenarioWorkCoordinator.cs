using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using EditorCore.Controls.Micro;

using EditorCore;

namespace PhoenixEditor
{
   public partial class ScenarioWorkCoordinator : UserControl
   {
      public ScenarioWorkCoordinator()
      {
         InitializeComponent();

         Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         this.TerrainWorkTopicControl.Topic = workTopics["Terrain"];
         this.SimWorkTopicControl.Topic = workTopics["Sim"];
         this.ArtObjectsTopicControl.Topic = workTopics["ArtObjects"];
         this.SoundWorkTopicControl.Topic = workTopics["Sounds"];
         this.LightsWorkTopicControl.Topic = workTopics["Lights"];
         this.MasksWorkTopicControl.Topic = workTopics["Masks"];

         this.TerrainWorkTopicControl.StateChanged += new EventHandler(TerrainWorkTopicControl_StateChanged);
         this.SimWorkTopicControl.StateChanged += new EventHandler(SimWorkTopicControl_StateChanged);
         this.ArtObjectsTopicControl.StateChanged += new EventHandler(ArtObjectsTopicControl_StateChanged);
         this.SoundWorkTopicControl.StateChanged += new EventHandler(SoundWorkTopicControl_StateChanged);
         this.LightsWorkTopicControl.StateChanged += new EventHandler(LightsWorkTopicControl_StateChanged);
         this.MasksWorkTopicControl.StateChanged += new EventHandler(MasksWorkTopicControl_StateChanged);

         this.SavingLabel.Text = "Save: " + Path.GetFileNameWithoutExtension(CoreGlobals.ScenarioFile);

         UpdateState();
      }

 

      void UpdateState()
      {
         bool filesBlocked = false;
         Dictionary<string, WorkTopic> workTopics = CoreGlobals.getScenarioWorkTopicManager().getScenarioWorkTopics();
         Dictionary<string, WorkTopic>.Enumerator it = workTopics.GetEnumerator();
         while (it.MoveNext())
         {
            if (it.Current.Value.mDataPattern != WorkTopic.eDataPatterm.cPrimary)
               continue;



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
            this.SaveButton.Enabled = false;
         }
         else
         {
            this.SaveButton.Enabled = true;
         }

         CoreGlobals.getScenarioWorkTopicManager().NotifyTopicsUpdated();

      }

      void MasksWorkTopicControl_StateChanged(object sender, EventArgs e)
      {
         UpdateState();
      }

      void LightsWorkTopicControl_StateChanged(object sender, EventArgs e)
      {
         UpdateState();
      }

      void SimWorkTopicControl_StateChanged(object sender, EventArgs e)
      {
         UpdateState();
      }

      void SoundWorkTopicControl_StateChanged(object sender, EventArgs e)
      {
         UpdateState();
      }

      void ArtObjectsTopicControl_StateChanged(object sender, EventArgs e)
      {
         UpdateState();
      }

      void TerrainWorkTopicControl_StateChanged(object sender, EventArgs e)
      {
         UpdateState();
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         CoreGlobals.getScenarioWorkTopicManager().sDontPromtResolvedSave = this.DontAskAgainCheckBox.Checked;
         this.ParentForm.DialogResult = DialogResult.OK;
         this.ParentForm.Close();
      }

      private void CancelButton_Click(object sender, EventArgs e)
      {
         this.ParentForm.DialogResult = DialogResult.Cancel;
         this.ParentForm.Close();

      }

      private void ScenarioWorkCoordinator_Load(object sender, EventArgs e)
      {

      }




   }



}
