using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using EditorCore;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ScenarioScriptsManager : UserControl
   {
      public ScenarioScriptsManager()
      {
         InitializeComponent();

         ExternalScriptsList.AutoScroll = true;
         InternalScriptsList.AutoScroll = true;

         ExternalScriptsList.AddMetaDataForProp("ExternalScriptInfo", "FileName", "FileFilter", "Trigger Scripts (*" + CoreGlobals.getWorkPaths().mScriptExtention + ")|*" + CoreGlobals.getWorkPaths().mScriptExtention);
         ExternalScriptsList.AddMetaDataForProp("ExternalScriptInfo", "FileName", "RootDirectory", CoreGlobals.getWorkPaths().mScriptTriggerDirectory);
         ExternalScriptsList.AddMetaDataForProp("ExternalScriptInfo", "FileName", "StartingDirectory", CoreGlobals.getWorkPaths().mScriptTriggerDirectory);
         ExternalScriptsList.SetTypeEditor("ExternalScriptInfo", "FileName", typeof(FileNameProperty));


         ExternalScriptsList.AddMetaDataForProp("ExternalScriptInfo", "Description", "Multiline", true);
         InternalScriptsList.AddMetaDataForProp("InternalScriptDescription", "Description", "Multiline", true);


         //basicTypedSuperList1.AddMetaDataForProp("ScenarioInfoXml", "Type", "SimpleEnumeration", new string[] { "Playtest", "Development", "Test" });
         //basicTypedSuperList1.SetTypeEditor("ScenarioInfoXml", "Type", typeof(EnumeratedProperty));

         //mExternalDefinitions = SimGlobals.getSimMain().ExternalScripts;
         //mInternalDefinitions = SimGlobals.getSimMain().
         SimGlobals.getSimMain().SimLoaded += new SimMain.SimChanged(ScenarioScriptsManager_SimLoaded);

         ExternalScriptsList.ObjectList = mExternalDefinitions;
         ExternalScriptsList.mListDataObjectType = typeof(ExternalScriptInfo);
         ExternalScriptsList.NewObjectAdded += new BasicTypedSuperList.ObjectChanged(ExternalScriptsList_NewObjectAdded);
         ExternalScriptsList.ObjectDeleted += new BasicTypedSuperList.ObjectChanged(ExternalScriptsList_ObjectDeleted);

         InternalScriptsList.ObjectList = mInternalDefinitions;//new List<InternalScriptDescription>();
         InternalScriptsList.mListDataObjectType = typeof(InternalScriptDescription);

         InternalScriptsList.NewObjectAdded += new BasicTypedSuperList.ObjectChanged(InternalScriptsList_NewObjectAdded);
         InternalScriptsList.ObjectDeleted += new BasicTypedSuperList.ObjectChanged(InternalScriptsList_ObjectDeleted);


         buildModeCombo.Items.AddRange(Enum.GetNames(typeof(TriggerFinalBake.eBuildMode)));
         buildModeCombo.SelectedIndex = 0;


         //hide the bake stuff for now
         bakeGroupBox.Visible = false;
         bakeGroupBox.Enabled = false;

      }

      void ScenarioScriptsManager_SimLoaded(SimMain sender)
      {
         //mExternalDefinitions = SimGlobals.getSimMain().ExternalScripts;

      }

      void ExternalScriptsList_ObjectDeleted(ObjectEditorControl sender, object selectedObject)
      {
         SimGlobals.getSimMain().ExternalScripts = mExternalDefinitions;
      }

      void ExternalScriptsList_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      {
         SimGlobals.getSimMain().ExternalScripts = mExternalDefinitions;
      }



      List<InternalScriptDescription> mInternalDefinitions = new List<InternalScriptDescription>();
      List<ExternalScriptInfo> mExternalDefinitions = new List<ExternalScriptInfo>();


      SimMain mSimMain;

      public SimMain SimMain
      {
         set
         {
            mSimMain = value;
            //LoadScenarioPlayerData();
            //ExternalScriptsList.ObjectList = mSimMain.ExternalScripts;
         }

      }
      public TabControl mParentTabs = null;
      public BaseClientPage mBaseClientParent = null;



      public void AddNewTab( TriggerRoot root)
      {
         InternalScriptDescription newPage = new InternalScriptDescription();

         newPage.Name = root.Name;

         
         mInternalDefinitions.Add(newPage);

         AddNewTab(newPage, root, root.Name);

         
      }
      public void UpdateData()
      {
         //mInternalDefinitions = SimGlobals.getSimMain();
         InternalScriptsList.ObjectList = mInternalDefinitions;//new List<InternalScriptDescription>();

         mExternalDefinitions = SimGlobals.getSimMain().ExternalScripts;
         ExternalScriptsList.ObjectList = mExternalDefinitions;

      }

      public void ClearData()
      {
         mInternalDefinitions = new List<InternalScriptDescription>();
         mExternalDefinitions = new List<ExternalScriptInfo>(); 
         InternalScriptsList.ObjectList = mInternalDefinitions;//new List<InternalScriptDescription>();
         ExternalScriptsList.ObjectList = mExternalDefinitions;   

      }


      public void AddNewTab(InternalScriptDescription newPage, TriggerRoot root, string name)
      {
         if (newPage != null && mParentTabs != null)
         {
            this.SuspendLayout();
            TabPage p = new TabPage();
            mParentTabs.Controls.Add(p);
            TriggerEditor ed = new TriggerEditor();
            p.Controls.Add(ed);
            ed.Dock = DockStyle.Fill;
            ed.mBaseClientParent = this.mBaseClientParent;

            ed.TriggerData = root;

            newPage.mTriggerRoot = root;
            newPage.mOwnedPage = p;

            newPage.Name = name;// "Script";

            p.Tag = ed;
            this.ResumeLayout();
         }
      }
      void InternalScriptsList_ObjectDeleted(ObjectEditorControl sender, object selectedObject)
      {
         //throw new Exception("The method or operation is not implemented.");

         //MessageBox.Show("The method or operation is not implemented.");
         //return;

         PhoenixEditor.ScenarioEditor.InternalScriptDescription des = selectedObject as PhoenixEditor.ScenarioEditor.InternalScriptDescription;
         if (des != null && MessageBox.Show("", "Are you sure you want to delete the script " + des.Name, MessageBoxButtons.YesNo) == DialogResult.Yes)
         {
            if (mParentTabs != null && mSimMain != null)
            {
               Control toremove = null;
               foreach (Control c in mParentTabs.Controls)
               {
                  if (c == des.mOwnedPage)
                     toremove = c;
               }
               if(toremove != null)
               {
                  mParentTabs.Controls.Remove(toremove);
               }
               TriggerRoot rootToRemove = null;
               foreach(TriggerRoot r in mSimMain.TriggerData)
               {
                  if(r == des.mTriggerRoot)
                  {
                     rootToRemove = r;
                  }
               }
               if (rootToRemove != null)
               {
                  mSimMain.TriggerData.Remove(rootToRemove);
               }
            }


         }

         //if (mParentTabs != null)
         //{
         //   foreach(Control c in mParentTabs)
         //   {



         //   }

         //}

      }

      void InternalScriptsList_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      {
         InternalScriptDescription newPage = selectedObject as InternalScriptDescription;

         //if(newPage != null)
         //{
         //   if(mParentTabs != null)
         //   {
               //TabPage p = new TabPage();
               //mParentTabs.Controls.Add(p);
               //TriggerEditor ed = new TriggerEditor();
               //p.Controls.Add(ed);
               //ed.Dock = DockStyle.Fill;
               //ed.mBaseClientParent = this.mBaseClientParent;

               //TriggerRoot newRoot = new TriggerRoot();
               //mSimMain.TriggerData.Add(newRoot);
               //ed.TriggerData = newRoot;

               //newPage.mOwnedPage = p;

               //newPage.Name = "Script";

               //p.Tag = ed;

               TriggerRoot newRoot = new TriggerRoot();
               mSimMain.TriggerData.Add(newRoot);

               AddNewTab(newPage, newRoot, "Script");
               //TabPage p = new TabPage();
               //mParentTabs.Controls.Add(p);
               //TriggerEditor ed = new TriggerEditor();
               //p.Controls.Add(ed);
               //ed.Dock = DockStyle.Fill;
               //ed.mBaseClientParent = this.mBaseClientParent;

 
               //ed.TriggerData = newRoot;

               //newPage.mOwnedPage = p;

               //newPage.Name = "Script";

               //p.Tag = ed;
         //   }

         //}

      }

      private void bakeFinalButton_Click(object sender, EventArgs e)
      {
         //check topic permission
         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission("Sim") == false)
         {
            return;
         }         
         
         TriggerFinalBake.eBuildMode mode = TriggerFinalBake.eBuildMode.NoOptimizations;
         try
         {
            mode = (TriggerFinalBake.eBuildMode)Enum.Parse(typeof(TriggerFinalBake.eBuildMode), buildModeCombo.SelectedItem.ToString());
         }
         catch (System.Exception ex2)
         {
            CoreGlobals.getErrorManager().OnException(ex2);
         }

         //export
         string fileName = Path.Combine(CoreGlobals.ScenarioDirectory, CoreGlobals.ScenarioFile);      
         if (TriggerFinalBake.eBuildMode.NoOptimizations == mode)
         {
            XMBProcessor.CreateXMB(fileName, false);
         }
         else
         {
            string tempFile = TriggerFinalBake.OptimizeScenarioScripts(fileName, mode);
            XMBProcessor.CreateXMB(tempFile, false);
            File.Copy(tempFile + ".xmb", fileName + ".xmb", true);
         }

      }
   }


   public class InternalScriptDescription
   {
      string mName = "Script";
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;

            if(mOwnedPage != null)
            {
               mOwnedPage.Text = mName;

            }
            if(mTriggerRoot != null)
            {
               mTriggerRoot.Name = mName;
               mTriggerRoot.TriggerEditorData.TriggerSystem.Name = mName;
            }

         }
      }


      public TriggerRoot mTriggerRoot = null;
      public TabPage mOwnedPage = null;


   }



}
