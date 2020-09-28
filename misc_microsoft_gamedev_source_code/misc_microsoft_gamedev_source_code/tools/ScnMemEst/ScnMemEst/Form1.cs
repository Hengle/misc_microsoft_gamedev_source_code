using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.IO;
using System.Windows.Forms;
using System.Xml.Serialization;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace ScnMemEst
{
   public partial class Form1 : Form
   {

      string outputDir = "ScnMemoryEst\\";

      public Form1()
      {
         InitializeComponent();

         outputDir = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory) + "\\ScnMemoryEst\\";

         textBox1.Text = outputDir;
      }

      private void populateScenarioListBox()
      {
         scenarioListBox.Items.Clear();

         List<string> scnList = CalcScnMemEst.generateScenarioList(checkBox1.Checked,checkBox2.Checked,checkBox3.Checked,checkBox4.Checked);
         for (int i = 0; i < scnList.Count; i++)
            scenarioListBox.Items.Add(scnList[i]);
      }

      private void Form1_Load(object sender, EventArgs e)
      {
         populateScenarioListBox();
      }


      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         populateScenarioListBox();
      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         populateScenarioListBox();
      }

      private void checkBox3_CheckedChanged(object sender, EventArgs e)
      {
         populateScenarioListBox();
      }

      private void checkBox4_CheckedChanged(object sender, EventArgs e)
      {
         populateScenarioListBox();
      }

      private void button1_Click(object sender, EventArgs e)
      {
         DDXBridge.init();

         if(!Directory.Exists(outputDir))
            Directory.CreateDirectory(outputDir);

         List<string> selectedFiles = new List<string>();
         for (int i = 0; i < scenarioListBox.SelectedItems.Count;i++ )
         {
            selectedFiles.Add(scenarioListBox.SelectedItems[i] as string);
         }

         for (int i = 0; i < selectedFiles.Count; i++)
         {

            ScnMemoryEstimate scnEst = new ScnMemoryEstimate();

            XboxModelEstimate xme = new XboxModelEstimate();
            xme.estimateMemory(selectedFiles[i], CalcScnMemEst.gameDirectory, scnEst);

            XboxTerrainEstimate xte = new XboxTerrainEstimate();
            xte.estimateMemory(selectedFiles[i], CalcScnMemEst.gameDirectory, scnEst);

            scnEst.exportToFile(outputDir + Path.GetFileNameWithoutExtension(selectedFiles[i]) + ".csv");
         }

         DDXBridge.destroy();

         Process.Start(outputDir);
      }

      private void button2_Click(object sender, EventArgs e)
      {
         Shell32.ShellClass shell = new Shell32.ShellClass();
         Shell32.Folder2 folder = (Shell32.Folder2)shell.BrowseForFolder(
           this.Handle.ToInt32(), // Window handle as type int
           "Select Folder...", // Window caption
           0, // Option flags...the only part you'll have to hard-code values for if you want them
           Shell32.ShellSpecialFolderConstants.ssfDESKTOP // Optional root folder, default is Desktop
         );
         if (folder == null)
            return;

         this.textBox1.Text = folder.Self.Path;
         outputDir = folder.Self.Path; ;
      }
   }





   class CalcScnMemEst
   {
      static public string gameDirectory = null;


      static string calculateGameDir()
      {

         string gameDir = AppDomain.CurrentDomain.BaseDirectory;// mBaseDirectory;

         if (gameDir.Contains("\\work\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\work\\")) + "\\work";
         }
         else if (gameDir.Contains("\\production\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\production\\")) + "\\production\\work";
         }
         else if (gameDir.Contains("\\xbox\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\xbox\\")) + "\\xbox\\work";
         }
         else if (gameDir.Contains("\\x\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\x\\")) + "\\x\\work";
         }
         else if (gameDir.Contains("\\X\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\X\\")) + "\\X\\work";
         }
         else
         {
            return "";
         }

         return gameDir;

      }

      public enum eListFilter
      {
         eShowAll = 0,
         ePlaytestOnly,
      }
      public static List<string> generateScenarioList(bool development, bool playtest, bool campaign, bool alpha)
      {
         if (gameDirectory == null)
            gameDirectory = calculateGameDir();

         List<string> scenarioList = new List<string>();
         string mDescriptionFilename = gameDirectory + @"\data\scenariodescriptions.xml";
         ScenarioDescriptionsXml mScenarioDescriptions = null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(ScenarioDescriptionsXml), new Type[] { });
            Stream st = File.OpenRead(mDescriptionFilename);
            mScenarioDescriptions = (ScenarioDescriptionsXml)s.Deserialize(st);
            st.Close();


            for (int i = 0; i < mScenarioDescriptions.ScenarioInfoList.Count; i++)
            {
               bool add = false;

               add |= (campaign && mScenarioDescriptions.ScenarioInfoList[i].Type == "Campaign");
               add |= (development && mScenarioDescriptions.ScenarioInfoList[i].Type == "Development");
               add |= (playtest && mScenarioDescriptions.ScenarioInfoList[i].Type == "Playtest");
               add |= (alpha && mScenarioDescriptions.ScenarioInfoList[i].Type == "Alpha");


               if (add)
                  scenarioList.Add(mScenarioDescriptions.ScenarioInfoList[i].File);
            }


            return scenarioList;
         }
         catch (System.Exception ex)
         {

         }
         return null;
      }




   };

   #region XML copies
   [XmlRoot("ScenarioDescriptions")]
   public class ScenarioDescriptionsXml
   {
      List<ScenarioInfoXml> mScenarioInfo = new List<ScenarioInfoXml>();

      //[XmlArrayItem(ElementName = "ScenarioInfo", Type = typeof(PlayerPositionXML))]
      //[XmlArray("ScenarioInfo")]
      [XmlElement("ScenarioInfo", typeof(ScenarioInfoXml))]
      public List<ScenarioInfoXml> ScenarioInfoList
      {
         get
         {
            return mScenarioInfo;
         }
         set
         {
            mScenarioInfo = value;
         }
      }
   }

   [XmlRoot("ScenarioInfo")]
   public class ScenarioInfoXml
   {
      string mMapNameKeyFrame = "placeholder";
      string mMaxPlayers = "2";
      int mNameStringID = 0;
      int mInfoStringID = 0;
      string mType = "Test";
      string mScenario = "";

      [XmlAttribute]
      public string MapName
      {
         get
         {
            return mMapNameKeyFrame;
         }
         set
         {
            mMapNameKeyFrame = value;
         }
      }

      [XmlAttribute]
      public string MaxPlayers
      {
         get
         {
            return mMaxPlayers;
         }
         set
         {
            mMaxPlayers = value;
         }
      }


      [XmlAttribute]
      public int NameStringID
      {
         get
         {
            return mNameStringID;
         }
         set
         {
            mNameStringID = value;
         }
      }

      [XmlAttribute]
      public int InfoStringID
      {
         get
         {
            return mInfoStringID;
         }
         set
         {
            mInfoStringID = value;
         }
      }

      [XmlAttribute]
      public string Type
      {
         get
         {
            return mType;
         }
         set
         {
            mType = value;
         }
      }

      [XmlAttribute]
      public string File
      {
         get
         {
            return mScenario;
         }
         set
         {
            mScenario = value;
         }
      }
   }
   #endregion



}