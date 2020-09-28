using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using SimEditor;
using EditorCore;
using PhoenixEditor;
using PhoenixEditor.ScenarioEditor;


namespace PhoenixEditor.ClientTabPages
{
   public partial class ScriptPage : EditorCore.BaseClientPage
   {
      public ScriptPage()
      {
         InitializeComponent();

         TriggerSystemMain.Init();

         //this.previewNavWindow1.MasterControl = triggerHostArea1;
         triggerHostArea1.ViewChanged += new EventHandler(triggerHostArea1_ViewChanged);
         previewNavWindow1.BorderStyle = BorderStyle.FixedSingle;

         TriggerData = new TriggerRoot();

         triggerHostArea1.ScanComplete += new EventHandler(triggerHostArea1_ScanComplete);
         ErrorPanel.Click += new EventHandler(ErrorPanel_Click);


         buildMode.Items.AddRange(Enum.GetNames(typeof(TriggerFinalBake.eBuildMode)));
         buildMode.SelectedIndex = 0;
      }

      protected string mSavedFileName = "";

      void ErrorPanel_Click(object sender, EventArgs e)
      {
         triggerHostArea1.Scan(false);
      }

      void triggerHostArea1_ScanComplete(object sender, EventArgs e)
      {
         if(triggerHostArea1.ErrorCount > 0)
         {
            ErrorPanel.BackColor = Color.Red;
         }
         else if(triggerHostArea1.WarningCount > 0)
         {
            ErrorPanel.BackColor = Color.Yellow;
         }
         else
         {
            ErrorPanel.BackColor = Color.Green;
         }
      }

      void triggerHostArea1_ViewChanged(object sender, EventArgs e)
      {
         this.previewNavWindow1.MasterControl = triggerHostArea1.ActiveNodeHostControl;
      }


      TriggerSystemMain mMainTriggerSystem = new TriggerSystemMain();

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerRoot TriggerData
      {
         set
         {
            mMainTriggerSystem.TriggerData = value;

            //load ui
            triggerHostArea1.CurrentTriggerNamespace = mMainTriggerSystem.MainNamespace;

            //triggerValueList1.ParentTriggerNamespace = mMainTriggerSystem.MainNamespace;
            
         }
         get
         {

            return mMainTriggerSystem.TriggerData;
         }
      }

      virtual protected string GetFilter()
      {
         return "Script (*" + CoreGlobals.getWorkPaths().mScriptExtention + ")|*" + CoreGlobals.getWorkPaths().mScriptExtention;
      }
      virtual protected string GetInitialDirectory()
      {
         return CoreGlobals.getWorkPaths().mScriptTriggerDirectory; 
      }


      private void LoadButton_Click(object sender, EventArgs e)
      {
         LoadScript();
      }

      private void LoadScript()
      {
         try
         {
            OpenFileDialog d = new OpenFileDialog();
            d.Filter = GetFilter();
            d.InitialDirectory = GetInitialDirectory();
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
               Stream st = File.OpenRead(d.FileName);
               TriggerData = (TriggerRoot)s.Deserialize(st);
               st.Close();

               this.Parent.Text = Path.GetFileName(d.FileName);

               mSavedFileName = d.FileName;
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      public override void save_file()
      {
         Save();
      }
      public override void save_file_as()
      {
         SaveAs();
      }
      public override void open_file()
      {
         LoadScript();
         //base.open_file();
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         Save();
      }

      private void Save()
      {
         try
         {
            if (mSavedFileName != "")
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
               Stream st = File.Open(mSavedFileName, FileMode.Create);

               TriggerRoot root = TriggerData;
               root.Name = Path.GetFileName(mSavedFileName);
               root.Type = mType;
               s.Serialize(st, root);
               st.Close();

               triggerHostArea1.Scan(true);

               TriggerFinalBake.eBuildMode mode = TriggerFinalBake.eBuildMode.NoOptimizations;

               try
               {
                  mode = (TriggerFinalBake.eBuildMode)Enum.Parse(typeof(TriggerFinalBake.eBuildMode), buildMode.SelectedItem.ToString());
               }
               catch (System.Exception ex2)
               {
                  CoreGlobals.getErrorManager().OnException(ex2);
               }

               //must have _raw_ tag for optimizations
               if (mSavedFileName.Contains(TriggerFinalBake.sTriggerRaw) == false && TriggerFinalBake.eBuildMode.NoOptimizations != mode) 
               {
                  CoreGlobals.ShowMessage("filename not compatible with optimizations.");
                  mode = TriggerFinalBake.eBuildMode.NoOptimizations;
               }

               if (TriggerFinalBake.eBuildMode.NoOptimizations == mode)
               {
                  XMBProcessor.CreateXMB(mSavedFileName, false);
               }
               else
               {
                  string tempFile = TriggerFinalBake.OptimizeTriggerScript(mSavedFileName, mode);
                  XMBProcessor.CreateXMB(tempFile, false);

                  //File.Copy(tempFile + ".xmb", mSavedFileName + ".xmb", true);
               }
            }
            else
            {
               SaveAs();
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      protected void SaveAs()
      {
         try
         {

            SaveFileDialog d = new SaveFileDialog();
            d.Filter = GetFilter();
            d.InitialDirectory = GetInitialDirectory();
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerRoot), new Type[] { });
               Stream st = File.Open(d.FileName, FileMode.Create);
               mSavedFileName = d.FileName;

               TriggerRoot root = TriggerData;
               root.Name = Path.GetFileName(mSavedFileName);
               root.Type = mType;
               s.Serialize(st, root);
               
               //s.Serialize(st, TriggerData);
               st.Close();
               this.Parent.Text = Path.GetFileName(d.FileName);

               triggerHostArea1.Scan(true);


               XMBProcessor.CreateXMB(mSavedFileName, false);

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }

      }

      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         SaveAs();
      }

      protected string mType = "TriggerScript";
   }

   public class PowersPage : ScriptPage
   {
      public PowersPage()
      {
         mType = "Power";
      }
      protected override string GetFilter()
      {
         return "Powers (*" + CoreGlobals.getWorkPaths().mScriptPowerExtention + ")|*" + CoreGlobals.getWorkPaths().mScriptPowerExtention;

      }
      protected override string GetInitialDirectory()
      {
         return CoreGlobals.getWorkPaths().mScriptPowerDirectory; 

      }
   }

   public class AbilityPage : ScriptPage
   {
      public AbilityPage()
      {
         mType = "Ability";
      }
      protected override string GetFilter()
      {
         return "Abilities (*" + CoreGlobals.getWorkPaths().mScriptAbilityExtention + ")|*" + CoreGlobals.getWorkPaths().mScriptAbilityExtention;

      }
      protected override string GetInitialDirectory()
      {
         return CoreGlobals.getWorkPaths().mScriptAbilityDirectory;

      }
   }

}

