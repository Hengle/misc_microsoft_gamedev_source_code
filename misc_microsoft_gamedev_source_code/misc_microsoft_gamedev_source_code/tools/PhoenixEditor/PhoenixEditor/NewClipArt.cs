using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;

using Terrain;
using EditorCore;

using Xceed.SmartUI;
using Xceed.SmartUI.Controls;
using Xceed.SmartUI.Controls.OptionList;
using Xceed.SmartUI.Controls.TreeView;
using Xceed.SmartUI.Controls.ToolBar;
using Xceed.SmartUI.Controls.CheckedListBox;

namespace PhoenixEditor
{
   public partial class NewClipArt : Form
   {
      string mAutoSaveFileName = "";
      string mTags = "";
      string mFolder = "";

      public NewClipArt()
      {
         InitializeComponent();
         string path = CoreGlobals.getWorkPaths().mClipArt;

         InitTreeHandlers(smartTreeView1);
        
        
         DoUpdateUI();
         subfolderPicker1.RootFolder = path;
         subfolderPicker1.FolderSelected += new EventHandler(subfolderPicker1_FolderSelected);


         if (CoreGlobals.UsingPerforce == false)
            MarkForAddCheckBox.Enabled = false;
      }

      void subfolderPicker1_FolderSelected(object sender, EventArgs e)
      {
         DoUpdateUI();
         //throw new Exception("The method or operation is not implemented.");
      }
      ClipArtData mData = null;
      public void SetClipArtData(ClipArtData data)
      {
         mData = data;
         pictureBox1.Image = mData.mThumbnail;
         setExportAll();
         
      }
      private void Cancelbutton_Click(object sender, EventArgs e)
      {

      }

      private void InitTreeHandlers(SmartTreeView treeview)
      {
         List<SmartItem> itemList = FlattenTree(treeview.Items.GetFirstItem(SmartItemState.Any));
         foreach (SmartItem item in itemList)
         {
            item.Click += new SmartItemClickEventHandler(item_Click);
            if (item is TextBoxTool)
            {
             
               TextBoxTool tb = (TextBoxTool)item;
               tb.VirtualTextBox.TextChanged += new EventHandler(VirtualTextBox_TextChanged);
            }
         }
      }

      void VirtualTextBox_TextChanged(object sender, EventArgs e)
      {
         DoUpdateUI();
      }

      void item_Click(object sender, SmartItemClickEventArgs e)
      {
         DoUpdateUI();         
      }



      private void DoUpdateUI()
      {
         string autoname = "";
         List<string> metadata = GetMetaDataFromTree(this.smartTreeView1, out autoname);
         mTags = "";
         foreach(string s in metadata)
            mTags+=(s + ",");

         mFolder = subfolderPicker1.SelectedPath;

         if (metadata.Count == 0)
         {
            QuickSavebutton.Enabled = false;
            //QuickSavebutton.Text = "---pick options firs---";
            FileNameLabel.Text = "Please choose metadata options first.";
            FileNameLabel.ForeColor = Color.Red;
         }
         else
         {
            QuickSavebutton.Enabled = true;
            mAutoSaveFileName = buildName(autoname, mFolder);
            //QuickSavebutton.Text = subfolderPicker1.RelativePath + "\\" + Path.GetFileName(mAutoSaveFileName); 
            //QuickSavebutton.ForeColor = Color.Green;

            FileNameLabel.Text = subfolderPicker1.RelativePath + "\\" + Path.GetFileName(mAutoSaveFileName);
            FileNameLabel.ForeColor = Color.Green;
         }
      }

      string buildName(string quickname, string folder)
      {
         
         int number = DateTime.Now.Millisecond;
         string name = Path.Combine(folder, quickname + number.ToString() + ".esd");
         while(File.Exists(name) == true)
         {
            number++;
            name = Path.Combine(folder, quickname + number.ToString() + ".esd");
         }

         return name;
      }


      List<SmartItem> FlattenTree(SmartItem root)
      {
         List<SmartItem> itemList = new List<SmartItem>();
         itemList.Add(root);
         SmartItem item = root.Items.GetFirstItem(SmartItemState.Any);
         while (item != null)
         {
            itemList.AddRange(FlattenTree(item).ToArray());
            item = root.Items.GetNextItem(item, SmartItemState.Any);
         }
         return itemList;
      }

