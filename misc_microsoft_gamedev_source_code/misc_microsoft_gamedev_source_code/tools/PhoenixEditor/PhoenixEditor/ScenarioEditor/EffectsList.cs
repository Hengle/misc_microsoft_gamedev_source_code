using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using System.IO;
using System.Text.RegularExpressions;

using SimEditor;
using EditorCore;

using EditorCore.Controls.Micro;
namespace PhoenixEditor.ScenarioEditor
{
   public partial class EffectsList : PhoenixEditor.ScenarioEditor.TriggerComponentList, IContentProvider
   {
      public EffectsList()
      {
         InitializeComponent();

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

      bool mEffectsOnFalse = false;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public bool EffectsOnFalse
      {
         get
         {
            return mEffectsOnFalse;
         }
         set
         {
            mEffectsOnFalse = value;
         }
      }

      public void Init()
      {
         List<string> effects = new List<string>();
         if (TriggerSystemMain.mTriggerDefinitions != null)
         {

            foreach (string S in TriggerSystemMain.mTriggerDefinitions.GetEffectNames())
            {
               effects.Add(S);
            }
            effects.Sort();
            //SetSimpleMenu(effects);
         }
      }
      public override void OnDelete(SuperListDragButton button)
      {
         bool deleteWorked = false;
         FunctorEditor f = button.Tag as FunctorEditor;
         if (f != null)
         {
            TriggerEffect effect = f.Tag as TriggerEffect;
            if (f != null)
            {
               if (ParentTriggerNamespace.DeleteEffect(Trigger, effect, EffectsOnFalse, true))
               {
                  base.OnDelete(button);
                  deleteWorked = true;
               }
            }
         }
         if (deleteWorked == false)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning("Error deleting effect.");
         }
      }

      override protected void OnReordered()
      {
         List<TriggerEffect> effects = new List<TriggerEffect>();

         foreach (Control c in mControls)
         {
            FunctorEditor f = c as FunctorEditor;
            if (f != null)
            {
               TriggerEffect effect = f.Tag as TriggerEffect;
               if (effect != null)
               {
                  effects.Add(effect);
               }
            }
         }
         ParentTriggerNamespace.ReOrderEffects(Trigger, effects, EffectsOnFalse);
      }

      protected override void HandleMenuOption(string text, object tag)
      {
         TriggerEffect effect;
         Dictionary<int, TriggerValue> values;
         TriggerSystemMain.mTriggerDefinitions.GetTriggerEffect(text, out effect, out values);
   
         int newID;
         mParentTriggerNamespace.InsertEffect(this.Trigger.ID, effect, values,EffectsOnFalse, out newID);
  
         Control c = BuildUIFromEffect(effect, values);

         this.AddRow(c);
      }

      public void AddExistingEffectToUI(TriggerEffect effect, Dictionary<int, TriggerValue> values)
      {
         Control c = BuildUIFromEffect(effect, values);

         this.AddRow(c);
      }

      private void ReloadUI()
      {
         this.BatchSuspend();
         this.ClearControls();
         
         List<TriggerEffect> effectList;// = mParentTriggerNamespace.WalkEffects(Trigger);
         if(EffectsOnFalse)
         {
            effectList = this.Trigger.TriggerEffectsFalse.Effects;
         }
         else
         {
            effectList = this.Trigger.TriggerEffects.Effects;
         }
         foreach (TriggerEffect e in effectList)
         {
            if ((e.Type == TriggerSystemMain.mTriggerDefinitions.GetDeactivateEffectName())
            || (e.Type == TriggerSystemMain.mTriggerDefinitions.GetActivateEffectName()))
            {
               continue;
            }
       
            AddExistingEffectToUI(e, mParentTriggerNamespace.GetValues());
         }
         this.BatchResume();
      }

