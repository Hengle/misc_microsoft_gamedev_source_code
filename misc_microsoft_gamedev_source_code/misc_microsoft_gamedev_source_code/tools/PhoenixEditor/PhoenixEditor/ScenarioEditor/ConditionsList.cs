using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class ConditionsList : PhoenixEditor.ScenarioEditor.TriggerComponentList
   {
      public ConditionsList()
      {
         InitializeComponent();

         //SetSimpleMenu()
         Init();
      }

      TriggerNamespace mParentTriggerNamespace = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerNamespace ParentTriggerNamespace
      {
         set
         {
            mParentTriggerNamespace = value;
         }
         get
         {
            return mParentTriggerNamespace;
         }
      }
      Trigger mParentTrigger = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public Trigger Trigger
      {
         get
         {
            return mParentTrigger;
         }
         set
         {
            mParentTrigger = value;
         }
      }

      public void Init()
      {
         List<string> conditions = new List<string>();
         if(TriggerSystemMain.mTriggerDefinitions != null)
         {
            foreach (string S in TriggerSystemMain.mTriggerDefinitions.GetConditionNames())
            {
               conditions.Add(S);
            }
            conditions.Sort();
            //SetSimpleMenu(conditions);
         }
      }
      public override void OnDelete(SuperListDragButton button)
      {
         bool deleteWorked = false;
         FunctorEditor f = button.Tag as FunctorEditor;
         if(f != null)
         {
            TriggerCondition cond = f.Tag as TriggerCondition;
            if(f != null)
            {
               if (ParentTriggerNamespace.DeleteCondition(Trigger, cond, true))
               {
                  base.OnDelete(button);
                  deleteWorked = true;
               }
            }
         }
         if(deleteWorked == false)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning("Error deleting condition.");
         }
      }

      //...ugh, this will be a tree in the future...
      override protected void OnReordered()
      {
         List<TriggerCondition> conditions = new List<TriggerCondition>();

         foreach (Control c in mControls)
         {
            FunctorEditor f = c as FunctorEditor;
            if (f != null)
            {
               TriggerCondition cond = f.Tag as TriggerCondition;
               if(cond != null)
               {
                  conditions.Add(cond);
               }
            }
         }
         ParentTriggerNamespace.ReOrderConditions(Trigger, conditions);
      }

      protected override void HandleMenuOption(string text, object tag)
      {
         TriggerCondition condition;
         Dictionary<int, TriggerValue> values;
         TriggerSystemMain.mTriggerDefinitions.GetTriggerCondition(text, out condition, out values);

         int newID;
         mParentTriggerNamespace.InsertCondition(this.Trigger.ID, condition, values, out newID);

         Control c = BuildUIFromCondition(condition,values);

         this.AddRow(c);
      }

      public void AddExistingConditionToUI(TriggerCondition condition, Dictionary<int, TriggerValue> values)
      {
         Control c = BuildUIFromCondition(condition, values);

         this.AddRow(c);
      }

      private void ReloadUI()
      {
         this.BatchSuspend();
         this.ClearControls();
         List<TriggerCondition> conditionList = mParentTriggerNamespace.WalkConditions(Trigger.TriggerConditions.Child);
         foreach (TriggerCondition c in conditionList)
         {
            AddExistingConditionToUI(c, mParentTriggerNamespace.GetValues());
         }
         this.BatchResume();
      }

      //
      public Control BuildUIFromCondition(TriggerCondition condition, Dictionary<int, TriggerValue> values)
      {
         FunctorEditor f = new FunctorEditor();
         f.LayoutStyle = FunctorEditor.eLayoutStyle.VerticleList;

         f.Dock = DockStyle.Fill;

         f.Tag = condition;

         //f.FunctionName = condition.Type;
         UpdateComponentVisuals(condition, f);

         f.LogicalHost = this;

         f.FunctionNameClicked += new EventHandler(f_FunctionNameClicked);
         f.FunctionNameRightClick += new EventHandler(f_FunctionNameRightClick);
         f.FunctionNameHover += new EventHandler(f_HotSelect);

         List<Control> inputList = new List<Control>();
         List<Control> outputList = new List<Control>();
         bool bErrors = false;

         bool bWatchForChange = TriggerSystemMain.mTriggerDefinitions.IsDynamicComponent(condition.Type);

         foreach (TriggerVariable v in condition.Parameter)
         {
            try
            {

               if (v is TriggersInputVariable)
               {
                  inputList.Add(BuildUIFromConditionVariable(condition, v, values[v.ID], bWatchForChange));
               }
               else if (v is TriggersOutputVariable)
               {
                  outputList.Add(BuildUIFromConditionVariable(condition, v, values[v.ID], bWatchForChange));
               }
            }
            catch(System.Exception ex)
            {
               bErrors = true;
               CoreGlobals.getErrorManager().SendToErrorWarningViewer("Missing value: " + v.ID);

            }
            
         }

         f.SetupParameters(inputList,outputList);


         //Debug info
         if (condition.HasErrors == true || bErrors)
         {
            f.SetErrorText();
         }
         else if (condition.HasWarnings == true)
         {
            f.SetWarningText();
         }
         else if (condition.JustUpgraded == true)
         {
            f.SetUpdatedText();
         }

         UpdateComponentVisuals(condition, f);

         return f;
      }

      void f_HotSelect(object sender, EventArgs e)
      {
         //if(mComponentHelp != null)
         //{
         //   if(mComponentHelp.Active)
         //   {
         //      mComponentHelp.Active = false;
         //      mComponentHelp.Dispose();//.Hide(this.Parent.Parent);
         //   }
         //   mComponentHelp = new ToolTip();
         //}
 
         FunctorEditor f = sender as FunctorEditor;

         ToolTip t = mToolTipGroup.GetToolTip(sender.GetHashCode()); //mComponentHelp;//new ToolTip();
         Point p = f.PointToScreen(new Point(0, 0));
         p = this.PointToClient(p);
         TriggerComponent comp = f.Tag as TriggerComponent;
         TriggerComponentDefinition def;
         if (comp == null)
            return;
         if (TriggerSystemMain.mTriggerDefinitions.TryGetDefinition(comp.DBID, comp.Version, out def) == false)
            return;
         t.IsBalloon = true;
         string message = UpdateHelpText(f, def);
         if (message == "") message = f.FunctionName;
//         t.Show(message, this.Parent.Parent, 0, -45, 5000);
         t.Show(message, f, 50, -40, 5000);
      }

      Color mLastBackColor = Color.Empty;
      void UpdateComponentVisuals(TriggerCondition condition, FunctorEditor fe)
      {
         fe.FunctionName = ((condition.Invert == true) ? "! " : "") + condition.Type;

         if (condition.CommentOut == true)
         {
            if (mLastBackColor != Color.DarkGray)
               mLastBackColor = fe.BackColor;
            fe.BackColor = Color.DarkGray;
         }
         else if (mLastBackColor != Color.Empty)
         {
            fe.BackColor = Color.Empty;//mLastBackColor;
         }
      }

      ContextMenu mOptionsMenu = new ContextMenu();
      void f_FunctionNameRightClick(object sender, EventArgs e)
      {
         FunctorEditor fe = sender as FunctorEditor;
         TriggerCondition comp = fe.Tag as TriggerCondition;
         if (comp == null) return;

         mOptionsMenu = new ContextMenu();         
         
         MenuItem commentOutItem = new MenuItem("Comment Out");
         commentOutItem.Checked = comp.CommentOut;
         commentOutItem.Click += new EventHandler(commentOutItem_Click);
         commentOutItem.Tag = fe;
         mOptionsMenu.MenuItems.Add(commentOutItem);


         MenuItem invertItem = new MenuItem("Invert Result");
         invertItem.Checked = comp.Invert;
         invertItem.Click += new EventHandler(invertItem_Click);
         invertItem.Tag = fe;
         mOptionsMenu.MenuItems.Add(invertItem);

         if (comp.NeedsUpgrade == true)
         {
            MenuItem upgradeItem = new MenuItem("+Upgrade Version");
            upgradeItem.Click += new EventHandler(upgradeItem_Click);
            upgradeItem.Tag = fe;
            mOptionsMenu.MenuItems.Add(upgradeItem);
         }

         mOptionsMenu.Show(fe, new Point(0, 0));

      }

      void upgradeItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         FunctorEditor fe = mi.Tag as FunctorEditor;
         TriggerCondition comp = fe.Tag as TriggerCondition;
         if (comp == null) return;

         comp.CommentOut = true;
         //UpdateComponentVisuals(comp, fe);

         TriggerCondition condition;
         Dictionary<int, TriggerValue> values;
         TriggerSystemMain.mTriggerDefinitions.GetTriggerCondition(comp.Type, out condition, out values);
         int newID;

         mParentTriggerNamespace.InsertCondition(this.Trigger.ID, condition, values, out newID, comp);

         //copy over old values
         Dictionary<int, int> oldIDsBySigID = new Dictionary<int, int>();
         for (int i = 0; i < comp.Parameter.Count; i++)
         {
            oldIDsBySigID[comp.Parameter[i].SigID] = comp.Parameter[i].ID;
         }
         List<TriggerVariable> oldParams = comp.Parameter;
         for (int i = 0; i < condition.Parameter.Count; i++)
         {
            int oldVarID;
            if (oldIDsBySigID.TryGetValue(condition.Parameter[i].SigID, out oldVarID))
            {
               condition.Parameter[i].ID = oldVarID;

               values[oldVarID] = mParentTriggerNamespace.GetValues()[oldVarID];
            }
         }


         condition.Invert = comp.Invert;
         condition.JustUpgraded = true;
         //Control c = BuildUIFromCondition(condition, values);         
         //this.AddRow(c);

         ReloadUI();
      }

      void invertItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         FunctorEditor fe = mi.Tag as FunctorEditor;
         TriggerCondition comp = fe.Tag as TriggerCondition;
         if (comp == null) return;

         comp.Invert = !comp.Invert;
         UpdateComponentVisuals(comp, fe);
      }

      void commentOutItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         FunctorEditor fe = mi.Tag as FunctorEditor;
         TriggerCondition comp = fe.Tag as TriggerCondition;
         if (comp == null) return;

         comp.CommentOut = !comp.CommentOut;
         UpdateComponentVisuals(comp, fe);
      }


      void f_FunctionNameClicked(object sender, EventArgs e)
      {
         
         FunctorEditor fe = sender as FunctorEditor;
         if (fe == null) return;
         TriggerComponent comp = fe.Tag as TriggerComponent;
         if (comp == null) return;

         TriggerComponentWorksheet worksheet = new TriggerComponentWorksheet();
         if (worksheet.Setup(mParentTriggerNamespace, comp))
         {
            PopupEditor p = new PopupEditor();
            p.ShowPopup(this, worksheet);
         }
      }
      public Control BuildUIFromConditionVariable(TriggerComponent comp, TriggerVariable var, TriggerValue val, bool watchForChange)
      {

         TriggerParameterControl c = new TriggerParameterControl();
         c.Init(comp, var, val, ParentTriggerNamespace);//, Trigger);

         c.HotSelect += new EventHandler(c_HotSelect);
         c.HotSelectEnd += new EventHandler(c_HotSelectEnd);

         if (watchForChange)
         {
            c.LabelChanged += new EventHandler(c_LabelChanged);
         }

         return c;
      }


      void c_LabelChanged(object sender, EventArgs e)
      {
         TriggerParameterControl p = sender as TriggerParameterControl;
         if (p != null)
         {
            if (TriggerSystemMain.mTriggerDefinitions.IsSchemaType(p.GetValue().Type))
            {
               Dictionary<int, TriggerValue> values;
               TriggerComponent comp = p.GetComponent();
               if (TriggerSystemMain.mTriggerDefinitions.UpdateComponentUserDataVars(comp, p.GetValue().Value, mParentTriggerNamespace, out values))
               {
                  mParentTriggerNamespace.InsertVariableList(this.Trigger.ID, comp.Parameter, values);
                  ReloadUI();
                  InvokeNeedsResize();
               }
            }
         }
      }

      void c_HotSelectEnd(object sender, EventArgs e)
      {
         mToolTipGroup.GetToolTip(-1);

      }

      void c_HotSelect(object sender, EventArgs e)
      {
         TriggerParameterControl c = sender as TriggerParameterControl;
         //ToolTip t = mToolTipGroup.GetToolTip(c.Parent.GetHashCode());
         ToolTip t = mToolTipGroup.GetToolTip(c.GetHashCode());
         string message = c.GetValue().Type + ": \"" + c.GetVariable().Name + "\"";
         t.UseAnimation = false;
         t.UseFading = false;
         //t.Show(message, c, -120, 0, 3000);
         t.Show(message, c, 130, 0, 3000);
      }

      Form mpe = null;
      protected override void HandleAddItemButton(Control AddItemButton, Point p)
      {
         ICollection<string> names = TriggerSystemMain.mTriggerDefinitions.GetConditionNames();
         if (mpe != null && mpe.IsDisposed == false)
         {
            mpe.BringToFront();
            return;
         }
         AdvancedListItemPicker picker = new AdvancedListItemPicker();
         //Set all 
         picker.SetAllChoices(names);
         picker.ListItemSelected += new EventHandler(picker_ListItemSelected);

         //Group by input parameter type
         Dictionary<string, List<string>> inputParamGroups = new Dictionary<string, List<string>>();
         foreach (string s in names)
         {
            foreach (string type in TriggerSystemMain.mTriggerDefinitions.GetInputTypesUsed(s))
            {
               if (inputParamGroups.ContainsKey(type) == false)
               {
                  inputParamGroups[type] = new List<string>(); 
               }
               inputParamGroups[type].Add(s);
            }
         }
         picker.AddTab("Input Type", inputParamGroups);

         //Group by outpu parameter type
         Dictionary<string, List<string>> outputParamGroups = new Dictionary<string, List<string>>();
         foreach (string s in names)
         {
            foreach (string type in TriggerSystemMain.mTriggerDefinitions.GetOutputTypesUsed(s))
            {
               if (outputParamGroups.ContainsKey(type) == false)
               {
                  outputParamGroups[type] = new List<string>();
               }
               outputParamGroups[type].Add(s);
            }
         }
         picker.AddTab("Output Type", outputParamGroups);

         //Group by input parameter type
         Dictionary<string, List<string>> allParamGroups = new Dictionary<string, List<string>>();
         foreach (string s in names)
         {
            foreach (string type in TriggerSystemMain.mTriggerDefinitions.GetInputTypesUsed(s))
            {
               if (allParamGroups.ContainsKey(type) == false)
               {
                  allParamGroups[type] = new List<string>();
               }
               allParamGroups[type].Add(s);
            }
            foreach (string type in TriggerSystemMain.mTriggerDefinitions.GetOutputTypesUsed(s))
            {
               if (allParamGroups.ContainsKey(type) == false)
               {
                  allParamGroups[type] = new List<string>();
               }
               allParamGroups[type].Add(s);
            }
         }
         picker.AddTab("Type", allParamGroups);


         PopupEditor pe = new PopupEditor();
         mpe = pe.ShowPopup(this, picker);
      }

      void picker_ListItemSelected(object sender, EventArgs e)
      {
         AdvancedListItemPicker picker = sender as AdvancedListItemPicker;
         HandleMenuOption(picker.SelectedItem, null);
         mpe.Close();
         mpe = null;

      }

      public override void OnCopyRequest(SuperList otherlist, SuperListDragButton toMove)
      {
         //base.OnCopyRequest(otherlist, toMove);
         Control c = toMove.Tag as Control;
         FunctorEditor f = c as FunctorEditor;
         if (f == null) return;
         TriggerCondition comp = f.Tag as TriggerCondition;
         if (comp == null) return;
         //comp.CopyTo

         TriggerControl trigCont = otherlist.Parent.Parent as TriggerControl;

         TriggerClipboard temp = new TriggerClipboard();
         //temp.CopyValues(this.mParentTriggerNamespace.GetValueList());
         temp.mValues = this.ParentTriggerNamespace.GetValues();

         int newID;
         TriggerCondition e = temp.GetCondition(comp, this.mParentTriggerNamespace);

         ConditionsList ef = otherlist as ConditionsList;

         //mParentTriggerNamespace.InsertCondition(trigCont.Trigger.ID, e, temp.mValues, out newID);
         mParentTriggerNamespace.InsertCondition(trigCont.Trigger.ID, e, this.ParentTriggerNamespace.GetValues(), out newID);


         ef.AddExistingConditionToUI(e, this.mParentTriggerNamespace.GetValues());

         mParentTriggerNamespace.ProcessVarMapping();

      } 
   }
}

