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
using PhoenixEditor.ScenarioEditor;
using SimEditor;

namespace PhoenixEditor
{
   public partial class TriggerDataDefinitionPage : EditorCore.BaseClientPage
   {
      public TriggerDataDefinitionPage()
      {
         InitializeComponent();

         TriggerSystemMain.Init();

         mTriggerDefinitionFilename = CoreGlobals.getWorkPaths().mEditorSettings + "\\triggerDescription.xml";
         mTriggerDefinitionDBIDs = CoreGlobals.getWorkPaths().mEditorSettings + "\\triggerDBIDs.xml";

         ConditionsListBox.SelectedIndexChanged += new EventHandler(EffectsConditionsListBox_SelectedIndexChanged);
         EffectsListBox.SelectedIndexChanged += new EventHandler(EffectsListBox_SelectedIndexChanged);
         VersionListBox.SelectedIndexChanged += new EventHandler(VersionListBox_SelectedIndexChanged);


         ItemPropertyGrid.AddMetaDataForProp("ConditionDefinition", "Documentation", "Multiline", true);
         ItemPropertyGrid.AddMetaDataForProp("EffectDefinition", "Documentation", "Multiline", true);

         ItemPropertyGrid.IgnoreProperties("ConditionDefinition", new string[] { "Type", "InParameterDefinitions", "OutParameterDefinitions", "ParameterConversionOverrides" });
         ItemPropertyGrid.IgnoreProperties("EffectDefinition", new string[] { "Type", "InParameterDefinitions", "OutParameterDefinitions", "ParameterConversionOverrides" });
         ItemPropertyGrid.AddMetaDataForProps("ConditionDefinition", new string[] { "Version", "DBID", "MaxVarID" }, "ReadOnly", true);
         ItemPropertyGrid.AddMetaDataForProps("EffectDefinition", new string[] { "Version", "DBID", "MaxVarID" }, "ReadOnly", true);
         ItemPropertyGrid.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(On_AnyPropertyChanged);

         InVariblesList.AddMetaDataForProp("InParameterDefintion", "Type", "SimpleEnumeration", TriggerSystemMain.mTriggerDefinitions.GetTypeNames());
         InVariblesList.SetTypeEditor("InParameterDefintion", "Type", typeof(EnumeratedProperty));
         InVariblesList.AddMetaDataForProp("InParameterDefintion", "SigID", "ReadOnly", true);
         InVariblesList.AddMetaDataForProp("InParameterDefintion", "Type", "UpdateEvent", true);
         InVariblesList.SelectedObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(InVariblesList_SelectedObjectPropertyChanged);
         InVariblesList.NewObjectAdded += new BasicTypedSuperList.ObjectChanged(InVariblesList_NewObjectAdded);
         InVariblesList.mListDataObjectType = typeof(InParameterDefintion);
         InVariblesList.AutoScroll = true;
         InVariblesList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(On_AnyPropertyChanged);

         OutVariablesList.AddMetaDataForProp("OutParameterDefition", "Type", "SimpleEnumeration", TriggerSystemMain.mTriggerDefinitions.GetTypeNames());
         OutVariablesList.SetTypeEditor("OutParameterDefition", "Type", typeof(EnumeratedProperty));
         OutVariablesList.AddMetaDataForProp("OutParameterDefition", "SigID", "ReadOnly", true);
         OutVariablesList.AddMetaDataForProp("OutParameterDefition", "Type", "UpdateEvent", true);
         OutVariablesList.SelectedObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(OutVariablesList_SelectedObjectPropertyChanged);
         OutVariablesList.NewObjectAdded += new BasicTypedSuperList.ObjectChanged(OutVariablesList_NewObjectAdded);
         OutVariablesList.mListDataObjectType = typeof(OutParameterDefition);
         OutVariablesList.AutoScroll = true;
         OutVariablesList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(On_AnyPropertyChanged);

         ConversionList.AutoScroll = true;
         ConversionList.AddMetaDataForProp("ParameterConversionOverride", "OldParameter", "StringIntEnumeration", new Pair<List<int>, List<string>>());
         ConversionList.AddMetaDataForProp("ParameterConversionOverride", "NewParameter", "StringIntEnumeration", new Pair<List<int>, List<string>>());
         ConversionList.SetTypeEditor("ParameterConversionOverride", "OldParameter", typeof(EnumeratedProperty));
         ConversionList.SetTypeEditor("ParameterConversionOverride", "NewParameter", typeof(EnumeratedProperty));
         ConversionList.mListDataObjectType = typeof(ParameterConversionOverride);


         LoadData();
         LoadUI();

         UpdateVersionsButton.Visible = false ;

         SaveToNewVersionButton.Enabled = false;
         SaveToSelectedButton.Enabled = false;

         this.SaveButton.Visible = false;

         this.SaveNewNameButton.Click += new EventHandler(SaveNewNameButton_Click);
         NameTextBox.TextChanged+=new EventHandler(NameTextBox_TextChanged);
         SaveNewNameButton.Enabled = false;



         //add in the template editor
         //TemplateVersionControl tvc = new TemplateVersionControl();
         //tabPage2.Controls.Add(tvc);
         //tvc.Dock = DockStyle.Fill;
      }
      string mTriggerDefinitionDBIDs;