      //
      public Control BuildUIFromEffect(TriggerEffect effect, Dictionary<int, TriggerValue> values)
      {
         FunctorEditor f = new FunctorEditor();
         f.LayoutStyle = FunctorEditor.eLayoutStyle.VerticleList;
         f.Dock = DockStyle.Fill;

         f.Tag = effect;

         f.FunctionName = effect.Type;

         f.LogicalHost = this;

         f.FunctionNameClicked += new EventHandler(f_FunctionNameClicked);
         f.FunctionNameHover += new EventHandler(f_HotSelect);
         f.FunctionNameRightClick+=new EventHandler(f_FunctionNameRightClick);

         List<Control> inputList = new List<Control>();
         List<Control> outputList = new List<Control>();
         bool bErrors = false;


         bool bWatchForChange = TriggerSystemMain.mTriggerDefinitions.IsDynamicComponent(effect.Type);

         foreach (TriggerVariable v in effect.Parameter)
         {
            try
            {

               if (v is TriggersInputVariable)
               {
                  inputList.Add(BuildUIFromParameterVariable(effect, v, values[v.ID], bWatchForChange));
               }
               else if (v is TriggersOutputVariable)
               {
                  outputList.Add(BuildUIFromParameterVariable(effect, v, values[v.ID], bWatchForChange));
               }

            }
            catch (System.Exception ex)
            {
               bErrors = true;
               CoreGlobals.getErrorManager().SendToErrorWarningViewer("Missing value: " + v.ID);
            }
         }

         f.SetupParameters(inputList, outputList);

         //Debug info

         if (effect.HasErrors == true || bErrors)
         {
            f.SetErrorText();
         }
         else if (effect.HasWarnings == true)
         {
            f.SetWarningText();
         }
         else if (effect.JustUpgraded == true)
         {
            f.SetUpdatedText();
         }

         UpdateComponentVisuals(effect, f);

         return f;
      }


