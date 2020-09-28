using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml.Serialization;

using EditorCore;

namespace PhoenixEditor.ClientTabPages
{
   public partial class ScenarioDataPage : EditorCore.BaseClientPage
   {
      public ScenarioDataPage()
      {
         InitializeComponent();
         
         // lookup for the scenario file
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "File", "FileFilter", "Scenario (*.scn)|*.scn");
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "File", "RootDirectory", CoreGlobals.getWorkPaths().mGameScenarioDirectory);
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "File", "StartingDirectory", CoreGlobals.getWorkPaths().mGameScenarioDirectory);
         basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "File", typeof(FileNameProperty));

         // lookup for the loading screen
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "LoadingScreen", "FileFilter", "Flash (*.swf)|*.swf");
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "LoadingScreen", "RootDirectory", CoreGlobals.getWorkPaths().mGameDirectory);
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "LoadingScreen", "StartingDirectory", CoreGlobals.getWorkPaths().mGameFlashUIDirectory);
         basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "LoadingScreen", typeof(FileNameProperty));

         // lookup for the map image
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "MapName", "FileFilter", "ddx (*.ddx)|*.ddx");
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "MapName", "RootDirectory", CoreGlobals.getWorkPaths().mGameDirectory);
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "MapName", "StartingDirectory", CoreGlobals.getWorkPaths().mGameLoadmapDirectory);
         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "MapName", "FilePrefix", "img://");
         basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "MapName", typeof(FileNameProperty));

         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "Type", "SimpleEnumeration", new string[] {"Playtest","Development","Test","Campaign", "Alpha" });
         basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "Type", typeof(EnumeratedProperty));

         basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "MaxPlayers", "SimpleEnumeration", new string[] { "2", "4", "6" });
         basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "MaxPlayers", typeof(EnumeratedProperty));

         mDescriptionFilename = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "scenariodescriptions.xml");

         if(CoreGlobals.UsingPerforce == false)
         {
            CheckoutButton.Enabled = false;
            CheckInButton.Enabled = false;
            SaveButton.Enabled = false;
            return;

         }
         else
         {
            CheckoutButton.Enabled = false;
            CheckInButton.Enabled = false;

            //no perfore support yet...
            SaveButton.Enabled = true;
            basicTypedSuperList1.Enabled = true;

         }

         basicTypedSuperList1.AutoScroll = true;

         Load();
      }
      string mDescriptionFilename = "";

      ScenarioDescriptionsXml mScenarioDescriptions = null;

      public void Load()
      {
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(ScenarioDescriptionsXml), new Type[] { });
            Stream st = File.OpenRead(mDescriptionFilename);
            mScenarioDescriptions = (ScenarioDescriptionsXml)s.Deserialize(st);
            st.Close();

            this.basicTypedSuperList1.ObjectList = mScenarioDescriptions.ScenarioInfoList;
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void CheckoutButton_Click(object sender, EventArgs e)
      {

      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         try
         {

            XmlSerializer s = new XmlSerializer(typeof(ScenarioDescriptionsXml), new Type[] { });
            Stream st = File.Open(mDescriptionFilename, FileMode.Create);
            s.Serialize(st, mScenarioDescriptions);
            st.Close();

            XMBProcessor.CreateXMB(mDescriptionFilename, false);

         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void CheckInButton_Click(object sender, EventArgs e)
      {

      }

      private void AutoAddButton_Click(object sender, EventArgs e)
      {
         try
         {
            string[] subDirs = Directory.GetDirectories(CoreGlobals.getWorkPaths().mGameScenarioDirectory);

            foreach(string dir in subDirs)
            {
               string[] files = Directory.GetFiles(dir, "*.scn");
               foreach(string scenfile in files)
               {
                  if(SearchForFile(scenfile) == false)
                  {
                     ScenarioInfoXml scenInfo = new ScenarioInfoXml();
                     scenInfo.File = scenfile.Replace(CoreGlobals.getWorkPaths().mGameScenarioDirectory + "\\","");
                     //scenInfo.Name = Path.GetFileNameWithoutExtension(scenfile);
                     mScenarioDescriptions.ScenarioInfoList.Add(scenInfo);
                  }

               }
            }

            //basicTypedSuperList1.BatchSuspend();
           
            //this.basicTypedSuperList1.ObjectList = mScenarioDescriptions.ScenarioInfoList;
            basicTypedSuperList1.UpdateData();
            //basicTypedSuperList1.BatchResume();
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private bool SearchForFile( string filename)
      {
         foreach (ScenarioInfoXml info in mScenarioDescriptions.ScenarioInfoList)
         {
            if(filename.Contains(info.File) == true)
            {
               return true;
            }

         }
         return false;
      }



      private void AutoRemoveButton_Click(object sender, EventArgs e)
      {

      }

      private void basicTypedSuperList1_Load(object sender, EventArgs e)
      {

      }

   }
}

