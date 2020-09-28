using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;

using EditorCore;

namespace PhoenixEditor
{
   public partial class SoundObjectImporter : Form
   {
      public SoundObjectImporter()
      {
         InitializeComponent();

         loadProtoXML();
         LoadSoundNames();
         LoadUI();
         SetButtonStatus();
      }

      void SetButtonStatus()
      {
         if (this.SoundsToImport.Items.Count == 0)
         {
            this.ImportButton.Enabled = false;
            this.RemoveSelectedButton.Enabled = false;
         }
         else
         {
            this.ImportButton.Enabled = true;
            this.RemoveSelectedButton.Enabled = true;
         }
      }

      List<string> mSoundNameList = new List<string>();
      public void LoadSoundNames()
      {

         // Read sound bank
         string wwiseFilename = CoreGlobals.getWorkPaths().mGameDirectory + "\\sound\\wwise_material\\generatedSoundBanks\\Wwise_IDs.h";
         mSoundNameList.Clear();
         if (!File.Exists(wwiseFilename))
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Can't find wwise file: {0}", wwiseFilename));
         }

         // Create an instance of StreamReader to read from a file.
         // The using statement also closes the StreamReader.
         using (StreamReader sr = new StreamReader(wwiseFilename))
         {
            String line;

            // Skip all lines until I get to the Events section
            string eventString = "namespace EVENTS";

            while ((line = sr.ReadLine()) != null)
            {
               if (line.Contains(eventString))
               {
                  break;
               }
            }

            // Find next "{"
            while ((line = sr.ReadLine()) != null)
            {
               if (line.Contains("{"))
                  break;
            }
         
            // Start reading names now until closing bracket "}"
            while ((line = sr.ReadLine()) != null)
            {
               if (line.Contains("}"))
                  break;

               string soundname = line;

               // Select just the event name
               string[] splitSpace = soundname.Split(' ');
               if (splitSpace.Length >= 3)
               {
                  soundname = splitSpace[splitSpace.Length - 3];
               }

               soundname = soundname.Trim();               
               soundname = soundname.ToLower();

               // Add to list
               if (!string.IsNullOrEmpty(soundname))
               {
                  mSoundNameList.Add(soundname);
               }
            }
         }

      }
      void LoadUI()
      {
         SoundsToImport.SelectionMode = SelectionMode.MultiExtended;
         SoundsToChoose.SelectionMode = SelectionMode.MultiExtended;

         SoundsToChoose.Items.Clear();
         ExistingSoundsObjectsListBox.Items.Clear();

         foreach (string s in mSoundNameList)
         {
            string playSound = s;
            if (playSound.StartsWith("play") == false)
            {
               continue;
            }
            if (mExistingSoundObjects.Contains(s))
            {
               continue;
            }
            string stopSound = playSound.Replace("play", "stop");
            if (mSoundNameList.Contains(stopSound) == false)
            {
               //playSound = "--ERROR: missing stop--" + playSound;
               continue;
            }
            this.SoundsToChoose.Items.Add(playSound);
            
         }

         foreach (string objectName in mExistingSoundObjects)
         {
            this.ExistingSoundsObjectsListBox.Items.Add(soundPrefix + objectName);
         }

         this.SoundsToImport.Items.Clear();

      }
      string soundPrefix = "snd_";
      Dictionary<string, string> mSoundsToAdd = new Dictionary<string, string>();

      private void RemoveSelectedButton_Click(object sender, EventArgs e)
      {
         foreach (object o in SoundsToImport.SelectedItems)
         {
            mSoundsToAdd.Remove(o.ToString());
         }

         SetSoundsToImport();
         SetButtonStatus();
      }