      ContextMenu mOptionsMenu = new ContextMenu();
      void f_FunctionNameRightClick(object sender, EventArgs e)
      {
         mOptionsMenu = new ContextMenu();
         FunctorEditor fe = sender as FunctorEditor;
         TriggerEffect comp = fe.Tag as TriggerEffect;
         if (comp == null) return;

         MenuItem commentOutItem = new MenuItem("Comment Out");
         commentOutItem.Checked = comp.CommentOut;
         commentOutItem.Click += new EventHandler(commentOutItem_Click);
         commentOutItem.Tag = fe;
         mOptionsMenu.MenuItems.Add(commentOutItem);


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
         TriggerEffect comp = fe.Tag as TriggerEffect;
         if (comp == null) return;

         comp.CommentOut = true;
         //UpdateComponentVisuals(comp, fe);

         TriggerEffect effect;
         Dictionary<int, TriggerValue> values;
         TriggerSystemMain.mTriggerDefinitions.GetTriggerEffect(comp.Type, out effect, out values);
         int newID;
         mParentTriggerNamespace.InsertEffect(this.Trigger.ID, effect, values, this.EffectsOnFalse , out newID, comp);

         //copy over old values
         Dictionary<int, int> oldIDsBySigID = new Dictionary<int, int>();
         for (int i = 0; i < comp.Parameter.Count; i++)
         {
            oldIDsBySigID[comp.Parameter[i].SigID] = comp.Parameter[i].ID;
         }
         List<TriggerVariable> oldParams = comp.Parameter;
         for (int i = 0; i < effect.Parameter.Count; i++)
         {
            int oldVarID;
            if (oldIDsBySigID.TryGetValue(effect.Parameter[i].SigID, out oldVarID))
            {
               effect.Parameter[i].ID = oldVarID;

               values[oldVarID] = mParentTriggerNamespace.GetValues()[oldVarID];
            }
         }

         // Halwes - 12/10/2007 - This is a specific update for ModifyProtoData trigger effect revision 3 or older.
         if ((effect.Type == "ModifyProtoData") && (comp.Version <= 3))
         {
            foreach (TriggerVariable var in effect.Parameter)
            {
               if (var.GetName() == "DataType")
               {
                  if (values[var.GetID()].Value == "0")
                     values[var.GetID()].Value = "Enable";
                  else if (values[var.GetID()].Value == "1")
                     values[var.GetID()].Value = "Hitpoints";
                  else if (values[var.GetID()].Value == "2")
                     values[var.GetID()].Value = "Shieldpoints";
                  else if( values[var.GetID()].Value == "3" )
                     values[var.GetID()].Value = "AmmoMax";
                  else if( values[var.GetID()].Value == "4" )
                     values[var.GetID()].Value = "LOS";
                  else if( values[var.GetID()].Value == "5" )
                     values[var.GetID()].Value = "MaximumVelocity";
                  else if( values[var.GetID()].Value == "6" )
                     values[var.GetID()].Value = "MaximumRange";
                  else if( values[var.GetID()].Value == "7" )
                     values[var.GetID()].Value = "ResearchPoints";
                  else if( values[var.GetID()].Value == "8" )
                     values[var.GetID()].Value = "ResourceTrickleRate";
                  else if( values[var.GetID()].Value == "9" )
                     values[var.GetID()].Value = "MaximumResourceTrickleRate";
                  else if( values[var.GetID()].Value == "10" )
                     values[var.GetID()].Value = "RateAmount";
                  else if( values[var.GetID()].Value == "11" )
                     values[var.GetID()].Value = "RateMultiplier";
                  else if( values[var.GetID()].Value == "12" )
                     values[var.GetID()].Value = "Resource";
                  else if( values[var.GetID()].Value == "13" )
                     values[var.GetID()].Value = "Projectile";
                  else if( values[var.GetID()].Value == "14" )
                     values[var.GetID()].Value = "Damage";
                  else if( values[var.GetID()].Value == "15" )
                     values[var.GetID()].Value = "AttackRate";
                  else if( values[var.GetID()].Value == "16" )
                     values[var.GetID()].Value = "MinRange";
                  else if( values[var.GetID()].Value == "17" )
                     values[var.GetID()].Value = "AOERadius";
                  else if( values[var.GetID()].Value == "18" )
                     values[var.GetID()].Value = "AOEPrimaryTargetFactor";
                  else if( values[var.GetID()].Value == "19" )
                     values[var.GetID()].Value = "AOEDistanceFactor";
                  else if( values[var.GetID()].Value == "20" )
                     values[var.GetID()].Value = "AOEDamageFactor";
                  else if( values[var.GetID()].Value == "21" )
                     values[var.GetID()].Value = "Accuracy";
                  else if( values[var.GetID()].Value == "22" )
                     values[var.GetID()].Value = "MovingAccuracy";
                  else if( values[var.GetID()].Value == "23" )
                     values[var.GetID()].Value = "MaxDeviation";
                  else if( values[var.GetID()].Value == "24" )
                     values[var.GetID()].Value = "MovingMaxDeviation";
                  else if( values[var.GetID()].Value == "25" )
                     values[var.GetID()].Value = "DataAccuracyDistanceFactor";
                  else if( values[var.GetID()].Value == "26" )
                     values[var.GetID()].Value = "AccuracyDeviationFactor";
                  else if( values[var.GetID()].Value == "27" )
                     values[var.GetID()].Value = "MaxVelocityLead";
                  else if( values[var.GetID()].Value == "28" )
                     values[var.GetID()].Value = "WorkRate";
                  else if( values[var.GetID()].Value == "29" )
                     values[var.GetID()].Value = "BuildPoints";
                  else if( values[var.GetID()].Value == "30" )
                     values[var.GetID()].Value = "Cost";
                  else if( values[var.GetID()].Value == "31" )
                     values[var.GetID()].Value = "AutoCloak";
                  else if( values[var.GetID()].Value == "32" )
                     values[var.GetID()].Value = "MoveWhileCloaked";
                  else if( values[var.GetID()].Value == "33" )
                     values[var.GetID()].Value = "ActionEnable";
                  else if( values[var.GetID()].Value == "34" )
                     values[var.GetID()].Value = "CommandEnable";
                  else if( values[var.GetID()].Value == "35" )
                     values[var.GetID()].Value = "BountyResource";
                  else if( values[var.GetID()].Value == "36" )
                     values[var.GetID()].Value = "TributeCost";
                  else if( values[var.GetID()].Value == "37" )
                     values[var.GetID()].Value = "ShieldRegenRate";
                  else if( values[var.GetID()].Value == "38" )
                     values[var.GetID()].Value = "ShieldRegenDelay";
                  else if( values[var.GetID()].Value == "39" )
                     values[var.GetID()].Value = "DamageModifier";
                  else if( values[var.GetID()].Value == "40" )
                     values[var.GetID()].Value = "PopCap";
                  else if( values[var.GetID()].Value == "41" )
                     values[var.GetID()].Value = "PopMax";
                  else if( values[var.GetID()].Value == "42" )
                     values[var.GetID()].Value = "UnitTrainLimit";
                  else if( values[var.GetID()].Value == "43" )
                     values[var.GetID()].Value = "SquadTrainLimit";
                  else if( values[var.GetID()].Value == "44" )
                     values[var.GetID()].Value = "SquadTrainLimit";
                  else if( values[var.GetID()].Value == "45" )
                     values[var.GetID()].Value = "RepairCost";
                  else if( values[var.GetID()].Value == "46" )
                     values[var.GetID()].Value = "RepairTime";
                  else if( values[var.GetID()].Value == "47" )
                     values[var.GetID()].Value = "PowerRechargeTime";
                  else if( values[var.GetID()].Value == "48" )
                     values[var.GetID()].Value = "PowerUseLimit";
               }
               else if (var.GetName() == "AmountRelation")
               {
                  if( values[var.GetID()].Value == "0" )
                     values[var.GetID()].Value = "Absolute";
                  else if( values[var.GetID()].Value == "1" )
                     values[var.GetID()].Value = "BasePercent";
                  else if( values[var.GetID()].Value == "2" )
                     values[var.GetID()].Value = "Percent";
                  else if( values[var.GetID()].Value == "3" )
                     values[var.GetID()].Value = "Assign";
                  else if( values[var.GetID()].Value == "4" )
                     values[var.GetID()].Value = "BasePercentAssign";
               }
            }
         }

         effect.JustUpgraded = true;
         //Control c = BuildUIFromEffect(effect, values);         
         //this.AddRow(c);

         ReloadUI();
      }


