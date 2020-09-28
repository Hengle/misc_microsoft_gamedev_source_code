using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

using SimEditor;
using EditorCore;
using PhoenixEditor.ScenarioEditor;

namespace PhoenixEditor
{
   public partial class WorldObjectList : Xceed.DockingWindows.ToolWindow // : Form
   {  
      public WorldObjectList()
      {
         InitializeComponent();
         TypeComboBox.SelectedIndex = 0;
         listView1.MouseDown += new MouseEventHandler(listView1_MouseDown);
         listView1.MouseUp += new MouseEventHandler(listView1_MouseUp);

         this.betterPropertyGrid1.AddMetaDataForProps("LightXML", new string[] { "Position", "Rotation","Direction" } , "Ignore", true);
         this.betterPropertyGrid1.AddMetaDataForProps("SimObjectData", new string[] { "Position", "Rotation" } , "Ignore", true);
         this.betterPropertyGrid1.AddMetaDataForProps("PlayerPositionXML", new string[] { "Position", "Rotation", "Forward" } , "Ignore", true);
         this.betterPropertyGrid1.AddMetaDataForProps("HelperAreaObject", new string[] { "Position", "Direction", "Color" } , "Ignore", true);
         this.betterPropertyGrid1.AddMetaDataForProps("HelperAreaBoxObject", new string[] { "Position", "Direction", "Color" } , "Ignore", true);
         this.betterPropertyGrid1.AddMetaDataForProps("GameDesignValueSphere", new string[] { "Position", "XMLColor" } , "Ignore", true);
         this.betterPropertyGrid1.AddMetaDataForProp("SimObjectData", "ID", "ReadOnly", true);
         this.betterPropertyGrid1.AddMetaDataForProp("HelperAreaObject", "ID", "ReadOnly", true);
         this.betterPropertyGrid1.AddMetaDataForProp("HelperAreaBoxObject", "ID", "ReadOnly", true);

         
         this.betterPropertyGrid1.SetTypeEditor("SimObjectData", "Group", typeof(EnumeratedProperty));
         this.betterPropertyGrid1.SetTypeEditor("HelperAreaObject", "Group", typeof(EnumeratedProperty));
         this.betterPropertyGrid1.SetTypeEditor("HelperAreaBoxObject", "Group", typeof(EnumeratedProperty));
         this.betterPropertyGrid1.SetTypeEditor("GameDesignValueSphere", "Group", typeof(EnumeratedProperty));


         this.betterPropertyGrid1.AddMetaDataForProp("SimObjectData", "Department", "ReadOnly", true);
         this.betterPropertyGrid1.AddMetaDataForProp("SimObjectData", "Department", "StringIntEnumeration", EnumUtils.EnumToPairList(typeof(eDepartment)));
         this.betterPropertyGrid1.SetTypeEditor("SimObjectData", "Department", typeof(EnumeratedProperty));


         //this.betterPropertyGrid1.AddMetaDataForProp("GameDesignValueSphere", "ID", "ReadOnly", true);
         //this.betterPropertyGrid1.AddMetaDataForProp("GameDesignValueSphere", "DesignerData", "Ignore", true);
         this.betterPropertyGrid1.SetTypeEditor("GameDesignValueSphere", "DesignerData", typeof(DesignerDataEditorProperty));
         this.betterPropertyGrid1.AddMetaDataForProp("GameDesignValueSphere", "ID", "ReadOnly", true);
         this.betterPropertyGrid1.SetTypeEditor("GameDesignLine", "DesignerData", typeof(DesignerDataEditorProperty));
         this.betterPropertyGrid1.AddMetaDataForProp("GameDesignLine", "ID", "ReadOnly", true);
         this.betterPropertyGrid1.SetTypeEditor("GameDesignValuePoint", "DesignerData", typeof(DesignerDataEditorProperty));
         this.betterPropertyGrid1.AddMetaDataForProp("GameDesignValuePoint", "ID", "ReadOnly", true);

         this.betterPropertyGrid1.AddMetaDataForProp("SimObjectData", "TintValue", "Min",0.0f);
         this.betterPropertyGrid1.AddMetaDataForProp("SimObjectData", "TintValue", "Max", 1.0f);



         //this.betterPropertyGrid1
         betterPropertyGrid1.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(betterPropertyGrid1_AnyPropertyChanged);


         try
         {

            FileStream f = new FileStream(CoreGlobals.getWorkPaths().mEditorSettings + "\\LocalLightUISettings.xml", FileMode.Open, FileAccess.Read);
            this.betterPropertyGrid1.LoadSettingsFromStream(f);
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().SendToErrorWarningViewer(ex.ToString());
         }
      }

