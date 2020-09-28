using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using System.IO;
using System.Xml.Serialization;
using System.Threading;


using EditorCore;
using Terrain;
using SimEditor;




namespace PhoenixEditor
{
   public partial class TempLightSettings : UserControl, IEditorWindow//: Form, IEditorWindow
   {
      public TempLightSettings()
      {
         InitializeComponent();

         try
         {

            FileStream f = new FileStream(CoreGlobals.getWorkPaths().mEditorSettings + "\\GlobalLightUISettings.xml", FileMode.Open, FileAccess.Read);
            //Stream s = File.Open(CoreGlobals.getWorkPaths().mEditorSettings + "\\GlobalLightUISettings.xml", FileMode.Open);

            this.betterPropertyGrid1.LoadSettingsFromStream(f);
           betterPropertyGrid1.SetTypeEditor("lightSetXML", "ObjectPropertyForFLSGen", typeof(ScenarioEditor.PositionHelperObject));

         }
         catch(System.Exception ex)
         {
            ex.ToString();
         }
         reload();

         
      }

      public void reload()
      {
         try
         {

            if (this.IsDisposed)
               return;


            SkyBoxFile = CoreGlobals.ScenarioSkyboxFilename;
            EnvMapFile = CoreGlobals.ScenarioTerrainEnvMapFilename;

            string fileName;

            if (CoreGlobals.ScenarioDirectory != "")
            {

               fileName = Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioLightsetFilename);
            }
            else
            {
               //   MessageBox.Show("you must save the project first...(for now)");
               //this.Close();
               fileName = CoreGlobals.getWorkPaths().GetDefaultLightset();


               // fileName = CoreGlobals.getWorkPaths().
            }
            if (File.Exists(fileName))
            {
               loadXML(fileName,true);
            }
         }
         catch(System.Exception ex)
         {

         }


      }

      lightSetXML mLightSet = new lightSetXML();