      private void AddSelectedButton_Click(object sender, EventArgs e)
      {
         foreach(object o in SoundsToChoose.SelectedItems)
         {
            string playSound = o.ToString();
            if (playSound.StartsWith("play") == false)
            {
               continue;
            }
            if (mExistingSoundObjects.Contains(playSound))
            {
               continue;
            }
            string stopSound = playSound.Replace("play", "stop");
            if(mSoundNameList.Contains(stopSound) == false)
            {
               continue;
            }
            mSoundsToAdd[playSound] = stopSound;
         }

         SetSoundsToImport();
         SetButtonStatus();
      }

      void SetSoundsToImport()
      {
         this.SoundsToImport.Items.Clear();
         Dictionary<string, string>.Enumerator it = mSoundsToAdd.GetEnumerator();
         while (it.MoveNext())
         {
            SoundsToImport.Items.Add(it.Current.Key);
         }
      }

      bool UsingPerforce
      {
         get
         {
            return true;
         }
      }
      bool DoCheckin
      {
         get
         {
            return true;
         }
      }
      bool Revert
      {
         get
         {
            return false;
         }
      }

      private void ImportButton_Click(object sender, EventArgs e)
      {
         SimpleFileStatus status =  CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(CoreGlobals.getWorkPaths().GetProtoObjectFile());
         if(status.CheckedOutThisUser == true)
         {
            MessageBox.Show("Please check in objects.xml and try again");            
            return;
         }

         if (this.UsingPerforce)
         {
            mChangeList = CoreGlobals.getPerforce().GetNewChangeList("Batch Sound Objects Import");

            if (mChangeList == null)
            {
               MessageBox.Show("Can't get new change list!");
               return;
            }
         }
         
         //Sync
         if (this.UsingPerforce)
         {
            if (mChangeList != null)
            {
               if (mChangeList.SyncFile(CoreGlobals.getWorkPaths().GetProtoObjectFile()) == false)
               {
                  MessageBox.Show("Perforce error: " + mChangeList.GetLastError());
                  return;
               }
               loadProtoXML();
            }
            
         }

         //Start Edit
         if (this.UsingPerforce)
         {
            if(mChangeList.EditFile(CoreGlobals.getWorkPaths().GetProtoObjectFile()) == false)
            {
               MessageBox.Show("Perforce error: " + mChangeList.GetLastError());
               return;
            }

            loadProtoXML();

         }

         bool bSoundBehindFOW = SoundBehindFOWCheckBox.Checked;

         //Add Objects
         XmlNode protoNode = mProtoObjectsDoc.GetElementsByTagName("Objects")[0];

         Dictionary<string, string>.Enumerator it = mSoundsToAdd.GetEnumerator();
         while (it.MoveNext())
         {
            mHightestID = mHightestID + 1;
            XmlNode toImport = CreateSoundObject(mHightestID, it.Current.Key, it.Current.Value, bSoundBehindFOW);
            if (toImport != null)
            {
               protoNode.AppendChild(toImport);
            }

         }
         mSoundsToAdd.Clear();
         try
         {
            mProtoObjectsDoc.Save(CoreGlobals.getWorkPaths().GetProtoObjectFile());
            if (this.UsingPerforce)
            {
               mChangeList.AddOrEdit(CoreGlobals.getWorkPaths().GetProtoObjectFile() + ".xmb");
            }
            DBIDHelperTool.Run();
            EditorCore.XMBProcessor.CreateXMB(CoreGlobals.getWorkPaths().GetProtoObjectFile(), false);

         }
         catch (UnauthorizedAccessException unEx)
         {
            MessageBox.Show("Error writing to objects.xml");
         }
         //Checkin
         if (DoCheckin && this.UsingPerforce)
         {   
            Checkin();            
         }
         //or revert?
         else if (Revert && this.UsingPerforce)
         {
            mChangeList.Revert();
         }

         //refresh:
         loadProtoXML();
         LoadSoundNames();
         LoadUI();
         SetButtonStatus();
      }
      /*
      <Object name="template_sound_object" id="0">
         <Sound Type="Exist">play_fx_proj_artillery_01</Sound>
         <Sound Type="StopExist">stop_fx_proj_artillery_01</Sound>
      </Object>
      */
      public XmlNode CreateSoundObject(int id, string existSound, string stopExistSound, bool behindFOW)
      {
         XmlNode unit = null;
 
         unit = mSoundTemplate.Clone();

         XmlAttribute nameAttr = (XmlAttribute)unit.Attributes.GetNamedItem("name");
         nameAttr.Value = soundPrefix + existSound;
         //((XmlElement)unit).SetAttribute("id", id.ToString());
         ((XmlElement)unit).SetAttribute("dbid", ""); //this will be updated by the id tool

         XmlNodeList list = unit.SelectNodes("//Sound[@Type='Exist']");
         for (int i = 0; i < list.Count; i++)
         {
            list.Item(i).InnerText = existSound;
         }
         list = unit.SelectNodes("//Sound[@Type='StopExist']");
         for (int i = 0; i < list.Count; i++)
         {
            list.Item(i).InnerText = stopExistSound;
         }
         if (behindFOW == true)
         {
            //Do we have FOW flag already?
            if (unit.InnerText.Contains("SoundBehindFOW") == false)
            {
               //XmlNodeList list2 = unit.SelectNodes("//Flag[Text='SoundBehindFOW']");
               //<Flag>SoundBehindFOW</Flag>

               XmlElement fowFlag = mSoundTemplate.OwnerDocument.CreateElement("Flag");
               fowFlag.InnerXml = "SoundBehindFOW";
               unit.AppendChild(fowFlag);
            }
         }

         return unit;
      }