      void betterPropertyGrid1_AnyPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         this.mbPropertiesChanged = true;
         //throw new Exception("The method or operation is not implemented.");
      }

      bool mbValidSelectionState = false;
      void listView1_MouseUp(object sender, MouseEventArgs e)
      {
         mbValidSelectionState = true;
         WorldObjectListRefresh();
      }

      void listView1_MouseDown(object sender, MouseEventArgs e)
      {
         mbValidSelectionState = false;
      }

      private void RefreshButton_Click(object sender, EventArgs e)
      {
         List<EditorObject> objects = SimGlobals.getSimMain().mEditorObjects;
         PopulateListBox(objects);
      }

      List<EditorObject> mObjectsInList = new List<EditorObject>();
      private void PopulateListBox(List<EditorObject> objects)
      {
         mObjectsInList.Clear();
         foreach (EditorObject obj in objects)
         {
            if (obj is ISubControl)
               continue;

            if(mFilterSettings.ContainsKey(obj.ToString()) == false)
            {
               mFilterSettings[obj.ToString()] = true;
            }
         }


         listView1.Clear();
         for (int i = 0; i < objects.Count; i++)
         {
            if (objects[i] is ISubControl)
               continue;

            ObjectListItemWrapper item = new ObjectListItemWrapper(objects[i], ShowNamesCheckBox.Checked);
            if (!listView1.Items.Contains(item))
            {

               if(mFilterSettings[objects[i].ToString()] == true)
               {
                  mObjectsInList.Add(objects[i]);
                  listView1.Items.Add(item);
               }
            }

         }
         /*
            if (listView1.Items.Count > 0)
            {
               List<int> itemCodes = new List<int>();
               foreach (ObjectListItemWrapper item in listView1.Items)
               {
                  itemCodes.Add(item.mEditorObject.GetHashCode());
               }
               foreach (EditorObject obj in objects)
               {
                  if (itemCodes.Remove(obj.GetHashCode()) == false)
                  {

                     break;
                  }
               }
               if ((itemCodes.Count == 0))
                  return;
            }
         if (listView1.Items.Count == 0 && objects.Count == 0)
            return;

         listView1.Clear();
         foreach (EditorObject obj in objects)
         {
            ObjectListItemWrapper item = new ObjectListItemWrapper(obj);
            listView1.Items.Add(item);
         }*/
         
         

      }
      public bool mbPauseListUpdate = false;


      //Wow listview sucks
      private void listView1_SelectedIndexChanged(object sender, EventArgs e)
      {

         //if (mbPauseListUpdate || !mbValidSelectionState) return;

         ////SimGlobals.getSimMain().selectItems().Clear();
         
         ////System.Threading.Thread.Sleep(50);

         
         //SimGlobals.getSimMain().mSelectedEditorObjects.Clear();
         //foreach(ObjectListItemWrapper item in listView1.SelectedItems)
         //{
         //   if(item.mEditorObject != null)
         //      SimGlobals.getSimMain().addSelectedObject(item.mEditorObject);//.mSelectedEditorObjects.Add(item.mEditorObject);
         //}
         //SetProperties();



      }

      private void WorldObjectListRefresh()
      {
         if (mbPauseListUpdate || !mbValidSelectionState) return;

         SimGlobals.getSimMain().mSelectedEditorObjects.Clear();
         foreach (ObjectListItemWrapper item in listView1.SelectedItems)
         {
            if (item.mEditorObject != null)
               SimGlobals.getSimMain().addSelectedObject(item.mEditorObject);//.mSelectedEditorObjects.Add(item.mEditorObject);
         }
         SetProperties();
      }

      