      void NameTextBox_TextChanged(object sender, EventArgs e)
      {
         SaveNewNameButton.Enabled = true;
      }



      void On_AnyPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         SaveToSelectedButton.Enabled = true;

         if (GetCurrentDefinition().Version == mTopVersion[GetCurrentDefinition().Type])
         {
            SaveToNewVersionButton.Enabled = true;
         }
      }



      void OutVariablesList_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      {

         TriggerComponentDefinition currentDef = GetCurrentDefinition();
         currentDef.MaxVarID++;
         ParameterDefintion paramDef = selectedObject as ParameterDefintion;
         paramDef.SigID = currentDef.MaxVarID;  
         sender.UpdateData();
      }

      void InVariblesList_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      {
  
         TriggerComponentDefinition currentDef = GetCurrentDefinition();
         currentDef.MaxVarID++;
         ParameterDefintion paramDef = selectedObject as ParameterDefintion;
         paramDef.SigID = currentDef.MaxVarID;
         sender.UpdateData();

      }

      public TriggerComponentDefinition GetCurrentDefinition()
      {
         if (mTempCondDef != null)
            return mTempCondDef;
         if (mTempEffectDef != null)
            return mTempEffectDef;
         return null;

      }

      void OutVariablesList_SelectedObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         OutParameterDefition def = selectedObject as OutParameterDefition;
         if (prop.Name == "Type" && def != null)
         {
            def.Name = prop.PresentationValue.ToString();
            sender.UpdateData();
         }

      }

      void InVariblesList_SelectedObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         InParameterDefintion def = selectedObject as InParameterDefintion;
         if (prop.Name == "Type" && def != null)
         {
            def.Name = prop.PresentationValue.ToString();
            sender.UpdateData();
         }

      }

      string mTriggerDefinitionFilename;
      TriggerDefinition mTriggerDefinition = null;

      public void LoadData()
      {
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(TriggerDefinition), new Type[] { });
            Stream st = File.OpenRead(mTriggerDefinitionFilename);
            mTriggerDefinition = (TriggerDefinition)s.Deserialize(st);
            st.Close();


         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }
      public void SaveData()
      {
         try
         {

            XmlSerializer s = new XmlSerializer(typeof(TriggerDefinition), new Type[] { });
            Stream st = File.Open(mTriggerDefinitionFilename, FileMode.Create);
            s.Serialize(st, mTriggerDefinition);
            st.Close();

            //Not used by game, no xmb needed

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }


      Dictionary<string, List<object>> mEntries = new Dictionary<string, List<object>>();
      Dictionary<string, int> mTopVersion = new Dictionary<string, int>();


      public void LoadUI()
      {
         mEntries.Clear();
         ConditionsListBox.Items.Clear();
         EffectsListBox.Items.Clear();
         mTopVersion.Clear();
         bool bHideObsolete = HideObsoleteCheckBox.Checked;
         foreach(ConditionDefinition item in mTriggerDefinition.ConditionDefinitions)
         {

            string key = item.Type;
            if (mEntries.ContainsKey(key) == false)
            {
               mEntries[key] = new List<object>();

               //ConditionsListBox.Items.Add(key);
            }
            mEntries[key].Add(item);

            if(mTopVersion.ContainsKey(key) == false)
            {
               mTopVersion[key] = item.Version;
            }
            if(item.Version > mTopVersion[key])
            {
               mTopVersion[key] = item.Version;
            }
         }
         foreach (EffectDefinition item in mTriggerDefinition.EffectDefinitions)
         {

            string key = item.Type;
            if (mEntries.ContainsKey(key) == false)
            {
               mEntries[key] = new List<object>();

               //EffectsListBox.Items.Add(key);

            }
            mEntries[key].Add(item);

            if (mTopVersion.ContainsKey(key) == false)
            {
               mTopVersion[key] = item.Version;
            }
            if (item.Version > mTopVersion[key])
            {
               mTopVersion[key] = item.Version;
            }
         }
         Dictionary<string, int>.Enumerator it = mTopVersion.GetEnumerator();
         while(it.MoveNext())
         {
            

            foreach(object entry in  mEntries[it.Current.Key] )
            {
               if(entry is ConditionDefinition)
               {
                  ConditionDefinition def = entry as ConditionDefinition;
                  if(def.Version == it.Current.Value)
                  {
                     if (bHideObsolete && def.Obsolete == true)
                        continue;
                     else
                        ConditionsListBox.Items.Add(it.Current.Key);
                  }
               }
               else if(entry is EffectDefinition)
               {
                  EffectDefinition def = entry as EffectDefinition;
                  if (def.Version == it.Current.Value)
                  {
                     if (bHideObsolete && def.Obsolete == true)
                        continue;
                     else
                        EffectsListBox.Items.Add(it.Current.Key);
                  }
               }
            }
            

         }
      


         ConditionsListBox.Sorted = true;
         EffectsListBox.Sorted = true;

  
      }
      void EffectsConditionsListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         string key = ConditionsListBox.SelectedItem as string;

         NameTextBox.Text = key;
         SaveNewNameButton.Enabled = false;

         if(key != null)
         {
            VersionListBox.Items.Clear();
            foreach (object o in mEntries[key])
            {
               //VersionListBox.Items.Add(o);

               VersionListBox.Items.Insert(0,o);
            }

            VersionListBox.SelectedIndex = 0;
         }

      }
      void EffectsListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         string key = EffectsListBox.SelectedItem as string;

         NameTextBox.Text = key;
         SaveNewNameButton.Enabled = false;

         if (key != null)
         {
            VersionListBox.Items.Clear();
            foreach (object o in mEntries[key])
            {
               //VersionListBox.Items.Add(o);

               VersionListBox.Items.Insert(0, o);

            }

            VersionListBox.SelectedIndex = 0;
         }
      }
      ConditionDefinition mTempCondDef = null;
      EffectDefinition mTempEffectDef = null;

      ConditionDefinition mCurrentCondDef = null;
      EffectDefinition mCurrentEffectDef = null;

      void VersionListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         SaveToNewVersionButton.Enabled = false;
         SaveToSelectedButton.Enabled = false;

         mTempCondDef = null;
         mTempEffectDef = null;


         ConditionDefinition condDef = VersionListBox.SelectedItem as ConditionDefinition;
         EffectDefinition effDef = VersionListBox.SelectedItem as EffectDefinition;

         mCurrentCondDef = condDef;
         mCurrentEffectDef = effDef;

         TriggerComponentDefinition compDef = VersionListBox.SelectedItem as TriggerComponentDefinition;


         if(condDef != null)
         {
            mTempCondDef = new ConditionDefinition();
            condDef.CopyTo(mTempCondDef);

            ItemPropertyGrid.SelectedObject = mTempCondDef;
            InVariblesList.ObjectList = mTempCondDef.InParameterDefinitions;
            OutVariablesList.ObjectList = mTempCondDef.OutParameterDefinitions;
            ConversionList.ObjectList = mTempCondDef.ParameterConversionOverrides;

         }
         if(effDef != null)
         {
            mTempEffectDef = new EffectDefinition();
            effDef.CopyTo(mTempEffectDef);

            ItemPropertyGrid.SelectedObject = mTempEffectDef;
            InVariblesList.ObjectList = mTempEffectDef.InParameterDefinitions;
            OutVariablesList.ObjectList = mTempEffectDef.OutParameterDefinitions;
            ConversionList.ObjectList = mTempEffectDef.ParameterConversionOverrides;

         }
         if (compDef != null)
         {

            int version = compDef.Version;
            if (version > 1)
            {
               TriggerComponentDefinition lastversion = null;
               foreach (object o in mEntries[compDef.Type])
               {
                  TriggerComponentDefinition oldCompDef = o as TriggerComponentDefinition;
                  if (oldCompDef != null)
                  {
                     if(oldCompDef.Version == version - 1)
                     {
                        lastversion = oldCompDef;
                        break;
                     }
                  }
               }

               ConversionList.AddMetaDataForProp("ParameterConversionOverride", "OldParameter", "StringIntEnumeration", GetParameterConversionInfo(lastversion));
               ConversionList.AddMetaDataForProp("ParameterConversionOverride", "NewParameter", "StringIntEnumeration", GetParameterConversionInfo(compDef));
               ConversionList.ObjectList = compDef.ParameterConversionOverrides;
            }




         }
      
      }
      Pair<List<int>, List<string>> GetParameterConversionInfo(TriggerComponentDefinition def)
      {
         Pair<List<int>, List<string>> entries = new Pair<List<int>, List<string>>();
         entries.Key = new List<int>();
         entries.Value = new List<string>();
         foreach(ParameterDefintion p in def.InParameterDefinitions)
         {
            entries.Key.Add(p.SigID);
            entries.Value.Add(String.Format("IN {0} {1} {2}", p.SigID, p.Name, p.Type));
         }
         foreach (ParameterDefintion p in def.OutParameterDefinitions)
         {
            entries.Key.Add(p.SigID);
            entries.Value.Add(String.Format("OUT {0} {1} {2}", p.SigID, p.Name,  p.Type));
         }
         return entries;
      }

      private void UpdateVersionsButton_Click(object sender, EventArgs e)
      {
         UpdateVersionInfo();
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         SaveData();
      }


      public void UpdateVersionInfo()
      {
         //if (mTriggerDefinition.MaxID == -1)
         //   mTriggerDefinition.MaxID = 0;

         foreach (TriggerComponentDefinition item in mTriggerDefinition.ConditionDefinitions)
         {     
            item.DBID = GetNewDBID();

            if (item.Version == -1)
               item.Version = 1;
            UpdateParameterVersionInfo(item);
         }
         foreach (TriggerComponentDefinition item in mTriggerDefinition.EffectDefinitions)
         {
            item.DBID = GetNewDBID();

            if (item.Version == -1)
               item.Version = 1;
            UpdateParameterVersionInfo(item);
         }
      }
      public void UpdateParameterVersionInfo(TriggerComponentDefinition item)
      {
         // Note:  This function used to be a one-time converting function for when we moved to having variable ID's and sig ID's.
         //BASSERT(item.MaxVarID >= 0);
         //if (item.MaxVarID == -1)
         //   item.MaxVarID = 0;
         //if (item.MaxVarID == -1)
         //   item.MaxVarID = 0;
         //foreach(ParameterDefintion def in item.InParameterDefinitions)
         //{
         //   if(def.SigID == -1)
         //   {
         //      item.MaxVarID++;
         //      def.SigID = item.MaxVarID;
         //   }
         //}
         //foreach (ParameterDefintion def in item.OutParameterDefinitions)
         //{
         //   if (def.SigID == -1)
         //   {
         //      item.MaxVarID++;
         //      def.SigID = item.MaxVarID;
         //   }
         //}
      }

      public int GetNewDBID()
      {
         //mTriggerDefinition.MaxID++;
         //return mTriggerDefinition.MaxID;

         int newDBID;
         if (DBIDsXML.GetNewDBID(mTriggerDefinitionDBIDs, out newDBID) == false)
         {
            throw new System.Exception("error getting new dbid from " + mTriggerDefinitionDBIDs);
         }
         return newDBID;
      }

      private void AddConditionButton_Click(object sender, EventArgs e)
      {
         ConditionDefinition newDef = new ConditionDefinition();
         newDef.DBID = GetNewDBID();
         newDef.Type = "NewCondition";
         mTriggerDefinition.ConditionDefinitions.Add(newDef);
         RefreshUI();
         ConditionsListBox.SelectedItem = "NewCondition";
      }

      private void AddEffectButton_Click(object sender, EventArgs e)
      {
         EffectDefinition newDef = new EffectDefinition();
         newDef.DBID = GetNewDBID();
         newDef.Type = "NewEffect";
         mTriggerDefinition.EffectDefinitions.Add(newDef);
         RefreshUI();
         EffectsListBox.SelectedItem = "NewEffect";
      }

      private void SaveToNewVersionButton_Click(object sender, EventArgs e)
      {
         int selected = VersionListBox.SelectedIndex;

         if (mTempCondDef != null)
         {
            mTempCondDef.Version++;
            mTriggerDefinition.ConditionDefinitions.Add(mTempCondDef);
            SaveData();
            RefreshUI();
            //ConditionsListBox.SelectedItem = 0;
            ConditionsListBox.SelectedItem = mCurrentCondDef.Type;

         }
         if (mTempEffectDef != null)
         {
            mTempEffectDef.Version++;
            mTriggerDefinition.EffectDefinitions.Add(mTempEffectDef);
            SaveData();
            RefreshUI();
            //EffectsListBox.SelectedItem = 0;
            EffectsListBox.SelectedItem = mTempEffectDef.Type;

         }

         VersionListBox.SelectedIndex = selected;

      }

      private void SaveToSelectedButton_Click(object sender, EventArgs e)
      {
         int selected = VersionListBox.SelectedIndex;
         
         if (mTempCondDef != null)
         {
            mTempCondDef.CopyTo(mCurrentCondDef);
            SaveData();
            RefreshUI();
            ConditionsListBox.SelectedItem = mCurrentCondDef.Type;
         }
         if (mTempEffectDef != null)
         {
            mTempEffectDef.CopyTo(mCurrentEffectDef);
            SaveData();
            RefreshUI();
            EffectsListBox.SelectedItem = mCurrentEffectDef.Type;
         }
         VersionListBox.SelectedIndex = selected;


      }
      void SaveNewNameButton_Click(object sender, EventArgs e)
      {
         string newName = this.NameTextBox.Text;
         if (mTempCondDef != null)
         {
            int idToRename = mTempCondDef.DBID;

            foreach (TriggerComponentDefinition item in mTriggerDefinition.ConditionDefinitions)
            {
               if(item.DBID == idToRename)
               {
                  item.Type = newName;
               }

            }
            SaveData();
            RefreshUI();
            ConditionsListBox.SelectedItem = newName;
         }
         if (mTempEffectDef != null)
         {
            int idToRename = mTempEffectDef.DBID;

            foreach (TriggerComponentDefinition item in mTriggerDefinition.EffectDefinitions)
            {
               if (item.DBID == idToRename)
               {
                  item.Type = newName;
               }
            }
            SaveData();
            RefreshUI();
            EffectsListBox.SelectedItem = newName;
         }


      }

      public void RefreshUI()
      {
         LoadUI();
      }

      private void ScanFillesButton_Click(object sender, EventArgs e)
      {
         ScanFiles(false,null);
      }


      class ScanResults
      {
         public ScanResults(string fileName, string fileType, TriggerSystemDebugInfo info)
         {
            mFileName = fileName;
            mFileType = fileType;
            mInfo = info;   
         }
         string mFileName;
         string mFileType;
         TriggerSystemDebugInfo mInfo;

         public string Type
         {
            get
            {
               return mInfo.mType.ToString();
            }
         }
         public string FileName
         {
            get
            {
               return mFileName;
            }
         }
         public string FileType
         {
            get
            {
               return mFileType;
            }
         }
         public string Component
         {
            get
            {
               return mInfo.GetComponentString();
            }
         }
         public string Text
         {
            get
            {
               
               return mInfo.mErrorText;
            }
         }
         public string Level
         {
            get
            {
               return mInfo.mLevel.ToString();
            }
         }
         //public string Component
         //{


         //}

      }
      static List<ScanResults> mScanResults = new List<ScanResults>();

      List<string> SplitList(List<string> list, int part, int total)
      {
         List<string> output = new List<string>();
         int size = list.Count / total;
         int start = size * (part );
         int end = size * (part + 1);
         if (part == total - 1)
         {
            end = list.Count-1;
         }
         for (int i = start; i < end; i++)
         {
            output.Add(list[i]);
         }
         return output;
      }

      private void ScanFiles(bool bfilter, List<string> filterList)
      {
         DateTime start = DateTime.Now;

         ImageList l = SharedResources.GetImageList(new string[] { "Error.bmp", "Warning.bmp" });

         Cursor last = Cursor.Current;
         Cursor.Current = Cursors.WaitCursor;

         //ScanResultsTreeView.Nodes.Clear();

         mScanResults.Clear();

         TriggerSystemMain.mTriggerDefinitions.ReloadData();

         int numThreads = 4;

         List<string> scenarioFiles = new List<string>();
         scenarioFiles.AddRange( CoreGlobals.getWorkPaths().GetScenarioFiles() );

         List<string> templates = new List<string>();
         templates.AddRange( TriggerSystemMain.mTriggerDefinitions.GetTemplateFileNames());

         List<string> scripts = new List<string>();
         //scripts.AddRange(TriggerSystemMain.mTriggerDefinitions.GetPowerFileNames());
         //scripts.AddRange(TriggerSystemMain.mTriggerDefinitions.GetAbilityFileNames());
         scripts.AddRange(TriggerSystemMain.mTriggerDefinitions.GetGeneralScriptFileNames());


         for (int i = 0; i < numThreads; i++)
         {

            mRemainingTasks++;
            ScanSettings settings = new ScanSettings();
            settings.bFilter = bfilter;
            settings.filterList = filterList;

            settings.scenarios = SplitList(scenarioFiles, i, numThreads);
            settings.templates = SplitList(templates, i, numThreads);
            settings.scripts = SplitList(scripts, i, numThreads);


            //settings.resultsList = 
            System.Threading.Thread t = new System.Threading.Thread(new System.Threading.ParameterizedThreadStart(InternalScan));
            t.Start(settings);

            //t.Start();

         }

         while (mRemainingTasks > 0)
         {
            System.Threading.Thread.Sleep(1000);
         }


         //mScanResults.AddRange(scanResults);
         this.gridControl1.DataSource = mScanResults;
         Cursor.Current = last;



         MessageBox.Show(((TimeSpan)(DateTime.Now - start)).TotalSeconds.ToString());

         //return mScanResults;
      }

      static int mRemainingTasks = 0;

      class ScanSettings
      {
         public bool bFilter;
         public List<string> filterList;

         public List<string> scripts;
         public List<string> templates;  // TriggerSystemMain.mTriggerDefinitions.GetTemplateFileNames();
         public List<string> scenarios;

      }

      private static void InternalScan(object input)//bool bfilter, List<string> filterList)
      {
         ScanSettings settings = input as ScanSettings;
         bool bfilter = settings.bFilter;
         List<string> filterList = settings.filterList;

         //TreeNode templateNode = new TreeNode("Scanning Templates");
         List<ScanResults> scanResults = mScanResults;// new List<ScanResults>();

         List<string> templates = settings.templates;// TriggerSystemMain.mTriggerDefinitions.GetTemplateFileNames();

         foreach (string filename in templates)
         {
            if (bfilter == true && filterList.Contains(filename.ToLower()) == false)
               continue;
            try
            {
               if (filename.Contains("customswatches.xml") == true)
                  continue;

               XmlSerializer s = new XmlSerializer(typeof(TriggerTemplateDefinition), new Type[] { });
               Stream st = File.OpenRead(filename);
               TriggerTemplateDefinition def = (TriggerTemplateDefinition)s.Deserialize(st);
               st.Close();


               TriggerNamespace ns = new TriggerNamespace();
               //ns.TriggerData = def.TriggerSystemRoot;
               //TriggerRoot output = ns.TriggerData;

               ns.mDebugInfo.Clear();
               ns.PreProcessObjectGraph(def.TriggerSystemRoot, false, def.TriggerTemplateMapping.GetActivatedTriggers(), def.TriggerTemplateMapping.GetInitializedVars());

               if (ns.mDebugInfo.Count > 0)
               {
                  lock (scanResults)
                  {
                     //TreeNode thisFile = new TreeNode();
                     foreach (TriggerSystemDebugInfo d in ns.mDebugInfo)
                     {
                        //TreeNode info = new TreeNode(d.ToString());
                        //info.Tag = d;
                        //SetNodeProps(info, d);
                        //thisFile.Nodes.Add(info);
     
                        scanResults.Add(new ScanResults(filename, "Template", d));
                     }
                  }
                     //thisFile.Text = filename;
                     //templateNode.Nodes.Add(thisFile);
               
               }

            }
            catch (System.Exception ex)
            {
               //templateNode.Nodes.Add("Fatal error loading " + filename + " " + ex.ToString());
            }
         }
         //this.ScanResultsTreeView.Nodes.Add(templateNode);
         //this.ScanResultsTreeView.Refresh();



         //TreeNode powerNode = new TreeNode("Scanning Powers/Abilites/Triggerscripts");

         List<string> scripts = settings.scripts;// new List<string>();
         //scripts.AddRange(TriggerSystemMain.mTriggerDefinitions.GetPowerFileNames());
         //scripts.AddRange(TriggerSystemMain.mTriggerDefinitions.GetAbilityFileNames());
         //scripts.AddRange(TriggerSystemMain.mTriggerDefinitions.GetGeneralScriptFileNames());

         foreach (string filename in scripts)
         {
            if (bfilter == true && filterList.Contains(filename.ToLower()) == false)
               continue;
            try
            {
               if (filename.Contains("customswatches.xml") == true)
                  continue;

               XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
               Stream st = File.OpenRead(filename);
               TriggerRoot root = (TriggerRoot)s.Deserialize(st);
               st.Close();

               //TriggerNamespace ns = new TriggerNamespace();
               //ns.TriggerData = root;
               ////TriggerRoot output = ns.TriggerData;

               TriggerNamespace ns = new TriggerNamespace();
               ns.mDebugInfo.Clear();
               ns.PreProcessObjectGraph(root, false);

               if (ns.mDebugInfo.Count > 0)
               {
                  //TreeNode thisFile = new TreeNode();
                  lock (scanResults)
                  {                  
                     foreach (TriggerSystemDebugInfo d in ns.mDebugInfo)
                     {

                     //TreeNode info = new TreeNode(d.ToString());
                     //info.Tag = d;
                     //SetNodeProps(info, d);
                     //thisFile.Nodes.Add(info);

                        scanResults.Add(new ScanResults(filename, "AbilityPowerTScript", d));
                     }
                  }
                  //thisFile.Text = filename;
                  //powerNode.Nodes.Add(thisFile);
               }
            }
            catch (System.Exception ex)
            {
               //powerNode.Nodes.Add("Fatal error loading " + filename + " " + ex.ToString());
            }
         }

         //this.ScanResultsTreeView.Nodes.Add(powerNode);
         //this.ScanResultsTreeView.Refresh();

         //TreeNode scenarioNode = new TreeNode("Scanning Scenarios");

         //string[] scenarioFiles = CoreGlobals.getWorkPaths().GetScenarioFiles();
         List<string> scenarioFiles = settings.scenarios;
         foreach (string filename in scenarioFiles)
         {
            if (bfilter == true && filterList.Contains(filename.ToLower()) == false)
               continue;
            try
            {
               if (filename.Contains("customswatches.xml") == true)
                  continue;

               XmlSerializer s = new XmlSerializer(typeof(ScenarioXML), new Type[] { });
               Stream st = File.OpenRead(filename);
               ScenarioXML scenXml = (ScenarioXML)s.Deserialize(st);
               st.Close();

               foreach (TriggerRoot ts in scenXml.mTriggers)
               {

                  //TriggerNamespace ns = new TriggerNamespace();
                  //ns.TriggerData = ts;
                  ////TriggerRoot output = ns.TriggerData;

                  TriggerNamespace ns = new TriggerNamespace();
                  ns.mDebugInfo.Clear();
                  ns.PreProcessObjectGraph(ts, false);

                  if (ns.mDebugInfo.Count > 0)
                  {
                     lock (scanResults)
                     {
                        //TreeNode thisFile = new TreeNode();
                        foreach (TriggerSystemDebugInfo d in ns.mDebugInfo)
                        {

                           //TreeNode info = new TreeNode(d.ToString());
                           //SetNodeProps(info, d);
                           //info.Tag = d;
                           //thisFile.Nodes.Add(info);
                           scanResults.Add(new ScanResults(filename, "Scenario", d));

                        }
                     }
                     //thisFile.Text = filename;
                     //scenarioNode.Nodes.Add(thisFile);
                  }
               }
            }
            catch (System.Exception ex)
            {
               //scenarioNode.Nodes.Add("Fatal error loading " + filename + " " + ex.ToString());
            }
         }

         System.Threading.Interlocked.Decrement(ref mRemainingTasks);
         //this.ScanResultsTreeView.Nodes.Add(scenarioNode);

         //ScanResultsTreeView.ImageList = l;

         //this.ScanResultsTreeView.Refresh();
         //return scanResults;
      }

      private void SetNodeProps(TreeNode info, TriggerSystemDebugInfo d)
      {
         if (d.mLevel == TriggerSystemDebugLevel.Error)
         {
            info.ImageIndex = 0;
            info.SelectedImageIndex = 0;
         }
         else
         {
            info.ImageIndex = 1;
            info.SelectedImageIndex = 1;          
         }
      }

      private void HideObsoleteCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         LoadUI();
      }

      private void ScanFileButton_Click(object sender, EventArgs e)
      {

         OpenFileDialog ofd = new OpenFileDialog();
         ofd.Multiselect = true;
         //ofd.InitialDirectory = 
         if(ofd.ShowDialog() == DialogResult.OK)
         {
            List<string> fileList = new List<string>();
            foreach(string s in ofd.FileNames)
               fileList.Add(s.ToLower());
            ScanFiles(true, fileList);

         }
        
      }

   
   }
}