      void commentOutItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         FunctorEditor fe = mi.Tag as FunctorEditor;
         TriggerEffect comp = fe.Tag as TriggerEffect;
         if (comp == null) return;

         comp.CommentOut = !comp.CommentOut;
         UpdateComponentVisuals(comp, fe);
      }
      Color mLastBackColor = Color.Empty;
      void UpdateComponentVisuals(TriggerEffect effect, FunctorEditor fe)
      {

         if (effect.CommentOut == true)
         {
            if (mLastBackColor != Color.DarkGray)
               mLastBackColor = fe.BackColor;
            fe.BackColor = Color.DarkGray;
         }
         else if (mLastBackColor != Color.Empty)
         {
            fe.BackColor = Color.Empty;// mLastBackColor;
         }
      }

      void f_HotSelect(object sender, EventArgs e)
      {
         //if (mComponentHelp != null)
         //{
         //   if (mComponentHelp.Active)
         //   {
         //      mComponentHelp.Active = false;
         //      mComponentHelp.Dispose();//.Hide(this.Parent.Parent);
         //   }
         //   mComponentHelp = new ToolTip();
         //}       

         FunctorEditor f = sender as FunctorEditor;

         ToolTip t = mToolTipGroup.GetToolTip(sender.GetHashCode()); //mComponentHelp;
         t.IsBalloon = true;
         Point p = f.PointToScreen(new Point(0, 0));
         p = this.PointToClient(p);
         TriggerComponent comp = f.Tag as TriggerComponent;
         TriggerComponentDefinition def;
         if (comp == null)
            return;
         if (TriggerSystemMain.mTriggerDefinitions.TryGetDefinition(comp.DBID, comp.Version, out def) == false)
            return;

         string message = UpdateHelpText(f, def);
         if (message == "") message = f.FunctionName;
         t.Show(message, f, 50, -40, 5000);

         //t.Show(message, f, f.Width, -45, 5000);
         //t.Show(message, this, 140, p.Y - 2, 3000);
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
      public Control BuildUIFromParameterVariable(TriggerComponent comp, TriggerVariable var, TriggerValue val, bool watchForChange)
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
         try
         {
            //return;
            TriggerParameterControl c = sender as TriggerParameterControl;
            //ToolTip t = mToolTipGroup.GetToolTip(c.Parent.GetHashCode());//new ToolTip();
            ToolTip t = mToolTipGroup.GetToolTip(c.GetHashCode());//new ToolTip();
            Point p = c.PointToScreen(new Point(0, 0));
            p = this.PointToClient(p);
            string message = c.GetValue().Type + ": \"" + c.GetVariable().Name + "\"";
            t.UseAnimation = false;
            t.UseFading = false;
            t.Show(message, this, 140, p.Y - 2, 3000);
         }
         catch (System.Exception ex)
         {
            MessageBox.Show("Emergency save to crap1.triggerscript");
            System.Xml.Serialization.XmlSerializer s = new System.Xml.Serialization.XmlSerializer(typeof(TriggerRoot), new Type[] { });
            System.IO.Stream st = System.IO.File.Open(System.IO.Path.Combine(CoreGlobals.getWorkPaths().mScriptTriggerDirectory , "crap1.triggerscript"), System.IO.FileMode.Create);
            s.Serialize(st, mParentTriggerNamespace.TriggerData);
            st.Close();
         }
      }



      //class SmartStringTree
      //{