      protected class ObjectListItemWrapper : ListViewItem
      {
         public EditorObject mEditorObject = null;
         public ObjectListItemWrapper(EditorObject obj, bool useObjectName)
         {
            mEditorObject = obj;

            if (mEditorObject == null)
            {
               Text = "NULL OBJECT ERROR!!";
               return;
            }

            string displayValue;
            if (useObjectName == true )
            {
               Text = obj.Name;
            }
            else
            {
               Text = mEditorObject.ToString();
            }
         }
      }

      private void listView1_DoubleClick(object sender, EventArgs e)
      {
         if(listView1.SelectedItems.Count >= 1)
         {
           ObjectListItemWrapper item =  listView1.SelectedItems[0] as ObjectListItemWrapper;

           SimGlobals.getSimMain().LookAtObject(item.mEditorObject);
         }
      }

      Dictionary<string, bool> mFilterSettings = new Dictionary<string, bool>();
      List<EditorObject> mLastVisibleObjects = new List<EditorObject>();
      private void updateListFromSim()
      {
         

         int playerFilter = -1;
         if (PlayerFilterComboBox.SelectedItem != null && PlayerFilterComboBox.SelectedItem.ToString() != "All")
         {
            int.TryParse(PlayerFilterComboBox.SelectedItem.ToString(), out playerFilter);
         }

         List<EditorObject> visibleObjects = SimGlobals.getSimMain().getEditorObjects(ListVisibleCheckBox.Checked, (SimMain.eFilterTypes)TypeComboBox.SelectedIndex, playerFilter, onlyRenderList.Checked);
         if (!CompareObjectLists(visibleObjects, mLastVisibleObjects))
         {        
    

            mLastVisibleObjects = visibleObjects;
            PopulateListBox(visibleObjects);
            mLastSelectedObjects.Clear();
         }
      }

      private bool CompareObjectLists(List<EditorObject> listA, List<EditorObject> listB)
      {
         if (listA.Count == listB.Count)
         {
            long hashA = 0;
            long hashB = 0;
            foreach (EditorObject obj in listA)
            {
               hashA += obj.GetHashCode();
            }
            foreach (EditorObject obj in listB)
            {
               hashB += obj.GetHashCode();
            }
            if (hashA == hashB)
               return true;
         }
         return false;
       }


      List<EditorObject> mLastSelectedObjects = new List<EditorObject>();

      bool mbPropertiesChanged = false;

      private void updateSelectionFromSim()
      {

         if (CompareObjectLists(SimGlobals.getSimMain().mSelectedEditorObjects, mLastSelectedObjects))
         {
            return;//nothing changed
         }


         mLastSelectedObjects.Clear();
         mLastSelectedObjects.AddRange(SimGlobals.getSimMain().mSelectedEditorObjects);

         if (mbPropertiesChanged)
         {
            int playerFilter = -1;
            if (PlayerFilterComboBox.SelectedItem != null && PlayerFilterComboBox.SelectedItem.ToString() != "All")
            {
               int.TryParse(PlayerFilterComboBox.SelectedItem.ToString(), out playerFilter);
            }

            mbPropertiesChanged = false;
            List<EditorObject> visibleObjects = SimGlobals.getSimMain().getEditorObjects(ListVisibleCheckBox.Checked, (SimMain.eFilterTypes)TypeComboBox.SelectedIndex, playerFilter, onlyRenderList.Checked);
            PopulateListBox(visibleObjects);
         }

         listView1.Focus();
         mbPauseListUpdate = true;
         listView1.BeginUpdate();
         listView1.SelectedItems.Clear();
         foreach (ObjectListItemWrapper item in listView1.Items)
         {
            if (SimGlobals.getSimMain().mSelectedEditorObjects.Contains(item.mEditorObject))
            {
               item.Selected = true;
            }
         }
         listView1.EndUpdate();
         mbPauseListUpdate = false;
      }

