using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TemplateVersionControl : UserControl
   {
      public TemplateVersionControl()
      {
         InitializeComponent();


         ItemPropertyGrid.IgnoreProperties("TemplateVersionInfo", new string[] { "Name"});
         ItemPropertyGrid.AddMetaDataForProps("TemplateVersionInfo", new string[] { "Version", "DBID" }, "ReadOnly", true);
         ItemPropertyGrid.AddMetaDataForProp("TemplateVersionInfo", "File", "FileFilter", "Template (*.xml)|*.xml");
         ItemPropertyGrid.AddMetaDataForProp("TemplateVersionInfo", "File", "RootDirectory", CoreGlobals.getWorkPaths().mTemplateRoot);
         ItemPropertyGrid.AddMetaDataForProp("TemplateVersionInfo", "File", "StartingDirectory", CoreGlobals.getWorkPaths().mTemplateRoot);
         ItemPropertyGrid.SetTypeEditor("TemplateVersionInfo", "File", typeof(FileNameProperty));
         //ItemPropertyGrid.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(On_AnyPropertyChanged);

         LoadData();
         LoadUI();


      }

      protected void LoadData()
      {
         TriggerSystemMain.mTriggerDefinitions.LoadTemplateDescriptions();
      }
      protected void SaveData()
      {
         TriggerSystemMain.mTriggerDefinitions.SaveTemplateDescriptions();
      }
      class VersionListItem
      {
         public VersionListItem(TemplateVersionInfo info)
         {
            mDBID = info.DBID;
            mName = info.Name;
         }
         public override string ToString()
         {
            return mName;
         }
         public int mDBID;
         public string mName;
      }

      protected void LoadUI()
      {
         this.TemplateNamesListBox.Items.Clear();

         if(this.HideObsoleteCheckBox.Checked == true)
         {        
            foreach(TemplateVersionInfo info in  TriggerSystemMain.mTriggerDefinitions.mCurrentTemplates.Values)
            {
               TemplateNamesListBox.Items.Add(new VersionListItem(info));
            }
         }
         else
         {
            foreach (Dictionary<int,TemplateVersionInfo> allInfo in TriggerSystemMain.mTriggerDefinitions.mAllTemplateInformation.Values)
            {            
               Dictionary<int,TemplateVersionInfo>.Enumerator it = allInfo.GetEnumerator();
               if(it.MoveNext())
               {
                  TemplateNamesListBox.Items.Add(new VersionListItem(it.Current.Value));
               }    
            }

         }

      }
      private void TemplateNamesListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         VersionListItem info = TemplateNamesListBox.SelectedItem as VersionListItem;
         if (info == null)
            return;

         this.VersionsListBox.Items.Clear();
         Dictionary<int,TemplateVersionInfo> allInfo = TriggerSystemMain.mTriggerDefinitions.mAllTemplateInformation[info.mDBID];
         Dictionary<int, TemplateVersionInfo>.Enumerator it = allInfo.GetEnumerator();
         while(it.MoveNext())
         {
            this.VersionsListBox.Items.Add(it.Current.Value);
         }
         VersionsListBox.SelectedIndex = 0;

      }

      private void VersionsListBox_SelectedValueChanged(object sender, EventArgs e)
      {
         TemplateVersionInfo info = VersionsListBox.SelectedItem as TemplateVersionInfo;
         if(info != null)
         {
            this.ItemPropertyGrid.SelectedObject = info;
            this.NameTextBox.Text = info.Name;
         }
      }

      private void AddTemplateButton_Click(object sender, EventArgs e)
      {
         //OpenFileDialog ofd = new OpenFileDialog();

         int dbid = TriggerSystemMain.mTriggerDefinitions.mTemplateVersionFile.GetNextDBID();
         TemplateVersionInfo newInfo = new TemplateVersionInfo();

         newInfo.DBID = dbid;
         newInfo.Version = 1;
         {

            TriggerSystemMain.mTriggerDefinitions.mTemplateVersionFile.TemplateInfo.Add(newInfo);
         }

         SaveData();
         LoadData();
         LoadUI();
      }

      private void AddNewVersionButton_Click(object sender, EventArgs e)
      {
         VersionListItem info = TemplateNamesListBox.SelectedItem as VersionListItem;
         if (info == null)
            return;

         TemplateVersionInfo newInfo = new TemplateVersionInfo();
         newInfo.DBID = info.mDBID;
         newInfo.Name = info.mName;

         Dictionary<int,TemplateVersionInfo>.Enumerator it = TriggerSystemMain.mTriggerDefinitions.mAllTemplateInformation[info.mDBID].GetEnumerator();
         int topversion = -1;
         while(it.MoveNext())
         {
            if(it.Current.Value.Version > topversion)
               topversion = it.Current.Value.Version;
         }
         topversion++;

         newInfo.Version = topversion;
         {


            TriggerSystemMain.mTriggerDefinitions.mTemplateVersionFile.TemplateInfo.Add(newInfo);
         }


         SaveData();
         LoadData();
         LoadUI();
      }

      private void SaveChangesButton_Click(object sender, EventArgs e)
      {
         SaveData();
         LoadData();
         LoadUI();

      }



      private void HideObsoleteCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         LoadUI();
      }


   }
}