      public List<string> GetMetaDataFromTree(SmartTreeView treeview, out string quickName)
      {
         List<string> metadata = new List<string>();
         StringWriter b = new StringWriter();
         List<SmartItem> itemList = FlattenTree(treeview.Items.GetFirstItem(SmartItemState.Any));

         string tagName = "";
         string mainName = "";
         quickName = "";
         foreach (SmartItem item in itemList)
         {
            if (item is CheckedListBoxItem )
            {
               if (((CheckedListBoxItem)item).Checked == true)
               {
                  metadata.Add(item.Tag.ToString());
               }
            }
            if (item is RadioButtonNode)
            {

               if (((RadioButtonNode)item).Checked == true)
               {
                  metadata.Add(item.Tag.ToString());
                  mainName = item.Tag.ToString();
               }
            }
            if (item is TextBoxTool)
            {
               if (((TextBoxTool)item).Text != "")
               {
                  TextBoxTool tb = (TextBoxTool)item;

                  if (tb.VirtualTextBox.Text != "")
                  { 
                     metadata.Add(tb.VirtualTextBox.Text.ToString());

                     if (tb.Text == "Tag")
                     {
                        tagName = tb.VirtualTextBox.Text;

                     }
                  }

               }
               
            }
         }

         //build a quickname
         int names = 0;
         int maxNames = 4;
         if (tagName != "")
         {
            quickName += tagName;
            names++;
         }
         if (mainName != "")
         {
            quickName += mainName;
            names++;
         }
         foreach(string s in metadata)
         {
            if(names <= maxNames && s != tagName && s != mainName)
            {
               quickName += s;
               names++;
            }
         }
         return metadata;
      }



      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         SaveFileDialog d = new SaveFileDialog();
         d.InitialDirectory = CoreGlobals.getSaveLoadPaths().mClipArt;
         d.Filter = "ES ClipArt files (*.esd)|*.esd";
         d.FilterIndex = 0;

         if(d.ShowDialog() == DialogResult.OK)
         {
            mData.mFileName =  d.FileName;
            Save();
            this.DialogResult = DialogResult.OK;
            this.Close();
         }
      }

      private void QuickSavebutton_Click(object sender, EventArgs e)
      {

         mData.mFileName = mAutoSaveFileName;
         
         Save();
      }

      private void Save()
      {
         try
         {
            mData.mMetadata.mTags = mTags;//
            mData.mMetadata.mSourceControl = (this.MarkForAddCheckBox.Checked == true) ? "InPerforce" : "";
            mData.mMetadata.mCreator = System.Environment.UserName;
            //mData.mMetadata.mDescription = MetaDescriptionTextBox.Text;
            //GetNewChangeList()
            mData.Save();

            if (CoreGlobals.UsingPerforce == true && MarkForAddCheckBox.Checked == true)
            {
               SimpleFileStatus status = CoreGlobals.getPerforce().GetFileStatusSimple(mData.mFileName);
               if (status.InPerforce == false)
               {
                  PerforceChangeList list = CoreGlobals.getPerforce().GetNewChangeList("Auto checkin:Clipart");
                  list.AddFile(mData.mFileName);
                  
                  list.Submitchanges();
               }
            }
            this.DialogResult = DialogResult.OK;
            this.Close();
         }
         catch(System.Exception ex)
         {
            MessageBox.Show(ex.ToString());
         }
      }

      private void button1_Click(object sender, EventArgs e)
      {
         
      }

      private void setExportAll()
      {
         mData.mDoSaveData["CopiedVertexData"] = VertexCheckBox.Checked = true ;
         mData.mDoSaveData["CopiedTextureData"] = checkBox1.Checked = true;
         mData.mDoSaveData["CopiedUnitData"] = checkBox3.Checked = true;
      }

      private void VertexCheckBox_CheckedChanged(object sender, EventArgs e)
      {
         mData.mDoSaveData["CopiedVertexData"] = VertexCheckBox.Checked;
        // mData.SetThumbnail(TerrainGlobals.getTerrainFrontEnd().renderThumbnail(mData.mDoSaveData["CopiedTextureData"]));
        // pictureBox1.Image = mData.mThumbnail;
        // pictureBox1.Invalidate();
      }

      private void checkBox1_CheckedChanged(object sender, EventArgs e)
      {
         mData.mDoSaveData["CopiedTextureData"] = checkBox1.Checked;
         mData.SetThumbnail(TerrainGlobals.getTerrainFrontEnd().renderThumbnail(mData.mDoSaveData["CopiedTextureData"], mData.mDoSaveData["CopiedUnitData"]));
         pictureBox1.Image = mData.mThumbnail;
         pictureBox1.Invalidate();
      }

      private void checkBox3_CheckedChanged(object sender, EventArgs e)
      {
         mData.mDoSaveData["CopiedUnitData"] = checkBox3.Checked;
         mData.SetThumbnail(TerrainGlobals.getTerrainFrontEnd().renderThumbnail(mData.mDoSaveData["CopiedTextureData"], mData.mDoSaveData["CopiedUnitData"]));
         pictureBox1.Image = mData.mThumbnail;
         pictureBox1.Invalidate();
      }

      private void NewClipArt_Load(object sender, EventArgs e)
      {
         setExportAll();
      }
   }
}