      public void loadXML(string fileName)
      {
         try
         {
            textBox1.Text = fileName;


            //XmlSerializer s = new XmlSerializer(typeof(lightSetXML), new Type[] { });
            //Stream st = File.OpenRead(fileName);
            //mLightSet = (lightSetXML)s.Deserialize(st);
            //st.Close();

            mLightSet = SimGlobals.getSimMain().LoadScenarioLights(fileName,true);

            this.betterPropertyGrid1.SelectedObject = mLightSet;

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
      public void loadXML(string fileName, bool bOnlyGlobalSettings)
      {
         try
         {
            //textBox1.Text = fileName;

            List<LightXML> saveThelocalLights = mLightSet.mLights;

            //XmlSerializer s = new XmlSerializer(typeof(lightSetXML), new Type[] { });
            //Stream st = File.OpenRead(fileName);
            //mLightSet = (lightSetXML)s.Deserialize(st);
            //st.Close();
            mLightSet = SimGlobals.getSimMain().LoadScenarioLights(fileName, !bOnlyGlobalSettings);

            if (bOnlyGlobalSettings == true)
            {
               mLightSet.mLights = saveThelocalLights;
            }

            

            this.betterPropertyGrid1.SelectedObject = mLightSet;
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      public bool saveXML(string fileName)
      {
         if (!Monitor.TryEnter(this))
            return false;
         try
         {


            if (fileName == "")
               return false;

            if (File.Exists(fileName))
            {
               File.SetAttributes(fileName, FileAttributes.Normal);
               File.Delete(fileName);
            }
            //if (File.Exists(fileName) && ((File.GetAttributes(fileName) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly))
            if (File.Exists(fileName) && ((File.GetAttributes(fileName) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly))
            {
               MessageBox.Show(fileName + " is readonly.  please check it out");
               LiveUpdateCheckBox.Checked = false;
               return false;
            }
            try
            {
               //textBox1.Text = fileName;

               //XmlSerializer s = new XmlSerializer(typeof(lightSetXML), new Type[] { });
               //Stream st = File.Open(fileName, FileMode.OpenOrCreate);
               //s.Serialize(st, mLightSet);
               //st.Close();

               SimGlobals.getSimMain().SaveScenarioLights(fileName, this.mLightSet);

               return true;
            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnException(ex);
               LiveUpdateCheckBox.Checked = false;

            }
         }
         finally
         {
            Monitor.Exit(this);

         }
         return false;
      }

  
      private void Import_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();
         d.Filter = "Global Lightset (*.gls)|*.gls";
         d.FilterIndex = 0;
         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameLightsetBaseDirectory;

         if (d.ShowDialog() == DialogResult.OK)
         {
            loadXML(d.FileName,true); 
            SimGlobals.getSimMain().SetLightsDirty();


            CoreGlobals.getSaveLoadPaths().mGameLightsetBaseDirectory = Path.GetDirectoryName(d.FileName);

         }
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         SaveScenarioFile();
      }

      private void SaveScenarioFile()
      {
         string fileName = Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioLightsetFilename);
         saveXML(fileName); 
   

      }
      private void SaveQuickUpdateFile()
      {
         string scenerioPath = CoreGlobals.getWorkPaths().mGameScenarioDirectory;
         string fileName = scenerioPath + @"\quickview\quickview.gls";

         saveXML(fileName);
        
      }

      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "Global Lightset (*.gls)|*.gls";
         d.FilterIndex = 0;
         d.InitialDirectory = CoreGlobals.getWorkPaths().mGameLightsetBaseDirectory;

         if (d.ShowDialog() == DialogResult.OK)
         {

            if (saveXML(d.FileName))
            {
               CoreGlobals.ScenarioLightsetFilename = Path.GetFileName(d.FileName);
            }

         }
      }


      private void button1_Click(object sender, EventArgs e)
      {

         //if (mPerforce.IsPerforceInstalled())
         //{
         //   List<string> files = new List<string>();
         //   //files.Add(CoreGlobals.ProjectDirectory + "\\" + CoreGlobals.ScenarioLightsetFilename);
         //   files.Add("//depot/phoenix/xbox/work/scenario/fds3/" + CoreGlobals.ScenarioLightsetFilename);
         //   mTheList = mPerforce.CreateChangeList();
         //   //mPerforce.P4Add(files);
         //   //mPerforce.P4Checkout(files);



         //}
      }
      private void button3_Click(object sender, EventArgs e)
      {

      }

      private void LiveUpdateCheckBox_CheckedChanged(object sender, EventArgs e)
      {

      }

      private void LiveUpdateTimer_Tick(object sender, EventArgs e)
      {
         if (LiveUpdateCheckBox.Checked == true)
         {
            SaveScenarioFile();
            SaveQuickUpdateFile();
         }
      }

      //<Sky>environment\sky\sky_dome_01.vis</Sky>
      private void PickSkyboxButton_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();

         d.Filter = "Skybox vis (*.vis)|*.vis";
         d.FilterIndex = 0;
         d.InitialDirectory = CoreGlobals.getWorkPaths().mGameSkyBoxDirectory;

         if (d.ShowDialog() == DialogResult.OK)
         {
            SkyBoxFile = d.FileName;
         }
         
      }

      private string mSkyboxFileName = @"environment\sky\sky_dome_01";//.vis";

      string SkyBoxFile
      {
         get
         {

            return mSkyboxFileName;
         }
         set
         {
            //ResourcePathInfoOneOffArtRelative path = new ResourcePathInfoOneOffArtRelative(value);
            //if (path.IsFilePath && path.IsArtRelativePath)
            string filename = value;
            if(value == null)
            {
               filename = mSkyboxFileName;

            }

            string skyrelativePath = Path.Combine(@"environment\sky", Path.GetFileNameWithoutExtension(filename));

            //if(File.Exists(value))
            {
               mSkyboxFileName = skyrelativePath;
               if (filename == "")
               {
                  mSkyboxFileName = "";
               }
               this.SkyBoxFileTextBox.Text = mSkyboxFileName;

               CoreGlobals.ScenarioSkyboxFilename = mSkyboxFileName;
               SimGlobals.getSimMain().SetDirty();
            }
         }


      }

      private void SkyBoxFileTextBox_KeyPress(object sender, KeyPressEventArgs e)
      {         
         e.Handled = true;
      }


      private string mTerrEnvMapFileName = @"environment\sky\cloudy_sky_dome_en.ddx";

      string EnvMapFile
      {
         get
         {

            return mTerrEnvMapFileName;
         }
         set
         {
            string filename = value;
            if (value == null)
            {
               filename = mTerrEnvMapFileName;

            }

            string skyrelativePath = Path.Combine(@"environment\sky", Path.GetFileName(filename));



            //if(File.Exists(value))
            {
               mTerrEnvMapFileName = skyrelativePath;
               if (filename == "")
               {
                  mTerrEnvMapFileName = "";
               }
               this.EnvMapFileTextBox.Text = mTerrEnvMapFileName;

               CoreGlobals.ScenarioTerrainEnvMapFilename = mTerrEnvMapFileName;
               SimGlobals.getSimMain().SetDirty();
            }
         }


      }




      private void PickEnvMapboxButton_Click(object sender, EventArgs e)
      {
         OpenFileDialog d = new OpenFileDialog();

         d.Filter = "HDR Skydome Texture (*.ddx)|*.ddx";
         d.FilterIndex = 0;
         d.InitialDirectory = CoreGlobals.getWorkPaths().mGameSkyBoxDirectory;

         if (d.ShowDialog() == DialogResult.OK)
         {
            EnvMapFile = d.FileName;
         }
      }

      private void EnvMapFileTextBox_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void propertyGrid1_Click(object sender, EventArgs e)
      {

      }

      private void ExportButton_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.Filter = "Global Lightset (*.gls)|*.gls";
         d.FilterIndex = 0;
         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mGameLightsetBaseDirectory;

         if (d.ShowDialog() == DialogResult.OK)
         {
            saveXML(d.FileName);

            CoreGlobals.getSaveLoadPaths().mGameLightsetBaseDirectory = Path.GetDirectoryName(d.FileName);
         }
      }

      private void button4_Click(object sender, EventArgs e)
      {
         if (mLightSet.getObjectIDForFLSGen() == -1)
         {
            MessageBox.Show("You must specify a light probe object before generating an FLS file");
            return;
         }
         
         EditorObject eo = SimGlobals.getSimMain().GetEditorObjectByID(mLightSet.getObjectIDForFLSGen());
         if (eo == null)
            return;

         HelperPositionObject hao = eo as HelperPositionObject;

         string lightsetFile = Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioLightsetFilename);
         string flsName = Path.ChangeExtension(lightsetFile, ".fls");

       //  CoreGlobals.getEditorMain().mIGUI.doQuickView(false);

       //  System.Threading.Thread.Sleep(10000);
         XFSInterface.generateFLS(hao.getPosition().X, hao.getPosition().Y, hao.getPosition().Z, flsName);
      }

      private void ClearSkyDomeButton_Click(object sender, EventArgs e)
      {
         SkyBoxFile = "";
      }

      private void ClearTerrainEnvButton_Click(object sender, EventArgs e)
      {
         EnvMapFile = "";
      }

   }




}