      //   Dictionary<string, List<string>> mMap = null;// = new Dictionary<string, List<string>>();
      //   ICollection<string> mNames = null;

      //   public void Init(ICollection<string> rawInput)
      //   {
      //      Dictionary<string, List<string>> map = new Dictionary<string, List<string>>();
      //      mNames = rawInput;
      //      List<string> toIgnore = new List<string>();
      //      toIgnore.Add("KBSF");
      //      //toIgnore.AddRange(TriggerSystemMain.mTriggerDefinitions.GetTypeNames());
      //      mMap = BuildMap(rawInput, toIgnore, 3);
      //   }

      //   private static Dictionary<string, List<string>> BuildMap(ICollection<string> rawInput, ICollection<string> toIgnore, int minMatches)
      //   {
      //      Dictionary<string, List<string>> map = new Dictionary<string, List<string>>();
      //      foreach (string s in rawInput)
      //      {
      //         string[] camelSplit = Regex.Split(s, @"(?<!^)(?=[A-Z])");
      //         s.GetHashCode();
      //         string buffer = "";
      //         foreach (string w in camelSplit)
      //         {
      //            if (w.Length == 1)
      //            {
      //               buffer += w;
      //               continue;
      //            }
      //            else
      //            {
      //               if (buffer.Length > 0)
      //               {
      //                  if (map.ContainsKey(buffer) == false)
      //                  {
      //                     map[buffer] = new List<string>();
      //                  }
      //                  if (buffer.Contains("(") || buffer.Contains(")") || toIgnore.Contains(buffer))
      //                  {
      //                     //skip it...
      //                  }
      //                  else
      //                  {
      //                     map[buffer].Add(s);
      //                  }
      //               }
      //               buffer = "";
      //            }
      //            if (map.ContainsKey(w) == false)
      //            {
      //               map[w] = new List<string>();
      //            }
      //            if (w.Contains("(") || w.Contains(")") || toIgnore.Contains(w))
      //               continue;
      //            map[w].Add(s);
      //         }
      //      }

      //      List<string> toRemove = new List<string>();

      //      Dictionary<string, List<string>>.Enumerator it = map.GetEnumerator();
      //      while (it.MoveNext())
      //      {
      //         if (it.Current.Value.Count < minMatches)
      //         {
      //            toRemove.Add(it.Current.Key);
      //         }
      //      }
      //      foreach (string s in toRemove)
      //      {
      //         map.Remove(s);
      //      }
      //      return map;
      //   }

      //   static int compareIntString(Pair<int, string> a, Pair<int, string> b)
      //   {
      //      return b.Key - a.Key;
      //   }
      //   public TreeNode BuildTree()
      //   {
      //      int minCount = 3;
            
      //      TreeNode root = new TreeNode();
      //      root.Text = "ALL";

      //      List<string> topLevel = new List<string>();            
      //      List<string> usedGroups = new List<string>();
      //      List<string> usedEntries = new List<string>();

      //      List<Pair<int, string>> sizes = new List<Pair<int, string>>();
      //      Dictionary<string, List<string>>.Enumerator it2 = mMap.GetEnumerator();
      //      while (it2.MoveNext())
      //      {
      //         sizes.Add(new Pair<int,string>(it2.Current.Value.Count, it2.Current.Key));
      //      }
      //      sizes.Sort(compareIntString);

      //      List<string> toIgnore = new List<string>();
      //      foreach (Pair<int, string> pair in sizes)
      //      {
      //         string thisGroup = pair.Value;
      //         if (usedGroups.Contains(thisGroup))
      //            continue;

      //         //TreeNode n = new TreeNode();
      //         //n.Text = thisGroup;
      //         //root.Nodes.Add(n);

      //         toIgnore.Clear();
      //         toIgnore.Add(thisGroup);
      //         List<string> values = mMap[thisGroup];

      //         Dictionary<string, List<string>> moreSubStrings = BuildMap(values, toIgnore, 3);
      //         //add substrings ... 
      //         Dictionary<string, List<string>>.Enumerator it3 = moreSubStrings.GetEnumerator();
      //         while (it3.MoveNext())
      //         {
      //            string subGroup = it3.Current.Key;
      //            if (usedGroups.Contains(subGroup))
      //               continue;

      //            if (!mMap.ContainsKey(subGroup) || Set.IsSubset<string>(mMap[subGroup], it3.Current.Value) == false)
      //            {
      //               continue;
      //            }

      //            //TreeNode n2 = new TreeNode();
      //            //n2.Text = subGroup;
      //            //n.Nodes.Add(n2);

