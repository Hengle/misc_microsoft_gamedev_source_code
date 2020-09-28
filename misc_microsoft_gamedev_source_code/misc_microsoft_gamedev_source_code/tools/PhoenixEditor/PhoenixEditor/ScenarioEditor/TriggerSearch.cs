using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerSearch : UserControl
   {
      public TriggerSearch()
      {
         InitializeComponent();




      }

  


      TriggerVarSearch mVarSearch = new TriggerVarSearch();
      TriggerSearchClass mTriggerSearch = new TriggerSearchClass();
      ComponentSearch mComponentSearch = new ComponentSearch();

      TriggerHostArea mHostArea = null;
      public void Init(TriggerHostArea area)
      {
         mHostArea = area;
         
         //Init var search
         VarSearchBetterPropertyGrid.AddMetaDataForProp("TriggerVarSearch", "Type", "SimpleEnumeration", TriggerSystemMain.mTriggerDefinitions.GetTypeNames());
         VarSearchBetterPropertyGrid.SetTypeEditor("TriggerVarSearch", "Type", typeof(EnumeratedProperty));

         //VarSearchBetterPropertyGrid.AddMetaDataForProp("TriggerVarSearch", "Group", "SimpleEnumeration", mHostArea.CurrentTriggerNamespace.GetGroupNames());
         //VarSearchBetterPropertyGrid.SetTypeEditor("TriggerVarSearch", "Group", typeof(EnumeratedProperty));


         this.VarSearchBetterPropertyGrid.SelectedObject = mVarSearch;
         VarListView.MultiSelect = false;
         VarSearchGridControl.SelectionMode = SelectionMode.One;
         VarSearchGridControl.SelectedRowsChanged += new EventHandler(VarSearchGridControl_SelectedRowsChanged);


         //Trigger Search
         this.TriggerGridControl.SelectionMode = SelectionMode.One;
         this.TriggerGridControl.SelectedRowsChanged += new EventHandler(TriggerGridControl_SelectedRowsChanged);
         TriggerBetterPropertyGrid.SelectedObject = mTriggerSearch;

      
         //Component Search
         this.ComponentGridControl.SelectionMode = SelectionMode.One;
         this.ComponentGridControl.SelectedRowsChanged += new EventHandler(ComponentGridControl_SelectedRowsChanged);        

         ComponentBetterPropertyGrid.AddMetaDataForProp("ComponentSearch", "Effect", "SimpleEnumeration", TriggerSystemMain.mTriggerDefinitions.GetEffectNames());
         ComponentBetterPropertyGrid.SetTypeEditor("ComponentSearch", "Effect", typeof(EnumeratedProperty));
         ComponentBetterPropertyGrid.AddMetaDataForProp("ComponentSearch", "Condition", "SimpleEnumeration", TriggerSystemMain.mTriggerDefinitions.GetConditionNames());
         ComponentBetterPropertyGrid.SetTypeEditor("ComponentSearch", "Condition", typeof(EnumeratedProperty));

         this.ComponentBetterPropertyGrid.SelectedObject = mComponentSearch;

         
      }






      private bool ValidateVar(TriggerValue val)
      {
         if (val.Name == "")
         {
            if(mVarSearch.SearchConstants == false)
              return false;
         }
         if (mVarSearch.Name != null && mVarSearch.Name != "")
         {
            if (val.Name.Contains(mVarSearch.Name) == false)
               return false;
         }
         if (mVarSearch.Type != null && mVarSearch.Type != "")
         {
            if (val.Type != mVarSearch.Type)
            {
               return false;
            }

         }
         //if (mVarSearch.Group != null && mVarSearch.Group != "")
         //{
         //   if(val.

         //}
         return true;
      }
      private void VarClearSearchButton_Click(object sender, EventArgs e)
      {
         mbPaused = true;
         mVarSearch = new TriggerVarSearch();
         this.VarSearchBetterPropertyGrid.SelectedObject = mVarSearch;
         VarListView.Items.Clear();
         this.VarSearchGridControl.DataSource = null;
         FoundVarLabel.Text = "";
         mbPaused = false;
      }
      private void VarSearchButton_Click(object sender, EventArgs e)
      {
         mbPaused = true;
         Dictionary<int, TriggerValue> values = mHostArea.CurrentTriggerNamespace.GetValues();
         Dictionary<int, TriggerValue>.Enumerator it = values.GetEnumerator();
         List<int> found = new List<int>();

         if (mVarSearch.ID != null && mVarSearch.ID != "")
         {
            int id;
            if(int.TryParse(mVarSearch.ID, out id))
            {
               TriggerValue val;
               if(values.TryGetValue(id, out val))
               {
                  if (ValidateVar(val) == true)
                     found.Add(val.ID);
               }
            }
         }
         else  //check em all
         {
            while (it.MoveNext())
            {
               if (ValidateVar(it.Current.Value) == true)
                  found.Add(it.Current.Key);
            }
         }


         //set ui
         VarListView.Items.Clear();
         foreach(int i in found)
         {
            ListViewItem item = new ListViewItem();
            if (values[i].Name == "")
            {
               if (values[i].Value != null)
               {
                  //item.Text = "Const id=" + i.ToString() + " " + values[i].Value.Substring(0, Math.Min(values[i].Value.Length, 6)) + "...";
                  item.Text = values[i].Value.Substring(0, Math.Min(values[i].Value.Length, 300)) + "   " + "Const id=" + i.ToString() + " ";// +"...";
               }
               else if (values[i].IsNull == true)
               {
                  item.Text = "Const id=" + i.ToString() + " #ignore";
               }
               else 
               {
                  item.Text = "Const id=" + i.ToString() + " #error?";
               }
            }
            else
            {
               item.Text = values[i].Name;
            }
            item.Tag = i;
            VarListView.Items.Add(item);
            VarListView.Sorting = SortOrder.Descending;
            //VarListView.getse
         }

         mbPaused = false;
      }

      private void VarListView_SelectedIndexChanged(object sender, EventArgs e)
      {
         mbPaused = true;
         List<TriggerVarJump> jumps = new List<TriggerVarJump>();
         if (VarListView.SelectedItems.Count > 0)
         {
            int id = (int)VarListView.SelectedItems[0].Tag;

            List<Trigger> triggers = mHostArea.CurrentTriggerNamespace.WalkTriggers();
            Dictionary<int, GroupUI> groups = mHostArea.CurrentTriggerNamespace.GetGroups();
            foreach (Trigger t in triggers)
            {
               GroupUI group = null;
               groups.TryGetValue(t.GroupID, out group);

               List<TriggerCondition> conditions = mHostArea.CurrentTriggerNamespace.WalkConditions(t);
               foreach (TriggerCondition c in conditions)
               {
                  List<TriggerVariable> vars = mHostArea.CurrentTriggerNamespace.WalkVariables(c);
                  foreach (TriggerVariable var in vars)
                  {
                     if (var.ID == id)
                     {
                        jumps.Add(new TriggerVarJump(group, t, var, c, mHostArea.CurrentTriggerNamespace.getTemplateMapping(t.TemplateID)));
                     }
                  }
               }
               List<TriggerEffect> effects = mHostArea.CurrentTriggerNamespace.WalkEffects(t);
               foreach (TriggerEffect ef in effects)
               {
                  List<TriggerVariable> vars = mHostArea.CurrentTriggerNamespace.WalkVariables(ef);
                  foreach (TriggerVariable var in vars)
                  {
                     if (var.ID == id)
                     {
                        jumps.Add(new TriggerVarJump(group, t, var, ef, mHostArea.CurrentTriggerNamespace.getTemplateMapping(t.TemplateID)));
                     }
                  }
               }



            }

            FoundVarLabel.Text = "Found " + mHostArea.CurrentTriggerNamespace.GetValues()[id].Name + " in " + jumps.Count + " locations:";

         }
         else
         {
            FoundVarLabel.Text = "";
         }

         this.VarSearchGridControl.DataSource = jumps;

         VarSearchGridControl.SelectedRows.Clear();


         mbPaused = false;

      }

      bool mbPaused = false;

      void VarSearchGridControl_SelectedRowsChanged(object sender, EventArgs e)
      {
         if (mbPaused)
            return;
         if (VarSearchGridControl.SelectedRows.Count > 0)
         {
            Xceed.Grid.DataRow row = VarSearchGridControl.SelectedRows[0] as Xceed.Grid.DataRow;
            if(row != null)
            {
               TriggerVarJump jump = ((TriggerVarJump)row.SourceObject);

               if (jump.map != null)
               {
                  mHostArea.SnapViewToItem(jump.map);
               }
               else
               {
                  mHostArea.SnapViewToItem(jump.comp);
                  TriggerValue val = mHostArea.CurrentTriggerNamespace.GetValues()[jump.var.ID];
                  mHostArea.UIUpdate(val, new BasicArgument(BasicArgument.eArgumentType.Select), eUpdateVisibilty.AnyVisiblity);
               }
            }
         }
         //throw new Exception("The method or operation is not implemented.");
      }

      public class TriggerVarSearch
      {
         string mName;

         public string Name
         {
            get { return mName; }
            set { mName = value; }
         }

         string mID;

         public string ID
         {
            get { return mID; }
            set { mID = value; }
         }

         //string mGroup;

         //public string Group
         //{
         //   get { return mGroup; }
         //   set { mGroup = value; }
         //}

         string mType;

         public string Type
         {
            get { return mType; }
            set { mType = value; }
         }

         bool mSearchConstants = false;

         public bool SearchConstants
         {
            get { return mSearchConstants; }
            set { mSearchConstants = value; }
         }

      }
      public class TriggerVarJump
      {
         public TriggerVarJump(GroupUI g, Trigger t, TriggerVariable v, TriggerComponent c, TriggerTemplateMapping m)
         {
            trig = t;
            var = v;
            comp = c;
            group = g;
            map = m;

         }

         public GroupUI group;
         public Trigger trig;
         public TriggerVariable var;
         public TriggerComponent comp;
         public TriggerTemplateMapping map;
         public string Group
         {
            get
            {
               if (group == null)
               {
                  return "";
               }
               return group.Title;
            }
         }
         public string Trigger
         {
            get
            {
               return trig.Name;

            }
         }
         public string Component
         {
            get
            {
               return comp.Type;
            }
         }
         public string Name
         {
            get
            {
               //if (var.Name == "")
               //{
               //   return "constant id=" + var.ID.ToString() ;
               //}              
               return var.Name;
            }
         }

         public string IO
         {
            get
            {
               if (var is TriggersOutputVariable)
               {
                  return "Output";
               }
               else
               {
                  return "Input";
               }
               //return var.Name;
            }
         }

         public string Template
         {
            get
            {
               if (map == null)
               {
                  return "";
               }
               else
               {
                  return map.Name + "  ," + map.ID;
               }
            }
         }

      }



      class TriggerSearchClass
      {
         string mName;

         public string Name
         {
            get { return mName; }
            set { mName = value; }
         }

         string mID;

         public string ID
         {
            get { return mID; }
            set { mID = value; }
         }
      }

      class TriggerJump
      {
         public TriggerJump(GroupUI g, Trigger t, TriggerTemplateMapping m)
         {
            trig = t;
            group = g;
            map = m;
         }

         public GroupUI group;
         public Trigger trig;
         public TriggerTemplateMapping map;
         public string Group
         {
            get
            {
               if (group == null)
               {
                  return "";
               }
               return group.Title;
            }
         }
         public string Trigger
         {
            get
            {
               return trig.Name;

            }
         }
         public int ID
         {
            get
            {
               return trig.ID;

            }
         }
         public string Template
         {
            get
            {
               if (map == null)
               {
                  return "";
               }
               else
               {
                  return map.Name + "  ," + map.ID;
               }
            }
         }
      }

      class ComponentSearch
      {
         string mEffect;

         public string Effect
         {
            get { return mEffect; }
            set { mEffect = value; }
         }

         string mCondition;

         public string Condition
         {
            get { return mCondition; }
            set { mCondition = value; }
         }
      }

      class ComponentJump
      {
         public ComponentJump(GroupUI g, Trigger t, TriggerComponent c, TriggerTemplateMapping m)
         {
            trig = t;
            comp = c;
            group = g;
            map = m;
         }

         public GroupUI group;
         public Trigger trig;
         public TriggerComponent comp;
         public TriggerTemplateMapping map;

         public string Group
         {
            get
            {
               if (group == null)
               {
                  return "";
               }
               return group.Title;
            }
         }
         public string Trigger
         {
            get
            {
               return trig.Name;

            }
         }
         public string Component
         {
            get
            {
               return comp.Type;
            }
         }

         public string Template
         {
            get
            {
               if (map == null)
               {
                  return "";
               }
               else
               {
                  return map.Name + "  ," + map.ID;
               }
            }
         }
      }

      void TriggerGridControl_SelectedRowsChanged(object sender, EventArgs e)
      {
         if (mbPaused)
            return;
         if (TriggerGridControl.SelectedRows.Count > 0)
         {
            Xceed.Grid.DataRow row = TriggerGridControl.SelectedRows[0] as Xceed.Grid.DataRow;
            if (row != null)
            {
               TriggerJump jump = ((TriggerJump)row.SourceObject);
               if (jump.map != null)
               {
                  mHostArea.SnapViewToItem(jump.map);
               }
               else
               {
                  mHostArea.SnapViewToItem(jump.trig);
               }
            }
         }
      }

      private void TriggerClearButton_Click(object sender, EventArgs e)
      {
         TriggerGridControl.DataRows.Clear();
         mTriggerSearch = new TriggerSearchClass();
         TriggerBetterPropertyGrid.SelectedObject = mTriggerSearch;
      }

      bool ValidateTrigger(Trigger t)
      {    
         if (mTriggerSearch.Name != null && mTriggerSearch.Name != "")
         {
            if (t.Name.Contains(mTriggerSearch.Name) == false)
               return false;
         }
         if (mTriggerSearch.ID != null && mTriggerSearch.ID != "")
         {
            int id;
            if (int.TryParse(mTriggerSearch.ID, out id))
            {
               if (id != t.ID)
                  return false;
            }
         }
         return true;
      }

      private void TriggerSearchButton_Click(object sender, EventArgs e)
      {
         mbPaused = true;
         List<Trigger> triggers = mHostArea.CurrentTriggerNamespace.WalkTriggers();

         List<TriggerJump> found = new List<TriggerJump>();
         Dictionary<int, GroupUI> groups = mHostArea.CurrentTriggerNamespace.GetGroups();
         Dictionary<int, TriggerTemplateMapping> templates = mHostArea.CurrentTriggerNamespace.getTemplateMappings();
         foreach(Trigger t in triggers)
         {            
            if (ValidateTrigger(t) == true)
            {
               GroupUI group = null;
               TriggerTemplateMapping map = null;
               groups.TryGetValue(t.GroupID, out group);
               templates.TryGetValue(t.TemplateID, out map);
               found.Add(new TriggerJump(group, t, map));
            }            
         }
         TriggerGridControl.DataSource = found;
         TriggerGridControl.SelectedRows.Clear();

         mbPaused = false;
      }

      void ComponentGridControl_SelectedRowsChanged(object sender, EventArgs e)
      {
         if (mbPaused)
            return;
         if (ComponentGridControl.SelectedRows.Count > 0)
         {
            Xceed.Grid.DataRow row = ComponentGridControl.SelectedRows[0] as Xceed.Grid.DataRow;
            if (row != null)
            {
               ComponentJump jump = ((ComponentJump)row.SourceObject);
               if (jump.map != null)
               {
                  mHostArea.SnapViewToItem(jump.map);
               }
               else
               {
                  mHostArea.SnapViewToItem(jump.comp);
               }
            }
         }
      }

      private void ComponentClearButton_Click(object sender, EventArgs e)
      {
         ComponentGridControl.DataRows.Clear();
         mComponentSearch = new ComponentSearch();
         ComponentBetterPropertyGrid.SelectedObject = mComponentSearch;
      }

      bool ValidateComponent(TriggerComponent comp)
      {
         if (mComponentSearch.Effect != null && mComponentSearch.Effect != "")
         {
            if (comp is TriggerEffect)
            {
               if (mComponentSearch.Effect != comp.Type)
                  return false;
            }
            else
            {
               return false;
            }
         }
         if (mComponentSearch.Condition != null && mComponentSearch.Condition != "")
         {
            if (comp is TriggerCondition)
            {
               if (mComponentSearch.Condition != comp.Type)
                  return false;
            }
            else
            {
               return false;
            }
         }
         return true;
      }

      private void ComponentSearchButton_Click(object sender, EventArgs e)
      {
         mbPaused = true;
         List<Trigger> triggers = mHostArea.CurrentTriggerNamespace.WalkTriggers();

         List<ComponentJump> found = new List<ComponentJump>();
         Dictionary<int, GroupUI> groups = mHostArea.CurrentTriggerNamespace.GetGroups();

         foreach (Trigger t in triggers)
         {
            GroupUI group = null;
            groups.TryGetValue(t.GroupID, out group);

            List<TriggerCondition> conditions = mHostArea.CurrentTriggerNamespace.WalkConditions(t);
            foreach (TriggerCondition c in conditions)
            {
               if (ValidateComponent(c) == true)
               {
                  found.Add(new ComponentJump(group, t, c, mHostArea.CurrentTriggerNamespace.getTemplateMapping(t.TemplateID)));
               }         
            }
            List<TriggerEffect> effects = mHostArea.CurrentTriggerNamespace.WalkEffects(t);
            foreach (TriggerEffect ef in effects)
            {
               if (ValidateComponent(ef) == true)
               {
                  found.Add(new ComponentJump(group, t, ef, mHostArea.CurrentTriggerNamespace.getTemplateMapping(t.TemplateID)));
               }   
            }  
         }
         ComponentGridControl.DataSource = found;
         ComponentGridControl.SelectedRows.Clear();

         mbPaused = false;
      }


   }
}
