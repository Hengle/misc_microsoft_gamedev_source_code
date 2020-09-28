using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Xml.Serialization;

using System.IO;

using EditorCore;
using EditorCore.Controls;
using EditorCore.Controls.Micro;

using PhoenixEditor.ScenarioEditor;
using SimEditor;

namespace PhoenixEditor.ClientTabPages
{
   public partial class HintDatabasePage : EditorCore.BaseClientPage 
   {
      public HintDatabasePage()
      {
         InitializeComponent();

         //load db
         mFilename = CoreGlobals.getWorkPaths().mGameDataDirectory + "\\concepts.xml";
         if (File.Exists(mFilename))
         {
            mDatabase = BaseLoader<ProtoConceptDatabase>.Load(mFilename);
         }

         mConceptEditor = propertyList1;
         //set metadata
         //mConceptEditor.SetTypeEditor("ProtoConcept", "MessageStringID", typeof(LocStringIDProperty));
         //mConceptEditor.AddMetaDataForProps("ProtoConcept", new string[] { "MessageStringID" }, "Compact", true);
         mConceptEditor.SetTypeEditor("ProtoConcept", "Prerequisites", typeof(TriggerPropConceptList2));
         mConceptEditor.AddMetaDataForProp("ProtoConcept", "Prerequisites", "StringIntSource", GetConceptEnumeration());
         mConceptEditor.AddMetaDataForProp("ProtoConcept", "Prerequisites", "AllowRepeats", false);
         mConceptEditor.AddMetaDataForProp("ProtoConcept", "Prerequisites", "AutoSort", true);
         
         mConceptEditor.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(mConceptEditor_AnyPropertyChanged);


         LoadUI();
      }

      void mConceptEditor_AnyPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         if (prop.Name == "Name")
         {
            LoadUI();
         }
      }

      string mFilename;

      ProtoConceptDatabase mDatabase = new ProtoConceptDatabase();
      Dictionary<string, ProtoConcept> mConceptsByName = new Dictionary<string, ProtoConcept>();

      ObjectEditorControl mConceptEditor;

      public void LoadUI()
      {
         mConceptsByName.Clear();
         ConceptsListbox.Items.Clear();
         foreach (ProtoConcept c in mDatabase.mProtoConcepts)
         {
            mConceptsByName[c.Name] = c;
            ConceptsListbox.Items.Add(c.Name.ToString());
         }

         //reload concept properties
         //  uhhh, nevermind.
         //mConceptEditor.AddMetaDataForProp("ProtoConcept", "Prerequisites", "StringIntSource", GetConceptEnumeration());
      }

      public Pair<List<int>, List<string>> GetConceptEnumeration()
      {
         Pair<List<int>, List<string>> entries = new Pair<List<int>, List<string>>();
         entries.Key = new List<int>();
         entries.Value = new List<string>();
         //entries.Value.AddRange(Enum.GetNames(t));
         //entries.Key.AddRange((int[])Enum.GetValues(t));

         foreach (ProtoConcept c in mDatabase.mProtoConcepts)
         {
            entries.Value.Add( c.Name);
            entries.Key.Add(c.ID);
         }

         return entries;
      }


      public void NewProtoConcept()
      {
         //DBIDs check!!

         ProtoConcept c = new ProtoConcept();
         c.Name = "NEW CONCEPT";
         mDatabase.mProtoConcepts.Add(c);

         LoadUI();

         SetSelectedItem(c);
      }
      public void DeleteConcept(ProtoConcept c)
      {
         mDatabase.mProtoConcepts.Remove(c);
         LoadUI();

         SetSelectedItem(null);
      }
      TriggerNamespace mNamespace = new TriggerNamespace();


      public void SetSelectedItemByName(string conceptName)
      {
         ProtoConcept c;
         if (mConceptsByName.TryGetValue(conceptName, out c))
         {
            SetSelectedItem(c);
         }
      }
      public void SetSelectedItem(ProtoConcept concept)
      {
         mConceptEditor.SelectedObject = concept;
      }