      //            usedGroups.Add(subGroup);
      //         }
               
      //         usedGroups.Add(thisGroup);
      //         topLevel.Add(thisGroup);
      //      }


      //      //sizes.Sort( new delegate Comparison<Pair<int,string>>(Pair<int,string> a, Pair<int,string> b){ return    } );
            
      //      topLevel.Sort();

      //      foreach (string s in topLevel)
      //      {
      //         TreeNode n = new TreeNode();
      //         n.Text = s;
      //         foreach (string subEntry in mMap[s])
      //         {
      //            TreeNode n2 = new TreeNode();
      //            n2.Text = subEntry;
      //            n.Nodes.Add(n2);

      //            usedEntries.Add(subEntry);
      //         }
      //         root.Nodes.Add(n);
      //      }

      //      TreeNode unsorted = new TreeNode();
      //      unsorted.Text = "UNSORTED";
      //      foreach (string s in mNames)
      //      {
      //         if (usedEntries.Contains(s) == false)
      //         {
      //            TreeNode n2 = new TreeNode();
      //            n2.Text = s;
      //            unsorted.Nodes.Add(n2);
      //         }
      //      }
      //      root.Nodes.Add(unsorted);

      //      return root;
      //   }
         

      //}

      static SmartStringTree mSmartTree = new SmartStringTree();


      Form mpe = null;
      protected override void HandleAddItemButton(Control AddItemButton, Point p)
      {
         ICollection<string> names = TriggerSystemMain.mTriggerDefinitions.GetEffectNames();
         if (mpe != null && mpe.IsDisposed == false)
         {
            mpe.BringToFront();
            return;
         }
         //////////////////////////////////////////////////////
         ////mSmartTree.Init(names);
         ////TreeNode root = mSmartTree.BuildTree();
         //TreeNode root = SmartStringTree.BuildTree(names, new string[] { "KBSF" });
         //PopupEditor pe2 = new PopupEditor();
         //TreeView tv = new TreeView();
         //tv.Height = 500;
         //tv.Width = 300;
         //tv.Nodes.Add(root);
         //mpe = pe2.ShowPopup(this, tv);
         //return;

         ///////////////////////////////////////////////////////
         //PopupEditor pe2 = new PopupEditor();
         //SuperPicker tv = new SuperPicker();

         //string filename = CoreGlobals.getWorkPaths().mEditorSettings + "\\tempMenu.xml";
         //CategoryTreeNode settings = new CategoryTreeNode();
         //if (File.Exists(filename))
         //{
         //   settings = BaseLoader<CategoryTreeNode>.Load(filename);
         //}
         //settings.mFileName = filename;
         //settings.mAllEntries.AddRange(names);  
         //tv.Setup(settings);

         //tv.mContentProfivider = this;

         //mpe = pe2.ShowPopup(this, tv);
         //return;

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
         picker.Height += 100;
         picker.Width += 50;
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
         TriggerEffect comp = f.Tag as TriggerEffect;
         if (comp == null) return;
         //comp.CopyTo

         TriggerControl trigCont = otherlist.Parent.Parent as TriggerControl;

         TriggerClipboard temp = new TriggerClipboard();
         //temp.CopyValues(this.mParentTriggerNamespace.GetValueList());
         temp.mValues = this.mParentTriggerNamespace.GetValues();

         int newID;
         TriggerEffect e = temp.GetEffect(comp, this.mParentTriggerNamespace);//, out newID);
         EffectsList ef = otherlist as EffectsList;

         //mParentTriggerNamespace.InsertEffect(trigCont.Trigger.ID, e, temp.mValues, EffectsOnFalse, out newID);
         mParentTriggerNamespace.InsertEffect(trigCont.Trigger.ID, e, this.mParentTriggerNamespace.GetValues(), ef.EffectsOnFalse, out newID);

         ef.AddExistingEffectToUI(e, this.mParentTriggerNamespace.GetValues());

         mParentTriggerNamespace.ProcessVarMapping();

      }



      #region IContentProvider Members

      public Control ProvideContent(string key, Control lastContent)
      {
         TriggerComponentDefinition def;
         if (TriggerSystemMain.mTriggerDefinitions.TryGetDefinition(key, true, out def) == false)
         {
            return null;
         }
         ComponentPickerHelp ph = new ComponentPickerHelp();
         ph.Component = def;
         return ph;
      }

      #endregion
   }
}

