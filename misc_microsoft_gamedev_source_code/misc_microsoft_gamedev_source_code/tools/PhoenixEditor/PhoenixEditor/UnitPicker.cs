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
   public partial class UnitPicker : Xceed.DockingWindows.ToolWindow //: Form
   {
      bool m_bLoaded = false;
      List<UISimObjectItem> mItems = new List<UISimObjectItem>();



      public UnitPicker()
      {
         InitializeComponent();
         SquadTreeView.Enabled = true;
      }

      public void DelayedLoad()
      {

         if (!m_bLoaded)
         {
            LoadListData();
            FilterComboBox.SelectedIndex = 0;
            PlayerIDComboBox.SelectedIndex = 1;

            PopulateListBox();

            LoadTreeData(this.UnitTreeView);
            LoadSquadTreeData(this.SquadTreeView);

            m_bLoaded = true;
         }
      }

      public void ReLoadData()
      {
         m_bLoaded = false;

         UnitTreeView.Nodes.Clear();
         UnitListBox.Items.Clear();
         mImageList.Images.Clear();

         DelayedLoad();
      }

      ImageList mImageList = new ImageList();
      public void LoadTreeDataFileCentric(TreeView treeView)
      {
         TreeNode root = new TreeNode("All");
         TreeNode noAnimFile = new TreeNode("No Anim File");
         root.Nodes.Add(noAnimFile);
         //mImageList.ImageSize = new System.Drawing.Size(32, 32);
         //mImageList.Images.Add(new Bitmap(32,32)) ;
         string artpath = CoreGlobals.getWorkPaths().mGameArtDirectory;
         foreach (SimUnitXML unit in SimGlobals.getSimMain().mSimFileData.mProtoObjectsXML.mUnits)
         {
            TreeNode n = new TreeNode();
            n.Text = unit.mName;
            n.Tag = unit;
            //Image i = unit.getImage();
            //if(i != null)
            //{
            //   n.ImageIndex = mImageList.Images.Count;
            //   n.SelectedImageIndex = mImageList.Images.Count;
            //   mImageList.Images.Add(i);
            //}

            if (unit.mAnimFile != null && unit.mAnimFile.Length > 0)
            {
               string folder = Path.GetDirectoryName(unit.mAnimFile);
               string[] folders = folder.Split('\\');

               string fullpath = artpath;

               TreeNode parentNode = root;
               for (int i = 0; i < folders.Length; i++ )
               {
                  string folderName = folders[i];
                  fullpath = Path.Combine(fullpath, folderName);

                  if(folderName == "falcon_01")
                  {
                     
                  }

                  int index = parentNode.Nodes.IndexOfKey(folderName);
                  if(index >= 0)
                  {
                     parentNode = parentNode.Nodes[index];
                  }
                  else
                  {
                     TreeNode oldparent = parentNode;
                     parentNode = new TreeNode(folderName);
                     parentNode.Name = folderName;

                     if (Directory.Exists(fullpath) == false)
                     {
                        parentNode.ForeColor = Color.Red;
                     }

                     oldparent.Nodes.Add(parentNode);
                  }

               }

               if (File.Exists(Path.Combine(artpath, unit.mAnimFile)) == false)
               {
                  n.ForeColor = Color.Red;
               }
               parentNode.Nodes.Add(n);

            }
            else
            {
               n.ForeColor = Color.Red;
               noAnimFile.Nodes.Add(n);
            }

         }
         //treeView.ImageList = mImageList;
         treeView.Nodes.Add(root);
         treeView.Sort();
         root.Expand();
      }


      public void LoadTreeData(TreeView treeView)
      {
         TreeNode root = new TreeNode("All");
         TreeNode noAnimFile = new TreeNode("No Anim File");
         root.Nodes.Add(noAnimFile);
         string artpath = CoreGlobals.getWorkPaths().mGameArtDirectory;
         foreach (SimUnitXML unit in SimGlobals.getSimMain().mSimFileData.mProtoObjectsXML.mUnits)
         {
            string[] namePaths = unit.mName.Split('_');

            TreeNode parentNode = root;

            int bonus = 0;
            int extension;
            if(int.TryParse(namePaths[namePaths.Length - 1], out extension))
            {
               bonus++;
            }

            string shortName = namePaths[namePaths.Length - (1 + bonus)];
            for (int i = bonus - 1; i >= 0; i--)
            {
               shortName += "_" + namePaths[namePaths.Length - (1 + i)];
            }

            TreeNode n = new TreeNode();
            n.Text = shortName;
            n.Tag = unit;
         

            for (int i = 0; i < namePaths.Length - (1 + bonus); i++)
            {
               string folderName = namePaths[i];

               int index = parentNode.Nodes.IndexOfKey(folderName);
               if (index >= 0)
               {
                  parentNode = parentNode.Nodes[index];
               }
               else
               {
                  TreeNode oldparent = parentNode;
                  parentNode = new TreeNode(folderName);
                  parentNode.Name = folderName;
                  oldparent.Nodes.Add(parentNode);
               }

            }

            if (unit.mAnimFile == null || unit.mAnimFile.Length == 0)
            {
               n.ForeColor = Color.Red;
            }
            else if (File.Exists(Path.Combine(artpath, unit.mAnimFile)) == false)
            {
               n.ForeColor = Color.Red;
            }
            else
            {
              // n.ToolTipText = Path.Combine(artpath, unit.mAnimFile);
            }

            parentNode.Nodes.Add(n);
         }
         //treeView.ImageList = mImageList;
         treeView.Nodes.Add(root);
         treeView.Sort();
         root.Expand();
      }

      public void LoadListData()
      {
 
         mImageList.ImageSize = new System.Drawing.Size(32, 32);
         mImageList.Images.Add(new Bitmap(32, 32));


         if (SimGlobals.getSimMain().mSimFileData.mProtoObjectsXML != null)
         {
            foreach (SimUnitXML unit in SimGlobals.getSimMain().mSimFileData.mProtoObjectsXML.mUnits)
            {
               UISimObjectItem n = new UISimObjectItem(unit);
               mItems.Add(n);
            }
         }
        
      }


      public class UISimObjectItem
      {
         public SimUnitXML mUnitXML;
         public UISimObjectItem(SimUnitXML unit)
         {
            mUnitXML = unit;
         }

         public override string ToString()
         {
            return mUnitXML.mName;
         }
      }

      public void PopulateListBox()
      {
         string search = FilterComboBox.Text;
         UnitListBox.Items.Clear();
         if (search == "All")
         {
            UnitListBox.Items.AddRange(mItems.ToArray());
         }

         else
         {
            foreach (UISimObjectItem n in mItems)
            {
               if (n.mUnitXML.mName.Contains(search))
               {
                  UnitListBox.Items.Add(n);
               }

            }
         }

      }
      private void FilterComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         PopulateListBox();
         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();

      }

      private void FilterComboBox_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void comboBox1_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void PlayerIDComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         IUnitPicker pickerInterface = CoreGlobals.getEditorMain().mIGUI.getActiveClientPage() as IUnitPicker;
         if (pickerInterface != null)
         {
            pickerInterface.PlayerIdSelectedChanged(System.Convert.ToInt32(PlayerIDComboBox.Text));
         }
         CoreGlobals.getEditorMain().mIGUI.SetClientFocus();
      }


      private void UnitTreeView_AfterSelect(object sender, TreeViewEventArgs e)
      {
         //if(e.Node.Tag != null)
         //{
         //   SimUnitXML unit = e.Node.Tag as SimUnitXML;

         //   if(unit != null)
         //   {
         //      SimGlobals.getSimMain().SelectedProtoUnit = unit;
         //      SimGlobals.getSimMain().PlaceItemMode();
         //   }
         //}
      }

      private void UnitTreeView_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      private void UnitTreeView_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void UnitPicker_Load(object sender, EventArgs e)
      {

      }

      private void tabControl1_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void UnitTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         if (e.Node.Tag != null)
         {
            SimUnitXML unit = e.Node.Tag as SimUnitXML;

            if (unit != null)
            {
               IUnitPicker pickerInterface = CoreGlobals.getEditorMain().mIGUI.getActiveClientPage() as IUnitPicker;
               if (pickerInterface != null)
               {
                  pickerInterface.UnitSelectedChanged(unit);
               }
            }
         }
      }

      private void UnitListBox_SelectedIndexChanged(object sender, EventArgs e)
      {
         if (UnitListBox.SelectedItems.Count == 1)
         {
            SimUnitXML unit = ((UISimObjectItem)UnitListBox.SelectedItems[0]).mUnitXML;

            if (unit != null)
            {
               IUnitPicker pickerInterface = CoreGlobals.getEditorMain().mIGUI.getActiveClientPage() as IUnitPicker;
               if (pickerInterface != null)
               {
                  pickerInterface.UnitSelectedChanged(unit);
               }
            }
         }
      }

      private void UnitListBox_Click(object sender, EventArgs e)
      {
         if (UnitListBox.SelectedItems.Count == 1)
         {
            SimUnitXML unit = ((UISimObjectItem)UnitListBox.SelectedItems[0]).mUnitXML;

            if (unit != null)
            {
               IUnitPicker pickerInterface = CoreGlobals.getEditorMain().mIGUI.getActiveClientPage() as IUnitPicker;
               if (pickerInterface != null)
               {
                  pickerInterface.UnitSelectedChanged(unit);
               }
            }
         }
      }

      private void UnitListBox_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      private void UnitListBox_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         advancedBrushBox.Enabled = checkBox1.Checked;
         SimGlobals.getSimMain().mPlaceItemMultipleMode = checkBox1.Checked;
      }


      private void checkBox3_CheckedChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().mAllowBoundsOverlapPlacement = checkBox3.Checked;
      }

      private void numericUpDown1_ValueChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().mMultipleFillPercent = (float)numericUpDown1.Value;
      }

      private void numericUpDown2_ValueChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().mMultipleMaxRotation = (float)numericUpDown2.Value;
      }

      private void numericUpDown3_ValueChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().mMultipleRadius = (float)numericUpDown3.Value;
      }



      public void LoadSquadTreeData(TreeView treeView)
      {
         if (SimGlobals.getSimMain().mSimFileData.mProtoSquadsXML == null)
            return;

         TreeNode root = new TreeNode("All");
         //TreeNode noAnimFile = new TreeNode("No Anim File");
         //root.Nodes.Add(noAnimFile);
         string artpath = CoreGlobals.getWorkPaths().mGameArtDirectory;
         foreach (ProtoSquadXml squad in SimGlobals.getSimMain().mSimFileData.mProtoSquadsXML.mSquads)
         {
            string[] namePaths = squad.Name.Split('_');

            TreeNode parentNode = root;

            int bonus = 0;
            int extension;
            if (int.TryParse(namePaths[namePaths.Length - 1], out extension))
            {
               bonus++;
            }

            string shortName = namePaths[namePaths.Length - (1 + bonus)];
            for (int i = bonus - 1; i >= 0; i--)
            {
               shortName += "_" + namePaths[namePaths.Length - (1 + i)];
            }

            TreeNode n = new TreeNode();
            n.Text = shortName;
            n.Tag = squad;


            for (int i = 0; i < namePaths.Length - (1 + bonus); i++)
            {
               string folderName = namePaths[i];

               int index = parentNode.Nodes.IndexOfKey(folderName);
               if (index >= 0)
               {
                  parentNode = parentNode.Nodes[index];
               }
               else
               {
                  TreeNode oldparent = parentNode;
                  parentNode = new TreeNode(folderName);
                  parentNode.Name = folderName;
                  oldparent.Nodes.Add(parentNode);
               }

            }

            //if (unit.mAnimFile == null || unit.mAnimFile.Length == 0)
            //{
            //   n.ForeColor = Color.Red;
            //}
            //else if (File.Exists(Path.Combine(artpath, unit.mAnimFile)) == false)
            //{
            //   n.ForeColor = Color.Red;
            //}
            //else
            //{
            //   // n.ToolTipText = Path.Combine(artpath, unit.mAnimFile);
            //}

            parentNode.Nodes.Add(n);
         }
         //treeView.ImageList = mImageList;
         treeView.Nodes.Add(root);
         treeView.Sort();
         root.Expand();
      }

      private void SquadTreeView_KeyDown(object sender, KeyEventArgs e)
      {
         e.Handled = true;
      }

      private void SquadTreeView_KeyPress(object sender, KeyPressEventArgs e)
      {
         e.Handled = true;
      }

      private void SquadTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         if (e.Node.Tag != null)
         {
            ProtoSquadXml squad = e.Node.Tag as ProtoSquadXml;

            if (squad != null)
            {
               ISquadPicker pickerInterface = CoreGlobals.getEditorMain().mIGUI.getActiveClientPage() as ISquadPicker;
               if (pickerInterface != null)
               {
                  pickerInterface.SquadSelectedChanged(squad);
               }
            }
         }
      }

      private void label1_Click(object sender, EventArgs e)
      {
         if(ModifierKeys == Keys.Shift)
            SquadTreeView.Enabled = true;
      }

      List<ObjectPlacementSettings> mObjectPlacementSettings = new List<ObjectPlacementSettings>();
      private void button1_Click(object sender, EventArgs e)
      {
         if (CoreGlobals.getEditorMain().mPhoenixScenarioEditor.CheckTopicPermission("Sim") == false)
         {
            return;
         }

         string res = SimGlobals.getSimMain().placeItemsToMask(mObjectPlacementSettings);
         if (res != "")
         {
            MessageBox.Show(res);
         }
      }

      private void FillToMaskSettingsButton_Click(object sender, EventArgs e)
      {

         PopupEditor pe = new PopupEditor();         
         BasicTypedSuperList listbox = new BasicTypedSuperList();
         listbox.AutoScroll = true;
         listbox.AddMetaDataForProps("ObjectPlacementSettings", new string[] { "MinMaskRange", "MaxMaskRange", "MidMaskRange" }, "Min", 0);
         listbox.AddMetaDataForProps("ObjectPlacementSettings", new string[] { "MinMaskRange", "MaxMaskRange", "MidMaskRange" }, "Max", 1);
         listbox.AddMetaDataForProps("ObjectPlacementSettings", new string[] { "NumberToPlace", "UseTerrainSlope" }, "Min", 0);
         listbox.AddMetaDataForProps("ObjectPlacementSettings", new string[] { "NumberToPlace", "UseTerrainSlope" }, "Max", 100);

         listbox.AddMetaDataForProp("ObjectPlacementSettings", "Object", "SimpleEnumeration", TriggerSystemMain.mSimResources.mProtoObjectData.mProtoObjectList.ToArray());
         listbox.SetTypeEditor("ObjectPlacementSettings", "Object", typeof(EnumeratedProperty));

         listbox.AddMetaDataForProp("ObjectPlacementSettings", "Object", "EditorProperty.Width", 200);
         listbox.AddMetaDataForProps("ObjectPlacementSettings", new string[] { "MinMaskRange", "MaxMaskRange", "MidMaskRange", "NumberToPlace", "UseTerrainSlope" }, "EditorProperty.Width", 150);

         listbox.mListDataObjectType = typeof(ObjectPlacementSettings);
         listbox.ObjectList = mObjectPlacementSettings;
         listbox.Width = 1300;
         pe.ShowPopup(this, listbox, FormBorderStyle.SizableToolWindow);
      }



      private void SaveFillSettingsButton_Click(object sender, EventArgs e)
      {
         if (mObjectPlacementSettings.Count == 0)
            return;
         try
         {
            SaveFileDialog d = new SaveFileDialog();
            d.Filter = "Random Environment (*.renv)|*.renv";
            //d.InitialDirectory = pr
            if (d.ShowDialog() == DialogResult.OK)
            {
               ObjectPlacementSettingsFile.Save(d.FileName, mObjectPlacementSettings);
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void LoadFillSetttingsButton_Click(object sender, EventArgs e)
      {
         try
         {
            OpenFileDialog d = new OpenFileDialog();
            d.Filter = "Random Environment (*.renv)|*.renv";
            //d.InitialDirectory = GetInitialDirectory();
            if (d.ShowDialog() == DialogResult.OK)
            {
               mObjectPlacementSettings = ObjectPlacementSettingsFile.Load(d.FileName);
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }

      private void selectEnvironmentComboBox_SelectedIndexChanged(object sender, EventArgs e)
      {

      }

      private void checkBox2_CheckedChanged(object sender, EventArgs e)
      {
         SimGlobals.getSimMain().mPaintObjectVariations = checkBox2.Checked;
      }
      

   }
   


}