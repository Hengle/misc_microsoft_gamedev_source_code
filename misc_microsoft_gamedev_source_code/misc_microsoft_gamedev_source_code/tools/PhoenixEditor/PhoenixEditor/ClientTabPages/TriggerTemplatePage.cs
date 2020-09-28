using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using System.IO;
using System.Xml.Serialization;

using EditorCore;
using PhoenixEditor;
using PhoenixEditor.ScenarioEditor;
using SimEditor;


namespace PhoenixEditor.ClientTabPages
{
   public partial class TriggerTemplatePage : EditorCore.BaseClientPage
   {
      public TriggerTemplatePage()
      {
         InitializeComponent();

         TriggerSystemMain.Init();

         triggerHostArea1.ViewChanged += new EventHandler(triggerHostArea1_ViewChanged);
         previewNavWindow1.BorderStyle = BorderStyle.FixedSingle;

         TemplateAttributesPropertyGrid.IgnoreProperties("TriggerTemplateMapping", new string[]{"InputMappings","OutputMappings","TriggerInputs","TriggerOutputs","GroupID","ID","X","Y","Name","CommentOut"});

         TemplateAttributesPropertyGrid.AddMetaDataForProp("TriggerTemplateMapping", "SizeX", "Min", 30);
         TemplateAttributesPropertyGrid.AddMetaDataForProp( "TriggerTemplateMapping", "SizeX", "Max", 800 );
         TemplateAttributesPropertyGrid.AddMetaDataForProp("TriggerTemplateMapping", "SizeY", "Min", 30);
         TemplateAttributesPropertyGrid.AddMetaDataForProp("TriggerTemplateMapping", "SizeY", "Max", 500);

         TemplateAttributesPropertyGrid.AddMetaDataForProp("TriggerTemplateMapping", "Image", "FileFilter", "Images (*.bmp)|*.bmp");
         TemplateAttributesPropertyGrid.AddMetaDataForProp("TriggerTemplateMapping", "Image", "RootDirectory", CoreGlobals.getWorkPaths().mBaseDirectory);
         TemplateAttributesPropertyGrid.AddMetaDataForProp("TriggerTemplateMapping", "Image", "StartingDirectory", Path.Combine(CoreGlobals.getWorkPaths().mAppIconDir, "TriggerEditor"));
         TemplateAttributesPropertyGrid.SetTypeEditor("TriggerTemplateMapping", "Image", typeof(FileNameProperty));


         TemplateAttributesPropertyGrid.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(onAnyPropertyChanged);

         InputVarsBasicTypedSuperList.UseLabels = false;
         OutputVarsBasicTypedSuperList.UseLabels = false;
         InputTriggersBasicTypedSuperList.UseLabels = false;
         OutputTriggersBasicTypedSuperList.UseLabels = false;

         InputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateInputVariable", "Type", "ReadOnly", true);
         InputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateInputVariable", "ID", "Ignore", true);
         //InputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateInputVariable", "Optional", "Ignore", true);
         InputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateInputVariable", "SigID", "Ignore", true);
         InputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateInputVariable", "BindID", "TriggerNamespace", mMainTriggerSystem.MainNamespace);
         InputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateInputVariable", "BindID", "UpdateEvent", true);
         InputVarsBasicTypedSuperList.SetTypeEditor("TriggersTemplateInputVariable", "BindID", typeof(VariableIDProperty));
         InputVarsBasicTypedSuperList.mListDataObjectType = typeof(TriggersTemplateInputVariable);
         InputVarsBasicTypedSuperList.SelectedObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(basicTypedSuperList1_SelectedObjectPropertyChanged);
         InputVarsBasicTypedSuperList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(onAnyPropertyChanged);
         InputVarsBasicTypedSuperList.NeedsResize += new EventHandler(BasicTypedSuperList_Changed);


         OutputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateOutputVariable", "Type", "ReadOnly", true);
         OutputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateOutputVariable", "ID", "Ignore", true);
         //OutputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateOutputVariable", "Optional", "Ignore", true);
         OutputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateOutputVariable", "SigID", "Ignore", true);
         OutputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateOutputVariable", "BindID", "TriggerNamespace", mMainTriggerSystem.MainNamespace);
         OutputVarsBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateOutputVariable", "BindID", "UpdateEvent", true);
         OutputVarsBasicTypedSuperList.SetTypeEditor("TriggersTemplateOutputVariable", "BindID", typeof(VariableIDProperty));
         OutputVarsBasicTypedSuperList.mListDataObjectType = typeof(TriggersTemplateOutputVariable);
         OutputVarsBasicTypedSuperList.SelectedObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(basicTypedSuperList1_SelectedObjectPropertyChanged);
         OutputVarsBasicTypedSuperList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(onAnyPropertyChanged);
         OutputVarsBasicTypedSuperList.NeedsResize += new EventHandler(BasicTypedSuperList_Changed);


         InputTriggersBasicTypedSuperList.IgnoreProperties("TriggersTemplateInputActionBinder", new string[] { "Color", "TargetIDs", "_TargetID", "BindID" });
         InputTriggersBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateInputActionBinder", "_BindID", "ReadOnly", true);
         InputTriggersBasicTypedSuperList.SetTypeEditor("TriggersTemplateInputActionBinder", "_BindID", typeof(TriggerBindIDProperty));
         InputTriggersBasicTypedSuperList.SelectedObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(InputTriggersBasicTypedSuperList_SelectedObjectPropertyChanged);
         InputTriggersBasicTypedSuperList.mListDataObjectType = typeof(TriggersTemplateInputActionBinder);
         InputTriggersBasicTypedSuperList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(onAnyPropertyChanged);
         InputTriggersBasicTypedSuperList.NeedsResize += new EventHandler(BasicTypedSuperList_Changed);

         OutputTriggersBasicTypedSuperList.IgnoreProperties("TriggersTemplateOutputActionBinder", new string[] { "Color", "TargetIDs", "_TargetID", "BindID" });
         OutputTriggersBasicTypedSuperList.AddMetaDataForProp("TriggersTemplateOutputActionBinder", "_BindID", "ReadOnly", true);
         OutputTriggersBasicTypedSuperList.SetTypeEditor("TriggersTemplateOutputActionBinder", "_BindID", typeof(TriggerBindIDProperty));
         OutputTriggersBasicTypedSuperList.SelectedObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(OutputTriggersBasicTypedSuperList_SelectedObjectPropertyChanged);
         OutputTriggersBasicTypedSuperList.mListDataObjectType = typeof(TriggersTemplateOutputActionBinder);
         OutputTriggersBasicTypedSuperList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(onAnyPropertyChanged);
         OutputTriggersBasicTypedSuperList.NeedsResize += new EventHandler(BasicTypedSuperList_Changed);




         TriggerRoot trigroot = new TriggerRoot();
         mFakeTriggerSystem.TriggerData = trigroot;
         triggerHostArea2.CurrentTriggerNamespace = mFakeTriggerSystem.MainNamespace;



         TemplateDefinition = new TriggerTemplateDefinition();
      }