      private void TypeComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         updateListFromSim();
         MainWindow.mMainWindow.SetClientFocus();
      }
      private void PlayerFilterComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         updateListFromSim();
         MainWindow.mMainWindow.SetClientFocus();
      }

      bool mbLastDepartmentPermission = false;
      int mLastDepartementMode = 0;
      private void timer1_Tick(object sender, EventArgs e)
      {
         timer1.Interval = 250;
         if (SimGlobals.getSimMain().EditorListChanged() || ListVisibleCheckBox.Checked)
         {
            
            updateListFromSim();
            updateSelectionFromSim();
         }
         else
         {
            updateSelectionFromSim(); 
         }

         if (SimGlobals.getSimMain().AllowDepartmentChanges != mbLastDepartmentPermission)
         {
            mbLastDepartmentPermission = SimGlobals.getSimMain().AllowDepartmentChanges;

            betterPropertyGrid1.AddMetaDataForProp("SimObjectData", "Department", "ReadOnly", !mbLastDepartmentPermission);
            betterPropertyGrid1.SelectedObjects = betterPropertyGrid1.SelectedObjects;
         }

         if (mLastDepartementMode != SimGlobals.getSimMain().GetCurrentDepartmentMode())
         {
            mLastDepartementMode = SimGlobals.getSimMain().GetCurrentDepartmentMode();
            updateListFromSim();
         }

         //else if (SimGlobals.getSimMain().selectedListChanged())
         //{
         //   updateSelectionFromSim();
         //   SetProperties();
         //}
         
      }
      private void PropertiesChangedTimer_Tick(object sender, EventArgs e)
      {
         if (SimGlobals.getSimMain().selectedListChanged())
         {
            updateSelectionFromSim();
            SetProperties();
         }
      }

      private void BBToggleCheckBox_CheckedChanged(object sender, EventArgs e)
      {

         SimGlobals.getSimMain().ToggleShowBB(BBToggleCheckBox.Checked);
      }

      private void listView1_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      private void listView1_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      public void SetProperties()
      {
         List<object> properties = new List<object>();

         bool bOmniPresent = false;
         foreach (EditorObject obj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            if (obj is SpotLight && !bOmniPresent)
            {
               this.betterPropertyGrid1.AddMetaDataForProps("LightXML", new string[] { "InnerAngle", "OuterAngle" } , "Ignore", false);
            }
            else if (obj is OmniLight)
            {
               bOmniPresent = true;
               this.betterPropertyGrid1.AddMetaDataForProps("LightXML", new string[] { "InnerAngle", "OuterAngle" } , "Ignore", true);
            }

            object props = obj.GetProperties();
            if (props != null)
               properties.Add(props);
         }

         //SimGlobals.getSimMain().ObjectGroups
         Pair<List<int>, List<string>> entries = new Pair<List<int>, List<string>>();
         entries.Key = new List<int>();
         entries.Value = new List<string>();
         entries.Key.Add(-1);
         entries.Value.Add("Default");
         foreach(ObjectGroup g in SimGlobals.getSimMain().ObjectGroups)
         {
            entries.Key.Add(g.ID);
            entries.Value.Add(g.Name);
         }

         betterPropertyGrid1.AddMetaDataForProp("SimObjectData", "Group", "StringIntEnumeration", entries);
         betterPropertyGrid1.AddMetaDataForProp("HelperAreaObject", "Group", "StringIntEnumeration", entries);
         betterPropertyGrid1.AddMetaDataForProp("HelperAreaBoxObject", "Group", "StringIntEnumeration", entries);
         betterPropertyGrid1.AddMetaDataForProp("GameDesignValueSphere", "Group", "StringIntEnumeration", entries);

    
         //if(properties.Count > 0)
         //this.propertyGrid1.SelectedObjects = properties.ToArray();

         if (properties.Count > 0)
         {
            this.betterPropertyGrid1.SelectedObjects = properties.ToArray();
         }

         else
         {
            this.betterPropertyGrid1.SelectedObject = null;
         }
         //ListView.SelectedIndexCollection lst = listView1.SelectedIndices;
         updateListFromSim();
    //     for (int i = 0; i < lst.Count;i++ )
     //       listView1.SelectedIndices.Add(lst[i]);
      }

      private void ListVisibleCheckBox_CheckedChanged(object sender, EventArgs e)
      {
      }

      private void onlyRenderList_CheckedChanged(object sender, EventArgs e)
      {
         //updateListFromSim();
         if (onlyRenderList.Checked == true)
         {
            SimGlobals.getSimMain().setVisibleObjects(mObjectsInList);
         }
         else
         {
            SimGlobals.getSimMain().setVisibleObjects(SimGlobals.getSimMain().mEditorObjects);
         }

         MainWindow.mMainWindow.SetClientFocus();

      }
      private void LockByListCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         //updateListFromSim();
         if (LockByListCheckBox.Checked == true)
         {

            SimGlobals.getSimMain().setSelectableObjects(mObjectsInList);
         }
         else
         {
            SimGlobals.getSimMain().setSelectableObjects(SimGlobals.getSimMain().mEditorObjects);
         }
         MainWindow.mMainWindow.SetClientFocus();

      }
      PopupEditor mPopupfilter = new PopupEditor();
      CheckedListBox mFilterSettingsList = null;
      TreeView mFilterTree = new TreeView();
      Dictionary<string, bool> mCategoryFilterSettings = new Dictionary<string, bool>();
      private void ListFilterButton_Click(object sender, EventArgs e)
      {

         if (mPopupfilter.mbOpen == false)
         {
            mFilterSettingsList = new CheckedListBox();
            mFilterSettingsList.Width = 200;
            mFilterSettingsList.Height = 300;
            mFilterSettingsList.CheckOnClick = true;
            mFilterSettingsList.Sorted = true;
            if (mCategoryFilterSettings.ContainsKey("_Check ALL") == false)
               mCategoryFilterSettings["_Check ALL"] = true;
            mFilterSettingsList.Items.Add("_Check ALL", mCategoryFilterSettings["_Check ALL"]);
            Dictionary<string, bool>.Enumerator it = mFilterSettings.GetEnumerator();
            Dictionary<string, bool> categories = new Dictionary<string, bool>();

            while (it.MoveNext())
            {
               string[] category = it.Current.Key.Split('_');
               string accumulator = "";
               for (int i = 0; i < category.Length; i++)
               {
                  int notused;
                  if ((category.Length > 2) && (i == (category.Length - 2)) && (int.TryParse(category[i + 1], out notused) == true))
                  {
                     break;
                  }
                  if (accumulator != "")
                     accumulator += "_";
                  accumulator += category[i];
                  if (categories.ContainsKey(accumulator) == false)
                  {
                     categories[accumulator] = true;

                     if (mCategoryFilterSettings.ContainsKey(accumulator) == false)
                        mCategoryFilterSettings[accumulator] = true;
                     mFilterSettingsList.Items.Add(accumulator, mCategoryFilterSettings[accumulator]);


                  }
               }
            }
            it = mFilterSettings.GetEnumerator();
            while (it.MoveNext())
            {
               if (mCategoryFilterSettings.ContainsKey(it.Current.Key) == false)
               {
                  mFilterSettingsList.Items.Add(it.Current.Key, it.Current.Value);
               }
            }
            mFilterSettingsList.ItemCheck += new ItemCheckEventHandler(mFilterSettingsList_ItemCheck);
            mPopupfilter.ShowPopup(this, mFilterSettingsList, FormBorderStyle.SizableToolWindow);
         }

         //if (mPopupfilter.mbOpen == false)
         //{
         //   mFilterTree.CheckBoxes = true;
         //   mFilterTree.Nodes.Clear();

         //   mFilterTree.Nodes.Add("All");


         //}
      }

      bool mbDontUpdateFilter = false;

      void mFilterSettingsList_ItemCheck(object sender, ItemCheckEventArgs e)
      {

         if (mbDontUpdateFilter) return;

         mbDontUpdateFilter = true;

         String name = mFilterSettingsList.Items[e.Index].ToString();

         if (name == "_Check ALL")
         {
            Dictionary<string, bool>.Enumerator it = mFilterSettings.GetEnumerator();
            List<string> whatTheF = new List<string>();
            whatTheF.AddRange(mFilterSettings.Keys);
            bool state = (e.NewValue == CheckState.Checked) ? true : false;
            foreach (string keyName in whatTheF)
            {

               mFilterSettings[keyName] = state;
            }
            mCategoryFilterSettings["_Check ALL"] = state;
            whatTheF.Clear(); whatTheF.AddRange(mCategoryFilterSettings.Keys);
            foreach (string keyName in whatTheF)
            {
               mCategoryFilterSettings[keyName] = state;               
            }
            for (int i = 0; i < mFilterSettingsList.Items.Count; i++)
            {
               mFilterSettingsList.SetItemChecked(i, state);
            }

         }
         else if (mFilterSettings.ContainsKey(name) == false) // category
         {
            Dictionary<string, bool>.Enumerator it = mFilterSettings.GetEnumerator();
            List<string> whatTheF = new List<string>();
            whatTheF.AddRange(mFilterSettings.Keys);
            bool state = (e.NewValue == CheckState.Checked) ? true : false;
            foreach (string keyName in whatTheF)
            {
               if (keyName.StartsWith(name))
               {
                  mFilterSettings[keyName] = state;
               }
            }
            whatTheF.Clear(); whatTheF.AddRange(mCategoryFilterSettings.Keys);
            foreach (string keyName in whatTheF)
            {
               if (keyName.StartsWith(name))
               {
                  mCategoryFilterSettings[keyName] = state;
               }
            }
            for (int i = 0; i < mFilterSettingsList.Items.Count; i++)
            {
               string find = mFilterSettingsList.Items[i].ToString();
               if (mFilterSettings.ContainsKey(find))
               {
                  mFilterSettingsList.SetItemChecked(i, mFilterSettings[find]);
               }
               if(mCategoryFilterSettings.ContainsKey(find))
               {
                  mFilterSettingsList.SetItemChecked(i, mCategoryFilterSettings[find]);
               }
            }

            //mFilterSettings[name] = (e.NewValue == CheckState.Checked) ? true : false;
         }
         else
         {
            mFilterSettings[name] = (e.NewValue == CheckState.Checked) ? true : false;

         }
         PopulateListBox(mLastVisibleObjects);

         mbDontUpdateFilter = false;

         
         CoreGlobals.getEditorMain().mOneFrame = true;
      }

      private void ShowNamesCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         PopulateListBox(mLastVisibleObjects);
      }

      PopupEditor mPopupGroups = new PopupEditor();

      private void GroupButton_Click(object sender, EventArgs e)
      {
         if (mPopupGroups.mbOpen == false)
         {
            BasicTypedSuperList basicList = new BasicTypedSuperList();

            basicList.AutoScroll = true;
            basicList.AddMetaDataForProp("ObjectGroup", "ID", "ReadOnly", true);

            basicList.ObjectList = SimGlobals.getSimMain().ObjectGroups;
            basicList.mListDataObjectType = typeof(ObjectGroup);
            basicList.WrapContents = false;
            basicList.Width += 50;

            basicList.NewObjectAdded += new BasicTypedSuperList.ObjectChanged(basicList_NewObjectAdded);
            basicList.ObjectDeleted += new BasicTypedSuperList.ObjectChanged(basicList_ObjectDeleted);
            basicList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(basicList_AnyObjectPropertyChanged);

            mPopupGroups.ShowPopup(this, basicList, FormBorderStyle.SizableToolWindow);
         }
      }

      void basicList_AnyObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         CoreGlobals.getEditorMain().mOneFrame = true;
      }

      void basicList_ObjectDeleted(ObjectEditorControl sender, object selectedObject)
      {
         SimGlobals.getSimMain().ClearGroupCache();
      }

      void basicList_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      {
         ((ObjectGroup)selectedObject).ID = SimGlobals.getSimMain().ObjectGroups.Count;
      }

      PlacementHelper mPlacementHelper = new PlacementHelper();
      PopupEditor mPlacementEditor = new PopupEditor();

      private void MovementButton_Click(object sender, EventArgs e)
      {
         if (mPlacementEditor.mbOpen == false)
         {
            mPlacementHelper = new PlacementHelper();
            mPlacementEditor.ShowPopup(this, mPlacementHelper);
         }         
      }

      private void RenderNamesCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().mShowNames = RenderNamesCheckBox.Checked;
         CoreGlobals.getEditorMain().mOneFrame = true;
      }




   }
}