      public void Checkin()
      {
         System.Threading.Thread.Sleep(2000);
         mChangeList.Submitchanges();   
      }

      XmlNode mSoundTemplate = null;
      XmlDocument mProtoObjectsDoc = null;
      public int mHightestID = 0;
      Dictionary<string, bool> mObjectNames = new Dictionary<string, bool>();
      PerforceChangeList mChangeList = null;
      List<string> mExistingSoundObjects = new List<string>();
      List<string> mDuplicates = new List<string>();
      public void loadProtoXML()
      {
         string fileName = CoreGlobals.getWorkPaths().GetProtoObjectFile();
         mProtoObjectsDoc = new XmlDocument();
         mProtoObjectsDoc.PreserveWhitespace = true;
         mProtoObjectsDoc.Load(fileName);

         mObjectNames.Clear();
         mExistingSoundObjects.Clear();

         XmlNodeList units = mProtoObjectsDoc.GetElementsByTagName("Object");
         foreach (XmlNode unit in units)
         {
            XmlAttribute nameAttr = (XmlAttribute)unit.Attributes.GetNamedItem("name");
            if (nameAttr == null)
               continue;
            if (mObjectNames.ContainsKey(nameAttr.Value))
            {
               mDuplicates.Add(nameAttr.Value);
            }

            mObjectNames[nameAttr.Value] = true;

            //XmlAttribute idAttr = (XmlAttribute)unit.Attributes.GetNamedItem("id");
            //if (idAttr != null)
            //{
            //   int id = System.Convert.ToInt32((string)idAttr.Value);
            //   if (id > mHightestID)
            //   {
            //      mHightestID = id;
            //   }
            //}

            if (nameAttr.Value.Contains("template_sound_object"))
            {
               mSoundTemplate = unit;
            }

            if (!nameAttr.Value.StartsWith(soundPrefix))
            {
               continue;
            }


            mExistingSoundObjects.Add(nameAttr.Value.Replace(soundPrefix, ""));
         }
       
         if (mDuplicates.Count > 0)
         {
            StringWriter sw = new StringWriter();
            sw.WriteLine("Duplicate Objects:");
            foreach (string s in mDuplicates)
            {
               sw.WriteLine(s);
            }
            CoreGlobals.ShowMessage(sw.ToString());
         }
         
      }

      private void SoundObjectImporter_Load(object sender, EventArgs e)
      {

      }

   }
}