      private void NewConceptButton_Click(object sender, EventArgs e)
      {
         NewProtoConcept();
      }

      private void ConceptsListbox_MouseClick(object sender, MouseEventArgs e)
      {
         if (ConceptsListbox.SelectedItem != null)
         {
            SetSelectedItemByName(ConceptsListbox.SelectedItem.ToString());
         }
      }

      private void DeleteConceptButton_Click(object sender, EventArgs e)
      {
         if (ConceptsListbox.SelectedItem != null)
         {
            string conceptName = ConceptsListbox.SelectedItem.ToString();
            ProtoConcept c;
            if (mConceptsByName.TryGetValue(conceptName, out c))
            {
               if (MessageBox.Show("Delete: " + conceptName, "", MessageBoxButtons.OKCancel) == DialogResult.OK)
               {
                  DeleteConcept(c);
               }
            }
         } 
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         string fileName = mFilename;
         if (File.Exists(fileName) == false)
         {
            CoreGlobals.ShowMessage("File Missing: " + fileName);
            return;
         }
         if ((File.GetAttributes(fileName) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
         {
            CoreGlobals.ShowMessage("Please check out: " + fileName);
            return;
         }


         BaseLoader<ProtoConceptDatabase>.Save(mFilename, mDatabase);
         XMBProcessor.CreateXMB(mFilename, false);

      }
   }

   [XmlRoot("ProtoConceptDatabase")]
   public class ProtoConceptDatabase
   {
      [XmlElement("ProtoConcept", typeof(ProtoConcept))]
      public List<ProtoConcept> mProtoConcepts = new List<ProtoConcept>();
   }

   [XmlRoot("ProtoConcept")]
   public class ProtoConcept
   {
      private int mID;
      [XmlAttribute("id")]
      public int ID
      {
         get
         {
            return mID;
         }
         set
         {
            mID = value;
         }
      }

      private string mName;
      [XmlAttribute("name")]
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
         }
      }

      //private int mMessageStringID = 0;
      //[XmlAttribute("MessageStringID")]
      //public int MessageStringID
      //{
      //   get
      //   {
      //      return mMessageStringID;
      //   }
      //   set
      //   {
      //      mMessageStringID = value;
      //   }
      //}




      private float mInitialWaitTime = 30;
      [XmlAttribute("InitialWaitTime")]
      public float InitialWaitTime
      {
         get
         {
            return mInitialWaitTime;
         }
         set
         {
            mInitialWaitTime = value;
         }
      }

      private float mCoolDownTime = 300;
      [XmlElement("CoolDownTime")]
      public float CoolDownTime
      {
         get
         {
            return mCoolDownTime;
         }
         set
         {
            mCoolDownTime = value;
         }
      }

      private float mCoolDownIncrement = 0;
      [XmlElement("CoolDownIncrement")]
      public float CoolDownIncrement
      {
         get
         {
            return mCoolDownIncrement;
         }
         set
         {
            mCoolDownIncrement = value;
         }
      }


      //skirmish and spc?
      private int mTimesPerGame = 2;
      [XmlElement("TimesPerGame")]
      public int TimesPerGame
      {
         get
         {
            return mTimesPerGame;
         }
         set
         {
            mTimesPerGame = value;
         }
      }
      private int mMaxGamesAllowed = 3;
      [XmlElement("MaxGamesAllowed")]
      public int MaxGamesAllowed
      {
         get
         {
            return mMaxGamesAllowed;
         }
         set
         {
            mMaxGamesAllowed = value;
         }
      }

      //private float mMessageDisplayTime = 20;
      //[XmlAttribute("MessageDisplayTime")]
      //public float MessageDisplayTime
      //{
      //   get
      //   {
      //      return mMessageDisplayTime;
      //   }
      //   set
      //   {
      //      mMessageDisplayTime = value;
      //   }
      //}   

      private string mPrerequisites;
      [XmlElement("Prerequisites")]
      public string Prerequisites
      {
         get
         {
            return mPrerequisites;
         }
         set
         {
            mPrerequisites = value;
         }
      }

   }  
}