      void triggerHostArea1_ViewChanged(object sender, EventArgs e)
      {
         this.previewNavWindow1.MasterControl = triggerHostArea1.ActiveNodeHostControl;
      }

      void BasicTypedSuperList_Changed(object sender, EventArgs e)
      {
         RenderTemplate();
      }

      TriggerSystemMain mFakeTriggerSystem = new TriggerSystemMain();

      public void RenderTemplate()
      {

         mFakeTriggerSystem.MainNamespace.ClearData();
         this.triggerHostArea2.Controls.Clear();


         TemplateControl newC = new TemplateControl(this.triggerHostArea2.mRootLevel, this.triggerHostArea2);
         newC.mbDemoMode = true;
         newC.ParentTriggerNamespace = mFakeTriggerSystem.MainNamespace;
         newC.TriggerTemplateMapping = this.mTemplateDefinition.TriggerTemplateMapping;
         newC.Location = new Point(100, 0);
         triggerHostArea2.Controls.Add(newC);

         triggerHostArea2.Refresh();

         triggerHostArea2.AutoScroll = false;
      }

      void onAnyPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         RenderTemplate();

      }

      void OutputTriggersBasicTypedSuperList_SelectedObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {

      }

      void InputTriggersBasicTypedSuperList_SelectedObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {

      }
      void basicTypedSuperList1_SelectedObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         TriggersTemplateVariableBinder varBinder = selectedObject as TriggersTemplateVariableBinder;
         if (varBinder != null)
         {
            TriggerValue value;
            if (mMainTriggerSystem.MainNamespace.GetValues().TryGetValue(varBinder.BindID, out value))
            {
               varBinder.Type = value.Type;
               varBinder.Name = value.Name;

               sender.UpdateData();
            }
            
         }
      }


      TriggerSystemMain mMainTriggerSystem = new TriggerSystemMain();

      private TriggerTemplateDefinition mTemplateDefinition = null;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerTemplateDefinition TemplateDefinition
      {
         set
         {
            //mMainTriggerSystem.TriggerData = value;
            //load ui
            //triggerHostArea1.CurrentTriggerNamespace = mMainTriggerSystem.MainNamespace;
            mTemplateDefinition = value;
            mMainTriggerSystem.TriggerData = value.TriggerSystemRoot;
            triggerHostArea1.CurrentTriggerNamespace = mMainTriggerSystem.MainNamespace;
            //triggerHostArea1.
            InputVarsBasicTypedSuperList.ObjectList = mTemplateDefinition.TriggerTemplateMapping.InputMappings;

            OutputVarsBasicTypedSuperList.ObjectList = mTemplateDefinition.TriggerTemplateMapping.OutputMappings;

            OutputTriggersBasicTypedSuperList.ObjectList = mTemplateDefinition.TriggerTemplateMapping.TriggerOutputs;
            InputTriggersBasicTypedSuperList.ObjectList = mTemplateDefinition.TriggerTemplateMapping.TriggerInputs;

            this.TemplateAttributesPropertyGrid.SelectedObject = mTemplateDefinition.TriggerTemplateMapping;

            RenderTemplate();
         }
         get
         {

            //mMainTriggerSystem.MainNamespace.RenderOutputData();
            mTemplateDefinition.TriggerSystemRoot = mMainTriggerSystem.TriggerData;

            mTemplateDefinition.TriggerTemplateMapping.FinalizeForSave();

            return mTemplateDefinition;
         }
      }
      //public override void save_file()
      //{
      //   SaveAsTemplate();
      //}

      //public override void save_file_as()
      //{
      //}

      private void LoadButton_Click(object sender, EventArgs e)
      {
         try
         {
            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "Templates (*.xml)|*.xml";
            d.InitialDirectory = CoreGlobals.getWorkPaths().mTemplateRoot ;
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerTemplateDefinition), new Type[] { });
               Stream st = File.OpenRead(d.FileName);
               TemplateDefinition = (TriggerTemplateDefinition)s.Deserialize(st);
               st.Close();

               this.Parent.Text = Path.GetFileName(d.FileName);

               saveAsName = d.FileName;
            }

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         if (saveAsName == "")
         {
            SaveAsTemplate();
         }
         else
         {
            try
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerTemplateDefinition), new Type[] { });
               Stream st = File.Open(saveAsName, FileMode.Create);
               s.Serialize(st, TemplateDefinition);
               st.Close();
               triggerHostArea1.Scan(true);
            }
            catch (System.Exception ex)
            {
               CoreGlobals.getErrorManager().OnException(ex);
            }
         }
      }

      string saveAsName = "";

      private void SaveAsTemplate()
      {
         try
         {

            SaveFileDialog d = new SaveFileDialog();
            d.Filter = "Templates (*.xml)|*.xml";
            d.InitialDirectory = CoreGlobals.getWorkPaths().mTemplateRoot;
            if (d.ShowDialog() == DialogResult.OK)
            {
               XmlSerializer s = new XmlSerializer(typeof(TriggerTemplateDefinition), new Type[] { });
               Stream st = File.Open(d.FileName, FileMode.Create);

               s.Serialize(st, TemplateDefinition);
               st.Close();
               this.Parent.Text = Path.GetFileName(d.FileName);

               saveAsName = d.FileName;

               triggerHostArea1.Scan(true);

               //Not used by game, no xmb needed

            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void ReloadTemplatesButton_Click(object sender, EventArgs e)
      {
         TriggerSystemMain.mTriggerDefinitions.LoadTemplates();
      }

      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         SaveAsTemplate();
      }


   }
   
}
