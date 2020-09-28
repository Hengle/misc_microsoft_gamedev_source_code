using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.IO;
using System.Windows.Forms;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using EditorCore;
using Terrain;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ScenarioSettings : UserControl
   {
      public ScenarioSettings()
      {
         InitializeComponent();

         //lightSetListControl.AddMetaDataForProp("EditorLightset", "File", "FileFilter", "Scenario (*.scn)|*.scn");


         minXBounds.ValueChanged += new EventHandler(minXBounds_ValueChanged);
         maxXBounds.ValueChanged += new EventHandler(maxXBounds_ValueChanged);
         minZBounds.ValueChanged += new EventHandler(minZBounds_ValueChanged);
         maxZBounds.ValueChanged += new EventHandler(maxZBounds_ValueChanged);


      }

      /// <summary>
      /// This will prompt the user to check out the proper work topic if needed.
      /// </summary>
      private void SettingsChanged()
      {
         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission("Sim") == false)
         {
            return;
         }

      }
      bool mbThisErrorOnce1 = false;
      bool isLoading = false;
      public void isActive()
      {

         isLoading = true;
         minXBounds.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         maxXBounds.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         minZBounds.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         maxZBounds.Setup(0, TerrainGlobals.getTerrain().getNumXVerts(), false);
         
         
         minXBounds.NumericValue = CoreGlobals.mPlayableBoundsMinX;
         maxXBounds.NumericValue = CoreGlobals.mPlayableBoundsMaxX;
         minZBounds.NumericValue = CoreGlobals.mPlayableBoundsMinZ;
         maxZBounds.NumericValue = CoreGlobals.mPlayableBoundsMaxZ;

         textBox1.Text = CoreGlobals.ScenarioMinimapTextureFilename;
        
         string[] temp = new string[CoreGlobals.ScenarioSoundbankFilenames.Count];
         for(int i = 0; i < CoreGlobals.ScenarioSoundbankFilenames.Count; i++)
         {
            temp[i] = CoreGlobals.ScenarioSoundbankFilenames[i];            
         }
         soundBankBox.Lines = temp;

         //-- DJB: Yeah this is ugly, but we're at the end of the project, and it's not gonna change.
         worldComboBox.Items.Clear();
         worldComboBox.Items.Add("");
         worldComboBox.Items.Add("Harvest");
         worldComboBox.Items.Add("Arcadia");
         worldComboBox.Items.Add("SWI");
         worldComboBox.Items.Add("SWE");

         worldComboBox.Text = CoreGlobals.ScenarioWorld;

         cinematicListBox.Items.Clear();
         talkingHeadVideos.Items.Clear();

         int count = CoreGlobals.getGameResources().getNumCinematics();
         for(int i = 0; i < count; i++)
         {
            EditorCinematic ecin = CoreGlobals.getGameResources().getCinematic(i);
            cinematicListBox.Items.Add(ecin.Name);
         }


         count = CoreGlobals.getGameResources().getNumTalkingHeadVideos();
         for (int i = 0; i < count; i++)
         {
            EditorCinematic ecin = CoreGlobals.getGameResources().getTalkingHeadVideo(i);
            talkingHeadVideos.Items.Add(ecin.Name);
         }

         count = CoreGlobals.getGameResources().getNumLightsets();
         
         LightsetListBox.Items.Clear();

         for (int i = 0; i < count; i++)
         {
            EditorLightset ecin = CoreGlobals.getGameResources().getLightset(i);
            LightsetListBox.Items.Add(ecin.getIDString());
         }

         comboBox1.Items.Clear();
         comboBox2.Items.Clear();
         int numSplatTex = TerrainGlobals.getTexturing().getActiveTextureCount();
         for (int i = 0; i < numSplatTex; i++)
         {
            if (TerrainGlobals.getTexturing().getActiveTexture(i) == null )
            {
               if (mbThisErrorOnce1 == false)
               {
                  mbThisErrorOnce1 = true;
                  CoreGlobals.ShowMessage("Please report this error to Andrew and Colt: invalid index in isActive ");
               }

               continue;
            }
            comboBox1.Items.Add(Path.GetFileName(TerrainGlobals.getTexturing().getActiveTexture(i).mFilename));
            comboBox2.Items.Add(Path.GetFileName(TerrainGlobals.getTexturing().getActiveTexture(i).mFilename));
         }
         if (CoreGlobals.ScenarioBuildingTextureIndexUNSC >= comboBox1.Items.Count)
            CoreGlobals.ScenarioBuildingTextureIndexUNSC = 0;
         comboBox1.SelectedIndex = CoreGlobals.ScenarioBuildingTextureIndexUNSC;

         if (CoreGlobals.ScenarioBuildingTextureIndexCOVN >= comboBox1.Items.Count)
            CoreGlobals.ScenarioBuildingTextureIndexCOVN = 0;
         comboBox2.SelectedIndex = CoreGlobals.ScenarioBuildingTextureIndexCOVN;

         VeterancyCheck.Checked = CoreGlobals.mbAllowVeterancy;
         LoadVisRepBox.Checked = CoreGlobals.mbLoadTerrainVisRep;


         GlobalExcludeUnitsOptionChooser.Enabled = true;
         GlobalExcludeUnitsOptionChooser.SetOptions(TriggerSystemMain.mSimResources.mObjectTypeData.mObjectTypeList);
         GlobalExcludeUnitsOptionChooser.BoundSelectionList = SimGlobals.getSimMain().GlobalExcludeObjects;
         GlobalExcludeUnitsOptionChooser.AllowRepeats = false;
         GlobalExcludeUnitsOptionChooser.AutoSort = true;

         isLoading = false;
      }

      void maxZBounds_ValueChanged(object sender, EventArgs e)
      {
         if (isLoading) return;
         if (maxZBounds.NumericValue < CoreGlobals.mPlayableBoundsMinZ)
         {
            maxZBounds.NumericValue = CoreGlobals.mPlayableBoundsMinZ + 1;
            return;
         }
         CoreGlobals.mPlayableBoundsMaxZ = (int)(maxZBounds.NumericValue);

         SettingsChanged();
      }

      void minZBounds_ValueChanged(object sender, EventArgs e)
      {
         if (isLoading) return;
         if (minZBounds.NumericValue > CoreGlobals.mPlayableBoundsMaxZ)
         {
            minZBounds.NumericValue = CoreGlobals.mPlayableBoundsMaxZ - 1;
            return;
         }
         CoreGlobals.mPlayableBoundsMinZ = (int)(minZBounds.NumericValue);

         SettingsChanged();
      }

      void maxXBounds_ValueChanged(object sender, EventArgs e)
      {
         if (isLoading) return;
         if (maxXBounds.NumericValue < CoreGlobals.mPlayableBoundsMinX)
         {
            maxXBounds.NumericValue = CoreGlobals.mPlayableBoundsMinX + 1;
            return;
         }
         CoreGlobals.mPlayableBoundsMaxX = (int)(maxXBounds.NumericValue);

         SettingsChanged();
      }

      void minXBounds_ValueChanged(object sender, EventArgs e)
      {
         if (isLoading) return;
         if (minXBounds.NumericValue > CoreGlobals.mPlayableBoundsMaxX)
         {
            minXBounds.NumericValue = CoreGlobals.mPlayableBoundsMaxX - 1;
            return;
         }
         CoreGlobals.mPlayableBoundsMinX = (int)(minXBounds.NumericValue);

         SettingsChanged();
      }

      public void setMinX(int x)
      {
         minXBounds.NumericValue = x;
      }
      public void setMaxX(int x)
      {
         maxXBounds.NumericValue = x;
      }
      public void setMinZ(int x)
      {
         minZBounds.NumericValue = x;
      }
      public void setMaxZ(int x)
      {
         maxZBounds.NumericValue = x;
      }
      public int getMinX(int x)
      {
         return (int)(minXBounds.NumericValue);
      }
      public int getMaxX(int x)
      {
         return (int)(maxXBounds.NumericValue);
      }
      public int getMinZ(int x)
      {
         return (int)(minZBounds.NumericValue);
      }
      public int getMaxZ(int x)
      {
         return (int)(maxZBounds.NumericValue);
      }

      private void button1_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();

         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameMinimapDirectory;
         
         d.Filter = "ES Texture File (*.ddx)|*.ddx";
         d.FilterIndex = 0;

         if (d.ShowDialog() == DialogResult.OK)
         {
            CoreGlobals.ScenarioMinimapTextureFilename = d.FileName.Replace(CoreGlobals.getWorkPaths().mGameArtDirectory, "");
            textBox1.Text = CoreGlobals.ScenarioMinimapTextureFilename;
         }

         SettingsChanged();
      }

      private void cinematicAddButton_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();

         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameArtDirectory;

         d.Filter = "ES Cinematic File (*.cin)|*.cin";
         d.FilterIndex = 0;

         if (d.ShowDialog() == DialogResult.OK)
         {
            string filename = d.FileName.Replace(CoreGlobals.getWorkPaths().mGameArtDirectory, "");

            // Remove extension
            filename = filename.Remove(filename.Length - 4);

            //kill leading \\
            filename = filename.TrimStart(new char[] { '\\' });

            // Check to see if this is a duplicate
            if (!CoreGlobals.getGameResources().ContainsCinematic(filename))
            {
               // Add to data
               CoreGlobals.getGameResources().AddCinematic(filename);

               // Pause painting
               cinematicListBox.BeginUpdate();

               // Add to list box
               cinematicListBox.Items.Add(filename);

               // Assign list box selection to the new objective
               cinematicListBox.SelectedIndex = cinematicListBox.Items.IndexOf(filename);

               // Resume painting
               cinematicListBox.EndUpdate();
            }
            else
            {
               MessageBox.Show("The cinematic you have selected is already on the list");
            }
         }

         SettingsChanged();
      }

      private void cinematicDeleteButton_Click(object sender, EventArgs e)
      {
         if (cinematicListBox.SelectedItem == null)
         {
            return;
         }

         // Delete objective from local list
         CoreGlobals.getGameResources().RemoveCinematic(cinematicListBox.SelectedItem.ToString());

         // Pause painting
         cinematicListBox.BeginUpdate();

         // Grab objective from list box
         object deletedCinematic = cinematicListBox.SelectedItem;

         // Select the next cinematic in the list box         
         if ((cinematicListBox.SelectedIndex + 1) < cinematicListBox.Items.Count)
         {
            cinematicListBox.SelectedIndex += 1;
         }
         else if (cinematicListBox.Items.Count >= 2)
         {
            cinematicListBox.SelectedIndex -= 1;
         }

         // Delete the objective from list box
         cinematicListBox.Items.Remove(deletedCinematic);

         // Continue painting
         cinematicListBox.EndUpdate();


         SettingsChanged();
      }

      private void AddLightsetButton_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();

         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameArtDirectory + "\\lightsets";

         d.Filter = "ES Lightset File (*.gls)|*.gls";
         d.FilterIndex = 0;

         if (d.ShowDialog() == DialogResult.OK)
         {
            string filename = d.FileName.Replace(CoreGlobals.getWorkPaths().mGameArtDirectory, "");

            // Remove extension
            filename = filename.Remove(filename.Length - 4);

            //kill leading \\
            filename = filename.TrimStart(new char[] { '\\' });

            // Check to see if this is a duplicate
            if (!CoreGlobals.getGameResources().ContainsLightset(filename))
            {
               // Add to data
               EditorLightset set = CoreGlobals.getGameResources().AddLightset(filename);

               // Pause painting
               LightsetListBox.BeginUpdate();

               // Add to list box
               LightsetListBox.Items.Add(set.getIDString());

               // Assign list box selection to the new objective
               LightsetListBox.SelectedIndex = LightsetListBox.Items.IndexOf(filename);

               // Resume painting
               LightsetListBox.EndUpdate();
            }
            else
            {
               MessageBox.Show("The Lightset you have selected is already on the list");
            }
         }

         SettingsChanged();

      }

      private void DeleteLightsetButton_Click(object sender, EventArgs e)
      {
         if (LightsetListBox.SelectedItem == null)
         {
            return;
         }

         // Delete objective from local list
         string name = LightsetListBox.SelectedItem.ToString().Split('#')[1];

         CoreGlobals.getGameResources().RemoveLightset(name);

         // Pause painting
         LightsetListBox.BeginUpdate();

         // Grab objective from list box
         object deletedLightset = LightsetListBox.SelectedItem;

         // Select the next Lightset in the list box         
         if ((LightsetListBox.SelectedIndex + 1) < LightsetListBox.Items.Count)
         {
            LightsetListBox.SelectedIndex += 1;
         }
         else if (LightsetListBox.Items.Count >= 2)
         {
            LightsetListBox.SelectedIndex -= 1;
         }

         // Delete the objective from list box
         LightsetListBox.Items.Remove(deletedLightset);

         // Continue painting
         LightsetListBox.EndUpdate();

         SettingsChanged();

      }

      private void textBox2_TextChanged(object sender, EventArgs e)
      {
         CoreGlobals.ScenarioSoundbankFilenames.Clear();
         for (int i = 0; i < soundBankBox.Lines.Length; i++)
         {
            if (soundBankBox.Lines[i] == "")
               continue;
            CoreGlobals.ScenarioSoundbankFilenames.Add(soundBankBox.Lines[i]);
         }

         SettingsChanged();
      }

      private void button2_Click(object sender, EventArgs e)
      {
        
      }

      private void button3_Click(object sender, EventArgs e)
      {
        
      }

      private void LightsetListBox_SelectedIndexChanged(object sender, EventArgs e)
      {

      }

      private void generateFLSToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if (LightsetListBox.SelectedItem == null)
            return;

          EditorLightset ls = CoreGlobals.getGameResources().getLightset(LightsetListBox.SelectedIndex);
          if (ls.ObjectIDForFLSGen == -1)
          {
             MessageBox.Show("You must specify a light probe object before generating an FLS file");
             return;
          }

         EditorObject eo = SimGlobals.getSimMain().GetEditorObjectByID(ls.ObjectIDForFLSGen) ;
         if (eo == null)
         {
            MessageBox.Show("The specified light probe object does not exist. Please select a new one.");
            ls.ObjectPropertyForFLSGen = "";
            return;  
         }

         HelperPositionObject hao = eo as HelperPositionObject;

         if(hao==null)
         {
            MessageBox.Show("Error converting EditorObject to HelperPositionObject");
            ls.ObjectPropertyForFLSGen = "";
            return;  
         }

         Vector3 pos = hao.getPosition();

         string lightsetFile = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\" + LightsetListBox.SelectedItem.ToString().Split('#')[1];
         string flsName = lightsetFile + ".fls";

         CoreGlobals.getEditorMain().mIGUI.doQuickView();

         System.Threading.Thread.Sleep(8000);
         XFSInterface.generateFLS(pos.X, pos.Y, pos.Z , flsName);
      }

      private void setFLSGenLocationToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if (LightsetListBox.SelectedIndex == -1)
            return;

         PopupEditor pe = new PopupEditor();
         BetterPropertyGrid bpg = new BetterPropertyGrid();
         bpg.Height = 40;
         
         EditorLightset ls = CoreGlobals.getGameResources().getLightset(LightsetListBox.SelectedIndex);

         bpg.IgnoreProperties("EditorLightset", new string[] { "ID", "Name" });
         bpg.SetTypeEditor("EditorLightset", "ObjectPropertyForFLSGen", typeof(PositionHelperObject));
         bpg.SelectedObject = ls;

         pe.ShowPopup(this, bpg);
      }

      private void textBox1_TextChanged(object sender, EventArgs e)
      {

      }

      private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
      {
         CoreGlobals.ScenarioBuildingTextureIndexUNSC = comboBox1.SelectedIndex;
         SettingsChanged();
      }

      private void comboBox2_SelectedIndexChanged(object sender, EventArgs e)
      {
         CoreGlobals.ScenarioBuildingTextureIndexCOVN = comboBox2.SelectedIndex;
         SettingsChanged();
      }

      private void worldComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         CoreGlobals.ScenarioWorld = worldComboBox.Text;         
         SettingsChanged();
      }

      private void talkingHeadAddBtn_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();

         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mTalkingHeadDirectory;

         d.Filter = "Bink Files (*.bik)|*.bik";
         d.FilterIndex = 0;
         d.Multiselect = true;

         if (d.ShowDialog() == DialogResult.OK)
         {
            string[] filenames = d.FileNames;

            for (int i=0; i<filenames.Length; i++)
            {
               //string filename = d.FileName.Replace(CoreGlobals.getWorkPaths().mTalkingHeadDirectory, "");
               string filename = filenames[i].Replace(CoreGlobals.getWorkPaths().mTalkingHeadDirectory, "");

               // Remove extension
               filename = filename.Remove(filename.Length - 4);

               //kill leading \\
               filename = filename.TrimStart(new char[] { '\\' });

               // Check to see if this is a duplicate
               if (!CoreGlobals.getGameResources().ContainsTalkingHeadVideo(filename))
               {
                  // Add to data
                  CoreGlobals.getGameResources().AddTalkingHeadVideo(filename);

                  // Pause painting
                  talkingHeadVideos.BeginUpdate();

                  // Add to list box
                  talkingHeadVideos.Items.Add(filename);

                  // Assign list box selection to the new objective
                  talkingHeadVideos.SelectedIndex = talkingHeadVideos.Items.IndexOf(filename);

                  // Resume painting
                  talkingHeadVideos.EndUpdate();
               }
            }
         }

         SettingsChanged();
      }

      private void talkingHeadDeleteBtn_Click(object sender, EventArgs e)
      {
         if (talkingHeadVideos.SelectedItem == null)
         {
            return;
         }

         //CLM this delete function was flakey..
         object selectedItem = talkingHeadVideos.SelectedItem;
         string seletedItemString = talkingHeadVideos.SelectedItem.ToString();
         int selectedIndex = talkingHeadVideos.SelectedIndex;

         

         // Delete objective from local list
         CoreGlobals.getGameResources().RemoveTalkingHeadVideo(seletedItemString);
         

         // Pause painting
         talkingHeadVideos.BeginUpdate();

         // Delete the objective from list box
         talkingHeadVideos.Items.Remove(selectedItem);

         // Select the next cinematic in the list box         
         if (selectedIndex >= talkingHeadVideos.Items.Count)
         {
            talkingHeadVideos.SelectedIndex = talkingHeadVideos.Items.Count-1;
         } 
         else
         {
            talkingHeadVideos.SelectedIndex = selectedIndex;
         }

         
         // Continue painting
         talkingHeadVideos.EndUpdate();

         SettingsChanged();
      }

      private void VeterancyCheck_CheckedChanged(object sender, EventArgs e)
      {
         CoreGlobals.mbAllowVeterancy = VeterancyCheck.Checked;
      }

      private void LoadVisRepBox_CheckedChanged(object sender, EventArgs e)
      {
         CoreGlobals.mbLoadTerrainVisRep = LoadVisRepBox.Checked;
      }

      ///READ THIS/////////////////////////////////////////////
      ///READ THIS/////////////////////////////////////////////
      ///READ THIS/////////////////////////////////////////////
      ///If you add to scenario settings, please add the new "SettingsChanged();"  
      
   }
}
